// kartestreuen.cpp - Stufe 9 Punkt 8: Modelle verteilen (ctest karte_streuen)
//
// Ein Boden (Oberkante 0) und ein Block (Oberkante 128): 30 Orte, alle auf einer begehbaren
// Oberseite, im Bereich, mindestens 48 auseinander; die Grenze laesst nur 5 zu; eingesetzt als
// misc_model_static (ein Befehl, Undo); der Optimierer zaehlt sie gegen MAX_MISC_ENTS 2000
// (Jedi Academy SP, Movie Duels; Jedi Outcast kennt die Grenze nicht).
#include <cmath>
#include <cstdio>
#include <string>

#include "bhed/karte/grenzen.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/optimierer.h"
#include "bhed/karte/rueckgaengig.h"
#include "bhed/karte/spielkollision.h"
#include "bhed/karte/streuen.h"

namespace {
int g_fehler = 0;
void expect(const char* was, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was);
    if (!ok) { ++g_fehler; }
}
std::string quader(int x0, int y0, int z0, int x1, int y1, int z1) {
    char z[1024];
    std::snprintf(z, sizeof(z),
                  "{\n( %d %d %d ) ( %d %d %d ) ( %d %d %d ) test/wand 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) test/wand 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) test/wand 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) test/wand 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) test/wand 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) test/wand 0 0 0 0.5 0.5 0 0 0\n}\n",
                  x0, y0, z0, x0, y1, z0, x0, y0, z1, x1, y0, z0, x1, y0, z1, x1, y1, z0, x0, y0, z0, x0, y0, z1, x1, y0, z0, x0, y1, z0, x1, y1,
                  z0, x0, y1, z1, x0, y0, z0, x1, y0, z0, x0, y1, z0, x0, y0, z1, x0, y1, z1, x1, y0, z1);
    return z;
}
}  // namespace

int main() {
    using namespace bhed;
    using namespace bhed::karte;
    Karte k;
    (void)karteLesen("{\n\"classname\" \"worldspawn\"\n" + quader(-512, -512, -16, 512, 512, 0) + quader(-64, -64, 0, 64, 64, 128) + "}\n", k);
    k.anmelden();
    KollisionsKarte koll;
    (void)kollisionAusKarte(k, [](const std::string&) { return std::vector<std::string>{}; }, true, koll);
    StreuOptionen o;
    o.anzahl = 30;
    o.skalaMin = 0.8F;
    o.skalaMax = 1.2F;
    const StreuErgebnis r = streuOrte(koll, Vektor3f(-400, -400, -16), Vektor3f(400, 400, 300), o, 2000);
    bool auf = r.orte.size() == 30, abstand = true;
    for (std::size_t i = 0; i < r.orte.size(); ++i) {
        const Vektor3f& p = r.orte[i].ort;
        const bool block = std::fabs(p[0]) <= 64 && std::fabs(p[1]) <= 64;
        auf = auf && std::fabs(p[2] - (block ? 128.0F : 0.0F)) < 0.2F && std::fabs(p[0]) <= 400 && std::fabs(p[1]) <= 400 && r.orte[i].skala >= 0.8F &&
              r.orte[i].skala <= 1.2F;
        for (std::size_t j = 0; j < i; ++j) {
            const float dx = p[0] - r.orte[j].ort[0], dy = p[1] - r.orte[j].ort[1], dz = p[2] - r.orte[j].ort[2];
            abstand = abstand && dx * dx + dy * dy + dz * dz >= 48.0F * 48.0F;
        }
    }
    expect("30 Orte auf den Oberseiten (Boden 0, Block 128), im Bereich, Skala 0,8-1,2", auf);
    expect("mindestens 48 auseinander", abstand);
    const StreuErgebnis eng = streuOrte(koll, Vektor3f(-400, -400, -16), Vektor3f(400, 400, 300), o, 5);
    expect("die Grenze laesst nur 5 zu: 5 Orte, begrenzt", eng.orte.size() == 5 && eng.begrenzt);

    const std::size_t n = streuEinfuegen(k, r, "misc_model_static", "models/map_objects/test/fass.md3");
    expect("eingesetzt: 30 misc_model_static mit model, gewaehlt", n == 30 && miscModelStaticZahl(k) == 30 && k.auswahl.anzahl() == 30);
    ZaehlOptionen zo;
    long long gezaehlt = -1;
    for (const Zaehlung& z : zaehleKarte(kartenAbbild(k), zo)) {
        if (z.id == "laufzeit.miscents") { gezaehlt = z.wert; }
    }
    const Grenze* g = grenzeSuchen(GrenzSpiel::JkaSp, "laufzeit.miscents");
    expect("der Optimierer zaehlt 30 gegen MAX_MISC_ENTS 2000 (JA SP)", gezaehlt == 30 && g != nullptr && g->erlaubt == 2000);
    expect("Movie Duels ebenso, Jedi Outcast ohne die Grenze",
           grenzeSuchen(GrenzSpiel::MovieDuels, "laufzeit.miscents") != nullptr && grenzeSuchen(GrenzSpiel::Jk2, "laufzeit.miscents") == nullptr);
    (void)k.undo.undo();
    expect("ein Undo nimmt alle zurueck", miscModelStaticZahl(k) == 0);
    std::printf("%s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
