// komponenten.h - Menue-Editor M7 Punkt 3: Komponenten - verknuepfte Kopien eines Bauteils. Das Original
// und jede Kopie sind normale itemDefs; eine Kommentarzeile direkt davor macht sie kenntlich:
//
//     // jkstudio:komponente knopf_gross
//     itemDef { name knopf ... rect 100 100 120 30 ... }
//     // jkstudio:instanz knopf_gross
//     itemDef { name knopf_2 ... rect 300 100 120 30 ... }
//
// Die Engine liest nur die itemDefs (Kommentare zaehlen nicht). Der Editor gleicht nach jeder Aenderung
// jede Kopie mit dem Original ab: alles wie beim Original, nur der eigene Name und die eigene Lage
// (x, y des rect) bleiben - eine Aenderung am Original wirkt so auf alle Kopien.
// Eigener Code.
#ifndef BHED_MENUE_KOMPONENTEN_H
#define BHED_MENUE_KOMPONENTEN_H

#include "bhed/menue/menue.h"

#include <string>
#include <vector>

namespace bhed::menue {

inline constexpr const char* kKomponenteKennung = "// jkstudio:komponente";
inline constexpr const char* kInstanzKennung = "// jkstudio:instanz";

struct Komponente {
    std::string name;
    int menu = -1;
    int original = -1;            // Item mit "// jkstudio:komponente <name>"
    std::vector<int> instanzen;   // Items mit "// jkstudio:instanz <name>" im selben Menue
};

// Alle Komponenten der Datei (nur Items der Hauptdatei; ohne Original bleiben Kopien unverbunden)
[[nodiscard]] std::vector<Komponente> komponenten(const Datei& d, const std::string& text);
// Kennung des Items: 'k' Original, 'i' Kopie, 0 keine; name = Name der Komponente
[[nodiscard]] char komponentenRolle(const Datei& d, const std::string& text, int menu, int item, std::string& name);

// Ein Item zum Original einer Komponente machen (Kennungszeile davor, mit seiner Einrueckung)
[[nodiscard]] bool alsKomponente(const Datei& d, const std::string& text, int menu, int item, const std::string& name,
                                 std::vector<Ersetzung>& out);
// Eine Kopie des Originals hinter dem letzten Item der Komponente einfuegen, an (x, y), Name instanzName
[[nodiscard]] bool kopieEinfuegen(const Datei& d, const std::string& text, const Komponente& k, float x, float y,
                                  const std::string& instanzName, std::vector<Ersetzung>& out);
// Die Ersetzungen, die jede Kopie wieder wie ihr Original machen (eigener Name und eigene Lage bleiben)
[[nodiscard]] std::vector<Ersetzung> kopienAbgleichen(const Datei& d, const std::string& text);
// Der ganze Unterschied zwischen zwei Texten als eine Ersetzung (gleicher Anfang und Schluss bleiben)
[[nodiscard]] std::vector<Ersetzung> alsEineErsetzung(const std::string& alt, const std::string& neu);

}  // namespace bhed::menue

#endif  // BHED_MENUE_KOMPONENTEN_H
