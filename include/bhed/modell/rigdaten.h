// modell/rigdaten.h - was das Control Rig im Modelldokument traegt (Modell-Block Stufe 3)
//
// Die Schluessel der Griffe (je Sequenz eine Schicht) und die Zuordnungen von Hand aus der Uebersicht gehoeren ins
// Modelldokument, damit Strg+Z sie zuruecknimmt wie jede andere Aenderung. Die Rechnung steht in bhed/modell/rig.h.
#ifndef BHED_MODELL_RIGDATEN_H
#define BHED_MODELL_RIGDATEN_H

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

struct RigSchluessel {
    int bild = 0;
    float q[4] = {1.0F, 0.0F, 0.0F, 0.0F};   // w x y z, im Rahmen des Knochens (M)
    float t[3] = {0.0F, 0.0F, 0.0F};
};
struct RigSchicht {
    std::vector<std::vector<RigSchluessel>> knochen;   // je Knochen, nach Bild sortiert
    [[nodiscard]] bool leer() const noexcept;
    [[nodiscard]] int schluesselZahl() const noexcept;
    [[nodiscard]] bool hatSchluessel(int knochen, int bild) const noexcept;
};

struct RigDaten {
    std::map<std::string, RigSchicht> schichten;   // je Sequenz (Name der animation.cfg-Zeile, "" ohne Sequenz)
    std::vector<Zuordnung> zuordnung;
};

}  // namespace bhed::modell
#endif
