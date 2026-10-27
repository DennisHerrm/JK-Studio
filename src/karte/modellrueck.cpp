// modellrueck.cpp - Karten-Editor Stufe 8a Punkt 3: misc_model aus der Geometrie
// einer fertigen .bsp zurueckgewinnen (siehe modellrueck.h).
//
// Portiert aus q3map2 (NetRadiant-Custom 20260114: model.cpp InsertModel,
// ModelGetTransform - Lage, Winkel und Skalierung eines misc_model;
// surface_meta.cpp - wie forcemeta-Modelle in Meta-Flaechen aufgehen).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   Copyright (C) 2026 BehavEd-Remake contributors.
//
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. This program is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/modellrueck.h"

#include "bhed/karte/bspoeffnen.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texdef.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <map>
#include <tuple>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace bhed::karte {

namespace {

constexpr double kGrad = 57.29577951308232;

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    std::replace(s.begin(), s.end(), '\\', '/');
    return s;
}

std::string ohneEndung(const std::string& s) {
    const std::size_t p = s.rfind('.');
    const std::size_t q = s.rfind('/');
    if (p == std::string::npos || (q != std::string::npos && p < q)) { return s; }
    return s.substr(0, p);
}

std::string ordnerVon(const std::string& s) {
    const std::size_t q = s.rfind('/');
    return q == std::string::npos ? std::string() : s.substr(0, q);
}

template <typename T>
bool lies(const std::string& d, std::size_t& pos, T& v) {
    if (pos + sizeof(T) > d.size()) { return false; }
    std::memcpy(&v, d.data() + pos, sizeof(T));
    pos += sizeof(T);
    return true;
}

bool liesText(const std::string& d, std::size_t& pos, std::string& t) {
    std::uint32_t n = 0;
    if (!lies(d, pos, n) || pos + n > d.size()) { return false; }
    t.assign(d.data() + pos, n);
    pos += n;
    return true;
}

// Dreiecksmenge als Fingerabdruck: jedes Dreieck sortiert, die Liste sortiert
std::uint64_t dreiecksSignatur(std::vector<std::array<std::uint32_t, 3>> d) {
    for (auto& t : d) { std::sort(t.begin(), t.end()); }
    std::sort(d.begin(), d.end());
    std::uint64_t h = 1469598103934665603ULL;
    for (const auto& t : d) {
        for (const std::uint32_t x : t) {
            h ^= x;
            h *= 1099511628211ULL;
        }
    }
    return h;
}

// Symmetrische 3x3-Eigenzerlegung (Jacobi): Werte absteigend, Vektoren als Spalten
void eigen3(const double a0[3][3], double w[3], double v[3][3]) {
    double a[3][3];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            a[i][j] = a0[i][j];
            v[i][j] = i == j ? 1.0 : 0.0;
        }
    }
    for (int iter = 0; iter < 50; ++iter) {
        double off = std::fabs(a[0][1]) + std::fabs(a[0][2]) + std::fabs(a[1][2]);
        if (off < 1e-30) { break; }
        for (int p = 0; p < 2; ++p) {
            for (int q = p + 1; q < 3; ++q) {
                if (std::fabs(a[p][q]) < 1e-300) { continue; }
                const double th = (a[q][q] - a[p][p]) / (2.0 * a[p][q]);
                const double t = (th >= 0 ? 1.0 : -1.0) / (std::fabs(th) + std::sqrt(th * th + 1.0));
                const double c = 1.0 / std::sqrt(t * t + 1.0);
                const double s = t * c;
                for (int k = 0; k < 3; ++k) {
                    const double akp = a[k][p];
                    const double akq = a[k][q];
                    a[k][p] = c * akp - s * akq;
                    a[k][q] = s * akp + c * akq;
                }
                for (int k = 0; k < 3; ++k) {
                    const double apk = a[p][k];
                    const double aqk = a[q][k];
                    a[p][k] = c * apk - s * aqk;
                    a[q][k] = s * apk + c * aqk;
                }
                for (int k = 0; k < 3; ++k) {
                    const double vkp = v[k][p];
                    const double vkq = v[k][q];
                    v[k][p] = c * vkp - s * vkq;
                    v[k][q] = s * vkp + c * vkq;
                }
            }
        }
    }
    int ord[3] = {0, 1, 2};
    std::sort(ord, ord + 3, [&a](int x, int y) { return a[x][x] > a[y][y]; });
    double vv[3][3];
    for (int k = 0; k < 3; ++k) {
        w[k] = a[ord[k]][ord[k]];
        for (int i = 0; i < 3; ++i) { vv[i][k] = v[i][ord[k]]; }
    }
    std::memcpy(v, vv, sizeof(vv));
}

double det3(const double m[3][3]) {
    return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
           m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

// Affine Ausgleichsrechnung: A (Zeilen = Welt, Spalten = Modell) und t mit v = A m + t.
// haendigkeit: +1/-1 aus der Dreiecksreihenfolge (fuer ebene Netze). false bei zu wenig Ecken.
bool affinLoesen(const std::vector<std::array<double, 3>>& m, const std::vector<std::array<double, 3>>& v, int haendigkeit, double A[3][3],
                 double t[3], bool* eben) {
    const std::size_t n = m.size();
    if (n < 3 || v.size() != n) { return false; }
    double mc[3] = {0, 0, 0};
    double vc[3] = {0, 0, 0};
    for (std::size_t i = 0; i < n; ++i) {
        for (int k = 0; k < 3; ++k) {
            mc[k] += m[i][k];
            vc[k] += v[i][k];
        }
    }
    for (int k = 0; k < 3; ++k) {
        mc[k] /= static_cast<double>(n);
        vc[k] /= static_cast<double>(n);
    }
    double S[3][3] = {};   // Sum X X^T
    double C[3][3] = {};   // Sum Y X^T
    for (std::size_t i = 0; i < n; ++i) {
        double x[3];
        double y[3];
        for (int k = 0; k < 3; ++k) {
            x[k] = m[i][k] - mc[k];
            y[k] = v[i][k] - vc[k];
        }
        for (int a = 0; a < 3; ++a) {
            for (int b = 0; b < 3; ++b) {
                S[a][b] += x[a] * x[b];
                C[a][b] += y[a] * x[b];
            }
        }
    }
    double w[3];
    double E[3][3];
    eigen3(S, w, E);
    if (w[0] <= 0.0) { return false; }
    // A = C S^-1 = sum_k (C e_k) e_k^T / w_k
    double a[3][3];   // a[.][k] = A e_k
    const bool flach = w[2] < 1e-9 * w[0];
    if (eben != nullptr) { *eben = flach; }
    if (w[1] < 1e-9 * w[0]) { return false; }   // Linie: nicht bestimmbar
    for (int k = 0; k < (flach ? 2 : 3); ++k) {
        for (int r = 0; r < 3; ++r) {
            double s = 0;
            for (int c = 0; c < 3; ++c) { s += C[r][c] * E[c][k]; }
            a[r][k] = s / w[k];
        }
    }
    if (flach) {
        // Senkrecht zur Ebene ist nichts gemessen: die mittlere Skalierung in der Ebene,
        // die Richtung aus dem Kreuzprodukt, das Vorzeichen aus der Haendigkeit
        const double kx = a[1][0] * a[2][1] - a[2][0] * a[1][1];
        const double ky = a[2][0] * a[0][1] - a[0][0] * a[2][1];
        const double kz = a[0][0] * a[1][1] - a[1][0] * a[0][1];
        const double kl = std::sqrt(kx * kx + ky * ky + kz * kz);
        if (kl < 1e-12) { return false; }
        const double l1 = std::sqrt(a[0][0] * a[0][0] + a[1][0] * a[1][0] + a[2][0] * a[2][0]);
        const double l2 = std::sqrt(a[0][1] * a[0][1] + a[1][1] * a[1][1] + a[2][1] * a[2][1]);
        const double s = std::sqrt(l1 * l2);
        const double detE = det3(E);
        const double vz = (haendigkeit >= 0 ? 1.0 : -1.0) * (detE >= 0 ? 1.0 : -1.0);
        a[0][2] = vz * s * kx / kl;
        a[1][2] = vz * s * ky / kl;
        a[2][2] = vz * s * kz / kl;
    }
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            double s = 0;
            for (int k = 0; k < 3; ++k) { s += a[r][k] * E[c][k]; }
            A[r][c] = s;
        }
    }
    for (int r = 0; r < 3; ++r) { t[r] = vc[r] - (A[r][0] * mc[0] + A[r][1] * mc[1] + A[r][2] * mc[2]); }
    return true;
}

struct Raw {
    std::vector<std::string> shader;
    std::vector<std::array<float, 4>> ebenen;
    struct Seite {
        int ebene;
        int shader;
    };
    std::vector<Seite> seiten;
    struct Modell {
        int erste;
        int anzahl;
    };
    std::vector<Modell> modelle;
    struct Flaeche {
        int shader, typ, fv, nv, fi, ni;
    };
    std::vector<Flaeche> flaechen;
    std::vector<std::array<float, 3>> xyz;
    std::vector<std::array<float, 2>> st;
    std::vector<int> idx;
    std::string entitaeten;
};

bool rawLesen(const std::string& d, Raw& r) {
    BspDatei b;
    if (!bspLesen(d, b) || b.ident != "RBSP" || b.lumps.size() < 18) { return false; }
    const auto& L = b.lumps;
    for (std::size_t i = 0; i + 72 <= L[1].size(); i += 72) { r.shader.emplace_back(L[1].data() + i, strnlen(L[1].data() + i, 64)); }
    for (std::size_t i = 0; i + 16 <= L[2].size(); i += 16) {
        std::array<float, 4> e{};
        std::memcpy(e.data(), L[2].data() + i, 16);
        r.ebenen.push_back(e);
    }
    for (std::size_t i = 0; i + 40 <= L[7].size(); i += 40) {
        int v[4];
        std::memcpy(v, L[7].data() + i + 24, 16);
        r.modelle.push_back({v[0], v[1]});
    }
    for (std::size_t i = 0; i + 12 <= L[9].size(); i += 12) {
        int v[2];
        std::memcpy(v, L[9].data() + i, 8);
        r.seiten.push_back({v[0], v[1]});
    }
    for (std::size_t i = 0; i + 80 <= L[10].size(); i += 80) {
        std::array<float, 3> p{};
        std::array<float, 2> s{};
        std::memcpy(p.data(), L[10].data() + i, 12);
        std::memcpy(s.data(), L[10].data() + i + 12, 8);
        r.xyz.push_back(p);
        r.st.push_back(s);
    }
    for (std::size_t i = 0; i + 4 <= L[11].size(); i += 4) {
        int v = 0;
        std::memcpy(&v, L[11].data() + i, 4);
        r.idx.push_back(v);
    }
    for (std::size_t i = 0; i + 148 <= L[13].size(); i += 148) {
        int v[7];
        std::memcpy(v, L[13].data() + i, 28);
        r.flaechen.push_back({v[0], v[2], v[3], v[4], v[5], v[6]});
    }
    r.entitaeten = bspEntitaetenText(b);
    return true;
}

// Die Entitaeten des Lumps (nur Schluessel)
std::vector<std::vector<std::pair<std::string, std::string>>> entitaetenLesen(const std::string& t) {
    std::vector<std::vector<std::pair<std::string, std::string>>> out;
    std::size_t i = 0;
    std::vector<std::pair<std::string, std::string>> cur;
    bool drin = false;
    const auto token = [&](std::string& s) {
        while (i < t.size() && t[i] != '"' && t[i] != '{' && t[i] != '}') { ++i; }
        if (i >= t.size()) { return 0; }
        if (t[i] == '{') {
            ++i;
            return 1;
        }
        if (t[i] == '}') {
            ++i;
            return 2;
        }
        const std::size_t e = t.find('"', i + 1);
        if (e == std::string::npos) {
            i = t.size();
            return 0;
        }
        s = t.substr(i + 1, e - i - 1);
        i = e + 1;
        return 3;
    };
    std::string k;
    std::string w;
    for (;;) {
        const int a = token(k);
        if (a == 0) { break; }
        if (a == 1) {
            cur.clear();
            drin = true;
        } else if (a == 2) {
            if (drin) { out.push_back(cur); }
            drin = false;
        } else if (token(w) == 3) {
            cur.emplace_back(k, w);
        }
    }
    return out;
}

std::string wertVon(const std::vector<std::pair<std::string, std::string>>& e, const char* k) {
    for (const auto& [a, b] : e) {
        if (klein(a) == k) { return b; }
    }
    return {};
}

std::string zahl(double v, int stellen) {
    if (std::fabs(v) < 0.5 * std::pow(10.0, -stellen)) { v = 0; }
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

struct Vorlage {
    int modell;
    int netz;
    std::uint64_t signatur;
};

struct Paarung {
    int flaeche;
    int modell;
    int netz;
    double A[3][3];
    double t[3];
    double rest;
    double maxF;
};

}  // namespace

bool modellDumpLesen(const std::string& d, std::vector<ModellDatei>& out) {
    out.clear();
    std::size_t pos = 0;
    if (d.size() < 12 || d.compare(0, 4, "BHMD") != 0) { return false; }
    pos = 4;
    std::uint32_t fassung = 0;
    std::uint32_t anzahl = 0;
    if (!lies(d, pos, fassung) || (fassung != 1 && fassung != 2) || !lies(d, pos, anzahl)) { return false; }
    for (std::uint32_t i = 0; i < anzahl; ++i) {
        ModellDatei m;
        std::uint32_t frame = 0;
        std::uint32_t geladen = 0;
        std::uint32_t netze = 0;
        if (!liesText(d, pos, m.pfad) || !lies(d, pos, frame) || !lies(d, pos, geladen) || !lies(d, pos, netze)) { return false; }
        m.frame = static_cast<int>(frame);
        m.geladen = geladen != 0;
        for (std::uint32_t k = 0; k < netze; ++k) {
            ModellNetz n;
            std::uint32_t nv = 0;
            std::uint32_t nf = 0;
            if (!liesText(d, pos, n.shader) || !lies(d, pos, nv) || !lies(d, pos, nf)) { return false; }
            const std::size_t stBytes = fassung >= 2 ? static_cast<std::size_t>(nv) * 8 : 0;
            if (pos + static_cast<std::size_t>(nv) * 12 + stBytes + static_cast<std::size_t>(nf) * 12 > d.size()) { return false; }
            n.punkte.resize(nv);
            std::memcpy(n.punkte.data(), d.data() + pos, static_cast<std::size_t>(nv) * 12);
            pos += static_cast<std::size_t>(nv) * 12;
            n.st.resize(nv);
            if (stBytes > 0) {
                std::memcpy(n.st.data(), d.data() + pos, stBytes);
                pos += stBytes;
            }
            n.dreiecke.resize(nf);
            std::memcpy(n.dreiecke.data(), d.data() + pos, static_cast<std::size_t>(nf) * 12);
            pos += static_cast<std::size_t>(nf) * 12;
            m.netze.push_back(std::move(n));
        }
        out.push_back(std::move(m));
    }
    return true;
}

std::vector<std::string> modellKandidaten(const std::vector<std::string>& pfade, const std::vector<std::string>& bspShader) {
    std::set<std::string> ordner;
    for (const std::string& s : bspShader) {
        const std::string o = ordnerVon(klein(s));
        if (!o.empty()) { ordner.insert(o); }
    }
    std::vector<std::string> out;
    std::set<std::string> schon;
    for (const std::string& p : pfade) {
        const std::string k = klein(p);
        const std::size_t punkt = k.rfind('.');
        if (punkt == std::string::npos) { continue; }
        const std::string end = k.substr(punkt + 1);
        if (end != "md3" && end != "ase" && end != "obj" && end != "lwo" && end != "3ds" && end != "ms3d") { continue; }
        if (ordner.count(ordnerVon(k)) == 0) { continue; }
        if (schon.insert(k).second) { out.push_back(p); }
    }
    std::sort(out.begin(), out.end());
    return out;
}

void lageMatrix(const ModellLage& l, double m[3][3]) {
    // ModelGetTransform: angles_pyr2rpy, matrix4_rotation_for_euler_xyz_degrees (Spalten = Achsen)
    const double sx = std::sin(l.winkel[2] / kGrad);
    const double cx = std::cos(l.winkel[2] / kGrad);
    const double sy = std::sin(l.winkel[0] / kGrad);
    const double cy = std::cos(l.winkel[0] / kGrad);
    const double sz = std::sin(l.winkel[1] / kGrad);
    const double cz = std::cos(l.winkel[1] / kGrad);
    const double X[3] = {cy * cz, cy * sz, -sy};
    const double Y[3] = {sx * sy * cz - cx * sz, sx * sy * sz + cx * cz, sx * cy};
    const double Z[3] = {cx * sy * cz + sx * sz, cx * sy * sz - sx * cz, cx * cy};
    for (int r = 0; r < 3; ++r) {
        m[r][0] = X[r] * l.skala[0];
        m[r][1] = Y[r] * l.skala[1];
        m[r][2] = Z[r] * l.skala[2];
    }
}

ModellLage lageAusMatrix(const double m[3][3], const double t[3]) {
    ModellLage l;
    double R[3][3];
    for (int c = 0; c < 3; ++c) {
        const double s = std::sqrt(m[0][c] * m[0][c] + m[1][c] * m[1][c] + m[2][c] * m[2][c]);
        l.skala[c] = s;
        for (int r = 0; r < 3; ++r) { R[r][c] = s > 0 ? m[r][c] / s : 0.0; }
    }
    if (det3(R) < 0) {   // Spiegelung: eine Skalierung negativ (die x-Achse, wie "-500 500 500")
        l.skala[0] = -l.skala[0];
        for (int r = 0; r < 3; ++r) { R[r][0] = -R[r][0]; }
    }
    // Euler aus der Drehung (Spalten X, Y, Z): X.z = -sin(pitch)
    const double sp = std::clamp(-R[2][0], -1.0, 1.0);
    l.winkel[0] = std::asin(sp) * kGrad;
    if (std::fabs(sp) < 0.999999) {
        l.winkel[1] = std::atan2(R[1][0], R[0][0]) * kGrad;
        l.winkel[2] = std::atan2(R[2][1], R[2][2]) * kGrad;
    } else {   // senkrecht: Gier und Rollen fallen zusammen, Rollen 0
        l.winkel[1] = std::atan2(-R[0][1], R[1][1]) * kGrad;
        l.winkel[2] = 0;
    }
    for (int k = 0; k < 3; ++k) { l.origin[k] = t[k]; }
    return l;
}

std::vector<std::pair<std::string, std::string>> trefferSchluessel(const ModellTreffer& t) {
    std::vector<std::pair<std::string, std::string>> kv;
    kv.emplace_back("classname", "misc_model");
    kv.emplace_back("model", t.pfad);
    for (const auto& x : lageSchluessel(t.lage)) { kv.push_back(x); }
    if (!t.ziel.empty()) { kv.emplace_back("target", t.ziel); }
    if (t.forceMeta) { kv.emplace_back("spawnflags", "4"); }
    for (std::size_t i = 0; i < t.remaps.size(); ++i) {
        kv.emplace_back(i == 0 ? std::string("_remap") : "_remap" + std::to_string(i + 1), t.remaps[i].first + ";" + t.remaps[i].second);
    }
    return kv;
}

std::vector<std::pair<std::string, std::string>> lageSchluessel(const ModellLage& l) {
    std::vector<std::pair<std::string, std::string>> kv;
    kv.emplace_back("origin", zahl(l.origin[0], 2) + " " + zahl(l.origin[1], 2) + " " + zahl(l.origin[2], 2));
    const bool nurGier = std::fabs(l.winkel[0]) < 0.005 && std::fabs(l.winkel[2]) < 0.005;
    if (nurGier) {
        if (std::fabs(l.winkel[1]) >= 0.005) { kv.emplace_back("angle", zahl(l.winkel[1], 2)); }
    } else {
        kv.emplace_back("angles", zahl(l.winkel[0], 2) + " " + zahl(l.winkel[1], 2) + " " + zahl(l.winkel[2], 2));
    }
    const double s0 = l.skala[0];
    const bool gleich = std::fabs(l.skala[1] - s0) < 1e-4 * std::max(1.0, std::fabs(s0)) && std::fabs(l.skala[2] - s0) < 1e-4 * std::max(1.0, std::fabs(s0));
    if (gleich) {
        if (std::fabs(s0 - 1.0) > 1e-4) { kv.emplace_back("modelscale", zahl(s0, 4)); }
    } else {
        kv.emplace_back("modelscale_vec", zahl(l.skala[0], 4) + " " + zahl(l.skala[1], 4) + " " + zahl(l.skala[2], 4));
    }
    return kv;
}

ModellLage lageAusSchluesseln(const std::vector<std::pair<std::string, std::string>>& kv) {
    ModellLage l;
    const std::string o = wertVon(kv, "origin");
    if (!o.empty()) { (void)std::sscanf(o.c_str(), "%lf %lf %lf", &l.origin[0], &l.origin[1], &l.origin[2]); }
    const std::string sv = wertVon(kv, "modelscale_vec");
    const std::string s = wertVon(kv, "modelscale");
    if (!sv.empty()) {
        (void)std::sscanf(sv.c_str(), "%lf %lf %lf", &l.skala[0], &l.skala[1], &l.skala[2]);
    } else if (!s.empty()) {
        l.skala[0] = l.skala[1] = l.skala[2] = std::atof(s.c_str());
    }
    const std::string a = wertVon(kv, "angles");
    const std::string g = wertVon(kv, "angle");
    if (!a.empty()) {
        (void)std::sscanf(a.c_str(), "%lf %lf %lf", &l.winkel[0], &l.winkel[1], &l.winkel[2]);
    } else if (!g.empty()) {
        l.winkel[1] = std::atof(g.c_str());
    }
    return l;
}

RueckErgebnis modelleZurueckgewinnen(const std::string& bspDaten, const std::vector<ModellDatei>& modelle, const RueckOptionen& o) {
    RueckErgebnis erg;
    Raw r;
    if (!rawLesen(bspDaten, r)) { return erg; }
    // Vorlagen je Eckenzahl
    std::unordered_map<int, std::vector<Vorlage>> vorlagen;
    for (std::size_t m = 0; m < modelle.size(); ++m) {
        for (std::size_t k = 0; k < modelle[m].netze.size(); ++k) {
            const ModellNetz& n = modelle[m].netze[k];
            if (n.punkte.size() < 3 || n.dreiecke.empty()) { continue; }
            vorlagen[static_cast<int>(n.punkte.size())].push_back({static_cast<int>(m), static_cast<int>(k), dreiecksSignatur(n.dreiecke)});
        }
    }
    // Zu welchem BSP-Modell gehoert eine Flaeche
    std::vector<int> bspModellVon(r.flaechen.size(), 0);
    for (std::size_t mi = 0; mi < r.modelle.size(); ++mi) {
        for (int f = r.modelle[mi].erste; f < r.modelle[mi].erste + r.modelle[mi].anzahl; ++f) {
            if (f >= 0 && f < static_cast<int>(bspModellVon.size())) { bspModellVon[static_cast<std::size_t>(f)] = static_cast<int>(mi); }
        }
    }
    std::vector<Paarung> paare;
    std::vector<bool> getroffen(r.flaechen.size(), false);
    std::vector<int> soup;
    for (std::size_t f = 0; f < r.flaechen.size(); ++f) {
        if (r.flaechen[f].typ == 3) { soup.push_back(static_cast<int>(f)); }
    }
    erg.dreiecksflaechen = soup.size();
    for (std::size_t si = 0; si < soup.size(); ++si) {
        if (o.fortschritt && (si % 8) == 0 && !o.fortschritt(static_cast<double>(si) / static_cast<double>(std::max<std::size_t>(1, soup.size())))) {
            erg.abgebrochen = true;
            return erg;
        }
        const int f = soup[si];
        const Raw::Flaeche& fl = r.flaechen[static_cast<std::size_t>(f)];
        const auto it = vorlagen.find(fl.nv);
        if (it == vorlagen.end() || fl.fv < 0 || fl.fv + fl.nv > static_cast<int>(r.xyz.size()) || fl.fi < 0 ||
            fl.fi + fl.ni > static_cast<int>(r.idx.size()) || fl.ni % 3 != 0) {
            continue;
        }
        std::vector<std::array<std::uint32_t, 3>> dr;
        for (int i = 0; i < fl.ni; i += 3) {
            dr.push_back({static_cast<std::uint32_t>(r.idx[static_cast<std::size_t>(fl.fi + i)]), static_cast<std::uint32_t>(r.idx[static_cast<std::size_t>(fl.fi + i + 1)]),
                          static_cast<std::uint32_t>(r.idx[static_cast<std::size_t>(fl.fi + i + 2)])});
        }
        const std::uint64_t sig = dreiecksSignatur(dr);
        std::vector<std::array<double, 3>> v(static_cast<std::size_t>(fl.nv));
        double lo[3] = {1e30, 1e30, 1e30};
        double hi[3] = {-1e30, -1e30, -1e30};
        for (int i = 0; i < fl.nv; ++i) {
            for (int k = 0; k < 3; ++k) {
                v[static_cast<std::size_t>(i)][k] = r.xyz[static_cast<std::size_t>(fl.fv + i)][k];
                lo[k] = std::min(lo[k], v[static_cast<std::size_t>(i)][k]);
                hi[k] = std::max(hi[k], v[static_cast<std::size_t>(i)][k]);
            }
        }
        const double ausdehnung = std::max({hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2]});
        for (const Vorlage& vl : it->second) {
            if (vl.signatur != sig) { continue; }
            const ModellNetz& n = modelle[static_cast<std::size_t>(vl.modell)].netze[static_cast<std::size_t>(vl.netz)];
            // Haendigkeit: dasselbe Dreieck in gleicher (zyklischer) Reihenfolge?
            int hand = 1;
            {
                std::array<std::uint32_t, 3> a = n.dreiecke.front();
                auto s = a;
                std::sort(s.begin(), s.end());
                for (const auto& b : dr) {
                    auto sb = b;
                    std::sort(sb.begin(), sb.end());
                    if (sb != s) { continue; }
                    const bool gleich = (b[0] == a[0] && b[1] == a[1]) || (b[0] == a[1] && b[1] == a[2]) || (b[0] == a[2] && b[1] == a[0]);
                    hand = gleich ? 1 : -1;
                    break;
                }
            }
            std::vector<std::array<double, 3>> mp(n.punkte.size());
            for (std::size_t i = 0; i < n.punkte.size(); ++i) {
                for (int k = 0; k < 3; ++k) { mp[i][k] = n.punkte[i][k]; }
            }
            Paarung p{};
            p.flaeche = f;
            p.modell = vl.modell;
            p.netz = vl.netz;
            bool eben = false;
            if (!affinLoesen(mp, v, hand, p.A, p.t, &eben)) { continue; }
            double summe = 0;
            double maxF = 0;
            for (std::size_t i = 0; i < mp.size(); ++i) {
                double d2 = 0;
                for (int k = 0; k < 3; ++k) {
                    const double x = p.A[k][0] * mp[i][0] + p.A[k][1] * mp[i][1] + p.A[k][2] * mp[i][2] + p.t[k] - v[i][k];
                    d2 += x * x;
                }
                const double d = std::sqrt(d2);
                summe += d;
                maxF = std::max(maxF, d);
            }
            p.rest = summe / static_cast<double>(mp.size());
            p.maxF = maxF;
            if (maxF > o.schwelle + o.relSchwelle * ausdehnung) { continue; }
            paare.push_back(p);
        }
    }
    // Paarungen zu Treffern: je Modell Gruppen mit gleicher Abbildung
    std::vector<bool> benutzt(paare.size(), false);
    std::vector<ModellTreffer> treffer;
    std::vector<std::vector<int>> trefferPaare;
    const auto gleicheLage = [](const Paarung& a, const Paarung& b) {
        double s = 0;
        double d = 0;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                s = std::max(s, std::fabs(a.A[i][j]));
                d = std::max(d, std::fabs(a.A[i][j] - b.A[i][j]));
            }
        }
        double dt = 0;
        for (int i = 0; i < 3; ++i) { dt = std::max(dt, std::fabs(a.t[i] - b.t[i])); }
        return d <= 1e-3 * std::max(1.0, s) && dt <= 0.25;
    };
    // Reihenfolge: groesste Netze zuerst (sie bestimmen die Lage am besten)
    std::vector<std::size_t> ord(paare.size());
    for (std::size_t i = 0; i < ord.size(); ++i) { ord[i] = i; }
    std::stable_sort(ord.begin(), ord.end(), [&](std::size_t a, std::size_t b) {
        return modelle[static_cast<std::size_t>(paare[a].modell)].netze[static_cast<std::size_t>(paare[a].netz)].punkte.size() >
               modelle[static_cast<std::size_t>(paare[b].modell)].netze[static_cast<std::size_t>(paare[b].netz)].punkte.size();
    });
    struct Gruppe {
        int modell;
        std::vector<int> paare;
        std::set<int> netze;
        std::size_t ecken = 0;
    };
    std::vector<Gruppe> gruppen;
    for (const std::size_t i : ord) {
        bool rein = false;
        for (Gruppe& g : gruppen) {
            if (g.modell != paare[i].modell || g.netze.count(paare[i].netz) != 0 || !gleicheLage(paare[static_cast<std::size_t>(g.paare.front())], paare[i])) {
                continue;
            }
            g.paare.push_back(static_cast<int>(i));
            g.netze.insert(paare[i].netz);
            g.ecken += modelle[static_cast<std::size_t>(paare[i].modell)].netze[static_cast<std::size_t>(paare[i].netz)].punkte.size();
            rein = true;
            break;
        }
        if (!rein) {
            Gruppe g;
            g.modell = paare[i].modell;
            g.paare.push_back(static_cast<int>(i));
            g.netze.insert(paare[i].netz);
            g.ecken = modelle[static_cast<std::size_t>(paare[i].modell)].netze[static_cast<std::size_t>(paare[i].netz)].punkte.size();
            gruppen.push_back(std::move(g));
        }
    }
    // Eine Flaeche gehoert der Gruppe mit dem groessten Anteil gefundener Netze (bei
    // gleichen Modellen mehrdeutig)
    const auto anteil = [&](const Gruppe& g) {
        const ModellDatei& m = modelle[static_cast<std::size_t>(g.modell)];
        std::set<std::string> bspShader;
        for (const std::string& s : r.shader) { bspShader.insert(klein(s)); }
        std::size_t moeglich = 0;
        for (std::size_t k = 0; k < m.netze.size(); ++k) {
            // ein Netz, dessen Shader die .bsp gar nicht hat, ist nodraw (oder umbenannt) - zaehlt nur, wenn gefunden
            if (bspShader.count(klein(ohneEndung(m.netze[k].shader))) != 0 || g.netze.count(static_cast<int>(k)) != 0) { moeglich += m.netze[k].punkte.size(); }
        }
        return moeglich > 0 ? static_cast<double>(g.ecken) / static_cast<double>(moeglich) : 0.0;
    };
    std::vector<double> gAnteil(gruppen.size());
    for (std::size_t gi = 0; gi < gruppen.size(); ++gi) { gAnteil[gi] = anteil(gruppen[gi]); }
    std::vector<int> besitzer(r.flaechen.size(), -1);
    std::vector<bool> mehrdeutig(gruppen.size(), false);
    for (std::size_t gi = 0; gi < gruppen.size(); ++gi) {
        for (const int pi : gruppen[gi].paare) {
            const int f = paare[static_cast<std::size_t>(pi)].flaeche;
            int& b = besitzer[static_cast<std::size_t>(f)];
            if (b < 0 || gAnteil[gi] > gAnteil[static_cast<std::size_t>(b)] + 1e-9) {
                b = static_cast<int>(gi);
            } else if (std::fabs(gAnteil[gi] - gAnteil[static_cast<std::size_t>(b)]) <= 1e-9 && gruppen[gi].modell != gruppen[static_cast<std::size_t>(b)].modell) {
                mehrdeutig[static_cast<std::size_t>(b)] = true;
            }
        }
    }
    const auto ents = entitaetenLesen(r.entitaeten);
    for (std::size_t gi = 0; gi < gruppen.size(); ++gi) {
        Gruppe& g = gruppen[gi];
        std::vector<int> eigene;
        for (const int pi : g.paare) {
            if (besitzer[static_cast<std::size_t>(paare[static_cast<std::size_t>(pi)].flaeche)] == static_cast<int>(gi)) { eigene.push_back(pi); }
        }
        if (eigene.empty() || eigene.size() != g.paare.size()) { continue; }
        const ModellDatei& m = modelle[static_cast<std::size_t>(g.modell)];
        // Lage aus allen Ecken der Gruppe
        std::vector<std::array<double, 3>> mp;
        std::vector<std::array<double, 3>> vp;
        for (const int pi : eigene) {
            const Paarung& p = paare[static_cast<std::size_t>(pi)];
            const ModellNetz& n = m.netze[static_cast<std::size_t>(p.netz)];
            const Raw::Flaeche& fl = r.flaechen[static_cast<std::size_t>(p.flaeche)];
            for (std::size_t i = 0; i < n.punkte.size(); ++i) {
                mp.push_back({n.punkte[i][0], n.punkte[i][1], n.punkte[i][2]});
                const auto& x = r.xyz[static_cast<std::size_t>(fl.fv) + i];
                vp.push_back({x[0], x[1], x[2]});
            }
        }
        double A[3][3];
        double t[3];
        const Paarung& erster = paare[static_cast<std::size_t>(eigene.front())];
        bool eben = false;
        if (!affinLoesen(mp, vp, det3(erster.A) >= 0 ? 1 : -1, A, t, &eben)) {
            std::memcpy(A, erster.A, sizeof(A));
            std::memcpy(t, erster.t, sizeof(t));
        }
        ModellTreffer tr;
        tr.pfad = m.pfad;
        tr.bspModell = bspModellVon[static_cast<std::size_t>(erster.flaeche)];
        tr.lage = lageAusMatrix(A, t);
        double summe = 0;
        double maxF = 0;
        for (std::size_t i = 0; i < mp.size(); ++i) {
            double d2 = 0;
            for (int k = 0; k < 3; ++k) {
                const double x = A[k][0] * mp[i][0] + A[k][1] * mp[i][1] + A[k][2] * mp[i][2] + t[k] - vp[i][k];
                d2 += x * x;
            }
            summe += std::sqrt(d2);
            maxF = std::max(maxF, std::sqrt(d2));
        }
        tr.rest = mp.empty() ? 0.0 : summe / static_cast<double>(mp.size());
        tr.maxFehler = maxF;
        tr.netzeGefunden = static_cast<int>(eigene.size());
        tr.netzeGesamt = static_cast<int>(m.netze.size());
        tr.mehrdeutig = mehrdeutig[gi];
        for (const int pi : eigene) {
            const Paarung& p = paare[static_cast<std::size_t>(pi)];
            tr.flaechen.push_back(p.flaeche);
            getroffen[static_cast<std::size_t>(p.flaeche)] = true;
            const std::string von = ohneEndung(m.netze[static_cast<std::size_t>(p.netz)].shader);
            const int sh = r.flaechen[static_cast<std::size_t>(p.flaeche)].shader;
            const std::string zu = sh >= 0 && sh < static_cast<int>(r.shader.size()) ? r.shader[static_cast<std::size_t>(sh)] : std::string();
            if (klein(von) != klein(zu) && !zu.empty()) {
                const auto paar = std::make_pair(von, zu);
                if (std::find(tr.remaps.begin(), tr.remaps.end(), paar) == tr.remaps.end()) { tr.remaps.push_back(paar); }
            }
        }
        std::sort(tr.flaechen.begin(), tr.flaechen.end());
        // Untermodell: die Entitaet mit "model" "*N" - ihr Ursprung kommt dazu (InsertModel
        // rechnet origin - parent_origin), "target" = ihr targetname
        if (tr.bspModell > 0) {
            const std::string name = "*" + std::to_string(tr.bspModell);
            for (const auto& e : ents) {
                if (wertVon(e, "model") != name) { continue; }
                double po[3] = {0, 0, 0};
                const std::string ou = wertVon(e, "origin");
                if (!ou.empty()) { (void)std::sscanf(ou.c_str(), "%lf %lf %lf", &po[0], &po[1], &po[2]); }
                for (int k = 0; k < 3; ++k) { tr.lage.origin[k] += po[k]; }
                tr.ziel = wertVon(e, "targetname");
                break;
            }
        }
        treffer.push_back(std::move(tr));
    }
    erg.treffer = std::move(treffer);
    // --- Zweiter Durchgang: forcemeta (spawnflags 4 oder q3map_forceMeta) - die Dreiecke
    // sind mit anderen zu Meta-Flaechen verschmolzen (MergeMetaTriangles), die Reihenfolge
    // ist weg, die Ecken selbst bleiben (T-Junctions fuegen nur Ecken auf Kanten ein).
    // Gesucht wird eine affine Abbildung, die zwei benachbarte, nicht ebene Dreiecke des
    // Modells auf zwei benachbarte Dreiecke der .bsp legt; gilt sie fuer (fast) alle Ecken
    // des Modells, ist es ein Treffer.
    struct Dr {
        int f;
        int v[3];   // Ecken (global)
    };
    std::vector<Dr> alle;
    for (std::size_t f = 0; f < r.flaechen.size(); ++f) {
        const Raw::Flaeche& fl = r.flaechen[f];
        if (getroffen[f] || fl.typ == 2 || fl.typ == 4 || fl.ni % 3 != 0 || fl.fv < 0 || fl.fi < 0 || fl.fi + fl.ni > static_cast<int>(r.idx.size())) {
            continue;
        }
        for (int i = 0; i < fl.ni; i += 3) {
            Dr d{static_cast<int>(f), {}};
            bool ok = true;
            for (int k = 0; k < 3; ++k) {
                d.v[k] = fl.fv + r.idx[static_cast<std::size_t>(fl.fi + i + k)];
                ok = ok && d.v[k] >= 0 && d.v[k] < static_cast<int>(r.xyz.size());
            }
            if (ok) { alle.push_back(d); }
        }
    }
    std::vector<bool> beansprucht(alle.size(), false);
    std::unordered_map<std::string, std::vector<int>> shaderIndizes;   // Name (klein) -> Shader-Nummern
    for (std::size_t i = 0; i < r.shader.size(); ++i) { shaderIndizes[klein(r.shader[i])].push_back(static_cast<int>(i)); }
    std::unordered_map<int, std::vector<int>> dreieckeJeShader;
    for (std::size_t i = 0; i < alle.size(); ++i) { dreieckeJeShader[r.flaechen[static_cast<std::size_t>(alle[i].f)].shader].push_back(static_cast<int>(i)); }
    const auto schluessel = [](double x, double y, double z, double h) {
        const auto q = [h](double v) { return static_cast<std::int64_t>(std::floor(v / h)); };
        return (static_cast<std::uint64_t>(q(x) & 0x1FFFFF) << 42) ^ (static_cast<std::uint64_t>(q(y) & 0x1FFFFF) << 21) ^ static_cast<std::uint64_t>(q(z) & 0x1FFFFF);
    };
    const auto vol = [](const double* a, const double* b, const double* c, const double* d) {
        const double u[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
        const double v[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
        const double w[3] = {d[0] - a[0], d[1] - a[1], d[2] - a[2]};
        const double dd = u[0] * (v[1] * w[2] - v[2] * w[1]) - u[1] * (v[0] * w[2] - v[2] * w[0]) + u[2] * (v[0] * w[1] - v[1] * w[0]);
        const double l = std::sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]) * std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]) *
                         std::sqrt(w[0] * w[0] + w[1] * w[1] + w[2] * w[2]);
        return l > 0 ? std::fabs(dd) / l : 0.0;
    };
    for (std::size_t m = 0; m < modelle.size(); ++m) {
        const ModellDatei& md = modelle[m];
        if (!md.geladen || md.netze.empty()) { continue; }
        if (o.fortschritt && !o.fortschritt(0.5 + 0.4 * static_cast<double>(m) / static_cast<double>(modelle.size()))) {
            erg.abgebrochen = true;
            return erg;
        }
        // Die Dreiecke der .bsp mit den Shadern des Modells
        std::vector<int> pool;
        std::vector<int> netzMitPool;
        for (std::size_t k = 0; k < md.netze.size(); ++k) {
            const auto it = shaderIndizes.find(klein(ohneEndung(md.netze[k].shader)));
            if (it == shaderIndizes.end()) { continue; }
            bool hat = false;
            for (const int si : it->second) {
                const auto jt = dreieckeJeShader.find(si);
                if (jt == dreieckeJeShader.end()) { continue; }
                for (const int di : jt->second) {
                    if (!beansprucht[static_cast<std::size_t>(di)]) {
                        pool.push_back(di);
                        hat = true;
                    }
                }
            }
            if (hat) { netzMitPool.push_back(static_cast<int>(k)); }
        }
        if (pool.size() < 2 || netzMitPool.empty()) { continue; }
        std::sort(pool.begin(), pool.end());
        pool.erase(std::unique(pool.begin(), pool.end()), pool.end());
        // Ecken des Modells (alle Netze mit Dreiecken in der .bsp) mit Texturkoordinaten: sie
        // unterscheiden spiegelgleiche Lagen (ein symmetrisches Modell passt sonst auch gespiegelt)
        std::vector<std::array<double, 3>> mEcken;
        std::vector<std::array<float, 2>> mSt;
        for (const int k : netzMitPool) {
            const ModellNetz& nz = md.netze[static_cast<std::size_t>(k)];
            for (std::size_t i = 0; i < nz.punkte.size(); ++i) {
                mEcken.push_back({nz.punkte[i][0], nz.punkte[i][1], nz.punkte[i][2]});
                mSt.push_back(i < nz.st.size() ? nz.st[i] : std::array<float, 2>{0, 0});
            }
        }
        const bool mitSt = !md.netze.empty() && !md.netze.front().st.empty();
        if (mEcken.size() < 4) { continue; }
        // Bezugsviereck im Modell: zwei Dreiecke mit gemeinsamer Kante, moeglichst nicht eben
        struct Viereck {
            double a[3], b[3], c[3], d[3];
        };
        std::vector<Viereck> bezug;
        {
            std::vector<std::tuple<double, int, int, int, int>> kand;   // Guete, a, b, c, d (Indizes im groessten Netz)
            int groesstes = netzMitPool.front();
            for (const int k : netzMitPool) {
                if (md.netze[static_cast<std::size_t>(k)].dreiecke.size() > md.netze[static_cast<std::size_t>(groesstes)].dreiecke.size()) { groesstes = k; }
            }
            const ModellNetz& n = md.netze[static_cast<std::size_t>(groesstes)];
            // Nachbarschaft ueber die Lage, nicht die Nummer: assimp teilt Ecken mit
            // verschiedener Normale oder Texturkoordinate (obj-Netze teilen fast keine)
            std::vector<std::uint32_t> kanon(n.punkte.size());
            {
                std::map<std::array<std::int64_t, 3>, std::uint32_t> erste;
                for (std::size_t i = 0; i < n.punkte.size(); ++i) {
                    const std::array<std::int64_t, 3> q{std::llround(n.punkte[i][0] * 4096.0), std::llround(n.punkte[i][1] * 4096.0),
                                                        std::llround(n.punkte[i][2] * 4096.0)};
                    kanon[i] = erste.emplace(q, static_cast<std::uint32_t>(i)).first->second;
                }
            }
            std::map<std::pair<std::uint32_t, std::uint32_t>, std::vector<int>> kanten;
            for (std::size_t t = 0; t < n.dreiecke.size(); ++t) {
                for (int e = 0; e < 3; ++e) {
                    const std::uint32_t x = kanon[n.dreiecke[t][e]];
                    const std::uint32_t y = kanon[n.dreiecke[t][(e + 1) % 3]];
                    kanten[{std::min(x, y), std::max(x, y)}].push_back(static_cast<int>(t));
                }
            }
            const auto P = [&n](std::uint32_t i, double* out) {
                for (int k = 0; k < 3; ++k) { out[k] = n.punkte[i][k]; }
            };
            for (const auto& [kante, ts] : kanten) {
                if (ts.size() != 2) { continue; }
                const auto& t1 = n.dreiecke[static_cast<std::size_t>(ts[0])];
                const auto& t2 = n.dreiecke[static_cast<std::size_t>(ts[1])];
                std::uint32_t a = 0;
                std::uint32_t d = 0;
                for (int k = 0; k < 3; ++k) {
                    if (kanon[t1[k]] != kante.first && kanon[t1[k]] != kante.second) { a = t1[k]; }
                    if (kanon[t2[k]] != kante.first && kanon[t2[k]] != kante.second) { d = t2[k]; }
                }
                double pa[3];
                double pb[3];
                double pc[3];
                double pd[3];
                P(a, pa);
                P(kante.first, pb);
                P(kante.second, pc);
                P(d, pd);
                const double v = vol(pa, pb, pc, pd);
                if (v < 0.15) { continue; }
                kand.emplace_back(v, static_cast<int>(a), static_cast<int>(kante.first), static_cast<int>(kante.second), static_cast<int>(d));
            }
            std::sort(kand.begin(), kand.end(), [](const auto& x, const auto& y) { return std::get<0>(x) > std::get<0>(y); });
            for (std::size_t i = 0; i < kand.size() && bezug.size() < 12; ++i) {
                Viereck q{};
                P(static_cast<std::uint32_t>(std::get<1>(kand[i])), q.a);
                P(static_cast<std::uint32_t>(std::get<2>(kand[i])), q.b);
                P(static_cast<std::uint32_t>(std::get<3>(kand[i])), q.c);
                P(static_cast<std::uint32_t>(std::get<4>(kand[i])), q.d);
                bezug.push_back(q);
            }
        }
        if (bezug.empty()) { continue; }
        // Kandidaten eines Durchlaufs gelten fuer alle Vorkommen des Modells: gesucht wird
        // einmal, nach jedem Treffer werden die uebrigen gegen den Rest neu bewertet
        struct Kandidat {
            double A[3][3];
            double t[3];
            bool gut;
        };
        std::vector<Kandidat> vorrat;
        bool gescannt = false;
        for (int instanz = 0; instanz < 64; ++instanz) {
            // Ecken des Pools im Raster (Suche nach naechster Ecke)
            std::unordered_map<std::uint64_t, std::vector<int>> raster;
            constexpr double kH = 1.0;
            std::vector<int> poolEcken;
            for (const int di : pool) {
                if (beansprucht[static_cast<std::size_t>(di)]) { continue; }
                for (const int v : alle[static_cast<std::size_t>(di)].v) { poolEcken.push_back(v); }
            }
            std::sort(poolEcken.begin(), poolEcken.end());
            poolEcken.erase(std::unique(poolEcken.begin(), poolEcken.end()), poolEcken.end());
            for (const int v : poolEcken) {
                const auto& p = r.xyz[static_cast<std::size_t>(v)];
                raster[schluessel(p[0], p[1], p[2], kH)].push_back(v);
            }
            const auto naechste = [&](const double* p, double tol, const std::array<float, 2>* st = nullptr) {
                const std::int64_t cx = static_cast<std::int64_t>(std::floor(p[0] / kH));
                const std::int64_t cy = static_cast<std::int64_t>(std::floor(p[1] / kH));
                const std::int64_t cz = static_cast<std::int64_t>(std::floor(p[2] / kH));
                int best = -1;
                double bd = tol * tol;
                for (std::int64_t dx = -1; dx <= 1; ++dx) {
                    for (std::int64_t dy = -1; dy <= 1; ++dy) {
                        for (std::int64_t dz = -1; dz <= 1; ++dz) {
                            const std::uint64_t key = (static_cast<std::uint64_t>((cx + dx) & 0x1FFFFF) << 42) ^ (static_cast<std::uint64_t>((cy + dy) & 0x1FFFFF) << 21) ^
                                                      static_cast<std::uint64_t>((cz + dz) & 0x1FFFFF);
                            const auto it = raster.find(key);
                            if (it == raster.end()) { continue; }
                            for (const int v : it->second) {
                                const auto& q = r.xyz[static_cast<std::size_t>(v)];
                                const double d2 = (q[0] - p[0]) * (q[0] - p[0]) + (q[1] - p[1]) * (q[1] - p[1]) + (q[2] - p[2]) * (q[2] - p[2]);
                                if (st != nullptr && (std::fabs(r.st[static_cast<std::size_t>(v)][0] - (*st)[0]) > 1e-3F ||
                                                      std::fabs(r.st[static_cast<std::size_t>(v)][1] - (*st)[1]) > 1e-3F)) {
                                    continue;
                                }
                                if (d2 <= bd) {
                                    bd = d2;
                                    best = v;
                                }
                            }
                        }
                    }
                }
                return best;
            };
            // Kanten des Pools ueber die Lage der Ecken (verschiedene Flaechen haben eigene Ecken)
            const auto eckSchluessel = [&](int v) {
                const auto& p = r.xyz[static_cast<std::size_t>(v)];
                return schluessel(p[0], p[1], p[2], 1.0 / 32.0);
            };
            std::unordered_map<std::uint64_t, std::vector<std::pair<int, int>>> kanten;   // Kante -> (Dreieck, gegenueberliegende Ecke)
            for (const int di : pool) {
                if (gescannt || beansprucht[static_cast<std::size_t>(di)]) { continue; }
                const Dr& d = alle[static_cast<std::size_t>(di)];
                for (int e = 0; e < 3; ++e) {
                    const std::uint64_t x = eckSchluessel(d.v[e]);
                    const std::uint64_t y = eckSchluessel(d.v[(e + 1) % 3]);
                    const std::uint64_t k = std::min(x, y) * 1000003ULL ^ std::max(x, y);
                    kanten[k].emplace_back(di, (e + 2) % 3);
                }
            }
            std::vector<Kandidat> kandidaten = vorrat;
            bool gefunden = false;   // Notbremse: zu viele Kandidaten
            double A[3][3];
            double t[3];
            const double probeTol = 0.05;
            std::vector<std::size_t> proben;
            for (std::size_t i = 0; i < 12; ++i) { proben.push_back((i * 7919 + 13) % mEcken.size()); }
            for (const auto& [k, liste] : kanten) {
                if (gefunden) { break; }
                if (liste.size() < 2 || liste.size() > 4) { continue; }
                for (std::size_t i1 = 0; i1 < liste.size() && !gefunden; ++i1) {
                    for (std::size_t i2 = 0; i2 < liste.size() && !gefunden; ++i2) {
                        if (i1 == i2) { continue; }
                        const Dr& d1 = alle[static_cast<std::size_t>(liste[i1].first)];
                        const Dr& d2 = alle[static_cast<std::size_t>(liste[i2].first)];
                        const int o1 = d1.v[liste[i1].second];
                        const int o2 = d2.v[liste[i2].second];
                        const int e0 = d1.v[(liste[i1].second + 1) % 3];
                        const int e1 = d1.v[(liste[i1].second + 2) % 3];
                        double po1[3];
                        double pe0[3];
                        double pe1[3];
                        double po2[3];
                        for (int c = 0; c < 3; ++c) {
                            po1[c] = r.xyz[static_cast<std::size_t>(o1)][c];
                            pe0[c] = r.xyz[static_cast<std::size_t>(e0)][c];
                            pe1[c] = r.xyz[static_cast<std::size_t>(e1)][c];
                            po2[c] = r.xyz[static_cast<std::size_t>(o2)][c];
                        }
                        if (vol(po1, pe0, pe1, po2) < 0.05) { continue; }
                        for (const Viereck& q : bezug) {
                            if (gefunden) { break; }
                            for (int richtung = 0; richtung < 2 && !gefunden; ++richtung) {
                                const double* w0 = po1;
                                const double* w1 = richtung == 0 ? pe0 : pe1;
                                const double* w2 = richtung == 0 ? pe1 : pe0;
                                const double* w3 = po2;
                                // A [b-a, c-a, d-a] = [w1-w0, w2-w0, w3-w0]
                                double M[3][3];
                                double W[3][3];
                                for (int rr = 0; rr < 3; ++rr) {
                                    M[rr][0] = q.b[rr] - q.a[rr];
                                    M[rr][1] = q.c[rr] - q.a[rr];
                                    M[rr][2] = q.d[rr] - q.a[rr];
                                    W[rr][0] = w1[rr] - w0[rr];
                                    W[rr][1] = w2[rr] - w0[rr];
                                    W[rr][2] = w3[rr] - w0[rr];
                                }
                                const double dm = det3(M);
                                if (std::fabs(dm) < 1e-12) { continue; }
                                double Mi[3][3];
                                Mi[0][0] = (M[1][1] * M[2][2] - M[1][2] * M[2][1]) / dm;
                                Mi[0][1] = (M[0][2] * M[2][1] - M[0][1] * M[2][2]) / dm;
                                Mi[0][2] = (M[0][1] * M[1][2] - M[0][2] * M[1][1]) / dm;
                                Mi[1][0] = (M[1][2] * M[2][0] - M[1][0] * M[2][2]) / dm;
                                Mi[1][1] = (M[0][0] * M[2][2] - M[0][2] * M[2][0]) / dm;
                                Mi[1][2] = (M[0][2] * M[1][0] - M[0][0] * M[1][2]) / dm;
                                Mi[2][0] = (M[1][0] * M[2][1] - M[1][1] * M[2][0]) / dm;
                                Mi[2][1] = (M[0][1] * M[2][0] - M[0][0] * M[2][1]) / dm;
                                Mi[2][2] = (M[0][0] * M[1][1] - M[0][1] * M[1][0]) / dm;
                                for (int rr = 0; rr < 3; ++rr) {
                                    for (int cc = 0; cc < 3; ++cc) { A[rr][cc] = W[rr][0] * Mi[0][cc] + W[rr][1] * Mi[1][cc] + W[rr][2] * Mi[2][cc]; }
                                    t[rr] = w0[rr] - (A[rr][0] * q.a[0] + A[rr][1] * q.a[1] + A[rr][2] * q.a[2]);
                                }
                                const auto abbild = [&](const std::array<double, 3>& p, double* out) {
                                    for (int rr = 0; rr < 3; ++rr) { out[rr] = A[rr][0] * p[0] + A[rr][1] * p[1] + A[rr][2] * p[2] + t[rr]; }
                                };
                                // Schnellpruefung: 12 Proben, ab dem zweiten Fehlgriff verworfen
                                int treffend = 0;
                                int daneben = 0;
                                for (const std::size_t pi : proben) {
                                    double x[3];
                                    abbild(mEcken[pi], x);
                                    if (naechste(x, probeTol + 1e-5 * (std::fabs(x[0]) + std::fabs(x[1]) + std::fabs(x[2])), mitSt ? &mSt[pi] : nullptr) >= 0) {
                                        ++treffend;
                                    } else if (++daneben > 1) {
                                        break;
                                    }
                                }
                                if (treffend < 11) { continue; }
                                bool schonDa = false;   // dieselbe Abbildung aus einem anderen Dreieckspaar
                                for (const Kandidat& kd : kandidaten) {
                                    double dA = 0;
                                    double sA = 0;
                                    double dt = 0;
                                    for (int rr = 0; rr < 3; ++rr) {
                                        dt = std::max(dt, std::fabs(kd.t[rr] - t[rr]));
                                        for (int cc = 0; cc < 3; ++cc) {
                                            dA = std::max(dA, std::fabs(kd.A[rr][cc] - A[rr][cc]));
                                            sA = std::max(sA, std::fabs(A[rr][cc]));
                                        }
                                    }
                                    if (dA <= 1e-4 * std::max(1.0, sA) && dt <= 0.05) {
                                        schonDa = true;
                                        break;
                                    }
                                }
                                if (schonDa) { continue; }
                                std::size_t alleTreffend = 0;
                                for (std::size_t pi = 0; pi < mEcken.size(); ++pi) {
                                    double x[3];
                                    abbild(mEcken[pi], x);
                                    alleTreffend +=
                                        naechste(x, probeTol + 1e-5 * (std::fabs(x[0]) + std::fabs(x[1]) + std::fabs(x[2])), mitSt ? &mSt[pi] : nullptr) >= 0 ? 1U : 0U;
                                }
                                Kandidat kd{};
                                std::memcpy(kd.A, A, sizeof(A));
                                std::memcpy(kd.t, t, sizeof(t));
                                kd.gut = static_cast<double>(alleTreffend) >= 0.9 * static_cast<double>(mEcken.size());
                                kandidaten.push_back(kd);
                                if (kandidaten.size() > 256) { gefunden = true; }   // genug (Notbremse)
                            }
                        }
                    }
                }
            }
            if (!gescannt) {
                vorrat = kandidaten;
                gescannt = true;
            }
            // Unter den guten Abbildungen: die, deren Ecken sich mit der besten decken, sind
            // gleichwertig (ein symmetrisches Modell, z. B. 4-fach drehsymmetrisch, liegt mit
            // denselben Ecken und Texturkoordinaten auch gedreht) - genommen wird die
            // einfachste (kleinste Winkel), wie ein Mapper sie setzen wuerde.
            {
                std::vector<std::vector<int>> treffMengen;
                std::vector<std::size_t> gute;
                for (std::size_t ki = 0; ki < kandidaten.size(); ++ki) {
                    if (!kandidaten[ki].gut) { continue; }
                    std::vector<int> menge;
                    std::size_t getroffenEcken = 0;
                    for (std::size_t pi = 0; pi < mEcken.size(); ++pi) {
                        double x[3];
                        for (int rr = 0; rr < 3; ++rr) {
                            x[rr] = kandidaten[ki].A[rr][0] * mEcken[pi][0] + kandidaten[ki].A[rr][1] * mEcken[pi][1] + kandidaten[ki].A[rr][2] * mEcken[pi][2] +
                                    kandidaten[ki].t[rr];
                        }
                        const int v = naechste(x, probeTol + 1e-5 * (std::fabs(x[0]) + std::fabs(x[1]) + std::fabs(x[2])), mitSt ? &mSt[pi] : nullptr);
                        if (v >= 0) {
                            menge.push_back(v);
                            ++getroffenEcken;
                        }
                    }
                    // gegen den Rest des Pools: nach einem Treffer passen die Kandidaten seiner
                    // Stelle nicht mehr
                    if (static_cast<double>(getroffenEcken) < 0.9 * static_cast<double>(mEcken.size())) {
                        vorrat[ki].gut = false;
                        continue;
                    }
                    std::sort(menge.begin(), menge.end());
                    menge.erase(std::unique(menge.begin(), menge.end()), menge.end());
                    treffMengen.push_back(std::move(menge));
                    gute.push_back(ki);
                }
                if (gute.empty()) { break; }
                std::size_t erste = 0;
                for (std::size_t i = 1; i < gute.size(); ++i) {
                    if (treffMengen[i].size() > treffMengen[erste].size()) { erste = i; }
                }
                const auto einfach = [&](std::size_t ki) {
                    const ModellLage l = lageAusMatrix(kandidaten[ki].A, kandidaten[ki].t);
                    double s = 0;
                    for (const double w : l.winkel) {
                        double x = std::fmod(std::fabs(w), 360.0);
                        s += std::min(x, 360.0 - x);
                    }
                    for (const double sk : l.skala) { s += sk < 0 ? 360.0 : 0.0; }
                    return s;
                };
                std::size_t wahl = gute[erste];
                for (std::size_t i = 0; i < gute.size(); ++i) {
                    std::vector<int> schnitt;
                    std::set_intersection(treffMengen[i].begin(), treffMengen[i].end(), treffMengen[erste].begin(), treffMengen[erste].end(),
                                          std::back_inserter(schnitt));
                    const std::size_t vereint = treffMengen[i].size() + treffMengen[erste].size() - schnitt.size();
                    if (vereint == 0 || static_cast<double>(schnitt.size()) < 0.9 * static_cast<double>(vereint)) { continue; }
                    if (einfach(gute[i]) < einfach(wahl) - 1e-6) { wahl = gute[i]; }
                }
                std::memcpy(A, kandidaten[wahl].A, sizeof(A));
                std::memcpy(t, kandidaten[wahl].t, sizeof(t));
                vorrat[wahl].gut = false;
            }
            // Ausgleich ueber alle zugeordneten Ecken, dann die Dreiecke des Pools beanspruchen
            std::vector<std::array<double, 3>> mp;
            std::vector<std::array<double, 3>> vp;
            std::unordered_map<std::uint64_t, int> abgebildet;   // Ecken der .bsp, die das Modell trifft
            for (const auto& p : mEcken) {
                double x[3];
                for (int rr = 0; rr < 3; ++rr) { x[rr] = A[rr][0] * p[0] + A[rr][1] * p[1] + A[rr][2] * p[2] + t[rr]; }
                const int v = naechste(x, probeTol + 1e-5 * (std::fabs(x[0]) + std::fabs(x[1]) + std::fabs(x[2])));
                if (v < 0) { continue; }
                mp.push_back(p);
                const auto& q = r.xyz[static_cast<std::size_t>(v)];
                vp.push_back({q[0], q[1], q[2]});
                abgebildet[eckSchluessel(v)] = v;
            }
            double A2[3][3];
            double t2[3];
            if (affinLoesen(mp, vp, det3(A) >= 0 ? 1 : -1, A2, t2, nullptr)) {
                std::memcpy(A, A2, sizeof(A));
                std::memcpy(t, t2, sizeof(t));
            }
            ModellTreffer tr;
            tr.pfad = md.pfad;
            tr.lage = lageAusMatrix(A, t);
            tr.forceMeta = true;
            tr.netzeGefunden = static_cast<int>(netzMitPool.size());
            tr.netzeGesamt = static_cast<int>(md.netze.size());
            double summe = 0;
            for (std::size_t i = 0; i < mp.size(); ++i) {
                double d2 = 0;
                for (int k2 = 0; k2 < 3; ++k2) {
                    const double x = A[k2][0] * mp[i][0] + A[k2][1] * mp[i][1] + A[k2][2] * mp[i][2] + t[k2] - vp[i][k2];
                    d2 += x * x;
                }
                summe += std::sqrt(d2);
                tr.maxFehler = std::max(tr.maxFehler, std::sqrt(d2));
            }
            tr.rest = mp.empty() ? 0.0 : summe / static_cast<double>(mp.size());
            std::set<int> fl;
            for (const int di : pool) {
                const Dr& d = alle[static_cast<std::size_t>(di)];
                // ein Dreieck gehoert dem Modell, wenn seine Ecken auf Kanten/Ecken des Modells
                // liegen; T-Junction-Ecken liegen auf Kanten - es genuegen zwei Modell-Ecken
                int auf = 0;
                for (const int v : d.v) { auf += abgebildet.count(eckSchluessel(v)) != 0 ? 1 : 0; }
                if (auf >= 2) {
                    beansprucht[static_cast<std::size_t>(di)] = true;
                    fl.insert(d.f);
                }
            }
            // Dreiecke, die T-Junction-Ecken zerteilt haben, tragen manchmal nur eine Modell-Ecke:
            // sie gehoeren dazu, wenn sie eine Kante mit einem schon beanspruchten teilen
            for (int runde = 0; runde < 8; ++runde) {
                std::unordered_set<std::uint64_t> kantenDrin;
                for (const int di : pool) {
                    if (!beansprucht[static_cast<std::size_t>(di)]) { continue; }
                    const Dr& d = alle[static_cast<std::size_t>(di)];
                    for (int e = 0; e < 3; ++e) {
                        const std::uint64_t x = eckSchluessel(d.v[e]);
                        const std::uint64_t y = eckSchluessel(d.v[(e + 1) % 3]);
                        kantenDrin.insert(std::min(x, y) * 1000003ULL ^ std::max(x, y));
                    }
                }
                int neu = 0;
                for (const int di : pool) {
                    if (beansprucht[static_cast<std::size_t>(di)]) { continue; }
                    const Dr& d = alle[static_cast<std::size_t>(di)];
                    int auf = 0;
                    bool kante = false;
                    for (int e = 0; e < 3; ++e) {
                        auf += abgebildet.count(eckSchluessel(d.v[e])) != 0 ? 1 : 0;
                        const std::uint64_t x = eckSchluessel(d.v[e]);
                        const std::uint64_t y = eckSchluessel(d.v[(e + 1) % 3]);
                        kante = kante || kantenDrin.count(std::min(x, y) * 1000003ULL ^ std::max(x, y)) != 0;
                    }
                    if (auf >= 1 && kante) {
                        beansprucht[static_cast<std::size_t>(di)] = true;
                        fl.insert(d.f);
                        ++neu;
                    }
                }
                if (neu == 0) { break; }
            }
            tr.flaechen.assign(fl.begin(), fl.end());
            if (tr.flaechen.empty()) { break; }
            tr.bspModell = bspModellVon[static_cast<std::size_t>(tr.flaechen.front())];
            erg.treffer.push_back(std::move(tr));
        }
    }
    // --- Nicht erkannt: Dreiecke (ausser Patches), die weder einem Treffer gehoeren noch
    // auf einer Pinselseite mit demselben Shader liegen - eingebackene Modelle, deren Datei
    // fehlt, oder Modellflaechen, die nicht sicher passten
    // Die Ecken einer Zeichenflaeche liegen auf 1/8 gerastert (q3map2 SnapDrawVert), die
    // Normale eines kurzen, schraegen Dreiecks weicht darum von der Ebene der Pinselseite ab:
    // verglichen wird nicht die Normale, sondern der Abstand der drei Ecken (<= 0,2) zu einer
    // Pinselebene mit aehnlicher Normale (Skalarprodukt >= 0,98) und demselben Shader-Namen.
    // derselbe Shader steht mit verschiedenen Flags mehrmals im Lump (EmitShader): je Name eine Nummer
    std::vector<int> kanonShader(r.shader.size());
    {
        std::unordered_map<std::string, int> erste;
        for (std::size_t i = 0; i < r.shader.size(); ++i) { kanonShader[i] = erste.emplace(klein(r.shader[i]), static_cast<int>(i)).first->second; }
    }
    const auto kanon = [&kanonShader](int sh) { return sh >= 0 && sh < static_cast<int>(kanonShader.size()) ? kanonShader[static_cast<std::size_t>(sh)] : sh; };
    const auto eimer = [](int sh, std::int64_t nx, std::int64_t ny, std::int64_t nz) {
        std::uint64_t h = static_cast<std::uint64_t>(sh) * 0x9E3779B97F4A7C15ULL;
        for (const std::int64_t v : {nx, ny, nz}) { h = (h ^ static_cast<std::uint64_t>(v)) * 1099511628211ULL; }
        return h;
    };
    std::unordered_map<std::uint64_t, std::vector<std::array<double, 4>>> ebenen;
    {
        std::set<std::pair<int, int>> schon;   // (Shader, Ebene) einmal
        for (const Raw::Seite& sd : r.seiten) {
            if (sd.ebene < 0 || sd.ebene >= static_cast<int>(r.ebenen.size())) { continue; }
            const int ks = kanon(sd.shader);
            if (!schon.insert({ks, sd.ebene}).second) { continue; }
            const auto& e = r.ebenen[static_cast<std::size_t>(sd.ebene)];
            for (const double sg : {1.0, -1.0}) {
                const std::array<double, 4> pe{sg * e[0], sg * e[1], sg * e[2], sg * e[3]};
                ebenen[eimer(ks, std::llround(pe[0] * 10.0), std::llround(pe[1] * 10.0), std::llround(pe[2] * 10.0))].push_back(pe);
            }
        }
    }
    std::set<int> unerkannteFlaechen;
    for (std::size_t i = 0; i < alle.size(); ++i) {
        if (beansprucht[i]) { continue; }
        const Dr& d = alle[i];
        const auto& a = r.xyz[static_cast<std::size_t>(d.v[0])];
        const auto& b = r.xyz[static_cast<std::size_t>(d.v[1])];
        const auto& c = r.xyz[static_cast<std::size_t>(d.v[2])];
        const double u[3] = {static_cast<double>(b[0]) - a[0], static_cast<double>(b[1]) - a[1], static_cast<double>(b[2]) - a[2]};
        const double w[3] = {static_cast<double>(c[0]) - a[0], static_cast<double>(c[1]) - a[1], static_cast<double>(c[2]) - a[2]};
        double nrm[3] = {u[1] * w[2] - u[2] * w[1], u[2] * w[0] - u[0] * w[2], u[0] * w[1] - u[1] * w[0]};
        const double l = std::sqrt(nrm[0] * nrm[0] + nrm[1] * nrm[1] + nrm[2] * nrm[2]);
        if (l < 1e-6) { continue; }   // entartet
        for (double& x : nrm) { x /= l; }
        const int sh = r.flaechen[static_cast<std::size_t>(d.f)].shader;
        const int ks = kanon(sh);
        bool auf = false;
        const std::int64_t nx = std::llround(nrm[0] * 10.0);
        const std::int64_t ny = std::llround(nrm[1] * 10.0);
        const std::int64_t nz = std::llround(nrm[2] * 10.0);
        for (std::int64_t ax = -1; ax <= 1 && !auf; ++ax) {
            for (std::int64_t ay = -1; ay <= 1 && !auf; ++ay) {
                for (std::int64_t az = -1; az <= 1 && !auf; ++az) {
                    const auto it = ebenen.find(eimer(ks, nx + ax, ny + ay, nz + az));
                    if (it == ebenen.end()) { continue; }
                    for (const auto& e : it->second) {
                        if (e[0] * nrm[0] + e[1] * nrm[1] + e[2] * nrm[2] < 0.95) { continue; }
                        bool alle3 = true;
                        for (const auto* q : {&a, &b, &c}) {
                            alle3 = alle3 && std::fabs(e[0] * (*q)[0] + e[1] * (*q)[1] + e[2] * (*q)[2] - e[3]) <= 0.5;
                        }
                        if (alle3) {
                            auf = true;
                            break;
                        }
                    }
                }
            }
        }
        if (auf) { continue; }
        RueckErgebnis::Dreieck t3;
        t3.flaeche = d.f;
        t3.shader = sh >= 0 && sh < static_cast<int>(r.shader.size()) ? r.shader[static_cast<std::size_t>(sh)] : std::string();
        for (int k = 0; k < 3; ++k) {
            for (int c2 = 0; c2 < 3; ++c2) { t3.p[k][c2] = r.xyz[static_cast<std::size_t>(d.v[k])][c2]; }
            t3.st[k][0] = r.st[static_cast<std::size_t>(d.v[k])][0];
            t3.st[k][1] = r.st[static_cast<std::size_t>(d.v[k])][1];
        }
        erg.unerkannteDreiecke.push_back(t3);
        unerkannteFlaechen.insert(d.f);
    }
    erg.unerkannt.assign(unerkannteFlaechen.begin(), unerkannteFlaechen.end());
    std::sort(erg.treffer.begin(), erg.treffer.end(), [](const ModellTreffer& a, const ModellTreffer& b) { return a.flaechen.front() < b.flaechen.front(); });
    if (o.fortschritt) { (void)o.fortschritt(1.0); }
    return erg;
}

namespace {

struct Poly {
    std::vector<std::array<double, 3>> p;
    std::vector<std::array<double, 2>> st;
};

// Affine Texturabbildung der Ebene aus drei nicht kollinearen Punkten; false wenn entartet
bool stAffin(const Poly& q, const double n[3], double S[4], double T[4]) {
    const std::size_t m = q.p.size();
    for (std::size_t i = 0; i < m; ++i) {
        for (std::size_t j = i + 1; j < m; ++j) {
            for (std::size_t k = j + 1; k < m; ++k) {
                const double u[3] = {q.p[j][0] - q.p[i][0], q.p[j][1] - q.p[i][1], q.p[j][2] - q.p[i][2]};
                const double v[3] = {q.p[k][0] - q.p[i][0], q.p[k][1] - q.p[i][1], q.p[k][2] - q.p[i][2]};
                // 3x3: Zeilen u, v, n -> Koeffizienten so, dass s(p) = s_i + g . (p - p_i), g in der Ebene
                double M[3][3] = {{u[0], u[1], u[2]}, {v[0], v[1], v[2]}, {n[0], n[1], n[2]}};
                const double d = det3(M);
                if (std::fabs(d) < 1e-6) { continue; }
                double I[3][3];
                I[0][0] = (M[1][1] * M[2][2] - M[1][2] * M[2][1]) / d;
                I[0][1] = (M[0][2] * M[2][1] - M[0][1] * M[2][2]) / d;
                I[0][2] = (M[0][1] * M[1][2] - M[0][2] * M[1][1]) / d;
                I[1][0] = (M[1][2] * M[2][0] - M[1][0] * M[2][2]) / d;
                I[1][1] = (M[0][0] * M[2][2] - M[0][2] * M[2][0]) / d;
                I[1][2] = (M[0][2] * M[1][0] - M[0][0] * M[1][2]) / d;
                I[2][0] = (M[1][0] * M[2][1] - M[1][1] * M[2][0]) / d;
                I[2][1] = (M[0][1] * M[2][0] - M[0][0] * M[2][1]) / d;
                I[2][2] = (M[0][0] * M[1][1] - M[0][1] * M[1][0]) / d;
                for (int c = 0; c < 2; ++c) {
                    const double rhs[3] = {q.st[j][c] - q.st[i][c], q.st[k][c] - q.st[i][c], 0.0};
                    double* out = c == 0 ? S : T;
                    for (int a = 0; a < 3; ++a) { out[a] = I[a][0] * rhs[0] + I[a][1] * rhs[1] + I[a][2] * rhs[2]; }
                    out[3] = q.st[i][c] - (out[0] * q.p[i][0] + out[1] * q.p[i][1] + out[2] * q.p[i][2]);
                }
                return true;
            }
        }
    }
    return false;
}

bool konvex(const std::vector<std::array<double, 3>>& p, const double n[3]) {
    const std::size_t m = p.size();
    if (m < 3) { return false; }
    for (std::size_t i = 0; i < m; ++i) {
        const auto& a = p[(i + m - 1) % m];
        const auto& b = p[i];
        const auto& c = p[(i + 1) % m];
        const double u[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
        const double v[3] = {c[0] - b[0], c[1] - b[1], c[2] - b[2]};
        const double x[3] = {u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]};
        const double lu = std::sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
        const double lv = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        if (lu < 1e-6 || lv < 1e-6) { return false; }
        if ((x[0] * n[0] + x[1] * n[1] + x[2] * n[2]) / (lu * lv) < -1e-6) { return false; }
    }
    return true;
}

// Punkte auf einer Geraden zwischen ihren Nachbarn weg
void kollinearWeg(Poly& q, const double n[3]) {
    bool weg = true;
    while (weg && q.p.size() > 3) {
        weg = false;
        const std::size_t m = q.p.size();
        for (std::size_t i = 0; i < m; ++i) {
            const auto& a = q.p[(i + m - 1) % m];
            const auto& b = q.p[i];
            const auto& c = q.p[(i + 1) % m];
            const double u[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
            const double v[3] = {c[0] - b[0], c[1] - b[1], c[2] - b[2]};
            const double x[3] = {u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]};
            const double lu = std::sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
            const double lv = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
            if (lu < 1e-6 || lv < 1e-6 || std::fabs(x[0] * n[0] + x[1] * n[1] + x[2] * n[2]) / (lu * lv) < 1e-6) {
                q.p.erase(q.p.begin() + static_cast<std::ptrdiff_t>(i));
                q.st.erase(q.st.begin() + static_cast<std::ptrdiff_t>(i));
                weg = true;
                break;
            }
        }
    }
}

}  // namespace

std::vector<ModellVieleck> unerkannteVielecke(const std::vector<RueckErgebnisDreieck>& dreiecke) {
    // Gruppen: Flaeche, Shader, Ebene (gerastert)
    std::map<std::tuple<int, std::string, std::int64_t, std::int64_t, std::int64_t, std::int64_t>, std::vector<std::size_t>> gruppen;
    std::vector<std::array<double, 3>> normalen(dreiecke.size());
    for (std::size_t i = 0; i < dreiecke.size(); ++i) {
        const auto& d = dreiecke[i];
        const double u[3] = {d.p[1][0] - d.p[0][0], d.p[1][1] - d.p[0][1], d.p[1][2] - d.p[0][2]};
        const double v[3] = {d.p[2][0] - d.p[0][0], d.p[2][1] - d.p[0][1], d.p[2][2] - d.p[0][2]};
        // q3map2 zeichnet Dreiecke im Uhrzeigersinn (von vorn): sichtbar ist v x u
        double n[3] = {v[1] * u[2] - v[2] * u[1], v[2] * u[0] - v[0] * u[2], v[0] * u[1] - v[1] * u[0]};
        const double l = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        if (l < 1e-9) { continue; }
        for (double& x : n) { x /= l; }
        normalen[i] = {n[0], n[1], n[2]};
        const double dist = n[0] * d.p[0][0] + n[1] * d.p[0][1] + n[2] * d.p[0][2];
        gruppen[{d.flaeche, d.shader, std::llround(n[0] * 1000.0), std::llround(n[1] * 1000.0), std::llround(n[2] * 1000.0), std::llround(dist * 8.0)}].push_back(i);
    }
    std::vector<ModellVieleck> out;
    for (auto& [schl, liste] : gruppen) {
        const auto& n0 = normalen[liste.front()];
        const double n[3] = {n0[0], n0[1], n0[2]};
        std::vector<Poly> polys;
        for (const std::size_t i : liste) {
            const auto& d = dreiecke[i];
            Poly q;
            // gegen den Uhrzeigersinn um n: q3map2s Reihenfolge umgekehrt
            for (const int k : {0, 2, 1}) {
                q.p.push_back({d.p[k][0], d.p[k][1], d.p[k][2]});
                q.st.push_back({d.st[k][0], d.st[k][1]});
            }
            polys.push_back(std::move(q));
        }
        const auto gleich = [](const std::array<double, 3>& a, const std::array<double, 3>& b) {
            return std::fabs(a[0] - b[0]) < 1e-4 && std::fabs(a[1] - b[1]) < 1e-4 && std::fabs(a[2] - b[2]) < 1e-4;
        };
        // gierig zusammenlegen (je Gruppe begrenzt)
        bool weiter = polys.size() > 1 && polys.size() < 2000;
        while (weiter) {
            weiter = false;
            for (std::size_t a = 0; a < polys.size() && !weiter; ++a) {
                for (std::size_t b = a + 1; b < polys.size() && !weiter; ++b) {
                    const Poly& P = polys[a];
                    const Poly& Q = polys[b];
                    if (P.p.size() + Q.p.size() > 34) { continue; }
                    for (std::size_t i = 0; i < P.p.size() && !weiter; ++i) {
                        const std::size_t i2 = (i + 1) % P.p.size();
                        for (std::size_t j = 0; j < Q.p.size() && !weiter; ++j) {
                            const std::size_t j2 = (j + 1) % Q.p.size();
                            if (!gleich(P.p[i], Q.p[j2]) || !gleich(P.p[i2], Q.p[j])) { continue; }
                            // P: ... i, i2 ...; Q: ... j, j2 ... mit P[i]=Q[j2], P[i2]=Q[j]
                            Poly R;
                            for (std::size_t k = 0; k < P.p.size(); ++k) {
                                const std::size_t x = (i2 + k) % P.p.size();
                                R.p.push_back(P.p[x]);
                                R.st.push_back(P.st[x]);
                                if (x == i) { break; }
                            }
                            for (std::size_t k = 1; k + 1 < Q.p.size(); ++k) {
                                const std::size_t x = (j2 + k) % Q.p.size();
                                R.p.push_back(Q.p[x]);
                                R.st.push_back(Q.st[x]);
                            }
                            kollinearWeg(R, n);
                            if (!konvex(R.p, n)) { continue; }
                            double S[4];
                            double T[4];
                            if (!stAffin(R, n, S, T)) { continue; }
                            bool affin = true;
                            for (std::size_t k = 0; k < R.p.size() && affin; ++k) {
                                const double s = S[0] * R.p[k][0] + S[1] * R.p[k][1] + S[2] * R.p[k][2] + S[3];
                                const double t = T[0] * R.p[k][0] + T[1] * R.p[k][1] + T[2] * R.p[k][2] + T[3];
                                affin = std::fabs(s - R.st[k][0]) < 1e-3 && std::fabs(t - R.st[k][1]) < 1e-3;
                            }
                            if (!affin) { continue; }
                            polys[a] = std::move(R);
                            polys.erase(polys.begin() + static_cast<std::ptrdiff_t>(b));
                            weiter = true;
                        }
                    }
                }
            }
        }
        for (Poly& q : polys) {
            ModellVieleck v;
            v.punkte = q.p;
            for (int k = 0; k < 3; ++k) { v.normale[k] = n[k]; }
            v.shader = std::get<1>(schl);
            if (!stAffin(q, n, v.stS, v.stT)) { continue; }
            out.push_back(std::move(v));
        }
    }
    return out;
}

std::shared_ptr<Pinsel> vieleckPinsel(const ModellVieleck& v, double dicke, const std::string& rueckShader, const Kontext& k) {
    const std::size_t m = v.punkte.size();
    if (m < 3) { return nullptr; }
    const Vektor3d n(v.normale[0], v.normale[1], v.normale[2]);
    const auto P = [&v](std::size_t i) { return Vektor3d(v.punkte[i][0], v.punkte[i][1], v.punkte[i][2]); };
    auto pinsel = std::make_shared<Pinsel>();
    // Eine Flaeche mit der Aussennormale N durch Q: p0 = Q, p1 = Q + U, p2 = Q + U x N
    // (Brush_ConstructCuboid: aussen = (p2 - p0) x (p1 - p0)).
    const auto ebene = [&](const Vektor3d& Q, Vektor3d U, const Vektor3d& N, const std::string& shader, bool vorn) {
        const double lu = std::sqrt(U.x() * U.x() + U.y() * U.y() + U.z() * U.z());
        if (lu < 1e-9) { return false; }
        U = Vektor3d(U.x() / lu, U.y() / lu, U.z() / lu);
        const Vektor3d W(U.y() * N.z() - U.z() * N.y(), U.z() * N.x() - U.x() * N.z(), U.x() * N.y() - U.y() * N.x());
        const EbenenPunkte p{Q, Vektor3d(Q.x() + U.x() * 64.0, Q.y() + U.y() * 64.0, Q.z() + U.z() * 64.0),
                             Vektor3d(Q.x() + W.x() * 64.0, Q.y() + W.y() * 64.0, Q.z() + W.z() * 64.0)};
        TexturProjektion proj;
        texdefStandard(k.art(), proj);
        if (vorn) {
            Vektor3d st[3];
            for (int i = 0; i < 3; ++i) {
                st[i] = Vektor3d(v.stS[0] * p[i].x() + v.stS[1] * p[i].y() + v.stS[2] * p[i].z() + v.stS[3],
                                 v.stT[0] * p[i].x() + v.stT[1] * p[i].y() + v.stT[2] * p[i].z() + v.stT[3], 0.0);
            }
            const auto [w, h] = k.groesse(shader);
            texdefAusSt(k.art(), proj, p, st, w, h);
        }
        return pinsel->ebeneHinzufuegen(p[0], p[1], p[2], shader, proj, k) != nullptr;
    };
    bool ok = ebene(P(0), Vektor3d(P(1).x() - P(0).x(), P(1).y() - P(0).y(), P(1).z() - P(0).z()), n, v.shader, true);
    const Vektor3d Pb(P(0).x() - n.x() * dicke, P(0).y() - n.y() * dicke, P(0).z() - n.z() * dicke);
    ok = ok && ebene(Pb, Vektor3d(P(1).x() - P(0).x(), P(1).y() - P(0).y(), P(1).z() - P(0).z()), Vektor3d(-n.x(), -n.y(), -n.z()), rueckShader, false);
    for (std::size_t i = 0; ok && i < m; ++i) {
        const Vektor3d a = P(i);
        const Vektor3d b = P((i + 1) % m);
        const Vektor3d e(b.x() - a.x(), b.y() - a.y(), b.z() - a.z());
        Vektor3d aussen(e.y() * n.z() - e.z() * n.y(), e.z() * n.x() - e.x() * n.z(), e.x() * n.y() - e.y() * n.x());
        const double la = std::sqrt(aussen.x() * aussen.x() + aussen.y() * aussen.y() + aussen.z() * aussen.z());
        if (la < 1e-9) { continue; }
        aussen = Vektor3d(aussen.x() / la, aussen.y() / la, aussen.z() / la);
        ok = ebene(a, e, aussen, rueckShader, false);
    }
    if (!ok) { return nullptr; }
    pinsel->setzeDetail(true);
    return pinsel;
}

}  // namespace bhed::karte
