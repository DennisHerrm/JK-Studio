// kartebearbeitung.cpp - Karten-Editor Stufe 1b, Punkt 7: Verschieben,
// Drehen, Skalieren, Verzerren, Groesse aendern und neue Pinsel wie
// NetRadiant-Custom (bhed/karte/manipulator.h, bhed/karte/bearbeitung.h).
//
// Sollwerte aus radiant/selection.cpp (SelectManipulator, MoveSelected,
// endMove mit den Rueckgaengig-Namen, translateSelected/rotateSelected/
// scaleSelected, TransformOriginManipulator), radiant/selection_mtor_*.cpp
// (Pfeile 64 lang, Bildviereck 16, Kreise 64 / 1.15 * 64), libs/pivot.h
// (Manipulator in Bildpunkten gleich gross), selection_mtor_drag.cpp
// (ziehen, Ebenen waehlen, DragNewBrush), xywindow.cpp (NewBrushDrag) und
// plugins/entity (origin, angle).
#include "bhed/karte/auswahlbefehle.h"
#include "bhed/karte/bearbeitung.h"
#include "bhed/karte/kamera.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texdef.h"
#include "bhed/karte/xyansicht.h"

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
bool nah(float a, float b, float eps = 1e-3F) { return std::fabs(a - b) <= eps; }

std::shared_ptr<Pinsel> quader(Karte& k, const Vektor3f& mitte, const Vektor3f& halb) {
    TexturProjektion proj;
    texdefStandard(TexdefArt::Quake, proj);
    auto p = std::make_shared<Pinsel>();
    p->quaderBauen(Kasten(mitte, halb), "textures/a/b", proj, k.kontext);
    k.weltHolenOderAnlegen()->kindEinfuegen(p);
    return p;
}

XyAnsicht ansicht2d(Sicht s, float massstab) {
    XyAnsicht v;
    v.typ = s;
    v.breite = 800;
    v.hoehe = 600;
    v.massstab = massstab;
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

// Weltpunkt der XY-Ansicht als Geraetepunkt (ueber die Projektion der Ansicht)
GeraeteVektor geraet(const XyAnsicht& v, float x, float y) {
    const Vektor3f p = projiziert(transformierterVektor4(multipliziert(v.projektion(), v.modelview()), Vektor4f(x, y, 0, 1)));
    return GeraeteVektor(p[0], p[1]);
}

// Manipulator_::getEpsilon: Verschieben/Drehen/Skalieren/Verzerren 8, Ziehen 12 Bildpunkte
const GeraeteVektor kEps8(8.0F / 800.0F, 8.0F / 600.0F);
const GeraeteVektor kEps12(12.0F / 800.0F, 12.0F / 600.0F);

Kasten grenzen(const Pinsel& p, const Karte& k) { return p.grenzen(k.kontext); }
bool kastenNah(const Kasten& a, const Vektor3f& o, const Vektor3f& e) {
    return nah(a.origin[0], o[0]) && nah(a.origin[1], o[1]) && nah(a.origin[2], o[2]) && nah(a.extents[0], e[0]) && nah(a.extents[1], e[1]) &&
           nah(a.extents[2], e[2]);
}

void rechnung() {
    // 90 Grad um z: quantisiert genau
    const Matrix4 r = drehungAusQuaternionQuantisiert(quaternionAchseWinkel(kAchseZ, kPi / 2));
    pruefe(r[0] == 0 && r[1] == 1 && r[4] == -1 && r[5] == 0, "matrix4_rotation_for_quaternion_quantised: 90 Grad genau");
    const Vektor3f w = winkelGedreht(Vektor3f(0, 0, 0), quaternionAchseWinkel(kAchseZ, kPi / 2));
    pruefe(nah(w[0], 0) && nah(w[1], 0) && nah(w[2], 90), "angles_rotated: Gier 90");
    pruefe(zahlText(16) == "16" && zahlText(0.5F) == "0.5" && zahlText(-0.707107F) == "-0.707107", "%g wie NRCs TextOutputStream");
    Verzerrung v;
    v.index = 4;
    v.wert = 0.5F;
    const Matrix4 t = transformAusKomponenten(Vektor3f(1, 2, 3), kQuaternionEins, Vektor3f(2, 1, 1), v);
    pruefe(t[0] == 2 && t[4] == 0.5F && t[12] == 1 && t[14] == 3, "matrix4_transform_for_components");
    // Pivot2World: in 2D ist der Manipulator in Bildpunkten gleich gross
    const XyAnsicht a = ansicht2d(Sicht::XY, 2);
    const Blick b = blick2d(a);
    Matrix4 welt;
    pivotWeltRaum(welt, verschiebungsMatrix(Vektor3f(10, 0, 0)), b.modelview(), b.projektion(), b.bildfeld());
    const Vektor3f spitze = transformierterPunkt(welt, Vektor3f(64, 0, 0));
    pruefe(nah(spitze[0], 10 + 32, 1e-2F), "Pfeil 64 Bildpunkte bei Massstab 2 = 32 Einheiten");
}

void verschieben2d() {
    Karte k;
    auto p = quader(k, Vektor3f(0, 0, 0), Vektor3f(32, 32, 32));
    k.auswahl.waehlen(k.welt(), p);
    AuswahlBearbeitung be(k);
    const XyAnsicht a = ansicht2d(Sicht::XY, 1);
    const Blick b = blick2d(a);
    pruefe(be.modus() == ManipModus::Verschieben, "Vorgabe des Auswahlsystems: Verschieben (SetManipulatorMode( eTranslate ))");
    // auf den x-Pfeil
    pruefe(be.manipulatorWaehlen(b, geraet(a, 30, 0), kEps8), "Klick auf den x-Pfeil greift");
    be.auswahlBewegen(b, geraet(a, 62, 0));
    pruefe(kastenNah(grenzen(*p, k), Vektor3f(32, 0, 0), Vektor3f(32, 32, 32)), "x-Pfeil: 32 nach rechts, nur x");
    be.auswahlBewegen(b, geraet(a, 62, 40));
    pruefe(kastenNah(grenzen(*p, k), Vektor3f(32, 0, 0), Vektor3f(32, 32, 32)), "x-Pfeil: y-Bewegung zaehlt nicht");
    pruefe(!be.bewegenBeenden(), "endMove: kein Ursprung");
    pruefe(k.undo.undoName() == "translateTool -xyz 32 0 0", "Rueckgaengig-Name: " + k.undo.undoName());
    k.undo.undo();
    pruefe(kastenNah(grenzen(*p, k), Vektor3f(0, 0, 0), Vektor3f(32, 32, 32)), "Rueckgaengig stellt die Lage wieder her");
    // Bildviereck (16): frei, Raster 8
    be.pivotGeaendert();
    pruefe(be.manipulatorWaehlen(b, geraet(a, 12, 12), kEps8), "Klick ins Bildviereck greift");
    be.auswahlBewegen(b, geraet(a, 12 + 19, 12 - 13));
    pruefe(kastenNah(grenzen(*p, k), Vektor3f(16, -16, 0), Vektor3f(32, 32, 32)), "frei: (19,-13) eingerastet auf (16,-16)");
    be.bewegenBeenden();
    pruefe(k.undo.undoName() == "translateTool -xyz 16 -16 0", "Name frei: " + k.undo.undoName());
    // Mitte: der Transformationsursprung (vor dem Manipulator)
    be.pivotGeaendert();
    const Vektor3f mitte = be.pivotZuWelt().t().vec3();
    pruefe(be.manipulatorWaehlen(b, geraet(a, mitte[0], mitte[1]), kEps8) && !be.bewegtSich(), "Mitte: Ursprung statt Manipulator");
    be.auswahlBewegen(b, geraet(a, mitte[0] + 32 + 3, mitte[1]));
    pruefe(be.bewegenBeenden() && be.pivotEigen(), "Ursprung verschoben: eigener Pivot");
    const Vektor3f neu = be.pivotZuWelt().t().vec3();
    pruefe(nah(neu[0], 48) && nah(neu[1], -16), "Ursprung rastet an der Kastenkante (48) ein");
    // Strg ohne Treffer: nichts (g_modifiers != c_modifierNone)
    be.zusatz.strg = true;
    pruefe(!be.manipulatorWaehlen(b, geraet(a, 50, -16), kEps8), "mit Strg greift der Pfeil nicht");
    be.zusatz.strg = false;
    // Zeichnung: x- und y-Pfeil, z steht senkrecht (nicht gezeigt)
    be.setzeModus(ManipModus::Verschieben);
    ManipZeichnung z;
    be.zeichnen(b, z);
    pruefe(z.draht.size() == 2 + 4 && !z.flach.empty() && z.grossePunkte.size() == 1, "Zeichnung: 2 Pfeile, Bildviereck, Ursprung");
}

void drehenSkalieren2d() {
    Karte k;
    auto p = quader(k, Vektor3f(0, 0, 0), Vektor3f(32, 16, 16));
    k.auswahl.waehlen(k.welt(), p);
    AuswahlBearbeitung be(k);
    const XyAnsicht a = ansicht2d(Sicht::XY, 1);
    const Blick b = blick2d(a);
    be.setzeModus(ManipModus::Drehen);
    // Bildkreis (1.15 * 64) unter 45 Grad: Achse = Blickrichtung
    pruefe(be.manipulatorWaehlen(b, geraet(a, 52, 52), kEps8), "Drehen: Bildkreis greift");
    be.auswahlBewegen(b, geraet(a, -52, 52));
    pruefe(kastenNah(grenzen(*p, k), Vektor3f(0, 0, 0), Vektor3f(16, 32, 16)), "90 Grad um die Blickachse: x und y getauscht");
    be.bewegenBeenden();
    // NRC schreibt die x/y/z-Teile des Quaternions, -0 wie printf
    pruefe(k.undo.undoName() == "rotateTool -eulerXYZ 0 -0 0.707107" || k.undo.undoName() == "rotateTool -eulerXYZ 0 0 -0.707107",
           "Name drehen: " + k.undo.undoName());
    k.undo.undo();
    // rotateSelected mit Punktentitaet: Ursprung um den Pivot, angle-Schluessel
    auto ent = k.neueEntitaet("info_player_start", false);
    ent->setzen("origin", "64 0 0");
    k.entitaetEinfuegen(ent);
    k.auswahl.leeren();
    k.auswahl.waehlen(ent);
    be.pivotGeaendert();
    k.undo.start();
    be.dreheAuswahl(quaternionAchseWinkel(kAchseZ, kPi / 2));
    k.undo.finish("rotate");
    pruefe(ent->wert("origin") == "64 0 0" && ent->wert("angle") == "90", "Punktentitaet um sich selbst: angle 90 (" + ent->wert("angle") + ")");
    k.auswahl.waehlen(k.welt(), p);
    be.pivotGeaendert();
    k.undo.start();
    be.verschiebeAuswahl(Vektor3f(16, 0, 0));
    k.undo.finish("move");
    pruefe(ent->wert("origin") == "80 0 0", "translateSelected: origin " + ent->wert("origin"));
    k.undo.undo();
    k.auswahl.leeren();
    k.auswahl.waehlen(k.welt(), p);
    // Skalieren: x-Linie
    be.setzeModus(ManipModus::Skalieren);
    pruefe(be.manipulatorWaehlen(b, geraet(a, 30, 0), kEps8), "Skalieren: x-Linie greift");
    be.auswahlBewegen(b, geraet(a, 62, 0));
    pruefe(kastenNah(grenzen(*p, k), Vektor3f(0, 0, 0), Vektor3f(64, 16, 16)), "x doppelt so breit (Kastenweg)");
    be.bewegenBeenden();
    pruefe(k.undo.undoName() == "scaleTool -scale 2 1 1", "Name skalieren: " + k.undo.undoName());
    ManipZeichnung z;
    be.zeichnen(b, z);
    pruefe(z.ohneTiefe.size() == 3 + 4, "Skalier-Manipulator: 3 Linien + Viereck ohne Tiefentest ($POINT)");
}

void ziehen2d() {
    Karte k;
    auto p = quader(k, Vektor3f(0, 0, 0), Vektor3f(32, 32, 32));
    k.auswahl.waehlen(k.welt(), p);
    AuswahlBearbeitung be(k);
    be.setzeModus(ManipModus::Ziehen);
    const XyAnsicht a = ansicht2d(Sicht::XY, 1);
    const Blick b = blick2d(a);
    // in den Pinsel: ziehen
    pruefe(be.manipulatorWaehlen(b, geraet(a, 0, 0), kEps12), "Ziehen: Klick in den Pinsel greift");
    be.auswahlBewegen(b, geraet(a, 17, 7));
    pruefe(kastenNah(grenzen(*p, k), Vektor3f(16, 8, 0), Vektor3f(32, 32, 32)), "gezogen um (16,8) (Raster 8)");
    be.bewegenBeenden();
    pruefe(k.undo.undoName() == "dragTool", "Name ziehen: " + k.undo.undoName());
    k.undo.undo();
    // neben den Pinsel (rechts, 18 Bildpunkte): die +x-Flaeche
    be.pivotGeaendert();
    pruefe(be.manipulatorWaehlen(b, geraet(a, 50, 0), kEps12), "Ziehen neben dem Pinsel: Ebene gewaehlt");
    pruefe(k.auswahl.flaechen().size() == 1, "genau die +x-Flaeche ist (voruebergehend) gewaehlt");
    be.auswahlBewegen(b, geraet(a, 66, 0));
    const Kasten g = grenzen(*p, k);
    pruefe(nah(g.origin[0] - g.extents[0], -32) && nah(g.origin[0] + g.extents[0], 48) && nah(g.extents[1], 32), "Groesse: +x-Seite auf 48");
    be.bewegenBeenden();
    pruefe(k.auswahl.flaechen().empty() && k.auswahl.anzahl() == 1, "danach keine Flaechen mehr gewaehlt");
    pruefe(k.undo.undoName() == "dragTool", "Name Groesse: " + k.undo.undoName());
    // ueber die Gegenseite hinaus: binaere Suche nach der groessten sinnvollen Verschiebung
    k.undo.undo();
    be.pivotGeaendert();
    pruefe(be.manipulatorWaehlen(b, geraet(a, 50, 0), kEps12), "Ziehen neben dem Pinsel (2)");
    be.auswahlBewegen(b, geraet(a, -80, 0));
    const Kasten g2 = grenzen(*p, k);
    pruefe(nah(g2.origin[0] - g2.extents[0], -32) && nah(g2.origin[0] + g2.extents[0], -24), "ueber die Gegenseite: bleibt 8 breit (-32..-24)");
    be.bewegenBeenden();
    // Rueckgaengig ohne Bewegung: kein Eintrag
    const std::size_t vorher = k.undo.undoAnzahl();
    be.pivotGeaendert();
    pruefe(be.manipulatorWaehlen(b, geraet(a, 0, 0), kEps12), "Klick ohne Bewegung");
    be.bewegenBeenden();
    pruefe(k.undo.undoAnzahl() == vorher, "ohne Bewegung entsteht kein Rueckgaengig-Eintrag");
}

void neuerPinsel() {
    Karte k;
    (void)k.weltHolenOderAnlegen();
    AuswahlBearbeitung be(k);
    const XyAnsicht a = ansicht2d(Sicht::XY, 1);
    pruefe(be.neuerPinselMoeglich(), "ohne Auswahl: NewBrushDrag");
    be.neuerPinselBeginnen(a, 400 + 17, 300 - 15);
    be.neuerPinselZiehen(a, 400 + 79, 300 - 49, false, false);
    pruefe(be.neuerPinselBeenden(), "Pinsel entstanden");
    pruefe(k.undo.undoName() == "brushDragNew", "Name: " + k.undo.undoName());
    pruefe(k.auswahl.anzahl() == 1, "der neue Pinsel ist gewaehlt");
    auto neu = std::dynamic_pointer_cast<Pinsel>(k.auswahl.elemente()[0].primitiv.lock());
    pruefe(neu != nullptr && kastenNah(grenzen(*neu, k), Vektor3f(48, 32, 0), Vektor3f(32, 16, 64)), "Grenzen (16..80, 16..48, Arbeitsbereich -64..64)");
    pruefe(neu != nullptr && neu->anzahl() == 6 && (*neu)[0].shader() == "textures/", "6 Flaechen, Shader des Texturfensters (texdef_name_default)");
    k.undo.undo();
    pruefe(k.welt()->kinder().empty(), "Rueckgaengig entfernt den Pinsel");
    // Quadrat mit Shift
    k.auswahl.leeren();
    be.neuerPinselBeginnen(a, 400, 300);
    be.neuerPinselZiehen(a, 400 + 64, 300 - 16, true, false);
    be.neuerPinselBeenden();
    neu = std::dynamic_pointer_cast<Pinsel>(k.auswahl.elemente()[0].primitiv.lock());
    pruefe(neu != nullptr && nah(grenzen(*neu, k).extents[0], 32) && nah(grenzen(*neu, k).extents[1], 32), "Shift: Quadrat");
    // 3D: Ziehen ohne Auswahl baut einen Pinsel (DragNewBrush), 256 + Raster * Wurzel 3 vor der Kamera
    Karte k3;
    (void)k3.weltHolenOderAnlegen();
    AuswahlBearbeitung be3(k3);
    be3.setzeModus(ManipModus::Ziehen);
    const Blick b = blick3d(Vektor3f(0, 0, 64), Vektor3f(-30, 0, 0));
    pruefe(be3.manipulatorWaehlen(b, GeraeteVektor(0, 0), kEps12), "3D ohne Auswahl: neuer Pinsel");
    be3.auswahlBewegen(b, GeraeteVektor(0.2F, -0.2F));
    pruefe(k3.auswahl.anzahl() == 1, "der Pinsel entsteht beim Ziehen und ist gewaehlt");
    be3.bewegenBeenden();
    pruefe(k3.undo.undoName() == "dragTool", "3D-Pinsel: Name " + k3.undo.undoName());
    if (k3.auswahl.anzahl() == 1) {
        auto p3 = std::dynamic_pointer_cast<Pinsel>(k3.auswahl.elemente()[0].primitiv.lock());
        const Kasten g3 = grenzen(*p3, k3);
        // Startpunkt: (0,0,64) + (cos 30, 0, -sin 30) * (256 + 8 * sqrt 3) = (233.8, 0, -71) -> Raster (232, 0, -72)
        pruefe(nah(g3.origin[0] - g3.extents[0], 232) || nah(g3.origin[0] + g3.extents[0], 232), "3D-Pinsel beginnt am eingerasteten Startpunkt x 232");
        pruefe(nah(g3.extents[2], 4), "3D-Pinsel: Hoehe ein Raster (8)");
    }
}

void einrasten() {
    Karte k;
    auto p = quader(k, Vektor3f(17, 17, 17), Vektor3f(16, 16, 16));   // 1..33
    p->eckenEinrasten(8, k.kontext);
    pruefe(kastenNah(grenzen(*p, k), Vektor3f(16, 16, 16), Vektor3f(16, 16, 16)) && p->anzahl() == 6, "Ecken aufs Raster: 1..33 -> 0..32, 6 Flaechen");
    // um 45 Grad gedreht: die Huelle der eingerasteten Ecken
    auto q = quader(k, Vektor3f(0, 0, 0), Vektor3f(32, 32, 32));
    q->transformieren(drehungZGrad(45), k.kontext);
    q->eckenEinrasten(8, k.kontext);
    const Kasten g = grenzen(*q, k);
    pruefe(nah(g.extents[0], 48) && nah(g.extents[1], 48) && nah(g.extents[2], 32) && q->anzahl() == 6, "gedreht: Ecken auf (+-48,0) und (0,+-48)");
}

void befehle() {
    // libs/uniquenames.h
    const Name n = nameLesen("func_door01");
    pruefe(n.text == "func_door" && n.zahl.wert == 1 && n.zahl.mindestBreite == 2 && nameSchreiben(n) == "func_door01", "name_read / name_write");
    EindeutigeNamen namen;
    namen.einfuegen(nameLesen("t1"));
    pruefe(nameSchreiben(namen.eindeutig(nameLesen("t1"))) == "t2" && nameSchreiben(namen.eindeutig(nameLesen("t5"))) == "t5", "make_unique");
    pruefe(schluesselIstName("target2") && schluesselIstName("killtarget") && !schluesselIstName("target2x"), "keyIsNameQuake3");

    Karte k;
    AuswahlEinstellung e;
    auto a = quader(k, Vektor3f(0, 0, 0), Vektor3f(16, 16, 16));
    auto b = quader(k, Vektor3f(64, 0, 0), Vektor3f(16, 16, 16));
    const auto welt = k.welt();
    // Klonen (Space): der Klon ist gewaehlt, das Original nicht
    k.auswahl.waehlen(welt, a);
    klonen(k, e, false);
    pruefe(welt->kinder().size() == 3 && k.auswahl.anzahl() == 1 && !k.auswahl.gewaehlt(welt.get(), a.get()), "Klonen: neuer Pinsel gewaehlt");
    pruefe(k.undo.undoName() == "cloneSelected", "Name: " + k.undo.undoName());
    k.undo.undo();
    pruefe(welt->kinder().size() == 2, "Klonen rueckgaengig");
    // Umkehren (I)
    k.auswahl.leeren();
    k.auswahl.waehlen(welt, a);
    auswahlUmkehren(k, e);
    pruefe(k.auswahl.anzahl() == 1 && k.auswahl.gewaehlt(welt.get(), b.get()), "Umkehren: der andere Pinsel");
    // Entitaeten mit Namen: Klonen mit eindeutigen Namen (Shift+Space)
    auto n1 = k.neueEntitaet("info_null", false);
    n1->setzen("origin", "0 0 0");
    n1->setzen("targetname", "t1");
    k.entitaetEinfuegen(n1);
    auto n2 = k.neueEntitaet("info_null", false);
    n2->setzen("origin", "32 0 0");
    k.entitaetEinfuegen(n2);
    auto l = k.neueEntitaet("light", false);
    l->setzen("origin", "0 32 0");
    k.entitaetEinfuegen(l);
    k.auswahl.leeren();
    k.auswahl.waehlen(n1);
    klonen(k, e, true);
    const auto klon = k.auswahl.elemente().empty() ? nullptr : k.auswahl.elemente()[0].entitaet.lock();
    pruefe(klon != nullptr && klon != n1 && klon->wert("targetname") == "t2", "Shift+Space: targetname eindeutig (t2)");
    k.undo.undo();
    k.auswahl.leeren();
    k.auswahl.waehlen(n1);
    klonen(k, e, false);
    const auto klon2 = k.auswahl.elemente().empty() ? nullptr : k.auswahl.elemente()[0].entitaet.lock();
    pruefe(klon2 != nullptr && klon2->wert("targetname") == "t1", "Space: Namen bleiben (t1)");
    k.undo.undo();
    // Alle vom Typ (Shift+A)
    k.auswahl.leeren();
    k.auswahl.waehlen(n1);
    alleVomTyp(k, e, "textures/");
    pruefe(k.auswahl.anzahl() == 2 && k.auswahl.gewaehlt(n1.get()) && k.auswahl.gewaehlt(n2.get()), "Shift+A: beide info_null");
    k.auswahl.leeren();
    alleVomTyp(k, e, "textures/a/b");
    pruefe(k.auswahl.anzahl() == 2 && k.auswahl.gewaehlt(welt.get(), a.get()), "Shift+A ohne Entitaet: Pinsel mit dem Shader des Texturfensters");
    // Kopieren / Einfuegen
    k.auswahl.leeren();
    k.auswahl.waehlen(welt, b);
    const std::string text = kopieren(k);
    pruefe(text.find("// brush 0") != std::string::npos && text.find("// brush 1") == std::string::npos && text.find("\"classname\" \"worldspawn\"") != std::string::npos,
           "Kopieren: worldspawn mit genau dem gewaehlten Pinsel");
    einfuegen(k, text);
    pruefe(welt->kinder().size() == 3 && k.auswahl.anzahl() == 1 && !k.auswahl.gewaehlt(welt.get(), b.get()), "Einfuegen: der eingefuegte Pinsel ist gewaehlt");
    pruefe(k.undo.undoName() == "paste", "Name: " + k.undo.undoName());
    // Loeschen
    loescheAuswahl(k, e);
    pruefe(welt->kinder().size() == 2 && k.auswahl.anzahl() == 0 && k.undo.undoName() == "deleteSelected", "Loeschen");
    // Verstecken / Zeigen
    k.auswahl.waehlen(welt, a);
    pruefe(versteckeGewaehlte(k, false) && a->versteckt && k.auswahl.anzahl() == 0 && !welt->versteckt, "H: versteckt und abgewaehlt");
    zeigeVersteckte(k);
    pruefe(!a->versteckt, "Shift+H: wieder sichtbar");
    // Am Raster ausrichten
    auto c = quader(k, Vektor3f(17, 17, 17), Vektor3f(16, 16, 16));
    k.auswahl.leeren();
    k.auswahl.waehlen(welt, c);
    k.auswahl.waehlen(n2);
    n2->setzen("origin", "35 3 0");
    amRasterAusrichten(k, e, 8);
    pruefe(kastenNah(grenzen(*c, k), Vektor3f(16, 16, 16), Vektor3f(16, 16, 16)) && n2->wert("origin") == "32 0 0", "Ctrl+G: Pinsel und Entitaet aufs Raster");
    pruefe(k.undo.undoName() == "snapSelected -grid 8", "Name: " + k.undo.undoName());
    // Abwaehlen (Esc): erst Flaechen, dann alles
    k.auswahl.flaecheWaehlen(c, c->flaechen()[0]);
    auswahlAufheben(k, e);
    pruefe(k.auswahl.flaechen().empty() && k.auswahl.anzahl() == 2, "Esc: zuerst die Flaechen");
    auswahlAufheben(k, e);
    pruefe(k.auswahl.anzahl() == 0, "Esc: dann alles");
    const Vektor3f s = schubsRichtung(0, 8, Sicht::XZ);
    pruefe(s == Vektor3f(-8, 0, 0) && schubsRichtung(1, 8, Sicht::YZ) == Vektor3f(0, 0, 8), "NudgeSelection: Achsen je Ansicht");
}

void verzerren2d() {
    Karte k;
    auto p = quader(k, Vektor3f(0, 0, 0), Vektor3f(32, 32, 32));
    k.auswahl.waehlen(k.welt(), p);
    AuswahlBearbeitung be(k);
    be.setzeModus(ManipModus::Verzerren);
    const XyAnsicht a = ansicht2d(Sicht::XY, 1);
    const Blick b = blick2d(a);
    // Kasten innen: frei verschieben (TranslateFreeXY_Z)
    pruefe(be.manipulatorWaehlen(b, geraet(a, 5, 5), kEps8), "Transform: Kasten innen greift");
    be.auswahlBewegen(b, geraet(a, 5 + 32, 5));
    pruefe(kastenNah(grenzen(*p, k), Vektor3f(32, 0, 0), Vektor3f(32, 32, 32)), "Transform: verschoben");
    be.bewegenBeenden();
    pruefe(k.undo.undoName() == "transformTool", "Name Transform: " + k.undo.undoName());
}

}  // namespace

int main() {
    const auto schritt = [](const char* n) {
        std::printf("-- %s\n", n);
        std::fflush(stdout);
    };
    schritt("rechnung");
    rechnung();
    schritt("verschieben2d");
    verschieben2d();
    schritt("drehenSkalieren2d");
    drehenSkalieren2d();
    schritt("ziehen2d");
    ziehen2d();
    schritt("neuerPinsel");
    neuerPinsel();
    schritt("einrasten");
    einrasten();
    schritt("befehle");
    befehle();
    schritt("verzerren2d");
    verzerren2d();
    std::printf("kartebearbeitung: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
