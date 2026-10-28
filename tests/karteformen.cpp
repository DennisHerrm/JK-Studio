// karteformen.cpp - Stufe 9 Punkt 9: Gelaende formen (ctest karte_formen)
//
// Ein ebener Terrain-Patch 9 x 9 (NRC ebeneBauen, 512 breit): Heben in der Mitte hebt die Mitte
// um die Staerke und den Rand nicht, Senken gleicht es aus, Glaetten nimmt eine Spitze zurueck,
// Ebnen zieht zur Hoehe; ein Strich ist ein Befehl, ein Undo stellt den Patch wieder her.
#include <cmath>
#include <cstdio>
#include <memory>

#include "bhed/karte/gelaende.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/rueckgaengig.h"

namespace {
int g_fehler = 0;
void expect(const char* was, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was);
    if (!ok) { ++g_fehler; }
}
}  // namespace

int main() {
    using namespace bhed::karte;
    Karte k;
    (void)karteLesen("{\n\"classname\" \"worldspawn\"\n}\n", k);
    k.anmelden();
    auto p = std::make_shared<Patch>();
    Kasten b;
    b.origin = Vektor3f(0, 0, 0);
    b.extents = Vektor3f(256, 256, 0);
    p->ebeneBauen(b, 2, 9, 9);
    p->setzeShader("textures/test/gras");
    {
        Befehl c(k.undo, "patch");
        k.welt()->kindEinfuegen(p);
    }
    const auto z = [&](std::size_t r, std::size_t c) { return p->punkt(r, c).vertex[2]; };
    FormPinsel f;
    f.radius = 128.0F;
    f.staerke = 16.0F;
    {
        Befehl c(k.undo, "terrainSculpt");
        (void)formenAufKarte(k, Vektor3f(0, 0, 0), f, 0.0F);
    }
    expect("Heben: Mitte +16, Rand unveraendert", std::fabs(z(4, 4) - 16.0F) < 1e-3F && z(0, 0) == 0.0F && z(4, 0) == 0.0F);
    expect("Nachbar (64 weg) mit (1 - 64/128)^2 = 0,25", std::fabs(z(4, 5) - 4.0F) < 1e-3F);
    (void)k.undo.undo();
    expect("ein Undo stellt den Patch wieder her", z(4, 4) == 0.0F && z(4, 5) == 0.0F);
    {
        Befehl c(k.undo, "terrainSculpt");
        (void)formenAufKarte(k, Vektor3f(0, 0, 0), f, 0.0F);
        (void)formenAufKarte(k, Vektor3f(0, 0, 0), f, 0.0F);
    }
    expect("zwei Anwendungen in einem Strich: +32", std::fabs(z(4, 4) - 32.0F) < 1e-3F);
    const float vorher = z(4, 4);
    f.art = FormArt::Glaetten;
    f.staerke = 8.0F;
    {
        Befehl c(k.undo, "terrainSculpt");
        (void)formenAufKarte(k, Vektor3f(0, 0, 0), f, 0.0F);
    }
    expect("Glaetten nimmt die Spitze zurueck (Richtung Mittel der Nachbarn)", z(4, 4) < vorher && z(4, 4) > 0.0F);
    f.art = FormArt::Ebnen;
    {
        Befehl c(k.undo, "terrainSculpt");
        for (int i = 0; i < 20; ++i) { (void)formenAufKarte(k, Vektor3f(0, 0, 0), f, 0.0F); }
    }
    expect("Ebnen zieht zur Hoehe 0", std::fabs(z(4, 4)) < 0.5F);
    f.art = FormArt::Senken;
    expect("ausserhalb des Radius bewegt sich nichts", formenAufKarte(k, Vektor3f(2000, 2000, 0), f, 0.0F) == 0);
    std::printf("%s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
