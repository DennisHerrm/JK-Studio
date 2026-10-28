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
#include "bhed/karte/optimierer.h"

#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>
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

// Stufe 9c: Flaechenlicht wie q3map2 -fast (PointToPolygonFormFactor, Unterteilung, Rueckschein)
void flaeche() {
    // 16 x 16 in 256 Abstand, gegenueber: FF ~ A / (pi d^2) = 256 / (pi 65536) = 0.001243
    const std::vector<Vektor3f> q = {Vektor3f(-8, -8, 256), Vektor3f(8, -8, 256), Vektor3f(8, 8, 256), Vektor3f(-8, 8, 256)};
    const float ff = echtFormFaktor(Vektor3f(0, 0, 0), Vektor3f(0, 0, 1), q);
    pruefe(nah(std::fabs(ff), 256.0F / (3.14159265F * 65536.0F), 2e-5F), "PointToPolygonFormFactor klein und weit: A / (pi d^2) (ist " + std::to_string(ff) + ")");
    // halbe Kugel ganz bedeckt: |FF| = 1 (ein riesiges Quadrat dicht darueber)
    const std::vector<Vektor3f> gross = {Vektor3f(-1e5F, -1e5F, 1), Vektor3f(1e5F, -1e5F, 1), Vektor3f(1e5F, 1e5F, 1), Vektor3f(-1e5F, 1e5F, 1)};
    pruefe(nah(std::fabs(echtFormFaktor(Vektor3f(0, 0, 0), Vektor3f(0, 0, 1), gross)), 1.0F, 0.01F), "PointToPolygonFormFactor: ganze Halbkugel = 1");
    // 512 x 512 Decke (Normale nach unten), q3map_surfacelight 100, Unterteilung 128
    EchtFlaechenShader s;
    s.flaechenlicht = 100;
    s.unterteilung = 128;
    std::vector<EchtLicht> l;
    // im Uhrzeigersinn von vorn (unten) wie die Dreiecke der .bsp
    const std::vector<Vektor3f> d1 = {Vektor3f(-256, -256, 256), Vektor3f(256, 256, 256), Vektor3f(-256, 256, 256)};
    const std::vector<Vektor3f> d2 = {Vektor3f(-256, -256, 256), Vektor3f(256, -256, 256), Vektor3f(256, 256, 256)};
    echtFlaechenLichter(d1, Vektor3f(0, 0, -1), s, l);
    echtFlaechenLichter(d2, Vektor3f(0, 0, -1), s, l);
    float haupt = 0;
    float rueck = 0;
    std::size_t nHaupt = 0;
    for (const EchtLicht& x : l) {
        if (x.normale[2] < 0) {
            haupt += x.photonen;
            ++nHaupt;
        } else {
            rueck += x.photonen;
        }
    }
    pruefe(nHaupt > 2 && l.size() == nHaupt * 2, "Unterteilung 128: viele Stuecke, je eines mit Rueckschein (" + std::to_string(nHaupt) + ")");
    pruefe(nah(haupt, 100.0F * 512.0F * 512.0F * 0.25F, 100.0F * 512.0F * 512.0F * 0.25F * 0.01F), "photons = value * area * areaScale (Summe)");
    pruefe(nah(rueck, haupt * 0.35F, haupt * 0.01F), "Rueckschein: 7 * 0.05 der Photonen, 23 hinter der Flaeche");
    pruefe(nah(l[0].add, 100.0F * 3.0F * 0.25F, 1e-3F) && l[0].huelle > 100.0F, "add = value * formFactorValueScale * areaScale, Huelle aus dem Formfaktor");
    // unter der Mitte, Boden (Normale nach oben): alle Hauptlichter zusammen
    Vektor3f summe(0, 0, 0);
    for (const EchtLicht& x : l) { summe = summe + echtLichtBeitrag(x, Vektor3f(0, 0, 0), Vektor3f(0, 0, 1)); }
    float ganz = 0;
    {
        const std::vector<Vektor3f> w = {Vektor3f(-256, -256, 256), Vektor3f(-256, 256, 256), Vektor3f(256, 256, 256), Vektor3f(256, -256, 256)};
        ganz = std::fabs(echtFormFaktor(Vektor3f(0, 0, 0), Vektor3f(0, 0, 1), w)) * 75.0F;
    }
    pruefe(summe[0] > ganz * 0.9F && summe[0] < ganz * 1.1F, "Summe der Stuecke wie der Formfaktor der ganzen Flaeche (" + std::to_string(summe[0]) + " / " + std::to_string(ganz) + ")");
    pruefe(echtLichtBeitrag(l[0], Vektor3f(0, 0, 300), Vektor3f(0, 0, -1))[0] == 0, "hinter dem Flaechenlicht: nichts (nicht zweiseitig)");
}

// q3map_skyLight 100 3: (3-1)*4 = 8 Winkel, 2 Hoehen, dazu die senkrechte: 17 Sonnen, zusammen 100
void himmel() {
    std::vector<EchtLicht> l;
    EchtSonne::Himmelslicht h;
    h.wert = 100;
    h.schritte = 3;
    echtHimmelsLichter(h, Vektor3f(1, 1, 1), l);
    float summe = 0;
    for (const EchtLicht& x : l) { summe += x.photonen; }
    pruefe(l.size() == 17 && nah(summe, 100.0F, 0.01F) && l.back().sonnenRichtung[2] == 1.0F, "q3map_skyLight 100 3: 17 Sonnen, Summe 100 (" + std::to_string(l.size()) + ")");
    // Shader-Skript: "sun" wie SOF2, zwei Sonnen, q3map_skyLight, q3map_lightSubdivide
    const std::string text = R"(textures/test/himmel
{
 q3map_surfacelight 145
 q3map_lightsubdivide 1024
 sun 0.8 1 0.8 170 25 80
 q3map_sunExt 1 1 1 50 90 45 2 16
 q3map_skyLight 60 4
 surfaceparm sky
 skyParms textures/skies/x 512 -
}
textures/test/gitter
{
 surfaceparm alphashadow
 cull none
 {
 map textures/test/gitter.tga
 alphaFunc GE128
 }
}
textures/test/glas
{
 surfaceparm trans
 {
 map $lightmap
 blendFunc GL_DST_COLOR GL_ZERO
 }
}
)";
    const ShaderSkript a = shaderSkriptLesen(text, "textures/test/himmel");
    pruefe(a.sonnen.size() == 2 && a.sonne && a.sonneStaerke == 170 && a.himmelsLichter.size() == 1 && a.himmelsLichter[0].schritte == 4 &&
               a.lichtUnterteilung == 1024 && a.himmel,
           "Shader: sun (SOF2) und q3map_sunExt, q3map_skyLight, q3map_lightSubdivide");
    const ShaderSkript b = shaderSkriptLesen(text, "textures/test/gitter");
    pruefe(b.alphaSchatten && b.durchscheinend && b.zweiseitig && b.stufeAlpha == 3, "Shader: alphashadow, cull none, alphaFunc GE128");
    const ShaderSkript c = shaderSkriptLesen(text, "textures/test/glas");
    pruefe(c.durchscheinend && !c.alphaSchatten && c.mischQuelle == 5 && c.mischZiel == 2, "Shader: trans, blendFunc GL_DST_COLOR GL_ZERO");
}

}  // namespace

int main() {
    flaeche();
    himmel();
    punkt();
    farbeUndLinear();
    spotUndSonne();
    welt();
    std::printf("karteechtlicht: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
