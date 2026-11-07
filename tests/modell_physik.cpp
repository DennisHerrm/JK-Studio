// modell_physik.cpp - Modell-Block Stufe 6: Auto-Physik (bhed/modell/physik.h)
//
//   Massen         Summe der Anteile 1 (Winter Tab. 4.1); Schwerpunkt der Grundstellung des JKA-Skeletts gleich der
//                  Rechnung von Hand aus der Knochentabelle; verschoben -> um dasselbe verschoben
//   Stuetzflaeche  Stand gerade: Schwerpunkt innen, keine Warnung; Oberkoerper 90 Grad vorgebeugt: Warnung (Rot-Probe "Absprung
//                  mit Schwerpunkt ausserhalb der Stuetzflaeche messbar")
//   Parabel        Sprung als Dreieck (Becken linear hoch und runter): ohne Physik weicht der Schwerpunkt messbar von der
//                  Parabel ab (Rot-Probe), mit Physik liegt er auf c(a) + v0 t - g t^2 / 2 (g = 800), Absprung und Landung
//                  aus den Kontakten und von Hand
//   Drehimpuls     im Flug schwingt ein Arm: ohne Physik aendert sich L um den Schwerpunkt, mit Physik ist er konstant; Lage
//                  bei Absprung und Landung bleibt
//   Fusskontakt    das Becken wandert bei gebeugten Knien, die Fuesse rutschen ohne Physik mit, mit Physik nicht
//   Federkette     ltail (Rockschoss) folgt einem Ruck des Beckens gedaempft: Ueberschwingen, Huellkurve faellt, am Ende ruhig
//   Filter         aus / Staerke 0 -> gebacken Byte fuer Byte wie ohne Physik; die Schluessel der Schicht bleiben unangetastet;
//                  an -> die gebackene GLA traegt die Parabel
//
// Aufruf: modellphysiktest
#include "bhed/gla.h"
#include "bhed/modell/animkurven.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/ik.h"
#include "bhed/modell/physik.h"
#include "bhed/modell/rig.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
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

void eintrag(std::vector<std::uint8_t>& pool, float tx, float ty, float tz) {
    const std::uint16_t e[7] = {3 * 16383, 2 * 16383, 2 * 16383, 2 * 16383, static_cast<std::uint16_t>(std::lround((tx + 512.0F) * 64.0F)),
                                static_cast<std::uint16_t>(std::lround((ty + 512.0F) * 64.0F)), static_cast<std::uint16_t>(std::lround((tz + 512.0F) * 64.0F))};
    for (const std::uint16_t v : e) {
        pool.push_back(static_cast<std::uint8_t>(v & 0xFF));
        pool.push_back(static_cast<std::uint8_t>(v >> 8));
    }
}

// Das JKA-Skelett der Hand-Tabelle; das Becken geht in Bild f um weg(f) (Wurzelbewegung der GLA)
GlaAnimation skelett(int bilder, const std::function<std::array<float, 3>(int)>& weg) {
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
    eintrag(g.bonePool, 0.0F, 0.0F, 0.0F);
    for (int f = 0; f < bilder; ++f) {
        const std::array<float, 3> w = weg(f);
        eintrag(g.bonePool, w[0], w[1], w[2]);
    }
    g.numFrames = bilder;
    g.indexes.assign(static_cast<std::size_t>(bilder) * g.bones.size(), 0U);
    for (int f = 0; f < bilder; ++f) {
        g.indexes[static_cast<std::size_t>(f) * g.bones.size() + static_cast<std::size_t>(becken)] = static_cast<std::uint32_t>(f + 1);
    }
    return g;
}

int knochen(const GlaAnimation& g, const char* n) { return boneIndex(g, n); }

// Das Rig wie mit Modell: Ferse, Ballen und Spitze je Fuss (rigAnlegen nimmt sie aus den Ecken des Fusses; hier von Hand
// unter dem Knoechel, vorn ist -Y wie im Skelett der Spiele)
Rig rigMitFuss(const GlaAnimation& g) {
    Rig r = rigAnlegen(g, Inhalt{}, 0, {});
    for (const char* sd : {"l", "r"}) {
        const int k = knochen(g, (std::string(sd) + "talus").c_str());
        const BoneMatrix& b = g.bones[static_cast<std::size_t>(k)].basePose;
        const float x = b.m[0][3];
        const float y = b.m[1][3];
        const Seite seite = sd[0] == 'l' ? Seite::Links : Seite::Rechts;
        const float orte[3][3] = {{x, y + 3.0F, 0.0F}, {x, y - 5.0F, 0.5F}, {x, y - 8.0F, 0.0F}};
        const GriffArt arten[3] = {GriffArt::Ferse, GriffArt::Ballen, GriffArt::Zehe};
        for (int i = 0; i < 3; ++i) {
            Griff gr;
            gr.art = arten[i];
            gr.seite = seite;
            gr.knochen = k;
            gr.ik = true;
            std::memcpy(gr.ort, orte[i], sizeof gr.ort);
            r.griffe.push_back(gr);
        }
    }
    return r;
}

Vec3 ortVon(const BoneMatrix& m) { return {m.m[0][3], m.m[1][3], m.m[2][3]}; }

float abstand(const Vec3& a, const Vec3& b) { return std::hypot(a[0] - b[0], a[1] - b[1], a[2] - b[2]); }

// Boden: der tiefste Fusspunkt der Grundstellung (Bild 0)
float bodenVon(const GlaAnimation& g, const MassenModell& mm, const RigSchicht& s, int bild) {
    std::vector<BoneMatrix> w;
    rigWelt(g, s, bild, bild, 0.0F, w);
    float z = 1e30F;
    for (const auto& seite : mm.fuss) {
        for (const PhysikPunkt& p : seite) { z = std::min(z, punktWelt(g, p, w)[2]); }
    }
    return z;
}

// Groesste Abweichung des Schwerpunkts von der Parabel durch c(a) und c(l) mit g
float parabelAbweichung(const GlaAnimation& g, const MassenModell& mm, const RigSchicht& s, int a, int l, double dt, double gz) {
    std::vector<BoneMatrix> w;
    rigWelt(g, s, a, a, 0.0F, w);
    const Vec3 ca = schwerpunkt(g, mm, w);
    rigWelt(g, s, l, l, 0.0F, w);
    const Vec3 cl = schwerpunkt(g, mm, w);
    const double tt = (l - a) * dt;
    float abw = 0.0F;
    for (int f = a; f <= l; ++f) {
        const double t = (f - a) * dt;
        Vec3 p{};
        for (int k = 0; k < 3; ++k) {
            const double gk = k == 2 ? -gz : 0.0;
            const double v0 = (cl[static_cast<std::size_t>(k)] - ca[static_cast<std::size_t>(k)] - 0.5 * gk * tt * tt) / tt;
            p[static_cast<std::size_t>(k)] = static_cast<float>(ca[static_cast<std::size_t>(k)] + v0 * t + 0.5 * gk * t * t);
        }
        rigWelt(g, s, f, f, 0.0F, w);
        abw = std::max(abw, abstand(schwerpunkt(g, mm, w), p));
    }
    return abw;
}

// Schwankung von L im Flug, bezogen auf das groesste |L|
float drehimpulsSchwankung(const GlaAnimation& g, const MassenModell& mm, const RigSchicht& s, int a, int l, double dt) {
    std::vector<Vec3> ls;
    std::vector<BoneMatrix> w0;
    std::vector<BoneMatrix> w1;
    for (int f = a; f < l; ++f) {
        rigWelt(g, s, f, f, 0.0F, w0);
        rigWelt(g, s, f + 1, f + 1, 0.0F, w1);
        ls.push_back(drehimpuls(g, mm, w0, w1, dt));
    }
    Vec3 m{};
    float gross = 1e-9F;
    for (const Vec3& x : ls) {
        for (int k = 0; k < 3; ++k) { m[static_cast<std::size_t>(k)] += x[static_cast<std::size_t>(k)] / static_cast<float>(ls.size()); }
        gross = std::max(gross, std::hypot(x[0], x[1], x[2]));
    }
    float abw = 0.0F;
    for (const Vec3& x : ls) { abw = std::max(abw, abstand(x, m) / gross); }
    return abw;
}

float drehWinkel(const BoneMatrix& a, const BoneMatrix& b) {
    const BoneMatrix ra = starr(a);
    const BoneMatrix rb = starr(b);
    double tr = 0.0;
    for (int i = 0; i < 3; ++i) {
        for (int k = 0; k < 3; ++k) { tr += static_cast<double>(ra.m[k][i]) * rb.m[k][i]; }
    }
    return static_cast<float>(std::acos(std::clamp((tr - 1.0) * 0.5, -1.0, 1.0)) * 180.0 / 3.14159265358979);
}

}  // namespace

int main() {
    const double dt = 0.05;   // 20 fps (frameLerp 50 ms)

    // --- Massen und Schwerpunkt bekannter Posen --------------------------------------------------------------------------
    {
        const GlaAnimation g = skelett(2, [](int) { return std::array<float, 3>{0.0F, 0.0F, 0.0F}; });
        const Rig r = rigMitFuss(g);
        const MassenModell mm = massenModell(g, r);
        expect("Massenmodell vollstaendig (" + mm.fehler + ")", mm.gueltig() && mm.segmente.size() == 14);
        float summe = 0.0F;
        for (const PhysikSegment& sg : mm.segmente) { summe += sg.anteil; }
        expect("Summe der Massenanteile 1 (" + std::to_string(summe) + ")", std::fabs(summe - 1.0F) < 1e-5F);
        // Von Hand aus der Tabelle (Grundstellung: Welt = Grundstellung)
        const auto o = [&](const char* n) {
            const BoneMatrix& b = g.bones[static_cast<std::size_t>(knochen(g, n))].basePose;
            return ortVon(b);
        };
        const auto auf = [](const Vec3& a, const Vec3& b, float t) { return Vec3{a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t}; };
        const auto mitte = [](const Vec3& a, const Vec3& b) { return Vec3{(a[0] + b[0]) / 2, (a[1] + b[1]) / 2, (a[2] + b[2]) / 2}; };
        double sp[3] = {0, 0, 0};
        const auto dazu = [&](const Vec3& p, double m) {
            for (int k = 0; k < 3; ++k) { sp[k] += m * p[static_cast<std::size_t>(k)]; }
        };
        for (const char* s : {"l", "r"}) {
            const std::string x(s);
            dazu(auf(o((x + "humerus").c_str()), o((x + "radius").c_str()), 0.436F), 0.028);
            dazu(auf(o((x + "radius").c_str()), o((x + "hand").c_str()), 0.430F), 0.016);
            dazu(auf(o((x + "femurYZ").c_str()), o((x + "tibia").c_str()), 0.433F), 0.100);
            dazu(auf(o((x + "tibia").c_str()), o((x + "talus").c_str()), 0.433F), 0.0465);
        }
        const Vec3 schulter = mitte(o("lhumerus"), o("rhumerus"));
        dazu(o("cranium"), 0.081);
        dazu(auf(mitte(o("lfemurYZ"), o("rfemurYZ")), schulter, 0.5F), 0.497);
        std::vector<BoneMatrix> w;
        g.worldMatrices(0, w);
        std::vector<Vec3> seg;
        segmentOrte(g, mm, w, seg);
        // Haende und Fuesse aus dem Modell (hier aus Fingern bzw. Zehen) - ihre Orte nimmt die Probe vom Modell
        for (std::size_t i = 0; i < mm.segmente.size(); ++i) {
            const std::string n = mm.segmente[i].name;
            if (n.rfind("hand", 0) == 0 || n.rfind("foot", 0) == 0) { dazu(seg[i], mm.segmente[i].anteil); }
        }
        const Vec3 c = schwerpunkt(g, mm, w);
        const Vec3 erwartet{static_cast<float>(sp[0]), static_cast<float>(sp[1]), static_cast<float>(sp[2])};
        char z[200];
        std::snprintf(z, sizeof z, "Schwerpunkt der Grundstellung %.3f %.3f %.3f gleich der Rechnung von Hand %.3f %.3f %.3f", c[0], c[1], c[2], erwartet[0], erwartet[1],
                      erwartet[2]);
        expect(z, abstand(c, erwartet) < 1e-3F);
        expect("Schwerpunkt symmetrisch (x = 0) und zwischen Becken und Brust", std::fabs(c[0]) < 1e-3F && c[2] > o("pelvis")[2] && c[2] < schulter[2]);
        // Verschoben (GLA: Becken + (5, -3, 7)) -> Schwerpunkt um dasselbe
        const GlaAnimation g2 = skelett(2, [](int) { return std::array<float, 3>{5.0F, -3.0F, 7.0F}; });
        std::vector<BoneMatrix> w2;
        g2.worldMatrices(0, w2);
        const Vec3 c2 = schwerpunkt(g2, mm, w2);
        expect("verschobene Pose: Schwerpunkt um (5, -3, 7)", abstand(c2, Vec3{c[0] + 5.0F, c[1] - 3.0F, c[2] + 7.0F}) < 2e-3F);
    }

    // --- Stuetzflaeche und Warnung --------------------------------------------------------------------------------------
    {
        const GlaAnimation g = skelett(10, [](int) { return std::array<float, 3>{0.0F, 0.0F, 0.0F}; });
        const Rig r = rigMitFuss(g);
        const MassenModell mm = massenModell(g, r);
        RigSchicht s;
        PhysikUmgebung u;
        u.bodenZ = bodenVon(g, mm, s, 0);
        PhysikBericht b;
        (void)physikAnalyse(g, r, s, 0, 10, u, b);
        expect("gerader Stand: beide Fuesse im Kontakt, Stuetzflaeche mit " + std::to_string(b.bilder[0].stuetz.size()) + " Ecken, keine Warnung",
               b.bilder[0].imStand && b.bilder[0].stuetz.size() >= 3 && b.warnungen == 0);
        // Rot: der Oberkoerper ab lower_lumbar 90 Grad nach vorn (um die Seitenachse X), ab Bild 5
        const float x[3] = {1.0F, 0.0F, 0.0F};
        const int lumbar = knochen(g, "lower_lumbar");
        rigSchluesselSetzen(s, lumbar, 4, BoneMatrix::identity());
        float vorn = 0.0F;
        for (const float grad : {90.0F, -90.0F}) {
            RigSchicht t = s;
            rigSchluesselSetzen(t, lumbar, 5, drehungUm(x, grad));
            PhysikBericht bt;
            (void)physikAnalyse(g, r, t, 0, 10, u, bt);
            if (bt.warnBild >= 0) {
                vorn = grad;
                b = bt;
            }
        }
        char z[200];
        std::snprintf(z, sizeof z, "vorgebeugt (%g Grad): Warnung ab Bild %d, Schwerpunkt %.1f ausserhalb der Stuetzflaeche", static_cast<double>(vorn), b.warnBild,
                      b.warnBild >= 0 ? static_cast<double>(b.bilder[static_cast<std::size_t>(b.warnBild)].abstand) : 0.0);
        expect(z, vorn != 0.0F && b.warnBild == 5 && b.bilder[5].abstand > 0.3F && !b.bilder[4].ausserhalb);
    }

    // --- Sprung: Parabel und Drehimpuls ---------------------------------------------------------------------------------
    {
        // Bild 0-9 Stand, 10-29 Dreieck bis 30 Einheiten hoch und 20 nach vorn, 30-39 Stand
        const auto weg = [](int f) {
            std::array<float, 3> w{0.0F, 0.0F, 0.0F};
            if (f > 9 && f < 30) { w[2] = 30.0F * (1.0F - std::fabs(static_cast<float>(f) - 19.5F) / 10.5F); }
            w[1] = f <= 9 ? 0.0F : (f >= 30 ? -20.0F : -20.0F * static_cast<float>(f - 9) / 21.0F);
            return w;
        };
        const GlaAnimation g = skelett(40, weg);
        const Rig r = rigMitFuss(g);
        const MassenModell mm = massenModell(g, r);
        RigSchicht s;
        // ein Arm schwingt im Flug (lhumerus um die Seitenachse, 0 -> 120 Grad zwischen Bild 12 und 26)
        const float x[3] = {1.0F, 0.0F, 0.0F};
        const int arm = knochen(g, "lhumerus");
        rigSchluesselSetzen(s, arm, 12, BoneMatrix::identity());
        rigSchluesselSetzen(s, arm, 26, drehungUm(x, 120.0F));
        PhysikUmgebung u;
        u.bodenZ = bodenVon(g, mm, s, 0);
        u.bildMs = 50.0;
        const RigSchicht ohne = s;
        PhysikBericht b;
        s.physik.e.an = true;
        s.physik.e.kontakte = false;
        const bool ok = physikRechnen(g, r, s, 0, 40, u, &b);
        const int a = b.fluege.empty() ? -1 : b.fluege[0].first;
        const int l = b.fluege.empty() ? -1 : b.fluege[0].second;
        expect("Flug aus den Kontakten: Absprung " + std::to_string(a) + ", Landung " + std::to_string(l), ok && b.fluege.size() == 1 && a >= 9 && a <= 12 && l >= 27 && l <= 30);
        if (a >= 0) {
            const float vorher = parabelAbweichung(g, mm, ohne, a, l, dt, 800.0);
            const float nachher = parabelAbweichung(g, mm, s, a, l, dt, 800.0);
            expect("Rot: ohne Physik weicht der Schwerpunkt " + std::to_string(vorher) + " von der Parabel ab", vorher > 5.0F);
            expect("mit Physik auf der Parabel (g 800): Abweichung " + std::to_string(nachher) + " (Bericht " + std::to_string(b.parabelAbw) + ")",
                   nachher < 0.01F && b.parabelAbw < 0.01F);
            const float lv = drehimpulsSchwankung(g, mm, ohne, a, l, dt);
            const float ln = drehimpulsSchwankung(g, mm, s, a, l, dt);
            expect("Rot: Drehimpuls ohne Physik schwankt um " + std::to_string(lv * 100.0F) + " %", lv > 0.2F);
            expect("Drehimpuls mit Physik konstant: Schwankung " + std::to_string(ln * 100.0F) + " % (" + std::to_string(b.iterationen) + " Iterationen)", ln < 0.01F);
            // Lage bei Absprung und Landung wie ohne Physik
            std::vector<BoneMatrix> w0;
            std::vector<BoneMatrix> w1;
            float rand = 0.0F;
            for (const int f : {a, l}) {
                rigWelt(g, ohne, f, f, 0.0F, w0);
                rigWelt(g, s, f, f, 0.0F, w1);
                for (std::size_t k = 0; k < w0.size(); ++k) { rand = std::max(rand, abstand(ortVon(w0[k]), ortVon(w1[k]))); }
            }
            expect("Absprung und Landung bleiben (" + std::to_string(rand) + ")", rand < 0.01F);
            // Original unangetastet
            bool gleich = s.knochen.size() == ohne.knochen.size();
            for (std::size_t k = 0; gleich && k < s.knochen.size(); ++k) {
                gleich = s.knochen[k].size() == ohne.knochen[k].size() &&
                         (s.knochen[k].empty() || std::memcmp(s.knochen[k].data(), ohne.knochen[k].data(), s.knochen[k].size() * sizeof(RigSchluessel)) == 0);
            }
            expect("die Schluessel der Schicht bleiben unangetastet, Physik in eigener Spur (" + std::to_string(s.physik.knochen.size()) + ")",
                   gleich && !s.physik.knochen.empty());
            // Schwerkraft der Karte
            RigSchicht m = ohne;
            m.physik.e.an = true;
            m.physik.e.kontakte = false;
            m.physik.e.schwerkraft = 400.0F;
            (void)physikRechnen(g, r, m, 0, 40, u, nullptr);
            expect("Schwerkraft 400 (Karte): Parabel mit 400", parabelAbweichung(g, mm, m, a, l, dt, 400.0) < 0.01F && parabelAbweichung(g, mm, m, a, l, dt, 800.0) > 1.0F);
            // Absprung und Landung von Hand
            RigSchicht h = ohne;
            h.physik.e.an = true;
            h.physik.e.kontakte = false;
            h.physik.e.absprung = 14;
            h.physik.e.landung = 24;
            PhysikBericht bh;
            (void)physikRechnen(g, r, h, 0, 40, u, &bh);
            expect("Absprung 14 / Landung 24 von Hand: Parabel dazwischen", bh.fluege.size() == 1 && bh.fluege[0].first == 14 && bh.fluege[0].second == 24 &&
                                                                                parabelAbweichung(g, mm, h, 14, 24, dt, 800.0) < 0.01F);
            // Staerke 0,5: dazwischen
            RigSchicht halb = s;
            halb.physik.e.staerke = 0.5F;
            const float ph = parabelAbweichung(g, mm, halb, a, l, dt, 800.0);
            expect("Staerke 0,5: Abweichung " + std::to_string(ph) + " zwischen beiden", ph > nachher && ph < vorher);

            // --- Filter: aus und Staerke 0 -> gebacken Byte fuer Byte wie ohne Physik; an -> Parabel in der GLA ------------
            GlaAnimation g0;
            GlaAnimation g1;
            GlaAnimation g2;
            (void)rigBacken(g, ohne, 0, 40, false, "x", g0);
            RigSchicht aus = s;
            aus.physik.e.an = false;
            (void)rigBacken(g, aus, 0, 40, false, "x", g1);
            expect("Filter aus: gebacken Byte fuer Byte wie ohne Physik", g0.bonePool == g1.bonePool && g0.indexes == g1.indexes);
            RigSchicht null = s;
            null.physik.e.staerke = 0.0F;
            (void)rigBacken(g, null, 0, 40, false, "x", g2);
            expect("Staerke 0: gebacken Byte fuer Byte wie ohne Physik", g0.bonePool == g2.bonePool && g0.indexes == g2.indexes);
            GlaAnimation gp;
            (void)rigBacken(g, s, 0, 40, false, "x", gp);
            const float gebacken = parabelAbweichung(gp, mm, RigSchicht{}, a, l, dt, 800.0);
            expect("Filter an: die gebackene GLA traegt die Parabel (" + std::to_string(gebacken) + ", Kompression 1/64)", gebacken < 0.1F && g0.bonePool != gp.bonePool);
            // Werkzeuge, die Posen lesen und Schluessel schreiben, nehmen die Schicht ohne Physik (sonst kaeme sie doppelt)
            RigSchicht sp1 = s;
            RigSchicht sp2 = ohne;
            sp2.physik.e = s.physik.e;
            sp2.physik.e.an = false;
            (void)animationSpiegeln(g, r, sp1, 0, 40);
            (void)animationSpiegeln(g, r, sp2, 0, 40);
            bool spiegelGleich = sp1.knochen.size() == sp2.knochen.size();
            for (std::size_t k = 0; spiegelGleich && k < sp1.knochen.size(); ++k) {
                spiegelGleich = sp1.knochen[k].size() == sp2.knochen[k].size() &&
                                (sp1.knochen[k].empty() || std::memcmp(sp1.knochen[k].data(), sp2.knochen[k].data(), sp1.knochen[k].size() * sizeof(RigSchluessel)) == 0);
            }
            RigSchicht h1 = s;
            RigSchicht h2 = sp2;
            h2.knochen = ohne.knochen;
            (void)handPoseSetzen(g, r, Seite::Links, -1, 20, h1);
            (void)handPoseSetzen(g, r, Seite::Links, -1, 20, h2);
            bool handGleich = h1.knochen.size() == h2.knochen.size();
            for (std::size_t k = 0; handGleich && k < h1.knochen.size(); ++k) {
                handGleich = h1.knochen[k].size() == h2.knochen[k].size() &&
                             (h1.knochen[k].empty() || std::memcmp(h1.knochen[k].data(), h2.knochen[k].data(), h1.knochen[k].size() * sizeof(RigSchluessel)) == 0);
            }
            expect("Spiegeln und Handpose schreiben die Schluessel ohne die Physik-Spur (Physik an = aus)", spiegelGleich && handGleich);
        }
    }

    // --- Fusskontakt ----------------------------------------------------------------------------------------------------
    {
        // Knie gebeugt (Schicht), das Becken wandert 6 Einheiten nach vorn - die Fuesse gingen mit
        const GlaAnimation g = skelett(25, [](int f) { return std::array<float, 3>{0.0F, -0.3F * static_cast<float>(std::min(f, 20)), 0.0F}; });
        const Rig r = rigMitFuss(g);
        const MassenModell mm = massenModell(g, r);
        RigSchicht s;
        const float x[3] = {1.0F, 0.0F, 0.0F};
        for (const char* sd : {"l", "r"}) {
            const std::string n(sd);
            rigSchluesselSetzen(s, knochen(g, (n + "femurYZ").c_str()), 0, drehungUm(x, -30.0F));
            rigSchluesselSetzen(s, knochen(g, (n + "tibia").c_str()), 0, drehungUm(x, 60.0F));
            rigSchluesselSetzen(s, knochen(g, (n + "talus").c_str()), 0, drehungUm(x, -30.0F));
        }
        PhysikUmgebung u;
        u.bodenZ = bodenVon(g, mm, s, 0);
        const RigSchicht ohne = s;
        s.physik.e.an = true;
        PhysikBericht b;
        (void)physikRechnen(g, r, s, 0, 25, u, &b);
        const auto rutschen = [&](const RigSchicht& t) {
            float weg = 0.0F;
            std::vector<BoneMatrix> w;
            rigWelt(g, t, 0, 0, 0.0F, w);
            const Vec3 p0 = punktWelt(g, mm.fuss[0][0], w);
            for (int f = 1; f < 25; ++f) {
                rigWelt(g, t, f, f, 0.0F, w);
                const Vec3 p = punktWelt(g, mm.fuss[0][0], w);
                weg = std::max(weg, std::hypot(p[0] - p0[0], p[1] - p0[1]));
            }
            return weg;
        };
        const float vorher = rutschen(ohne);
        const float nachher = rutschen(s);
        expect("Rot: ohne Physik rutscht der linke Fuss " + std::to_string(vorher) + " Einheiten", vorher > 5.0F);
        expect("mit Physik bleibt er stehen (" + std::to_string(nachher) + ", Kontakt in " + std::to_string(b.bilder[24].kontakt[0]) + ")", nachher < 0.02F && b.bilder[24].kontakt[0]);
    }

    // --- Federkette -----------------------------------------------------------------------------------------------------
    {
        const GlaAnimation g = skelett(80, [](int) { return std::array<float, 3>{0.0F, 0.0F, 0.0F}; });
        const Rig r = rigMitFuss(g);
        const std::vector<int> vorschlag = kettenVorschlag(r, g);
        const int ltail = knochen(g, "ltail");
        expect("Vorschlag Federketten: ltail und rtail (" + std::to_string(vorschlag.size()) + ")",
               std::find(vorschlag.begin(), vorschlag.end(), ltail) != vorschlag.end() && std::find(vorschlag.begin(), vorschlag.end(), knochen(g, "rtail")) != vorschlag.end());
        RigSchicht s;
        s.neuInterp = Interp::Linear;
        const float z[3] = {0.0F, 0.0F, 1.0F};
        const int femur = knochen(g, "lfemurYZ");
        rigSchluesselSetzen(s, femur, 5, BoneMatrix::identity());
        rigSchluesselSetzen(s, femur, 7, drehungUm(z, 40.0F));
        PhysikUmgebung u;
        const RigSchicht ohne = s;
        s.physik.e.an = true;
        s.physik.e.kontakte = false;   // nur die Feder (die Huefte dreht den Fuss, die Kontakte hielten ihn - der Oberschenkel wiche ab)
        s.physik.e.ketten.push_back(PhysikKette{ltail, 2.0F, 0.3F});
        PhysikBericht b;
        (void)physikRechnen(g, r, s, 0, 80, u, &b);
        std::vector<float> fehler;
        std::vector<BoneMatrix> w0;
        std::vector<BoneMatrix> w1;
        for (int f = 0; f < 80; ++f) {
            rigWelt(g, ohne, f, f, 0.0F, w0);
            rigWelt(g, s, f, f, 0.0F, w1);
            fehler.push_back(drehWinkel(w0[static_cast<std::size_t>(ltail)], w1[static_cast<std::size_t>(ltail)]));
        }
        const float frueh = *std::max_element(fehler.begin() + 6, fehler.begin() + 20);
        const float spaet = *std::max_element(fehler.begin() + 50, fehler.end());
        // Ueberschwingen: der Abstand faellt nach dem Ruck auf (nahe) 0 und steigt wieder (Nulldurchgang der Schwingung)
        int minima = 0;
        for (std::size_t i = 8; i + 1 < 50; ++i) { minima += fehler[i] < fehler[i - 1] && fehler[i] <= fehler[i + 1] ? 1 : 0; }
        char t[200];
        std::snprintf(t, sizeof t, "Federkette ltail: Nachlauf %.2f Grad nach dem Ruck, spaeter %.3f Grad, %d Umkehrpunkte, vor dem Ruck %.4f", frueh, spaet, minima,
                      fehler[4]);
        expect(t, frueh > 3.0F && spaet < frueh * 0.05F && minima >= 1 && fehler[4] < 1e-3F && b.kettenKnochen == 1);
    }

    std::printf("%s (%d Fehler)\n", fails == 0 ? "ALLE OK" : "FEHLER", fails);
    return fails == 0 ? 0 : 1;
}
