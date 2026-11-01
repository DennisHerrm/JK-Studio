// assistent.cpp - Menue-Editor M6 Punkt 6: Einstellungsseiten-Assistent (siehe assistent.h).
//
// Eigener Code.
#include "bhed/menue/assistent.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <map>

namespace bhed::menue {

namespace {

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

std::string zahl(float f) {
    char b[48];
    std::snprintf(b, sizeof(b), "%g", static_cast<double>(f));
    return b;
}

std::string zit(const std::string& s) { return "\"" + s + "\""; }

}  // namespace

std::vector<CvarSteuerung> cvarSteuerungen(const std::vector<const Datei*>& dateien) {
    std::map<std::string, CvarSteuerung> m;
    for (const Datei* d : dateien) {
        for (const Menu& mn : d->menus) {
            for (const Item& it : mn.items) {
                if (it.cvar.empty() || m.count(klein(it.cvar)) != 0) { continue; }
                CvarSteuerung c;
                c.cvar = it.cvar;
                c.text = it.text;
                c.typ = it.type;
                c.herkunft = d->pfad + ": " + mn.window.name;
                if (it.type == ITEM_TYPE_SLIDER) {
                    if (!(it.maxVal > it.minVal)) { continue; }
                    c.vorgabe = it.defVal;
                    c.min = it.minVal;
                    c.max = it.maxVal;
                } else if (it.type == ITEM_TYPE_MULTI) {
                    if (!it.multiFeeder.empty() || it.cvarList.empty()) { continue; }   // Liste aus dem Code (Feeder): nicht nachbaubar
                    c.zeichenWerte = it.strDef;
                    for (std::size_t i = 0; i < it.cvarList.size(); ++i) {
                        const std::string w = it.strDef ? (i < it.cvarStr.size() ? it.cvarStr[i] : std::string())
                                                        : (i < it.cvarValue.size() ? zahl(it.cvarValue[i]) : std::string("0"));
                        c.liste.emplace_back(it.cvarList[i], w);
                    }
                } else if (it.type != ITEM_TYPE_YESNO) {
                    continue;
                }
                m.emplace(klein(it.cvar), c);
            }
        }
    }
    std::vector<CvarSteuerung> v;
    for (auto& [k, c] : m) { v.push_back(std::move(c)); }
    return v;
}

std::string einstellungsSeite(const std::vector<CvarSteuerung>& wahl, const std::string& name, const std::string& einr, const std::string& nl) {
    std::string s;
    auto z = [&](int tiefe, const std::string& t) { s += einr + std::string(static_cast<std::size_t>(tiefe), '\t') + t + nl; };
    auto kv = [&](int tiefe, const std::string& k, const std::string& v) { z(tiefe, v.empty() ? k : k + "\t\t" + v); };
    z(0, "menuDef");
    z(0, "{");
    kv(1, "name", zit(name));
    kv(1, "fullScreen", "1");
    kv(1, "rect", "0 0 640 480");
    kv(1, "visible", "1");
    kv(1, "focusColor", "1 1 1 1");
    kv(1, "onESC", "{ close " + name + " }");
    auto item = [&](const std::vector<std::pair<std::string, std::string>>& k) {
        z(1, "itemDef");
        z(1, "{");
        for (const auto& [a, b] : k) { kv(2, a, b); }
        z(1, "}");
    };
    item({{"name", "background"}, {"rect", "0 0 640 480"}, {"style", "WINDOW_STYLE_FILLED"}, {"backcolor", "0.02 0.03 0.08 1"}, {"decoration", ""}, {"visible", "1"}});
    item({{"name", "title"}, {"rect", "0 30 640 50"}, {"text", zit("SETTINGS")}, {"textscale", "1.4"}, {"textalign", "ITEM_ALIGN_CENTER"},
          {"textalignx", "320"}, {"forecolor", "1 0.8 0.3 1"}, {"decoration", ""}, {"visible", "1"}});
    const bool zweiSpalten = wahl.size() > 10;
    for (std::size_t i = 0; i < wahl.size(); ++i) {
        const CvarSteuerung& c = wahl[i];
        const int spalte = zweiSpalten ? static_cast<int>(i % 2) : 0;
        const int zeile = zweiSpalten ? static_cast<int>(i / 2) : static_cast<int>(i);
        const int x = zweiSpalten ? 20 + spalte * 310 : 60;
        const int b = zweiSpalten ? 300 : 520;
        std::string feld;
        for (char ch : klein(c.cvar)) { feld += std::isalnum(static_cast<unsigned char>(ch)) != 0 ? ch : '_'; }
        std::vector<std::pair<std::string, std::string>> k = {
            {"name", "setting_" + feld},
            {"type", c.typ == ITEM_TYPE_SLIDER ? "ITEM_TYPE_SLIDER" : c.typ == ITEM_TYPE_MULTI ? "ITEM_TYPE_MULTI" : "ITEM_TYPE_YESNO"},
            {"rect", std::to_string(x) + " " + std::to_string(100 + zeile * 32) + " " + std::to_string(b) + " 28"},
            {"text", zit(c.text.empty() ? c.cvar + ":" : c.text)},
            {"textscale", "0.8"},
            {"textalign", "ITEM_ALIGN_RIGHT"},
            {"textalignx", std::to_string(b / 2)},
            {"forecolor", "0.9 0.9 1 1"},
            {"visible", "1"}};
        if (c.typ == ITEM_TYPE_SLIDER) {
            k.emplace_back("cvarFloat", zit(c.cvar) + " " + zahl(c.vorgabe) + " " + zahl(c.min) + " " + zahl(c.max));
        } else {
            k.emplace_back("cvar", zit(c.cvar));
        }
        if (c.typ == ITEM_TYPE_MULTI) {
            std::string l = "{";
            for (const auto& [t, w] : c.liste) { l += " " + zit(t) + " " + (c.zeichenWerte ? zit(w) : w); }
            k.emplace_back(c.zeichenWerte ? "cvarStrList" : "cvarFloatList", l + " }");
        }
        item(k);
    }
    item({{"name", "back"}, {"type", "ITEM_TYPE_BUTTON"}, {"rect", "40 430 160 30"}, {"style", "WINDOW_STYLE_FILLED"}, {"backcolor", "0.1 0.2 0.4 0.6"},
          {"text", zit("Back")}, {"textscale", "1"}, {"textalign", "ITEM_ALIGN_CENTER"}, {"textalignx", "80"}, {"forecolor", "0.9 0.9 1 1"},
          {"visible", "1"}, {"action", "{ close " + name + " }"}});
    z(0, "}");
    return s;
}

}  // namespace bhed::menue
