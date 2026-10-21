// karteeditor.cpp - Karten-Editor Stufe 1b: Kernteile der Oberflaeche, die sich
// ohne Fenster pruefen lassen.
//
// Punkt 2 (Arbeitsbereich):
//   - zuletzt geoeffnete Karten wie NRC (radiant/mru.cpp): neun Eintraege,
//     vorhandener Name wandert nach vorn, fehlende Datei faellt still heraus,
//     Beschriftung "&<n>- <Pfad>" mit verdoppeltem '&', leere Liste "&1"
//     bzw. nach dem Leeren "Recent Files";
//   - "Karte geaendert" wie NRCs UndoFileChangeTracker (libs/maplib.h): ein
//     Befehl aendert, Rueckgaengig bis zum gespeicherten Stand macht wieder
//     "gespeichert", ein neuer Befehl nach Rueckgaengig macht den gespeicherten
//     Stand unerreichbar; Ausgabe "Undo: ..." wie globalOutputStream.
#include "bhed/karte/karte.h"
#include "bhed/karte/zuletzt.h"

#include <cstdio>
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

void zuletztPruefen() {
    ZuletztListe z;
    pruefe(z.beschriftung(0) == "&1" && !z.ersterAktiv(), "leer: erster Eintrag \"&1\", ausgegraut");
    for (int i = 1; i <= 10; ++i) { z.hinzufuegen("C:/maps/k" + std::to_string(i) + ".map"); }
    pruefe(z.anzahl() == 9 && z.pfade().front() == "C:/maps/k10.map" && z.pfade().back() == "C:/maps/k2.map",
           "MRU_MAX 9: neuester vorn, der aelteste faellt heraus");
    z.hinzufuegen("C:/maps/k5.map");
    pruefe(z.pfade()[0] == "C:/maps/k5.map" && z.pfade()[1] == "C:/maps/k10.map" && z.anzahl() == 9,
           "vorhandener Name wandert nach vorn, keine Dublette");
    z.hinzufuegen("C:/maps/K5.map");
    pruefe(z.anzahl() == 9 && z.pfade()[0] == "C:/maps/K5.map" && z.pfade()[1] == "C:/maps/k5.map",
           "strcmp: anders geschrieben ist ein anderer Eintrag");
    z.setzen({"C:/a & b/x.map", "C:/y.map"});
    pruefe(z.beschriftung(0) == "&1- C:/a && b/x.map" && z.beschriftung(1) == "&2- C:/y.map",
           "Beschriftung \"&<n>- Pfad\", '&' verdoppelt (EscapedMnemonic)");
    pruefe(ohneMnemonik(z.beschriftung(0)) == "1- C:/a & b/x.map", "Anzeige ohne Mnemonik");
    pruefe(z.aktivieren(1, true) == "C:/y.map" && z.pfade()[0] == "C:/y.map", "lesbar: nach vorn, Pfad zum Laden");
    pruefe(z.aktivieren(0, false).empty() && z.anzahl() == 1 && z.pfade()[0] == "C:/a & b/x.map",
           "nicht lesbar: faellt still aus der Liste");
    pruefe(z.aktivieren(0, false).empty() && z.anzahl() == 0 && z.beschriftung(0) == "Recent Files" && !z.ersterAktiv(),
           "letzter entfernt: \"Recent Files\", ausgegraut");
    std::vector<std::string> viele;
    for (int i = 0; i < 12; ++i) { viele.push_back("p" + std::to_string(i)); }
    z.setzen(viele);
    pruefe(z.anzahl() == 9 && z.beschriftung(0) == "&1- p0", "gespeicherte Liste: hoechstens neun (MRU_Init)");
}

void geaendertPruefen() {
    Karte karte;
    std::vector<std::string> ausgabe;
    karte.undo.ausgabe = [&](const std::string& s) { ausgabe.push_back(s); };
    auto welt = karte.weltHolenOderAnlegen();
    karte.anmelden();
    karte.undo.clear();
    karte.undo.alsGespeichertMarkieren();
    pruefe(karte.undo.gespeichert(), "nach Laden/Neu: gespeichert");
    {
        Befehl b(karte.undo, "entitySetKeyValue -key \"message\" -value \"a\"");
        welt->setzen("message", "a");
    }
    pruefe(!karte.undo.gespeichert(), "Befehl: geaendert");
    pruefe(!ausgabe.empty() && ausgabe.back() == "entitySetKeyValue -key \"message\" -value \"a\"",
           "finish schreibt den Befehlsnamen in die Ausgabe");
    {
        Befehl b(karte.undo, "leer");
    }
    pruefe(ausgabe.size() == 1, "Befehl ohne Aenderung: keine Ausgabe, kein Schritt");
    karte.undo.alsGespeichertMarkieren();
    {
        Befehl b(karte.undo, "zwei");
        welt->setzen("message", "b");
    }
    pruefe(!karte.undo.gespeichert(), "nach dem Speichern wieder geaendert");
    pruefe(karte.undo.undo() && karte.undo.gespeichert() && welt->wert("message") == "a",
           "Rueckgaengig bis zum gespeicherten Stand: wieder gespeichert");
    pruefe(ausgabe.back() == "Undo: zwei", "Ausgabe \"Undo: <Befehl>\"");
    pruefe(karte.undo.redo() && !karte.undo.gespeichert(), "Wiederholen: geaendert");
    pruefe(karte.undo.undo() && karte.undo.undo() && !karte.undo.gespeichert(), "vor den gespeicherten Stand: geaendert");
    {
        Befehl b(karte.undo, "drei");
        welt->setzen("message", "c");
    }
    pruefe(karte.undo.undo() && !karte.undo.gespeichert(),
           "neuer Befehl nach Rueckgaengig: gespeicherter Stand unerreichbar (MAPFILE_MAX_CHANGES)");
    karte.undo.undo();
    pruefe(ausgabe.back() == "Undo: no undo available", "\"Undo: no undo available\"");
}

}  // namespace

int main() {
    zuletztPruefen();
    geaendertPruefen();
    std::printf("karteeditor: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
