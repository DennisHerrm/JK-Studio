// selbsttest_teil6.inc - rc608 Teil 6 (Modus "teil6"): Portale und Schmerz im laufenden Programm.
//
// 1. Karte duel_mustafar: die Tueren schalten ihre Portale, hinter der geschlossenen Tuer ist nichts
//    zu zeichnen (R_RecursiveWorldNode, areamask); r_DynamicGlow je Regelwerk.
// 2. Mission auf md_am_sith: ein Wachgeschuetz trifft eine Figur. Unter Movie Duels zuckt sie nach dem
//    Trefferort (G_MissileImpacted, 2 s Pause, Torso 400 ms), unter OpenJK nach NPC_ChoosePainAnimation.
//    Die Figur steht trotzdem: die Bolzen schieben nicht (DAMAGE_NO_KNOCKBACK).
// 3. Eine Explosion (misc_model_breakable) stoesst die Figur (G_RadiusDamage, G_ApplyKnockback).
std::vector<Schritt> teil6Schritte() {
    namespace gp = geschuetze_probe;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<pruefung3::Zustand>();
    auto regelVorher = std::make_shared<int>(0);
    add(tu("Teil 6: Ausgangslage", [=] {
        for (const std::string& o : g_app->settings.gamePaths) {
            std::error_code ec;
            if (z->mdArchiv.empty() && std::filesystem::is_regular_file(::bhed::utf8Pfad(o + "\\assets1.pk3"), ec)) {
                z->mdArchiv = o + "\\assets1.pk3";
            }
        }
        *regelVorher = g_app->settings.regelwerk;
        g_app->leftMode = 1;
        g_app->missionPickOpen = false;
    }));
    add(pruefe("Teil 6: r_DynamicGlow - Movie Duels an", [] {
        g_app->settings.regelwerk = kRegelwerkMovieDuels;
        regelwerkAnwendenJetzt();
        return g_app->showGlow;
    }));
    add(pruefe("Teil 6: r_DynamicGlow - OpenJK aus (tr_init.cpp:1525)", [] {
        g_app->settings.regelwerk = kRegelwerkOpenJK;
        regelwerkAnwendenJetzt();
        return !g_app->showGlow;
    }));
    add(tu("Teil 6: zurueck zu Movie Duels, Karte duel_mustafar", [=] {
        g_app->settings.regelwerk = kRegelwerkMovieDuels;
        regelwerkAnwendenJetzt();
        mission1::ungespeicherteEntitiesVergessen();
        missionenAusArchiv(z->mdArchiv, "duel_mustafar");
    }));
    add(warteBis("Teil 6: duel_mustafar geladen", [] { return g_app->map.path == "maps/duel_mustafar.bsp" && g_app->camTrackValid; }, 4000));
    add(pause(0.5));
    add(pruefe("Teil 7: Weather/duel_mustafar.cfg gilt unter Movie Duels (r_weather 3: eine Wolke snowflake2)", [] {
        return regelwerkMovieDuels() && wetterWolkenMitBild("gfx/effects/snowflake2") == 1;
    }));
    add(pruefe("Teil 7: r_picmip halbiert je Stufe, Grenze der Grafikkarte bleibt", [] {
        image::Image im;
        im.width = 512;
        im.height = 256;
        const int merk = g_app->settings.bildPicmip;
        g_app->settings.bildPicmip = 0;
        const int s0 = bildGrenzeFuer(im);
        g_app->settings.bildPicmip = 2;
        const int s2 = bildGrenzeFuer(im);
        g_app->settings.bildPicmip = merk;
        return s0 == 512 && s2 == 128;
    }));
    add(pruefe("Teil 6: die Tueren schalten Portale, Gebiet 9 ist vom Start aus nicht zu zeichnen", [] {
        const float start[3] = {-1024.0F, 7320.0F, 24.0F};
        areaPortaleJetzt(start);
        const std::vector<char> m = flaechenHinterPortalen(g_app->geo, g_app->areaPortale, punktGebiet(g_app->geo, start));
        std::size_t verdeckt = 0;
        for (const char c : m) { verdeckt += c != 0 ? 1U : 0U; }
        diag::detail("Knopftest: Teil 6 - " + std::to_string(g_app->portalEnts.size()) + " Portal-Entities, Gebiete " + std::to_string(g_app->areaPortale.gebiete()) +
                     ", vom Start verdeckt " + std::to_string(verdeckt) + " von " + std::to_string(m.size()));
        return !g_app->portalEnts.empty() && g_app->areaPortale.gebiete() > 1 && verdeckt > 0U && m.size() > 1193U && m[1193] != 0;
    }));

    add(tu("Teil 6: Grundkarte md_am_sith", [=] {
        mission1::ungespeicherteEntitiesVergessen();
        missionenAusArchiv(z->mdArchiv, "md_am_sith");
    }));
    add(warteBis("Teil 6: Grundkarte geladen", [] { return g_app->map.path == "maps/md_am_sith.bsp" && g_app->camTrackValid && jk2probe::figurenGeladen(); }, 4000));
    add(tu("Teil 6: begehbaren Ort merken", [=] {
        prepareScene();
        for (std::size_t ai = 0; ai < g_app->scene.actors.size() && z->typ.empty(); ++ai) {
            const Actor& a = g_app->scene.actors[ai];
            for (const ActorStep& st : a.steps) {
                if (z->typ.empty() && st.kind == ActorStep::Kind::WalkTo && !a.npcType.empty() && ai < g_app->actorAssets.size() && g_app->actorAssets[ai] && !g_app->actorAssets[ai]->model.empty()) {
                    for (int k = 0; k < 3; ++k) { z->ort[k] = st.from[k]; }
                    z->typ = a.npcType;
                }
            }
        }
    }));
    const auto treffer = [&](const char* name) {
        add(tu(std::string("Teil 6: Mission ") + name + " - Wachgeschuetz neben der Figur", [=] {
            pruefung3::szeneBauen(*z, std::string("t6") + name, {{"t6_ziel", 0.0F, 0.0F}}, gp::ziel("t6_ziel", 2000));
            const float o[3] = {z->ort[0] + 96.0F, z->ort[1] - 96.0F, z->ort[2] - 24.0F};
            (void)gp::neue("misc_sentry_turret", o, {{"angles", "0 135 0"}});
            g_app->camTrackValid = false;
            g_app->mapDirty = true;
        }));
        add(warteBis(std::string("Teil 6: Szene ") + name, [=] { return !z->bereit || (g_app->camTrackValid && pruefung3::figur("t6_ziel") >= 0 && jk2probe::figurenGeladen()); }, 3000));
        add(pause(0.5));
    };
    treffer("md");
    add(pruefe("Teil 6: Movie Duels - G_MissileImpacted: Animation nach Trefferort, Torso 400, 2 s Pause, kein Stoss", [] {
        prepareScene();
        const int fi = pruefung3::figur("t6_ziel");
        if (fi < 0) { return false; }
        const Actor& a = g_app->scene.actors[static_cast<std::size_t>(fi)];
        std::size_t zahl = 0;
        double letzte = -1.0e9;
        bool abstand = true;
        bool torso = true;
        for (const SchmerzAufruf& sch : g_app->schmerzen) {
            if (sch.figur != a.name || sch.anim.empty() || sch.torsoMs <= 0.0) { continue; }
            ++zahl;
            abstand = abstand && sch.ms - letzte >= 2000.0 - 1e-6;
            torso = torso && sch.torsoMs == 400.0;
            letzte = sch.ms;
        }
        bool imBild = false;
        for (const SchmerzAufruf& sch : g_app->schmerzen) {
            if (sch.figur == a.name && !sch.anim.empty() && sch.torsoMs > 0.0) {
                const ActorState st = a.at(sch.ms + 100.0);
                imBild = imBild || st.animation == sch.anim;
            }
        }
        diag::detail("Knopftest: Teil 6 - " + std::to_string(zahl) + " Zuckungen, Stoesse " + std::to_string(g_app->stoesseNeu.size()));
        return zahl > 0U && abstand && torso && imBild && g_app->stoesseNeu.empty();
    }));
    add(tu("Teil 6: OpenJK", [] {
        g_app->settings.regelwerk = kRegelwerkOpenJK;
        regelwerkAnwendenJetzt();
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    treffer("oj");
    add(pruefe("Teil 6: OpenJK - NPC_ChoosePainAnimation: die ganze Animation, painDebounceTime", [] {
        prepareScene();
        const int fi = pruefung3::figur("t6_ziel");
        if (fi < 0) { return false; }
        const Actor& a = g_app->scene.actors[static_cast<std::size_t>(fi)];
        std::size_t zahl = 0;
        bool ganz = true;
        for (const SchmerzAufruf& sch : g_app->schmerzen) {
            if (sch.figur != a.name || sch.anim.empty()) { continue; }
            ++zahl;
            ganz = ganz && sch.torsoMs == 0.0;
        }
        diag::detail("Knopftest: Teil 6 OpenJK - " + std::to_string(zahl) + " Schmerzanimationen");
        return zahl > 0U && ganz;
    }));
    add(tu("Teil 6: Mission Explosion - fx_target_beam sprengt eine Kiste neben der Figur", [=] {
        g_app->settings.regelwerk = kRegelwerkMovieDuels;
        regelwerkAnwendenJetzt();
        pruefung3::szeneBauen(*z, "t6boom", {{"t6_ziel", 0.0F, 0.0F}}, gp::ziel("t6_ziel", 2000));
        const float k[3] = {z->ort[0] + 64.0F, z->ort[1], z->ort[2] - 24.0F};
        (void)gp::neue("misc_model_breakable", k, {{"model", "models/map_objects/imperial/crate_banded.md3"}, {"health", "1"}, {"splashDamage", "100"}, {"splashRadius", "256"}, {"targetname", "t6kiste"}});
        const float q[3] = {z->ort[0] + 64.0F, z->ort[1] + 128.0F, z->ort[2]};
        (void)gp::neue("fx_target_beam", q, {{"target", "t6strahlziel"}, {"damage", "50"}, {"wait", "1000"}, {"fxFile", "env/hevil_bolt"}});
        const float ziel[3] = {k[0], k[1], k[2] + 8.0F};
        (void)gp::neue("info_null", ziel, {{"targetname", "t6strahlziel"}});
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    add(warteBis("Teil 6: Szene Explosion", [=] { return !z->bereit || (g_app->camTrackValid && pruefung3::figur("t6_ziel") >= 0 && jk2probe::figurenGeladen()); }, 3000));
    add(pause(0.5));
    add(pruefe("Teil 6: G_RadiusDamage - die Figur wird vom Knall weggestossen und bleibt am neuen Ort", [] {
        prepareScene();
        const int fi = pruefung3::figur("t6_ziel");
        if (fi < 0) { return false; }
        const Actor& a = g_app->scene.actors[static_cast<std::size_t>(fi)];
        const auto it = g_app->stoesseNeu.find(a.name);
        if (it == g_app->stoesseNeu.end() || it->second.empty()) {
            diag::detail("Knopftest: Teil 6 - kein Stoss");
            return false;
        }
        const double t = it->second.front().ms;
        const ActorState vor = a.at(t - 60.0);
        const ActorState nach = a.at(t + 5000.0);
        const float dx = nach.pos[0] - vor.pos[0];
        const float dy = nach.pos[1] - vor.pos[1];
        diag::detail("Knopftest: Teil 6 - Stoss bei " + std::to_string(t) + " ms, Tempo " + std::to_string(it->second.front().tempo[0]) + " " + std::to_string(it->second.front().tempo[1]) + " " +
                     std::to_string(it->second.front().tempo[2]) + ", Weg " + std::to_string(dx) + " " + std::to_string(dy));
        return std::sqrt(dx * dx + dy * dy) > 1.0F && dx < 0.0F;   // die Kiste steht bei +x
    }));
    add(tu("Teil 6: Regelwerk zurueck", [=] {
        g_app->settings.regelwerk = *regelVorher;
        regelwerkAnwendenJetzt();
    }));
    return s;
}
