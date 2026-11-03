// sicherschreibentest.cpp - rc609 W2a Punkt 5 (Fund F5): sicheres Schreiben
//
// Ein simulierter Schreibfehler (mitten im Schreiben, beim Schliessen, beim
// Umbenennen) laesst die alte Datei unveraendert, meldet false und laesst keine
// Temp-Datei liegen. Eine von einem anderen Programm geoeffnete Datei (wie das
// Spiel seine .pk3 offen haelt) meldet "gesperrt" und bleibt ebenso stehen.
//
// Aufruf: sicherschreibentest <Arbeitsordner>
#include "bhed/sicherschreiben.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

namespace {
int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was.c_str());
    if (!ok) { ++g_fehler; }
}
std::string lies(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}
void schreibeRoh(const fs::path& p, const std::string& t) {
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    f.write(t.data(), static_cast<std::streamsize>(t.size()));
}
bool tmpDa(const fs::path& p) {
    std::error_code ec;
    return fs::exists(fs::path(p.string() + ".tmp"), ec);
}
}  // namespace

int main(int argc, char** argv) {
    const fs::path ordner = fs::path(argc > 1 ? argv[1] : "sicherschreiben_lauf") / "sicherschreiben";
    std::error_code ec;
    fs::remove_all(ordner, ec);
    fs::create_directories(ordner, ec);
    const fs::path datei = ordner / "probe.ent";
    const std::string alt = "{\n\"classname\" \"worldspawn\"\n}\n";
    std::string neu;
    for (int i = 0; i < 400; ++i) { neu += "{\n\"classname\" \"info_null\"\n\"targetname\" \"n" + std::to_string(i) + "\"\n}\n"; }

    // Normalfall
    schreibeRoh(datei, alt);
    pruefe(bhed::sicherSchreiben(datei.string(), neu), "Normalfall: true");
    pruefe(lies(datei) == neu, "Normalfall: neuer Inhalt vollstaendig");
    pruefe(!tmpDa(datei), "Normalfall: keine Temp-Datei");

    // Simulierte Fehler: 1 mitten im Schreiben, 2 beim Schliessen, 3 beim Umbenennen
    const char* const namen[4] = {"", "Schreiben", "Schliessen", "Umbenennen"};
    for (int stufe = 1; stufe <= 3; ++stufe) {
        schreibeRoh(datei, alt);
        bhed::sicherSchreibenFehlerEinbauen(stufe);
        std::string fehler;
        bhed::SchreibFehler art = bhed::SchreibFehler::Keiner;
        const bool ok = bhed::sicherSchreiben(datei.string(), neu, &fehler, &art);
        const std::string vorher = lies(datei);
        std::printf("  Fehler beim %s: Rueckgabe %s, Datei %zu Bytes (alt %zu), Meldung \"%s\"\n", namen[stufe], ok ? "true" : "false",
                    vorher.size(), alt.size(), fehler.c_str());
        pruefe(!ok, std::string("Fehler beim ") + namen[stufe] + ": meldet false");
        pruefe(vorher == alt, std::string("Fehler beim ") + namen[stufe] + ": alte Datei unveraendert");
        pruefe(!tmpDa(datei), std::string("Fehler beim ") + namen[stufe] + ": keine Temp-Datei");
        pruefe(!fehler.empty() && art != bhed::SchreibFehler::Keiner, std::string("Fehler beim ") + namen[stufe] + ": mit Meldung");
    }

    // Gesperrt (ein anderes Programm haelt die Datei offen, wie das Spiel seine .pk3)
    schreibeRoh(datei, alt);
    {
        std::ifstream offen(datei, std::ios::binary);
        pruefe(bhed::dateiGesperrt(datei.string()), "gesperrt: erkannt, solange offen");
        std::string fehler;
        bhed::SchreibFehler art = bhed::SchreibFehler::Keiner;
        const bool ok = bhed::sicherSchreiben(datei.string(), neu, &fehler, &art);
        pruefe(!ok && art == bhed::SchreibFehler::Gesperrt, "gesperrt: false mit Art Gesperrt (" + fehler + ")");
    }
    pruefe(lies(datei) == alt, "gesperrt: alte Datei unveraendert");
    pruefe(!tmpDa(datei), "gesperrt: keine Temp-Datei");
    pruefe(!bhed::dateiGesperrt(datei.string()), "nach dem Schliessen nicht mehr gesperrt");

    // Fehlende Ordner entstehen
    const fs::path tief = ordner / "maps" / "neu" / "x.ent";
    pruefe(bhed::sicherSchreiben(tief.string(), alt) && lies(tief) == alt, "fehlende Ordner werden angelegt");

    fs::remove_all(ordner, ec);
    std::printf("sicherschreiben: %d Fehler\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
