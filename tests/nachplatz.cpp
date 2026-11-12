// nachplatz.cpp - TEMPO (12.11.2026): bhed/nachplatz.h verhaelt sich wie std::map<int, T>.
//
// Dieselbe zufaellige Folge von Befehlen (setzen ueber operator[], lesen ueber find, loeschen, leeren, tauschen) geht an
// einen bhed::NachPlatz und an einen std::map; nach jedem Befehl muessen Groesse, groesster Platz, das Gefundene und der
// ganze Durchlauf (Reihenfolge, Platz, Wert) gleich sein.
#include "bhed/nachplatz.h"

#include <cstddef>
#include <cstdio>
#include <map>
#include <random>

namespace {

int g_fehler = 0;

void pruefe(bool ok, const char* was, int schritt) {
    if (!ok) {
        ++g_fehler;
        if (g_fehler <= 20) { std::printf("FEHLER: %s (Schritt %d)\n", was, schritt); }
    }
}

struct Quelle {   // wie gpu::BildQuelle
    const void* px = nullptr;
    int w = 0;
    int h = 0;
    bool operator==(const Quelle& o) const { return px == o.px && w == o.w && h == o.h; }
};

template <typename T, typename Wert>
void gleichPruefen(const bhed::NachPlatz<T>& feld, const std::map<int, T>& baum, int schritt, Wert gleich) {
    pruefe(feld.size() == baum.size(), "Groesse", schritt);
    pruefe(feld.empty() == baum.empty(), "leer", schritt);
    pruefe(feld.groessterPlatz() == (baum.empty() ? -1 : baum.rbegin()->first), "groesster Platz", schritt);
    auto b = baum.begin();
    std::size_t n = 0;
    for (const auto& [platz, wert] : feld) {
        if (b == baum.end()) { break; }
        pruefe(platz == b->first, "Durchlauf: Platz", schritt);
        pruefe(gleich(wert, b->second), "Durchlauf: Wert", schritt);
        ++b;
        ++n;
    }
    pruefe(n == baum.size() && b == baum.end(), "Durchlauf: Laenge", schritt);
}

}  // namespace

int main() {
    std::mt19937 zufall(20261112U);
    int schritte = 0;
    for (const int plaetze : {1, 7, 300, 5000}) {
        bhed::NachPlatz<int> feld;
        std::map<int, int> baum;
        bhed::NachPlatz<int> feld2;
        std::map<int, int> baum2;
        bhed::NachPlatz<Quelle> quellen;
        std::map<int, Quelle> quellenBaum;
        std::uniform_int_distribution<int> platzWahl(0, plaetze - 1);
        std::uniform_int_distribution<int> befehl(0, 99);
        for (int s = 0; s < 4000; ++s, ++schritte) {
            const int p = platzWahl(zufall);
            const int was = befehl(zufall);
            if (was < 45) {   // setzen
                feld[p] = s;
                baum[p] = s;
                quellen[p] = Quelle{&feld, s, p};
                quellenBaum[p] = Quelle{&feld, s, p};
            } else if (was < 60) {   // operator[] legt an (Wert T{})
                pruefe(feld[p] == baum[p], "operator[] liest bzw. legt an", s);
                pruefe(quellen[p] == quellenBaum[p], "operator[] Quelle", s);
            } else if (was < 85) {   // suchen, und was gefunden ist loeschen (wie holeBild bei veralteter Quelle)
                const auto f = feld.find(p);
                const auto b = baum.find(p);
                pruefe((f != feld.end()) == (b != baum.end()), "find: da oder nicht", s);
                if (f != feld.end() && b != baum.end()) {
                    pruefe(f->first == b->first && f->second == b->second, "find: Platz und Wert", s);
                    if (was >= 75) {
                        feld.erase(f);
                        baum.erase(b);
                    }
                }
            } else if (was < 88) {   // ausserhalb
                pruefe(feld.find(-1) == feld.end() && feld.find(plaetze + 100) == feld.end(), "find ausserhalb", s);
            } else if (was < 90) {
                feld.clear();
                baum.clear();
            } else if (was < 95) {   // tauschen wie tauscheSzene
                std::swap(feld, feld2);
                std::swap(baum, baum2);
            } else {   // im Durchlauf aendern (wie das Freigeben aller Bilder)
                for (auto& [platz, wert] : feld) { wert += platz; }
                for (auto& [platz, wert] : baum) { wert += platz; }
            }
            gleichPruefen(feld, baum, s, [](int a, int b) { return a == b; });
            if (s % 16 == 0) {
                gleichPruefen(feld2, baum2, s, [](int a, int b) { return a == b; });
                gleichPruefen(quellen, quellenBaum, s, [](const Quelle& a, const Quelle& b) { return a == b; });
            }
        }
    }
    std::printf("%s (%d Schritte, %d Fehler)\n", g_fehler == 0 ? "nachplatz ok" : "nachplatz FEHLER", schritte, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
