// modell/rigdaten.h - was das Control Rig im Modelldokument traegt (Modell-Block Stufe 3)
//
// Die Schluessel der Griffe (je Sequenz eine Schicht) und die Zuordnungen von Hand aus der Uebersicht gehoeren ins
// Modelldokument, damit Strg+Z sie zuruecknimmt wie jede andere Aenderung. Die Rechnung steht in bhed/modell/rig.h.
#ifndef BHED_MODELL_RIGDATEN_H
#define BHED_MODELL_RIGDATEN_H

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace bhed::modell {

enum class Rolle : std::uint8_t {
    Keine = 0,
    Wurzel,          // model_root
    Bewegung,        // Motion (Wurzelbewegung)
    Becken,
    Wirbel,
    Hals,
    Kopf,
    Gesicht,         // face, Kiefer, Augen, Lippen
    Schluessel,      // Schluesselbein / Schulter
    Oberarm,
    OberarmTwist,    // lhumerusX
    Unterarm,
    UnterarmTwist,   // lradiusX
    Hand,
    Finger,
    Oberschenkel,
    OberschenkelTwist,   // lfemurX
    Unterschenkel,
    Fuss,
    Zehen,
    Anhang,          // Schwanz, Rockschoesse (ltail/rtail)
    TagKnochen,      // *hang_tag_bone, rhand_tag_bone
    Sonstige,        // erkannt, aber ohne Rolle im Rig (Schild, Waffe, Fluegel)
};
inline constexpr int kRollen = 23;
enum class Seite : std::uint8_t { Mitte = 0, Links, Rechts };

// Eine Zuordnung von Hand (Uebersicht): Knochen -> Rolle und Seite.
struct Zuordnung {
    int knochen = -1;
    Rolle rolle = Rolle::Keine;
    Seite seite = Seite::Mitte;
};

// Stufe 3b: Interpolation des Abschnitts ab diesem Schluessel bis zum naechsten (wie BezTriple::ipo in Blender, der linke
// Schluessel bestimmt den Abschnitt). Linear dreht mit slerp (Stufe 3, wie der Linear-Rotation-Controller von 3ds Max),
// Bezier rechnet je Kanal eine F-Kurve wie Blender (fcurve.cc fcurve_eval_keyframes_interpolate) und normiert das
// Quaternion danach (BKE_pchan_to_mat4).
enum class Interp : std::uint8_t { Konstant = 0, Linear, Bezier };
// Henkelarten in der Reihenfolge von Blender (DNA_curve_enums.h eBezTriple_Handle: HD_FREE, HD_AUTO, HD_VECT, HD_ALIGN,
// HD_AUTO_ANIM). Die Rechnung steht in bhed/modell/animkurven.h.
enum class Henkel : std::uint8_t { Frei = 0, Auto, Vektor, Ausgerichtet, AutoGeklemmt };
inline constexpr int kKanaele = 7;   // Quaternion w x y z, Verschiebung x y z

struct RigSchluessel {
    int bild = 0;
    float q[4] = {1.0F, 0.0F, 0.0F, 0.0F};   // w x y z, im Rahmen des Knochens (M)
    float t[3] = {0.0F, 0.0F, 0.0F};
    Interp interp = Interp::Linear;          // Stufe 3 war linear; neue Schluessel der Oberflaeche nehmen die Vorgabe
    // Henkel je Kanal, relativ zum Schluessel (Bilder, Wert): links (dx <= 0) und rechts (dx >= 0). Ein neuer Schluessel
    // hat sie wie Blender bei +-1 Bild auf gleicher Hoehe (animrig/intern/fcurve.cc initialize_bezt); die Auto-Arten
    // rechnet henkelNeu nach jeder Aenderung.
    Henkel henkelL[kKanaele] = {Henkel::AutoGeklemmt, Henkel::AutoGeklemmt, Henkel::AutoGeklemmt, Henkel::AutoGeklemmt,
                                Henkel::AutoGeklemmt, Henkel::AutoGeklemmt, Henkel::AutoGeklemmt};
    Henkel henkelR[kKanaele] = {Henkel::AutoGeklemmt, Henkel::AutoGeklemmt, Henkel::AutoGeklemmt, Henkel::AutoGeklemmt,
                                Henkel::AutoGeklemmt, Henkel::AutoGeklemmt, Henkel::AutoGeklemmt};
    float hl[kKanaele][2] = {{-1.0F, 0.0F}, {-1.0F, 0.0F}, {-1.0F, 0.0F}, {-1.0F, 0.0F}, {-1.0F, 0.0F}, {-1.0F, 0.0F}, {-1.0F, 0.0F}};
    float hr[kKanaele][2] = {{1.0F, 0.0F}, {1.0F, 0.0F}, {1.0F, 0.0F}, {1.0F, 0.0F}, {1.0F, 0.0F}, {1.0F, 0.0F}, {1.0F, 0.0F}};
};
// Stufe 4: IK (Rechnung in bhed/modell/ik.h). Ein Glied der Kette, wie der FK-Griff aus dem Rig (Knochen, mitgedrehte
// Knochen, Drehknochen mit seinem Anteil) - kopiert, damit Ansicht und Backen ohne das Rig rechnen.
struct IkGlied {
    int knochen = -1;
    std::vector<int> mit;
    int twist = -1;
    int twistVon = -1;
    float twistAnteil = 0.0F;
};
enum class IkArt : std::uint8_t { ArmLinks = 0, ArmRechts, BeinLinks, BeinRechts };
inline constexpr int kIkArten = 4;
// Was ein IK-Schluessel ausser dem Ziel traegt (je Schluessel, Bild wie IkSpur::ziel).
struct IkWert {
    float ik = 1.0F;       // 1 IK, 0 FK (Blender: 1 - Regler IK-FK von Rigify); gilt bis zum naechsten Schluessel
    int raum = -1;         // Child Of / Space Switching: -1 Welt, sonst der Knochen; gilt bis zum naechsten Schluessel
    float pol[3]{};        // Pol-Ziel (Max: Schwenkziel) im Raum von IkSpur::polRaum
    float rolle = 0.0F;    // Fussrolle in Grad wie der Fersen-Regler von Rigify: < 0 um die Ferse, > 0 um den Ballen
    float spitze = 0.0F;   // Anteil einer Rolle > 0 um die Zehenspitze (Rigify Toe_Roll)
    float zehen[4] = {1.0F, 0.0F, 0.0F, 0.0F};   // Drehung der Zehen zum ungerollten Fuss (w x y z, aus der FK-Pose)
};
struct IkSpur {
    IkArt art = IkArt::ArmRechts;
    IkGlied oben, unten, ende, zehen;   // Ober-/Unterarm (-schenkel), Hand/Fuss, Zehen (knochen -1: keine)
    int polRaum = -1;                   // der Knochen ueber dem Glied (Schluesselbein, Becken)
    float ferse[3]{};                   // Drehpunkte der Fussrolle in der Grundstellung
    float ballen[3]{};
    float spitzePunkt[3]{};
    bool hatFerse = false;
    bool hatBallen = false;
    bool hatSpitze = false;
    float rollAchse[3]{};               // Achse der Fussrolle in der Grundstellung (oben x vorn)
    float polAbstand = 0.0F;            // Abstand des Pols vom Gelenk (Laenge des oberen Glieds)
    std::vector<RigSchluessel> ziel;    // Ort (t) und Drehung (q) des Endknochens im Raum; Bezier wie die Schicht
    std::vector<IkWert> wert;           // je Schluessel von `ziel`
};

// Stufe 7 (offen aus Stufe 4): eine IK-Spur als Zeile im Dopesheet. In einer Schluesselwahl (animkurven.h) steht ihr
// Schluessel als Knochen kIkWahl - Spur; Verschieben, Skalieren, Einrasten und Loeschen nehmen ihn wie einen FK-Schluessel.
inline constexpr int kIkWahl = -1000;
[[nodiscard]] constexpr int ikWahlCode(int spur) noexcept { return kIkWahl - spur; }
[[nodiscard]] constexpr int ikWahlSpur(int code) noexcept { return code <= kIkWahl ? kIkWahl - code : -1; }

struct RigSchicht {
    std::vector<std::vector<RigSchluessel>> knochen;   // je Knochen, nach Bild sortiert
    std::vector<IkSpur> ik;                            // Stufe 4: IK je Glied (auf den FK-Schluesseln)
    // Stufe 3b: false - die Schluessel liegen ueber den Bildern der GLA (A' = A B M B^-1, Stufe 3); true - ein Knochen
    // mit Schluesseln ist ganz durch sie beschrieben, ueber der Bindepose (A' = B M B^-1, "Reduzieren" legt so an).
    bool bindeBasis = false;
    // Interpolation neuer Schluessel (die Oberflaeche setzt die Vorgabe des Modus: Bezier wie Max und Blender)
    Interp neuInterp = Interp::Linear;
    [[nodiscard]] bool leer() const noexcept;
    [[nodiscard]] int schluesselZahl() const noexcept;
    [[nodiscard]] bool hatSchluessel(int knochen, int bild) const noexcept;
};

// Stufe 3b: eine Pose der Posen-Bibliothek (Weltmatrizen je Knochenname, wie RigPose; 12 Werte je Knochen).
struct BibliothekPose {
    std::string name;
    std::vector<std::string> namen;
    std::vector<std::array<float, 12>> welt;
};

struct RigDaten {
    std::map<std::string, RigSchicht> schichten;   // je Sequenz (Name der animation.cfg-Zeile, "" ohne Sequenz)
    std::vector<Zuordnung> zuordnung;
    std::vector<BibliothekPose> posen;             // Stufe 3b
};

}  // namespace bhed::modell
#endif
