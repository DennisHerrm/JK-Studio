// kartetransform.cpp - Karten-Editor Stufe 2, Punkt 5: Drehen, Spiegeln,
// Arbitrary rotation/scale und Repeat/Reset Transforms wie
// NetRadiant-Custom 20260114 (radiant/select.cpp:532-660, 1017-1160,
// 1580-1760; radiant/selection.cpp:918-946, 1583-1734).
//
// Aufbau: Pinsel A 0..16 und B 48..64 x 0..16 x 0..16, beide gewaehlt; die
// Auswahlgrenzen haben die Mitte (32 8 8), der Pivot liegt dort (auf dem
// Fangraster 8). Sollwerte von Hand: Drehung um z mit -90 Grad
// (x, y) -> (y, -x), mit +90 (x, y) -> (-y, x), um den Pivot.
#include "bhed/karte/bearbeitung.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texdef.h"

#include <cmath>
#include <cstdio>
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
bool nah(double a, double b, double eps = 1e-3) { return std::fabs(a - b) <= eps; }

bool kastenNah(const Kasten& a, const Vektor3f& mn, const Vektor3f& mx) {
    return nah(a.origin[0] - a.extents[0], mn[0]) && nah(a.origin[1] - a.extents[1], mn[1]) && nah(a.origin[2] - a.extents[2], mn[2]) &&
           nah(a.origin[0] + a.extents[0], mx[0]) && nah(a.origin[1] + a.extents[1], mx[1]) && nah(a.origin[2] + a.extents[2], mx[2]);
}
std::string text(const Kasten& a) {
    char t[160];
    std::snprintf(t, sizeof(t), "(%g %g %g)..(%g %g %g)", static_cast<double>(a.origin[0] - a.extents[0]), static_cast<double>(a.origin[1] - a.extents[1]),
                  static_cast<double>(a.origin[2] - a.extents[2]), static_cast<double>(a.origin[0] + a.extents[0]),
                  static_cast<double>(a.origin[1] + a.extents[1]), static_cast<double>(a.origin[2] + a.extents[2]));
    return t;
}

struct Aufbau {
    Karte k;
    AuswahlBearbeitung be{k};
    std::shared_ptr<Pinsel> a;
    std::shared_ptr<Pinsel> b;
    Aufbau() {
        a = quader(Vektor3f(8, 8, 8));
        b = quader(Vektor3f(56, 8, 8));
        k.auswahl.waehlen(k.welt(), a);
        k.auswahl.waehlen(k.welt(), b);
        be.pivotGeaendert();
        k.undo.clear();
    }
    std::shared_ptr<Pinsel> quader(const Vektor3f& mitte) {
        TexturProjektion proj;
        texdefStandard(TexdefArt::Quake, proj);
        auto p = std::make_shared<Pinsel>();
        p->quaderBauen(Kasten(mitte, Vektor3f(8, 8, 8)), "textures/a/b", proj, k.kontext);
        k.weltHolenOderAnlegen()->kindEinfuegen(p);
        return p;
    }
    Kasten ga() { return a->grenzen(k.kontext); }
    Kasten gb() { return b->grenzen(k.kontext); }
};

void spiegeln() {
    Aufbau u;
    auswahlSpiegeln(u.be, u.k, 0);
    pruefe(kastenNah(u.ga(), Vektor3f(48, 0, 0), Vektor3f(64, 16, 16)) && kastenNah(u.gb(), Vektor3f(0, 0, 0), Vektor3f(16, 16, 16)),
           "Flip X: A und B tauschen den Platz " + text(u.ga()));
    pruefe(u.k.undo.undoName() == "mirrorSelected -axis x", "Flip X: Rueckgaengig-Name " + u.k.undo.undoName());
    pruefe(u.a->hatBeitragendeFlaechen(u.k.kontext) && u.a->anzahl() == 6, "Flip X: der Pinsel bleibt gueltig");
    u.k.undo.undo();
    auswahlSpiegeln(u.be, u.k, 1);
    pruefe(kastenNah(u.ga(), Vektor3f(0, 0, 0), Vektor3f(16, 16, 16)) && u.k.undo.undoName() == "mirrorSelected -axis y", "Flip Y: an y 8 gespiegelt");
    // nach der Ansicht: XY waagerecht x, senkrecht y; YZ waagerecht y, senkrecht z
    u.k.undo.undo();
    auswahlSpiegelnSicht(u.be, u.k, Sicht::XY, true);
    pruefe(u.k.undo.undoName() == "mirrorSelected -axis x", "Flip Horizontally (XY): x");
    auswahlSpiegelnSicht(u.be, u.k, Sicht::XY, false);
    pruefe(u.k.undo.undoName() == "mirrorSelected -axis y", "Flip Vertically (XY): y");
    auswahlSpiegelnSicht(u.be, u.k, Sicht::YZ, true);
    pruefe(u.k.undo.undoName() == "mirrorSelected -axis y", "Flip Horizontally (YZ): y");
    auswahlSpiegelnSicht(u.be, u.k, Sicht::YZ, false);
    pruefe(u.k.undo.undoName() == "mirrorSelected -axis z", "Flip Vertically (YZ): z");
    auswahlSpiegelnSicht(u.be, u.k, Sicht::XZ, false);
    pruefe(u.k.undo.undoName() == "mirrorSelected -axis z", "Flip Vertically (XZ): z");
}

void drehen() {
    {
        Aufbau u;
        auswahlDrehen(u.be, u.k, 2);   // z -90: A Mitte (8 8) -> relativ (-24 0) -> (0 24) -> (32 32)
        pruefe(kastenNah(u.ga(), Vektor3f(24, 24, 0), Vektor3f(40, 40, 16)) && kastenNah(u.gb(), Vektor3f(24, -24, 0), Vektor3f(40, -8, 16)),
               "Rotate Z (-90): A nach (24 24), B nach (24 -24) " + text(u.ga()) + " " + text(u.gb()));
        pruefe(u.k.undo.undoName() == "rotateSelected -axis z -angle -90", "Rotate Z: Rueckgaengig-Name");
    }
    {
        Aufbau u;
        auswahlDrehenSicht(u.be, u.k, Sicht::XY, true);   // Clockwise in XY: z -90
        pruefe(kastenNah(u.ga(), Vektor3f(24, 24, 0), Vektor3f(40, 40, 16)) && u.k.undo.undoName() == "rotateSelected Clockwise 90",
               "Rotate Clockwise (XY) = z -90 " + text(u.ga()));
        u.k.undo.undo();
        auswahlDrehenSicht(u.be, u.k, Sicht::XY, false);   // Anticlockwise: z +90 -> A (32 -16)
        pruefe(kastenNah(u.ga(), Vektor3f(24, -24, 0), Vektor3f(40, -8, 16)) && u.k.undo.undoName() == "rotateSelected Anticlockwise 90",
               "Rotate Anticlockwise (XY) = z +90 " + text(u.ga()));
    }
    {
        Aufbau u;
        auswahlDrehen(u.be, u.k, 1);   // y +90 um (32 8 8): (x, z) -> (z, -x): A (8 8) rel (-24 0) -> (0 24): Mitte (32 8 32)
        pruefe(kastenNah(u.ga(), Vektor3f(24, 0, 24), Vektor3f(40, 16, 40)) && u.k.undo.undoName() == "rotateSelected -axis y -angle 90",
               "Rotate Y (+90) " + text(u.ga()));
    }
    {   // Arbitrary rotation 0 0 90 (ohne Einrasten des Pivots)
        Aufbau u;
        auswahlDrehenEuler(u.be, u.k, Vektor3f(0, 0, 90));
        pruefe(kastenNah(u.ga(), Vektor3f(24, -24, 0), Vektor3f(40, -8, 16)), "Arbitrary rotation z 90 " + text(u.ga()));
        pruefe(u.k.undo.undoName() == "rotateSelectedEulerXYZ -x 0 -y 0 -z 90", "Arbitrary rotation: Rueckgaengig-Name " + u.k.undo.undoName());
        const Quaternion q = quaternionAusEulerGrad(Vektor3f(0, 0, 90));
        pruefe(nah(q[2], std::sqrt(0.5)) && nah(q[3], std::sqrt(0.5)) && nah(q[0], 0) && nah(q[1], 0), "quaternion_for_euler_xyz_degrees z 90");
    }
    {   // Arbitrary scale 2 1 1: um x 32
        Aufbau u;
        auswahlSkalieren(u.be, u.k, Vektor3f(2, 1, 1));
        pruefe(kastenNah(u.ga(), Vektor3f(-32, 0, 0), Vektor3f(0, 16, 16)) && kastenNah(u.gb(), Vektor3f(64, 0, 0), Vektor3f(96, 16, 16)),
               "Arbitrary scale 2 1 1 " + text(u.ga()));
        pruefe(u.k.undo.undoName() == "scaleSelected -x 2 -y 1 -z 1", "Arbitrary scale: Rueckgaengig-Name " + u.k.undo.undoName());
    }
}

void wiederholen() {
    Aufbau u;
    pruefe(u.be.wiederholbar().eins() && u.be.transformText(0) == "  Translate 0 0 0" && u.be.transformText(1) == "  Rotate 0 0 0" &&
               u.be.transformText(2) == "  Scale 1 1 1" && u.be.transformText(3) == "  Skew 4 0",
           "Repeat: Texte am Anfang");
    u.be.transformWiederholen();
    pruefe(u.k.undo.undoAnzahl() == 0 && u.k.welt()->kinder().size() == 2, "Repeat mit Identitaet: nichts");
    u.k.undo.start();
    u.be.verschiebeAuswahl(Vektor3f(16, 0, 0));
    u.k.undo.finish("translate");
    pruefe(u.be.transformText(0) == "x Translate 16 0 0", "Repeat: Text nach dem Verschieben '" + u.be.transformText(0) + "'");
    u.be.transformWiederholen();
    const auto& kinder = u.k.welt()->kinder();
    pruefe(kinder.size() == 4 && u.k.undo.undoName() == "repeatTransforms", "Repeat: Kopien (4 Pinsel)");
    if (kinder.size() == 4) {
        const Kasten k2 = std::static_pointer_cast<Pinsel>(kinder[2])->grenzen(u.k.kontext);
        pruefe(kastenNah(k2, Vektor3f(32, 0, 0), Vektor3f(48, 16, 16)) && kastenNah(u.ga(), Vektor3f(16, 0, 0), Vektor3f(32, 16, 16)),
               "Repeat: die Kopie von A weitere 16 verschoben, A bleibt " + text(k2));
        pruefe(u.k.auswahl.anzahl() == 2 && u.k.auswahl.gewaehlt(u.k.welt().get(), kinder[2].get()) && !u.k.auswahl.gewaehlt(u.k.welt().get(), u.a.get()),
               "Repeat: die Kopien sind gewaehlt");
    }
    // Drehung dazu: Repeat wendet Verschiebung und Drehung an
    auswahlDrehen(u.be, u.k, 2);
    pruefe(u.be.transformText(1).rfind("x Rotate 0 0 -0.707107", 0) == 0, "Repeat: Text der Drehung '" + u.be.transformText(1) + "'");
    u.be.transformZuruecksetzen(0);
    pruefe(u.be.transformText(0) == "  Translate 0 0 0" && u.be.transformText(1)[0] == 'x', "Reset Translate: nur die Verschiebung");
    u.be.transformZuruecksetzen();
    pruefe(u.be.wiederholbar().eins(), "Reset Transforms: alles");
    u.k.undo.start();
    u.be.skaliereAuswahl(Vektor3f(2, 1, 1));
    u.k.undo.finish("scale");
    pruefe(u.be.transformText(2) == "x Scale 2 1 1", "Repeat: Text der Skalierung");
    u.be.setzeModus(ManipModus::Clipper);
    pruefe(u.be.wiederholbar().eins(), "Wechsel in den Clipper: resetTransforms( eClip ) setzt alles zurueck");
}

void entitaet() {
    Karte k;
    AuswahlBearbeitung be{k};
    auto ent = k.neueEntitaet("info_player_start", false);
    ent->setzen("origin", "64 0 0");
    k.entitaetEinfuegen(ent);
    k.auswahl.waehlen(ent);
    be.pivotGeaendert();
    auswahlDrehenEuler(be, k, Vektor3f(0, 0, 90));
    pruefe(ent->wert("origin") == "64 0 0" && ent->wert("angle") == "90", "Arbitrary rotation einer Punktentitaet: angle 90 (" + ent->wert("angle") + ")");
    auswahlSpiegeln(be, k, 0);
    pruefe(ent->wert("origin") == "64 0 0", "Flip X einer Punktentitaet: der Ursprung bleibt (Pivot = Ursprung)");
}

}  // namespace

int main() {
    spiegeln();
    drehen();
    wiederholen();
    entitaet();
    std::printf("kartetransform: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
