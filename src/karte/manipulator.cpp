// manipulator.cpp - Karten-Editor: Rechnung fuer die Manipulatoren.
// Portiert aus NetRadiant-Custom 20260114 (Quellen in manipulator.h).
//
//   Copyright (C) 2001-2006, William Joseph.
//   All Rights Reserved.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/manipulator.h"

#include "bhed/karte/xyansicht.h"

#include <cmath>
#include <utility>

namespace bhed::karte {

// --- Transformationen -------------------------------------------------------------

bool Transformationen::eins() const {
    return verschiebung == Vektor3f(0, 0, 0) && drehung[0] == 0 && drehung[1] == 0 && drehung[2] == 0 && drehung[3] == 1 &&
           skalierung == Vektor3f(1, 1, 1) && verzerrung == Verzerrung();
}

Matrix4 Transformationen::matrix() const { return transformAusKomponenten(verschiebung, drehung, skalierung, verzerrung); }

namespace {

constexpr double kHalbeWurzel2 = 0.70710678118654752440084436210485;   // c_half_sqrt2

// quaternion_component_is_90: NRC vergleicht ohne Betrag der Differenz.
bool komponenteIst90(float k) { return (std::fabs(k) - kHalbeWurzel2) < 0.001; }

}  // namespace

Matrix4 drehungAusQuaternionQuantisiert(const Quaternion& q) {
    if (q.y() == 0 && q.z() == 0 && komponenteIst90(q.x()) && komponenteIst90(q.w())) {
        return drehungSinCosX((q.x() > 0) ? 1.0F : -1.0F, 0);
    }
    if (q.x() == 0 && q.z() == 0 && komponenteIst90(q.y()) && komponenteIst90(q.w())) {
        return drehungSinCosY((q.y() > 0) ? 1.0F : -1.0F, 0);
    }
    if (q.x() == 0 && q.y() == 0 && komponenteIst90(q.z()) && komponenteIst90(q.w())) {
        return drehungSinCosZ((q.z() > 0) ? 1.0F : -1.0F, 0);
    }
    return drehungAusQuaternion(q);
}

Matrix4 transformAusKomponenten(const Vektor3f& t, const Quaternion& r, const Vektor3f& s, const Verzerrung& v) {
    Matrix4 m = drehungAusQuaternionQuantisiert(r);
    m[v.index] += v.wert;
    for (std::size_t i = 0; i < 3; ++i) {
        m[i] *= s.x();
        m[4 + i] *= s.y();
        m[8 + i] *= s.z();
    }
    m.tx() = t.x();
    m.ty() = t.y();
    m.tz() = t.z();
    return m;
}

Quaternion quaternionMal(const Quaternion& a, const Quaternion& b) {
    return Quaternion(a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1], a[3] * b[1] + a[1] * b[3] + a[2] * b[0] - a[0] * b[2],
                      a[3] * b[2] + a[2] * b[3] + a[0] * b[1] - a[1] * b[0], a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2]);
}

Quaternion quaternionInvers(const Quaternion& q) { return Quaternion(-q[0], -q[1], -q[2], q[3]); }

Quaternion quaternionAusEinheitsvektoren(const Vektor3f& von, const Vektor3f& nach) {
    return quaternionNormiert(Quaternion(kreuz(von, nach), static_cast<float>(1.0 + punkt(von, nach))));
}

// quaternion_for_unit_vectors_for_bounds (selection_mtable_translate.h:155-175)
Quaternion quaternionFuerKasten(const Vektor3f& axialVon, const Vektor3f& nach) {
    // Schritt fuer Schritt von der groessten Komponente zur kleinsten
    std::size_t ids[3] = {groessteBetragsKomponente(nach), 0, 0};
    ids[1] = (ids[0] + 1) % 3;
    ids[2] = (ids[0] + 2) % 3;
    if (std::fabs(nach[ids[2]]) > std::fabs(nach[ids[1]])) { std::swap(ids[2], ids[1]); }
    Vektor3f schritte[3] = {kAchsen[ids[0]] * std::copysign(1.0F, nach[ids[0]]), nach, nach};
    Quaternion drehung = quaternionAusEinheitsvektorenSicher(axialVon, schritte[0]);
    if (std::fabs(nach[ids[1]]) > 1e-6F) {
        schritte[1][ids[2]] = 0;
        normieren(schritte[1]);
        drehung = quaternionMal(quaternionAusEinheitsvektoren(schritte[0], schritte[1]), drehung);
        if (std::fabs(nach[ids[2]]) > 1e-6F) { drehung = quaternionMal(quaternionAusEinheitsvektoren(schritte[1], nach), drehung); }
    }
    return drehung;
}

Vektor3f eulerXyzGrad(const Matrix4& m) {
    const double a = std::asin(-m[2]);
    const double ca = std::cos(a);
    Vektor3f e;
    if (std::fabs(ca) > 0.005) {   // Gimbal lock?
        e = Vektor3f(static_cast<float>(std::atan2(m[6] / ca, m[10] / ca)), static_cast<float>(a),
                     static_cast<float>(std::atan2(m[1] / ca, m[0] / ca)));
    } else {
        e = Vektor3f(static_cast<float>(std::atan2(-m[9], m[5])), static_cast<float>(a), 0);
    }
    return Vektor3f(static_cast<float>(bogenZuGrad(e.x())), static_cast<float>(bogenZuGrad(e.y())),
                    static_cast<float>(bogenZuGrad(e.z())));
}

Matrix4 drehungEulerXyzGradQuantisiert(const Vektor3f& w) {
    if (w[0] == 0 && w[1] == 0) { return drehungZGrad(w[2]); }
    if (w[0] == 0 && w[2] == 0) { return drehungYGrad(w[1]); }
    if (w[1] == 0 && w[2] == 0) { return drehungXGrad(w[0]); }
    return drehungEulerXyzGrad(w);
}

Vektor3f winkelGedreht(const Vektor3f& winkel, const Quaternion& drehung) {
    const Vektor3f e = eulerXyzGrad(multipliziert(drehungAusQuaternionQuantisiert(drehung), drehungEulerXyzGradQuantisiert(winkel)));
    constexpr float kKleinster = 0.0001F;   // ANGLEKEY_SMALLEST
    const auto null = [](float f) { return std::fabs(f) < kKleinster ? 0.0F : f; };   // float_snapped_to_zero
    return Vektor3f(null(e.x()), null(e.y()), null(e.z()));
}

Vektor3f skalierungMitVorzeichen(const Matrix4& m) {
    return Vektor3f(static_cast<float>(std::copysign(laenge(m.x().vec3()), m.x().vec3().x())),
                    static_cast<float>(std::copysign(laenge(m.y().vec3()), m.y().vec3().y())),
                    static_cast<float>(std::copysign(laenge(m.z().vec3()), m.z().vec3().z())));
}

// aabb_snap_translation (selection_mtable_translate.h:27-44)
void kastenVerschiebungEinrasten(Vektor3f& v, const Kasten& grenzen, float fang) {
    const Vektor3f mx = grenzen.origin + grenzen.extents;
    const Vektor3f mn = grenzen.origin - grenzen.extents;
    for (std::size_t i = 0; i < 3; ++i) {
        if (std::fabs(v[i]) > 1e-2F) {
            const float nach1 = zahlEingerastet(mx[i] + v[i], fang);
            const float nach2 = zahlEingerastet(mn[i] + v[i], fang);
            const float abstand1 = std::fabs(std::fabs(mx[i] + v[i]) - std::fabs(nach1));
            const float abstand2 = std::fabs(std::fabs(mn[i] + v[i]) - std::fabs(nach2));
            v[i] = abstand2 > abstand1 ? nach1 - mx[i] : nach2 - mn[i];
        }
    }
}

Kasten kastenFuerGedrehtenKasten(const Kasten& k, const Matrix4& m) {
    return Kasten(transformierterPunkt(m, k.origin),
                  Vektor3f(std::fabs(m[0] * k.extents[0]) + std::fabs(m[4] * k.extents[1]) + std::fabs(m[8] * k.extents[2]),
                           std::fabs(m[1] * k.extents[0]) + std::fabs(m[5] * k.extents[1]) + std::fabs(m[9] * k.extents[2]),
                           std::fabs(m[2] * k.extents[0]) + std::fabs(m[6] * k.extents[1]) + std::fabs(m[10] * k.extents[2])));
}

// --- Pivot (libs/pivot.h) ----------------------------------------------------------

namespace {

// billboard_viewplaneOriented (#if 1)
Matrix4 blickebenenAusrichtung(const Matrix4& weltZuBild) {
    Matrix4 r = kEinheitsMatrix;
    const Vektor3f x = normiert(weltZuBild.x().vec3());
    const Vektor3f y = normiert(weltZuBild.y().vec3());
    const Vektor3f z = normiert(weltZuBild.z().vec3());
    r.y().vec3() = Vektor3f(x.y(), y.y(), z.y());
    r.z().vec3() = -Vektor3f(x.z(), y.z(), z.z());
    r.x().vec3() = normiert(kreuz(r.y().vec3(), r.z().vec3()));
    r.y().vec3() = kreuz(r.z().vec3(), r.x().vec3());
    return r;
}

// billboard_viewpointOriented (#if 1)
Matrix4 blickpunktAusrichtung(const Matrix4& weltZuBild) {
    const Matrix4 bildZuWelt = vollInvers(weltZuBild);
    Matrix4 r = kEinheitsMatrix;
    r.y().vec3() = normiert(bildZuWelt.y().vec3());
    r.z().vec3() = -normiert(bildZuWelt.z().vec3());
    r.x().vec3() = normiert(kreuz(r.y().vec3(), r.z().vec3()));
    r.y().vec3() = kreuz(r.z().vec3(), r.x().vec3());
    return r;
}

// ConstructObject2Screen
Matrix4 objektZuBild(const Matrix4& objektZuWelt, const Matrix4& mv, const Matrix4& proj, const Matrix4& vp) {
    Matrix4 m = vp;
    multipliziere(m, proj);
    multipliziere(m, mv);
    multipliziere(m, objektZuWelt);
    return m;
}

// pivot_scale
Matrix4 pivotMassstab(const Matrix4& pivotZuBild) {
    Matrix4 vor = kEinheitsMatrix;
    vor[0] = static_cast<float>(laenge(pivotZuBild.x().vec3()));
    vor[5] = static_cast<float>(laenge(pivotZuBild.y().vec3()));
    vor[10] = static_cast<float>(laenge(pivotZuBild.z().vec3()));
    Matrix4 s = pivotZuBild;
    multipliziere(s, vor);
    s = vollInvers(s);
    multipliziere(s, pivotZuBild);
    return s;
}

// pivot_perspective: Massstab mit (dem Kehrwert von) W
Matrix4 pivotPerspektive(const Matrix4& pivotZuBild) {
    Matrix4 s = kEinheitsMatrix;
    s[0] = s[5] = s[10] = pivotZuBild[15];
    return s;
}

}  // namespace

void pivotWeltRaum(Matrix4& aus, const Matrix4& pivotZuWelt, const Matrix4& mv, const Matrix4& proj, const Matrix4& vp) {
    aus = pivotZuWelt;
    const Matrix4 pivotZuBild = objektZuBild(pivotZuWelt, mv, proj, vp);
    multipliziere(aus, pivotMassstab(pivotZuBild));
    multipliziere(aus, pivotPerspektive(pivotZuBild));
}

void pivotBlickpunktRaum(Matrix4& aus, Vektor3f& achse, const Matrix4& pivotZuWelt, const Matrix4& mv, const Matrix4& proj,
                         const Matrix4& vp) {
    aus = pivotZuWelt;
    const Matrix4 pivotZuBild = objektZuBild(pivotZuWelt, mv, proj, vp);
    multipliziere(aus, pivotMassstab(pivotZuBild));
    const Matrix4 dreh = blickpunktAusrichtung(pivotZuBild);
    achse = dreh.z().vec3();
    multipliziere(aus, dreh);
    multipliziere(aus, pivotPerspektive(pivotZuBild));
}

void pivotBlickebenenRaum(Matrix4& aus, const Matrix4& pivotZuWelt, const Matrix4& mv, const Matrix4& proj, const Matrix4& vp) {
    aus = pivotZuWelt;
    const Matrix4 pivotZuBild = objektZuBild(pivotZuWelt, mv, proj, vp);
    multipliziere(aus, pivotMassstab(pivotZuBild));
    multipliziere(aus, blickebenenAusrichtung(pivotZuBild));
    multipliziere(aus, pivotPerspektive(pivotZuBild));
}

Matrix4 geraetZuManip(const Matrix4& objektZuWelt, const Matrix4& mv, const Matrix4& proj, const Matrix4& vp) {
    const Matrix4 pivotZuBild = objektZuBild(objektZuWelt, mv, proj, vp);
    // ConstructObject2Device
    Matrix4 m = proj;
    multipliziere(m, mv);
    multipliziere(m, objektZuWelt);
    multipliziere(m, pivotMassstab(pivotZuBild));
    multipliziere(m, pivotPerspektive(pivotZuBild));
    return vollInvers(m);
}

void PivotZuWelt::aktualisieren(const Matrix4& pivotZuWelt, const Matrix4& mv, const Matrix4& proj, const Matrix4& vp) {
    pivotWeltRaum(weltRaum, pivotZuWelt, mv, proj, vp);
    pivotBlickpunktRaum(blickpunktRaum, bildAchse, pivotZuWelt, mv, proj, vp);
    pivotBlickebenenRaum(blickebenenRaum, pivotZuWelt, mv, proj, vp);
}

// --- Geometrie -------------------------------------------------------------------

void pfeilLinie(float laenge, Vektor3f linie[2], std::size_t achse) {
    linie[0] = Vektor3f(0, 0, 0);
    linie[1] = Vektor3f(0, 0, 0);
    linie[1][achse] = laenge - kPfeilKopfLaenge;
}

void pfeilKopf(std::size_t segmente, float laenge, FlachPunkt* punkte, std::size_t achse) {
    // TripleRemapXYZ / YZX / ZXY: x() greift auf Komponente "achse" zu,
    // y() auf achse+1, z() auf achse+2.
    const std::size_t ix = achse;
    const std::size_t iy = (achse + 1) % 3;
    const std::size_t iz = (achse + 2) % 3;
    const auto setze = [&](Vektor3f& v, double x, double y, double z) {
        v[ix] = static_cast<float>(x);
        v[iy] = static_cast<float>(y);
        v[iz] = static_cast<float>(z);
    };
    const std::size_t dreiecke = (segmente << 3);
    const double segment = 2 * kPi / static_cast<double>(dreiecke);
    const double kl = kPfeilKopfLaenge;
    const double kr = kPfeilKopfRadius;
    for (std::size_t i = 0; i < dreiecke; ++i) {
        const double di = static_cast<double>(i);
        setze(punkte[i * 6 + 0].p, laenge - kl, kr * std::cos(di * segment), kr * std::sin(di * segment));
        setze(punkte[i * 6 + 0].n, kr / kl, std::cos(di * segment), std::sin(di * segment));
        setze(punkte[i * 6 + 1].p, laenge, 0, 0);
        setze(punkte[i * 6 + 1].n, kr / kl, std::cos((di + 0.5) * segment), std::sin((di + 0.5) * segment));
        setze(punkte[i * 6 + 2].p, laenge - kl, kr * std::cos((di + 1) * segment), kr * std::sin((di + 1) * segment));
        setze(punkte[i * 6 + 2].n, kr / kl, std::cos((di + 1) * segment), std::sin((di + 1) * segment));
        setze(punkte[i * 6 + 3].p, laenge - kl, 0, 0);
        setze(punkte[i * 6 + 3].n, -1, 0, 0);
        setze(punkte[i * 6 + 4].p, laenge - kl, kr * std::cos(di * segment), kr * std::sin(di * segment));
        setze(punkte[i * 6 + 4].n, -1, 0, 0);
        setze(punkte[i * 6 + 5].p, laenge - kl, kr * std::cos((di + 1) * segment), kr * std::sin((di + 1) * segment));
        setze(punkte[i * 6 + 5].n, -1, 0, 0);
    }
}

namespace {

// RemapXYZ / RemapYZX / RemapZXY::set
void umordnen(Vektor3f& v, float x, float y, float z, Umordnung u) {
    switch (u) {
    case Umordnung::XYZ: v = Vektor3f(x, y, z); break;
    case Umordnung::YZX: v = Vektor3f(z, x, y); break;
    case Umordnung::ZXY: v = Vektor3f(y, z, x); break;
    }
}

}  // namespace

void kreisPunkte(std::size_t segmente, float radius, Vektor3f* p, Umordnung u) {
    const double schritt = kPi / static_cast<double>(segmente << 2);
    std::size_t anzahl = 0;
    float x = radius;
    float y = 0;
    while (anzahl < segmente) {
        Vektor3f* i = p + anzahl;
        Vektor3f* j = p + ((segmente << 1) - (anzahl + 1));
        Vektor3f* k = i + (segmente << 1);
        Vektor3f* l = j + (segmente << 1);
        Vektor3f* m = i + (segmente << 2);
        Vektor3f* n = j + (segmente << 2);
        Vektor3f* o = k + (segmente << 2);
        Vektor3f* q = l + (segmente << 2);
        umordnen(*i, x, -y, 0, u);
        umordnen(*k, -y, -x, 0, u);
        umordnen(*m, -x, y, 0, u);
        umordnen(*o, y, x, 0, u);
        ++anzahl;
        {
            const double theta = schritt * static_cast<double>(anzahl);
            x = static_cast<float>(radius * std::cos(theta));
            y = static_cast<float>(radius * std::sin(theta));
        }
        umordnen(*j, y, -x, 0, u);
        umordnen(*l, -x, -y, 0, u);
        umordnen(*n, -y, x, 0, u);
        umordnen(*q, x, y, 0, u);
    }
}

void halbkreisPunkte(std::size_t segmente, float radius, Vektor3f* p, Umordnung u) {
    const double schritt = kPi / static_cast<double>(segmente << 2);
    std::size_t anzahl = 0;
    float x = radius;
    float y = 0;
    umordnen(p[segmente << 2], -radius, 0, 0, u);
    while (anzahl < segmente) {
        Vektor3f* i = p + anzahl;
        Vektor3f* j = p + ((segmente << 1) - (anzahl + 1));
        Vektor3f* k = i + (segmente << 1);
        Vektor3f* l = j + (segmente << 1);
        umordnen(*i, x, -y, 0, u);
        umordnen(*k, -y, -x, 0, u);
        ++anzahl;
        {
            const double theta = schritt * static_cast<double>(anzahl);
            x = static_cast<float>(radius * std::cos(theta));
            y = static_cast<float>(radius * std::sin(theta));
        }
        umordnen(*j, y, -x, 0, u);
        umordnen(*l, -x, -y, 0, u);
    }
}

void viereckPunkte(float r, Vektor3f q[4]) {
    q[0] = Vektor3f(-r, r, 0);
    q[1] = Vektor3f(r, r, 0);
    q[2] = Vektor3f(r, -r, 0);
    q[3] = Vektor3f(-r, -r, 0);
}

}  // namespace bhed::karte
