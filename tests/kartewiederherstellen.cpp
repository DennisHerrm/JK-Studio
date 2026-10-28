// kartewiederherstellen.cpp - Stufe 9 Punkt 12: nach einem Absturz wiederherstellen
// (ctest karte_wiederherstellen)
//
// Ohne Sitzungsdatei: nichts. Mit Sitzungsdatei und einem Autosave, das neuer ist als die Karte:
// anbieten; aelter: nicht; Karte fehlt: anbieten; ohne Namen: das Autosave im Benutzerordner;
// sauber beendet (Datei geloescht): nichts. Dateien in einem eigenen Ordner unter TEMP.
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "bhed/karte/kartendatei.h"
#include "bhed/karte/wiederherstellen.h"

namespace {
int g_fehler = 0;
void expect(const char* was, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was);
    if (!ok) { ++g_fehler; }
}
namespace fs = std::filesystem;
std::string text(const fs::path& p) {
    const std::u8string u = p.generic_u8string();
    return std::string(u.begin(), u.end());
}
void schreiben(const fs::path& p, const char* inhalt) { std::ofstream(p, std::ios::binary) << inhalt; }
}  // namespace

int main() {
    using namespace bhed::karte;
    std::error_code ec;
    const fs::path o = fs::temp_directory_path(ec) / "jkstudio_wiederherstellen_test";
    fs::remove_all(o, ec);
    fs::create_directories(o, ec);
    const std::string sitzung = text(o / kSitzungsDatei);
    const std::string karte = text(o / "raum.map");
    const std::string unbenannt = text(o / "autosave.map");
    expect("ohne Sitzungsdatei: nichts", !wiederherstellungPruefen(sitzung, unbenannt, 2).anbieten);

    schreiben(o / "raum.map", "{\n}\n");
    sitzungSchreiben(sitzung, karte, 1);
    expect("Sitzung da, kein Autosave: nichts", !wiederherstellungPruefen(sitzung, unbenannt, 2).anbieten);
    const std::string auto1 = autosavePfad(karte);
    schreiben(fs::path(std::u8string(auto1.begin(), auto1.end())), "{\n// neuer\n}\n");
    fs::last_write_time(o / "raum.map", fs::last_write_time(o / "raum.map", ec) - std::chrono::minutes(5), ec);
    Wiederherstellung w = wiederherstellungPruefen(sitzung, unbenannt, 2);
    expect("Autosave neuer als die Karte: anbieten, mit Karte und Autosave", w.anbieten && w.karte == karte && w.autosave == auto1);
    expect("dieselbe Sitzung (gleicher Prozess) ist kein Absturz", !wiederherstellungPruefen(sitzung, unbenannt, 1).anbieten);
    fs::last_write_time(o / "raum.map", fs::last_write_time(o / "raum.map", ec) + std::chrono::minutes(10), ec);
    expect("Karte neuer (nach dem Autosave gespeichert): nicht anbieten", !wiederherstellungPruefen(sitzung, unbenannt, 2).anbieten);
    fs::remove(o / "raum.map", ec);
    expect("Karte fehlt, Autosave da: anbieten", wiederherstellungPruefen(sitzung, unbenannt, 2).anbieten);
    sitzungSchreiben(sitzung, "unnamed.map", 1);
    schreiben(o / "autosave.map", "{\n}\n");
    w = wiederherstellungPruefen(sitzung, unbenannt, 2);
    expect("ohne Namen: das Autosave im Benutzerordner", w.anbieten && w.autosave == unbenannt);
    sitzungLoeschen(sitzung);
    expect("sauber beendet (Sitzungsdatei geloescht): nichts", !wiederherstellungPruefen(sitzung, unbenannt, 2).anbieten);
    fs::remove_all(o, ec);
    std::printf("%s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
