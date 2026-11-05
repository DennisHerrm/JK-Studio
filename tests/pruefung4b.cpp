// pruefung4b.cpp - Pruefung 4b: Entities, Effekte, Kamera - je Punkt gegen die
// Formel der Engine (OpenJK, Movie Duels, Jedi Outcast) mit eigenen Daten.
//
// 1. useBBox im Effektpfad: efx::sim gegen eine Abschrift von
//    CParticle::UpdateOrigin mit der Kastenspur der Kollisionskarte.
// 2. trigger_hurt gegen den Spieler: sein Tod haelt die Skripte an.
// 4. Fahrzeuge: Schwerkraft nach der .veh (Laeufer, Jaeger, Gleiter).
// 5c. misc_model_breakable target4: G_RunObject als Abschrift.
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
#include "bhed/abwurf.h"
#include "bhed/bspgeo.h"
#include "bhed/camtrack.h"
#include "bhed/efx/sim.h"
#include "bhed/geschuetz.h"
#include "bhed/gegenstaende.h"
#include "bhed/kollision.h"
#include "bhed/mover.h"
#include "bhed/roff.h"
#include "bhed/scene.h"
#include "bhed/script.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
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

// 5c. misc_model_breakable target4: misc_model_breakable_gravity_init +
// misc_model_throw_at_target4 + G_RunObject/G_BounceObject (g_breakable.cpp:
// 629-702, 1035-1103; g_object.cpp:36-263 OpenJK) als Abschrift gegen
// objektAbsetzen/wirfObjekt. Welt: Boden z -16..0 und eine Wand x 40..56.
struct WurfWelt {
    NavSpur spur(const NavVec& a, const NavVec& mi, const NavVec& ma, const NavVec& b) const {
        const float bodenOrt[3] = {0.0F, 0.0F, -8.0F};
        const float bodenLo[3] = {-2000.0F, -2000.0F, -8.0F};
        const float bodenHi[3] = {2000.0F, 2000.0F, 8.0F};
        const float wandOrt[3] = {48.0F, 0.0F, 0.0F};
        const float wandLo[3] = {-8.0F, -2000.0F, -16.0F};
        const float wandHi[3] = {8.0F, 2000.0F, 2000.0F};
        KollisionsSpur r = kollisionsSpurKasten(a.data(), mi.data(), ma.data(), b.data(), 1U, bodenOrt, bodenLo, bodenHi, 1U);
        const KollisionsSpur w = kollisionsSpurKasten(a.data(), mi.data(), ma.data(), b.data(), 1U, wandOrt, wandLo, wandHi, 1U);
        if (w.fraction < r.fraction) { r = w; }
        NavSpur n;
        n.startsolid = r.startsolid || w.startsolid;
        n.allsolid = r.allsolid;
        n.fraction = r.fraction;
        n.endpos = r.endpos;
        n.normal = r.normale;
        return n;
    }
};

struct EngineObjekt {
    // trajectory_t
    int typ = 0;   // 0 TR_STATIONARY, 1 TR_GRAVITY
    double zeit = 0.0;
    NavVec basis{};
    NavVec delta{};
    NavVec cur{};
    NavVec winkel{};
    float masse = 0.0F;
    NavVec eval(double t) const {
        if (typ == 0) { return basis; }
        const auto d = static_cast<float>((t - zeit) * 0.001);
        return {basis[0] + d * delta[0], basis[1] + d * delta[1], basis[2] + d * delta[2] - 0.5F * 800.0F * d * d};
    }
};

// G_RunObject zur Zeit t (level.previousTime = t - 50).
void engineRunObject(EngineObjekt& o, double t, const WurfWelt& w, const NavVec& mi, const NavVec& ma) {
    if (o.typ == 0) {
        o.typ = 1;
        o.basis = o.cur;
        o.zeit = t - 50.0;
    }
    const NavVec oldOrg = o.cur;
    const NavVec origin = o.eval(t);
    if (origin == o.cur) { return; }
    NavSpur tr = w.spur(o.cur, mi, ma, origin);
    if (!tr.startsolid && !tr.allsolid && tr.fraction != 0.0F) {
        o.cur = tr.endpos;
    } else {
        tr.fraction = 0.0F;
    }
    (void)oldOrg;
    if (tr.fraction == 1.0F) { return; }
    if (tr.normal[2] < 0.7F) {
        if (tr.fraction <= 0.0F) {
            o.cur = tr.endpos;
            o.basis = tr.endpos;
            o.delta = {0.0F, 0.0F, 0.0F};
            o.zeit = t;
            return;
        }
        // G_BounceObject
        const double hitTime = (t - 50.0) + 50.0 * static_cast<double>(tr.fraction);
        NavVec v = o.delta;
        v[2] -= 800.0F * static_cast<float>((hitTime - o.zeit) * 0.001);
        const float dot = v[0] * tr.normal[0] + v[1] * tr.normal[1] + v[2] * tr.normal[2];
        const float bf = std::min(60.0F / o.masse, 1.0F);
        for (std::size_t k = 0; k < 3; ++k) { o.delta[k] = (v[k] - 2.0F * dot * bf * tr.normal[k]) * 0.5F; }
        o.cur = tr.endpos;
        o.zeit = hitTime;
        o.basis = o.cur;
        return;
    }
    // pitch_roll_for_slope auf ebenem Boden: Nicken und Rollen 0
    o.winkel[0] = 0.0F;
    o.winkel[2] = 0.0F;
    o.typ = 0;
    o.basis = o.cur;
    o.delta = {0.0F, 0.0F, 0.0F};
}

void objektWurf() {
    std::printf("--- 5c. misc_model_breakable target4 (G_RunObject) ---\n");
    const WurfWelt welt;
    const NavSpurFn spur = [&welt](const NavVec& a, const NavVec& mi, const NavVec& ma, const NavVec& b, int,
                                   std::uint32_t) { return welt.spur(a, mi, ma, b); };
    ObjektStart s;
    s.ort = {0.0F, 0.0F, 40.0F};
    s.winkel = {10.0F, 45.0F, 5.0F};
    s.mins = {-8.0F, -8.0F, -8.0F};
    s.maxs = {8.0F, 8.0F, 8.0F};
    const ObjektStart ab = objektAbsetzen(s, spur);
    // dropToFloor: Spur von z 41 bis MIN_WORLD_COORD -> der Kasten 0.125 ueber dem Boden
    const NavSpur fall = welt.spur({0.0F, 0.0F, 41.0F}, s.mins, s.maxs, {0.0F, 0.0F, -65536.0F});
    pruefe(std::fabs(ab.ort[2] - fall.endpos[2]) < 1e-4F && ab.winkel[0] == 0.0F && ab.winkel[2] == 0.0F &&
               ab.winkel[1] == 45.0F,
           "Spawn: auf den Boden (z " + std::to_string(ab.ort[2]) + "), Nicken/Rollen vom ebenen Boden 0, Gier 45");
    const NavVec ziel{400.0F, 0.0F, 8.0F};
    const AbwurfBahn b = wirfObjekt(ab, {1000.0}, ziel, spur);
    // Abschrift
    EngineObjekt e;
    e.cur = ab.ort;
    e.winkel = ab.winkel;
    const float l8 = std::sqrt(3.0F * 64.0F);
    e.masse = l8 + l8;
    {
        NavVec dir{ziel[0] - e.cur[0], ziel[1] - e.cur[1], ziel[2] - e.cur[2]};
        const float len = std::sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
        for (float& d : dir) { d /= len; }
        float kb = 200.0F - len;
        if (kb < 100.0F) { kb = 100.0F; }
        const float m = std::max(e.masse, 50.0F);
        e.typ = 1;
        e.zeit = 1000.0;
        e.basis = e.cur;
        for (std::size_t k = 0; k < 3; ++k) { e.delta[k] = dir[k] * 1000.0F * kb / m * 0.8F; }
        e.delta[2] = dir[2] * 1000.0F * kb / m * 1.5F;
        if (e.delta[2] < kb) { e.delta[2] = kb; }
    }
    bool gleich = true;
    std::string text;
    double ruhe = -1.0;
    for (double t = 1050.0; t <= 6000.0; t += 50.0) {
        engineRunObject(e, t, welt, s.mins, s.maxs);
        NavVec o{};
        NavVec w{};
        const bool da = b.bei(t, o, w);
        // Gezeichnet wird s.pos zur Zeit t (CG_CalcEntityLerpPositions, seit Pruefung 4a in
        // AbwurfBahn::bei); im Bild des Abpralls ist das nicht currentOrigin.
        const NavVec bahn = e.eval(t);
        gleich = gleich && da && std::fabs(o[0] - bahn[0]) < 1e-3F && std::fabs(o[2] - bahn[2]) < 1e-3F &&
                 std::fabs(w[0] - e.winkel[0]) < 1e-3F;
        if (static_cast<long>(t) % 500 == 0) {
            char z[80];
            std::snprintf(z, sizeof(z), " %d:%.1f,%.1f/%.1f,%.1f", static_cast<int>(t), static_cast<double>(o[0]),
                          static_cast<double>(o[2]), static_cast<double>(e.cur[0]), static_cast<double>(e.cur[2]));
            text += z;
        }
        if (ruhe < 0.0 && e.typ == 0) { ruhe = t; }
    }
    pruefe(gleich && ruhe > 0.0 && std::fabs(b.ruheMs - ruhe) < 0.5,
           "Wurf zum Ziel, Abprall an der Wand, liegt bei " + std::to_string(static_cast<int>(b.ruheMs)) +
               " (Engine " + std::to_string(static_cast<int>(ruhe)) + "; t:Vorschau x,z/Engine x,z)" + text);
    bool effekt = !b.effekte.empty();
    for (const AbwurfBahn::Effekt& f : b.effekte) { effekt = effekt && f.datei == "melee/kick_impact_silent"; }
    pruefe(effekt && b.klaenge.size() == b.effekte.size(),
           "Aufprall: kick_impact_silent und objectHit je Treffer (" + std::to_string(b.effekte.size()) + ")");
}

// 4. Fahrzeuge: Laeufer/Tiere/Jaeger fallen mit der Schwerkraft, ein Gleiter (hoverHeight > 0) schwebt wie im
// Spiel. rc608 Teil 13e: Die Hoehen kommen aus dem Spiel selbst (Movie Duels, Karte g13d_reit in SP/g13d, swoop
// mit hoverHeight 30, hoverStrength 35, friction 1.5; Ort je Serverbild per ICARUS-print, g_ICARUSDebug 4):
// Spawn 61 ueber dem Boden, NPC_Begin und erstes NPC_Think je 200 ms, dann 50. Index k gilt fuer level.time
// 3900 + 50 k, der Wert ist die Hoehe ueber der Ruhelage (Boden + 24,125 im Spiel, Boden + 24 in der Vorschau).
// kSchwebenLeer: niemand steigt auf (log_c); kSchwebenRitt: ein Reiter steigt im Bild 4650 (k 15) auf, 550 ms
// (log_d).
struct SchwebeWert {
    std::size_t k;
    float h;
};
constexpr SchwebeWert kSchwebenLeer[] = {
    {0, 6.2844F}, {1, 12.4080F}, {2, 17.7220F}, {4, 23.9473F}, {5, 24.1619F}, {6, 23.0826F}, {8, 17.6824F},
    {10, 10.1931F}, {13, 2.3425F}, {15, 0.9609F}, {16, 1.2422F}, {17, 2.1535F}, {20, 7.3188F}, {25, 12.1289F},
    {26, 11.3085F}, {30, 7.2996F}, {35, 3.7203F}, {40, 6.8332F}, {50, 5.6213F}, {60, 8.4097F}, {70, 4.2792F}};
constexpr SchwebeWert kSchwebenRitt[] = {
    {13, 2.3425F}, {15, 0.2814F}, {16, 0.0000F}, {17, 1.0955F}, {20, 3.7104F}, {25, 1.7994F}, {26, 1.3898F},
    {30, 5.3098F}, {35, 11.8727F}, {40, 8.0331F}, {50, 6.1011F}, {60, 6.3361F}, {70, 7.7724F}};

void fahrzeuge() {
    std::printf("--- 4. Fahrzeuge: Schwerkraft nach der .veh ---\n");
    SchwerkraftHilfe h;
    h.endeMs = 3600.0;
    h.strahl = [](const std::array<float, 3>& von, const std::array<float, 3>& bis, double, std::array<float, 3>& treffer) {
        if (von[2] >= 0.0F && bis[2] <= 0.0F) {
            treffer = {von[0], von[1], 0.0F};
            return true;
        }
        return false;
    };
    const auto fahrzeug = [](const char* name, const char* typ, float hoehe, float kraft) {
        Actor a;
        a.name = name;
        a.npcType = name;
        a.haveStart = true;
        a.start[0] = 0.0F;
        a.start[1] = 0.0F;
        a.start[2] = 100.0F;
        a.ohneSchwerkraft = true;   // kein Fallen im Wegplaner
        a.fahrzeug.da = true;
        a.fahrzeug.typ = typ;
        a.fahrzeug.schwebeHoehe = hoehe;
        a.fahrzeug.schwebeKraft = kraft;
        a.fahrzeug.hang = 0.75F;
        a.fahrzeug.reibung = 1.5F;
        a.fahrzeug.beschleunigung = 20.0F;
        return a;
    };
    Scene sc;
    sc.actors.push_back(fahrzeug("atst_vehicle", "VH_WALKER", 0.0F, 0.0F));
    sc.actors.push_back(fahrzeug("swoop", "VH_SPEEDER", 30.0F, 35.0F));
    sc.actors.push_back(fahrzeug("x-wing", "VH_FIGHTER", 80.0F, 10.0F));
    sc.actors.push_back(fahrzeug("swoop", "VH_SPEEDER", 30.0F, 35.0F));
    // Gleich hoch ueber der Ruhelage wie im Spiel (61 - 0,125).
    sc.actors[1].start[2] = 60.875F;
    sc.actors[3].start[2] = 60.875F;
    sc.actors[3].aufsteigFenster.emplace_back(750.0, 1300.0);
    schwerkraftEinrechnen(sc, h);
    const auto hoeheBei = [](const Actor& a, std::size_t k) {
        return k < a.hoehe.size() ? a.hoehe[k] : a.start[2];
    };
    // hoehe[k] gilt fuer t = k * 50 ab dem Skriptanfang; davor zwei Spawn-Takte.
    pruefe(std::fabs(hoeheBei(sc.actors[0], 40U) - 24.0F) < 0.01F && std::fabs(hoeheBei(sc.actors[2], 40U) - 24.0F) < 0.01F,
           "Laeufer und Jaeger fallen auf den Boden (z " + std::to_string(hoeheBei(sc.actors[0], 40U)) + ", " +
               std::to_string(hoeheBei(sc.actors[2], 40U)) + ")");
    const auto vergleich = [&](const Actor& a, const auto& werte, const char* was) {
        bool gleich = true;
        std::string text;
        for (const SchwebeWert& w : werte) {
            const float v = hoeheBei(a, w.k) - 24.0F;
            gleich = gleich && std::fabs(v - w.h) < 0.01F;
            char z[64];
            std::snprintf(z, sizeof(z), " %zu:%.3f/%.3f", w.k, static_cast<double>(v), static_cast<double>(w.h));
            text += z;
        }
        pruefe(gleich, std::string(was) + " (k:Vorschau/Spiel ueber der Ruhelage)" + text);
    };
    vergleich(sc.actors[1], kSchwebenLeer, "Gleiter schwebt wie im Spiel");
    vergleich(sc.actors[3], kSchwebenRitt, "Gleiter beim Aufsteigen wie im Spiel");
}

// 2. trigger_hurt gegen den Spieler: stirbt er (player_die), steht
// cg.missionStatusDeadTime auf level.time + 1000 (g_combat.cpp:4044 OpenJK);
// ClientThink setzt stop_icarus, sobald pm_type PM_DEAD ist, keine Kamera
// laeuft und missionStatusDeadTime < level.time (g_active.cpp:4862-4865) -
// im Bild danach laeuft kein Skript mehr.
void spielerTod() {
    std::printf("--- 2. trigger_hurt toetet den Spieler ---\n");
    const Script s = lies("wait ( 5000.000 );\nprint ( \"spaet\" );\n");
    AblaufWelt w;
    w.hurtTode.push_back(AblaufWelt::HurtTod{500.0, "player", -1, false});
    const Ablauf a = simuliereAblauf(s, "", w);
    pruefe(std::fabs(a.skripteStoppMs - 1550.0) < 0.5,
           "Skripte halten im ersten Bild nach 500 + 1000 an (" + std::to_string(a.skripteStoppMs) + ")");
    const Script mitKamera = lies("camera ( /*@CAMERA_COMMANDS*/ ENABLE );\nwait ( 3000.000 );\n"
                                  "camera ( /*@CAMERA_COMMANDS*/ DISABLE );\nwait ( 5000.000 );\n");
    const Ablauf b = simuliereAblauf(mitKamera, "", w);
    pruefe(b.skripteStoppMs > 3000.0 && b.skripteStoppMs < 3200.0,
           "mit laufender Kamera erst nach DISABLE (" + std::to_string(b.skripteStoppMs) + ")");
}

// --- rc608: GeschuetzSim gegen Abschriften von pas_think, turret_base_think, G_RunMissile ------------------
//
// Eine Welt ohne Waende: nur der Kasten der Figur und (auf Wunsch) eine Flaeche mit surfaceFlags, die
// Bolzen an der Mitte des Modells mit POSITIVE_Y entlang der Gier.
struct GWelt {
    float zielLo[3]{};
    float zielHi[3]{};
    int zielNr = kSchussFigur;
    bool wand = false;
    float wandX = 0.0F;
    std::uint32_t wandFlags = 0;
};

GeschuetzWelt gWelt(const GWelt& g) {
    GeschuetzWelt w;
    w.spur = [g](double, const float start[3], const float mins[3], const float maxs[3], const float ende[3], int ignoriere, bool) {
        SchussSpur t;
        for (int k = 0; k < 3; ++k) { t.endpos[k] = ende[k]; }
        // Die Wand steht nur Geschossen im Weg (mit Kasten), die Sicht geht durch.
        if (g.wand && maxs[0] != 0.0F && (start[0] - g.wandX) * (ende[0] - g.wandX) < 0.0F) {
            const float f = (g.wandX - start[0]) / (ende[0] - start[0]);
            t.fraction = f;
            for (int k = 0; k < 3; ++k) { t.endpos[k] = start[k] + f * (ende[k] - start[k]); }
            t.normale[0] = start[0] < g.wandX ? -1.0F : 1.0F;
            t.flaechenFlags = g.wandFlags;
            t.entity = kSchussWelt;
        }
        if (ignoriere != g.zielNr) {
            const KastenSpur k = figurKastenSpur(start, ende, mins, maxs, g.zielLo, g.zielHi);
            if (k.fraction < t.fraction) {
                t.fraction = k.fraction;
                t.entity = g.zielNr;
                for (int q = 0; q < 3; ++q) {
                    t.endpos[q] = start[q] + k.fraction * (ende[q] - start[q]);
                    t.normale[q] = k.normale[q];
                }
            }
        }
        return t;
    };
    w.pvs = [](const float[3], const float[3]) { return true; };
    w.inhalt = [](const float[3], int) { return 0U; };
    w.bolzen = [](std::size_t, double, const GeschuetzKnochen& k, const char*, float m[3][4]) {
        const float gier = (k.winkel[1] + k.pasGier) * 3.14159265358979F / 180.0F;
        for (int r = 0; r < 3; ++r) { m[r][3] = k.ort[r]; }
        m[0][1] = std::cos(gier);
        m[1][1] = std::sin(gier);
        m[2][1] = 0.0F;
        return true;
    };
    return w;
}

SchussZiel gZiel(const float ort[3]) {
    SchussZiel z;
    z.nummer = kSchussFigur;
    z.team = kTeamFeind;
    for (int k = 0; k < 3; ++k) {
        z.ort[k] = ort[k];
        z.auge[k] = ort[k];
        z.lo[k] = ort[k] + (k == 2 ? -24.0F : -15.0F);
        z.hi[k] = ort[k] + (k == 2 ? 40.0F : 15.0F);
    }
    z.auge[2] += 36.0F;
    return z;
}

GWelt gWeltFuer(const SchussZiel& z) {
    GWelt g;
    for (int k = 0; k < 3; ++k) {
        g.zielLo[k] = z.lo[k];
        g.zielHi[k] = z.hi[k];
    }
    return g;
}

std::vector<SchadenAuftrag> gLauf(GeschuetzSim& sim, const std::vector<SchussZiel>& ziele, const GeschuetzWelt& w, double bis) {
    std::vector<SchadenAuftrag> a;
    for (double t = -200.0; t <= bis + 1e-6; t += 50.0) { sim.bild(t, ziele, w, a); }
    return a;
}

void geschuetzWache() {
    std::printf("--- rc608: misc_sentry_turret (pas_think) ---\n");
    GeschuetzDaten d;
    d.art = GeschuetzArt::Wache;
    d.nummer = 5;
    GeschuetzSim sim;
    sim.setze({d}, TodRegelwerk::OpenJK, -200.0, true);
    const float ort[3] = {0.0F, 200.0F, 0.0F};
    SchussZiel z = gZiel(ort);
    z.auge[2] = 40.0F;   // pas_think zielt auf eyePoint - 40: genau waagerecht
    const std::vector<SchadenAuftrag> a = gLauf(sim, {z}, gWelt(gWeltFuer(z)), 6000.0);
    const GeschuetzLauf& l = sim.laeufe().front();
    pruefe(!l.aufklappen.empty() && std::fabs(l.aufklappen.front().first - 800.0) < 0.5, "das erste pas_think klappt 1000 ms nach dem Spawnen auf");
    // Abschrift: speed = 0, ziel 90; je think (ab 800 + 1200, FRAMETIME) +10 bis auf 90.
    float speed = 0.0F;
    bool gierGleich = true;
    int thinks = 0;
    for (const GeschuetzStand& s : l.staende) {
        if (!s.gedacht || s.ms <= 800.0) { continue; }
        speed = winkelNorm360(speed);   // AngleNormalize360: auf 360/65536 abgeschnitten
        const float diff = winkelAbzug(speed, 90.0F);
        if (std::fabs(diff) > 0.25F) {
            if (std::fabs(diff) > 10.0F) {
                speed += diff > 0.0F ? -10.0F : 10.0F;
            } else {
                speed -= diff;
            }
        }
        gierGleich = gierGleich && std::fabs(winkelAbzug(s.pasGier, speed)) < 0.01F;
        ++thinks;
    }
    pruefe(gierGleich && thinks > 20, "bone_hinge wie die Abschrift von pas_think (+10 Grad je think bis zum Ziel, " + std::to_string(thinks) + " thinks)");
    pruefe(!l.schuesse.empty() && l.schuesse.front().ms >= 2900.0 - 1e-6 && l.schuesse.front().ms < 3300.0,
           "erster Schuss 900..1100 ms nach dem Finden (" + std::to_string(l.schuesse.empty() ? -1.0 : l.schuesse.front().ms) + ")");
    bool auftragGleich = !a.empty();
    for (const SchadenAuftrag& x : a) {
        auftragGleich = auftragGleich && !x.radius && x.ziel == kSchussFigur && x.schaden == 2 && x.dflags == kSchadenOhneRueckstoss && x.angreifer == 5;
    }
    pruefe(auftragGleich, "G_MissileImpacted: PAS_DAMAGE 2, DAMAGE_NO_KNOCKBACK, attacker das Geschuetz (" + std::to_string(a.size()) + " Treffer)");
    // CreateMissile mit 900: Flugzeit bis zum Kasten in ganzen Bildern von 50 ms (G_RunMissile).
    const Geschoss* g = sim.geschosse().empty() ? nullptr : &sim.geschosse().front();
    bool flug = false;
    if (g != nullptr && g->endeMs >= 0.0) {
        const float o[3] = {g->bahnen.front().basis[0], g->bahnen.front().basis[1], g->bahnen.front().basis[2]};
        const float dy = (z.lo[1] - 1.0F) - o[1];
        const double bilder = std::ceil(static_cast<double>(dy) / (900.0 * 0.05));
        flug = std::fabs((g->endeMs - g->abMs) - bilder * 50.0) < 50.5 && g->waffe == 1 && g->fleisch && g->einschlag;
    }
    pruefe(flug, "das Geschoss (WP_TURRET, 900) trifft nach der Flugzeit und zaehlt als Fleisch");
    bool startup = false;
    bool shutdown = false;
    for (const GeschuetzKlang& k : l.klaenge) {
        startup = startup || std::string(k.datei) == "sound/chars/turret/startup.wav";
        shutdown = shutdown || std::string(k.datei) == "sound/chars/turret/shutdown.wav";
    }
    pruefe(startup && !shutdown, "startup.wav beim Finden, solange der Feind steht kein shutdown.wav");

    // Der Feind faellt: pas_adjust_enemy gibt ihn frei, sobald bounceCount abgelaufen ist.
    GeschuetzSim sim2;
    sim2.setze({d}, TodRegelwerk::OpenJK, -200.0, true);
    std::vector<SchadenAuftrag> a2;
    const GeschuetzWelt w2 = gWelt(gWeltFuer(z));
    SchussZiel lebt = z;
    for (double t = -200.0; t <= 8000.0 + 1e-6; t += 50.0) {
        lebt.health = t < 4000.0 ? 100 : 0;
        sim2.bild(t, {lebt}, w2, a2);
    }
    const GeschuetzLauf& l2 = sim2.laeufe().front();
    pruefe(!l2.schuesse.empty() && l2.schuesse.back().ms <= 4000.0 + 650.0 + 100.0, "nach dem Tod hoechstens bounceCount (500 + 150 ms) weiter (letzter Schuss " +
           std::to_string(l2.schuesse.empty() ? -1.0 : l2.schuesse.back().ms) + ")");
    bool aus = false;
    for (const GeschuetzKlang& k : l2.klaenge) { aus = aus || std::string(k.datei) == "sound/chars/turret/shutdown.wav"; }
    pruefe(aus, "dann shutdown.wav");

    // Wer im Team des Geschuetzes ist oder FL_NOTARGET hat, wird nicht gefunden (pas_find_enemies).
    SchussZiel gleich = z;
    gleich.team = kTeamNeutral;
    SchussZiel versteckt = z;
    versteckt.notarget = true;
    for (const SchussZiel* x : {&gleich, &versteckt}) {
        GeschuetzSim s3;
        s3.setze({d}, TodRegelwerk::OpenJK, -200.0, true);
        (void)gLauf(s3, {*x}, gWelt(gWeltFuer(*x)), 5000.0);
        pruefe(s3.laeufe().front().schuesse.empty(), x == &gleich ? "noDamageTeam (Vorgabe TEAM_NEUTRAL): kein Ziel" : "FL_NOTARGET: kein Ziel");
    }
}

void geschuetzTurm() {
    std::printf("--- rc608: misc_turret (turret_base_think, turret_aim) ---\n");
    GeschuetzDaten d;
    d.art = GeschuetzArt::Turm;
    d.nummer = 9;
    d.team = "player";
    d.ursprung[2] = 30.0F;
    GeschuetzSim sim;
    sim.setze({d}, TodRegelwerk::OpenJK, -200.0, true);
    const float ort[3] = {-60.0F, 200.0F, 0.0F};
    const SchussZiel z = gZiel(ort);
    const std::vector<SchadenAuftrag> a = gLauf(sim, {z}, gWelt(gWeltFuer(z)), 6000.0);
    const GeschuetzLauf& l = sim.laeufe().front();
    pruefe(!l.schuesse.empty() && l.schuesse.front().ms >= 1700.0 - 1e-6 && l.schuesse.front().ms < 2000.0,
           "erster Schuss 1400 ms nach dem Finden im ersten think (spawn + 500) (" + std::to_string(l.schuesse.empty() ? -1.0 : l.schuesse.front().ms) + ")");
    float maxDelta = 0.0F;
    float maxNick = 0.0F;
    float nickAlt = 0.0F;
    for (const GeschuetzStand& s : l.staende) {
        if (!s.gedacht) { continue; }
        maxDelta = std::max(maxDelta, std::fabs(s.aposDelta[1]));
        maxNick = std::max(maxNick, std::fabs(winkelAbzug(s.nick, nickAlt)));
        nickAlt = s.nick;
    }
    pruefe(maxDelta > 0.0F && maxDelta <= 70.0F + 1e-3F, "turret_aim: s.apos hoechstens 14 Grad * 5 (" + std::to_string(maxDelta) + ")");
    pruefe(maxNick > 0.0F && maxNick <= 3.0F + 1e-3F, "turret_aim: Nick hoechstens 3 Grad je think (" + std::to_string(maxNick) + ")");
    bool gleich = !a.empty();
    for (const SchadenAuftrag& x : a) {
        gleich = gleich && !x.radius && x.schaden == 5 && x.dflags == (kSchadenOhneRueckstoss | kSchadenSchwereWaffe);
    }
    pruefe(gleich, "turret_fire: damage 5, DAMAGE_NO_KNOCKBACK | DAMAGE_HEAVY_WEAP_CLASS");
    double abstand = 1.0e9;
    for (std::size_t i = 1; i < l.schuesse.size(); ++i) { abstand = std::min(abstand, l.schuesse[i].ms - l.schuesse[i - 1].ms); }
    pruefe(abstand >= 150.0 - 1e-6, "wait: 150 + Zufall * 55 ms - im Takt der thinks (" + std::to_string(abstand) + ")");

    // Abschaltung (Start_Off): turret_base_use schaltet das Bild (s.frame 1 schwarz)
    GeschuetzDaten aus = d;
    aus.spawnflags = 1;
    aus.uses = {3000.0};
    GeschuetzSim sim2;
    sim2.setze({aus}, TodRegelwerk::OpenJK, -200.0, true);
    (void)gLauf(sim2, {z}, gWelt(gWeltFuer(z)), 6000.0);
    const GeschuetzLauf& l2 = sim2.laeufe().front();
    pruefe(!l2.schuesse.empty() && l2.schuesse.front().ms >= 3000.0 && l2.staende.front().bild == 1 && l2.staende.back().bild == 0,
           "Start_Off: schwarz und still bis zum use");

    // Regelwerk: Streuung von turret_fire - OpenJK mit random, Jedi Outcast nicht (kein Zufall in der Richtung).
    GeschuetzDaten mitStreuung = d;
    mitStreuung.random = 5.0F;
    const auto richtungen = [&](TodRegelwerk r) {
        GeschuetzSim s;
        s.setze({mitStreuung}, r, -200.0, true);
        (void)gLauf(s, {z}, gWelt(gWeltFuer(z)), 6000.0);
        std::vector<float> v;
        for (const GeschuetzSchuss& x : s.laeufe().front().schuesse) { v.push_back(x.richtung[0]); }
        return v;
    };
    const auto streut = [](const std::vector<float>& v) {
        // Die zweite Haelfte: das Geschuetz hat sich eingedreht.
        for (std::size_t i = v.size() / 2U + 1U; i < v.size(); ++i) {
            if (std::fabs(v[i] - v[i - 1]) > 0.02F) { return true; }
        }
        return false;
    };
    pruefe(streut(richtungen(TodRegelwerk::OpenJK)), "OpenJK: Streuung mit random");
    pruefe(!streut(richtungen(TodRegelwerk::JediOutcast)), "Jedi Outcast: keine Streuung");
    pruefe(trefferOrtFaktor(kOrtKopf, TodRegelwerk::OpenJK) == 2.0F && trefferOrtFaktor(kOrtKopf, TodRegelwerk::MovieDuels) == 1.0F &&
               trefferOrtFaktor(kOrtKopf, TodRegelwerk::JediOutcast) == 2.0F && trefferOrtFaktor(kOrtFussR, TodRegelwerk::OpenJK) == 0.25F,
           "damageModifier: Kopf 2.0 in OpenJK und Jedi Outcast, 1.0 in Movie Duels; Fuss 0.25");
}

void geschuetzGeschoss() {
    std::printf("--- rc608: G_RunMissile, G_MissileImpact ---\n");
    GeschuetzDaten d;
    d.art = GeschuetzArt::Turm;
    d.nummer = 3;
    d.team = "player";
    d.ursprung[0] = -100.0F;
    d.ursprung[2] = 50.0F;
    // Die Geschosse fliegen nach +x auf die Wand zu; dahinter steht die Figur.
    const GeschuetzDaten seitlich = d;
    GeschuetzSim sim;
    sim.setze({seitlich}, TodRegelwerk::OpenJK, -200.0, true);
    const float ortX[3] = {300.0F, 0.0F, 30.0F};
    SchussZiel zx = gZiel(ortX);
    GWelt gx = gWeltFuer(zx);
    gx.wand = true;
    gx.wandX = 100.0F;
    gx.wandFlags = kFlaecheOhneEinschlag;
    (void)gLauf(sim, {zx}, gWelt(gx), 6000.0);
    bool freiOhneEinschlag = false;
    for (const Geschoss& x : sim.geschosse()) { freiOhneEinschlag = freiOhneEinschlag || (x.endeMs >= 0.0 && !x.einschlag && !x.fleisch); }
    pruefe(freiOhneEinschlag, "SURF_NOIMPACT: \"never explode or bounce on sky\" - ohne Einschlag frei");
    gx.wandFlags = kFlaecheKraftfeld;
    GeschuetzSim sim2;
    sim2.setze({seitlich}, TodRegelwerk::OpenJK, -200.0, true);
    (void)gLauf(sim2, {zx}, gWelt(gx), 3000.0);
    bool abprall = false;
    for (const Geschoss& x : sim2.geschosse()) {
        if (x.bahnen.size() < 2U) { continue; }
        abprall = abprall || (x.besitzer == kSchussKeine && x.bahnen[1].delta[0] == -x.bahnen[0].delta[0] && x.bahnen[1].delta[1] == x.bahnen[0].delta[1]);
    }
    pruefe(abprall, "SURF_FORCEFIELD: G_BounceMissile spiegelt an der Ebene, der Besitzer ist danach frei");
    // Ohne Wand und Treffer: G_FreeEntity nach 10 s.
    GWelt leer;
    leer.zielLo[0] = leer.zielHi[0] = 1.0e6F;
    SchussZiel fern = zx;
    GeschuetzSim sim3;
    sim3.setze({seitlich}, TodRegelwerk::OpenJK, -200.0, true);
    (void)gLauf(sim3, {fern}, gWelt(leer), 14000.0);
    const Geschoss* ge = sim3.geschosse().empty() ? nullptr : &sim3.geschosse().front();
    pruefe(ge != nullptr && ge->endeMs >= 0.0 && std::fabs(ge->endeMs - ge->abMs - 10000.0) < 50.5 && !ge->einschlag, "ohne Treffer: frei nach der Lebenszeit 10000");
}

void geschuetzTod() {
    std::printf("--- rc608: G_Damage auf Geschuetze, turret_die, ObjectDie ---\n");
    GeschuetzDaten w;
    w.art = GeschuetzArt::Wache;
    w.nummer = 1;
    w.ursprung[2] = 10.0F;
    w.hatTarget = true;
    GeschuetzSim sim;
    sim.setze({w}, TodRegelwerk::OpenJK, -200.0, true);
    std::vector<SchadenAuftrag> folgen;
    const float punkt[3] = {0.0F, 0.0F, 20.0F};
    pruefe(sim.schaden(1, 1000.0, 49, 0, SchussMittel::Energie, kSchussFigur, punkt, folgen), "misc_sentry_turret nimmt Schaden (health 50)");
    const GeschuetzLauf& l = sim.laeufe().front();
    pruefe(l.totMs < 0.0 && l.effekte.size() == 1U && std::string(l.effekte[0].effekt) == "sparks/spark_exp_nosnd", "TurretPain: Funken, noch nicht tot");
    (void)sim.schaden(1, 1100.0, 1, 0, SchussMittel::Energie, kSchussFigur, punkt, folgen);
    pruefe(l.totMs == 1100.0 && l.freiMs == 1100.0, "turret_die -> ObjectDie: frei im selben Augenblick");
    pruefe(l.effekte.size() == 2U && std::string(l.effekte[1].effekt) == "turret/explode" && std::fabs(l.effekte[1].ort[2] - 22.0F) < 1e-3F && l.effekte[1].richtung[2] == 1.0F,
           "fxID turret/explode 12 Einheiten ueber dem Ursprung, nach oben (der spawnflags-2-Trick von SP_PAS)");
    pruefe(sim.benutzungen().size() == 1U && sim.benutzungen()[0].art == 0, "G_UseTargets(target)");
    pruefe(folgen.empty(), "PAS hat keinen Radiusschaden");
    float lo[3];
    float hi[3];
    pruefe(!sim.steht(1, 1200.0, lo, hi) && sim.steht(1, 1000.0, lo, hi), "die Spuren sehen es bis zur Freigabe");

    GeschuetzDaten t;
    t.art = GeschuetzArt::Turm;
    t.nummer = 2;
    t.hatTarget = true;
    GeschuetzSim sim2;
    sim2.setze({t}, TodRegelwerk::OpenJK, -200.0, true);
    std::vector<SchadenAuftrag> f2;
    (void)sim2.schaden(2, 500.0, 100, 0, SchussMittel::Energie, kSchussFigur, punkt, f2);
    const GeschuetzLauf& l2 = sim2.laeufe().front();
    pruefe(l2.totMs == 500.0 && l2.freiMs < 0.0 && l2.staende.back().bild == 1, "misc_turret: das Schadensmodell bleibt (modelindex2), s.frame 1");
    pruefe(f2.size() == 1U && f2[0].radius && f2[0].schaden == 10 && f2[0].weite == 25.0F && f2[0].ignoriere == kSchussFigur,
           "G_RadiusDamage(currentOrigin, attacker, 10, 25, attacker, MOD_UNKNOWN)");

    GeschuetzDaten g2;
    g2.art = GeschuetzArt::TurmG2;
    g2.nummer = 4;
    g2.spawnflags = 4;   // CANRESPAWN
    g2.count = 3000;
    GeschuetzSim sim3;
    sim3.setze({g2}, TodRegelwerk::MovieDuels, -200.0, true);
    std::vector<SchadenAuftrag> f3;
    for (double b = -200.0; b <= 400.0; b += 50.0) { sim3.bild(b, {}, GeschuetzWelt{}, f3); }
    (void)sim3.schaden(4, 400.0, 100, 0, SchussMittel::Energie, kSchussFigur, punkt, f3);
    for (double b = 450.0; b <= 4000.0; b += 50.0) { sim3.bild(b, {}, GeschuetzWelt{}, f3); }
    const GeschuetzLauf& l3 = sim3.laeufe().front();
    pruefe(l3.wiederMs.size() == 1U && std::fabs(l3.wiederMs[0] - 3400.0) < 50.5, "misc_turretG2 CANRESPAWN: wieder nach count ms (Movie Duels)");
}

// rc610 F46 B4-1: Movie Duels spielt beim Turbolaser (misc_turretG2 TURBO) weder shutdown noch ping noch startup
// (g_turret.cpp MD 584, 634, 752); OpenJK sucht mit dem Turbo keine Feinde (dort bleibt alles still), der Turm ohne
// Turbo spielt alle drei. B4-2: mit mp_rules ist ein Fahrzeug mit Pilot ein Ziel (Team des Piloten), eines ohne nicht,
// ein Reiter in einem Fahrzeug mit hideRider auch nicht (g_turret.cpp MD 681-704, turret_target_team 201-208).
void geschuetzF46() {
    std::printf("--- rc610 F46: misc_turretG2 Turbo-Klaenge, Fahrzeuge als Ziel (Movie Duels) ---\n");
    const float ort[3] = {-60.0F, 200.0F, 0.0F};
    const SchussZiel z = gZiel(ort);
    const auto lauf = [&](int spawnflags, TodRegelwerk r, const SchussZiel& ziel, double weg) {
        GeschuetzDaten d;
        d.art = GeschuetzArt::TurmG2;
        d.nummer = 4;
        d.team = "player";
        d.spawnflags = spawnflags;
        d.ursprung[2] = 30.0F;
        auto sim = std::make_shared<GeschuetzSim>();
        sim->setze({d}, r, -200.0, true);
        const GeschuetzWelt w = gWelt(gWeltFuer(ziel));
        std::vector<SchadenAuftrag> a;
        for (double t = -200.0; t <= 12000.0 + 1e-6; t += 50.0) {
            if (t < weg) {
                sim->bild(t, {ziel}, w, a);
            } else {
                sim->bild(t, {}, w, a);
            }
        }
        return sim;
    };
    const auto klaenge = [](const GeschuetzLauf& l) {
        std::string k;
        for (const char* n : {"startup", "shutdown", "ping"}) {
            const std::string pfad = std::string("sound/chars/turret/") + n + ".wav";
            const bool da = std::any_of(l.klaenge.begin(), l.klaenge.end(), [&](const GeschuetzKlang& x) { return pfad == x.datei; });
            if (da) { k += (k.empty() ? "" : ",") + std::string(n); }
        }
        return k;
    };
    {
        SchussZiel spieler = z;   // der Turbo nimmt noDamageTeam TEAM_ENEMY (ohne alliedTeam/teamnodmg)
        spieler.team = kTeamSpieler;
        const auto md = lauf(8, TodRegelwerk::MovieDuels, spieler, 3000.0);
        const GeschuetzLauf& l = md->laeufe().front();
        pruefe(!l.schuesse.empty() && klaenge(l).empty(), "MD Turbo: feuert, aber kein startup, shutdown, ping (" + klaenge(l) + ")");
        const auto mdTurm = lauf(0, TodRegelwerk::MovieDuels, z, 3000.0);
        pruefe(klaenge(mdTurm->laeufe().front()) == "startup,shutdown,ping", "MD ohne Turbo: startup, shutdown, ping (" + klaenge(mdTurm->laeufe().front()) + ")");
        const auto oj = lauf(0, TodRegelwerk::OpenJK, z, 3000.0);
        pruefe(klaenge(oj->laeufe().front()) == "startup,shutdown,ping", "OpenJK ohne Turbo: startup, shutdown, ping (" + klaenge(oj->laeufe().front()) + ")");
        const auto ojTurbo = lauf(8, TodRegelwerk::OpenJK, spieler, 3000.0);
        pruefe(ojTurbo->laeufe().front().schuesse.empty() && klaenge(ojTurbo->laeufe().front()).empty(), "OpenJK Turbo: sucht keine Feinde, still");
    }
    {
        SchussZiel f = z;   // ein Fahrzeug (eigenes Team frei), der Pilot ist ein Feind
        f.fahrzeug = true;
        f.team = kTeamFrei;
        f.hatPilot = true;
        f.pilotTeam = kTeamFeind;
        pruefe(!lauf(0, TodRegelwerk::MovieDuels, f, 1.0e9)->laeufe().front().schuesse.empty(), "MD: Fahrzeug mit feindlichem Pilot ist ein Ziel");
        SchussZiel ohne = f;
        ohne.hatPilot = false;
        ohne.team = kTeamFeind;
        pruefe(lauf(0, TodRegelwerk::MovieDuels, ohne, 1.0e9)->laeufe().front().schuesse.empty(), "MD: Fahrzeug ohne Pilot ist kein Ziel");
        SchussZiel freund = f;
        freund.pilotTeam = kTeamSpieler;
        freund.team = kTeamFeind;
        pruefe(lauf(0, TodRegelwerk::MovieDuels, freund, 1.0e9)->laeufe().front().schuesse.empty(), "MD: Fahrzeug zaehlt mit dem Team des Piloten (noDamageTeam)");
        SchussZiel versteckt = f;
        versteckt.pilotNotarget = true;
        pruefe(lauf(0, TodRegelwerk::MovieDuels, versteckt, 1.0e9)->laeufe().front().schuesse.empty(), "MD: Pilot mit FL_NOTARGET - kein Ziel");
        SchussZiel reiter = z;
        reiter.reiterVerborgen = true;
        pruefe(lauf(0, TodRegelwerk::MovieDuels, reiter, 1.0e9)->laeufe().front().schuesse.empty(), "MD: Reiter in einem Fahrzeug mit hideRider ist kein Ziel");
        pruefe(!lauf(0, TodRegelwerk::OpenJK, reiter, 1.0e9)->laeufe().front().schuesse.empty(), "OpenJK: kein Fahrzeugfall, der Reiter bleibt ein Ziel");
    }
}

void g2Strahl() {
    std::printf("--- rc608: G2_SegmentTriangleTest (Spur ohne Kasten) ---\n");
    const std::vector<float> ecken = {0.0F, -10.0F, -10.0F, 0.0F, 10.0F, -10.0F, 0.0F, 0.0F, 10.0F};
    const std::vector<std::uint32_t> idx = {0U, 1U, 2U};
    const float start[3] = {-20.0F, 0.0F, 0.0F};
    const float ende[3] = {20.0F, 0.0F, 0.0F};
    G2Kollision vorlage;
    vorlage.flaeche = 3;
    std::vector<G2Kollision> aus;
    const int n = g2StrahlKollisionen(ecken, idx, start, ende, vorlage, aus);
    pruefe(n == 1 && std::fabs(aus[0].abstand - 20.0F) < 1e-3F && std::fabs(aus[0].punkt[0]) < 1e-3F && aus[0].flaeche == 3 && std::fabs(std::fabs(aus[0].normale[0]) - 1.0F) < 1e-3F,
           "Treffer mit Punkt, Abstand und Normale");
    const float neben[3] = {-20.0F, 50.0F, 0.0F};
    const float neben2[3] = {20.0F, 50.0F, 0.0F};
    aus.clear();
    pruefe(g2StrahlKollisionen(ecken, idx, neben, neben2, vorlage, aus) == 0, "daneben");
    const float kurz[3] = {-10.0F, 0.0F, 0.0F};
    aus.clear();
    pruefe(g2StrahlKollisionen(ecken, idx, start, kurz, vorlage, aus) == 0, "die Strecke endet vor dem Dreieck");
    aus.clear();
    (void)g2RadiusKollisionen(ecken, idx, start, ende, 2.0F, vorlage, aus);
    pruefe(aus.size() == 1U && std::fabs(std::fabs(aus[0].normale[0]) - 1.0F) < 1e-3F, "G2_RadiusTracePolys: die Normale kommt mit");
}

}  // namespace

int main() {
    kameraAccelZoom();
    schwertFarben();
    bruchVerzoegerung();
    entfernenVorAdd();
    teilchenSpur();
    liegendeSchwerter();
    objektWurf();
    fahrzeuge();
    spielerTod();
    geschuetzWache();
    geschuetzTurm();
    geschuetzGeschoss();
    geschuetzTod();
    geschuetzF46();
    g2Strahl();
    std::printf("%d Fehler\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
