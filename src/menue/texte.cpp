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
#include <string>
#include <string_view>

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

namespace {
// CStringEdPackage::REMKill: ab "//" weg, wenn davor eine gerade Zahl " steht
std::string ohneKommentar(std::string z) {
    std::size_t von = 0;
    int anf = 0;
    while (true) {
        const std::size_t k = z.find("//", von);
        if (k == std::string::npos) { return z; }
        for (std::size_t i = von; i < k; ++i) { anf += z[i] == '"' ? 1 : 0; }
        if ((anf & 1) == 0) { return z.substr(0, k); }
        von = k + 2;
    }
}
// CheckLineForKeyword: Anfang ohne Gross/Klein, danach Leer und Tab weg
bool schluessel(const char* k, std::string_view& z) {
    const std::size_t n = std::char_traits<char>::length(k);
    if (z.size() < n) { return false; }
    for (std::size_t i = 0; i < n; ++i) {
        if (std::toupper(static_cast<unsigned char>(z[i])) != std::toupper(static_cast<unsigned char>(k[i]))) { return false; }
    }
    z.remove_prefix(n);
    while (!z.empty() && (z.front() == ' ' || z.front() == '\t')) { z.remove_prefix(1); }
    return true;
}
// InsideQuotes: Leer weg, ein fuehrendes ", Rest der Zeile, Leer am Ende weg, ein " am Ende weg
std::string inAnfuehrung(std::string_view z) {
    while (!z.empty() && (z.front() == ' ' || z.front() == '\t')) { z.remove_prefix(1); }
    if (!z.empty() && z.front() == '"') { z.remove_prefix(1); }
    std::string s(z);
    if (!s.empty()) {
        while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) { s.pop_back(); }
        if (!s.empty() && s.back() == '"') { s.pop_back(); }
    }
    return s;
}
// ConvertCRLiterals_Read und CopeWithDumbStringData (nur die SBCS-Sprachen)
std::string aufbereiten(std::string s, const std::string& sprache) {
    for (std::size_t i; (i = s.find("\\n")) != std::string::npos;) { s.replace(i, 2, "\n"); }
    static const char* const sbcs[] = {"ENGLISH", "FRENCH", "GERMAN", "ITALIAN", "SPANISH", "POLISH", "RUSSIAN"};
    bool ja = false;
    for (const char* x : sbcs) { ja = ja || gross(sprache) == x; }
    if (!ja) { return s; }
    std::string r;
    for (std::size_t i = 0; i < s.size(); ++i) {
        const auto c = static_cast<unsigned char>(s[i]);
        if (c == 0x85) { r += "..."; continue; }   // Auslassungszeichen
        char o = s[i];
        if (c == 0x92 || c == 0x91) { o = '\''; }
        else if (c == 0x0B) { o = '.'; }
        else if (c == 0x93 || c == 0x94) { o = '"'; }
        else if (c == 0x96 || c == 0x97) { o = '-'; }
        else if (c == 0x09) { o = ' '; }
        r += o;
    }
    for (std::size_t i; (i = r.find("?.")) != std::string::npos;) { r[i + 1] = ' '; }
    return r;
}
}  // namespace

// CStringEdPackage::ParseLine (stringed_ingame.cpp) Zeile fuer Zeile
void Texte::ladeStr(const std::string& dateiname, const std::string& text, const std::string& sprache) {
    std::string paket = dateiname;
    const std::size_t s = paket.find_last_of("/\\");
    if (s != std::string::npos) { paket = paket.substr(s + 1); }
    const std::size_t p = paket.find('.');
    if (p != std::string::npos) { paket = paket.substr(0, p); }
    paket = gross(paket);
    const bool englischLaden = gross(sprache) == "ENGLISH";
    std::string ref;
    std::string englisch;
    zeilen(text, [&](const std::string& roh) {
        const std::string ohne = ohneKommentar(roh);
        std::string_view z = ohne;
        while (!z.empty() && (z.front() == ' ' || z.front() == '\t')) { z.remove_prefix(1); }
        if (schluessel("REFERENCE", z)) {
            ref = paket + "_" + gross(inAnfuehrung(z));
            englisch.clear();
            return;
        }
        if (ref.empty() || z.size() < 5) { return; }
        std::string_view l = z;
        if (!schluessel("LANG_", l)) { return; }
        std::size_t e = 0;
        while (e < l.size() && l[e] != ' ' && l[e] != '\t') { ++e; }
        const std::string diese(l.substr(0, e));
        const std::string satz = aufbereiten(inAnfuehrung(l.substr(e)), diese);
        if (englischLaden) {
            werte[ref] = satz;
        } else if (gross(diese) == "ENGLISH") {
            werte[ref] = satz;   // SetString(..., bEnglishDebug)
            englisch = satz;
        } else {
            werte[ref] = satz == "#same" ? englisch : satz;
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

namespace {
// ConvertCRLiterals_Write: Zeilenumbruch als \n, " als ' (StringEd kennt kein Entkommen)
std::string fuerDatei(const std::string& s) {
    std::string r;
    for (const char c : s) {
        if (c == '\n') { r += "\\n"; }
        else if (c == '"') { r += '\''; }
        else { r += c; }
    }
    return r;
}
}  // namespace

std::string schreibeStr(const std::string& sprache, const std::vector<TextEintrag>& e) {
    const std::string sp = gross(sprache);
    std::string s = "VERSION \"1\"\r\nCONFIG W:\\bin\\striped.cfg\r\nFILENOTES \"\"\r\n\r\n";
    for (const TextEintrag& x : e) {
        s += "REFERENCE           " + gross(x.schluessel) + "\r\n";
        s += "LANG_ENGLISH        \"" + fuerDatei(x.englisch) + "\"\r\n";
        if (sp != "ENGLISH") { s += "LANG_" + sp + "    \"" + fuerDatei(x.uebersetzung.empty() ? std::string("#same") : x.uebersetzung) + "\"\r\n"; }
        s += "\r\n";
    }
    return s + "ENDMARKER\r\n";
}

std::string schreibeSpSprachen(const std::string& paket, const std::vector<TextEintragSprachen>& e) {
    std::string s = "VERSION 1\r\nCONFIG W:\\bin\\striped.cfg\r\nID 255\r\nREFERENCE " + gross(paket) + "\r\nDESCRIPTION \"\"\r\nCOUNT " +
                    std::to_string(e.size()) + "\r\n";
    for (std::size_t i = 0; i < e.size(); ++i) {
        s += "INDEX " + std::to_string(i) + "\r\n{\r\n   REFERENCE " + gross(e[i].schluessel) + "\r\n";
        for (const auto& [nr, t] : e[i].texte) {
            if (nr == 1 || !t.empty()) { s += "   TEXT_LANGUAGE" + std::to_string(nr) + " \"" + fuerDatei(t) + "\"\r\n"; }
        }
        s += "}\r\n";
    }
    return s;
}

std::string schreibeSp(const std::string& paket, int sprache, const std::vector<TextEintrag>& e) {
    std::string s = "VERSION 1\r\nCONFIG W:\\bin\\striped.cfg\r\nID 255\r\nREFERENCE " + gross(paket) + "\r\nDESCRIPTION \"\"\r\nCOUNT " +
                    std::to_string(e.size()) + "\r\n";
    for (std::size_t i = 0; i < e.size(); ++i) {
        s += "INDEX " + std::to_string(i) + "\r\n{\r\n   REFERENCE " + gross(e[i].schluessel) + "\r\n   TEXT_LANGUAGE1 \"" + fuerDatei(e[i].englisch) + "\"\r\n";
        if (sprache != 1) { s += "   TEXT_LANGUAGE" + std::to_string(sprache) + " \"" + fuerDatei(e[i].uebersetzung) + "\"\r\n"; }
        s += "}\r\n";
    }
    return s;
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
