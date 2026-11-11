// modell_autoskin.cpp - AP12: Auto-Skin (bhed/modell/autoskin.h)
//
//   Formen        Geschwister-Ketten (Schluesselbein -> Oberarm -> Drehknochen -> Unterarm -> Drehknochen -> Hand ->
//                 Finger nach Zaehlnamen), Wurzel/Bewegung/Tag-Knochen und Gruppenknoten tragen nichts, Blattknochen
//                 bekommen die Strecke durch den Schwerpunkt ihrer Zelle
//   Voxel         gerader Stab: der gerade Abstand (freie Sicht); U-Stueck: der Weg durch das Netz statt durch die Luft;
//                 Knochen neben dem Netz; loses Teil ohne Knochen
//   Gewichte      zwei Knochen in einer Roehre: am Gelenk halb/halb, spiegelgleich, steifer = schmaler
//   JKA-Grenzen   hoechstens 4 je Ecke, hoechstens 32 Knochen je Flaeche, 10 Bit wie das Spiel, Naehte zu, zweimal gleich
//   Uebertragen   dasselbe Netz: die Gewichte des Originals; ferne Punkte: Voxel-Gewichte
//   Vergleich     gewichteVergleichen und verformungsFehler an gerechneten Werten
//   Raven         (mit <JA base>) Gewichte von Raven-Figuren loeschen, neu rechnen, mit Ravens Gewichten vergleichen:
//                 staerkster Knochen gleich, mittlere Gewichtsabweichung, Verformungsfehler in Einheiten ueber
//                 Spielanimationen; Grenzen des Spiels an jedem Ergebnis
//
// Aufruf: modellautoskintest [<JA base>]
//         modellautoskintest <JA base> --suche [<figur> ...]        Messung fuer die Vorgaben (Steifigkeit, Glaetten, Schwelle)
//         modellautoskintest <JA base> --ausreisser <figur> [huelle|kyle]   die Ecken mit dem groessten Verformungsfehler
//         modellautoskintest <JA base> --abstand [<figur> ...]      Messung fuer die Vorgabe "Groesster Abstand" (Uebertragen)
#include "bhed/gla.h"
#include "bhed/glm.h"
#include "bhed/modell/autoskin.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/gewichte.h"
#include "bhed/pk3.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <tuple>
#include <vector>

namespace {
int fails = 0;
int pruefungen = 0;
void expect(const std::string& what, bool ok) {
    ++pruefungen;
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", what.c_str());
    if (!ok) { ++fails; }
}
std::string f(const char* fmt, double a = 0, double b = 0, double c = 0, double d = 0) {
    char z[400];
    std::snprintf(z, sizeof z, fmt, a, b, c, d);
    return z;
}

using namespace bhed;
using namespace bhed::modell;
using V3 = std::array<double, 3>;

// --- Bausteine ---------------------------------------------------------------------------------------------------

struct KnochenAngabe {
    std::string name;
    int eltern;
    V3 ort;
};

// Ein Skelett in der Grundstellung (Achsen der Welt), `bilder` Bilder in der Grundstellung (roh, lokale Stellung A = 1).
GlaAnimation skelett(const std::vector<KnochenAngabe>& k, int bilder = 1) {
    GlaAnimation g;
    g.name = "test";
    g.scale = 1.0F;
    g.numFrames = bilder;
    for (const KnochenAngabe& a : k) {
        GlaBone b;
        b.name = a.name;
        b.parent = a.eltern;
        b.basePose = BoneMatrix::identity();
        b.basePoseInv = BoneMatrix::identity();
        for (int r = 0; r < 3; ++r) {
            b.basePose.m[r][3] = static_cast<float>(a.ort[static_cast<std::size_t>(r)]);
            b.basePoseInv.m[r][3] = -static_cast<float>(a.ort[static_cast<std::size_t>(r)]);
        }
        g.bones.push_back(b);
    }
    g.roh.assign(static_cast<std::size_t>(bilder) * k.size(), BoneMatrix::identity());
    return g;
}

int knochen(const GlaAnimation& g, const std::string& name) { return boneIndex(g, name); }

Ecke ecke(double x, double y, double z) {
    Ecke e;
    e.ort[0] = static_cast<float>(x);
    e.ort[1] = static_cast<float>(y);
    e.ort[2] = static_cast<float>(z);
    e.normale[2] = 1.0F;
    e.gewicht[0] = 1.0F;
    return e;
}

// Eine geschlossene Roehre entlang einer Achse (0 x, 1 y, 2 z) von a bis b mit `ringe` + 1 Ringen zu je `seiten` Ecken
// und zwei Deckeln; die Ringe liegen bei a + i * (b - a) / ringe.
struct Roh {
    std::vector<Ecke> ecken;
    std::vector<std::uint32_t> idx;
};
void roehre(Roh& r, int achse, double a, double b, double radius, int ringe, int seiten, double mx = 0.0, double my = 0.0) {
    const auto basis = static_cast<std::uint32_t>(r.ecken.size());
    const double kPi = 3.14159265358979323846;
    const auto setze = [&](double laengs, double u, double v) {
        double p[3];
        p[achse] = laengs;
        p[(achse + 1) % 3] = u + mx;
        p[(achse + 2) % 3] = v + my;
        r.ecken.push_back(ecke(p[0], p[1], p[2]));
    };
    for (int i = 0; i <= ringe; ++i) {
        for (int j = 0; j < seiten; ++j) {
            const double w = 2.0 * kPi * j / seiten;
            setze(a + (b - a) * i / ringe, radius * std::cos(w), radius * std::sin(w));
        }
    }
    const auto ring = [&](int i, int j) { return basis + static_cast<std::uint32_t>(i * seiten + (j % seiten)); };
    for (int i = 0; i < ringe; ++i) {
        for (int j = 0; j < seiten; ++j) {
            r.idx.insert(r.idx.end(), {ring(i, j), ring(i + 1, j), ring(i + 1, j + 1), ring(i, j), ring(i + 1, j + 1), ring(i, j + 1)});
        }
    }
    const auto mitteA = static_cast<std::uint32_t>(r.ecken.size());
    setze(a, 0.0, 0.0);
    const auto mitteB = static_cast<std::uint32_t>(r.ecken.size());
    setze(b, 0.0, 0.0);
    for (int j = 0; j < seiten; ++j) {
        r.idx.insert(r.idx.end(), {mitteA, ring(0, j + 1), ring(0, j), mitteB, ring(ringe, j), ring(ringe, j + 1)});
    }
}

Teil teilAus(const std::string& name, const Roh& r, std::uint32_t flaggen = 0) {
    Teil t;
    t.name = name;
    t.flaggen = flaggen;
    t.stufen.resize(1);
    t.stufen[0].aufbauen(r.ecken, r.idx);
    return t;
}

Inhalt modellAus(std::vector<Teil> teile, int knochenZahl) {
    Inhalt in;
    in.herkunft = Herkunft::Glm;
    in.name = "test";
    in.knochenZahl = knochenZahl;
    in.stufenZahl = 1;
    in.teile = std::move(teile);
    return in;
}

double gewicht(const Ecke& e, int k) {
    double w = 0.0;
    for (int j = 0; j < 4; ++j) {
        if (e.knochen[j] == k) { w += static_cast<double>(e.gewicht[j]); }
    }
    return w;
}

// Steht in der Ecke, was eine .glm tragen kann und das Spiel liest (G2_GetVertBoneWeight, mdx_format.h:334-352)? n Gewichte
// > 0, danach nichts; die ersten n - 1 sind 10-Bit-Werte (Kehrwert * Zahl), absteigend; das letzte ist 1 - Summe der
// vorigen, in float gerechnet wie das Spiel; kein Knochen doppelt.
bool wieImSpiel(const Ecke& e) {
    int n = 0;
    while (n < 4 && e.gewicht[n] > 0.0F) { ++n; }
    if (n == 0) { return false; }
    for (int k = n; k < 4; ++k) {
        if (e.gewicht[k] != 0.0F) { return false; }
    }
    float summe = 0.0F;
    for (int k = 0; k + 1 < n; ++k) {
        const int bits = static_cast<int>(std::lround(static_cast<double>(e.gewicht[k]) * 1023.0));
        const float soll = g2Gewicht(bits);
        if (bits < 1 || bits > 1023 || std::memcmp(&soll, &e.gewicht[k], sizeof soll) != 0) { return false; }
        if (k > 0 && e.gewicht[k] > e.gewicht[k - 1]) { return false; }
        summe += e.gewicht[k];
    }
    const float rest = 1.0F - summe;
    if (std::memcmp(&rest, &e.gewicht[n - 1], sizeof rest) != 0) { return false; }
    for (int a = 0; a < n; ++a) {
        for (int b = a + 1; b < n; ++b) {
            if (e.knochen[a] == e.knochen[b]) { return false; }
        }
    }
    return true;
}

bool alleWieImSpiel(const Inhalt& in) {
    for (const Teil& t : in.teile) {
        if (t.istTag()) { continue; }
        for (const Netz& n : t.stufen) {
            for (const Ecke& e : n.ecken) {
                if (wieImSpiel(e)) { continue; }
                std::printf("    nicht wie im Spiel: %s, Knochen %d %d %d %d, Gewichte %.9g %.9g %.9g %.9g\n", t.name.c_str(), e.knochen[0], e.knochen[1],
                            e.knochen[2], e.knochen[3], static_cast<double>(e.gewicht[0]), static_cast<double>(e.gewicht[1]),
                            static_cast<double>(e.gewicht[2]), static_cast<double>(e.gewicht[3]));
                return false;
            }
        }
    }
    return true;
}

bool gleicheGewichte(const Ecke& a, const Ecke& b) {
    return std::memcmp(a.knochen, b.knochen, sizeof a.knochen) == 0 && std::memcmp(a.gewicht, b.gewicht, sizeof a.gewicht) == 0;
}

// Ein Arm wie in der _humanoid.gla von JKA: Drehknochen, Hand und Finger haengen flach unter rradius, der Oberarm
// neben dem Schluesselbein unter thoracic (Orte der rechten Seite aus der Datei, gerundet).
std::vector<KnochenAngabe> armWieJka() {
    return {
        {"model_root", -1, {0, 0, 0}},        // 0
        {"pelvis", 0, {0, 0, 36}},            // 1
        {"Motion", 1, {0, 0, 36.5}},          // 2
        {"thoracic", 1, {0, 0, 52}},          // 3
        {"cervical", 3, {0, 0, 57}},          // 4
        {"cranium", 4, {0, 0, 60}},           // 5
        {"face", 5, {0, 0, 60}},              // 6
        {"jaw", 6, {0, -1, 61}},              // 7
        {"rclavical", 3, {0, 0, 54.5}},       // 8
        {"rhumerus", 3, {-6, 0, 54.5}},       // 9
        {"rhumerusX", 9, {-9.5, 0, 50.5}},    // 10
        {"rradius", 9, {-13, 0, 46.5}},       // 11
        {"rradiusX", 11, {-16, 0, 43}},       // 12
        {"rhand", 11, {-19, 0, 40}},          // 13
        {"r_d1_j1", 11, {-19, -0.6, 39.2}},   // 14
        {"r_d1_j2", 11, {-19, -1.4, 37.9}},   // 15
        {"r_d2_j1", 11, {-21, -0.9, 37.4}},   // 16
        {"r_d2_j2", 11, {-20.8, -1.2, 36.1}}, // 17
        {"rhang_tag_bone", 11, {-20, 0, 37}}, // 18
        {"lclavical", 3, {0, 0, 54.5}},       // 19
        {"lhumerus", 3, {6, 0, 54.5}},        // 20
    };
}

void pruefeFormen() {
    std::printf("Formen\n");
    const GlaAnimation g = skelett(armWieJka());
    const Erkennung e = erkenneKnochen(g);
    AutoSkinOptionen o;
    const Inhalt leer;
    std::vector<KnochenForm> fo = knochenFormen(g, e, o, leer);
    const auto fe = [&](const char* n) { return fo[static_cast<std::size_t>(knochen(g, n))].formEltern; };
    const auto traegt = [&](const char* n) { return fo[static_cast<std::size_t>(knochen(g, n))].traegt; };
    expect("Wurzel, Motion und Tag-Knochen tragen nichts", !traegt("model_root") && !traegt("Motion") && !traegt("rhang_tag_bone"));
    expect("Gesicht traegt ohne Detailknochen nichts", !traegt("face") && !traegt("jaw"));
    expect("Oberarm haengt am Schluesselbein seiner Seite", fe("rhumerus") == knochen(g, "rclavical") && fe("lhumerus") == knochen(g, "lclavical"));
    expect("Drehknochen sind Kettenglieder: Oberarm -> humerusX -> Unterarm -> radiusX -> Hand",
           fe("rhumerusX") == knochen(g, "rhumerus") && fe("rradius") == knochen(g, "rhumerusX") && fe("rradiusX") == knochen(g, "rradius") &&
               fe("rhand") == knochen(g, "rradiusX"));
    expect("erstes Fingerglied haengt an der Hand, das zweite am ersten (Zaehlname)",
           fe("r_d1_j1") == knochen(g, "rhand") && fe("r_d2_j1") == knochen(g, "rhand") && fe("r_d1_j2") == knochen(g, "r_d1_j1") &&
               fe("r_d2_j2") == knochen(g, "r_d2_j1"));
    {
        const KnochenForm& h = fo[static_cast<std::size_t>(knochen(g, "rhumerus"))];
        const bool ok = h.strecken.size() == 1U && h.strecken[0][3] == -9.5 && h.strecken[0][5] == 50.5 && !h.blatt;
        expect("Form des Oberarms: eine Strecke von der Schulter zum Drehknochen", ok);
        const KnochenForm& hand = fo[static_cast<std::size_t>(knochen(g, "rhand"))];
        expect("Form der Hand: je eine Strecke zu den ersten Fingergliedern", hand.strecken.size() == 2U);
        const KnochenForm& sp = fo[static_cast<std::size_t>(knochen(g, "r_d1_j2"))];
        expect("Fingerspitze ohne Netz: Blatt, ein Punkt", sp.blatt && sp.strecken.size() == 1U && sp.strecken[0][0] == sp.strecken[0][3]);
    }
    o.drehknochen = false;
    fo = knochenFormen(g, e, o, leer);
    expect("ohne Drehknochen: Oberarm -> Unterarm -> Hand, die Drehknochen tragen nichts",
           !traegt("rhumerusX") && !traegt("rradiusX") && fe("rradius") == knochen(g, "rhumerus") && fe("rhand") == knochen(g, "rradius"));
    o.drehknochen = true;
    o.detailknochen = true;
    fo = knochenFormen(g, e, o, leer);
    expect("mit Detailknochen: der Gruppenknoten face traegt nichts, der Kiefer haengt am Schaedel",
           !traegt("face") && traegt("jaw") && fe("jaw") == knochen(g, "cranium"));
    o.detailknochen = false;
    o.knochen.assign(g.bones.size(), 0);
    o.knochen[static_cast<std::size_t>(knochen(g, "rhumerus"))] = 1;
    o.knochen[static_cast<std::size_t>(knochen(g, "rhand"))] = 1;
    fo = knochenFormen(g, e, o, leer);
    int tragend = 0;
    for (const KnochenForm& k : fo) { tragend += k.traegt ? 1 : 0; }
    expect("eigene Auswahl: nur die gewaehlten tragen, die Hand haengt ueber die anderen hinweg am Oberarm",
           tragend == 2 && fe("rhand") == knochen(g, "rhumerus"));

    // Blattknochen: Roehre von x = -8 bis 8, ein Knochen von -8 bis 0, sein Kind (Blatt) bei 0
    const GlaAnimation zwei = skelett({{"a", -1, {-8, 0, 0}}, {"b", 0, {0, 0, 0}}});
    Roh r;
    roehre(r, 0, -8.0, 8.0, 2.0, 16, 12);
    const Inhalt in = modellAus({teilAus("rohr", r)}, 2);
    const std::vector<KnochenForm> zf = knochenFormen(zwei, erkenneKnochen(zwei), AutoSkinOptionen{}, in);
    const std::array<double, 6>& s = zf[1].strecken[0];
    std::printf("    Blatt: Strecke bis %.6f %.6f %.6f\n", s[3], s[4], s[5]);
    // Die Zelle des Blatts sind die Ecken bei x >= 0 (am Ring x = 0 steht es mit dem Vorgaenger gleich - der Punkt zaehlt
    // zum Blatt). Das Ende liegt beim doppelten flaechengewichteten Schwerpunkt der Zelle; hier unabhaengig gerechnet:
    // je Ecke ein Drittel der Flaeche ihrer Dreiecke.
    const Netz& rohr = in.teile[0].stufen[0];
    const std::vector<std::uint32_t> idx = rohr.indizes();
    std::vector<double> fl(rohr.ecken.size(), 0.0);
    for (std::size_t k = 0; k + 2U < idx.size(); k += 3U) {
        const float* a = rohr.ecken[idx[k]].ort;
        const float* b = rohr.ecken[idx[k + 1U]].ort;
        const float* c = rohr.ecken[idx[k + 2U]].ort;
        const double u[3] = {static_cast<double>(b[0]) - a[0], static_cast<double>(b[1]) - a[1], static_cast<double>(b[2]) - a[2]};
        const double v[3] = {static_cast<double>(c[0]) - a[0], static_cast<double>(c[1]) - a[1], static_cast<double>(c[2]) - a[2]};
        const double kr[3] = {u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]};
        const double flaeche = 0.5 * std::sqrt(kr[0] * kr[0] + kr[1] * kr[1] + kr[2] * kr[2]);
        for (std::size_t j = 0; j < 3U; ++j) { fl[idx[k + j]] += flaeche / 3.0; }
    }
    double summeX = 0.0;
    double summe = 0.0;
    for (std::size_t v = 0; v < rohr.ecken.size(); ++v) {
        if (rohr.ecken[v].ort[0] < 0.0F) { continue; }
        summeX += fl[v] * static_cast<double>(rohr.ecken[v].ort[0]);
        summe += fl[v];
    }
    const double soll = 2.0 * summeX / summe;
    expect("Blatt: Strecke entlang der Roehre, quer 0 (Rundung der Winkel)", zf[1].blatt && std::abs(s[4]) < 1e-12 && std::abs(s[5]) < 1e-12);
    // glatte Roehre: Mantel 32 pi bei x = 4, Deckel 4 pi bei x = 8 -> 2 * 160 / 36 = 8,89; mit 12 Seiten und dem ganzen
    // Ring bei x = 0 (er traegt auch die Flaeche links von sich) 8,39
    expect(f("Blatt: Ende bei x = %.6f, nachgerechnet %.6f (auf 1e-9)", s[3], soll), std::abs(s[3] - soll) < 1e-9 && s[3] > 8.0 && s[3] < 9.0);
}

void pruefeVoxel() {
    std::printf("Voxel\n");
    // gerader Stab mit einem Knochen von -6 bis 6: jeder Punkt des Mantels sieht den Knochen
    const GlaAnimation g = skelett({{"a", -1, {-6, 0, 0}}, {"b", 0, {6, 0, 0}}});
    Roh r;
    roehre(r, 0, -8.0, 8.0, 2.0, 16, 12);
    const Inhalt in = modellAus({teilAus("rohr", r)}, 2);
    AutoSkinOptionen o;
    o.aufloesung = 64;
    const AutoSkinFeld feld = autoSkinFeld(in, g, o);
    double groesste = 0.0;
    for (std::size_t v = 0; v < in.teile[0].stufen[0].ecken.size(); ++v) {
        const Ecke& e = in.teile[0].stufen[0].ecken[v];
        const double x = std::clamp(static_cast<double>(e.ort[0]), -6.0, 6.0);
        const double soll = std::sqrt((e.ort[0] - x) * (e.ort[0] - x) + static_cast<double>(e.ort[1]) * e.ort[1] + static_cast<double>(e.ort[2]) * e.ort[2]);
        groesste = std::max(groesste, std::abs(static_cast<double>(feld.d(feld.punktVon[0][0][v], 0)) - soll));
    }
    std::printf("    Stab: %d x %d x %d Voxel zu %.4f, fest %d, innen %d, freie Sicht %d von %d, groesste Abweichung %.3g\n", feld.voxel[0],
                feld.voxel[1], feld.voxel[2], feld.voxelGroesse, feld.voxelFest, feld.voxelInnen, feld.punkteSicht, feld.punkte, groesste);
    expect("Stab: das Innere ist fest (Flutfuellung von aussen)", feld.voxelInnen > 0 && feld.voxelFest > feld.voxelInnen);
    expect("Stab: jeder Punkt sieht beide Knochen", feld.punkteSicht == 2 * feld.punkte && feld.knochenAussen == 0 && feld.punkteUnerreicht == 0);
    // float-Rundung des abgelegten Abstands (Werte bis 3): 2^-22
    expect(f("Stab: mit freier Sicht der gerade Abstand (Abweichung %.3g, erlaubt 5e-7)", groesste), groesste <= 5e-7);

    // U-Stueck: zwei Staebe entlang x bei y = -3 und y = +3, verbunden bei x = 8 bis 10; der Knochen liegt im Stab bei
    // y = -3 von x = -8 bis -6. Ein Punkt am offenen Ende des anderen Stabs ist durch die Luft 6 entfernt, durch das
    // Netz aber einmal um das U herum.
    Roh u;
    roehre(u, 0, -10.0, 10.0, 1.0, 20, 8, -3.0);
    roehre(u, 0, -10.0, 10.0, 1.0, 20, 8, 3.0);
    roehre(u, 1, -3.0, 3.0, 1.0, 6, 8, 0.0, 9.0);   // Querstueck entlang y bei x = 9 (Achse y: u = z, v = x)
    const GlaAnimation ug = skelett({{"a", -1, {-8, -3, 0}}, {"b", 0, {-6, -3, 0}}});
    const Inhalt uin = modellAus({teilAus("u", u)}, 2);
    const AutoSkinFeld uf = autoSkinFeld(uin, ug, o);
    // der Punkt in der Mitte des Deckels bei x = -10, y = +3
    int ziel = -1;
    for (std::size_t v = 0; v < uin.teile[0].stufen[0].ecken.size(); ++v) {
        const Ecke& e = uin.teile[0].stufen[0].ecken[v];
        if (e.ort[0] == -10.0F && e.ort[1] == 3.0F && e.ort[2] == 0.0F) { ziel = static_cast<int>(v); }
    }
    const double weg = ziel >= 0 ? static_cast<double>(uf.d(uf.punktVon[0][0][static_cast<std::size_t>(ziel)], 0)) : -1.0;
    // kuerzester Weg durch das U: vom Knochenende (-6, -3) den Stab entlang bis zur Innenkante des Querstuecks, hinueber
    // und zurueck. Untere Schranke mit Abkuerzen ueber die Innenkanten (x = 8, y = -2 und y = +2): sqrt(14^2 + 1) + 4 +
    // sqrt(18^2 + 1) = 36,06. Der Weg ueber 26 Nachbarn ist hoechstens 1,12-mal so lang wie der gerade (Richtung 2:1:1:
    // (sqrt(3) + 1) / sqrt(6) = 1,115).
    std::printf("    U: Weg zum offenen Ende des anderen Stabs %.3f (durch die Luft 6,3)\n", weg);
    expect(f("U: der Weg fuehrt durch das Netz (%.2f, erlaubt 36,0 bis 40,4)", weg), weg >= 36.0 && weg <= 36.06 * 1.12);
    AutoSkinOptionen oh = o;
    oh.verfahren = AutoSkinVerfahren::Huelle;
    const AutoSkinFeld hf = autoSkinFeld(uin, ug, oh);
    const double luft = ziel >= 0 ? static_cast<double>(hf.d(hf.punktVon[0][0][static_cast<std::size_t>(ziel)], 0)) : -1.0;
    // der naechste Punkt des Knochens ist sein Anfang (-8, -3, 0): sqrt(4 + 36); float-Rundung des abgelegten Abstands
    expect(f("U: die Huelle misst durch die Luft (%.6f, soll sqrt(40) = 6,324555)", luft), std::abs(luft - std::sqrt(40.0)) < 5e-7);

    // Knochen neben dem Netz und ein loses Teil ohne Knochen
    Roh los;
    roehre(los, 0, -8.0, 8.0, 2.0, 16, 12);
    roehre(los, 0, -2.0, 2.0, 1.0, 4, 8, 0.0, 30.0);   // lose, 30 ueber dem Stab
    const GlaAnimation lg = skelett({{"a", -1, {-6, 0, 0}}, {"b", 0, {6, 0, 0}}, {"neben", -1, {0, 8, 0}}});
    const Inhalt lin = modellAus({teilAus("rohr", los)}, 3);
    AutoSkinOptionen ol;
    ol.aufloesung = 64;
    const AutoSkinFeld lf = autoSkinFeld(lin, lg, ol);
    std::printf("    Knochen aussen %d, Punkte ohne Weg %d von %d\n", lf.knochenAussen, lf.punkteUnerreicht, lf.punkte);
    expect("Knochen neben dem Netz wird gezaehlt und erreicht das Netz trotzdem", lf.knochenAussen == 1 && lf.d(lf.punktVon[0][0][0], 2) < kAutoSkinUnerreicht);
    // das lose Stueck: 5 Ringe zu 8 Ecken und zwei Deckelmitten
    expect("loses Teil ohne Knochen: 42 Punkte ohne Weg, sie bekommen den geraden Abstand",
           lf.punkteUnerreicht == 42 && lf.d(lf.punktVon[0][0][lin.teile[0].stufen[0].ecken.size() - 1U], 0) < kAutoSkinUnerreicht);
}

void pruefeGewichte() {
    std::printf("Gewichte\n");
    // Roehre von -8 bis 8, Gelenk bei 0: Knochen a von -8 bis 0, Knochen b (Blatt) von 0 bis zum Schwerpunkt-Ende
    const GlaAnimation g = skelett({{"a", -1, {-8, 0, 0}}, {"b", 0, {0, 0, 0}}});
    Roh r;
    roehre(r, 0, -8.0, 8.0, 2.0, 16, 12);
    Inhalt in = modellAus({teilAus("rohr", r)}, 2);
    AutoSkinOptionen o;
    o.aufloesung = 64;
    o.glaetten = 0;
    AutoSkinBericht b;
    expect("autoSkin laeuft", autoSkin(in, g, o, b) && b.ok && b.ecken == static_cast<int>(in.teile[0].stufen[0].ecken.size()));
    const Netz& n = in.teile[0].stufen[0];
    // Ring i liegt bei x = -8 + i, 12 Ecken je Ring
    const auto ringGewicht = [&](const Netz& netz, int ring, int k) { return gewicht(netz.ecken[static_cast<std::size_t>(ring * 12)], k); };
    std::printf("    Gewicht von b je Ring:");
    for (int i = 0; i <= 16; ++i) { std::printf(" %.3f", ringGewicht(n, i, 1)); }
    std::printf("\n");
    bool rund = true;
    bool steigt = true;
    for (int i = 0; i <= 16; ++i) {
        // die Ecken eines Rings liegen auf float gerundet nicht genau gleich weit von der Achse: ein 1023stel
        for (int j = 1; j < 12; ++j) {
            rund = rund && std::abs(gewicht(n.ecken[static_cast<std::size_t>(i * 12)], 1) - gewicht(n.ecken[static_cast<std::size_t>(i * 12 + j)], 1)) <= 1.0 / 1023.0;
        }
        if (i > 0) { steigt = steigt && ringGewicht(n, i, 1) >= ringGewicht(n, i - 1, 1); }
    }
    expect("um die Achse herum gleiche Gewichte (jede Ecke eines Rings auf ein 1023stel)", rund);
    expect("das Gewicht des zweiten Knochens steigt entlang der Roehre", steigt);
    expect(f("am Gelenk halb/halb (%.4f, auf ein 1023stel)", ringGewicht(n, 8, 1)), std::abs(ringGewicht(n, 8, 1) - 0.5) <= 1.0 / 1023.0);
    expect("jede Ecke, wie das Spiel sie liest", alleWieImSpiel(in));
    // Gewicht ~ (naechster Abstand / Abstand)^k: Ring 7 (x = -1) ist von a 2 entfernt, von b sqrt(5); Ring 0 (x = -8) von
    // a 2, von b sqrt(68)
    const double k = autoSkinExponent(o.steifigkeit);
    const double ende = std::pow(2.0 / std::sqrt(68.0), k) / (1.0 + std::pow(2.0 / std::sqrt(68.0), k));
    expect(f("am Ende: %.4f, gerechnet %.4f (auf ein 1023stel)", ringGewicht(n, 0, 1), ende), std::abs(ringGewicht(n, 0, 1) - ende) <= 1.0 / 1023.0);
    const double soll = std::pow(2.0 / std::sqrt(5.0), k) / (1.0 + std::pow(2.0 / std::sqrt(5.0), k));
    expect(f("Ring neben dem Gelenk: %.4f, gerechnet %.4f (auf ein 1023stel)", ringGewicht(n, 7, 1), soll),
           std::abs(ringGewicht(n, 7, 1) - soll) <= 1.0 / 1023.0);
    Inhalt steif = modellAus({teilAus("rohr", r)}, 2);
    AutoSkinOptionen os = o;
    os.steifigkeit = 1.0F;
    AutoSkinBericht bs;
    (void)autoSkin(steif, g, os, bs);
    expect(f("steifer: der Uebergang wird schmaler (Ring neben dem Gelenk %.4f statt %.4f)", ringGewicht(steif.teile[0].stufen[0], 7, 1),
             ringGewicht(n, 7, 1)),
           ringGewicht(steif.teile[0].stufen[0], 7, 1) < ringGewicht(n, 7, 1));
    expect("Exponent: 0 -> 2, 1 -> 32, linear (0,3 -> 11)",
           autoSkinExponent(0.0F) == 2.0 && autoSkinExponent(1.0F) == 32.0 && std::abs(autoSkinExponent(0.3F) - 11.0) < 1e-6 &&
               autoSkinExponent(-1.0F) == 2.0 && autoSkinExponent(7.0F) == 32.0);
    // Glaetten: ein Durchgang zieht jeden Ring zum Mittel seiner Nachbarn
    Inhalt glatt = modellAus({teilAus("rohr", r)}, 2);
    AutoSkinOptionen og = o;
    og.glaetten = 2;
    AutoSkinBericht bg;
    (void)autoSkin(glatt, g, og, bg);
    // drei Ringe vor dem Gelenk (x = -3) liegt der Verlauf noch nach oben gewoelbt (0,000 / 0,002 / 0,022): das Mittel der
    // Nachbarn hebt den Wert. Am Ring neben dem Gelenk ist er nach unten gewoelbt (0,022 / 0,227 / 0,500), dort senkt es ihn.
    expect(f("Glaetten: der Uebergang wird breiter (drei Ringe vor dem Gelenk %.4f statt %.4f), am Gelenk bleibt halb/halb",
             ringGewicht(glatt.teile[0].stufen[0], 5, 1), ringGewicht(n, 5, 1)),
           ringGewicht(glatt.teile[0].stufen[0], 5, 1) > ringGewicht(n, 5, 1) &&
               std::abs(ringGewicht(glatt.teile[0].stufen[0], 8, 1) - 0.5) <= 1.0 / 1023.0);
    // Huelle: dieselbe Roehre, derselbe Wert (alle Punkte sehen beide Knochen)
    Inhalt huelle = modellAus({teilAus("rohr", r)}, 2);
    AutoSkinOptionen oh = o;
    oh.verfahren = AutoSkinVerfahren::Huelle;
    AutoSkinBericht bh;
    (void)autoSkin(huelle, g, oh, bh);
    bool gleich = true;
    for (std::size_t v = 0; v < n.ecken.size(); ++v) { gleich = gleich && gleicheGewichte(n.ecken[v], huelle.teile[0].stufen[0].ecken[v]); }
    expect("Huelle: auf der geraden Roehre bitgleich mit Voxel (freie Sicht ueberall)", gleich && bh.voxelFest == 0);
}

void pruefeGrenzen() {
    std::printf("JKA-Grenzen\n");
    // sechs Knochen im Stern um die Mitte einer Kugel-artigen Roehre: nahe der Mitte ziehen mehr als vier
    const GlaAnimation stern = skelett({{"mitte", -1, {0, 0, 0}}, {"a", 0, {3, 0, 0}}, {"b", 0, {-3, 0, 0}}, {"c", 0, {0, 3, 0}}, {"d", 0, {0, -3, 0}},
                                        {"e", 0, {0, 0, 3}}, {"f", 0, {0, 0, -3}}});
    Roh r;
    roehre(r, 2, -4.0, 4.0, 4.0, 8, 12);
    Inhalt in = modellAus({teilAus("kugel", r)}, 7);
    AutoSkinOptionen o;
    o.aufloesung = 64;
    o.steifigkeit = 0.0F;
    AutoSkinBericht b;
    (void)autoSkin(in, stern, o, b);
    std::printf("    Stern: %d Ecken, %d gekuerzt, meiste Gewichte %d (Grenze %d), Knochen benutzt %d von %d\n", b.ecken, b.eckenGekuerzt, b.gewichteMax,
                b.gewichteGrenze, b.knochenBenutzt, b.knochenTragend);
    expect("hoechstens 4 Gewichte je Ecke, und gekuerzt wurde wirklich", b.gewichteMax == 4 && b.gewichteGrenze == 4 && b.eckenGekuerzt > 0);
    expect("jede Ecke, wie das Spiel sie liest (Summe 1, absteigend, 10 Bit)", alleWieImSpiel(in));

    // 40 Knochen in einer Kette durch EINE lange Flaeche: mehr als 32 Knochen je Flaeche kann die .glm nicht tragen
    std::vector<KnochenAngabe> kette;
    for (int i = 0; i < 40; ++i) { kette.push_back({"glied" + std::to_string(100 + i), i - 1, {static_cast<double>(i) * 2.0, 0, 0}}); }
    const GlaAnimation kg = skelett(kette);
    Roh lang;
    roehre(lang, 0, -1.0, 81.0, 1.0, 82, 8);
    Inhalt lin = modellAus({teilAus("lang", lang)}, 40);
    AutoSkinOptionen ol;
    ol.aufloesung = 256;
    ol.glaetten = 0;
    AutoSkinBericht lb;
    (void)autoSkin(lin, kg, ol, lb);
    std::printf("    Kette: Knochen je Flaeche %d (Grenze %d), %d Flaeche gekuerzt, Knochen benutzt %d von %d\n", lb.knochenJeFlaecheMax,
                lb.knochenGrenze, lb.flaechenGekuerzt, lb.knochenBenutzt, lb.knochenTragend);
    expect("hoechstens 32 Knochen je Flaeche, 8 fielen weg", lb.knochenGrenze == 32 && lb.knochenJeFlaecheMax == 32 && lb.flaechenGekuerzt == 1 &&
                                                                 lb.knochenTragend == 40 && lb.knochenBenutzt == 32);
    expect("nach dem Kuerzen traegt jede Ecke weiter Gewichte wie im Spiel", alleWieImSpiel(lin) && mass(lin.teile[0].stufen[0]).knochen == 32);
    // wem der eigene Knochen genommen wurde, der haengt am naechsten, den die Flaeche behaelt: bei 8 von 40 Knochen im
    // Abstand 2 liegt der hoechstens 8 Glieder weiter
    double weitester = 0.0;
    for (const Ecke& e : lin.teile[0].stufen[0].ecken) {
        weitester = std::max(weitester, std::abs(static_cast<double>(e.ort[0]) - 2.0 * e.knochen[0]));
    }
    expect(f("keine Ecke haengt an einem fernen Knochen (weitester %.1f Einheiten, erlaubt 18)", weitester), weitester <= 18.0);

    // Naehte: dieselbe Roehre in zwei Flaechen geteilt (Ring bei x = 0 doppelt), dazu ein Tag und eine abgewaehlte Flaeche
    const GlaAnimation g = skelett({{"a", -1, {-8, 0, 0}}, {"b", 0, {-1, 0, 0}}, {"c", 1, {5, 0, 0}}});
    Roh links;
    Roh rechts;
    roehre(links, 0, -8.0, 0.0, 2.0, 8, 12);
    roehre(rechts, 0, 0.0, 8.0, 2.0, 8, 12);
    Roh tag;
    tag.ecken = {ecke(0, 0, 3), ecke(1, 0, 3), ecke(0, 1, 3)};
    tag.idx = {0, 1, 2};
    Roh fern;
    roehre(fern, 0, -8.0, 8.0, 0.5, 4, 6, 0.0, 6.0);
    const auto bauen = [&] {
        Inhalt x = modellAus({teilAus("links", links), teilAus("rechts", rechts), teilAus("*tag", tag, GlmSurface::kFlagBolt), teilAus("fern", fern)}, 3);
        for (Ecke& e : x.teile[3].stufen[0].ecken) { e.knochen[0] = 2; }
        malAnlegen(x.teile[0].stufen[0]);
        return x;
    };
    Inhalt naht = bauen();
    AutoSkinOptionen on;
    on.aufloesung = 64;
    on.glaetten = 2;
    on.teile = {1, 1, 1, 0};
    AutoSkinBericht nb;
    (void)autoSkin(naht, g, on, nb);
    bool zu = true;
    int paare = 0;
    for (const Ecke& a : naht.teile[0].stufen[0].ecken) {
        for (const Ecke& c : naht.teile[1].stufen[0].ecken) {
            if (std::memcmp(a.ort, c.ort, sizeof a.ort) != 0) { continue; }
            ++paare;
            zu = zu && gleicheGewichte(a, c);
        }
    }
    expect(f("Naht: %.0f Eckenpaare gleichen Orts in zwei Flaechen tragen bitgleiche Gewichte (mit Glaetten)", paare), zu && paare == 13);
    expect("Mallisten der gewichteten Flaeche sind weg", naht.teile[0].stufen[0].mal.empty());
    expect("Tag und abgewaehlte Flaeche bleiben, wie sie waren",
           naht.teile[2].stufen[0].ecken[0].gewicht[0] == 1.0F && naht.teile[2].stufen[0].ecken[0].knochen[0] == 0 &&
               naht.teile[3].stufen[0].ecken[0].knochen[0] == 2 && nb.ecken == static_cast<int>(naht.teile[0].stufen[0].ecken.size() +
                                                                                                  naht.teile[1].stufen[0].ecken.size()));
    Inhalt nochmal = bauen();
    AutoSkinBericht nb2;
    (void)autoSkin(nochmal, g, on, nb2);
    bool gleich = true;
    for (std::size_t t = 0; t < 2U; ++t) {
        for (std::size_t v = 0; v < naht.teile[t].stufen[0].ecken.size(); ++v) {
            gleich = gleich && gleicheGewichte(naht.teile[t].stufen[0].ecken[v], nochmal.teile[t].stufen[0].ecken[v]);
        }
    }
    expect("zweimal gerechnet: bitgleich (die Knochen laufen in mehreren Faeden)", gleich);
    // mit dem fertigen Feld (Abstaende) nur die Gewichte neu: dasselbe; ein Feld eines anderen Netzes wird nicht benutzt
    const Inhalt frisch = bauen();
    const AutoSkinFeld feld = autoSkinFeld(frisch, g, on);
    Inhalt mitFeld = bauen();
    AutoSkinBericht nb3;
    (void)autoSkin(mitFeld, g, on, nb3, nullptr, {}, &feld);
    const AutoSkinFeld fremd = autoSkinFeld(lin, kg, ol);
    Inhalt mitFremd = bauen();
    AutoSkinBericht nb4;
    (void)autoSkin(mitFremd, g, on, nb4, nullptr, {}, &fremd);
    bool feldGleich = true;
    for (std::size_t t = 0; t < 2U; ++t) {
        for (std::size_t v = 0; v < naht.teile[t].stufen[0].ecken.size(); ++v) {
            feldGleich = feldGleich && gleicheGewichte(naht.teile[t].stufen[0].ecken[v], mitFeld.teile[t].stufen[0].ecken[v]) &&
                         gleicheGewichte(naht.teile[t].stufen[0].ecken[v], mitFremd.teile[t].stufen[0].ecken[v]);
        }
    }
    expect("mit fertigem Feld bitgleich, ein fremdes Feld wird nicht benutzt", feldGleich && nb3.voxelFest == nb.voxelFest && nb4.voxelFest == nb.voxelFest);

    // Spiegeln danach: links/rechts ueber die Namen
    const GlaAnimation sg = skelett({{"mitte", -1, {0, 0, 0}}, {"larm", 0, {2, 0, 0}}, {"rarm", 0, {-2, 0, 0}}});
    Roh quer;
    roehre(quer, 0, -8.0, 8.0, 1.0, 16, 8);
    Inhalt sin = modellAus({teilAus("quer", quer)}, 3);
    AutoSkinOptionen osp;
    osp.aufloesung = 64;
    osp.spiegeln = true;
    AutoSkinBericht sb;
    (void)autoSkin(sin, sg, osp, sb);
    bool spiegel = sb.gespiegelt > 0;
    const Netz& sn = sin.teile[0].stufen[0];
    for (const Ecke& a : sn.ecken) {
        for (const Ecke& c : sn.ecken) {
            if (a.ort[0] <= 0.05F || c.ort[0] != -a.ort[0] || c.ort[1] != a.ort[1] || c.ort[2] != a.ort[2]) { continue; }
            spiegel = spiegel && std::abs(gewicht(a, 1) - gewicht(c, 2)) < 1e-6 && std::abs(gewicht(a, 0) - gewicht(c, 0)) < 1e-6;
        }
    }
    expect(f("danach spiegeln: %.0f Ecken, larm links wie rarm rechts", sb.gespiegelt), spiegel && alleWieImSpiel(sin));

    // Fehler
    Inhalt leer;
    AutoSkinBericht fb;
    expect("ohne Skelett: Fehler skelett", !autoSkin(naht, GlaAnimation{}, on, fb) && fb.fehler == "skelett");
    expect("ohne Flaeche: Fehler flaechen", !autoSkin(leer, g, on, fb) && fb.fehler == "flaechen");
    AutoSkinOptionen ou = on;
    ou.verfahren = AutoSkinVerfahren::Uebertragen;
    expect("Uebertragen ohne Original: Fehler vorlage", !autoSkin(naht, g, ou, fb) && fb.fehler == "vorlage");
    AutoSkinOptionen ok = on;
    ok.knochen = {0, 0, 0};
    expect("kein Knochen traegt: Fehler knochen, das Netz bleibt", !autoSkin(nochmal, g, ok, fb) && fb.fehler == "knochen" &&
                                                                        gleicheGewichte(naht.teile[0].stufen[0].ecken[0], nochmal.teile[0].stufen[0].ecken[0]));
}

void pruefeUebertragen() {
    std::printf("Uebertragen\n");
    const GlaAnimation g = skelett({{"a", -1, {-8, 0, 0}}, {"b", 0, {0, 0, 0}}, {"c", 1, {4, 0, 0}}});
    Roh r;
    roehre(r, 0, -8.0, 8.0, 2.0, 16, 12);
    // das Original: Gewichte von Hand, ein Verlauf von a nach b ueber die Ringe 4 bis 12 (10 Bit)
    Inhalt original = modellAus({teilAus("rohr", r)}, 3);
    for (Ecke& e : original.teile[0].stufen[0].ecken) {
        const double t = std::clamp((static_cast<double>(e.ort[0]) + 4.0) / 8.0, 0.0, 1.0);
        (void)aufEcke({KnochenGewicht{0, static_cast<float>(1.0 - t)}, KnochenGewicht{1, static_cast<float>(t)}}, e);
    }
    Inhalt ziel = modellAus({teilAus("rohr", r)}, 3);
    AutoSkinOptionen o;
    o.verfahren = AutoSkinVerfahren::Uebertragen;
    o.aufloesung = 64;
    AutoSkinBericht b;
    expect("laeuft", autoSkin(ziel, g, o, b, &original) && b.uebertragen == b.punkte && b.fern == 0 && b.voxelFest == 0);
    bool gleich = true;
    for (std::size_t v = 0; v < ziel.teile[0].stufen[0].ecken.size(); ++v) {
        gleich = gleich && gleicheGewichte(ziel.teile[0].stufen[0].ecken[v], original.teile[0].stufen[0].ecken[v]);
    }
    expect("dasselbe Netz: bitgleich die Gewichte des Originals (auch mit Glaetten 2 - Punkte vom Original bleiben)", gleich);
    // ein feineres Netz (doppelt so viele Ringe): zwischen zwei Ringen des Originals das Mittel
    Roh fein;
    roehre(fein, 0, -8.0, 8.0, 2.0, 32, 12);
    Inhalt feiner = modellAus({teilAus("rohr", fein)}, 3);
    (void)autoSkin(feiner, g, o, b, &original);
    // Ring 17 des feinen Netzes liegt bei x = 0,5 zwischen den Ringen bei 0 (t = 0,5) und 1 (t = 0,625): 0,5625
    const double w = gewicht(feiner.teile[0].stufen[0].ecken[17U * 12U], 1);
    expect(f("feineres Netz: zwischen zwei Ringen das Mittel (%.4f, soll 0,5625 auf zwei 1023stel)", w), std::abs(w - 0.5625) <= 2.0 / 1023.0);
    // die Knochen der Vorlage auf andere Nummern legen: a -> c, b -> b
    Inhalt umgelegt = modellAus({teilAus("rohr", r)}, 3);
    (void)autoSkin(umgelegt, g, o, b, &original, {2, 1, -1});
    expect("Knochen der Vorlage umgelegt", gewicht(umgelegt.teile[0].stufen[0].ecken[0], 2) == 1.0 && gewicht(umgelegt.teile[0].stufen[0].ecken[0], 0) == 0.0);
    // ferne Punkte: eine zweite Roehre 10 daneben, weiter als uebertragenAbstand vom Original
    Roh zwei = r;
    roehre(zwei, 0, 0.0, 8.0, 1.0, 8, 8, 10.0);
    Inhalt mitFern = modellAus({teilAus("rohr", zwei)}, 3);
    (void)autoSkin(mitFern, g, o, b, &original);
    std::printf("    fern: %d vom Original, %d mit Voxel-Gewichten, %d ohne Weg\n", b.uebertragen, b.fern, b.punkteUnerreicht);
    expect("ferne Punkte bekommen Voxel-Gewichte, die nahen die des Originals", b.fern == 74 && b.uebertragen == static_cast<int>(r.ecken.size()) &&
                                                                                  b.voxelFest > 0 && alleWieImSpiel(mitFern) &&
                                                                                  gleicheGewichte(mitFern.teile[0].stufen[0].ecken[0],
                                                                                                  original.teile[0].stufen[0].ecken[0]));
}

void pruefeVergleich() {
    std::printf("Vergleich\n");
    // zwei Knochen, Bild 1 schiebt b um 3 nach oben
    GlaAnimation g = skelett({{"a", -1, {-8, 0, 0}}, {"b", 0, {0, 0, 0}}}, 2);
    g.roh[2U + 1U].m[2][3] = 3.0F;
    Roh r;
    roehre(r, 0, -8.0, 8.0, 2.0, 16, 12);
    Inhalt a = modellAus({teilAus("rohr", r)}, 2);
    Inhalt b = a;
    const GewichtVergleich gleich = gewichteVergleichen(a, b, 0, 2);
    expect("gleiche Gewichte: jeder staerkste Knochen gleich, Abweichung 0", gleich.ecken == static_cast<int>(r.ecken.size()) &&
                                                                               gleich.hauptGleich == gleich.ecken && gleich.abweichungMittel == 0.0);
    // 12 Ecken (Ring 0) in b ganz auf Knochen b, 12 (Ring 1) zu 25 %
    for (std::size_t v = 0; v < 12U; ++v) {
        (void)aufEcke({KnochenGewicht{1, 1.0F}}, b.teile[0].stufen[0].ecken[v]);
        (void)aufEcke({KnochenGewicht{0, 0.75F}, KnochenGewicht{1, 0.25F}}, b.teile[0].stufen[0].ecken[12U + v]);
    }
    const GewichtVergleich v = gewichteVergleichen(a, b, 0, 2);
    const double w25 = gewicht(b.teile[0].stufen[0].ecken[12], 1);   // 0,25 auf 10 Bit
    const double soll = (12.0 * 1.0 + 12.0 * w25) / static_cast<double>(v.ecken);
    expect(f("12 Ecken anders: staerkster Knochen an %.0f von %.0f gleich, Abweichung %.6f (soll %.6f)", v.hauptGleich, v.ecken, v.abweichungMittel, soll),
           v.hauptGleich == v.ecken - 12 && std::abs(v.abweichungMittel - soll) < 1e-12 && v.eckenJeKnochen[0] == v.ecken &&
               v.hauptGleichJeKnochen[0] == v.ecken - 12);
    const VerformungsFehler f0 = verformungsFehler(a, b, g, {0}, 0);
    const VerformungsFehler f1 = verformungsFehler(a, b, g, {1}, 0);
    // im Bild 1 liegt eine Ecke mit Gewichtsunterschied dw um 3 * dw daneben: Ring 0 um 3, Ring 1 um 3 * w25
    const double mittel = (12.0 * 3.0 + 12.0 * 3.0 * w25) / static_cast<double>(v.ecken);
    expect("Grundstellung: kein Verformungsfehler", f0.bilder == 1 && f0.max == 0.0 && f0.mittel == 0.0);
    expect(f("Bild mit Knochen 3 hoeher: groesster Fehler %.6f (soll 3), Mittel %.6f (soll %.6f)", f1.max, f1.mittel, mittel),
           std::abs(f1.max - 3.0) < 1e-6 && std::abs(f1.mittel - mittel) < 1e-6 && f1.ecken == v.ecken);
    expect("Bild ausserhalb der GLA zaehlt nicht", verformungsFehler(a, b, g, {7, -1}, 0).bilder == 0);
}

// --- Raven-Modelle ---------------------------------------------------------------------------------------------------

struct Spiel {
    std::vector<GamePath> ordner;
    bool lies(const std::string& name, std::string& aus) const { return liesWieSpiel(ordner, name, false, aus); }
};

void gewichteLoeschen(Inhalt& in) {
    for (Teil& t : in.teile) {
        if (t.istTag()) { continue; }
        for (Netz& n : t.stufen) {
            for (Ecke& e : n.ecken) {
                for (int k = 0; k < 4; ++k) {
                    e.knochen[k] = 0;
                    e.gewicht[k] = k == 0 ? 1.0F : 0.0F;
                }
            }
        }
    }
}

struct Ergebnis {
    std::string name;
    int ecken = 0;
    double haupt = 0;       // Anteil
    double abweichung = 0;
    VerformungsFehler fehler;
    AutoSkinBericht bericht;
    double ms = 0;
    bool grenzen = false;
};

// Bilder aus den Spielanimationen: Stehen, Gehen, Laufen, Ducken, ein Saebelhieb, eine Rolle - je Abschnitt hoechstens
// acht Bilder in gleichem Abstand.
std::vector<int> spielBilder(const std::vector<AnimEntry>& cfg, int glaBilder) {
    std::vector<int> bilder;
    for (const char* name : {"BOTH_STAND1", "BOTH_WALK1", "BOTH_RUN1", "BOTH_CROUCH1WALK", "BOTH_A1_T__B_", "BOTH_ROLL_F", "BOTH_JUMP1", "BOTH_DEATH1"}) {
        for (const AnimEntry& e : cfg) {
            if (e.name != name || e.numFrames <= 0) { continue; }
            const int zahl = std::min(e.numFrames, 8);
            for (int i = 0; i < zahl; ++i) {
                const int bild = e.firstFrame + (zahl > 1 ? i * (e.numFrames - 1) / (zahl - 1) : 0);
                if (bild >= 0 && bild < glaBilder) { bilder.push_back(bild); }
            }
        }
    }
    return bilder;
}

Ergebnis messen(const std::string& name, const Inhalt& raven, const GlaAnimation& g, const std::vector<int>& bilder, const AutoSkinOptionen& o,
                const Inhalt* vorlage = nullptr, const AutoSkinFeld* feld = nullptr) {
    Ergebnis r;
    r.name = name;
    Inhalt neu = raven;
    gewichteLoeschen(neu);
    const auto start = std::chrono::steady_clock::now();
    (void)autoSkin(neu, g, o, r.bericht, vorlage, {}, feld);
    r.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    const GewichtVergleich v = gewichteVergleichen(raven, neu, 0, static_cast<int>(g.bones.size()));
    r.ecken = v.ecken;
    r.haupt = v.ecken > 0 ? static_cast<double>(v.hauptGleich) / v.ecken : 0.0;
    r.abweichung = v.abweichungMittel;
    r.fehler = verformungsFehler(raven, neu, g, bilder, 0);
    const Grenzen& gr = grenzen(o.ziel);
    r.grenzen = r.bericht.ok && r.bericht.gewichteMax <= gr.gewichteJeEcke && r.bericht.knochenJeFlaecheMax <= gr.knochenJeFlaeche && alleWieImSpiel(neu);
    return r;
}

void zeile(const char* verfahren, const Ergebnis& r) {
    std::printf("    %-12s %-14s %5d Ecken | staerkster Knochen gleich %5.1f %% | Gewichtsabweichung %.3f | Verformung Mittel %.3f, 95 %% %.3f, "
                "groesste %.2f Einheiten (%d Bilder) | Knochen je Flaeche %d, benutzt %d | %.0f ms\n",
                verfahren, r.name.c_str(), r.ecken, 100.0 * r.haupt, r.abweichung, r.fehler.mittel, r.fehler.p95, r.fehler.max, r.fehler.bilder,
                r.bericht.knochenJeFlaecheMax, r.bericht.knochenBenutzt, r.ms);
}

bool ladeModell(const Spiel& sp, const std::string& pfad, Inhalt& aus) {
    std::string bytes;
    return sp.lies(pfad, bytes) && ausGlm(bytes, aus);
}

void pruefeRaven(const std::string& jaBase) {
    std::printf("Raven (%s)\n", jaBase.c_str());
    Spiel sp;
    sp.ordner.resize(1);
    if (!scanGamePath(jaBase, sp.ordner[0]) || sp.ordner[0].archives.empty()) {
        std::printf("  uebersprungen: keine .pk3 in %s\n", jaBase.c_str());
        return;
    }
    std::string bytes;
    GlaAnimation g;
    if (!sp.lies("models/players/_humanoid/_humanoid.gla", bytes) || !readGla(bytes, g)) {
        std::printf("  uebersprungen: _humanoid.gla nicht gefunden\n");
        return;
    }
    std::string cfgText;
    (void)sp.lies("models/players/_humanoid/animation.cfg", cfgText);
    const std::vector<int> bilder = spielBilder(parseAnimationCfg(cfgText), g.numFrames);
    expect(f("Bilder aus den Spielanimationen: %.0f", static_cast<double>(bilder.size())), bilder.size() >= 40U);
    AutoSkinOptionen voxel;
    AutoSkinOptionen huelle;
    huelle.verfahren = AutoSkinVerfahren::Huelle;
    AutoSkinOptionen ueber;
    ueber.verfahren = AutoSkinVerfahren::Uebertragen;
    Inhalt kyle;
    const bool hatKyle = ladeModell(sp, "models/players/kyle/model.glm", kyle);
    bool alleGrenzen = true;
    int gemessen = 0;
    int uebertragene = 0;
    // Summen ueber die Figuren: Voxel, Huelle, von kyle uebertragen (und Voxel nur ueber die Figuren mit Uebertragung)
    double hauptV = 0.0;
    double verfV = 0.0;
    double verfH = 0.0;
    double hauptU = 0.0;
    double verfU = 0.0;
    double verfVU = 0.0;
    double schlechteste = 0.0;
    for (const char* name : {"kyle", "luke", "stormtrooper", "tavion", "reborn", "jawa", "jedi_hm", "rebel", "rodian", "lando"}) {
        Inhalt raven;
        if (!ladeModell(sp, std::string("models/players/") + name + "/model.glm", raven)) {
            std::printf("    %s: nicht gefunden\n", name);
            continue;
        }
        if (raven.knochenZahl != static_cast<int>(g.bones.size())) {
            std::printf("    %s: anderes Skelett (%d Knochen)\n", name, raven.knochenZahl);
            continue;
        }
        ++gemessen;
        const Ergebnis v = messen(name, raven, g, bilder, voxel);
        zeile("Voxel", v);
        const Ergebnis h = messen(name, raven, g, bilder, huelle);
        zeile("Huelle", h);
        alleGrenzen = alleGrenzen && v.grenzen && h.grenzen;
        hauptV += v.haupt;
        verfV += v.fehler.mittel;
        verfH += h.fehler.mittel;
        schlechteste = std::max(schlechteste, v.fehler.mittel);
        if (hatKyle && std::string(name) != "kyle") {
            const Ergebnis u = messen(name, raven, g, bilder, ueber, &kyle);
            zeile("von kyle", u);
            std::printf("        vom Original %d Punkte, mit Voxel-Gewichten %d\n", u.bericht.uebertragen, u.bericht.fern);
            alleGrenzen = alleGrenzen && u.grenzen;
            ++uebertragene;
            hauptU += u.haupt;
            verfU += u.fehler.mittel;
            verfVU += v.fehler.mittel;
        }
        std::printf("        Voxel %d x %d x %d zu %.3f, fest %d, innen %d, Knochen aussen %d, Punkte ohne Weg %d, Ecken gekuerzt %d, Flaechen gekuerzt %d\n",
                    v.bericht.voxel[0], v.bericht.voxel[1], v.bericht.voxel[2], v.bericht.voxelGroesse, v.bericht.voxelFest, v.bericht.voxelInnen,
                    v.bericht.knochenAussen, v.bericht.punkteUnerreicht, v.bericht.eckenGekuerzt, v.bericht.flaechenGekuerzt);
    }
    expect(f("%.0f Raven-Figuren gemessen", gemessen), gemessen >= 7);
    expect("jedes Ergebnis haelt die Grenzen des Spiels (4 je Ecke, 32 je Flaeche, 10 Bit)", alleGrenzen);
    // Die Schwellen unten sind Rueckschrittswaechter, keine Ziele: gemessen am 11.11.2026 an den zehn Figuren oben (drei davon -
    // kyle, luke, stormtrooper - dienten der Wahl der Vorgaben, sieben sind Gegenprobe), Vorgaben Steifigkeit 0,3, Glaetten 2,
    // Aufloesung 256. Die Rechnung ist auf demselben Rechner bitgleich; der Spielraum (ein halber Prozentpunkt bzw. 3 %) deckt
    // andere Uebersetzer und Rechner (pow, sqrt und die Rundung auf 10 Bit an der Grenze).
    if (gemessen == 10 && uebertragene == 9) {
        const double n = 10.0;
        std::printf("    Mittel ueber 10 Figuren: Voxel staerkster Knochen gleich %.2f %%, Verformung %.4f Einheiten (schlechteste Figur %.4f); Huelle "
                    "Verformung %.4f\n",
                    100.0 * hauptV / n, verfV / n, schlechteste, verfH / n);
        std::printf("    Mittel ueber 9 Figuren: von kyle uebertragen staerkster Knochen gleich %.2f %%, Verformung %.4f Einheiten (Voxel an denselben "
                    "%.4f)\n",
                    100.0 * hauptU / 9.0, verfU / 9.0, verfVU / 9.0);
        // gemessen 71,91 %
        expect(f("Voxel: staerkster Knochen im Mittel an %.2f %% der Ecken wie bei Raven (mindestens 71,4)", 100.0 * hauptV / n), 100.0 * hauptV / n >= 71.4);
        // gemessen 0,3063, die schlechteste Figur (jawa) 0,772
        expect(f("Voxel: mittlere Verformungsabweichung %.4f Einheiten (hoechstens 0,316), schlechteste Figur %.4f (hoechstens 0,80)", verfV / n,
                 schlechteste),
               verfV / n <= 0.316 && schlechteste <= 0.80);
        // gemessen Huelle 0,3563: der Weg durch das Netz ist im Mittel nicht schlechter als der gerade Abstand
        expect(f("Voxel nicht schlechter als Huelle (%.4f gegen %.4f Einheiten)", verfV / n, verfH / n), verfV <= verfH);
        // gemessen 81,09 % und 0,1991; Voxel an denselben neun Figuren 0,3204
        expect(f("von kyle uebertragen: staerkster Knochen an %.2f %% wie bei Raven (mindestens 80,5), Verformung %.4f Einheiten (hoechstens 0,206)",
                 100.0 * hauptU / 9.0, verfU / 9.0),
               100.0 * hauptU / 9.0 >= 80.5 && verfU / 9.0 <= 0.206);
        expect(f("von einem Original uebertragen ist naeher an Raven als gerechnet (%.4f gegen %.4f Einheiten)", verfU / 9.0, verfVU / 9.0),
               verfU < verfVU);
    } else {
        std::printf("    Schwellen nicht geprueft: %d Figuren, %d Uebertragungen (erwartet 10 und 9)\n", gemessen, uebertragene);
    }
    // Von sich selbst uebertragen: die Gewichte der Datei kommen zurueck. Nicht bitgleich - Ecken gleichen Orts werden ein Punkt
    // (in der Datei tragen einige verschiedene Gewichte), nicht absteigend abgelegte Gewichte werden umsortiert.
    if (hatKyle) {
        const Ergebnis selbst = messen("kyle", kyle, g, bilder, ueber, &kyle);
        zeile("von sich", selbst);
        Inhalt neu = kyle;
        gewichteLoeschen(neu);
        AutoSkinBericht b;
        (void)autoSkin(neu, g, ueber, b, &kyle);
        const GewichtVergleich v = gewichteVergleichen(kyle, neu, 0, static_cast<int>(g.bones.size()));
        std::printf("        von sich: vom Original %d Punkte, fern %d; staerkster Knochen gleich an %d von %d Ecken, Gewichtsabweichung %.6f, Verformung "
                    "Mittel %.6f, groesste %.6f\n",
                    selbst.bericht.uebertragen, selbst.bericht.fern, v.hauptGleich, v.ecken, v.abweichungMittel, selbst.fehler.mittel, selbst.fehler.max);
        // gemessen: 2779 von 2782 Ecken (99,89 %), Gewichtsabweichung 0,000726, Verformung im Mittel 0,000002, groesste 0,0147 Einheiten;
        // 41 Punkte groeberer Stufen liegen weiter als 2 von der Stufe 0 ihrer Flaeche und bekommen Voxel-Gewichte. Schranken: eine
        // Ecke mehr, das Fuenffache des Mittels, ein Drittel mehr beim groessten.
        expect(f("kyle von sich selbst: staerkster Knochen an %.0f von %.0f Ecken gleich (mindestens alle bis auf 4), Verformung Mittel %.6f (hoechstens "
                 "0,00001), groesste %.4f Einheiten (hoechstens 0,02)",
                 v.hauptGleich, v.ecken, selbst.fehler.mittel, selbst.fehler.max),
               v.hauptGleich >= v.ecken - 4 && selbst.fehler.mittel <= 1e-5 && selbst.fehler.max <= 0.02);
    }
    // Die Vorgabe "ohne Gesicht und Anhaenge" an kyle gemessen: mit ihnen nehmen die Gesichtsknochen dem Schaedel seine Ecken
    if (hatKyle) {
        AutoSkinOptionen mitDetail;
        mitDetail.detailknochen = true;
        const Ergebnis ohne = messen("kyle", kyle, g, bilder, voxel);
        const Ergebnis mit = messen("kyle+Detail", kyle, g, bilder, mitDetail);
        zeile("mit Detail", mit);
        Inhalt neu = kyle;
        gewichteLoeschen(neu);
        AutoSkinBericht b;
        (void)autoSkin(neu, g, mitDetail, b);
        const GewichtVergleich v = gewichteVergleichen(kyle, neu, 0, static_cast<int>(g.bones.size()));
        const auto schaedel = static_cast<std::size_t>(boneIndex(g, "cranium"));
        std::printf("        Schaedel: bei Raven an %d Ecken der staerkste Knochen, mit Detailknochen davon noch an %d\n", v.eckenJeKnochen[schaedel],
                    v.hauptGleichJeKnochen[schaedel]);
        expect(f("Vorgabe ohne Gesicht und Anhaenge: naeher an Raven (staerkster Knochen %.1f %% gegen %.1f %% mit ihnen)", 100.0 * ohne.haupt,
                 100.0 * mit.haupt),
               ohne.haupt > mit.haupt && mit.grenzen && 2 * v.hauptGleichJeKnochen[schaedel] < v.eckenJeKnochen[schaedel]);
    }
    // Gegenprobe an Skeletten, die nicht zum Finden der Regeln dienten: Kreaturen mit eigener GLA
    for (const char* name : {"rancor", "wampa", "tauntaun"}) {
        Inhalt raven;
        GlaAnimation eigene;
        std::string glaBytes;
        if (!ladeModell(sp, std::string("models/players/") + name + "/model.glm", raven) || !sp.lies(raven.glaPfad + ".gla", glaBytes) ||
            !readGla(glaBytes, eigene) || raven.knochenZahl != static_cast<int>(eigene.bones.size())) {
            std::printf("    %s: nicht gefunden oder ohne passende GLA\n", name);
            continue;
        }
        // 40 Bilder in gleichem Abstand ueber die ganze GLA
        std::vector<int> eigeneBilder;
        for (int i = 0; i < 40; ++i) { eigeneBilder.push_back(static_cast<int>(static_cast<long long>(i) * (eigene.numFrames - 1) / 39)); }
        const Ergebnis v = messen(name, raven, eigene, eigeneBilder, voxel);
        zeile("Voxel", v);
        const Ergebnis h = messen(name, raven, eigene, eigeneBilder, huelle);
        zeile("Huelle", h);
        std::printf("        %s: %d Knochen, %d tragen, Voxel %d x %d x %d zu %.3f, Knochen aussen %d, Punkte ohne Weg %d\n", raven.glaPfad.c_str(),
                    static_cast<int>(eigene.bones.size()), v.bericht.knochenTragend, v.bericht.voxel[0], v.bericht.voxel[1], v.bericht.voxel[2],
                    v.bericht.voxelGroesse, v.bericht.knochenAussen, v.bericht.punkteUnerreicht);
        expect(std::string(name) + ": eigenes Skelett, das Ergebnis haelt die Grenzen des Spiels", v.grenzen && h.grenzen && v.bericht.knochenTragend > 1);
    }
}

// Auskunft (kein Test): die Ecken, die unter den neuen Gewichten am weitesten von ihrem Ort unter Ravens Gewichten liegen.
int ausreisser(const std::string& jaBase, const std::string& figur, const std::string& verfahren) {
    Spiel sp;
    sp.ordner.resize(1);
    std::string bytes;
    GlaAnimation g;
    Inhalt raven;
    if (!scanGamePath(jaBase, sp.ordner[0]) || !sp.lies("models/players/_humanoid/_humanoid.gla", bytes) || !readGla(bytes, g) ||
        !ladeModell(sp, "models/players/" + figur + "/model.glm", raven)) {
        return 1;
    }
    std::string cfgText;
    (void)sp.lies("models/players/_humanoid/animation.cfg", cfgText);
    const std::vector<AnimEntry> cfg = parseAnimationCfg(cfgText);
    const std::vector<int> bilder = spielBilder(cfg, g.numFrames);
    AutoSkinOptionen o;
    Inhalt kyle;
    const Inhalt* vorlage = nullptr;
    if (verfahren == "huelle") { o.verfahren = AutoSkinVerfahren::Huelle; }
    if (verfahren == "kyle" && ladeModell(sp, "models/players/kyle/model.glm", kyle)) {
        o.verfahren = AutoSkinVerfahren::Uebertragen;
        vorlage = &kyle;
    }
    Inhalt neu = raven;
    gewichteLoeschen(neu);
    AutoSkinBericht b;
    (void)autoSkin(neu, g, o, b, vorlage);
    struct Fund {
        double weit = 0.0;
        int bild = 0;
        std::size_t teil = 0;
        std::size_t ecke = 0;
    };
    std::vector<Fund> funde;
    std::vector<BoneMatrix> welt;
    std::vector<BoneMatrix> haut(g.bones.size());
    for (std::size_t t = 0; t < raven.teile.size(); ++t) {
        if (raven.teile[t].istTag()) { continue; }
        for (std::size_t e = 0; e < raven.teile[t].stufen[0].ecken.size(); ++e) { funde.push_back(Fund{0.0, 0, t, e}); }
    }
    for (const int bild : bilder) {
        g.worldMatrices(bild, welt);
        for (std::size_t k = 0; k < g.bones.size(); ++k) { haut[k] = welt[k] * g.bones[k].basePoseInv; }
        for (Fund& fu : funde) {
            double ort[2][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
            const Ecke* paar[2] = {&raven.teile[fu.teil].stufen[0].ecken[fu.ecke], &neu.teile[fu.teil].stufen[0].ecken[fu.ecke]};
            for (int w = 0; w < 2; ++w) {
                for (int k = 0; k < 4; ++k) {
                    if (!(paar[w]->gewicht[k] > 0.0F)) { continue; }
                    float aus[3];
                    haut[paar[w]->knochen[k]].transform(paar[w]->ort, aus);
                    for (int r = 0; r < 3; ++r) { ort[w][r] += static_cast<double>(paar[w]->gewicht[k]) * static_cast<double>(aus[r]); }
                }
            }
            const double d = std::sqrt((ort[0][0] - ort[1][0]) * (ort[0][0] - ort[1][0]) + (ort[0][1] - ort[1][1]) * (ort[0][1] - ort[1][1]) +
                                       (ort[0][2] - ort[1][2]) * (ort[0][2] - ort[1][2]));
            if (d > fu.weit) {
                fu.weit = d;
                fu.bild = bild;
            }
        }
    }
    std::sort(funde.begin(), funde.end(), [](const Fund& a, const Fund& c) { return a.weit > c.weit; });
    const auto liste = [&](const Ecke& e) {
        std::string z;
        for (int k = 0; k < 4; ++k) {
            if (e.gewicht[k] > 0.0F) { z += g.bones[e.knochen[k]].name + " " + f("%.3f", static_cast<double>(e.gewicht[k])) + "  "; }
        }
        return z;
    };
    std::printf("%s (%s): die 12 Ecken mit dem groessten Abstand ueber %d Bilder\n", figur.c_str(), verfahren.empty() ? "voxel" : verfahren.c_str(),
                static_cast<int>(bilder.size()));
    for (std::size_t i = 0; i < funde.size() && i < 12U; ++i) {
        const Fund& fu = funde[i];
        const Ecke& r = raven.teile[fu.teil].stufen[0].ecken[fu.ecke];
        std::string anim = "?";
        for (const AnimEntry& a : cfg) {
            if (fu.bild >= a.firstFrame && fu.bild < a.firstFrame + a.numFrames) { anim = a.name; }
        }
        std::printf("  %6.2f Einheiten im Bild %d (%s), %s Ecke %d bei %.1f %.1f %.1f\n      Raven: %s\n      neu:   %s\n", fu.weit, fu.bild, anim.c_str(),
                    raven.teile[fu.teil].name.c_str(), static_cast<int>(fu.ecke), static_cast<double>(r.ort[0]), static_cast<double>(r.ort[1]),
                    static_cast<double>(r.ort[2]), liste(r).c_str(), liste(neu.teile[fu.teil].stufen[0].ecken[fu.ecke]).c_str());
    }
    return 0;
}

// Messung fuer die Vorgaben (kein Test): modellautoskintest <JA base> --suche [<figur> ...]
// Je Steifigkeit, Glaetten und Schwelle das Mittel ueber die Figuren (Vorgabe: kyle, luke, stormtrooper - die uebrigen
// Figuren des Tests dienen als Gegenprobe und werden hier nicht gemessen).
int sucheEinstellungen(const std::string& jaBase, const std::vector<std::string>& wahl) {
    Spiel sp;
    sp.ordner.resize(1);
    std::string bytes;
    GlaAnimation g;
    if (!scanGamePath(jaBase, sp.ordner[0]) || !sp.lies("models/players/_humanoid/_humanoid.gla", bytes) || !readGla(bytes, g)) { return 1; }
    std::string cfgText;
    (void)sp.lies("models/players/_humanoid/animation.cfg", cfgText);
    const std::vector<int> bilder = spielBilder(parseAnimationCfg(cfgText), g.numFrames);
    std::vector<Inhalt> figuren;
    std::vector<AutoSkinFeld> felder;
    for (const std::string& name : wahl.empty() ? std::vector<std::string>{"kyle", "luke", "stormtrooper"} : wahl) {
        Inhalt in;
        if (!ladeModell(sp, "models/players/" + name + "/model.glm", in)) { continue; }
        felder.push_back(autoSkinFeld(in, g, AutoSkinOptionen{}));
        figuren.push_back(std::move(in));
    }
    std::printf("Suche ueber %d Figuren, %d Bilder\n", static_cast<int>(figuren.size()), static_cast<int>(bilder.size()));
    std::printf("steif exponent glaetten schwelle | staerkster gleich %% | Gewichtsabweichung | Verformung Mittel | 95 %% | groesste\n");
    for (const float steif : {0.05F, 0.1F, 0.2F, 0.3F, 0.4F, 0.5F, 0.65F}) {
        for (const int glatt : {0, 1, 2}) {
            for (const float schwelle : {0.0F, 0.03F, 0.06F, 0.1F}) {
                AutoSkinOptionen o;
                o.steifigkeit = steif;
                o.glaetten = glatt;
                o.schwelle = schwelle;
                double haupt = 0.0;
                double abw = 0.0;
                double mittel = 0.0;
                double p95 = 0.0;
                double groesste = 0.0;
                for (std::size_t i = 0; i < figuren.size(); ++i) {
                    const Ergebnis r = messen("", figuren[i], g, bilder, o, nullptr, &felder[i]);
                    haupt += r.haupt;
                    abw += r.abweichung;
                    mittel += r.fehler.mittel;
                    p95 += r.fehler.p95;
                    groesste = std::max(groesste, r.fehler.max);
                }
                const double n = static_cast<double>(std::max<std::size_t>(figuren.size(), 1U));
                std::printf("%.2f %6.2f %d %.2f | %5.1f | %.4f | %.4f | %.4f | %.2f\n", static_cast<double>(steif), autoSkinExponent(steif), glatt,
                            static_cast<double>(schwelle), 100.0 * haupt / n, abw / n, mittel / n, p95 / n, groesste);
                std::fflush(stdout);
            }
        }
    }
    return 0;
}

// Messung fuer die Vorgabe uebertragenAbstand (kein Test): von kyle auf die Figuren (Vorgabe luke, stormtrooper, jawa) mit
// verschiedenen groessten Abstaenden; das Voxel-Feld je Figur einmal.
int sucheAbstand(const std::string& jaBase, const std::vector<std::string>& wahl) {
    Spiel sp;
    sp.ordner.resize(1);
    std::string bytes;
    GlaAnimation g;
    Inhalt kyle;
    if (!scanGamePath(jaBase, sp.ordner[0]) || !sp.lies("models/players/_humanoid/_humanoid.gla", bytes) || !readGla(bytes, g) ||
        !ladeModell(sp, "models/players/kyle/model.glm", kyle)) {
        return 1;
    }
    std::string cfgText;
    (void)sp.lies("models/players/_humanoid/animation.cfg", cfgText);
    const std::vector<int> bilder = spielBilder(parseAnimationCfg(cfgText), g.numFrames);
    std::printf("Abstand | je Figur: staerkster gleich %%, Verformung Mittel, Punkte mit Voxel-Gewichten\n");
    for (const std::string& name : wahl.empty() ? std::vector<std::string>{"luke", "stormtrooper", "jawa"} : wahl) {
        Inhalt in;
        if (!ladeModell(sp, "models/players/" + name + "/model.glm", in)) { continue; }
        const AutoSkinFeld feld = autoSkinFeld(in, g, AutoSkinOptionen{});
        for (const float abstand : {0.25F, 0.5F, 1.0F, 2.0F, 3.0F, 4.0F, 8.0F, 1000.0F}) {
            AutoSkinOptionen o;
            o.verfahren = AutoSkinVerfahren::Uebertragen;
            o.uebertragenAbstand = abstand;
            const Ergebnis r = messen(name, in, g, bilder, o, &kyle, &feld);
            std::printf("%-14s %7.2f | %5.1f | %.4f | 95 %% %.4f | groesste %.2f | %d von %d\n", name.c_str(), static_cast<double>(abstand), 100.0 * r.haupt,
                        r.fehler.mittel, r.fehler.p95, r.fehler.max, r.bericht.fern, r.bericht.punkte);
            std::fflush(stdout);
        }
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc > 2 && std::string(argv[2]) == "--abstand") {
        std::vector<std::string> wahl;
        for (int i = 3; i < argc; ++i) { wahl.emplace_back(argv[i]); }
        return sucheAbstand(argv[1], wahl);
    }
    if (argc > 3 && std::string(argv[2]) == "--ausreisser") { return ausreisser(argv[1], argv[3], argc > 4 ? argv[4] : ""); }
    if (argc > 2 && std::string(argv[2]) == "--suche") {
        std::vector<std::string> wahl;
        for (int i = 3; i < argc; ++i) { wahl.emplace_back(argv[i]); }
        return sucheEinstellungen(argv[1], wahl);
    }
    pruefeFormen();
    pruefeVoxel();
    pruefeGewichte();
    pruefeGrenzen();
    pruefeUebertragen();
    pruefeVergleich();
    if (argc > 1 && argv[1][0] != '\0') {
        pruefeRaven(argv[1]);
    } else {
        std::printf("Raven: uebersprungen (kein <JA base>)\n");
    }
    std::printf("modellautoskintest: %d Pruefungen, %d Fehler\n", pruefungen, fails);
    return fails == 0 ? 0 : 1;
}
