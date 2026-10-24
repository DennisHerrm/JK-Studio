// kameraszene.cpp - Karten-Editor: was die 3D-Kamera zeichnet.
// Portiert aus NetRadiant-Custom 20260114 (Quellen in kameraszene.h).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/kameraszene.h"

#include "bhed/karte/modell.h"

#include "bhed/karte/karte.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <map>

namespace bhed::karte {

namespace {

std::string klein(const std::string& s) {
    std::string r = s;
    for (char& c : r) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return r;
}

// aabb_corners (aabb.h:187-200)
std::array<Vektor3f, 8> kastenEcken(const Kasten& k) {
    const Vektor3f mn = k.origin - k.extents;
    const Vektor3f mx = k.origin + k.extents;
    return {Vektor3f(mn[0], mx[1], mx[2]), Vektor3f(mx[0], mx[1], mx[2]), Vektor3f(mx[0], mn[1], mx[2]),
            Vektor3f(mn[0], mn[1], mx[2]), Vektor3f(mn[0], mx[1], mn[2]), Vektor3f(mx[0], mx[1], mn[2]),
            Vektor3f(mx[0], mn[1], mn[2]), Vektor3f(mn[0], mn[1], mn[2])};
}

// Ein Dreieck mit Umriss (GL_LINE eines GL_TRIANGLES / Streifen-Dreiecks).
void dreieck(const KameraPunkt& a, const KameraPunkt& b, const KameraPunkt& c, std::vector<KameraPunkt>& dreiecke,
             std::vector<KameraKante>* kanten) {
    dreiecke.push_back(a);
    dreiecke.push_back(b);
    dreiecke.push_back(c);
    if (kanten != nullptr) {
        kanten->push_back({a.pos, b.pos, c.pos, a.farbe, b.farbe, true});
        kanten->push_back({b.pos, c.pos, a.pos, b.farbe, c.farbe, true});
        kanten->push_back({c.pos, a.pos, b.pos, c.farbe, a.farbe, true});
    }
}

// string_parse_vector3
bool dreiZahlen(const std::string& s, Vektor3f& v) {
    float a = 0;
    float b = 0;
    float c = 0;
    char rest = 0;
    const int n = std::sscanf(s.c_str(), "%f %f %f %c", &a, &b, &c, &rest);
    if (n != 3) { return false; }
    v = Vektor3f(a, b, c);
    return true;
}

Vektor3f ursprungVon(const Entitaet& ent) {
    Vektor3f o(0, 0, 0);
    if (!dreiZahlen(ent.wert("origin"), o)) { o = Vektor3f(0, 0, 0); }
    return o;
}

Farbe klassenFarbe(const EntitaetsKlasse* kl) {
    return kl != nullptr ? farbeAus(kl->farbe[0], kl->farbe[1], kl->farbe[2]) : farbeAus(0, 0.5F, 0);
}

// Die zuletzt gesetzte Normale vor einem Pfeil: aabb_draw_flatshade endet mit
// aabb_normals[5] = (0, 0, -1). Bei Gruppen ist sie nicht bestimmbar (der
// vorige Eintrag im selben Zustand) - dieselbe Annahme.
const Vektor3f kPfeilNormale(0, 0, -1);

}  // namespace

// --- Einzelteile ------------------------------------------------------------

void polygonFuellung(const std::vector<KameraPunkt>& ecken, std::vector<KameraPunkt>& dreiecke,
                     std::vector<KameraKante>* kanten) {
    const std::size_t n = ecken.size();
    if (n < 3) { return; }
    for (std::size_t i = 1; i + 1 < n; ++i) {
        dreiecke.push_back(ecken[0]);
        dreiecke.push_back(ecken[i]);
        dreiecke.push_back(ecken[i + 1]);
    }
    if (kanten != nullptr) {
        for (std::size_t i = 0; i < n; ++i) {
            const KameraPunkt& a = ecken[i];
            const KameraPunkt& b = ecken[(i + 1) % n];
            kanten->push_back({a.pos, b.pos, ecken[(i + 2) % n].pos, a.farbe, b.farbe, true});
        }
    }
}

void kastenFuellung(const Kasten& k, Farbe farbe, std::vector<KameraPunkt>& dreiecke, std::vector<KameraKante>* kanten) {
    const std::array<Vektor3f, 8> p = kastenEcken(k);
    // aabb_draw_flatshade (entitylib.h:140-180) mit aabb_normals (aabb.h:243-250)
    static const int quads[6][4] = {{2, 1, 5, 6}, {1, 0, 4, 5}, {0, 1, 2, 3}, {0, 3, 7, 4}, {3, 2, 6, 7}, {7, 6, 5, 4}};
    static const float normalen[6][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {-1, 0, 0}, {0, -1, 0}, {0, 0, -1}};
    std::vector<KameraPunkt> ecken(4);
    for (int q = 0; q < 6; ++q) {
        for (int i = 0; i < 4; ++i) {
            ecken[static_cast<std::size_t>(i)] = {p[static_cast<std::size_t>(quads[q][i])],
                                                  Vektor3f(normalen[q][0], normalen[q][1], normalen[q][2]), Vektor2f(0, 0), farbe};
        }
        polygonFuellung(ecken, dreiecke, kanten);
    }
}

void lichtOktaederFuellung(const Kasten& k, Farbe farbe, std::vector<KameraPunkt>& dreiecke, std::vector<KameraKante>* kanten) {
    // light_vertices (light.cpp:488-500): oben, unten, Mitte +y, +x, -y, -x
    const Vektor3f mx = k.origin + k.extents;
    const Vektor3f mn = k.origin - k.extents;
    const Vektor3f& m = k.origin;
    const Vektor3f p[6] = {Vektor3f(m[0], m[1], mx[2]), Vektor3f(m[0], m[1], mn[2]), Vektor3f(m[0], mx[1], m[2]),
                           Vektor3f(mx[0], m[1], m[2]), Vektor3f(m[0], mn[1], m[2]), Vektor3f(mn[0], m[1], m[2])};
    // light_draw, Zweig RENDER_LIGHTING (light.cpp:521-593): glNormal steht
    // jeweils vor der dritten Ecke, die ersten beiden Ecken eines Dreiecks
    // tragen also die Normale des vorigen. Vor dem ersten Dreieck gilt die
    // zuletzt gesetzte - beim vorigen Licht normals[4].
    const float f = 0.70710678F;
    const Vektor3f n[8] = {Vektor3f(0, f, f),  Vektor3f(f, 0, f),  Vektor3f(0, -f, f),  Vektor3f(-f, 0, f),
                           Vektor3f(0, f, -f), Vektor3f(f, 0, -f), Vektor3f(0, -f, -f), Vektor3f(-f, 0, -f)};
    struct D {
        int a, b, c;
        int na, nc;   // Normale der ersten beiden Ecken, der dritten
    };
    static const D folge[8] = {{0, 2, 3, 4, 0}, {0, 3, 4, 0, 1}, {0, 4, 5, 1, 2}, {0, 5, 2, 2, 3},
                               {1, 2, 5, 3, 7}, {1, 5, 4, 7, 6}, {1, 4, 3, 6, 5}, {1, 3, 2, 5, 4}};
    for (const D& d : folge) {
        const KameraPunkt a{p[d.a], n[d.na], Vektor2f(0, 0), farbe};
        const KameraPunkt b{p[d.b], n[d.na], Vektor2f(0, 0), farbe};
        const KameraPunkt c{p[d.c], n[d.nc], Vektor2f(0, 0), farbe};
        dreieck(a, b, c, dreiecke, kanten);
    }
}

const std::vector<Vektor3f>& kugelPunkte() {
    static const std::vector<Vektor3f> punkte = [] {
        // sphere_construct_fill (light.cpp:179-202), SPHERE_FILL_SIDES 12
        constexpr int kSeiten = 12;
        std::vector<Vektor3f> v;
        v.reserve(553);
        const double step = kPi / kSeiten;
        const auto kart = [](double lon, double lat) {   // cartesian
            return Vektor3f(static_cast<float>(std::cos(lon) * std::fabs(std::cos(lat))),
                            static_cast<float>(std::sin(lon) * std::fabs(std::cos(lat))), static_cast<float>(std::sin(lat)));
        };
        v.push_back(Vektor3f(0, 0, -1));
        for (int i = 0; i < kSeiten * 2; i += 2) {
            for (int j = -kSeiten / 2 + 1; j < kSeiten / 2; ++j) {
                v.push_back(kart(step * i, step * j));
                v.push_back(kart(step * (i + 1), step * j));
            }
            v.push_back(Vektor3f(0, 0, 1));
            for (int j = kSeiten / 2 - 1; j > -kSeiten / 2; --j) {
                v.push_back(kart(step * (i + 1), step * j));
                v.push_back(kart(step * (i + 2), step * j));
            }
            v.push_back(Vektor3f(0, 0, -1));
        }
        return v;
    }();
    return punkte;
}

void kugelFuellung(const Vektor3f& mitte, float radius, Farbe farbe, std::vector<KameraPunkt>& dreiecke,
                   std::vector<KameraKante>* kanten) {
    if (!(radius > 0)) { return; }
    const std::vector<Vektor3f>& p = kugelPunkte();
    // sphere_draw_fill sendet keine Normale: es gilt die zuletzt gesetzte,
    // hier die OpenGL-Vorgabe (0, 0, 1).
    const auto ecke = [&](std::size_t i) { return KameraPunkt{mitte + p[i] * radius, Vektor3f(0, 0, 1), Vektor2f(0, 0), farbe}; };
    // GL_TRIANGLE_STRIP: Dreieck k ist (k, k+1, k+2), bei ungeradem k (k+1, k, k+2)
    for (std::size_t k = 0; k + 2 < p.size(); ++k) {
        if ((k & 1U) == 0U) {
            dreieck(ecke(k), ecke(k + 1), ecke(k + 2), dreiecke, kanten);
        } else {
            dreieck(ecke(k + 1), ecke(k), ecke(k + 2), dreiecke, kanten);
        }
    }
}

void pfeilLinien(const Vektor3f& ursprung, const Vektor3f& winkel, Farbe farbe, const Vektor3f& normale,
                 std::vector<KameraKante>& aus) {
    std::vector<Linie> l;
    pfeil(ursprung, winkel, farbe, l);
    for (const Linie& x : l) { aus.push_back({x.a, x.b, normale, farbe, farbe, false}); }
}

LichtRadien lichtRadien(const std::string& lightUnterstrich, const std::string& light, const std::string& scale,
                        const std::string& fade, const std::string& spawnflags) {
    // string_read_float / string_read_int (atof, atoi)
    const float primaer = static_cast<float>(std::atof(lightUnterstrich.c_str()));
    const float sekundaer = static_cast<float>(std::atof(light.c_str()));
    float s = static_cast<float>(std::atof(scale.c_str()));
    if (s <= 0) { s = 1; }
    float f = static_cast<float>(std::atof(fade.c_str()));
    if (f <= 0) { f = 1; }
    const int flags = std::atoi(spawnflags.c_str());
    LichtRadien r;
    r.linear = (flags & 1) != 0;   // spawnflags_linear (LIGHTTYPE_DEFAULT)
    const float staerke = primaer != 0 ? primaer : sekundaer != 0 ? sekundaer : 300.0F;   // getIntensity
    const float intensitaet = std::fabs(staerke * s);
    constexpr float kPunkt = 7500.0F;           // c_pointScale
    constexpr float kLinear = 1.0F / 8000.0F;   // c_linearScale
    const float toleranz[3] = {1.0F, 48.0F, 255.0F};
    for (std::size_t i = 0; i < 3; ++i) {
        if (r.linear) {
            r.r[i] = ((intensitaet * kPunkt * kLinear) - toleranz[i]) / f;   // light_radius_linear / m_fade
        } else {
            r.r[i] = std::sqrt(intensitaet * kPunkt / toleranz[i]);   // light_radius
        }
    }
    return r;
}

void kugelDraht(const Vektor3f& mitte, float radius, Farbe farbe, std::vector<Linie>& aus) {
    if (!(radius > 0)) { return; }
    // sphere_construct_wire (light.cpp:406-432), SPHERE_WIRE_SIDES 24
    constexpr int kSeiten = 24;
    for (int kreis = 0; kreis < 3; ++kreis) {
        Vektor3f p[kSeiten];
        for (int i = 0; i < kSeiten; ++i) {
            const float ds = static_cast<float>(std::sin((i * 2 * kPi) / kSeiten));
            const float dc = static_cast<float>(std::cos((i * 2 * kPi) / kSeiten));
            p[i] = kreis == 0 ? Vektor3f(dc, ds, 0) : kreis == 1 ? Vektor3f(dc, 0, ds) : Vektor3f(0, dc, ds);
        }
        for (int i = 0; i < kSeiten; ++i) {   // GL_LINE_LOOP
            aus.push_back({mitte + p[i] * radius, mitte + p[(i + 1) % kSeiten] * radius, farbe});
        }
    }
}

Vektor3f lichtFarbeNormiert(const std::string& wert) {
    Vektor3f c(1, 1, 1);
    if (!dreiZahlen(wert, c)) { return Vektor3f(1, 1, 1); }
    const float m = std::max({c[0], c[1], c[2]});   // vector3_max_component
    if (m == 0) { return Vektor3f(1, 1, 1); }
    return c / m;
}

void workzone3d(const Kasten& b, float raster, std::vector<KameraKante>& rot, std::vector<KameraKante>& weiss) {
    constexpr std::size_t kPlatz = 9999;   // std::array<Vector3, 9999> m_verticesarr
    for (std::size_t i = 0; i < 3; ++i) {
        const std::size_t i2 = (i + 1) % 3;
        const std::size_t i3 = (i + 2) % 3;
        const float offset = 1024;
        const Vektor3f a2 = kAchsen[i2] * b.extents;
        const Vektor3f a3 = kAchsen[i3] * b.extents;
        std::vector<Vektor3f> punkte;
        punkte.push_back(b.origin + a2 + a3);
        if (b.extents[i2] != 0) { punkte.push_back(b.origin - a2 + a3); }
        if (b.extents[i3] != 0) {
            punkte.push_back(b.origin + a2 - a3);
            if (b.extents[i2] != 0) { punkte.push_back(b.origin - a2 - a3); }
        }
        float grid = raster;
        const float ausdehnung = std::max(0.0F, b.extents[i]);
        std::size_t ungefaehr = static_cast<std::size_t>((ausdehnung + offset) * 4 / grid + 8);
        if (ungefaehr > kPlatz) {   // prevent overlow
            ungefaehr = kPlatz;
            grid = (ausdehnung + offset) * 4 / static_cast<float>(ungefaehr - 8);
            grid = std::exp2(std::ceil(std::log2(grid)));   // round to next PoT
        }
        float coord = zahlEingerastet(b.origin[i] - ausdehnung - offset, grid);
        const bool start0 = zahlEingerastet(coord, grid * 2) == coord;
        std::vector<float> lage;          // Koordinate auf Achse i je Ecke
        std::vector<unsigned> alpha;      // Colour4b(..., alpha * 255)
        std::vector<bool> kante;          // edgearr
        std::size_t count = 0;
        const auto alphaFuer = [&](float c) {
            const float a = std::clamp((offset + b.extents[i] - std::fabs(c - b.origin[i])) / offset, 0.0F, 1.0F);
            return static_cast<unsigned>(static_cast<unsigned char>(a * 255));
        };
        for (; count + 4 < ungefaehr; count += 4) {
            const unsigned a0 = alphaFuer(coord);
            lage.push_back(coord);
            lage.push_back(coord);
            alpha.push_back(a0);
            alpha.push_back(a0);
            coord += grid;
            const unsigned a2w = alphaFuer(coord);
            lage.push_back(coord);
            lage.push_back(coord);
            alpha.push_back(a2w);
            alpha.push_back(a2w);
            coord += grid;
            kante.push_back(false);
            kante.push_back(true);
            kante.push_back(false);
            kante.push_back(true);
        }
        if (punkte.size() == 1) {
            punkte.push_back(punkte[0] + kAchsen[i2] * 8.0F);
            for (std::size_t k = 0; k < count; k += 4) { kante[k + 1] = false; }
        }
        for (std::size_t j = 0; j + 1 < punkte.size(); j += 2) {
            const Vektor3f& pj = punkte[j];
            const Vektor3f& pjj = punkte[j + 1];
            const auto ecke = [&](std::size_t v) {
                const std::size_t r = v % 4;
                Vektor3f p = (r == 0 || r == 3) ? pj : pjj;
                p[i] = lage[v];
                return p;
            };
            const auto quads = [&](std::size_t erste, std::size_t anzahl, bool roh, std::vector<KameraKante>& aus) {
                for (std::size_t q = erste; q + 4 <= erste + anzahl; q += 4) {
                    for (std::size_t e = 0; e < 4; ++e) {
                        const std::size_t v = q + e;
                        const std::size_t w = q + ((e + 1) % 4);
                        if (!kante[v]) { continue; }
                        const Farbe fa = roh ? (0x000000FFU | (alpha[v] << 24)) : (0x00FFFFFFU | (alpha[v] << 24));
                        const Farbe fb = roh ? (0x000000FFU | (alpha[w] << 24)) : (0x00FFFFFFU | (alpha[w] << 24));
                        aus.push_back({ecke(v), ecke(w), ecke(q + ((e + 2) % 4)), fa, fb, true});
                    }
                }
            };
            // glDrawArrays( GL_QUADS, start0? 0 : 2, count - ( start0? 4 : 2 ) ) usw.
            if (count >= (start0 ? 4U : 2U)) { quads(start0 ? 0 : 2, count - (start0 ? 4 : 2), true, rot); }
            if (count >= (start0 ? 2U : 4U)) { quads(start0 ? 2 : 0, count - (start0 ? 2 : 4), false, weiss); }
        }
    }
}

// --- Die Szene ----------------------------------------------------------------

KameraSzene kameraSzeneBauen(const Karte& karte, const KameraSzenenOptionen& o) {
    KameraSzene s;
    const Kontext& k = karte.kontext;
    std::map<std::string, std::size_t> shaderStapel;
    std::map<Farbe, std::size_t> fuellStapel;
    std::map<Farbe, std::size_t> additivStapel;
    // Der Shader der letzten Flaeche (meist dieselbe wie davor): ohne Kleinschreiben und Suchen.
    const std::string* letzterShader = nullptr;
    std::size_t letzterStapel = 0;
    const auto stapel = [&](StapelArt art, const std::string& shader, Farbe farbe) -> KameraStapel& {
        if (art == StapelArt::Shader && letzterShader != nullptr && *letzterShader == shader) { return s.stapel[letzterStapel]; }
        std::map<std::string, std::size_t>* sm = nullptr;
        std::map<Farbe, std::size_t>* fm = nullptr;
        std::string schluessel;
        if (art == StapelArt::Shader) {
            sm = &shaderStapel;
            schluessel = klein(shader);
            const auto it = sm->find(schluessel);
            if (it != sm->end()) {
                letzterShader = &shader;
                letzterStapel = it->second;
                return s.stapel[it->second];
            }
        } else {
            fm = art == StapelArt::Fuellung ? &fuellStapel : &additivStapel;
            const auto it = fm->find(farbe);
            if (it != fm->end()) { return s.stapel[it->second]; }
        }
        KameraStapel neu;
        neu.art = art;
        neu.shader = shader;
        neu.farbe = farbe;
        s.stapel.push_back(std::move(neu));
        if (sm != nullptr) {
            (*sm)[schluessel] = s.stapel.size() - 1;
            letzterShader = &shader;
            letzterStapel = s.stapel.size() - 1;
        } else {
            (*fm)[farbe] = s.stapel.size() - 1;
        }
        return s.stapel.back();
    };
    // Objekte und Matrix-Wechsel je Zustand (OpenGLStateBucket::render: der
    // erste zaehlt immer, dann jeder Wechsel); 0 = Einheitsmatrix, sonst die
    // Entitaet (localToWorld = Ursprung).
    const void* tintLetzte = nullptr;
    const void* overlayLetzte = nullptr;
    const auto zaehlen = [](std::size_t& objekte, std::size_t& transforms, const void*& letzt, const void* matrix) {
        if (objekte == 0 || letzt != matrix) { ++transforms; }
        letzt = matrix;
        ++objekte;
    };
    const auto objekt = [&](KameraStapel& st, const void* matrix) { zaehlen(st.objekte, st.transforms, st.letzteMatrix, matrix); };
    const bool geometrie = o.teil != KameraTeil::Rest;   // Shader-Stapel der Pinsel und Patches
    const bool rest = o.teil != KameraTeil::Geometrie;   // Punktentitaeten, Gruppen, Namen, Auswahl
    const auto tintObjekt = [&](const void* matrix) { zaehlen(s.tintObjekte, s.tintTransforms, tintLetzte, matrix); };
    const auto overlayObjekt = [&](const void* matrix) { zaehlen(s.overlayObjekte, s.overlayTransforms, overlayLetzte, matrix); };
    const auto wahlDazu = [&](std::size_t dreieckeVorher, const std::vector<KameraPunkt>& dreiecke, bool tint) {
        if (tint) { s.wahlDreiecke.insert(s.wahlDreiecke.end(), dreiecke.begin() + static_cast<std::ptrdiff_t>(dreieckeVorher), dreiecke.end()); }
    };
    std::vector<KameraPunkt> ecken;
    std::vector<KameraKante> umriss;
    for (const auto& ent : karte.entitaeten()) {
        if (!ent->sichtbar()) { continue; }   // Node::visible (mit den Kindern)
        const EntitaetsKlasse* kl = ent->klasse();
        const bool welt = ent->klassenname() == "worldspawn";
        const bool entGew = rest && karte.auswahl.gewaehlt(ent.get());
        bool kindGew = false;
        Kasten kinderBox;
        for (const auto& p : ent->kinder()) {
            if (!p->sichtbar()) { continue; }
            const bool selbstGew = rest && karte.auswahl.gewaehlt(ent.get(), p.get());
            const bool gew = entGew || selbstGew;   // RenderHighlighted: Zustand der Eltern gilt fuer die Kinder
            // Nur der Rest: unausgewaehlte Pinsel und Patches tragen nur ihre Grenzen bei.
            if (!geometrie && !gew) {
                const Kasten g = p->art() == Primitiv::Art::Pinsel ? static_cast<const Pinsel&>(*p).grenzen(k)
                                                                    : static_cast<const Patch&>(*p).grenzen();
                s.etagen.push_back({g, false});
                kastenErweiternSicher(kinderBox, g);
                continue;
            }
            kindGew = kindGew || selbstGew;
            if (p->art() == Primitiv::Art::Pinsel) {
                const auto& pinsel = static_cast<const Pinsel&>(*p);
                pinsel.bRep(k);
                const Kasten g = pinsel.grenzen(k);
                if (rest) {
                    s.etagen.push_back({g, selbstGew});
                    kastenErweiternSicher(kinderBox, g);
                }
                for (const auto& fl : pinsel.flaechen()) {
                    if (!fl->traegtBei() || fl->gefiltert) { continue; }   // FaceInstance::render: !m_face->isFiltered()
                    const Wicklung& w = fl->wicklung();
                    const Vektor3d& nd = fl->ebene().normal();
                    const Vektor3f n(static_cast<float>(nd[0]), static_cast<float>(nd[1]), static_cast<float>(nd[2]));
                    ecken.clear();
                    for (std::size_t i = 0; i < w.numpoints; ++i) {
                        const Vektor3d& v = w[i].vertex;
                        ecken.push_back({Vektor3f(static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2])), n,
                                         w[i].texcoord, 0xFFFFFFFFU});
                    }
                    if (!rest) {
                        // Nur Geometrie: alle Umrisse in kanten (die Auswahl zeichnet der Rest)
                        KameraStapel& st = stapel(StapelArt::Shader, fl->shader(), 0xFFFFFFFFU);
                        polygonFuellung(ecken, st.dreiecke, o.kanten ? &st.kanten : nullptr);
                        objekt(st, nullptr);
                        continue;
                    }
                    std::vector<KameraPunkt> nurWahl;
                    KameraStapel* stp = geometrie ? &stapel(StapelArt::Shader, fl->shader(), 0xFFFFFFFFU) : nullptr;
                    std::vector<KameraPunkt>& dreieckZiel = stp != nullptr ? stp->dreiecke : nurWahl;
                    const std::size_t vorher = dreieckZiel.size();
                    polygonFuellung(ecken, dreieckZiel, (o.kanten && stp != nullptr) ? (gew ? &stp->kantenGewaehlt : &stp->kanten) : nullptr);
                    if (stp != nullptr) { objekt(*stp, nullptr); }
                    if (gew) {
                        tintObjekt(nullptr);
                        overlayObjekt(nullptr);
                        ++s.wahlPrimitive;
                        wahlDazu(vorher, dreieckZiel, true);
                        umriss.clear();
                        std::vector<KameraPunkt> leer;
                        polygonFuellung(ecken, leer, &umriss);
                        s.wahlKanten.insert(s.wahlKanten.end(), umriss.begin(), umriss.end());
                    }
                }
            } else {
                const auto& patch = static_cast<const Patch&>(*p);
                if (rest) {
                    s.etagen.push_back({patch.grenzen(), selbstGew});
                    kastenErweiternSicher(kinderBox, patch.grenzen());
                }
                const PatchTesselierung& t = patch.tesselierung();
                std::vector<KameraPunkt> nurWahl;
                KameraStapel* stp = geometrie ? &stapel(StapelArt::Shader, patch.shader(), 0xFFFFFFFFU) : nullptr;
                std::vector<KameraPunkt>& dreieckZiel = stp != nullptr ? stp->dreiecke : nurWahl;
                const std::size_t vorher = dreieckZiel.size();
                std::vector<KameraKante>* ziel =
                    (o.kanten && stp != nullptr) ? ((gew && rest) ? &stp->kantenGewaehlt : &stp->kanten) : nullptr;
                const auto punktAus = [&](unsigned int index) {
                    const NetzPunkt& v = t.vertices[index];
                    return KameraPunkt{v.vertex, v.normal, v.texcoord, 0xFFFFFFFFU};
                };
                for (std::size_t sIdx = 0; sIdx < t.numStrips; ++sIdx) {
                    const unsigned int* q = t.indices.data() + (sIdx * t.lenStrips);
                    // GL_QUAD_STRIP: Vierecke (2i, 2i+1, 2i+3, 2i+2)
                    for (std::size_t i = 0; i + 3 < t.lenStrips; i += 2) {
                        const KameraPunkt a = punktAus(q[i]);
                        const KameraPunkt b = punktAus(q[i + 1]);
                        const KameraPunkt c = punktAus(q[i + 3]);
                        const KameraPunkt d = punktAus(q[i + 2]);
                        dreieckZiel.push_back(a);
                        dreieckZiel.push_back(b);
                        dreieckZiel.push_back(d);
                        dreieckZiel.push_back(d);
                        dreieckZiel.push_back(b);
                        dreieckZiel.push_back(c);
                        const KameraKante kk[4] = {{a.pos, b.pos, c.pos, a.farbe, b.farbe, true},
                                                   {b.pos, c.pos, d.pos, b.farbe, c.farbe, true},
                                                   {c.pos, d.pos, a.pos, c.farbe, d.farbe, true},
                                                   {d.pos, a.pos, b.pos, d.farbe, a.farbe, true}};
                        if (ziel != nullptr) { ziel->insert(ziel->end(), std::begin(kk), std::end(kk)); }
                        if (gew) { s.wahlKanten.insert(s.wahlKanten.end(), std::begin(kk), std::end(kk)); }
                    }
                }
                if (stp != nullptr) { objekt(*stp, nullptr); }
                if (gew) {
                    tintObjekt(nullptr);
                    overlayObjekt(nullptr);
                    ++s.wahlPrimitive;
                    wahlDazu(vorher, dreieckZiel, true);
                }
            }
        }
        if (!rest) { continue; }   // nur Geometrie: keine Punktentitaeten, Gruppen, Namen
        const Farbe kf = klassenFarbe(kl);
        if (!ent->istBehaelter() && k.modelle != nullptr && kl != nullptr && kl->miscmodelIs) {
            // MiscModel: das Modell (PicoModel/md3 Model::render mit den Shadern der
            // Flaechen, Skin und _remap), der Name am Ursprung
            const ModellAnsicht a = k.modelle->ansicht(*ent);
            const Matrix4 m = miscModellMatrix(*ent);
            s.etagen.push_back({modellWeltKasten(a, m), entGew});
            if (a.modell) {
                for (std::size_t fi = 0; fi < a.modell->flaechen.size(); ++fi) {
                    const ModellFlaeche& f = a.modell->flaechen[fi];
                    const std::string& sh = fi < a.shader.size() ? a.shader[fi] : f.shader;
                    if (sh.empty()) { continue; }
                    KameraStapel& st = stapel(StapelArt::Shader, sh, 0xFFFFFFFFU);
                    const std::size_t vorher = st.dreiecke.size();
                    for (std::size_t i = 0; i + 2 < f.indizes.size(); i += 3) {
                        std::vector<KameraPunkt> d(3);
                        for (std::size_t j = 0; j < 3; ++j) {
                            const ModellPunkt& p = f.punkte[f.indizes[i + j]];
                            Vektor3f n = transformierteRichtung(m, p.normale);
                            const float l = static_cast<float>(laenge(n));
                            if (l > 0) { n = n * (1.0F / l); }
                            d[j] = {transformierterPunkt(m, p.ort), n, p.st, 0xFFFFFFFFU};
                        }
                        polygonFuellung(d, st.dreiecke, o.kanten ? (entGew ? &st.kantenGewaehlt : &st.kanten) : nullptr);
                        if (entGew) {
                            std::vector<KameraPunkt> leer;
                            polygonFuellung(d, leer, &s.wahlKanten);
                        }
                    }
                    objekt(st, ent.get());
                    if (entGew) {
                        tintObjekt(ent.get());
                        overlayObjekt(ent.get());
                        wahlDazu(vorher, st.dreiecke, true);
                    }
                }
            }
            if (o.namen || entGew) {
                const std::string tn = o.zielnamen ? ent->wert("targetname") : std::string();
                s.namen.push_back({ursprungVon(*ent), tn.empty() ? ent->klassenname() : tn, kf, entGew ? 1 : 0});
            }
        } else if (!ent->istBehaelter()) {
            // Punktentitaet (GenericEntity / Light)
            const Vektor3f ursprung = ursprungVon(*ent);
            const Vektor3f mins = kl != nullptr ? kl->mins : Vektor3f(-8, -8, -8);
            const Vektor3f maxs = kl != nullptr ? kl->maxs : Vektor3f(8, 8, 8);
            const Kasten box = kastenAusMinMax(ursprung + mins, ursprung + maxs);
            s.etagen.push_back({box, entGew});
            const bool licht = ent->klassenname() == "light" || ent->klassenname() == "lightJunior";
            Vektor3f namePos = ursprung;
            if (licht) {
                // Light::renderSolid: m_colour.state() - "(r g b)" der normierten _color
                const Vektor3f c = lichtFarbeNormiert(ent->wert("_color"));
                const Farbe lf = farbeAus(c[0], c[1], c[2]);
                KameraStapel& st = stapel(StapelArt::Fuellung, "", lf);
                const std::size_t vorher = st.dreiecke.size();
                lichtOktaederFuellung(box, lf, st.dreiecke, o.kanten ? (entGew ? &st.kantenGewaehlt : &st.kanten) : nullptr);
                objekt(st, ent.get());
                if (entGew) {
                    tintObjekt(ent.get());
                    overlayObjekt(ent.get());
                    wahlDazu(vorher, st.dreiecke, true);
                    std::vector<KameraPunkt> leer;
                    lichtOktaederFuellung(box, lf, leer, &s.wahlKanten);
                    if (o.lichtRadien && !ent->hat("target")) {
                        // m_colour_add = m_colour / 8, Zustand "{r g b}"
                        const Farbe af = farbeAus(c[0] / 8, c[1] / 8, c[2] / 8);
                        KameraStapel& ad = stapel(StapelArt::Additiv, "", af);
                        const LichtRadien r = lichtRadien(ent->wert("_light"), ent->wert("light"), ent->wert("scale"),
                                                          ent->wert("fade"), ent->wert("spawnflags"));
                        for (const float radius : r.gezeichnet()) {
                            kugelFuellung(box.origin, radius, af, ad.dreiecke, o.kanten ? &ad.kanten : nullptr);
                        }
                        objekt(ad, ent.get());   // m_radii_fill: ein Objekt fuer beide Kugeln
                    }
                }
                namePos = box.origin;   // m_aabb_light.origin
            } else {
                KameraStapel& st = stapel(StapelArt::Fuellung, "", kf);
                const std::size_t vorher = st.dreiecke.size();
                kastenFuellung(box, kf, st.dreiecke, o.kanten ? (entGew ? &st.kantenGewaehlt : &st.kanten) : nullptr);
                objekt(st, ent.get());
                if (entGew) {
                    tintObjekt(ent.get());
                    overlayObjekt(ent.get());
                }
                std::vector<KameraKante> pfeilL;
                if (o.winkel) {
                    const Vektor3f w = entitaetsWinkel(ent->wert("angle"), ent->wert("angles"), ent->hat("angles"), false);
                    pfeilLinien(box.origin, w, kf, kPfeilNormale, pfeilL);
                    st.linien.insert(st.linien.end(), pfeilL.begin(), pfeilL.end());
                    objekt(st, ent.get());
                    if (entGew) {
                        tintObjekt(ent.get());
                        overlayObjekt(ent.get());
                    }
                }
                if (entGew) {
                    wahlDazu(vorher, st.dreiecke, true);
                    std::vector<KameraPunkt> leer;
                    kastenFuellung(box, kf, leer, &s.wahlKanten);
                    s.wahlKanten.insert(s.wahlKanten.end(), pfeilL.begin(), pfeilL.end());
                    s.wahlTintLinien.insert(s.wahlTintLinien.end(), pfeilL.begin(), pfeilL.end());
                }
            }
            if (o.namen || entGew) {
                const std::string tn = o.zielnamen ? ent->wert("targetname") : std::string();
                s.namen.push_back({namePos, tn.empty() ? ent->klassenname() : tn, kf, entGew ? 1 : 0});
            }
        } else if (!welt) {
            // Group: Name und Pfeil in der Mitte der Kinder (group.cpp:169-190)
            const Vektor3f mitte = kastenGueltig(kinderBox) && ausdehnungGueltig(kinderBox.extents.x()) ? kinderBox.origin : ursprungVon(*ent);
            if (entGew || kindGew || o.namen) {
                const std::string tn = o.zielnamen ? ent->wert("targetname") : std::string();
                s.namen.push_back({mitte, tn.empty() ? ent->klassenname() : tn, kf, entGew ? 1 : kindGew ? 2 : 0});
            }
            const bool winkelZeigen = (kl != nullptr && kl->hasAngles) || ent->hat("angle") || ent->hat("angles");
            if (winkelZeigen && o.winkel) {
                const Vektor3f w = entitaetsWinkel(ent->wert("angle"), ent->wert("angles"), ent->hat("angles"), true);
                std::vector<KameraKante> pfeilL;
                pfeilLinien(mitte, w, kf, kPfeilNormale, pfeilL);
                KameraStapel& st = stapel(StapelArt::Fuellung, "", kf);
                st.linien.insert(st.linien.end(), pfeilL.begin(), pfeilL.end());
                objekt(st, nullptr);   // g_matrix4_identity
                if (entGew || kindGew) {
                    s.wahlKanten.insert(s.wahlKanten.end(), pfeilL.begin(), pfeilL.end());
                    overlayObjekt(nullptr);
                }
                if (entGew) {
                    s.wahlTintLinien.insert(s.wahlTintLinien.end(), pfeilL.begin(), pfeilL.end());
                    tintObjekt(nullptr);
                }
            }
        }
    }
    for (const KameraStapel& st : s.stapel) { s.dreieckAnzahl += st.dreiecke.size() / 3; }
    return s;
}

}  // namespace bhed::karte
