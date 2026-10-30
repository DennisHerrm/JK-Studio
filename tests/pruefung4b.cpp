// pruefung4b.cpp - Pruefung 4b: Entities, Effekte, Kamera - je Punkt gegen die
// Formel der Engine (OpenJK, Movie Duels, Jedi Outcast) mit eigenen Daten.
//
// 6. CAMERA_ACCEL und ZOOM gleichzeitig: client_camera (FOV, FOV2, FOV_time,
//    FOV_duration, FOV_vel, FOV_acc, CAMERA_ACCEL/CAMERA_ZOOMING) Bild fuer
//    Bild wie CGCam_Update (cg_camera.cpp:1101-1183 OpenJK) gegen
//    CameraTrack::at.
// 3. forced_rgb_saber_colours (Movie Duels): die RGB-Zweige sind hinter
//    "saberColor"/"saber2Color" unerreichbar - eine RGB-Zeile aendert nichts;
//    Jedi Outcast kennt nur "saberColor".
// 5. Zweites use in der Bruchverzoegerung: funcBBrushUse -> funcBBrushDie
//    stellt nextthink neu (und der use-Skript laeuft erneut); ein use in den
//    50 ms vor G_FreeEntity bricht neu (g_breakable.cpp:81-215 OpenJK).
#include "bhed/ablauf.h"
#include "bhed/bspgeo.h"
#include "bhed/camtrack.h"
#include "bhed/mover.h"
#include "bhed/roff.h"
#include "bhed/scene.h"
#include "bhed/script.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
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
    MapData m;
    m.entities = {ent("func_breakable", {{"targetname", "fels"}, {"target", "danach"}, {"delay", "2"}, {"model", "*1"},
                                         {"usescript", "x/ruf"}}),
                  ent("func_breakable", {{"targetname", "glas"}, {"target", "danach2"}, {"model", "*2"}}),
                  ent("target_relay", {{"targetname", "danach"}}), ent("target_relay", {{"targetname", "danach2"}})};
    auto ruf = std::make_shared<Script>(lies("print ( \"ruf\" );\n"));
    AblaufWelt w;
    w.karte = &m;
    w.skript = [ruf](const std::string& p) -> const Script* { return p == "x/ruf" ? ruf.get() : nullptr; };
    // fels: use bei 0 und bei 1000 (delay 2 s) -> Bruch bei 3000, nicht 2000.
    // glas: use bei 0 (Bruch sofort, G_FreeEntity bei 50) und bei 20 -> bricht
    // neu (Ziele zweimal), danach bei 100 nichts mehr.
    const Script s = lies("use ( \"fels\" );\nuse ( \"glas\" );\nwait ( 20.000 );\nuse ( \"glas\" );\n"
                          "wait ( 80.000 );\nuse ( \"glas\" );\nwait ( 900.000 );\nuse ( \"fels\" );\nwait ( 4000.000 );\n");
    const Ablauf a = simuliereAblauf(s, "", w);
    std::vector<double> danach;
    std::vector<double> danach2;
    int rufe = 0;
    for (const Ablauf::Benutzung& b : a.benutzt) {
        if (b.name == "danach") { danach.push_back(b.ms); }
        if (b.name == "danach2") { danach2.push_back(b.ms); }
    }
    const std::function<void(const std::vector<Node>&)> zaehle = [&](const std::vector<Node>& ns) {
        for (const Node& n : ns) {
            if (n.name == "print" && !n.args.empty() && n.args[0].text == "ruf") { ++rufe; }
            zaehle(n.children);
        }
    };
    zaehle(a.flach.nodes);
    std::string zeiten;
    for (const double t : danach) { zeiten += " " + std::to_string(static_cast<int>(t)); }
    pruefe(danach.size() == 1U && std::fabs(danach[0] - 3000.0) < 0.5,
           "Nachbau: zweites use bei 1000 schiebt den Bruch auf 3000 (Ziele bei" + zeiten + ")");
    pruefe(rufe == 2, "Nachbau: der use-Skript laeuft bei jedem use (" + std::to_string(rufe) + " mal)");
    std::string z2;
    for (const double t : danach2) { z2 += " " + std::to_string(static_cast<int>(t)); }
    pruefe(danach2.size() == 2U && std::fabs(danach2[0]) < 0.5 && std::fabs(danach2[1] - 20.0) < 0.5,
           "Nachbau: use 20 ms nach dem Bruch (vor G_FreeEntity) bricht neu, bei 100 nichts (Ziele bei" + z2 + ")");
    BspGeometry g;
    g.models.resize(3);
    for (int i = 1; i <= 2; ++i) {
        auto& mo = g.models[static_cast<std::size_t>(i)];
        mo.maxs[0] = 8;
        mo.maxs[1] = 64;
        mo.maxs[2] = 128;
    }
    MoverSim sim;
    sim.baue(m, g, {{0.0, "fels"}, {0.0, "glas"}, {20.0, "glas"}, {100.0, "glas"}, {1000.0, "fels"}}, nullptr, 6000.0);
    int felsBrueche = 0;
    int glasBrueche = 0;
    double felsBei = -1.0;
    std::string gz;
    for (const Bruch& b : sim.brueche()) {
        if (b.entity == 0) {
            ++felsBrueche;
            felsBei = b.ms;
        }
        if (b.entity == 1) {
            ++glasBrueche;
            gz += " " + std::to_string(static_cast<int>(b.ms));
        }
    }
    pruefe(felsBrueche == 1 && std::fabs(felsBei - 3000.0) < 0.5 && sim.at(1, 3000).sichtbar && !sim.at(1, 3050).sichtbar,
           "MoverSim: ein Bruch bei 3000, der Brush bis 3050 (" + std::to_string(static_cast<int>(felsBei)) + ")");
    pruefe(glasBrueche == 2 && sim.at(2, 60).sichtbar && !sim.at(2, 70).sichtbar,
           "MoverSim: glas bricht bei 0 und 20, weg ab 70 (Brueche bei" + gz + ")");
}

}  // namespace

int main() {
    kameraAccelZoom();
    schwertFarben();
    bruchVerzoegerung();
    std::printf("%d Fehler\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
