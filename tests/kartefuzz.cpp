// kartefuzz.cpp - Karten-Editor Stufe 6 Punkt 5: kaputte Karten- und BSP-Dateien.
//
// Ein Editor bekommt halbfertige Dateien, abgeschnittene Downloads und Handarbeit zu
// sehen; er darf nie abstuerzen. Gemutiert werden echte Eingaben (tests/karte/*.map,
// tests/karte/q3map2/*.map, die .bsp/.prt daneben):
//   - abschneiden, einzelne Bytes kippen, Zeilen loeschen oder doppeln, Bloecke vertauschen
//   - riesige oder kaputte Zahlen (1e39, -1e39, nan, inf, 0x7fffffff), sehr lange Woerter
//     (Shadernamen mit 70000 Zeichen, GT#676), offene oder ueberzaehlige Klammern
//   - patchDef2/3 mit falschen oder riesigen Groessen, brushDef mit kaputter Matrix
// Forderungen:
//   1. karteLesen, karteImportieren, bspLesen/bspEntitaetenText, portaleLaden,
//      punktdateiLesen: kein Absturz (unter ASan/UBSan: kein Treffer)
//   2. was karteLesen liest, wird nach hoechstens vier Schreib-Durchgaengen fest (siehe kartePruefen)
//
//   kartefuzz <tests/karte> [runden]
#include "bhed/karte/baumonitor.h"
#include "bhed/karte/bspoeffnen.h"
#include "bhed/karte/kartendatei.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/portale.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace bhed::karte;

namespace {

int g_fehler = 0;

std::string lesen(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

std::vector<std::string> zeilen(const std::string& t) {
    std::vector<std::string> z;
    std::size_t a = 0;
    while (a <= t.size()) {
        const std::size_t e = t.find('\n', a);
        z.push_back(t.substr(a, e == std::string::npos ? std::string::npos : e - a));
        if (e == std::string::npos) { break; }
        a = e + 1;
    }
    return z;
}

std::string verbinden(const std::vector<std::string>& z) {
    std::string t;
    for (std::size_t i = 0; i < z.size(); ++i) {
        t += z[i];
        if (i + 1 < z.size()) { t += '\n'; }
    }
    return t;
}

const char* const kGift[] = {"1e39", "-1e39", "nan", "-nan", "inf", "-inf", "2147483647", "-2147483648", "4294967296", "1e-45", "0", "-0",
                             "(", ")", "{", "}", "\"", "//", "/*", "patchDef2", "patchDef3", "brushDef", "brushDef3", "( 1000000 1000000 0 0 0 )",
                             "( 0 0 0 0 0 )", "( 3 3 0 0 0 )", "( ( 1e39 0 0 ) ( 0 nan 0 ) )"};

std::string mutieren(std::string t, std::mt19937& z) {
    std::uniform_int_distribution<int> art(0, 9);
    const int schritte = 1 + static_cast<int>(z() % 4);
    for (int s = 0; s < schritte && !t.empty(); ++s) {
        const std::size_t pos = z() % t.size();
        switch (art(z)) {
        case 0: t.resize(pos); break;   // abgeschnitten
        case 1: t[pos] = static_cast<char>(z() & 0xFF); break;   // ein Byte gekippt
        case 2: {   // eine Zeile weg
            auto v = zeilen(t);
            if (!v.empty()) { v.erase(v.begin() + static_cast<long>(z() % v.size())); }
            t = verbinden(v);
            break;
        }
        case 3: {   // eine Zeile doppelt
            auto v = zeilen(t);
            if (!v.empty()) {
                const std::size_t i = z() % v.size();
                v.insert(v.begin() + static_cast<long>(i), v[i]);
            }
            t = verbinden(v);
            break;
        }
        case 4: {   // ein Wort durch Gift ersetzt
            const std::size_t a = t.find_first_of(" \t\n", pos);
            if (a == std::string::npos) { break; }
            const std::size_t e = t.find_first_of(" \t\n", a + 1);
            t.replace(a + 1, (e == std::string::npos ? t.size() : e) - a - 1, kGift[z() % (sizeof(kGift) / sizeof(kGift[0]))]);
            break;
        }
        case 5: t.insert(pos, std::string(70000, 'x')); break;   // sehr langes Wort (GT#676)
        case 6: t.insert(pos, std::string(1 + z() % 64, '{')); break;
        case 7: t.insert(pos, std::string(1 + z() % 64, '}')); break;
        case 8: {   // zwei Zeilenbloecke vertauscht
            auto v = zeilen(t);
            if (v.size() > 4) {
                const std::size_t a = z() % v.size();
                const std::size_t b = z() % v.size();
                std::swap(v[a], v[b]);
            }
            t = verbinden(v);
            break;
        }
        default: t.insert(pos, std::string(1, '\0')); break;   // NUL mitten im Text
        }
    }
    return t;
}

std::string bspMutieren(std::string d, std::mt19937& z) {
    const int schritte = 1 + static_cast<int>(z() % 3);
    for (int s = 0; s < schritte && !d.empty(); ++s) {
        switch (z() % 5) {
        case 0: d.resize(z() % d.size()); break;
        case 1: d[z() % d.size()] = static_cast<char>(z() & 0xFF); break;
        case 2:   // Lump-Verzeichnis: Versatz/Laenge kaputt
            if (d.size() >= 8 + 18 * 8) {
                const std::size_t i = 8 + (z() % 36) * 4;
                const std::int32_t werte[] = {-1, 0x7fffffff, static_cast<std::int32_t>(0x80000000U), static_cast<std::int32_t>(d.size()),
                                              static_cast<std::int32_t>(d.size() + 1), 3};
                const std::int32_t w = werte[z() % 6];
                std::memcpy(d.data() + i, &w, 4);
            }
            break;
        case 3: d.replace(0, std::min<std::size_t>(4, d.size()), z() % 2 ? "IBSP" : "RBSP"); break;
        default: d.insert(z() % d.size(), std::string(1 + z() % 32, static_cast<char>(z() & 0xFF))); break;
        }
    }
    return d;
}

void kartePruefen(const std::string& text, const std::string& woher) {
    Karte k;
    (void)karteLesen(text, k);
    // Wie in NRC kann ein Schreiben Teile verlieren, die danach entartet sind: ein Pinsel ohne
    // beitragende Flaeche wird nur als "// brush N" geschrieben (Map-Schreiber: Kommentar;
    // BrushTokenExporter: ohne hasContributingFaces nichts), und die 10 Nachkommastellen von
    // Decimal machen aus 1e-45 eine 0 - zwei gleiche Ebenenpunkte, beim naechsten Lesen keine
    // Ebene. Jeder Durchgang kann also nur Entartetes entfernen; spaetestens nach vier
    // Durchgaengen muss der Text fest sein.
    std::string a = karteSchreiben(k);
    std::string b;
    for (int durchgang = 0; durchgang < 4; ++durchgang) {
        Karte k2;
        (void)karteLesen(a, k2);
        b = karteSchreiben(k2);
        if (a == b) { break; }
        a = b;
        b.clear();
    }
    if (a != b) {
        ++g_fehler;
        if (g_fehler <= 5) {
            std::printf("FEHL  kein Festpunkt nach %s (%zu / %zu Bytes)\n", woher.c_str(), a.size(), b.size());
            std::ofstream("kartefuzz_fehl_" + std::to_string(g_fehler) + ".map", std::ios::binary) << text;
        }
    }
    // Import in eine Karte mit Inhalt (Map_ImportFile)
    Karte ziel;
    (void)karteLesen("{\n\"classname\" \"worldspawn\"\n}\n", ziel);
    karteImportieren(ziel, text);
    (void)karteSchreiben(ziel);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "Aufruf: kartefuzz <tests/karte> [runden]\n");
        return 2;
    }
    if (std::string(argv[1]) == "--einzeln" && argc > 2) {   // Untersuchung: beide Schreibdurchgaenge in Dateien
        Karte k;
        (void)karteLesen(lesen(argv[2]), k);
        const std::string a = karteSchreiben(k);
        Karte k2;
        (void)karteLesen(a, k2);
        const std::string b = karteSchreiben(k2);
        std::ofstream(std::string(argv[2]) + ".a", std::ios::binary) << a;
        std::ofstream(std::string(argv[2]) + ".b", std::ios::binary) << b;
        std::printf("%s\n", a == b ? "Festpunkt" : "verschieden");
        return a == b ? 0 : 1;
    }
    const int runden = argc > 2 ? std::atoi(argv[2]) : 2000;
    std::vector<std::string> karten;
    std::vector<std::string> bsps;
    std::vector<std::string> prts;
    for (const auto& e : fs::recursive_directory_iterator(argv[1])) {
        if (!e.is_regular_file()) { continue; }
        const std::string ext = e.path().extension().string();
        if (ext == ".map") { karten.push_back(lesen(e.path())); }
        if (ext == ".bsp") { bsps.push_back(lesen(e.path())); }
        if (ext == ".prt") { prts.push_back(lesen(e.path())); }
    }
    if (karten.empty() || bsps.empty() || prts.empty()) {
        std::fprintf(stderr, "kartefuzz: keine Eingaben in %s (%zu .map, %zu .bsp, %zu .prt)\n", argv[1], karten.size(), bsps.size(), prts.size());
        return 2;
    }
    {   // Decimal (ostream_write): grosse Werte passen nicht in NRCs 22 Zeichen - der Wert muss bleiben
        const std::string t = R"({
"classname" "worldspawn"
{
( 100000000000000000000 64 64 ) ( 64 0 64 ) ( 0 64 64 ) a/b 0 0 0 0.5 0.5 0 0 0
( 64 64 64 ) ( 0 64 64 ) ( 64 64 0 ) a/b 0 0 0 0.5 0.5 0 0 0
( 64 64 64 ) ( 64 64 0 ) ( 64 0 64 ) a/b 0 0 0 0.5 0.5 0 0 0
( 0 0 0 ) ( 64 0 0 ) ( 0 64 0 ) a/b 0 0 0 0.5 0.5 0 0 0
( 0 0 0 ) ( 0 0 64 ) ( 64 0 0 ) a/b 0 0 0 0.5 0.5 0 0 0
( 0 0 0 ) ( 0 64 0 ) ( 0 0 64 ) a/b 0 0 0 0.5 0.5 0 0 0
}
}
)";
        Karte k;
        (void)karteLesen(t, k);
        const std::string a = karteSchreiben(k);
        if (a.find("( 100000000000000000000 64 64 )") == std::string::npos) {
            ++g_fehler;
            std::printf("FEHL  Decimal: 1e20 als Ebenenpunkt nicht erhalten:\n%s\n", a.c_str());
        }
    }
    std::mt19937 z(20261025U);
    // die Eingaben selbst
    for (const std::string& t : karten) { kartePruefen(t, "Original"); }
    for (int r = 0; r < runden; ++r) {
        const std::string& basis = karten[static_cast<std::size_t>(r) % karten.size()];
        kartePruefen(mutieren(basis, z), "Runde " + std::to_string(r));
        if (r % 4 == 0) {
            BspDatei b;
            const std::string d = bspMutieren(bsps[static_cast<std::size_t>(r) % bsps.size()], z);
            if (bspLesen(d, b)) {
                const std::string ents = bspEntitaetenText(b);
                (void)bspHatLichter(ents);
                (void)bspEntitaetenEinsetzen(d, d);
            }
            std::vector<BspPortal> portale;
            bool hint = false;
            PortalZufall zufall;
            const std::string p = mutieren(prts[static_cast<std::size_t>(r) % prts.size()], z);
            (void)portaleLaden("fuzz.prt", &p, portale, hint, zufall);
            std::string meldung;
            (void)punktdateiLesen(mutieren("1 2 3\n4 5 6\n-7.5 8 9\n", z), &meldung);
        }
    }
    std::printf("kartefuzz: %zu Karten, %zu BSP, %zu Portaldateien, %d Runden, %d Fehlschlaege\n", karten.size(), bsps.size(), prts.size(), runden, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
