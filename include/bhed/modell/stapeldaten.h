// modell/stapeldaten.h - ein Eintrag des Modifier-Stapels einer Flaeche (AP13S, MBENTWURF Teil B, rc612)
//
// Der kleine, JKA-eigene Stapel des Model-Tabs (plaene/modellblock_stufe0/vorbilder_max_blender.md, Zeile "Modifier-Stapel";
// Abgleichtabelle M1, M2, W1): je Flaeche (Teil) eine Liste von Eintraegen in der Reihenfolge, in der sie rechnen. Wie 3ds Max
// zeigt die Liste sie von unten nach oben, wie Blender von oben nach unten - gerechnet wird in beiden gleich.
// Die Daten stehen im Dokument (bhed/modell/dokument.h, Teil::stapel): jede Aenderung am Stapel ist ein Rueckgaengig-Schritt.
// Rechnen: bhed/modell/stapel.h.
#ifndef BHED_MODELL_STAPELDATEN_H
#define BHED_MODELL_STAPELDATEN_H

#include <cstdint>

namespace bhed::modell {

// Spiegeln: wie 3ds Max "Symmetry" und Blender "Mirror" - die Flaeche an einer Achsenebene schneiden und eine Seite auf die
// andere spiegeln (wof::symmetrisieren). Glaetten: wie 3ds Max "TurboSmooth" und Blender "Subdivision Surface" - unterteilen
// nach Catmull-Clark (stapel.h).
enum class ModArt : std::uint8_t { Spiegeln = 0, Glaetten = 1 };

// Glaetten: so viele Unterteilungen hoechstens (jede macht aus einem Dreieck drei Vierecke, also sechs Dreiecke; aus einem
// Viereck vier: nach drei Schritten sind es 96-mal so viele Dreiecke wie vorher).
inline constexpr int kGlaettenStufenMax = 3;

struct Modifikator {
    ModArt art = ModArt::Spiegeln;
    // Kennung im Dokument (bleibt beim Umordnen; 0: noch keine)
    std::uint32_t kennung = 0;
    // das Auge: der Eintrag rechnet (in der Ansicht und - mit `datei` - in der Datei)
    bool an = true;
    // der Eintrag geht beim Speichern und beim Export in die Datei. false: "nur Vorschau" - die Ansicht zeigt ihn, die Datei
    // bleibt, wie sie ohne ihn waere.
    bool datei = true;
    // Spiegeln: die Achse quer zur Ebene (0 X, 1 Y, 2 Z; die Ebene geht durch den Ursprung des Modells) und die Richtung
    // (true: die Plus-Seite bleibt und wird auf die Minus-Seite gespiegelt)
    std::uint8_t achse = 0;
    bool vonPlus = true;
    // Glaetten: Zahl der Unterteilungen (1..kGlaettenStufenMax); offene Raender bleiben, wo sie sind (die Flaechen einer .glm
    // stossen an ihren Raendern aneinander - ein Rand, der sich mitglaettet, reisst dort auf)
    std::uint8_t stufen = 1;
    bool raenderHalten = true;

    [[nodiscard]] bool operator==(const Modifikator&) const = default;
};

}  // namespace bhed::modell
#endif
