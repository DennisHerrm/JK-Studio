// kartegrundformen.cpp - Karten-Editor Stufe 2, Punkt 4: Pinsel-Grundformen
// wie NetRadiant-Custom 20260114 (radiant/brushmanip.cpp:76-460, 805-816).
//
// Sollwerte von Hand gerechnet:
//   - Prism/Cone: die Seitenebenen beruehren den Kreis mit dem Radius
//     max_extent_2d (Punkt bei Winkel i * 2pi / n, Richtung tangential):
//     ein umschriebenes n-Eck mit Inkreis r, Flaeche n r^2 tan(pi/n).
//   - Sphere: n (n - 1) + n Ebenen durch Punkte auf der Kugel (Radius
//     max_extent): alle Ecken liegen hoechstens r vom Mittelpunkt.
//   - Rock: jede Ebene beruehrt die Kugel (Punkt mid + n r, Normale n).
//   - Icosahedron: 20 Dreiecke, unterteilt 80, gestutzt 12 + 20 = 32 Flaechen.
#include "bhed/karte/grundformen.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texdef.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}
bool nah(double a, double b, double eps = 1e-2) { return std::fabs(a - b) <= eps; }

struct Aufbau {
    Karte k;
    std::shared_ptr<Pinsel> p;
    explicit Aufbau(const Vektor3f& mitte = Vektor3f(0, 0, 32), const Vektor3f& halb = Vektor3f(32, 32, 32)) {
        TexturProjektion proj;
        texdefStandard(TexdefArt::Quake, proj);
        p = std::make_shared<Pinsel>();
        p->quaderBauen(Kasten(mitte, halb), "textures/a/b", proj, k.kontext);
        k.weltHolenOderAnlegen()->kindEinfuegen(p);
        k.auswahl.waehlen(k.welt(), p);
        k.undo.clear();
    }
};

double volumen(const Pinsel& p, const Kontext& k) {
    p.bRep(k);
    double v = 0;
    for (const auto& f : p.flaechen()) {
        const Wicklung& w = f->wicklung();
        if (w.numpoints < 3) { continue; }
        Vektor3d a(0, 0, 0);
        for (std::size_t i = 1; i + 1 < w.numpoints; ++i) { a += kreuz(w[i].vertex - w[0].vertex, w[i + 1].vertex - w[0].vertex); }
        v += f->ebene().dist() * laenge(a) * 0.5 / 3.0;
    }
    return v;
}
std::size_t beitragend(const Pinsel& p, const Kontext& k) {
    p.bRep(k);
    std::size_t n = 0;
    for (const auto& f : p.flaechen()) { n += f->traegtBei() ? 1U : 0U; }
    return n;
}
// groesster und kleinster Abstand der Ecken von m
void eckenAbstand(const Pinsel& p, const Kontext& k, const Vektor3d& m, double& mn, double& mx) {
    p.bRep(k);
    mn = 1e9;
    mx = 0;
    for (const auto& f : p.flaechen()) {
        const Wicklung& w = f->wicklung();
        for (std::size_t i = 0; i < w.numpoints; ++i) {
            const double d = laenge(w[i].vertex - m);
            mn = std::min(mn, d);
            mx = std::max(mx, d);
        }
    }
}
bool hatEcke(const Pinsel& p, const Kontext& k, const Vektor3d& v) {
    p.bRep(k);
    for (const auto& f : p.flaechen()) {
        const Wicklung& w = f->wicklung();
        for (std::size_t i = 0; i < w.numpoints; ++i) {
            if (laengeQuadrat(w[i].vertex - v) < 1e-4) { return true; }
        }
    }
    return false;
}

void prisma() {
    Aufbau a;
    Protokoll pr;
    grundformAuswahl(a.k, Grundform::Prisma, 8, false, 2, "textures/a/prisma", pr);
    const Kontext& k = a.k.kontext;
    pruefe(a.p->anzahl() == 10 && beitragend(*a.p, k) == 10, "Prism 8: 10 Flaechen");
    pruefe(a.k.undo.undoName() == "brushPrism -sides 8 -axis 2", "Prism: Rueckgaengig-Name " + a.k.undo.undoName());
    const double soll = 8.0 * 32.0 * 32.0 * std::tan(kPi / 8.0) * 64.0;
    pruefe(nah(volumen(*a.p, k), soll, 0.5), "Prism 8: umschriebenes Achteck, Rauminhalt " + std::to_string(volumen(*a.p, k)));
    const Kasten g = a.p->grenzen(k);
    pruefe(nah(g.extents[0], 32) && nah(g.extents[1], 32) && nah(g.extents[2], 32) && nah(g.origin[2], 32), "Prism: Grenzen bleiben");
    pruefe(a.p->flaechen()[2]->shader() == "textures/a/prisma", "Prism: Shader des Browsers");
    a.k.undo.undo();
    pruefe(a.p->anzahl() == 6, "Prism: Rueckgaengig");
    // Achse 0 (YZ-Ansicht): Deckel mit Normale +-x, Radius max(ey, ez)
    Aufbau b(Vektor3f(0, 0, 0), Vektor3f(16, 32, 32));
    grundformAuswahl(b.k, Grundform::Prisma, 4, false, 0, "textures/a/prisma", pr);
    const Kontext& kb = b.k.kontext;
    pruefe(b.p->anzahl() == 6 && std::fabs(b.p->flaechen()[0]->ebene().normal()[0]) > 0.999 && b.k.undo.undoName() == "brushPrism -sides 4 -axis 0",
           "Prism Achse x: Deckel +-x");
    pruefe(nah(volumen(*b.p, kb), 32.0 * 64.0 * 64.0, 0.5), "Prism 4 Achse x: Quader 32 x 64 x 64");
}

void kegel() {
    Aufbau a;
    Protokoll pr;
    grundformAuswahl(a.k, Grundform::Kegel, 6, false, 2, "textures/a/kegel", pr);
    const Kontext& k = a.k.kontext;
    pruefe(a.p->anzahl() == 7 && beitragend(*a.p, k) == 7, "Cone 6: 7 Flaechen");
    pruefe(hatEcke(*a.p, k, Vektor3d(0, 0, 64)), "Cone: Spitze oben in der Mitte");
    const double soll = 6.0 * 32.0 * 32.0 * std::tan(kPi / 6.0) * 64.0 / 3.0;
    pruefe(nah(volumen(*a.p, k), soll, 0.5), "Cone 6: Rauminhalt " + std::to_string(volumen(*a.p, k)));
    pruefe(a.k.undo.undoName() == "brushCone -sides 6 -axis 2", "Cone: Rueckgaengig-Name");
}

void kugel() {
    Aufbau a;
    Protokoll pr;
    grundformAuswahl(a.k, Grundform::Kugel, 8, false, 2, "textures/a/kugel", pr);
    const Kontext& k = a.k.kontext;
    double mn = 0;
    double mx = 0;
    eckenAbstand(*a.p, k, Vektor3d(0, 0, 32), mn, mx);
    pruefe(a.p->anzahl() == 64, "Sphere 8: 8 * 7 + 8 = 64 Ebenen (" + std::to_string(a.p->anzahl()) + ")");
    pruefe(mx < 32.01 && mx > 31.99, "Sphere: Ecken auf der Kugel r 32 (max " + std::to_string(mx) + ")");
    pruefe(beitragend(*a.p, k) >= 48, "Sphere: Flaechen tragen bei (" + std::to_string(beitragend(*a.p, k)) + ")");
    pruefe(a.k.undo.undoName() == "brushSphere -sides 8", "Sphere: Rueckgaengig-Name");
    // zu viele Seiten: Fehler, nichts geaendert, kein Schritt
    Aufbau b;
    Protokoll pf;
    grundformAuswahl(b.k, Grundform::Kugel, 32, false, 2, "textures/a/kugel", pf);
    pruefe(b.p->anzahl() == 6 && pf.size() == 1 && pf[0].art == ProtokollTeil::Art::Fehler &&
               pf[0].text == "brushSphere: sides 32: too many sides, maximum is 31\n" && b.k.undo.undoAnzahl() == 0,
           "Sphere 32: Fehler, nichts geaendert");
    Protokoll pp;
    grundformAuswahl(b.k, Grundform::Prisma, 2, false, 2, "textures/a/kugel", pp);
    pruefe(!pp.empty() && pp[0].text == "brushPrism: sides 2: too few sides, minimum is 3\n", "Prism 2: zu wenige Seiten");
}

void fels() {
    Aufbau a;
    Protokoll pr;
    std::srand(1);
    grundformAuswahl(a.k, Grundform::Fels, 32, false, 2, "textures/a/fels", pr);
    bool tangential = a.p->anzahl() == 32;
    for (const auto& f : a.p->flaechen()) {
        const double d = abstandZuPunkt(f->ebene(), Vektor3d(0, 0, 32));
        tangential = tangential && std::fabs(std::fabs(d) - 32.0) < 0.05;
    }
    pruefe(tangential, "Rock 32: 32 Ebenen, jede beruehrt die Kugel r 32");
    pruefe(a.k.undo.undoName() == "brushRock -sides 32", "Rock: Rueckgaengig-Name");
}

void ikosaeder() {
    const auto zahl = [](std::size_t u, bool stutzen) {
        Aufbau a;
        Protokoll pr;
        grundformAuswahl(a.k, Grundform::Ikosaeder, u, stutzen, 2, "textures/a/iko", pr);
        return std::make_pair(a.p->anzahl(), beitragend(*a.p, a.k.kontext));
    };
    const auto z0 = zahl(0, false);
    const auto z1 = zahl(1, false);
    const auto zs = zahl(0, true);
    pruefe(z0.first == 20 && z0.second == 20, "Icosahedron 0: 20 Flaechen (" + std::to_string(z0.first) + ")");
    pruefe(z1.first == 80 && z1.second == 80, "Icosahedron 1: 80 Flaechen (" + std::to_string(z1.first) + ")");
    pruefe(zs.first == 32 && zs.second == 32, "Icosahedron gestutzt: 12 + 20 = 32 Flaechen (" + std::to_string(zs.first) + ")");
    Aufbau a;
    Protokoll pr;
    grundformAuswahl(a.k, Grundform::Ikosaeder, 1, false, 2, "textures/a/iko", pr);
    double mn = 0;
    double mx = 0;
    eckenAbstand(*a.p, a.k.kontext, Vektor3d(0, 0, 32), mn, mx);
    pruefe(nah(mx, 32, 0.02) && nah(mn, 32, 0.02), "Icosahedron: alle Ecken auf der Kugel");
    pruefe(a.k.undo.undoName() == "brushIcosahedron -subdivisions 1", "Icosahedron: Rueckgaengig-Name");
}

void bereiche() {
    const SeitenBereich p = seitenBereich(Grundform::Prisma);
    const SeitenBereich s = seitenBereich(Grundform::Kugel);
    const SeitenBereich r = seitenBereich(Grundform::Fels);
    const SeitenBereich i = seitenBereich(Grundform::Ikosaeder);
    pruefe(p.vorgabe == 8 && p.min == 3 && p.max == 1022 && s.max == 31 && r.vorgabe == 32 && r.min == 10 && r.max == 1000 && i.vorgabe == 1 &&
               i.max == 2,
           "DoSides: Vorgaben und Bereiche");
}

}  // namespace

int main() {
    prisma();
    kegel();
    kugel();
    fels();
    ikosaeder();
    bereiche();
    std::printf("kartegrundformen: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
