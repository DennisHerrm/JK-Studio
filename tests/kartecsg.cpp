// kartecsg.cpp - Karten-Editor Stufe 2, Punkte 2 und 3: Clipper (Clip, Split,
// Flip, 2 und 3 Punkte, Punkte auf der Szene, Wrap Merge ohne Schnitt,
// Einstellungen) und CSG wie NetRadiant-Custom 20260114.
//
// Sollwerte von Hand gerechnet (ganze Zahlen, Kommentare je Fall):
//   - Clipper: radiant/selection_mtor_clip.cpp (newPointIndex, newPoint,
//     updatePlane: in 2D der dritte Punkt p0 - viewdir * |p0 - p1|),
//     radiant/csg.cpp:736-806 (BrushSplitByPlaneSelected: addPlane( p0, p1, p2 )
//     behaelt die Rueckseite; Split: das Teil mit p0, p2, p1),
//     radiant/clippertool.cpp:121-152 (Clipper_do, Reset Points/Flip, Flip).
//   - Wrap Merge: csg.cpp:1115-1214 (Huelle aus Ecken + Clipper-Punkten).
#include "bhed/karte/auswahlbefehle.h"
#include "bhed/karte/bearbeitung.h"
#include "bhed/karte/csg.h"
#include "bhed/karte/kamera.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/komponenten.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texdef.h"
#include "bhed/karte/xyansicht.h"

#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}
bool nah(double a, double b, double eps = 1e-3) { return std::fabs(a - b) <= eps; }

std::shared_ptr<Pinsel> quader(Karte& k, const Vektor3f& mitte, const Vektor3f& halb, const char* shader = "textures/a/b") {
    TexturProjektion proj;
    texdefStandard(TexdefArt::Quake, proj);
    auto p = std::make_shared<Pinsel>();
    p->quaderBauen(Kasten(mitte, halb), shader, proj, k.kontext);
    k.weltHolenOderAnlegen()->kindEinfuegen(p);
    return p;
}
XyAnsicht ansicht2d(Sicht s) {
    XyAnsicht v;
    v.typ = s;
    v.breite = 800;
    v.hoehe = 600;
    v.massstab = 1;
    v.ursprung = Vektor3f(0, 0, 0);
    return v;
}
Blick blick2d(const XyAnsicht& v) {
    Blick b(false);
    b.bauen(v.projektion(), v.modelview(), 800, 600);
    return b;
}
Blick blick3d(const Vektor3f& o, const Vektor3f& w) {
    Kamera k;
    k.breite = 800;
    k.hoehe = 600;
    k.ursprung = o;
    k.winkel = w;
    k.projektionAktualisieren(KameraEinstellungen());
    k.modelviewAktualisieren();
    Blick b(true);
    b.bauen(k.projektion, k.modelview, 800, 600);
    return b;
}
GeraeteVektor geraetXy(const XyAnsicht& v, float x, float y) {
    const Vektor3f p = projiziert(transformierterVektor4(multipliziert(v.projektion(), v.modelview()), Vektor4f(x, y, 0, 1)));
    return GeraeteVektor(p[0], p[1]);
}
GeraeteVektor geraet3d(const Blick& b, const Vektor3f& w) {
    const Vektor3f p = projiziert(transformierterVektor4(b.sicht(), Vektor4f(w, 1)));
    return GeraeteVektor(p[0], p[1]);
}
// Manipulator_::getEpsilon im Clipper: eps / 12 * (12 + 4)
const GeraeteVektor kEpsClip(16.0F / 800.0F, 16.0F / 600.0F);

bool kastenNah(const Kasten& a, const Vektor3f& mn, const Vektor3f& mx) {
    return nah(a.origin[0] - a.extents[0], mn[0]) && nah(a.origin[1] - a.extents[1], mn[1]) && nah(a.origin[2] - a.extents[2], mn[2]) &&
           nah(a.origin[0] + a.extents[0], mx[0]) && nah(a.origin[1] + a.extents[1], mx[1]) && nah(a.origin[2] + a.extents[2], mx[2]);
}
std::string kastenText(const Kasten& a) {
    char t[160];
    std::snprintf(t, sizeof(t), "(%g %g %g)..(%g %g %g)", static_cast<double>(a.origin[0] - a.extents[0]), static_cast<double>(a.origin[1] - a.extents[1]),
                  static_cast<double>(a.origin[2] - a.extents[2]), static_cast<double>(a.origin[0] + a.extents[0]),
                  static_cast<double>(a.origin[1] + a.extents[1]), static_cast<double>(a.origin[2] + a.extents[2]));
    return t;
}
std::vector<std::shared_ptr<Pinsel>> pinselListe(const Karte& k) {
    std::vector<std::shared_ptr<Pinsel>> r;
    for (const auto& c : k.welt()->kinder()) { r.push_back(std::static_pointer_cast<Pinsel>(c)); }
    return r;
}
bool hatEcke(const Pinsel& p, const Karte& k, const Vektor3d& v) {
    for (const FlaechenPunkt& fp : p.ecken(k.kontext)) {
        if (laengeQuadrat(p.eckpunkt(fp) - v) < 1e-6) { return true; }
    }
    return false;
}

// Clipper in 2D: zwei Klicks bei (0,-64) und (0,64); die Tiefe aus den
// Grenzen der Auswahl (z = 32 fuer Punkt 1 und 2), der dritte Punkt
// p0 - viewdir * 128 = (0,-64,160). plane3_for_points (libs/math/plane.h:142-147):
// Normale = (p2 - p0) x (p1 - p0) = (0,0,128) x (0,128,0) = (-16384,0,0):
// Ebene x = 0 mit Normale -x, der Clip behaelt x 0..32.
struct Clipper2d {
    Karte k;
    std::shared_ptr<Pinsel> p;
    AuswahlBearbeitung be{k};
    XyAnsicht a = ansicht2d(Sicht::XY);
    Blick b = blick2d(a);
    explicit Clipper2d(bool zweiPunkte = true) {
        p = quader(k, Vektor3f(0, 0, 0), Vektor3f(32, 32, 32));
        k.auswahl.waehlen(k.welt(), p);
        be.clipper2Punkte2d = zweiPunkte;
        be.setzeModus(ManipModus::Clipper);
        be.pivotGeaendert();
        (void)be.grenzen();
    }
    void klick(float x, float y) {
        (void)be.manipulatorWaehlen(b, geraetXy(a, x, y), kEpsClip);
        be.bewegenBeenden();
    }
    void ausfuehren(bool teilen, const char* name) {
        Protokoll pr;
        CsgUmgebung u;
        u.browserShader = "textures/a/schnitt";
        Befehl befehl(k.undo, name);
        be.clipperAusfuehren(teilen, u, pr);
    }
};

void clipper2d() {
    {
        Clipper2d c;
        c.klick(0, -64);
        pruefe(c.be.clipperPunkte.anzahl == 1 && c.be.clipperPunkte.punkte[0] == Vektor3d(0, -64, 32), "Punkt 1 bei (0,-64,32) (Tiefe aus den Grenzen)");
        c.klick(0, 64);
        pruefe(c.be.clipperPunkte.anzahl == 2 && c.be.clipperPunkte.punkte[1] == Vektor3d(0, 64, 32) && c.be.clipperPunkte.punkte[2] == Vektor3d(0, -64, 160),
               "Punkt 2 bei (0,64,32), Punkt 3 = p0 - viewdir * 128 = (0,-64,160)");
        pruefe(c.be.clipperEbeneOk(), "Clipper_ok_plane");
        c.ausfuehren(false, "clipperClip");
        pruefe(kastenNah(c.p->grenzen(c.k.kontext), Vektor3f(0, -32, -32), Vektor3f(32, 32, 32)), "Clip: die Rueckseite bleibt (x 0..32) " + kastenText(c.p->grenzen(c.k.kontext)));
        bool caulk = false;
        for (const auto& f : c.p->flaechen()) { caulk = caulk || (nah(f->ebene().normal()[0], -1, 1e-6) && f->shader() == "textures/common/caulk"); }
        pruefe(caulk, "Caulk Clipper Cuts: die neue Flaeche hat textures/common/caulk");
        pruefe(c.be.clipperPunkte.anzahl == 0, "Reset Points on Split: die Punkte sind weg");
        pruefe(c.k.undo.undoName() == "clipperClip", "Name clipperClip");
        c.k.undo.undo();
        pruefe(kastenNah(c.p->grenzen(c.k.kontext), Vektor3f(-32, -32, -32), Vektor3f(32, 32, 32)), "Rueckgaengig: Quader");
    }
    {   // Split: beide Haelften, beide gewaehlt
        Clipper2d c;
        c.be.clipperCaulk = false;
        c.klick(0, -64);
        c.klick(0, 64);
        c.ausfuehren(true, "clipperSplit");
        const auto liste = pinselListe(c.k);
        pruefe(liste.size() == 2 && c.k.auswahl.anzahl() == 2, "Split: zwei Pinsel, beide gewaehlt");
        if (liste.size() == 2) {
            pruefe(kastenNah(liste[0]->grenzen(c.k.kontext), Vektor3f(0, -32, -32), Vektor3f(32, 32, 32)) &&
                       kastenNah(liste[1]->grenzen(c.k.kontext), Vektor3f(-32, -32, -32), Vektor3f(0, 32, 32)),
                   "Split: der alte Pinsel x 0..32, das neue Teil (p0, p2, p1) x -32..0");
            bool browser = false;
            for (const auto& f : liste[1]->flaechen()) { browser = browser || (nah(f->ebene().normal()[0], 1, 1e-6) && f->shader() == "textures/a/schnitt"); }
            pruefe(browser, "ohne Caulk: die Schnittflaeche hat den Shader des Texturbrowsers");
        }
    }
    {   // Flip: die andere Seite
        Clipper2d c;
        c.klick(0, -64);
        c.klick(0, 64);
        c.be.clipperUmdrehen();
        pruefe(c.be.clipperUmgedreht, "Flip: umgedreht");
        c.ausfuehren(false, "clipperClip");
        pruefe(kastenNah(c.p->grenzen(c.k.kontext), Vektor3f(-32, -32, -32), Vektor3f(0, 32, 32)), "Flip + Clip: x -32..0 bleibt");
        pruefe(!c.be.clipperUmgedreht, "Reset Flipped State");
    }
    {   // Reset Points on Split aus: die Punkte bleiben, ein zweiter Clip schneidet wieder
        Clipper2d c;
        c.be.clipperPunkteZuruecksetzen = false;
        c.klick(0, -64);
        c.klick(0, 64);
        c.ausfuehren(false, "clipperClip");
        pruefe(c.be.clipperPunkte.anzahl == 2, "ohne Reset Points: zwei Punkte bleiben");
    }
    {   // 2 Points in 2D Views aus: der dritte Klick setzt Punkt 3 auf die Unterseite
        Clipper2d c(false);
        c.klick(-64, -64);
        c.klick(64, -64);
        c.klick(64, 64);
        pruefe(c.be.clipperPunkte.anzahl == 3 && c.be.clipperPunkte.punkte[2] == Vektor3d(64, 64, -32), "drei Punkte in 2D: Punkt 3 bei z = -32");
        // Ebene durch (-64,-64,32), (64,-64,32), (64,64,-32): (p2-p0) x (p1-p0) = (0,-8192,-16384),
        // Abstand 0: es bleibt y + 2z >= 0; die Kanten x=32,y=32 und x=32,y=-32 werden bei z=-16 bzw. 16 geschnitten
        c.ausfuehren(false, "clipperClip");
        pruefe(!hatEcke(*c.p, c.k, Vektor3d(32, 32, -32)) && !hatEcke(*c.p, c.k, Vektor3d(32, -32, -32)) && hatEcke(*c.p, c.k, Vektor3d(32, 32, 32)) &&
                   hatEcke(*c.p, c.k, Vektor3d(32, 32, -16)) && hatEcke(*c.p, c.k, Vektor3d(32, -32, 16)),
               "drei Punkte: schraeger Schnitt y + 2z >= 0 (neue Ecken (32,32,-16), (32,-32,16))");
    }
    {   // ganz vor der Ebene: der Clip loescht den Pinsel (csg.cpp:782-787)
        Clipper2d c;
        c.klick(64, -32);
        c.klick(64, 32);
        // Punkte (64,-32,32), (64,32,32), (64,-32,96): Normale (p2-p0) x (p1-p0) = (-4096,0,0), der Quader liegt davor
        c.ausfuehren(false, "clipperClip");
        pruefe(c.k.welt()->kinder().empty() && c.k.auswahl.anzahl() == 0, "Clip: ein Pinsel ganz vor der Ebene wird geloescht");
    }
    {   // keine Schnittmenge: CSG_WrapMerge mit den Clipper-Punkten
        Clipper2d c;
        c.klick(-64, -32);
        c.klick(-64, 32);
        // Punkte (-64,-32,32), (-64,32,32), (-64,-32,96): Normale -x, Abstand 64, der Quader liegt dahinter
        c.ausfuehren(false, "clipperClip");
        const auto liste = pinselListe(c.k);
        pruefe(liste.size() == 1 && liste[0] == c.p, "Wrap Merge: derselbe Pinsel (der zuletzt gewaehlte wird neu gebaut)");
        // CSG_WrapMerge nimmt nur die gesetzten Punkte (_count = 2), nicht den errechneten dritten
        pruefe(hatEcke(*c.p, c.k, Vektor3d(-64, -32, 32)) && hatEcke(*c.p, c.k, Vektor3d(-64, 32, 32)) && hatEcke(*c.p, c.k, Vektor3d(32, -32, -32)) &&
                   !hatEcke(*c.p, c.k, Vektor3d(-64, -32, 96)) && c.p->ecken(c.k.kontext).size() == 8,
               "Wrap Merge: Huelle aus Quader und den zwei Clipper-Punkten (8 Ecken)");
    }
}

// Clipper in 3D: drei Punkte auf der Szene (testselect_scene_point,
// testSelected_scene_snapped_point): (8,-16,32), (8,16,32) auf der Deckflaeche,
// (8,-32,0) auf der Vorderseite: Normale (p2-p0) x (p1-p0) = (1024,0,0) -> +x,
// der Clip behaelt x -32..8.
void clipper3d() {
    Karte k;
    auto p = quader(k, Vektor3f(0, 0, 0), Vektor3f(32, 32, 32));
    k.auswahl.waehlen(k.welt(), p);
    AuswahlBearbeitung be(k);
    be.setzeModus(ManipModus::Clipper);
    const Blick b = blick3d(Vektor3f(0, -200, 200), Vektor3f(-40, 90, 0));
    for (const Vektor3f& w : {Vektor3f(8, -16, 32), Vektor3f(8, 16, 32), Vektor3f(8, -32, 0)}) {
        (void)be.manipulatorWaehlen(b, geraet3d(b, w), kEpsClip);
        be.bewegenBeenden();
    }
    pruefe(be.clipperPunkte.anzahl == 3 && be.clipperPunkte.punkte[0] == Vektor3d(8, -16, 32) && be.clipperPunkte.punkte[1] == Vektor3d(8, 16, 32) &&
               be.clipperPunkte.punkte[2] == Vektor3d(8, -32, 0),
           "3D: drei Punkte auf der Szene, aufs Raster gesetzt");
    {
        Protokoll pr;
        Befehl befehl(k.undo, "clipperClip");
        be.clipperAusfuehren(false, CsgUmgebung(), pr);
    }
    pruefe(kastenNah(p->grenzen(k.kontext), Vektor3f(-32, -32, -32), Vektor3f(8, 32, 32)), "3D-Clip: x -32..8 " + kastenText(p->grenzen(k.kontext)));
    // ein vierter Klick beginnt von vorn (newPointIndex: 3 % 3 = 0)
    be.setzeModus(ManipModus::Verschieben);
    be.setzeModus(ManipModus::Clipper);
    pruefe(be.clipperPunkte.anzahl == 0, "Wechsel in den Clipper: Punkte leer (reset)");
}

// --- Punkt 3: CSG --------------------------------------------------------------

// Rauminhalt aus den Wicklungen: Summe Abstand * Flaeche / 3 (Ebenen nach aussen)
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
double volumenAlle(const Karte& k) {
    double v = 0;
    for (const auto& p : pinselListe(k)) { v += volumen(*p, k.kontext); }
    return v;
}
std::size_t beitragend(const Pinsel& p, const Kontext& k) {
    p.bRep(k);
    std::size_t n = 0;
    for (const auto& f : p.flaechen()) { n += f->traegtBei() ? 1U : 0U; }
    return n;
}
std::size_t mitShader(const Pinsel& p, const std::string& s) {
    std::size_t n = 0;
    for (const auto& f : p.flaechen()) { n += f->shader() == s ? 1U : 0U; }
    return n;
}
std::string letzteZeile(const Protokoll& pr) { return pr.empty() ? std::string() : pr.back().text; }
bool kastenIn(const Kasten& a, const Vektor3f& mn, const Vektor3f& mx) {
    for (std::size_t i = 0; i < 3; ++i) {
        if (a.origin[i] - a.extents[i] < mn[i] - 1e-3F || a.origin[i] + a.extents[i] > mx[i] + 1e-3F) { return false; }
    }
    return true;
}

// SubtractBrushesFromUnselected (csg.cpp:574-708): Ziel 0..64, Schneider
// 16..48 x 16..48 x 32..96. Die Flaechen des Schneiders nach der Achse
// geordnet (Z > Y > X, die Gegenflaeche gleich dahinter); die +z-Flaeche
// (z = 96) trifft nicht, die -z-Flaeche schneidet die Platte z 0..32 ab,
// dann y (zwei Platten ueber die ganze Breite), dann x (zwei Stuecke):
// 5 Teile, 64^3 - 32^3 = 229376.
void subtrahieren() {
    Karte k;
    auto ziel = quader(k, Vektor3f(32, 32, 32), Vektor3f(32, 32, 32));
    auto schneider = quader(k, Vektor3f(32, 32, 64), Vektor3f(16, 16, 32));
    k.undo.clear();
    {
        Protokoll pr;
        csgSubtrahieren(k, pr);
        pruefe(pr.size() == 1 && pr[0].art == ProtokollTeil::Art::Warnung && pr[0].text == "CSG Subtract: No brushes selected.\n",
               "Subtract ohne Auswahl: Warnung");
        pruefe(k.undo.undoAnzahl() == 0, "Subtract ohne Auswahl: kein Schritt");
    }
    k.auswahl.waehlen(k.welt(), schneider);
    Protokoll pr;
    csgSubtrahieren(k, pr);
    const auto l = pinselListe(k);
    pruefe(l.size() == 6 && l[0] == schneider, "Subtract: Schneider + 5 Teile (" + std::to_string(l.size()) + ")");
    pruefe(pr.size() == 2 && pr[0].text == "CSG Subtract: Subtracting 1 brushes.\n" && pr[1].text == "CSG Subtract: Result: 5 fragments from 1 brush.\n",
           "Subtract: Ausgabe '" + letzteZeile(pr) + "'");
    pruefe(k.undo.undoName() == "brushSubtract", "Subtract: Rueckgaengig-Name " + k.undo.undoName());
    if (l.size() == 6) {
        pruefe(kastenNah(l[1]->grenzen(k.kontext), Vektor3f(0, 0, 0), Vektor3f(64, 64, 32)), "Subtract: zuerst die z-Platte " + kastenText(l[1]->grenzen(k.kontext)));
        const auto platteY = [&](const Kasten& b) {
            return kastenNah(b, Vektor3f(0, 48, 32), Vektor3f(64, 64, 64)) || kastenNah(b, Vektor3f(0, 0, 32), Vektor3f(64, 16, 64));
        };
        const auto stueckX = [&](const Kasten& b) {
            return kastenNah(b, Vektor3f(48, 16, 32), Vektor3f(64, 48, 64)) || kastenNah(b, Vektor3f(0, 16, 32), Vektor3f(16, 48, 64));
        };
        pruefe(platteY(l[2]->grenzen(k.kontext)) && platteY(l[3]->grenzen(k.kontext)), "Subtract: dann die y-Platten " + kastenText(l[2]->grenzen(k.kontext)));
        pruefe(stueckX(l[4]->grenzen(k.kontext)) && stueckX(l[5]->grenzen(k.kontext)), "Subtract: dann die x-Stuecke " + kastenText(l[4]->grenzen(k.kontext)));
        double v = 0;
        for (std::size_t i = 1; i < l.size(); ++i) { v += volumen(*l[i], k.kontext); }
        pruefe(nah(v, 229376.0, 0.5), "Subtract: Rauminhalt " + std::to_string(v));
    }
    k.undo.undo();
    const auto z = pinselListe(k);
    pruefe(z.size() == 2 && z[0] == ziel && kastenNah(ziel->grenzen(k.kontext), Vektor3f(0, 0, 0), Vektor3f(64, 64, 64)), "Subtract: Rueckgaengig");
}

// CSG_Merge / Brush_merge (csg.cpp:840-981): 0..32 und 32..64 in x.
void vereinigen() {
    {
        Karte k;
        auto a = quader(k, Vektor3f(16, 16, 16), Vektor3f(16, 16, 16));
        auto b = quader(k, Vektor3f(48, 16, 16), Vektor3f(16, 16, 16));
        k.auswahl.waehlen(k.welt(), a);
        {
            Protokoll pr;
            csgVereinigen(k, pr);
            pruefe(letzteZeile(pr) == "CSG Merge: At least two brushes have to be selected.\n", "Merge mit einem Pinsel: Warnung");
        }
        k.auswahl.waehlen(k.welt(), b);
        Protokoll pr;
        csgVereinigen(k, pr);
        const auto l = pinselListe(k);
        pruefe(l.size() == 1 && kastenNah(l[0]->grenzen(k.kontext), Vektor3f(0, 0, 0), Vektor3f(64, 32, 32)) && l[0]->anzahl() == 6,
               "Merge: ein Pinsel 0..64 mit 6 Flaechen");
        pruefe(l.size() == 1 && k.auswahl.anzahl() == 1 && k.auswahl.gewaehlt(k.welt().get(), l[0].get()), "Merge: der neue Pinsel ist gewaehlt");
        pruefe(letzteZeile(pr) == "CSG Merge: Succeeded.\n" && k.undo.undoName() == "brushMerge", "Merge: Ausgabe und Rueckgaengig-Name");
        k.undo.undo();
        pruefe(pinselListe(k).size() == 2, "Merge: Rueckgaengig");
    }
    {   // versetzt: b.+x (x = 48) liegt vor a.+x (x = 32) - Winding_PlanesConcave
        Karte k;
        auto a = quader(k, Vektor3f(16, 16, 16), Vektor3f(16, 16, 16));
        auto b = quader(k, Vektor3f(32, 48, 16), Vektor3f(16, 16, 16));
        k.auswahl.waehlen(k.welt(), a);
        k.auswahl.waehlen(k.welt(), b);
        k.undo.clear();
        Protokoll pr;
        csgVereinigen(k, pr);
        pruefe(pinselListe(k).size() == 2 && letzteZeile(pr) == "CSG Merge: Failed - result would not be convex.\n",
               "Merge versetzt: abgelehnt");
    }
    {   // L aus drei Wuerfeln: wie NRC fallen gegenueberliegende Ebenen aller
        // Pinsel weg (b.+y gegen c.-y, c.+x gegen b.-x), also das volle Quadrat
        Karte k;
        auto a = quader(k, Vektor3f(16, 16, 16), Vektor3f(16, 16, 16));
        auto b = quader(k, Vektor3f(48, 16, 16), Vektor3f(16, 16, 16));
        auto c = quader(k, Vektor3f(16, 48, 16), Vektor3f(16, 16, 16));
        for (const auto& p : {a, b, c}) { k.auswahl.waehlen(k.welt(), p); }
        Protokoll pr;
        csgVereinigen(k, pr);
        const auto l = pinselListe(k);
        pruefe(l.size() == 1 && kastenNah(l[0]->grenzen(k.kontext), Vektor3f(0, 0, 0), Vektor3f(64, 64, 32)), "Merge L aus drei: das Quadrat wie NRC");
    }
}

// CSG_WrapMerge (csg.cpp:1115-1214): Huelle zweier Wuerfel 0..16.
void huelle() {
    CsgUmgebung u;
    u.browserShader = "textures/a/huelle";
    {   // in einer Reihe: die 6 Ebenen kommen aus den Flaechen, b (zuletzt gewaehlt) wird neu gebaut
        Karte k;
        auto a = quader(k, Vektor3f(8, 8, 8), Vektor3f(8, 8, 8));
        auto b = quader(k, Vektor3f(56, 8, 8), Vektor3f(8, 8, 8));
        k.auswahl.waehlen(k.welt(), a);
        k.auswahl.waehlen(k.welt(), b);
        Protokoll pr;
        csgHuellenVereinigen(k, u, ClipperPunkte{}, pr);
        const auto l = pinselListe(k);
        pruefe(l.size() == 1 && l[0] == b && kastenNah(b->grenzen(k.kontext), Vektor3f(0, 0, 0), Vektor3f(64, 16, 16)) && beitragend(*b, k.kontext) == 6 &&
                   mitShader(*b, "textures/a/b") == 6,
               "Wrap Merge: b wird 0..64 mit 6 Flaechen aus den alten");
        pruefe(k.undo.undoName() == "brushWrapMerge", "Wrap Merge: Rueckgaengig-Name");
    }
    {   // ueber Eck: zwei schraege Seiten aus der Huelle (Shader des Browsers)
        Karte k;
        auto a = quader(k, Vektor3f(8, 8, 8), Vektor3f(8, 8, 8));
        auto b = quader(k, Vektor3f(56, 56, 8), Vektor3f(8, 8, 8));
        k.auswahl.waehlen(k.welt(), a);
        k.auswahl.waehlen(k.welt(), b);
        Protokoll pr;
        csgHuellenVereinigen(k, u, ClipperPunkte{}, pr);
        const auto l = pinselListe(k);
        pruefe(l.size() == 1 && beitragend(*l[0], k.kontext) == 8 && mitShader(*l[0], "textures/a/huelle") == 2,
               "Wrap Merge ueber Eck: 8 Flaechen, 2 neue");
        // Sechseck 64x64 ohne zwei Dreiecke (48*48/2) mal 16
        pruefe(l.size() == 1 && nah(volumen(*l[0], k.kontext), (64.0 * 64.0 - 48.0 * 48.0) * 16.0, 0.5), "Wrap Merge ueber Eck: Rauminhalt");
    }
}

// CSG_Intersect (csg.cpp:1216-1259): der zuletzt gewaehlte bleibt.
void schneiden() {
    {
        Karte k;
        auto a = quader(k, Vektor3f(16, 16, 16), Vektor3f(16, 16, 16));
        auto b = quader(k, Vektor3f(32, 32, 32), Vektor3f(16, 16, 16));
        k.auswahl.waehlen(k.welt(), a);
        k.auswahl.waehlen(k.welt(), b);
        Protokoll pr;
        csgSchneiden(k, pr);
        const auto l = pinselListe(k);
        pruefe(l.size() == 1 && l[0] == b && kastenNah(b->grenzen(k.kontext), Vektor3f(16, 16, 16), Vektor3f(32, 32, 32)), "Intersect: b wird 16..32");
        pruefe(k.undo.undoName() == "brushIntersect", "Intersect: Rueckgaengig-Name");
    }
    {   // getrennt: kein Rest, alle gewaehlten weg
        Karte k;
        auto a = quader(k, Vektor3f(8, 8, 8), Vektor3f(8, 8, 8));
        auto b = quader(k, Vektor3f(40, 40, 40), Vektor3f(8, 8, 8));
        auto c = quader(k, Vektor3f(200, 0, 0), Vektor3f(8, 8, 8));
        k.auswahl.waehlen(k.welt(), a);
        k.auswahl.waehlen(k.welt(), b);
        Protokoll pr;
        csgSchneiden(k, pr);
        const auto l = pinselListe(k);
        pruefe(l.size() == 1 && l[0] == c && letzteZeile(pr) == "CSG Intersect: Failed - result would not be convex.\n", "Intersect getrennt: beide weg");
    }
}

std::shared_ptr<Pinsel> raumWuerfel(Karte& k) {
    auto p = quader(k, Vektor3f(32, 32, 32), Vektor3f(32, 32, 32));
    k.auswahl.waehlen(k.welt(), p);
    return p;
}

// CSG_MakeRoom (csg.cpp:1401-1414): ePull mit dem Raster, Caulk, Innen weg.
void raum() {
    Karte k;
    (void)raumWuerfel(k);
    CsgUmgebung u;
    u.raster = 8;
    u.caulkShader = "textures/common/caulk";
    csgRaumBauen(k, u);
    const auto l = pinselListe(k);
    pruefe(l.size() == 6, "Make Room: 6 Waende (" + std::to_string(l.size()) + ")");
    pruefe(k.undo.undoName() == "makeRoom", "Make Room: Rueckgaengig-Name");
    int platten = 0;
    for (const auto& p : l) {
        const Kasten b = p->grenzen(k.kontext);
        const bool passt = kastenNah(b, Vektor3f(64, 0, 0), Vektor3f(72, 64, 64)) || kastenNah(b, Vektor3f(-8, 0, 0), Vektor3f(0, 64, 64)) ||
                           kastenNah(b, Vektor3f(0, 64, 0), Vektor3f(64, 72, 64)) || kastenNah(b, Vektor3f(0, -8, 0), Vektor3f(64, 0, 64)) ||
                           kastenNah(b, Vektor3f(0, 0, 64), Vektor3f(64, 64, 72)) || kastenNah(b, Vektor3f(0, 0, -8), Vektor3f(64, 64, 0));
        platten += passt ? 1 : 0;
        pruefe(p->anzahl() == 6 && mitShader(*p, "textures/a/b") == 1 && mitShader(*p, "textures/common/caulk") == 5,
               "Make Room: innen die alte Textur, sonst Caulk " + kastenText(b));
    }
    pruefe(platten == 6, "Make Room: Waende 8 dick aussen an jeder Seite");
    k.undo.undo();
    pruefe(pinselListe(k).size() == 1, "Make Room: Rueckgaengig");
}

// CSG_Hollow (csg.cpp:1438-1447) mit den Einstellungen des CSG Tools.
void aushoehlen() {
    CsgUmgebung u;
    u.caulkShader = "textures/common/caulk";
    u.browserShader = "textures/a/browser";
    HohlEinstellung e;   // Vorgaben des Dialogs: 16, -faces, Caulk, Innen weg
    {
        Karte k;
        (void)raumWuerfel(k);
        e.art = HohlArt::Ziehen;
        csgAushoehlen(k, u, e, "brushHollow::Pull");
        const auto l = pinselListe(k);
        pruefe(l.size() == 6 && nah(volumenAlle(k), 6.0 * 64 * 64 * 16, 0.5), "Hollow Pull: 6 Platten aussen");
        pruefe(k.undo.undoName() == "brushHollow::Pull", "Hollow Pull: Rueckgaengig-Name");
    }
    {   // Wrap: innen, die Flaechen nacheinander (das Original waechst nach innen): 64^3 - 32^3
        Karte k;
        (void)raumWuerfel(k);
        e.art = HohlArt::Huelle;
        csgAushoehlen(k, u, e, "brushHollow::Wrap");
        const auto l = pinselListe(k);
        bool innen = true;
        for (const auto& p : l) { innen = innen && kastenIn(p->grenzen(k.kontext), Vektor3f(0, 0, 0), Vektor3f(64, 64, 64)); }
        pruefe(l.size() == 6 && innen && nah(volumenAlle(k), 229376.0, 0.5), "Hollow Wrap: 6 Waende innen, " + std::to_string(volumenAlle(k)));
    }
    {   // Extrude: je Flaeche eine Platte aussen, Seiten mit dem Shader des Browsers
        Karte k;
        (void)raumWuerfel(k);
        e.art = HohlArt::Extrudieren;
        csgAushoehlen(k, u, e, "brushHollow::Extrude");
        const auto l = pinselListe(k);
        pruefe(l.size() == 6 && nah(volumenAlle(k), 6.0 * 64 * 64 * 16, 0.5), "Hollow Extrude: 6 Platten");
        pruefe(!l.empty() && mitShader(*l[0], "textures/a/browser") == 4 && mitShader(*l[0], "textures/a/b") == 2, "Hollow Extrude: 4 Seiten neu");
    }
    {   // Diag: Gehrungen, zusammen 96^3 - 64^3
        Karte k;
        (void)raumWuerfel(k);
        e.art = HohlArt::Diagonal;
        csgAushoehlen(k, u, e, "brushHollow::Diag");
        const auto l = pinselListe(k);
        bool drin = true;
        for (const auto& p : l) { drin = drin && kastenIn(p->grenzen(k.kontext), Vektor3f(-16, -16, -16), Vektor3f(80, 80, 80)); }
        pruefe(l.size() == 6 && drin && nah(volumenAlle(k), 622592.0, 0.5), "Hollow Diag: 6 Pyramidenstuempfe, " + std::to_string(volumenAlle(k)));
    }
    {   // Innen behalten: das Original bleibt, seine Flaechen werden Caulk (ePull)
        Karte k;
        auto p = raumWuerfel(k);
        HohlEinstellung b = e;
        b.art = HohlArt::Ziehen;
        b.innenEntfernen = false;
        csgAushoehlen(k, u, b, "brushHollow::Pull");
        pruefe(pinselListe(k).size() == 7 && mitShader(*p, "textures/common/caulk") == 6, "Hollow ohne Innen weg: 7 Pinsel, innen Caulk");
    }
    // Ausschluss (HollowSettings::excludeFaces): die obere Flaeche gewaehlt
    const auto oben = [](Karte& k, const std::shared_ptr<Pinsel>& p) {
        for (const auto& f : p->flaechen()) {
            if (f->ebene().normal()[2] > 0.5) { k.auswahl.flaecheWaehlen(p, f); }
        }
    };
    {
        Karte k;
        auto p = raumWuerfel(k);
        oben(k, p);
        HohlEinstellung b = e;
        b.art = HohlArt::Ziehen;
        csgAushoehlen(k, u, b, "brushHollow::Pull");
        bool keineDecke = true;
        for (const auto& q : pinselListe(k)) { keineDecke = keineDecke && q->grenzen(k.kontext).origin[2] + q->grenzen(k.kontext).extents[2] < 64.5F; }
        pruefe(pinselListe(k).size() == 5 && keineDecke, "-faces: ohne die gewaehlte Decke 5 Platten");
    }
    {
        Karte k;
        auto p = raumWuerfel(k);
        oben(k, p);
        HohlEinstellung b = e;
        b.art = HohlArt::Ziehen;
        b.gewaehlteAusschliessen = false;
        csgAushoehlen(k, u, b, "brushHollow::Pull");
        const auto l = pinselListe(k);
        pruefe(l.size() == 1 && kastenNah(l[0]->grenzen(k.kontext), Vektor3f(0, 0, 64), Vektor3f(64, 64, 80)), "+faces: nur die Decke");
    }
    {
        Karte k;
        (void)raumWuerfel(k);
        HohlEinstellung b = e;
        b.art = HohlArt::Ziehen;
        b.nachAchse = true;
        b.achse = Vektor3f(0, 0, 1);
        csgAushoehlen(k, u, b, "brushHollow::Pull");
        bool seiten = true;
        for (const auto& q : pinselListe(k)) { seiten = seiten && nah(q->grenzen(k.kontext).extents[2], 32); }
        pruefe(pinselListe(k).size() == 4 && seiten, "-proj (z): nur die 4 Seiten");
    }
}

// CSGdlg_BrushShrink / CSGdlg_BrushExpand (csg.cpp:1451-1467)
void versetzen() {
    Karte k;
    auto p = raumWuerfel(k);
    HohlEinstellung e;
    csgPinselVersetzen(k, e, false);
    pruefe(kastenNah(p->grenzen(k.kontext), Vektor3f(-16, -16, -16), Vektor3f(80, 80, 80)) && k.undo.undoName() == "Expand brush", "Expand: -16..80");
    e.versatz = 8;
    csgPinselVersetzen(k, e, true);
    pruefe(kastenNah(p->grenzen(k.kontext), Vektor3f(-8, -8, -8), Vektor3f(72, 72, 72)) && k.undo.undoName() == "Shrink brush", "Shrink: -8..72");
}

// Select_MakeDetail / Select_MakeStructural (brushmanip.cpp:1402-1410)
void detail() {
    Karte k;
    auto p = raumWuerfel(k);
    auto q = quader(k, Vektor3f(200, 0, 0), Vektor3f(8, 8, 8));
    pinselDetailSetzen(k, true);
    bool alle = true;
    for (const auto& f : p->flaechen()) { alle = alle && f->istDetail(); }
    pruefe(alle && p->istDetail() && !q->istDetail() && k.undo.undoName() == "brushSetDetail", "Make detail: alle Flaechen des gewaehlten");
    pinselDetailSetzen(k, false);
    pruefe(!p->istDetail() && k.undo.undoName() == "brushClearDetail", "Make structural");
    k.undo.undo();
    pruefe(p->istDetail(), "Make structural rueckgaengig");
}

}  // namespace

int main() {
    const auto schritt = [](const char* n) {
        std::printf("-- %s\n", n);
        std::fflush(stdout);
    };
    schritt("clipper2d");
    clipper2d();
    schritt("clipper3d");
    clipper3d();
    schritt("subtrahieren");
    subtrahieren();
    schritt("vereinigen");
    vereinigen();
    schritt("huelle");
    huelle();
    schritt("schneiden");
    schneiden();
    schritt("raum");
    raum();
    schritt("aushoehlen");
    aushoehlen();
    schritt("versetzen");
    versetzen();
    schritt("detail");
    detail();
    std::printf("kartecsg: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
