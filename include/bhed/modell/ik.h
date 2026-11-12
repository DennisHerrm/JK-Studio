// modell/ik.h - Modell-Block Stufe 4: IK/FK auf dem Control Rig (Arme und Beine), Fussrolle, Einrasten, Space Switching
//
// Zwei-Glied-IK  analytisch (Kosinussatz). Die Ebene der Kette spannen Wurzel -> Ziel und Wurzel -> Pol auf, das mittlere
//                Gelenk liegt auf der Seite des Pols - so wie Blender den Pol setzt (intern/iksolver/intern/
//                IK_QJacobianSolver.cpp, ConstrainPoleVector: "lookat"-Matrix aus dir = Ziel - Wurzel und up = Pol - Wurzel,
//                blender/blender 67e5a5f8) und wie der HI-Solver von 3ds Max mit Schwenkziel (Doku: das Gelenk zeigt zum
//                Ziel). Fuer zwei Glieder fester Laenge ist die Loesung damit eindeutig; der iterative Loeser von Blender
//                naehert sie an, hier ist sie geschlossen. Ausser Reichweite streckt sich die Kette zum Ziel hin (wie Blender).
//                Das obere Glied dreht so, dass die Beugeachse des Gelenks (die Normale der Ebene aus beiden Gliedern) in die
//                neue Ebene geht; das untere dreht um diese Achse (Scharnier); der Endknochen (Hand, Fuss) nimmt die Drehung
//                des Ziels (wie die IK-Hand von Rigify, deren Endknochen die Lage des IK-Reglers kopiert).
//                Die IK ist eine Aenderung auf der Pose des Bilds wie in Blender: jedes Glied beginnt mit seiner jetzigen
//                (FK-)Lage, der Loeser gibt die Aenderung (iksolver_plugin.cc execute_posetree: basis aus pose_mat ohne
//                rest_basis, IK_SetTransform; Ergebnis basis_change, where_is_ik_bone) - so bleibt die Verdrehung der Glieder
//                um ihre Achse aus der Animation. Der Regler mischt diese Aenderung wie Blender den Einfluss.
// Fussrolle      wie die Rolle des Fersen-Reglers von Rigify (rigify/rigs/limbs/leg.py, rig_roll_mch_bones, blender-addons
//                b42d6862): Rolle < 0 dreht den Fuss um die Ferse (MCH roll2, LIMIT_ROTATION nur negativ), Rolle > 0 um den
//                Ballen (MCH roll1 kopiert die ganze Drehung), davon der Anteil Toe_Roll um die Zehenspitze (roll3,
//                max(0, x * i)); die Zehen bleiben bei der Rolle um den Ballen liegen (der Zehenknochen folgt roll2/roll3,
//                get_mch_heel_toe_output). Ohne Zehenknochen (JKA _humanoid) gibt es keinen Ballen: die Rolle > 0 geht ganz
//                um die Zehenspitze.
// Einrasten      "IK -> FK" (Rigify ik2fk): das Ziel auf den Endknochen der FK-Pose, der Pol in die Ebene der FK-Kette
//                (wie rigify match_pole_target: Mitte der Kette plus Senkrechte - hier genau die Richtung des Gelenks, die
//                geschlossene Loesung braucht keine Suche); "FK -> IK" (Rigify fk2ik): FK-Schluessel der Kette auf die
//                sichtbare IK-Pose. Wie 3ds Max IK/FK-Einrasten (HI-Solver, Bewegung > IK-Solver).
// IK/FK, Raum    je Schluessel einer Spur: IK-Anteil und Raum (Child Of: Welt oder ein Knochen) gelten bis zum naechsten
//                Schluessel (wie ein geschalteter Elternwechsel, 3ds Max Verknuepfungs-Beschraenkung); das Ziel
//                interpoliert Bezier wie die Schicht (Henkel je Lauf gleichen Raums), Pol und Rolle linear.
// Ergebnis       die IK wirkt auf die M der Schicht (rigNachMalAuf, rigBacken): je Glied X'(b) = U X(b) wie ein gedrehter
//                FK-Griff (rigGriffDrehen), auch die mitgedrehten Knochen und der Drehknochen.
#ifndef BHED_MODELL_IK_H
#define BHED_MODELL_IK_H

#include <utility>
#include <vector>

#include "bhed/gla.h"
#include "bhed/modell/rig.h"
#include "bhed/modell/rigdaten.h"

namespace bhed::modell {

// Zwei-Glied-IK: Wurzel a, Gelenk b, Ende c (Weltorte), Ziel, Pol. Gibt das neue Gelenk und Ende; false: ausser
// Reichweite (gestreckt zum Ziel hin) oder keine Ebene (Ziel auf der Linie zum Pol: dann die Ebene der jetzigen Kette).
bool zweiGliedIk(const float a[3], const float b[3], const float c[3], const float ziel[3], const float pol[3], float bNeu[3], float cNeu[3]);

// Die Spur eines Glieds aus dem Rig (Ketten der Griffe Ober-/Unterarm/Hand bzw. -schenkel/Fuss, Zehen, Pol, Fussrolle).
// false: das Rig hat das Glied nicht.
[[nodiscard]] bool ikSpurAnlegen(const GlaAnimation& g, const Rig& r, IkArt art, IkSpur& aus);
[[nodiscard]] int ikSpurIndex(const RigSchicht& s, IkArt art) noexcept;
// Die Spur zum Knochen (Glieder der Kette, Zehen) bzw. -1.
[[nodiscard]] int ikSpurVonKnochen(const RigSchicht& s, int knochen) noexcept;
[[nodiscard]] bool ikArm(IkArt a) noexcept;

// Das ausgewertete Ziel einer Spur am Bild in der Welt.
struct IkZiel {
    BoneMatrix ziel = BoneMatrix::identity();   // Weltlage des Endknochens (starr, ohne Fussrolle)
    float pol[3]{};
    float ik = 0.0F;
    int raum = -1;
    float rolle = 0.0F;
    float spitze = 0.0F;
    float zehen[4] = {1.0F, 0.0F, 0.0F, 0.0F};
    bool da = false;
};
// `welt` ist die Welt des Bilds (fuer Raum und Pol-Raum).
[[nodiscard]] IkZiel ikZielAm(const IkSpur& sp, float bild, const std::vector<BoneMatrix>& welt);
// Der IK-Anteil am Bild (ohne Schluessel 0).
[[nodiscard]] float ikAnteilAm(const IkSpur& sp, float bild) noexcept;

// Die IK aller Spuren mit Anteil > 0 auf die Welt und die M (wie rigNachMalAuf sie gibt, also nach der Bindebasis)
// anwenden. Gibt die geaenderten Knochen. ohneSpur >= 0: diese Spur auslassen (Einrasten liest die FK-Pose).
std::vector<int> ikAnwenden(const GlaAnimation& g, const RigSchicht& s, float bild, std::vector<BoneMatrix>& welt,
                            std::vector<std::pair<int, BoneMatrix>>& mEff, int ohneSpur = -1);

// Die Weltorte der Fussrolle (Ferse, Ballen, Spitze) fuer eine Lage des Fusses und die Weltmatrix der gerollten Lage:
// W = R_Ferse(min(r, 0)) R_Spitze(max(r, 0) s) R_Ballen(max(r, 0) (1 - s)), Ziel' = W Ziel; Zehen: R_Ferse R_Spitze.
struct FussRolle {
    BoneMatrix fuss = BoneMatrix::identity();    // W
    BoneMatrix zehen = BoneMatrix::identity();   // W ohne den Ballen
};
// `form` = starr(X)^-1 X des Fusses (Mass und Scherung der Weltmatrix), damit die Punkte dort liegen, wo man sie sieht.
[[nodiscard]] FussRolle fussRolle(const GlaAnimation& g, const IkSpur& sp, const BoneMatrix& ziel, float rolle, float spitze,
                                  const BoneMatrix& form = BoneMatrix::identity());

// Schluessel einer Spur setzen (ersetzt einen am selben Bild), Henkel je Lauf gleichen Raums neu.
void ikSchluesselSetzen(IkSpur& sp, int bild, const BoneMatrix& zielImRaum, const IkWert& w);
void ikSchluesselLoeschen(IkSpur& sp, int bild);
void ikHenkelNeu(IkSpur& sp);
// Weltlage <-> Lage im Raum (Knochen -1: Welt). Der Raum ist die starre Lage des Knochens (normiertes q, Ort).
[[nodiscard]] BoneMatrix ikImRaum(const std::vector<BoneMatrix>& welt, int raum, const BoneMatrix& weltLage);
[[nodiscard]] BoneMatrix ikAusRaum(const std::vector<BoneMatrix>& welt, int raum, const BoneMatrix& imRaum);
[[nodiscard]] BoneMatrix starr(const BoneMatrix& m) noexcept;   // die Drehung (Spalten nach Gram-Schmidt), Ort bleibt

// Einrasten "IK -> FK": Ziel und Pol der Spur am Bild auf die FK-Pose (Welt ohne diese Spur), Rolle 0; ik = Anteil des
// neuen Schluessels. Raum: der des Schluessels am Bild (oder davor), raum >= -1 setzt einen anderen.
void ikAufFk(const GlaAnimation& g, RigSchicht& s, int spur, int bild, float ik, int raum = -2);
// Dasselbe an jedem Bild von [von, bis] (ein Schluessel je Bild, Henkel einmal am Ende).
void ikAufFkBereich(const GlaAnimation& g, RigSchicht& s, int spur, int von, int bis, float ik);
// Einrasten "FK -> IK": die FK-Schluessel der Kette am Bild auf die sichtbare Pose (mit IK). Gibt die Zahl der Knochen.
int fkAufIk(const GlaAnimation& g, RigSchicht& s, int spur, int bild);
// Dasselbe ueber die Bilder [von, bis]: erst alle ausgewertet, dann geschrieben (Rigify "Snap & Bake").
int fkAufIkBereich(const GlaAnimation& g, RigSchicht& s, int spur, int von, int bis);
// IK/FK wechseln am Bild: einrasten = ohne Sprung (vorher IK -> FK bzw. FK -> IK), sonst nur der Anteil (der Sprung
// bleibt sichtbar - die Pruefung muss ihn finden).
void ikFkWechseln(const GlaAnimation& g, RigSchicht& s, int spur, int bild, float ik, bool einrasten);
// ENT2: IK/FK umschalten AB dem Bild wie das IK/FK-Verhaeltnis eines CAT-Glieds (eine Spur, die bis zum naechsten Schluessel
// gilt): der Anteil wechselt am Bild und an den folgenden Schluesseln, die noch den alten Anteil tragen (bis zum naechsten,
// der schon anders umgeschaltet ist). einrasten = an jedem dieser Bilder vorher die andere Seite angleichen (CAT "Match IK and
// FK", Rigify-Snap): nach IK das Ziel und den Pol auf die FK-Pose, nach FK die FK-Schluessel auf die sichtbare IK-Pose - die
// Pose an diesen Bildern bleibt. Gibt das letzte umgeschaltete Bild.
int ikFkUmschaltenAb(const GlaAnimation& g, RigSchicht& s, int spur, int bild, float ik, bool einrasten);
// ENT2: die FK-Schluessel der Kette einer Spur (Glieder, mitgedrehte Knochen, Drehknochen) an jedem Bild von [von, bis] mit ihrem
// jetzigen Wert - erst alle ausgewertet, dann gesetzt, die Pose bleibt an jedem Bild. Wie die IK-Spur nach "Auf Rig legen" (ein
// Schluessel je Bild, wie ein CAT-Rig nach dem Uebertragen): eine Aenderung an einem Bild (Ziel, Pol, Rolle mit Auto-Einrasten)
// bleibt dort, die Nachbarbilder behalten ihre Pose.
void ikKetteJedesBild(const GlaAnimation& g, RigSchicht& s, int spur, int von, int bis);
// ENT2: die groesste Abweichung (Ort in Einheiten) der Knochen eines Glieds - die Kette ab dem oberen Glied mit allem, was an
// ihr haengt (Hand und Finger, Fuss und Zehen) - zwischen zwei Schichten ueber die Bilder [von, bis]; je Spur von `b`.
[[nodiscard]] std::vector<float> ikGliedAbweichung(const GlaAnimation& g, const RigSchicht& a, const RigSchicht& b, int von, int bis);
// Den Raum der Spur ab dem Bild wechseln, ohne Sprung (Schluessel am Bild davor im alten Raum, am Bild im neuen).
void ikRaumWechseln(const GlaAnimation& g, RigSchicht& s, int spur, int bild, int raum, int erstesBild);
// Das Ziel am Bild in der Welt setzen (Ort und Drehung), Pol und Rolle wie ausgewertet, IK 1; danach die FK-Schluessel
// der Kette auf das Ergebnis (wie Auto-Einrasten von 3ds Max): die Schicht zeigt es sofort.
void ikZielSetzen(const GlaAnimation& g, RigSchicht& s, int spur, int bild, const BoneMatrix& weltZiel);
// Ist das Glied am Bild FK: die Nachbarbilder (in [erstes, letztes]) halten ihre jetzige Pose (FK-Schluessel der Kette und
// IK-Schluessel mit Anteil 0), damit ein IK-Schluessel am Bild nur dort wirkt (Pfad ziehen).
void ikNurAmBild(const GlaAnimation& g, RigSchicht& s, int spur, int bild, int erstes, int letztes);
void ikPolSetzen(const GlaAnimation& g, RigSchicht& s, int spur, int bild, const float weltPol[3]);
void ikRolleSetzen(const GlaAnimation& g, RigSchicht& s, int spur, int bild, float rolle, float spitze);

// Der groesste Unterschied der Weltmatrizen (Drehung und Ort in Einheiten) zweier Schichten am Bild - ein Sprung.
[[nodiscard]] float ikSprung(const GlaAnimation& g, const RigSchicht& a, const RigSchicht& b, int bild);

}  // namespace bhed::modell
#endif
