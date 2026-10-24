// karteregion.cpp - Karten-Editor Stufe 2, Punkt 8: Auswahl und Datei wie
// NetRadiant-Custom 20260114 - Select Inside/Touching, Expand Selection To
// Primitives, Region (Off, Set XY, Set Brush, Set Selection, Save Region),
// Map Info, Find Brush, Import und Autosave.
//
// Sollwerte von Hand gerechnet:
//   - select.cpp:182-210: Touching |a.o - b.o| <= a.e + b.e je Achse,
//     Inside |a.o - b.o| <= a.e - b.e (Grenzfall gleich gilt).
//   - map.cpp:1412-1533: Region schliesst aus, was den Kasten nicht schneidet
//     (aabb_intersects_aabb: echt kleiner), worldspawn bleibt sichtbar.
//   - map.cpp:1316-1366: Save Region - Waende um min - (256,256,8) ..
//     max + (256,256,512), 32 dick; Startpunkt an der Kamera, 64 innerhalb.
//   - map.cpp:843-978: Map Info zaehlt worldspawn mit; func_group, _decal,
//     func_detail* und light*, misc_model, info_null sind nicht "ingame".
//   - autosave.cpp:118-158: nach mehr als 60 * Minuten Sekunden mit
//     geaenderter Karte; ohne Aenderung laeuft die Uhr neu an.
#include "bhed/karte/auswahlbefehle.h"
#include "bhed/karte/kartendatei.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/patchbefehle.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/region.h"
#include "bhed/karte/texdef.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
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
bool kastenNah(const Kasten& a, const Kasten& b) {
    for (std::size_t i = 0; i < 3; ++i) {
        if (!nah(a.origin[i], b.origin[i]) || !nah(a.extents[i], b.extents[i])) { return false; }
    }
    return true;
}

std::shared_ptr<Pinsel> quader(Karte& k, const std::shared_ptr<Entitaet>& ent, const Vektor3f& mitte, const Vektor3f& halb) {
    TexturProjektion proj;
    texdefStandard(TexdefArt::Quake, proj);
    auto p = std::make_shared<Pinsel>();
    p->quaderBauen(Kasten(mitte, halb), "textures/a/b", proj, k.kontext);
    ent->kindEinfuegen(p);
    return p;
}
std::shared_ptr<Pinsel> weltQuader(Karte& k, const Vektor3f& mitte, const Vektor3f& halb) {
    return quader(k, k.weltHolenOderAnlegen(), mitte, halb);
}
std::shared_ptr<Entitaet> punkt(Karte& k, const char* klasse, const char* ursprung) {
    auto e = k.neueEntitaet(klasse, false);
    e->setzen("origin", ursprung);
    k.entitaetEinfuegen(e);
    return e;
}
bool gewaehlt(const Karte& k, const std::shared_ptr<Entitaet>& e, const std::shared_ptr<Primitiv>& p = nullptr) {
    return k.auswahl.gewaehlt(e.get(), p.get());
}

// Die Szene: A [-64,64]^2 x [0,64], B daneben (beruehrt A), C weit weg,
// E halb im grossen Kasten D, ein Licht ueber A, ein Punkt in A.
struct Szene {
    Karte k;
    std::shared_ptr<Entitaet> welt;
    std::shared_ptr<Pinsel> a, b, c, e;
    std::shared_ptr<Entitaet> licht, innen;
    Szene() {
        welt = k.weltHolenOderAnlegen();
        a = weltQuader(k, Vektor3f(0, 0, 32), Vektor3f(64, 64, 32));
        b = weltQuader(k, Vektor3f(96, 0, 32), Vektor3f(32, 32, 32));     // |0-96| = 64+32: beruehrt
        c = weltQuader(k, Vektor3f(300, 0, 32), Vektor3f(16, 16, 16));
        e = weltQuader(k, Vektor3f(250, 0, 0), Vektor3f(16, 16, 16));      // 250 > 256-16: nicht ganz in D
        licht = punkt(k, "light", "0 0 128");                             // |32-128| = 96 > 32+8
        innen = punkt(k, "info_null", "0 0 40");                          // Kasten 8: in A
    }
};

void beruehrend() {
    Szene s;
    AuswahlEinstellung e;
    s.k.auswahl.waehlen(s.welt, s.a);
    const std::size_t undo = s.k.undo.undoAnzahl();
    auswahlBeruehrend(s.k, e);
    pruefe(gewaehlt(s.k, s.welt, s.a) && gewaehlt(s.k, s.welt, s.b) && gewaehlt(s.k, s.innen), "Touching: A, B (Kante an Kante) und der Punkt in A");
    pruefe(!gewaehlt(s.k, s.welt, s.c) && !gewaehlt(s.k, s.licht) && !gewaehlt(s.k, s.welt, s.e), "Touching: C, E und das Licht nicht");
    pruefe(s.k.auswahl.anzahl() == 3 && s.k.undo.undoAnzahl() == undo, "Touching: 3 gewaehlt, kein Rueckgaengig-Schritt");
    pruefe(!gewaehlt(s.k, s.welt), "Touching: worldspawn selbst nicht");
    // versteckt (Node::visible) zaehlt nicht
    s.k.auswahl.leeren();
    s.b->versteckt = true;
    s.k.auswahl.waehlen(s.welt, s.a);
    auswahlBeruehrend(s.k, e);
    pruefe(!gewaehlt(s.k, s.welt, s.b), "Touching: der versteckte B nicht");
    s.b->versteckt = false;
    // nur im Primitiv-Modus; ohne gewaehlten Pinsel nichts
    s.k.auswahl.leeren();
    s.k.auswahl.waehlen(s.welt, s.a);
    AuswahlEinstellung ent;
    ent.modus = AuswahlModus::Entitaet;
    auswahlBeruehrend(s.k, ent);
    pruefe(s.k.auswahl.anzahl() == 1, "Touching im Entity-Modus: nichts");
    s.k.auswahl.leeren();
    s.k.auswahl.waehlen(s.innen);
    auswahlBeruehrend(s.k, e);
    pruefe(s.k.auswahl.anzahl() == 1, "Touching nur mit einer Punktentitaet: nichts (nur Pinsel geben Kaesten)");
}

void innen() {
    Szene s;
    AuswahlEinstellung e;
    const auto d = weltQuader(s.k, Vektor3f(0, 0, 0), Vektor3f(256, 256, 256));
    s.k.auswahl.waehlen(s.welt, d);
    auswahlInnen(s.k, e);
    pruefe(s.k.undo.undoName() == "deleteSelected", "Inside: D geloescht mit \"deleteSelected\" (" + s.k.undo.undoName() + ")");
    pruefe(s.welt->kinder().size() == 4, "Inside: 4 Pinsel bleiben (" + std::to_string(s.welt->kinder().size()) + ")");
    pruefe(gewaehlt(s.k, s.welt, s.a) && gewaehlt(s.k, s.welt, s.b) && gewaehlt(s.k, s.innen) && gewaehlt(s.k, s.licht),
           "Inside: A, B, Punkt und Licht ganz in D");
    pruefe(!gewaehlt(s.k, s.welt, s.c) && !gewaehlt(s.k, s.welt, s.e), "Inside: C (300) und E (250 + 16 > 256) nicht");
    pruefe(s.k.auswahl.anzahl() == 4, "Inside: 4 gewaehlt (" + std::to_string(s.k.auswahl.anzahl()) + ")");
    s.k.undo.undo();
    pruefe(s.welt->kinder().size() == 5, "Inside rueckgaengig: D wieder da");
}

void aufPrimitive() {
    Szene s;
    auto gruppe = s.k.neueEntitaet("func_group", true);
    s.k.entitaetEinfuegen(gruppe);
    const auto g1 = quader(s.k, gruppe, Vektor3f(0, 500, 0), Vektor3f(8, 8, 8));
    const auto g2 = quader(s.k, gruppe, Vektor3f(0, 600, 0), Vektor3f(8, 8, 8));
    s.k.auswahl.waehlen(gruppe, g1);
    auswahlAufPrimitiveErweitern(s.k);
    pruefe(gewaehlt(s.k, gruppe, g1) && gewaehlt(s.k, gruppe, g2) && !gewaehlt(s.k, gruppe), "Ctrl+E: alle Pinsel der Gruppe, nicht die Gruppe");
    pruefe(s.k.auswahl.anzahl() == 2, "Ctrl+E: die Welt bleibt aussen vor");
    s.k.auswahl.leeren();
    s.c->versteckt = true;
    s.k.auswahl.waehlen(s.welt, s.a);
    s.k.auswahl.waehlen(s.licht);
    auswahlAufPrimitiveErweitern(s.k);
    pruefe(gewaehlt(s.k, s.welt, s.b) && gewaehlt(s.k, s.welt, s.e) && !gewaehlt(s.k, s.welt, s.c) && gewaehlt(s.k, s.licht),
           "Ctrl+E mit einem Weltpinsel: alle sichtbaren Weltpinsel, das Licht bleibt");
    pruefe(!gewaehlt(s.k, s.welt), "Ctrl+E: worldspawn selbst nicht gewaehlt");
}

void region() {
    Szene s;
    Region r;
    regionStandard(r, 65536.0F);
    s.k.auswahl.waehlen(s.welt, s.a);
    regionAuswahl(s.k, r, 65536.0F);
    pruefe(r.aktiv && nah(r.min[0], -64) && nah(r.min[2], 0) && nah(r.max[1], 64) && nah(r.max[2], 64), "Set Selection: Bereich = A");
    pruefe(s.a->sichtbar() && !s.b->sichtbar() && !s.c->sichtbar() && !s.licht->sichtbar() && !s.innen->sichtbar(),
           "Set Selection: nur A sichtbar (auch der Punkt in A nicht - er ist nicht gewaehlt)");
    pruefe(s.welt->sichtbar() && s.k.auswahl.anzahl() == 0, "Set Selection: worldspawn sichtbar, Auswahl leer");
    regionAus(s.k, r, 65536.0F);
    pruefe(!r.aktiv && s.b->sichtbar() && s.licht->sichtbar() && nah(r.max[0], 65536), "Region Off: alles sichtbar, Bereich +-65536");
    // Set XY: mit dem Standardbereich geschnitten
    regionXY(s.k, r, Vektor3f(-100, -100, -65472), Vektor3f(100, 100, 65472));
    pruefe(r.aktiv && nah(r.min[0], -100) && nah(r.max[2], 65472), "Set XY: Bereich -100..100");
    pruefe(s.a->sichtbar() && s.b->sichtbar() && !s.c->sichtbar() && !s.e->sichtbar() && s.licht->sichtbar(),
           "Set XY: A, B (96 < 32+100) und Licht sichtbar, C und E nicht");
    regionXY(s.k, r, Vektor3f(0, -1000, -1000), Vektor3f(1000, 1000, 1000));
    pruefe(nah(r.min[0], 0) && nah(r.max[0], 100), "Set XY: der neue Bereich mit dem alten geschnitten (0..100)");
    // Set XY waehlt Ausgeschlossene ab
    regionAus(s.k, r, 65536.0F);
    s.k.auswahl.waehlen(s.welt, s.c);
    s.k.auswahl.waehlen(s.welt, s.a);
    regionXY(s.k, r, Vektor3f(-100, -100, -100), Vektor3f(100, 100, 100));
    pruefe(!gewaehlt(s.k, s.welt, s.c) && gewaehlt(s.k, s.welt, s.a), "Set XY: der ausgeschlossene C ist abgewaehlt");
    // Map_RegionBounds um das Licht: worldspawn (z -16..64) schneidet nicht, bleibt aber sichtbar
    regionGrenzen(s.k, r, Kasten(Vektor3f(0, 0, 128), Vektor3f(8, 8, 8)), 65536.0F);
    pruefe(s.licht->sichtbar() && !s.a->sichtbar() && s.welt->sichtbar(), "Region um das Licht: worldspawn bleibt sichtbar (neue Pinsel)");
    // Set Brush: der zuletzt gewaehlte bestimmt den Bereich und wird geloescht
    regionAus(s.k, r, 65536.0F);
    s.k.auswahl.leeren();
    s.k.auswahl.waehlen(s.welt, s.a);
    s.k.auswahl.waehlen(s.welt, s.c);
    AuswahlEinstellung e;
    pruefe(regionPinsel(s.k, r, 65536.0F, e), "Set Brush: ausgefuehrt");
    pruefe(nah(r.min[0], 284) && nah(r.max[0], 316) && nah(r.min[2], 16) && nah(r.max[2], 48), "Set Brush: Bereich = C");
    pruefe(s.welt->kinder().size() == 3 && s.k.undo.undoName() == "deleteSelected", "Set Brush: C geloescht (\"deleteSelected\")");
    pruefe(!s.a->sichtbar() && !s.b->sichtbar() && !s.e->sichtbar() && !s.licht->sichtbar(), "Set Brush: A, B, E (234..266 < 284) und Licht ausgeschlossen");
    pruefe(s.welt->sichtbar(), "Set Brush: worldspawn bleibt sichtbar");
    s.k.auswahl.leeren();
    pruefe(!regionPinsel(s.k, r, 65536.0F, e), "Set Brush ohne Auswahl: \"Nothing is selected!\"");
    // Set Selection ohne Auswahl = Region Off
    regionAuswahl(s.k, r, 65536.0F);
    pruefe(!r.aktiv && s.a->sichtbar(), "Set Selection ohne Auswahl: Region Off");
}

void regionSpeichern() {
    // die Waende
    Kontext kon;
    std::array<std::shared_ptr<Pinsel>, 6> w;
    std::array<Pinsel*, 6> roh{};
    for (std::size_t i = 0; i < 6; ++i) {
        w[i] = std::make_shared<Pinsel>();
        roh[i] = w[i].get();
    }
    regionPinselBauen(roh, Vektor3f(-320, -320, -8), Vektor3f(320, 320, 576), "textures/", kon);
    pruefe(kastenNah(w[0]->grenzen(kon), kastenAusMinMax(Vektor3f(-352, -352, -40), Vektor3f(-320, 352, 608))), "Wand 0: -352..-320 in X");
    pruefe(kastenNah(w[3]->grenzen(kon), kastenAusMinMax(Vektor3f(320, -352, -40), Vektor3f(352, 352, 608))), "Wand 3: 320..352 in X");
    pruefe(kastenNah(w[5]->grenzen(kon), kastenAusMinMax(Vektor3f(-352, -352, 576), Vektor3f(352, 352, 608))), "Wand 5: Decke 576..608");
    pruefe(w[2]->flaechen().size() == 6 && w[2]->flaechen()[0]->shader() == "textures/", "Waende: Quader mit textures/");
    std::string warnung;
    pruefe(regionKastenShader("", &warnung) == "textures/" && warnung.empty(), "Region Box Shader leer: textures/");
    pruefe(regionKastenShader("common/caulk", &warnung) == "textures/" && warnung == "g_regionBoxShader \"common/caulk\" !texdef_name_valid()",
           "Region Box Shader ungueltig: Warnung");
    pruefe(regionKastenShader("textures/common/caulk", nullptr) == "textures/common/caulk", "Region Box Shader gueltig");
    // der Text: nur A, die sechs Waende und der Startpunkt
    Szene s;
    Region r;
    s.k.auswahl.waehlen(s.welt, s.a);
    regionAuswahl(s.k, r, 65536.0F);
    const std::size_t kinder = s.welt->kinder().size();
    const std::size_t entitaeten = s.k.entitaeten().size();
    const std::string text = regionText(s.k, r, Vektor3f(0, 0, 0), 90.7F, "textures/");
    Karte gelesen;
    (void)karteLesen(text, gelesen);
    const auto gw = gelesen.welt();
    pruefe(gw && gw->kinder().size() == 7, "Save Region: worldspawn mit A und sechs Waenden (" + std::to_string(gw ? gw->kinder().size() : 0) + ")");
    pruefe(gelesen.entitaeten().size() == 2 && gelesen.entitaeten().back()->klassenname() == "info_player_start",
           "Save Region: nur worldspawn und der Startpunkt am Ende");
    pruefe(gelesen.entitaeten().back()->wert("origin") == "0 0 56" && gelesen.entitaeten().back()->wert("angle") == "90",
           "Startpunkt: an der Kamera, auf -8 + 64 gezogen, angle (int)90.7 (" + gelesen.entitaeten().back()->wert("origin") + ")");
    pruefe(gw && kastenNah(static_cast<Pinsel&>(*gw->kinder()[1]).grenzen(gelesen.kontext),
                           kastenAusMinMax(Vektor3f(-352, -352, -40), Vektor3f(-320, 352, 608))),
           "Save Region: erste Wand hinter A");
    pruefe(s.welt->kinder().size() == kinder && s.k.entitaeten().size() == entitaeten, "Save Region: die Karte danach wie vorher");
    pruefe(text.find("\"classname\" \"light\"") == std::string::npos, "Save Region: das ausgeschlossene Licht fehlt");
}

void info() {
    Karte k;
    weltQuader(k, Vektor3f(0, 0, 0), Vektor3f(8, 8, 8));
    weltQuader(k, Vektor3f(100, 0, 0), Vektor3f(8, 8, 8));
    patchVorlage(k, kastenAusMinMax(Vektor3f(0, 0, 0), Vektor3f(64, 64, 0)), "textures/a/b", PatchVorlage::Ebene, 2);
    auto gruppe = k.neueEntitaet("func_group", true);
    k.entitaetEinfuegen(gruppe);
    quader(k, gruppe, Vektor3f(0, 300, 0), Vektor3f(8, 8, 8));
    auto ausloeser = k.neueEntitaet("trigger_multiple", true);
    k.entitaetEinfuegen(ausloeser);
    quader(k, ausloeser, Vektor3f(0, 400, 0), Vektor3f(8, 8, 8));
    auto detail = k.neueEntitaet("func_detail_x", true);
    k.entitaetEinfuegen(detail);
    quader(k, detail, Vektor3f(0, 500, 0), Vektor3f(8, 8, 8));
    punkt(k, "light", "0 0 0");
    punkt(k, "lightJunior", "0 0 0");
    punkt(k, "misc_model", "0 0 0");
    punkt(k, "info_null", "0 0 0");
    punkt(k, "info_player_start", "0 0 0");
    punkt(k, "info_player_start", "8 0 0");
    const KartenInfo i = karteInfo(k);
    pruefe(i.pinsel == 5 && i.patches == 1, "Map Info: 5 Pinsel, 1 Patch (" + std::to_string(i.pinsel) + ", " + std::to_string(i.patches) + ")");
    pruefe(i.entitaeten == 10, "Map Info: 10 Entitaeten mit worldspawn (" + std::to_string(i.entitaeten) + ")");
    pruefe(i.gruppen == 4 && i.gruppenImSpiel == 2, "Map Info: 4 Gruppen, ingame worldspawn + trigger (" + std::to_string(i.gruppenImSpiel) + ")");
    pruefe(i.imSpiel == 4, "Map Info: ingame 2 Gruppen + 2 Startpunkte (" + std::to_string(i.imSpiel) + ")");
    pruefe(i.aufschluesselung.size() == 9 && i.aufschluesselung.at("info_player_start") == 2 && i.aufschluesselung.begin()->first == "func_detail_x",
           "Map Info: Aufschluesselung nach classname");
}

void finden() {
    Szene s;
    int ent = -1;
    int pin = -1;
    auswahlNummern(s.k, ent, pin);
    pruefe(ent == 0 && pin == 0, "Find: ohne Auswahl 0, 0");
    s.k.auswahl.waehlen(s.welt, s.a);
    s.k.auswahl.waehlen(s.welt, s.c);
    auswahlNummern(s.k, ent, pin);
    pruefe(ent == 0 && pin == 2, "Find: der zuletzt gewaehlte C = Entitaet 0, Pinsel 2");
    s.k.auswahl.waehlen(s.innen);
    auswahlNummern(s.k, ent, pin);
    pruefe(ent == 2 && pin == 0, "Find: der Punkt = Entitaet 2, Pinsel 0");
    s.k.auswahl.leeren();
    Vektor3f mitte;
    pruefe(pinselFinden(s.k, 0, 1, &mitte) && gewaehlt(s.k, s.welt, s.b) && nah(mitte[0], 96) && nah(mitte[2], 32), "Find 0/1: B gewaehlt, Mitte 96 0 32");
    pruefe(pinselFinden(s.k, 0, 3) && s.k.auswahl.anzahl() == 2, "Find 0/3: E dazu (die Auswahl bleibt)");
    pruefe(!pinselFinden(s.k, 0, 9) && !pinselFinden(s.k, 7, 0), "Find: ausserhalb nichts");
    pruefe(pinselFinden(s.k, 1, 5, &mitte) && gewaehlt(s.k, s.licht) && nah(mitte[2], 128), "Find 1/x: das Licht ohne Kinder");
}

void importieren() {
    Karte quelle;
    weltQuader(quelle, Vektor3f(0, 0, 500), Vektor3f(8, 8, 8));
    punkt(quelle, "light", "0 0 600");
    const std::string text = karteSchreiben(quelle);
    Szene s;
    s.k.auswahl.waehlen(s.welt, s.a);
    karteImportieren(s.k, text);
    pruefe(s.k.undo.undoName() == "mapImport", "Import: Rueckgaengig \"mapImport\"");
    pruefe(s.welt->kinder().size() == 5 && s.k.entitaeten().size() == 4, "Import: der Pinsel in worldspawn, das Licht dazu");
    pruefe(s.k.auswahl.anzahl() == 3 && gewaehlt(s.k, s.welt, s.a), "Import: die vorige Auswahl bleibt, das Neue dazu");
    s.k.undo.undo();
    pruefe(s.welt->kinder().size() == 4 && s.k.entitaeten().size() == 3, "Import rueckgaengig");
}

void autosave() {
    AutoSpeichern a;
    using A = AutoSpeichern::Aktion;
    pruefe(a.pruefen(1000, 0, false) == A::Nichts, "Autosave: Start");
    pruefe(a.pruefen(1000 + 901, 0, false) == A::Nichts, "Autosave: ohne Aenderung nichts (Uhr neu)");
    pruefe(a.pruefen(2000, 1, false) == A::Nichts, "Autosave: geaendert, erst 99 s");
    pruefe(a.pruefen(1901 + 900, 1, false) == A::Nichts, "Autosave: genau 900 s noch nicht (echt groesser)");
    pruefe(a.pruefen(1901 + 901, 1, false) == A::Speichern, "Autosave: nach 901 s speichern");
    pruefe(a.pruefen(1901 + 901 + 1000, 1, false) == A::Nichts, "Autosave: danach ohne neue Aenderung nichts");
    AutoSpeichern b;
    b.schnappschuesse = true;
    pruefe(b.pruefen(100, 0, false) == A::Nichts && b.pruefen(150, 1, false) == A::Nichts && b.pruefen(1001, 1, false) == A::Schnappschuss,
           "Autosave: Schnappschuss");
    AutoSpeichern c;
    c.schnappschuesse = true;
    pruefe(c.pruefen(100, 0, true) == A::Nichts && c.pruefen(1001, 1, true) == A::Speichern, "Autosave: unbenannt nie Schnappschuss");
    AutoSpeichern d;
    d.an = false;
    d.minuten = 1;
    pruefe(d.pruefen(100, 0, false) == A::Nichts && d.pruefen(160, 1, false) == A::Nichts && d.pruefen(161, 1, false) == A::Uebersprungen,
           "Autosave aus, 1 Minute: \"Autosave skipped...\" nach 61 s");
    pruefe(autosavePfad("C:/maps/test.map") == "C:/maps/test.autosave.map", "autosave: name.autosave.map");
    pruefe(autosavePfad("C:\\m.d\\test") == "C:\\m.d\\test.autosave", "autosave: ohne Endung (der Punkt im Ordner zaehlt nicht)");
    pruefe(schnappschussOrdner("C:/maps/test.map") == "C:/maps/snapshots", "snapshots-Ordner");
    namespace fs = std::filesystem;
    const fs::path tmp = fs::temp_directory_path() / "bhed_karteregion_test";
    std::error_code ec;
    fs::remove_all(tmp, ec);
    fs::create_directories(tmp / "snapshots", ec);
    { std::ofstream(tmp / "snapshots" / "x.0.map") << "12345"; }
    { std::ofstream(tmp / "snapshots" / "x.1.map") << "123"; }
    std::uintmax_t summe = 0;
    const std::string karte = (tmp / "x.map").generic_string();
    const std::string pfad = schnappschussPfad(karte, &summe);
    pruefe(pfad == (tmp / "x.map").generic_string().substr(0, karte.size() - 5) + "snapshots/x.2.map" && summe == 8,
           "Snapshot: der erste freie (x.2.map), Summe 8 Byte (" + pfad + ")");
    fs::remove_all(tmp, ec);
}

}  // namespace

int main() {
    beruehrend();
    innen();
    aufPrimitive();
    region();
    regionSpeichern();
    info();
    finden();
    importieren();
    autosave();
    std::printf("karteregion: %d Fehler\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
