// spieldaten.h - Menue-Editor M1: Dateien aus den Spielordnern lesen (nur
// lesen) - pk3 in der Reihenfolge der Engine, spaetere gewinnen, lose
// Dateien im Ordner vor den Archiven desselben Ordners.
//
// Eigener Code.
#ifndef BHED_MENUE_SPIELDATEN_H
#define BHED_MENUE_SPIELDATEN_H

#include "bhed/menue/menue.h"
#include "bhed/pk3.h"

#include <string>
#include <vector>

namespace bhed::menue {

struct Spieldaten {
    std::vector<GamePath> ordner;   // niedrigste Prioritaet zuerst (base, dann Mod)

    // Ordner anhaengen (z. B. ".../base", dann ".../MD"). false = keine pk3.
    bool ordnerDazu(const std::string& pfad);
    [[nodiscard]] bool leer() const noexcept { return ordner.empty(); }
    // Gross/Klein egal, / oder \.
    [[nodiscard]] bool lies(const std::string& pfad, std::string& out) const;
    [[nodiscard]] bool gibt(const std::string& pfad) const;
    // Alle Dateien unter praefix mit einer der Endungen (klein, mit Punkt),
    // ohne Doppelte, Pfad wie im Archiv mit der hoechsten Prioritaet.
    [[nodiscard]] std::vector<std::string> liste(const std::string& praefix,
                                                 const std::vector<std::string>& endungen) const;
    // Lader fuer menue::lies (#include, menudef.h).
    [[nodiscard]] Lader lader() const;
};

// Die @MENUS-Texte des Spiels: JA/MD strings/<sprache>/*.str, JK2 strip/*.sp.
[[nodiscard]] Texte ladeTexte(const Spieldaten& d, Spiel spiel, const std::string& sprache = "English");

}  // namespace bhed::menue

#endif  // BHED_MENUE_SPIELDATEN_H
