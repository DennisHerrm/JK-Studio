// diagfaeden.cpp - rc610 F78 (Fund B7-1): das Protokoll aus vielen Faeden
//
// diag::warn und diag::detail kommen auch aus Hintergrundfaeden (Arbeiter von
// bildVorratFuellen, Updater, Schnell-Backen, Waechter). Acht Faeden schreiben
// je 20 000 Zeilen in beide Protokolle; danach muessen genau so viele Zeilen im
// Speicher und in beiden Dateien stehen, jede vollstaendig.
#include "bhed/diag.h"

#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr int kFaeden = 8;
constexpr int kZeilen = 20000;

int fehler = 0;
void expect(const char* was, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was);
    if (!ok) { ++fehler; }
}

std::string zeile(int faden, int i) { return "Faden " + std::to_string(faden) + " Zeile " + std::to_string(i) + " Ende"; }

// Zeilen der Datei, die genau so aussehen, wie ein Faden sie schrieb (mit dem
// Vorsatz `vorsatz`); `kaputt` zaehlt alle anderen.
int vollstaendige(const std::string& pfad, const std::string& vorsatz, int& kaputt) {
    std::ifstream f(pfad, std::ios::binary);
    std::string z;
    int n = 0;
    kaputt = 0;
    while (std::getline(f, z)) {
        if (!z.empty() && z.back() == '\r') { z.pop_back(); }   // das Detailprotokoll ist eine Textdatei
        if (z.rfind(vorsatz + "Faden ", 0) == 0 && z.size() > 5 && z.compare(z.size() - 5, 5, " Ende") == 0) {
            ++n;
        } else if (z.rfind(vorsatz, 0) == 0 && z.find("Faden") != std::string::npos) {
            ++kaputt;
        }
    }
    return n;
}

}  // namespace

int main(int argc, char** argv) {
    namespace fs = std::filesystem;
    const fs::path ordner = argc > 1 ? fs::path(argv[1]) : fs::temp_directory_path();
    const std::string ablauf = (ordner / "bhed_diagfaeden.log").string();
    const std::string detail = (ordner / "bhed_diagfaeden-detail.log").string();
    bhed::diag::resetForTesting();
    expect("Ablaufprotokoll geoeffnet", bhed::diag::open(ablauf));
    expect("Detailprotokoll geoeffnet", bhed::diag::openDetail(detail));
    const std::size_t vorher = bhed::diag::lines().size();

    std::vector<std::thread> faeden;
    for (int t = 0; t < kFaeden; ++t) {
        faeden.emplace_back([t] {
            for (int i = 0; i < kZeilen; ++i) {
                const std::string z = zeile(t, i);
                bhed::diag::warn(z);
                bhed::diag::detail(z);
            }
        });
    }
    for (std::thread& f : faeden) { f.join(); }
    const std::size_t im = bhed::diag::lines().size() - vorher;
    bhed::diag::closeDetail();
    bhed::diag::close();

    const int soll = kFaeden * kZeilen;
    std::printf("      im Speicher %zu, soll %d\n", im, soll);
    expect("diag::lines(): genau 8 x 20000 Zeilen dazu", im == static_cast<std::size_t>(soll));
    int kaputtA = 0;
    int kaputtD = 0;
    const int inA = vollstaendige(ablauf, "WARN ", kaputtA);
    const int inD = vollstaendige(detail, "", kaputtD);
    std::printf("      Ablaufprotokoll %d vollstaendig, %d kaputt; Detailprotokoll %d vollstaendig, %d kaputt\n", inA, kaputtA,
                inD, kaputtD);
    expect("Ablaufprotokoll: jede Zeile vollstaendig und keine fehlt", inA == soll && kaputtA == 0);
    expect("Detailprotokoll: jede Zeile vollstaendig und keine fehlt", inD == soll && kaputtD == 0);

    std::error_code ec;
    for (const std::string& p : {ablauf, detail}) {
        fs::remove(p, ec);
        fs::remove(p + ".vorher", ec);
    }
    std::printf("%s\n", fehler == 0 ? "diag_faeden: alles ok" : "diag_faeden: FEHLER");
    return fehler == 0 ? 0 : 1;
}
