// bsplumps.cpp - rc609 J: Shader-, Fog- und Flaechen-Lump fertiger Karten (siehe bsplumps.h).
//
// Formate (q3map2 bspfile_ibsp.cpp / bspfile_rbsp.cpp, OJK code/qcommon/qfiles.h):
//   dshader_t    char shader[64]; int surfaceFlags; int contentFlags;            72 Bytes
//   dfog_t       char shader[64]; int brushNum; int visibleSide;                  72 Bytes
//   dbrush_t     int firstSide, numSides, shaderNum;                              12 Bytes
//   dbrushside_t IBSP: int planeNum, shaderNum;  RBSP: dazu int drawSurfNum       8 / 12 Bytes
//   dsurface_t   int shaderNum, fogNum, surfaceType, ...; lightmapNum[0] bei
//                IBSP an Stelle 28 (104 Bytes), bei RBSP nach lightmapStyles[4] und
//                vertexStyles[4] an Stelle 36 (148 Bytes)
// Lumps: 1 Shader, 8 Brushes, 9 Brush-Seiten, 12 Fogs, 13 Flaechen (IBSP und RBSP gleich).
//
// Pruefungen wie die Engine laedt (OJK code/, MD9 code/ gleich, JK2-Modus von OpenJK ebenso):
//   - cm_load.cpp CMod_LoadShaders: "funny lump size", "Map with no shaders"; Name mit
//     Q_strncpyz(..., MAX_QPATH) -> hoechstens 63 Zeichen.
//   - cm_load.cpp CMod_LoadBrushes/CMod_LoadBrushSides: "bad shaderNum" - die Brushes zeigen
//     per Nummer auf Shader, deshalb werden Shader nur angehaengt, nie entfernt.
//   - tr_bsp.cpp ShaderForShaderNum: "ShaderForShaderNum: bad num" fuer die shaderNum jeder
//     Flaeche (ParseFace, ParseMesh, ParseTriSurf, ParseFlare).
//   - tr_bsp.cpp ParseFace & Co.: fogIndex = fogNum + 1 ungeprueft als Index in worldData.fogs
//     -> fogNum muss -1 .. Anzahl-1 sein.
//   - tr_bsp.cpp R_LoadFogs: brushNum -1 = global, sonst "fog brushNumber out of range" und
//     "fog brush sideNumber out of range" (firstSide > sidesCount - 6); visibleSide liest
//     sides[firstSide + sideNum] ungeprueft -> -1 oder eine Seite des Brushes. Beim globalen
//     Nebel bliebe firstSide der des vorigen Nebels stehen; q3map2 schreibt dort -1, das wird
//     verlangt.
//   - qfiles.h MAX_MAP_SHADERS 0x400.
//
// Portiert aus OpenJK (code/qcommon/cm_load.cpp CMod_LoadShaders/CMod_LoadBrushes/
// CMod_LoadBrushSides/CMod_LoadPatches, code/rd-vanilla/tr_bsp.cpp ShaderForShaderNum/
// ParseFace/R_LoadFogs, code/qcommon/qfiles.h, code/game/surfaceflags.h und
// codeJK2/game/surfaceflags.h; Movie Duels 9 ebenso) und aus q3map2 (bspfile_ibsp.cpp,
// bspfile_rbsp.cpp: dshader_t, dfog_t, dbrush_t, dbrushside_t, dsurface_t).
//
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2001 - 2013, Activision, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//
//   This file is part of the OpenJK source code. OpenJK is free software; you
//   can redistribute it and/or modify it under the terms of the GNU General
//   Public License version 2 as published by the Free Software Foundation.
//   This program is distributed in the hope that it will be useful, but WITHOUT
//   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//   FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
//   details (LICENSE).
#include "bhed/karte/bsplumps.h"

#include "bhed/karte/bspoeffnen.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace bhed::karte {

namespace {

constexpr std::size_t kLumpShader = 1;
constexpr std::size_t kLumpBrushes = 8;
constexpr std::size_t kLumpSeiten = 9;
constexpr std::size_t kLumpNebel = 12;
constexpr std::size_t kLumpFlaechen = 13;
constexpr std::size_t kEintrag = 72;   // dshader_t, dfog_t

std::size_t flaechenGroesse(bool rbsp) { return rbsp ? 148U : 104U; }
std::size_t lightmapStelle(bool rbsp) { return rbsp ? 36U : 28U; }
std::size_t seitenGroesse(bool rbsp) { return rbsp ? 12U : 8U; }

std::int32_t liesInt(const std::string& d, std::size_t o) {
    std::uint32_t v = 0;
    for (std::size_t i = 0; i < 4; ++i) { v |= static_cast<std::uint32_t>(static_cast<unsigned char>(d[o + i])) << (8 * i); }
    return static_cast<std::int32_t>(v);
}
void schreibInt(std::string& d, std::size_t o, std::uint32_t v) {
    for (std::size_t i = 0; i < 4; ++i) { d[o + i] = static_cast<char>((v >> (8 * i)) & 0xFFU); }
}
std::string liesName(const std::string& d, std::size_t o) {
    const char* p = d.data() + o;
    return std::string(p, ::strnlen(p, 64));
}
void schreibName(std::string& d, std::size_t o, const std::string& name) {
    for (std::size_t i = 0; i < 64; ++i) { d[o + i] = i < name.size() ? name[i] : '\0'; }
}

bool nameErlaubt(const std::string& n, std::string& warum) {
    if (n.empty()) {
        warum = "empty shader name";
        return false;
    }
    if (n.size() > kShaderNameMax) {
        warum = "shader name longer than 63 characters (dshader_t shader[MAX_QPATH], Q_strncpyz in CMod_LoadShaders)";
        return false;
    }
    for (const char c : n) {
        if (static_cast<unsigned char>(c) < 32 || c == '"') {
            warum = "shader name with a quote or control character";
            return false;
        }
    }
    return true;
}

// Art des Eintrags, an dem eine Pruefung scheitert (fuer die Zeile im Text).
enum class Art { Sonst, Shader, Nebel, Flaeche };

std::string pruefenIntern(const BspLumps& alt, const BspLumps& neu, Art& art, int& nr) {
    art = Art::Sonst;
    nr = -1;
    const auto fehlerBei = [&](Art a, std::size_t i, const std::string& m) {
        art = a;
        nr = static_cast<int>(i);
        return m;
    };
    if (neu.rbsp != alt.rbsp || neu.flaechen.size() != alt.flaechen.size() || neu.brushes.size() != alt.brushes.size() ||
        neu.seitenShader != alt.seitenShader) {
        return "the lumps belong to another map";
    }
    if (neu.shader.empty()) { return "Map with no shaders (CMod_LoadShaders, cm_load.cpp)"; }
    if (neu.shader.size() > kMaxMapShaders) {
        return fehlerBei(Art::Shader, kMaxMapShaders, "more than MAX_MAP_SHADERS (1024, qfiles.h) shaders");
    }
    if (neu.shader.size() < alt.shader.size()) {
        return fehlerBei(Art::Shader, neu.shader.size(),
                         "shaders can only be appended - brushes, brush sides and surfaces point to them by number "
                         "(CMod_LoadBrushes, CMod_LoadBrushSides: bad shaderNum)");
    }
    for (std::size_t i = 0; i < neu.shader.size(); ++i) {
        std::string warum;
        const bool gleich = i < alt.shader.size() && neu.shader[i].name == alt.shader[i].name;
        if (!gleich && !nameErlaubt(neu.shader[i].name, warum)) { return fehlerBei(Art::Shader, i, warum); }
    }
    if (neu.nebel.size() != alt.nebel.size()) {
        return fehlerBei(Art::Nebel, std::min(neu.nebel.size(), alt.nebel.size()),
                         "fogs can not be added or removed - surfaces point to them by number (fogNum)");
    }
    const std::size_t seiten = neu.seitenShader.size();
    for (std::size_t i = 0; i < neu.nebel.size(); ++i) {
        const LumpNebel& f = neu.nebel[i];
        std::string warum;
        if (f.shader != alt.nebel[i].shader && !nameErlaubt(f.shader, warum)) { return fehlerBei(Art::Nebel, i, warum); }
        if (f.brushNum == -1) {
            if (f.visibleSide != -1) {
                return fehlerBei(Art::Nebel, i, "a global fog (brushNum -1) has no visible side (-1, as q3map2 writes it; R_LoadFogs)");
            }
            continue;
        }
        if (f.brushNum < 0 || static_cast<std::size_t>(f.brushNum) >= neu.brushes.size()) {
            return fehlerBei(Art::Nebel, i, "fog brushNumber out of range (R_LoadFogs, tr_bsp.cpp)");
        }
        const LumpBrush& b = neu.brushes[static_cast<std::size_t>(f.brushNum)];
        if (seiten < 6 || b.ersteSeite < 0 || static_cast<std::size_t>(b.ersteSeite) > seiten - 6) {
            return fehlerBei(Art::Nebel, i, "fog brush sideNumber out of range (R_LoadFogs, tr_bsp.cpp)");
        }
        if (f.visibleSide < -1 || f.visibleSide >= b.seiten) {
            return fehlerBei(Art::Nebel, i, "visibleSide is not a side of the fog brush (-1 or 0.." + std::to_string(b.seiten - 1) +
                                                "; R_LoadFogs reads sides[firstSide + visibleSide] unchecked)");
        }
    }
    for (std::size_t i = 0; i < neu.flaechen.size(); ++i) {
        const LumpFlaeche& f = neu.flaechen[i];
        const LumpFlaeche& a = alt.flaechen[i];
        if (f.typ != a.typ || f.lightmap != a.lightmap) { return fehlerBei(Art::Flaeche, i, "only shaderNum and fogNum of a surface can change"); }
        if (f.shaderNum < 0 || static_cast<std::size_t>(f.shaderNum) >= neu.shader.size()) {
            return fehlerBei(Art::Flaeche, i, "ShaderForShaderNum: bad num " + std::to_string(f.shaderNum) + " (tr_bsp.cpp)");
        }
        if (f.fogNum < -1 || f.fogNum >= static_cast<std::int32_t>(neu.nebel.size())) {
            return fehlerBei(Art::Flaeche, i, "fogNum " + std::to_string(f.fogNum) + " is no fog of the map (ParseFace: fogIndex = fogNum + 1)");
        }
    }
    for (std::size_t i = 0; i < neu.brushes.size(); ++i) {
        const LumpBrush& b = neu.brushes[i];
        const LumpBrush& a = alt.brushes[i];
        if (b.ersteSeite != a.ersteSeite || b.seiten != a.seiten || b.shaderNum != a.shaderNum) { return "the brushes can not change"; }
    }
    return {};
}

// --- Text -----------------------------------------------------------------------------------

struct Zeile {
    std::string text;   // ohne Kommentar, ohne Rand
    int nr = 0;         // ab 1
};
std::vector<Zeile> zeilenVon(const std::string& t) {
    std::vector<Zeile> z;
    std::size_t a = 0;
    int nr = 0;
    while (a <= t.size()) {
        std::size_t e = t.find('\n', a);
        if (e == std::string::npos) { e = t.size(); }
        std::string s = t.substr(a, e - a);
        ++nr;
        bool inAnf = false;
        for (std::size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '"') { inAnf = !inAnf; }
            if (!inAnf && s[i] == '/' && i + 1 < s.size() && s[i + 1] == '/') {
                s.resize(i);
                break;
            }
        }
        std::size_t v = 0;
        while (v < s.size() && static_cast<unsigned char>(s[v]) <= ' ') { ++v; }
        std::size_t h = s.size();
        while (h > v && static_cast<unsigned char>(s[h - 1]) <= ' ') { --h; }
        z.push_back({s.substr(v, h - v), nr});
        if (e == t.size()) { break; }
        a = e + 1;
    }
    return z;
}
std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

// Eine Zahl am Anfang von s ab p: dezimal (auch mit -) oder 0x...; p dahinter.
bool zahl(const std::string& s, std::size_t& p, long long& wert) {
    while (p < s.size() && static_cast<unsigned char>(s[p]) <= ' ') { ++p; }
    const std::size_t a = p;
    bool minus = false;
    if (p < s.size() && (s[p] == '-' || s[p] == '+')) {
        minus = s[p] == '-';
        ++p;
    }
    int basis = 10;
    if (p + 1 < s.size() && s[p] == '0' && (s[p + 1] == 'x' || s[p + 1] == 'X')) {
        basis = 16;
        p += 2;
    }
    const std::size_t ziffern = p;
    unsigned long long v = 0;
    while (p < s.size()) {
        const char c = s[p];
        int d = -1;
        if (c >= '0' && c <= '9') { d = c - '0'; }
        if (basis == 16 && c >= 'a' && c <= 'f') { d = c - 'a' + 10; }
        if (basis == 16 && c >= 'A' && c <= 'F') { d = c - 'A' + 10; }
        if (d < 0) { break; }
        v = v * static_cast<unsigned>(basis) + static_cast<unsigned>(d);
        if (v > 0xFFFFFFFFULL) { return false; }
        ++p;
    }
    if (p == ziffern || (p < s.size() && static_cast<unsigned char>(s[p]) > ' ')) {
        p = a;
        return false;
    }
    wert = minus ? -static_cast<long long>(v) : static_cast<long long>(v);
    return true;
}
bool zitat(const std::string& s, std::size_t& p, std::string& out) {
    while (p < s.size() && static_cast<unsigned char>(s[p]) <= ' ') { ++p; }
    if (p >= s.size() || s[p] != '"') { return false; }
    const std::size_t e = s.find('"', p + 1);
    if (e == std::string::npos) { return false; }
    out = s.substr(p + 1, e - p - 1);
    p = e + 1;
    return true;
}
std::string hex(std::uint32_t v) {
    char b[16];
    std::snprintf(b, sizeof(b), "0x%08X", v);
    return b;
}

}  // namespace

bool bspLumpsLesen(const std::string& bsp, BspLumps& out, std::string* fehler) {
    out = BspLumps{};
    BspDatei d;
    const auto f = [fehler](const std::string& m) {
        if (fehler != nullptr) { *fehler = m; }
        return false;
    };
    if (!bspLesen(bsp, d)) { return f("not an IBSP/RBSP file"); }
    if (d.ident == "IBSP" && d.version != 46) { return f("IBSP version " + std::to_string(d.version) + " (only 46)"); }
    if (d.ident == "RBSP" && d.version != 1) { return f("RBSP version " + std::to_string(d.version) + " (only 1)"); }
    out.rbsp = d.ident == "RBSP";
    const std::string& sh = d.lumps[kLumpShader];
    const std::string& br = d.lumps[kLumpBrushes];
    const std::string& se = d.lumps[kLumpSeiten];
    const std::string& fo = d.lumps[kLumpNebel];
    const std::string& su = d.lumps[kLumpFlaechen];
    const std::size_t fg = flaechenGroesse(out.rbsp);
    const std::size_t sg = seitenGroesse(out.rbsp);
    if (sh.size() % kEintrag != 0 || fo.size() % kEintrag != 0 || br.size() % 12 != 0 || se.size() % sg != 0 || su.size() % fg != 0) {
        return f("funny lump size");   // CMod_LoadShaders, R_LoadFogs, CMod_LoadBrushes ...
    }
    for (std::size_t o = 0; o < sh.size(); o += kEintrag) {
        out.shader.push_back({liesName(sh, o), static_cast<std::uint32_t>(liesInt(sh, o + 64)), static_cast<std::uint32_t>(liesInt(sh, o + 68))});
    }
    for (std::size_t o = 0; o < fo.size(); o += kEintrag) { out.nebel.push_back({liesName(fo, o), liesInt(fo, o + 64), liesInt(fo, o + 68)}); }
    for (std::size_t o = 0; o < br.size(); o += 12) { out.brushes.push_back({liesInt(br, o), liesInt(br, o + 4), liesInt(br, o + 8)}); }
    for (std::size_t o = 0; o < se.size(); o += sg) { out.seitenShader.push_back(liesInt(se, o + 4)); }
    for (std::size_t o = 0; o < su.size(); o += fg) {
        out.flaechen.push_back({liesInt(su, o), liesInt(su, o + 4), liesInt(su, o + 8), liesInt(su, o + lightmapStelle(out.rbsp))});
    }
    return true;
}

std::string bspLumpsPruefen(const BspLumps& alt, const BspLumps& neu) {
    Art art = Art::Sonst;
    int nr = -1;
    return pruefenIntern(alt, neu, art, nr);
}

std::string bspLumpsSchreiben(const std::string& original, const BspLumps& neu, std::string* fehler) {
    const auto f = [fehler](const std::string& m) {
        if (fehler != nullptr) { *fehler = m; }
        return std::string();
    };
    BspLumps alt;
    std::string fl;
    if (!bspLumpsLesen(original, alt, &fl)) { return f(fl); }
    if (const std::string m = bspLumpsPruefen(alt, neu); !m.empty()) { return f(m); }
    BspDatei d;
    (void)bspLesen(original, d);
    // Shader: unveraenderte Eintraege Byte fuer Byte (auch was hinter der NUL steht)
    std::string sh = d.lumps[kLumpShader];
    sh.resize(neu.shader.size() * kEintrag, '\0');
    for (std::size_t i = 0; i < neu.shader.size(); ++i) {
        const std::size_t o = i * kEintrag;
        if (i >= alt.shader.size() || neu.shader[i].name != alt.shader[i].name) { schreibName(sh, o, neu.shader[i].name); }
        schreibInt(sh, o + 64, neu.shader[i].surfaceFlags);
        schreibInt(sh, o + 68, neu.shader[i].contentFlags);
    }
    std::string fo = d.lumps[kLumpNebel];
    for (std::size_t i = 0; i < neu.nebel.size(); ++i) {
        const std::size_t o = i * kEintrag;
        if (neu.nebel[i].shader != alt.nebel[i].shader) { schreibName(fo, o, neu.nebel[i].shader); }
        schreibInt(fo, o + 64, static_cast<std::uint32_t>(neu.nebel[i].brushNum));
        schreibInt(fo, o + 68, static_cast<std::uint32_t>(neu.nebel[i].visibleSide));
    }
    std::string su = d.lumps[kLumpFlaechen];
    const std::size_t fg = flaechenGroesse(neu.rbsp);
    for (std::size_t i = 0; i < neu.flaechen.size(); ++i) {
        schreibInt(su, i * fg, static_cast<std::uint32_t>(neu.flaechen[i].shaderNum));
        schreibInt(su, i * fg + 4, static_cast<std::uint32_t>(neu.flaechen[i].fogNum));
    }
    std::string out = bspMitLumps(original, {{kLumpShader, sh}, {kLumpNebel, fo}, {kLumpFlaechen, su}});
    if (out.empty()) { return f("writing the lumps failed"); }
    return out;
}

std::string lumpsAlsText(const BspLumps& l) {
    std::string t = "// shaders: nr \"name\" surfaceFlags contentFlags\n[shaders]\n";
    for (std::size_t i = 0; i < l.shader.size(); ++i) {
        t += std::to_string(i) + " \"" + l.shader[i].name + "\" " + hex(l.shader[i].surfaceFlags) + " " + hex(l.shader[i].contentFlags) + "\n";
    }
    t += "\n// fogs: nr \"shader\" brushNum visibleSide\n[fogs]\n";
    for (std::size_t i = 0; i < l.nebel.size(); ++i) {
        t += std::to_string(i) + " \"" + l.nebel[i].shader + "\" " + std::to_string(l.nebel[i].brushNum) + " " +
             std::to_string(l.nebel[i].visibleSide) + "\n";
    }
    return t;
}

bool lumpsAusText(const std::string& text, const BspLumps& basis, BspLumps& out, LumpTextFehler* fehler) {
    out = basis;
    out.shader.clear();
    out.nebel.clear();
    const auto f = [fehler](int zeile, const std::string& m) {
        if (fehler != nullptr) {
            fehler->zeile = zeile;
            fehler->text = m;
        }
        return false;
    };
    int teil = 0;   // 1 shaders, 2 fogs
    int zeileShader = 0;
    int zeileNebel = 0;
    std::vector<int> zeileJeShader;
    std::vector<int> zeileJeNebel;
    bool hatteShader = false;
    bool hatteNebel = false;
    int letzte = 1;
    for (const Zeile& z : zeilenVon(text)) {
        letzte = z.nr;
        if (z.text.empty()) { continue; }
        const std::string k = klein(z.text);
        if (k == "[shaders]") {
            if (hatteShader) { return f(z.nr, "[shaders] appears twice"); }
            hatteShader = true;
            teil = 1;
            zeileShader = z.nr;
            continue;
        }
        if (k == "[fogs]") {
            if (hatteNebel) { return f(z.nr, "[fogs] appears twice"); }
            hatteNebel = true;
            teil = 2;
            zeileNebel = z.nr;
            continue;
        }
        if (teil == 0) { return f(z.nr, "expected [shaders] or [fogs]"); }
        std::size_t p = 0;
        long long nr = 0;
        if (!zahl(z.text, p, nr)) { return f(z.nr, "expected the number of the entry"); }
        const std::size_t soll = teil == 1 ? out.shader.size() : out.nebel.size();
        if (nr != static_cast<long long>(soll)) { return f(z.nr, "expected entry number " + std::to_string(soll)); }
        std::string name;
        if (!zitat(z.text, p, name)) { return f(z.nr, "expected a quoted shader name"); }
        long long a = 0;
        long long b = 0;
        if (!zahl(z.text, p, a)) { return f(z.nr, teil == 1 ? "expected surfaceFlags (number)" : "expected brushNum (number)"); }
        if (!zahl(z.text, p, b)) { return f(z.nr, teil == 1 ? "expected contentFlags (number)" : "expected visibleSide (number)"); }
        while (p < z.text.size() && static_cast<unsigned char>(z.text[p]) <= ' ') { ++p; }
        if (p < z.text.size()) { return f(z.nr, "unexpected text after the entry"); }
        if (teil == 1) {
            if (a < -2147483648LL || b < -2147483648LL) { return f(z.nr, "number out of range"); }
            out.shader.push_back({name, static_cast<std::uint32_t>(a), static_cast<std::uint32_t>(b)});
            zeileJeShader.push_back(z.nr);
        } else {
            if (a < -2147483648LL || a > 2147483647LL || b < -2147483648LL || b > 2147483647LL) { return f(z.nr, "number out of range"); }
            out.nebel.push_back({name, static_cast<std::int32_t>(a), static_cast<std::int32_t>(b)});
            zeileJeNebel.push_back(z.nr);
        }
    }
    if (!hatteShader) { return f(letzte, "[shaders] is missing"); }
    if (!hatteNebel) { return f(letzte, "[fogs] is missing"); }
    Art art = Art::Sonst;
    int nr = -1;
    const std::string m = pruefenIntern(basis, out, art, nr);
    if (m.empty()) { return true; }
    int zeile = letzte;
    if (art == Art::Shader) {
        zeile = nr >= 0 && static_cast<std::size_t>(nr) < zeileJeShader.size() ? zeileJeShader[static_cast<std::size_t>(nr)]
                : zeileJeShader.empty()                                       ? zeileShader
                                                                              : zeileJeShader.back();
    } else if (art == Art::Nebel) {
        zeile = nr >= 0 && static_cast<std::size_t>(nr) < zeileJeNebel.size() ? zeileJeNebel[static_cast<std::size_t>(nr)]
                : zeileJeNebel.empty()                                       ? zeileNebel
                                                                             : zeileJeNebel.back();
    }
    return f(zeile, m);
}

int lumpTextZeile(const std::string& text, bool nebel, int nr) {
    int teil = 0;
    for (const Zeile& z : zeilenVon(text)) {
        const std::string k = klein(z.text);
        if (k == "[shaders]") { teil = 1; }
        if (k == "[fogs]") { teil = 2; }
        std::size_t p = 0;
        long long n = 0;
        if (teil == (nebel ? 2 : 1) && !z.text.empty() && z.text[0] != '[' && zahl(z.text, p, n) && n == nr) { return z.nr; }
    }
    return -1;
}

int lumpTextEintrag(const std::string& text, int zeile, bool& nebel) {
    int teil = 0;
    for (const Zeile& z : zeilenVon(text)) {
        const std::string k = klein(z.text);
        if (k == "[shaders]") { teil = 1; }
        if (k == "[fogs]") { teil = 2; }
        if (z.nr != zeile) { continue; }
        std::size_t p = 0;
        long long n = 0;
        if (teil == 0 || z.text.empty() || z.text[0] == '[' || !zahl(z.text, p, n)) { return -1; }
        nebel = teil == 2;
        return static_cast<int>(n);
    }
    return -1;
}

int shaderEintrag(BspLumps& l, const std::string& name, std::uint32_t surfaceFlags, std::uint32_t contentFlags) {
    for (std::size_t i = 0; i < l.shader.size(); ++i) {
        const LumpShader& s = l.shader[i];
        if (s.name == name && s.surfaceFlags == surfaceFlags && s.contentFlags == contentFlags) { return static_cast<int>(i); }
    }
    l.shader.push_back({name, surfaceFlags, contentFlags});
    return static_cast<int>(l.shader.size()) - 1;
}

ShaderNutzung shaderNutzung(const BspLumps& l, int shader) {
    ShaderNutzung n;
    for (const LumpFlaeche& f : l.flaechen) {
        if (f.shaderNum != shader) { continue; }
        ++n.flaechen;
        if (f.typ == 2) { ++n.patches; }   // MST_PATCH: CMod_LoadPatches
    }
    for (const LumpBrush& b : l.brushes) { n.brushes += b.shaderNum == shader ? 1 : 0; }
    for (const std::int32_t s : l.seitenShader) { n.seiten += s == shader ? 1 : 0; }
    return n;
}

// --- surfaceflags.h ---------------------------------------------------------------------------
// OJK code/game/surfaceflags.h:32-99, codeJK2/game/surfaceflags.h:31-98 und MD9
// code/game/surfaceflags.h: dieselben Bits und Namen (die UNUSED-Bits fehlen hier).
const std::vector<FlagName>& surfaceFlagNamen(FlagSpiel) {
    static const std::vector<FlagName> v = {
        {0x00002000U, "SURF_SKY"},      {0x00004000U, "SURF_SLICK"},      {0x00008000U, "SURF_METALSTEPS"}, {0x00010000U, "SURF_FORCEFIELD"},
        {0x00040000U, "SURF_NODAMAGE"}, {0x00080000U, "SURF_NOIMPACT"},   {0x00100000U, "SURF_NOMARKS"},    {0x00200000U, "SURF_NODRAW"},
        {0x00400000U, "SURF_NOSTEPS"},  {0x00800000U, "SURF_NODLIGHT"},   {0x01000000U, "SURF_NOMISCENTS"}, {0x02000000U, "SURF_FORCESIGHT"},
    };
    return v;
}
const std::vector<FlagName>& contentFlagNamen(FlagSpiel) {
    static const std::vector<FlagName> v = {
        {0x00000001U, "CONTENTS_SOLID"},       {0x00000002U, "CONTENTS_LAVA"},       {0x00000004U, "CONTENTS_WATER"},
        {0x00000008U, "CONTENTS_FOG"},         {0x00000010U, "CONTENTS_PLAYERCLIP"}, {0x00000020U, "CONTENTS_MONSTERCLIP"},
        {0x00000040U, "CONTENTS_BOTCLIP"},     {0x00000080U, "CONTENTS_SHOTCLIP"},   {0x00000100U, "CONTENTS_BODY"},
        {0x00000200U, "CONTENTS_CORPSE"},      {0x00000400U, "CONTENTS_TRIGGER"},    {0x00000800U, "CONTENTS_NODROP"},
        {0x00001000U, "CONTENTS_TERRAIN"},     {0x00002000U, "CONTENTS_LADDER"},     {0x00004000U, "CONTENTS_ABSEIL"},
        {0x00008000U, "CONTENTS_OPAQUE"},      {0x00010000U, "CONTENTS_OUTSIDE"},    {0x00020000U, "CONTENTS_SLIME"},
        {0x00040000U, "CONTENTS_LIGHTSABER"},  {0x00080000U, "CONTENTS_TELEPORTER"}, {0x00100000U, "CONTENTS_ITEM"},
        {0x08000000U, "CONTENTS_DETAIL"},      {0x10000000U, "CONTENTS_INSIDE"},     {0x80000000U, "CONTENTS_TRANSLUCENT"},
    };
    return v;
}
// MATERIAL_NONE .. MATERIAL_COMPUTER (OJK code/game/surfaceflags.h:104-136, JK2 gleich). MD9 hat an
// Stelle 31 MATERIAL_LAVA und MATERIAL_COMPUTER 32 - das passt nicht mehr in MATERIAL_MASK (0x1f)
// und kann in den surfaceFlags nicht stehen.
const std::vector<FlagName>& materialNamen(FlagSpiel s) {
    static const std::vector<FlagName> jka = {
        {0, "MATERIAL_NONE"},        {1, "MATERIAL_SOLIDWOOD"},    {2, "MATERIAL_HOLLOWWOOD"}, {3, "MATERIAL_SOLIDMETAL"},
        {4, "MATERIAL_HOLLOWMETAL"}, {5, "MATERIAL_SHORTGRASS"},   {6, "MATERIAL_LONGGRASS"},  {7, "MATERIAL_DIRT"},
        {8, "MATERIAL_SAND"},        {9, "MATERIAL_GRAVEL"},       {10, "MATERIAL_GLASS"},     {11, "MATERIAL_CONCRETE"},
        {12, "MATERIAL_MARBLE"},     {13, "MATERIAL_WATER"},       {14, "MATERIAL_SNOW"},      {15, "MATERIAL_ICE"},
        {16, "MATERIAL_FLESH"},      {17, "MATERIAL_MUD"},         {18, "MATERIAL_BPGLASS"},   {19, "MATERIAL_DRYLEAVES"},
        {20, "MATERIAL_GREENLEAVES"}, {21, "MATERIAL_FABRIC"},     {22, "MATERIAL_CANVAS"},    {23, "MATERIAL_ROCK"},
        {24, "MATERIAL_RUBBER"},     {25, "MATERIAL_PLASTIC"},     {26, "MATERIAL_TILES"},     {27, "MATERIAL_CARPET"},
        {28, "MATERIAL_PLASTER"},    {29, "MATERIAL_SHATTERGLASS"}, {30, "MATERIAL_ARMOR"},    {31, "MATERIAL_COMPUTER"},
    };
    static const std::vector<FlagName> md = [] {
        std::vector<FlagName> v = jka;
        v[31] = {31, "MATERIAL_LAVA"};
        return v;
    }();
    return s == FlagSpiel::MovieDuels ? md : jka;
}

}  // namespace bhed::karte
