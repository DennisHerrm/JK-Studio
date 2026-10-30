// seitenvorlagen.cpp - Menue-Editor M6 Punkt 5: Vorlagen ganzer Menues (siehe seitenvorlagen.h).
//
// Eigener Code.
#include "bhed/menue/seitenvorlagen.h"

#include <vector>

namespace bhed::menue {

namespace {

struct Bau {
    std::string einr, nl, s;
    void zeile(int tiefe, const std::string& t) { s += einr + std::string(static_cast<std::size_t>(tiefe), '\t') + t + nl; }
    void kv(int tiefe, const std::string& k, const std::string& v) { zeile(tiefe, v.empty() ? k : k + "\t\t" + v); }
    // itemDef mit festen Zeilen; rest = weitere Schluessel (je ein Eintrag)
    void item(const std::string& name, const std::vector<std::pair<std::string, std::string>>& rest) {
        zeile(1, "itemDef");
        zeile(1, "{");
        kv(2, "name", name);
        for (const auto& [k, v] : rest) { kv(2, k, v); }
        zeile(1, "}");
    }
};

std::string zit(const std::string& s) { return "\"" + s + "\""; }

void knopf(Bau& b, const std::string& name, const std::string& text, const std::string& rect, const std::string& action) {
    b.item(name, {{"type", "ITEM_TYPE_BUTTON"},
                  {"rect", rect},
                  {"style", "WINDOW_STYLE_FILLED"},
                  {"backcolor", "0.1 0.2 0.4 0.6"},
                  {"text", zit(text)},
                  {"textscale", "1"},
                  {"textalign", "ITEM_ALIGN_CENTER"},
                  {"textalignx", "90"},
                  {"textaligny", "2"},
                  {"forecolor", "0.9 0.9 1 1"},
                  {"visible", "1"},
                  {"mouseEnter", "{ setitemcolor " + name + " backcolor 0.2 0.4 0.8 0.8 }"},
                  {"mouseExit", "{ setitemcolor " + name + " backcolor 0.1 0.2 0.4 0.6 }"},
                  {"action", "{ " + action + " }"}});
}

void titel(Bau& b, const std::string& text) {
    b.item("titel", {{"rect", "0 30 640 60"},
                     {"text", zit(text)},
                     {"textscale", "1.6"},
                     {"textalign", "ITEM_ALIGN_CENTER"},
                     {"textalignx", "320"},
                     {"forecolor", "1 0.8 0.3 1"},
                     {"decoration", ""},
                     {"visible", "1"}});
}

void hintergrund(Bau& b) {
    b.item("hintergrund", {{"rect", "0 0 640 480"}, {"style", "WINDOW_STYLE_FILLED"}, {"backcolor", "0.02 0.03 0.08 1"}, {"decoration", ""}, {"visible", "1"}});
}

}  // namespace

std::string seitenVorlageName(SeitenVorlage art, Dialekt d) {
    switch (art) {
        case SeitenVorlage::Hauptmenue: return d == Dialekt::MP ? "main" : "mainMenu";
        case SeitenVorlage::Einstellungen: return "settingsMenu";
        case SeitenVorlage::Credits: return "creditsMenu";
        case SeitenVorlage::Ladebildschirm: return d == Dialekt::MP ? "Connect" : "loadscreen";
    }
    return "newMenu";
}

std::string seitenVorlage(SeitenVorlage art, Dialekt d, const std::string& name, const std::string& einr, const std::string& nl) {
    Bau b{einr, nl, {}};
    b.zeile(0, "menuDef");
    b.zeile(0, "{");
    b.kv(1, "name", zit(name));
    b.kv(1, "fullScreen", "1");
    b.kv(1, "rect", "0 0 640 480");
    b.kv(1, "visible", "1");
    b.kv(1, "focusColor", "1 1 1 1");
    switch (art) {
        case SeitenVorlage::Hauptmenue:
            hintergrund(b);
            titel(b, "MY MOD");
            knopf(b, "knopf_einstellungen", "Settings", "230 200 180 30", "open " + seitenVorlageName(SeitenVorlage::Einstellungen, d));
            knopf(b, "knopf_credits", "Credits", "230 245 180 30", "open " + seitenVorlageName(SeitenVorlage::Credits, d));
            knopf(b, "knopf_beenden", "Quit", "230 290 180 30", "exec \"quit\"");
            break;
        case SeitenVorlage::Einstellungen: {
            b.kv(1, "onOpen", "{ setcvar ui_jks_tab 1 }");
            b.kv(1, "onESC", "{ close " + name + " }");
            hintergrund(b);
            titel(b, "SETTINGS");
            const char* const reiter[] = {"Video", "Sound", "Controls"};
            for (int i = 0; i < 3; ++i) {
                const std::string n = std::to_string(i + 1);
                knopf(b, "reiter" + n, reiter[i], std::to_string(40 + i * 190) + " 100 180 30", "setcvar ui_jks_tab " + n);
            }
            struct Feld {
                int reiter;
                const char *name, *text, *art, *cvar;
            };
            const Feld felder[] = {{1, "vollbild", "Fullscreen:", "yesno", "r_fullscreen"},
                                   {1, "helligkeit", "Brightness:", "slider", "r_gamma"},
                                   {2, "lautstaerke", "Effects volume:", "slider", "s_volume"},
                                   {2, "musik", "Music volume:", "slider", "s_musicvolume"},
                                   {3, "rennen", "Always run:", "yesno", "cl_run"},
                                   {3, "maus", "Mouse speed:", "slider", "sensitivity"}};
            int zeile = 0, letzter = 0;
            for (const Feld& f : felder) {
                zeile = f.reiter == letzter ? zeile + 1 : 0;
                letzter = f.reiter;
                std::vector<std::pair<std::string, std::string>> k = {
                    {"type", std::string(f.art) == "yesno" ? "ITEM_TYPE_YESNO" : "ITEM_TYPE_SLIDER"},
                    {"rect", "120 " + std::to_string(170 + zeile * 40) + " 400 30"},
                    {"text", zit(f.text)},
                    {"textscale", "1"},
                    {"textalign", "ITEM_ALIGN_RIGHT"},
                    {"textalignx", "180"},
                    {"forecolor", "0.9 0.9 1 1"},
                    {"cvarTest", zit("ui_jks_tab")},
                    {"showCvar", "{ \"" + std::to_string(f.reiter) + "\" }"},
                    {"visible", "1"}};
                // Bereiche wie die Setup-Menues der Spiele (cvarFloat <cvar> <vorgabe> <min> <max>)
                if (std::string(f.cvar) == "r_gamma") { k.emplace_back("cvarFloat", "\"r_gamma\" 1 0.5 3"); }
                else if (std::string(f.cvar) == "s_volume") { k.emplace_back("cvarFloat", "\"s_volume\" 0.8 0 1"); }
                else if (std::string(f.cvar) == "s_musicvolume") { k.emplace_back("cvarFloat", "\"s_musicvolume\" 0.25 0 1"); }
                else if (std::string(f.cvar) == "sensitivity") { k.emplace_back("cvarFloat", "\"sensitivity\" 5 2 30"); }
                else { k.emplace_back("cvar", zit(f.cvar)); }
                b.item(f.name, k);
            }
            knopf(b, "zurueck", "Back", "40 420 180 30", "close " + name);
            break;
        }
        case SeitenVorlage::Credits: {
            b.kv(1, "onESC", "{ close " + name + " }");
            hintergrund(b);
            titel(b, "CREDITS");
            const char* const zeilen[] = {"Design: your name", "Menus: made with JK Studio", "Thanks: the Jedi Academy modding community"};
            for (int i = 0; i < 3; ++i) {
                b.item("zeile" + std::to_string(i + 1), {{"rect", "0 " + std::to_string(160 + i * 40) + " 640 30"},
                                                         {"text", zit(zeilen[i])},
                                                         {"textscale", "1"},
                                                         {"textalign", "ITEM_ALIGN_CENTER"},
                                                         {"textalignx", "320"},
                                                         {"forecolor", "0.9 0.9 1 1"},
                                                         {"decoration", ""},
                                                         {"visible", "1"}});
            }
            knopf(b, "zurueck", "Back", "40 420 180 30", "close " + name);
            break;
        }
        case SeitenVorlage::Ladebildschirm:
            hintergrund(b);
            titel(b, "LOADING");
            b.item("balken_rahmen", {{"rect", "120 400 400 16"}, {"style", "WINDOW_STYLE_EMPTY"}, {"border", "1"}, {"bordersize", "1"},
                                     {"bordercolor", "0.6 0.7 1 1"}, {"decoration", ""}, {"visible", "1"}});
            b.item("balken", {{"rect", "122 402 240 12"}, {"style", "WINDOW_STYLE_FILLED"}, {"backcolor", "0.3 0.5 1 1"}, {"decoration", ""}, {"visible", "1"}});
            break;
    }
    b.zeile(0, "}");
    return b.s;
}

}  // namespace bhed::menue
