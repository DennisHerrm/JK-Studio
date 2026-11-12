// lichthash.cpp - lcfix: der Hash und die Schluessel des Licht-Caches ohne Karte (ctest q3map2_lichthash)
//
// Fund P4-2 der Codepruefung seit rc611: BhedHash::daten legte die Laenge des Rests (1-7 Bytes) in die Bits 48-50 des
// letzten Worts - bei 7 Bytes liegt dort das letzte Byte, seine unteren drei Bits gingen im "oder" unter. Zwei Texte
// der Laenge 7, 15, 23 ..., die sich nur darin unterscheiden ("1 1 0.5" / "1 1 0.2"), bekamen denselben Abdruck.
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../src/q3map2/bhed_lichtcache.h"

namespace {
int fehler = 0;
void expect(const char* was, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was);
    if (!ok) { ++fehler; }
}

BhedSig vonText(const char* s) {
    BhedHash h;
    h.text(s);
    return h.ende();
}

BhedSig vonDaten(const std::vector<unsigned char>& d) {
    BhedHash h;
    h.daten(d.data(), d.size());
    return h.ende();
}
}  // namespace

int main() {
    expect("\"1 1 0.5\" und \"1 1 0.2\" (7 Zeichen) haben verschiedene Abdruecke", !(vonText("1 1 0.5") == vonText("1 1 0.2")));
    expect("\"255 99 99 256 1\" und \"255 99 99 256 7\" (15 Zeichen) haben verschiedene Abdruecke",
           !(vonText("255 99 99 256 1") == vonText("255 99 99 256 7")));
    expect("\"-thresh\" und \"-thresi\" (7 Zeichen, letztes Zeichen ein Bit anders)", !(vonText("-thresh") == vonText("-thresi")));

    // jede Laenge 0..40, jedes Byte, jedes Bit: ein gekipptes Bit aendert den Abdruck
    bool jedesBit = true;
    int ersteLaenge = -1;
    for (int n = 0; n <= 40; ++n) {
        std::vector<unsigned char> d(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) { d[static_cast<std::size_t>(i)] = static_cast<unsigned char>(0x30 + i * 7); }
        const BhedSig grund = vonDaten(d);
        for (int i = 0; i < n; ++i) {
            for (int bit = 0; bit < 8; ++bit) {
                std::vector<unsigned char> e = d;
                e[static_cast<std::size_t>(i)] = static_cast<unsigned char>(e[static_cast<std::size_t>(i)] ^ (1u << bit));
                if (vonDaten(e) == grund) {
                    jedesBit = false;
                    if (ersteLaenge < 0) { ersteLaenge = n; }
                }
            }
        }
    }
    if (!jedesBit) { std::printf("        (zuerst bei Laenge %d)\n", ersteLaenge); }
    expect("Laenge 0 bis 40: jedes gekippte Bit jedes Bytes aendert den Abdruck", jedesBit);

    // die Laenge des Rests zaehlt: dieselben Bytes, mit Nullen verlaengert, sind andere Daten
    bool laengen = true;
    for (int n = 0; n < 24; ++n) {
        std::vector<unsigned char> d(static_cast<std::size_t>(n), static_cast<unsigned char>(0x41));
        std::vector<unsigned char> e = d;
        e.push_back(0);
        if (vonDaten(d) == vonDaten(e)) { laengen = false; }
    }
    expect("Laenge 0 bis 23: ein angehaengtes Nullbyte aendert den Abdruck", laengen);

    // der Rest eines Feldes vermischt sich nicht mit dem naechsten: (7 Bytes, dann 1 Byte) gegen (8 Bytes am Stueck)
    {
        const unsigned char acht[8] = {1, 2, 3, 4, 5, 6, 7, 8};
        BhedHash a, b;
        a.daten(acht, 7);
        a.daten(acht + 7, 1);
        b.daten(acht, 8);
        expect("7 Bytes und 1 Byte nacheinander sind nicht dasselbe wie 8 Bytes am Stueck", !(a.ende() == b.ende()));
    }

    // Fund P4-1 (c): -subdivisions steht vor -light, nicht in dessen Schaltern - es muss trotzdem im Schalter-Schluessel stehen
    {
        const std::vector<std::string> schalter = {"-fast", "-patchshadows"};
        expect("Schalter-Schluessel: -subdivisions 8 und 32 geben verschiedene Schluessel",
               !(BhedLichtSchalterSig("ja", schalter, 8) == BhedLichtSchalterSig("ja", schalter, 32)));
        expect("Schalter-Schluessel: gleiche Eingaben geben denselben Schluessel",
               BhedLichtSchalterSig("ja", schalter, 8) == BhedLichtSchalterSig("ja", schalter, 8));
        expect("Schalter-Schluessel: ein anderes Spiel gibt einen anderen Schluessel",
               !(BhedLichtSchalterSig("ja", schalter, 8) == BhedLichtSchalterSig("jk2", schalter, 8)));
    }

    std::printf(fehler == 0 ? "\nalle Proben des Licht-Cache-Hashes bestanden\n" : "\nFEHLGESCHLAGEN (%d)\n", fehler);
    return fehler == 0 ? 0 : 1;
}
