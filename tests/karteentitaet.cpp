// karteentitaet.cpp - Karten-Editor Stufe 1a, Punkt 2d: Entitaeten, Kartenbaum,
// Auswahl, Rueckgaengig (bhed/karte/karte.h, eklasse.h).
//
// Geprueft wird NRCs Verhalten:
//   - EntityKeyValues: Reihenfolge des Einfuegens, Ueberschreiben an Ort und
//     Stelle, leerer Wert loescht, Schluessel mit Gross/klein unterschieden,
//     gesichert nur bei echter Aenderung (KeyValue::assign);
//   - node_for_eclass: "classname" steht immer zuerst;
//   - getClassName ist der Klassenname, isContainer = !fixedsize;
//   - Eclass_ForName: ohne Gross/klein, Ersatzklasse 16er-Wuerfel ohne Pinsel,
//     leerer Name -> UNKNOWN_CLASS;
//   - worldspawn suchen und als erstes Kind anlegen;
//   - Kinder und Entitaetenliste mit Rueckgaengig/Wiederholen;
//   - Auswahl in Reihenfolge, Flaechen, Aufraeumen geloeschter Objekte.
#include "bhed/karte/karte.h"

#include <cstdio>
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

std::string reihenfolge(const Entitaet& e) {
    std::string s;
    for (const auto& [k, v] : e.schluessel()) { s += k + "=" + v + ";"; }
    return s;
}

}  // namespace

int main() {
    // --- Klassenbestand --------------------------------------------------
    {
        EntitaetsKlassen kl;
        auto licht = std::make_unique<EntitaetsKlasse>();
        licht->setzeName("light");
        licht->fixedsize = true;
        licht->mins = Vektor3f(-8, -8, -8);
        licht->maxs = Vektor3f(8, 8, 8);
        pruefe(kl.einfuegen(std::move(licht)), "Klasse einfuegen");
        auto zweite = std::make_unique<EntitaetsKlasse>();
        zweite->setzeName("LIGHT");
        pruefe(!kl.einfuegen(std::move(zweite)), "gleicher Name ohne Gross/klein wird verworfen (EClass_InsertSortedList)");
        pruefe(kl.findenOderEinfuegen("Light", false)->name == "light", "Eclass_ForName ohne Gross/klein -> vorhandene Klasse");
        EntitaetsKlasse* neu = kl.findenOderEinfuegen("func_unbekannt", true);
        pruefe(!neu->fixedsize && neu->kommentar == "Not found in source." && neu->farbe == Vektor3f(0, 0.5f, 0),
               "Ersatzklasse mit Pinseln: Gruppe, gruen, 'Not found in source.'");
        EntitaetsKlasse* punkt = kl.findenOderEinfuegen("target_unbekannt", false);
        pruefe(punkt->fixedsize && punkt->mins == Vektor3f(-8, -8, -8) && punkt->maxs == Vektor3f(8, 8, 8),
               "Ersatzklasse ohne Pinsel: fester Kasten -8..8 (smallbox)");
        pruefe(kl.findenOderEinfuegen("", true)->name == "UNKNOWN_CLASS", "leerer Name -> UNKNOWN_CLASS");
        EntitaetsKlasse m;
        m.setzeName("misc_gamemodel");
        EntitaetsKlasse m2;
        m2.setzeName("model_static");
        EntitaetsKlasse m3;
        m3.setzeName("misc_model_breakable");   // endet nicht auf "model"
        pruefe(m.miscmodelIs && m2.miscmodelIs && !m3.miscmodelIs,
               "name_set: misc_*model und model_static sind Modelle, misc_model_breakable nicht");
    }

    // --- Schluessel ------------------------------------------------------
    {
        Karte karte;
        auto e = karte.neueEntitaet("info_player_start", false);
        pruefe(reihenfolge(*e) == "classname=info_player_start;", "node_for_eclass: classname zuerst");
        e->setzen("origin", "0 0 24");
        e->setzen("angle", "90");
        e->setzen("classname", "info_player_start");
        e->setzen("origin", "8 0 24");
        pruefe(reihenfolge(*e) == "classname=info_player_start;origin=8 0 24;angle=90;",
               "setKeyValue: Reihenfolge des Einfuegens, Ueberschreiben an Ort und Stelle");
        e->setzen("angle", "");
        pruefe(!e->hat("angle") && reihenfolge(*e) == "classname=info_player_start;origin=8 0 24;",
               "setKeyValue mit leerem Wert loescht");
        e->setzen("Origin", "1 2 3");
        pruefe(e->wert("origin") == "8 0 24" && e->wert("Origin") == "1 2 3", "Schluessel unterscheiden Gross/klein");
        pruefe(e->klassenname() == "info_player_start" && !e->istBehaelter(),
               "ohne Definitionen: Ersatzklasse ohne Pinsel ist eine Punktentitaet");
    }

    // --- Behaelter und Klassenname --------------------------------------
    {
        EntitaetsKlassen kl;
        auto w = std::make_unique<EntitaetsKlasse>();
        w->setzeName("worldspawn");
        kl.einfuegen(std::move(w));
        auto s = std::make_unique<EntitaetsKlasse>();
        s->setzeName("info_player_start");
        s->fixedsize = true;
        kl.einfuegen(std::move(s));
        Karte karte;
        karte.klassen = &kl;
        auto welt = karte.neueEntitaet("WorldSpawn", true);
        pruefe(welt->klassenname() == "worldspawn" && welt->wert("classname") == "worldspawn" && welt->istBehaelter(),
               "Klasse ohne Gross/klein gefunden, classname = Klassenname, Behaelter");
        auto start = karte.neueEntitaet("info_player_start", false);
        pruefe(!start->istBehaelter(), "fixedsize -> kein Behaelter (isContainer)");
        start->setzen("classname", "foo");
        pruefe(start->klassenname() == "info_player_start", "getClassName ist der Klassenname, nicht der Schluessel");
    }

    // --- worldspawn, Kinder, Rueckgaengig --------------------------------
    {
        Karte karte;
        auto licht = karte.neueEntitaet("light", false);
        {
            Befehl b(karte.undo, "lichtEinfuegen");
            karte.entitaetEinfuegen(licht);
        }
        std::shared_ptr<Entitaet> welt;
        {
            Befehl b(karte.undo, "weltAnlegen");
            welt = karte.weltHolenOderAnlegen();
        }
        pruefe(karte.entitaeten().size() == 2 && karte.entitaeten()[0] == welt && karte.welt() == welt,
               "worldspawn wird als ERSTES Kind angelegt (Node_insertChildFirst)");
        auto pinsel = std::make_shared<Pinsel>();
        pinsel->quaderBauen(Kasten(Vektor3f(0, 0, 0), Vektor3f(8, 8, 8)), "textures/a", TexturProjektion(), karte.kontext);
        {
            Befehl b(karte.undo, "pinselEinfuegen");
            welt->kindEinfuegen(pinsel);
        }
        pruefe(welt->kinder().size() == 1, "Pinsel ist Kind der worldspawn");
        {
            Befehl b(karte.undo, "flaecheAendern");
            (*pinsel)[0].setzeShader("textures/b", karte.kontext);
        }
        pruefe((*pinsel)[0].shader() == "textures/b", "Shader geaendert");
        {
            Befehl b(karte.undo, "lichtWert");
            licht->setzen("light", "300");
            licht->setzen("light", "300");   // gleich: keine zweite Sicherung noetig
        }
        {
            Befehl b(karte.undo, "gleicherWert");
            licht->setzen("light", "300");
        }
        pruefe(karte.undo.undoAnzahl() == 5, "gleicher Wert legt keinen Undo-Schritt an (5 Schritte)");
        pruefe(karte.undo.undo() && !licht->hat("light"), "Undo: Schluessel weg");
        pruefe(karte.undo.undo() && (*pinsel)[0].shader() == "textures/a", "Undo: Flaeche wieder alt");
        pruefe(karte.undo.undo() && welt->kinder().empty(), "Undo: Pinsel wieder heraus");
        pruefe(karte.undo.redo() && welt->kinder().size() == 1, "Redo: Pinsel wieder drin");
        // nach dem Redo ist der Pinsel wieder angemeldet: Aenderung ist rueckgaengig machbar
        {
            Befehl b(karte.undo, "flaecheAendern2");
            (*pinsel)[1].setzeShader("textures/c", karte.kontext);
        }
        pruefe(karte.undo.undo() && (*pinsel)[1].shader() == "textures/a", "wieder eingefuegter Pinsel ist angemeldet");
        pruefe(karte.undo.undo() && karte.undo.undo() && karte.entitaeten().size() == 1 && karte.entitaeten()[0] == licht,
               "Undo bis vor die worldspawn");
    }

    // --- Auswahl ---------------------------------------------------------
    {
        Karte karte;
        auto welt = karte.weltHolenOderAnlegen();
        auto p1 = std::make_shared<Pinsel>();
        auto p2 = std::make_shared<Pinsel>();
        p1->quaderBauen(Kasten(Vektor3f(0, 0, 0), Vektor3f(8, 8, 8)), "textures/a", TexturProjektion(), karte.kontext);
        p2->quaderBauen(Kasten(Vektor3f(32, 0, 0), Vektor3f(8, 8, 8)), "textures/a", TexturProjektion(), karte.kontext);
        welt->kindEinfuegen(p1);
        welt->kindEinfuegen(p2);
        auto licht = karte.neueEntitaet("light", false);
        karte.entitaetEinfuegen(licht);
        karte.auswahl.waehlen(welt, p2);
        karte.auswahl.waehlen(licht);
        karte.auswahl.waehlen(welt, p1);
        karte.auswahl.waehlen(welt, p2);   // doppelt: bleibt einmal
        pruefe(karte.auswahl.anzahl() == 3 && karte.auswahl.elemente().back().primitiv.lock() == p1,
               "Auswahl in Reihenfolge, letzte = zuletzt gewaehlt");
        karte.auswahl.umschalten(licht);
        pruefe(!karte.auswahl.gewaehlt(licht.get()) && karte.auswahl.anzahl() == 2, "umschalten waehlt ab");
        karte.auswahl.flaecheWaehlen(p1, p1->flaechen()[2]);
        pruefe(karte.auswahl.flaecheGewaehlt(p1->flaechen()[2].get()) && karte.auswahl.flaechen().size() == 1, "Flaeche gewaehlt");
        welt->kindEntfernen(p1.get());
        karte.undo.clear();
        p1.reset();
        karte.auswahl.aufraeumen();
        pruefe(karte.auswahl.anzahl() == 1 && karte.auswahl.flaechen().empty(), "Aufraeumen entfernt geloeschte Objekte");
        karte.auswahl.leeren();
        pruefe(karte.auswahl.anzahl() == 0, "leeren");
    }

    std::printf("karteentitaet: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
