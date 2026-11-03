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
// rc608 Teil 13c: die Welt ist Modell 0; die Brush-Modelle kommen nur ueber ihre Entity und in ihrer Stellung beim Laden
// ins Bild (brushModelleImSpiel, mit ihnen ein model2), die Gebietsportale gelten (portalOffenImSpiel), die Halme
// (grasNetzFuer) und die fx_runner (fxRunnerAus, buildEffectMesh; Portal-Entities nur im Himmelsportal) wie die Vorschau.
//
// Nicht dabei (anders als die Vorschau mit ihrer Mission): Figuren, Mover in Bewegung (ohne Mission benutzt sie nichts),
// das dynamische Licht der Effekte, Emittermodelle.
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
#include "bhed/saberkontakt.h"   // kInhaltFest

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
    std::map<std::pair<int, int>, std::vector<char>> pvs;   // je Gebiet und Cluster des Auges
    int pvsGebiet = -3;
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
    // rc608 Teil 13c: die Brush-Modelle ("*N") nach Nummer wie brushMeshes der Vorschau, und ihre Stellungen
    std::map<int, BspMesh> brushNetze;
    MoverSim moverSim;
    // rc608 Teil 13c: surfaceSprites wie die Vorschau (grasNetzFuer): die Halm-Stufen der Shader, je Sicht ein Netz
    // (RB_DrawSurfaceSprites rechnet je Sicht), die Brush-Modelle dieses Bildes
    std::vector<std::vector<App::GrasStufe>> grasStufen;
    int grasStand = 0;
    BspMesh grasNetz;
    BspMesh grasNetzPortal;
    std::vector<MoverDraw> grasBrush;
    // rc608 Teil 13c: fx_runner wie die Vorschau (fxRunnerAus, effectFollowUpsAt, buildEffectMesh) mit eigenen Dateien und
    // Shaderplaetzen (die Vorschau leert ihre bei jedem Kartenwechsel); je Sicht ein Netz
    std::map<std::string, efx::Effect> effekte;
    EffectShaderSlots effektPlaetze;
    std::vector<EffectInstance> effektLaeufer;
    std::vector<EffectInstance> effektLaeuferPortal;   // Portal-Entities (G_PortalifyEntities): nur im Himmelsportal
    BspMesh effektNetz;
    BspMesh effektNetzPortal;
    AreaPortale portale;   // Stand beim Laden: welche Gebietsportale offen sind (portalOffenImSpiel)
    bool portal = false;
    float portalOrt[3] = {0.0F, 0.0F, 0.0F};
    std::uint64_t nummerZaehler = 0;
    std::shared_ptr<const KollisionsKarte> kollision;
};

// Ein Kartenmodell oder Brush-Modell in diesem Bild - wie MoverDraw der Vorschau (offset, Drehung, Groesse, Licht).
// Ein Brush-Modell traegt seine Lightmaps (modellLicht false), ein .md3 das Licht aus dem Lichtgitter.
struct ModellBild {
    const BspMesh* netz = nullptr;
    float welt[16]{};
    float ort[3]{};
    bool modellLicht = true;
    GridLight licht;
};
// nie zerstoert: beim Beenden ist das Grafikgeraet schon weg
Szene& g_szene = *new Szene;
SpielbildStand g_stand;
double g_zeitFest = -1.0;   // rc608 Teil 12: spielbildZeitFest

// Die fx_runner der Karte wie loadMapEffects der Vorschau (fxRunnerAus): ohne Mission laeuft nur, wer nicht auf ein use
// wartet (STARTOFF, ONESHOT: fx_runner_link setzt nextthink -1). Dazu die Folgeeffekte aller Stufen samt Shadern
// (ladeFolgeeffekte), damit kein Bild nachladen muss. Laeuft, solange das Ladeziel die Szene des Editors ist.
void effekteAufstellen(Szene& s) {
    s.effektLaeufer.clear();
    s.effekte.clear();
    s.effektPlaetze.clear();
    const EffektZiel ziel{&s.effekte, &s.bilder, &s.effektPlaetze};
    std::vector<const efx::Effect*> offen;
    for (const MapEntity& e : s.entities) {
        if (e.classname != "fx_runner" || !gespawntImSpiel(e)) { continue; }
        EffectInstance inst;
        if (!fxRunnerAus(e, s.entities, ziel, inst) || inst.startMs < 0.0) { continue; }
        s.effektLaeufer.push_back(inst);
        offen.push_back(inst.effect);
    }
    ladeFolgeeffekte(std::move(offen), nullptr, nullptr, ziel);
}

// G_PortalifyEntities (g_misc.cpp:675-701 OpenJK, Movie Duels ebenso): eine Entity im PVS des misc_skyportal (SV_inPVS:
// Cluster und verbundene Gebiete) mit freier Punktspur CONTENTS_SOLID von dort ist eine Portal-Entity. Ihre Effekte spielt
// CFxScheduler nur in der Szene des Himmelsportals (AddScheduledEffects(true), cg_view.cpp:1927), die anderen nur in der
// Hauptszene (:2205). Die Spur trifft auch Brush-Modelle (gi.trace mit Entities) - in ihrer Stellung beim Laden.
bool portalEntitaet(const Szene& s, const float ort[3]) {
    if (!s.portal || !s.kollision) { return false; }
    const int a = s.geo.leafAt(s.portalOrt);
    const int b = s.geo.leafAt(ort);
    if (a < 0 || b < 0 || static_cast<std::size_t>(a) >= s.geo.leafs.size() || static_cast<std::size_t>(b) >= s.geo.leafs.size()) { return false; }
    const BspGeometry::Leaf& la = s.geo.leafs[static_cast<std::size_t>(a)];
    const BspGeometry::Leaf& lb = s.geo.leafs[static_cast<std::size_t>(b)];
    if (!s.geo.clusterVisible(la.cluster, lb.cluster) || !s.portale.verbunden(la.area, lb.area)) { return false; }
    const float null3[3] = {0.0F, 0.0F, 0.0F};
    if (kollisionsSpur(*s.kollision, s.portalOrt, null3, null3, ort, kInhaltFest).fraction < 1.0F) { return false; }
    for (const BrushStellung& m : brushModelleImSpiel(s.entities, s.geo, s.moverSim, 0.0, {})) {
        if (kollisionsSpur(*s.kollision, s.portalOrt, null3, null3, ort, kInhaltFest, m.modell, m.ort, m.winkel).fraction < 1.0F) { return false; }
    }
    return true;
}

void effektePortalTeilen(Szene& s) {
    s.effektLaeuferPortal.clear();
    std::vector<EffectInstance> haupt;
    for (const EffectInstance& l : s.effektLaeufer) { (portalEntitaet(s, l.origin) ? s.effektLaeuferPortal : haupt).push_back(l); }
    s.effektLaeufer = std::move(haupt);
}

// Das Effektnetz einer Sicht wie baueEffektNetz der Vorschau: Laeufer und Folgeeffekte, Vierecke quer zum Blick dieser
// Kamera (CParticle::Cull mit ihrem Auge), die Vollbildblitze (CFlash) dazu.
void effektNetzBauen(Szene& s, const Camera& cam, int breite, int hoehe, float zeit, bool portal, BspMesh& netz) {
    netz = BspMesh{};
    const std::vector<EffectInstance>& laeufer = portal ? s.effektLaeuferPortal : s.effektLaeufer;
    if (!g_app->showEffects || laeufer.empty()) { return; }
    const double ms = static_cast<double>(zeit) * 1000.0;
    std::vector<EffectInstance> alle = laeufer;
    const std::vector<EffectInstance> folgen = effectFollowUpsAt(alle, ms, &s.geo, [&s](const std::string& name) -> const efx::Effect* {
        const auto it = s.effekte.find("effects/" + name + ".efx");
        return it == s.effekte.end() || it->second.primitives.empty() ? nullptr : &it->second;
    });
    alle.insert(alle.end(), folgen.begin(), folgen.end());
    float vorn[3];
    float rechts[3];
    float hoch[3];
    cam.forward(vorn);
    cam.right(rechts);
    cam.up(hoch);
    EffectCull auge;
    for (int k = 0; k < 3; ++k) { auge.viewer[k] = cam.pos[k]; }
    auge.haveViewer = true;
    auge.sichtTabelle = false;
    netz = buildEffectMesh(alle, ms, rechts, hoch, &s.effektPlaetze, &s.geo, &auge);
    const float seiten = static_cast<float>(breite) / static_cast<float>(std::max(hoehe, 1));
    const float fovX = 2.0F * std::atan(std::tan(cam.fovY * 3.14159265F / 360.0F) * seiten) * 180.0F / 3.14159265F;
    appendScreenFlashes(netz, effectScreenFlashesAt(alle, ms, cam.pos, vorn), cam.pos, vorn, rechts, hoch, fovX, &s.effektPlaetze);
}

// Die Szene aus der .bsp, wie loadMapFromBytes sie fuer die Vorschau anlegt.
bool aufbauen(const std::string& daten, std::uint64_t nummer) {
    Szene& s = g_szene;
    s.nummer = nummer;
    s.gueltig = false;
    // Die Grafikpuffer der alten Modellnetze (Schluessel: Adresse) in der Szene des Editors freigeben.
    gpu::szeneWaehlen(1);
    for (const auto& [name, netz] : s.modellNetze) { gpu::vergissNetz(&netz); }
    for (const auto& [modell, netz] : s.brushNetze) { gpu::vergissNetz(&netz); }
    gpu::szeneWaehlen(0);
    s.modellNetze.clear();
    s.brushNetze.clear();
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
    // rc608 Teil 13c: die Stellungen der Brush-Modelle wie in der Vorschau (InitMover, START_OPEN, START_OFF), ohne
    // Mission: keine Benutzungen, keine Figuren in den Ausloesefeldern - der Stand beim Laden der Karte.
    s.moverSim = MoverSim{};
    s.moverSim.spielstufe = regelwerkSpielstufe();
    s.moverSim.baue(m, s.geo, {}, {}, 0.0);
    // Die Gebietsportale (cm_test.cpp): am Anfang alle zu; offen, wo eine Entity es beim Laden schon ist (eine Tuer
    // steht in ihrer Anfangsstellung, also zu; func_wall/func_usable START_OFF offen). Dieselbe Regel wie die Vorschau.
    {
        int gebiete = 0;
        for (const BspGeometry::Leaf& lf : s.geo.leafs) { gebiete = std::max(gebiete, lf.area + 1); }
        s.portale = AreaPortale{};
        s.portale.anlegen(gebiete);
        for (const PortalEntity& p : portalEntities(m, s.geo, !regelwerkJediOutcast(), [](const MapEntity& e) { return gespawntImSpiel(e); })) {
            if (portalOffenImSpiel(p, s.moverSim, 0.0)) { s.portale.setze(p.gebiete.a, p.gebiete.b, true); }
        }
    }
    s.entities = std::move(m.entities);
    // Die Welt ist Modell 0 (R_AddWorldSurfaces); die anderen Modelle zeichnet das Spiel nur an ihrer Entity
    // (brushModelleImSpiel). Mit Himmel: die Himmelsbox braucht die Himmelsflaechen (RB_ClipSkyPolygons).
    s.netz = s.geo.models.empty() ? buildMesh(s.geo, g_app->meshDetail, false) : buildModelMesh(s.geo, 0, g_app->meshDetail, false);
    s.bilder = TextureSet{};
    s.grasStufen.clear();
    s.md3.clear();
    s.modelle.clear();
    s.fehlendeModelle.clear();
    int modelleDa = 0;
    int modelleFehlen = 0;
    gpu::szeneWaehlen(1);
    // Die Modelle haengen ihre Bilder hinten an, solange das Ladeziel noch die Szene des Editors ist.
    loadMapTexturesFuer(
        s.geo, s.bilder, s.bildNamen,
        [&s, &modelleDa, &modelleFehlen] {
            kartenModelleAufstellen({&s.entities, &s.geo, &s.bilder, &s.md3, &s.modelle, &s.fehlendeModelle}, modelleDa, modelleFehlen);
            effekteAufstellen(s);
        },
        GrasStufenZiel{&s.grasStufen, &s.grasStand});
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
    effektePortalTeilen(s);

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
    s.pvsGebiet = -3;
    s.pvsCluster = -3;
    s.gueltig = !s.netz.batches.empty();
    std::size_t halmStufen = 0;
    for (const auto& v : s.grasStufen) { halmStufen += v.size(); }
    diag::detail("Karten-Editor: Spielbild - " + std::to_string(s.netz.batches.size()) + " Stapel, " + std::to_string(s.bilder.found) + " Texturen, " +
                 std::to_string(s.bilder.missing) + " fehlen, " + std::to_string(modelleDa) + " Modelle, " + std::to_string(modelleFehlen) +
                 " fehlen, " + std::to_string(halmStufen) + " Halm-Stufen" + (s.portal ? ", Himmelsportal" : ""));
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

// Die Brush-Modelle und Kartenmodelle in diesem Bild - wie die Vorschau (app_view3d_bedienung.inc): die Brush-Modelle aus
// brushModelleImSpiel (dieselbe Regel) mit ihren Lightmaps, ein model2 in der Stellung seines Brush-Modells (CG_Mover:
// lerpOrigin/lerpAngles), die Kartenmodelle mit dem Bild der Animation nach der Zeit (20 je Sekunde); Stellung mit
// baueMoverWelt, Licht der Modelle aus dem Lichtgitter am Ort plus lichtHoehe (CG_DrawMiscEnts: origin + zoffset + 1).
std::vector<ModellBild> modelleImBild(Szene& s, float zeit) {
    std::vector<ModellBild> aus;
    std::map<int, BrushStellung> brushStand;
    s.grasBrush.clear();
    for (const BrushStellung& b : brushModelleImSpiel(s.entities, s.geo, s.moverSim, static_cast<double>(zeit) * 1000.0, {})) {
        auto netz = s.brushNetze.find(b.modell);
        if (netz == s.brushNetze.end()) { netz = s.brushNetze.emplace(b.modell, buildModelMesh(s.geo, b.modell, g_app->meshDetail, false)).first; }
        if (netz->second.indexes.empty()) { continue; }
        brushStand[b.modell] = b;
        MoverDraw mv;   // fuer die Halme auf Brush-Modellen (baueGras)
        mv.brush = b.modell;
        for (int k = 0; k < 3; ++k) { mv.offset[k] = b.ort[k]; }
        mv.pitch = b.winkel[0];
        mv.yaw = b.winkel[1];
        mv.roll = b.winkel[2];
        s.grasBrush.push_back(mv);
        ModellBild m;
        m.netz = &netz->second;
        m.modellLicht = false;
        gpu::baueMoverWelt(nullptr, b.ort, b.winkel[1], b.winkel[0], b.winkel[2], b.winkel[0] != 0.0F || b.winkel[2] != 0.0F, m.welt);
        for (int k = 0; k < 3; ++k) { m.ort[k] = b.ort[k]; }
        aus.push_back(m);
    }
    for (const App::MapModel& mm : s.modelle) {
        if (mm.anBrush > 0) {
            const auto st = brushStand.find(mm.anBrush);
            const auto datei = s.md3.find(mm.path);
            if (st == brushStand.end() || datei == s.md3.end()) { continue; }
            auto netz = s.modellNetze.find(mm.path + "#0");
            if (netz == s.modellNetze.end()) { netz = s.modellNetze.emplace(mm.path + "#0", md3ToMesh(datei->second, 0, mm.shaderBase)).first; }
            if (netz->second.indexes.empty()) { continue; }
            const BrushStellung& b = st->second;
            ModellBild m;
            m.netz = &netz->second;
            gpu::baueMoverWelt(nullptr, b.ort, b.winkel[1], b.winkel[0], b.winkel[2], b.winkel[0] != 0.0F || b.winkel[2] != 0.0F, m.welt);
            for (int k = 0; k < 3; ++k) { m.ort[k] = b.ort[k]; }
            m.licht = sampleLightGrid(s.geo, b.ort, false, gpu::kSpielLightmapFaktor);
            aus.push_back(m);
            continue;
        }
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
    // R_MarkLeaves: das PVS des Clusters, in dem die Kamera steht, und die Gebiete, die mit ihrem verbunden sind (areamask).
    const int blatt = s.geo.leafAt(cam.pos);
    const bool imBaum = blatt >= 0 && static_cast<std::size_t>(blatt) < s.geo.leafs.size();
    const int cluster = imBaum ? s.geo.leafs[static_cast<std::size_t>(blatt)].cluster : -1;
    const int gebiet = imBaum ? s.geo.leafs[static_cast<std::size_t>(blatt)].area : -1;
    if (cluster != s.pvsCluster || gebiet != s.pvsGebiet) {
        s.pvsCluster = cluster;
        s.pvsGebiet = gebiet;
        if (s.pvs.size() >= 64U) { s.pvs.clear(); }   // je Eintrag eine Zahl je Flaeche: nicht ohne Grenze wachsen
        auto it = s.pvs.find({gebiet, cluster});
        if (it == s.pvs.end()) { it = s.pvs.emplace(std::make_pair(gebiet, cluster), flaechenAusserSicht(s.geo, s.portale, gebiet, cluster)).first; }
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
    WetterQuelle wq;
    wq.entities = &s.entities;
    wq.geo = &s.geo;
    wq.karte = s.kartenName;
    wq.stand = s.nummer;
    wq.kollision = s.kollision;
    wq.ms = static_cast<double>(zeit) * 1000.0;
    // RB_DrawSurfaceSprites je Sicht mit ihren viewParms (Hauptbild und Himmelsportal); rangescalefactor 1: die Kamera ist
    // die erste Sicht (R_SurfaceSpriteFrameUpdate nimmt deren Bildwinkel als Standard).
    BspMesh& gras = portal ? s.grasNetzPortal : s.grasNetz;
    {
        GrasQuelle q;
        q.welt = &s.netz;
        q.geo = &s.geo;
        q.stufen = &s.grasStufen;
        q.stand = s.grasStand;
        q.brushNetze = &s.brushNetze;
        q.brushStand = &s.grasBrush;
        q.ms = static_cast<double>(zeit) * 1000.0;
        q.regnet = wetterFuerGrasIn(wq, true, q.wind);
        q.editor = true;
        const int vierecke = grasNetzFuer(q, cam, 1.0F, gras);
        if (!portal) { g_stand.grasVierecke = vierecke; }
    }
    BspMesh& effekte = portal ? s.effektNetzPortal : s.effektNetz;
    effektNetzBauen(s, cam, breite, hoehe, zeit, portal, effekte);
    if (!portal) {
        g_stand.effektLaeufer = static_cast<int>(s.effektLaeufer.size());
        g_stand.effektEcken = static_cast<int>(effekte.verts.size());
    } else {
        g_stand.portalEffektEcken = static_cast<int>(effekte.verts.size());
    }
    int aufrufe = 0;
    int modellAufrufe = 0;
    // Die Halme (netzId 3) wie zeichneGras der Vorschau: deckend nach der Karte, gemischt vor ihr; ihr Nebel gleich danach
    // (SQuickSprite::Flush mit tr.fogImage und GLS_DEPTHFUNC_EQUAL).
    const auto halme = [&](gpu::Lage lage, bool erst) {
        if (gras.indexes.empty()) { return; }
        std::string fg;
        aufrufe += gpu::zeichneKarte(gras, &s.bilder, &s.geo, vp, cam.pos, zeit, gpu::kSpielLightmapFaktor, 0.0F, 3, vorn, rechts, hoch, fokus, &fg,
                                     nullptr, lage);
        if (f.empty()) { f = fg; }
        if (g_app->showFog && !s.geo.nebel.empty()) {
            static const std::vector<gpu::FigurSchatten> kKeineFiguren;
            std::string fn;
            aufrufe += gpu::zeichneNebel(gras, &s.bilder, &s.geo, vp, zeit, cam.pos, nebelListeFuer(s.geo), kKeineFiguren, &fn, nullptr, -1,
                                         erst ? 1 : 2);
        }
    };
    // Erst alles Deckende ueber alle Quellen, dann der Nebel dazu (RB_FogPass nach den Stufen), dann alles Gemischte -
    // wie die Vorschau (app_view3d_bedienung.inc, die beiden Durchgaenge).
    for (int durchgang = 0; durchgang < 2; ++durchgang) {
        const gpu::Lage lage = durchgang == 0 ? gpu::Lage::Deckend : gpu::Lage::Gemischt;
        if (durchgang == 1) { halme(lage, false); }
        aufrufe += gpu::zeichneKarte(s.netz, &s.bilder, &s.geo, vp, cam.pos, zeit, gpu::kSpielLightmapFaktor, 0.0F, 0, vorn, rechts, hoch, fokus, &f,
                                     nullptr, lage);
        // Das Effektnetz als zweites Netz nach der Karte (netzId 1), wie die Vorschau.
        if (!effekte.indexes.empty()) {
            std::string fe;
            aufrufe += gpu::zeichneKarte(effekte, &s.bilder, &s.geo, vp, cam.pos, zeit, gpu::kSpielLightmapFaktor, 0.0F, 1, vorn, rechts, hoch, fokus,
                                         &fe, nullptr, lage);
            if (f.empty()) { f = fe; }
        }
        if (durchgang == 0) { halme(lage, true); }
        for (const ModellBild& m : modelle) {
            std::string fm;
            modellAufrufe += gpu::zeichneMover(*m.netz, &s.bilder, &s.geo, vp, m.welt, cam.pos, vorn, rechts, hoch, fokus, zeit,
                                               gpu::kSpielLightmapFaktor, 0.0F, &fm, nullptr, lage, m.modellLicht ? &m.licht : nullptr);
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
            std::vector<gpu::KlingenQuad> teilchen;
            g_stand.wetterTeilchen = wetterTeilchenFuer(wq, true, teilchen, cam);
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
            // Die Effektteilchen mit glow-Stufe gehen mit ins Gluehen, der Rauch nach dem Schalter der Vorschau.
            gpu::gluehenEffekte(g_app->showEffects ? &effekte : nullptr);
            aufrufe += gpu::zeichneGluehen(s.netz, &s.bilder, &s.geo, vp, zeit, 320, 240, true, &f,
                                           (g_app->showEffects && g_app->rauchImGluehen) ? &effekte : nullptr, gluehModelle);
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
    g_stand.portalEffektEcken = 0;
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
