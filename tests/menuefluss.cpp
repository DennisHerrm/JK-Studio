// menuefluss.cpp - Menue-Editor M6 Punkt 1: Menuefluss (open/close wie Item_RunScript,
// Einstiege wie ui_main), Sackgassen, unerreichbare Menues, fehlende Zurueck-Wege, fehlende Ziele.
#include "bhed/menue/fluss.h"

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
const FlussMenue* finde(const Fluss& f, const std::string& n) {
    for (const FlussMenue& m : f.menues) {
        if (m.name == n) { return &m; }
    }
    return nullptr;
}
std::string menue(const std::string& name, const std::string& rest) {
    return "\tmenuDef\n\t{\n\t\tname " + name + "\n\t\trect 0 0 640 480\n\t\tvisible 1\n" + rest + "\t}\n";
}
std::string knopf(const std::string& name, const std::string& action) {
    return "\t\titemDef\n\t\t{\n\t\t\tname " + name + "\n\t\t\ttype ITEM_TYPE_BUTTON\n\t\t\trect 10 10 80 20\n\t\t\tvisible 1\n\t\t\taction { " +
           action + " }\n\t\t}\n";
}
}  // namespace

int main() {
    // Datei 1: Hauptmenue (Einstieg im SP), Optionen mit Zurueck, Credits ohne Zurueck,
    // Falle (Sackgasse), Waise (nie geoeffnet), Anzeige ohne Eingabe
    const std::string t1 = "{\n" + menue("mainMenu", knopf("opt", "play \"sound/interface/button1.wav\" ; open optionen") +
                                                         knopf("cred", "open credits") + knopf("falle", "open falle") +
                                                         knopf("weg", "open gibtsnicht")) +
                           menue("optionen", knopf("zurueck", "close optionen") + knopf("mehr", "open OPTIONEN2")) +
                           menue("credits", knopf("weiter", "open falle")) + menue("falle", knopf("nichts", "play \"sound/interface/button1.wav\"")) +
                           menue("waise", knopf("zu", "close waise")) +
                           menue("anzeige", "\t\titemDef\n\t\t{\n\t\t\tname t\n\t\t\ttext \"x\"\n\t\t\trect 0 0 10 10\n\t\t\tdecoration\n\t\t\tvisible 1\n\t\t}\n") +
                           "}\n";
    // Datei 2 (gleicher Menuesatz): optionen2 geht ueber onESC zurueck (open optionen)
    const std::string t2 = "{\n" + menue("optionen2", "\t\tonESC { open optionen }\n" + knopf("a", "setcvar x 1")) + "}\n";
    LeseOptionen o;
    o.dialekt = Dialekt::SP;
    o.spiel = Spiel::JA;
    const Datei d1 = lies("ui/eins.menu", t1, o);
    const Datei d2 = lies("ui/zwei.menu", t2, o);
    pruefe(d1.menus.size() == 6 && d2.menus.size() == 1, "beide Dateien gelesen");
    const Fluss f = menueFluss({&d1, &d2});
    const FlussMenue* mm = finde(f, "mainMenu");
    const FlussMenue* op = finde(f, "optionen");
    const FlussMenue* op2 = finde(f, "optionen2");
    const FlussMenue* cr = finde(f, "credits");
    const FlussMenue* fa = finde(f, "falle");
    const FlussMenue* wa = finde(f, "waise");
    const FlussMenue* an = finde(f, "anzeige");
    const FlussMenue* gn = finde(f, "gibtsnicht");
    pruefe(mm && op && op2 && cr && fa && wa && an && gn, "alle Knoten da (auch das fehlende Ziel)");
    if (g_fehler != 0) { return 1; }
    pruefe(mm->einstieg && mm->ebene == 0 && mm->erreichbar, "mainMenu ist Einstieg (UI_MainMenu)");
    pruefe(op->ebene == 1 && op2->ebene == 2 && op2->datei == 1, "open ohne Gross/Klein, ueber Dateien hinweg (Menus_FindByName)");
    pruefe(!op->ohneZurueck && !op->sackgasse, "optionen: close <selbst> ist der Weg zurueck");
    pruefe(!op2->ohneZurueck, "optionen2: onESC oeffnet die Eltern - zurueck");
    pruefe(cr->ohneZurueck && !cr->sackgasse, "credits: Ausgang (open falle), aber kein Weg zurueck");
    pruefe(fa->sackgasse, "falle: Knopf ohne open/close - Sackgasse");
    pruefe(!wa->erreichbar && wa->ebene == -1 && !wa->sackgasse, "waise: nie geoeffnet - unerreichbar, aber mit Ausgang");
    pruefe(!an->eingabe && !an->sackgasse, "anzeige: ohne Eingabe keine Sackgasse");
    pruefe(gn->datei == -1 && gn->erreichbar, "gibtsnicht: fehlendes Ziel als eigener Knoten");
    int kantenMain = 0;
    for (const FlussKante& k : f.kanten) {
        if (f.menues[static_cast<std::size_t>(k.von)].name == "mainMenu") {
            ++kantenMain;
            pruefe(k.item >= 0 && k.wo.find(": action") != std::string::npos, "Kante kennt Item und Ereignis");
        }
    }
    pruefe(kantenMain == 4, "vier open im Hauptmenue");
    // MP: Einstieg main, nicht mainMenu
    LeseOptionen mp;
    mp.dialekt = Dialekt::MP;
    std::string t3 = "{\n" + menue("main", knopf("q", "open quit")) + menue("quit", knopf("n", "close quit")) + "}\n";
    for (std::size_t p = t3.find("ITEM_TYPE_BUTTON"); p != std::string::npos; p = t3.find("ITEM_TYPE_BUTTON")) { t3.replace(p, 16, "1"); }
    const Datei d3 = lies("ui/jamp/eins.menu", t3, mp);
    const Fluss g = menueFluss({&d3});
    pruefe(finde(g, "main") && finde(g, "main")->einstieg && finde(g, "quit") && finde(g, "quit")->ebene == 1, "MP: main ist Einstieg (UI_SetActiveMenu)");
    if (g_fehler == 0) { std::printf("menue_fluss: alles gruen\n"); }
    return g_fehler == 0 ? 0 : 1;
}
