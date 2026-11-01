// feeder.cpp - Menue-Editor M2 Punkt 1: Listbox-Feeder aus den Spieldaten.
// Portiert aus OpenJK (code/ui/ui_main.cpp, codemp/ui/ui_main.c, codemp/ui/ui_gameinfo.c).
// Herkunft und Lizenz siehe include/bhed/menue/feeder.h (OpenJK
// code/ui/ui_main.cpp, codemp/ui/ui_main.c, codemp/ui/ui_gameinfo.c,
// code/qcommon/files.cpp).
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
#include "bhed/menue/feeder.h"

#include "bhed/pk3.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <set>

namespace bhed::menue {

namespace {

// M7: die festen Tabellen der Movie-Duels-Oberflaeche (tools/gen_md_tabellen.py)
struct MdFigur {
    const char* name;
    const char* icon;
    const char* npc;
    const char* model;
    const char* skin;
    int era;
    const char* desc;
    bool subDiv;
};
struct MdDuellKarte {
    const char* name;
    const char* desc;
    const char* bsp;
    const char* bild;
};
struct MdGriff {
    const char* name;
    const char* griff;
};
#include "md_tabellen.inc"

constexpr int kMdEpochenZahl = static_cast<int>(std::size(kMdEpochen));   // TOTAL_ERAS
constexpr int kMdFigurenZahl = static_cast<int>(std::size(kMdFiguren));   // NO_OF_MD_MODELS

std::string klein(std::string s);

// M7: COM_ParseExt / SkipRestOfLine ueber saberParms (codemp/qcommon/q_shared.c), fuer
// WP_SaberGetHiltInfo und WP_SaberParseParm (codemp/game/bg_saberLoad.c)
struct SaberText {
    const std::string& t;
    std::size_t p = 0;
    bool ende() const { return p >= t.size(); }
    // COM_ParseExt(&p, allowLineBreaks): "" am Ende bzw. am Zeilenende ohne allowLineBreaks
    std::string wort(bool zeilen) {
        while (true) {
            while (p < t.size() && static_cast<unsigned char>(t[p]) <= ' ') {
                if (t[p] == '\n' && !zeilen) { return {}; }
                ++p;
            }
            if (p >= t.size()) { return {}; }
            if (t.compare(p, 2, "//") == 0) {
                while (p < t.size() && t[p] != '\n') { ++p; }
                continue;
            }
            if (t.compare(p, 2, "/*") == 0) {
                const std::size_t e = t.find("*/", p + 2);
                p = e == std::string::npos ? t.size() : e + 2;
                continue;
            }
            break;
        }
        std::string w;
        if (t[p] == '"') {
            ++p;
            while (p < t.size() && t[p] != '"') { w += t[p++]; }
            if (p < t.size()) { ++p; }
            return w;
        }
        while (p < t.size() && static_cast<unsigned char>(t[p]) > ' ') { w += t[p++]; }
        return w;
    }
    void zeilenRest() {   // SkipRestOfLine
        while (p < t.size() && t[p] != '\n') { ++p; }
        if (p < t.size()) { ++p; }
    }
    void klammerBlock() {   // SkipBracedSection(&p, 0): mit Tiefe 0 genau ein Wort
        int tiefe = 0;
        do {
            const std::string w = wort(true);
            if (w == "{") { ++tiefe; }
            else if (w == "}") { --tiefe; }
        } while (tiefe != 0 && !ende());
    }
};

// WP_SaberParseParm(saberName, parmname)
std::string saberParm(const std::string& parms, const std::string& name, const std::string& parm) {
    SaberText s{parms};
    while (true) {
        const std::string w = s.wort(true);
        if (w.empty()) { return {}; }
        if (klein(w) == klein(name)) { break; }
        s.klammerBlock();
    }
    if (s.wort(true) != "{") { return {}; }
    while (true) {
        const std::string w = s.wort(true);
        if (w.empty() || w == "}") { return {}; }
        if (klein(w) == klein(parm)) { return s.wort(false); }
        s.zeilenRest();
    }
}

// MD_BG_GenerateEraIndexes: eraIndex[TOTAL_ERAS + 1] (die Abschlusszeile setzt den letzten)
std::array<int, kMdEpochenZahl + 1> mdEpochenIndex() {
    std::array<int, kMdEpochenZahl + 1> e{};
    int f = 0;
    for (int i = 0; i < kMdFigurenZahl; ++i) {
        if (kMdFiguren[i].era != f && f < kMdEpochenZahl) {
            ++f;
            e[static_cast<std::size_t>(f)] = i;
        }
    }
    return e;
}

std::string klein(std::string s) {
    for (char& c : s) { c = c == '\\' ? '/' : static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}
int tiefe(const std::string& s) { return static_cast<int>(std::count(s.begin(), s.end(), '/')); }
bool endetMitOhneFall(const std::string& s, const std::string& e) {
    return s.size() >= e.size() && klein(s.substr(s.size() - e.size())) == klein(e);
}
std::string ohneEndung(const std::string& s) {   // COM_StripExtension
    const std::size_t p = s.rfind('.');
    const std::size_t sl = s.rfind('/');
    return (p != std::string::npos && (sl == std::string::npos || p > sl)) ? s.substr(0, p) : s;
}

// COM_ParseExt (vereinfacht wie im Spiel: Kommentare, "..." und Woerter)
struct Woerter {
    const std::string& t;
    std::size_t p = 0;
    explicit Woerter(const std::string& s) : t(s) {}
    std::string naechstes(bool zeilen, bool* zeileUmbruch = nullptr) {
        while (p < t.size()) {
            const char c = t[p];
            if (c == '\n') {
                if (!zeilen) {
                    if (zeileUmbruch != nullptr) { *zeileUmbruch = true; }
                    return {};
                }
                ++p;
            } else if (static_cast<unsigned char>(c) <= ' ') {
                ++p;
            } else if (c == '/' && p + 1 < t.size() && t[p + 1] == '/') {
                while (p < t.size() && t[p] != '\n') { ++p; }
            } else if (c == '/' && p + 1 < t.size() && t[p + 1] == '*') {
                p += 2;
                while (p + 1 < t.size() && !(t[p] == '*' && t[p + 1] == '/')) { ++p; }
                p += 2;
            } else {
                break;
            }
        }
        if (p >= t.size()) { return {}; }
        std::string w;
        if (t[p] == '"') {
            ++p;
            while (p < t.size() && t[p] != '"') { w += t[p++]; }
            ++p;
            return w;
        }
        while (p < t.size() && static_cast<unsigned char>(t[p]) > ' ') { w += t[p++]; }
        return w;
    }
};

// Movie-Tabellen (code/ui/ui_main.cpp datapadMoveTitleData, datapadMoveData[..].title)
const char* const kMoveTitel[] = {"@MENUS_ACROBATICS", "@MENUS_SINGLE_FAST", "@MENUS_SINGLE_MEDIUM",
                                  "@MENUS_SINGLE_STRONG", "@MENUS_DUAL_SABERS", "@MENUS_SABER_STAFF"};
const std::vector<std::vector<const char*>>& moveDaten() {
    static const std::vector<std::vector<const char*>> d = {
        {"@MENUS_FORCE_JUMP1", "@MENUS_FORCE_FLIP", "@MENUS_ROLL", "@MENUS_BACKFLIP_OFF_WALL", "@MENUS_SIDEFLIP_OFF_WALL", "@MENUS_WALL_RUN",
         "@MENUS_LONG_JUMP", "@MENUS_WALL_GRAB_JUMP", "@MENUS_RUN_UP_WALL_BACKFLIP", "@MENUS_JUMPUP_FROM_KNOCKDOWN",
         "@MENUS_JUMPKICK_FROM_KNOCKDOWN", "@MENUS_ROLL_FROM_KNOCKDOWN"},
        {"@MENUS_STAB_BACK", "@MENUS_LUNGE_ATTACK", "@MENUS_FORCE_PULL_IMPALE", "@MENUS_FAST_ATTACK_KATA", "@MENUS_ATTACK_ENEMYONGROUND",
         "@MENUS_CARTWHEEL", "@MENUS_BOTH_ROLL_STAB"},
        {"@MENUS_SLASH_BACK", "@MENUS_FLIP_ATTACK", "@MENUS_FORCE_PULL_SLASH", "@MENUS_MEDIUM_ATTACK_KATA", "@MENUS_ATTACK_ENEMYONGROUND",
         "@MENUS_CARTWHEEL", "@MENUS_BOTH_ROLL_STAB"},
        {"@MENUS_SLASH_BACK", "@MENUS_JUMP_ATTACK", "@MENUS_FORCE_PULL_SLASH", "@MENUS_STRONG_ATTACK_KATA", "@MENUS_ATTACK_ENEMYONGROUND",
         "@MENUS_CARTWHEEL", "@MENUS_BOTH_ROLL_STAB"},
        {"@MENUS_SLASH_BACK", "@MENUS_FLIP_FORWARD_ATTACK", "@MENUS_DUAL_SABERS_TWIRL", "@MENUS_ATTACK_ENEMYONGROUND",
         "@MENUS_DUAL_SABER_BARRIER", "@MENUS_DUAL_STAB_FRONT_BACK", "@MENUS_DUAL_STAB_LEFT_RIGHT", "@MENUS_CARTWHEEL",
         "@MENUS_BOTH_ROLL_STAB"},
        {"@MENUS_STAB_BACK", "@MENUS_BACK_FLIP_ATTACK", "@MENUS_SABER_STAFF_TWIRL", "@MENUS_ATTACK_ENEMYONGROUND", "@MENUS_SPINNING_KATA",
         "@MENUS_KICK1", "@MENUS_JUMP_KICK", "@MENUS_SPLIT_KICK", "@MENUS_SPIN_KICK", "@MENUS_FLIP_KICK", "@MENUS_BUTTERFLY_ATTACK",
         "@MENUS_BOTH_ROLL_STAB"},
    };
    return d;
}
}  // namespace

std::vector<std::string> dateiListe(const Spieldaten& d, const std::string& pfadRoh, const std::string& endung) {
    // FS_ListFilteredFiles: Suchpfade von der hoechsten Prioritaet abwaerts -
    // je Spielordner zuerst die losen Dateien (Sys_ListFiles), dann die pk3
    // rueckwaerts (paksort, jede vorne eingehaengt) in Archivreihenfolge;
    // FS_AddFileToList nimmt jeden Namen (Gross/Klein egal) nur einmal.
    std::string pfad = klein(pfadRoh);
    while (!pfad.empty() && pfad.back() == '/') { pfad.pop_back(); }
    const int pfadTiefe = pfad.empty() ? -1 : tiefe(pfad);
    std::vector<std::string> v;
    std::set<std::string> gesehen;
    auto dazu = [&](const std::string& n) {
        if (gesehen.insert(klein(n)).second) { v.push_back(n); }
    };
    for (auto o = d.ordner.rbegin(); o != d.ordner.rend(); ++o) {
        if (!o->directory.empty()) {
            std::error_code ec;
            const std::filesystem::path dir = std::filesystem::path(o->directory) / pfadRoh;
            std::vector<std::string> lose;
            if (std::filesystem::is_directory(dir, ec)) {
                for (const auto& e : std::filesystem::directory_iterator(dir, ec)) {
                    const std::u8string u = e.path().filename().u8string();
                    const std::string n(u.begin(), u.end());
                    if (endung == "/") {
                        if (e.is_directory(ec)) { lose.push_back(n); }
                    } else if (e.is_regular_file(ec) && (endung.empty() || endetMitOhneFall(n, endung))) {
                        lose.push_back(n);
                    }
                }
            }
            std::sort(lose.begin(), lose.end(), [](const std::string& a, const std::string& b) { return klein(a) < klein(b); });
            for (const std::string& n : lose) { dazu(n); }
        }
        for (auto a = o->archives.rbegin(); a != o->archives.rend(); ++a) {
            const bool ordnerListe = endung == "/";
            const std::size_t n = ordnerListe ? a->directories.size() : a->entries.size();
            for (std::size_t i = 0; i < n; ++i) {
                const std::string& name = ordnerListe ? a->directories[i] : a->entries[i].name;
                const std::string kl = klein(name);
                const int t = tiefe(kl);
                if (!pfad.empty() && (kl.size() <= pfad.size() || kl.compare(0, pfad.size(), pfad) != 0 || kl[pfad.size()] != '/')) { continue; }
                if ((t - pfadTiefe) > 2) { continue; }
                if (!endung.empty() && !endetMitOhneFall(kl, endung)) { continue; }
                std::string rest = name.substr(pfad.empty() ? 0 : pfad.size() + 1);
                if (ordnerListe) {
                    if (rest.empty()) { continue; }   // "models/players/" selbst: leerer Name
                    rest.pop_back();                  // "kyle/" -> "kyle"
                }
                dazu(rest);
            }
        }
    }
    return v;
}

void FeederDaten::baue() {
    gebaut = true;
    spezies.clear();
    sprachen.clear();
    mods.clear();
    karten.clear();
    koepfe.clear();
    demos.clear();
    filme.clear();
    kraefte.clear();
    spielstaende.clear();
    spieltypen.clear();
    joinSpieltypen.clear();
    bots.clear();
    kartenDatei.clear();
    if (daten == nullptr) { return; }
    const Spieldaten& d = *daten;
    // UI_BuildPlayerModel_List (SP und MP gleich aufgebaut)
    for (const std::string& dir : dateiListe(d, "models/players", "/")) {
        if (dir == "." || dir == "..") { continue; }
        std::string choice;
        if (!d.lies("models/players/" + dir + "/PlayerChoice.txt", choice)) { continue; }
        Spezies s;
        s.name = dir;
        // UI_ParseColorData
        Woerter w(choice);
        while (true) {
            const std::string shader = w.naechstes(true);
            if (shader.empty()) { break; }
            if (w.naechstes(true) != "{") { break; }
            std::string aktion;
            bool ok = true;
            while (true) {
                const std::string x = w.naechstes(true);
                if (x.empty()) {
                    ok = false;
                    break;
                }
                if (x == "}") { break; }
                aktion += x + " ";
            }
            if (!ok) { break; }
            s.farbShader.push_back(shader);
            s.farbAktion.push_back(aktion);
        }
        int teile = 0;
        for (const std::string& f : dateiListe(d, "models/players/" + dir, ".skin")) {
            const std::string skin = ohneEndung(f);
            // IsImageFile: icon_<skin>.jpg, sonst .png, sonst .tga
            const std::string ic = "models/players/" + dir + "/icon_" + skin;
            if (!d.gibt(ic + ".jpg") && !d.gibt(ic + ".png") && !d.gibt(ic + ".tga")) { continue; }
            const std::string k = klein(skin);
            if (k.compare(0, 5, "head_") == 0) {
                s.kopf.push_back(skin);
                teile |= 1;
            } else if (k.compare(0, 6, "torso_") == 0) {
                s.rumpf.push_back(skin);
                teile |= 2;
            } else if (k.compare(0, 6, "lower_") == 0) {
                s.beine.push_back(skin);
                teile |= 4;
            }
        }
        if (teile != 7) { continue; }   // nicht fuer jeden Teil eine Haut
        spezies.push_back(s);
    }
    // UI_LoadLanguages (SE_GetNumLanguages): die Ordner unter strings/
    for (const std::string& s : dateiListe(d, "strings", "/")) { sprachen.push_back(s); }
    // UI_LoadMods (FS_GetFileList("$modlist")): Ordner neben base mit description.txt
    if (!basisOrdner.empty()) {
        std::error_code ec;
        std::vector<std::string> ord;
        for (const auto& e : std::filesystem::directory_iterator(basisOrdner, ec)) {
            if (!e.is_directory(ec)) { continue; }
            const std::u8string u = e.path().filename().u8string();
            const std::string n(u.begin(), u.end());
            if (klein(n) == "base") { continue; }
            ord.push_back(n);
        }
        std::sort(ord.begin(), ord.end(), [](const std::string& a, const std::string& b) { return klein(a) < klein(b); });
        for (const std::string& n : ord) {
            const std::filesystem::path dir = std::filesystem::path(basisOrdner) / n;
            bool hatPk3 = false;
            for (const auto& e : std::filesystem::directory_iterator(dir, ec)) {
                hatPk3 = hatPk3 || endetMitOhneFall(e.path().filename().string(), ".pk3");
            }
            if (!hatPk3) { continue; }   // FS_GetModList: nur Ordner mit pk3
            std::string beschr;
            std::FILE* f = std::fopen((dir / "description.txt").string().c_str(), "rb");
            if (f != nullptr) {
                char buf[256] = {};
                const std::size_t n2 = std::fread(buf, 1, sizeof(buf) - 1, f);
                std::fclose(f);
                beschr.assign(buf, n2);
                while (!beschr.empty() && (beschr.back() == '\n' || beschr.back() == '\r')) { beschr.pop_back(); }
            }
            mods.emplace_back(n, beschr);
        }
    }
    // Spielstaende (SP ReadSaveDirectory): saves/*.sav der Spielordner
    for (const std::string& o : ordner) {
        std::error_code ec;
        const std::filesystem::path dir = std::filesystem::path(o) / "saves";
        std::vector<std::pair<std::string, std::string>> st;
        for (const auto& e : std::filesystem::directory_iterator(dir, ec)) {
            if (!e.is_regular_file(ec) || !endetMitOhneFall(e.path().filename().string(), ".sav")) { continue; }
            const auto zeit = std::chrono::clock_cast<std::chrono::system_clock>(e.last_write_time(ec));
            const std::time_t tt = std::chrono::system_clock::to_time_t(zeit);
            std::tm tm{};
#if defined(_WIN32)
            (void)localtime_s(&tm, &tt);
#else
            (void)localtime_r(&tt, &tm);
#endif
            char buf[64];
            std::strftime(buf, sizeof(buf), "%m/%d/%Y %H:%M", &tm);
            st.emplace_back(ohneEndung(e.path().filename().string()), buf);
        }
        std::sort(st.begin(), st.end());
        for (auto& s : st) { spielstaende.push_back(s); }
    }
    if (dialekt == Dialekt::MP) {
        // UI_LoadArenas: scripts/*.arena, je { map ... longname ... type ... }
        std::vector<std::string> arenen = dateiListe(d, "scripts", ".arena");
        // rc608: Jedi Outcast hat fast alle in scripts/arenas.txt (die Kartenliste blieb leer)
        if (spiel == Spiel::JK2) { arenen.insert(arenen.begin(), "arenas.txt"); }
        for (const std::string& f : arenen) {
            std::string t;
            if (!d.lies("scripts/" + f, t)) { continue; }
            Woerter w(t);
            while (true) {
                const std::string tok = w.naechstes(true);
                if (tok != "{") { break; }
                std::string map, lang, typ;
                while (true) {
                    const std::string k = w.naechstes(true);
                    if (k.empty() || k == "}") { break; }
                    bool umbruch = false;
                    std::string val = w.naechstes(false, &umbruch);
                    if (val.empty()) { val = "<NULL>"; }
                    if (klein(k) == "map") { map = val; }
                    else if (klein(k) == "longname") { lang = val; }
                    else if (klein(k) == "type") { typ = val; }
                }
                // typeBits: ohne type FFA + Jedi Master; "ffa" = FFA, Team, Jedi Master
                karten.emplace_back(lang.empty() ? map : lang, typ.empty() || typ.find("ffa") != std::string::npos ? "1" : "0");
                kartenDatei.push_back(map);
            }
        }
        // UI_BuildQ3Model_List (M5 1:1): je <ordner>/*.skin der Teil ab dem ersten '_' ("model_default"
        // -> "_default"), gilt, wenn icon_<rest>.jpg/.png/.tga da ist (bIsImageFile); Name "<ordner>/<rest>",
        // ohne Doppelte, hoechstens MAX_Q3PLAYERMODELS (1024)
        for (const std::string& dir : dateiListe(d, "models/players", "/")) {
            if (dir == "." || dir == "..") { continue; }
            for (const std::string& f : dateiListe(d, "models/players/" + dir, ".skin")) {
                std::string skin = ohneEndung(f);
                const std::size_t u = skin.find('_');
                if (u != std::string::npos) { skin = skin.substr(u); }
                if (skin.size() < 2) { continue; }
                const std::string ic = "models/players/" + dir + "/icon_" + skin.substr(1);
                if (!d.gibt(ic + ".jpg") && !d.gibt(ic + ".png") && !d.gibt(ic + ".tga")) { continue; }
                if (skin[0] == '_') { skin[0] = '/'; }
                const std::string name = dir + skin;
                if (std::any_of(koepfe.begin(), koepfe.end(), [&](const std::string& k) { return klein(k) == klein(name); })) { continue; }
                koepfe.push_back(name);
                if (koepfe.size() >= 1024) { break; }
            }
            if (koepfe.size() >= 1024) { break; }
        }
        // UI_ParseGameInfo("ui/jamp/gameinfo.txt", Jedi Outcast ui/jk2mp/gameinfo.txt): gametypes und
        // joingametypes { { "Name" gtEnum } ... } (GameType_Parse)
        std::string gi;
        if (d.lies(spiel == Spiel::JK2 ? "ui/jk2mp/gameinfo.txt" : "ui/jamp/gameinfo.txt", gi)) {
            Woerter w(gi);
            for (std::string tok = w.naechstes(true); !tok.empty(); tok = w.naechstes(true)) {
                const std::string t = klein(tok);
                if (t != "gametypes" && t != "joingametypes") { continue; }
                auto& liste = t == "gametypes" ? spieltypen : joinSpieltypen;
                liste.clear();
                if (w.naechstes(true) != "{") { break; }
                for (std::string x = w.naechstes(true); x == "{"; x = w.naechstes(true)) {
                    const std::string name = w.naechstes(true);
                    const int gt = std::atoi(w.naechstes(true).c_str());
                    liste.emplace_back(name, gt);
                    if (w.naechstes(true) != "}") { break; }
                }
            }
        }
        // UI_LoadBots: botfiles/bots.txt (g_botsFile leer), dann scripts/*.bot; je { ... name <x> ... }
        auto botsAus = [&](const std::string& pfad) {
            std::string t;
            if (!d.lies(pfad, t)) { return; }
            Woerter w(t);
            for (std::string tok = w.naechstes(true); tok == "{"; tok = w.naechstes(true)) {
                std::string name;
                for (std::string k = w.naechstes(true); !k.empty() && k != "}"; k = w.naechstes(true)) {
                    const std::string v = w.naechstes(false);
                    if (klein(k) == "name") { name = v; }
                }
                bots.push_back(name.empty() ? std::string("Kyle") : name);   // UI_GetBotNameByNumber: ohne name "Kyle"
            }
        };
        botsAus("botfiles/bots.txt");
        for (const std::string& f : dateiListe(d, "scripts", ".bot")) { botsAus("scripts/" + f); }
        // M7: WP_SaberLoadParms (alle ext_data/sabers/*.sab hintereinander) und WP_SaberGetHiltInfo: jeder
        // Name mit "{" in einer folgenden Zeile, ohne notInMP; twoHanded -> Stab-Liste (je MAX_SABER_HILTS - 1)
        std::string parms;
        for (const std::string& f : dateiListe(d, "ext_data/sabers", ".sab")) {
            std::string t;
            if (d.lies("ext_data/sabers/" + f, t)) { parms += t; }
        }
        saberListen(parms, saberEinhand, saberStab);
        for (const std::string& f : dateiListe(d, "demos", ".dm_26")) { demos.push_back(ohneEndung(f)); }   // UI_LoadDemos
        for (const std::string& f : dateiListe(d, "video", ".roq")) { filme.push_back(ohneEndung(f)); }     // UI_LoadMovies
        kraefte.emplace_back("Custom");   // UI_LoadForceConfig_List: [0] immer "custom"
        for (const std::string& f : dateiListe(d, "forcecfg/light", ".fcf")) { kraefte.push_back(ohneEndung(f)); }
    }
}

int FeederDaten::speziesIndex(const std::string& modell) const {
    int i = 0;
    for (std::size_t k = 0; k < spezies.size(); ++k) {
        if (klein(spezies[k].name) != klein(modell)) { continue; }
        i = static_cast<int>(k);
        if (dialekt == Dialekt::MP) { break; }   // MP: break; SP: Schleife laeuft weiter
    }
    return i;
}

std::vector<FeederEintrag> FeederDaten::eintraege(float feeder, int sp, int titel) const {
    std::vector<FeederEintrag> r;
    auto zeile = [&](std::string a, std::string bild = {}) {
        FeederEintrag e;
        e.spalten.push_back(std::move(a));
        e.bild = std::move(bild);
        r.push_back(std::move(e));
    };
    const int f = static_cast<int>(feeder);
    if (static_cast<float>(f) != feeder) { return r; }
    const Spezies* s = (sp >= 0 && static_cast<std::size_t>(sp) < spezies.size()) ? &spezies[static_cast<std::size_t>(sp)] : nullptr;
    auto haeute = [&](const std::vector<std::string>& v) {
        if (s == nullptr) { return; }
        for (const std::string& h : v) {
            // UI_FeederItemText: *handle = icon_<skin>.jpg
            FeederEintrag e;
            e.spalten.push_back(h);
            e.spaltenBild.push_back("models/players/" + s->name + "/icon_" + h + ".jpg");
            e.bild = e.spaltenBild.back();   // UI_FeederItemImage
            r.push_back(std::move(e));
        }
    };
    auto farben = [&]() {
        if (s == nullptr) { return; }
        for (const std::string& c : s->farbShader) {
            FeederEintrag e;
            e.spalten.push_back(c);
            e.spaltenBild.push_back(c);
            e.bild = c;
            r.push_back(std::move(e));
        }
    };
    auto moves = [&]() {
        const auto& m = moveDaten();
        const auto t = static_cast<std::size_t>(std::clamp(titel, 0, static_cast<int>(m.size()) - 1));
        for (const char* x : m[t]) { zeile(x); }
    };
    auto modListe = [&]() {
        for (const auto& [n, b] : mods) { zeile(b.empty() ? n : b); }
    };
    if (dialekt == Dialekt::SP) {
        switch (f) {
            case feeder_sp::SAVEGAMES:
                for (const auto& [n, dt] : spielstaende) {
                    FeederEintrag e;
                    e.spalten = {n, dt};
                    r.push_back(e);
                }
                break;
            case feeder_sp::MOVES: moves(); break;
            case feeder_sp::MOVES_TITLES:
                for (const char* x : kMoveTitel) { zeile(x); }
                break;
            case feeder_sp::PLAYER_SPECIES:
                for (const Spezies& x : spezies) { zeile(x.name); }
                break;
            case feeder_sp::LANGUAGES:
                if (spiel != Spiel::JK2) {   // JK2_MODE: NULL
                    for (const std::string& x : sprachen) { zeile(x); }
                }
                break;
            case feeder_sp::PLAYER_SKIN_HEAD: haeute(s != nullptr ? s->kopf : std::vector<std::string>{}); break;
            case feeder_sp::PLAYER_SKIN_TORSO: haeute(s != nullptr ? s->rumpf : std::vector<std::string>{}); break;
            case feeder_sp::PLAYER_SKIN_LEGS: haeute(s != nullptr ? s->beine : std::vector<std::string>{}); break;
            case feeder_sp::COLORCHOICES: farben(); break;
            case feeder_sp::MODS: modListe(); break;
            default: break;
        }
        if (spiel == Spiel::MD) { mdEintraege(f, r); }
        return r;
    }
    switch (f) {
        case feeder_mp::MAPS:   // nur Einzelspieler-Karten (GT_SINGLE_PLAYER): keine Arena traegt das
            break;
        case feeder_mp::ALLMAPS:
            for (const auto& [n, ffa] : karten) {
                if (ffa == "1") { zeile(n); }
            }
            break;
        case feeder_mp::Q3HEADS:
            // UI_SelectedTeamHead: nur Koepfe mit "/default" (uiSkinColor TEAM_FREE beim Start)
            for (const std::string& k : koepfe) {
                if (k.find("/default") == std::string::npos) { continue; }
                const std::size_t p = k.rfind('/');
                const std::string dir = p == std::string::npos ? k : k.substr(0, p);
                const std::string skin = p == std::string::npos ? "default" : k.substr(p + 1);
                zeile(k, "models/players/" + dir + "/icon_" + skin + ".jpg");
            }
            break;
        case feeder_mp::MODS: modListe(); break;
        case feeder_mp::DEMOS:
            for (const std::string& x : demos) { zeile(x); }
            break;
        case feeder_mp::CINEMATICS:
            for (const std::string& x : filme) { zeile(x); }
            break;
        case feeder_mp::FORCECFG:
            for (const std::string& x : kraefte) { zeile(x); }
            break;
        case feeder_mp::PLAYER_SPECIES:
            for (const Spezies& x : spezies) { zeile(x.name); }
            break;
        case feeder_mp::PLAYER_SKIN_HEAD: haeute(s != nullptr ? s->kopf : std::vector<std::string>{}); break;
        case feeder_mp::PLAYER_SKIN_TORSO: haeute(s != nullptr ? s->rumpf : std::vector<std::string>{}); break;
        case feeder_mp::PLAYER_SKIN_LEGS: haeute(s != nullptr ? s->beine : std::vector<std::string>{}); break;
        case feeder_mp::COLORCHOICES: farben(); break;
        case feeder_mp::SABER_SINGLE_INFO:   // M7: UI_SaberProperNameForSaber ("@" ueber SE_GetStringTextString)
            for (const std::string& x : saberEinhand) { zeile(!x.empty() && x[0] == '@' && se ? se(x.substr(1)) : x); }
            break;
        case feeder_mp::SABER_STAFF_INFO:
            for (const std::string& x : saberStab) { zeile(!x.empty() && x[0] == '@' && se ? se(x.substr(1)) : x); }
            break;
        case feeder_mp::PLAYER_LIST:   // M7: uiInfo.playerNames (Beispiel: Spielername und zwei Bots)
            for (const char* x : {"Padawan", "Kyle", "Jan"}) { zeile(x); }
            break;
        case feeder_mp::MOVES: moves(); break;
        case feeder_mp::MOVES_TITLES:
            for (const char* x : kMoveTitel) { zeile(x); }
            break;
        case feeder_mp::LANGUAGES:   // MP UI_FeederItemText: return 0 (nichts zu zeichnen)
            for (std::size_t i = 0; i < sprachen.size(); ++i) {
                FeederEintrag e;
                r.push_back(e);
            }
            break;
        default: break;   // Server, Spieler, Siege ...: ohne laufendes Spiel leer
    }
    return r;
}

// M7: Movie Duels FEEDER_MD_FACTION .. FEEDER_STAFF_SABER_HILTS (code/ui/ui_main.cpp UI_FeederCount,
// UI_FeederItemText, UI_FeederItemImage) mit uiEra = 0, uiModelIndex = 0 wie nach dem Start
void FeederDaten::mdEintraege(int f, std::vector<FeederEintrag>& r) const {
    const auto e = mdEpochenIndex();
    constexpr int uiEra = 0, uiModelIndex = 0;
    auto bild = [](int i) {   // gfx/menus/charactermenuicons/<icon, sonst npc>
        const MdFigur& m = kMdFiguren[i];
        return std::string("gfx/menus/charactermenuicons/") + (m.icon[0] == 0 ? m.npc : m.icon);
    };
    auto eintrag = [&](std::string text, std::string b = {}) {
        FeederEintrag x;
        x.spalten.push_back(std::move(text));
        x.bild = std::move(b);
        r.push_back(std::move(x));
    };
    switch (f) {
        case 0x54:   // FEEDER_MD_FACTION
            for (const char* n : kMdEpochen) { eintrag(n); }
            break;
        case 0x55: {   // FEEDER_MD_MODELS: Figuren der Epoche ohne die Unterteilungen (MD_UI_SelectedTeamHead_SubDivs)
            int anzahl = e[uiEra + 1] - e[uiEra];
            for (int i = 0; i < e[uiEra + 1] - e[uiEra]; ++i) {
                if (kMdFiguren[e[uiEra] + i].subDiv) { --anzahl; }
            }
            for (int index = 0; index < anzahl; ++index) {
                int c = 0;
                for (int i = e[uiEra]; i < e[uiEra + 1]; ++i) {
                    if (!kMdFiguren[i].subDiv) {
                        for (int k = 1; k < kMdFigurenZahl && i + k < kMdFigurenZahl; ++k) {
                            if (!kMdFiguren[i + k].subDiv) { break; }
                            --c;
                        }
                    }
                    if (c == index) {
                        while (i > 0 && kMdFiguren[i].subDiv) { --i; }
                        eintrag("", bild(i));
                        break;
                    }
                    ++c;
                }
            }
            break;
        }
        case 0x56: {   // FEEDER_MD_VARIANTS: die gewaehlte Figur und ihre Unterteilungen
            int anzahl = 1;
            for (int i = 1; i < kMdFigurenZahl && uiModelIndex + i < kMdFigurenZahl; ++i) {
                if (!kMdFiguren[uiModelIndex + i].subDiv) {
                    anzahl = i;
                    break;
                }
            }
            for (int index = 0; index < anzahl; ++index) {
                const int i = e[uiEra] + index + uiModelIndex - e[uiEra];   // MD_UI_SelectedTeamHead + Versatz
                if (i >= uiModelIndex && i < uiModelIndex + anzahl) { eintrag("", bild(i)); }
            }
            break;
        }
        case 0x57:   // FEEDER_DUEL_MAPS
            for (const MdDuellKarte& k : kMdDuellKarten) { eintrag(k.name); }
            break;
        case 0x58:   // FEEDER_SINGLE_SABER_HILTS
            for (const MdGriff& g : kMdEinhandGriffe) { eintrag(g.name); }
            break;
        case 0x59:   // FEEDER_STAFF_SABER_HILTS
            for (const MdGriff& g : kMdStabGriffe) { eintrag(g.name); }
            break;
        default: break;
    }
}

int FeederDaten::art(float feeder) const {
    const int f = static_cast<int>(feeder);
    if (static_cast<float>(f) != feeder) { return 0; }
    if (dialekt == Dialekt::SP) {
        switch (f) {
            case feeder_sp::SAVEGAMES:
            case feeder_sp::MODS:
            case feeder_sp::PLAYER_SPECIES:
            case feeder_sp::PLAYER_SKIN_HEAD:
            case feeder_sp::PLAYER_SKIN_TORSO:
            case feeder_sp::PLAYER_SKIN_LEGS:
            case feeder_sp::COLORCHOICES:
            case feeder_sp::MOVES:
            case feeder_sp::MOVES_TITLES:
            case feeder_sp::LANGUAGES: return 1;
            default: return (spiel == Spiel::MD && f >= 0x54 && f <= 0x59) ? 1 : 0;
        }
    }
    switch (f) {
        case feeder_mp::PLAYER_LIST: return 2;   // nur im laufenden Spiel gefuellt: Beispielnamen
        case feeder_mp::MAPS:
        case feeder_mp::SERVERS:   // leer bis zur Abfrage (wie im Hauptmenue)
        case feeder_mp::ALLMAPS:
        case 0x05:                 // FEEDER_REDTEAM_LIST
        case 0x06:                 // FEEDER_BLUETEAM_LIST
        case 0x08:                 // FEEDER_TEAM_LIST
        case feeder_mp::MODS:
        case feeder_mp::DEMOS:
        case 0x0b:                 // FEEDER_SCOREBOARD
        case feeder_mp::Q3HEADS:
        case 0x0d:                 // FEEDER_SERVERSTATUS: leer bis zur Abfrage
        case 0x0e:                 // FEEDER_FINDPLAYER: leer bis zur Suche
        case feeder_mp::CINEMATICS:
        case feeder_mp::FORCECFG:
        case 0x11:                 // FEEDER_SIEGE_TEAM1/2: ohne Siege-Partie leer
        case 0x12:
        case feeder_mp::PLAYER_SPECIES:
        case feeder_mp::PLAYER_SKIN_HEAD:
        case feeder_mp::PLAYER_SKIN_TORSO:
        case feeder_mp::PLAYER_SKIN_LEGS:
        case feeder_mp::COLORCHOICES:
        case 0x24:                 // FEEDER_SIEGE_BASE_CLASS / _CLASS_WEAPONS/_INVENTORY/_FORCE: ohne Klasse leer
        case 0x25:
        case 0x26:
        case 0x27:
        case feeder_mp::LANGUAGES:
        case feeder_mp::MOVES:
        case feeder_mp::MOVES_TITLES:
        case feeder_mp::SABER_SINGLE_INFO:
        case feeder_mp::SABER_STAFF_INFO: return 1;
        default: return (f >= 0x18 && f <= 0x23) ? 1 : 0;   // FEEDER_TEAM1/2_*: Siege-Klassen der Partie
    }
}

void saberListen(const std::string& parms, std::vector<std::string>& einhand, std::vector<std::string>& stab) {
    einhand.clear();
    stab.clear();
    SaberText st{parms};
    while (!st.ende()) {
        const std::string name = st.wort(true);
        if (name.empty()) { continue; }
        st.zeilenRest();
        if (st.wort(true) != "{") { continue; }
        const std::string nichtMp = saberParm(parms, name, "notInMP");
        if (!nichtMp.empty() && std::atoi(nichtMp.c_str()) != 0) {
            st.klammerBlock();
            continue;
        }
        // UI_SaberProperNameForSaber: "name" ("@" = StringEd)
        const std::string proper = saberParm(parms, name, "name");
        const std::string zwei = saberParm(parms, name, "twoHanded");
        auto& liste = (!zwei.empty() && std::atoi(zwei.c_str()) != 0) ? stab : einhand;
        if (liste.size() < 255) { liste.push_back(proper); }   // MAX_SABER_HILTS 256, NULL-Ende
        st.klammerBlock();
    }
}

std::string mdDuellKarteBild(int index) {
    if (index < 0 || index >= static_cast<int>(std::size(kMdDuellKarten))) { return {}; }
    return kMdDuellKarten[index].bild;
}

}  // namespace bhed::menue
