// menuebauteile.cpp - Menue-Editor M2 Punkt 3 und 4: Detailbereich in drei
// Ebenen und Bauteil-Bibliothek.
//
//   - Der Detailbereich (MD-Einstellungsmenue als Beispiel) liest sich in SP
//     und MP ohne Meldung der Engine, bleibt unter MAX_MENUITEMS und laesst
//     sich bedienen: onOpen oeffnet Reiter 0/Unterreiter 0, Klick auf einen
//     Unterreiter zeigt dessen Detail, die Pfeile blaettern die Reiter.
//   - Jede Standardvorlage liest sich in SP und MP ohne Meldung.
//   - Eigene Vorlagen speichern und wieder laden.
#include "bhed/menue/bauteile.h"
#include "bhed/menue/menue.h"
#include "bhed/menue/skript.h"
#include "bhed/menue/vorschau.h"

#include <cstddef>
#include <cstdio>
#include <filesystem>
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
int itemNr(const Menu& m, const std::string& name) {
    for (std::size_t i = 0; i < m.items.size(); ++i) {
        if (m.items[i].window.name == name) { return static_cast<int>(i); }
    }
    return -1;
}
}  // namespace

int main() {
    const DetailOptionen o = beispielDetail();
    for (const Dialekt dl : {Dialekt::SP, Dialekt::MP}) {
        const bool sp = dl == Dialekt::SP;
        LeseOptionen lo;
        lo.dialekt = dl;
        if (!sp) {   // MP: die Namen kommen aus menudef.h
            lo.globaleDefines = "ui/jamp/menudef.h";
            lo.lader = [](const std::string&, std::string& inhalt) {
                inhalt = "#define ITEM_TYPE_BUTTON 1\n#define ITEM_TYPE_SLIDER 10\n#define ITEM_TYPE_YESNO 11\n#define ITEM_TYPE_LISTBOX 6\n"
                         "#define WINDOW_STYLE_FILLED 1\n#define ITEM_ALIGN_CENTER 1\n#define ITEM_ALIGN_RIGHT 2\n";
                return true;
            };
        }
        const std::string text = "{\n\tmenuDef\n\t{\n\t\tname \"set\"\n\t\trect 0 0 640 480\n\t\tvisible 1\n\t\tonOpen { " + detailOnOpen(o) +
                                 " }\n" + erzeugeDetailbereich(o, dl) + "\t}\n}\n";
        const Datei d = lies(sp ? "ui/set.menu" : "ui/jamp/set.menu", text, lo);
        const std::string was = sp ? "SP" : "MP";
        pruefe(d.menus.size() == 1 && d.meldungen.empty(), was + ": Detailbereich ohne Meldung gelesen");
        if (!d.meldungen.empty()) { std::printf("      %s\n", meldungText(d, d.meldungen[0]).c_str()); }
        if (d.menus.empty()) { continue; }
        const Menu& m = d.menus[0];
        std::printf("menuebauteile %s: Detailbereich %zu Items fuer %zu Reiter\n", was.c_str(), m.items.size(), o.reiter.size());
        pruefe(m.items.size() < 256, was + ": unter MAX_MENUITEMS (256)");
        Laufzeit l;
        l.start(d);
        l.oeffne("set");
        pruefe(l.cvar("ui_tab") == "0" && l.cvar("ui_sel") == "0_0" && l.cvar("ui_tabofs") == "0", was + ": onOpen oeffnet Reiter 0");
        Umgebung u;
        l.fuelle(0, u);
        const int t00 = itemNr(m, "dt_text_0_0"), t01 = itemNr(m, "dt_text_0_1");
        pruefe(t00 >= 0 && t01 >= 0 && itemSichtbar(m.items[static_cast<std::size_t>(t00)], l.items[0][static_cast<std::size_t>(t00)].flags, u) &&
                   !itemSichtbar(m.items[static_cast<std::size_t>(t01)], l.items[0][static_cast<std::size_t>(t01)].flags, u),
               was + ": Detail 0_0 sichtbar, 0_1 nicht");
        // Klick auf Unterreiter 0_1
        const int u01 = itemNr(m, "dt_unter_0_1");
        if (u01 >= 0) {
            const Rechteck& r = m.items[static_cast<std::size_t>(u01)].window.rect;
            l.klick(0, r.x + 5, r.y + 5);
        }
        pruefe(l.cvar("ui_sel") == "0_1", was + ": Klick auf Unterreiter setzt ui_sel 0_1");
        // Reiter 2 klicken -> ui_tab 2, ui_sel 2_0
        const int r2 = itemNr(m, "dt_reiter_2_0");
        if (r2 >= 0) {
            const Rechteck& r = m.items[static_cast<std::size_t>(r2)].window.rect;
            l.klick(0, r.x + 5, r.y + 5);
        }
        pruefe(l.cvar("ui_tab") == "2" && l.cvar("ui_sel") == "2_0", was + ": Klick auf Reiter 3 setzt ui_tab 2 und ui_sel 2_0");
        // blaettern: Pfeil rechts bei Stellung 0
        const int re = itemNr(m, "dt_rechts_0");
        if (re >= 0) {
            const Rechteck& r = m.items[static_cast<std::size_t>(re)].window.rect;
            l.klick(0, r.x + 3, r.y + 5);
        }
        pruefe(l.cvar("ui_tabofs") == "1", was + ": Pfeil rechts blaettert die Reiter (ui_tabofs 1)");
        l.fuelle(0, u);
        const int r4 = itemNr(m, "dt_reiter_4_1");
        pruefe(r4 >= 0 && itemSichtbar(m.items[static_cast<std::size_t>(r4)], l.items[0][static_cast<std::size_t>(r4)].flags, u),
               was + ": nach dem Blaettern ist Reiter 5 sichtbar");
        // Vorlagen
        for (const Vorlage& v : standardVorlagen(dl)) {
            const Datei vd = lies(sp ? "ui/v.menu" : "ui/jamp/v.menu", "{ menuDef { name v rect 0 0 640 480\n" + v.text + " } }\n", lo);
            pruefe(vd.menus.size() == 1 && !vd.menus[0].items.empty() && vd.meldungen.empty(), was + ": Vorlage " + v.id + " ohne Meldung");
        }
    }
    // eigene Vorlagen
    const std::string ordner = (std::filesystem::temp_directory_path() / "bhed_menue_vorlagen_test").string();
    std::error_code ec;
    std::filesystem::remove_all(ordner, ec);
    pruefe(speichereVorlage(ordner, "Mein Knopf!", "itemDef { name x rect 0 0 10 10 }\n"), "eigene Vorlage gespeichert");
    const std::vector<Vorlage> ev = ladeEigeneVorlagen(ordner);
    pruefe(ev.size() == 1 && ev[0].eigen && ev[0].id == "Mein_Knopf_", "eigene Vorlage geladen (Dateiname bereinigt)");
    std::filesystem::remove_all(ordner, ec);

    std::printf("menuebauteile: %s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
