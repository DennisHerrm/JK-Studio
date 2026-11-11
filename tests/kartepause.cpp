// kartepause.cpp - Karten-Editor, vis2c: die Dateischritte von "Bauen anhalten" und "Bauen fortsetzen"
// (bhed/karte/baupause.h) als Befehle, fuer den ctest q3map2_baupause (tools/karte/q3map2_baupause.py). Der Test
// baut mit bhed_q3map2 und ruft zwischen den Schritten genau das auf, was der Editor aufruft.
//
// Aufruf: kartepause <befehl> ...
//   visstand <befehle.txt>              je Zeile ein Befehl, Ausgabe mit -bhedvisstate (befehlMitVisStand)
//   sichern <stamm> <schritt> <befehle.txt> [<ausgabe-des-schritts.txt>]
//                                       Zwischenstand anlegen (bauPauseSichern); mit der Ausgabe eines beendeten
//                                       Licht-Schritts vorher dessen Cache retten (bauLichtCacheRettenDatei)
//   befehlealt <stamm>                  die Befehle ab dem angehaltenen Schritt auf den Kopien, je Zeile einer
//   altfertig <stamm>                   Ergebnisse neben die Karte (bauPauseAltFertig)
//   aktuell <stamm>                     VIS-Zwischenstand zurueck neben die .bsp (bauPauseAktuellVorbereiten)
//   vorlicht <stamm>                    die .bsp vor einem Licht-Schritt sichern (bauVorLichtSichern)
//   vorlichtweg <stamm>                 ... und nach dem Schritt wieder weg
//   lichtretten <stamm> <ausgabe.txt>   nur den Cache retten, die .bsp von vor dem Schritt zurueck (Abbrechen)
// Rueckgabe 0, wenn es gelang; die Ausgabe nennt, was getan wurde.
#include "bhed/karte/baufortschritt.h"
#include "bhed/karte/baupause.h"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace bhed::karte;

namespace {

std::string lies(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

std::vector<std::string> zeilen(const std::string& p) {
    std::vector<std::string> z;
    std::istringstream s(lies(p));
    for (std::string x; std::getline(s, x);) {
        if (!x.empty() && x.back() == '\r') { x.pop_back(); }
        if (!x.empty()) { z.push_back(x); }
    }
    return z;
}

// die Ausgabe eines Schritts, wie der Editor sie liest
BauSchrittBeobachter beobachten(const std::string& datei) {
    BauSchrittBeobachter b;
    b.text(lies(datei), 0.0);
    return b;
}

int lichtRetten(const std::string& stamm, const BauSchrittBeobachter& b) {
    const BauLichtCache& c = b.lichtCache();
    const bool gerettet = bauLichtCacheRettenDatei(stamm, c);
    std::printf("light cache: %d of %d passes written, %s\n", c.geschrieben, c.paesse, gerettet ? "rescued" : "not rescued");
    return gerettet ? c.geschrieben : 0;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string befehl = argc > 1 ? argv[1] : "";
    if (befehl == "visstand" && argc == 3) {
        for (const std::string& z : zeilen(argv[2])) { std::printf("%s\n", befehlMitVisStand(z).c_str()); }
        return 0;
    }
    if (befehl == "sichern" && (argc == 5 || argc == 6)) {
        BauPause p;
        p.eintrag = "ctest";
        p.stamm = argv[2];
        p.karte = p.stamm + ".map";
        p.mapDatei = p.stamm + ".map";
        p.schritt = static_cast<std::size_t>(std::atoi(argv[3]));
        p.befehle = zeilen(argv[4]);
        p.fertig.assign(p.schritt, 1.0);
        p.zeit = "ctest";
        if (argc == 6) {
            const BauSchrittBeobachter b = beobachten(argv[5]);
            p.lichtPaesse = lichtRetten(p.stamm, b);
            p.lichtPaesseVon = b.lichtCache().paesse;
            if (b.visStand().zustand == BauVisStand::Zustand::Angehalten) {
                p.visFertig = b.visStand().fertig;
                p.visAlle = b.visStand().alle;
            }
        }
        p.kartenHash = bauDateiHash(p.mapDatei);
        std::string fehler;
        if (!bauPauseSichern(p, &fehler)) {
            std::printf("FEHLER %s\n", fehler.c_str());
            return 1;
        }
        std::printf("saved state: step %d, vis %d of %d, light passes %d of %d, %lld bytes\n", static_cast<int>(p.schritt), p.visFertig, p.visAlle,
                    p.lichtPaesse, p.lichtPaesseVon, bauPauseGroesse(p.stamm));
        return 0;
    }
    BauPause p;
    if (befehl == "befehlealt" && argc == 3 && bauPauseLesen(argv[2], p)) {
        const std::vector<std::string> b = bauPauseBefehleAlt(p);
        for (std::size_t i = p.schritt; i < b.size(); ++i) { std::printf("%s\n", b[i].c_str()); }
        return 0;
    }
    if (befehl == "altfertig" && argc == 3 && bauPauseLesen(argv[2], p)) {
        std::string fehler;
        if (!bauPauseAltFertig(p, &fehler)) {
            std::printf("FEHLER %s\n", fehler.c_str());
            return 1;
        }
        return 0;
    }
    if (befehl == "aktuell" && argc == 3 && bauPauseLesen(argv[2], p)) {
        std::printf("vis state %s\n", bauPauseAktuellVorbereiten(p) ? "put back" : "none");
        return 0;
    }
    if (befehl == "vorlicht" && argc == 3) { return bauVorLichtSichern(argv[2]) ? 0 : 1; }
    if (befehl == "vorlichtweg" && argc == 3) {
        bauVorLichtWeg(argv[2]);
        return 0;
    }
    if (befehl == "lichtretten" && argc == 4) {
        (void)lichtRetten(argv[2], beobachten(argv[3]));
        return bauVorLichtZurueck(argv[2]) ? 0 : 1;
    }
    std::printf("kartepause: unknown command or no saved state\n");
    return 2;
}
