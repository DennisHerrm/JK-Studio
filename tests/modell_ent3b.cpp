// modell_ent3b.cpp - ENTSCHEID3B (plaene/entscheidungen_1211.md B14, B17): aeltere Missions-Daten, IK/FK-Spur, Plattform
//
//   alt        tests/figuranim/ent3b_figur_alt.txt kommt aus dem Programm VOR ENTSCHEID3B (karte1 fe0cbece7, figurZeilen): zwei
//              IK-Spuren mit dem Anteil im IK-Schluessel (bis rc612: gilt bis zum naechsten), dazu je Zehntel-Bild die Weltmatrizen,
//              die das alte Programm daraus rechnete. Geladen mit dem neuen Programm muss jede Matrix gleich sein (< 1e-6).
//
// Aufruf: modellent3btest <tests/figuranim/ent3b_figur_alt.txt>
//         modellent3btest --alt-schreiben <datei>   schreibt die Datei (nur mit dem alten Programm sinnvoll)
#include "bhed/gla.h"
#include "bhed/modell/animkurven.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/figuranim.h"
#include "bhed/modell/ik.h"
#include "bhed/modell/rig.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
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

void eintrag(std::vector<std::uint8_t>& pool, float tx) {
    const std::uint16_t e[7] = {3 * 16383, 2 * 16383, 2 * 16383, 2 * 16383, static_cast<std::uint16_t>((tx + 512.0F) * 64.0F), 512 * 64, 512 * 64};
    for (const std::uint16_t v : e) {
        pool.push_back(static_cast<std::uint8_t>(v & 0xFF));
        pool.push_back(static_cast<std::uint8_t>(v >> 8));
    }
}

// Das JKA-Skelett der Hand-Tabelle (wie modell_figuranim); das Becken geht in Bild f um 0,5 f Einheiten entlang X
GlaAnimation skelett(int bilder) {
    const RigHandSkelett& h = tabelle("jka");
    GlaAnimation g;
    g.name = "_humanoid";
    g.skeletonName = "models/players/_humanoid/_humanoid";
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
    const int becken = boneIndex(g, "pelvis");
    eintrag(g.bonePool, 0.0F);
    for (int f = 0; f < bilder; ++f) { eintrag(g.bonePool, 0.5F * static_cast<float>(f)); }
    g.numFrames = bilder;
    g.indexes.assign(static_cast<std::size_t>(bilder) * g.bones.size(), 0U);
    for (int f = 0; f < bilder && becken >= 0; ++f) {
        g.indexes[static_cast<std::size_t>(f) * g.bones.size() + static_cast<std::size_t>(becken)] = static_cast<std::uint32_t>(f + 1);
    }
    return g;
}

constexpr int kBilder = 30;

std::size_t uz(int i) { return static_cast<std::size_t>(i); }

// Die Knochen, deren Matrizen die Datei traegt: Ober-/Unterarm, Hand rechts; Ober-/Unterschenkel, Fuss links
std::vector<int> pruefKnochen(const GlaAnimation& g) {
    std::vector<int> k;
    for (const char* n : {"rhumerus", "rradius", "rhand", "lfemurYZ", "ltibia", "ltalus"}) { k.push_back(boneIndex(g, n)); }
    return k;
}

// Die alte Schicht: Arm R mit einem IK-Schluessel je Bild (Ziel aus der FK-Pose, dann 6 Einheiten hoch - IK und FK verschieden),
// Anteil 1, an den Bildern 10 bis 19 Anteil 0 (FK); Bein L mit drei Schluesseln (0: Anteil 1, 15: 0,5, 25: 1), Ziel 3 Einheiten vor
FigurAnim alteFigur(const GlaAnimation& g, const Rig& rig) {
    FigurAnim a;
    a.figur = "ent3b_alt";
    a.basis = "BOTH_STAND1";
    a.bilder = kBilder;
    a.schicht.knochen.assign(g.bones.size(), {});
    a.schicht.neuInterp = Interp::Bezier;
    IkSpur arm;
    IkSpur bein;
    if (!ikSpurAnlegen(g, rig, IkArt::ArmRechts, arm) || !ikSpurAnlegen(g, rig, IkArt::BeinLinks, bein)) { return a; }
    a.schicht.ik.push_back(arm);
    a.schicht.ik.push_back(bein);
    ikAufFkBereich(g, a.schicht, 0, 0, kBilder - 1, 1.0F);
    IkSpur& sa = a.schicht.ik[0];
    for (std::size_t i = 0; i < sa.ziel.size(); ++i) {
        sa.ziel[i].t[2] += 6.0F;
        if (sa.ziel[i].bild >= 10 && sa.ziel[i].bild <= 19) { sa.wert[i].ik = 0.0F; }
    }
    ikHenkelNeu(sa);
    for (const auto& [bild, anteil] : {std::pair{0, 1.0F}, std::pair{15, 0.5F}, std::pair{25, 1.0F}}) { ikAufFk(g, a.schicht, 1, bild, anteil); }
    IkSpur& sb = a.schicht.ik[1];
    for (RigSchluessel& k : sb.ziel) { k.t[1] -= 3.0F; }
    ikHenkelNeu(sb);
    return a;
}

std::string zahl(float v) {
    char b[32];
    std::snprintf(b, sizeof b, "%.9g", static_cast<double>(v));
    return b;
}

// Je Zehntel-Bild 0 .. kBilder-1 die Matrizen der Pruefknochen als Zeile "// erwartet <bild> m00 ... m23 ..."
std::vector<std::string> erwartetZeilen(const GlaAnimation& g, const RigSchicht& s) {
    std::vector<std::string> z;
    const std::vector<int> k = pruefKnochen(g);
    std::vector<BoneMatrix> w;
    for (int i = 0; i <= (kBilder - 1) * 10; ++i) {
        const int f = i / 10;
        const float t = static_cast<float>(i % 10) / 10.0F;
        rigWelt(g, s, f, std::min(f + 1, kBilder - 1), t, w);
        std::string l = "// erwartet " + std::to_string(i);
        for (const int b : k) {
            for (const auto& r : w[static_cast<std::size_t>(b)].m) {
                for (const float v : r) { l += " " + zahl(v); }
            }
        }
        z.push_back(l);
    }
    return z;
}

std::vector<std::string> zeilenAus(const std::string& pfad) {
    std::vector<std::string> z;
    std::ifstream f(pfad, std::ios::binary);
    std::string l;
    while (std::getline(f, l)) {
        if (!l.empty() && l.back() == '\r') { l.pop_back(); }
        z.push_back(l);
    }
    return z;
}

}  // namespace

int main(int argc, char** argv) {
    const GlaAnimation g = skelett(kBilder);
    const Inhalt leer;
    const Rig rig = rigAnlegen(g, leer, 0, {});
    if (argc >= 3 && std::string(argv[1]) == "--alt-schreiben") {
        const FigurAnim a = alteFigur(g, rig);
        std::ofstream o(argv[2], std::ios::binary);
        o << "// ENTSCHEID3B: Missions-Daten einer Figur aus dem Programm vor ENTSCHEID3B (karte1 fe0cbece7, figurZeilen) und die Weltmatrizen,\n"
             "// die dieses Programm je Zehntel-Bild daraus rechnete (rhumerus rradius rhand lfemurYZ ltibia ltalus, je 12 Zahlen).\n";
        for (const std::string& l : figurZeilen(a)) { o << l << "\n"; }
        for (const std::string& l : erwartetZeilen(g, a.schicht)) { o << l << "\n"; }
        std::printf("geschrieben: %s (%zu IK-Spuren)\n", argv[2], a.schicht.ik.size());
        return a.schicht.ik.size() == 2U ? 0 : 1;
    }
    if (argc < 2) {
        std::printf("Aufruf: modellent3btest <ent3b_figur_alt.txt>\n");
        return 2;
    }
    // --- B14: die alte Datei laedt unveraendert und rechnet wie bisher ----------------------------------------------------
    const std::vector<std::string> alle = zeilenAus(argv[1]);
    std::vector<std::string> figur;
    std::vector<std::vector<float>> erwartet;
    for (const std::string& l : alle) {
        if (istFigurMarke(l) || istFigurDaten(l)) {
            figur.push_back(l);
        } else if (l.rfind("// erwartet ", 0) == 0) {
            std::istringstream is(l.substr(12));
            int nr = 0;
            is >> nr;
            std::vector<float> v;
            float x = 0.0F;
            while (is >> x) { v.push_back(x); }
            erwartet.push_back(v);
        }
    }
    FigurAnim a;
    std::string fehler;
    const bool gelesen = figurAusZeilen(figur, g, rig, a, &fehler);
    expect("alt: die Datei liest sich (" + std::to_string(figur.size()) + " Zeilen, " + std::to_string(erwartet.size()) + " Zehntel-Bilder erwartet)" + (fehler.empty() ? "" : ": " + fehler),
           gelesen && a.schicht.ik.size() == 2U && erwartet.size() == static_cast<std::size_t>((kBilder - 1) * 10 + 1));
    {
        const std::vector<std::string> neu = erwartetZeilen(g, a.schicht);
        float d = 0.0F;
        std::size_t n = 0;
        for (std::size_t i = 0; i < neu.size() && i < erwartet.size(); ++i) {
            std::istringstream is(neu[i].substr(12));
            int nr = 0;
            is >> nr;
            float x = 0.0F;
            std::size_t j = 0;
            while (is >> x) {
                if (j < erwartet[i].size()) { d = std::max(d, std::fabs(x - erwartet[i][j])); }
                ++j;
                ++n;
            }
        }
        std::printf("    alt: %zu Zahlen gegen das alte Programm, groesster Unterschied %.3g\n", n, static_cast<double>(d));
        expect("alt: jede Weltmatrix an jedem Zehntel-Bild wie im alten Programm (< 1e-6)", n > 0 && d < 1e-6F);
    }
    if (a.schicht.ik.size() != 2U) {
        std::printf("FEHLER (%d Fehler)\n", fails + 1);
        return 1;
    }
    // --- B14: die Anteile der alten Datei sind Schluessel der IK/FK-Spur mit Interpolation Konstant (dieselbe Treppe) ------------
    {
        bool konstant = true;
        for (const IkSpur& sp : a.schicht.ik) {
            for (const RigSchluessel& k : sp.anteil) { konstant = konstant && k.interp == Interp::Konstant; }
        }
        std::printf("    alt: Schluessel der IK/FK-Spur Arm %zu, Bein %zu\n", a.schicht.ik[0].anteil.size(), a.schicht.ik[1].anteil.size());
        expect("alt: Arm 3 Schluessel (0: IK, 10: FK, 20: IK), Bein 3 (0: 1, 15: 0,5, 25: 1), alle Konstant", konstant && a.schicht.ik[0].anteil.size() == 3U &&
                                                                                                          a.schicht.ik[1].anteil.size() == 3U &&
                                                                                                          ikAnteilAm(a.schicht.ik[1], 20.0F) == 0.5F);
        // neu gespeichert: eine Zeile ikfk je Glied, gelesen dasselbe
        const std::vector<std::string> z = figurZeilen(a);
        int ikfk = 0;
        for (const std::string& l : z) { ikfk += l.find(" ikfk ") != std::string::npos ? 1 : 0; }
        FigurAnim b;
        const bool ok = figurAusZeilen(z, g, rig, b, nullptr);
        expect("neu: je Glied eine Zeile ikfk, wieder gelesen jede Matrix gleich", ok && ikfk == 2 && erwartetZeilen(g, b.schicht) == erwartetZeilen(g, a.schicht));
    }
    // --- B14: zwischen zwei Schluesseln ueberblendet, ohne Sprung ------------------------------------------------------------
    {
        RigSchicht ueber = a.schicht;
        IkSpur& arm = ueber.ik[0];
        arm.anteil.clear();
        ikAnteilSetzen(arm, 5, 1.0F);
        ikAnteilSetzen(arm, 15, 0.0F);
        RigSchicht ik = ueber;
        RigSchicht fk = ueber;
        ik.ik[0].anteil.clear();
        ikAnteilSetzen(ik.ik[0], 0, 1.0F);
        fk.ik[0].anteil.clear();
        ikAnteilSetzen(fk.ik[0], 0, 0.0F);
        bool fallend = true;
        float sprungAnteil = 0.0F;
        for (int i = 50; i < 150; ++i) {
            const float x = static_cast<float>(i) / 10.0F;
            const float r0 = ikAnteilAm(arm, x);
            const float r1 = ikAnteilAm(arm, x + 0.1F);
            fallend = fallend && r1 <= r0 + 1e-6F;
            sprungAnteil = std::max(sprungAnteil, r0 - r1);
        }
        const int hand = boneIndex(g, "rhand");
        std::vector<BoneMatrix> w;
        std::vector<BoneMatrix> wi;
        std::vector<BoneMatrix> wf;
        const auto ort = [&](const RigSchicht& s, float x, std::vector<BoneMatrix>& o) {
            const int f = static_cast<int>(std::floor(x));
            rigWelt(g, s, f, std::min(f + 1, kBilder - 1), x - static_cast<float>(f), o);
        };
        ort(ueber, 5.0F, w);
        ort(ik, 5.0F, wi);
        float anfang = 0.0F;
        for (std::size_t k = 0; k < w.size(); ++k) {
            for (int r = 0; r < 3; ++r) { anfang = std::max(anfang, std::fabs(w[k].m[r][3] - wi[k].m[r][3])); }
        }
        ort(ueber, 15.0F, w);
        ort(fk, 15.0F, wf);
        float ende = 0.0F;
        for (std::size_t k = 0; k < w.size(); ++k) {
            for (int r = 0; r < 3; ++r) { ende = std::max(ende, std::fabs(w[k].m[r][3] - wf[k].m[r][3])); }
        }
        ort(ik, 10.0F, wi);
        ort(fk, 10.0F, wf);
        const float abstand = std::hypot(wi[uz(hand)].m[0][3] - wf[uz(hand)].m[0][3], wi[uz(hand)].m[1][3] - wf[uz(hand)].m[1][3], wi[uz(hand)].m[2][3] - wf[uz(hand)].m[2][3]);
        float schritt = 0.0F;
        std::vector<BoneMatrix> vor;
        ort(ueber, 5.0F, vor);
        for (int i = 51; i <= 150; ++i) {
            ort(ueber, static_cast<float>(i) / 10.0F, w);
            schritt = std::max(schritt, std::hypot(w[uz(hand)].m[0][3] - vor[uz(hand)].m[0][3], w[uz(hand)].m[1][3] - vor[uz(hand)].m[1][3], w[uz(hand)].m[2][3] - vor[uz(hand)].m[2][3]));
            vor = w;
        }
        std::printf("    ueberblenden: Anteil 10 = %.4f, groesster Anteil-Schritt je Zehntel %.4f, +5 gegen IK %.2g, +15 gegen FK %.2g, Hand IK/FK %.3f, groesster Schritt %.4f\n",
                    static_cast<double>(ikAnteilAm(arm, 10.0F)), static_cast<double>(sprungAnteil), static_cast<double>(anfang), static_cast<double>(ende), static_cast<double>(abstand),
                    static_cast<double>(schritt));
        expect("ueberblenden: der Anteil faellt stetig von 1 (+5) auf 0 (+15), in der Mitte 0,5 (Bezier, Auto Clamped)", fallend && ikAnteilAm(arm, 5.0F) == 1.0F &&
                                                                                                                        ikAnteilAm(arm, 15.0F) == 0.0F &&
                                                                                                                        std::fabs(ikAnteilAm(arm, 10.0F) - 0.5F) < 0.01F &&
                                                                                                                        sprungAnteil < 0.03F);
        expect("ueberblenden: kein Sprung - +5 die IK-Pose, +15 die FK-Pose (< 0,0001), dazwischen je Zehntel-Bild hoechstens ein Viertel des Abstands IK/FK",
               anfang < 1e-4F && ende < 1e-4F && abstand > 4.0F && schritt < abstand * 0.25F);
    }
    // --- B14: ab dem Bild umschalten - davor bleibt der alte Anteil, bis zum naechsten Wechsel der neue -------------------------
    {
        RigSchicht s = a.schicht;
        (void)ikFkUmschaltenAb(g, s, 0, 5, 0.0F, false);
        const IkSpur& arm = s.ik[0];
        std::printf("    ab Bild 5 FK: 4 -> %.3f, 5 -> %.3f, 9 -> %.3f, 19 -> %.3f, 20 -> %.3f\n", static_cast<double>(ikAnteilAm(arm, 4.0F)), static_cast<double>(ikAnteilAm(arm, 5.0F)),
                    static_cast<double>(ikAnteilAm(arm, 9.0F)), static_cast<double>(ikAnteilAm(arm, 19.0F)), static_cast<double>(ikAnteilAm(arm, 20.0F)));
        expect("ab dem Bild: bis 4 IK, ab 5 FK (auch 10 bis 19, schon FK), ab 20 wieder IK", ikAnteilAm(arm, 4.0F) == 1.0F && ikAnteilAm(arm, 5.0F) == 0.0F &&
                                                                                            ikAnteilAm(arm, 9.0F) == 0.0F && ikAnteilAm(arm, 19.0F) == 0.0F &&
                                                                                            ikAnteilAm(arm, 20.0F) == 1.0F);
    }
    // --- B17: die Plattform verschiebt und dreht die ganze Figur (mit den IK-Zielen im Raum Welt) ---------------------------------
    {
        RigSchicht s = a.schicht;
        const float z[3] = {0.0F, 0.0F, 1.0F};
        BoneMatrix p = drehungUm(z, 30.0F);
        p.m[0][3] = 10.0F;
        p.m[1][3] = -4.0F;
        plattformSetzen(s, 0, p);
        // die Knochen der Glieder unter IK (Oberarm/-schenkel und alles darunter) gegen den Rest
        std::vector<char> glied(g.bones.size(), 0);
        for (const IkSpur& sp : s.ik) {
            if (sp.oben.knochen >= 0) { glied[uz(sp.oben.knochen)] = 1; }
        }
        for (const int k : g.topologicalOrder()) {
            const int e = g.bones[uz(k)].parent;
            if (e >= 0 && glied[uz(e)] != 0) { glied[uz(k)] = 1; }
        }
        float abwRest = 0.0F;
        float abwGlied = 0.0F;
        std::vector<BoneMatrix> w0;
        std::vector<BoneMatrix> w1;
        for (const int f : {0, 7, 12, 25}) {
            rigWelt(g, a.schicht, f, f, 0.0F, w0);
            rigWelt(g, s, f, f, 0.0F, w1);
            for (std::size_t k = 0; k < w0.size() && k < w1.size(); ++k) {
                const BoneMatrix soll = p * w0[k];
                for (int r = 0; r < 3; ++r) {
                    for (int c = 0; c < 4; ++c) {
                        float& abw = glied[k] != 0 ? abwGlied : abwRest;
                        abw = std::max(abw, std::fabs(soll.m[r][c] - w1[k].m[r][c]));
                    }
                }
            }
        }
        std::printf("    Plattform: jede Weltmatrix = P * vorher, groesster Unterschied ohne Glieder %.3g, Glieder unter IK %.3g\n", static_cast<double>(abwRest),
                    static_cast<double>(abwGlied));
        // Grenzen: ohne IK rechnet nur die Plattform in den Wurzelknochen (float, Orte bis 50 Einheiten: 1e-4). Unter IK loest die
        // Zwei-Glied-IK das Kniegelenk ueber acos - nahe der gestreckten Kette veraendert ein Rundungsunterschied der Eingabe von 1e-7 den
        // Winkel um sqrt(2e-7) = 4,5e-4 (gemessen 0,0015 an ltail auch bei reiner Verschiebung, ENT3B_GRAD=0); Grenze: eine Stufe der
        // gepackten Verschiebung des Spiels (compressBone, 1/64 Einheit) - was darunter liegt, kann die GLA nicht unterscheiden
        expect("Plattform: jeder Knochen an jedem Bild steht bei P * vorher - auch Hand und Fuss unter IK (Ziele im Raum Welt gehen mit)", abwRest < 1e-4F && abwGlied < 1.0F / 64.0F);
        // backen: die GLA traegt die Plattform im Wurzelknochen; reduzieren behaelt sie als Spur (nicht doppelt)
        GlaAnimation gebacken;
        const bool gb = rigBacken(g, s, 0, kBilder, false, g.name, gebacken);
        float backFehler = 0.0F;
        for (int f = 0; gb && f < kBilder; f += 5) {
            gebacken.worldMatrices(f, w0);
            rigWelt(g, s, f, f, 0.0F, w1);
            for (std::size_t k = 0; k < w0.size() && k < w1.size(); ++k) {
                for (int r = 0; r < 3; ++r) { backFehler = std::max(backFehler, std::fabs(w0[k].m[r][3] - w1[k].m[r][3])); }
            }
        }
        // reduzieren mit und ohne Plattform: mit ihr dieselbe Schicht, nur die Plattform darueber (nicht in die Wurzel gebacken und dann
        // noch einmal angewandt)
        RigSchicht red;
        RigSchicht redOhne;
        const bool gr = rigReduzieren(g, s, 0, kBilder, red) && rigReduzieren(g, a.schicht, 0, kBilder, redOhne);
        float redFehler = 0.0F;
        for (int f = 0; gr && f < kBilder; f += 3) {
            rigWelt(g, red, f, f, 0.0F, w0);
            rigWelt(g, redOhne, f, f, 0.0F, w1);
            for (std::size_t k = 0; k < w0.size() && k < w1.size(); ++k) {
                const BoneMatrix soll = p * w1[k];
                for (int r = 0; r < 3; ++r) { redFehler = std::max(redFehler, std::fabs(w0[k].m[r][3] - soll.m[r][3])); }
            }
        }
        std::printf("    Plattform gebacken: groesster Ortsunterschied %.4f; reduziert %.4f, Spur behalten %zu\n", static_cast<double>(backFehler), static_cast<double>(redFehler),
                    red.plattform.size());
        // Grenzen: das Backen packt Orte auf 1/64 je Achse (compressBone), entlang der Kette bis zur Hand 8/64; reduzieren wie ENT2 1/16
        expect("Plattform gebacken: die GLA zeigt die Figur auf der Plattform (< 8/64 Einheit)", gb && backFehler < 0.125F);
        expect("Plattform reduziert: die Spur bleibt, die reduzierte Schicht ist die ohne Plattform mal P (nicht doppelt; < 1/64 wie oben)", gr && red.plattform.size() == 1U &&
                                                                                                                                             redFehler < 1.0F / 64.0F);
        // Missions-Datei: eine Zeile plattform, wieder gelesen dasselbe
        FigurAnim mit = a;
        mit.schicht = s;
        FigurAnim b;
        const bool ok = figurAusZeilen(figurZeilen(mit), g, rig, b, nullptr);
        expect("Plattform: Missions-Zeilen hin und zurueck, jede Matrix gleich", ok && b.schicht.plattform.size() == 1U && erwartetZeilen(g, b.schicht) == erwartetZeilen(g, s));
    }
    std::printf("%s (%d Fehler)\n", fails == 0 ? "OK" : "FEHLER", fails);
    return fails == 0 ? 0 : 1;
}
