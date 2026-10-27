// uebergangtest.cpp - Stufe 8c: der Updater von rc604 holt JK Studio.
//
// Die installierten Fassungen bis rc605 tragen den Updater von rc601 (unveraendert
// bis rc605). Er fragt GitHub nach /repos/DennisHerrm/BehavEd-Remake/releases/latest
// (nach der Umbenennung leitet GitHub mit 301 weiter), laedt das erste .zip, setzt den
// Eintrag "behaved.exe" ganz oben an die Stelle der laufenden .exe und alle anderen
// Dateien daneben - ausser eine eigenstaendige .exe laeuft (kein data\base daneben),
// dann nur das Programm. Danach startet er die laufende .exe neu.
//
// Dieses Programm ist genau dieser Code (tests/uebergang/alt_update.inc und
// alt_install.inc, erzeugt aus git 8cc7bd1e = rc604), nur gegen einen lokalen
// Testserver statt api.github.com und mit einer simulierten laufenden .exe. Den
// Server, das Paket und die Pruefung danach (die eingespielte behaved.exe wirklich
// starten) macht tools/karte/updater_uebergang.py - ohne Netz zu GitHub.
//
//   uebergangtest <api-basis "http://127.0.0.1:<port>/repos/"> <laufende .exe> <ablage>
//
// Rueckgabe 0: das Release gefunden, geladen und eingespielt (Stand Fertig).
#include <windows.h>
#include <winhttp.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "bhed/diag.h"
#include "bhed/i18n.h"
#include "bhed/json.h"
#include "bhed/pk3.h"
#include "bhed/update.h"

#include "update.h"   // gui/update.h: Zustand, Stand (gleich geblieben)

#include "uebergang/alt_update.inc"

namespace bhed::gui::updater_alt {
namespace {

namespace fs = std::filesystem;
using ::bhed::gui::updater::Stand;
using ::bhed::gui::updater::Zustand;
using ::bhed::i18n::Str;
using ::bhed::i18n::tr;

std::wstring g_simExe;
std::wstring g_simApi;
fs::path g_simTemp;
constexpr const char* kAltesRepo = "DennisHerrm/BehavEd-Remake";
constexpr const char* kAlteFassung = "rc604";
std::string lokaleFassung() { return kAlteFassung; }

// aus gui/update_win32.cpp von rc604 (steht dort vor setze)
struct Geteilt {
    std::mutex m;
    Zustand z;
    bool fensterOffen = false;
    bool hinweisWeg = false;
    std::atomic<bool> laeuft{false};
};
Geteilt& g() {
    static Geteilt* x = new Geteilt();
    return *x;
}

#include "uebergang/alt_install.inc"

}  // namespace

int lauf(int argc, wchar_t** argv) {
    if (argc != 4) {
        std::printf("Aufruf: uebergangtest <api-basis> <laufende .exe> <ablage>\n");
        return 2;
    }
    g_simApi = argv[1];
    g_simExe = argv[2];
    g_simTemp = argv[3];
    pruefAuftrag();
    Zustand z = g().z;
    std::printf("pruefen: Stand %d, Release %s, Zip %s%s%s\n", static_cast<int>(z.stand), z.release.tag.c_str(),
                alt_update::zipAsset(z.release) != nullptr ? alt_update::zipAsset(z.release)->name.c_str() : "-",
                z.meldung.empty() ? "" : ", ", z.meldung.c_str());
    if (z.stand != Stand::Verfuegbar) { return 1; }
    installAuftrag(z.release);
    z = g().z;
    std::printf("installieren: Stand %d, %d Dateien%s%s\n", static_cast<int>(z.stand), z.dateien, z.meldung.empty() ? "" : ", ", z.meldung.c_str());
    return z.stand == Stand::Fertig ? 0 : 1;
}

}  // namespace bhed::gui::updater_alt

int wmain(int argc, wchar_t** argv) { return bhed::gui::updater_alt::lauf(argc, argv); }
