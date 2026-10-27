// karteundoliste.cpp - Karten-Editor Stufe 8b Punkt 7: der Undo-Verlauf als Liste.
//
// Fuenf Befehle mit NRC-Namen: die Liste nennt sie in der Reihenfolge, nach
// Rueckgaengig stehen die Wiederholen-Schritte (naechster zuerst); ein Sprung
// zu einem frueheren Stand und zurueck gibt die Karte byte-gleich, ein Sprung
// ausserhalb wird abgelehnt, ein neuer Befehl verwirft die Wiederholen-Liste.
#include "bhed/karte/bearbeitung.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/rueckgaengig.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
int g_pruefungen = 0;
void pruefe(bool ok, const std::string& was) {
    ++g_pruefungen;
    if (!ok) { ++g_fehler; }
    std::printf("%s  %s\n", ok ? "OK  " : "FEHL", was.c_str());
}

std::string liste(const std::vector<std::string>& v) {
    std::string s;
    for (const auto& x : v) { s += (s.empty() ? "" : " | ") + x; }
    return s;
}

}  // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    const std::string t = "{\n\"classname\" \"worldspawn\"\n}\n{\n\"classname\" \"info_null\"\n\"origin\" \"0 0 0\"\n}\n";
    Karte k;
    (void)karteLesen(t, k);
    k.anmelden();
    const auto ent = k.entitaeten()[1];
    std::vector<std::string> staende{karteSchreiben(k)};
    const char* namen[5] = {"entitySetKeyValue -key \"a\" -value \"1\"", "entitySetKeyValue -key \"b\" -value \"2\"",
                            "translateTool -xyz 8 0 0", "entitySetKeyValue -key \"a\" -value \"\"", "entitySetKeyValue -key \"c\" -value \"3\""};
    for (int i = 0; i < 5; ++i) {
        Befehl b(k.undo, namen[i]);
        switch (i) {
        case 0: ent->setzen("a", "1"); break;
        case 1: ent->setzen("b", "2"); break;
        case 2: ent->setzen("origin", "8 0 0"); break;
        case 3: ent->setzen("a", ""); break;
        default: ent->setzen("c", "3"); break;
        }
        staende.push_back(karteSchreiben(k));
    }
    {
        Befehl b(k.undo, "leer");   // ohne Aenderung: kein Eintrag (wie NRC)
    }
    const auto u = k.undo.undoNamen();
    pruefe(u.size() == 5 && u[0] == namen[0] && u[4] == namen[4] && k.undo.redoNamen().empty(),
           "Liste: 5 Schritte mit NRC-Namen, aeltester zuerst, ein leerer Befehl fehlt: " + liste(u));
    pruefe(k.undo.springen(1) && karteSchreiben(k) == staende[1] && k.undo.undoAnzahl() == 1 && k.undo.redoAnzahl() == 4,
           "Sprung zu Schritt 1: byte-gleich mit dem Stand nach Schritt 1");
    const auto r = k.undo.redoNamen();
    pruefe(r.size() == 4 && r[0] == namen[1] && r[3] == namen[4], "Wiederholen-Liste: naechster zuerst: " + liste(r));
    pruefe(k.undo.springen(4) && karteSchreiben(k) == staende[4], "Sprung vor zu Schritt 4: byte-gleich");
    pruefe(k.undo.springen(0) && karteSchreiben(k) == staende[0], "Sprung zum Anfang: byte-gleich mit der geladenen Karte");
    pruefe(k.undo.springen(5) && karteSchreiben(k) == staende[5] && k.undo.undoNamen() == u, "Sprung zurueck ans Ende: byte-gleich, Liste wie vorher");
    pruefe(!k.undo.springen(6) && karteSchreiben(k) == staende[5], "Sprung ausserhalb: abgelehnt, nichts geaendert");
    pruefe(k.undo.springen(2) && karteSchreiben(k) == staende[2], "Sprung zu Schritt 2");
    {
        Befehl b(k.undo, "entitySetKeyValue -key \"d\" -value \"4\"");
        ent->setzen("d", "4");
    }
    pruefe(k.undo.undoAnzahl() == 3 && k.undo.redoNamen().empty() && k.undo.undoNamen().back() == "entitySetKeyValue -key \"d\" -value \"4\"",
           "neuer Befehl nach dem Sprung: Wiederholen-Liste verworfen (wie NRC)");
    std::printf("%d Pruefungen, %d Fehler\n", g_pruefungen, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
