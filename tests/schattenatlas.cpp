// schattenatlas.cpp - rc610 LICHT1: Plaetze der Schattenwuerfel im Atlas (bhed/schattenatlas.h).
//
// Sollwerte von Hand: Seite 3072 x 2048 x 2 Bytes = 12 MiB; je Seite 4 Wuerfel zu 512, 16 zu 256, 64 zu 128, 256 zu 64;
// Groesse nach der Huelle: Texel am Rand 2 * Huelle / Groesse <= 3 (Huelle 768 -> 512, 384 -> 256, 192 -> 128, 96 -> 64).
#include "bhed/schattenatlas.h"

#include <array>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

using namespace bhed;

namespace {

int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

constexpr std::size_t kMiB = 1024U * 1024U;

void groessen() {
    pruefe(kAtlasSeitenBytes == 12U * kMiB, "Seite 12 MiB");
    pruefe(wuerfelJeSeite(512) == 4 && wuerfelJeSeite(256) == 16 && wuerfelJeSeite(128) == 64 && wuerfelJeSeite(64) == 256, "Wuerfel je Seite");
    pruefe(wuerfelBytes(512) == 3U * kMiB && wuerfelBytes(64) == 48U * 1024U, "Bytes je Wuerfel");
    pruefe(wuerfelGroesseFuer(10000.0F) == 512 && wuerfelGroesseFuer(769.0F) == 512 && wuerfelGroesseFuer(768.0F) == 512, "grosse Huelle 512");
    pruefe(wuerfelGroesseFuer(767.0F) == 512 && wuerfelGroesseFuer(384.0F) == 256 && wuerfelGroesseFuer(385.0F) == 512, "Grenze 384/385");
    pruefe(wuerfelGroesseFuer(192.0F) == 128 && wuerfelGroesseFuer(193.0F) == 256, "Grenze 192/193");
    pruefe(wuerfelGroesseFuer(96.0F) == 64 && wuerfelGroesseFuer(97.0F) == 128 && wuerfelGroesseFuer(1.0F) == 64, "Grenze 96/97, klein 64");
}

void belegen() {
    SchattenAtlas a;
    a.budgetSetzen(2U * kAtlasSeitenBytes);
    const AtlasPlatz p0 = a.belegen(512);
    const AtlasPlatz p1 = a.belegen(512);
    pruefe(p0.gueltig() && p0.seite == 0 && p0.wuerfel == 0 && p1.seite == 0 && p1.wuerfel == 1, "zwei 512er auf Seite 0");
    const AtlasPlatz k = a.belegen(64);
    pruefe(k.gueltig() && k.seite == 1 && k.wuerfel == 0 && k.groesse == 64, "64er bekommt eine eigene Seite");
    (void)a.belegen(512);
    (void)a.belegen(512);
    const AtlasPlatz voll = a.belegen(512);
    pruefe(!voll.gueltig() && a.seiten() == 2 && a.bytes() == 2U * kAtlasSeitenBytes, "Budget voll: kein dritter Platz");
    a.freigeben(p1);
    const AtlasPlatz wieder = a.belegen(512);
    pruefe(wieder.seite == 0 && wieder.wuerfel == 1, "freigegebener Platz kommt wieder");
    a.freigeben(k);
    const AtlasPlatz andere = a.belegen(256);   // die leere Seite 1 nimmt jetzt 256er
    pruefe(andere.gueltig() && andere.seite == 1 && andere.groesse == 256 && a.seiten() == 2, "leere Seite fuer eine andere Groesse");
    pruefe(a.belegt() == 5, "fuenf belegt");
    a.freigeben(andere);
    a.freigeben(andere);   // doppelt: nichts
    pruefe(a.belegt() == 4, "doppelt freigeben aendert nichts");
    pruefe(a.belegteSeiten() == 1 && a.seiten() == 2, "eine Seite belegt, zwei da");
    a.kuerzen();
    pruefe(a.seiten() == 1 && a.bytes() == kAtlasSeitenBytes, "die leere Seite am Ende ist weg");
}

void verteilen() {
    // 320 Lichter mit Huelle 750 (Hangar, light 75): 512 je Licht, 80 Seiten = 960 MiB
    const std::vector<float> hangar(320, 750.0F);
    std::vector<int> g = wuerfelGroessenVerteilen(hangar, 2048U * kMiB);
    bool alle = true;
    for (const int x : g) { alle = alle && x == 512; }
    pruefe(alle, "Hangar: alle 512 im Budget 2 GiB");
    // Budget 480 MiB = 40 Seiten: 320 Wuerfel zu 256 brauchen 20 Seiten, 512 brauchten 80 -> Mischung
    g = wuerfelGroessenVerteilen(hangar, 480U * kMiB);
    std::array<std::size_t, 4> n{};
    for (const int x : g) {
        for (std::size_t i = 0; i < 4; ++i) { n[i] += x == kWuerfelGroessen[i] ? 1U : 0U; }
    }
    pruefe(atlasSeitenFuer(n) <= 40 && n[2] == 0 && n[3] == 0 && n[0] + n[1] == 320 && n[0] > 0, "Hangar 480 MiB: 512 und 256, nichts kleiner");
    // kleinste Huellen zuerst kleiner
    // neun 512er brauchen 3 Seiten, das Budget hat 2: die ersten fuenf der kleinen Huellen werden 256 (4 x 512 + 5 x 256)
    const std::vector<float> neun = {2000.0F, 700.0F, 700.0F, 700.0F, 700.0F, 700.0F, 700.0F, 700.0F, 700.0F};
    g = wuerfelGroessenVerteilen(neun, 24U * kMiB);
    pruefe(g[0] == 512 && g[1] == 256 && g[5] == 256 && g[6] == 512 && g[8] == 512, "kleinste Huelle zuerst eine Stufe kleiner");
    // zu wenig fuer alles: die kleinsten ohne Wuerfel
    const std::vector<float> viele(600, 50.0F);   // 64er, 256 je Seite
    g = wuerfelGroessenVerteilen(viele, 24U * kMiB);   // 2 Seiten = 512 Wuerfel
    std::size_t ohne = 0;
    for (const int x : g) { ohne += x == 0 ? 1U : 0U; }
    pruefe(ohne == 88, "600 Lichter, Platz fuer 512: 88 ohne Wuerfel");
    pruefe(wuerfelGroessenVerteilen({}, 0).empty(), "keine Lichter");
}

}  // namespace

int main() {
    groessen();
    belegen();
    verteilen();
    std::printf("%s (%d Fehler)\n", g_fehler == 0 ? "schattenatlas ok" : "schattenatlas FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
