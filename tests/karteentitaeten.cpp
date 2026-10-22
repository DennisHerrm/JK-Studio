// karteentitaeten.cpp - Karten-Editor Stufe 1b, Punkt 9: Entitaeten ohne
// Oberflaeche (bhed/karte/entitaetbefehle.h).
//
// Geprueft wird, was der Entity Inspector, das Entitaetenmenue der 2D-Ansicht
// und das Entity-Menue von NRC tun: Schluessel der Auswahl (der erste Wert
// gewinnt, Vorgaben der Klasse), Schluessel setzen/loeschen/alle loeschen,
// Spawnflags, Klasse wechseln (Punkt -> Punkt, Welt-Pinsel -> Gruppe, Gruppe ->
// Punkt, Punkt -> Gruppe mit Platzhalter-Quader, Gruppe -> Gruppe, worldspawn
// = Gruppe aufloesen), Entitaet erzeugen (Punkt am Ursprung, Gruppe ohne
// Pinsel = Fehler, trigger_ mit dem Trigger-Shader, light/misc_model mit
// offenem Rueckgaengig-Schritt), Primitive umhaengen, Verbinden (target /
// targetname, eindeutiger Name), Farbe setzen/normieren, Auswahl nach
// Schluessel/Wert, verbundene Entitaeten, Auswahl auf Entitaeten erweitern und
// die Gruppierung des Entitaetenmenues. Rueckgaengig-Namen wie NRC.
#include "bhed/karte/eklasse.h"
#include "bhed/karte/entitaetbefehle.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texdef.h"

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

const char* const kEnt =
    "<?xml version=\"1.0\"?>\n<classes>\n"
    "<group name=\"worldspawn\" color=\"0 0 0\">\n</group>\n"
    "<group name=\"func_group\" color=\"0 .4 1\">\n</group>\n"
    "<group name=\"func_door\" color=\"0 .5 .8\">\n<real key=\"speed\" name=\"Speed\" value=\"100\">units</real>\n"
    "<flag key=\"START_OPEN\" name=\"Start Open\" bit=\"0\">open</flag>\n</group>\n"
    "<group name=\"trigger_multiple\" color=\".5 .5 .5\">\n</group>\n"
    "<point name=\"info_null\" color=\"0 .5 0\" box=\"-4 -4 -4 4 4 4\">\n</point>\n"
    "<point name=\"info_notnull\" color=\"0 .5 0\" box=\"-4 -4 -4 4 4 4\">\n</point>\n"
    "<point name=\"light\" color=\"0 1 0\" box=\"-8 -8 -8 8 8 8\">\n<real key=\"light\" name=\"Intensity\" value=\"300\">i</real>\n"
    "<color key=\"_color\" name=\"Color\" value=\"1 1 1\">c</color>\n</point>\n"
    "<point name=\"misc_model\" color=\"1 .5 .25\" box=\"-16 -16 -16 16 16 16\">\n<model key=\"model\" name=\"Model\">m</model>\n</point>\n"
    "</classes>\n";

std::shared_ptr<Pinsel> quader(Karte& k, const std::shared_ptr<Entitaet>& e, const Vektor3f& mitte) {
    TexturProjektion proj;
    texdefStandard(TexdefArt::Quake, proj);
    auto p = std::make_shared<Pinsel>();
    p->quaderBauen(Kasten(mitte, Vektor3f(16, 16, 16)), "textures/stein/a", proj, k.kontext);
    e->kindEinfuegen(p);
    return p;
}

std::shared_ptr<Entitaet> punkt(Karte& k, const char* klasse, const char* ursprung) {
    auto e = k.neueEntitaet(klasse, false);
    e->setzen("origin", ursprung);
    k.entitaetEinfuegen(e);
    return e;
}

std::shared_ptr<Entitaet> finde(const Karte& k, const std::string& klasse, std::size_t nummer = 0) {
    for (const auto& e : k.entitaeten()) {
        if (e->klassenname() == klasse && nummer-- == 0) { return e; }
    }
    return nullptr;
}

std::size_t zahl(const Karte& k, const std::string& klasse) {
    std::size_t n = 0;
    for (const auto& e : k.entitaeten()) { n += e->klassenname() == klasse ? 1U : 0U; }
    return n;
}

struct Welt {
    EntitaetsKlassen klassen;
    Karte k;
    std::shared_ptr<Entitaet> welt;
    Welt() {
        (void)klassen.entLesen(kEnt, "test.ent");
        k.klassen = &klassen;
        welt = k.weltHolenOderAnlegen();
        k.anmelden();
    }
};

// --- Entity Inspector ---------------------------------------------------------------

void inspektor() {
    Welt w;
    Karte& k = w.k;
    auto a = punkt(k, "light", "0 0 0");
    auto b = punkt(k, "light", "64 0 0");
    a->setzen("light", "500");
    b->setzen("light", "200");
    b->setzen("target", "t1");
    auto tuer = k.neueEntitaet("func_door", true);
    k.entitaetEinfuegen(tuer);
    auto tp = quader(k, tuer, Vektor3f(0, 128, 0));

    k.auswahl.waehlen(a);
    k.auswahl.waehlen(b);
    GewaehlteSchluessel g = gewaehlteSchluessel(k);
    pruefe(g.werte["light"] == "500" && g.werte["target"] == "t1" && g.werte["classname"] == "light",
           "Entity_GetKeyValues_Selected: der erste Wert gewinnt, Schluessel aller Gewaehlten");
    pruefe(g.wert("_color") == "1 1 1" && g.wert("light") == "500" && g.wert("fehlt").empty(),
           "SelectedEntity_getValueForKey: Wert, sonst Vorgabe der Klasse, sonst leer");

    schluesselSetzenRueckgaengig(k, "_color", "1 0 0");
    pruefe(a->wert("_color") == "1 0 0" && b->wert("_color") == "1 0 0", "entitySetKeyValue auf alle Gewaehlten");
    pruefe(k.undo.undoName() == "entitySetKeyValue -key \"_color\" -value \"1 0 0\"", "Rueckgaengig-Name entitySetKeyValue");

    // ein gewaehltes Primitiv meint seine Entitaet
    k.auswahl.leeren();
    k.auswahl.waehlen(tuer, tp);
    schluesselSetzenRueckgaengig(k, "speed", "50");
    pruefe(tuer->wert("speed") == "50" && !w.welt->hat("speed"), "Primitiv gewaehlt: Schluessel an seiner Entitaet");
    g = gewaehlteSchluessel(k);
    pruefe(g.werte["classname"] == "func_door" && g.vorgaben["speed"] == "100", "Schluessel ueber das gewaehlte Primitiv");

    schluesselLoeschenAuswahl(k, "classname");
    pruefe(tuer->wert("classname") == "func_door", "classname laesst sich nicht loeschen");
    schluesselLoeschenAuswahl(k, "speed");
    pruefe(!tuer->hat("speed") && k.undo.undoName() == "entityDeleteKey -key speed", "entityDeleteKey");
    k.undo.undo();
    pruefe(tuer->wert("speed") == "50", "Rueckgaengig: speed wieder da");

    spawnflagsSetzen(k, 5);
    pruefe(tuer->wert("spawnflags") == "5" && k.undo.undoName() == "entitySetSpawnflags -flags 5", "entitySetSpawnflags 5");
    spawnflagsSetzen(k, 0);
    pruefe(!tuer->hat("spawnflags") && k.undo.undoName() == "entitySetSpawnflags -flags 0", "Spawnflags 0 loescht den Schluessel");

    tuer->setzen("origin", "1 2 3");
    alleSchluesselLoeschen(k, gewaehlteSchluessel(k));
    pruefe(tuer->wert("classname") == "func_door" && tuer->wert("origin") == "1 2 3" && !tuer->hat("speed") &&
               k.undo.undoName() == "entityClear",
           "entityClear: alles ausser classname und origin");
}

// --- Klasse wechseln (Scene_EntitySetClassname_Selected) -----------------------------

void klasseWechseln() {
    {   // Punkt -> Punkt: Schluessel kopiert, neue Entitaet am Ende
        Welt w;
        Karte& k = w.k;
        auto a = punkt(k, "info_null", "8 16 24");
        a->setzen("targetname", "x");
        k.auswahl.waehlen(a);
        klasseSetzenAuswahl(k, "info_notnull");
        auto n = finde(k, "info_notnull");
        pruefe(zahl(k, "info_null") == 0 && n && n->wert("origin") == "8 16 24" && n->wert("targetname") == "x" &&
                   n->schluessel().front().first == "classname" && n->wert("classname") == "info_notnull",
               "Punkt -> Punkt: Schluessel kopiert, classname zuerst");
        pruefe(k.undo.undoName() == "entitySetClass -class info_notnull" && k.auswahl.anzahl() == 0,
               "entitySetClass; die neue Entitaet ist nicht gewaehlt (neue Instanz)");
        k.undo.undo();
        pruefe(zahl(k, "info_null") == 1 && zahl(k, "info_notnull") == 0, "Rueckgaengig: die alte Entitaet ist zurueck");
    }
    {   // Welt-Pinsel -> Gruppe
        Welt w;
        Karte& k = w.k;
        auto p1 = quader(k, w.welt, Vektor3f(0, 0, 0));
        auto p2 = quader(k, w.welt, Vektor3f(64, 0, 0));
        (void)quader(k, w.welt, Vektor3f(128, 0, 0));
        k.auswahl.waehlen(w.welt, p1);
        k.auswahl.waehlen(w.welt, p2);
        klasseSetzenAuswahl(k, "func_door");
        auto t = finde(k, "func_door");
        pruefe(t && t->kinder().size() == 2 && t->kinder()[0] == p1 && t->kinder()[1] == p2 && w.welt->kinder().size() == 1,
               "Welt-Pinsel -> func_door: die gewaehlten Pinsel wandern in die neue Gruppe");
        pruefe(k.auswahl.anzahl() == 2 && k.auswahl.gewaehlt(t.get(), p1.get()) && k.auswahl.gewaehlt(t.get(), p2.get()),
               "Welt-Pinsel -> Gruppe: die Kinder der neuen Gruppe sind gewaehlt");
        // Welt-Pinsel -> Punktklasse: nichts
        k.auswahl.leeren();
        k.auswahl.waehlen(w.welt, w.welt->kinder()[0]);
        const std::size_t vorher = k.entitaeten().size();
        klasseSetzenAuswahl(k, "info_null");
        pruefe(k.entitaeten().size() == vorher && w.welt->kinder().size() == 1, "Welt-Pinsel -> Punktklasse: keine Aenderung");
    }
    {   // Gruppe -> Punkt: origin = Mitte der Pinsel
        Welt w;
        Karte& k = w.k;
        auto t = k.neueEntitaet("func_door", true);
        k.entitaetEinfuegen(t);
        (void)quader(k, t, Vektor3f(0, 0, 0));
        (void)quader(k, t, Vektor3f(64, 0, 32));
        t->setzen("targetname", "tuer");
        k.auswahl.waehlen(t);
        klasseSetzenAuswahl(k, "info_null");
        auto n = finde(k, "info_null");
        pruefe(zahl(k, "func_door") == 0 && n && n->wert("origin") == "32 0 16" && n->wert("targetname") == "tuer" && n->kinder().empty(),
               "Gruppe -> Punkt: origin = Mitte der Grenzen, Pinsel weg");
    }
    {   // Punkt -> Gruppe: Platzhalter-Quader 16 um origin, origin geloescht
        Welt w;
        Karte& k = w.k;
        auto a = punkt(k, "info_null", "100 200 300");
        k.auswahl.waehlen(a);
        klasseSetzenAuswahl(k, "func_door");
        auto t = finde(k, "func_door");
        bool quaderOk = false;
        if (t && t->kinder().size() == 1 && t->kinder()[0]->art() == Primitiv::Art::Pinsel) {
            const auto& p = static_cast<const Pinsel&>(*t->kinder()[0]);
            const Kasten& b = p.grenzen(k.kontext);
            quaderOk = b.origin == Vektor3f(100, 200, 300) && b.extents == Vektor3f(16, 16, 16) && p.flaechen().size() == 6 &&
                       p.flaechen()[0]->shader() == "textures/";
        }
        pruefe(quaderOk && !t->hat("origin"), "Punkt -> Gruppe: Quader 32 gross um origin mit \"textures/\", origin geloescht");
    }
    {   // Gruppe -> Gruppe und worldspawn = Gruppe aufloesen
        Welt w;
        Karte& k = w.k;
        auto t = k.neueEntitaet("func_door", true);
        k.entitaetEinfuegen(t);
        auto p = quader(k, t, Vektor3f(0, 0, 0));
        k.auswahl.waehlen(t, p);
        klasseSetzenAuswahl(k, "func_group");
        auto g = finde(k, "func_group");
        pruefe(zahl(k, "func_door") == 0 && g && g->kinder().size() == 1 && g->kinder()[0] == p, "Gruppe -> Gruppe: Pinsel umgehaengt");
        k.auswahl.waehlen(g, p);
        klasseSetzenAuswahl(k, "worldspawn");
        pruefe(zahl(k, "func_group") == 0 && w.welt->kinder().size() == 1 && w.welt->kinder()[0] == p &&
                   k.undo.undoName() == "ungroupSelectedEntities",
               "worldspawn: Gruppe aufgeloest, Pinsel in der Welt (ungroupSelectedEntities)");
        k.undo.undo();
        pruefe(zahl(k, "func_group") == 1 && w.welt->kinder().empty(), "Rueckgaengig: Gruppe wieder da");
    }
    {   // nichts gewaehlt: kein Befehl
        Welt w;
        const std::size_t n = w.k.undo.undoAnzahl();
        klasseSetzenAuswahl(w.k, "info_null");
        pruefe(w.k.undo.undoAnzahl() == n, "ohne Auswahl kein Befehl");
    }
}

// --- Entitaet erzeugen (Entity_createFromSelection) ----------------------------------

void erzeugen() {
    {
        Welt w;
        Karte& k = w.k;
        auto p = quader(k, w.welt, Vektor3f(0, 0, 0));
        k.auswahl.waehlen(w.welt, p);
        ErzeugtErgebnis r = entitaetErzeugen(k, "info_null", Vektor3f(8, 16, -24));
        pruefe(r.entitaet && r.entitaet->wert("origin") == "8 16 -24" && !r.offen && k.undo.undoName() == "entityCreate -class info_null",
               "Punkt: origin gesetzt, entityCreate -class info_null");
        pruefe(k.auswahl.anzahl() == 1 && k.auswahl.gewaehlt(r.entitaet.get()), "Punkt: nur die neue Entitaet gewaehlt");
        pruefe(w.welt->kinder().size() == 1, "Punkt: die gewaehlten Pinsel bleiben in der Welt");
    }
    {
        Welt w;
        Karte& k = w.k;
        const std::size_t vorher = k.entitaeten().size();
        ErzeugtErgebnis r = entitaetErzeugen(k, "func_door", Vektor3f(0, 0, 0));
        pruefe(!r.entitaet && r.fehler == "failed to create a group entity - no brushes are selected" && k.entitaeten().size() == vorher,
               "Gruppe ohne Pinsel: Fehlermeldung, nichts erzeugt");
    }
    {
        Welt w;
        Karte& k = w.k;
        auto p = quader(k, w.welt, Vektor3f(0, 0, 0));
        k.auswahl.waehlen(w.welt, p);
        ErzeugtErgebnis r = entitaetErzeugen(k, "trigger_multiple", Vektor3f(0, 0, 0));
        pruefe(r.entitaet && r.entitaet->kinder().size() == 1 && p->flaechen()[0]->shader() == "textures/system/trigger" &&
                   w.welt->kinder().empty() && k.auswahl.gewaehlt(r.entitaet.get(), p.get()),
               "trigger_: Pinsel umgehaengt, Trigger-Shader (GetCommonShader), Kinder gewaehlt");
        k.undo.undo();
        pruefe(p->flaechen()[0]->shader() == "textures/stein/a" && w.welt->kinder().size() == 1 && zahl(k, "trigger_multiple") == 0,
               "Rueckgaengig: Shader und Pinsel zurueck");
    }
    {
        Welt w;
        Karte& k = w.k;
        ErzeugtErgebnis r = entitaetErzeugen(k, "light", Vektor3f(0, 0, 0));
        pruefe(r.entitaet && r.lichtFragen && r.offen && !r.modellFragen, "light: nach der Lichtstaerke fragen, Schritt offen");
        r.entitaet->setzen("light", "300");
        befehlAbschliessen(k, r);
        pruefe(!r.offen && k.undo.undoName() == "entityCreate -class light", "light: Schritt mit dem Schluessel abgeschlossen");
        k.undo.undo();
        pruefe(zahl(k, "light") == 0, "Rueckgaengig: Licht samt light-Schluessel weg");
        ErzeugtErgebnis m = entitaetErzeugen(k, "misc_model", Vektor3f(0, 0, 0));
        pruefe(m.entitaet && m.modellFragen && m.offen, "misc_model ohne Vorgabe fuer model: nach dem Modell fragen");
        befehlAbschliessen(k, m);
    }
    {
        Welt w;
        Karte& k = w.k;
        auto t = k.neueEntitaet("func_door", true);
        k.entitaetEinfuegen(t);
        auto p1 = quader(k, t, Vektor3f(0, 0, 0));
        auto p2 = quader(k, t, Vektor3f(64, 0, 0));
        k.auswahl.waehlen(t, p1);
        (void)primitiveZurWelt(k);
        pruefe(w.welt->kinder().size() == 1 && t->kinder().size() == 1 && k.undo.undoName() == "ungroupSelectedPrimitives",
               "Entity_ungroupSelectedPrimitives: das Primitiv in die Welt");
        k.auswahl.waehlen(t, p2);
        (void)primitiveZurWelt(k);
        pruefe(zahl(k, "func_door") == 0 && w.welt->kinder().size() == 2, "leer gewordene Gruppe geloescht");
    }
}

// --- Umhaengen, Verbinden, Farbe ---------------------------------------------------

void umhaengenVerbinden() {
    {
        Welt w;
        Karte& k = w.k;
        auto t = k.neueEntitaet("func_door", true);
        k.entitaetEinfuegen(t);
        auto tp = quader(k, t, Vektor3f(0, 0, 0));
        auto p = quader(k, w.welt, Vektor3f(64, 0, 0));
        std::string f;
        k.auswahl.waehlen(w.welt, p);
        pruefe(!primitiveZurEntitaet(k, true, &f) && f == "Source and target entity primitives should be selected!",
               "Move Primitives: weniger als zwei -> Fehler");
        k.auswahl.waehlen(t, tp);
        pruefe(primitiveZurEntitaet(k, true) && t->kinder().size() == 2 && w.welt->kinder().empty() &&
                   k.undo.undoName() == "movePrimitivesToEntity \"func_door\"",
               "EntityMovePrimitivesToLast: in die Gruppe des zuletzt Gewaehlten");
        k.auswahl.leeren();
        k.auswahl.waehlen(t, tp);
        auto q = quader(k, w.welt, Vektor3f(128, 0, 0));
        k.auswahl.waehlen(w.welt, q);
        pruefe(primitiveZurEntitaet(k, false) && t->kinder().size() == 3, "EntityMovePrimitivesToFirst: in die des zuerst Gewaehlten");
    }
    {
        Welt w;
        Karte& k = w.k;
        auto a = punkt(k, "info_null", "0 0 0");
        auto b = punkt(k, "info_null", "64 0 0");
        std::string f;
        k.auswahl.waehlen(a);
        pruefe(!entitaetenVerbinden(k, 0, &f) && f == "entityConnectSelected: exactly two instances must be selected", "Verbinden: nur eine");
        k.auswahl.waehlen(b);
        pruefe(entitaetenVerbinden(k, 0) && a->wert("target") == "info_null1" && b->wert("targetname") == "info_null1" &&
                   k.undo.undoName() == "entityConnectSelected",
               "Verbinden: neuer Name <classname>1");
        auto c = punkt(k, "info_null", "128 0 0");
        k.auswahl.leeren();
        k.auswahl.waehlen(b);
        k.auswahl.waehlen(c);
        pruefe(entitaetenVerbinden(k, 0) && b->wert("target") == "info_null2" && c->wert("targetname") == "info_null2",
               "Verbinden: makeUnique -> info_null2");
        auto d = punkt(k, "info_null", "0 64 0");
        k.auswahl.leeren();
        k.auswahl.waehlen(a);
        k.auswahl.waehlen(d);
        pruefe(entitaetenVerbinden(k, 0) && a->wert("target") == "info_null1" && d->wert("targetname") == "info_null1",
               "Verbinden: der vorhandene target-Wert hat Vorrang");
        k.auswahl.leeren();
        auto t = k.neueEntitaet("func_door", true);
        k.entitaetEinfuegen(t);
        auto t1 = quader(k, t, Vektor3f(0, 0, 0));
        auto t2 = quader(k, t, Vektor3f(64, 0, 0));
        k.auswahl.waehlen(t, t1);
        k.auswahl.waehlen(t, t2);
        pruefe(!entitaetenVerbinden(k, 0, &f) && f == "entityConnectSelected: the selected instances must not both be from the same entity",
               "Verbinden: zwei Pinsel derselben Entitaet");
    }
    {
        pruefe(farbeNormiert(Vektor3f(2, 1, 0)) == Vektor3f(1, 0.5F, 0) && farbeNormiert(Vektor3f(0, 0, 0)) == Vektor3f(1, 1, 1),
               "NormalizeColor");
        Welt w;
        Karte& k = w.k;
        auto l = punkt(k, "light", "0 0 0");
        k.auswahl.waehlen(l);
        pruefe(!farbeNormierenAuswahl(k), "Normalize ohne _color: nichts");
        farbeSetzenAuswahl(k, Vektor3f(0.5F, 0.25F, 0));
        pruefe(l->wert("_color") == "0.5 0.25 0" && k.undo.undoName() == "entitySetColour 0.5 0.25 0", "entitySetColour");
        Vektor3f n;
        pruefe(farbeNormierenAuswahl(k, &n) && l->wert("_color") == "1 0.5 0" && n == Vektor3f(1, 0.5F, 0) &&
                   k.undo.undoName() == "entityNormalizeColour 1 0.5 0",
               "entityNormalizeColour");
    }
}

// --- Auswahl ---------------------------------------------------------------------------

void auswahl() {
    Welt w;
    Karte& k = w.k;
    auto a = punkt(k, "info_null", "0 0 0");
    auto b = punkt(k, "info_null", "64 0 0");
    auto c = punkt(k, "light", "128 0 0");
    a->setzen("target", "Ziel");
    b->setzen("targetname", "Ziel");
    b->setzen("target", "weiter");
    c->setzen("targetname", "weiter");
    auto t = k.neueEntitaet("func_door", true);
    k.entitaetEinfuegen(t);
    auto t1 = quader(k, t, Vektor3f(0, 0, 0));
    auto t2 = quader(k, t, Vektor3f(64, 0, 0));
    t->setzen("targetname", "ZIEL");
    w.welt->setzen("message", "ziel");

    waehleNachSchluesselWert(k, "targetname", nullptr);
    pruefe(k.auswahl.anzahl() == 5 && k.auswahl.gewaehlt(b.get()) && k.auswahl.gewaehlt(c.get()) && k.auswahl.gewaehlt(t.get()) &&
               k.auswahl.gewaehlt(t.get(), t1.get()) && k.auswahl.gewaehlt(t.get(), t2.get()),
           "Select by key: Entitaeten mit dem Schluessel, Gruppen samt Pinseln");
    waehleNachSchluesselWert(k, nullptr, "ziel");
    pruefe(k.auswahl.anzahl() == 5 && k.auswahl.gewaehlt(a.get()) && !k.auswahl.gewaehlt(w.welt.get()),
           "Select by value: ohne Gross/klein, worldspawn nie");
    waehleNachSchluesselWert(k, "targetname", "ziel");
    pruefe(k.auswahl.anzahl() == 4 && k.auswahl.gewaehlt(b.get()) && k.auswahl.gewaehlt(t.get()), "Select by key + value");
    waehleNachSchluesselWert(k, "", nullptr);
    pruefe(k.auswahl.anzahl() == 0, "leerer Schluessel: nur abgewaehlt");

    std::string f;
    k.auswahl.waehlen(b);
    pruefe(waehleVerbundene(k, true, false) && k.auswahl.anzahl() == 1 && k.auswahl.gewaehlt(a.get()), "Select targeting entities");
    k.auswahl.leeren();
    k.auswahl.waehlen(b);
    pruefe(waehleVerbundene(k, false, true) && k.auswahl.anzahl() == 1 && k.auswahl.gewaehlt(c.get()), "Select targets");
    k.auswahl.leeren();
    k.auswahl.waehlen(b);
    pruefe(waehleVerbundene(k, true, true) && k.auswahl.anzahl() == 3 && k.auswahl.gewaehlt(a.get()) && k.auswahl.gewaehlt(b.get()) &&
               k.auswahl.gewaehlt(c.get()),
           "Select connected: beide Richtungen, die Auswahl bleibt");
    k.auswahl.leeren();
    k.auswahl.waehlen(t, t1);
    pruefe(!waehleVerbundene(k, false, true, &f) && f == "SelectConnectedEntities: nothing found", "nothing found");

    k.auswahl.leeren();
    k.auswahl.waehlen(t, t1);
    auswahlAufEntitaetenErweitern(k);
    pruefe(k.auswahl.anzahl() == 3 && k.auswahl.gewaehlt(t.get()) && k.auswahl.gewaehlt(t.get(), t2.get()),
           "Expand Selection To Entities: Gruppe und alle Pinsel");
    auto wp1 = quader(k, w.welt, Vektor3f(0, 0, 0));
    auto wp2 = quader(k, w.welt, Vektor3f(64, 0, 0));
    k.auswahl.leeren();
    k.auswahl.waehlen(w.welt, wp1);
    auswahlAufEntitaetenErweitern(k);
    pruefe(k.auswahl.anzahl() == 2 && !k.auswahl.gewaehlt(w.welt.get()) && k.auswahl.gewaehlt(w.welt.get(), wp2.get()),
           "Expand Selection: worldspawn selbst nicht, aber alle seine Pinsel");
}

// --- Entitaetenmenue (EntityClassMenuInserter) ----------------------------------------

void menue() {
    const std::vector<std::string> klassen{"_decal", "ammo_blaster", "ammo_detpack", "func_door", "info_null", "info_player_start",
                                           "light", "misc_model", "misc_turret", "worldspawn"};
    const auto m = entitaetMenue(klassen);
    const bool ok = m.size() == 7 && m[0].name == "_decal" && m[0].kinder.empty() && m[1].name == "ammo" &&
                    m[1].kinder == std::vector<std::string>{"ammo_blaster", "ammo_detpack"} && m[2].name == "func_door" &&
                    m[3].name == "info" && m[3].kinder.size() == 2 && m[4].name == "light" && m[5].name == "misc" &&
                    m[5].kinder.size() == 2 && m[6].name == "worldspawn";
    pruefe(ok, "Menue: gleicher Anfang bis '_' mit dem naechsten -> Untermenue, '_' vorn nicht, einzelne bleiben oben");
    const auto m2 = entitaetMenue({"target_a", "target_b", "team_x", "team_y", "trigger_one"});
    pruefe(m2.size() == 3 && m2[0].name == "target" && m2[1].name == "team" && m2[1].kinder.size() == 2 && m2[2].name == "trigger_one",
           "Menue: Untermenue wechselt, letzte Einzelklasse oben");
}

}  // namespace

int main() {
    inspektor();
    klasseWechseln();
    erzeugen();
    umhaengenVerbinden();
    auswahl();
    menue();
    std::printf("%d Fehler\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
