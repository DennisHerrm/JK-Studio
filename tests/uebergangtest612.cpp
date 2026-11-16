// uebergangtest612.cpp - REL-TEMPO: der Updater von rc612 holt ein Release im neuen Format.
//
// Die installierten Fassungen rc606 bis rc612 tragen den Updater der Stufe 8d (gui/update_win32.cpp pruefAuftrag und
// installAuftrag, src/update.cpp, src/update_einspielen.cpp). Er nimmt das ERSTE .zip des Releases, prueft es gegen
// SHA256SUMS.txt (und die Signatur, sobald ein Schluessel im Programm steht) und spielt es ueber den Zwischenordner ein.
// Ab REL-TEMPO liegen daneben der Starter, das Manifest und die Deltas (ohne .zip-Endung) - eine eigenstaendige .exe gibt es
// nicht mehr. Dieses Programm ist genau dieser Code (tests/uebergang/alt612_*.inc, erzeugt aus git 348f0e345 von
// tools/karte/gen_uebergang612.py), nur gegen einen lokalen Testserver statt api.github.com und mit einer simulierten
// laufenden .exe. Server, Paket und die Pruefung danach macht tools/karte/updater_uebergang.py.
//
//   uebergangtest612 <api-basis "http://127.0.0.1:<port>/repos/"> <laufende .exe> <ablage>
//
// Rueckgabe 0: das Release gefunden, geladen und eingespielt (Stand Fertig).
#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "bhed/diag.h"
#include "bhed/i18n.h"
#include "bhed/json.h"
#include "bhed/pk3.h"
#include "bhed/update.h"

#include "update.h"   // gui/update.h: Zustand, Stand, TestNetz (seit rc612 gleich)

#include "uebergang/alt612_update.inc"
#include "uebergang/alt612_einspielen.inc"

namespace bhed::gui::updater_alt612 {
namespace {

namespace fs = std::filesystem;
using ::bhed::gui::updater::Stand;
using ::bhed::gui::updater::TestNetz;
using ::bhed::gui::updater::Zustand;
using ::bhed::i18n::Str;
using ::bhed::i18n::tr;

std::wstring g_simExe;
std::wstring g_simApi;
fs::path g_simTemp;
constexpr const char* kAlteFassung = "rc612";
std::string lokaleFassung() { return kAlteFassung; }

#include "uebergang/alt612_install.inc"

}  // namespace

int lauf(int argc, wchar_t** argv) {
    if (argc != 4) {
        std::printf("Aufruf: uebergangtest612 <api-basis> <laufende .exe> <ablage>\n");
        return 2;
    }
    g_simApi = argv[1];
    g_simExe = argv[2];
    g_simTemp = argv[3];
    pruefAuftrag();
    Zustand z = g().z;
    const bhed::alt612_update::Asset* zip = bhed::alt612_update::zipAsset(z.release);
    std::printf("pruefen: Stand %d, Release %s, Zip %s%s%s\n", static_cast<int>(z.stand), z.release.tag.c_str(),
                zip != nullptr ? zip->name.c_str() : "-", z.meldung.empty() ? "" : ", ", z.meldung.c_str());
    if (z.stand != Stand::Verfuegbar) { return 1; }
    installAuftrag(z.release);
    z = g().z;
    std::printf("installieren: Stand %d, %d Dateien%s%s\n", static_cast<int>(z.stand), z.dateien, z.meldung.empty() ? "" : ", ",
                z.meldung.c_str());
    return z.stand == Stand::Fertig ? 0 : 1;
}

}  // namespace bhed::gui::updater_alt612

int wmain(int argc, wchar_t** argv) { return bhed::gui::updater_alt612::lauf(argc, argv); }
