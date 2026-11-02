// echtlicht.cpp - Karten-Editor Stufe 7 Punkt 5: die Lichter der Karte wie
// q3map2 -light (siehe echtlicht.h).
//
// Portiert aus q3map2 (NetRadiant-Custom 20260114: tools/quake3/q3map2/light.cpp
// CreateEntityLights, CreateSunLight, LightContributionToSample;
// CreateSkyLights, LightContributionToSample, PointToPolygonFormFactor;
// light_ydnar.cpp SetupEnvelopes; light_bounce.cpp RadLightForTriangles,
// RadSubdivideDiffuseLight, RadClipWindingEpsilon).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   Copyright (C) 2026 BehavEd-Remake contributors.
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version (LICENSE).
#include "bhed/karte/echtlicht.h"

#include "bhed/karte/karte.h"
#include "bhed/karte/modell.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"

#include <algorithm>
#include <cctype>
#include <array>
#include <cmath>
#include <cstdlib>
#include <sstream>

namespace bhed::karte {

namespace {

// rc608 Teil 11: q3map2 liest Schluessel ohne Gross- und Kleinschreibung (CASE_INSENSITIVE_EPAIRS, EPAIR_EQUAL =
// striEqual, q3map2.h:109-112; entity_t::valueForKey, bspfile_abstract.cpp:535) - "_minVertexlight" auf
// duel_geonosis_arena gilt so wie "_minvertexlight". Der erste passende Schluessel zaehlt.
std::string wertQ3(const Entitaet& e, const std::string& k) {
    for (const auto& [s, w] : e.schluessel()) {
        if (s.size() == k.size() && std::equal(s.begin(), s.end(), k.begin(), [](char a, char b) {
                return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
            })) {
            return w;
        }
    }
    return {};
}

bool zahlen(const std::string& s, float* v, int n) {
    std::istringstream in(s);
    for (int i = 0; i < n; ++i) {
        if (!(in >> v[i])) { return false; }
    }
    return true;
}

float zahl(const Entitaet& e, const char* a, const char* b = nullptr, float vorgabe = 0) {
    for (const char* k : {a, b}) {
        if (k == nullptr) { continue; }
        const std::string w = wertQ3(e, k);
        if (!w.empty()) { return static_cast<float>(std::atof(w.c_str())); }
    }
    return vorgabe;
}

// ColorNormalize (mathlib): durch die groesste Komponente teilen
Vektor3f normiert(Vektor3f c) {
    const float m = std::max({c[0], c[1], c[2]});
    if (m == 0) { return Vektor3f(1, 1, 1); }
    return c / m;
}

bool klassePraefix(const std::string& c, const char* p) {
    const std::size_t n = std::char_traits<char>::length(p);
    if (c.size() < n) { return false; }
    for (std::size_t i = 0; i < n; ++i) {
        if (std::tolower(static_cast<unsigned char>(c[i])) != p[i]) { return false; }
    }
    return true;
}

// SetupEnvelopes mit -fast (light_ydnar.cpp:3830-3855): ab hier kommt nichts mehr an
float punkt3(const Vektor3f& a, const Vektor3f& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

void huelleSetzen(EchtLicht& l) {
    if (l.art == EchtLicht::Art::Sonne) {
        l.huelle = 1e9F;
        return;
    }
    if (l.art == EchtLicht::Art::Flaeche && !l.wicklung.empty()) {
        // exactPointToPolygon mit FastActual (light_ydnar.cpp:4851-4868): "ugly hack"
        // radius = 100, 110, ... bis f * add <= falloffTolerance. Entlang der Normale
        // faellt der Formfaktor stetig: dieselbe erste Stufe per Halbierung statt
        // bis zu 52000 Schritten (sonst kostete jede Aenderung grosser Karten Zeit).
        l.huelle = 65536.0F * 8.0F;   // MAX_WORLD_COORD * 8
        const Vektor3f dir(-l.normale[0], -l.normale[1], -l.normale[2]);
        const auto unter = [&l, &dir](int k) {
            const float radius = 100.0F + 10.0F * static_cast<float>(k);
            const Vektor3f o = l.ursprung + l.normale * radius;
            return std::fabs(echtFormFaktor(o, dir, l.wicklung)) * l.add <= kEchtAbfallGrenze;
        };
        const int n = static_cast<int>((65536.0F * 8.0F - 100.0F) / 10.0F);
        if (!unter(n - 1)) { return; }
        int lo = 0;
        int hi = n - 1;   // unter(hi) gilt
        while (lo < hi) {
            const int m = lo + (hi - lo) / 2;
            if (unter(m)) {
                hi = m;
            } else {
                lo = m + 1;
            }
        }
        l.huelle = 100.0F + 10.0F * static_cast<float>(lo);
        return;
    }
    if (l.linear) {
        l.huelle = ((l.photonen * kEchtLinearSkala) - kEchtAbfallGrenze) / std::max(l.fade, 1e-6F);
    } else {
        l.huelle = std::sqrt(std::max(l.photonen, 0.0F) / kEchtAbfallGrenze);
    }
}

Vektor3f kugelRichtung(float gradA, float gradB) {   // vector3_for_spherical
    const float a = gradA * 3.14159265358979F / 180.0F;
    const float b = gradB * 3.14159265358979F / 180.0F;
    return Vektor3f(std::cos(a) * std::cos(b), std::sin(a) * std::cos(b), std::sin(b));
}

// RadClipWindingEpsilon (light_bounce.cpp) mit einer Achsenebene: vorn (>= dist) und hinten
constexpr float kClipEpsilon = 0.125F;   // RADIOSITY_CLIP_EPSILON
void wicklungTeilen(const std::vector<Vektor3f>& w, int achse, float dist, std::vector<Vektor3f>& vorn, std::vector<Vektor3f>& hinten,
                    const std::vector<Vektor3f>* nw = nullptr, std::vector<Vektor3f>* nVorn = nullptr, std::vector<Vektor3f>* nHinten = nullptr) {
    vorn.clear();
    hinten.clear();
    const bool mitN = nw != nullptr && nVorn != nullptr && nHinten != nullptr && nw->size() == w.size();
    if (mitN) {
        nVorn->clear();
        nHinten->clear();
    }
    const std::size_t n = w.size();
    std::vector<float> d(n + 1);
    std::vector<int> seite(n + 1);
    int zahl[3] = {0, 0, 0};
    for (std::size_t i = 0; i < n; ++i) {
        d[i] = w[i][achse] - dist;
        seite[i] = d[i] > kClipEpsilon ? 0 : d[i] < -kClipEpsilon ? 1 : 2;
        ++zahl[seite[i]];
    }
    d[n] = d[0];
    seite[n] = seite[0];
    if (zahl[0] == 0) {   // nichts vorn
        hinten = w;
        if (mitN) { *nHinten = *nw; }
        return;
    }
    if (zahl[1] == 0) {
        vorn = w;
        if (mitN) { *nVorn = *nw; }
        return;
    }
    for (std::size_t i = 0; i < n; ++i) {
        const Vektor3f& p1 = w[i];
        if (seite[i] == 2) {
            vorn.push_back(p1);
            hinten.push_back(p1);
            if (mitN) {
                nVorn->push_back((*nw)[i]);
                nHinten->push_back((*nw)[i]);
            }
            continue;
        }
        (seite[i] == 0 ? vorn : hinten).push_back(p1);
        if (mitN) { (seite[i] == 0 ? *nVorn : *nHinten).push_back((*nw)[i]); }
        if (seite[i + 1] == 2 || seite[i + 1] == seite[i]) { continue; }
        const Vektor3f& p2 = w[(i + 1) % n];
        const float dd = d[i] / (d[i] - d[i + 1]);
        Vektor3f mitte = p1 + (p2 - p1) * dd;
        mitte[achse] = dist;   // achsenparallel: genau auf der Ebene
        vorn.push_back(mitte);
        hinten.push_back(mitte);
        if (mitN) {   // mid.normal = v1.normal + ( v2.normal - v1.normal ) * dot; VectorNormalize
            Vektor3f nm = (*nw)[i] + ((*nw)[(i + 1) % n] - (*nw)[i]) * dd;
            const float l = std::sqrt(punkt3(nm, nm));
            if (l != 0) { nm = nm / l; }
            nVorn->push_back(nm);
            nHinten->push_back(nm);
        }
    }
}

// nw: eine Normale je Ecke (Stufe 9d, Modelle und Patches) - die Normale des Lichts ist ihr Mittel;
// ohne (nullptr) gilt "normale" (Pinselflaechen: alle Ecken haben die Ebenennormale)
void unterteilen(const std::vector<Vektor3f>& w, const Vektor3f& normale, const EchtFlaechenShader& s, float unterteilung, std::vector<EchtLicht>& aus,
                 int tiefe, const std::vector<Vektor3f>* nw = nullptr) {
    if (w.size() < 3 || tiefe > 64) { return; }
    Vektor3f mn = w[0];
    Vektor3f mx = w[0];
    for (const Vektor3f& v : w) {
        for (int i = 0; i < 3; ++i) {
            mn[i] = std::min(mn[i], v[i]);
            mx[i] = std::max(mx[i], v[i]);
        }
    }
    for (int i = 0; i < 3; ++i) {
        if (mx[i] - mn[i] > unterteilung) {
            std::vector<Vektor3f> vorn;
            std::vector<Vektor3f> hinten;
            std::vector<Vektor3f> nVorn;
            std::vector<Vektor3f> nHinten;
            wicklungTeilen(w, i, (mx[i] + mn[i]) * 0.5F, vorn, hinten, nw, &nVorn, &nHinten);
            unterteilen(vorn, normale, s, unterteilung, aus, tiefe + 1, nw != nullptr ? &nVorn : nullptr);
            unterteilen(hinten, normale, s, unterteilung, aus, tiefe + 1, nw != nullptr ? &nHinten : nullptr);
            return;
        }
    }
    float flaeche = 0;
    for (std::size_t i = 2; i < w.size(); ++i) {
        const Vektor3f a = w[i - 1] - w[0];
        const Vektor3f b = w[i] - w[0];
        const Vektor3f x(a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]);
        flaeche += 0.5F * std::sqrt(punkt3(x, x));
    }
    if (flaeche < 1 || flaeche > 20000000.0F) { return; }
    // "create an average normal" (RadSubdivideDiffuseLight)
    Vektor3f nMittel = normale;
    if (nw != nullptr && nw->size() == w.size()) {
        nMittel = Vektor3f(0, 0, 0);
        for (const Vektor3f& v : *nw) { nMittel = nMittel + v; }
        nMittel = nMittel / static_cast<float>(nw->size());
        const float l = std::sqrt(punkt3(nMittel, nMittel));
        if (l == 0) { return; }
        nMittel = nMittel / l;
    }
    EchtLicht l;
    l.art = EchtLicht::Art::Flaeche;
    l.wicklung = w;
    if (l.wicklung.size() > static_cast<std::size_t>(kEchtWicklungMax)) { l.wicklung.resize(static_cast<std::size_t>(kEchtWicklungMax)); }
    l.photonen = s.flaechenlicht * flaeche * kEchtFlaechenSkala;
    l.add = s.flaechenlicht * kEchtFormFaktorSkala * kEchtFlaechenSkala;
    l.farbe = s.farbe;
    l.normale = nMittel;
    l.ursprung = (mn + mx) * 0.5F + nMittel;   // minmax.origin(), "nudge it off the plane a bit"
    l.zweiseitig = s.zweiseitig;
    l.winkel = true;
    huelleSetzen(l);
    aus.push_back(l);
    // optionally create area backsplash light (light_bounce.cpp, #if 1)
    if (s.backsplashAnteil > 0 && !s.himmel) {
        EchtLicht r;
        r.art = EchtLicht::Art::Flaeche;
        r.photonen = l.photonen * 7.0F * s.backsplashAnteil;
        r.add = l.add * 7.0F * s.backsplashAnteil;
        r.farbe = s.farbe;
        for (std::size_t i = w.size(); i-- != 0;) { r.wicklung.push_back(w[i] + nMittel * s.backsplashWeite); }
        if (r.wicklung.size() > static_cast<std::size_t>(kEchtWicklungMax)) { r.wicklung.resize(static_cast<std::size_t>(kEchtWicklungMax)); }
        r.ursprung = nMittel * s.backsplashWeite + l.ursprung;
        r.normale = Vektor3f(-nMittel[0], -nMittel[1], -nMittel[2]);
        r.winkel = true;
        huelleSetzen(r);
        aus.push_back(r);
    }
}

}  // namespace

float echtFormFaktor(const Vektor3f& punkt, const Vektor3f& normale, const std::vector<Vektor3f>& w) {
    const std::size_t n = w.size();
    if (n < 3) { return 0; }
    std::vector<Vektor3f> dirs(n + 1);
    for (std::size_t i = 0; i < n; ++i) {
        Vektor3f d = w[i] - punkt;
        const float l = std::sqrt(punkt3(d, d));
        if (l != 0) { d = d / l; }
        dirs[i] = d;
    }
    dirs[n] = dirs[0];
    double total = 0;
    for (std::size_t i = 0; i < n; ++i) {
        const Vektor3f& a = dirs[i];
        const Vektor3f& b = dirs[i + 1];
        Vektor3f tri(a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]);
        const float l = std::sqrt(punkt3(tri, tri));
        if (l < 0.0001F) { continue; }
        tri = tri / l;
        const double winkel = std::acos(std::clamp(static_cast<double>(punkt3(a, b)), -1.0, 1.0));
        const double facing = punkt3(normale, tri);
        total += facing * winkel;
        if (total > 6.3 || total < -6.3) { return 0; }
    }
    return static_cast<float>(total * (1.0 / (2.0 * 3.14159265358979323846)));
}

void echtFlaechenLichter(const std::vector<Vektor3f>& wicklung, const Vektor3f& normale, const EchtFlaechenShader& s, std::vector<EchtLicht>& aus) {
    if (s.flaechenlicht <= 0) { return; }
    const float u = s.unterteilung > 0 ? s.unterteilung : kEchtUnterteilung;
    EchtFlaechenShader n = s;
    n.farbe = normiert(s.farbe);   // si.color: ColorNormalize der mittleren Farbe
    unterteilen(wicklung, normale, n, u, aus, 0);
}

void echtFlaechenLichter(const std::vector<Vektor3f>& wicklung, const std::vector<Vektor3f>& normalen, const EchtFlaechenShader& s,
                         std::vector<EchtLicht>& aus) {
    if (s.flaechenlicht <= 0 || normalen.size() != wicklung.size()) { return; }
    const float u = s.unterteilung > 0 ? s.unterteilung : kEchtUnterteilung;
    EchtFlaechenShader n = s;
    n.farbe = normiert(s.farbe);
    unterteilen(wicklung, Vektor3f(0, 0, 1), n, u, aus, 0, &normalen);
}

// --- Stufe 9d: Patches wie in -light ----------------------------------------------------

namespace {

using V3d = Vektor3<double>;
V3d d3(const Vektor3f& v) { return V3d(v[0], v[1], v[2]); }
double punktD(const V3d& a, const V3d& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
V3d kreuzD(const V3d& a, const V3d& b) { return V3d(a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]); }
V3d plusD(const V3d& a, const V3d& b) { return V3d(a[0] + b[0], a[1] + b[1], a[2] + b[2]); }
V3d minusD(const V3d& a, const V3d& b) { return V3d(a[0] - b[0], a[1] - b[1], a[2] - b[2]); }
V3d malD(const V3d& a, double f) { return V3d(a[0] * f, a[1] * f, a[2] * f); }
// VectorNormalize (Laenge in double, Ergebnis wie Vector3 in float)
float normierenF(Vektor3f& v) {
    const double l = std::sqrt(punktD(d3(v), d3(v)));
    if (l == 0) {
        v = Vektor3f(0, 0, 0);
        return 0;
    }
    v = Vektor3f(static_cast<float>(v[0] / l), static_cast<float>(v[1] / l), static_cast<float>(v[2] / l));
    return static_cast<float>(l);
}

// bspDrawVert_t mit dem, was die Flaechenlichter und die Meta-Dreiecke brauchen (xyz, normal, st)
struct NPunkt {
    Vektor3f ort;
    Vektor3f normale;
    std::array<float, 2> st{};
};
// MAX_EXPANDED_AXIS (q3map2.h)
constexpr int kAchseMax = 128;
struct NetzF {
    int breite = 0;
    int hoehe = 0;
    std::vector<NPunkt> v;   // Zeile * breite + Spalte
    NPunkt& at(int zeile, int spalte) { return v[static_cast<std::size_t>(zeile * breite + spalte)]; }
    [[nodiscard]] const NPunkt& at(int zeile, int spalte) const { return v[static_cast<std::size_t>(zeile * breite + spalte)]; }
};

// LerpDrawVert (mesh.cpp): Mitte, Normale a + b normiert (bei Null die von a)
NPunkt mitteNP(const NPunkt& a, const NPunkt& b) {
    NPunkt o;
    o.ort = Vektor3f((a.ort[0] + b.ort[0]) * 0.5F, (a.ort[1] + b.ort[1]) * 0.5F, (a.ort[2] + b.ort[2]) * 0.5F);
    o.st = {(a.st[0] + b.st[0]) * 0.5F, (a.st[1] + b.st[1]) * 0.5F};   // vector2_mid
    o.normale = a.normale + b.normale;
    if (normierenF(o.normale) == 0) { o.normale = a.normale; }
    return o;
}

// ExpandLongestCurve (patch.cpp)
void laengsteKurve(float& laengste, const Vektor3f& a, const Vektor3f& b, const Vektor3f& c) {
    Vektor3f ab = b - a;
    if (normierenF(ab) < 0.125F) { return; }
    Vektor3f bc = c - b;
    if (normierenF(bc) < 0.125F) { return; }
    Vektor3f ac = c - a;
    if (normierenF(ac) < 0.125F) { return; }
    if (punkt3(ab, bc) > 0.99F && punkt3(ab, ac) > 0.99F) { return; }
    ab = b - a;
    bc = c - b;
    Vektor3f letzter = a;
    float len = 0;
    float tt = 0;
    for (int i = 0; i < 8; ++i, tt += (1.0F / 8.0F)) {   // APPROX_SUBDIVISION
        Vektor3f delta = ab * (1.0F - tt) + bc * tt;
        const Vektor3f pt = a + delta;
        delta = pt - letzter;
        len += std::sqrt(punkt3(delta, delta));
        letzter = pt;
    }
    laengste = std::max(laengste, len);
}

// IterationsForCurve (mesh.cpp)
int iterationenFuerKurve(float len, int unterteilungen) {
    int it = 0;
    for (; it < 3; ++it) {
        const int facetten = static_cast<int>(unterteilungen * 16 * std::pow(2.0, it));
        if (static_cast<float>(facetten) >= len) { break; }
    }
    return it;
}

// MakeMeshNormals (mesh.cpp)
void netzNormalen(NetzF& in) {
    static const int nachbarn[8][2] = {{0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}};
    bool wrapBreite = false;
    {
        int i = 0;
        for (; i < in.hoehe; ++i) {
            const Vektor3f d = in.at(i, 0).ort - in.at(i, in.breite - 1).ort;
            if (std::sqrt(punktD(d3(d), d3(d))) > 1) { break; }
        }
        wrapBreite = i == in.hoehe;
    }
    bool wrapHoehe = false;
    {
        int i = 0;
        for (; i < in.breite; ++i) {
            const Vektor3f d = in.at(0, i).ort - in.at(in.hoehe - 1, i).ort;
            if (std::sqrt(punktD(d3(d), d3(d))) > 1) { break; }
        }
        wrapHoehe = i == in.breite;
    }
    for (int i = 0; i < in.breite; ++i) {
        for (int j = 0; j < in.hoehe; ++j) {
            const Vektor3f basis = in.at(j, i).ort;
            Vektor3f um[8];
            bool gut[8];
            for (int k = 0; k < 8; ++k) {
                um[k] = Vektor3f(0, 0, 0);
                gut[k] = false;
                for (int dist = 1; dist <= 3; ++dist) {
                    int x = i + nachbarn[k][0] * dist;
                    int y = j + nachbarn[k][1] * dist;
                    if (wrapBreite) {
                        if (x < 0) {
                            x = in.breite - 1 + x;
                        } else if (x >= in.breite) {
                            x = 1 + x - in.breite;
                        }
                    }
                    if (wrapHoehe) {
                        if (y < 0) {
                            y = in.hoehe - 1 + y;
                        } else if (y >= in.hoehe) {
                            y = 1 + y - in.hoehe;
                        }
                    }
                    if (x < 0 || x >= in.breite || y < 0 || y >= in.hoehe) { break; }   // edge of patch
                    Vektor3f temp = in.at(y, x).ort - basis;
                    if (normierenF(temp) == 0) { continue; }   // degenerate edge, get more dist
                    gut[k] = true;
                    um[k] = temp;
                    break;
                }
            }
            Vektor3f summe(0, 0, 0);
            for (int k = 0; k < 8; ++k) {
                if (!gut[k] || !gut[(k + 1) & 7]) { continue; }
                const V3d a = d3(um[(k + 1) & 7]);
                const V3d b = d3(um[k]);
                Vektor3f n(static_cast<float>(a[1] * b[2] - a[2] * b[1]), static_cast<float>(a[2] * b[0] - a[0] * b[2]),
                           static_cast<float>(a[0] * b[1] - a[1] * b[0]));
                if (normierenF(n) == 0) { continue; }
                summe = summe + n;
            }
            (void)normierenF(summe);   // VectorNormalized
            in.at(j, i).normale = summe;
        }
    }
}

// PutMeshOnCurve (mesh.cpp)
void aufKurve(NetzF& in) {
    const auto lerp3 = [](const NPunkt& vor, NPunkt& mitte, const NPunkt& nach) {
        mitte.ort = (vor.ort + mitte.ort * 2.0F + nach.ort) * 0.25F;
        for (std::size_t k = 0; k < 2; ++k) { mitte.st[k] = (vor.st[k] + mitte.st[k] * 2.0F + nach.st[k]) * 0.25F; }   // "interpolating st coords"
    };
    for (int i = 0; i < in.breite; ++i) {
        for (int j = 1; j < in.hoehe; j += 2) { lerp3(in.at(j - 1, i), in.at(j, i), in.at(j + 1, i)); }
    }
    for (int j = 0; j < in.hoehe; ++j) {
        for (int i = 1; i < in.breite; i += 2) { lerp3(in.at(j, i - 1), in.at(j, i), in.at(j, i + 1)); }
    }
}

using Ausdehnung = std::vector<std::array<NPunkt, kAchseMax>>;
Ausdehnung ausdehnen(const NetzF& in) {
    Ausdehnung ex(kAchseMax);
    for (int h = 0; h < in.hoehe; ++h) {
        for (int w = 0; w < in.breite; ++w) { ex[static_cast<std::size_t>(h)][static_cast<std::size_t>(w)] = in.at(h, w); }
    }
    return ex;
}
NetzF zusammen(const Ausdehnung& ex, int breite, int hoehe) {   // bhed_mesh_from_rows
    NetzF o;
    o.breite = breite;
    o.hoehe = hoehe;
    o.v.resize(static_cast<std::size_t>(breite * hoehe));
    for (int i = 0; i < hoehe; ++i) {
        for (int w = 0; w < breite; ++w) { o.at(i, w) = ex[static_cast<std::size_t>(i)][static_cast<std::size_t>(w)]; }
    }
    return o;
}

// SubdivideMesh2 (mesh.cpp)
NetzF unterteileNetz(const NetzF& in, int iterationen) {
    Ausdehnung ex = ausdehnen(in);
    int breite = in.breite;
    int hoehe = in.hoehe;
    const auto E = [&ex](int zeile, int spalte) -> NPunkt& { return ex[static_cast<std::size_t>(zeile)][static_cast<std::size_t>(spalte)]; };
    for (; iterationen > 0; --iterationen) {
        for (int j = 0; j + 2 < breite; j += 4) {
            if (breite + 2 >= kAchseMax) { break; }
            breite += 2;
            for (int i = 0; i < hoehe; ++i) {
                for (int k = breite - 1; k > j + 3; --k) { E(i, k) = E(i, k - 2); }
                E(i, j + 3) = mitteNP(E(i, j + 1), E(i, j + 2));
                E(i, j + 1) = mitteNP(E(i, j + 0), E(i, j + 1));
                E(i, j + 2) = mitteNP(E(i, j + 1), E(i, j + 3));
            }
        }
        for (int j = 0; j + 2 < hoehe; j += 4) {
            if (hoehe + 2 >= kAchseMax) { break; }
            hoehe += 2;
            for (int i = 0; i < breite; ++i) {
                for (int k = hoehe - 1; k > j + 3; --k) { E(k, i) = E(k - 2, i); }
                E(j + 3, i) = mitteNP(E(j + 1, i), E(j + 2, i));
                E(j + 1, i) = mitteNP(E(j + 0, i), E(j + 1, i));
                E(j + 2, i) = mitteNP(E(j + 1, i), E(j + 3, i));
            }
        }
    }
    return zusammen(ex, breite, hoehe);
}

// ProjectPointOntoVector, RemoveLinearMeshColumnsRows (mesh.cpp)
V3d aufStrahl(const Vektor3f& punkt, const Vektor3f& start, const Vektor3f& ende) {
    const Vektor3f pv = punkt - start;
    Vektor3f vec = ende - start;
    (void)normierenF(vec);
    return d3(start + vec * punkt3(pv, vec));
}
NetzF ohneGeradeZeilen(const NetzF& in) {
    Ausdehnung ex = ausdehnen(in);
    int breite = in.breite;
    int hoehe = in.hoehe;
    const auto E = [&ex](int zeile, int spalte) -> NPunkt& { return ex[static_cast<std::size_t>(zeile)][static_cast<std::size_t>(spalte)]; };
    for (int j = 1; j < breite - 1; ++j) {
        double maxLaenge = 0;
        for (int i = 0; i < hoehe; ++i) {
            const V3d d = minusD(d3(E(i, j).ort), aufStrahl(E(i, j).ort, E(i, j - 1).ort, E(i, j + 1).ort));
            maxLaenge = std::max(maxLaenge, std::sqrt(punktD(d, d)));
        }
        if (maxLaenge < 0.1) {
            --breite;
            for (int i = 0; i < hoehe; ++i) {
                for (int k = j; k < breite; ++k) { E(i, k) = E(i, k + 1); }
            }
            --j;
        }
    }
    for (int j = 1; j < hoehe - 1; ++j) {
        double maxLaenge = 0;
        for (int i = 0; i < breite; ++i) {
            const V3d d = minusD(d3(E(j, i).ort), aufStrahl(E(j, i).ort, E(j - 1, i).ort, E(j + 1, i).ort));
            maxLaenge = std::max(maxLaenge, std::sqrt(punktD(d, d)));
        }
        if (maxLaenge < 0.1) {
            --hoehe;
            for (int i = 0; i < breite; ++i) {
                for (int k = j; k < hoehe; ++k) { E(k, i) = E(k + 1, i); }
            }
            --j;
        }
    }
    return zusammen(ex, breite, hoehe);
}

// ParsePatch: "delete and warn about degenerate patches"
bool patchEntartet(const std::vector<Vektor3f>& m) {
    const auto gleich = [](const Vektor3f& a, const Vektor3f& b) {   // VectorCompare (EQUAL_EPSILON 0.001)
        return std::fabs(a[0] - b[0]) < 0.001F && std::fabs(a[1] - b[1]) < 0.001F && std::fabs(a[2] - b[2]) < 0.001F;
    };
    Vektor3f delta(0, 0, 0);
    float laenge = 0;
    for (std::size_t i = 1; i < m.size() && laenge == 0; ++i) {
        delta = m[0] - m[i];
        laenge = normierenF(delta);
    }
    if (laenge == 0) { return true; }
    for (std::size_t i = 1; i < m.size(); ++i) {
        Vektor3f d2 = m[0] - m[i];
        if (normierenF(d2) != 0) {
            const Vektor3f d3n(-d2[0], -d2[1], -d2[2]);
            if (!gleich(delta, d2) && !gleich(delta, d3n)) { return false; }
        }
    }
    return true;
}

}  // namespace

EchtNetz echtPatchNetz(const std::vector<Vektor3f>& kontrolle, int breite, int hoehe, int unterteilungen) {
    EchtNetz aus;
    // MAX_PATCH_SIZE 32
    if (breite < 3 || hoehe < 3 || breite > 32 || hoehe > 32 || kontrolle.size() != static_cast<std::size_t>(breite * hoehe)) { return aus; }
    if (patchEntartet(kontrolle)) { return aus; }
    NetzF m;
    m.breite = breite;
    m.hoehe = hoehe;
    m.v.resize(kontrolle.size());
    for (std::size_t i = 0; i < kontrolle.size(); ++i) { m.v[i].ort = kontrolle[i]; }
    float laengste = 0;
    for (int j = 0; j + 2 < breite; j += 2) {
        for (int i = 0; i + 2 < hoehe; i += 2) {
            laengsteKurve(laengste, m.at(i, j).ort, m.at(i, j + 1).ort, m.at(i, j + 2).ort);   // row
            laengsteKurve(laengste, m.at(i, j).ort, m.at(i + 1, j).ort, m.at(i + 2, j).ort);   // col
        }
    }
    // DrawSurfaceForMesh: Normalen der Kontrollpunkte
    NetzF kopie = m;
    netzNormalen(kopie);
    for (std::size_t i = 0; i < m.v.size(); ++i) { m.v[i].normale = kopie.v[i].normale; }
    aufKurve(kopie);
    netzNormalen(kopie);
    for (std::size_t i = 0; i < m.v.size(); ++i) {
        if (punkt3(kopie.v[i].normale, m.v[i].normale) < 0.75F) { m.v[i].normale = kopie.v[i].normale; }
    }
    // TessellatedMesh( mesh, IterationsForCurve( longestCurve, patchSubdivisions ) )
    NetzF t = unterteileNetz(m, iterationenFuerKurve(laengste, std::max(1, unterteilungen)));
    aufKurve(t);
    t = ohneGeradeZeilen(t);
    aus.breite = t.breite;
    aus.hoehe = t.hoehe;
    for (const NPunkt& p : t.v) {
        aus.ort.push_back(p.ort);
        aus.normale.push_back(p.normale);
    }
    return aus;
}

bool echtPatchSteuernetz(std::vector<EchtNetzPunkt>& netz, int breite, int hoehe, float& kurve) {
    kurve = 0;
    if (breite < 3 || hoehe < 3 || breite > 32 || hoehe > 32 || netz.size() != static_cast<std::size_t>(breite * hoehe)) { return false; }
    std::vector<Vektor3f> orte;
    for (const EchtNetzPunkt& p : netz) { orte.push_back(p.ort); }
    if (patchEntartet(orte)) { return false; }
    NetzF m;
    m.breite = breite;
    m.hoehe = hoehe;
    m.v.resize(netz.size());
    for (std::size_t i = 0; i < netz.size(); ++i) {
        m.v[i].ort = netz[i].ort;
        m.v[i].st = netz[i].st;
    }
    for (int j = 0; j + 2 < breite; j += 2) {   // ParsePatch
        for (int i = 0; i + 2 < hoehe; i += 2) {
            laengsteKurve(kurve, m.at(i, j).ort, m.at(i, j + 1).ort, m.at(i, j + 2).ort);
            laengsteKurve(kurve, m.at(i, j).ort, m.at(i + 1, j).ort, m.at(i + 2, j).ort);
        }
    }
    NetzF kopie = m;   // DrawSurfaceForMesh
    netzNormalen(kopie);
    for (std::size_t i = 0; i < m.v.size(); ++i) { m.v[i].normale = kopie.v[i].normale; }
    aufKurve(kopie);
    netzNormalen(kopie);
    for (std::size_t i = 0; i < m.v.size(); ++i) {
        if (punkt3(kopie.v[i].normale, m.v[i].normale) < 0.75F) { m.v[i].normale = kopie.v[i].normale; }
    }
    // "construct a plane from the first vert": eben, wenn alle Normalen gleich (VectorCompare) und alle Punkte auf
    // der Ebene liegen (EQUAL_EPSILON); dann bekommt jeder Punkt die Normale der Ebene
    const Vektor3f n = m.v[0].normale;
    const float dist = punkt3(m.v[0].ort, n);
    const bool eben = std::none_of(m.v.begin(), m.v.end(), [&n, dist](const NPunkt& p) {
        const bool gleich = std::fabs(n[0] - p.normale[0]) < 0.001F && std::fabs(n[1] - p.normale[1]) < 0.001F && std::fabs(n[2] - p.normale[2]) < 0.001F;
        return !gleich || std::fabs(static_cast<double>(punkt3(n, p.ort)) - static_cast<double>(dist)) > 0.001;
    });
    for (std::size_t i = 0; i < netz.size(); ++i) { netz[i].normale = eben ? n : m.v[i].normale; }
    return true;
}

int echtIterationenFuerKurve(float laenge, int unterteilungen) { return iterationenFuerKurve(laenge, unterteilungen); }

void echtNetzTesselieren(std::vector<EchtNetzPunkt>& netz, int& breite, int& hoehe, int iterationen) {
    NetzF m;
    m.breite = breite;
    m.hoehe = hoehe;
    m.v.resize(netz.size());
    for (std::size_t i = 0; i < netz.size(); ++i) { m.v[i] = NPunkt{netz[i].ort, netz[i].normale, netz[i].st}; }
    NetzF t = unterteileNetz(m, iterationen);   // TessellatedMesh
    aufKurve(t);
    t = ohneGeradeZeilen(t);
    breite = t.breite;
    hoehe = t.hoehe;
    netz.clear();
    for (const NPunkt& p : t.v) { netz.push_back(EchtNetzPunkt{p.ort, p.normale, p.st}); }
}

void echtPatchFlaechenLichter(const EchtNetz& netz, const EchtFlaechenShader& s, std::vector<EchtLicht>& aus) {
    if (s.flaechenlicht <= 0 || netz.breite < 2 || netz.hoehe < 2) { return; }
    const auto P = [&netz](int i) { return netz.ort[static_cast<std::size_t>(i)]; };
    const auto N = [&netz](int i) { return netz.normale[static_cast<std::size_t>(i)]; };
    // MeshQuadIterator: Zeilen y, Spalten x, Ecken nach der Radix (x + y) & 1
    for (int y = 0; y < netz.hoehe - 1; ++y) {
        for (int x = 0; x < netz.breite - 1; ++x) {
            const int pw[5] = {x + (y * netz.breite), x + ((y + 1) * netz.breite), x + 1 + ((y + 1) * netz.breite), x + 1 + (y * netz.breite),
                               x + (y * netz.breite)};
            const int r = (x + y) & 1;
            const int q[4] = {pw[r + 0], pw[r + 1], pw[r + 2], pw[r + 3]};
            // PlaneFromPoints( plane, q0, q1, q2 ) && |plane3_distance_to_point( plane, q3 )| < PLANAR_EPSILON
            const V3d nk = kreuzD(d3(P(q[2]) - P(q[0])), d3(P(q[1]) - P(q[0])));
            Vektor3f n(static_cast<float>(nk[0]), static_cast<float>(nk[1]), static_cast<float>(nk[2]));
            bool eben = normierenF(n) != 0;
            if (eben) {
                const float dist = punkt3(P(q[0]), n);
                eben = std::fabs(punkt3(n, P(q[3])) - dist) < 0.1F;
            }
            if (eben) {
                echtFlaechenLichter({P(q[0]), P(q[1]), P(q[2]), P(q[3])}, std::vector<Vektor3f>{N(q[0]), N(q[1]), N(q[2]), N(q[3])}, s, aus);
            } else {
                echtFlaechenLichter({P(q[0]), P(q[1]), P(q[2])}, std::vector<Vektor3f>{N(q[0]), N(q[1]), N(q[2])}, s, aus);
                echtFlaechenLichter({P(q[0]), P(q[2]), P(q[3])}, std::vector<Vektor3f>{N(q[0]), N(q[2]), N(q[3])}, s, aus);
            }
        }
    }
}

// --- Stufe 9d: die Dreiecke von -meta (StripFaceSurface) ---------------------------------

std::vector<int> echtFlaechenStreifen(const std::vector<Vektor3f>& w, bool* mitte) {
    if (mitte != nullptr) { *mitte = false; }
    std::vector<int> idx;
    const int n = static_cast<int>(w.size());
    if (n < 3) { return idx; }
    if (n == 3) { return {0, 1, 2}; }
    // ydnar: find smallest coordinate
    int least = 0;
    for (int i = 0; i < n; ++i) {
        const Vektor3f& v1 = w[static_cast<std::size_t>(i)];
        const Vektor3f& v2 = w[static_cast<std::size_t>(least)];
        if (v1[0] < v2[0] || (v1[0] == v2[0] && v1[1] < v2[1]) || (v1[0] == v2[0] && v1[1] == v2[1] && v1[2] < v2[2])) { least = i; }
    }
    // triangle_min_angle_squared_sin, triangle_area2x (qmath.h); TINY_AREA 1.0
    const auto minWinkel = [](const Vektor3f& a, const Vektor3f& b, const Vektor3f& c) {
        const Vektor3f df[3] = {b - a, c - a, c - b};
        const double l[3] = {punktD(d3(df[0]), d3(df[0])), punktD(d3(df[1]), d3(df[1])), punktD(d3(df[2]), d3(df[2]))};
        const int mini = (l[0] < l[1]) ? ((l[0] < l[2]) ? 0 : 2) : ((l[1] < l[2]) ? 1 : 2);
        if (l[mini] == 0) { return 0.0; }
        const int minj = mini == 2 ? 0 : mini + 1;
        const int mink = minj == 2 ? 0 : minj + 1;
        const V3d x = kreuzD(d3(df[minj]), d3(df[mink]));
        return punktD(x, x) / (l[minj] * l[mink]);
    };
    const auto flaeche2 = [](const Vektor3f& a, const Vektor3f& b, const Vektor3f& c) {
        const V3d x = kreuzD(d3(b - a), d3(c - a));
        return std::sqrt(punktD(x, x));
    };
    const auto wrap = [n](int i) { return i < 0 ? i + n : i < n ? i : i - n; };
    for (int r = 0; r < n; ++r) {
        double flaeche = 1e300;
        double winkel = 1e300;
        idx.clear();
        // walk the winding in both directions (ohne T-Verbindungen: abwechselnd)
        int i = wrap(r + least);
        int j = wrap(i - 1);
        int k = 0;
        int swap = 0;
        for (;;) {
            swap ^= 1;
            if (swap != 0) {
                k = j;
                j = wrap(j - 1);
            } else {
                k = i;
                i = wrap(i + 1);
            }
            if (i == j) { break; }
            const Vektor3f& a = w[static_cast<std::size_t>(i)];
            const Vektor3f& b = w[static_cast<std::size_t>(j)];
            const Vektor3f& c = w[static_cast<std::size_t>(k)];
            winkel = std::min(winkel, minWinkel(a, b, c));
            flaeche = std::min(flaeche, flaeche2(a, b, c));
            if (!(winkel > 1e-5 && flaeche > 1.0)) { break; }
            idx.push_back(i);
            idx.push_back(j);
            idx.push_back(k);
        }
        if (winkel > 1e-5 && flaeche > 1.0) { return idx; }
    }
    // FanFaceSurface: der Mittelpunkt vorn, Faecher (Mitte, prev, next)
    if (mitte != nullptr) { *mitte = true; }
    idx.clear();
    for (int prev = n - 1, next = 0; next < n; prev = next++) {
        idx.push_back(n);
        idx.push_back(prev);
        idx.push_back(next);
    }
    return idx;
}

std::vector<EchtSonne::Probe> echtHimmelsProben(const std::vector<unsigned char>* const rgba[6], const int* breite, const int* hoehe) {
    std::vector<EchtSonne::Probe> r;
    for (int i = 0; i < 6; ++i) {
        if (rgba[i] == nullptr || breite[i] <= 0 || hoehe[i] <= 0 ||
            rgba[i]->size() < static_cast<std::size_t>(breite[i]) * static_cast<std::size_t>(hoehe[i]) * 4U) {
            return r;
        }
    }
    constexpr int res = 64;
    r.reserve(6 * res * res);
    for (int i = 0; i < 6; ++i) {
        const int bw = breite[i];
        const int bh = hoehe[i];
        for (int u = 0; u < res; ++u) {
            for (int v = 0; v < res; ++v) {
                Vektor3f n(1.0F, 1.0F - static_cast<float>(u) * 2.0F / res, 1.0F - static_cast<float>(v) * 2.0F / res);
                n = n / std::sqrt(punkt3(n, n));
                const int w0 = bw * u / res;
                const int w1 = std::max(w0 + 1, bw * (u + 1) / res);
                const int h0 = bh * v / res;
                const int h1 = std::max(h0 + 1, bh * (v + 1) / res);
                Vektor3f c(0, 0, 0);
                for (int x = w0; x < w1; ++x) {
                    for (int y = h0; y < h1; ++y) {
                        const std::size_t k = (static_cast<std::size_t>(y) * static_cast<std::size_t>(bw) + static_cast<std::size_t>(x)) * 4U;
                        c = c + Vektor3f((*rgba[i])[k], (*rgba[i])[k + 1], (*rgba[i])[k + 2]);
                    }
                }
                c = c / static_cast<float>((w1 - w0) * (h1 - h0));
                c = c * n[0];   // color *= dot( normal, g_vector3_axis_x )
                // postrotate everything from _rt (+x): lf (-1,-1,1), rt, ft z -90, bk z +90, up y -90, dn y +90
                Vektor3f d = n;
                switch (i) {
                case 0: d = Vektor3f(-n[0], -n[1], n[2]); break;
                case 1: break;
                case 2: d = Vektor3f(n[1], -n[0], n[2]); break;
                case 3: d = Vektor3f(-n[1], n[0], n[2]); break;
                case 4: d = Vektor3f(-n[2], n[1], n[0]); break;
                default: d = Vektor3f(n[2], n[1], -n[0]); break;
                }
                r.push_back({c / 255.0F, d});
            }
        }
    }
    return r;
}

void echtHimmelsLichter(const EchtSonne::Himmelslicht& h, const Vektor3f& farbe, std::vector<EchtLicht>& aus, const std::vector<EchtSonne::Probe>* proben) {
    if (h.wert <= 0 || h.schritte < 2 || h.horizontMin > h.horizontMax) { return; }
    const bool mitProben = proben != nullptr && !proben->empty() && h.farbeAusHimmel;
    const Vektor3f grenze = h.horizontMin >= 0 ? Vektor3f(0, 0, 1) : h.horizontMax <= 0 ? Vektor3f(0, 0, -1) : Vektor3f(0, 0, 0);
    const auto probeFarbe = [&](const Vektor3f& richtung) {   // SkyProbes::sampleColour
        Vektor3f c(0, 0, 0);
        double gewicht = 0;
        for (const EchtSonne::Probe& p : *proben) {
            const double d = punkt3(p.normale, richtung);
            if (d > 0 && punkt3(p.normale, grenze) >= 0) {
                c = c + p.farbe * static_cast<float>(d);
                gewicht += d;
            }
        }
        return gewicht != 0 ? c / static_cast<float>(gewicht) : c;
    };
    const std::size_t erste = aus.size();
    const bool unten = h.horizontMin == -90;
    const bool oben = h.horizontMax == 90;
    const int winkelSchritte = (h.schritte - 1) * 4;
    const float hoeheSchritt = 90.0F / static_cast<float>(h.schritte);
    const float pi = 3.14159265358979F;
    const float hoeheBogen = hoeheSchritt * pi / 180.0F;
    const float winkelBogen = (360.0F / static_cast<float>(winkelSchritte)) * pi / 180.0F;
    const float hMin = unten ? -90.0F + hoeheSchritt * 1.5F : static_cast<float>(h.horizontMin) + hoeheSchritt * 0.5F;
    const float hMax = oben ? 90.0F - hoeheSchritt * 1.5F : static_cast<float>(h.horizontMax) - hoeheSchritt * 0.5F;
    // float_to_integer( 1 + max( 0, ( eleMax - eleMin ) / eleStep ) ): rundet
    const int hoeheSchritte = static_cast<int>(std::lround(1.0F + std::max(0.0F, (hMax - hMin) / hoeheSchritt)));
    const int anzahl = winkelSchritte * hoeheSchritte + (unten ? 1 : 0) + (oben ? 1 : 0);
    const float photonen = h.wert / static_cast<float>(anzahl) * static_cast<float>(std::max(0.25, (h.horizontMax - h.horizontMin) / 90.0));
    const auto sonne = [&](const Vektor3f& richtung) {
        EchtLicht l;
        l.art = EchtLicht::Art::Sonne;
        l.farbe = mitProben ? probeFarbe(richtung) : farbe;
        l.photonen = photonen;
        l.sonnenRichtung = richtung;
        l.winkel = true;
        l.himmel = true;
        huelleSetzen(l);
        aus.push_back(l);
    };
    float hoehe = std::min(hMin, static_cast<float>(h.horizontMax)) * pi / 180.0F;
    float winkel = 0;
    for (int i = 0; i < hoeheSchritte; ++i) {
        for (int j = 0; j < winkelSchritte; ++j) {
            sonne(Vektor3f(std::cos(winkel) * std::cos(hoehe), std::sin(winkel) * std::cos(hoehe), std::sin(hoehe)));
            winkel += winkelBogen;
        }
        hoehe += hoeheBogen;
        winkel += winkelBogen / static_cast<float>(hoeheSchritte);
    }
    if (unten) { sonne(Vektor3f(0, 0, -1)); }
    if (oben) { sonne(Vektor3f(0, 0, 1)); }
    if (mitProben) {   // auf das Groesste aller Sonnen normieren
        float mx = 0;
        for (std::size_t i = erste; i < aus.size(); ++i) { mx = std::max({mx, aus[i].farbe[0], aus[i].farbe[1], aus[i].farbe[2]}); }
        if (mx != 0) {
            for (std::size_t i = erste; i < aus.size(); ++i) { aus[i].farbe = aus[i].farbe / mx; }
        }
    }
}

namespace {

// CreateSunLight (light.cpp): numSamples Sonnen, je photons / numSamples; die erste in der
// Richtung, die weiteren mit Abweichung (Zufall von q3map2, siehe EchtZufall)
void sonnenLichter(const EchtLicht& vorlage, const Vektor3f& richtung, float photonen, float abweichung, int proben, EchtZufall& zufall,
                   std::vector<EchtLicht>& aus) {
    proben = std::max(proben, 1);
    static int gruppen = 0;   // die Proben einer Sonne (sun_t) gehoeren zusammen
    const int gruppe = ++gruppen;
    for (int i = 0; i < proben; ++i) {
        Vektor3f r = richtung;
        if (i != 0) {
            const double d = std::sqrt(static_cast<double>(richtung[0]) * richtung[0] + static_cast<double>(richtung[1]) * richtung[1]);
            double winkel = std::atan2(static_cast<double>(richtung[1]), static_cast<double>(richtung[0]));
            double hoehe = std::atan2(static_cast<double>(richtung[2]), d);
            float da = 0;
            float de = 0;
            do {
                da = (zufall.naechste() * 2.0F - 1.0F) * abweichung;
                de = (zufall.naechste() * 2.0F - 1.0F) * abweichung;
            } while ((da * da + de * de) > (abweichung * abweichung));
            winkel += da;
            hoehe += de;
            r = Vektor3f(static_cast<float>(std::cos(winkel) * std::cos(hoehe)), static_cast<float>(std::sin(winkel) * std::cos(hoehe)),
                         static_cast<float>(std::sin(hoehe)));
        }
        EchtLicht l = vorlage;
        l.art = EchtLicht::Art::Sonne;
        l.sonnenRichtung = r;
        l.photonen = photonen / static_cast<float>(proben);
        l.winkel = true;
        l.sonnenGruppe = gruppe;
        l.sonnenProbe = i;
        huelleSetzen(l);
        aus.push_back(l);
    }
}

// Mitte der Grenzen (info.minmax.origin())
Vektor3f grenzenMitte(const std::vector<Vektor3f>& p) {
    Vektor3f mn = p.front();
    Vektor3f mx = p.front();
    for (const Vektor3f& v : p) {
        for (int i = 0; i < 3; ++i) {
            mn[i] = std::min(mn[i], v[i]);
            mx[i] = std::max(mx[i], v[i]);
        }
    }
    return (mn + mx) * 0.5F;
}

// CreateSurfaceLights: "autosprite shaders become point lights"
void autospriteLicht(const EchtFlaechenShader& s, const std::vector<Vektor3f>& punkte, const Entitaet* quelle, std::vector<EchtLicht>& aus) {
    if (punkte.empty()) { return; }
    EchtLicht l;
    l.art = EchtLicht::Art::Punkt;
    l.photonen = s.flaechenlicht * kEchtPunktSkala;
    l.fade = 1;
    l.ursprung = grenzenMitte(punkte);
    l.farbe = normiert(s.farbe);
    l.winkel = true;
    l.quelle = quelle;
    huelleSetzen(l);
    aus.push_back(l);
}

// GetEntityShadowFlags: "_castShadows", "_cs" (atoi); nicht da: vorgabe
int wirftSchatten(const Entitaet& e, int vorgabe) {
    for (const char* k : {"_castShadows", "_cs"}) {
        const std::string v = wertQ3(e, k);
        if (!v.empty()) { return std::atoi(v.c_str()); }
    }
    return vorgabe;
}

}  // namespace

void echtModellFlaechen(const Karte& k, const std::function<void(const EchtModellFlaeche&)>& je) {
    if (k.kontext.modelle == nullptr || !je) { return; }
    const auto welt = k.welt();
    for (const auto& ep : k.entitaeten()) {
        const Entitaet& e = *ep;
        if (wertQ3(e, "classname") != "misc_model") { continue; }
        // AddTriangleModels: ohne "_target"/"target" in der Welt, sonst in der Pinsel-Entitaet mit diesem
        // "_targetname"/"targetname" (gibt es keine, kommt das Modell nirgends hin)
        std::string ziel = wertQ3(e, "_target");
        if (ziel.empty()) { ziel = wertQ3(e, "target"); }
        const Entitaet* eltern = nullptr;
        if (!ziel.empty()) {
            for (const auto& b : k.entitaeten()) {
                if (b == welt || b.get() == &e || b->kinder().empty()) { continue; }
                std::string tn = wertQ3(*b, "_targetname");
                if (tn.empty()) { tn = wertQ3(*b, "targetname"); }
                if (tn == ziel) {
                    eltern = b.get();
                    break;
                }
            }
            if (eltern == nullptr) { continue; }
        }
        if (wertQ3(e, "model").empty()) { continue; }
        const ModellAnsicht a = k.kontext.modelle->ansicht(e);
        if (!a.modell || a.modell->null || a.modell->flaechen.empty()) { continue; }
        // ParseEntityCompileParams: in der Welt castShadows 1 (WORLDSPAWN_CAST_SHADOWS), sonst 0; die Schluessel
        // des Modells, dann die der Eltern. -light nimmt Flaechen einer Pinsel-Entitaet nur, wenn diese selbst
        // _castShadows hat (PopulateTraceNodes, ENTITY_CAST_SHADOWS 0).
        int wirft = 0;
        if (eltern == nullptr) {
            wirft = wirftSchatten(e, welt ? wirftSchatten(*welt, 1) : 1);
        } else if (wirftSchatten(*eltern, 0) > 0) {
            wirft = wirftSchatten(e, wirftSchatten(*eltern, 0));
        }
        const Matrix4 m = miscModellMatrix(e);
        EchtModellFlaeche f;
        f.quelle = &e;
        f.inWelt = eltern == nullptr;
        f.wirft = wirft;
        for (std::size_t i = 0; i < a.modell->flaechen.size() && i < a.shader.size(); ++i) {
            if (a.shader[i].empty()) { continue; }   // die Skin bildet die Flaeche nicht ab
            const ModellFlaeche& mf = a.modell->flaechen[i];
            f.shader = a.shader[i];
            f.ort.clear();
            f.normale.clear();
            f.st.clear();
            for (std::size_t j = 0; j + 2 < mf.indizes.size(); j += 3) {
                for (std::size_t c = 0; c < 3; ++c) {
                    const std::uint32_t ix = mf.indizes[j + c];
                    if (ix >= mf.punkte.size()) { break; }
                    const ModellPunkt& p = mf.punkte[ix];
                    f.ort.push_back(transformierterPunkt(m, p.ort));
                    f.normale.push_back(transformierteNormale(m, p.normale));
                    f.st.push_back(p.st);
                }
                while (f.ort.size() % 3 != 0) {   // ungueltiger Index: das Dreieck weg
                    f.ort.pop_back();
                    f.normale.pop_back();
                    f.st.pop_back();
                }
            }
            if (!f.ort.empty()) { je(f); }
        }
    }
}

// --- Stufe 9d: Nebel ---------------------------------------------------------------------

std::vector<EchtNebel> echtNebelSammeln(const Karte& k, const std::function<EchtNebelShader(const std::string&)>& shader) {
    std::vector<EchtNebel> aus;
    if (!shader) { return aus; }
    // die Pinsel, die Blaetter fuellen (visibleHull: was davon im Freien liegt, ist sichtbar)
    std::vector<const Pinsel*> deckende;
    for (const auto& ep : k.entitaeten()) {
        for (const auto& prim : ep->kinder()) {
            if (prim->art() != Primitiv::Art::Pinsel) { continue; }
            const auto& p = static_cast<const Pinsel&>(*prim);
            bool deckend = !p.flaechen().empty();
            for (const auto& fl : p.flaechen()) {
                const EchtNebelShader s = shader(fl->shader());
                if (s.nebel || !s.deckend) {
                    deckend = false;
                    break;
                }
            }
            if (deckend) { deckende.push_back(&p); }
        }
    }
    const auto inPinsel = [](const Pinsel& p, const Vektor3f& q) {
        for (const auto& fl : p.flaechen()) {
            const Vektor3d& n = fl->ebene().normal();
            if (n[0] * q[0] + n[1] * q[1] + n[2] * q[2] - fl->ebene().dist() > -0.01) { return false; }
        }
        return true;
    };
    for (const auto& ep : k.entitaeten()) {
        for (const auto& prim : ep->kinder()) {
            if (prim->art() != Primitiv::Art::Pinsel) { continue; }
            const auto& p = static_cast<const Pinsel&>(*prim);
            std::string nebelShader;
            EchtNebelShader ns;
            for (const auto& fl : p.flaechen()) {
                const EchtNebelShader s = shader(fl->shader());
                if (s.nebel) {
                    nebelShader = fl->shader();
                    ns = s;
                    break;
                }
            }
            if (nebelShader.empty()) { continue; }
            EchtNebel n;
            n.shader = nebelShader;
            n.quelle = ep.get();
            n.farbe = ns.farbe;
            n.tcScale = 1.0F / (std::max(ns.tiefe, 1.0F) * 8.0F);
            std::vector<const Flaeche*> seiten;
            for (const auto& fl : p.flaechen()) {
                if (!fl->traegtBei()) { continue; }
                const Vektor3d& nn = fl->ebene().normal();
                n.ebenen.push_back(static_cast<float>(nn[0]));
                n.ebenen.push_back(static_cast<float>(nn[1]));
                n.ebenen.push_back(static_cast<float>(nn[2]));
                n.ebenen.push_back(static_cast<float>(fl->ebene().dist()));
                seiten.push_back(fl.get());
            }
            if (seiten.size() < 4) { continue; }
            // sichtbar (visibleHull nicht leer): ein Teil der Seite liegt nicht in einem deckenden Pinsel -
            // geprueft an der Mitte und den zur Mitte gerueckten Ecken, je 1 Einheit vor der Seite
            const auto sichtbar = [&](const Flaeche& fl) {
                const Wicklung& wk = fl.wicklung();
                if (wk.numpoints < 3) { return false; }
                if (shader(fl.shader()).nichtZeichnen) { return false; }
                Vektor3f mitte(0, 0, 0);
                for (std::size_t i = 0; i < wk.numpoints; ++i) { mitte = mitte + Vektor3f(wk[i].vertex); }
                mitte = mitte / static_cast<float>(wk.numpoints);
                const Vektor3d& nd = fl.ebene().normal();
                const Vektor3f nf(static_cast<float>(nd[0]), static_cast<float>(nd[1]), static_cast<float>(nd[2]));
                std::vector<Vektor3f> proben = {mitte};
                for (std::size_t i = 0; i < wk.numpoints; ++i) { proben.push_back(mitte + (Vektor3f(wk[i].vertex) - mitte) * 0.9F); }
                for (const Vektor3f& q0 : proben) {
                    const Vektor3f q = q0 + nf;
                    bool zu = false;
                    for (const Pinsel* d : deckende) {
                        if (d != &p && inPinsel(*d, q)) {
                            zu = true;
                            break;
                        }
                    }
                    if (!zu) { return true; }
                }
                return false;
            };
            int sicht = -1;
            const Vektor3f fd = ns.richtung;
            if (fd[0] != 0 || fd[1] != 0 || fd[2] != 0) {   // "if shader specifies an explicit direction"
                double besterDot = 0;
                for (std::size_t j = 0; j < seiten.size(); ++j) {
                    const Vektor3d& nn = seiten[j]->ebene().normal();
                    const double d = fd[0] * nn[0] + fd[1] * nn[1] + fd[2] * nn[2];
                    if (d < besterDot) {
                        besterDot = d;
                        sicht = static_cast<int>(j);
                    }
                }
            }
            if (sicht < 0) {
                // EmitFogs: die achsparallelen Seiten in der Reihenfolge von q3map2 (-X +X -Y +Y -Z +Z), rueckwaerts
                std::vector<bool> achsig(seiten.size(), false);
                for (int j = 5; j >= 0 && sicht < 0; --j) {
                    const int achse = j / 2;
                    const double richtung = (j % 2) != 0 ? 1.0 : -1.0;
                    for (std::size_t s = 0; s < seiten.size(); ++s) {
                        const Vektor3d& nn = seiten[s]->ebene().normal();
                        if (std::fabs(nn[static_cast<std::size_t>(achse)] - richtung) > 1e-6) { continue; }
                        achsig[s] = true;
                        if (sichtbar(*seiten[s])) { sicht = static_cast<int>(s); }
                        break;
                    }
                }
                for (std::size_t s = 0; s < seiten.size() && sicht < 0; ++s) {   // "try other sides"
                    if (!achsig[s] && sichtbar(*seiten[s])) { sicht = static_cast<int>(s); }
                }
            }
            if (sicht >= 0) {
                const Vektor3d& nn = seiten[static_cast<std::size_t>(sicht)]->ebene().normal();
                n.hatFlaeche = true;
                n.ebene[0] = static_cast<float>(-nn[0]);
                n.ebene[1] = static_cast<float>(-nn[1]);
                n.ebene[2] = static_cast<float>(-nn[2]);
                n.ebene[3] = static_cast<float>(seiten[static_cast<std::size_t>(sicht)]->ebene().dist());
            }
            aus.push_back(std::move(n));
        }
    }
    // ydnar: global fog (worldspawn "_fog" / "fog")
    if (const auto welt = k.welt()) {
        std::string g = wertQ3(*welt, "_fog");
        if (g.empty()) { g = wertQ3(*welt, "fog"); }
        if (!g.empty()) {
            const EchtNebelShader s = shader(g);
            EchtNebel n;
            n.shader = g;
            n.global = true;
            n.quelle = welt.get();
            if (s.nebel) {
                n.farbe = s.farbe;
                n.tcScale = 1.0F / (std::max(s.tiefe, 1.0F) * 8.0F);
            } else {   // R_LoadFogs: "bad shader" - Rot, Tiefe 250
                n.farbe = Vektor3f(1, 0, 0);
                n.tcScale = 1.0F / (250.0F * 8.0F);
            }
            aus.push_back(std::move(n));
        }
    }
    return aus;
}

float echtNebelDichte(const EchtNebel& n, const Vektor3f& punkt, const Vektor3f& auge, const Vektor3f& vorn) {
    // RB_CalcFogTexCoords: s = (Abstand entlang der Blickrichtung) * tcScale + 1/512
    float s = (punkt3(punkt, vorn) - punkt3(auge, vorn)) * n.tcScale + 1.0F / 512.0F;
    float t = 1.0F;
    float augeT = 1.0F;
    if (n.hatFlaeche) {
        t = punkt[0] * n.ebene[0] + punkt[1] * n.ebene[1] + punkt[2] * n.ebene[2] + n.ebene[3];
        augeT = auge[0] * n.ebene[0] + auge[1] * n.ebene[1] + auge[2] * n.ebene[2] + n.ebene[3];
    }
    if (augeT < 0) {   // eye_outside
        t = t < 1.0F ? 1.0F / 32.0F : 1.0F / 32.0F + 30.0F / 32.0F * t / (t - augeT);
    } else {
        t = t < 0 ? 1.0F / 32.0F : 31.0F / 32.0F;
    }
    // R_FogFactor mit tr.fogTable (sqrt)
    s -= 1.0F / 512.0F;
    if (s < 0 || t < 1.0F / 32.0F) { return 0; }
    if (t < 31.0F / 32.0F) { s *= (t - 1.0F / 32.0F) / (30.0F / 32.0F); }
    s = std::min(s * 8.0F, 1.0F);
    return std::sqrt(s);   // tr.fogTable: pow( i / 255, 0.5 ) - wie die Kartenvorschau ohne Tabellenstufen
}

EchtLichtWelt echtLichterSammeln(const Karte& k, const std::function<EchtFlaechenShader(const std::string&)>& flaechenShader,
                                 const std::function<EchtSonne(const std::string&)>& sonne) {
    EchtLichtWelt w;
    EchtZufall zufall;   // Stufe 9d: srand( 0 ) - erst die Jitter-Kopien der Lichter, dann die Sonnen
    const auto welt = k.welt();
    if (welt) {
        // LightWorld (light.cpp): _color, _ambient, _minlight
        float c[3] = {1, 1, 1};
        Vektor3f farbe(1, 1, 1);
        if (zahlen(wertQ3(*welt, "_color"), c, 3) && (c[0] != 0 || c[1] != 0 || c[2] != 0)) { farbe = Vektor3f(c[0], c[1], c[2]); }
        w.umgebung = farbe * zahl(*welt, "_ambient", "ambient");
        w.minimum = farbe * zahl(*welt, "_minlight");
        // rc608 Teil 11: _minvertexlight (light.cpp:2333), sonst gilt _minlight auch fuer die Ecken (:2347)
        w.minimumEcken = wertQ3(*welt, "_minvertexlight").empty() ? w.minimum : farbe * zahl(*welt, "_minvertexlight");
    }
    // rc608 Teil 11: _noshadersun laesst die Sonnen aller Shader weg, q3map_skyLight bleibt (CreateSurfaceLights,
    // light.cpp:593; boolForKey: der Wert beginnt mit '1', bspfile_abstract.cpp:570)
    const bool keineShaderSonne = welt && !wertQ3(*welt, "_noshadersun").empty() && wertQ3(*welt, "_noshadersun")[0] == '1';
    // targetname -> Entitaet (FindTargetEntity)
    const auto ziel = [&k](const std::string& name) -> const Entitaet* {
        for (const auto& e : k.entitaeten()) {
            if (wertQ3(*e, "targetname") == name) { return e.get(); }
        }
        return nullptr;
    };
    for (const auto& ep : k.entitaeten()) {
        const Entitaet& e = *ep;
        const std::string klasse = wertQ3(e, "classname");
        // CreateEntityLights: "lightJunior" nur fuer das Gitter, sonst Praefix "light"
        if (!klassePraefix(klasse, "light") || klassePraefix(klasse, "lightjunior")) { continue; }
        EchtLicht l;
        l.quelle = &e;
        const int flags = static_cast<int>(zahl(e, "spawnflags"));
        l.linear = (flags & 1) != 0;
        l.winkel = (flags & 3) == 0;   // 1: linear ohne Winkel, 2: ohne Winkel
        l.fade = l.linear ? zahl(e, "fade") : 1.0F;
        if (l.fade == 0) { l.fade = 1; }
        l.winkelSkala = zahl(e, "_anglescale");
        if (l.winkelSkala != 0) { l.winkel = true; }
        float o[3] = {0, 0, 0};
        (void)zahlen(wertQ3(e, "origin"), o, 3);
        l.ursprung = Vektor3f(o[0], o[1], o[2]);
        l.stil = static_cast<int>(zahl(e, "_style", "style"));
        float staerke = zahl(e, "_light", "light", 300.0F);
        if (staerke == 0) { staerke = 300.0F; }
        if (const float s = zahl(e, "scale"); s != 0) { staerke *= s; }
        // ydnar: get deviance and samples (Stufe 9d: mit den Jitter-Kopien wie q3map2)
        float abweichung = zahl(e, "_deviance", "_deviation");
        if (wertQ3(e, "_deviance").empty() && wertQ3(e, "_deviation").empty()) { abweichung = zahl(e, "_jitter"); }
        abweichung = std::max(0.0F, abweichung);
        const int proben = std::max(1, std::atoi(wertQ3(e, "_samples").c_str()));
        staerke /= static_cast<float>(proben);
        float f[3];
        if (zahlen(wertQ3(e, "_color"), f, 3)) {
            l.farbe = (flags & 32) != 0 ? Vektor3f(f[0], f[1], f[2]) : normiert(Vektor3f(f[0], f[1], f[2]));
        }
        l.extraAbstand = zahl(e, "_extradist");
        l.photonen = staerke;
        if (staerke < 0) {
            l.negativ = true;   // SetupEnvelopes: negative Lichter
            l.photonen = -l.photonen;
        }
        const std::string zielName = wertQ3(e, "target");
        const Entitaet* z = zielName.empty() ? nullptr : ziel(zielName);
        if (z != nullptr) {
            float zo[3] = {0, 0, 0};
            (void)zahlen(wertQ3(*z, "origin"), zo, 3);
            Vektor3f n = Vektor3f(zo[0], zo[1], zo[2]) - l.ursprung;
            float abstand = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
            if (abstand > 0) { n = n / abstand; }
            float radius = zahl(e, "radius");
            if (radius == 0) { radius = 64; }
            if (abstand == 0) { abstand = 64; }
            l.normale = n;
            l.radiusJeAbstand = (radius + 16) / abstand;
            l.art = EchtLicht::Art::Spot;
            l.linear = false;   // Spotlichter: immer nicht linear, mit Winkel
            l.winkel = true;
            l.fade = 1;
            if (zahl(e, "_sun") != 0) {
                // CreateSunLight: Richtung = -normal, photons = intensity (schon durch numSamples), numSamples Sonnen
                // mit deviance in Grad
                sonnenLichter(l, Vektor3f(-n[0], -n[1], -n[2]), staerke, abweichung * 3.14159265358979F / 180.0F, proben, zufall, w.lichter);
                continue;
            } else {
                l.photonen *= kEchtSpotSkala;
            }
        } else {
            l.photonen *= kEchtPunktSkala;
        }
        huelleSetzen(l);
        w.lichter.push_back(l);
        // jitter the light (CreateEntityLights)
        for (int j = 1; j < proben; ++j) {
            EchtLicht l2 = l;
            l2.ursprung[0] = l.ursprung[0] + (zufall.naechste() * 2.0F - 1.0F) * abweichung;
            l2.ursprung[1] = l.ursprung[1] + (zufall.naechste() * 2.0F - 1.0F) * abweichung;
            l2.ursprung[2] = l.ursprung[2] + (zufall.naechste() * 2.0F - 1.0F) * abweichung;
            w.lichter.push_back(l2);
        }
    }
    // Himmel (q3map_sun) und Flaechenlicht (q3map_surfacelight) aus den Pinselflaechen
    std::vector<std::string> sonnen;
    const auto himmelPruefen = [&](const std::string& sh) {
        if (!sonne || std::find(sonnen.begin(), sonnen.end(), sh) != sonnen.end()) { return; }
        const EchtSonne s = sonne(sh);
        if (s.da || !s.himmel.empty()) { sonnen.push_back(sh); }
        std::vector<EchtSonne::Sonne> liste = s.sonnen;
        if (s.da && liste.empty()) { liste.push_back({s.farbe, s.staerke, s.grad, s.hoehe, 0.0F, 1}); }
        if (keineShaderSonne) { liste.clear(); }
        for (const EchtSonne::Sonne& so : liste) {
            // CreateSunLight mit q3map_sunExt <deviance> <samples> (Stufe 9d)
            EchtLicht l;
            l.farbe = normiert(so.farbe);
            sonnenLichter(l, kugelRichtung(so.grad, so.hoehe), so.staerke, so.abweichung, so.proben, zufall, w.lichter);
        }
        for (const EchtSonne::Himmelslicht& h : s.himmel) { echtHimmelsLichter(h, normiert(s.shaderFarbe), w.lichter, &s.proben); }
    };
    // die Dreiecke der .bsp laufen im Uhrzeigersinn von vorn (die Rechte-Hand-Normale zeigt nach
    // hinten) - sonst wird PointToPolygonFormFactor vor der Flaeche negativ
    const auto vonVorn = [](std::vector<Vektor3f>& d, const Vektor3f& n, std::vector<Vektor3f>* nd = nullptr) {
        const Vektor3f u = d[1] - d[0];
        const Vektor3f v = d[2] - d[0];
        const Vektor3f rh(u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]);
        if (punkt3(rh, n) > 0) {
            std::swap(d[1], d[2]);
            if (nd != nullptr) { std::swap((*nd)[1], (*nd)[2]); }
        }
    };
    for (const auto& ep : k.entitaeten()) {
        for (const auto& prim : ep->kinder()) {
            if (prim->art() == Primitiv::Art::Patch) {
                // Stufe 9d: RadLightForPatch mit dem Netz von TessellatedMesh
                const auto& patch = static_cast<const Patch&>(*prim);
                himmelPruefen(patch.shader());
                if (!flaechenShader) { continue; }
                const EchtFlaechenShader fs = flaechenShader(patch.shader());
                if (fs.flaechenlicht <= 0) { continue; }
                std::vector<Vektor3f> kontrolle;
                for (const PatchPunkt& pp : patch.punkte()) { kontrolle.push_back(pp.vertex); }
                const std::size_t vorher = w.lichter.size();
                if (fs.autosprite) {
                    autospriteLicht(fs, kontrolle, ep.get(), w.lichter);
                } else {
                    echtPatchFlaechenLichter(echtPatchNetz(kontrolle, static_cast<int>(patch.breite()), static_cast<int>(patch.hoehe())), fs, w.lichter);
                }
                for (std::size_t i = vorher; i < w.lichter.size(); ++i) { w.lichter[i].quelle = ep.get(); }
                continue;
            }
            if (prim->art() != Primitiv::Art::Pinsel) { continue; }
            const auto& pinsel = static_cast<const Pinsel&>(*prim);
            for (const auto& fl : pinsel.flaechen()) {
                if (!fl->traegtBei()) { continue; }
                const std::string& sh = fl->shader();
                himmelPruefen(sh);
                if (!flaechenShader) { continue; }
                const EchtFlaechenShader fs = flaechenShader(sh);
                if (fs.flaechenlicht <= 0) { continue; }
                const Wicklung& wk = fl->wicklung();
                if (wk.numpoints < 3) { continue; }
                const Vektor3d& nd = fl->ebene().normal();
                const Vektor3f n(static_cast<float>(nd[0]), static_cast<float>(nd[1]), static_cast<float>(nd[2]));
                const std::size_t vorher = w.lichter.size();
                std::vector<Vektor3f> ecken;
                for (std::size_t i = 0; i < wk.numpoints; ++i) { ecken.push_back(Vektor3f(wk[i].vertex)); }
                if (fs.autosprite) {
                    autospriteLicht(fs, ecken, ep.get(), w.lichter);
                } else {
                    // RadLightForTriangles: die Dreiecke von -meta (Stufe 9d: StripFaceSurface statt Faecher),
                    // jedes unterteilt nach q3map_lightSubdivide
                    bool mitte = false;
                    const std::vector<int> idx = echtFlaechenStreifen(ecken, &mitte);
                    if (mitte) {   // FanFaceSurface: der Mittelpunkt (Mittel der Ecken in double)
                        double c[3] = {0, 0, 0};
                        for (const Vektor3f& v : ecken) {
                            for (int a = 0; a < 3; ++a) { c[a] += v[static_cast<std::size_t>(a)]; }
                        }
                        const double iv = 1.0 / static_cast<double>(ecken.size());
                        ecken.push_back(Vektor3f(static_cast<float>(c[0] * iv), static_cast<float>(c[1] * iv), static_cast<float>(c[2] * iv)));
                    }
                    for (std::size_t i = 0; i + 2 < idx.size(); i += 3) {
                        std::vector<Vektor3f> dreieck = {ecken[static_cast<std::size_t>(idx[i])], ecken[static_cast<std::size_t>(idx[i + 1])],
                                                         ecken[static_cast<std::size_t>(idx[i + 2])]};
                        vonVorn(dreieck, n);
                        echtFlaechenLichter(dreieck, n, fs, w.lichter);
                    }
                }
                for (std::size_t i = vorher; i < w.lichter.size(); ++i) { w.lichter[i].quelle = ep.get(); }
            }
        }
    }
    // Stufe 9d: die Flaechen der misc_model (RadLightForTriangles ueber ihre Dreiecke, Normale je Ecke)
    if (sonne || flaechenShader) {
        echtModellFlaechen(k, [&](const EchtModellFlaeche& f) {
            himmelPruefen(f.shader);
            if (!flaechenShader) { return; }
            const EchtFlaechenShader fs = flaechenShader(f.shader);
            if (fs.flaechenlicht <= 0) { return; }
            const std::size_t vorher = w.lichter.size();
            if (fs.autosprite) {
                autospriteLicht(fs, f.ort, f.quelle, w.lichter);
            } else {
                for (std::size_t i = 0; i + 2 < f.ort.size(); i += 3) {
                    std::vector<Vektor3f> d = {f.ort[i], f.ort[i + 1], f.ort[i + 2]};
                    std::vector<Vektor3f> nd = {f.normale[i], f.normale[i + 1], f.normale[i + 2]};
                    Vektor3f nm = nd[0] + nd[1] + nd[2];
                    const float l = std::sqrt(punkt3(nm, nm));
                    if (l == 0) { continue; }
                    vonVorn(d, nm / l, &nd);
                    echtFlaechenLichter(d, nd, fs, w.lichter);
                }
            }
            for (std::size_t i = vorher; i < w.lichter.size(); ++i) { w.lichter[i].quelle = f.quelle; }
        });
    }
    return w;
}

Vektor3f echtLichtBeitrag(const EchtLicht& l, const Vektor3f& punkt, const Vektor3f& normale) {
    const auto dot = [](const Vektor3f& a, const Vektor3f& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; };
    if (l.art == EchtLicht::Art::Sonne) {
        float winkel = l.winkel ? dot(normale, l.sonnenRichtung) : 1.0F;
        const float add = l.photonen * winkel;
        if (add <= 0) { return Vektor3f(0, 0, 0); }
        return l.farbe * add;
    }
    if (l.art == EchtLicht::Art::Flaeche && !l.wicklung.empty()) {
        // exact point to polygon form factor (light.cpp:822-937 ohne -faster)
        const float ebene = dot(l.ursprung, l.normale);   // light->dist
        const float d = dot(punkt, l.normale) - ebene;
        if (d < 3.0F) {
            if (!l.zweiseitig && d < -1) { return Vektor3f(0, 0, 0); }
            if (d > -3.0F && dot(normale, l.normale) > 0.9F) { return Vektor3f(0, 0, 0); }
        }
        const Vektor3f gedrueckt = (d > -8.0F && d < 8.0F) ? punkt + l.normale * (8.0F - d) : punkt;
        const Vektor3f zum = l.ursprung - punkt;
        if (std::sqrt(dot(zum, zum)) >= l.huelle) { return Vektor3f(0, 0, 0); }
        float faktor = echtFormFaktor(gedrueckt, normale, l.wicklung);
        if (faktor == 0) { return Vektor3f(0, 0, 0); }
        if (faktor < 0) {
            if (!l.zweiseitig) { return Vektor3f(0, 0, 0); }
            faktor = -faktor;
            const Vektor3f z2 = (l.ursprung - l.normale * 2.0F) - punkt;
            if (std::sqrt(dot(z2, z2)) >= l.huelle) { return Vektor3f(0, 0, 0); }
        }
        const float add = faktor * l.add;
        if (add <= kEchtAbfallGrenze) { return Vektor3f(0, 0, 0); }
        return l.farbe * add;
    }
    // hinter der Flaeche (MrE): vector3_dot( light->origin, normal ) - vector3_dot( origin, normal ) < 0
    if (dot(l.ursprung, normale) - dot(punkt, normale) < 0) { return Vektor3f(0, 0, 0); }
    Vektor3f d = l.ursprung - punkt;
    float abstand = std::sqrt(dot(d, d));
    if (abstand >= l.huelle) { return Vektor3f(0, 0, 0); }
    const Vektor3f richtung = abstand > 0 ? d / abstand : Vektor3f(0, 0, 1);
    abstand = std::max(16.0F, std::sqrt(abstand * abstand + l.extraAbstand * l.extraAbstand));
    float winkel = 1.0F;
    if (l.winkel) { winkel = dot(normale, richtung); }
    if (l.winkelSkala != 0) {
        winkel /= l.winkelSkala;
        winkel = std::min(winkel, 1.0F);
    }
    float add = 0;
    if (l.art == EchtLicht::Art::Flaeche) {
        // "faster"-Naeherung der Flaechenlichter (light.cpp:853-881)
        winkel *= -dot(l.normale, richtung);
        if (winkel <= 0) { return Vektor3f(0, 0, 0); }
        add = l.photonen / (abstand * abstand) * winkel;
    } else if (l.linear) {
        add = std::max(0.0F, winkel * l.photonen * kEchtLinearSkala - (abstand * l.fade));
    } else {
        add = std::max(0.0F, (l.photonen / (abstand * abstand)) * winkel);
    }
    if (l.art == EchtLicht::Art::Spot) {
        const float entlang = dot(punkt - l.ursprung, l.normale);   // -dot( displacement, normal )
        if (entlang < 0) { return Vektor3f(0, 0, 0); }
        const Vektor3f amAbstand = l.ursprung + l.normale * entlang;
        const float radiusDort = l.radiusJeAbstand * entlang;
        const Vektor3f seitlich = punkt - amAbstand;
        const float r = std::sqrt(dot(seitlich, seitlich));
        if (r >= radiusDort) { return Vektor3f(0, 0, 0); }
        if (r > radiusDort - 32.0F) { add = std::max(0.0F, add * ((radiusDort - r) / 32.0F)); }
    }
    // FastActual: zu schwach -> nichts (light.cpp:1131)
    if (add <= kEchtAbfallGrenze) { return Vektor3f(0, 0, 0); }
    return l.farbe * (l.negativ ? -add : add);
}

Vektor3f echtLuxel(const EchtLichtWelt& w, const Vektor3f& punkt, const Vektor3f& normale) {
    Vektor3f s = w.umgebung;
    for (const EchtLicht& l : w.lichter) { s = s + echtLichtBeitrag(l, punkt, normale); }
    for (int i = 0; i < 3; ++i) { s[i] = std::max(s[i], w.minimum[i]); }
    return s;
}

}  // namespace bhed::karte
