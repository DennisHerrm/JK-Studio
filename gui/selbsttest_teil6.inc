// selbsttest_teil6.inc - rc608 Teil 6 (Modus "teil6"): Portale und Schmerz im laufenden Programm.
//
// 1. Karte duel_mustafar: die Tueren schalten ihre Portale, hinter der geschlossenen Tuer ist nichts
//    zu zeichnen (R_RecursiveWorldNode, areamask); r_DynamicGlow je Regelwerk.
// 2. Mission auf md_am_sith: ein Wachgeschuetz trifft eine Figur. Unter Movie Duels zuckt sie nach dem
//    Trefferort (G_MissileImpacted, 2 s Pause, Torso 400 ms), unter OpenJK nach NPC_ChoosePainAnimation.
//    Die Figur steht trotzdem: die Bolzen schieben nicht (DAMAGE_NO_KNOCKBACK).
// 3. Eine Explosion (misc_model_breakable) stoesst die Figur (G_RadiusDamage, G_ApplyKnockback).
namespace teil8 {
// Die Rechtecke der Himmelsbox fuer eine Kamera wie im Spiel (cg_fov 80 bei 1920 x 1440), alle Portale zu oder alle offen.
struct HimmelProbe {
    std::vector<HimmelRechteck> rechtecke;
    std::size_t flaechen = 0U;
};
inline HimmelProbe himmelProbe(const float kam[4], bool offen) {
    std::vector<HimmelKeule> keulen(g_app->geo.shaders.size(), kKeinHimmel);
    for (std::size_t i = 0; i < keulen.size(); ++i) {
        if (g_app->geo.shaders[i].isSky()) { keulen[i] = kHimmelVorn; }
    }
    HimmelBox box;
    box.anlegen(g_app->geo, g_app->mesh, keulen);
    AreaPortale p;
    int gebiete = 0;
    for (const BspGeometry::Leaf& lf : g_app->geo.leafs) { gebiete = std::max(gebiete, lf.area + 1); }
    p.anlegen(gebiete);
    if (offen) {
        for (int a = 1; a < gebiete; ++a) { p.setze(0, a, true); }
    }
    const float w = kam[3] * 3.14159265F / 180.0F;
    HimmelSicht sicht;
    for (int k = 0; k < 3; ++k) {
        sicht.auge[k] = kam[k];
        sicht.pvsOrt[k] = kam[k];
    }
    sicht.vorn[0] = std::cos(w);
    sicht.vorn[1] = std::sin(w);
    sicht.links[0] = -std::sin(w);
    sicht.links[1] = std::cos(w);
    sicht.hoch[2] = 1.0F;
    sicht.fovX = 80.0F;
    sicht.fovY = 64.3727F;
    sicht.portale = &p;
    sicht.gebiet = punktGebiet(g_app->geo, kam);
    HimmelProbe aus;
    box.rechne(sicht, aus.rechtecke);
    aus.flaechen = box.flaechenImBild();
    return aus;
}
// Die Richtung eines Bildpunkts im Spielbild 1920 x 1440.
inline bool deckt(const HimmelProbe& h, const float kam[4], int px, int py) {
    const float w = kam[3] * 3.14159265F / 180.0F;
    const float a = (static_cast<float>(px) + 0.5F) / 1920.0F * 2.0F - 1.0F;
    const float b = 1.0F - (static_cast<float>(py) + 0.5F) / 1440.0F * 2.0F;
    const float tx = std::tan(40.0F * 3.14159265F / 180.0F);
    const float d[3] = {std::cos(w) + a * tx * std::sin(w), std::sin(w) - a * tx * std::cos(w), b * tx * 0.75F};
    return himmelDeckt(h.rechtecke, d);
}
}  // namespace teil8

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
        const std::vector<char> m = flaechenAusserSicht(g_app->geo, g_app->areaPortale, punktGebiet(g_app->geo, start), -1);
        std::size_t verdeckt = 0;
        for (const char c : m) { verdeckt += c != 0 ? 1U : 0U; }
        diag::detail("Knopftest: Teil 6 - " + std::to_string(g_app->portalEnts.size()) + " Portal-Entities, Gebiete " + std::to_string(g_app->areaPortale.gebiete()) +
                     ", vom Start verdeckt " + std::to_string(verdeckt) + " von " + std::to_string(m.size()));
        return !g_app->portalEnts.empty() && g_app->areaPortale.gebiete() > 1 && verdeckt > 0U && m.size() > 1193U && m[1193] != 0;
    }));
    // Endrunde rc608: Eine Aenderung im Entity-Editor (hier: die Tuer wird zu info_null) gilt sofort, auch wenn die Zahl
    // der Entities gleich bleibt. Vorher hing die Liste nur an Karte und Zahl.
    auto portaleVorher = std::make_shared<std::size_t>(0);
    auto tuer = std::make_shared<std::pair<std::size_t, std::string>>(0, "");
    add(tu("Teil 6: eine Portal-Tuer im Entity-Editor zu info_null", [=] {
        const float start[3] = {-1024.0F, 7320.0F, 24.0F};
        areaPortaleJetzt(start);
        *portaleVorher = g_app->portalEnts.size();
        const std::string modell = g_app->portalEnts.empty() ? std::string() : "*" + std::to_string(g_app->portalEnts[0].modell);
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            const std::string* m = g_app->map.entities[i].find("model");
            if (m != nullptr && *m == modell) {
                *tuer = {i, g_app->map.entities[i].classname};
                g_app->map.entities[i].classname = "info_null";
                ++g_app->entStand;   // wie nachAenderung (app_mission.cpp) nach jeder Aenderung im Entity-Editor
                break;
            }
        }
        areaPortaleJetzt(start);
    }));
    add(pruefe("Teil 6: nach der Aenderung im Entity-Editor ein Portal weniger (Schluessel mit entStand)", [=] {
        diag::detail("Knopftest: Teil 6 - Portal-Entities vorher " + std::to_string(*portaleVorher) + ", nachher " + std::to_string(g_app->portalEnts.size()) +
                     ", Tuer " + tuer->second);
        return !tuer->second.empty() && *portaleVorher > 0U && g_app->portalEnts.size() + 1U == *portaleVorher;
    }));
    add(tu("Teil 6: die Tuer zurueck", [=] {
        if (!tuer->second.empty() && tuer->first < g_app->map.entities.size()) {
            g_app->map.entities[tuer->first].classname = tuer->second;
            ++g_app->entStand;
        }
    }));

    // rc608 Teil 8: Der Himmel: nur die Rechtecke aus RB_StageIteratorSky. Dieselbe Kamera wie die Fotos der Testkarten
    // (ohne Entities, alle Portale zu). Im Spiel gemessen: das Feld rot, beide Fenster auf t3_byss schwarz.
    add(pruefe("Teil 8: duel_mustafar, alle Tueren zu: Das rote Feld liegt im Rechteck der Himmelsbox, der Himmel hinter den Tueren zaehlt nicht", [] {
        const float kam[4] = {-1080.57F, 7376.57F, 76.25F, 315.0F};
        const teil8::HimmelProbe zu = teil8::himmelProbe(kam, false);
        const teil8::HimmelProbe auf = teil8::himmelProbe(kam, true);
        diag::detail("Knopftest: Teil 8 Himmel duel_mustafar - Rechtecke " + std::to_string(zu.rechtecke.size()) + ", Flaechen zu " + std::to_string(zu.flaechen) +
                     ", offen " + std::to_string(auf.flaechen));
        return teil8::deckt(zu, kam, 1405, 723) && zu.flaechen < auf.flaechen;
    }));
    // Endrunde rc608: vergissKarte (jeder Kartenwechsel) leert den Zwischenspeicher der Himmelsbox. Vorher hing er nur an
    // Netzadresse, Zahl der Aufrufe und Bildersatz; eine neue Karte mit derselben Adresse und Zahl rechnete mit den alten Flaechen.
    add(pruefe("Teil 8: duel_mustafar gezeichnet - der Zwischenspeicher der Himmelsbox haelt ein Netz", [] { return gpu::himmelBoxGemerkt(); }));
    // Im selben Schritt: das naechste Bild fuellt den Speicher fuer die Karte gleich wieder.
    add(pruefe("Teil 8: nach vergissKarte (wie beim Kartenwechsel) haelt der Zwischenspeicher der Himmelsbox kein Netz mehr", [] {
        gpu::vergissKarte();
        g_app->mapDirty = true;
        return !gpu::himmelBoxGemerkt();
    }));
    add(tu("Teil 8: eine Mission mit dem Spieler", [] {
        Script neu;
        std::vector<Diag> d;
        const std::string text = "//Generated by BehavEd\r\n\r\naffect ( \"player\", /*@AFFECT_TYPE*/ FLUSH )\r\n{\r\n"
                                 "\tset ( /*@SET_TYPES*/ \"SET_INVINCIBLE\", \"true\" );\r\n\twait ( 12000.000 );\r\n}\r\n";
        if (readScript(text, neu, d)) {
            g_app->doc.ersetzeSkript(std::move(neu), "Teil 8");
            rebuildTree();
        }
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    add(warteBis("Teil 8: Szene", [] {
        return g_app->camTrackValid && std::any_of(g_app->scene.actors.begin(), g_app->scene.actors.end(), [](const Actor& a) { return a.echterSpieler; }) && jk2probe::figurenGeladen();
    }, 3000));
    add(tu("Teil 8: Kamera in ein anderes Gebiet als der Spieler", [=] {
        const float start[3] = {-1024.0F, 7320.0F, 24.0F};
        const int spielerGebiet = punktGebiet(g_app->geo, start);
        for (const BspGeometry::Leaf& lf : g_app->geo.leafs) {
            if (lf.cluster < 0 || lf.area < 0 || lf.area == spielerGebiet) { continue; }
            const float m[3] = {(lf.mins[0] + lf.maxs[0]) * 0.5F, (lf.mins[1] + lf.maxs[1]) * 0.5F, (lf.mins[2] + lf.maxs[2]) * 0.5F};
            if (punktGebiet(g_app->geo, m) != lf.area) { continue; }
            for (int k = 0; k < 3; ++k) { g_app->cam.pos[k] = m[k]; }
            break;
        }
        g_app->followCam = false;
        g_app->throughCamera = false;
        g_app->playing = false;
        for (auto& f : g_app->figurGezeichnet) { f = {-99999.0F, 0.0F, 0.0F}; }
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(pruefe("Teil 8: SV_BuildClientSnapshot: Der Spieler bleibt im Bild, wenn die Kamera in einem anderen Gebiet steht", [] {
        const float start[3] = {-1024.0F, 7320.0F, 24.0F};
        std::size_t spieler = g_app->scene.actors.size();
        for (std::size_t i = 0; i < g_app->scene.actors.size(); ++i) {
            if (g_app->scene.actors[i].echterSpieler) { spieler = i; }
        }
        const bool anders = punktGebiet(g_app->geo, g_app->cam.pos) != punktGebiet(g_app->geo, start);
        diag::detail("Knopftest: Teil 8 Spieler - Figuren " + std::to_string(g_app->scene.actors.size()) + ", Gebiet Kamera " + std::to_string(punktGebiet(g_app->geo, g_app->cam.pos)) +
                     ", Spieler " + std::to_string(punktGebiet(g_app->geo, start)));
        return anders && spieler < g_app->figurGezeichnet.size() && g_app->figurGezeichnet[spieler][0] > -90000.0F;
    }));
    add(tu("Teil 8: t3_byss laden", [] { (void)loadMapFromArchive("maps/t3_byss.bsp"); }));
    add(warteBis("Teil 8: t3_byss bereit", [] { return g_app->map.path == "maps/t3_byss.bsp" && g_app->mapBereit; }, 4000));
    add(pruefe("Teil 8: t3_byss, alle Tueren zu: Beide Fenster liegen ausserhalb des Rechtecks, dort bleibt der Bildpuffer", [] {
        const float kam[4] = {792.57F, 168.57F, 76.25F, 225.0F};
        const teil8::HimmelProbe zu = teil8::himmelProbe(kam, false);
        const teil8::HimmelProbe auf = teil8::himmelProbe(kam, true);
        diag::detail("Knopftest: Teil 8 Himmel t3_byss - Rechtecke " + std::to_string(zu.rechtecke.size()) + ", offen " + std::to_string(auf.rechtecke.size()) + ", Flaechen zu " +
                     std::to_string(zu.flaechen) + ", offen " + std::to_string(auf.flaechen));
        return !teil8::deckt(zu, kam, 810, 300) && !teil8::deckt(zu, kam, 1670, 380);
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
