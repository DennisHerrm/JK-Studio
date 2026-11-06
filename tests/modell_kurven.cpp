// modell_kurven.cpp - Modell-Block Stufe 3b: Kurven, Werkzeuge, Timing, Werte, Spiegeln, Aufraeumen, Reduzieren
//
//   Kurven     Henkel und Auswertung wie Blender (Quellen in bhed/modell/animkurven.h), gegen Werte von Hand: Auto ohne
//              Glaettung (calchandleNurb_intern: Laenge = Abstand / 2,5614, Richtung aus beiden Nachbarn), Auto Clamped
//              am Extremum waagerecht, Vektor ein Drittel zum Nachbarn, Ausgerichtet gegenueber, die Enden waagerecht
//              (konstante Extrapolation), Continuous Acceleration: Henkel ein Drittel des Abstands, zweite Ableitung
//              stetig am inneren Schluessel; findzero/berekeny, correct_bezpart
//   Werkzeuge  Breakdown, Blend to Neighbor/Default, Push/Pull, Ease, Glaetten (keyframes_general.cc), Posen-Schieber
//              (pose_slide.cc) gegen Werte von Hand
//   Timing     Verschieben, Skalieren, Einrasten, Zwischenbilder, Zeitversatz, Zeiten teilen; Werte; Anim Offset
//   Spiegeln   die ganze Sequenz: je Bild und Knochenpaar W'(b) = S W(p) B(p)^-1 S B(b)
//   Reduzieren eine bewegte GLA: wenige Schluessel, gebacken jedes Knochen-Bild innerhalb der Grenze aus compressBone und
//              mit derselben Verschiebung; Gegenprobe: ein entfernter Schluessel muss die Pruefung reissen
//
// Aufruf: modellkurventest
#include "bhed/gla.h"
#include "bhed/modell/animkurven.h"
#include "bhed/modell/dokument.h"
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
bool nah(float a, float b, float e = 1e-4F) { return std::fabs(a - b) <= e; }

using namespace bhed;
using namespace bhed::modell;

// Ein Kanal (Verschiebung x = Kanal 4) mit Schluesseln (Bild, Wert) und einer Henkelart.
std::vector<RigSchluessel> kurve(const std::vector<std::pair<int, float>>& p, Henkel h, Interp ip = Interp::Bezier) {
    std::vector<RigSchluessel> v;
    for (const auto& [b, y] : p) {
        RigSchluessel k;
        k.bild = b;
        k.interp = ip;
        k.t[0] = y;
        for (int c = 0; c < kKanaele; ++c) { k.henkelL[c] = k.henkelR[c] = h; }
        v.push_back(k);
    }
    return v;
}

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

float maxDiff(const BoneMatrix& a, const BoneMatrix& b) {
    float d = 0.0F;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j) { d = std::max(d, std::fabs(a.m[i][j] - b.m[i][j])); }
    }
    return d;
}

float winkelZ(const BoneMatrix& m) { return std::atan2(m.m[1][0], m.m[0][0]) * 180.0F / 3.14159265F; }

}  // namespace

int main() {
    // --- Henkel wie Blender ------------------------------------------------------------------------------------------
    {
        // (0,0) (10,5) (20,20), Auto ohne Glaettung: tvec = (2, 2), Laenge 10 / 2,5614 je Seite
        std::vector<RigSchluessel> v = kurve({{0, 0.0F}, {10, 5.0F}, {20, 20.0F}}, Henkel::Auto);
        henkelNeu(v, false);
        const float l = 10.0F / 2.5614F;
        std::printf("    Auto: links (%.5f, %.5f) rechts (%.5f, %.5f), erwartet +-%.5f\n", static_cast<double>(v[1].hl[4][0]),
                    static_cast<double>(v[1].hl[4][1]), static_cast<double>(v[1].hr[4][0]), static_cast<double>(v[1].hr[4][1]), static_cast<double>(l));
        expect("Auto: Henkel = (Abstand / 2,5614) * (1, 1) wie calchandleNurb_intern", nah(v[1].hl[4][0], -l) && nah(v[1].hl[4][1], -l) && nah(v[1].hr[4][0], l) &&
                                                                                       nah(v[1].hr[4][1], l));
        expect("Auto: Enden waagerecht (konstante Extrapolation)", nah(v[0].hr[4][1], 0.0F) && nah(v[2].hl[4][1], 0.0F));
        // Auto Clamped am Extremum: (0,0) (10,10) (20,2) - Mitte ist das Maximum, der Henkel waagerecht
        std::vector<RigSchluessel> w = kurve({{0, 0.0F}, {10, 10.0F}, {20, 2.0F}}, Henkel::AutoGeklemmt);
        henkelNeu(w, false);
        std::vector<RigSchluessel> wa = kurve({{0, 0.0F}, {10, 10.0F}, {20, 2.0F}}, Henkel::Auto);
        henkelNeu(wa, false);
        expect("Auto Clamped: am Extremum waagerecht", nah(w[1].hl[4][1], 0.0F) && nah(w[1].hr[4][1], 0.0F));
        expect("Auto (ohne Klemme): am Extremum geneigt", std::fabs(wa[1].hl[4][1]) > 0.1F);
        float hoechst = 0.0F;
        for (int i = 0; i <= 200; ++i) { hoechst = std::max(hoechst, kanalAuswerten(w, 4, static_cast<float>(i) * 0.1F)); }
        expect("Auto Clamped: kein Ueberschwingen ueber den Schluessel", hoechst <= 10.0F + 1e-4F);
        // Vektor: ein Drittel zum Nachbarn
        std::vector<RigSchluessel> vk = kurve({{0, 0.0F}, {9, 3.0F}, {21, 9.0F}}, Henkel::Vektor);
        henkelNeu(vk, false);
        expect("Vektor: links (p1 - p2) / 3, rechts (p3 - p2) / 3", nah(vk[1].hl[4][0], -3.0F) && nah(vk[1].hl[4][1], -1.0F) && nah(vk[1].hr[4][0], 4.0F) &&
                                                                  nah(vk[1].hr[4][1], 2.0F));
        // grosse Bildnummern (Movie Duels _humanoid hat 60861 Frames): float traegt bei 19000 nur noch 0,002 - die Henkel
        // muessen trotzdem genau ein Drittel des Abstands haben (mb5_ui6: 3,33398 statt 3,33333)
        std::vector<RigSchluessel> vg = kurve({{19000, 0.0F}, {19010, 3.0F}, {19020, 9.0F}}, Henkel::Vektor);
        henkelNeu(vg, false);
        std::printf("    Vektor bei Frame 19010: links %.6f, rechts %.6f\n", static_cast<double>(vg[1].hl[4][0]), static_cast<double>(vg[1].hr[4][0]));
        expect("Vektor bei Frame 19010: Henkel genau ein Drittel (10/3) des Abstands (Zeit relativ gerechnet)", nah(vg[1].hl[4][0], -10.0F / 3.0F, 1e-5F) && nah(vg[1].hr[4][0], 10.0F / 3.0F, 1e-5F));
        std::vector<RigSchluessel> vb = kurve({{40000, 0.0F}, {40010, 10.0F}}, Henkel::AutoGeklemmt);
        henkelNeu(vb, true);
        expect("Bezier bei Frame 40005: Mitte 5 wie bei Frame 5", nah(kanalAuswerten(vb, 4, 40005.0F), 5.0F, 1e-3F));
        // Ausgerichtet: der linke folgt dem rechten (gegenueber, eigene Laenge)
        std::vector<RigSchluessel> va = kurve({{0, 0.0F}, {10, 5.0F}, {20, 0.0F}}, Henkel::Ausgerichtet);
        va[1].hl[4][0] = -2.0F;
        va[1].hl[4][1] = 0.0F;
        va[1].hr[4][0] = 3.0F;
        va[1].hr[4][1] = 4.0F;
        henkelNeu(va, false);
        expect("Ausgerichtet: linker Henkel gegenueber dem rechten, Laenge bleibt 2", nah(va[1].hl[4][0], -1.2F) && nah(va[1].hl[4][1], -1.6F));
        // Frei: bleibt wie gesetzt
        std::vector<RigSchluessel> vf = kurve({{0, 0.0F}, {10, 5.0F}, {20, 0.0F}}, Henkel::Frei);
        vf[1].hl[4][0] = -2.0F;
        vf[1].hl[4][1] = 7.0F;
        henkelNeu(vf, true);
        expect("Frei: der Henkel bleibt", nah(vf[1].hl[4][0], -2.0F) && nah(vf[1].hl[4][1], 7.0F));
        // Continuous Acceleration (Vorgabe): x-Laenge ein Drittel, zweite Ableitung stetig am inneren Schluessel
        std::vector<RigSchluessel> vc = kurve({{0, 0.0F}, {10, 5.0F}, {30, 20.0F}, {40, 21.0F}}, Henkel::Auto);
        henkelNeu(vc, true);
        expect("Glatt: Henkel ein Drittel des Abstands (10/3 links, 20/3 rechts)", nah(vc[1].hl[4][0], -10.0F / 3.0F) && nah(vc[1].hr[4][0], 20.0F / 3.0F));
        // einseitige zweite Differenzen am Schluessel (Bild 10), Schritt 0,05
        const auto seiten = [](const std::vector<RigSchluessel>& k, float& links, float& rechts) {
            const float h = 0.05F;
            links = (kanalAuswerten(k, 4, 10.0F) - 2.0F * kanalAuswerten(k, 4, 10.0F - h) + kanalAuswerten(k, 4, 10.0F - 2.0F * h)) / (h * h);
            rechts = (kanalAuswerten(k, 4, 10.0F + 2.0F * h) - 2.0F * kanalAuswerten(k, 4, 10.0F + h) + kanalAuswerten(k, 4, 10.0F)) / (h * h);
        };
        float al = 0.0F;
        float ar = 0.0F;
        seiten(vc, al, ar);
        std::printf("    Glatt: zweite Ableitung links %.4f rechts %.4f\n", static_cast<double>(al), static_cast<double>(ar));
        expect("Glatt: zweite Ableitung stetig am Schluessel (Continuous Acceleration)", std::fabs(al - ar) < 0.02F);
        std::vector<RigSchluessel> vn = vc;
        henkelNeu(vn, false);
        float bl = 0.0F;
        float bri = 0.0F;
        seiten(vn, bl, bri);
        std::printf("    ohne Glaettung: links %.4f rechts %.4f\n", static_cast<double>(bl), static_cast<double>(bri));
        expect("Gegenprobe ohne Glaettung: zweite Ableitung springt", std::fabs(bl - bri) > 0.05F);
    }
    // --- Auswertung ----------------------------------------------------------------------------------------------------
    {
        float o[32];
        expect("findzero: lineare Henkel (0, 1/3, 2/3, 1) ergeben t = x", bez::findeNull(0.25F, 0.0F, 1.0F / 3.0F, 2.0F / 3.0F, 1.0F, o) == 1 && nah(o[0], 0.25F));
        expect("berekeny: t = 0,5 auf (0, 0, 1, 1) = 0,5", nah(bez::yBei(0.0F, 0.0F, 1.0F, 1.0F, 0.5F), 0.5F));
        float v1[2] = {0.0F, 0.0F};
        float v2[2] = {15.0F, 3.0F};
        float v3[2] = {5.0F, 1.0F};
        const float v4[2] = {10.0F, 1.0F};
        bez::korrigiere(v1, v2, v3, v4);
        expect("correct_bezpart: zu langer Henkel auf den Abstand gekuerzt (15 -> 10)", nah(v2[0], 10.0F) && nah(v2[1], 2.0F) && nah(v3[0], 5.0F));
        std::vector<RigSchluessel> v = kurve({{0, 0.0F}, {10, 10.0F}}, Henkel::AutoGeklemmt);
        henkelNeu(v, true);
        expect("Bezier: an den Schluesseln genau ihr Wert", nah(kanalAuswerten(v, 4, 0.0F), 0.0F) && nah(kanalAuswerten(v, 4, 10.0F), 10.0F));
        expect("Bezier mit waagerechten Enden: Mitte 5 (punktsymmetrisch)", nah(kanalAuswerten(v, 4, 5.0F), 5.0F, 1e-3F));
        expect("Bezier: Ease-in (Bild 2 unter linear)", kanalAuswerten(v, 4, 2.0F) < 2.0F);
        std::vector<RigSchluessel> k = kurve({{0, 0.0F}, {10, 10.0F}}, Henkel::AutoGeklemmt, Interp::Konstant);
        expect("Konstant: haelt bis zum naechsten", nah(kanalAuswerten(k, 4, 9.5F), 0.0F));
        std::vector<RigSchluessel> li = kurve({{0, 0.0F}, {10, 10.0F}}, Henkel::AutoGeklemmt, Interp::Linear);
        expect("Linear: 2,5 bei Bild 2,5", nah(kanalAuswerten(li, 4, 2.5F), 2.5F));
        expect("ausserhalb konstant", nah(kanalAuswerten(li, 4, -5.0F), 0.0F) && nah(kanalAuswerten(li, 4, 50.0F), 10.0F));
        // Quaternion mit Bezier: normiert
        RigSchicht s;
        s.neuInterp = Interp::Bezier;
        const float z[3] = {0.0F, 0.0F, 1.0F};
        rigSchluesselSetzen(s, 0, 0, BoneMatrix::identity());
        rigSchluesselSetzen(s, 0, 10, drehungUm(z, 90.0F));
        rigSchluesselSetzen(s, 0, 20, drehungUm(z, 180.0F));
        BoneMatrix m;
        (void)rigWert(s, 0, 10.0F, m);
        expect("Bezier-Quaternion: am Schluessel 90 Grad", nah(winkelZ(m), 90.0F, 1e-3F));
        (void)rigWert(s, 0, 5.0F, m);
        const float det = m.m[0][0] * m.m[1][1] - m.m[0][1] * m.m[1][0];
        expect("Bezier-Quaternion: dazwischen eine Drehung (normiert, det 1)", nah(det, 1.0F, 1e-4F) && winkelZ(m) > 0.0F && winkelZ(m) < 90.0F);
        expect("Halbkugel: q bei 180 Grad dem Nachbarn zugewandt", s.knochen[0][2].q[0] * s.knochen[0][1].q[0] + s.knochen[0][2].q[3] * s.knochen[0][1].q[3] >= 0.0F);
    }
    // --- Werkzeuge auf Schluesseln ---------------------------------------------------------------------------------------
    {
        const auto drei = [] {
            RigSchicht s;
            s.knochen.resize(1);
            s.knochen[0] = kurve({{0, 0.0F}, {10, 7.0F}, {20, 10.0F}}, Henkel::AutoGeklemmt);
            henkelNeu(s.knochen[0]);
            return s;
        };
        const Wahl mitte{{0, 10}};
        RigSchicht s = drei();
        (void)kurvenWerkzeug(s, mitte, KurvenWerkzeug::Breakdown, 0.0F);
        expect("Breakdown 0: Mitte zwischen den Nachbarn (5)", nah(s.knochen[0][1].t[0], 5.0F));
        s = drei();
        (void)kurvenWerkzeug(s, mitte, KurvenWerkzeug::Breakdown, 1.0F);
        expect("Breakdown 1: der rechte Nachbar (10)", nah(s.knochen[0][1].t[0], 10.0F));
        s = drei();
        (void)kurvenWerkzeug(s, mitte, KurvenWerkzeug::BlendNachbar, -0.5F);
        expect("Blend to Neighbor -0,5: halb zum linken (3,5)", nah(s.knochen[0][1].t[0], 3.5F));
        s = drei();
        (void)kurvenWerkzeug(s, mitte, KurvenWerkzeug::BlendVorgabe, 1.0F);
        expect("Blend to Default 1: Vorgabe 0 (q w = 1)", nah(s.knochen[0][1].t[0], 0.0F) && nah(s.knochen[0][1].q[0], 1.0F));
        s = drei();
        (void)kurvenWerkzeug(s, mitte, KurvenWerkzeug::PushPull, 2.0F);
        expect("Push/Pull 2: Abstand zur Geraden verdoppelt (5 + 2 * 2 = 9)", nah(s.knochen[0][1].t[0], 9.0F));
        s = drei();
        (void)kurvenWerkzeug(s, mitte, KurvenWerkzeug::PushPull, 1.0F);
        expect("Push/Pull 1: unveraendert", nah(s.knochen[0][1].t[0], 7.0F));
        s = drei();
        (void)kurvenWerkzeug(s, mitte, KurvenWerkzeug::Ease, 0.0F);
        expect("Ease 0: Mitte des S (5)", nah(s.knochen[0][1].t[0], 5.0F));
        s = drei();
        (void)kurvenWerkzeug(s, mitte, KurvenWerkzeug::Ease, 1.0F);
        expect("Ease 1: auf dem S ueber der Mitte (9,2195 von Hand: sigmoid mit shift -1, Schaerfe 2)", nah(s.knochen[0][1].t[0], 9.2195F, 1e-3F));
        RigSchicht sp;
        sp.knochen.resize(1);
        sp.knochen[0] = kurve({{0, 0.0F}, {1, 0.0F}, {2, 0.0F}, {3, 10.0F}, {4, 0.0F}, {5, 0.0F}, {6, 0.0F}}, Henkel::AutoGeklemmt, Interp::Linear);
        Wahl alle;
        for (int b = 0; b <= 6; ++b) { alle.push_back({0, b}); }
        (void)kurvenWerkzeug(sp, alle, KurvenWerkzeug::Glaetten, 1.0F);
        std::printf("    Glaetten: Spitze 10 -> %.3f, Nachbar 0 -> %.3f\n", static_cast<double>(sp.knochen[0][3].t[0]), static_cast<double>(sp.knochen[0][2].t[0]));
        expect("Glaetten: Spitze kleiner, Nachbarn heben sich", sp.knochen[0][3].t[0] < 10.0F && sp.knochen[0][2].t[0] > 0.0F);
        // Posen-Schieber
        RigSchicht ps;
        const float z[3] = {0.0F, 0.0F, 1.0F};
        rigSchluesselSetzen(ps, 0, 0, BoneMatrix::identity());
        rigSchluesselSetzen(ps, 0, 10, drehungUm(z, 90.0F));
        (void)poseSchieben(ps, {0}, 4, PosenSchieber::Breakdown, 0.5F);
        BoneMatrix m;
        (void)rigWert(ps, 0, 4.0F, m);
        expect("Posen-Breakdown 0,5 bei Bild 4: 45 Grad (slerp der Nachbarn)", nah(winkelZ(m), 45.0F, 1e-3F) && ps.knochen[0].size() == 3U);
        (void)poseSchieben(ps, {0}, 4, PosenSchieber::Relax, 1.0F);
        (void)rigWert(ps, 0, 4.0F, m);
        expect("Posen-Relax 1: auf die Gerade zwischen den Nachbarn (36 Grad bei 4/10)", nah(winkelZ(m), 36.0F, 1e-2F));
        (void)poseSchieben(ps, {0}, 4, PosenSchieber::BlendRuhe, 1.0F);
        (void)rigWert(ps, 0, 4.0F, m);
        expect("Posen-Blend to Rest 1: Einheit", nah(winkelZ(m), 0.0F, 1e-3F));
    }
    // --- Timing ----------------------------------------------------------------------------------------------------------
    {
        RigSchicht s;
        s.knochen.resize(2);
        s.knochen[0] = kurve({{0, 0.0F}, {10, 1.0F}, {20, 2.0F}}, Henkel::AutoGeklemmt);
        s.knochen[1] = kurve({{5, 5.0F}, {15, 6.0F}}, Henkel::AutoGeklemmt);
        Wahl w = schluesselVerschieben(s, {{0, 10}}, 3);
        expect("Verschieben +3: 10 -> 13, Auswahl folgt", s.knochen[0][1].bild == 13 && w.size() == 1U && w[0].bild == 13);
        w = schluesselVerschieben(s, {{0, 13}}, 7);
        expect("Verschieben auf einen Schluessel: ersetzt ihn (2 Schluessel, Wert 1)", s.knochen[0].size() == 2U && s.knochen[0][1].bild == 20 && nah(s.knochen[0][1].t[0], 1.0F));
        s.knochen[0] = kurve({{0, 0.0F}, {10, 1.0F}, {20, 2.0F}}, Henkel::AutoGeklemmt);
        (void)schluesselSkalieren(s, {{0, 0}, {0, 10}, {0, 20}}, 0.5F, 0.0F);
        expect("Skalieren 0,5 um 0: 0, 5, 10", s.knochen[0][1].bild == 5 && s.knochen[0][2].bild == 10);
        (void)schluesselEinrasten(s, {{0, 5}, {0, 10}}, Einrasten::ErsterAufCursor, 30);
        expect("Erster auf Cursor 30: 30, 35", s.knochen[0][1].bild == 30 && s.knochen[0][2].bild == 35);
        (void)schluesselEinrasten(s, {{0, 30}, {0, 35}}, Einrasten::LetzterAufCursor, 30);
        expect("Letzter auf Cursor 30: 25, 30", s.knochen[0][1].bild == 25 && s.knochen[0][2].bild == 30);
        (void)zeitEinfuegen(s, 10, 4);
        expect("4 Zwischenbilder ab 10: 25 -> 29, Knochen 2: 15 -> 19, 5 bleibt", s.knochen[0][1].bild == 29 && s.knochen[1][1].bild == 19 && s.knochen[1][0].bild == 5);
        (void)zeitEntfernen(s, 18, 2);
        expect("2 Bilder ab 18 entfernen: 19 faellt weg, 29 -> 27", s.knochen[1].size() == 1U && s.knochen[0][1].bild == 27);
        RigSchicht zv;
        zv.knochen.resize(1);
        zv.knochen[0] = kurve({{0, 0.0F}, {10, 10.0F}, {20, 20.0F}}, Henkel::AutoGeklemmt, Interp::Linear);
        (void)zeitVersatz(zv, {{0, 10}}, 5.0F);
        expect("Zeitversatz +5: Wert der Kurve bei 15 (15)", nah(zv.knochen[0][1].t[0], 15.0F));
        zv.knochen[0] = kurve({{0, 0.0F}, {10, 10.0F}, {20, 20.0F}}, Henkel::AutoGeklemmt, Interp::Linear);
        (void)zeitVersatz(zv, {{0, 10}}, 15.0F);
        expect("Zeitversatz +15 zyklisch: 25 -> 5 + Spanne 20 = 25", nah(zv.knochen[0][1].t[0], 25.0F));
        RigSchicht te;
        te.knochen.resize(2);
        te.knochen[0] = kurve({{0, 0.0F}, {10, 10.0F}}, Henkel::AutoGeklemmt, Interp::Linear);
        te.knochen[1] = kurve({{0, 0.0F}, {4, 4.0F}, {8, 8.0F}}, Henkel::AutoGeklemmt, Interp::Linear);
        (void)zeitenTeilen(te, {0, 1}, -1);
        expect("Zeiten teilen: Knochen 1 hat 0, 4, 8, 10 (Wert 4 bei 4)", te.knochen[0].size() == 4U && nah(te.knochen[0][1].t[0], 4.0F) && te.knochen[1].size() == 4U);
        te.knochen[0] = kurve({{0, 0.0F}, {10, 10.0F}}, Henkel::AutoGeklemmt, Interp::Linear);
        te.knochen[1] = kurve({{0, 0.0F}, {4, 4.0F}, {8, 8.0F}}, Henkel::AutoGeklemmt, Interp::Linear);
        (void)zeitenTeilen(te, {0, 1}, 1);
        expect("Zeiten vom zuletzt gewaehlten: Knochen 1 genau 0, 4, 8", te.knochen[0].size() == 3U && te.knochen[0][2].bild == 8 && nah(te.knochen[0][2].t[0], 8.0F));
    }
    // --- Werte und Anim Offset -------------------------------------------------------------------------------------------
    {
        RigSchicht s;
        const float z[3] = {0.0F, 0.0F, 1.0F};
        BoneMatrix m30 = drehungUm(z, 30.0F);
        m30.m[0][3] = 2.0F;
        rigSchluesselSetzen(s, 0, 0, m30);
        rigSchluesselSetzen(s, 0, 10, drehungUm(z, 60.0F));
        rigSchluesselSetzen(s, 0, 20, drehungUm(z, 90.0F));
        RigSchicht a = s;
        (void)werteAendern(a, {{0, 0}}, WertArt::Negieren, 0);
        BoneMatrix m;
        (void)rigWert(a, 0, 0.0F, m);
        expect("Negieren: -30 Grad, t -2", nah(winkelZ(m), -30.0F, 1e-3F) && nah(m.m[0][3], -2.0F));
        a = s;
        (void)werteAendern(a, {{0, 20}}, WertArt::AktuellerWert, 10);
        (void)rigWert(a, 0, 20.0F, m);
        expect("Auf aktuellen Wert (Cursor 10): 60 Grad", nah(winkelZ(m), 60.0F, 1e-3F));
        a = s;
        (void)werteAendern(a, {{0, 10}}, WertArt::Null, 0);
        (void)rigWert(a, 0, 10.0F, m);
        expect("Null: Einheit", nah(winkelZ(m), 0.0F, 1e-3F));
        a = s;
        (void)werteAendern(a, {{0, 0}, {0, 10}, {0, 20}}, WertArt::Umkehren, 0);
        (void)rigWert(a, 0, 0.0F, m);
        expect("Umkehren: am Anfang jetzt 90 Grad", nah(winkelZ(m), 90.0F, 1e-3F));
        a = s;
        (void)werteAendern(a, {{0, 0}, {0, 10}, {0, 20}}, WertArt::SpiegelnCursor, 25);
        expect("Spiegeln am Cursor 25: 30, 40, 50", a.knochen[0][0].bild == 30 && a.knochen[0][2].bild == 50);
        a = s;
        RigSchluessel vorher = a.knochen[0][1];
        rigSchluesselSetzen(a, 0, 10, drehungUm(z, 70.0F));
        const RigSchluessel nachher = a.knochen[0][1];
        (void)animOffset(a, 0, vorher, nachher, {});
        (void)rigWert(a, 0, 20.0F, m);
        BoneMatrix m0;
        (void)rigWert(a, 0, 0.0F, m0);
        expect("Anim Offset: +10 Grad auch bei 0 (40) und 20 (100)", nah(winkelZ(m0), 40.0F, 1e-2F) && nah(winkelZ(m), 100.0F, 1e-2F) && nah(m0.m[0][3], 2.0F));
    }
    // --- Spiegeln (JKA-Skelett) ------------------------------------------------------------------------------------------
    {
        const RigHandSkelett& h = tabelle("jka");
        GlaAnimation g = skelett(h, 12);
        const Rig r = rigAnlegen(g, Inhalt{}, 0, {});
        RigSchicht s;
        const int lh = boneIndex(g, "lhumerus");
        const int rh = boneIndex(g, "rhumerus");
        const float ax[3] = {0.3F, 0.5F, 0.8F};
        for (const int f : {0, 5, 11}) {
            rigSchluesselSetzen(s, lh, f, drehungUm(ax, 20.0F + 3.0F * static_cast<float>(f)));
            rigSchluesselSetzen(s, rh, f, drehungUm(ax, -10.0F));
        }
        std::vector<std::vector<BoneMatrix>> vorher;
        std::vector<BoneMatrix> welt;
        for (int f = 0; f < 12; ++f) {
            rigWelt(g, s, f, f, 0.0F, welt);
            vorher.push_back(welt);
        }
        (void)animationSpiegeln(g, r, s, 0, 12);
        BoneMatrix S = BoneMatrix::identity();
        S.m[r.erkennung.seitenAchse][r.erkennung.seitenAchse] = -1.0F;
        float schlimm = 0.0F;
        int paare = 0;
        for (int f = 0; f < 12; ++f) {
            rigWelt(g, s, f, f, 0.0F, welt);
            for (std::size_t b = 0; b < g.bones.size(); ++b) {
                const int p = r.partner[b];
                if (p < 0) { continue; }
                const BoneMatrix soll = S * vorher[static_cast<std::size_t>(f)][static_cast<std::size_t>(p)] * g.bones[static_cast<std::size_t>(p)].basePoseInv * S * g.bones[b].basePose;
                schlimm = std::max(schlimm, maxDiff(welt[b], soll));
                ++paare;
            }
        }
        std::printf("    Spiegeln: %d Knochen-Bilder, groesster Abstand %.6f\n", paare, static_cast<double>(schlimm));
        expect("Animation spiegeln: jedes Bild, jeder Knochen = S W(Partner) B^-1 S B", schlimm < 2e-4F && paare == 12 * static_cast<int>(g.bones.size()));
        rigWelt(g, s, 5, 5, 0.0F, welt);
        expect("Spiegeln: der rechte Oberarm dreht jetzt wie vorher der linke (gespiegelt, nicht gleich)", maxDiff(welt[static_cast<std::size_t>(rh)], vorher[5][static_cast<std::size_t>(rh)]) > 0.01F);
    }
    // --- Aufraeumen ------------------------------------------------------------------------------------------------------
    {
        RigSchicht s;
        s.knochen.resize(3);
        s.knochen[0] = kurve({{0, 1.0F}, {5, 1.0F}, {10, 1.0F}, {15, 3.0F}, {20, 3.0F}}, Henkel::AutoGeklemmt);
        s.knochen[1] = kurve({{0, 2.0F}, {10, 2.0F}, {20, 2.0F}}, Henkel::AutoGeklemmt);
        s.knochen[2] = kurve({{0, 0.0F}, {10, 0.0F}}, Henkel::AutoGeklemmt);
        RigSchicht a = s;
        const int weg = ueberfluessigeEntfernen(a, {});
        expect("Ueberfluessige wie clean_fcurve: 5 und 20 (gleich wie die Nachbarn) fallen, 0, 10, 15 bleiben; gleiche Knochen behalten einen",
               a.knochen[0].size() == 3U && a.knochen[0][1].bild == 10 && a.knochen[0][2].bild == 15 && a.knochen[1].size() == 1U && a.knochen[2].size() == 1U && weg == 5);
        a = s;
        (void)statischeEntfernen(a, {});
        expect("Statisch: ein Schluessel fuer Knochen 1, Knochen 2 (Einheit) ganz weg", a.knochen[1].size() == 1U && a.knochen[2].empty() && a.knochen[0].size() == 5U);
        a = s;
        a.bindeBasis = true;
        (void)statischeEntfernen(a, {});
        expect("Statisch mit Bindebasis: die Einheit bleibt als ein Schluessel", a.knochen[2].size() == 1U);
        RigSchicht q;
        q.knochen.resize(1);
        q.knochen[0] = kurve({{0, 0.0F}, {10, 0.0F}}, Henkel::AutoGeklemmt);
        q.knochen[0][1].q[0] = -1.0F;
        expect("Quaternion-Filter: das gewendete q zurueck (1 Schluessel)", quatFilter(q, {}) == 1 && q.knochen[0][1].q[0] > 0.0F);
    }
    // --- Reduzieren ------------------------------------------------------------------------------------------------------
    {
        const RigHandSkelett& h = tabelle("jka");
        const GlaAnimation g = skelett(h, 60);
        // eine bewegte Quelle: je Knochen lineare Schluessel (slerp) bei 0, 20, 40, 59, gebacken
        RigSchicht qs;
        std::uint32_t zufall = 4711U;
        const auto naechste = [&]() {
            zufall = zufall * 1664525U + 1013904223U;
            return static_cast<float>(zufall >> 8) / static_cast<float>(1U << 24);
        };
        for (int b = 0; b < h.zahl; ++b) {
            for (const int f : {0, 20, 40, 59}) {
                const float achse[3] = {naechste() - 0.5F, naechste() - 0.5F, naechste() - 0.5F};
                BoneMatrix m = drehungUm(achse, (naechste() - 0.5F) * 70.0F);
                if (b == 1) { m.m[2][3] = (naechste() - 0.5F) * 6.0F; }
                rigSchluesselSetzen(qs, b, f, m);
            }
        }
        GlaAnimation quelle;
        BackBericht bb;
        expect("Quelle backen (60 Bilder, 53 Knochen)", rigBacken(g, qs, 0, 60, true, "quelle", quelle, &bb));
        const RigSchicht leer;
        RigSchicht red;
        ReduzierBericht br;
        expect("Reduzieren", rigReduzieren(quelle, leer, 0, 60, red, &br));
        std::printf("    Reduzieren: %lld -> %lld Schluessel (linear %d, Bezier %d, statisch %d), groesster Anteil %.4f (%s), ueber der Grenze %d, eine Stufe %lld\n", br.vorher,
                    br.nachher, br.linear, br.bezier, br.statisch, br.maxAnteil, br.schlimmster.c_str(), br.ueberGrenze, br.eineStufe);
        expect("Reduzieren: weniger als ein Drittel der Schluessel", br.nachher * 3 < br.vorher);
        expect("Reduzieren: gebacken jedes Knochen-Bild innerhalb der Grenze, Verschiebung hoechstens eine Stufe", br.ueberGrenze == 0 && br.maxAnteil <= 1.0);
        expect("Reduzieren: Bindebasis", red.bindeBasis);
        // Gegenprobe: an einem Knochen mit vielen Schluesseln den mittleren entfernen - die Pruefung muss es merken
        int bester = 0;
        for (std::size_t b = 0; b < red.knochen.size(); ++b) {
            if (red.knochen[b].size() > red.knochen[static_cast<std::size_t>(bester)].size()) { bester = static_cast<int>(b); }
        }
        RigSchicht kaputt = red;
        auto& v = kaputt.knochen[static_cast<std::size_t>(bester)];
        v.erase(v.begin() + static_cast<std::ptrdiff_t>(v.size() / 2));
        henkelNeu(v);
        ReduzierBericht pr;
        expect("Gegenprobe: ohne einen noetigen Schluessel liegen Bilder ueber der Grenze", reduziertPruefen(quelle, leer, kaputt, 0, 60, pr) && pr.ueberGrenze > 0);
        std::printf("    Gegenprobe: %d Knochen-Bilder ueber der Grenze, Anteil %.3f\n", pr.ueberGrenze, pr.maxAnteil);
        // Bindebasis in der Ansicht (rigWelt, M_eff = B^-1 A^-1 B M) gegen die Rechnung des Backens ohne Kompression:
        // X(b) = X(p) B(p)^-1 (B M B^-1) B
        std::vector<BoneMatrix> welt;
        float schlimm = 0.0F;
        for (const int f : {0, 7, 33, 59}) {
            rigWelt(quelle, red, f, f, 0.0F, welt);
            std::vector<BoneMatrix> soll(quelle.bones.size(), BoneMatrix::identity());
            for (const int b : quelle.topologicalOrder()) {
                const GlaBone& kb = quelle.bones[static_cast<std::size_t>(b)];
                BoneMatrix m = BoneMatrix::identity();
                (void)rigWert(red, b, static_cast<float>(f), m);
                const BoneMatrix a = kb.basePose * m * kb.basePoseInv;
                soll[static_cast<std::size_t>(b)] = kb.parent < 0 ? a * kb.basePose
                                                                  : soll[static_cast<std::size_t>(kb.parent)] * quelle.bones[static_cast<std::size_t>(kb.parent)].basePoseInv * a * kb.basePose;
            }
            for (std::size_t b = 0; b < welt.size(); ++b) { schlimm = std::max(schlimm, maxDiff(welt[b], soll[b])); }
        }
        std::printf("    Ansicht der Bindebasis gegen die Rechnung des Backens: %.6f\n", static_cast<double>(schlimm));
        expect("Bindebasis in der Ansicht = Rechnung des Backens (unter 0,001)", schlimm < 1e-3F);
    }
    std::printf("%s (%d Fehler)\n", fails == 0 ? "OK" : "FEHLER", fails);
    return fails == 0 ? 0 : 1;
}
