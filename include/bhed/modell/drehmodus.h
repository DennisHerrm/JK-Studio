// modell/drehmodus.h - die Drehmodi eines Knochens wie Blender (ENTSCHEID3A B9, rc612)
//
// Blenders Seitenleiste (N, Reiter Item) zeigt fuer einen Knochen im Posenmodus die lokalen Werte des Pose-Kanals
// (matrix_basis: bezogen auf die Ruhelage, in den Achsen des Knochens) und die Drehung im Drehmodus des Knochens
// (PoseBone.rotation_mode: Quaternion (WXYZ), sechs Euler-Reihenfolgen, Achse/Winkel). Diese Datei rechnet zwischen einer
// Drehmatrix und den Werten jedes Modus - mit Blenders Formeln (BLI_math_rotation: eulO_to_mat3, mat3_normalized_to_eulO,
// mat3_normalized_to_quat, quat_to_axis_angle, axis_angle_to_mat3), belegt gegen Blender 5.0 mathutils
// (tests/modell_drehmodus.cpp, Referenz Y:\werkzeuge\ent3a\blender_dreh.py).
//
// Matrizen: m[Zeile][Spalte], Drehung von Spaltenvektoren (wie BoneMatrix). Winkel im Bogenmass.
#ifndef BHED_MODELL_DREHMODUS_H
#define BHED_MODELL_DREHMODUS_H

#include <cstdint>

namespace bhed::modell {

// Reihenfolge wie Blenders Aufzaehlung rotation_mode (QUATERNION, XYZ, XZY, YXZ, YZX, ZXY, ZYX, AXIS_ANGLE). "XYZ Euler" dreht
// zuerst um X, dann Y, dann Z: R = Rz * Ry * Rx.
enum class DrehModus : std::uint8_t { Quaternion = 0, Xyz, Xzy, Yxz, Yzx, Zxy, Zyx, AchseWinkel };
inline constexpr int kDrehModi = 8;

// Wie viele Werte der Modus zeigt: 4 (W, X, Y, Z bzw. Winkel, X, Y, Z) oder 3 (Euler X, Y, Z).
[[nodiscard]] int drehWerteZahl(DrehModus m);
[[nodiscard]] bool istEuler(DrehModus m);

// Euler (Blender eulO_to_mat3 / mat3_normalized_to_eulO: von den zwei Loesungen die mit der kleineren Summe der Betraege).
void matrixAusEuler(const float e[3], DrehModus o, float m[3][3]);
void eulerAusMatrix(const float m[3][3], DrehModus o, float e[3]);
// Quaternion w, x, y, z (Blender mat3_normalized_to_quat: kanonisch mit w >= 0; quat_to_mat3 nach dem Normieren wie
// BKE_pchan_rot_to_mat3 - ein eingetippter Wert muss nicht normiert sein).
void quatAusMatrix(const float m[3][3], float q[4]);
void matrixAusQuat(const float q[4], float m[3][3]);
// Achse/Winkel: aw = Winkel, x, y, z (Blender quat_to_axis_angle; ohne Drehung die Achse Y wie Blender; axis_angle_to_mat3
// normiert die Achse, eine Nullachse ist keine Drehung).
void achseWinkelAusQuat(const float q[4], float aw[4]);
void matrixAusAchseWinkel(const float aw[4], float m[3][3]);

// Die Werte des Modus aus einer Drehmatrix (Spalten normiert) und zurueck. werte[0..2] sind bei Euler X, Y, Z; sonst w/Winkel, x, y, z.
void drehWerte(const float m[3][3], DrehModus o, float werte[4]);
void matrixAusDrehWerten(const float werte[4], DrehModus o, float m[3][3]);

}  // namespace bhed::modell
#endif
