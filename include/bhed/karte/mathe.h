// mathe.h - Karten-Editor: Vektoren, Ebenen, Matrizen, Strahlen, Kaesten.
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): libs/generic/vector.h,
// libs/math/vector.h, libs/math/plane.h, libs/math/line.h, libs/math/matrix.h,
// libs/math/aabb.h, libs/math/pi.h, libs/math/quaternion.h (Auszug).
//
//   Copyright (C) 2001-2006, William Joseph. All Rights Reserved.
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
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
// Warum ein eigener Nachbau statt der float[3] aus bsp.h: NRC rechnet gemischt
// in float und double, und das Ergebnis haengt genau davon ab. Ein Beispiel:
// vector3_dot zweier float-Vektoren multipliziert und addiert in float und gibt
// erst die Summe als double zurueck; vector3_added(float, double) rechnet die
// Summe in double und schneidet auf den Typ des LINKEN Vektors ab. Die
// Schablonen hier tun dasselbe Ausdruck fuer Ausdruck. Wo NRC eine
// stillschweigende Umwandlung double->float macht (Konstruktoraufrufe), steht
// hier ein static_cast - gleiche Bedeutung, aber ohne Warnung (/W4).
//
// Namen: deutsch, der NRC-Name steht jeweils im Kommentar.
#ifndef BHED_KARTE_MATHE_H
#define BHED_KARTE_MATHE_H

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <utility>

namespace bhed::karte {

// --- Vektoren (libs/generic/vector.h) -------------------------------------

template<typename E>
class Vektor2 {   // BasicVector2
    E m_e[2];
public:
    Vektor2() : m_e{E(0), E(0)} {}
    template<typename O>
    Vektor2(const Vektor2<O>& o) : m_e{static_cast<E>(o.x()), static_cast<E>(o.y())} {}   // NOLINT
    Vektor2(const E& x_, const E& y_) : m_e{x_, y_} {}
    explicit Vektor2(const E& v) : m_e{v, v} {}
    E& x() { return m_e[0]; }
    const E& x() const { return m_e[0]; }
    E& y() { return m_e[1]; }
    const E& y() const { return m_e[1]; }
    const E& operator[](std::size_t i) const { return m_e[i]; }
    E& operator[](std::size_t i) { return m_e[i]; }
    E* data() { return m_e; }
    const E* data() const { return m_e; }
};

template<typename E>
class Vektor3 {   // BasicVector3
    E m_e[3];
public:
    // NRC laesst die Elemente uninitialisiert; hier 0, damit der Uebersetzer
    // nicht warnt. Jede gelesene Stelle wird in NRC vorher beschrieben.
    Vektor3() : m_e{E(0), E(0), E(0)} {}
    template<typename O>
    Vektor3(const Vektor3<O>& o)   // NOLINT
        : m_e{static_cast<E>(o.x()), static_cast<E>(o.y()), static_cast<E>(o.z())} {}
    template<typename O>
    explicit Vektor3(const Vektor2<O>& v2, const E& z_)
        : m_e{static_cast<E>(v2.x()), static_cast<E>(v2.y()), z_} {}
    Vektor3(const E& x_, const E& y_, const E& z_) : m_e{x_, y_, z_} {}
    explicit Vektor3(const E& v) : m_e{v, v, v} {}
    E& x() { return m_e[0]; }
    const E& x() const { return m_e[0]; }
    E& y() { return m_e[1]; }
    const E& y() const { return m_e[1]; }
    E& z() { return m_e[2]; }
    const E& z() const { return m_e[2]; }
    const E& operator[](std::size_t i) const { return m_e[i]; }
    E& operator[](std::size_t i) { return m_e[i]; }
    E* data() { return m_e; }
    const E* data() const { return m_e; }
    Vektor2<E>& vec2() { return reinterpret_cast<Vektor2<E>&>(m_e[0]); }
    const Vektor2<E>& vec2() const { return reinterpret_cast<const Vektor2<E>&>(m_e[0]); }
    void set(const E v) { m_e[0] = m_e[1] = m_e[2] = v; }
};

template<typename E>
class Vektor4 {   // BasicVector4
    E m_e[4];
public:
    Vektor4() : m_e{E(0), E(0), E(0), E(0)} {}
    Vektor4(E x_, E y_, E z_, E w_) : m_e{x_, y_, z_, w_} {}
    Vektor4(const Vektor3<E>& v3, E w_) : m_e{v3.x(), v3.y(), v3.z(), w_} {}
    explicit Vektor4(const E& v) : m_e{v, v, v, v} {}
    E& x() { return m_e[0]; }
    const E& x() const { return m_e[0]; }
    E& y() { return m_e[1]; }
    const E& y() const { return m_e[1]; }
    E& z() { return m_e[2]; }
    const E& z() const { return m_e[2]; }
    E& w() { return m_e[3]; }
    const E& w() const { return m_e[3]; }
    // NRC: der const-Zugriff liefert eine Kopie (Element, nicht const Element&).
    E operator[](std::size_t i) const { return m_e[i]; }
    E& operator[](std::size_t i) { return m_e[i]; }
    E* data() { return m_e; }
    const E* data() const { return m_e; }
    Vektor3<E>& vec3() { return reinterpret_cast<Vektor3<E>&>(m_e[0]); }
    const Vektor3<E>& vec3() const { return reinterpret_cast<const Vektor3<E>&>(m_e[0]); }
};

using Vektor2f = Vektor2<float>;    // Vector2
using Vektor3f = Vektor3<float>;    // Vector3
using Vektor3d = Vektor3<double>;   // DoubleVector3
using Vektor4f = Vektor4<float>;    // Vector4

// --- Zahlen (libs/math/vector.h:35-86) -------------------------------------

// float_equal_epsilon
template<typename E, typename O>
inline bool gleichEps(const E& self, const O& other, const E& eps) {
    return std::fabs(other - self) < eps;
}
// float_mid
template<typename E>
inline E zahlMitte(const E& self, const E& other) { return E((self + other) * 0.5); }
// float_to_integer: lrint, also "zur naechsten" und nicht abschneiden.
template<typename E>
inline int zuGanzzahl(const E& f) { return static_cast<int>(std::lrint(f)); }
// float_snapped: llrint hat mehr Bits. Das Produkt hat den Typ des Rasters -
// ist snap ein float (Face::snapto), rechnet NRC hier in float.
template<typename E, typename O>
inline E zahlEingerastet(const E& f, const O& snap) {
    if (snap == 0) { return f; }
    return E(std::llrint(f / snap) * snap);
}
// float_is_integer
template<typename E>
inline bool istGanzzahl(const E& f) { return f == E(zuGanzzahl(f)); }
// float_mod_range / float_mod
template<typename E, typename M>
inline E zahlModBereich(const E& self, const M& modulus) {
    return E((self < 0.0) ? self + modulus : self);
}
template<typename E, typename M>
inline E zahlMod(const E& self, const M& modulus) {
    return zahlModBereich(E(std::fmod(static_cast<double>(self), static_cast<double>(modulus))), modulus);
}
// float_divided
template<typename E>
inline E zahlGeteilt(E f, E other) { return f / other; }

// --- Vektor2-Rechnung (libs/math/vector.h:89-277) ---------------------------

template<typename E, typename O>
inline bool operator==(const Vektor2<E>& a, const Vektor2<O>& b) { return a.x() == b.x() && a.y() == b.y(); }
template<typename E, typename O>
inline bool operator!=(const Vektor2<E>& a, const Vektor2<O>& b) { return !(a == b); }
template<typename E, typename O>
inline Vektor2<E> operator+(const Vektor2<E>& a, const Vektor2<O>& b) {   // vector2_added
    return Vektor2<E>(E(a.x() + b.x()), E(a.y() + b.y()));
}
template<typename E, typename O>
inline void operator+=(Vektor2<E>& a, const Vektor2<O>& b) {   // vector2_add
    a.x() += E(b.x());
    a.y() += E(b.y());
}
template<typename E, typename O>
inline Vektor2<E> operator-(const Vektor2<E>& a, const Vektor2<O>& b) {   // vector2_subtracted
    return Vektor2<E>(E(a.x() - b.x()), E(a.y() - b.y()));
}
template<typename E, typename O>
inline Vektor2<E> mal(const Vektor2<E>& a, O s) {   // vector2_scaled (Zahl)
    return Vektor2<E>(E(a.x() * s), E(a.y() * s));
}
template<typename E, typename O>
inline void teilen(Vektor2<E>& a, O d) {   // vector2_divide (Zahl)
    a.x() /= E(d);
    a.y() /= E(d);
}
template<typename E, typename O>
inline double punkt(const Vektor2<E>& a, const Vektor2<O>& b) {   // vector2_dot
    return a.x() * b.x() + a.y() * b.y();
}
template<typename E>
inline double laengeQuadrat(const Vektor2<E>& a) { return punkt(a, a); }   // vector2_length_squared
template<typename E>
inline double laenge(const Vektor2<E>& a) { return std::sqrt(laengeQuadrat(a)); }   // vector2_length
template<typename E, typename O>
inline double kreuz(const Vektor2<E>& a, const Vektor2<O>& b) {   // vector2_cross
    return a.x() * b.y() - a.y() * b.x();
}
template<typename E>
inline Vektor2<E> normiert(const Vektor2<E>& a) {   // vector2_normalised
    return mal(a, zahlGeteilt(1.0, laenge(a)));
}

// --- Vektor3-Rechnung (libs/math/vector.h:279-575) --------------------------

inline const Vektor3f kNullVektor(0, 0, 0);                    // g_vector3_identity
inline const Vektor3f kAchseX(1, 0, 0);                        // g_vector3_axis_x
inline const Vektor3f kAchseY(0, 1, 0);                        // g_vector3_axis_y
inline const Vektor3f kAchseZ(0, 0, 1);                        // g_vector3_axis_z
inline const Vektor3f kAchsen[3] = {kAchseX, kAchseY, kAchseZ};   // g_vector3_axes

template<typename E, typename O>
inline void tauschen(Vektor3<E>& a, Vektor3<O>& b) {   // vector3_swap
    std::swap(a.x(), b.x());
    std::swap(a.y(), b.y());
    std::swap(a.z(), b.z());
}
template<typename E, typename O>
inline bool operator==(const Vektor3<E>& a, const Vektor3<O>& b) {   // vector3_equal
    return a.x() == b.x() && a.y() == b.y() && a.z() == b.z();
}
template<typename E, typename O>
inline bool operator!=(const Vektor3<E>& a, const Vektor3<O>& b) { return !(a == b); }
template<typename E, typename O, typename Eps>
inline bool gleichEps(const Vektor3<E>& a, const Vektor3<O>& b, Eps eps) {   // vector3_equal_epsilon
    return gleichEps(a.x(), b.x(), eps) && gleichEps(a.y(), b.y(), eps) && gleichEps(a.z(), b.z(), eps);
}
template<typename E, typename O>
inline Vektor3<E> operator+(const Vektor3<E>& a, const Vektor3<O>& b) {   // vector3_added
    return Vektor3<E>(E(a.x() + b.x()), E(a.y() + b.y()), E(a.z() + b.z()));
}
template<typename E, typename O>
inline void operator+=(Vektor3<E>& a, const Vektor3<O>& b) {   // vector3_add
    a.x() += static_cast<E>(b.x());
    a.y() += static_cast<E>(b.y());
    a.z() += static_cast<E>(b.z());
}
template<typename E, typename O>
inline Vektor3<E> operator-(const Vektor3<E>& a, const Vektor3<O>& b) {   // vector3_subtracted
    return Vektor3<E>(E(a.x() - b.x()), E(a.y() - b.y()), E(a.z() - b.z()));
}
template<typename E, typename O>
inline void operator-=(Vektor3<E>& a, const Vektor3<O>& b) {   // vector3_subtract
    a.x() -= static_cast<E>(b.x());
    a.y() -= static_cast<E>(b.y());
    a.z() -= static_cast<E>(b.z());
}
template<typename E, typename O>
inline Vektor3<E> operator*(const Vektor3<E>& a, const Vektor3<O>& b) {   // vector3_scaled (Vektor)
    return Vektor3<E>(E(a.x() * b.x()), E(a.y() * b.y()), E(a.z() * b.z()));
}
template<typename E, typename O>
inline Vektor3<E> operator*(const Vektor3<E>& a, const O& s) {   // vector3_scaled (Zahl)
    return Vektor3<E>(E(a.x() * s), E(a.y() * s), E(a.z() * s));
}
template<typename E, typename O>
inline void operator*=(Vektor3<E>& a, const O& s) {   // vector3_scale (Zahl)
    a.x() *= static_cast<E>(s);
    a.y() *= static_cast<E>(s);
    a.z() *= static_cast<E>(s);
}
template<typename E, typename O>
inline Vektor3<E> operator/(const Vektor3<E>& a, const Vektor3<O>& b) {   // vector3_divided (Vektor)
    return Vektor3<E>(E(a.x() / b.x()), E(a.y() / b.y()), E(a.z() / b.z()));
}
template<typename E, typename O>
inline Vektor3<E> operator/(const Vektor3<E>& a, const O& d) {   // vector3_divided (Zahl)
    return Vektor3<E>(E(a.x() / d), E(a.y() / d), E(a.z() / d));
}
template<typename E, typename O>
inline void operator/=(Vektor3<E>& a, const O& d) {   // vector3_divide (Zahl)
    a.x() /= static_cast<E>(d);
    a.y() /= static_cast<E>(d);
    a.z() /= static_cast<E>(d);
}
template<typename E, typename O>
inline double punkt(const Vektor3<E>& a, const Vektor3<O>& b) {   // vector3_dot
    return a.x() * b.x() + a.y() * b.y() + a.z() * b.z();
}
template<typename E>
inline Vektor3<E> mitte(const Vektor3<E>& a, const Vektor3<E>& b) {   // vector3_mid
    return (a + b) * 0.5;
}
template<typename E, typename O>
inline Vektor3<E> kreuz(const Vektor3<E>& a, const Vektor3<O>& b) {   // vector3_cross
    return Vektor3<E>(E(a.y() * b.z() - a.z() * b.y()),
                      E(a.z() * b.x() - a.x() * b.z()),
                      E(a.x() * b.y() - a.y() * b.x()));
}
template<typename E>
inline Vektor3<E> operator-(const Vektor3<E>& a) {   // vector3_negated
    return Vektor3<E>(-a.x(), -a.y(), -a.z());
}
template<typename E>
inline void negieren(Vektor3<E>& a) { a = -a; }   // vector3_negate
template<typename E>
inline double laengeQuadrat(const Vektor3<E>& a) { return punkt(a, a); }   // vector3_length_squared
template<typename E>
inline double laenge(const Vektor3<E>& a) { return std::sqrt(laengeQuadrat(a)); }   // vector3_length
template<typename E>
inline Vektor3<E> normiert(const Vektor3<E>& a) {   // vector3_normalised
    return a * zahlGeteilt(1.0, laenge(a));
}
template<typename E>
inline void normieren(Vektor3<E>& a) { a = normiert(a); }   // vector3_normalise
template<typename E>
inline Vektor3<E> eingerastet(const Vektor3<E>& a) {   // vector3_snapped
    return Vektor3<E>(E(zuGanzzahl(a.x())), E(zuGanzzahl(a.y())), E(zuGanzzahl(a.z())));
}
template<typename E, typename O>
inline Vektor3<E> eingerastet(const Vektor3<E>& a, const O& snap) {   // vector3_snapped (Raster)
    return Vektor3<E>(E(zahlEingerastet(a.x(), snap)), E(zahlEingerastet(a.y(), snap)), E(zahlEingerastet(a.z(), snap)));
}
template<typename E, typename O>
inline void einrasten(Vektor3<E>& a, const O& snap) { a = eingerastet(a, snap); }   // vector3_snap
// vector3_max_abs_component_index
template<typename E>
inline std::size_t groessteBetragsKomponente(const Vektor3<E>& a) {
    const std::size_t maxi = (std::fabs(a[1]) > std::fabs(a[0])) ? 1 : 0;
    return (std::fabs(a[2]) > std::fabs(a[maxi])) ? 2 : maxi;
}

// --- Kreiszahl (libs/math/pi.h) --------------------------------------------

constexpr double kPi = 3.1415926535897932384626433832795;   // c_pi
constexpr double kHalbPi = kPi / 2;                           // c_half_pi
constexpr double kGradZuBogen = kPi / 180.0;                  // c_DEG2RADMULT
constexpr double kBogenZuGrad = 180.0 / kPi;                  // c_RAD2DEGMULT
inline double bogenZuGrad(double r) { return r * kBogenZuGrad; }   // radians_to_degrees
inline double gradZuBogen(double g) { return g * kGradZuBogen; }   // degrees_to_radians

// --- Ebene (libs/math/plane.h) ---------------------------------------------

template<typename T>
class Ebene3 {   // Plane3___
public:
    T a = 0, b = 0, c = 0, d = 0;
    Ebene3() = default;
    Ebene3(double a_, double b_, double c_, double d_)
        : a(static_cast<T>(a_)), b(static_cast<T>(b_)), c(static_cast<T>(c_)), d(static_cast<T>(d_)) {}
    template<typename E>
    Ebene3(const Vektor3<E>& n, double dist)
        : a(static_cast<T>(n.x())), b(static_cast<T>(n.y())), c(static_cast<T>(n.z())), d(static_cast<T>(dist)) {}
    // Wie NRC: normal() ist ein Blick auf a, b, c als Vektor.
    Vektor3<T>& normal() { return reinterpret_cast<Vektor3<T>&>(a); }
    const Vektor3<T>& normal() const { return reinterpret_cast<const Vektor3<T>&>(a); }
    T& dist() { return d; }
    const T& dist() const { return d; }
};
using Ebene3d = Ebene3<double>;   // Plane3
using EbenenPunkte = std::array<Vektor3d, 3>;   // PlanePoints

template<typename T>
inline Ebene3<T> umgedreht(const Ebene3<T>& e) {   // plane3_flipped
    return Ebene3<T>(-e.normal(), -e.dist());
}
constexpr double kEbeneNormaleEps = 0.0001f;   // c_PLANE_NORMAL_EPSILON (ein float-Literal, wie in NRC)
constexpr double kEbeneAbstandEps = 0.02;      // c_PLANE_DIST_EPSILON
inline bool ebenenGleich(const Ebene3d& a, const Ebene3d& b) {   // plane3_equal
    return gleichEps(a.normal(), b.normal(), kEbeneNormaleEps) && gleichEps(a.dist(), b.dist(), kEbeneAbstandEps);
}
inline bool ebenenGegenueber(const Ebene3d& a, const Ebene3d& b) {   // plane3_opposing
    return ebenenGleich(a, umgedreht(b));
}
inline bool ebeneGueltig(const Ebene3d& e) {   // plane3_valid
    return gleichEps(punkt(e.normal(), e.normal()), 1.0, 0.01);
}
// plane3_for_points - Reihenfolge von aussen gesehen:
//   1
//   |
//   0-----2
template<typename E>
inline Ebene3d ebeneAusPunkten(const Vektor3<E>& p0, const Vektor3<E>& p1, const Vektor3<E>& p2) {
    Ebene3d e;
    e.normal() = normiert(kreuz(p2 - p0, p1 - p0));
    e.dist() = punkt(p0, e.normal());
    return e;
}
inline Ebene3d ebeneAusPunkten(const EbenenPunkte& p) { return ebeneAusPunkten(p[0], p[1], p[2]); }
// plane3_distance_to_point
template<typename P, typename V>
inline double abstandZuPunkt(const Ebene3<P>& e, const Vektor3<V>& p) {
    return punkt(p, e.normal()) - e.dist();
}
// plane3_project_point (entlang einer Richtung)
template<typename T, typename U>
inline Vektor3<T> punktAufEbene(const Ebene3d& e, const Vektor3<T>& p, const Vektor3<U>& richtung) {
    const double f = punkt(e.normal(), richtung);
    const double d = (punkt(e.normal() * e.dist() - p, e.normal())) / f;
    return p + richtung * d;
}
// plane3_project_point (senkrecht)
template<typename P, typename V>
inline Vektor3<V> punktAufEbene(const Ebene3<P>& e, const Vektor3<V>& p) {
    return p - e.normal() * abstandZuPunkt(e, p);
}
// plane3_translated
template<typename P, typename V>
inline Ebene3<P> ebeneVerschoben(const Ebene3<P>& e, const Vektor3<V>& v) {
    return Ebene3<P>(e.normal(), e.dist() + punkt(e.normal(), v));
}

// --- Strahl (libs/math/line.h) ---------------------------------------------

template<typename T>
class Strahl {   // BasicRay
public:
    Vektor3<T> origin, direction;
    Strahl() = default;
    Strahl(const Vektor3<T>& o, const Vektor3<T>& d) : origin(o), direction(d) {}
};
using Strahlf = Strahl<float>;    // Ray
using Strahld = Strahl<double>;   // DoubleRay

template<typename T>
inline Strahl<T> strahlAusPunkten(const Vektor3<T>& o, const Vektor3<T>& p2) {   // ray_for_points
    return Strahl<T>(o, normiert(p2 - o));
}
// ray_squared_distance_to_point
template<typename T, typename T2>
inline double strahlAbstandQuadrat(const Strahl<T>& s, const Vektor3<T2>& p) {
    return laengeQuadrat(p - (s.origin + s.direction * punkt(p - s.origin, s.direction)));
}
// ray_distance_to_plane
inline double strahlAbstandZuEbene(const Strahlf& s, const Ebene3d& e) {
    return -abstandZuPunkt(e, s.origin) / punkt(s.direction, e.normal());
}
// ray_intersect_plane
template<typename T>
inline Vektor3<T> strahlSchnittEbene(const Strahl<T>& s, const Ebene3d& e) {
    return s.origin + s.direction * (-abstandZuPunkt(e, s.origin) / punkt(s.direction, e.normal()));
}
// plane3_intersect_plane3
inline Strahld ebenenSchnitt(const Ebene3d& e, const Ebene3d& o) {
    Strahld l;
    l.direction = kreuz(e.normal(), o.normal());
    switch (groessteBetragsKomponente(l.direction)) {
    case 0:
        l.origin.x() = 0;
        l.origin.y() = (-o.dist() * e.normal().z() - -e.dist() * o.normal().z()) / l.direction.x();
        l.origin.z() = (-e.dist() * o.normal().y() - -o.dist() * e.normal().y()) / l.direction.x();
        break;
    case 1:
        l.origin.x() = (-e.dist() * o.normal().z() - -o.dist() * e.normal().z()) / l.direction.y();
        l.origin.y() = 0;
        l.origin.z() = (-o.dist() * e.normal().x() - -e.dist() * o.normal().x()) / l.direction.y();
        break;
    case 2:
        l.origin.x() = (-o.dist() * e.normal().y() - -e.dist() * o.normal().y()) / l.direction.z();
        l.origin.y() = (-e.dist() * o.normal().x() - -o.dist() * e.normal().x()) / l.direction.z();
        l.origin.z() = 0;
        break;
    default:
        break;
    }
    return l;
}

// --- Matrix (libs/math/matrix.h) - float, spaltenweise ----------------------
//
//   [0] [4]  [8] [12]
//   [1] [5]  [9] [13]
//   [2] [6] [10] [14]
//   [3] [7] [11] [15]
class Matrix4 {
    float m[16];
public:
    Matrix4() : m{} {}
    Matrix4(float xx, float xy, float xz, float xw, float yx, float yy, float yz, float yw,
            float zx, float zy, float zz, float zw, float tx, float ty, float tz, float tw)
        : m{xx, xy, xz, xw, yx, yy, yz, yw, zx, zy, zz, zw, tx, ty, tz, tw} {}
    float& xx() { return m[0]; }
    const float& xx() const { return m[0]; }
    float& xy() { return m[1]; }
    const float& xy() const { return m[1]; }
    float& xz() { return m[2]; }
    const float& xz() const { return m[2]; }
    float& yx() { return m[4]; }
    const float& yx() const { return m[4]; }
    float& yy() { return m[5]; }
    const float& yy() const { return m[5]; }
    float& yz() { return m[6]; }
    const float& yz() const { return m[6]; }
    float& zx() { return m[8]; }
    const float& zx() const { return m[8]; }
    float& zy() { return m[9]; }
    const float& zy() const { return m[9]; }
    float& zz() { return m[10]; }
    const float& zz() const { return m[10]; }
    float& tx() { return m[12]; }
    const float& tx() const { return m[12]; }
    float& ty() { return m[13]; }
    const float& ty() const { return m[13]; }
    float& tz() { return m[14]; }
    const float& tz() const { return m[14]; }
    Vektor4f& x() { return reinterpret_cast<Vektor4f&>(m[0]); }
    const Vektor4f& x() const { return reinterpret_cast<const Vektor4f&>(m[0]); }
    Vektor4f& y() { return reinterpret_cast<Vektor4f&>(m[4]); }
    const Vektor4f& y() const { return reinterpret_cast<const Vektor4f&>(m[4]); }
    Vektor4f& z() { return reinterpret_cast<Vektor4f&>(m[8]); }
    const Vektor4f& z() const { return reinterpret_cast<const Vektor4f&>(m[8]); }
    Vektor4f& t() { return reinterpret_cast<Vektor4f&>(m[12]); }
    const Vektor4f& t() const { return reinterpret_cast<const Vektor4f&>(m[12]); }
    const float& operator[](std::size_t i) const { return m[i]; }
    float& operator[](std::size_t i) { return m[i]; }
};

inline const Matrix4 kEinheitsMatrix(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);   // g_matrix4_identity

inline bool operator==(const Matrix4& a, const Matrix4& b) {
    for (std::size_t i = 0; i < 16; ++i) {
        if (!(a[i] == b[i])) { return false; }
    }
    return true;
}

enum class Haendigkeit { Rechts, Links };   // Matrix4Handedness
inline Haendigkeit haendigkeit(const Matrix4& s) {   // matrix4_handedness
    return (punkt(kreuz(s.x().vec3(), s.y().vec3()), s.z().vec3()) < 0.0) ? Haendigkeit::Links : Haendigkeit::Rechts;
}

// matrix4_multiplied_by_matrix4: self * other (other wird zuerst angewandt).
inline Matrix4 multipliziert(const Matrix4& s, const Matrix4& o) {
    return Matrix4(
        o[0] * s[0] + o[1] * s[4] + o[2] * s[8] + o[3] * s[12],
        o[0] * s[1] + o[1] * s[5] + o[2] * s[9] + o[3] * s[13],
        o[0] * s[2] + o[1] * s[6] + o[2] * s[10] + o[3] * s[14],
        o[0] * s[3] + o[1] * s[7] + o[2] * s[11] + o[3] * s[15],
        o[4] * s[0] + o[5] * s[4] + o[6] * s[8] + o[7] * s[12],
        o[4] * s[1] + o[5] * s[5] + o[6] * s[9] + o[7] * s[13],
        o[4] * s[2] + o[5] * s[6] + o[6] * s[10] + o[7] * s[14],
        o[4] * s[3] + o[5] * s[7] + o[6] * s[11] + o[7] * s[15],
        o[8] * s[0] + o[9] * s[4] + o[10] * s[8] + o[11] * s[12],
        o[8] * s[1] + o[9] * s[5] + o[10] * s[9] + o[11] * s[13],
        o[8] * s[2] + o[9] * s[6] + o[10] * s[10] + o[11] * s[14],
        o[8] * s[3] + o[9] * s[7] + o[10] * s[11] + o[11] * s[15],
        o[12] * s[0] + o[13] * s[4] + o[14] * s[8] + o[15] * s[12],
        o[12] * s[1] + o[13] * s[5] + o[14] * s[9] + o[15] * s[13],
        o[12] * s[2] + o[13] * s[6] + o[14] * s[10] + o[15] * s[14],
        o[12] * s[3] + o[13] * s[7] + o[14] * s[11] + o[15] * s[15]);
}
inline void multipliziere(Matrix4& s, const Matrix4& o) { s = multipliziert(s, o); }   // matrix4_multiply_by_matrix4
inline void vormultipliziere(Matrix4& s, const Matrix4& o) { s = multipliziert(o, s); }   // matrix4_premultiply_by_matrix4

// matrix4_transformed_point - in der Genauigkeit des Punkts (double * float = double).
template<typename E>
inline Vektor3<E> transformierterPunkt(const Matrix4& s, const Vektor3<E>& p) {
    return Vektor3<E>(static_cast<E>(p[0] * s[0] + p[1] * s[4] + p[2] * s[8] + s[12]),
                      static_cast<E>(p[0] * s[1] + p[1] * s[5] + p[2] * s[9] + s[13]),
                      static_cast<E>(p[0] * s[2] + p[1] * s[6] + p[2] * s[10] + s[14]));
}
template<typename E>
inline void transformierePunkt(const Matrix4& s, Vektor3<E>& p) { p = transformierterPunkt(s, p); }
// matrix4_transformed_direction
template<typename E>
inline Vektor3<E> transformierteRichtung(const Matrix4& s, const Vektor3<E>& d) {
    return Vektor3<E>(static_cast<E>(d[0] * s[0] + d[1] * s[4] + d[2] * s[8]),
                      static_cast<E>(d[0] * s[1] + d[1] * s[5] + d[2] * s[9]),
                      static_cast<E>(d[0] * s[2] + d[1] * s[6] + d[2] * s[10]));
}
template<typename E>
inline void transformiereRichtung(const Matrix4& s, Vektor3<E>& d) { d = transformierteRichtung(s, d); }

inline void transponieren(Matrix4& s) {   // matrix4_transpose
    std::swap(s[1], s[4]);
    std::swap(s[2], s[8]);
    std::swap(s[3], s[12]);
    std::swap(s[6], s[9]);
    std::swap(s[7], s[13]);
    std::swap(s[11], s[14]);
}
inline Matrix4 transponiert(const Matrix4& s) {   // matrix4_transposed
    return Matrix4(s[0], s[4], s[8], s[12], s[1], s[5], s[9], s[13], s[2], s[6], s[10], s[14], s[3], s[7], s[11], s[15]);
}

// matrix4_affine_inverse (Graphics Gems 2). Determinante in double, Elemente
// werden beim Speichern zu float.
inline Matrix4 affinInvers(const Matrix4& s) {
    Matrix4 r;
    double det = s[0] * (s[5] * s[10] - s[9] * s[6]) - s[1] * (s[4] * s[10] - s[8] * s[6]) + s[2] * (s[4] * s[9] - s[8] * s[5]);
    det = 1.0 / det;
    r[0] = static_cast<float>((s[5] * s[10] - s[6] * s[9]) * det);
    r[1] = static_cast<float>(-(s[1] * s[10] - s[2] * s[9]) * det);
    r[2] = static_cast<float>((s[1] * s[6] - s[2] * s[5]) * det);
    r[3] = 0;
    r[4] = static_cast<float>(-(s[4] * s[10] - s[6] * s[8]) * det);
    r[5] = static_cast<float>((s[0] * s[10] - s[2] * s[8]) * det);
    r[6] = static_cast<float>(-(s[0] * s[6] - s[2] * s[4]) * det);
    r[7] = 0;
    r[8] = static_cast<float>((s[4] * s[9] - s[5] * s[8]) * det);
    r[9] = static_cast<float>(-(s[0] * s[9] - s[1] * s[8]) * det);
    r[10] = static_cast<float>((s[0] * s[5] - s[1] * s[4]) * det);
    r[11] = 0;
    r[12] = -(s[12] * r[0] + s[13] * r[4] + s[14] * r[8]);
    r[13] = -(s[12] * r[1] + s[13] * r[5] + s[14] * r[9]);
    r[14] = -(s[12] * r[2] + s[13] * r[6] + s[14] * r[10]);
    r[15] = 1;
    return r;
}
inline void affinInvertieren(Matrix4& s) { s = affinInvers(s); }   // matrix4_affine_invert
// matrix4_for_normal_transform: M^-T
inline Matrix4 normalenMatrix(const Matrix4& s) { return transponiert(affinInvers(s)); }
// matrix4_transformed_normal
template<typename E>
inline Vektor3<E> transformierteNormale(const Matrix4& s, const Vektor3<E>& n) {
    return normiert(transformierteRichtung(normalenMatrix(s), n));
}
// matrix4_translation_for_vec3
inline Matrix4 verschiebungsMatrix(const Vektor3f& v) {
    return Matrix4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, v[0], v[1], v[2], 1);
}
inline void verschiebeUm(Matrix4& s, const Vektor3f& v) { multipliziere(s, verschiebungsMatrix(v)); }   // matrix4_translate_by_vec3
// matrix4_scale_for_vec3
inline Matrix4 skalierungsMatrix(const Vektor3f& v) {
    return Matrix4(v[0], 0, 0, 0, 0, v[1], 0, 0, 0, 0, v[2], 0, 0, 0, 0, 1);
}
inline void skaliereUm(Matrix4& s, const Vektor3f& v) { multipliziere(s, skalierungsMatrix(v)); }   // matrix4_scale_by_vec3
// matrix4_pivoted_scale_by_vec3
inline void skaliereUmPunkt(Matrix4& s, const Vektor3f& v, const Vektor3f& pivot) {
    verschiebeUm(s, pivot);
    skaliereUm(s, v);
    verschiebeUm(s, -pivot);
}
// matrix4_rotation_for_sincos_x/y/z und matrix4_rotation_for_x/y/z(_degrees)
inline Matrix4 drehungSinCosX(float s, float c) { return Matrix4(1, 0, 0, 0, 0, c, s, 0, 0, -s, c, 0, 0, 0, 0, 1); }
inline Matrix4 drehungSinCosY(float s, float c) { return Matrix4(c, 0, -s, 0, 0, 1, 0, 0, s, 0, c, 0, 0, 0, 0, 1); }
inline Matrix4 drehungSinCosZ(float s, float c) { return Matrix4(c, s, 0, 0, -s, c, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1); }
inline Matrix4 drehungX(double x) { return drehungSinCosX(static_cast<float>(std::sin(x)), static_cast<float>(std::cos(x))); }
inline Matrix4 drehungY(double y) { return drehungSinCosY(static_cast<float>(std::sin(y)), static_cast<float>(std::cos(y))); }
inline Matrix4 drehungZ(double z) { return drehungSinCosZ(static_cast<float>(std::sin(z)), static_cast<float>(std::cos(z))); }
inline Matrix4 drehungXGrad(float x) { return drehungX(gradZuBogen(x)); }
inline Matrix4 drehungYGrad(float y) { return drehungY(gradZuBogen(y)); }
inline Matrix4 drehungZGrad(float z) { return drehungZ(gradZuBogen(z)); }

// --- Quaternion (libs/math/quaternion.h, Auszug) ---------------------------

using Quaternion = Vektor4f;
// quaternion_for_axisangle: sin(Winkel/2) als float, cos in double -> float.
inline Quaternion quaternionAchseWinkel(const Vektor3f& achse, double winkel) {
    winkel *= 0.5;
    const float sa = static_cast<float>(std::sin(winkel));
    return Quaternion(achse[0] * sa, achse[1] * sa, achse[2] * sa, static_cast<float>(std::cos(winkel)));
}
// matrix4_rotation_for_quaternion (der #else-Zweig, quaternion.h:146-176)
inline Matrix4 drehungAusQuaternion(const Quaternion& q) {
    const double x2 = q[0] + q[0];
    const double y2 = q[1] + q[1];
    const double z2 = q[2] + q[2];
    const double xx = q[0] * x2;
    const double xy = q[0] * y2;
    const double xz = q[0] * z2;
    const double yy = q[1] * y2;
    const double yz = q[1] * z2;
    const double zz = q[2] * z2;
    const double wx = q[3] * x2;
    const double wy = q[3] * y2;
    const double wz = q[3] * z2;
    return Matrix4(static_cast<float>(1.0 - (yy + zz)), static_cast<float>(xy + wz), static_cast<float>(xz - wy), 0,
                   static_cast<float>(xy - wz), static_cast<float>(1.0 - (xx + zz)), static_cast<float>(yz + wx), 0,
                   static_cast<float>(xz + wy), static_cast<float>(yz - wx), static_cast<float>(1.0 - (xx + yy)), 0,
                   0, 0, 0, 1);
}
// matrix4_rotation_for_axisangle
inline Matrix4 drehungAchseWinkel(const Vektor3f& achse, double winkel) {
    return drehungAusQuaternion(quaternionAchseWinkel(achse, winkel));
}
// quaternion_normalised (quaternion.h:82-90): Quadratsumme in float.
inline Quaternion quaternionNormiert(const Quaternion& q) {
    const float summe = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
    const double n = (1.0 / std::sqrt(static_cast<double>(summe)));
    return Quaternion(static_cast<float>(q[0] * n), static_cast<float>(q[1] * n), static_cast<float>(q[2] * n),
                      static_cast<float>(q[3] * n));
}
// vector3_min_abs_component_index (vector.h:560-564)
template<typename E>
inline std::size_t kleinsteBetragsKomponente(const Vektor3<E>& a) {
    const std::size_t mini = (std::fabs(a[1]) < std::fabs(a[0])) ? 1 : 0;
    return (std::fabs(a[2]) < std::fabs(a[mini])) ? 2 : mini;
}
// quaternion_for_unit_vectors_safe (quaternion.h:102-109): 180 Grad gesondert.
template<typename T>
inline Quaternion quaternionAusEinheitsvektorenSicher(const Vektor3<T>& von, const Vektor3<T>& nach) {
    const Quaternion q(Vektor3f(kreuz(von, nach)), static_cast<float>(1.0 + punkt(von, nach)));
    const float len = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
    return len != 0 ? quaternionNormiert(q)
                    : quaternionNormiert(Quaternion(Vektor3f(kreuz(von, kAchsen[kleinsteBetragsKomponente(von)])), 0));
}

// --- Achsparalleler Kasten (libs/math/aabb.h) -------------------------------

class Kasten {   // AABB
public:
    Vektor3f origin{0, 0, 0}, extents{-1, -1, -1};
    Kasten() = default;
    Kasten(const Vektor3f& o, const Vektor3f& e) : origin(o), extents(e) {}
};
constexpr float kKastenMax = FLT_MAX;   // c_aabb_max
inline bool ausdehnungGueltig(float f) { return f >= 0 && f <= kKastenMax; }   // extents_valid
inline bool ursprungGueltig(float f) { return f >= -kKastenMax && f <= kKastenMax; }   // origin_valid
inline bool kastenGueltig(const Kasten& k) {   // aabb_valid
    return ursprungGueltig(k.origin[0]) && ursprungGueltig(k.origin[1]) && ursprungGueltig(k.origin[2])
        && ausdehnungGueltig(k.extents[0]) && ausdehnungGueltig(k.extents[1]) && ausdehnungGueltig(k.extents[2]);
}
inline Kasten kastenAusMinMax(const Vektor3f& mn, const Vektor3f& mx) {   // aabb_for_minmax
    Kasten k;
    k.origin = mitte(mn, mx);
    k.extents = mx - k.origin;
    return k;
}
// AABBExtend<Index>::apply(point)
inline void kastenErweiternAchse(Kasten& k, const Vektor3f& p, std::size_t i) {
    const float verschiebung = p[i] - k.origin[i];
    const float halbeDifferenz = 0.5f * (std::fabs(verschiebung) - k.extents[i]);
    if (halbeDifferenz > 0) {
        k.origin[i] += std::copysign(halbeDifferenz, verschiebung);
        k.extents[i] += halbeDifferenz;
    }
}
inline void kastenErweitern(Kasten& k, const Vektor3f& p) {   // aabb_extend_by_point
    kastenErweiternAchse(k, p, 0);
    kastenErweiternAchse(k, p, 1);
    kastenErweiternAchse(k, p, 2);
}
inline void kastenErweiternSicher(Kasten& k, const Vektor3f& p) {   // aabb_extend_by_point_safe
    if (kastenGueltig(k)) {
        kastenErweitern(k, p);
    } else {
        k.origin = p;
        k.extents = Vektor3f(0, 0, 0);
    }
}
// AABBExtend<Index>::apply(AABB)
inline void kastenErweiternAchse(Kasten& k, const Kasten& o, std::size_t i) {
    const float verschiebung = o.origin[i] - k.origin[i];
    const float differenz = o.extents[i] - k.extents[i];
    if (std::fabs(verschiebung) > std::fabs(differenz)) {
        const float halbeDifferenz = 0.5f * (std::fabs(verschiebung) + differenz);
        if (halbeDifferenz > 0) {
            k.origin[i] += std::copysign(halbeDifferenz, verschiebung);
            k.extents[i] += halbeDifferenz;
        }
    } else if (differenz > 0) {
        k.origin[i] = o.origin[i];
        k.extents[i] = o.extents[i];
    }
}
inline void kastenErweiternSicher(Kasten& k, const Kasten& o) {   // aabb_extend_by_aabb_safe
    if (kastenGueltig(k) && kastenGueltig(o)) {
        kastenErweiternAchse(k, o, 0);
        kastenErweiternAchse(k, o, 1);
        kastenErweiternAchse(k, o, 2);
    } else if (kastenGueltig(o)) {
        k = o;
    }
}

}  // namespace bhed::karte
#endif
