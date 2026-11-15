// cwimport.cpp - Karten-Editor, CW-IMPORT: Teil 2 der Clone-Wars-Kette (siehe cwimport.h). Eigene Funktionen, nicht in
// NetRadiant-Custom; die Masse der Spielfigur aus OpenJK und Movie Duels (Spielfigur, optimierer.h).
//
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//
//   OpenJK is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 2
//   as published by the Free Software Foundation (LICENSE).
#include "bhed/karte/cwimport.h"

#include <cstdio>

namespace bhed::karte {

namespace {

std::string zahl(double v) {
    char t[32];
    std::snprintf(t, sizeof(t), "%g", v);
    return t;
}

}  // namespace

FigurZeichnung testfigurZeichnung(const FigurLage& l, const Spielfigur& f, const CwMasse& m) {
    FigurZeichnung z;
    const Vektor3f& o = l.ursprung;
    const auto v = [&o](double x, double y, double h) {
        return Vektor3f(o.x() + static_cast<float>(x), o.y() + static_cast<float>(y), o.z() + static_cast<float>(h));
    };
    const auto linie = [&z](const Vektor3f& a, const Vektor3f& b, FigurArt art) { z.linien.push_back({a, b, art}); };
    // Ein Rechteck in der Hoehe h (waagerecht, halbe Seite s)
    const auto ring = [&](double s, double h, FigurArt art) {
        linie(v(-s, -s, h), v(s, -s, h), art);
        linie(v(s, -s, h), v(s, s, h), art);
        linie(v(s, s, h), v(-s, s, h), art);
        linie(v(-s, s, h), v(-s, -s, h), art);
    };
    const double h = f.halb;
    const double unten = f.unten;
    const double oben = l.geduckt ? f.geduckt : f.oben;
    // der Kasten (mins/maxs): zwei Ringe und vier Kanten
    ring(h, unten, FigurArt::Kasten);
    ring(h, oben, FigurArt::Kasten);
    for (const double x : {-h, h}) {
        for (const double y : {-h, h}) { linie(v(x, y, unten), v(x, y, oben), FigurArt::Kasten); }
    }
    z.mins = v(-h, -h, unten);
    z.maxs = v(h, h, oben);
    z.texte.push_back({v(h, h, oben), zahl(oben - unten), FigurArt::Kasten});
    // der Ursprung (Hueftenhoehe)
    linie(v(-4, 0, 0), v(4, 0, 0), FigurArt::Ursprung);
    linie(v(0, -4, 0), v(0, 4, 0), FigurArt::Ursprung);
    linie(v(0, 0, -4), v(0, 0, 4), FigurArt::Ursprung);
    if (!l.marken) { return z; }
    // die andere Hoehe: stehend die Oberkante geduckt, geduckt die Oberkante stehend (so viel Platz braucht das Aufstehen)
    const double andere = l.geduckt ? f.oben : f.geduckt;
    ring(h, andere, FigurArt::Hoehe);
    z.texte.push_back({v(-h, h, andere), zahl(andere - unten), FigurArt::Hoehe});
    // STEPSIZE ueber den Fuessen, etwas breiter als der Kasten
    ring(h + 8.0, unten + f.stufe, FigurArt::Stufe);
    z.texte.push_back({v(h + 8.0, -h - 8.0, unten + f.stufe), "STEPSIZE " + zahl(f.stufe), FigurArt::Stufe});
    // Tuer und Gang: Rahmen um die Figur, in der Ebene x = Ursprung (quer: y = Ursprung), unten auf den Fuessen
    const auto rahmen = [&](double breite, double hoehe, FigurArt art) {
        const double b = breite * 0.5;
        const auto p = [&](double seite, double hh) { return l.quer ? v(seite, 0, hh) : v(0, seite, hh); };
        linie(p(-b, unten), p(-b, unten + hoehe), art);
        linie(p(b, unten), p(b, unten + hoehe), art);
        linie(p(-b, unten + hoehe), p(b, unten + hoehe), art);
        linie(p(-b, unten), p(b, unten), art);
        z.texte.push_back({p(b, unten + hoehe), zahl(hoehe) + " x " + zahl(breite), art});
    };
    rahmen(m.tuerBreite, m.tuerHoehe, FigurArt::Tuer);
    rahmen(m.gangBreite, m.gangHoehe, FigurArt::Gang);
    return z;
}

Vektor3f testfigurAufBoden(const Vektor3f& boden, const Spielfigur& f) {
    return Vektor3f(boden.x(), boden.y(), boden.z() - static_cast<float>(f.unten));
}

const char* testfigurSequenz(bool geduckt) { return geduckt ? "BOTH_CROUCH1" : "BOTH_STAND1"; }

}  // namespace bhed::karte
