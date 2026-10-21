// wicklung.cpp - Karten-Editor: Polygone einer Pinselflaeche.
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): radiant/winding.cpp.
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
#include "bhed/karte/wicklung.h"

#include <algorithm>

namespace bhed::karte {

// Winding_forFixedWinding (winding.h:207-217): nur Punkt und Nachbar.
void wicklungAusFester(Wicklung& w, const FesteWicklung& f) {
    w.resize(f.size());
    w.numpoints = f.size();
    for (std::size_t i = 0; i < f.size(); ++i) {
        w[i].vertex[0] = f[i].vertex[0];
        w[i].vertex[1] = f[i].vertex[1];
        w[i].vertex[2] = f[i].vertex[2];
        w[i].adjacent = f[i].adjacent;
    }
}

// Winding_createInfinite (winding.cpp:126-214, der #else-Zweig). "unendlich"
// so klein wie moeglich halten, das verbessert die Genauigkeit beim Schneiden.
void wicklungUnendlich(FesteWicklung& w, const Ebene3d& e, double unendlich) {
    const auto normal = e.normal();
    const auto maxi = groessteBetragsKomponente(normal);

    if (!std::isnormal(normal[maxi])) {
        // NRC: globalErrorStream() << "invalid plane\n";
        return;
    }

    const Vektor3d vup0 = (maxi == 2) ? Vektor3d(0, -normal[2], normal[1]) : Vektor3d(-normal[1], normal[0], 0);
    const Vektor3d vright0 = kreuz(vup0, normal);
    const Vektor3d org = normal * e.dist();

    const Vektor3d vup = vup0 * unendlich * 2.22;
    const Vektor3d vright = vright0 * unendlich * 2.22;

    // ein riesiges achsparalleles Viereck auf die Ebene legen
    Strahld ray;
    ray.origin = org - vright + vup;
    ray.direction = normiert(vright0);
    w.push_back(FesterPunkt(ray.origin, ray, kMaxFlaechen));
    ray.origin = org + vright + vup;
    ray.direction = normiert(-vup0);
    w.push_back(FesterPunkt(ray.origin, ray, kMaxFlaechen));
    ray.origin = org + vright - vup;
    ray.direction = normiert(-vright0);
    w.push_back(FesterPunkt(ray.origin, ray, kMaxFlaechen));
    ray.origin = org - vright - vup;
    ray.direction = normiert(vup0);
    w.push_back(FesterPunkt(ray.origin, ray, kMaxFlaechen));
}

namespace {

// Winding_ClassifyDistance (winding.cpp:217-225)
EbenenSeite abstandEinordnen(const double abstand, const double eps) {
    if (abstand > eps) { return kVorne; }
    if (abstand < -eps) { return kHinten; }
    return kAuf;
}

constexpr double kAufEpsSchnitt = 1.0 / (1 << 12);   // ON_EPSILON_CLIP (winding.cpp:256)

}  // namespace

// Winding_TestPlane (winding.cpp:230-235): true, wenn
//   !umgedreht und die Wicklung ganz HINTEN oder AUF liegt,
//   umgedreht und ganz VORNE oder AUF.
bool wicklungPruefeEbene(const Wicklung& w, const Ebene3d& e, bool umgedreht) {
    const int test = umgedreht ? kHinten : kVorne;
    return std::none_of(w.begin(), w.end(), [&](const WicklungsPunkt& v) {
        return test == abstandEinordnen(abstandZuPunkt(e, v.vertex), kAufEps);
    });
}

// Winding_PlanesConcave (winding.cpp:238-240)
bool wicklungenKonkav(const Wicklung& a, const Wicklung& b, const Ebene3d& ea, const Ebene3d& eb) {
    return !wicklungPruefeEbene(a, eb, false) || !wicklungPruefeEbene(b, ea, false);
}

// Winding_ClassifyPlane (winding.cpp:242-249)
PinselTeilung wicklungEinordnen(const Wicklung& w, const Ebene3d& e) {
    PinselTeilung t;
    for (const auto& v : w) {
        ++t.counts[abstandEinordnen(abstandZuPunkt(e, v.vertex), kAufEps)];
    }
    return t;
}

// WindingVertex_ClassifyPlane (winding.cpp:251-253)
void punktEinordnen(const Vektor3d& v, const Ebene3d& e, PinselTeilung& t) {
    ++t.counts[abstandEinordnen(abstandZuPunkt(e, v), kAufEps)];
}

// Winding_Clip (winding.cpp:262-318): die Wicklung auf "ebene" an "schnitt"
// zuschneiden. Ganz vorne -> unveraendert, ganz hinten -> leer. Die neue Kante
// auf der Schnittebene bekommt "nachbar".
void wicklungSchneiden(const FesteWicklung& w, const Ebene3d& ebene, const Ebene3d& schnitt, std::size_t nachbar,
                       FesteWicklung& ergebnis) {
    EbenenSeite seite = abstandEinordnen(abstandZuPunkt(schnitt, w.back().vertex), kAufEpsSchnitt);
    EbenenSeite naechsteSeite = kAuf;
    // fuer jede Kante
    for (std::size_t next = 0, i = w.size() - 1; next != w.size(); i = next, ++next, seite = naechsteSeite) {
        naechsteSeite = abstandEinordnen(abstandZuPunkt(schnitt, w[next].vertex), kAufEpsSchnitt);
        const FesterPunkt& vertex = w[i];

        // erster Punkt der Kante liegt AUF der Ebene
        if (seite == kAuf) {
            if (naechsteSeite == kHinten) {
                // diese Kante liegt auf der Schnittebene
                ergebnis.push_back(FesterPunkt(vertex.vertex, ebenenSchnitt(ebene, schnitt), nachbar));
            } else {
                ergebnis.push_back(vertex);
            }
            continue;
        }

        // erster Punkt VORNE: uebernehmen
        if (seite == kVorne) {
            ergebnis.push_back(vertex);
        }
        if (naechsteSeite == kAuf) {
            continue;
        }
        if (naechsteSeite == seite) {
            continue;
        }
        if (seite == kVorne && w.size() == 2) {
            continue;
        }
        // VORNE nach HINTEN oder umgekehrt: Schnittpunkt anhaengen
        const Vektor3d mid(strahlSchnittEbene(vertex.edge, schnitt));
        if (seite == kVorne) {
            ergebnis.push_back(FesterPunkt(mid, ebenenSchnitt(ebene, schnitt), nachbar));
        } else {
            ergebnis.push_back(FesterPunkt(mid, vertex.edge, vertex.adjacent));
        }
    }
}

// Winding_FindAdjacent (winding.cpp:320-329)
std::size_t wicklungFindeNachbar(const Wicklung& w, std::size_t flaeche) {
    for (std::size_t i = 0; i < w.numpoints; ++i) {
        if (w[i].adjacent == flaeche) { return i; }
    }
    return kMaxFlaechen;
}

// Winding_Opposite (winding.cpp:331-353): der Punkt mit dem groessten Abstand
// zur Kante index->anderer.
std::size_t wicklungGegenueber(const Wicklung& w, std::size_t index, std::size_t anderer) {
    double besterAbstand = 0;
    std::size_t besterIndex = kMaxFlaechen;
    const Strahld kante(strahlAusPunkten(w[index].vertex, w[anderer].vertex));
    for (std::size_t i = 0; i < w.numpoints; ++i) {
        if (i == index || i == anderer) { continue; }
        const double d = strahlAbstandQuadrat(kante, w[i].vertex);
        if (d > besterAbstand) {
            besterAbstand = d;
            besterIndex = i;
        }
    }
    return besterIndex;
}

std::size_t wicklungGegenueber(const Wicklung& w, std::size_t index) {
    return wicklungGegenueber(w, index, wicklungNaechster(w, index));
}

// Winding_Centroid (winding.cpp:360-382): Flaechenschwerpunkt in der
// Projektionsebene, die dritte Koordinate per Strahl auf die Ebene.
void wicklungSchwerpunkt(const Wicklung& w, const Ebene3d& e, Vektor3f& schwerpunkt) {
    double area2 = 0;
    double xSum = 0;
    double ySum = 0;
    const Projektionsachse achse = projektionsachseFuerNormale(e.normal());
    const Indexumbau r = indexumbauFuerAchse(achse);
    for (std::size_t i = w.numpoints - 1, j = 0; j < w.numpoints; i = j, ++j) {
        const double ai = w[i].vertex[r.x] * w[j].vertex[r.y] - w[j].vertex[r.x] * w[i].vertex[r.y];
        area2 += ai;
        xSum += (w[j].vertex[r.x] + w[i].vertex[r.x]) * ai;
        ySum += (w[j].vertex[r.y] + w[i].vertex[r.y]) * ai;
    }
    schwerpunkt[r.x] = static_cast<float>(xSum / (3 * area2));
    schwerpunkt[r.y] = static_cast<float>(ySum / (3 * area2));
    {
        Strahlf ray(Vektor3f(0, 0, 0), Vektor3f(0, 0, 0));
        ray.origin[r.x] = schwerpunkt[r.x];
        ray.origin[r.y] = schwerpunkt[r.y];
        ray.direction[r.z] = 1;
        schwerpunkt[r.z] = static_cast<float>(strahlAbstandZuEbene(ray, e));
    }
}

}  // namespace bhed::karte
