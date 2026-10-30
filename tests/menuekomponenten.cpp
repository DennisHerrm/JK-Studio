// menuekomponenten.cpp - Menue-Editor M7 Punkt 3: Komponenten (verknuepfte Kopien mit Kommentar-Kennung).
#include "bhed/menue/komponenten.h"

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
Datei lesen(const std::string& t) {
    LeseOptionen o;
    o.dialekt = Dialekt::SP;
    return lies("ui/k.menu", t, o);
}
}  // namespace

int main() {
    std::string t = "{\n\tmenuDef\n\t{\n\t\tname k\n\t\trect 0 0 640 480\n\t\tvisible 1\n"
                    "\t\titemDef\n\t\t{\n\t\t\tname knopf\n\t\t\ttext \"Los\"\n\t\t\trect 100 100 120 30\n\t\t\tforecolor 1 1 1 1\n\t\t\tvisible 1\n\t\t}\n"
                    "\t\titemDef\n\t\t{\n\t\t\tname anders\n\t\t\trect 10 10 20 20\n\t\t\tvisible 1\n\t\t}\n"
                    "\t}\n}\n";
    Datei d = lesen(t);
    std::vector<Ersetzung> e;
    pruefe(alsKomponente(d, t, 0, 0, "gross", e) && e.size() == 1, "Original anlegen");
    t = anwenden(t, e);
    pruefe(t.find("\t\t// jkstudio:komponente gross\n\t\titemDef") != std::string::npos, "Kennungszeile mit der Einrueckung des Items davor");
    d = lesen(t);
    std::string name;
    pruefe(komponentenRolle(d, t, 0, 0, name) == 'k' && name == "gross" && d.menus[0].items.size() == 2, "Rolle Original, Datei liest weiter zwei Items");
    std::vector<Komponente> k = komponenten(d, t);
    e.clear();
    pruefe(k.size() == 1 && kopieEinfuegen(d, t, k[0], 300, 200, "knopf_2", e), "Kopie einfuegen");
    t = anwenden(t, e);
    d = lesen(t);
    k = komponenten(d, t);
    pruefe(d.menus[0].items.size() == 3 && k.size() == 1 && k[0].instanzen.size() == 1 && k[0].instanzen[0] == 1, "Kopie direkt hinter dem Original");
    const Item& kopie = d.menus[0].items[1];
    pruefe(kopie.window.name == "knopf_2" && kopie.window.rect.x == 300 && kopie.window.rect.y == 200 && kopie.window.rect.w == 120 && kopie.text == "Los",
           "Kopie: eigener Name und Lage, sonst wie das Original");
    pruefe(kopienAbgleichen(d, t).empty(), "frisch eingefuegt: nichts abzugleichen");
    // Original aendern: Text und Breite -> die Kopie folgt, Name und Lage bleiben
    std::vector<Ersetzung> a;
    (void)setzeEigenschaft(d, 0, 0, "text", {"\"Start\""}, a);
    Rechteck r = d.menus[0].items[0].window.rect;
    r.w = 150;
    (void)setzeRect(d, 0, 0, r, a);
    t = anwenden(t, a);
    d = lesen(t);
    std::vector<Ersetzung> s = kopienAbgleichen(d, t);
    pruefe(s.size() == 1, "nach Aenderung des Originals eine Ersetzung fuer die Kopie");
    const std::string vorher = t;
    t = anwenden(t, s);
    d = lesen(t);
    pruefe(d.menus[0].items[1].text == "Start" && d.menus[0].items[1].window.rect.w == 150 && d.menus[0].items[1].window.rect.x == 300 &&
               d.menus[0].items[1].window.name == "knopf_2",
           "Kopie abgeglichen: Text und Breite vom Original, eigener Name und Lage");
    pruefe(kopienAbgleichen(d, t).empty(), "danach stabil");
    // eine Ersetzung fuer alles (ein Rueckgaengig-Schritt)
    const std::vector<Ersetzung> eins = alsEineErsetzung(vorher, t);
    pruefe(eins.size() == 1 && anwenden(vorher, eins) == t, "alsEineErsetzung");
    // die Kopie verschieben bleibt (Lage gehoert der Kopie)
    r = d.menus[0].items[1].window.rect;
    r.x = 400;
    a.clear();
    (void)setzeRect(d, 0, 1, r, a);
    t = anwenden(t, a);
    d = lesen(t);
    pruefe(kopienAbgleichen(d, t).empty(), "verschobene Kopie bleibt verschoben");
    // eine Kopie ohne Original bleibt unverbunden; doppelt anlegen geht nicht
    e.clear();
    pruefe(!alsKomponente(d, t, 0, 0, "neu", e) && !alsKomponente(d, t, 0, 2, "mit leer", e), "kein zweites Original, kein Name mit Leerzeichen");
    std::printf("menuekomponenten: %s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
