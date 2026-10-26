// zusammenfuehren.cpp - Karten-Editor Stufe 8a Punkt 5c: "Zur fertigen Karte
// hinzufuegen" (siehe zusammenfuehren.h).
//
//   Copyright (C) 2026 BehavEd-Remake contributors.
//
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. This program is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/zusammenfuehren.h"

#include "bhed/bsp.h"
#include "bhed/bspgeo.h"
#include "bhed/kollision.h"
#include "bhed/karte/bspoeffnen.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>

namespace bhed::karte {

namespace {

constexpr std::uint32_t kMaskePlayerSolid = 0x1111U;   // MASK_PLAYERSOLID (bg_public.h:716)
constexpr std::uint32_t kInhaltFest = 0x1U | 0x10U;    // CONTENTS_SOLID | CONTENTS_PLAYERCLIP

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

std::string anf(const std::string& s) { return "\"" + s + "\""; }

std::string wert(const MapEntity& e, const char* k) {
    for (const auto& [a, b] : e.keys) {
        if (klein(a) == k) { return b; }
    }
    return {};
}

struct Teil {
    KollisionsKarte cm;
    std::vector<MapEntity> ents;
    std::vector<std::string> shader;   // klein
    bool ok = false;
};

Teil teilLesen(const std::string& daten) {
    Teil t;
    BspDatei b;
    if (!bspLesen(daten, b) || b.lumps.size() < 2 || !leseKollision(daten, t.cm)) { return t; }
    parseEntities(bspEntitaetenText(b), t.ents);
    for (std::size_t i = 0; i + 72 <= b.lumps[1].size(); i += 72) { t.shader.push_back(klein(std::string(b.lumps[1].data() + i, strnlen(b.lumps[1].data() + i, 64)))); }
    t.ok = true;
    return t;
}

// 0 nichts, 1 inside, 2 outside (q3map2 surfaceparm inside/outside: system/inside, system/outside)
int wetterArt(const Teil& t, const KollisionsKarte::Pinsel& p) {
    int art = 0;
    for (int s = p.ersteSeite; s < p.ersteSeite + p.seiten; ++s) {
        if (s < 0 || s >= static_cast<int>(t.cm.seiten.size())) { continue; }
        const int sh = t.cm.seiten[static_cast<std::size_t>(s)].shader;
        if (sh < 0 || sh >= static_cast<int>(t.shader.size())) { continue; }
        const std::string& n = t.shader[static_cast<std::size_t>(sh)];
        if (n.size() >= 13 && n.compare(n.size() - 13, 13, "system/inside") == 0) { art |= 1; }
        if (n.size() >= 14 && n.compare(n.size() - 14, 14, "system/outside") == 0) { art |= 2; }
    }
    return art;
}

struct Kasten {
    float mins[3];
    float maxs[3];
};

std::vector<Kasten> zonen(const Teil& t) {
    std::vector<Kasten> z;
    for (const MapEntity& e : t.ents) {
        if (klein(wert(e, "classname")) != "misc_weather_zone") { continue; }
        const std::string m = wert(e, "model");
        if (m.size() < 2 || m[0] != '*') { continue; }
        const int n = std::atoi(m.c_str() + 1);
        if (n <= 0 || n >= static_cast<int>(t.cm.modelle.size())) { continue; }
        const auto& mo = t.cm.modelle[static_cast<std::size_t>(n)];
        Kasten k{};
        for (int i = 0; i < 3; ++i) {
            k.mins[i] = mo.mins[i];
            k.maxs[i] = mo.maxs[i];
        }
        z.push_back(k);
    }
    return z;
}

}  // namespace

std::string mergeBefehl(const Q3map2Aufruf& a, const std::string& hauptBsp, const std::string& teilBsp) {
    return anf(a.programm) + " -v -game " + (a.spielTyp.empty() ? std::string("ja") : a.spielTyp) + " -fs_basepath " + anf(a.enginePfad) +
           " -fs_homepath " + anf(a.benutzerPfad) + " -fs_game " + a.fsGame + " -mergebsp -world -fixnames " + anf(hauptBsp) + " " + anf(teilBsp);
}

std::string mergeZiel(const std::string& hauptBsp) {
    // path_set_extension( source, "_merged.bsp" )
    const std::size_t punkt = hauptBsp.rfind('.');
    const std::size_t trenner = hauptBsp.find_last_of("/\\");
    const std::string stamm = (punkt == std::string::npos || (trenner != std::string::npos && punkt < trenner)) ? hauptBsp : hauptBsp.substr(0, punkt);
    return stamm + "_merged.bsp";
}

std::vector<MergeBefund> mergeVorPruefen(const std::string& hauptBsp, const std::string& teilBsp) {
    std::vector<MergeBefund> out;
    const Teil h = teilLesen(hauptBsp);
    const Teil t = teilLesen(teilBsp);
    if (!h.ok || !t.ok) {
        out.push_back({MergeBefund::Schwere::Fehler, "lesen", "a .bsp could not be read", 1});
        return out;
    }
    // Wetter: inside und outside zusammen laden nicht (JKHub 13136)
    long long hIn = 0;
    long long hOut = 0;
    long long tIn = 0;
    long long tOut = 0;
    for (const auto& p : h.cm.pinsel) {
        const int a = wetterArt(h, p);
        hIn += (a & 1) != 0 ? 1 : 0;
        hOut += (a & 2) != 0 ? 1 : 0;
    }
    std::vector<int> teilWetter;
    for (std::size_t i = 0; i < t.cm.pinsel.size(); ++i) {
        const int a = wetterArt(t, t.cm.pinsel[i]);
        tIn += (a & 1) != 0 ? 1 : 0;
        tOut += (a & 2) != 0 ? 1 : 0;
        if (a != 0) { teilWetter.push_back(static_cast<int>(i)); }
    }
    if (hIn + tIn > 0 && hOut + tOut > 0) {
        char z[256];
        std::snprintf(z, sizeof(z), "inside and outside weather brushes mixed after merging (map %lld/%lld, addition %lld/%lld): the map does not load", hIn, hOut,
                      tIn, tOut);
        out.push_back({MergeBefund::Schwere::Fehler, "wetter", z, hIn + hOut + tIn + tOut});
    }
    // Zonen: gibt es misc_weather_zone, gelten Inside/Outside nur darin
    std::vector<Kasten> z = zonen(h);
    for (const Kasten& k : zonen(t)) { z.push_back(k); }
    if (!z.empty() && !teilWetter.empty()) {
        long long draussen = 0;
        for (const int i : teilWetter) {
            const auto& p = t.cm.pinsel[static_cast<std::size_t>(i)];
            bool drin = false;
            for (const Kasten& k : z) {
                bool d = true;
                for (int a = 0; a < 3; ++a) { d = d && p.mins[a] >= k.mins[a] - 1.0F && p.maxs[a] <= k.maxs[a] + 1.0F; }
                drin = drin || d;
            }
            draussen += drin ? 0 : 1;
        }
        if (draussen > 0) {
            char zt[256];
            std::snprintf(zt, sizeof(zt), "%lld weather brushes of the addition outside every misc_weather_zone (%zu zones): they have no effect", draussen, z.size());
            out.push_back({MergeBefund::Schwere::Warnung, "zone", zt, draussen});
        }
    }
    // Namen: -fixnames benennt gleiche target/targetname des Teils um
    std::set<std::string> hNamen;
    for (const MapEntity& e : h.ents) {
        for (const char* k : {"targetname", "target"}) {
            const std::string v = wert(e, k);
            if (!v.empty()) { hNamen.insert(klein(v)); }
        }
    }
    std::set<std::string> gleich;
    for (const MapEntity& e : t.ents) {
        for (const char* k : {"targetname", "target"}) {
            const std::string v = wert(e, k);
            if (!v.empty() && hNamen.count(klein(v)) != 0) { gleich.insert(v); }
        }
    }
    if (!gleich.empty()) {
        std::string liste;
        for (const std::string& n : gleich) { liste += (liste.empty() ? "" : ", ") + n; }
        out.push_back({MergeBefund::Schwere::Hinweis, "namen",
                       std::to_string(gleich.size()) + " target/targetname of the addition also exist in the map; -fixnames renames them (links between map and addition are lost): " + liste,
                       static_cast<long long>(gleich.size())});
    }
    return out;
}

int weltBrushes(const std::string& bspDaten) {
    KollisionsKarte k;
    if (!leseKollision(bspDaten, k) || k.modelle.empty()) { return 0; }
    return k.modelle[0].zahl;
}

MergeKollision mergeKollisionPruefen(const std::string& mergedBsp, int teilWeltBrushes) {
    MergeKollision r;
    KollisionsKarte k;
    if (!leseKollision(mergedBsp, k) || k.modelle.empty()) { return r; }
    r.geladen = true;
    const auto& welt = k.modelle[0];
    const int erste = welt.erster + welt.zahl - teilWeltBrushes;
    const float mins[3] = {-15, -15, -24};
    const float maxs[3] = {15, 15, 32};
    for (int b = std::max(welt.erster, erste); b < welt.erster + welt.zahl; ++b) {
        const int idx = b;   // dmodel_t.firstBrush zaehlt im Brush-Lump
        if (idx < 0 || idx >= static_cast<int>(k.pinsel.size())) { continue; }
        const auto& p = k.pinsel[static_cast<std::size_t>(idx)];
        if ((p.inhalt & kInhaltFest) == 0) { continue; }
        const float mitte[3] = {(p.mins[0] + p.maxs[0]) * 0.5F, (p.mins[1] + p.maxs[1]) * 0.5F, (p.mins[2] + p.maxs[2]) * 0.5F};
        // von aussen auf die Mitte: oben, dann die vier Seiten, dann unten
        bool geprueft = false;
        bool stoppt = false;
        const float abstand[6][3] = {{0, 0, 1}, {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, -1}};
        for (const auto& richtung : abstand) {
            float start[3];
            for (int a = 0; a < 3; ++a) {
                const float halb = (p.maxs[a] - p.mins[a]) * 0.5F;
                start[a] = mitte[a] + richtung[a] * (halb + 64.0F);
            }
            const KollisionsSpur frei = kollisionsSpur(k, start, mins, maxs, start, kMaskePlayerSolid);
            if (frei.startsolid || frei.allsolid) { continue; }
            geprueft = true;
            const KollisionsSpur s = kollisionsSpur(k, start, mins, maxs, mitte, kMaskePlayerSolid);
            stoppt = s.fraction < 1.0F && !s.startsolid;
            break;
        }
        if (!geprueft) {
            ++r.nichtPruefbar;
            continue;
        }
        ++r.gesamt;
        r.stoppen += stoppt ? 1 : 0;
    }
    return r;
}

MergeLicht mergeLichtAusHauptkarte(const std::string& hauptBsp, const double mins[3], const double maxs[3], double rand) {
    MergeLicht m;
    BspGeometry g;
    if (readBspGeometry(hauptBsp, g) && !g.lightGrid.empty()) {
        double summe[3] = {0, 0, 0};
        for (long long z = 0; z < g.lightGridBounds[2]; ++z) {
            for (long long y = 0; y < g.lightGridBounds[1]; ++y) {
                for (long long x = 0; x < g.lightGridBounds[0]; ++x) {
                    const double p[3] = {g.lightGridOrigin[0] + static_cast<double>(x) * g.lightGridSize[0], g.lightGridOrigin[1] + static_cast<double>(y) * g.lightGridSize[1],
                                         g.lightGridOrigin[2] + static_cast<double>(z) * g.lightGridSize[2]};
                    bool drin = true;
                    for (int a = 0; a < 3; ++a) { drin = drin && p[a] >= mins[a] && p[a] <= maxs[a]; }
                    if (!drin) { continue; }
                    const long long i = x + y * g.lightGridBounds[0] + z * g.lightGridBounds[0] * g.lightGridBounds[1];
                    if (i < 0 || static_cast<std::size_t>(i) >= g.lightArray.size()) { continue; }
                    const std::size_t idx = g.lightArray[static_cast<std::size_t>(i)];
                    if (idx >= g.lightGrid.size() || g.lightGrid[idx].styles[0] == 0xFF) { continue; }
                    // Umgebungslicht plus ein Viertel des gerichteten (light.cpp TraceGrid: ambient == 0.25 * sum)
                    for (int c = 0; c < 3; ++c) { summe[c] += g.lightGrid[idx].ambient[0][c] + 0.25 * g.lightGrid[idx].direct[0][c]; }
                    ++m.gitterPunkte;
                }
            }
        }
        if (m.gitterPunkte > 0) {
            for (double& s : summe) { s /= m.gitterPunkte; }
            const double mx = std::max({summe[0], summe[1], summe[2]});
            m.ambient = mx;
            for (int c = 0; c < 3; ++c) { m.farbe[c] = mx > 0 ? summe[c] / mx : 1.0; }
        }
    }
    // stehen gebliebene Lichter im Kasten + Rand
    BspDatei b;
    std::vector<MapEntity> ents;
    if (bspLesen(hauptBsp, b)) { parseEntities(bspEntitaetenText(b), ents); }
    for (const MapEntity& e : ents) {
        // nur Lichter ohne targetname (-keeplights): ein schaltbares Licht hat in der Hauptkarte seinen
        // Stil; im Teil bekaeme es einen eigenen (32 + j, SetLightStyles) - es wuerde nicht mitschalten
        if (klein(wert(e, "classname")).rfind("light", 0) != 0 || !wert(e, "targetname").empty()) { continue; }
        double p[3];
        if (std::sscanf(wert(e, "origin").c_str(), "%lf %lf %lf", &p[0], &p[1], &p[2]) != 3) { continue; }
        bool nah = true;
        for (int a = 0; a < 3; ++a) { nah = nah && p[a] >= mins[a] - rand && p[a] <= maxs[a] + rand; }
        if (!nah) { continue; }
        std::vector<std::pair<std::string, std::string>> kv;
        for (const auto& [k, v] : e.keys) {
            kv.emplace_back(k, v);
        }
        m.lichter.push_back(std::move(kv));
    }
    return m;
}

}  // namespace bhed::karte
