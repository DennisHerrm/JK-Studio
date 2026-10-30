// fluss.cpp - Menue-Editor M6 Punkt 1: Menuefluss (siehe fluss.h). Portiert aus OpenJK
// (codemp/ui/ui_shared.c, code/ui/ui_shared.cpp, ui_main, cgame).
//
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2001 - 2013, Activision, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//
//   OpenJK is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License version 2 as published by the
//   Free Software Foundation. This program is distributed in the hope that it
//   will be useful, but WITHOUT ANY WARRANTY; without even the implied
//   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//   GNU General Public License for more details (LICENSE).
#include "bhed/menue/fluss.h"

#include "bhed/menue/ereignisse.h"

#include <algorithm>
#include <cctype>
#include <deque>
#include <map>

namespace bhed::menue {

namespace {

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

// Ein Skript mit Herkunft (Menue-Skript oder Item-Ereignis)
struct Skript {
    const std::string* text;
    std::string wo;
    int item;
};

std::vector<Skript> skripteVon(const Menu& m) {
    std::vector<Skript> v;
    const std::pair<const std::string*, const char*> menuS[] = {
        {&m.onOpen, "onOpen"}, {&m.onClose, "onClose"}, {&m.onESC, "onESC"}, {&m.onAccept, "onAccept"}};
    for (const auto& [t, n] : menuS) {
        if (!t->empty()) { v.push_back({t, n, -1}); }
    }
    for (std::size_t i = 0; i < m.items.size(); ++i) {
        const Item& it = m.items[i];
        const std::string nm = it.window.name.empty() ? "#" + std::to_string(i) : it.window.name;
        const std::pair<const std::string*, const char*> itS[] = {
            {&it.action, "action"},         {&it.onFocus, "onFocus"}, {&it.leaveFocus, "leaveFocus"}, {&it.mouseEnter, "mouseEnter"},
            {&it.mouseExit, "mouseExit"},   {&it.accept, "accept"},   {&it.selectNext, "selectNext"}, {&it.selectPrev, "selectPrev"},
            {&it.doubleClick, "doubleClick"}};
        for (const auto& [t, n] : itS) {
            if (!t->empty()) { v.push_back({t, nm + ": " + n, static_cast<int>(i)}); }
        }
    }
    return v;
}

// Item_SetFocus / Item_HandleKey: was ohne Skript Eingaben annimmt
bool nimmtEingabe(const Item& it) {
    if ((it.window.flags & WINDOW_DECORATION) != 0) { return false; }
    if (!it.action.empty()) { return true; }
    switch (it.type) {
        case ITEM_TYPE_RADIOBUTTON: case ITEM_TYPE_CHECKBOX: case ITEM_TYPE_EDITFIELD: case ITEM_TYPE_COMBO: case ITEM_TYPE_LISTBOX:
        case ITEM_TYPE_NUMERICFIELD: case ITEM_TYPE_SLIDER: case ITEM_TYPE_YESNO: case ITEM_TYPE_MULTI: case ITEM_TYPE_BIND:
        case ITEM_TYPE_SLIDER_INTEGER:
            return true;
        default:
            return false;
    }
}

}  // namespace

std::vector<std::string> einstiegsMenues(Dialekt d) {
    if (d == Dialekt::MP) {
        // codemp/ui/ui_main.c: UI_SetActiveMenu, UI_RunMenuScript, UI_DrawConnectScreen ("Connect") ...
        return {"main", "Connect", "error_popmenu", "team", "endofgame", "ingame", "ingame_player", "ingame_playerforce", "siege_popmenu",
                "ingame_siegeobjectives", "ingame_voicechat", "ingame_siegeclass", "setup_menu2", "password_request", "bad_server"};
    }
    // code/ui/ui_main.cpp: UI_MainMenu, UI_SetActiveMenu, UI_DataPadMenu, UI_RunMenuScript ...;
    // code/cgame: loadscreen (cg_main.cpp, cg_info.cpp), lefthud/righthud (cg_draw.cpp)
    return {"mainMenu", "loadscreen", "lefthud", "righthud", "error_popmenu", "setup_menu2", "missionfailed_menu", "mainhud", "characterMenu", "ingameMainMenu",
            "datapadMissionMenu", "datapadForcePowersMenu", "ingameForceHelp", "ingameloadMenu", "ingamesaveMenu",
            "ingameMissionSelect1", "ingameMissionSelect2", "ingameMissionSelect3", "ingameWpnSelectHelp"};
}

Fluss menueFluss(const std::vector<const Datei*>& dateien) {
    Fluss f;
    std::map<std::string, int> nachName;   // klein -> erster Knoten (Menus_FindByName)
    Dialekt dl = Dialekt::SP;
    for (std::size_t di = 0; di < dateien.size(); ++di) {
        const Datei& d = *dateien[di];
        if (di == 0) { dl = d.dialekt; }
        for (std::size_t mi = 0; mi < d.menus.size(); ++mi) {
            FlussMenue n;
            n.name = d.menus[mi].window.name;
            n.datei = static_cast<int>(di);
            n.menu = static_cast<int>(mi);
            const int k = static_cast<int>(f.menues.size());
            f.menues.push_back(n);
            nachName.emplace(klein(n.name), k);
        }
    }
    auto knoten = [&](const std::string& name) {
        const auto it = nachName.find(klein(name));
        if (it != nachName.end()) { return it->second; }
        FlussMenue n;   // fehlt: nur als Ziel bekannt
        n.name = name;
        const int k = static_cast<int>(f.menues.size());
        f.menues.push_back(n);
        nachName.emplace(klein(name), k);
        return k;
    };
    const std::size_t echte = f.menues.size();
    std::vector<bool> ausgang(echte, false), zurueckSelbst(echte, false);
    std::vector<std::vector<std::string>> geoeffnet(echte);   // Ziele von open je Menue (klein)
    for (std::size_t k = 0; k < echte; ++k) {
        const Datei& d = *dateien[static_cast<std::size_t>(f.menues[k].datei)];
        const Menu& m = d.menus[static_cast<std::size_t>(f.menues[k].menu)];
        const std::string selbst = klein(m.window.name);
        bool eingabe = !m.onESC.empty();
        for (const Item& it : m.items) { eingabe = eingabe || nimmtEingabe(it); }
        f.menues[k].eingabe = eingabe;
        for (const Skript& s : skripteVon(m)) {
            for (const Baustein& b : zerlege(*s.text, d.dialekt, d.spiel)) {
                const std::string bf = klein(b.befehl);
                if (bf == "opengotomenu") {
                    ausgang[k] = true;
                } else if (bf == "open" && !b.args.empty()) {
                    const std::string ziel = klein(b.args[0]);
                    if (ziel != selbst) { ausgang[k] = true; }
                    geoeffnet[k].push_back(ziel);
                    const int z = knoten(b.args[0]);
                    f.kanten.push_back({static_cast<int>(k), z, s.wo, s.item});
                } else if (bf == "close" && !b.args.empty()) {
                    const std::string ziel = klein(b.args[0]);
                    if (ziel == selbst || ziel == "all") {
                        ausgang[k] = true;
                        zurueckSelbst[k] = true;
                    }
                }
            }
        }
    }
    // Einstiege und Ebenen (Breitensuche ueber open)
    std::deque<int> q;
    for (const std::string& e : einstiegsMenues(dl)) {
        const auto it = nachName.find(klein(e));
        if (it == nachName.end() || static_cast<std::size_t>(it->second) >= echte) { continue; }
        FlussMenue& n = f.menues[static_cast<std::size_t>(it->second)];
        n.einstieg = true;
        n.ebene = 0;
        q.push_back(it->second);
    }
    while (!q.empty()) {
        const int k = q.front();
        q.pop_front();
        for (const FlussKante& e : f.kanten) {
            if (e.von != k) { continue; }
            FlussMenue& z = f.menues[static_cast<std::size_t>(e.nach)];
            if (z.ebene < 0) {
                z.ebene = f.menues[static_cast<std::size_t>(k)].ebene + 1;
                q.push_back(e.nach);
            }
        }
    }
    for (std::size_t k = 0; k < f.menues.size(); ++k) {
        FlussMenue& n = f.menues[k];
        n.erreichbar = n.ebene >= 0;
        if (k >= echte) { continue; }
        n.sackgasse = n.eingabe && !ausgang[k];
        // zurueck: close <selbst>/close all, oder es oeffnet eines der Menues, die es oeffnen
        bool hatEltern = false, zurueck = zurueckSelbst[k];
        for (const FlussKante& e : f.kanten) {
            if (e.nach != static_cast<int>(k) || e.von == static_cast<int>(k)) { continue; }
            hatEltern = true;
            const std::string eltern = klein(f.menues[static_cast<std::size_t>(e.von)].name);
            zurueck = zurueck || std::find(geoeffnet[k].begin(), geoeffnet[k].end(), eltern) != geoeffnet[k].end();
        }
        n.ohneZurueck = hatEltern && n.eingabe && !n.einstieg && !zurueck;
    }
    return f;
}

}  // namespace bhed::menue
