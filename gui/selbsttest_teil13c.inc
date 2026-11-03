// selbsttest_teil13c.inc - rc608 Teil 13c (Modus "teil13c"): Brush-Modelle und Gebietsportale im Spielbild des
// Karten-Editors, Patch-LOD an Movern. Karten aus den Spieldaten als Vorlage, Kameras wie die Spielbilder aus Teil 8
// (cg_fov 80, 1920 x 1440, Movie Duels); die Sollwerte sind im Spiel gemessen (SP/g8/fotos/e_stern_0.png,
// SP/g8/fotos/e_must_0.png).
namespace teil13c {
namespace ke = karteneditor;

// duel_yavin im Spielbild, ueber der groessten Flaeche mit surfaceSprites (grasTestZiel der Vorschau), 72 Einheiten hoch,
// 30 Grad nach unten, Gier 45; `vorschauHalme`: die Halme, die die Vorschau fuer dieselbe Kamera baut. Auch fuer leistung.
inline std::vector<Schritt> yavinSzene(const std::shared_ptr<int>& vorschauHalme) {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto ziel = std::make_shared<std::array<float, 3>>();
    add(tu("Teil 13c: duel_yavin in der Vorschau", [] {
        if (!loadMapFromArchive("maps/duel_yavin.bsp")) { meldeFehler("Teil 13c: maps/duel_yavin.bsp nicht geladen"); }
    }));
    add(warteBis("Teil 13c: Vorschau bereit", [] { return g_app->mapBereit; }, 900));
    add(tu("Teil 13c: Vorschau ueber der Grasflaeche", [ziel] {
        float m[3] = {0.0F, 0.0F, 0.0F};
        (void)grasTestZiel(m, true, false);
        for (int k = 0; k < 3; ++k) { (*ziel)[static_cast<std::size_t>(k)] = m[k]; }
        g_app->playMs = 0.0;
        g_app->zeigeGras = true;
        for (int k = 0; k < 3; ++k) { g_app->cam.pos[k] = m[k]; }
        g_app->cam.pos[2] += 72.0F;
        g_app->cam.angles[0] = 30.0F;
        g_app->cam.angles[1] = 45.0F;
        g_app->cam.angles[2] = 0.0F;
    }));
    // Die Halme der Vorschau fuer diese Kamera (baueGrasNetz wie in jedem Bild der Vorschau, rangescalefactor 1).
    add(tu("Teil 13c: Halme der Vorschau", [vorschauHalme] {
        baueGrasNetz(g_app->cam, 1.0F, nullptr);
        *vorschauHalme = g_app->grasVierecke;
    }));
    add(tu("Teil 13c: duel_yavin als Vorlage", [ziel] {
        const std::array<float, 5> kam = {(*ziel)[0], (*ziel)[1], (*ziel)[2] + 72.0F, 30.0F, 45.0F};
        (void)teil13b::vorlage("duel_yavin", kam).tun(0);
    }));
    add(warteBis("Teil 13c: Texturen duel_yavin", [] { return ke::kameraBilderFertig(); }, 20000));
    add(pause(1.5));
    add(stufe9c::langeWarten("Teil 13c: Spielbild duel_yavin", [] { return ke::editor().echtLicht.spielbildGezeigt; }, 20000));
    return s;
}

// md_am_sith im Spielbild, die Kamera 300 Einheiten seitlich eines waagerechten Dampfstrahls (fx_runner mit target, ohne
// use); `vorschauEcken`: das Effektnetz der Vorschau fuer dieselbe Kamera zur Zeit 10 s. Auch fuer leistung.
inline std::vector<Schritt> amSithSzene(const std::shared_ptr<std::size_t>& vorschauEcken) {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto kam = std::make_shared<std::array<float, 5>>();
    add(tu("Teil 13c: md_am_sith in der Vorschau", [] {
        if (!loadMapFromArchive("maps/md_am_sith.bsp")) { meldeFehler("Teil 13c: maps/md_am_sith.bsp nicht geladen"); }
    }));
    add(warteBis("Teil 13c: Vorschau md_am_sith bereit", [] { return g_app->mapBereit; }, 900));
    add(tu("Teil 13c: Effektnetz der Vorschau", [kam, vorschauEcken] {
        std::vector<EffectInstance> laeufer;
        bool kamDa = false;
        for (const MapEntity& e : g_app->map.entities) {
            EffectInstance inst;
            if (e.classname != "fx_runner" || !gespawntImSpiel(e) || !fxRunnerAus(e, g_app->map.entities, {}, inst) || inst.startMs < 0.0) { continue; }
            laeufer.push_back(inst);
            // Die Kamera seitlich vor einen waagerecht gerichteten Laeufer (Dampfstrahl mit target), an einen Ort im Freien
            // (Blatt mit Cluster), Blick auf die Mitte des Strahls.
            if (kamDa || std::fabs(inst.normal[2]) > 0.5F) { continue; }
            const float seite[3] = {-inst.normal[1], inst.normal[0], 0.0F};
            const float mitte[3] = {inst.origin[0] + inst.normal[0] * 150.0F, inst.origin[1] + inst.normal[1] * 150.0F, inst.origin[2]};
            for (const float s : {300.0F, -300.0F}) {
                const float ort[3] = {mitte[0] + seite[0] * s, mitte[1] + seite[1] * s, mitte[2] + 16.0F};
                const int blatt = g_app->geo.leafAt(ort);
                if (blatt < 0 || static_cast<std::size_t>(blatt) >= g_app->geo.leafs.size() || g_app->geo.leafs[static_cast<std::size_t>(blatt)].cluster < 0) { continue; }
                const float d[3] = {mitte[0] - ort[0], mitte[1] - ort[1], mitte[2] - ort[2]};
                const float gier = std::atan2(d[1], d[0]) * 180.0F / 3.14159265F;
                const float nick = -std::atan2(d[2], std::sqrt(d[0] * d[0] + d[1] * d[1])) * 180.0F / 3.14159265F;
                *kam = {ort[0], ort[1], ort[2], nick, gier};
                kamDa = true;
                break;
            }
        }
        if (!kamDa) { meldeFehler("Teil 13c: kein Platz fuer die Kamera an einem Dampfstrahl"); }
        Camera c;
        for (int k = 0; k < 3; ++k) { c.pos[k] = (*kam)[static_cast<std::size_t>(k)]; }
        c.angles[0] = (*kam)[3];
        c.angles[1] = (*kam)[4];
        c.angles[2] = 0.0F;
        float vorn[3];
        float rechts[3];
        float hoch[3];
        c.forward(vorn);
        c.right(rechts);
        c.up(hoch);
        constexpr double kMs = 10000.0;
        std::vector<EffectInstance> alle = laeufer;
        const std::vector<EffectInstance> folgen = effectFollowUpsAt(alle, kMs, &g_app->geo, [](const std::string& n) { return effectByName(n); });
        alle.insert(alle.end(), folgen.begin(), folgen.end());
        EffectCull auge;
        for (int k = 0; k < 3; ++k) { auge.viewer[k] = c.pos[k]; }
        auge.haveViewer = true;
        auge.sichtTabelle = false;
        const BspMesh netz = buildEffectMesh(alle, kMs, rechts, hoch, &g_app->effectShaders, &g_app->geo, &auge);
        *vorschauEcken = netz.verts.size();
        diag::detail("Knopftest: Teil 13c Vorschau " + std::to_string(laeufer.size()) + " Laeufer, " + std::to_string(*vorschauEcken) + " Ecken");
    }));
    add(tu("Teil 13c: md_am_sith als Vorlage", [kam] { (void)teil13b::vorlage("md_am_sith", *kam).tun(0); }));
    add(warteBis("Teil 13c: Texturen md_am_sith", [] { return ke::kameraBilderFertig(); }, 20000));
    add(pause(1.5));
    add(stufe9c::langeWarten("Teil 13c: Spielbild md_am_sith", [] { return ke::editor().echtLicht.spielbildGezeigt; }, 20000));
    return s;
}

inline std::vector<Schritt> schritte() {
    std::vector<Schritt> s = stufe9c::einrichten();
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto bild = std::make_shared<stufe9c::Bild>();
    auto warten = [&add](const std::string& was) {
        add(warteBis("Teil 13c: Texturen " + was, [] { return ke::kameraBilderFertig(); }, 20000));
        add(pause(1.5));
        add(stufe9c::langeWarten("Teil 13c: Spielbild " + was, [] { return ke::editor().echtLicht.spielbildGezeigt; }, 20000));
        add(tu("Teil 13c: feste Zeit " + was, [] { ke::spielbildZeitFest(10.0); }));
        add(pause(0.3));
    };

    // 1. duel_deathstar, hinten die Druckschotten *6/*7 (func_door ohne START_OPEN, targetname blastdoorclose_door): das
    // Spiel zeichnet sie an ihrer Entity in der Anfangsstellung pos1, also zu (InitMover). Ihre Flaechen stehen in der .bsp
    // in einem eigenen Blatt ausserhalb des Baums der Welt; das Spielbild nahm sie mit der Welt und verdeckte sie ueber das
    // PVS jenes Blatts - ein offener heller Durchgang. Spiel 16,5 18,3 21,0.
    add(teil13b::vorlage("duel_deathstar", {32.0F, -5112.0F, 84.25F, 0.0F, 90.0F}));
    warten("duel_deathstar");
    add(tu("Teil 13c: Bild duel_deathstar", [bild] { teil13b::bildLesen(*bild, "k13c_deathstar"); }));
    add(pruefe("Teil 13c: Brush-Modelle an ihrer Entity - die geschlossenen Schotten von duel_deathstar wie im Spiel (16,5 18,3 21,0, hoechstens 5 % daneben)",
               [bild] {
                   const auto b = teil11::mittel(*bild, 900, 680, 1015, 765);
                   diag::detail("Knopftest: Teil 13c Schotten " + teil11::text3(b));
                   return bild->breite == 1920 && teil11::nahAm(b, {16.5, 18.3, 21.0}, 0.05);
               }));

    // 2. duel_mustafar, die Doppeltuer *35..*38 (func_door ohne targetname, lip 24): im Spiel zu, im Spielbild fehlte sie
    // (derselbe Grund). Spiel 53,8 39,5 30,7.
    add(teil13b::vorlage("duel_mustafar", {688.0F, 2256.0F, 76.25F, 0.0F, 270.0F}));
    warten("duel_mustafar");
    add(tu("Teil 13c: Bild duel_mustafar", [bild] { teil13b::bildLesen(*bild, "k13c_mustafar"); }));
    add(pruefe("Teil 13c: Brush-Modelle an ihrer Entity - die Tuer von duel_mustafar wie im Spiel (53,8 39,5 30,7, hoechstens 5 % daneben)", [bild] {
        const auto b = teil11::mittel(*bild, 520, 450, 780, 900);
        diag::detail("Knopftest: Teil 13c Tuer " + teil11::text3(b));
        return teil11::nahAm(b, {53.8, 39.5, 30.7}, 0.05);
    }));

    // 3. Patch-LOD an Movern: duel_jeditemple (Kamera aus dem Selbsttest leistung) hat Tueren mit Patches (*55: 73).
    // RB_SurfaceGrid waehlt ihre Spalten und Zeilen nach dem Abstand von lodOrigin, den LodErrorForVolume mit der
    // Stellung der Entity in die Welt rechnet - vorher nahmen Mover das volle Gitter.
    auto lodVorher = std::make_shared<std::uint64_t>(0);
    add(tu("Teil 13c: Patch-LOD der Mover zaehlen", [lodVorher] { *lodVorher = gpu::moverPatchLodAenderungen(); }));
    add(teil13b::vorlage("duel_jeditemple", {-1250.0F, -20.0F, -1113.75F, 0.0F, 90.0F}));
    warten("duel_jeditemple");
    add(pruefe("Teil 13c: Patches an Movern - Gitter nach Abstand wie die Welt (RB_SurfaceGrid mit der Stellung der Entity)", [lodVorher] {
        const std::uint64_t lod = gpu::moverPatchLodAenderungen() - *lodVorher;
        diag::detail("Knopftest: Teil 13c Mover-Patches nach Abstand neu " + std::to_string(lod) + ", Modelle im Bild " +
                     std::to_string(ke::spielbildStand().modelle));
        // Endrunde rc608: gemessen 166 (sieben Laeufe gleich); verlangt die Haelfte.
        return lod >= 166U / 2U;
    }));

    // 4. surfaceSprites im Spielbild ueber denselben Code wie die Vorschau (grasNetzFuer, RB_DrawSurfaceSprites):
    // duel_yavin, ueber der groessten Flaeche mit surfaceSprites (grasTestZiel), 72 Einheiten hoch, 30 Grad nach unten,
    // Gier 45, Zeit 0. Die Vorschau rechnet fuer dieselbe Kamera ihre Halme; das Spielbild muss gleich viele bauen (die
    // Zahl haengt nicht am Bildwinkel: RB_DrawSurfaceSprites laesst nur Flaechen hinter dem Auge und ausser Reichweite weg)
    // und sie zeichnen (Bild mit gegen ohne Halme).
    auto vorschauHalme = std::make_shared<int>(0);
    for (Schritt& x : yavinSzene(vorschauHalme)) { s.push_back(std::move(x)); }
    add(tu("Teil 13c: Zeit 0, ohne Halme", [] {
        ke::spielbildZeitFest(0.0);
        g_app->zeigeGras = false;
    }));
    add(pause(0.3));
    auto ohneHalme = std::make_shared<stufe9c::Bild>();
    add(tu("Teil 13c: Bild duel_yavin ohne Halme", [ohneHalme] { teil13b::bildLesen(*ohneHalme, "k13c_yavin_ohne"); }));
    add(tu("Teil 13c: Halme an", [] { g_app->zeigeGras = true; }));
    add(pause(0.3));
    add(tu("Teil 13c: Bild duel_yavin", [bild] { teil13b::bildLesen(*bild, "k13c_yavin"); }));
    add(pruefe("Teil 13c: surfaceSprites im Spielbild - so viele Halme wie die Vorschau fuer dieselbe Kamera, sichtbar", [bild, ohneHalme, vorschauHalme] {
        const int editor = ke::spielbildStand().grasVierecke;
        long anders = 0;
        const std::size_t n = std::min(bild->rgba.size(), ohneHalme->rgba.size()) / 4U;
        for (std::size_t i = 0; i < n; ++i) {
            const int d = std::abs(int(bild->rgba[i * 4]) - int(ohneHalme->rgba[i * 4])) + std::abs(int(bild->rgba[i * 4 + 1]) - int(ohneHalme->rgba[i * 4 + 1])) +
                          std::abs(int(bild->rgba[i * 4 + 2]) - int(ohneHalme->rgba[i * 4 + 2]));
            if (d > 24) { ++anders; }
        }
        const double anteil = n > 0U ? static_cast<double>(anders) / static_cast<double>(n) : 0.0;
        diag::detail("Knopftest: Teil 13c Halme Spielbild " + std::to_string(editor) + ", Vorschau " + std::to_string(*vorschauHalme) +
                     ", Bildpunkte anders " + std::to_string(anteil * 100.0) + " %");
        return editor > 50 && editor == *vorschauHalme && anteil > 0.02;
    }));

    // 5. fx_runner im Spielbild ueber denselben Code wie die Vorschau (fxRunnerAus, effectFollowUpsAt, buildEffectMesh):
    // md_am_sith, die Kamera 300 Einheiten seitlich eines waagerechten Dampfstrahls (fx_runner mit target, ohne use), Zeit 10 s. Die
    // Rechnung der Vorschau (Laeufer aus der Karte der Vorschau, ihre Dateien und Shaderplaetze) ergibt fuer dieselbe
    // Kamera dasselbe Netz; das Spielbild zeichnet es (Bild mit gegen ohne Effekte).
    auto vorschauEcken = std::make_shared<std::size_t>(0);
    for (Schritt& x : amSithSzene(vorschauEcken)) { s.push_back(std::move(x)); }
    add(tu("Teil 13c: Zeit 10 s, ohne Effekte", [] {
        ke::spielbildZeitFest(10.0);
        g_app->showEffects = false;
    }));
    add(pause(0.3));
    auto ohneFx = std::make_shared<stufe9c::Bild>();
    add(tu("Teil 13c: Bild md_am_sith ohne Effekte", [ohneFx] { teil13b::bildLesen(*ohneFx, "k13c_amsith_ohne"); }));
    add(tu("Teil 13c: Effekte an", [] { g_app->showEffects = true; }));
    add(pause(0.3));
    add(tu("Teil 13c: Bild md_am_sith", [bild] { teil13b::bildLesen(*bild, "k13c_amsith"); }));
    add(pruefe("Teil 13c: fx_runner im Spielbild - dasselbe Effektnetz wie die Vorschau fuer dieselbe Kamera, sichtbar", [bild, ohneFx, vorschauEcken] {
        const ke::SpielbildStand st = ke::spielbildStand();
        long anders = 0;
        for (std::size_t i = 0; i + 3 < bild->rgba.size() && i + 3 < ohneFx->rgba.size(); i += 4) {
            if (bild->rgba[i] != ohneFx->rgba[i] || bild->rgba[i + 1] != ohneFx->rgba[i + 1] || bild->rgba[i + 2] != ohneFx->rgba[i + 2]) { ++anders; }
        }
        diag::detail("Knopftest: Teil 13c Effekte Spielbild " + std::to_string(st.effektLaeufer) + " Laeufer, " + std::to_string(st.effektEcken) +
                     " Ecken, Vorschau " + std::to_string(*vorschauEcken) + " Ecken, Bildpunkte anders " + std::to_string(anders));
        return st.effektEcken > 0 && static_cast<std::size_t>(st.effektEcken) == *vorschauEcken && anders > 0;
    }));

    // 6. Himmelsportal, ein Blick, in dem das Portal etwas anderes zeigt als die Himmelsbox: duel_invisible_hand vom
    // Spielerstart der Duelle aus (Spiel SP/g13c/erg/spiel_ih_1.png, viewpos 8620 560 -6899, Blick 0): hinter den Scheiben
    // die Schiffe der Portalszene (CG_DrawSkyBoxPortal). Spiel: Schiff links 65,7 69,9 70,7, Sternzerstoerer rechts
    // 51,2 46,9 53,5, Streifen oben rechts 60,6 47,4 33,9. Ohne Portal stuende dort die Himmelsbox.
    add(teil13b::vorlage("duel_invisible_hand", {8620.0F, 560.0F, -6899.0F, 0.0F, 0.0F}));
    warten("duel_invisible_hand");
    add(tu("Teil 13c: Bild duel_invisible_hand", [bild] { teil13b::bildLesen(*bild, "k13c_ih"); }));
    add(pruefe("Teil 13c: misc_skyportal - die Schiffe der Portalszene hinter den Scheiben wie im Spiel (je hoechstens 5 % daneben)", [bild] {
        const auto links = teil11::mittel(*bild, 120, 560, 600, 800);
        const auto rechts = teil11::mittel(*bild, 1060, 620, 1500, 780);
        const auto streifen = teil11::mittel(*bild, 1440, 120, 1700, 420);
        diag::detail("Knopftest: Teil 13c Portal Schiff " + teil11::text3(links) + ", Zerstoerer " + teil11::text3(rechts) + ", Streifen " +
                     teil11::text3(streifen) + ", Effekte im Portal " + std::to_string(ke::spielbildStand().portalEffektEcken) + " Ecken");
        return ke::spielbildStand().himmelPortal && teil11::nahAm(links, {65.7, 69.9, 70.7}, 0.05) && teil11::nahAm(rechts, {51.2, 46.9, 53.5}, 0.05) &&
               teil11::nahAm(streifen, {60.6, 47.4, 33.9}, 0.05);
    }));

    add(tu("Teil 13c: aufraeumen", [] {
        ke::spielbildZeitFest(-1.0);
        ke::vorlageEntfernen();
        ke::Editor& ed = ke::editor();
        ed.kamFenster.messBreite = 0;
        ed.kamFenster.messHoehe = 0;
        ed.filterMaske = 0;
    }));
    return s;
}

}  // namespace teil13c
