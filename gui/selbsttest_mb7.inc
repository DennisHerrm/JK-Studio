// selbsttest_mb7.inc - Modell-Block Stufe 7: Figuren im Mission-Tab animieren und fuer das Spiel bauen
// (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=mb7)
//
//   Aufbau          eine neue Mission mb7szene (Grundkarte BHED_MB7_BSP von der Platte, sonst md_am_sith aus den Archiven),
//                   drei NPC_spawner mb7_a/b/c, ein Skript mit Kamera von oben; eine vorhandene Karten-GLA
//                   _humanoid_mb7szene mit BOTH_CIN_1 in einem eigenen Spielordner (Zusammenfuehren)
//   Erreichbarkeit  Inspektor der Figur (Knopf), Rechtsklick der Entity, Menue des Fensters, Aktionssuche (X), Rechtsklick
//                   auf einen Griff, Tooltip mit Taste an jeder Aktion
//   eine Figur      Animieren legt den Block an (BOTH_CIN_2, BOTH_CIN_1 der vorhandenen Karten-GLA bleibt), die Vorschau spielt
//                   den Platz aus der gebackenen Karten-GLA; W am Becken (Maus) = Wurzelweg, SET_ORIGIN = Start + Weg;
//                   W an der Hand = IK-Ziel (Hand am Ziel); E am Kopf = FK-Schluessel; Fuss auf den Boden; Ziel auf eine Flaeche
//                   unter der Maus; ein Rueckgaengig-Schritt je Zug
//   SET_ORIGIN      mit: nach der Folgeanimation steht das Becken, wo es am Ende stand; ohne (Rot-Probe aus Stufe 0): die Figur
//                   springt um den Weg zurueck
//   mehrere Figuren b wartet auf das Signal von a (Start = Ende von a), c haelt mit der linken Hand die rechte Hand von b
//                   (fremder Raum, gebacken in die GLA von c)
//   Dopesheet       IK-Zeile da; eine IK-Raute ziehen verschiebt sie (FK-Schluessel der Kette mit)
//   wie Blender     G an der Hand (modal), Klick uebernimmt; Esc bricht ab
//   Bauen           GLA, animation.cfg, Skript (.txt/.IBI) im Ausgabeordner; Pruefungen ohne Befund; zweimal gebaut ->
//                   Byte fuer Byte gleich; die GLA der Vorschau = die gebaute Datei
//   mb7spiel        BHED_MODELL_NUR=mb7spiel: dieselbe Szene, gebaut nach BHED_FIGUR_BAU (mit, ohne SET_ORIGIN, eng: c 28
//                   Einheiten neben b), Mission als .pk3, dazu die Lage der Becken der Vorschau je 250 ms (mb7_vorschau.txt)
//                   und Fotos durch die Skriptkamera je 500 ms im Spielfenster 1920x1440 fuer den Vergleich im Spiel
//                   (Laufskripte mb7/spiel.py, orig_spiel.py, vergleich2.py); Schatten unter Bolzen 0; eng: c kommt spaet
//   mb7textur       BHED_MODELL_NUR=mb7textur: eine Karte ohne Texturen zeigt tr.defaultShader
// Eingebunden in selbsttest.cpp (nach selbsttest_mb6.inc).
namespace fa7 {

struct Zustand {
    std::string fehler;
    std::string bsp;           // Grundkarte von der Platte (BHED_MB7_BSP)
    float spieler[3]{};        // info_player_start
    float boden = 0.0F;
    std::string typ;           // NPC_type
    std::string skelett;       // animFile des Modells
    std::string ordner;        // Ausgabeordner
    std::size_t tiefe = 0;
    std::vector<std::string> zeilen;
    std::array<float, 3> handZiel{};
    std::array<float, 3> ursprung{};
    std::string gla1;
    float wegVorher = 0.0F;
    bool bereit = false;
};

std::string v3(const float* v) {
    char b[96];
    std::snprintf(b, sizeof b, "%.3f %.3f %.3f", static_cast<double>(v[0]), static_cast<double>(v[1]), static_cast<double>(v[2]));
    return b;
}

int figur(const std::string& n) {
    for (std::size_t i = 0; i < g_app->scene.actors.size(); ++i) {
        if (g_app->scene.actors[i].name == n) { return static_cast<int>(i); }
    }
    return -1;
}

// Die Knochen der Figur so, wie die Vorschau sie zeichnet (figurPose: Bilder der gebackenen Karten-GLA)
bool gezeichnet(const std::string& n, double ms, std::vector<BoneMatrix>& welt, float fw[16], const GlaAnimation** anim = nullptr) {
    const int ai = figur(n);
    if (ai < 0 || static_cast<std::size_t>(ai) >= g_app->actorAssets.size() || !g_app->actorAssets[static_cast<std::size_t>(ai)]) { return false; }
    const Actor& a = g_app->scene.actors[static_cast<std::size_t>(ai)];
    ActorDraw d;
    d.figur = ai;
    if (!figurPose(a, *g_app->actorAssets[static_cast<std::size_t>(ai)], a.at(ms), ms, d, welt, fw)) { return false; }
    if (anim != nullptr) { *anim = d.anim; }
    return true;
}

std::array<float, 3> knochenInKarte(const std::vector<BoneMatrix>& w, const float fw[16], const GlaAnimation& g, const char* name) {
    const int k = boneIndex(g, name);
    std::array<float, 3> o{};
    if (k < 0 || static_cast<std::size_t>(k) >= w.size()) { return o; }
    const BoneMatrix& m = w[static_cast<std::size_t>(k)];
    for (int r = 0; r < 3; ++r) { o[static_cast<std::size_t>(r)] = fw[r * 4] * m.m[0][3] + fw[r * 4 + 1] * m.m[1][3] + fw[r * 4 + 2] * m.m[2][3] + fw[r * 4 + 3]; }
    return o;
}

float ab(const std::array<float, 3>& a, const std::array<float, 3>& b) { return std::hypot(std::hypot(a[0] - b[0], a[1] - b[1]), a[2] - b[2]); }
float ab2(const std::array<float, 3>& a, const std::array<float, 3>& b) { return std::hypot(a[0] - b[0], a[1] - b[1]); }

// Stufe 6: das Fenster "Figur animieren" fuer ein Foto ganz ins Hauptfenster (die Handbuchbilder ph_hb7_* zeigten es nicht:
// es lag ausserhalb des Hauptfensters, mit Viewports ein eigenes Fenster) - meldet die Lage vorher
Schritt fensterImBild() {
    return tu("Fenster Figur animieren ganz im Hauptfenster", [] {
        ImGuiWindow* w = ImGui::FindWindowByName("###figuranim");
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        if (w == nullptr) {
            diag::detail("Knopftest: Fenster Figur animieren - nicht da");
            return;
        }
        const bool innen = w->Viewport == vp && w->Pos.x >= vp->WorkPos.x && w->Pos.y >= vp->WorkPos.y &&
                           w->Pos.x + w->Size.x <= vp->WorkPos.x + vp->WorkSize.x && w->Pos.y + w->Size.y <= vp->WorkPos.y + vp->WorkSize.y;
        diag::detail("Knopftest: Fenster Figur animieren - Lage " + std::to_string(w->Pos.x) + " " + std::to_string(w->Pos.y) + ", Groesse " +
                     std::to_string(w->Size.x) + " " + std::to_string(w->Size.y) + ", Hauptfenster " + std::to_string(vp->WorkPos.x) + " " +
                     std::to_string(vp->WorkPos.y) + " " + std::to_string(vp->WorkSize.x) + " " + std::to_string(vp->WorkSize.y) + ", eigenes Fenster " +
                     std::to_string(w->Viewport != vp) + ", ganz innen " + std::to_string(innen));
        if (innen) { return; }
        const float fs = ImGui::GetFontSize();
        const ImVec2 groesse{std::min(w->Size.x, vp->WorkSize.x - 2.0F * fs), std::min(w->Size.y, vp->WorkSize.y - 5.0F * fs)};
        ImGui::SetWindowSize(w, groesse);
        ImGui::SetWindowPos(w, ImVec2{vp->WorkPos.x + vp->WorkSize.x - groesse.x - fs, vp->WorkPos.y + 4.0F * fs});
    });
}

std::vector<std::string> zeilen(const std::string& f) { return modell::figurZeilenImSkript(g_app->doc.script(), f); }

// SET_ORIGIN im Block der Figur
bool setOrigin(const std::string& f, std::array<float, 3>& o) {
    const Script& s = g_app->doc.script();
    const auto i = modell::findeFigurBlock(s, f);
    if (!i) { return false; }
    for (std::size_t j = *i; j < s.nodes.size(); ++j) {
        if (s.nodes[j].kind != Node::Kind::Command || s.nodes[j].name != "affect") { continue; }
        for (const Node& c : s.nodes[j].children) {
            if (c.name == "set" && c.args.size() >= 2 && c.args[0].text == "SET_ORIGIN") {
                return std::sscanf(c.args[1].text.c_str(), "%f %f %f", &o[0], &o[1], &o[2]) == 3;
            }
        }
        return false;
    }
    return false;
}

const modell::FigurAnim* animVon(const std::string& f, modell::FigurAnim& puffer) {
    const std::vector<std::string> z = zeilen(f);
    GlaAnimation leer;
    if (z.empty() || !modell::figurAusZeilen({z.front()}, leer, modell::Rig{}, puffer)) { return nullptr; }
    return &puffer;
}

ImVec2 griffOrt(const std::string& name) {
    ImVec2 p{-1.0F, -1.0F};
    (void)figurAnimGriffSchirm(figurAnimGriffSuchen(name), p);
    return p;
}

Schritt zeitAuf(const std::string& f, int bild) {
    return tu("MB7: Zeit auf Bild " + std::to_string(bild) + " von " + f, [=] {
        modell::FigurAnim a;
        const double lerp = animVon(f, a) != nullptr ? modell::figurBildMs(a) : 50.0;
        g_app->playMs = figurAnimStartMs(f) + static_cast<double>(bild) * lerp;
        g_app->playing = false;
        g_app->mapDirty = true;
    });
}

// Die Szene: Mission, Figuren, Skript, vorhandene Karten-GLA
void aufbau(std::vector<Schritt>& s, std::shared_ptr<Zustand> z) {
    const auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("MB7: Ausgangslage (Laufordner, Missionen, Grundkarte)", [=] {
        const std::string wurzel = mb::verbinde(mb::laufOrdner(), "mb7_missionen");
        _putenv_s("BHED_MISSIONEN", wurzel.c_str());
        if (const char* b = std::getenv("BHED_MB7_BSP"); b != nullptr && b[0] != '\0') { z->bsp = b; }
        z->ordner = std::getenv("BHED_FIGUR_BAU") != nullptr ? std::getenv("BHED_FIGUR_BAU") : mb::verbinde(mb::laufOrdner(), "mb7_bau");
        g_app->leftMode = 1;
        g_app->missionPickOpen = false;
        g_app->settings.modellBedienung = 0;   // wie 3ds Max zuerst
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        diag::detail("Knopftest: MB7 - Grundkarte " + (z->bsp.empty() ? std::string("maps/md_am_sith.bsp") : z->bsp) + ", Ausgabe " + z->ordner);
    }));
    add(tu("MB7: Neue Mission mb7szene", [=] {
        std::string f;
        if (!neueMissionAnlegen(z->bsp.empty() ? std::string("maps/md_am_sith.bsp") : z->bsp, "mb7szene", "mb7szene", &f)) { z->fehler += " Mission: " + f; }
    }));
    add(warteBis("MB7: Karte mb7szene geladen", [] { return g_app->map.path == "maps/mb7szene.bsp" && !g_app->geo.empty(); }, 4000));
    add(tu("MB7: Spielerstart, NPC-Typ mit _humanoid, drei Spawner, Skript mit Kamera von oben", [=] {
        for (const MapEntity& e : g_app->map.entities) {
            if (e.classname == "info_player_start") { (void)entityOrt(e, z->spieler); }
        }
        // ein Typ, dessen Modell das Standard-Skelett nimmt (Karten-GLA nur fuer _humanoid, NPC_stats.cpp)
        for (const char* t : {"kyle", "Kyle", "luke", "Luke", "jan", "Jan"}) {
            const auto it = g_app->npcMap.find(t);
            if (it != g_app->npcMap.end() && z->typ.empty()) { z->typ = t; }
        }
        if (z->typ.empty()) { z->typ = "kyle"; }
        const TraceTreffer t = traceRay(g_app->geo, z->spieler, std::array<float, 3>{z->spieler[0], z->spieler[1], z->spieler[2] - 512.0F}.data());
        z->boden = t.hit ? t.point[2] : z->spieler[2] - 24.0F;
        const char* namen[3] = {"mb7_a", "mb7_b", "mb7_c"};
        const float versatz[3][2] = {{96.0F, -96.0F}, {96.0F, 32.0F}, {152.0F, 32.0F}};
        const char* gier[3] = {"0", "0", "180"};
        for (int i = 0; i < 3; ++i) {
            const float o[3] = {z->spieler[0] + versatz[i][0], z->spieler[1] + versatz[i][1], z->boden + 24.0F};
            const int e = neueEntityAn("NPC_spawner", o);
            if (e < 0) {
                z->fehler += " Spawner";
                return;
            }
            (void)entityKeySetzen(e, "NPC_type", z->typ);
            (void)entityKeySetzen(e, "NPC_targetname", namen[i]);
            (void)entityKeySetzen(e, "angle", gier[i]);
        }
        const float kam[3] = {z->spieler[0] + 128.0F, z->spieler[1] - 16.0F, z->boden + 240.0F};
        const std::string text = std::string("//Generated by BehavEd\n\ncamera ( ENABLE );\ncamera ( ZOOM, 110.000, 0.000 );\ncamera ( MOVE, < ") + v3(kam) +
                                 " >, 0.000 );\ncamera ( PAN, < 90.000 0.000 0.000 >, < 0.000 0.000 0.000 >, 0.000 );\nwait ( 16000.000 );\ncamera ( DISABLE );\n";
        Script neu;
        std::vector<Diag> d;
        if (!readScript(text, neu, d)) {
            z->fehler += " Skript";
            return;
        }
        g_app->doc.ersetzeSkript(std::move(neu), "mb7szene");
        rebuildTree();
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
        z->bereit = true;
    }));
    add(warteBis("MB7: Szene mit drei Figuren", [=] {
        return !z->bereit || (g_app->camTrackValid && figur("mb7_a") >= 0 && figur("mb7_b") >= 0 && figur("mb7_c") >= 0 && jk2probe::figurenGeladen());
    }, 3000));
    add(tu("MB7: vorhandene Karten-GLA _humanoid_mb7szene (BOTH_CIN_1 = BOTH_STAND1) in einem eigenen Spielordner", [=] {
        const int ai = figur("mb7_a");
        if (ai < 0 || !g_app->actorAssets[static_cast<std::size_t>(ai)]) {
            z->fehler += " Figur a";
            return;
        }
        z->skelett = g_app->actorAssets[static_cast<std::size_t>(ai)]->model.animFile;
        std::string gla;
        std::string cfg;
        const std::string ordner = z->skelett.substr(0, z->skelett.find_last_of('/'));
        bool da = readFromArchives(z->skelett + ".gla", gla) && (readFromArchives(z->skelett + ".cfg", cfg) || readFromArchives(ordner + "/animation.cfg", cfg));
        std::string neu;
        for (const AnimEntry& e : parseAnimationCfg(cfg)) {
            if (e.name == "BOTH_STAND1") { neu = "BOTH_CIN_1\t" + std::to_string(e.firstFrame) + "\t" + std::to_string(e.numFrames) + "\t-1\t" + std::to_string(e.fps) + "\n"; }
        }
        // Name im Kopf = Ordner (mdxaHeader_t.name, kartenGlaPruefen)
        const std::string kopf = "models/players/_humanoid_mb7szene/_humanoid_mb7szene";
        if (da && gla.size() > 72U) {
            std::fill(gla.begin() + 8, gla.begin() + 72, '\0');
            std::copy(kopf.begin(), kopf.end(), gla.begin() + 8);
        }
        da = da && !neu.empty();
        const std::string spiel = mb::verbinde(mb::laufOrdner(), "mb7spiel");
        std::error_code ec;
        std::filesystem::create_directories(::bhed::utf8Pfad(spiel), ec);
        std::string f;
        if (da && writePk3(mb::verbinde(spiel, "zz_mb7szene.pk3"), {{kopf + ".gla", gla}, {"models/players/_humanoid_mb7szene/animation.cfg", neu}}, &f)) {
            g_app->settings.gamePaths.push_back(spiel);
            rescanGamePaths();
            refreshPk3List();
            g_app->skeletons.erase(kopf);
            g_app->camTrackValid = false;
        } else {
            z->fehler += " Karten-GLA: " + f;
        }
    }));
    add(warteBis("MB7: Szene neu mit der vorhandenen Karten-GLA", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 3000));
    add(pruefe("MB7: Aufbau - drei Figuren mit Ghoul2-Skelett, Karten-GLA mit BOTH_CIN_1", [=] {
        prepareScene();
        const auto it = g_app->skeletons.find("models/players/_humanoid_mb7szene/_humanoid_mb7szene");
        const bool kino = it != g_app->skeletons.end() && !it->second.anim.empty() && !it->second.sections.empty();
        diag::detail("Knopftest: MB7 Aufbau - Typ " + z->typ + ", Skelett " + z->skelett + ", Spieler " + v3(z->spieler) + ", Boden " + std::to_string(z->boden) +
                     ", Karten-GLA " + std::to_string(kino) + (z->fehler.empty() ? std::string() : ", Fehler:" + z->fehler));
        return z->fehler.empty() && figurAnimierbar(figur("mb7_a")) && figurAnimierbar(figur("mb7_b")) && figurAnimierbar(figur("mb7_c")) && kino;
    }));
}

std::vector<Schritt> uiSchritte(std::shared_ptr<Zustand> z) {
    std::vector<Schritt> s;
    const auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    aufbau(s, z);
    // --- Erreichbarkeit: Inspektor, Rechtsklick, Menue, Suche, Tooltips ---
    add(tu("MB7: Kamera auf die Figuren (schraeg von hinten oben)", [=] {
        g_app->throughCamera = false;
        g_app->cam.pos[0] = z->spieler[0] - 40.0F;
        g_app->cam.pos[1] = z->spieler[1] - 30.0F;
        g_app->cam.pos[2] = z->boden + 190.0F;
        const float ziel[3] = {z->spieler[0] + 110.0F, z->spieler[1] - 30.0F, z->boden + 30.0F};
        aimAngles(g_app->cam.pos, ziel, g_app->cam.angles);
        g_app->mapDirty = true;
    }));
    add(tu("MB7: Spawner von mb7_a waehlen (Inspektor), Zeit 1 s", [] {
        entityAuswahlSetzen(entityFuerFigur(figur("mb7_a")));
        g_app->playMs = 1000.0;
        g_app->playing = false;
    }));
    add(pause(0.3));
    add(klick(tr(Str::FaAnimieren)));   // der Knopf im Inspektor der Figur
    add(warteBis("MB7: Szene nach dem Anlegen", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 3000));
    add(pruefe("MB7: Inspektor 'Figur animieren' legt den Block an - BOTH_CIN_2 (BOTH_CIN_1 der vorhandenen Karten-GLA bleibt), ein Rueckgaengig-Schritt, Fenster offen",
               [=] {
                   modell::FigurAnim a;
                   const bool da = animVon("mb7_a", a) != nullptr;
                   diag::detail("Knopftest: MB7 angelegt - Platz " + (da ? a.platz : std::string("-")) + ", Basis " + (da ? a.basis : std::string("-")) + ", Start " +
                                std::to_string(da ? a.startMs : -1.0) + ", Fenster " + std::to_string(g_app->figAnim.offen) + ", Meldung " + g_app->figAnim.meldung);
                   return da && a.platz == "BOTH_CIN_2" && a.startMs == 1000.0 && g_app->figAnim.offen && g_app->figAnim.figur == "mb7_a" && g_app->doc.dirty();
               }));
    add(pruefe("MB7: die Vorschau spielt BOTH_CIN_2 aus der gebackenen Karten-GLA (Anhang, kein T-Pose), die Karten-GLA hat BOTH_CIN_1 und BOTH_CIN_2", [=] {
        const int ai = figur("mb7_a");
        const ActorState st = g_app->scene.actors[static_cast<std::size_t>(ai)].at(1500.0);
        bool c1 = false;
        bool c2 = false;
        for (const AnimEntry& e : g_app->figurKino.sections) {
            c1 = c1 || e.name == "BOTH_CIN_1";
            c2 = c2 || e.name == "BOTH_CIN_2";
        }
        std::vector<BoneMatrix> w;
        float fw[16];
        const GlaAnimation* g = nullptr;
        const bool gez = gezeichnet("mb7_a", 1500.0, w, fw, &g);
        diag::detail("Knopftest: MB7 Vorschau - Animation " + st.animation + ", Karten-GLA " + std::to_string(g_app->figurKino.anim.numFrames) + " Bilder, " +
                     std::to_string(g_app->figurKino.sections.size()) + " Zeilen, Anhang " + std::to_string(g != nullptr && g->anhang == &g_app->figurKino.anim));
        return st.animation == "BOTH_CIN_2" && c1 && c2 && gez && g != nullptr && g->anhang == &g_app->figurKino.anim;
    }));
    add(pruefe("MB7: jede Aktion mit Tooltip, Aktionen mit Taste nennen sie im Tooltip", [] {
        int ohne = 0;
        int mitTaste = 0;
        for (const std::string& t : figurAnimAktionsTexte()) {
            const std::size_t bar = t.find('|');
            if (bar == std::string::npos || bar + 1 >= t.size()) { ++ohne; }
            if (t.find("  (") != std::string::npos) { mitTaste += t.find(":", bar) != std::string::npos ? 1 : -100; }
        }
        diag::detail("Knopftest: MB7 Aktionen " + std::to_string(figurAnimAktionsTexte().size()) + ", ohne Tooltip " + std::to_string(ohne) + ", mit Taste " +
                     std::to_string(mitTaste));
        return ohne == 0 && mitTaste >= 5;
    }));
    add(klick(tr(Str::FaGruppePosieren), "###figuranim"));   // Menue des Fensters
    add(pause(0.2));
    add(pruefe("MB7: Menue des Fensters 'Posieren' offen", [] { return finde(tr(Str::FaGizmoVerschieben), "", 1, false, "") != nullptr; }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(zeitAuf("mb7_a", 0));
    add(pause(0.3));
    add(klickAn("Ansicht (Fokus)", [] { return ImVec2{g_app->kartenAnsicht[0] + 40.0F, g_app->kartenAnsicht[3] - 40.0F}; }));
    add(taste(ImGuiKey_X, "X"));
    add(pause(0.2));
    add(pruefe("MB7: X ueber der Ansicht oeffnet 'Alle Aktionen suchen' (wie der Model-Tab im Max-Modus)", [] { return g_app->figAnim.sucheSichtbar; }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    // --- Griffe in der Ansicht ---
    add(mission1::probeFoto("mb7_griffe"));
    add(pruefe("MB7: Griffe des Control Rigs auf der Figur gezeichnet, Wurzelweg mit einem Punkt je Bild", [] {
        const ImVec2 q = griffOrt("pelvis");
        diag::detail("Knopftest: MB7 Griffe " + std::to_string(g_app->figAnim.griffeGezeichnet) + ", Pfadpunkte " + std::to_string(g_app->figAnim.pfadPunkte) +
                     ", Becken-Griff " + std::to_string(figurAnimGriffSuchen("pelvis")) + " bei " + std::to_string(q.x) + "/" + std::to_string(q.y) + ", Ansicht " +
                     std::to_string(g_app->kartenAnsicht[0]) + "/" + std::to_string(g_app->kartenAnsicht[1]) + "-" + std::to_string(g_app->kartenAnsicht[2]) + "/" +
                     std::to_string(g_app->kartenAnsicht[3]) + ", offen " + std::to_string(g_app->figAnim.offen) + " posieren " + std::to_string(g_app->figAnim.posieren) +
                     " spielt " + std::to_string(g_app->playing) + " Kamera " + std::to_string(g_app->camViewActive) + " Figur " + g_app->figAnim.figur);
        return g_app->figAnim.griffeGezeichnet >= 15 && g_app->figAnim.pfadPunkte >= 50;
    }));
    add(klickAn("Rechtsklick auf den Kopf-Griff", [] { return griffOrt("cranium"); }, 1));
    add(pause(0.2));
    add(pruefe("MB7: Rechtsklick auf einen Griff oeffnet das Menue der Animation", [] { return g_app->figAnim.kontextSichtbar; }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(tu("MB7: Stand vor dem Zug am Becken", [=] {
        z->tiefe = g_app->doc.undoDepth();
        z->zeilen = zeilen("mb7_a");
        const std::vector<std::string> v;
    }));
    add(zeitAuf("mb7_a", 59));
    add(pause(0.3));
    add(taste(ImGuiKey_W, "W"));
    add(ziehenAn("Becken am letzten Bild (W)", [] { return griffOrt("pelvis"); }, 120.0F, 0.0F, 0));
    add(warteBis("MB7: Szene nach dem Zug", [] { return g_app->camTrackValid; }, 600));
    add(pruefe("MB7: W am Becken - Schluessel am Bild 59, ein Rueckgaengig-Schritt, der Wurzelweg wird laenger, SET_ORIGIN = Start + Weg des Beckens", [=] {
        modell::FigurAnim a;
        std::array<float, 3> o{};
        const bool so = setOrigin("mb7_a", o);
        std::vector<BoneMatrix> w0;
        float fw0[16];
        std::vector<BoneMatrix> w1;
        float fw1[16];
        const GlaAnimation* g = nullptr;
        // Becken am Ende (gebackene Karten-GLA) und im ersten Bild der Folgeanimation nach SET_ORIGIN
        modell::FigurAnim b;
        (void)animVon("mb7_a", b);
        const double ende = figurAnimStartMs("mb7_a") + modell::figurDauerMs(b) - 25.0;
        const bool g0 = gezeichnet("mb7_a", ende, w0, fw0, &g);
        const bool g1 = gezeichnet("mb7_a", ende + 400.0, w1, fw1);
        const std::array<float, 3> p0 = g0 && g != nullptr ? knochenInKarte(w0, fw0, *g, "pelvis") : std::array<float, 3>{};
        const std::array<float, 3> p1 = g1 && g != nullptr ? knochenInKarte(w1, fw1, *g, "pelvis") : std::array<float, 3>{};
        const ActorState st0 = g_app->scene.actors[static_cast<std::size_t>(figur("mb7_a"))].at(500.0);
        z->ursprung = o;
        diag::detail("Knopftest: MB7 Becken - Rueckgaengig " + std::to_string(g_app->doc.undoDepth() - z->tiefe) + ", SET_ORIGIN " + v3(o.data()) + ", Start " +
                     v3(st0.pos) + ", Becken am Ende " + v3(p0.data()) + ", nach dem Wechsel " + v3(p1.data()) + " (" + std::to_string(ab2(p0, p1)) + ")");
        return so && g_app->doc.undoDepth() == z->tiefe + 1U && zeilen("mb7_a") != z->zeilen && std::hypot(o[0] - st0.pos[0], o[1] - st0.pos[1]) > 30.0F &&
               ab2(p0, p1) < 8.0F;
    }));
    add(tu("MB7: Rot-Probe - SET_ORIGIN aus", [] { (void)figurAnimEinstellen([](modell::FigurAnim& a) { a.setOrigin = false; }); }));
    add(warteBis("MB7: Szene ohne SET_ORIGIN", [] { return g_app->camTrackValid; }, 600));
    add(pruefe("MB7: ohne SET_ORIGIN springt die Figur beim Wechsel zurueck (wie in Stufe 0 im Spiel gemessen)", [=] {
        modell::FigurAnim b;
        (void)animVon("mb7_a", b);
        const double ende = figurAnimStartMs("mb7_a") + modell::figurDauerMs(b) - 25.0;
        std::vector<BoneMatrix> w0;
        std::vector<BoneMatrix> w1;
        float fw0[16];
        float fw1[16];
        const GlaAnimation* g = nullptr;
        const bool ok = gezeichnet("mb7_a", ende, w0, fw0, &g) && gezeichnet("mb7_a", ende + 400.0, w1, fw1);
        const float sprung = ok && g != nullptr ? ab2(knochenInKarte(w0, fw0, *g, "pelvis"), knochenInKarte(w1, fw1, *g, "pelvis")) : 0.0F;
        std::array<float, 3> o{};
        diag::detail("Knopftest: MB7 ohne SET_ORIGIN - Sprung " + std::to_string(sprung) + " Einheiten, SET_ORIGIN im Block " + std::to_string(setOrigin("mb7_a", o)));
        return !setOrigin("mb7_a", o) && sprung > 30.0F;
    }));
    add(tu("MB7: SET_ORIGIN wieder an", [] { (void)figurAnimEinstellen([](modell::FigurAnim& a) { a.setOrigin = true; }); }));
    add(warteBis("MB7: Szene mit SET_ORIGIN", [] { return g_app->camTrackValid; }, 600));
    // Hand (IK) und Kopf (FK)
    add(zeitAuf("mb7_a", 30));
    add(pause(0.3));
    add(ziehenAn("rechte Hand am Bild 30 (W = IK)", [] { return griffOrt("rhand"); }, 0.0F, -40.0F, 0));
    add(warteBis("MB7: Szene nach der Hand", [] { return g_app->camTrackValid; }, 600));
    add(pruefe("MB7: W an der Hand ist IK - IK-Schluessel am Bild 30, die Hand der Vorschau am IK-Ziel", [=] {
        int griff = figurAnimGriffSuchen(std::string(tr(Str::MbIkArmR)) + " IK");
        ImVec2 q;
        const bool zielDa = griff >= 0 && figurAnimGriffSchirm(griff, q);
        std::vector<std::array<float, 3>> orte;
        std::vector<std::string> namen;
        const bool da = figurAnimWeltBei("mb7_a", 30, orte, &namen);
        std::array<float, 3> hand{};
        for (std::size_t i = 0; i < namen.size(); ++i) {
            if (namen[i] == "rhand") { hand = orte[i]; }
        }
        // der gezeichnete Ort (gebacken) gegen den der Schicht
        std::vector<BoneMatrix> w;
        float fw[16];
        const GlaAnimation* g = nullptr;
        const bool gz = gezeichnet("mb7_a", figurAnimStartMs("mb7_a") + 30.0 * 50.0, w, fw, &g);
        const std::array<float, 3> handG = gz && g != nullptr ? knochenInKarte(w, fw, *g, "rhand") : std::array<float, 3>{};
        z->handZiel = hand;
        diag::detail("Knopftest: MB7 Hand - IK-Griff " + std::to_string(griff) + ", Hand " + v3(hand.data()) + ", gezeichnet " + v3(handG.data()) + " (" +
                     std::to_string(ab(hand, handG)) + ")");
        return zielDa && da && gz && ab(hand, handG) < 0.5F;
    }));
    add(taste(ImGuiKey_E, "E"));
    add(ziehenAn("Kopf am Bild 30 (E = drehen)", [] { return griffOrt("cranium"); }, 50.0F, 0.0F, 0));
    add(warteBis("MB7: Szene nach dem Kopf", [] { return g_app->camTrackValid; }, 600));
    add(pruefe("MB7: E am Kopf ist FK - Schluessel am Kopfknochen", [] {
        const std::vector<std::string> z2 = zeilen("mb7_a");
        // die Daten des Kopfknochens (k <index>)
        const int ai = figur("mb7_a");
        const int k = boneIndex(g_app->actorAssets[static_cast<std::size_t>(ai)]->skeleton->anim, "cranium");
        const std::string marke = "\"mb7_a\" k " + std::to_string(k) + " ";
        return std::any_of(z2.begin(), z2.end(), [&](const std::string& l) { return l.find(marke) != std::string::npos; });
    }));
    // Fuss auf den Boden, Ziel auf eine Flaeche
    add(tu("MB7: linken Fuss waehlen, Fuss auf den Boden", [] {
        g_app->figAnim.griffWahl = figurAnimGriffSuchen("ltalus");
        (void)figurAnimAktion(11);
    }));
    add(warteBis("MB7: Szene nach dem Fuss", [] { return g_app->camTrackValid; }, 600));
    add(pruefe("MB7: Fuss auf den Boden - der Fuss steht so hoch ueber der Flaeche wie im ersten Bild der Grundanimation ueber der Sohle", [=] {
        std::vector<std::array<float, 3>> o0;
        std::vector<std::array<float, 3>> o;
        std::vector<std::string> n;
        (void)figurAnimWeltBei("mb7_a", 0, o0, &n);
        (void)figurAnimWeltBei("mb7_a", 30, o, nullptr);
        float h0 = 0.0F;
        float h = 0.0F;
        for (std::size_t i = 0; i < n.size(); ++i) {
            if (n[i] == "ltalus") {
                h0 = o0[i][2];
                h = o[i][2];
            }
        }
        const ActorState st = g_app->scene.actors[static_cast<std::size_t>(figur("mb7_a"))].at(500.0);
        diag::detail("Knopftest: MB7 Fuss - Bild 0 " + std::to_string(h0 - (st.pos[2] - 24.0F)) + " ueber der Sohle, Bild 30 " + std::to_string(h - z->boden) +
                     " ueber dem Boden, Meldung " + g_app->figAnim.meldung);
        return std::fabs((h - z->boden) - (h0 - (st.pos[2] - 24.0F))) < 0.05F;
    }));
    add(tu("MB7: rechte Hand waehlen, Ziel auf Flaeche", [] {
        g_app->figAnim.griffWahl = figurAnimGriffSuchen("rhand");
        (void)figurAnimAktion(12);
    }));
    add(klickAn("Boden neben der Figur", [] {
        const int ai = figur("mb7_a");
        const ActorState st = g_app->scene.actors[static_cast<std::size_t>(ai)].at(g_app->playMs);
        const float p[3] = {st.pos[0] + 20.0F, st.pos[1] - 10.0F, st.pos[2] - 24.0F};
        ImVec2 q;
        (void)kartenPunktAufSchirm(p, q);
        return q;
    }));
    add(warteBis("MB7: Szene nach dem Ziel", [] { return g_app->camTrackValid; }, 600));
    add(pruefe("MB7: Ziel auf Flaeche - die rechte Hand liegt auf dem Punkt unter der Maus (Strahl in die Karte)", [=] {
        std::vector<std::array<float, 3>> o;
        std::vector<std::string> n;
        (void)figurAnimWeltBei("mb7_a", 30, o, &n);
        std::array<float, 3> hand{};
        for (std::size_t i = 0; i < n.size(); ++i) {
            if (n[i] == "rhand") { hand = o[i]; }
        }
        float ziel[3] = {0.0F, 0.0F, 0.0F};
        const bool da = figurAnimIkZiel("mb7_a", static_cast<int>(modell::IkArt::ArmRechts), 30, ziel);
        const std::array<float, 3> zz{ziel[0], ziel[1], ziel[2]};
        const std::array<float, 3> treffer{g_app->figAnim.letzterTreffer[0], g_app->figAnim.letzterTreffer[1], g_app->figAnim.letzterTreffer[2]};
        diag::detail("Knopftest: MB7 Ziel auf Flaeche - IK-Ziel " + v3(ziel) + ", Treffer " + v3(treffer.data()) + ", Hand " + v3(hand.data()) + " vorher " +
                     v3(z->handZiel.data()) + ", Meldung " + g_app->figAnim.meldung);
        // das Ziel liegt auf dem Treffer; die Hand geht dorthin (ausser Reichweite gestreckt: naeher als vorher)
        return da && ab(zz, treffer) < 0.01F && ab(hand, zz) < ab(z->handZiel, zz) && !g_app->figAnim.zielKlick;
    }));
    // Dopesheet: IK-Zeile, Raute ziehen
    add(pruefe("MB7: Dopesheet zeigt IK-Zeilen (Rauten der IK-Schluessel)", [] {
        diag::detail("Knopftest: MB7 Dopesheet - Rauten " + std::to_string(g_app->figAnim.rauten) + ", IK " + std::to_string(g_app->figAnim.ikRauten));
        return g_app->figAnim.ikRauten >= 1;
    }));
    add(ziehenAn("IK-Raute am Bild 30 nach rechts", [] {
        for (const auto& r : g_app->figAnim.rautenOrte) {
            if (r[0] <= static_cast<float>(modell::kIkWahl) && static_cast<int>(r[1]) == 30) { return ImVec2{r[2], r[3]}; }
        }
        return ImVec2{-1.0F, -1.0F};
    }, 60.0F, 0.0F, 0));
    add(warteBis("MB7: Szene nach dem Dopesheet", [] { return g_app->camTrackValid; }, 600));
    add(pruefe("MB7: IK-Raute gezogen - der IK-Schluessel liegt spaeter, die FK-Schluessel der Kette am alten Bild gingen mit", [] {
        const std::vector<std::string> z2 = zeilen("mb7_a");
        bool ik30 = false;
        bool ikSpaeter = false;
        for (const std::string& l : z2) {
            if (l.find("\"mb7_a\" ik 1 ") == std::string::npos) { continue; }
            ik30 = l.find(" 30 2 ") != std::string::npos;
            for (int b = 31; b < 60; ++b) { ikSpaeter = ikSpaeter || l.find(" " + std::to_string(b) + " 2 ") != std::string::npos; }
        }
        diag::detail("Knopftest: MB7 IK-Zeile verschoben - noch 30 " + std::to_string(ik30) + ", spaeter " + std::to_string(ikSpaeter) + ", Meldung " + g_app->figAnim.meldung);
        return !ik30 && ikSpaeter;
    }));
    // Strg+Z: ein Schritt zurueck
    add(tu("MB7: Stand vor Strg+Z", [=] { z->zeilen = zeilen("mb7_a"); }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(warteBis("MB7: Szene nach Strg+Z", [] { return g_app->camTrackValid; }, 600));
    add(pruefe("MB7: Strg+Z nimmt den letzten Schritt zurueck", [=] { return zeilen("mb7_a") != z->zeilen && !zeilen("mb7_a").empty(); }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Y, "Strg+Y"));
    add(warteBis("MB7: Szene nach Strg+Y", [] { return g_app->camTrackValid; }, 600));
    // --- mehrere Figuren ---
    add(tu("MB7: Figur b wartet auf das Signal von a, Figur c haelt b", [] {
        (void)figurAnimEinstellen([](modell::FigurAnim& a) { a.sendeSignal = "mb7_los"; });
        g_app->playMs = 0.0;
        (void)figurAnimStarten("mb7_b");
        (void)figurAnimEinstellen([](modell::FigurAnim& a) {
            a.warteSignal = "mb7_los";
            a.basis = "BOTH_STAND1";
        });
        (void)figurAnimStarten("mb7_c");
    }));
    add(warteBis("MB7: Szene mit drei Bloecken", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 1200));
    add(pruefe("MB7: b beginnt, wenn a das Signal sendet (Ende von a), drei Plaetze verschieden", [] {
        modell::FigurAnim a;
        (void)animVon("mb7_a", a);
        const double endeA = figurAnimStartMs("mb7_a") + modell::figurDauerMs(a);
        const double startB = figurAnimStartMs("mb7_b");
        modell::FigurAnim b;
        modell::FigurAnim c;
        (void)animVon("mb7_b", b);
        (void)animVon("mb7_c", c);
        diag::detail("Knopftest: MB7 Signal - Ende a " + std::to_string(endeA) + ", Start b " + std::to_string(startB) + ", Plaetze " + a.platz + " " + b.platz + " " + c.platz);
        return std::fabs(startB - endeA) <= 100.0 && a.platz != b.platz && b.platz != c.platz && a.platz != c.platz;
    }));
    add(tu("MB7: b bewegt das Becken (Schluessel 0 und 59)", [] {
        g_app->figAnim.figur = "mb7_b";
        (void)figurAnimEinstellen([](modell::FigurAnim& a) {
            const int ai = figur("mb7_b");
            const int k = boneIndex(g_app->actorAssets[static_cast<std::size_t>(ai)]->skeleton->anim, "pelvis");
            BoneMatrix m = BoneMatrix::identity();
            modell::rigSchluesselSetzen(a.schicht, k, 0, m);
            m.m[1][3] = 10.0F;
            modell::rigSchluesselSetzen(a.schicht, k, a.bilder - 1, m);
        });
    }));
    add(warteBis("MB7: Szene", [] { return g_app->camTrackValid; }, 600));
    add(tu("MB7: c haelt mit der linken Hand die rechte Hand von b ab Bild 10 (Halten)", [] {
        g_app->figAnim.figur = "mb7_c";
        g_app->playMs = figurAnimStartMs("mb7_c") + 10.0 * 50.0;
        if (!figurAnimHalten(static_cast<int>(modell::IkArt::ArmLinks), "mb7_b", "rhand")) { meldeFehler("MB7: Halten nicht gesetzt: " + g_app->figAnim.meldung); }
    }));
    add(warteBis("MB7: Szene nach dem Halten", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 600));
    add(pruefe("MB7: c haelt b - der Abstand der Haende bleibt, waehrend b sich bewegt (gezeichnet aus der gebackenen GLA von c)", [] {
        float d[2] = {0.0F, 0.0F};
        int ok = 0;
        for (int i = 0; i < 2; ++i) {
            const double ms = figurAnimStartMs("mb7_c") + (i == 0 ? 12.0 : 55.0) * 50.0;
            std::vector<BoneMatrix> wb;
            std::vector<BoneMatrix> wc;
            float fb[16];
            float fc[16];
            const GlaAnimation* gb = nullptr;
            const GlaAnimation* gc = nullptr;
            if (gezeichnet("mb7_b", ms, wb, fb, &gb) && gezeichnet("mb7_c", ms, wc, fc, &gc) && gb != nullptr && gc != nullptr) {
                d[i] = ab(knochenInKarte(wb, fb, *gb, "rhand"), knochenInKarte(wc, fc, *gc, "lhand"));
                ++ok;
            }
        }
        std::vector<std::array<float, 3>> b0;
        std::vector<std::array<float, 3>> b1;
        std::vector<std::string> n;
        (void)figurAnimWeltBei("mb7_b", 12, b0, &n);
        (void)figurAnimWeltBei("mb7_b", 55, b1, nullptr);
        float weg = 0.0F;
        for (std::size_t i = 0; i < n.size(); ++i) {
            if (n[i] == "rhand") { weg = ab(b0[i], b1[i]); }
        }
        diag::detail("Knopftest: MB7 Halten - Abstand der Haende " + std::to_string(d[0]) + " / " + std::to_string(d[1]) + ", Hand von b bewegt " + std::to_string(weg));
        return ok == 2 && std::fabs(d[0] - d[1]) < 0.75F && weg > 3.0F;
    }));
    // --- wie Blender: G modal ---
    add(tu("MB7: wie Blender, Figur a, Bild 40, rechte Hand", [=] {
        modellBedienungSetzen(1);
        g_app->figAnim.figur = "mb7_a";
        g_app->playMs = figurAnimStartMs("mb7_a") + 40.0 * 50.0;
        g_app->figAnim.griffWahl = figurAnimGriffSuchen(std::string(tr(Str::MbIkArmR)) + " IK");
        z->zeilen = zeilen("mb7_a");
        z->tiefe = g_app->doc.undoDepth();
    }));
    add(pause(0.3));
    add(klickAn("Ansicht (Fokus, mittlere Taste)", [] { return ImVec2{g_app->kartenAnsicht[0] + 40.0F, g_app->kartenAnsicht[3] - 40.0F}; }, 2));
    add(taste(ImGuiKey_G, "G"));
    add(tu("MB7: Maus bewegen", [] { setzeMaus(ImVec2{g_app->kartenAnsicht[0] + 90.0F, g_app->kartenAnsicht[3] - 60.0F}); }));
    add(pause(0.2));
    add(pruefe("MB7: G laeuft modal (ohne gedrueckte Taste)", [] { return g_app->figAnim.zieht && g_app->figAnim.modal; }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pruefe("MB7: Esc bricht G ab - nichts geaendert", [=] { return !g_app->figAnim.zieht && zeilen("mb7_a") == z->zeilen && g_app->doc.undoDepth() == z->tiefe; }));
    add(taste(ImGuiKey_G, "G"));
    add(tu("MB7: Maus bewegen", [] { setzeMaus(ImVec2{g_app->kartenAnsicht[0] + 100.0F, g_app->kartenAnsicht[3] - 70.0F}); }));
    add(pause(0.2));
    add(klickAn("Klick uebernimmt", [] { return ImVec2{g_app->kartenAnsicht[0] + 100.0F, g_app->kartenAnsicht[3] - 70.0F}; }));
    add(warteBis("MB7: Szene nach G", [] { return g_app->camTrackValid; }, 600));
    add(pruefe("MB7: G + Klick uebernimmt (ein Rueckgaengig-Schritt)", [=] { return zeilen("mb7_a") != z->zeilen && g_app->doc.undoDepth() == z->tiefe + 1U; }));
    add(tu("MB7: zurueck zu wie 3ds Max", [] { modellBedienungSetzen(0); }));
    // --- Bauen ---
    add(tu("MB7: Fuer das Spiel bauen (zweimal, Karten-GLA)", [=] {
        g_app->figAnim.ziel = 0;
        const std::string a = mb::verbinde(z->ordner, "eins");
        const std::string b = mb::verbinde(z->ordner, "zwei");
        if (!figurAnimBauen(a, false)) {
            meldeFehler("MB7: Bauen: " + g_app->figAnim.meldung + (g_app->figAnim.befunde.empty() ? std::string() : " / " + g_app->figAnim.befunde.front()));
        }
        (void)figurAnimBauen(b, true);
    }));
    add(pruefe("MB7: gebaut - GLA (Name im Kopf), animation.cfg mit BOTH_CIN_1..4, Skript .IBI; zweimal gebaut Byte fuer Byte gleich; = GLA der Vorschau", [=] {
        const std::string rel = "models/players/_humanoid_mb7szene/";
        const std::string g1 = slurp(mb::verbinde(mb::verbinde(z->ordner, "eins"), rel + "_humanoid_mb7szene.gla"));
        const std::string g2 = slurp(mb::verbinde(mb::verbinde(z->ordner, "zwei"), rel + "_humanoid_mb7szene.gla"));
        const std::string cfg = slurp(mb::verbinde(mb::verbinde(z->ordner, "eins"), rel + "animation.cfg"));
        const std::string ref = aktiverSkriptPfad();
        const std::string ibi = slurp(mb::verbinde(mb::verbinde(z->ordner, "eins"), "scripts/" + ref + ".IBI"));
        GlaAnimation g;
        const bool lesbar = readGla(g1, g);
        std::string vorschau;
        (void)schreibGla(g_app->figurKino.anim, vorschau);
        int cin = 0;
        for (const AnimEntry& e : parseAnimationCfg(cfg)) { cin += e.name.rfind("BOTH_CIN_", 0) == 0 ? 1 : 0; }
        diag::detail("Knopftest: MB7 gebaut - GLA " + std::to_string(g1.size()) + " Bytes, " + std::to_string(g.numFrames) + " Bilder, " + std::to_string(g.bones.size()) +
                     " Knochen, Kopf " + g.name + ", cfg " + std::to_string(cin) + " BOTH_CIN, IBI " + std::to_string(ibi.size()) + " Bytes (scripts/" + ref +
                     "), gleich " + std::to_string(g1 == g2) + ", = Vorschau " + std::to_string(g1 == vorschau) + ", Meldung " + g_app->figAnim.meldung);
        return lesbar && !g1.empty() && g1 == g2 && g1 == vorschau && cin == 4 && ibi.size() > 30U &&
               modell::kartenGlaPruefen(g, "models/players/_humanoid_mb7szene/_humanoid_mb7szene", modell::Zielspiel::MdSp).empty();
    }));
    add(pruefe("MB7: Rundlauf - Knochen ohne Schicht (Kopf von c) tragen die Bilder der Grundanimation (14 Bytes gleich)", [] {
        modell::FigurAnim c;
        if (animVon("mb7_c", c) == nullptr) { return false; }
        const auto it = std::find_if(g_app->figurKino.sections.begin(), g_app->figurKino.sections.end(), [&](const AnimEntry& e) { return e.name == c.platz; });
        const int ai = figur("mb7_c");
        if (it == g_app->figurKino.sections.end() || ai < 0) { return false; }
        const App::Skeleton* sk = g_app->actorAssets[static_cast<std::size_t>(ai)]->skeleton;
        GlaAnimation basis;
        (void)modell::basisGla(sk->anim, sk->grundGemerkt ? sk->grundListe : sk->sections, c.basis, it->numFrames, c.fps, basis);
        const GlaAnimation& k = g_app->figurKino.anim;
        const int kopf = boneIndex(k, "cranium");
        int gleich = 0;
        for (int f = 0; f < it->numFrames && kopf >= 0; ++f) {
            const std::uint8_t* p = k.bonePool.data() + static_cast<std::size_t>(k.indexes[static_cast<std::size_t>(it->firstFrame + f) * k.bones.size() + static_cast<std::size_t>(kopf)]) * 14U;
            const std::uint8_t* q = basis.bonePool.data() + static_cast<std::size_t>(basis.indexes[static_cast<std::size_t>(f) * basis.bones.size() + static_cast<std::size_t>(kopf)]) * 14U;
            gleich += std::memcmp(p, q, 14U) == 0 ? 1 : 0;
        }
        diag::detail("Knopftest: MB7 Rundlauf - Kopf von c " + std::to_string(gleich) + " von " + std::to_string(it->numFrames) + " Bildern gleich");
        return gleich == it->numFrames;
    }));
    return s;
}

// Die Szene fuer den Vergleich im Spiel: drei Figuren, a geht (Wurzelweg) und gibt b frei, b geht zur Seite, c haelt b an der
// Hand; gebaut mit SET_ORIGIN (Ordner "mit") und ohne (Ordner "ohne", die Rot-Probe aus Stufe 0); dazu die Mission als .pk3
// und die Lage der Figuren in der Vorschau je 250 ms (mb7_vorschau_<art>.txt im Ausgabeordner).
std::vector<Schritt> spielSchritte(std::shared_ptr<Zustand> z) {
    std::vector<Schritt> s;
    const auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    aufbau(s, z);
    add(tu("MB7 Spiel: drei Figuren animieren; Spielfenster 1920 x 1440 wie spiel.py", [] {
        g_app->settings.spielSeiten = 1920.0F / 1440.0F;
        g_app->playMs = 1000.0;
        (void)figurAnimStarten("mb7_a");
        (void)figurAnimEinstellen([](modell::FigurAnim& a) {
            a.bilder = 60;
            a.sendeSignal = "mb7_los";
        });
    }));
    add(warteBis("MB7 Spiel: Szene", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 1200));
    add(tu("MB7 Spiel: a geht 120 Einheiten nach vorn (+X der Karte)", [] {
        const float null[3] = {0.0F, 0.0F, 0.0F};
        const float vor[3] = {120.0F, 0.0F, 0.0F};
        (void)figurAnimBeckenWeg("mb7_a", 0, null);
        (void)figurAnimBeckenWeg("mb7_a", 59, vor);
        g_app->playMs = 0.0;
        (void)figurAnimStarten("mb7_b");
        (void)figurAnimEinstellen([](modell::FigurAnim& a) {
            a.bilder = 40;
            a.warteSignal = "mb7_los";
        });
    }));
    add(warteBis("MB7 Spiel: Szene", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 1200));
    add(tu("MB7 Spiel: b geht 60 Einheiten zur Seite (-Y der Karte); c ab 3,5 s", [] {
        const float null[3] = {0.0F, 0.0F, 0.0F};
        const float seite[3] = {0.0F, -60.0F, 0.0F};
        (void)figurAnimBeckenWeg("mb7_b", 0, null);
        (void)figurAnimBeckenWeg("mb7_b", 39, seite);
        g_app->playMs = 3500.0;
        (void)figurAnimStarten("mb7_c");
        (void)figurAnimEinstellen([](modell::FigurAnim& a) { a.bilder = 70; });
    }));
    add(warteBis("MB7 Spiel: Szene", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 1200));
    add(tu("MB7 Spiel: c haelt die rechte Hand von b ab Bild 12", [] {
        g_app->figAnim.figur = "mb7_c";
        g_app->playMs = figurAnimStartMs("mb7_c") + 12.0 * 50.0;
        if (!figurAnimHalten(static_cast<int>(modell::IkArt::ArmLinks), "mb7_b", "rhand")) { meldeFehler("MB7 Spiel: Halten: " + g_app->figAnim.meldung); }
    }));
    add(warteBis("MB7 Spiel: Szene", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 1200));
    // eng (Fund 1): mit SET_ORIGIN, aber der Spawner von c 28 Einheiten neben dem von b - NPC_Begin von c scheitert an
    // SpotWouldTelefrag, bis b per SET_ORIGIN weggeht (ablauf.cpp npcBegin); das affect auf c entfaellt dann.
    for (const char* art : {"mit", "ohne", "eng"}) {
        const std::string a = art;
        if (a == "eng") {
            add(tu("MB7 Spiel: SET_ORIGIN wieder an, Spawner von c 28 Einheiten neben b", [=] {
                for (const char* f : {"mb7_a", "mb7_b", "mb7_c"}) {
                    g_app->figAnim.figur = f;
                    (void)figurAnimEinstellen([](modell::FigurAnim& x) { x.setOrigin = true; });
                }
                const int eb = entityFuerFigur(figur("mb7_b"));
                const int ec = entityFuerFigur(figur("mb7_c"));
                float ob[3]{};
                if (eb < 0 || ec < 0 || !entityOrt(g_app->map.entities[static_cast<std::size_t>(eb)], ob)) {
                    meldeFehler("MB7 Spiel: Spawner b/c");
                    return;
                }
                const float oc[3] = {ob[0] + 28.0F, ob[1], ob[2]};
                (void)entityKeySetzen(ec, "origin", v3(oc));
                g_app->camTrackValid = false;
                g_app->mapDirty = true;
            }));
            add(warteBis("MB7 Spiel: Szene eng", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 1200));
        }
        if (a == "ohne") {
            add(tu("MB7 Spiel: SET_ORIGIN aus (alle drei)", [] {
                for (const char* f : {"mb7_a", "mb7_b", "mb7_c"}) {
                    g_app->figAnim.figur = f;
                    (void)figurAnimEinstellen([](modell::FigurAnim& x) { x.setOrigin = false; });
                }
            }));
            add(warteBis("MB7 Spiel: Szene ohne SET_ORIGIN", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 1200));
        }
        add(tu("MB7 Spiel: bauen und Vorschau aufzeichnen (" + a + ")", [=] {
            prepareScene();
            const std::string o = mb::verbinde(z->ordner, a);
            g_app->figAnim.ziel = 0;
            if (!figurAnimBauen(o, true)) { meldeFehler("MB7 Spiel: Bauen " + a + ": " + g_app->figAnim.meldung); }
            std::string f;
            if (!missionSpeichernNach(mb::verbinde(o, "zz_mb7szene.pk3"), &f)) { meldeFehler("MB7 Spiel: Mission: " + f); }
            std::string text = "# t_ms figur becken_x becken_y becken_z mitte_x mitte_y mitte_z animation\n";
            for (int t = 0; t <= 12000; t += 250) {
                for (const char* fig : {"mb7_a", "mb7_b", "mb7_c"}) {
                    std::vector<BoneMatrix> w;
                    float fw[16];
                    const GlaAnimation* g = nullptr;
                    if (!gezeichnet(fig, static_cast<double>(t), w, fw, &g) || g == nullptr) { continue; }
                    const std::array<float, 3> p = knochenInKarte(w, fw, *g, "pelvis");
                    std::array<float, 3> m{};
                    for (std::size_t b = 0; b < w.size(); ++b) {
                        const std::array<float, 3> q = knochenInKarte(w, fw, *g, g->bones[b].name.c_str());
                        for (int k = 0; k < 3; ++k) { m[static_cast<std::size_t>(k)] += q[static_cast<std::size_t>(k)] / static_cast<float>(w.size()); }
                    }
                    char z2[300];
                    std::snprintf(z2, sizeof z2, "%d %s %.3f %.3f %.3f %.3f %.3f %.3f %s\n", t, fig, static_cast<double>(p[0]), static_cast<double>(p[1]),
                                  static_cast<double>(p[2]), static_cast<double>(m[0]), static_cast<double>(m[1]), static_cast<double>(m[2]),
                                  g_app->scene.actors[static_cast<std::size_t>(figur(fig))].at(t).animation.c_str());
                    text += z2;
                }
            }
            for (const MapEntity& e : g_app->map.entities) {
                if (e.classname == "info_player_start") { text += "# info_player_start " + e.origin + "\n"; }
            }
            for (const char* fig : {"mb7_a", "mb7_b", "mb7_c"}) {
                const auto it = g_app->ablauf.erscheint.find(fig);
                text += std::string("# erscheint ") + fig + " " + std::to_string(it != g_app->ablauf.erscheint.end() ? it->second : -1.0) + "\n";
            }
            text += "# skript " + aktiverSkriptPfad() + "\n";
            {
                const float kam[3] = {z->spieler[0] + 128.0F, z->spieler[1] - 16.0F, z->boden + 240.0F};
                text += "# kamera " + v3(kam) + "\n";
            }
            std::string fe;
            if (!sicherSchreiben(mb::verbinde(o, "mb7_vorschau.txt"), text, &fe)) { meldeFehler("MB7 Spiel: " + fe); }
            diag::detail("Knopftest: MB7 Spiel - gebaut nach " + o + ", " + g_app->figAnim.meldung);
        }));
        // Fotos der Vorschau durch die Skriptkamera an den Vergleichszeiten (Fenster "Figur animieren" zu, Griffe aus)
        if (a == "mit") {
            add(pruefe("MB7 Spiel: Schatten von a bei 3 s unter Bolzen 0 *head_eyes (CG_PlayerShadow mit Karten-GLA), nicht am Spawner", [] {
                const int ai = figur("mb7_a");
                if (ai < 0 || !g_app->actorAssets[static_cast<std::size_t>(ai)]) { return false; }
                const Actor& f = g_app->scene.actors[static_cast<std::size_t>(ai)];
                ActorDraw d;
                d.figur = ai;
                std::vector<BoneMatrix> w;
                float fw[16];
                if (!figurPose(f, *g_app->actorAssets[static_cast<std::size_t>(ai)], f.at(3000.0), 3000.0, d, w, fw)) { return false; }
                float o[3];
                schattenUrsprung(d, w, o);
                const std::array<float, 3> wurzel = knochenInKarte(w, fw, *d.anim, "cervical");   // die Augen liegen ueber dem Hals
                const std::array<float, 3> ort{d.pos[0], d.pos[1], d.pos[2]};
                const std::array<float, 3> sch{o[0], o[1], o[2]};
                diag::detail("Knopftest: MB7 Schatten - Ursprung " + v3(d.pos) + ", Schatten " + v3(o) + ", cervical " + v3(wurzel.data()));
                return ab2(sch, ort) > 20.0F && ab2(sch, wurzel) < 8.0F && o[2] > wurzel[2];
            }));
        }
        for (int ms = 0; ms <= (a == "eng" ? 9000 : 12000); ms += 500) {
            add(tu("MB7 Spiel: Vorschau durch die Kamera bei " + std::to_string(ms) + " ms (" + a + ")", [=] {
                g_app->figAnim.offen = false;
                g_app->throughCamera = true;
                g_app->playing = false;
                g_app->playMs = static_cast<double>(ms);
                g_app->mapDirty = true;
            }));
            add(pause(0.5));
            add(mission1::probeFoto("mb7_vorschau_" + a + "_" + std::to_string(ms)));
        }
        add(tu("MB7 Spiel: Kamera wieder frei", [] { g_app->throughCamera = false; }));
    }
    add(pruefe("MB7 Spiel: eng - c erscheint erst, wenn b per SET_ORIGIN weg ist (NPC_Begin wiederholt alle 500 ms)", [] {
        const auto b = g_app->ablauf.erscheint.find("mb7_b");
        const auto c = g_app->ablauf.erscheint.find("mb7_c");
        const double tb = b != g_app->ablauf.erscheint.end() ? b->second : -1.0;
        const double tc = c != g_app->ablauf.erscheint.end() ? c->second : -1.0;
        diag::detail("Knopftest: MB7 eng - b erscheint " + std::to_string(tb) + " ms, c " + std::to_string(tc) + " ms");
        // b gibt es vom Laden an (nicht in erscheint); b geht per SET_ORIGIN bei 6 s weg (a 1-4 s, b 4-6 s)
        return tb < 0.0 && tc >= 6000.0 && tc <= 6600.0;
    }));
    add(pruefe("MB7 Spiel: drei Fassungen gebaut (GLA, Skript, Mission, Vorschau)", [=] {
        int da = 0;
        for (const char* a : {"mit", "ohne", "eng"}) {
            const std::string o = mb::verbinde(z->ordner, a);
            da += !slurp(mb::verbinde(o, "models/players/_humanoid_mb7szene/_humanoid_mb7szene.gla")).empty() ? 1 : 0;
            da += !slurp(mb::verbinde(o, "zz_mb7szene.pk3")).empty() ? 1 : 0;
            da += !slurp(mb::verbinde(o, "mb7_vorschau.txt")).empty() ? 1 : 0;
        }
        return da == 9;
    }));
    return s;
}

// Die Bilder der Handbuchseite mission-animieren (BHED_HB_NUR=mb7): eine Figur mit Griffen und Dopesheet, Bauen, drei Figuren
std::vector<Schritt> handbuchSchritte() {
    auto z = std::make_shared<Zustand>();
    std::vector<Schritt> s;
    const auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    aufbau(s, z);
    add(tu("MB7 Handbuch: Kamera auf die Figuren, a animieren (Wurzelweg, Hand mit IK)", [=] {
        g_app->throughCamera = false;
        g_app->cam.pos[0] = z->spieler[0] - 60.0F;
        g_app->cam.pos[1] = z->spieler[1] - 140.0F;
        g_app->cam.pos[2] = z->boden + 150.0F;
        const float ziel[3] = {z->spieler[0] + 130.0F, z->spieler[1] - 30.0F, z->boden + 30.0F};
        aimAngles(g_app->cam.pos, ziel, g_app->cam.angles);
        g_app->playMs = 1000.0;
        (void)figurAnimStarten("mb7_a");
        (void)figurAnimEinstellen([](modell::FigurAnim& a) { a.sendeSignal = "mb7_los"; });
        const float null[3] = {0.0F, 0.0F, 0.0F};
        const float vor[3] = {120.0F, 0.0F, 0.0F};
        (void)figurAnimBeckenWeg("mb7_a", 0, null);
        (void)figurAnimBeckenWeg("mb7_a", 59, vor);
    }));
    add(warteBis("MB7 Handbuch: Szene", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 1200));
    add(zeitAuf("mb7_a", 30));
    add(pause(0.4));
    add(tu("MB7 Handbuch: rechte Hand IK, Ziel waehlen", [] {
        g_app->figAnim.griffWahl = figurAnimGriffSuchen("rhand");
        (void)figurAnimAktion(7);   // IK ein
        g_app->figAnim.griffWahl = figurAnimGriffSuchen(std::string(tr(Str::MbIkArmR)) + " IK");
    }));
    add(warteBis("MB7 Handbuch: Szene", [] { return g_app->camTrackValid; }, 600));
    add(pause(0.6));
    add(fensterImBild());
    add(pause(0.4));
    add(hilfe2::schrittFoto("h_mb7_posieren"));
    add(tu("MB7 Handbuch: b wartet auf a, c haelt b; Bauen aufklappen", [] {
        g_app->playMs = 0.0;
        (void)figurAnimStarten("mb7_b");
        (void)figurAnimEinstellen([](modell::FigurAnim& a) { a.warteSignal = "mb7_los"; });
        const float null[3] = {0.0F, 0.0F, 0.0F};
        const float seite[3] = {0.0F, -60.0F, 0.0F};
        (void)figurAnimBeckenWeg("mb7_b", 0, null);
        (void)figurAnimBeckenWeg("mb7_b", 59, seite);
        g_app->playMs = 3500.0;
        (void)figurAnimStarten("mb7_c");
    }));
    add(warteBis("MB7 Handbuch: Szene", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 1200));
    add(tu("MB7 Handbuch: c haelt b, Zeit 5,2 s", [] {
        g_app->figAnim.figur = "mb7_c";
        g_app->playMs = figurAnimStartMs("mb7_c") + 12.0 * 50.0;
        (void)figurAnimHalten(static_cast<int>(modell::IkArt::ArmLinks), "mb7_b", "rhand");
        g_app->figAnim.zeigeGlieder = true;
        g_app->playMs = 5200.0;
    }));
    add(warteBis("MB7 Handbuch: Szene", [] { return g_app->camTrackValid; }, 600));
    add(pause(0.6));
    add(fensterImBild());
    add(pause(0.4));
    add(hilfe2::schrittFoto("h_mb7_mehrere"));
    add(tu("MB7 Handbuch: Fuer das Spiel bauen (Fenster mit der Meldung)", [=] {
        g_app->figAnim.zeigeGlieder = false;
        g_app->figAnim.figur = "mb7_a";
        g_app->figAnim.offen = true;
        std::snprintf(g_app->figAnim.ordner, sizeof g_app->figAnim.ordner, "%s", mb::verbinde(z->ordner, "hb").c_str());
        (void)figurAnimBauen(g_app->figAnim.ordner, false);
    }));
    add(pause(0.6));
    add(fensterImBild());
    add(pause(0.4));
    add(hilfe2::schrittFoto("h_mb7_bauen"));
    return s;
}

// Bildzeit (BHED_MODELL_NUR=mb7zeit, allein, BHED_D3D_DEBUG=0): die Szene ohne Figurenanimation, dieselbe mit drei animierten
// Figuren beim Abspielen und beim Posieren mit Griffen, Pfaden, Dopesheet und Kurven (m1Messblock: Bild-CPU und Bildabstand)
std::vector<Schritt> zeitSchritte() {
    auto z = std::make_shared<Zustand>();
    std::vector<Schritt> s;
    const auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    aufbau(s, z);
    add(tu("MB7 Zeit: Kamera auf die Figuren", [=] {
        g_app->throughCamera = false;
        g_app->cam.pos[0] = z->spieler[0] - 40.0F;
        g_app->cam.pos[1] = z->spieler[1] - 30.0F;
        g_app->cam.pos[2] = z->boden + 190.0F;
        const float ziel[3] = {z->spieler[0] + 110.0F, z->spieler[1] - 30.0F, z->boden + 30.0F};
        aimAngles(g_app->cam.pos, ziel, g_app->cam.angles);
    }));
    add(m1Messblock("Mission", "drei Figuren ohne Figurenanimation, Abspielen", [] {
        g_app->playMs = 0.0;
        g_app->playing = true;
    }));
    add(tu("MB7 Zeit: drei Figuren animieren (Wurzelweg, IK, Halten)", [] {
        g_app->playing = false;
        g_app->playMs = 1000.0;
        (void)figurAnimStarten("mb7_a");
        const float null[3] = {0.0F, 0.0F, 0.0F};
        const float vor[3] = {120.0F, 0.0F, 0.0F};
        (void)figurAnimBeckenWeg("mb7_a", 0, null);
        (void)figurAnimBeckenWeg("mb7_a", 59, vor);
        g_app->playMs = 0.0;
        (void)figurAnimStarten("mb7_b");
        const float seite[3] = {0.0F, -60.0F, 0.0F};
        (void)figurAnimBeckenWeg("mb7_b", 0, null);
        (void)figurAnimBeckenWeg("mb7_b", 59, seite);
        g_app->playMs = 1000.0;
        (void)figurAnimStarten("mb7_c");
    }));
    add(warteBis("MB7 Zeit: Szene", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 1200));
    add(tu("MB7 Zeit: c haelt b", [] {
        g_app->figAnim.figur = "mb7_c";
        g_app->playMs = figurAnimStartMs("mb7_c") + 12.0 * 50.0;
        (void)figurAnimHalten(static_cast<int>(modell::IkArt::ArmLinks), "mb7_b", "rhand");
    }));
    add(warteBis("MB7 Zeit: Szene", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 1200));
    add(m1Messblock("Mission", "drei Figuren animiert, Abspielen", [] {
        g_app->figAnim.offen = false;
        g_app->playMs = 0.0;
        g_app->playing = true;
    }));
    add(m1Messblock("Mission", "Posieren: Griffe, Wurzelweg, Pfade der Glieder, Dopesheet und Kurven offen", [] {
        g_app->playing = false;
        g_app->figAnim.offen = true;
        g_app->figAnim.figur = "mb7_a";
        g_app->figAnim.zeigeGlieder = true;
        g_app->figAnim.kurvenZeigen = true;
        g_app->figAnim.griffWahl = figurAnimGriffSuchen("pelvis");
        g_app->playMs = figurAnimStartMs("mb7_a") + 30.0 * 50.0;
        g_app->mapDirty = true;
    }));
    add(pruefe("MB7 Zeit: Ueberlagerung gezeichnet", [] {
        diag::detail("Knopftest: MB7 Zeit - Griffe " + std::to_string(g_app->figAnim.griffeGezeichnet) + ", Pfadpunkte " + std::to_string(g_app->figAnim.pfadPunkte) +
                     ", Kurvenpunkte " + std::to_string(g_app->figAnim.kurvenPunkte) + ", Ueberlagerung " + std::to_string(g_app->figAnim.ueberlagerungMs) + " ms");
        return g_app->figAnim.griffeGezeichnet > 0 && g_app->figAnim.kurvenPunkte > 0;
    }));
    return s;
}

// Fund 2 (BHED_MODELL_NUR=mb7textur, BHED_MB7_LEER=<.bsp ohne Texturen>): eine Karte, deren Shader es nicht gibt. Der
// Renderer nimmt dann tr.defaultShader - eine Stufe mit "*default", 16x16 Grau 32 mit weissem Rand (R_FindShader ->
// defaultShader, ShaderForShaderNum, tr_shader.cpp:3500-3509, R_CreateDefaultImage tr_image.cpp:1282-1310 OpenJK). Die Vorschau
// zeichnete solche Flaechen gar nicht: ein schwarzes Bild. Geprueft im Mission-Tab vom Spielerstart aus (Blick auf die Wand).
std::vector<Schritt> texturSchritte() {
    std::vector<Schritt> s;
    const auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("MB7 Textur: Mission aus der Karte ohne Texturen", [] {
        const std::string wurzel = mb::verbinde(mb::laufOrdner(), "mb7_missionen");
        _putenv_s("BHED_MISSIONEN", wurzel.c_str());
        g_app->leftMode = 1;
        g_app->missionPickOpen = false;
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        const char* b = std::getenv("BHED_MB7_LEER");
        std::string f;
        if (b == nullptr || !neueMissionAnlegen(b, "mb7leer", "mb7leer", &f)) { meldeFehler("MB7 Textur: Mission: " + f); }
    }));
    add(warteBis("MB7 Textur: Karte geladen", [] { return g_app->map.path == "maps/mb7leer.bsp" && !g_app->geo.empty(); }, 4000));
    add(tu("MB7 Textur: Kamera am Spielerstart, Blick nach +X", [] {
        float o[3]{};
        for (const MapEntity& e : g_app->map.entities) {
            if (e.classname == "info_player_start") { (void)entityOrt(e, o); }
        }
        g_app->throughCamera = false;
        for (int k = 0; k < 3; ++k) { g_app->cam.pos[k] = o[k]; }
        g_app->cam.pos[2] += 40.0F;
        const float ziel[3] = {o[0] + 200.0F, o[1], o[2] + 40.0F};
        aimAngles(g_app->cam.pos, ziel, g_app->cam.angles);
        g_app->mapDirty = true;
    }));
    add(pause(1.0));
    add(mission1::probeFoto("mb7_leer"));
    add(pruefe("MB7 Textur: Flaechen ohne Shader mit tr.defaultShader (Grau 32, weisser Rand) statt gar nicht", [] {
        mission1::FotoBild b;
        if (!b.lesen("mb7_leer")) { return false; }
        int grau = 0;
        int weiss = 0;
        int alle = 0;
        for (int y = static_cast<int>(g_app->kartenAnsicht[1]) + 4; y < static_cast<int>(g_app->kartenAnsicht[3]) - 4; y += 2) {
            for (int x = static_cast<int>(g_app->kartenAnsicht[0]) + 4; x < static_cast<int>(g_app->kartenAnsicht[2]) - 4; x += 2) {
                const std::array<int, 3> p = b.bei(x, y);
                const int l = (p[0] + p[1] + p[2]) / 3;
                ++alle;
                grau += l >= 8 && l <= 90 && std::abs(p[0] - p[2]) < 12 ? 1 : 0;
                weiss += l >= 120 && std::abs(p[0] - p[2]) < 24 ? 1 : 0;
            }
        }
        diag::detail("Knopftest: MB7 Textur - " + std::to_string(alle) + " Punkte, grau " + std::to_string(grau) + ", hell " + std::to_string(weiss));
        return alle > 0 && grau * 2 > alle && weiss > 50;
    }));
    return s;
}

std::vector<Schritt> schritte(const std::string& n) {
    if (n == "mb7zeit") { return zeitSchritte(); }
    if (n == "mb7textur") { return texturSchritte(); }
    auto z = std::make_shared<Zustand>();
    if (n == "mb7") { return uiSchritte(z); }
    if (n == "mb7spiel") { return spielSchritte(z); }
    return {};
}

}  // namespace fa7
