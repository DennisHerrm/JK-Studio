// app_view3d_spiel.inc - aus gui/app_view3d.cpp geschnitten (Stufe 8d Punkt 5, unveraendert): das Spiel in der Vorschau - gi.trace, Laserfallen, Geschuetze, Gegenstaende, Klingen
//
// Eingebunden an genau dieser Stelle von gui/app_view3d.cpp (im selben Namensraum, mit allen Helfern davor);
// keine eigene Uebersetzungseinheit - der vorverarbeitete Text ist derselbe wie vor dem Schnitt.
// Abschnitte: rc604: gi.trace fuer laserTrapThink zur Zeit ms; rc604 fix604b (B5): der Strahl der misc_trip_mine; rc604: Wachgeschuetz und Ionenkanone bewegen sich wie im Spiel; rc602 kartenpruefung: Gegenstaende der Karte; rc604 fix604b (B5): misc_trip_mine; rc604 fix604b (B3): Geschuetze und Waffenstellungen; Griffe und Klingenbilder VORLADEN; Die Kino-Skelette der Karte

// rc604 fix604b (B5): Lage und Strahl einer misc_trip_mine (tripMineLage, unten).
namespace {
struct TripMineLage {
    float ort[3]{};
    float winkel[3]{};
    float normale[3]{};
    float strahlEnde[3]{};
    float origin2[3]{};             // rc604: s.origin2 (Start der Spur von laserTrapThink)
    bool strahl = false;            // an ab dem Spawn (misc_trip_mine_activate)
    bool sichtbarAbSpawn = true;    // rc604: targetname + START_OFF -> EF_NODRAW
    bool hatName = false;           // rc604: targetname -> e_UseFunc
};
bool tripMineLage(const MapEntity& e, TripMineLage& l);
void winkelZuAchsen(const float w[3], float achse[3][3]);
const App::ActorAssets* gegenstandGlmFuer(const std::string& pfad);
}  // namespace

// --- rc604: gi.trace fuer laserTrapThink zur Zeit ms ------------------------
//
// gi.trace(&tr, s.origin2, -4, 4, end, s.number, MASK_SHOT, G2_RETURNONHIT, 0)
// (wp_trip_mine.cpp:150 OpenJK) -> SV_Trace (code/server/sv_world.cpp:840-
// 983): erst die Welt (CM_BoxTrace; fraction 0 -> fertig), dann
// SV_ClipMoveToEntities (:600-834): jede Entity mit passenden contents -
// Brush-Modelle mit CM_TransformedBoxTrace an currentOrigin/currentAngles (hier:
// wo Skript und Mover-Simulation sie zur Zeit ms haben), Figuren als Kasten
// (CM_TempBoxModel, Winkel 0). Trifft der Kasten einer Figur, prueft
// G2API_CollisionDetect das Netz (G2_TraceModels -> G2_TraceSurfaces ->
// G2_RadiusTracePolys mit radius = (maxs[0] - mins[0]) / 2); ohne Treffer
// gilt wieder die Spur von vorher (clip->trace = oldTrace).
// rc605 (Rest): dazu andere Fallen (touchLaserTrap: Kasten -4..4, contents
// CONTENTS_SHOTCLIP, wp_trip_mine.cpp:46-50 OpenJK, bis G_FreeEntity nach
// WP_Explode), misc_model_breakable (Kasten aus mins/maxs, misc_model_breakable_init
// g_breakable.cpp:808-831 MD: spawnflags 1 -> CONTENTS_SOLID|..., sonst mit health
// CONTENTS_SHOTCLIP; nach dem Bruch contents 0 ausser spawnflags 4, 590-600) und
// die weiteren Ghoul2-Modelle der Figur am Bolzen (Waffe, Schwertgriff -
// G2_TraceModels geht ueber das ganze ghoul2-Feld). func_breakable ist ein
// Brush-Modell und steht schon in den Brushes.
namespace {
std::string kleinFalle(std::string s) {   // rc605 (Rest)
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}
bool g_fallenOhneAnbau = false;   // rc605 (Rest) Selbsttest: nur der Koerper
struct FallenMine {
    std::size_t entity = 0;
    float ort[3]{};
    double wegMs = 1.0e300;   // G_FreeEntity (aus laserfalleLauf, siehe fallenEffekteSetzen)
    double zwangMs = 1.0e300; // rc605 (Ende): von einer anderen Falle gesprengt (fallenKette)
    double schadenAb = 1.0e300;   // rc605 (Ende): takedamage ab (touchLaserTrap / misc_trip_mine_activate)
};
struct FallenModell {
    std::size_t entity = 0;
    std::string pfad;          // model (klein), fuer MoverSim::modellAt
    float mins[3]{};           // Weltkasten
    float maxs[3]{};
    bool festNachBruch = false;   // spawnflags 4
};
struct FallenBrush {
    std::size_t entity = 0;
    int modell = 0;
    float ort0[3]{};
    float radius = 0.0F;
    const Actor* skript = nullptr;
};
struct FallenWelt {
    std::shared_ptr<const KollisionsKarte> k;
    std::vector<FallenBrush> brushes;
    std::vector<std::size_t> figuren;   // Index in scene.actors
    std::vector<FallenMine> minen;      // rc605 (Rest)
    std::vector<FallenModell> modelle;  // rc605 (Rest)
};
FallenWelt g_fallenWelt;
// rc605 (Ende): CanDamage (code/game/g_combat.cpp:6780-6847 OpenJK): Punktspuren
// MASK_SOLID von der Explosion zur Mitte des Kastens und zu +-15 in x/y - gegen die
// Welt und die Brush-Entities zur Zeit ms (Koerper sind nicht MASK_SOLID).
bool fallenKannTreffenBei(double ms, const float von[3], const float mitte[3]) {
    const FallenWelt& fw = g_fallenWelt;
    if (!fw.k) { return true; }
    const float null3[3] = {0.0F, 0.0F, 0.0F};
    const std::uint32_t maske = kNavSolid | kNavTerrain;   // MASK_SOLID
    const auto frei = [&](const float ziel[3]) {
        if (kollisionsSpur(*fw.k, von, null3, null3, ziel, maske).fraction < 1.0F) { return false; }
        for (const FallenBrush& b : fw.brushes) {
            float ort[3] = {b.ort0[0], b.ort0[1], b.ort0[2]};
            float winkel[3] = {0.0F, 0.0F, 0.0F};
            if (b.skript != nullptr) {
                const ActorState st = b.skript->at(ms);
                if (!st.visible) { continue; }
                for (int q = 0; q < 3; ++q) {
                    ort[q] = st.pos[q];
                    winkel[q] = st.angles[q];
                }
            } else {
                const MoverSim::Stand m = g_app->moverSim.at(b.modell, ms);
                if (m.bekannt) {
                    if (!m.sichtbar) { continue; }
                    for (int q = 0; q < 3; ++q) {
                        ort[q] = m.origin[q];
                        winkel[q] = m.angles[q];
                    }
                }
            }
            if (kollisionsSpur(*fw.k, von, null3, null3, ziel, maske, b.modell, ort, winkel).fraction < 1.0F) { return false; }
        }
        return true;
    };
    if (frei(mitte)) { return true; }
    for (const float dx : {15.0F, -15.0F}) {
        for (const float dy : {15.0F, -15.0F}) {
            const float z[3] = {mitte[0] + dx, mitte[1] + dy, mitte[2]};
            if (frei(z)) { return true; }
        }
    }
    return false;
}

void fallenWeltBauen() {
    FallenWelt& w = g_fallenWelt;
    w = FallenWelt{};
    g_app->fallenNetzTreffer = 0;
    g_app->fallenKastenTreffer = 0;
    g_app->fallenFallenTreffer = 0;   // rc605 (Rest)
    g_app->fallenModellTreffer = 0;
    g_app->fallenAnbauTreffer = 0;
    w.k = geladeneKollision(g_app->map.path);
    std::map<int, const Actor*> skript;
    for (std::size_t ai = 0; ai < g_app->scene.actors.size(); ++ai) {
        const Actor& a = g_app->scene.actors[ai];
        if (!a.haveStart) { continue; }
        if (a.brushModel > 0) {
            skript[a.brushModel] = &a;
        } else {
            w.figuren.push_back(ai);
        }
    }
    const BspGeometry& geo = g_app->geo;
    for (std::size_t ei = 0; ei < g_app->map.entities.size(); ++ei) {
        const MapEntity& e = g_app->map.entities[ei];
        const std::string* mdl = e.find("model");
        if (mdl == nullptr || mdl->size() < 2 || (*mdl)[0] != '*' || e.classname.rfind("trigger_", 0) == 0 ||
            !gespawntImSpiel(e)) {
            continue;
        }
        const int nr = std::atoi(mdl->c_str() + 1);
        if (nr <= 0 || static_cast<std::size_t>(nr) >= geo.models.size()) { continue; }
        FallenBrush b;
        b.entity = ei;
        b.modell = nr;
        if (!e.origin.empty()) { (void)std::sscanf(e.origin.c_str(), "%f %f %f", &b.ort0[0], &b.ort0[1], &b.ort0[2]); }
        const auto sk = skript.find(nr);
        b.skript = sk == skript.end() ? nullptr : sk->second;
        const BspGeometry::SubModel& sm = geo.models[static_cast<std::size_t>(nr)];
        float r2 = 0.0F;
        for (int q = 0; q < 3; ++q) {
            const float m = std::max(std::fabs(sm.mins[q]), std::fabs(sm.maxs[q]));
            r2 += m * m;
        }
        b.radius = std::sqrt(r2) + 1.0F;
        w.brushes.push_back(b);
    }
    // rc605 (Rest): andere Fallen und misc_model_breakable.
    for (std::size_t ei = 0; ei < g_app->map.entities.size(); ++ei) {
        const MapEntity& e = g_app->map.entities[ei];
        if (!gespawntImSpiel(e)) { continue; }
        const std::string kl = kleinFalle(e.classname);
        if (kl == "misc_trip_mine") {
            TripMineLage tl;
            if (!kartenpruefung::imSpiel(e.classname, regelwerkMovieDuels()) || !tripMineLage(e, tl)) { continue; }
            FallenMine m;
            m.entity = ei;
            for (int q = 0; q < 3; ++q) { m.ort[q] = tl.ort[q]; }
            // rc605 (Ende): touchLaserTrap setzt takedamage (wp_trip_mine.cpp:51
            // OpenJK); SP_misc_trip_mine nimmt es bei targetname + START_OFF
            // wieder weg, bis misc_trip_mine_activate es setzt (g_misc.cpp
            // :1808-1816, 1870-1875).
            if (tl.sichtbarAbSpawn) {
                m.schadenAb = kNavKartenSpawnMs;
            } else {
                for (const auto& [ent, ms] : g_app->moverSim.entityUses()) {
                    if (ent == ei) { m.schadenAb = std::min(m.schadenAb, ms); }
                }
            }
            w.minen.push_back(m);
        } else if (kl == "misc_model_breakable") {
            // SP_misc_model_breakable (g_breakable.cpp:1215-1300 MD)
            const std::string* mdl = e.find("model");
            if (mdl == nullptr || mdl->empty()) { continue; }
            int sf = 0;
            int health = 0;
            if (const std::string* v = e.find("spawnflags")) { sf = std::atoi(v->c_str()); }
            if (const std::string* v = e.find("health")) { health = std::atoi(v->c_str()); }
            if ((sf & 1) == 0 && health == 0) { continue; }   // contents 0
            float o[3] = {0.0F, 0.0F, 0.0F};
            if (!e.origin.empty()) { (void)std::sscanf(e.origin.c_str(), "%f %f %f", &o[0], &o[1], &o[2]); }
            float mi[3] = {0.0F, 0.0F, 0.0F};
            float ma[3] = {0.0F, 0.0F, 0.0F};
            if (const std::string* v = e.find("mins")) { (void)std::sscanf(v->c_str(), "%f %f %f", &mi[0], &mi[1], &mi[2]); }
            if (const std::string* v = e.find("maxs")) { (void)std::sscanf(v->c_str(), "%f %f %f", &ma[0], &ma[1], &ma[2]); }
            if (mi[0] == 0.0F && mi[1] == 0.0F && mi[2] == 0.0F) { mi[0] = mi[1] = mi[2] = -16.0F; }
            if (ma[0] == 0.0F && ma[1] == 0.0F && ma[2] == 0.0F) { ma[0] = ma[1] = ma[2] = 16.0F; }
            if (kleinFalle(*mdl) == "models/map_objects/ships/tie_bomber.md3") {
                mi[0] = mi[1] = mi[2] = -80.0F;
                ma[0] = ma[1] = ma[2] = 80.0F;
            }
            float skala[3] = {0.0F, 0.0F, 0.0F};
            bool mitSkala = false;
            if (const std::string* v = e.find("modelscale_vec"); v != nullptr &&
                std::sscanf(v->c_str(), "%f %f %f", &skala[0], &skala[1], &skala[2]) == 3 &&
                (skala[0] != 0.0F || skala[1] != 0.0F || skala[2] != 0.0F)) {
                mitSkala = true;
            } else if (const std::string* s1 = e.find("modelscale"); s1 != nullptr && std::atof(s1->c_str()) != 0.0) {
                skala[0] = skala[1] = skala[2] = static_cast<float>(std::atof(s1->c_str()));
                mitSkala = true;
            }
            if (mitSkala) {
                for (int q = 0; q < 2; ++q) {
                    mi[q] *= skala[q];
                    ma[q] *= skala[q];
                }
                ma[2] *= skala[2];
                const float alt = mi[2];
                mi[2] *= skala[2];
                o[2] += alt - mi[2];   // "adjust origin accordingly"
            }
            FallenModell m;
            m.entity = ei;
            m.pfad = kleinFalle(*mdl);
            for (int q = 0; q < 3; ++q) {
                m.mins[q] = o[q] + mi[q];
                m.maxs[q] = o[q] + ma[q];
            }
            m.festNachBruch = (sf & 4) != 0;
            w.modelle.push_back(m);
        }
    }
}

// G2_TraceModels fuer das Koerpermodell der Figur ai zur Zeit ms (dieselbe
// Knochenrechnung wie die Zeichnung). -1: kein Netz zu pruefen, 0/1: Treffer.
int figurNetzTreffer(std::size_t ai, const ActorState& st, double ms, const float start[3], const float ende[3],
                     float radius) {
    if (ai >= g_app->actorAssets.size() || g_app->actorAssets.size() != g_app->scene.actors.size() ||
        !g_app->actorAssets[ai]) {
        return -1;
    }
    const Actor& a = g_app->scene.actors[ai];
    const App::ActorAssets& assets = *g_app->actorAssets[ai];
    if (assets.model.empty()) { return -1; }
    ActorDraw d;
    d.figur = static_cast<int>(ai);
    d.model = &assets.model;
    figurBilderSetzen(a, assets, st, ms, d);
    if (d.anim == nullptr) { return -1; }
    for (int q = 0; q < 3; ++q) { d.pos[q] = st.pos[q]; }
    d.yaw = st.angles[1];
    npcSkalaFuer(a.npcType, d.skala);
    anbautenFuer(a, st, ms, d);
    figurKopfGier(st, d);
    d.yawOffset = static_cast<float>(g_app->actorYaw);
    std::vector<BoneMatrix> welt;
    std::vector<BoneMatrix> hilf;
    if (!figurKnochen(d, welt, hilf) || welt.empty()) { return -1; }
    std::vector<BoneMatrix> skin;
    gpu::baueKnochenmatrizen(welt, d.anim->bones, skin);
    float fw[16];
    figurWeltMatrix(d, fw);
    const GlmModel& m = assets.model;
    // G2_TraceSurfaces: rekursiv ab der Wurzel; offFlags = Ueberschreibung
    // (die .skin mit "*off": G2SURFACEFLAG_OFF) oder die Flaggen der Hierarchie;
    // G2SURFACEFLAG_NODESCENDANTS (0x100) schneidet die Kinder ab.
    constexpr std::uint32_t kOhneKinder = 0x100U;
    const auto aus = [&m](std::size_t si) {
        const GlmSurface& sf = m.surfaces[si];
        return sf.skinnedOff ? GlmSurface::kFlagOff : sf.flags;
    };
    std::vector<float> ecken;
    std::vector<std::uint32_t> idx;
    for (std::size_t si = 0; si < m.surfaces.size(); ++si) {
        const GlmSurface& sf = m.surfaces[si];
        if (aus(si) != 0U || sf.verts.empty()) { continue; }
        bool abgeschnitten = false;
        for (int p = sf.parent, tiefe = 0; p >= 0 && static_cast<std::size_t>(p) < m.surfaces.size() && tiefe < 256;
             p = m.surfaces[static_cast<std::size_t>(p)].parent, ++tiefe) {
            if ((aus(static_cast<std::size_t>(p)) & kOhneKinder) != 0U) { abgeschnitten = true; }
        }
        if (abgeschnitten) { continue; }
        ecken.clear();
        ecken.reserve(sf.verts.size() * 3U);
        for (const GlmVertex& v : sf.verts) {
            float p[3] = {0.0F, 0.0F, 0.0F};
            for (int b = 0; b < 4; ++b) {
                const float wgt = v.weights[b];
                if (wgt == 0.0F || v.bones[b] >= skin.size()) { continue; }
                const BoneMatrix& bm = skin[v.bones[b]];
                for (int r = 0; r < 3; ++r) {
                    p[r] += wgt * (bm.m[r][0] * v.xyz[0] + bm.m[r][1] * v.xyz[1] + bm.m[r][2] * v.xyz[2] + bm.m[r][3]);
                }
            }
            for (int r = 0; r < 3; ++r) {
                ecken.push_back(fw[r * 4] * p[0] + fw[r * 4 + 1] * p[1] + fw[r * 4 + 2] * p[2] + fw[r * 4 + 3]);
            }
        }
        idx.assign(sf.indexes.begin(), sf.indexes.end());
        if (g2RadiusTreffer(ecken, idx, start, ende, radius)) { return 1; }
    }
    // rc605 (Rest): die weiteren Modelle des ghoul2-Felds (Waffe, Schwertgriff,
    // Kinomodell) an ihrem Bolzen - wie die Zeichnung (Griff am Handbolzen,
    // baueGriffWelt), Ecken starr im Raum des Modells.
    for (int ai2 = 0; ai2 < d.anbauten && !g_fallenOhneAnbau; ++ai2) {
        const AnbauDraw& an = d.anbau[static_cast<std::size_t>(ai2)];
        if (an.model == nullptr) { continue; }
        BoneMatrix hand{};
        const int hs = surfaceIndex(m, an.bolzen);
        if (hs < 0 || !boltMatrix(m, hs, welt, *d.anim, hand)) { continue; }
        float gw[16];
        gpu::baueGriffWelt(fw, hand, gw);
        for (const GlmSurface& sf : an.model->surfaces) {
            if (sf.flags != 0U || sf.skinnedOff || sf.verts.empty()) { continue; }
            ecken.clear();
            for (const GlmVertex& v : sf.verts) {
                for (int r = 0; r < 3; ++r) {
                    ecken.push_back(gw[r * 4] * v.xyz[0] + gw[r * 4 + 1] * v.xyz[1] + gw[r * 4 + 2] * v.xyz[2] + gw[r * 4 + 3]);
                }
            }
            idx.assign(sf.indexes.begin(), sf.indexes.end());
            if (g2RadiusTreffer(ecken, idx, start, ende, radius)) {
                ++g_app->fallenAnbauTreffer;
                return 1;
            }
        }
    }
    return 0;
}

// g2 false: EG2_Collision 0 (G2_NOCOLLIDE) - nur der Kasten der Figur
// (fx_target_beam_fire, g_fx.cpp:1039 OpenJK).
LaserfalleSpur laserfalleSpurBei(double ms, const float start[3], const float mins[3], const float maxs[3],
                                 const float ende[3], std::size_t selbst, bool g2 = true) {
    LaserfalleSpur r;
    for (int q = 0; q < 3; ++q) { r.endpos[q] = ende[q]; }
    const FallenWelt& w = g_fallenWelt;
    if (!w.k) { return r; }
    const KollisionsSpur welt = kollisionsSpur(*w.k, start, mins, maxs, ende, kMaskeSchussWelt);
    float frac = welt.fraction;
    bool startsolid = welt.startsolid;
    bool allsolid = welt.allsolid;
    bool figur = false;
    float endpos[3] = {welt.endpos[0], welt.endpos[1], welt.endpos[2]};
    float normale[3] = {welt.normale[0], welt.normale[1], welt.normale[2]};
    if (frac != 0.0F) {
        float lo[3];
        float hi[3];
        for (int q = 0; q < 3; ++q) {
            lo[q] = std::min(start[q], ende[q]) + mins[q] - 1.0F;
            hi[q] = std::max(start[q], ende[q]) + maxs[q] + 1.0F;
        }
        for (const FallenBrush& b : w.brushes) {
            if (allsolid) { break; }
            if (b.entity == selbst) { continue; }
            float ort[3] = {b.ort0[0], b.ort0[1], b.ort0[2]};
            float winkel[3] = {0.0F, 0.0F, 0.0F};
            if (b.skript != nullptr) {
                const ActorState st = b.skript->at(ms);
                if (!st.visible) { continue; }
                for (int q = 0; q < 3; ++q) {
                    ort[q] = st.pos[q];
                    winkel[q] = st.angles[q];
                }
            } else {
                const MoverSim::Stand m = g_app->moverSim.at(b.modell, ms);
                if (m.bekannt) {
                    if (!m.sichtbar) { continue; }
                    for (int q = 0; q < 3; ++q) {
                        ort[q] = m.origin[q];
                        winkel[q] = m.angles[q];
                    }
                }
            }
            bool weit = false;
            for (int q = 0; q < 3; ++q) {
                if (ort[q] + b.radius < lo[q] || ort[q] - b.radius > hi[q]) { weit = true; }
            }
            if (weit) { continue; }
            const KollisionsSpur t = kollisionsSpur(*w.k, start, mins, maxs, ende, kMaskeSchussWelt, b.modell, ort, winkel);
            if (t.allsolid) {
                allsolid = true;
                figur = false;
            } else if (t.startsolid) {
                if (!startsolid) { figur = false; }
                startsolid = true;
            }
            if (t.fraction < frac) {
                frac = t.fraction;
                for (int q = 0; q < 3; ++q) {
                    endpos[q] = t.endpos[static_cast<std::size_t>(q)];
                    normale[q] = t.normale[static_cast<std::size_t>(q)];
                }
                figur = false;
                if (t.allsolid) { allsolid = true; }
            }
        }
        // rc605 (Rest): andere Fallen (CONTENTS_SHOTCLIP, -4..4) und
        // misc_model_breakable (CM_TempBoxModel, Winkel 0) - Punkt-Entities ohne
        // Ghoul2: der Kasten gilt.
        const auto kastenNehmen = [&](const float kmin[3], const float kmax[3]) {
            bool weit = false;
            for (int q = 0; q < 3; ++q) {
                if (kmax[q] + 1.0F < lo[q] || kmin[q] - 1.0F > hi[q]) { weit = true; }
            }
            if (weit) { return; }
            const KastenSpur ks = figurKastenSpur(start, ende, mins, maxs, kmin, kmax);
            if (ks.allsolid) {
                allsolid = true;
                figur = false;
            } else if (ks.startsolid) {
                if (!startsolid) { figur = false; }
                startsolid = true;
            }
            if (ks.fraction < frac) {
                frac = ks.fraction;
                for (int q = 0; q < 3; ++q) {
                    endpos[q] = start[q] + ks.fraction * (ende[q] - start[q]);
                    normale[q] = ks.normale[q];
                }
                figur = false;
                if (ks.allsolid) { allsolid = true; }
            }
        };
        for (const FallenMine& fm : w.minen) {
            if (allsolid) { break; }
            if (fm.entity == selbst || ms >= fm.wegMs) { continue; }
            const float kmin[3] = {fm.ort[0] - 4.0F, fm.ort[1] - 4.0F, fm.ort[2] - 4.0F};
            const float kmax[3] = {fm.ort[0] + 4.0F, fm.ort[1] + 4.0F, fm.ort[2] + 4.0F};
            const float altFrac = frac;
            kastenNehmen(kmin, kmax);
            if (frac < altFrac) { ++g_app->fallenFallenTreffer; }
        }
        for (const FallenModell& fm : w.modelle) {
            if (allsolid) { break; }
            if (fm.entity == selbst) { continue; }
            const MoverSim::ModellStand bs = g_app->moverSim.modellAt(fm.entity, ms);
            const bool gebrochen = bs.bekannt && (!bs.sichtbar || bs.modell != fm.pfad);
            if (gebrochen && !fm.festNachBruch) { continue; }
            const float altFrac = frac;
            kastenNehmen(fm.mins, fm.maxs);
            if (frac < altFrac) { ++g_app->fallenModellTreffer; }
        }
        for (const std::size_t ai : w.figuren) {
            if (allsolid) { break; }
            const Actor& a = g_app->scene.actors[ai];
            const ActorState st = a.at(ms);
            if (!st.visible || st.removed) { continue; }
            const float hb = a.halbBreite;
            const float kmin[3] = {st.pos[0] - hb, st.pos[1] - hb, st.pos[2] - 24.0F};
            const float kmax[3] = {st.pos[0] + hb, st.pos[1] + hb, st.pos[2] + (st.crouched ? 16.0F : a.hoeheOben)};
            bool weit = false;
            for (int q = 0; q < 3; ++q) {
                if (kmax[q] + 1.0F < lo[q] || kmin[q] - 1.0F > hi[q]) { weit = true; }
            }
            if (weit) { continue; }
            const KastenSpur ks = figurKastenSpur(start, ende, mins, maxs, kmin, kmax);
            // SV_ClipMoveToEntities: oldTrace, dann allsolid/startsolid und die
            // kuerzere fraction uebernehmen; trace.entityNum == diese Figur ->
            // Ghoul2-Pruefung.
            const float altFrac = frac;
            const bool altStart = startsolid;
            const bool altAll = allsolid;
            const bool altFigur = figur;
            const float altEnd[3] = {endpos[0], endpos[1], endpos[2]};
            const float altNorm[3] = {normale[0], normale[1], normale[2]};
            bool diese = false;
            if (ks.allsolid) {
                allsolid = true;
                diese = true;
            } else if (ks.startsolid) {
                startsolid = true;
                diese = true;
            }
            if (ks.fraction < frac) {
                frac = ks.fraction;
                for (int q = 0; q < 3; ++q) {
                    endpos[q] = start[q] + ks.fraction * (ende[q] - start[q]);
                    normale[q] = ks.normale[q];
                }
                if (ks.allsolid) { allsolid = true; }
                diese = true;
            }
            if (!diese) { continue; }
            if (!g2) {   // G2_NOCOLLIDE: der Kasten gilt
                figur = true;
                continue;
            }
            const float radius = (maxs[0] - mins[0]) / 2.0F;
            const int netz = figurNetzTreffer(ai, st, ms, start, ende, radius);
            if (netz == 0) {
                frac = altFrac;
                startsolid = altStart;
                allsolid = altAll;
                figur = altFigur;
                for (int q = 0; q < 3; ++q) {
                    endpos[q] = altEnd[q];
                    normale[q] = altNorm[q];
                }
                continue;
            }
            ++(netz > 0 ? g_app->fallenNetzTreffer : g_app->fallenKastenTreffer);
            figur = true;
        }
    }
    for (int q = 0; q < 3; ++q) {
        r.endpos[q] = endpos[q];
        r.normale[q] = normale[q];
    }
    r.startsolid = startsolid;
    r.figur = figur;
    return r;
}
}  // namespace

// --- rc604 fix604b (B5): der Strahl der misc_trip_mine ----------------
//
// CG_General (code/cgame/cg_ents.cpp:754-795 OpenJK) spielt bei EF_FIRING in
// JEDEM Bild "tripMine/laser" an origin + 6.6 * axis[0] - eine Kopie, deren
// line1/line2 origin2 auf pos4 gesetzt ist (cheapOrg2Calc: Weltlage) - und
// "tripMine/laserImpactGlow" an pos4. Ohne life leben die Teilchen 50 ms
// (Vorgabe der Vorlage): als Wiederholung alle 50 ms, solange der Strahl
// mit diesem pos4 steht.
//
// rc604: wann und bis wohin, rechnet laserfalleLauf (bhed/mover.h) Denkbild
// fuer Denkbild (laserTrapThink alle FRAMETIME): die Spur laeuft gegen die
// Welt, die Brush-Modelle dort, wo sie dann stehen, und die Figuren (Kasten,
// dann G2_RadiusTracePolys am Netz) - siehe laserfalleSpurBei. Eine Figur im
// Strahl laesst die Falle explodieren (WP_Explode: "tripMine/explosion",
// die Schleife aus, nach 50 ms ist sie fort). Die uses (misc_trip_mine_
// activate) kommen aus der Mover-Simulation: START_OFF wird scharf, eine
// scharfe Falle geht aus.
// --- rc604: Wachgeschuetz und Ionenkanone bewegen sich wie im Spiel ---------
//
// misc_sentry_turret (SP_PAS, code/game/g_turret.cpp:1784-1890 OpenJK): das
// erste pas_think (spawn + 1000, nach pas_use + 50) klappt das psgun.glm auf -
// G2API_SetBoneAnimIndex(rootBone, 0, 11, BONE_ANIM_OVERRIDE_FREEZE, 0.8)
// (:1626-1633). misc_ion_cannon (ion_cannon_think, :1930-1976): je Schuss
// G_PlayEffect("env/ion_cannon") am Bolzen *flash02 (Ort und POSITIVE_Y der
// Bolzenmatrix mit s.angles, s.origin und modelScale 2 zur Zeit des Schusses),
// dann die Animation 0..8 mit 0.6. Die Zeiten rechnen pasAufklappenMs und
// ionenkanoneLauf (bhed/mover.h); die uses kommen aus der Mover-Simulation.
// rc605 FU4: das Schwenken des Wachgeschuetzes ohne Feind (pas_think: bone_hinge
// per G2API_SetBoneAngles, BONE_ANGLES_POSTMULT) - pasSchwenkLauf (bhed/mover.h)
// und geschuetzKnochen. Nicht nachgebildet: alles mit einem Feind (Zielen,
// pas_fire; pas_find_enemies findet auch den Spieler, ausser mit SET_NO_TARGET);
// G_UseTargets2 (target2) der Ionenkanone.
bool geschuetzKnochen(const App::ActorAssets& as, const std::vector<KnochenAnim>& anims, double ms,
                      std::vector<BoneMatrix>& welt, std::vector<BoneMatrix>& skin,
                      const std::vector<PasSchwenk>* schwenk = nullptr) {
    const App::Skeleton* sk = skeletonFor(as.model);
    if (sk == nullptr || sk->anim.empty()) { return false; }
    // rc605 FU4: pas_think ohne Feind - G2API_SetBoneAngles("bone_hinge",
    // (0, 0, speed - s.angles[YAW]), BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z,
    // POSITIVE_X) vom letzten think vor ms (g_turret.cpp:2118-2123 MD). G2_Generate_
    // Matrix: up POSITIVE_Y -> newAngles[1] = angles[0], left POSITIVE_Z ->
    // newAngles[0] = angles[1], forward POSITIVE_X -> newAngles[2] = angles[2];
    // Create_Matrix = AnglesToAxis als Spalten (G2_misc.cpp:1691-1712 OpenJK).
    // bone_barrel/bone_gback bekommen (-random, 0, 0) = 0: keine Aenderung.
    std::vector<std::pair<int, BoneMatrix>> nachMal;
    if (schwenk != nullptr) {
        const PasSchwenk* letzter = nullptr;
        for (const PasSchwenk& p : *schwenk) {
            if (p.ms > ms) { break; }
            letzter = &p;
        }
        if (letzter != nullptr) {
            int scharnier = -1;
            for (std::size_t b = 0; b < sk->anim.bones.size(); ++b) {
                const std::string& kn = sk->anim.bones[b].name;
                if (kn.size() == 10 && std::equal(kn.begin(), kn.end(), "bone_hinge", [](char x, char y) {
                        return std::tolower(static_cast<unsigned char>(x)) == y;
                    })) {
                    scharnier = static_cast<int>(b);
                }
            }
            if (scharnier >= 0) {
                const float w[3] = {0.0F, 0.0F, letzter->gier};
                float achse[3][3];
                winkelZuAchsen(w, achse);
                BoneMatrix m = BoneMatrix::identity();
                for (int r = 0; r < 3; ++r) {
                    for (int c = 0; c < 3; ++c) { m.m[r][c] = achse[c][r]; }
                    m.m[r][3] = 0.0F;
                }
                nachMal.emplace_back(scharnier, m);
            }
        }
    }
    // Die zuletzt begonnene Animation gilt (G2API_SetBoneAnimIndex ersetzt
    // die vorige); vorher steht der Knochen auf Bild 0 (G2_TransformGhoulBones).
    int bild = 0;
    int naechstes = 0;
    float lerp = 0.0F;
    for (const KnochenAnim& a : anims) {
        if (a.startMs > ms) { break; }
        g2Bild(a, ms, bild, naechstes, lerp);
    }
    sk->anim.worldMatricesLerp(bild, naechstes, lerp, {}, nachMal, welt);
    if (welt.empty()) { return false; }
    gpu::baueKnochenmatrizen(welt, sk->anim.bones, skin);
    return true;
}

// rc605 FU4 (Selbsttest geschuetzanim): um wie viel Grad der Schwenk den
// Knochen `knochen` des Wachgeschuetzes `ent` bei ms gegen die Stellung ohne
// G2API_SetBoneAngles dreht (Winkel der Drehung zwischen beiden Matrizen;
// -1: kein Modell, keine Animation, kein Knochen).
float pasKnochenDrehung(std::size_t ent, double ms, const std::string& knochen) {
    const App::ActorAssets* as = gegenstandGlmFuer("models/items/psgun.glm");
    const auto ga = g_app->geschuetzAnims.find(ent);
    const auto ps = g_app->pasSchwenks.find(ent);
    const App::Skeleton* sk = as != nullptr ? skeletonFor(as->model) : nullptr;
    if (as == nullptr || sk == nullptr || ga == g_app->geschuetzAnims.end() || ps == g_app->pasSchwenks.end()) { return -1.0F; }
    std::vector<BoneMatrix> w1;
    std::vector<BoneMatrix> s1;
    std::vector<BoneMatrix> w2;
    std::vector<BoneMatrix> s2;
    if (!geschuetzKnochen(*as, ga->second, ms, w1, s1, nullptr) || !geschuetzKnochen(*as, ga->second, ms, w2, s2, &ps->second)) {
        return -1.0F;
    }
    // Namen ohne Gross/Klein (G2_Find_Bone: Q_stricmp); "*kind": das erste Kind
    // von bone_hinge.
    const auto gleich = [](const std::string& x, const std::string& y) {
        return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin(), [](char p, char q) {
                   return std::tolower(static_cast<unsigned char>(p)) == std::tolower(static_cast<unsigned char>(q));
               });
    };
    int scharnier = -1;
    for (std::size_t b = 0; b < sk->anim.bones.size(); ++b) {
        if (gleich(sk->anim.bones[b].name, "bone_hinge")) { scharnier = static_cast<int>(b); }
    }
    for (std::size_t b = 0; b < sk->anim.bones.size() && b < w1.size() && b < w2.size(); ++b) {
        const bool treffer = knochen == "*kind" ? (scharnier >= 0 && sk->anim.bones[b].parent == scharnier)
                                                : gleich(sk->anim.bones[b].name, knochen);
        if (!treffer) { continue; }
        // Spur von R1^T * R2 nach Normieren der Spalten: cos = (Spur - 1) / 2.
        double spur = 0.0;
        for (int c = 0; c < 3; ++c) {
            double l1 = 0.0;
            double l2 = 0.0;
            double d = 0.0;
            for (int r = 0; r < 3; ++r) {
                l1 += static_cast<double>(w1[b].m[r][c]) * w1[b].m[r][c];
                l2 += static_cast<double>(w2[b].m[r][c]) * w2[b].m[r][c];
                d += static_cast<double>(w1[b].m[r][c]) * w2[b].m[r][c];
            }
            spur += (l1 > 0.0 && l2 > 0.0) ? d / std::sqrt(l1 * l2) : 0.0;
        }
        return static_cast<float>(std::acos(std::clamp((spur - 1.0) / 2.0, -1.0, 1.0)) * 180.0 / 3.14159265358979323846);
    }
    return -1.0F;
}

void geschuetzeTakten() {
    g_app->geschuetzAnims.clear();
    g_app->pasSchwenks.clear();   // rc605 FU4
    g_app->ionenSchuesse = 0;
    const double endeMs = std::max(g_app->camTrack.durationMs, g_app->timeline.practicalEndMs) + 5000.0;
    const bool md = regelwerkMovieDuels();
    const std::string fxPfad = "effects/env/ion_cannon.efx";
    for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
        const MapEntity& e = g_app->map.entities[i];
        const bool pas = e.classname == "misc_sentry_turret";
        const bool ion = e.classname == "misc_ion_cannon";
        if ((!pas && !ion) || !kartenpruefung::imSpiel(e.classname, md) || !gespawntImSpiel(e) || e.origin.empty()) {
            continue;
        }
        int sf = 0;
        if (const std::string* s = e.find("spawnflags")) { sf = std::atoi(s->c_str()); }
        std::vector<double> uses;
        for (const auto& [ent, ms] : g_app->moverSim.entityUses()) {
            if (ent == i) { uses.push_back(ms); }
        }
        std::stable_sort(uses.begin(), uses.end());
        if (pas) {
            const double t = pasAufklappenMs((sf & 1) != 0, kNavKartenSpawnMs, uses, endeMs);
            // Ohne Animation steht der Knochen auf Bild 0 - auch das ist ein Eintrag.
            std::vector<KnochenAnim>& anims = g_app->geschuetzAnims[i];
            if (t >= 0.0) { anims.push_back(KnochenAnim{t, 0, 11, 0.8F}); }
            // rc605 FU4: der Schwenk ohne Feind (pas_think); s.angles[YAW] und
            // count (SP_PAS: 0 -> 150) aus der Entity.
            float gier = 0.0F;
            if (const std::string* a = e.find("angles")) {
                float w3[3] = {0.0F, 0.0F, 0.0F};
                (void)std::sscanf(a->c_str(), "%f %f %f", &w3[0], &w3[1], &w3[2]);
                gier = w3[1];
            } else if (const std::string* a1 = e.find("angle")) {
                gier = static_cast<float>(std::atof(a1->c_str()));
            }
            int munition = 0;
            if (const std::string* c = e.find("count")) { munition = std::atoi(c->c_str()); }
            if (munition == 0) { munition = 150; }
            g_app->pasSchwenks[i] = pasSchwenkLauf((sf & 1) != 0, kNavKartenSpawnMs, uses, endeMs, gier, munition);
            continue;
        }
        IonenkanoneEingabe ein;
        ein.startOff = (sf & 1) != 0;
        ein.bursts = (sf & 2) != 0;
        const auto zahl = [&e](const char* k) {
            const std::string* v = e.find(k);
            return v == nullptr ? 0.0F : static_cast<float>(std::atof(v->c_str()));
        };
        ein.wait = zahl("wait");
        ein.random = zahl("random");
        ein.delay = static_cast<float>(static_cast<int>(zahl("delay")));   // FOFS(delay) ist int
        ein.spawnMs = kNavKartenSpawnMs;
        ein.uses = uses;
        ein.endeMs = endeMs;
        ein.saat = static_cast<std::uint64_t>(i) * 0x9e3779b97f4a7c15ULL + 0x632be59bd9b4e019ULL;
        const IonenkanoneErgebnis lr = ionenkanoneLauf(ein);
        std::vector<KnochenAnim>& anims = g_app->geschuetzAnims[i];
        if (lr.schuesse.empty()) { continue; }
        const App::ActorAssets* as = gegenstandGlmFuer("models/map_objects/imp_mine/ion_cannon.glm");
        const bool fx = ladeEffektDatei(fxPfad, "env/ion_cannon");
        float ort[3] = {0.0F, 0.0F, 0.0F};
        float w[3] = {0.0F, 0.0F, 0.0F};
        (void)std::sscanf(e.origin.c_str(), "%f %f %f", &ort[0], &ort[1], &ort[2]);
        if (const std::string* a = e.find("angles")) {
            (void)std::sscanf(a->c_str(), "%f %f %f", &w[0], &w[1], &w[2]);
        } else if (const std::string* a1 = e.find("angle")) {
            w[1] = static_cast<float>(std::atof(a1->c_str()));
        }
        float achse[3][3];
        winkelZuAchsen(w, achse);
        const int bolzen = as != nullptr ? surfaceIndex(as->model, "*flash02") : -1;
        std::vector<BoneMatrix> welt;
        std::vector<BoneMatrix> skin;
        for (std::size_t k = 0; k < lr.schuesse.size(); ++k) {
            const double t = lr.schuesse[k];
            const App::Skeleton* sk = as != nullptr ? skeletonFor(as->model) : nullptr;
            if (fx && as != nullptr && sk != nullptr && bolzen >= 0 && geschuetzKnochen(*as, anims, t, welt, skin)) {
                BoneMatrix b{};
                if (boltMatrix(as->model, bolzen, welt, sk->anim, b)) {
                    EffectInstance x;
                    x.effect = &g_app->effects[fxPfad];
                    float laenge = 0.0F;
                    for (int r = 0; r < 3; ++r) {
                        // modelScale 2 (SP_misc_ion_cannon), AnglesToAxis(s.angles).
                        x.origin[r] = ort[r];
                        x.normal[r] = 0.0F;
                        for (int q = 0; q < 3; ++q) {
                            x.origin[r] += achse[q][r] * 2.0F * b.m[q][3];
                            x.normal[r] += achse[q][r] * b.m[q][1];
                        }
                        laenge += x.normal[r] * x.normal[r];
                    }
                    laenge = std::sqrt(laenge);
                    for (float& n : x.normal) { n = laenge > 0.0F ? n / laenge : 0.0F; }
                    x.startMs = t;
                    x.loops = false;
                    x.seed = static_cast<unsigned>(i * 2654435761U + k * 40503U) | 1U;
                    g_app->effectRunners.push_back(x);
                    ++g_app->ionenSchuesse;
                }
            }
            anims.push_back(KnochenAnim{t, 0, 8, 0.6F});
        }
        diag::detail("misc_ion_cannon #" + std::to_string(i) + ": " + std::to_string(lr.schuesse.size()) +
                     " Schuesse, der erste bei " + std::to_string(static_cast<int>(lr.schuesse.front())) + " ms");
    }
}

// rc605 (Rest) Selbsttest: G2_TraceModels einer Figur (Netz der Figur, mit oder
// ohne die Modelle am Bolzen) fuer eine Spur mit Radius 4 (Kasten -4..4).
int fallenFigurProbe(std::size_t ai, double ms, const float start[3], const float ende[3], bool mitAnbau) {
    if (ai >= g_app->scene.actors.size()) { return -1; }
    const ActorState st = g_app->scene.actors[ai].at(ms);
    g_fallenOhneAnbau = !mitAnbau;
    const int r = figurNetzTreffer(ai, st, ms, start, ende, 4.0F);
    g_fallenOhneAnbau = false;
    return r;
}

// rc605 (Rest) Selbsttest: die Spur von laserTrapThink der Falle `entity` zur
// Zeit ms (neuBauen: die Hindernisse vorher aus den Entities neu lesen).
bool fallenStrahlProbe(std::size_t entity, double ms, float ende[3], float normale[3], bool neuBauen) {
    if (neuBauen) { fallenWeltBauen(); }
    TripMineLage tl;
    if (entity >= g_app->map.entities.size() || !tripMineLage(g_app->map.entities[entity], tl)) { return false; }
    float en[3];
    for (int q = 0; q < 3; ++q) { en[q] = tl.ort[q] + 2048.0F * tl.normale[q]; }
    const float mn[3] = {-4.0F, -4.0F, -4.0F};
    const float mx[3] = {4.0F, 4.0F, 4.0F};
    const LaserfalleSpur s = laserfalleSpurBei(ms, tl.origin2, mn, mx, en, entity);
    for (int q = 0; q < 3; ++q) {
        ende[q] = s.endpos[q];
        normale[q] = s.normale[q];
    }
    return true;
}

void fallenEffekteSetzen() {
    {
        // rc604: ein zweiter Aufruf (prepareScene, nach loadActorModels - dann
        // mit dem Netz der Figuren) ersetzt die Instanzen des ersten.
        std::set<const efx::Effect*> alt;
        for (const auto& [pfad, fx] : g_app->effects) {
            if (pfad.rfind("effects/tripMine/", 0) == 0) { alt.insert(&fx); }
        }
        g_app->effectRunners.erase(std::remove_if(g_app->effectRunners.begin(), g_app->effectRunners.end(),
                                                  [&alt](const EffectInstance& x) { return alt.count(x.effect) != 0U; }),
                                   g_app->effectRunners.end());
        int strahlen = 0;
        const std::string laserPfad = "effects/tripMine/laser.efx";
        const std::string glutPfad = "effects/tripMine/laserImpactGlow.efx";
        const std::string knallPfad = "effects/tripMine/explosion.efx";
        const bool laser = ladeEffektDatei(laserPfad, "tripMine/laser");
        const bool glut = ladeEffektDatei(glutPfad, "tripMine/laserImpactGlow");
        const bool knall = ladeEffektDatei(knallPfad, "tripMine/explosion");
        g_app->fallenKlaenge.clear();   // rc604
        g_app->fallenSicht.clear();
        g_app->fallenExplosionen = 0;
        g_app->fallenKettenExplosionen = 0;   // rc605 (Ende)
        g_app->fallenUsesNeu.clear();   // rc605 (fix605)
        g_app->fallenKnalleNeu.clear();   // rc605 (Ende)
        fallenWeltBauen();
        const auto fallenBeginn = std::chrono::steady_clock::now();
        std::size_t fallenSpuren = 0;
        const double endeMs = std::max(g_app->camTrack.durationMs, g_app->timeline.practicalEndMs) + 5000.0;
        // rc605 (Rest): eine Falle haelt die Strahlen der anderen auf, bis
        // G_FreeEntity sie nach WP_Explode entfernt - die Lebenszeiten haengen
        // voneinander ab: rechnen, bis sie stehen (hoechstens 4 Runden).
        const auto eingabeFuer = [&](std::size_t i, const TripMineLage& tl) {
            LaserfalleEingabe ein;
            for (int q = 0; q < 3; ++q) {
                ein.ort[q] = tl.ort[q];
                ein.richtung[q] = tl.normale[q];
                ein.origin2[q] = tl.origin2[q];
            }
            ein.anAbSpawn = tl.strahl;
            ein.sichtbarAbSpawn = tl.sichtbarAbSpawn;
            ein.hatUse = tl.hatName;
            ein.spawnMs = kNavKartenSpawnMs;
            ein.endeMs = endeMs;
            for (const auto& [ent, ms] : g_app->moverSim.entityUses()) {
                if (ent == i) { ein.uses.push_back(ms); }
            }
            std::stable_sort(ein.uses.begin(), ein.uses.end());
            for (const FallenMine& fm : g_fallenWelt.minen) {
                if (fm.entity == i) { ein.zwangMs = fm.zwangMs; }
            }
            return ein;
        };
        // rc605 (Ende): Kettenexplosionen. WP_Explode (g_weapon.cpp:185-230
        // OpenJK) nimmt sich takedamage und ruft G_RadiusDamage(currentOrigin,
        // attacker, splashDamage 90, splashRadius 256) - LT_SPLASH_DAM /
        // LT_SPLASH_RAD (weapons.h OpenJK, Movie Duels und codeJK2 gleich; die
        // weapons.dat von base und MD setzen sie nicht). G_RadiusDamage
        // (code/game/g_combat.cpp:6850-6955 OpenJK; g_combat.cpp:9704 ff. MD):
        // je Entity mit takedamage und contents der Abstand zum absmin/absmax
        // (Falle: +-4, SV_LinkEntity +1), unter dem Radius points = damage *
        // (1 - dist / radius), CanDamage (Punktspuren MASK_SOLID zur Mitte und
        // +-15 in x/y), G_Damage mit (int)points: health 15 (touchLaserTrap)
        // abwaerts, bei <= 0 WP_ExplosiveDie -> WP_Explode sofort (noch im
        // G_RadiusDamage der ersten, in der Reihenfolge der Entities).
        const auto kannTreffen = [](double ms, const float von[3], const float mitte[3]) {
            return fallenKannTreffenBei(ms, von, mitte);   // rc605 (Ende): gemeinsam mit den Figuren
        };
        // natuerlich: je Falle die Zeit, zu der eine Figur sie ausloest (1e300:
        // nie). Ergebnis: zwangMs je Falle; true, wenn sich etwas aenderte.
        const auto fallenKette = [&](const std::map<std::size_t, double>& natuerlich) {
            std::vector<FallenMine>& mi = g_fallenWelt.minen;
            std::vector<double> knall(mi.size(), 1.0e300);
            std::vector<int> leben(mi.size(), 15);
            std::vector<double> zwang(mi.size(), 1.0e300);
            std::function<void(std::size_t, double)> sprengt = [&](std::size_t a, double t) {
                for (std::size_t b = 0; b < mi.size(); ++b) {
                    if (b == a || knall[b] <= t || t < mi[b].schadenAb) { continue; }
                    float v[3];
                    for (int q = 0; q < 3; ++q) {
                        const float lo = mi[b].ort[q] - 5.0F;
                        const float hi = mi[b].ort[q] + 5.0F;
                        const float o = mi[a].ort[q];
                        v[q] = o < lo ? lo - o : (o > hi ? o - hi : 0.0F);
                    }
                    const float dist = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
                    if (dist >= 256.0F) { continue; }
                    const float points = 90.0F * (1.0F - dist / 256.0F);
                    if (!kannTreffen(t, mi[a].ort, mi[b].ort)) { continue; }
                    leben[b] -= static_cast<int>(points);
                    if (leben[b] <= 0) {
                        knall[b] = t;
                        zwang[b] = t;
                        sprengt(b, t);
                    }
                }
            };
            std::vector<std::pair<double, std::size_t>> reihe;
            for (std::size_t i = 0; i < mi.size(); ++i) {
                const auto it = natuerlich.find(mi[i].entity);
                if (it != natuerlich.end() && it->second < 1.0e299) { reihe.emplace_back(it->second, i); }
            }
            std::stable_sort(reihe.begin(), reihe.end(), [](const auto& x, const auto& y) { return x.first < y.first; });
            for (const auto& [t, a] : reihe) {
                if (knall[a] <= t) { continue; }
                knall[a] = t;
                sprengt(a, t);
            }
            bool anders = false;
            for (std::size_t i = 0; i < mi.size(); ++i) {
                if (mi[i].zwangMs != zwang[i]) {
                    mi[i].zwangMs = zwang[i];
                    anders = true;
                }
            }
            return anders;
        };
        if (g_fallenWelt.minen.size() > 1U) {
            for (int runde = 0; runde < 4; ++runde) {
                std::map<std::size_t, double> weg;
                std::map<std::size_t, double> natuerlich;   // rc605 (Ende)
                for (const FallenMine& fm : g_fallenWelt.minen) {
                    TripMineLage tl;
                    if (!tripMineLage(g_app->map.entities[fm.entity], tl)) { continue; }
                    const std::size_t i = fm.entity;
                    const LaserfalleErgebnis lr = laserfalleLauf(
                        eingabeFuer(i, tl), [i, &fallenSpuren](double ms, const float s[3], const float mn[3], const float mx[3], const float en[3]) {
                            ++fallenSpuren;
                            return laserfalleSpurBei(ms, s, mn, mx, en, i);
                        });
                    weg[i] = lr.wegMs >= 0.0 ? lr.wegMs : 1.0e300;
                    natuerlich[i] = (lr.explosionen.empty() || lr.gezwungen) ? 1.0e300 : lr.explosionen.front();
                }
                bool gleich = !fallenKette(natuerlich);   // rc605 (Ende)
                for (FallenMine& fm : g_fallenWelt.minen) {
                    const auto it = weg.find(fm.entity);
                    const double neu = it == weg.end() ? 1.0e300 : it->second;
                    if (neu != fm.wegMs) {
                        gleich = false;
                        fm.wegMs = neu;
                    }
                }
                if (gleich) { break; }
            }
            g_app->fallenFallenTreffer = 0;
            g_app->fallenModellTreffer = 0;
            g_app->fallenAnbauTreffer = 0;
            g_app->fallenNetzTreffer = 0;
            g_app->fallenKastenTreffer = 0;
        }
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            const MapEntity& e = g_app->map.entities[i];
            TripMineLage tl;
            if (!kartenpruefung::imSpiel(e.classname, regelwerkMovieDuels()) || !gespawntImSpiel(e) || !tripMineLage(e, tl)) {
                continue;
            }
            LaserfalleEingabe ein;
            for (int q = 0; q < 3; ++q) {
                ein.ort[q] = tl.ort[q];
                ein.richtung[q] = tl.normale[q];
                ein.origin2[q] = tl.origin2[q];
            }
            ein.anAbSpawn = tl.strahl;
            ein.sichtbarAbSpawn = tl.sichtbarAbSpawn;
            ein.hatUse = tl.hatName;
            ein.spawnMs = kNavKartenSpawnMs;
            ein.endeMs = endeMs;
            for (const auto& [ent, ms] : g_app->moverSim.entityUses()) {
                if (ent == i) { ein.uses.push_back(ms); }
            }
            std::stable_sort(ein.uses.begin(), ein.uses.end());
            for (const FallenMine& fm : g_fallenWelt.minen) {
                if (fm.entity == i) { ein.zwangMs = fm.zwangMs; }   // rc605 (Ende)
            }
            const LaserfalleErgebnis lr = laserfalleLauf(
                ein, [i, &fallenSpuren](double ms, const float s[3], const float mn[3], const float mx[3], const float en[3]) {
                    ++fallenSpuren;
                    return laserfalleSpurBei(ms, s, mn, mx, en, i);
                });
            g_app->fallenSicht[i] = {lr.sichtbarAbMs, lr.wegMs};
            // laserTrapThink, erster Durchgang ("arm me", wp_trip_mine.cpp:
            // 140-146 OpenJK): G_Sound(warning.wav) - eine Temp-Entity
            // EV_GENERAL_SOUND am Ort - und s.loopSound = hum_loop.wav, bis die
            // Falle ausgeht oder explodiert.
            for (const double ms : lr.scharfMs) {
                KartenKlang kk;
                kk.entity = i;
                kk.datei = "sound/weapons/laser_trap/warning.wav";
                kk.ms = ms;
                kk.tempEntity = true;
                for (int q = 0; q < 3; ++q) { kk.ort[q] = tl.ort[q]; }
                g_app->fallenKlaenge.push_back(std::move(kk));
            }
            for (const auto& [ab, bis] : lr.summen) {
                KartenKlang kk;
                kk.entity = i;
                kk.datei = "sound/weapons/laser_trap/hum_loop.wav";
                kk.ms = ab;
                kk.bisMs = bis;
                kk.schleife = true;
                for (int q = 0; q < 3; ++q) { kk.ort[q] = tl.ort[q]; }
                g_app->fallenKlaenge.push_back(std::move(kk));
            }
            // WP_Explode (g_weapon.cpp:185-230 OpenJK): G_PlayEffect(fxID,
            // currentOrigin, AngleVectors(s.angles)) - forward ist die Normale.
            if (knall) {
                float achse[3][3];
                winkelZuAchsen(tl.winkel, achse);
                for (const double ms : lr.explosionen) {
                    EffectInstance x;
                    x.effect = &g_app->effects[knallPfad];
                    for (int q = 0; q < 3; ++q) {
                        x.origin[q] = tl.ort[q];
                        x.normal[q] = achse[0][q];
                    }
                    x.startMs = ms;
                    x.loops = false;
                    x.seed = static_cast<unsigned>(i * 2654435761U + 12345U) | 1U;
                    g_app->effectRunners.push_back(x);
                }
            }
            g_app->fallenExplosionen += lr.explosionen.size();
            // rc605 (Ende): G_RadiusDamage trifft auch Figuren (hurtTodeBerechnen).
            for (const double ms : lr.explosionen) {
                App::FallenKnall kn;
                kn.ms = ms;
                for (int q = 0; q < 3; ++q) { kn.ort[q] = tl.ort[q]; }
                kn.entity = static_cast<int>(i);
                g_app->fallenKnalleNeu.push_back(kn);
            }
            // rc605 (fix605): WP_Explode -> G_UseTargets( self, attacker ).
            if (const std::string* ziel = e.find("target"); ziel != nullptr && !ziel->empty()) {
                for (const double ms : lr.explosionen) {
                    g_app->fallenUsesNeu.push_back(AblaufWelt::FallenUse{ms, *ziel, static_cast<int>(i)});
                }
            }
            if (!lr.explosionen.empty() && lr.gezwungen) {
                diag::detail("misc_trip_mine #" + std::to_string(i) + ": von einer anderen Falle gesprengt (G_RadiusDamage) - WP_Explode bei " +
                             std::to_string(static_cast<int>(lr.explosionen.front())) + " ms");
                ++g_app->fallenKettenExplosionen;
            } else if (!lr.explosionen.empty()) {
                diag::detail("misc_trip_mine #" + std::to_string(i) + ": eine Figur im Strahl - WP_Explode bei " +
                             std::to_string(static_cast<int>(lr.explosionen.front())) + " ms (Treffer am Netz " +
                             std::to_string(g_app->fallenNetzTreffer) + ", nur Kasten " + std::to_string(g_app->fallenKastenTreffer) +
                             ", davon an einem Modell am Bolzen " + std::to_string(g_app->fallenAnbauTreffer) + ")");
            }
            for (std::size_t si = 0; si < lr.strahlen.size(); ++si) {
                const LaserfalleStrahl& ls = lr.strahlen[si];
                EffectInstance inst;
                inst.startMs = ls.abMs;
                inst.endMs = ls.bisMs;
                inst.loops = true;
                inst.intervalMs = 50.0F;
                if (laser) {
                    const std::string kopie = laserPfad + "#" + std::to_string(i) + (si == 0 ? "" : "_" + std::to_string(si));
                    efx::Effect fx = g_app->effects[laserPfad];
                    for (efx::Primitive& pr : fx.primitives) {
                        if (pr.name == "line1" || pr.name == "line2") {
                            pr.origin2.min = efx::Vec3{ls.ende[0], ls.ende[1], ls.ende[2]};
                            pr.origin2.max = pr.origin2.min;
                            pr.origin2.set = true;
                            pr.origin2.ranged = false;
                        }
                    }
                    g_app->effects[kopie] = std::move(fx);
                    EffectInstance s1 = inst;
                    s1.effect = &g_app->effects[kopie];
                    for (int q = 0; q < 3; ++q) {
                        s1.origin[q] = tl.ort[q] + 6.6F * tl.normale[q];
                        s1.normal[q] = tl.normale[q];
                    }
                    s1.seed = static_cast<unsigned>(i * 2654435761U) | 1U;
                    g_app->effectRunners.push_back(s1);
                }
                if (glut) {
                    EffectInstance s2 = inst;
                    s2.effect = &g_app->effects[glutPfad];
                    for (int q = 0; q < 3; ++q) {
                        s2.origin[q] = ls.ende[q];
                        s2.normal[q] = tl.normale[q];
                    }
                    s2.seed = static_cast<unsigned>(i * 40503U + 977U) | 1U;
                    g_app->effectRunners.push_back(s2);
                }
            }
            if (!lr.strahlen.empty()) { ++strahlen; }
        }
        if (strahlen != 0) {
            const double dauer =
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - fallenBeginn).count();
            char z[160];
            std::snprintf(z, sizeof(z), "%d Strahlen von misc_trip_mine (%zu Spuren von laserTrapThink, %.1f ms)", strahlen,
                          fallenSpuren, dauer);
            diag::info(z);
        }
    }
}

// rc604 Endrunde: die Folgeeffekte (impactfx, deathfx, emitfx, playfx) Stufe
// fuer Stufe vorab lesen, samt Shadern. Aus loadMapEffects herausgezogen,
// damit auch ein mitten im Bild nachgeladener Effekt (effectByName, etwa ein
// Schusseffekt) seine Shader und Folgeeffekte gleich bekommt - sonst holte
// erst das naechste prepareScene sie nach (md_ga_jedi: 5-8 Bilder).
void ladeFolgeeffekte(std::vector<const efx::Effect*> offen, int* nachgeladenAus, int* ohneDateiAus) {
    std::set<std::string> gesehen;
    int nachgeladen = 0;
    int ohneDatei = 0;
    while (!offen.empty()) {
        const efx::Effect* ef = offen.back();
        offen.pop_back();
        for (const efx::Primitive& pr : ef->primitives) {
            for (const auto* liste :
                 {&pr.impactFx, &pr.deathFx, &pr.emitFx, &pr.playFx}) {
                for (const std::string& name : *liste) {
                    if (!gesehen.insert(name).second) {
                        continue;
                    }
                    const std::string pfad = "effects/" + name + ".efx";
                    if (g_app->effects.count(pfad) != 0U) {
                        continue;   // schon da (Runner oder Waffe)
                    }
                    std::string text;
                    efx::Effect gelesen;
                    if (readFromArchives(pfad, text)) {
                        const efx::ReadResult rr = efx::read(text);
                        if (!rr.hasErrors()) {
                            gelesen = rr.effect;
                        }
                    }
                    // Auch den Fehlschlag merken - effectByName fragt
                    // sonst je Bild erneut im Archiv nach.
                    const auto put =
                        g_app->effects.emplace(pfad, std::move(gelesen));
                    if (put.first->second.primitives.empty()) {
                        ++ohneDatei;
                        diag::detail("Folgeeffekt " + pfad +
                                     " nicht gefunden oder leer");
                        continue;
                    }
                    ladeEffektShader(put.first->second);
                    offen.push_back(&put.first->second);
                    ++nachgeladen;
                }
            }
        }
    }
    if (nachgeladenAus != nullptr) { *nachgeladenAus = nachgeladen; }
    if (ohneDateiAus != nullptr) { *ohneDateiAus = ohneDatei; }
}

void loadMapEffects() {
    diag::Step step("Effekte suchen");
    g_app->effectRunners.clear();
    int gefunden = 0;
    int fehlt = 0;
    int wartend = 0;
    for (const MapEntity& e : g_app->map.entities) {
        if (e.classname != "fx_runner" || !gespawntImSpiel(e)) {   // rc604
            continue;
        }
        const std::string* file = e.find("fxFile");
        if (file == nullptr || file->empty() || e.origin.empty()) {
            continue;
        }
        std::string name = *file;
        const std::size_t dot = name.find_last_of('.');
        const std::size_t slash = name.find_last_of('/');
        if (dot != std::string::npos &&
            (slash == std::string::npos || dot > slash)) {
            name = name.substr(0, dot);
        }
        const std::string path = "effects/" + name + ".efx";

        if (!ladeEffektDatei(path, *file)) {
            ++fehlt;
            continue;
        }

        EffectInstance inst;
        inst.effect = &g_app->effects[path];
        float o[3] = {0, 0, 0};
        (void)std::sscanf(e.origin.c_str(), "%f %f %f", &o[0], &o[1], &o[2]);
        for (int k = 0; k < 3; ++k) {
            inst.origin[k] = o[k];
        }
        // --- Die Richtung des Runners --------------------------------
        //
        // Sie war nie gesetzt und stand damit auf der Vorgabe (0,0,1) -
        // zufaellig richtig, aber aus dem falschen Grund.
        //
        // SP_fx_runner (g_fx.cpp:242) sagt es ausdruecklich:
        //
        //     if ( !G_SpawnAngleHack( "angle", "0", ent->s.angles ))
        //     {
        //         // didn't have angles, so give us the default of up
        //         VectorSet( ent->s.angles, -90, 0, 0 );
        //     }
        //
        // Ohne `angle` zeigt ein fx_runner also NACH OBEN, nicht nach +x.
        // Das ist keine Kleinigkeit: in md_am_sith hat KEIN EINZIGER der
        // 41 Runner ein `angles`. Neun davon haben ein `target`, die
        // uebrigen gar nichts.
        //
        // Und es zaehlt erst, seit die oertliche Geschwindigkeit in diese
        // Achse gedreht wird (siehe inAchse in efxdraw.cpp). Vorher war
        // die Richtung ohne Wirkung - deshalb ist sie nie aufgefallen.
        inst.normal[0] = 0.0F;
        inst.normal[1] = 0.0F;
        inst.normal[2] = 1.0F;
        if (const std::string* t = e.find("target")) {
            // `target` zeigt auf einen Punkt; die Richtung ist der Weg
            // dorthin. Das ist die uebliche Art, einen Dampfstrahl zu
            // richten - neun der Runner in md_am_sith machen es so.
            for (const MapEntity& z : g_app->map.entities) {
                const std::string* tn = z.find("targetname");
                if (tn == nullptr || *tn != *t) {
                    continue;
                }
                float p2[3] = {0, 0, 0};
                (void)std::sscanf(z.origin.c_str(), "%f %f %f", &p2[0],
                                  &p2[1], &p2[2]);
                float d[3] = {p2[0] - o[0], p2[1] - o[1], p2[2] - o[2]};
                const float l = std::sqrt(d[0] * d[0] + d[1] * d[1] +
                                          d[2] * d[2]);
                if (l > 0.0001F) {
                    for (int k = 0; k < 3; ++k) {
                        inst.normal[k] = d[k] / l;
                    }
                }
                break;
            }
        } else if (const std::string* a = e.find("angle")) {
            const float gier = static_cast<float>(std::atof(a->c_str())) *
                               3.14159265F / 180.0F;
            inst.normal[0] = std::cos(gier);
            inst.normal[1] = std::sin(gier);
            inst.normal[2] = 0.0F;
        }
        // Die Regeln stehen in code/game/g_fx.cpp, und ich hatte sie mir
        // vorher AUSGEDACHT. Was wirklich gilt:
        //
        //   SP_fx_runner():
        //     G_SpawnInt( "delay", "200", &ent->delay );
        //     G_SpawnFloat( "random", "0", &ent->random );
        //
        //   fx_runner_think(), Zeile 70:
        //     ent->nextthink = level.time + ent->delay
        //                      + Q_flrand(0,1) * ent->random;
        //
        //   fx_runner_link():
        //     if ( spawnflags & 1 || spawnflags & 2 )  // STARTOFF || ONESHOT
        //         ent->nextthink = -1;                 // wartet auf use
        //     else
        //         ent->nextthink = level.time + 200;   // laeuft sofort los
        //
        // Also: delay ist der WIEDERHOLABSTAND in Millisekunden, Vorgabe
        // 200 - keine einmalige Verzoegerung. random streut ihn. Ein "wait"
        // gibt es gar nicht; das hatte ich erfunden. Und ein Runner mit
        // STARTOFF oder ONESHOT laeuft erst, wenn ihn etwas benutzt.
        constexpr int kStartOff = 1;
        constexpr int kOneShot = 2;
        int flags = 0;
        if (const std::string* sf = e.find("spawnflags")) {
            flags = std::atoi(sf->c_str());
        }
        const bool wartet = (flags & (kStartOff | kOneShot)) != 0;
        inst.loops = !wartet;
        inst.intervalMs = 200.0F;
        if (const std::string* d = e.find("delay")) {
            const float v = static_cast<float>(std::atof(d->c_str()));
            if (v > 0.0F) {
                inst.intervalMs = v;
            }
        }
        // random streut den Abstand. Fuer eine wiederholbare Vorschau die
        // MITTE nehmen - dieselbe Wahl wie bei $random$ in der Zeitleiste.
        if (const std::string* rnd = e.find("random")) {
            inst.intervalMs += static_cast<float>(std::atof(rnd->c_str())) * 0.5F;
        }
        // Der erste Durchgang kommt 600 ms nach dem Start: SP_fx_runner
        // setzt `nextthink = level.time + 400` (fx_runner_link, "Give us a
        // bit of time to spawn in the other entities"), fx_runner_link dann
        // `nextthink = level.time + 200` (g_fx.cpp, MD wie OpenJK). Bis rc601
        // stand hier nur die 200 aus fx_runner_link.
        // Wer auf ein use wartet, faengt hier nicht an - die Starts rechnet
        // die Mover-Simulation (fxStarts, unten).
        inst.startMs = wartet ? -1.0 : 600.0;
        // Der Ausgangswert haengt am Ort: zwei Runner nebeneinander sollen
        // nicht Partikel fuer Partikel gleich aussehen.
        inst.seed = static_cast<unsigned>(
            std::abs(static_cast<int>(o[0]) * 73856093 ^
                     static_cast<int>(o[1]) * 19349663 ^
                     static_cast<int>(o[2]) * 83492791));
        if (inst.startMs < 0.0) {
            // STARTOFF oder ONESHOT: laeuft erst auf ein use. Wann das
            // kommt, rechnet die Mover-Simulation aus der Zeitleiste
            // (fx_runner_use, g_fx.cpp): ONESHOT einmal, START_OFF ab dem
            // use und bis zum naechsten.
            const auto ei = static_cast<std::size_t>(&e - g_app->map.entities.data());
            bool gestartet = false;
            for (const FxRunnerStart& f : g_app->moverSim.fxStarts()) {
                if (f.entity != ei) {
                    continue;
                }
                EffectInstance an = inst;
                an.startMs = f.ms;
                an.loops = !f.oneShot;
                an.endMs = f.bisMs;
                g_app->effectRunners.push_back(an);
                gestartet = true;
            }
            if (gestartet) {
                ++gefunden;
            } else {
                ++wartend;
            }
            continue;
        }
        g_app->effectRunners.push_back(inst);
        ++gefunden;
    }
    fallenEffekteSetzen();   // rc604 fix604b (B5) / rc604: misc_trip_mine
    geschuetzeTakten();      // rc604: misc_sentry_turret, misc_ion_cannon
    // --- rc604 fix604b (B6): fx_target_beam ----------------------------------
    //
    // SP_fx_target_beam (code/game/g_fx.cpp:1195-1225 OpenJK): fx_target_beam_link
    // nach START_TIME_LINK_ENTS (FRAMETIME), ohne STARTOFF (1) und ONE_SHOT (8) ein
    // FRAMETIME spaeter fx_target_beam_fire_start; danach feuert think, sobald
    // attackDebounceTime vorbei ist: wait (Sekunden * 1000) ab FRAMETIME, sonst
    // FRAMETIME (+- random, hier die Mitte). fx_target_beam_fire: Punktspur
    // MASK_SHOT vom Ursprung zum target (G_Find), Ende = trace.endpos (OPEN, 2:
    // das target), pos1 = Flaechennormale (OPEN: -dir); EV_TARGET_BEAM_DRAW ->
    // CG_DrawTargetBeam (code/cgame/cg_effects.cpp:1002-1046): eine Kopie von
    // fxFile ("env/targ_beam") mit "beam"/"glow" origin2 = Ende an start,
    // Richtung start - Ende, dazu fxFile2 ("env/targ_beam_impact", NO_IMPACT 16:
    // keins) am Ende mit pos1. Spur nur gegen die Welt. rc604: auch per use
    // (STARTOFF/ONE_SHOT) und per use aus - MoverSim::strahlStarts.
    {
        // rc604: jeder Schuss einzeln wie das Spiel (zielStrahlFeuer: link, use,
        // think im FRAMETIME-Takt mit der Abklingzeit), das Ende je Schuss neu:
        // fx_target_beam_fire nimmt enemy->currentOrigin (das target, wo es
        // gerade ist - ein func_train faehrt, die Mover-Simulation weiss wo),
        // bei info_null s.origin2 (das Ende des letzten Schusses), und spurt
        // MASK_SHOT ohne Ghoul2 (EG2_Collision 0) gegen Welt, Brush-Modelle und
        // Figurenkaesten (laserfalleSpurBei). Schaden: G_Damage auf das Opfer
        // der Spur wird nicht nachgebildet (die Figur spielt weiter wie im
        // Skript; vjun2 schreibt "dmg", das SP_fx_target_beam mit
        // G_SpawnInt("damage", "0") ohnehin auf 0 setzt). Schuesse mit
        // demselben Ende im gleichen Abstand werden eine Wiederholung.
        int strahlen = 0;
        for (std::size_t i = 0; g_fallenWelt.k && i < g_app->map.entities.size(); ++i) {
            const MapEntity& e = g_app->map.entities[i];
            if (e.classname != "fx_target_beam" || !kartenpruefung::imSpiel(e.classname, regelwerkMovieDuels()) ||
                !gespawntImSpiel(e)) {
                continue;
            }
            int sf = 0;
            if (const std::string* s = e.find("spawnflags")) { sf = std::atoi(s->c_str()); }
            const std::string* ziel = e.find("target");
            const MapEntity* zielEnt = nullptr;
            for (const MapEntity& z : g_app->map.entities) {
                if (ziel != nullptr && !ziel->empty() && z.targetname == *ziel && gespawntImSpiel(z)) {
                    zielEnt = &z;
                    break;
                }
            }
            if (zielEnt == nullptr || e.origin.empty()) { continue; }
            ZielStrahlEingabe ein;
            ein.startOff = (sf & 1) != 0;
            ein.oneShot = (sf & 8) != 0;
            if (const std::string* w = e.find("wait")) { ein.waitMs = static_cast<float>(std::atof(w->c_str())) * 1000.0F; }
            if (const std::string* r = e.find("random")) { ein.randomMs = static_cast<float>(std::atof(r->c_str())) * 1000.0F; }
            ein.spawnMs = kNavKartenSpawnMs;
            ein.endeMs = std::max(g_app->camTrack.durationMs, g_app->timeline.practicalEndMs) + 5000.0;
            ein.saat = static_cast<std::uint64_t>(i) * 0xd6e8feb86659fd93ULL + 0x2545f4914f6cdd1dULL;
            for (const auto& [ent, ms] : g_app->moverSim.entityUses()) {
                if (ent == i) { ein.uses.push_back(ms); }
            }
            std::stable_sort(ein.uses.begin(), ein.uses.end());
            const std::vector<double> feuer = zielStrahlFeuer(ein);
            if (feuer.empty()) { continue; }
            float start[3] = {0.0F, 0.0F, 0.0F};
            float origin2[3] = {0.0F, 0.0F, 0.0F};
            (void)std::sscanf(e.origin.c_str(), "%f %f %f", &start[0], &start[1], &start[2]);
            if (!zielEnt->origin.empty()) {
                (void)std::sscanf(zielEnt->origin.c_str(), "%f %f %f", &origin2[0], &origin2[1], &origin2[2]);
            }
            // G_SetEnemy ausser bei info_null; enemy->currentOrigin zur Zeit t.
            const bool feind = zielEnt->classname != "info_null";
            int zielModell = 0;
            if (const std::string* m = zielEnt->find("model"); m != nullptr && m->size() > 1 && (*m)[0] == '*') {
                zielModell = std::atoi(m->c_str() + 1);
            }
            const Actor* zielSkript = nullptr;
            for (const Actor& a : g_app->scene.actors) {
                if (zielModell > 0 && a.brushModel == zielModell && a.haveStart) { zielSkript = &a; }
            }
            const auto zielOrt = [&](double t, float o[3]) {
                if (!feind) {
                    for (int q = 0; q < 3; ++q) { o[q] = origin2[q]; }
                    return;
                }
                (void)std::sscanf(zielEnt->origin.c_str(), "%f %f %f", &o[0], &o[1], &o[2]);
                if (zielSkript != nullptr) {
                    const ActorState st = zielSkript->at(t);
                    for (int q = 0; q < 3; ++q) { o[q] = st.pos[q]; }
                } else if (zielModell > 0) {
                    const MoverSim::Stand m = g_app->moverSim.at(zielModell, t);
                    if (m.bekannt) {
                        for (int q = 0; q < 3; ++q) { o[q] = m.origin[q]; }
                    }
                }
            };
            struct Schuss {
                double t;
                float ende[3];
                float norm[3];
            };
            std::vector<Schuss> schuesse;
            schuesse.reserve(feuer.size());
            const bool offen = (sf & 2) != 0;
            const float null3[3] = {0.0F, 0.0F, 0.0F};
            for (const double t : feuer) {
                float org[3];
                zielOrt(t, org);
                float dir[3];
                float laenge = 0.0F;
                for (int q = 0; q < 3; ++q) {
                    dir[q] = org[q] - start[q];
                    laenge += dir[q] * dir[q];
                }
                laenge = std::sqrt(laenge);
                for (float& d : dir) { d = laenge > 0.0F ? d / laenge : 0.0F; }
                const LaserfalleSpur sp = laserfalleSpurBei(t, start, null3, null3, org, static_cast<std::size_t>(-1), false);
                Schuss x;
                x.t = t;
                for (int q = 0; q < 3; ++q) {
                    x.ende[q] = offen ? org[q] : sp.endpos[q];
                    x.norm[q] = offen ? -dir[q] : sp.normale[q];
                    origin2[q] = x.ende[q];   // VectorCopy(end, ent->s.origin2)
                }
                schuesse.push_back(x);
            }
            std::string fx = "env/targ_beam";
            if (const std::string* f = e.find("fxFile"); f != nullptr && !f->empty()) { fx = *f; }
            std::string fx2 = "env/targ_beam_impact";
            if (const std::string* f2 = e.find("fxFile2"); f2 != nullptr && !f2->empty()) { fx2 = *f2; }
            const std::string pfad = "effects/" + fx + (fx.find(".efx") == std::string::npos ? ".efx" : "");
            const std::string pfad2 = "effects/" + fx2 + (fx2.find(".efx") == std::string::npos ? ".efx" : "");
            const bool strahlFx = ladeEffektDatei(pfad, fx);
            const bool einschlagFx = (sf & 16) == 0 && ladeEffektDatei(pfad2, fx2);
            // Stuecke: gleiches Ende und gleicher Abstand -> eine Wiederholung.
            std::size_t a0 = 0;
            int kopien = 0;
            while (a0 < schuesse.size()) {
                std::size_t a1 = a0 + 1;
                const auto gleich = [&](const Schuss& p, const Schuss& q) {
                    for (int k = 0; k < 3; ++k) {
                        if (p.ende[k] != q.ende[k] || p.norm[k] != q.norm[k]) { return false; }
                    }
                    return true;
                };
                const double abstand = a1 < schuesse.size() ? schuesse[a1].t - schuesse[a0].t : 0.0;
                while (a1 < schuesse.size() && gleich(schuesse[a1], schuesse[a0]) &&
                       std::fabs((schuesse[a1].t - schuesse[a1 - 1].t) - abstand) < 0.01) {
                    ++a1;
                }
                const Schuss& s0 = schuesse[a0];
                EffectInstance inst;
                inst.startMs = s0.t;
                inst.loops = a1 - a0 > 1U;
                inst.intervalMs = static_cast<float>(abstand);
                inst.endMs = inst.loops ? schuesse[a1 - 1].t + 0.5 : -1.0;
                if (strahlFx) {
                    const std::string kopie = pfad + "#tb" + std::to_string(i) + (kopien == 0 ? "" : "_" + std::to_string(kopien));
                    efx::Effect kopf = g_app->effects[pfad];
                    for (efx::Primitive& pr : kopf.primitives) {
                        if (pr.name == "beam" || pr.name == "glow") {
                            pr.origin2.min = efx::Vec3{s0.ende[0], s0.ende[1], s0.ende[2]};
                            pr.origin2.max = pr.origin2.min;
                            pr.origin2.set = true;
                            pr.origin2.ranged = false;
                        }
                    }
                    g_app->effects[kopie] = std::move(kopf);
                    EffectInstance b = inst;
                    b.effect = &g_app->effects[kopie];
                    float rueck[3];
                    float rl = 0.0F;
                    for (int q = 0; q < 3; ++q) {
                        rueck[q] = start[q] - s0.ende[q];
                        rl += rueck[q] * rueck[q];
                    }
                    rl = std::sqrt(rl);
                    for (int q = 0; q < 3; ++q) {
                        b.origin[q] = start[q];
                        b.normal[q] = rl > 0.0F ? rueck[q] / rl : 0.0F;
                    }
                    b.seed = static_cast<unsigned>(i * 2246822519U + 31U) | 1U;
                    g_app->effectRunners.push_back(b);
                }
                if (einschlagFx) {
                    EffectInstance x = inst;
                    x.effect = &g_app->effects[pfad2];
                    for (int q = 0; q < 3; ++q) {
                        x.origin[q] = s0.ende[q];
                        x.normal[q] = s0.norm[q];
                    }
                    x.seed = static_cast<unsigned>(i * 3266489917U + 7U) | 1U;
                    g_app->effectRunners.push_back(x);
                }
                ++kopien;
                a0 = a1;
            }
            ++strahlen;
            if (kopien > 1) {
                diag::detail("fx_target_beam #" + std::to_string(i) + ": " + std::to_string(feuer.size()) + " Schuesse, " +
                             std::to_string(kopien) + " Stuecke mit eigenem Ende");
            }
        }
        if (strahlen != 0) { diag::info(std::to_string(strahlen) + " Strahlen von fx_target_beam"); }
    }
    // --- Was beim Zerbrechen spielt (CG_MiscModelExplosion) --------------
    //
    // func_breakable und misc_model_breakable, die ein use zerbricht: je
    // Material "chunks/metalexplode", "chunks/glassbreak" ..., mehrfach im
    // Kasten verteilt und von der Mitte weg gerichtet - die Engine spielt
    // sie mit theFxScheduler.PlayEffect(name, org, dir). Wann und wo,
    // rechnet MoverSim (bruchEffekte); hier nur Datei und Instanz.
    //
    // Sie stehen in effectRunners wie jeder Effekt - dann zeichnen,
    // beleuchten und klingen sie auf demselben Weg. Gezaehlt werden sie
    // eigens (bruchFxZahl), damit der Knopftest die per use gestarteten
    // Effekte weiterhin abziehen kann.
    g_app->bruchFxZahl = 0;
    for (const BruchEffekt& bf : g_app->moverSim.bruchEffekte()) {
        const std::string path = "effects/" + bf.effekt + ".efx";
        if (!ladeEffektDatei(path, bf.effekt)) {
            continue;
        }
        EffectInstance inst;
        inst.effect = &g_app->effects[path];
        for (int k = 0; k < 3; ++k) {
            inst.origin[k] = bf.ort[k];
            inst.normal[k] = bf.richtung[k];
        }
        inst.startMs = bf.ms;
        inst.loops = false;
        inst.seed = static_cast<unsigned>(g_app->bruchFxZahl * 2654435761U + bf.entity * 40503U) | 1U;
        g_app->effectRunners.push_back(inst);
        ++g_app->bruchFxZahl;
    }
    if (g_app->bruchFxZahl != 0U) {
        diag::info(std::to_string(g_app->bruchFxZahl) + " Brucheffekte (func_breakable, misc_model_breakable)");
    }
    // --- Was die Notizen einer ROFF-Bahn starten (rc592) ------------------
    //
    // "effect <name> [versatz [winkel]]" in einer .rof: G_PlayEffect am Ort
    // der Entity in dem Bild, das die Notiz traegt (G_RoffNotetrackCallback).
    // Wann und wo rechnet die Szene; hier nur Datei und Instanz.
    g_app->roffFxZahl = 0;
    for (const RoffEffektStart& rf : roffEffektStarts(g_app->scene)) {
        if (!ladeEffektDatei(rf.pfad, rf.name)) {
            continue;
        }
        EffectInstance inst;
        inst.effect = &g_app->effects[rf.pfad];
        for (int k = 0; k < 3; ++k) {
            inst.origin[k] = rf.ort[k];
            inst.normal[k] = rf.richtung[k];
        }
        inst.startMs = rf.ms;
        inst.loops = false;
        inst.seed = static_cast<unsigned>(g_app->roffFxZahl * 2246822519U + 977U) | 1U;
        g_app->effectRunners.push_back(inst);
        ++g_app->roffFxZahl;
    }
    if (g_app->roffFxZahl != 0U) {
        diag::info(std::to_string(g_app->roffFxZahl) + " Effekte aus ROFF-Notizen");
    }
    diag::info(std::to_string(gefunden) + " fx_runner laufen, " +
               std::to_string(wartend) + " warten auf ein use (STARTOFF/ONESHOT), " +
               std::to_string(fehlt) + " ohne Datei, " +
               std::to_string(g_app->effects.size()) + " Effektdateien gelesen");

    // --- Die Folgeeffekte gleich mitladen, samt Shadern -----------------
    //
    // impactfx, deathfx, emitfx und playfx nennen weitere Effekte. Bis rc568
    // wurden sie erst beim ersten Gebrauch geholt (effectByName) - mitten im
    // Bild, und ohne ihre Shader einzutragen. Hier werden sie Stufe fuer
    // Stufe vorab gelesen, bis nichts Neues mehr dazukommt. Damit stehen
    // auch ihre Emittermodelle im naechsten Schritt bereit (die Schleife
    // dort geht ueber g_app->effects).
    {
        std::vector<const efx::Effect*> offen;
        for (const auto& [pfad, ef] : g_app->effects) {
            (void)pfad;
            offen.push_back(&ef);
        }
        int nachgeladen = 0;
        int ohneDatei = 0;
        ladeFolgeeffekte(std::move(offen), &nachgeladen, &ohneDatei);
        if (nachgeladen != 0 || ohneDatei != 0) {
            diag::info(std::to_string(nachgeladen) +
                       " Folgeeffekte vorab geladen, " +
                       std::to_string(ohneDatei) + " fehlen");
        }
    }

    // --- Die Modelle, die an Emittern haengen, gleich mitladen -----------
    //
    // "Emitters don't draw themselves, but they may need to add an
    // attached model" (FxPrimitives.cpp:1375). Den Rauch gibt es seit
    // rc323, hier kommt der Koerper dazu.
    //
    // HIER und nicht beim ersten Gebrauch: eine .md3 mitten in einem Bild
    // zu lesen kostet zwanzig bis fuenfzig Millisekunden, und das haben wir
    // in rc307 bis rc311 dreimal durchgemacht.
    {
        g_app->emitterModels.clear();
        int neu = 0;
        int fehlt2 = 0;
        for (const auto& [pfad, ef] : g_app->effects) {
            (void)pfad;
            for (const efx::Primitive& pr : ef.primitives) {
                if ((pr.flags & efx::kFlagAttachedModel) == 0U ||
                    pr.models.empty()) {
                    continue;
                }
                for (const std::string& roh : pr.models) {
                    std::string low = roh;
                    for (char& c : low) {
                        c = static_cast<char>(
                            std::tolower(static_cast<unsigned char>(c)));
                    }
                    if (g_app->emitterModels.count(low) != 0U) {
                        continue;   // schon da
                    }
                    std::string data;
                    if (!readFromArchives(low, data)) {
                        diag::detail("Emittermodell \"" + low +
                                     "\" nicht gefunden");
                        ++fehlt2;
                        continue;
                    }
                    Md3Model mm;
                    std::string err;
                    if (!readMd3(data, mm, &err)) {
                        diag::detail("Emittermodell \"" + low +
                                     "\" nicht lesbar: " + err);
                        ++fehlt2;
                        continue;
                    }
                    App::EmitterModel em;
                    em.path = low;
                    // Die Texturen der Flaechen hinten anhaengen - derselbe
                    // Weg wie bei den Kartenmodellen. rc605: als Modellblock
                    // (modellBlock) - emitterModels wird bei jedem
                    // loadMapEffects geleert, und jedes prepareScene haengte
                    // die Bilder neu an (seit die Effekte der Laserfallen und
                    // Geschuetze immer geladen werden: 4-6 Bilder je Reiter,
                    // Modus tuerlauf "ein zweites prepareScene legt keine
                    // Bilder an").
                    em.shaderBase = modellBlock("e|" + low, [&mm] {
                        for (const Md3Surface& sf : mm.surfaces) {
                            TextureSet::Tex t;
                            loadTextureFor(sf.shader, t);
                            g_app->textures.byShader.push_back(std::move(t));
                        }
                    });
                    g_app->md3Files[low] = std::move(mm);
                    g_app->emitterModels[low] = std::move(em);
                    ++neu;
                }
            }
        }
        if (neu != 0 || fehlt2 != 0) {
            diag::info(std::to_string(neu) + " Emittermodelle geladen, " +
                       std::to_string(fehlt2) + " fehlen");
        }
    }

    // --- Was in diesen Effekten steckt, und was davon behaved kann -------
    //
    // Gebeten: "baue bei allem Debug ein, dass du sehen kannst, wenn was
    // nicht stimmt."
    //
    // Genau hier war es noetig. Bis rc271 stand im Protokoll nur, WIE VIELE
    // Effekte laufen - nicht, was in ihnen steht. Dass 156 Klangprimitive
    // ueber alle Dateien nie gespielt wurden, ist mir deshalb erst beim
    // Auszaehlen von aussen aufgefallen, nicht beim Lesen des Protokolls.
    //
    // Die Zeile nennt jetzt je Primitivtyp die Anzahl und ob behaved damit
    // etwas anfaengt. Wer ein Protokoll liest, sieht damit sofort, ob ein
    // fehlender Effekt an einer nicht gefundenen Datei liegt oder an einem
    // Typ, den es noch nicht gibt.
    {
        std::map<efx::PrimitiveType, int> zahl;
        for (const auto& [pfad, ef] : g_app->effects) {
            (void)pfad;
            for (const efx::Primitive& pr : ef.primitives) {
                ++zahl[pr.type];
            }
        }
        auto kann = [](efx::PrimitiveType t) {
            switch (t) {
                case efx::PrimitiveType::Particle:
                case efx::PrimitiveType::OrientedParticle:
                case efx::PrimitiveType::Tail:
                case efx::PrimitiveType::Line:
                case efx::PrimitiveType::Electricity:
                case efx::PrimitiveType::Cylinder:
                    return "gezeichnet";
                case efx::PrimitiveType::FxRunner:
                    // Seit rc333 - er zeichnet nichts, er startet einen
                    // anderen Effekt. "gezeichnet" waere hier irrefuehrend.
                    return "startet seinen playfx-Effekt";
                case efx::PrimitiveType::Decal:
                    // Seit rc332 - als flaches Viereck auf der Flaeche,
                    // ohne das Umschlagen um Kanten (CM_MarkFragments).
                    return "gezeichnet (flach, ohne Kantenumschlag)";
                case efx::PrimitiveType::Sound:
                    return "gespielt";
                case efx::PrimitiveType::Light:
                    return "beleuchtet";
                case efx::PrimitiveType::CameraShake:
                    // effectShakeAt, laengst gebaut - stand trotzdem hier
                    // als "NOCH NICHT".
                    return "wackelt die Kamera";
                case efx::PrimitiveType::Emitter:
                    // Seit rc569 je Teilchen: Modell (useModel) mit
                    // Groessenkurve und Drehung, Fahne (emitFx), Aufprall
                    // und Tod.
                    return "Modell, Fahne und Folgeeffekte";
                case efx::PrimitiveType::ScreenFlash:
                    // Seit rc569: Sprite vor dem Auge wie CFlash::Draw.
                    return "Vollbildblitz";
                default:
                    return "NOCH NICHT";
            }
        };
        // NUR bei Aenderung. Im Protokoll standen dieselben drei Zeilen
        // dreimal hintereinander - einmal je Aufbau der Ansicht. Wer ein
        // Protokoll liest, haelt Wiederholungen fuer einen Hinweis und
        // sucht nach einem Unterschied, den es nicht gibt.
        std::string uebersicht;
        for (const auto& [t, c] : zahl) {
            uebersicht += std::string("   ") + efx::typeName(t) + ": " +
                          std::to_string(c) + " - " + kann(t) + "\n";
        }
        if (uebersicht != g_app->lastEffectSummary) {
            g_app->lastEffectSummary = uebersicht;
            std::size_t at = 0;
            while (at < uebersicht.size()) {
                const std::size_t nl = uebersicht.find('\n', at);
                diag::detail(uebersicht.substr(at, nl - at));
                at = nl + 1;
            }
        }
    }

    // --- Und was von den FLAGS beachtet wird -----------------------------
    //
    // --- Diese Meldung hat GELOGEN, und das ist schlimmer als keine ------
    //
    // Hier stand bis rc357: "der Zeichner beachtet derzeit kein einziges".
    // Das war zum Zeitpunkt des Schreibens richtig und ist seit mehreren
    // Runden falsch. Im Protokoll des Nutzers stand deshalb
    //
    //     Flags, die behaved NICHT auswertet (der Zeichner beachtet
    //     derzeit kein einziges):
    //        6x impactFx - kein Aufpralleffekt
    //        1x useAlpha - Alpha nicht als Helligkeit
    //        2x usePhysics - keine Schwerkraft
    //
    // waehrend alle drei laengst gebaut sind (efxdraw.cpp:378, :455, :765).
    // Eine Meldung, die das Gegenteil der Wahrheit sagt, schickt jeden -
    // mich eingeschlossen - hinter dem falschen Fehler her.
    //
    // Nachgezaehlt: von den zwoelf Flags dieser Liste sind ACHT gebaut.
    //
    // Und der Text war auch sachlich falsch. `usePhysics` ist NICHT die
    // Schwerkraft: FX_APPLY_PHYSICS schaltet in UpdateOrigin
    // (FxPrimitives.cpp:266) den Spurtest und das Abprallen ein. Die
    // Schwerkraft kommt aus einem EIGENEN Feld ueber mAccel und wirkt
    // unabhaengig davon. Genau diese Verwechslung steht schon in der
    // Uebergabe als Falle - "Zahlen gegen die Datei halten, nicht 'es
    // bewegt sich' pruefen".
    //
    // Deshalb wird jetzt beides gemeldet, getrennt: was GEBAUT ist und
    // was FEHLT. Wer nur eine Haelfte meldet, meldet frueher oder spaeter
    // wieder das Falsche.
    //
    // Der Abgleich, Stand jetzt:
    //
    //   TYPEN: die Engine kennt dreizehn (FxScheduler.h:150 ff.). behaved
    //   liest alle dreizehn und setzt seit rc569 alle dreizehn um - siehe
    //   die Uebersicht darueber. (ScreenFlash fehlt in den 391 Dateien aus
    //   assets1, steht aber 19 mal in den 649 Dateien von base und MD.)
    //
    //   FLAGS: gezaehlt ueber die 649 Dateien von base und MD -
    //
    //     gebaut    usePhysics 464x, useModel 144x, useBBox 121x, useAlpha,
    //               impactFx, emitFx, deathFx, depthHack, setShaderTime,
    //               killOnImpact
    //     ohne      expensivePhysics 115x (wir spuren immer),
    //     Wirkung   relative 0x (nur MP-Parser)
    //
    {
        // Drei Staende, nicht zwei.
        //
        // `expensivePhysics` hat mich das gelehrt: es ist weder gebaut noch
        // fehlend. Die Engine erzwingt damit den Spurtest, den sie sonst
        // erst nach einer billigen Vorpruefung macht - und behaved spurt
        // ohnehin immer. Das Flag hat hier schlicht nichts mehr zu tun.
        //
        // Es als "gebaut" einzutragen waere gelogen (der Zeichner fasst es
        // nie an), als "fehlend" auch (es fehlt nichts). Der dritte Stand
        // ist die einzige wahre Auskunft - und der Pruefer
        // tools/lint_efxflags.py kennt ihn.
        enum class Stand { Gebaut, Fehlt, OhneWirkung };
        struct Merkmal {
            std::uint32_t bit;
            const char* name;
            const char* folge;
            Stand stand;
        };
        static constexpr Merkmal merkmale[] = {
            // --- gebaut ---------------------------------------------------
            {efx::kFlagDepthHack, "depthHack",
             "zeichnet vor allem anderen", Stand::Gebaut},
            {efx::kFlagApplyPhysics, "usePhysics",
             "Spurtest und Abprallen", Stand::Gebaut},
            {efx::kFlagEmitFx, "emitFx", "erzeugt Folgeeffekte", Stand::Gebaut},
            {efx::kFlagKillOnImpact, "killOnImpact",
             "endet an der Wand", Stand::Gebaut},
            {efx::kFlagUseAlpha, "useAlpha",
             "Blende in den Alphakanal statt in die Farbe", Stand::Gebaut},
            {efx::kFlagImpactRunsFx, "impactFx",
             "Aufpralleffekt", Stand::Gebaut},
            {efx::kFlagDeathRunsFx, "deathFx", "Endeffekt", Stand::Gebaut},
            {efx::kFlagAttachedModel, "attachedModel", "Modell", Stand::Gebaut},
            // --- ohne Wirkung -------------------------------------------
            // `relative` liest nur der MP-Parser. Im Einzelspieler entsteht
            // FX_RELATIVE allein zur Laufzeit, wenn ein Effekt an einem Bolt
            // gespielt wird (FxScheduler.cpp:1542) - dafuer gibt es seit
            // rc569 EffectInstance::anker/relativ. Das Wort in der Datei hat
            // fuer Movie Duels keine Wirkung, weder im Spiel noch hier.
            {efx::kFlagRelative, "relative",
             "ohne Wirkung - der SP-Parser ueberliest es", Stand::OhneWirkung},
            // `expensivePhysics` ERZWINGT in der Engine den Spurtest;
            // ohne das Flag prueft sie erst billig mit CG_PointContents und
            // spurt nur, wenn dort etwas Festes steht
            // (FxPrimitives.cpp:269). behaved spurt bei eingeschalteter
            // Physik IMMER - das ist strenger, nicht laxer. Das Flag hat
            // hier also nichts mehr zu tun, und "fehlt" waere die falsche
            // Auskunft.
            //
            // Es steht trotzdem in der Liste, damit die Zahl im Protokoll
            // erscheint: 60 Vorkommen in 35 Dateien.
            {efx::kFlagExpensivePhysics, "expensivePhysics",
             "ohne Wirkung - wir spuren ohnehin immer", Stand::OhneWirkung},
            {efx::kFlagSetShaderTime, "setShaderTime",
             "Zeitursprung auf den Beginn des Effekts", Stand::Gebaut},
            // Seit rc569: Strahl mit Kastenvorlauf (efxdraw.cpp, spurFuer) -
            // das Teilchen bleibt um seine Ausdehnung vor der Flaeche
            // stehen. Eine Naeherung, kein echter Kastentest.
            {efx::kFlagUseBBox, "useBBox",
             "Spurtest mit Kastenvorlauf (Naeherung)", Stand::Gebaut},
        };
        // Getrennt zaehlen. Beides zu melden ist der Punkt: eine Liste
        // allein sagt nie, ob sie die gebauten oder die fehlenden meint.
        std::map<std::string, int> gebaut;
        std::map<std::string, int> fehltZahl;
        std::map<std::string, int> ohne;
        for (const auto& [pfad, ef] : g_app->effects) {
            (void)pfad;
            for (const efx::Primitive& pr : ef.primitives) {
                for (const Merkmal& m : merkmale) {
                    if ((pr.flags & m.bit) != 0U) {
                        auto& wohin = (m.stand == Stand::Gebaut) ? gebaut
                                      : (m.stand == Stand::Fehlt) ? fehltZahl
                                                                  : ohne;
                        ++wohin[std::string(m.name) + " - " + m.folge];
                    }
                }
            }
        }
        // --- Und was von den TYPEN noch fehlt --------------------------
        //
        // Gebeten: "vergiss nicht, bei der Sache Debug einzubauen, damit du
        // siehst. Es soll alles ins Log schreiben."
        //
        // Die Typenuebersicht darueber sagt, WAS vorkommt. Diese Zeile sagt
        // zusaetzlich, was davon in DIESER Mission noch fehlt - und bei den
        // Decals, welche Einschraenkung gilt.
        {
            int decals = 0;
            int fxrunner = 0;
            for (const auto& [pf, ef] : g_app->effects) {
                (void)pf;
                for (const efx::Primitive& pr : ef.primitives) {
                    if (pr.type == efx::PrimitiveType::Decal) {
                        ++decals;
                    } else if (pr.type == efx::PrimitiveType::FxRunner) {
                        ++fxrunner;
                    }
                }
            }
            if (decals != 0) {
                diag::info(std::to_string(decals) +
                           " Decals: flach auf die Flaeche gelegt. Die "
                           "Engine schneidet sie zusaetzlich gegen die "
                           "Geometrie (CM_MarkFragments) - ueber einer Kante "
                           "steht unseres durch die Wand.");
            }
            if (fxrunner != 0) {
                diag::info(std::to_string(fxrunner) +
                           " FxRunner: spielen ihren playfx-Effekt am "
                           "eigenen Ort (seit rc333).");
            }
        }

        if (!gebaut.empty()) {
            diag::info("Flags, die behaved AUSWERTET:");
            for (const auto& [was, n] : gebaut) {
                diag::info("   " + std::to_string(n) + "x " + was);
            }
        }
        if (!ohne.empty()) {
            diag::info("Flags ohne Wirkung fuer eine Vorschau:");
            for (const auto& [was, n] : ohne) {
                diag::info("   " + std::to_string(n) + "x " + was);
            }
        }
        if (fehltZahl.empty()) {
            diag::info("Flags: kein unbeachtetes kommt in dieser Mission vor");
        } else {
            diag::info("Flags, die behaved NICHT auswertet:");
            for (const auto& [was, n] : fehltZahl) {
                diag::info("   " + std::to_string(n) + "x " + was);
            }
        }
    }

}

// Die .md3-Modelle der Karte einlesen.
//
// misc_model_static und misc_model_breakable verweisen ueber "model" auf
// eine Datei. In den Karten der Mod sind das 1549 Entities - Konsolen,
// Kisten, Rohre, Raumschiffe. Ohne sie fehlt der halbe Raum.
//
// Die Texturen kommen ueber den Shadernamen der Flaeche, genau wie bei der
// Karte selbst. Sie werden HINTEN an textures.byShader angehaengt; die
// Nummern der Karte bleiben damit unberuehrt.

// --- rc602 kartenpruefung: Gegenstaende der Karte ---------------------------
//
// Gefunden mit der Kartenpruefung: auf 87 von 278 Karten lagen Medipacks,
// Schilde, Waffen und Munition, die das Spiel zeigt - behaved zeichnete sie
// nicht. Nachgebildet: G_CallSpawn (itemlist aus ext_data/items.dat),
// G_SpawnGEntityFromSpawnVars (notsingle, Schwierigkeit), G_SpawnItem
// (targetname: erst per use) und FinishSpawningItem (Fallen mit den mins/maxs
// des Gegenstands, im Festen entfernt) - bhed/gegenstaende.h. Gezeichnet wie
// CG_Item (cg_ents.cpp:1108 MD): .md3 mit ent.axis = AnglesToAxis(spinAngles)
// (ITMSF_VERTICAL kippt), .glm ueber Ghoul2 mit G2_GenerateWorldMatrix(
// lerpAngles) (tr_ghoul2.cpp:2829, G2_misc.cpp:1724) - dort wirkt das Kippen
// nicht. Nicht nachgebildet: das Leuchten eines liegenden Lichtschwerts
// (CG_Item "Dropped saber glow effect") und weapon_saber mit NPC_type.
namespace {

// AnglesToAxis (q_math.c): axis[0] = forward, axis[1] = -right, axis[2] = up.
void winkelZuAchsen(const float w[3], float achse[3][3]) {
    constexpr float kGrad = 3.14159265358979323846F / 180.0F;
    const float sy = std::sin(w[1] * kGrad);
    const float cy = std::cos(w[1] * kGrad);
    const float sp = std::sin(w[0] * kGrad);
    const float cp = std::cos(w[0] * kGrad);
    const float sr = std::sin(w[2] * kGrad);
    const float cr = std::cos(w[2] * kGrad);
    achse[0][0] = cp * cy;
    achse[0][1] = cp * sy;
    achse[0][2] = -sp;
    const float rechts[3] = {-1.0F * sr * sp * cy + -1.0F * cr * -sy, -1.0F * sr * sp * sy + -1.0F * cr * cy,
                             -1.0F * sr * cp};
    for (int k = 0; k < 3; ++k) { achse[1][k] = -rechts[k]; }
    achse[2][0] = cr * sp * cy + -sr * -sy;
    achse[2][1] = cr * sp * sy + -sr * cy;
    achse[2][2] = cr * cp;
}

// Ghoul2-Modell eines Gegenstands, einmal je Pfad (auch der Fehlschlag).
const App::ActorAssets* gegenstandGlmFuer(const std::string& pfad) {
    const auto have = g_app->gegenstandGlmCache.find(pfad);
    if (have != g_app->gegenstandGlmCache.end()) {
        return have->second.model.empty() ? nullptr : &have->second;
    }
    App::ActorAssets a;
    std::string data;
    if (readFromArchives(pfad, data)) {
        std::string err;
        if (readGlm(data, a.model, &err)) {
            a.textures = texturesFor(a.model);
        } else {
            diag::detail("Gegenstand " + pfad + " nicht lesbar: " + err);
            a.model = GlmModel{};
        }
    } else {
        g_app->fehlendeModelle.push_back(pfad);
    }
    const auto put = g_app->gegenstandGlmCache.emplace(pfad, std::move(a));
    return put.first->second.model.empty() ? nullptr : &put.first->second;
}

// Selbsttest: versteckte md3-Gegenstaende (Entity -> echter Ort).
std::map<std::size_t, std::array<float, 3>> g_versteckteMd3;
// Warum ein Gegenstand nicht steht (wartet auf use, Schwierigkeit, im Festen).
// rc604 fix604b (B3): dasselbe Modell mit festem animMap-Bild (EF_SHADER_ANIM:
// CG setzt RF_SETANIMINDEX und skinNum = s.frame, R_BindAnimatedImage nimmt
// dann dieses Bild statt der Zeit - code/rd-vanilla/tr_shade.cpp). Ein eigener
// Eintrag je Bild, damit der Zeichner nichts Neues wissen muss.
const App::ActorAssets* gegenstandGlmFuer(const std::string& pfad, int bild) {
    if (bild < 0) { return gegenstandGlmFuer(pfad); }
    const std::string schluessel = pfad + "|bild" + std::to_string(bild);
    const auto have = g_app->gegenstandGlmCache.find(schluessel);
    if (have != g_app->gegenstandGlmCache.end()) {
        return have->second.model.empty() ? nullptr : &have->second;
    }
    const App::ActorAssets* basis = gegenstandGlmFuer(pfad);
    App::ActorAssets a;
    if (basis != nullptr) {
        a.model = basis->model;
        a.textures = basis->textures;
        a.textures.zeichenSpeicher.reset();
        for (auto& flaeche : a.textures.stufen) {
            for (ModelTextures::FigurStufe& fs : flaeche) {
                if (fs.bilder.size() <= 1U) { continue; }
                const std::size_t i = std::min(static_cast<std::size_t>(bild), fs.bilder.size() - 1U);
                fs.bilder = {fs.bilder[i]};
                if (i < fs.pixel.size()) {
                    fs.pixel = {fs.pixel[i]};
                }
                fs.animFreq = 0.0F;
            }
        }
    }
    const auto put = g_app->gegenstandGlmCache.emplace(schluessel, std::move(a));
    return put.first->second.model.empty() ? nullptr : &put.first->second;
}

std::map<std::size_t, std::string> g_gegenstandGrund;

// --- rc604 fix604b (B5): misc_trip_mine -----------------------------------
//
// SP_misc_trip_mine (code/game/g_misc.cpp:1819-1868 OpenJK): Spur MASK_SHOT vom
// Ursprung 128 entlang AngleVectors(s.angles); im Festen oder ins Leere bricht
// das Spiel mit ERR_DROP ab (hier: nicht aufgestellt). CreateLaserTrap /
// touchLaserTrap / WP_Stick (wp_trip_mine.cpp:37-70, 177-215; g_weapon.cpp
// WP_Stick, fudge 0): das Modell weaponData[WP_TRIP_MINE].missileMdl
// (ext_data/weapons.dat: models/weapons2/laser_trap/laser_trap_w.glm) an
// trace.endpos, Winkel vectoangles(plane.normal); origin2 = Kastenspur (+-4)
// von endpos + 32 * Normale zurueck nach endpos. An ist die Falle ohne
// targetname oder mit START_ON (1): misc_trip_mine_activate -> laserTrapThink
// ein FRAMETIME spaeter, EF_FIRING; pos4 = Kastenspur von origin2 bis
// trBase + 2048 * Normale. targetname mit START_OFF (4): EF_NODRAW bis zum use.
// Hier nur die Lage beim Spawn (Spur gegen die Welt); was danach in jedem
// FRAMETIME geschieht, rechnet laserfalleLauf (rc604).
bool tripMineLage(const MapEntity& e, TripMineLage& l) {
    if (e.classname != "misc_trip_mine" || e.origin.empty()) { return false; }
    const std::shared_ptr<const KollisionsKarte> k = geladeneKollision(g_app->map.path);
    if (!k) { return false; }
    int sf = 0;
    if (const std::string* s = e.find("spawnflags")) { sf = std::atoi(s->c_str()); }
    const bool name = e.find("targetname") != nullptr && !e.find("targetname")->empty();
    // rc604: auch targetname + START_OFF - EF_NODRAW bis zum use (laserfalleLauf).
    l.hatName = name;
    l.sichtbarAbSpawn = !(name && (sf & 4) != 0);
    float o[3] = {0.0F, 0.0F, 0.0F};
    float w[3] = {0.0F, 0.0F, 0.0F};
    (void)std::sscanf(e.origin.c_str(), "%f %f %f", &o[0], &o[1], &o[2]);
    if (const std::string* a = e.find("angles")) {
        (void)std::sscanf(a->c_str(), "%f %f %f", &w[0], &w[1], &w[2]);
    } else if (const std::string* a1 = e.find("angle")) {
        w[1] = static_cast<float>(std::atof(a1->c_str()));
    }
    float achse[3][3];
    winkelZuAchsen(w, achse);
    const float null3[3] = {0.0F, 0.0F, 0.0F};
    float ende[3];
    for (int q = 0; q < 3; ++q) { ende[q] = o[q] + 128.0F * achse[0][q]; }
    const KollisionsSpur t = kollisionsSpur(*k, o, null3, null3, ende, kMaskeSchussWelt);
    if (t.allsolid || t.startsolid || t.fraction >= 1.0F) { return false; }
    for (int q = 0; q < 3; ++q) {
        l.ort[q] = t.endpos[static_cast<std::size_t>(q)];
        l.normale[q] = t.normale[static_cast<std::size_t>(q)];
    }
    // vectoangles (q_math.c)
    const float x = l.normale[0];
    const float y = l.normale[1];
    const float z = l.normale[2];
    float gier = 0.0F;
    float nick = 0.0F;
    constexpr float kGrad = 180.0F / 3.14159265358979323846F;
    if (y == 0.0F && x == 0.0F) {
        nick = z > 0.0F ? 90.0F : 270.0F;
    } else {
        gier = std::atan2(y, x) * kGrad;
        if (gier < 0.0F) { gier += 360.0F; }
        nick = std::atan2(z, std::sqrt(x * x + y * y)) * kGrad;
        if (nick < 0.0F) { nick += 360.0F; }
    }
    l.winkel[0] = -nick;
    l.winkel[1] = gier;
    l.winkel[2] = 0.0F;
    l.strahl = !name || (sf & 1) != 0;
    {
        const float mn[3] = {-4.0F, -4.0F, -4.0F};
        const float mx[3] = {4.0F, 4.0F, 4.0F};
        float o2[3];
        for (int q = 0; q < 3; ++q) { o2[q] = l.ort[q] + 32.0F * l.normale[q]; }
        const KollisionsSpur t2 = kollisionsSpur(*k, o2, mn, mx, l.ort, kMaskeSchussWelt);
        for (int q = 0; q < 3; ++q) { o2[q] = t2.endpos[static_cast<std::size_t>(q)]; }
        for (int q = 0; q < 3; ++q) { l.origin2[q] = o2[q]; }
        float e2[3];
        for (int q = 0; q < 3; ++q) { e2[q] = l.ort[q] + 2048.0F * l.normale[q]; }
        const KollisionsSpur t3 = kollisionsSpur(*k, o2, mn, mx, e2, kMaskeSchussWelt);
        for (int q = 0; q < 3; ++q) { l.strahlEnde[q] = t3.endpos[static_cast<std::size_t>(q)]; }
    }
    return true;
}

// --- rc604 fix604b (B3): Geschuetze und Waffenstellungen ------------------
//
// Die Kartenpruefung fand sie auf 30 Karten ungezeichnet: misc_turretG2 (13),
// misc_turret (7), emplaced_gun und emplaced_eweb (je 6), misc_sentry_turret,
// misc_ion_cannon. Ihre Spawnfunktionen setzen ein Ghoul2-Modell fest im Code
// und stellen es an origin mit G_SetAngles(s.angles) auf; die Knochenwinkel,
// die sie setzen (Bone_body, swivel_bone, cannon_Yrot ... mit vec3_origin),
// lassen die Grundhaltung stehen. Ohne Spieler und Feind bewegt sich nichts.
// misc_panel_turret (ein md3) steht in der Liste der festen md3 oben.
void geschuetzeAufstellen(int& gefunden, int& fehlt) {
    const bool md = regelwerkMovieDuels();
    for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
        const MapEntity& e = g_app->map.entities[i];
        const std::string& c = e.classname;
        if (!gespawntImSpiel(e)) { continue; }   // rc604
        int sf = 0;
        if (const std::string* s = e.find("spawnflags")) { sf = std::atoi(s->c_str()); }
        std::string pfad;
        float skala = 1.0F;
        bool kopfueber = false;
        int bild = -1;
        if (c == "misc_turret") {
            // SP_misc_turret (code/game/g_turret.cpp:865-891 OpenJK, md_g_turret.cpp:1014 MD):
            // turret_set_models -> turret_canon.glm; finish_spawning_turret; Start_Off
            // (spawnflags 1) s.frame 1 "black", sonst 0 "glow", EF_SHADER_ANIM.
            pfad = "models/map_objects/imp_mine/turret_canon.glm";
            kopfueber = (sf & 2) != 0;
            bild = (sf & 1) != 0 ? 1 : 0;
        } else if (c == "misc_turretG2") {
            // SP_misc_turretG2 (md_g_turret.cpp:1151-1280 MD, nur Movie Duels): spawnflags
            // der Karte - 8 turbo (laser_cannon_model.glm), 2 kopfueber, 1 Start_Off;
            // "customscale" (bis 1023) / 100 als s.modelScale.
            if (!kartenpruefung::imSpiel(c, md)) { continue; }
            pfad = (sf & 8) != 0 ? "models/map_objects/wedge/laser_cannon_model.glm"
                                 : "models/map_objects/imp_mine/turret_canon.glm";
            kopfueber = (sf & 2) != 0;
            bild = (sf & 1) != 0 ? 1 : 0;
            int cs = 0;
            if (const std::string* s = e.find("customscale")) { cs = std::atoi(s->c_str()); }
            cs = std::min(cs, 1023);
            if (cs > 0) { skala = static_cast<float>(cs) / 100.0F; }
        } else if (c == "emplaced_gun") {
            // SP_emplaced_gun (code/game/g_emplaced.cpp:786-860 OpenJK): turret_chair.glm.
            pfad = "models/map_objects/imp_mine/turret_chair.glm";
        } else if (c == "emplaced_eweb") {
            // SP_emplaced_eweb (g_emplaced.cpp:416-490 OpenJK): eweb_model.glm.
            pfad = "models/map_objects/hoth/eweb_model.glm";
        } else if (c == "misc_sentry_turret") {
            // SP_PAS (g_turret.cpp:1784-1800 OpenJK): models/items/psgun.glm, modelScale 1.
            // (pas_think klappt es nach 1 s per Animation auf - nicht nachgebildet.)
            pfad = "models/items/psgun.glm";
        } else if (c == "misc_trip_mine") {
            // rc604 fix604b (B5): siehe tripMineLage.
            TripMineLage tl;
            if (!kartenpruefung::imSpiel(c, md) || !tripMineLage(e, tl)) { continue; }
            static const std::string kFalle = "models/weapons2/laser_trap/laser_trap_w.glm";
            if (gegenstandGlmFuer(kFalle) == nullptr) {
                ++fehlt;
                continue;
            }
            App::GegenstandGlm gg;
            gg.entity = i;
            gg.pfad = kFalle;
            float achse[3][3];
            winkelZuAchsen(tl.winkel, achse);
            for (int r = 0; r < 3; ++r) {
                for (int k = 0; k < 3; ++k) { gg.welt[r * 4 + k] = achse[k][r]; }
                gg.welt[r * 4 + 3] = tl.ort[r];
                gg.ort[r] = tl.ort[r];
            }
            gg.welt[15] = 1.0F;
            g_app->gegenstandGlm.push_back(std::move(gg));
            ++gefunden;
            continue;
        } else if (c == "misc_ion_cannon") {
            // SP_misc_ion_cannon (g_turret.cpp:2064-2080 OpenJK): ion_cannon.glm.
            // rc604: mit s.modelScale 2 2 2 (g_turret.cpp:2074) - CG_General
            // skaliert die Achsen damit (ScaleModelAxis).
            pfad = "models/map_objects/imp_mine/ion_cannon.glm";
            skala = 2.0F;
        } else {
            continue;
        }
        if (!kartenpruefung::imSpiel(c, md) || e.origin.empty()) { continue; }
        if (gegenstandGlmFuer(pfad) == nullptr) {
            ++fehlt;
            continue;
        }
        float ort[3] = {0.0F, 0.0F, 0.0F};
        float w[3] = {0.0F, 0.0F, 0.0F};
        (void)std::sscanf(e.origin.c_str(), "%f %f %f", &ort[0], &ort[1], &ort[2]);
        if (const std::string* a = e.find("angles")) {
            (void)std::sscanf(a->c_str(), "%f %f %f", &w[0], &w[1], &w[2]);
        } else if (const std::string* a1 = e.find("angle")) {
            w[1] = static_cast<float>(std::atof(a1->c_str()));
        }
        if (kopfueber) {
            // finish_spawning_turret: "if ( base->spawnflags & 2 ) { s.angles[ROLL] += 180;
            // s.origin[2] -= 22.0f; }" (g_turret.cpp:895-899 OpenJK)
            w[2] += 180.0F;
            ort[2] -= 22.0F;
        }
        App::GegenstandGlm gg;
        gg.entity = i;
        gg.pfad = pfad;
        gg.bildIndex = bild;
        float achse[3][3];
        winkelZuAchsen(w, achse);
        for (int r = 0; r < 3; ++r) {
            for (int k = 0; k < 3; ++k) { gg.welt[r * 4 + k] = achse[k][r] * skala; }
            gg.welt[r * 4 + 3] = ort[r];
            gg.ort[r] = ort[r];
        }
        gg.welt[15] = 1.0F;
        g_app->gegenstandGlm.push_back(std::move(gg));
        ++gefunden;
    }
}

void gegenstaendeAufstellen(int& gefunden, int& fehlt) {
    g_app->gegenstandGlm.clear();
    g_versteckteMd3.clear();
    g_gegenstandGrund.clear();
    std::string dat;
    if (!readFromArchives("ext_data/items.dat", dat)) {
        diag::detail("Gegenstaende: ext_data/items.dat nicht gefunden");
        return;
    }
    const bool md = regelwerkMovieDuels();
    const Gegenstandsliste liste = leseGegenstaende(dat, md, regelwerkJediOutcast());   // rc604 jk2
    const std::shared_ptr<const KollisionsKarte> k = geladeneKollision(g_app->map.path);
    // Was beim Fallen im Weg ist: die Welt und die Brush-Entities an ihrem
    // Platz beim Spawnen (G_Trace -> SV_ClipMoveToEntities,
    // CM_TransformedBoxTrace). Ob eine Brush-Entity fest ist, entscheiden hier
    // die Inhalte ihrer Brushes gegen die Maske (im Spiel zusaetzlich
    // ent->contents - func_wall START_OFF etwa hat keine).
    struct Pinsel {
        int modell = 0;
        float ursprung[3]{};
        float winkel[3]{};
    };
    std::vector<Pinsel> pinsel;
    for (const MapEntity& e : g_app->map.entities) {
        const std::string* m = e.find("model");
        int nr = 0;
        if (m == nullptr || m->size() < 2 || (*m)[0] != '*' || !parseInt(m->substr(1), nr) || nr <= 0) { continue; }
        if (e.classname.rfind("trigger_", 0) == 0) { continue; }
        Pinsel p;
        p.modell = nr;
        (void)std::sscanf(e.origin.c_str(), "%f %f %f", &p.ursprung[0], &p.ursprung[1], &p.ursprung[2]);
        if (const std::string* w = e.find("angles")) {
            (void)std::sscanf(w->c_str(), "%f %f %f", &p.winkel[0], &p.winkel[1], &p.winkel[2]);
        } else if (const std::string* w1 = e.find("angle")) {
            p.winkel[1] = static_cast<float>(std::atof(w1->c_str()));
        }
        pinsel.push_back(p);
    }
    // MASK_SOLID | CONTENTS_PLAYERCLIP (bg_public.h:998, surfaceflags.h:37 MD).
    constexpr std::uint32_t kMaske = kMaskeFest | 0x00000010U;
    const GegenstandSpur spur = [&](const std::array<float, 3>& start, const std::array<float, 3>& mins,
                                    const std::array<float, 3>& maxs, const std::array<float, 3>& ende,
                                    std::array<float, 3>& endpos, bool& startsolid) {
        KollisionsSpur best = kollisionsSpur(*k, start.data(), mins.data(), maxs.data(), ende.data(), kMaske);
        startsolid = best.startsolid;
        for (const Pinsel& p : pinsel) {
            const KollisionsSpur t = kollisionsSpur(*k, start.data(), mins.data(), maxs.data(), ende.data(), kMaske,
                                                    p.modell, p.ursprung, p.winkel);
            if (t.startsolid) { startsolid = true; }
            if (t.fraction < best.fraction) { best = t; }
        }
        endpos = best.endpos;
    };
    int zahl = 0;
    int ohne = 0;
    for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
        const MapEntity& e = g_app->map.entities[i];
        const Gegenstand* g = liste.spawnbar(e.classname);
        if (g == nullptr) { continue; }
        const GegenstandAufstellung a =
            gegenstandAufstellen(e, *g, regelwerkSpielstufe(), md, k ? spur : GegenstandSpur{});
        if (!a.sichtbar) {
            diag::detail("Gegenstand " + e.classname + " #" + std::to_string(i) + ": " + a.grund);
            g_gegenstandGrund[i] = a.grund;
            ++ohne;
            continue;
        }
        if (a.glm) {
            if (gegenstandGlmFuer(a.modell) == nullptr) {
                // Auch im Spiel nichts: G2API_InitGhoul2Model scheitert, und
                // cgi_R_RegisterModel findet die Datei ebenso wenig.
                g_gegenstandGrund[i] = "Modell fehlt: " + a.modell;
                ++fehlt;
                continue;
            }
            App::GegenstandGlm gg;
            gg.entity = i;
            gg.pfad = a.modell;
            float achse[3][3];
            winkelZuAchsen(a.winkel.data(), achse);
            // G2_GenerateWorldMatrix: matrix[r][c] = axis[c][r], Spalte 3 = Ort.
            for (int r = 0; r < 3; ++r) {
                for (int c = 0; c < 3; ++c) { gg.welt[r * 4 + c] = achse[c][r]; }
                gg.welt[r * 4 + 3] = a.ort[static_cast<std::size_t>(r)];
                gg.ort[r] = a.ort[static_cast<std::size_t>(r)];
            }
            gg.welt[15] = 1.0F;
            g_app->gegenstandGlm.push_back(std::move(gg));
            ++zahl;
            continue;
        }
        std::string low = a.modell;
        for (char& c : low) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        if (g_app->md3Files.find(low) == g_app->md3Files.end()) {
            std::string data;
            Md3Model mm;
            std::string err;
            if (!readFromArchives(a.modell, data)) {
                g_app->fehlendeModelle.push_back(a.modell);
                g_gegenstandGrund[i] = "Modell fehlt: " + a.modell;
                ++fehlt;
                continue;
            }
            if (!readMd3(data, mm, &err)) {
                diag::info("Gegenstand nicht lesbar: " + a.modell + " - " + err);
                g_gegenstandGrund[i] = "Modell nicht lesbar: " + a.modell;
                ++fehlt;
                continue;
            }
            g_app->md3Files[low] = std::move(mm);
        }
        const Md3Model& mm = g_app->md3Files[low];
        if (mm.empty()) {
            g_gegenstandGrund[i] = "Modell ohne Flaechen: " + a.modell;   // zeichnet auch im Spiel nichts
            continue;
        }
        App::MapModel item;
        item.path = low;
        item.frames = mm.numFrames;
        item.standbild = true;   // CG_Item: ent.frame bleibt 0
        item.entity = i;
        for (int q = 0; q < 3; ++q) {
            item.origin[q] = a.ort[static_cast<std::size_t>(q)];
            item.angles[q] = a.md3Winkel[static_cast<std::size_t>(q)];
        }
        item.shaderBase = modellBlock("g|" + low, [&mm] {
            for (const Md3Surface& sf : mm.surfaces) {
                TextureSet::Tex t;
                loadTextureFor(sf.shader, t);
                g_app->textures.byShader.push_back(std::move(t));
            }
        });
        g_app->mapModels.push_back(std::move(item));
        ++gefunden;
        ++zahl;
    }
    diag::info("Gegenstaende: " + std::to_string(zahl) + " aufgestellt, " + std::to_string(ohne) +
               " nicht (wartet/unsichtbar/entfernt)");
}

}  // namespace

bool gegenstandInstanz(std::size_t entity, float ort[3], std::string* modell, bool* glm) {
    for (const App::GegenstandGlm& gg : g_app->gegenstandGlm) {
        if (gg.entity != entity) { continue; }
        for (int k = 0; k < 3; ++k) { ort[k] = gg.ort[k]; }
        if (modell != nullptr) { *modell = gg.pfad; }
        if (glm != nullptr) { *glm = true; }
        return true;
    }
    std::string dat;
    for (const App::MapModel& mm : g_app->mapModels) {
        if (mm.entity != entity || mm.anBrush != 0) { continue; }
        const MapEntity& e = g_app->map.entities[entity];
        if (e.classname.rfind("misc_", 0) == 0) { continue; }
        const auto v = g_versteckteMd3.find(entity);
        for (int k = 0; k < 3; ++k) { ort[k] = v != g_versteckteMd3.end() ? v->second[static_cast<std::size_t>(k)] : mm.origin[k]; }
        if (modell != nullptr) { *modell = mm.path; }
        if (glm != nullptr) { *glm = false; }
        return true;
    }
    return false;
}

bool gegenstandNichtImSpiel(std::size_t entity, std::string* grund) {
    const auto it = g_gegenstandGrund.find(entity);
    if (it == g_gegenstandGrund.end()) { return false; }
    if (grund != nullptr) { *grund = it->second; }
    return true;
}

void gegenstandVerstecken(std::size_t entity, bool aus) {
    for (App::GegenstandGlm& gg : g_app->gegenstandGlm) {
        if (gg.entity == entity) { gg.versteckt = aus; }
    }
    for (App::MapModel& mm : g_app->mapModels) {
        if (mm.entity != entity || mm.anBrush != 0) { continue; }
        if (aus && g_versteckteMd3.count(entity) == 0U) {
            g_versteckteMd3[entity] = {mm.origin[0], mm.origin[1], mm.origin[2]};
            mm.origin[2] -= 100000.0F;
        } else if (!aus) {
            const auto v = g_versteckteMd3.find(entity);
            if (v != g_versteckteMd3.end()) {
                for (int k = 0; k < 3; ++k) { mm.origin[k] = v->second[static_cast<std::size_t>(k)]; }
                g_versteckteMd3.erase(v);
            }
        }
    }
}

void loadMapModels() {
    diag::Step step("Modelle der Karte suchen");
    g_app->mapModels.clear();
    g_app->fehlendeModelle.clear();
    // rc605: die Modelle der Emitter (useModel) laedt loadMapEffects DAVOR in
    // dieselbe Liste - sie bleiben. Vorher fiel hier jedes Effektmodell weg
    // (emitterModels kannte es noch, md3Files nicht mehr), und kein Emitter-
    // modell wurde gezeichnet (etwa die Nebelvierecke von duel_exegol).
    std::map<std::string, Md3Model> emitterDateien;
    for (const auto& [pfad, em] : g_app->emitterModels) {
        const auto it = g_app->md3Files.find(pfad);
        if (it != g_app->md3Files.end()) { emitterDateien.emplace(pfad, std::move(it->second)); }
    }
    g_app->md3Files = std::move(emitterDateien);
    // Die Grafikpuffer hingen an diesen Netzen (Schluessel: Adresse).
    for (const auto& [name, netz] : g_app->md3Meshes) {
        gpu::vergissNetz(&netz);
    }
    g_app->md3Meshes.clear();

    int gefunden = 0;
    int fehlt = 0;
    for (const MapEntity& e : g_app->map.entities) {
        if (!gespawntImSpiel(e)) { continue; }   // rc604
        if (miscModelStaticImFesten(e)) {   // Stufe 6 (OpenJK#1358)
            diag::info("misc_model_static bei " + e.origin + ": Pruefpunkt im Festen, das Spiel zeichnet es nie (OpenJK#1358)");
            continue;
        }
        const std::string* mp = e.find("model");
        // Die Waffenstaender nehmen ihr Modell NICHT aus dem Schluessel -
        // g_misc_model.cpp setzt es fest: SP_misc_model_gun_rack
        // G_ModelIndex("models/map_objects/kejim/weaponsrack.md3"),
        // spawn_rack_goods (ammo_rack) ".../kejim/weaponsrung.md3".
        // In den MD-Karten steht oft imperial/... (gibt es nicht) oder gar
        // nichts - 270 Staender, die vorher fehlten.
        static const std::string kGunRack = "models/map_objects/kejim/weaponsrack.md3";
        static const std::string kAmmoRack = "models/map_objects/kejim/weaponsrung.md3";
        // Ebenso fest im Code (g_misc.cpp): SP_misc_exploding_crate,
        // SP_misc_gas_tank, SP_misc_crystal_crate, SP_misc_model_bomb_planted
        // und SP_misc_model_beacon. Die Kiste "rocket_crate_dan" in md_afh
        // fehlte so.
        static const std::pair<const char*, std::string> kFest[] = {
            {"misc_exploding_crate", "models/map_objects/nar_shaddar/crate_xplode.md3"},
            {"misc_gas_tank", "models/map_objects/imp_mine/tank.md3"},
            {"misc_crystal_crate", "models/map_objects/imp_mine/crate_open.md3"},
            {"misc_model_bomb_planted", "models/map_objects/factory/bomb_new_deact.md3"},
            {"misc_model_beacon", "models/map_objects/wedge/beacon.md3"},
            // rc602 kartenpruefung: SP_misc_shield_floor_unit (g_misc.cpp:2400
            // MD), SP_misc_ammo_floor_unit (:2638), SP_misc_security_panel
            // (g_mover.cpp:2869), SP_misc_camera (g_misc.cpp:953, der Kopf;
            // der Fuss kommt unten dazu).
            {"misc_shield_floor_unit", "models/items/a_shield_converter.md3"},
            {"misc_ammo_floor_unit", "models/items/a_pwr_converter.md3"},
            {"misc_security_panel", "models/map_objects/kejim/sec_panel.md3"},
            {"misc_camera", "models/map_objects/kejim/impcam.md3"},
            // rc604 fix604b (B3): SP_misc_panel_turret (code/game/g_turret.cpp:2484-2516
            // OpenJK): ladyluck_gun.md3 an origin, G_SetAngles(s.angles).
            {"misc_panel_turret", "models/map_objects/imp_mine/ladyluck_gun.md3"},
        };
        bool fest = false;
        if (e.classname == "misc_model_gun_rack") {
            mp = &kGunRack;
        } else if (e.classname == "misc_model_ammo_rack") {
            mp = &kAmmoRack;
        } else {
            for (const auto& [klasse, pfad] : kFest) {
                if (e.classname == klasse) {
                    mp = &pfad;
                    fest = true;
                }
            }
        }
        // "model2" an einem Brush-Mover (func_static, func_door, ...): das md3
        // haengt am Mover und faehrt mit ihm - etwa ein Schiffsmodell an
        // einem unsichtbaren Brush (rc592; vorher fehlte es ganz).
        int anBrush = 0;
        if (const std::string* m2 = e.find("model2");
            m2 != nullptr && m2->size() > 4 && mp != nullptr && !mp->empty() && (*mp)[0] == '*' &&
            e.classname.rfind("func_", 0) == 0) {
            int nr = 0;
            if (parseInt(mp->substr(1), nr) && nr > 0) {
                anBrush = nr;
                mp = m2;
            }
        }
        // rc604 fix604b (B6): ohne "origin" steht eine gespawnte Entity am
        // Nullpunkt - G_SetOrigin(self, self->s.origin) mit dem leeren s.origin
        // (G_SpawnGEntityFromSpawnVars liest nur, was dasteht). vjun3 hat so
        // einen misc_model_ammo_rack mit nichts als der Klasse.
        const bool gespawntesModell = e.classname.rfind("misc_model", 0) == 0 || fest;
        if (mp == nullptr || mp->size() < 5 || (e.origin.empty() && anBrush == 0 && !gespawntesModell)) {
            continue;
        }
        // Nur, was die Engine als Modell aufstellt: misc_model_static,
        // _breakable, _gun_rack, _ammo_rack, _beacon ... Ein target_speaker
        // mit "model"-Schluessel (yavin1b) oder misc_siege_item (nur im
        // Mehrspieler) zeichnet das Einzelspieler-Spiel nicht.
        if (e.classname.rfind("misc_model", 0) != 0 && !fest && anBrush == 0) {
            continue;
        }
        std::string low = *mp;
        for (char& c : low) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (low.size() < 4 || low.compare(low.size() - 4, 4, ".md3") != 0) {
            continue;   // "*12" ist ein Brush-Modell, das laeuft anderswo
        }

        if (g_app->md3Files.find(low) == g_app->md3Files.end()) {
            std::string data;
            if (!readFromArchives(*mp, data)) {
                diag::info("kein Modell: " + *mp);
                g_app->fehlendeModelle.push_back(*mp);
                ++fehlt;
                continue;
            }
            Md3Model mm;
            std::string err;
            if (!readMd3(data, mm, &err)) {
                diag::info("Modell nicht lesbar: " + *mp + " - " + err);
                ++fehlt;
                continue;
            }
            g_app->md3Files[low] = std::move(mm);
        }
        const Md3Model& mm = g_app->md3Files[low];
        if (mm.empty()) {
            continue;   // gueltig, aber ohne Flaechen (reines Tag-Modell)
        }

        App::MapModel item;
        item.path = low;
        item.frames = mm.numFrames;
        item.entity = static_cast<std::size_t>(&e - g_app->map.entities.data());
        item.anBrush = anBrush;
        (void)std::sscanf(e.origin.c_str(), "%f %f %f", &item.origin[0],
                          &item.origin[1], &item.origin[2]);
        // Dieselbe Regel wie bei den Figuren: "angles" nimmt drei Werte,
        // "angle" nur den Gierwinkel (g_spawn.cpp, F_ANGLEHACK).
        // Winkel und Groesse wie SP_misc_model_static / G_ParseField:
        // "angle" setzt die Gier, "angles" alle drei; "modelscale_vec" je
        // Achse, "modelscale" (ungleich null) ueberschreibt alle drei.
        float einheitlich = 0.0F;
        for (const auto& [k, v] : e.keys) {
            std::string lk = k;
            for (char& c : lk) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            if (lk == "angles") {
                float a[3] = {0, 0, 0};
                (void)std::sscanf(v.c_str(), "%f %f %f", &a[0], &a[1], &a[2]);
                for (int q = 0; q < 3; ++q) { item.angles[q] = a[q]; }
            } else if (lk == "angle") {
                item.angles[0] = 0.0F;
                item.angles[1] = static_cast<float>(std::atof(v.c_str()));
                item.angles[2] = 0.0F;
            } else if (lk == "modelscale_vec") {
                float s[3] = {1, 1, 1};
                (void)std::sscanf(v.c_str(), "%f %f %f", &s[0], &s[1], &s[2]);
                for (int q = 0; q < 3; ++q) { item.scale[q] = s[q]; }
            } else if (lk == "modelscale") {
                einheitlich = static_cast<float>(std::atof(v.c_str()));
            }
        }
        if (einheitlich != 0.0F) {
            for (int q = 0; q < 3; ++q) { item.scale[q] = einheitlich; }
        }
        if (anBrush > 0) {
            // "angle" ist bei Tueren die Bewegungsrichtung, keine Drehung -
            // das model2 dreht nur mit den Winkeln des Movers.
            for (int q = 0; q < 3; ++q) {
                item.angles[q] = 0.0F;
                item.scale[q] = 1.0F;
            }
        }

        // SP_misc_shield_floor_unit / SP_misc_ammo_floor_unit: nur das erste
        // animMap-Bild (siehe unten) - ein eigener Block.
        const bool ladestation = e.classname == "misc_shield_floor_unit" || e.classname == "misc_ammo_floor_unit";
        item.shaderBase = modellBlock("m|" + low + (ladestation ? "|bild0" : ""), [&] {
            for (const Md3Surface& sf : mm.surfaces) {
                TextureSet::Tex t;
                loadTextureFor(sf.shader, t);
                // Ins Protokoll, WELCHE Flaeche welchen Test bekommt.
                //
                // Gemeldet: "leider sind die Geonosianer immer noch so."
                //
                // Zweimal am falschen Ende gesucht - erst im Figurenzeichner,
                // dann im Flaechenzeichner. Die 34 gri_head.md3 sind
                // KARTENMODELLE und gehen einen dritten Weg. Ohne eine Zeile,
                // die sagt, was bei einem Modell ankommt, faellt so etwas erst
                // beim Hinsehen auf - und dann ist eine Runde vorbei.
                if (t.alphaTest != AlphaTest::None) {
                    diag::detail("Kartenmodell " + item.path + ", Flaeche \"" +
                                 sf.shader + "\": Alphatest an");
                }
                // rc602 kartenpruefung: EF_SHADER_ANIM (die Ladestationen,
                // g_misc.cpp:2404/2639 MD) - der Shader zeigt das animMap-Bild
                // s.frame (CG: RF_SETANIMINDEX, skinNum = s.frame; RB_ComputeTexMods/
                // R_BindAnimatedImage "if (backEnd.currentEntity->e.renderfx &
                // RF_SETANIMINDEX) index = skinNum"), nicht nach der Zeit. Beim
                // Kartenstart ist s.frame 0: das erste Bild, voll geladen.
                if (ladestation && !t.animFrames.empty()) {
                    t.animFrames.resize(1);
                    t.animFreq = 0.0F;
                }
                g_app->textures.byShader.push_back(std::move(t));
            }
        });
        // SP_misc_camera: der Fuss impcam_base.md3 ist eine eigene Entity,
        // 16 hoeher, mit denselben Winkeln (G_SetAngles(base, self->s.angles)).
        if (e.classname == "misc_camera") {
            static const std::string kFuss = "models/map_objects/kejim/impcam_base.md3";
            if (g_app->md3Files.find(kFuss) == g_app->md3Files.end()) {
                std::string data;
                Md3Model fm;
                std::string err;
                if (readFromArchives(kFuss, data) && readMd3(data, fm, &err)) {
                    g_app->md3Files[kFuss] = std::move(fm);
                } else {
                    g_app->fehlendeModelle.push_back(kFuss);
                }
            }
            const auto fuss = g_app->md3Files.find(kFuss);
            if (fuss != g_app->md3Files.end() && !fuss->second.empty()) {
                App::MapModel b = item;
                b.path = kFuss;
                b.frames = fuss->second.numFrames;
                b.origin[2] += 16.0F;
                b.shaderBase = modellBlock("m|" + kFuss, [&fuss] {
                    for (const Md3Surface& sf : fuss->second.surfaces) {
                        TextureSet::Tex t;
                        loadTextureFor(sf.shader, t);
                        g_app->textures.byShader.push_back(std::move(t));
                    }
                });
                g_app->mapModels.push_back(std::move(b));
            }
        }
        g_app->mapModels.push_back(std::move(item));
        ++gefunden;
    }
    gegenstaendeAufstellen(gefunden, fehlt);   // rc602 kartenpruefung
    geschuetzeAufstellen(gefunden, fehlt);     // rc604 fix604b (B3)
    diag::info(std::to_string(gefunden) + " Modelle aufgestellt, " +
               std::to_string(fehlt) + " fehlen, " +
               std::to_string(g_app->md3Files.size()) + " Dateien gelesen");

    // --- Was erst ein use braucht ----------------------------------------
    //
    // Das Schadensmodell "<name>_d1.md3" eines misc_model_breakable, das
    // Benutzmodell "_u1" und die Bruchstuecke (models/chunks/..., oder das
    // eigene "_c1"). Welche es sind, weiss MoverSim (nebenModelle) - hier
    // werden sie VORHER gelesen, nicht mitten im Bild.
    //
    // Fehlt ein _d1, zeigt die Engine nichts: G_ModelIndex vergibt die
    // Nummer trotzdem, R_RegisterModel findet keine Datei, und Modell 0
    // wird nicht gezeichnet. Genau so bleibt es hier weg.
    g_app->nebenModelle.clear();
    int neben = 0;
    for (const std::string& pfad : g_app->moverSim.nebenModelle()) {
        const std::string low = pfad;   // MoverSim liefert schon klein
        if (g_app->md3Files.find(low) == g_app->md3Files.end()) {
            std::string data;
            if (!readFromArchives(low, data)) {
                diag::detail("Nebenmodell fehlt (die Engine zeigt dann nichts): " + low);
                continue;
            }
            Md3Model mm;
            std::string err;
            if (!readMd3(data, mm, &err)) {
                diag::detail("Nebenmodell nicht lesbar: " + low + " - " + err);
                continue;
            }
            g_app->md3Files[low] = std::move(mm);
        }
        const Md3Model& mm = g_app->md3Files[low];
        if (mm.empty()) {
            continue;
        }
        const int basis = modellBlock("m|" + low, [&mm] {
            for (const Md3Surface& sf : mm.surfaces) {
                TextureSet::Tex t;
                loadTextureFor(sf.shader, t);
                g_app->textures.byShader.push_back(std::move(t));
            }
        });
        g_app->nebenModelle[low] = basis;
        ++neben;
    }
    if (neben != 0) {
        diag::info(std::to_string(neben) + " Neben- und Bruchstueckmodelle gelesen");
    }
}

// Alles, was die Ansicht braucht, JETZT bereitstellen.
//
// Vorher geschah das im Zeichnen: drawTimeline() baute die Szene, wenn
// camTrackValid falsch war, und leerte actorAssets; das naechste Bild sah
// die Groessen auseinanderlaufen und lud Figurenmodelle, Effekte und
// .md3-Modelle nach - mitten im Bild. Gemeldet als "erst laedt sie und ich
// sehe die map und wenn ich es dann bewege haengt es kurz und laedt
// nochmal irgendwas".
//
// Der Grund ist einfach: gezeichnet wird nur bei mapDirty. Nach dem Laden
// stand ein Bild, danach passierte nichts mehr - bis eine Bewegung ein
// neues Bild anforderte, und DANN lief das Nachladen los.
//
// Jetzt laeuft es in der Ladephase, wo es hingehoert: der Fortschritt steht
// im Protokoll, und die Ansicht ist fertig, sobald sie erscheint.
void prepareScene();

// --- Griffe und Klingenbilder VORLADEN -----------------------------------
//
// Aus deinem Protokoll:
//
//     Nachgeladen mitten im Bild: Schwertgriff "...saber_mace.glm" - 51.9 ms
//     ... zehn Stueck, zusammen rund 510 ms
//     Zeit je Bild 535.0 ms: ... Szene 511.8 ...
//
// Damit ist das erste Bild erklaert: zehn Schwertgriffe, jeder rund fuenfzig
// Millisekunden. Und waehrend des Abspielens:
//
//     Nachgeladen mitten im Bild: Klingenbild "...purple_glow2" - 11.2 ms
//     Zeit je Bild 53.3 ms: ... Szene 14.5 ...
//
// Die 14,5 aus rc305 waren also die Klingenbilder, nicht die Griffe.
//
// Warum fuenfzig Millisekunden fuer eine kleine Datei? Nicht das Suchen -
// das ist eine Hashtabelle. Es ist das Auspacken, Dekodieren und die
// Bildpyramide der GRIFFTEXTUREN, mehrere je Griff.
//
// Schneller machen laesst sich das kaum. Aber es muss nicht mitten in ein
// Bild fallen: WELCHE Griffe und Farben vorkommen, steht in den .npc der
// Figuren, und die sind hier schon geladen.
//
// WICHTIG, und in rc307 hatte ich es falsch behauptet: das hier laeuft
// INNERHALB der Bildzeitmessung, denn der ganze Ablauf ist bildgetrieben.
// Die halbe Sekunde bleibt also im ERSTEN Bild stehen und wandert nicht in
// den Ladevorgang.
//
// Was es tut, ist trotzdem das Richtige: es holt alles EINMAL beim ersten
// Bild, statt es spaeter mitten ins Abspielen fallen zu lassen. Ein
// langsames erstes Bild beim Missionsstart faellt nicht auf; ein Ruckler
// mitten in einer Szene schon.
void preloadSaberAssets() {
    std::size_t griffe = 0;
    std::size_t bilder = 0;
    // Jedes Modell, das eine Figur im Lauf der Szene in die Hand nehmen
    // kann: Waffen aus SET_WEAPON und der .npc, Griffe aus SET_SABER1/2 und
    // der .npc, Kinomodelle. Actor::modelle fuehrt sie alle.
    for (const Actor& a : g_app->scene.actors) {
        for (const std::string& m : a.modelle) {
            if (!m.empty() && g_app->hiltCache.find(m) == g_app->hiltCache.end()) {
                (void)hiltFor(m);
                ++griffe;
            }
        }
    }

    // --- ALLE Klingenfarben, nicht nur die gefundenen -------------------
    //
    // Mein erster Versuch (rc307) hat nur die Farben aus den .sab der
    // vorhandenen Figuren geholt. Dein Protokoll zeigte danach trotzdem:
    //
    //     Nachgeladen: Klingenbild "...purple_glow2" - 10.8 ms (spielt)
    //     Nachgeladen: Klingenbild "...blue_glow2"   - 10.7 ms (spielt)
    //     Nachgeladen: Klingenbild "...green_glow2"  - 11.2 ms (spielt)
    //
    // Drei Farben, die ich nicht erwischt hatte. WARUM, weiss ich nicht
    // sicher - und ich habe in dieser Sitzung schon zweimal an dieser
    // Stelle falsch geraten.
    //
    // Also nicht noch einmal raten: es gibt genau NEUN Klingenfarben
    // (siehe bladeColorFor). Alle zu holen kostet einmalig gut hundert
    // Millisekunden beim Laden und ist dafuer VOLLSTAENDIG - keine
    // Herleitung, die daneben liegen kann.
    for (const char* farbe : {"red", "orange", "yellow", "green", "blue",
                              "purple", "white", "black", "unstable_red"}) {
        const std::size_t vorher = g_app->texCache.size();
        (void)bladeTexture(farbe, true);
        (void)bladeTexture(farbe, false);
        bilder += g_app->texCache.size() - vorher;
    }
    if (griffe != 0 || bilder != 0) {
        diag::info("Vorgeladen: " + std::to_string(griffe) +
                   " Schwertgriffe, " + std::to_string(bilder) +
                   " Klingenbilder");
    }
}

// --- Die Kino-Skelette der Karte ---------------------------------------------
//
// NPC_stats.cpp:1245: fuer jeden Standard-Humanoiden (G_StandardHumanoid,
// g_client.cpp:1284) laedt die Engine zusaetzlich
//     models/players/_humanoid_<karte>/_humanoid_<karte>.gla
// und dessen animation.cfg als zweite Animationsdatei. Die BOTH_CIN_*-
// Animationen der Zwischensequenzen stehen NUR dort - ohne sie blieb eine
// Figur in academy1 bei "BOTH_CIN_1" einfach stehen (Szenenlauf 27.09.:
// "steht nicht in der .cfg von _humanoid").
//
// Das Grundskelett ist geteilt (ein Eintrag fuer alle Karten), deshalb
// wird der Anhang bei jedem Aufbau auf die AKTUELLE Karte abgeglichen:
// alte Kino-Abschnitte weg, neue dran.
void kinoSkeletteAbgleichen() {
    static const char* const kHumanoid[] = {
        "_humanoid", "JK2anims", "_humanoid_ani", "_humanoid_bdroid", "_humanoid_ben", "_humanoid_cal",
        "_humanoid_clo", "_humanoid_deka", "_humanoid_df2", "_humanoid_dooku", "_humanoid_galen",
        "_humanoid_gon", "_humanoid_grievous", "_humanoid_jabba", "_humanoid_jango", "_humanoid_kotor",
        "_humanoid_luke", "_humanoid_mace", "_humanoid_maul", "_humanoid_md", "_humanoid_melee",
        "_humanoid_obi", "_humanoid_obi3", "_humanoid_pal", "_humanoid_reb", "_humanoid_ren", "_humanoid_rey",
        "_humanoid_sbd", "_humanoid_vader", "_humanoid_yoda", "protocol", "assassin_droid", "saber_droid",
        "hazardtrooper", "rockettrooper", "wampa", "galak_mech", "droideka", "kotor_monster"};
    const auto klein = [](std::string x) {
        for (char& c : x) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (c == '\\') { c = '/'; }
        }
        return x;
    };
    // Die Karte: "maps/academy1.bsp" -> "academy1" (level.mapname ohne Pfad).
    std::string karte = klein(g_app->map.path);
    if (const auto s = karte.find_last_of('/'); s != std::string::npos) { karte = karte.substr(s + 1); }
    if (karte.size() > 4 && karte.compare(karte.size() - 4, 4, ".bsp") == 0) { karte.resize(karte.size() - 4); }
    const std::string kinoName = "models/players/_humanoid_" + karte + "/_humanoid_" + karte;
    // Das Kino-Skelett einmal lesen (auch "nicht da" merken: leeres anim).
    App::Skeleton* kino = nullptr;
    if (!karte.empty()) {
        auto it = g_app->skeletons.find(kinoName);
        if (it == g_app->skeletons.end()) {
            App::Skeleton neu;
            std::string data;
            if (readFromArchives(kinoName + ".gla", data)) {
                std::string err;
                if (!readGla(data, neu.anim, &err)) {
                    diag::detail("Kino-Skelett " + kinoName + " nicht lesbar: " + err);
                }
                neu.sections = animSectionsFor(kinoName);
                diag::info("Kino-Skelett " + kinoName + ": " + std::to_string(neu.anim.numFrames) + " Bilder, " +
                           std::to_string(neu.sections.size()) + " Abschnitte");
            }
            it = g_app->skeletons.emplace(kinoName, std::move(neu)).first;
        }
        if (!it->second.anim.empty()) {
            kino = &it->second;
        }
    }
    for (auto& [name, sk] : g_app->skeletons) {
        if (&sk == kino) {
            continue;
        }
        // Der Skelettname der Engine ist der Ordner: ".../_humanoid_md/_humanoid" -> "_humanoid_md".
        std::string ordner = name;
        if (const auto s = ordner.find_last_of("/\\"); s != std::string::npos) { ordner = ordner.substr(0, s); }
        if (const auto s = ordner.find_last_of("/\\"); s != std::string::npos) { ordner = ordner.substr(s + 1); }
        bool humanoid = false;
        for (const char* h : kHumanoid) {
            humanoid = humanoid || klein(ordner) == klein(h);
        }
        if (!humanoid) {
            continue;
        }
        if (sk.kinoKarte == karte && sk.grundAbschnitte != static_cast<std::size_t>(-1)) {
            continue;   // schon fuer diese Karte
        }
        if (sk.grundAbschnitte == static_cast<std::size_t>(-1)) {
            sk.grundAbschnitte = sk.sections.size();
        }
        sk.sections.resize(sk.grundAbschnitte);
        sk.anim.anhang = nullptr;
        sk.kinoKarte = karte;
        if (kino == nullptr) {
            continue;
        }
        if (kino->anim.bones.size() != sk.anim.bones.size()) {
            diag::detail("Kino-Skelett " + kinoName + ": " + std::to_string(kino->anim.bones.size()) +
                         " Knochen, " + name + " hat " + std::to_string(sk.anim.bones.size()) + " - nicht angehaengt");
            continue;
        }
        sk.anim.anhang = &kino->anim;
        for (AnimEntry e : kino->sections) {
            e.firstFrame += sk.anim.numFrames;
            sk.sections.push_back(std::move(e));
        }
        diag::detail("Kino-Skelett " + kinoName + " an " + name + " angehaengt (" +
                     std::to_string(kino->sections.size()) + " Abschnitte)");
    }
}

// rc604 jk2: die NPC-Beschreibungen des Spiels lesen. Jedi Academy / Movie
// Duels: alle ext_data/npcs/*.npc (NPC_LoadParms, code/game/NPC_stats.cpp).
// Jedi Outcast: ext_data/NPCs.cfg, danach ext_data/*.npc (nur dieser Ordner,
// gi.FS_GetFileList("ext_data", ".npc"), codeJK2/game/NPC_stats.cpp:2226-2249),
// der erste Eintrag gewinnt (parseNpcFile mit jk2). Rueckgabe: gelesene Dateien.
namespace {
// Die Archive, in denen readFromArchives "name" suchen wuerde, in seiner
// Reihenfolge (rueckwaerts durch Spielordner und Archive).
std::vector<std::pair<std::string, Pk3Entry>> npcArchive(const std::string& name) {
    std::vector<std::pair<std::string, Pk3Entry>> liste;
    for (auto gp = g_app->gamePaths.rbegin(); gp != g_app->gamePaths.rend(); ++gp) {
        for (auto a = gp->archives.rbegin(); a != gp->archives.rend(); ++a) {
            if (const Pk3Entry* e = a->find(name)) { liste.emplace_back(a->path, *e); }
        }
    }
    return liste;
}
}  // namespace

NpcLeseAuftrag npcLeseAuftrag() {
    NpcLeseAuftrag auftrag;
    if (regelwerkJediOutcast()) {
        auftrag.jk2 = true;
        auftrag.dateien.push_back({"ext_data/NPCs.cfg", true, npcArchive("ext_data/NPCs.cfg")});
        for (const GamePath& gp : g_app->gamePaths) {
            for (const FoundFile& f : findByExtension(gp, {".npc"})) {
                std::string n = f.name;
                for (char& c : n) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
                if (n.rfind("ext_data/", 0) != 0 || n.find('/', 9) != std::string::npos) { continue; }
                auftrag.dateien.push_back({f.name, true, npcArchive(f.name)});
            }
        }
        return auftrag;
    }
    for (const GamePath& gp : g_app->gamePaths) {
        for (const FoundFile& f : findByExtension(gp, {".npc"})) { auftrag.dateien.push_back({f.name, false, npcArchive(f.name)}); }
    }
    return auftrag;
}

int npcAuftragLesen(const NpcLeseAuftrag& auftrag, NpcMap& m, std::vector<std::string>& meldungen) {
    int dateien = 0;
    for (std::size_t i = 0; i < auftrag.dateien.size(); ++i) {
        const NpcLeseAuftrag::Datei& d = auftrag.dateien[i];
        // readFromArchives: das erste lesbare Archiv gilt
        std::string text;
        bool gelesen = false;
        for (const auto& [pfad, eintrag] : d.archive) {
            Pk3 archiv;
            archiv.path = pfad;
            std::string err;
            if (readPk3File(archiv, eintrag, text, &err)) {
                gelesen = true;
                break;
            }
            meldungen.push_back("Auspacken fehlgeschlagen: " + d.name + " - " + err);
        }
        if (gelesen) {
            parseNpcFile(text, m, d.jk2);
            ++dateien;
        } else if (auftrag.jk2 && i == 0) {
            meldungen.push_back("Jedi Outcast: ext_data/NPCs.cfg nicht gefunden");
        }
    }
    return dateien;
}

int npcDateienLesen(NpcMap& m) {
    std::vector<std::string> meldungen;
    const int dateien = npcAuftragLesen(npcLeseAuftrag(), m, meldungen);
    for (const std::string& z : meldungen) {
        if (z.rfind("Auspacken", 0) == 0) { diag::warn(z); } else { diag::info(z); }
    }
    return dateien;
}

void loadActorModels() {
    diag::Step step("Figurenmodelle suchen");
    static const std::shared_ptr<const App::ActorAssets> kOhneModell = std::make_shared<const App::ActorAssets>();
    g_app->actorAssets.assign(g_app->scene.actors.size(), kOhneModell);
    if (g_app->scene.actors.empty()) {
        return;
    }
    if (g_app->gamePaths.empty()) {
        rescanGamePaths();
    }

    // --- Woher kommt die Zeit? -------------------------------------------
    //
    // "Figurenmodelle suchen" dauerte beim Anwender 14,6 s, und das ist der
    // groesste Einzelposten beim Laden einer Mission. Bevor daran etwas
    // geaendert wird, muss klar sein, WELCHER Teil davon es ist - sonst
    // baut man einen Zwischenspeicher fuer die falsche Haelfte.
    //
    // Drei Kandidaten, hier einzeln aufsummiert:
    //   die .npc-Dateien lesen (242 Stueck ueber zwei Spielordner),
    //   die .glm-Modelle lesen (aus den Archiven, also mit Entpacken),
    //   die Skelette lesen (.gla, im Protokoll mit 48000 Bildern).
    //
    // Einzelne Schritte je Figur waeren unlesbar; deshalb Summen.
    using Uhr = std::chrono::steady_clock;
    double msNpc = 0.0;
    double msGlm = 0.0;
    double msTex = 0.0;
    double msGla = 0.0;
    const auto zeitAb = [](Uhr::time_point t0) {
        return std::chrono::duration<double, std::milli>(Uhr::now() - t0)
            .count();
    };

    // Die NPC-Beschreibungen einmal einlesen.
    const auto tNpc = Uhr::now();
    if (!g_app->npcMapRead) {
        g_app->npcMapRead = true;
        // Ueber ALLE Spielordner, nicht nur den zuletzt gewaehlten. Was
        // wo gefunden wurde, kommt ins Protokoll - sonst laesst sich
        // hinterher nicht sagen, ob ein Ordner ueberhaupt durchsucht wurde.
        // rc604 jk2: ueber npcDateienLesen (Jedi Outcast: NPCs.cfg).
        const int files = npcDateienLesen(g_app->npcMap);
        diag::info(std::to_string(files) + (regelwerkJediOutcast() ? " NPC-Dateien (NPCs.cfg, Jedi Outcast) in "
                                                                   : " .npc-Dateien in ") +
                   std::to_string(g_app->gamePaths.size()) + " Spielordnern -> " +
                   std::to_string(g_app->npcMap.size()) + " NPC-Typen");
        // Der Typ "player" (NPC_Player-Spawner und die Spielerfigur selbst)
        // steht in keiner .npc: NPC_ParseParms nimmt dafuer die Cvars des
        // Spielers - g_char_model "jedi_hm", g_saber "single_1"
        // (g_main.cpp:801, 808). Deren Vorgaben, solange keine .npc
        // "player" etwas anderes sagt.
        // "playerModel player" in einer .npc heisst dasselbe: das Modell des
        // Spielers (NPC_stats.cpp:1364: Q_stricmp("player", ...) ->
        // g_char_model). Movie Duels hat so einen Eintrag - behaved suchte
        // vorher models/players/player/ und fand nichts (270 Szenen ohne
        // Spielerfigur im Szenenlauf vom 27.09.).
        // rc604 jk2: in Jedi Outcast ist der Spieler immer "kyle"
        // (G_SetG2PlayerModel(ent, "kyle", ...), codeJK2/game/g_client.cpp:1414,
        // 1727) - es gibt kein g_char_model.
        const char* const spielerModell = regelwerkJediOutcast() ? "kyle" : "jedi_hm";
        for (auto& [typ, def] : g_app->npcMap) {
            (void)typ;
            std::string pm = def.playerModel;
            for (char& c : pm) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
            if (pm == "player") {
                def.playerModel = spielerModell;
            }
        }
        // --- Fahrzeuge: das Modell aus ext_data/vehicles/*.veh -------------
        //
        // NPC_ParseParms nimmt bei CLASS_VEHICLE g_vehicleInfo[...].model
        // (NPC_stats.cpp:4562 ff.), gefunden ueber den NPC_type als `name`
        // der .veh (BG_VehicleGetIndex). Der Skin: bei einer Liste der, den
        // soundSet nennt, sonst ein zufaelliger - hier der erste.
        {
            std::map<std::string, std::pair<std::string, std::string>> fahrzeuge;   // name -> model, skin
            for (const GamePath& gp : g_app->gamePaths) {
                for (const FoundFile& f : findByExtension(gp, {".veh"})) {
                    std::string text;
                    if (!readFromArchives(f.name, text)) {
                        continue;
                    }
                    std::string vname;
                    std::string vmodel;
                    std::string vskin;
                    std::istringstream is(text);
                    std::string zeile;
                    while (std::getline(is, zeile)) {
                        std::istringstream zs(zeile);
                        std::string k;
                        std::string v;
                        zs >> k >> v;
                        for (char& c : k) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
                        if (v.size() >= 2 && v.front() == '"' && v.back() == '"') { v = v.substr(1, v.size() - 2); }
                        if (k == "name" && vname.empty()) { vname = v; }
                        if (k == "model" && vmodel.empty()) { vmodel = v; }
                        if (k == "skin" && vskin.empty()) { vskin = v; }
                    }
                    for (char& c : vname) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
                    if (!vname.empty() && !vmodel.empty()) {
                        fahrzeuge[vname] = {vmodel, vskin.substr(0, vskin.find('|'))};
                    }
                }
            }
            for (auto& [typ, def] : g_app->npcMap) {
                if (!def.playerModel.empty()) {
                    continue;
                }
                std::string kl = def.klasse;
                for (char& c : kl) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
                if (kl != "CLASS_VEHICLE") {
                    continue;
                }
                const auto v = fahrzeuge.find(typ);
                if (v == fahrzeuge.end()) {
                    continue;
                }
                def.playerModel = v->second.first;
                def.customSkin = v->second.second.empty() ? "default" : v->second.second;
            }
            diag::info(std::to_string(fahrzeuge.size()) + " Fahrzeuge (.veh)");
        }
        if (g_app->npcMap.find("player") == g_app->npcMap.end() && regelwerkJediOutcast()) {
            // rc604 jk2: der Spieler von Jedi Outcast - NPC_type "kyle",
            // NPC_class CLASS_KYLE (g_client.cpp:1620-1624), Modell kyle
            // (:1727), TEAM_PLAYER (:1626), saberColor SABER_BLUE (:1666).
            // Das Schwert kommt in JK2 aus der Klasse (WP_SaberInitBladeData).
            NpcDef spieler;
            const auto kyle = g_app->npcMap.find("kyle");
            if (kyle != g_app->npcMap.end()) { spieler = kyle->second; }
            spieler.playerModel = "kyle";
            spieler.customSkin = "default";
            spieler.klasse = "CLASS_KYLE";
            spieler.team = "TEAM_PLAYER";
            spieler.weapon = "WP_SABER";
            spieler.saberFarbe = {};
            spieler.saberFarbe[0][0] = "blue";
            g_app->npcMap["player"] = spieler;
        }
        if (g_app->npcMap.find("player") == g_app->npcMap.end()) {
            NpcDef spieler;
            spieler.playerModel = "jedi_hm";
            spieler.saber = "single_1";
            spieler.weapon = "WP_SABER";
            g_app->npcMap["player"] = spieler;
        }

        // --- Und die Klingen -------------------------------------------
        //
        // Gemeldet: "weder die Lichtschwerter werden geladen und
        // ordentlich angezeigt als auch die Sounds."
        //
        // Derselbe Weg wie bei den .npc-Dateien, eine Stufe weiter: der
        // `saber`-Schluessel eines NPC nennt einen Eintrag in einer
        // .sab-Datei, und der traegt Modell, Klaenge, Farbe und Laenge.
        int sabs = 0;
        for (const GamePath& gp : g_app->gamePaths) {
            for (const FoundFile& f : findByExtension(gp, {".sab"})) {
                std::string text;
                if (readFromArchives(f.name, text)) {
                    parseSaberFile(text, g_app->saberMap);
                    ++sabs;
                }
            }
        }
        diag::info(std::to_string(sabs) + " .sab-Dateien -> " +
                   std::to_string(g_app->saberMap.size()) + " Klingen");

        // --- Und die Waffen ---------------------------------------------
        //
        // Gefragt: "wo bekommen wir das Projektil her?" Von hier.
        // ext_data/weapons.dat nennt je Waffe das Modell und den
        // Muendungsblitz; der Blitz ist eine gewoehnliche .efx, und das
        // Modell traegt den Tag "tag_flash", an dem die Muendung sitzt.
        std::string wdat;
        if (readFromArchives("ext_data/weapons.dat", wdat)) {
            parseWeaponsDat(wdat, g_app->weaponMap);
        }
        int mitBlitz = 0;
        int ohneBlitz = 0;
        for (const auto& [typ, def] : g_app->weaponMap) {
            (void)typ;
            if (def.muzzleEffect.empty()) { ++ohneBlitz; } else { ++mitBlitz; }
        }
        diag::info(std::to_string(g_app->weaponMap.size()) +
                   " Waffen aus weapons.dat, " + std::to_string(mitBlitz) +
                   " mit Muendungsblitz, " + std::to_string(ohneBlitz) +
                   " ohne");
        if (g_app->weaponMap.empty()) {
            diag::info("   ext_data/weapons.dat NICHT GEFUNDEN - ohne sie "
                       "gibt es keine Muendungsblitze und keine Geschosse");
        }
    }
    msNpc = zeitAb(tNpc);

    // --- Vorwaermen in drei Stufen ---------------------------------------
    //
    // Gemessen: Texturen 1265 ms, Skelette 948 ms - beides der Reihe nach.
    // Beides ist Lesen und Entziffern ohne gemeinsamen Zustand, also
    // verteilbar; derselbe Griff wie bei den Kartentexturen in rc189.
    //
    // Drei Stufen, und die Reihenfolge ist der Kern:
    //
    //   1. Modelle lesen (verteilt). Erst daraus weiss man, WELCHE Texturen
    //      und WELCHES Skelett gebraucht werden.
    //   2. Die dabei gesammelten Namen VEREINIGEN.
    //   3. Erst dann verteilt entziffern - je Name genau einmal.
    //
    // Warum nicht gleich alles je Modell parallel? Weil sich zwoelf Modelle
    // vier Skelette teilen. Zwoelf Faeden wuerden dieselbe .gla mehrfach
    // laden - und das waere LANGSAMER als der Reihe nach. Erst vereinigen,
    // dann verteilen.
    //
    // Die Ergebnisse landen in denselben Tabellen, die der Hauptdurchgang
    // danach abfragt; er findet also alles fertig vor. Die Modelle werden
    // dabei ein zweites Mal gelesen (49 ms fuer alle), und das ist der
    // Preis dafuer, dass es nur EINE Fassung des Zusammenbaus gibt.
    {
        // Stufe 0: welche Verzeichnisse ueberhaupt? Billig, der Reihe nach.
        std::vector<std::string> dirs;
        for (const Actor& a : g_app->scene.actors) {
            if (a.npcType.empty()) {
                continue;
            }
            std::string k = a.npcType;
            for (char& c : k) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            const auto itn = g_app->npcMap.find(k);
            if (itn == g_app->npcMap.end() || itn->second.playerModel.empty()) {
                continue;   // auch MD3-Figuren (legsmodel) - siehe prepareScene
            }
            const std::string dir = "models/players/" + itn->second.playerModel;
            // Der Schluessel traegt den SKIN mit.
            //
            // Gemeldet: "was noch fehlt, sind richtige Skinvarianten von den
            // Modellen."
            //
            // In assets1 nennen 73 NPC-Typen einen eigenen Skin, und
            // darunter ist DASSELBE Modell mit verschiedenen: cultist in
            // braun und rot, reborn_new in blau und rot. Ein Puffer, der
            // nur nach dem Modellnamen schluesselt, gibt dem zweiten den
            // Skin des ersten - und zwar dem, der zufaellig spaeter dran
            // ist.
            const std::string schluessel = dir + "|" + itn->second.customSkin;
            if (g_app->modelCache.count(schluessel) != 0U) {
                continue;
            }
            if (std::find(dirs.begin(), dirs.end(), schluessel) ==
                dirs.end()) {
                dirs.push_back(schluessel);
            }
        }

        // Ein Arbeitstrupp, der eine Aufgabe ueber einen gemeinsamen Zaehler
        // abarbeitet - wie beim Vorwaermen der Kartentexturen.
        const auto trupp = [](std::size_t auftraege,
                              const std::function<void(std::size_t, std::size_t)>& tun) {
            if (auftraege == 0) {
                return std::size_t{0};
            }
            const unsigned kerne =
                std::max(1U, std::thread::hardware_concurrency());
            const std::size_t faeden =
                std::min<std::size_t>(kerne, auftraege);
            std::atomic<std::size_t> naechste{0};
            const auto lauf = [&](std::size_t fach) {
                for (;;) {
                    const std::size_t k = naechste.fetch_add(1);
                    if (k >= auftraege) {
                        return;
                    }
                    tun(k, fach);
                }
            };
            std::vector<std::thread> mannschaft;
            mannschaft.reserve(faeden);
            for (std::size_t f = 1; f < faeden; ++f) {
                mannschaft.emplace_back(lauf, f);
            }
            lauf(0);
            for (std::thread& th : mannschaft) {
                th.join();
            }
            return faeden;
        };

        // Stufe 1: die Modelle lesen - nur, um an die Namen zu kommen.
        std::vector<GlmModel> modelle(dirs.size());
        std::vector<char> heil(dirs.size(), 0);
        trupp(dirs.size(), [&](std::size_t k, std::size_t) {
            std::string data;
            // dirs[] traegt jetzt "Pfad|Skin". Fuer die Datei zaehlt nur
            // der Pfad; der Skin entscheidet gleich, WELCHE .skin gelesen
            // wird.
            const std::size_t strich = dirs[k].find('|');
            const std::string pfad = dirs[k].substr(0, strich);
            const std::string skinName =
                (strich == std::string::npos) ? std::string("default")
                                              : dirs[k].substr(strich + 1);
            if (!readFromArchives(pfad + "/model.glm", data)) {
                return;
            }
            std::string err;
            if (!readGlm(data, modelle[k], &err)) {
                return;
            }
            // Die HAUT dazu - und genau hier lag der Fehler im ersten
            // Anlauf.
            //
            // Die Texturnamen stehen nicht im Modell, sondern in der
            // .skin: erst applySkin() traegt sie in die Flaechen ein. Ohne
            // diesen Schritt waren alle Namen leer, es gab nichts zu
            // vereinigen, und das Protokoll sagte es klar - "0 Bilder, 0
            // Faeden". Die Skelette wurden trotzdem vorgewaermt, weil ihr
            // Pfad im Kopf des Modells steht.
            //
            // Dieselbe Reihenfolge wie im Hauptdurchgang: erst
            // model_default.skin, sonst model.skin.
            // Erst der genannte Skin, dann default, dann model.skin.
            //
            // Die Reihenfolge ist die der Engine: sie baut
            // model_<customSkin>.skin (NPC_stats.cpp:1908) und faellt auf
            // die Vorgabe "default" zurueck, wenn keiner genannt ist
            // (ebenda:1597). Der dritte Schritt ist unserer - alte Modelle
            // haben nur model.skin.
            for (const std::string& which :
                 {"model_" + skinName + ".skin",
                  std::string("model_default.skin"),
                  std::string("model.skin")}) {
                std::string skin;
                if (readFromArchives(pfad + "/" + which, skin)) {
                    applySkin(skin, modelle[k]);
                    break;
                }
            }
            heil[k] = 1;
        });

        // Stufe 2: vereinigen.
        std::vector<std::string> texNamen;
        std::vector<std::string> glaNamen;
        for (std::size_t k = 0; k < dirs.size(); ++k) {
            if (heil[k] == 0) {
                continue;
            }
            for (const auto& sf : modelle[k].surfaces) {
                if (sf.texture.empty()) {
                    continue;
                }
                if (std::find(texNamen.begin(), texNamen.end(), sf.texture) ==
                    texNamen.end()) {
                    texNamen.push_back(sf.texture);
                }
            }
            std::string an = modelle[k].animFile;
            if (an.empty()) {
                an = "models/players/_humanoid/_humanoid";
            }
            if (g_app->skeletons.count(an) == 0U &&
                std::find(glaNamen.begin(), glaNamen.end(), an) ==
                    glaNamen.end()) {
                glaNamen.push_back(an);
            }
        }

        // Stufe 3a: die Texturen - je Name genau einmal.
        std::vector<std::pair<std::string, TextureSet::Tex>> texErgebnis(
            texNamen.size());
        std::vector<char> texOk(texNamen.size(), 0);
        // ABSICHTLICH nur die direkten Kandidaten, nicht der Umweg ueber
        // die Shadertabelle. Die wird moeglicherweise erst beim ersten
        // Zugriff aufgebaut, und ein Wettlauf darauf waere teurer als die
        // wenigen Bilder, die dann im Hauptdurchgang der Reihe nach
        // aufgeloest werden - im Protokoll die als "ueber Shaderskripte"
        // gezaehlten, bei md_am_sith fuenf Stueck.
        const std::size_t texFaeden =
            trupp(texNamen.size(), [&](std::size_t k, std::size_t) {
                const std::string key =
                    image::mappingName(texNamen[k]) + "@256";
                // Schon da? Dann nicht noch einmal entziffern - derselbe
                // Fehler wie bei den Kartentexturen, nur hier.
                if (g_app->texCache.count(key) != 0U) {
                    return;
                }
                for (const std::string& name : textureCandidates(texNamen[k])) {
                    std::string data;
                    if (!readFromArchives(name, data)) {
                        continue;
                    }
                    const image::Image im = image::decode(
                        reinterpret_cast<const unsigned char*>(data.data()),
                        data.size());
                    if (!im.ok || im.width <= 0 || im.height <= 0) {
                        continue;
                    }
                    TextureSet::Tex t;
                    image::shrinkTo(im, 256, t.width, t.height, t.rgba);
                    texErgebnis[k] = {key, std::move(t)};
                    texOk[k] = 1;
                    return;
                }
            });
        std::size_t texNeu = 0;
        for (std::size_t k = 0; k < texNamen.size(); ++k) {
            if (texOk[k] != 0 &&
                g_app->texCache.emplace(texErgebnis[k].first,
                                        texErgebnis[k].second).second) {
                ++texNeu;
            }
        }

        // Stufe 3b: die Skelette - vier grosse .gla, vier Faeden.
        std::vector<GlaAnimation> glaErgebnis(glaNamen.size());
        std::vector<std::string> glaArchiv(glaNamen.size());
        std::vector<char> glaOk(glaNamen.size(), 0);
        trupp(glaNamen.size(), [&](std::size_t k, std::size_t) {
            std::string data;
            if (!readFromArchives(glaNamen[k] + ".gla", data, &glaArchiv[k])) {
                return;
            }
            std::string err;
            if (readGla(data, glaErgebnis[k], &err)) {
                glaOk[k] = 1;
            }
        });
        for (std::size_t k = 0; k < glaNamen.size(); ++k) {
            if (glaOk[k] == 0) {
                continue;
            }
            App::Skeleton sk;
            sk.anim = std::move(glaErgebnis[k]);
            sk.sections = animSectionsFor(glaNamen[k]);
            diag::info("Skelett " + glaNamen[k] + ": " +
                       std::to_string(sk.anim.bones.size()) + " Knochen, " +
                       std::to_string(sk.anim.numFrames) + " Bilder, " +
                       std::to_string(sk.sections.size()) + " Abschnitte, aus " +
                       glaArchiv[k]);
            g_app->skeletons[glaNamen[k]] = std::move(sk);
        }

        if (!dirs.empty()) {
            diag::info("Vorgewaermt: " + std::to_string(dirs.size()) +
                       " Modelle, " + std::to_string(texNeu) + " Bilder, " +
                       std::to_string(glaNamen.size()) + " Skelette, " +
                       std::to_string(texFaeden) + " Faeden");
        }
    }

    int found = 0;
    // Mitzaehlen, WO die Kette reisst:
    //   NPC_type fehlt -> kein Eintrag in den .npc -> kein Modell im Archiv
    //   -> Modell nicht lesbar
    int noType = 0;
    int noNpc = 0;
    int noModel = 0;
    int badModel = 0;
    for (std::size_t i = 0; i < g_app->scene.actors.size(); ++i) {
        const Actor& a = g_app->scene.actors[i];
        if (a.npcType.empty()) {
            // MIT NAMEN. "10 von 11 Figuren mit Modell" sagt nicht, WELCHE
            // fehlt - und genau die will man nachsehen. Der Grund steht
            // dabei, damit klar ist, an welcher Stelle die Kette reisst.
            diag::info("ohne Modell: " + a.name +
                       " - kein NPC_type (kein NPC_*-Spawner mit diesem "
                       "Namen in der Karte)");
            ++noType;
            continue;
        }
        std::string key = a.npcType;
        for (char& c : key) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        const auto it = g_app->npcMap.find(key);
        if (it == g_app->npcMap.end()) {
            diag::info("ohne Modell: " + a.name + " - NPC_type \"" + a.npcType +
                       "\" steht in keiner .npc");
            ++noNpc;
            continue;
        }
        if (it->second.playerModel.empty() && !it->second.legsModel.empty()) {
            // Eine MD3-Figur: sie zeichnet der Kartenmodell-Weg (prepareScene).
            diag::detail("Figur " + a.name + ": MD3-Teile models/players/" + it->second.legsModel +
                         "/lower.md3");
            continue;
        }
        const std::string dir = "models/players/" + it->second.playerModel;

        // Schon geladen? Dann nur noch abschreiben.
        //
        // In einer Massenszene sind die meisten Figuren dasselbe Modell.
        // Im Protokoll von shank stand rund zwanzigmal hintereinander "42
        // gefunden, 0 nicht gefunden" - jedes Mal derselbe Sturmtruppler,
        // neu gelesen, neu entpackt, neu verkleinert.
        const std::string schluessel = dir + "|" + it->second.customSkin;
        const auto cached = g_app->modelCache.find(schluessel);
        if (cached != g_app->modelCache.end()) {
            g_app->actorAssets[i] = cached->second;
            ++found;
            continue;
        }
        const auto neu = std::make_shared<App::ActorAssets>();
        App::ActorAssets& assets = *neu;
        assets.tried = true;

        std::string data;
        const auto tGlm = Uhr::now();
        if (!readFromArchives(dir + "/model.glm", data)) {
            // Den gesuchten Pfad SAMT Quelle nennen: der NPC-Typ steht im
            // Skript, der Modellname in der .npc, und das Modell in einer
            // dritten Datei. Ohne alle drei Namen ist nicht zu sagen, wo es
            // hakt.
            diag::info("ohne Modell: " + a.name + " - " + a.npcType + " -> " +
                       it->second.playerModel + " -> " + dir + "/model.glm fehlt");
            ++noModel;
            continue;
        }
        GlmModel gm;
        std::string err;
        if (!readGlm(data, gm, &err)) {
            diag::info("ohne Modell: " + a.name + " - " + dir + " nicht lesbar: " + err);
            ++badModel;
            continue;
        }
        // Die uebliche Haut daneben.
        // Erst der genannte Skin, dann default, dann model.skin - dieselbe
        // Reihenfolge wie im Vorwaermen weiter oben. Zwei Stellen, die
        // dasselbe tun, muessen es GLEICH tun; laufen sie auseinander,
        // haengt das Aussehen davon ab, welcher Weg zuerst drankam.
        bool skinGefunden = false;
        for (const std::string& which :
             {"model_" + it->second.customSkin + ".skin",
              std::string("model_default.skin"), std::string("model.skin")}) {
            std::string skin;
            if (readFromArchives(dir + "/" + which, skin)) {
                applySkin(skin, gm);
                skinGefunden = true;
                if (which != "model_" + it->second.customSkin + ".skin") {
                    diag::detail("Figur " + a.name + ": Skin \"" +
                                 it->second.customSkin + "\" nicht gefunden, " +
                                 which + " genommen");
                }
                break;
            }
        }
        if (!skinGefunden) {
            diag::detail("Figur " + a.name + ": KEINE .skin unter " + dir +
                         " - die Flaechen bleiben ohne Texturnamen");
        }
        assets.model = std::move(gm);
        msGlm += zeitAb(tGlm);
        // Texturen dazu, ueber denselben Weg wie beim einzelnen Modell.
        const auto tTex = Uhr::now();
        assets.textures = texturesFor(assets.model);
        msTex += zeitAb(tTex);
        // Und das Skelett, dessen Pfad im Kopf des Modells steht.
        const auto tGla = Uhr::now();
        assets.skeleton = skeletonFor(assets.model);
        msGla += zeitAb(tGla);
        // --- Knochenzahl von Modell und Skelett -------------------------
        //
        // R_LoadMDXM (tr_ghoul2.cpp:3683) laedt ein Modell NICHT, dessen
        // Knochenzahl nicht zur .gla passt ("has different bones than
        // anim") - ausser es ist ein JK2-Modell, das umgelegt wird (das tut
        // readGlm). Gefunden mit tests/meshmasse.cpp: models/players/sbd
        // aus zzzz_zz_MD_Models_PT.pk3 nennt _humanoid (53) und hat 43.
        // behaved zeichnet es trotzdem, sagt es aber - so wie die Engine.
        if (assets.skeleton != nullptr && !assets.model.empty() &&
            static_cast<int>(assets.skeleton->anim.bones.size()) != assets.model.numBones) {
            diag::info("Modell " + assets.model.name + " hat " + std::to_string(assets.model.numBones) +
                       " Knochen, sein Skelett " + assets.model.animFile + " " +
                       std::to_string(assets.skeleton->anim.bones.size()) +
                       " - die Engine laedt es so nicht (R_LoadMDXM: different bones than anim)");
        }
        // --- Welche Figur welches Skelett bekommen hat ------------------
        //
        // Gemeldet: "die Jango-Figur hat keine Animation, sie steht in der
        // Root-Pose fest."
        //
        // Das Protokoll zeigte bisher NUR, welche Skelette geladen wurden -
        // nicht, welche Figur welches abbekommt. Damit liess sich der Fall
        // nicht auseinanderhalten:
        //
        //   kein Skelett          -> gezeichnet wird die Grundstellung des
        //                            .glm, und das IST die Root-Pose
        //   Skelett, aber der
        //   Animationsname fehlt
        //   in seiner .cfg        -> frameForAnimationIn gibt -1 zurueck,
        //                            und wir fallen auf die vorige zurueck
        //
        // Beides sieht auf dem Bildschirm gleich aus und hat verschiedene
        // Ursachen. Die Zeile hier nennt Ross und Reiter.
        //
        // Ins Detailprotokoll, nicht ins normale: bei 64 Figuren waeren das
        // 64 Zeilen, und im Normalfall interessiert keine davon.
        if (assets.skeleton == nullptr) {
            diag::detail("Figur " + a.name + " (" + dir +
                         "): KEIN Skelett - wird in der Grundstellung "
                         "gezeichnet. Kopf des .glm nennt \"" +
                         assets.model.animFile + "\"");
        } else {
            diag::detail("Figur " + a.name + " (" + dir + "): Skelett \"" +
                         assets.model.animFile + "\", " +
                         std::to_string(assets.skeleton->sections.size()) +
                         " Abschnitte");
        }
        g_app->modelCache[schluessel] = neu;
        g_app->actorAssets[i] = neu;
        ++found;
    }
    // rc604 fix604: beim Kartenwechsel die Modelle freigeben, die keine Figur
    // mehr nennt (nur noch modelCache haelt sie) - sie sind schnell wieder
    // gelesen. Bis rc603 blieb jedes Modell jeder Karte bis zum Beenden.
    if (g_app->modelCacheKarte != g_app->map.path) {
        g_app->modelCacheKarte = g_app->map.path;
        std::size_t frei = 0;
        for (auto it = g_app->modelCache.begin(); it != g_app->modelCache.end();) {
            if (it->second.use_count() == 1) {
                it = g_app->modelCache.erase(it);
                ++frei;
            } else {
                ++it;
            }
        }
        if (frei != 0U) {
            diag::detail("Figurenmodelle: " + std::to_string(frei) + " der vorigen Karte freigegeben, " +
                         std::to_string(g_app->modelCache.size()) + " bleiben");
        }
    }
    diag::info(std::to_string(found) + " von " +
               std::to_string(g_app->scene.actors.size()) + " Figuren mit Modell");
    {
        char zeiten[192];
        std::snprintf(zeiten, sizeof(zeiten),
                      "Zeit: .npc %.0f ms, .glm %.0f ms, Texturen %.0f ms, "
                      "Skelette %.0f ms",
                      msNpc, msGlm, msTex, msGla);
        diag::info(zeiten);
    }

    // Den Grund im Klartext merken, statt eines pauschalen Hinweises.
    //
    // "Keine Modelle gefunden - Spielordner hinzufuegen" ist nutzlos, wenn
    // Spielordner da sind. Die Kette hat vier Glieder, und welches gerissen
    // ist, sagt genau, was zu tun waere.
    char why[224];
    if (found > 0) {
        std::snprintf(why, sizeof(why), tr(Str::ActorsFound), found,
                      static_cast<int>(g_app->scene.actors.size()));
    } else if (g_app->npcMap.empty()) {
        std::snprintf(why, sizeof(why), "%s", tr(Str::ActorsNoNpcFiles));
    } else if (noType > 0 && noNpc == 0 && noModel == 0) {
        std::snprintf(why, sizeof(why), tr(Str::ActorsNoType), noType);
    } else if (noNpc > 0) {
        std::snprintf(why, sizeof(why), tr(Str::ActorsNoNpcEntry), noNpc,
                      static_cast<int>(g_app->npcMap.size()));
    } else if (noModel > 0) {
        std::snprintf(why, sizeof(why), tr(Str::ActorsNoGlm), noModel);
    } else if (badModel > 0) {
        std::snprintf(why, sizeof(why), tr(Str::ActorsBadGlm), badModel);
    } else if (g_app->scene.actors.empty()) {
        // GAR KEINE Figuren - bisher der einzige Fall, der nichts sagte.
        //
        // Eine Figur entsteht aus einem affect-Block. Hat das Skript keinen,
        // gibt es nichts zu zeigen, und das ist kein Fehler.
        std::snprintf(why, sizeof(why), "%s", tr(Str::ActorsNoAffect));
    } else {
        // Alle Faelle sind oben abgedeckt; bleibt trotzdem einer uebrig,
        // soll er sichtbar werden statt still zu verschwinden.
        std::snprintf(why, sizeof(why), tr(Str::ActorsFound), found,
                      static_cast<int>(g_app->scene.actors.size()));
    }
    g_app->actorWhy = why;
    // `actorWhy` ist ANZEIGETEXT und folgt der Sprache der Oberflaeche.
    // In shanks rc549-Protokoll stand deshalb mitten im deutschen Ablauf
    // eine englische Zeile:
    //
    //     Figurenmodelle: 27 of 31 figures have a model
    //
    // Das Protokoll ist sonst durchgehend fest deutsch. Der Zusatz sagt,
    // warum diese eine Zeile die Sprache wechselt - sonst sucht man den
    // Fehler dort, wo keiner ist.
    diag::info("Figurenmodelle (Anzeigetext, Sprache der Oberflaeche): " +
               g_app->actorWhy);

    // Wenn KEINE Figur einen NPC_type hat, liegt es fast nie am Skript,
    // sondern an den Entities. Die drei Zahlen hier sagen, an welcher.
    //
    // Der Fall, der dazu gefuehrt hat: ein Protokoll meldete "0 von 64
    // Figuren mit Modell", und das legte den Verdacht auf fehlende .glm -
    // dabei kam die Kette schon eine Stufe frueher nicht in Gang. Ohne
    // diese Zeilen liess sich das aus der Ferne nicht unterscheiden.
    if (noType > 0 && found == 0) {
        int npcEnts = 0;
        int withType = 0;
        int plainSpawner = 0;
        for (const MapEntity& e : g_app->map.entities) {
            if (e.classname.rfind("NPC_", 0) != 0) {
                continue;
            }
            ++npcEnts;
            if (e.classname == "NPC_spawner") {
                ++plainSpawner;
            }
            if (!effektiverNpcTyp(e).empty()) {   // rc602: auch aus der Klasse
                ++withType;
            }
        }
        diag::info("   Entities der Karte: " +
                   std::to_string(g_app->map.entities.size()) + ", davon NPC_*: " +
                   std::to_string(npcEnts) + " (NPC_spawner: " +
                   std::to_string(plainSpawner) + "), mit NPC_type: " +
                   std::to_string(withType));
        if (npcEnts == 0) {
            diag::info("   kein einziges NPC_* - stehen die Figuren in einer "
                       ".ent neben der .bsp? Dann wurde sie nicht geladen.");
        }
    }
    g_app->mapDirty = true;
}

// Das Skelett zum geladenen Modell holen.
//
// Der Pfad steht im Modell selbst - "models/players/_humanoid/_humanoid".
// Wir suchen erst dort in den Archiven und fragen nur nach, wenn nichts zu
// finden ist. Die animation.cfg liegt daneben und nennt die Abschnitte.
void loadSkeletonForModel() {
    diag::Step step("Skelett laden");
    if (g_app->model.animFile.empty()) {
        return;
    }
    std::string data;
    const std::string gla = g_app->model.animFile + ".gla";
    if (!readFromArchives(gla, data)) {
        const std::string p = platform::openFileDialog(
            tr(Str::ModelLoadGla), "Ghoul2 skeleton (*.gla)|*.gla|All files (*.*)|*.*",
            g_app->settings.lastDir);
        if (p.empty()) {
            return;
        }
        data = slurp(p);
        // Die animation.cfg liegt daneben.
        const std::string cfg = slurp(directoryOf(p) + "/animation.cfg");
        if (!cfg.empty()) {
            g_app->animList = parseAnimationCfg(cfg);
        }
    } else {
        // Dieselbe Reihenfolge wie bei den Figuren - erst <name>/<name>.cfg,
        // dann <name>/animation.cfg. Vorher stand hier nur der zweite Weg,
        // und damit gab es zwei verschiedene Regeln im selben Programm.
        g_app->animList = animSectionsFor(g_app->model.animFile);
    }

    GlaAnimation a;
    std::string err;
    if (!readGla(data, a, &err)) {
        step.fail(err);
        platform::showError(gla + "\n" + err, tr(Str::AppTitle));
        return;
    }
    diag::info(std::to_string(a.bones.size()) + " Knochen, " +
               std::to_string(a.numFrames) + " Bilder, " +
               std::to_string(g_app->animList.size()) + " Abschnitte");
    g_app->anim = std::move(a);
    g_app->animIndex = -1;
    g_app->animFrame = 0;
    g_app->modelDirty = true;
}

// Die Modellansicht.
//
// Ein .glm zeigt eine Figur in ihrer RUHELAGE. Die Bewegung steht in der
// .gla, deren Pfad im Kopf des Modells steht - bei model.glm ist das
// "models/players/_humanoid/_humanoid". Ohne sie ist das Modell ein
// Standbild; das steht auch so unter der Ansicht, damit niemand ein fehlendes
// Merkmal fuer einen Fehler haelt.
// Die Werkzeugleiste der Modellansicht. Steht ueber dem Rahmen, damit die
// Ansicht auf gleicher Hoehe mit dem Skriptfenster beginnt.

void drawModelView() {
    // Ohne Modell nur der Hinweis - die Bedienleiste steht trotzdem, damit
    // die Anordnung nicht springt, sobald etwas geladen wird.
    const bool haveModel = !g_app->model.empty();
    // Platz fuer die Bedienleiste unten freihalten.
    // Kein Platz mehr fuer die Bedienung reservieren: sie steht jetzt UNTER
    // allen drei Spalten und nicht mehr hier. Das Bild bekommt die ganze
    // Hoehe der Spalte.
    const float panelH = 0.0F;
    if (!haveModel) {
        const ImVec2 free = ImGui::GetContentRegionAvail();
        ImGui::BeginChild("modelempty",
                          ImVec2{free.x, std::max(free.y - panelH, 40.0F)});
        ImGui::TextDisabled("%s", tr(Str::ModelNone));
        ImGui::EndChild();
        return;
    }
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const int w = std::max(64, static_cast<int>(avail.x));
    const int h = std::max(64, static_cast<int>(avail.y - panelH));

    // Die Ansicht in einen EIGENEN Bereich: nur hier wirkt die Maus. Sonst
    // zoomt das Rad auch dann, wenn man in der Animationsliste blaettert.
    // OHNE Innenabstand: das Bild ist genau so gross wie sein Kind. Mit
    // Abstand passt es nicht hinein, ImGui setzt einen Rollbalken daneben,
    // und der Rahmen endet sichtbar hoeher als die Spalten rechts davon.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    ImGui::BeginChild("modelcanvas", ImVec2{static_cast<float>(w),
                                            static_cast<float>(h)},
                      ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollWithMouse |
                      ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar();
    const bool hovered = ImGui::IsWindowHovered();
    ImGuiIO& io = ImGui::GetIO();
    if (hovered && ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
        if (io.KeyAlt) {
            g_app->modelOrbit.turn(g_app->modelCam, -io.MouseDelta.x * 0.4F,
                                   -io.MouseDelta.y * 0.4F);
        } else {
            g_app->modelOrbit.pan(g_app->modelCam, io.MouseDelta.x, io.MouseDelta.y);
        }
    }
    if (hovered && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        // Bei einem einzelnen Modell ist die linke Taste frei - da erwartet
        // man das Drehen, wie in jedem Modellbetrachter.
        g_app->modelOrbit.turn(g_app->modelCam, -io.MouseDelta.x * 0.4F,
                               -io.MouseDelta.y * 0.4F);
    }
    if (hovered && io.MouseWheel != 0.0F) {
        g_app->modelOrbit.zoom(g_app->modelCam, io.MouseWheel);
    }

    modellDrehteller();
    if (g_app->modelImage.width != w || g_app->modelImage.height != h) {
        g_app->modelImage.width = w;
        g_app->modelImage.height = h;
        g_app->modelDirty = true;
    }
    const float now[7] = {g_app->modelCam.pos[0], g_app->modelCam.pos[1],
                          g_app->modelCam.pos[2], g_app->modelCam.angles[0],
                          g_app->modelCam.angles[1], g_app->modelCam.angles[2],
                          g_app->modelOrbit.distance};
    for (int k = 0; k < 7; ++k) {
        if (std::fabs(now[k] - g_app->lastModelCam[k]) > 0.0001F) {
            g_app->modelDirty = true;
        }
        g_app->lastModelCam[k] = now[k];
    }

    // --- Ueber die Grafikkarte, in ein eigenes Renderziel -----------------
    //
    // Bis rc568 zeichnete das Modellfenster der Software-Rasterer
    // (renderModel). Jetzt derselbe Figuren-Shader wie in der Karte, mit dem
    // Verlauf des alten Betrachters als Hintergrund.
    if (!gpu::verfuegbar()) {
        ImGui::TextWrapped("%s", tr(Str::MapNeedsD3d));
    } else {
        if (g_app->modelDirty && gpu::bereiteModellZiel(w, h)) {
            std::vector<BoneMatrix> knochen;
            if (!g_app->anim.bones.empty() && g_app->anim.numFrames > 0) {
                std::vector<BoneMatrix> welt;
                g_app->anim.worldMatrices(
                    std::clamp(g_app->animFrame, 0, g_app->anim.numFrames - 1), welt);
                gpu::baueKnochenmatrizen(welt, g_app->anim.bones, knochen);
            }
            float vp[16];
            gpu::baueViewProj(g_app->modelCam, w, h, 1.0F, 8192.0F, vp);
            std::string fehler;
            const int n = gpu::zeichneModellAnsicht(
                g_app->model, (g_app->showModelTextures && g_app->modelTextures.found != 0)
                                  ? &g_app->modelTextures
                                  : nullptr,
                knochen, vp, g_app->showCaps, &fehler);
            if (n == 0 && !fehler.empty()) {
                diag::detail("Modellfenster: " + fehler);
            }
            for (const std::string& m : gpu::debugMeldungen()) {
                diag::detail("Direct3D (Modellfenster): " + m);
            }
            g_app->modelDirty = false;
        }
        if (gpu::modellZielTextur() != nullptr) {
            ImGui::Image(reinterpret_cast<ImTextureID>(gpu::modellZielTextur()),
                         ImVec2{static_cast<float>(w), static_cast<float>(h)});
            modellUeberlagerung(ImGui::GetItemRectMin(), ImVec2{static_cast<float>(w), static_cast<float>(h)});
        }
    }
    ImGui::EndChild();

}

// drawModelToolbar und drawModelPanel stehen seit rc591 in gui/app_modell.cpp.

// Die Skripte, auf die die geladene Karte verweist.
//
// So haengt eine Mission zusammen: die Karte selbst weiss nichts von
// Skripten, aber ihre Entities tun es - target_scriptrunner nennt ein
// usescript, ein NPC_spawner ein deathscript. Bei md_tfoaj_jedi sind das
// sieben, und alle liegen im selben Archiv.
//
// Damit muss niemand mehr raten, welches Skript zu einer Karte gehoert.
void drawMapScriptList() {
    if (g_app->map.scripts.empty()) {
        ImGui::TextDisabled("%s", tr(Str::MapScriptsNone));
        return;
    }
    for (std::size_t i = 0; i < g_app->map.scripts.size(); ++i) {
        const MapData::ScriptRef& r = g_app->map.scripts[i];
        ImGui::PushID(static_cast<int>(i));

        // Liegt die Datei vor? Erst .txt, dann .ibi - beides kommt vor, und
        // die Endung ist mal gross, mal klein geschrieben. Der Archivindex
        // nimmt beides.
        std::string data;
        std::string found;
        for (const char* ext : {".txt", ".icarus", ".ibi"}) {
            if (readFromArchives("scripts/" + r.path + ext, data)) {
                found = ext;
                break;
            }
        }

        ImGui::BeginDisabled(found.empty());
        // withUnsaved statt confirmDiscard.
        //
        // confirmDiscard gab es einmal - eine Windows-MessageBox antwortet
        // sofort, also konnte man ihr Ergebnis in einer Bedingung benutzen.
        // Seit die Frage im eigenen Stil gestellt wird, dauert sie mehrere
        // Bilder, und was danach geschehen soll, wird MITGEGEBEN.
        //
        // Diese eine Stelle war beim Umbau uebersehen worden - sie stand
        // als einzige nicht in app.cpp. Uebersetzt hat es trotzdem, weil
        // die Erklaerung im Kopf blieb; erst der Bindeschritt haette es
        // gemeldet, und der laeuft hier nicht.
        if (ImGui::Button(r.path.c_str()) && !found.empty()) {
            const std::string dat = data;
            const std::string nm = r.path + found;
            withUnsaved([dat, nm] { oeffneSkriptAusSpeicher(dat, nm); });
        }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s von %s", r.key.c_str(), r.owner.c_str());
        }
        if (found.empty()) {
            ImGui::SameLine();
            ImGui::TextDisabled("%s", tr(Str::MapScriptMissing));
        }
        ImGui::PopID();
    }
}

// Eine .pk3 waehlen und die Missionen darin anzeigen.
//
// Das ist der Einstieg, den man sich wuenscht: eine Datei waehlen, und alles
// weitere ergibt sich. Die Kette steht in den Daten, man muss sie nur
// ablaufen - Karte, Entities daneben, Skripte aus den Entities, Figuren aus
// den Skripten, Modelle aus den NPC-Typen.
void loadMissionArchive() {
    const std::string p = platform::openFileDialog(
        tr(Str::LoadMission), "Game archives (*.pk3)|*.pk3|All files (*.*)|*.*",
        g_app->settings.lastDir);
    if (p.empty()) {
        return;
    }
    missionenAusArchiv(p);
}

namespace {
// rc603: die Zeit der letzten Aenderung eines Archivs (fuer den Schluessel).
std::string archivStand(const std::string& pfad) {
    std::error_code ec;
    const auto t = std::filesystem::last_write_time(::bhed::utf8Pfad(pfad), ec);
    return ec ? std::string("?") : std::to_string(static_cast<long long>(t.time_since_epoch().count()));
}

// rc603: der Schluessel einer Karte im Zwischenspeicher missionsVerweise -
// Archiv mit Aenderungszeit, Eintrag, Versatz und Groesse von .bsp und .ent.
// Wird ein Archiv neu gepackt (Save mission), aendert sich seine Zeit.
std::string verweisSchluessel(const MissionsAuftrag& a, const std::string& bspStand, const std::string& entStand) {
    return a.bspArchiv + "@" + bspStand + "|" + a.bsp.name + "|" + std::to_string(a.bsp.localHeaderOffset) + "|" +
           std::to_string(a.bsp.size) + "|" + a.entArchiv + "@" + entStand + "|" + a.ent.name + "|" +
           std::to_string(a.ent.localHeaderOffset) + "|" + std::to_string(a.ent.size);
}

std::string nurDateiname(const std::string& p) {
    const std::size_t s = p.find_last_of("/\\");
    return s == std::string::npos ? p : p.substr(s + 1);
}

DateiGibt skriptGibt() {
    return [](const std::string& n) {
        std::string k = n;
        for (char& c : k) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return g_app->missionsSkriptNamen.count(k) != 0U;
    };
}
}  // namespace

void missionsSucheBeenden() {
    if (g_app->missionsSuche) {
        MissionsSuche& su = *g_app->missionsSuche;
        su.abbrechen();
        // Was schon gelesen ist, in den Zwischenspeicher - das naechste "Load
        // mission" desselben Spiels liest es nicht noch einmal.
        std::size_t behalten = 0;
        for (std::size_t i = 0; i < su.gesamt() && i < g_app->missionsSuchSchluessel.size(); ++i) {
            if (su.bearbeitet()[i] != 0 && su.gelesen()[i] != 0) {
                g_app->missionsVerweise[g_app->missionsSuchSchluessel[i]] = su.verweise()[i];
                ++behalten;
            }
        }
        diag::detail("Missionssuche abgebrochen bei " + std::to_string(su.erledigt()) + " von " +
                     std::to_string(su.gesamt()) + " Karten, " + std::to_string(behalten) + " gemerkt");
        g_app->missionsSuche.reset();
    }
    g_app->missionsSuchKarten.clear();
    g_app->missionsSuchArchive.clear();
    g_app->missionsSuchSchluessel.clear();
}

void missionsSucheUebernehmen() {
    if (!g_app->missionsSuche || !g_app->missionsSuche->fertig()) {
        return;
    }
    const MissionsSuche& su = *g_app->missionsSuche;
    const DateiGibt gibt = skriptGibt();
    std::size_t nichtLesbar = 0;
    for (std::size_t i = 0; i < su.gesamt() && i < g_app->missionsSuchKarten.size(); ++i) {
        // Eine Karte, die sich nicht lesen laesst, bleibt ohne Skripte - wie
        // bisher in findMissions.
        if (su.gelesen()[i] != 0) {
            g_app->missionsVerweise[g_app->missionsSuchSchluessel[i]] = su.verweise()[i];
        } else {
            ++nichtLesbar;
        }
        Mission m = missionAus(g_app->missionsSuchKarten[i], su.verweise()[i], gibt);
        m.archiv = g_app->missionsSuchArchive[i];
        g_app->missions.push_back(std::move(m));
    }
    sortiereMissionen(g_app->missions);
    const double ms =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - g_app->missionsSucheAb).count();
    diag::info("Missionssuche fertig: " + std::to_string(su.gesamt()) + " Karten im Hintergrund gelesen (" +
               std::to_string(static_cast<long>(ms)) + " ms)" +
               (nichtLesbar != 0U ? ", " + std::to_string(nichtLesbar) + " nicht lesbar" : std::string()) + ", " +
               std::to_string(g_app->missions.size()) + " Karten in der Auswahl");
    g_app->missionsSuche.reset();
    g_app->missionsSuchKarten.clear();
    g_app->missionsSuchArchive.clear();
    g_app->missionsSuchSchluessel.clear();
}

void missionenAusArchiv(const std::string& p, const std::string& wunsch) {
    diag::Step step("Mission suchen: " + p);
    g_app->settings.lastDir = directoryOf(p);
    g_app->missionArchive = p;
    missionsSucheBeenden();

    // Erst pruefen, ob es ueberhaupt ein Archiv ist - vor jeder Aenderung an
    // den Spielordnern.
    {
        Pk3 archive;
        std::string err;
        if (!readPk3Directory(p, archive, &err)) {
            step.fail(err);
            platform::showError(p + "\n" + err, tr(Str::AppTitle));
            return;
        }
    }

    // --- rc603: das Spiel des Archivs --------------------------------------
    //
    // Vorher: addGamePathDirectory(directoryOf(p)) - der Ordner kam DAUERHAFT
    // in die Einstellungen, auch der eines anderen Spiels (JK2), und gewann
    // als spaeterer Ordner sogar gegen die eingerichteten. Jetzt bestimmt
    // spielordnerFuerArchiv die Ordner des Spiels; gehoeren sie nicht zu den
    // eingerichteten, gelten sie nur fuer diese Sitzung.
    // rc604: die eingerichteten Ordner DES Spiels, zu dem das Archiv gehoert -
    // eine JK2-pk3 nie auf den Ordnern von Jedi Academy (und umgekehrt).
    const bool archivJk2 = jediOutcastOrdner(ordnerVonDatei(p));
    const std::vector<std::string>& eingerichtet =
        archivJk2 ? g_app->settings.gamePathsJk2 : g_app->settings.gamePaths;
    const SpielordnerWahl wahl =
        spielordnerFuerArchiv(p, eingerichtet, g_app->sitzungsSpielordner, [](const std::string& o) {
            std::error_code ec;
            for (const auto& e : std::filesystem::directory_iterator(::bhed::utf8Pfad(o), ec)) {
                std::string ext = e.path().extension().string();
                for (char& c : ext) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
                if (ext == ".pk3") {
                    return true;
                }
            }
            return false;
        });
    // rc604: auch bei "eingerichtet" die Ordner der Wahl - sie gehoeren
    // vielleicht zum anderen Spiel; gleichen sie den geltenden, endet die
    // Sitzung (setzeSitzungsSpielordner).
    setzeSitzungsSpielordner(wahl.ordner);

    // --- Alle Archive des Spiels, in der Suchreihenfolge der Engine ---------
    //
    // Nur die Namen, die hier zaehlen (maps/, scripts/) - so bleibt die
    // Abschrift klein, auch bei 40 Archiven von Movie Duels.
    std::vector<SpielArchiv> spiel;
    std::vector<const Pk3*> pk3s;
    for (const GamePath& gp : g_app->gamePaths) {
        for (const Pk3& a : gp.archives) {
            SpielArchiv sa;
            sa.pfad = a.path;
            for (const Pk3Entry& e : a.entries) {
                const char c = e.name.empty() ? '\0' : static_cast<char>(std::tolower(static_cast<unsigned char>(e.name[0])));
                if (c == 'm' || c == 's') {
                    sa.namen.push_back(e.name);
                }
            }
            spiel.push_back(std::move(sa));
            pk3s.push_back(&a);
        }
    }
    const std::vector<MissionsKarte> karten = missionsKarten(spiel, p);
    std::vector<std::string> staende;
    staende.reserve(pk3s.size());
    for (const Pk3* a : pk3s) {
        staende.push_back(archivStand(a->path));
    }
    g_app->missionsSkriptNamen = skriptNamen(spiel);
    const DateiGibt gibt = skriptGibt();

    // Je Karte: aus dem Zwischenspeicher, sofort (im gewaehlten Archiv - so
    // viel wie bisher), oder im Hintergrund.
    g_app->missions.clear();
    std::vector<MissionsAuftrag> hinten;
    std::size_t sofort = 0;
    std::size_t gemerkt = 0;
    for (const MissionsKarte& k : karten) {
        MissionsAuftrag a;
        const Pk3& bspArc = *pk3s[k.bspArchiv];
        const Pk3Entry* be = bspArc.find(k.bsp);
        if (be == nullptr) {
            continue;
        }
        a.bspArchiv = bspArc.path;
        a.bsp = *be;
        if (!k.ent.empty()) {
            const Pk3& entArc = *pk3s[k.entArchiv];
            if (const Pk3Entry* ee = entArc.find(k.ent)) {
                a.entArchiv = entArc.path;
                a.ent = *ee;
            }
        }
        const std::string schluessel =
            verweisSchluessel(a, staende[k.bspArchiv], a.entArchiv.empty() ? std::string() : staende[k.entArchiv]);
        const auto merk = g_app->missionsVerweise.find(schluessel);
        const bool gesucht = !wunsch.empty() && k.name == wunsch;
        if (merk != g_app->missionsVerweise.end() || k.imGewaehlten || gesucht) {
            std::vector<MapData::ScriptRef> verweise;
            if (merk != g_app->missionsVerweise.end()) {
                verweise = merk->second;
                ++gemerkt;
            } else {
                if (leseSkriptVerweise(a, verweise)) {
                    g_app->missionsVerweise[schluessel] = verweise;
                }
                ++sofort;
            }
            Mission m = missionAus(k, verweise, gibt);
            m.archiv = nurDateiname(a.bspArchiv);
            // Aus "zuletzt geoeffnet": die gemerkte Mission, ohne Auswahl
            // (rc599) - auch wenn sie nicht im gewaehlten Archiv liegt.
            if (gesucht) {
                diag::info("Mission \"" + wunsch + "\" aus " + m.archiv + " (zuletzt geoeffnet)");
                g_app->missions.push_back(m);
                loadMission(m);
                return;
            }
            g_app->missions.push_back(std::move(m));
            continue;
        }
        g_app->missionsSuchKarten.push_back(k);
        g_app->missionsSuchArchive.push_back(nurDateiname(a.bspArchiv));
        g_app->missionsSuchSchluessel.push_back(schluessel);
        hinten.push_back(std::move(a));
    }
    sortiereMissionen(g_app->missions);
    diag::info(std::to_string(karten.size()) + " Karten im Spiel (" + std::to_string(spiel.size()) + " Archive), " +
               std::to_string(sofort) + " gleich gelesen, " + std::to_string(gemerkt) + " gemerkt, " +
               std::to_string(hinten.size()) + " im Hintergrund");
    if (!hinten.empty()) {
        // Faeden: genug, dass auch Movie Duels (rund 3 GB Karten) in wenigen
        // Sekunden durch ist, aber hoechstens 8 - jeder haelt eine ganze
        // Karte (bis ~30 MB) im Speicher, und das Bild soll fluessig bleiben.
        const unsigned kerne = std::max(1U, std::thread::hardware_concurrency());
        const unsigned faeden = std::max(1U, std::min(8U, kerne > 2U ? kerne - 2U : 1U));
        g_app->missionsSucheAb = std::chrono::steady_clock::now();
        g_app->missionsSuche = std::make_unique<MissionsSuche>(std::move(hinten), faeden);
    }

    // Genau eine spielbare Mission im ganzen Spiel? Dann direkt laden.
    //
    // Bei mehreren wird gefragt - und das ist der Normalfall. Solange noch im
    // Hintergrund gelesen wird, steht die Zahl nicht fest: dann die Auswahl.
    if (!g_app->missionsSuche) {
        int playable = 0;
        const Mission* only = nullptr;
        for (const Mission& m : g_app->missions) {
            if (m.playable()) {
                ++playable;
                only = &m;
            }
        }
        if (playable == 1 && only != nullptr) {
            const Mission kopie = *only;
            loadMission(kopie);
            return;
        }
    }
    g_app->missionsFilter[0] = '\0';
    g_app->missionPickOpen = true;
}

// Eine Mission einrichten: Karte, Entities, erstes Skript.
void loadMission(const Mission& m) {
    // --- Die Karte des VORIGEN Reiters festhalten -------------------------
    //
    // Gemeldet: eine Mission zu laden oeffnet richtigerweise einen neuen
    // Reiter - aber im ersten war die Karte danach auch geladen.
    //
    // Der Grund liegt in der Reihenfolge: hier wird ERST die Karte geladen,
    // und das Skript danach legt den neuen Reiter an. Beim Anlegen parkt das
    // Programm den alten - und schreibt ihm dabei die Karte zu, die gerade
    // lebt. Das ist inzwischen die neue.
    //
    // Also merken, was der alte Reiter hatte, und es ihm hinterher
    // zurueckgeben. Sauberer waere, den Reiter vor der Karte anzulegen -
    // dazu muesste aber das Anlegen aus dem Skriptoeffnen heraus, und das
    // ist ein eigener Umbau.
    // Geaenderte, nicht gespeicherte Entities derselben Karte gingen beim
    // Neuladen verloren - erst fragen.
    if (g_app->entitiesUngespeichert.count(m.mapFile) != 0U) {
        if (!g_app->frageOffen) {
            char t[600];
            std::snprintf(t, sizeof(t), tr(Str::MissionUnsavedReload), m.name.c_str());
            frage(t, [m] {
                g_app->entitiesUngespeichert.erase(m.mapFile);
                loadMission(m);
            }, [] {});
        }
        return;
    }
    const int vorherTab = g_app->activeTab;
    const std::string vorherMapId = g_app->map.path;
    const std::string vorherMapPath = g_app->settings.mapPath;
    diag::Step step("Mission laden: " + m.name);
    // rc603: die Auswahl ist erledigt - keine Karten mehr im Hintergrund
    // lesen, das Laden braucht die Platte und die Kerne selbst.
    missionsSucheBeenden();

    std::string data;
    if (!readFromArchives(m.mapFile, data)) {
        step.fail("Karte nicht lesbar: " + m.mapFile);
        return;
    }
    // Die Mission ausdruecklich laden heisst: ihre .ent, nicht was von
    // frueher gemerkt ist.
    vergissEntities(m.mapFile);
    loadMapFromBytes(data, m.mapFile);
    // Aus einem Archiv gibt es keinen Pfad auf der Platte.
    g_app->settings.mapPath.clear();

    // Die .ent hat Vorrang: Movie Duels laesst die .bsp unveraendert und
    // legt die Entities daneben. Ohne sie bekaeme man die Entities der
    // Originalkarte - ohne die Figuren der Mission.
    if (!m.entFile.empty() && readFromArchives(m.entFile, data)) {
        std::vector<MapEntity> ents;
        parseEntities(data, ents);
        if (!ents.empty()) {
            g_app->map.entities = std::move(ents);
            collectNames(g_app->map);
            merkeEntities();
            diag::info(std::to_string(g_app->map.entities.size()) +
                       " Entities aus " + m.entFile);
        }
    }

    // --- ALLE Skripte der Mission, jedes in einen eigenen Reiter ----------
    //
    // Gewuenscht: "wenn eine Mission mehrere Cutscenes hat, wuerde ich gerne
    // alle laden koennen und auch umschalten koennen."
    //
    // Vorher stand hier ein `break` nach dem ersten - meistens dem intro.
    // Die Mission kennt aber alle, und seit rc235 teilen sie sich die Karte,
    // ohne sie einander wegzunehmen. Umschalten ist dann das, was es
    // ohnehin schon gibt: das Reiterband, die geteilte Ansicht, das
    // Klappfeld ueber jedem Feld.
    //
    // Eine Obergrenze, weil manche Missionen sehr viele haben: 27 Karten
    // wurden in Ep3 gefunden, und ebenso viele Reiter waeren unbedienbar.
    // Wer mehr braucht, oeffnet sie einzeln.
    // Alle, nicht acht.
    //
    // Gemeldet: "ich muesste alle laden koennen, auch wenn es mehr sind als
    // 8." Bei md_ga_jedi waren es 24 - 16 blieben liegen.
    //
    // Die Grenze war Vorsicht vor unbedienbar vielen Reitern. Die ist
    // unbegruendet: das Reiterband rollt, und wer sie nicht braucht,
    // schliesst sie. Ein Skript, das nicht geladen ist, kann man dagegen
    // gar nicht ansehen - das ist die schlimmere Einschraenkung.
    //
    // Teuer ist es nicht: die Karte wird seit rc235 geteilt, ein weiteres
    // Skript kostet nur das Lesen der Datei.
    constexpr std::size_t kMaxSkripte = 999;
    std::size_t geladen = 0;
    std::size_t uebersprungen = 0;
    g_app->missionErstesSkript.clear();
    // Der Reiter des ERSTEN Skripts - gemerkt, nicht ausgerechnet. Hier
    // stand "Reiter minus geladene": das traf nur, solange alle neuen Reiter
    // hinten am Stueck lagen. Lag ein wiederverwendeter leerer Reiter in der
    // Mitte, oder war das erste Skript schon offen (Mission zum zweiten Mal
    // geladen), stand danach ein fremdes Skript vorn (Selbsttest "mission").
    int erstesReiter = -1;
    // --- rc603: in der Reihenfolge, in der das Spiel die Skripte startet ----
    //
    // Gewuenscht: "kann er die scripts bei load mission in der Reihenfolge
    // als Tab oeffnen, wie sie ingame abgespielt werden?" Bisher kamen sie in
    // der Reihenfolge der Entities - bei t2_trip zuerst common/use_turbo, das
    // erst ein Trigger des Spielers startet, und das Intro an siebter Stelle.
    //
    // Jetzt laeuft die Karte im ICARUS-Nachbau vom Laden an
    // (simuliereKartenstart): spawnscripts, worldspawn, trigger_always,
    // Spawner ohne targetname, die Ziele der Startposition beim ClientSpawn,
    // und alles, was daraus per run, use, Relais, Spawn und kill folgt. Die
    // Reiter oeffnen nach dem ersten Start; was dort nie startet (Trigger
    // beim Betreten, Kampf), steht dahinter in der bisherigen Reihenfolge.
    // Vorn steht das erste.
    const std::vector<std::size_t> folge = [&m] {
        std::vector<std::string> pfade;
        pfade.reserve(m.scripts.size());
        for (const Mission::Script& sc : m.scripts) {
            pfade.push_back(sc.ref);
        }
        const auto ab = std::chrono::steady_clock::now();
        AblaufWelt welt = ablaufWelt();
        welt.spielstufe = regelwerkSpielstufe();
        const auto abSim = std::chrono::steady_clock::now();
        const NavPlanZaehler wegeVorher = navPlanZaehler();
        const Ablauf start = simuliereKartenstart(welt);
        const NavPlanZaehler wegeNachher = navPlanZaehler();
        std::vector<std::size_t> f = skripteNachStart(pfade, start);
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - ab).count();
        const double msSim = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - abSim).count();
        diag::detail("Kartenstart: Welt " + std::to_string(static_cast<long>(ms - msSim)) + " ms, Nachbau " +
                     std::to_string(static_cast<long>(msSim)) + " ms, Wege " +
                     std::to_string(wegeNachher.kalt - wegeVorher.kalt) + " neu (" +
                     std::to_string(static_cast<long>(wegeNachher.msKalt - wegeVorher.msKalt)) + " ms)");
        // Fuers Protokoll: die Zeit je Skript (wie skripteNachStart vergleicht -
        // ohne Gross/Klein; die Pfade aus der Karte haben keine Endung).
        const auto gleichOhneFall = [](const std::string& a, const std::string& b) {
            return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
                       return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
                   });
        };
        std::string text;
        for (const std::size_t i : f) {
            double t = -1.0;
            for (std::size_t k = 0; k < start.skripte.size() && k < start.skriptBeginn.size(); ++k) {
                if (gleichOhneFall(start.skripte[k], pfade[i])) {
                    t = start.skriptBeginn[k];
                }
            }
            text += " " + pfade[i] + (t >= 0.0 ? "@" + std::to_string(static_cast<long>(t)) : std::string("@-"));
        }
        char z[160];
        std::snprintf(z, sizeof(z), "Reihenfolge der Skripte (Kartenstart, %.1f ms, %ld Befehle, bis %.0f ms):",
                      ms, static_cast<long>(start.befehle), start.laufMs);
        diag::info(std::string(z) + text);
        for (const std::string& h : start.hinweise) {
            diag::detail("Kartenstart: " + h);
        }
        return f;
    }();
    for (const std::size_t si : folge) {
        const Mission::Script& sc2 = m.scripts[si];
        if (sc2.file.empty()) {
            continue;
        }
        if (g_app->missionErstesSkript.empty()) {
            g_app->missionErstesSkript = sc2.file;
        }
        if (geladen >= kMaxSkripte) {
            ++uebersprungen;
            continue;
        }
        if (readFromArchives(sc2.file, data)) {
            // Schon offen (die Mission zum zweiten Mal geladen)? Dann bleibt
            // der Reiter samt Aenderungen und Verlauf - siehe
            // oeffneSkriptAusSpeicher.
            //
            // Auch, wenn es aus dem Missionsordner auf der Platte offen ist
            // (nach Save mission): sonst stand es zweimal da (11.10.).
            int offen = reiterMitSkript(sc2.file);
            if (offen < 0) { offen = reiterFuerSkriptRef(sc2.ref); }
            if (offen >= 0) {
                if (erstesReiter < 0) { erstesReiter = offen; }
                continue;
            }
            openScriptFromMemory(data, sc2.file);
            if (erstesReiter < 0) { erstesReiter = g_app->activeTab; }
            ++geladen;
        }
    }
    diag::info("Skripte der Mission: " + std::to_string(geladen) +
               " geladen" +
               (uebersprungen != 0U
                    ? ", " + std::to_string(uebersprungen) +
                          " nicht (Grenze " + std::to_string(kMaxSkripte) + ")"
                    : std::string()));

    // Zurueck auf das ERSTE - meistens das intro, und das will man sehen.
    //
    // Ohne das stuende man im letzten geladenen, was willkuerlich waere.
    if (erstesReiter >= 0 && erstesReiter < static_cast<int>(g_app->tabs.size())) {
        activateTab(erstesReiter);
    }

    // Und jetzt alles auf einmal, statt beim ersten Bewegen.
    prepareScene();
    // Fuer "zuletzt geoeffnete Missionen" (rc599): Archiv und Name.
    if (!g_app->missionArchive.empty()) {
        g_app->settings.noteRecentMission("pk3|" + g_app->missionArchive + "|" + m.name);
    }
    // Dem vorigen Reiter seine Karte zurueckgeben - siehe oben.
    //
    // Nur, wenn wirklich ein NEUER Reiter entstanden ist. Wurde die Mission
    // in denselben geladen (weil er leer und ungeaendert war), gehoert ihm
    // die neue Karte zu Recht.
    if (g_app->activeTab != vorherTab && vorherTab >= 0 &&
        static_cast<std::size_t>(vorherTab) < g_app->tabs.size()) {
        App::Parked& alt = g_app->tabs[static_cast<std::size_t>(vorherTab)];
        alt.mapId = vorherMapId;
        alt.mapPath = vorherMapPath;
        diag::detail("Mission in neuem Reiter: Reiter " +
                     std::to_string(vorherTab) + " behaelt seine Karte \"" +
                     (vorherMapId.empty() ? std::string("(keine)")
                                          : vorherMapId) +
                     "\"");
    }
    g_app->leftMode = 1;
    g_app->missionPickOpen = false;
    missionAusGeladener(m);
    char msg[256];
    std::snprintf(msg, sizeof(msg), tr(Str::MissionLoaded), m.name.c_str(),
                  static_cast<int>(g_app->map.entities.size()),
                  static_cast<int>(m.scripts.size()));
    addStatus(msg);
}

// Die Auswahl der Missionen.
//
// rc603: die Missionen des ganzen Spiels - oben die des gewaehlten Archivs,
// darunter alle weiteren (im Hintergrund gelesen, solange steht dort, wie
// weit). Bei Movie Duels sind das rund 300 Karten: die Liste rollt in einem
// eigenen Feld, Kopf (Spielordner, Filter) und Abbrechen bleiben stehen.
namespace {
// Eine Zeile der Auswahl; true, wenn diese Mission geladen werden soll.
bool missionsZeile(std::size_t i) {
    const Mission& m = g_app->missions[i];
    ImGui::PushID(static_cast<int>(i));
    ImGui::BeginDisabled(!m.playable());
    const bool geklickt = ImGui::Button(m.name.c_str(), ImVec2{ImGui::GetFontSize() * 14.0F, 0.0F});
    ImGui::EndDisabled();
    if (geklickt) {
        ImGui::PopID();
        return true;
    }
    ImGui::SameLine();
    if (m.playable()) {
        ImGui::TextDisabled("%d", static_cast<int>(m.scripts.size()));

        // --- Teilen sich mehrere Missionen eine Karte? ----------------
        //
        // Gefragt: "sind das alle Missionen, oder teilen sich davon
        // welche was? Das muesste ich wissen."
        //
        // Die Antwort steht in den Daten - sie wurde nur nicht gezeigt.
        // Jede Mission kennt ihre .bsp; tragen zwei dieselbe, spielen
        // sie auf derselben Karte. Bei Movie Duels ist das der Regelfall:
        // md_afif2_jedi und md_afif2_sith sind dieselbe Karte, einmal
        // aus Sicht des Jedi und einmal des Sith.
        //
        // Das ist keine Nebensache: wer eine davon laedt, sieht die
        // Geometrie der anderen mit, und die Skripte laufen ueber
        // dieselben Entities.
        {
            int gleicheKarte = 0;
            for (const Mission& andere : g_app->missions) {
                if (andere.playable() && andere.mapFile == m.mapFile) {
                    ++gleicheKarte;
                }
            }
            if (gleicheKarte > 1) {
                ImGui::SameLine();
                ImGui::TextDisabled("%s %d", tr(Str::MissionSharesMap), gleicheKarte);
            }
        }
        // Beim Ueberfahren zeigen, WAS geladen wird - Entity-Datei und
        // die Skripte. Sonst raet man am Kartennamen herum.
        if (ImGui::IsItemHovered() || ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(m.mapFile.c_str());
            // rc603: aus welchem Archiv die Engine die Karte laedt.
            if (!m.archiv.empty()) {
                ImGui::TextDisabled(tr(Str::MissionFromArchive), m.archiv.c_str());
            }
            // Und im Hinweisfenster die Namen dazu - so sieht man
            // sofort, WELCHE es sind.
            for (const Mission& andere : g_app->missions) {
                if (andere.playable() && andere.mapFile == m.mapFile && andere.name != m.name) {
                    ImGui::TextDisabled("%s %s", tr(Str::MissionSharesMap), andere.name.c_str());
                }
            }
            if (!m.entFile.empty()) {
                ImGui::TextDisabled("%s", m.entFile.c_str());
            }
            ImGui::Separator();
            for (const Mission::Script& sc : m.scripts) {
                if (sc.file.empty()) {
                    ImGui::TextDisabled("%s  (%s)", sc.ref.c_str(), tr(Str::MapScriptMissing));
                } else {
                    ImGui::TextUnformatted(sc.ref.c_str());
                }
            }
            ImGui::EndTooltip();
        }
    } else {
        ImGui::TextDisabled("%s", tr(Str::MissionDuel));
    }
    ImGui::PopID();
    return false;
}

bool passtZumFilter(const Mission& m, const std::string& filter) {
    if (filter.empty()) {
        return true;
    }
    std::string n = m.name;
    for (char& c : n) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return n.find(filter) != std::string::npos;
}
}  // namespace

void drawMissionPicker() {
    if (!g_app->missionPickOpen) {
        // Zu (Abbrechen, eine Mission gewaehlt, von aussen geschlossen):
        // die Hintergrundsuche wird nicht mehr gebraucht.
        if (g_app->missionsSuche) {
            missionsSucheBeenden();
        }
        return;
    }
    missionsSucheUebernehmen();
    ImGui::OpenPopup("##missionpick");
    const ImVec2 centre = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2{0.5F, 0.5F});
    if (!ImGui::BeginPopupModal("##missionpick", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }
    ImGui::TextUnformatted(tr(Str::MissionPick));
    // rc603: welche Spielordner gelten - das Spiel, dessen Missionen hier
    // stehen. Die vollen Pfade im Hinweis.
    {
        std::string kurz;
        for (const GamePath& gp : g_app->gamePaths) {
            kurz += (kurz.empty() ? "" : ", ") + nurDateiname(gp.directory);
        }
        ImGui::TextDisabled("%s %s", tr(Str::MissionGameFolders), kurz.c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            for (const GamePath& gp : g_app->gamePaths) {
                ImGui::TextUnformatted(gp.directory.c_str());
            }
            ImGui::EndTooltip();
        }
        if (!g_app->sitzungsSpielordner.empty()) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4{0.90F, 0.75F, 0.35F, 1.0F}, "%s", tr(Str::MissionSessionOnly));
        }
    }
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14.0F);
    ImGui::InputTextWithHint("##missionfilter", tr(Str::Pk3Filter), g_app->missionsFilter,
                             sizeof(g_app->missionsFilter));
    std::string filter = g_app->missionsFilter;
    for (char& c : filter) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    ImGui::Separator();

    std::size_t eigene = 0;
    std::size_t weitere = 0;
    for (const Mission& m : g_app->missions) {
        if (!passtZumFilter(m, filter)) {
            continue;
        }
        (m.imGewaehlten ? eigene : weitere) += 1;
    }
    const bool sucht = static_cast<bool>(g_app->missionsSuche);
    // Hoehe: so viele Zeilen wie da sind, hoechstens 22 - dann rollt das Feld.
    const float zeile = ImGui::GetFrameHeightWithSpacing();
    const float zeilen = static_cast<float>(eigene + weitere) + (eigene > 0 ? 1.5F : 0.0F) +
                         ((weitere > 0 || sucht) ? 1.5F : 0.0F) + (sucht ? 2.0F : 0.0F);
    const float hoehe = std::max(2.0F, std::min(22.0F, zeilen)) * zeile + ImGui::GetStyle().WindowPadding.y * 2.0F;
    std::optional<Mission> laden;
    const bool anyPlayable =
        std::any_of(g_app->missions.begin(), g_app->missions.end(), [](const Mission& m) { return m.playable(); });
    if (ImGui::BeginChild("##missionliste", ImVec2{ImGui::GetFontSize() * 34.0F, hoehe}, ImGuiChildFlags_None)) {
        if (eigene > 0) {
            char kopf[600];
            std::snprintf(kopf, sizeof(kopf), tr(Str::MissionInArchive), nurDateiname(g_app->missionArchive).c_str());
            ImGui::SeparatorText(kopf);
            for (std::size_t i = 0; i < g_app->missions.size() && !laden; ++i) {
                const Mission& m = g_app->missions[i];
                if (m.imGewaehlten && passtZumFilter(m, filter) && missionsZeile(i)) {
                    laden = m;   // loadMission raeumt die Liste auf
                }
            }
        }
        if ((weitere > 0 || sucht) && !laden) {
            ImGui::SeparatorText(tr(Str::MissionOtherArchives));
            if (sucht) {
                const MissionsSuche& su = *g_app->missionsSuche;
                char z[200];
                std::snprintf(z, sizeof(z), tr(Str::MissionSearching), static_cast<int>(su.erledigt()),
                              static_cast<int>(su.gesamt()));
                ImGui::TextDisabled("%s", z);
                ImGui::ProgressBar(su.gesamt() > 0 ? static_cast<float>(su.erledigt()) / static_cast<float>(su.gesamt()) : 1.0F,
                                   ImVec2{ImGui::GetFontSize() * 20.0F, 0.0F}, "");
            }
            for (std::size_t i = 0; i < g_app->missions.size() && !laden; ++i) {
                const Mission& m = g_app->missions[i];
                if (!m.imGewaehlten && passtZumFilter(m, filter) && missionsZeile(i)) {
                    laden = m;
                }
            }
        }
    }
    ImGui::EndChild();
    if (laden) {
        // Die Auswahl ist erledigt - auch wenn loadMission erst fragt
        // (ungesicherte Entities derselben Karte). Vorher blieb
        // missionPickOpen dann stehen, und nach der Frage ging die Auswahl
        // wieder auf, hinter jedem weiteren Fenster (rc603, Selbsttest
        // missionsbau: "Cancel" im Fenster Save mission nicht zu finden).
        g_app->missionPickOpen = false;
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        loadMission(*laden);
        return;
    }
    if (!anyPlayable && !sucht) {
        ImGui::TextDisabled("%s", tr(Str::MissionNone));
    }
    ImGui::Separator();
    if (ImGui::Button(tr(Str::EditorCancel))) {
        g_app->missionPickOpen = false;
        missionsSucheBeenden();
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

// Der Ebenen-Verwalter fuer Entities.
//
// Nach dem Vorbild von 3ds Max: eine Klasse ist eine Ebene, das Auge davor
// schaltet sie um, ein Klick auf den Namen klappt ihre Mitglieder auf. Was
// dort "Layer Explorer" heisst, ist hier nach Klassennamen gruppiert - eine
// Karte hat keine Ebenen, aber classname trennt genauso sauber.
//
// Zwei Griffe aus Max uebernommen:
//   * Alt-Klick auf ein Auge stellt die Gruppe ALLEIN (Solo). In csTools
//     heisst das "Alt+click a button to solo it".
//   * Eine versteckte Ebene versteckt ihre Mitglieder mit, auch wenn deren
//     eigenes Auge offen ist.
namespace {

// Das Auge der Entity-Liste. Die Kennung bleibt "O" (sichtbar) / "-" - so
// hiessen die Knoepfe vorher, und der Selbsttest sucht sie so.
// + zum Aufklappen, - zum Zuklappen (wie im Skriptbaum und im Windows-Explorer).
bool klappKnopf(bool offen) {
    const float h = ImGui::GetTextLineHeight();
    const bool geklickt = ImGui::InvisibleButton("##klapp", ImVec2{h, h});
    const bool ueber = ImGui::IsItemHovered();
    const ImVec2 a = ImGui::GetItemRectMin();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 f = ImGui::GetColorU32(ueber ? ImGuiCol_Text : ImGuiCol_TextDisabled);
    const float r = std::floor(h * 0.30F);
    const ImVec2 m{std::floor(a.x + h * 0.5F) + 0.5F, std::floor(a.y + h * 0.5F) + 0.5F};
    const float d = std::max(1.0F, h / 14.0F);
    dl->AddRect(ImVec2{m.x - r - 1.0F, m.y - r - 1.0F}, ImVec2{m.x + r + 1.0F, m.y + r + 1.0F}, f, 2.0F, 0, d);
    dl->AddLine(ImVec2{m.x - r + 2.0F, m.y}, ImVec2{m.x + r - 1.0F, m.y}, f, d);
    if (!offen) { dl->AddLine(ImVec2{m.x, m.y - r + 2.0F}, ImVec2{m.x, m.y + r - 1.0F}, f, d); }
    if (ueber) { ImGui::SetTooltip("%s", tr(offen ? Str::EntCollapse : Str::EntExpand)); }
    return geklickt;
}

bool augeKnopf(const char* kennung, bool sichtbar) {
    const float h = ImGui::GetTextLineHeight();
    const bool geklickt = ImGui::InvisibleButton(kennung, ImVec2{h, h});
    const bool ueber = ImGui::IsItemHovered();
    const ImVec2 a = ImGui::GetItemRectMin();
    ImVec4 f = ImGui::GetStyleColorVec4(sichtbar ? ImGuiCol_Text : ImGuiCol_TextDisabled);
    if (!ueber) { f.w *= sichtbar ? 0.75F : 0.6F; }
    uiIcon(ImGui::GetWindowDrawList(), sichtbar ? UiIcon::Sichtbar : UiIcon::Versteckt,
           ImVec2{std::floor(a.x), std::floor(a.y)}, h, ImGui::GetColorU32(f));
    if (ueber) { ImGui::SetTooltip("%s", tr(sichtbar ? Str::EntHide : Str::EntShow)); }
    return geklickt;
}

void listenIcon(UiIcon icon, ImU32 farbe) {
    const float h = ImGui::GetTextLineHeight();
    const ImVec2 a = ImGui::GetCursorScreenPos();
    uiIcon(ImGui::GetWindowDrawList(), icon, ImVec2{std::floor(a.x), std::floor(a.y)}, h, farbe);
    ImGui::Dummy(ImVec2{h, h});
}

}  // namespace

void drawEntityLayers(float height) {
    // --- Die Entities, die man beim Schreiben einer Szene braucht ---------
    //
    // Vorher vier Klassen (waypoint_navgoal, target_position, NPC_spawner,
    // target_scriptrunner). Fuer eine Zwischensequenz fehlten gerade die
    // wichtigsten: die Kameramarken (ref_tag - in md_ga_jedi steht JEDE
    // Kamera auf einer), alle NPC_*-Klassen, Effekte, Lautsprecher, Musik
    // und die Brushes, die ein Skript bewegt.
    //
    // Dazu eine Suche und je Eintrag, was man damit tut: Doppelklick fliegt
    // hin, Rechtsklick setzt den passenden Befehl hinter die gewaehlte Zeile
    // - "Kamera auf diese Marke" (MOVE + PAN mit $tag$), "Laufziel",
    // "Blickziel", "affect-Block", "use". So entsteht eine Szene aus der
    // Karte heraus, ohne Namen abzutippen.
    // Darunter der Inspektor (gui/app_mission.cpp). Die Aufteilung steht
    // immer gleich - auch ohne Auswahl -, damit beim Anklicken nichts springt.
    // `height` ist wie ueberall in der Anordnung meist NEGATIV: "bis unten,
    // abzueglich Fusszeile" (l.frameH). Erst die wirkliche Hoehe ausrechnen.
    const float gesamt = height > 0.0F ? height : ImGui::GetContentRegionAvail().y + height;
    const float listenH = std::max(gesamt * 0.38F, ImGui::GetFontSize() * 7.0F);
    g_app->entFokusListe = false;
    g_app->entListeHover = -1;
    ImGui::BeginChild("entlayers", ImVec2{ImGui::GetFontSize() * kEntitySpalte, listenH},
                      ImGuiChildFlags_Borders);
    g_app->entFokusListe = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    static char suche[64] = "";
    ImGui::SetNextItemWidth(-1.0F);
    ImGui::InputTextWithHint("##entsuche", tr(Str::EntitySearch), suche, sizeof(suche));
    std::string such = suche;
    for (char& c : such) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    const auto klein = [](std::string x) {
        for (char& c : x) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        return x;
    };
    for (int kat = 0; kat < entityKategorien(); ++kat) {
        // Welche Entities, welche Klassen, wie viele sichtbar?
        std::vector<std::size_t> drin;
        std::set<std::string> klassen;
        int shown = 0;
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            const MapEntity& e = g_app->map.entities[i];
            if (entityKategorie(e) != kat) {
                continue;
            }
            const std::string name = entitySkriptName(e);
            if (!such.empty() && klein(name).find(such) == std::string::npos &&
                klein(e.classname).find(such) == std::string::npos) {
                continue;
            }
            drin.push_back(i);
            klassen.insert(e.classname);
            if (g_app->entityVisible(i, e.classname)) {
                ++shown;
            }
        }
        if (drin.empty()) {
            continue;
        }
        const EntityKat& ek = entityKategorieInfo(kat);
        ImGui::PushID(kat);
        bool alleVersteckt = true;
        for (const std::string& k2 : klassen) {
            alleVersteckt = alleVersteckt && g_app->hiddenClasses.count(k2) != 0;
        }
        // Davor + / - zum Auf- und Zuklappen, dann ein Auge statt "O"/"-"
        // (rc588), dann das Icon der Gruppe in ihrer Farbe - dasselbe, das in
        // der Karte ueber der Entity steht. shank: "ein plus machen zum
        // aufklappen und es dann als minus zeigen zum zuklappen und ein auge
        // neben dran".
        const std::string schluessel = "kat" + std::to_string(kat);
        const bool open = (g_app->expandedClass == schluessel) || !such.empty();
        if (klappKnopf(open)) {
            g_app->expandedClass = (g_app->expandedClass == schluessel) ? std::string() : schluessel;
        }
        ImGui::SameLine();
        if (augeKnopf(alleVersteckt ? "-" : "O", !alleVersteckt)) {
            if (ImGui::GetIO().KeyAlt) {
                // Alt+Klick: nur diese Gruppe zeigen (so sagt es der Tooltip
                // der Liste; rc599 - vorher schaltete Alt wie ein Klick).
                for (const MapEntity& e2 : g_app->map.entities) { g_app->hiddenClasses.insert(e2.classname); }
                for (const std::string& k2 : klassen) { g_app->hiddenClasses.erase(k2); }
            } else {
                for (const std::string& k2 : klassen) {
                    if (alleVersteckt) {
                        g_app->hiddenClasses.erase(k2);
                    } else {
                        g_app->hiddenClasses.insert(k2);
                    }
                }
            }
            g_app->mapDirty = true;
        }
        ImGui::SameLine();
        listenIcon(ek.icon, IM_COL32(ek.r, ek.g, ek.b, 255));
        ImGui::SameLine();
        if (ImGui::Selectable(tr(ek.titel), open)) {
            g_app->expandedClass = (g_app->expandedClass == schluessel) ? std::string() : schluessel;
        }
        ImGui::SameLine();
        ImGui::TextDisabled(tr(Str::EntityCount), shown, static_cast<int>(drin.size()));
        if (open) {
            // Um die Breite des Klappknopfs eingerueckt: die Augen der
            // Mitglieder stehen dann unter dem Auge der Gruppe.
            ImGui::Indent(ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.x);
            for (const std::size_t i : drin) {
                const MapEntity& e = g_app->map.entities[i];
                ImGui::PushID(static_cast<int>(i));
                const bool own = g_app->hiddenEntities.count(i) != 0;
                ImGui::BeginDisabled(g_app->hiddenClasses.count(e.classname) != 0);
                if (augeKnopf(own ? "-" : "O", !own)) {
                    if (own) {
                        g_app->hiddenEntities.erase(i);
                    } else {
                        g_app->hiddenEntities.insert(i);
                    }
                    g_app->mapDirty = true;
                }
                ImGui::EndDisabled();
                ImGui::SameLine();
                listenIcon(entityIcon(e), IM_COL32(ek.r, ek.g, ek.b, 255));
                ImGui::SameLine();
                const std::string name = entitySkriptName(e);
                const std::string label = name.empty() ? std::string("(") + e.classname + ")" : name;
                if (ImGui::Selectable(label.c_str(), entityIstGewaehlt(static_cast<int>(i)),
                                      ImGuiSelectableFlags_AllowDoubleClick)) {
                    if (ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeyShift) {
                        entityAuswahlUmschalten(static_cast<int>(i));
                    } else {
                        entityAuswahlSetzen(static_cast<int>(i));
                    }
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        zurEntityFliegen(i);
                    }
                }
                // In den Graphen ziehen (rc597): dort erscheint sie als Knoten an
                // der Abwurfstelle.
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoHoldToOpenOthers)) {
                    const int nr = static_cast<int>(i);
                    ImGui::SetDragDropPayload("bhed_entity", &nr, sizeof(nr));
                    listenIcon(entityIcon(e), IM_COL32(ek.r, ek.g, ek.b, 255));
                    ImGui::SameLine();
                    ImGui::TextUnformatted(label.c_str());
                    ImGui::EndDragDropSource();
                }
                // Ueberfahren in der Liste hebt die Entity in der Ansicht hervor.
                if (ImGui::IsItemHovered()) { g_app->entListeHover = static_cast<int>(i); }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(e.classname.c_str());
                    ImGui::TextDisabled("%s", tr(Str::EntityRowHint));
                    ImGui::EndTooltip();
                }
                if (ImGui::BeginPopupContextItem("entmenu")) {
                    g_app->pickedEntity = static_cast<int>(i);
                    entityMenue(i);
                    ImGui::EndPopup();
                }
                ImGui::PopID();
            }
            ImGui::Unindent(ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.x);
        }
        ImGui::PopID();
    }
    ImGui::EndChild();
    const float rest = gesamt - listenH - ImGui::GetStyle().ItemSpacing.y;
    if (rest > ImGui::GetFontSize() * 3.0F) {
        // Negativ weitergeben, wie es kam: dann endet der Inspektor genau
        // dort, wo die Kartenansicht daneben endet.
        // Kamera gewaehlt (und keine Entity): ihre Einstellungen (rc588).
        if (g_app->cameraSelected && g_app->pickedEntity < 0 && g_app->selectedActor < 0) {
            drawKameraInspector(height > 0.0F ? rest : height);
        } else {
            drawEntityInspector(height > 0.0F ? rest : height);
        }
    }
}

