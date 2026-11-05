// umzugtest.cpp - Stufe 8c: der Einstellungsordner zieht von %APPDATA%\\behaved
// nach %APPDATA%\\jkstudio um (siehe bhed/umzug.h) - ohne Fenster, in einem
// eigenen Ordner unter %TEMP% (der echte Ordner des Anwenders wird nie angefasst).
#ifdef _WIN32
#include <windows.h>
#endif

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

#ifdef _WIN32
    // rc610 F78 (Fund B8-1): eine Datei im alten Ordner laesst sich nicht kopieren
    // (ein anderes Programm haelt sie ohne Teilen offen). Kein neuer Ordner, die
    // Sitzung bleibt im alten; der naechste Start kopiert.
    {
        const fs::path b2 = basis / "zweiter";
        schreib(b2 / kAlterOrdner / "behaved.cfg", cfg);
        schreib(b2 / kAlterOrdner / "gesperrt.txt", "x");
        HANDLE h = CreateFileW((b2 / kAlterOrdner / "gesperrt.txt").c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        const Wahl w1 = ordnerWaehlen(b2);
        CloseHandle(h);
        std::printf("      (%s)\n", w1.umzug.fehler.c_str());
        expect("gescheiterte Uebernahme: kein neuer Ordner, diese Sitzung im alten",
               h != INVALID_HANDLE_VALUE && !w1.umzug.fehler.empty() && w1.ordner == b2 / kAlterOrdner &&
                   !fs::exists(b2 / kNeuerOrdner) && !fs::exists(b2 / (std::string(kNeuerOrdner) + ".umzug")));
        const Wahl w2 = ordnerWaehlen(b2);
        expect("naechster Start ohne Sperre: kopiert, neuer Ordner mit behaved.cfg",
               w2.umzug.uebernommen && w2.ordner == b2 / kNeuerOrdner && lies(b2 / kNeuerOrdner / "behaved.cfg") == cfg);
        const Wahl w3 = ordnerWaehlen(basis / "leer");
        expect("ohne alten Ordner: der neue wird angelegt", w3.umzug.fehler.empty() && fs::is_directory(basis / "leer" / kNeuerOrdner));
    }
#endif

    fs::remove_all(basis, ec);
    std::printf(fehler == 0 ? "\nalle Umzugsproben bestanden\n" : "\nFEHLGESCHLAGEN (%d)\n", fehler);
    return fehler == 0 ? 0 : 1;
}
