// modell_posiert.cpp - mbvers: Bearbeiten in der Pose (bhed/modell/posiert.h)
//
//   Bindepose     ein Skelett (ein Elternteil steht hinter seinem Kind) mit der Einheit als lokaler Matrix: die Welt aus
//                 worldMatrices und worldMatricesLerp (auch zwischen zwei Bildern gemischt) ist exakt die Bindepose; ein
//                 Knochen um 1/64 verschoben oder eine falsche Zahl von Matrizen: nicht
//   Mischung      zwei Knochen gemischt: gezeigt -> Bindepose -> gezeigt kommt am selben Ort an (Rechengenauigkeit von
//                 float); ortGezeigt -> ortZurueck (double) kommt bis auf 1e-12 an derselben Ecke an, auch bei einer 0
//   nicht umkehrbar  Determinante 0 (zwei Knochen genau gegeneinander, je halb) und fast 0 (der Ort laege ausserhalb der
//                 Welt): nach dem staerksten Knochen, keine NaN; ohne gueltigen Knochen: Rueckweg::Keiner, die Ecke bleibt
//   Normale       der erste Knochen dreht sie (G2_SkinSurface), auch mit Gewicht 0; hin und zurueck
//   Arbeitsnetz   Kasten auf einem um 90 Grad gedrehten, verschobenen Knochen: in die Pose, Extrude 8 nach oben (gezeigt),
//                 zurueck - unveraenderte Punkte bitgleich, die Kappe 8 entlang der Normale der Bindepose, ihre Normalen
//                 zurueckgedreht; ohne Knochen aendert nichts
//
// Aufruf: modellposierttest
#include "bhed/gla.h"
#include "bhed/glm.h"
#include "bhed/modell/posiert.h"
#include "bhed/modell/wof/poly.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {
int fails = 0;
void expect(const std::string& what, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", what.c_str());
    if (!ok) { ++fails; }
}
using namespace bhed;
using namespace bhed::modell;
using namespace bhed::modell::wof;

// Rechengenauigkeit: alle Orte dieses Tests liegen unter 64, dort ist ein float-Schritt (ulp) 2^-18 = 3,8e-6.
// gezeigterOrt rundet je Knochen bei drei Produkten und drei Summen je Achse, beim Gewichten und beim Summieren - mit
// zwei Knochen hoechstens 16 Rundungen; die Umkehrung rechnet in double und rundet einmal (ein halber Schritt).
constexpr float kUlp64 = 1.0F / 262144.0F;   // 2^-18
constexpr float kRundlauf = 16.0F * kUlp64;  // 6,1e-5

BoneMatrix drehZ(float grad, float tx, float ty, float tz) {
    const float w = grad * 3.14159265F / 180.0F;
    BoneMatrix m = BoneMatrix::identity();
    m.m[0][0] = std::cos(w);
    m.m[0][1] = -std::sin(w);
    m.m[1][0] = std::sin(w);
    m.m[1][1] = std::cos(w);
    m.m[0][3] = tx;
    m.m[1][3] = ty;
    m.m[2][3] = tz;
    return m;
}

BoneMatrix drehX(float grad, float tx, float ty, float tz) {
    const float w = grad * 3.14159265F / 180.0F;
    BoneMatrix m = BoneMatrix::identity();
    m.m[1][1] = std::cos(w);
    m.m[1][2] = -std::sin(w);
    m.m[2][1] = std::sin(w);
    m.m[2][2] = std::cos(w);
    m.m[0][3] = tx;
    m.m[1][3] = ty;
    m.m[2][3] = tz;
    return m;
}

// Die Umkehrung einer starren Matrix (Drehung transponiert)
BoneMatrix starrUmgekehrt(const BoneMatrix& m) {
    BoneMatrix o;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) { o.m[r][c] = m.m[c][r]; }
    }
    for (int r = 0; r < 3; ++r) { o.m[r][3] = -(o.m[r][0] * m.m[0][3] + o.m[r][1] * m.m[1][3] + o.m[r][2] * m.m[2][3]); }
    return o;
}

GlmVertex ecke(float x, float y, float z, int b0, float w0, int b1, float w1) {
    GlmVertex v;
    v.xyz[0] = x;
    v.xyz[1] = y;
    v.xyz[2] = z;
    v.bones[0] = static_cast<std::uint8_t>(b0);
    v.weights[0] = w0;
    v.bones[1] = static_cast<std::uint8_t>(b1);
    v.weights[1] = w1;
    return v;
}

void bindeposeTests() {
    std::printf("Bindepose\n");
    // Knochen 0 haengt an Knochen 2 (Elternteil hinter dem Kind, wie acht Knochen der _humanoid.gla), 1 an 0
    GlaAnimation g;
    g.bones.resize(3);
    g.bones[2].parent = -1;
    g.bones[2].basePose = drehZ(30.0F, 1.0F, 2.0F, 24.0F);
    g.bones[0].parent = 2;
    g.bones[0].basePose = drehX(-70.0F, 3.5F, -2.25F, 31.0F);
    g.bones[1].parent = 0;
    g.bones[1].basePose = drehZ(115.0F, 9.0F, 0.5F, 40.0F);
    for (GlaBone& b : g.bones) { b.basePoseInv = starrUmgekehrt(b.basePose); }
    g.numFrames = 3;
    g.roh.assign(9U, BoneMatrix::identity());   // Bild 0 und 1: die Einheit
    g.roh[2U * 3U + 1U].m[0][3] = 1.0F / 64.0F;   // Bild 2: Knochen 1 um den kleinsten Schritt der Datei verschoben
    std::vector<BoneMatrix> welt;
    g.worldMatrices(0, welt);
    expect("worldMatrices, lokale Matrizen die Einheit: Bindepose", bindepose(g.bones, welt));
    g.worldMatricesLerp(0, 1, 0.3F, welt);
    expect("worldMatricesLerp zwischen zwei solchen Bildern (Anteil 0,3): Bindepose", bindepose(g.bones, welt));
    // die Skin-Matrizen selbst sind dabei nur gerundet die Einheit - darum prueft bindepose() die Kette, nicht sie
    bool skinExakt = true;
    for (std::size_t b = 0; b < 3U; ++b) {
        const BoneMatrix s = welt[b] * g.bones[b].basePoseInv;
        const BoneMatrix e = BoneMatrix::identity();
        skinExakt = skinExakt && std::memcmp(&s, &e, sizeof s) == 0;
    }
    std::printf("        (Skin-Matrizen dieser Pose bitgleich die Einheit: %s)\n", skinExakt ? "ja" : "nein");
    g.worldMatricesLerp(2, 2, 0.0F, welt);
    expect("ein Knochen um 1/64 verschoben: nicht die Bindepose", !bindepose(g.bones, welt));
    g.worldMatricesLerp(0, 2, 0.5F, welt);
    expect("halb zu diesem Bild gemischt: nicht die Bindepose", !bindepose(g.bones, welt));
    g.worldMatrices(0, welt);
    welt.pop_back();
    expect("eine Matrix zu wenig: nicht die Bindepose", !bindepose(g.bones, welt));
    expect("kein Skelett, keine Matrizen: Bindepose", bindepose({}, {}));
}

void mischungTests() {
    std::printf("Mischung\n");
    const std::vector<BoneMatrix> k = {BoneMatrix::identity(), drehZ(90.0F, 10.0F, -4.0F, 3.0F), drehZ(180.0F, 0.0F, 0.0F, 0.0F), drehZ(120.0F, 0.0F, 0.0F, 0.0F)};
    float schlimmst = 0.0F;
    bool alleExakt = true;
    double nichts = 0.0;
    for (int i = 0; i < 20; ++i) {
        const float w = 0.05F * static_cast<float>(i);
        GlmVertex v = ecke(12.0F + static_cast<float>(i), i % 2 == 0 ? -7.0F : 0.0F, 30.0F - static_cast<float>(i), 0, 1.0F - w, 1, w);
        if (w <= 0.0F) { v.weights[1] = 0.0F; }
        const Mischung m = mischung(k, v);
        // ein gezeigter Ort (um 5 und -3 verschoben) zurueck und wieder gezeigt
        float p[3];
        gezeigterOrt(k, v, p);
        const double ziel[3] = {static_cast<double>(p[0]) + 5.0, static_cast<double>(p[1]), static_cast<double>(p[2]) - 3.0};
        GlmVertex z = v;
        alleExakt = alleExakt && ortZurueck(m, ziel, z.xyz) == Rueckweg::Exakt;
        float q[3];
        gezeigterOrt(k, z, q);
        for (int a = 0; a < 3; ++a) { schlimmst = std::max(schlimmst, std::fabs(q[a] - static_cast<float>(ziel[a]))); }
        // der gezeigte Ort in double und gleich zurueck: dieselbe Ecke bis auf die Rundung der Umkehrung in double
        double pd[3];
        ortGezeigt(m, v.xyz, pd);
        float u[3];
        (void)ortZurueck(m, pd, u);
        for (int a = 0; a < 3; ++a) { nichts = std::max(nichts, std::fabs(static_cast<double>(u[a]) - static_cast<double>(v.xyz[a]))); }
    }
    char b[160];
    std::snprintf(b, sizeof b, "gezeigt -> Bindepose -> gezeigt ueber 20 Mischungen 0..0,95: %.2e <= %.2e (16 float-Schritte)", static_cast<double>(schlimmst),
                  static_cast<double>(kRundlauf));
    expect(b, alleExakt && schlimmst <= kRundlauf);
    std::snprintf(b, sizeof b, "ortGezeigt -> ortZurueck (double) an denselben 20 Ecken, jede zweite mit y = 0: %.2e <= 1e-12", nichts);
    expect(b, nichts <= 1e-12);

    // 120 Grad gegeneinander, je halb: auf die Haelfte gestaucht (cos 60), aber umkehrbar - exakt
    {
        const GlmVertex v = ecke(8.0F, 3.0F, 5.0F, 0, 0.5F, 3, 0.5F);
        const Mischung m = mischung(k, v);
        float p[3];
        gezeigterOrt(k, v, p);
        const double ziel[3] = {static_cast<double>(p[0]) + 2.0, static_cast<double>(p[1]) - 1.0, static_cast<double>(p[2])};
        GlmVertex z = v;
        const Rueckweg weg = ortZurueck(m, ziel, z.xyz);
        float q[3];
        gezeigterOrt(k, z, q);
        const float f = std::max({std::fabs(q[0] - static_cast<float>(ziel[0])), std::fabs(q[1] - static_cast<float>(ziel[1])), std::fabs(q[2] - static_cast<float>(ziel[2]))});
        std::snprintf(b, sizeof b, "120 Grad gegeneinander, je halb (gestaucht auf die Haelfte): exakt umgekehrt, %.2e <= %.2e", static_cast<double>(f),
                      static_cast<double>(kRundlauf));
        expect(b, weg == Rueckweg::Exakt && f <= kRundlauf);
    }

    // Determinante genau 0: Einheit und Drehung um genau 180 Grad um Z, je halb
    {
        std::vector<BoneMatrix> g = {BoneMatrix::identity(), BoneMatrix::identity()};
        g[1].m[0][0] = -1.0F;
        g[1].m[1][1] = -1.0F;
        g[1].m[0][3] = 2.0F;
        const Mischung halb = mischung(g, ecke(1.0F, 2.0F, 3.0F, 0, 0.5F, 1, 0.5F));
        float v[3] = {1.0F, 2.0F, 3.0F};
        const double p[3] = {4.0, 5.0, 6.0};
        const Rueckweg weg = ortZurueck(halb, p, v);
        expect("Determinante 0: nicht umkehrbar, nach dem ersten der gleich starken Knochen (hier die Einheit), keine NaN",
               !halb.umkehrbar && halb.hatStark && weg == Rueckweg::Staerkster && v[0] == 4.0F && v[1] == 5.0F && v[2] == 6.0F);
    }
    // fast 0: drehZ(180) aus float-Pi hat sin = -8,7e-8 - die Umkehrung gaebe einen Ort bei 1e8, ausserhalb der Welt
    {
        const Mischung fast = mischung(k, ecke(1.0F, 2.0F, 3.0F, 0, 0.5F, 2, 0.5F));
        float v[3] = {1.0F, 2.0F, 3.0F};
        const double p[3] = {4.0, 5.0, 6.0};
        const Rueckweg weg = ortZurueck(fast, p, v);
        expect("Determinante fast 0 (der Ort laege ausserhalb von MAX_WORLD_COORD): nach dem staerksten Knochen, endlich",
               fast.umkehrbar && weg == Rueckweg::Staerkster && std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2]) && std::fabs(v[0] - 4.0F) < 1e-6F &&
                   std::fabs(v[1] - 5.0F) < 1e-6F && v[2] == 6.0F);
    }
    {
        const Mischung keiner = mischung(k, ecke(1.0F, 2.0F, 3.0F, 7, 1.0F, 9, 0.0F));
        float v[3] = {1.0F, 2.0F, 3.0F};
        const double p[3] = {4.0, 5.0, 6.0};
        expect("ohne gueltigen Knochen (Nummer ausserhalb): Rueckweg::Keiner, die Ecke bleibt",
               ortZurueck(keiner, p, v) == Rueckweg::Keiner && v[0] == 1.0F && v[1] == 2.0F && v[2] == 3.0F);
    }

    // Normale: Knochen 1 (90 Grad um Z) steht vorn mit Gewicht 0, die Ecke haengt ganz an Knochen 0
    const Mischung n = mischung(k, ecke(0.0F, 0.0F, 0.0F, 1, 0.0F, 0, 1.0F));
    const float nx[3] = {1.0F, 0.0F, 0.0F};
    double gezeigt[3];
    float wieder[3];
    normaleGezeigt(n, nx, gezeigt);
    normaleZurueck(n, gezeigt, wieder);
    expect("Normale: der erste Knochen dreht sie (auch mit Gewicht 0): (1,0,0) -> (0,1,0) und zurueck",
           std::fabs(gezeigt[0]) < 1e-6 && std::fabs(gezeigt[1] - 1.0) < 1e-6 && wieder[0] == 1.0F && std::fabs(wieder[1]) < 1e-12F && wieder[2] == 0.0F && n.a[0][0] == 1.0);
}

PolyNetz kasten() {
    PolyNetz n;
    n.teilBeruehrt.assign(1U, 0);
    n.teilPunkte.assign(1U, 0);
    GrundParam g;
    g.form = Grundform::Kasten;
    g.laenge = g.breite = g.hoehe = 16.0F;
    (void)grundkoerper(n, g, 0);
    n.verdichten();
    for (PFlaeche& f : n.flaechen) {
        for (PEcke& c : f.ecken) {
            c.e.knochen[0] = 1;   // starr auf Knochen 1
            c.e.gewicht[0] = 1.0F;
        }
    }
    return n;
}

int obenVieleck(const PolyNetz& n) {
    for (std::size_t f = 0; f < n.flaechen.size(); ++f) {
        bool alle = true;
        for (const PEcke& e : n.flaechen[f].ecken) { alle = alle && std::fabs(n.punkte[static_cast<std::size_t>(e.punkt)].ort[2] - 16.0F) < 1e-4F; }
        if (alle && !n.flaechen[f].weg) { return static_cast<int>(f); }
    }
    return -1;
}

void netzTests() {
    std::printf("Arbeitsnetz\n");
    const std::vector<BoneMatrix> k = {BoneMatrix::identity(), drehX(90.0F, 20.0F, 5.0F, -8.0F)};
    PolyNetz n = kasten();
    const PolyNetz vorher = n;
    const int oben = obenVieleck(n);
    expect("Kasten mit Deckel bei z = 16", oben >= 0);
    const NetzPose pose = inPose(n, k);
    expect("in der Pose: aktiv, ein Punkt gezeigt gedreht", pose.aktiv && std::fabs(n.punkte[0].ort[0] - (vorher.punkte[0].ort[0] + 20.0F)) < 1e-4F);
    const Plan p = extrudeFlaechen(n, {oben}, ExtrudeArt::Gruppe);
    betragAnwenden(n, p, 8.0F);
    const std::vector<char> neu = normalenPunkte(n);
    normalenNeu(n, 60.0F);
    const int nichtUmkehrbar = ausPose(n, pose, k, neu);
    expect("zurueck: jede Mischung umkehrbar", nichtUmkehrbar == 0);
    bool gleich = true;
    for (std::size_t i = 0; i < vorher.punkte.size(); ++i) {
        const bool deckel = std::fabs(vorher.punkte[i].ort[2] - 16.0F) < 1e-4F;
        if (!deckel) { gleich = gleich && std::memcmp(n.punkte[i].ort, vorher.punkte[i].ort, sizeof vorher.punkte[i].ort) == 0; }
    }
    expect("unveraenderte Punkte bitgleich wie vor der Pose", gleich);
    float hoch = 1e30F;
    float tief = -1e30F;
    int kappe = 0;
    bool normaleOben = true;
    for (const PFlaeche& f : n.flaechen) {
        if (f.weg) { continue; }
        bool alleOben = true;
        for (const PEcke& c : f.ecken) { alleOben = alleOben && n.punkte[static_cast<std::size_t>(c.punkt)].ort[2] > 20.0F; }
        if (!alleOben) { continue; }
        ++kappe;
        for (const PEcke& c : f.ecken) {
            hoch = std::min(hoch, n.punkte[static_cast<std::size_t>(c.punkt)].ort[2]);
            tief = std::max(tief, n.punkte[static_cast<std::size_t>(c.punkt)].ort[2]);
            normaleOben = normaleOben && std::fabs(c.e.normale[2] - 1.0F) < 1e-6F;
        }
    }
    char b[160];
    std::snprintf(b, sizeof b, "Kappe gezeigt entlang der gezeigten Normale extrudiert = in der Bindepose z 24 (%.6f .. %.6f, <= %.2e)", static_cast<double>(hoch),
                  static_cast<double>(tief), static_cast<double>(kRundlauf));
    expect(b, kappe == 1 && std::fabs(hoch - 24.0F) <= kRundlauf && std::fabs(tief - 24.0F) <= kRundlauf);
    expect("Normalen der Kappe zurueckgedreht: (0, 0, 1)", normaleOben);

    PolyNetz r = kasten();
    const PolyNetz r0 = r;
    const NetzPose ruhe = inPose(r, {});
    bool ruheGleich = !ruhe.aktiv && ausPose(r, ruhe, {}, {}) == 0 && r.punkte.size() == r0.punkte.size();
    for (std::size_t i = 0; ruheGleich && i < r0.punkte.size(); ++i) { ruheGleich = std::memcmp(r.punkte[i].ort, r0.punkte[i].ort, sizeof r0.punkte[i].ort) == 0; }
    expect("ohne Knochen: nicht aktiv, nichts veraendert", ruheGleich);
}

}  // namespace

int main() {
    bindeposeTests();
    mischungTests();
    netzTests();
    std::printf("%s: %d Fehler\n", fails == 0 ? "OK" : "FEHLER", fails);
    return fails == 0 ? 0 : 1;
}
