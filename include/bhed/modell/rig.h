// modell/rig.h - Knochen-Erkennung, Control Rig (FK), Schluessel-Schicht und Backen (Modell-Block Stufe 3)
//
// Erkennung (erkenneKnochen): je Knochen eine Rolle (Becken, Wirbel, Oberarm ...) und eine Seite, aus drei Quellen:
//   Name       Muster der drei Spiele (pelvis, lhumerus, r_d1_j1, L_upper_arm, leg_left_bone01, femur_3 ...)
//   Hierarchie der erste Knochen der Mitte ueber einem Seitenknochen (Ansatz) trennt Beine (am Becken, reichen am
//              tiefsten) von Armen; JK2 haengt alle Glieder flach an pelvis/thoracic, das geht ueber dieselbe Regel
//   Lage       die Grundstellung (basePose, Spalte 3): die Seite ueber die Seitenachse, die Reihenfolge in einer Kette
//              ueber den Abstand vom Ansatz, Becken < Wirbel < Hals < Kopf ueber die Hoehe
// Widersprechen sich Name und Lage, gilt die Lage, und der Knochen ist "unklar" (die Uebersicht zeigt ihn gelb).
// Die Achsen kommen aus den Daten: oben ist Z (Ghoul2), die Seitenachse ist die Achse, auf der sich die Knochenpaare
// am weitesten unterscheiden (JKA/JK2/MD: X, links positiv).
//
// Rig (rigAnlegen): Griffe fuer Becken, Wirbel, Hals, Kopf, Schluesselbein, Ober-/Unterarm, Hand, Ober-/Unterschenkel,
// Fuss, Zehen; Pol-Ziele fuer Ellbogen/Knie und die Fussrolle (Ferse, Ballen, Zehe) sind schon angelegt und warten auf
// IK (Stufe 4); der Saebelgriff sitzt am Tag *r_hand (*l_hand). Drehen eines Griffs ist FK: die Drehung gilt dem Knochen
// wie G2API_SetBoneAngles mit BONE_ANGLES_POSTMULT (tr_ghoul2.cpp:1800-1812, G2_TransformBone OpenJK): Endmatrix *
// BasePose * M * BasePoseInv, hier X(b) * M mit X = worldMatrices. Knochen, die zur Hand gehoeren, aber im Skelett nicht
// unter ihr haengen (JKA: Finger und rhang_tag_bone unter rradius), dreht die Hand mit; der Drehknochen (rradiusX) nimmt
// den Anteil der Drehung um die Achse des Segments, der seiner Lage auf der Strecke entspricht (Grundstellung).
//
// Schicht (RigSchicht): je Knochen Schluessel {Bild, Drehung q, Verschiebung t}; zwischen zwei Schluesseln slerp/lerp,
// davor/danach der erste/letzte. Ausgewertet gibt sie M je Knochen (rigNachMal) fuer worldMatricesLerp.
//
// Backen (rigBacken): A'(b) = A(b) * B(b) * M * B(b)^-1 (aus X'(b) = X(b) * M, Kinder erben), dann wie Carcass/G2C
// compressBone (14 Bytes, MC_UnCompressQuat umgekehrt). Ohne M bleiben die 14 Bytes der Quelle unveraendert.
#ifndef BHED_MODELL_RIG_H
#define BHED_MODELL_RIG_H

#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "bhed/gla.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/rigdaten.h"

namespace bhed::modell {



// Englischer Schluessel einer Rolle fuer Berichte und Tests ("pelvis", "upperarm", ...).
[[nodiscard]] const char* rollenSchluessel(Rolle r) noexcept;
[[nodiscard]] bool rolleAusSchluessel(const std::string& s, Rolle& r) noexcept;
// Gehoert die Rolle zu einem Arm / Bein (fuer die Gliedmassen-Ketten)?
[[nodiscard]] bool armRolle(Rolle r) noexcept;
[[nodiscard]] bool beinRolle(Rolle r) noexcept;

struct KnochenErkennung {
    Rolle rolle = Rolle::Keine;
    Seite seite = Seite::Mitte;
    Rolle nachName = Rolle::Keine;
    Seite seiteName = Seite::Mitte;
    Seite seiteLage = Seite::Mitte;
    bool unklar = false;
    bool vonHand = false;     // in der Uebersicht zugeordnet
    std::string grund;        // "seite", "reihenfolge", "glied", "name" (unklar) oder leer
};

struct Erkennung {
    std::vector<KnochenErkennung> knochen;
    int seitenAchse = 0;      // 0 X, 1 Y
    float linksVorzeichen = 1.0F;
    float hoehe = 0.0F;       // Hoehe der Grundstellung (oben minus unten)
    int erkannt = 0;          // mit Rolle (ohne Keine)
    int unklar = 0;
};



[[nodiscard]] Erkennung erkenneKnochen(const GlaAnimation& g);
// Die Erkennung mit den Zuordnungen von Hand (sie gelten zuletzt). Mit einem Modell (`in`) zaehlt fuer Knochen, die in
// der Mitte liegen (JKA: lclavical/rclavical bei x = 0), die Lage der Ecken, die sie tragen.
[[nodiscard]] Erkennung erkenneKnochen(const GlaAnimation& g, const std::vector<Zuordnung>& vonHand, const Inhalt* in = nullptr, int stufe = 0);
// Der Spiegelname nach den Namensregeln der Spiele (lhumerus <-> rhumerus, l_d1_j1 <-> r_d1_j1, L_thigh <-> R_thigh,
// leg_left_bone01 <-> leg_right_bone01); leer, wenn der Name keine Seite traegt.
[[nodiscard]] std::string spiegelName(const std::string& name);
// Je Knochen der Partner auf der Gegenseite (-1 keiner; Mitte: er selbst). Erst ueber den Namen, sonst ueber die
// gespiegelte Lage mit gleicher Rolle.
[[nodiscard]] std::vector<int> spiegelPartner(const GlaAnimation& g, const Erkennung& e);

// --- Tags (Flaechen mit G2SURFACEFLAG_ISBOLT) ------------------------------------------------------------------------
enum class TagArt : std::uint8_t { Sonstige = 0, SaebelRechts, SaebelLinks, Huefte, Kopf, Ruecken };
struct TagErkennung {
    int teil = -1;
    std::string name;
    int knochen = -1;   // der Knochen mit dem groessten Gewicht an der ersten Ecke
    TagArt art = TagArt::Sonstige;
    Seite seite = Seite::Mitte;
    bool unklar = false;
    float ort[3]{};     // Mitte des Tag-Dreiecks in der Grundstellung
};
[[nodiscard]] std::vector<TagErkennung> erkenneTags(const Inhalt& in, int stufe, const Erkennung& e);

// --- Control Rig -----------------------------------------------------------------------------------------------------
enum class GriffArt : std::uint8_t {
    Becken = 0, Wirbel, Hals, Kopf, Schluessel, Oberarm, Unterarm, Hand, Oberschenkel, Unterschenkel, Fuss, Zehen,
    PolEllbogen, PolKnie, Ferse, Ballen, Zehe, Saebelgriff
};
inline constexpr int kGriffArten = 18;

struct Griff {
    GriffArt art = GriffArt::Becken;
    Seite seite = Seite::Mitte;
    int knochen = -1;            // der gedrehte Knochen (FK); bei Pol/Fussrolle der Knochen, an dem das Ziel haengt
    std::vector<int> mit;        // Knochen, die mitgedreht werden (Finger unter dem Unterarm)
    int twist = -1;              // Drehknochen dieses Segments (-1 keiner)
    int twistVon = -1;           // Anfang des Segments (Unterarm), Achse twistVon -> knochen
    float twistAnteil = 0.0F;    // Lage des Drehknochens auf der Strecke (Grundstellung)
    bool ik = false;             // wirkt erst mit IK (Stufe 4): Pol-Ziele, Ferse/Ballen/Zehe
    float ort[3]{};              // Pol/Fussrolle/Saebelgriff: Ort in der Grundstellung
    std::string tag;             // Saebelgriff: das Tag
    int nummer = 0;              // mehrere gleicher Art/Seite (Wirbel 1..n, Beine einer Kreatur)
};

struct Rig {
    std::vector<Griff> griffe;
    Erkennung erkennung;
    std::vector<int> partner;
    std::vector<TagErkennung> tags;
    [[nodiscard]] bool leer() const noexcept { return griffe.empty(); }
    [[nodiscard]] int griffVon(int knochen) const noexcept;   // der FK-Griff eines Knochens, -1 keiner
};

// Das Rig mit einem Klick: Erkennung (mit den Zuordnungen von Hand), Griffe, Tags und Fussrolle aus dem Modell
// (`in` darf leer sein - dann ohne Tags und ohne Ferse/Zehe).
[[nodiscard]] Rig rigAnlegen(const GlaAnimation& g, const Inhalt& in, int stufe, const std::vector<Zuordnung>& vonHand);

// --- Schluessel-Schicht ----------------------------------------------------------------------------------------------


// M eines Knochens zum Bild (false: keine Schluessel fuer ihn).
[[nodiscard]] bool rigWert(const RigSchicht& s, int knochen, float bild, BoneMatrix& m);
// Dasselbe fuer eine Schluesselliste (Stufe 3b: Reduzieren, Kurveneditor).
[[nodiscard]] bool schluesselWert(const std::vector<RigSchluessel>& v, float bild, BoneMatrix& m);
// Alle M zum Bild, fuer worldMatricesLerp (nachMal).
void rigNachMal(const RigSchicht& s, float bild, std::vector<std::pair<int, BoneMatrix>>& aus);
// Dasselbe fuer worldMatricesLerp(bildA, bildB, anteil): mit Bindebasis umgerechnet (M_eff = B^-1 A^-1 B M, Stufe 3b),
// Stufe 4: dazu die IK der Spuren (bhed/modell/ik.h; `ueber` wie bei worldMatricesLerp, z. B. der Oberkoerper).
void rigNachMalAuf(const GlaAnimation& g, const RigSchicht& s, int bildA, int bildB, float anteil, std::vector<std::pair<int, BoneMatrix>>& aus,
                   const std::vector<BoneFrameOverride>& ueber = {});
// Nur die FK-Schluessel (ohne IK), wie rigNachMalAuf bis Stufe 3b.
void rigNachMalFk(const GlaAnimation& g, const RigSchicht& s, int bildA, int bildB, float anteil, std::vector<std::pair<int, BoneMatrix>>& aus);
// Einen Schluessel setzen (ersetzt einen am selben Bild).
void rigSchluesselSetzen(RigSchicht& s, int knochen, int bild, const BoneMatrix& m);
void rigSchluesselLoeschen(RigSchicht& s, int knochen, int bild);

// Die Welt (worldMatricesLerp mit der Schicht) zum Bild bild (+ Anteil zum naechsten).
void rigWelt(const GlaAnimation& g, const RigSchicht& s, int bildA, int bildB, float anteil, std::vector<BoneMatrix>& welt);

// Eine Drehung des Griffs in Weltachsen (um den Ursprung seines Knochens) auf das Bild anwenden: Schluessel fuer den
// Knochen, die mitgedrehten und den Drehknochen. `welt` ist die Welt dieses Bilds mit der Schicht (rigWelt).
// `drehung` ist eine reine Drehung (Spalte 3 = 0), `weg` eine Verschiebung in Weltachsen (nur Becken/Wurzel).
void rigGriffDrehen(const GlaAnimation& g, const Rig& r, int griff, int bild, const std::vector<BoneMatrix>& welt, const BoneMatrix& drehung,
                    const float weg[3], RigSchicht& s);

// Stufe 3b: einen Knochen in Weltachsen verschieben (Bewegungspfad ziehen): X'(b) = T(weg) X(b), ein Schluessel am Bild.
void rigKnochenVerschieben(const GlaAnimation& g, RigSchicht& s, int knochen, int bild, const std::vector<BoneMatrix>& welt, const float weg[3]);

// --- Posen -----------------------------------------------------------------------------------------------------------
struct RigPose {
    std::vector<std::string> namen;
    std::vector<BoneMatrix> welt;   // Weltmatrizen (worldMatrices: Endmatrix * BasePose)
};
[[nodiscard]] RigPose poseAus(const GlaAnimation& g, const std::vector<BoneMatrix>& welt);
// Eine Pose als Schluessel am Bild: fuer jeden Knochen mit gleichem Namen M so, dass die relative Lage zum Eltern der
// Pose entspricht. nurKnochen leer = alle. gespiegelt: die Pose der Gegenseite, an der Seitenachse gespiegelt
// (D'(b) = S D(p(b)) S mit D = Welt * Grundstellung^-1).
int poseEinfuegen(const GlaAnimation& g, const Rig& r, const RigPose& p, bool gespiegelt, int bild, const std::vector<int>& nurKnochen,
                  RigSchicht& s);
// Auf die Bindepose (A = Einheit) zuruecksetzen - die gewaehlten Knochen oder alle (leer).
int bindePose(const GlaAnimation& g, int bild, const std::vector<int>& nurKnochen, RigSchicht& s);

// Handposen aus den Animationen des Spiels (Posen-Bibliothek): die Finger einer Hand relativ zur Hand wie im Bild
// `quellBild` der GLA. Offen ist die Bindepose. Gibt die Zahl der gesetzten Finger.
int handPoseSetzen(const GlaAnimation& g, const Rig& r, Seite seite, int quellBild, int bild, RigSchicht& s);

// --- Backen ----------------------------------------------------------------------------------------------------------
struct BackBericht {
    int bilder = 0;
    int vorrat = 0;
    int unveraendert = 0;   // Knochen-Bilder mit den 14 Bytes der Quelle
    int neu = 0;            // durch compressBone
    std::string fehler;
};
// Die Bilder [erstes, erstes + anzahl) der Quelle mit der Schicht in eine neue GLA (Skelett der Quelle, Name kopfName).
// alleNeu: auch unveraenderte Knochen durch compressBone (Rundlauf-Pruefung der Kompression).
[[nodiscard]] bool rigBacken(const GlaAnimation& q, const RigSchicht& s, int erstes, int anzahl, bool alleNeu, const std::string& kopfName,
                             GlaAnimation& aus, BackBericht* bericht = nullptr);

// --- Bewegungspfade --------------------------------------------------------------------------------------------------
// Die Weltorte eines Knochens ueber die Bilder [von, bis] (mit der Schicht).
[[nodiscard]] std::vector<std::array<float, 3>> bewegungsPfad(const GlaAnimation& g, const RigSchicht& s, int knochen, int von, int bis);

// Hilfen fuer Tests und die Oberflaeche
[[nodiscard]] BoneMatrix starrInvers(const BoneMatrix& m) noexcept;   // allgemeine 3x4-Umkehr (auch nicht orthonormal)
[[nodiscard]] BoneMatrix drehungUm(const float achse[3], float grad) noexcept;
void quatAusMatrix(const BoneMatrix& m, float q[4]) noexcept;
[[nodiscard]] BoneMatrix matrixAusQuat(const float q[4]) noexcept;

}  // namespace bhed::modell
#endif
