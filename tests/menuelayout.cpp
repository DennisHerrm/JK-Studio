// menuelayout.cpp - Menue-Editor M5 Punkt 1: Layouts wie Qt (nebeneinander, untereinander,
// Raster, Teiler), Abstaende, Raender, Spacer, Ausrichtung, breitester Text aller Sprachen,
// versteckte Kinder, Kommentarzeile im .menu (Rundlauf, Datei ohne Layout byte-gleich).
// M5b: show/hide je Zustand (setitemrect in den umschaltenden Skripten).
#include "bhed/menue/layout.h"

#include "bhed/menue/skript.h"

#include <cmath>
#include <cstdio>
#include <map>
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
bool gleich(const Rechteck& a, float x, float y, float w, float h) {
    return std::fabs(a.x - x) < 0.01F && std::fabs(a.y - y) < 0.01F && std::fabs(a.w - w) < 0.01F && std::fabs(a.h - h) < 0.01F;
}
std::string zeige(const Rechteck& r) {
    char b[80];
    std::snprintf(b, sizeof(b), "%g %g %g %g", static_cast<double>(r.x), static_cast<double>(r.y), static_cast<double>(r.w),
                  static_cast<double>(r.h));
    return b;
}
}  // namespace

int main() {
    std::map<std::string, Rechteck> rects;
    rects["panel"] = Rechteck{100, 50, 440, 40};
    rects["a"] = Rechteck{0, 0, 50, 20};
    rects["b"] = Rechteck{0, 0, 60, 20};
    rects["c"] = Rechteck{0, 0, 70, 30};
    rects["lang"] = Rechteck{0, 0, 40, 20};
    std::map<std::string, float> texte;
    texte["lang"] = 90;
    std::map<std::string, bool> versteckt;
    LayoutGroessen g;
    g.rect = [&](const std::string& n, Rechteck& r) {
        const auto it = rects.find(n);
        if (it == rects.end()) { return false; }
        r = it->second;
        return true;
    };
    g.textBreite = [&](const std::string& n) {
        const auto it = texte.find(n);
        return it == texte.end() ? 0.0F : it->second;
    };
    g.sichtbar = [&](const std::string& n) { return !versteckt[n]; };

    // Zeile lesen/schreiben
    Layout l;
    pruefe(liesLayoutZeile("\t\t// jkstudio:layout name=leiste type=hbox frame=panel spacing=8 margins=4,5,6,7 halign=center valign=fill items=a,*,b,*2,c,@innen\r\n", l),
           "Layout-Zeile gelesen");
    pruefe(l.name == "leiste" && l.art == LayoutArt::Waagrecht && l.rahmenItem == "panel" && l.abstand == 8 && l.rand[0] == 4 && l.rand[3] == 7 &&
               l.h == LayoutAusrichtung::Mitte && l.v == LayoutAusrichtung::Fuellen && l.kinder.size() == 6 && l.kinder[1].spacer &&
               l.kinder[3].dehnung == 2 && l.kinder[5].layout && l.kinder[5].name == "innen",
           "Layout-Zeile: alle Angaben");
    Layout l2;
    pruefe(liesLayoutZeile(layoutZeile(l), l2) && layoutZeile(l2) == layoutZeile(l), "Layout-Zeile: Rundlauf");
    Layout sonder;
    sonder.name = "mit leer";
    sonder.kinder.push_back(LayoutKind{"knopf, 1", false, false, 1});
    pruefe(liesLayoutZeile(layoutZeile(sonder), l2) && l2.name == "mit leer" && l2.kinder.size() == 1 && l2.kinder[0].name == "knopf, 1",
           "Layout-Zeile: Namen mit Leerzeichen und Komma (%XX)");
    pruefe(!liesLayoutZeile("// nur ein Kommentar", l2), "andere Kommentare sind keine Layouts");

    // nebeneinander: Rahmen 100,50,440,40, Raender 4 -> innen 104..536 (432), Abstand 8
    Layout h;
    h.name = "h";
    h.rahmenItem = "panel";
    h.abstand = 8;
    h.rand[0] = h.rand[1] = h.rand[2] = h.rand[3] = 4;
    h.kinder = {{"a", false, false, 1}, {"b", false, false, 1}, {"c", false, false, 1}};
    LayoutErgebnis e = rechneLayouts({h}, g);
    pruefe(gleich(e.rects["a"], 104, 54, 50, 20) && gleich(e.rects["b"], 162, 54, 60, 20) && gleich(e.rects["c"], 230, 54, 70, 30),
           "hbox: links, Abstand 8, Rand 4 (" + zeige(e.rects["b"]) + ")");
    h.h = LayoutAusrichtung::Mitte;
    h.v = LayoutAusrichtung::Mitte;
    e = rechneLayouts({h}, g);
    // frei = 432 - 180 - 16 = 236 -> Start 104 + 118; quer: (32 - 20) / 2 = 6
    pruefe(gleich(e.rects["a"], 222, 60, 50, 20) && gleich(e.rects["c"], 348, 55, 70, 30), "hbox: mittig in beiden Achsen (" + zeige(e.rects["a"]) + ")");
    h.h = LayoutAusrichtung::Fuellen;
    h.v = LayoutAusrichtung::Fuellen;
    e = rechneLayouts({h}, g);
    // frei 236 / 3 = 78,67 je Kind
    pruefe(gleich(e.rects["a"], 104, 54, 129, 32) && e.rects["c"].x + e.rects["c"].w == 536, "hbox: fuellen bis an den Rand (" + zeige(e.rects["a"]) + ")");
    // Spacer: der ganze freie Platz dorthin (1:2)
    h.h = LayoutAusrichtung::Anfang;
    h.v = LayoutAusrichtung::Anfang;
    h.kinder = {{"a", false, false, 1}, {"", true, false, 1}, {"b", false, false, 1}, {"", true, false, 2}, {"c", false, false, 1}};
    e = rechneLayouts({h}, g);
    // frei = 432 - 180 - 4*8 = 220 -> 73,33 und 146,67
    pruefe(gleich(e.rects["a"], 104, 54, 50, 20) && gleich(e.rects["b"], 243, 54, 60, 20) && e.rects["c"].x + e.rects["c"].w == 536,
           "hbox: Spacer 1:2 nehmen den freien Platz (" + zeige(e.rects["b"]) + ")");

    // untereinander mit fester Flaeche (rect), quer fuellen
    Layout v;
    v.name = "v";
    v.art = LayoutArt::Senkrecht;
    v.rahmen = Rechteck{10, 10, 200, 300};
    v.abstand = 10;
    v.h = LayoutAusrichtung::Fuellen;
    v.kinder = {{"a", false, false, 1}, {"b", false, false, 1}, {"c", false, false, 1}};
    e = rechneLayouts({v}, g);
    pruefe(gleich(e.rects["a"], 10, 10, 200, 20) && gleich(e.rects["b"], 10, 40, 200, 20) && gleich(e.rects["c"], 10, 70, 200, 30),
           "vbox: untereinander, Breite gefuellt (" + zeige(e.rects["c"]) + ")");

    // breitester Text aller Sprachen: nebeneinander waechst das Item, im Raster gibt es eine Warnung
    Layout t = h;
    t.kinder = {{"lang", false, false, 1}};
    e = rechneLayouts({t}, g);
    pruefe(gleich(e.rects["lang"], 104, 54, 90, 20) && e.warnungen.empty(), "hbox: Breite = laengster Text aller Sprachen (90 statt 40)");
    Layout r;
    r.name = "r";
    r.art = LayoutArt::Raster;
    r.spalten = 2;
    r.rahmen = Rechteck{0, 0, 100, 100};
    r.abstand = 10;
    r.h = r.v = LayoutAusrichtung::Fuellen;
    r.kinder = {{"a", false, false, 1}, {"", true, false, 1}, {"b", false, false, 1}, {"lang", false, false, 1}};
    e = rechneLayouts({r}, g);
    pruefe(gleich(e.rects["a"], 0, 0, 45, 45) && gleich(e.rects["b"], 0, 55, 45, 45) && gleich(e.rects["lang"], 55, 55, 45, 45),
           "Raster 2 Spalten, Spacer = leere Zelle (" + zeige(e.rects["lang"]) + ")");
    pruefe(e.warnungen.size() == 1 && e.warnungen[0] == "lang", "Raster: Warnung, wenn der laengste Text nicht in die Zelle passt");

    // Teiler: zwei Spalten mit 6 Punkten Teiler, links 30 %
    Layout s;
    s.name = "s";
    s.art = LayoutArt::Teiler;
    s.rahmen = Rechteck{0, 0, 406, 200};
    s.teilung = 0.3F;
    s.kinder = {{"a", false, false, 1}, {"innen", false, true, 1}};
    Layout innen;
    innen.name = "innen";
    innen.art = LayoutArt::Senkrecht;
    innen.abstand = 4;
    innen.kinder = {{"b", false, false, 1}, {"c", false, false, 1}};
    e = rechneLayouts({innen, s}, g);
    pruefe(gleich(e.rects["a"], 0, 0, 120, 200) && gleich(e.flaechen["innen"], 126, 0, 280, 200), "Teiler: 30 % links, Teiler 6, Rest rechts");
    pruefe(gleich(e.rects["b"], 126, 0, 60, 20) && gleich(e.rects["c"], 126, 24, 70, 30), "verschachtelt: inneres Layout in der rechten Spalte");

    // versteckte Kinder: hidden=skip laesst sie aus (rect bleibt), sonst halten sie ihren Platz
    versteckt["b"] = true;
    Layout hv = v;
    hv.h = LayoutAusrichtung::Anfang;
    e = rechneLayouts({hv}, g);
    pruefe(gleich(e.rects["c"], 10, 70, 70, 30), "versteckt, hidden=keep: Platz bleibt frei");
    hv.versteckteAuslassen = true;
    e = rechneLayouts({hv}, g);
    pruefe(gleich(e.rects["c"], 10, 40, 70, 30) && e.rects.count("b") == 0, "versteckt, hidden=skip: rueckt auf");
    versteckt.clear();
    Layout fehlt = v;
    fehlt.kinder.push_back({"gibtsnicht", false, false, 1});
    e = rechneLayouts({fehlt}, g);
    pruefe(e.fehlend.size() == 1 && e.fehlend[0] == "gibtsnicht", "fehlendes Kind gemeldet");

    // im .menu: Zeile einfuegen, lesen, ersetzen, loeschen; ohne Layout byte-gleich
    LeseOptionen o;
    o.dialekt = Dialekt::SP;
    const std::string text =
        "{\r\n\tmenuDef\r\n\t{\r\n\t\tname m\r\n\t\trect 0 0 640 480\r\n"
        "\t\titemDef\r\n\t\t{\r\n\t\t\tname panel\r\n\t\t\trect 100 50 440 40\r\n\t\t}\r\n"
        "\t\titemDef\r\n\t\t{\r\n\t\t\tname a\r\n\t\t\trect 0 0 50 20\r\n\t\t}\r\n"
        "\t\titemDef\r\n\t\t{\r\n\t\t\tname b\r\n\t\t\trect 0 0 60 20\r\n\t\t}\r\n\t}\r\n}\r\n";
    const Datei d = lies("ui/l.menu", text, o);
    pruefe(schreib(d) == text && liesLayouts(d, 0).empty(), "ohne Layout: Datei unveraendert");
    Layout neu;
    neu.name = "leiste";
    neu.rahmenItem = "panel";
    neu.abstand = 8;
    neu.kinder = {{"a", false, false, 1}, {"b", false, false, 1}};
    std::vector<Ersetzung> x;
    pruefe(layoutSchreiben(d, 0, neu, false, x), "Layout-Zeile eingefuegt");
    const std::string mit = anwenden(text, x);
    pruefe(mit.find("\t\t// jkstudio:layout name=leiste type=hbox frame=panel spacing=8 items=a,b\r\n\t\titemDef") != std::string::npos,
           "Zeile vor dem ersten itemDef, gleiche Einrueckung und CRLF");
    const Datei d2 = lies("ui/l.menu", mit, o);
    pruefe(d2.meldungen.empty() && d2.menus.size() == 1 && d2.menus[0].items.size() == 3, "das Spiel ueberliest die Zeile (keine Meldung)");
    const auto ls = liesLayouts(d2, 0);
    pruefe(ls.size() == 1 && ls[0].name == "leiste" && ls[0].kinder.size() == 2 && layoutVon(ls, "b") == 0 && layoutVon(ls, "panel") == 0 &&
               layoutVon(ls, "x") == -1,
           "Layout wieder gelesen");
    // rects anwenden (wie die Flaeche: setzeRect je Kind)
    LayoutGroessen gd;
    gd.rect = [&](const std::string& n, Rechteck& rr) {
        for (const Item& it : d2.menus[0].items) {
            if (it.window.name == n) {
                rr = it.window.rectClient;
                return true;
            }
        }
        return false;
    };
    e = rechneLayouts(ls, gd);
    x.clear();
    for (std::size_t i = 0; i < d2.menus[0].items.size(); ++i) {
        const auto it = e.rects.find(d2.menus[0].items[i].window.name);
        if (it != e.rects.end()) { pruefe(setzeRect(d2, 0, static_cast<int>(i), it->second, x), "setzeRect"); }
    }
    const Datei d3 = lies("ui/l.menu", anwenden(mit, x), o);
    pruefe(gleich(d3.menus[0].items[1].window.rectClient, 100, 50, 50, 20) && gleich(d3.menus[0].items[2].window.rectClient, 158, 50, 60, 20),
           "rects im Text nach dem Layout");
    neu.abstand = 12;
    x.clear();
    pruefe(layoutSchreiben(d2, 0, neu, false, x) && x.size() == 1, "Layout-Zeile ersetzt");
    const std::string ersetzt = anwenden(mit, x);
    pruefe(ersetzt.find("spacing=12") != std::string::npos && ersetzt.find("spacing=8") == std::string::npos, "nur die eine Zeile neu");
    x.clear();
    pruefe(layoutSchreiben(d2, 0, neu, true, x), "Layout-Zeile geloescht");
    pruefe(anwenden(mit, x) == text, "nach dem Loeschen wieder byte-gleich");
    Layout zwei;
    zwei.name = freierLayoutName(ls);
    pruefe(zwei.name == "layout1", "freier Layoutname");

    // M5b: show/hide je Zustand -> setitemrect in den Skripten, die Layout-Kinder umschalten
    for (const Dialekt dl : {Dialekt::SP, Dialekt::MP}) {
        const std::string dn = dl == Dialekt::MP ? "MP: " : "SP: ";
        LeseOptionen oz;
        oz.dialekt = dl;
        // type als Zahl (MP) bzw. als Name (SP: ItemParse_type ueber die Namenstabelle)
        const std::string ty = dl == Dialekt::MP ? "1" : "ITEM_TYPE_BUTTON";
        const std::string tz =
            "{\n menuDef\n {\n  name z\n  rect 20 10 600 460\n  visible 1\n  onOpen { hide b }\n"
            "  // jkstudio:layout name=leiste type=hbox frame=panel spacing=8 hidden=skip items=a,b,c\n"
            "  itemDef { name panel rect 100 50 440 40 visible 1 }\n"
            "  itemDef { name a rect 100 50 50 20 visible 1 }\n"
            "  itemDef { name b rect 0 0 60 20 visible 1 }\n"
            "  itemDef { name c rect 158 50 70 20 visible 1 }\n"
            "  itemDef { name zeig type " + ty + " rect 10 400 50 20 visible 1 action { show b } }\n"
            "  itemDef { name weg type " + ty + " rect 70 400 50 20 visible 1\n   action\n   {\n    play \"sound/x.wav\" ;\n    hide b\n   }\n  }\n"
            "  itemDef { name anders type " + ty + " rect 130 400 50 20 visible 1 action { setcvar x 1 } }\n"
            " }\n}\n";
        const Datei dz = lies("ui/z.menu", tz, oz);
        pruefe(dz.meldungen.empty() && dz.menus.size() == 1 && dz.menus[0].items.size() == 7, dn + "Zustands-Menue gelesen");
        const auto lz = liesLayouts(dz, 0);
        auto groessen = [](const Datei& dd) {
            LayoutGroessen gg;
            gg.rect = [&dd](const std::string& n, Rechteck& rr) {
                for (const Item& it : dd.menus[0].items) {
                    if (it.window.name == n) {
                        rr = it.window.rectClient;
                        return true;
                    }
                }
                return false;
            };
            return gg;
        };
        Laufzeit offen;
        offen.start(dz);
        offen.oeffne("z");
        const LayoutZustaende zs = layoutZustaende(dz, 0, lz, groessen(dz), offen, true);
        pruefe(zs.lageWechselt && zs.skripte.size() == 3, dn + "drei Skripte schalten Layout-Kinder um (onOpen, zeig, weg), setcvar nicht");
        // Script_SetItemRect: SP setzt window.rect (Menue 20/10 dazu), MP addiert das Menue selbst
        const float ox = dl == Dialekt::SP ? 20.0F : 0.0F, oy = dl == Dialekt::SP ? 10.0F : 0.0F;
        auto befehl = [&](const char* n, float bx, float by, float bw, float bh) {
            return std::string("setitemrect ") + n + " " + zahlText(bx + ox) + " " + zahlText(by + oy) + " " + zahlText(bw) + " " + zahlText(bh);
        };
        for (const LayoutZustandSkript& sk : zs.skripte) {
            if (sk.item == 4) {
                pruefe(sk.befehle.size() == 2 && sk.befehle[0] == befehl("b", 158, 50, 60, 20) && sk.befehle[1] == befehl("c", 226, 50, 70, 20),
                       dn + "show b: b und c bekommen die Lage mit b (a bleibt, keine Zeile)");
            } else {
                pruefe(sk.befehle.size() == 1 && sk.befehle[0] == befehl("c", 158, 50, 70, 20), dn + "hide b / onOpen: c zurueck in den Grundzustand");
            }
        }
        const std::string tn = anwenden(tz, zs.ersetzungen);
        pruefe(tn.find("action { show b ; " + befehl("b", 158, 50, 60, 20) + " ; " + befehl("c", 226, 50, 70, 20) + " ; }") != std::string::npos,
               dn + "einzeiliges Skript: Befehle angehaengt");
        pruefe(tn.find("    hide b ;\n    " + befehl("c", 158, 50, 70, 20) + " ;\n   }") != std::string::npos,
               dn + "mehrzeiliges Skript: eigene Zeile mit der Einrueckung des letzten Befehls");
        const Datei dn2 = lies("ui/z.menu", tn, oz);
        pruefe(dn2.meldungen.empty(), dn + "das Spiel liest die Skripte ohne Meldung");
        // die Laufzeit (wie Item_RunScript) legt die Kinder nach dem Klick wie das Layout (Bildschirm = rect + Menue 20/10)
        Laufzeit lk;
        lk.start(dn2);
        lk.oeffne("z");
        lk.fuehreAus(0, 4, dn2.menus[0].items[4].action);
        pruefe(gleich(lk.rects[0][2], 178, 60, 60, 20) && gleich(lk.rects[0][3], 246, 60, 70, 20), dn + "nach show b: b und c wie im Layout");
        lk.fuehreAus(0, 5, dn2.menus[0].items[5].action);
        pruefe(gleich(lk.rects[0][3], 178, 60, 70, 20), dn + "nach hide b: c wieder neben a");
        // Rundlauf: noch einmal gerechnet aendert nichts (alte setitemrect werden ersetzt, nicht verdoppelt)
        Laufzeit offen2;
        offen2.start(dn2);
        offen2.oeffne("z");
        const auto zs2 = layoutZustaende(dn2, 0, liesLayouts(dn2, 0), groessen(dn2), offen2, true);
        pruefe(zs2.ersetzungen.empty() && zs2.skripte.size() == 3, dn + "zweites Rechnen: Skripte bleiben gleich");
        // Dialekt ohne setitemrect: nichts schreiben, aber melden
        const auto zs3 = layoutZustaende(dz, 0, lz, groessen(dz), offen, false);
        pruefe(zs3.ersetzungen.empty() && zs3.lageWechselt, dn + "Dialekt ohne setitemrect: keine Ersetzung, Lagewechsel gemeldet");
        // Layout aufgeloest: die erzeugten setitemrect fallen aus den Skripten, der Rest bleibt
        const auto zs4 = layoutZustaende(dn2, 0, {}, groessen(dn2), offen2, true, {"a", "b", "c"});
        const std::string ohne = anwenden(tn, zs4.ersetzungen);
        pruefe(ohne.find("setitemrect") == std::string::npos && ohne.find("action { show b ; }") != std::string::npos &&
                   ohne.find("    hide b ;\n   }") != std::string::npos,
               dn + "Layout aufgeloest: setitemrect entfernt, Skripte sonst wie vorher (ein ; bleibt)");
    }

    std::printf("menuelayout: %s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
