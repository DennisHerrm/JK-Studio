// kurven.cpp - Karten-Editor, Stufe 9 Punkt 7: Kurven-Werkzeug (siehe kurven.h)
//
// Portiert aus NetRadiant-Custom 20260114 (Patch, der Zylinder-Kreis aus 9 Kontrollpunkten);
// die Kurve ist neu.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
#include "bhed/karte/kurven.h"

#include "bhed/karte/karte.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/rueckgaengig.h"

#include <cmath>
#include <cstdio>
#include <functional>

namespace bhed::karte {

namespace {

Vektor3f catmull(const Vektor3f& p0, const Vektor3f& p1, const Vektor3f& p2, const Vektor3f& p3, float t) {
    const float t2 = t * t, t3 = t2 * t;
    Vektor3f r;
    for (std::size_t i = 0; i < 3; ++i) {
        r[i] = 0.5F * ((2.0F * p1[i]) + (-p0[i] + p2[i]) * t + (2.0F * p0[i] - 5.0F * p1[i] + 4.0F * p2[i] - p3[i]) * t2 +
                       (-p0[i] + 3.0F * p1[i] - 3.0F * p2[i] + p3[i]) * t3);
    }
    return r;
}

float laenge3(const Vektor3f& v) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }
Vektor3f norm3(const Vektor3f& v) {
    const float l = laenge3(v);
    return l > 0 ? Vektor3f(v[0] / l, v[1] / l, v[2] / l) : Vektor3f(1, 0, 0);
}
Vektor3f kreuz3(const Vektor3f& a, const Vektor3f& b) { return Vektor3f(a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]); }

struct Rahmen {
    Vektor3f p, seite, oben;
    float weg = 0;   // Bogenlaenge bis hier
};

std::vector<Rahmen> rahmen(const std::vector<Vektor3f>& pts) {
    std::vector<Rahmen> r(pts.size());
    float weg = 0;
    for (std::size_t i = 0; i < pts.size(); ++i) {
        const Vektor3f a = pts[i > 0 ? i - 1 : i], b = pts[i + 1 < pts.size() ? i + 1 : i];
        const Vektor3f t = norm3(b - a);
        Vektor3f s = kreuz3(t, Vektor3f(0, 0, 1));
        s = laenge3(s) < 1e-4F ? Vektor3f(1, 0, 0) : norm3(s);
        if (i > 0) { weg += laenge3(pts[i] - pts[i - 1]); }
        r[i] = {pts[i], s, kreuz3(s, t), weg};
    }
    return r;
}

std::shared_ptr<Patch> patchAus(std::size_t zeilen, std::size_t spalten, const std::function<Vektor3f(std::size_t, std::size_t)>& ort,
                                const std::function<Vektor2f(std::size_t, std::size_t)>& tex, const std::string& shader) {
    auto p = std::make_shared<Patch>();
    p->setzeGroesse(spalten, zeilen);
    for (std::size_t r = 0; r < zeilen; ++r) {
        for (std::size_t c = 0; c < spalten; ++c) {
            p->punkt(r, c).vertex = ort(r, c);
            p->punkt(r, c).texcoord = tex(r, c);
        }
    }
    p->setzeShader(shader);
    p->punkteGeaendert();
    return p;
}

// Querschnitt entlang der Kurve: je Spalte ein Punkt (x seitlich, y nach oben) im Rahmen
std::shared_ptr<Patch> profil(const std::vector<Rahmen>& rs, const std::vector<std::array<float, 2>>& q, const std::string& shader) {
    std::vector<float> u(q.size(), 0.0F);   // Laenge im Querschnitt fuer s
    for (std::size_t c = 1; c < q.size(); ++c) { u[c] = u[c - 1] + std::hypot(q[c][0] - q[c - 1][0], q[c][1] - q[c - 1][1]); }
    return patchAus(
        rs.size(), q.size(),
        [&](std::size_t r, std::size_t c) { return rs[r].p + rs[r].seite * q[c][0] + rs[r].oben * q[c][1]; },
        [&](std::size_t r, std::size_t c) { return Vektor2f(u[c] / 128.0F, rs[r].weg / 128.0F); }, shader);
}

std::vector<std::array<float, 2>> kreis(float r, float mx, float my) {
    std::vector<std::array<float, 2>> q;
    for (int k = 8; k >= 0; --k) {   // eCylinder: auf dem Kreis (gerade) und Ecken des Quadrats (ungerade); im Uhrzeigersinn = Vorderseite aussen
        const float a = static_cast<float>(k) * 3.14159265F / 4.0F;
        const float rr = (k % 2 == 1) ? r * std::sqrt(2.0F) : r;
        q.push_back({mx + std::cos(a) * rr, my + std::sin(a) * rr});
    }
    return q;
}

}  // namespace

std::vector<Vektor3f> kurvePunkte(const std::vector<Vektor3f>& kp, float schritt) {
    std::vector<Vektor3f> dicht;
    if (kp.size() < 2) { return dicht; }
    for (std::size_t i = 0; i + 1 < kp.size(); ++i) {
        const Vektor3f p0 = kp[i > 0 ? i - 1 : i], p1 = kp[i], p2 = kp[i + 1], p3 = kp[i + 2 < kp.size() ? i + 2 : i + 1];
        for (int s = 0; s < 32; ++s) { dicht.push_back(catmull(p0, p1, p2, p3, static_cast<float>(s) / 32.0F)); }
    }
    dicht.push_back(kp.back());
    std::vector<float> weg(dicht.size(), 0.0F);
    for (std::size_t i = 1; i < dicht.size(); ++i) { weg[i] = weg[i - 1] + laenge3(dicht[i] - dicht[i - 1]); }
    const float l = weg.back();
    int n = static_cast<int>(std::lround(l / std::max(1.0F, schritt)));
    if (n < 2) { n = 2; }
    if (n % 2 == 1) { ++n; }   // n Abschnitte, gerade: n + 1 Punkte, ungerade Zahl
    std::vector<Vektor3f> r;
    std::size_t j = 0;
    for (int i = 0; i <= n; ++i) {
        const float ziel = l * static_cast<float>(i) / static_cast<float>(n);
        while (j + 1 < weg.size() && weg[j + 1] < ziel) { ++j; }
        if (j + 1 >= weg.size()) {
            r.push_back(dicht.back());
            continue;
        }
        const float t = weg[j + 1] > weg[j] ? (ziel - weg[j]) / (weg[j + 1] - weg[j]) : 0.0F;
        r.push_back(dicht[j] + (dicht[j + 1] - dicht[j]) * t);
    }
    return r;
}

std::vector<std::shared_ptr<Patch>> kurvenPatches(const std::vector<Vektor3f>& kontroll, KurvenArt art, const KurvenOptionen& o) {
    std::vector<std::shared_ptr<Patch>> v;
    const std::vector<Vektor3f> pts = kurvePunkte(kontroll, o.schritt);
    if (pts.size() < 3) { return v; }
    const std::vector<Rahmen> rs = rahmen(pts);
    const float h = o.breite * 0.5F;
    switch (art) {
    case KurvenArt::Weg:
        v.push_back(profil(rs, {{-h, 0}, {0, 0}, {h, 0}}, o.shader));   // Vorderseite oben (links der Laufrichtung des Querschnitts)
        break;
    case KurvenArt::Rohr:
        v.push_back(profil(rs, kreis(o.radius, 0, o.rohrHoehe), o.shader));
        break;
    case KurvenArt::Gelaender:
        v.push_back(profil(rs, kreis(o.gelaenderRadius, -h, o.gelaenderHoehe), o.shader));
        v.push_back(profil(rs, kreis(o.gelaenderRadius, h, o.gelaenderHoehe), o.shader));
        break;
    case KurvenArt::Bruecke: {
        const float d = o.dicke;
        // Deck mit Seiten: Ecken als Punkte auf der Kurve, dazwischen die Kontrollpunkte
        v.push_back(profil(rs, {{-h, -d}, {-h, -d * 0.5F}, {-h, 0}, {0, 0}, {h, 0}, {h, -d * 0.5F}, {h, -d}}, o.shader));
        v.push_back(profil(rs, {{h, -d}, {0, -d}, {-h, -d}}, o.shader));   // Unterseite
        break;
    }
    case KurvenArt::PfadEcken:
    case KurvenArt::Wegpunkte:
        break;
    }
    return v;
}

std::size_t kurveEinfuegen(Karte& k, const std::vector<Vektor3f>& kontroll, KurvenArt art, const KurvenOptionen& o, const std::string& praefix) {
    if (kontroll.size() < 2) { return 0; }
    Befehl undo(k.undo, "curveTool");
    k.auswahl.leeren();
    std::size_t n = 0;
    if (art == KurvenArt::PfadEcken || art == KurvenArt::Wegpunkte) {
        const std::vector<Vektor3f> pts = kurvePunkte(kontroll, o.schritt);
        const std::string klasse = art == KurvenArt::PfadEcken ? "path_corner" : "waypoint";
        for (std::size_t i = 0; i < pts.size(); ++i) {
            auto e = k.neueEntitaet(klasse, false);
            char z[96];
            std::snprintf(z, sizeof(z), "%g %g %g", static_cast<double>(pts[i][0]), static_cast<double>(pts[i][1]),
                          static_cast<double>(pts[i][2] + (art == KurvenArt::Wegpunkte ? 24.0F : 0.0F)));
            e->setzen("origin", z);
            e->setzen("targetname", praefix + std::to_string(i + 1));
            if (i + 1 < pts.size()) { e->setzen("target", praefix + std::to_string(i + 2)); }
            k.entitaetEinfuegen(e);
            k.auswahl.waehlen(e);
            ++n;
        }
        return n;
    }
    for (const auto& p : kurvenPatches(kontroll, art, o)) {
        p->schicht = static_cast<int>(k.aktuelleSchicht);
        k.welt()->kindEinfuegen(p);
        k.auswahl.waehlen(k.welt(), p);
        ++n;
    }
    return n;
}

}  // namespace bhed::karte
