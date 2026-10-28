// sprungpruefung.cpp - Karten-Editor, Stufe 9 Punkt 4: Sprungweite und Platz pruefen
// (siehe sprungpruefung.h). Portiert aus OpenJK: die Werte und die Bewegung (navSpielerPmove).
//
//   Copyright (C) 1999-2005 Id Software, Inc.; 2000-2013 Raven Software; 2013-2026 OpenJK contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
#include "bhed/karte/sprungpruefung.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <utility>

namespace bhed::karte {

namespace {

constexpr float kMinWalkNormal = 0.7F;   // MIN_WALK_NORMAL
constexpr float kStufe = 18.0F;          // STEPSIZE
const NavVec kMins{-16.0F, -16.0F, -24.0F};
const NavVec kMaxsStehen{16.0F, 16.0F, 40.0F};
const NavVec kMaxsDucken{16.0F, 16.0F, 16.0F};

NavSpur spurAus(const NavSpurFn& f, const NavVec& a, const NavVec& mi, const NavVec& ma, const NavVec& b) {
    return f(a, mi, ma, b, kNavKeineEntity, kNavMaskPlayerSolid);
}

float waagerecht(const NavVec& a, const NavVec& b) { return std::hypot(b[0] - a[0], b[1] - a[1]); }

}  // namespace

bool sprungSimulieren(const NavSpurFn& spur, const NavVec& start, float gier, int machtStufe, bool jk2, int bildMs, NavVec& landung,
                      std::vector<NavVec>* bahn, float* scheitel) {
    NavSpieler s;
    s.ort = start;
    s.gier = gier;
    s.jk2 = jk2;
    s.machtStufe = machtStufe;
    s.amBoden = true;
    const float rad = gier * 3.14159265F / 180.0F;
    s.tempo = {std::cos(rad) * 250.0F, std::sin(rad) * 250.0F, 0.0F};   // Anlauf mit g_speed
    NavSpielerEingabe e;
    e.vor = 127;
    e.hoch = 127;
    bool inLuft = false;
    float oben = start[2];
    if (bahn != nullptr) { bahn->push_back(start); }
    for (int t = 0, bild = 0; t < 4000; t += bildMs, ++bild) {
        navSpielerPmove(s, spur, e, bildMs);
        oben = std::max(oben, s.ort[2]);
        if (bahn != nullptr && bild % 3 == 2) { bahn->push_back(s.ort); }
        if (!s.amBoden) { inLuft = true; }
        if (inLuft && s.amBoden) {
            landung = s.ort;
            if (bahn != nullptr) { bahn->push_back(s.ort); }
            if (scheitel != nullptr) { *scheitel = oben; }
            return true;
        }
        if (s.ort[2] < start[2] - 4096.0F) { break; }
    }
    if (scheitel != nullptr) { *scheitel = oben; }
    return false;
}

SprungBericht sprungPruefen(const NavSpurFn& spur, const SprungOptionen& o, const std::atomic<bool>* abbrechen, std::atomic<int>* fortschritt) {
    SprungBericht r;
    const float rs = std::max(8.0F, o.raster);
    const int nx = std::max(1, static_cast<int>((o.bereichMax[0] - o.bereichMin[0]) / rs) + 1);
    const int ny = std::max(1, static_cast<int>((o.bereichMax[1] - o.bereichMin[1]) / rs) + 1);
    std::map<std::pair<int, int>, std::vector<std::size_t>> spalten;
    // --- Platz: der geduckte Kasten faellt; Boden mit Normale z >= 0.7 ---
    for (int iy = 0; iy < ny; ++iy) {
        for (int ix = 0; ix < nx; ++ix) {
            if (abbrechen != nullptr && abbrechen->load()) { return r; }
            const float x = o.bereichMin[0] + static_cast<float>(ix) * rs;
            const float y = o.bereichMin[1] + static_cast<float>(iy) * rs;
            float z = o.bereichMax[2];
            int boeden = 0;
            while (z > o.bereichMin[2] && boeden < 4) {
                const NavVec p{x, y, z};
                if (spurAus(spur, p, kMins, kMaxsDucken, p).startsolid) {
                    z -= 16.0F;
                    continue;
                }
                const NavSpur tr = spurAus(spur, p, kMins, kMaxsDucken, NavVec{x, y, o.bereichMin[2]});
                if (tr.fraction >= 1.0F) { break; }
                if (tr.normal[2] >= kMinWalkNormal) {
                    StandPunkt sp;
                    sp.ort = tr.endpos;
                    sp.stehen = !spurAus(spur, tr.endpos, kMins, kMaxsStehen, tr.endpos).startsolid;
                    spalten[{ix, iy}].push_back(r.punkte.size());
                    r.punkte.push_back(sp);
                    ++boeden;
                }
                z = tr.endpos[2] - 48.0F;   // unter den Boden: weiter suchen
            }
        }
        if (fortschritt != nullptr) { fortschritt->store(200 * (iy + 1) / ny); }
    }
    // --- Kanten: ein Nachbar ohne Boden in Stufenhoehe ---
    std::vector<std::size_t> kanten;
    for (const auto& [k, liste] : spalten) {
        for (const std::size_t i : liste) {
            const StandPunkt& sp = r.punkte[i];
            if (!sp.stehen) { continue; }
            bool kante = false;
            for (int dy = -1; dy <= 1 && !kante; ++dy) {
                for (int dx = -1; dx <= 1 && !kante; ++dx) {
                    if (dx == 0 && dy == 0) { continue; }
                    const auto it = spalten.find({k.first + dx, k.second + dy});
                    bool da = false;
                    if (it != spalten.end()) {
                        for (const std::size_t j : it->second) { da = da || std::fabs(r.punkte[j].ort[2] - sp.ort[2]) <= kStufe; }
                    }
                    kante = !da;
                }
            }
            if (kante) {
                r.punkte[i].kante = true;
                kanten.push_back(i);
            }
        }
    }
    r.kanten = kanten.size();
    // gleichmaessig hoechstens maxKanten
    std::vector<std::size_t> starts;
    const std::size_t n = kanten.size();
    const std::size_t m = std::min<std::size_t>(n, static_cast<std::size_t>(std::max(1, o.maxKanten)));
    for (std::size_t i = 0; i < m; ++i) { starts.push_back(kanten[i * n / m]); }
    // --- Spruenge ---
    const auto boden = [&](const NavVec& p, float unter) {   // gibt es unter p einen Boden bis z "unter"?
        const NavSpur tr = spurAus(spur, p, kMins, kMaxsDucken, NavVec{p[0], p[1], unter});
        return tr.fraction < 1.0F && tr.normal[2] >= kMinWalkNormal && !tr.startsolid;
    };
    for (std::size_t si = 0; si < starts.size(); ++si) {
        const NavVec start = r.punkte[starts[si]].ort;
        for (int d = 0; d < std::max(1, o.richtungen); ++d) {
            if (abbrechen != nullptr && abbrechen->load()) { return r; }
            const float gier = 360.0F * static_cast<float>(d) / static_cast<float>(std::max(1, o.richtungen));
            const auto ziel = [&](const NavVec& l, const std::vector<NavVec>& bahn) {
                if (waagerecht(start, l) < rs * 1.5F || l[2] < start[2] - o.fallGrenze) { return false; }
                if (l[2] - start[2] > kStufe) { return true; }   // hinauf gesprungen
                // eine Luecke: unter der Mitte der Bahn kein Boden in Stufenhoehe
                const NavVec mitte = bahn[bahn.size() / 2];
                return !boden(mitte, std::min(start[2], l[2]) - kStufe - 1.0F);
            };
            NavVec l0{};
            std::vector<NavVec> b0;
            const bool g0 = sprungSimulieren(spur, start, gier, 0, o.jk2, o.bildMs, l0, &b0);
            ++r.simuliert;
            const bool z0 = g0 && ziel(l0, b0);
            if (z0) { r.spruenge.push_back({start, l0, false, waagerecht(start, l0), l0[2] - start[2], b0}); }
            if (o.machtStufe > 0) {
                NavVec l1{};
                std::vector<NavVec> b1;
                const bool g1 = sprungSimulieren(spur, start, gier, o.machtStufe, o.jk2, o.bildMs, l1, &b1);
                ++r.simuliert;
                const bool weiter = !z0 || waagerecht(start, l1) > waagerecht(start, l0) + rs || l1[2] > l0[2] + kStufe;
                if (g1 && ziel(l1, b1) && weiter) { r.spruenge.push_back({start, l1, true, waagerecht(start, l1), l1[2] - start[2], b1}); }
            }
        }
        if (fortschritt != nullptr) { fortschritt->store(200 + static_cast<int>(800 * (si + 1) / std::max<std::size_t>(1, starts.size()))); }
    }
    if (fortschritt != nullptr) { fortschritt->store(1000); }
    return r;
}

}  // namespace bhed::karte
