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
#include "bhed/karte/modell.h"
#include "bhed/karte/optimierer.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
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
    // SkyProbes: nur _up hell - die senkrechte Sonne bekommt die volle Farbe, die flachen kaum etwas
    {
        std::vector<unsigned char> dunkel(4 * 4 * 4, 0);
        std::vector<unsigned char> hell(4 * 4 * 4, 255);
        const std::vector<unsigned char>* bilder[6] = {&dunkel, &dunkel, &dunkel, &dunkel, &hell, &dunkel};
        const int b[6] = {4, 4, 4, 4, 4, 4};
        const std::vector<EchtSonne::Probe> proben = echtHimmelsProben(bilder, b, b);
        std::vector<EchtLicht> m;
        echtHimmelsLichter(h, Vektor3f(1, 1, 1), m, &proben);
        pruefe(proben.size() == 6U * 64U * 64U && m.size() == 17 && nah(m.back().farbe[0], 1.0F, 1e-4F) && m.front().farbe[0] < 0.8F,
               "SkyProbes: Farbe je Richtung aus den Himmelsbildern, auf das Groesste normiert (flach " + std::to_string(m.front().farbe[0]) + ")");
    }
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
               a.lichtUnterteilung == 1024 && a.himmel && a.himmelsBild == "textures/skies/x",
           "Shader: sun (SOF2) und q3map_sunExt, q3map_skyLight, q3map_lightSubdivide");
    const ShaderSkript b = shaderSkriptLesen(text, "textures/test/gitter");
    pruefe(b.alphaSchatten && b.durchscheinend && b.zweiseitig && b.stufeAlpha == 3 && b.stufeBild == "textures/test/gitter",
           "Shader: alphashadow, cull none, alphaFunc GE128, Bild der ersten Stufe ohne Endung");
    const ShaderSkript c = shaderSkriptLesen(text, "textures/test/glas");
    pruefe(c.durchscheinend && !c.alphaSchatten && c.mischQuelle == 5 && c.mischZiel == 2, "Shader: trans, blendFunc GL_DST_COLOR GL_ZERO");
    // Stufe 9d: q3map_sunExt <r g b> <Staerke> <Grad> <Hoehe> <deviance> <samples>
    pruefe(a.sonnen.size() == 2 && a.sonnen[0].proben == 1 && a.sonnen[1].abweichung == 2 && a.sonnen[1].proben == 16,
           "Shader: q3map_sunExt mit Abweichung 2 und 16 Proben, sun ohne");
}

// --- Stufe 9d ------------------------------------------------------------------------------

// MSVC rand() nach srand( 0 ): 38, 7719, 21238 (RAND_MAX 32767)
void zufall() {
    EchtZufall z;
    const float a = z.naechste();
    const float b = z.naechste();
    const float c = z.naechste();
    pruefe(nah(a, 38.0F / 32767.0F, 1e-7F) && nah(b, 7719.0F / 32767.0F, 1e-7F) && nah(c, 21238.0F / 32767.0F, 1e-7F), "Zufall wie srand( 0 ) / rand() von MSVC");
}

// CreateEntityLights mit _samples/_deviance, CreateSunLight mit numSamples
void proben() {
    Karte k;
    k.weltHolenOderAnlegen();
    auto e = licht(k, "0 0 128", "400");
    e->setzen("_samples", "4");
    e->setzen("_deviance", "8");
    const EchtLichtWelt w = sammeln(k);
    float summe = 0;
    bool nahDran = true;
    for (const EchtLicht& l : w.lichter) {
        summe += l.photonen;
        nahDran = nahDran && std::fabs(l.ursprung[0]) <= 8 && std::fabs(l.ursprung[1]) <= 8 && std::fabs(l.ursprung[2] - 128) <= 8;
    }
    pruefe(w.lichter.size() == 4 && nah(summe, 400.0F * 7500.0F, 1.0F) && nahDran && w.lichter[0].ursprung[2] == 128 && w.lichter[1].ursprung[2] != 128,
           "_samples 4: vier Lichter je photons / 4, die drei Kopien um hoechstens _deviance verschoben (" + std::to_string(w.lichter.size()) + ")");
    // erste Kopie: x = 128 + (38 / 32767 * 2 - 1) * 8 usw. (Zufall von q3map2)
    pruefe(nah(w.lichter[1].ursprung[0], (38.0F / 32767.0F * 2.0F - 1.0F) * 8.0F, 1e-4F), "Jitter der ersten Kopie aus dem Zufall von q3map2");
    // Sonne mit Abweichung: Shader-Sonne ueber die Rueckruf-Funktion
    Karte k2;
    auto welt = k2.weltHolenOderAnlegen();
    std::vector<EchtLicht> aus;
    EchtSonne::Sonne so;
    so.staerke = 100;
    so.grad = 0;
    so.hoehe = 45;
    so.abweichung = 5.0F * 3.14159265F / 180.0F;
    so.proben = 8;
    EchtSonne s;
    s.da = true;
    s.sonnen = {so};
    // eine Himmelsflaeche: ein Pinsel ist hier nicht noetig - die Sonne kommt ueber eine Flaeche; stattdessen
    // die Patch-Flaeche (himmelPruefen gilt auch fuer Patches)
    auto patch = std::make_shared<Patch>();
    patch->setzeGroesse(3, 3);
    for (std::size_t r = 0; r < 3; ++r) {
        for (std::size_t c = 0; c < 3; ++c) { patch->punkt(r, c).vertex = Vektor3f(static_cast<float>(c) * 64.0F, static_cast<float>(r) * 64.0F, 512.0F); }
    }
    patch->setzeShader("textures/test/himmel");
    welt->kindEinfuegen(patch);
    const EchtLichtWelt ws = echtLichterSammeln(k2, nullptr, [&s](const std::string&) { return s; });
    float sumS = 0;
    float maxWinkel = 0;
    const Vektor3f r0 = ws.lichter.empty() ? Vektor3f(0, 0, 0) : ws.lichter[0].sonnenRichtung;
    for (const EchtLicht& l : ws.lichter) {
        sumS += l.photonen;
        const float d = std::clamp(l.sonnenRichtung[0] * r0[0] + l.sonnenRichtung[1] * r0[1] + l.sonnenRichtung[2] * r0[2], -1.0F, 1.0F);
        maxWinkel = std::max(maxWinkel, std::acos(d) * 180.0F / 3.14159265F);
    }
    pruefe(ws.lichter.size() == 8 && nah(sumS, 100.0F, 1e-3F) && nah(r0[2], std::sin(45.0F * 3.14159265F / 180.0F), 1e-5F) && maxWinkel > 0.1F &&
               maxWinkel <= 5.0F * 1.4143F + 0.01F,
           "q3map_sunExt 8 Proben: acht Sonnen, Summe 100, die erste genau, die anderen innerhalb der Abweichung (" + std::to_string(ws.lichter.size()) +
               ", " + std::to_string(maxWinkel) + " Grad)");
}

// Patches wie in -light: Netz von TessellatedMesh, Flaechenlicht wie RadLightForPatch
void patches() {
    // eben (3 x 3, 128 x 128): RemoveLinearMeshColumnsRows laesst 2 x 2 uebrig
    std::vector<Vektor3f> eben;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) { eben.push_back(Vektor3f(static_cast<float>(c) * 64.0F, static_cast<float>(r) * 64.0F, 0.0F)); }
    }
    const EchtNetz n = echtPatchNetz(eben, 3, 3);
    pruefe(n.breite == 2 && n.hoehe == 2 && std::fabs(std::fabs(n.normale[0][2]) - 1.0F) < 1e-5F, "ebener Patch: 2 x 2 nach RemoveLinearMeshColumnsRows, Normale senkrecht");
    // gewoelbt: die Mitte 64 hoeher -> longestCurve > 0, Unterteilung
    std::vector<Vektor3f> bogen = eben;
    for (int r = 0; r < 3; ++r) { bogen[static_cast<std::size_t>(r * 3 + 1)][2] = 64.0F; }
    const EchtNetz b = echtPatchNetz(bogen, 3, 3);
    pruefe(b.breite > 3 && b.hoehe == 2, "gewoelbter Patch: in der Bogenrichtung unterteilt, quer gerade (" + std::to_string(b.breite) + " x " + std::to_string(b.hoehe) + ")");
    // entartet (alle Punkte auf einer Linie): kein Netz (ParsePatch verwirft ihn)
    std::vector<Vektor3f> linie;
    for (int i = 0; i < 9; ++i) { linie.push_back(Vektor3f(static_cast<float>(i) * 8.0F, 0, 0)); }
    pruefe(echtPatchNetz(linie, 3, 3).ort.empty(), "entarteter Patch: verworfen");
    // Flaechenlicht: dieselbe Summe der Photonen wie ein gleich grosser Flaechenausschnitt
    EchtFlaechenShader s;
    s.flaechenlicht = 100;
    s.backsplashAnteil = 0;
    std::vector<EchtLicht> l;
    echtPatchFlaechenLichter(n, s, l);
    float summe = 0;
    for (const EchtLicht& x : l) { summe += x.photonen; }
    pruefe(l.size() == 1 && l[0].wicklung.size() == 4 && nah(summe, 100.0F * 128.0F * 128.0F * 0.25F, 1.0F),
           "ebener Patch: ein Viereck, photons = value * area * areaScale (" + std::to_string(l.size()) + ")");
    // die Normale zeigt zur Seite, auf die das Licht faellt: davor hell, dahinter nichts
    const Vektor3f nl = l.empty() ? Vektor3f(0, 0, 1) : l[0].normale;
    const Vektor3f vorn = Vektor3f(64, 64, 0) + nl * 64.0F;
    const Vektor3f hinten = Vektor3f(64, 64, 0) - nl * 64.0F;
    pruefe(!l.empty() && echtLichtBeitrag(l[0], vorn, Vektor3f(-nl[0], -nl[1], -nl[2]))[0] > 0 && echtLichtBeitrag(l[0], hinten, nl)[0] == 0,
           "Patch-Flaechenlicht: vor der Normale hell, dahinter dunkel");
    std::vector<EchtLicht> lb;
    echtPatchFlaechenLichter(b, s, lb);
    pruefe(lb.size() >= 4, "gewoelbter Patch: mehrere Flaechenlichter (" + std::to_string(lb.size()) + ")");
}

// StripFaceSurface: Viereck -> zwei Dreiecke ab der kleinsten Ecke; entartet -> Faecher um die Mitte
void streifen() {
    const std::vector<Vektor3f> q = {Vektor3f(64, 0, 0), Vektor3f(64, 64, 0), Vektor3f(0, 64, 0), Vektor3f(0, 0, 0)};
    bool mitte = true;
    const std::vector<int> i = echtFlaechenStreifen(q, &mitte);
    // least = 3 (0 0 0); i = 3, j = 2: (3, 1, 2), (0, 1, 3)
    pruefe(!mitte && i == std::vector<int>({3, 1, 2, 0, 1, 3}), "StripFaceSurface: Viereck ab der kleinsten Ecke");
    const std::vector<Vektor3f> fuenf = {Vektor3f(0, 0, 0), Vektor3f(64, 0, 0), Vektor3f(128, 0, 0), Vektor3f(128, 64, 0), Vektor3f(0, 64, 0)};
    const std::vector<int> f = echtFlaechenStreifen(fuenf, &mitte);
    pruefe(f.size() == 9, "Fuenfeck mit einer Ecke auf der Kante: drei Dreiecke (" + std::to_string(f.size()) + ")");
    const std::vector<Vektor3f> duenn = {Vektor3f(0, 0, 0), Vektor3f(100, 0, 0), Vektor3f(200, 0.001F, 0), Vektor3f(300, 0, 0)};
    const std::vector<int> d = echtFlaechenStreifen(duenn, &mitte);
    pruefe(mitte && d.size() == 12 && d[0] == 4, "entartet: FanFaceSurface um den Mittelpunkt");
}

// misc_model: Flaechen und Schatten wie AddTriangleModels / ParseEntityCompileParams
class TestModelle final : public ModellQuelle {
public:
    TestModelle() {
        auto m = std::make_shared<Modell>();
        ModellFlaeche f;
        f.shader = "textures/test/leuchte";
        f.punkte = {{Vektor3f(0, 0, 0), Vektor3f(0, 0, 1), Vektor2f(0, 0)}, {Vektor3f(64, 0, 0), Vektor3f(0, 0, 1), Vektor2f(1, 0)},
                    {Vektor3f(0, 64, 0), Vektor3f(0, 0, 1), Vektor2f(0, 1)}};
        f.indizes = {0, 1, 2};
        m->flaechen.push_back(f);
        m_modell = m;
    }
    [[nodiscard]] ModellAnsicht ansicht(const Entitaet&) const override {
        ModellAnsicht a;
        a.modell = m_modell;
        a.shader = {"textures/test/leuchte"};
        return a;
    }

private:
    std::shared_ptr<const Modell> m_modell;
};

void modelle() {
    TestModelle quelle;
    Karte k;
    k.kontext.modelle = &quelle;
    k.weltHolenOderAnlegen();
    const auto mm = [&k](const char* ursprung) {
        auto e = k.neueEntitaet("misc_model", false);
        e->setzen("origin", ursprung);
        e->setzen("model", "models/test.md3");
        k.entitaetEinfuegen(e);
        return e;
    };
    auto a = mm("0 0 0");
    auto b = mm("1000 0 0");
    b->setzen("_castShadows", "0");
    auto c = mm("2000 0 0");
    c->setzen("target", "gibtsnicht");   // kein Ziel: nicht in der .bsp
    std::vector<int> wirft;
    std::vector<float> x;
    echtModellFlaechen(k, [&](const EchtModellFlaeche& f) {
        wirft.push_back(f.wirft);
        x.push_back(f.ort.empty() ? -1.0F : f.ort[0][0]);
    });
    pruefe(wirft == std::vector<int>({1, 0}) && x.size() == 2 && x[1] == 1000.0F,
           "misc_model: in der Welt castShadows 1, _castShadows 0 wirft nicht, target ohne Ziel fehlt");
    EchtFlaechenShader s;
    s.flaechenlicht = 1000;
    s.backsplashAnteil = 0;
    const EchtLichtWelt w = echtLichterSammeln(k, [&s](const std::string& sh) { return sh == "textures/test/leuchte" ? s : EchtFlaechenShader{}; }, nullptr);
    float summe = 0;
    std::size_t flaechen = 0;
    for (const EchtLicht& l : w.lichter) {
        if (l.art != EchtLicht::Art::Flaeche) { continue; }
        ++flaechen;
        summe += l.photonen;
    }
    pruefe(flaechen == 2 && nah(summe, 2.0F * 1000.0F * 2048.0F * 0.25F, 1.0F) && w.lichter[0].normale[2] > 0.99F,
           "misc_model mit q3map_surfacelight: je Dreieck ein Flaechenlicht (RadLightForTriangles), Normale aus den Ecken (" + std::to_string(flaechen) + ")");
    const Vektor3f vorn = echtLichtBeitrag(w.lichter[0], Vektor3f(16, 16, 64), Vektor3f(0, 0, -1));
    pruefe(vorn[0] > 0, "misc_model-Flaechenlicht: ueber dem Dreieck hell");
}

}  // namespace

// Nebel: R_LoadFogs, EmitFogs (sichtbare Seite), RB_CalcFogTexCoords + R_FogFactor. Sollwert von Hand:
// Nebel z 0..128 (Flaeche oben, ebene 0 0 -1 128), Tiefe 256 -> tcScale 1/2048, Auge 0 0 300, Blick nach
// unten, Stelle 0 0 0: eye_t = -172 (draussen), t = 128 -> 1/32 + 30/32 * 128 / 300 = 0.43125;
// s = 300 / 2048 = 0.146484 -> * (0.43125 - 1/32) / (30/32) = 0.0625 -> * 8 = 0.5 -> sqrt = 0.70711
void nebel() {
    Karte k;
    auto welt = k.weltHolenOderAnlegen();
    TexturProjektion proj;
    texdefStandard(TexdefArt::Quake, proj);
    const auto quader = [&](const Vektor3f& mitte, const Vektor3f& halb, const char* shader) {
        auto p = std::make_shared<Pinsel>();
        p->quaderBauen(Kasten(mitte, halb), shader, proj, k.kontext);
        p->bRep(k.kontext);
        welt->kindEinfuegen(p);
        return p;
    };
    quader(Vektor3f(0, 0, -8), Vektor3f(512, 512, 8), "textures/test/stein");   // Boden: deckt die Unterseite des Nebels
    quader(Vektor3f(0, 0, 64), Vektor3f(256, 256, 64), "textures/test/nebel");
    const auto shader = [](const std::string& sh) {
        EchtNebelShader r;
        if (sh == "textures/test/nebel") {
            r.nebel = true;
            r.farbe = Vektor3f(0.1F, 0.9F, 0.1F);
            r.tiefe = 256;
        }
        return r;
    };
    std::vector<EchtNebel> n = echtNebelSammeln(k, shader);
    pruefe(n.size() == 1 && n[0].hatFlaeche && n[0].ebene[2] == -1.0F && n[0].ebene[3] == 128.0F && nah(n[0].tcScale, 1.0F / 2048.0F, 1e-9F),
           "Nebelpinsel: sichtbare Seite +Z (EmitFogs: achsparallel von +Z rueckwaerts), tcScale 1 / (256 * 8)");
    const float d = n.empty() ? 0.0F : echtNebelDichte(n[0], Vektor3f(0, 0, 0), Vektor3f(0, 0, 300), Vektor3f(0, 0, -1));
    pruefe(nah(d, 0.70711F, 1e-4F), "RB_CalcFogTexCoords + R_FogFactor: Boden aus 300 von oben 0.70711 (ist " + std::to_string(d) + ")");
    const float innen = n.empty() ? 0.0F : echtNebelDichte(n[0], Vektor3f(0, 0, 0), Vektor3f(0, 0, 100), Vektor3f(0, 0, -1));
    pruefe(nah(innen, std::sqrt(100.0F / 2048.0F * 8.0F), 1e-4F), "Auge im Nebel: t = 31/32, nur der Abstand (ist " + std::to_string(innen) + ")");
    pruefe(n.empty() || echtNebelDichte(n[0], Vektor3f(0, 0, 200), Vektor3f(0, 0, 300), Vektor3f(0, 0, -1)) == 0.0F, "ueber der Nebelflaeche: kein Nebel");
    // q3map_fogDir erzwingt die Seite, deren Normale am meisten entgegen zeigt
    const auto mitRichtung = [&shader](const std::string& sh) {
        EchtNebelShader r = shader(sh);
        if (r.nebel) { r.richtung = Vektor3f(1, 0, 0); }
        return r;
    };
    n = echtNebelSammeln(k, mitRichtung);
    pruefe(n.size() == 1 && n[0].hatFlaeche && n[0].ebene[0] == 1.0F, "q3map_fogDir 1 0 0: die Seite -X ist die Nebelflaeche");
    // globaler Nebel (worldspawn _fog), ohne Shader: "bad shader" rot, Tiefe 250
    welt->setzen("_fog", "textures/test/fehlt");
    n = echtNebelSammeln(k, shader);
    pruefe(n.size() == 2 && n[1].global && n[1].farbe[0] == 1.0F && n[1].farbe[1] == 0.0F && nah(n[1].tcScale, 1.0F / 2000.0F, 1e-9F),
           "globaler Nebel aus worldspawn _fog; fehlt der Shader: rot, Tiefe 250 (R_LoadFogs)");
    // Shader-Skript: fogparms mit Klammern als eigene Woerter und angeklebt, q3map_fogDir
    const ShaderSkript a = shaderSkriptLesen("textures/n/a\n{\n fogparms ( 0.5 0.25 1 ) 300\n q3map_fogDir 0 0 -1\n}\n", "textures/n/a");
    const ShaderSkript b = shaderSkriptLesen("textures/n/b\n{\n fogparms (0.5 0.25 1) 64\n}\n", "textures/n/b");
    pruefe(a.nebelParms && a.nebelFarbe[1] == 0.25 && a.nebelTiefe == 300 && a.nebelRichtung[2] == -1 && b.nebelParms && b.nebelFarbe[0] == 0.5 &&
               b.nebelTiefe == 64,
           "Shader: fogParms ( r g b ) Tiefe, q3map_fogDir");
}

int main() {
    nebel();
    zufall();
    proben();
    patches();
    streifen();
    modelle();
    flaeche();
    himmel();
    punkt();
    farbeUndLinear();
    spotUndSonne();
    welt();
    std::printf("karteechtlicht: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
