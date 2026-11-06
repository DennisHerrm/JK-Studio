// modell/g2.h - eine Ghoul2-Instanz wie im Spiel (Modell-Block 1a-1)
//
// Der Bereich "Ansehen/Pruefen" des Model-Tabs zeigt ein Modell wie ModView
// (Raven, JACoders/OpenJK-tools ModView/, GPL v2), aber nach den Regeln der
// Engine. Hier stehen diese Regeln, 1:1 aus OpenJK (code/rd-vanilla,
// codemp/rd-vanilla) und Movie Duels Update 9 portiert:
//
//   Flaechen an/aus     G2_SetSurfaceOnOff (G2_surfaces.cpp:210-251): eine
//                       Ueberschreibung je Flaeche, nur OFF und NODESCENDANTS
//                       kommen von aussen, die anderen Flaggen bleiben.
//   Wurzel              G2_SetRootSurface (G2_surfaces.cpp:296-312)
//   Haut "*off"         G2API_SetSurfaceOnOffFromSkin (tr_ghoul2.cpp:2208-2236)
//   gezeichnet          RenderSurfaces (tr_ghoul2.cpp:2239-2438): ab der
//                       Wurzel rekursiv, gezeichnet wird eine Flaeche mit
//                       offFlags == 0, NODESCENDANTS beendet den Abstieg.
//   LOD                 G2_ComputeLOD (tr_ghoul2.cpp:834-905) mit
//                       ProjectRadius (tr_mesh.cpp:32-67)
//   Grenzen             je Zielspiel aus den Quellen (siehe grenzen())
#ifndef BHED_MODELL_G2_H
#define BHED_MODELL_G2_H

#include <cstdint>
#include <string>
#include <vector>

#include "bhed/gla.h"
#include "bhed/modell/dokument.h"

namespace bhed::modell {

// mdx_format.h (OpenJK code/rd-common): G2SURFACEFLAG_ISBOLT 0x1, _OFF 0x2,
// _NODESCENDANTS 0x100.
inline constexpr std::uint32_t kG2Bolzen = 0x1U;
inline constexpr std::uint32_t kG2Aus = 0x2U;
inline constexpr std::uint32_t kG2OhneKinder = 0x100U;

struct FlaechenSchalter {   // surfaceInfo_t (Flaeche, offFlags)
    int flaeche = -1;
    std::uint32_t offFlags = 0;
};

struct G2Zustand {
    std::vector<FlaechenSchalter> slist;   // mSlist
    int wurzel = 0;                        // mSurfaceRoot
};

// G2_SetSurfaceOnOff: false, wenn es die Flaeche nicht gibt.
bool setzeFlaecheAnAus(const Inhalt& in, G2Zustand& z, const std::string& name, std::uint32_t offFlags);
// G2_SetRootSurface
bool setzeWurzel(const Inhalt& in, G2Zustand& z, const std::string& name);
// G2API_SetSurfaceOnOffFromSkin: leert die Ueberschreibungen und schaltet
// jede Flaeche der Haut mit "*off" ab, deren eigene Flaggen nicht schon OFF
// tragen. `hautAus` sind die Flaechennamen der Haut mit "*off" (nach der
// "_off"-Regel von R_RegisterSkin, siehe applySkin in glm.cpp).
void hautAnwenden(const Inhalt& in, G2Zustand& z, const std::vector<std::string>& hautAus);
// Die offFlags einer Flaeche (Ueberschreibung vor Flaggen der Datei).
[[nodiscard]] std::uint32_t offFlags(const Inhalt& in, const G2Zustand& z, int flaeche);
// RenderSurfaces: je Flaeche 1, wenn sie gezeichnet wird. Ein .md3 ist kein
// Ghoul2-Modell (R_AddMD3Surfaces zeichnet jede Flaeche): dort alle 1.
[[nodiscard]] std::vector<char> gezeichnet(const Inhalt& in, const G2Zustand& z);

// --- LOD wie im Spiel -------------------------------------------------------
struct LodLage {
    int stufenZahl = 1;
    float radius = 0.0F;        // ent->e.radius
    float skala = 1.0F;         // groesster Wert von ent->e.modelScale (0 -> 1)
    float abstand = 0.0F;       // Abstand des Ursprungs entlang der Blickachse
    float projektion5 = 1.0F;   // projectionMatrix[5] = 1 / tan(fovY / 2)
    float lodscale = 10.0F;     // r_lodscale
    int lodbias = 0;            // r_lodbias
    int instanzBias = 0;        // CGhoul2Info::mLodBias
};
[[nodiscard]] float projiziereRadius(float r, float abstand, float projektion5) noexcept;   // ProjectRadius
[[nodiscard]] int lodWieImSpiel(const LodLage& l) noexcept;                                    // G2_ComputeLOD

// --- Grenzen je Zielspiel ----------------------------------------------------
enum class Zielspiel : std::uint8_t { JkaSp, JkaMp, Jk2Sp, MdSp, MdMp };
inline constexpr int kZielspiele = 5;

struct Grenzen {
    const char* name;
    int eckenJeFlaeche;     // SHADER_MAX_VERTEXES: ab diesem Wert ERR_DROP (RB_CheckOverflow)
    int dreieckeJeFlaeche;  // SHADER_MAX_INDEXES: RB_SurfaceGhoul gibt numTriangles als Indexzahl
    int knochenJeFlaeche;   // iMAX_G2_BONEREFS_PER_SURFACE
    int gewichteJeEcke;     // iMAX_G2_BONEWEIGHTS_PER_VERT
    int g2Modelle;          // MAX_G2_MODELS
    float lodscale;         // r_lodscale (Vorgabe)
    float figurRadius;      // ent->e.radius einer Figur (SP: 0, MP: CG_RadiusForCent 64)
    bool tPoseHinterGla;    // Animation hinter dem Ende der GLA: T-Pose (OpenJK-Fassungen)
};
[[nodiscard]] const Grenzen& grenzen(Zielspiel s) noexcept;

// --- Pruefungen --------------------------------------------------------------
enum class BefundArt : std::uint8_t { Ecken, Dreiecke, Knochen, Gewichte, G2Modelle, AnimHinterGla, ShaderFehlt };

struct Befund {
    BefundArt art = BefundArt::Ecken;
    int flaeche = -1;   // Teil-Index, -1 = Modell
    int stufe = 0;      // LOD-Stufe
    int wert = 0;
    int grenze = 0;
    std::string name;   // Flaeche oder Animation
};

// Was eine Flaeche einer Stufe an die Grenzen fuehrt.
struct FlaechenMass {
    int ecken = 0;
    int dreiecke = 0;
    int knochen = 0;       // verschiedene Knochen mit Gewicht > 0
    int gewichteMax = 0;   // meiste Gewichte > 0 an einer Ecke
};
[[nodiscard]] FlaechenMass mass(const Netz& n);
// Ein Mass gegen die Grenzen: ein Befund je ueberschrittener Grenze.
[[nodiscard]] std::vector<Befund> pruefeMass(const FlaechenMass& m, const Grenzen& g);
// Ein Abschnitt der animation.cfg gegen die Bildzahl der GLA: endFrame =
// firstFrame + numFrames muss <= numFrames der GLA sein (G2_bones.cpp:838-839).
[[nodiscard]] bool animInnerhalb(const AnimEntry& e, int glaBilder) noexcept;
// Alles zusammen: jede Flaeche jeder Stufe (Tags ausgenommen), die
// Abschnitte, die Zahl der Ghoul2-Instanzen, fehlende Bilder (hatBild je
// Flaeche, leer = nicht pruefen; nur gezeichnete Flaechen).
[[nodiscard]] std::vector<Befund> pruefeModell(const Inhalt& in, Zielspiel s, const std::vector<AnimEntry>& anims, int glaBilder,
                                               int g2Instanzen, const std::vector<char>& hatBild,
                                               const std::vector<char>& gezeichneteFlaechen);

}  // namespace bhed::modell
#endif
