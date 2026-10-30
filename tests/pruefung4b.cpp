// pruefung4b.cpp - Pruefung 4b: Entities, Effekte, Kamera - je Punkt gegen die
// Formel der Engine (OpenJK, Movie Duels, Jedi Outcast) mit eigenen Daten.
//
// 1. useBBox im Effektpfad: efx::sim gegen eine Abschrift von
//    CParticle::UpdateOrigin mit der Kastenspur der Kollisionskarte.
// 6. CAMERA_ACCEL und ZOOM gleichzeitig: client_camera (FOV, FOV2, FOV_time,
//    FOV_duration, FOV_vel, FOV_acc, CAMERA_ACCEL/CAMERA_ZOOMING) Bild fuer
//    Bild wie CGCam_Update (cg_camera.cpp:1101-1183 OpenJK) gegen
//    CameraTrack::at.
// 3a. weapon_saber mit NPC_type: das Modell aus der .sab (FinishSpawningItem).
// 3. forced_rgb_saber_colours (Movie Duels): die RGB-Zweige sind hinter
//    "saberColor"/"saber2Color" unerreichbar - eine RGB-Zeile aendert nichts;
//    Jedi Outcast kennt nur "saberColor".
// 5. Zweites use in der Bruchverzoegerung: funcBBrushUse -> funcBBrushDie
//    stellt nextthink neu (und der use-Skript laeuft erneut); ein use vor
//    G_FreeEntity bricht neu (g_breakable.cpp:81-215 OpenJK). 5a: Remove vor
//    dem ersten Add gegen eine Abschrift des ghoul2-Felds.
#include "bhed/ablauf.h"
#include "bhed/bspgeo.h"
#include "bhed/camtrack.h"
#include "bhed/efx/sim.h"
#include "bhed/gegenstaende.h"
#include "bhed/kollision.h"
#include "bhed/mover.h"
#include "bhed/roff.h"
#include "bhed/scene.h"
#include "bhed/script.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace bhed;

namespace {

int g_fehler = 0;

void pruefe(bool ok, const std::string& was) {
    std::printf("%s %s\n", ok ? "OK    " : "FEHLER", was.c_str());
    if (!ok) { ++g_fehler; }
}

bool nah(float a, float b, float d = 0.01F) { return std::fabs(a - b) <= d; }

std::string zahl(float w) {
    char b[32];
    std::snprintf(b, sizeof b, "%.3f", static_cast<double>(w));
    return b;
}

// --- 6. CAMERA_ACCEL und ZOOM ---------------------------------------------------

// client_camera, nur der Bildwinkel - eigene Abschrift von cg_camera.cpp
// (CGCam_SetFOV :353, CGCam_Zoom :364, CGCam_Zoom2 :379, CGCam_ZoomAccel :395,
// CGCam_Enable :91-92, CGCam_Update :1101-1183 OpenJK).
struct EngineKamera {
    float FOV = 90.0F;
    float FOV2 = 90.0F;
    int FOV_time = 0;
    float FOV_duration = 0.0F;
    float FOV_vel = 0.0F;
    float FOV_acc = 0.0F;
    bool accel = false;
    bool zooming = false;
    void setFov(float f) { FOV = f; }
    void zoom(float f, float duration, int t) {
        if (duration == 0.0F) {
            setFov(f);
            return;
        }
        zooming = true;
        FOV_time = t;
        FOV2 = f;
        FOV_duration = duration;
    }
    void zoom2(float f, float f2, float duration, int t) {
        if (duration == 0.0F) {
            setFov(f2);
            return;
        }
        zooming = true;
        FOV_time = t;
        FOV = f;
        FOV2 = f2;
        FOV_duration = duration;
    }
    void zoomAccel(float initial, float vel, float acc, float duration, int t) {
        if (duration == 0.0F) { return; }
        accel = true;
        FOV_time = t;
        FOV2 = initial;
        FOV_vel = vel;
        FOV_acc = acc;
        FOV_duration = duration;
    }
    void enable() {
        FOV = 90.0F;
        FOV2 = 90.0F;
    }
    // CGCam_Update im Bild cg.time = t: der Wert fuer CG_CalcFOVFromX.
    float update(int t) {
        if (accel) {
            float actual = FOV;
            const float tt = static_cast<float>(t - FOV_time) * 0.001F;
            if (static_cast<float>(FOV_time) + FOV_duration < static_cast<float>(t)) {
                accel = false;
            } else {
                actual = FOV2 + FOV_vel * tt + 0.5F * FOV_acc * tt * tt;
                actual = std::clamp(actual, 1.0F, 180.0F);
                FOV = actual;
            }
            return actual;
        }
        if (zooming) {
            if (static_cast<float>(FOV_time) + FOV_duration < static_cast<float>(t)) {
                FOV = FOV2;
                zooming = false;
                return FOV;
            }
            return FOV + ((FOV2 - FOV) / FOV_duration) * static_cast<float>(t - FOV_time);
        }
        return FOV;
    }
};

// Bild fuer Bild (1 ms) mit Ereignissen vor dem Bild ihrer Zeit; Werte an den
// Probezeiten.
std::vector<float> engineLauf(const std::vector<std::pair<int, std::function<void(EngineKamera&, int)>>>& ereignisse,
                              const std::vector<int>& proben) {
    EngineKamera k;
    std::vector<float> aus;
    std::size_t e = 0;
    std::size_t p = 0;
    const int ende = proben.empty() ? 0 : proben.back();
    for (int t = 0; t <= ende; ++t) {
        while (e < ereignisse.size() && ereignisse[e].first <= t) {
            ereignisse[e].second(k, t);
            ++e;
        }
        const float w = k.update(t);
        while (p < proben.size() && proben[p] == t) {
            aus.push_back(w);
            ++p;
        }
    }
    return aus;
}

Roff bahnMitNotiz(const std::string& notiz) {
    Roff r;
    r.fassung = 2;
    r.bildMs = 100;
    r.lerp = 10;
    for (int k = 0; k < 4; ++k) {
        RoffBild b;
        b.ort[0] = 10.0F;
        r.bilder.push_back(b);
    }
    r.notizen = {notiz};
    r.bilder[0].ersteNotiz = 0;
    r.bilder[0].notizen = 1;
    return r;
}

CameraTrack kamera(const std::string& skript, const std::shared_ptr<const Roff>& bahn) {
    Script s;
    std::vector<Diag> d;
    (void)readScript("//Generated by BehavEd\r\n" + skript, s, d);
    return buildCameraTrack(s, nullptr, [bahn](const std::string& name) {
        return name == "test/bahn" ? bahn : std::shared_ptr<const Roff>{};
    });
}

void vergleiche(const std::string& was, const CameraTrack& t, const std::vector<int>& proben,
                const std::vector<float>& soll) {
    bool gleich = soll.size() == proben.size();
    std::string text;
    for (std::size_t i = 0; gleich && i < proben.size(); ++i) {
        const float ist = t.at(static_cast<double>(proben[i])).fovX;
        text += " " + std::to_string(proben[i]) + ":" + zahl(ist) + "/" + zahl(soll[i]);
        if (!nah(ist, soll[i])) { gleich = false; }
    }
    pruefe(gleich, was + " (Zeit:Vorschau/Engine)" + text);
}

void kameraAccelZoom() {
    std::printf("--- 6. CAMERA_ACCEL und ZOOM gleichzeitig ---\n");
    const std::string vorspann = "camera ( ENABLE );\n";
    // a) "fovaccel 90 -40 20 1000" ab 0, ZOOM 50 ueber 1000 bei 500: die
    //    Beschleunigung laeuft von 50 aus mit der Zoomdauer weiter, bei 1500
    //    springt das Bild auf 50.
    {
        const auto bahn = std::make_shared<Roff>(bahnMitNotiz("fovaccel 90 -40 20 1000"));
        const CameraTrack t = kamera(vorspann + "camera ( PATH, \"test/bahn\" );\nwait ( 500.000 );\n"
                                                "camera ( ZOOM, 50.000, 1000.000 );\nwait ( 3000.000 );\n",
                                     bahn);
        const std::vector<int> proben{250, 499, 750, 1000, 1250, 1499, 1510, 2000, 3000};
        const std::vector<float> soll = engineLauf(
            {{0, [](EngineKamera& k, int) { k.enable(); }},
             {0, [](EngineKamera& k, int z) { k.zoomAccel(90.0F, -40.0F, 20.0F, 1000.0F, z); }},
             {500, [](EngineKamera& k, int z) { k.zoom(50.0F, 1000.0F, z); }}},
            proben);
        vergleiche("a) fovaccel, dann ZOOM: von 50 weiter beschleunigt, Sprung auf 50 am Ende", t, proben, soll);
    }
    // b) ZOOM 40 ueber 2000 ab 0, "fovaccel 90 -40 20 500" bei 500: nach dem
    //    Ende der Beschleunigung (1000) gilt FOV2 der Notiz - 90, nicht 40.
    {
        const auto bahn = std::make_shared<Roff>(bahnMitNotiz("fovaccel 90 -40 20 500"));
        const CameraTrack t = kamera(vorspann + "camera ( ZOOM, 40.000, 2000.000 );\nwait ( 500.000 );\n"
                                                "camera ( PATH, \"test/bahn\" );\nwait ( 3000.000 );\n",
                                     bahn);
        const std::vector<int> proben{250, 499, 750, 999, 1010, 1500, 2500};
        const std::vector<float> soll = engineLauf(
            {{0, [](EngineKamera& k, int) { k.enable(); }},
             {0, [](EngineKamera& k, int z) { k.zoom(40.0F, 2000.0F, z); }},
             {500, [](EngineKamera& k, int z) { k.zoomAccel(90.0F, -40.0F, 20.0F, 500.0F, z); }}},
            proben);
        vergleiche("b) ZOOM, dann fovaccel: am Ende springt das Bild auf den Anfang der Notiz", t, proben, soll);
    }
    // c) "fovaccel" ab 0, "fovzoom 30 60 800" in einer zweiten Bahn bei 300:
    //    CGCam_Zoom2 laesst CAMERA_ACCEL an - es rechnet von 60 aus.
    {
        auto doppelt = std::make_shared<Roff>(bahnMitNotiz("fovaccel 90 -40 20 2000"));
        doppelt->notizen.emplace_back("fovzoom 30 60 800");
        doppelt->bilder[3].ersteNotiz = 1;
        doppelt->bilder[3].notizen = 1;
        const CameraTrack t = kamera(vorspann + "camera ( PATH, \"test/bahn\" );\nwait ( 3000.000 );\n", doppelt);
        const std::vector<int> proben{200, 299, 500, 1000, 1099, 1110, 2000};
        const std::vector<float> soll = engineLauf(
            {{0, [](EngineKamera& k, int) { k.enable(); }},
             {0, [](EngineKamera& k, int z) { k.zoomAccel(90.0F, -40.0F, 20.0F, 2000.0F, z); }},
             {300, [](EngineKamera& k, int z) { k.zoom2(30.0F, 60.0F, 800.0F, z); }}},
            proben);
        vergleiche("c) fovaccel, dann fovzoom: CAMERA_ACCEL rechnet von FOV2 = 60 mit 800 ms", t, proben, soll);
    }
    // d) ENABLE waehrend "fovaccel": FOV2 = 90, die Beschleunigung laeuft mit
    //    ihrer alten Zeit weiter.
    {
        const auto bahn = std::make_shared<Roff>(bahnMitNotiz("fovaccel 60 20 0 1500"));
        const CameraTrack t = kamera(vorspann + "camera ( PATH, \"test/bahn\" );\nwait ( 700.000 );\n"
                                                "camera ( ENABLE );\nwait ( 3000.000 );\n",
                                     bahn);
        const std::vector<int> proben{300, 699, 701, 1200, 1499, 1510, 2500};
        const std::vector<float> soll = engineLauf(
            {{0, [](EngineKamera& k, int) { k.enable(); }},
             {0, [](EngineKamera& k, int z) { k.zoomAccel(60.0F, 20.0F, 0.0F, 1500.0F, z); }},
             {700, [](EngineKamera& k, int) { k.enable(); }}},
            proben);
        vergleiche("d) ENABLE waehrend fovaccel: weiter von 90 mit der alten Zeit", t, proben, soll);
    }
    // e) ZOOM ohne Dauer waehrend "fovaccel": CGCam_SetFOV, das naechste Bild
    //    ueberschreibt FOV - nichts zu sehen.
    {
        const auto bahn = std::make_shared<Roff>(bahnMitNotiz("fovaccel 90 -40 20 1000"));
        const CameraTrack t = kamera(vorspann + "camera ( PATH, \"test/bahn\" );\nwait ( 400.000 );\n"
                                                "camera ( ZOOM, 10.000, 0.000 );\nwait ( 3000.000 );\n",
                                     bahn);
        const std::vector<int> proben{200, 401, 800, 1200};
        const std::vector<float> soll = engineLauf(
            {{0, [](EngineKamera& k, int) { k.enable(); }},
             {0, [](EngineKamera& k, int z) { k.zoomAccel(90.0F, -40.0F, 20.0F, 1000.0F, z); }},
             {400, [](EngineKamera& k, int z) { k.zoom(10.0F, 0.0F, z); }}},
            proben);
        vergleiche("e) ZOOM ohne Dauer waehrend fovaccel: ohne Wirkung", t, proben, soll);
    }
    // f) Ohne Beschleunigung bleibt ZOOM wie bisher: 90 -> 30 ueber 1000.
    {
        const CameraTrack t = kamera(vorspann + "camera ( ZOOM, 30.000, 1000.000 );\nwait ( 2000.000 );\n", nullptr);
        const std::vector<int> proben{0, 500, 999, 1500};
        const std::vector<float> soll = engineLauf(
            {{0, [](EngineKamera& k, int) { k.enable(); }}, {0, [](EngineKamera& k, int z) { k.zoom(30.0F, 1000.0F, z); }}},
            proben);
        vergleiche("f) ZOOM allein", t, proben, soll);
    }
}

// --- 3. forced_rgb_saber_colours ---------------------------------------------

void schwertFarben() {
    std::printf("--- 3. saberColorRGB / forced_rgb_saber_colours ---\n");
    const std::string text =
        "test_rgb\n{\n\tplayerModel jedi_hm\n\tsaberColor blue\n\tsaberColorRGB red\n"
        "\tsaber2ColorRGB3 green\n\tsaberColor3 purple\n\tsaberColorRGB4 orange\n\tsaberLength 30\n}\n";
    NpcMap m;
    parseNpcFile(text, m, false);
    const auto it = m.find("test_rgb");
    bool ok = it != m.end();
    std::string ist;
    if (ok) {
        const NpcDef& d = it->second;
        for (int b = 0; b < kMaxKlingen; ++b) {
            const std::string soll = b == 2 ? "purple" : "blue";
            ist += d.saberFarbe[0][static_cast<std::size_t>(b)] + ",";
            if (d.saberFarbe[0][static_cast<std::size_t>(b)] != soll) { ok = false; }
            if (!d.saberFarbe[1][static_cast<std::size_t>(b)].empty()) { ok = false; }
        }
        ok = ok && nah(d.saberLaenge[0][0], 30.0F);
    }
    pruefe(ok, "OpenJK/Movie Duels: saberColorRGB, saber2ColorRGB3, saberColorRGB4 aendern nichts (" + ist + ")");
    const std::string jk =
        "test_jk\n{\n\tplayerModel kyle\n\tsaberColor green\n\tsaberColor2 red\n\tsaber2Color blue\n"
        "\tsaberLength 20\n}\n";
    NpcMap j;
    parseNpcFile(jk, j, true);
    const auto jt = j.find("test_jk");
    bool jok = jt != j.end();
    if (jok) {
        const NpcDef& d = jt->second;
        for (int b = 0; b < kMaxKlingen; ++b) {
            if (d.saberFarbe[0][static_cast<std::size_t>(b)] != "green" || !d.saberFarbe[1][static_cast<std::size_t>(b)].empty()) {
                jok = false;
            }
        }
        jok = jok && d.saberLaenge[0][0] == 0.0F;
    }
    pruefe(jok, "Jedi Outcast: nur saberColor (gruen), saberColor2/saber2Color/saberLength unbekannt");
}

// --- 5. func_breakable: zweites use in der Verzoegerung --------------------------

MapEntity ent(const std::string& klasse, std::vector<std::pair<std::string, std::string>> keys) {
    MapEntity e;
    e.classname = klasse;
    for (const auto& [k, v] : keys) {
        if (k == "targetname") { e.targetname = v; }
        if (k == "origin") { e.origin = v; }
    }
    keys.emplace_back("classname", klasse);
    e.keys = std::move(keys);
    return e;
}

Script lies(const std::string& rumpf) {
    Script s;
    std::vector<Diag> d;
    (void)readScript("//Generated by BehavEd\r\n\r\n" + rumpf, s, d);
    return s;
}

void bruchVerzoegerung() {
    std::printf("--- 5. func_breakable: zweites use in der Bruchverzoegerung ---\n");
    // Takt 50 ms; wait endet im ersten Bild NACH stempel + dauer
    // (TaskManager.cpp:1141 "<"). G_RunThink laeuft am Platz der Entity vor
    // ihrem ICARUS (g_main.cpp:1062-1088). Der Startskript-Traeger kommt nach
    // allen Karten-Entities, "vorn" (Entity 1) vor "glas2" (Entity 4).
    MapData m;
    m.entities = {ent("func_usable", {{"targetname", "vorn"}, {"usescript", "x/vorn"}, {"model", "*4"}}),
                  ent("func_breakable", {{"targetname", "fels"}, {"target", "danach"}, {"delay", "2"}, {"model", "*1"},
                                         {"usescript", "x/ruf"}}),
                  ent("func_breakable", {{"targetname", "glas"}, {"target", "danach2"}, {"model", "*2"}}),
                  ent("func_breakable", {{"targetname", "glas2"}, {"target", "danach3"}, {"model", "*3"}}),
                  ent("target_relay", {{"targetname", "danach"}}), ent("target_relay", {{"targetname", "danach2"}}),
                  ent("target_relay", {{"targetname", "danach3"}})};
    auto ruf = std::make_shared<Script>(lies("print ( \"ruf\" );\n"));
    // vorn: bei 50 (der Traeger kommt nach vorn, das Skript startet im
    // naechsten Bild) Bruch, G_FreeEntity bei 100; use bei 100 vor dem Platz
    // von glas2 -> bricht neu (frei bei 150), ebenso bei 150; bei 250 ist es fort.
    auto vorn = std::make_shared<Script>(lies("use ( \"glas2\" );\nwait ( 49.000 );\nuse ( \"glas2\" );\n"
                                              "wait ( 49.000 );\nuse ( \"glas2\" );\nwait ( 99.000 );\nuse ( \"glas2\" );\n"));
    AblaufWelt w;
    w.karte = &m;
    w.skript = [ruf, vorn](const std::string& p) -> const Script* {
        return p == "x/ruf" ? ruf.get() : p == "x/vorn" ? vorn.get() : nullptr;
    };
    // fels: use bei 0 und bei 1050 (wait 49 -> 50, wait 950 -> 1050), delay 2 s
    // -> ein Bruch bei 3050, nicht bei 2000. glas: zweimal im Bild 0 -> zwei
    // Brueche; bei 50 ist G_FreeEntity (Platz 3) vor dem Traeger dran -> nichts.
    const Script s = lies("use ( \"fels\" );\nuse ( \"glas\" );\nuse ( \"glas\" );\nuse ( \"vorn\" );\n"
                          "wait ( 49.000 );\nuse ( \"glas\" );\nwait ( 950.000 );\nuse ( \"fels\" );\nwait ( 4000.000 );\n");
    const Ablauf a = simuliereAblauf(s, "", w);
    const auto zeiten = [&a](const char* name) {
        std::vector<double> z;
        for (const Ablauf::Benutzung& b : a.benutzt) {
            if (b.name == name) { z.push_back(b.ms); }
        }
        return z;
    };
    const auto text = [](const std::vector<double>& z) {
        std::string t;
        for (const double x : z) { t += " " + std::to_string(static_cast<int>(x)); }
        return t;
    };
    const auto gleich = [](const std::vector<double>& z, const std::vector<double>& soll) {
        if (z.size() != soll.size()) { return false; }
        for (std::size_t i = 0; i < z.size(); ++i) {
            if (std::fabs(z[i] - soll[i]) > 0.5) { return false; }
        }
        return true;
    };
    int rufe = 0;
    const std::function<void(const std::vector<Node>&)> zaehle = [&](const std::vector<Node>& ns) {
        for (const Node& n : ns) {
            if (n.name == "print" && !n.args.empty() && n.args[0].text == "ruf") { ++rufe; }
            zaehle(n.children);
        }
    };
    zaehle(a.flach.nodes);
    const std::vector<double> felsUse = zeiten("fels");
    pruefe(gleich(felsUse, {0.0, 1050.0}), "Nachbau: fels benutzt bei 0 und 1050 (" + text(felsUse) + ")");
    pruefe(gleich(zeiten("danach"), {3050.0}),
           "Nachbau: das zweite use stellt nextthink neu - ein Bruch bei 3050 (Ziele bei" + text(zeiten("danach")) + ")");
    pruefe(rufe == 2, "Nachbau: der use-Skript laeuft bei jedem use (" + std::to_string(rufe) + " mal)");
    pruefe(gleich(zeiten("danach2"), {0.0, 0.0}),
           "Nachbau: glas bricht zweimal im Bild 0, bei 50 ist es fort (Ziele bei" + text(zeiten("danach2")) + ")");
    pruefe(gleich(zeiten("danach3"), {50.0, 100.0, 150.0}),
           "Nachbau: glas2 - ein use vor seinem Platz im Bild von G_FreeEntity bricht neu (Ziele bei" +
               text(zeiten("danach3")) + ")");
    BspGeometry g;
    g.models.resize(5);
    for (int i = 1; i <= 4; ++i) {
        auto& mo = g.models[static_cast<std::size_t>(i)];
        mo.maxs[0] = 8;
        mo.maxs[1] = 64;
        mo.maxs[2] = 128;
    }
    // Wie die Ansicht (app_view3d.cpp): alle uses des Nachbaus, aufgeloest,
    // mit ihrem Platz im Bild.
    std::vector<MoverUse> uses;
    for (const Ablauf::Benutzung& b : a.benutzt) { uses.push_back(MoverUse{b.ms, b.name, true, b.ausloeser, b.reihe}); }
    MoverSim sim;
    sim.baue(m, g, uses, nullptr, 6000.0);
    std::vector<double> brueche[4];
    for (const Bruch& b : sim.brueche()) {
        if (b.entity >= 1U && b.entity <= 3U) { brueche[b.entity].push_back(b.ms); }
    }
    pruefe(gleich(brueche[1], {3050.0}) && sim.at(1, 3050).sichtbar && !sim.at(1, 3100).sichtbar,
           "MoverSim: fels bricht einmal bei 3050, der Brush steht bis 3100 (" + text(brueche[1]) + ")");
    pruefe(gleich(brueche[2], {0.0, 0.0}) && sim.at(2, 49).sichtbar && !sim.at(2, 50).sichtbar,
           "MoverSim: glas bricht zweimal bei 0, weg ab 50 (" + text(brueche[2]) + ")");
    pruefe(gleich(brueche[3], {50.0, 100.0, 150.0}) && sim.at(3, 199).sichtbar && !sim.at(3, 200).sichtbar,
           "MoverSim: glas2 bricht bei 50, 100, 150, weg ab 200 (" + text(brueche[3]) + ")");
}

// 5a. Remove vor dem ersten Add. Abschrift des ghoul2-Felds der Engine:
// G2API_InitGhoul2Model nimmt das erste Fach mit mModelindex < 0, sonst ein
// neues (G2_API.cpp); G2API_AttachG2Model scheitert ohne Bolzenliste am Ziel
// ("Empty Bolt List"); G2API_RemoveGhoul2Model(ghoul2, cinematicModel) setzt
// das Fach zurueck; G2_Sort_Models zeichnet Wurzeln und deren Kinder.
struct G2Fach {
    std::string datei;
    bool gueltig = false;
    bool bolzen = false;   // mBltlist nicht leer (der Koerper)
    int haengtAn = -1;     // mModelBoltLink
};
struct G2Feld {
    std::vector<G2Fach> fach{G2Fach{"koerper", true, true, -1}};
    int cinematicModel = 0;   // die Entity ist genullt
    int init(const std::string& datei) {
        for (std::size_t i = 0; i < fach.size(); ++i) {
            if (!fach[i].gueltig) {
                fach[i] = G2Fach{datei, true, false, -1};
                return static_cast<int>(i);
            }
        }
        fach.push_back(G2Fach{datei, true, false, -1});
        return static_cast<int>(fach.size()) - 1;
    }
    void add(const std::string& datei) {   // Q3_AddRHandModel, playerModel 0
        cinematicModel = init(datei);
        const G2Fach& ziel = fach[0];
        if (ziel.gueltig && ziel.bolzen) {
            fach[static_cast<std::size_t>(cinematicModel)].haengtAn = 0;
        }
    }
    void remove() {   // Q3_RemoveRHandModel
        if (cinematicModel >= 0 && fach[static_cast<std::size_t>(cinematicModel)].gueltig) {
            fach[static_cast<std::size_t>(cinematicModel)] = G2Fach{};
        }
    }
    // Was gezeichnet wird: "datei@wurzel" / "datei@hand" (G2_Sort_Models).
    std::vector<std::string> bild() const {
        std::vector<std::string> v;
        for (const G2Fach& f : fach) {
            if (!f.gueltig) { continue; }
            if (f.haengtAn < 0) {
                v.push_back(f.datei + "@wurzel");
            } else if (fach[static_cast<std::size_t>(f.haengtAn)].gueltig) {
                v.push_back(f.datei + "@hand");
            }
        }
        std::sort(v.begin(), v.end());
        return v;
    }
};

std::vector<std::string> bildVon(const Actor& a, const ActorState& st) {
    std::vector<std::string> v;
    if (!st.koerperEntfernt) { v.emplace_back("koerper@wurzel"); }
    for (const KinoModell& k : st.kino) {
        if (k.modell < 0) { continue; }
        v.push_back(a.modelle[static_cast<std::size_t>(k.modell)] + (k.anWurzel ? "@wurzel" : "@hand"));
    }
    std::sort(v.begin(), v.end());
    return v;
}

void entfernenVorAdd() {
    std::printf("--- 5a. Remove vor dem ersten Add ---\n");
    struct Schritt { bool add; const char* datei; };
    const auto pruefeFolge = [](const char* was, const std::vector<Schritt>& folge) {
        std::string rumpf = "affect ( \"soldat\", /*@AFFECT_TYPE*/ FLUSH )\r\n{\r\n";
        for (const Schritt& sch : folge) {
            rumpf += "\twait ( 1000.000 );\r\n\tset ( /*@SET_TYPES*/ \"";
            rumpf += sch.add ? "SET_ADDRHANDBOLT_MODEL" : "SET_REMOVERHANDBOLT_MODEL";
            rumpf += std::string("\", \"") + sch.datei + "\" );\r\n";
        }
        rumpf += "}\r\n";
        Script sc;
        std::vector<Diag> d;
        (void)readScript("//Generated by BehavEd\r\n" + rumpf, sc, d);
        const MapData m;
        const Scene s = buildScene(sc, m);
        if (s.actors.empty()) {
            pruefe(false, std::string(was) + ": keine Figur");
            return;
        }
        const Actor& a = s.actors.front();
        G2Feld g;
        bool gleich = true;
        std::string text;
        for (std::size_t i = 0; i < folge.size(); ++i) {
            if (folge[i].add) {
                g.add(folge[i].datei);
            } else {
                g.remove();
            }
            const std::vector<std::string> soll = g.bild();
            const std::vector<std::string> ist = bildVon(a, a.at(1000.0 * static_cast<double>(i + 1) + 500.0));
            gleich = gleich && soll == ist;
            text += " [";
            for (const std::string& x : ist) { text += " " + x; }
            text += " ]";
        }
        pruefe(gleich, std::string(was) + ":" + text);
    };
    pruefeFolge("Remove vor dem Add entfernt die Figur, ein zweites tut nichts, Adds danach an der Wurzel",
                {{false, "x"}, {false, "x"}, {true, "models/items/a.glm"}, {true, "models/items/b.glm"}, {false, "x"},
                 {false, "x"}});
    pruefeFolge("Add, Remove, Remove: die Figur bleibt, das Modell an der Hand",
                {{true, "models/items/a.glm"}, {false, "x"}, {false, "x"}, {true, "models/items/b.glm"}});
}

// 1. useBBox im Effektpfad: CParticle::UpdateOrigin (FxPrimitives.cpp:
// 280-345 OpenJK) als Abschrift - je Bild (com_maxfps 125: 8 ms) erst
// UpdateVelocity, dann der neue Ort; Physik nur, wenn CG_PointContents dort
// fest ist (ausser expensivePhysics); Spur mit Kasten (useBBox) bzw. Punkt;
// startsolid/allsolid/fraction 1 heisst kein Treffer; sonst die Beschleunigung
// fuer den Weg bis zur Wand, spiegeln, daempfen, liegen bei vel.z < 4.
struct TeilchenWelt {
    float ort[3] = {0.0F, 0.0F, -8.0F};
    float lo[3] = {-1000.0F, -1000.0F, -8.0F};
    float hi[3] = {1000.0F, 1000.0F, 8.0F};   // Boden z -16 .. 0
    bool fest(const float q[3]) const {
        for (int a = 0; a < 3; ++a) {
            if (q[a] - ort[a] < lo[a] || q[a] - ort[a] > hi[a]) { return false; }
        }
        return true;
    }
};

struct TeilchenLauf {
    float o[3];
    float v[3];
    bool liegt = false;
};

// Die Abschrift: Lage nach `bilder` Bildern zu dt Sekunden.
TeilchenLauf engineTeilchen(const TeilchenWelt& w, const float start[3], const float vel[3], float g, float el,
                            bool kasten, float halb, bool teuer, int bilder, float dt) {
    TeilchenLauf t;
    float a[3] = {0.0F, 0.0F, g};
    for (int k = 0; k < 3; ++k) {
        t.o[k] = start[k];
        t.v[k] = vel[k];
    }
    const float mi[3] = {kasten ? -halb : 0.0F, kasten ? -halb : 0.0F, kasten ? -halb : 0.0F};
    const float ma[3] = {kasten ? halb : 0.0F, kasten ? halb : 0.0F, kasten ? halb : 0.0F};
    for (int b = 0; b < bilder; ++b) {
        for (int k = 0; k < 3; ++k) { t.v[k] += a[k] * dt; }   // UpdateVelocity
        float neu[3];
        for (int k = 0; k < 3; ++k) { neu[k] = t.o[k] + dt * t.v[k]; }
        if (!t.liegt && (teuer || w.fest(neu))) {
            const KollisionsSpur tr = kollisionsSpurKasten(t.o, mi, ma, neu, 1U, w.ort, w.lo, w.hi, 1U);
            if (!(tr.startsolid || tr.allsolid || tr.fraction == 1.0F)) {
                for (int k = 0; k < 3; ++k) { t.v[k] += dt * tr.fraction * a[k]; }
                const float dot = t.v[0] * tr.normale[0] + t.v[1] * tr.normale[1] + t.v[2] * tr.normale[2];
                for (int k = 0; k < 3; ++k) {
                    t.v[k] += -2.0F * dot * tr.normale[static_cast<std::size_t>(k)];
                    t.v[k] *= el;
                }
                if (tr.normale[2] > 0.0F && t.v[2] < 4.0F) {
                    for (int k = 0; k < 3; ++k) {
                        t.v[k] = 0.0F;
                        a[k] = 0.0F;
                    }
                    t.liegt = true;
                }
                for (int k = 0; k < 3; ++k) { t.o[k] = tr.endpos[static_cast<std::size_t>(k)]; }
                continue;
            }
        }
        for (int k = 0; k < 3; ++k) { t.o[k] = neu[k]; }
    }
    return t;
}

void teilchenSpur() {
    std::printf("--- 1. useBBox: Kastenspur der Kollisionskarte im Effektpfad ---\n");
    const TeilchenWelt w;
    struct Fall {
        const char* was;
        bool kasten;
        bool teuer;
    };
    const Fall faelle[] = {{"Punkt", false, false}, {"Kasten +-5 ohne expensivePhysics", true, false},
                           {"Kasten +-5 mit expensivePhysics", true, true}};
    const float start[3] = {0.0F, 0.0F, 100.0F};
    const float vel[3] = {30.0F, 0.0F, 0.0F};
    for (const Fall& f : faelle) {
        const float lo[3] = {f.kasten ? -5.0F : 0.0F, f.kasten ? -5.0F : 0.0F, f.kasten ? -5.0F : 0.0F};
        const float hi[3] = {f.kasten ? 5.0F : 0.0F, f.kasten ? 5.0F : 0.0F, f.kasten ? 5.0F : 0.0F};
        // Wie efxdraw.cpp spurFuer: erst der Inhalt am neuen Ort, dann die Spur.
        const efx::sim::TraceFn spur = [&w, lo, hi, f](const efx::camera::Vec3& von, const efx::camera::Vec3& bis,
                                                         float, efx::sim::Spur& aus) {
            const float b[3] = {bis.x, bis.y, bis.z};
            if (!f.teuer && !w.fest(b)) { return false; }
            const float a[3] = {von.x, von.y, von.z};
            const KollisionsSpur t = kollisionsSpurKasten(a, lo, hi, b, 1U, w.ort, w.lo, w.hi, 1U);
            if (t.startsolid || t.allsolid || t.fraction >= 1.0F) { return false; }
            aus.punkt = efx::camera::Vec3{t.endpos[0], t.endpos[1], t.endpos[2]};
            aus.normale = efx::camera::Vec3{t.normale[0], t.normale[1], t.normale[2]};
            aus.anteil = t.fraction;
            return true;
        };
        bool gleich = true;
        std::string zeilen;
        for (const int bilder : {50, 62, 63, 100, 200, 400}) {
            const TeilchenLauf e = engineTeilchen(w, start, vel, -800.0F, 0.5F, f.kasten, 5.0F, f.teuer, bilder, 0.008F);
            const efx::sim::FlugErgebnis r = efx::sim::flugbahn(
                efx::camera::Vec3{start[0], start[1], start[2]}, efx::camera::Vec3{vel[0], vel[1], vel[2]},
                efx::camera::Vec3{0.0F, 0.0F, 0.0F}, -800.0F, static_cast<float>(bilder) * 0.008F, 0.5F, false, spur);
            gleich = gleich && std::fabs(r.position.z - e.o[2]) < 0.01F && std::fabs(r.position.x - e.o[0]) < 0.01F;
            char z[96];
            std::snprintf(z, sizeof(z), " %d:%.2f/%.2f", bilder * 8, static_cast<double>(r.position.z),
                          static_cast<double>(e.o[2]));
            zeilen += z;
        }
        pruefe(gleich, std::string(f.was) + " (ms:Vorschau/Engine z)" + zeilen);
    }
    const TeilchenLauf durch = engineTeilchen(w, start, vel, -800.0F, 0.5F, true, 5.0F, false, 400, 0.008F);
    const TeilchenLauf liegt = engineTeilchen(w, start, vel, -800.0F, 0.5F, true, 5.0F, true, 400, 0.008F);
    pruefe(durch.o[2] < -16.0F && liegt.liegt && std::fabs(liegt.o[2] - 5.0F) < 0.2F,
           "Engine: ohne expensivePhysics faellt der Kasten durch (startsolid), mit liegt er auf (z " +
               std::to_string(durch.o[2]) + " / " + std::to_string(liegt.o[2]) + ")");
}

// 3a. weapon_saber mit NPC_type: FinishSpawningItem (g_items.cpp:1138-1158
// OpenJK, 1589-1616 MD) - das Modell aus WP_SaberParseParms(NPC_type),
// "player" mit g_saber "single_1", unbekannt oder ohne saberModel die
// Vorgabe; Jedi Outcast nimmt immer world_model.
void liegendeSchwerter() {
    std::printf("--- 3a. weapon_saber mit NPC_type ---\n");
    std::map<std::string, SaberDef> sabers;
    sabers["single_1"].model = "models/weapons2/saber_spieler/saber_spieler.glm";
    sabers["dual_3"].model = "models/weapons2/saber_3/saber_3.glm";
    sabers["ohne_modell"].name = "ohne";
    Gegenstand g;
    g.classname = "weapon_saber";
    g.worldmodel = "models/weapons2/saber/saber_w.glm";
    g.typ = "IT_WEAPON";
    g.tag = "WP_SABER";
    const auto stelle = [](const char* npc) {
        MapEntity e;
        e.classname = "weapon_saber";
        e.origin = "0 0 0";
        if (npc != nullptr) { e.keys.emplace_back("NPC_type", npc); }
        e.keys.emplace_back("spawnflags", "1");   // ITMSF_SUSPEND: ohne Fallen
        return e;
    };
    struct Fall {
        const char* npc;
        bool jk2;
        const char* soll;
    };
    const Fall faelle[] = {{"Dual_3", false, "models/weapons2/saber_3/saber_3.glm"},
                           {"player", false, "models/weapons2/saber_spieler/saber_spieler.glm"},
                           {"unbekannt", false, kStandardGriff},
                           {"ohne_modell", false, kStandardGriff},
                           {nullptr, false, "models/weapons2/saber/saber_w.glm"},
                           {"dual_3", true, "models/weapons2/saber/saber_w.glm"}};
    bool alle = true;
    std::string text;
    for (const Fall& f : faelle) {
        const GegenstandAufstellung a = gegenstandAufstellen(stelle(f.npc), g, 1, true, {}, &sabers, f.jk2);
        alle = alle && a.sichtbar && a.modell == f.soll && a.glm;
        text += std::string(" ") + (f.npc != nullptr ? f.npc : "-") + (f.jk2 ? "(JK2)" : "") + "=" + a.modell;
    }
    pruefe(alle, "Modell je NPC_type:" + text);
}

}  // namespace

int main() {
    kameraAccelZoom();
    schwertFarben();
    bruchVerzoegerung();
    entfernenVorAdd();
    teilchenSpur();
    liegendeSchwerter();
    std::printf("%d Fehler\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
