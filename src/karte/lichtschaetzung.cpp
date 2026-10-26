// lichtschaetzung.cpp - Karten-Editor Stufe 8a Punkt 4: Lichter einer fertigen .bsp
// schaetzen (siehe lichtschaetzung.h; experimentell).
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
#include "bhed/karte/lichtschaetzung.h"

#include "bhed/bsp.h"
#include "bhed/bspgeo.h"
#include "bhed/karte/bspoeffnen.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <set>
#include <unordered_map>

namespace bhed::karte {

namespace {

constexpr double kPointScale = 7500.0;   // light.cpp pointScale

struct Strahl {
    double c[3];   // Mitte der Zelle
    double d[3];   // Richtung zum Licht
    double weit;   // bis zum ersten Festen
    double gerichtet[3];
    double staerke;   // groesster Farbanteil des gerichteten Lichts
};

std::uint64_t schluessel(std::int64_t x, std::int64_t y, std::int64_t z) {
    return (static_cast<std::uint64_t>(x & 0x1FFFFF) << 42) ^ (static_cast<std::uint64_t>(y & 0x1FFFFF) << 21) ^ static_cast<std::uint64_t>(z & 0x1FFFFF);
}

std::string zahl(double v, int stellen) {
    char z[64];
    std::snprintf(z, sizeof(z), "%.*f", stellen, v);
    std::string s = z;
    if (s.find('.') != std::string::npos) {
        while (!s.empty() && s.back() == '0') { s.pop_back(); }
        if (!s.empty() && s.back() == '.') { s.pop_back(); }
    }
    if (s == "-0") { s = "0"; }
    return s;
}

// Punkt mit kleinster gewichteter Quadratsumme der Abstaende zu den Strahlen
bool naechsterPunkt(const std::vector<Strahl>& s, const std::vector<int>& welche, double p[3]) {
    double M[3][3] = {};
    double b[3] = {0, 0, 0};
    double gesamt = 0;
    for (const int i : welche) {
        const Strahl& r = s[static_cast<std::size_t>(i)];
        const double w = std::sqrt(r.staerke);
        gesamt += w;
        for (int a = 0; a < 3; ++a) {
            for (int c = 0; c < 3; ++c) {
                const double m = (a == c ? 1.0 : 0.0) - r.d[a] * r.d[c];
                M[a][c] += w * m;
                b[a] += w * m * r.c[c];
            }
        }
    }
    if (gesamt <= 0) { return false; }
    const double det = M[0][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1]) - M[0][1] * (M[1][0] * M[2][2] - M[1][2] * M[2][0]) +
                       M[0][2] * (M[1][0] * M[2][1] - M[1][1] * M[2][0]);
    if (std::fabs(det) < 1e-9 * gesamt * gesamt * gesamt) { return false; }
    double I[3][3];
    I[0][0] = (M[1][1] * M[2][2] - M[1][2] * M[2][1]) / det;
    I[0][1] = (M[0][2] * M[2][1] - M[0][1] * M[2][2]) / det;
    I[0][2] = (M[0][1] * M[1][2] - M[0][2] * M[1][1]) / det;
    I[1][0] = (M[1][2] * M[2][0] - M[1][0] * M[2][2]) / det;
    I[1][1] = (M[0][0] * M[2][2] - M[0][2] * M[2][0]) / det;
    I[1][2] = (M[0][2] * M[1][0] - M[0][0] * M[1][2]) / det;
    I[2][0] = (M[1][0] * M[2][1] - M[1][1] * M[2][0]) / det;
    I[2][1] = (M[0][1] * M[2][0] - M[0][0] * M[2][1]) / det;
    I[2][2] = (M[0][0] * M[1][1] - M[0][1] * M[1][0]) / det;
    for (int a = 0; a < 3; ++a) { p[a] = I[a][0] * b[0] + I[a][1] * b[1] + I[a][2] * b[2]; }
    return true;
}

// Abstand eines Punkts zum Strahl, t = Lage auf dem Strahl
double abstand(const Strahl& r, const double p[3], double* t) {
    const double v[3] = {p[0] - r.c[0], p[1] - r.c[1], p[2] - r.c[2]};
    const double tt = v[0] * r.d[0] + v[1] * r.d[1] + v[2] * r.d[2];
    if (t != nullptr) { *t = tt; }
    const double q[3] = {v[0] - tt * r.d[0], v[1] - tt * r.d[1], v[2] - tt * r.d[2]};
    return std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2]);
}

}  // namespace

std::vector<GeschaetztesLicht> lichterSchaetzen(const std::string& bspDaten, const LichtSchaetzOptionen& o) {
    std::vector<GeschaetztesLicht> out;
    BspGeometry g;
    if (!readBspGeometry(bspDaten, g) || g.lightGrid.empty() || g.lightArray.empty()) { return out; }
    const auto weiter = [&o](double x) { return !o.fortschritt || o.fortschritt(x); };
    // 1. Strahlen aus dem Gitter
    std::vector<Strahl> strahlen;
    const long long bx = g.lightGridBounds[0];
    const long long by = g.lightGridBounds[1];
    const long long bz = g.lightGridBounds[2];
    constexpr double kZuRad = 2.0 * 3.14159265358979 / 256.0;
    for (long long z = 0; z < bz; ++z) {
        if (!weiter(0.3 * static_cast<double>(z) / static_cast<double>(std::max(1LL, bz)))) { return {}; }
        for (long long y = 0; y < by; ++y) {
            for (long long x = 0; x < bx; ++x) {
                const long long i = x + y * bx + z * bx * by;
                if (i < 0 || static_cast<std::size_t>(i) >= g.lightArray.size()) { continue; }
                const std::size_t idx = g.lightArray[static_cast<std::size_t>(i)];
                if (idx >= g.lightGrid.size()) { continue; }
                const BspGeometry::LightGridPoint& p = g.lightGrid[idx];
                if (p.styles[0] == 0xFF) { continue; }
                const double staerke = std::max({p.direct[0][0], p.direct[0][1], p.direct[0][2]});
                if (staerke < 6) { continue; }
                Strahl s{};
                s.c[0] = g.lightGridOrigin[0] + static_cast<double>(x) * g.lightGridSize[0];
                s.c[1] = g.lightGridOrigin[1] + static_cast<double>(y) * g.lightGridSize[1];
                s.c[2] = g.lightGridOrigin[2] + static_cast<double>(z) * g.lightGridSize[2];
                const double lat = static_cast<double>(p.latLong[1]) * kZuRad;
                const double lng = static_cast<double>(p.latLong[0]) * kZuRad;
                s.d[0] = std::cos(lat) * std::sin(lng);
                s.d[1] = std::sin(lat) * std::sin(lng);
                s.d[2] = std::cos(lng);
                for (int c = 0; c < 3; ++c) { s.gerichtet[c] = p.direct[0][c]; }
                s.staerke = staerke;
                s.weit = o.reichweite;
                strahlen.push_back(s);
            }
        }
    }
    // 2. Wie weit jeder Strahl frei ist (das Licht steht im selben Raum)
    for (std::size_t i = 0; i < strahlen.size(); ++i) {
        if ((i % 4096) == 0 && !weiter(0.3 + 0.3 * static_cast<double>(i) / static_cast<double>(strahlen.size()))) { return {}; }
        Strahl& s = strahlen[i];
        const float a[3] = {static_cast<float>(s.c[0]), static_cast<float>(s.c[1]), static_cast<float>(s.c[2])};
        const float e[3] = {static_cast<float>(s.c[0] + s.d[0] * o.reichweite), static_cast<float>(s.c[1] + s.d[1] * o.reichweite),
                            static_cast<float>(s.c[2] + s.d[2] * o.reichweite)};
        const TraceTreffer t = traceRay(g, a, e);
        if (t.hit) { s.weit = std::max(0.0, static_cast<double>(t.fraction) * o.reichweite); }
    }
    // 3. Stimmen im Raster
    const double h = o.raster;
    std::unordered_map<std::uint64_t, double> stimmen;
    const auto zelle = [h](double v) { return static_cast<std::int64_t>(std::floor(v / h)); };
    for (const Strahl& s : strahlen) {
        const double w = std::sqrt(s.staerke);
        for (double t = 16.0; t < s.weit; t += h * 0.5) {
            stimmen[schluessel(zelle(s.c[0] + s.d[0] * t), zelle(s.c[1] + s.d[1] * t), zelle(s.c[2] + s.d[2] * t))] += w;
        }
    }
    if (!weiter(0.7)) { return {}; }
    // 4. Gipfel: groesser als alle 26 Nachbarn
    double groesste = 0;
    for (const auto& [k, w] : stimmen) { groesste = std::max(groesste, w); }
    struct Gipfel {
        std::int64_t x, y, z;
        double w;
    };
    std::vector<Gipfel> gipfel;
    for (const auto& [k, w] : stimmen) {
        if (w < o.mindestStimmen * groesste) { continue; }
        std::int64_t x = static_cast<std::int64_t>((k >> 42) & 0x1FFFFF);
        std::int64_t y = static_cast<std::int64_t>((k >> 21) & 0x1FFFFF);
        std::int64_t z = static_cast<std::int64_t>(k & 0x1FFFFF);
        if (x >= (1 << 20)) { x -= (1 << 21); }
        if (y >= (1 << 20)) { y -= (1 << 21); }
        if (z >= (1 << 20)) { z -= (1 << 21); }
        bool max = true;
        for (int dx = -1; dx <= 1 && max; ++dx) {
            for (int dy = -1; dy <= 1 && max; ++dy) {
                for (int dz = -1; dz <= 1 && max; ++dz) {
                    if (dx == 0 && dy == 0 && dz == 0) { continue; }
                    const auto it = stimmen.find(schluessel(x + dx, y + dy, z + dz));
                    if (it != stimmen.end() && it->second > w) { max = false; }
                }
            }
        }
        if (max) { gipfel.push_back({x, y, z, w}); }
    }
    std::sort(gipfel.begin(), gipfel.end(), [](const Gipfel& a, const Gipfel& b) { return a.w > b.w; });
    if (gipfel.size() > 4000) { gipfel.resize(4000); }
    // 5. Je Gipfel die Strahlen, die nahe vorbeilaufen, und der Punkt dazu
    std::unordered_map<std::uint64_t, std::vector<int>> strahlenJeZelle;
    {
        std::set<std::uint64_t> gz;
        for (const Gipfel& p : gipfel) { gz.insert(schluessel(p.x, p.y, p.z)); }
        for (std::size_t i = 0; i < strahlen.size(); ++i) {
            const Strahl& s = strahlen[i];
            std::uint64_t letzte = ~0ULL;
            for (double t = 16.0; t < s.weit; t += h * 0.5) {
                const std::uint64_t k = schluessel(zelle(s.c[0] + s.d[0] * t), zelle(s.c[1] + s.d[1] * t), zelle(s.c[2] + s.d[2] * t));
                if (k == letzte) { continue; }
                letzte = k;
                if (gz.count(k) != 0) { strahlenJeZelle[k].push_back(static_cast<int>(i)); }
            }
        }
    }
    if (!weiter(0.85)) { return {}; }
    // Ein Strahl, den ein gefundenes Licht erklaert (er laeuft nahe daran vorbei), stimmt
    // fuer kein weiteres - sonst entstehen um jedes echte Licht Nebengipfel
    std::vector<bool> erklaert(strahlen.size(), false);
    const auto nahe = [](const Strahl& s, const double p[3]) {
        double t = 0;
        const double d = abstand(s, p, &t);
        return t > 8.0 && t < s.weit + 32.0 && d < std::max(12.0, 0.06 * t);
    };
    for (const Gipfel& gp : gipfel) {
        const auto it = strahlenJeZelle.find(schluessel(gp.x, gp.y, gp.z));
        if (it == strahlenJeZelle.end()) { continue; }
        std::vector<int> offen;
        for (const int i : it->second) {
            if (!erklaert[static_cast<std::size_t>(i)]) { offen.push_back(i); }
        }
        if (offen.size() < 8) { continue; }
        std::vector<int> welche = offen;
        double p[3] = {(static_cast<double>(gp.x) + 0.5) * h, (static_cast<double>(gp.y) + 0.5) * h, (static_cast<double>(gp.z) + 0.5) * h};
        for (int iter = 0; iter < 3; ++iter) {
            double q[3];
            if (!naechsterPunkt(strahlen, welche, q)) { break; }
            p[0] = q[0];
            p[1] = q[1];
            p[2] = q[2];
            std::vector<int> neu;
            for (const int i : offen) {
                if (nahe(strahlen[static_cast<std::size_t>(i)], p)) { neu.push_back(i); }
            }
            if (neu.size() < 8) { break; }
            welche.swap(neu);
        }
        if (welche.size() < 8) { continue; }
        // Staerke und Farbe aus ungesaettigten Zellen
        std::vector<double> lichtWerte;
        double farbe[3] = {0, 0, 0};
        for (const int i : welche) {
            const Strahl& s = strahlen[static_cast<std::size_t>(i)];
            const double dx = p[0] - s.c[0];
            const double dy = p[1] - s.c[1];
            const double dz = p[2] - s.c[2];
            const double d = std::max(16.0, std::sqrt(dx * dx + dy * dy + dz * dz));
            if (s.staerke >= 250 || d < 24.0) { continue; }
            lichtWerte.push_back(s.staerke * d * d / kPointScale);
            for (int c = 0; c < 3; ++c) { farbe[c] += s.gerichtet[c] / s.staerke; }
        }
        if (lichtWerte.size() < 2) { continue; }
        std::nth_element(lichtWerte.begin(), lichtWerte.begin() + static_cast<std::ptrdiff_t>(lichtWerte.size() / 2), lichtWerte.end());
        GeschaetztesLicht l;
        for (int c = 0; c < 3; ++c) { l.origin[c] = p[c]; }
        l.licht = lichtWerte[lichtWerte.size() / 2];
        const double fm = std::max({farbe[0], farbe[1], farbe[2]});
        for (int c = 0; c < 3; ++c) { l.farbe[c] = fm > 0 ? farbe[c] / fm : 1.0; }
        l.stimmen = gp.w;
        l.zellen = static_cast<int>(lichtWerte.size());
        // im Festen? dann verworfen
        bool frei = true;
        for (const GeschaetztesLicht& a : out) {
            const double dx = a.origin[0] - l.origin[0];
            const double dy = a.origin[1] - l.origin[1];
            const double dz = a.origin[2] - l.origin[2];
            if (dx * dx + dy * dy + dz * dz < 48.0 * 48.0) {
                frei = false;
                break;
            }
        }
        if (!frei) { continue; }
        out.push_back(l);
        for (std::size_t i = 0; i < strahlen.size(); ++i) {
            if (!erklaert[i] && nahe(strahlen[i], p)) { erklaert[i] = true; }
        }
    }
    // 6. Spotlicht-Ziele: info_null mit targetname, auf die nichts mehr zeigt
    std::vector<MapEntity> ents;
    {
        BspDatei b;
        if (bspLesen(bspDaten, b)) { parseEntities(bspEntitaetenText(b), ents); }
    }
    // Lichter, die in der .bsp stehen geblieben sind (mit targetname bzw. -keeplights), sind
    // keine Schaetzung: was nahe daran liegt, faellt weg
    for (const MapEntity& e : ents) {
        std::string klasse;
        std::string ort;
        for (const auto& [k, v] : e.keys) {
            if (k == "classname") { klasse = v; }
            if (k == "origin") { ort = v; }
        }
        double q[3];
        if (klasse.rfind("light", 0) != 0 || std::sscanf(ort.c_str(), "%lf %lf %lf", &q[0], &q[1], &q[2]) != 3) { continue; }
        out.erase(std::remove_if(out.begin(), out.end(),
                                 [&q](const GeschaetztesLicht& l) {
                                     const double dx = l.origin[0] - q[0];
                                     const double dy = l.origin[1] - q[1];
                                     const double dz = l.origin[2] - q[2];
                                     return dx * dx + dy * dy + dz * dz < 48.0 * 48.0;
                                 }),
                  out.end());
    }
    std::set<std::string> gezielt;
    for (const MapEntity& e : ents) {
        for (const auto& [k, v] : e.keys) {
            if (k == "target" || k == "target2") { gezielt.insert(v); }
        }
    }
    struct Ziel {
        std::string name;
        double p[3];
        bool vergeben = false;
    };
    std::vector<Ziel> ziele;
    for (const MapEntity& e : ents) {
        std::string klasse;
        std::string name;
        std::string ort;
        for (const auto& [k, v] : e.keys) {
            if (k == "classname") { klasse = v; }
            if (k == "targetname") { name = v; }
            if (k == "origin") { ort = v; }
        }
        if ((klasse != "info_null" && klasse != "info_notnull") || name.empty() || gezielt.count(name) != 0) { continue; }
        Ziel z;
        z.name = name;
        if (std::sscanf(ort.c_str(), "%lf %lf %lf", &z.p[0], &z.p[1], &z.p[2]) == 3) { ziele.push_back(z); }
    }
    std::sort(out.begin(), out.end(), [](const GeschaetztesLicht& a, const GeschaetztesLicht& b) { return a.stimmen > b.stimmen; });
    for (GeschaetztesLicht& l : out) {
        int best = -1;
        double bd = 1024.0 * 1024.0;
        for (std::size_t i = 0; i < ziele.size(); ++i) {
            if (ziele[i].vergeben || ziele[i].p[2] > l.origin[2] - 8.0) { continue; }
            const double dx = ziele[i].p[0] - l.origin[0];
            const double dy = ziele[i].p[1] - l.origin[1];
            const double dz = ziele[i].p[2] - l.origin[2];
            const double d2 = dx * dx + dy * dy + dz * dz;
            if (d2 >= bd) { continue; }
            const float a[3] = {static_cast<float>(l.origin[0]), static_cast<float>(l.origin[1]), static_cast<float>(l.origin[2])};
            const float e[3] = {static_cast<float>(ziele[i].p[0]), static_cast<float>(ziele[i].p[1]), static_cast<float>(ziele[i].p[2])};
            if (traceRay(g, a, e).hit) { continue; }
            bd = d2;
            best = static_cast<int>(i);
        }
        if (best >= 0) {
            ziele[static_cast<std::size_t>(best)].vergeben = true;
            l.ziel = ziele[static_cast<std::size_t>(best)].name;
        }
    }
    (void)weiter(1.0);
    return out;
}

std::vector<std::pair<std::string, std::string>> lichtSchluessel(const GeschaetztesLicht& l) {
    std::vector<std::pair<std::string, std::string>> kv;
    kv.emplace_back("classname", "light");
    kv.emplace_back("origin", zahl(l.origin[0], 1) + " " + zahl(l.origin[1], 1) + " " + zahl(l.origin[2], 1));
    kv.emplace_back("light", zahl(l.licht, 0));
    if (l.farbe[0] < 0.995 || l.farbe[1] < 0.995 || l.farbe[2] < 0.995) {
        kv.emplace_back("_color", zahl(l.farbe[0], 3) + " " + zahl(l.farbe[1], 3) + " " + zahl(l.farbe[2], 3));
    }
    if (!l.ziel.empty()) { kv.emplace_back("target", l.ziel); }
    kv.emplace_back("_bhed_geschaetzt", "1");
    return kv;
}

}  // namespace bhed::karte
