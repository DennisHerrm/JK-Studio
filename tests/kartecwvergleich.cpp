// kartecwvergleich.cpp - CW-IMPORT, Gegenprobe "drin, aber nicht Standard" (shank 14.11.): was der Optimierer auf einer Karte meldet
// und was der Karten-Editor beim Speichern schreibt, als Text - einmal mit dem Kern dieses Zweigs, einmal mit dem Kern von karte1
// 348f0e345 (dieselbe Datei gegen dessen Kopfdateien und bhed_core.lib uebersetzt, Y:\werkzeuge\cwimport\basis). Gleiche Ausgabe =
// gleiches Verhalten. Ohne den Schalter (PruefOptionen::clonewars, BHED_KARTE_CLONEWARS_SCHALTER) kennt der alte Kern nur "aus".
//
// Aufruf: kartecwvergleich <ausgabe.txt> an|aus <name>=<karte.map>[,<karte.bsp>] ...
//   je Karte: die Befunde (Kennung, Schwere, Anzahl, Text, Objekte), danach die Grenzen (Wert/erlaubt/Ampel) und die Karte so,
//   wie karteSchreiben sie nach dem Lesen schreibt (FNV-1a und Laenge; die ganze Datei als <ausgabe>.<name>.map).
// Ohne Argumente: eine eingebaute kleine Karte (ctest karte_cwvergleich: laeuft und schreibt, Schalter aus meldet keine
// Clone-Wars-Pruefung).
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/optimierer.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace bhed::karte;

namespace {

std::string lesen(const std::string& pfad) {
    std::ifstream f(pfad, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

std::uint64_t fnv(const std::string& s) {
    std::uint64_t h = 1469598103934665603ULL;
    for (const char c : s) {
        h ^= static_cast<unsigned char>(c);
        h *= 1099511628211ULL;
    }
    return h;
}

// Die Kennungen der Clone-Wars-Pruefungen (src/karte/optimierer_cw.cpp)
const std::set<std::string>& cwKennungen() {
    static const std::set<std::string> k = {"laufohnekollision", "kollisionsabstand", "unsichtbarekollision", "fastdeckungsgleich", "stufehoch",
                                            "tuerklein",         "durchgangeng",      "gangeng",              "bildnpot",           "bildfehlt",
                                            "bildgroesse",       "modellteilflaeche", "modellweltkoord",      "md3grenzen",         "requisitschwebt",
                                            "requisitsteckt"};
    return k;
}

struct Ergebnis {
    std::string text;
    int cwBefunde = 0;
};

Ergebnis karte(const std::string& name, const std::string& mapText, const std::string& bsp, bool an, const std::string& ausgabe) {
    Ergebnis r;
    std::ostringstream o;
    Karte k;
    (void)karteLesen(mapText, k);
    k.anmelden();
    const KartenAbbild a = kartenAbbild(k);
    ZaehlOptionen zo;
    zo.spiel = GrenzSpiel::MovieDuels;   // CW-PRUEF lief die Originalkarten mit Regelwerk Auto = Movie Duels
    std::vector<Zaehlung> z = zaehleKarte(a, zo);
    BspAnalyse ba;
    if (!bsp.empty()) {
        ba = analysiereBsp(bsp, zo);
        for (const Zaehlung& x : ba.werte) { z.push_back(x); }
    }
    PruefOptionen po;
    po.spiel = GrenzSpiel::MovieDuels;
    po.bewertungen = bewerten(po.spiel, z, 0.9);
    if (!bsp.empty() && ba.ok) {
        po.bsp = &ba;
        po.bspDaten = &bsp;
    }
#ifdef BHED_KARTE_CLONEWARS_SCHALTER
    po.clonewars = an;
#else
    if (an) { o << "FEHLER: dieser Kern kennt den Schalter nicht\n"; }
#endif
    const std::vector<Befund> b = pruefeKarte(a, po);
    o << "== " << name << " (" << (an ? "an" : "aus") << ")\n== Befunde: " << b.size() << "\n";
    for (const Befund& x : b) {
        if (cwKennungen().count(x.pruefung) != 0U) { ++r.cwBefunde; }
        o << x.pruefung << " | " << schwereName(x.schwere) << " | " << x.anzahl << " | " << x.detail << " | " << x.objekte.size() << " Objekte\n";
        for (const ObjektRef& ref : x.objekte) { o << "    " << ref.entitaet << "/" << ref.primitiv << "/" << ref.flaeche << " " << ref.text << "\n"; }
    }
    o << "== Grenzen\n";
    for (const Bewertung& w : po.bewertungen) {
        o << (w.grenze != nullptr ? w.grenze->id : std::string("?")) << " " << w.zaehlung.wert << "/" << (w.grenze != nullptr ? w.grenze->erlaubt : 0) << " "
          << ampelName(w.ampel) << "\n";
    }
    const std::string gespeichert = karteSchreiben(k);
    o << "== Gespeichert: " << gespeichert.size() << " Bytes, FNV-1a " << std::hex << fnv(gespeichert) << std::dec << "\n";
    if (!ausgabe.empty()) {
        std::ofstream m(ausgabe + "." + name + ".map", std::ios::binary);
        m << gespeichert;
    }
    r.text = o.str();
    return r;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        // ctest: eine kleine Karte mit einer Stufe 24 hoch (stufehoch, wenn die Clone-Wars-Pruefungen laufen)
        std::string t = "{\n\"classname\" \"worldspawn\"\n";
        const auto kasten = [&t](int x0, int y0, int z0, int x1, int y1, int z1) {
            char b[1024];
            std::snprintf(b, sizeof(b),
                          "{\n( %d %d %d ) ( %d %d %d ) ( %d %d %d ) textures/test/fest 0 0 0 0.5 0.5 0 0 0\n"
                          "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) textures/test/fest 0 0 0 0.5 0.5 0 0 0\n"
                          "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) textures/test/fest 0 0 0 0.5 0.5 0 0 0\n"
                          "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) textures/test/fest 0 0 0 0.5 0.5 0 0 0\n"
                          "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) textures/test/fest 0 0 0 0.5 0.5 0 0 0\n"
                          "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) textures/test/fest 0 0 0 0.5 0.5 0 0 0\n}\n",
                          x0, y0, z0, x0, y1, z0, x0, y0, z1,   // -x
                          x1, y0, z0, x1, y0, z1, x1, y1, z0,   // +x
                          x0, y0, z0, x0, y0, z1, x1, y0, z0,   // -y
                          x0, y1, z0, x1, y1, z0, x0, y1, z1,   // +y
                          x0, y0, z0, x1, y0, z0, x0, y1, z0,   // -z
                          x0, y0, z1, x0, y1, z1, x1, y0, z1);  // +z
            t += b;
        };
        kasten(-512, -512, -16, 512, 512, 0);
        kasten(0, -128, 0, 192, 128, 24);
        t += "}\n{\n\"classname\" \"info_player_start\"\n\"origin\" \"-256 0 24\"\n}\n";
        const Ergebnis aus = karte("test", t, std::string(), false, std::string());
        int fehler = 0;
        if (aus.cwBefunde != 0) {
            std::printf("FEHL  Schalter aus: %d Clone-Wars-Befunde\n", aus.cwBefunde);
            ++fehler;
        }
#ifdef BHED_KARTE_CLONEWARS_SCHALTER
        const Ergebnis an = karte("test", t, std::string(), true, std::string());
        if (an.cwBefunde == 0) {
            std::printf("FEHL  Schalter an: keine Clone-Wars-Befunde (Stufe 24 erwartet)\n");
            ++fehler;
        }
#endif
        std::printf("%s", aus.text.c_str());
        std::printf("kartecwvergleich: %d Fehlschlaege\n", fehler);
        return fehler == 0 ? 0 : 1;
    }
    if (argc < 4) {
        std::fprintf(stderr, "Aufruf: kartecwvergleich <ausgabe.txt> an|aus <name>=<karte.map>[,<karte.bsp>] ...\n");
        return 2;
    }
    const std::string ausgabe = argv[1];
    const bool an = std::string(argv[2]) == "an";
    std::string alles;
    for (int i = 3; i < argc; ++i) {
        const std::string arg = argv[i];
        const std::size_t gleich = arg.find('=');
        if (gleich == std::string::npos) { continue; }
        const std::string name = arg.substr(0, gleich);
        std::string map = arg.substr(gleich + 1);
        std::string bsp;
        if (const std::size_t komma = map.find(','); komma != std::string::npos) {
            bsp = map.substr(komma + 1);
            map = map.substr(0, komma);
        }
        const Ergebnis e = karte(name, lesen(map), bsp.empty() ? std::string() : lesen(bsp), an, ausgabe);
        alles += e.text;
        std::printf("%s: %d Befunde-Zeilen, davon Clone-Wars %d\n", name.c_str(), static_cast<int>(std::count(e.text.begin(), e.text.end(), '\n')),
                    e.cwBefunde);
    }
    std::ofstream f(ausgabe, std::ios::binary);
    f << alles;
    return 0;
}
