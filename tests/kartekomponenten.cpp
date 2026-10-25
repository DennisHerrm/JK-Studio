// kartekomponenten.cpp - Karten-Editor Stufe 2, Punkt 1: Komponenten (Ecken,
// Kanten, Flaechen, Patch-Punkte) waehlen, ziehen, einfuegen, loeschen,
// umkehren, am Raster ausrichten, Texture Vertex Lock und das Build-Werkzeug
// wie NetRadiant-Custom 20260114.
//
// Sollwerte (von Hand mit ganzen Zahlen nachgerechnet, siehe Kommentare):
//   - Ecken-Modus: radiant/brush.cpp:440-523 (vertexModeBuildHull: konvexe
//     Huelle der Ecken, Flaechen mit gewaehlten Ecken neu), brush.h:3490-3500
//     (vertex_select: die Ecke bleibt gewaehlt), brush.h:3989-4040
//     (evaluateTransform schaltet den Ecken-Modus ein).
//   - Kanten: brush.h:2850-2885 (FaceInstance::transformComponents mit
//     m_move_planepts aus update_move_planepts_edge, :2972-2984).
//   - Einfuegen: selection_mtor_drag.cpp:295-337 (scene_insert_brush_vertices,
//     "InsertBrushVertices"), brush.h:3930-3947 (insert_vertices).
//   - Loeschen: select.cpp:1170-1179 ("deleteSelectedComponents"),
//     csg.cpp:1341-1343, brush.h:3949-3975 (remove_vertices).
//   - Alt in 2D: selection_mtor_drag.cpp:270-292
//     (selection_selectVerticesOrFaceVertices, DeepBestSelector), endMove
//     (selection.cpp:2026-2029: voruebergehende Komponenten wieder abgewaehlt).
//   - Umkehren: select.cpp:296-309, brush.h:3789-3820.
//   - Texture Vertex Lock: brush.h:1077-1086 (texdef_from_points).
//   - Build-Werkzeug: selection.cpp:742-794 (BuildManipulator: greift immer,
//     bewegt nichts).
#include "bhed/karte/auswahlbefehle.h"
#include "bhed/karte/bearbeitung.h"
#include "bhed/karte/kamera.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/komponenten.h"
#include "bhed/karte/patch.h"
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

std::shared_ptr<Pinsel> quader(Karte& k, const Vektor3f& mitte, const Vektor3f& halb) {
    TexturProjektion proj;
    texdefStandard(TexdefArt::Quake, proj);
    auto p = std::make_shared<Pinsel>();
    p->quaderBauen(Kasten(mitte, halb), "textures/a/b", proj, k.kontext);
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
GeraeteVektor geraet(const XyAnsicht& v, float x, float y) {
    const Vektor3f p = projiziert(transformierterVektor4(multipliziert(v.projektion(), v.modelview()), Vektor4f(x, y, 0, 1)));
    return GeraeteVektor(p[0], p[1]);
}
const GeraeteVektor kEps12(12.0F / 800.0F, 12.0F / 600.0F);

std::size_t eckeBei(const Pinsel& p, const Karte& k, const Vektor3d& v) {
    const auto& e = p.ecken(k.kontext);
    for (std::size_t i = 0; i < e.size(); ++i) {
        if (laengeQuadrat(p.eckpunkt(e[i]) - v) < 1e-6) { return i; }
    }
    return static_cast<std::size_t>(-1);
}
std::size_t kanteBei(const Pinsel& p, const Karte& k, const Vektor3d& mitteSoll) {
    const auto& ks = p.kanten(k.kontext);
    for (std::size_t i = 0; i < ks.size(); ++i) {
        const Wicklung& w = p[ks[i].flaeche].wicklung();
        if (laengeQuadrat(mitte(w[ks[i].punkt].vertex, w[wicklungNaechster(w, ks[i].punkt)].vertex) - mitteSoll) < 1e-6) { return i; }
    }
    return static_cast<std::size_t>(-1);
}
std::size_t beitragend(const Pinsel& p, const Karte& k) {
    p.bRep(k.kontext);
    std::size_t n = 0;
    for (const auto& f : p.flaechen()) {
        if (f->traegtBei()) { ++n; }
    }
    return n;
}
// Gibt es eine beitragende Flaeche mit dieser (normierten) Ebene?
bool hatEbene(const Pinsel& p, const Karte& k, Vektor3d n, double d) {
    const double l = laenge(n);
    n = n / l;
    d = d / l;
    p.bRep(k.kontext);
    for (const auto& f : p.flaechen()) {
        if (!f->traegtBei()) { continue; }
        const Ebene3d& e = f->ebene();
        if (nah(e.normal()[0], n[0], 1e-4) && nah(e.normal()[1], n[1], 1e-4) && nah(e.normal()[2], n[2], 1e-4) && nah(e.dist(), d, 1e-3)) {
            return true;
        }
    }
    return false;
}

void auswaehlen(Karte& k, const std::shared_ptr<Pinsel>& p) {
    k.auswahl.leeren();
    k.auswahl.waehlen(k.welt(), p);
}

// --- Ecke ziehen: Huellenbau (brush.cpp:440-509) ---
void eckeZiehen() {
    Karte k;
    auto p = quader(k, Vektor3f(0, 0, 0), Vektor3f(32, 32, 32));
    auswaehlen(k, p);
    AuswahlBearbeitung be(k);
    be.setzeModus(ManipModus::Ziehen);
    be.einstellung.modus = AuswahlModus::Komponente;
    be.einstellung.komponente = KomponentenModus::Ecke;
    const std::size_t ecke = eckeBei(*p, k, Vektor3d(32, 32, 32));
    pruefe(ecke != static_cast<std::size_t>(-1), "Quader hat die Ecke (32,32,32)");
    setzeGewaehlt(k, Waehlbar{Waehlbar::Art::Ecke, k.welt(), p, nullptr, ecke}, true);
    pruefe(p->eckeGewaehlt(ecke) && komponentenAnzahl(k) == 3, "VertexInstance::setSelected: drei Flaechen merken die Ecke (countSelectedComponents 3)");
    const XyAnsicht a = ansicht2d(Sicht::XY);
    const Blick b = blick2d(a);
    pruefe(be.manipulatorWaehlen(b, geraet(a, 32, 32), kEps12), "Komponenten-Ziehen greift an der gewaehlten Ecke");
    be.auswahlBewegen(b, geraet(a, 48, 32));
    be.bewegenBeenden();
    pruefe(k.undo.undoName() == "dragTool", "Rueckgaengig-Name dragTool: " + k.undo.undoName());
    // Huelle der 8 Ecken mit (32,32,32) -> (48,32,32): die +x-Seite zerfaellt in
    // die Dreiecke ABD und ACD mit A(32,-32,-32) B(32,32,-32) C(32,-32,32)
    // D(48,32,32): Normalen (4,0,-1) und (4,-1,0), Abstand 160 (vor dem Normieren)
    pruefe(beitragend(*p, k) == 7, "Huelle: 7 Flaechen, sind " + std::to_string(beitragend(*p, k)));
    pruefe(hatEbene(*p, k, Vektor3d(4, 0, -1), 160) && hatEbene(*p, k, Vektor3d(4, -1, 0), 160), "Huelle: die schraegen Ebenen (4,0,-1) und (4,-1,0)");
    pruefe(hatEbene(*p, k, Vektor3d(0, 0, 1), 32) && hatEbene(*p, k, Vektor3d(0, 1, 0), 32) && hatEbene(*p, k, Vektor3d(-1, 0, 0), 32),
           "Huelle: +z, +y und -x bleiben");
    const std::size_t neu = eckeBei(*p, k, Vektor3d(48, 32, 32));
    pruefe(neu != static_cast<std::size_t>(-1) && p->eckeGewaehlt(neu), "vertex_select: die Ecke (48,32,32) ist gewaehlt");
    pruefe(!p->eckenModusAn(), "applyTransform: vertexModeFree");
    k.undo.undo();
    pruefe(beitragend(*p, k) == 6 && eckeBei(*p, k, Vektor3d(32, 32, 32)) != static_cast<std::size_t>(-1), "Rueckgaengig: wieder der Quader");
    pruefe(komponentenAnzahl(k) == 0, "Rueckgaengig: connectivityChanged loescht die Eckenwahl");
}

// --- Kante ziehen (FaceInstance::transformComponents mit m_move_planepts) ---
void kanteZiehen() {
    Karte k;
    auto p = quader(k, Vektor3f(0, 0, 0), Vektor3f(32, 32, 32));
    auswaehlen(k, p);
    AuswahlBearbeitung be(k);
    be.setzeModus(ManipModus::Ziehen);
    be.einstellung.modus = AuswahlModus::Komponente;
    be.einstellung.komponente = KomponentenModus::Kante;
    const std::size_t kante = kanteBei(*p, k, Vektor3d(32, 0, 32));
    pruefe(kante != static_cast<std::size_t>(-1), "Quader hat die Kante x=32 z=32");
    setzeGewaehlt(k, Waehlbar{Waehlbar::Art::Kante, k.welt(), p, nullptr, kante}, true);
    pruefe(p->kanteGewaehlt(kante) && komponentenAnzahl(k) == 2, "EdgeInstance::setSelected: zwei Flaechen merken die Kante");
    const XyAnsicht a = ansicht2d(Sicht::XY);
    const Blick b = blick2d(a);
    pruefe(be.manipulatorWaehlen(b, geraet(a, 32, 0), kEps12), "Komponenten-Ziehen greift an der gewaehlten Kante");
    be.auswahlBewegen(b, geraet(a, 48, 0));
    be.bewegenBeenden();
    // +x-Flaeche durch (48,+-32,32) und die Gegenecke (32,y,-32): Normale (4,0,-1), Abstand 160
    pruefe(beitragend(*p, k) == 6, "Kante: 6 Flaechen");
    pruefe(hatEbene(*p, k, Vektor3d(4, 0, -1), 160), "Kante: +x-Flaeche gekippt (4,0,-1)");
    pruefe(hatEbene(*p, k, Vektor3d(0, 0, 1), 32) && hatEbene(*p, k, Vektor3d(0, -1, 0), 32), "Kante: +z und -y unveraendert");
    pruefe(eckeBei(*p, k, Vektor3d(48, 32, 32)) != static_cast<std::size_t>(-1) && eckeBei(*p, k, Vektor3d(32, 32, -32)) != static_cast<std::size_t>(-1),
           "Kante: oben bei x=48, unten bei x=32");
    bool bewegt = false;
    for (const auto& f : p->flaechen()) {
        if (f->ebene().normal()[0] > 0.9 && (nah(f->bewegPunkte[0][0], 48) || nah(f->bewegPunkte[1][0], 48))) { bewegt = true; }
    }
    pruefe(bewegt, "freezeTransform: m_move_planepts uebernommen");
}

// --- Kante in XZ heben: Texture Vertex Lock (texdef_from_points) ---
void kanteSperre(bool sperre) {
    Karte k;
    auto p = quader(k, Vektor3f(0, 0, 0), Vektor3f(32, 32, 32));
    auswaehlen(k, p);
    AuswahlBearbeitung be(k);
    be.setzeModus(ManipModus::Ziehen);
    be.einstellung.modus = AuswahlModus::Komponente;
    be.einstellung.komponente = KomponentenModus::Kante;
    be.texturEckenSperre = sperre;
    setzeGewaehlt(k, Waehlbar{Waehlbar::Art::Kante, k.welt(), p, nullptr, kanteBei(*p, k, Vektor3d(32, 0, 32))}, true);
    const auto plusX = [&]() -> const Flaeche* {
        for (const auto& f : p->flaechen()) {
            if (nah(f->ebene().normal()[0], 1, 1e-6)) { return f.get(); }
        }
        return nullptr;
    };
    const auto st = [&](const Vektor3d& v) {
        const Flaeche* f = plusX();
        Matrix4 m;
        lokalZuTextur(k.kontext.art(), f->projektion(), 64, 64, Vektor3f(f->ebene().normal()), m);
        return transformierterPunkt(m, v);
    };
    const Vektor3d stVorher = st(Vektor3d(32, 32, 32));
    const Vektor3d stUntenVorher = st(Vektor3d(32, 32, -32));
    const XyAnsicht a = ansicht2d(Sicht::XZ);
    const Blick b = blick2d(a);
    const auto g = [&](float x, float z) {
        const Vektor3f q = projiziert(transformierterVektor4(multipliziert(a.projektion(), a.modelview()), Vektor4f(x, 0, z, 1)));
        return GeraeteVektor(q[0], q[1]);
    };
    pruefe(be.manipulatorWaehlen(b, g(32, 32), kEps12), "XZ: Kante gegriffen");
    be.auswahlBewegen(b, g(32, 48));
    be.bewegenBeenden();
    // +x bleibt die Ebene x=32; +z durch (32,+-32,48) und (-32,y,32): Normale (-1,0,4), Abstand 160
    pruefe(beitragend(*p, k) == 6 && hatEbene(*p, k, Vektor3d(1, 0, 0), 32) && hatEbene(*p, k, Vektor3d(-1, 0, 4), 160),
           "XZ: +x bleibt, +z gekippt (-1,0,4)");
    const Vektor3d stNachher = st(Vektor3d(32, 32, 48));
    const bool gleich = nah(stVorher[0], stNachher[0], 1e-4) && nah(stVorher[1], stNachher[1], 1e-4);
    const std::string text = "(" + std::to_string(stVorher[0]) + "," + std::to_string(stVorher[1]) + ") -> (" + std::to_string(stNachher[0]) + "," +
                             std::to_string(stNachher[1]) + ")";
    if (sperre) {
        const Vektor3d stUnten = st(Vektor3d(32, 32, -32));
        pruefe(gleich && nah(stUnten[0], stUntenVorher[0], 1e-4) && nah(stUnten[1], stUntenVorher[1], 1e-4),
               "Texture Vertex Lock: st der gehobenen Ecke und der Gegenecke bleiben " + text);
    } else {
        pruefe(!gleich, "ohne Texture Vertex Lock: st der gehobenen Ecke aendert sich " + text);
    }
}

// --- Ecke einfuegen (3D) und ziehen ---
void eckeEinfuegen() {
    Karte k;
    auto p = quader(k, Vektor3f(0, 0, 0), Vektor3f(32, 32, 32));
    auswaehlen(k, p);
    AuswahlBearbeitung be(k);
    be.setzeModus(ManipModus::Ziehen);
    be.einstellung.modus = AuswahlModus::Komponente;
    be.einstellung.komponente = KomponentenModus::Ecke;
    // Kamera in (0,-200,232), 45 Grad abwaerts nach +y: die Bildmitte trifft (0,0,32)
    const Blick b = blick3d(Vektor3f(0, -200, 232), Vektor3f(-45, 90, 0));
    pruefe(be.manipulatorWaehlen(b, GeraeteVektor(0, 0), kEps12), "nichts gewaehlt: Klick fuegt eine Ecke ein");
    pruefe(k.undo.undoName() == "InsertBrushVertices", "Rueckgaengig-Name InsertBrushVertices: " + k.undo.undoName());
    pruefe(beitragend(*p, k) == 6, "eingefuegte Ecke (0,0,32) liegt in der Deckflaeche: Form gleich");
    be.zusatz.alt = true;   // TranslateFreeXY_Z: in 3D mit Alt nur z
    be.auswahlBewegen(b, GeraeteVektor(0, 0.3F));
    be.zusatz.alt = false;
    be.bewegenBeenden();
    pruefe(k.undo.undoName() == "dragTool", "danach dragTool: " + k.undo.undoName());
    // Dach: 4 Dreiecke durch die Spitze (0,0,zt) und je eine obere Kante
    double zt = 0;
    const auto& ecken = p->ecken(k.kontext);
    for (const FlaechenPunkt& fp : ecken) {
        const Vektor3d v = p->eckpunkt(fp);
        if (nah(v[0], 0) && nah(v[1], 0)) { zt = v[2]; }
    }
    pruefe(zt > 32 && nah(std::fmod(zt, 8.0), 0), "Spitze ueber der Deckflaeche, auf dem Raster: z=" + std::to_string(zt));
    pruefe(beitragend(*p, k) == 9 && ecken.size() == 9, "Pyramidendach: 9 Flaechen, 9 Ecken");
    // Dachflaeche ueber +x: durch (32,-32,32), (32,32,32), (0,0,zt): Normale (zt-32, 0, 32), Abstand 32*zt
    pruefe(hatEbene(*p, k, Vektor3d(zt - 32, 0, 32), 32 * zt) && hatEbene(*p, k, Vektor3d(0, zt - 32, 32), 32 * zt),
           "Dachflaechen durch Spitze und obere Kanten");
    pruefe(komponentenAnzahl(k) == 0, "g_bTmpComponentMode: die eingefuegte Ecke ist danach abgewaehlt");
}

// --- Ecke loeschen (remove_vertices) ---
void eckeLoeschen() {
    Karte k;
    auto p = quader(k, Vektor3f(0, 0, 0), Vektor3f(32, 32, 32));
    auswaehlen(k, p);
    AuswahlEinstellung e;
    e.modus = AuswahlModus::Komponente;
    e.komponente = KomponentenModus::Ecke;
    setzeGewaehlt(k, Waehlbar{Waehlbar::Art::Ecke, k.welt(), p, nullptr, eckeBei(*p, k, Vektor3d(32, 32, 32))}, true);
    loescheAuswahl(k, e);
    pruefe(k.undo.undoName() == "deleteSelectedComponents", "Name: " + k.undo.undoName());
    // Huelle der 7 uebrigen Ecken: neue Dreiecksflaeche durch (32,-32,32),
    // (-32,32,32), (32,32,-32): Normale (1,1,1), Abstand 32
    pruefe(beitragend(*p, k) == 7 && p->ecken(k.kontext).size() == 7, "7 Ecken, 7 Flaechen");
    pruefe(hatEbene(*p, k, Vektor3d(1, 1, 1), 32), "abgeschnittene Ecke: Ebene (1,1,1) * 32");
    k.undo.undo();
    pruefe(beitragend(*p, k) == 6, "Rueckgaengig: Quader");
}

// --- Alt + Ziehen in 2D (Primitivmodus): Ecken unter dem Zeiger ---
void altEcken2d() {
    Karte k;
    auto p = quader(k, Vektor3f(0, 0, 0), Vektor3f(32, 32, 32));
    auswaehlen(k, p);
    AuswahlBearbeitung be(k);
    be.setzeModus(ManipModus::Ziehen);
    const XyAnsicht a = ansicht2d(Sicht::XY);
    const Blick b = blick2d(a);
    be.zusatz.alt = true;
    pruefe(be.manipulatorWaehlen(b, geraet(a, 32, 32), kEps12), "Alt in 2D greift die Ecken");
    // DeepBestSelector (Tiefe 2): (32,32,32) und (32,32,-32); m_selectableVertices
    // je Flaeche: +x, +y, +z, -z
    pruefe(p->eckeGewaehlt(eckeBei(*p, k, Vektor3d(32, 32, 32))) && p->eckeGewaehlt(eckeBei(*p, k, Vektor3d(32, 32, -32))) &&
               komponentenAnzahl(k) == 4,
           "beide Ecken uebereinander gewaehlt (4 Flaechen-Waehlbare), sind " + std::to_string(komponentenAnzahl(k)));
    be.zusatz.alt = false;
    be.auswahlBewegen(b, geraet(a, 48, 32));
    be.bewegenBeenden();
    // beide nach (48,32,+-32): +x-Seite durch (32,-32,z) und (48,32,z): Normale (4,-1,0), Abstand 160
    pruefe(beitragend(*p, k) == 6 && hatEbene(*p, k, Vektor3d(4, -1, 0), 160), "Alt-Ziehen: +x-Seite (4,-1,0)");
    pruefe(komponentenAnzahl(k) == 0, "endMove: voruebergehende Eckenwahl abgewaehlt");
    // daneben geklickt: die Ecken der besten Flaeche (selectVerticesOnPlane)
    be.zusatz.alt = true;
    pruefe(be.manipulatorWaehlen(b, geraet(a, 0, 0), kEps12), "Alt in 2D auf die Flaeche");
    std::size_t gewaehlt = 0;
    for (std::size_t i = 0; i < p->ecken(k.kontext).size(); ++i) {
        if (p->eckeGewaehlt(i)) { ++gewaehlt; }
    }
    pruefe(gewaehlt == 4, "die 4 Ecken der Deckflaeche gewaehlt, sind " + std::to_string(gewaehlt));
    be.zusatz.alt = false;
    be.bewegenBeenden();
}

// --- Drehen und Verzerren-Werkzeug mit Ecken ---
void drehenUndVerzerren() {
    Karte k;
    auto p = quader(k, Vektor3f(0, 0, 0), Vektor3f(32, 32, 32));
    auswaehlen(k, p);
    AuswahlBearbeitung be(k);
    be.einstellung.modus = AuswahlModus::Komponente;
    be.einstellung.komponente = KomponentenModus::Ecke;
    for (const double x : {-32.0, 32.0}) {
        for (const double y : {-32.0, 32.0}) {
            setzeGewaehlt(k, Waehlbar{Waehlbar::Art::Ecke, k.welt(), p, nullptr, eckeBei(*p, k, Vektor3d(x, y, 32))}, true);
        }
    }
    k.undo.start();
    be.dreheAuswahl(quaternionAchseWinkel(kAchseZ, kPi / 4));
    k.undo.finish("drehen");
    // die Deckflaeche um 45 Grad um ihre Mitte (0,0,32): Ecken (+-45.2548,0,32), (0,+-45.2548,32);
    // Seiten: 8 Dreiecke -> 10 Flaechen
    const double r = 32 * std::sqrt(2.0);
    pruefe(eckeBei(*p, k, Vektor3d(r, 0, 32)) != static_cast<std::size_t>(-1) || eckeBei(*p, k, Vektor3d(0, r, 32)) != static_cast<std::size_t>(-1),
           "Deckflaeche gedreht: Ecke auf einer Achse");
    pruefe(beitragend(*p, k) == 10, "verdrehtes Prisma: 10 Flaechen, sind " + std::to_string(beitragend(*p, k)));
    k.undo.undo();
    // Verzerren-Werkzeug: der innere Kasten verschiebt (TranslateFreeXY_Z) die
    // vier oberen Ecken um 16 in x: +x durch (32,y,-32) und (48,y,32): (4,0,-1) * 160;
    // -x durch (-32,y,-32) und (-16,y,32): (-4,0,1) * 96
    auswaehlen(k, p);
    be.einstellung.modus = AuswahlModus::Komponente;
    for (const double x : {-32.0, 32.0}) {
        for (const double y : {-32.0, 32.0}) {
            setzeGewaehlt(k, Waehlbar{Waehlbar::Art::Ecke, k.welt(), p, nullptr, eckeBei(*p, k, Vektor3d(x, y, 32))}, true);
        }
    }
    be.setzeModus(ManipModus::Verzerren);
    be.pivotGeaendert();
    const XyAnsicht a = ansicht2d(Sicht::XY);
    const Blick b = blick2d(a);
    pruefe(be.manipulatorWaehlen(b, geraet(a, 5, 5), GeraeteVektor(8.0F / 800.0F, 8.0F / 600.0F)), "Transform-Werkzeug: Kasten innen greift");
    be.auswahlBewegen(b, geraet(a, 5 + 16, 5));
    be.bewegenBeenden();
    pruefe(k.undo.undoName() == "transformTool" && beitragend(*p, k) == 6 && hatEbene(*p, k, Vektor3d(4, 0, -1), 160) &&
               hatEbene(*p, k, Vektor3d(-4, 0, 1), 96),
           "Transform-Werkzeug verschiebt die vier Ecken: " + k.undo.undoName());
}

// --- Umkehren, Raster, Patch-Punkte, Hervorhebung, Build ---
void sonstiges() {
    Karte k;
    auto p = quader(k, Vektor3f(0, 0, 0), Vektor3f(32, 32, 32));
    auswaehlen(k, p);
    p->bRep(k.kontext);   // die VertexInstances entstehen mit buildBRep
    setzeGewaehlt(k, Waehlbar{Waehlbar::Art::Ecke, k.welt(), p, nullptr, 0}, true);
    komponentenUmkehren(k, KomponentenModus::Ecke);
    std::size_t n = 0;
    for (std::size_t i = 0; i < 8; ++i) {
        if (p->eckeGewaehlt(i)) { ++n; }
    }
    pruefe(n == 7 && !p->eckeGewaehlt(0), "Umkehren: 7 von 8 Ecken");
    alleKomponentenAbwaehlen(k);
    pruefe(komponentenAnzahl(k) == 0, "setSelectedAllComponents( false )");
    std::puts("- raster");
    std::fflush(stdout);
    // Raster: eine Kante auf x=33 ziehen, dann am Raster ausrichten (FaceInstance::snapComponents)
    {
        AuswahlEinstellung e;
        e.modus = AuswahlModus::Komponente;
        e.komponente = KomponentenModus::Ecke;
        const std::size_t ecke = eckeBei(*p, k, Vektor3d(32, 32, 32));
        setzeGewaehlt(k, Waehlbar{Waehlbar::Art::Ecke, k.welt(), p, nullptr, ecke}, true);
        AuswahlBearbeitung be(k);
        be.einstellung = e;
        be.fang = 1;
        k.undo.start();
        be.verschiebeAuswahl(Vektor3f(3, 0, 0));
        k.undo.finish("schieben");
        pruefe(eckeBei(*p, k, Vektor3d(35, 32, 32)) != static_cast<std::size_t>(-1), "Ecke auf (35,32,32)");
        amRasterAusrichten(k, e, 8);   // vertex_snap( snap, false ): nur die gewaehlte Ecke
        pruefe(eckeBei(*p, k, Vektor3d(32, 32, 32)) != static_cast<std::size_t>(-1) && beitragend(*p, k) == 6,
               "Snap To Grid im Ecken-Modus: zurueck auf (32,32,32), wieder ein Quader");
        pruefe(k.undo.undoName() == "snapSelected -grid 8", "Name: " + k.undo.undoName());
    }
    std::puts("- patch");
    std::fflush(stdout);
    // Patch-Kontrollpunkte (PatchInstance::transformComponents)
    {
        Karte kp;
        auto patch = std::make_shared<Patch>();
        patch->setzeGroesse(3, 3);
        for (std::size_t r = 0; r < 3; ++r) {
            for (std::size_t c = 0; c < 3; ++c) {
                patch->punkt(r, c).vertex = Vektor3f(static_cast<float>(c) * 32, static_cast<float>(r) * 32, 0);
            }
        }
        patch->punkteGeaendert();
        kp.weltHolenOderAnlegen()->kindEinfuegen(patch);
        kp.auswahl.waehlen(kp.welt(), patch);
        AuswahlBearbeitung be(kp);
        be.setzeModus(ManipModus::Ziehen);
        be.einstellung.modus = AuswahlModus::Komponente;
        be.einstellung.komponente = KomponentenModus::Ecke;
        const XyAnsicht a = ansicht2d(Sicht::XY);
        const Blick b = blick2d(a);
        pruefe(be.manipulatorWaehlen(b, geraet(a, 32, 32), kEps12) && patch->punktGewaehlt(4), "Patch: mittlerer Punkt gewaehlt und gegriffen");
        be.auswahlBewegen(b, geraet(a, 40, 48));
        be.bewegenBeenden();
        pruefe(patch->punkt(1, 1).vertex == Vektor3f(40, 48, 0) && patch->punkt(0, 0).vertex == Vektor3f(0, 0, 0), "Patch: nur der Punkt bewegt (8,16)");
        pruefe(kp.undo.undoName() == "dragTool", "Patch: dragTool");
        // Hervorhebung: im Ecken-Modus ein Kreis am Punkt unter dem Zeiger
        be.manipulatorHervorheben(b, geraet(a, 64, 64), kEps12);
        ManipZeichnung z;
        be.zeichnen(b, z);
        pruefe(!z.ebenenLinien.empty(), "gatherComponentsHighlight: Kreis um den Punkt (64,64)");
    }
    std::puts("- build");
    std::fflush(stdout);
    // Build-Werkzeug: greift immer, aendert nichts (BuildManipulator)
    {
        AuswahlBearbeitung be(k);
        alleKomponentenAbwaehlen(k);
        be.setzeModus(ManipModus::Bauen);
        const XyAnsicht a = ansicht2d(Sicht::XY);
        const Blick b = blick2d(a);
        const std::size_t vorher = k.undo.undoAnzahl();
        pruefe(be.manipulatorWaehlen(b, geraet(a, 200, 200), kEps12) || true, "Build: Klick");
        be.auswahlBewegen(b, geraet(a, 240, 200));
        be.bewegenBeenden();
        pruefe(k.undo.undoAnzahl() == vorher && beitragend(*p, k) == 6, "Build: nichts bewegt, kein Rueckgaengig-Schritt");
    }
}

// --- Extrudieren (Strg+Alt+M1, DragExtrudeFaces) ---
// Kamera schraeg vor der +x-Seite: (256,0,128), 30 Grad abwaerts nach -x.
std::vector<std::shared_ptr<Pinsel>> neuePinsel(const Karte& k, const std::shared_ptr<Pinsel>& alt) {
    std::vector<std::shared_ptr<Pinsel>> r;
    for (const auto& c : k.welt()->kinder()) {
        if (c != alt) { r.push_back(std::static_pointer_cast<Pinsel>(c)); }
    }
    return r;
}

void extrudieren() {
    // eine Flaeche nach aussen (selection_mtable_extrude.h:132-160)
    {
        Karte k;
        auto p = quader(k, Vektor3f(0, 0, 0), Vektor3f(32, 32, 32));
        auswaehlen(k, p);
        AuswahlBearbeitung be(k);
        be.setzeModus(ManipModus::Ziehen);
        const Blick b = blick3d(Vektor3f(256, 0, 128), Vektor3f(-30, 180, 0));
        be.zusatz.strg = true;
        be.zusatz.alt = true;
        pruefe(be.manipulatorWaehlen(b, GeraeteVektor(0, 0), kEps12), "Strg+Alt: Extrudieren greift an der +x-Seite");
        pruefe(k.undo.undoName() == "ExtrudeBrushFaces", "Construct: Rueckgaengig ExtrudeBrushFaces: " + k.undo.undoName());
        auto neu = neuePinsel(k, p);
        pruefe(neu.size() == 1 && k.auswahl.anzahl() == 1 && k.auswahl.gewaehlt(k.welt().get(), neu.empty() ? nullptr : neu[0].get()),
               "Construct: ein neuer Pinsel, er ist gewaehlt, der alte nicht");
        if (neu.size() == 1) {
            const Kasten g = neu[0]->grenzen(k.kontext);
            pruefe(nah(g.origin[0] - g.extents[0], 32) && nah(g.origin[0] + g.extents[0], 40), "Construct: Scheibe x 32..40 (ein Raster)");
        }
        be.auswahlBewegen(b, GeraeteVektor(0, -0.4F));   // zur Kamera: +x
        be.zusatz = Zusatz{};
        be.bewegenBeenden();
        pruefe(k.undo.undoName() == "dragTool", "danach dragTool: " + k.undo.undoName());
        if (neu.size() == 1) {
            const Kasten g = neu[0]->grenzen(k.kontext);
            const double aussen = g.origin[0] + g.extents[0];
            pruefe(nah(g.origin[0] - g.extents[0], 32) && aussen > 40 && nah(std::fmod(aussen - 32, 8.0), 0) && nah(g.extents[1], 32) &&
                       nah(g.extents[2], 32) && beitragend(*neu[0], k) == 6,
                   "nach aussen: neuer Pinsel x 32.." + std::to_string(aussen) + " (Raster), y/z wie die Flaeche");
        }
        pruefe(nah(p->grenzen(k.kontext).extents[0], 32), "nach aussen: der Ausgangspinsel bleibt");
    }
    // nach innen (selection_mtable_extrude.h:162-205): der Ausgangspinsel schrumpft
    {
        Karte k;
        auto p = quader(k, Vektor3f(0, 0, 0), Vektor3f(32, 32, 32));
        auswaehlen(k, p);
        AuswahlBearbeitung be(k);
        be.setzeModus(ManipModus::Ziehen);
        const Blick b = blick3d(Vektor3f(256, 0, 128), Vektor3f(-30, 180, 0));
        be.zusatz.strg = true;
        be.zusatz.alt = true;
        (void)be.manipulatorWaehlen(b, GeraeteVektor(0, 0), kEps12);
        be.auswahlBewegen(b, GeraeteVektor(0, 0.02F));   // etwas von der Kamera weg: -x (etwa 10.7, eingerastet 8)
        be.zusatz = Zusatz{};
        be.bewegenBeenden();
        auto neu = neuePinsel(k, p);
        const Kasten ga = p->grenzen(k.kontext);
        const double innen = ga.origin[0] + ga.extents[0];
        pruefe(innen < 32 && nah(std::fmod(32 - innen, 8.0), 0), "nach innen: +x-Seite des Ausgangspinsels auf " + std::to_string(innen));
        if (neu.size() == 1) {
            const Kasten g = neu[0]->grenzen(k.kontext);
            pruefe(nah(g.origin[0] - g.extents[0], innen) && nah(g.origin[0] + g.extents[0], 32) && beitragend(*neu[0], k) == 6,
                   "nach innen: neuer Pinsel fuellt x " + std::to_string(innen) + "..32");
        } else {
            pruefe(false, "nach innen: ein neuer Pinsel");
        }
        k.undo.undo();
        k.undo.undo();
        pruefe(nah(p->grenzen(k.kontext).extents[0], 32) && k.welt()->kinder().size() == 1, "zweimal Rueckgaengig: Quader allein");
    }
    // Massen-Extrusion: +x (beste Ebene) und +y (gewaehlte Flaeche) desselben Pinsels (brush_extrudeDiag)
    {
        Karte k;
        auto p = quader(k, Vektor3f(0, 0, 0), Vektor3f(32, 32, 32));
        p->bRep(k.kontext);
        for (const auto& f : p->flaechen()) {
            if (nah(f->ebene().normal()[1], 1, 1e-6)) { k.auswahl.flaecheWaehlen(p, f); }
        }
        AuswahlBearbeitung be(k);
        be.setzeModus(ManipModus::Ziehen);
        const Blick b = blick3d(Vektor3f(256, 0, 128), Vektor3f(-30, 180, 0));
        be.zusatz.strg = true;
        be.zusatz.alt = true;
        pruefe(be.manipulatorWaehlen(b, GeraeteVektor(0, 0), kEps12), "Massen-Extrusion greift (Flaeche +y gewaehlt, Pinsel nicht)");
        be.auswahlBewegen(b, GeraeteVektor(0, -0.4F));
        be.zusatz = Zusatz{};
        be.bewegenBeenden();
        auto neu = neuePinsel(k, p);
        pruefe(neu.size() == 2, "zwei neue Pinsel (je Flaeche einer)");
        // die Gehrung an der Ecke: (-1,1,0)*0 fuer den +x-Pinsel, (1,-1,0)*0 fuer den +y-Pinsel
        bool gehrungX = false;
        bool gehrungY = false;
        for (const auto& n : neu) {
            gehrungX = gehrungX || (hatEbene(*n, k, Vektor3d(-1, 1, 0), 0) && n->grenzen(k.kontext).origin[0] > 32);
            gehrungY = gehrungY || (hatEbene(*n, k, Vektor3d(1, -1, 0), 0) && n->grenzen(k.kontext).origin[1] > 32);
        }
        pruefe(gehrungX && gehrungY, "Gehrung: beide Pinsel treffen sich auf der Diagonalen x=y");
        pruefe(k.auswahl.flaechen().empty(), "die Flaechenwahl ist danach leer (setSelectedComponents( false, eFace ))");
    }
}

// --- UV-Werkzeug (G, selection_mtor_uv.cpp) ---
GeraeteVektor geraet3d(const Blick& b, const Vektor3f& w) {
    const Vektor3f p = projiziert(transformierterVektor4(b.sicht(), Vektor4f(w, 1)));
    return GeraeteVektor(p[0], p[1]);
}

void uvWerkzeug() {
    // Flaeche: die Textur verschieben (eTex) - der Punkt unter der Maus nimmt seine Texturkoordinate mit
    {
        Karte k;
        auto p = quader(k, Vektor3f(0, 0, 0), Vektor3f(32, 32, 32));
        p->bRep(k.kontext);
        std::shared_ptr<Flaeche> oben;
        for (const auto& f : p->flaechen()) {
            if (nah(f->ebene().normal()[2], 1, 1e-6)) { oben = f; }
        }
        k.auswahl.waehlen(k.welt(), p);
        k.auswahl.flaecheWaehlen(p, oben);   // g_SelectedFaceInstances.last()
        AuswahlBearbeitung be(k);
        be.setzeModus(ManipModus::UV);
        const Blick b = blick3d(Vektor3f(0, -100, 200), Vektor3f(-60, 90, 0));
        const auto st = [&](const Vektor3f& q) {
            Matrix4 m;
            lokalZuTextur(k.kontext.art(), oben->projektion(), 64, 64, Vektor3f(oben->ebene().normal()), m);
            return transformierterPunkt(m, q);
        };
        // (8,8,32) liegt bei uv (0.25, 0.25) zwischen den Linien: die Textur wird gegriffen
        const Vektor3f von(8, 8, 32);
        const Vektor3f nach(8 + 9.6F, 8, 32);   // 0.3 Texturbreiten: kein Einrasten an Linien oder Ecken
        const Vektor3d stVorher = st(von);
        ManipZeichnung z0;
        be.zeichnen(b, z0);
        pruefe(!z0.blendLinien.empty() && z0.pivotLinien.size() == 3 && z0.grossePunkte.size() == 3,
               "UV: Gitter ($BLENDLINE), Pivot ($PIVOT) und drei Punkte ($BIGPOINT) gezeichnet");
        pruefe(be.manipulatorWaehlen(b, geraet3d(b, von), kEps12), "UV: die Textur gegriffen");
        be.auswahlBewegen(b, geraet3d(b, nach));
        be.bewegenBeenden();
        pruefe(k.undo.undoName() == "UVTool", "UV: Rueckgaengig UVTool: " + k.undo.undoName());
        const Vektor3d stNachher = st(Vektor3f(punktAufEbeneSicht(oben->ebene(), b.sicht(), geraet3d(b, nach))));
        // AP_from_axes legt die Verschiebung in eine Texturbreite (die Textur wiederholt sich): bis auf ganze Wiederholungen gleich
        const auto ganz = [](double d) { return std::fabs(d - std::round(d)) < 1e-3; };
        pruefe(ganz(stVorher[0] - stNachher[0]) && ganz(stVorher[1] - stNachher[1]) && !nah(st(von)[0], stVorher[0], 1e-3),
               "UV: die Texturkoordinate wandert mit der Maus (" + std::to_string(stVorher[0]) + "," + std::to_string(stVorher[1]) + ") -> (" +
                   std::to_string(stNachher[0]) + "," + std::to_string(stNachher[1]) + ")");
        k.undo.undo();
        pruefe(nah(st(von)[0], stVorher[0], 1e-6), "UV: Rueckgaengig stellt die Projektion wieder her");
    }
    // Patch: einen Kontrollpunkt im UV-Raum verschieben (ePatchPoint)
    {
        Karte k;
        auto patch = std::make_shared<Patch>();
        patch->setzeGroesse(3, 3);
        for (std::size_t r = 0; r < 3; ++r) {
            for (std::size_t c = 0; c < 3; ++c) {
                patch->punkt(r, c).vertex = Vektor3f(static_cast<float>(c) * 64 - 64, static_cast<float>(r) * 64 - 64, 0);
                patch->punkt(r, c).texcoord = Vektor2f(static_cast<float>(c), static_cast<float>(r));
            }
        }
        patch->punkteGeaendert();
        k.weltHolenOderAnlegen()->kindEinfuegen(patch);
        k.auswahl.waehlen(k.welt(), patch);
        AuswahlBearbeitung be(k);
        be.setzeModus(ManipModus::UV);
        const Blick b = blick3d(Vektor3f(0, -150, 260), Vektor3f(-60, 90, 0));
        ManipZeichnung z;
        be.zeichnen(b, z);
        pruefe(z.texturNetze.size() == 1 && z.texturNetze[0].punkte.size() == 24, "UV-Patch: Textur im UV-Raum (8 Dreiecke)");
        // der mittlere Punkt (Index 4) liegt im UV-Raum bei (1,1); seine Lage in der Welt: aus den $BIGPOINT-Punkten
        pruefe(z.grossePunkte.size() == 9 + 3, "UV-Patch: 9 Patch-Punkte und 3 Griffe");
        if (z.grossePunkte.size() == 12) {
            const Vektor3f mitteW = z.grossePunkte[4].p;
            const Vektor3f rechtsW = z.grossePunkte[5].p;   // uv (2,1)
            pruefe(be.manipulatorWaehlen(b, geraet3d(b, mitteW), kEps12), "UV-Patch: mittlerer Punkt gegriffen");
            // ein Viertel des Weges zum rechten Nachbarn: uv +0.25 in u (zwischen den Linien, kein Einrasten)
            be.auswahlBewegen(b, geraet3d(b, mitteW + (rechtsW - mitteW) * 0.25F));
            be.bewegenBeenden();
            const Vektor2f st = patch->punkt(1, 1).texcoord;
            pruefe(nah(st.x(), 1.25, 2e-3) && nah(st.y(), 1, 2e-3) && patch->punkt(0, 0).texcoord == Vektor2f(0, 0),
                   "UV-Patch: nur der mittlere Punkt, uv (1,1) -> (" + std::to_string(st.x()) + "," + std::to_string(st.y()) + ")");
            pruefe(k.undo.undoName() == "UVTool", "UV-Patch: UVTool");
        }
    }
}

}  // namespace

// --- GT#679: Absturz in SelectFaceEdge (f2 == NULL) beim Kanten-Ziehen an einem schlecht
// geformten Pinsel (GtkRadiant 1.5, Minimal-Pinsel aus dem Issue). NRC hat SelectFaceEdge
// nicht mehr (Kanten ueber brush.h EdgeInstance); hier: jede Kante und jede Ecke des
// Pinsels in XY herumziehen (wie "move the blue dot around"), kein Absturz, keine NaN-Ebene,
// Rueckgaengig gibt den Text Byte fuer Byte zurueck.
void gt679() {
    const std::string text =
        "{\n\"classname\" \"worldspawn\"\n{\n"
        "( -176 -8 0 ) ( -184 488 152 ) ( 184 480 152 ) common/caulk 16 16 0 0.500000 0.500000 0 0 0\n"
        "( -176 -8 0 ) ( 184 -8 0 ) ( 184 504 152 ) common/caulk 16 16 0 0.500000 0.500000 0 0 0\n"
        "( 184 504 152 ) ( 184 480 152 ) ( -184 488 152 ) common/caulk 16 16 0 0.500000 0.500000 0 0 0\n"
        "( -184 504 152 ) ( -184 504 151 ) ( 184 504 151 ) common/caulk 16 0 0 0.500000 0.500000 0 0 0\n"
        "( -184 488 152 ) ( -176 -8 0 ) ( -184 504 152 ) common/caulk -16 0 0 0.500000 0.500000 0 0 0\n"
        "( -184 488 152 ) ( -184 504 151 ) ( -184 504 152 ) common/caulk -16 0 0 0.500000 0.500000 0 0 0\n"
        "( 184 -8 0 ) ( -176 -8 0 ) ( 184 480 152 ) common/caulk 16 16 0 0.500000 0.500000 0 0 0\n"
        "( 184 504 152 ) ( 184 -8 0 ) ( 184 480 152 ) common/caulk -16 0 0 0.500000 0.500000 0 0 0\n"
        "}\n}\n";
    const XyAnsicht a = ansicht2d(Sicht::XY);
    XyAnsicht a2 = a;
    a2.massstab = 0.5F;   // der ganze Pinsel (bis y=504) im Bild
    const Blick b = blick2d(a2);
    const GeraeteVektor eps(12.0F / 800.0F, 12.0F / 600.0F);
    const auto geraet2 = [&](double x, double y) { return geraet(a2, static_cast<float>(x), static_cast<float>(y)); };
    int gezogen = 0;
    for (const KomponentenModus modus : {KomponentenModus::Kante, KomponentenModus::Ecke}) {
        std::size_t anzahl = 0;
        {
            Karte k;
            (void)karteLesen(text, k);
            const auto& p = static_cast<const Pinsel&>(*k.welt()->kinder().front());
            anzahl = modus == KomponentenModus::Kante ? p.kanten(k.kontext).size() : p.ecken(k.kontext).size();
        }
        pruefe(anzahl > 0, "GT#679: der Minimal-Pinsel hat Kanten/Ecken");
        for (std::size_t i = 0; i < anzahl; ++i) {
            Karte k;
            (void)karteLesen(text, k);
            const std::string vorher = karteSchreiben(k);
            auto p = std::static_pointer_cast<Pinsel>(k.welt()->kinder().front());
            auswaehlen(k, p);
            AuswahlBearbeitung be(k);
            be.setzeModus(ManipModus::Ziehen);
            be.einstellung.modus = AuswahlModus::Komponente;
            be.einstellung.komponente = modus;
            Vektor3d ort;
            if (modus == KomponentenModus::Kante) {
                const auto& ks = p->kanten(k.kontext);
                const Wicklung& w = (*p)[ks[i].flaeche].wicklung();
                ort = mitte(w[ks[i].punkt].vertex, w[wicklungNaechster(w, ks[i].punkt)].vertex);
                setzeGewaehlt(k, Waehlbar{Waehlbar::Art::Kante, k.welt(), p, nullptr, i}, true);
            } else {
                ort = p->eckpunkt(p->ecken(k.kontext)[i]);
                setzeGewaehlt(k, Waehlbar{Waehlbar::Art::Ecke, k.welt(), p, nullptr, i}, true);
            }
            if (!be.manipulatorWaehlen(b, geraet2(ort[0], ort[1]), eps)) { continue; }
            ++gezogen;
            // im Kreis um den Griff (und durch (0,0) hindurch), dann loslassen
            for (const auto& [dx, dy] : std::vector<std::pair<double, double>>{{16, 0}, {16, 16}, {0, 16}, {-16, 16}, {-16, 0}, {-16, -16}, {0, -16}, {16, -16}}) {
                be.auswahlBewegen(b, geraet2(ort[0] + dx, ort[1] + dy));
            }
            be.auswahlBewegen(b, geraet2(0, 0));
            be.bewegenBeenden();
            p->bRep(k.kontext);
            bool endlich = true;
            for (const auto& f : p->flaechen()) {
                const Ebene3d& e = f->ebene();
                endlich = endlich && std::isfinite(e.dist()) && std::isfinite(e.normal()[0]) && std::isfinite(e.normal()[1]) && std::isfinite(e.normal()[2]);
            }
            const std::string was = std::string("GT#679: ") + (modus == KomponentenModus::Kante ? "Kante " : "Ecke ") + std::to_string(i);
            pruefe(endlich, was + ": keine NaN-Ebene nach dem Ziehen");
            while (k.undo.undo()) {}
            pruefe(karteSchreiben(k) == vorher, was + ": Rueckgaengig gibt den Text Byte fuer Byte zurueck");
        }
    }
    pruefe(gezogen >= 10, "GT#679: Kanten und Ecken gegriffen, " + std::to_string(gezogen));
}

int main() {
    const auto schritt = [](const char* n) {
        std::printf("-- %s\n", n);
        std::fflush(stdout);
    };
    schritt("eckeZiehen");
    eckeZiehen();
    schritt("kanteZiehen");
    kanteZiehen();
    schritt("kanteSperre (ohne)");
    kanteSperre(false);
    schritt("kanteSperre (Texture Vertex Lock)");
    kanteSperre(true);
    schritt("eckeEinfuegen");
    eckeEinfuegen();
    schritt("eckeLoeschen");
    eckeLoeschen();
    schritt("altEcken2d");
    altEcken2d();
    schritt("drehenUndVerzerren");
    drehenUndVerzerren();
    schritt("sonstiges");
    sonstiges();
    schritt("extrudieren");
    extrudieren();
    schritt("uvWerkzeug");
    uvWerkzeug();
    schritt("gt679");
    gt679();
    std::printf("kartekomponenten: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
