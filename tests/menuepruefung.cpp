// menuepruefung.cpp - Menue-Editor M2 Punkt 7: Pruefungen vor dem Export.
//
//   - Grenzen je Fassung: MAX_MENUITEMS (SP 150, MP 512, Movie Duels 4096),
//     Skriptlaenge (PC_Script_Parse SP 4096, MP 2048), MAX_MULTI_CVARS;
//   - nie sichtbare Items (ausserhalb 640x480, ohne Groesse, visible 0 ohne
//     ein Skript, das sie zeigt) - ein per show gezeigtes Item ist kein Befund;
//   - fehlende Dateien (Hintergrund, Modell, Video, Klang);
//   - Vertraeglichkeit: ein Movie-Duels-Typ ist in JA/JK2 unbekannt.
#include "bhed/menue/menue.h"
#include "bhed/menue/pruefung.h"

#include <cstdio>
#include <cstdlib>
#include <set>
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
int zaehle(const std::vector<Befund>& b, BefundArt art) {
    int n = 0;
    for (const Befund& x : b) { n += x.art == art ? 1 : 0; }
    return n;
}
bool hat(const std::vector<Befund>& b, BefundArt art, const std::string& name) {
    for (const Befund& x : b) {
        if (x.art == art && (x.name == name || x.wert == name)) { return true; }
    }
    return false;
}
Datei lese(const std::string& text, Dialekt d, Spiel s) {
    LeseOptionen o;
    o.dialekt = d;
    o.spiel = s;
    return lies(d == Dialekt::MP ? "ui/jamp/probe.menu" : "ui/probe.menu", text, o);
}
std::string viele(int n) {
    std::string t = "{\nmenuDef\n{\nname viele\nrect 0 0 640 480\n";
    for (int i = 0; i < n; ++i) { t += "itemDef { name i" + std::to_string(i) + " rect 10 10 20 20 style WINDOW_STYLE_FILLED visible 1 }\n"; }
    return t + "}\n}\n";
}
}  // namespace

int main() {
    // Grenzen: 151 Items sind fuer JA SP zu viel, fuer Movie Duels nicht
    {
        const Datei sp = lese(viele(151), Dialekt::SP, Spiel::JA);
        const std::vector<Befund> b = pruefeFuerSpiel(sp, PruefKontext{});
        pruefe(zaehle(b, BefundArt::ZuVieleItems) == 1, "Grenze: 151 Items in JA SP (MAX_MENUITEMS 150)");
        const Datei md = lese(viele(151), Dialekt::SP, Spiel::MD);
        pruefe(zaehle(pruefeFuerSpiel(md, PruefKontext{}), BefundArt::ZuVieleItems) == 0, "Grenze: 151 Items in Movie Duels (4096)");
        const Grenzen g = grenzenFuer(Dialekt::MP, Spiel::JA);
        pruefe(g.menuItems == 512 && g.skript == 2048 && g.menus == 128, "Grenzen MP: 512 Items, 2048 Zeichen Skript, 128 Menues");
    }
    // Skriptlaenge: ein action-Skript ueber 2048 Zeichen ist in MP zu lang, in SP nicht
    {
        std::string act;
        for (int i = 0; i < 100; ++i) { act += "setcvar ui_probe_" + std::to_string(i) + " 1 ; "; }
        const std::string t = "{\nmenuDef\n{\nname s\nrect 0 0 640 480\nitemDef { name k rect 10 10 50 20 visible 1 action { " + act + "} }\n}\n}\n";
        const Datei mp = lese(t, Dialekt::MP, Spiel::JA);
        const Datei sp = lese(t, Dialekt::SP, Spiel::JA);
        pruefe(zaehle(pruefeFuerSpiel(mp, PruefKontext{}), BefundArt::SkriptZuLang) == 1, "Grenze: Skript ueber 2048 Zeichen in MP");
        pruefe(zaehle(pruefeFuerSpiel(sp, PruefKontext{}), BefundArt::SkriptZuLang) == 0, "Grenze: dasselbe Skript in SP (4096)");
        // M6 Punkt 10: Live-Auslastung mit derselben Laenge wie die Pruefung
        const Auslastung a = auslastung(mp, 0);
        pruefe(a.items == 1 && a.menus == 1 && a.skript > 2048 && a.skriptWo == "k: action" && a.g.skript == 2048, "Auslastung: laengstes Skript k: action");
        pruefe(auslastung(lese(viele(151), Dialekt::SP, Spiel::JA), 0).items == 151 && auslastung(sp, 0).g.menuItems == 150, "Auslastung: Items gegen MAX_MENUITEMS");
    }
    // nie sichtbar
    {
        const std::string t = "{\nmenuDef\n{\nname n\nrect 0 0 640 480\n"
                              "itemDef { name draussen rect 700 10 50 20 style WINDOW_STYLE_FILLED visible 1 }\n"
                              "itemDef { name versteckt rect 10 10 50 20 style WINDOW_STYLE_FILLED visible 0 }\n"
                              "itemDef { name gezeigt rect 10 40 50 20 style WINDOW_STYLE_FILLED visible 0 }\n"
                              "itemDef { name knopf rect 10 70 50 20 style WINDOW_STYLE_FILLED visible 1 action { show gezeigt } }\n"
                              "itemDef { name leer rect 10 100 50 20 visible 0 }\n"
                              "}\n}\n";
        const Datei dn = lese(t, Dialekt::SP, Spiel::JA);
        const std::vector<Befund> b = pruefeFuerSpiel(dn, PruefKontext{});
        if (std::getenv("MENUEPRUEFUNG_ZEIGE") != nullptr) {
            for (const Befund& x : b) { std::printf("  %d %d/%d %s %s\n", static_cast<int>(x.art), x.menu, x.item, x.name.c_str(), x.wert.c_str()); }
            for (const Item& it : dn.menus[0].items) {
                std::printf("  %s style %d flags %x rect %g %g %g %g\n", it.window.name.c_str(), it.window.style, static_cast<unsigned>(it.window.flags),
                            static_cast<double>(it.window.rect.x), static_cast<double>(it.window.rect.y), static_cast<double>(it.window.rect.w),
                            static_cast<double>(it.window.rect.h));
            }
            for (const Meldung& m : dn.meldungen) { std::printf("  meldung %d %s\n", m.zeile, m.text.c_str()); }
        }
        pruefe(hat(b, BefundArt::NieSichtbar, "draussen"), "Nie sichtbar: ausserhalb 640x480");
        pruefe(hat(b, BefundArt::NieSichtbar, "versteckt"), "Nie sichtbar: visible 0 ohne Skript");
        pruefe(!hat(b, BefundArt::NieSichtbar, "gezeigt"), "Nie sichtbar: per show gezeigt - kein Befund");
        pruefe(!hat(b, BefundArt::NieSichtbar, "leer"), "Nie sichtbar: zeichnet nichts - kein Befund");
    }
    // M4: cinematic im Einzelspieler zeichnet die Engine nicht
    {
        const std::string t = "{\nmenuDef\n{\nname c\nrect 0 0 640 480\n"
                              "itemDef { name kino rect 0 0 320 240 style WINDOW_STYLE_CINEMATIC cinematic \"x\" visible 1 }\n}\n}\n";
        const std::vector<Befund> sp = pruefeFuerSpiel(lese(t, Dialekt::SP, Spiel::JA), PruefKontext{});
        const std::vector<Befund> mp = pruefeFuerSpiel(lese(t, Dialekt::MP, Spiel::JA), PruefKontext{});
        const auto hatK = [](const std::vector<Befund>& b) {
            for (const Befund& x : b) {
                if (x.art == BefundArt::CinematicSp) { return true; }
            }
            return false;
        };
        pruefe(hatK(sp) && !hatK(mp), "cinematic: Hinweis nur im Einzelspieler (Window_Paint SP zeichnet es nicht)");
    }
    // fehlende Dateien und Vertraeglichkeit
    {
        const std::string t = "{\nmenuDef\n{\nname f\nrect 0 0 640 480\n"
                              "itemDef { name bild rect 10 10 50 20 style WINDOW_STYLE_SHADER background \"gfx/menus/gibtsnicht\" visible 1 }\n"
                              "itemDef { name da rect 10 40 50 20 style WINDOW_STYLE_SHADER background \"gfx/menus/da\" visible 1 }\n"
                              "itemDef { name film rect 10 70 50 20 style WINDOW_STYLE_FILLED cinematic \"intro\" visible 1 }\n"
                              "itemDef { name klang rect 10 100 50 20 style WINDOW_STYLE_FILLED visible 1 focusSound \"sound/fehlt.wav\" }\n"
                              "itemDef { name schieber type ITEM_TYPE_SLIDER_INTEGER rect 10 130 50 20 style WINDOW_STYLE_FILLED visible 1 }\n"
                              "}\n}\n";
        PruefKontext k;
        const std::set<std::string> dateien = {"video/intro.roq"};
        k.gibt = [&](const std::string& p) { return dateien.count(p) != 0; };
        k.gibtBild = [](const std::string& n) { return n == "gfx/menus/da"; };
        const Datei md = lese(t, Dialekt::SP, Spiel::MD);
        const std::vector<Befund> b = pruefeFuerSpiel(md, k);
        pruefe(hat(b, BefundArt::DateiFehlt, "gfx/menus/gibtsnicht") && !hat(b, BefundArt::DateiFehlt, "gfx/menus/da"), "Datei fehlt: Hintergrund");
        pruefe(!hat(b, BefundArt::DateiFehlt, "video/intro.roq"), "Datei da: cinematic intro -> video/intro.roq");
        pruefe(hat(b, BefundArt::DateiFehlt, "sound/fehlt.wav"), "Datei fehlt: focusSound");
        bool ja = false, mdSelbst = false;
        for (const Befund& x : b) {
            if (x.art == BefundArt::UnbekanntIn && x.name == "ITEM_TYPE_SLIDER_INTEGER") {
                ja = ja || x.wert == "JA SP";
                mdSelbst = mdSelbst || x.wert == "MD SP";
            }
        }
        pruefe(ja && !mdSelbst, "Vertraeglichkeit: Movie-Duels-Typ 15 in JA SP unbekannt, in MD nicht");
    }
    std::printf("menuepruefung: %s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
