// app_karte_spielbild.cpp - rc608 Teil 11: "Built map as in the game" im Karten-Editor.
//
// Die gebaute Karte (Schnell-Backen, sonst die Vorlage) laeuft durch denselben Zeichner wie die Vorschau im
// Mission-Tab (gpumap): alle Stufen der Shader mit tcGen, rgbGen, tcMod, Blend und Gluehen, die Himmelsbox
// (RB_StageIteratorSky), der Nebel (RB_FogPass), die Sichtbarkeit (R_MarkLeaves), Lichtstile und am Ende die
// Gamma-Tabelle des Spiels. Es gibt keine zweite Formel: die Texturen laedt loadMapTexturesFuer nach den Regeln
// der Vorschau, gezeichnet wird mit gpu::zeichneKarte/zeichneNebel/zeichneGluehen in einer eigenen Szene
// (gpu::szeneWaehlen(1)) und einem eigenen Zielsatz (gpu::kZielKarteneditor), damit Vorschau und Editor ihre
// Bilder auf der Grafikkarte behalten.
//
// Nicht dabei (anders als die Vorschau mit ihrer Mission): Figuren, Mover in Bewegung (Brush-Modelle stehen,
// wo q3map2 sie abgelegt hat), Effekte, misc_model_static, Wetter, Gras (surfaceSprites), misc_skyportal.
#include "karte_echtlicht.h"
#include "karteneditor.h"

#include "app_internal.h"
#include "gpumap.h"

#include "bhed/areaportal.h"
#include "bhed/bsp.h"
#include "bhed/bspgeo.h"
#include "bhed/gpucam.h"
#include "bhed/mover.h"
#include "bhed/regelwerk.h"

#include <chrono>
#include <cmath>
#include <map>
#include <unordered_set>

namespace bhed::gui {
std::vector<gpu::NebelGpu> nebelListeFuer(const BspGeometry& geo);   // app_view3d.cpp
}

namespace bhed::gui::karteneditor {

namespace {

struct Szene {
    std::uint64_t nummer = 0;
    bool gueltig = false;
    BspGeometry geo;
    BspMesh netz;
    TextureSet bilder;
    std::unordered_set<std::string> bildNamen;
    std::map<int, std::vector<char>> pvs;   // je Cluster des Auges
    int pvsCluster = -3;
    float distanceCull = 12000.0F;          // tr.distanceCull (worldspawn "distanceCull", R_LoadWorld)
    float hintergrund[3] = {0.0F, 0.0F, 0.0F};
    bool globalerNebel = false;
};
// nie zerstoert: beim Beenden ist das Grafikgeraet schon weg
Szene& g_szene = *new Szene;
SpielbildStand g_stand;
const AreaPortale g_keinePortale;   // ohne Mission sind alle Portale zu, gezaehlt wird nur das PVS

// Die Szene aus der .bsp, wie loadMapFromBytes sie fuer die Vorschau anlegt.
bool aufbauen(const std::string& daten, std::uint64_t nummer) {
    Szene& s = g_szene;
    s.nummer = nummer;
    s.gueltig = false;
    BspGeometry geo;
    MapData m;
    std::string fehlerText;
    if (!readBspGeometry(daten, geo, &fehlerText) || !readBsp(daten, m, &fehlerText)) {
        diag::info("Karten-Editor: Spielbild - " + fehlerText);
        return false;
    }
    const std::array<bool, 256> dyn = lichtStilDynamisch(m.entities);
    (void)stileEinrechnen(geo, lichtStilGewichte(m.entities), &dyn);
    (void)stileAnwenden(geo, lichtStilGewichteZuZeit(m.entities, {}, 0.0));
    s.geo = std::move(geo);
    // Mit Himmel: die Himmelsbox braucht die Himmelsflaechen (RB_ClipSkyPolygons).
    s.netz = buildMesh(s.geo, g_app->meshDetail, false);
    s.bilder = TextureSet{};
    gpu::szeneWaehlen(1);
    loadMapTexturesFuer(s.geo, s.bilder, s.bildNamen);
    gpu::szeneWaehlen(0);
    s.distanceCull = 12000.0F;
    for (const MapEntity& e : m.entities) {
        if (e.classname != "worldspawn") { continue; }
        for (const auto& [k, v] : e.keys) {
            if (k.size() == 12U && std::equal(k.begin(), k.end(), "distancecull", [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == b; })) {
                float wert = 0.0F;
                if (std::sscanf(v.c_str(), "%f", &wert) == 1) { s.distanceCull = wert; }
            }
        }
        break;
    }
    // Hintergrund: die Farbe des globalen Nebels (RB_BeginDrawingView), wie die Vorschau.
    s.globalerNebel = false;
    for (float& c : s.hintergrund) { c = 0.0F; }
    for (const BspGeometry::Nebel& n : s.geo.nebel) {
        if (!n.global || !n.gueltig) { continue; }
        s.globalerNebel = true;
        std::string k = n.shader;
        for (char& ch : k) { ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); }
        const auto it = g_app->nebelMap.find(k);
        if (it != g_app->nebelMap.end()) {
            for (int c = 0; c < 3; ++c) { s.hintergrund[c] = it->second.farbe[c]; }
        }
        break;
    }
    s.pvs.clear();
    s.pvsCluster = -3;
    s.gueltig = !s.netz.batches.empty();
    diag::detail("Karten-Editor: Spielbild - " + std::to_string(s.netz.batches.size()) + " Stapel, " + std::to_string(s.bilder.found) + " Texturen, " +
                 std::to_string(s.bilder.missing) + " fehlen");
    return s.gueltig;
}

// Die Kamera des Editors als Kamera der Vorschau: Ort, Blick (der Editor nickt mit umgekehrtem Vorzeichen) und
// derselbe Bildwinkel wie kameraProjektion (das Sichtfeld gilt fuer die laengere Seite).
Camera alsSpielKamera(const karte::Kamera& k, int breite, int hoehe, float sichtfeld) {
    Camera c;
    for (int i = 0; i < 3; ++i) { c.pos[i] = k.ursprung[i]; }
    c.angles[0] = -k.winkel[0];
    c.angles[1] = k.winkel[1];
    c.angles[2] = k.winkel[2];
    const double halb = std::tan(sichtfeld * 0.5 * 3.14159265358979323846 / 180.0);
    const double halbH = breite >= hoehe ? halb * static_cast<double>(hoehe) / static_cast<double>(breite) : halb;
    c.fovY = static_cast<float>(2.0 * std::atan(halbH) * 180.0 / 3.14159265358979323846);
    return c;
}

}  // namespace

void* spielbildZeichnen(const std::shared_ptr<const std::string>& daten, std::uint64_t nummer, const karte::Kamera& k, int breite, int hoehe) {
    if (!daten || breite <= 0 || hoehe <= 0 || !gpu::verfuegbar()) { return nullptr; }
    const auto t0 = std::chrono::steady_clock::now();
    if (g_szene.nummer != nummer && !aufbauen(*daten, nummer)) { return nullptr; }
    Szene& s = g_szene;
    if (!s.gueltig) { return nullptr; }
    gpu::szeneWaehlen(1);
    gpu::waehleZiel(gpu::kZielKarteneditor);
    void* bild = nullptr;
    gpu::neuesBild();
    gpu::setzeMultisample(multisampleWirksam(g_app->settings.rExtMultisample, regelwerkMovieDuels() && !regelwerkJediOutcast()));
    gpu::setzeHintergrund(s.hintergrund[0], s.hintergrund[1], s.hintergrund[2]);
    int aufrufe = 0;
    std::string fehlerText;
    if (gpu::bereiteZiel(breite, hoehe)) {
        const Camera cam = alsSpielKamera(k, breite, hoehe, editor().kameraE.sichtfeld);
        const float fern = kartenFernFuer(s.geo);
        float vp[16];
        gpu::baueViewProj(cam, breite, hoehe, 4.0F, fern, vp);   // r_znear 4
        // R_MarkLeaves: das PVS des Clusters, in dem die Kamera steht.
        const int blatt = s.geo.leafAt(cam.pos);
        const int cluster = blatt >= 0 && static_cast<std::size_t>(blatt) < s.geo.leafs.size() ? s.geo.leafs[static_cast<std::size_t>(blatt)].cluster : -1;
        if (cluster != s.pvsCluster) {
            s.pvsCluster = cluster;
            if (s.pvs.size() >= 64U) { s.pvs.clear(); }   // je Eintrag eine Zahl je Flaeche: nicht ohne Grenze wachsen
            auto it = s.pvs.find(cluster);
            if (it == s.pvs.end()) { it = s.pvs.emplace(cluster, flaechenAusserSicht(s.geo, g_keinePortale, -1, cluster)).first; }
            gpu::setzeVerdeckteFlaechen(&s.geo, &it->second);
            g_stand.flaechenVerdeckt = 0;
            for (const char v : it->second) { g_stand.flaechenVerdeckt += v != 0 ? 1U : 0U; }
        }
        gpu::HimmelVorgabe hv;
        for (int i = 0; i < 3; ++i) {
            hv.augeOrt[i] = cam.pos[i];
            hv.pvsOrt[i] = cam.pos[i];
        }
        hv.fern = fern;
        hv.distanceCull = s.distanceCull;
        hv.szeneLoescht = g_app->showGlow && s.globalerNebel;
        gpu::setzeHimmelSicht(hv);
        float vorn[3];
        float rechts[3];
        float hoch[3];
        cam.forward(vorn);
        cam.right(rechts);
        cam.up(hoch);
        const float fokus = 1.0F / std::tan(cam.fovY * 3.14159265F / 180.0F * 0.5F);
        const float zeit = static_cast<float>(ImGui::GetTime());
        std::string f;
        // Erst alles Deckende, dann der Nebel dazu (RB_FogPass nach den Stufen), dann alles Gemischte - wie die
        // Vorschau (app_view3d_bedienung.inc, die beiden Durchgaenge).
        aufrufe += gpu::zeichneKarte(s.netz, &s.bilder, &s.geo, vp, cam.pos, zeit, gpu::kSpielLightmapFaktor, 0.0F, 0, vorn, rechts, hoch, fokus, &f,
                                     nullptr, gpu::Lage::Deckend);
        if (g_app->showFog && !s.geo.nebel.empty()) {
            static const std::vector<gpu::FigurSchatten> kKeineFiguren;
            aufrufe += gpu::zeichneNebel(s.netz, &s.bilder, &s.geo, vp, zeit, cam.pos, nebelListeFuer(s.geo), kKeineFiguren, &f);
        }
        aufrufe += gpu::zeichneKarte(s.netz, &s.bilder, &s.geo, vp, cam.pos, zeit, gpu::kSpielLightmapFaktor, 0.0F, 0, vorn, rechts, hoch, fokus, &f,
                                     nullptr, gpu::Lage::Gemischt);
        if (g_app->showGlow && s.bilder.found > 0 && aufrufe > 0) {
            static const std::vector<gpu::GluehQuelle> kKeineWeiteren;
            aufrufe += gpu::zeichneGluehen(s.netz, &s.bilder, &s.geo, vp, zeit, 320, 240, true, &f, nullptr, kKeineWeiteren);
        }
        // r_gammaShaders (nur Movie Duels, die Quelle von OpenJK hat es nicht) - wie die Vorschau.
        if (regelwerkMovieDuels()) { gpu::spielGammaAnwenden(); }
        bild = gpu::zielTextur();
        fehlerText = f;
    } else {
        fehlerText = "kein Renderziel";
    }
    gpu::waehleZiel(0);
    gpu::szeneWaehlen(0);
    if (g_stand.nummer != nummer || (aufrufe > 0) != (g_stand.aufrufe > 0)) {
        diag::detail("Karten-Editor: Spielbild - " + std::to_string(aufrufe) + " Zeichenaufrufe" + (fehlerText.empty() ? std::string() : " (" + fehlerText + ")"));
    }
    g_stand.aufrufe = aufrufe;
    g_stand.nummer = nummer;
    g_stand.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    return aufrufe > 0 ? bild : nullptr;
}

SpielbildStand spielbildStand() { return g_stand; }

}  // namespace bhed::gui::karteneditor
