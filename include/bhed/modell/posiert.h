// modell/posiert.h - Bearbeiten in der Pose (mbvers)
//
// shank, 10.11.2026: "ein bug wenn ich was bewege, dann geht ein mesh nach links obwohl ich alles nach rechts bewege".
// Der Model-Tab zeigt das Netz in der Pose der Animation, das Dokument fuehrt die Ecken in der Bindepose. Was in der
// gezeigten Ansicht gezogen, gedreht oder skaliert wird, rechnet dieses Modul je Ecke exakt in die Bindepose zurueck:
// ueber die Umkehrung ihrer gemischten Knochenmatrix (lineares Skinning wie die Vorschau und das Spiel,
// p = Summe w_i M_i v; G2_SkinSurface/RB_SurfaceGhoul2, tr_ghoul2.cpp). So bewegt sich, was man sieht, so, wie man zieht
// - wie Blender im Bearbeitungsmodus unter einem Armature-Modifier (transform_convert_mesh.cc,
// transform_convert_mesh_crazyspace_transdata_set: je Ecke die Verformungsmatrix `defmat` und ihre Umkehrung
// invert_m3_m3).
//
// Die Bindepose selbst (bindepose(): jede Weltmatrix ist die der Grundstellung) und Modelle ohne Skelett rechnen nichts
// um: die Aufrufer nehmen dann ihren bisherigen Weg (bitgleich).
#ifndef BHED_MODELL_POSIERT_H
#define BHED_MODELL_POSIERT_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "bhed/gla.h"
#include "bhed/glm.h"
#include "bhed/modell/wof/poly.h"

namespace bhed::modell {

// Ist die Pose `welt` (GlaAnimation::worldMatrices...) exakt die Bindepose des Skeletts? Dann ist jede Skin-Matrix
// (Welt * BasePoseInv) die Einheit und jede gemischte Matrix auch: das Gezeigte IST die Bindepose. Geprueft wird ohne
// Schwelle: jede Weltmatrix gleicht Wert fuer Wert der, die dieselbe Kette (gla.cpp: Welt = Welt(Eltern) * BasePoseInv
// (Eltern) * lokal * BasePose) mit der Einheit als lokaler Matrix jedes Knochens liefert. Ein leeres Skelett: true.
// Gemessen (Y:\werkzeuge\mbv\gla_bild0.py): Bild 0 der _humanoid.gla ist NICHT die Bindepose (kein Knochen: die Figur
// steht 24 bis 27 tiefer, die Oberarme sind 48,1 Grad gedreht, die Beine 3,3 Grad).
[[nodiscard]] bool bindepose(const std::vector<GlaBone>& bones, const std::vector<BoneMatrix>& welt);

// Der gezeigte Ort einer Ecke wie haut() der Ansicht: Summe w_i (M_i v) ueber die Knochen mit Gewicht > 0 und einer
// Nummer im Skelett; ohne Knochen der Ort selbst.
void gezeigterOrt(const std::vector<BoneMatrix>& knochen, const GlmVertex& v, float out[3]);

// Die gemischte Matrix einer Ecke und ihre Umkehrung (double).
struct Mischung {
    double a[3][4]{};       // Summe w_i M_i: gezeigt = a * (v, 1)
    double inv[3][4]{};     // Bindepose = inv * (p, 1), wenn `umkehrbar`
    double stark[3][4]{};   // die Umkehrung des staerksten Knochens allein, wenn `hatStark`
    double n[3][3]{};       // die Normale dreht der erste Knochen (G2_SkinSurface: G2_GetVertBoneIndex(v, 0); REST3d)
    double nInv[3][3]{};
    bool umkehrbar = false;   // Determinante der Mischung != 0 (Blender invert_m3_m3)
    bool hatStark = false;
};
// `knochen` nicht leer.
[[nodiscard]] Mischung mischung(const std::vector<BoneMatrix>& knochen, const GlmVertex& v);

// Der gezeigte Ort in double (a * v), ohne die Rundung der Ansicht auf float: hin und gleich wieder zurueck kommt bis auf
// die Rundung der Umkehrung in double (2^-53 mal Ort mal Kondition) am selben Ort an. Bitgleich ist das nicht (aus einer
// 0 wird 1e-15): wer eine Ecke nicht bewegt, laesst sie stehen, statt sie hin und zurueck zu rechnen.
void ortGezeigt(const Mischung& m, const float v[3], double p[3]);

// Wie ein gezeigter Ort in die Bindepose zurueckkam.
enum class Rueckweg : std::uint8_t {
    Exakt,        // ueber die Umkehrung der Mischung
    Staerkster,   // die Mischung ist in dieser Pose nicht umkehrbar: nach dem staerksten Knochen (starr)
    Keiner,       // kein gueltiger Knochen: die Ecke bleibt, `v` unveraendert
};
// Nicht umkehrbar heisst: die Determinante ist 0, oder der zurueckgerechnete Ort waere keine Koordinate des Spiels (nicht
// endlich oder ausserhalb von +-kWeltGrenze). Keine NaN, kein Ort ausserhalb der Welt.
Rueckweg ortZurueck(const Mischung& m, const double p[3], float v[3]);
// MAX_WORLD_COORD (64 * 1024; code/qcommon/q_shared.h:802 OpenJK): weiter reicht keine Koordinate des Spiels.
inline constexpr double kWeltGrenze = 64.0 * 1024.0;

// Eine Normale der Bindepose, wie die Ansicht sie zeigt (n * normale), und zurueck (nInv * gezeigt; nicht normiert). Das
// Gezeigte in double (wie ortGezeigt).
void normaleGezeigt(const Mischung& m, const float normale[3], double out[3]);
void normaleZurueck(const Mischung& m, const double gezeigt[3], float out[3]);

namespace wof {

// Das Arbeitsnetz der Modellier-Werkzeuge in der Pose: die Punkte gezeigt (Gewichte der ersten Ecke je Punkt), die Orte
// der Bindepose zum bitgleichen Zuruecklegen unveraenderter Punkte. Die Normalen der Ecken bleiben in der Bindepose (die
// Werkzeuge mischen sie nur; neu gerechnet werden sie in normalenNeu und dann zurueckgedreht).
struct NetzPose {
    bool aktiv = false;
    std::size_t punkte = 0;
    std::vector<std::array<float, 3>> bind;
    std::vector<std::array<float, 3>> gezeigt;
};
// Ohne Knochen: nichts (aktiv = false).
[[nodiscard]] NetzPose inPose(PolyNetz& n, const std::vector<BoneMatrix>& knochen);
// Die Punkte, deren Ecken normalenNeu neu rechnet (Ecken an neuen Vielecken) - vor normalenNeu aufrufen.
[[nodiscard]] std::vector<char> normalenPunkte(const PolyNetz& n);
// Zurueck in die Bindepose: unveraenderte Punkte bitgleich, neue und bewegte ueber die Umkehrung ihrer Mischung; die
// Normalen der Ecken an `normalenNeu`-Punkten und an neuen Punkten ueber nInv. Liefert die Zahl der Punkte, deren
// Mischung nicht umkehrbar war (Rueckweg::Staerkster oder Keiner).
int ausPose(PolyNetz& n, const NetzPose& p, const std::vector<BoneMatrix>& knochen, const std::vector<char>& normalenNeu);

}  // namespace wof
}  // namespace bhed::modell

#endif
