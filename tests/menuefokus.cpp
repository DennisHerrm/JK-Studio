// menuefokus.cpp - Menue-Editor M6 Punkt 2: Tab-Reihenfolge wie Menu_SetNextCursorItem/Item_SetFocus
// (ab cursorItem -1 beginnt die Engine bei Index 1) und Neuordnen per Klickfolge.
#include "bhed/menue/skript.h"
#include "bhed/menue/werkzeuge.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace bhed::menue;

namespace {
int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}
std::string item(const std::string& name, const std::string& rest) {
    return "\t\titemDef\n\t\t{\n\t\t\tname " + name + "\n\t\t\trect 10 10 80 20\n" + rest + "\t\t}\n";
}
std::vector<std::string> namen(const Datei& d, const std::vector<int>& v) {
    std::vector<std::string> r;
    for (int i : v) { r.push_back(d.menus[0].items[static_cast<std::size_t>(i)].window.name); }
    return r;
}
}  // namespace

int main() {
    const std::string knopf = "\t\t\ttype ITEM_TYPE_BUTTON\n\t\t\tvisible 1\n\t\t\taction { close m }\n";
    const std::string t = "{\n\tmenuDef\n\t{\n\t\tname m\n\t\trect 0 0 640 480\n\t\tvisible 1\n" + item("a", knopf) +
                          item("deko", "\t\t\tdecoration\n\t\t\tvisible 1\n") + item("weg", "\t\t\ttype ITEM_TYPE_BUTTON\n\t\t\tvisible 0\n") +
                          item("b", knopf) + item("c", knopf) +
                          item("aus", knopf + "\t\t\tcvarTest \"x\"\n\t\t\tenableCvar { 1 }\n") +
                          item("txt", "\t\t\ttext \"Hallo\"\n\t\t\tvisible 1\n") + "\t}\n}\n";
    LeseOptionen o;
    const Datei d = lies("ui/fokus.menu", t, o);
    pruefe(d.menus.size() == 1 && d.menus[0].items.size() == 7 && d.meldungen.empty(), "gelesen ohne Meldung");
    Laufzeit l;
    l.start(d);
    l.oeffne("m");
    // x ist leer: enableCvar { 1 } sperrt "aus"; Dekoration und versteckte fehlen; ab -1 zuerst Index 1+,
    // "a" (Index 0) kommt nach dem Umlauf; Text-Items nimmt Item_SetFocus an
    const std::vector<int> folge = l.tabReihenfolge(0);
    pruefe(namen(d, folge) == std::vector<std::string>{"b", "c", "txt", "a"}, "Tab-Reihenfolge b c txt a");
    l.setzeCvar("x", "1");
    pruefe(namen(d, l.tabReihenfolge(0)) == std::vector<std::string>{"b", "c", "aus", "txt", "a"}, "enableCvar erfuellt: aus kommt dazu");
    // gewollt: c, a, txt, b - auf die Plaetze der jetzigen Folge verteilt
    std::vector<int> gewollt;
    for (const char* n : {"c", "a", "txt", "b"}) {
        for (std::size_t i = 0; i < d.menus[0].items.size(); ++i) {
            if (d.menus[0].items[i].window.name == n) { gewollt.push_back(static_cast<int>(i)); }
        }
    }
    std::vector<Ersetzung> e;
    pruefe(itemsNeuOrdnen(d, 0, folge, gewollt, e), "neu ordnen");
    const Datei d2 = lies("ui/fokus.menu", anwenden(d.text, e), o);
    pruefe(d2.meldungen.empty() && d2.menus.size() == 1 && d2.menus[0].items.size() == 7, "neu gelesen ohne Meldung");
    Laufzeit l2;
    l2.start(d2);
    l2.oeffne("m");
    pruefe(namen(d2, l2.tabReihenfolge(0)) == std::vector<std::string>{"c", "a", "txt", "b"}, "Tab-Reihenfolge wie geklickt");
    pruefe(d2.menus[0].items[1].window.name == "deko" && d2.menus[0].items[2].window.name == "weg" && d2.menus[0].items[5].window.name == "aus",
           "die anderen Items bleiben an ihrer Stelle");
    std::vector<Ersetzung> f;
    pruefe(!itemsNeuOrdnen(d, 0, folge, {0, 0, 3, 4}, f), "doppelte Items abgelehnt");
    if (g_fehler == 0) { std::printf("menue_fokus: alles gruen\n"); }
    return g_fehler == 0 ? 0 : 1;
}
