// profil.cpp - Menue-Editor M5b Punkt 4: Mod-Profile (siehe profil.h).
//
// Eigener Code. Die Menuelisten der eingebauten Profile: OpenJK code/ui/ui_main.cpp
// (ui_menuFiles "ui/menus.txt"), codemp/ui/ui_main.c (ui/jampmenus.txt, ui/jampingame.txt),
// Jedi Outcast MP ui/jk2mpmenus.txt, Movie Duels ui/MovieDuels-MP-Menus.txt.
#include "bhed/menue/profil.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace bhed::menue {

namespace {

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

std::string wert(const std::string& s) {
    if (!s.empty() && s.find_first_of(" \t\"\\") == std::string::npos) { return s; }
    std::string o = "\"";
    for (const char c : s) {
        if (c == '"' || c == '\\') { o += '\\'; }
        o += c;
    }
    return o + "\"";
}

std::vector<std::string> woerter(const std::string& z) {
    std::vector<std::string> v;
    std::size_t p = 0;
    while (p < z.size()) {
        while (p < z.size() && (z[p] == ' ' || z[p] == '\t')) { ++p; }
        if (p >= z.size()) { break; }
        std::string w;
        if (z[p] == '"') {
            ++p;
            while (p < z.size() && z[p] != '"') {
                if (z[p] == '\\' && p + 1 < z.size()) { ++p; }
                w += z[p++];
            }
            if (p < z.size()) { ++p; }
        } else {
            while (p < z.size() && z[p] != ' ' && z[p] != '\t') { w += z[p++]; }
        }
        v.push_back(w);
    }
    return v;
}

const char* spielKurz(Spiel s) { return s == Spiel::JK2 ? "jk2" : s == Spiel::MD ? "md" : "ja"; }

bool artGueltig(const std::string& a) {
    static const char* const arten[] = {"int", "float", "string", "token", "rect", "color", "vec3", "script", "none"};
    return std::any_of(std::begin(arten), std::end(arten), [&](const char* x) { return a == x; });
}

}  // namespace

std::string profilText(const ModProfil& p) {
    std::string s = "jksprofile 1\n";
    s += "name " + wert(p.name) + "\n";
    s += std::string("game ") + spielKurz(p.spiel) + "\n";
    for (const std::string& m : p.menuelistenSp) { s += "menulist sp " + wert(m) + "\n"; }
    for (const std::string& m : p.menuelistenMp) { s += "menulist mp " + wert(m) + "\n"; }
    for (const auto& [n, a] : p.itemSchluessel) { s += "itemkey " + wert(n) + " " + a + "\n"; }
    for (const auto& [n, a] : p.menuSchluessel) { s += "menukey " + wert(n) + " " + a + "\n"; }
    for (const auto& [z, n] : p.ownerdraws) { s += "ownerdraw " + std::to_string(z) + " " + wert(n) + "\n"; }
    for (const auto& [z, n] : p.feeder) { s += "feeder " + std::to_string(z) + " " + wert(n) + "\n"; }
    for (const auto& [n, w] : p.uiSkripte) { s += "uiscript " + wert(n) + " " + std::to_string(w) + "\n"; }
    for (const auto& [n, w] : p.cvars) { s += "cvar " + wert(n) + " " + wert(w) + "\n"; }
    for (const std::string& z : p.unbekannt) { s += z + "\n"; }
    return s;
}

bool liesProfil(const std::string& text, ModProfil& out, int& fehlerZeile) {
    ModProfil p;
    bool kopf = false;
    int nr = 0;
    std::size_t a = 0;
    fehlerZeile = 0;
    while (a <= text.size()) {
        std::size_t e = text.find('\n', a);
        if (e == std::string::npos) { e = text.size(); }
        std::string z = text.substr(a, e - a);
        a = e + 1;
        ++nr;
        if (!z.empty() && z.back() == '\r') { z.pop_back(); }
        const std::vector<std::string> w = woerter(z);
        if (w.empty() || w[0].compare(0, 2, "//") == 0) { continue; }
        const std::string& k = w[0];
        if (!kopf) {
            if (k != "jksprofile" || w.size() < 2 || std::atoi(w[1].c_str()) != 1) {
                fehlerZeile = nr;
                return false;
            }
            kopf = true;
            continue;
        }
        const std::string v1 = w.size() > 1 ? w[1] : std::string(), v2 = w.size() > 2 ? w[2] : std::string();
        // Art und Zahlen pruefen: was nicht passt, bleibt als unverstandene Zeile erhalten
        const bool zahl1 = !v1.empty() && (std::isdigit(static_cast<unsigned char>(v1[0])) != 0 || v1[0] == '-');
        if (k == "name" && w.size() > 1) { p.name = v1; }
        else if (k == "game" && w.size() > 1) { p.spiel = v1 == "jk2" ? Spiel::JK2 : v1 == "md" ? Spiel::MD : Spiel::JA; }
        else if (k == "menulist" && w.size() > 2 && (v1 == "sp" || v1 == "mp")) { (v1 == "sp" ? p.menuelistenSp : p.menuelistenMp).push_back(v2); }
        else if ((k == "itemkey" || k == "menukey") && w.size() > 2 && artGueltig(klein(v2))) {
            (k == "itemkey" ? p.itemSchluessel : p.menuSchluessel)[v1] = klein(v2);
        } else if ((k == "ownerdraw" || k == "feeder") && w.size() > 2 && zahl1) {
            (k == "ownerdraw" ? p.ownerdraws : p.feeder)[static_cast<int>(std::strtol(v1.c_str(), nullptr, 0))] = v2;
        } else if (k == "uiscript" && w.size() > 2) { p.uiSkripte[v1] = std::atoi(v2.c_str()); }
        else if (k == "cvar" && w.size() > 2) { p.cvars[v1] = v2; }
        else { p.unbekannt.push_back(z); }
    }
    if (!kopf) {
        fehlerZeile = 1;
        return false;
    }
    out = p;
    return true;
}

const std::vector<ModProfil>& eingebauteProfile() {
    static const std::vector<ModProfil> v = [] {
        ModProfil ja;
        ja.name = "Jedi Academy";
        ja.spiel = Spiel::JA;
        ja.menuelistenSp = {"ui/menus.txt"};
        ja.menuelistenMp = {"ui/jampmenus.txt", "ui/jampingame.txt"};
        ModProfil jk2;
        jk2.name = "Jedi Outcast";
        jk2.spiel = Spiel::JK2;
        jk2.menuelistenSp = {"ui/menus.txt"};
        jk2.menuelistenMp = {"ui/jk2mpmenus.txt", "ui/jk2mpingame.txt"};
        ModProfil md;
        md.name = "Movie Duels";
        md.spiel = Spiel::MD;
        md.menuelistenSp = {"ui/menus.txt"};
        md.menuelistenMp = {"ui/MovieDuels-MP-Menus.txt"};
        return std::vector<ModProfil>{ja, jk2, md};
    }();
    return v;
}

const ModProfil& eingebautesProfil(Spiel s) {
    const auto& v = eingebauteProfile();
    return v[s == Spiel::JK2 ? 1 : s == Spiel::MD ? 2 : 0];
}

std::vector<std::string> menuelisten(const ModProfil& p, Dialekt d) {
    std::vector<std::string> v;
    auto dazu = [&](const ModProfil& q) {
        for (const std::string& m : d == Dialekt::MP ? q.menuelistenMp : q.menuelistenSp) {
            if (std::none_of(v.begin(), v.end(), [&](const std::string& x) { return klein(x) == klein(m); })) { v.push_back(m); }
        }
    };
    dazu(p);
    for (const ModProfil& q : eingebauteProfile()) { dazu(q); }
    return v;
}

std::string profilSchluesselArt(const ModProfil& p, bool item, const std::string& name, const std::string** kanon) {
    const auto& tab = item ? p.itemSchluessel : p.menuSchluessel;
    for (const auto& [n, a] : tab) {
        if (klein(n) == klein(name)) {
            if (kanon != nullptr) { *kanon = &n; }
            return a;
        }
    }
    return {};
}

std::string profilVorlage(const std::string& name, Spiel basis) {
    std::string s;
    s += "// JK Studio mod profile - fill in what the mod's engine knows in addition to the base game.\n";
    s += "// One entry per line; lines starting with // are comments. Delete the examples you do not need.\n";
    s += "// Without the mod's source code: look at the mod's .menu files - every keyword, ownerdraw number,\n";
    s += "// feeder number or uiScript name the editor reports as unknown belongs here.\n";
    s += "jksprofile 1\n";
    s += "name " + wert(name) + "\n";
    s += std::string("// whose tables the mod extends: ja (Jedi Academy), jk2 (Jedi Outcast), md (Movie Duels)\n");
    s += std::string("game ") + spielKurz(basis) + "\n";
    s += "// menu lists the mod's UI loads (UI_Load / ui_menuFilesMP), searched in this order\n";
    s += "menulist sp ui/menus.txt\n";
    s += "menulist mp ui/jampmenus.txt\n";
    s += "// extra itemDef / menuDef keywords and the kind of their value:\n";
    s += "//   int float string token rect color vec3 script none\n";
    s += "// itemkey exampleScale float\n";
    s += "// itemkey exampleOnHold script\n";
    s += "// menukey exampleMusic string\n";
    s += "// ownerdraw numbers and names (from the mod's menudef.h), shown as labelled placeholders\n";
    s += "// ownerdraw 300 EXAMPLE_MAP_PREVIEW\n";
    s += "// feeder numbers and names (FEEDER_* in menudef.h)\n";
    s += "// feeder 40 FEEDER_EXAMPLE_LIST\n";
    s += "// uiScript names and how many words follow the name (UI_RunMenuScript)\n";
    s += "// uiscript exampleStartGame 0\n";
    s += "// uiscript exampleSetClass 1\n";
    s += "// start values of cvars the mod's UI creates (cvarTable)\n";
    s += "// cvar ui_example_class \"0\"\n";
    return s;
}

}  // namespace bhed::menue
