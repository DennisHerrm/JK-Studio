// aufnahmetest.cpp - Zeitleiste 2 Punkt 1: aus einer gesteuerten Figur werden
// Laufziele - nur wo noetig (bhed/aufnahme.h).
#include "bhed/aufnahme.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <vector>

using namespace bhed;

namespace {

int fehler = 0;

void pruefe(bool ok, const char* was) {
    std::printf("  %s  %s\n", ok ? "ok   " : "FEHLER", was);
    if (!ok) { ++fehler; }
}

std::vector<AufnahmeProbe> strecke(float x0, float y0, float x1, float y1, int n, bool gehen = false) {
    std::vector<AufnahmeProbe> p;
    for (int i = 0; i <= n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(n);
        AufnahmeProbe a;
        a.ort = {x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, 24.0F};
        a.gehen = gehen;
        p.push_back(a);
    }
    return p;
}

void anhaengen(std::vector<AufnahmeProbe>& a, const std::vector<AufnahmeProbe>& b) { a.insert(a.end(), b.begin() + 1, b.end()); }

}  // namespace

int main() {
    std::printf("Punkte nur wo noetig\n");
    const NavSpurFn ohne;
    std::vector<AufnahmeProbe> gerade = strecke(0, 0, 400, 0, 40);
    gerade[20].ort[1] = 10.0F;   // ein kleiner Schlenker unter der Toleranz
    std::vector<std::size_t> k = aufnahmeVereinfachen(gerade, 24.0F, ohne);
    pruefe(k.size() == 1 && k[0] == 40, "gerade mit kleinem Schlenker: nur das Ende");
    std::vector<AufnahmeProbe> l = strecke(0, 0, 300, 0, 30);
    anhaengen(l, strecke(300, 0, 300, 300, 30));
    k = aufnahmeVereinfachen(l, 24.0F, ohne);
    pruefe(k.size() == 2 && k[0] == 30 && k[1] == 60, "um die Ecke: die Ecke und das Ende");
    k = aufnahmeVereinfachen(l, 500.0F, ohne);
    pruefe(k.size() == 1 && k[0] == 60, "grosse Toleranz: nur das Ende");
    std::vector<AufnahmeProbe> gang = strecke(0, 0, 200, 0, 20);
    anhaengen(gang, strecke(200, 0, 400, 0, 20, true));
    k = aufnahmeVereinfachen(gang, 24.0F, ohne);
    pruefe(k.size() == 2 && k[0] == 20 && !gang[k[0]].gehen && gang[k[1]].gehen, "Gangart wechselt: der Punkt davor bleibt");
    pruefe(aufnahmeVereinfachen(strecke(0, 0, 10, 0, 5), 24.0F, ohne).empty(), "kaum bewegt: keine Laufziele");
    // Eine Wand bei x 200 um y 0: der Weg ging aussen herum (y 60), die gerade
    // Strecke waere blockiert - der Punkt aussen bleibt, auch in der Toleranz.
    const NavSpurFn wand = [](const NavVec& a, const NavVec&, const NavVec&, const NavVec& b, int, std::uint32_t) {
        NavSpur t;
        const bool kreuzt = (a[0] < 200.0F) != (b[0] < 200.0F);
        if (kreuzt && std::fabs((a[1] + b[1]) * 0.5F) < 20.0F) { t.fraction = 0.5F; }
        return t;
    };
    std::vector<AufnahmeProbe> um = strecke(0, 0, 200, 60, 20);
    anhaengen(um, strecke(200, 60, 400, 0, 20));
    k = aufnahmeVereinfachen(um, 100.0F, wand);
    std::printf("  um die Wand: %zu Punkte\n", k.size());
    pruefe(k.size() == 2 && k[0] == 20 && k[1] == 40, "eine Wand auf der Abkuerzung: ein Punkt bleibt, obwohl in der Toleranz");
    k = aufnahmeVereinfachen(um, 100.0F, ohne);
    pruefe(k.size() == 1, "ohne Wand reicht das Ende");
    std::printf("%s (%d Fehler)\n", fehler == 0 ? "alles gut" : "FEHLER", fehler);
    return fehler == 0 ? 0 : 1;
}
