// grundformen.cpp - Karten-Editor: Pinsel-Grundformen.
// Portiert aus NetRadiant-Custom 20260114 (Quellen in grundformen.h).
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
#include "bhed/karte/grundformen.h"

#include "bhed/karte/karte.h"
#include "bhed/karte/texdef.h"

#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif
#include "../../third_party/quickhull/QuickHull.hpp"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

namespace bhed::karte {

namespace {

constexpr double kZweiPi = 2 * kPi;   // c_2pi

float maxAusdehnung(const Vektor3f& e) { return std::max(std::max(e[0], e[1]), e[2]); }   // max_extent

float maxAusdehnung2d(const Vektor3f& e, std::size_t achse) {   // max_extent_2d
    switch (achse) {
    case 0: return std::max(e[1], e[2]);
    case 1: return std::max(e[0], e[2]);
    default: return std::max(e[0], e[1]);
    }
}

bool seitenPruefen(const char* name, std::size_t seiten, std::size_t min, std::size_t max, Protokoll& p) {
    if (seiten < min) {
        fehler(p, std::string(name) + ": sides " + std::to_string(seiten) + ": too few sides, minimum is " + std::to_string(min) + "\n");
        return false;
    }
    if (seiten > max) {
        fehler(p, std::string(name) + ": sides " + std::to_string(seiten) + ": too many sides, maximum is " + std::to_string(max) + "\n");
        return false;
    }
    return true;
}

// float = float + float * double (die Rechnung in double, gespeichert als float)
float f(double v) { return static_cast<float>(v); }

// vector3_for_spherical (libs/math/vector.h:546-552): ein Vector3 aus double
Vektor3f kugelVektor(double theta, double phi) {
    return Vektor3f(f(std::cos(theta) * std::cos(phi)), f(std::sin(theta) * std::cos(phi)), f(std::sin(phi)));
}

// icosahedron::vdata / tindices (brushmanip.cpp:333-347)
constexpr double kIkoX = .525731112119133606;
constexpr double kIkoZ = .850650808352039932;
const Vektor3d kIkoEcken[12] = {Vektor3d(-kIkoX, 0, kIkoZ), Vektor3d(kIkoX, 0, kIkoZ),  Vektor3d(-kIkoX, 0, -kIkoZ), Vektor3d(kIkoX, 0, -kIkoZ),
                                Vektor3d(0, kIkoZ, kIkoX),  Vektor3d(0, kIkoZ, -kIkoX), Vektor3d(0, -kIkoZ, kIkoX),  Vektor3d(0, -kIkoZ, -kIkoX),
                                Vektor3d(kIkoZ, kIkoX, 0),  Vektor3d(-kIkoZ, kIkoX, 0), Vektor3d(kIkoZ, -kIkoX, 0),  Vektor3d(-kIkoZ, -kIkoX, 0)};
constexpr unsigned int kIkoDreiecke[20][3] = {{0, 4, 1}, {0, 9, 4},  {9, 5, 4},  {4, 5, 8},  {4, 8, 1},  {8, 10, 1}, {8, 3, 10},
                                               {5, 3, 8}, {5, 2, 3},  {2, 7, 3},  {7, 10, 3}, {7, 6, 10}, {7, 11, 6}, {11, 0, 6},
                                               {0, 1, 6}, {6, 1, 10}, {9, 0, 11}, {9, 11, 2}, {9, 2, 5},  {7, 2, 11}};

// icosahedron::drawtri (brushmanip.cpp:349-369)
void dreieck(const Vektor3d& a, const Vektor3d& b, const Vektor3d& c, std::size_t unterteilungen, bool stutzen,
             std::vector<quickhull::Vector3<double>>& wolke) {
    if (unterteilungen == 0) {
        const auto dazu = [&wolke](const Vektor3d& q) { wolke.emplace_back(q.x(), q.y(), q.z()); };
        if (stutzen) {   // "this is not quite correct after subdivision performed"
            dazu((a * (2.0 / 3.0)) + (b * (1.0 / 3.0)));
            dazu((a * (1.0 / 3.0)) + (b * (2.0 / 3.0)));
            dazu((a * (2.0 / 3.0)) + (c * (1.0 / 3.0)));
            dazu((a * (1.0 / 3.0)) + (c * (2.0 / 3.0)));
            dazu((b * (2.0 / 3.0)) + (c * (1.0 / 3.0)));
            dazu((b * (1.0 / 3.0)) + (c * (2.0 / 3.0)));
        } else {
            dazu(a);
            dazu(b);
            dazu(c);
        }
    } else {
        const Vektor3d ab = normiert(a + b);
        const Vektor3d ac = normiert(a + c);
        const Vektor3d bc = normiert(b + c);
        dreieck(a, ab, ac, unterteilungen - 1, stutzen, wolke);
        dreieck(b, bc, ab, unterteilungen - 1, stutzen, wolke);
        dreieck(c, ac, bc, unterteilungen - 1, stutzen, wolke);
        dreieck(ab, bc, ac, unterteilungen - 1, stutzen, wolke);
    }
}

}  // namespace

SeitenBereich seitenBereich(Grundform art) {
    switch (art) {
    case Grundform::Prisma:
    case Grundform::Kegel: return {8, 3, 1022};
    case Grundform::Kugel: return {8, 3, 31};
    case Grundform::Fels: return {32, 10, 1000};
    case Grundform::Ikosaeder: return {1, 0, 2};   // "possible with 3, but buggy"
    default: return {0, 0, 99};                     // QSpinBox-Vorgabe
    }
}

// Brush_ConstructPrism (brushmanip.cpp:96-160)
void prismaBauen(Pinsel& b, const Kasten& grenzen, std::size_t seiten, std::size_t achse, const std::string& shader,
                 const TexturProjektion& projektion, const Kontext& k, Protokoll& p) {
    if (!seitenPruefen("brushPrism", seiten, 3, kMaxFlaechen - 2, p)) { return; }
    b.leeren();
    const Vektor3f mins(grenzen.origin - grenzen.extents);
    const Vektor3f maxs(grenzen.origin + grenzen.extents);
    const float radius = maxAusdehnung2d(grenzen.extents, achse);
    const Vektor3f& mid = grenzen.origin;
    const std::size_t x = (achse + 1) % 3;
    const std::size_t y = (achse + 2) % 3;
    const std::size_t z = achse;
    Vektor3f pp[3];
    pp[2][x] = mins[x];
    pp[2][y] = mins[y];
    pp[2][z] = maxs[z];
    pp[1][x] = maxs[x];
    pp[1][y] = mins[y];
    pp[1][z] = maxs[z];
    pp[0][x] = maxs[x];
    pp[0][y] = maxs[y];
    pp[0][z] = maxs[z];
    b.ebeneHinzufuegen(pp[0], pp[1], pp[2], shader, projektion, k);
    pp[0][x] = mins[x];
    pp[0][y] = mins[y];
    pp[0][z] = mins[z];
    pp[1][x] = maxs[x];
    pp[1][y] = mins[y];
    pp[1][z] = mins[z];
    pp[2][x] = maxs[x];
    pp[2][y] = maxs[y];
    pp[2][z] = mins[z];
    b.ebeneHinzufuegen(pp[0], pp[1], pp[2], shader, projektion, k);
    for (std::size_t i = 0; i < seiten; ++i) {
        const double sv = std::sin(static_cast<double>(i) * kZweiPi / static_cast<double>(seiten));
        const double cv = std::cos(static_cast<double>(i) * kZweiPi / static_cast<double>(seiten));
        pp[0][x] = f(mid[x] + radius * cv);
        pp[0][y] = f(mid[y] + radius * sv);
        pp[0][z] = mins[z];
        pp[1][x] = pp[0][x];
        pp[1][y] = pp[0][y];
        pp[1][z] = maxs[z];
        pp[2][x] = f(pp[0][x] - radius * sv);
        pp[2][y] = f(pp[0][y] + radius * cv);
        pp[2][z] = maxs[z];
        b.ebeneHinzufuegen(pp[0], pp[1], pp[2], shader, projektion, k);
    }
}

// Brush_ConstructCone (brushmanip.cpp:166-213)
void kegelBauen(Pinsel& b, const Kasten& grenzen, std::size_t seiten, std::size_t achse, const std::string& shader,
                const TexturProjektion& projektion, const Kontext& k, Protokoll& p) {
    if (!seitenPruefen("brushCone", seiten, 3, kMaxFlaechen - 1, p)) { return; }
    b.leeren();
    const Vektor3f mins(grenzen.origin - grenzen.extents);
    const Vektor3f maxs(grenzen.origin + grenzen.extents);
    const float radius = maxAusdehnung2d(grenzen.extents, achse);
    const Vektor3f& mid = grenzen.origin;
    const std::size_t x = (achse + 1) % 3;
    const std::size_t y = (achse + 2) % 3;
    const std::size_t z = achse;
    Vektor3f pp[3];
    pp[0][x] = mins[x];
    pp[0][y] = mins[y];
    pp[0][z] = mins[z];
    pp[1][x] = maxs[x];
    pp[1][y] = mins[y];
    pp[1][z] = mins[z];
    pp[2][x] = maxs[x];
    pp[2][y] = maxs[y];
    pp[2][z] = mins[z];
    b.ebeneHinzufuegen(pp[0], pp[1], pp[2], shader, projektion, k);
    for (std::size_t i = 0; i < seiten; ++i) {
        const double sv = std::sin(static_cast<double>(i) * kZweiPi / static_cast<double>(seiten));
        const double cv = std::cos(static_cast<double>(i) * kZweiPi / static_cast<double>(seiten));
        pp[0][x] = f(mid[x] + radius * cv);
        pp[0][y] = f(mid[y] + radius * sv);
        pp[0][z] = mins[z];
        pp[1][x] = mid[x];
        pp[1][y] = mid[y];
        pp[1][z] = maxs[z];
        pp[2][x] = f(pp[0][x] - radius * sv);
        pp[2][y] = f(pp[0][y] + radius * cv);
        pp[2][z] = mins[z];
        b.ebeneHinzufuegen(pp[0], pp[1], pp[2], shader, projektion, k);
    }
}

// Brush_ConstructSphere (brushmanip.cpp:218-266)
void kugelBauen(Pinsel& b, const Kasten& grenzen, std::size_t seiten, const std::string& shader, const TexturProjektion& projektion,
                const Kontext& k, Protokoll& p) {
    if (!seitenPruefen("brushSphere", seiten, 3, 31, p)) { return; }
    b.leeren();
    const float radius = maxAusdehnung(grenzen.extents);
    const Vektor3f& mid = grenzen.origin;
    Vektor3f pp[3];
    const double dt = 2 * kPi / static_cast<double>(seiten);
    const double dp = kPi / static_cast<double>(seiten);
    for (std::size_t i = 0; i < seiten; ++i) {
        for (std::size_t j = 0; j < seiten - 1; ++j) {
            const double t = static_cast<double>(i) * dt;
            const double ph = static_cast<double>(f(static_cast<double>(j) * dp - kPi / 2));   // double p = float( j * dp - c_pi / 2 )
            pp[0] = mid + kugelVektor(t, ph) * radius;
            pp[1] = mid + kugelVektor(t, ph + dp) * radius;
            pp[2] = mid + kugelVektor(t + dt, ph + dp) * radius;
            b.ebeneHinzufuegen(pp[0], pp[1], pp[2], shader, projektion, k);
        }
    }
    {
        const double ph = static_cast<double>(seiten - 1) * dp - kPi / 2;
        for (std::size_t i = 0; i < seiten; ++i) {
            const double t = static_cast<double>(i) * dt;
            pp[0] = mid + kugelVektor(t, ph) * radius;
            pp[1] = mid + kugelVektor(t + dt, ph + dp) * radius;
            pp[2] = mid + kugelVektor(t + dt, ph) * radius;
            b.ebeneHinzufuegen(pp[0], pp[1], pp[2], shader, projektion, k);
        }
    }
}

// Brush_ConstructRock (brushmanip.cpp:271-327)
void felsBauen(Pinsel& b, const Kasten& grenzen, std::size_t seiten, const std::string& shader, const TexturProjektion& projektion,
               const Kontext& k, Protokoll& p) {
    if (!seitenPruefen("brushRock", seiten, 10, 1000, p)) { return; }
    b.leeren();
    const float radius = maxAusdehnung(grenzen.extents);
    const Vektor3f& mid = grenzen.origin;
    Vektor3f pp[3];
    for (std::size_t j = 0; j < seiten; ++j) {
        pp[0][0] = static_cast<float>(std::rand() - (RAND_MAX / 2));
        pp[0][1] = static_cast<float>(std::rand() - (RAND_MAX / 2));
        pp[0][2] = static_cast<float>(std::rand() - (RAND_MAX / 2));
        normieren(pp[0]);
        berechneAchsenbasis(pp[0], pp[1], pp[2]);   // zwei Vektoren senkrecht zu pp[0]
        pp[0] = mid + pp[0] * radius;
        pp[1] = pp[0] + pp[1] * radius;
        pp[2] = pp[0] + pp[2] * radius;
        b.ebeneHinzufuegen(pp[0], pp[1], pp[2], shader, projektion, k);
    }
}

// icosahedron::Brush_ConstructIcosahedron (brushmanip.cpp:371-403)
void ikosaederBauen(Pinsel& b, const Kasten& grenzen, std::size_t unterteilungen, bool stutzen, const std::string& shader,
                    const TexturProjektion& projektion, const Kontext& k) {
    b.leeren();
    const float radius = maxAusdehnung(grenzen.extents);
    const Vektor3d mid(grenzen.origin);
    std::vector<quickhull::Vector3<double>> wolke;
    for (const auto& d : kIkoDreiecke) { dreieck(kIkoEcken[d[0]], kIkoEcken[d[1]], kIkoEcken[d[2]], unterteilungen, stutzen, wolke); }
    quickhull::QuickHull<double> huelle;
    auto ergebnis = huelle.getConvexHull(wolke, true, true);
    const auto& indizes = ergebnis.getIndexBuffer();
    const std::size_t dreiecke = indizes.size() / 3;
    std::vector<Ebene3d> ebenen;
    for (std::size_t i = 0; i < dreiecke; ++i) {
        EbenenPunkte q{};
        for (std::size_t j = 0; j < 3; ++j) {
            const auto& w = wolke[indizes[i * 3 + j]];
            q[j] = Vektor3d(w.x, w.y, w.z);
        }
        const Ebene3d e = ebeneAusPunkten(q);
        if (ebeneGueltig(e) && std::none_of(ebenen.begin(), ebenen.end(), [&](const Ebene3d& x) { return ebenenGleich(e, x); })) {
            ebenen.push_back(e);
            b.ebeneHinzufuegen(q[0] * radius + mid, q[1] * radius + mid, q[2] * radius + mid, shader, projektion, k);
        }
    }
}

void grundformBauen(Karte& k, Pinsel& b, Grundform art, const Kasten& grenzen, std::size_t seiten, bool option, std::size_t achse,
                    const std::string& shader, Protokoll& p) {
    TexturProjektion projektion;   // TextureTransform_getDefault
    texdefStandard(k.kontext.art(), projektion);
    const Kontext& kt = k.kontext;
    switch (art) {
    case Grundform::Quader: {
        Befehl befehl(k.undo, "brushCuboid");
        b.quaderBauen(grenzen, shader, projektion, kt);
        break;
    }
    case Grundform::Prisma: {
        Befehl befehl(k.undo, "brushPrism -sides " + std::to_string(seiten) + " -axis " + std::to_string(achse));
        prismaBauen(b, grenzen, seiten, achse, shader, projektion, kt, p);
        break;
    }
    case Grundform::Kegel: {
        Befehl befehl(k.undo, "brushCone -sides " + std::to_string(seiten) + " -axis " + std::to_string(achse));
        kegelBauen(b, grenzen, seiten, achse, shader, projektion, kt, p);
        break;
    }
    case Grundform::Kugel: {
        Befehl befehl(k.undo, "brushSphere -sides " + std::to_string(seiten));
        kugelBauen(b, grenzen, seiten, shader, projektion, kt, p);
        break;
    }
    case Grundform::Fels: {
        Befehl befehl(k.undo, "brushRock -sides " + std::to_string(seiten));
        felsBauen(b, grenzen, seiten, shader, projektion, kt, p);
        break;
    }
    case Grundform::Ikosaeder: {
        Befehl befehl(k.undo, "brushIcosahedron -subdivisions " + std::to_string(seiten));
        ikosaederBauen(b, grenzen, seiten, option, shader, projektion, kt);
        break;
    }
    }
}

void grundformAuswahl(Karte& k, Grundform art, std::size_t seiten, bool option, std::size_t achse, const std::string& shader, Protokoll& p) {
    if (k.auswahl.anzahl() == 0) { return; }
    const AuswahlElement& el = k.auswahl.elemente().back();   // ultimateSelected
    const auto pinsel = std::dynamic_pointer_cast<Pinsel>(el.primitiv.lock());
    if (!pinsel) { return; }
    const Kasten grenzen = pinsel->grenzen(k.kontext);   // "copy bounds because the brush will be modified"
    grundformBauen(k, *pinsel, art, grenzen, seiten, option, achse, shader, p);
}

}  // namespace bhed::karte
