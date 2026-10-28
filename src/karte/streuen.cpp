// streuen.cpp - Karten-Editor, Stufe 9 Punkt 8: Modelle verteilen (siehe streuen.h)
//
// Portiert aus OpenJK (die Grenze MAX_MISC_ENTS, MIN_WALK_NORMAL); die Streuung ist neu.
//
//   Copyright (C) 1999-2005 Id Software, Inc.; 2000-2013 Raven Software; 2013-2026 OpenJK contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
#include "bhed/karte/streuen.h"

#include "bhed/karte/karte.h"
#include "bhed/karte/rueckgaengig.h"

#include <cmath>
#include <cstdio>
#include <random>

namespace bhed::karte {

StreuErgebnis streuOrte(const KollisionsKarte& koll, const Vektor3f& mn, const Vektor3f& mx, const StreuOptionen& o, int erlaubt) {
    StreuErgebnis r;
    int ziel = o.anzahl;
    if (erlaubt < ziel) {
        ziel = std::max(0, erlaubt);
        r.begrenzt = true;
    }
    std::mt19937 zufall(o.startwert);
    std::uniform_real_distribution<float> ux(mn[0], mx[0]), uy(mn[1], mx[1]), ug(0.0F, 360.0F), us(o.skalaMin, o.skalaMax);
    const float null[3] = {0, 0, 0};
    for (int versuch = 0; versuch < o.anzahl * 30 && static_cast<int>(r.orte.size()) < ziel; ++versuch) {
        const float x = ux(zufall), y = uy(zufall);
        const float a[3] = {x, y, mx[2] + 1.0F};
        const float b[3] = {x, y, mn[2] - 1.0F};
        const KollisionsSpur s = kollisionsSpur(koll, a, null, null, b, 1U, 0);   // CONTENTS_SOLID
        if (s.startsolid || s.fraction >= 1.0F || s.normale[2] < o.minNormaleZ) { continue; }
        const Vektor3f p(s.endpos[0], s.endpos[1], s.endpos[2]);
        bool frei = true;
        for (const StreuOrt& q : r.orte) {
            const float dx = q.ort[0] - p[0], dy = q.ort[1] - p[1], dz = q.ort[2] - p[2];
            if (dx * dx + dy * dy + dz * dz < o.minAbstand * o.minAbstand) {
                frei = false;
                break;
            }
        }
        if (!frei) { continue; }
        StreuOrt so;
        so.ort = p;
        so.gier = o.drehen ? std::floor(ug(zufall)) : 0.0F;
        so.skala = o.skalaMax > o.skalaMin ? std::round(us(zufall) * 100.0F) / 100.0F : o.skalaMin;
        r.orte.push_back(so);
    }
    return r;
}

int miscModelStaticZahl(const Karte& k) {
    int n = 0;
    for (const auto& e : k.entitaeten()) { n += e->wert("classname") == "misc_model_static" ? 1 : 0; }
    return n;
}

std::size_t streuEinfuegen(Karte& k, const StreuErgebnis& e, const std::string& klasse, const std::string& modell) {
    if (e.orte.empty()) { return 0; }
    Befehl undo(k.undo, "scatterModels");
    k.auswahl.leeren();
    for (const StreuOrt& o : e.orte) {
        auto en = k.neueEntitaet(klasse, false);
        char z[96];
        std::snprintf(z, sizeof(z), "%g %g %g", static_cast<double>(o.ort[0]), static_cast<double>(o.ort[1]), static_cast<double>(o.ort[2]));
        en->setzen("origin", z);
        en->setzen("model", modell);
        if (o.gier != 0.0F) {
            std::snprintf(z, sizeof(z), "%g", static_cast<double>(o.gier));
            en->setzen("angle", z);
        }
        if (o.skala != 1.0F) {
            std::snprintf(z, sizeof(z), "%g", static_cast<double>(o.skala));
            en->setzen("modelscale", z);
        }
        k.entitaetEinfuegen(en);
        k.auswahl.waehlen(en);
    }
    return e.orte.size();
}

}  // namespace bhed::karte
