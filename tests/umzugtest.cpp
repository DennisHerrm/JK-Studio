// umzugtest.cpp - Stufe 8c: der Einstellungsordner zieht von %APPDATA%\\behaved
// nach %APPDATA%\\jkstudio um (siehe bhed/umzug.h) - ohne Fenster, in einem
// eigenen Ordner unter %TEMP% (der echte Ordner des Anwenders wird nie angefasst).
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>

#include "bhed/umzug.h"

namespace {

namespace fs = std::filesystem;
int fehler = 0;

void expect(const char* was, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was);
    if (!ok) { ++fehler; }
}

void schreib(const fs::path& p, const std::string& text) {
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
    std::ofstream f(p, std::ios::binary);
    f << text;
}

std::string lies(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream o;
    o << f.rdbuf();
    return o.str();
}

}  // namespace

int main() {
    using namespace bhed::umzug;
    std::error_code ec;
    const fs::path basis = fs::temp_directory_path() / "bhed_umzugtest";
    fs::remove_all(basis, ec);
    const fs::path alt = basis / kAlterOrdner;
    const fs::path neu = basis / kNeuerOrdner;
    const std::string cfg = "language=ja\ntheme=Dark\nrecent0=C:/karten/meine.map\n";
    schreib(alt / "behaved.cfg", cfg);
    schreib(alt / "karte_prtview.ini", "[Config]\n");
    schreib(alt / "tour_stand.txt", "3\n");
    schreib(alt / "karte_export" / "x.obj", "o\n");
    schreib(alt / "shader" / "abc.cso", "cache");
    schreib(alt / "data" / "base" / "behaved.bhc", "eingebaut");
    schreib(alt / "karte_bau" / "raum.map", "arbeit");
    schreib(alt / "karte_modellindex_1234.txt", "index");
    schreib(alt / "behaved.log", "log");

    expect("auslassen: Shader-Zwischenspeicher, Modell-Index, Daten, Arbeitsordner, Protokolle",
           auslassen("shader/abc.cso") && auslassen("karte_modellindex_99.txt") && auslassen("data/base/behaved.bhc") &&
               auslassen("karte_bau/raum.map") && auslassen("karte_bsp/duel1/x.bsp") && auslassen("Behaved.LOG"));
    expect("auslassen: Einstellungen und eigene Ausgaben bleiben drin",
           !auslassen("behaved.cfg") && !auslassen("karte_export/x.obj") && !auslassen("tour_stand.txt") && !auslassen("karte_prtview.ini"));

    const Ergebnis e = uebernehmen(alt, neu);
    expect("erster Start: uebernommen, ohne Fehler", e.uebernommen && e.fehler.empty());
    expect("die Einstellungen sind im neuen Ordner (Sprache, Thema, letzte Karte)", lies(neu / "behaved.cfg") == cfg);
    expect("eigene Dateien kommen mit (prtview, Tutorial-Stand, Export)",
           fs::exists(neu / "karte_prtview.ini") && fs::exists(neu / "tour_stand.txt") && fs::exists(neu / "karte_export" / "x.obj"));
    expect("Zwischenspeicher und Arbeitsordner entstehen neu (nicht kopiert)",
           !fs::exists(neu / "shader") && !fs::exists(neu / "data") && !fs::exists(neu / "karte_bau") &&
               !fs::exists(neu / "karte_modellindex_1234.txt") && !fs::exists(neu / "behaved.log"));
    expect("das Original bleibt unveraendert", lies(alt / "behaved.cfg") == cfg && fs::exists(alt / "shader" / "abc.cso") &&
                                                   fs::exists(alt / "data" / "base" / "behaved.bhc"));
    expect("kein Zwischenordner .umzug bleibt liegen", !fs::exists(basis / (std::string(kNeuerOrdner) + ".umzug")));

    schreib(neu / "behaved.cfg", "language=de\n");
    const Ergebnis e2 = uebernehmen(alt, neu);
    expect("zweiter Start: nichts geschieht, der neue Ordner wird nicht ueberschrieben",
           !e2.uebernommen && e2.fehler.empty() && lies(neu / "behaved.cfg") == "language=de\n");

    const Ergebnis e3 = uebernehmen(basis / "gibtsnicht", basis / "frisch");
    expect("ohne alten Ordner: nichts zu tun", !e3.uebernommen && e3.fehler.empty() && !fs::exists(basis / "frisch"));

    fs::remove_all(basis, ec);
    std::printf(fehler == 0 ? "\nalle Umzugsproben bestanden\n" : "\nFEHLGESCHLAGEN (%d)\n", fehler);
    return fehler == 0 ? 0 : 1;
}
