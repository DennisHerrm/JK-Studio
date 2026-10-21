// wicklung.h - Karten-Editor: Polygone einer Pinselflaeche ("Windings").
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): radiant/winding.h und
// radiant/winding.cpp.
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
//
// Eine Wicklung ist das konvexe Polygon, das eine Ebene mit den anderen Ebenen
// des Pinsels bildet. Jeder Eckpunkt merkt sich die Nachbarflaeche der Kante
// zum naechsten Punkt (adjacent) - daraus baut pinsel.cpp Kanten- und
// Eckenringe. Eckpunkte in double, Texturkoordinaten in float, wie NRC.
#ifndef BHED_KARTE_WICKLUNG_H
#define BHED_KARTE_WICKLUNG_H

#include "bhed/karte/mathe.h"

#include <cstddef>
#include <vector>

namespace bhed::karte {

// ProjectionAxis (winding.h:32-37)
enum class Projektionsachse { X = 0, Y = 1, Z = 2 };

constexpr float kProjektionsachseEps = static_cast<float>(0.0001);   // ProjectionAxisEpsilon

// projectionaxis_better
inline bool projektionsachseBesser(float achse, float andere) {
    return std::fabs(achse) > std::fabs(andere) + kProjektionsachseEps;
}
// projectionaxis_for_normal: Vorrang Z > X > Y.
inline Projektionsachse projektionsachseFuerNormale(const Vektor3f& n) {
    return projektionsachseBesser(n[1], n[0])
               ? (projektionsachseBesser(n[1], n[2]) ? Projektionsachse::Y : Projektionsachse::Z)
               : (projektionsachseBesser(n[0], n[2]) ? Projektionsachse::X : Projektionsachse::Z);
}

// indexremap_t / indexremap_for_projectionaxis
struct Indexumbau {
    std::size_t x, y, z;
};
inline Indexumbau indexumbauFuerAchse(Projektionsachse a) {
    switch (a) {
    case Projektionsachse::X: return {1, 2, 0};
    case Projektionsachse::Y: return {2, 0, 1};
    default: return {0, 1, 2};
    }
}

// PlaneClassification
enum EbenenSeite { kVorne = 0, kHinten = 1, kAuf = 2 };

constexpr std::size_t kMaxPunkteJeWicklung = 64;   // MAX_POINTS_ON_WINDING
constexpr std::size_t kMaxFlaechen = 1024;         // c_brush_maxFaces

// WindingVertex
struct WicklungsPunkt {
    Vektor3d vertex;
    Vektor2f texcoord;
    Vektor3f tangent;
    Vektor3f bitangent;
    std::size_t adjacent = 0;
};

// Winding: feste Kapazitaet (points), davon numpoints benutzt.
struct Wicklung {
    std::size_t numpoints = 0;
    std::vector<WicklungsPunkt> points;

    Wicklung() = default;
    explicit Wicklung(std::size_t n) : points(n) {}
    void resize(std::size_t n) {   // Winding::resize: neue Kapazitaet, leer
        points.resize(n);
        numpoints = 0;
    }
    WicklungsPunkt* begin() { return points.data(); }
    const WicklungsPunkt* begin() const { return points.data(); }
    WicklungsPunkt* end() { return points.data() + numpoints; }
    const WicklungsPunkt* end() const { return points.data() + numpoints; }
    WicklungsPunkt& operator[](std::size_t i) { return points[i]; }
    const WicklungsPunkt& operator[](std::size_t i) const { return points[i]; }
    void push_back(const WicklungsPunkt& p) { points[numpoints++] = p; }
    // Winding::erase - schiebt die folgenden Punkte nach vorn.
    void erase(WicklungsPunkt* p) {
        for (WicklungsPunkt* i = p + 1; i != end(); p = i, ++i) { *p = *i; }
        --numpoints;
    }
};

// FixedWindingVertex / FixedWinding: Arbeitsform beim Zuschneiden.
struct FesterPunkt {
    Vektor3d vertex;
    Strahld edge;
    std::size_t adjacent;
    FesterPunkt(const Vektor3d& v, const Strahld& e, std::size_t a) : vertex(v), edge(e), adjacent(a) {}
};
struct FesteWicklung {
    std::vector<FesterPunkt> points;
    FesteWicklung() { points.reserve(kMaxPunkteJeWicklung); }
    void clear() { points.clear(); }
    void push_back(const FesterPunkt& p) { points.push_back(p); }
    [[nodiscard]] std::size_t size() const { return points.size(); }
    FesterPunkt& operator[](std::size_t i) { return points[i]; }
    const FesterPunkt& operator[](std::size_t i) const { return points[i]; }
    FesterPunkt& back() { return points.back(); }
    const FesterPunkt& back() const { return points.back(); }
};

// Winding_forFixedWinding
void wicklungAusFester(Wicklung& w, const FesteWicklung& f);

// Winding_wrap / Winding_next
inline std::size_t wicklungRing(const Wicklung& w, std::size_t i) { return i % w.numpoints; }
inline std::size_t wicklungNaechster(const Wicklung& w, std::size_t i) { return wicklungRing(w, ++i); }

// Winding_createInfinite
void wicklungUnendlich(FesteWicklung& w, const Ebene3d& e, double unendlich);

constexpr double kAufEps = 1.0 / (1 << 8);   // ON_EPSILON

// Edge_isDegenerate
inline bool kanteEntartet(const Vektor3d& a, const Vektor3d& b) {
    return laengeQuadrat(b - a) < (kAufEps * kAufEps);
}

// Winding_Clip
void wicklungSchneiden(const FesteWicklung& w, const Ebene3d& ebene, const Ebene3d& schnitt, std::size_t nachbar,
                       FesteWicklung& ergebnis);

// brushsplit_t
struct PinselTeilung {
    std::size_t counts[3] = {0, 0, 0};
    PinselTeilung& operator+=(const PinselTeilung& o) {
        counts[0] += o.counts[0];
        counts[1] += o.counts[1];
        counts[2] += o.counts[2];
        return *this;
    }
};
PinselTeilung wicklungEinordnen(const Wicklung& w, const Ebene3d& e);                     // Winding_ClassifyPlane
void punktEinordnen(const Vektor3d& v, const Ebene3d& e, PinselTeilung& t);              // WindingVertex_ClassifyPlane
bool wicklungenKonkav(const Wicklung& a, const Wicklung& b, const Ebene3d& ea, const Ebene3d& eb);   // Winding_PlanesConcave
bool wicklungPruefeEbene(const Wicklung& w, const Ebene3d& e, bool umgedreht);           // Winding_TestPlane
std::size_t wicklungFindeNachbar(const Wicklung& w, std::size_t flaeche);                 // Winding_FindAdjacent
std::size_t wicklungGegenueber(const Wicklung& w, std::size_t index, std::size_t anderer);   // Winding_Opposite
std::size_t wicklungGegenueber(const Wicklung& w, std::size_t index);                     // Winding_Opposite
void wicklungSchwerpunkt(const Wicklung& w, const Ebene3d& e, Vektor3f& schwerpunkt);     // Winding_Centroid

}  // namespace bhed::karte
#endif
