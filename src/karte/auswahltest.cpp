// auswahltest.cpp - Karten-Editor: Treffertests fuer die Auswahl.
// Portiert aus NetRadiant-Custom 20260114 (Quellen in auswahltest.h).
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
#include "bhed/karte/auswahltest.h"

#include <algorithm>
#include <cmath>

namespace bhed::karte {

namespace {

template<typename A, typename B>
double skalar(const Vektor3<A>& a, const Vektor3<B>& b) { return punkt(a, b); }   // vector3_dot

// float_equal_epsilon
bool fastGleich(float a, float b, float eps) { return std::fabs(b - a) < eps; }

// --- Schneiden in homogenen Koordinaten (frustum.h:54-362) -------------------

using KlippErgebnis = unsigned char;
constexpr KlippErgebnis kKlippOk = 0x00;
constexpr KlippErgebnis kKlippLtX = 0x01;
constexpr KlippErgebnis kKlippGtX = 0x02;
constexpr KlippErgebnis kKlippLtY = 0x04;
constexpr KlippErgebnis kKlippGtY = 0x08;
constexpr KlippErgebnis kKlippLtZ = 0x10;
constexpr KlippErgebnis kKlippGtZ = 0x20;
constexpr KlippErgebnis kKlippAlles = 0x3F;

// Vector4ClipLT / Vector4ClipGT
bool klippLt(const Vektor4f& v, int i) { return v[static_cast<std::size_t>(i)] < v[3]; }
bool klippGt(const Vektor4f& v, int i) { return v[static_cast<std::size_t>(i)] > -v[3]; }
double skalaLt(const Vektor4f& s, const Vektor4f& o, int i) {
    return (s[static_cast<std::size_t>(i)] - s[3]) / (o[3] - o[static_cast<std::size_t>(i)]);
}
double skalaGt(const Vektor4f& s, const Vektor4f& o, int i) {
    return (s[static_cast<std::size_t>(i)] + s[3]) / (-o[3] - o[static_cast<std::size_t>(i)]);
}

Vektor4f differenz(const Vektor4f& a, const Vektor4f& b) { return Vektor4f(a[0] - b[0], a[1] - b[1], a[2] - b[2], a[3] - b[3]); }

// Vector4ClipPolygon<ClipPlane>::apply
std::size_t polygonSchneiden(const Vektor4f* erste, const Vektor4f* letzte, Vektor4f* aus, int index, bool lt) {
    if (erste == letzte) { return 0; }
    const Vektor4f* naechste = erste;
    const Vektor4f* i = letzte - 1;
    Vektor4f* anfang = aus;
    const auto vergleich = [&](const Vektor4f& v) { return lt ? klippLt(v, index) : klippGt(v, index); };
    bool b0 = vergleich(*i);
    while (naechste != letzte) {
        const bool b1 = vergleich(*naechste);
        if (b0 ^ b1) {
            *aus = differenz(*naechste, *i);
            const double s = lt ? skalaLt(*i, *aus, index) : skalaGt(*i, *aus, index);
            for (std::size_t k = 0; k < 4; ++k) { (*aus)[k] = static_cast<float>((*i)[k] + (s * (*aus)[k])); }
            ++aus;
        }
        if (b1) {
            *aus = *naechste;
            ++aus;
        }
        i = naechste;
        ++naechste;
        b0 = b1;
    }
    return static_cast<std::size_t>(aus - anfang);
}

KlippErgebnis punktKlippen(const Vektor4f& v) {   // homogenous_clip_point
    KlippErgebnis r = kKlippAlles;
    if (klippLt(v, 0)) { r &= static_cast<KlippErgebnis>(~kKlippLtX); }
    if (klippGt(v, 0)) { r &= static_cast<KlippErgebnis>(~kKlippGtX); }
    if (klippLt(v, 1)) { r &= static_cast<KlippErgebnis>(~kKlippLtY); }
    if (klippGt(v, 1)) { r &= static_cast<KlippErgebnis>(~kKlippGtY); }
    if (klippLt(v, 2)) { r &= static_cast<KlippErgebnis>(~kKlippLtZ); }
    if (klippGt(v, 2)) { r &= static_cast<KlippErgebnis>(~kKlippGtZ); }
    return r;
}

KlippErgebnis matrixPunktKlippen(const Matrix4& m, const Vektor3f& p, Vektor4f& geklippt) {   // matrix4_clip_point
    geklippt = transformierterVektor4(m, Vektor4f(p[0], p[1], p[2], 1));
    return punktKlippen(geklippt);
}

std::size_t dreieckKlippen(Vektor4f geklippt[9]) {   // homogenous_clip_triangle
    Vektor4f puffer[9];
    std::size_t n = 3;
    n = polygonSchneiden(geklippt, geklippt + n, puffer, 0, true);
    n = polygonSchneiden(puffer, puffer + n, geklippt, 0, false);
    n = polygonSchneiden(geklippt, geklippt + n, puffer, 1, true);
    n = polygonSchneiden(puffer, puffer + n, geklippt, 1, false);
    n = polygonSchneiden(geklippt, geklippt + n, puffer, 2, true);
    return polygonSchneiden(puffer, puffer + n, geklippt, 2, false);
}

std::size_t matrixDreieckKlippen(const Matrix4& m, const Vektor3f& p0, const Vektor3f& p1, const Vektor3f& p2, Vektor4f geklippt[9]) {
    geklippt[0] = transformierterVektor4(m, Vektor4f(p0[0], p0[1], p0[2], 1));
    geklippt[1] = transformierterVektor4(m, Vektor4f(p1[0], p1[1], p1[2], 1));
    geklippt[2] = transformierterVektor4(m, Vektor4f(p2[0], p2[1], p2[2], 1));
    return dreieckKlippen(geklippt);
}

std::size_t linieKlippen(Vektor4f geklippt[2]) {   // homogenous_clip_line
    const Vektor4f& p0 = geklippt[0];
    const Vektor4f& p1 = geklippt[1];
    {
        const KlippErgebnis m0 = punktKlippen(geklippt[0]);
        const KlippErgebnis m1 = punktKlippen(geklippt[1]);
        if ((m0 | m1) == kKlippOk) { return 2; }   // both points passed all planes
        if ((m0 & m1) != 0) { return 0; }          // both points failed any one plane
    }
    for (int achse = 0; achse < 3; ++achse) {
        for (const bool lt : {true, false}) {
            const bool index = lt ? klippLt(p0, achse) : klippGt(p0, achse);
            const bool index1 = lt ? klippLt(p1, achse) : klippGt(p1, achse);
            if (index ^ index1) {
                Vektor4f clip = differenz(p1, p0);
                const auto a = static_cast<std::size_t>(achse);
                const double scale = lt ? (p0[a] - p0[3]) / (clip[3] - clip[a]) : (p0[a] + p0[3]) / (-clip[3] - clip[a]);
                for (std::size_t k = 0; k < 4; ++k) { clip[k] = static_cast<float>(p0[k] + (scale * clip[k])); }
                geklippt[index ? 1 : 0] = clip;
            } else if (!index) {
                return 0;
            }
        }
    }
    return 2;
}

std::size_t matrixLinieKlippen(const Matrix4& m, const Vektor3f& p0, const Vektor3f& p1, Vektor4f geklippt[2]) {
    geklippt[0] = transformierterVektor4(m, Vektor4f(p0[0], p0[1], p0[2], 1));
    geklippt[1] = transformierterVektor4(m, Vektor4f(p1[0], p1[1], p1[2], 1));
    return linieKlippen(geklippt);
}

// --- BestPoint (selection_volume.h:40-148) ------------------------------------

// point_test_polygon_2d: crossing number test [Franklin, 2000]
bool punktImPolygon2d(const Vektor3f& p, const Vektor3f* anfang, const Vektor3f* ende) {
    std::size_t kreuzungen = 0;
    for (const Vektor3f *vorher = ende - 1, *jetzt = anfang; jetzt != ende; vorher = jetzt, ++jetzt) {
        if ((((*vorher)[1] <= p[1]) && ((*jetzt)[1] > p[1])) || (((*vorher)[1] > p[1]) && ((*jetzt)[1] <= p[1]))) {
            const float vt = (p[1] - (*vorher)[1]) / ((*jetzt)[1] - (*vorher)[1]);
            if (p[0] < (*vorher)[0] + vt * ((*jetzt)[0] - (*vorher)[0])) { ++kreuzungen; }
        }
    }
    return (kreuzungen & 0x1U) != 0;
}

double dreieckFlaecheXY(const Vektor3f& p0, const Vektor3f& p1, const Vektor3f& p2) {   // triangle_signed_area_XY
    return ((p1[0] - p0[0]) * (p2[1] - p0[1])) - ((p2[0] - p0[0]) * (p1[1] - p0[1]));
}

// line_closest_point
Vektor3f naechsterPunktAufLinie(const Vektor3f& start, const Vektor3f& ende, const Vektor3f& ziel) {
    const Vektor3f v = ende - start;
    const Vektor3f w = ziel - start;
    const double c1 = skalar(w, v);
    if (c1 <= 0) { return start; }
    const double c2 = skalar(v, v);
    if (c2 <= c1) { return ende; }
    return start + v * (c1 / c2);
}

// ray_distance_to_plane( Ray( (x,y,0), (0,0,1) ), plane )
double strahlZEbene(float x, float y, const Ebene3d& e) {
    const double abstand = (x * e.a) + (y * e.b) + (0.0 * e.c) - e.d;   // plane3_distance_to_point
    return -abstand / e.c;
}

void besterPunkt(std::size_t anzahl, Vektor4f geklippt[9], Treffer& bester, Wegschneiden wegschneiden, const Ebene3d* ebene = nullptr) {
    Vektor3f n[9];
    for (std::size_t i = 0; i < anzahl; ++i) {
        n[i][0] = geklippt[i][0] / geklippt[i][3];
        n[i][1] = geklippt[i][1] / geklippt[i][3];
        n[i][2] = geklippt[i][2] / geklippt[i][3];
    }
    if (wegschneiden != Wegschneiden::Keins && anzahl > 2) {
        const double flaeche = dreieckFlaecheXY(n[0], n[1], n[2]);
        if ((wegschneiden == Wegschneiden::CW && flaeche > 0) || (wegschneiden == Wegschneiden::CCW && flaeche < 0)) { return; }
    }
    if (anzahl == 2) {
        const Vektor3f p = naechsterPunktAufLinie(n[0], n[1], Vektor3f(0, 0, 0));
        naeherZuweisen(bester, Treffer(p.z(), static_cast<float>(laengeQuadrat(Vektor3f(p.x(), p.y(), 0)))));
    } else if (anzahl > 2 && !punktImPolygon2d(Vektor3f(0, 0, 0), n, n + anzahl)) {
        Ebene3d eigen;
        if (ebene == nullptr) {
            eigen = ebeneDreiPunkte(n[0], n[1], n[2]);
            ebene = &eigen;
        }
        const Vektor3f* ende = n + anzahl;
        for (const Vektor3f *vorher = ende - 1, *jetzt = n; jetzt != ende; vorher = jetzt, ++jetzt) {
            Vektor3f p = naechsterPunktAufLinie(*vorher, *jetzt, Vektor3f(0, 0, 0));
            const float tiefe = p.z();
            p.z() = 0;
            const float abstand = static_cast<float>(laengeQuadrat(p));
            if (ebene->c == 0) {
                naeherZuweisen(bester, Treffer(tiefe, abstand));
            } else {
                naeherZuweisen(bester, Treffer(tiefe, abstand, static_cast<float>(strahlZEbene(0, 0, *ebene))));
            }
        }
    } else if (anzahl > 2) {
        Ebene3d eigen;
        if (ebene == nullptr) {
            eigen = ebeneDreiPunkte(n[0], n[1], n[2]);
            ebene = &eigen;
        }
        naeherZuweisen(bester, Treffer(static_cast<float>(strahlZEbene(0, 0, *ebene)), 0, static_cast<float>(strahlZEbene(10, 8, *ebene))));
    }
}

Treffer punktTreffer(const Vektor4f& g) {   // select_point_from_clipped
    return Treffer(g[2] / g[3], static_cast<float>(laengeQuadrat(Vektor3f(g[0] / g[3], g[1] / g[3], 0))));
}

}  // namespace

// --- Treffer --------------------------------------------------------------------

bool Treffer::operator<(const Treffer& o) const {
    if (m_indirekt != o.m_indirekt) { return o.m_indirekt; }
    if (m_indirekt && o.m_indirekt) {
        if (std::fabs(m_abstand - o.m_abstand) > 1e-3F) { return m_abstand < o.m_abstand; }
        if (std::fabs(m_tiefe - o.m_tiefe) > 1e-6F) { return m_tiefe < o.m_tiefe; }
        return m_tiefe2 > o.m_tiefe2;
    }
    if (m_tiefe != o.m_tiefe) { return m_tiefe < o.m_tiefe; }
    return false;
}

bool Treffer::gleichEps(const Treffer& o, float abstandEps, float tiefeEps) const {
    if (m_indirekt != o.m_indirekt) { return false; }
    if (m_indirekt && o.m_indirekt) {
        return fastGleich(m_abstand, o.m_abstand, abstandEps) && fastGleich(m_tiefe, o.m_tiefe, tiefeEps) &&
               fastGleich(m_tiefe2, o.m_tiefe2, 3e-7F);
    }
    return fastGleich(m_abstand, o.m_abstand, abstandEps) && fastGleich(m_tiefe, o.m_tiefe, tiefeEps) &&
           fastGleich(m_tiefe2, o.m_tiefe2, tiefeEps);
}

// --- Kasten, Blick ------------------------------------------------------------

AuswahlRechteck auswahlKastenPunkt(const GeraeteVektor& p, const GeraeteVektor& eps) {
    AuswahlRechteck r;
    r.min[0] = p[0] - eps[0];
    r.min[1] = p[1] - eps[1];
    r.max[0] = p[0] + eps[0];
    r.max[1] = p[1] + eps[1];
    return r;
}

AuswahlRechteck auswahlKastenFlaeche(const GeraeteVektor& p, const GeraeteVektor& d) {
    AuswahlRechteck r;
    r.min[0] = p[0] + std::min(d[0], 0.0F);
    r.min[1] = p[1] + std::min(d[1], 0.0F);
    r.max[0] = p[0] + std::max(d[0], 0.0F);
    r.max[1] = p[1] + std::max(d[1], 0.0F);
    r.art = d[0] * d[1] < 0 ? AuswahlRechteck::Art::Umschalten : d[0] < 0 ? AuswahlRechteck::Art::Abwaehlen : AuswahlRechteck::Art::Auswaehlen;
    return r;
}

void Blick::bauen(const Matrix4& projektion, const Matrix4& modelview, std::size_t breite, std::size_t hoehe) {
    m_modelview = modelview;
    m_projektion = projektion;
    m_viewport = kEinheitsMatrix;
    m_viewport[0] = static_cast<float>(breite / 2);
    m_viewport[5] = static_cast<float>(hoehe / 2);
    if (std::fabs(m_projektion[11]) > 0.0000001F) {
        m_viewport[10] = m_projektion[0] * m_viewport[0];
    } else {
        m_viewport[10] = 1 / m_projektion[10];
    }
    neu();
}

void Blick::scheren(float minX, float maxX, float minY, float maxY) {
    m_schere = kEinheitsMatrix;
    m_schere[0] = static_cast<float>((maxX - minX) * 0.5);
    m_schere[5] = static_cast<float>((maxY - minY) * 0.5);
    m_schere[12] = static_cast<float>((minX + maxX) * 0.5);
    m_schere[13] = static_cast<float>((minY + maxY) * 0.5);
    m_schere = vollInvers(m_schere);
    neu();
}

void Blick::ohneSchere() {
    m_schere = kEinheitsMatrix;
    neu();
}

void Blick::neu() {
    m_viewproj = multipliziert(multipliziert(m_schere, m_projektion), m_modelview);
    m_frustum = frustumAusViewProj(m_viewproj);
    // viewer_from_viewproj (frustum.h:640-650)
    Vektor4f v = transformierterVektor4(vollInvers(m_viewproj), Vektor4f(0, 0, -1, 0));
    if (v[3] != 0) {
        v[0] /= v[3];
        v[1] /= v[3];
        v[2] /= v[3];
        v[3] /= v[3];
    }
    m_betrachter = v;
    m_blickrichtung = normiert(m_fuellen ? Vektor3f(-m_modelview[2], -m_modelview[6], -m_modelview[10])
                                         : Vektor3f(m_modelview[2], m_modelview[6], m_modelview[10]));
}

void auswahlTestBauen(Blick& blick, const AuswahlRechteck& k) { blick.scheren(k.min[0], k.max[0], k.min[1], k.max[1]); }

// --- AuswahlVolumen -------------------------------------------------------------

void AuswahlVolumen::netzBeginnen(const Matrix4& lokalZuWelt, bool zweiseitig) {
    m_lokalZuSicht = multipliziert(m_blick.sicht(), lokalZuWelt);
    // Cull back-facing polygons based on winding being clockwise or counter-clockwise.
    // Don't cull if the view is wireframe and the polygons are two-sided.
    m_wegschneiden = (zweiseitig && !m_blick.fuellen()) ? Wegschneiden::Keins
                     : haendigkeit(lokalZuWelt) == Haendigkeit::Rechts ? Wegschneiden::CW
                                                                       : Wegschneiden::CCW;
    m_bildZuWelt = vollInvers(m_lokalZuSicht);
}

void AuswahlVolumen::testPunkt(const Vektor3f& punkt, Treffer& bester) const {
    Vektor4f g;
    if (matrixPunktKlippen(m_lokalZuSicht, punkt, g) == kKlippOk) { bester = punktTreffer(g); }
}

void AuswahlVolumen::testPolygon(const Vektor3d* punkte, std::size_t anzahl, Treffer& bester, const EbenenPunkte& ebene) const {
    const auto projiziertD = [&](const Vektor3d& p) {
        // matrix4_transformed_vector4( m_local2view, BasicVector4<double>( p, 1 ) ) - in double
        const Matrix4& m = m_lokalZuSicht;
        const double x = (m[0] * p[0]) + (m[4] * p[1]) + (m[8] * p[2]) + m[12];
        const double y = (m[1] * p[0]) + (m[5] * p[1]) + (m[9] * p[2]) + m[13];
        const double z = (m[2] * p[0]) + (m[6] * p[1]) + (m[10] * p[2]) + m[14];
        const double w = (m[3] * p[0]) + (m[7] * p[1]) + (m[11] * p[2]) + m[15];
        const double r = 1.0 / w;   // vector4_projected
        return Vektor3d(x * r, y * r, z * r);
    };
    const Vektor3d a = projiziertD(ebene[0]);
    const Vektor3d b = projiziertD(ebene[1]);
    const Vektor3d c = projiziertD(ebene[2]);
    // plane3_for_points (double)
    Ebene3d e;
    e.normal() = normiert(kreuz(c - a, b - a));
    e.dist() = skalar(a, e.normal());
    const auto f = [](const Vektor3d& v) { return Vektor3f(static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2])); };
    Vektor4f g[9];
    for (std::size_t i = 0; i + 2 < anzahl; ++i) {
        besterPunkt(matrixDreieckKlippen(m_lokalZuSicht, f(punkte[0]), f(punkte[i + 1]), f(punkte[i + 2]), g), g, bester, m_wegschneiden, &e);
    }
}

void AuswahlVolumen::testLinienSchleife(const Vektor3f* p, std::size_t anzahl, Treffer& bester) const {
    if (anzahl == 0) { return; }
    Vektor4f g[9];
    for (std::size_t i = 0, vorher = anzahl - 1; i < anzahl; vorher = i, ++i) {
        besterPunkt(matrixLinieKlippen(m_lokalZuSicht, p[vorher], p[i], g), g, bester, m_wegschneiden);
    }
}

void AuswahlVolumen::testLinienZug(const Vektor3f* p, std::size_t anzahl, Treffer& bester) const {
    if (anzahl == 0) { return; }
    Vektor4f g[9];
    for (std::size_t i = 0; i + 1 < anzahl; ++i) {
        besterPunkt(matrixLinieKlippen(m_lokalZuSicht, p[i], p[i + 1], g), g, bester, m_wegschneiden);
    }
}

void AuswahlVolumen::testLinien(const Vektor3f* p, std::size_t anzahl, Treffer& bester) const {
    if (anzahl == 0) { return; }
    Vektor4f g[9];
    for (std::size_t i = 0; i + 1 < anzahl; i += 2) {
        besterPunkt(matrixLinieKlippen(m_lokalZuSicht, p[i], p[i + 1], g), g, bester, m_wegschneiden);
    }
}

void AuswahlVolumen::testDreiecke(const Vektor3f* p, const unsigned int* idx, std::size_t anzahl, Treffer& bester) const {
    Vektor4f g[9];
    for (std::size_t i = 0; i + 2 < anzahl; i += 3) {
        besterPunkt(matrixDreieckKlippen(m_lokalZuSicht, p[idx[i]], p[idx[i + 1]], p[idx[i + 2]], g), g, bester, m_wegschneiden);
    }
}

void AuswahlVolumen::testVierecke(const Vektor3f* p, const unsigned int* idx, std::size_t anzahl, Treffer& bester) const {
    Vektor4f g[9];
    for (std::size_t i = 0; i + 3 < anzahl; i += 4) {
        besterPunkt(matrixDreieckKlippen(m_lokalZuSicht, p[idx[i]], p[idx[i + 1]], p[idx[i + 3]], g), g, bester, m_wegschneiden);
        besterPunkt(matrixDreieckKlippen(m_lokalZuSicht, p[idx[i + 1]], p[idx[i + 2]], p[idx[i + 3]], g), g, bester, m_wegschneiden);
    }
}

void AuswahlVolumen::testViereckStreifen(const unsigned char* punkte, std::size_t schritt, const unsigned int* idx, std::size_t anzahl,
                                         Treffer& bester) const {
    const auto p = [&](unsigned int i) -> const Vektor3f& { return *reinterpret_cast<const Vektor3f*>(punkte + (i * schritt)); };
    Vektor4f g[9];
    for (std::size_t i = 0; i + 2 < anzahl; i += 2) {
        besterPunkt(matrixDreieckKlippen(m_lokalZuSicht, p(idx[i]), p(idx[i + 1]), p(idx[i + 2]), g), g, bester, m_wegschneiden);
        besterPunkt(matrixDreieckKlippen(m_lokalZuSicht, p(idx[i + 2]), p(idx[i + 1]), p(idx[i + 3]), g), g, bester, m_wegschneiden);
    }
}

// --- Rechenhilfen (selection_.h:163-280) ----------------------------------------

Vektor3f punktFuerGeraetePunkt(const Matrix4& m, const GeraeteVektor& xy, float z) {
    return projiziert(transformierterVektor4(m, Vektor4f(xy.x(), xy.y(), z, 1)));
}

Strahl3 strahlFuerGeraetePunkt(const Matrix4& m, const GeraeteVektor& xy) {
    // ray_for_points( x, y, zNear; x, y, 0 )
    const Vektor3f a = punktFuerGeraetePunkt(m, xy, -1);
    const Vektor3f b = punktFuerGeraetePunkt(m, xy, 0);
    return Strahl3{a, normiert(b - a)};
}

Vektor3f punktAufKugel(const Matrix4& m, const GeraeteVektor& xy, float radius) {
    // sphere_intersect_ray( g_origin, radius, ray )
    const Strahl3 s = strahlFuerGeraetePunkt(m, xy);
    const Vektor3f schnitt = Vektor3f(0, 0, 0) - s.ursprung;
    const double a = skalar(schnitt, s.richtung);
    const double d = (radius * radius) - (skalar(schnitt, schnitt) - (a * a));
    if (d > 0) { return s.ursprung + s.richtung * (a - std::sqrt(d)); }
    return s.ursprung + s.richtung * a;
}

Vektor3f punktAufAchse(const Vektor3f& achse, const Matrix4& m, const GeraeteVektor& xy) {
    // ray_intersect_ray( ray, Ray( 0, axis ) )
    const Strahl3 ray = strahlFuerGeraetePunkt(m, xy);
    const Strahl3 other{Vektor3f(0, 0, 0), achse};
    const Vektor3f schnitt = ray.ursprung - other.ursprung;
    const double dot = skalar(ray.richtung, other.richtung);
    const double d = skalar(ray.richtung, schnitt);
    const double e = skalar(other.richtung, schnitt);
    const double gross = 1 - (dot * dot);
    if (gross < 0.000001) { return other.ursprung + other.richtung * e; }   // the lines are almost parallel
    return other.ursprung + other.richtung * ((e - (dot * d)) / gross);
}

Vektor3f punktAufEbeneGeraet(const Matrix4& geraetZuObjekt, const GeraeteVektor& xy) {
    const Matrix4 objektZuGeraet = vollInvers(geraetZuObjekt);
    return projiziert(transformierterVektor4(geraetZuObjekt, Vektor4f(xy.x(), xy.y(), objektZuGeraet[14] / objektZuGeraet[15], 1)));
}

Vektor3f strahlSchnittEbene3(const Strahl3& s, const Ebene3d& e) {
    const double abstand = skalar(s.ursprung, e.normal()) - e.dist();
    return s.ursprung + s.richtung * (-abstand / skalar(s.richtung, e.normal()));
}

Vektor3f punktAufEbeneSicht(const Ebene3d& ebene, const Matrix4& objektZuGeraet, const GeraeteVektor& xy) {
    return strahlSchnittEbene3(strahlFuerGeraetePunkt(vollInvers(objektZuGeraet), xy), ebene);
}

Vektor3f translationLokalZuObjekt(const Vektor3f& lokal, const Matrix4& lokalZuObjekt) {
    Matrix4 m = lokalZuObjekt;
    verschiebeUm(m, lokal);
    return multipliziert(m, vollInvers(lokalZuObjekt)).t().vec3();
}

float winkelZwischen(const Vektor3f& a, const Vektor3f& b) {
    return static_cast<float>(2.0 * std::atan2(laenge(a - b), laenge(a + b)));
}

void aufAchseBeschraenken(Vektor3f& v, const Vektor3f& achse) {
    v = normiert(v + achse * static_cast<float>(-skalar(v, achse)));
}

float winkelFuerAchse(const Vektor3f& a, const Vektor3f& b, const Vektor3f& achse) {
    if (skalar(achse, kreuz(a, b)) > 0) { return winkelZwischen(a, b); }
    return -winkelZwischen(a, b);
}

float abstandFuerAchse(const Vektor3f& a, const Vektor3f& b, const Vektor3f& achse) {
    return static_cast<float>(skalar(b, achse) - skalar(a, achse));
}

// --- BestPoint-Tests fuer Manipulatoren (selection_volume.h:155-271) --------

void punktBesterPunkt(const Matrix4& m, const Vektor3f& punkt, Treffer& bester) {
    Vektor4f geklippt;
    if (matrixPunktKlippen(m, punkt, geklippt) == kKlippOk) { naeherZuweisen(bester, punktTreffer(geklippt)); }
}

void linienZugBesterPunkt(const Matrix4& m, const Vektor3f* p, std::size_t anzahl, Treffer& bester) {
    Vektor4f geklippt[9];
    for (std::size_t i = 0; (i + 1) < anzahl; ++i) {
        const std::size_t n = matrixLinieKlippen(m, p[i], p[i + 1], geklippt);
        besterPunkt(n, geklippt, bester, Wegschneiden::Keins);
    }
}

void linienSchleifeBesterPunkt(const Matrix4& m, const Vektor3f* p, std::size_t anzahl, Treffer& bester) {
    Vektor4f geklippt[9];
    for (std::size_t i = 0; i < anzahl; ++i) {
        const std::size_t n = matrixLinieKlippen(m, p[i], p[(i + 1) % anzahl], geklippt);
        besterPunkt(n, geklippt, bester, Wegschneiden::Keins);
    }
}

void linieBesterPunkt(const Matrix4& m, const Vektor3f p[2], Treffer& bester) {
    Vektor4f geklippt[9];
    const std::size_t n = matrixLinieKlippen(m, p[0], p[1], geklippt);
    besterPunkt(n, geklippt, bester, Wegschneiden::Keins);
}

void kreisBesterPunkt(const Matrix4& m, Wegschneiden w, const Vektor3f* p, std::size_t anzahl, Treffer& bester) {
    Vektor4f geklippt[9];
    for (std::size_t i = 0; i < anzahl; ++i) {
        const std::size_t n = matrixDreieckKlippen(m, Vektor3f(0, 0, 0), p[i], p[(i + 1) % anzahl], geklippt);
        besterPunkt(n, geklippt, bester, w);
    }
}

void viereckBesterPunkt(const Matrix4& m, Wegschneiden w, const Vektor3f p[4], Treffer& bester) {
    Vektor4f geklippt[9];
    {
        const std::size_t n = matrixDreieckKlippen(m, p[0], p[1], p[3], geklippt);
        besterPunkt(n, geklippt, bester, w);
    }
    {
        const std::size_t n = matrixDreieckKlippen(m, p[1], p[2], p[3], geklippt);
        besterPunkt(n, geklippt, bester, w);
    }
}

std::array<Vektor3f, 8> kastenEcken(const Kasten& k) {
    const Vektor3f mn = k.origin - k.extents;
    const Vektor3f mx = k.origin + k.extents;
    return {Vektor3f(mn[0], mx[1], mx[2]), Vektor3f(mx[0], mx[1], mx[2]), Vektor3f(mx[0], mn[1], mx[2]), Vektor3f(mn[0], mn[1], mx[2]),
            Vektor3f(mn[0], mx[1], mn[2]), Vektor3f(mx[0], mx[1], mn[2]), Vektor3f(mx[0], mn[1], mn[2]), Vektor3f(mn[0], mn[1], mn[2])};
}

void kastenBesterPunkt(const Matrix4& m, Wegschneiden w, const Kasten& kasten, Treffer& bester) {
    static constexpr unsigned int kIndizes[24] = {2, 1, 5, 6, 1, 0, 4, 5, 0, 1, 2, 3, 3, 7, 4, 0, 3, 2, 6, 7, 7, 6, 5, 4};
    const std::array<Vektor3f, 8> p = kastenEcken(kasten);
    Vektor4f geklippt[9];
    for (std::size_t i = 0; i < 24; i += 4) {
        besterPunkt(matrixDreieckKlippen(m, p[kIndizes[i]], p[kIndizes[i + 1]], p[kIndizes[i + 3]], geklippt), geklippt, bester, w);
        besterPunkt(matrixDreieckKlippen(m, p[kIndizes[i + 1]], p[kIndizes[i + 2]], p[kIndizes[i + 3]], geklippt), geklippt, bester, w);
    }
}

void dreieckeBesterPunkt(const Matrix4& m, Wegschneiden w, const Vektor3f* p, std::size_t anzahl, Treffer& bester) {
    for (std::size_t i = 0; i + 2 < anzahl; i += 3) {
        Vektor4f geklippt[9];
        besterPunkt(matrixDreieckKlippen(m, p[i], p[i + 1], p[i + 2], geklippt), geklippt, bester, w);
    }
}

std::size_t linieAnNahebeneKlippen(const Matrix4& m, Vektor3f& start, Vektor3f& ende) {
    Vektor4f p[2] = {transformierterVektor4(m, Vektor4f(start, 1)), transformierterVektor4(m, Vektor4f(ende, 1))};
    {
        const bool ok0 = klippGt(p[0], 2);   // CLIP_Z_GT_W
        const bool ok1 = klippGt(p[1], 2);
        if (ok0 && ok1) {
            start = projiziert(p[0]);
            ende = projiziert(p[1]);
            return 2;
        }
        if (!ok0 && !ok1) { return 0; }
    }
    {
        const bool index = klippGt(p[0], 2);
        if (index ^ klippGt(p[1], 2)) {
            Vektor4f clip = differenz(p[1], p[0]);
            const double scale = (p[0][2] + p[0][3]) / (-clip[3] - clip[2]);
            for (std::size_t k = 0; k < 4; ++k) { clip[k] = static_cast<float>(p[0][k] + scale * clip[k]); }
            p[index ? 1 : 0] = clip;
        } else if (!index) {
            return 0;
        }
    }
    start = projiziert(p[0]);
    ende = projiziert(p[1]);
    return 2;
}

GeraeteVektor fensterZuGeraet(float x, float y, int breite, int hoehe) {
    // screen_normalised: ( ( 2.0f * pos ) / size ) - 1.0f
    return GeraeteVektor(((2.0F * x) / static_cast<float>(breite)) - 1.0F,
                         ((2.0F * (static_cast<float>(hoehe) - 1 - y)) / static_cast<float>(hoehe)) - 1.0F);
}

GeraeteVektor geraetBegrenzt(const GeraeteVektor& g) {
    return GeraeteVektor(std::clamp(g[0], -1.0F, 1.0F), std::clamp(g[1], -1.0F, 1.0F));
}

Ebene3d ebeneDreiPunkte(const Vektor3f& p0, const Vektor3f& p1, const Vektor3f& p2) {
    Ebene3d e;
    const Vektor3f n = normiert(kreuz(p2 - p0, p1 - p0));
    e.normal() = Vektor3d(n[0], n[1], n[2]);
    e.dist() = skalar(p0, e.normal());
    return e;
}

}  // namespace bhed::karte
