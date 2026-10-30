// stile.h - Menue-Editor M6 Punkt 4: Stile zentral. Alle Farben, Schriften, Textgroessen und
// Hintergruende einer Datei auf einen Blick (welcher Wert, wie oft, in welchen Schluesseln) und
// an allen Stellen auf einmal aendern - als normale Werte im .menu (forecolor, font, background ...),
// eine Ersetzung je Stelle, ein Rueckgaengig-Schritt.
//
// Eigener Code.
#ifndef BHED_MENUE_STILE_H
#define BHED_MENUE_STILE_H

#include "bhed/menue/menue.h"

#include <string>
#include <vector>

namespace bhed::menue {

enum class StilArt { Farbe, Schrift, Textgroesse, Hintergrund };

struct StilWert {
    StilArt art = StilArt::Farbe;
    std::string wert;                    // einheitlich geschrieben ("1 0.8 0.2 1", "2", "0.8", "gfx/menus/x")
    std::vector<std::string> schluessel; // in welchen Schluesseln (forecolor, focuscolor ...)
    int anzahl = 0;                      // Stellen in der Hauptdatei
    int gesperrt = 0;                    // Stellen aus #include oder Makro (bleiben)
};

// Die Werte der Datei je Art, haeufigste zuerst.
[[nodiscard]] std::vector<StilWert> stileImMenue(const Datei& d);

// Jede Stelle mit dem Wert w (gleiche Art) bekommt neu (Argumente als Text, z. B. {"1","0","0","1"}).
// false = keine Stelle in der Hauptdatei.
[[nodiscard]] bool stilErsetzen(const Datei& d, const StilWert& w, const std::vector<std::string>& neu, std::vector<Ersetzung>& out);

}  // namespace bhed::menue

#endif  // BHED_MENUE_STILE_H
