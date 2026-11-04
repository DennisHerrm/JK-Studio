// bspmerge.cpp - rc609 B1: Brush-Entities in fertige Karten einfuegen, Stufe 1 (siehe bspmerge.h).
//
// Portiert aus q3map2 (NetRadiant-Custom 20260114, third_party/q3map2/tools/quake3/q3map2):
//   convert_bsp.cpp MergeBSPMain   das Umrechnen der Indizes und *N (Abweichungen in bspmerge.h)
//   writebsp.cpp EmitShader        Wiederverwenden eines Shaders (Flags gleich, Name ohne Gross/klein)
//   writebsp.cpp EmitLeaf          das Blatt eines Untermodells (Kasten, leafbrushes, keine Flaechen)
//   bspfile_ibsp.cpp/_rbsp.cpp     die Strukturen (dmodel_t 40, dshader_t 72, dplane_t 16,
//                                  dleaf_t 48, dbrush_t 12, dbrushside_t 8 bzw. 12 Bytes)
// und aus OpenJK / Movie Duels 9 (code/ und codemp/qcommon/cm_load.cpp: CMod_LoadSubmodels,
// CMod_LoadBrushes, CMod_LoadBrushSides; cm_local.h; q_shared.h MAX_SUBMODELS; qfiles.h MAX_MAP_*;
// code/game/surfaceflags.h CONTENTS_SOLID).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2001 - 2013, Activision, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//   Copyright (C) 2026 BehavEd-Remake contributors.
//
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. This program is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/bspmerge.h"

#include "bhed/bsp.h"
#include "bhed/karte/bspoeffnen.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace bhed::karte {

namespace {

// Lump-Nummern (bspfile_ibsp.cpp / bspfile_rbsp.cpp, fuer 0-16 gleich)
constexpr std::size_t kEnt = 0;
constexpr std::size_t kShader = 1;
constexpr std::size_t kEbenen = 2;
constexpr std::size_t kBlaetter = 4;
constexpr std::size_t kBlattFlaechen = 5;
constexpr std::size_t kBlattPinsel = 6;
constexpr std::size_t kModelle = 7;
constexpr std::size_t kPinsel = 8;
constexpr std::size_t kSeiten = 9;
constexpr std::size_t kFlaechen = 13;

constexpr std::uint32_t kContentsSolid = 0x00000001U;   // CONTENTS_SOLID (surfaceflags.h, JKA und JK2)

// qfiles.h (OJK code/ und codemp/, MD9 ebenso): Grenzen des Formats und der Werkzeuge. Die Engine
// prueft sie beim Laden nicht (cm_load.cpp liest jede Zahl) - darum nur Warnungen.
struct FormatGrenze {
    std::size_t lump;
    std::size_t hoechstens;
    const char* name;
};
constexpr std::array<FormatGrenze, 6> kFormatGrenzen{{
    {kShader, 0x400, "MAX_MAP_SHADERS"},
    {kEbenen, 0x20000, "MAX_MAP_PLANES"},
    {kBlaetter, 0x20000, "MAX_MAP_LEAFS"},
    {kBlattPinsel, 0x40000, "MAX_MAP_LEAFBRUSHES"},
    {kPinsel, 0x8000, "MAX_MAP_BRUSHES"},
    {kSeiten, 0x20000, "MAX_MAP_BRUSHSIDES"},
}};
constexpr std::size_t kMaxMapEntstring = 0x40000;   // MAX_MAP_ENTSTRING

std::int32_t i32(const std::string& b, std::size_t at) {
    std::uint32_t v = 0;
    for (std::size_t i = 0; i < 4; ++i) { v |= static_cast<std::uint32_t>(static_cast<unsigned char>(b[at + i])) << (8 * i); }
    return static_cast<std::int32_t>(v);
}
float f32(const std::string& b, std::size_t at) {
    const std::uint32_t v = static_cast<std::uint32_t>(i32(b, at));
    float f = 0.0F;
    std::memcpy(&f, &v, sizeof f);
    return f;
}
void dazu(std::string& b, std::int32_t v) {
    const auto u = static_cast<std::uint32_t>(v);
    for (std::size_t i = 0; i < 4; ++i) { b += static_cast<char>((u >> (8 * i)) & 0xFFU); }
}
std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}
// dshader_t.shader: char[MAX_QPATH], bis zur NUL
std::string shaderName(const std::string& lump, std::size_t i) {
    const std::string roh = lump.substr(i * 72, 64);
    return roh.substr(0, roh.find('\0'));
}

struct Teilpinsel {
    std::vector<Halbraum> h;
    float mins[3]{};
    float maxs[3]{};
    std::uint32_t inhalt = 0;
    int modell = 0;   // *k im Teil
};

// Die Halbraeume eines Brushes (Seiten -> Ebenen), verschoben um den Ursprung der Entity, und sein
// Kasten wie CM_BoundBrush (cm_load.cpp: die ersten sechs Seiten sind die achsparallelen).
bool pinselLesen(const BspDatei& d, std::size_t seitenGroesse, int nr, const double ursprung[3], Teilpinsel& p) {
    const std::string& br = d.lumps[kPinsel];
    const std::string& sd = d.lumps[kSeiten];
    const std::string& pl = d.lumps[kEbenen];
    const int erste = i32(br, static_cast<std::size_t>(nr) * 12);
    const int zahl = i32(br, static_cast<std::size_t>(nr) * 12 + 4);
    if (erste < 0 || zahl < 6 || static_cast<std::size_t>(erste) + static_cast<std::size_t>(zahl) > sd.size() / seitenGroesse) { return false; }
    p.h.clear();
    for (int s = 0; s < zahl; ++s) {
        const int e = i32(sd, static_cast<std::size_t>(erste + s) * seitenGroesse);
        if (e < 0 || static_cast<std::size_t>(e) >= pl.size() / 16) { return false; }
        Halbraum h;
        double nd = 0.0;
        for (int k = 0; k < 3; ++k) {
            h.n[k] = f32(pl, static_cast<std::size_t>(e) * 16 + static_cast<std::size_t>(k) * 4);
            nd += h.n[k] * ursprung[k];
        }
        h.d = f32(pl, static_cast<std::size_t>(e) * 16 + 12) + nd;
        p.h.push_back(h);
    }
    for (int k = 0; k < 3; ++k) {
        p.mins[k] = static_cast<float>(-p.h[static_cast<std::size_t>(2 * k)].d);
        p.maxs[k] = static_cast<float>(p.h[static_cast<std::size_t>(2 * k + 1)].d);
    }
    return true;
}

bool kaestenBeruehren(const Teilpinsel& a, const Teilpinsel& b) {
    for (int k = 0; k < 3; ++k) {
        if (a.mins[k] >= b.maxs[k] || b.mins[k] >= a.maxs[k]) { return false; }
    }
    return true;
}

}  // namespace

bool MergeErgebnis::hatFehler() const {
    return std::any_of(meldungen.begin(), meldungen.end(), [](const MergeMeldung& m) { return m.fehler; });
}

std::vector<ModellGrenze> modellGrenzen(MergeSpiel s) {
    switch (s) {
    case MergeSpiel::Jk2:
        // OpenJK baut JK2 aus code/ (JK2_MODE); fuer JK2 MP gibt es dort keine Quelle.
        return {{"JK2 SP", 511, "OpenJK code/qcommon/cm_load.cpp CMod_LoadSubmodels, CM_ClipHandleToModel, cm_local.h BOX_MODEL_HANDLE"},
                {"JK2 MP", 510, "assumed like JKA MP (no JK2 MP source in OpenJK)"}};
    case MergeSpiel::MovieDuels:
        return {{"Movie Duels SP", 511, "MD9 code/qcommon/cm_load.cpp CMod_LoadSubmodels, CM_ClipHandleToModel, cm_local.h BOX_MODEL_HANDLE"},
                {"Movie Duels MP", 510, "MD9 codemp/qcommon/cm_local.h CAPSULE_MODEL_HANDLE (MAX_SUBMODELS-2)"}};
    case MergeSpiel::Jka:
    default:
        return {{"JKA SP", 511, "OpenJK code/qcommon/cm_load.cpp CMod_LoadSubmodels, CM_ClipHandleToModel, cm_local.h BOX_MODEL_HANDLE"},
                {"JKA MP", 510, "OpenJK codemp/qcommon/cm_local.h CAPSULE_MODEL_HANDLE (MAX_SUBMODELS-2)"}};
    }
}

bool pinselUeberschneiden(const std::vector<Halbraum>& a, const std::vector<Halbraum>& b, double epsilon) {
    std::vector<Halbraum> h;
    h.reserve(a.size() + b.size());
    for (const auto* v : {&a, &b}) {
        for (Halbraum x : *v) {
            x.d -= epsilon;
            h.push_back(x);
        }
    }
    const auto kreuz = [](const double* p, const double* q, double* r) {
        r[0] = p[1] * q[2] - p[2] * q[1];
        r[1] = p[2] * q[0] - p[0] * q[2];
        r[2] = p[0] * q[1] - p[1] * q[0];
    };
    const std::size_t n = h.size();
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            double jk[3];
            for (std::size_t k = j + 1; k < n; ++k) {
                kreuz(h[j].n, h[k].n, jk);
                const double det = h[i].n[0] * jk[0] + h[i].n[1] * jk[1] + h[i].n[2] * jk[2];
                if (std::fabs(det) < 1e-9) { continue; }
                double ki[3];
                double ij[3];
                kreuz(h[k].n, h[i].n, ki);
                kreuz(h[i].n, h[j].n, ij);
                double x[3];
                for (int c = 0; c < 3; ++c) { x[c] = (h[i].d * jk[c] + h[j].d * ki[c] + h[k].d * ij[c]) / det; }
                bool drin = true;
                for (std::size_t l = 0; drin && l < n; ++l) {
                    drin = h[l].n[0] * x[0] + h[l].n[1] * x[1] + h[l].n[2] * x[2] <= h[l].d + 1e-4;
                }
                if (drin) { return true; }
            }
        }
    }
    return false;
}

MergeErgebnis bspModelleAnhaengen(const std::string& original, const std::string& teil, MergeSpiel spiel) {
    MergeErgebnis r;
    const auto melde = [&r](bool fehler, const char* pruefung, const std::string& text) { r.meldungen.push_back({fehler, pruefung, text}); };
    BspDatei a;
    BspDatei b;
    if (!bspLesen(original, a)) {
        melde(true, "format", "The compiled map is not an IBSP/RBSP file");
        return r;
    }
    if (!bspLesen(teil, b)) {
        melde(true, "format", "The part compile is not an IBSP/RBSP file");
        return r;
    }
    if (a.ident != b.ident || a.version != b.version) {
        melde(true, "format",
              "Different BSP formats: " + a.ident + " " + std::to_string(a.version) + " and " + b.ident + " " + std::to_string(b.version));
        return r;
    }
    const bool rbsp = a.ident == "RBSP";
    const std::size_t seitenGroesse = rbsp ? 12 : 8;            // dbrushside_t (RBSP: + drawSurfNum)
    const std::size_t flaechenGroesse = rbsp ? 148 : 104;       // dsurface_t
    // "funny lump size" (cm_load.cpp CMod_Load*): jede Laenge ein Vielfaches der Struktur
    const std::array<std::pair<std::size_t, std::size_t>, 9> groessen{
        {{kShader, 72}, {kEbenen, 16}, {kBlaetter, 48}, {kBlattFlaechen, 4}, {kBlattPinsel, 4}, {kModelle, 40}, {kPinsel, 12}, {kSeiten, seitenGroesse}, {kFlaechen, flaechenGroesse}}};
    for (const BspDatei* d : {&a, &b}) {
        for (const auto& [nr, gr] : groessen) {
            if (d->lumps[nr].size() % gr != 0) {
                melde(true, "format", std::string(d == &a ? "Compiled map" : "Part compile") + ": funny lump size in lump " + std::to_string(nr));
                return r;
            }
        }
    }
    const auto zahl = [](const BspDatei& d, std::size_t nr, std::size_t gr) { return d.lumps[nr].size() / gr; };
    const std::size_t modelleA = zahl(a, kModelle, 40);
    const std::size_t modelleT = zahl(b, kModelle, 40);
    if (modelleA < 1 || modelleT < 1) {
        melde(true, "format", "Map with no models");   // CMod_LoadSubmodels
        return r;
    }
    // Die Welt des Teils (Modell 0) ist nur der Platzhalter, ohne den q3map2 nichts baut (ProcessModels,
    // bsp.cpp: eine Entity ohne Brushes wird uebersprungen - auch die worldspawn, dann verrutschen die *N);
    // wie MergeBSPMain ("minus world") wird sie nicht uebernommen.
    const std::string& mdT = b.lumps[kModelle];
    if (modelleT < 2) { melde(true, "format", "The part compile has no brush entities"); }
    for (std::size_t m = 1; m < modelleT; ++m) {
        if (i32(mdT, m * 40 + 28) != 0) {
            melde(true, "sichtbar", "Model *" + std::to_string(m) + " of the part has " + std::to_string(i32(mdT, m * 40 + 28)) +
                                        " visible surfaces - visible brush entities need lightmaps (stage 3); use nodraw or trigger shaders");
        }
    }
    const std::size_t gesamt = modelleA + modelleT - 1;
    for (const ModellGrenze& g : modellGrenzen(spiel)) {
        if (gesamt > static_cast<std::size_t>(g.hoechstens)) {
            melde(true, "grenze", std::string(g.name) + ": " + std::to_string(gesamt) + " models, at most " + std::to_string(g.hoechstens) + " (" +
                                      g.quelle + ")");
        }
    }
    if (r.hatFehler()) { return r; }

    // --- Shader (EmitShader: gleiche Flags und Name ohne Gross/klein -> wiederverwenden) ----------
    std::string shOut = a.lumps[kShader];
    std::vector<int> shaderNeu(zahl(b, kShader, 72), -1);
    bool kaputt = false;
    const auto shaderFuer = [&](int t) {
        if (t < 0 || static_cast<std::size_t>(t) >= shaderNeu.size()) {
            kaputt = true;
            return 0;
        }
        int& z = shaderNeu[static_cast<std::size_t>(t)];
        if (z >= 0) { return z; }
        const std::string& sb = b.lumps[kShader];
        const std::string name = klein(shaderName(sb, static_cast<std::size_t>(t)));
        const std::int32_t sf = i32(sb, static_cast<std::size_t>(t) * 72 + 64);
        const std::int32_t cf = i32(sb, static_cast<std::size_t>(t) * 72 + 68);
        for (std::size_t i = 0; i < shOut.size() / 72; ++i) {
            if (i32(shOut, i * 72 + 64) == sf && i32(shOut, i * 72 + 68) == cf && klein(shaderName(shOut, i)) == name) {
                z = static_cast<int>(i);
                return z;
            }
        }
        z = static_cast<int>(shOut.size() / 72);
        shOut += sb.substr(static_cast<std::size_t>(t) * 72, 72);
        return z;
    };
    // --- Ebenen: das Paar (n & ~1, n | 1); Byte-gleiches Paar an gerader Stelle wiederverwenden -----
    std::string plOut = a.lumps[kEbenen];
    std::map<std::string, int> paareA;
    for (std::size_t i = 0; i + 1 < plOut.size() / 16; i += 2) { paareA.emplace(plOut.substr(i * 16, 32), static_cast<int>(i)); }
    std::map<int, int> paarNeu;
    const auto ebeneFuer = [&](int t) {
        const std::size_t zahlT = b.lumps[kEbenen].size() / 16;
        if (t < 0 || static_cast<std::size_t>(t) >= zahlT) {
            kaputt = true;
            return 0;
        }
        const int paar = t & ~1;
        auto it = paarNeu.find(paar);
        if (it == paarNeu.end()) {
            const std::size_t laenge = static_cast<std::size_t>(paar) + 1 < zahlT ? 32 : 16;
            const std::string bytes = b.lumps[kEbenen].substr(static_cast<std::size_t>(paar) * 16, laenge);
            const auto vorhanden = laenge == 32 ? paareA.find(bytes) : paareA.end();
            int basis = 0;
            if (vorhanden != paareA.end()) {
                basis = vorhanden->second;
            } else {
                basis = static_cast<int>(plOut.size() / 16);
                plOut += bytes;
            }
            it = paarNeu.emplace(paar, basis).first;
        }
        return it->second + (t & 1);
    };

    // --- Entities des Teils (q3map2 hat "model" "*k" und bei origin-Brushes "origin" gesetzt) -----
    std::vector<MapEntity> entT;
    parseEntities(bspEntitaetenText(b), entT);
    if (entT.empty() || entT[0].classname != "worldspawn") {
        melde(true, "format", "The part compile has no worldspawn");
        return r;
    }
    std::vector<std::array<double, 3>> ursprung(modelleT, std::array<double, 3>{0.0, 0.0, 0.0});
    for (const MapEntity& e : entT) {
        const std::string* m = e.find("model");
        if (m == nullptr || m->empty() || (*m)[0] != '*') { continue; }
        const int k = std::atoi(m->c_str() + 1);
        const std::string* o = e.find("origin");
        if (k > 0 && static_cast<std::size_t>(k) < modelleT && o != nullptr) {
            std::array<double, 3>& u = ursprung[static_cast<std::size_t>(k)];
            if (std::sscanf(o->c_str(), "%lf %lf %lf", &u[0], &u[1], &u[2]) != 3) { u = {0.0, 0.0, 0.0}; }   // vectorForKey
        }
    }

    // --- Modelle, Brushes, Seiten, Blaetter --------------------------------------------------------
    std::string brOut = a.lumps[kPinsel];
    std::string sdOut = a.lumps[kSeiten];
    std::string mdOut = a.lumps[kModelle];
    std::string lfOut = a.lumps[kBlaetter];
    std::string lbOut = a.lumps[kBlattPinsel];
    const auto flaechenA = static_cast<std::int32_t>(zahl(a, kFlaechen, flaechenGroesse));
    const auto blattFlaechenA = static_cast<std::int32_t>(zahl(a, kBlattFlaechen, 4));
    const std::string& brT = b.lumps[kPinsel];
    const std::string& sdT = b.lumps[kSeiten];
    const std::string& lfT = b.lumps[kBlaetter];
    const std::string& lbT = b.lumps[kBlattPinsel];
    std::vector<Teilpinsel> neuePinsel;
    for (std::size_t m = 1; m < modelleT; ++m) {
        const int ersterT = i32(mdT, m * 40 + 32);
        const int zahlT = i32(mdT, m * 40 + 36);
        if (ersterT < 0 || zahlT < 0 || static_cast<std::size_t>(ersterT) + static_cast<std::size_t>(zahlT) > brT.size() / 12) {
            melde(true, "format", "Model *" + std::to_string(m) + " of the part: brushes out of range");
            return r;
        }
        const auto ersterNeu = static_cast<std::int32_t>(brOut.size() / 12);
        for (int i = 0; i < zahlT; ++i) {
            const std::size_t at = static_cast<std::size_t>(ersterT + i) * 12;
            const int ersteSeite = i32(brT, at);
            const int seiten = i32(brT, at + 4);
            if (ersteSeite < 0 || seiten < 0 || static_cast<std::size_t>(ersteSeite) + static_cast<std::size_t>(seiten) > sdT.size() / seitenGroesse) {
                melde(true, "format", "Model *" + std::to_string(m) + " of the part: brush sides out of range");
                return r;
            }
            const auto seiteNeu = static_cast<std::int32_t>(sdOut.size() / seitenGroesse);
            for (int s = 0; s < seiten; ++s) {
                const std::size_t q = static_cast<std::size_t>(ersteSeite + s) * seitenGroesse;
                dazu(sdOut, ebeneFuer(i32(sdT, q)));
                dazu(sdOut, shaderFuer(i32(sdT, q + 4)));
                if (rbsp) {
                    // dbrushside_t.drawSurfNum: ohne Flaeche bleibt der Wert, den EmitBrushes (writebsp.cpp, emplace_back)
                    // hinterlaesst; das Spiel liest ihn nicht (CMod_LoadBrushSides: planeNum, shaderNum). Modelle mit
                    // Flaechen kommen hier nicht an (Fehler "sichtbar" oben).
                    dazu(sdOut, i32(sdT, q + 8));
                }
            }
            dazu(brOut, seiteNeu);
            dazu(brOut, seiten);
            dazu(brOut, shaderFuer(i32(brT, at + 8)));
            Teilpinsel p;
            p.modell = static_cast<int>(m);
            const int shT = i32(brT, at + 8);
            p.inhalt = shT >= 0 && static_cast<std::size_t>(shT) < shaderNeu.size()
                           ? static_cast<std::uint32_t>(i32(b.lumps[kShader], static_cast<std::size_t>(shT) * 72 + 68))
                           : 0U;
            if (pinselLesen(b, seitenGroesse, ersterT + i, ursprung[m].data(), p)) { neuePinsel.push_back(std::move(p)); }
        }
        if (kaputt) {
            melde(true, "format", "Model *" + std::to_string(m) + " of the part: plane or shader out of range");
            return r;
        }
        // dmodel_t: mins, maxs (wie gebaut), firstSurface (die Zahl der Flaechen, keine eigenen), brushes
        mdOut += mdT.substr(m * 40, 24);
        dazu(mdOut, flaechenA);
        dazu(mdOut, 0);
        dazu(mdOut, ersterNeu);
        dazu(mdOut, zahlT);
        // Das Blatt des Modells (EmitLeaf): im Teil das Blatt mit genau diesen leafbrushes
        std::multiset<int> soll;
        for (int i = 0; i < zahlT; ++i) { soll.insert(ersterT + i); }
        std::size_t blatt = lfT.size() / 48;
        for (std::size_t l = 0; l < lfT.size() / 48 && blatt == lfT.size() / 48; ++l) {
            const int erste = i32(lfT, l * 48 + 40);
            const int anzahl = i32(lfT, l * 48 + 44);
            if (anzahl != zahlT || erste < 0 || static_cast<std::size_t>(erste) + static_cast<std::size_t>(anzahl) > lbT.size() / 4) { continue; }
            std::multiset<int> ist;
            for (int i = 0; i < anzahl; ++i) { ist.insert(i32(lbT, static_cast<std::size_t>(erste + i) * 4)); }
            if (ist == soll) { blatt = l; }
        }
        if (blatt == lfT.size() / 48) {
            melde(true, "format", "Model *" + std::to_string(m) + " of the part has no leaf (q3map2 EmitLeaf)");
            return r;
        }
        lfOut += lfT.substr(blatt * 48, 32);   // cluster, area, mins, maxs wie gebaut
        dazu(lfOut, blattFlaechenA);           // firstLeafSurface: bspLeafSurfaces.size(), keine Flaechen
        dazu(lfOut, 0);
        dazu(lfOut, static_cast<std::int32_t>(lbOut.size() / 4));
        dazu(lfOut, zahlT);
        const int erste = i32(lfT, blatt * 48 + 40);
        for (int i = 0; i < zahlT; ++i) { dazu(lbOut, ersterNeu + (i32(lbT, static_cast<std::size_t>(erste + i) * 4) - ersterT)); }
    }

    // --- Entity-Lump: die Entities des Teils (ohne worldspawn) mit *(k + Modelle - 1) ---------------
    std::string entNeu;
    std::vector<MapEntity> entA;
    const std::string textA = bspEntitaetenText(a);
    parseEntities(textA, entA);
    std::set<std::string> namen;
    for (const auto* liste : {&entA, &entT}) {
        for (const MapEntity& e : *liste) {
            if (!e.targetname.empty()) { namen.insert(klein(e.targetname)); }   // G_Find: Q_stricmp
        }
    }
    for (std::size_t i = 1; i < entT.size(); ++i) {
        MapEntity e = entT[i];
        entNeu += "{\n";
        for (auto& [k, v] : e.keys) {
            if (klein(k) == "model" && !v.empty() && v[0] == '*') {
                v = "*" + std::to_string(std::atoi(v.c_str() + 1) + static_cast<int>(modelleA) - 1);   // MergeBSPMain
            }
            entNeu += "\"" + k + "\" \"" + v + "\"\n";
            if ((klein(k) == "target" || klein(k) == "target2") && namen.count(klein(v)) == 0U) {
                melde(false, "ziel", "New " + e.classname + ": " + k + " \"" + v + "\" matches no targetname in the map");
            }
        }
        entNeu += "}\n";
        r.klassen.push_back(e.classname);
    }
    const std::string entOut = textA + entNeu + '\0';

    // --- Ueberschneidung mit soliden Welt-Brushes --------------------------------------------------
    {
        const std::string& mdA = a.lumps[kModelle];
        const int ersterW = i32(mdA, 32);
        const int zahlW = i32(mdA, 36);
        const double null[3] = {0.0, 0.0, 0.0};
        std::vector<Teilpinsel> welt;
        for (int i = 0; i < zahlW && ersterW >= 0; ++i) {
            const int nr = ersterW + i;
            if (static_cast<std::size_t>(nr) >= a.lumps[kPinsel].size() / 12) { break; }
            const int sh = i32(a.lumps[kPinsel], static_cast<std::size_t>(nr) * 12 + 8);
            if (sh < 0 || static_cast<std::size_t>(sh) >= a.lumps[kShader].size() / 72) { continue; }
            if ((static_cast<std::uint32_t>(i32(a.lumps[kShader], static_cast<std::size_t>(sh) * 72 + 68)) & kContentsSolid) == 0U) { continue; }
            Teilpinsel w;
            if (pinselLesen(a, seitenGroesse, nr, null, w)) { welt.push_back(std::move(w)); }
        }
        std::map<int, int> treffer;   // Modell -> Brushes mit Ueberschneidung
        for (const Teilpinsel& p : neuePinsel) {
            if ((p.inhalt & kContentsSolid) == 0U) { continue; }   // Trigger duerfen in die Wand reichen
            for (const Teilpinsel& w : welt) {
                if (kaestenBeruehren(p, w) && pinselUeberschneiden(p.h, w.h)) {
                    ++treffer[p.modell];
                    break;
                }
            }
        }
        for (const auto& [m, n] : treffer) {
            melde(false, "ueberschneidung", "New *" + std::to_string(m + static_cast<int>(modelleA) - 1) + ": " + std::to_string(n) +
                                                " solid brush(es) overlap solid world brushes");
        }
    }

    // --- Grenzen des Formats (Warnungen) ------------------------------------------------------------
    const std::map<std::size_t, const std::string*> neu{{kShader, &shOut}, {kEbenen, &plOut}, {kBlaetter, &lfOut},
                                                         {kBlattPinsel, &lbOut}, {kPinsel, &brOut}, {kSeiten, &sdOut}};
    const std::map<std::size_t, std::size_t> strukt{{kShader, 72}, {kEbenen, 16}, {kBlaetter, 48}, {kBlattPinsel, 4}, {kPinsel, 12}, {kSeiten, seitenGroesse}};
    for (const FormatGrenze& g : kFormatGrenzen) {
        const std::size_t n = neu.at(g.lump)->size() / strukt.at(g.lump);
        if (n > g.hoechstens) {
            melde(false, "grenze", std::string(g.name) + " exceeded: " + std::to_string(n) + " > " + std::to_string(g.hoechstens) + " (qfiles.h)");
        }
    }
    if (entOut.size() > kMaxMapEntstring) {
        melde(false, "grenze", "MAX_MAP_ENTSTRING exceeded: " + std::to_string(entOut.size()) + " > " + std::to_string(kMaxMapEntstring) + " (qfiles.h)");
    }

    r.bsp = bspMitLumps(original, {{kEnt, entOut},
                                   {kShader, shOut},
                                   {kEbenen, plOut},
                                   {kBlaetter, lfOut},
                                   {kBlattPinsel, lbOut},
                                   {kModelle, mdOut},
                                   {kPinsel, brOut},
                                   {kSeiten, sdOut}});
    r.ersteNummer = static_cast<int>(modelleA);
    r.neueModelle = static_cast<int>(modelleT - 1);
    return r;
}

// --- Karten-Editor ------------------------------------------------------------------------------

namespace {
// FNV-1a ueber die Zeilen einer Entity in der .map (ohne Kommentare wie "// entity N"); mitSchluesseln:
// auch die Zeilen "key" "value" (sonst nur die Primitive, wie geometrieFingerabdruck).
std::uint64_t abdruck(const Karte& k, const Entitaet& e, bool mitSchluesseln) {
    std::uint64_t h = 1469598103934665603ULL;
    const std::string text = karteSchreiben(k, [&e](const Entitaet& x) { return &x == &e; }, [](const Entitaet&, const Primitiv&) { return true; });
    std::size_t a = 0;
    while (a < text.size()) {
        std::size_t b = text.find('\n', a);
        if (b == std::string::npos) { b = text.size(); }
        if (b > a && (mitSchluesseln || text[a] != '"') && text.compare(a, 2, "//") != 0) {
            for (std::size_t i = a; i < b; ++i) {
                h ^= static_cast<unsigned char>(text[i]);
                h *= 1099511628211ULL;
            }
            h ^= 0xFFU;
            h *= 1099511628211ULL;
        }
        a = b + 1;
    }
    return h;
}
bool zaehlt(const Karte& k, const Entitaet& e) {
    return !e.kinder().empty() && &e != k.welt().get() && !eingebackeneEntitaet(e.klassenname());
}
bool punkt(const Karte& k, const Entitaet& e) {
    return e.kinder().empty() && &e != k.welt().get() && !eingebackeneEntitaet(e.klassenname());
}
}  // namespace

std::uint64_t pinselAbdruck(const Karte& k, const Entitaet& e) { return abdruck(k, e, false); }
std::uint64_t entitaetAbdruck(const Karte& k, const Entitaet& e) { return abdruck(k, e, true); }

std::vector<std::uint64_t> punktAbdruecke(const Karte& k) {
    std::vector<std::uint64_t> v;
    for (const auto& e : k.entitaeten()) {
        if (punkt(k, *e)) { v.push_back(entitaetAbdruck(k, *e)); }
    }
    return v;
}

std::vector<const Entitaet*> neuePunktEntitaeten(const Karte& k, const std::vector<std::uint64_t>& vorher, int* geaendert) {
    std::multiset<std::uint64_t> frei(vorher.begin(), vorher.end());
    std::vector<const Entitaet*> neu;
    for (const auto& e : k.entitaeten()) {
        if (!punkt(k, *e)) { continue; }
        const auto it = frei.find(entitaetAbdruck(k, *e));
        if (it != frei.end()) {
            frei.erase(it);
        } else {
            neu.push_back(e.get());
        }
    }
    if (geaendert != nullptr) { *geaendert = static_cast<int>(frei.size()); }
    return neu;
}

std::vector<std::uint64_t> pinselAbdruecke(const Karte& k) {
    std::vector<std::uint64_t> v;
    for (const auto& e : k.entitaeten()) {
        if (zaehlt(k, *e)) { v.push_back(pinselAbdruck(k, *e)); }
    }
    return v;
}

std::vector<const Entitaet*> neuePinselEntitaeten(const Karte& k, const std::vector<std::uint64_t>& vorher) {
    std::multiset<std::uint64_t> frei(vorher.begin(), vorher.end());
    std::vector<const Entitaet*> neu;
    for (const auto& e : k.entitaeten()) {
        if (!zaehlt(k, *e)) { continue; }
        const auto it = frei.find(pinselAbdruck(k, *e));
        if (it != frei.end()) {
            frei.erase(it);
        } else {
            neu.push_back(e.get());
        }
    }
    return neu;
}

std::string teilKarte(const Karte& k, const std::vector<const Entitaet*>& neu) {
    const Entitaet* welt = k.welt().get();
    std::string text = karteSchreiben(
        k, [&](const Entitaet& e) { return &e == welt || std::find(neu.begin(), neu.end(), &e) != neu.end(); },
        [welt](const Entitaet& e, const Primitiv&) { return &e != welt; });
    // Der Platzhalter: eine Kopie des ersten Brushes der neuen Entities vor das Ende der worldspawn (sie ist die
    // erste Entity und hat hier keine Primitive). Eine Kopie, weil q3map2 das Pinselformat mit dem ersten Brush der
    // Datei festlegt (ParseMapEntity, map.cpp: g_brushType) - Quake, Valve 220 und brushDef duerfen nicht gemischt
    // werden. Die Welt des Teils uebernimmt bspModelleAnhaengen nicht.
    const std::size_t ende = text.find("\n}\n");
    const std::size_t marke = ende == std::string::npos ? ende : text.find("\n// brush ", ende);
    const std::size_t anfang = marke == std::string::npos ? marke : text.find('\n', marke + 1);
    if (anfang == std::string::npos) { return text; }
    std::size_t pos = anfang + 1;
    int tiefe = 0;
    while (pos < text.size()) {
        std::size_t z = text.find('\n', pos);
        if (z == std::string::npos) { z = text.size(); }
        const std::string zeile = text.substr(pos, z - pos);
        tiefe += zeile == "{" ? 1 : (zeile == "}" ? -1 : 0);
        pos = z + 1;
        if (tiefe == 0) { break; }
    }
    // Mit textures/system/caulk statt der Shader der Kopie: ohne strukturellen, soliden Brush hat die Welt keinen
    // Baum (q3map2 bricht mit "Backwards tree volume" ab). Der Shader steht in jeder Seitenzeile als erstes Wort nach
    // der letzten ")" (Quake und Valve 220: nach dem dritten Punkt, brushDef: nach der Texturmatrix).
    std::string kopie;
    std::size_t a = anfang + 1;
    while (a < pos) {
        std::size_t z = text.find('\n', a);
        if (z == std::string::npos || z > pos) { z = pos; }
        std::string zeile = text.substr(a, z - a);
        const std::size_t klammer = zeile.rfind(')');
        if (!zeile.empty() && zeile[0] == '(' && klammer != std::string::npos) {
            const std::size_t w0 = zeile.find_first_not_of(' ', klammer + 1);
            const std::size_t w1 = w0 == std::string::npos ? w0 : zeile.find(' ', w0);
            if (w0 != std::string::npos) { zeile.replace(w0, (w1 == std::string::npos ? zeile.size() : w1) - w0, "system/caulk"); }
        }
        kopie += zeile + "\n";
        a = z + 1;
    }
    text.insert(ende + 1, kopie);
    return text;
}

}  // namespace bhed::karte
