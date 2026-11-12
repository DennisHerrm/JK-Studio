// stabilsort.h - TEMPO (12.11.2026): stabile Reihenfolge nach einem float-Schluessel in O(n).
//
// Die Lichtliste des Echtzeit-Lichts ordnet je Bild einige tausend Lichter zweimal (nach dem Beitrag am Auge, nach der
// Tiefe). std::stable_sort bewegte dabei die ganzen Eintraege (40 bzw. 16 Byte) durch n log n Vergleiche. Hier: die
// Schluessel als Ganzzahlen, drei Zaehldurchgaenge (LSD-Radix, 11 + 11 + 10 Bit), heraus kommt nur die Reihenfolge.
//
// Zusage: dieselbe Reihenfolge wie std::stable_sort mit dem Vergleich a < b (aufsteigend) bzw. a > b (absteigend) -
// gleiche Schluessel behalten ihre Reihenfolge, -0 und +0 gelten als gleich, +-unendlich ordnet wie der Vergleich.
// Mit einem NaN als Schluessel ist der Vergleich keine Ordnung: dann aendert die Funktion nichts und liefert false
// (der Aufrufer sortiert wie bisher mit std::stable_sort). Pruefung: tests/stabilsort.cpp.
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace bhed {

// reihe[k] = Index des k-ten Elements in sortierter Folge (0 .. n-1). arbeit: Zwischenspeicher, den der Aufrufer ueber
// die Bilder behaelt (keine Speicheranforderung je Bild).
[[nodiscard]] inline bool stabileReihenfolge(const float* schluessel, std::size_t n, bool absteigend, std::vector<std::uint32_t>& reihe,
                                             std::vector<std::uint64_t>& arbeit) {
    if (n > 0xFFFFFFFFULL) { return false; }
    // (Schluessel als ordnungserhaltende Ganzzahl) << 32 | Index
    arbeit.resize(n * 2U);
    std::uint64_t* a = arbeit.data();
    std::uint64_t* b = arbeit.data() + n;
    for (std::size_t i = 0; i < n; ++i) {
        float f = schluessel[i];
        if (f != f) { return false; }          // NaN
        if (absteigend) { f = -f; }
        if (f == 0.0F) { f = 0.0F; }           // -0 wie +0
        std::uint32_t bits = 0;
        std::memcpy(&bits, &f, sizeof(bits));
        // negative Zahlen: alle Bits umkehren (groesserer Betrag = kleiner), positive: das Vorzeichenbit setzen
        const std::uint32_t k = (bits & 0x80000000U) != 0U ? ~bits : (bits | 0x80000000U);
        a[i] = (static_cast<std::uint64_t>(k) << 32) | static_cast<std::uint64_t>(i);
    }
    reihe.resize(n);
    if (n < 128U) {
        // wenige: Einfuegen (stabil) - die Zaehlfelder lohnen nicht
        for (std::size_t i = 1; i < n; ++i) {
            const std::uint64_t x = a[i];
            std::size_t j = i;
            while (j > 0 && (a[j - 1] >> 32) > (x >> 32)) {
                a[j] = a[j - 1];
                --j;
            }
            a[j] = x;
        }
    } else {
        static constexpr int kSchub[3] = {32, 43, 54};
        static constexpr std::uint32_t kMaske[3] = {0x7FFU, 0x7FFU, 0x3FFU};
        std::uint32_t zahl[3][2048];
        std::memset(zahl, 0, sizeof(zahl));
        for (std::size_t i = 0; i < n; ++i) {
            for (int d = 0; d < 3; ++d) { ++zahl[d][static_cast<std::uint32_t>(a[i] >> kSchub[d]) & kMaske[d]]; }
        }
        for (int d = 0; d < 3; ++d) {
            std::uint32_t summe = 0;
            for (std::uint32_t& z : zahl[d]) {
                const std::uint32_t c = z;
                z = summe;
                summe += c;
            }
            for (std::size_t i = 0; i < n; ++i) { b[zahl[d][static_cast<std::uint32_t>(a[i] >> kSchub[d]) & kMaske[d]]++] = a[i]; }
            std::swap(a, b);
        }
    }
    for (std::size_t i = 0; i < n; ++i) { reihe[i] = static_cast<std::uint32_t>(a[i] & 0xFFFFFFFFULL); }
    return true;
}

}  // namespace bhed
