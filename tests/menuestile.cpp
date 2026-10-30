// menuestile.cpp - Menue-Editor M6 Punkt 4: Stile zentral - Werte sammeln (Farben ueber alle
// Farbschluessel, Schriften, Textgroessen, Hintergruende) und an allen Stellen auf einmal ersetzen;
// Abstaende, Kommentare und Zitate bleiben, der Parser liest das Ergebnis ohne Meldung.
#include "bhed/menue/stile.h"

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
const StilWert* finde(const std::vector<StilWert>& v, StilArt a, const std::string& w) {
    for (const StilWert& x : v) {
        if (x.art == a && x.wert == w) { return &x; }
    }
    return nullptr;
}
}  // namespace

int main() {
    const std::string t =
        "{\n\tmenuDef\n\t{\n\t\tname m\n\t\trect 0 0 640 480\n\t\tvisible 1\n\t\tfocusColor 1 0.80 0.2 1\n\t\tbackground \"gfx/menus/hinten\"\n"
        "\t\titemDef\n\t\t{\n\t\t\tname a\n\t\t\trect 10 10 80 20\n\t\t\tforecolor 1  .8  0.2 1 // gelb\n\t\t\ttextscale 0.8\n\t\t\tfont 2\n\t\t\tvisible 1\n\t\t}\n"
        "\t\titemDef\n\t\t{\n\t\t\tname b\n\t\t\trect 10 40 80 20\n\t\t\tforecolor 1 1 1 1\n\t\t\ttextscale .80\n\t\t\tfont 2\n"
        "\t\t\tbackground gfx/menus/hinten\n\t\t\tvisible 1\n\t\t}\n\t}\n}\n";
    LeseOptionen o;
    const Datei d = lies("ui/stile.menu", t, o);
    pruefe(d.meldungen.empty(), "Probe ohne Meldung gelesen");
    const std::vector<StilWert> v = stileImMenue(d);
    const StilWert* gelb = finde(v, StilArt::Farbe, "1 0.8 0.2 1");
    pruefe(gelb != nullptr && gelb->anzahl == 2 && gelb->schluessel.size() == 2, "Farbe 1 0.8 0.2 1: zweimal, in focuscolor und forecolor");
    pruefe(finde(v, StilArt::Farbe, "1 1 1 1") != nullptr, "Farbe weiss");
    const StilWert* gr = finde(v, StilArt::Textgroesse, "0.8");
    pruefe(gr != nullptr && gr->anzahl == 2, "Textgroesse 0.8 (0.8 und .80) zweimal");
    const StilWert* sc = finde(v, StilArt::Schrift, "2");
    pruefe(sc != nullptr && sc->anzahl == 2, "Schrift 2 zweimal");
    const StilWert* hg = finde(v, StilArt::Hintergrund, "gfx/menus/hinten");
    pruefe(hg != nullptr && hg->anzahl == 2, "Hintergrund zweimal (mit und ohne Anfuehrungszeichen)");
    pruefe(!v.empty() && v.front().art == StilArt::Farbe && v.front().wert == "1 0.8 0.2 1", "haeufigste Farbe zuerst");
    if (g_fehler != 0) { return 1; }
    std::vector<Ersetzung> e;
    pruefe(stilErsetzen(d, *gelb, {"0", "0.5", "1", "1"}, e), "Farbe ersetzen");
    pruefe(stilErsetzen(d, *hg, {"gfx/menus/neu"}, e), "Hintergrund ersetzen");
    const std::string n = anwenden(d.text, e);
    const Datei d2 = lies("ui/stile.menu", n, o);
    pruefe(d2.meldungen.empty(), "Ergebnis ohne Meldung gelesen");
    pruefe(n.find("forecolor 0  0.5  1 1 // gelb") != std::string::npos, "Abstaende und Kommentar bleiben");
    pruefe(n.find("focusColor 0 0.5 1 1") != std::string::npos, "focusColor mit geaendert");
    pruefe(n.find("background \"gfx/menus/neu\"") != std::string::npos && n.find("background gfx/menus/neu") != std::string::npos,
           "Zitat bleibt wie es war");
    pruefe(d2.menus[0].items[0].window.foreColor.b == 1.0F && d2.menus[0].focusColor.g == 0.5F, "Werte im Modell");
    pruefe(finde(stileImMenue(d2), StilArt::Farbe, "1 0.8 0.2 1") == nullptr, "alter Wert verschwunden");
    if (g_fehler == 0) { std::printf("menue_stile: alles gruen\n"); }
    return g_fehler == 0 ? 0 : 1;
}
