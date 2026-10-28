// kartekurven.cpp - Stufe 9 Punkt 7: Kurven-Werkzeug (ctest karte_kurven)
//
// Abtastung (ungerade Zahl gleich weiter Punkte, durch die Kontrollpunkte), Weg (3 Spalten,
// gleiche Vorderseite wie NRCs ebener Patch nach oben), Rohr (9 Spalten, Kreis wie eCylinder),
// Gelaender (zwei Rohre), Bruecke (Deck 7 Spalten, Unterseite 3), path_corner-Kette mit
// targetname/target in einem Befehl, Undo.
#include <cmath>
#include <cstdio>
#include <string>

#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/kurven.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/rueckgaengig.h"

namespace {
int g_fehler = 0;
void expect(const char* was, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was);
    if (!ok) { ++g_fehler; }
}
using namespace bhed::karte;
float vorderZ(const Patch& p) {   // (d/Spalte) x (d/Zeile), z
    const Vektor3f a = p.punkt(0, 0).vertex, b = p.punkt(0, 1).vertex, c = p.punkt(1, 0).vertex;
    const Vektor3f u = b - a, v = c - a;
    return u[0] * v[1] - u[1] * v[0];
}
}  // namespace

int main() {
    std::printf("--- Abtastung ---\n");
    const auto g = kurvePunkte({Vektor3f(0, 0, 0), Vektor3f(512, 0, 0)}, 64.0F);
    bool gleich = g.size() == 9;
    for (std::size_t i = 0; gleich && i < g.size(); ++i) { gleich = std::fabs(g[i][0] - 64.0F * static_cast<float>(i)) < 0.5F && std::fabs(g[i][1]) < 1e-3F; }
    expect("gerade 512, Schritt 64: 9 Punkte, gleich weit, Anfang und Ende genau", gleich);
    const auto b = kurvePunkte({Vektor3f(0, 0, 0), Vektor3f(256, 256, 0), Vektor3f(512, 0, 0)}, 64.0F);
    float nah = 1e9F;
    for (const auto& p : b) { nah = std::min(nah, std::hypot(p[0] - 256.0F, p[1] - 256.0F)); }
    expect("Bogen durch den mittleren Kontrollpunkt, ungerade Zahl Punkte", nah < 24.0F && b.size() % 2 == 1);

    std::printf("--- Patches ---\n");
    KurvenOptionen o;
    o.shader = "textures/test/stein";
    const auto weg = kurvenPatches({Vektor3f(0, 0, 0), Vektor3f(512, 0, 0)}, KurvenArt::Weg, o);
    Patch nrc;
    Kasten k0;
    k0.origin = Vektor3f(0, 0, 0);
    k0.extents = Vektor3f(64, 64, 0);
    nrc.ebeneBauen(k0, 2, 3, 3);
    expect("Weg: ein Patch 3 x 9, 64 links und rechts, Shader", weg.size() == 1 && weg[0]->breite() == 3 && weg[0]->hoehe() == 9 &&
                                                                 std::fabs(std::fabs(weg[0]->punkt(4, 0).vertex[1]) - 64.0F) < 0.01F &&
                                                                 weg[0]->shader() == "textures/test/stein");
    std::printf("      Vorderseite z: Weg %.1f, NRC-Ebene %.1f\n", weg.empty() ? 0.0 : static_cast<double>(vorderZ(*weg[0])), static_cast<double>(vorderZ(nrc)));
    expect("Weg zeigt nach oben wie NRCs ebener Patch (gleiche Vorderseite)", !weg.empty() && (vorderZ(*weg[0]) > 0) == (vorderZ(nrc) > 0));
    const auto rohr = kurvenPatches({Vektor3f(0, 0, 0), Vektor3f(512, 0, 0)}, KurvenArt::Rohr, o);
    bool kreis = rohr.size() == 1 && rohr[0]->breite() == 9;
    for (std::size_t c = 0; kreis && c < 9; c += 2) {
        const Vektor3f q = rohr[0]->punkt(2, c).vertex;
        kreis = std::fabs(std::hypot(q[1], q[2] - o.rohrHoehe) - o.radius) < 0.01F;
    }
    expect("Rohr: 9 Spalten, die geraden auf dem Kreis (Radius 16 um Hoehe 16)", kreis);
    expect("Gelaender: zwei Rohre", kurvenPatches({Vektor3f(0, 0, 0), Vektor3f(512, 0, 0)}, KurvenArt::Gelaender, o).size() == 2);
    const auto br = kurvenPatches({Vektor3f(0, 0, 0), Vektor3f(512, 0, 0)}, KurvenArt::Bruecke, o);
    expect("Bruecke: Deck mit Seiten (7 Spalten) und Unterseite (3)", br.size() == 2 && br[0]->breite() == 7 && br[1]->breite() == 3);

    std::printf("--- Einfuegen ---\n");
    Karte k;
    (void)karteLesen("{\n\"classname\" \"worldspawn\"\n}\n", k);
    k.anmelden();
    const std::size_t vorher = k.entitaeten().size();
    const std::size_t n = kurveEinfuegen(k, {Vektor3f(0, 0, 0), Vektor3f(512, 0, 0)}, KurvenArt::PfadEcken, o, "bahn");
    bool kette = n == 9 && k.entitaeten().size() == vorher + 9 && k.auswahl.anzahl() == 9;
    int gefunden = 0;
    for (const auto& e : k.entitaeten()) {
        if (e->wert("classname") != "path_corner") { continue; }
        ++gefunden;
        if (e->wert("targetname") == "bahn1") { kette = kette && e->wert("target") == "bahn2" && e->wert("origin") == "0 0 0"; }
        if (e->wert("targetname") == "bahn9") { kette = kette && e->wert("target").empty(); }
    }
    expect("path_corner-Kette bahn1 -> ... -> bahn9, gewaehlt", kette && gefunden == 9);
    (void)k.undo.undo();
    expect("ein Undo nimmt die Kette zurueck", k.entitaeten().size() == vorher);
    const std::size_t np = kurveEinfuegen(k, {Vektor3f(0, 0, 0), Vektor3f(512, 0, 0)}, KurvenArt::Bruecke, o, "x");
    expect("Bruecke eingefuegt: zwei Patches im worldspawn", np == 2 && k.welt()->kinder().size() == 2);
    std::printf("%s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
