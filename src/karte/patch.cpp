// patch.cpp - Karten-Editor: Bezier-Patches und ihre Anzeige-Tesselierung.
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): radiant/patch.cpp
// (Zeilennummern), radiant/patch.h, libs/render.h
// (ArbitraryMeshTriangle_calcTangents).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   Copyright (C) 2001-2006, William Joseph. All Rights Reserved.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/patch.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <forward_list>
#include <limits>

namespace bhed::karte {

namespace {

// BEZIERCURVETREE_MAX_INDEX (patch.h:121)
constexpr std::size_t kKurvenbaumMaxIndex = std::size_t(1) << (std::numeric_limits<std::size_t>::digits - 1);
constexpr std::size_t kMaxUnterteilungsTiefe = 16;   // PATCH_MAX_SUBDIVISION_DEPTH (:119)

// DEGEN_* / SPLIT / AVERAGE (:1648-1655)
constexpr unsigned int DEGEN_0a = 0x01;
constexpr unsigned int DEGEN_1a = 0x02;
constexpr unsigned int DEGEN_2a = 0x04;
constexpr unsigned int DEGEN_0b = 0x08;
constexpr unsigned int DEGEN_1b = 0x10;
constexpr unsigned int DEGEN_2b = 0x20;
constexpr unsigned int SPLIT = 0x40;
constexpr unsigned int AVERAGE = 0x80;

// BezierCurve (patch.h:114-119)
struct Bezierkurve {
    Vektor3f crd;
    Vektor3f left;
    Vektor3f right;
};

// BezierCurve_IsCurved (:80-111)
bool kurveGekruemmt(const Bezierkurve& k, int schwellwert) {
    const Vektor3f vTemp(k.right - k.left);
    Vektor3f v1(k.crd - k.left);
    Vektor3f v2(k.right - k.crd);
    if (v1 == kNullVektor || vTemp == v1) {   // 1->2 == 0 oder 1->2 == 1->3
        return false;
    }
    normieren(v1);
    normieren(v2);
    if (v1 == v2) {
        return false;
    }
    Vektor3f v3(vTemp);
    const double breite = laenge(v3);
    v3 *= 1.0 / breite;   // vector3_scale: Faktor erst nach float
    if (v1 == v3 && v2 == v3) {
        return false;
    }
    const double winkel = std::acos(punkt(v1, v2)) / kPi;
    const double index = breite * winkel;
    return index > static_cast<double>(schwellwert);
}

// BezierInterpolate (:113-117)
void bezierTeilen(Bezierkurve& k) {
    k.left = mitte(k.left, k.crd);
    k.right = mitte(k.crd, k.right);
    k.crd = mitte(k.left, k.right);
}

// subarray_get_degen (:1658-1697)
unsigned int teilfeldEntartung(const PatchPunkt* teil, std::size_t strideU, std::size_t strideV) {
    unsigned int n = 0;
    const PatchPunkt* p1 = teil;
    const PatchPunkt* p2 = p1 + strideU;
    if (p1->vertex == p2->vertex) { n |= DEGEN_0a; }
    p1 = p2;
    p2 = p1 + strideU;
    if (p1->vertex == p2->vertex) { n |= DEGEN_0b; }

    p1 = teil + strideV;
    p2 = p1 + strideU;
    if (p1->vertex == p2->vertex) { n |= DEGEN_1a; }
    p1 = p2;
    p2 = p1 + strideU;
    if (p1->vertex == p2->vertex) { n |= DEGEN_1b; }

    p1 = teil + (strideV << 1);
    p2 = p1 + strideU;
    if (p1->vertex == p2->vertex) { n |= DEGEN_2a; }
    p1 = p2;
    p2 = p1 + strideU;
    if (p1->vertex == p2->vertex) { n |= DEGEN_2b; }
    return n;
}

// BezierInterpolate3 / BezierInterpolate2 (:1706-1719)
void bezier3(const Vektor3f& start, Vektor3f& left, Vektor3f& mid, Vektor3f& right, const Vektor3f& end) {
    left = mitte(start, mid);
    right = mitte(mid, end);
    mid = mitte(left, right);
}
void bezier2(const Vektor2f& start, Vektor2f& left, Vektor2f& mid, Vektor2f& right, const Vektor2f& end) {
    left[0] = zahlMitte(start[0], mid[0]);
    left[1] = zahlMitte(start[1], mid[1]);
    right[0] = zahlMitte(mid[0], end[0]);
    right[1] = zahlMitte(mid[1], end[1]);
    mid[0] = zahlMitte(left[0], right[0]);
    mid[1] = zahlMitte(left[1], right[1]);
}

// vector3_linear_interpolated / vector2_linear_interpolated (:1792-1798)
Vektor3f linear3(const Vektor3f& a, const Vektor3f& b, double t) { return (a * (1.0 - t)) + (b * t); }
Vektor2f linear2(const Vektor2f& a, const Vektor2f& b, double t) { return mal(a, 1.0 - t) + mal(b, t); }

// normalise_safe (:1800-1804)
void normierenSicher(Vektor3f& n) {
    if (!(n == kNullVektor)) { normieren(n); }
}

// QuadraticBezier_evaluate (:1806-1813)
void quadratischBezier(const PatchPunkt& a, const PatchPunkt& b, const PatchPunkt& c, double t, PatchPunkt& punkt,
                       PatchPunkt& left, PatchPunkt& right) {
    left.vertex = linear3(a.vertex, b.vertex, t);
    left.texcoord = linear2(a.texcoord, b.texcoord, t);
    right.vertex = linear3(b.vertex, c.vertex, t);
    right.texcoord = linear2(b.texcoord, c.texcoord, t);
    punkt.vertex = linear3(left.vertex, right.vertex, t);
    punkt.texcoord = linear2(left.texcoord, right.texcoord, t);
}

// ArbitraryMeshTriangle_calcTangents (libs/render.h:1170-1240)
void dreieckTangenten(const NetzPunkt& a, const NetzPunkt& b, const NetzPunkt& c, Vektor3f& s, Vektor3f& t) {
    s = Vektor3f(0, 0, 0);
    t = Vektor3f(0, 0, 0);
    for (std::size_t achse = 0; achse < 3; ++achse) {
        const Vektor3f cross(kreuz(Vektor3f(b.vertex[achse], b.texcoord.x(), b.texcoord.y())
                                       - Vektor3f(a.vertex[achse], a.texcoord.x(), a.texcoord.y()),
                                   Vektor3f(c.vertex[achse], c.texcoord.x(), c.texcoord.y())
                                       - Vektor3f(a.vertex[achse], a.texcoord.x(), a.texcoord.y())));
        if (std::fabs(cross.x()) > 0.000001f) {
            s[achse] = -cross.y() / cross.x();
        }
        if (std::fabs(cross.x()) > 0.000001f) {
            t[achse] = -cross.z() / cross.x();
        }
    }
}

// vertex_assign_ctrl / vertex_clear_normal (:2362-2371)
void punktSetzen(NetzPunkt& v, const PatchPunkt& c) {
    v.vertex = c.vertex;
    v.texcoord = c.texcoord;
}
void normaleLoeschen(NetzPunkt& v) {
    v.normal = Vektor3f(0, 0, 0);
    v.tangent = Vektor3f(0, 0, 0);
    v.bitangent = Vektor3f(0, 0, 0);
}

// tangents_remove_degenerate (:2373-2430)
void entarteteTangentenErsetzen(Vektor3f tangents[6], Vektor2f texTangents[6], unsigned int f) {
    if (f & DEGEN_0a) {
        const std::size_t i = (f & DEGEN_0b) ? ((f & DEGEN_1a) ? ((f & DEGEN_1b) ? ((f & DEGEN_2a) ? 5 : 4) : 3) : 2) : 1;
        tangents[0] = tangents[i];
        texTangents[0] = texTangents[i];
    }
    if (f & DEGEN_0b) {
        const std::size_t i = (f & DEGEN_0a) ? ((f & DEGEN_1b) ? ((f & DEGEN_1a) ? ((f & DEGEN_2b) ? 4 : 5) : 2) : 3) : 0;
        tangents[1] = tangents[i];
        texTangents[1] = texTangents[i];
    }
    if (f & DEGEN_2a) {
        const std::size_t i = (f & DEGEN_2b) ? ((f & DEGEN_1a) ? ((f & DEGEN_1b) ? ((f & DEGEN_0a) ? 1 : 0) : 3) : 2) : 5;
        tangents[4] = tangents[i];
        texTangents[4] = texTangents[i];
    }
    if (f & DEGEN_2b) {
        const std::size_t i = (f & DEGEN_2a) ? ((f & DEGEN_1b) ? ((f & DEGEN_1a) ? ((f & DEGEN_0b) ? 0 : 1) : 2) : 3) : 4;
        tangents[5] = tangents[i];
        texTangents[5] = texTangents[i];
    }
}

// bestTangents00/01/10/11 (:2432-2546)
void besteTangenten00(unsigned int f, double dot, double length, std::size_t& i0, std::size_t& i1) {
    if (std::fabs(dot + length) < 0.001) {   // entgegengesetzt = entartet
        if (!(f & DEGEN_1a)) { i0 = 2; i1 = 0; }
        else if (!(f & DEGEN_0b)) { i0 = 0; i1 = 1; }
        else { i0 = 1; i1 = 0; }
    } else if (std::fabs(dot - length) < 0.001) {   // gleiche Richtung = entartet
        if (f & DEGEN_0b) { i0 = 0; i1 = 1; }
        else { i0 = 1; i1 = 0; }
    }
}
void besteTangenten01(unsigned int f, double dot, double length, std::size_t& i0, std::size_t& i1) {
    if (std::fabs(dot - length) < 0.001) {
        if (!(f & DEGEN_1a)) { i0 = 2; i1 = 1; }
        else if (!(f & DEGEN_2b)) { i0 = 4; i1 = 0; }
        else { i0 = 5; i1 = 1; }
    } else if (std::fabs(dot + length) < 0.001) {
        if (f & DEGEN_2b) { i0 = 4; i1 = 0; }
        else { i0 = 5; i1 = 1; }
    }
}
void besteTangenten10(unsigned int f, double dot, double length, std::size_t& i0, std::size_t& i1) {
    if (std::fabs(dot - length) < 0.001) {
        if (!(f & DEGEN_1b)) { i0 = 3; i1 = 4; }
        else if (!(f & DEGEN_0a)) { i0 = 1; i1 = 5; }
        else { i0 = 0; i1 = 4; }
    } else if (std::fabs(dot + length) < 0.001) {
        if (f & DEGEN_0a) { i0 = 1; i1 = 5; }
        else { i0 = 0; i1 = 4; }
    }
}
void besteTangenten11(unsigned int f, double dot, double length, std::size_t& i0, std::size_t& i1) {
    if (std::fabs(dot + length) < 0.001) {
        if (!(f & DEGEN_1b)) { i0 = 3; i1 = 5; }
        else if (!(f & DEGEN_2a)) { i0 = 5; i1 = 4; }
        else { i0 = 4; i1 = 5; }
    } else if (std::fabs(dot - length) < 0.001) {
        if (f & DEGEN_2a) { i0 = 5; i1 = 4; }
        else { i0 = 4; i1 = 5; }
    }
}

bool namenGleichOhneFall(const std::string& a, const std::string& b) {   // shader_equal
    if (a.size() != b.size()) { return false; }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) { return false; }
    }
    return true;
}

}  // namespace

// BezierCurveTree (patch.h:123-128)
struct Patch::Kurvenbaum {
    std::size_t index = 0;
    std::unique_ptr<Kurvenbaum> left;
    std::unique_ptr<Kurvenbaum> right;
    [[nodiscard]] bool blatt() const { return left == nullptr && right == nullptr; }   // BezierCurveTree_isLeaf
};

namespace {

// BezierCurveTree_FromCurveList (:121-154). Nach der ersten gekruemmten Kurve
// werden alle folgenden geteilt; die davor bleiben aussen vor (wie NRC).
template<typename Baum>
void kurvenbaumAusListe(Baum* baum, std::forward_list<Bezierkurve>& liste, int schwellwert, std::size_t tiefe = 0) {
    std::forward_list<Bezierkurve> linksListe;
    std::forward_list<Bezierkurve> rechtsListe;
    bool teilen = false;
    for (Bezierkurve& k : liste) {
        if (teilen || kurveGekruemmt(k, schwellwert)) {
            teilen = true;
            Bezierkurve& links = linksListe.emplace_front();
            Bezierkurve& rechts = rechtsListe.emplace_front();
            links.left = k.left;
            rechts.right = k.right;
            bezierTeilen(k);
            links.crd = k.left;
            rechts.crd = k.right;
            links.right = k.crd;
            rechts.left = k.crd;
        }
    }
    if (!linksListe.empty() && !rechtsListe.empty() && tiefe != kMaxUnterteilungsTiefe) {
        baum->left = std::make_unique<Baum>();
        baum->right = std::make_unique<Baum>();
        kurvenbaumAusListe(baum->left.get(), linksListe, schwellwert, tiefe + 1);
        kurvenbaumAusListe(baum->right.get(), rechtsListe, schwellwert, tiefe + 1);
    } else {
        baum->left.reset();
        baum->right.reset();
    }
}

// BezierCurveTree_Setup (:63-78)
template<typename Baum>
std::size_t kurvenbaumIndizes(Baum* k, std::size_t index, std::size_t stride) {
    if (k) {
        if (k->left && k->right) {
            index = kurvenbaumIndizes(k->left.get(), index, stride);
            k->index = index * stride;
            index++;
            index = kurvenbaumIndizes(k->right.get(), index, stride);
        } else {
            k->index = kKurvenbaumMaxIndex;
        }
    }
    return index;
}

struct PatchMemento final : Memento {   // Patch::SavedState
    std::size_t width = 0, height = 0;
    std::string shader;
    std::vector<PatchPunkt> ctrl;
    bool patchDef3 = false;
    std::size_t unterteilungX = 0, unterteilungY = 0;
};

}  // namespace

Patch::Patch() = default;
Patch::~Patch() = default;

std::shared_ptr<Patch> Patch::kopie() const {
    auto p = std::make_shared<Patch>();
    p->schicht = schicht;
    p->m_shader = m_shader;
    p->m_width = m_width;
    p->m_height = m_height;
    p->m_ctrl = m_ctrl;
    p->patchDef3 = patchDef3;
    p->unterteilungX = unterteilungX;
    p->unterteilungY = unterteilungY;
    p->punkteGeaendert();
    return p;
}

// setDims (:156-186)
void Patch::setzeGroesse(std::size_t w, std::size_t h) {
    if ((w % 2) == 0) {
        w -= 1;
    }
    if (w > kMaxPatchBreite) {
        w = kMaxPatchBreite;
    } else if (w < kMinPatchBreite) {
        w = kMinPatchBreite;
    }
    if ((h % 2) == 0) {
        m_height -= 1;   // so in NRC (patch.cpp:170): m_height, nicht h
    }
    if (h > kMaxPatchHoehe) {
        h = kMaxPatchHoehe;
    } else if (h < kMinPatchHoehe) {
        h = kMinPatchHoehe;
    }
    m_width = w;
    m_height = h;
    if (m_width * m_height != m_ctrl.size()) {
        m_ctrl.resize(m_width * m_height);
        punktWahl.assign(m_ctrl.size(), 0);   // onAllocate
    }
}

// SetShader (patch.h:825-847)
void Patch::setzeShader(const std::string& name) {
    if (namenGleichOhneFall(m_shader, name)) {
        return;
    }
    sichern();
    m_shader = name;
}

// isValid (:196-213)
bool Patch::gueltig() const {
    if (!m_width || !m_height) {
        return false;
    }
    // Stufe 8d (kartefuzz unter UBSan): in NRC haben die Kontrollpunkte immer width*height Eintraege;
    // hier koennen Groesse und Punkte getrennt gesetzt werden - passen sie nicht, liefe kurvenBauen
    // ueber das Feld hinaus (bei leerem Feld ueber einen Nullzeiger).
    if (m_ctrl.size() != static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height)) {
        return false;
    }
    for (const auto& c : m_ctrl) {
        if (!(c.vertex.x() == c.vertex.x()) || !(c.vertex.y() == c.vertex.y()) || !(c.vertex.z() == c.vertex.z())
            || !(c.texcoord.x() == c.texcoord.x()) || !(c.texcoord.y() == c.texcoord.y())) {
            return false;   // NRC: "patch has invalid control points"
        }
    }
    return true;
}

// controlPointsChanged -> UpdateCachedData (:215-290) + AccumulateBBox (:765-776)
namespace {
int g_patchSchwelle = kPatchUnterteilungVorgabe;   // g_PatchSubdivideThreshold
}  // namespace

int patchUnterteilungSchwelle() { return g_patchSchwelle; }
void setzePatchUnterteilungSchwelle(int s) { g_patchSchwelle = s; }

void Patch::punkteGeaendert(int schwellwert) {
    if (schwellwert < 0) { schwellwert = g_patchSchwelle; }
    if (!gueltig()) {
        m_tess = PatchTesselierung();
        m_kurvenU.clear();
        m_kurvenV.clear();
        m_grenzen = Kasten();
        return;
    }
    kurvenBauen(true, schwellwert);
    kurvenBauen(false, schwellwert);
    netzBauen();
    m_grenzen = Kasten();
    for (const auto& c : m_ctrl) {
        kastenErweiternSicher(m_grenzen, c.vertex);
    }
}

// BuildTesselationCurves (:2244-2360)
void Patch::kurvenBauen(bool zeilen, int schwellwert) {
    std::size_t arrayStride = 0, laenge = 0, quer = 0, strideU = 0, strideV = 0;
    if (zeilen) {   // ROW
        arrayStride = 1;
        laenge = (m_width - 1) >> 1;
        quer = m_height;
        strideU = 1;
        strideV = m_width;
        if (!patchDef3) { m_kurvenU.clear(); }
    } else {   // COL
        arrayStride = m_tess.nArrayWidth;
        laenge = (m_height - 1) >> 1;
        quer = m_width;
        strideU = m_width;
        strideV = 1;
        if (!patchDef3) { m_kurvenV.clear(); }
    }

    std::vector<std::size_t> arrayLength(laenge);
    std::vector<std::unique_ptr<Kurvenbaum>> baeume(laenge);
    std::size_t nArrayLength = 1;

    if (patchDef3) {
        for (auto& l : arrayLength) {
            l = zeilen ? unterteilungX : unterteilungY;
            nArrayLength += l;
        }
    } else {
        // Liste der Kontrollkurven je Teilfeld-Spalte, adaptiv unterteilt,
        // zusammengefasst als Binaerbaum
        for (std::size_t i = 0; i != laenge; ++i) {
            const PatchPunkt* p1 = m_ctrl.data() + (i * 2 * strideU);
            std::forward_list<Bezierkurve> liste;
            for (std::size_t j = 0; j < quer; j += 2) {
                {   // direkt aus einer Zeile Kontrollpunkte
                    Bezierkurve& k = liste.emplace_front();
                    k.crd = (p1 + strideU)->vertex;
                    k.left = p1->vertex;
                    k.right = (p1 + (strideU << 1))->vertex;
                }
                if (j + 2 >= quer) {
                    break;
                }
                // Stufe 8d (kartefuzz unter UBSan): p2/p3 erst hier - in der letzten Zeile zeigten sie
                // zwei Zeilen hinter das Feld (undefiniert, auch ohne Zugriff). NRC rechnet sie vorher.
                const PatchPunkt* p2 = p1 + strideV;
                const PatchPunkt* p3 = p2 + strideV;
                {   // aus drei Spalten interpoliert
                    Bezierkurve& k = liste.emplace_front();
                    k.crd = mitte((p1 + strideU)->vertex, (p3 + strideU)->vertex);
                    k.left = mitte(p1->vertex, p3->vertex);
                    k.right = mitte((p1 + (strideU << 1))->vertex, (p3 + (strideU << 1))->vertex);
                    k.crd = mitte(k.crd, (p2 + strideU)->vertex);
                    k.left = mitte(k.left, p2->vertex);
                    k.right = mitte(k.right, (p2 + (strideU << 1))->vertex);
                }
                p1 = p3;
            }
            baeume[i] = std::make_unique<Kurvenbaum>();
            kurvenbaumAusListe(baeume[i].get(), liste, schwellwert);
            // Teilfeldbreite und Gesamtbreite
            arrayLength[i] = kurvenbaumIndizes(baeume[i].get(), nArrayLength, arrayStride) - (nArrayLength - 1);
            nArrayLength += arrayLength[i];
        }
    }

    if (zeilen) {
        m_tess.nArrayWidth = nArrayLength;
        m_tess.arrayWidth = std::move(arrayLength);
        if (!patchDef3) { m_kurvenU = std::move(baeume); }
    } else {
        m_tess.nArrayHeight = nArrayLength;
        m_tess.arrayHeight = std::move(arrayLength);
        if (!patchDef3) { m_kurvenV = std::move(baeume); }
    }
}

// accumulateVertexTangentSpace (:2548-2574)
void Patch::tangentenSammeln(std::size_t index, Vektor3f tangentX[6], Vektor3f tangentY[6], Vektor2f tangentS[6],
                             Vektor2f tangentT[6], std::size_t index0, std::size_t index1) {
    {
        const Vektor3f normal(kreuz(tangentX[index0], tangentY[index1]));
        if (!(normal == kNullVektor)) {
            m_tess.vertices[index].normal += normiert(normal);
        }
    }
    {
        NetzPunkt a, b, c;
        a.vertex = Vektor3f(0, 0, 0);
        a.texcoord = Vektor2f(0, 0);
        b.vertex = tangentX[index0];
        b.texcoord = tangentS[index0];
        c.vertex = tangentY[index1];
        c.texcoord = tangentT[index1];
        Vektor3f s, t;
        dreieckTangenten(a, b, c, s, t);
        if (!(s == kNullVektor)) {
            m_tess.vertices[index].tangent += normiert(s);
        }
        if (!(t == kNullVektor)) {
            m_tess.vertices[index].bitangent += normiert(t);
        }
    }
}

// TesselateSubMatrixFixed (:1815-1898)
void Patch::teilfeldFest(NetzPunkt* vertices, std::size_t strideX, std::size_t strideY, unsigned int flagsX,
                         unsigned int flagsY, PatchPunkt* teil[3][3]) {
    const double incrementU = 1.0 / static_cast<double>(unterteilungX);
    const double incrementV = 1.0 / static_cast<double>(unterteilungY);
    const std::size_t width = unterteilungX + 1;
    const std::size_t height = unterteilungY + 1;

    for (std::size_t i = 0; i != width; ++i) {
        const double tU = (i + 1 == width) ? 1 : static_cast<double>(i) * incrementU;
        PatchPunkt pointX[3];
        PatchPunkt leftX[3];
        PatchPunkt rightX[3];
        quadratischBezier(*teil[0][0], *teil[0][1], *teil[0][2], tU, pointX[0], leftX[0], rightX[0]);
        quadratischBezier(*teil[1][0], *teil[1][1], *teil[1][2], tU, pointX[1], leftX[1], rightX[1]);
        quadratischBezier(*teil[2][0], *teil[2][1], *teil[2][2], tU, pointX[2], leftX[2], rightX[2]);

        NetzPunkt* p = vertices + i * strideX;
        for (std::size_t j = 0; j != height; ++j) {
            if ((j == 0 || j + 1 == height) && (i == 0 || i + 1 == width)) {
                // Ecken stehen schon (vertex_assign_ctrl)
            } else {
                const double tV = (j + 1 == height) ? 1 : static_cast<double>(j) * incrementV;
                PatchPunkt pointY[3];
                PatchPunkt leftY[3];
                PatchPunkt rightY[3];
                quadratischBezier(*teil[0][0], *teil[1][0], *teil[2][0], tV, pointY[0], leftY[0], rightY[0]);
                quadratischBezier(*teil[0][1], *teil[1][1], *teil[2][1], tV, pointY[1], leftY[1], rightY[1]);
                quadratischBezier(*teil[0][2], *teil[1][2], *teil[2][2], tV, pointY[2], leftY[2], rightY[2]);

                PatchPunkt punkt;
                PatchPunkt left;
                PatchPunkt right;
                quadratischBezier(pointX[0], pointX[1], pointX[2], tV, punkt, left, right);
                PatchPunkt up;
                PatchPunkt down;
                quadratischBezier(pointY[0], pointY[1], pointY[2], tU, punkt, up, down);

                p->vertex = punkt.vertex;
                p->texcoord = punkt.texcoord;

                NetzPunkt a, b, c;
                a.vertex = left.vertex;
                a.texcoord = left.texcoord;
                b.vertex = right.vertex;
                b.texcoord = right.texcoord;
                if (i != 0) {
                    c.vertex = up.vertex;
                    c.texcoord = up.texcoord;
                } else {
                    c.vertex = down.vertex;
                    c.texcoord = down.texcoord;
                }

                const Vektor3f normal = normiert(kreuz(right.vertex - left.vertex, up.vertex - down.vertex));

                Vektor3f tangent, bitangent;
                dreieckTangenten(a, b, c, tangent, bitangent);
                normieren(tangent);
                normieren(bitangent);

                if (((flagsX & AVERAGE) != 0 && i == 0) || ((flagsY & AVERAGE) != 0 && j == 0)) {
                    p->normal = normiert(p->normal + normal);
                    p->tangent = normiert(p->tangent + tangent);
                    p->bitangent = normiert(p->bitangent + bitangent);
                } else {
                    p->normal = normal;
                    p->tangent = tangent;
                    p->bitangent = bitangent;
                }
            }
            p += strideY;
        }
    }
}

// TesselateSubMatrix (:1900-2242)
void Patch::teilfeld(const Kurvenbaum* BX, const Kurvenbaum* BY, std::size_t offStartX, std::size_t offStartY,
                     std::size_t offEndX, std::size_t offEndY, std::size_t nFlagsX, std::size_t nFlagsY,
                     const Vektor3f& left, const Vektor3f& mid, const Vektor3f& right, const Vektor2f& texLeft,
                     const Vektor2f& texMid, const Vektor2f& texRight, bool bTranspose) {
    std::vector<NetzPunkt>& V = m_tess.vertices;
    Vektor3f tmp;
    Vektor3f vertex_0_0, vertex_0_1, vertex_1_0, vertex_1_1, vertex_2_0, vertex_2_1;
    Vektor2f texTmp;
    Vektor2f texcoord_0_0, texcoord_0_1, texcoord_1_0, texcoord_1_1, texcoord_2_0, texcoord_2_1;

    {
        // Texturkoordinaten
        bezier2(V[offStartX + offStartY].texcoord, texcoord_0_0, V[BX->index + offStartY].texcoord, texcoord_0_1,
                V[offEndX + offStartY].texcoord);
        bezier2(V[offStartX + offEndY].texcoord, texcoord_2_0, V[BX->index + offEndY].texcoord, texcoord_2_1,
                V[offEndX + offEndY].texcoord);
        texTmp = texMid;
        bezier2(texLeft, texcoord_1_0, texTmp, texcoord_1_1, texRight);

        if (!BY->blatt()) {
            V[BX->index + BY->index].texcoord = texTmp;
        }
        if (!BX->left->blatt()) {
            V[BX->left->index + offStartY].texcoord = texcoord_0_0;
            V[BX->left->index + offEndY].texcoord = texcoord_2_0;
            if (!BY->blatt()) {
                V[BX->left->index + BY->index].texcoord = texcoord_1_0;
            }
        }
        if (!BX->right->blatt()) {
            V[BX->right->index + offStartY].texcoord = texcoord_0_1;
            V[BX->right->index + offEndY].texcoord = texcoord_2_1;
            if (!BY->blatt()) {
                V[BX->right->index + BY->index].texcoord = texcoord_1_1;
            }
        }

        // Punkte
        bezier3(V[offStartX + offStartY].vertex, vertex_0_0, V[BX->index + offStartY].vertex, vertex_0_1,
                V[offEndX + offStartY].vertex);
        bezier3(V[offStartX + offEndY].vertex, vertex_2_0, V[BX->index + offEndY].vertex, vertex_2_1,
                V[offEndX + offEndY].vertex);
        tmp = mid;
        bezier3(left, vertex_1_0, tmp, vertex_1_1, right);

        if (!BY->blatt()) {
            V[BX->index + BY->index].vertex = tmp;
        }
        if (!BX->left->blatt()) {
            V[BX->left->index + offStartY].vertex = vertex_0_0;
            V[BX->left->index + offEndY].vertex = vertex_2_0;
            if (!BY->blatt()) {
                V[BX->left->index + BY->index].vertex = vertex_1_0;
            }
        }
        if (!BX->right->blatt()) {
            V[BX->right->index + offStartY].vertex = vertex_0_1;
            V[BX->right->index + offEndY].vertex = vertex_2_1;
            if (!BY->blatt()) {
                V[BX->right->index + BY->index].vertex = vertex_1_1;
            }
        }

        // Normalen
        if (nFlagsX & SPLIT) {
            NetzPunkt a, b, c;
            Vektor3f tangentU;
            if (!(nFlagsX & DEGEN_0a) || !(nFlagsX & DEGEN_0b)) {
                tangentU = vertex_0_1 - vertex_0_0;
                a.vertex = vertex_0_0;
                a.texcoord = texcoord_0_0;
                c.vertex = vertex_0_1;
                c.texcoord = texcoord_0_1;
            } else if (!(nFlagsX & DEGEN_1a) || !(nFlagsX & DEGEN_1b)) {
                tangentU = vertex_1_1 - vertex_1_0;
                a.vertex = vertex_1_0;
                a.texcoord = texcoord_1_0;
                c.vertex = vertex_1_1;
                c.texcoord = texcoord_1_1;
            } else {
                tangentU = vertex_2_1 - vertex_2_0;
                a.vertex = vertex_2_0;
                a.texcoord = texcoord_2_0;
                c.vertex = vertex_2_1;
                c.texcoord = texcoord_2_1;
            }

            Vektor3f tangentV;
            if ((nFlagsY & DEGEN_0a) && (nFlagsY & DEGEN_1a) && (nFlagsY & DEGEN_2a)) {
                tangentV = V[BX->index + offEndY].vertex - tmp;
                b.vertex = tmp;
                b.texcoord = texTmp;
            } else {
                tangentV = tmp - V[BX->index + offStartY].vertex;
                b.vertex = tmp;
                b.texcoord = texTmp;
            }

            Vektor3f normal, s, t;
            NetzPunkt& v = V[offStartY + BX->index];
            Vektor3f& p = v.normal;
            Vektor3f& ps = v.tangent;
            Vektor3f& pt = v.bitangent;

            if (bTranspose) {
                normal = kreuz(tangentV, tangentU);
            } else {
                normal = kreuz(tangentU, tangentV);
            }
            normierenSicher(normal);

            dreieckTangenten(a, b, c, s, t);
            normierenSicher(s);
            normierenSicher(t);

            if (nFlagsX & AVERAGE) {
                p = normiert(p + normal);
                ps = normiert(ps + s);
                pt = normiert(pt + t);
            } else {
                p = normal;
                ps = s;
                pt = t;
            }
        }

        {
            NetzPunkt a, b, c;
            Vektor3f tangentU;
            if (!(nFlagsX & DEGEN_2a) || !(nFlagsX & DEGEN_2b)) {
                tangentU = vertex_2_1 - vertex_2_0;
                a.vertex = vertex_2_0;
                a.texcoord = texcoord_2_0;
                c.vertex = vertex_2_1;
                c.texcoord = texcoord_2_1;
            } else if (!(nFlagsX & DEGEN_1a) || !(nFlagsX & DEGEN_1b)) {
                tangentU = vertex_1_1 - vertex_1_0;
                a.vertex = vertex_1_0;
                a.texcoord = texcoord_1_0;
                c.vertex = vertex_1_1;
                c.texcoord = texcoord_1_1;
            } else {
                tangentU = vertex_0_1 - vertex_0_0;
                a.vertex = vertex_0_0;
                a.texcoord = texcoord_0_0;
                c.vertex = vertex_0_1;
                c.texcoord = texcoord_0_1;
            }

            Vektor3f tangentV;
            if ((nFlagsY & DEGEN_0b) && (nFlagsY & DEGEN_1b) && (nFlagsY & DEGEN_2b)) {
                tangentV = tmp - V[BX->index + offStartY].vertex;
                b.vertex = tmp;
                b.texcoord = texTmp;
            } else {
                tangentV = V[BX->index + offEndY].vertex - tmp;
                b.vertex = tmp;
                b.texcoord = texTmp;
            }

            NetzPunkt& v = V[offEndY + BX->index];
            Vektor3f& p = v.normal;
            Vektor3f& ps = v.tangent;
            Vektor3f& pt = v.bitangent;

            if (bTranspose) {
                p = kreuz(tangentV, tangentU);
            } else {
                p = kreuz(tangentU, tangentV);
            }
            normierenSicher(p);

            dreieckTangenten(a, b, c, ps, pt);
            normierenSicher(ps);
            normierenSicher(pt);
        }
    }

    std::size_t newFlagsX = 0;
    std::size_t newFlagsY = 0;

    if ((nFlagsX & DEGEN_0a) && (nFlagsX & DEGEN_0b)) { newFlagsX |= DEGEN_0a; newFlagsX |= DEGEN_0b; }
    if ((nFlagsX & DEGEN_1a) && (nFlagsX & DEGEN_1b)) { newFlagsX |= DEGEN_1a; newFlagsX |= DEGEN_1b; }
    if ((nFlagsX & DEGEN_2a) && (nFlagsX & DEGEN_2b)) { newFlagsX |= DEGEN_2a; newFlagsX |= DEGEN_2b; }
    if ((nFlagsY & DEGEN_0a) && (nFlagsY & DEGEN_1a) && (nFlagsY & DEGEN_2a)) {
        newFlagsY |= DEGEN_0a; newFlagsY |= DEGEN_1a; newFlagsY |= DEGEN_2a;
    }
    if ((nFlagsY & DEGEN_0b) && (nFlagsY & DEGEN_1b) && (nFlagsY & DEGEN_2b)) {
        newFlagsY |= DEGEN_0b; newFlagsY |= DEGEN_1b; newFlagsY |= DEGEN_2b;
    }

    newFlagsX |= (nFlagsX & SPLIT);
    newFlagsX |= (nFlagsX & AVERAGE);

    if (!BY->blatt()) {
        {
            const std::size_t nTemp = newFlagsY;
            if ((nFlagsY & DEGEN_0a) && (nFlagsY & DEGEN_0b)) {
                newFlagsY |= DEGEN_0a;
                newFlagsY |= DEGEN_0b;
            }
            newFlagsY |= (nFlagsY & SPLIT);
            newFlagsY |= (nFlagsY & AVERAGE);

            Vektor3f& p = V[BX->index + BY->index].vertex;
            const Vektor3f vTemp(p);
            Vektor2f& p2 = V[BX->index + BY->index].texcoord;
            const Vektor2f stTemp(p2);

            teilfeld(BY, BX->left.get(), offStartY, offStartX, offEndY, BX->index, newFlagsY, newFlagsX, vertex_0_0,
                     vertex_1_0, vertex_2_0, texcoord_0_0, texcoord_1_0, texcoord_2_0, !bTranspose);

            newFlagsY = nTemp;
            // p und p2 koennen durch die Rekursion nicht verschoben sein (vertices
            // waechst nicht), aber neu adressieren ist sicherer
            V[BX->index + BY->index].vertex = vTemp;
            V[BX->index + BY->index].texcoord = stTemp;
        }

        if ((nFlagsY & DEGEN_2a) && (nFlagsY & DEGEN_2b)) {
            newFlagsY |= DEGEN_2a;
            newFlagsY |= DEGEN_2b;
        }

        teilfeld(BY, BX->right.get(), offStartY, BX->index, offEndY, offEndX, newFlagsY, newFlagsX, vertex_0_1,
                 vertex_1_1, vertex_2_1, texcoord_0_1, texcoord_1_1, texcoord_2_1, !bTranspose);
    } else {
        if (!BX->left->blatt()) {
            teilfeld(BX->left.get(), BY, offStartX, offStartY, BX->index, offEndY, newFlagsX, newFlagsY, left, vertex_1_0,
                     tmp, texLeft, texcoord_1_0, texTmp, bTranspose);
        }
        if (!BX->right->blatt()) {
            teilfeld(BX->right.get(), BY, BX->index, offStartY, offEndX, offEndY, newFlagsX, newFlagsY, tmp, vertex_1_1,
                     right, texTmp, texcoord_1_1, texRight, bTranspose);
        }
    }
}

// BuildVertexArray (:2578-2867)
void Patch::drahtgitter(std::vector<Vektor3f>& paare) const {
    const auto zug = [&paare](const NetzPunkt* p, std::size_t schritt, std::size_t anzahl) {
        for (std::size_t i = 1; i < anzahl; ++i) {
            paare.push_back(p[(i - 1) * schritt].vertex);
            paare.push_back(p[i * schritt].vertex);
        }
    };
    const auto& v = m_tess.vertices;
    if (v.empty()) { return; }
    if (patchDef3) {
        // RenderablePatchFixedWireframe: GL_QUAD_STRIP je Streifen, im 2D als
        // Umriss jedes Vierecks (2i, 2i+1, 2i+3, 2i+2).
        for (std::size_t s = 0; s < m_tess.numStrips; ++s) {
            const unsigned int* idx = m_tess.indices.data() + s * m_tess.lenStrips;
            for (std::size_t q = 0; q + 3 < m_tess.lenStrips; q += 2) {
                const unsigned int a = idx[q];
                const unsigned int b = idx[q + 1];
                const unsigned int c = idx[q + 3];
                const unsigned int d = idx[q + 2];
                for (const auto& [x, y] : {std::pair{a, b}, std::pair{b, c}, std::pair{c, d}, std::pair{d, a}}) {
                    paare.push_back(v[x].vertex);
                    paare.push_back(v[y].vertex);
                }
            }
        }
        return;
    }
    // RenderablePatchWireframe: Zeilen (Kurvenbaeume V) ...
    std::size_t n = 0;
    for (std::size_t i = 0; i <= m_kurvenV.size(); ++i) {
        zug(&v[n], 1, m_tess.nArrayWidth);
        if (i == m_kurvenV.size()) { break; }
        if (!m_kurvenV[i]->blatt()) { zug(&v[m_kurvenV[i]->index], 1, m_tess.nArrayWidth); }
        n += m_tess.arrayHeight[i] * m_tess.nArrayWidth;
    }
    // ... und Spalten (Kurvenbaeume U), Schritt eine Zeile.
    std::size_t p = 0;
    for (std::size_t i = 0; i <= m_kurvenU.size(); ++i) {
        zug(&v[p], m_tess.nArrayWidth, m_tess.nArrayHeight);
        if (i == m_kurvenU.size()) { break; }
        if (!m_kurvenU[i]->blatt()) { zug(&v[m_kurvenU[i]->index], m_tess.nArrayWidth, m_tess.nArrayHeight); }
        p += m_tess.arrayWidth[i];
    }
}

void Patch::netzBauen() {
    const std::size_t strideU = 1;
    const std::size_t strideV = m_width;
    const std::size_t numElems = m_tess.nArrayWidth * m_tess.nArrayHeight;
    const bool bWidthStrips = (m_tess.nArrayWidth >= m_tess.nArrayHeight);

    // Array::resize: neue (leere) Elemente nur bei anderer Groesse
    if (m_tess.vertices.size() != numElems) {
        m_tess.vertices.assign(numElems, NetzPunkt{});
    }
    m_tess.indices.assign(m_tess.nArrayWidth * 2 * (m_tess.nArrayHeight - 1), 0);

    // Streifen-Indizes
    if (bWidthStrips) {
        m_tess.numStrips = m_tess.nArrayHeight - 1;
        m_tess.lenStrips = m_tess.nArrayWidth * 2;
        for (std::size_t i = 0; i < m_tess.nArrayWidth; ++i) {
            for (std::size_t j = 0; j < m_tess.numStrips; ++j) {
                m_tess.indices[(j * m_tess.lenStrips) + i * 2] = static_cast<unsigned int>(j * m_tess.nArrayWidth + i);
                m_tess.indices[(j * m_tess.lenStrips) + i * 2 + 1] = static_cast<unsigned int>((j + 1) * m_tess.nArrayWidth + i);
            }
        }
    } else {
        m_tess.numStrips = m_tess.nArrayWidth - 1;
        m_tess.lenStrips = m_tess.nArrayHeight * 2;
        for (std::size_t i = 0; i < m_tess.nArrayHeight; ++i) {
            for (std::size_t j = 0; j < m_tess.numStrips; ++j) {
                m_tess.indices[(j * m_tess.lenStrips) + i * 2] =
                    static_cast<unsigned int>(((m_tess.nArrayHeight - 1) - i) * m_tess.nArrayWidth + j);
                m_tess.indices[(j * m_tess.lenStrips) + i * 2 + 1] =
                    static_cast<unsigned int>(((m_tess.nArrayHeight - 1) - i) * m_tess.nArrayWidth + j + 1);
            }
        }
    }

    PatchPunkt* pCtrl = m_ctrl.data();
    for (std::size_t j = 0, offStartY = 0; j + 1 < m_height; j += 2, pCtrl += (strideU + strideV)) {
        // Versatz dieses Teilfelds
        const bool leafY = patchDef3 ? false : m_kurvenV[j >> 1]->blatt();
        const std::size_t offMidY = patchDef3 ? 0 : m_kurvenV[j >> 1]->index;
        const std::size_t widthY = m_tess.arrayHeight[j >> 1] * m_tess.nArrayWidth;
        const std::size_t offEndY = offStartY + widthY;

        for (std::size_t i = 0, offStartX = 0; i + 1 < m_width; i += 2, pCtrl += (strideU << 1)) {
            const bool leafX = patchDef3 ? false : m_kurvenU[i >> 1]->blatt();
            const std::size_t offMidX = patchDef3 ? 0 : m_kurvenU[i >> 1]->index;
            const std::size_t widthX = m_tess.arrayWidth[i >> 1];
            const std::size_t offEndX = offStartX + widthX;

            PatchPunkt* teil[3][3];
            teil[0][0] = pCtrl;
            teil[0][1] = teil[0][0] + strideU;
            teil[0][2] = teil[0][1] + strideU;
            teil[1][0] = teil[0][0] + strideV;
            teil[1][1] = teil[1][0] + strideU;
            teil[1][2] = teil[1][1] + strideU;
            teil[2][0] = teil[1][0] + strideV;
            teil[2][1] = teil[2][0] + strideU;
            teil[2][2] = teil[2][1] + strideU;

            // Punkte auf dem Patch
            if (i == 0 && j == 0) { normaleLoeschen(m_tess.vertices[offStartX + offStartY]); }
            punktSetzen(m_tess.vertices[offStartX + offStartY], *teil[0][0]);
            if (j == 0) { normaleLoeschen(m_tess.vertices[offEndX + offStartY]); }
            punktSetzen(m_tess.vertices[offEndX + offStartY], *teil[0][2]);
            if (i == 0) { normaleLoeschen(m_tess.vertices[offStartX + offEndY]); }
            punktSetzen(m_tess.vertices[offStartX + offEndY], *teil[2][0]);
            normaleLoeschen(m_tess.vertices[offEndX + offEndY]);
            punktSetzen(m_tess.vertices[offEndX + offEndY], *teil[2][2]);

            if (!patchDef3) {
                // die uebrigen Kontrollpunkte
                if (!leafX) {
                    punktSetzen(m_tess.vertices[offMidX + offStartY], *teil[0][1]);
                    punktSetzen(m_tess.vertices[offMidX + offEndY], *teil[2][1]);
                }
                if (!leafY) {
                    punktSetzen(m_tess.vertices[offStartX + offMidY], *teil[1][0]);
                    punktSetzen(m_tess.vertices[offEndX + offMidY], *teil[1][2]);
                    if (!leafX) {
                        punktSetzen(m_tess.vertices[offMidX + offMidY], *teil[1][1]);
                    }
                }
            }

            // alle 12 Kanten auf Entartung pruefen
            unsigned int nFlagsX = teilfeldEntartung(pCtrl, strideU, strideV);
            unsigned int nFlagsY = teilfeldEntartung(pCtrl, strideV, strideU);
            Vektor3f tangentX[6], tangentY[6];
            Vektor2f tangentS[6], tangentT[6];

            if (!(nFlagsX & DEGEN_0a)) { tangentX[0] = teil[0][1]->vertex - teil[0][0]->vertex; tangentS[0] = teil[0][1]->texcoord - teil[0][0]->texcoord; }
            if (!(nFlagsX & DEGEN_0b)) { tangentX[1] = teil[0][2]->vertex - teil[0][1]->vertex; tangentS[1] = teil[0][2]->texcoord - teil[0][1]->texcoord; }
            if (!(nFlagsX & DEGEN_1a)) { tangentX[2] = teil[1][1]->vertex - teil[1][0]->vertex; tangentS[2] = teil[1][1]->texcoord - teil[1][0]->texcoord; }
            if (!(nFlagsX & DEGEN_1b)) { tangentX[3] = teil[1][2]->vertex - teil[1][1]->vertex; tangentS[3] = teil[1][2]->texcoord - teil[1][1]->texcoord; }
            if (!(nFlagsX & DEGEN_2a)) { tangentX[4] = teil[2][1]->vertex - teil[2][0]->vertex; tangentS[4] = teil[2][1]->texcoord - teil[2][0]->texcoord; }
            if (!(nFlagsX & DEGEN_2b)) { tangentX[5] = teil[2][2]->vertex - teil[2][1]->vertex; tangentS[5] = teil[2][2]->texcoord - teil[2][1]->texcoord; }

            if (!(nFlagsY & DEGEN_0a)) { tangentY[0] = teil[1][0]->vertex - teil[0][0]->vertex; tangentT[0] = teil[1][0]->texcoord - teil[0][0]->texcoord; }
            if (!(nFlagsY & DEGEN_0b)) { tangentY[1] = teil[2][0]->vertex - teil[1][0]->vertex; tangentT[1] = teil[2][0]->texcoord - teil[1][0]->texcoord; }
            if (!(nFlagsY & DEGEN_1a)) { tangentY[2] = teil[1][1]->vertex - teil[0][1]->vertex; tangentT[2] = teil[1][1]->texcoord - teil[0][1]->texcoord; }
            if (!(nFlagsY & DEGEN_1b)) { tangentY[3] = teil[2][1]->vertex - teil[1][1]->vertex; tangentT[3] = teil[2][1]->texcoord - teil[1][1]->texcoord; }
            if (!(nFlagsY & DEGEN_2a)) { tangentY[4] = teil[1][2]->vertex - teil[0][2]->vertex; tangentT[4] = teil[1][2]->texcoord - teil[0][2]->texcoord; }
            if (!(nFlagsY & DEGEN_2b)) { tangentY[5] = teil[2][2]->vertex - teil[1][2]->vertex; tangentT[5] = teil[2][2]->texcoord - teil[1][2]->texcoord; }

            // entartete Kanten borgen die naechste parallele gueltige Tangente
            entarteteTangentenErsetzen(tangentX, tangentS, nFlagsX);
            entarteteTangentenErsetzen(tangentY, tangentT, nFlagsY);

            {   // x=0, y=0
                const std::size_t index = offStartX + offStartY;
                std::size_t index0 = 0;
                std::size_t index1 = 0;
                const double dot = karte::punkt(tangentX[index0], tangentY[index1]);
                const double length = laenge(tangentX[index0]) * laenge(tangentY[index1]);
                besteTangenten00(nFlagsX, dot, length, index0, index1);
                tangentenSammeln(index, tangentX, tangentY, tangentS, tangentT, index0, index1);
            }
            {   // x=1, y=0
                const std::size_t index = offEndX + offStartY;
                std::size_t index0 = 1;
                std::size_t index1 = 4;
                const double dot = karte::punkt(tangentX[index0], tangentY[index1]);
                const double length = laenge(tangentX[index0]) * laenge(tangentY[index1]);
                besteTangenten10(nFlagsX, dot, length, index0, index1);
                tangentenSammeln(index, tangentX, tangentY, tangentS, tangentT, index0, index1);
            }
            {   // x=0, y=1 - NRC nimmt hier tangentX[index1] fuer die Laenge (:2783)
                const std::size_t index = offStartX + offEndY;
                std::size_t index0 = 4;
                std::size_t index1 = 1;
                const double dot = karte::punkt(tangentX[index0], tangentY[index1]);
                const double length = laenge(tangentX[index1]) * laenge(tangentY[index1]);
                besteTangenten01(nFlagsX, dot, length, index0, index1);
                tangentenSammeln(index, tangentX, tangentY, tangentS, tangentT, index0, index1);
            }
            {   // x=1, y=1
                const std::size_t index = offEndX + offEndY;
                std::size_t index0 = 5;
                std::size_t index1 = 5;
                const double dot = karte::punkt(tangentX[index0], tangentY[index1]);
                const double length = laenge(tangentX[index0]) * laenge(tangentY[index1]);
                besteTangenten11(nFlagsX, dot, length, index0, index1);
                tangentenSammeln(index, tangentX, tangentY, tangentS, tangentT, index0, index1);
            }

            // Normalen, die nicht mehr weiter gesammelt werden, normieren
            if (i != 0 || j != 0) {
                normierenSicher(m_tess.vertices[offStartX + offStartY].normal);
                normierenSicher(m_tess.vertices[offStartX + offStartY].tangent);
                normierenSicher(m_tess.vertices[offStartX + offStartY].bitangent);
            }
            if (i + 3 == m_width) {
                normierenSicher(m_tess.vertices[offEndX + offStartY].normal);
                normierenSicher(m_tess.vertices[offEndX + offStartY].tangent);
                normierenSicher(m_tess.vertices[offEndX + offStartY].bitangent);
            }
            if (j + 3 == m_height) {
                normierenSicher(m_tess.vertices[offStartX + offEndY].normal);
                normierenSicher(m_tess.vertices[offStartX + offEndY].tangent);
                normierenSicher(m_tess.vertices[offStartX + offEndY].bitangent);
            }
            if (i + 3 == m_width && j + 3 == m_height) {
                normierenSicher(m_tess.vertices[offEndX + offEndY].normal);
                normierenSicher(m_tess.vertices[offEndX + offEndY].tangent);
                normierenSicher(m_tess.vertices[offEndX + offEndY].bitangent);
            }

            // gemeinsame Kanten mitteln
            if (j != 0) { nFlagsX |= AVERAGE; }
            if (i != 0) { nFlagsY |= AVERAGE; }
            // gemeinsame Kanten nur einmal auswerten
            nFlagsX |= SPLIT;
            nFlagsY |= SPLIT;

            // gekruemmt: rekursiv tesselieren
            if (patchDef3) {
                teilfeldFest(m_tess.vertices.data() + offStartX + offStartY, 1, m_tess.nArrayWidth, nFlagsX, nFlagsY, teil);
            } else {
                if (!leafX) {
                    teilfeld(m_kurvenU[i >> 1].get(), m_kurvenV[j >> 1].get(), offStartX, offStartY, offEndX, offEndY,
                             nFlagsX, nFlagsY, teil[1][0]->vertex, teil[1][1]->vertex, teil[1][2]->vertex,
                             teil[1][0]->texcoord, teil[1][1]->texcoord, teil[1][2]->texcoord, false);
                } else if (!leafY) {
                    teilfeld(m_kurvenV[j >> 1].get(), m_kurvenU[i >> 1].get(), offStartY, offStartX, offEndY, offEndX,
                             nFlagsY, nFlagsX, teil[0][1]->vertex, teil[1][1]->vertex, teil[2][1]->vertex,
                             teil[0][1]->texcoord, teil[1][1]->texcoord, teil[2][1]->texcoord, true);
                }
            }
            offStartX = offEndX;
        }
        offStartY = offEndY;
    }
}

// transform (patch.h:650-660) + freezeTransform
void Patch::transformieren(const Matrix4& m) {
    sichern();
    for (auto& c : m_ctrl) {
        transformierePunkt(m, c.vertex);
    }
    if (haendigkeit(m) == Haendigkeit::Links) {
        // PatchControlArray_invert (patch.h:172-183)
        std::vector<PatchPunkt> tmp(m_width);
        PatchPunkt* from = m_ctrl.data() + (m_width * (m_height - 1));
        PatchPunkt* to = m_ctrl.data();
        for (std::size_t h = 0; h != ((m_height - 1) >> 1); ++h, to += m_width, from -= m_width) {
            std::copy(to, to + m_width, tmp.data());
            std::copy(from, from + m_width, to);
            std::copy(tmp.data(), tmp.data() + m_width, from);
        }
    }
    punkteGeaendert();
}

// snapto (patch.h:695-704)
void Patch::einrasten(float raster) {
    sichern();
    for (auto& c : m_ctrl) {
        karte::einrasten(c.vertex, raster);
    }
    punkteGeaendert();
}

// ProjectTexture( TextureProjection projection, const Vector3& normal ) (:1212-1228)
void Patch::texturProjizieren(TexturProjektion projektion, const Vektor3f& normal, const Kontext& k) {
    sichern();
    const auto [w, h] = k.groesse(m_shader);
    projektion.bp.addScale(w, h);
    Matrix4 lokalZuTex;
    lokalZuTextur(k.art(), projektion, w, h, normal, lokalZuTex);
    for (auto& c : m_ctrl) {
        const Vektor3f st = transformierterPunkt(lokalZuTex, c.vertex);
        c.texcoord = Vektor2f(st[0], st[1]);
    }
    punkteGeaendert();
}

// Patch_setTexture (surfacedialog.cpp:1497-1502): p.m_texcoord = matrix4_transformed_point( mat, Vector3( p.m_texcoord, 0 ) ).vec2()
void Patch::texturkoordinatenTransformieren(const Matrix4& m) {
    sichern();
    for (auto& c : m_ctrl) {
        const Vektor3f st = transformierterPunkt(m, Vektor3f(c.texcoord[0], c.texcoord[1], 0));
        c.texcoord = Vektor2f(st[0], st[1]);
    }
    punkteGeaendert();
}

// InvertMatrix (:292-298)
void Patch::matrixUmkehren() {
    sichern();
    std::vector<PatchPunkt> tmp(m_width);
    PatchPunkt* from = m_ctrl.data() + (m_width * (m_height - 1));
    PatchPunkt* to = m_ctrl.data();
    for (std::size_t h = 0; h != ((m_height - 1) >> 1); ++h, to += m_width, from -= m_width) {
        std::copy(to, to + m_width, tmp.data());
        std::copy(from, from + m_width, to);
        std::copy(tmp.data(), tmp.data() + m_width, from);
    }
    punkteGeaendert();
}

// TransposeMatrix (:300-321)
void Patch::matrixTransponieren() {
    sichern();
    const std::vector<PatchPunkt> tmp(m_ctrl);
    const PatchPunkt* from = tmp.data();
    for (std::size_t h = 0; h != m_height; ++h) {
        PatchPunkt* to = m_ctrl.data() + h;
        for (std::size_t w = 0; w != m_width; ++w, ++from, to += m_height) {
            *to = *from;
        }
    }
    std::swap(m_width, m_height);
    punkteGeaendert();
}

std::unique_ptr<Memento> Patch::zustandSichern() const {
    auto m = std::make_unique<PatchMemento>();
    m->width = m_width;
    m->height = m_height;
    m->shader = m_shader;
    m->ctrl = m_ctrl;
    m->patchDef3 = patchDef3;
    m->unterteilungX = unterteilungX;
    m->unterteilungY = unterteilungY;
    return m;
}

// importState (patch.h:937-961)
void Patch::durchschnittsAchsen(Vektor3f& wRicht, Vektor3f& hRicht) const {
    wRicht = hRicht = Vektor3f(0, 0, 0);
    for (std::size_t r = 0; r < m_height; ++r) { wRicht += punkt(r, m_width - 1).vertex - punkt(r, 0).vertex; }
    for (std::size_t c = 0; c < m_width; ++c) { hRicht += punkt(m_height - 1, c).vertex - punkt(0, c).vertex; }
    // sonst die laengste Sehne
    if (gleichEps(wRicht, Vektor3f(0, 0, 0), 1.F)) {
        float besteLaenge = 0;
        for (std::size_t r = 0; r < m_height; ++r) {
            for (std::size_t c = 0; c + 1 < m_width; ++c) {
                const Vektor3f richtung = punkt(r, c + 1).vertex - punkt(r, c).vertex;
                const auto l = static_cast<float>(laenge(richtung));
                if (l - besteLaenge > 1) {
                    besteLaenge = l;
                    wRicht = richtung;
                }
            }
        }
    }
    if (gleichEps(hRicht, Vektor3f(0, 0, 0), 1.F)) {
        float besteLaenge = 0;
        for (std::size_t c = 0; c < m_width; ++c) {
            for (std::size_t r = 0; r + 1 < m_height; ++r) {
                const Vektor3f richtung = punkt(r + 1, c).vertex - punkt(r, c).vertex;
                const auto l = static_cast<float>(laenge(richtung));
                if (l - besteLaenge > 1) {
                    besteLaenge = l;
                    hRicht = richtung;
                }
            }
        }
    }
    if (gleichEps(kreuz(wRicht, hRicht), Vektor3f(0, 0, 0), 0.1F)) {
        wRicht = kAchseX;
        hRicht = kAchseY;
    }
}

Vektor3f Patch::durchschnittsNormale() const {
    Vektor3f w;
    Vektor3f h;
    durchschnittsAchsen(w, h);
    return normiert(kreuz(w, h));
}

void Patch::punkteSetzen(const std::vector<PatchPunkt>& p) {
    sichern();
    m_ctrl = p;
    punkteGeaendert();
}

void Patch::zustandLaden(const Memento& mm) {
    zustandZuruecksetzen(mm);
    punktWahl.assign(m_ctrl.size(), 0);   // onAllocate
}

void Patch::zustandZuruecksetzen(const Memento& mm) {
    sichern();
    const auto& m = static_cast<const PatchMemento&>(mm);
    m_width = m.width;
    m_height = m.height;
    setzeShader(m.shader);
    m_ctrl = m.ctrl;
    patchDef3 = m.patchDef3;
    unterteilungX = m.unterteilungX;
    unterteilungY = m.unterteilungY;
    if (punktWahl.size() != m_ctrl.size()) { punktWahl.assign(m_ctrl.size(), 0); }
    punkteGeaendert();
}

}  // namespace bhed::karte
