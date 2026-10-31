// saberspuren.h - Paket saber (rc601, Review): Brandspuren der Lichtschwerter
// an Waenden, wie im Spiel.
//
// Was das Spiel tut, wenn eine Klinge an einer Wand entlangfaehrt
// (CG_AddSaberBladeGo, cg_players.cpp:14404 MD, 6489 OJ): es ruft
// CG_CreateSaberMarks( oldPos, endpos, normal ) (cg_players.cpp:13689 MD,
// 5847 OJ). Das legt ein schmales Viereck (Radius 0.65) von der letzten zur
// jetzigen Trefferstelle, schneidet es mit cgi_CM_MarkFragments - im Renderer
// R_MarkFragments (rd-vanilla/tr_marks.cpp) - auf die Flaechen der Karte und
// legt je Bruchstueck ZWEI Marken in den Markenspeicher (cg_marks.cpp): die
// Brandspur (gfx/damage/rivetmark, 10 s) und das Gluehen
// (gfx/effects/saberDamageGlow, Startzeit 8500 ms frueher, also 1,5 s, ab
// 0,5 s abblendend). Der Speicher hat 256 Plaetze (MAX_MARK_POLYS,
// cg_local.h:71); ist er voll, faellt die aelteste Gruppe gleicher Zeit
// (CG_AllocMark). CG_AddMarks wirft jedes Bild ab, was aelter als 10 s ist,
// und setzt die Farben.
//
// Movie Duels laeuft mit rd-vanilla (com_rend2 "0", cg_main.cpp:787 MD) - dessen
// tr_marks.cpp ist dieselbe wie in OpenJK. Unterschiede gibt es nur in
// CG_CreateSaberMarks (siehe dort).
//
// Alles hier ist rein: keine App, kein Fenster. Die Welt kommt als SpurWelt
// (aus der .bsp gebaut, spurWeltAus), der Zufall (Q_flrand) als Saat.
#ifndef BHED_SABERSPUREN_H
#define BHED_SABERSPUREN_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace bhed {

struct BspGeometry;

// --- Die Welt, wie R_MarkFragments sie sieht ---------------------------------
//
// cplane_t mit type (PlaneTypeForNormal) und signbits (SetPlaneSignbits).
struct SpurEbene {
    float normale[3]{};
    float abstand = 0.0F;
    int typ = 3;          // PLANE_X 0, PLANE_Y 1, PLANE_Z 2, PLANE_NON_AXIAL 3
    int vorzeichen = 0;   // signbits
};
[[nodiscard]] SpurEbene spurEbene(const float normale[3], float abstand) noexcept;
// BoxOnPlaneSide (q_math.c:948 OJ): 1 vorn, 2 hinten, 3 beides.
[[nodiscard]] int spurKastenSeite(const float mins[3], const float maxs[3], const SpurEbene& p) noexcept;
// Q_rsqrt (q_math.c:360 OJ, eine Newton-Stufe) - VectorNormalizeFast.
[[nodiscard]] float spurRsqrt(float zahl) noexcept;

struct SpurFlaeche {
    // surfaceType_t: SF_FACE, SF_GRID, SF_TRIANGLES; alles andere (Flares,
    // SF_SKIP fuer NODRAW-Patches) laesst R_BoxSurfaces_r liegen.
    enum class Art : std::uint8_t { Keine, Flaeche, Gitter, Dreiecke };
    Art art = Art::Keine;
    // shader->surfaceFlags & (SURF_NOIMPACT | SURF_NOMARKS) oder
    // shader->contentFlags & CONTENTS_FOG (tr_marks.cpp:150 OJ).
    bool ausgenommen = false;
    SpurEbene ebene;                            // nur Flaeche (srfSurfaceFace_t::plane)
    std::vector<std::array<float, 3>> punkte;   // Flaeche/Dreiecke: Ecken; Gitter: verts
    std::vector<int> indizes;                   // Flaeche/Dreiecke
    int breite = 0;                             // Gitter
    int hoehe = 0;
    float mins[3]{};                            // Kasten der Punkte (nur zum Vorsortieren)
    float maxs[3]{};
};

struct SpurWelt {
    std::vector<SpurEbene> ebenen;
    struct Knoten {
        int ebene = 0;
        int kinder[2]{};   // >= 0 Knoten, < 0 Blatt -(n + 1) wie dnode_t
    };
    std::vector<Knoten> knoten;
    struct Blatt {
        int erste = 0;
        int zahl = 0;
    };
    std::vector<Blatt> blaetter;
    std::vector<int> blattFlaechen;   // leafsurfaces
    std::vector<SpurFlaeche> flaechen;
    // surf->viewCount / tr.viewCount
    mutable std::vector<int> gesehen;
    mutable int zaehler = 0;
    [[nodiscard]] bool leer() const noexcept { return knoten.empty() || flaechen.empty(); }
};

// Die Welt einer Karte: Knoten, Blaetter, Flaechen; Patches werden mit
// R_SubdividePatchToGrid (r_subdivisions "4", tr_init.cpp:1543 OJ, :1612 MD)
// zu Gittern. Die Flags der Shader kommen aus dem Shaderverzeichnis der .bsp
// (q3map2 schreibt dort die surfaceparms der Shaderdateien hinein).
[[nodiscard]] SpurWelt spurWeltAus(const BspGeometry& geo);

// --- Die Flaechen einer Welt ohne .bsp (Karten-Editor, reste1 Punkt 1) -------
//
// Der Karten-Editor hat keine .bsp. Die Flaechen kommen fertig herein: eine
// ebene Flaeche wie srfSurfaceFace_t (Ecken, Faecher 0, i, i + 1, Ebene aus der
// Normalen und der ersten Ecke), ein Patch als Gitter wie im Spiel
// (R_SubdividePatchToGrid, r_subdivisions "4"), eine Dreiecksflaeche wie
// srfTriangles_t (reste2 Punkt 4: misc_model, die q3map2 in die Welt setzt).
// Den Baum mit den Verweisen der Blaetter baut markenWeltAusKarte
// (bhed/karte/spielkollision.h) wie q3map2.
[[nodiscard]] SpurFlaeche spurPolygon(const std::vector<std::array<float, 3>>& ecken, const float normale[3], bool ausgenommen);
// Eine ebene Flaeche mit eigener Dreiecksliste (ParseFace: die Ebene aus der Normale und dem ersten Punkt)
[[nodiscard]] SpurFlaeche spurFlaecheIndiziert(const std::vector<std::array<float, 3>>& punkte, const std::vector<int>& indizes, const float normale[3], bool ausgenommen);
[[nodiscard]] SpurFlaeche spurPatch(int breite, int hoehe, const std::vector<std::array<float, 3>>& kontrolle, bool ausgenommen);
[[nodiscard]] SpurFlaeche spurDreiecke(const std::vector<std::array<float, 3>>& punkte, const std::vector<int>& indizes, bool ausgenommen);

// R_SubdividePatchToGrid (tr_curve.cpp:287 OJ), nur die Orte. punkte: breite x
// hoehe Kontrollpunkte (Zeile fuer Zeile). Liefert das Gitter wie
// srfGridMesh_t::verts (Zeile j, Spalte i an j * breiteAus + i).
void spurGitter(int breite, int hoehe, const std::vector<std::array<float, 3>>& punkte, float unterteilung,
                std::vector<std::array<float, 3>>& aus, int& breiteAus, int& hoeheAus);

struct SpurBruchstueck {   // markFragment_t
    int ersterPunkt = 0;
    int zahl = 0;
};
// R_MarkFragments (tr_marks.cpp:254 OJ) - woertlich, mit einer reinen
// Abkuerzung: Dreiecke, die ganz ausserhalb des um 32 vergroesserten Kastens
// liegen, werden nicht erst zerschnitten (so auch die rend2-Fassung von Movie
// Duels); was die Schnitte uebrig liessen, laege ohnehin darin - das
// Ergebnis ist dasselbe.
int spurBruchstuecke(const SpurWelt& welt, int zahlPunkte, const float (*punkte)[3], const float projektion[3],
                     int maxPunkte, float (*punktPuffer)[3], int maxBruchstuecke, SpurBruchstueck* bruchstuecke);

// --- Der Markenspeicher (cg_marks.cpp) ---------------------------------------
inline constexpr int kSpurMaxPolys = 256;        // MAX_MARK_POLYS (cg_local.h:71)
inline constexpr int kSpurMaxEcken = 10;         // MAX_VERTS_ON_POLY (cg_local.h:70)
inline constexpr int kSpurGesamtZeit = 10000;    // MARK_TOTAL_TIME (cg_marks.cpp)
inline constexpr int kSpurBlendZeit = 1000;      // MARK_FADE_TIME

struct SpurPoly {   // markPoly_t
    int zeit = 0;                 // mark->time
    bool alphaFade = false;
    // 0 gfx/damage/rivetmark, 1 gfx/effects/saberDamageGlow; Pruefung 3: ab
    // kSpurShaderEffekt der Shader eines Effekt-Decals (spurShaderNummer)
    int shader = 0;
    int zahl = 0;                 // poly.numVerts
    float xyz[kSpurMaxEcken][3]{};
    float st[kSpurMaxEcken][2]{};
    std::uint8_t farbe[4]{};      // mark->color
    // Pruefung 3: verts[].modulate[3] beim Anlegen - CG_ImpactMark schreibt
    // colors[3] hinein (cg_marks.cpp:153-157 OJ), CG_CreateSaberMarks 0 (s. u.)
    std::uint8_t eckenAlpha = 0;
};
// Pruefung 3: ein Decal aus einem Effekt (FxScheduler -> CG_ImpactMark) liegt
// im selben Speicher wie die Brandspuren der Schwerter (cg_markPolys, 256
// Plaetze, CG_AllocMark). Sein Shader ist eine Nummer ab kSpurShaderEffekt,
// die spurShaderNummer je Name einmal vergibt; spurShaderName gibt ihn zurueck.
inline constexpr int kSpurShaderEffekt = 2;
[[nodiscard]] int spurShaderNummer(const std::string& name);
[[nodiscard]] std::string spurShaderName(int nummer);
// Die Farbe, mit der CG_AddMarks die Ecken zur Zeit cgZeit zeichnet
// (verts[].modulate). Die Alpha bleibt ausserhalb des Abblendens beim Wert,
// den die Marke beim Anlegen in die Ecken schrieb (eckenAlpha): bei
// CG_CreateSaberMarks setzt Movie Duels sie mit "polyVert_t verts[]{}" auf 0,
// OpenJK laesst sie ungesetzt - beide Shader lesen sie nicht (rivetmark:
// blendFunc GL_DST_COLOR GL_SRC_COLOR, saberDamageGlow: GL_ONE GL_ONE), dort
// steht 0; CG_ImpactMark schreibt die Alpha des Decals.
void spurFarbe(const SpurPoly& p, int cgZeit, std::uint8_t aus[4]) noexcept;

class SpurSpeicher {
public:
    SpurSpeicher();
    // CG_AllocMark: gelingt immer; ist kein Platz frei, faellt die aelteste
    // Gruppe gleicher Zeit.
    SpurPoly& neu();
    // CG_AddMarks: alles fort, was aelter als MARK_TOTAL_TIME ist.
    void bild(int cgZeit);
    // Die lebenden Marken, neueste zuerst (Zeichenreihenfolge von CG_AddMarks).
    [[nodiscard]] std::vector<const SpurPoly*> lebende() const;
    [[nodiscard]] int zahl() const noexcept { return m_zahl; }

private:
    void frei(int i);
    std::array<SpurPoly, kSpurMaxPolys> m_polys{};
    std::array<int, kSpurMaxPolys> m_vor{};
    std::array<int, kSpurMaxPolys> m_nach{};
    std::array<bool, kSpurMaxPolys> m_aktiv{};
    int m_kopf = -1;     // cg_activeMarkPolys.nextMark (neueste)
    int m_schwanz = -1;  // cg_activeMarkPolys.prevMark (aelteste)
    int m_frei = 0;      // cg_freeMarkPolys
    std::array<int, kSpurMaxPolys> m_freiNach{};
    int m_zahl = 0;
};

// CG_CreateSaberMarks (cg_players.cpp:13689 MD, 5847 OJ), Regelwerk nach
// saberMovieDuelsRegeln(). cgZeit ist cg.time des Bildes; saat ersetzt
// Q_flrand. Haengt die neuen Marken an `neu` an (in der Reihenfolge, in der
// das Spiel CG_AllocMark ruft) - der Aufrufer legt sie in den Speicher.
void saberSpurenErzeugen(const SpurWelt& welt, const float start[3], const float ende[3], const float normale[3],
                         int cgZeit, std::uint64_t saat, std::vector<SpurPoly>& neu);

}  // namespace bhed

#endif
