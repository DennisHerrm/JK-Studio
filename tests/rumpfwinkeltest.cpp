// rumpfwinkeltest - Zeitleiste 6: Rumpf, Hals und Kopf wie CG_G2ClientSpineAngles
// und CG_G2ClientNeckAngles (code/cgame/cg_players.cpp OpenJK, Movie Duels),
// die Knochenwinkel wie G2_Generate_Matrix (G2_bones.cpp) und die
// Motion-Bone-Korrektur (cg_motionBoneComp 2).
#include "bhed/rumpfwinkel.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace bhed;

namespace {

int fehler = 0;

void pruefe(bool ok, const std::string& was) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was.c_str());
    if (!ok) { ++fehler; }
}

bool nah(double a, double b, double tol = 1e-3) { return std::fabs(a - b) <= tol; }

bool spalte(const BoneMatrix& m, int c, float x, float y, float z) {
    return nah(m.m[0][c], x) && nah(m.m[1][c], y) && nah(m.m[2][c], z);
}

// AngleNormalize180 auf 65536 Stufen: der erwartete Wert wie in der Engine
float q(float a) { return (360.0F / 65536) * static_cast<float>(static_cast<int>(a * (65536 / 360.0F)) & 65535); }
float n180(float a) { return q(a) > 180.0F ? q(a) - 360.0F : q(a); }

}  // namespace

int main() {
    std::printf("G2_Generate_Matrix (POSTMULT)\n");
    {
        // Mensch: oben POSITIVE_X, rechts NEGATIVE_Y, vorn NEGATIVE_Z -> newAngles (P, R, Y)
        const float gier[3] = {0.0F, 90.0F, 0.0F};
        const BoneMatrix m = g2WinkelMatrix(gier, G2Achse::PosX, G2Achse::NegY, G2Achse::NegZ);
        // AnglesToAxis(0, 0, 90): vorn (1,0,0), links = -rechts (0,0,1), oben (0,-1,0)
        pruefe(spalte(m, 0, 1, 0, 0) && spalte(m, 1, 0, 0, 1) && spalte(m, 2, 0, -1, 0),
               "Gier 90 dreht um die Knochenachse X (die Wirbelsaeule)");
        const float nick[3] = {10.0F, 0.0F, 0.0F};
        const BoneMatrix n = g2WinkelMatrix(nick, G2Achse::PosX, G2Achse::NegY, G2Achse::NegZ);
        const float s = std::sin(10.0F * 3.14159265F / 180.0F);
        const float c = std::cos(10.0F * 3.14159265F / 180.0F);
        pruefe(spalte(n, 0, c, 0, -s) && spalte(n, 1, 0, 1, 0) && spalte(n, 2, s, 0, c), "Nick 10 dreht um Y");
        const float roll[3] = {0.0F, 0.0F, 90.0F};
        const BoneMatrix r = g2WinkelMatrix(roll, G2Achse::PosX, G2Achse::NegY, G2Achse::NegZ);
        // AnglesToAxis(0, 90, 0): vorn (0,1,0), links (-1,0,0), oben (0,0,1)
        pruefe(spalte(r, 0, 0, 1, 0) && spalte(r, 1, -1, 0, 0) && spalte(r, 2, 0, 0, 1), "Roll 90 dreht um Z");
        // Die Lider (POSITIVE_Y, POSITIVE_Z, POSITIVE_X) mit (0, YAW, 0): newAngles (YAW, 0, 0)
        const float lid[3] = {0.0F, 10.0F, 0.0F};
        pruefe(spalte(g2WinkelMatrix(lid, G2Achse::PosY, G2Achse::PosZ, G2Achse::PosX), 0, c, 0, -s), "Lider: YAW als Nick");
        // Rancor (NEGATIVE_X, POSITIVE_Y, POSITIVE_Z): je +180 - bei 0 Grad die Einheit
        const float null[3] = {0.0F, 0.0F, 0.0F};
        const BoneMatrix e = g2WinkelMatrix(null, G2Achse::NegX, G2Achse::PosY, G2Achse::PosZ);
        pruefe(spalte(e, 0, 1, 0, 0) && spalte(e, 1, 0, 1, 0) && spalte(e, 2, 0, 0, 1), "Rancor: 180/180/180 ist die Einheit");
    }

    std::printf("Motion-Bolzen\n");
    {
        // die Wurzel der Engine (identityMatrix in G2API_GetBoltMatrix): keine Korrektur
        const float wurzel[3][4] = {{0, -1, 0, 0}, {1, 0, 0, 0}, {0, 0, 1, 0}};
        float w[3];
        motionWinkelAusBolzen(wurzel, w);
        pruefe(nah(w[0], 0) && nah(w[1], 0) && nah(w[2], 0), "Grundstellung: 0, 0, 0");
        // BOTH_WALK2 (_humanoid.gla): NEGATIVE_Y 60 Grad nach oben -> Nick -60
        const float c = 0.5F;
        const float s = std::sqrt(3.0F) * 0.5F;
        const float hoch[3][4] = {{0, -c, 0, 0}, {1, 0, 0, 0}, {0, -s, 1, 0}};
        motionWinkelAusBolzen(hoch, w);
        pruefe(nah(w[0], -60.0F, 1e-2) && nah(w[1], 0.0F), "nach oben: Nick -60");
        // NEGATIVE_X nach unten geneigt -> Roll
        const float seit[3][4] = {{0, -1, 0, 0}, {c, 0, 0, 0}, {s, 0, 1, 0}};
        motionWinkelAusBolzen(seit, w);
        // motionAngles[ROLL] = -tempAng[PITCH] = 300, AngleNormalize180 macht -60 daraus
        pruefe(nah(n180(w[2]), -60.0F, 1e-2), "rechts nach unten: Roll -60");
    }

    std::printf("Klassen und Bedingungen\n");
    {
        pruefe(rumpfKlasseFuer("CLASS_DROIDEKA", true) == RumpfKlasse::Droideka, "CLASS_DROIDEKA in Movie Duels");
        pruefe(rumpfKlasseFuer("CLASS_DROIDEKA", false) == RumpfKlasse::Mensch, "OpenJK kennt CLASS_DROIDEKA nicht");
        pruefe(rumpfKlasseFuer("CLASS_R2D2", false) == RumpfKlasse::Ohne, "R2D2 ohne Rumpf und Hals");
        pruefe(rumpfKlasseFuer("CLASS_STORMTROOPER", false) == RumpfKlasse::Mensch, "Sturmtruppler: drei Knochen");
        pruefe(rumpfSchlechteKnochen(RumpfKlasse::Wampa) && !rumpfSchlechteKnochen(RumpfKlasse::Mensch), "G_ClassHasBadBones");
        pruefe(rumpfMotionAusgleich(RumpfKlasse::Mensch, "BOTH_WALK2", "BOTH_STAND1", false), "Beine und Oberkoerper verschieden");
        pruefe(!rumpfMotionAusgleich(RumpfKlasse::Mensch, "BOTH_WALK2", "BOTH_WALK2", false), "gleiche Animation: keine Korrektur");
        pruefe(!rumpfMotionAusgleich(RumpfKlasse::Mensch, "BOTH_FLIP_F", "BOTH_STAND1", false), "PM_FlippingAnim");
        pruefe(rumpfMotionAusgleich(RumpfKlasse::Mensch, "BOTH_FLIP_F_ANI", "BOTH_STAND1", false) &&
                   !rumpfMotionAusgleich(RumpfKlasse::Mensch, "BOTH_FLIP_F_ANI", "BOTH_STAND1", true),
               "BOTH_FLIP_F_ANI nur in Movie Duels ein Salto");
        pruefe(!rumpfMotionAusgleich(RumpfKlasse::Mensch, "BOTH_STAND1", "BOTH_T1_BR_BL", false), "PM_SpinningSaberAnim am Oberkoerper");
        pruefe(!rumpfMotionAusgleich(RumpfKlasse::HazardTrooper, "BOTH_WALK2", "BOTH_STAND1", false), "schlechte Knochen: keine Korrektur");
        pruefe(rumpfAnDerKante("BOTH_LEDGE_HOLD", true) && !rumpfAnDerKante("BOTH_LEDGE_HOLD", false), "PM_InLedgeMove nur MD");
        G2Achse o;
        G2Achse r;
        G2Achse v;
        rumpfKnochenAchsen(RumpfKlasse::Mensch, "lower_lumbar", o, r, v);
        pruefe(o == G2Achse::PosX && r == G2Achse::NegY && v == G2Achse::NegZ, "Mensch: POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z");
        rumpfKnochenAchsen(RumpfKlasse::AssassinDroid, "upper_lumbar", o, r, v);
        pruefe(o == G2Achse::NegX && r == G2Achse::PosY && v == G2Achse::PosZ, "Assassin Droid upper_lumbar");
        rumpfKnochenAchsen(RumpfKlasse::AssassinDroid, "thoracic", o, r, v);
        pruefe(o == G2Achse::PosX && r == G2Achse::NegY && v == G2Achse::NegZ, "Assassin Droid thoracic wie der Mensch");
        rumpfKnochenAchsen(RumpfKlasse::HazardTrooper, "pelvis", o, r, v);
        pruefe(o == G2Achse::PosZ && r == G2Achse::NegX && v == G2Achse::NegY, "Hazard Trooper pelvis");
    }

    std::printf("CG_G2ClientSpineAngles / CG_G2ClientNeckAngles\n");
    {
        RumpfEingabe e;
        e.motion = true;
        e.motionWinkel[0] = -60.2F;   // BOTH_WALK2
        RumpfWinkel w = rumpfWinkel(e);
        const float v = q(60.2F);
        pruefe(w.lumbarSetzen && w.thoracicSetzen && w.halsSetzen, "Mensch: alle fuenf Knochen gesetzt");
        pruefe(nah(w.lower[0], v * 0.4F) && nah(w.upper[0], v * 0.4F), "lower/upper_lumbar je 0,4 des Nicks");
        pruefe(nah(w.thoracic[0], v * 0.2F * 0.5F), "thoracic 0,2 - und vom Hals zur Haelfte gemischt");
        pruefe(nah(w.cervical[0], 0) && nah(w.cranium[0], 0), "ohne Blick bleiben Hals und Kopf");

        RumpfEingabe g;
        g.blick[1] = 20.0F;   // Rumpf gegen die Beine (Aufnahme)
        w = rumpfWinkel(g);
        pruefe(nah(w.lower[1], q(20.0F) * 0.45F) && nah(w.upper[1], q(20.0F) * 0.35F) && nah(w.thoracic[1], q(20.0F) * 0.2F * 0.5F),
               "Gier 0,45 / 0,35 / 0,20 (halb)");

        RumpfEingabe k;
        k.kopf[1] = 30.0F;
        w = rumpfWinkel(k);
        pruefe(nah(w.thoracic[1], 3.0F) && nah(w.cervical[1], 9.0F) && nah(w.cranium[1], 18.0F), "Blick 0,1 / 0,3 / 0,6");
        k.kopf[1] = 80.0F;
        w = rumpfWinkel(k);
        pruefe(nah(w.cranium[1], 30.0F), "Blick gekappt auf 50 (headClampMaxAngles)");
        k.kopf[1] = -80.0F;
        w = rumpfWinkel(k);
        pruefe(nah(w.cranium[1], -33.0F), "und auf -55");

        RumpfEingabe beide = g;
        beide.kopf[1] = 30.0F;
        w = rumpfWinkel(beide);
        pruefe(nah(w.thoracic[1], (q(20.0F) * 0.2F + 3.0F) * 0.5F), "thoracic: (Rumpf + Blick) / 2");

        RumpfEingabe h = e;
        h.klasse = RumpfKlasse::HazardTrooper;
        h.motion = false;
        h.blick[1] = 20.0F;
        w = rumpfWinkel(h);
        pruefe(w.lumbarSetzen && !w.thoracicSetzen && !w.halsSetzen && nah(w.lower[1], q(20.0F) * 0.55F), "Hazard Trooper");

        RumpfEingabe a;
        a.klasse = RumpfKlasse::AssassinDroid;
        a.blick[1] = 20.0F;
        a.kopf[1] = 10.0F;
        w = rumpfWinkel(a);
        pruefe(nah(w.upper[1], q(20.0F)) && nah(w.lower[1], 0) && nah(w.thoracic[1], 0) && nah(w.cranium[1], 10.0F),
               "Assassin Droid: je Knochen eine Achse");

        RumpfEingabe sd;
        sd.klasse = RumpfKlasse::SaberDroid;
        sd.kopf[1] = 10.0F;
        w = rumpfWinkel(sd);
        pruefe(!w.lumbarSetzen && !w.thoracicSetzen && w.halsSetzen && nah(w.cervical[1], 10.0F) && nah(w.cranium[1], 0),
               "Saber Droid: kein Rumpf, Hals giert");

        RumpfEingabe rc;
        rc.klasse = RumpfKlasse::Rancor;
        rc.blick[1] = 20.0F;
        w = rumpfWinkel(rc);
        pruefe(nah(w.lower[1], 0) && nah(w.upper[1], 0) && !w.halsSetzen && nah(w.thoracic[1], 0), "Rancor: keine Gier am Rumpf");

        RumpfEingabe md = e;
        md.kante = true;
        md.kopf[1] = 30.0F;
        w = rumpfWinkel(md);
        pruefe(nah(w.lower[0], 0) && nah(w.thoracic[0], 0) && nah(w.cranium[1], 0) && nah(w.thoracic[1], 3.0F),
               "MD an der Kante: kein Nick, Kopf und Hals fest");

        RumpfEingabe ohne = e;
        ohne.klasse = RumpfKlasse::Ohne;
        w = rumpfWinkel(ohne);
        pruefe(!w.lumbarSetzen && !w.thoracicSetzen && !w.halsSetzen, "GONK & Co.: nichts");
    }

    std::printf("Anlegen\n");
    {
        GlaAnimation anim;
        const char* namen[] = {"pelvis", "lower_lumbar", "upper_lumbar", "thoracic", "cervical", "cranium"};
        std::vector<BoneMatrix> welt;
        for (int i = 0; i < 6; ++i) {
            GlaBone b;
            b.name = namen[i];
            b.parent = i - 1;
            anim.bones.push_back(b);
            BoneMatrix m = BoneMatrix::identity();
            m.m[2][3] = 30.0F + 8.0F * static_cast<float>(i);
            welt.push_back(m);
        }
        RumpfWinkel w;
        w.lumbarSetzen = true;
        w.lower[0] = 90.0F;   // Nick am lower_lumbar: der ganze Oberkoerper kippt um dessen Y
        rumpfWinkelAnlegen(anim, welt, RumpfKlasse::Mensch, w);
        // Knochen-Z (Hoehe 8 je Wirbel) wird zu Knochen-X: der cranium liegt 32 vor dem lower_lumbar
        pruefe(nah(welt[5].m[0][3], 32.0F, 1e-2) && nah(welt[5].m[2][3], 38.0F, 1e-2), "der Teilbaum kippt mit");
        pruefe(nah(welt[0].m[2][3], 30.0F) && nah(welt[0].m[0][0], 1.0F), "das Becken bleibt");
    }

    std::printf("%s (%d Fehler)\n", fehler == 0 ? "alles gut" : "FEHLER", fehler);
    return fehler == 0 ? 0 : 1;
}
