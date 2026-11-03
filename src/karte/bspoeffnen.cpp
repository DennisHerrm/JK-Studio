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
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <utility>
#include <vector>

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


// --- rc609 W2a: OnlyEnts ohne q3map2 (siehe bspoeffnen.h) ---------------------------------
namespace {

// StripTrailing (bspfile_abstract.cpp:343): end[-1] <= 32 mit char (vorzeichenbehaftet unter
// MSVC und clang-cl) - auch Bytes ab 0x80 fallen am Ende weg.
std::string ohneEnde(const std::string& s) {
    std::size_t e = s.size();
    while (e > 0 && static_cast<signed char>(s[e - 1]) <= 32) { --e; }
    return s.substr(0, e);
}
bool gleichOhneGross(const std::string& a, const std::string& b) { return klein(a) == klein(b); }   // EPAIR_EQUAL = striEqual
const std::string* wertVon(const EntitaetPaare& e, const std::string& k) {   // entity_t::valueForKey
    for (const auto& p : e) {
        if (gleichOhneGross(p.first, k)) { return &p.second; }
    }
    return nullptr;
}
void wertSetzen(EntitaetPaare& e, const std::string& k, const std::string& v) {   // entity_t::setKeyValue
    for (auto& p : e) {
        if (gleichOhneGross(p.first, k)) {
            p.second = v;
            return;
        }
    }
    e.emplace_back(k, v);
}
bool nichtLeer(const std::string* w) { return w != nullptr && !w->empty(); }

// Die erste Entity (worldspawn) eines Entity-Lumps wie ParseEntities/ParseEPair.
EntitaetPaare ersteEntitaet(const std::string& text) {
    EntitaetPaare aus;
    std::size_t i = text.find('{');
    const std::size_t ende = i == std::string::npos ? std::string::npos : text.find('}', i);
    while (i != std::string::npos && ende != std::string::npos && i < ende) {
        const std::size_t a = text.find('"', i);
        const std::size_t b = a == std::string::npos ? a : text.find('"', a + 1);
        const std::size_t c = b == std::string::npos ? b : text.find('"', b + 1);
        const std::size_t d = c == std::string::npos ? c : text.find('"', c + 1);
        if (d == std::string::npos || d > ende) { break; }
        std::string k = ohneEnde(text.substr(a + 1, b - a - 1));
        std::string v = ohneEnde(text.substr(c + 1, d - c - 1));
        if (!k.empty() && !v.empty()) { aus.emplace_back(std::move(k), std::move(v)); }
        i = d + 1;
    }
    return aus;
}

}  // namespace

std::string nurEntitaetenKarte(const std::vector<EntitaetPaare>& entitaeten) {
    std::string s;
    for (std::size_t i = 0; i < entitaeten.size(); ++i) {
        s += "// entity " + std::to_string(i) + "\n{\n";
        for (const auto& [k, v] : entitaeten[i]) { s += "\"" + k + "\" \"" + v + "\"\n"; }
        s += "}\n";
    }
    return s;
}

std::string bspNurEntitaetenLump(const std::string& original, const std::vector<EntitaetPaare>& entitaeten, std::string* fehler) {
    const auto nein = [fehler](const std::string& t) {
        if (fehler != nullptr) { *fehler = t; }
        return std::string();
    };
    BspDatei alt;
    if (!bspLesen(original, alt)) { return nein("not a BSP file"); }
    if (entitaeten.empty()) { return nein("No entity (worldspawn missing)"); }   // LoadMapFile, map.cpp:1716
    const std::string altText = bspEntitaetenText(alt);
    // OnlyEnts: _q3map2_cmdline, _q3map2_version, gridsize der alten worldspawn
    const EntitaetPaare altWelt = ersteEntitaet(altText);
    // LoadMapFile -> ParseMapEntity -> ParseEPair: StripTrailing, leere fallen weg
    std::vector<EntitaetPaare> ents;
    ents.reserve(entitaeten.size());
    for (const EntitaetPaare& e : entitaeten) {
        EntitaetPaare n;
        for (const auto& [k, v] : e) {
            std::string kk = ohneEnde(k);
            std::string vv = ohneEnde(v);
            if (!kk.empty() && !vv.empty()) { n.emplace_back(std::move(kk), std::move(vv)); }
        }
        ents.push_back(std::move(n));
    }
    // SetLightStyles (writebsp.cpp:232-300), keepLights wie der Karten-Editor (-keeplights, wenn
    // die Original-.bsp Lichter hat: bspNurEntitaeten, gui/app_karte_bsp.cpp)
    constexpr int kMaxSwitchedLights = 32;   // q3map2.h:193 MAX_SWITCHED_LIGHTS
    constexpr int kLsNone = 0xFF;            // q3map2.h:196 LS_NONE
    if (bspHatLichter(altText)) { wertSetzen(ents[0], "_keepLights", "1"); }
    bool keepLights = false;
    if (const std::string* w = wertVon(ents[0], "_keepLights"); nichtLeer(w)) { keepLights = (*w)[0] == '1'; }
    const bool rbsp = alt.ident == "RBSP";   // g_game->load == LoadRBSPFile
    std::vector<std::pair<std::string, int>> stile;
    for (std::size_t i = 1; i < ents.size(); ++i) {
        EntitaetPaare& e = ents[i];
        const std::string* kl = wertVon(e, "classname");
        if (kl == nullptr || klein(*kl).rfind("light", 0) != 0) { continue; }   // classname_prefixed( "light" )
        const std::string* t = wertVon(e, "targetname");
        if (!(nichtLeer(t) && rbsp)) {
            if (!keepLights) { e.clear(); }
            continue;
        }
        const std::string ziel = *t;
        const std::string* sw = wertVon(e, "style");
        const int stil = nichtLeer(sw) ? std::atoi(sw->c_str()) : 0;   // intForKey
        if (stil < 0 || stil >= kLsNone) { return nein("Invalid lightstyle (" + std::to_string(stil) + ") on entity " + std::to_string(i)); }
        std::size_t j = 0;
        while (j < stile.size() && !(stile[j].second == stil && stile[j].first == ziel)) { ++j; }
        if (j >= stile.size()) {
            if (static_cast<int>(stile.size()) == kMaxSwitchedLights) {
                return nein("MAX_SWITCHED_LIGHTS (32) exceeded, reduce the number of lights with targetnames");
            }
            stile.emplace_back(ziel, stil);
        }
        wertSetzen(e, "style", std::to_string(kMaxSwitchedLights + static_cast<int>(j)));
        if (stil != 0) { wertSetzen(e, "switch_style", std::to_string(stil)); }   // LS_NORMAL
    }
    for (const char* k : {"_q3map2_cmdline", "_q3map2_version", "gridsize"}) {
        if (const std::string* w = wertVon(altWelt, k); nichtLeer(w)) { wertSetzen(ents[0], k, *w); }
    }
    // UnparseEntities (bspfile_abstract.cpp:451-493)
    bool keepModels = false;
    if (const std::string* w = wertVon(ents[0], "_keepModels"); nichtLeer(w)) { keepModels = (*w)[0] == '1'; }
    std::string lump;
    for (const EntitaetPaare& e : ents) {
        if (e.empty()) { continue; }
        const std::string* kl = wertVon(e, "classname");
        const std::string k = kl != nullptr ? klein(*kl) : std::string();
        if ((k == "misc_model" && !keepModels) || k == "_decal" || k == "_skybox") { continue; }
        lump += "{\n";
        for (const auto& [kk, vv] : e) { lump += "\"" + ohneEnde(kk) + "\" \"" + ohneEnde(vv) + "\"\n"; }
        lump += "}\n";
    }
    lump += '\0';
    return lump;
}

std::string bspNurEntitaetenEinsetzen(const std::string& original, const std::vector<EntitaetPaare>& entitaeten, std::string* fehler) {
    const std::string lump = bspNurEntitaetenLump(original, entitaeten, fehler);
    BspDatei b;
    if (lump.empty() || !bspLesen(original, b)) { return {}; }
    // Eine BSP derselben Art, die nur den neuen Entity-Lump traegt - dann wie der Karten-Editor einsetzen.
    const std::size_t anzahl = b.lumps.size();
    std::string kopf(anzahl * 8, '\0');
    const auto schreibeInt = [&kopf](std::size_t o, std::uint32_t v) {
        for (std::size_t i = 0; i < 4; ++i) { kopf[o + i] = static_cast<char>((v >> (8 * i)) & 0xFFU); }
    };
    const auto anfang = static_cast<std::uint32_t>(8 + anzahl * 8);
    schreibeInt(0, anfang);
    schreibeInt(4, static_cast<std::uint32_t>(lump.size()));
    for (std::size_t i = 1; i < anzahl; ++i) { schreibeInt(i * 8, anfang + static_cast<std::uint32_t>(lump.size())); }
    return bspEntitaetenEinsetzen(original, original.substr(0, 8) + kopf + lump);
}

std::string q3map2TypFuerBsp(const std::string& daten, const std::string& eigenerTyp) {
    if (daten.size() < 8) { return eigenerTyp; }
    const std::string ident = daten.substr(0, 4);
    const int version = static_cast<int>(static_cast<unsigned char>(daten[4])) | (static_cast<int>(static_cast<unsigned char>(daten[5])) << 8) |
                        (static_cast<int>(static_cast<unsigned char>(daten[6])) << 16) | (static_cast<int>(static_cast<unsigned char>(daten[7])) << 24);
    if (ident == "IBSP" && version == 46) { return "quake3"; }
    if (ident == "IBSP" && version == 47) { return "wolf"; }
    if (ident == "FBSP") { return "qfusion"; }
    return eigenerTyp;
}

}  // namespace bhed::karte
