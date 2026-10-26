// echtlicht.cpp - Karten-Editor Stufe 7 Punkt 5: die Lichter der Karte wie
// q3map2 -light (siehe echtlicht.h).
//
// Portiert aus q3map2 (NetRadiant-Custom 20260114: tools/quake3/q3map2/light.cpp
// CreateEntityLights, CreateSunLight, LightContributionToSample;
// light_ydnar.cpp SetupEnvelopes; light_bounce.cpp RadLightForTriangles).
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
void huelleSetzen(EchtLicht& l) {
    if (l.art == EchtLicht::Art::Sonne) {
        l.huelle = 1e9F;
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

}  // namespace

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
                    if (s.da) {
                        sonnen.push_back(sh);
                        EchtLicht l;
                        l.art = EchtLicht::Art::Sonne;
                        l.farbe = normiert(s.farbe);
                        l.photonen = s.staerke;
                        l.sonnenRichtung = kugelRichtung(s.grad, s.hoehe);
                        l.winkel = true;
                        huelleSetzen(l);
                        w.lichter.push_back(l);
                    }
                }
                if (!flaechenShader) { continue; }
                const EchtFlaechenShader fs = flaechenShader(sh);
                if (fs.flaechenlicht <= 0) { continue; }
                // Naeherung: ein Licht je Flaeche in ihrem Schwerpunkt mit der Flaeche
                // (RadLightForTriangles: photons = value * area * areaScale je Teilstueck)
                const Wicklung& wk = fl->wicklung();
                if (wk.numpoints < 3) { continue; }
                Vektor3d mitte(0, 0, 0);
                double flaeche = 0;
                for (std::size_t i = 2; i < wk.numpoints; ++i) {
                    const Vektor3d a = wk[i - 1].vertex - wk[0].vertex;
                    const Vektor3d b = wk[i].vertex - wk[0].vertex;
                    const Vektor3d x(a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]);
                    flaeche += 0.5 * std::sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
                }
                for (std::size_t i = 0; i < wk.numpoints; ++i) { mitte = mitte + wk[i].vertex; }
                mitte = mitte / static_cast<double>(wk.numpoints);
                const Vektor3d& nd = fl->ebene().normal();
                EchtLicht l;
                l.art = EchtLicht::Art::Flaeche;
                l.normale = Vektor3f(static_cast<float>(nd[0]), static_cast<float>(nd[1]), static_cast<float>(nd[2]));
                l.ursprung = Vektor3f(static_cast<float>(mitte[0]), static_cast<float>(mitte[1]), static_cast<float>(mitte[2])) + l.normale * 1.0F;
                l.farbe = normiert(fs.farbe);
                l.photonen = fs.flaechenlicht * static_cast<float>(flaeche) * kEchtFlaechenSkala;
                l.winkel = true;
                l.quelle = ep.get();
                huelleSetzen(l);
                w.lichter.push_back(l);
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
