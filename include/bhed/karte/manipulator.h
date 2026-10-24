// manipulator.h - Karten-Editor: Rechnung fuer die Manipulatoren.
//
// Portiert aus NetRadiant-Custom 20260114: libs/transformlib.h (Skew,
// Transforms, matrix4_transform_for_components), libs/math/quaternion.h
// (quaternion_multiplied_by_quaternion, quaternion_inverse,
// quaternion_for_unit_vectors, matrix4_rotation_for_quaternion_quantised),
// libs/math/matrix.h (matrix4_get_rotation_euler_xyz_degrees,
// matrix4_get_scale_vec3_signed), plugins/entity/angles.h (angles_rotated,
// matrix4_rotation_for_euler_xyz_degrees_quantised), plugins/entity/angle.h
// (ANGLEKEY_SMALLEST), libs/pivot.h (billboard_*, ConstructObject2Screen,
// pivot_scale, pivot_perspective, ConstructDevice2Manip, Pivot2World_*),
// radiant/selection_.h (Pivot2World, ModifierFlagsExt),
// radiant/selection_mtable_translate.h (aabb_snap_translation,
// quaternion_for_unit_vectors_for_bounds), radiant/selection_render.h und
// libs/render.h (g_colour_*, draw_arrowline, draw_arrowhead, draw_circle,
// draw_semicircle, draw_quad).
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
#ifndef BHED_KARTE_MANIPULATOR_H
#define BHED_KARTE_MANIPULATOR_H

#include "bhed/karte/mathe.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace bhed::karte {

// --- Zusatztasten (selection_.h: ModifierFlagsExt g_modifiers) ----------------

struct Zusatz {
    bool shift = false;
    bool strg = false;
    bool alt = false;
    [[nodiscard]] bool keine() const { return !shift && !strg && !alt; }   // == c_modifierNone
    [[nodiscard]] bool genau(bool s, bool c, bool a) const { return shift == s && strg == c && alt == a; }
};

// --- Transformationen (libs/transformlib.h) ------------------------------------

// Skew: ein Element der Matrix (index 4: x(y) usw.) bekommt "wert" dazu.
struct Verzerrung {
    std::size_t index = 4;   // c_skew_identity = Skew( 4, 0 )
    float wert = 0;
    bool operator==(const Verzerrung& o) const { return (wert == 0 && o.wert == 0) || (index == o.index && wert == o.wert); }
};
inline const Quaternion kQuaternionEins(0, 0, 0, 1);   // c_quaternion_identity

// Transforms
struct Transformationen {
    Vektor3f verschiebung{0, 0, 0};
    Quaternion drehung = kQuaternionEins;
    Vektor3f skalierung{1, 1, 1};
    Verzerrung verzerrung;
    [[nodiscard]] bool eins() const;   // isIdentity
    [[nodiscard]] Matrix4 matrix() const;   // calculateTransform
};

// matrix4_rotation_for_quaternion_quantised (quaternion.h:184-210)
[[nodiscard]] Matrix4 drehungAusQuaternionQuantisiert(const Quaternion& q);
// matrix4_transform_for_components (transformlib.h)
[[nodiscard]] Matrix4 transformAusKomponenten(const Vektor3f& t, const Quaternion& r, const Vektor3f& s, const Verzerrung& v);
// quaternion_multiplied_by_quaternion / quaternion_inverse
[[nodiscard]] Quaternion quaternionMal(const Quaternion& a, const Quaternion& b);
[[nodiscard]] Quaternion quaternionInvers(const Quaternion& q);
// quaternion_for_unit_vectors: normiert ( kreuz, 1 + punkt )
[[nodiscard]] Quaternion quaternionAusEinheitsvektoren(const Vektor3f& von, const Vektor3f& nach);
// quaternion_for_unit_vectors_for_bounds
[[nodiscard]] Quaternion quaternionFuerKasten(const Vektor3f& axialVon, const Vektor3f& nach);
// matrix4_get_rotation_euler_xyz_degrees
[[nodiscard]] Vektor3f eulerXyzGrad(const Matrix4& m);
// matrix4_rotation_for_euler_xyz_degrees_quantised (angles.h)
[[nodiscard]] Matrix4 drehungEulerXyzGradQuantisiert(const Vektor3f& winkel);
// angles_rotated (angles.h:108-118), ANGLEKEY_SMALLEST 0.0001
[[nodiscard]] Vektor3f winkelGedreht(const Vektor3f& winkel, const Quaternion& drehung);
// matrix4_get_scale_vec3_signed
[[nodiscard]] Vektor3f skalierungMitVorzeichen(const Matrix4& m);
// aabb_snap_translation: die geaenderten Achsen so, dass der Kasten am
// naechsten Rasterstrich liegt.
void kastenVerschiebungEinrasten(Vektor3f& v, const Kasten& grenzen, float fang);
// aabb_for_oriented_aabb (aabb.h)
[[nodiscard]] Kasten kastenFuerGedrehtenKasten(const Kasten& k, const Matrix4& m);

// --- Pivot (libs/pivot.h, radiant/selection_.h) ---------------------------------

void pivotWeltRaum(Matrix4& aus, const Matrix4& pivotZuWelt, const Matrix4& mv, const Matrix4& proj, const Matrix4& vp);
void pivotBlickpunktRaum(Matrix4& aus, Vektor3f& achse, const Matrix4& pivotZuWelt, const Matrix4& mv, const Matrix4& proj,
                         const Matrix4& vp);
void pivotBlickebenenRaum(Matrix4& aus, const Matrix4& pivotZuWelt, const Matrix4& mv, const Matrix4& proj, const Matrix4& vp);
// ConstructDevice2Manip
[[nodiscard]] Matrix4 geraetZuManip(const Matrix4& objektZuWelt, const Matrix4& mv, const Matrix4& proj, const Matrix4& vp);

struct PivotZuWelt {   // Pivot2World
    Matrix4 weltRaum;          // m_worldSpace
    Matrix4 blickpunktRaum;    // m_viewpointSpace
    Matrix4 blickebenenRaum;   // m_viewplaneSpace
    Vektor3f bildAchse;        // m_axis_screen
    void aktualisieren(const Matrix4& pivotZuWelt, const Matrix4& mv, const Matrix4& proj, const Matrix4& vp);
};

// --- Zeichnung der Manipulatoren -------------------------------------------------

struct ManipFarbe {   // Colour4b
    std::uint8_t r = 0, g = 0, b = 0, a = 255;
    bool operator==(const ManipFarbe& o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }
};
inline constexpr ManipFarbe kFarbeX{255, 0, 0, 255};             // g_colour_x
inline constexpr ManipFarbe kFarbeY{0, 255, 0, 255};             // g_colour_y
inline constexpr ManipFarbe kFarbeZ{0, 0, 255, 255};             // g_colour_z
inline constexpr ManipFarbe kFarbeKugel{0, 0, 0, 255};           // g_colour_sphere
inline constexpr ManipFarbe kFarbeBild{0, 255, 255, 255};        // g_colour_screen
inline constexpr ManipFarbe kFarbeGewaehlt{255, 255, 0, 255};    // g_colour_selected
// colourSelected
inline ManipFarbe farbeGewaehlt(const ManipFarbe& f, bool gewaehlt) { return gewaehlt ? kFarbeGewaehlt : f; }

// Was ein Manipulator in einer Ansicht zeichnet - alles in Weltkoordinaten.
struct ManipZeichnung {
    struct Linie {
        Vektor3f a, b;
        ManipFarbe farbe;
    };
    struct Dreieck {
        Vektor3f p[3];
        Vektor3f n[3];
        ManipFarbe farbe;
    };
    struct Punkt {
        Vektor3f p;
        ManipFarbe farbe;
    };
    std::vector<Linie> draht;                    // $WIRE_OVERLAY
    std::vector<Linie> ohneTiefe;                // $POINT (Skalier-Manipulator: ohne Tiefentest)
    std::vector<Dreieck> flach;                  // $FLATSHADE_OVERLAY
    std::vector<Punkt> grossePunkte;             // $BIGPOINT
    std::vector<std::vector<Vektor3f>> ebenen;   // $PLANE_WIRE_OVERLAY: Polygone
    std::vector<Linie> ebenenLinien;             // $PLANE_WIRE_OVERLAY: Kreis um eine Ecke
    // UV-Werkzeug (selection_mtor_uv.cpp)
    std::vector<Linie> blendLinien;              // $BLENDLINE: Gitter, Pivotlinien, Kreis (mit Alpha, ohne Tiefentest)
    std::vector<Linie> pivotLinien;              // $PIVOT (RenderablePivot): 2 breit, mit Tiefentest
    struct TexturNetz {                          // $uvtool/patchtexture: Dreiecke mit der Textur eines Shaders
        std::string shader;
        std::vector<Vektor3f> punkte;
        std::vector<Vektor2f> st;
        Vektor3f normale;
    };
    std::vector<TexturNetz> texturNetze;
    void leeren() {
        draht.clear();
        ohneTiefe.clear();
        flach.clear();
        grossePunkte.clear();
        ebenen.clear();
        ebenenLinien.clear();
        blendLinien.clear();
        pivotLinien.clear();
        texturNetze.clear();
    }
};

// Geometrie wie NRC sie einmal baut (render.h, selection_render.h,
// selection_mtor_rotate.cpp). "achse": 0 = x, 1 = y, 2 = z.
constexpr float kPfeilKopfLaenge = 16;   // arrowhead_length
constexpr float kPfeilKopfRadius = 4;    // arrowhead_radius
void pfeilLinie(float laenge, Vektor3f linie[2], std::size_t achse);   // draw_arrowline
struct FlachPunkt {   // FlatShadedVertex
    Vektor3f p;
    Vektor3f n;
};
// draw_arrowhead mit TripleRemapXYZ (achse 0), YZX (1), ZXY (2)
void pfeilKopf(std::size_t segmente, float laenge, FlachPunkt* punkte, std::size_t achse);
enum class Umordnung { XYZ, YZX, ZXY };   // RemapXYZ / RemapYZX / RemapZXY
void kreisPunkte(std::size_t segmente, float radius, Vektor3f* punkte, Umordnung u);       // draw_circle (segmente << 3 Punkte)
void halbkreisPunkte(std::size_t segmente, float radius, Vektor3f* punkte, Umordnung u);   // draw_semicircle ((segmente << 2) + 1)
void viereckPunkte(float radius, Vektor3f viereck[4]);                                    // draw_quad

}  // namespace bhed::karte

#endif
