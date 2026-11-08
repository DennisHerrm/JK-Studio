// selbsttest_geschuetze.inc - rc608: die Geschuetze im laufenden Programm (Modus "geschuetze").
//
// Eigene Missionen auf der Geometrie von md_am_sith (wie Modus "pruefung3"), je Punkt eine
// Probe gegen die Formel der Engine (g_turret.cpp, g_missile.cpp, g_combat.cpp OpenJK/MD):
//
// 1. misc_sentry_turret: Aufklappen, Feind finden, drehen (pas_think), Schuesse ab
//    attackDebounceTime; die Figur stirbt am Schaden (G_MissileImpacted -> G_Damage); danach
//    haelt das Geschuetz an. Effekte, Geschosse in der Luft.
// 2. SET_NOTARGET haelt es zurueck, bis das Skript es aufhebt.
// 3. misc_turret: Grenzen von turret_aim, erster Schuss nach startup; misc_turretG2 nur unter
//    Movie Duels (umschaltbar).
// 4. Schaden an den Entities (fx_target_beam "damage"): Wachgeschuetz stirbt (target,
//    turret/explode, G_FreeEntity), misc_model_breakable zerbricht und seine Explosion
//    (G_RadiusDamage) trifft die Figur, misc_trip_mine explodiert, ein func_breakable der Karte
//    zerbricht (Endrunde rc608: funcBBrushPain/funcBBrushDie, G_FreeEntity).
// 5. Turbolaser (misc_turretG2 spawnflags 8, Movie Duels: SP_misc_turretG2 mit MP_RULES): Werte aus
//    SP_misc_turretG2 (wait 500, damage 10, splashDamage 200), turb_muzzle_flash, Muendungen
//    abwechselnd (alt_fire, TurboLaser_SetBoneAnim 0/1 und 2/3), die Figur stirbt (Endrunde rc608).
// Fotos nur mit BHED_FOTOS. Mit BHED_GV_PK3 (Testkarte gv_turm in einem pk3) nur die Fotos fuer den
// Vergleich mit dem Spiel, mit BHED_GESCHUETZ_BILD nur das Bild fuer das Handbuch.
namespace geschuetze_probe {

// Das Skript einer Figur: steht still, Team Feind (damit jedes Geschuetz sie sieht), Lebenspunkte.
std::string ziel(const std::string& name, int leben, const std::string& mehr = {}, int wartenMs = 12000) {
    return "affect ( \"" + name + "\", /*@AFFECT_TYPE*/ FLUSH )\r\n{\r\n"
           "\tset ( /*@SET_TYPES*/ \"SET_BEHAVIOR_STATE\", \"BS_CINEMATIC\" );\r\n"
           "\tset ( /*@SET_TYPES*/ \"SET_PLAYER_TEAM\", \"TEAM_ENEMY\" );\r\n"
           "\tset ( /*@SET_TYPES*/ \"SET_HEALTH\", " + std::to_string(leben) + " );\r\n" + mehr +
           "\twait ( " + std::to_string(wartenMs) + ".000 );\r\n}\r\n\r\n";
}

const GeschuetzLauf* lauf(int entity) {
    for (const GeschuetzLauf& l : g_app->geschuetzSim.laeufe()) {
        if (l.daten.nummer == entity) { return &l; }
    }
    return nullptr;
}

int neue(const char* klasse, const float o[3], const std::vector<std::pair<std::string, std::string>>& keys) {
    const int e = neueEntityAn(klasse, o);
    if (e < 0) { return e; }
    for (const auto& [k, v] : keys) { (void)entityKeySetzen(e, k, v); }
    return e;
}

}  // namespace geschuetze_probe

std::vector<Schritt> geschuetzSimSchritte() {
    namespace gp = geschuetze_probe;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<pruefung3::Zustand>();
    auto regelVorher = std::make_shared<int>(0);
    // Die Nummern der neuen Entities: die Grundkarte bringt eigene Geschuetze und Fallen mit.
    auto pas = std::make_shared<int>(-1);
    auto turm = std::make_shared<int>(-1);
    auto turmG2 = std::make_shared<int>(-1);
    auto kisteNr = std::make_shared<int>(-1);
    auto falleNr = std::make_shared<int>(-1);
    add(tu("Geschuetze: Ausgangslage (eingerichtete Spielordner mit Movie Duels)", [=] {
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
    add(tu("Geschuetze: Grundkarte md_am_sith laden", [=] {
        mission1::ungespeicherteEntitiesVergessen();
        missionenAusArchiv(z->mdArchiv, "md_am_sith");
    }));
    add(warteBis("Geschuetze: Grundkarte geladen", [] {
        return g_app->map.path == "maps/md_am_sith.bsp" && g_app->camTrackValid && jk2probe::figurenGeladen();
    }, 4000));
    add(tu("Geschuetze: begehbaren Ort und NPC_type einer laufenden Figur merken", [=] {
        prepareScene();
        for (std::size_t ai = 0; ai < g_app->scene.actors.size() && z->typ.empty(); ++ai) {
            const Actor& a = g_app->scene.actors[ai];
            for (const ActorStep& st : a.steps) {
                if (z->typ.empty() && st.kind == ActorStep::Kind::WalkTo && !a.npcType.empty() && ai < g_app->actorAssets.size() &&
                    g_app->actorAssets[ai] && !g_app->actorAssets[ai]->model.empty()) {
                    for (int k = 0; k < 3; ++k) { z->ort[k] = st.from[k]; }
                    z->typ = a.npcType;
                }
            }
        }
        diag::detail("Knopftest: Geschuetze - Ort " + mission1::v3(z->ort) + ", NPC_type " + z->typ);
    }));
    const auto warteSzene = [&](const char* name, const char* figur) {
        add(warteBis(std::string("Geschuetze: Szene von ") + name, [=] {
            return !z->bereit || (g_app->camTrackValid && pruefung3::figur(figur) >= 0 && jk2probe::figurenGeladen());
        }, 3000));
        add(pause(0.5));
    };

    // --- 1. misc_sentry_turret gegen eine Figur -------------------------------------------
    add(tu("Geschuetze 1: Mission g1 - Wachgeschuetz 160 neben der Figur (10 Lebenspunkte)", [=] {
        pruefung3::szeneBauen(*z, "g1pas", {{"g1_ziel", 0.0F, 0.0F}}, gp::ziel("g1_ziel", 10));
        const float o[3] = {z->ort[0] + 96.0F, z->ort[1] - 96.0F, z->ort[2] - 24.0F};
        *pas = gp::neue("misc_sentry_turret", o, {{"angles", "0 135 0"}});
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    warteSzene("g1", "g1_ziel");
    add(pruefe("Geschuetze 1: pas_think - aufklappen bei 800, Feind bei 2000, erster Schuss 900..1100 + Zufallsbild danach", [=] {
        prepareScene();
        const GeschuetzLauf* l = gp::lauf(*pas);
        if (l == nullptr || l->aufklappen.empty()) { return false; }
        const double erster = l->schuesse.empty() ? -1.0 : l->schuesse.front().ms;
        diag::detail("Knopftest: Geschuetze 1 - aufgeklappt " + std::to_string(l->aufklappen.front().first) + ", Schuesse " +
                     std::to_string(l->schuesse.size()) + ", erster " + std::to_string(erster));
        return std::fabs(l->aufklappen.front().first - 800.0) < 0.5 && erster >= 2900.0 - 1e-6 && erster < 3500.0;
    }));
    add(pruefe("Geschuetze 1: bone_hinge zeigt auf renderInfo.eyePoint - 40 (AngleSubtract < 1 Grad), hoechstens 10 Grad je think", [=] {
        const GeschuetzLauf* l = gp::lauf(*pas);
        const int fi = pruefung3::figur("g1_ziel");
        if (l == nullptr || fi < 0) { return false; }
        const Actor& a = g_app->scene.actors[static_cast<std::size_t>(fi)];
        float auge[3];
        const ActorState st = a.at(3500.0);
        for (int q = 0; q < 3; ++q) { auge[q] = st.pos[q]; }
        if (static_cast<std::size_t>(fi) < g_app->actorAssets.size() && g_app->actorAssets[static_cast<std::size_t>(fi)]) {
            (void)figurAugenPunkt(a, *g_app->actorAssets[static_cast<std::size_t>(fi)], 3500.0, auge);
        }
        const float dir[3] = {auge[0] - l->ursprung[0], auge[1] - l->ursprung[1], auge[2] - 40.0F - l->ursprung[2]};
        float w[3];
        engineVectoangles(dir, w);
        const GeschuetzStand* letzter = nullptr;
        float groesster = 0.0F;
        float vorher = 0.0F;
        bool hat = false;
        for (const GeschuetzStand& x : l->staende) {
            if (!x.gedacht || x.ms > 3500.0) { continue; }
            if (hat) { groesster = std::max(groesster, std::fabs(winkelAbzug(x.pasGier, vorher))); }
            vorher = x.pasGier;
            hat = true;
            letzter = &x;
        }
        if (letzter == nullptr) { return false; }
        const float rest = std::fabs(winkelAbzug(letzter->pasGier + l->winkel[1], w[1]));
        diag::detail("Knopftest: Geschuetze 1 - Gier " + pruefung3::grad(letzter->pasGier + l->winkel[1]) + " gegen " + pruefung3::grad(w[1]) +
                     ", groesster Schritt " + pruefung3::grad(groesster));
        return rest < 1.0F && groesster <= 10.0F + 1e-3F;
    }));
    add(pruefe("Geschuetze 1: der Schaden toetet die Figur, das Geschuetz haelt danach an (bounceCount < 650 ms), Effekte und Geschosse", [=] {
        const GeschuetzLauf* l = gp::lauf(*pas);
        const int fi = pruefung3::figur("g1_ziel");
        if (l == nullptr || fi < 0 || l->schuesse.empty()) { return false; }
        const double tod = g_app->scene.actors[static_cast<std::size_t>(fi)].getoetetMs;
        const double letzter = l->schuesse.back().ms;
        std::size_t inDerLuft = 0;
        std::vector<EffectInstance> fx;
        geschuetzGeschosseZeichnen(l->schuesse.front().ms + 60.0, fx);
        inDerLuft = fx.size();
        std::size_t festeFx = 0;
        for (const EffectInstance& x : g_app->effectRunners) {
            if (x.ausGeschuetz) { ++festeFx; }
        }
        // Die Richtung des ersten Geschosses ist in den ersten 75 ms verkuerzt.
        float laenge = 0.0F;
        if (!fx.empty()) {
            for (const float v : fx.front().normal) { laenge += v * v; }
            laenge = std::sqrt(laenge);
        }
        diag::detail("Knopftest: Geschuetze 1 - Tod " + std::to_string(tod) + ", letzter Schuss " + std::to_string(letzter) + ", Schuesse " +
                     std::to_string(g_app->geschuetzSchuesse) + ", Einschlaege " + std::to_string(g_app->geschuetzEinschlaege) +
                     ", in der Luft " + std::to_string(inDerLuft) + ", Laenge " + pruefung3::grad(laenge) + ", feste Effekte " + std::to_string(festeFx));
        return tod > 0.0 && letzter <= tod + 650.0 + 100.0 && g_app->geschuetzEinschlaege > 0 && inDerLuft > 0 && festeFx > 0 &&
               std::fabs(laenge - (60.0F / 75.0F * 0.95F + 0.05F)) < 0.02F;
    }));
    add(tu("Geschuetze 1: Foto bei einem Schuss", [=] {
        const GeschuetzLauf* l = gp::lauf(*pas);
        g_app->followCam = false;
        g_app->cam.pos[0] = z->ort[0] + 80.0F;
        g_app->cam.pos[1] = z->ort[1] - 150.0F;
        g_app->cam.pos[2] = z->ort[2] + 40.0F;
        g_app->cam.angles[1] = 90.0F;
        g_app->cam.angles[0] = 8.0F;
        g_app->playMs = l != nullptr && !l->schuesse.empty() ? l->schuesse.front().ms + 40.0 : 3000.0;
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(foto("geschuetz_wache_schuss"));

    // --- 2. SET_NOTARGET ---------------------------------------------------------------------
    add(tu("Geschuetze 2: Mission g2 - die Figur ist bis 4000 ms kein Ziel", [=] {
        pruefung3::szeneBauen(*z, "g2ziel", {{"g2_ziel", 0.0F, 0.0F}},
                              gp::ziel("g2_ziel", 10000,
                                       "\tset ( /*@SET_TYPES*/ \"SET_NOTARGET\", \"true\" );\r\n\twait ( 4000.000 );\r\n"
                                       "\tset ( /*@SET_TYPES*/ \"SET_NOTARGET\", \"false\" );\r\n", 4000));
        const float o[3] = {z->ort[0] + 96.0F, z->ort[1] - 96.0F, z->ort[2] - 24.0F};
        *pas = gp::neue("misc_sentry_turret", o, {{"angles", "0 135 0"}});
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    warteSzene("g2", "g2_ziel");
    add(pruefe("Geschuetze 2: FL_NOTARGET - kein Schuss vor 4000 + 900 ms, danach feuert es", [=] {
        prepareScene();
        const GeschuetzLauf* l = gp::lauf(*pas);
        if (l == nullptr) { return false; }
        const double erster = l->schuesse.empty() ? -1.0 : l->schuesse.front().ms;
        diag::detail("Knopftest: Geschuetze 2 - erster Schuss " + std::to_string(erster));
        return erster >= 4900.0 - 1e-6;
    }));

    // --- 3. misc_turret, misc_turretG2 ------------------------------------------------------
    add(tu("Geschuetze 3: Mission g3 - misc_turret (team player) und misc_turretG2 ueber der Figur", [=] {
        pruefung3::szeneBauen(*z, "g3turm", {{"g3_ziel", 0.0F, 0.0F}}, gp::ziel("g3_ziel", 40));
        const float o[3] = {z->ort[0] + 96.0F, z->ort[1] - 64.0F, z->ort[2] + 60.0F};
        *turm = gp::neue("misc_turret", o, {{"team", "player"}, {"angle", "180"}});
        const float o2[3] = {z->ort[0] + 96.0F, z->ort[1] + 64.0F, z->ort[2] + 60.0F};
        *turmG2 = gp::neue("misc_turretG2", o2, {{"alliedTeam", "1"}, {"angle", "180"}});
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    warteSzene("g3", "g3_ziel");
    add(pruefe("Geschuetze 3: turret_base_think - erster Schuss frei 300 + 1400, s.apos hoechstens 14 Grad x 5 je think, Bone_body 3 Grad, s.frame 0", [=] {
        prepareScene();
        const GeschuetzLauf* l = gp::lauf(*turm);
        if (l == nullptr) { return false; }
        const double erster = l->schuesse.empty() ? -1.0 : l->schuesse.front().ms;
        float maxDelta = 0.0F;
        float maxNick = 0.0F;
        float nickVorher = 0.0F;
        for (const GeschuetzStand& x : l->staende) {
            if (!x.gedacht) { continue; }
            maxDelta = std::max(maxDelta, std::fabs(x.aposDelta[1]));
            maxNick = std::max(maxNick, std::fabs(winkelAbzug(x.nick, nickVorher)));
            nickVorher = x.nick;
        }
        diag::detail("Knopftest: Geschuetze 3 - erster Schuss " + std::to_string(erster) + ", Schuesse " + std::to_string(l->schuesse.size()) +
                     ", groesstes trDelta " + pruefung3::grad(maxDelta) + ", groesster Nickschritt " + pruefung3::grad(maxNick) + ", Bild " +
                     std::to_string(l->staende.back().bild));
        return erster >= 1700.0 - 1e-6 && erster < 2300.0 && maxDelta > 0.0F && maxDelta <= 70.0F + 0.05F && maxNick > 0.0F && maxNick <= 3.0F + 0.05F &&
               l->staende.back().bild == 0;
    }));
    add(pruefe("Geschuetze 3: Movie Duels - misc_turretG2 ist dabei und schiesst (ohne MP-Spur vor dem Feind), die Figur stirbt", [=] {
        const GeschuetzLauf* l = gp::lauf(*turmG2);
        const int fi = pruefung3::figur("g3_ziel");
        const double tod = fi >= 0 ? g_app->scene.actors[static_cast<std::size_t>(fi)].getoetetMs : -1.0;
        diag::detail("Knopftest: Geschuetze 3 - G2 " + std::string(l != nullptr ? std::to_string(l->schuesse.size()) : "fehlt") + ", Tod " + std::to_string(tod));
        return l != nullptr && !l->schuesse.empty() && tod > 0.0;
    }));
    add(tu("Geschuetze 3: Foto des Turms im Feuer", [=] {
        const GeschuetzLauf* l = gp::lauf(*turm);
        g_app->followCam = false;
        g_app->cam.pos[0] = z->ort[0] + 20.0F;
        g_app->cam.pos[1] = z->ort[1] - 190.0F;
        g_app->cam.pos[2] = z->ort[2] + 60.0F;
        g_app->cam.angles[1] = 75.0F;
        g_app->cam.angles[0] = 2.0F;
        g_app->playMs = l != nullptr && l->schuesse.size() > 3U ? l->schuesse[3].ms + 40.0 : 3000.0;
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(foto("geschuetz_turm_schuss"));
    add(tu("Geschuetze 3: Regelwerk OpenJK", [=] {
        g_app->settings.regelwerk = kRegelwerkOpenJK;
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    add(warteBis("Geschuetze 3: Szene unter OpenJK", [=] {
        return g_app->camTrackValid && jk2probe::figurenGeladen();
    }, 3000));
    add(pause(0.5));
    add(pruefe("Geschuetze 3: OpenJK - misc_turretG2 gibt es dort nicht, misc_turret bleibt (je Regelwerk umschaltbar)", [=] {
        prepareScene();
        const bool g2 = gp::lauf(*turmG2) != nullptr;
        const bool turmDa = gp::lauf(*turm) != nullptr;
        diag::detail("Knopftest: Geschuetze 3 - OpenJK: G2 " + std::to_string(g2) + ", misc_turret " + std::to_string(turmDa));
        return !g2 && turmDa;
    }));
    add(tu("Geschuetze 3: Regelwerk zurueck", [=] {
        g_app->settings.regelwerk = *regelVorher;
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    add(warteBis("Geschuetze 3: Szene wieder da", [=] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 3000));

    // --- 4. Schaden an Entities (je eine Mission, damit nichts das andere ausloest) -----------
    const auto strahl = [](const float mitte[3], const char* name) {
        float a[3] = {mitte[0], mitte[1] - 40.0F, mitte[2]};
        float b[3] = {mitte[0], mitte[1] + 40.0F, mitte[2]};
        (void)gp::neue("info_notnull", b, {{"targetname", name}});
        (void)gp::neue("fx_target_beam", a, {{"target", name}, {"damage", "30"}, {"wait", "0.5"}});
    };
    add(tu("Geschuetze 4a: Mission g4a - Strahl (damage 30) auf ein Wachgeschuetz mit target", [=] {
        pruefung3::szeneBauen(*z, "g4apas", {{"g4_ziel", 0.0F, 0.0F}}, gp::ziel("g4_ziel", 1000, {}, 5000));
        const float o[3] = {z->ort[0] + 96.0F, z->ort[1] - 96.0F, z->ort[2] - 24.0F};
        *pas = gp::neue("misc_sentry_turret", o, {{"angles", "0 0 0"}, {"target", "g4_tot"}, {"radius", "1"}});
        (void)gp::neue("target_relay", z->ort, {{"targetname", "g4_tot"}});
        const float mitte[3] = {o[0], o[1], o[2] + 9.0F};
        strahl(mitte, "g4_a");
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    warteSzene("g4a", "g4_ziel");
    add(pruefe("Geschuetze 4a: fx_target_beam toetet das Wachgeschuetz (health 50, zwei Treffer): turret/explode, target, weg", [=] {
        prepareScene();
        const GeschuetzLauf* l = gp::lauf(*pas);
        if (l == nullptr) { return false; }
        bool benutzt = false;
        for (const Ablauf::Benutzung& b : g_app->ablauf.benutzt) {
            if (b.name == "g4_tot") { benutzt = std::fabs(b.ms - l->totMs) < 100.5; }
        }
        std::string fx;
        for (const GeschuetzEffekt& e : l->effekte) { fx += std::string(e.effekt) + " "; }
        diag::detail("Knopftest: Geschuetze 4a - tot " + std::to_string(l->totMs) + ", frei " + std::to_string(l->freiMs) + ", Effekte " + fx + ", target " + std::to_string(benutzt));
        return l->totMs > 0.0 && l->freiMs == l->totMs && benutzt && fx.find("turret/explode") != std::string::npos;
    }));
    add(tu("Geschuetze 4b: Mission g4b - Strahl auf ein misc_model_breakable mit Explosion neben der Figur", [=] {
        pruefung3::szeneBauen(*z, "g4bkiste", {{"g4_ziel", 0.0F, 0.0F}}, gp::ziel("g4_ziel", 40, {}, 5000));
        const float kiste[3] = {z->ort[0] + 64.0F, z->ort[1], z->ort[2]};
        *kisteNr = gp::neue("misc_model_breakable", kiste,
                            {{"model", "models/map_objects/imp_mine/crate_open.md3"}, {"health", "50"}, {"splashDamage", "90"}, {"splashRadius", "256"}});
        strahl(kiste, "g4_b");
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    warteSzene("g4b", "g4_ziel");
    add(pruefe("Geschuetze 4b: misc_model_breakable zerbricht am Strahl, G_RadiusDamage (90, 256, CanDamage) trifft die Figur: 40 Lebenspunkte sind weg", [=] {
        prepareScene();
        const int fi = pruefung3::figur("g4_ziel");
        if (*kisteNr < 0 || fi < 0) { return false; }
        double bruch = -1.0;
        for (const AblaufWelt::SchadenBruch& b : g_app->schadenBrueche) {
            if (b.entity == *kisteNr) { bruch = b.ms; }
        }
        const MoverSim::ModellStand vor = g_app->moverSim.modellAt(static_cast<std::size_t>(*kisteNr), 100.0);
        const MoverSim::ModellStand nach = g_app->moverSim.modellAt(static_cast<std::size_t>(*kisteNr), bruch + 100.0);
        const double tod = g_app->scene.actors[static_cast<std::size_t>(fi)].getoetetMs;
        diag::detail("Knopftest: Geschuetze 4b - Bruch " + std::to_string(bruch) + ", Modell vorher \"" + vor.modell + "\", danach \"" + nach.modell + "\" sichtbar " +
                     std::to_string(nach.sichtbar) + ", Tod " + std::to_string(tod));
        return bruch > 0.0 && std::fabs(tod - bruch) < 100.5 && (!nach.sichtbar || nach.modell != vor.modell);
    }));
    add(tu("Geschuetze 4c: Mission g4c - Strahl auf eine misc_trip_mine am Boden", [=] {
        pruefung3::szeneBauen(*z, "g4cfalle", {{"g4_ziel", 0.0F, 0.0F}}, gp::ziel("g4_ziel", 1000, {}, 5000));
        // Die Falle liegt am Boden (angles 90 0 0: die Spur 128 nach unten).
        const float falle[3] = {z->ort[0] + 96.0F, z->ort[1] + 64.0F, z->ort[2]};
        *falleNr = gp::neue("misc_trip_mine", falle, {{"angles", "90 0 0"}, {"count", "-1"}});
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    warteSzene("g4c", "g4_ziel");
    add(tu("Geschuetze 4c: der Strahl durch die Falle, dort, wo sie liegt (tripMineLage)", [=] {
        prepareScene();
        for (const App::GegenstandGlm& gg : g_app->gegenstandGlm) {
            if (gg.entity == static_cast<std::size_t>(*falleNr)) {
                diag::detail("Knopftest: Geschuetze 4c - Falle bei " + mission1::v3(gg.ort));
                const float mitte[3] = {gg.ort[0], gg.ort[1], gg.ort[2] + 1.0F};
                strahl(mitte, "g4_c");
            }
        }
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    warteSzene("g4c", "g4_ziel");
    add(pruefe("Geschuetze 4c: misc_trip_mine (health 15) explodiert beim ersten Treffer", [=] {
        prepareScene();
        if (*falleNr < 0) { return false; }
        const auto it = g_app->fallenFremd.find(*falleNr);
        bool knall = false;
        for (const App::FallenKnall& k : g_app->fallenKnalle) { knall = knall || k.entity == *falleNr; }
        diag::detail("Knopftest: Geschuetze 4c - Falle: " + std::string(it != g_app->fallenFremd.end() ? std::to_string(it->second) : "kein Treffer") + ", Knall " +
                     std::to_string(knall));
        return it != g_app->fallenFremd.end() && knall;
    }));

    // --- 4d. func_breakable der Karte als Ziel (Endrunde rc608) -------------------------------
    // Der naechste func_breakable mit Brush-Modell; health 50, spawnflags 0 (nicht nur Schwert/schwere Waffen):
    // zwei Treffer zu 30 (funcBBrushPain, dann funcBBrushDie -> G_FreeEntity), danach ist das Modell weg.
    auto bruchNr = std::make_shared<int>(-1);
    auto bruchModell = std::make_shared<int>(-1);
    add(tu("Geschuetze 4d: Mission g4d - Strahl (damage 30) durch den naechsten func_breakable der Karte", [=] {
        pruefung3::szeneBauen(*z, "g4dbruch", {{"g4_ziel", 0.0F, 0.0F}}, gp::ziel("g4_ziel", 1000, {}, 5000));
        float bester = 1.0e30F;
        float lo[3]{}, hi[3]{};
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            const MapEntity& e = g_app->map.entities[i];
            const std::string* m = e.find("model");
            if (e.classname != "func_breakable" || m == nullptr || m->size() < 2 || (*m)[0] != '*') { continue; }
            const int nr = std::atoi(m->c_str() + 1);
            if (nr <= 0 || static_cast<std::size_t>(nr) >= g_app->geo.models.size()) { continue; }
            const BspGeometry::SubModel& sm = g_app->geo.models[static_cast<std::size_t>(nr)];
            float d = 0.0F;
            for (int q = 0; q < 3; ++q) { d += std::pow(0.5F * (sm.mins[q] + sm.maxs[q]) - z->ort[q], 2.0F); }
            if (d < bester) {
                bester = d;
                *bruchNr = static_cast<int>(i);
                *bruchModell = nr;
                for (int q = 0; q < 3; ++q) {
                    lo[q] = sm.mins[q];
                    hi[q] = sm.maxs[q];
                }
            }
        }
        if (*bruchNr < 0) { return; }
        (void)entityKeySetzen(*bruchNr, "health", "50");
        (void)entityKeySetzen(*bruchNr, "spawnflags", "0");
        // Der Strahl quer durch die duennste waagerechte Richtung, Anfang und Ziel 16 ausserhalb des Kastens
        const int achse = (hi[0] - lo[0]) <= (hi[1] - lo[1]) ? 0 : 1;
        float a[3], b[3];
        for (int q = 0; q < 3; ++q) { a[q] = b[q] = 0.5F * (lo[q] + hi[q]); }
        a[achse] = lo[achse] - 16.0F;
        b[achse] = hi[achse] + 16.0F;
        diag::detail("Knopftest: Geschuetze 4d - func_breakable Entity " + std::to_string(*bruchNr) + " *" + std::to_string(*bruchModell) + ", Strahl " +
                     mission1::v3(a) + " -> " + mission1::v3(b));
        (void)gp::neue("info_notnull", b, {{"targetname", "g4_d"}});
        (void)gp::neue("fx_target_beam", a, {{"target", "g4_d"}, {"damage", "30"}, {"wait", "0.5"}});
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    warteSzene("g4d", "g4_ziel");
    add(pruefe("Geschuetze 4d: func_breakable (health 50) zerbricht beim zweiten Treffer, sein Modell ist danach weg", [=] {
        prepareScene();
        if (*bruchNr < 0) { return false; }
        double bruch = -1.0;
        for (const AblaufWelt::SchadenBruch& b : g_app->schadenBrueche) {
            if (b.entity == *bruchNr) { bruch = b.ms; }
        }
        const MoverSim::Stand vor = g_app->moverSim.at(*bruchModell, std::max(0.0, bruch - 100.0));
        const MoverSim::Stand nach = g_app->moverSim.at(*bruchModell, bruch + 100.0);
        diag::detail("Knopftest: Geschuetze 4d - Bruch " + std::to_string(bruch) + ", vorher sichtbar " + std::to_string(vor.sichtbar) + ", danach " +
                     std::to_string(nach.sichtbar));
        return bruch >= 450.0 && vor.sichtbar && !nach.sichtbar;   // der erste Treffer (0 ms) bricht ihn nicht, der zweite (wait 0.5) schon
    }));

    // --- 5. Turbolaser (Endrunde rc608) ------------------------------------------------------
    auto turbo = std::make_shared<int>(-1);
    add(tu("Geschuetze 5: Mission g5 - misc_turretG2 mit spawnflags 8 (Turbolaser) ueber der Figur", [=] {
        pruefung3::szeneBauen(*z, "g5turbo", {{"g5_ziel", 0.0F, 0.0F}}, gp::ziel("g5_ziel", 40));
        const float o[3] = {z->ort[0] + 96.0F, z->ort[1] + 64.0F, z->ort[2] + 60.0F};
        *turbo = gp::neue("misc_turretG2", o, {{"spawnflags", "8"}, {"angle", "180"}});
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    warteSzene("g5", "g5_ziel");
    add(pruefe("Geschuetze 5: Turbolaser - turb_muzzle_flash alle 500 ms, Muendungen abwechselnd (Knochen 0/1, 2/3), die Figur stirbt", [=] {
        prepareScene();
        const GeschuetzLauf* l = gp::lauf(*turbo);
        const int fi = pruefung3::figur("g5_ziel");
        if (l == nullptr || fi < 0 || l->schuesse.size() < 2U) { return false; }
        bool effekt = true;
        double kleinsterAbstand = 1.0e30;
        for (std::size_t k = 0; k < l->schuesse.size(); ++k) {
            effekt = effekt && std::string(l->schuesse[k].effekt) == "turret/turb_muzzle_flash";
            if (k > 0) { kleinsterAbstand = std::min(kleinsterAbstand, l->schuesse[k].ms - l->schuesse[k - 1].ms); }
        }
        std::string anims;
        bool wechselt = true;
        int vorher = -1;
        for (const auto& [ms, ab] : l->turboAnims) {
            anims += std::to_string(ab.first) + "/" + std::to_string(ab.second) + " ";
            if (ab.first == 4) { continue; }
            wechselt = wechselt && ab.first != vorher;
            vorher = ab.first;
        }
        const double tod = g_app->scene.actors[static_cast<std::size_t>(fi)].getoetetMs;
        diag::detail("Knopftest: Geschuetze 5 - Schuesse " + std::to_string(l->schuesse.size()) + ", kleinster Abstand " + std::to_string(kleinsterAbstand) +
                     ", Effekt " + l->schuesse.front().effekt + ", Knochen " + anims + ", Tod " + std::to_string(tod));
        return l->turbo && effekt && kleinsterAbstand >= 500.0 - 1e-6 && wechselt && vorher >= 0 && tod > 0.0;
    }));
    return s;
}

// Vergleich mit dem Spiel: dieselbe Testkarte wie dort (BHED_GV_KARTE), der Spieler ist das Ziel, die
// Kamera wie im Spiel (BHED_GV_KAMERA "x y z nick gier", cg_fov 80 bei 4:3). BHED_GV_ANSICHT "1920x1440":
// das Fenster so gross, dass die Kartenansicht mindestens so gross ist und nichts skaliert werden muss.
// Mit Wiedergabe bei Tempo 0, damit das Bild wie beim Abspielen keine Bearbeitungshilfen zeigt.
std::vector<Schritt> geschuetzVergleichSchritte(const std::string& archiv) {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const char* const kk = std::getenv("BHED_GV_KARTE");
    const std::string karte = kk != nullptr ? kk : "gv_turm";
    float kam[5] = {269.7F, 6916.7F, 76.0F, 8.0F, 326.0F};
    if (const char* k = std::getenv("BHED_GV_KAMERA")) { (void)std::sscanf(k, "%f %f %f %f %f", &kam[0], &kam[1], &kam[2], &kam[3], &kam[4]); }
    // MR7 P42: das Regelwerk vor dem Vergleich ("Auto" aus den Einstellungen) - am Ende wieder so
    auto regelAnfang = std::make_shared<int>(0);
    add(tu("Geschuetze Vergleich: Karte " + karte, [=] {
        *regelAnfang = g_app->settings.regelwerk;
        if (const char* f = std::getenv("BHED_GV_FENSTER")) {
            int b = 0;
            int h = 0;
            if (std::sscanf(f, "%dx%d", &b, &h) == 2) { (void)platform::hauptfensterGroesse(b, h); }
        }
        g_app->leftMode = 1;
        g_app->missionPickOpen = false;
        if (std::getenv("BHED_GV_OPENJK") != nullptr) { g_app->settings.regelwerk = kRegelwerkOpenJK; }
        mission1::ungespeicherteEntitiesVergessen();
        missionenAusArchiv(archiv, karte);
    }));
    add(warteBis("Geschuetze Vergleich: Karte geladen", [=] { return g_app->map.path == "maps/" + karte + ".bsp"; }, 4000));
    // rc608 Teil 10: BHED_GV_GLUEHEN=0 wie r_DynamicGlow 0 im Spiel (Byss-Lichtleiste ohne Gluehen vergleichen).
    if (const char* gl = std::getenv("BHED_GV_GLUEHEN"); gl != nullptr && gl[0] == '0') {
        add(tu("Geschuetze Vergleich: ohne Gluehen", [] { g_app->showGlow = false; }));
    }
    // rc608 Teil 10b: BHED_GV_SCHWERTLICHT="<cg_saberLightIntensity> <cg_saberLightRadius>" wie in der Konfiguration
    // des verglichenen Spiels (die Aufnahme-Konfiguration im SP hat 1.2 1.5 aus dem Einstellungsmenue von Movie Duels).
    if (const char* sl = std::getenv("BHED_GV_SCHWERTLICHT")) {
        float i = 0.0F;
        float r = 0.0F;
        if (std::sscanf(sl, "%f %f", &i, &r) == 2) {
            add(tu("Geschuetze Vergleich: Schwertlicht", [i, r] {
                g_app->settings.cgSaberLightIntensity = i;
                g_app->settings.cgSaberLightRadius = r;
            }));
        }
    }
    // rc608 Teil 10b: BHED_GV_DREHANIM=1 wie cg_turnAnims 1 im Spiel.
    if (const char* da = std::getenv("BHED_GV_DREHANIM"); da != nullptr && da[0] == '1') {
        add(tu("Geschuetze Vergleich: cg_turnAnims 1", [] { g_app->settings.cgTurnAnims = true; }));
    }
    if (const char* f = std::getenv("BHED_GV_ANSICHT")) {
        int b = 0;
        int h = 0;
        if (std::sscanf(f, "%dx%d", &b, &h) == 2) {
            // Die Seitenleisten wachsen mit dem Fenster: in Schritten nachfuehren.
            for (int runde = 0; runde < 8; ++runde) {
                add(tu("Geschuetze Vergleich: Ansicht " + std::to_string(runde), [=] {
                    const std::pair<int, int> jetzt = platform::hauptfensterGroesse(0, 0);
                    const int vw = std::max(1, g_app->mapViewW);
                    const int vh = std::max(1, g_app->mapViewH);
                    const int db = b - vw;
                    const int dh = h - vh;
                    // Die Leisten wachsen mit, also jede Runde um die Luecke: Das konvergiert ohne Ueberschwingen.
                    const int nb = jetzt.first + db;
                    const int nh = jetzt.second + dh;
                    (void)platform::hauptfensterGroesse(nb, nh);
                }));
                add(pause(0.5));
            }
        }
    }
    add(tu("Geschuetze Vergleich: der Spieler ist unsterblich", [=] {
        Script neu;
        std::vector<Diag> d;
        // BHED_GV_SKRIPT: ein eigenes Skript fuer den Vergleich (Tod, Holster).
        std::string text = "//Generated by BehavEd\r\n\r\naffect ( \"player\", /*@AFFECT_TYPE*/ FLUSH )\r\n{\r\n"
                           "\tset ( /*@SET_TYPES*/ \"SET_INVINCIBLE\", \"true\" );\r\n\twait ( 12000.000 );\r\n}\r\n";
        if (const char* datei = std::getenv("BHED_GV_SKRIPT")) { text = dateiInhalt(datei); }
        if (readScript(text, neu, d)) {
            g_app->doc.ersetzeSkript(std::move(neu), "Geschuetze Vergleich");
            rebuildTree();
        }
        g_app->followCam = false;
        g_app->camTrackValid = false;
        // rc608: Der Vergleich mit dem Spiel zeigt den Spieler wie im Spiel ("Show player" an), auch in Skripten mit Kamera.
        g_app->spielerInCutscene = true;
        g_app->mapDirty = true;
    }));
    add(warteBis("Geschuetze Vergleich: Szene", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 3000));
    const bool sauber = std::getenv("BHED_GV_ANSICHT") != nullptr;
    std::vector<int> zeiten = {4000, 4040, 6000, 6040};
    if (const char* z = std::getenv("BHED_GV_ZEITEN")) {
        zeiten.clear();
        std::istringstream liste(z);
        for (int ms = 0; liste >> ms;) { zeiten.push_back(ms); }
    }
    // BHED_GV_FAHRT "dx dy dz dnick dgier": die Kamera wandert je Bild um diese Werte (Bildfolgen gegen das Spiel).
    float fahrt[5] = {0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
    if (const char* f = std::getenv("BHED_GV_FAHRT")) {
        (void)std::sscanf(f, "%f %f %f %f %f", &fahrt[0], &fahrt[1], &fahrt[2], &fahrt[3], &fahrt[4]);
    }
    int bildNr = 0;
    for (const int ms : zeiten) {
        const float nr = static_cast<float>(bildNr++);
        add(tu("Geschuetze Vergleich: Bild bei " + std::to_string(ms), [=] {
            entityAuswahlSetzen(-1);
            g_app->cam.pos[0] = kam[0] + nr * fahrt[0];
            g_app->cam.pos[1] = kam[1] + nr * fahrt[1];
            g_app->cam.pos[2] = kam[2] + nr * fahrt[2];
            g_app->cam.angles[0] = kam[3] + nr * fahrt[3];
            g_app->cam.angles[1] = kam[4] + nr * fahrt[4];
            g_app->cam.angles[2] = 0.0F;
            g_app->cam.fovY = 64.366444F;   // CG_CalcFov: atan2(1440, 1920 / tan(40 Grad)) * 360 / pi (REST3b: vorher 64,3727, verrechnet)
            g_app->playMs = ms;
            g_app->playing = sauber;
            g_app->abspielTempo = sauber ? 0.0F : 1.0F;
            g_app->mapDirty = true;
        }));
        add(pause(0.5));
        add(tu("Geschuetze Vergleich: Werte bei " + std::to_string(ms), [] {
            char z[200];
            for (const auto& o : g_app->figurGezeichnet) {
                const GridLight l = sampleLightGrid(g_app->geo, o.data(), false, g_app->mapBrightness);
                std::snprintf(z, sizeof(z), "Geschuetze Vergleich: Figur bei %.1f %.1f %.1f, Licht %.1f %.1f %.1f / %.1f %.1f %.1f, Ansicht %d x %d", o[0], o[1], o[2], l.ambient[0], l.ambient[1],
                              l.ambient[2], l.directed[0], l.directed[1], l.directed[2], g_app->mapViewW, g_app->mapViewH);
                diag::detail(z);
            }
            // rc608 Teil 13d: Lage, Winkel und Haltung je Figur wie gezeichnet (Reiten gegen das Spiel, dort per
            // print(get(...)) im Skript gemessen).
            for (const Actor& a : g_app->scene.actors) {
                if (!a.haveStart) { continue; }
                ActorState st = a.at(g_app->playMs);
                fahrzeugLenken(static_cast<std::size_t>(&a - g_app->scene.actors.data()), g_app->playMs, st);
                ActorDraw rd;
                reitenAnwenden(a, g_app->playMs, st, rd);
                std::snprintf(z, sizeof(z), "Geschuetze Vergleich: %s bei %.1f %.1f %.1f, Winkel %.1f %.1f %.1f, %s%s%s", a.name.c_str(),
                              static_cast<double>(st.pos[0]), static_cast<double>(st.pos[1]), static_cast<double>(st.pos[2]),
                              static_cast<double>(st.angles[0]), static_cast<double>(st.angles[1]), static_cast<double>(st.angles[2]),
                              st.animation.c_str(), st.visible ? "" : " (verborgen)", a.fahrzeug.da ? (" " + a.fahrzeug.typ).c_str() : "");
                diag::detail(z);
            }
            std::snprintf(z, sizeof(z), "Geschuetze Vergleich: Bildzeit %.1f ms", static_cast<double>(1000.0F / std::max(ImGui::GetIO().Framerate, 0.1F)));
            diag::detail(z);
            std::snprintf(z, sizeof(z), "Geschuetze Vergleich: Ansicht von %.0f %.0f bis %.0f %.0f", static_cast<double>(g_app->kartenAnsicht[0]), static_cast<double>(g_app->kartenAnsicht[1]),
                          static_cast<double>(g_app->kartenAnsicht[2]), static_cast<double>(g_app->kartenAnsicht[3]));
            diag::detail(z);
        }));
        add(foto(karte + "_" + std::to_string(ms)));
    }
    // MR6 P20-P22 (BHED_GV_DXT=1): Texturkompression wie Upload32 (rd-vanilla tr_image.cpp:629-689) - unter Jedi Academy/Outcast
    // geht jedes Bild mit allowTC als DXT5 hoch, auch die Bilder von Shadern mit $lightmap-Stufe (R_FindImageFile aus ParseStage) und
    // die Himmelsseiten (ParseSkyParms, tr_shader.cpp:1970); unter Movie Duels keins. Danach wechselt das Regelwerk: die schon
    // hochgeladenen Bilder muessen neu hoch, nach der Regel des neuen Regelwerks.
    if (std::getenv("BHED_GV_DXT") != nullptr) {
        auto bezeichne = [](const char* was, const gpu::BilderDxt& b) {
            char z[220];
            std::snprintf(z, sizeof(z), "Knopftest: DXT %s: Bilder %d, davon DXT5 %d; mit $lightmap-Stufe %d, davon DXT5 %d; Himmelsseiten %d, davon DXT5 %d", was,
                          b.bilder, b.komprimiert, b.lightmapStufe, b.lightmapStufeKomprimiert, b.himmelSeiten, b.himmelKomprimiert);
            diag::info(z);
        };
        add(pruefe("Geschuetze Vergleich: Texturkompression wie das Spiel", [=] {
            const gpu::BilderDxt b = gpu::bilderDxt(true);
            const bool md = regelwerkMovieDuels();
            bezeichne(md ? "Movie Duels" : "JKA/JK2", b);
            // Himmel: sechs Seiten ohne und mit noTC (die Karte zeigt ihren Himmel nicht in jeder Kamera)
            const gpu::BilderDxt h = gpu::himmelProbe(false);
            const gpu::BilderDxt hn = gpu::himmelProbe(true);
            bezeichne("Himmelsprobe ohne noTC", h);
            bezeichne("Himmelsprobe mit noTC", hn);
            const bool himmel = h.himmelSeiten == 6 && h.himmelKomprimiert == (md ? 0 : 6) && hn.himmelKomprimiert == 0;
            if (md) { return himmel && b.bilder > 0 && b.komprimiert == 0 && b.himmelKomprimiert == 0; }
            return himmel && b.lightmapStufe > 0 && b.lightmapStufeKomprimiert == b.lightmapStufe && b.himmelKomprimiert == b.himmelSeiten;
        }));
        // MR8 Punkt 1: ein Bild je Name wie im Spiel - R_FindImageFile_NoLoad (rd-vanilla tr_image.cpp:936-971) gibt das Bild des
        // ERSTEN Ladens zurueck, mit dessen allowTC, mipmap und glWrapClampMode. Zwei Shader der Karte (die ersten beiden mit Bild in
        // der Reihenfolge von RE_LoadWorldMap: Nebel, dann die Flaechen der Datei) bekommen fuer die Probe ein Skript mit DEMSELBEN
        // Bild; je Fall hat einer noTC / nomipmaps / clampMap - einmal der erste, einmal der zweite. Wie das Spiel: beide Flaechen
        // zeigen dasselbe Bild (gleiche Bytes und Stufen auf der Grafikkarte), so wie der Erste es geladen hat.
        struct M8Probe {
            int platz[2] = {-1, -1};
            std::string name[2];
            std::string bild;
            bool alt[2] = {false, false};
            ShaderInfo altInfo[2];
        };
        auto m8 = std::make_shared<M8Probe>();
        const auto m8klein = [](std::string n) {
            for (char& c : n) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
            return n;
        };
        add(tu("Geschuetze Vergleich: MR8 - zwei Shader der Karte und ein Bild fuer beide", [m8, m8klein] {
            // Reihenfolge wie RE_LoadWorldMap: R_LoadFogs, dann R_LoadSurfaces (tr_bsp.cpp:1420-1427)
            std::vector<int> folge;
            for (const BspGeometry::Nebel& n : g_app->geo.nebel) {
                for (std::size_t i = 0; i < g_app->geo.shaders.size(); ++i) {
                    if (m8klein(g_app->geo.shaders[i].name) == m8klein(n.shader)) { folge.push_back(static_cast<int>(i)); }
                }
            }
            for (const BspSurface& f : g_app->geo.surfaces) {
                if (f.type != BspSurface::Type::Bad) { folge.push_back(f.shader); }
            }
            int k = 0;
            for (const int i : folge) {
                if (k == 2 || i < 0 || static_cast<std::size_t>(i) >= g_app->geo.shaders.size() ||
                    static_cast<std::size_t>(i) >= g_app->textures.byShader.size()) { continue; }
                const BspShader& sh = g_app->geo.shaders[static_cast<std::size_t>(i)];
                const TextureSet::Tex& t = g_app->textures.byShader[static_cast<std::size_t>(i)];
                if (!sh.isDrawn() || sh.isSky() || t.rgba.empty() || t.width < 16 || t.height < 16 || !t.skyFaces.empty() ||
                    (k == 1 && i == m8->platz[0])) { continue; }
                m8->platz[k] = i;
                m8->name[k] = sh.name;
                ++k;
            }
            if (k < 2) { return; }
            // das Bild des zweiten (Skript: seine Bildstufe, sonst der Name - R_FindShader tr_shader.cpp:3502)
            const auto it = g_app->shaderMap.find(m8klein(m8->name[1]));
            m8->bild = it != g_app->shaderMap.end() && !it->second.image.empty() ? it->second.image : m8->name[1];
            for (int j = 0; j < 2; ++j) {
                const auto a = g_app->shaderMap.find(m8klein(m8->name[j]));
                m8->alt[j] = a != g_app->shaderMap.end();
                if (m8->alt[j]) { m8->altInfo[j] = a->second; }
            }
            diag::detail("Knopftest: MR8 - Platz " + std::to_string(m8->platz[0]) + " " + m8->name[0] + ", Platz " + std::to_string(m8->platz[1]) + " " +
                         m8->name[1] + ", Bild " + m8->bild);
        }));
        struct M8Fall {
            const char* name;
            const char* erst;    // Zeilen vor der Stufe des ersten
            const char* zweit;   // ... des zweiten
            const char* art1;    // map / clampMap
            const char* art2;
        };
        static constexpr M8Fall kM8Faelle[6] = {{"noTC beim ersten", "noTC\n", "", "map", "map"},
                                                {"noTC beim zweiten", "", "noTC\n", "map", "map"},
                                                {"nomipmaps beim ersten", "nomipmaps\n", "", "map", "map"},
                                                {"nomipmaps beim zweiten", "", "nomipmaps\n", "map", "map"},
                                                {"clampMap beim ersten", "", "", "clampMap", "map"},
                                                {"clampMap beim zweiten", "", "", "map", "clampMap"}};
        for (int f = 0; f < 6; ++f) {
            const M8Fall fall = kM8Faelle[f];
            add(tu(std::string("Geschuetze Vergleich: MR8 - ") + fall.name, [m8, m8klein, fall] {
                if (m8->platz[1] < 0) { return; }
                const std::string text = m8->name[0] + "\n{\n" + fall.erst + "{\n" + fall.art1 + " " + m8->bild + "\n}\n}\n" + m8->name[1] + "\n{\n" +
                                         fall.zweit + "{\n" + fall.art2 + " " + m8->bild + "\n}\n}\n";
                ShaderMap probe;
                parseShaderScript(text, probe);
                for (int j = 0; j < 2; ++j) { g_app->shaderMap[m8klein(m8->name[j])] = probe[m8klein(m8->name[j])]; }
                loadMapTextures();
            }));
            add(pruefe(std::string("Geschuetze Vergleich: MR8 - ") + fall.name + ": beide Flaechen zeigen das Bild, wie der erste es lud", [m8, fall, f] {
                if (m8->platz[1] < 0) {
                    diag::detail("Knopftest: MR8 - keine zwei Shader mit Bild");
                    return false;
                }
                std::vector<std::uint8_t> px[2];
                int b[2] = {0, 0};
                int h[2] = {0, 0};
                int stufen[2] = {0, 0};
                bool klammern[2] = {false, false};
                for (int j = 0; j < 2; ++j) {
                    if (!gpu::leseKartenBild(&g_app->textures, m8->platz[j], 0, px[j], b[j], h[j], &stufen[j])) {
                        diag::detail("Knopftest: MR8 - Platz " + std::to_string(m8->platz[j]) + " nicht lesbar");
                        return false;
                    }
                    klammern[j] = g_app->textures.byShader[static_cast<std::size_t>(m8->platz[j])].clamp;
                }
                const std::vector<std::uint8_t>& roh = g_app->textures.byShader[static_cast<std::size_t>(m8->platz[0])].rgba;
                const bool dxt[2] = {px[0] != roh, px[1] != roh};
                char z[300];
                std::snprintf(z, sizeof(z), "Knopftest: MR8 %s - erster: DXT5 %d, Stufen %d, Klammern %d; zweiter: DXT5 %d, Stufen %d, Klammern %d; Bytes gleich %d",
                              fall.name, dxt[0] ? 1 : 0, stufen[0], klammern[0] ? 1 : 0, dxt[1] ? 1 : 0, stufen[1], klammern[1] ? 1 : 0, px[0] == px[1] ? 1 : 0);
                diag::detail(z);
                const bool gleich = px[0] == px[1] && b[0] == b[1] && h[0] == h[1] && stufen[0] == stufen[1] && klammern[0] == klammern[1];
                const int volleStufen = 1 + static_cast<int>(std::floor(std::log2(static_cast<double>(std::max(b[0], h[0])))));
                switch (f) {
                    case 0: return gleich && !dxt[0];                                            // noTC zuerst: unkomprimiert
                    case 1: return gleich && dxt[0];                                             // allowTC zuerst: DXT5
                    case 2: return gleich && stufen[0] == 1;                                     // ohne Mipmaps zuerst
                    case 3: return gleich && stufen[0] == volleStufen;                           // mit Mipmaps zuerst
                    case 4: return gleich && klammern[0];                                        // GL_CLAMP zuerst
                    default: return gleich && !klammern[0];                                      // GL_REPEAT zuerst
                }
            }));
        }
        add(tu("Geschuetze Vergleich: MR8 - die Shader wie vorher", [m8, m8klein] {
            if (m8->platz[1] < 0) { return; }
            for (int j = 0; j < 2; ++j) {
                if (m8->alt[j]) {
                    g_app->shaderMap[m8klein(m8->name[j])] = m8->altInfo[j];
                } else {
                    g_app->shaderMap.erase(m8klein(m8->name[j]));
                }
            }
            loadMapTextures();
        }));
        // MR9 Punkt 1: das Spiel registriert VOR der Welt, was der Server beim Spawnen laedt (G2API_InitGhoul2Model ->
        // RE_RegisterModel; FinishSpawningItem im RunFrame 1300 von SV_SpawnServer) und die Effekte (CG_RegisterEffects vor
        // cgi_R_LoadWorldMap). Probe: der erste Shader des ersten Gegenstands der Karte und der erste Weltshader mit Bild (MR8)
        // nennen dasselbe Bild, einer davon mit noTC. Wie im Spiel entscheidet der Gegenstand, nicht die Welt.
        struct M9Probe {
            std::string shader;   // Shader des Gegenstands (klein, ohne Endung)
            std::string bild;     // Bild des Weltshaders
            bool alt = false;
            ShaderInfo altInfo;
        };
        auto m9 = std::make_shared<M9Probe>();
        add(tu("Geschuetze Vergleich: MR9 - ein Gegenstand der Karte und ein Weltshader mit demselben Bild", [m8, m9, m8klein] {
            if (m8->platz[0] < 0) { return; }
            std::string dat;
            if (!readFromArchives("ext_data/items.dat", dat)) { return; }
            const Gegenstandsliste liste = leseGegenstaende(dat, regelwerkMovieDuels(), regelwerkJediOutcast());
            for (const MapEntity& e : g_app->map.entities) {
                const Gegenstand* g = liste.spawnbar(e.classname);
                if (g == nullptr || !e.targetname.empty() || !gespawntImSpiel(e)) { continue; }
                std::string d;
                Md3Model mm;
                std::string err;
                if (!readFromArchives(gegenstandWeltModell(e, *g), d) || !readMd3(d, mm, &err) || mm.surfaces.empty()) { continue; }
                std::string sh = m8klein(mm.surfaces[0].shader);
                const std::size_t punkt = sh.find_last_of('.');
                if (punkt != std::string::npos && sh.find('/', punkt) == std::string::npos) { sh.resize(punkt); }
                m9->shader = sh;
                break;
            }
            if (m9->shader.empty()) { return; }
            const auto w = g_app->shaderMap.find(m8klein(m8->name[0]));
            m9->bild = w != g_app->shaderMap.end() && !w->second.image.empty() ? w->second.image : m8->name[0];
            const auto a = g_app->shaderMap.find(m9->shader);
            m9->alt = a != g_app->shaderMap.end();
            if (m9->alt) { m9->altInfo = a->second; }
            diag::detail("Knopftest: MR9 - Gegenstand " + m9->shader + ", Welt Platz " + std::to_string(m8->platz[0]) + " " + m8->name[0] +
                         ", Bild " + m9->bild);
        }));
        struct M9Fall {
            const char* name;
            const char* gegenstand;   // Zeilen vor der Stufe
            const char* welt;
        };
        static constexpr M9Fall kM9Faelle[2] = {{"noTC am Gegenstand", "noTC\n", ""}, {"noTC an der Welt", "", "noTC\n"}};
        for (int f = 0; f < 2; ++f) {
            const M9Fall fall = kM9Faelle[f];
            add(tu(std::string("Geschuetze Vergleich: MR9 - ") + fall.name, [m8, m9, m8klein, fall] {
                if (m9->shader.empty()) { return; }
                const std::string text = m9->shader + "\n{\n" + fall.gegenstand + "{\nmap " + m9->bild + "\n}\n}\n" + m8->name[0] + "\n{\n" +
                                         fall.welt + "{\nmap " + m9->bild + "\n}\n}\n";
                ShaderMap probe;
                parseShaderScript(text, probe);
                g_app->shaderMap[m9->shader] = probe[m9->shader];
                g_app->shaderMap[m8klein(m8->name[0])] = probe[m8klein(m8->name[0])];
                loadMapTextures();
            }));
            add(pruefe(std::string("Geschuetze Vergleich: MR9 - ") + fall.name + ": der Gegenstand laedt das Bild vor der Welt", [m8, m9, fall, f] {
                if (m9->shader.empty()) {
                    diag::detail("Knopftest: MR9 - kein Gegenstand mit .md3 oder kein Weltshader mit Bild");
                    return false;
                }
                std::vector<std::uint8_t> px;
                int b = 0;
                int h = 0;
                int stufen = 0;
                if (!gpu::leseKartenBild(&g_app->textures, m8->platz[0], 0, px, b, h, &stufen)) { return false; }
                const bool dxt = px != g_app->textures.byShader[static_cast<std::size_t>(m8->platz[0])].rgba;
                const BildWahl* w = g_app->bildRegister.wahl(m9->bild);
                diag::detail(std::string("Knopftest: MR9 ") + fall.name + " - Welt DXT5 " + (dxt ? "1" : "0") + ", zuerst geladen von " +
                             (w != nullptr ? w->shader : std::string("-")));
                return f == 0 ? !dxt : dxt;
            }));
        }
        add(tu("Geschuetze Vergleich: MR9 - die Shader wie vorher", [m8, m9, m8klein] {
            if (m9->shader.empty()) { return; }
            if (m9->alt) {
                g_app->shaderMap[m9->shader] = m9->altInfo;
            } else {
                g_app->shaderMap.erase(m9->shader);
            }
            if (m8->alt[0]) {
                g_app->shaderMap[m8klein(m8->name[0])] = m8->altInfo[0];
            } else {
                g_app->shaderMap.erase(m8klein(m8->name[0]));
            }
            loadMapTextures();
        }));
        // MR9 Punkt 2: fehlt das Bild einer Stufe, bricht ParseStage ab (tr_shader.cpp:1263-1268), ParseShader scheitert, der
        // Shader wird defaultShader und die Flaeche zeigt tr.defaultShader (tr_bsp.cpp:352) - 16x16 Grau mit weissem Rand.
        // Die Stufe danach registriert nichts. MR9 Punkt 3: r_picmip nur fuer Bilder mit allowPicmip (Upload32, nopicmip).
        struct M9b {
            int picmip = 0;
            int breite[2] = {0, 0};   // Dateibreite der Bilder von Platz 0 und 1
        };
        auto m9b = std::make_shared<M9b>();
        add(tu("Geschuetze Vergleich: MR9 P2 - Stufenbild fehlt, danach eine Stufe mit vorhandenem Bild", [m8, m9b, m8klein] {
            if (m8->platz[1] < 0) { return; }
            const std::string text = m8->name[0] + "\n{\n{\nmap " + m8->bild + "\n}\n{\nmap textures/m9/gibt_es_nicht\n}\n{\nmap textures/m9/danach\n}\n}\n";
            ShaderMap probe;
            parseShaderScript(text, probe);
            g_app->shaderMap[m8klein(m8->name[0])] = probe[m8klein(m8->name[0])];
            loadMapTextures();
        }));
        add(pruefe("Geschuetze Vergleich: MR9 P2 - die Flaeche zeigt tr.defaultShader, die Stufe danach ist nicht registriert", [m8] {
            if (m8->platz[1] < 0) { return false; }
            const TextureSet::Tex& t = g_app->textures.byShader[static_cast<std::size_t>(m8->platz[0])];
            const TextureSet::Tex std = standardShaderTextur();
            const bool kasten = t.width == 16 && t.height == 16 && t.rgba == std.rgba;
            const bool danach = g_app->bildRegister.wahl("textures/m9/danach") != nullptr;
            diag::detail(std::string("Knopftest: MR9 P2 - ") + std::to_string(t.width) + "x" + std::to_string(t.height) + ", Kasten " +
                         (kasten ? "1" : "0") + ", Stufe danach registriert " + (danach ? "1" : "0") + ", erstes Bild registriert " +
                         (g_app->bildRegister.wahl(m8->bild) != nullptr ? "1" : "0"));
            return kasten && !danach && g_app->bildRegister.wahl(m8->bild) != nullptr;
        }));
        add(tu("Geschuetze Vergleich: MR9 P3 - nopicmip am ersten Weltshader, r_picmip 1", [m8, m9b, m8klein] {
            if (m8->platz[1] < 0) { return; }
            // das eigene Bild des ersten Weltshaders aus seinem urspruenglichen Skript (m8->bild ist das des zweiten, und P2 hat dem
            // ersten noch sein Probeskript gegeben)
            const std::string b0 = m8->alt[0] && !m8->altInfo[0].image.empty() ? m8->altInfo[0].image : m8->name[0];
            const std::string text = m8->name[0] + "\n{\nnopicmip\n{\nmap " + b0 + "\n}\n}\n";
            ShaderMap probe;
            parseShaderScript(text, probe);
            g_app->shaderMap[m8klein(m8->name[0])] = probe[m8klein(m8->name[0])];
            for (int j = 0; j < 2; ++j) {
                const auto it = g_app->shaderMap.find(m8klein(m8->name[j]));
                const std::string b = j == 0 ? b0 : (it != g_app->shaderMap.end() && !it->second.image.empty() ? it->second.image : m8->name[1]);
                for (const std::string& c : textureCandidates(b)) {
                    std::string data;
                    if (!readFromArchives(c, data)) { continue; }
                    const image::Image im = image::decode(reinterpret_cast<const unsigned char*>(data.data()), data.size());
                    if (im.ok) { m9b->breite[j] = std::max(im.width, im.height); }
                    break;
                }
            }
            m9b->picmip = g_app->settings.bildPicmip;
            g_app->settings.bildPicmip = 1;
            loadMapTextures();
        }));
        add(pruefe("Geschuetze Vergleich: MR9 P3 - das nopicmip-Bild bleibt gross, das andere halbiert r_picmip", [m8, m9b] {
            if (m8->platz[1] < 0 || m9b->breite[0] <= 0 || m9b->breite[1] <= 0) { return false; }
            const TextureSet::Tex& a = g_app->textures.byShader[static_cast<std::size_t>(m8->platz[0])];
            const TextureSet::Tex& b = g_app->textures.byShader[static_cast<std::size_t>(m8->platz[1])];
            const int ka = std::max(a.width, a.height);
            const int kb = std::max(b.width, b.height);
            diag::detail("Knopftest: MR9 P3 - nopicmip " + std::to_string(ka) + " (Datei " + std::to_string(m9b->breite[0]) + "), mit picmip " +
                         std::to_string(kb) + " (Datei " + std::to_string(m9b->breite[1]) + ")");
            return ka == std::min(m9b->breite[0], kBildGrenze) && kb == std::clamp(m9b->breite[1] >> 1, 1, kBildGrenze);
        }));
        add(tu("Geschuetze Vergleich: MR9 P2/P3 - Shader und r_picmip wie vorher", [m8, m9b, m8klein] {
            if (m8->platz[1] < 0) { return; }
            g_app->settings.bildPicmip = m9b->picmip;
            if (m8->alt[0]) {
                g_app->shaderMap[m8klein(m8->name[0])] = m8->altInfo[0];
            } else {
                g_app->shaderMap.erase(m8klein(m8->name[0]));
            }
            loadMapTextures();
        }));
        // P58 (PRUEF8): noTC im Hautshader einer Figur gilt fuer ihre Bilder wie bei der Karte (ParseStage allowTC = !shader.noTC,
        // tr_shader.cpp:1263/1288/1329) - die Grundflaeche (bySurface), die Stufen (figurStufenListe) und die Gluehstufen.
        add(pruefe("Geschuetze Vergleich: P58 - noTC im Hautshader einer Figur", [m8] {
            if (m8->platz[1] < 0) { return false; }
            const std::string text = "m9/figurnotc\n{\nnoTC\n{\nmap " + m8->bild + "\n}\n}\n" +
                                     "m9/figurnotc2\n{\nnoTC\n{\nmap " + m8->bild + "\n}\n{\nmap " + m8->bild + "\nblendFunc GL_ONE GL_ONE\nglow\n}\n}\n" +
                                     "m9/figurtc\n{\n{\nmap " + m8->bild + "\n}\n}\n";
            ShaderMap probe;
            parseShaderScript(text, probe);
            const char* namen[3] = {"m9/figurnotc", "m9/figurnotc2", "m9/figurtc"};
            for (const char* n : namen) { g_app->shaderMap[n] = probe[n]; }
            GlmModel gm;
            gm.surfaces.resize(3);
            for (int k = 0; k < 3; ++k) { gm.surfaces[static_cast<std::size_t>(k)].texture = namen[k]; }
            const ModelTextures mt = texturesFor(gm);
            for (const char* n : namen) { g_app->shaderMap.erase(n); }
            int stufen = 0;
            int stufenOhneTc = 0;
            if (mt.stufen.size() > 1U) {
                for (const ModelTextures::FigurStufe& fs : mt.stufen[1]) {
                    for (const TextureSet::Tex& z : fs.bilder) {
                        ++stufen;
                        stufenOhneTc += z.keinTc ? 1 : 0;
                    }
                }
            }
            int glueh = 0;
            int gluehOhneTc = 0;
            if (mt.gluehStufen.size() > 1U) {
                for (const TextureSet::Tex& z : mt.gluehStufen[1]) {
                    ++glueh;
                    gluehOhneTc += z.keinTc ? 1 : 0;
                }
            }
            const bool grund = mt.bySurface.size() == 3U && !mt.bySurface[0].rgba.empty() && mt.bySurface[0].keinTc && !mt.bySurface[2].keinTc;
            diag::detail("Knopftest: P58 - Grundflaeche noTC " + std::string(mt.bySurface.size() == 3U && mt.bySurface[0].keinTc ? "1" : "0") +
                         ", ohne noTC " + std::string(mt.bySurface.size() == 3U && mt.bySurface[2].keinTc ? "1" : "0") + "; Stufen " +
                         std::to_string(stufenOhneTc) + "/" + std::to_string(stufen) + " ohne TC; Gluehstufen " + std::to_string(gluehOhneTc) +
                         "/" + std::to_string(glueh));
            return grund && stufen > 0 && stufenOhneTc == stufen && gluehOhneTc == glueh;   // Gluehstufen nur, wenn die Figur welche hat
        }));
        // P59 (PRUEF8, Figuren): dieselbe Datei zweimal im Hautshader, die zweite Stufe nach noTC - R_FindImageFile_NoLoad gibt beim
        // zweiten Mal das Bild des ersten zurueck (allowTC des ersten, rd-vanilla tr_image.cpp:936-971): beide Stufen komprimiert.
        add(pruefe("Geschuetze Vergleich: P59 - dasselbe Bild zweimal im Hautshader einer Figur", [m8] {
            if (m8->platz[1] < 0) { return false; }
            const std::string text = "m9/figurzweimal\n{\n{\nmap " + m8->bild + "\n}\nnoTC\n{\nmap " + m8->bild + "\nblendFunc GL_ONE GL_ONE\n}\n}\n";
            ShaderMap probe;
            parseShaderScript(text, probe);
            g_app->shaderMap["m9/figurzweimal"] = probe["m9/figurzweimal"];
            GlmModel gm;
            gm.surfaces.resize(1);
            gm.surfaces[0].texture = "m9/figurzweimal";
            const ModelTextures mt = texturesFor(gm);
            g_app->shaderMap.erase("m9/figurzweimal");
            int stufen = 0;
            int ohneTc = 0;
            if (!mt.stufen.empty()) {
                for (const ModelTextures::FigurStufe& fs : mt.stufen[0]) {
                    for (const TextureSet::Tex& z : fs.bilder) {
                        ++stufen;
                        ohneTc += z.keinTc ? 1 : 0;
                    }
                }
            }
            diag::detail("Knopftest: P59 Figur - Stufen " + std::to_string(stufen) + ", davon ohne TC " + std::to_string(ohneTc) + " (soll 2, 0)");
            return stufen == 2 && ohneTc == 0;
        }));
        // P59 (PRUEF8): dieselbe Datei zweimal im selben Shader, die zweite Stufe nach noTC - im Spiel gibt R_FindImageFile_NoLoad
        // beim zweiten Mal das Bild des ersten zurueck (allowTC des ersten, tr_image.cpp:936-971): beide Stufen DXT5.
        add(tu("Geschuetze Vergleich: P59 - dasselbe Bild zweimal im Shader, das zweite nach noTC", [m8, m8klein] {
            if (m8->platz[1] < 0) { return; }
            const std::string text = m8->name[0] + "\n{\n{\nmap " + m8->bild + "\n}\nnoTC\n{\nmap " + m8->bild + "\nblendFunc GL_ONE GL_ONE\n}\n}\n";
            ShaderMap probe;
            parseShaderScript(text, probe);
            g_app->shaderMap[m8klein(m8->name[0])] = probe[m8klein(m8->name[0])];
            loadMapTextures();
        }));
        add(pruefe("Geschuetze Vergleich: P59 - beide Stufen wie das erste Laden (DXT5)", [m8, m8klein] {
            if (m8->platz[1] < 0) { return false; }
            const TextureSet::Tex& t = g_app->textures.byShader[static_cast<std::size_t>(m8->platz[0])];
            const bool zweite = !t.extraStages.empty();
            const bool zweiteKeinTc = zweite && g_app->textures.byShader[static_cast<std::size_t>(t.extraStages[0])].keinTc;
            diag::detail(std::string("Knopftest: P59 - Grundstufe keinTc ") + (t.keinTc ? "1" : "0") + ", Zusatzstufen " +
                         std::to_string(t.extraStages.size()) + ", zweite keinTc " + (zweiteKeinTc ? "1" : "0"));
            const bool ok = zweite && !t.keinTc && !zweiteKeinTc;
            if (m8->alt[0]) {
                g_app->shaderMap[m8klein(m8->name[0])] = m8->altInfo[0];
            } else {
                g_app->shaderMap.erase(m8klein(m8->name[0]));
            }
            loadMapTextures();
            return ok;
        }));
        // MR7 P39: das Spielbild des Karten-Editors (gpu-Szene 1, "Built map as in the game") mit derselben Karte - seine Bilder
        // liegen geparkt, solange die Vorschau zeichnet. Nach dem Wechsel muessen auch sie nach der neuen Regel neu hoch.
        constexpr std::uint64_t kP39Nummer = 0x5039000000000001ULL;
        auto bspP39 = std::make_shared<std::shared_ptr<const std::string>>();
        const auto spielbildP39 = [kam, archiv, bspP39] {
            if (!*bspP39) {   // die .bsp der Vorschau aus dem Archiv (BHED_GV_PK3)
                Pk3 arc;
                std::string bytes;
                const Pk3Entry* e = readPk3Directory(archiv, arc) ? arc.find(g_app->map.path) : nullptr;
                if (e == nullptr || !readPk3File(arc, *e, bytes)) { return false; }
                *bspP39 = std::make_shared<const std::string>(std::move(bytes));
            }
            karte::Kamera k;
            k.ursprung = karte::Vektor3f(kam[0], kam[1], kam[2]);
            k.winkel = karte::Vektor3f(-kam[3], kam[4], 0.0F);
            return karteneditor::spielbildZeichnen(*bspP39, kP39Nummer, g_app->map.path, k, 640, 480) != nullptr;
        };
        add(tu("Geschuetze Vergleich: P39 - Spielbild des Karten-Editors (Szene 1) mit derselben Karte", [=] {
            const bool bild = spielbildP39();
            const gpu::BilderDxt b = gpu::bilderDxt(true);
            bezeichne(bild ? "Szene 1 (Spielbild) vor dem Wechsel" : "Szene 1 (Spielbild) vor dem Wechsel, KEIN BILD", b);
        }));
        // MR8 Punkt 2: Vorschau (Szene 0) und Spielbild (Szene 1) zeigen dieselbe Karte nach denselben Regeln - gleiche Bytes liegen
        // nur einmal auf der Grafikkarte.
        add(pruefe("Geschuetze Vergleich: MR8 P2 - Vorschau und Spielbild teilen die Kartenbilder mit gleichen Bytes", [] {
            const auto a = gpu::kartenBildTexturen(0);
            const auto b = gpu::kartenBildTexturen(1);
            std::set<const void*> res;
            long long summe = 0;
            long long eindeutig = 0;
            for (const auto* liste : {&a, &b}) {
                for (const auto& [r, n] : *liste) {
                    summe += n;
                    if (res.insert(r).second) { eindeutig += n; }
                }
            }
            std::set<const void*> nullRes;
            for (const auto& [r, n] : a) { (void)n; nullRes.insert(r); }
            int gemeinsam = 0;
            for (const auto& [r, n] : b) { (void)n; gemeinsam += nullRes.count(r) != 0U ? 1 : 0; }
            char z[300];
            std::snprintf(z, sizeof(z), "Knopftest: MR8 P2 - Szene 0: %zu Bilder, Szene 1: %zu Bilder, davon gemeinsam %d; Bytes zusammen %.1f MB, belegt %.1f MB; DXGI %.1f MB",
                          a.size(), b.size(), gemeinsam, static_cast<double>(summe) / 1048576.0, static_cast<double>(eindeutig) / 1048576.0,
                          static_cast<double>(gpu::speicherStand().grafikBytes) / 1048576.0);
            diag::detail(z);
            const std::size_t weniger = std::min(a.size(), b.size());   // die Vorschau laedt nur, was sie gezeichnet hat
            const gpu::BilderTeilen tb = gpu::bilderTeilen();
            std::snprintf(z, sizeof(z), "Knopftest: MR8 P2 - geteilt %d Bilder (%.1f MB), angelegt %d (%.1f MB), jetzt auf der Grafikkarte %d (%.1f MB)", tb.geteilt,
                          static_cast<double>(tb.geteiltBytes) / 1048576.0, tb.angelegt, static_cast<double>(tb.angelegtBytes) / 1048576.0, tb.lebend,
                          static_cast<double>(tb.lebendBytes) / 1048576.0);
            diag::detail(z);
            return weniger > 0U && static_cast<std::size_t>(gemeinsam) * 10U >= weniger * 9U;
        }));
        auto vorher = std::make_shared<bool>(false);
        add(tu("Geschuetze Vergleich: Regelwerk wechseln (Movie Duels <-> OpenJK)", [=] {
            *vorher = regelwerkMovieDuels();
            g_app->settings.regelwerk = *vorher ? kRegelwerkOpenJK : kRegelwerkMovieDuels;
            (void)regelwerkPruefen();
            g_app->mapDirty = true;
        }));
        add(pause(1.0));
        add(pruefe("Geschuetze Vergleich: nach dem Wechsel alle Bilder neu, nach der neuen Regel", [=] {
            const gpu::BilderDxt b = gpu::bilderDxt(true);
            const bool md = regelwerkMovieDuels();
            bezeichne(md ? "nach dem Wechsel, Movie Duels" : "nach dem Wechsel, JKA/JK2", b);
            if (md == *vorher) { return false; }
            if (md) { return b.lightmapStufe > 0 && b.komprimiert == 0; }
            return b.lightmapStufe > 0 && b.lightmapStufeKomprimiert == b.lightmapStufe;
        }));
        add(pruefe("Geschuetze Vergleich: P39 - nach dem Wechsel laedt auch das Spielbild des Karten-Editors (Szene 1) neu, nach der neuen Regel", [=] {
            (void)gpu::bilderDxt(true);
            const bool bild = spielbildP39();
            const gpu::BilderDxt b = gpu::bilderDxt(true);
            const bool md = regelwerkMovieDuels();
            bezeichne(md ? "Szene 1 nach dem Wechsel, Movie Duels" : "Szene 1 nach dem Wechsel, JKA/JK2", b);
            if (!bild || b.lightmapStufe == 0) { return false; }
            return md ? b.komprimiert == 0 : b.lightmapStufeKomprimiert == b.lightmapStufe;
        }));
    }
    add(tu("Geschuetze Vergleich: Regelwerk wie vor dem Vergleich", [=] {
        g_app->settings.regelwerk = *regelAnfang;
        (void)regelwerkPruefen();
        g_app->mapDirty = true;
    }));
    add(pruefe("Geschuetze Vergleich: P42 - das Regelwerk steht am Ende wieder wie vor dem Vergleich", [=] {
        diag::detail("Knopftest: Geschuetze Vergleich - Regelwerk am Ende " + std::to_string(g_app->settings.regelwerk) + ", vorher " +
                     std::to_string(*regelAnfang));
        return g_app->settings.regelwerk == *regelAnfang;
    }));
    return s;
}

// Das Bild fuer das Handbuch (BHED_SPRACHE, BHED_FOTOS): ein misc_turret im Feuer, das ganze Fenster ohne Meldungen.
std::vector<Schritt> geschuetzBildSchritte() {
    namespace gp = geschuetze_probe;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<pruefung3::Zustand>();
    add(tu("Geschuetze Bild: md_am_sith laden", [=] {
        for (const std::string& o : g_app->settings.gamePaths) {
            std::error_code ec;
            if (z->mdArchiv.empty() && std::filesystem::is_regular_file(::bhed::utf8Pfad(o + "\\assets1.pk3"), ec)) {
                z->mdArchiv = o + "\\assets1.pk3";
            }
        }
        g_app->leftMode = 1;
        g_app->missionPickOpen = false;
        mission1::ungespeicherteEntitiesVergessen();
        missionenAusArchiv(z->mdArchiv, "md_am_sith");
    }));
    add(warteBis("Geschuetze Bild: Grundkarte geladen", [] {
        return g_app->map.path == "maps/md_am_sith.bsp" && g_app->camTrackValid && jk2probe::figurenGeladen();
    }, 4000));
    add(tu("Geschuetze Bild: Mission", [=] {
        prepareScene();
        for (std::size_t ai = 0; ai < g_app->scene.actors.size() && z->typ.empty(); ++ai) {
            const Actor& a = g_app->scene.actors[ai];
            for (const ActorStep& st : a.steps) {
                if (z->typ.empty() && st.kind == ActorStep::Kind::WalkTo && !a.npcType.empty() && ai < g_app->actorAssets.size() &&
                    g_app->actorAssets[ai] && !g_app->actorAssets[ai]->model.empty()) {
                    for (int k = 0; k < 3; ++k) { z->ort[k] = st.from[k]; }
                    z->typ = a.npcType;
                }
            }
        }
        pruefung3::szeneBauen(*z, "gbild", {{"g3_ziel", 0.0F, 0.0F}}, gp::ziel("g3_ziel", 1000));
        const float o[3] = {z->ort[0] + 96.0F, z->ort[1] - 64.0F, z->ort[2] + 60.0F};
        (void)gp::neue("misc_turret", o, {{"team", "player"}, {"angle", "180"}});
        const float o2[3] = {z->ort[0] + 96.0F, z->ort[1] + 64.0F, z->ort[2] + 60.0F};
        (void)gp::neue("misc_turretG2", o2, {{"alliedTeam", "1"}, {"angle", "180"}});
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    add(warteBis("Geschuetze Bild: Szene", [=] { return !z->bereit || (g_app->camTrackValid && pruefung3::figur("g3_ziel") >= 0 && jk2probe::figurenGeladen()); }, 3000));
    add(pause(0.5));
    add(tu("Geschuetze Bild: Kamera", [=] {
        prepareScene();
        entityAuswahlSetzen(-1);
        g_app->followCam = false;
        g_app->cam.pos[0] = z->ort[0] + 20.0F;
        g_app->cam.pos[1] = z->ort[1] - 190.0F;
        g_app->cam.pos[2] = z->ort[2] + 60.0F;
        g_app->cam.angles[1] = 75.0F;
        g_app->cam.angles[0] = 2.0F;
        g_app->playMs = 3040.0;
        g_app->playing = false;
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(hb::fotoTeil("geschuetz_feuer", [] { return ImRect(0.0F, 0.0F, ImGui::GetIO().DisplaySize.x, std::min(ImGui::GetIO().DisplaySize.y, 1150.0F)); }));
    return s;
}
