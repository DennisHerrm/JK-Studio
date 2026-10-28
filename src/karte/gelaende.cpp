// gelaende.cpp - Karten-Editor, Stufe 9 Punkt 9: Gelaende formen (siehe gelaende.h)
//
// Portiert aus NetRadiant-Custom 20260114 (Patch, controlPointsChanged, Undo); das Formen ist neu.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
#include "bhed/karte/gelaende.h"

#include "bhed/karte/karte.h"
#include "bhed/karte/patch.h"

#include <cmath>
#include <vector>

namespace bhed::karte {

std::size_t formenAnwenden(Patch& p, const Vektor3f& mitte, const FormPinsel& f, float ebeneZ) {
    const std::size_t w = p.breite(), h = p.hoehe();
    if (w == 0 || h == 0 || f.radius <= 0) { return 0; }
    std::vector<float> alt(w * h);
    for (std::size_t i = 0; i < w * h; ++i) { alt[i] = p.punkte()[i].vertex[2]; }
    std::size_t n = 0;
    std::vector<PatchPunkt> neu = p.punkte();
    for (std::size_t r = 0; r < h; ++r) {
        for (std::size_t c = 0; c < w; ++c) {
            const PatchPunkt& q = p.punkt(r, c);
            const float d = std::hypot(q.vertex[0] - mitte[0], q.vertex[1] - mitte[1]);
            if (d >= f.radius) { continue; }
            const float g = (1.0F - d / f.radius) * (1.0F - d / f.radius);
            float z = q.vertex[2];
            switch (f.art) {
            case FormArt::Heben: z += f.staerke * g; break;
            case FormArt::Senken: z -= f.staerke * g; break;
            case FormArt::Glaetten: {
                float s = 0;
                int m = 0;
                if (r > 0) { s += alt[(r - 1) * w + c]; ++m; }
                if (r + 1 < h) { s += alt[(r + 1) * w + c]; ++m; }
                if (c > 0) { s += alt[r * w + c - 1]; ++m; }
                if (c + 1 < w) { s += alt[r * w + c + 1]; ++m; }
                if (m > 0) { z += (s / static_cast<float>(m) - z) * std::min(1.0F, g * f.staerke / 8.0F); }
                break;
            }
            case FormArt::Ebnen: z += (ebeneZ - z) * std::min(1.0F, g * f.staerke / 8.0F); break;
            }
            if (z == q.vertex[2]) { continue; }
            neu[r * w + c].vertex[2] = z;
            ++n;
        }
    }
    if (n > 0) { p.punkteSetzen(neu); }   // undoSave, setzen, controlPointsChanged
    return n;
}

std::size_t formenAufKarte(Karte& k, const Vektor3f& mitte, const FormPinsel& f, float ebeneZ) {
    std::size_t n = 0;
    for (const auto& e : k.entitaeten()) {
        for (const auto& pr : e->kinder()) {
            if (!pr || pr->art() != Primitiv::Art::Patch || !pr->sichtbar()) { continue; }
            auto& p = static_cast<Patch&>(*pr);
            const Kasten& b = p.grenzen();
            if (std::fabs(b.origin[0] - mitte[0]) > b.extents[0] + f.radius || std::fabs(b.origin[1] - mitte[1]) > b.extents[1] + f.radius) { continue; }
            n += formenAnwenden(p, mitte, f, ebeneZ);
        }
    }
    return n;
}

}  // namespace bhed::karte
