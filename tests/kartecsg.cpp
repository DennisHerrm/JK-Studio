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
    std::printf("kartecsg: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
