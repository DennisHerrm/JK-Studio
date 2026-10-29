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
#include "bhed/karte/pinsel.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>

namespace bhed::karte {

namespace {

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
        const std::string w = e.wert(k);
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
void wicklungTeilen(const std::vector<Vektor3f>& w, int achse, float dist, std::vector<Vektor3f>& vorn, std::vector<Vektor3f>& hinten) {
    vorn.clear();
    hinten.clear();
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
        return;
    }
    if (zahl[1] == 0) {
        vorn = w;
        return;
    }
    for (std::size_t i = 0; i < n; ++i) {
        const Vektor3f& p1 = w[i];
        if (seite[i] == 2) {
            vorn.push_back(p1);
            hinten.push_back(p1);
            continue;
        }
        (seite[i] == 0 ? vorn : hinten).push_back(p1);
        if (seite[i + 1] == 2 || seite[i + 1] == seite[i]) { continue; }
        const Vektor3f& p2 = w[(i + 1) % n];
        const float dd = d[i] / (d[i] - d[i + 1]);
        Vektor3f mitte = p1 + (p2 - p1) * dd;
        mitte[achse] = dist;   // achsenparallel: genau auf der Ebene
        vorn.push_back(mitte);
        hinten.push_back(mitte);
    }
}

void unterteilen(const std::vector<Vektor3f>& w, const Vektor3f& normale, const EchtFlaechenShader& s, float unterteilung, std::vector<EchtLicht>& aus,
                 int tiefe) {
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
            wicklungTeilen(w, i, (mx[i] + mn[i]) * 0.5F, vorn, hinten);
            unterteilen(vorn, normale, s, unterteilung, aus, tiefe + 1);
            unterteilen(hinten, normale, s, unterteilung, aus, tiefe + 1);
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
    EchtLicht l;
    l.art = EchtLicht::Art::Flaeche;
    l.wicklung = w;
    if (l.wicklung.size() > static_cast<std::size_t>(kEchtWicklungMax)) { l.wicklung.resize(static_cast<std::size_t>(kEchtWicklungMax)); }
    l.photonen = s.flaechenlicht * flaeche * kEchtFlaechenSkala;
    l.add = s.flaechenlicht * kEchtFormFaktorSkala * kEchtFlaechenSkala;
    l.farbe = s.farbe;
    l.normale = normale;
    l.ursprung = (mn + mx) * 0.5F + normale;   // minmax.origin(), "nudge it off the plane a bit"
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
        for (std::size_t i = w.size(); i-- != 0;) { r.wicklung.push_back(w[i] + normale * s.backsplashWeite); }
        if (r.wicklung.size() > static_cast<std::size_t>(kEchtWicklungMax)) { r.wicklung.resize(static_cast<std::size_t>(kEchtWicklungMax)); }
        r.ursprung = normale * s.backsplashWeite + l.ursprung;
        r.normale = Vektor3f(-normale[0], -normale[1], -normale[2]);
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

EchtLichtWelt echtLichterSammeln(const Karte& k, const std::function<EchtFlaechenShader(const std::string&)>& flaechenShader,
                                 const std::function<EchtSonne(const std::string&)>& sonne) {
    EchtLichtWelt w;
    const auto welt = k.welt();
    if (welt) {
        // LightWorld (light.cpp): _color, _ambient, _minlight
        float c[3] = {1, 1, 1};
        Vektor3f farbe(1, 1, 1);
        if (zahlen(welt->wert("_color"), c, 3) && (c[0] != 0 || c[1] != 0 || c[2] != 0)) { farbe = Vektor3f(c[0], c[1], c[2]); }
        w.umgebung = farbe * zahl(*welt, "_ambient", "ambient");
        w.minimum = farbe * zahl(*welt, "_minlight");
    }
    // targetname -> Entitaet (FindTargetEntity)
    const auto ziel = [&k](const std::string& name) -> const Entitaet* {
        for (const auto& e : k.entitaeten()) {
            if (e->wert("targetname") == name) { return e.get(); }
        }
        return nullptr;
    };
    for (const auto& ep : k.entitaeten()) {
        const Entitaet& e = *ep;
        const std::string klasse = e.wert("classname");
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
        (void)zahlen(e.wert("origin"), o, 3);
        l.ursprung = Vektor3f(o[0], o[1], o[2]);
        l.stil = static_cast<int>(zahl(e, "_style", "style"));
        float staerke = zahl(e, "_light", "light", 300.0F);
        if (staerke == 0) { staerke = 300.0F; }
        if (const float s = zahl(e, "scale"); s != 0) { staerke *= s; }
        const int proben = std::max(1, static_cast<int>(zahl(e, "_samples")));
        staerke /= static_cast<float>(proben);   // die Jitter-Kopien fehlen in der Vorschau
        float f[3];
        if (zahlen(e.wert("_color"), f, 3)) {
            l.farbe = (flags & 32) != 0 ? Vektor3f(f[0], f[1], f[2]) : normiert(Vektor3f(f[0], f[1], f[2]));
        }
        l.extraAbstand = zahl(e, "_extradist");
        l.photonen = staerke;
        if (staerke < 0) {
            l.negativ = true;   // SetupEnvelopes: negative Lichter
            l.photonen = -l.photonen;
        }
        const std::string zielName = e.wert("target");
        const Entitaet* z = zielName.empty() ? nullptr : ziel(zielName);
        if (z != nullptr) {
            float zo[3] = {0, 0, 0};
            (void)zahlen(z->wert("origin"), zo, 3);
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
                l.art = EchtLicht::Art::Sonne;   // CreateSunLight: Richtung = -normal
                l.sonnenRichtung = Vektor3f(-n[0], -n[1], -n[2]);
                l.photonen = staerke;
            } else {
                l.photonen *= kEchtSpotSkala;
            }
        } else {
            l.photonen *= kEchtPunktSkala;
        }
        huelleSetzen(l);
        w.lichter.push_back(l);
    }
    // Himmel (q3map_sun) und Flaechenlicht (q3map_surfacelight) aus den Pinselflaechen
    std::vector<std::string> sonnen;
    for (const auto& ep : k.entitaeten()) {
        for (const auto& prim : ep->kinder()) {
            if (prim->art() != Primitiv::Art::Pinsel) { continue; }
            const auto& pinsel = static_cast<const Pinsel&>(*prim);
            for (const auto& fl : pinsel.flaechen()) {
                if (!fl->traegtBei()) { continue; }
                const std::string& sh = fl->shader();
                if (sonne && std::find(sonnen.begin(), sonnen.end(), sh) == sonnen.end()) {
                    const EchtSonne s = sonne(sh);
                    if (s.da || !s.himmel.empty()) { sonnen.push_back(sh); }
                    std::vector<EchtSonne::Sonne> liste = s.sonnen;
                    if (s.da && liste.empty()) { liste.push_back({s.farbe, s.staerke, s.grad, s.hoehe}); }
                    for (const EchtSonne::Sonne& so : liste) {
                        // CreateSunLight (numSamples 1: die Jitter-Kopien fehlen in der Vorschau)
                        EchtLicht l;
                        l.art = EchtLicht::Art::Sonne;
                        l.farbe = normiert(so.farbe);
                        l.photonen = so.staerke;
                        l.sonnenRichtung = kugelRichtung(so.grad, so.hoehe);
                        l.winkel = true;
                        huelleSetzen(l);
                        w.lichter.push_back(l);
                    }
                    for (const EchtSonne::Himmelslicht& h : s.himmel) { echtHimmelsLichter(h, normiert(s.shaderFarbe), w.lichter, &s.proben); }
                }
                if (!flaechenShader) { continue; }
                const EchtFlaechenShader fs = flaechenShader(sh);
                if (fs.flaechenlicht <= 0) { continue; }
                // RadLightForTriangles: jedes Dreieck der Flaeche (Faecher statt der
                // Streifen von -meta) unterteilt nach q3map_lightSubdivide
                const Wicklung& wk = fl->wicklung();
                if (wk.numpoints < 3) { continue; }
                const Vektor3d& nd = fl->ebene().normal();
                const Vektor3f n(static_cast<float>(nd[0]), static_cast<float>(nd[1]), static_cast<float>(nd[2]));
                const std::size_t vorher = w.lichter.size();
                for (std::size_t i = 2; i < wk.numpoints; ++i) {
                    std::vector<Vektor3f> dreieck = {Vektor3f(wk[0].vertex), Vektor3f(wk[i - 1].vertex), Vektor3f(wk[i].vertex)};
                    // die Dreiecke der .bsp laufen im Uhrzeigersinn von vorn (die
                    // Rechte-Hand-Normale zeigt nach hinten) - sonst wird
                    // PointToPolygonFormFactor vor der Flaeche negativ
                    const Vektor3f u = dreieck[1] - dreieck[0];
                    const Vektor3f v = dreieck[2] - dreieck[0];
                    const Vektor3f rh(u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]);
                    if (punkt3(rh, n) > 0) { std::swap(dreieck[1], dreieck[2]); }
                    echtFlaechenLichter(dreieck, n, fs, w.lichter);
                }
                for (std::size_t i = vorher; i < w.lichter.size(); ++i) { w.lichter[i].quelle = ep.get(); }
            }
        }
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
