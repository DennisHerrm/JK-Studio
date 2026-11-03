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
// rc608 Teil 13b: dazu ueber denselben Code wie die Vorschau die Kartenmodelle (kartenModelleAufstellen: misc_model_static
// und die festen Modelle, Licht aus dem Lichtgitter wie CG_DrawMiscEnts), das Himmelsportal (CG_DrawSkyBoxPortal: erst
// die Szene vom misc_skyportal aus, die Hauptszene zeigt dann statt des Himmels dieses Bild) und das Wetter
// (wetterTeilchenFuer: fx_rain, fx_snow, fx_spacedust, fx_wind, Weather/<karte>.cfg). misc_model braucht nichts:
// q3map2 baut es in die Welt ein und laesst die Entity weg (UnparseEntities).
//
// Nicht dabei (anders als die Vorschau mit ihrer Mission): Figuren, Mover in Bewegung (Brush-Modelle stehen,
// wo q3map2 sie abgelegt hat, und mit ihnen ein model2), Effekte, Gras (surfaceSprites).
#include "karte_echtlicht.h"
#include "karteneditor.h"

#include "app_internal.h"
#include "gpumap.h"

#include "bhed/areaportal.h"
#include "bhed/bsp.h"
#include "bhed/bspgeo.h"
#include "bhed/gpucam.h"
#include "bhed/gpuskin.h"
#include "bhed/kollision.h"
#include "bhed/md3.h"
#include "bhed/mover.h"
#include "bhed/regelwerk.h"

#include <chrono>
#include <cmath>
#include <map>
#include <unordered_set>

namespace bhed::gui {
std::vector<gpu::NebelGpu> nebelListeFuer(const BspGeometry& geo);   // app_view3d.cpp
int nebelFuerIn(const BspGeometry& geo, const float ort[3]);          // app_view3d.cpp
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
    // rc608 Teil 13b
    std::vector<MapEntity> entities;
    std::string kartenName;
    std::map<std::string, Md3Model> md3;
    std::vector<App::MapModel> modelle;
    std::vector<std::string> fehlendeModelle;
    std::map<std::string, BspMesh> modellNetze;   // "Pfad#Bild" wie md3Meshes der Vorschau
    bool portal = false;
    float portalOrt[3] = {0.0F, 0.0F, 0.0F};
    std::uint64_t nummerZaehler = 0;
    std::shared_ptr<const KollisionsKarte> kollision;
};

// Ein Kartenmodell in diesem Bild - wie MoverDraw der Vorschau (offset, Drehung, Groesse, Licht).
struct ModellBild {
    const BspMesh* netz = nullptr;
    float welt[16]{};
    float ort[3]{};
    GridLight licht;
};
// nie zerstoert: beim Beenden ist das Grafikgeraet schon weg
Szene& g_szene = *new Szene;
SpielbildStand g_stand;
double g_zeitFest = -1.0;   // rc608 Teil 12: spielbildZeitFest
const AreaPortale g_keinePortale;   // ohne Mission sind alle Portale zu, gezaehlt wird nur das PVS

// Die Szene aus der .bsp, wie loadMapFromBytes sie fuer die Vorschau anlegt.
bool aufbauen(const std::string& daten, std::uint64_t nummer) {
    Szene& s = g_szene;
    s.nummer = nummer;
    s.gueltig = false;
    // Die Grafikpuffer der alten Modellnetze (Schluessel: Adresse) in der Szene des Editors freigeben.
    gpu::szeneWaehlen(1);
    for (const auto& [name, netz] : s.modellNetze) { gpu::vergissNetz(&netz); }
    gpu::szeneWaehlen(0);
    s.modellNetze.clear();
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
    s.entities = std::move(m.entities);
    // Mit Himmel: die Himmelsbox braucht die Himmelsflaechen (RB_ClipSkyPolygons).
    s.netz = buildMesh(s.geo, g_app->meshDetail, false);
    s.bilder = TextureSet{};
    s.md3.clear();
    s.modelle.clear();
    s.fehlendeModelle.clear();
    int modelleDa = 0;
    int modelleFehlen = 0;
    gpu::szeneWaehlen(1);
    // Die Modelle haengen ihre Bilder hinten an, solange das Ladeziel noch die Szene des Editors ist.
    loadMapTexturesFuer(s.geo, s.bilder, s.bildNamen, [&s, &modelleDa, &modelleFehlen] {
        kartenModelleAufstellen({&s.entities, &s.geo, &s.bilder, &s.md3, &s.modelle, &s.fehlendeModelle}, modelleDa, modelleFehlen);
    });
    gpu::szeneWaehlen(0);
    // CG_DrawSkyBoxPortal: der Ort des misc_skyportal (himmelPortalOrt der Vorschau).
    s.portal = false;
    for (const MapEntity& e : s.entities) {
        if (e.classname != "misc_skyportal" || e.origin.empty()) { continue; }
        s.portal = std::sscanf(e.origin.c_str(), "%f %f %f", &s.portalOrt[0], &s.portalOrt[1], &s.portalOrt[2]) == 3;
        break;
    }
    // Das Wetter braucht ContentsOutside der Karte (nur, wenn eine Wetter-Entity oder Movie Duels eine Wetterdatei hat).
    s.kollision.reset();
    {
        auto k = std::make_shared<KollisionsKarte>();
        if (leseKollision(daten, *k)) { s.kollision = std::move(k); }
    }

    s.distanceCull = 12000.0F;
    for (const MapEntity& e : s.entities) {
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
                 std::to_string(s.bilder.missing) + " fehlen, " + std::to_string(modelleDa) + " Modelle, " + std::to_string(modelleFehlen) +
                 " fehlen" + (s.portal ? ", Himmelsportal" : ""));
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

// Die Kartenmodelle in diesem Bild - wie die Schleife ueber mapModels der Vorschau (app_view3d_bedienung.inc): Bild der
// Animation nach der Zeit (20 je Sekunde), Stellung mit baueMoverWelt, Licht aus dem Lichtgitter am Ort plus lichtHoehe
// (CG_DrawMiscEnts: origin + zoffset + 1). Ein model2 faehrt mit seinem Brush-Modell und bleibt hier weg.
std::vector<ModellBild> modelleImBild(Szene& s, float zeit) {
    std::vector<ModellBild> aus;
    for (const App::MapModel& mm : s.modelle) {
        if (mm.anBrush > 0) { continue; }
        const auto datei = s.md3.find(mm.path);
        if (datei == s.md3.end()) { continue; }
        int bild = 0;
        const int bilder = datei->second.numFrames;
        if (bilder > 1 && !mm.standbild) { bild = static_cast<int>(static_cast<double>(zeit) * 1000.0 * 0.020) % bilder; }
        const std::string key = mm.path + "#" + std::to_string(bild);
        auto netz = s.modellNetze.find(key);
        if (netz == s.modellNetze.end()) { netz = s.modellNetze.emplace(key, md3ToMesh(datei->second, bild, mm.shaderBase)).first; }
        if (netz->second.indexes.empty()) { continue; }
        ModellBild m;
        m.netz = &netz->second;
        const float nullpunkt[3] = {0.0F, 0.0F, 0.0F};
        // Dieselbe Bedingung wie die Vorschau: getaumelt wird nur, wenn Nick oder Roll ungleich null sind.
        gpu::baueMoverWelt(nullpunkt, mm.origin, mm.angles[1], mm.angles[0], mm.angles[2], mm.angles[0] != 0.0F || mm.angles[2] != 0.0F, m.welt,
                           mm.scale);
        for (int k = 0; k < 3; ++k) { m.ort[k] = mm.origin[k]; }
        const float o[3] = {mm.origin[0], mm.origin[1], mm.origin[2] + mm.lichtHoehe};
        m.licht = sampleLightGrid(s.geo, o, false, gpu::kSpielLightmapFaktor);
        aus.push_back(m);
    }
    return aus;
}

// Eine Sicht der Szene in das gewaehlte Ziel: Karte und Modelle deckend, Nebel, Karte und Modelle gemischt, Wetter,
// Gluehen, Gamma. Der Durchgang des Himmelsportals (`portal`) zeichnet wie das Nebenbild der Vorschau kein Wetter,
// kein Gluehen und keine Gamma-Tabelle (nebenbild, imPortalDurchgang).
int sichtZeichnen(Szene& s, const Camera& cam, int breite, int hoehe, float zeit, bool portal, std::string& f) {
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
        if (!portal) {
            g_stand.flaechenVerdeckt = 0;
            for (const char v : it->second) { g_stand.flaechenVerdeckt += v != 0 ? 1U : 0U; }
        }
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
    const std::vector<ModellBild> modelle = modelleImBild(s, zeit);
    if (!portal) { g_stand.modelle = static_cast<int>(modelle.size()); }
    int aufrufe = 0;
    int modellAufrufe = 0;
    // Erst alles Deckende ueber alle Quellen, dann der Nebel dazu (RB_FogPass nach den Stufen), dann alles Gemischte -
    // wie die Vorschau (app_view3d_bedienung.inc, die beiden Durchgaenge).
    for (int durchgang = 0; durchgang < 2; ++durchgang) {
        const gpu::Lage lage = durchgang == 0 ? gpu::Lage::Deckend : gpu::Lage::Gemischt;
        aufrufe += gpu::zeichneKarte(s.netz, &s.bilder, &s.geo, vp, cam.pos, zeit, gpu::kSpielLightmapFaktor, 0.0F, 0, vorn, rechts, hoch, fokus, &f,
                                     nullptr, lage);
        for (const ModellBild& m : modelle) {
            std::string fm;
            modellAufrufe += gpu::zeichneMover(*m.netz, &s.bilder, &s.geo, vp, m.welt, cam.pos, vorn, rechts, hoch, fokus, zeit,
                                               gpu::kSpielLightmapFaktor, 0.0F, &fm, nullptr, lage, &m.licht);
            if (f.empty()) { f = fm; }
        }
        if (durchgang == 0 && g_app->showFog && !s.geo.nebel.empty()) {
            static const std::vector<gpu::FigurSchatten> kKeineFiguren;
            const std::vector<gpu::NebelGpu> nebel = nebelListeFuer(s.geo);
            aufrufe += gpu::zeichneNebel(s.netz, &s.bilder, &s.geo, vp, zeit, cam.pos, nebel, kKeineFiguren, &f);
            // Kartenmodelle mit dem Nebel ihres Ursprungs (wie die Vorschau, nebelFuer).
            for (const ModellBild& m : modelle) {
                std::string fn;
                modellAufrufe += gpu::zeichneNebel(*m.netz, &s.bilder, &s.geo, vp, zeit, cam.pos, nebel, kKeineFiguren, &fn, m.welt, nebelFuerIn(s.geo, m.ort));
            }
        }
    }
    aufrufe += modellAufrufe;
    if (!portal) {
        g_stand.modellAufrufe = modellAufrufe;
        // Das Wetter nach allem anderen: additiv, tiefengetestet (wie die Klingen der Vorschau).
        g_stand.wetterTeilchen = 0;
        if (g_app->zeigeWetter) {
            WetterQuelle q;
            q.entities = &s.entities;
            q.geo = &s.geo;
            q.karte = s.kartenName;
            q.stand = s.nummer;
            q.kollision = s.kollision;
            q.ms = static_cast<double>(zeit) * 1000.0;
            std::vector<gpu::KlingenQuad> teilchen;
            g_stand.wetterTeilchen = wetterTeilchenFuer(q, true, teilchen, cam);
            if (!teilchen.empty()) {
                std::string fk;
                aufrufe += gpu::zeichneKlingen(teilchen, vp, &fk);
            }
        }
        if (g_app->showGlow && s.bilder.found > 0 && aufrufe > 0) {
            std::vector<gpu::GluehQuelle> gluehModelle;
            for (const ModellBild& m : modelle) {
                gpu::GluehQuelle q;
                q.mesh = m.netz;
                std::copy(m.welt, m.welt + 16, q.welt);
                q.modellLicht = m.licht;
                gluehModelle.push_back(q);
            }
            aufrufe += gpu::zeichneGluehen(s.netz, &s.bilder, &s.geo, vp, zeit, 320, 240, true, &f, nullptr, gluehModelle);
        }
        // r_gammaShaders (nur Movie Duels, die Quelle von OpenJK hat es nicht) - wie die Vorschau.
        if (regelwerkMovieDuels()) { gpu::spielGammaAnwenden(); }
    }
    return aufrufe;
}

}  // namespace

void* spielbildZeichnen(const std::shared_ptr<const std::string>& daten, std::uint64_t nummer, const std::string& kartenName, const karte::Kamera& k,
                        int breite, int hoehe) {
    if (!daten || breite <= 0 || hoehe <= 0 || !gpu::verfuegbar()) { return nullptr; }
    const auto t0 = std::chrono::steady_clock::now();
    if (g_szene.nummer != nummer && !aufbauen(*daten, nummer)) { return nullptr; }
    Szene& s = g_szene;
    if (!s.gueltig) { return nullptr; }
    s.kartenName = kartenName;
    gpu::szeneWaehlen(1);
    void* bild = nullptr;
    gpu::neuesBild();
    gpu::setzeMultisample(multisampleWirksam(g_app->settings.rExtMultisample, regelwerkMovieDuels() && !regelwerkJediOutcast()));
    gpu::setzeHintergrund(s.hintergrund[0], s.hintergrund[1], s.hintergrund[2]);
    int aufrufe = 0;
    std::string fehlerText;
    const Camera cam = alsSpielKamera(k, breite, hoehe, editor().kameraE.sichtfeld);
    const float zeit = static_cast<float>(g_zeitFest >= 0.0 ? g_zeitFest : ImGui::GetTime());
    std::string f;
    // CG_DrawSkyBoxPortal: erst die Szene vom Ort des misc_skyportal aus, mit demselben Blick und Bildwinkel, samt
    // Himmelsbox; die Hauptszene zeichnet dann keinen Himmel (RDF_SKYBOXPORTAL) - dort steht das Portalbild. Ohne
    // Himmel (Schalter der Vorschau) nicht.
    g_stand.himmelPortal = false;
    if (s.portal && g_app->showSky) {
        gpu::waehleZiel(gpu::kZielKarteneditorPortal);
        if (gpu::bereiteZiel(breite, hoehe)) {
            Camera pk = cam;
            for (int i = 0; i < 3; ++i) { pk.pos[i] = s.portalOrt[i]; }
            std::string fp;
            g_stand.himmelPortal = sichtZeichnen(s, pk, breite, hoehe, zeit, true, fp) > 0;
        }
    }
    gpu::waehleZiel(gpu::kZielKarteneditor);
    if (gpu::bereiteZiel(breite, hoehe)) {
        if (g_stand.himmelPortal) { gpu::setzeHimmelPortal(gpu::kZielKarteneditorPortal); }
        aufrufe = sichtZeichnen(s, cam, breite, hoehe, zeit, false, f);
        gpu::setzeHimmelPortal(-1);
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
void spielbildZeitFest(double sekunden) { g_zeitFest = sekunden; }

}  // namespace bhed::gui::karteneditor
