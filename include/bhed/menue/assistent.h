// assistent.h - Menue-Editor M6 Punkt 6: Einstellungsseiten-Assistent aus Cvars. Welche
// Steuerung zu einer Cvar passt und mit welchem Bereich, lernt er aus den Original-Menues des
// Spiels (ITEM_TYPE_SLIDER mit cvarFloat <vorgabe> <min> <max>, ITEM_TYPE_YESNO,
// ITEM_TYPE_MULTI mit cvarFloatList/cvarStrList) - so wie das Spiel sie selbst anbietet.
//
// Eigener Code.
#ifndef BHED_MENUE_ASSISTENT_H
#define BHED_MENUE_ASSISTENT_H

#include "bhed/menue/menue.h"

#include <string>
#include <utility>
#include <vector>

namespace bhed::menue {

struct CvarSteuerung {
    std::string cvar;                 // wie im Original geschrieben
    std::string text;                 // Beschriftung des Originals (auch @-Schluessel)
    int typ = 0;                      // ITEM_TYPE_SLIDER, _YESNO oder _MULTI
    float vorgabe = 0, min = 0, max = 0;   // Schieber
    std::vector<std::pair<std::string, std::string>> liste;   // Auswahl: Text, Wert
    bool zeichenWerte = false;        // cvarStrList statt cvarFloatList
    std::string herkunft;             // "ui/setup.menu: setup_menu"
};

// Alle Cvars mit Steuerung aus den Dateien (die erste Fundstelle gilt), nach Namen sortiert.
[[nodiscard]] std::vector<CvarSteuerung> cvarSteuerungen(const std::vector<const Datei*>& dateien);

// Ein menuDef mit den gewaehlten Steuerungen untereinander (ab 11 in zwei Spalten), Titel und
// Zurueck-Knopf; Schluessel, die jede Fassung liest.
[[nodiscard]] std::string einstellungsSeite(const std::vector<CvarSteuerung>& wahl, const std::string& name, const std::string& einr,
                                            const std::string& nl);

}  // namespace bhed::menue

#endif  // BHED_MENUE_ASSISTENT_H
