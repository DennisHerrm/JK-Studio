// geometrieimport.cpp - Karten-Editor, Stufe 9 Punkt 2: Geometrie zu Brushes (siehe geometrieimport.h)
//
// Portiert aus q3map2 (NRC 20260114): Regeln aus brush.cpp (CreateBrushWindings) und
// facebsp.cpp (SelectSplitPlaneNum), neu geschrieben.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
#include "bhed/karte/geometrieimport.h"

#include "bhed/karte/karte.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/rueckgaengig.h"
#include "bhed/karte/texdef.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <map>
#include <tuple>
#include <unordered_map>

#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace bhed::karte {

namespace {

struct V {
    double x = 0, y = 0, z = 0;
};
V operator+(V a, V b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
V operator-(V a, V b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
V operator*(V a, double s) { return {a.x * s, a.y * s, a.z * s}; }
double dot(V a, V b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
V cross(V a, V b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
double len(V a) { return std::sqrt(dot(a, a)); }
V norm(V a) {
    const double l = len(a);
    return l > 0 ? a * (1.0 / l) : V{};
}
V vonP(const ImportPunkt& p) { return {p.x, p.y, p.z}; }
ImportPunkt zuP(V v) { return {v.x, v.y, v.z}; }

constexpr double kEps = 0.01;      // Ebenen-Abstand (q3map2 ON_EPSILON 0.1 - feiner, die Netze sind genau)
constexpr double kGross = 131072;  // Kasten fuer CreateBrushWindings (MAX_WORLD_COORD * 2)

struct Ebene {
    V n;
    double d = 0;
    int quelle = -1;   // Index der Netz-Ebene, deren Flaeche hier liegt (-1: keine = caulk)
};

double abst(const Ebene& e, V p) { return dot(e.n, p) - e.d; }

using Wind = std::vector<V>;

// ClipWindingEpsilon: die Seite hinter der Ebene behalten
Wind hinten(const Wind& w, const Ebene& e) {
    Wind r;
    const std::size_t n = w.size();
    for (std::size_t i = 0; i < n; ++i) {
        const V a = w[i];
        const V b = w[(i + 1) % n];
        const double da = abst(e, a);
        const double db = abst(e, b);
        if (da <= kEps) { r.push_back(a); }
        if ((da < -kEps && db > kEps) || (da > kEps && db < -kEps)) { r.push_back(a + (b - a) * (da / (da - db))); }
    }
    return r;
}

void teilen(const Wind& w, const Ebene& e, Wind& vorn, Wind& hint) {
    vorn.clear();
    hint.clear();
    const std::size_t n = w.size();
    for (std::size_t i = 0; i < n; ++i) {
        const V a = w[i];
        const V b = w[(i + 1) % n];
        const double da = abst(e, a);
        const double db = abst(e, b);
        if (da >= -kEps) { vorn.push_back(a); }
        if (da <= kEps) { hint.push_back(a); }
        if ((da < -kEps && db > kEps) || (da > kEps && db < -kEps)) {
            const V m = a + (b - a) * (da / (da - db));
            vorn.push_back(m);
            hint.push_back(m);
        }
    }
}

double flaeche(const Wind& w, V n) {
    V s{};
    for (std::size_t i = 1; i + 1 < w.size(); ++i) { s = s + cross(w[i] - w[0], w[i + 1] - w[0]); }
    return 0.5 * dot(s, n);
}

// BaseWindingForPlane
Wind grundWindung(const Ebene& e) {
    const V hoch = std::fabs(e.n.z) < 0.9 ? V{0, 0, 1} : V{1, 0, 0};
    const V u = norm(cross(hoch, e.n));
    const V v = cross(e.n, u);
    const V c = e.n * e.d;
    return {c + (u * -1 + v * -1) * kGross, c + (u + v * -1) * kGross, c + (u + v) * kGross, c + (u * -1 + v) * kGross};
}

// Die Netz-Daten eines Teils: geschweisste Punkte, Dreiecke, Ebenen
struct Daten {
    std::vector<V> p;
    std::vector<std::array<double, 2>> uv;
    std::vector<std::array<std::uint32_t, 3>> tri;   // geschweisst (Position)
    std::vector<std::array<std::uint32_t, 3>> triUv; // Original-Indizes (uv)
    std::vector<int> triEbene;
    std::vector<Ebene> ebenen;
    std::vector<int> gegen;   // die entgegengesetzte Ebene (-1)
};

Daten vorbereiten(const ImportTeil& t) {
    Daten d;
    std::map<std::tuple<long long, long long, long long>, std::uint32_t> weld;
    std::vector<std::uint32_t> neu(t.punkte.size());
    for (std::size_t i = 0; i < t.punkte.size(); ++i) {
        const auto k = std::make_tuple(std::llround(t.punkte[i].x * 1000), std::llround(t.punkte[i].y * 1000), std::llround(t.punkte[i].z * 1000));
        const auto it = weld.find(k);
        if (it != weld.end()) {
            neu[i] = it->second;
        } else {
            neu[i] = static_cast<std::uint32_t>(d.p.size());
            weld.emplace(k, neu[i]);
            d.p.push_back(vonP(t.punkte[i]));
        }
    }
    d.uv = t.uv;
    std::map<std::tuple<long long, long long, long long, long long>, int> ebIndex;
    const auto schluessel = [](V n, double dd) {
        return std::make_tuple(std::llround(n.x * 1e4), std::llround(n.y * 1e4), std::llround(n.z * 1e4), std::llround(dd * 16));
    };
    for (const auto& tr : t.dreiecke) {
        if (tr[0] >= t.punkte.size() || tr[1] >= t.punkte.size() || tr[2] >= t.punkte.size()) { continue; }
        const std::array<std::uint32_t, 3> w = {neu[tr[0]], neu[tr[1]], neu[tr[2]]};
        if (w[0] == w[1] || w[1] == w[2] || w[0] == w[2]) { continue; }
        const V a = d.p[w[0]];
        const V n = cross(d.p[w[1]] - a, d.p[w[2]] - a);
        if (len(n) < 1e-6) { continue; }
        const V nn = norm(n);
        const double dd = dot(nn, a);
        const auto k = schluessel(nn, dd);
        int idx = 0;
        if (const auto it = ebIndex.find(k); it != ebIndex.end()) {
            idx = it->second;
        } else {
            idx = static_cast<int>(d.ebenen.size());
            ebIndex.emplace(k, idx);
            d.ebenen.push_back({nn, dd, idx});
        }
        d.tri.push_back(w);
        d.triUv.push_back(tr);
        d.triEbene.push_back(idx);
    }
    d.gegen.assign(d.ebenen.size(), -1);
    for (std::size_t i = 0; i < d.ebenen.size(); ++i) {
        const Ebene& e = d.ebenen[i];
        if (const auto it = ebIndex.find(schluessel(e.n * -1, -e.d)); it != ebIndex.end()) { d.gegen[i] = it->second; }
    }
    return d;
}

bool geschlossenDaten(const Daten& d) {
    if (d.tri.empty()) { return false; }
    std::map<std::pair<std::uint32_t, std::uint32_t>, int> kanten;
    for (const auto& t : d.tri) {
        for (int i = 0; i < 3; ++i) { ++kanten[{t[static_cast<std::size_t>(i)], t[static_cast<std::size_t>((i + 1) % 3)]}]; }
    }
    for (const auto& [k, n] : kanten) {
        if (n != 1) { return false; }
        const auto g = kanten.find({k.second, k.first});
        if (g == kanten.end() || g->second != 1) { return false; }
    }
    return true;
}

// Punkt in einem konvexen Polygon (auf dessen Ebene, Normale n)
bool innen(const Wind& w, V n, V q) {
    for (std::size_t i = 0; i < w.size(); ++i) {
        const V a = w[i];
        const V b = w[(i + 1) % w.size()];
        if (dot(cross(b - a, q - a), n) < -kEps) { return false; }
    }
    return true;
}

// st an einem Punkt der Dreiecksebene (affin aus dem Dreieck fortgesetzt)
std::array<double, 2> stAn(const Daten& d, std::size_t tri, V q) {
    const auto& w = d.tri[tri];
    const auto& o = d.triUv[tri];
    const V a = d.p[w[0]], b = d.p[w[1]], c = d.p[w[2]];
    const V e0 = b - a, e1 = c - a, e2 = q - a;
    const double d00 = dot(e0, e0), d01 = dot(e0, e1), d11 = dot(e1, e1), d20 = dot(e2, e0), d21 = dot(e2, e1);
    const double den = d00 * d11 - d01 * d01;
    if (std::fabs(den) < 1e-12) { return {0, 0}; }
    const double v = (d11 * d20 - d01 * d21) / den;
    const double ww = (d00 * d21 - d01 * d20) / den;
    const double u = 1.0 - v - ww;
    const auto& ua = d.uv[o[0]];
    const auto& ub = d.uv[o[1]];
    const auto& uc = d.uv[o[2]];
    return {u * ua[0] + v * ub[0] + ww * uc[0], u * ua[1] + v * ub[1] + ww * uc[1]};
}

struct Bauer {
    const Daten& d;
    const ImportTeil& t;
    std::vector<std::vector<std::size_t>> triJeEbene;
    std::vector<ImportPinsel> aus;
    std::string shader;   // Material bzw. clip
    std::string caulk;
    bool alleClip = false;
    std::size_t knoten = 0;
    std::size_t grenze = 0;
    bool abgebrochen = false;

    // Ein Brush aus Halbraeumen: Windungen (CreateBrushWindings), leere Seiten weg
    void pinselAusEbenen(const std::vector<Ebene>& eb) {
        ImportPinsel pin;
        for (std::size_t i = 0; i < eb.size(); ++i) {
            Wind w = grundWindung(eb[i]);
            for (std::size_t j = 0; j < eb.size() && w.size() >= 3; ++j) {
                if (j != i) { w = hinten(w, eb[j]); }
            }
            if (w.size() < 3 || flaeche(w, eb[i].n) < 0.01) { continue; }
            // doppelte Ebene (gleiche Normale und Abstand) nur einmal
            bool doppelt = false;
            for (std::size_t j = 0; j < i; ++j) {
                if (dot(eb[j].n, eb[i].n) > 1 - 1e-9 && std::fabs(eb[j].d - eb[i].d) < 1e-6) { doppelt = true; }
            }
            if (doppelt) { continue; }
            ImportSeite s;
            const std::size_t k = w.size();
            const V a = w[0], b = w[k / 3 == 0 ? 1 : k / 3], c = w[(2 * k) / 3 == k / 3 ? k - 1 : (2 * k) / 3];
            s.p = {zuP(a), zuP(c), zuP(b)};   // (p2-p0) x (p1-p0) = (b-a) x (c-a) = n
            s.shader = caulk;
            if (eb[i].quelle >= 0 && static_cast<std::size_t>(eb[i].quelle) < triJeEbene.size()) {
                // eine Netz-Flaeche dieser Ebene, die die Seite ueberdeckt
                V mitte{};
                for (const V& q : w) { mitte = mitte + q; }
                mitte = mitte * (1.0 / static_cast<double>(w.size()));
                std::size_t treffer = static_cast<std::size_t>(-1);
                for (std::size_t tr : triJeEbene[static_cast<std::size_t>(eb[i].quelle)]) {
                    const auto& ti = d.tri[tr];
                    const Wind dw = {d.p[ti[0]], d.p[ti[1]], d.p[ti[2]]};
                    const V tm = (dw[0] + dw[1] + dw[2]) * (1.0 / 3.0);
                    if (innen(dw, eb[i].n, mitte) || innen(w, eb[i].n, tm)) {
                        treffer = tr;
                        break;
                    }
                }
                if (treffer != static_cast<std::size_t>(-1)) {
                    s.shader = shader;
                    if (t.hatUv && !alleClip) {
                        const std::array<V, 3> pp = {a, c, b};
                        for (std::size_t m = 0; m < 3; ++m) {
                            const auto st = stAn(d, treffer, pp[m]);
                            s.st[m] = {st[0], st[1], 0};
                        }
                        s.uvGesetzt = true;
                    }
                }
            }
            if (alleClip) { s.shader = shader; }
            pin.seiten.push_back(std::move(s));
        }
        if (pin.seiten.size() >= 4) { aus.push_back(std::move(pin)); }
    }

    struct Poly {
        Wind w;
        int ebene;
    };

    // Solid-Leaf-BSP: festes Blatt = konvexer Brush
    void bsp(std::vector<Poly> polys, std::vector<Ebene>& pfad, std::vector<char>& benutzt) {
        if (abgebrochen) { return; }
        if (++knoten > grenze * 64 + 4096 || aus.size() > grenze) {
            abgebrochen = true;
            return;
        }
        // SelectSplitPlaneNum: Kandidaten aus den noch nicht benutzten Ebenen
        std::vector<int> kand;
        for (const Poly& p : polys) {
            if (!benutzt[static_cast<std::size_t>(p.ebene)] && std::find(kand.begin(), kand.end(), p.ebene) == kand.end()) { kand.push_back(p.ebene); }
        }
        if (kand.empty()) { return; }   // nur Rand-Flaechen: leeres Blatt
        int best = kand.front();
        long long bestWert = -1;
        const std::size_t schritt = std::max<std::size_t>(1, kand.size() / 24);
        for (std::size_t ci = 0; ci < kand.size(); ci += schritt) {
            const Ebene& e = d.ebenen[static_cast<std::size_t>(kand[ci])];
            long long vorn = 0, hint = 0, split = 0;
            for (const Poly& p : polys) {
                bool v = false, h = false;
                for (const V& q : p.w) {
                    const double a = abst(e, q);
                    if (a > kEps) { v = true; }
                    if (a < -kEps) { h = true; }
                }
                if (v && h) {
                    ++split;
                } else if (v) {
                    ++vorn;
                } else if (h) {
                    ++hint;
                }
            }
            long long wert = 5 * split + std::llabs(vorn - hint);
            const bool axial = std::fabs(e.n.x) > 0.9999 || std::fabs(e.n.y) > 0.9999 || std::fabs(e.n.z) > 0.9999;
            if (!axial) { wert += 5; }   // q3map2: axiale Ebenen bevorzugt
            if (bestWert < 0 || wert < bestWert) {
                bestWert = wert;
                best = kand[ci];
            }
        }
        const Ebene s = d.ebenen[static_cast<std::size_t>(best)];
        std::vector<Poly> vorn, hint;
        for (Poly& p : polys) {
            if (p.ebene == best) {
                vorn.push_back(std::move(p));   // gleich ausgerichtet auf dem Teiler: Rand der Vorderseite
                continue;
            }
            if (d.gegen[static_cast<std::size_t>(best)] == p.ebene) {
                hint.push_back(std::move(p));   // entgegengesetzt: Rand der Rueckseite
                continue;
            }
            Wind a, b;
            teilen(p.w, s, a, b);
            const bool av = a.size() >= 3 && std::fabs(flaeche(a, d.ebenen[static_cast<std::size_t>(p.ebene)].n)) > 1e-4;
            const bool bv = b.size() >= 3 && std::fabs(flaeche(b, d.ebenen[static_cast<std::size_t>(p.ebene)].n)) > 1e-4;
            bool ganzVorn = true, ganzHinten = true;
            for (const V& q : p.w) {
                const double x = abst(s, q);
                if (x < -kEps) { ganzVorn = false; }
                if (x > kEps) { ganzHinten = false; }
            }
            if (ganzVorn) {
                vorn.push_back(std::move(p));
            } else if (ganzHinten) {
                hint.push_back(std::move(p));
            } else {
                if (av) { vorn.push_back({std::move(a), p.ebene}); }
                if (bv) { hint.push_back({std::move(b), p.ebene}); }
            }
        }
        polys.clear();
        const std::size_t g = static_cast<std::size_t>(d.gegen[static_cast<std::size_t>(best)]);
        const char altS = benutzt[static_cast<std::size_t>(best)];
        const char altG = d.gegen[static_cast<std::size_t>(best)] >= 0 ? benutzt[g] : 0;
        benutzt[static_cast<std::size_t>(best)] = 1;
        if (d.gegen[static_cast<std::size_t>(best)] >= 0) { benutzt[g] = 1; }
        // Rueckseite: ohne Flaechen fest
        Ebene hs = s;
        hs.quelle = best;
        pfad.push_back(hs);
        if (hint.empty()) {
            pinselAusEbenen(pfad);
        } else {
            bsp(std::move(hint), pfad, benutzt);
        }
        pfad.pop_back();
        // Vorderseite: ohne Flaechen leer
        if (!vorn.empty()) {
            Ebene vs{s.n * -1, -s.d, d.gegen[static_cast<std::size_t>(best)]};
            pfad.push_back(vs);
            bsp(std::move(vorn), pfad, benutzt);
            pfad.pop_back();
        }
        benutzt[static_cast<std::size_t>(best)] = altS;
        if (d.gegen[static_cast<std::size_t>(best)] >= 0) { benutzt[g] = altG; }
    }

    // Offene Netze: planare, konvexe Flaechen als Prisma nach hinten
    void prismen(double dicke) {
        for (std::size_t e = 0; e < d.ebenen.size(); ++e) {
            const Ebene& eb = d.ebenen[e];
            std::vector<std::vector<std::uint32_t>> polys;
            for (std::size_t tr : triJeEbene[e]) { polys.push_back({d.tri[tr][0], d.tri[tr][1], d.tri[tr][2]}); }
            // benachbarte Polygone zusammenlegen, solange das Ergebnis konvex bleibt
            bool weiter = true;
            while (weiter) {
                weiter = false;
                for (std::size_t i = 0; i < polys.size() && !weiter; ++i) {
                    for (std::size_t j = i + 1; j < polys.size() && !weiter; ++j) {
                        const auto& A = polys[i];
                        const auto& B = polys[j];
                        for (std::size_t ai = 0; ai < A.size() && !weiter; ++ai) {
                            const std::uint32_t a0 = A[ai], a1 = A[(ai + 1) % A.size()];
                            for (std::size_t bi = 0; bi < B.size(); ++bi) {
                                if (B[bi] != a1 || B[(bi + 1) % B.size()] != a0) { continue; }
                                std::vector<std::uint32_t> m;
                                for (std::size_t k = 0; k < A.size(); ++k) { m.push_back(A[(ai + 1 + k) % A.size()]); }   // a1 ... a0
                                for (std::size_t k = 2; k < B.size(); ++k) { m.push_back(B[(bi + k) % B.size()]); }       // ohne a0, a1
                                // Kollineare Punkte weg, dann konvex?
                                bool konvex = m.size() >= 3;
                                std::vector<std::uint32_t> r;
                                for (std::size_t k = 0; k < m.size(); ++k) {
                                    const V p0 = d.p[m[(k + m.size() - 1) % m.size()]], p1 = d.p[m[k]], p2 = d.p[m[(k + 1) % m.size()]];
                                    const double c = dot(cross(p1 - p0, p2 - p1), eb.n);
                                    if (c < -1e-6) { konvex = false; }
                                    if (c > 1e-6) { r.push_back(m[k]); }
                                }
                                if (konvex && r.size() >= 3) {
                                    polys[i] = r;
                                    polys.erase(polys.begin() + static_cast<std::ptrdiff_t>(j));
                                    weiter = true;
                                }
                                break;
                            }
                        }
                    }
                }
            }
            for (const auto& poly : polys) {
                std::vector<Ebene> pr;
                Ebene oben = eb;
                oben.quelle = static_cast<int>(e);
                pr.push_back(oben);
                pr.push_back({eb.n * -1, -(eb.d - dicke), -1});
                for (std::size_t k = 0; k < poly.size(); ++k) {
                    const V a = d.p[poly[k]], b = d.p[poly[(k + 1) % poly.size()]];
                    const V no = norm(cross(b - a, eb.n));
                    pr.push_back({no, dot(no, a), -1});
                }
                pinselAusEbenen(pr);
            }
        }
    }
};

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

}  // namespace

std::string materialShader(const std::string& material, const ZerlegOptionen& o) {
    std::string m = material;
    const std::size_t sl = m.find_last_of("/\\");
    if (sl != std::string::npos) { m = m.substr(sl + 1); }
    const std::size_t pk = m.rfind('.');
    if (pk != std::string::npos && pk > 0) { m = m.substr(0, pk); }
    m = klein(m);
    std::string r;
    for (const char c : m) { r += (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '-') ? c : '_'; }
    if (r.empty()) { return o.ohneMaterial; }
    return o.shaderVorsatz + r;
}

bool teilGeschlossen(const ImportTeil& t) { return geschlossenDaten(vorbereiten(t)); }

TeilErgebnis teilUmwandeln(const ImportTeil& t, TeilArt wahl, const ZerlegOptionen& o) {
    TeilErgebnis r;
    const Daten d = vorbereiten(t);
    r.dreiecke = d.tri.size();
    r.ebenen = d.ebenen.size();
    r.geschlossen = geschlossenDaten(d);
    double groesste = 0;
    if (!d.p.empty()) {
        V mn = d.p[0], mx = d.p[0];
        for (const V& p : d.p) {
            mn = {std::min(mn.x, p.x), std::min(mn.y, p.y), std::min(mn.z, p.z)};
            mx = {std::max(mx.x, p.x), std::max(mx.y, p.y), std::max(mx.z, p.z)};
        }
        groesste = std::max({mx.x - mn.x, mx.y - mn.y, mx.z - mn.z});
    }
    r.detail = !r.geschlossen || groesste < o.detailGroesse;
    if (d.tri.empty()) { return r; }
    const auto bauen = [&](bool clip, std::size_t grenze, bool& ok) {
        Bauer b{d, t, {}, {}, clip ? o.clip : (t.material.empty() ? o.ohneMaterial : materialShader(t.material, o)), o.caulk, clip, 0, grenze, false};
        b.triJeEbene.assign(d.ebenen.size(), {});
        for (std::size_t i = 0; i < d.tri.size(); ++i) { b.triJeEbene[static_cast<std::size_t>(d.triEbene[i])].push_back(i); }
        if (r.geschlossen) {
            std::vector<Bauer::Poly> polys;
            for (std::size_t i = 0; i < d.tri.size(); ++i) { polys.push_back({{d.p[d.tri[i][0]], d.p[d.tri[i][1]], d.p[d.tri[i][2]]}, d.triEbene[i]}); }
            std::vector<Ebene> pfad;
            std::vector<char> benutzt(d.ebenen.size(), 0);
            b.bsp(std::move(polys), pfad, benutzt);
        } else {
            b.prismen(o.dicke);
        }
        ok = !b.abgebrochen && !b.aus.empty();
        return std::move(b.aus);
    };
    bool ok = false;
    TeilArt art = wahl;
    if (wahl == TeilArt::Auto) {
        r.pinsel = bauen(false, o.maxPinsel, ok);
        art = (r.geschlossen && ok) ? TeilArt::Pinsel : TeilArt::ModellClip;
        if (art == TeilArt::ModellClip) { r.pinsel = bauen(true, std::max<std::size_t>(o.maxPinsel, 4096), ok); }
    } else {
        r.pinsel = bauen(wahl == TeilArt::ModellClip, std::max<std::size_t>(o.maxPinsel, 4096), ok);
    }
    r.art = art;
    for (ImportPinsel& p : r.pinsel) { p.detail = r.detail || art == TeilArt::ModellClip; }
    return r;
}

EinfuegeErgebnis importEinfuegen(Karte& k, const std::vector<TeilErgebnis>& teile, const std::string& modellPfad) {
    EinfuegeErgebnis r;
    Befehl undo(k.undo, "importGeometry");
    k.auswahl.leeren();
    const auto welt = k.welt();
    bool modell = false;
    const auto vek = [](const ImportPunkt& p) { return Vektor3d(p.x, p.y, p.z); };
    for (const TeilErgebnis& t : teile) {
        modell = modell || t.art == TeilArt::ModellClip;
        for (const ImportPinsel& ip : t.pinsel) {
            auto p = std::make_shared<Pinsel>();
            for (const ImportSeite& s : ip.seiten) {
                const EbenenPunkte pp{vek(s.p[0]), vek(s.p[1]), vek(s.p[2])};
                TexturProjektion proj;
                texdefStandard(k.kontext.art(), proj);
                if (s.uvGesetzt) {
                    Vektor3d st[3];
                    for (std::size_t i = 0; i < 3; ++i) { st[i] = Vektor3d(s.st[i].x, s.st[i].y, 0.0); }
                    const auto [w, h] = k.kontext.groesse(s.shader);
                    texdefAusSt(k.kontext.art(), proj, pp, st, w, h);
                }
                (void)p->ebeneHinzufuegen(pp[0], pp[1], pp[2], s.shader, proj, k.kontext);
            }
            if (p->anzahl() < 4) { continue; }
            if (ip.detail) { p->setzeDetail(true); }
            p->schicht = static_cast<int>(k.aktuelleSchicht);
            welt->kindEinfuegen(p);
            k.auswahl.waehlen(welt, p);
            ++r.pinsel;
        }
    }
    if (modell && !modellPfad.empty()) {
        auto ent = k.neueEntitaet("misc_model", false);
        ent->setzen("origin", "0 0 0");
        ent->setzen("model", modellPfad);
        k.entitaetEinfuegen(ent);
        k.auswahl.waehlen(ent);
        ++r.modelle;
    }
    return r;
}

std::string modellObj(const std::vector<const ImportTeil*>& teile, const ZerlegOptionen& o) {
    std::string s = "# JK Studio: Modell+Clip-Teile (Stufe 9 Punkt 2)\n";
    std::size_t basis = 1;
    char z[160];
    for (const ImportTeil* t : teile) {
        s += "o " + (t->name.empty() ? std::string("part") : t->name) + "\n";
        for (const ImportPunkt& p : t->punkte) {
            std::snprintf(z, sizeof(z), "v %.6f %.6f %.6f\n", p.x, p.y, p.z);
            s += z;
        }
        for (const auto& uv : t->uv) {
            std::snprintf(z, sizeof(z), "vt %.6f %.6f\n", uv[0], 1.0 - uv[1]);
            s += z;
        }
        s += "usemtl " + (t->material.empty() ? o.ohneMaterial : materialShader(t->material, o)) + "\n";
        for (const auto& d : t->dreiecke) {
            std::snprintf(z, sizeof(z), "f %zu/%zu %zu/%zu %zu/%zu\n", basis + d[0], basis + d[0], basis + d[1], basis + d[1], basis + d[2], basis + d[2]);
            s += z;
        }
        basis += t->punkte.size();
    }
    return s;
}

// --- Lesen ueber assimp ---------------------------------------------------------------

namespace {

void knotenSammeln(const aiScene* sc, const aiNode* k, const aiMatrix4x4& eltern, const NetzOptionen& o, ImportNetz& out) {
    const aiMatrix4x4 m = eltern * k->mTransformation;
    for (unsigned i = 0; i < k->mNumMeshes; ++i) {
        const aiMesh* me = sc->mMeshes[k->mMeshes[i]];
        if (me == nullptr || me->mNumVertices == 0) { continue; }
        ImportTeil t;
        t.name = k->mName.length > 0 ? std::string(k->mName.C_Str()) : std::string(me->mName.C_Str());
        if (t.name.empty()) { t.name = "part" + std::to_string(out.teile.size() + 1); }
        if (me->mMaterialIndex < sc->mNumMaterials) {
            aiString n;
            const aiMaterial* mat = sc->mMaterials[me->mMaterialIndex];
            if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &n) == AI_SUCCESS && n.length > 0) {
                t.material = n.C_Str();
            } else if (mat->Get(AI_MATKEY_NAME, n) == AI_SUCCESS) {
                t.material = n.C_Str();
            }
        }
        t.hatUv = me->HasTextureCoords(0);
        for (unsigned v = 0; v < me->mNumVertices; ++v) {
            const aiVector3D p = m * me->mVertices[v];
            ImportPunkt q{p.x * o.skala, p.y * o.skala, p.z * o.skala};
            if (o.yOben) { q = {p.x * o.skala, -p.z * o.skala, p.y * o.skala}; }
            t.punkte.push_back(q);
            if (t.hatUv) {
                // assimp liefert UV mit v nach oben (glTF wird umgedreht) - Q3: t nach unten
                t.uv.push_back({me->mTextureCoords[0][v].x, 1.0 - me->mTextureCoords[0][v].y});
            } else {
                t.uv.push_back({0, 0});
            }
        }
        // gespiegelte Lage (negative Determinante): Umlaufsinn umdrehen
        const bool spiegel = m.Determinant() < 0;
        for (unsigned f = 0; f < me->mNumFaces; ++f) {
            const aiFace& fa = me->mFaces[f];
            if (fa.mNumIndices != 3) { continue; }
            if (spiegel) {
                t.dreiecke.push_back({fa.mIndices[0], fa.mIndices[2], fa.mIndices[1]});
            } else {
                t.dreiecke.push_back({fa.mIndices[0], fa.mIndices[1], fa.mIndices[2]});
            }
        }
        if (!t.dreiecke.empty()) { out.teile.push_back(std::move(t)); }
    }
    for (unsigned c = 0; c < k->mNumChildren; ++c) { knotenSammeln(sc, k->mChildren[c], m, o, out); }
}

}  // namespace

bool netzLesen(const std::string& bytes, const std::string& endung, const NetzOptionen& o, ImportNetz& out, std::string* fehler) {
    out.teile.clear();
    const std::string e = klein(endung);
    if (e != "obj" && e != "fbx" && e != "glb" && e != "gltf") {
        if (fehler != nullptr) { *fehler = "unsupported format ." + endung; }
        return false;
    }
    if (bytes.empty()) {
        if (fehler != nullptr) { *fehler = "empty file"; }
        return false;
    }
    Assimp::Importer imp;
    // ohne eigene Normalen, ohne Optimierung: die Geometrie wie gespeichert, nur in Dreiecke zerlegt
    const aiScene* sc = imp.ReadFileFromMemory(bytes.data(), bytes.size(), aiProcess_Triangulate | aiProcess_SortByPType, e.c_str());
    if (sc == nullptr || sc->mRootNode == nullptr) {
        if (fehler != nullptr) { *fehler = imp.GetErrorString(); }
        return false;
    }
    knotenSammeln(sc, sc->mRootNode, aiMatrix4x4(), o, out);
    if (out.teile.empty()) {
        if (fehler != nullptr) { *fehler = "no triangles"; }
        return false;
    }
    return true;
}

}  // namespace bhed::karte
