// menuewerkzeug.cpp - Menue-Editor M1 Punkt 4: Einrasten, Ausrichten,
// Verteilen, Items einfuegen/kopieren/loeschen/verschieben ueber Spannen
// (nur die betroffenen Zeilen aendern sich).
#include "bhed/menue/werkzeuge.h"

#include <cmath>
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
bool nah(float a, float b) { return std::fabs(a - b) < 1e-3F; }
}  // namespace

int main() {
    // Einrasten an der rechten Kante eines anderen (Toleranz 4)
    Einrasten e = einrasten(Rechteck{102, 50, 20, 20}, {Rechteck{0, 0, 100, 30}}, 8, 4, false, true);
    pruefe(nah(e.dx, -2) && !e.linien.empty() && e.linien.front().senkrecht && nah(e.linien.front().wert, 100), "Einrasten an Kante x=100");
    e = einrasten(Rechteck{13, 13, 20, 20}, {}, 8, 2, true, false);
    pruefe(nah(e.dx, 3) && nah(e.dy, 3), "Einrasten am Raster 8 -> 16");
    std::vector<Rechteck> r{{10, 10, 50, 20}, {30, 40, 20, 20}, {5, 80, 70, 10}};
    ausrichten(r, Ausrichtung::Links);
    pruefe(nah(r[1].x, 10) && nah(r[2].x, 10), "links ausrichten");
    ausrichten(r, Ausrichtung::MitteH);
    pruefe(nah(r[1].x, 25) && nah(r[2].x, 0), "mittig ausrichten");
    std::vector<Rechteck> v{{0, 0, 10, 10}, {50, 0, 10, 10}, {100, 0, 20, 10}};
    verteilen(v, true);
    pruefe(nah(v[1].x, 50 - 5 + 5) || nah(v[1].x, 50), "verteilen: gleiche Luecken");
    pruefe(nah(v[1].x - (v[0].x + v[0].w), v[2].x - (v[1].x + v[1].w)), "verteilen: Luecken gleich");

    const std::string t =
        "{\n"
        "\tmenuDef\n"
        "\t{\n"
        "\t\tname\t\"m\"\n"
        "\t\titemDef\n"
        "\t\t{\n"
        "\t\t\tname\t\teins\n"
        "\t\t\trect\t\t0 0 10 10\n"
        "\t\t\tvisible\t\t1\n"
        "\t\t}\n"
        "\t\titemDef\n"
        "\t\t{\n"
        "\t\t\tname\t\tzwei\n"
        "\t\t\trect\t\t20 0 10 10\n"
        "\t\t\tvisible\t\t1\n"
        "\t\t}\n"
        "\t}\n"
        "}\n";
    LeseOptionen o;
    o.dialekt = Dialekt::SP;
    const Datei d = lies("ui/w.menu", t, o);
    pruefe(d.menus.size() == 1 && d.menus[0].items.size() == 2, "gelesen");

    std::vector<Ersetzung> x;
    pruefe(itemLoeschen(d, 0, 0, x), "loeschen");
    std::string neu = anwenden(d.text, x);
    pruefe(neu.find("eins") == std::string::npos && neu.find("\t\tname\t\"m\"\n\t\titemDef\n\t\t{\n\t\t\tname\t\tzwei") != std::string::npos,
           "Item samt Zeilen entfernt, Rest unberuehrt");

    x.clear();
    pruefe(itemVerschieben(d, 0, 0, true, x), "verschieben");
    neu = anwenden(d.text, x);
    Datei d2 = lies("ui/w.menu", neu, o);
    pruefe(d2.menus[0].items.size() == 2 && d2.menus[0].items[0].window.name == "zwei" && d2.menus[0].items[1].window.name == "eins" &&
               neu.size() == t.size(),
           "Reihenfolge getauscht, gleiche Laenge");

    // M2 Ebenenliste: an eine Stelle ziehen - drei Items, Kommentar zwischen den Bloecken bleibt stehen
    {
        const std::string t3 = "{\n\tmenuDef\n\t{\n\t\tname\tm\n"
                               "\t\titemDef { name a rect 0 0 10 10 }\n"
                               "\t\t// Kommentar\n"
                               "\t\titemDef { name b rect 0 0 10 10 }\n"
                               "\t\titemDef { name c rect 0 0 10 10 }\n"
                               "\t}\n}\n";
        const Datei d3 = lies("ui/w.menu", t3, o);
        x.clear();
        pruefe(itemAnStelle(d3, 0, 2, 0, x) && x.size() == 1, "an Stelle: c nach ganz hinten, eine Ersetzung");
        const std::string n3 = anwenden(d3.text, x);
        const Datei e3 = lies("ui/w.menu", n3, o);
        pruefe(e3.menus[0].items.size() == 3 && e3.menus[0].items[0].window.name == "c" && e3.menus[0].items[1].window.name == "a" &&
                   e3.menus[0].items[2].window.name == "b" && n3.size() == t3.size() && n3.find("// Kommentar") != std::string::npos,
               "an Stelle: Reihenfolge c a b, Kommentar und Laenge bleiben");
        x.clear();
        pruefe(itemAnStelle(d3, 0, 0, 2, x), "an Stelle: a nach ganz vorn");
        const Datei f3 = lies("ui/w.menu", anwenden(d3.text, x), o);
        pruefe(f3.menus[0].items[0].window.name == "b" && f3.menus[0].items[2].window.name == "a", "an Stelle: Reihenfolge b c a");
    }

    // Kopieren + Einfuegen
    const std::string kopie = umbenennen(itemText(d, 0, 0), freierName(d.menus[0], "eins"), Dialekt::SP);
    pruefe(kopie.find("name\t\teins_2") != std::string::npos, "Kopie umbenannt: eins_2");
    x.clear();
    pruefe(itemEinfuegen(d, 0, kopie, x), "einfuegen");
    neu = anwenden(d.text, x);
    d2 = lies("ui/w.menu", neu, o);
    pruefe(d2.menus[0].items.size() == 3 && d2.menus[0].items[2].window.name == "eins_2" && neu.find("\t\t}\n\t}\n}\n") != std::string::npos,
           "Kopie am Ende des Menues, vor der }");

    // Vorlagen aller Typen lesen sich in SP und MP
    for (const int typ : {-1, ITEM_TYPE_TEXT, ITEM_TYPE_BUTTON, ITEM_TYPE_YESNO, ITEM_TYPE_MULTI, ITEM_TYPE_SLIDER, ITEM_TYPE_EDITFIELD,
                          ITEM_TYPE_LISTBOX, ITEM_TYPE_MODEL, ITEM_TYPE_BIND}) {
        x.clear();
        const std::string vorlage = itemVorlage(typ, Dialekt::SP, "neu", Rechteck{1, 2, 3, 4}, "\t\t", "\n");
        pruefe(itemEinfuegen(d, 0, vorlage, x), "Vorlage einfuegen");
        d2 = lies("ui/w.menu", anwenden(d.text, x), o);
        const bool ok = d2.menus.size() == 1 && d2.menus[0].ok && d2.menus[0].items.size() == 3 &&
                        d2.menus[0].items[2].window.name == "neu" && nah(d2.menus[0].items[2].window.rectClient.w, 3) &&
                        d2.meldungen.empty() && (typ < 0 || d2.menus[0].items[2].type == typ);
        pruefe(ok, "Vorlage Typ " + std::to_string(typ) + " liest sich ohne Meldung (SP)");
    }

    std::printf("menuewerkzeug: %s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
