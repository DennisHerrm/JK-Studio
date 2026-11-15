// kartecwimport.cpp - Karten-Editor, CW-IMPORT (src/karte/cwimport.cpp): die Testfigur (Kasten der Engine, Marken), Massstab und
// Achsen beim Import, Massstab je Raum, Treppe zu Clip-Rampe, glatte Clip-Wand, Ebene Collision.
#include "bhed/karte/cwimport.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
int g_pruefungen = 0;
void pruefe(bool ok, const std::string& was) {
    ++g_pruefungen;
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}
bool nah(double a, double b, double e = 1e-4) { return std::fabs(a - b) <= e; }
bool nahV(const Vektor3f& a, double x, double y, double z) { return nah(a.x(), x) && nah(a.y(), y) && nah(a.z(), z); }

bool hatText(const FigurZeichnung& z, const std::string& t) {
    for (const FigurText& x : z.texte) {
        if (x.text == t) { return true; }
    }
    return false;
}
// Die groesste Hoehe der Linien einer Art (relativ zum Ursprung) und die groesste waagerechte Ausdehnung in y bzw. x
void ausdehnung(const FigurZeichnung& z, FigurArt art, const Vektor3f& o, double& hoch, double& y, double& x) {
    hoch = -1e9;
    y = 0;
    x = 0;
    for (const FigurLinie& l : z.linien) {
        if (l.art != art) { continue; }
        for (const Vektor3f& p : {l.a, l.b}) {
            hoch = std::max(hoch, static_cast<double>(p.z() - o.z()));
            y = std::max(y, std::fabs(static_cast<double>(p.y() - o.y())));
            x = std::max(x, std::fabs(static_cast<double>(p.x() - o.x())));
        }
    }
}

void testeTestfigur() {
    const CwMasse m;   // Vorgaben aus CW-PRUEF: Tuer 110 x 64, Gang 128 x 96
    FigurLage l;
    l.ursprung = Vektor3f(100, 200, 24);
    {
        const FigurZeichnung z = testfigurZeichnung(l, spielfigur(GrenzSpiel::MovieDuels), m);
        pruefe(nahV(z.mins, 84, 184, 0) && nahV(z.maxs, 116, 216, 64), "Movie Duels: Kasten -16..16 x -24..40 um den Ursprung (g_client.cpp:94-96, bg_public.h:76-77)");
        pruefe(hatText(z, "64"), "Hoehe 64 beschriftet");
        double hoch = 0, y = 0, x = 0;
        ausdehnung(z, FigurArt::Hoehe, l.ursprung, hoch, y, x);
        pruefe(nah(hoch, 16) && hatText(z, "40"), "stehend: Marke der Oberkante geduckt 16 (CROUCH_MAXS_2), 40 ueber den Fuessen");
        ausdehnung(z, FigurArt::Stufe, l.ursprung, hoch, y, x);
        pruefe(nah(hoch, -24 + 18) && hatText(z, "STEPSIZE 18"), "STEPSIZE 18 ueber den Fuessen (bg_public.h:93)");
        ausdehnung(z, FigurArt::Tuer, l.ursprung, hoch, y, x);
        pruefe(nah(hoch, -24 + 110) && nah(y, 32) && nah(x, 0) && hatText(z, "110 x 64"), "Tuerrahmen 110 x 64 um die Figur, Ebene x = Ursprung");
        ausdehnung(z, FigurArt::Gang, l.ursprung, hoch, y, x);
        pruefe(nah(hoch, -24 + 128) && nah(y, 48) && hatText(z, "128 x 96"), "Gangrahmen 128 x 96");
    }
    {
        FigurLage g = l;
        g.geduckt = true;
        g.quer = true;
        const FigurZeichnung z = testfigurZeichnung(g, spielfigur(GrenzSpiel::JkaSp), m);
        pruefe(nahV(z.maxs, 116, 216, 24 + 16) && hatText(z, "40"), "geduckt: Oberkante 16 (bg_public.h:53), 40 hoch");
        double hoch = 0, y = 0, x = 0;
        ausdehnung(z, FigurArt::Hoehe, g.ursprung, hoch, y, x);
        pruefe(nah(hoch, 40), "geduckt: Marke der Oberkante stehend 40 (Platz zum Aufstehen)");
        ausdehnung(z, FigurArt::Tuer, g.ursprung, hoch, y, x);
        pruefe(nah(x, 32) && nah(y, 0), "quer: Tuerrahmen in der Ebene y = Ursprung");
    }
    {
        const FigurZeichnung z = testfigurZeichnung(l, spielfigur(GrenzSpiel::JkaMp), m);
        pruefe(nah(z.maxs.x() - z.mins.x(), 30), "Mehrspieler: 30 breit (codemp/game/g_client.c:31)");
    }
    {
        FigurLage o = l;
        o.marken = false;
        const FigurZeichnung z = testfigurZeichnung(o, spielfigur(GrenzSpiel::JkaSp), m);
        bool nurKasten = true;
        for (const FigurLinie& x : z.linien) { nurKasten = nurKasten && (x.art == FigurArt::Kasten || x.art == FigurArt::Ursprung); }
        pruefe(nurKasten && z.linien.size() == 12 + 3, "ohne Marken: 12 Kanten und das Ursprungskreuz");
    }
    {
        CwMasse eigen;
        eigen.tuerHoehe = 96;
        eigen.tuerBreite = 48;
        const FigurZeichnung z = testfigurZeichnung(l, spielfigur(GrenzSpiel::JkaSp), eigen);
        pruefe(hatText(z, "96 x 48"), "Tuermass aus den Einstellungen des Optimierers");
    }
    pruefe(nahV(testfigurAufBoden(Vektor3f(1, 2, 3), spielfigur(GrenzSpiel::JkaSp)), 1, 2, 27), "auf dem Boden: Ursprung 24 ueber den Fuessen");
    pruefe(std::string(testfigurSequenz(false)) == "BOTH_STAND1" && std::string(testfigurSequenz(true)) == "BOTH_CROUCH1",
           "Pose wie PM_Footsteps ohne Bewegung: BOTH_STAND1, geduckt BOTH_CROUCH1");
}

}  // namespace

int main() {
    testeTestfigur();
    std::printf("kartecwimport: %d Pruefungen, %d Fehlschlaege\n", g_pruefungen, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
