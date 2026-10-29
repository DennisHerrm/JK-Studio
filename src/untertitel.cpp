// untertitel.cpp - Untertitel wie das Spiel (siehe bhed/untertitel.h)
//
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
#include "bhed/untertitel.h"

#include <cctype>

namespace bhed::untertitel {
namespace {

std::string gross(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
    return s;
}

}  // namespace

std::vector<std::string> pakete(const std::string& karte, Spiel spiel, const std::function<bool(const std::string&)>& paketDa) {
    // mapname aus der serverinfo: ohne "maps/" und ".bsp"; stripLevelName[0]
    // ist alles nach dem letzten '/', gross (Q_strupr).
    std::string name = karte;
    const std::size_t strich = name.find_last_of("/\\");
    if (strich != std::string::npos) { name = name.substr(strich + 1); }
    const std::size_t punkt = name.find('.');
    if (punkt != std::string::npos) { name.resize(punkt); }
    name = gross(name);
    if (name.empty()) { return {}; }
    std::vector<std::string> p{name};
    const auto zusatz = [&p, &name] {
        // "additional SP files needed for some levels..."
        if (name == "KEJIM_BASE" || name == "KEJIM_POST") { p = {name, "ARTUS_MINE"}; }
        if (name == "DOOM_DETENTION" || name == "DOOM_SHIELDS") { p = {name, "DOOM_COMM"}; }
        if (name == "DOOM_COMM") { p = {name, "CAIRN_BAY"}; }
        if (name == "NS_STARPAD") { p = {name, "ARTUS_TOPSIDE", "BESPIN_UNDERCITY"}; }
        if (name == "BESPIN_PLATFORM") { p = {name, "BESPIN_UNDERCITY"}; }
    };
    const auto umbenennen = [&name]() -> bool {
        if (name == "YAVIN_FINAL" || name == "YAVIN_SWAMP") { name = "YAVIN_CANYON"; return true; }
        if (name == "YAVIN_TRIAL") { name = "YAVIN_TEMPLE"; return true; }
        if (name == "VALLEY") { name = "ARTUS_TOPSIDE"; return true; }
        return false;
    };
    switch (spiel) {
        case Spiel::JediAcademy:
            // code/cgame/cg_servercmds.cpp: "JKA..." - der JK2-Teil ist auskommentiert.
            if (name == "YAVIN1B") { p.emplace_back("YAVIN1"); }
            break;
        case Spiel::MovieDuels:
            // cgame/cg_servercmds.cpp: YAVIN1B, sonst die JK2-Karten ohne cgi_SP_Register.
            if (name == "YAVIN1B") {
                p.emplace_back("YAVIN1");
            } else if (umbenennen()) {
                p = {name};
            } else {
                zusatz();
            }
            break;
        case Spiel::JediOutcast:
            // codeJK2/cgame/cg_servercmds.cpp: umbenannt nur, wenn das Paket der
            // Karte nicht laedt; die Zusatzpakete nur, wenn es laedt.
            if (!paketDa || !paketDa(name)) {
                if (umbenennen()) { p = {name}; }
            } else {
                zusatz();
            }
            break;
    }
    return p;
}

std::string referenz(const std::string& klang) {
    // PlayIcarusSound: Q_strlwr, COM_StripExtension; CG_CaptionText: strrchr '/'.
    std::string n = klang;
    for (char& c : n) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (c == '\\') { c = '/'; }
    }
    const std::size_t strich = n.find_last_of('/');
    const std::size_t punkt = n.find_last_of('.');
    if (punkt != std::string::npos && (strich == std::string::npos || punkt > strich)) { n.resize(punkt); }
    if (strich == std::string::npos) { return {}; }
    return n.substr(strich + 1);
}

std::string text(const std::string& klang, const std::vector<std::string>& pakete, const std::map<std::string, std::string>& werte,
                 std::string* schluessel) {
    const std::string ref = gross(referenz(klang));
    if (ref.empty()) { return {}; }
    if (ref[0] == '#') {
        // "the striped package name is already built in"
        const std::string k = ref.substr(1);
        if (schluessel != nullptr) { *schluessel = k; }
        const auto it = werte.find(k);
        return it != werte.end() ? it->second : std::string();
    }
    for (const std::string& p : pakete) {
        const std::string k = p + "_" + ref;
        if (schluessel != nullptr && schluessel->empty()) { *schluessel = k; }
        const auto it = werte.find(k);
        if (it != werte.end() && !it->second.empty()) {
            if (schluessel != nullptr) { *schluessel = k; }
            return it->second;
        }
    }
    return {};
}

std::string jk2Reinigen(std::string t) {
    for (char& c : t) {
        const auto u = static_cast<unsigned char>(c);
        if (u == 0x92 || u == 0x91) { c = '\''; }
        else if (u == 0x93 || u == 0x94) { c = '"'; }
        else if (u == 0x0B || u == 0x85) { c = '.'; }   // 0x85: "can't do in-string replace ... so just forget it"
        else if (u == 0x96 || u == 0x97) { c = '-'; }
        else if (u == 0x09) { c = ' '; }
    }
    for (std::size_t i; (i = t.find("?.")) != std::string::npos;) { t[i + 1] = ' '; }
    return t;
}

}  // namespace bhed::untertitel
