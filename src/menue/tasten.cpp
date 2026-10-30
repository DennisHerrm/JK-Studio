// tasten.cpp - Menue-Editor M7: Tastenbelegung wie im Spiel (Herkunft und Lizenz im Kopf von
// include/bhed/menue/tasten.h).
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
#include "bhed/menue/tasten.h"

#include <cctype>
#include <cstdio>
#include <iterator>

namespace bhed::menue {

namespace {

struct TastenName {
    unsigned upper, lower;
    const char* name;
    int keynum;
};
#include "tasten.inc"

constexpr int kMaxTasten = static_cast<int>(std::size(kTastenNamen));   // MAX_KEYS

bool gleich(const std::string& a, const std::string& b) {   // Q_stricmp
    if (a.size() != b.size()) { return false; }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) { return false; }
    }
    return true;
}

// Cmd_TokenizeString: Woerter, "..." zusammen, // beendet die Zeile
std::vector<std::string> woerter(const std::string& z) {
    std::vector<std::string> w;
    std::size_t p = 0;
    while (p < z.size()) {
        while (p < z.size() && static_cast<unsigned char>(z[p]) <= ' ') { ++p; }
        if (p >= z.size()) { break; }
        if (z.compare(p, 2, "//") == 0) { break; }
        std::string t;
        if (z[p] == '"') {
            ++p;
            while (p < z.size() && z[p] != '"') { t += z[p++]; }
            if (p < z.size()) { ++p; }
        } else {
            while (p < z.size() && static_cast<unsigned char>(z[p]) > ' ') { t += z[p++]; }
        }
        w.push_back(t);
    }
    return w;
}

void setze(Tastenbelegung& b, int nr, const std::string& was) {   // Key_SetBinding: unter keynames[keynum].upper
    if (nr < 0 || nr >= kMaxTasten) { return; }
    const auto i = static_cast<std::size_t>(kTastenNamen[nr].upper);
    if (i < b.bindung.size()) { b.bindung[i] = was; }
}

}  // namespace

Tastenbelegung::Tastenbelegung() : bindung(static_cast<std::size_t>(kMaxTasten)) {}

int tastenNummer(const std::string& s) {
    if (s.empty()) { return -1; }
    if (s.size() == 1) { return static_cast<int>(kTastenNamen[static_cast<unsigned char>(s[0])].upper); }
    for (int i = 0; i < kMaxTasten; ++i) {
        if (kTastenNamen[i].name != nullptr && gleich(s, kTastenNamen[i].name)) { return kTastenNamen[i].keynum; }
    }
    if (s.size() == 4) {   // Com_HexStrToInt ("0x41")
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
            int n = 0;
            for (std::size_t i = 2; i < 4; ++i) {
                const int c = std::tolower(static_cast<unsigned char>(s[i]));
                if (c >= '0' && c <= '9') { n = n * 16 + (c - '0'); }
                else if (c >= 'a' && c <= 'f') { n = n * 16 + (c - 'a' + 10); }
                else { return -1; }
            }
            return n;
        }
    }
    return -1;
}

std::string tastenName(int k) {
    if (k == -1) { return "<KEY NOT FOUND>"; }
    if (k < 0 || k >= kMaxTasten) { return "<OUT OF RANGE>"; }
    if (kTastenNamen[k].name != nullptr) { return kTastenNamen[k].name; }
    if (k > 0 && k < 256 && kTastenNamen[k].lower != 0) {   // Key_KeyToAscii (SPACE/ENTER haben Namen)
        return std::string(1, static_cast<char>(kTastenNamen[k].upper));
    }
    char b[8];
    std::snprintf(b, sizeof(b), "0x%c%c", "0123456789ABCDEF"[(k >> 4) & 15], "0123456789ABCDEF"[k & 15]);   // Key_KeyToHex
    return b;
}

void configAusfuehren(const std::string& text, Tastenbelegung& b, const std::function<bool(const std::string&, std::string&)>& lader, int tiefe) {
    // Cbuf: Befehle enden an ";" ausserhalb von Anfuehrungszeichen und am Zeilenende
    std::string befehl;
    bool zitat = false;
    auto ausfuehren = [&](const std::string& z) {
        const std::vector<std::string> w = woerter(z);
        if (w.empty()) { return; }
        if (gleich(w[0], "bind") && w.size() >= 3) {   // Key_Bind_f: Cmd_ArgsFrom(2)
            std::string was = w[2];
            for (std::size_t i = 3; i < w.size(); ++i) { was += " " + w[i]; }
            setze(b, tastenNummer(w[1]), was);
        } else if (gleich(w[0], "unbind") && w.size() == 2) {
            setze(b, tastenNummer(w[1]), "");
        } else if (gleich(w[0], "unbindall")) {
            for (std::string& x : b.bindung) { x.clear(); }
        } else if (gleich(w[0], "exec") && w.size() >= 2 && tiefe < 8 && lader) {
            std::string inhalt;
            std::string datei = w[1];
            if (datei.find('.') == std::string::npos) { datei += ".cfg"; }   // COM_DefaultExtension
            if (lader(datei, inhalt)) { configAusfuehren(inhalt, b, lader, tiefe + 1); }
        }
    };
    for (std::size_t i = 0; i <= text.size(); ++i) {
        const char c = i < text.size() ? text[i] : '\n';
        if (c == '"') { zitat = !zitat; }
        if ((c == ';' && !zitat) || c == '\n' || c == '\r') {
            ausfuehren(befehl);
            befehl.clear();
            if (c != ';') { zitat = false; }
            continue;
        }
        befehl += c;
    }
}

std::string vorgabeConfig(Dialekt d, Spiel s) {
    if (s == Spiel::MD) { return d == Dialekt::MP ? "MovieDuels-MP-default.cfg" : "MovieDuels-SP-default.cfg"; }
    return d == Dialekt::MP ? "mpdefault.cfg" : "default.cfg";
}

std::string bindungsText(const Tastenbelegung& b, const std::string& befehl, Dialekt d, Spiel s,
                         const std::function<std::string(const std::string&)>& se) {
    const char* const* liste = kBindSp;
    std::size_t anzahl = std::size(kBindSp);
    if (s == Spiel::MD && d == Dialekt::SP) {
        liste = kBindMd;
        anzahl = std::size(kBindMd);
    } else if (d == Dialekt::MP) {
        liste = kBindMp;
        anzahl = std::size(kBindMp);
    } else if (s == Spiel::JK2) {
        liste = kBindJk2;
        anzahl = std::size(kBindJk2);
    }
    for (std::size_t i = 0; i < anzahl; ++i) {
        if (!gleich(befehl, liste[i])) { continue; }
        int taste[2] = {-1, -1};   // Controls_GetKeyAssignment: die ersten zwei Tasten in Nummernfolge
        int n = 0;
        for (int j = 0; j < kMaxTasten && n < 2; ++j) {
            const std::string& x = b.bindung[static_cast<std::size_t>(j)];
            if (!x.empty() && gleich(x, befehl)) { taste[n++] = j; }
        }
        if (taste[0] == -1) { break; }
        auto name = [&](int k) {   // Key_KeynumToStringBuf: KEYNAMES_KEYNAME_<name>, sonst der Name
            const std::string roh = tastenName(k);
            const std::string freund = se ? se("KEYNAMES_KEYNAME_" + roh) : std::string();
            return freund.empty() ? roh : freund;
        };
        if (taste[1] == -1) { return name(taste[0]); }
        std::string oder = se ? se(s == Spiel::JK2 && d == Dialekt::SP ? "MENUS3_KEYBIND_OR" : "MENUS_KEYBIND_OR") : std::string("or");
        if (oder.size() > 31) { oder.resize(31); }   // char sOR[32]
        return name(taste[0]) + " " + oder + " " + name(taste[1]);
    }
    return "???";
}

}  // namespace bhed::menue
