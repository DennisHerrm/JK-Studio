// modell_ik.cpp - Modell-Block Stufe 4: IK/FK (bhed/modell/ik.h)
//
//   Zwei-Glied-IK  Ziel erreicht (unter 0,001), Gelenk in der Ebene Wurzel/Ziel/Pol auf der Seite des Pols, die
//                  Gliedlaengen bleiben; ausser Reichweite gestreckt zum Ziel hin
//   Kette          an JKA _humanoid (Hierarchie) und JK2 _humanoid (flach unter thoracic/pelvis, Mitnehmen): Hand und Fuss
//                  am Ziel, Gelenk in der Pol-Ebene, Laengen gleich
//   Fussrolle      Rolle < 0: die Ferse bleibt liegen; > 0: der Ballen, die Zehen behalten ihre Lage (JK2 ltarsal); Anteil
//                  Spitze 1: die Zehenspitze bleibt liegen
//   Einrasten      an jedem Bild IK -> FK (Ziel und Pol aus der FK-Pose) und zurueck: gebacken jedes Knochen-Bild innerhalb
//                  der Grenze aus compressBone (Stufe 3); FK -> IK (FK-Schluessel aus der IK-Pose) ebenso
//   Wechsel        ohne Einrasten muss der Sprung messbar sein (rot), mit Einrasten keiner
//   Raum           Space Switching: kein Sprung am Wechsel, danach folgt die Hand der anderen Hand starr
//   Reduzieren     nur ein Knochen bzw. eine Spanne (aus 3b offen): der Rest bleibt, gebacken innerhalb der Grenze
//   Posen-Datei    Text hin und zurueck gleich
//
// Aufruf: modelliktest
#include "bhed/gla.h"
#include "bhed/modell/animkurven.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/ik.h"
#include "bhed/modell/rig.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "rig_handtabelle.inc"

namespace {
int fails = 0;
void expect(const std::string& what, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", what.c_str());
    if (!ok) { ++fails; }
}

using namespace bhed;
using namespace bhed::modell;

const RigHandSkelett& tabelle(const char* name) {
    for (const RigHandSkelett& s : kRigHandSkelette) {
        if (std::strcmp(s.name, name) == 0) { return s; }
    }
    return kRigHandSkelette[0];
}

GlaAnimation skelett(const RigHandSkelett& h, int bilder) {
    GlaAnimation g;
    g.name = h.name;
    g.skeletonName = h.name;
    g.scale = 0.64F;
    for (int i = 0; i < h.zahl; ++i) {
        GlaBone b;
        b.name = h.knochen[i].name;
        b.parent = h.knochen[i].eltern;
        b.basePose = BoneMatrix::identity();
        b.basePose.m[0][3] = h.knochen[i].x;
        b.basePose.m[1][3] = h.knochen[i].y;
        b.basePose.m[2][3] = h.knochen[i].z;
        b.basePoseInv = BoneMatrix::identity();
        b.basePoseInv.m[0][3] = -h.knochen[i].x;
        b.basePoseInv.m[1][3] = -h.knochen[i].y;
        b.basePoseInv.m[2][3] = -h.knochen[i].z;
        g.bones.push_back(b);
    }
    for (int i = 0; i < h.zahl; ++i) {
        if (g.bones[static_cast<std::size_t>(i)].parent >= 0) { g.bones[static_cast<std::size_t>(g.bones[static_cast<std::size_t>(i)].parent)].kinder.push_back(i); }
    }
    const std::uint16_t e[7] = {3 * 16383, 2 * 16383, 2 * 16383, 2 * 16383, 512 * 64, 512 * 64, 512 * 64};
    for (const std::uint16_t v : e) {
        g.bonePool.push_back(static_cast<std::uint8_t>(v & 0xFF));
        g.bonePool.push_back(static_cast<std::uint8_t>(v >> 8));
    }
    g.numFrames = bilder;
    g.indexes.assign(static_cast<std::size_t>(bilder) * g.bones.size(), 0U);
    return g;
}

// Dasselbe Skelett mit einer Drehung in jeder Grundstellung (die GLA der Spiele haben gedrehte basePose; die Bindepose
// bleibt dieselbe: X(b) = B(b) ohne Animation)
// Gedreht und gestreckt (Mass 0,64 in der Grundstellung): Weltmatrizen, die keine reine Drehung sind - im Programm stand der
// Drehpunkt der Fussrolle des Sturmtrupplers so 2,5 Einheiten neben dem, den matrixToQuat allein ergab (mb6_ui6). Eine
// Scherung bleibt aussen vor: die Schluessel der Schicht sind Drehungen (Stufe 3), eine Scherung im M halten sie nicht.
GlaAnimation gestreckt(GlaAnimation g) {
    for (GlaBone& b : g.bones) {
        BoneMatrix f = BoneMatrix::identity();
        f.m[0][0] = 0.64F;
        f.m[1][1] = 0.64F;
        f.m[2][2] = 0.64F;
        const float o[3] = {b.basePose.m[0][3], b.basePose.m[1][3], b.basePose.m[2][3]};
        b.basePose = b.basePose * f;
        for (int k = 0; k < 3; ++k) { b.basePose.m[k][3] = o[k]; }
        b.basePoseInv = starrInvers(b.basePose);
    }
    return g;
}

GlaAnimation gedreht(GlaAnimation g) {
    std::uint32_t zufall = 99U;
    const auto naechste = [&]() {
        zufall = zufall * 1664525U + 1013904223U;
        return static_cast<float>(zufall >> 8) / static_cast<float>(1U << 24);
    };
    for (GlaBone& b : g.bones) {
        const float achse[3] = {naechste() - 0.5F, naechste() - 0.5F, naechste() - 0.5F};
        BoneMatrix r = drehungUm(achse, (naechste() - 0.5F) * 160.0F);
        r.m[0][3] = b.basePose.m[0][3];
        r.m[1][3] = b.basePose.m[1][3];
        r.m[2][3] = b.basePose.m[2][3];
        b.basePose = r;
        b.basePoseInv = starrInvers(r);
    }
    return g;
}

int knochen(const GlaAnimation& g, const char* n) {
    for (std::size_t i = 0; i < g.bones.size(); ++i) {
        if (g.bones[i].name == n) { return static_cast<int>(i); }
    }
    return -1;
}

// Eine bewegte GLA: je Knochen lineare Schluessel bei 0, 20, 40, 59 (Drehung bis 35 Grad), gebacken wie das Spiel sie liest
GlaAnimation bewegt(const GlaAnimation& g, std::uint32_t zufall) {
    RigSchicht qs;
    const auto naechste = [&]() {
        zufall = zufall * 1664525U + 1013904223U;
        return static_cast<float>(zufall >> 8) / static_cast<float>(1U << 24);
    };
    for (std::size_t b = 0; b < g.bones.size(); ++b) {
        for (const int f : {0, 20, 40, 59}) {
            const float achse[3] = {naechste() - 0.5F, naechste() - 0.5F, naechste() - 0.5F};
            BoneMatrix m = drehungUm(achse, (naechste() - 0.5F) * 35.0F);
            rigSchluesselSetzen(qs, static_cast<int>(b), f, m);
        }
    }
    GlaAnimation quelle;
    BackBericht bb;
    (void)rigBacken(g, qs, 0, g.numFrames, true, "quelle", quelle, &bb);
    return quelle;
}

float ab(const float a[3], const float b[3]) { return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2])); }
void ortVon(const BoneMatrix& m, float p[3]) {
    p[0] = m.m[0][3];
    p[1] = m.m[1][3];
    p[2] = m.m[2][3];
}

// Gebacken: wie viele Knochen-Bilder liegen ueber der Grenze (Drehung > Grenze aus compressBone oder mehr als eine Stufe
// Verschiebung), und der groesste Anteil
struct Vergleich {
    int ueber = 0;
    double anteil = 0.0;
    int gleich = 0;
    int alle = 0;
};
Vergleich backenVergleich(const GlaAnimation& g, const RigSchicht& a, const RigSchicht& b) {
    Vergleich v;
    GlaAnimation ga;
    GlaAnimation gb;
    BackBericht x;
    if (!rigBacken(g, a, 0, g.numFrames, false, "a", ga, &x) || !rigBacken(g, b, 0, g.numFrames, false, "b", gb, &x)) {
        v.ueber = -1;
        return v;
    }
    for (std::size_t at = 0; at < ga.indexes.size(); ++at) {
        const std::uint8_t* p = ga.bonePool.data() + static_cast<std::size_t>(ga.indexes[at]) * 14U;
        const std::uint8_t* q = gb.bonePool.data() + static_cast<std::size_t>(gb.indexes[at]) * 14U;
        const double an = drehAbstand(p, q) / kompressionsGrenze(p);
        v.anteil = std::max(v.anteil, an);
        v.ueber += (an > 1.0 || verschiebungsStufen(p, q) > 1) ? 1 : 0;
        v.gleich += std::memcmp(p, q, 14U) == 0 ? 1 : 0;
        ++v.alle;
    }
    return v;
}

// Kette pruefen: Endknochen am Ziel, Gelenk in der Pol-Ebene, Laengen gleich (gegen die FK-Welt vorher)
struct KettenPruefung {
    float amZiel = 0.0F;
    float ausEbene = 0.0F;
    float laenge = 0.0F;
    bool polSeite = true;
};
KettenPruefung kettePruefen(const IkSpur& sp, const std::vector<BoneMatrix>& vorher, const std::vector<BoneMatrix>& nachher, const IkZiel& z) {
    KettenPruefung k;
    float a0[3], b0[3], c0[3], a[3], b[3], c[3];
    ortVon(vorher[static_cast<std::size_t>(sp.oben.knochen)], a0);
    ortVon(vorher[static_cast<std::size_t>(sp.unten.knochen)], b0);
    ortVon(vorher[static_cast<std::size_t>(sp.ende.knochen)], c0);
    ortVon(nachher[static_cast<std::size_t>(sp.oben.knochen)], a);
    ortVon(nachher[static_cast<std::size_t>(sp.unten.knochen)], b);
    ortVon(nachher[static_cast<std::size_t>(sp.ende.knochen)], c);
    float t[3];
    ortVon(z.ziel, t);
    k.amZiel = ab(c, t);
    k.laenge = std::max(std::fabs(ab(a, b) - ab(a0, b0)), std::fabs(ab(b, c) - ab(b0, c0)));
    // Normale der Ebene (Ziel - Wurzel) x (Pol - Wurzel)
    const float u[3] = {t[0] - a[0], t[1] - a[1], t[2] - a[2]};
    const float p[3] = {z.pol[0] - a[0], z.pol[1] - a[1], z.pol[2] - a[2]};
    float n[3] = {u[1] * p[2] - u[2] * p[1], u[2] * p[0] - u[0] * p[2], u[0] * p[1] - u[1] * p[0]};
    const float ln = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    for (float& x : n) { x /= std::max(ln, 1e-9F); }
    const float e[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    k.ausEbene = std::fabs(e[0] * n[0] + e[1] * n[1] + e[2] * n[2]);
    // Seite: das Gelenk und der Pol liegen auf derselben Seite der Linie Wurzel -> Ziel
    const float lu = std::sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
    const auto senk = [&](const float v[3], float o[3]) {
        const float d = (v[0] * u[0] + v[1] * u[1] + v[2] * u[2]) / std::max(lu * lu, 1e-9F);
        for (int i = 0; i < 3; ++i) { o[i] = v[i] - u[i] * d; }
    };
    float se[3], sp2[3];
    senk(e, se);
    senk(p, sp2);
    k.polSeite = se[0] * sp2[0] + se[1] * sp2[1] + se[2] * sp2[2] > 0.0F;
    return k;
}

void kettenTest(const char* name) {
    const RigHandSkelett& h = tabelle(name);
    const GlaAnimation g0 = skelett(h, 60);
    const GlaAnimation g = bewegt(g0, 4711U);
    const Inhalt leer;
    const Rig rig = rigAnlegen(g, leer, 0, {});
    std::printf("  --- %s (%d Knochen) ---\n", name, h.zahl);
    for (int a = 0; a < kIkArten; ++a) {
        const IkArt art = static_cast<IkArt>(a);
        IkSpur sp;
        const bool ok = ikSpurAnlegen(g, rig, art, sp);
        static const char* const kName[4] = {"Arm L", "Arm R", "Bein L", "Bein R"};
        expect(std::string(name) + " " + kName[a] + ": Spur aus dem Rig (Ober-, Unter-, Endglied)", ok);
        if (!ok) { continue; }
        // --- Einrasten an jedem Bild: IK -> FK, dann ausgewertet gegen die FK-Pose ---------------------------------------
        RigSchicht fk;
        RigSchicht ik;
        ik.ik.push_back(sp);
        for (int f = 0; f < g.numFrames; ++f) { ikAufFk(g, ik, 0, f, 1.0F); }
        const Vergleich v1 = backenVergleich(g, fk, ik);
        std::printf("    %s %s: IK -> FK an %d Bildern, gebacken: %d / %d Knochen-Bilder Byte fuer Byte, groesster Anteil %.4f, ueber %d\n", name, kName[a],
                    g.numFrames, v1.gleich, v1.alle, v1.anteil, v1.ueber);
        expect(std::string(name) + " " + kName[a] + ": IK -> FK an jedem Bild, IK zeigt dieselbe Pose (gebacken innerhalb der Grenze)", v1.ueber == 0 && v1.alle > 0);
        // --- das Ziel bewegen: Hand/Fuss am Ziel, Gelenk in der Pol-Ebene, Laengen gleich ------------------------------
        float schlimmZiel = 0.0F;
        float schlimmEbene = 0.0F;
        float schlimmLaenge = 0.0F;
        bool seite = true;
        // eine duenne Schicht: IK-Schluessel an Bild 0 und 59 aus der FK-Pose, dazu bewegte Ziele an 5, 30, 52
        RigSchicht gez;
        gez.ik.push_back(sp);
        ikAufFk(g, gez, 0, 0, 1.0F);
        ikAufFk(g, gez, 0, g.numFrames - 1, 1.0F);
        for (const int f : {5, 30, 52}) {
            std::vector<BoneMatrix> w0;
            rigWelt(g, gez, f, f, 0.0F, w0);
            BoneMatrix z = starr(w0[static_cast<std::size_t>(sp.ende.knochen)]);
            // in Reichweite: ein Fuenftel zur Wurzel hin und etwas zur Seite
            float a0[3];
            ortVon(w0[static_cast<std::size_t>(sp.oben.knochen)], a0);
            const float seitlich[3] = {0.6F, -0.4F, 0.3F};
            for (int k = 0; k < 3; ++k) { z.m[k][3] += (a0[k] - z.m[k][3]) * 0.2F + seitlich[k]; }
            ikZielSetzen(g, gez, 0, f, z);
            std::vector<BoneMatrix> w1;
            rigWelt(g, gez, f, f, 0.0F, w1);
            const IkZiel zz = ikZielAm(gez.ik[0], static_cast<float>(f), w1);
            const KettenPruefung k = kettePruefen(gez.ik[0], w0, w1, zz);
            schlimmZiel = std::max(schlimmZiel, k.amZiel);
            schlimmEbene = std::max(schlimmEbene, k.ausEbene);
            schlimmLaenge = std::max(schlimmLaenge, k.laenge);
            seite = seite && k.polSeite;
        }
        std::printf("    %s %s: Ziel bewegt: Abstand zum Ziel %.6f, aus der Pol-Ebene %.6f, Laenge %.6f\n", name, kName[a], static_cast<double>(schlimmZiel),
                    static_cast<double>(schlimmEbene), static_cast<double>(schlimmLaenge));
        expect(std::string(name) + " " + kName[a] + ": Endknochen am Ziel (< 0,001)", schlimmZiel < 1e-3F);
        expect(std::string(name) + " " + kName[a] + ": Gelenk in der Ebene Wurzel/Ziel/Pol (< 0,001), auf der Seite des Pols", schlimmEbene < 1e-3F && seite);
        expect(std::string(name) + " " + kName[a] + ": Gliedlaengen bleiben (< 0,001)", schlimmLaenge < 1e-3F);
        // die FK-Schluessel am Bild zeigen das Ergebnis sofort (Auto-Einrasten): IK aus = dieselbe Pose an diesem Bild
        {
            RigSchicht aus = gez;
            aus.ik.clear();
            float d = 0.0F;
            for (const int f : {5, 30, 52}) { d = std::max(d, ikSprung(g, gez, aus, f)); }
            expect(std::string(name) + " " + kName[a] + ": FK-Schluessel am Bild = IK-Pose (Auto-Einrasten, < 0,001)", d < 1e-3F);
        }
        // --- FK -> IK an jedem Bild und IK aus: dieselbe Animation gebacken -----------------------------------------------
        {
            RigSchicht fkSnap = gez;
            (void)fkAufIkBereich(g, fkSnap, 0, 0, g.numFrames - 1);
            fkSnap.ik.clear();
            const Vergleich v2 = backenVergleich(g, gez, fkSnap);
            std::printf("    %s %s: FK -> IK an %d Bildern, IK aus: groesster Anteil %.4f, ueber %d, Byte fuer Byte %d / %d\n", name, kName[a], g.numFrames, v2.anteil,
                        v2.ueber, v2.gleich, v2.alle);
            expect(std::string(name) + " " + kName[a] + ": FK -> IK an jedem Bild, ohne IK dieselbe Animation (innerhalb der Grenze)", v2.ueber == 0 && v2.alle > 0);
        }
        // --- Wechsel ohne / mit Einrasten ---------------------------------------------------------------------------
        {
            RigSchicht ohne = gez;
            ikFkWechseln(g, ohne, 0, 30, 0.0F, false);
            RigSchicht mit = gez;
            ikFkWechseln(g, mit, 0, 30, 0.0F, true);
            const float so = ikSprung(g, gez, ohne, 30);
            const float sm = ikSprung(g, gez, mit, 30);
            std::printf("    %s %s: IK -> FK am Bild 30: Sprung ohne Einrasten %.4f, mit %.6f\n", name, kName[a], static_cast<double>(so), static_cast<double>(sm));
            // am Bild 30 liegt ein IK-Schluessel mit FK-Schluesseln (Auto-Einrasten) - an Bild 40 nicht: dort springt es
            RigSchicht ohne40 = gez;
            ikFkWechseln(g, ohne40, 0, 40, 0.0F, false);
            RigSchicht mit40 = gez;
            ikFkWechseln(g, mit40, 0, 40, 0.0F, true);
            const float so40 = ikSprung(g, gez, ohne40, 40);
            const float sm40 = ikSprung(g, gez, mit40, 40);
            std::printf("    %s %s: IK -> FK am Bild 40: Sprung ohne Einrasten %.4f, mit %.6f\n", name, kName[a], static_cast<double>(so40), static_cast<double>(sm40));
            expect(std::string(name) + " " + kName[a] + ": rot - Wechsel IK -> FK ohne Einrasten zwischen Schluesseln springt (> 0,05)", so40 > 0.05F);
            expect(std::string(name) + " " + kName[a] + ": Wechsel IK -> FK mit Einrasten ohne Sprung (< 0,001)", sm < 1e-3F && sm40 < 1e-3F);
            // FK -> IK: ab Bild 10 FK (eingerastet), an Bild 20 das obere Glied in FK um 25 Grad gedreht; dort wieder IK
            RigSchicht basis = gez;
            ikFkWechseln(g, basis, 0, 10, 0.0F, true);
            {
                BoneMatrix m = BoneMatrix::identity();
                (void)rigWert(basis, sp.oben.knochen, 20.0F, m);
                const float ax[3] = {0.3F, 1.0F, 0.2F};
                rigSchluesselSetzen(basis, sp.oben.knochen, 20, m * drehungUm(ax, 25.0F));
            }
            RigSchicht o2 = basis;
            ikFkWechseln(g, o2, 0, 20, 1.0F, false);
            RigSchicht m2 = basis;
            ikFkWechseln(g, m2, 0, 20, 1.0F, true);
            const float so2 = ikSprung(g, basis, o2, 20);
            const float sm2 = ikSprung(g, basis, m2, 20);
            std::printf("    %s %s: FK -> IK am Bild 20: Sprung ohne Einrasten %.4f, mit %.6f\n", name, kName[a], static_cast<double>(so2), static_cast<double>(sm2));
            expect(std::string(name) + " " + kName[a] + ": rot - Wechsel FK -> IK ohne Einrasten springt (> 0,05)", so2 > 0.05F);
            expect(std::string(name) + " " + kName[a] + ": Wechsel FK -> IK mit Einrasten ohne Sprung (< 0,001)", sm2 < 1e-3F);
        }
    }
}

}  // namespace

int main() {
    // --- Zwei-Glied-IK allein -----------------------------------------------------------------------------------------
    {
        const float a[3] = {0.0F, 0.0F, 0.0F};
        const float b[3] = {0.0F, 0.0F, -10.0F};
        const float c[3] = {0.0F, 3.0F, -19.5F};
        const float ziel[3] = {4.0F, 6.0F, -12.0F};
        const float pol[3] = {0.0F, -20.0F, -8.0F};
        float bn[3];
        float cn[3];
        const bool ok = zweiGliedIk(a, b, c, ziel, pol, bn, cn);
        const float l1 = ab(a, b);
        const float l2 = ab(b, c);
        const float u[3] = {ziel[0], ziel[1], ziel[2]};
        float n[3] = {u[1] * pol[2] - u[2] * pol[1], u[2] * pol[0] - u[0] * pol[2], u[0] * pol[1] - u[1] * pol[0]};
        const float ln = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        const float aus = std::fabs((bn[0] * n[0] + bn[1] * n[1] + bn[2] * n[2]) / ln);
        std::printf("    Zwei-Glied: Ende %.6f vom Ziel, Laengen %.6f / %.6f (vorher %.6f / %.6f), aus der Ebene %.6f\n", static_cast<double>(ab(cn, ziel)),
                    static_cast<double>(ab(a, bn)), static_cast<double>(ab(bn, cn)), static_cast<double>(l1), static_cast<double>(l2), static_cast<double>(aus));
        expect("Zwei-Glied: Ziel erreicht", ok && ab(cn, ziel) < 1e-4F);
        expect("Zwei-Glied: Laengen bleiben", std::fabs(ab(a, bn) - l1) < 1e-4F && std::fabs(ab(bn, cn) - l2) < 1e-4F);
        expect("Zwei-Glied: Gelenk in der Ebene Wurzel/Ziel/Pol (ConstrainPoleVector)", aus < 1e-4F);
        // Seite des Pols: das Gelenk liegt wie der Pol (y < 0) vor der Linie
        const float d = (bn[0] * u[0] + bn[1] * u[1] + bn[2] * u[2]) / (u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
        const float s[3] = {bn[0] - u[0] * d, bn[1] - u[1] * d, bn[2] - u[2] * d};
        const float dp = (pol[0] * u[0] + pol[1] * u[1] + pol[2] * u[2]) / (u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
        const float sp[3] = {pol[0] - u[0] * dp, pol[1] - u[1] * dp, pol[2] - u[2] * dp};
        expect("Zwei-Glied: Gelenk auf der Seite des Pols", s[0] * sp[0] + s[1] * sp[1] + s[2] * sp[2] > 0.0F);
        // ausser Reichweite: gestreckt zum Ziel
        const float weit[3] = {0.0F, 30.0F, -30.0F};
        const bool ok2 = zweiGliedIk(a, b, c, weit, pol, bn, cn);
        const float lw = ab(a, weit);
        const float rich = (cn[0] * weit[0] + cn[1] * weit[1] + cn[2] * weit[2]) / (ab(a, cn) * lw);
        expect("Zwei-Glied ausser Reichweite: false, gestreckt (Ende bei l1 + l2 auf der Linie zum Ziel)", !ok2 && std::fabs(ab(a, cn) - (l1 + l2)) < 1e-3F && rich > 0.99999F);
    }
    // --- starr: die Drehung einer gestreckten Matrix, mitdrehend (starr(U X) = U starr(X)) ----------------------------------
    {
        const float a1[3] = {0.3F, -0.8F, 0.5F};
        const float a2[3] = {-0.6F, 0.2F, 0.7F};
        BoneMatrix x = drehungUm(a1, 63.0F);
        BoneMatrix f = BoneMatrix::identity();
        f.m[0][0] = 0.64F;
        f.m[1][1] = 0.64F;
        f.m[2][2] = 0.64F;
        f.m[1][2] = 0.05F;
        x = x * f;
        const BoneMatrix u = drehungUm(a2, 41.0F);
        const BoneMatrix l = starr(u * x);
        const BoneMatrix r = u * starr(x);
        float d = 0.0F;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) { d = std::max(d, std::fabs(l.m[i][j] - r.m[i][j])); }
        }
        // die alte Rechnung ueber das Quaternion allein (matrixToQuat erwartet eine reine Drehung)
        float q1[4];
        float q2[4];
        quatAusMatrix(u * x, q1);
        quatAusMatrix(x, q2);
        const BoneMatrix lq = matrixAusQuat(q1);
        const BoneMatrix rq = u * matrixAusQuat(q2);
        float dq = 0.0F;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) { dq = std::max(dq, std::fabs(lq.m[i][j] - rq.m[i][j])); }
        }
        std::printf("    starr gestreckt: mitdrehend %.6f, nur ueber das Quaternion %.6f\n", static_cast<double>(d), static_cast<double>(dq));
        expect("starr: bei Mass 0,64 und Scherung mitdrehend (< 1e-5); rot ueber das Quaternion allein (> 0,01)", d < 1e-5F && dq > 0.01F);
    }
    // --- Ketten an echten Skeletten (Hierarchie JKA, flach JK2) --------------------------------------------------------
    kettenTest("jka");
    kettenTest("jk2");
    // --- Fussrolle ------------------------------------------------------------------------------------------------------
    {
        const GlaAnimation g = gedreht(skelett(tabelle("jk2"), 60));
        const Inhalt leer;
        const Rig rig = rigAnlegen(g, leer, 0, {});
        IkSpur sp;
        expect("Fussrolle: Spur Bein R (JK2, mit Zehenknochen rtarsal)", ikSpurAnlegen(g, rig, IkArt::BeinRechts, sp) && sp.zehen.knochen == knochen(g, "rtarsal"));
        // das Knie gebeugt (FK ueber die Griffe, JK2 flach: Mitnehmen), damit jede Rolle in Reichweite bleibt
        RigSchicht beuge;
        {
            int go = -1;
            int gu = -1;
            for (std::size_t i = 0; i < rig.griffe.size(); ++i) {
                if (rig.griffe[i].seite != Seite::Rechts) { continue; }
                if (rig.griffe[i].art == GriffArt::Oberschenkel) { go = static_cast<int>(i); }
                if (rig.griffe[i].art == GriffArt::Unterschenkel) { gu = static_cast<int>(i); }
            }
            const float ax[3] = {1.0F, 0.0F, 0.0F};
            std::vector<BoneMatrix> w;
            rigWelt(g, beuge, 25, 25, 0.0F, w);
            rigGriffDrehen(g, rig, go, 25, w, drehungUm(ax, -35.0F), nullptr, beuge);
            rigWelt(g, beuge, 25, 25, 0.0F, w);
            rigGriffDrehen(g, rig, gu, 25, w, drehungUm(ax, 70.0F), nullptr, beuge);
        }
        // Drehpunkte wie rigAnlegen sie aus dem Netz setzt: Ferse hinter dem Knoechel am Boden, Ballen am Zehenknochen,
        // Zehenspitze vor dem Ballen (ohne Netz im Test von Hand)
        const int fuss = knochen(g, "rtalus");
        const int zeh = knochen(g, "rtarsal");
        const BoneMatrix& bf = g.bones[static_cast<std::size_t>(fuss)].basePose;
        const BoneMatrix& bz = g.bones[static_cast<std::size_t>(zeh)].basePose;
        const float ferse[3] = {bf.m[0][3], bf.m[1][3] + 3.0F, 0.0F};
        const float spitze[3] = {bz.m[0][3], bz.m[1][3] - 4.0F, 0.0F};
        std::memcpy(sp.ferse, ferse, sizeof ferse);
        std::memcpy(sp.spitzePunkt, spitze, sizeof spitze);
        sp.ballen[0] = bz.m[0][3];
        sp.ballen[1] = bz.m[1][3];
        sp.ballen[2] = bz.m[2][3];
        sp.hatFerse = sp.hatBallen = sp.hatSpitze = true;
        std::printf("    Rollachse (Grundstellung): %.3f %.3f %.3f\n", static_cast<double>(sp.rollAchse[0]), static_cast<double>(sp.rollAchse[1]), static_cast<double>(sp.rollAchse[2]));
        RigSchicht s = beuge;
        s.ik.push_back(sp);
        const int f = 25;
        ikAufFk(g, s, 0, f, 1.0F);
        std::vector<BoneMatrix> w0;
        rigWelt(g, s, f, f, 0.0F, w0);
        const auto welt = [&](const float p[3], const std::vector<BoneMatrix>& w) {
            float l[3];
            float o[3];
            g.bones[static_cast<std::size_t>(fuss)].basePoseInv.transform(p, l);
            starr(w[static_cast<std::size_t>(fuss)]).transform(l, o);
            return std::vector<float>{o[0], o[1], o[2]};
        };
        const auto zehWelt = [&](const float p[3], const std::vector<BoneMatrix>& w) {
            float l[3];
            float o[3];
            g.bones[static_cast<std::size_t>(zeh)].basePoseInv.transform(p, l);
            starr(w[static_cast<std::size_t>(zeh)]).transform(l, o);
            return std::vector<float>{o[0], o[1], o[2]};
        };
        const auto abv = [](const std::vector<float>& x, const std::vector<float>& y) { return std::sqrt((x[0] - y[0]) * (x[0] - y[0]) + (x[1] - y[1]) * (x[1] - y[1]) + (x[2] - y[2]) * (x[2] - y[2])); };
        // Ferse
        RigSchicht r1 = s;
        ikRolleSetzen(g, r1, 0, f, -25.0F, 0.0F);
        std::vector<BoneMatrix> w1;
        rigWelt(g, r1, f, f, 0.0F, w1);
        const float dFerse = abv(welt(ferse, w0), welt(ferse, w1));
        float k0[3];
        float k1[3];
        ortVon(w0[static_cast<std::size_t>(fuss)], k0);
        ortVon(w1[static_cast<std::size_t>(fuss)], k1);
        std::printf("    Rolle -25: Ferse %.6f bewegt, Knoechel %.3f\n", static_cast<double>(dFerse), static_cast<double>(ab(k0, k1)));
        expect("Fussrolle < 0: die Ferse bleibt liegen, der Fuss dreht (Rigify roll2)", dFerse < 1e-3F && ab(k0, k1) > 0.5F);
        // Ballen: der Zehenknochen behaelt Ort und Drehung
        RigSchicht r2 = s;
        ikRolleSetzen(g, r2, 0, f, 30.0F, 0.0F);
        std::vector<BoneMatrix> w2;
        rigWelt(g, r2, f, f, 0.0F, w2);
        float z0[3];
        float z2[3];
        ortVon(w0[static_cast<std::size_t>(zeh)], z0);
        ortVon(w2[static_cast<std::size_t>(zeh)], z2);
        const float dSpitzeZ = abv(zehWelt(spitze, w0), zehWelt(spitze, w2));
        std::printf("    Rolle +30 (Ballen): Ballen %.6f, Zehenspitze am Zehenknochen %.6f, Knoechel %.3f\n", static_cast<double>(abv(welt(sp.ballen, w0), welt(sp.ballen, w2))),
                    static_cast<double>(dSpitzeZ), static_cast<double>(ab(k0, std::vector<float>{w2[static_cast<std::size_t>(fuss)].m[0][3], w2[static_cast<std::size_t>(fuss)].m[1][3], w2[static_cast<std::size_t>(fuss)].m[2][3]}.data())));
        expect("Fussrolle > 0: um den Ballen, der Ballen bleibt (Rigify roll1)", abv(welt(sp.ballen, w0), welt(sp.ballen, w2)) < 1e-3F);
        expect("Fussrolle > 0: die Zehen bleiben liegen (Rigify: Zehen an roll2/roll3)", ab(z0, z2) < 1e-3F && dSpitzeZ < 1e-3F);
        // Spitze: Anteil 1 - die Zehenspitze bleibt
        RigSchicht r3 = s;
        ikRolleSetzen(g, r3, 0, f, 30.0F, 1.0F);
        std::vector<BoneMatrix> w3;
        rigWelt(g, r3, f, f, 0.0F, w3);
        const float dSp = abv(zehWelt(spitze, w0), zehWelt(spitze, w3));
        std::printf("    Rolle +30, Spitze 1: Zehenspitze %.6f\n", static_cast<double>(dSp));
        expect("Fussrolle > 0 mit Anteil Spitze 1: die Zehenspitze bleibt liegen (Rigify roll3)", dSp < 1e-3F);
    }
    // --- Fussrolle an JKA _humanoid (ohne Zehenknochen, gedrehte Grundstellung, Drehknochen rfemurX), bewegte Animation ------
    {
        const GlaAnimation g = bewegt(gestreckt(gedreht(skelett(tabelle("jka"), 60))), 4711U);
        const Inhalt leer;
        const Rig rig = rigAnlegen(g, leer, 0, {});
        IkSpur sp;
        expect("Fussrolle JKA: Spur Bein R ohne Zehenknochen", ikSpurAnlegen(g, rig, IkArt::BeinRechts, sp) && sp.zehen.knochen < 0);
        const int fuss = knochen(g, "rtalus");
        const BoneMatrix& bf = g.bones[static_cast<std::size_t>(fuss)].basePose;
        const float ferse[3] = {bf.m[0][3], bf.m[1][3] + 3.0F, bf.m[2][3] - 6.5F};
        const float spitze[3] = {bf.m[0][3], bf.m[1][3] - 8.0F, bf.m[2][3] - 6.5F};
        std::memcpy(sp.ferse, ferse, sizeof ferse);
        std::memcpy(sp.spitzePunkt, spitze, sizeof spitze);
        sp.hatFerse = sp.hatSpitze = true;
        int bestes = 0;
        float knie = -2.0F;
        for (int f = 0; f < g.numFrames; ++f) {
            std::vector<BoneMatrix> w;
            g.worldMatrices(f, w);
            float a[3], b[3], c[3];
            ortVon(w[static_cast<std::size_t>(sp.oben.knochen)], a);
            ortVon(w[static_cast<std::size_t>(sp.unten.knochen)], b);
            ortVon(w[static_cast<std::size_t>(sp.ende.knochen)], c);
            const float co = ((a[0] - b[0]) * (c[0] - b[0]) + (a[1] - b[1]) * (c[1] - b[1]) + (a[2] - b[2]) * (c[2] - b[2])) / (ab(a, b) * ab(c, b));
            if (co > knie) {
                knie = co;
                bestes = f;
            }
        }
        RigSchicht s;
        s.ik.push_back(sp);
        ikAufFk(g, s, 0, bestes, 1.0F);
        std::vector<BoneMatrix> w0;
        rigWelt(g, s, bestes, bestes, 0.0F, w0);
        const auto punkt = [&](const float p[3], const std::vector<BoneMatrix>& w, bool steif) {
            float l[3];
            float o[3];
            g.bones[static_cast<std::size_t>(fuss)].basePoseInv.transform(p, l);
            (steif ? starr(w[static_cast<std::size_t>(fuss)]) : w[static_cast<std::size_t>(fuss)]).transform(l, o);
            return std::vector<float>{o[0], o[1], o[2]};
        };
        const auto abv = [](const std::vector<float>& x, const std::vector<float>& y) { return std::sqrt((x[0] - y[0]) * (x[0] - y[0]) + (x[1] - y[1]) * (x[1] - y[1]) + (x[2] - y[2]) * (x[2] - y[2])); };
        for (const float r : {-15.0F, 15.0F}) {
            RigSchicht x = s;
            ikRolleSetzen(g, x, 0, bestes, r, 1.0F);
            std::vector<BoneMatrix> w1;
            rigWelt(g, x, bestes, bestes, 0.0F, w1);
            const float* p = r < 0.0F ? ferse : spitze;
            std::printf("    JKA Rolle %.0f (Bild %d): Drehpunkt %.6f (starr %.6f)\n", static_cast<double>(r), bestes, static_cast<double>(abv(punkt(p, w0, false), punkt(p, w1, false))),
                        static_cast<double>(abv(punkt(p, w0, true), punkt(p, w1, true))));
            expect(std::string("Fussrolle JKA ") + (r < 0.0F ? "Ferse" : "Spitze") + ": der Drehpunkt bleibt liegen (< 0,01)", abv(punkt(p, w0, false), punkt(p, w1, false)) < 1e-2F);
        }
    }
    // --- Space Switching: rechte Hand an der linken ---------------------------------------------------------------------
    {
        const GlaAnimation g = bewegt(skelett(tabelle("jka"), 60), 1234U);
        const Inhalt leer;
        const Rig rig = rigAnlegen(g, leer, 0, {});
        IkSpur sp;
        (void)ikSpurAnlegen(g, rig, IkArt::ArmRechts, sp);
        RigSchicht s;
        s.ik.push_back(sp);
        ikAufFk(g, s, 0, 0, 1.0F);
        ikAufFk(g, s, 0, 59, 1.0F);
        const RigSchicht vor = s;
        const int lhand = knochen(g, "lhand");
        const int rhand = knochen(g, "rhand");
        ikRaumWechseln(g, s, 0, 20, lhand, 0);
        const float sprung = ikSprung(g, vor, s, 20);
        const float sprung19 = ikSprung(g, vor, s, 19);
        std::printf("    Raum: Sprung am Wechsel (Bild 20) %.6f, Bild 19 %.6f\n", static_cast<double>(sprung), static_cast<double>(sprung19));
        expect("Space Switching: kein Sprung am Wechsel (Bild 19 und 20, < 0,001)", sprung < 1e-3F && sprung19 < 1e-3F);
        std::vector<BoneMatrix> w20;
        std::vector<BoneMatrix> w40;
        rigWelt(g, s, 20, 20, 0.0F, w20);
        rigWelt(g, s, 40, 40, 0.0F, w40);
        const BoneMatrix rel20 = starrInvers(starr(w20[static_cast<std::size_t>(lhand)])) * starr(w20[static_cast<std::size_t>(rhand)]);
        const BoneMatrix rel40 = starrInvers(starr(w40[static_cast<std::size_t>(lhand)])) * starr(w40[static_cast<std::size_t>(rhand)]);
        float d = 0.0F;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 4; ++j) { d = std::max(d, std::fabs(rel20.m[i][j] - rel40.m[i][j])); }
        }
        float l20[3];
        float l40[3];
        ortVon(w20[static_cast<std::size_t>(lhand)], l20);
        ortVon(w40[static_cast<std::size_t>(lhand)], l40);
        std::printf("    Raum: die linke Hand wandert %.3f, die rechte relativ zu ihr %.6f\n", static_cast<double>(ab(l20, l40)), static_cast<double>(d));
        expect("Space Switching: danach folgt die rechte Hand der linken starr (< 0,001)", d < 1e-3F && ab(l20, l40) > 0.5F);
        // Rueckwechsel in die Welt an Bild 45 ohne Sprung
        const RigSchicht vor2 = s;
        ikRaumWechseln(g, s, 0, 45, -1, 0);
        expect("Space Switching: zurueck in die Welt ohne Sprung (< 0,001)", ikSprung(g, vor2, s, 45) < 1e-3F && ikSprung(g, vor2, s, 44) < 1e-3F);
    }
    // --- Pfad ziehen: ein IK-Schluessel nur an diesem Bild (die Nachbarn halten die Pose) ----------------------------------
    {
        const GlaAnimation g = bewegt(skelett(tabelle("jka"), 60), 2024U);
        const Inhalt leer;
        const Rig rig = rigAnlegen(g, leer, 0, {});
        RigSchicht s;
        IkSpur sp;
        (void)ikSpurAnlegen(g, rig, IkArt::ArmRechts, sp);
        s.ik.push_back(sp);
        ikNurAmBild(g, s, 0, 30, 0, 59);
        std::vector<BoneMatrix> w;
        rigWelt(g, s, 30, 30, 0.0F, w);
        BoneMatrix z = starr(w[static_cast<std::size_t>(sp.ende.knochen)]);
        z.m[2][3] += 1.0F;
        ikZielSetzen(g, s, 0, 30, z);
        GlaAnimation ga;
        GlaAnimation gb;
        BackBericht x;
        (void)rigBacken(g, RigSchicht{}, 0, 60, false, "a", ga, &x);
        (void)rigBacken(g, s, 0, 60, false, "b", gb, &x);
        int aussen = 0;
        int innen = 0;
        for (std::size_t at = 0; at < ga.indexes.size(); ++at) {
            const std::uint8_t* p = ga.bonePool.data() + static_cast<std::size_t>(ga.indexes[at]) * 14U;
            const std::uint8_t* q = gb.bonePool.data() + static_cast<std::size_t>(gb.indexes[at]) * 14U;
            const bool ueber = drehAbstand(p, q) / kompressionsGrenze(p) > 1.0 || verschiebungsStufen(p, q) > 1;
            (at / g.bones.size() == 30U ? innen : aussen) += ueber ? 1 : 0;
        }
        std::printf("    Pfad an Bild 30: geaendert an Bild 30 %d Knochen, an den anderen 59 Bildern %d Knochen-Frames ueber der Grenze\n", innen, aussen);
        expect("Pfad ziehen: der IK-Schluessel wirkt nur an seinem Bild (die anderen Bilder innerhalb der Grenze)", innen > 0 && aussen == 0);
        // rot: ohne die Nachbarn haelt der eine Schluessel die Hand fuer die ganze Sequenz fest
        RigSchicht r;
        r.ik.push_back(sp);
        ikZielSetzen(g, r, 0, 30, z);
        GlaAnimation gr;
        (void)rigBacken(g, r, 0, 60, false, "r", gr, &x);
        int aussenRot = 0;
        for (std::size_t at = 0; at < ga.indexes.size(); ++at) {
            if (at / g.bones.size() == 30U) { continue; }
            const std::uint8_t* p = ga.bonePool.data() + static_cast<std::size_t>(ga.indexes[at]) * 14U;
            const std::uint8_t* q = gr.bonePool.data() + static_cast<std::size_t>(gr.indexes[at]) * 14U;
            aussenRot += (drehAbstand(p, q) / kompressionsGrenze(p) > 1.0 || verschiebungsStufen(p, q) > 1) ? 1 : 0;
        }
        expect("rot - Pfad ziehen ohne die Nachbarn: andere Bilder aendern sich", aussenRot > 0);
    }
    // --- IK aus (Anteil 0 ueberall): Backen Byte fuer Byte wie ohne Spur ---------------------------------------------------
    {
        const GlaAnimation g = bewegt(skelett(tabelle("jka"), 60), 77U);
        const Inhalt leer;
        const Rig rig = rigAnlegen(g, leer, 0, {});
        RigSchicht a;
        for (int f : {0, 20, 40}) { rigSchluesselSetzen(a, knochen(g, "rhumerus"), f, drehungUm(std::vector<float>{0.0F, 1.0F, 0.0F}.data(), static_cast<float>(f))); }
        RigSchicht b = a;
        IkSpur sp;
        (void)ikSpurAnlegen(g, rig, IkArt::ArmRechts, sp);
        b.ik.push_back(sp);
        ikAufFk(g, b, 0, 10, 0.0F);
        const Vergleich v = backenVergleich(g, a, b);
        expect("IK mit Anteil 0: gebacken Byte fuer Byte wie ohne Spur", v.gleich == v.alle && v.alle > 0);
    }
    // --- Reduzieren mit Auswahl (aus 3b offen) --------------------------------------------------------------------------
    {
        const GlaAnimation g = bewegt(skelett(tabelle("jka"), 60), 4711U);
        const RigSchicht leer;
        // nur ein Knochen
        ReduzierAuswahl nur;
        nur.knochen = {knochen(g, "rhumerus")};
        RigSchicht r1;
        ReduzierBericht b1;
        expect("Reduzieren nur rhumerus", rigReduzieren(g, leer, 0, 60, r1, &b1, &nur));
        std::size_t andere = 0;
        for (std::size_t k = 0; k < r1.knochen.size(); ++k) {
            if (static_cast<int>(k) != nur.knochen[0]) { andere += r1.knochen[k].size(); }
        }
        std::printf("    Reduzieren nur rhumerus: %zu Schluessel an rhumerus, %zu an anderen, Anteil %.4f, ueber %d\n", r1.knochen[static_cast<std::size_t>(nur.knochen[0])].size(),
                    andere, b1.maxAnteil, b1.ueberGrenze);
        expect("Reduzieren nur ein Knochen: die anderen ohne Schluessel (bleiben die GLA), gebacken innerhalb der Grenze", andere == 0 && b1.gewaehlt == 1 &&
                                                                                                                         !r1.knochen[static_cast<std::size_t>(nur.knochen[0])].empty() &&
                                                                                                                         b1.ueberGrenze == 0);
        // Spanne auf einer reduzierten Schicht: Schluessel ausserhalb bleiben Bit fuer Bit
        RigSchicht voll;
        ReduzierBericht bv;
        (void)rigReduzieren(g, leer, 0, 60, voll, &bv);
        ReduzierAuswahl sp;
        sp.von = 20;
        sp.bis = 39;
        RigSchicht r2;
        ReduzierBericht b2;
        expect("Reduzieren Spanne 20..39 einer reduzierten Schicht", rigReduzieren(g, voll, 0, 60, r2, &b2, &sp));
        int gleichAussen = 0;
        int aussen = 0;
        for (std::size_t k = 0; k < voll.knochen.size(); ++k) {
            for (const RigSchluessel& x : voll.knochen[k]) {
                if (x.bild >= 20 && x.bild <= 39) { continue; }
                ++aussen;
                for (const RigSchluessel& y : r2.knochen[k]) {
                    if (y.bild == x.bild && std::memcmp(x.q, y.q, sizeof x.q) == 0 && std::memcmp(x.t, y.t, sizeof x.t) == 0) { ++gleichAussen; }
                }
            }
        }
        std::printf("    Reduzieren Spanne: %d / %d Schluessel ausserhalb gleich, zusaetzlich aussen %d, Anteil %.4f, ueber %d (Schicht vorher %lld -> %lld)\n", gleichAussen,
                    aussen, b2.aussen, b2.maxAnteil, b2.ueberGrenze, bv.nachher, b2.nachher);
        expect("Reduzieren Spanne: Schluessel ausserhalb unveraendert, gebacken innerhalb der Grenze", gleichAussen == aussen && aussen > 0 && b2.ueberGrenze == 0);
        // auf der GLA (ohne Schicht) mit Spanne: ausserhalb je Bild ein Schluessel, innen weniger
        RigSchicht r3;
        ReduzierBericht b3;
        (void)rigReduzieren(g, leer, 0, 60, r3, &b3, &sp);
        const std::size_t z = r3.knochen[static_cast<std::size_t>(knochen(g, "rhand"))].size();
        std::printf("    Reduzieren Spanne auf der GLA: rhand %zu Schluessel (40 ausserhalb dicht), dicht %lld, ueber %d\n", z, b3.dicht, b3.ueberGrenze);
        expect("Reduzieren Spanne auf der GLA: ausserhalb dicht, innen reduziert, innerhalb der Grenze", z >= 40U && z < 60U && b3.ueberGrenze == 0);
    }
    // --- Posen-Bibliothek als Text ----------------------------------------------------------------------------------------
    {
        std::vector<BibliothekPose> p(2);
        p[0].name = "Stand A";
        p[0].namen = {"pelvis", "rhand"};
        p[0].welt = {{1.0F, 0.0F, 0.0F, 1.25F, 0.0F, 1.0F, 0.0F, -3.5F, 0.0F, 0.0F, 1.0F, 40.0F}, {0.70710678F, -0.70710678F, 0.0F, 0.1F, 0.70710678F, 0.70710678F, 0.0F, 0.2F, 0.0F, 0.0F, 1.0F, 1e-7F}};
        p[1].name = "leer";
        const std::string t = posenText(p);
        std::vector<BibliothekPose> q(1);
        q[0].name = "Stand A";   // gleicher Name wird ersetzt
        std::string fehler;
        const bool ok = posenAusText(t, q, fehler);
        bool gleich = ok && q.size() == 2 && q[0].name == "Stand A" && q[0].namen == p[0].namen && q[1].name == "leer" && q[1].namen.empty();
        for (std::size_t k = 0; gleich && k < 2; ++k) { gleich = std::memcmp(q[0].welt[k].data(), p[0].welt[k].data(), sizeof(float) * 12U) == 0; }
        expect("Posen-Datei: Text hin und zurueck Bit fuer Bit, gleicher Name ersetzt", gleich);
        std::vector<BibliothekPose> r;
        expect("Posen-Datei: fremder Text abgelehnt", !posenAusText("JKStudioPosen 2\n", r, fehler) && !fehler.empty());
    }
    std::printf("%s (%d FEHL)\n", fails == 0 ? "alles ok" : "FEHLER", fails);
    return fails == 0 ? 0 : 1;
}
