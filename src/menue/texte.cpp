// texte.cpp - Menue-Editor M1: die @MENUS-Texte aus den StringEd-Dateien
// (strings/<Sprache>/*.str, Jedi Academy und Movie Duels) und den
// StripEd-Dateien (strip/*.sp, Jedi Outcast).
//
// Portiert aus OpenJK (code/qcommon/stringed_ingame.cpp: der Schluessel ist
// PAKET_REFERENZ in Grossbuchstaben, "#same" heisst "wie Englisch"; codeJK2
// strip: REFERENCE des Pakets, dann je Eintrag REFERENCE und
// TEXT_LANGUAGE<n>).
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
#include "bhed/menue/menue.h"

#include <cctype>

namespace bhed::menue {

namespace {

std::string gross(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
    return s;
}

// Woerter einer Zeile: Namen und "Strings" (mit \n als Zeilenumbruch).
std::vector<std::string> woerter(const std::string& z) {
    std::vector<std::string> v;
    std::size_t i = 0;
    while (i < z.size()) {
        while (i < z.size() && static_cast<unsigned char>(z[i]) <= ' ') { ++i; }
        if (i >= z.size()) { break; }
        if (z[i] == '/' && i + 1 < z.size() && z[i + 1] == '/') { break; }
        std::string w;
        if (z[i] == '"') {
            ++i;
            while (i < z.size() && z[i] != '"') {
                if (z[i] == '\\' && i + 1 < z.size() && z[i + 1] == 'n') {
                    w += '\n';
                    i += 2;
                    continue;
                }
                w += z[i++];
            }
            ++i;
        } else {
            while (i < z.size() && static_cast<unsigned char>(z[i]) > ' ') { w += z[i++]; }
        }
        v.push_back(w);
    }
    return v;
}

template <typename F>
void zeilen(const std::string& text, F f) {
    std::size_t a = 0;
    while (a <= text.size()) {
        std::size_t e = text.find('\n', a);
        if (e == std::string::npos) { e = text.size(); }
        std::string z = text.substr(a, e - a);
        if (!z.empty() && z.back() == '\r') { z.pop_back(); }
        f(z);
        a = e + 1;
    }
}

}  // namespace

void Texte::ladeStr(const std::string& dateiname, const std::string& text, const std::string& sprache) {
    std::string paket = dateiname;
    const std::size_t s = paket.find_last_of("/\\");
    if (s != std::string::npos) { paket = paket.substr(s + 1); }
    const std::size_t p = paket.find('.');
    if (p != std::string::npos) { paket = paket.substr(0, p); }
    paket = gross(paket);
    const std::string lang = "LANG_" + gross(sprache);
    std::string ref;
    std::string englisch;
    zeilen(text, [&](const std::string& z) {
        const std::vector<std::string> w = woerter(z);
        if (w.size() < 2) { return; }
        const std::string k = gross(w[0]);
        if (k == "REFERENCE") {
            ref = paket + "_" + gross(w[1]);
            englisch.clear();
        } else if (!ref.empty() && k == "LANG_ENGLISH") {
            englisch = w[1];
            if (werte.find(ref) == werte.end() || lang == "LANG_ENGLISH") { werte[ref] = w[1]; }
        } else if (!ref.empty() && k == lang) {
            werte[ref] = (w[1] == "#same") ? englisch : w[1];
        }
    });
}

void Texte::ladeSp(const std::string& text, int sprache) {
    std::string paket;
    std::string ref;
    int tiefe = 0;
    const std::string lang = "TEXT_LANGUAGE" + std::to_string(sprache);
    zeilen(text, [&](const std::string& z) {
        const std::vector<std::string> w = woerter(z);
        if (w.empty()) { return; }
        if (w[0] == "{") { ++tiefe; return; }
        if (w[0] == "}") { --tiefe; return; }
        if (w.size() < 2) { return; }
        const std::string k = gross(w[0]);
        if (k == "REFERENCE") {
            if (tiefe == 0) { paket = gross(w[1]); }
            else { ref = paket + "_" + gross(w[1]); }
        } else if (tiefe > 0 && !ref.empty() && k == "TEXT_LANGUAGE1") {
            if (werte.find(ref) == werte.end() || sprache == 1) { werte[ref] = w[1]; }
        } else if (tiefe > 0 && !ref.empty() && k == lang) {
            werte[ref] = w[1];
        }
    });
}

std::string Texte::aufloesen(const std::string& s) const {
    if (s.size() < 2 || s[0] != '@') { return s; }
    const auto it = werte.find(gross(s.substr(1)));
    return it == werte.end() ? s : it->second;
}

bool Texte::kennt(const std::string& s) const {
    return s.size() >= 2 && s[0] == '@' && werte.count(gross(s.substr(1))) != 0;
}

}  // namespace bhed::menue
