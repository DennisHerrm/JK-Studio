// menueseiten.cpp - Menue-Editor M6 Punkt 5: Vorlagen ganzer Menues. Jede Vorlage liest jede
// Fassung (JA/JK2/MD SP, JA MP mit den Defines aus menudef.h) ohne Meldung; zusammen ergeben sie
// einen Fluss ohne Sackgasse und ohne fehlenden Weg zurueck; die Einstellungen haben drei Reiter.
#include "bhed/menue/assistent.h"
#include "bhed/menue/fluss.h"
#include "bhed/menue/seitenvorlagen.h"
#include "bhed/menue/skript.h"

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
// die Werte aus ui/jamp/menudef.h (gleich in jk2mp), die die Vorlagen nutzen
const char* const kDefines =
    "#define ITEM_TYPE_BUTTON 1\n#define ITEM_TYPE_SLIDER 10\n#define ITEM_TYPE_YESNO 11\n#define WINDOW_STYLE_EMPTY 0\n"
    "#define WINDOW_STYLE_FILLED 1\n#define ITEM_ALIGN_LEFT 0\n#define ITEM_ALIGN_CENTER 1\n#define ITEM_ALIGN_RIGHT 2\n";
}  // namespace

int main() {
    const SeitenVorlage arten[] = {SeitenVorlage::Hauptmenue, SeitenVorlage::Einstellungen, SeitenVorlage::Credits, SeitenVorlage::Ladebildschirm};
    struct Fassung {
        Dialekt d;
        Spiel s;
        const char* name;
    };
    const Fassung fassungen[] = {{Dialekt::SP, Spiel::JA, "JA SP"}, {Dialekt::SP, Spiel::JK2, "JK2 SP"}, {Dialekt::SP, Spiel::MD, "MD SP"},
                                 {Dialekt::MP, Spiel::JA, "JA MP"}, {Dialekt::MP, Spiel::JK2, "JK2 MP"}};
    for (const Fassung& f : fassungen) {
        std::string alle = "{\n";
        for (const SeitenVorlage a : arten) { alle += seitenVorlage(a, f.d, seitenVorlageName(a, f.d), "\t", "\n"); }
        alle += "}\n";
        LeseOptionen o;
        o.dialekt = f.d;
        o.spiel = f.s;
        if (f.d == Dialekt::MP) {
            o.globaleDefines = "ui/menudef.h";
            o.lader = [](const std::string& p, std::string& t) {
                if (p != "ui/menudef.h") { return false; }
                t = kDefines;
                return true;
            };
        }
        const Datei d = lies(f.d == Dialekt::MP ? "ui/jamp/vorlagen.menu" : "ui/vorlagen.menu", alle, o);
        for (const Meldung& m : d.meldungen) { std::printf("  %s Zeile %d: %s\n", f.name, m.zeile, m.text.c_str()); }
        pruefe(d.meldungen.empty() && d.menus.size() == 4, std::string(f.name) + ": vier Vorlagen ohne Meldung gelesen");
        if (d.menus.size() != 4) { continue; }
        const Fluss fl = menueFluss({&d});
        int probleme = 0, einstiege = 0;
        for (const FlussMenue& n : fl.menues) {
            probleme += (n.datei < 0 || !n.erreichbar || n.sackgasse || n.ohneZurueck) ? 1 : 0;
            einstiege += n.einstieg ? 1 : 0;
        }
        pruefe(probleme == 0 && einstiege == 2, std::string(f.name) + ": Fluss ohne Probleme, Hauptmenue und Ladebildschirm oeffnet die Engine");
        int reiter = 0;
        for (const Seite& s : seitenImMenue(d)) { reiter += (s.menu == 1 && !s.cvar.empty()) ? 1 : 0; }
        pruefe(reiter == 3, std::string(f.name) + ": Einstellungen mit drei Reitern (ui_jks_tab)");
        pruefe(d.menus[1].items.size() == 12 && d.menus[1].items[5].type == ITEM_TYPE_YESNO && d.menus[1].items[6].type == ITEM_TYPE_SLIDER &&
                   d.menus[1].items[6].maxVal == 3.0F,
               std::string(f.name) + ": Einstellungen mit Ja/Nein und Schieber (r_gamma 0.5 bis 3)");
    }
    // M6 Punkt 6: Assistent - Steuerungen aus einem "Original" lernen, Seite bauen, wieder lesen
    {
        const std::string orig =
            "{\n\tmenuDef\n\t{\n\t\tname setup\n\t\trect 0 0 640 480\n"
            "\t\titemDef { name a type ITEM_TYPE_SLIDER text \"Volume:\" cvarFloat \"s_volume\" 0.8 0 1 rect 0 0 10 10 visible 1 }\n"
            "\t\titemDef { name b type ITEM_TYPE_YESNO text \"@MENUS_FULLSCREEN\" cvar \"r_fullscreen\" rect 0 0 10 10 visible 1 }\n"
            "\t\titemDef { name c type ITEM_TYPE_MULTI text \"Detail:\" cvar \"r_detail\" cvarFloatList { \"Low\" 0 \"High\" 1 } rect 0 0 10 10 visible 1 }\n"
            "\t\titemDef { name d type ITEM_TYPE_MULTI text \"Mode:\" cvar \"ui_mode\" cvarStrList { \"A b\" \"a\" \"B\" \"b\" } rect 0 0 10 10 visible 1 }\n"
            "\t\titemDef { name e type ITEM_TYPE_YESNO text \"Again:\" cvar \"R_FULLSCREEN\" rect 0 0 10 10 visible 1 }\n"
            "\t\titemDef { name f type ITEM_TYPE_LISTBOX cvar \"ui_liste\" feeder 1 rect 0 0 10 10 visible 1 }\n"
            "\t}\n}\n";
        LeseOptionen o;
        const Datei d = lies("ui/setup.menu", orig, o);
        for (const Meldung& m : d.meldungen) { std::printf("  Original Zeile %d: %s\n", m.zeile, m.text.c_str()); }
        const std::vector<CvarSteuerung> c = cvarSteuerungen({&d});
        pruefe(c.size() == 4, "Assistent: vier Steuerungen (Liste ohne, doppelte Cvar einmal)");
        const std::string seite = einstellungsSeite(c, "meineEinstellungen", "\t", "\n");
        const Datei e = lies("ui/neu.menu", "{\n" + seite + "}\n", o);
        for (const Meldung& m : e.meldungen) { std::printf("  Assistent Zeile %d: %s\n", m.zeile, m.text.c_str()); }
        pruefe(e.meldungen.empty() && e.menus.size() == 1 && e.menus[0].items.size() == 7, "Assistent: Seite ohne Meldung, 4 Steuerungen + Hintergrund, Titel, Zurueck");
        bool schieber = false, liste = false, zeichen = false, ja = false;
        for (const Item& it : e.menus[0].items) {
            schieber = schieber || (it.type == ITEM_TYPE_SLIDER && it.cvar == "s_volume" && it.minVal == 0.0F && it.maxVal == 1.0F && it.defVal == 0.8F);
            liste = liste || (it.type == ITEM_TYPE_MULTI && it.cvarList.size() == 2 && it.cvarValue.size() == 2 && it.cvarValue[1] == 1.0F);
            zeichen = zeichen || (it.type == ITEM_TYPE_MULTI && it.strDef && it.cvarList.size() == 2 && it.cvarList[0] == "A b" && it.cvarStr.size() == 2 &&
                                  it.cvarStr[1] == "b");
            ja = ja || (it.type == ITEM_TYPE_YESNO && it.text == "@MENUS_FULLSCREEN");
        }
        pruefe(schieber && liste && zeichen && ja, "Assistent: Bereiche, Listen und Beschriftung wie im Original");
        const Fluss fl = menueFluss({&e});
        pruefe(fl.menues.size() == 1 && !fl.menues[0].sackgasse, "Assistent: Seite hat einen Weg hinaus");
    }
    if (g_fehler == 0) { std::printf("menue_seiten: alles gruen\n"); }
    return g_fehler == 0 ? 0 : 1;
}
