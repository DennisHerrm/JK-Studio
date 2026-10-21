// pinsel.h - Karten-Editor: Pinsel (konvexer Koerper aus Ebenen) und Flaechen.
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): radiant/brush.h,
// radiant/brush.cpp (Face, FacePlane, FaceShader, FaceTexdef, Brush,
// buildWindings, buildBRep), radiant/brushmanip.cpp (Brush_ConstructCuboid),
// libs/render.h (UniqueVertexBuffer, Auszug).
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
//
// Aufbau wie NRC: eine Flaeche ist drei Ebenenpunkte (in double), ein
// Shadername ("textures/..."), Inhalts-/Oberflaechenflags und eine
// Texturprojektion. Der Pinsel baut daraus die Wicklungen (buildWindings),
// raeumt entartete Kanten und Flaechen auf und bildet Kanten- und Eckenlisten
// (buildBRep) - die braucht die Auswahl von Kanten und Ecken in Stufe 1b.
//
// Was NRC in globalen Zustaenden haelt (Brush::m_type, g_bp_globals, der
// Shadercache mit den Texturgroessen, g_brush_texturelock_enabled), steht hier
// in einem Kontext, der den Funktionen mitgegeben wird.
#ifndef BHED_KARTE_PINSEL_H
#define BHED_KARTE_PINSEL_H

#include "bhed/karte/mathe.h"
#include "bhed/karte/rueckgaengig.h"
#include "bhed/karte/texdef.h"
#include "bhed/karte/wicklung.h"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace bhed::karte {

constexpr unsigned kDetailBit = 27;              // BRUSH_DETAIL_FLAG
constexpr int kDetailMaske = 1 << kDetailBit;    // BRUSH_DETAIL_MASK

constexpr double kMaxWeltKoordinate = 64 * 1024;    // g_MaxWorldCoord (map.h:98)
constexpr double kMinWeltKoordinate = -64 * 1024;   // g_MinWorldCoord
constexpr const char* kTexturPraefix = "textures/";  // GlobalTexturePrefix_get (Quake3-Shadersystem)

// Die drei Pinselformate eines Quake3-Spiels (EBrushType). NRC erkennt sie
// beim Lesen am ersten Pinsel (plugins/mapq3/plugin.cpp).
enum class PinselFormat { Quake3, Quake3BP, Quake3Valve220 };

// BrushType_getTexdefType
inline TexdefArt texdefArtFuer(PinselFormat f) {
    switch (f) {
    case PinselFormat::Quake3BP: return TexdefArt::BrushPrimitives;
    case PinselFormat::Quake3Valve220: return TexdefArt::Valve;
    default: return TexdefArt::Quake;
    }
}

// Texturgroesse eines Shaders (FaceShader::width/height). Ohne Angabe gilt
// 64x64 - das ist NRCs Ersatzbild bitmaps/notex.png (texwindow.cpp:1650).
class TexturGroessen {
public:
    virtual ~TexturGroessen() = default;
    [[nodiscard]] virtual std::pair<std::size_t, std::size_t> groesse(const std::string& shader) const = 0;
};

struct Kontext {
    PinselFormat format = PinselFormat::Quake3;
    const TexturGroessen* texturen = nullptr;
    bool texturLock = false;   // g_brush_texturelock_enabled ("TogTexLock", Shift+T)
    [[nodiscard]] TexdefArt art() const { return texdefArtFuer(format); }
    [[nodiscard]] std::pair<std::size_t, std::size_t> groesse(const std::string& shader) const {
        if (texturen) { return texturen->groesse(shader); }
        return {64, 64};
    }
};

// ContentsFlagsValue (brush.h:271-298)
struct InhaltFlags {
    int flaechenFlags = 0;   // m_surfaceFlags
    int inhaltFlags = 0;     // m_contentFlags
    int wert = 0;            // m_value
    bool angegeben = false;  // m_specified
};

class Pinsel;

// Face (brush.h:884-1359) mit FacePlane, FaceShader und FaceTexdef.
class Flaeche final : public Rueckgaengigbar {
public:
    Flaeche();
    // Face(p0, p1, p2, shader, projection): die Projektion kommt in
    // Texturgroessen-Masstab und bekommt addScale (FaceTexdef::realiseShader).
    Flaeche(const Vektor3d& p0, const Vektor3d& p1, const Vektor3d& p2, const std::string& shader,
            const TexturProjektion& projektion, const Kontext& k);
    // Face(other): Kopie (der Basis wird nicht zurueckgesetzt).
    Flaeche(const Flaeche& andere, const Kontext& k);

    // --- FacePlane ---
    [[nodiscard]] const EbenenPunkte& ebenenPunkte() const { return m_planepts; }
    [[nodiscard]] const Ebene3d& ebene() const { return m_plane; }   // plane3()
    void setzeEbenenPunkte(const EbenenPunkte& p);   // FacePlane::copy + planeChanged
    // Nur fuer den Leser: Punkte setzen und MakePlane, ohne Undo.
    void ladeEbenenPunkte(const EbenenPunkte& p);
    void umdrehen();   // flipWinding / FacePlane::reverse

    // --- FaceShader ---
    [[nodiscard]] const std::string& shader() const { return m_shader; }
    void setzeShader(const std::string& name, const Kontext& k);   // SetShader
    void ladeShader(const std::string& name) { m_shader = name; }
    [[nodiscard]] const InhaltFlags& flags() const { return m_flags; }
    InhaltFlags& flagsRoh() { return m_flags; }   // fuer den Leser
    void setzeFlags(const InhaltFlags& f);       // SetFlags (Detail bleibt)
    [[nodiscard]] bool istDetail() const { return (m_flags.inhaltFlags & kDetailMaske) != 0; }
    void setzeDetail(bool detail);               // setDetail

    // --- FaceTexdef ---
    [[nodiscard]] const TexturProjektion& projektion() const { return m_projektion; }
    TexturProjektion& projektionRoh() { return m_projektion; }   // fuer den Leser
    bool projektionInitialisiert = false;   // m_projectionInitialised
    bool skalierungAngewandt = false;       // m_scaleApplied
    void setzeBasis(const Vektor3f& normal);   // FaceTexdef::setBasis (ComputeAxisBase)
    // GetTexdef: Projektion ohne Texturgroessen-Masstab (normalised).
    [[nodiscard]] TexturProjektion texdefHolen(const Kontext& k) const;
    // SetTexdef / ShiftTexdef / ScaleTexdef / RotateTexdef / FitTexture /
    // ProjectTexture / Convert - jeweils mit Undo.
    void setzeTexdef(const TexturProjektion& p, const Kontext& k, bool basisSetzen = true, bool basisZuruecksetzen = false);
    void setzeTexdef(const float* hShift, const float* vShift, const float* hScale, const float* vScale,
                     const float* rotation, const Kontext& k);
    void verschiebeTextur(float s, float t, const Kontext& k);
    void skaliereTextur(float s, float t, const Kontext& k);
    void dreheTextur(float winkel, const Kontext& k);
    void passeTexturEin(float sWiederholung, float tWiederholung, bool nurAbmessung, const Kontext& k);
    void projiziereTextur(const Texdef& td, const Vektor3f* richtung, const Kontext& k);
    void projiziereTextur(const TexturProjektion& p, const Vektor3f& normal, const Kontext& k);
    void rechneTexdefUm(TexdefArt von, TexdefArt nach, const Kontext& k);   // Face::Convert

    // --- Transformation (Face::transform + freezeTransform) ---
    void transformieren(const Matrix4& m, bool spiegeln, const Kontext& k);

    // --- Wicklung ---
    [[nodiscard]] const Wicklung& wicklung() const { return m_wicklung; }
    Wicklung& wicklungRoh() { return m_wicklung; }
    [[nodiscard]] const Vektor3f& schwerpunkt() const { return m_schwerpunkt; }
    void schwerpunktBerechnen();   // construct_centroid
    void texturkoordinatenSetzen(const Kontext& k);   // EmitTextureCoordinates
    [[nodiscard]] bool traegtBei() const { return m_wicklung.numpoints > 2; }   // contributes
    [[nodiscard]] bool istBegrenzt() const;   // is_bounded

    // Undo
    [[nodiscard]] std::unique_ptr<Memento> zustandSichern() const override;
    void zustandLaden(const Memento& m) override;

    Pinsel* besitzer = nullptr;   // FaceObserver

private:
    void ebeneGeaendert();   // planeChanged

    EbenenPunkte m_planepts{};
    Ebene3d m_plane;
    std::string m_shader = kTexturPraefix;   // texdef_name_default
    InhaltFlags m_flags;
    TexturProjektion m_projektion;
    Wicklung m_wicklung;
    Vektor3f m_schwerpunkt;
};

// Eine eindeutige Kante bzw. Ecke des Pinsels (SelectableEdge / SelectableVertex):
// Flaeche und Punkt in deren Wicklung.
struct FlaechenPunkt {   // FaceVertexId
    std::size_t flaeche = 0;
    std::size_t punkt = 0;
};
struct KantenFlaechen {   // EdgeFaces
    std::size_t erste = kMaxFlaechen;
    std::size_t zweite = kMaxFlaechen;
};
struct KantenIndizes {   // EdgeRenderIndices
    unsigned int erste = 0;
    unsigned int zweite = 0;
};

class Primitiv : public Rueckgaengigbar {
public:
    enum class Art { Pinsel, Patch };
    [[nodiscard]] virtual Art art() const = 0;
    // Node::m_layer: Index in Karte::schichten; kSchichtUnerreichbar fuer eine
    // Ebene, die NRC beim Lesen nicht in den Baum haengen konnte (Kreis).
    int schicht = 0;
};
constexpr int kSchichtKeine = -1;          // m_layer == nullptr (Gruppenentitaeten)
constexpr int kSchichtUnerreichbar = -2;   // m_ownIndex bleibt LAYERIDXPARENT (-1)

// Brush (brush.h:1553-2527)
class Pinsel final : public Primitiv {
public:
    using Flaechen = std::vector<std::shared_ptr<Flaeche>>;

    Pinsel() = default;
    [[nodiscard]] Art art() const override { return Art::Pinsel; }
    // Brush::copy: tiefe Kopie
    [[nodiscard]] std::shared_ptr<Pinsel> kopie(const Kontext& k) const;

    [[nodiscard]] const Flaechen& flaechen() const { return m_flaechen; }
    [[nodiscard]] std::size_t anzahl() const { return m_flaechen.size(); }
    [[nodiscard]] bool leer() const { return m_flaechen.empty(); }
    Flaeche& operator[](std::size_t i) { return *m_flaechen[i]; }
    const Flaeche& operator[](std::size_t i) const { return *m_flaechen[i]; }

    // Fuer den Leser: Flaeche anhaengen, ohne Undo (push_back).
    Flaeche& neueFlaecheLaden();
    // addPlane: neue Flaeche aus drei Punkten (mit Undo). nullptr bei 1024 Flaechen.
    Flaeche* ebeneHinzufuegen(const Vektor3d& p0, const Vektor3d& p1, const Vektor3d& p2, const std::string& shader,
                              const TexturProjektion& projektion, const Kontext& k, int inhaltFlags = 0);
    Flaeche* flaecheHinzufuegen(const Flaeche& f, const Kontext& k);   // addFace (Kopie)
    void flaecheEntfernen(std::size_t index);        // erase
    void leeren();                                   // clear
    void leereFlaechenEntfernen(const Kontext& k);   // removeEmptyFaces
    void quaderBauen(const Kasten& grenzen, const std::string& shader, const TexturProjektion& projektion,
                     const Kontext& k);              // Brush_ConstructCuboid

    [[nodiscard]] bool istDetail() const { return !m_flaechen.empty() && m_flaechen.front()->istDetail(); }
    void setzeDetail(bool detail);   // MakeDetail / MakeStructural

    // Brush::transform + freezeTransform (mit Texture Lock, wenn k.texturLock).
    void transformieren(const Matrix4& m, const Kontext& k);

    // B-Rep (buildBRep). Wird nach jeder Aenderung beim naechsten Zugriff
    // neu gebaut (evaluateBRep); bRep() erzwingt es.
    void bRep(const Kontext& k) const;
    void geaendert() { m_ebeneGeaendert = true; }   // planeChanged
    [[nodiscard]] bool entartet(const Kontext& k) const { bRep(k); return m_entartet; }
    [[nodiscard]] const Kasten& grenzen(const Kontext& k) const { bRep(k); return m_grenzen; }   // localAABB
    [[nodiscard]] bool hatBeitragendeFlaechen(const Kontext& k) const;   // hasContributingFaces
    [[nodiscard]] bool traegtBei() const;   // contributes (ohne Auswertung)
    [[nodiscard]] const std::vector<FlaechenPunkt>& ecken(const Kontext& k) const { bRep(k); return m_ecken; }
    [[nodiscard]] const std::vector<FlaechenPunkt>& kanten(const Kontext& k) const { bRep(k); return m_kanten; }
    [[nodiscard]] const std::vector<KantenFlaechen>& kantenFlaechen(const Kontext& k) const { bRep(k); return m_kantenFlaechen; }
    [[nodiscard]] const std::vector<KantenIndizes>& kantenIndizes(const Kontext& k) const { bRep(k); return m_kantenIndizes; }
    // "Final B-Rep: inconsistent vertex count" (brush.cpp:322) - Euler V+F-E != 2
    [[nodiscard]] bool eulerFehler(const Kontext& k) const { bRep(k); return m_eulerFehler; }
    [[nodiscard]] Vektor3d eckpunkt(const FlaechenPunkt& fp) const { return m_flaechen[fp.flaeche]->wicklung()[fp.punkt].vertex; }

    // Undo (BrushUndoMemento: nur die Liste der Flaechen-Zeiger)
    [[nodiscard]] std::unique_ptr<Memento> zustandSichern() const override;
    void zustandLaden(const Memento& m) override;

    // Undo-Anmeldung samt Flaechen (instanceAttach)
    void anmeldenMitFlaechen(Rueckgaengig* u);
    void abmeldenMitFlaechen();

private:
    friend class Flaeche;
    void anhaengen(std::shared_ptr<Flaeche> f);   // push_back
    bool ebeneEinzigartig(std::size_t index) const;   // plane_unique
    void wicklungFuerSchnittEbene(Wicklung& w, const Ebene3d& e) const;   // windingForClipPlane
    void entarteteKantenEntfernen();      // removeDegenerateEdges
    void entarteteFlaechenEntfernen();    // removeDegenerateFaces
    void doppelteKantenEntfernen();       // removeDuplicateEdges
    void verbindungenPruefen();           // verifyConnectivityGraph
    bool istBegrenzt() const;             // isBounded
    bool wicklungenBauen(const Kontext& k);   // buildWindings
    void bRepBauen(const Kontext& k);         // buildBRep
    std::size_t absoluterIndex(FlaechenPunkt fp) const;

    Flaechen m_flaechen;
    mutable bool m_ebeneGeaendert = true;
    bool m_entartet = false;
    bool m_eulerFehler = false;
    Kasten m_grenzen;
    std::vector<FlaechenPunkt> m_ecken;
    std::vector<FlaechenPunkt> m_kanten;
    std::vector<KantenFlaechen> m_kantenFlaechen;
    std::vector<KantenIndizes> m_kantenIndizes;
};

// plane3_inside (brush.h:1445-1454): true, wenn "self" beim Bauen Vorrang hat.
bool ebeneInnen(const Ebene3d& self, const Ebene3d& other, bool selfIsLater);

}  // namespace bhed::karte
#endif
