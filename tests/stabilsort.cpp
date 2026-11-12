// stabilsort.cpp - TEMPO (12.11.2026): bhed/stabilsort.h liefert dieselbe Reihenfolge wie std::stable_sort.
//
// Gegen std::stable_sort mit dem Vergleich der Lichtliste (a.wert > b.wert bzw. a.tiefe < b.tiefe): Zufallszahlen,
// viele gleiche Schluessel, +-0, +-unendlich, denormale, kleine und grosse Mengen (unter und ueber der Grenze zum
// Zaehlen). Ein NaN: die Funktion aendert nichts und sagt es.
#include "bhed/stabilsort.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <limits>
#include <random>

namespace {

int g_fehler = 0;

void pruefe(bool ok, const char* was, std::size_t n, int art) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHLER: %s (n %zu, Art %d)\n", was, n, art);
    }
}

struct Eintrag {
    float wert = 0;
    std::uint32_t nr = 0;
};

std::vector<float> schluessel(std::size_t n, int art, std::mt19937& zufall) {
    std::vector<float> s(n);
    std::uniform_real_distribution<float> gleich(-1000.0F, 1000.0F);
    std::uniform_int_distribution<int> wenig(-3, 3);
    std::uniform_int_distribution<int> wuerfel(0, 15);
    for (std::size_t i = 0; i < n; ++i) {
        switch (art) {
            case 0: s[i] = gleich(zufall); break;                                  // alle verschieden (fast)
            case 1: s[i] = static_cast<float>(wenig(zufall)); break;               // viele gleiche, mit 0
            case 2: s[i] = wenig(zufall) == 0 ? -0.0F : 0.0F; break;               // nur +-0
            case 3: {                                                              // Sonderwerte gemischt
                const int w = wuerfel(zufall);
                s[i] = w == 0   ? std::numeric_limits<float>::infinity()
                       : w == 1 ? -std::numeric_limits<float>::infinity()
                       : w == 2 ? std::numeric_limits<float>::denorm_min()
                       : w == 3 ? -std::numeric_limits<float>::denorm_min()
                       : w == 4 ? -0.0F
                       : w == 5 ? 0.0F
                       : w == 6 ? std::numeric_limits<float>::max()
                       : w == 7 ? std::numeric_limits<float>::lowest()
                       : w == 8 ? std::numeric_limits<float>::min()
                                : gleich(zufall) * 1.0e-30F;
                break;
            }
            case 4: s[i] = static_cast<float>(i); break;                           // schon sortiert
            case 5: s[i] = static_cast<float>(n - i); break;                       // umgekehrt
            default: s[i] = std::ldexp(gleich(zufall), wenig(zufall) * 20); break; // viele Groessenordnungen
        }
    }
    return s;
}

}  // namespace

int main() {
    std::mt19937 zufall(20261112U);
    std::vector<std::uint32_t> reihe;
    std::vector<std::uint64_t> arbeit;
    std::size_t faelle = 0;
    for (const std::size_t n : {std::size_t{0}, std::size_t{1}, std::size_t{2}, std::size_t{3}, std::size_t{17}, std::size_t{127}, std::size_t{128},
                                std::size_t{129}, std::size_t{1000}, std::size_t{4099}, std::size_t{70001}}) {
        for (int art = 0; art < 7; ++art) {
            for (int runde = 0; runde < (n > 10000 ? 2 : 6); ++runde) {
                const std::vector<float> s = schluessel(n, art, zufall);
                for (const bool ab : {false, true}) {
                    std::vector<Eintrag> soll(n);
                    for (std::size_t i = 0; i < n; ++i) { soll[i] = {s[i], static_cast<std::uint32_t>(i)}; }
                    if (ab) {
                        std::stable_sort(soll.begin(), soll.end(), [](const Eintrag& a, const Eintrag& b) { return a.wert > b.wert; });
                    } else {
                        std::stable_sort(soll.begin(), soll.end(), [](const Eintrag& a, const Eintrag& b) { return a.wert < b.wert; });
                    }
                    const bool ok = bhed::stabileReihenfolge(s.data(), n, ab, reihe, arbeit);
                    pruefe(ok, "ohne NaN muss die Reihenfolge kommen", n, art);
                    pruefe(reihe.size() == n, "Laenge der Reihenfolge", n, art);
                    bool gleich = reihe.size() == n;
                    for (std::size_t i = 0; gleich && i < n; ++i) { gleich = reihe[i] == soll[i].nr; }
                    pruefe(gleich, ab ? "absteigend wie std::stable_sort" : "aufsteigend wie std::stable_sort", n, art);
                    ++faelle;
                }
            }
        }
    }
    // NaN: nichts aendern, false
    for (const std::size_t n : {std::size_t{5}, std::size_t{500}}) {
        std::vector<float> s = schluessel(n, 0, zufall);
        s[n / 2] = std::numeric_limits<float>::quiet_NaN();
        reihe.assign(3, 77U);
        const bool ok = bhed::stabileReihenfolge(s.data(), n, true, reihe, arbeit);
        pruefe(!ok, "mit NaN muss false kommen", n, -1);
        pruefe(reihe.size() == 3U && reihe[0] == 77U, "mit NaN bleibt die Reihenfolge unberuehrt", n, -1);
        ++faelle;
    }
    std::printf("%s (%zu Faelle, %d Fehler)\n", g_fehler == 0 ? "stabilsort ok" : "stabilsort FEHLER", faelle, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
