// gui/app_modelltab_knochen.h - AP13K (MBENTWURF Teil B, rc612): Knochen-Darstellung (Abgleich R14), Rig-Stile, Durchsicht
//
// Eigener Kopf (nicht in modelltab.h), eingebunden nur von den Dateien, die ihn brauchen.
//
//   R14            Knochen je Bedienmodus wie im Entwurf: wie 3ds Max der Bone-Kasten (Flossen schaltbar), wie Blender
//                  "Display As" Oktaeder, Stick, B-Bone, Huelle, Draht (Armature > Viewport Display; ausgelesen mit Blender
//                  5.0: OCTAHEDRAL ist Vorgabe).
//   Durchsicht     3ds Max See-Through (Alt+X, Objekteigenschaft: "makes the object or selection translucent in viewports ...
//                  such as a character with bones inside") - gilt der Figur, also in allen Ansichten; Blender X-Ray (Alt+Z,
//                  View3DShading.show_xray, xray_alpha 0,5) - je 3D-Ansicht.
//   Im Vordergrund Blender Object.show_in_front ("Make the object display in front of others"); aus: die Figur verdeckt Knochen
//                  und Griffe wie in 3ds Max. Vorgabe an (so zeichnete das Programm bis rc611).
//   Rig-Stile      Wunsch shank 12.11.: Standard (Entwurf), Control Rig (Ringe), CAT-Kaesten, Blender-Oktaeder - nur das
//                  Aussehen (bhed/modell/rigformen.h), fuer das Skelett und die Griffe des Control Rigs.
// Gezeichnet werden Knochen und Griffe im Bild der Ansicht (gpu::ModellAnsichtArt::formen): Linien als Streifen in
// Bildpunkten mit weichem Rand, unter Direct3D 12 und Direct3D 11 dieselben Ecken. Auswahl trifft die sichtbare Form.
#ifndef BHED_GUI_APP_MODELLTAB_KNOCHEN_H
#define BHED_GUI_APP_MODELLTAB_KNOCHEN_H

#include <string>

#include "imgui.h"

#include "bhed/modell/rigformen.h"

namespace bhed::gpu {
struct ModellAnsichtArt;
}

namespace bhed::gui {

struct ModellAnsicht;

// Blender Armature.display_type in dessen Reihenfolge
enum class KnochenArt : int { Oktaeder = 0, Stick, BBone, Huelle, Draht };
inline constexpr int kKnochenArten = 5;

// Deckkraft der Figur in der Durchsicht: Blender View3DShading.xray_alpha (Vorgabe 0,5, ausgelesen). Autodesk nennt fuer
// See-Through keinen Wert ("translucent") - derselbe.
inline constexpr float kDurchsichtAlpha = 0.5F;

// Was eine sichtbare Form ist (KdFormInfo::gestalt): die Knochen der Darstellungsarten, sonst 10 + modell::FormGestalt
enum class KdGestalt : int { BoneKasten = 0, Oktaeder, Stick, BBone, Huelle, Draht, Stil = 10 };

struct KdFormInfo {
    int knochen = -1;       // der Knochen, den ein Klick waehlt (-1: keiner)
    int griff = -1;         // der Griff, den ein Klick waehlt (-1: keiner)
    bool schmuck = false;
    int gestalt = 0;        // KdGestalt bzw. 10 + modell::FormGestalt
    float farbe[3]{};       // 0..1, ohne Auswahl
    float masse[3]{};       // CAT-Kasten: Laenge, Breite, Tiefe in Einheiten; Ring: Radius
    int linien = 0;
    int flaechen = 0;
    int punkte = 0;         // Scheiben und Kreise zum Schirm (Gelenke von Stick)
    bool gewaehlt = false;
};

struct ModellKd {
    bool durchsicht[4]{};         // je Ansicht
    bool hilfsknochen = false;    // TEST1411 H: Drehhilfen (lradiusX ...) als Kasten statt als Hilfspunkt
    bool oeffnen = false;         // Aktion: die Gruppe im rechten Feld aufklappen und zeigen
    // Selbsttest: was das letzte Bild gezeichnet hat
    int formenGezeichnet = 0;     // Formen der aktiven Ansicht
    int eckenGezeichnet = 0;      // Ecken an den Zeichner (aktive Ansicht)
    int beschriftungen = 0;       // IK/FK am Glied (Stil Control Rig)
    int rolloutBild = -1;
    int menueBild = -1;
};
[[nodiscard]] ModellKd& modellKd();

// --- Einstellungen (behaved.cfg: modelBoneDisplay, modelRigStyle, modelBonesInFront) ---
[[nodiscard]] KnochenArt modellKnochenArt();
void modellKnochenArtSetzen(KnochenArt a);
[[nodiscard]] modell::RigStil modellRigStil();
void modellRigStilSetzen(modell::RigStil s);
// Zeichnet das Skelett in diesem Stil wie Blender (Bedienmodus wie Blender oder Stil Blender-Oktaeder)? Dann gilt "Display As".
[[nodiscard]] bool modellKnochenWieBlender();
// Durchsicht umschalten: wie 3ds Max fuer die Figur (alle Ansichten), wie Blender fuer die Ansicht `ansicht`.
void modellDurchsichtUmschalten(int ansicht);
[[nodiscard]] bool modellDurchsicht(int ansicht);

// --- Zeichnen ---
// Die Formen (Knochen, Griffe) der Ansicht fuer den Zeichner und was sonst zu ihnen gehoert (im Vordergrund, Durchsicht).
// w, h: Groesse des Ziels in Bildpunkten (das Foto ist groesser als die Ansicht).
void modellKdFormen(const ModellAnsicht& a, int w, int h, gpu::ModellAnsichtArt& art);
// Was das Bild der Ansicht durch Knochen, Griffe und Durchsicht bestimmt (fuer das Neuzeichnen).
[[nodiscard]] std::string modellKdStand(const ModellAnsicht& a, int w, int h);
// Ueber dem Bild: IK/FK am Glied (Stil Control Rig).
void modellKdUeberlagerung(const ModellAnsicht& a, ImDrawList* dl);
// Zeichnet dieser Stil die Griffe als Formen im Bild (sonst die Zeichen des Entwurfs, gui/app_modelltab_rig.cpp)?
[[nodiscard]] bool modellKdGriffeAlsFormen();

// --- Auswahl: die sichtbare Form unter der Maus ---
// Der Griff (-1 keiner). Nur wenn modellKdGriffeAlsFormen().
[[nodiscard]] int modellKdGriffUnter(const ModellAnsicht& a, ImVec2 maus);
// Der Knochen, dessen gezeichnete Form unter der Maus liegt (-1 keiner; auch -1, wenn keine Knochen gezeichnet sind).
[[nodiscard]] int modellKdKnochenUnter(const ModellAnsicht& a, ImVec2 maus);

// --- Bedienung ---
bool modellKdTasten();                                  // Alt+X (wie 3ds Max), Alt+Z (wie Blender)
bool modellKdAktionenSuche(const std::string& such);    // "Alle Aktionen suchen"
void modellKdMenue(int ansicht);                        // Kopfzeile der Ansicht und Rechtsklick: im Vordergrund, Anzeigen als, Rig-Stil
void modellKdMenueSchattierung(int ansicht);            // ... im Menue der Schattierung: Durchsicht (wie 3ds Max) / Roentgen (wie Blender)
void modellKdRollout();                                 // die Gruppe im rechten Feld (Bereich Animation)

// --- Selbsttest ---
[[nodiscard]] int modellKdFormZahl();
// TEST1411 H: Anfang und Ende des Kastens einer Knochen-Form und seine volle Breite am Gelenk (false: kein Kasten, z. B. ein Hilfspunkt)
[[nodiscard]] bool modellKdFormAchse(int form, float a[3], float b[3], float& breite);
// TEST1411 H: das Folgegelenk der Kette (-1: Endknochen) und ob ein Knochen als Drehhilfe gilt
[[nodiscard]] int modellKdKetteFolge(int knochen);
[[nodiscard]] bool modellKdKetteHilfe(int knochen);
[[nodiscard]] KdFormInfo modellKdFormInfo(int form);
[[nodiscard]] int modellKdFormVonGriff(int griff);       // -1 keine
[[nodiscard]] int modellKdFormVonKnochen(int knochen);   // -1 keine
// Ein Punkt auf einer sichtbaren Linie der Form in der Ansicht (Schirm), der keiner anderen Form naeher liegt; false: keiner.
[[nodiscard]] bool modellKdFormPunkt(int form, int ansicht, ImVec2& out);
// Der Massstab der Figur gegen die Vorbilder (modell::rigMassstab) und die Hoehe des Bodens.
[[nodiscard]] float modellKdMassstab();

}  // namespace bhed::gui
#endif
