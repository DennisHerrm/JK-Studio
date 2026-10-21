// pinsel.cpp - Karten-Editor: Pinsel und Flaechen.
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): radiant/brush.h,
// radiant/brush.cpp, radiant/brushmanip.cpp (Brush_ConstructCuboid),
// libs/render.h (UniqueVertexBuffer). Zeilennummern: brush.h, wenn nicht anders
// angegeben.
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
#include "bhed/karte/pinsel.h"

#include <algorithm>

namespace bhed::karte {

// plane3_inside (:1445-1454)
bool ebeneInnen(const Ebene3d& self, const Ebene3d& other, bool selfIsLater) {
    if (gleichEps(self.normal(), other.normal(), 0.001)) {
        // gleiche Ebene? die mit dem kleineren Index gewinnt
        if (self.dist() == other.dist()) {
            return selfIsLater;
        }
        return self.dist() < other.dist();
    }
    return true;
}

// ===================== Flaeche ==============================================

namespace {

struct FlaechenMemento final : Memento {   // Face::SavedState
    EbenenPunkte planepts{};
    TexturProjektion projektion;
    std::string shader;
    InhaltFlags flags;
};

}  // namespace

// Face( FaceObserver* ) (:942-954)
Flaeche::Flaeche() {
    // FaceTexdef( m_shader, TextureProjection(), false ): nicht initialisiert,
    // also auch kein addScale beim Realisieren.
    m_planepts[0] = Vektor3d(0, 0, 0);
    m_planepts[1] = Vektor3d(64, 0, 0);
    m_planepts[2] = Vektor3d(0, 64, 0);
    m_plane = ebeneAusPunkten(m_planepts);
    setzeBasis(m_plane.normal());
}

// Face( p0, p1, p2, shader, projection ) (:955-974)
Flaeche::Flaeche(const Vektor3d& p0, const Vektor3d& p1, const Vektor3d& p2, const std::string& shader,
                 const TexturProjektion& projektion, const Kontext& k)
    : m_shader(shader), m_projektion(projektion) {
    projektionInitialisiert = true;
    // FaceTexdef::realiseShader: initialisiert und noch nicht skaliert -> addScale
    const auto [w, h] = k.groesse(m_shader);
    m_projektion.bp.addScale(w, h);
    skalierungAngewandt = true;
    m_planepts[0] = p0;
    m_planepts[1] = p1;
    m_planepts[2] = p2;
    m_plane = ebeneAusPunkten(m_planepts);
    setzeBasis(m_plane.normal());
}

// Face( const Face& other, FaceObserver* ) (:975-988): Projektion als
// normalised() (removeScale), beim Realisieren wieder addScale. Die Basis
// wird nicht zurueckgesetzt.
Flaeche::Flaeche(const Flaeche& andere, const Kontext& k)
    : Rueckgaengigbar(), m_planepts(andere.m_planepts), m_shader(andere.m_shader), m_flags(andere.m_flags),
      m_projektion(andere.m_projektion) {
    const auto [w, h] = k.groesse(m_shader);
    m_projektion.bp.removeScale(w, h);   // other.getTexdef().normalised()
    projektionInitialisiert = true;
    m_projektion.bp.addScale(w, h);      // realiseShader
    skalierungAngewandt = true;
    m_plane = ebeneAusPunkten(m_planepts);
}

void Flaeche::ebeneGeaendert() {
    if (besitzer) { besitzer->geaendert(); }
}

void Flaeche::ladeEbenenPunkte(const EbenenPunkte& p) {   // FacePlane_importTokens: Punkte + MakePlane
    m_planepts = p;
    m_plane = ebeneAusPunkten(m_planepts);
}

void Flaeche::setzeEbenenPunkte(const EbenenPunkte& p) {   // assign_planepts + freezeTransform
    sichern();
    m_planepts = p;
    m_plane = ebeneAusPunkten(m_planepts);
    ebeneGeaendert();
}

// flipWinding / FacePlane::reverse (:691-701)
void Flaeche::umdrehen() {
    sichern();
    tauschen(m_planepts[0], m_planepts[2]);
    m_plane = ebeneAusPunkten(m_planepts);
    ebeneGeaendert();
}

// FaceTexdef::setBasis (:595-597, "face projection")
void Flaeche::setzeBasis(const Vektor3f& normal) {
    berechneAchsenbasis(normal, m_projektion.basisS, m_projektion.basisT);
}

// SetShader (:1194-1198) mit FaceShader::setShader: beim Wechsel wird die
// BP-Matrix ueber unrealise/realise von der alten auf die neue Texturgroesse
// umgerechnet (removeScale mit alter, addScale mit neuer Groesse).
void Flaeche::setzeShader(const std::string& name, const Kontext& k) {
    sichern();
    if (projektionInitialisiert && skalierungAngewandt) {   // FaceTexdef::unrealiseShader
        const auto [w, h] = k.groesse(m_shader);
        m_projektion.bp.removeScale(w, h);
        skalierungAngewandt = false;
    }
    m_shader = name;
    if (projektionInitialisiert && !skalierungAngewandt) {   // FaceTexdef::realiseShader
        const auto [w, h] = k.groesse(m_shader);
        m_projektion.bp.addScale(w, h);
        skalierungAngewandt = true;
    }
    ebeneGeaendert();
}

// SetFlags (:1230-1236) / ContentsFlagsValue_assignMasked (:300-310)
void Flaeche::setzeFlags(const InhaltFlags& f) {
    sichern();
    const bool detail = (m_flags.inhaltFlags & kDetailMaske) != 0;
    m_flags = f;
    if (detail) {
        m_flags.inhaltFlags |= kDetailMaske;
    } else {
        m_flags.inhaltFlags &= ~kDetailMaske;
    }
    ebeneGeaendert();
}

// setDetail (:1332-1351). Die Flags der Textur sind bei Quake3-Shadern 0
// (nur Quake2-.wal traegt welche).
void Flaeche::setzeDetail(bool detail) {
    sichern();
    if (detail && !istDetail()) {
        if (m_flags.angegeben) {
            m_flags.inhaltFlags |= kDetailMaske;
        } else {
            m_flags = InhaltFlags{0, 0 | kDetailMaske, 0, false};
        }
    } else if (!detail && istDetail()) {
        if (m_flags.angegeben) {
            m_flags.inhaltFlags &= ~kDetailMaske;
        } else {
            m_flags = InhaltFlags{0, 0, 0, false};
        }
    }
    ebeneGeaendert();
}

// GetTexdef -> FaceTexdef::normalised (:582-586)
TexturProjektion Flaeche::texdefHolen(const Kontext& k) const {
    TexturProjektion p = m_projektion;
    const auto [w, h] = k.groesse(m_shader);
    p.bp.removeScale(w, h);
    return p;
}

// SetTexdef (:1212-1219) mit FaceTexdef::setTexdef (:520-524)
void Flaeche::setzeTexdef(const TexturProjektion& p, const Kontext& k, bool basisSetzen, bool basisZuruecksetzen) {
    sichern();
    const auto [w, h] = k.groesse(m_shader);
    m_projektion.bp.removeScale(w, h);
    texdefZuweisen(k.art(), m_projektion, p, basisSetzen);
    m_projektion.bp.addScale(w, h);
    if (basisZuruecksetzen) {
        setzeBasis(m_plane.normal());
    }
    ebeneGeaendert();
}

// SetTexdef (Einzelwerte, :1221-1225) mit FaceTexdef::setTexdef (:526-533)
void Flaeche::setzeTexdef(const float* hShift, const float* vShift, const float* hScale, const float* vScale,
                          const float* rotation, const Kontext& k) {
    sichern();
    const auto [w, h] = k.groesse(m_shader);
    m_projektion.bp.removeScale(w, h);
    texdefZuweisen(k.art(), m_projektion, hShift, vShift, hScale, vScale, rotation);
    m_projektion.bp.addScale(w, h);
    ebeneGeaendert();
}

// ShiftTexdef (:1238-1242) / FaceTexdef::shift (:535-541)
void Flaeche::verschiebeTextur(float s, float t, const Kontext& k) {
    sichern();
    const auto [w, h] = k.groesse(m_shader);
    m_projektion.bp.removeScale(w, h);
    texdefVerschieben(k.art(), m_projektion, s, t);
    m_projektion.bp.addScale(w, h);
    ebeneGeaendert();
}

// ScaleTexdef (:1244-1248) / FaceTexdef::scale (:543-547)
void Flaeche::skaliereTextur(float s, float t, const Kontext& k) {
    sichern();
    const auto [w, h] = k.groesse(m_shader);
    m_projektion.bp.removeScale(w, h);
    texdefSkalieren(k.art(), m_projektion, s, t);
    m_projektion.bp.addScale(w, h);
    ebeneGeaendert();
}

// RotateTexdef (:1250-1254) / FaceTexdef::rotate (:549-553)
void Flaeche::dreheTextur(float winkel, const Kontext& k) {
    sichern();
    const auto [w, h] = k.groesse(m_shader);
    m_projektion.bp.removeScale(w, h);
    texdefDrehen(k.art(), m_projektion, winkel);
    m_projektion.bp.addScale(w, h);
    ebeneGeaendert();
}

// FitTexture (:1273-1277) - ohne remove/addScale
void Flaeche::passeTexturEin(float sWiederholung, float tWiederholung, bool nurAbmessung, const Kontext& k) {
    sichern();
    const auto [w, h] = k.groesse(m_shader);
    texdefEinpassen(k.art(), m_projektion, w, h, m_plane.normal(), m_wicklung, sWiederholung, tWiederholung, nurAbmessung);
    ebeneGeaendert();
}

// ProjectTexture (texdef, :1256-1260)
void Flaeche::projiziereTextur(const Texdef& td, const Vektor3f* richtung, const Kontext& k) {
    sichern();
    const auto [w, h] = k.groesse(m_shader);
    texturProjizieren(k.art(), m_projektion, w, h, m_plane, td, richtung);
    ebeneGeaendert();
}

// ProjectTexture (projection, :1262-1266)
void Flaeche::projiziereTextur(const TexturProjektion& p, const Vektor3f& normal, const Kontext& k) {
    sichern();
    const auto [w, h] = k.groesse(m_shader);
    texturProjizieren(k.art(), m_projektion, w, h, m_plane, p, normal);
    ebeneGeaendert();
}

// Convert (:1268-1271) / FaceTexdef::Convert (:563-566) - in NRC ohne undoSave
void Flaeche::rechneTexdefUm(TexdefArt von, TexdefArt nach, const Kontext& k) {
    const auto [w, h] = k.groesse(m_shader);
    texdefUmrechnen(von, nach, m_plane, m_projektion, w, h);
    skalierungAngewandt = true;
    ebeneGeaendert();
}

// transform (:1096-1117) und freezeTransform (:1130-1135). NRC aendert erst
// die "Transformed"-Kopien und friert dann ein (undoSave sichert dabei den
// alten Stand); hier wird zuerst gesichert und dann direkt geaendert.
void Flaeche::transformieren(const Matrix4& m, bool spiegeln, const Kontext& k) {
    sichern();
    const auto [w, h] = k.groesse(m_shader);
    if (k.texturLock) {
        // m_centroid_cached ist der Schwerpunkt beim Beginn der Bewegung
        const Vektor3f invariante = traegtBei() ? m_schwerpunkt : Vektor3f(m_plane.normal() * m_plane.dist());
        texdefTransformiertGesperrt(k.art(), m_projektion, w, h, m_plane, m, invariante);
    } else if (k.art() == TexdefArt::Valve) {
        // in float normiert, dann double (:1104)
        const Vektor3d von = normiert(kreuz(m_projektion.basisS, m_projektion.basisT));
        const Vektor3d nach = transformierteNormale(m, von);
        const Matrix4 mat = drehungAusQuaternion(quaternionAusEinheitsvektorenSicher(von, nach));
        m_projektion.basisS = normiert(transformierteRichtung(mat, m_projektion.basisS));
        m_projektion.basisT = normiert(transformierteRichtung(mat, m_projektion.basisT));
    }
    // FacePlane::transform (:702-731)
    transformierePunkt(m, m_planepts[0]);
    transformierePunkt(m, m_planepts[1]);
    transformierePunkt(m, m_planepts[2]);
    if (spiegeln) {
        tauschen(m_planepts[0], m_planepts[2]);
    }
    m_plane = ebeneAusPunkten(m_planepts);
    ebeneGeaendert();
}

// construct_centroid (:1288-1290)
void Flaeche::schwerpunktBerechnen() { wicklungSchwerpunkt(m_wicklung, m_plane, m_schwerpunkt); }

// EmitTextureCoordinates (:1279-1281)
void Flaeche::texturkoordinatenSetzen(const Kontext& k) {
    const auto [w, h] = k.groesse(m_shader);
    karte::texturkoordinatenSetzen(k.art(), m_projektion, w, h, m_wicklung, m_plane.normal(), kEinheitsMatrix);
}

// is_bounded (:1356-1358)
bool Flaeche::istBegrenzt() const {
    return std::none_of(m_wicklung.begin(), m_wicklung.end(),
                        [](const WicklungsPunkt& v) { return v.adjacent == kMaxFlaechen; });
}

std::unique_ptr<Memento> Flaeche::zustandSichern() const {
    auto m = std::make_unique<FlaechenMemento>();
    m->planepts = m_planepts;
    m->projektion = m_projektion;
    m->shader = m_shader;
    m->flags = m_flags;
    return m;
}

// importState (:1042-1052). NRC setzt den Shader (mit BP-Umrechnung) und dann
// die Projektion per Texdef_Assign; der sichtbare Endstand ist die gesicherte
// Projektion - hier wird sie ganz zurueckgeschrieben.
void Flaeche::zustandLaden(const Memento& mm) {
    sichern();
    const auto& m = static_cast<const FlaechenMemento&>(mm);
    m_planepts = m.planepts;
    m_plane = ebeneAusPunkten(m_planepts);
    m_shader = m.shader;
    m_flags = m.flags;
    m_projektion = m.projektion;
    ebeneGeaendert();
}

// ===================== Pinsel ===============================================

namespace {

struct PinselMemento final : Memento {   // BrushUndoMemento: nur die Zeiger
    Pinsel::Flaechen flaechen;
};

// SListNode / ProximalVertex (brush.cpp:52-91)
struct SListNode {
    const SListNode* next = nullptr;
};
struct NaheEcke {   // ProximalVertex
    const SListNode* knoten;
    bool operator==(const NaheEcke& o) const {
        const SListNode* v = knoten;
        std::size_t schleife = 0;
        do {
            if (v == o.knoten) { return true; }
            v = v->next;
            if (!(schleife < kMaxFlaechen)) { break; }
            ++schleife;
        } while (v != knoten);
        return false;
    }
    bool operator<(const NaheEcke& o) const {
        if (!(operator==(o))) { return knoten < o.knoten; }
        return false;
    }
};

// UniqueVertexBuffer (libs/render.h:152-270)
template<typename V>
class EindeutigePuffer {
public:
    explicit EindeutigePuffer(std::vector<V>& d) : m_daten(d) {}
    unsigned int einfuegen(const V& v) {   // insert
        if (m_daten.empty()) {
            m_daten.push_back(v);
            m_baum.push_back(Knoten());
            return 0;
        }
        if (m_daten[m_prev0] == v) { return m_prev0; }
        if (m_prev1 != m_prev0 && m_daten[m_prev1] == v) { return m_prev1; }
        if (m_prev2 != m_prev0 && m_prev2 != m_prev1 && m_daten[m_prev2] == v) { return m_prev2; }
        m_prev2 = m_prev1;
        m_prev1 = m_prev0;
        m_prev0 = findenOderEinfuegen(v);
        return m_prev0;
    }
private:
    struct Knoten {
        unsigned int links = 0;
        unsigned int rechts = 0;
    };
    unsigned int findenOderEinfuegen(const V& v) {   // find_or_insert
        unsigned int index = 0;
        while (true) {
            if (v < m_daten[index]) {
                Knoten& k = m_baum[index];
                if (k.links != 0) {
                    index = k.links;
                    continue;
                }
                k.links = static_cast<unsigned int>(m_baum.size());
                m_baum.push_back(Knoten());
                m_daten.push_back(v);
                return static_cast<unsigned int>(m_baum.size() - 1);
            }
            if (m_daten[index] < v) {
                Knoten& k = m_baum[index];
                if (k.rechts != 0) {
                    index = k.rechts;
                    continue;
                }
                k.rechts = static_cast<unsigned int>(m_baum.size());
                m_baum.push_back(Knoten());
                m_daten.push_back(v);
                return static_cast<unsigned int>(m_baum.size() - 1);
            }
            return index;
        }
    }
    std::vector<V>& m_daten;
    std::vector<Knoten> m_baum;
    unsigned int m_prev0 = 0;
    unsigned int m_prev1 = 0;
    unsigned int m_prev2 = 0;
};

// next_edge (:1460-1470)
FlaechenPunkt naechsteKante(const Pinsel::Flaechen& f, FlaechenPunkt fp) {
    const std::size_t nachbarFlaeche = f[fp.flaeche]->wicklung()[fp.punkt].adjacent;
    const std::size_t nachbarPunkt = wicklungFindeNachbar(f[nachbarFlaeche]->wicklung(), fp.flaeche);
    if (nachbarPunkt == kMaxFlaechen) {
        return fp;
    }
    return FlaechenPunkt{nachbarFlaeche, nachbarPunkt};
}

// next_vertex (:1473-1476)
FlaechenPunkt naechsteEcke(const Pinsel::Flaechen& f, FlaechenPunkt fp) {
    const FlaechenPunkt k = naechsteKante(f, fp);
    return FlaechenPunkt{k.flaeche, wicklungNaechster(f[k.flaeche]->wicklung(), k.punkt)};
}

}  // namespace

std::shared_ptr<Pinsel> Pinsel::kopie(const Kontext& k) const {   // Brush( const Brush& other ) -> copy
    auto p = std::make_shared<Pinsel>();
    p->schicht = schicht;
    for (const auto& f : m_flaechen) {
        p->flaecheHinzufuegen(*f, k);
    }
    p->geaendert();
    return p;
}

// push_back (:2038-2048): angemeldete Pinsel melden die Flaeche gleich an.
void Pinsel::anhaengen(std::shared_ptr<Flaeche> f) {
    f->besitzer = this;
    if (undoSystem()) { f->anmelden(undoSystem()); }
    m_flaechen.push_back(std::move(f));
}

Flaeche& Pinsel::neueFlaecheLaden() {
    anhaengen(std::make_shared<Flaeche>());
    m_ebeneGeaendert = true;
    return *m_flaechen.back();
}

// addPlane (:1985-1995)
Flaeche* Pinsel::ebeneHinzufuegen(const Vektor3d& p0, const Vektor3d& p1, const Vektor3d& p2, const std::string& shader,
                                  const TexturProjektion& projektion, const Kontext& k, int inhaltFlags) {
    if (m_flaechen.size() == kMaxFlaechen) {
        return nullptr;
    }
    sichern();
    anhaengen(std::make_shared<Flaeche>(p0, p1, p2, shader, projektion, k));
    m_flaechen.back()->flagsRoh().inhaltFlags = inhaltFlags;
    m_flaechen.back()->setzeDetail(istDetail());
    geaendert();
    return m_flaechen.back().get();
}

// addFace (:1973-1982)
Flaeche* Pinsel::flaecheHinzufuegen(const Flaeche& f, const Kontext& k) {
    if (m_flaechen.size() == kMaxFlaechen) {
        return nullptr;
    }
    sichern();
    anhaengen(std::make_shared<Flaeche>(f, k));
    m_flaechen.back()->setzeDetail(istDetail());
    geaendert();
    return m_flaechen.back().get();
}

// erase (:2060-2070); gesichert wird hier, damit es rueckgaengig geht.
void Pinsel::flaecheEntfernen(std::size_t index) {
    sichern();
    m_flaechen[index]->abmelden();
    m_flaechen[index]->besitzer = nullptr;
    m_flaechen.erase(m_flaechen.begin() + static_cast<std::ptrdiff_t>(index));
    geaendert();
}

// clear (:2079-2090)
void Pinsel::leeren() {
    sichern();
    for (auto& f : m_flaechen) {
        f->abmelden();
        f->besitzer = nullptr;
    }
    m_flaechen.clear();
    geaendert();
}

// removeEmptyFaces (:2105-2122)
void Pinsel::leereFlaechenEntfernen(const Kontext& k) {
    bRep(k);
    std::size_t i = 0;
    while (i < m_flaechen.size()) {
        if (!m_flaechen[i]->traegtBei()) {
            flaecheEntfernen(i);
        } else {
            ++i;
        }
    }
}

// Brush_ConstructCuboid (brushmanip.cpp:37-74)
void Pinsel::quaderBauen(const Kasten& grenzen, const std::string& shader, const TexturProjektion& projektion,
                         const Kontext& k) {
    const unsigned char box[3][2] = {{0, 1}, {2, 0}, {1, 2}};
    Vektor3f mins(grenzen.origin - grenzen.extents);
    Vektor3f maxs(grenzen.origin + grenzen.extents);
    for (std::size_t i = 0; i < 3; ++i) {
        if (mins[i] < static_cast<float>(kMinWeltKoordinate)) { mins[i] = static_cast<float>(kMinWeltKoordinate); }
        if (maxs[i] > static_cast<float>(kMaxWeltKoordinate)) { maxs[i] = static_cast<float>(kMaxWeltKoordinate); }
    }
    leeren();
    m_flaechen.reserve(6);
    for (int i = 0; i < 3; ++i) {
        Vektor3f p1(maxs);
        Vektor3f p2(maxs);
        p2[box[i][0]] = mins[box[i][0]];
        p1[box[i][1]] = mins[box[i][1]];
        ebeneHinzufuegen(maxs, p1, p2, shader, projektion, k);
    }
    for (int i = 0; i < 3; ++i) {
        Vektor3f p1(mins);
        Vektor3f p2(mins);
        p1[box[i][0]] = maxs[box[i][0]];
        p2[box[i][1]] = maxs[box[i][1]];
        ebeneHinzufuegen(mins, p1, p2, shader, projektion, k);
    }
}

void Pinsel::setzeDetail(bool detail) {
    for (auto& f : m_flaechen) {
        f->setzeDetail(detail);
    }
}

// Brush::transform (:1850-1857) + freezeTransform
void Pinsel::transformieren(const Matrix4& m, const Kontext& k) {
    bRep(k);   // Schwerpunkte und Wicklungen fuer den Texture Lock
    const bool spiegeln = haendigkeit(m) == Haendigkeit::Links;
    for (auto& f : m_flaechen) {
        f->transformieren(m, spiegeln, k);
    }
    geaendert();
}

// plane_unique (:2331-2340)
bool Pinsel::ebeneEinzigartig(std::size_t index) const {
    for (std::size_t i = 0; i < m_flaechen.size(); ++i) {
        if (index != i && !ebeneInnen(m_flaechen[index]->ebene(), m_flaechen[i]->ebene(), index < i)) {
            return false;
        }
    }
    return true;
}

// windingForClipPlane (:2125-2188)
void Pinsel::wicklungFuerSchnittEbene(Wicklung& w, const Ebene3d& e) const {
    FesteWicklung puffer[2];
    bool tausch = false;
    // ein Polygon, das praktisch unendlich gross ist
    wicklungUnendlich(puffer[tausch], e, kMaxWeltKoordinate);
    // mit allen anderen Flaechen zuschneiden
    for (std::size_t i = 0; i < m_flaechen.size(); ++i) {
        const Flaeche& schnitt = *m_flaechen[i];
        if (ebenenGleich(schnitt.ebene(), e) || !ebeneGueltig(schnitt.ebene()) || !ebeneEinzigartig(i)
            || ebenenGegenueber(e, schnitt.ebene())) {
            continue;
        }
        if (puffer[tausch].points.empty()) {
            break;
        }
        puffer[!tausch].clear();
        {
            // umgedreht, weil die Rueckseite bleiben soll
            const Ebene3d schnittEbene(-schnitt.ebene().normal(), -schnitt.ebene().dist());
            wicklungSchneiden(puffer[tausch], e, schnittEbene, i, puffer[!tausch]);
        }
        tausch = !tausch;
    }
    wicklungAusFester(w, puffer[tausch]);
}

// removeDegenerateEdges (:2343-2368)
void Pinsel::entarteteKantenEntfernen() {
    for (std::size_t i = 0; i < m_flaechen.size(); ++i) {
        Wicklung& w = m_flaechen[i]->wicklungRoh();
        for (WicklungsPunkt* j = w.begin(); j != w.end();) {
            const std::size_t index = static_cast<std::size_t>(j - w.begin());
            const std::size_t next = wicklungNaechster(w, index);
            if (kanteEntartet(w[index].vertex, w[next].vertex)) {
                Wicklung& andere = m_flaechen[w[index].adjacent]->wicklungRoh();
                const std::size_t nachbar = wicklungFindeNachbar(andere, i);
                if (nachbar != kMaxFlaechen) {
                    andere.erase(andere.begin() + nachbar);
                }
                w.erase(j);
            } else {
                ++j;
            }
        }
    }
}

// removeDegenerateFaces (:2371-2407): Flaechen mit zwei Punkten ("Kante")
// ungueltig machen, die Nachbarschaft bleibt erhalten.
void Pinsel::entarteteFlaechenEntfernen() {
    for (std::size_t i = 0; i < m_flaechen.size(); ++i) {
        Wicklung& degen = m_flaechen[i]->wicklungRoh();
        if (degen.numpoints == 2) {
            {
                Wicklung& w = m_flaechen[degen[0].adjacent]->wicklungRoh();
                const std::size_t index = wicklungFindeNachbar(w, i);
                if (index != kMaxFlaechen) {
                    w[index].adjacent = degen[1].adjacent;
                }
            }
            {
                Wicklung& w = m_flaechen[degen[1].adjacent]->wicklungRoh();
                const std::size_t index = wicklungFindeNachbar(w, i);
                if (index != kMaxFlaechen) {
                    w[index].adjacent = degen[0].adjacent;
                }
            }
            degen.resize(0);
        }
    }
}

// removeDuplicateEdges (:2410-2433)
void Pinsel::doppelteKantenEntfernen() {
    for (auto& f : m_flaechen) {
        Wicklung& w = f->wicklungRoh();
        for (std::size_t j = 0; j != w.numpoints;) {
            const std::size_t next = wicklungNaechster(w, j);
            if (w[j].adjacent == w[next].adjacent) {
                w.erase(w.begin() + next);
            } else {
                ++j;
            }
        }
    }
}

// verifyConnectivityGraph (:2436-2463): einseitige Kanten entfernen.
void Pinsel::verbindungenPruefen() {
    for (std::size_t i = 0; i < m_flaechen.size(); ++i) {
        Wicklung& w = m_flaechen[i]->wicklungRoh();
        for (WicklungsPunkt* j = w.begin(); j != w.end();) {
            if (j->adjacent == kMaxFlaechen || wicklungFindeNachbar(m_flaechen[j->adjacent]->wicklung(), i) == kMaxFlaechen) {
                w.erase(j);
            } else {
                ++j;
            }
        }
    }
}

// isBounded (:2466-2474)
bool Pinsel::istBegrenzt() const {
    return std::all_of(m_flaechen.begin(), m_flaechen.end(), [](const auto& f) { return f->istBegrenzt(); });
}

// buildWindings (:2477-2523)
bool Pinsel::wicklungenBauen(const Kontext& k) {
    m_grenzen = Kasten();
    for (std::size_t i = 0; i < m_flaechen.size(); ++i) {
        Flaeche& f = *m_flaechen[i];
        if (!ebeneGueltig(f.ebene()) || !ebeneEinzigartig(i)) {
            f.wicklungRoh().resize(0);
        } else {
            wicklungFuerSchnittEbene(f.wicklungRoh(), f.ebene());
            for (const auto& v : f.wicklung()) {
                kastenErweiternSicher(m_grenzen, v.vertex);
            }
            f.texturkoordinatenSetzen(k);
        }
    }
    const bool entartet = !istBegrenzt();
    if (!entartet) {
        // die Reihenfolge der Aufraeumschritte ist wichtig
        entarteteKantenEntfernen();
        entarteteFlaechenEntfernen();
        doppelteKantenEntfernen();
        verbindungenPruefen();
    }
    return entartet;
}

std::size_t Pinsel::absoluterIndex(FlaechenPunkt fp) const {   // absoluteIndex (:1914-1921)
    std::size_t index = 0;
    for (std::size_t i = 0; i < fp.flaeche; ++i) {
        index += m_flaechen[i]->wicklung().numpoints;
    }
    return index + fp.punkt;
}

// buildBRep (brush.cpp:98-332)
void Pinsel::bRepBauen(const Kontext& k) {
    const bool entartet = wicklungenBauen(k);
    m_entartet = entartet;
    m_eulerFehler = false;

    std::size_t flaechenAnzahl = 0;
    std::size_t punkteAnzahl = 0;
    for (const auto& f : m_flaechen) {
        if (f->traegtBei()) { ++flaechenAnzahl; }
        punkteAnzahl += f->wicklung().numpoints;
    }

    // Summe der Punkte aller Flaechen eines gueltigen Polyeders ist gerade
    if (entartet || flaechenAnzahl < 4 || punkteAnzahl != (punkteAnzahl >> 1) << 1) {
        m_ecken.clear();
        m_kanten.clear();
        m_kantenFlaechen.clear();
        m_kantenIndizes.clear();
        for (auto& f : m_flaechen) {
            f->wicklungRoh().resize(0);
        }
        return;
    }

    std::vector<FlaechenPunkt> flaechenPunkte;
    flaechenPunkte.reserve(punkteAnzahl);
    for (std::size_t i = 0; i != m_flaechen.size(); ++i) {
        for (std::size_t j = 0; j < m_flaechen[i]->wicklung().numpoints; ++j) {
            flaechenPunkte.push_back(FlaechenPunkt{i, j});
        }
    }

    // Kanten: Kantenpaare als Ring, eindeutig ueber ProximalVertex
    std::vector<unsigned int> kantenIndex;
    std::vector<NaheEcke> eindeutigeKanten;
    kantenIndex.reserve(flaechenPunkte.size());
    eindeutigeKanten.reserve(flaechenPunkte.size());
    std::vector<SListNode> kantenPaare(flaechenPunkte.size());
    for (std::size_t i = 0; i < flaechenPunkte.size(); ++i) {
        kantenPaare[i].next = kantenPaare.data() + absoluterIndex(naechsteKante(m_flaechen, flaechenPunkte[i]));
    }
    {
        EindeutigePuffer<NaheEcke> einfueger(eindeutigeKanten);
        for (const auto& knoten : kantenPaare) {
            kantenIndex.push_back(einfueger.einfuegen(NaheEcke{&knoten}));
        }
    }
    m_kanten.clear();
    m_kanten.reserve(eindeutigeKanten.size());
    for (const auto& v : eindeutigeKanten) {
        m_kanten.push_back(flaechenPunkte[static_cast<std::size_t>(v.knoten - kantenPaare.data())]);
    }
    m_kantenFlaechen.resize(eindeutigeKanten.size());
    for (std::size_t i = 0; i < eindeutigeKanten.size(); ++i) {
        const FlaechenPunkt fp = flaechenPunkte[static_cast<std::size_t>(eindeutigeKanten[i].knoten - kantenPaare.data())];
        m_kantenFlaechen[i] = KantenFlaechen{fp.flaeche, m_flaechen[fp.flaeche]->wicklung()[fp.punkt].adjacent};
    }

    // Ecken: Eckenringe
    std::vector<unsigned int> eckenIndex;
    std::vector<NaheEcke> eindeutigeEcken;
    eckenIndex.reserve(flaechenPunkte.size());
    eindeutigeEcken.reserve(flaechenPunkte.size());
    std::vector<SListNode> eckenRinge(flaechenPunkte.size());
    for (std::size_t i = 0; i < flaechenPunkte.size(); ++i) {
        eckenRinge[i].next = eckenRinge.data() + absoluterIndex(naechsteEcke(m_flaechen, flaechenPunkte[i]));
    }
    {
        EindeutigePuffer<NaheEcke> einfueger(eindeutigeEcken);
        for (const auto& knoten : eckenRinge) {
            eckenIndex.push_back(einfueger.einfuegen(NaheEcke{&knoten}));
        }
    }
    m_ecken.clear();
    m_ecken.reserve(eindeutigeEcken.size());
    for (const auto& v : eindeutigeEcken) {
        m_ecken.push_back(flaechenPunkte[static_cast<std::size_t>(v.knoten - eckenRinge.data())]);
    }

    // NRC: "Final B-Rep: inconsistent vertex count"
    if ((eindeutigeEcken.size() + flaechenAnzahl) - eindeutigeKanten.size() != 2) {
        m_eulerFehler = true;
    }

    // Kantenliste fuer die Drahtgitter-Darstellung
    m_kantenIndizes.assign(kantenIndex.size(), KantenIndizes{});
    for (std::size_t i = 0, zaehler = 0; i < m_flaechen.size(); ++i) {
        const Wicklung& w = m_flaechen[i]->wicklung();
        for (std::size_t j = 0; j < w.numpoints; ++j) {
            const unsigned int kante = kantenIndex[zaehler + j];
            m_kantenIndizes[kante].erste = eckenIndex[zaehler + j];
            m_kantenIndizes[kante].zweite = eckenIndex[zaehler + wicklungNaechster(w, j)];
        }
        zaehler += w.numpoints;
    }
    m_kantenIndizes.resize(eindeutigeKanten.size());

    for (auto& f : m_flaechen) {
        f->schwerpunktBerechnen();
    }
}

void Pinsel::bRep(const Kontext& k) const {   // evaluateBRep (:1788-1793)
    if (m_ebeneGeaendert) {
        m_ebeneGeaendert = false;
        const_cast<Pinsel*>(this)->bRepBauen(k);
    }
}

bool Pinsel::hatBeitragendeFlaechen(const Kontext& k) const {   // hasContributingFaces (:2099-2101)
    bRep(k);
    return std::any_of(m_flaechen.begin(), m_flaechen.end(), [](const auto& f) { return f->traegtBei(); });
}

// contributes (:2233-2298): nur pruefen, ob der Pinsel Sinn ergibt.
bool Pinsel::traegtBei() const {
    std::size_t beitragend = 0;
    for (std::size_t i = 0; i < m_flaechen.size(); ++i) {
        const Flaeche& f = *m_flaechen[i];
        if (!(ebeneGueltig(f.ebene()) && ebeneEinzigartig(i))) { continue; }
        FesteWicklung puffer[2];
        bool tausch = false;
        wicklungUnendlich(puffer[tausch], f.ebene(), kMaxWeltKoordinate);
        for (std::size_t j = 0; j < m_flaechen.size(); ++j) {
            const Flaeche& schnitt = *m_flaechen[j];
            if (ebenenGleich(schnitt.ebene(), f.ebene()) || !ebeneGueltig(schnitt.ebene()) || !ebeneEinzigartig(j)
                || ebenenGegenueber(f.ebene(), schnitt.ebene())) {
                continue;
            }
            if (puffer[tausch].points.empty()) { break; }
            puffer[!tausch].clear();
            const Ebene3d schnittEbene(-schnitt.ebene().normal(), -schnitt.ebene().dist());
            wicklungSchneiden(puffer[tausch], f.ebene(), schnittEbene, j, puffer[!tausch]);
            tausch = !tausch;
        }
        if (puffer[tausch].size() > 2) {
            ++beitragend;
            if (beitragend > 3) { return true; }
        }
    }
    return false;
}

std::unique_ptr<Memento> Pinsel::zustandSichern() const {
    auto m = std::make_unique<PinselMemento>();
    m->flaechen = m_flaechen;
    return m;
}

// importState (:1957-1966) mit appendFaces (:1923-1929)
void Pinsel::zustandLaden(const Memento& mm) {
    sichern();
    const auto& m = static_cast<const PinselMemento&>(mm);
    for (auto& f : m_flaechen) {
        f->abmelden();
        f->besitzer = nullptr;
    }
    m_flaechen.clear();
    for (const auto& f : m.flaechen) {
        anhaengen(f);
    }
    geaendert();
}

void Pinsel::anmeldenMitFlaechen(Rueckgaengig* u) {
    anmelden(u);
    for (auto& f : m_flaechen) {
        f->besitzer = this;
        f->anmelden(u);
    }
}

void Pinsel::abmeldenMitFlaechen() {
    for (auto& f : m_flaechen) {
        f->abmelden();
    }
    abmelden();
}

}  // namespace bhed::karte
