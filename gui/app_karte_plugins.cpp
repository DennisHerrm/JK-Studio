// app_karte_plugins.cpp - Karten-Editor, Stufe 4 Punkt 3: die Oberflaeche des
// Plugins bobToolz (die Befehle selbst: src/karte/bobtoolz*.cpp).
//
// Portiert aus NetRadiant-Custom 20260114, contrib/bobtoolz:
//   bobToolz-GTK.cpp:107-176 (QERPlug_Dispatch) und :184-279 (Werkzeugleiste),
//   funchandlers-GTK.cpp (Do*: was vor und nach den Dialogen geschieht),
//   dialogs/dialogs-gtk.cpp (DoIntersectBox, DoPolygonBox, DoBuildStairsBox,
//   DoDoorsBox, DoApertureDoorsBox, DoPathPlotterBox, DoResetTextureBox,
//   DoMakeChainBox), DBobView.cpp / DTrainDrawer.cpp / DVisDrawer.cpp
//   (Zeichnen, Dialog "BSP leafs"), DTreePlanter.h/.cpp (mouseDown ueber
//   XYWindowMouseDown_connect), lists.cpp (LoadLists, LoadListStore).
//
// Masse: die Dialoge sind QFormLayout / QVBoxLayout / QHBoxLayout mit
// SetFixedSize (Rand 11, Abstand 6, Bausteine siehe karte_qtdialog.h). An
// den Fotos (Massstab 2) gemessen: Intersect 192 x 157 (OK bei 15/124,
// Cancel bei 101/124), Make Chain 361 x 100, Path Plotter 367 x 180 (drei
// Knoepfe je 80 rechtsbuendig bei 104 / 190 / 276, y 152). Ohne Foto nach
// denselben Regeln: Polygon Builder, Stair Builder, Door Builder, Aperture
// Door Builder, Texture Reset, BSP leafs.
//
//   BobToolz plugin for GtkRadiant
//   Copyright (C) 2001 Gordon Biggans
//
//   Urspruenglich unter der GNU Lesser General Public License 2.1 (oder
//   spaeter); nach deren Abschnitt 3 hier unter der GNU General Public
//   License, Version 2: This program is free software; you can redistribute
//   it and/or modify it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; version 2 of the License. This
//   program is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//   FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
//   details (LICENSE).
#include "karteneditor.h"

#include "app_internal.h"
#include "karte_gpu.h"
#include "karte_qtdialog.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace bhed::gui::karteneditor {

namespace {

namespace bt = karte::bobtoolz;
using i18n::Str;

Editor& e() { return editor(); }

// --- Hilfen ------------------------------------------------------------------------

// Die Meldungen der Befehle in die Konsole (globalOutput/Warning/ErrorStream)
void melden(const bt::Meldungen& m) {
    for (const bt::Meldung& x : m) {
        std::string t = x.text;
        while (!t.empty() && t.back() == '\n') { t.pop_back(); }
        if (x.art == 2) {
            fehler(t);
        } else if (x.art == 1) {
            warnung(t);
        } else {
            ausgabe(t);
        }
    }
}

// Nach jedem Befehl: die Ansichten neu, die Auswahl neu gelesen
void nachBefehl() {
    ++e().sichtStand;
    auswahlGeaendert();
}

// GetFilename( buffer, "bt/..." ): neben dem Programm data/karte/bt
std::string btPfad(const char* name) { return directoryOf(g_app->dataDir) + "/karte/bt/" + name; }

bool btLesen(const char* name, std::string& text) {
    const std::string p = btPfad(name);
    std::ifstream f(std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(p.data()), p.size())), std::ios::binary);
    if (!f) { return false; }
    std::ostringstream s;
    s << f.rdbuf();
    text = s.str();
    return true;
}

// LoadLists (funchandlers-GTK.cpp:60-69) mit LoadExclusionList (lists.cpp:24-46)
void listenLaden() {
    BobToolzZustand& b = e().bob;
    if (b.listenGeladen) { return; }
    std::string t;
    bool ok = true;
    for (const auto& [name, liste] : {std::pair<const char*, std::vector<std::string>*>{"bt-el1.txt", &b.ausschluss}, {"bt-el2.txt", &b.ausschlussFlaeche}}) {
        if (btLesen(name, t)) {
            *liste = bt::listeLesen(t);
        } else {
            fehler("Failed To Load Exclusion List: " + btPfad(name));
            ok = false;
        }
    }
    b.listenGeladen = ok;
}

// LoadListStore (lists.cpp:48-70)
std::vector<std::string> listeLaden(const char* name) {
    std::string t;
    if (!btLesen(name, t)) {
        fehler(std::string("Failed To Load GList: ") + btPfad(name));
        return {};
    }
    return bt::listeLesen(t);
}

// rand() / srand( time( nullptr ) ) der C-Laufzeit (MinGW und MSVC: dieselbe
// lineare Folge); im Selbsttest die Zeit 0, damit die Namen fest sind.
bt::Umgebung umgebung() {
    Editor& ed = e();
    bt::Umgebung u;
    u.aktuelleTextur = ed.texBro.shader;   // GetCurrentTexture
    u.ausschluss = ed.bob.ausschluss;
    u.ausschlussFlaeche = ed.bob.ausschlussFlaeche;
    u.kamera = ed.kamera.ursprung;
    u.raster = ed.raster.groesse();
    u.zufall = [] { return std::rand(); };
    u.zufallStart = [](unsigned s) { std::srand(s); };
    u.zeit = [] { return std::getenv("BHED_EDITORTEST") != nullptr ? 0LL : static_cast<long long>(std::time(nullptr)); };
    u.texturGroesse = [](const std::string& s) {
        const auto g = e().karte.kontext.groesse(s);
        return std::pair<int, int>{static_cast<int>(g.first), static_cast<int>(g.second)};
    };
    return u;
}

// TexturePath: "textures/" vorn (get) bzw. ohne (get_short)
std::string mitPraefix(const std::string& t) {
    const std::string p = "textures/";
    return t.size() >= p.size() && std::equal(p.begin(), p.end(), t.begin(), [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == b; })
               ? t
               : p + t;
}
std::string ohnePraefix(const std::string& t) {
    const std::string p = "textures/";
    return t.size() >= p.size() && std::equal(p.begin(), p.end(), t.begin(), [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == b; })
               ? t.substr(p.size())
               : t;
}

void dialogOeffnen(int art) {
    BobToolzZustand& b = e().bob;
    b.dialog = art;
    b.oeffnen = true;
}

// --- die Befehle (QERPlug_Dispatch) ----------------------------------------------------

void doIntersect() {   // DoIntersect: erst der Dialog
    e().bob.intersect = bt::IntersectRS{};
    dialogOeffnen(1);
}

void doPolygons() {   // DoPolygons: Auswahl pruefen, dann DoPolygonBox
    Editor& ed = e();
    const bt::Meldungen m = bt::polygonPruefen(ed.karte);
    if (!m.empty()) {
        melden(m);
        return;
    }
    ed.bob.polygon = bt::PolygonRS{};
    dialogOeffnen(2);
}

void doStairs() {   // DoBuildStairs: rs.mainTexture = GetCurrentTexture()
    Editor& ed = e();
    const bt::Meldungen m = bt::stairPruefen(ed.karte);
    if (!m.empty()) {
        melden(m);
        return;
    }
    ed.bob.treppe = bt::BuildStairsRS{};
    ed.bob.treppe.hauptTextur = ed.texBro.shader;
    dialogOeffnen(3);
}

void doDoors() {   // DoBuildDoors: mainTexture ohne "textures/"
    Editor& ed = e();
    const bt::Meldungen m = bt::doorPruefen(ed.karte);
    if (!m.empty()) {
        melden(m);
        return;
    }
    ed.bob.tuer = bt::DoorRS{};
    ed.bob.tuer.hauptTextur = ohnePraefix(ed.texBro.shader);
    ed.bob.tuerTexturen = listeLaden("door-tex.txt");
    ed.bob.rahmenTexturen = listeLaden("door-tex-trim.txt");
    dialogOeffnen(4);
}

void doAperture() {   // DoBuildApertureDoors: static ApertureDoorRS rs
    Editor& ed = e();
    const bt::Meldungen m = bt::aperturePruefen(ed.karte);
    if (!m.empty()) {
        melden(m);
        return;
    }
    ed.bob.tuerTexturen = listeLaden("door-tex.txt");
    ed.bob.rahmenTexturen = listeLaden("door-tex-trim.txt");
    dialogOeffnen(5);
}

void doPathPlotter() {   // DoPathPlotter: erst der Dialog
    e().bob.bahnWerte = bt::PathPlotterRS{};
    dialogOeffnen(6);
}

void doResetTextures() {   // DoResetTextures: die Gruppen aus, die Zahlen wie neu gebaut
    bt::ResetTextureRS& rs = e().bob.textur;
    rs.nameErsetzen = false;
    rs.skalaSetzen[0] = rs.skalaSetzen[1] = false;
    rs.verschiebungSetzen[0] = rs.verschiebungSetzen[1] = false;
    rs.drehungSetzen = false;
    rs.skala[0] = rs.skala[1] = 0.5F;   // DoubleSpinBox( -1024, 1024, .5, 3, .25 )
    rs.verschiebung[0] = rs.verschiebung[1] = 0.0F;
    rs.drehung = 0;
    dialogOeffnen(7);
}

void doMakeChain() {   // DoMakeChain: SpinBox( 1, 1000 ) beginnt bei 1
    e().bob.kette = bt::MakeChainRS{};
    dialogOeffnen(8);
}

void doVisAnalyse() {   // DoVisAnalyse (funchandlers-GTK.cpp:604-624)
    Editor& ed = e();
    std::string karte = ed.name;
    const std::size_t punkt = karte.rfind('.');
    const std::size_t strich = karte.find_last_of("/\\");
    if (punkt != std::string::npos && (strich == std::string::npos || punkt > strich)) { karte.resize(punkt); }   // PathExtensionless
    const std::string bsp = karte + ".bsp";
    karte::Vektor3f ursprung = ed.kamera.ursprung;   // Camera_getOrigin
    karte::Kasten g;
    if (ed.karte.auswahl.anzahl() != 0 && auswahlGrenzen(g)) { ursprung = g.origin; }   // getBoundsSelected().origin
    const bool neu = !ed.bob.vis.offen;
    const bt::Meldungen m = bt::visAnalyse(bsp, ursprung, ed.bob.vis);
    for (const bt::Meldung& x : m) {
        // LoadBSPFile: DoMessageBox( ..., "Error", EMessageBoxType::Error )
        if (x.text == "BSP file not found" || x.text == "Cant find a valid IBSP/RBSP/FBSP file" || x.text == "File is incorrect version") {
            qtMeldung("Error", x.text, MeldungArt::Fehler, false, nullptr);
        } else {
            melden({x});
        }
    }
    if (neu && ed.bob.vis.offen) { ed.bob.visOeffnen = true; }   // ui_show
    ++ed.sichtStand;
}

void doTreePlanter() {   // DoTreePlanter: an / aus
    Editor& ed = e();
    if (ed.bob.planter.aktiv) {
        ed.bob.planter = bt::TreePlanter{};
        return;
    }
    std::string konfig;
    const bool da = btLesen("tp_ent.txt", konfig);
    bt::Meldungen m = bt::treePlanterStarten(ed.karte, da ? konfig : std::string(), ed.bob.planter);
    for (bt::Meldung& x : m) {   // parsed no entity name from "<GetFilename( "bt/tp_ent.txt" )>"
        const std::string alt = "\"bt/tp_ent.txt\"";
        const std::size_t p = x.text.find(alt);
        if (p != std::string::npos) { x.text.replace(p, alt.size(), "\"" + btPfad("tp_ent.txt") + "\""); }
    }
    melden(m);
}

void doTrainPathPlot() {   // DoTrainPathPlot: neu bauen
    Editor& ed = e();
    melden(bt::splinesBauen(ed.karte, ed.bob.splines));
    ++ed.sichtStand;
}

void direkt(const std::function<bt::Meldungen(karte::Karte&)>& f) {
    melden(f(e().karte));
    nachBefehl();
}

// --- die Dialoge ---------------------------------------------------------------------

constexpr float kRand = 11.0F;
constexpr float kAbstand = 6.0F;

// Intersect (DoIntersectBox): QVBoxLayout, SetFixedSize
void intersectZeichnen(BobToolzZustand& b, bool& fertig, bool& ok) {
    bt::IntersectRS& rs = b.intersect;
    const std::string r1 = trK(Str::KeBtWholeMap);
    const std::string r2 = trK(Str::KeBtSelBrushes);
    const std::string c1 = trK(Str::KeBtDetail);
    const std::string c2 = trK(Str::KeBtDupOnly);
    const std::vector<std::string> kn{trK(Str::KePrefOk), trK(Str::KeCancel)};
    const float innen = std::max({qtd::radioHinweis(r1), qtd::radioHinweis(r2), qtd::hakenHinweis(c1), qtd::hakenHinweis(c2), qtd::knoepfeHinweis(kn)});
    const float innenB = kRand + innen + kRand;
    const float innenH = kRand + 20 + 6 + 20 + 6 + 3 + 6 + 20 + 6 + 20 + 6 + 22 + kRand;   // 157 wie im Foto
    if (!qtd::beginnen(std::string(trK(Str::KeBtIntersect)) + "###btintersect", b.oeffnen, innenB, innenH)) {
        fertig = qtd::geschlossen();
        return;
    }
    qtd::merke("Dialog", 0, 0, innenB, innenH);
    int ganz = rs.ganzeKarte ? 0 : 1;
    (void)qtd::radio(r1, "btwholemap", ganz, 0, kRand, 11);
    (void)qtd::radio(r2, "btselbrushes", ganz, 1, kRand, 37);
    rs.ganzeKarte = ganz == 0;
    qtd::hLinie(kRand, 63, innen);
    (void)qtd::haken(c1, "btdetail", rs.detail, kRand, 72);
    (void)qtd::haken(c2, "btduponly", rs.nurDuplikate, kRand, 98);
    const int k = qtd::knoepfe(kn, "btintersectok", innenB - kRand, 124, 0, 1);
    fertig = k >= 0;
    ok = k == 0;
    qtd::beenden(fertig);
}

// Polygon Builder (DoPolygonBox): QHBoxLayout( QFormLayout | QVBoxLayout )
void polygonZeichnen(BobToolzZustand& b, bool& fertig, bool& ok) {
    bt::PolygonRS& rs = b.polygon;
    const std::string l1 = trK(Str::KeBtSides);
    const std::string l2 = trK(Str::KeBtBorderW);
    const std::string c1 = trK(Str::KeBtUseBorder);
    const std::string c2 = trK(Str::KeBtInverse);
    const std::string c3 = trK(Str::KeBtAlignTop);
    const std::vector<std::string> kn{trK(Str::KePrefOk), trK(Str::KeCancel)};
    const float labelB = std::max(qtd::textBreite(l1), qtd::textBreite(l2));
    const float feldB = std::max({qtd::spinHinweis(3, 128, 0), qtd::spinHinweis(8, 256, 0), qtd::knoepfeHinweis(kn)});
    const float x1 = kRand + labelB + kAbstand;
    const float formRechts = x1 + feldB;
    const float vboxB = std::max({qtd::hakenHinweis(c1), qtd::hakenHinweis(c2), qtd::hakenHinweis(c3)});
    const float vboxX = formRechts + kAbstand;
    const float innenB = vboxX + vboxB + kRand;
    const float formH = 22 + 6 + 22 + 6 + 22;   // 78
    const float innenH = kRand + formH + kRand;
    if (!qtd::beginnen(std::string(trK(Str::KeBtPolygon)) + "###btpolygon", b.oeffnen, innenB, innenH)) {
        fertig = qtd::geschlossen();
        return;
    }
    qtd::merke("Dialog", 0, 0, innenB, innenH);
    (void)qtd::ganzFeld(l1, "btsides", rs.seiten, 3, 128, 1, kRand, x1, feldB, 11);
    (void)qtd::ganzFeld(l2, "btborder", rs.randBreite, 8, 256, 1, kRand, x1, feldB, 39, rs.rand);   // setEnabled( false ) bis "Use Border"
    const int k = qtd::knoepfe(kn, "btpolygonok", formRechts, 67, 0, 1);
    // die Haken in der Hoehe 78 verteilt (qGeomCalc: der Rest vor, zwischen und nach den Elementen)
    std::vector<karte::LayoutFeld> v(3);
    for (auto& f : v) {
        f.minimum = 20;
        f.hinweis = 20;
        f.maximum = 20;
        f.leer = false;
    }
    karte::geomCalc(v, 0, 3, static_cast<int>(kRand), static_cast<int>(formH), static_cast<int>(kAbstand));
    const bool randAlt = rs.rand;
    const bool inversAlt = rs.invers;
    (void)qtd::haken(c1, "btuseborder", rs.rand, vboxX, static_cast<float>(v[0].pos));
    (void)qtd::haken(c2, "btinverse", rs.invers, vboxX, static_cast<float>(v[1].pos));
    (void)qtd::haken(c3, "btaligntop", rs.obenAusrichten, vboxX, static_cast<float>(v[2].pos));
    if (rs.invers && !inversAlt) { rs.rand = false; }   // either of border or inverse may be used
    if (rs.rand && !randAlt) { rs.invers = false; }
    fertig = k >= 0;
    ok = k == 0;
    qtd::beenden(fertig);
}

// Stair Builder (DoBuildStairsBox): QFormLayout
void treppeZeichnen(BobToolzZustand& b, bool& fertig, bool& ok) {
    bt::BuildStairsRS& rs = b.treppe;
    const std::string lh = trK(Str::KeBtStairH);
    const std::string ld = trK(Str::KeBtDirection);
    const std::string ls = trK(Str::KeBtStyle);
    const std::string lm = trK(Str::KeBtMainTex);
    const std::string lr = trK(Str::KeBtRiserTex);
    const std::string cd = trK(Str::KeBtUseDetail);
    const std::vector<std::string> richtungen{trK(Str::KeBtNorth), trK(Str::KeBtSouth), trK(Str::KeBtEast), trK(Str::KeBtWest)};
    const std::vector<std::string> stile{trK(Str::KeBtOriginal), trK(Str::KeBtBobStyle), trK(Str::KeBtCorner)};
    const std::vector<std::string> kn{trK(Str::KePrefOk), trK(Str::KeCancel)};
    const float labelB = std::max({qtd::textBreite(lh), qtd::textBreite(ld), qtd::textBreite(ls), qtd::textBreite(lm), qtd::textBreite(lr)});
    const float feldB = std::max({qtd::spinHinweis(1, 1024, 0), qtd::radioReiheHinweis(richtungen), qtd::radioReiheHinweis(stile),
                                  qtd::hakenHinweis(cd), qtd::zeileHinweis(), qtd::knoepfeHinweis(kn)});
    const float x1 = kRand + labelB + kAbstand;
    const float innenB = x1 + feldB + kRand;
    const float innenH = kRand + 22 + 6 + 20 + 6 + 20 + 6 + 20 + 6 + 22 + 6 + 22 + 6 + 22 + kRand;   // 212
    if (!qtd::beginnen(std::string(trK(Str::KeBtStair)) + "###btstair", b.oeffnen, innenB, innenH)) {
        fertig = qtd::geschlossen();
        return;
    }
    qtd::merke("Dialog", 0, 0, innenB, innenH);
    (void)qtd::ganzFeld(lh, "btstairh", rs.stufenHoehe, 1, 1024, 1, kRand, x1, feldB, 11);
    qtd::text(ld, kRand, 39, 20);
    qtd::radioReihe(richtungen, "btrichtung", rs.richtung, x1, feldB, 39);
    qtd::text(ls, kRand, 65, 20);
    const int stilAlt = rs.stil;
    qtd::radioReihe(stile, "btstil", rs.stil, x1, feldB, 65);
    (void)stilAlt;
    (void)qtd::haken(cd, "btusedetail", rs.detail, x1, 91, 20, rs.stil == bt::STYLE_BOB);   // nur bei Bob's Style
    if (rs.stil != bt::STYLE_BOB) { rs.detail = false; }
    qtd::text(lm, kRand, 117, 22);
    (void)qtd::zeileFeld("btmaintex", rs.hauptTextur, x1, feldB, 117);
    qtd::text(lr, kRand, 145, 22);
    (void)qtd::zeileFeld("btrisertex", rs.stirnTextur, x1, feldB, 145);
    const int k = qtd::knoepfe(kn, "btstairok", innenB - kRand, 173, 0, 1);
    fertig = k >= 0;
    ok = k == 0;
    qtd::beenden(fertig);
}

// Door Builder (DoDoorsBox): QFormLayout
void tuerZeichnen(BobToolzZustand& b, bool& fertig, bool& ok) {
    bt::DoorRS& rs = b.tuer;
    const std::string lt = trK(Str::KeBtDoorTex);
    const std::string lr = trK(Str::KeBtTrimTex);
    const std::string lo = trK(Str::KeBtOrientation);
    const std::string c1 = trK(Str::KeBtScaleMainH);
    const std::string c2 = trK(Str::KeBtScaleMainV);
    const std::string c3 = trK(Str::KeBtScaleTrimH);
    const std::string c4 = trK(Str::KeBtScaleTrimV);
    const std::string ns = trK(Str::KeBtNS);
    const std::string ew = trK(Str::KeBtEW);
    const std::vector<std::string> kn{trK(Str::KePrefOk), trK(Str::KeCancel)};
    const float labelB = std::max({qtd::textBreite(lt), qtd::textBreite(lr), qtd::textBreite(lo)});
    const float feldB = std::max({qtd::kombiHinweis(b.tuerTexturen), qtd::kombiHinweis(b.rahmenTexturen), qtd::hakenHinweis(c1), qtd::hakenHinweis(c2),
                                  qtd::hakenHinweis(c3), qtd::hakenHinweis(c4), qtd::radioHinweis(ns), qtd::radioHinweis(ew), qtd::knoepfeHinweis(kn)});
    const float x1 = kRand + labelB + kAbstand;
    const float innenB = x1 + feldB + kRand;
    const float innenH = kRand + 22 + 6 + 20 + 6 + 20 + 6 + 22 + 6 + 20 + 6 + 20 + 6 + 20 + 6 + 20 + 6 + 22 + kRand;   // 266
    if (!qtd::beginnen(std::string(trK(Str::KeBtDoor)) + "###btdoor", b.oeffnen, innenB, innenH)) {
        fertig = qtd::geschlossen();
        return;
    }
    qtd::merke("Dialog", 0, 0, innenB, innenH);
    qtd::text(lt, kRand, 11, 22);
    (void)qtd::kombiFeld("btdoortex", rs.hauptTextur, b.tuerTexturen, x1, feldB, 11);
    (void)qtd::haken(c1, "btscalemainh", rs.hauptH, x1, 39);
    (void)qtd::haken(c2, "btscalemainv", rs.hauptV, x1, 65);
    qtd::text(lr, kRand, 91, 22);
    (void)qtd::kombiFeld("bttrimtex", rs.randTextur, b.rahmenTexturen, x1, feldB, 91);
    (void)qtd::haken(c3, "btscaletrimh", rs.randH, x1, 119);
    (void)qtd::haken(c4, "btscaletrimv", rs.randV, x1, 145);
    qtd::text(lo, kRand, 171, 20);
    (void)qtd::radio(ns, "btns", rs.ausrichtung, bt::DIRECTION_NS, x1, 171);
    (void)qtd::radio(ew, "btew", rs.ausrichtung, bt::DIRECTION_EW, x1, 197);
    const int k = qtd::knoepfe(kn, "btdoorok", innenB - kRand, 223, 0, 1);
    fertig = k >= 0;
    ok = k == 0;
    qtd::beenden(fertig);
}

// Ein QTabWidget: die Reiter oben (Hoehe 22), darunter der Rahmen der Seite.
// Gibt die Hoehe der Reiterzeile zurueck; aktiv = currentIndex.
float reiter(const std::string& id, const std::vector<std::string>& texte, int& aktiv, float x, float y, float b, float seitenH) {
    float xi = x;
    for (std::size_t i = 0; i < texte.size(); ++i) {
        const float w = qtd::textBreite(texte[i]) + 16.0F;   // QTabBar: Text + 2 x PM_TabBarTabHSpace / 2
        ImGui::SetCursorScreenPos(qtd::bp(xi, y));
        const bool an = aktiv == static_cast<int>(i);
        if (ImGui::InvisibleButton(("##" + id + std::to_string(i)).c_str(), ImVec2{qtd::px(w), qtd::px(22.0F)})) { aktiv = static_cast<int>(i); }
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(qtd::bp(xi, y + (an ? 0.0F : 2.0F)), qtd::bp(xi + w, y + 22.0F),
                          ImGui::GetColorU32(an ? ImGuiCol_TabSelected : (ImGui::IsItemHovered() ? ImGuiCol_TabHovered : ImGuiCol_Tab)));
        qtd::text(texte[i], xi + 8.0F, y + (an ? 0.0F : 2.0F), 20.0F);
        qtd::merke(id + std::to_string(i), xi, y, w, 22.0F);
        xi += w;
    }
    qtd::rahmen(x, y + 22.0F, b, seitenH);
    return 22.0F;
}

// Aperture Door Builder (DoApertureDoorsBox): QFormLayout mit QTabWidgets
void aperturZeichnen(BobToolzZustand& b, bool& fertig, bool& ok) {
    bt::ApertureDoorRS& rs = b.apertur;
    const std::vector<std::string> kn{trK(Str::KePrefOk), trK(Str::KeCancel)};
    const std::string lMain = trK(Str::KeBtMainTex);
    const std::string lTrim = trK(Str::KeBtTrimTex2);
    const std::string lDist = trK(Str::KeBtSetDistance);
    const std::string lAngle = trK(Str::KeBtOpenAngle);
    const std::string lHealth = trK(Str::KeBtHealth);
    const std::string lWait = trK(Str::KeBtWait);
    const std::string lSeg = trK(Str::KeBtSegments);
    const std::string lOff = trK(Str::KeBtOffsetStart);
    const std::string lD1 = trK(Str::KeBtDepth1);
    const std::string lD2 = trK(Str::KeBtDepth2);
    const std::string lRound = trK(Str::KeBtRound);
    const std::string lTime = trK(Str::KeBtTime);
    const std::string lSpeed = trK(Str::KeBtSpeed);
    const std::string lOffset = trK(Str::KeBtOffset);
    // Feldspalte des aeusseren Formulars
    const float labelB = std::max({qtd::textBreite(lMain), qtd::textBreite(lTrim), qtd::hakenHinweis(lDist), qtd::textBreite(lAngle), qtd::textBreite(lWait)});
    const float zahlB = qtd::spinHinweis(-999999, 999999, 2);
    const float innenLabelB = std::max({qtd::textBreite(lSeg), qtd::textBreite(lMain), qtd::textBreite(lTrim), qtd::textBreite(lD1), qtd::textBreite(lD2),
                                        qtd::textBreite(lTime), qtd::textBreite(lSpeed)});
    const float innenFeldB = std::max({qtd::kombiHinweis(b.tuerTexturen), qtd::kombiHinweis(b.rahmenTexturen), zahlB, qtd::spinHinweis(3, 256, 0)});
    const float seiteB = 11.0F + innenLabelB + 6.0F + innenFeldB + 11.0F + 2.0F;   // Rand der Seite + Rahmen
    const float stilleB = 11.0F + qtd::textBreite(lOffset) + 3.0F * (6.0F + qtd::spinHinweis(-999999, 999999, 0, "X: ")) + 11.0F + 2.0F;
    const float feldB = std::max({qtd::kombiHinweis(b.tuerTexturen), qtd::kombiHinweis(b.rahmenTexturen), zahlB, qtd::hakenHinweis(lHealth),
                                  qtd::knoepfeHinweis(kn)});
    const float x1 = kRand + labelB + kAbstand;
    const float innenB = std::max({x1 + feldB, kRand + seiteB, kRand + stilleB}) + kRand;
    const float tabB = innenB - 2.0F * kRand;
    // Hoehen: Reiter 22 + Seite (Rand 11 + Zeilen + 11)
    const float s1 = 11 + 22 + 6 + 20 + 11 + 2;                   // From params
    const float sInnen = 11 + 22 + 6 + 22 + 6 + 22 + 6 + 22 + 11 + 2;   // Segmented / Sloped (vier Zeilen)
    const float s2 = 11 + 22 + sInnen + 11 + 2;                   // Normal segments mit dem inneren Reiter
    const float s3 = 11 + 22 + 11 + 2;                            // Silence: Brush (eine Zeile)
    const float s4 = 11 + 22 + 11 + 2;                            // Open time / speed
    float y = kRand;
    const float yTab1 = y;
    y += 22 + s1 + 6;
    const float yMain = y;
    y += 22 + 6;
    const float yTrim = y;
    y += 22 + 6;
    const float yTab2 = y;
    y += 22 + s2 + 6;
    const float yTab3 = y;
    y += 22 + s3 + 6;
    const float yTab4 = y;
    y += 22 + s4 + 6;
    const float yDist = y;
    y += 22 + 6;
    const float yAngle = y;
    y += 22 + 6;
    const float yHealth = y;
    y += 20 + 6;
    const float yWait = y;
    y += 22 + 6;
    const float yKn = y;
    const float innenH = yKn + 22 + kRand;
    if (!qtd::beginnen(std::string(trK(Str::KeBtAperture)) + "###btaperture", b.oeffnen, innenB, innenH)) {
        fertig = qtd::geschlossen();
        return;
    }
    qtd::merke("Dialog", 0, 0, innenB, innenH);
    const float sx = kRand + 1.0F + 11.0F;                 // linke Kante in einer Seite
    const float sx1 = sx + innenLabelB + kAbstand;         // Feldspalte in einer Seite
    const float sfB = tabB - 2.0F - 22.0F - innenLabelB - kAbstand;
    // From params / From winding
    int t1 = rs.ausWicklung ? 1 : 0;
    (void)reiter("btfromtab", {trK(Str::KeBtFromParams), trK(Str::KeBtFromWinding)}, t1, kRand, yTab1, tabB, s1);
    rs.ausWicklung = t1 == 1;
    if (t1 == 0) {
        (void)qtd::ganzFeld(lSeg, "btsegments", rs.segmente, 3, 256, 1, sx, sx1, sfB, yTab1 + 22 + 12);
        (void)qtd::haken(lOff, "btoffsetstart", rs.startVersetzt, sx1, yTab1 + 22 + 12 + 28);
    }
    std::string haupt = ohnePraefix(rs.hauptTextur);
    std::string rand = ohnePraefix(rs.randTextur);
    qtd::text(lMain, kRand, yMain, 22);
    (void)qtd::kombiFeld("btapmain", haupt, b.tuerTexturen, x1, innenB - kRand - x1, yMain);
    qtd::text(lTrim, kRand, yTrim, 22);
    (void)qtd::kombiFeld("btaptrim", rand, b.rahmenTexturen, x1, innenB - kRand - x1, yTrim);
    rs.hauptTextur = mitPraefix(haupt);
    rs.randTextur = mitPraefix(rand);
    // Normal segments / Sloped segments
    int t2 = rs.schraegeSegmente ? 1 : 0;
    (void)reiter("btslopedtab", {trK(Str::KeBtNormalSeg), trK(Str::KeBtSlopedSeg)}, t2, kRand, yTab2, tabB, s2);
    rs.schraegeSegmente = t2 == 1;
    const float sy = yTab2 + 22 + 12;
    if (t2 == 0) {
        int ti = static_cast<int>(rs.innen);
        (void)reiter("btinnertab", {trK(Str::KeBtNoLayer), trK(Str::KeBtSegmented), trK(Str::KeBtSloped)}, ti, sx, sy, tabB - 24.0F, sInnen);
        rs.innen = static_cast<bt::ApertureDoorRS::Innen>(ti);
        if (ti != 0) {   // dasselbe Formular in "Segmented" und "Sloped"
            const float ix = sx + 12.0F;
            const float ix1 = ix + innenLabelB + kAbstand;
            const float ifB = tabB - 24.0F - 24.0F - innenLabelB - kAbstand;
            std::string ih = ohnePraefix(rs.innenHauptTextur);
            std::string ir = ohnePraefix(rs.innenRandTextur);
            const float iy = sy + 22 + 12;
            qtd::text(lMain, ix, iy, 22);
            (void)qtd::kombiFeld("btapinmain", ih, b.tuerTexturen, ix1, ifB, iy);
            qtd::text(lTrim, ix, iy + 28, 22);
            (void)qtd::kombiFeld("btapintrim", ir, b.rahmenTexturen, ix1, ifB, iy + 28);
            rs.innenHauptTextur = mitPraefix(ih);
            rs.innenRandTextur = mitPraefix(ir);
            (void)qtd::zahlFeld(lD1, "btapind1", rs.innenTiefe1, 0, 999999, 2, 1, ix, ix1, ifB, iy + 56);
            (void)qtd::zahlFeld(lD2, "btapind2", rs.innenTiefe2, 0, 999999, 2, 1, ix, ix1, ifB, iy + 84);
        }
    } else {
        (void)qtd::haken(lRound, "btapround", rs.schraegRund, sx1, sy);
        (void)qtd::zahlFeld(lD1, "btapsd1", rs.schraegTiefe1, 0, 999999, 2, 1, sx, sx1, sfB, sy + 26);
        (void)qtd::zahlFeld(lD2, "btapsd2", rs.schraegTiefe2, 0, 999999, 2, 1, sx, sx1, sfB, sy + 54);
    }
    // Silence: no / Brush
    int t3 = static_cast<int>(rs.stille);
    (void)reiter("btsilencetab", {trK(Str::KeBtSilenceNo), trK(Str::KeBtBrush)}, t3, kRand, yTab3, tabB, s3);
    rs.stille = static_cast<bt::ApertureDoorRS::Stille>(t3);
    if (t3 == 1) {   // QHBoxLayout: "Offset", drei SpinBox mit "X: " / "Y: " / "Z: "
        const float hy = yTab3 + 22 + 12;
        qtd::text(lOffset, sx, hy, 22);
        const float w = (tabB - 24.0F - qtd::textBreite(lOffset) - 3.0F * kAbstand) / 3.0F;
        static const char* const achsen[3] = {"X: ", "Y: ", "Z: "};
        for (int i = 0; i < 3; ++i) {
            double v = rs.stilleVersatz[static_cast<std::size_t>(i)];
            (void)qtd::zahlFeld("", "btapsil" + std::to_string(i), v, -999999, 999999, 0, 256, -1.0F,
                                sx + qtd::textBreite(lOffset) + kAbstand + static_cast<float>(i) * (w + kAbstand), w, hy, true, achsen[i]);
            rs.stilleVersatz[static_cast<std::size_t>(i)] = v;
        }
    }
    // Open time / Open speed
    int t4 = rs.tempoNutzen ? 1 : 0;
    (void)reiter("btspeedtab", {trK(Str::KeBtOpenTime), trK(Str::KeBtOpenSpeed)}, t4, kRand, yTab4, tabB, s4);
    rs.tempoNutzen = t4 == 1;
    if (t4 == 0) {
        (void)qtd::zahlFeld(lTime, "btaptime", rs.zeit, 0.001, 256, 3, 0.1, sx, sx1, sfB, yTab4 + 22 + 12);
    } else {
        (void)qtd::zahlFeld(lSpeed, "btapspeed", rs.tempo, -1, 999999, 2, 1, sx, sx1, sfB, yTab4 + 22 + 12);   // negative speed = infinite speed
    }
    // Set distance (QCheckBox als Beschriftung), Open Angle *, Health, Wait
    (void)qtd::haken(lDist, "btapsetdist", rs.distanzSetzen, kRand, yDist, 22);
    (void)qtd::zahlFeld("", "btapdist", rs.distanz, -999999, 999999, 2, 1, -1.0F, x1, innenB - kRand - x1, yDist, rs.distanzSetzen);
    (void)qtd::zahlFeld(lAngle, "btapangle", rs.oeffnungsWinkel, -180, 180, 3, 1, kRand, x1, innenB - kRand - x1, yAngle, true, "",
                        trK(Str::KeBtOpenAngleTip));
    (void)qtd::haken(lHealth, "btaphealth", rs.gesundheit, x1, yHealth);
    (void)qtd::zahlFeld(lWait, "btapwait", rs.warten, -1, 999999, 2, 1, kRand, x1, innenB - kRand - x1, yWait);
    const int k = qtd::knoepfe(kn, "btapok", innenB - kRand, yKn, 0, 1);
    fertig = k >= 0;
    ok = k == 0;
    qtd::beenden(fertig);
}

// Path Plotter (DoPathPlotterBox): QFormLayout; Enable / Disable / Cancel
void bahnZeichnen(BobToolzZustand& b, bool& fertig, int& ergebnis) {
    bt::PathPlotterRS& rs = b.bahnWerte;
    const std::string l1 = trK(Str::KeBtPoints);
    const std::string l2 = trK(Str::KeBtMultiplier);
    const std::string l3 = trK(Str::KeBtGravity);
    const std::string c1 = trK(Str::KeBtNoUpdate);
    const std::string c2 = trK(Str::KeBtShowExtra);
    const std::vector<std::string> kn{trK(Str::KeBtEnable), trK(Str::KeBtDisable), trK(Str::KeCancel)};
    const float labelB = std::max({qtd::textBreite(l1), qtd::textBreite(l2), qtd::textBreite(l3)});
    const float feldB = std::max({qtd::spinHinweis(1, 200, 0), qtd::spinHinweis(1, 10, 2), qtd::spinHinweis(-10000, -1, 2), qtd::hakenHinweis(c1),
                                  qtd::hakenHinweis(c2), qtd::knoepfeHinweis(kn)});
    const float x1 = kRand + labelB + kAbstand;
    const float innenB = x1 + feldB + kRand;
    const float innenH = kRand + 22 + 6 + 22 + 6 + 22 + 6 + 20 + 6 + 20 + 6 + 22 + kRand;   // 180 wie im Foto
    if (!qtd::beginnen(std::string(trK(Str::KeBtPathPlotter)) + "###btpathplotter", b.oeffnen, innenB, innenH)) {
        fertig = qtd::geschlossen();
        return;
    }
    qtd::merke("Dialog", 0, 0, innenB, innenH);
    (void)qtd::ganzFeld(l1, "btpoints", rs.punkte, 1, 200, 1, kRand, x1, feldB, 11);
    double faktor = rs.faktor;
    (void)qtd::zahlFeld(l2, "btmultiplier", faktor, 1, 10, 2, 1, kRand, x1, feldB, 39, true, "", trK(Str::KeBtMultiplierTip));
    rs.faktor = static_cast<float>(faktor);
    double schwer = rs.schwerkraft;
    (void)qtd::zahlFeld(l3, "btgravity", schwer, -10000, -1, 2, 1, kRand, x1, feldB, 67);
    rs.schwerkraft = static_cast<float>(schwer);
    (void)qtd::haken(c1, "btnoupdate", rs.keineAktualisierung, x1, 95);
    (void)qtd::haken(c2, "btshowextra", rs.hilfslinien, x1, 121);
    const int k = qtd::knoepfe(kn, "btpathok", innenB - kRand, 147, 0, 2);
    fertig = k >= 0;
    ergebnis = k;
    qtd::beenden(fertig);
}

// Eine ankreuzbare QGroupBox mit einem Formular: Titel-Haken bei (x, y), Rahmen
// 18 tiefer; aus = Inhalt gesperrt.
void gruppe(const std::string& titel, const std::string& id, bool& an, float x, float y, float b, float h) {
    (void)qtd::haken(titel, id, an, x + 7.0F, y, 14.0F);
    qtd::rahmen(x, y + 7.0F, b, h - 7.0F);
}

// Texture Reset (DoResetTextureBox): QVBoxLayout mit QLabel und sechs
// ankreuzbaren QGroupBox (QFormLayout); Use Selected Brushes / Use All Brushes / Cancel
void texturZeichnen(BobToolzZustand& b, bool& fertig, int& ergebnis) {
    Editor& ed = e();
    bt::ResetTextureRS& rs = b.textur;
    const std::string lCur = std::string(trK(Str::KeBtCurTex)) + ed.texBro.shader;
    const std::string gNames = trK(Str::KeBtResetNames);
    const std::string lOld = trK(Str::KeBtOldName);
    const std::string lNew = trK(Str::KeBtNewName);
    const std::string titel[5] = {trK(Str::KeBtNewHScale), trK(Str::KeBtNewVScale), trK(Str::KeBtNewHShift), trK(Str::KeBtNewVShift), trK(Str::KeBtNewRot)};
    const std::vector<std::string> kn{trK(Str::KeBtUseSel), trK(Str::KeBtUseAll), trK(Str::KeCancel)};
    const float formLabel = std::max(qtd::textBreite(lOld), qtd::textBreite(lNew));
    float gruppeB = std::max(qtd::hakenHinweis(gNames) + 14.0F, 11.0F + formLabel + 6.0F + qtd::zeileHinweis() + 11.0F);
    for (const auto& t : titel) { gruppeB = std::max(gruppeB, qtd::hakenHinweis(t) + 14.0F); }
    gruppeB = std::max(gruppeB, 22.0F + qtd::spinHinweis(-999999, 999999, 2));
    const float innen = std::max({qtd::textBreite(lCur), gruppeB, qtd::knoepfeHinweis(kn)});
    const float innenB = kRand + innen + kRand;
    const float gNamenH = 7 + 11 + 22 + 6 + 22 + 11;   // Titel, Rand, zwei Zeilen
    const float gZahlH = 7 + 11 + 22 + 11;
    const float innenH = kRand + 16 + 6 + gNamenH + 6 + 5 * (gZahlH + 6) + 22 + kRand;
    if (!qtd::beginnen(std::string(trK(Str::KeBtTexReset)) + "###bttexreset", b.oeffnen, innenB, innenH)) {
        fertig = qtd::geschlossen();
        return;
    }
    qtd::merke("Dialog", 0, 0, innenB, innenH);
    qtd::text(lCur, kRand, kRand, 16);
    float y = kRand + 16 + 6;
    gruppe(gNames, "btresetnames", rs.nameErsetzen, kRand, y, innen, gNamenH);
    const float fx = kRand + 11.0F;
    const float fx1 = fx + formLabel + kAbstand;
    const float fB = innen - 22.0F - formLabel - kAbstand;
    qtd::text(lOld, fx, y + 18, 22, rs.nameErsetzen);
    (void)qtd::zeileFeld("btoldname", rs.alterName, fx1, fB, y + 18, 255, rs.nameErsetzen);
    qtd::text(lNew, fx, y + 46, 22, rs.nameErsetzen);
    (void)qtd::zeileFeld("btnewname", rs.neuerName, fx1, fB, y + 46, 255, rs.nameErsetzen);
    y += gNamenH + 6;
    bool* an[5] = {&rs.skalaSetzen[0], &rs.skalaSetzen[1], &rs.verschiebungSetzen[0], &rs.verschiebungSetzen[1], &rs.drehungSetzen};
    for (int i = 0; i < 5; ++i) {
        gruppe(titel[i], "btresetgrp" + std::to_string(i), *an[i], kRand, y, innen, gZahlH);
        double v = i < 2 ? rs.skala[i] : (i < 4 ? rs.verschiebung[i - 2] : rs.drehung);
        if (i < 2) {
            (void)qtd::zahlFeld("", "btresetval" + std::to_string(i), v, -1024, 1024, 3, 0.25, -1.0F, fx, innen - 22.0F, y + 18, *an[i]);
            rs.skala[i] = static_cast<float>(v);
        } else if (i < 4) {
            (void)qtd::zahlFeld("", "btresetval" + std::to_string(i), v, -999999, 999999, 2, 1, -1.0F, fx, innen - 22.0F, y + 18, *an[i]);
            rs.verschiebung[i - 2] = static_cast<float>(v);
        } else {   // DoubleSpinBox( -360, 360, 0, 2, 1, true ): rotation ist int
            (void)qtd::zahlFeld("", "btresetval4", v, -360, 360, 2, 1, -1.0F, fx, innen - 22.0F, y + 18, *an[i]);
            rs.drehung = static_cast<int>(v);
        }
        y += gZahlH + 6;
    }
    const int k = qtd::knoepfe(kn, "btresetok", innenB - kRand, y, 0, 2);
    fertig = k >= 0;
    ergebnis = k;
    qtd::beenden(fertig);
}

// Make Chain (DoMakeChainBox): QFormLayout
void ketteZeichnen(BobToolzZustand& b, bool& fertig, bool& ok) {
    bt::MakeChainRS& rs = b.kette;
    const std::string l1 = trK(Str::KeBtChainNum);
    const std::string l2 = trK(Str::KeBtChainName);
    const std::vector<std::string> kn{trK(Str::KePrefOk), trK(Str::KeCancel)};
    const float labelB = std::max(qtd::textBreite(l1), qtd::textBreite(l2));
    const float feldB = std::max({qtd::spinHinweis(1, 1000, 0), qtd::zeileHinweis(), qtd::knoepfeHinweis(kn)});
    const float x1 = kRand + labelB + kAbstand;
    const float innenB = x1 + feldB + kRand;
    const float innenH = kRand + 22 + 6 + 22 + 6 + 22 + kRand;   // 100 wie im Foto
    if (!qtd::beginnen(std::string(trK(Str::KeBtMakeChain)) + "###btmakechain", b.oeffnen, innenB, innenH)) {
        fertig = qtd::geschlossen();
        return;
    }
    qtd::merke("Dialog", 0, 0, innenB, innenH);
    (void)qtd::ganzFeld(l1, "btchainnum", rs.glieder, 1, 1000, 1, kRand, x1, feldB, 11);
    qtd::text(l2, kRand, 39, 22);
    (void)qtd::zeileFeld("btchainname", rs.name, x1, feldB, 39, 255);
    const int k = qtd::knoepfe(kn, "btchainok", innenB - kRand, 67, 0, 1);
    fertig = k >= 0;
    ok = k == 0;
    qtd::beenden(fertig);
}

// "BSP leafs" (DVisDrawer::ui_create): nicht modal; Tabelle, "Color per
// leaf", "From Cam or Selection". Schliessen = g_VisView.reset().
void visZeichnen() {
    Editor& ed = e();
    BobToolzZustand& b = ed.bob;
    if (!b.vis.offen) { return; }
    const std::vector<std::string> kopf{trK(Str::KeBtLeafNr), trK(Str::KeBtLeafsVis), trK(Str::KeBtSurfsVis), trK(Str::KeBtShadersVis)};
    float tabB = 2.0F;
    for (const auto& k : kopf) { tabB += qtd::textBreite(k) + 4.0F + 20.0F + 4.0F; }   // Kopf: Text + Sortierpfeil
    const std::string farbe = b.vis.farbeJeFlaeche ? trK(Str::KeBtColSurf) : trK(Str::KeBtColLeaf);
    const std::string knopf = trK(Str::KeBtFromCam);
    const float innenB = kRand + std::max({tabB, qtd::hakenHinweis(farbe), qtd::knopfHinweis(knopf)}) + kRand;
    const float tabH = 192.0F;   // QAbstractScrollArea: 10 Zeilen + Kopf (AdjustToContentsOnFirstShow, begrenzt)
    const float innenH = kRand + tabH + 6 + 20 + 6 + 22 + kRand;
    bool oeffnen = b.visOeffnen;
    if (!qtd::beginnen(std::string(trK(Str::KeBtBspLeafs)) + "###btbspleafs", oeffnen, innenB, innenH, false)) {
        b.visOeffnen = false;
        if (qtd::geschlossen()) {   // closeEvent: g_VisView.reset()
            b.vis = bt::VisAnsicht{};
            ++ed.sichtStand;
        }
        return;
    }
    b.visOeffnen = false;
    qtd::merke("BspLeafs", 0, 0, innenB, innenH);
    ImGui::SetCursorScreenPos(qtd::bp(kRand, kRand));
    const ImGuiTableFlags tf = ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit;
    if (ImGui::BeginTable("##btleaftabelle", 4, tf, ImVec2{qtd::px(innenB - 2.0F * kRand), qtd::px(tabH)})) {
        for (const auto& k : kopf) { ImGui::TableSetupColumn(k.c_str()); }
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();
        for (const bt::VisBlatt& z : b.vis.liste) {   // sortiert nach "surfs visible" absteigend (setSortIndicator( 2, Descending ))
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            const bool gewaehlt = z.blatt == b.vis.gewaehlt;
            if (ImGui::Selectable((std::to_string(z.blatt) + "##btblatt" + std::to_string(z.blatt)).c_str(), gewaehlt, ImGuiSelectableFlags_SpanAllColumns) &&
                !gewaehlt) {
                bt::visBlattWaehlen(b.vis, z.blatt);   // currentCellChanged
                ++ed.sichtStand;
            }
            if (gewaehlt && ImGui::IsWindowAppearing()) { ImGui::SetScrollHereY(); }
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d", z.blaetter);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%d", z.flaechen);
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%d", z.shader);
        }
        ImGui::EndTable();
    }
    bool jeFlaeche = b.vis.farbeJeFlaeche;
    if (qtd::haken(farbe, "btcolorper", jeFlaeche, kRand, kRand + tabH + 6)) {   // stateChanged: Text, ClearPoints, dieselbe Zeile neu
        b.vis.farbeJeFlaeche = jeFlaeche;
        const int zeile = b.vis.gewaehlt;
        b.vis.gewaehlt = -1;
        bt::visBlattWaehlen(b.vis, zeile);
        ++ed.sichtStand;
    }
    if (qtd::knoepfe({knopf}, "btfromcam", innenB - kRand, kRand + tabH + 6 + 20 + 6, -1, -1) == 0) { doVisAnalyse(); }
    qtd::beenden(false);
}

// Was nach OK geschieht (die zweite Haelfte der Do*-Funktionen)
void dialogAusfuehren(int art, int ergebnis) {
    Editor& ed = e();
    BobToolzZustand& b = ed.bob;
    karte::Karte& k = ed.karte;
    switch (art) {
    case 1: melden(bt::intersect(k, b.intersect, umgebung())); break;
    case 2: melden(bt::polygonBuilder(k, b.polygon, umgebung())); break;
    case 3: melden(bt::stairBuilder(k, b.treppe)); break;
    case 4: melden(bt::doorBuilder(k, b.tuer, umgebung())); break;
    case 5: melden(bt::apertureDoor(k, b.apertur, umgebung())); break;
    case 6:
        if (ergebnis == 0) {   // eIDYES: DBobView_setEntity
            melden(bt::pathPlotter(k, b.bahnWerte, b.bahn));
        } else if (ergebnis == 1) {   // eIDNO: g_PathView.reset()
            b.bahn = bt::Bahn{};
        }
        break;
    case 7:
        if (ergebnis == 0 || ergebnis == 1) { melden(bt::resetTextures(k, b.textur, ergebnis == 1)); }   // eIDOK / eIDYES
        break;
    case 8: melden(bt::makeChain(k, b.kette)); break;
    default: break;
    }
    nachBefehl();
}

karte::Linie linie(const karte::Vektor3f& a, const karte::Vektor3f& c, karte::Farbe f) { return karte::Linie{a, c, f}; }

kartegpu::LinienPunkt lp(const karte::Vektor3f& v) {
    kartegpu::LinienPunkt q{};
    q.pos[0] = v[0];
    q.pos[1] = v[1];
    q.pos[2] = v[2];
    q.farbe = 0xFFFFFFFFU;
    return q;
}

}  // namespace

// --- oeffentlich -----------------------------------------------------------------------

std::vector<PluginUmsetzung> pluginUmsetzungen() {
    std::vector<PluginUmsetzung> v;
    // QERPlug_Dispatch: vor jedem Befehl LoadLists()
    const auto bob = [&v](const char* name, std::function<void()> f, std::function<bool()> an = nullptr) {
        const std::string n = name;
        v.push_back({std::string("bobToolz::") + name, [f, n] {
                         listenLaden();
                         e().bob.letzterBefehl = n;
                         f();
                     },
                     std::move(an)});
    };
    bob("About", [] { qtMeldung("About", bt::kAbout, MeldungArt::Info, false, nullptr); });
    bob("StairBuilder", [] { doStairs(); });
    bob("ApertureDoor", [] { doAperture(); });
    bob("DoorBuilder", [] { doDoors(); });
    bob("FindDuplicates", [] { direkt([](karte::Karte& k) { return bt::findDuplicates(k); }); });
    bob("Intersect", [] { doIntersect(); });
    bob("MakeChain", [] { doMakeChain(); });
    bob("PathPlotter", [] { doPathPlotter(); });
    bob("ResetTextures", [] { doResetTextures(); });
    bob("PitOMatic", [] { direkt([](karte::Karte& k) { return bt::pitOMatic(k, umgebung()); }); });
    bob("VisViewer", [] { doVisAnalyse(); });
    bob("BrushCleanup", [] { direkt([](karte::Karte& k) { return bt::brushCleanup(k); }); });
    bob("PolygonBuilder", [] { doPolygons(); });
    bob("CaulkSelection", [] { direkt([](karte::Karte& k) { return bt::caulkSelection(k); }); });
    bob("ExplodeEntity", [] { direkt([](karte::Karte& k) { return bt::explodeEntity(k); }); });
    bob("TreePlanter", [] { doTreePlanter(); }, [] { return e().bob.planter.aktiv; });   // eToggleButton
    bob("DropEntity", [] { direkt([](karte::Karte& k) { return bt::dropEntities(k, e().bob.planter.aktiv ? &e().bob.planter : nullptr); }); });
    bob("PlotSplines", [] { doTrainPathPlot(); });
    bob("MergePatches", [] { direkt([](karte::Karte& k) { return bt::mergePatches(k); }); });
    bob("SplitPatch", [] { direkt([](karte::Karte& k) { return bt::splitPatch(k, 0); }); });
    bob("SplitPatchColumns", [] { direkt([](karte::Karte& k) { return bt::splitPatch(k, 1); }); });
    bob("SplitPatchRows", [] { direkt([](karte::Karte& k) { return bt::splitPatch(k, 2); }); });
    bob("FlipTerrain", [] { direkt([](karte::Karte& k) { return bt::flipTerrain(k); }); });
    return v;
}

void pluginDialogeZeichnen() {
    Editor& ed = e();
    BobToolzZustand& b = ed.bob;
    // DBobView: folgt dem Ziel (TargetChanged / UpdatePath), solange nicht "No Dynamic Update"
    if (b.bahn.aktiv && b.bahn.aktualisieren) { (void)bt::bahnAktualisieren(ed.karte, b.bahn); }
    visZeichnen();
    if (b.dialog == 0) { return; }
    bool fertig = false;
    bool ok = false;
    int ergebnis = -1;
    const int art = b.dialog;
    switch (art) {
    case 1: intersectZeichnen(b, fertig, ok); break;
    case 2: polygonZeichnen(b, fertig, ok); break;
    case 3: treppeZeichnen(b, fertig, ok); break;
    case 4: tuerZeichnen(b, fertig, ok); break;
    case 5: aperturZeichnen(b, fertig, ok); break;
    case 6: bahnZeichnen(b, fertig, ergebnis); break;
    case 7: texturZeichnen(b, fertig, ergebnis); break;
    case 8: ketteZeichnen(b, fertig, ok); break;
    default: b.dialog = 0; return;
    }
    if (!fertig) { return; }
    b.dialog = 0;
    if (art == 6 || art == 7) {
        if (ergebnis >= 0) { dialogAusfuehren(art, ergebnis); }
    } else if (ok) {
        dialogAusfuehren(art, 0);
    }
}

void pluginsFreigeben() {   // BobToolz_destroy
    BobToolzZustand& b = e().bob;
    b.bahn = bt::Bahn{};
    b.vis = bt::VisAnsicht{};
    b.splines = bt::Splines{};
    b.planter = bt::TreePlanter{};
    b.dialog = 0;
}

bool pluginXyDruck(int fenster, int knopf, const karte::Vektor3f& welt) {
    Editor& ed = e();
    BobToolzZustand& b = ed.bob;
    // XYWindowMouseDown_connect haengt am Hauptfenster (GetXYWnd); DTreePlanter::mouseDown:
    // nur M1 und nur in der Draufsicht (XYWindow_getViewType() == XY)
    if (!b.planter.aktiv || fenster != 0 || knopf != 0 || ed.xy[0].ansicht.typ != karte::Sicht::XY) { return false; }
    bt::Meldungen m;
    if (bt::treePlanterKlick(ed.karte, b.planter, welt, umgebung(), m)) { ++b.gepflanzt; }
    melden(m);
    if (b.planter.autoVerknuepfen) { doTrainPathPlot(); }   // if( m_autoLink ) DoTrainPathPlot()
    nachBefehl();
    return true;   // SIGNAL_STOP_EMISSION
}

std::vector<PluginLinien> pluginLinien2d() {
    const BobToolzZustand& b = e().bob;
    std::vector<PluginLinien> v;
    // DBobView::renderWireframe -> renderSolid: $bobtoolz/bobview/line (rot, 2) und
    // mit "Show Bounding Lines" viermal versetzt in $bobtoolz/bobview/box (0.25 0.75 0.75, 1)
    if (b.bahn.aktiv && b.bahn.punkte.size() >= 2) {
        PluginLinien rot;
        rot.breite = 2.0F;
        const karte::Farbe fr = karte::farbeAus(1, 0, 0);
        for (std::size_t i = 0; i + 1 < b.bahn.punkte.size(); ++i) { rot.linien.push_back(linie(b.bahn.punkte[i], b.bahn.punkte[i + 1], fr)); }
        v.push_back(std::move(rot));
        if (b.bahn.hilfslinien) {
            PluginLinien kasten;
            const karte::Farbe fk = karte::farbeAus(0.25F, 0.75F, 0.75F);
            for (const karte::Vektor3f& d : bt::bahnKastenVersaetze()) {
                for (std::size_t i = 0; i + 1 < b.bahn.punkte.size(); ++i) { kasten.linien.push_back(linie(b.bahn.punkte[i] + d, b.bahn.punkte[i + 1] + d, fk)); }
            }
            v.push_back(std::move(kasten));
        }
    }
    // DTrainDrawer::renderWireframe: $bobtoolz/traindrawer/wireframe (rot, 1)
    if (b.splines.anzeigen) {
        PluginLinien s;
        const karte::Farbe fr = karte::farbeAus(1, 0, 0);
        for (const auto& l : b.splines.linien) {
            for (std::size_t i = 0; i + 1 < l.size(); ++i) { s.linien.push_back(linie(l[i], l[i + 1], fr)); }
        }
        v.push_back(std::move(s));
    }
    // DVisDrawer::renderWireframe: die Dreiecke und Wicklungen als Umriss (ohne RENDER_FILL)
    if (b.vis.offen && (!b.vis.flaechen.empty() || !b.vis.wicklungen.empty())) {
        PluginLinien s;
        for (const bt::VisFlaeche& f : b.vis.flaechen) {
            const karte::Farbe fc = karte::farbeAus(f.farbe[0], f.farbe[1], f.farbe[2]);
            for (std::size_t i = 0; i + 2 < f.indizes.size(); i += 3) {
                for (std::size_t j = 0; j < 3; ++j) { s.linien.push_back(linie(f.ecken[f.indizes[i + j]], f.ecken[f.indizes[i + ((j + 1) % 3)]], fc)); }
            }
        }
        const karte::Farbe fw = karte::farbeAus(1, 1, 1);
        for (const auto& w : b.vis.wicklungen) {
            for (std::size_t i = 0; i < w.size(); ++i) { s.linien.push_back(linie(w[i], w[(i + 1) % w.size()], fw)); }
        }
        v.push_back(std::move(s));
    }
    return v;
}

void pluginKameraVoll(const float* matrix, bool fuellen) {
    (void)fuellen;
    const BobToolzZustand& b = e().bob;
    if (!b.bahn.aktiv || b.bahn.punkte.size() < 2) { return; }
    // DBobView: RENDER_COLOURWRITE | RENDER_DEPTHWRITE | RENDER_DEPTHTEST, eSortFullbright
    const auto strich = [&](const karte::Vektor3f& d, float r, float g, float bl, float breite) {
        std::vector<kartegpu::LinienPunkt> p;
        for (std::size_t i = 0; i + 1 < b.bahn.punkte.size(); ++i) {
            p.push_back(lp(b.bahn.punkte[i] + d));
            p.push_back(lp(b.bahn.punkte[i + 1] + d));
        }
        kartegpu::Zustand3 z;
        z.matrix = matrix;
        z.farbe[0] = r;
        z.farbe[1] = g;
        z.farbe[2] = bl;
        z.farbe[3] = 1;
        z.tiefe = kartegpu::TiefenTest::Kleiner;
        z.tiefeSchreiben = true;
        z.wegschneiden = false;
        z.breite = breite;
        kartegpu::linien(p, z);
    };
    strich(karte::Vektor3f(0, 0, 0), 1, 0, 0, 2);
    if (b.bahn.hilfslinien) {
        for (const karte::Vektor3f& d : bt::bahnKastenVersaetze()) { strich(d, 0.25F, 0.75F, 0.75F, 1); }
    }
}

void pluginKameraUeber(const float* matrix, bool fuellen) {
    const BobToolzZustand& b = e().bob;
    // DTrainDrawer: RENDER_COLOURWRITE | RENDER_DEPTHWRITE | RENDER_BLEND (ohne Tiefentest),
    // eWireframeOnly rot 1, eFullMaterials weiss 2
    if (b.splines.anzeigen) {
        std::vector<kartegpu::LinienPunkt> p;
        for (const auto& l : b.splines.linien) {
            for (std::size_t i = 0; i + 1 < l.size(); ++i) {
                p.push_back(lp(l[i]));
                p.push_back(lp(l[i + 1]));
            }
        }
        kartegpu::Zustand3 z;
        z.matrix = matrix;
        z.farbe[0] = 1;
        z.farbe[1] = fuellen ? 1.0F : 0.0F;
        z.farbe[2] = fuellen ? 1.0F : 0.0F;
        z.farbe[3] = 1;
        z.tiefe = kartegpu::TiefenTest::Aus;
        z.tiefeSchreiben = false;   // ohne GL_DEPTH_TEST schreibt OpenGL keine Tiefe
        z.wegschneiden = false;
        z.mischen = kartegpu::Mischen::Alpha;
        z.breite = fuellen ? 2.0F : 1.0F;
        kartegpu::linien(p, z);
    }
    // DVisDrawer::renderSolid: RENDER_FILL | RENDER_BLEND | RENDER_COLOURWRITE |
    // RENDER_COLOURCHANGE | RENDER_DEPTHTEST (GL_LEQUAL), GL_POLYGON_OFFSET_FILL;
    // die Flaechen in ihrer Farbe mit Alpha 0.5, die Wicklungen weiss 0.5
    if (b.vis.offen && (!b.vis.flaechen.empty() || !b.vis.wicklungen.empty())) {
        std::vector<kartegpu::Punkt3> d;
        const auto ecke = [&d](const karte::Vektor3f& v, std::uint32_t f) {
            kartegpu::Punkt3 q{};
            q.pos[0] = v[0];
            q.pos[1] = v[1];
            q.pos[2] = v[2];
            q.normale[2] = 1;
            q.farbe = f;
            d.push_back(q);
        };
        for (const bt::VisFlaeche& f : b.vis.flaechen) {
            const std::uint32_t c = karte::farbeAus(f.farbe[0], f.farbe[1], f.farbe[2], 0.5F);
            for (const unsigned i : f.indizes) { ecke(f.ecken[i], c); }
        }
        const std::uint32_t weiss = karte::farbeAus(1, 1, 1, 0.5F);
        for (const auto& w : b.vis.wicklungen) {
            for (std::size_t i = 1; i + 1 < w.size(); ++i) {   // GL_POLYGON
                ecke(w[0], weiss);
                ecke(w[i], weiss);
                ecke(w[i + 1], weiss);
            }
        }
        kartegpu::Zustand3 z;
        z.matrix = matrix;
        z.tiefe = kartegpu::TiefenTest::KleinerGleich;
        z.tiefeSchreiben = false;
        z.wegschneiden = false;
        z.mischen = kartegpu::Mischen::Alpha;
        z.versatz = true;   // GL_POLYGON_OFFSET_FILL
        kartegpu::dreiecke(d, z);
    }
}

}  // namespace bhed::gui::karteneditor
