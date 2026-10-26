// bspoeffnen.cpp - Karten-Editor, Stufe 4: fertige Karten oeffnen und das
// Originallicht behalten (siehe bspoeffnen.h).
//
// Portiert aus NetRadiant-Custom 20260114 (radiant/map.cpp:1539-1622) und
// q3map2 (bsp.cpp:570-605, 614-650, 870-897; writebsp.cpp UnparseEntities;
// bspfile_ibsp.cpp, bspfile_rbsp.cpp).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/bspoeffnen.h"

#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/texturen.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <map>

namespace bhed::karte {

namespace {

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

bool endetAufBsp(const std::string& p) {
    return p.size() > 4 && klein(p.substr(p.size() - 4)) == ".bsp";
}

// Quoted( s ) (stream/stringstream.h)
std::string inAnfuehrung(const std::string& s) { return "\"" + s + "\""; }

std::string gemeinsamerAnfang(const Q3map2Aufruf& a) {
    return inAnfuehrung(a.programm) + " -v -game " + (a.spielTyp.empty() ? std::string("quake3") : a.spielTyp) + " -fs_basepath " +
           inAnfuehrung(a.enginePfad) + " -fs_homepath " + inAnfuehrung(a.benutzerPfad) + " -fs_game " + a.fsGame;
}

// FNV-1a, 64 Bit
void mischen(std::uint64_t& h, const std::string& s) {
    for (const char c : s) {
        h ^= static_cast<unsigned char>(c);
        h *= 1099511628211ULL;
    }
    h ^= 0xFFU;   // Trenner
    h *= 1099511628211ULL;
}

std::int32_t leseInt(const std::string& d, std::size_t o) {
    std::uint32_t v = 0;
    for (std::size_t i = 0; i < 4; ++i) { v |= static_cast<std::uint32_t>(static_cast<unsigned char>(d[o + i])) << (8 * i); }
    return static_cast<std::int32_t>(v);
}

}  // namespace

std::vector<BspEintrag> bspListe(const Dateisystem& fs) {
    std::vector<BspEintrag> out;
    std::map<std::string, std::size_t> gesehen;
    for (const std::string& archiv : fs.archive(false)) {
        fs.dateienImArchiv(archiv, "maps/", 8, [&](const std::string& pfad) {
            if (!endetAufBsp(pfad)) { return; }
            const std::string k = klein(pfad);
            if (gesehen.count(k) != 0U) { return; }   // ein frueheres Archiv verdeckt es
            gesehen.emplace(k, out.size());
            out.push_back({pfad, archiv});
        });
    }
    std::sort(out.begin(), out.end(), [](const BspEintrag& a, const BspEintrag& b) { return klein(a.pfad) < klein(b.pfad); });
    return out;
}

std::string convertBefehl(const Q3map2Aufruf& a, bool brushPrimitives, const std::string& bsp) {
    // map.cpp:1585-1594: ... << " -convert -format " << ( QUAKE ? "map" : "map_bp" ) << ' ' << Quoted( filename )
    return gemeinsamerAnfang(a) + " -convert -format " + (brushPrimitives ? "map_bp" : "map") + " " + inAnfuehrung(bsp);
}

std::string convertZiel(const std::string& bsp) {
    const std::size_t punkt = bsp.rfind('.');
    const std::size_t trenner = bsp.find_last_of("/\\");
    const std::string stamm = (punkt == std::string::npos || (trenner != std::string::npos && punkt < trenner)) ? bsp : bsp.substr(0, punkt);
    return stamm + "_converted.map";   // PathExtensionless( filename ) + "_converted.map"
}

std::string onlyentsBefehl(const Q3map2Aufruf& a, const std::string& map, bool lichterBehalten) {
    return gemeinsamerAnfang(a) + " -bsp -onlyents " + (lichterBehalten ? "-keeplights " : "") + inAnfuehrung(map);
}

std::string bhedModelleBefehl(const Q3map2Aufruf& a, const std::vector<std::string>& pakPfade, const std::string& liste, const std::string& aus) {
    std::string s = gemeinsamerAnfang(a);
    for (const std::string& p : pakPfade) {
        if (!p.empty()) { s += " -fs_pakpath " + inAnfuehrung(p); }
    }
    return s + " -bhedmodels " + inAnfuehrung(liste) + " " + inAnfuehrung(aus);
}

bool eingebackeneEntitaet(const std::string& klassenname) {
    // UnparseEntities (writebsp.cpp): striEqual( classname, "misc_model" / "_decal" / "_skybox" )
    const std::string k = klein(klassenname);
    return k == "misc_model" || k == "_decal" || k == "_skybox";
}

std::uint64_t geometrieFingerabdruck(const Karte& k) {
    std::uint64_t h = 1469598103934665603ULL;
    // Entitaeten mit Primitiven: nur die Primitive (Schluessel und Kommentare
    // wie "// entity N" fallen weg - neue Punkt-Entities verschieben die Nummern)
    const std::string pinsel = karteSchreiben(
        k, [](const Entitaet& e) { return !e.kinder().empty() && !eingebackeneEntitaet(e.klassenname()); },
        [](const Entitaet&, const Primitiv&) { return true; });
    std::size_t a = 0;
    while (a < pinsel.size()) {
        std::size_t b = pinsel.find('\n', a);
        if (b == std::string::npos) { b = pinsel.size(); }
        const std::string zeile = pinsel.substr(a, b - a);
        if (!zeile.empty() && zeile[0] != '"' && zeile.rfind("//", 0) != 0) { mischen(h, zeile); }
        a = b + 1;
    }
    // eingebackene Entitaeten ganz (ihre Schluessel sind Geometrie)
    const std::string gebacken = karteSchreiben(k, [](const Entitaet& e) { return eingebackeneEntitaet(e.klassenname()); },
                                                [](const Entitaet&, const Primitiv&) { return true; });
    a = 0;
    while (a < gebacken.size()) {
        std::size_t b = gebacken.find('\n', a);
        if (b == std::string::npos) { b = gebacken.size(); }
        const std::string zeile = gebacken.substr(a, b - a);
        if (!zeile.empty() && zeile.rfind("//", 0) != 0) { mischen(h, zeile); }
        a = b + 1;
    }
    return h;
}

bool bspLesen(const std::string& daten, BspDatei& out) {
    out = BspDatei{};
    if (daten.size() < 8) { return false; }
    out.ident = daten.substr(0, 4);
    out.version = leseInt(daten, 4);
    // bspHeader_t / rbspHeader_t: ident, version, lumps[HEADER_LUMPS] (offset, length)
    const std::size_t anzahl = out.ident == "RBSP" ? 18 : (out.ident == "IBSP" ? 17 : 0);
    if (anzahl == 0 || daten.size() < 8 + anzahl * 8) { return false; }
    for (std::size_t i = 0; i < anzahl; ++i) {
        const std::int32_t o = leseInt(daten, 8 + i * 8);
        const std::int32_t l = leseInt(daten, 12 + i * 8);
        if (o < 0 || l < 0 || static_cast<std::size_t>(o) + static_cast<std::size_t>(l) > daten.size()) { return false; }
        out.lumps.push_back(daten.substr(static_cast<std::size_t>(o), static_cast<std::size_t>(l)));
    }
    return true;
}

std::string bspEntitaetenText(const BspDatei& b) {
    if (b.lumps.empty()) { return {}; }
    std::string t = b.lumps[0];
    const std::size_t nul = t.find('\0');
    if (nul != std::string::npos) { t.resize(nul); }
    return t;
}

bool bspHatLichter(const std::string& text) {
    // "classname" "light..." (SetLightStyles: classname_prefixed( "light" ))
    std::size_t p = 0;
    while ((p = text.find("\"classname\"", p)) != std::string::npos) {
        p += 11;
        const std::size_t a = text.find('"', p);
        if (a == std::string::npos) { break; }
        if (klein(text.substr(a + 1, 5)) == "light") { return true; }
        p = a + 1;
    }
    return false;
}

std::vector<std::pair<std::string, int>> schaltlichter(const std::string& text, bool ausBsp) {
    // Entity_parseTokens vereinfacht: { "schluessel" "wert" ... { Pinsel } }; Pinsel werden uebersprungen
    std::vector<std::pair<std::string, int>> out;
    std::size_t i = 0;
    const std::size_t n = text.size();
    const auto wort = [&](std::string& w) -> bool {
        while (true) {   // Leerraum und // Kommentare
            while (i < n && std::isspace(static_cast<unsigned char>(text[i])) != 0) { ++i; }
            if (i < n && text[i] == '/' && i + 1 < n && text[i + 1] == '/') {
                while (i < n && text[i] != '\n') { ++i; }
                continue;
            }
            break;
        }
        if (i >= n) { return false; }
        if (text[i] == '"') {
            const std::size_t e = text.find('"', i + 1);
            w = text.substr(i + 1, (e == std::string::npos ? n : e) - i - 1);
            i = e == std::string::npos ? n : e + 1;
            return true;
        }
        const std::size_t a = i;
        while (i < n && std::isspace(static_cast<unsigned char>(text[i])) == 0) { ++i; }
        w = text.substr(a, i - a);
        return true;
    };
    std::string w;
    int tiefe = 0;
    std::map<std::string, std::string> ent;
    while (wort(w)) {
        if (w == "{") {
            if (++tiefe == 1) { ent.clear(); }
            continue;
        }
        if (w == "}") {
            if (--tiefe == 0) {
                const std::string klasse = klein(ent["classname"]);
                const std::string t = ent["targetname"];
                if (klasse.rfind("light", 0) == 0 && !t.empty()) {
                    const int stil = std::atoi((ausBsp ? ent["switch_style"] : ent["style"]).c_str());
                    const auto paar = std::make_pair(t, stil);
                    if (std::find(out.begin(), out.end(), paar) == out.end()) { out.push_back(paar); }
                }
            }
            tiefe = std::max(tiefe, 0);
            continue;
        }
        if (tiefe == 1) {
            std::string wert;
            if (!wort(wert)) { break; }
            ent[w] = wert;
        }
    }
    return out;
}

std::string bspEntitaetenEinsetzen(const std::string& original, const std::string& neu) {
    BspDatei a;
    BspDatei b;
    if (!bspLesen(original, a) || !bspLesen(neu, b) || a.ident != b.ident || a.version != b.version) { return {}; }
    const std::size_t anzahl = a.lumps.size();
    // Reihenfolge der Lumps im Original (nach Versatz)
    std::vector<std::size_t> reihe(anzahl);
    std::vector<std::int32_t> versatz(anzahl);
    for (std::size_t i = 0; i < anzahl; ++i) {
        reihe[i] = i;
        versatz[i] = leseInt(original, 8 + i * 8);
    }
    std::stable_sort(reihe.begin(), reihe.end(), [&](std::size_t x, std::size_t y) { return versatz[x] < versatz[y]; });
    std::string out = original.substr(0, 8 + anzahl * 8);   // ident, version, Platz fuer die Lumps
    const auto schreibeInt = [&out](std::size_t o, std::uint32_t v) {
        for (std::size_t i = 0; i < 4; ++i) { out[o + i] = static_cast<char>((v >> (8 * i)) & 0xFFU); }
    };
    for (const std::size_t i : reihe) {
        const std::string& daten = i == 0 ? b.lumps[0] : a.lumps[i];
        schreibeInt(8 + i * 8, static_cast<std::uint32_t>(out.size()));
        schreibeInt(12 + i * 8, static_cast<std::uint32_t>(daten.size()));
        out += daten;
        while (out.size() % 4 != 0) { out += '\0'; }   // AddLump: SafeWrite( file, padding, ( length + 3 ) & ~3 )
    }
    return out;
}

}  // namespace bhed::karte
