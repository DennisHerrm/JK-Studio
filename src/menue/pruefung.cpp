// pruefung.cpp - Menue-Editor M2 Punkt 7: Pruefungen vor dem Export.
// Portiert aus OpenJK (die Grenzen aus code/ui/ui_shared.h, codemp/ui/ui_shared.h, Movie Duels); die Pruefungen sind eigener Code.
// Grenzwerte aus OpenJK/Movie Duels (siehe include/bhed/menue/pruefung.h),
// die Pruefungen sind eigener Code.
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
#include "bhed/menue/pruefung.h"

#include <algorithm>
#include <cctype>
#include <set>

namespace bhed::menue {

namespace {
std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}
const char* fassungsName(Dialekt d, Spiel s) {
    if (s == Spiel::JK2) { return d == Dialekt::SP ? "JK2 SP" : "JK2 MP"; }
    if (s == Spiel::MD) { return d == Dialekt::SP ? "MD SP" : "MD MP"; }
    return d == Dialekt::SP ? "JA SP" : "JA MP";
}
// Items und Gruppen, die der Code der Engine selbst ueber ihren Namen anfasst (zeigt, verbirgt,
// fuellt): OpenJK code/ui/ui_main.cpp + ui_shared.cpp, codemp/ui/ui_main.c + ui_shared.c,
// Movie Duels code/ui/ui_main.cpp + ui_shared.cpp (Menu_FindItemByName, Menu_ShowItemByName,
// Menu_ShowGroup ... mit festem Namen). Von der .menu aus nicht zu sehen - kein "nie sichtbar".
// "*_fbutton": va("%s_fbutton", powerEnums[...].title).
const char* const kEngineNamen[] = {
    "absorb_fbutton", "allocate_text", "allocated_text", "applyChanges", "att_01", "att_02", "att_03", "attack", "bad_server",
    "base_class_force_feed", "base_class_inventory_feed", "base_class_weapons_feed", "beginmission", "box2bottom", "box2middle",
    "box3bottom", "box3middle", "char_desc", "char_name", "char_title", "character", "chosenpistol1_button", "chosenpistol1_icon",
    "chosenthrowweapon_button", "chosenthrowweapon_icon", "chosenweapon1_icon", "chosenweapon2_icon", "class_button", "class_count",
    "classdescription", "colorbox", "darkpowers", "darkpowers_team", "deallocate_fbutton", "def_01", "def_02", "def_03", "def_04",
    "defend", "description", "duelmapdesc", "feeders", "forcealloc", "forcepowerlevel", "headlistbox", "hiltbut", "hiltbut1",
    "hiltbut2", "hiltbut_staves", "humanbotfield", "humanbotnonfield", "ingame_player", "ingame_playerforce", "item_desc",
    "itemdescription", "jedinonjedi", "lightpowers", "lightpowers_team", "loadgamelist", "lowerlistbox", "maplist", "minidesc",
    "modellist", "noforce", "nosaber", "obj_longdesc", "objective_pic", "onteam1", "onteam2", "password_request", "playerapply",
    "playerforceblue", "playerforcejoin", "playerforcered", "playerforcespectate", "rep_01", "rep_02", "rep_03", "rep_04", "rep_05",
    "reply", "req_01", "req_02", "req_03", "req_04", "req_05", "req_06", "request", "sabernoneconfigme", "singlesaberhiltslist",
    "singlesaberhiltslist2", "spot", "spot_01", "spot_02", "spot_03", "spot_04", "spot_05", "staffsaberhiltslist", "stats", "tac_01",
    "tac_02", "tac_03", "tac_04", "tac_05", "tac_06", "tactics", "torsolistbox", "variantlist", "weaponbutton", "yesforce", "yessaber"};
bool engineName(const std::string& n) {
    if (n.empty()) { return false; }
    const std::string k = klein(n);
    // Com_sprintf-Namen: "%s_fbutton", "%s_hexpic", "%s_iconpic", "%s_level%ddesc"
    for (const char* s : {"_fbutton", "_hexpic", "_iconpic", "desc"}) {
        const std::size_t n2 = std::char_traits<char>::length(s);
        if (k.size() <= n2 || k.compare(k.size() - n2, n2, s) != 0) { continue; }
        if (std::string(s) != "desc") { return true; }
        const std::size_t lv = k.rfind("_level");
        if (lv != std::string::npos && lv + 6 < k.size() - 4 && std::isdigit(static_cast<unsigned char>(k[lv + 6])) != 0) { return true; }
    }
    for (const char* e : kEngineNamen) {
        if (k == klein(e)) { return true; }
    }
    return false;
}
// Klaenge: S_RegisterSound -> S_LoadSound_Actual nimmt ohne .wav die .mp3 (OpenJK
// code/client/snd_mem.cpp: COM_StripExtension, erst .wav, dann .mp3).
bool klangDa(const PruefKontext& k, const std::string& name) {
    if (!k.gibt || k.gibt(name)) { return true; }
    std::string b = name;
    const std::size_t sl = b.find_last_of("/\\");
    const std::size_t pt = b.rfind('.');
    if (pt != std::string::npos && (sl == std::string::npos || pt > sl)) { b = b.substr(0, pt); }
    return k.gibt(b + ".wav") || k.gibt(b + ".mp3");
}
// Ziel wie Menu_ItemsMatchingGroup: Name oder Gruppe ohne Gross/Klein, "abc*" = Anfang mit Gross/Klein
bool trifft(const std::string& ziel, const Item& it) {
    const std::size_t stern = ziel.find('*');
    if (stern != std::string::npos) {
        return (it.window.name.size() >= stern && it.window.name.compare(0, stern, ziel, 0, stern) == 0) ||
               (it.window.group.size() >= stern && it.window.group.compare(0, stern, ziel, 0, stern) == 0);
    }
    return klein(it.window.name) == klein(ziel) || (!it.window.group.empty() && klein(it.window.group) == klein(ziel));
}

// Die Woerter eines Skripts (wie Item_RunScript sie liest: durch Leerzeichen getrennt, "..." am Stueck)
std::vector<std::string> skriptWoerter(const std::string& s) {
    std::vector<std::string> w;
    std::size_t i = 0;
    while (i < s.size()) {
        while (i < s.size() && static_cast<unsigned char>(s[i]) <= ' ') { ++i; }
        if (i >= s.size()) { break; }
        std::string x;
        if (s[i] == '"') {
            ++i;
            while (i < s.size() && s[i] != '"') { x += s[i++]; }
            ++i;
        } else {
            while (i < s.size() && static_cast<unsigned char>(s[i]) > ' ' && s[i] != ';') { x += s[i++]; }
            if (x.empty()) { x = s[i++]; }
        }
        w.push_back(x);
    }
    return w;
}
}  // namespace

Grenzen grenzenFuer(Dialekt d, Spiel s) {
    if (d == Dialekt::MP) { return Grenzen{512, 128, 2048, 64, 10, 16}; }   // codemp/ui/ui_shared.h, PC_Script_Parse 2048
    if (s == Spiel::MD) { return Grenzen{4096, 256, 4096, 128, 10, 16}; }   // Movie Duels code/ui/ui_shared.h
    return Grenzen{150, 64, 4096, 64, 10, 16};                               // code/ui/ui_shared.h, PC_Script_Parse 4096
}

std::vector<Befund> pruefeFuerSpiel(const Datei& d, const PruefKontext& k) {
    std::vector<Befund> r;
    const Grenzen g = grenzenFuer(d.dialekt, d.spiel);
    auto melde = [&](BefundArt art, int menu, int item, int a, int b, std::string name, std::string wert, bool fehler) {
        Befund x;
        x.art = art;
        x.menu = menu;
        x.item = item;
        x.a = a;
        x.b = b;
        x.name = std::move(name);
        x.wert = std::move(wert);
        x.fehler = fehler;
        r.push_back(std::move(x));
    };
    if (static_cast<int>(d.menus.size()) > g.menus) { melde(BefundArt::ZuVieleMenues, -1, -1, static_cast<int>(d.menus.size()), g.menus, {}, {}, true); }
    for (std::size_t mi = 0; mi < d.menus.size(); ++mi) {
        const Menu& m = d.menus[mi];
        const int mn = static_cast<int>(mi);
        // Die Skripte dieses Menues (Script_Show & Co. suchen nur im eigenen Menue): wen zeigen
        // sie (show, fadein), wen bewegen sie (setitemrect, transition, orbit)?
        std::vector<std::string> gezeigt, bewegt;
        {
            std::vector<const std::string*> skripte = {&m.onOpen, &m.onClose, &m.onESC, &m.onAccept};
            for (const Item& it : m.items) {
                for (const std::string* s : {&it.action, &it.onFocus, &it.leaveFocus, &it.mouseEnter, &it.mouseExit, &it.mouseEnterText,
                                             &it.mouseExitText, &it.accept, &it.selectNext, &it.selectPrev, &it.doubleClick}) {
                    skripte.push_back(s);
                }
            }
            for (const std::string* s : skripte) {
                const std::vector<std::string> w = skriptWoerter(*s);
                for (std::size_t i = 0; i + 1 < w.size(); ++i) {
                    const std::string c = klein(w[i]);
                    // uiScript <befehl> <namen...>: die Engine bekommt die Namen und fasst die Items an
                    if (c == "uiscript") {
                        for (std::size_t j = i + 2; j < w.size() && w[j] != ";"; ++j) {
                            gezeigt.push_back(w[j]);
                            bewegt.push_back(w[j]);
                        }
                    }
                    if (c == "show" || c == "fadein") { gezeigt.push_back(w[i + 1]); }
                    if (c == "setitemrect" || c == "transition" || c == "transition2" || c == "orbit") {
                        gezeigt.push_back(w[i + 1]);
                        bewegt.push_back(w[i + 1]);
                    }
                }
            }
        }
        auto inListe = [](const std::vector<std::string>& l, const Item& it) {
            return std::any_of(l.begin(), l.end(), [&](const std::string& z) { return trifft(z, it); });
        };
        // play / playlooped mit der Stelle (je Item bzw. Menue)
        auto klaenge = [&](int item, const std::vector<Eigenschaft>& eig) {
            for (const Eigenschaft& e : eig) {
                if (e.args.empty() || e.args.front().s != "{") { continue; }
                for (std::size_t i = 1; i + 1 < e.args.size(); ++i) {
                    const std::string c = klein(e.args[i].s);
                    if ((c == "play" || c == "playlooped") && !klangDa(k, e.args[i + 1].s)) {
                        melde(BefundArt::DateiFehlt, mn, item, 0, 0, e.args[i + 1].s, c, false);
                    }
                }
            }
        };
        klaenge(-1, m.eigenschaften);
        if (static_cast<int>(m.items.size()) > g.menuItems) {
            melde(BefundArt::ZuVieleItems, mn, -1, static_cast<int>(m.items.size()), g.menuItems, m.window.name, {}, true);
        }
        auto skript = [&](int item, const std::vector<Eigenschaft>& eig) {
            for (const Eigenschaft& e : eig) {
                if (e.args.empty() || e.args.front().s != "{") { continue; }
                // PC_Script_Parse: je Wort + " ", Woerter laenger als 1 in "" (Q_strcat bis zur Grenze)
                std::size_t n = 0;
                for (std::size_t i = 1; i + 1 < e.args.size(); ++i) { n += e.args[i].s.size() + (e.args[i].s.size() > 1 ? 2 : 0) + 1; }
                if (static_cast<int>(n) >= g.skript) { melde(BefundArt::SkriptZuLang, mn, item, static_cast<int>(n), g.skript, e.schluessel, {}, true); }
            }
        };
        skript(-1, m.eigenschaften);
        if (!m.font.empty() && k.gibt && !k.gibt("fonts/" + m.font + ".fontdat")) {   // RE_RegisterFont: fonts/%s.fontdat
            melde(BefundArt::DateiFehlt, mn, -1, 0, 0, m.font, "font", false);
        }
        if (!m.soundName.empty() && !klangDa(k, m.soundName)) { melde(BefundArt::DateiFehlt, mn, -1, 0, 0, m.soundName, "soundLoop", false); }
        if (!m.window.background.empty() && k.gibtBild && !k.gibtBild(m.window.background)) {
            melde(BefundArt::DateiFehlt, mn, -1, 0, 0, m.window.background, "background", false);
        }
        for (std::size_t ii = 0; ii < m.items.size(); ++ii) {
            const Item& it = m.items[ii];
            const int in = static_cast<int>(ii);
            skript(in, it.eigenschaften);
            klaenge(in, it.eigenschaften);
            if (static_cast<int>(it.cvarList.size()) > g.multiCvars) {
                melde(BefundArt::ZuVieleWerte, mn, in, static_cast<int>(it.cvarList.size()), g.multiCvars, it.window.name, {}, true);
            }
            if (static_cast<int>(it.colorRanges.size()) > g.farbBereiche) {
                melde(BefundArt::ZuVieleFarben, mn, in, static_cast<int>(it.colorRanges.size()), g.farbBereiche, it.window.name, {}, true);
            }
            if (static_cast<int>(it.columns.size()) > g.spalten) {
                melde(BefundArt::ZuVieleSpalten, mn, in, static_cast<int>(it.columns.size()), g.spalten, it.window.name, {}, true);
            }
            // nie zu sehen: etwas zum Zeichnen, aber kein Platz auf dem Schirm, oder visible 0 ohne Skript.
            // Text zeichnet auch ohne Groesse (an rect.x/y); bewegte Items (setitemrect, transition,
            // orbit) und solche, die der Code der Engine anfasst, bleiben aussen vor.
            const Rechteck& q = it.window.rect;
            const bool text = !it.text.empty();
            const bool zeichnet = it.window.style != WINDOW_STYLE_EMPTY || text || it.window.border != 0 || it.type == ITEM_TYPE_MODEL;
            const bool engine = engineName(it.window.name) || engineName(it.window.group);
            const bool draussen = q.x >= 640 || q.y >= 480 || (!text && (q.w <= 0 || q.h <= 0 || q.x + q.w <= 0 || q.y + q.h <= 0));
            if (zeichnet && draussen && !engine && !inListe(bewegt, it)) {
                melde(BefundArt::NieSichtbar, mn, in, 0, 0, "rect", it.window.name, false);
            }
            if (zeichnet && (it.window.flags & WINDOW_VISIBLE) == 0 && it.cvarTest.empty() && !engine && !inListe(gezeigt, it)) {
                melde(BefundArt::NieSichtbar, mn, in, 0, 0, "visible", it.window.name, false);
            }
            // fehlende Dateien
            if (!it.window.background.empty() && k.gibtBild && !k.gibtBild(it.window.background)) {
                melde(BefundArt::DateiFehlt, mn, in, 0, 0, it.window.background, "background", false);
            }
            if (!it.assetModel.empty() && klein(it.assetModel) != "ui_char_model" && k.gibt && !k.gibt(it.assetModel)) {   // ItemParse_asset_model
                melde(BefundArt::DateiFehlt, mn, in, 0, 0, it.assetModel, "asset_model", false);
            }
            if (!it.window.cinematicName.empty() && k.gibt) {   // CIN_PlayCinematic: ohne / -> video/, Endung .roq
                std::string v = it.window.cinematicName;
                if (v.find('/') == std::string::npos && v.find('\\') == std::string::npos) { v = "video/" + v; }
                if (v.find('.', v.find_last_of("/\\") + 1) == std::string::npos) { v += ".roq"; }
                if (!k.gibt(v)) { melde(BefundArt::DateiFehlt, mn, in, 0, 0, v, "cinematic", false); }
            }
            if (it.window.style == WINDOW_STYLE_CINEMATIC && d.dialekt == Dialekt::SP) {   // M4: SP-Window_Paint kennt es nicht
                melde(BefundArt::CinematicSp, mn, in, 0, 0, "style", it.window.cinematicName, false);
            }
            if (!it.focusSound.empty() && !klangDa(k, it.focusSound)) { melde(BefundArt::DateiFehlt, mn, in, 0, 0, it.focusSound, "focusSound", false); }
            // Vertraeglichkeit mit den anderen Fassungen
            for (const Spiel s : k.zielSpiele) {
                for (const Dialekt dl : {Dialekt::SP, Dialekt::MP}) {
                    if (dl != d.dialekt) { continue; }   // der Dialekt folgt dem Pfad (ui/ bzw. ui/jamp/)
                    const std::vector<std::string> tab = itemSchluessel(dl, s);
                    for (const Eigenschaft& e : it.eigenschaften) {
                        const std::string kanon = e.kanon.empty() ? e.schluessel : e.kanon;
                        if (std::find_if(tab.begin(), tab.end(), [&](const std::string& t) { return klein(t) == klein(kanon); }) == tab.end()) {
                            melde(BefundArt::UnbekanntIn, mn, in, 0, 0, kanon, fassungsName(dl, s), false);
                        }
                    }
                    if ((it.type == ITEM_TYPE_SLIDER_INTEGER || it.type == ITEM_TYPE_SLIDER_ROTATE) && s != Spiel::MD) {
                        melde(BefundArt::UnbekanntIn, mn, in, 0, 0, typName(it.type), fassungsName(dl, s), false);
                    }
                }
            }
        }
    }
    return r;
}

}  // namespace bhed::menue
