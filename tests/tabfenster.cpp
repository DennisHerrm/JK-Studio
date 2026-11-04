// Probe fuer rc609 T1 (bhed/fensteranordnung.h): ganze Tabs in eigenen
// Fenstern - Kodieren und Lesen der Liste, Ablehnen kaputter Zeilen und die
// Lage beim Laden (Bildschirm noch da, Bildschirm fehlt, zu gross).
#include "bhed/fensteranordnung.h"
#include "bhed/settings.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace bhed;
using fenster::Rechteck;
using fenster::TabFenster;

namespace {
int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    std::printf("%s %s\n", ok ? "OK    " : "FEHLER", was.c_str());
    if (!ok) { ++g_fehler; }
}
std::string text(const Rechteck& r) {
    return std::to_string(r.x) + "," + std::to_string(r.y) + " " + std::to_string(r.b) + "x" + std::to_string(r.h);
}
}  // namespace

int main() {
    // --- Kodieren und Lesen ---------------------------------------------------
    const std::vector<TabFenster> zwei = {
        TabFenster{1, Rechteck{2000, 40, 1800, 1000}, Rechteck{1920, 0, 2560, 1400}},
        TabFenster{3, Rechteck{-1700, -20, 1600, 900}, Rechteck{-1920, -40, 1920, 1040}},
    };
    const std::string z = fenster::tabFensterKodieren(zwei);
    pruefe(z == "tab1|1,2000,40,1800,1000,1920,0,2560,1400|3,-1700,-20,1600,900,-1920,-40,1920,1040", "kodiert: " + z);
    std::vector<TabFenster> l;
    pruefe(fenster::tabFensterDekodieren(z, l) && l == zwei, "dekodieren(kodieren(x)) == x (auch negative Lagen)");
    pruefe(fenster::tabFensterDekodieren("tab1", l) && l.empty(), "leere Liste: kein Tab in eigenem Fenster");
    pruefe(fenster::tabFensterKodieren({}) == "tab1", "leere Liste kodiert");

    // --- Kaputtes wird abgelehnt, das Ergebnis ist dann leer -------------------
    const char* const kaputt[] = {
        "tab0|1,0,0,10,10,0,0,100,100",                    // fremder Kopf
        "tab1x",                                           // Kopf mit Anhang
        "tab1|5,0,0,10,10,0,0,100,100",                    // Bereich 5 gibt es nicht
        "tab1|-1,0,0,10,10,0,0,100,100",                   // negativer Bereich
        "tab1|1,0,0,0,10,0,0,100,100",                     // Breite 0
        "tab1|1,0,0,10,10,0,0,100",                        // eine Zahl zu wenig
        "tab1|1,0,0,10,10,0,0,100,100,7",                  // eine zu viel
        "tab1|1,0,0,1a,10,0,0,100,100",                    // keine Zahl
        "tab1|1,0,0,10,10,0,0,100,100|1,5,5,10,10,0,0,100,100",   // Tab doppelt
        "tab1|1,0,0,99999999,10,0,0,100,100",              // Zahl zu lang
        "tab1|0,0,0,9,9,0,0,9,9|1,0,0,9,9,0,0,9,9|2,0,0,9,9,0,0,9,9|3,0,0,9,9,0,0,9,9|4,0,0,9,9,0,0,9,9",   // alle Tabs
    };
    for (const char* k : kaputt) {
        l = zwei;
        pruefe(!fenster::tabFensterDekodieren(k, l) && l.empty(), std::string("abgelehnt: ") + k);
    }

    // --- Lage beim Laden -------------------------------------------------------
    const Rechteck links{0, 0, 1920, 1040};
    const Rechteck rechts{1920, 0, 2560, 1400};
    const std::vector<Rechteck> beide = {links, rechts};
    const TabFenster f{1, Rechteck{2000, 40, 1800, 1000}, rechts};
    TabFenster n = fenster::aufBildschirm(f, beide, links);
    pruefe(n == f, "Bildschirm noch da: unveraendert (" + text(n.lage) + ")");

    // Anderer Arbeitsbereich (Taskleiste verschoben), Mitte liegt trotzdem drauf
    const Rechteck rechts2{1920, 40, 2560, 1360};
    n = fenster::aufBildschirm(f, {links, rechts2}, links);
    pruefe(n.lage == Rechteck{2000, 40, 1800, 1000} && n.bildschirm == rechts2, "Mitte auf einem Bildschirm: bleibt, neuer Arbeitsbereich (" +
                                                                                     text(n.lage) + " auf " + text(n.bildschirm) + ")");

    // Zweiter Bildschirm abgezogen: mittig auf den des Hauptfensters
    n = fenster::aufBildschirm(f, {links}, links);
    pruefe(n.lage == Rechteck{60, 20, 1800, 1000} && n.bildschirm == links, "Bildschirm fehlt: mittig auf dem Ersatz (" + text(n.lage) + ")");

    // Zu gross fuer den Ersatz: verkleinert, ganz darauf
    const TabFenster gross{3, Rechteck{2000, 0, 2500, 1400}, rechts};
    n = fenster::aufBildschirm(gross, {links}, links);
    pruefe(n.lage == Rechteck{0, 0, 1920, 1040}, "zu gross: auf den Ersatz verkleinert (" + text(n.lage) + ")");

    // Ragt ueber den Rand seines Bildschirms: hineingeschoben
    const TabFenster rand{2, Rechteck{1500, 600, 800, 600}, links};
    n = fenster::aufBildschirm(rand, {links}, links);
    pruefe(n.lage == Rechteck{1120, 440, 800, 600}, "ragt hinaus: hineingeschoben (" + text(n.lage) + ")");

    // --- Durch die Einstellungsdatei ------------------------------------------
    Settings s;
    s.fenster[fenster::kTabSchluessel] = z;
    s.fenster[fenster::schluesselTabBenannt(1, "Zwei Bildschirme")] = z;
    Settings t;
    pruefe(readSettings(writeSettings(s), t) && t.fenster == s.fenster, "Settings: laufende und benannte Liste ueberstehen Schreiben und Lesen");
    pruefe(fenster::benannte(t.fenster, 1).empty(), "benannte(): die Tab-Liste ist keine eigene Anordnung des Bereichs");

    std::printf("%s: %d Fehler\n", g_fehler == 0 ? "tabfenster gruen" : "tabfenster ROT", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
