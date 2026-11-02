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
//    (G_RadiusDamage) trifft die Figur, misc_trip_mine explodiert.
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
    add(tu("Geschuetze Vergleich: Karte " + karte, [=] {
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
            g_app->cam.fovY = 64.3727F;   // CalcFov: 2 * atan(tan(40 Grad) * 3 / 4)
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
            std::snprintf(z, sizeof(z), "Geschuetze Vergleich: Bildzeit %.1f ms", static_cast<double>(1000.0F / std::max(ImGui::GetIO().Framerate, 0.1F)));
            diag::detail(z);
            std::snprintf(z, sizeof(z), "Geschuetze Vergleich: Ansicht von %.0f %.0f bis %.0f %.0f", static_cast<double>(g_app->kartenAnsicht[0]), static_cast<double>(g_app->kartenAnsicht[1]),
                          static_cast<double>(g_app->kartenAnsicht[2]), static_cast<double>(g_app->kartenAnsicht[3]));
            diag::detail(z);
        }));
        add(foto(karte + "_" + std::to_string(ms)));
    }
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
