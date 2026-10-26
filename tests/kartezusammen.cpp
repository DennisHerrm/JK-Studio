// kartezusammen.cpp - Karten-Editor Stufe 8a Punkt 5c: "Zur fertigen Karte hinzufuegen"
// (bhed/karte/zusammenfuehren.h).
//
//   kartezusammen <haupt.bsp> <teil.bsp> [<merged.bsp>]
//
// Gibt je Zeile aus: "befund <schwere> <pruefung> <anzahl>", "kollision <geladen> <gesamt>
// <stoppen> <nichtPruefbar>", "teilbrushes <n>", "licht <ambient> <punkte> <lichter>" -
// tools/karte/q3map2_zusammen.py (ctest q3map2_zusammen) baut die Teile mit
// bhed_q3map2, fuehrt sie zusammen und prueft diese Zeilen.
#include "bhed/karte/zusammenfuehren.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

using namespace bhed::karte;

namespace {
std::string lies(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}
}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("Aufruf: kartezusammen <haupt.bsp> <teil.bsp> [<merged.bsp>]\n");
        return 2;
    }
    const std::string haupt = lies(argv[1]);
    const std::string teil = lies(argv[2]);
    if (haupt.empty() || teil.empty()) {
        std::printf("nicht lesbar\n");
        return 2;
    }
    for (const MergeBefund& b : mergeVorPruefen(haupt, teil)) {
        const char* s = b.schwere == MergeBefund::Schwere::Fehler ? "fehler" : b.schwere == MergeBefund::Schwere::Warnung ? "warnung" : "hinweis";
        std::printf("befund %s %s %lld %s\n", s, b.pruefung.c_str(), b.anzahl, b.text.c_str());
    }
    const int tb = weltBrushes(teil);
    std::printf("teilbrushes %d\n", tb);
    const double mins[3] = {-100000, -100000, -100000};
    const double maxs[3] = {100000, 100000, 100000};
    const MergeLicht l = mergeLichtAusHauptkarte(haupt, mins, maxs, 0.0);
    std::printf("licht %.1f %d %zu\n", l.ambient, l.gitterPunkte, l.lichter.size());
    if (argc >= 4) {
        const std::string m = lies(argv[3]);
        const MergeKollision k = mergeKollisionPruefen(m, tb);
        std::printf("kollision %d %d %d %d\n", k.geladen ? 1 : 0, k.gesamt, k.stoppen, k.nichtPruefbar);
    }
    return 0;
}
