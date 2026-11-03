// selbsttest_teil13d.inc - rc608 Teil 13d (Modus "teil13d"): Reiten wie im Spiel.
//
// Im Spiel gemessen (Movie Duels, SP/g13d, print ( $get( ... )$ ) je Serverbild): Ein NPC steigt per SET_VEHICLE auf
// einen per use gespawnten Gleiter, sitzt nach 550 ms (BOTH_VS_MOUNT_R, 40 % von 1300 ms, im naechsten Serverbild) in
// BOTH_VS_IDLE (Schwert aus: weggesteckt), dreht sich waehrend des Aufsteigens mit seiner Drehrate vom Fahrzeug
// zurueck zu seinem Blick, und der Gleiter folgt danach dem Blick des Reiters (SET_ANGLES, SET_DYAW). Ein Tauntaun
// steht in BOTH_VT_IDLE1. Karte: duel_tatooine_outpost mit seinen zwei NPC_Vehicle (swoop_cin), im Speicher umgebaut.
namespace teil13d {
inline std::size_t figur(const char* name) {
    for (std::size_t i = 0; i < g_app->scene.actors.size(); ++i) {
        if (g_app->scene.actors[i].name == name) { return i; }
    }
    return g_app->scene.actors.size();
}

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto gier = std::make_shared<float>(0.0F);
    add(tu("Teil 13d: duel_tatooine_outpost", [] {
        g_app->leftMode = 1;
        (void)loadMapFromArchive("maps/duel_tatooine_outpost.bsp");
    }));
    add(warteBis("Teil 13d: Karte bereit", [] { return g_app->map.path == "maps/duel_tatooine_outpost.bsp" && g_app->mapBereit; }, 4000));
    add(tu("Teil 13d: Gleiter per use, Reiter rechts daneben, Tauntaun", [gier] {
        int n = 0;
        MapEntity reiter;
        for (MapEntity& e : g_app->map.entities) {
            if (e.classname != "NPC_Vehicle") { continue; }
            if (n++ == 0) {
                e.keys.emplace_back("NPC_targetname", "t13d_bike");
                e.keys.emplace_back("targetname", "t13d_sp");
                e.targetname = "t13d_sp";
                float o[3] = {0.0F, 0.0F, 0.0F};
                (void)std::sscanf(e.origin.c_str(), "%f %f %f", &o[0], &o[1], &o[2]);
                if (const std::string* w = e.find("angle")) { *gier = static_cast<float>(std::atof(w->c_str())); }
                // rechts: AngleVectors(0 gier 0).right = (sin gier, -cos gier)
                const float g = *gier * 3.14159265F / 180.0F;
                char ort[96];
                std::snprintf(ort, sizeof(ort), "%.0f %.0f %.0f", static_cast<double>(o[0] + 92.0F * std::sin(g)),
                              static_cast<double>(o[1] - 92.0F * std::cos(g)), static_cast<double>(o[2]));
                reiter.classname = "NPC_spawner";
                reiter.origin = ort;
                reiter.keys = {{"classname", "NPC_spawner"}, {"origin", ort}, {"NPC_type", "jedi"}, {"NPC_targetname", "t13d_jr"},
                               {"angle", std::to_string(static_cast<int>(*gier))}};
            } else {
                for (auto& [k, v] : e.keys) {
                    if (k == "NPC_type") { v = "tauntaun"; }
                }
            }
        }
        g_app->map.entities.push_back(reiter);
        // Der Reiter schaut 90 Grad neben das Fahrzeug, steigt bei 1000 auf, schaut bei 4000 noch einmal 90 weiter und
        // steigt bei 6000 wieder ab (rc608 Teil 13e).
        const int g0 = static_cast<int>(*gier);
        teil9::laden(teil9::kKopf + "affect ( \"t13d_jr\", /*@AFFECT_TYPE*/ FLUSH )\r\n{\r\n"
                     "\tuse ( \"t13d_sp\" );\r\n"
                     "\tset ( /*@SET_TYPES*/ \"SET_BEHAVIOR_STATE\", \"BS_CINEMATIC\" );\r\n"
                     "\tset ( /*@SET_TYPES*/ \"SET_ANGLES\", \"0 " + std::to_string(g0 + 90) + " 0\" );\r\n"
                     "\twait ( 1000.000 );\r\n"
                     "\tset ( /*@SET_TYPES*/ \"SET_VEHICLE\", \"t13d_bike\" );\r\n"
                     "\twait ( 3000.000 );\r\n"
                     "\tset ( /*@SET_TYPES*/ \"SET_ANGLES\", \"0 " + std::to_string(g0 + 180) + " 0\" );\r\n"
                     "\twait ( 2000.000 );\r\n"
                     "\tset ( /*@SET_TYPES*/ \"SET_VEHICLE\", \"t13d_bike\" );\r\n"
                     "\twait ( 1000.000 );\r\n}\r\n", "Teil 13d");
    }));
    add(warteBis("Teil 13d: Szene", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 4000));
    add(pruefe("Teil 13d: use auf einen Fahrzeug-Spawner spawnt das Fahrzeug, SET_VEHICLE laesst den Reiter aufsteigen", [] {
        const std::size_t b = figur("t13d_bike");
        const std::size_t r = figur("t13d_jr");
        const bool da = b < g_app->scene.actors.size() && r < g_app->scene.actors.size();
        diag::detail("Knopftest: Teil 13d - Gleiter " + std::to_string(b < g_app->scene.actors.size()) + ", Ritte " +
                     (r < g_app->scene.actors.size() ? std::to_string(g_app->scene.actors[r].ritte.size()) : std::string("-")) +
                     (da && !g_app->scene.actors[r].ritte.empty() ? ", ab " + std::to_string(g_app->scene.actors[r].ritte[0].abMs) : std::string()));
        return da && g_app->scene.actors[r].ritte.size() == 1U && g_app->scene.actors[r].ritte[0].fahrzeug == b;
    }));
    add(pruefe("Teil 13d: Aufsteigen 550 ms (40 % von BOTH_VS_MOUNT_R, naechstes Serverbild), dann BOTH_VS_IDLE mit weggestecktem Schwert", [] {
        const std::size_t r = figur("t13d_jr");
        if (r >= g_app->scene.actors.size() || g_app->scene.actors[r].ritte.empty()) { return false; }
        const Actor& a = g_app->scene.actors[r];
        const double ab = a.ritte[0].abMs;
        const auto anim = [&](double t) {
            ActorState st = a.at(t);
            ActorDraw d;
            reitenAnwenden(a, t, st, d);
            return st.animation + "/" + st.waffe;
        };
        const std::string vor = anim(ab + 540.0);
        const std::string nach = anim(ab + 560.0);
        diag::detail("Knopftest: Teil 13d - bei +540 " + vor + ", bei +560 " + nach);
        return vor.rfind("BOTH_VS_MOUNT_R/", 0) == 0 && nach == "BOTH_VS_IDLE/WP_NONE";
    }));
    add(pruefe("Teil 13d: Waehrend des Aufsteigens dreht der Reiter vom Fahrzeug zurueck, danach folgt das Fahrzeug seinem Blick", [gier] {
        const std::size_t r = figur("t13d_jr");
        const std::size_t b = figur("t13d_bike");
        if (r >= g_app->scene.actors.size() || b >= g_app->scene.actors.size() || g_app->scene.actors[r].ritte.empty()) { return false; }
        const Actor& a = g_app->scene.actors[r];
        const double ab = a.ritte[0].abMs;
        const auto reiterGier = [&](double t) {
            ActorState st = a.at(t);
            ActorDraw d;
            reitenAnwenden(a, t, st, d);
            return shortestAngleDelta(*gier, st.angles[1]);
        };
        const auto bikeGier = [&](double t) {
            ActorState sv = g_app->scene.actors[b].at(t);
            fahrzeugLenken(b, t, sv);
            return shortestAngleDelta(*gier, sv.angles[1]);
        };
        const float rate = a.festeDrehrate >= 0.0F ? a.festeDrehrate : a.startDrehrate;
        const float r0 = reiterGier(ab + 10.0);
        const float r1 = reiterGier(ab + 60.0);
        const float bWaehrend = bikeGier(ab + 500.0);
        const float bDanach = bikeGier(ab + 600.0);
        const float bSpaet = bikeGier(4000.0 + 1000.0);
        diag::detail("Knopftest: Teil 13d - Reiter " + std::to_string(r0) + " / " + std::to_string(r1) + " (Drehrate " + std::to_string(rate) +
                     "), Gleiter " + std::to_string(bWaehrend) + " / " + std::to_string(bDanach) + " / " + std::to_string(bSpaet));
        return std::fabs(r0) < 0.01F && std::fabs(r1 - std::min(90.0F, rate * 0.05F)) < 0.01F && std::fabs(bWaehrend) < 0.01F &&
               std::fabs(bDanach - 90.0F) < 0.01F && std::fabs(std::fabs(bSpaet) - 180.0F) < 0.01F;
    }));
    add(pruefe("Teil 13e: Nach dem Absteigen bleibt die Waffe WP_NONE (Eject gibt keine zurueck)", [] {
        const std::size_t r = figur("t13d_jr");
        if (r >= g_app->scene.actors.size() || g_app->scene.actors[r].ritte.empty()) { return false; }
        const Actor& a = g_app->scene.actors[r];
        const double bis = a.ritte[0].bisMs;
        if (bis < 0.0) {
            diag::detail("Knopftest: Teil 13e - kein Absteigen");
            return false;
        }
        ActorState st = a.at(bis + 500.0);
        ActorDraw d;
        reitenAnwenden(a, bis + 500.0, st, d);
        diag::detail("Knopftest: Teil 13e - abgestiegen bei " + std::to_string(bis) + ", danach " + st.waffe);
        return st.waffe == "WP_NONE";
    }));
    // Im Spiel gesehen (SP/g13d): das Tier steht ab dem ersten Bild nach dem Spawn in BOTH_VT_IDLE1. Geprueft wird das
    // Bild der .gla, das die Ansicht fuer das Tier wirklich zeichnet (nicht der Zustand, den fahrzeugLenken setzt).
    auto tier = std::make_shared<std::size_t>(0);
    add(tu("Teil 13d: Ansicht auf das Tier bei 2000 ms", [tier] {
        *tier = g_app->scene.actors.size();
        for (std::size_t i = 0; i < g_app->scene.actors.size(); ++i) {
            const Actor& v = g_app->scene.actors[i];
            if (v.fahrzeug.da && v.fahrzeug.typ == "VH_ANIMAL") { *tier = i; }
        }
        g_app->playMs = 2000.0;
        g_app->playing = false;
        g_app->followCam = false;
        g_app->throughCamera = false;
        if (*tier < g_app->scene.actors.size()) {
            const ActorState st = g_app->scene.actors[*tier].at(2000.0);
            g_app->cam.pos[0] = st.pos[0] + 160.0F;
            g_app->cam.pos[1] = st.pos[1];
            g_app->cam.pos[2] = st.pos[2] + 40.0F;
            g_app->cam.angles[0] = 8.0F;
            g_app->cam.angles[1] = 180.0F;
            g_app->cam.angles[2] = 0.0F;
        }
        g_app->figurBildGezeichnet.assign(g_app->scene.actors.size(), -1);
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(pruefe("Teil 13d: Ein Tauntaun steht in BOTH_VT_IDLE1 (AnimateVehicle): das gezeichnete Bild liegt in diesem Abschnitt", [tier] {
        const std::size_t i = *tier;
        if (i >= g_app->scene.actors.size() || i >= g_app->actorAssets.size() || !g_app->actorAssets[i] || g_app->actorAssets[i]->skeleton == nullptr) {
            diag::detail("Knopftest: Teil 13d - kein Tier mit Skelett in der Szene");
            return false;
        }
        const int bild = i < g_app->figurBildGezeichnet.size() ? g_app->figurBildGezeichnet[i] : -1;
        for (const AnimEntry& e : g_app->actorAssets[i]->skeleton->sections) {
            if (e.name != "BOTH_VT_IDLE1") { continue; }
            diag::detail("Knopftest: Teil 13d - " + g_app->scene.actors[i].name + ": Bild " + std::to_string(bild) + ", BOTH_VT_IDLE1 " +
                         std::to_string(e.firstFrame) + " .. " + std::to_string(e.firstFrame + e.numFrames - 1));
            return e.numFrames > 0 && bild >= e.firstFrame && bild < e.firstFrame + e.numFrames;
        }
        diag::detail("Knopftest: Teil 13d - BOTH_VT_IDLE1 fehlt im Skelett des Tiers");
        return false;
    }));
    return s;
}
}  // namespace teil13d
