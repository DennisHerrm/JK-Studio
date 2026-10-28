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
    return it == cvars.end() ? std::string() : it->second;
}

void Laufzeit::setzeCvar(const std::string& n, const std::string& w) {
    cvars[klein(n)] = w;
    ++version;
}

void Laufzeit::oeffne(const std::string& name) {   // Menus_ActivateByName + Menus_Activate
    const int m = finde(name);
    if (m < 0) {
        protokoll.push_back("open " + name + ": kein menuDef mit diesem Namen in dieser Datei");
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
    std::size_t p = 0;
    std::string cmd;
    int schutz = 0;
    while (stringParse(s, p, cmd) && ++schutz < 1000) {
        if (cmd == ";") { continue; }
        const std::string c = klein(cmd);
        std::string a, b;
        if (c == "show" || c == "hide" || c == "fadein" || c == "fadeout") {
            if (stringParse(s, p, a)) {
                const bool zeigen = (c == "show" || c == "fadein");
                passend(a, [&](std::size_t i) {
                    if (zeigen) { z[i].flags |= WINDOW_VISIBLE; }
                    else { z[i].flags &= ~WINDOW_VISIBLE; }
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
                if (gleich(feld, "forecolor")) {
                    passend(a, [&](std::size_t i) {
                        vorder[static_cast<std::size_t>(menu)][i] = Farbe{zahl(r), zahl(g), zahl(bl), zahl(al)};
                        vorderGesetzt[static_cast<std::size_t>(menu)][i] = true;
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
                passend(a, [&](std::size_t i) {
                    rects[static_cast<std::size_t>(menu)][i] = Rechteck{m.window.rect.x + zahl(x), m.window.rect.y + zahl(y), zahl(w), zahl(h)};
                });
                ++version;
            }
        } else if (c == "transition" || c == "transition2" || c == "transition3") {
            // Menu_TransitionItemByName: hier sofort am Ziel (rectTo)
            std::string v[10];
            bool ok = stringParse(s, p, a);
            for (int i = 0; ok && i < 10; ++i) { ok = stringParse(s, p, v[i]); }
            if (ok) {
                passend(a, [&](std::size_t i) {
                    z[i].flags |= WINDOW_VISIBLE;
                    rects[static_cast<std::size_t>(menu)][i] =
                        Rechteck{m.window.rect.x + zahl(v[4]), m.window.rect.y + zahl(v[5]), zahl(v[6]), zahl(v[7])};
                });
                ++version;
            }
        } else if (c == "play" || c == "playlooped" || c == "playvoice") {
            if (stringParse(s, p, a)) { melde(cmd + " " + a); }
        } else if (c == "exec" || c == "uiscript" || c == "setitembackground" || c == "setasset" || c == "orbit" ||
                   c == "setplayermodel" || c == "setteamcolor" || c == "setitemflag" || c == "defer" || c == "rundeferred" ||
                   c == "delay" || c == "activate" || c == "showmenu" || c == "opengotomenu" || c == "stopvoice" ||
                   c == "incrementfeeder" || c == "decrementfeeder" || c == "disable" || c == "scale" ||
                   c == "setitemcolorcvar" || c == "setitemrectcvar" || c == "setitemexec") {
            // ohne Spiel nicht ausfuehrbar: bis zum ; melden
            std::string rest = cmd;
            while (stringParse(s, p, a) && a != ";") { rest += " " + a; }
            melde(rest);
            if (c == "showmenu" || c == "opengotomenu") {
                const std::size_t leer = rest.find(' ');
                if (leer != std::string::npos) { oeffne(rest.substr(leer + 1, rest.find(' ', leer + 1) - leer - 1)); }
            }
        } else {
            // DC->runScript: der Rest bis zum ; geht an UI_RunMenuScript
            std::string rest = cmd;
            while (stringParse(s, p, a) && a != ";") { rest += " " + a; }
            melde("uiScript: " + rest);
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
            if (vorderGesetzt[mi][i]) { m.items[i].window.foreColor = vorder[mi][i]; }
        }
    }
}

int Laufzeit::itemBei(int menu, float x, float y, const Umgebung& u) const {
    if (d == nullptr || menu < 0 || static_cast<std::size_t>(menu) >= d->menus.size()) { return -1; }
    const Menu& m = d->menus[static_cast<std::size_t>(menu)];
    for (std::size_t i = m.items.size(); i-- > 0;) {
        const Item& it = m.items[i];
        const int flags = items[static_cast<std::size_t>(menu)][i].flags;
        const Rechteck& r = rects[static_cast<std::size_t>(menu)][i];
        if ((flags & WINDOW_DECORATION) != 0 || !itemSichtbar(it, flags, u)) { continue; }   // Item_SetFocus
        if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h) { return static_cast<int>(i); }
    }
    return -1;
}

void Laufzeit::maus(int menu, float x, float y) {
    if (d == nullptr || menu < 0 || static_cast<std::size_t>(menu) >= d->menus.size()) { return; }
    Umgebung u;
    fuelle(menu, u);
    const int neu = itemBei(menu, x, y, u);
    const Menu& m = d->menus[static_cast<std::size_t>(menu)];
    auto& z = items[static_cast<std::size_t>(menu)];
    for (std::size_t i = 0; i < z.size(); ++i) {
        const bool war = (z[i].flags & WINDOW_MOUSEOVER) != 0;
        const bool ist = static_cast<int>(i) == neu;
        if (war && !ist) {
            z[i].flags &= ~(WINDOW_MOUSEOVER | WINDOW_HASFOCUS);
            ++version;
            fuehreAus(menu, static_cast<int>(i), m.items[i].mouseExit);
            fuehreAus(menu, static_cast<int>(i), m.items[i].leaveFocus);
        } else if (!war && ist) {
            for (ItemZustand& q : z) { q.flags &= ~WINDOW_HASFOCUS; }
            z[i].flags |= WINDOW_MOUSEOVER | WINDOW_HASFOCUS;
            ++version;
            fuehreAus(menu, static_cast<int>(i), m.items[i].mouseEnter);
            fuehreAus(menu, static_cast<int>(i), m.items[i].onFocus);
        }
    }
}

void Laufzeit::klick(int menu, float x, float y) {
    if (d == nullptr || menu < 0 || static_cast<std::size_t>(menu) >= d->menus.size()) { return; }
    Umgebung u;
    fuelle(menu, u);
    const int i = itemBei(menu, x, y, u);
    if (i < 0) { return; }
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
                const Rechteck tr = textRechteck(it, u.texte != nullptr ? u.texte->aufloesen(it.text) : it.text, u);
                x0 = tr.x + tr.w + 8;
            }
            const float rel = std::clamp((x - x0) / kSliderBreite, 0.0F, 1.0F);
            setzeCvar(it.cvar, zahlText(it.minVal + rel * (it.maxVal - it.minVal)));
            break;
        }
        default:
            break;
    }
    fuehreAus(menu, i, it.action);   // Item_Action
}

void Laufzeit::escape(int menu) {
    if (d == nullptr || menu < 0 || static_cast<std::size_t>(menu) >= d->menus.size()) { return; }
    fuehreAus(menu, -1, d->menus[static_cast<std::size_t>(menu)].onESC);
}

}  // namespace bhed::menue
