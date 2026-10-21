// auswahltest.h - Karten-Editor: Treffertests fuer die Auswahl.
//
// Portiert aus NetRadiant-Custom 20260114: include/selectable.h
// (SelectionIntersection, assign_if_closer), libs/math/frustum.h
// (homogenous_clip_point/_line/_triangle, matrix4_clip_*, Vector4ClipPolygon),
// libs/math/line.h (line_closest_point, ray_distance_to_plane,
// ray_intersect_plane), libs/math/plane.h (plane3_for_points),
// radiant/view.h (View: Construct, EnableScissor, viewer_from_viewproj),
// radiant/selection_.h (ConstructSelectionTest, SelectionBoxForPoint,
// SelectionBoxForArea, point_on_plane, point_on_axis, point_on_sphere,
// ray_for_device_point, translation_local2object, angle_for_axis, ...),
// radiant/rect_t.h und radiant/selection_volume.h (BestPoint,
// point_test_polygon_2d, SelectionVolume) sowie radiant/selection_selector.h
// (SelectionPool, BooleanSelector, BestSelector, BestPointSelector).
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
#ifndef BHED_KARTE_AUSWAHLTEST_H
#define BHED_KARTE_AUSWAHLTEST_H

#include "bhed/karte/kamera.h"
#include "bhed/karte/mathe.h"

#include <array>
#include <cstddef>
#include <vector>

namespace bhed::karte {

// SelectionIntersection (selectable.h:30-92)
class Treffer {
public:
    Treffer() = default;
    Treffer(float tiefe, float abstand) : m_tiefe(tiefe), m_abstand(abstand), m_indirekt(abstand != 0) {}
    Treffer(float tiefe, float abstand, float tiefe2)
        : m_tiefe(tiefe), m_abstand(abstand), m_tiefe2(tiefe2), m_indirekt(abstand != 0) {}
    bool operator<(const Treffer& o) const;
    [[nodiscard]] bool gleichEps(const Treffer& o, float abstandEps, float tiefeEps) const;   // equalEpsilon
    [[nodiscard]] float tiefe() const { return m_tiefe; }
    [[nodiscard]] float abstand() const { return m_abstand; }
    [[nodiscard]] float tiefe2() const { return m_tiefe2; }
    [[nodiscard]] bool gueltig() const { return m_tiefe < 1; }   // valid
    [[nodiscard]] bool indirekt() const { return m_indirekt; }

private:
    float m_tiefe = 1;
    float m_abstand = 2;
    float m_tiefe2 = -1;
    bool m_indirekt = true;
};
// assign_if_closer
inline void naeherZuweisen(Treffer& bester, const Treffer& neu) {
    if (neu < bester) { bester = neu; }
}

// rect_t (rect_t.h)
struct AuswahlRechteck {
    enum class Art { Auswaehlen, Abwaehlen, Umschalten };   // eSelect, eDeselect, eToggle
    float min[2]{0, 0};
    float max[2]{0, 0};
    Art art = Art::Auswaehlen;
};
using GeraeteVektor = Vektor2f;   // DeviceVector
[[nodiscard]] AuswahlRechteck auswahlKastenPunkt(const GeraeteVektor& p, const GeraeteVektor& eps);      // SelectionBoxForPoint
[[nodiscard]] AuswahlRechteck auswahlKastenFlaeche(const GeraeteVektor& p, const GeraeteVektor& delta);  // SelectionBoxForArea

// View (radiant/view.h): Modelview, Projektion, Viewport, Schere.
class Blick {
public:
    explicit Blick(bool fuellen = false) : m_fuellen(fuellen) {}
    // View::Construct
    void bauen(const Matrix4& projektion, const Matrix4& modelview, std::size_t breite, std::size_t hoehe);
    // EnableScissor / DisableScissor
    void scheren(float minX, float maxX, float minY, float maxY);
    void ohneSchere();
    [[nodiscard]] const Matrix4& sicht() const { return m_viewproj; }   // GetViewMatrix
    [[nodiscard]] const Matrix4& bildfeld() const { return m_viewport; }
    [[nodiscard]] const Matrix4& modelview() const { return m_modelview; }
    [[nodiscard]] const Matrix4& projektion() const { return m_projektion; }
    [[nodiscard]] bool fuellen() const { return m_fuellen; }   // fill
    [[nodiscard]] Vektor3f betrachter() const { return m_betrachter.vec3(); }   // getViewer
    [[nodiscard]] const Vektor3f& blickrichtung() const { return m_blickrichtung; }   // getViewDir
    [[nodiscard]] const Frustum& frustum() const { return m_frustum; }

private:
    void neu();   // construct
    Matrix4 m_modelview = kEinheitsMatrix;
    Matrix4 m_projektion = kEinheitsMatrix;
    Matrix4 m_viewport = kEinheitsMatrix;
    Matrix4 m_schere = kEinheitsMatrix;
    Matrix4 m_viewproj = kEinheitsMatrix;
    Vektor4f m_betrachter{0, 0, 0, 1};
    Frustum m_frustum;
    Vektor3f m_blickrichtung{0, 0, 1};
    bool m_fuellen = false;
};
// ConstructSelectionTest
void auswahlTestBauen(Blick& blick, const AuswahlRechteck& kasten);

enum class Wegschneiden { Keins, CW, CCW };   // EClipCull

// SelectionVolume (selection_volume.h:272-503)
class AuswahlVolumen {
public:
    explicit AuswahlVolumen(const Blick& blick) : m_blick(blick) {}
    [[nodiscard]] const Blick& blick() const { return m_blick; }   // getVolume
    [[nodiscard]] const Matrix4& bildZuWelt() const { return m_bildZuWelt; }   // getScreen2world
    void netzBeginnen(const Matrix4& lokalZuWelt, bool zweiseitig = false);   // BeginMesh
    void testPunkt(const Vektor3f& punkt, Treffer& bester) const;            // TestPoint
    void testPolygon(const Vektor3d* punkte, std::size_t anzahl, Treffer& bester, const EbenenPunkte& ebene) const;
    void testLinienSchleife(const Vektor3f* punkte, std::size_t anzahl, Treffer& bester) const;
    void testLinienZug(const Vektor3f* punkte, std::size_t anzahl, Treffer& bester) const;
    void testLinien(const Vektor3f* punkte, std::size_t anzahl, Treffer& bester) const;
    void testDreiecke(const Vektor3f* punkte, const unsigned int* indizes, std::size_t anzahl, Treffer& bester) const;
    void testVierecke(const Vektor3f* punkte, const unsigned int* indizes, std::size_t anzahl, Treffer& bester) const;
    // TestQuadStrip (Punkte im Abstand "schritt" Bytes, z. B. NetzPunkt::vertex)
    void testViereckStreifen(const unsigned char* punkte, std::size_t schritt, const unsigned int* indizes, std::size_t anzahl,
                             Treffer& bester) const;

private:
    const Blick& m_blick;
    Matrix4 m_lokalZuSicht = kEinheitsMatrix;
    Matrix4 m_bildZuWelt = kEinheitsMatrix;
    Wegschneiden m_wegschneiden = Wegschneiden::Keins;
};

// --- BestPoint-Tests fuer Manipulatoren (selection_volume.h:155-271) --------

// Point_BestPoint, LineStrip_BestPoint, LineLoop_BestPoint, Line_BestPoint
void punktBesterPunkt(const Matrix4& lokalZuSicht, const Vektor3f& punkt, Treffer& bester);
void linienZugBesterPunkt(const Matrix4& lokalZuSicht, const Vektor3f* punkte, std::size_t anzahl, Treffer& bester);
void linienSchleifeBesterPunkt(const Matrix4& lokalZuSicht, const Vektor3f* punkte, std::size_t anzahl, Treffer& bester);
void linieBesterPunkt(const Matrix4& lokalZuSicht, const Vektor3f punkte[2], Treffer& bester);
// Circle_BestPoint (Faecher um den Ursprung), Quad_BestPoint, AABB_BestPoint,
// Triangles_BestPoint (je drei Punkte ein Dreieck)
void kreisBesterPunkt(const Matrix4& lokalZuSicht, Wegschneiden w, const Vektor3f* punkte, std::size_t anzahl, Treffer& bester);
void viereckBesterPunkt(const Matrix4& lokalZuSicht, Wegschneiden w, const Vektor3f punkte[4], Treffer& bester);
void kastenBesterPunkt(const Matrix4& lokalZuSicht, Wegschneiden w, const Kasten& kasten, Treffer& bester);
void dreieckeBesterPunkt(const Matrix4& lokalZuSicht, Wegschneiden w, const Vektor3f* punkte, std::size_t anzahl, Treffer& bester);
// matrix4_clip_line_by_nearplane: 2 = (teilweise) vor der Nahebene, dann
// stehen start/ende projiziert da; 0 = ganz dahinter.
std::size_t linieAnNahebeneKlippen(const Matrix4& m, Vektor3f& start, Vektor3f& ende);
// aabb_corners (aabb.h:187-200)
[[nodiscard]] std::array<Vektor3f, 8> kastenEcken(const Kasten& k);

// --- Rechenhilfen aus selection_.h -------------------------------------------

struct Strahl3 {   // Ray (float)
    Vektor3f ursprung{0, 0, 0};
    Vektor3f richtung{0, 0, 1};
};
[[nodiscard]] Vektor3f punktFuerGeraetePunkt(const Matrix4& geraetZuObjekt, const GeraeteVektor& xy, float z);   // point_for_device_point
[[nodiscard]] Strahl3 strahlFuerGeraetePunkt(const Matrix4& geraetZuObjekt, const GeraeteVektor& xy);           // ray_for_device_point
[[nodiscard]] Vektor3f punktAufKugel(const Matrix4& geraetZuObjekt, const GeraeteVektor& xy, float radius = 64);  // point_on_sphere
[[nodiscard]] Vektor3f punktAufAchse(const Vektor3f& achse, const Matrix4& geraetZuObjekt, const GeraeteVektor& xy);  // point_on_axis
[[nodiscard]] Vektor3f punktAufEbeneGeraet(const Matrix4& geraetZuObjekt, const GeraeteVektor& xy);               // point_on_plane( device2object, xy )
[[nodiscard]] Vektor3f punktAufEbeneSicht(const Ebene3d& ebene, const Matrix4& objektZuGeraet, const GeraeteVektor& xy);  // point_on_plane( plane, object2device, xy )
[[nodiscard]] Vektor3f strahlSchnittEbene3(const Strahl3& s, const Ebene3d& e);   // ray_intersect_plane
[[nodiscard]] Vektor3f translationLokalZuObjekt(const Vektor3f& lokal, const Matrix4& lokalZuObjekt);   // translation_local2object
[[nodiscard]] float winkelZwischen(const Vektor3f& a, const Vektor3f& b);   // angle_between
void aufAchseBeschraenken(Vektor3f& v, const Vektor3f& achse);              // constrain_to_axis
[[nodiscard]] float winkelFuerAchse(const Vektor3f& a, const Vektor3f& b, const Vektor3f& achse);   // angle_for_axis
[[nodiscard]] float abstandFuerAchse(const Vektor3f& a, const Vektor3f& b, const Vektor3f& achse);  // distance_for_axis
// window_to_normalised_device / device_constrained
[[nodiscard]] GeraeteVektor fensterZuGeraet(float x, float y, int breite, int hoehe);
[[nodiscard]] GeraeteVektor geraetBegrenzt(const GeraeteVektor& g);
// plane3_for_points
[[nodiscard]] Ebene3d ebeneDreiPunkte(const Vektor3f& p0, const Vektor3f& p1, const Vektor3f& p2);

}  // namespace bhed::karte

#endif
