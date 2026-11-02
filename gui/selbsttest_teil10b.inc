// selbsttest_teil10b.inc - rc608 Teil 10b (Modus "teil10b").
//
// 1. Schwertlicht unter Movie Duels: cg_saberLightRadius und cg_saberLightIntensity vervielfachen Radius und Farbe des
//    Lichts (CG_DoSaberLight der installierten MovieDuels-gamex86_64.dll, im Spiel gemessen), ein Wert <= 0 gilt als 1.
//    Gleicher Zeitpunkt, also gleicher Zufall: das Verhaeltnis ist genau der Wert.
//    Eine Klinge hat ihr eigenes Licht (CG_DoSaber mit doLight), das gemischte (CG_DoSaberLight) erst ab drei Klingen.
// 2. cg_turnAnims: Die Drehanimation auf der Huefte laeuft, solange sich die Beine drehen (CG_PlayerLegsYawFromMovement
//    holt den Blick Bild fuer Bild ein) - nach einem SET_ANGLES noch, wenn die Gier schon steht.
// 3. Zwischenspeicher der Figurtexturen: Eine Figurflaeche, deren Bild nur ueber ein Shaderskript zu finden ist
//    (md_10thbrother, "models/players/10thbrother/extras_shiny" -> map models/players/10thbrother/extras), legt das Bild
//    unter dem Namen der gelesenen Datei ab. Eine Bildstufe "map <Shadername>" findet dann wie R_FindImageFile nichts.
namespace teil10b {
struct Licht {
    float radius = 0.0F;
    float farbe[3] = {0.0F, 0.0F, 0.0F};
    bool da = false;
    int zahl = 0;
};
inline Licht g_licht[3];

inline void schwertlicht(float intensitaet, float radius) {
    g_app->settings.cgSaberLightIntensity = intensitaet;
    g_app->settings.cgSaberLightRadius = radius;
    teil9::stelle(2500.0);
    g_app->schwertLichter.clear();
    g_app->mapDirty = true;
}

inline bool merke(int i) {
    if (g_app->schwertLichter.empty()) {
        g_app->mapDirty = true;
        return false;
    }
    // das Licht am naechsten beim Spieler (die zweite Figur kann ein eigenes haben)
    const ActorState st = g_app->scene.actors[teil9::spieler()].at(2500.0);
    const EffectLight* naechstes = nullptr;
    float best = 1.0e30F;
    for (const EffectLight& l : g_app->schwertLichter) {
        const float dx = l.origin[0] - st.pos[0];
        const float dy = l.origin[1] - st.pos[1];
        const float dz = l.origin[2] - st.pos[2];
        if (dx * dx + dy * dy + dz * dz < best) {
            best = dx * dx + dy * dy + dz * dz;
            naechstes = &l;
        }
    }
    const EffectLight& el = *naechstes;
    g_licht[i].zahl = static_cast<int>(g_app->schwertLichter.size());
    g_licht[i].radius = el.radius;
    for (int k = 0; k < 3; ++k) { g_licht[i].farbe[k] = el.color[k]; }
    g_licht[i].da = true;
    return true;
}

inline std::string text(const Licht& l) {
    char t[96];
    std::snprintf(t, sizeof(t), "r %.3f rgb %.3f %.3f %.3f", static_cast<double>(l.radius), static_cast<double>(l.farbe[0]),
                  static_cast<double>(l.farbe[1]), static_cast<double>(l.farbe[2]));
    return t;
}

inline int figur(const char* name) {
    for (std::size_t i = 0; i < g_app->scene.actors.size(); ++i) {
        std::string n = g_app->scene.actors[i].name;
        for (char& c : n) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        if (n == name) { return static_cast<int>(i); }
    }
    return -1;
}
}  // namespace teil10b

std::vector<Schritt> teil10bSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("Teil 10b: duel_jeditemple", [] {
        g_app->leftMode = 1;
        (void)loadMapFromArchive("maps/duel_jeditemple.bsp");
    }));
    add(warteBis("Teil 10b: Karte bereit", [] { return g_app->map.path == "maps/duel_jeditemple.bsp" && g_app->mapBereit; }, 4000));
    add(tu("Teil 10b: Spieler mit brennender Klinge, daneben md_10thbrother", [] {
        teil10::spielerStart(-1250.0F, 60.0F, -1150.0F, "90");
        const float o[3] = {-1250.0F, 160.0F, -1150.0F};
        const int n = neueEntityAn("NPC_spawner", o);
        if (n >= 0) {
            (void)entityKeySetzen(n, "NPC_targetname", "bruder");
            (void)entityKeySetzen(n, "NPC_type", "md_10thbrother");
            (void)entityAufBoden(n);
        }
        teil9::laden(teil9::kKopf + "affect ( \"player\", /*@AFFECT_TYPE*/ FLUSH )\r\n{\r\n"
                     "\tset ( /*@SET_TYPES*/ \"SET_WEAPON\", \"WP_SABER\" );\r\n"
                     "\tset ( /*@SET_TYPES*/ \"SET_SABERACTIVE\", \"true\" );\r\n"
                     "\twait ( 3000.000 );\r\n}\r\n"
                     "affect ( \"bruder\", /*@AFFECT_TYPE*/ FLUSH )\r\n{\r\n"
                     "\tset ( /*@SET_TYPES*/ \"SET_BEHAVIORSTATE\", \"BS_CINEMATIC\" );\r\n"
                     "\twait ( 1000.000 );\r\n"
                     "\tset ( /*@SET_TYPES*/ \"SET_ANGLES\", \"0 120 0\" );\r\n"
                     "\twait ( 3000.000 );\r\n}\r\n", "Teil 10b");
    }));
    add(warteBis("Teil 10b: Figuren geladen", [] {
        return g_app->camTrackValid && jk2probe::figurenGeladen() && teil9::spieler() < g_app->scene.actors.size() &&
               teil10b::figur("bruder") >= 0;
    }, 3000));
    add(tu("Teil 10b: Schwertlicht 1 / 1", [] { teil10b::schwertlicht(1.0F, 1.0F); }));
    add(warteBis("Teil 10b: Licht bei 1 / 1", [] { return teil10b::merke(0); }, 600));
    add(tu("Teil 10b: Schwertlicht 1,2 / 1,5", [] { teil10b::schwertlicht(1.2F, 1.5F); }));
    add(warteBis("Teil 10b: Licht bei 1,2 / 1,5", [] { return teil10b::merke(1); }, 600));
    add(tu("Teil 10b: Schwertlicht 0 / 0", [] { teil10b::schwertlicht(0.0F, 0.0F); }));
    add(warteBis("Teil 10b: Licht bei 0 / 0", [] { return teil10b::merke(2); }, 600));
    add(pruefe("Teil 10b: Movie Duels: cg_saberLightRadius 1,5 und cg_saberLightIntensity 1,2 vervielfachen, 0 gilt als 1", [] {
        using teil10b::g_licht;
        g_app->settings.cgSaberLightIntensity = 1.070312F;
        g_app->settings.cgSaberLightRadius = 0.703125F;
        diag::detail("Knopftest: Teil 10b - Regelwerk Movie Duels " + std::to_string(regelwerkMovieDuels()) + ", 1/1: " +
                     teil10b::text(g_licht[0]) + ", 1,2/1,5: " + teil10b::text(g_licht[1]) + ", 0/0: " + teil10b::text(g_licht[2]));
        if (!g_licht[0].da || !g_licht[1].da || !g_licht[2].da || g_licht[0].radius <= 0.0F) { return false; }
        bool gut = regelwerkMovieDuels() && std::fabs(g_licht[1].radius / g_licht[0].radius - 1.5F) < 1e-4F &&
                   std::fabs(g_licht[2].radius - g_licht[0].radius) < 1e-4F;
        for (int k = 0; k < 3; ++k) {
            gut = gut && std::fabs(g_licht[1].farbe[k] - 1.2F * g_licht[0].farbe[k]) < 1e-4F &&
                  std::fabs(g_licht[2].farbe[k] - g_licht[0].farbe[k]) < 1e-4F;
        }
        return gut;
    }));
    add(pruefe("Teil 10b: eine Klinge hat ihr eigenes Licht wie CG_DoSaber (Movie Duels: Laenge*2 + Zufall*10, rgb*0,66)", [] {
        using teil10b::g_licht;
        const ActorState st = g_app->scene.actors[teil9::spieler()].at(2500.0);
        const float l = klingenLaenge(st, 0, 0, 2500.0);
        diag::detail("Knopftest: Teil 10b - Klingenlaenge " + std::to_string(l) + ", Licht bei 1/1: " + teil10b::text(g_licht[0]) +
                     ", Lichter im Bild " + std::to_string(g_licht[0].zahl));
        return g_licht[0].da && l > 0.0F && g_licht[0].radius >= 2.0F * l - 1e-3F && g_licht[0].radius < 2.0F * l + 10.0F &&
               std::fabs(g_licht[0].farbe[0] - 0.66F) < 1e-4F && std::fabs(g_licht[0].farbe[1] - 0.66F) < 1e-4F &&
               std::fabs(g_licht[0].farbe[2] - 0.132F) < 1e-4F;
    }));
    add(pruefe("Teil 10b: cg_turnAnims: nach SET_ANGLES laeuft die Drehanimation, bis die Beine den Blick eingeholt haben", [] {
        const int b = teil10b::figur("bruder");
        if (b < 0 || static_cast<std::size_t>(b) >= g_app->actorAssets.size() || !g_app->actorAssets[static_cast<std::size_t>(b)]) { return false; }
        const Actor& a = g_app->scene.actors[static_cast<std::size_t>(b)];
        const App::ActorAssets& as = *g_app->actorAssets[static_cast<std::size_t>(b)];
        const bool vorher = g_app->settings.cgTurnAnims;
        g_app->settings.cgTurnAnims = true;
        const auto huefte = [&](double t) {
            ActorDraw d;
            std::vector<BoneMatrix> welt;
            float fw[16];
            return figurPose(a, as, a.at(t), t, d, welt, fw) && d.hipsFrame >= 0;
        };
        // erstes und letztes Bild mit Drehanimation zwischen 900 und 3000 ms
        double erstes = -1.0;
        double letztes = -1.0;
        for (double t = 900.0; t <= 3000.0; t += 8.0) {
            if (huefte(t)) {
                if (erstes < 0.0) { erstes = t; }
                letztes = t;
            }
        }
        // die letzte Aenderung der Gier (Q3_SetAngles -> Q3_SetDYaw: NPC_UpdateAngles dreht in Denkbildern)
        double gierEnde = 900.0;
        for (double t = 908.0; t <= 3000.0; t += 8.0) {
            if (std::fabs(shortestAngleDelta(a.at(t - 8.0).angles[1], a.at(t).angles[1])) > 0.0F) { gierEnde = t; }
        }
        // Bilder mit Drehanimation 60 bis 150 ms nach dem Ende der Drehung (Beine holen noch auf); bis Teil 10 keines
        int nachEnde = 0;
        for (double t = gierEnde + 64.0; t <= gierEnde + 150.0; t += 8.0) { nachEnde += huefte(t) ? 1 : 0; }
        const bool bei2900 = huefte(2900.0);
        g_app->settings.cgTurnAnims = vorher;
        diag::detail("Knopftest: Teil 10b - bruder Gier bei 900 " + std::to_string(a.at(900.0).angles[1]) + ", bei 1200 " +
                     std::to_string(a.at(1200.0).angles[1]) + ", Drehanimation von " + std::to_string(erstes) + " bis " + std::to_string(letztes) +
                     " ms, Gier steht ab " + std::to_string(gierEnde) + ", Bilder 64 bis 150 ms danach " + std::to_string(nachEnde) +
                     ", bei 2900 " + std::to_string(bei2900));
        return erstes >= 1000.0 && nachEnde > 0 && !bei2900;
    }));
    add(pruefe("Teil 10b: Bild ueber ein Shaderskript liegt unter dem Dateinamen, nicht unter dem Shadernamen (10thbrother)", [] {
        const std::string shader = "models/players/10thbrother/extras_shiny";
        const std::string datei = "models/players/10thbrother/extras";
        const int b = teil10b::figur("bruder");
        int gefunden = 0;
        int flaechen = 0;
        if (b >= 0 && static_cast<std::size_t>(b) < g_app->actorAssets.size() && g_app->actorAssets[static_cast<std::size_t>(b)]) {
            const App::ActorAssets& as = *g_app->actorAssets[static_cast<std::size_t>(b)];
            for (std::size_t i = 0; i < as.model.surfaces.size() && i < as.textures.bySurface.size(); ++i) {
                std::string n = as.model.surfaces[i].texture;
                for (char& c : n) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
                if (n.rfind(shader, 0) != 0) { continue; }
                ++flaechen;
                if (!as.textures.bySurface[i].rgba.empty()) { ++gefunden; }
            }
        }
        const bool unterShader = g_app->texCache.count(image::mappingName(shader) + "@" + bildStufeText()) != 0;
        const bool unterDatei = g_app->texCache.count(image::mappingName(datei) + "@" + bildStufeText()) != 0;
        const bool stufe = ladeStufenBild(shader) != nullptr;
        diag::detail("Knopftest: Teil 10b - extras_shiny Flaechen " + std::to_string(flaechen) + ", mit Bild " + std::to_string(gefunden) +
                     ", Zwischenspeicher unter Shadername " + std::to_string(unterShader) + ", unter Datei " + std::to_string(unterDatei) +
                     ", Bildstufe mit Shadername findet " + std::to_string(stufe));
        return flaechen > 0 && gefunden == flaechen && !unterShader && unterDatei && !stufe;
    }));
    return s;
}
