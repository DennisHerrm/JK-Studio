// kartebobtoolz.cpp - Karten-Editor Stufe 4 Punkt 3: das Plugin bobToolz
// ohne Oberflaeche (bhed/karte/bobtoolz.h).
//
// Geprueft wird die Wirkung jedes Befehls wie in NetRadiant-Custom 20260114,
// contrib/bobtoolz (funchandlers-GTK.cpp Do*, shapes.cpp, DShape.cpp,
// DPatch.cpp, DTreePlanter.cpp, DTrainDrawer.cpp, DBobView.cpp, visfind.cpp):
//   - Intersect / Find Duplicates: welche Pinsel gewaehlt werden, Meldung;
//   - Polygon Builder (regulaer, invers, mit Rand), Stair Builder (drei
//     Stile, Hoehe teilbar), Door Builder, Aperture Door, Pit-O-Matic: was
//     an die Stelle des gewaehlten Pinsels tritt (Pinsel, Entitaeten,
//     Schluessel, Grenzen, Texturen);
//   - Reset Textures, Caulk Selection, Brush Cleanup, Explode Entity, Make
//     Chain, Merge/Split Patches, Flip Terrain;
//   - Path Plotter (Bahn eines trigger_push), Plot Splines, Tree Planter und
//     Drop Entity, Vis Viewer (Blaetter einer .bsp);
//   - je Befehl ein Undo-Schritt "bobToolz.*".
//
// Aufruf: kartebobtoolz <tests/karte/q3map2>
#include "bhed/karte/bobtoolz.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace bhed::karte;
namespace bt = bhed::karte::bobtoolz;

namespace {

int g_fehler = 0;
int g_pruefungen = 0;
void pruefe(bool ok, const std::string& was) {
    ++g_pruefungen;
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

using V = std::array<double, 3>;
V minus(const V& a, const V& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
V kreuz(const V& a, const V& b) { return {(a[1] * b[2]) - (a[2] * b[1]), (a[2] * b[0]) - (a[0] * b[2]), (a[0] * b[1]) - (a[1] * b[0])}; }
double punkt(const V& a, const V& b) { return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]); }

// Ein Pinsel als Text der .map: jede Ebene aus drei Punkten, so gedreht, dass
// die Normale ( (p2 - p0) x (p1 - p0), Quake-Format ) von "innen" weg zeigt.
std::string pinselText(std::vector<std::array<V, 3>> ebenen, const V& innen, const std::string& shader) {
    std::string t = "{\n";
    char z[256];
    for (auto& e : ebenen) {
        const V n = kreuz(minus(e[2], e[0]), minus(e[1], e[0]));
        if (punkt(n, minus(e[0], innen)) < 0) { std::swap(e[1], e[2]); }
        std::snprintf(z, sizeof(z), "( %g %g %g ) ( %g %g %g ) ( %g %g %g ) %s 0 0 0 0.5 0.5 0 0 0\n", e[0][0], e[0][1], e[0][2], e[1][0], e[1][1],
                      e[1][2], e[2][0], e[2][1], e[2][2], shader.c_str());
        t += z;
    }
    return t + "}\n";
}

std::string quader(const V& a, const V& b, const std::string& shader = "test/wand") {
    const V innen{(a[0] + b[0]) / 2, (a[1] + b[1]) / 2, (a[2] + b[2]) / 2};
    return pinselText({{V{a[0], a[1], a[2]}, V{a[0], b[1], a[2]}, V{a[0], a[1], b[2]}},
                       {V{b[0], a[1], a[2]}, V{b[0], b[1], a[2]}, V{b[0], a[1], b[2]}},
                       {V{a[0], a[1], a[2]}, V{b[0], a[1], a[2]}, V{a[0], a[1], b[2]}},
                       {V{a[0], b[1], a[2]}, V{b[0], b[1], a[2]}, V{a[0], b[1], b[2]}},
                       {V{a[0], a[1], a[2]}, V{b[0], a[1], a[2]}, V{a[0], b[1], a[2]}},
                       {V{a[0], a[1], b[2]}, V{b[0], a[1], b[2]}, V{a[0], b[1], b[2]}}},
                      innen, shader);
}

std::string welt(const std::string& pinsel) { return "{\n\"classname\" \"worldspawn\"\n" + pinsel + "}\n"; }

void laden(Karte& k, const std::string& text) {
    k.leeren();
    (void)karteLesen(text, k);
    k.anmelden();
}

std::vector<std::shared_ptr<Pinsel>> pinselVon(const Entitaet& e) {
    std::vector<std::shared_ptr<Pinsel>> v;
    for (const auto& p : e.kinder()) {
        if (p->art() == Primitiv::Art::Pinsel) { v.push_back(std::static_pointer_cast<Pinsel>(p)); }
    }
    return v;
}
std::vector<std::shared_ptr<Patch>> patchesVon(const Entitaet& e) {
    std::vector<std::shared_ptr<Patch>> v;
    for (const auto& p : e.kinder()) {
        if (p->art() == Primitiv::Art::Patch) { v.push_back(std::static_pointer_cast<Patch>(p)); }
    }
    return v;
}
std::vector<std::shared_ptr<Entitaet>> klasse(const Karte& k, const std::string& name) {
    std::vector<std::shared_ptr<Entitaet>> v;
    for (const auto& e : k.entitaeten()) {
        if (e->klassenname() == name) { v.push_back(e); }
    }
    return v;
}
void alleWaehlen(Karte& k, const std::shared_ptr<Entitaet>& e) {
    for (const auto& p : e->kinder()) { k.auswahl.waehlen(e, p); }
}
std::array<float, 6> grenzen(const Karte& k, const Pinsel& p) {
    const Kasten b = p.grenzen(k.kontext);
    return {b.origin[0] - b.extents[0], b.origin[1] - b.extents[1], b.origin[2] - b.extents[2],
            b.origin[0] + b.extents[0], b.origin[1] + b.extents[1], b.origin[2] + b.extents[2]};
}
bool nah(float a, float b, float eps = 0.01F) { return std::fabs(a - b) <= eps; }
bool kastenGleich(const std::array<float, 6>& a, const std::array<float, 6>& b, float eps = 0.01F) {
    for (std::size_t i = 0; i < 6; ++i) {
        if (!nah(a[i], b[i], eps)) { return false; }
    }
    return true;
}
std::size_t flaechenMit(const Pinsel& p, const std::string& shader) {
    std::size_t n = 0;
    for (const auto& f : p.flaechen()) { n += f->shader() == shader ? 1 : 0; }
    return n;
}
bool hatFehler(const bt::Meldungen& m, const std::string& text) {
    return std::any_of(m.begin(), m.end(), [&](const bt::Meldung& x) { return x.art == 2 && x.text.find(text) != std::string::npos; });
}
bool hatAusgabe(const bt::Meldungen& m, const std::string& text) {
    return std::any_of(m.begin(), m.end(), [&](const bt::Meldung& x) { return x.art == 0 && x.text == text; });
}

bt::Umgebung umgebung() {
    bt::Umgebung u;
    u.aktuelleTextur = "textures/test/neu";
    u.zufall = [] { return 4711; };
    u.zufallStart = [](unsigned) {};
    u.zeit = [] { return 0LL; };
    u.raster = 8.0F;
    u.texturGroesse = [](const std::string& s) { return s == "textures/test/tuer" || s == "textures/test/rahmen" ? std::pair<int, int>{128, 256} : std::pair<int, int>{0, 0}; };
    return u;
}

// --- Listen, Intersect, Find Duplicates ------------------------------------------------

void listen() {
    const auto l = bt::listeLesen("textures/common/caulk\r\ntextures/common/nodraw\n\n");
    pruefe(l.size() == 2 && l[0] == "textures/common/caulk" && l[1] == "textures/common/nodraw", "LoadExclusionList: ein Wort je Zeile (fscanf %s)");
}

void intersect() {
    Karte k;
    // A und B ueberschneiden sich, C und D sind gleich, E ausgeschlossen (caulk) ueber F, G allein
    laden(k, welt(quader({0, 0, 0}, {64, 64, 64}) + quader({32, 32, 32}, {96, 96, 96}) + quader({200, 0, 0}, {264, 64, 64}) +
                  quader({200, 0, 0}, {264, 64, 64}) + quader({400, 0, 0}, {464, 64, 64}, "common/caulk") + quader({432, 0, 0}, {496, 64, 64}) +
                  quader({600, 0, 0}, {664, 64, 64})));
    const auto w = k.welt();
    const auto p = pinselVon(*w);
    pruefe(p.size() == 7, "Intersect: 7 Pinsel geladen");
    const bt::Umgebung u = [] {
        bt::Umgebung x = umgebung();
        x.ausschluss = {"common/caulk"};
        return x;
    }();
    bt::IntersectRS rs;
    bt::Meldungen m = bt::intersect(k, rs, u);
    pruefe(k.auswahl.anzahl() == 4 && k.auswahl.gewaehlt(w.get(), p[0].get()) && k.auswahl.gewaehlt(w.get(), p[1].get()) &&
               k.auswahl.gewaehlt(w.get(), p[2].get()) && k.auswahl.gewaehlt(w.get(), p[3].get()),
           "Intersect (ganze Karte): A, B, C, D gewaehlt - E ausgeschlossen, F und G allein");
    pruefe(hatAusgabe(m, "bobToolz Intersect: 4 intersecting brushes found.\n"), "Intersect: Meldung mit der Zahl");
    rs.nurDuplikate = true;
    m = bt::intersect(k, rs, u);
    pruefe(k.auswahl.anzahl() == 2 && k.auswahl.gewaehlt(w.get(), p[2].get()) && k.auswahl.gewaehlt(w.get(), p[3].get()),
           "Intersect, nur Duplikate: C und D");
    rs = bt::IntersectRS{};
    rs.ganzeKarte = false;
    k.auswahl.leeren();
    k.auswahl.waehlen(w, p[6]);
    m = bt::intersect(k, rs, u);
    pruefe(hatFehler(m, "Invalid number of brushes selected, choose at least 2."), "Intersect (Auswahl) mit einem Pinsel: Fehler");
    k.auswahl.waehlen(w, p[0]);
    k.auswahl.waehlen(w, p[1]);
    m = bt::intersect(k, rs, u);
    pruefe(k.auswahl.anzahl() == 2 && !k.auswahl.gewaehlt(w.get(), p[6].get()), "Intersect (Auswahl): nur A und B bleiben gewaehlt");
    // Find Duplicates: das zweite Exemplar wird gewaehlt
    k.auswahl.leeren();
    m = bt::findDuplicates(k);
    pruefe(k.auswahl.anzahl() == 1 && k.auswahl.gewaehlt(w.get(), p[3].get()), "Find Duplicates: D (das zweite Exemplar) gewaehlt");
    pruefe(hatAusgabe(m, "bobToolz Find Duplicates: 1 duplicate brushes found.\n"), "Find Duplicates: Meldung");
}

// --- Polygon Builder ---------------------------------------------------------------------

void polygone() {
    Karte k;
    const bt::Umgebung u = umgebung();
    laden(k, welt(quader({0, 0, 0}, {128, 128, 64})));
    auto w = k.welt();
    k.auswahl.waehlen(w, w->kinder().front());
    bt::PolygonRS rs;
    rs.seiten = 6;
    bt::Meldungen m = bt::polygonBuilder(k, rs, u);
    auto p = pinselVon(*w);
    pruefe(m.empty() && p.size() == 1 && p[0]->anzahl() == 8, "Polygon Builder: ein Prisma mit 6 Seiten + Boden + Deckel");
    pruefe(p.size() == 1 && flaechenMit(*p[0], "textures/test/neu") == 6 && flaechenMit(*p[0], "textures/common/caulk") == 2,
           "Polygon Builder: Seiten in der aktuellen Textur, Boden und Deckel caulk");
    if (p.size() == 1) {
        const auto g = grenzen(k, *p[0]);
        // sin( 2 pi i / 6 ) * 64: x von 64 - 55,43 bis 64 + 55,43, y voll
        pruefe(nah(g[0], 64.0F - 55.4256F) && nah(g[3], 64.0F + 55.4256F) && nah(g[1], 0) && nah(g[4], 128) && nah(g[2], 0) && nah(g[5], 64),
               "Polygon Builder: Ecken auf der Ellipse im Kasten");
    }
    pruefe(k.undo.undoName() == "bobToolz.polygons", "Polygon Builder: Undo-Schritt bobToolz.polygons");
    laden(k, welt(quader({0, 0, 0}, {128, 128, 64})));
    w = k.welt();
    k.auswahl.waehlen(w, w->kinder().front());
    rs.invers = true;
    m = bt::polygonBuilder(k, rs, u);
    pruefe(pinselVon(*w).size() == 6, "Polygon Builder invers: 6 Pinsel um das Loch");
    laden(k, welt(quader({0, 0, 0}, {128, 128, 64})));
    w = k.welt();
    k.auswahl.waehlen(w, w->kinder().front());
    rs.invers = false;
    rs.rand = true;
    rs.randBreite = 16;
    m = bt::polygonBuilder(k, rs, u);
    pruefe(pinselVon(*w).size() == 6, "Polygon Builder mit Rand: 6 Pinsel im Ring");
    k.auswahl.leeren();
    m = bt::polygonPruefen(k);
    pruefe(hatFehler(m, "Invalid number of brushes selected, choose 1 only."), "Polygon Builder ohne Auswahl: Fehler");
}

// --- Stair Builder -----------------------------------------------------------------------

void treppen() {
    Karte k;
    bt::BuildStairsRS rs;
    rs.stufenHoehe = 16;
    rs.richtung = bt::MOVE_NORTH;
    rs.hauptTextur = "textures/test/tritt";
    rs.stirnTextur = "textures/test/stirn";
    laden(k, welt(quader({0, 0, 0}, {128, 128, 64})));
    auto w = k.welt();
    k.auswahl.waehlen(w, w->kinder().front());
    bt::Meldungen m = bt::stairBuilder(k, rs);
    auto p = pinselVon(*w);
    pruefe(m.empty() && p.size() == 4, "Stair Builder (Original, Nord): 64 / 16 = 4 Stufen");
    if (p.size() == 4) {
        // SetInitialStairPos / MoveBlock: jede Stufe 32 tief (128 / 4) und vom
        // Boden an 16 hoeher als die vorige (vMin[2] bleibt, vMax[2] += 16)
        bool ok = true;
        for (std::size_t i = 0; i < 4; ++i) {
            const auto g = grenzen(k, *p[i]);
            const float y0 = 32.0F * static_cast<float>(i);
            ok = ok && kastenGleich(g, {0, y0, 0, 128, y0 + 32.0F, 16.0F * static_cast<float>(i + 1)});
        }
        pruefe(ok, "Stair Builder: Stufe i von y = 32 i bis 32 (i + 1), vom Boden bis 16 (i + 1)");
        pruefe(flaechenMit(*p[0], "textures/test/tritt") == 1 && flaechenMit(*p[0], "textures/test/stirn") == 1,
               "Stair Builder: Tritt oben in der Haupttextur, Stirn nach Sueden");
    }
    pruefe(k.undo.undoName() == "bobToolz.buildStairs", "Stair Builder: Undo-Schritt");
    // Hoehe nicht teilbar: Fehler, Pinsel bleibt
    laden(k, welt(quader({0, 0, 0}, {128, 128, 60})));
    w = k.welt();
    k.auswahl.waehlen(w, w->kinder().front());
    m = bt::stairBuilder(k, rs);
    pruefe(hatFehler(m, "Height of block must be divisable by stair height.") && pinselVon(*w).size() == 1, "Stair Builder: 60 / 16 - Fehler, Pinsel bleibt");
    // Bob's Style: Keil + 4 Stufen-Keile
    laden(k, welt(quader({0, 0, 0}, {128, 128, 64})));
    w = k.welt();
    k.auswahl.waehlen(w, w->kinder().front());
    rs.stil = bt::STYLE_BOB;
    rs.richtung = bt::MOVE_EAST;
    m = bt::stairBuilder(k, rs);
    pruefe(pinselVon(*w).size() == 5, "Stair Builder (Bob's Style, Ost): Keil und 4 Stufen");
    // Corner: BuildCornerStairs
    laden(k, welt(quader({0, 0, 0}, {128, 128, 64})));
    w = k.welt();
    k.auswahl.waehlen(w, w->kinder().front());
    rs.stil = bt::STYLE_CORNER;
    m = bt::stairBuilder(k, rs);
    pruefe(pinselVon(*w).size() >= 4, "Stair Builder (Corner): Stufen um die Ecke (" + std::to_string(pinselVon(*w).size()) + " Pinsel)");
}

// --- Door Builder, Aperture Door, Pit-O-Matic --------------------------------------------

void tueren() {
    Karte k;
    const bt::Umgebung u = umgebung();
    // DIRECTION_NS: xy = 0 - die Fluegel teilen sich die Breite in x
    laden(k, welt(quader({0, 0, 0}, {128, 16, 128})));
    auto w = k.welt();
    k.auswahl.waehlen(w, w->kinder().front());
    bt::DoorRS rs;
    rs.hauptTextur = "test/tuer";
    rs.randTextur = "test/rahmen";
    rs.ausrichtung = bt::DIRECTION_NS;
    bt::Meldungen m = bt::doorBuilder(k, rs, u);
    const auto tueren = klasse(k, "func_door");
    pruefe(m.empty() && tueren.size() == 2 && pinselVon(*w).empty(), "Door Builder: zwei func_door statt des Pinsels");
    if (tueren.size() == 2) {
        pruefe(tueren[0]->wert("angle") == "180" && tueren[1]->wert("angle") == "360", "Door Builder (N/S): angle 180 und 360");
        pruefe(tueren[0]->wert("team") == "t4711" && tueren[1]->wert("team") == "t4711", "Door Builder: gemeinsames team t<rand()>");
        const auto a = pinselVon(*tueren[0]);
        const auto b = pinselVon(*tueren[1]);
        for (const auto& q : {a, b}) {
            for (const auto& x : q) {
                const auto g = grenzen(k, *x);
            }
        }
        pruefe(a.size() == 1 && b.size() == 1 && kastenGleich(grenzen(k, *a[0]), {0, 0, 0, 64, 16, 128}) &&
                   kastenGleich(grenzen(k, *b[0]), {64, 0, 0, 128, 16, 128}),
               "Door Builder: je Fluegel die halbe Breite");
        pruefe(a.size() == 1 && flaechenMit(*a[0], "textures/test/tuer") == 2 && flaechenMit(*a[0], "textures/test/rahmen") == 1,
               "Door Builder: zwei Seiten in der Tuertextur, die Mitte im Rahmen");
    }
    pruefe(k.undo.undoName() == "bobToolz.buildDoors", "Door Builder: Undo-Schritt");
    // ohne Auswahl
    k.auswahl.leeren();
    pruefe(hatFehler(bt::doorPruefen(k), "choose 1 only"), "Door Builder ohne Auswahl: Fehler");
    // Aperture Door
    laden(k, welt(quader({0, 0, 0}, {16, 128, 128})));
    w = k.welt();
    k.auswahl.waehlen(w, w->kinder().front());
    bt::ApertureDoorRS ap;
    ap.segmente = 8;
    ap.hauptTextur = "textures/test/tuer";
    ap.randTextur = "textures/test/rahmen";
    m = bt::apertureDoor(k, ap, u);
    const auto segmente = klasse(k, "func_door");
    pruefe(segmente.size() == 8, "Aperture Door: 8 Segmente als func_door (" + std::to_string(segmente.size()) + ")");
    pruefe(k.undo.undoName() == "bobToolz.buildApertureDoor", "Aperture Door: Undo-Schritt (shapes.cpp:648)");
    // Pit-O-Matic
    laden(k, welt(quader({0, 0, 0}, {128, 128, 256})));
    w = k.welt();
    k.auswahl.waehlen(w, w->kinder().front());
    m = bt::pitOMatic(k, u);
    pruefe(klasse(k, "trigger_multiple").size() == 2 && klasse(k, "trigger_hurt").size() == 1 && klasse(k, "target_speaker").size() == 1 &&
               klasse(k, "target_remove_powerups").size() == 1,
           "Pit-O-Matic: 2 trigger_multiple, trigger_hurt, target_speaker, target_remove_powerups");
    const auto pw = pinselVon(*w);
    pruefe(pw.size() == 1 && flaechenMit(*pw[0], "textures/common/nodrop") == 6 && kastenGleich(grenzen(k, *pw[0]), {0, 0, 0, 128, 128, 64}),
           "Pit-O-Matic: nodrop-Pinsel unten (64 hoch) statt des Pinsels");
    const auto sp = klasse(k, "target_speaker");
    pruefe(sp.size() == 1 && sp[0]->wert("targetname") == "t9711_1" && sp[0]->wert("noise") == "*falling1.wav" &&
               sp[0]->wert("origin") == "64.000000 64.000000 272.000000",
           "Pit-O-Matic: Sprecher t<rand % 10000 + 5000>_1 16 ueber der Mitte");
    laden(k, welt(quader({0, 0, 0}, {128, 128, 128})));
    w = k.welt();
    k.auswahl.waehlen(w, w->kinder().front());
    m = bt::pitOMatic(k, u);
    pruefe(hatFehler(m, "Failed to make Pit, try making the brush bigger.") && pinselVon(*w).size() == 1, "Pit-O-Matic unter 196 hoch: Fehler");
}

// --- Texturen, Brush Cleanup, Explode Entity, Make Chain ----------------------------------

void texturen() {
    Karte k;
    laden(k, welt(quader({0, 0, 0}, {64, 64, 64}, "test/alt") + quader({100, 0, 0}, {164, 64, 64}, "test/alt")));
    auto w = k.welt();
    auto p = pinselVon(*w);
    k.auswahl.waehlen(w, p[0]);
    bt::ResetTextureRS rs;
    rs.nameErsetzen = true;
    rs.alterName = "textures/test/alt";
    rs.neuerName = "textures/test/neu";
    rs.skalaSetzen[0] = true;
    rs.skala[0] = 0.25F;
    bt::Meldungen m = bt::resetTextures(k, rs, false);
    // der geaenderte Pinsel wird geloescht und neu gebaut: er steht danach hinten
    p = pinselVon(*w);
    pruefe(p.size() == 2 && flaechenMit(*p[1], "textures/test/neu") == 6 && flaechenMit(*p[0], "textures/test/alt") == 6 &&
               nah(grenzen(k, *p[1])[0], 0),
           "Reset Textures (Auswahl): nur der gewaehlte Pinsel bekommt den neuen Namen (neu gebaut, hinten)");
    pruefe(nah(p[1]->flaechen()[0]->projektion().texdef.scale[0], 0.25F) && nah(p[1]->flaechen()[0]->projektion().texdef.scale[1], 0.5F),
           "Reset Textures: Skala S gesetzt, T bleibt");
    pruefe(k.undo.undoName() == "bobToolz.resetTextures", "Reset Textures: Undo-Schritt");
    m = bt::resetTextures(k, rs, true);
    p = pinselVon(*w);
    pruefe(flaechenMit(*p[0], "textures/test/neu") == 6 && flaechenMit(*p[1], "textures/test/neu") == 6, "Reset Textures (alle): auch der andere Pinsel");
    // Caulk Selection
    k.auswahl.leeren();
    k.auswahl.waehlen(w, p[1]);
    m = bt::caulkSelection(k);
    p = pinselVon(*w);
    pruefe(flaechenMit(*p[1], "textures/common/caulk") == 6 && flaechenMit(*p[0], "textures/common/caulk") == 0, "Caulk Selection: nur der gewaehlte");
    pruefe(k.undo.undoName() == "bobToolz.caulkSelection", "Caulk Selection: Undo-Schritt");
    // Brush Cleanup: zaehlt nur und waehlt alles ab
    laden(k, welt(quader({0, 0, 0}, {64, 64, 64}, "test/alt") + quader({100, 0, 0}, {164, 64, 64}, "test/alt")));
    w = k.welt();
    k.auswahl.waehlen(w, w->kinder()[1]);
    m = bt::brushCleanup(k);
    pruefe(hatAusgabe(m, "bobToolz FixBrushes: 0 invalid/duplicate planes removed.\n"), "Brush Cleanup: gueltige Pinsel - 0");
    // DMap::LoadAll: GlobalSelectionSystem().setSelectedAll( false ) (DMap.cpp:107)
    pruefe(k.auswahl.anzahl() == 0, "Brush Cleanup: nichts mehr gewaehlt (DMap::LoadAll)");
    k.auswahl.waehlen(w, w->kinder()[1]);
    m = bt::caulkSelection(k);
    p = pinselVon(*w);
    pruefe(flaechenMit(*p[1], "textures/common/caulk") == 6, "Brush Cleanup, dann Caulk Selection: der gewaehlte caulk");
}

void entitaeten() {
    Karte k;
    laden(k, welt("") + "{\n\"classname\" \"func_group\"\n\"_note\" \"x\"\n" + quader({0, 0, 0}, {16, 16, 16}) + quader({32, 0, 0}, {48, 16, 16}) +
                 quader({64, 0, 0}, {80, 16, 16}) + "}\n");
    auto g = klasse(k, "func_group");
    k.auswahl.waehlen(g[0], g[0]->kinder().front());
    bt::Meldungen m = bt::explodeEntity(k);
    g = klasse(k, "func_group");
    pruefe(m.empty() && g.size() == 3, "Explode Entity: drei func_group mit je einem Pinsel");
    pruefe(g.size() == 3 && g[2]->wert("_note") == "x" && pinselVon(*g[2]).size() == 1, "Explode Entity: Schluessel uebernommen");
    // ein Pinsel von worldspawn: path.pop() -> worldspawn (eigene Karte: oben ist
    // worldspawn ohne Pinsel ohne Klassen des Spiels eine Punktklasse geworden)
    {
        Karte k2;
        laden(k2, welt(quader({0, 0, 0}, {16, 16, 16})));
        k2.auswahl.waehlen(k2.welt(), k2.welt()->kinder().front());
        m = bt::explodeEntity(k2);
        pruefe(hatFehler(m, "can't explode worldspawn"), "Explode Entity auf worldspawn: Fehler");
    }
    // Make Chain
    bt::MakeChainRS mc;
    mc.glieder = 3;
    mc.name = "kette";
    m = bt::makeChain(k, mc);
    const auto haupt = klasse(k, "info_train_spline_main");
    const auto ctl = klasse(k, "info_train_spline_control");
    pruefe(haupt.size() == 3 && ctl.size() == 2, "Make Chain: 3 info_train_spline_main, 2 info_train_spline_control");
    if (haupt.size() == 3 && ctl.size() == 2) {
        pruefe(haupt[0]->wert("targetname") == "kette_pt0" && haupt[0]->wert("target") == "kette_pt1" && haupt[0]->wert("control") == "kette_ctl0" &&
                   haupt[1]->wert("origin") == "0 64 0" && !haupt[2]->hat("target"),
               "Make Chain: Namen, Ziele, Ursprung 0 64 i 0");
        pruefe(ctl[1]->wert("targetname") == "kette_ctl1" && ctl[1]->wert("origin") == "0 96 0", "Make Chain: Kontrollpunkte dazwischen");
    }
    // Plot Splines aus der Kette
    bt::Splines s;
    m = bt::splinesBauen(k, s);
    pruefe(s.anzeigen && s.linien.size() == 2, "Plot Splines: zwei Abschnitte");
    if (s.linien.size() == 2) {
        const auto& l = s.linien[0];
        // m_vertexList: vorn das Ziel, dann tension 0.99 .. 0 (hinten der Start)
        pruefe(l.size() == 102 && l.front() == Vektor3f(0, 64, 0) && l.back() == Vektor3f(0, 0, 0), "Plot Splines: 101 Punkte + Ziel, vom Ziel zum Start");
    }
    mc.glieder = 1002;
    m = bt::makeChain(k, mc);
    pruefe(hatFehler(m, "to many Elemets, limited to 1000."), "Make Chain ueber 1001: Fehler");
}

// --- Patches ---------------------------------------------------------------------------------

std::shared_ptr<Patch> flachPatch(float x0, float y0, float groesse, std::size_t n) {
    auto p = std::make_shared<Patch>();
    p->setzeGroesse(n, n);
    for (std::size_t r = 0; r < n; ++r) {
        for (std::size_t c = 0; c < n; ++c) {
            PatchPunkt& q = p->punkt(r, c);
            const float s = groesse / static_cast<float>(n - 1);
            q.vertex = Vektor3f(x0 + (static_cast<float>(c) * s), y0 + (static_cast<float>(r) * s), 0);
            q.texcoord = Vektor2f(static_cast<float>(c), static_cast<float>(r));
        }
    }
    p->setzeShader("textures/test/patch");
    p->punkteGeaendert();
    return p;
}

void patches() {
    Karte k;
    auto w = k.weltHolenOderAnlegen();
    auto a = flachPatch(0, 0, 64, 3);
    auto b = flachPatch(64, 0, 64, 3);
    w->kindEinfuegen(a);
    w->kindEinfuegen(b);
    k.auswahl.waehlen(w, a);
    k.auswahl.waehlen(w, b);
    bt::Meldungen m = bt::mergePatches(k);
    auto ps = patchesVon(*w);
    pruefe(m.empty() && ps.size() == 1 && ps[0]->punkte().size() == 15, "Merge Patches: zwei 3x3 an einer Kante -> ein Patch mit 5x3");
    if (ps.size() == 1) {
        float xmax = 0;
        for (const auto& q : ps[0]->punkte()) { xmax = std::max(xmax, q.vertex[0]); }
        pruefe(nah(xmax, 128) && ps[0]->shader() == "textures/test/patch", "Merge Patches: reicht bis x = 128, Shader bleibt");
    }
    pruefe(k.undo.undoName() == "bobToolz.mergePatches", "Merge Patches: Undo-Schritt");
    // nicht verbunden
    auto c = flachPatch(500, 0, 64, 3);
    auto d = flachPatch(700, 0, 64, 3);
    w->kindEinfuegen(c);
    w->kindEinfuegen(d);
    k.auswahl.leeren();
    k.auswahl.waehlen(w, c);
    k.auswahl.waehlen(w, d);
    m = bt::mergePatches(k);
    pruefe(hatFehler(m, "The selected patches are not mergable."), "Merge Patches ohne gemeinsame Kante: Fehler");
    // Split: 5x5 -> 4 Patches 3x3, Spalten -> 2, Zeilen -> 2
    for (int art = 0; art < 3; ++art) {
        Karte k2;
        auto w2 = k2.weltHolenOderAnlegen();
        auto e = flachPatch(0, 0, 128, 5);
        w2->kindEinfuegen(e);
        k2.auswahl.waehlen(w2, e);
        m = bt::splitPatch(k2, art);
        ps = patchesVon(*w2);
        static const char* const namen[3] = {"Split patch", "Split patch columns", "Split patch rows"};
        static const char* const undo[3] = {"bobToolz.splitPatch", "bobToolz.splitPatchCols", "bobToolz.splitPatchRows"};
        const std::size_t soll = art == 0 ? 4 : 2;
        bool form = ps.size() == soll;
        for (const auto& q : ps) { form = form && q->punkte().size() == (art == 0 ? 9U : 15U); }
        pruefe(form, std::string(namen[art]) + ": " + std::to_string(soll) + " Teile (" + std::to_string(ps.size()) + ")");
        pruefe(k2.undo.undoName() == undo[art], std::string(namen[art]) + ": Undo-Schritt");
    }
}

// --- Flip Terrain ----------------------------------------------------------------------------

void terrain() {
    // Viereck 0..64 mit den Hoehen A(0,0)=0, B(64,0)=16, C(64,64)=0, D(0,64)=16,
    // geteilt entlang A-C; nach Flip Terrain entlang B-D.
    const V a{0, 0, 0};
    const V b{64, 0, 16};
    const V c{64, 64, 0};
    const V d{0, 64, 16};
    const std::string p1 = pinselText({{V{0, 0, -16}, V{64, 0, -16}, V{0, 64, -16}},
                                       {V{0, 0, 0}, V{64, 0, 0}, V{0, 0, 16}},
                                       {V{64, 0, 0}, V{64, 64, 0}, V{64, 0, 16}},
                                       {V{0, 0, 0}, V{64, 64, 0}, V{0, 0, 16}},
                                       {a, b, c}},
                                      V{40, 20, -8}, "test/erde");
    const std::string p2 = pinselText({{V{0, 0, -16}, V{64, 0, -16}, V{0, 64, -16}},
                                       {V{0, 64, 0}, V{64, 64, 0}, V{0, 64, 16}},
                                       {V{0, 0, 0}, V{0, 64, 0}, V{0, 0, 16}},
                                       {V{0, 0, 0}, V{64, 64, 0}, V{0, 0, 16}},
                                       {a, c, d}},
                                      V{20, 40, -8}, "test/erde");
    Karte k;
    laden(k, welt(p1 + p2));
    auto w = k.welt();
    auto p = pinselVon(*w);
    pruefe(p.size() == 2 && p[0]->anzahl() == 5 && p[1]->anzahl() == 5, "Flip Terrain: zwei dreieckige Saeulen geladen");
    k.auswahl.waehlen(w, p[0]);
    k.auswahl.waehlen(w, p[1]);
    bt::Meldungen m = bt::flipTerrain(k);
    p = pinselVon(*w);
    pruefe(m.empty() && p.size() == 2, "Flip Terrain: zwei neue Pinsel");
    bool diagonaleBD = p.size() == 2;
    for (const auto& q : p) {
        (void)grenzen(k, *q);   // die Wicklungen (bRep)
        bool erde = false;
        for (const auto& f : q->flaechen()) {
            if (f->shader() != "textures/common/terrain") { continue; }
            erde = true;
            bool hatB = false;
            bool hatD = false;
            for (std::size_t i = 0; i < f->wicklung().numpoints; ++i) {
                const auto& v = f->wicklung().points[i].vertex;
                hatB = hatB || (std::fabs(v[0] - 64) < 0.01 && std::fabs(v[1]) < 0.01 && std::fabs(v[2] - 16) < 0.01);
                hatD = hatD || (std::fabs(v[0]) < 0.01 && std::fabs(v[1] - 64) < 0.01 && std::fabs(v[2] - 16) < 0.01);
            }
            diagonaleBD = diagonaleBD && hatB && hatD && f->wicklung().numpoints == 3;
        }
        diagonaleBD = diagonaleBD && erde;
    }
    pruefe(diagonaleBD, "Flip Terrain: die Oberseiten (common/terrain) teilen jetzt die Diagonale B-D");
    pruefe(k.undo.undoName() == "bobToolz.flipTerrain", "Flip Terrain: Undo-Schritt");
}

// --- Path Plotter --------------------------------------------------------------------------

void bahn() {
    Karte k;
    laden(k, welt("") + "{\n\"classname\" \"trigger_push\"\n\"target\" \"oben\"\n" + quader({-16, -16, 0}, {16, 16, 16}) + "}\n" +
                 "{\n\"classname\" \"target_position\"\n\"targetname\" \"oben\"\n\"origin\" \"256 0 200\"\n}\n");
    const auto t = klasse(k, "trigger_push");
    k.auswahl.waehlen(t[0], nullptr);
    bt::PathPlotterRS rs;
    bt::Bahn b;
    bt::Meldungen m = bt::pathPlotter(k, rs, b);
    pruefe(b.aktiv && b.punkte.size() == 50 && hatAusgabe(m, "Initialization of Path Plotter succeeded.\n"), "Path Plotter: 50 Punkte");
    if (b.punkte.size() == 50) {
        // Start: Mitte des Triggers (0 0 8); Scheitel 192 hoeher: v_z = sqrt( 1600 * 192 ), t = v_z / 800
        const float vz = std::sqrt(1600.0F * 192.0F);
        const float tf = vz / 800.0F;
        const float dt = 3.0F * tf / 50.0F;
        const float t10 = dt * 10.0F;
        pruefe(b.punkte[0] == Vektor3f(0, 0, 8), "Path Plotter: erster Punkt in der Mitte des trigger_push");
        pruefe(nah(b.punkte[10][0], 256.0F / tf * t10, 0.05F) && nah(b.punkte[10][2], 8.0F + (vz * t10) - (400.0F * t10 * t10), 0.05F),
               "Path Plotter: Wurfparabel mit g = -800 (CalculateTrajectory)");
    }
    // Ziel unter dem Start: keine Bahn
    k.auswahl.leeren();
    pruefe(hatFehler(bt::pathPlotter(k, rs, b), "Invalid number of entities selected"), "Path Plotter ohne Auswahl: Fehler");
}

// --- Tree Planter, Drop Entity -------------------------------------------------------------

void baeume() {
    Karte k;
    laden(k, welt(quader({-256, -256, -16}, {256, 256, 0})) + "{\n\"classname\" \"misc_model\"\n\"origin\" \"40 40 300\"\n}\n");
    auto w = k.welt();
    k.auswahl.waehlen(w, w->kinder().front());
    bt::TreePlanter tp;
    bt::Meldungen m = bt::treePlanterStarten(k, "{\n entity misc_model\n model \"models/baum.md3\"\n offset 8\n // Kommentar\n}\n", tp);
    pruefe(m.empty() && tp.aktiv && tp.klasse == "misc_model" && tp.modelle.size() == 1 && tp.versatz == 8, "Tree Planter: tp_ent.txt gelesen");
    const bt::Umgebung u = umgebung();
    const std::size_t vorher = klasse(k, "misc_model").size();
    const bool gepflanzt = bt::treePlanterKlick(k, tp, Vektor3f(13, -11, 0), u, m);
    const auto mm = klasse(k, "misc_model");
    pruefe(gepflanzt && mm.size() == vorher + 1, "Tree Planter: Klick pflanzt eine Entitaet");
    if (mm.size() == vorher + 1) {
        pruefe(mm.back()->wert("origin") == "16 -8 8" && mm.back()->wert("model") == "models/baum.md3",
               "Tree Planter: am Raster (8) auf der Oberseite + offset, Modell aus der Liste");
    }
    pruefe(!bt::treePlanterKlick(k, tp, Vektor3f(1000, 1000, 0), u, m), "Tree Planter: neben dem Boden nichts");
    // Drop Entity: die gewaehlte Entitaet faellt auf den Boden
    k.auswahl.leeren();
    k.auswahl.waehlen(mm.front(), nullptr);
    m = bt::dropEntities(k, &tp);
    pruefe(mm.front()->wert("origin") == "40 40 0", "Drop Entity: auf die Oberseite des Bodens");
    m = bt::dropEntities(k, nullptr);
    pruefe(hatFehler(m, "bobToolz::TreePlanter must be active"), "Drop Entity ohne Tree Planter: Fehler");
}

// --- Vis Viewer --------------------------------------------------------------------------------

void vis(const std::string& ordner) {
    bt::VisAnsicht v;
    bt::Meldungen m = bt::visAnalyse(ordner + "/raum.nrc.bsp", Vektor3f(-384, 128, 24), v);
    pruefe(v.offen && !v.liste.empty(), "Vis Viewer: Blaetter der .bsp (" + std::to_string(v.liste.size()) + ")");
    pruefe(v.gewaehlt >= 0 && !v.flaechen.empty(), "Vis Viewer: das Blatt am Ursprung mit sichtbaren Flaechen");
    bt::VisAnsicht leer;
    m = bt::visAnalyse(ordner + "/raum.nrc.bsp", Vektor3f(5000, 5000, 5000), leer);
    pruefe(hatFehler(m, "Point of interest is in the void!"), "Vis Viewer: Punkt ausserhalb - in the void");
    bt::VisAnsicht neu;
    m = bt::visAnalyse(ordner + "/fehlt.bsp", Vektor3f(0, 0, 0), neu);
    pruefe(!m.empty() && m[0].art == 2, "Vis Viewer: fehlende .bsp - Fehler");
}

}  // namespace

int main(int argc, char** argv) {
    const std::string ordner = argc > 1 ? argv[1] : "tests/karte/q3map2";
    listen();
    intersect();
    polygone();
    treppen();
    tueren();
    texturen();
    entitaeten();
    patches();
    terrain();
    bahn();
    baeume();
    vis(ordner);
    std::printf("kartebobtoolz: %d Pruefungen, %d Fehlschlaege\n", g_pruefungen, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
