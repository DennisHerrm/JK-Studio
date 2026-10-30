// ereignisse.h - Menue-Editor M5 Punkt 2: Ereignisse je Element wie in Unreal UMG.
// Hover = mouseEnter, Hover verlassen = mouseExit, Klick = action, Fokus = onFocus,
// Fokus verlassen = leaveFocus, Enter = accept. Die Aktionen sind Bausteine (show, hide,
// fadein, setitemcolor, play, open, setcvar, transition ...), die den Skripttext schreiben -
// der Text-Reiter zeigt dasselbe, weil alles ueber den Quelltext laeuft. Nur, was die
// Engine des Dialekts kennt (commandList von ui_shared, itemDef-Schluessel).
//
// Eigener Code.
#ifndef BHED_MENUE_EREIGNISSE_H
#define BHED_MENUE_EREIGNISSE_H

#include "bhed/menue/menue.h"

#include <string>
#include <vector>

namespace bhed::menue {

struct Ereignis {
    const char* schluessel;   // itemDef-Schluessel (mouseEnter ...)
    int art;                  // 0 Hover, 1 Hover verlassen, 2 Klick, 3 Fokus, 4 Fokus verlassen, 5 Enter
};
// Die Ereignisse, die der Dialekt als itemDef-Schluessel kennt (in dieser Reihenfolge).
[[nodiscard]] std::vector<Ereignis> ereignisseFuer(Dialekt d, Spiel s);

// Ein Befehl des Skripts mit seinen Woertern; zitiert[i]: Wort stand in Anfuehrungszeichen.
struct Baustein {
    std::string befehl;
    std::vector<std::string> args;
    std::vector<bool> zitiert;
};
// Zerlegt wie Item_RunScript (String_Parse; ";" trennt; ohne ";" liest jeder Befehl so viele
// Woerter, wie sein Handler nimmt; uiScript mit den Woertern des Skripts).
[[nodiscard]] std::vector<Baustein> zerlege(const std::string& skript, Dialekt d, Spiel s);
// Wieder zusammen: "befehl arg ... ; befehl ..." (Woerter mit Leerzeichen oder zitierte in "").
[[nodiscard]] std::string zusammensetzen(const std::vector<Baustein>& b);
// Die Befehle der commandList des Dialekts (fuer die Bausteinliste), dazu uiScript.
[[nodiscard]] std::vector<std::string> bausteinBefehle(Dialekt d, Spiel s);
// Woerter, die der Handler liest (-1 = bis ";" bzw. Ende).
[[nodiscard]] int bausteinWoerter(const std::string& befehl, Dialekt d);

// Skript eines Ereignisses setzen: vorhandenen Block { ... } ersetzen, sonst den Schluessel mit
// Block vor die } des itemDef (Einrueckung des Items). Leeres Skript entfernt den Schluessel.
[[nodiscard]] bool skriptSetzen(const Datei& d, int menu, int item, const char* schluessel, const std::string& skript,
                                std::vector<Ersetzung>& out);

// Fertige Hover-Effekte (nur Befehle, die die Engine des Dialekts hat):
//   Leuchten: neues Element <name>_glow (WINDOW_STYLE_SHADER gfx/menus/menu_buttonback, visible 0,
//             decoration) vor dem Element; mouseEnter show, mouseExit hide
//   Farbe:    mouseEnter setitemcolor <name> forecolor <farbe>, mouseExit zurueck auf forecolor
//   Rahmen:   border 1, bordercolor mit Alpha 0; mouseEnter setitemcolor bordercolor <farbe>
//   Groesser: mouseEnter transition auf rect + 4 Punkte je Seite, mouseExit zurueck
enum class HoverEffekt { Leuchten, Farbe, Rahmen, Groesser };
[[nodiscard]] bool hoverEffekt(const Datei& d, int menu, int item, HoverEffekt e, const Farbe& farbe, std::vector<Ersetzung>& out);

}  // namespace bhed::menue

#endif
