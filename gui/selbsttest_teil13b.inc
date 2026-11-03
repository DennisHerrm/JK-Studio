// selbsttest_teil13b.inc - rc608 Teil 13b (Modus "teil13b"): "Built map as in the game" mit Kartenmodellen,
// Himmelsportal und Wetter, Karten aus den Spieldaten als Vorlage, Kameras wie die Spielbilder aus Teil 8 (cg_fov 80,
// 1920 x 1440, Movie Duels). Die Sollwerte sind im Spiel gemessen (SP/g8/fotos/e_stern_0.png,
// SP/g13b/erg/spiel_exec.png, SP/g5/fotos/hoth_1.png, SP/g10/fotos/jtb10.png).
namespace teil13b {
namespace ke = karteneditor;

// Eine Karte der Spieldaten als Vorlage, Kamera wie im Spiel.
inline Schritt vorlage(const std::string& karte, const std::array<float, 5>& kam) {
    return tu("Teil 13b: " + karte + " als Vorlage", [karte, kam] {
        karteDialogeZu();
        ke::Editor& ed = ke::editor();
        ed.echtLicht.an = false;
        ed.echtLicht.backen = false;
        ed.echtLicht.spielbild = true;
        ed.karte.undo.alsGespeichertMarkieren();
        ke::neueKarte();
        std::string daten;
        if (!ed.karteSpiel || !ed.karteSpiel->fs.dateiLesen("maps/" + karte + ".bsp", daten)) {
            meldeFehler("Teil 13b: maps/" + karte + ".bsp nicht in den Spieldaten");
            return;
        }
        std::string f;
        if (!ke::vorlageLaden(daten, karte + ".bsp", &f)) { meldeFehler("Teil 13b: Vorlage: " + f); }
        teil11::Paar p;
        std::copy(kam.begin(), kam.end(), p.kam);
        teil11::kameraWieImSpiel(p);
    });
}

inline void bildLesen(stufe9c::Bild& b, const std::string& name) {
    if (!ke::kameraBild(b.rgba, b.breite, b.hoehe)) {
        meldeFehler("Teil 13b: Kamerabild nicht lesbar (" + name + ")");
        return;
    }
    const char* o = std::getenv("BHED_FOTOS");
    if (o != nullptr && o[0] != '\0') { stufe9c::bmpSchreiben(std::string(o) + "\\" + name + ".bmp", b); }
    const ke::SpielbildStand st = ke::spielbildStand();
    diag::detail("Knopftest: Teil 13b Bild " + name + " - Aufrufe " + std::to_string(st.aufrufe) + ", Modelle " + std::to_string(st.modelle) + " (" +
                 std::to_string(st.modellAufrufe) + " Aufrufe), Himmelsportal " + (st.himmelPortal ? "ja" : "nein") + ", Wetter " +
                 std::to_string(st.wetterTeilchen) + ", " + std::to_string(st.ms) + " ms");
}

inline std::vector<Schritt> schritte() {
    std::vector<Schritt> s = stufe9c::einrichten();
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto bild = std::make_shared<stufe9c::Bild>();
    auto ohne = std::make_shared<stufe9c::Bild>();
    auto warten = [&add](const std::string& was) {
        add(warteBis("Teil 13b: Texturen " + was, [] { return ke::kameraBilderFertig(); }, 20000));
        add(pause(1.5));
        add(stufe9c::langeWarten("Teil 13b: Spielbild " + was, [] { return ke::editor().echtLicht.spielbildGezeigt; }, 20000));
        add(tu("Teil 13b: feste Zeit " + was, [] { ke::spielbildZeitFest(10.0); }));
        add(pause(0.3));
    };

    // 1. misc_model_static: duel_deathstar, der Falke oben links (CG_DrawMiscEnts). Spiel 88,2 85,8 81,8.
    add(vorlage("duel_deathstar", {32.0F, -5112.0F, 84.25F, 0.0F, 90.0F}));
    warten("duel_deathstar");
    add(tu("Teil 13b: Bild duel_deathstar", [bild] { bildLesen(*bild, "k13b_deathstar"); }));
    add(pruefe("Teil 13b: misc_model_static im Spielbild - der Falke oben links wie im Spiel (88,2 85,8 81,8, hoechstens 5 % daneben)", [bild] {
        const auto b = teil11::mittel(*bild, 0, 0, 640, 400);
        diag::detail("Knopftest: Teil 13b Falke " + teil11::text3(b));
        return ke::spielbildStand().modelle > 0 && bild->breite == 1920 && teil11::nahAm(b, {88.2, 85.8, 81.8}, 0.05);
    }));

    // 2. misc_skyportal: duel_executor_bridge vom Spielerstart (Spiel SP/g13b/erg/spiel_exec.png, viewpos -16 -32 140,25,
    // Blick 90). Hinter den Scheiben (glass_reflective, GL_ZERO GL_ONE_MINUS_SRC_COLOR) Sterne und Schiffe der Portalszene
    // (CG_DrawSkyBoxPortal). Spiel mittlere Scheibe 17,6 19,1 18,3, linke 18,1 20,8 19,5.
    add(vorlage("duel_executor_bridge", {-16.0F, -32.0F, 140.25F, 0.0F, 90.0F}));
    warten("duel_executor_bridge");
    add(tu("Teil 13b: Bild duel_executor_bridge", [bild] { bildLesen(*bild, "k13b_executor"); }));
    add(pruefe("Teil 13b: misc_skyportal im Spielbild - die Scheiben der Bruecke wie im Spiel (17,6 19,1 18,3 und 18,1 20,8 19,5, hoechstens 5 % daneben)", [bild] {
        const auto mitte = teil11::mittel(*bild, 860, 560, 1130, 800);
        const auto links = teil11::mittel(*bild, 560, 540, 840, 800);
        diag::detail("Knopftest: Teil 13b Scheiben " + teil11::text3(mitte) + ", " + teil11::text3(links));
        return ke::spielbildStand().himmelPortal && teil11::nahAm(mitte, {17.6, 19.1, 18.3}, 0.05) && teil11::nahAm(links, {18.1, 20.8, 19.5}, 0.05);
    }));

    // 3. Wetter: duel_kamino_lp hat fx_rain (spawnflags 2: "rain", 1000 Tropfen). Kamera am Spielerstart.
    add(vorlage("duel_kamino_lp", {192.0F, -1496.0F, 83.25F, 0.0F, 180.0F}));
    warten("duel_kamino_lp");
    auto teilchen = std::make_shared<int>(0);
    add(tu("Teil 13b: Bild duel_kamino_lp", [bild, teilchen] {
        bildLesen(*bild, "k13b_kamino_lp");
        *teilchen = ke::spielbildStand().wetterTeilchen;
        g_app->zeigeWetter = false;
    }));
    add(pause(0.5));
    add(tu("Teil 13b: Bild duel_kamino_lp ohne Wetter", [ohne] {
        bildLesen(*ohne, "k13b_kamino_lp_ohne");
        g_app->zeigeWetter = true;
    }));
    add(pruefe("Teil 13b: Wetter im Spielbild - fx_rain zeichnet Tropfen (Teilchen und Bildpunkte, die ohne Wetter anders sind)", [bild, ohne, teilchen] {
        long anders = 0;
        for (std::size_t i = 0; i + 3 < bild->rgba.size() && i + 3 < ohne->rgba.size(); i += 4) {
            if (bild->rgba[i] != ohne->rgba[i] || bild->rgba[i + 1] != ohne->rgba[i + 1] || bild->rgba[i + 2] != ohne->rgba[i + 2]) { ++anders; }
        }
        diag::detail("Knopftest: Teil 13b Wetter - Teilchen " + std::to_string(*teilchen) + ", Bildpunkte anders " + std::to_string(anders));
        // Endrunde rc608: gemessen 1879 Teilchen und 8753 Bildpunkte (fuenf Laeufe gleich); verlangt die Haelfte.
        return *teilchen >= 1879 / 2 && anders >= 8753 / 2;
    }));

    // 4. Patches wie im Spiel (R_SubdividePatchToGrid, RB_SurfaceGrid mit r_lodCurveError 250): duel_jeditemple mit der
    // Kamera von g9_jtb (Teil 10, Spiel SP/g10/fotos/jtb10.png), die Saeule mit tcGen environment in der Mitte hinten.
    // Spiel oben 32,0 26,6 22,6, Mitte 33,4 27,6 23,5, unten 32,6 26,9 22,9; vorher (gleichmaessiges Netz) 0,86 bis 0,95.
    auto lodVorher = std::make_shared<std::uint64_t>(0);
    add(tu("Teil 13b: Patch-LOD zaehlen", [lodVorher] { *lodVorher = gpu::patchLodAenderungen(); }));
    add(vorlage("duel_jeditemple", {-1250.0F, -20.0F, -1113.75F, 0.0F, 90.0F}));
    warten("duel_jeditemple");
    add(tu("Teil 13b: Bild duel_jeditemple", [bild] { bildLesen(*bild, "k13b_jeditemple"); }));
    add(pruefe("Teil 13b: Patches wie im Spiel - die ferne Saeule mit tcGen environment wie im Spiel (je Drittel hoechstens 3 % daneben)", [bild, lodVorher] {
        const auto oben = teil11::mittel(*bild, 1090, 100, 1150, 300);
        const auto mitte = teil11::mittel(*bild, 1090, 300, 1150, 500);
        const auto unten = teil11::mittel(*bild, 1090, 500, 1150, 700);
        const std::uint64_t lod = gpu::patchLodAenderungen() - *lodVorher;
        diag::detail("Knopftest: Teil 13b Saeule oben " + teil11::text3(oben) + ", Mitte " + teil11::text3(mitte) + ", unten " + teil11::text3(unten) +
                     ", Patch-Abschnitte nach Abstand neu " + std::to_string(lod));
        // Endrunde rc608: gemessen 492 bis 500 neu gewaehlte Patches; verlangt die Haelfte (die Wahl selbst prueft ctest bspgeotest).
        return lod >= 492U / 2U && teil11::nahAm(oben, {32.0, 26.6, 22.6}, 0.03) && teil11::nahAm(mitte, {33.4, 27.6, 23.5}, 0.03) &&
               teil11::nahAm(unten, {32.6, 26.9, 22.9}, 0.03);
    }));

    // 5. Gegenprobe Patches: duel_hoth (g5_hoth aus Teil 8, Spiel SP/g5/fotos/hoth_1.png). Der Schein der Lampe im Tor
    // ist ein 3x3-Patch mit deformVertexes autoSprite: R_SubdividePatchToGrid laesst die geraden Zeilen weg, es bleiben
    // vier Ecken, das Spiel dreht sie zur Kamera und mischt GL_ONE GL_ONE_MINUS_SRC_COLOR. Spiel 179,8 179,8 205,9.
    add(vorlage("duel_hoth", {-361.0F, -348.0F, -161.4F, 0.0F, 0.0F}));
    warten("duel_hoth");
    add(tu("Teil 13b: Bild duel_hoth", [bild] { bildLesen(*bild, "k13b_hoth"); }));
    add(pruefe("Teil 13b: Gegenprobe Patches - der Schein im Tor von duel_hoth wie im Spiel (179,8 179,8 205,9, hoechstens 3 % daneben)", [bild] {
        const auto b = teil11::mittel(*bild, 960, 576, 1056, 672);
        diag::detail("Knopftest: Teil 13b Schein " + teil11::text3(b));
        return teil11::nahAm(b, {179.8, 179.8, 205.9}, 0.03);
    }));

    add(tu("Teil 13b: aufraeumen", [] {
        ke::spielbildZeitFest(-1.0);
        ke::vorlageEntfernen();
        ke::Editor& ed = ke::editor();
        ed.kamFenster.messBreite = 0;
        ed.kamFenster.messHoehe = 0;
        ed.filterMaske = 0;
    }));
    return s;
}

}  // namespace teil13b
