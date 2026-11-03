// app_wetter.cpp - Regen, Schnee, Staub und Nebelwolken wie im Spiel
//
// Die Pruefung "was zeigt das Spiel, was behaved nicht" (11.10.2026) fand:
// fx_rain, fx_snow, fx_wind und fx_spacedust standen nur als Entity-
// Definitionen da. Im Spiel melden sie Befehle an das Wettersystem des
// Renderers (g_fx.cpp: SP_CreateRain/Snow/Wind, G_FindConfigstringIndex in
// CS_WORLD_FX), und tr_WorldEffects.cpp macht daraus Partikelwolken um die
// Kamera:
//
//   lightrain   500 Tropfen, gfx/world/rain.jpg, 80 lang, 1,2 breit, 2000/s
//   rain       1000 Tropfen, sonst wie lightrain
//   heavyrain  1000 Tropfen, 2800/s
//   acidrain   1000, 2 breit, gruenlich (0.34 0.70 0.34)
//   snow       1000 Flocken, gfx/effects/snowflake1.tga, 300/s
//   spacedust  <count> Staub, gfx/effects/snowpuff1.tga, schwebt
//   fog         60 Wolken, gfx/effects/alpha_smoke2b.tga, 70 gross, 0.2 hell
//   wind / constantwind ( x y z )   treibt alles
//
// "lightsnow" und "heavysnow" kennt der Renderer NICHT (keine Zeile dafuer in
// tr_WorldEffects.cpp) - im Spiel schneit es dann nicht, hier also auch nicht.
//
// Movie Duels: dazu Weather/<karte>.cfg (IT_LoadWeatherParms, execq), jede Zeile "r_weather <n>" ein Satz aus
// RE_WorldEffectCommand. So kommt die Glut auf duel_mustafar in die Luft (r_weather 3, Spiel mit g_AllowWeather 0: keine).
//
// Wo es regnet: ContentsOutside der Engine, ohne Innen- und Aussen-Brushes gilt
// alles als draussen ausser festem Material und Wasser (je 32er-Zelle wie der Punktspeicher).
//
// Die Teilchen: mass 5..10 (CParticleCloud), je Bild v += Kraft / mass, v *= 0,7 ergibt
// konstant Kraft * 0,7 / 0,3 / mass. Mit Schwerkraft kommt ein Teilchen an der Ebene 500 ueber
// der Kamera (+-500) wieder und verschwindet 625 unter ihr; ohne bleibt es im Bereich.
// Farbe mal Alpha (mColor[3]) additiv, Sand mit Alpha.
//
// Die Teilchen sind aus der Zeit GERECHNET, nicht fortgeschrieben: angehalten
// steht der Regen still, beim Scrubben laeuft er mit, und dieselbe Zeit gibt
// dasselbe Bild.
#include "app_internal.h"
#include "gpumap.h"
#include "bhed/messpunkt.h"
#include "bhed/gpuskin.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace bhed::gui {

TextureSet::Tex shrink(const image::Image& im, int maxSize);

namespace {

struct Wolke {
    int anzahl = 0;
    std::string bild;
    float hoehe = 1.0F;      // Kantenlaenge: mHeight * 2, beim Band mHeight
    float breite = 1.0F;     // mWidth * 2, beim Band mWidth
    float fall = 300.0F;     // mGravity
    float farbe[3] = {1.0F, 1.0F, 1.0F};
    float alpha = 1.0F;      // mColor[3], bis dahin blendet ein Teilchen ein
    bool streifen = false;   // mOrientWithVelocity: Band entlang der Fallrichtung
    bool mitAlpha = false;   // mBlendMode 0
    float bereich = 500.0F;  // halbe Kante um die Kamera: Spawn-Ebene, ohne Schwerkraft mSpawnRange
    float unten = 625.0F;
    float oben = 500.0F;
    float masseMin = 5.0F;
    float masseMax = 10.0F;
};

struct Wetter {
    std::string schluessel;          // Karte + Anzahl Entities
    std::vector<Wolke> wolken;
    float wind[3] = {0.0F, 0.0F, 0.0F};
};
// rc608 Teil 13b: je Bild ein Satz - die Vorschau und das Spielbild des Karten-Editors haben je ihre Karte, ihr Wetter
// und ihren Draussen-Speicher. Die oeffentlichen Aufrufe waehlen Satz und Quelle (WetterQuelle).
struct WetterSatz {
    Wetter wetter;
    std::unordered_map<std::int64_t, bool> draussen;
    const void* geoStand = nullptr;
    std::size_t geoFlaechen = 0;
};
WetterSatz g_satzVorschau;
WetterSatz g_satzEditor;
WetterSatz* g_satz = &g_satzVorschau;
const WetterQuelle* g_quelle = nullptr;

// Die Quelle der Vorschau: Karte, Entities und Abspielzeit des Mission-Tabs.
WetterQuelle vorschauQuelle() {
    WetterQuelle q;
    q.entities = &g_app->map.entities;
    q.geo = &g_app->geo;
    q.karte = g_app->map.path;
    q.stand = g_app->entStand;
    q.kollision = geladeneKollision(g_app->map.path);
    q.ms = g_app->playMs;
    return q;
}

int flags(const MapEntity& e) {
    const std::string* f = e.find("spawnflags");
    return f != nullptr ? std::atoi(f->c_str()) : 0;
}

Wolke regen(int anzahl, float fall) {
    Wolke w;
    w.anzahl = anzahl;
    w.bild = "gfx/world/rain";
    w.hoehe = 80.0F;
    w.breite = 1.2F;
    w.fall = fall;
    w.farbe[0] = w.farbe[1] = w.farbe[2] = 0.5F;
    w.alpha = 0.5F;
    w.streifen = true;
    return w;
}

// Die Flocken von "snow" und "lava": 1000, Quadrat +-1, mColor 0,75, sonst die Vorgaben der Wolke.
Wolke flocken(const char* bildName) {
    Wolke w;
    w.anzahl = 1000;
    w.bild = bildName;
    w.hoehe = 2.0F;
    w.breite = 2.0F;
    w.fall = 300.0F;
    w.farbe[0] = w.farbe[1] = w.farbe[2] = 0.75F;
    w.alpha = 0.75F;
    return w;
}

// Ohne Schwerkraft: der Bereich mSpawnRange um die Kamera, die Teilchen laufen darin um.
void bereichSetzen(Wolke& w, float xy, float z) {
    w.bereich = xy;
    w.unten = z;
    w.oben = z;
}

Wolke nebel(float hell) {
    Wolke w;
    w.anzahl = 60;
    w.bild = "gfx/effects/alpha_smoke2b";
    w.hoehe = 70.0F;
    w.breite = 70.0F;
    w.fall = 0.0F;
    w.farbe[0] = w.farbe[1] = w.farbe[2] = hell;
    w.alpha = hell;
    bereichSetzen(w, 625.0F, 150.0F);
    return w;
}

Wolke raumstaub(int anzahl, float kante) {
    Wolke w;
    w.anzahl = anzahl;
    w.bild = "gfx/effects/snowpuff1";
    w.hoehe = kante;
    w.breite = kante;
    w.fall = 0.0F;
    w.farbe[0] = w.farbe[1] = w.farbe[2] = 0.75F;
    w.alpha = 0.75F;
    w.masseMin = 10.0F;
    w.masseMax = 30.0F;
    bereichSetzen(w, 1500.0F, 1500.0F);
    return w;
}

// RE_WorldEffectCommand mit einer Zahl (tr_WorldEffects.cpp:2118-2283 MD). 0 raeumt auch das Wetter der Entities.
void wetterSatz(int nummer) {
    switch (nummer) {
        case 0:
            g_satz->wetter.wolken.clear();
            for (float& k : g_satz->wetter.wind) { k = 0.0F; }
            break;
        case 8:
            g_satz->wetter.wind[0] += 120.0F;   // "wind" wie oben
            break;
        case 2:
            g_satz->wetter.wolken.push_back(regen(1000, 2000.0F));
            break;
        case 7: {
            Wolke w = regen(1000, 2000.0F);
            w.breite = 2.0F;
            w.farbe[0] = 0.34F;
            w.farbe[1] = 0.70F;
            w.farbe[2] = 0.34F;
            w.alpha = 0.70F;
            g_satz->wetter.wolken.push_back(w);
            break;
        }
        case 1:
            g_satz->wetter.wolken.push_back(flocken("gfx/effects/snowflake1"));
            break;
        case 3:
            g_satz->wetter.wolken.push_back(flocken("gfx/effects/snowflake2"));
            break;
        case 9:
            g_satz->wetter.wolken.push_back(raumstaub(1000, 2.4F));
            break;
        case 4: {
            Wolke w;
            w.anzahl = 400;
            w.bild = "gfx/effects/alpha_smoke2b";
            w.hoehe = 140.0F;
            w.breite = 140.0F;
            w.fall = 0.0F;
            w.farbe[0] = 0.9F;
            w.farbe[1] = 0.6F;
            w.farbe[2] = 0.0F;
            w.alpha = 0.5F;
            w.mitAlpha = true;
            w.masseMin = 10.0F;
            w.masseMax = 30.0F;
            bereichSetzen(w, 625.0F, 150.0F);
            g_satz->wetter.wolken.push_back(w);
            break;
        }
        default:
            break;   // 5 und 6: kein Zweig, die Engine gibt nur die Hilfe aus
    }
}

// IT_LoadWeatherParms: execq Weather/<mapname>
void wetterDerKarte() {
    std::string name = g_quelle->karte;
    const std::size_t st = name.find_last_of('/');
    if (st != std::string::npos) { name.erase(0, st + 1); }
    if (name.size() > 4 && name.compare(name.size() - 4, 4, ".bsp") == 0) { name.resize(name.size() - 4); }
    std::string text;
    if (name.empty() || !readFromArchives("Weather/" + name + ".cfg", text)) { return; }
    std::istringstream zeilen(text);
    for (std::string wort; zeilen >> wort;) {
        int nummer = 0;
        if (wort == "r_weather" && zeilen >> nummer) { wetterSatz(nummer); }
    }
    diag::detail("Wetter: Weather/" + name + ".cfg, " + std::to_string(g_satz->wetter.wolken.size()) + " Teilchenwolken");
}

void wetterBauen() {
    // Die Wolken haengen an den Entities (Flaggen, count, Wind): jede
    // Aenderung zaehlt (entStand). Der Draussen-Speicher haengt dagegen an der
    // GEOMETRIE - er bleibt beim Bearbeiten und geht erst mit einer neu
    // gelesenen Karte (Code-Pruefung rc592: vorher wirkte eine geaenderte
    // Flagge gar nicht, und eine neu kompilierte Karte behielt ihn).
    const std::string schl = g_quelle->karte + "|" + std::to_string(g_quelle->entities->size()) + "|" +
                             std::to_string(g_quelle->stand) + (regelwerkJediOutcast() ? "|jk2" : regelwerkMovieDuels() ? "|md" : "");
    if (schl == g_satz->wetter.schluessel) { return; }
    if (g_satz->geoStand != g_quelle->geo->surfaces.data() || g_satz->geoFlaechen != g_quelle->geo->surfaces.size()) {
        g_satz->geoStand = g_quelle->geo->surfaces.data();
        g_satz->geoFlaechen = g_quelle->geo->surfaces.size();
        g_satz->draussen.clear();
    }
    g_satz->wetter = Wetter{};
    g_satz->wetter.schluessel = schl;
    for (const MapEntity& e : *g_quelle->entities) {
        const int f = flags(e);
        if (regelwerkJediOutcast()) {
            // rc604 fix604 (JK2 Stufe 3): Jedi Outcast kennt nur fx_rain und
            // fx_snow (codeJK2/game/g_spawn.cpp:685-686), ohne Flaggen: "rain
            // init <count>" bzw. "snow init <count>" und "wind" (g_fx.cpp:
            // 276-321). Der Renderer liest nur das erste Wort - "rain" sind
            // 1000 Tropfen mit 2000/s, "snow" der Schnee wie unten
            // (code/rd-vanilla/tr_WorldEffects.cpp:1767, 1891, 1964); count
            // bleibt ungelesen. fx_wind und fx_spacedust spawnen nicht.
            if (e.classname == "fx_rain") {
                g_satz->wetter.wolken.push_back(regen(1000, 2000.0F));
                g_satz->wetter.wind[0] += 120.0F;   // "wind" wie oben
            } else if (e.classname == "fx_snow") {
                g_satz->wetter.wolken.push_back(flocken("gfx/effects/snowflake1"));
                g_satz->wetter.wind[0] += 120.0F;
            }
            continue;
        }
        if (e.classname == "fx_rain") {
            if ((f & 1) != 0) {
                g_satz->wetter.wolken.push_back(regen(500, 2000.0F));
            } else if ((f & 2) != 0) {
                g_satz->wetter.wolken.push_back(regen(1000, 2000.0F));
            } else if ((f & 4) != 0) {
                g_satz->wetter.wolken.push_back(regen(1000, 2800.0F));
                g_satz->wetter.wolken.push_back(nebel(0.2F));   // heavyrainfog
            } else if ((f & 8) != 0) {
                Wolke w = regen(1000, 2000.0F);
                w.breite = 2.0F;
                w.farbe[0] = 0.34F;
                w.farbe[1] = 0.70F;
                w.farbe[2] = 0.34F;
                w.alpha = 0.70F;
                g_satz->wetter.wolken.push_back(w);
            } else {
                g_satz->wetter.wolken.push_back(regen(500, 2000.0F));   // lightrain + wind
                g_satz->wetter.wind[0] += 120.0F;
            }
            if ((f & 32) != 0) { g_satz->wetter.wolken.push_back(nebel(0.2F)); }
        } else if (e.classname == "fx_snow") {
            // lightsnow (1) und heavysnow (4) kennt der Renderer nicht.
            if ((f & 1) == 0 && (f & 4) == 0) {
                g_satz->wetter.wolken.push_back(flocken("gfx/effects/snowflake1"));
                if ((f & 2) == 0) { g_satz->wetter.wolken.push_back(nebel(0.2F)); }   // snow + fog
            }
            if ((f & 8) != 0) { g_satz->wetter.wolken.push_back(nebel(0.2F)); }
        } else if (e.classname == "fx_lava" && regelwerkMovieDuels()) {
            // rc604 fix604b (B6): SP_CreateLava (md_g_fx.cpp:318-350 MD, nur Movie
            // Duels): 1 "lightlava", 2 "lava", 4 "heavylava", sonst "lava" + "fog";
            // 8 dazu "fog". Der Renderer kennt nur "lava" - 1000 Teilchen wie der
            // Schnee, Bild gfx/effects/snowflake2.tga (RE_WorldEffectCommand,
            // code/rd-vanilla/tr_WorldEffects.cpp:1909-1924 MD); "lightlava" und
            // "heavylava" uebergeht er (:2105-2112).
            if ((f & 1) == 0 && (f & 4) == 0) {
                g_satz->wetter.wolken.push_back(flocken("gfx/effects/snowflake2"));
                if ((f & 2) == 0) { g_satz->wetter.wolken.push_back(nebel(0.2F)); }   // lava + fog
            }
            if ((f & 8) != 0) { g_satz->wetter.wolken.push_back(nebel(0.2F)); }
        } else if (e.classname == "fx_spacedust") {
            const std::string* c = e.find("count");
            g_satz->wetter.wolken.push_back(raumstaub(std::clamp(c != nullptr ? std::atoi(c->c_str()) : 1000, 0, 8000), 2.4F));
        } else if (e.classname == "fx_wind") {
            if ((f & 1) != 0) { g_satz->wetter.wind[0] += 100.0F; }   // leichter Wind
            if ((f & 2) != 0) {
                // constantwind ( AngleVectors(angles) * speed )
                float ang[3] = {0.0F, 0.0F, 0.0F};
                if (const std::string* a = e.find("angles")) {
                    (void)std::sscanf(a->c_str(), "%f %f %f", &ang[0], &ang[1], &ang[2]);
                } else if (const std::string* a1 = e.find("angle")) {
                    ang[1] = static_cast<float>(std::atof(a1->c_str()));
                }
                const std::string* sp = e.find("speed");
                const float tempo = sp != nullptr ? static_cast<float>(std::atof(sp->c_str())) : 500.0F;
                const float p = ang[0] * 3.14159265F / 180.0F;
                const float y = ang[1] * 3.14159265F / 180.0F;
                g_satz->wetter.wind[0] += std::cos(p) * std::cos(y) * tempo;
                g_satz->wetter.wind[1] += std::cos(p) * std::sin(y) * tempo;
                g_satz->wetter.wind[2] += -std::sin(p) * tempo;
            }
            if ((f & 32) != 0) { g_satz->wetter.wolken.push_back(nebel(0.2F)); }
            if ((f & 64) != 0) { g_satz->wetter.wolken.push_back(nebel(0.1F)); }
        }
    }
    if (!g_satz->wetter.wolken.empty()) {
        diag::detail("Wetter: " + std::to_string(g_satz->wetter.wolken.size()) + " Teilchenwolken aus der Karte");
    }
    if (regelwerkMovieDuels() && !regelwerkJediOutcast()) { wetterDerKarte(); }
}

const TextureSet::Tex* bild(const std::string& pfad) {
    const auto have = g_app->texCache.find(pfad);
    if (have != g_app->texCache.end()) { return have->second.empty() ? nullptr : &have->second; }
    TextureSet::Tex t;
    for (const std::string& versuch : textureCandidates(pfad)) {
        std::string data;
        if (!readFromArchives(versuch, data)) { continue; }
        const image::Image im = image::decode(reinterpret_cast<const unsigned char*>(data.data()), data.size());
        if (!im.ok || im.width <= 0 || im.height <= 0) { continue; }
        t = shrink(im, 128);
        break;
    }
    const auto put = g_app->texCache.emplace(pfad, std::move(t));
    return put.first->second.empty() ? nullptr : &put.first->second;
}

// COutside::ContentsOutside (tr_WorldEffects.cpp:515 MD): Der Punktspeicher der Engine hat 32er-Zellen. Gibt es in
// der Karte Aussen-Brushes, gelten nur die als draussen, sonst alles ohne Innen-Brush.
constexpr std::uint32_t kInhaltDrinnen = 0x10000000U;   // CONTENTS_INSIDE
constexpr std::uint32_t kInhaltDraussen = 0x00010000U;  // CONTENTS_OUTSIDE
bool draussen(const KollisionsKarte* k, bool nurAussenBrushes, const float p[3]) {
    if (k == nullptr) { return true; }
    const auto zelle = [](float v) { return static_cast<std::int64_t>(std::floor(v / 32.0F)); };
    const std::int64_t schl = (zelle(p[0]) & 0xFFFFF) | ((zelle(p[1]) & 0xFFFFF) << 20) | ((zelle(p[2]) & 0xFFFFF) << 40);
    const auto it = g_satz->draussen.find(schl);
    if (it != g_satz->draussen.end()) { return it->second; }
    const float a[3] = {std::floor(p[0] / 32.0F) * 32.0F, std::floor(p[1] / 32.0F) * 32.0F, std::floor(p[2] / 32.0F) * 32.0F};
    const std::uint32_t c = kollisionsInhalt(*k, a);
    bool aus = (c & (kInhaltWasser | kInhaltFest)) == 0U;
    if (aus) { aus = nurAussenBrushes ? (c & kInhaltDraussen) != 0U : (c & kInhaltDrinnen) == 0U; }
    if (g_satz->draussen.size() > 200000U) { g_satz->draussen.clear(); }
    g_satz->draussen.emplace(schl, aus);
    return aus;
}

float zufall(std::uint32_t& s) {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return static_cast<float>(s & 0xFFFFFFU) / 16777216.0F;
}

}  // namespace

// rc604 fix604b (B6): wie viele Wolken dieses Bild tragen (Selbsttest).
bool wetterFuerGras(float wind[3]) {
    // R_IsRaining (tr_WorldEffects.cpp:2360): irgendeine Teilchenwolke. Die
    // Windzonen laufen nur, solange es Wolken gibt (RB_RenderWorldEffects
    // kehrt ohne sie vorher um, :1527) - ohne Wolken bleibt
    // mGlobalWindSpeed 0 (:1506).
    wind[0] = wind[1] = wind[2] = 0.0F;
    if (!g_app->zeigeWetter || g_app->geo.empty()) { return false; }
    const WetterQuelle q = vorschauQuelle();
    g_satz = &g_satzVorschau;
    g_quelle = &q;
    wetterBauen();
    if (g_satz->wetter.wolken.empty()) { return false; }
    for (int k = 0; k < 3; ++k) { wind[k] = g_satz->wetter.wind[k]; }
    return true;
}

int wetterWolkenMitBild(const std::string& bild) {
    const WetterQuelle q = vorschauQuelle();
    g_satz = &g_satzVorschau;
    g_quelle = &q;
    wetterBauen();
    int n = 0;
    for (const Wolke& w : g_satz->wetter.wolken) {
        if (w.bild == bild) { ++n; }
    }
    return n;
}

int wetterTeilchen(std::vector<gpu::KlingenQuad>& aus, const Camera& cam) {
    g_app->wetterTeilchenZahl = 0;   // auch, wenn gleich nichts kommt
    if (!g_app->zeigeWetter || g_app->geo.empty()) { return 0; }
    const WetterQuelle q = vorschauQuelle();
    g_app->wetterTeilchenZahl = wetterTeilchenFuer(q, false, aus, cam);
    return g_app->wetterTeilchenZahl;
}

int wetterTeilchenFuer(const WetterQuelle& quelle, bool editor, std::vector<gpu::KlingenQuad>& aus, const Camera& cam) {
    if (quelle.entities == nullptr || quelle.geo == nullptr || quelle.geo->empty()) { return 0; }
    g_satz = editor ? &g_satzEditor : &g_satzVorschau;
    g_quelle = &quelle;
    wetterBauen();
    if (g_satz->wetter.wolken.empty()) { return 0; }
    float rechts[3];
    float hoch[3];
    cam.right(rechts);
    cam.up(hoch);
    const double t = std::max(0.0, quelle.ms) / 1000.0;
    const std::shared_ptr<const KollisionsKarte>& kk = quelle.kollision;
    bool nurAussen = false;
    if (kk != nullptr) {
        for (const KollisionsKarte::Pinsel& b : kk->pinsel) {
            if ((b.inhalt & kInhaltDraussen) != 0U) { nurAussen = true; break; }
        }
    }
    int n = 0;
    for (std::size_t wi = 0; wi < g_satz->wetter.wolken.size(); ++wi) {
        const Wolke& w = g_satz->wetter.wolken[wi];
        const TextureSet::Tex* tex = bild(w.bild);
        const float B = w.bereich * 2.0F;
        const float BZ = w.unten + w.oben;
        const float kraft[3] = {g_satz->wetter.wind[0], g_satz->wetter.wind[1], g_satz->wetter.wind[2] - w.fall};
        std::uint32_t saat = 0x9E3779B9U ^ static_cast<std::uint32_t>(wi * 7919U + 1U);
        for (int i = 0; i < w.anzahl; ++i) {
            // Ort im Weltraster (die Wolke bleibt in der Welt stehen, wenn die
            // Kamera sich bewegt), um die Kamera herumgefaltet.
            const float s0 = zufall(saat);
            const float s1 = zufall(saat);
            const float s2 = zufall(saat);
            const float tempo = (0.7F / 0.3F) / (w.masseMin + (w.masseMax - w.masseMin) * zufall(saat));
            const float vel[3] = {kraft[0] * tempo, kraft[1] * tempo, kraft[2] * tempo};
            const float vlen = std::sqrt(vel[0] * vel[0] + vel[1] * vel[1] + vel[2] * vel[2]);
            float p[3];
            const float basis[3] = {s0 * B, s1 * B, s2 * BZ};
            for (int k = 0; k < 3; ++k) {
                const float lang = (k == 2) ? BZ : B;
                const float von = (k == 2) ? w.unten : w.bereich;
                const float v = basis[k] + vel[k] * static_cast<float>(t);
                // in [cam - von, cam - von + lang) falten
                float rel = std::fmod(v - (cam.pos[k] - von), lang);
                if (rel < 0.0F) { rel += lang; }
                p[k] = cam.pos[k] - von + rel;
            }
            if (!draussen(kk.get(), nurAussen, p)) { continue; }
            gpu::KlingenQuad q;
            if (w.streifen && vlen > 1.0F) {
                float oben[3];
                for (int k = 0; k < 3; ++k) { oben[k] = p[k] - vel[k] / vlen * w.hoehe; }
                gpu::baueKlingenband(oben, p, cam.pos, rechts, w.breite * 0.5F, q.ecke);
            } else {
                // Zur Kamera gedrehtes Quadrat.
                const float hb = w.breite * 0.5F;
                const float hh = w.hoehe * 0.5F;
                for (int k = 0; k < 3; ++k) {
                    q.ecke[0][k] = p[k] - rechts[k] * hb + hoch[k] * hh;
                    q.ecke[1][k] = p[k] + rechts[k] * hb + hoch[k] * hh;
                    q.ecke[2][k] = p[k] + rechts[k] * hb - hoch[k] * hh;
                    q.ecke[3][k] = p[k] - rechts[k] * hb - hoch[k] * hh;
                }
            }
            for (int k = 0; k < 3; ++k) { q.farbe[k] = w.farbe[k] * 255.0F; }
            q.tex = tex;
            q.spur = true;   // eingefaerbt und ausgeblendet, additiv
            q.staerke = w.alpha;
            if (w.mitAlpha) { q.mischung = 1; }
            aus.push_back(q);
            ++n;
        }
    }
    return n;
}


// --- Lens-Flares (rc592) -------------------------------------------------------------
//
// Bis rc591 uebersprungen ("Flares und Unfug"). Im Spiel (tr_surface.cpp,
// RB_SurfaceFlare; r_flares steht auf 1): ein Schild zur Kamera, 3 Einheiten
// vor der Flaeche entlang ihrer Normalen, Radius 30 (oder portalRange des
// Shaders), naeher als 512 Einheiten entsprechend kleiner, mindestens 5; hell
// nach dem Winkel zur Flaeche; nur, wenn der Punkt nicht verdeckt ist
// (RB_TestZFlare - hier ein Strahl von der Kamera).
int flareSchilde(std::vector<gpu::KlingenQuad>& aus, const Camera& cam, float aspekt) {
    BHED_MESS("Flares");
    g_app->flareZahl = 0;   // auch, wenn gleich nichts kommt
    if (!g_app->zeigeFlares || g_app->geo.empty()) { return 0; }
    float links[3];
    float oben[3];
    float f[3];
    cam.right(links);
    cam.up(oben);
    cam.forward(f);
    // Halbe Blickwinkel als Tangens - fuer den Test, ob das Schild ueberhaupt
    // im Bild liegt (siehe unten).
    const float tanY = std::tan(cam.fovY * 3.14159265F / 360.0F);
    const float tanX = tanY * (aspekt > 0.0F ? aspekt : 1.0F);
    // Neu holen, sobald texCache geleert wurde (texCacheStand) - die Adresse
    // von texCache bleibt dabei gleich, der Zeiger hing sonst ins Leere.
    static const TextureSet::Tex* ersatz = nullptr;
    static int ersatzStand = -1;
    if (ersatzStand != g_app->texCacheStand) {
        ersatz = bild("gfx/misc/flare");
        ersatzStand = g_app->texCacheStand;
    }
    // Erst die Kandidaten (im Bild, vor der Kamera), dann ihre Strahlen auf
    // alle Kerne verteilt (rc594), dann die Schilde in derselben Reihenfolge
    // wie vorher.
    struct Kandidat {
        std::size_t flaeche;
        float o[3];
        float dir[3];
        float radius;
        bool frei;
    };
    std::vector<Kandidat> kandidaten;
    for (std::size_t i = 0; i < g_app->geo.surfaces.size(); ++i) {
        const BspSurface& s = g_app->geo.surfaces[i];
        if (s.type != BspSurface::Type::Flare) { continue; }
        float o[3];
        for (int k = 0; k < 3; ++k) { o[k] = s.flareOrt[k] + s.flareNormale[k] * 3.0F; }
        float dir[3] = {o[0] - cam.pos[0], o[1] - cam.pos[1], o[2] - cam.pos[2]};
        const float dist = std::sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
        if (dist < 1.0F || dist > 16384.0F) { continue; }
        for (float& v : dir) { v /= dist; }
        // vor der Kamera?
        if (dir[0] * f[0] + dir[1] * f[1] + dir[2] * f[2] < 0.05F) { continue; }
        float radius = 30.0F;
        if (dist < 512.0F) { radius = radius * dist / 512.0F; }
        radius = std::max(radius, 5.0F);
        // --- Liegt das Schild im Bild? (rc592, Pruefung) ------------------
        //
        // Das Schild steht zur Kamera gedreht, alle Ecken in derselben Tiefe
        // z; seine Ausdehnung quer ist +-radius. Liegt es ganz ausserhalb des
        // Blickkegels, ist es unsichtbar - dann braucht es auch den Strahl
        // nicht, der quer durch die Karte geht. hoth2 hat 98 Flares; der
        // Strahl fuer jedes kostete rund 8 ms je Bild.
        {
            const float v[3] = {o[0] - cam.pos[0], o[1] - cam.pos[1], o[2] - cam.pos[2]};
            const float z = v[0] * f[0] + v[1] * f[1] + v[2] * f[2];
            const float x = v[0] * links[0] + v[1] * links[1] + v[2] * links[2];
            const float y = v[0] * oben[0] + v[1] * oben[1] + v[2] * oben[2];
            const float rand = radius * 1.05F + 1.0F;
            if (std::fabs(x) - rand > z * tanX || std::fabs(y) - rand > z * tanY) { continue; }
        }
        kandidaten.push_back({i, {o[0], o[1], o[2]}, {dir[0], dir[1], dir[2]}, radius, false});
    }
    // verdeckt?
    spurenVerteilt(kandidaten.size(), [&](std::size_t k) {
        const TraceTreffer t = traceRay(g_app->geo, cam.pos, kandidaten[k].o);
        kandidaten[k].frei = !(t.hit && t.fraction < 0.98F);
    });
    int n = 0;
    for (const Kandidat& kd : kandidaten) {
        if (!kd.frei) { continue; }
        const BspSurface& s = g_app->geo.surfaces[kd.flaeche];
        const float* o = kd.o;
        const float* dir = kd.dir;
        const float radius = kd.radius;
        float d = -(dir[0] * s.flareNormale[0] + dir[1] * s.flareNormale[1] + dir[2] * s.flareNormale[2]);
        if (d < 0.0F) { d = -d; }
        gpu::KlingenQuad q;
        for (int k = 0; k < 3; ++k) {
            q.ecke[0][k] = o[k] - links[k] * radius + oben[k] * radius;
            q.ecke[1][k] = o[k] + links[k] * radius + oben[k] * radius;
            q.ecke[2][k] = o[k] + links[k] * radius - oben[k] * radius;
            q.ecke[3][k] = o[k] - links[k] * radius - oben[k] * radius;
            // Das Bild des Flare-Shaders (rgbGen vertex): grau nach Winkel,
            // dazu die Farbe der Flaeche, wo q3map2 sie gesetzt hat.
            const float fc = (s.flareFarbe[0] + s.flareFarbe[1] + s.flareFarbe[2]) > 0.01F ? s.flareFarbe[k] : 1.0F;
            q.farbe[k] = std::clamp(d * fc * 255.0F, 0.0F, 255.0F);
        }
        const TextureSet::Tex* tex = nullptr;
        if (s.shader >= 0 && static_cast<std::size_t>(s.shader) < g_app->textures.byShader.size() &&
            !g_app->textures.byShader[static_cast<std::size_t>(s.shader)].empty()) {
            tex = &g_app->textures.byShader[static_cast<std::size_t>(s.shader)];
        }
        q.tex = tex != nullptr ? tex : ersatz;
        q.spur = true;
        q.staerke = 1.0F;
        aus.push_back(q);
        ++n;
    }
    g_app->flareZahl = n;
    return n;
}

}  // namespace bhed::gui
