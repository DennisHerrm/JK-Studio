// skript.cpp - Menue-Editor M1 Punkt 6: Skripte und Bedienung wie im Spiel.
//
// Portiert aus OpenJK: codemp/ui/ui_shared.c (Item_RunScript mit
// String_Parse, commandList: Script_Show, Script_Hide, Script_Open,
// Script_Close, Script_SetFocus, Script_SetCvar, Script_SetItemColor,
// Script_SetItemText, Script_SetBackground, Script_SetItemBackground,
// Script_SetItemRect, Script_Transition, Script_FadeIn/FadeOut,
// Script_Play/PlayLooped, Script_Exec; Menu_ItemsMatchingGroup,
// Menu_ShowItemByName, Menus_Activate/ActivateByName, Menus_CloseByName,
// Menus_CloseAll, Menu_RunCloseScript, Menu_ClearFocus, Item_SetFocus,
// Item_Action, Item_YesNo_HandleKey, Item_Multi_HandleKey,
// Item_Slider_HandleKey (Maus), Item_EnableShowViaCvar).
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
#include "bhed/menue/skript.h"

#include "bhed/menue/profil.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <set>

namespace bhed::menue {

namespace {

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}
bool gleich(const std::string& a, const std::string& b) { return klein(a) == klein(b); }

// Befehle, die nicht jede commandList hat (c klein): code/ui (JA SP) und Movie Duels haben
// playlooped auskommentiert; Jedi Outcast SP (jk2sp.exe 1.04, Namen im Programm gelesen) kennt
// setitemrect, setitemtext, setitembackground, transition2/3, delay, playVoice/stopVoice,
// setitemexec und increment-/decrementfeeder nicht. Unbekanntes geht wie im Spiel an DC->runScript.
bool inCommandList(const std::string& c, Dialekt dl, Spiel sp) {
    if (dl == Dialekt::SP && sp != Spiel::JK2 && c == "playlooped") { return false; }
    if (dl == Dialekt::SP && sp == Spiel::JK2) {
        static const char* const fehlt[] = {"setitemrect", "setitemtext", "setitembackground", "transition2", "transition3", "delay", "playvoice",
                                            "stopvoice", "setitemexec", "incrementfeeder", "decrementfeeder"};
        for (const char* f : fehlt) {
            if (c == f) { return false; }
        }
    }
    return true;
}

// M3: wie viele Woerter UI_RunMenuScript nach dem Namen liest (String_Parse/Int_Parse auf
// args; OpenJK code/ui/ui_main.cpp, codemp/ui/ui_main.c, Movie Duels code/ui/ui_main.cpp).
// Alle anderen Namen: keine - der Rest des Skripts laeuft weiter (Item_RunScript).
int uiSkriptWoerter(const std::string& name, Dialekt dl, Spiel sp) {
    struct Eintrag {
        const char* name;
        int woerter;
    };
    static const Eintrag kSp[] = {{"checkvid1data", 2}, {"startmap", 1}, {"clearmouseover", 1}, {"initallocforcepower", 1},
                                  {"affectforcepowerlevel", 1}, {"showforceleveldesc", 1}, {"loadmissionselectmenu", 1},
                                  {"giveweapon", 1}, {"equipweapon", 1}, {"addweaponselection", 7}, {"addthrowweaponselection", 7},
                                  {"removeweaponselection", 1}, {"normalweaponselection", 1}, {"highlightweaponselection", 1},
                                  {"giveinventory", 2}, {"update", 1}};
    static const Eintrag kMd[] = {{"mdupdate", 1}, {"md_wheelpick", 1}, {"giveammo", 3}, {"addpistolselection", 7}};
    static const Eintrag kMp[] = {{"serversort", 1}, {"orders", 1}, {"voiceordersteam", 1}, {"voiceorders", 1}, {"setforce", 1},
                                  {"clearmouseover", 1}, {"update", 1}, {"setsiegeobjbuttons", 3}, {"updatesiegeclasscnt", 1}};
    const std::string n = klein(name);
    if (dl == Dialekt::MP) {
        for (const Eintrag& e : kMp) {
            if (n == e.name) { return e.woerter; }
        }
        return 0;
    }
    for (const Eintrag& e : kSp) {
        if (n == e.name) { return e.woerter; }
    }
    if (sp == Spiel::MD) {
        for (const Eintrag& e : kMd) {
            if (n == e.name) { return e.woerter; }
        }
    }
    return 0;
}

// M3: die Befehle der commandList, die die Vorschau nur meldet - mit den Woertern, die ihr
// Handler liest (ui_shared Script_*). -1 = der Rest des Skripts (defer/delay halten es an).
int befehlWoerter(const std::string& c, Dialekt dl) {
    if (c == "defer" || c == "delay") { return -1; }
    if (c == "exec" || c == "setasset" || c == "setplayermodel" || c == "showmenu") { return 1; }
    if (c == "setitembackground" || c == "setitemflag" || c == "activate" || c == "disable" || c == "scale" || c == "setitemrectcvar" ||
        c == "setitemexec") {
        return 2;
    }
    if (c == "setitemcolorcvar") { return 3; }
    (void)dl;
    return 0;   // setteamcolor, rundeferred, opengotomenu, stopvoice, incrementfeeder, decrementfeeder
}

// String_Parse (COM_ParseExt mit allowLineBreaks = false) ueber einen Skripttext
bool stringParse(const std::string& s, std::size_t& p, std::string& out) {
    while (p < s.size() && static_cast<unsigned char>(s[p]) <= ' ') {
        if (s[p] == '\n') { return false; }
        ++p;
    }
    if (p >= s.size()) { return false; }
    out.clear();
    if (s[p] == '"') {
        ++p;
        while (p < s.size() && s[p] != '"') { out += s[p++]; }
        if (p < s.size()) { ++p; }
        return true;
    }
    while (p < s.size() && static_cast<signed char>(s[p]) > 32) { out += s[p++]; }
    return !out.empty();
}

float zahl(const std::string& s) { return static_cast<float>(std::atof(s.c_str())); }

void merke(std::vector<CvarFund>& v, const std::string& name, const std::string& wert, const std::string& wofuer, bool sicht) {
    if (name.empty()) { return; }
    auto it = std::find_if(v.begin(), v.end(), [&](const CvarFund& f) { return gleich(f.name, name); });
    if (it == v.end()) {
        v.push_back(CvarFund{name, {}, {}, false});
        it = v.end() - 1;
    }
    if (!wert.empty() && std::find(it->werte.begin(), it->werte.end(), wert) == it->werte.end()) { it->werte.push_back(wert); }
    if (!wofuer.empty() && std::find(it->wofuer.begin(), it->wofuer.end(), wofuer) == it->wofuer.end() && it->wofuer.size() < 20) {
        it->wofuer.push_back(wofuer);
    }
    it->sichtbarkeit = it->sichtbarkeit || sicht;
}

// setcvar X Y in einem Skript
void skriptCvars(std::vector<CvarFund>& v, const std::string& skript, const std::string& wo) {
    std::size_t p = 0;
    std::string w;
    while (stringParse(skript, p, w)) {
        if (gleich(w, "setcvar")) {
            std::string n, x;
            if (stringParse(skript, p, n) && stringParse(skript, p, x)) { merke(v, n, x, "setcvar (" + wo + ")", false); }
        }
    }
}

}  // namespace

int uiSkriptWortzahl(const std::string& name, Dialekt dl, Spiel sp) { return uiSkriptWoerter(name, dl, sp); }

std::vector<CvarFund> cvarsImMenue(const Datei& d) {
    std::vector<CvarFund> v;
    for (const Menu& m : d.menus) {
        const std::string mn = "menu " + m.window.name;
        for (const std::string* s : {&m.onOpen, &m.onClose, &m.onESC, &m.onAccept}) { skriptCvars(v, *s, mn); }
        for (const Item& it : m.items) {
            const std::string in = "item " + (it.window.name.empty() ? std::string("?") : it.window.name);
            if (!it.cvarTest.empty()) {
                std::size_t p = 0;
                std::string w;
                bool eins = false;
                while (stringParse(it.enableCvar, p, w)) {
                    if (w != ";") {
                        merke(v, it.cvarTest, w, "cvarTest (" + in + ")", true);
                        eins = true;
                    }
                }
                if (!eins) { merke(v, it.cvarTest, "", "cvarTest (" + in + ")", true); }
            }
            if (!it.cvar.empty()) {
                const std::string art = std::string(typName(it.type)).empty() ? "cvar" : typName(it.type);
                merke(v, it.cvar, "", art + " (" + in + ")", false);
                if (it.type == ITEM_TYPE_YESNO) {
                    merke(v, it.cvar, "0", "", false);
                    merke(v, it.cvar, "1", "", false);
                }
                for (std::size_t i = 0; i < it.cvarStr.size(); ++i) { merke(v, it.cvar, it.cvarStr[i], "", false); }
                for (const float f : it.cvarValue) { merke(v, it.cvar, zahlText(f), "", false); }
            }
            for (const std::string* s : {&it.action, &it.onFocus, &it.leaveFocus, &it.mouseEnter, &it.mouseExit, &it.accept}) {
                skriptCvars(v, *s, in);
            }
        }
    }
    std::sort(v.begin(), v.end(), [](const CvarFund& a, const CvarFund& b) {
        if (a.sichtbarkeit != b.sichtbarkeit) { return a.sichtbarkeit; }
        return klein(a.name) < klein(b.name);
    });
    return v;
}

std::vector<Seite> seitenImMenue(const Datei& d) {
    std::vector<Seite> s;
    for (std::size_t mi = 0; mi < d.menus.size(); ++mi) {
        const Menu& m = d.menus[mi];
        Seite ganz;
        ganz.menu = static_cast<int>(mi);
        ganz.titel = m.window.name.empty() ? "menuDef " + std::to_string(mi + 1) : m.window.name;
        s.push_back(ganz);
        // ui_tab-artig: ein cvarTest mit showCvar und mehreren Werten
        std::map<std::string, std::vector<std::string>> tabs;
        for (const Item& it : m.items) {
            if (it.cvarTest.empty() || (it.cvarFlags & CVAR_SHOW) == 0) { continue; }
            std::size_t p = 0;
            std::string w;
            while (stringParse(it.enableCvar, p, w)) {
                if (w == ";") { continue; }
                auto& l = tabs[it.cvarTest];
                if (std::find(l.begin(), l.end(), w) == l.end()) { l.push_back(w); }
            }
        }
        for (const auto& [cv, werte] : tabs) {
            if (werte.size() < 2) { continue; }
            for (const std::string& w : werte) {
                Seite t;
                t.menu = static_cast<int>(mi);
                t.cvar = cv;
                t.wert = w;
                t.titel = ganz.titel + " / " + cv + "=" + w;
                s.push_back(t);
            }
        }
    }
    return s;
}

// --- Laufzeit ------------------------------------------------------------------

void Laufzeit::start(const Datei& datei) {
    d = &datei;
    menuFlags.assign(datei.menus.size(), 0);
    items.clear();
    rects.clear();
    texte.clear();
    vorder.clear();
    vorderGesetzt.clear();
    hinten.clear();
    hintenGesetzt.clear();
    rand.clear();
    randGesetzt.clear();
    anim.clear();
    modellPfad.clear();
    modellHaut.clear();
    modellAnim.clear();
    animGesetzt.clear();
    letzterTakt = -1;
    hintergrund.assign(datei.menus.size(), std::string());
    for (const Menu& m : datei.menus) {
        std::vector<ItemZustand> z;
        std::vector<Rechteck> r;
        for (const Item& it : m.items) {
            z.push_back(ItemZustand{it.window.flags, true});
            r.push_back(it.window.rect);
        }
        items.push_back(z);
        rects.push_back(r);
        texte.emplace_back(m.items.size());
        vorder.emplace_back(m.items.size());
        vorderGesetzt.emplace_back(m.items.size(), false);
        hinten.emplace_back(m.items.size());
        hintenGesetzt.emplace_back(m.items.size(), false);
        rand.emplace_back(m.items.size());
        randGesetzt.emplace_back(m.items.size(), false);
        modellPfad.emplace_back(m.items.size());
        modellHaut.emplace_back(m.items.size());
        modellAnim.emplace_back(m.items.size());
        animGesetzt.emplace_back(m.items.size(), -1);
        std::vector<ItemAnimation> a(m.items.size());
        for (std::size_t i = 0; i < m.items.size(); ++i) {
            a[i].client = m.items[i].window.rectClient;
            a[i].alpha = m.items[i].window.foreColor.a;
        }
        anim.push_back(a);
    }
    for (std::size_t i = 0; i < datei.menus.size(); ++i) { menuFlags[i] = datei.menus[i].window.flags; }
    stapel.clear();
    ++version;
}

int Laufzeit::finde(const std::string& name) const {
    if (d == nullptr) { return -1; }
    for (std::size_t i = 0; i < d->menus.size(); ++i) {
        if (gleich(d->menus[i].window.name, name)) { return static_cast<int>(i); }
    }
    return -1;
}

std::string Laufzeit::cvar(const std::string& n) const {
    const auto it = cvars.find(klein(n));
    if (it != cvars.end()) { return it->second; }
    if (basis.cvar) { return basis.cvar(n); }   // die Werte des Editors (Vorschau-Cvars, sonst die Vorgaben der Engine)
    if (d != nullptr && d->profil) {   // M5b: Vorgaben der Mod (cvarTable laut Mod-Profil)
        for (const auto& [name, w] : d->profil->cvars) {
            if (gleich(name, n)) { return w; }
        }
    }
    return d != nullptr ? spielCvarVorgabe(d->dialekt, d->spiel, n) : std::string();   // was die Engine selbst anlegt
}

void Laufzeit::setzeCvar(const std::string& n, const std::string& w) {
    cvars[klein(n)] = w;
    ++version;
}

void Laufzeit::oeffne(const std::string& name) {   // Menus_ActivateByName + Menus_Activate
    const int m = finde(name);
    if (m < 0) {
        protokoll.push_back("open " + name + ": kein menuDef mit diesem Namen in dieser Datei");
        fremdesMenue = name;   // Menus_ActivateByName sucht in allen geladenen Menues (menus.txt)
        return;
    }
    int fokus = -1;
    for (std::size_t i = 0; i < menuFlags.size(); ++i) {
        if ((menuFlags[i] & WINDOW_HASFOCUS) != 0) { fokus = static_cast<int>(i); }
    }
    menuFlags[static_cast<std::size_t>(m)] |= WINDOW_HASFOCUS | WINDOW_VISIBLE;
    if (fokus >= 0 && fokus != m) {
        stapel.push_back(fokus);
        menuFlags[static_cast<std::size_t>(fokus)] &= ~WINDOW_HASFOCUS;
    }
    ++version;
    // Menus_Activate: soundLoop -> DC->startBackgroundTrack(soundName, soundName)
    if (!d->menus[static_cast<std::size_t>(m)].soundName.empty()) { klaenge.push_back("musik:" + d->menus[static_cast<std::size_t>(m)].soundName); }
    fuehreAus(m, -1, d->menus[static_cast<std::size_t>(m)].onOpen);
}

void Laufzeit::schliesse(const std::string& name) {
    if (d == nullptr) { return; }
    if (gleich(name, "all")) {   // Menus_CloseAll
        for (std::size_t i = 0; i < menuFlags.size(); ++i) {
            if ((menuFlags[i] & WINDOW_VISIBLE) != 0) { fuehreAus(static_cast<int>(i), -1, d->menus[i].onClose); }
            menuFlags[i] &= ~(WINDOW_HASFOCUS | WINDOW_VISIBLE);
        }
        stapel.clear();
        ++version;
        return;
    }
    const int m = finde(name);
    if (m < 0) { return; }
    if ((menuFlags[static_cast<std::size_t>(m)] & WINDOW_VISIBLE) != 0) {
        fuehreAus(m, -1, d->menus[static_cast<std::size_t>(m)].onClose);   // Menu_RunCloseScript
    }
    menuFlags[static_cast<std::size_t>(m)] &= ~(WINDOW_VISIBLE | WINDOW_HASFOCUS);
    if (!stapel.empty()) {
        const int vorher = stapel.back();
        stapel.pop_back();
        menuFlags[static_cast<std::size_t>(vorher)] |= WINDOW_HASFOCUS;
    }
    ++version;
}

void Laufzeit::fuehreAus(int menu, int item, const std::string& s) {
    if (d == nullptr || menu < 0 || static_cast<std::size_t>(menu) >= d->menus.size() || s.empty()) { return; }
    const Menu& m = d->menus[static_cast<std::size_t>(menu)];
    auto& z = items[static_cast<std::size_t>(menu)];
    auto melde = [&](const std::string& t) {
        if (static_cast<int>(protokoll.size()) < maxProtokoll) { protokoll.push_back(t); }
    };
    // Menu_ItemsMatchingGroup: name oder group gleich
    auto passend = [&](const std::string& n, auto f) {
        for (std::size_t i = 0; i < m.items.size(); ++i) {
            if (gleich(m.items[i].window.name, n) || (!m.items[i].window.group.empty() && gleich(m.items[i].window.group, n))) {
                f(i);
            }
        }
    };
    // M5b: uiScripts der Mod (Mod-Profil) mit ihrer Wortzahl, sonst die Tabelle der Spiele
    auto woerterNach = [&](const std::string& name) {
        if (d->profil) {
            for (const auto& [n, w] : d->profil->uiSkripte) {
                if (gleich(n, name)) { return w; }
            }
        }
        return uiSkriptWoerter(name, d->dialekt, d->spiel);
    };
    std::size_t p = 0;
    std::string cmd;
    int schutz = 0;
    while (stringParse(s, p, cmd) && ++schutz < 1000) {
        if (cmd == ";") { continue; }
        // nicht in der commandList dieses Spiels: leer, dann nimmt der letzte Zweig (DC->runScript) es
        const std::string c = inCommandList(klein(cmd), d->dialekt, d->spiel) ? klein(cmd) : std::string();
        std::string a, b;
        if (c == "show" || c == "hide") {
            if (stringParse(s, p, a)) {
                const bool zeigen = c == "show";
                passend(a, [&](std::size_t i) {
                    if (zeigen) { z[i].flags |= WINDOW_VISIBLE; }
                    else { z[i].flags &= ~WINDOW_VISIBLE; }
                });
                ++version;
            }
        } else if (c == "fadein" || c == "fadeout") {
            // Menu_FadeItemByName: das Ausblenden selbst macht Fade() im Takt (takt)
            if (stringParse(s, p, a)) {
                const bool aus = c == "fadeout";
                passend(a, [&](std::size_t i) {
                    if (aus) {
                        z[i].flags |= (WINDOW_FADINGOUT | WINDOW_VISIBLE);
                        z[i].flags &= ~WINDOW_FADINGIN;
                    } else {
                        z[i].flags |= (WINDOW_VISIBLE | WINDOW_FADINGIN);
                        z[i].flags &= ~WINDOW_FADINGOUT;
                    }
                });
                ++version;
            }
        } else if (c == "open") {
            if (stringParse(s, p, a)) { oeffne(a); }
        } else if (c == "close") {
            if (stringParse(s, p, a)) { schliesse(a); }
        } else if (c == "setfocus") {
            if (stringParse(s, p, a)) {
                for (std::size_t i = 0; i < m.items.size(); ++i) {
                    if (!gleich(m.items[i].window.name, a)) { continue; }
                    if ((z[i].flags & (WINDOW_DECORATION | WINDOW_HASFOCUS)) != 0) { break; }
                    for (ItemZustand& x : z) { x.flags &= ~WINDOW_HASFOCUS; }   // Menu_ClearFocus
                    z[i].flags |= WINDOW_HASFOCUS;
                    ++version;
                    fuehreAus(menu, static_cast<int>(i), m.items[i].onFocus);
                    // M3: Script_SetFocus spielt danach DC->Assets.itemFocusSound (nicht den des Items)
                    if (!fokusKlangStd.empty()) { klaenge.push_back(fokusKlangStd); }
                    break;
                }
            }
        } else if (c == "setcvar") {
            if (stringParse(s, p, a) && stringParse(s, p, b)) { setzeCvar(a, b); }
        } else if (c == "setcvartocvar") {
            if (stringParse(s, p, a) && stringParse(s, p, b)) { setzeCvar(a, cvar(b)); }
        } else if (c == "setitemcolor") {
            std::string feld, r, g, bl, al;
            if (stringParse(s, p, a) && stringParse(s, p, feld) && stringParse(s, p, r) && stringParse(s, p, g) &&
                stringParse(s, p, bl) && stringParse(s, p, al)) {
                // Script_SetItemColor: backcolor, forecolor (setzt WINDOW_FORECOLORSET), bordercolor
                const Farbe f{zahl(r), zahl(g), zahl(bl), zahl(al)};
                const std::size_t mu = static_cast<std::size_t>(menu);
                if (gleich(feld, "forecolor")) {
                    passend(a, [&](std::size_t i) {
                        vorder[mu][i] = f;
                        vorderGesetzt[mu][i] = true;
                        items[mu][i].flags |= WINDOW_FORECOLORSET;
                    });
                    ++version;
                } else if (gleich(feld, "backcolor")) {
                    passend(a, [&](std::size_t i) {
                        hinten[mu][i] = f;
                        hintenGesetzt[mu][i] = true;
                    });
                    ++version;
                } else if (gleich(feld, "bordercolor")) {
                    passend(a, [&](std::size_t i) {
                        rand[mu][i] = f;
                        randGesetzt[mu][i] = true;
                    });
                    ++version;
                }
            }
        } else if (c == "setitemtext") {
            if (stringParse(s, p, a) && stringParse(s, p, b)) {
                passend(a, [&](std::size_t i) { texte[static_cast<std::size_t>(menu)][i] = b; });
                ++version;
            }
        } else if (c == "setbackground") {
            if (stringParse(s, p, a)) {
                hintergrund[static_cast<std::size_t>(menu)] = a;
                ++version;
            }
        } else if (c == "setitemrect") {
            std::string x, y, w, h;
            if (stringParse(s, p, a) && stringParse(s, p, x) && stringParse(s, p, y) && stringParse(s, p, w) && stringParse(s, p, h)) {
                // Script_SetItemRect: codemp addiert menu->window.rect.x/y, code/ui (SP) setzt die Zahlen direkt
                const float ox = d->dialekt == Dialekt::MP ? m.window.rect.x : 0.0F, oy = d->dialekt == Dialekt::MP ? m.window.rect.y : 0.0F;
                passend(a, [&](std::size_t i) { rects[static_cast<std::size_t>(menu)][i] = Rechteck{ox + zahl(x), oy + zahl(y), zahl(w), zahl(h)}; });
                ++version;
            }
        } else if (c == "transition") {
            // Script_Transition: name rectFrom rectTo time amt -> Menu_TransitionItemByName
            std::string v[10];
            bool ok = stringParse(s, p, a);
            for (int i = 0; ok && i < 10; ++i) { ok = stringParse(s, p, v[i]); }
            if (ok) {
                const Rechteck von{zahl(v[0]), zahl(v[1]), zahl(v[2]), zahl(v[3])};
                const Rechteck nach{zahl(v[4]), zahl(v[5]), zahl(v[6]), zahl(v[7])};
                const float amt = zahl(v[9]);
                passend(a, [&](std::size_t i) {
                    ItemAnimation& an = anim[static_cast<std::size_t>(menu)][i];
                    z[i].flags |= (WINDOW_INTRANSITION | WINDOW_VISIBLE);
                    an.offsetTime = static_cast<int>(std::atoi(v[8].c_str()));
                    an.client = von;
                    an.effects = nach;
                    an.effects2 = Rechteck{std::fabs(nach.x - von.x) / amt, std::fabs(nach.y - von.y) / amt, std::fabs(nach.w - von.w) / amt,
                                           std::fabs(nach.h - von.h) / amt};
                    positionAus(menu, i);   // Item_UpdatePosition
                });
                ++version;
            }
        } else if (c == "transition2") {
            // Script_Transition2: name rectTo time amt, von = window.rect des ersten passenden Items
            std::string v[6];
            bool ok = stringParse(s, p, a);
            for (int i = 0; ok && i < 6; ++i) { ok = stringParse(s, p, v[i]); }
            if (ok) {
                const Rechteck nach{zahl(v[0]), zahl(v[1]), zahl(v[2]), zahl(v[3])};
                const float amt = zahl(v[5]);
                bool erstes = true;
                Rechteck von;
                passend(a, [&](std::size_t i) {
                    if (erstes) {   // "they'll all use the FIRST one's FROM" - window.rect (Bildschirm)
                        von = rects[static_cast<std::size_t>(menu)][i];
                        erstes = false;
                    }
                    ItemAnimation& an = anim[static_cast<std::size_t>(menu)][i];
                    z[i].flags |= (WINDOW_INTRANSITION | WINDOW_VISIBLE);
                    an.offsetTime = static_cast<int>(std::atoi(v[4].c_str()));
                    an.client = von;
                    an.effects = nach;
                    an.effects2 = Rechteck{std::fabs(nach.x - von.x) / amt, std::fabs(nach.y - von.y) / amt, std::fabs(nach.w - von.w) / amt,
                                           std::fabs(nach.h - von.h) / amt};
                    positionAus(menu, i);
                });
                ++version;
            }
        } else if (c == "orbit") {
            // Script_Orbit: name x y cx cy time -> Menu_OrbitItemByName
            std::string v[5];
            bool ok = stringParse(s, p, a);
            for (int i = 0; ok && i < 5; ++i) { ok = stringParse(s, p, v[i]); }
            if (ok) {
                passend(a, [&](std::size_t i) {
                    ItemAnimation& an = anim[static_cast<std::size_t>(menu)][i];
                    z[i].flags |= (WINDOW_ORBITING | WINDOW_VISIBLE);
                    an.offsetTime = static_cast<int>(std::atoi(v[4].c_str()));
                    an.effects.x = zahl(v[2]);
                    an.effects.y = zahl(v[3]);
                    an.client.x = zahl(v[0]);
                    an.client.y = zahl(v[1]);
                    positionAus(menu, i);
                });
                ++version;
            }
        } else if (c == "transition3") {
            std::string rest = cmd;
            while (stringParse(s, p, a) && a != ";") { rest += " " + a; }
            melde(rest);   // Modell-Uebergang (WINDOW_INTRANSITIONMODEL): braucht die 3D-Ansicht
        } else if (c == "uiscript") {
            // "uiScript" steht nicht in der commandList: DC->runScript liest den Namen und
            // dessen Woerter (UI_RunMenuScript), der Rest des Skripts laeuft weiter
            std::string name;
            (void)stringParse(s, p, name);
            std::string rest = cmd + " " + name;
            for (int w = woerterNach(name); w > 0 && stringParse(s, p, a); --w) { rest += " " + a; }
            if (!uiSkript(menu, klein(name))) { melde(rest); }
        } else if (c == "play" || c == "playlooped" || c == "playvoice") {
            // Script_Play: DC->startLocalSound(DC->registerSound(val), CHAN_LOCAL_SOUND);
            // playlooped: DC->startBackgroundTrack(val, val)
            if (stringParse(s, p, a)) {
                melde(cmd + " " + a);
                klaenge.push_back(c == "playlooped" ? "musik:" + a : a);
            }
        } else if (c == "exec" || c == "setitembackground" || c == "setasset" ||
                   c == "setplayermodel" || c == "setteamcolor" || c == "setitemflag" || c == "defer" || c == "rundeferred" ||
                   c == "delay" || c == "activate" || c == "showmenu" || c == "opengotomenu" || c == "stopvoice" ||
                   c == "incrementfeeder" || c == "decrementfeeder" || c == "disable" || c == "scale" ||
                   c == "setitemcolorcvar" || c == "setitemrectcvar" || c == "setitemexec") {
            // ohne Spiel nicht ausfuehrbar: mit den Woertern des Handlers melden
            std::string rest = cmd;
            const int woerter = befehlWoerter(c, d->dialekt);
            for (int w = woerter; (woerter < 0 || w > 0) && stringParse(s, p, a); --w) { rest += " " + a; }
            melde(rest);
            // M3 Punkt 3: Script_Exec gibt den Text an die Konsole - "music <intro> [<loop>]"
            // ist dort S_StartBackgroundTrack (Movie Duels main.menu onOpen)
            if (c == "exec" && klein(a).compare(0, 6, "music ") == 0) {
                std::size_t q = 6;
                std::string titel;
                if (stringParse(a, q, titel) && !titel.empty()) { klaenge.push_back("musik:" + titel); }
            }
            if (c == "showmenu" || c == "opengotomenu") {
                const std::size_t leer = rest.find(' ');
                if (leer != std::string::npos) { oeffne(rest.substr(leer + 1, rest.find(' ', leer + 1) - leer - 1)); }
            }
        } else {
            // nicht in der commandList: DC->runScript(&p) - UI_RunMenuScript liest das NAECHSTE
            // Wort als Namen (samt dessen Woertern), der Rest des Skripts laeuft weiter
            std::string name;
            (void)stringParse(s, p, name);
            std::string rest = cmd + " " + name;
            for (int w = woerterNach(name); w > 0 && stringParse(s, p, a); --w) { rest += " " + a; }
            if (name.empty() || !uiSkript(menu, klein(name))) { melde("uiScript: " + rest); }
        }
    }
    (void)item;
}

void Laufzeit::fuelle(int menu, Umgebung& u) const {
    u = basis;
    u.zustand.clear();
    if (menu >= 0 && static_cast<std::size_t>(menu) < items.size()) { u.zustand = items[static_cast<std::size_t>(menu)]; }
    u.cvar = [this](const std::string& n) { return cvar(n); };
}

void Laufzeit::anwenden(Datei& kopie) const {
    for (std::size_t mi = 0; mi < kopie.menus.size() && mi < rects.size(); ++mi) {
        Menu& m = kopie.menus[mi];
        if (!hintergrund[mi].empty()) { m.window.background = hintergrund[mi]; }
        for (std::size_t i = 0; i < m.items.size() && i < rects[mi].size(); ++i) {
            m.items[i].window.rect = rects[mi][i];
            if (!texte[mi][i].empty()) { m.items[i].text = texte[mi][i]; }
            if (!modellPfad[mi][i].empty()) { m.items[i].assetModel = modellPfad[mi][i]; }
            if (!modellHaut[mi][i].empty()) { m.items[i].g2skin = modellHaut[mi][i] == "-" ? std::string() : modellHaut[mi][i]; }
            if (!modellAnim[mi][i].empty()) { m.items[i].g2anim = modellAnim[mi][i]; }
            if (vorderGesetzt[mi][i]) { m.items[i].window.foreColor = vorder[mi][i]; }
            if (hintenGesetzt[mi][i]) { m.items[i].window.backColor = hinten[mi][i]; }
            if (randGesetzt[mi][i]) { m.items[i].window.borderColor = rand[mi][i]; }
        }
    }
}

namespace {
// Rect_ContainsPoint (ui_shared.cpp/.c): echt innerhalb, der Rand zaehlt nicht
bool imRechteck(const Rechteck& r, float x, float y) { return x > r.x && x < r.x + r.w && y > r.y && y < r.y + r.h; }
}  // namespace

int Laufzeit::flagsGemalt(int menu, std::size_t i, const Umgebung& u) const {
    const Item& it = d->menus[static_cast<std::size_t>(menu)].items[i];
    int f = items[static_cast<std::size_t>(menu)][i].flags;
    if (it.window.ownerDrawFlags != 0 && u.ownerDrawSichtbar) {   // Item_Paint: DC->ownerDrawVisible
        if (u.ownerDrawSichtbar(it.window.ownerDrawFlags)) {
            f |= WINDOW_VISIBLE;
        } else {
            f &= ~WINDOW_VISIBLE;
        }
    }
    return f;
}

Rechteck Laufzeit::textRectBei(int menu, std::size_t i, const Umgebung& u) const {
    Item it = d->menus[static_cast<std::size_t>(menu)].items[i];
    it.window.rect = rects[static_cast<std::size_t>(menu)][i];
    const std::string t = !it.text.empty() ? (u.texte != nullptr ? u.texte->imSpiel(it.text) : it.text) : (it.cvar.empty() ? std::string() : cvar(it.cvar));
    if (t.empty()) { return {}; }
    return textRechteck(it, t, u, d->spiel);
}

std::vector<int> Laufzeit::tabReihenfolge(int menu) const {
    std::vector<int> r;
    if (d == nullptr || menu < 0 || static_cast<std::size_t>(menu) >= d->menus.size()) { return r; }
    const Menu& m = d->menus[static_cast<std::size_t>(menu)];
    Umgebung u;
    fuelle(menu, u);
    const int n = static_cast<int>(m.items.size());
    auto kann = [&](int i) {   // Item_SetFocus: Pruefungen vor dem Setzen
        const Item& it = m.items[static_cast<std::size_t>(i)];
        const int flags = items[static_cast<std::size_t>(menu)][static_cast<std::size_t>(i)].flags;
        if ((flags & WINDOW_DECORATION) != 0 || !itemSichtbar(it, flags, u)) { return false; }
        if (d->dialekt == Dialekt::SP && (flags & WINDOW_INACTIVE) != 0) { return false; }
        return !((it.cvarFlags & (CVAR_ENABLE | CVAR_DISABLE)) != 0 && !itemZeigtUeberCvar(it, CVAR_ENABLE, u));
    };
    int cursor = -1;
    for (int schritt = 0; schritt <= n; ++schritt) {   // Menu_SetNextCursorItem
        bool wrapped = false;
        if (cursor == -1) {
            cursor = 0;
            wrapped = true;
        }
        int gefunden = -1;
        while (cursor < n) {
            ++cursor;
            if (cursor >= n && !wrapped) {
                wrapped = true;
                cursor = 0;
            }
            if (cursor < n && kann(cursor)) {
                gefunden = cursor;
                break;
            }
        }
        if (gefunden < 0 || std::find(r.begin(), r.end(), gefunden) != r.end()) { break; }
        r.push_back(gefunden);
    }
    return r;
}

// rc610 F46 B6-4: Menu_HandleMouseMove (OpenJK code/ui/ui_shared.cpp:9333-9430 SP, codemp/ui/ui_shared.c:6640-6713 MP;
// Movie Duels Update 9 gleich) mit Item_MouseEnter, Item_MouseLeave, Item_SetFocus und Menu_ClearFocus. Vorher bekam nur
// das oberste Element unter dem Zeiger mouseEnter und den Fokus, auch ein inaktives, und nie eine Dekoration.
void Laufzeit::maus(int menu, float x, float y) {
    if (d == nullptr || menu < 0 || static_cast<std::size_t>(menu) >= d->menus.size()) { return; }
    if ((menuFlags[static_cast<std::size_t>(menu)] & (WINDOW_VISIBLE | WINDOW_FORCED)) == 0) { return; }
    Umgebung u;
    fuelle(menu, u);
    const Menu& m = d->menus[static_cast<std::size_t>(menu)];
    auto& z = items[static_cast<std::size_t>(menu)];
    const bool sp = d->dialekt == Dialekt::SP;
    const auto cvarGesperrt = [&](const Item& it) {   // "items can be enabled and disabled based on cvars"
        return ((it.cvarFlags & (CVAR_ENABLE | CVAR_DISABLE)) != 0 && !itemZeigtUeberCvar(it, CVAR_ENABLE, u)) ||
               ((it.cvarFlags & (CVAR_SHOW | CVAR_HIDE)) != 0 && !itemZeigtUeberCvar(it, CVAR_SHOW, u));
    };
    // item->textRect beim Zeichnen; das MP rechnet hier mit r.y -= r.h, das SP nicht ("NOt sure why this is here")
    const auto textRectHier = [&](std::size_t i) {
        Rechteck r = textRectBei(menu, i, u);
        if (!sp) { r.y -= r.h; }
        return r;
    };
    const auto mausRaus = [&](std::size_t i) {   // Item_MouseLeave, Item_SetMouseOver(qfalse)
        if ((z[i].flags & WINDOW_MOUSEOVERTEXT) != 0) {
            fuehreAus(menu, static_cast<int>(i), m.items[i].mouseExitText);
            z[i].flags &= ~WINDOW_MOUSEOVERTEXT;
        }
        fuehreAus(menu, static_cast<int>(i), m.items[i].mouseExit);
        z[i].flags &= ~WINDOW_MOUSEOVER;
        ++version;
    };
    const auto mausRein = [&](std::size_t i) {   // Item_MouseEnter
        const Item& it = m.items[i];
        if (cvarGesperrt(it)) { return; }
        if (imRechteck(textRectHier(i), x, y)) {
            if ((z[i].flags & WINDOW_MOUSEOVERTEXT) == 0) {
                fuehreAus(menu, static_cast<int>(i), it.mouseEnterText);
                z[i].flags |= WINDOW_MOUSEOVERTEXT;
                ++version;
            }
        } else if ((z[i].flags & WINDOW_MOUSEOVERTEXT) != 0) {
            fuehreAus(menu, static_cast<int>(i), it.mouseExitText);
            z[i].flags &= ~WINDOW_MOUSEOVERTEXT;
            ++version;
        }
        if ((z[i].flags & WINDOW_MOUSEOVER) == 0) {
            fuehreAus(menu, static_cast<int>(i), it.mouseEnter);
            z[i].flags |= WINDOW_MOUSEOVER;
            ++version;
        }
    };
    const auto fokusWeg = [&]() {   // Menu_ClearFocus: SP leaveFocus nur beim Element mit Fokus, MP bei jedem
        int alt = -1;
        for (std::size_t i = 0; i < z.size() && i < m.items.size(); ++i) {
            const bool hatte = (z[i].flags & WINDOW_HASFOCUS) != 0;
            if (hatte) { alt = static_cast<int>(i); }
            z[i].flags &= ~WINDOW_HASFOCUS;
            if (hatte || !sp) { fuehreAus(menu, static_cast<int>(i), m.items[i].leaveFocus); }
        }
        return alt;
    };
    const auto fokus = [&](std::size_t i) {   // Item_SetFocus
        const Item& it = m.items[i];
        const int f = flagsGemalt(menu, i, u);
        if ((f & (WINDOW_DECORATION | WINDOW_HASFOCUS)) != 0 || (f & WINDOW_VISIBLE) == 0 || (sp && (f & WINDOW_INACTIVE) != 0)) { return false; }
        if (cvarGesperrt(it)) { return false; }
        const int alt = fokusWeg();
        ++version;
        bool klang = false;
        if (it.type == ITEM_TYPE_TEXT) {   // ein Text nimmt den Fokus nur in seinem textRect, sonst behaelt ihn der alte
            if (imRechteck(textRectHier(i), x, y)) {
                z[i].flags |= WINDOW_HASFOCUS;
                klang = true;
            } else if (alt >= 0) {
                z[static_cast<std::size_t>(alt)].flags |= WINDOW_HASFOCUS;
                fuehreAus(menu, alt, m.items[static_cast<std::size_t>(alt)].onFocus);
            }
        } else {
            z[i].flags |= WINDOW_HASFOCUS;
            fuehreAus(menu, static_cast<int>(i), it.onFocus);
            klang = true;
        }
        // focusSound des Items, sonst DC->Assets.itemFocusSound
        const std::string& fk = it.focusSound.empty() ? fokusKlangStd : it.focusSound;
        if (klang && !fk.empty()) { klaenge.push_back(fk); }
        return true;
    };
    // "this is the whole issue of focus vs. mouse over.. going through everything twice": im ersten Durchgang wirkt nur
    // das Verlassen, im zweiten mouseEnter und der Fokus fuer das erste, das ihn nimmt
    bool fokusGesetzt = false;
    for (int durchgang = 0; durchgang < 2; ++durchgang) {
        for (std::size_t i = 0; i < m.items.size() && i < z.size(); ++i) {
            const Item& it = m.items[i];
            const int f = flagsGemalt(menu, i, u);
            if ((f & (WINDOW_VISIBLE | WINDOW_FORCED)) == 0) { continue; }
            if (sp && (f & WINDOW_INACTIVE) != 0) { continue; }   // nur SP
            if (cvarGesperrt(it)) { continue; }
            if (imRechteck(rects[static_cast<std::size_t>(menu)][i], x, y)) {
                if (durchgang == 1 && (f & WINDOW_VISIBLE) != 0 && (f & WINDOW_FADINGOUT) == 0) {   // IsVisible
                    mausRein(i);
                    if (!fokusGesetzt) { fokusGesetzt = fokus(i); }
                }
            } else if ((z[i].flags & WINDOW_MOUSEOVER) != 0) {
                mausRaus(i);
            }
        }
    }
}

void Laufzeit::klick(int menu, float x, float y) {
    if (d == nullptr || menu < 0 || static_cast<std::size_t>(menu) >= d->menus.size()) { return; }
    Umgebung u;
    fuelle(menu, u);
    // rc610 F46 B6-4: Menu_HandleKey nimmt das (erste) Element mit dem Fokus; Item_HandleKey und A_MOUSE1 wirken nur,
    // wenn der Zeiger in seinem rect steht (code/ui/ui_shared.cpp Menu_HandleKey, codemp/ui/ui_shared.c ebenso)
    int i = -1;
    const auto& z = items[static_cast<std::size_t>(menu)];
    for (std::size_t k = 0; k < z.size() && i < 0; ++k) {
        if ((z[k].flags & WINDOW_HASFOCUS) != 0) { i = static_cast<int>(k); }
    }
    if (i < 0 || !imRechteck(rects[static_cast<std::size_t>(menu)][static_cast<std::size_t>(i)], x, y)) { return; }
    const Item& it = d->menus[static_cast<std::size_t>(menu)].items[static_cast<std::size_t>(i)];
    switch (it.type) {
        case ITEM_TYPE_YESNO:   // Item_YesNo_HandleKey
            if (!it.cvar.empty()) { setzeCvar(it.cvar, zahl(cvar(it.cvar)) != 0.0F ? "0" : "1"); }
            break;
        case ITEM_TYPE_MULTI: {   // Item_Multi_HandleKey: naechster Wert
            const std::size_t n = it.cvarList.size();
            if (n == 0 || it.cvar.empty()) { break; }
            std::size_t cur = 0;
            const std::string w = cvar(it.cvar);
            for (std::size_t k = 0; k < n; ++k) {
                if (it.strDef ? (k < it.cvarStr.size() && gleich(it.cvarStr[k], w))
                              : (k < it.cvarValue.size() && it.cvarValue[k] == zahl(w))) {
                    cur = k;
                }
            }
            const std::size_t nx = (cur + 1) % n;
            setzeCvar(it.cvar, it.strDef ? (nx < it.cvarStr.size() ? it.cvarStr[nx] : std::string()) : zahlText(it.cvarValue[nx]));
            break;
        }
        case ITEM_TYPE_SLIDER: {   // Item_Slider_HandleKey mit der Maus
            if (it.cvar.empty()) { break; }
            float x0 = it.window.rect.x;
            if (!it.text.empty()) {
                const Rechteck tr = textRechteck(it, u.texte != nullptr ? u.texte->imSpiel(it.text) : it.text, u);
                x0 = tr.x + tr.w + 8;
            }
            const float rel = std::clamp((x - x0) / sliderMasse(u.dialekt, d->spiel).breite, 0.0F, 1.0F);
            setzeCvar(it.cvar, zahlText(it.minVal + rel * (it.maxVal - it.minVal)));
            break;
        }
        case ITEM_TYPE_TEXTSCROLL: {   // M4: Item_TextScroll_HandleKey(A_MOUSE1) mit Item_TextScroll_OverLB
            Item ri = it;
            ri.window.rect = rects[static_cast<std::size_t>(menu)][static_cast<std::size_t>(i)];
            const int n = static_cast<int>(textScrollZeilen(ri, u).zeilen.size());
            if (static_cast<float>(n * it.lineHeight) <= ri.window.rect.h - 2) { break; }   // keine Leiste: nichts
            const Rechteck& r = ri.window.rect;
            const auto in = [&](float rx, float ry, float rw, float rh) { return x > rx && x < rx + rw && y > ry && y < ry + rh; };
            int& sp = items[static_cast<std::size_t>(menu)][static_cast<std::size_t>(i)].startPos;
            const int max = textScrollMaxScroll(ri, n);
            const int viewmax = it.lineHeight != 0 ? static_cast<int>(r.h / static_cast<float>(it.lineHeight)) : 0;
            const float lx = r.x + r.w - kScrollbar;
            const float daumen = textScrollDaumen(ri, n, sp);
            if (in(lx, r.y, kScrollbar, kScrollbar)) {
                sp = std::max(0, sp - 1);
            } else if (in(lx, r.y + r.h - kScrollbar, kScrollbar, kScrollbar)) {
                sp = std::min(max, sp + 1);
            } else if (in(lx, daumen, kScrollbar, kScrollbar)) {
                return;   // Daumen: die Engine faengt die Maus (Ziehen) - ein Klick allein bewegt nichts
            } else if (in(lx, r.y + kScrollbar, kScrollbar, daumen - (r.y + kScrollbar))) {
                sp = std::max(0, sp - viewmax);
            } else if (in(lx, daumen + kScrollbar, kScrollbar, r.y + r.h - kScrollbar)) {
                sp = std::min(max, sp + viewmax);
            } else {
                break;
            }
            ++version;
            return;   // Item_HandleKey hat die Taste genommen: keine Item_Action
        }
        default:
            break;
    }
    fuehreAus(menu, i, it.action);   // Item_Action
}

// M4: Mausrad ueber einer Textscroll mit Fokus - nur Movie Duels (Item_TextScroll_HandleKey,
// A_MWHEELUP/DOWN "Raz: Added"); OpenJK SP/MP kennen dort kein Mausrad.
void Laufzeit::rad(int menu, float x, float y, int schritte) {
    if (d == nullptr || d->spiel != Spiel::MD || menu < 0 || static_cast<std::size_t>(menu) >= d->menus.size() || schritte == 0) { return; }
    Umgebung u;
    fuelle(menu, u);
    const Menu& m = d->menus[static_cast<std::size_t>(menu)];
    for (std::size_t i = 0; i < m.items.size(); ++i) {
        ItemZustand& z = items[static_cast<std::size_t>(menu)][i];
        if (m.items[i].type != ITEM_TYPE_TEXTSCROLL || (z.flags & WINDOW_HASFOCUS) == 0) { continue; }
        Item ri = m.items[i];
        ri.window.rect = rects[static_cast<std::size_t>(menu)][i];
        const Rechteck& r = ri.window.rect;
        if (!(x > r.x && x < r.x + r.w && y > r.y && y < r.y + r.h)) { continue; }
        const int max = textScrollMaxScroll(ri, static_cast<int>(textScrollZeilen(ri, u).zeilen.size()));
        z.startPos = std::clamp(z.startPos - schritte, 0, max);   // rauf = -1 Zeile
        ++version;
    }
}

void Laufzeit::escape(int menu) {
    if (d == nullptr || menu < 0 || static_cast<std::size_t>(menu) >= d->menus.size()) { return; }
    fuehreAus(menu, -1, d->menus[static_cast<std::size_t>(menu)].onESC);
}

// Item_UpdatePosition -> Item_SetScreenCoords: window.rect = Menue-Ursprung
// (+ borderSize bei Rahmen) + rectClient
void Laufzeit::positionAus(int menu, std::size_t i) {
    const Menu& m = d->menus[static_cast<std::size_t>(menu)];
    const Item& it = m.items[i];
    const ItemAnimation& an = anim[static_cast<std::size_t>(menu)][i];
    float x = m.window.rect.x, y = m.window.rect.y;
    if (it.window.border != 0) {
        x += it.window.borderSize;
        y += it.window.borderSize;
    }
    rects[static_cast<std::size_t>(menu)][i] = Rechteck{x + an.client.x, y + an.client.y, an.client.w, an.client.h};
}

bool Laufzeit::animiert() const {
    for (const auto& z : items) {
        for (const ItemZustand& s : z) {
            if ((s.flags & (WINDOW_INTRANSITION | WINDOW_ORBITING | WINDOW_FADINGIN | WINDOW_FADINGOUT)) != 0) { return true; }
        }
    }
    return false;
}

int Laufzeit::takt(int jetzt) {
    if (d == nullptr) { return 0; }
    const int schritt = std::max(1, 1000 / std::max(1, spielFps));   // Com_Frame: min_msec = 1000 / com_maxfps
    if (letzterTakt < 0 || jetzt < letzterTakt) { letzterTakt = jetzt - schritt; }
    // nach einer langen Pause (Fenster verdeckt) nicht Tausende Bilder nachholen
    if (jetzt - letzterTakt > 2000) { letzterTakt = jetzt - 2000; }
    int bilder = 0;
    while (letzterTakt + schritt <= jetzt) {
        letzterTakt += schritt;
        jetztMs = letzterTakt;
        ++bilder;
        for (std::size_t mi = 0; mi < d->menus.size(); ++mi) {
            if ((menuFlags[mi] & WINDOW_VISIBLE) == 0) { continue; }   // Menu_PaintAll: nur sichtbare Menues
            const Menu& m = d->menus[mi];
            // Menu_Init: Werte aus assetGlobalDef, Schluessel des Menues ueberschreiben
            const float fadeClamp = bhed::menue::finde(m, "fadeClamp") != nullptr ? m.fadeClamp : fadeClampStd;
            const float fadeAmount = bhed::menue::finde(m, "fadeAmount") != nullptr ? m.fadeAmount : fadeAmountStd;
            const int fadeCycle = bhed::menue::finde(m, "fadeCycle") != nullptr ? m.fadeCycle : fadeCycleStd;
            auto& z = items[mi];
            for (std::size_t i = 0; i < m.items.size(); ++i) {
                ItemAnimation& an = anim[mi][i];
                int& fl = z[i].flags;
                const Item& it = m.items[i];
                if ((fl & WINDOW_ORBITING) != 0 && jetztMs > an.nextTime) {
                    an.nextTime = jetztMs + an.offsetTime;
                    const float w = an.client.w / 2;
                    const float h = an.client.h / 2;
                    const float rx = an.client.x + w - an.effects.x;
                    const float ry = an.client.y + h - an.effects.y;
                    const float a = 3.0F * 3.14159265358979323846F / 180.0F;
                    const float c = std::cos(a);
                    const float s = std::sin(a);
                    an.client.x = (rx * c - ry * s) + an.effects.x - w;
                    an.client.y = (rx * s + ry * c) + an.effects.y - h;
                    positionAus(static_cast<int>(mi), i);
                    ++version;
                }
                if ((fl & WINDOW_INTRANSITION) != 0 && jetztMs > an.nextTime) {
                    int done = 0;
                    an.nextTime = jetztMs + an.offsetTime;
                    auto schritt1 = [&](float& v, float ziel, float d2) {
                        if (v == ziel) {
                            done++;
                        } else if (v < ziel) {
                            v += d2;
                            if (v > ziel) {
                                v = ziel;
                                done++;
                            }
                        } else {
                            v -= d2;
                            if (v < ziel) {
                                v = ziel;
                                done++;
                            }
                        }
                    };
                    schritt1(an.client.x, an.effects.x, an.effects2.x);
                    schritt1(an.client.y, an.effects.y, an.effects2.y);
                    schritt1(an.client.w, an.effects.w, an.effects2.w);
                    schritt1(an.client.h, an.effects.h, an.effects2.h);
                    positionAus(static_cast<int>(mi), i);
                    if (done == 4) { fl &= ~WINDOW_INTRANSITION; }
                    ++version;
                }
                // M4: Item_Paint -> Window_Paint: WINDOW_STYLE_FILLED mit background blendet ueber
                // backColor[3] (Fade mit bFlags, derselbe nextTime wie danach Item_TextColor)
                if ((fl & (WINDOW_FADINGOUT | WINDOW_FADINGIN)) != 0 && (fl & WINDOW_VISIBLE) != 0 && it.window.style == WINDOW_STYLE_FILLED &&
                    !it.window.background.empty() && jetztMs > an.nextTime) {
                    Farbe f = hintenGesetzt[mi][i] ? hinten[mi][i] : it.window.backColor;
                    an.nextTime = jetztMs + fadeCycle;
                    if ((fl & WINDOW_FADINGOUT) != 0) {
                        f.a -= fadeAmount;
                        if (f.a <= 0.0F) { fl &= ~(WINDOW_FADINGOUT | WINDOW_VISIBLE); }
                    } else {
                        f.a += fadeAmount;
                        if (f.a >= fadeClamp) {
                            f.a = fadeClamp;
                            fl &= ~WINDOW_FADINGIN;
                        }
                    }
                    hinten[mi][i] = f;
                    hintenGesetzt[mi][i] = true;
                    ++version;
                }
                // Item_Paint: ownerDrawFlags/cvarTest/WINDOW_VISIBLE, dann Item_TextColor -> Fade
                if ((fl & (WINDOW_FADINGOUT | WINDOW_FADINGIN)) != 0 && (fl & WINDOW_VISIBLE) != 0 &&
                    (!it.text.empty() || it.type == ITEM_TYPE_OWNERDRAW) && jetztMs > an.nextTime) {
                    if (!an.alphaGesetzt) {
                        an.alpha = vorderGesetzt[mi][i] ? vorder[mi][i].a : it.window.foreColor.a;
                        an.alphaGesetzt = true;
                    }
                    an.nextTime = jetztMs + fadeCycle;
                    if ((fl & WINDOW_FADINGOUT) != 0) {
                        an.alpha -= fadeAmount;
                        if (an.alpha <= 0.0F) { fl &= ~(WINDOW_FADINGOUT | WINDOW_VISIBLE); }
                    } else {
                        an.alpha += fadeAmount;
                        if (an.alpha >= fadeClamp) {
                            an.alpha = fadeClamp;
                            fl &= ~WINDOW_FADINGIN;
                        }
                    }
                    Farbe f = vorderGesetzt[mi][i] ? vorder[mi][i] : it.window.foreColor;
                    f.a = an.alpha;
                    vorder[mi][i] = f;
                    vorderGesetzt[mi][i] = true;
                    ++version;
                }
            }
        }
    }
    return bilder;
}

// UI_RunMenuScript (code/ui/ui_main.cpp) fuer die Figuren- und Saeber-Skripte. Die Engine
// sucht dabei die Items fest ueber ihre Namen "character", "saber" und "saber2" im Menue mit
// dem Fokus (Menu_FindItemByName in UI_UpdateCharacter/UI_UpdateCharacterSkin/
// UI_UpdateSaberHilt) - das ist dort so verdrahtet und wird hier genau so uebernommen.
bool Laufzeit::uiSkript(int menu, const std::string& name) {
    auto setze = [&](const char* ziel, const char* quelle) { setzeCvar(ziel, cvar(quelle)); };
    auto itemNamens = [&](const char* n) {
        const Menu& m = d->menus[static_cast<std::size_t>(menu)];
        for (std::size_t i = 0; i < m.items.size(); ++i) {
            if (gleich(m.items[i].window.name, n)) { return static_cast<int>(i); }
        }
        return -1;
    };
    auto haut = [&]() {   // UI_UpdateCharacterSkin
        const int i = itemNamens("character");
        if (i < 0) { return; }
        modellHaut[static_cast<std::size_t>(menu)][static_cast<std::size_t>(i)] =
            "models/players/" + cvar("ui_char_model") + "/|" + cvar("ui_char_skin_head") + "|" + cvar("ui_char_skin_torso") + "|" +
            cvar("ui_char_skin_legs");
        ++version;
    };
    auto figur = [&]() {   // UI_UpdateCharacter
        const int i = itemNamens("character");
        if (i < 0) { return; }
        modellAnim[static_cast<std::size_t>(menu)][static_cast<std::size_t>(i)] = cvar("ui_char_anim");
        animGesetzt[static_cast<std::size_t>(menu)][static_cast<std::size_t>(i)] = jetztMs;
        modellPfad[static_cast<std::size_t>(menu)][static_cast<std::size_t>(i)] = "models/players/" + cvar("ui_char_model") + "/model.glm";
        haut();
    };
    auto griff = [&](bool zweiter) {   // UI_UpdateSaberHilt
        const int i = itemNamens(zweiter ? "saber2" : "saber");
        std::string modell, h;
        if (i < 0 || !saberModell || !saberModell(cvar(zweiter ? "ui_saber2" : "ui_saber"), modell, h)) { return; }
        modellPfad[static_cast<std::size_t>(menu)][static_cast<std::size_t>(i)] = modell;
        modellHaut[static_cast<std::size_t>(menu)][static_cast<std::size_t>(i)] = h.empty() ? std::string("-") : h;
        ++version;
    };
    if (name == "updateforcestatus") {
        // UpdateForceStatus (codemp/ui/ui_main.c), ohne Partie: CS_SERVERINFO leer, g_gametype 0. rc608: fehlte; darum standen in
        // ingame_player "The Force" und "Force Disabled", Apply und Join Game aufeinander.
        const auto zeigen = [&](const char* gruppe, bool an) {
            const Menu& m = d->menus[static_cast<std::size_t>(menu)];
            for (std::size_t i = 0; i < m.items.size(); ++i) {
                if (gleich(m.items[i].window.name, gruppe) || (!m.items[i].window.group.empty() && gleich(m.items[i].window.group, gruppe))) {
                    if (an) { items[static_cast<std::size_t>(menu)][i].flags |= WINDOW_VISIBLE; }
                    else { items[static_cast<std::size_t>(menu)][i].flags &= ~WINDOW_VISIBLE; }
                }
            }
        };
        if (gleich(d->menus[static_cast<std::size_t>(menu)].window.name, "ingame_player")) {
            const bool spektator = std::atoi(cvar("ui_myteam").c_str()) == 3;
            zeigen("jedinonjedi", false);
            zeigen("noforce", false);
            zeigen("yesforce", true);
            zeigen("nosaber", false);
            zeigen("yessaber", true);
            zeigen("playerapply", !spektator);
            zeigen("playerforcejoin", spektator);
            zeigen("playerforcered", !spektator);
            zeigen("playerforceblue", !spektator);
            zeigen("playerforcespectate", true);
            ++version;
        }
        // jk2mp (ui.qvm) legt ui_about_gametype dabei an; ohne ihn gelten die "hideCvar 0; 1 ..."-Elemente als sichtbar
        if (d->spiel == Spiel::JK2 && cvar("ui_about_gametype").empty()) { setzeCvar("ui_about_gametype", "0"); }
    }
    else if (name == "initallocforcepower" && d->dialekt == Dialekt::SP) {
        // UI_InitAllocForcePowers -> UI_ForcePowerWeaponsButton(qfalse): ohne Partie sind keine Kraefte verteilt
        const int i = itemNamens("weaponbutton");
        if (i >= 0) { items[static_cast<std::size_t>(menu)][static_cast<std::size_t>(i)].flags |= WINDOW_INACTIVE; }
        ++version;
    }
    else if (name == "getvideosetup") {
        // UI_GetVideoSetup (code/ui/ui_main.cpp, codemp/ui/ui_main.c): die Setup-Seiten zeigen Kopien der r_-Cvars.
        // rc608: fehlte; in Jedi Outcast stand darum "High Quality" und "Off" statt "Custom" und "On".
        if (cvar("ui_r_glCustom").empty()) { setzeCvar("ui_r_glCustom", "4"); }
        for (const char* teil : {"mode", "colorbits", "fullscreen", "lodbias", "picmip", "texturebits", "texturemode", "detailtextures",
                                 "ext_compress_textures", "depthbits", "subdivisions", "fastSky", "inGameVideo", "allowExtensions"}) {
            setzeCvar(std::string("ui_r_") + teil, cvar(std::string("r_") + teil));
        }
        setzeCvar("ui_r_modified", "0");
    }
    else if (name == "character") { figur(); }
    else if (name == "characterchanged") { figur(); }
    else if (name == "char_skin") { haut(); }
    else if (name == "saber_hilt") { griff(false); }
    else if (name == "saber2_hilt") { griff(true); }
    else if (name == "getcharcvars" && d->dialekt == Dialekt::MP) {   // UI_GetCharacterCvars (codemp ui_main.c)
        setze("ui_char_color_red", "char_color_red");
        setze("ui_char_color_green", "char_color_green");
        setze("ui_char_color_blue", "char_color_blue");
        // "model" = <ordner>/<kopf>|<rumpf>|<beine> (eigene Figur) -> ui_char_*; sonst bleibt ui_char_model
        const std::string mod = cvar("model");
        const std::size_t st = mod.rfind('/');
        if (st != std::string::npos && mod.find('|') != std::string::npos) {
            const std::string teile = mod.substr(st + 1);
            const std::size_t a = teile.find('|'), b = a == std::string::npos ? std::string::npos : teile.find('|', a + 1);
            if (b != std::string::npos) {
                setzeCvar("ui_char_model", mod.substr(0, st));
                setzeCvar("ui_char_skin_head", teile.substr(0, a));
                setzeCvar("ui_char_skin_torso", teile.substr(a + 1, b - a - 1));
                setzeCvar("ui_char_skin_legs", teile.substr(b + 1));
            }
        }
    } else if (name == "getcharcvars") {   // UI_GetCharacterCvars
        setze("ui_char_skin_head", "g_char_skin_head");
        setze("ui_char_skin_torso", "g_char_skin_torso");
        setze("ui_char_skin_legs", "g_char_skin_legs");
        setze("ui_char_color_red", "g_char_color_red");
        setze("ui_char_color_green", "g_char_color_green");
        setze("ui_char_color_blue", "g_char_color_blue");
        setze("ui_char_model", "g_char_model");
    } else if (name == "getsabercvars") {   // UI_GetSaberCvars
        setze("ui_saber_type", "g_saber_type");
        setze("ui_saber", "g_saber");
        setze("ui_saber2", "g_saber2");
        setze("ui_saber_color", "g_saber_color");
        setze("ui_saber2_color", "g_saber2_color");
        setzeCvar("ui_newfightingstyle", "0");
    } else if (name == "resetsabercvardefaults") {   // nur, wenn saber_menu nicht gesetzt ist
        if (std::atoi(cvar("saber_menu").c_str()) == 0) {   // UI_ResetSaberCvars
            setzeCvar("g_saber_type", "single");
            setzeCvar("g_saber", "single_1");
            setzeCvar("g_saber2", "");
            setzeCvar("ui_saber_type", "single");
            setzeCvar("ui_saber", "single_1");
            setzeCvar("ui_saber2", "");
        }
    } else {
        return false;
    }
    return true;
}

}  // namespace bhed::menue
