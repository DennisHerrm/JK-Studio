// patchwerkzeuge.cpp - Karten-Editor: Werkzeuge am Patch (Redisperse, Smooth,
// Insert/Remove, Caps, Textur-Operationen, Prefabs, Thicken).
//
// Portiert aus NetRadiant-Custom 20260114: radiant/patch.cpp:323-485
// (Redisperse, Smooth, InsertRemove), :445-485 (MakeCap), :487-585
// (FlipTexture, TranslateTexture, ScaleTexture, RotateTexture,
// SetTextureRepeat), :635-763 (texture_axis, CapTexture, NaturalTexture),
// :778-1061 (InsertPoints, RemovePoints), :1063-1174 (ConstructSeam),
// :1245-1589 (constructPlane, ConstructPrefab), :2870-3290
// (getAverageNormal, createThickenedOpposite, createThickenedWall).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
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

#include "bhed/karte/texdef.h"

#include <cmath>
#include <vector>

namespace bhed::karte {

namespace {

constexpr double kMPi = 3.14159265358979323846;   // M_PI

float mitteF(float a, float b) { return (a + b) * 0.5F; }   // float_mid

// getAverageNormal (patch.cpp:2870-2902)
Vektor3f mittlereNormale(const Vektor3f& n1, const Vektor3f& n2) {
    if (laengeQuadrat(n1) == 0) { return n2; }
    if (laengeQuadrat(n2) == 0) { return n1; }
    if (laengeQuadrat(n1 + n2) == 0) { return n1; }
    Vektor3f n = normiert(n1 + n2);
    float faktor = static_cast<float>(punkt(n1, n2));
    if (faktor > 1) { faktor = 1; }
    if (faktor < -1) { faktor = -1; }
    faktor = static_cast<float>(std::acos(faktor));
    faktor = static_cast<float>(std::cos(faktor * 0.5));
    if (faktor != 0) { n /= faktor; }
    return n;
}

bool leer(const Vektor3f& v) { return laengeQuadrat(v) == 0; }

}  // namespace

// Redisperse (patch.cpp:323-361)
void Patch::neuVerteilen(bool spalten) {
    sichern();
    std::size_t breite = 0;
    std::size_t hoehe = 0;
    std::size_t zeilenSchritt = 0;
    std::size_t spaltenSchritt = 0;
    if (spalten) {   // COL
        breite = (m_width - 1) >> 1;
        hoehe = m_height;
        spaltenSchritt = 1;
        zeilenSchritt = m_width;
    } else {   // ROW
        breite = (m_height - 1) >> 1;
        hoehe = m_width;
        spaltenSchritt = m_width;
        zeilenSchritt = 1;
    }
    for (std::size_t h = 0; h < hoehe; ++h) {
        PatchPunkt* p1 = m_ctrl.data() + (h * zeilenSchritt);
        for (std::size_t w = 0; w < breite; ++w) {
            PatchPunkt* p2 = p1 + spaltenSchritt;
            PatchPunkt* p3 = p2 + spaltenSchritt;
            p2->vertex = mitte(p1->vertex, p3->vertex);
            p1 = p3;
        }
    }
    punkteGeaendert();
}

// Smooth (patch.cpp:363-422)
void Patch::glaetten(bool spalten) {
    sichern();
    std::size_t breite = 0;
    std::size_t hoehe = 0;
    std::size_t zeilenSchritt = 0;
    std::size_t spaltenSchritt = 0;
    if (spalten) {
        breite = (m_width - 1) >> 1;
        hoehe = m_height;
        spaltenSchritt = 1;
        zeilenSchritt = m_width;
    } else {
        breite = (m_height - 1) >> 1;
        hoehe = m_width;
        spaltenSchritt = m_width;
        zeilenSchritt = 1;
    }
    bool rund = true;   // wrap
    for (std::size_t h = 0; h < hoehe; ++h) {
        const PatchPunkt* p1 = m_ctrl.data() + (h * zeilenSchritt);
        const PatchPunkt* p2 = p1 + (2 * breite) * spaltenSchritt;
        if (laengeQuadrat(p1->vertex - p2->vertex) > 1) {
            rund = false;
            break;
        }
    }
    for (std::size_t h = 0; h < hoehe; ++h) {
        PatchPunkt* p1 = m_ctrl.data() + (h * zeilenSchritt) + spaltenSchritt;
        for (std::size_t w = 0; w < breite - 1; ++w) {
            PatchPunkt* p2 = p1 + spaltenSchritt;
            PatchPunkt* p3 = p2 + spaltenSchritt;
            p2->vertex = mitte(p1->vertex, p3->vertex);
            p1 = p3;
        }
        if (rund) {
            p1 = m_ctrl.data() + (h * zeilenSchritt) + (2 * breite - 1) * spaltenSchritt;
            PatchPunkt* p2 = m_ctrl.data() + (h * zeilenSchritt);
            PatchPunkt* p2b = m_ctrl.data() + (h * zeilenSchritt) + (2 * breite) * spaltenSchritt;
            PatchPunkt* p3 = m_ctrl.data() + (h * zeilenSchritt) + spaltenSchritt;
            p2->vertex = p2b->vertex = mitte(p1->vertex, p3->vertex);
        }
    }
    punkteGeaendert();
}

// InsertRemove (patch.cpp:424-443)
void Patch::einfuegenEntfernen(bool einfuegen, bool spalte, bool erste) {
    sichern();
    if (einfuegen) {
        if (spalte && (m_width + 2 <= kMaxPatchBreite)) {
            punkteEinfuegen(true, erste);
        } else if (m_height + 2 <= kMaxPatchHoehe) {
            punkteEinfuegen(false, erste);
        }
    } else {
        if (spalte && (m_width - 2 >= kMinPatchBreite)) {
            punkteEntfernen(true, erste);
        } else if (m_height - 2 >= kMinPatchHoehe) {
            punkteEntfernen(false, erste);
        }
    }
    punkteGeaendert();
}

// InsertPoints (patch.cpp:778-928); die Suche nach gewaehlten Punkten ist in
// NRC auskommentiert ("if ( 0 )"), pos bleibt 0.
void Patch::punkteEinfuegen(bool spalten, bool erste) {
    std::size_t breite = 0;
    std::size_t hoehe = 0;
    std::size_t zeilenSchritt = 0;
    std::size_t spaltenSchritt = 0;
    if (!spalten) {   // ROW
        spaltenSchritt = 1;
        zeilenSchritt = m_width;
        breite = m_width;
        hoehe = m_height;
    } else {   // COL
        spaltenSchritt = m_width;
        zeilenSchritt = 1;
        breite = m_height;
        hoehe = m_width;
    }
    const std::vector<PatchPunkt> tmp(m_ctrl);
    std::size_t zeilenSchritt2 = 0;
    std::size_t spaltenSchritt2 = 0;
    if (!spalten) {
        setzeGroesse(m_width, m_height + 2);
        spaltenSchritt2 = 1;
        zeilenSchritt2 = m_width;
    } else {
        setzeGroesse(m_width + 2, m_height);
        spaltenSchritt2 = m_width;
        zeilenSchritt2 = 1;
    }
    std::size_t pos = erste ? 2 : hoehe - 1;
    if (pos >= hoehe) {
        pos = erste ? 2 : hoehe - 1;
    } else if (pos == 0) {
        pos = 2;
    } else if ((pos % 2) != 0) {
        ++pos;
    }
    for (std::size_t w = 0; w != breite; ++w) {
        const PatchPunkt* p1 = tmp.data() + (w * spaltenSchritt);
        PatchPunkt* p2 = m_ctrl.data() + (w * spaltenSchritt2);
        for (std::size_t h = 0; h != hoehe; ++h, p2 += zeilenSchritt2, p1 += zeilenSchritt) {
            if (h == pos) { p2 += 2 * zeilenSchritt2; }
            *p2 = *p1;
        }
        p1 = tmp.data() + (w * spaltenSchritt + pos * zeilenSchritt);
        p2 = m_ctrl.data() + (w * spaltenSchritt2 + pos * zeilenSchritt2);
        PatchPunkt* r2a = p2 + zeilenSchritt2;
        PatchPunkt* r2b = p2 - zeilenSchritt2;
        const PatchPunkt* c2a = p1 - 2 * zeilenSchritt;
        const PatchPunkt* c2b = p1 - zeilenSchritt;
        *(p2 + 2 * zeilenSchritt2) = *p1;   // zwei neue Zeilenpunkte
        *r2a = *c2b;
        for (std::size_t i = 0; i != 3; ++i) {
            r2a->vertex[i] = mitteF(c2b->vertex[i], p1->vertex[i]);
            r2b->vertex[i] = mitteF(c2a->vertex[i], c2b->vertex[i]);
            p2->vertex[i] = mitteF(r2a->vertex[i], r2b->vertex[i]);
        }
        for (std::size_t i = 0; i != 2; ++i) {
            r2a->texcoord[i] = mitteF(c2b->texcoord[i], p1->texcoord[i]);
            r2b->texcoord[i] = mitteF(c2a->texcoord[i], c2b->texcoord[i]);
            p2->texcoord[i] = mitteF(r2a->texcoord[i], r2b->texcoord[i]);
        }
    }
}

// RemovePoints (patch.cpp:930-1061)
void Patch::punkteEntfernen(bool spalten, bool erste) {
    std::size_t breite = 0;
    std::size_t hoehe = 0;
    std::size_t zeilenSchritt = 0;
    std::size_t spaltenSchritt = 0;
    if (!spalten) {
        spaltenSchritt = 1;
        zeilenSchritt = m_width;
        breite = m_width;
        hoehe = m_height;
    } else {
        spaltenSchritt = m_width;
        zeilenSchritt = 1;
        breite = m_height;
        hoehe = m_width;
    }
    const std::vector<PatchPunkt> tmp(m_ctrl);
    std::size_t zeilenSchritt2 = 0;
    std::size_t spaltenSchritt2 = 0;
    if (!spalten) {
        setzeGroesse(m_width, m_height - 2);
        spaltenSchritt2 = 1;
        zeilenSchritt2 = m_width;
    } else {
        setzeGroesse(m_width - 2, m_height);
        spaltenSchritt2 = m_width;
        zeilenSchritt2 = 1;
    }
    std::size_t pos = erste ? 2 : hoehe - 3;
    if (pos >= hoehe) {
        pos = erste ? 2 : hoehe - 3;
    } else if (pos == 0) {
        pos = 2;
    } else if (pos > hoehe - 3) {
        pos = hoehe - 3;
    } else if ((pos % 2) != 0) {
        ++pos;
    }
    for (std::size_t w = 0; w != breite; ++w) {
        const PatchPunkt* p1 = tmp.data() + (w * spaltenSchritt);
        PatchPunkt* p2 = m_ctrl.data() + (w * spaltenSchritt2);
        for (std::size_t h = 0; h != hoehe; ++h, p2 += zeilenSchritt2, p1 += zeilenSchritt) {
            if (h == pos) {
                p1 += 2 * zeilenSchritt2;   // so in NRC (row_stride2; hier gleich row_stride)
                h += 2;
            }
            *p2 = *p1;
        }
        p1 = tmp.data() + (w * spaltenSchritt + pos * zeilenSchritt);
        p2 = m_ctrl.data() + (w * spaltenSchritt2 + pos * zeilenSchritt2);
        PatchPunkt* q = p2 - zeilenSchritt2;
        for (std::size_t i = 0; i < 3; ++i) {
            q->vertex[i] = ((p1 + 2 * zeilenSchritt)->vertex[i] + (p1 - 2 * zeilenSchritt)->vertex[i]) * 0.5F;
            q->vertex[i] = q->vertex[i] + (2.0F * (p1->vertex[i] - q->vertex[i]));
        }
        for (std::size_t i = 0; i < 2; ++i) {
            q->texcoord[i] = ((p1 + 2 * zeilenSchritt)->texcoord[i] + (p1 - 2 * zeilenSchritt)->texcoord[i]) * 0.5F;
            q->texcoord[i] = q->texcoord[i] + (2.0F * (p1->texcoord[i] - q->texcoord[i]));
        }
    }
}

// MakeCap (patch.cpp:445-485)
void Patch::kappeMachen(Patch& ziel, PatchKappe art, bool spalten, bool erste, const Kontext& k) const {
    const std::size_t breite = spalten ? m_height : m_width;
    const std::size_t hoehe = spalten ? m_width : m_height;
    std::vector<Vektor3f> p(breite);
    const std::size_t index = erste ? 0 : hoehe - 1;
    for (std::size_t i = 0; i < breite; ++i) {
        p[erste ? i : (breite - 1) - i] = spalten ? punkt(i, index).vertex : punkt(index, i).vertex;
    }
    ziel.nahtBauen(art, p, breite, k);
}

// ConstructSeam (patch.cpp:1063-1174)
void Patch::nahtBauen(PatchKappe art, std::vector<Vektor3f>& p, std::size_t breite, const Kontext& k) {
    switch (art) {
    case PatchKappe::IBevel:
        setzeGroesse(3, 3);
        m_ctrl[0].vertex = p[0];
        m_ctrl[1].vertex = p[1];
        m_ctrl[2].vertex = p[1];
        m_ctrl[3].vertex = p[1];
        m_ctrl[4].vertex = p[1];
        m_ctrl[5].vertex = p[1];
        m_ctrl[6].vertex = p[2];
        m_ctrl[7].vertex = p[1];
        m_ctrl[8].vertex = p[1];
        break;
    case PatchKappe::Bevel: {
        setzeGroesse(3, 3);
        const Vektor3f p3(p[2] + (p[0] - p[1]));
        m_ctrl[0].vertex = p3;
        m_ctrl[1].vertex = p3;
        m_ctrl[2].vertex = p[2];
        m_ctrl[3].vertex = p3;
        m_ctrl[4].vertex = p3;
        m_ctrl[5].vertex = p[1];
        m_ctrl[6].vertex = p3;
        m_ctrl[7].vertex = p3;
        m_ctrl[8].vertex = p[0];
        break;
    }
    case PatchKappe::EndCap: {
        const Vektor3f p5(mitte(p[0], p[4]));
        setzeGroesse(3, 3);
        m_ctrl[0].vertex = p[0];
        m_ctrl[1].vertex = p5;
        m_ctrl[2].vertex = p[4];
        m_ctrl[3].vertex = p[1];
        m_ctrl[4].vertex = p[2];
        m_ctrl[5].vertex = p[3];
        m_ctrl[6].vertex = p[2];
        m_ctrl[7].vertex = p[2];
        m_ctrl[8].vertex = p[2];
        break;
    }
    case PatchKappe::IEndCap:
        setzeGroesse(5, 3);
        m_ctrl[0].vertex = p[4];
        m_ctrl[1].vertex = p[3];
        m_ctrl[2].vertex = p[2];
        m_ctrl[3].vertex = p[1];
        m_ctrl[4].vertex = p[0];
        m_ctrl[5].vertex = p[3];
        m_ctrl[6].vertex = p[3];
        m_ctrl[7].vertex = p[2];
        m_ctrl[8].vertex = p[1];
        m_ctrl[9].vertex = p[1];
        m_ctrl[10].vertex = p[3];
        m_ctrl[11].vertex = p[3];
        m_ctrl[12].vertex = p[2];
        m_ctrl[13].vertex = p[1];
        m_ctrl[14].vertex = p[1];
        break;
    case PatchKappe::Zylinder: {
        std::size_t mid = (breite - 1) >> 1;
        const bool entartet = (mid % 2) != 0;
        const std::size_t neueHoehe = mid + (entartet ? 2 : 1);
        setzeGroesse(3, neueHoehe);
        if (entartet) {
            ++mid;
            if (p.size() < breite + 2) { p.resize(breite + 2); }
            for (std::size_t i = breite; i != breite + 2; ++i) { p[i] = p[breite - 1]; }
        }
        {
            PatchPunkt* c = m_ctrl.data();
            for (std::size_t i = 0; i != m_height; ++i, c += m_width) { c->vertex = p[i]; }
        }
        {
            PatchPunkt* c = m_ctrl.data() + 2;
            const std::size_t h = m_height - 1;
            for (std::size_t i = 0; i != m_height; ++i, c += m_width) { c->vertex = p[h + (h - i)]; }
        }
        neuVerteilen(true);   // Redisperse( COL )
        break;
    }
    }
    texturKappen(k);
    punkteGeaendert();
}

// FlipTexture (patch.cpp:487-497)
void Patch::texturSpiegeln(int achse) {
    sichern();
    for (auto& c : m_ctrl) { c.texcoord[static_cast<std::size_t>(achse)] = -c.texcoord[static_cast<std::size_t>(achse)]; }
    punkteGeaendert();
}

// TranslateTexture (patch.cpp:499-513)
void Patch::texturVerschieben(float s, float t, const Kontext& k) {
    sichern();
    const auto [w, h] = k.groesse(m_shader);
    s = -1 * s / static_cast<float>(w);
    t = t / static_cast<float>(h);
    for (auto& c : m_ctrl) {
        c.texcoord[0] += s;
        c.texcoord[1] += t;
    }
    punkteGeaendert();
}

// ScaleTexture (patch.cpp:515-526)
void Patch::texturSkalieren(float s, float t) {
    sichern();
    for (auto& c : m_ctrl) {
        c.texcoord[0] *= s;
        c.texcoord[1] *= t;
    }
    punkteGeaendert();
}

// RotateTexture (patch.cpp:528-544)
void Patch::texturDrehen(float winkel) {
    sichern();
    const float s = static_cast<float>(std::sin(gradZuBogen(winkel)));
    const float c = static_cast<float>(std::cos(gradZuBogen(winkel)));
    for (auto& p : m_ctrl) {
        const float x = p.texcoord[0];
        const float y = p.texcoord[1];
        p.texcoord[0] = (x * c) - (y * s);
        p.texcoord[1] = (y * c) + (x * s);
    }
    punkteGeaendert();
}

// SetTextureRepeat (patch.cpp:547-569)
void Patch::texturWiederholung(float s, float t) {
    sichern();
    const float si = (s == 0 ? 1.F : s) / static_cast<float>(m_width - 1);
    const float ti = (t == 0 ? 1.F : t) / static_cast<float>(m_height - 1);
    PatchPunkt* ziel = m_ctrl.data();
    float tc = 0;
    for (std::size_t h = 0; h < m_height; ++h, tc += ti) {
        float sc = 0;
        for (std::size_t w = 0; w < m_width; ++w, sc += si) {
            ziel->texcoord[0] = sc;
            ziel->texcoord[1] = tc;
            ++ziel;
        }
    }
    punkteGeaendert();
}

// CapTexture (patch.cpp:646-685, der aktive Zweig)
void Patch::texturKappen(const Kontext& k) {
    const Vektor3f normal = durchschnittsNormale();
    TexturProjektion projektion;
    texdefStandard(k.art(), projektion);
    berechneAchsenbasis(normal, projektion.basisS, projektion.basisT);   // Valve220
    texturProjizieren(projektion, normal, k);
}

// NaturalTexture (patch.cpp:688-761): die laengste parallele Sehne je Zeile/Spalte
void Patch::texturNatuerlich(const Kontext& k) {
    sichern();
    const auto [tw, th] = k.groesse(m_shader);
    {
        const double groesse = static_cast<double>(tw) * static_cast<double>(standardTexturSkalierung());
        double best = 0;
        double tex = 0;
        PatchPunkt* pBreite = m_ctrl.data();
        for (std::size_t w = 0; w < m_width; ++w, ++pBreite) {
            {
                PatchPunkt* pHoehe = pBreite;
                for (std::size_t h = 0; h < m_height; ++h, pHoehe += m_width) { pHoehe->texcoord[0] = static_cast<float>(tex); }
            }
            if (w + 1 == m_width) { break; }
            {
                const PatchPunkt* pHoehe = pBreite;
                for (std::size_t h = 0; h < m_height; ++h, pHoehe += m_width) {
                    const double l = tex + (laenge(pHoehe->vertex - (pHoehe + 1)->vertex) / groesse);
                    if (std::fabs(l) > std::fabs(best)) { best = l; }
                }
            }
            tex = best;
        }
    }
    {
        const double groesse = -static_cast<double>(th) * static_cast<double>(standardTexturSkalierung());
        double best = 0;
        double tex = 0;
        PatchPunkt* pHoehe = m_ctrl.data();
        for (std::size_t h = 0; h < m_height; ++h, pHoehe += m_width) {
            {
                PatchPunkt* pBreite = pHoehe;
                for (std::size_t w = 0; w < m_width; ++w, ++pBreite) { pBreite->texcoord[1] = static_cast<float>(tex); }
            }
            if (h + 1 == m_height) { break; }
            {
                const PatchPunkt* pBreite = pHoehe;
                for (std::size_t w = 0; w < m_width; ++w, ++pBreite) {
                    const double l = tex + (laenge(pBreite->vertex - (pBreite + m_width)->vertex) / groesse);
                    if (std::fabs(l) > std::fabs(best)) { best = l; }
                }
            }
            tex = best;
        }
    }
    punkteGeaendert();
}

// constructPlane (patch.cpp:1245-1289)
void Patch::ebeneBauen(const Kasten& g, int achse, std::size_t breite, std::size_t hoehe) {
    setzeGroesse(breite, hoehe);
    std::size_t x = 0;
    std::size_t y = 1;
    std::size_t z = 2;
    switch (achse) {
    case 2: x = 0; y = 1; z = 2; break;
    case 1: x = 0; y = 2; z = 1; break;
    case 0: x = 1; y = 2; z = 0; break;
    default: return;
    }
    if (m_width < kMinPatchBreite || m_width > kMaxPatchBreite) { m_width = 3; }
    if (m_height < kMinPatchHoehe || m_height > kMaxPatchHoehe) { m_height = 3; }
    Vektor3f start;
    start[x] = g.origin[x] - g.extents[x];
    start[y] = g.origin[y] - g.extents[y];
    start[z] = g.origin[z] + g.extents[z];
    const float xAdj = std::fabs((start[x] - (g.origin[x] + g.extents[x])) / static_cast<float>(m_width - 1));
    const float yAdj = std::fabs((start[y] - (g.origin[y] + g.extents[y])) / static_cast<float>(m_height - 1));
    Vektor3f v;
    v[z] = start[z];
    PatchPunkt* c = m_ctrl.data();
    v[y] = start[y];
    for (std::size_t h = 0; h < m_height; ++h) {
        v[x] = start[x];
        for (std::size_t w = 0; w < m_width; ++w, ++c) {
            c->vertex = v;
            v[x] += xAdj;
        }
        v[y] += yAdj;
    }
}

// ConstructPrefab (patch.cpp:1291-1589)
void Patch::vorlageBauen(const Kasten& g, PatchVorlage art, int achse, std::size_t breite, std::size_t hoehe, const Kontext& k) {
    std::size_t x = 0;
    std::size_t y = 1;
    std::size_t z = 2;
    switch (achse) {
    case 2: x = 0; y = 1; z = 2; break;
    case 1: x = 2; y = 0; z = 1; break;   // so in NRC (die alte Belegung 0, 2, 1 auskommentiert)
    case 0: x = 1; y = 2; z = 0; break;
    default: return;
    }
    Vektor3f pos[3];
    if (art != PatchVorlage::Ebene) {
        pos[0] = g.origin - g.extents;
        pos[1] = g.origin;
        pos[2] = g.origin + g.extents;
    }
    if (art == PatchVorlage::Ebene) {
        ebeneBauen(g, achse, breite, hoehe);
    } else if (art == PatchVorlage::EckZylinder || art == PatchVorlage::Zylinder || art == PatchVorlage::DichterZylinder ||
               art == PatchVorlage::SehrDichterZylinder || art == PatchVorlage::Kegel || art == PatchVorlage::Kugel) {
        static const unsigned char zylIndex[] = {0, 0, 1, 0, 2, 0, 2, 1, 2, 2, 1, 2, 0, 2, 0, 1, 0, 0};
        PatchPunkt* start = nullptr;
        switch (art) {
        case PatchVorlage::EckZylinder:
            setzeGroesse(9, 3);
            start = m_ctrl.data();
            break;
        case PatchVorlage::DichterZylinder:
        case PatchVorlage::SehrDichterZylinder:
        case PatchVorlage::Zylinder:
        case PatchVorlage::Kegel:
            setzeGroesse(9, 3);
            start = m_ctrl.data() + 1;
            break;
        default:   // Kugel
            setzeGroesse(9, 5);
            start = m_ctrl.data() + (9 + 1);
            break;
        }
        for (std::size_t h = 0; h < 3; ++h, start += 9) {
            const unsigned char* idx = zylIndex;
            PatchPunkt* c = start;
            for (std::size_t w = 0; w < 8; ++w, ++c) {
                c->vertex[x] = pos[idx[0]][x];
                c->vertex[y] = pos[idx[1]][y];
                c->vertex[z] = pos[h][z];
                idx += 2;
            }
        }
        switch (art) {
        case PatchVorlage::EckZylinder: {
            PatchPunkt* c = m_ctrl.data();
            for (std::size_t h = 0; h < 3; ++h, c += 9) { c[8].vertex = c[0].vertex; }
            break;
        }
        case PatchVorlage::DichterZylinder:
        case PatchVorlage::SehrDichterZylinder:
        case PatchVorlage::Zylinder: {
            PatchPunkt* c = m_ctrl.data();
            for (std::size_t h = 0; h < 3; ++h, c += 9) { c[0].vertex = c[8].vertex; }
            break;
        }
        case PatchVorlage::Kegel: {
            PatchPunkt* c = m_ctrl.data();
            for (std::size_t h = 0; h < 2; ++h, c += 9) { c[0].vertex = c[8].vertex; }
            c = m_ctrl.data() + 9 * 2;
            for (std::size_t w = 0; w < 9; ++w, ++c) {
                c->vertex[x] = pos[1][x];
                c->vertex[y] = pos[1][y];
                c->vertex[z] = pos[2][z];
            }
            break;
        }
        default: {   // Kugel
            PatchPunkt* c = m_ctrl.data() + 9;
            for (std::size_t h = 0; h < 3; ++h, c += 9) { c[0].vertex = c[8].vertex; }
            c = m_ctrl.data();
            for (std::size_t w = 0; w < 9; ++w, ++c) {
                c->vertex[x] = pos[1][x];
                c->vertex[y] = pos[1][y];
                c->vertex[z] = pos[0][z];
            }
            c = m_ctrl.data() + (9 * 4);
            for (std::size_t w = 0; w < 9; ++w, ++c) {
                c->vertex[x] = pos[1][x];
                c->vertex[y] = pos[1][y];
                c->vertex[z] = pos[2][z];
            }
            break;
        }
        }
    } else if (art == PatchVorlage::ExakterZylinder) {
        const int n = static_cast<int>(breite - 1) / 2;   // Segmente
        setzeGroesse(breite, hoehe);
        const float f = static_cast<float>(1 / std::cos(kMPi / n));
        for (unsigned int i = 0; i < breite; ++i) {
            const float winkel = static_cast<float>((kMPi * i) / n);   // 0 .. 2pi
            const float xs = static_cast<float>(pos[1][x] + (pos[2][x] - pos[1][x]) * std::cos(static_cast<double>(winkel)) * ((i & 1U) != 0 ? f : 1.0F));
            const float ys = static_cast<float>(pos[1][y] + (pos[2][y] - pos[1][y]) * std::sin(static_cast<double>(winkel)) * ((i & 1U) != 0 ? f : 1.0F));
            for (unsigned int j = 0; j < hoehe; ++j) {
                const float zs = pos[0][z] + (pos[2][z] - pos[0][z]) * (static_cast<float>(j) / static_cast<float>(hoehe - 1));
                PatchPunkt& v = m_ctrl[j * breite + i];
                v.vertex[x] = xs;
                v.vertex[y] = ys;
                v.vertex[z] = zs;
            }
        }
    } else if (art == PatchVorlage::ExakterKegel) {
        const int n = static_cast<int>(breite - 1) / 2;
        setzeGroesse(breite, hoehe);
        const float f = static_cast<float>(1 / std::cos(kMPi / n));
        for (unsigned int i = 0; i < breite; ++i) {
            const float winkel = static_cast<float>((kMPi * i) / n);
            for (unsigned int j = 0; j < hoehe; ++j) {
                const float anteil = 1.0F - (static_cast<float>(j) / static_cast<float>(hoehe - 1));
                const float xs = static_cast<float>(pos[1][x] + anteil * (pos[2][x] - pos[1][x]) * std::cos(static_cast<double>(winkel)) * ((i & 1U) != 0 ? f : 1.0F));
                const float ys = static_cast<float>(pos[1][y] + anteil * (pos[2][y] - pos[1][y]) * std::sin(static_cast<double>(winkel)) * ((i & 1U) != 0 ? f : 1.0F));
                const float zs = pos[0][z] + (pos[2][z] - pos[0][z]) * (static_cast<float>(j) / static_cast<float>(hoehe - 1));
                PatchPunkt& v = m_ctrl[j * breite + i];
                v.vertex[x] = xs;
                v.vertex[y] = ys;
                v.vertex[z] = zs;
            }
        }
    } else if (art == PatchVorlage::ExakteKugel) {
        const int n = static_cast<int>(breite - 1) / 2;   // Segmente (Gier)
        const int m = static_cast<int>(hoehe - 1) / 2;    // Segmente (Neigung)
        setzeGroesse(breite, hoehe);
        const float f = static_cast<float>(1 / std::cos(kMPi / n));
        const float gg = static_cast<float>(1 / std::cos(kMPi / (2 * m)));
        for (unsigned int i = 0; i < breite; ++i) {
            const float winkel = static_cast<float>((kMPi * i) / n);
            for (unsigned int j = 0; j < hoehe; ++j) {
                const float winkel2 = static_cast<float>((kMPi * j) / (2 * m));
                const double fj = (j & 1U) != 0 ? gg : 1.0F;
                const double fi = (i & 1U) != 0 ? f : 1.0F;
                const float xs = static_cast<float>(pos[1][x] + (pos[2][x] - pos[1][x]) * std::sin(static_cast<double>(winkel2)) * fj * std::cos(static_cast<double>(winkel)) * fi);
                const float ys = static_cast<float>(pos[1][y] + (pos[2][y] - pos[1][y]) * std::sin(static_cast<double>(winkel2)) * fj * std::sin(static_cast<double>(winkel)) * fi);
                const float zs = static_cast<float>(pos[1][z] + (pos[2][z] - pos[1][z]) * -std::cos(static_cast<double>(winkel2)) * fj);
                PatchPunkt& v = m_ctrl[j * breite + i];
                v.vertex[x] = xs;
                v.vertex[y] = ys;
                v.vertex[z] = zs;
            }
        }
    } else if (art == PatchVorlage::Bevel) {
        static const unsigned char bevIndex[] = {0, 0, 2, 0, 2, 2};
        setzeGroesse(3, 3);
        PatchPunkt* c = m_ctrl.data();
        for (std::size_t h = 0; h < 3; ++h) {
            const unsigned char* idx = bevIndex;
            for (std::size_t w = 0; w < 3; ++w, idx += 2, ++c) {
                c->vertex[x] = pos[idx[0]][x];
                c->vertex[y] = pos[idx[1]][y];
                c->vertex[z] = pos[h][z];
            }
        }
    } else if (art == PatchVorlage::EndCap) {
        static const unsigned char endIndex[] = {2, 0, 2, 2, 1, 2, 0, 2, 0, 0};
        setzeGroesse(5, 3);
        PatchPunkt* c = m_ctrl.data();
        for (std::size_t h = 0; h < 3; ++h) {
            const unsigned char* idx = endIndex;
            for (std::size_t w = 0; w < 5; ++w, idx += 2, ++c) {
                c->vertex[x] = pos[idx[0]][x];
                c->vertex[y] = pos[idx[1]][y];
                c->vertex[z] = pos[h][z];
            }
        }
    }
    if (art == PatchVorlage::DichterZylinder) { einfuegenEntfernen(true, false, true); }
    if (art == PatchVorlage::SehrDichterZylinder) {
        einfuegenEntfernen(true, false, false);
        einfuegenEntfernen(true, false, true);
    }
    if (art == PatchVorlage::Ebene) {
        texturKappen(k);
    } else {
        texturNatuerlich(k);
    }
}

// createThickenedOpposite (patch.cpp:2905-3188)
void Patch::verdicktGegenueber(const Patch& quelle, float dicke, int achse, bool& ohne12, bool& ohne34) {
    setzeGroesse(quelle.breite(), quelle.hoehe());
    setzeShader(quelle.shader());
    Vektor3f achsenRichtung(0, 0, 0);   // 0 0 0: entlang der Punktnormalen
    switch (achse) {
    case 0: achsenRichtung = Vektor3f(1, 0, 0); break;
    case 1: achsenRichtung = Vektor3f(0, 1, 0); break;
    case 2: achsenRichtung = Vektor3f(0, 0, 1); break;
    default: break;
    }
    // Naehte noetig? (Anfang != Ende: kein Zylinder)
    for (std::size_t col = 0; col < m_width; ++col) {
        if (laengeQuadrat(quelle.punkt(0, col).vertex - quelle.punkt(m_height - 1, col).vertex) > 0.1F) {
            ohne12 = false;
            break;
        }
    }
    for (std::size_t row = 0; row < m_height; ++row) {
        if (laengeQuadrat(quelle.punkt(row, 0).vertex - quelle.punkt(row, m_width - 1).vertex) > 0.1F) {
            ohne34 = false;
            break;
        }
    }
    const auto norm = [](Vektor3f& v) {
        if (laengeQuadrat(v) != 0) { normieren(v); }
    };
    const auto summeBetrag = [](const Vektor3f& v) { return std::fabs(v[0]) + std::fabs(v[1]) + std::fabs(v[2]); };
    for (std::size_t col = 0; col < m_width; ++col) {
        for (std::size_t row = 0; row < m_height; ++row) {
            const PatchPunkt& cur = quelle.punkt(row, col);
            Vektor3f normal;
            if (achsenRichtung == Vektor3f(0, 0, 0)) {
                Vektor3f colT[2] = {Vektor3f(0, 0, 0), Vektor3f(0, 0, 0)};
                if ((col == 0 || col == m_width - 1) && !ohne34) {
                    const std::size_t naechste = (col == m_width - 1) ? (col - 1) : (col + 1);
                    colT[0] = quelle.punkt(row, naechste).vertex - cur.vertex;
                    colT[0] *= (col == m_width - 1) ? -1 : +1;
                    norm(colT[0]);
                } else {
                    std::size_t naechste = 0;
                    std::size_t vorige = 0;
                    if (col == 0) {
                        naechste = col + 1;
                        vorige = m_width - 2;
                    } else if (col == m_width - 1) {
                        naechste = 1;
                        vorige = col - 1;
                    } else {
                        naechste = col + 1;
                        vorige = col - 1;
                    }
                    colT[0] = quelle.punkt(row, naechste).vertex - cur.vertex;
                    colT[1] = quelle.punkt(row, vorige).vertex - cur.vertex;
                    colT[1] *= -1;
                    norm(colT[0]);
                    norm(colT[1]);
                    if (leer(colT[1] + colT[0]) || leer(colT[1] - colT[0])) { colT[1] = Vektor3f(0, 0, 0); }
                }
                Vektor3f rowT[2] = {Vektor3f(0, 0, 0), Vektor3f(0, 0, 0)};
                if ((row == 0 || row == m_height - 1) && !ohne12) {
                    const std::size_t naechste = (row == m_height - 1) ? (row - 1) : (row + 1);
                    rowT[0] = quelle.punkt(naechste, col).vertex - cur.vertex;
                    rowT[0] *= (row == m_height - 1) ? -1 : +1;
                    norm(rowT[0]);
                } else {
                    std::size_t naechste = 0;
                    std::size_t vorige = 0;
                    if (row == 0) {
                        naechste = row + 1;
                        vorige = m_height - 2;
                    } else if (row == m_height - 1) {
                        naechste = 1;
                        vorige = row - 1;
                    } else {
                        naechste = row + 1;
                        vorige = row - 1;
                    }
                    rowT[0] = quelle.punkt(naechste, col).vertex - cur.vertex;
                    rowT[1] = quelle.punkt(vorige, col).vertex - cur.vertex;
                    rowT[1] *= -1;
                    norm(rowT[0]);
                    norm(rowT[1]);
                    if (leer(rowT[1] + rowT[0]) || leer(rowT[1] - rowT[0])) { rowT[1] = Vektor3f(0, 0, 0); }
                }
                // parallele Paare bereinigen
                if (leer(rowT[0] + colT[0]) || leer(rowT[0] - colT[0])) { rowT[0] = Vektor3f(0, 0, 0); }
                if (leer(rowT[1] + colT[1]) || leer(rowT[1] - colT[1])) { rowT[1] = Vektor3f(0, 0, 0); }
                if (leer(rowT[0] + colT[1]) || leer(rowT[0] - colT[1])) { colT[1] = Vektor3f(0, 0, 0); }
                if (leer(rowT[1] + colT[0]) || leer(rowT[1] - colT[0])) { rowT[1] = Vektor3f(0, 0, 0); }
                // Platzhalter bereinigen
                if (leer(colT[0])) {
                    colT[0] = colT[1];
                    colT[1] = Vektor3f(0, 0, 0);
                }
                if (leer(rowT[0])) {
                    rowT[0] = rowT[1];
                    rowT[1] = Vektor3f(0, 0, 0);
                }
                if (leer(rowT[0]) || leer(colT[0])) {
                    normal = achsenRichtung;
                } else if (summeBetrag(colT[1]) > 0 && summeBetrag(rowT[1]) > 0) {
                    normal = mittlereNormale(normiert(kreuz(rowT[0], colT[0])), normiert(kreuz(rowT[1], colT[1])));
                } else if (summeBetrag(colT[1]) > 0) {
                    normal = mittlereNormale(normiert(kreuz(rowT[0], colT[0])), normiert(kreuz(rowT[0], colT[1])));
                } else if (summeBetrag(rowT[1]) > 0) {
                    normal = mittlereNormale(normiert(kreuz(rowT[0], colT[0])), normiert(kreuz(rowT[1], colT[0])));
                } else if (laengeQuadrat(kreuz(rowT[0], colT[0])) > 0) {
                    normal = normiert(kreuz(rowT[0], colT[0]));
                } else {
                    normal = achsenRichtung;
                }
            } else {
                normal = achsenRichtung;   // die vorgegebene Richtung
            }
            punkt(row, col).vertex = cur.vertex + normal * dicke;
            punkt(row, col).texcoord = cur.texcoord;
        }
    }
    punkteGeaendert();
}

// createThickenedWall (patch.cpp:3190-3290)
void Patch::verdicktWand(const Patch& quelle, const Patch& ziel, int wand, const Kontext& k) {
    setzeShader(quelle.shader());
    int start = 0;
    int ende = 0;
    int schritt = 1;
    int cols = 0;
    const int rows = 3;
    const int qb = static_cast<int>(quelle.breite());
    const int qh = static_cast<int>(quelle.hoehe());
    switch (wand) {
    case 0:
        cols = qb;
        start = 0;
        ende = qb - 1;
        schritt = 1;
        break;
    case 1:
        cols = qb;
        start = qb * (qh - 1);
        ende = qb * qh - 1;
        schritt = 1;
        break;
    case 2:
        cols = qh;
        start = 0;
        ende = qb * (qh - 1);
        schritt = qb;
        break;
    case 3:
        cols = qh;
        start = qb - 1;
        ende = qb * qh - 1;
        schritt = qb;
        break;
    default: break;
    }
    setzeGroesse(static_cast<std::size_t>(cols), static_cast<std::size_t>(rows));
    const auto& q = quelle.punkte();
    const auto& z = ziel.punkte();
    std::size_t col = 0;
    for (int idx = start; idx <= ende; idx += schritt, ++col) {
        const Vektor3f a = q[static_cast<std::size_t>(idx)].vertex;
        const Vektor3f b = z[static_cast<std::size_t>(idx)].vertex;
        Vektor3f m = a + b;
        m /= 2;
        punkt(0, col).vertex = a;
        punkt(1, col).vertex = m;
        punkt(2, col).vertex = b;
    }
    if (wand == 0 || wand == 3) { matrixUmkehren(); }
    punkteGeaendert();
    texturNatuerlich(k);
}

}  // namespace bhed::karte
