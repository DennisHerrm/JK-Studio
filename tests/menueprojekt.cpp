// menueprojekt.cpp - Menue-Editor M5b Punkt 3: Projektdatei *.jksmenu (Rundlauf, Fehler in
// normaler Sprache), Paket-Pfade, Sicherung (Autosave), Dateizuordnung (nur der Plan - der Test
// schreibt nie in die Registry).
#include "bhed/menue/projekt.h"

#include <cstdio>
#include <string>

using namespace bhed::menue;

namespace {
int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}
}  // namespace

int main() {
    Projekt p;
    p.spiel = Spiel::MD;
    p.dialekt = Dialekt::SP;
    p.profil = "Movie Duels";
    p.menues = {"ui/main.menu", "ui/mein menue.menu"};
    p.dateien["gfx/menus/knopf.tga"] = "ui/mein menue.menu";
    p.dateien["strings/english/mein.str"] = "ui/main.menu";
    p.offen = {"ui/mein menue.menu"};
    p.aktiv = 0;
    p.aufloesung = 7;
    p.balken = true;
    p.sprache = "Deutsch";
    p.cvars["ui_tab"] = "2";
    p.cvars["name"] = "Mein \"Held\"";
    p.exportPk3 = "C:\\Mods\\zz menu.pk3";
    p.nurAenderungen = true;
    const std::string t = projektText(p);
    Projekt q;
    ProjektFehler f;
    pruefe(liesProjekt(t, q, f), "Projekt gelesen");
    pruefe(q.spiel == Spiel::MD && q.dialekt == Dialekt::SP && q.profil == "Movie Duels" && q.menues == p.menues && q.offen == p.offen &&
               q.dateien == p.dateien && q.aktiv == 0 && q.aufloesung == 7 && q.balken && q.sprache == "Deutsch" && q.cvars == p.cvars && q.exportPk3 == p.exportPk3 &&
               q.nurAenderungen,
           "Rundlauf: alle Angaben (Leerzeichen, Anfuehrungszeichen, Backslash)");
    pruefe(projektText(q) == t, "Rundlauf: gleicher Text");
    pruefe(t.find("\r") == std::string::npos && t.compare(0, 2, "//") == 0 && t.find("\njksmenu 1\n") != std::string::npos,
           "Kopf: Kommentar, dann jksmenu 1");
    // CRLF (von Hand bearbeitet) und unbekannte Zeilen einer neueren Fassung
    std::string crlf;
    for (const char c : t) {
        if (c == '\n') { crlf += '\r'; }
        crlf += c;
    }
    crlf += "zukunft.wert 5\r\n";
    pruefe(liesProjekt(crlf, q, f) && q.menues == p.menues && q.sprache == "Deutsch", "CRLF und unbekannte Zeilen");
    // Fehler: kein Projekt, zu neu, Pfad aus dem Ordner heraus
    pruefe(!liesProjekt("menuDef { }\n", q, f) && f.art == ProjektFehler::Art::KeinProjekt && f.zeile == 1, "keine Projektdatei erkannt (Zeile 1)");
    pruefe(!liesProjekt("jksmenu 2\n", q, f) && f.art == ProjektFehler::Art::ZuNeu && f.wert == "2", "neuere Fassung gemeldet");
    pruefe(!liesProjekt("jksmenu 1\nmenu ../../Windows/x.menu\n", q, f) && f.art == ProjektFehler::Art::PfadAusserhalb && f.zeile == 2,
           "Pfad mit .. abgelehnt");
    pruefe(!liesProjekt("jksmenu 1\nmenu C:/x.menu\n", q, f) && f.art == ProjektFehler::Art::PfadAusserhalb, "absoluter Pfad abgelehnt");
    pruefe(!liesProjekt("jksmenu 1\nasset ui/a.menu ../x.tga\n", q, f) && f.art == ProjektFehler::Art::PfadAusserhalb && f.wert == "../x.tga",
           "Paketdatei ausserhalb abgelehnt");
    pruefe(!liesProjekt("", q, f) && f.art == ProjektFehler::Art::KeinProjekt, "leere Datei");
    // Paket
    pruefe(paketPfad("gfx/menus/knopf.tga") && paketPfad("ui/main.menu") && paketPfad("strings/english/mein.str"), "Paketdateien");
    pruefe(!paketPfad("mein.jksmenu") && !paketPfad(".jksautosave/1.menu") && !paketPfad("gfx/.git/x") && !paketPfad("../x.tga"),
           "nicht im Paket: Projektdatei, Sicherung, versteckte, ausserhalb");
    pruefe(projektUnterordner().size() == 5 && projektUnterordner()[0] == "ui", "Unterordner wie die pk3");
    // Sicherung
    const std::vector<SicherungsEintrag> se = {{"ui/main.menu", "D:\\Projekt\\ui\\main.menu", "1.menu"}, {"ui/neu.menu", "", "2.menu"}};
    std::vector<SicherungsEintrag> se2;
    std::int64_t zeit = 0;
    pruefe(liesSicherung(sicherungText(se, 1761750000), se2, zeit) && zeit == 1761750000 && se2.size() == 2 && se2[0].platte == se[0].platte &&
               se2[1].name == "ui/neu.menu" && se2[1].platte.empty() && se2[1].datei == "2.menu",
           "Sicherung: Rundlauf");
    pruefe(!liesSicherung("jksautosave 1\ntime 5\ndoc a b ../../x\n", se2, zeit), "Sicherung: Datei ausserhalb abgelehnt");
    // Dateizuordnung: nur der Plan (HKCU, ohne Adminrechte), geschrieben wird nur nach Rueckfrage
    const auto r = dateizuordnung("C:\\JK Studio\\jkstudio.exe", "JK Studio menu project");
    pruefe(r.size() == 4 && r[0].schluessel == "Software\\Classes\\.jksmenu" && r[0].wert == "JKStudio.MenuProject" &&
               r[3].schluessel == "Software\\Classes\\JKStudio.MenuProject\\shell\\open\\command" &&
               r[3].wert == "\"C:\\JK Studio\\jkstudio.exe\" \"%1\"",
           "Dateizuordnung: Schluessel unter HKCU\\Software\\Classes, Befehl mit %1 in Anfuehrungszeichen");
    std::printf("menueprojekt: %s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
