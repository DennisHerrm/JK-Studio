// karteechtlicht.cpp - Karten-Editor Stufe 7 Punkt 5: Echtzeit-Licht.
//
// Die Lichter einer Karte wie q3map2 -light (CreateEntityLights,
// SetupEnvelopes mit -fast) und ihr Beitrag nach LightContributionToSample
// (light.cpp:777-1177). Sollwerte von Hand aus den Formeln von q3map2:
//   Punktlicht "light" 300: photons = 300 * pointScale (7500) = 2250000,
//   add = photons / dist^2 * cos; linear (spawnflags 1): photons * 1/8000 -
//   dist * fade; Spot (target, radius 64): Kegel (radius + 16) / dist mit
//   weichem Rand 32; Sonne (_sun mit target): add = photons * cos.
#include "bhed/karte/echtlicht.h"
#include "bhed/karte/karte.h"

#include <cmath>
#include <cstdio>
#include <memory>
#include <string>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}
bool nah(float a, float b, float eps) { return std::fabs(a - b) <= eps; }

std::shared_ptr<Entitaet> licht(Karte& k, const char* ursprung, const char* staerke) {
    auto e = k.neueEntitaet("light", false);
    e->setzen("origin", ursprung);
    if (staerke != nullptr) { e->setzen("light", staerke); }
    k.entitaetEinfuegen(e);
    return e;
}

EchtLichtWelt sammeln(const Karte& k) { return echtLichterSammeln(k, nullptr, nullptr); }

void punkt() {
    Karte k;
    k.weltHolenOderAnlegen();
    licht(k, "0 0 128", "300");
    const EchtLichtWelt w = sammeln(k);
    pruefe(w.lichter.size() == 1 && w.lichter[0].art == EchtLicht::Art::Punkt, "ein Punktlicht");
    const EchtLicht& l = w.lichter[0];
    pruefe(nah(l.photonen, 2250000.0F, 1.0F), "photons = 300 * 7500");
    pruefe(nah(l.huelle, 1500.0F, 0.5F), "Huelle mit -fast: sqrt(photons / 1) = 1500");
    const Vektor3f c = echtLichtBeitrag(l, Vektor3f(0, 0, 0), Vektor3f(0, 0, 1));
    pruefe(nah(c[0], 2250000.0F / (128.0F * 128.0F), 0.01F), "128 darunter, senkrecht: photons / 128^2 = 137.3 (ist " + std::to_string(c[0]) + ")");
    // Licht verschieben: naeher -> heller, schraeg -> dunkler, dahinter -> nichts
    Karte k2;
    k2.weltHolenOderAnlegen();
    licht(k2, "0 0 64", "300");
    const Vektor3f naeher = echtLichtBeitrag(sammeln(k2).lichter[0], Vektor3f(0, 0, 0), Vektor3f(0, 0, 1));
    pruefe(naeher[0] > c[0] * 3.9F, "Licht naeher (64 statt 128): etwa viermal so hell");
    const Vektor3f schraeg = echtLichtBeitrag(l, Vektor3f(128, 0, 0), Vektor3f(0, 0, 1));
    pruefe(schraeg[0] < c[0] && schraeg[0] > 0, "schraeg darunter: dunkler");
    const Vektor3f dahinter = echtLichtBeitrag(l, Vektor3f(0, 0, 0), Vektor3f(0, 0, -1));
    pruefe(dahinter[0] == 0, "Flaeche zeigt weg: nichts");
    const Vektor3f nah16 = echtLichtBeitrag(l, Vektor3f(0, 0, 126), Vektor3f(0, 0, 1));
    pruefe(nah(nah16[0], 2250000.0F / 256.0F, 0.5F), "Abstand unter 16 zaehlt als 16");
    const Vektor3f weit = echtLichtBeitrag(l, Vektor3f(0, 0, -2000), Vektor3f(0, 0, 1));
    pruefe(weit[0] == 0, "jenseits der Huelle: nichts");
}

void farbeUndLinear() {
    Karte k;
    k.weltHolenOderAnlegen();
    auto e = licht(k, "0 0 100", "500");
    e->setzen("_color", "1 0.5 0");
    e->setzen("spawnflags", "1");
    e->setzen("fade", "2");
    const EchtLicht l = sammeln(k).lichter[0];
    pruefe(l.linear && !l.winkel, "spawnflags 1: linear, ohne Winkel");
    const Vektor3f c = echtLichtBeitrag(l, Vektor3f(0, 0, 0), Vektor3f(1, 0, 0));
    const float soll = 500.0F * 7500.0F / 8000.0F - 100.0F * 2.0F;
    pruefe(nah(c[0], soll, 0.05F) && nah(c[1], soll * 0.5F, 0.05F) && c[2] == 0, "linear: photons/8000 - dist*fade, Farbe normiert");
}

void spotUndSonne() {
    Karte k;
    k.weltHolenOderAnlegen();
    auto s = licht(k, "0 0 256", "300");
    s->setzen("target", "ziel");
    auto z = k.neueEntitaet("info_null", false);
    z->setzen("origin", "0 0 0");
    z->setzen("targetname", "ziel");
    k.entitaetEinfuegen(z);
    EchtLichtWelt w = sammeln(k);
    pruefe(w.lichter.size() == 1 && w.lichter[0].art == EchtLicht::Art::Spot, "Licht mit target: Spot");
    const EchtLicht& l = w.lichter[0];
    pruefe(nah(l.radiusJeAbstand, 80.0F / 256.0F, 1e-5F), "radiusByDist = (64 + 16) / 256");
    pruefe(echtLichtBeitrag(l, Vektor3f(0, 0, 0), Vektor3f(0, 0, 1))[0] > 0, "im Kegel: hell");
    pruefe(echtLichtBeitrag(l, Vektor3f(100, 0, 0), Vektor3f(0, 0, 1))[0] == 0, "ausserhalb des Kegels (Radius 80): dunkel");
    const float rand = echtLichtBeitrag(l, Vektor3f(70, 0, 0), Vektor3f(0, 0, 1))[0];
    const float mitte = echtLichtBeitrag(l, Vektor3f(0, 0, 0), Vektor3f(0, 0, 1))[0];
    pruefe(rand > 0 && rand < mitte * 0.5F, "weicher Rand (32 Einheiten)");
    s->setzen("_sun", "1");
    w = sammeln(k);
    pruefe(w.lichter.size() == 1 && w.lichter[0].art == EchtLicht::Art::Sonne, "_sun 1: Sonne");
    const Vektor3f sonne = echtLichtBeitrag(w.lichter[0], Vektor3f(5000, 0, 0), Vektor3f(0, 0, 1));
    pruefe(nah(sonne[0], 300.0F, 0.01F), "Sonne: photons * cos, ohne Abstand");
}

void welt() {
    Karte k;
    auto ws = k.weltHolenOderAnlegen();
    ws->setzen("_color", "0.5 0.5 1");
    ws->setzen("_ambient", "10");
    ws->setzen("_minlight", "20");
    const EchtLichtWelt w = sammeln(k);
    const Vektor3f x = echtLuxel(w, Vektor3f(0, 0, 0), Vektor3f(0, 0, 1));
    pruefe(nah(x[0], 10.0F, 1e-4F) && nah(x[2], 20.0F, 1e-4F), "Umgebung _color * _ambient, mindestens _color * _minlight");
}

}  // namespace

int main() {
    punkt();
    farbeUndLinear();
    spotUndSonne();
    welt();
    std::printf("karteechtlicht: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
