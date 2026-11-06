// modell_rig.cpp - Modell-Block Stufe 3: Knochen-Erkennung, Control Rig, Schicht, Backen, Posen
//
//   Erkennung   die Skelette JKA (53) und JK2 (72) aus der Hand-Tabelle (Namen, Eltern, Grundstellung der Spiel-GLA):
//               jede Rolle und Seite wie von Hand eingetragen; vertauschte Namen (links/rechts, Oberschenkel/Fuss,
//               Kopf/Becken, Arm/Bein, Schluesselbeine mit Ecken) richtig erkannt UND als unklar gemeldet
//   Rig         Griffe, Mitnehmen (JKA Finger unter rradius, JK2 alles flach), Drehknochen-Anteil aus der Lage
//   Schicht     slerp zwischen zwei Schluesseln, davor/danach der Randschluessel
//   Backen      ohne Aenderung Byte fuer Byte; alles neu durch compressBone innerhalb der Stufen von compressBone;
//               FK-Drehung gebacken = die Welt der Schicht
//   Posen       gespiegelt einfuegen gegen die Hand-Erwartung (Drehung um Y/Z kehrt sich um, um X nicht), Bindepose,
//               Handpose offen
//
// Aufruf: modellrigtest
#include "bhed/gla.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/rig.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <map>
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

std::uint16_t u16(const std::uint8_t* p) { return static_cast<std::uint16_t>(p[0] | (p[1] << 8)); }

// Ein Skelett aus der Hand-Tabelle: Grundstellung = Verschiebung an den Ort, numFrames Bilder in der Grundstellung.
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
    // ein Eintrag: Einheitsquaternion, keine Verschiebung (MC_UnCompressQuat: roh/16383 - 2, roh/64 - 512)
    const std::uint16_t e[7] = {3 * 16383, 2 * 16383, 2 * 16383, 2 * 16383, 512 * 64, 512 * 64, 512 * 64};
    for (const std::uint16_t v : e) {
        g.bonePool.push_back(static_cast<std::uint8_t>(v & 0xFF));
        g.bonePool.push_back(static_cast<std::uint8_t>(v >> 8));
    }
    g.numFrames = bilder;
    g.indexes.assign(static_cast<std::size_t>(bilder) * g.bones.size(), 0U);
    return g;
}

const RigHandSkelett& tabelle(const char* name) {
    for (const RigHandSkelett& s : kRigHandSkelette) {
        if (std::strcmp(s.name, name) == 0) { return s; }
    }
    return kRigHandSkelette[0];
}

char seiteZeichen(Seite s) { return s == Seite::Links ? 'L' : (s == Seite::Rechts ? 'R' : 'M'); }

int knochen(const GlaAnimation& g, const char* n) { return boneIndex(g, n); }

// Trefferquote gegen die Hand-Tabelle
int treffer(const GlaAnimation& g, const Erkennung& e, const RigHandSkelett& h, std::string& fehlt) {
    int t = 0;
    for (int i = 0; i < h.zahl; ++i) {
        const KnochenErkennung& k = e.knochen[static_cast<std::size_t>(i)];
        const bool rolle = std::strcmp(rollenSchluessel(k.rolle), h.knochen[i].rolle) == 0;
        const bool seite = h.knochen[i].seite == '?' || h.knochen[i].seite == seiteZeichen(k.seite);
        if (rolle && seite) {
            ++t;
        } else {
            fehlt += g.bones[static_cast<std::size_t>(i)].name + "=" + rollenSchluessel(k.rolle) + "/" + seiteZeichen(k.seite) + " ";
        }
    }
    return t;
}

// Namen zweier Knochen tauschen
void tausche(GlaAnimation& g, const char* a, const char* b) {
    const int x = knochen(g, a);
    const int y = knochen(g, b);
    std::swap(g.bones[static_cast<std::size_t>(x)].name, g.bones[static_cast<std::size_t>(y)].name);
}

// richtig nach der Hand-Tabelle (Lage) UND unklar?
bool richtigUnklar(const GlaAnimation& g, const Erkennung& e, const RigHandSkelett& h, int i) {
    (void)g;
    const KnochenErkennung& k = e.knochen[static_cast<std::size_t>(i)];
    return std::strcmp(rollenSchluessel(k.rolle), h.knochen[i].rolle) == 0 && h.knochen[i].seite == seiteZeichen(k.seite) && k.unklar;
}

void drehungsTeil(const BoneMatrix& a, float r[3][3]) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) { r[i][j] = a.m[i][j]; }
    }
}

float maxDiff3(const BoneMatrix& a, const BoneMatrix& b) {
    float d = 0.0F;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) { d = std::max(d, std::fabs(a.m[i][j] - b.m[i][j])); }
    }
    return d;
}

float ortDiff(const BoneMatrix& a, const BoneMatrix& b) {
    return std::sqrt((a.m[0][3] - b.m[0][3]) * (a.m[0][3] - b.m[0][3]) + (a.m[1][3] - b.m[1][3]) * (a.m[1][3] - b.m[1][3]) +
                     (a.m[2][3] - b.m[2][3]) * (a.m[2][3] - b.m[2][3]));
}

}  // namespace

int main() {
    // --- Erkennung gegen die Hand-Tabelle ----------------------------------------------------------------------------
    for (const char* sk : {"jka", "jk2", "droideka", "bantha"}) {
        const RigHandSkelett& h = tabelle(sk);
        const GlaAnimation g = skelett(h, 1);
        const Erkennung e = erkenneKnochen(g);
        std::string fehlt;
        const int t = treffer(g, e, h, fehlt);
        std::printf("  Erkennung %s: %d / %d, unklar %d %s\n", sk, t, h.zahl, e.unklar, fehlt.c_str());
        expect(std::string("Erkennung ") + sk + " alle Knochen wie die Hand-Tabelle", t == h.zahl);
        expect(std::string("Erkennung ") + sk + " ohne unklare Knochen", e.unklar == 0);
        expect(std::string("Seitenachse ") + sk + " X, links positiv", e.seitenAchse == 0 && e.linksVorzeichen > 0.0F);
    }
    // --- Spiegelnamen ------------------------------------------------------------------------------------------------
    expect("spiegelName lhumerus", spiegelName("lhumerus") == "rhumerus");
    expect("spiegelName rfemurYZ", spiegelName("rfemurYZ") == "lfemurYZ");
    expect("spiegelName r_d1_j1", spiegelName("r_d1_j1") == "l_d1_j1");
    expect("spiegelName L_thigh", spiegelName("L_thigh") == "R_thigh");
    expect("spiegelName leg_left_bone01", spiegelName("leg_left_bone01") == "leg_right_bone01");
    expect("spiegelName lhang_tag_bone", spiegelName("lhang_tag_bone") == "rhang_tag_bone");
    expect("spiegelName pelvis leer", spiegelName("pelvis").empty() && spiegelName("lower_lumbar").empty() && spiegelName("radius").empty());
    {
        const RigHandSkelett& h = tabelle("jka");
        const GlaAnimation g = skelett(h, 1);
        const Erkennung e = erkenneKnochen(g);
        const std::vector<int> p = spiegelPartner(g, e);
        bool alle = true;
        for (int i = 0; i < h.zahl; ++i) {
            const std::string n = g.bones[static_cast<std::size_t>(i)].name;
            const std::string sp = spiegelName(n);
            const int erwartet = sp.empty() ? i : knochen(g, sp.c_str());
            if (p[static_cast<std::size_t>(i)] != erwartet) {
                std::printf("    Partner %s -> %d\n", n.c_str(), p[static_cast<std::size_t>(i)]);
                alle = false;
            }
        }
        expect("JKA Spiegelpartner aller 53 Knochen", alle);
    }
    // --- Rot: vertauschte Namen ----------------------------------------------------------------------------------------
    {
        const RigHandSkelett& h = tabelle("jka");
        struct Fall {
            const char* a;
            const char* b;
        };
        for (const Fall f : {Fall{"lhumerus", "rhumerus"}, Fall{"rfemurYZ", "rtalus"}, Fall{"cranium", "pelvis"}, Fall{"rhumerus", "rfemurYZ"},
                             Fall{"lhand", "rtalus"}}) {
            GlaAnimation g = skelett(h, 1);
            const int x = knochen(g, f.a);
            const int y = knochen(g, f.b);
            tausche(g, f.a, f.b);
            const Erkennung e = erkenneKnochen(g);
            const bool ok = richtigUnklar(g, e, h, x) && richtigUnklar(g, e, h, y);
            std::printf("    vertauscht %s/%s: %s=%s/%c%s %s=%s/%c%s\n", f.a, f.b, f.a, rollenSchluessel(e.knochen[static_cast<std::size_t>(x)].rolle),
                        seiteZeichen(e.knochen[static_cast<std::size_t>(x)].seite), e.knochen[static_cast<std::size_t>(x)].unklar ? " unklar" : "", f.b,
                        rollenSchluessel(e.knochen[static_cast<std::size_t>(y)].rolle), seiteZeichen(e.knochen[static_cast<std::size_t>(y)].seite),
                        e.knochen[static_cast<std::size_t>(y)].unklar ? " unklar" : "");
            expect(std::string("vertauscht ") + f.a + "/" + f.b + ": nach Lage richtig und unklar", ok);
        }
        // Schluesselbeine liegen bei x = 0: nur ueber die Ecken, die sie tragen
        GlaAnimation g = skelett(h, 1);
        const int lc = knochen(g, "lclavical");
        const int rc = knochen(g, "rclavical");
        tausche(g, "lclavical", "rclavical");
        Inhalt in;
        Teil t;
        t.name = "torso";
        Netz netz;
        std::vector<Ecke> ecken;
        for (const auto& [kn, x] : {std::pair<int, float>{lc, 4.0F}, {rc, -4.0F}}) {
            for (int i = 0; i < 3; ++i) {
                Ecke ek;
                ek.ort[0] = x;
                ek.ort[1] = static_cast<float>(i);
                ek.ort[2] = 54.0F;
                ek.knochen[0] = static_cast<std::uint8_t>(kn);
                ek.gewicht[0] = 1.0F;
                ecken.push_back(ek);
            }
        }
        netz.aufbauen(ecken, {0, 1, 2, 3, 4, 5});
        t.stufen.push_back(netz);
        in.teile.push_back(t);
        const Erkennung ohne = erkenneKnochen(g);
        const Erkennung mit = erkenneKnochen(g, {}, &in, 0);
        expect("vertauschte Schluesselbeine ohne Ecken nicht erkennbar (Lage x = 0)", !richtigUnklar(g, ohne, h, lc));
        expect("vertauschte Schluesselbeine ueber die Ecken richtig und unklar", richtigUnklar(g, mit, h, lc) && richtigUnklar(g, mit, h, rc));
        // von Hand zugeordnet gilt
        const Erkennung hand = erkenneKnochen(g, {Zuordnung{lc, Rolle::Schluessel, Seite::Links}}, nullptr, 0);
        expect("Zuordnung von Hand gilt", hand.knochen[static_cast<std::size_t>(lc)].vonHand && hand.knochen[static_cast<std::size_t>(lc)].seite == Seite::Links);
    }
    // --- Ohne Beine (wie Marka Ragnos): die Arme bleiben Arme --------------------------------------------------------
    {
        const RigHandSkelett& h = tabelle("jka");
        const GlaAnimation voll = skelett(h, 1);
        GlaAnimation g;
        std::vector<int> neuIndex(voll.bones.size(), -1);
        for (std::size_t b = 0; b < voll.bones.size(); ++b) {
            const std::string r = h.knochen[b].rolle;
            if (r == "thigh" || r == "thigh_twist" || r == "calf" || r == "foot" || r == "tail") { continue; }
            neuIndex[b] = static_cast<int>(g.bones.size());
            g.bones.push_back(voll.bones[b]);
        }
        for (GlaBone& b : g.bones) { b.parent = b.parent >= 0 ? neuIndex[static_cast<std::size_t>(b.parent)] : -1; }
        g.numFrames = 1;
        const Erkennung e = erkenneKnochen(g);
        int arme = 0;
        for (const KnochenErkennung& k : e.knochen) { arme += (armRolle(k.rolle) && !k.unklar) ? 1 : 0; }
        std::printf("    ohne Beine: %d Armknochen erkannt, unklar %d\n", arme, e.unklar);
        expect("ohne Beine: jeder Armknochen bleibt Arm (keiner unklar)", e.unklar == 0 && arme == 2 * (1 + 1 + 1 + 1 + 1 + 1 + 6));
    }
    // --- Rig ---------------------------------------------------------------------------------------------------------
    const RigHandSkelett& hj = tabelle("jka");
    GlaAnimation g = skelett(hj, 40);
    const Inhalt leer;
    const Rig rig = rigAnlegen(g, leer, 0, {});
    int hand = -1;
    int oberarm = -1;
    for (std::size_t i = 0; i < rig.griffe.size(); ++i) {
        if (rig.griffe[i].art == GriffArt::Hand && rig.griffe[i].seite == Seite::Rechts) { hand = static_cast<int>(i); }
        if (rig.griffe[i].art == GriffArt::Oberarm && rig.griffe[i].seite == Seite::Rechts) { oberarm = static_cast<int>(i); }
    }
    expect("Rig: rechte Hand und rechter Oberarm", hand >= 0 && oberarm >= 0);
    if (hand < 0 || oberarm < 0) { return 1; }
    const Griff& gh = rig.griffe[static_cast<std::size_t>(hand)];
    expect("JKA Hand nimmt 6 Finger und rhang_tag_bone mit", gh.mit.size() == 7U);
    expect("JKA Hand: Drehknochen rradiusX", gh.twist == knochen(g, "rradiusX") && gh.twistVon == knochen(g, "rradius"));
    std::printf("    Drehknochen-Anteil rradiusX %.4f\n", static_cast<double>(gh.twistAnteil));
    expect("Drehknochen-Anteil = Lage auf der Strecke (0,49..0,51)", gh.twistAnteil > 0.49F && gh.twistAnteil < 0.51F);
    expect("JKA Oberarm nimmt nichts mit (alles haengt unter rhumerus)", rig.griffe[static_cast<std::size_t>(oberarm)].mit.empty());
    int pole = 0;
    for (const Griff& x : rig.griffe) { pole += (x.art == GriffArt::PolEllbogen || x.art == GriffArt::PolKnie) ? 1 : 0; }
    expect("Pol-Ziele fuer beide Ellbogen und Knie", pole == 4);
    {
        const RigHandSkelett& h2 = tabelle("jk2");
        const GlaAnimation g2 = skelett(h2, 1);
        const Rig r2 = rigAnlegen(g2, leer, 0, {});
        std::size_t mit = 0;
        for (const Griff& x : r2.griffe) {
            if (x.art == GriffArt::Oberarm && x.seite == Seite::Rechts) { mit = x.mit.size(); }
        }
        // JK2: rhumerusX, rradius, rradiusX, rhand, mc7, 15 Finger, rhang_tag_bone haengen alle an thoracic
        std::printf("    JK2 Oberarm nimmt %d Knochen mit\n", static_cast<int>(mit));
        expect("JK2 (flach): der Oberarm nimmt den ganzen Arm mit (21)", mit == 21U);
    }
    // --- Schicht -----------------------------------------------------------------------------------------------------
    {
        RigSchicht s;
        const float z[3] = {0.0F, 0.0F, 1.0F};
        rigSchluesselSetzen(s, 5, 0, BoneMatrix::identity());
        rigSchluesselSetzen(s, 5, 10, drehungUm(z, 90.0F));
        BoneMatrix m;
        (void)rigWert(s, 5, 5.0F, m);
        expect("slerp: halber Weg = 45 Grad", maxDiff3(m, drehungUm(z, 45.0F)) < 1e-5F);
        (void)rigWert(s, 5, 20.0F, m);
        expect("nach dem letzten Schluessel gilt er", maxDiff3(m, drehungUm(z, 90.0F)) < 1e-5F);
        rigSchluesselSetzen(s, 5, 10, BoneMatrix::identity());
        expect("Schluessel ersetzen am selben Bild", s.schluesselZahl() == 2);
    }
    // --- Quelle: eine bewegte Animation (Schluessel gebacken) --------------------------------------------------------
    GlaAnimation quelle;
    {
        RigSchicht s;
        std::uint32_t zufall = 12345U;
        const auto naechste = [&]() {
            zufall = zufall * 1664525U + 1013904223U;
            return static_cast<float>(zufall >> 8) / static_cast<float>(1U << 24);
        };
        for (int b = 0; b < hj.zahl; ++b) {
            for (const int f : {0, 13, 26, 39}) {
                const float achse[3] = {naechste() - 0.5F, naechste() - 0.5F, naechste() - 0.5F};
                BoneMatrix m = drehungUm(achse, (naechste() - 0.5F) * 80.0F);
                if (b == 1) { m.m[2][3] = (naechste() - 0.5F) * 6.0F; }   // das Becken bewegt sich
                rigSchluesselSetzen(s, b, f, m);
            }
        }
        BackBericht br;
        expect("Quelle backen", rigBacken(g, s, 0, 40, true, "quelle", quelle, &br));
        std::printf("    Quelle: %d Bilder, Vorrat %d\n", br.bilder, br.vorrat);
    }
    // --- Backen: Rundlauf -------------------------------------------------------------------------------------------
    {
        const RigSchicht leerS;
        GlaAnimation aus;
        BackBericht br;
        expect("ohne Aenderung backen", rigBacken(quelle, leerS, 0, 40, false, "aus", aus, &br));
        bool gleich = true;
        for (int f = 0; f < 40 && gleich; ++f) {
            for (int b = 0; b < hj.zahl; ++b) {
                const std::size_t at = static_cast<std::size_t>(f) * static_cast<std::size_t>(hj.zahl) + static_cast<std::size_t>(b);
                gleich = gleich && std::memcmp(quelle.bonePool.data() + quelle.indexes[at] * 14U, aus.bonePool.data() + aus.indexes[at] * 14U, 14U) == 0;
            }
        }
        expect("ohne Aenderung: jedes Knochen-Bild Byte fuer Byte (14 Bytes)", gleich && br.unveraendert == 40 * hj.zahl && br.neu == 0);
        GlaAnimation neu;
        expect("alles neu durch compressBone", rigBacken(quelle, leerS, 0, 40, true, "neu", neu, &br));
        int maxStufe = 0;
        int maxVerschiebung = 0;
        for (std::size_t at = 0; at < neu.indexes.size(); ++at) {
            const std::uint8_t* a = quelle.bonePool.data() + quelle.indexes[at] * 14U;
            const std::uint8_t* c = neu.bonePool.data() + neu.indexes[at] * 14U;
            int plus = 0;
            int minus = 0;
            for (int k = 0; k < 4; ++k) {
                plus = std::max(plus, std::abs(static_cast<int>(u16(a + 2 * k)) - static_cast<int>(u16(c + 2 * k))));
                // q und -q sind dieselbe Drehung: -x = (4 - (x + 2)) - 2 -> roh' = 4*16383 - roh
                minus = std::max(minus, std::abs(static_cast<int>(u16(a + 2 * k)) - (4 * 16383 - static_cast<int>(u16(c + 2 * k)))));
            }
            maxStufe = std::max(maxStufe, std::min(plus, minus));
            for (int k = 4; k < 7; ++k) { maxVerschiebung = std::max(maxVerschiebung, std::abs(static_cast<int>(u16(a + 2 * k)) - static_cast<int>(u16(c + 2 * k)))); }
        }
        std::printf("    alles neu: groesste Abweichung %d Stufen je Quaternion-Anteil, Verschiebung %d Stufen\n", maxStufe, maxVerschiebung);
        expect("alles neu: Quaternion hoechstens 2 Stufen je Anteil (je 0,5 Runden + 1 Suche optimizeQuat)", maxStufe <= 2);
        expect("alles neu: Verschiebung unveraendert (Vielfache von 1/64 bleiben)", maxVerschiebung == 0);
    }
    // --- FK: Hand drehen, backen, gegen die Welt der Schicht --------------------------------------------------------
    {
        RigSchicht s;
        std::vector<BoneMatrix> vorher;
        rigWelt(quelle, s, 20, 20, 0.0F, vorher);
        const float z[3] = {0.0F, 0.0F, 1.0F};
        const float nichts[3] = {0.0F, 0.0F, 0.0F};
        // Drehung um die Achse des Unterarms: Twist
        float achse[3];
        const int rr = knochen(quelle, "rradius");
        const int rh = knochen(quelle, "rhand");
        for (int k = 0; k < 3; ++k) { achse[k] = vorher[static_cast<std::size_t>(rh)].m[k][3] - vorher[static_cast<std::size_t>(rr)].m[k][3]; }
        rigGriffDrehen(quelle, rig, hand, 20, vorher, drehungUm(achse, 40.0F), nichts, s);
        std::vector<BoneMatrix> nachher;
        rigWelt(quelle, s, 20, 20, 0.0F, nachher);
        const int f1 = knochen(quelle, "r_d1_j1");
        const int rx = knochen(quelle, "rradiusX");
        // der Finger dreht um den Ursprung der Hand mit
        BoneMatrix um = drehungUm(achse, 40.0F);
        float p[3];
        float h0[3] = {vorher[static_cast<std::size_t>(rh)].m[0][3], vorher[static_cast<std::size_t>(rh)].m[1][3], vorher[static_cast<std::size_t>(rh)].m[2][3]};
        float d[3] = {vorher[static_cast<std::size_t>(f1)].m[0][3] - h0[0], vorher[static_cast<std::size_t>(f1)].m[1][3] - h0[1],
                      vorher[static_cast<std::size_t>(f1)].m[2][3] - h0[2]};
        um.transform(d, p);
        const float fehler = std::sqrt((p[0] + h0[0] - nachher[static_cast<std::size_t>(f1)].m[0][3]) * (p[0] + h0[0] - nachher[static_cast<std::size_t>(f1)].m[0][3]) +
                                       (p[1] + h0[1] - nachher[static_cast<std::size_t>(f1)].m[1][3]) * (p[1] + h0[1] - nachher[static_cast<std::size_t>(f1)].m[1][3]) +
                                       (p[2] + h0[2] - nachher[static_cast<std::size_t>(f1)].m[2][3]) * (p[2] + h0[2] - nachher[static_cast<std::size_t>(f1)].m[2][3]));
        expect("FK Hand: der Finger unter rradius dreht mit um die Hand", fehler < 1e-3F);
        expect("FK Hand: die Hand bleibt am Ort", ortDiff(vorher[static_cast<std::size_t>(rh)], nachher[static_cast<std::size_t>(rh)]) < 1e-3F);
        // rradiusX: Anteil * 40 Grad um dieselbe Achse
        const BoneMatrix dx = nachher[static_cast<std::size_t>(rx)] * starrInvers(vorher[static_cast<std::size_t>(rx)]);
        float q[4];
        quatAusMatrix(dx, q);
        const float l = std::sqrt(achse[0] * achse[0] + achse[1] * achse[1] + achse[2] * achse[2]);
        const float grad = 2.0F * std::atan2((q[1] * achse[0] + q[2] * achse[1] + q[3] * achse[2]) / l, q[0]) * 57.29578F;
        std::printf("    rradiusX dreht %.3f Grad (Anteil %.4f * 40 = %.3f)\n", static_cast<double>(grad), static_cast<double>(gh.twistAnteil),
                    static_cast<double>(gh.twistAnteil * 40.0F));
        expect("FK Hand: rradiusX nimmt den Anteil der Drehung", std::fabs(grad - gh.twistAnteil * 40.0F) < 0.05F);
        // Oberarm um Z drehen und backen
        rigGriffDrehen(quelle, rig, oberarm, 20, nachher, drehungUm(z, 25.0F), nichts, s);
        std::vector<BoneMatrix> soll;
        rigWelt(quelle, s, 20, 20, 0.0F, soll);
        GlaAnimation aus;
        BackBericht br;
        expect("FK gebacken", rigBacken(quelle, s, 0, 40, false, "fk", aus, &br));
        std::vector<BoneMatrix> ist;
        aus.worldMatrices(20, ist);
        float maxOrt = 0.0F;
        for (std::size_t b = 0; b < ist.size(); ++b) { maxOrt = std::max(maxOrt, ortDiff(ist[b], soll[b])); }
        std::printf("    FK gebacken gegen die Schicht: groesste Ortsabweichung %.5f Einheiten (%d neu, %d unveraendert)\n", static_cast<double>(maxOrt),
                    br.neu, br.unveraendert);
        // Grenze: je Knochen der Kette (bis 8) eine halbe Verschiebungsstufe je Achse (sqrt(3)/128) und 2 Quaternionstufen
        // (Drehfehler <= 4 * 2/16383 rad) am Hebel der Figur (<= 65 Einheiten).
        const float grenze = 8.0F * (std::sqrt(3.0F) / 128.0F + 4.0F * 2.0F / 16383.0F * 65.0F);
        expect("FK gebacken = die Welt der Schicht (innerhalb der Kompression)", maxOrt < grenze);
    }
    // --- Posen: gespiegelt einfuegen ----------------------------------------------------------------------------------
    {
        RigSchicht s;
        std::vector<BoneMatrix> w0;
        rigWelt(g, s, 0, 0, 0.0F, w0);   // Grundstellung
        const float y[3] = {0.0F, 1.0F, 0.0F};
        const float x[3] = {1.0F, 0.0F, 0.0F};
        const float nichts[3] = {0.0F, 0.0F, 0.0F};
        rigGriffDrehen(g, rig, oberarm, 0, w0, drehungUm(y, 30.0F), nichts, s);
        std::vector<BoneMatrix> w1;
        rigWelt(g, s, 0, 0, 0.0F, w1);
        int unterarm = -1;
        for (std::size_t i = 0; i < rig.griffe.size(); ++i) {
            if (rig.griffe[i].art == GriffArt::Unterarm && rig.griffe[i].seite == Seite::Rechts) { unterarm = static_cast<int>(i); }
        }
        rigGriffDrehen(g, rig, unterarm, 0, w1, drehungUm(x, 20.0F), nichts, s);
        std::vector<BoneMatrix> w2;
        rigWelt(g, s, 0, 0, 0.0F, w2);
        const RigPose pose = poseAus(g, w2);
        expect("gespiegelt einfuegen", poseEinfuegen(g, rig, pose, true, 5, {}, s) == hj.zahl);
        std::vector<BoneMatrix> w3;
        rigWelt(g, s, 5, 5, 0.0F, w3);
        const int lh = knochen(g, "lhumerus");
        const int lr = knochen(g, "lradius");
        const int lhand = knochen(g, "lhand");
        const int rhand = knochen(g, "rhand");
        // Hand-Erwartung: um Y (vorn-hinten) kehrt sich die Drehung um, um X (Seitenachse) nicht
        const BoneMatrix dOber = w3[static_cast<std::size_t>(lh)] * g.bones[static_cast<std::size_t>(lh)].basePoseInv;
        expect("gespiegelt: lhumerus um Y -30 Grad", maxDiff3(dOber, drehungUm(y, -30.0F)) < 1e-4F);
        const BoneMatrix dUnter = w3[static_cast<std::size_t>(lr)] * g.bones[static_cast<std::size_t>(lr)].basePoseInv;
        expect("gespiegelt: lradius = Rx(+20) * Ry(-30)", maxDiff3(dUnter, drehungUm(x, 20.0F) * drehungUm(y, -30.0F)) < 1e-4F);
        const float spiegelFehler = std::fabs(w3[static_cast<std::size_t>(lhand)].m[0][3] + w2[static_cast<std::size_t>(rhand)].m[0][3]) +
                                    std::fabs(w3[static_cast<std::size_t>(lhand)].m[1][3] - w2[static_cast<std::size_t>(rhand)].m[1][3]) +
                                    std::fabs(w3[static_cast<std::size_t>(lhand)].m[2][3] - w2[static_cast<std::size_t>(rhand)].m[2][3]);
        expect("gespiegelt: lhand liegt am gespiegelten Ort der rhand", spiegelFehler < 1e-3F);
        // Bindepose
        expect("Bindepose", bindePose(quelle, 7, {}, s) == hj.zahl);
        std::vector<BoneMatrix> wb;
        rigWelt(quelle, s, 7, 7, 0.0F, wb);
        float bd = 0.0F;
        for (std::size_t b = 0; b < wb.size(); ++b) {
            bd = std::max(bd, maxDiff3(wb[b], quelle.bones[b].basePose));
            bd = std::max(bd, ortDiff(wb[b], quelle.bones[b].basePose));
        }
        expect("Bindepose: jede Weltmatrix = Grundstellung", bd < 1e-3F);
        // Handpose offen
        RigSchicht s2;
        const int gesetzt = handPoseSetzen(quelle, rig, Seite::Rechts, -1, 9, s2);
        std::vector<BoneMatrix> wh;
        rigWelt(quelle, s2, 9, 9, 0.0F, wh);
        const int f2 = knochen(quelle, "r_d2_j2");
        const BoneMatrix rel = starrInvers(wh[static_cast<std::size_t>(rhand)]) * wh[static_cast<std::size_t>(f2)];
        const BoneMatrix relBinde = quelle.bones[static_cast<std::size_t>(rhand)].basePoseInv * quelle.bones[static_cast<std::size_t>(f2)].basePose;
        expect("Handpose offen: 6 Finger relativ zur Hand wie in der Bindepose", gesetzt == 6 && maxDiff3(rel, relBinde) < 1e-4F && ortDiff(rel, relBinde) < 1e-3F);
    }
    // --- Pfad ---------------------------------------------------------------------------------------------------------
    {
        RigSchicht s;
        const std::vector<std::array<float, 3>> pfad = bewegungsPfad(quelle, s, knochen(quelle, "rhand"), 10, 19);
        std::vector<BoneMatrix> w;
        quelle.worldMatrices(15, w);
        const int rh = knochen(quelle, "rhand");
        expect("Bewegungspfad: 10 Punkte, Bild 15 = Weltort", pfad.size() == 10U && std::fabs(pfad[5][0] - w[static_cast<std::size_t>(rh)].m[0][3]) < 1e-5F &&
                                                                std::fabs(pfad[5][2] - w[static_cast<std::size_t>(rh)].m[2][3]) < 1e-5F);
    }
    std::printf("%s (%d)\n", fails == 0 ? "modell_rig: alles ok" : "modell_rig: FEHLER", fails);
    return fails == 0 ? 0 : 1;
}
