// selbsttest_teil10.inc - rc608 Teil 10 (Modus "teil10").
//
// 1. Das Schwert des Spielers aus den Cvars (G_SetSabersFromCVars): g_saber "single_1", alle Klingen g_saber_color
//    "yellow" - vorher wuerfelte die Vorschau die Farbe wie bei einer Figur ohne Angabe.
// 2. PM_AdjustStandAnimForSlope auch fuer den Spieler (dritte Person): am Statuensockel von duel_jeditemple, Start wie
//    g9_jtb aus Teil 9b (-1250 60 -1150, Blick 90), wo das Spiel ihn rund 95 Bildpunkte tiefer zeigte.
// 3. Eine Lichtstufe mit GL_ONE GL_ZERO hinter einer Bildstufe ersetzt die Farbe (lichtErsetztBildstufen): die
//    Zuschauer auf duel_geonosis_arena (textures/md_ga/geonosian1) waren Bild mal Bild mal Licht, fast schwarz.
namespace teil10 {
inline void spielerStart(float x, float y, float z, const char* winkel) {
    int start = -1;
    for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
        if (g_app->map.entities[i].classname == "info_player_start") {
            start = static_cast<int>(i);
            break;
        }
    }
    const float o[3] = {x, y, z};
    if (start < 0) {
        start = neueEntityAn("info_player_start", o);
    } else {
        char t[96];
        std::snprintf(t, sizeof(t), "%g %g %g", static_cast<double>(x), static_cast<double>(y), static_cast<double>(z));
        (void)entityKeySetzen(start, "origin", t);
    }
    if (start >= 0) { (void)entityKeySetzen(start, "angle", winkel); }
}
}  // namespace teil10

std::vector<Schritt> teil10Schritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("Teil 10: duel_jeditemple", [] {
        g_app->leftMode = 1;
        (void)loadMapFromArchive("maps/duel_jeditemple.bsp");
    }));
    add(warteBis("Teil 10: Karte bereit", [] { return g_app->map.path == "maps/duel_jeditemple.bsp" && g_app->mapBereit; }, 4000));
    add(tu("Teil 10: Spieler am Statuensockel mit brennender Klinge", [] {
        teil10::spielerStart(-1250.0F, 60.0F, -1150.0F, "90");
        teil9::laden(teil9::kKopf + "affect ( \"player\", /*@AFFECT_TYPE*/ FLUSH )\r\n{\r\n"
                     "\tset ( /*@SET_TYPES*/ \"SET_WEAPON\", \"WP_SABER\" );\r\n"
                     "\tset ( /*@SET_TYPES*/ \"SET_SABERACTIVE\", \"true\" );\r\n"
                     "\twait ( 3000.000 );\r\n}\r\n", "Teil 10");
    }));
    add(warteBis("Teil 10: Spieler geladen", [] {
        return g_app->camTrackValid && jk2probe::figurenGeladen() && teil9::spieler() < g_app->scene.actors.size();
    }, 3000));
    add(pruefe("Teil 10: Schwert des Spielers aus g_saber und g_saber_color (single_1, alle Klingen gelb)", [] {
        const Actor& a = g_app->scene.actors[teil9::spieler()];
        const ActorState st = a.at(2500.0);
        const SaberZustand& sz = st.saber[0];
        const std::string art = sz.art >= 0 && static_cast<std::size_t>(sz.art) < a.saberArten.size()
                                    ? a.saberArten[static_cast<std::size_t>(sz.art)].name
                                    : std::string("-");
        std::string farben;
        bool gelb = sz.numBlades > 0;
        for (int b = 0; b < sz.numBlades && b < kMaxKlingen; ++b) {
            farben += " " + sz.klinge[static_cast<std::size_t>(b)].farbe;
            gelb = gelb && sz.klinge[static_cast<std::size_t>(b)].farbe == "yellow";
        }
        diag::detail("Knopftest: Teil 10 - Spieler Schwert " + art + ", Klingen " + std::to_string(sz.numBlades) + ":" + farben + ", an " +
                     std::to_string(st.saberActive));
        return art == "single_1" && gelb && st.saberActive;
    }));
    add(pruefe("Teil 10: Spieler am Hang: PM_AdjustStandAnimForSlope waehlt die Hang-Beine (LEGS_*)", [] {
        const Actor& a = g_app->scene.actors[teil9::spieler()];
        const ActorState st = a.at(2500.0);
        std::string z = "Knopftest: Teil 10 - Spieler bei " + std::to_string(st.pos[0]) + " " + std::to_string(st.pos[1]) + " " +
                        std::to_string(st.pos[2]) + ", Beine " + st.animation + ", Rumpf " + st.torsoAnimation + ", Hangwechsel";
        for (const auto& [t, b] : a.hangBeine) { z += " " + std::to_string(static_cast<int>(t)) + ":" + (b.empty() ? "-" : b); }
        diag::detail(z);
        return st.animation.rfind("LEGS_", 0) == 0;
    }));
    add(tu("Teil 10: duel_geonosis_arena", [] { (void)loadMapFromArchive("maps/duel_geonosis_arena.bsp"); }));
    add(warteBis("Teil 10: Arena bereit", [] { return g_app->map.path == "maps/duel_geonosis_arena.bsp" && g_app->mapBereit; }, 4000));
    add(pruefe("Teil 10: Lichtstufe GL_ONE GL_ZERO ersetzt: keine Zusatzstufe bis zum Bild danach (geonosian1, galler1)", [] {
        int faelle = 0;
        int richtig = 0;
        std::string z = "Knopftest: Teil 10 -";
        for (std::size_t i = 0; i < g_app->geo.shaders.size() && i < g_app->textures.byShader.size(); ++i) {
            std::string n = g_app->geo.shaders[i].name;
            for (char& c : n) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
            const auto it = g_app->shaderMap.find(n);
            if (it == g_app->shaderMap.end()) { continue; }
            const int k = lichtErsetztBildstufen(it->second);
            if (k < 0) { continue; }
            std::size_t danach = 0;
            for (std::size_t j = static_cast<std::size_t>(k) + 1; j < it->second.stages.size(); ++j) {
                if (!it->second.stages[j].image.empty() && !it->second.stages[j].lightmap) { ++danach; }
            }
            const TextureSet::Tex& t = g_app->textures.byShader[i];
            ++faelle;
            if (t.extraStages.size() == danach && t.lightmapStufe) { ++richtig; }
            z += " " + n + " Zusatz " + std::to_string(t.extraStages.size()) + " soll " + std::to_string(danach) + " Licht " +
                 std::to_string(t.lightmapStufe);
        }
        diag::detail(z);
        return faelle >= 2 && richtig == faelle;
    }));
    return s;
}
