// modell/autoskin.h - Auto-Skin: Gewichte eines Netzes aus dem Skelett seiner GLA (AP12, Abgleich W7)
//
// shank, 04.11.2026: "einen guten algorythmus zum auto skinning". Plan: plaene/modellblock_stufe0/
// skinning_und_retargeting.md, Abschnitt 3. Eigener Code; die Idee der geodaetischen Voxel-Bindung stammt von Dionne
// und de Lasa ("Geodesic Voxel Binding for Production Character Meshes", SCA 2013) - der Abstand eines Netzpunkts zu
// einem Knochen wird DURCH das Netz gemessen, nicht durch die Luft, so klebt ein Arm nicht am Rumpf. Abstand -> Gewicht,
// Knochenformen und alle JKA-Schritte sind von hier (das Paper war nicht abrufbar, es wurde nichts uebernommen).
//
// Drei Verfahren (AutoSkinVerfahren), danach dieselben JKA-Schritte:
//
//   Uebertragen  Gewichte vom naechsten Punkt eines Originals mit demselben Skelett (z. B. kyle), nach Flaechennamen
//                zugeordnet; Punkte, die weiter als `uebertragenAbstand` vom Original liegen, bekommen Voxel-Gewichte
//   Voxel        alle Flaechen der Stufe 0 (auch Kappen) in ein Voxelgitter, Inneres per Flutfuellung von aussen;
//                je Knochen der kuerzeste Weg durch die festen Voxel (Dijkstra, 26 Nachbarn); mit freier Sicht durch
//                das Netz gilt der gerade Abstand (er IST dann der kuerzeste Weg)
//   Huelle       der gerade Abstand zur Knochenform - schnell, springt aber ueber Luft (Notloesung)
//
// Knochenformen (knochenFormen): die GLA kennt nur Ursprung und Achsen eines Knochens, keine Laenge. Die Form eines
// Knochens sind die Strecken von seinem Ursprung zu den Urspruengen seiner Formkinder. Formkinder sind die Kinder im
// Skelett - ausser wo das Skelett eine Kette flach nebeneinander haengt (JKA: Drehknochen, Hand und Finger alle unter
// rradius, der Oberarm neben dem Schluesselbein unter thoracic): dann haengen Geschwister nach dem Rang ihrer Rolle
// (bhed/modell/rig.h, erkenneKnochen) bzw. nach Zaehlnamen (r_d1_j1 -> r_d1_j2) hintereinander. Gemessen an Ravens
// Modellen (Y:\werkzeuge\ap12\erkunden2.py, 7 Figuren, 25203 Ecken): lhumerusX traegt die untere Haelfte des Oberarms
// (Anteil 0,66 bis 0,83 bei 60 bis 90 % der Strecke), am Ellbogen mischen lhumerusX und lradius - der Drehknochen ist
// also ein Kettenglied von seinem Ursprung bis zum naechsten Gelenk. Ein Blattknochen (kein Formkind) bekommt eine
// Strecke durch den flaechengewichteten Schwerpunkt seiner Zelle (die Punkte, denen er am naechsten ist), doppelt so
// lang wie der Weg dorthin: der Schwerpunkt einer gleichmaessigen Roehre liegt in ihrer Mitte.
//
// Welche Knochen Gewichte tragen (ohne eigene Auswahl): alle ausser Wurzel, Bewegung und Tag-Knochen (Raven: 0 Ecken
// an model_root, Motion, lhang_tag_bone, rhang_tag_bone in den 7 Figuren) und ausser Knochen, die genau im Ursprung
// ihres Elternknochens sitzen und selbst Kinder haben (face unter cranium: 0 Ecken). Gesicht und Anhaenge (Rolle
// Gesicht, Anhang) nur mit `detailknochen`: ihre Urspruenge liegen an der Oberflaeche, und ohne Vorlage laesst sich
// ihr kleiner Bereich nicht aus der Lage ableiten (ctest an kyle: mit ihnen bleibt der Schaedel nur an 174 von 455 Ecken der
// staerkste Knochen, und der staerkste Knochen stimmt an 62,8 statt 74,3 % der Ecken mit Raven ueberein).
//
// JKA-Schritte (alle Verfahren): Ecken gleichen Orts sind ein Punkt und bekommen gleiche Gewichte (Naehte), Glaetten
// ueber das geschweisste Netz, hoechstens Grenzen::gewichteJeEcke je Ecke, hoechstens Grenzen::knochenJeFlaeche
// Knochen je Flaeche (die mit dem kleinsten Gesamtgewicht der Flaeche fallen weg), dann fuersSpiel (gewichte.h):
// 10 Bit, letztes Gewicht = 1 - Summe, wie G2_GetVertBoneWeight liest.
#ifndef BHED_MODELL_AUTOSKIN_H
#define BHED_MODELL_AUTOSKIN_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "bhed/gla.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/g2.h"
#include "bhed/modell/rig.h"

namespace bhed::modell {

enum class AutoSkinVerfahren : std::uint8_t { Uebertragen = 0, Voxel, Huelle };
inline constexpr int kAutoSkinVerfahren = 3;
// Voxel ueber die laengste Seite des Netzes. Unter 32 liegen Arm und Rumpf einer Figur im selben Voxel; ueber 512
// braucht allein das Gitter einer Figur mehr als 100 MB.
inline constexpr int kAutoSkinAufloesungMin = 32;
inline constexpr int kAutoSkinAufloesungMax = 512;
inline constexpr int kAutoSkinGlaettenMax = 10;

struct AutoSkinOptionen {
    AutoSkinVerfahren verfahren = AutoSkinVerfahren::Voxel;
    int aufloesung = 256;
    // 0 weich .. 1 steif: Gewicht ~ (kleinster Abstand / Abstand)^k, k = autoSkinExponent(steifigkeit)
    float steifigkeit = 0.3F;
    int glaetten = 2;             // Durchgaenge: Punkt = Mittel aus sich und dem Mittel seiner Nachbarn
    float schwelle = 0.0F;        // kleinere Gewichte fallen weg (nach Glaetten und Begrenzen), der Rest wird auf 1 gebracht
    bool naehte = true;           // Ecken gleichen Orts ueber Flaechen hinweg als ein Punkt (aus: nur innerhalb der Flaeche)
    bool drehknochen = true;      // Drehknochen (Rollen ...Twist) tragen Gewichte
    bool detailknochen = false;   // Gesicht und Anhaenge tragen Gewichte
    bool spiegeln = false;        // danach die Seite x > 0 auf x < 0 spiegeln (gewichte.h spiegeln)
    float spiegelToleranz = 0.05F;
    Zielspiel ziel = Zielspiel::JkaSp;
    // Uebertragen: weiter als so viele Einheiten vom Original -> Voxel-Gewichte fuer diesen Punkt
    float uebertragenAbstand = 2.0F;
    std::vector<char> knochen;    // leer: Vorgabe (siehe oben); sonst je Knochen 1 = traegt Gewichte
    std::vector<char> teile;      // leer: alle Flaechen ausser Tags; sonst je Teil 1 = neu gewichten
};

// Der Exponent k zur Steifigkeit (0..1).
[[nodiscard]] double autoSkinExponent(float steifigkeit) noexcept;

// Die Form eines Knochens fuer den Abstand.
struct KnochenForm {
    bool traegt = false;
    bool blatt = false;       // ohne Formkind: Strecke durch den Schwerpunkt der Zelle (oder ein Punkt)
    int formEltern = -1;      // der tragende Knochen, dessen Form bis zum Ursprung dieses Knochens reicht (-1 keiner)
    std::array<double, 3> ursprung{};
    std::vector<std::array<double, 6>> strecken;   // je Strecke Anfang und Ende
};
// `in` darf leer sein: dann bleiben Blattknochen Punkte.
[[nodiscard]] std::vector<KnochenForm> knochenFormen(const GlaAnimation& g, const Erkennung& e, const AutoSkinOptionen& o, const Inhalt& in);

inline constexpr float kAutoSkinUnerreicht = 3.0e38F;

// Die Abstaende aller Punkte zu allen Knochen (fuer autoSkin, Tests und spaeter die Anzeige).
struct AutoSkinFeld {
    int knochen = 0;
    int punkte = 0;
    std::vector<KnochenForm> formen;
    // [teil][stufe][ecke] -> Punkt, -1: die Flaeche wird nicht gewichtet
    std::vector<std::vector<std::vector<int>>> punktVon;
    std::vector<std::array<double, 3>> ort;        // je Punkt
    std::vector<float> abstand;                    // punkte * knochen; kAutoSkinUnerreicht: kein Weg / traegt nicht
    // Voxel
    int voxel[3]{};
    double voxelGroesse = 0.0;
    int voxelFest = 0;          // Voxel an der Oberflaeche oder im Inneren
    int voxelInnen = 0;         // davon im Inneren
    int knochenAussen = 0;      // Knochen, deren Form nirgends durch ein festes Voxel laeuft (offenes Netz, Knochen daneben)
    int punkteUnerreicht = 0;   // Punkte ohne Weg zu irgendeinem Knochen: gerader Abstand
    int punkteSicht = 0;        // Punkt-Knochen-Paare mit freier Sicht (gerader Abstand)
    [[nodiscard]] float d(int punkt, int k) const noexcept {
        return abstand[static_cast<std::size_t>(punkt) * static_cast<std::size_t>(knochen) + static_cast<std::size_t>(k)];
    }
};
[[nodiscard]] AutoSkinFeld autoSkinFeld(const Inhalt& in, const GlaAnimation& g, const AutoSkinOptionen& o);

struct AutoSkinBericht {
    bool ok = false;
    std::string fehler;           // "skelett" (GLA leer oder andere Knochenzahl), "flaechen" (nichts zu gewichten),
                                  // "knochen" (kein Knochen traegt), "vorlage" (Uebertragen ohne passendes Original)
    int ecken = 0;                // neu gewichtete Ecken (alle Stufen)
    int punkte = 0;
    int knochenTragend = 0;
    int knochenBenutzt = 0;       // Knochen mit Gewicht > 0 im Ergebnis
    int gewichteMax = 0;          // meiste Gewichte an einer Ecke im Ergebnis
    int eckenGekuerzt = 0;        // Ecken, die vor dem Begrenzen mehr Gewichte hatten als die Grenze
    int gewichteGrenze = 0;
    int knochenJeFlaecheMax = 0;  // meiste Knochen einer Flaeche im Ergebnis
    std::string knochenJeFlaecheName;
    int knochenGrenze = 0;
    int flaechenGekuerzt = 0;     // Flaechen, denen Knochen genommen wurden
    int uebertragen = 0;          // Uebertragen: Punkte vom Original
    int fern = 0;                 // Uebertragen: Punkte mit Voxel-Gewichten
    int gespiegelt = 0;
    int voxel[3]{};
    double voxelGroesse = 0.0;
    int voxelFest = 0;
    int voxelInnen = 0;
    int knochenAussen = 0;
    int punkteUnerreicht = 0;
};

// Die gewaehlten Flaechen aller Stufen neu gewichten. `vorlage` (nur Uebertragen): ein Original mit demselben Skelett;
// `vorlageKnochen` leer = gleiche Knochennummern, sonst je Knochen der Vorlage die Nummer in `g` (-1: faellt weg).
// Mallisten der gewichteten Flaechen verschwinden (das Ergebnis steht in den Ecken, wie nach uebernehmen()).
// `feld`: die Abstaende von autoSkinFeld fuer dasselbe Netz und dieselben Optionen (Verfahren, Aufloesung, Naehte, Knochen,
// Flaechen) - Steifigkeit, Glaetten und Schwelle aendern sie nicht, ein zweiter Lauf rechnet damit nur die Gewichte neu.
// Passt es nicht zum Netz (Punkte, Knochen), wird neu gerechnet.
bool autoSkin(Inhalt& in, const GlaAnimation& g, const AutoSkinOptionen& o, AutoSkinBericht& bericht, const Inhalt* vorlage = nullptr,
              const std::vector<int>& vorlageKnochen = {}, const AutoSkinFeld* feld = nullptr);

// --- Vergleich zweier Gewichtungen desselben Netzes (Gegenprobe an Raven-Modellen, Tabelle der Oberflaeche) --------
struct GewichtVergleich {
    int ecken = 0;
    int hauptGleich = 0;          // Ecken, deren staerkster Knochen gleich ist
    double abweichungMittel = 0;  // Mittel ueber die Ecken von 1/2 Summe |w_a - w_b| (0 gleich .. 1 kein gemeinsamer Knochen)
    std::vector<int> eckenJeKnochen;       // je Knochen: Ecken, an denen er in `a` der staerkste ist
    std::vector<int> hauptGleichJeKnochen; // ... und in `b` auch
};
// Flaechen ohne Tags; `teile` leer = alle. Die Netze muessen gleich viele Ecken haben (sonst zaehlt die Flaeche nicht).
[[nodiscard]] GewichtVergleich gewichteVergleichen(const Inhalt& a, const Inhalt& b, int stufe, int knochenZahl, const std::vector<char>& teile = {});

struct VerformungsFehler {
    int ecken = 0;
    int bilder = 0;
    double mittel = 0;   // Einheiten
    double p95 = 0;
    double max = 0;
};
// Der Abstand derselben Ecke unter beiden Gewichtungen, verformt wie das Spiel (Summe w_i * Knochen_i * Ort), ueber
// die Bilder `bilder` der GLA.
[[nodiscard]] VerformungsFehler verformungsFehler(const Inhalt& a, const Inhalt& b, const GlaAnimation& g, const std::vector<int>& bilder,
                                                  int stufe, const std::vector<char>& teile = {});

}  // namespace bhed::modell
#endif
