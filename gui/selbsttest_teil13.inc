// selbsttest_teil13.inc - rc608 Teil 13 (Modus "teil13").
//
// Schwertlicht eines Griffs ohne Klinge: riot_baton (MD_Hilts.pk3, melee_weapons.sab) hat noBlade 1 und kein noDlight.
// CG_AddSaberBladeGo zeichnet die Klinge nicht, ruft aber CG_DoSaberLight (cg_players.cpp:6720-6727 OpenJK, MD
// ebenso): ein Licht in der Mitte der Klinge, Farbe der Klinge, Radius Laenge*2 + Zufall*8, unter Movie Duels mal
// cg_saberLightRadius und die Farbe mal cg_saberLightIntensity.
std::vector<Schritt> teil13Schritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("Teil 13: duel_jeditemple", [] {
        g_app->leftMode = 1;
        (void)loadMapFromArchive("maps/duel_jeditemple.bsp");
    }));
    add(warteBis("Teil 13: Karte bereit", [] { return g_app->map.path == "maps/duel_jeditemple.bsp" && g_app->mapBereit; }, 4000));
    add(tu("Teil 13: Spieler mit riot_baton (noBlade, ohne noDlight)", [] {
        teil10::spielerStart(-1250.0F, 60.0F, -1150.0F, "90");
        teil9::laden(teil9::kKopf + "affect ( \"player\", /*@AFFECT_TYPE*/ FLUSH )\r\n{\r\n"
                     "\tset ( /*@SET_TYPES*/ \"SET_WEAPON\", \"WP_SABER\" );\r\n"
                     "\tset ( /*@SET_TYPES*/ \"SET_SABER1\", \"riot_baton\" );\r\n"
                     "\tset ( /*@SET_TYPES*/ \"SET_SABERACTIVE\", \"true\" );\r\n"
                     "\twait ( 3000.000 );\r\n}\r\n", "Teil 13");
    }));
    add(warteBis("Teil 13: Figur geladen", [] {
        return g_app->camTrackValid && jk2probe::figurenGeladen() && teil9::spieler() < g_app->scene.actors.size();
    }, 3000));
    add(tu("Teil 13: Schwertlicht 1 / 1", [] { teil10b::schwertlicht(1.0F, 1.0F); }));
    add(pause(1.5));   // ohne Licht soll die Pruefung unten scheitern, nicht das Warten
    add(tu("Teil 13: Licht merken", [] { (void)teil10b::merke(0); }));
    add(pruefe("Teil 13: Griff ohne Klinge (noBlade) leuchtet ueber CG_DoSaberLight: Laenge*2 + Zufall*8, Farbe der Klinge", [] {
        using teil10b::g_licht;
        const ActorState st = g_app->scene.actors[teil9::spieler()].at(2500.0);
        const float l = klingenLaenge(st, 0, 0, 2500.0);
        const Actor& akt = g_app->scene.actors[teil9::spieler()];
        const int art = st.saber[0].art;
        const std::string waffe = art >= 0 && static_cast<std::size_t>(art) < akt.saberArten.size() ? akt.saberArten[static_cast<std::size_t>(art)].name : std::string();
        diag::detail("Knopftest: Teil 13 - Schwert " + waffe + ", Klingenlaenge " + std::to_string(l) + ", Lichter im Bild " +
                     std::to_string(g_app->schwertLichter.size()) + ", naechstes " + (g_licht[0].da ? teil10b::text(g_licht[0]) : std::string("-")));
        g_app->settings.cgSaberLightIntensity = 1.070312F;
        g_app->settings.cgSaberLightRadius = 0.703125F;
        return waffe == "riot_baton" && l > 0.0F && g_licht[0].da && g_licht[0].radius >= 2.0F * l - 1e-3F && g_licht[0].radius <= 2.0F * l + 8.0F;
    }));
    return s;
}
