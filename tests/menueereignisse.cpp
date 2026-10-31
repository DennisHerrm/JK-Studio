// menueereignisse.cpp - Menue-Editor M5 Punkt 2: Ereignisse je Dialekt, Skripte in Bausteine
// zerlegen und wieder zusammensetzen (wie Item_RunScript liest), Skript eines Ereignisses setzen,
// fertige Hover-Effekte - das Ergebnis liest der Parser ohne Meldung, die Laufzeit fuehrt es aus.
#include "bhed/menue/ereignisse.h"
#include "bhed/menue/skript.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <string>

using namespace bhed::menue;

namespace {
int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}
const char* const kMenue =
    "{\n\tmenuDef\n\t{\n\t\tname m\n\t\trect 0 0 640 480\n\t\tvisible 1\n"
    "\t\titemDef\n\t\t{\n\t\t\tname knopf\n\t\t\ttype ITEM_TYPE_BUTTON\n\t\t\ttext \"Los\"\n\t\t\trect 100 100 80 20\n"
    "\t\t\tforecolor 1 1 1 1\n\t\t\tvisible 1\n\t\t\taction { play \"sound/interface/button1.wav\" ; close m }\n\t\t}\n"
    "\t\titemDef\n\t\t{\n\t\t\tname bild\n\t\t\tstyle WINDOW_STYLE_FILLED\n\t\t\trect 0 0 10 10\n\t\t\tvisible 0\n\t\t}\n\t}\n}\n";
// Skript wie gelesen (der Parser speichert jedes Wort in Anfuehrungszeichen) ohne Zitate
std::string klar(const std::string& s, const Datei& d) {
    std::string r;
    for (const Baustein& b : zerlege(s, d.dialekt, d.spiel)) {
        if (!r.empty()) { r += " ; "; }
        r += b.befehl;
        for (const std::string& a : b.args) { r += " " + a; }
    }
    return r;
}
}  // namespace

int main() {
    // Ereignisse: beide Dialekte kennen Hover, Klick, Fokus und Enter als itemDef-Schluessel
    for (const Dialekt dl : {Dialekt::SP, Dialekt::MP}) {
        const auto ev = ereignisseFuer(dl, Spiel::JA);
        bool hover = false, klick = false;
        for (const Ereignis& e : ev) {
            hover = hover || e.art == 0;
            klick = klick || e.art == 2;
        }
        pruefe(hover && klick, std::string(dl == Dialekt::SP ? "SP" : "MP") + ": mouseEnter und action vorhanden");
    }
    // Zerlegen: mit und ohne ";" (Movie Duels schreibt Skripte ohne ";"), Zitate bleiben
    auto b = zerlege("show a ; setitemcolor b forecolor 1 0 0 1 ; play \"sound/x y.wav\"", Dialekt::SP, Spiel::JA);
    pruefe(b.size() == 3 && b[0].befehl == "show" && b[0].args.size() == 1 && b[1].args.size() == 6 && b[2].args[0] == "sound/x y.wav" &&
               b[2].zitiert[0],
           "zerlegen mit ;");
    b = zerlege("hide a show b uiScript character transition c 0 0 10 10 0 0 20 20 10 1 open x", Dialekt::SP, Spiel::JA);
    pruefe(b.size() == 5 && b[1].befehl == "show" && b[2].befehl == "uiScript" && b[2].args.size() == 1 && b[3].args.size() == 11 &&
               b[4].befehl == "open",
           "zerlegen ohne ; nach den Wortzahlen der Handler (" + std::to_string(b.size()) + ")");
    pruefe(zusammensetzen(zerlege("show a ; play \"sound/x y.wav\"", Dialekt::SP, Spiel::JA)) == "show a ; play \"sound/x y.wav\"",
           "zusammensetzen: Rundlauf");
    const auto bef = bausteinBefehle(Dialekt::MP, Spiel::JA);
    pruefe(std::find(bef.begin(), bef.end(), "setcvartocvar") != bef.end(), "MP kennt setcvartocvar");
    const auto befSp = bausteinBefehle(Dialekt::SP, Spiel::JA);
    pruefe(std::find(befSp.begin(), befSp.end(), "setcvartocvar") == befSp.end(), "SP kennt setcvartocvar nicht (commandList)");
    // M5b: code/ui (JA SP) und Movie Duels haben playlooped auskommentiert; Jedi Outcast SP (jk2sp.exe) hat
    // playlooped, aber kein setitemrect/setitemtext/transition2/delay; MP hat playlooped
    auto hat = [](const std::vector<std::string>& v, const char* b) { return std::find(v.begin(), v.end(), b) != v.end(); };
    const auto befMd = bausteinBefehle(Dialekt::SP, Spiel::MD), befJk2 = bausteinBefehle(Dialekt::SP, Spiel::JK2);
    pruefe(!hat(befSp, "playlooped") && !hat(befMd, "playlooped") && hat(bef, "playlooped"), "playlooped nur in MP und Jedi Outcast");
    pruefe(hat(befJk2, "playlooped") && !hat(befJk2, "setitemrect") && !hat(befJk2, "setitemtext") && !hat(befJk2, "transition2") &&
               !hat(befJk2, "delay") && hat(befSp, "setitemrect") && hat(befMd, "setitemrect"),
           "Jedi Outcast SP: commandList wie jk2sp.exe");

    LeseOptionen o;
    o.dialekt = Dialekt::SP;
    const Datei d = lies("ui/e.menu", kMenue, o);
    pruefe(d.meldungen.empty() && d.menus.size() == 1 && d.menus[0].items.size() == 2, "Probe gelesen");
    // vorhandenen Block ersetzen
    std::vector<Ersetzung> x;
    pruefe(skriptSetzen(d, 0, 0, "action", "play \"sound/interface/button2.wav\" ; open weiter", x), "action ersetzt");
    Datei d2 = lies("ui/e.menu", anwenden(kMenue, x), o);
    pruefe(d2.meldungen.empty() && klar(d2.menus[0].items[0].action, d2) == "play sound/interface/button2.wav ; open weiter",
           "action: neuer Text, das Spiel liest ihn ohne Meldung");
    // neuen Schluessel anlegen und wieder entfernen
    x.clear();
    pruefe(skriptSetzen(d, 0, 0, "mouseEnter", "show bild", x), "mouseEnter neu");
    const std::string mitHover = anwenden(kMenue, x);
    pruefe(mitHover.find("\t\t\tmouseEnter { show bild }\n\t\t}") != std::string::npos, "mouseEnter als eigene Zeile vor der }");
    d2 = lies("ui/e.menu", mitHover, o);
    x.clear();
    pruefe(skriptSetzen(d2, 0, 0, "mouseEnter", "", x) && anwenden(mitHover, x) == kMenue, "leeres Skript entfernt die Zeile - byte-gleich");
    // Laufzeit: Hover zeigt das Bild (Item_MouseEnter -> mouseEnter)
    {
        Laufzeit lz;
        lz.start(d2);
        lz.oeffne("m");
        pruefe((lz.items[0][1].flags & WINDOW_VISIBLE) == 0, "Laufzeit: 'bild' erst versteckt");
        lz.maus(0, 120, 110);
        pruefe((lz.items[0][1].flags & WINDOW_VISIBLE) != 0, "Laufzeit: Hover zeigt 'bild'");
    }
    // Hover-Effekte: Leuchten (neues Element, show/hide), Farbe, Rahmen, Groesser
    for (const HoverEffekt e : {HoverEffekt::Leuchten, HoverEffekt::Farbe, HoverEffekt::Rahmen, HoverEffekt::Groesser}) {
        x.clear();
        const bool ok = hoverEffekt(d, 0, 0, e, Farbe{1, 0.8F, 0.2F, 1}, x);
        const Datei de = lies("ui/e.menu", anwenden(kMenue, x), o);
        const int k = e == HoverEffekt::Leuchten ? 1 : 0;   // Leuchten setzt das Glow-Element davor
        const Item* kn = de.menus.empty() || de.menus[0].items.size() <= static_cast<std::size_t>(k) ? nullptr : &de.menus[0].items[static_cast<std::size_t>(k)];
        bool passt = ok && de.meldungen.empty() && kn != nullptr && kn->window.name == "knopf" && !kn->mouseEnter.empty() && !kn->mouseExit.empty();
        if (passt && e == HoverEffekt::Leuchten) {
            passt = de.menus[0].items[0].window.name == "knopf_glow" && (de.menus[0].items[0].window.flags & WINDOW_VISIBLE) == 0 &&
                    klar(kn->mouseEnter, de) == "show knopf_glow" && klar(kn->mouseExit, de) == "hide knopf_glow";
        }
        if (passt && e == HoverEffekt::Farbe) { passt = klar(kn->mouseExit, de) == "setitemcolor knopf forecolor 1 1 1 1"; }
        if (passt && e == HoverEffekt::Rahmen) { passt = kn->window.border == 1 && kn->window.borderColor.a == 0; }
        if (passt && e == HoverEffekt::Groesser) { passt = klar(kn->mouseEnter, de) == "transition knopf 100 100 80 20 96 96 88 28 10 1"; }
        pruefe(passt, "Hover-Effekt " + std::to_string(static_cast<int>(e)) + " liest sich ohne Meldung und hat mouseEnter/mouseExit");
    }
    // M5b: Jedi Outcast SP (jk2sp.exe 1.04) kennt die JA-Woerter accept, selectNext/Prev, isCharacter,
    // isSaber/2, cvarSubString, model_g2*, scrollhidden, invertyesno, xoffset, lineHeight nicht
    // (menuDef: descTextStyle, onAccept, ignoreescape) - kein Ereignis "Enter", der Parser meldet sie
    {
        const auto ej = ereignisseFuer(Dialekt::SP, Spiel::JK2);
        pruefe(std::none_of(ej.begin(), ej.end(), [](const Ereignis& x) { return std::string(x.schluessel) == "accept"; }) && ej.size() == 5,
               "Jedi Outcast SP: kein accept-Ereignis");
        LeseOptionen oj;
        oj.dialekt = Dialekt::SP;
        oj.spiel = Spiel::JK2;
        const Datei dj = lies("ui/j.menu",
                              "{ menuDef { name m rect 0 0 640 480 ignoreescape 1 visible 1\n"
                              "  itemDef { name k type ITEM_TYPE_BUTTON rect 0 0 50 20 accept { close m } xoffset 3 visible 1 } } }\n",
                              oj);
        int unbekannt = 0;
        for (const Meldung& mj : dj.meldungen) { unbekannt += mj.text.find("Unknown") != std::string::npos ? 1 : 0; }
        pruefe(unbekannt >= 3, "Jedi Outcast SP: accept, xoffset und ignoreescape unbekannt wie im Spiel");
        oj.spiel = Spiel::JA;
        pruefe(lies("ui/j.menu", "{ menuDef { name m rect 0 0 640 480 ignoreescape 1 visible 1 } }\n", oj).meldungen.empty(), "Jedi Academy SP kennt ignoreescape");
    }
    std::printf("menueereignisse: %s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
