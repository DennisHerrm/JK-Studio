// modell/physik.h - Modell-Block Stufe 6: Auto-Physik (Schwerpunkt, Kontakte, Sprung, Drehimpuls, Federketten)
//
// Prinzip wie "AutoPhysics" von Cascadeur (geschlossen - nur die Idee, eigener Code): die Physik ist ein Filter ueber der
// Schluessel-Schicht. Sie liest die Pose jedes Bilds (FK und IK der Schicht, rigNachMalAuf) und schreibt je Knochen eine
// Korrektur P in eine eigene Spur (RigSchicht::physik); die Anzeige und das Backen rechnen X'' = X' P (BONE_ANGLES_POSTMULT
// wie die Schicht). Die Schluessel der Schicht bleiben unangetastet; aus = Anzeige und GLA wie ohne Physik.
//
// Massen     Punktmassen je Segment nach D. A. Winter, "Biomechanics and Motor Control of Human Movement", 4. Aufl. 2009,
//            Tabelle 4.1 (Daten von Dempster 1955): Anteil an der Koerpermasse und Lage des Segment-Schwerpunkts von
//            proximal - Hand 0,006 / 0,506, Unterarm 0,016 / 0,430, Oberarm 0,028 / 0,436, Fuss 0,0145 / 0,50,
//            Unterschenkel 0,0465 / 0,433, Oberschenkel 0,100 / 0,433, Kopf und Hals 0,081 / 1,000 (C7-T1 bis Gehoergang),
//            Rumpf 0,497 / 0,50 (grosser Rollhuegel bis Schultergelenk). Summe 1. Die Gelenke sind die Ursprungsorte der
//            Knochen aus der Knochen-Erkennung (Stufe 3): Rollhuegel = Mitte der Oberschenkel, Schultergelenk = Mitte der
//            Oberarme, Gehoergang = Kopf, Fussende = Zehen (sonst Ballen bzw. Spitze aus dem Modell), Handende = der
//            entfernteste Fingerknochen der Hand. Die Eigentraegheit der Segmente (Gyrationsradien) bleibt aussen vor.
// Stuetzflaeche die konvexe Huelle der Fusspunkte (Ferse, Ballen, Spitze aus dem Modell; ohne Modell Knoechel und Zehen)
//            der Fuesse im Kontakt, waagrecht. Warnung: im Stand (beide Fuesse im Kontakt) liegt der Schwerpunkt ausserhalb.
// Kontakt    tiefster Fusspunkt hoechstens kontaktAbstand ueber seinem tiefsten Stand in der Sequenz, gemessen ueber dem Boden
//            (Model-Tab: die Bodenebene; Mission-Tab: traceRay der Karte nach unten) - die Modelle stehen verschieden hoch. Waehrend eines Kontakts haelt die Bein-IK (Stufe 4, ikAnwenden) den tiefsten Fusspunkt
//            des ersten Kontaktbilds waagrecht fest - der Fuss rutscht nicht, Abrollen und Hoehe bleiben aus der Animation.
// Sprung     zwischen Absprung a (letztes Kontaktbild) und Landung l (erstes Kontaktbild danach) folgt der Schwerpunkt
//            c(t) = c(a) + v0 t + g t^2 / 2 mit g = (0, 0, -g_gravity) (Pmove: PM_StepSlideMove/PM_SlideMove rechnen mit
//            ps->gravity, bg_slidemove.cpp; g_gravity Vorgabe 800, g_main.cpp; worldspawn "gravity", g_spawn.cpp), v0 so,
//            dass c(l) bleibt. Der Ganzkoerper wird um den Schwerpunkt gedreht, damit der Drehimpuls um ihn konstant ist:
//            L(k+1/2) = sum m_i ((x_i(k) + x_i(k+1)) / 2) x (x_i(k+1) - x_i(k)) / dt, x_i = R(k) (p_i(k) - c(k)). Je
//            Bild loest Newton R(k+1) zu L0; L0 so, dass R(l) = Einheit (Lage bei der Landung wie die Animation).
// Federn     Nachlauf je Kettenknochen: die Weltdrehung S folgt der animierten T wie ein gedaempfter Drehschwinger
//            (w' = -(2 pi f)^2 log(S T^-1) - 2 zeta 2 pi f (w - wT), halbimplizit, 8 Teilschritte je Bild).
#ifndef BHED_MODELL_PHYSIK_H
#define BHED_MODELL_PHYSIK_H

#include <array>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "bhed/gla.h"
#include "bhed/modell/rig.h"
#include "bhed/modell/rigdaten.h"

namespace bhed::modell {

using Vec3 = std::array<float, 3>;

// Ein Punkt am Skelett: der Mittelwert der Ursprungsorte einiger Knochen oder ein Ort der Grundstellung an einem Knochen.
struct PhysikPunkt {
    std::vector<int> knochen;    // Ursprungsorte, gemittelt
    int an = -1;                 // sonst: Knochen, an dem `ort` (Grundstellung) haengt
    Vec3 ort{};
};
struct PhysikSegment {
    const char* name = "";       // englischer Schluessel fuer Berichte ("thigh_l", "trunk" ...)
    PhysikPunkt proximal;
    PhysikPunkt distal;
    float anteil = 0.0F;         // Anteil an der Koerpermasse (Winter Tab. 4.1)
    float lage = 0.0F;           // Schwerpunkt von proximal (Winter Tab. 4.1)
};
struct MassenModell {
    std::vector<PhysikSegment> segmente;
    int wurzel = -1;                         // der Knochen, ueber dem alle Segmente haengen (Becken)
    std::array<std::vector<PhysikPunkt>, 2> fuss;   // Fusspunkte links, rechts
    std::array<int, 2> bein{-1, -1};         // Index der IK-Spur in `spuren` (BeinLinks/BeinRechts)
    std::vector<IkSpur> spuren;
    std::string fehler;                      // leer: vollstaendig
    [[nodiscard]] bool gueltig() const noexcept { return fehler.empty() && !segmente.empty(); }
};
// Aus der Knochen-Erkennung des Rigs (Becken, Arme, Beine, Hals/Kopf je erkannt). Fehlt ein Segment, steht der Grund in
// `fehler` (Kreaturen ohne Menschenform: die Tabelle gilt nicht).
[[nodiscard]] MassenModell massenModell(const GlaAnimation& g, const Rig& r);
[[nodiscard]] Vec3 punktWelt(const GlaAnimation& g, const PhysikPunkt& p, const std::vector<BoneMatrix>& welt);
// Orte der Segment-Schwerpunkte und der Gesamtschwerpunkt.
void segmentOrte(const GlaAnimation& g, const MassenModell& mm, const std::vector<BoneMatrix>& welt, std::vector<Vec3>& orte);
[[nodiscard]] Vec3 schwerpunkt(const GlaAnimation& g, const MassenModell& mm, const std::vector<BoneMatrix>& welt);
// Drehimpuls um den Schwerpunkt zwischen zwei Bildern (Massenanteile, Einheiten^2/s), wie oben.
[[nodiscard]] Vec3 drehimpuls(const GlaAnimation& g, const MassenModell& mm, const std::vector<BoneMatrix>& welt0,
                              const std::vector<BoneMatrix>& welt1, double dt);

// Konvexe Huelle (x, y) gegen den Uhrzeigersinn und Punkt-in-Polygon (Rand zaehlt als innen).
[[nodiscard]] std::vector<std::array<float, 2>> konvexeHuelle(std::vector<std::array<float, 2>> p);
[[nodiscard]] bool imPolygon(const std::vector<std::array<float, 2>>& poly, float x, float y);
// Abstand eines Punkts von der Huelle (0 innen).
[[nodiscard]] float polygonAbstand(const std::vector<std::array<float, 2>>& poly, float x, float y);

struct PhysikUmgebung {
    float bodenZ = -24.0F;   // Bodenebene (Model-Tab; ModView-Boden)
    // Mission-Tab: Boden unter (x, y) ab zOben nach unten (traceRay der Karte), im Raum der Figur. false: kein Treffer
    // (dann gilt bodenZ).
    std::function<bool(float x, float y, float zOben, float& z)> boden;
    double bildMs = 50.0;    // ms je Bild (frameLerp der Sequenz)
};

struct PhysikBild {
    Vec3 sp{};                       // Schwerpunkt ohne Physik
    Vec3 spNeu{};                    // mit Physik
    std::array<bool, 2> kontakt{};   // links, rechts
    std::array<float, 2> fussHoehe{};   // tiefster Fusspunkt ueber dem Boden
    std::vector<std::array<float, 2>> stuetz;   // Stuetzflaeche (Huelle)
    float bodenZ = 0.0F;             // Boden unter dem Schwerpunkt
    bool imStand = false;            // beide Fuesse im Kontakt
    bool ausserhalb = false;         // Schwerpunkt ausserhalb der Stuetzflaeche
    float abstand = 0.0F;            // wie weit ausserhalb
    bool flug = false;
};
struct PhysikBericht {
    int erstes = 0;
    std::vector<PhysikBild> bilder;  // je Bild ab erstes
    std::vector<std::pair<int, int>> fluege;   // (Absprung, Landung), absolut
    int warnBild = -1;               // erstes Bild im Stand mit Schwerpunkt ausserhalb (absolut)
    int warnungen = 0;
    float parabelAbw = 0.0F;         // groesste Abweichung des neuen Schwerpunkts von der Parabel im Flug
    float drehimpulsVorher = 0.0F;   // groesste Abweichung |L - Mittel| ohne / mit Physik im Flug, bezogen auf 1 rad je Flug
    float drehimpulsNachher = 0.0F;
    int iterationen = 0;             // Newton aussen (Drehimpuls)
    int kettenKnochen = 0;
    std::string fehler;
};

// Die Physik-Spur der Schicht s fuer die Bilder [erstes, erstes + anzahl) neu rechnen (s.physik.knochen; Einstellungen in
// s.physik.e). Liest die Schicht ohne Physik. false: kein Massenmodell (Grund im Bericht), die Spur ist dann leer.
bool physikRechnen(const GlaAnimation& g, const Rig& r, RigSchicht& s, int erstes, int anzahl, const PhysikUmgebung& u,
                   PhysikBericht* bericht = nullptr);
// Nur die Analyse (Schwerpunkt, Kontakte, Stuetzflaeche, Fluege) ohne Spur.
bool physikAnalyse(const GlaAnimation& g, const Rig& r, const RigSchicht& s, int erstes, int anzahl, const PhysikUmgebung& u,
                   PhysikBericht& bericht);
// Die Korrekturen der Physik-Spur zum Bild (mit Staerke) auf die M (nach FK, IK und Bindebasis) multiplizieren: m = m P;
// Knochen ohne Eintrag bekommen P. Nichts, wenn der Filter aus ist.
void physikAuf(const RigSchicht& s, float bild, std::vector<std::pair<int, BoneMatrix>>& mEff);
// Hat die Schicht eine wirksame Physik-Spur?
[[nodiscard]] bool physikWirkt(const RigSchicht& s) noexcept;
// Vorschlag fuer Federketten: Knochen mit der Rolle Anhang (Rockschoesse, Schwanz) ohne Anhang-Eltern.
[[nodiscard]] std::vector<int> kettenVorschlag(const Rig& r, const GlaAnimation& g);
// Die Knochen einer Kette (Anfang und alle Nachkommen), Eltern zuerst.
[[nodiscard]] std::vector<int> kettenKnochen(const GlaAnimation& g, int anfang);

}  // namespace bhed::modell
#endif
