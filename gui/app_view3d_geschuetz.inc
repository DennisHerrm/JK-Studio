// app_view3d_geschuetz.inc - rc608: die Geschuetze in der Vorschau (bhed/geschuetz.h).
//
// Eingebunden in gui/app_view3d.cpp nach app_view3d_spiel.inc, im selben Namensraum.
// GeschuetzSim rechnet misc_sentry_turret, misc_turret und misc_turretG2 je Regelwerk
// (OpenJK, Movie Duels, Jedi Outcast). Hier steht, was sie von der Vorschau braucht:
// die Spur (schussSpurBei), das PVS, die Bolzen und Netze ihrer Modelle, die Trefferorte
// der Figuren - und was aus ihr wird: Stellung im Bild, Effekte, Klaenge. Den Schaden
// bringt hurtTodeBerechnen auf die Figuren und Entities (G_Damage, G_RadiusDamage).
namespace {

TodRegelwerk geschuetzRegelwerk() {
    return regelwerkJediOutcast() ? TodRegelwerk::JediOutcast
           : regelwerkMovieDuels() ? TodRegelwerk::MovieDuels : TodRegelwerk::OpenJK;
}

const GeschuetzLauf* geschuetzLaufVon(std::size_t entity) {
    for (const GeschuetzLauf& l : g_app->geschuetzSim.laeufe()) {
        if (l.daten.nummer == static_cast<int>(entity)) { return &l; }
    }
    return nullptr;
}

const char* geschuetzModellPfad(const GeschuetzLauf& l) {
    if (l.daten.art == GeschuetzArt::Wache) { return "models/items/psgun.glm"; }
    return l.turbo ? "models/map_objects/wedge/laser_cannon_model.glm" : "models/map_objects/imp_mine/turret_canon.glm";
}

// Was die Spawnfunktionen aus der Entity lesen (SP_PAS, SP_misc_turret,
// finish_spawning_turret, SP_misc_turretG2) - die Vorgaben setzt GeschuetzSim::setze.
std::vector<GeschuetzDaten> geschuetzDatenDerKarte() {
    std::vector<GeschuetzDaten> aus;
    const bool md = regelwerkMovieDuels();
    for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
        const MapEntity& e = g_app->map.entities[i];
        GeschuetzArt art = GeschuetzArt::Wache;
        if (e.classname == "misc_turret") {
            art = GeschuetzArt::Turm;
        } else if (e.classname == "misc_turretG2") {
            art = GeschuetzArt::TurmG2;
        } else if (e.classname != "misc_sentry_turret") {
            continue;
        }
        if (!kartenpruefung::imSpiel(e.classname, md) || !gespawntImSpiel(e) || e.origin.empty()) { continue; }
        const auto zahl = [&e](const char* k) {
            const std::string* v = e.find(k);
            return v == nullptr ? 0.0F : static_cast<float>(std::atof(v->c_str()));
        };
        const auto ganz = [&e](const char* k) {
            const std::string* v = e.find(k);
            return v == nullptr ? 0 : std::atoi(v->c_str());
        };
        const auto hat = [&e](const char* k) {
            const std::string* v = e.find(k);
            return v != nullptr && !v->empty();
        };
        GeschuetzDaten d;
        d.art = art;
        d.nummer = static_cast<int>(i);
        (void)std::sscanf(e.origin.c_str(), "%f %f %f", &d.ursprung[0], &d.ursprung[1], &d.ursprung[2]);
        if (const std::string* a = e.find("angles")) {
            (void)std::sscanf(a->c_str(), "%f %f %f", &d.winkel[0], &d.winkel[1], &d.winkel[2]);
        } else if (e.find("angle") != nullptr) {
            d.winkel[1] = zahl("angle");
        }
        d.spawnflags = ganz("spawnflags");
        d.radius = zahl("radius");
        d.wait = zahl("wait");
        d.random = zahl("random");
        d.shotspeed = zahl("shotspeed");
        d.damage = ganz("dmg");
        d.health = ganz("health");
        d.count = ganz("count");
        d.splashDamage = ganz("splashDamage");
        d.splashRadius = ganz("splashRadius");
        if (const std::string* t = e.find("team")) { d.team = *t; }
        d.alliedTeam = ganz("alliedTeam");
        d.teamnodmg = ganz("teamnodmg");
        d.customscale = ganz("customscale");
        d.painwait = ganz("painwait");
        d.hatTarget = hat("target");
        d.hatTarget2 = hat("target2");
        d.hatPaintarget = hat("paintarget");
        for (const auto& [ent, ms] : g_app->moverSim.entityUses()) {
            if (ent == i) { d.uses.push_back(ms); }
        }
        aus.push_back(std::move(d));
    }
    return aus;
}

// Der Knochenstand zur Zeit ms aus dem letzten Stand (s.apos wie CG_CalcEntityLerpPositions).
GeschuetzKnochen geschuetzKnochenBei(const GeschuetzLauf& l, double ms) {
    GeschuetzKnochen k;
    for (int q = 0; q < 3; ++q) {
        k.ort[q] = l.ursprung[q];
        k.winkel[q] = l.winkel[q];
    }
    const GeschuetzStand* s = nullptr;
    for (const GeschuetzStand& x : l.staende) {
        if (x.ms > ms) { break; }
        s = &x;
    }
    if (s == nullptr) { return k; }
    const float d = s->aposLinear ? static_cast<float>((ms - s->aposZeit) * 0.001) : 0.0F;
    for (int q = 0; q < 3; ++q) { k.winkel[q] = s->aposBasis[q] + d * s->aposDelta[q]; }
    k.nick = s->nick;
    k.pasGier = s->pasGier;
    k.pasNick = s->pasNick;
    return k;
}

// G2_ConstructGhoulSkeleton fuer ein Geschuetz: die Animation am Stamm und die
// G2API_SetBoneAngles der Simulation (BONE_ANGLES_POSTMULT, ohne Mischung:
// tr_ghoul2.cpp:1800-1812 OpenJK).
//   misc_sentry_turret  bone_hinge (0, 0, gier), bone_barrel und bone_gback (-random, 0, 0),
//                       POSITIVE_Y, POSITIVE_Z, POSITIVE_X (g_turret.cpp:1727-1731)
//   misc_turret         Bone_body (+-speed, 0, 0), dieselben Achsen (:397)
//   Turbolaser          pitch (0, 0, +-speed), turret_SetBoneAngles: POSITIVE_Y, NEGATIVE_Z,
//                       NEGATIVE_X (:385, :745-751)
// UPSIDE_DOWN kehrt das Vorzeichen um (:376-383, :390-396).
bool geschuetzSkelett(const GeschuetzLauf& l, const App::ActorAssets& as, const GeschuetzKnochen& k, double ms,
                      std::vector<BoneMatrix>& welt, std::vector<BoneMatrix>& skin) {
    const App::Skeleton* sk = skeletonFor(as.model);
    if (sk == nullptr || sk->anim.empty()) { return false; }
    std::vector<std::pair<int, BoneMatrix>> nachMal;
    const auto setze = [&](const char* name, float a, float b, float c, G2Achse oben, G2Achse links, G2Achse vorn) {
        const int nr = boneIndex(sk->anim, name);
        if (nr < 0) { return; }
        const float w[3] = {a, b, c};
        nachMal.emplace_back(nr, g2WinkelMatrix(w, oben, links, vorn));
    };
    const float vz = (l.daten.spawnflags & 2) != 0 ? 1.0F : -1.0F;
    if (l.daten.art == GeschuetzArt::Wache) {
        setze("bone_barrel", -k.pasNick, 0.0F, 0.0F, G2Achse::PosY, G2Achse::PosZ, G2Achse::PosX);
        setze("bone_gback", -k.pasNick, 0.0F, 0.0F, G2Achse::PosY, G2Achse::PosZ, G2Achse::PosX);
        setze("bone_hinge", 0.0F, 0.0F, k.pasGier, G2Achse::PosY, G2Achse::PosZ, G2Achse::PosX);
    } else if (l.turbo) {
        setze("pitch", 0.0F, 0.0F, -vz * k.nick, G2Achse::PosY, G2Achse::NegZ, G2Achse::NegX);
    } else {
        setze("Bone_body", vz * k.nick, 0.0F, 0.0F, G2Achse::PosY, G2Achse::PosZ, G2Achse::PosX);
    }
    int bild = 0;
    int naechstes = 0;
    float lerp = 0.0F;
    if (l.daten.art == GeschuetzArt::Wache) {
        // pas_think: G2API_SetBoneAnimIndex(rootBone, 0, 11, BONE_ANIM_OVERRIDE_FREEZE, 0.8)
        for (const auto& [ab, was] : l.aufklappen) {
            (void)was;
            if (ab > ms) { break; }
            g2Bild(KnochenAnim{ab, 0, 11, 0.8F}, ms, bild, naechstes, lerp);
        }
    } else if (l.turbo) {
        // TurboLaser_SetBoneAnim: BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND, 1.0, Mischung
        // 100 ms vom Bild davor; jede dieser Animationen hat genau ein Bild (start).
        int vorher = -1;
        for (const auto& [ab, f] : l.turboAnims) {
            if (ab > ms) { break; }
            const float seit = static_cast<float>(ms - ab);
            if (vorher >= 0 && vorher != f.first && seit < 100.0F) {
                bild = vorher;
                naechstes = f.first;
                lerp = seit / 100.0F;
            } else {
                bild = f.first;
                naechstes = f.first;
                lerp = 0.0F;
            }
            vorher = f.first;
        }
    }
    sk->anim.worldMatricesLerp(bild, naechstes, lerp, {}, nachMal, welt);
    if (welt.empty()) { return false; }
    gpu::baueKnochenmatrizen(welt, sk->anim.bones, skin);
    return true;
}

// G2API_GetBoltMatrix(ghoul2, 0, bolzen, &m, k.winkel, k.ort, ms, NULL, modelScale): nur
// die Verschiebung des Bolzens wird skaliert (G2_API.cpp:1760-1771 OpenJK).
bool geschuetzBolzenMatrix(const GeschuetzLauf& l, double ms, const GeschuetzKnochen& k, const char* bolzen, float m[3][4]) {
    const App::ActorAssets* as = gegenstandGlmFuer(geschuetzModellPfad(l));
    if (as == nullptr) { return false; }
    const App::Skeleton* sk = skeletonFor(as->model);
    if (sk == nullptr) { return false; }
    const int flaeche = surfaceIndex(as->model, bolzen);
    if (flaeche < 0) { return false; }
    std::vector<BoneMatrix> welt;
    std::vector<BoneMatrix> skin;
    if (!geschuetzSkelett(l, *as, k, ms, welt, skin)) { return false; }
    BoneMatrix b{};
    if (!boltMatrix(as->model, flaeche, welt, sk->anim, b)) { return false; }
    g2WurzelBolzen(b);
    float achse[3][3];
    winkelZuAchsen(k.winkel, achse);
    for (int r = 0; r < 3; ++r) {
        m[r][3] = k.ort[r];
        for (int c = 0; c < 3; ++c) {
            m[r][c] = 0.0F;
            for (int q = 0; q < 3; ++q) { m[r][c] += achse[q][r] * b.m[q][c]; }
        }
        for (int q = 0; q < 3; ++q) { m[r][3] += achse[q][r] * l.skala * b.m[q][3]; }
    }
    return true;
}

}  // namespace

// SV_ClipMoveToEntities: Steht das Geschuetz noch - und wo ist sein Kasten?
namespace {
bool geschuetzHindernis(std::size_t ent, double ms, float lo[3], float hi[3]) {
    return g_app->geschuetzSim.steht(static_cast<int>(ent), ms, lo, hi);
}

// G2_TraceModels gegen das Modell eines Geschuetzes (-1: kein Netz, sonst Treffer).
int geschuetzNetzTreffer(std::size_t ent, double ms, const float start[3], const float ende[3], float radius, bool lodGrob,
                         std::vector<G2Kollision>* kollisionen) {
    const GeschuetzLauf* l = geschuetzLaufVon(ent);
    if (l == nullptr) { return -1; }
    const App::ActorAssets* as = gegenstandGlmFuer(geschuetzModellPfad(*l));
    if (as == nullptr) { return -1; }
    const GeschuetzKnochen k = geschuetzKnochenBei(*l, ms);
    std::vector<BoneMatrix> welt;
    std::vector<BoneMatrix> skin;
    if (!geschuetzSkelett(*l, *as, k, ms, welt, skin)) { return -1; }
    float achse[3][3];
    winkelZuAchsen(k.winkel, achse);
    float roh[16]{};
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) { roh[r * 4 + c] = achse[c][r] * l->skala; }
        roh[r * 4 + 3] = k.ort[r];
    }
    roh[15] = 1.0F;
    float fw[16];
    g2WurzelWelt(roh, fw);
    std::vector<float> ecken;
    std::vector<std::uint32_t> idx;
    int treffer = 0;
    const GlmModel& m = as->model;
    for (std::size_t si = 0; si < m.surfaces.size(); ++si) {
        const GlmSurface& sf = m.surfaces[si];
        if (sf.skinnedOff || sf.flags != 0U) { continue; }
        const std::vector<GlmVertex>& verts = lodGrob && !sf.grobVerts.empty() ? sf.grobVerts : sf.verts;
        const std::vector<std::uint32_t>& ix = lodGrob && !sf.grobVerts.empty() ? sf.grobIndexes : sf.indexes;
        if (verts.empty()) { continue; }
        g2Ecken(verts, skin, fw, ecken);
        idx.assign(ix.begin(), ix.end());
        if (kollisionen != nullptr) {
            G2Kollision vorlage;
            vorlage.flaeche = static_cast<int>(si);
            vorlage.entity = static_cast<int>(ent);
            treffer += radius == 0.0F ? g2StrahlKollisionen(ecken, idx, start, ende, vorlage, *kollisionen)
                                      : g2RadiusKollisionen(ecken, idx, start, ende, radius, vorlage, *kollisionen);
        } else if (g2RadiusTreffer(ecken, idx, start, ende, radius)) {
            return 1;
        }
    }
    return treffer;
}

// gi.inPVS (SV_inPVS, sv_game.cpp OpenJK): die Zeile des Clusters von a, das Bit des
// Clusters von b, dazu CM_AreasConnected: ein Punkt im Festen (Area -1) sieht nichts.
// Ohne den Stand der Areaportale: geschlossene Tueren trennen hier nicht.
bool geschuetzInPvs(const float a[3], const float b[3]) {
    const BspGeometry& geo = g_app->geo;
    if (geo.empty()) { return true; }
    const int la = geo.leafAt(a);
    const int lb = geo.leafAt(b);
    if (la < 0 || lb < 0 || static_cast<std::size_t>(la) >= geo.leafs.size() || static_cast<std::size_t>(lb) >= geo.leafs.size()) {
        return false;
    }
    const auto& ba = geo.leafs[static_cast<std::size_t>(la)];
    const auto& bb = geo.leafs[static_cast<std::size_t>(lb)];
    if (ba.area < 0 || bb.area < 0) { return false; }
    if (geo.vis.empty() || geo.clusterBytes <= 0) { return true; }
    if (bb.cluster < 0) { return false; }
    const int zeile = ba.cluster < 0 || ba.cluster >= geo.numClusters ? 0 : ba.cluster;
    const auto stelle = static_cast<std::size_t>(zeile) * static_cast<std::size_t>(geo.clusterBytes) +
                        static_cast<std::size_t>(bb.cluster >> 3);
    return stelle < geo.vis.size() && (geo.vis[stelle] & (1U << (static_cast<unsigned>(bb.cluster) & 7U))) != 0U;
}

// gi.pointcontents (SV_PointContents, sv_world.cpp): die Welt, dann die Entities am Punkt -
// Brush-Modelle, Kaesten der Figuren (CONTENTS_BODY) und Geschuetze.
std::uint32_t geschuetzInhalt(double ms, const float p[3], int ignoriere) {
    const FallenWelt& w = g_fallenWelt;
    if (!w.k) { return 0U; }
    std::uint32_t c = kollisionsInhalt(*w.k, p);
    for (const FallenBrush& b : w.brushes) {
        if (static_cast<int>(b.entity) == ignoriere) { continue; }
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
        c |= kollisionsInhalt(*w.k, p, b.modell, ort, winkel);
    }
    constexpr std::uint32_t kKoerper = 0x00000100U;   // CONTENTS_BODY
    for (const std::size_t ai : w.figuren) {
        const Actor& a = g_app->scene.actors[ai];
        if (kSchussFigur + static_cast<int>(ai) == ignoriere) { continue; }
        const ActorState st = a.at(ms);
        if (!st.visible || st.removed) { continue; }
        const float hb = a.halbBreite;
        if (p[0] >= st.pos[0] - hb && p[0] <= st.pos[0] + hb && p[1] >= st.pos[1] - hb && p[1] <= st.pos[1] + hb &&
            p[2] >= st.pos[2] - 24.0F && p[2] <= st.pos[2] + (st.crouched ? 16.0F : a.hoeheOben)) {
            c |= kKoerper;
        }
    }
    for (const GeschuetzLauf& l : g_app->geschuetzSim.laeufe()) {
        if (l.daten.nummer == ignoriere) { continue; }
        float lo[3];
        float hi[3];
        if (!g_app->geschuetzSim.steht(l.daten.nummer, ms, lo, hi)) { continue; }
        if (p[0] >= lo[0] && p[0] <= hi[0] && p[1] >= lo[1] && p[1] <= hi[1] && p[2] >= lo[2] && p[2] <= hi[2]) {
            c |= l.daten.art == GeschuetzArt::Wache ? 0x00000280U : kKoerper;   // SHOTCLIP|CORPSE bzw. BODY
        }
    }
    return c;
}

// G_GetHitLocFromSurfName: was die Formel von der Figur liest.
int geschuetzTrefferOrt(int ziel, double ms, const std::string& flaeche, const float punkt[3]) {
    const auto ai = static_cast<std::size_t>(ziel - kSchussFigur);
    if (ai >= g_app->scene.actors.size() || ai >= g_app->actorAssets.size() || !g_app->actorAssets[ai]) { return kOrtKeiner; }
    const Actor& a = g_app->scene.actors[ai];
    const App::ActorAssets& assets = *g_app->actorAssets[ai];
    TrefferOrtLage lage;
    lage.klasse = a.klasse;
    lage.npcType = a.npcType;
    ActorDraw d;
    d.figur = static_cast<int>(ai);
    std::vector<BoneMatrix> welt;
    float fw[16];
    if (figurPose(a, assets, a.at(ms), ms, d, welt, fw)) {
        const auto bolzen = [&](const char* name, bool& hat, float o[3]) {
            BoneMatrix bm{};
            hat = boltMatrix(*d.model, surfaceIndex(*d.model, name), welt, *d.anim, bm);
            if (!hat) { return; }
            for (int r = 0; r < 3; ++r) {
                o[r] = fw[r * 4 + 0] * bm.m[0][3] + fw[r * 4 + 1] * bm.m[1][3] + fw[r * 4 + 2] * bm.m[2][3] + fw[r * 4 + 3];
            }
        };
        bolzen("*hips_l_knee", lage.knieL, lage.knieLOrt);
        bolzen("*hips_r_knee", lage.knieR, lage.knieROrt);
        bolzen("*l_hand", lage.handL, lage.handLOrt);
        bolzen("*r_hand", lage.handR, lage.handROrt);
        bolzen("*l_leg_foot", lage.fussL, lage.fussLOrt);
        bolzen("*r_leg_foot", lage.fussR, lage.fussROrt);
        // chestBolt "thoracic": renderInfo.torsoPoint
        const int bk = boneIndex(*d.anim, "thoracic");
        if (bk >= 0 && static_cast<std::size_t>(bk) < welt.size()) {
            const BoneMatrix& bm = welt[static_cast<std::size_t>(bk)];
            for (int r = 0; r < 3; ++r) {
                lage.rumpfOrt[r] = fw[r * 4 + 0] * bm.m[0][3] + fw[r * 4 + 1] * bm.m[1][3] + fw[r * 4 + 2] * bm.m[2][3] + fw[r * 4 + 3];
            }
        }
    }
    (void)figurRumpfWinkel(a, assets, ms, lage.rumpfWinkel);
    return trefferOrtAusFlaeche(lage, flaeche, punkt, geschuetzRegelwerk());
}

// Name der Flaeche einer G2-Kollision (G2API_GetSurfaceName) im ghoul2-Feld ihrer Entity.
std::string geschuetzFlaechenName(const G2Kollision& k) {
    const GlmModel* m = nullptr;
    if (k.entity >= kSchussFigur) {
        const auto ai = static_cast<std::size_t>(k.entity - kSchussFigur);
        if (ai < g_app->actorAssets.size() && g_app->actorAssets[ai] && k.modell == 0) { m = &g_app->actorAssets[ai]->model; }
    } else if (const GeschuetzLauf* l = geschuetzLaufVon(static_cast<std::size_t>(k.entity))) {
        if (const App::ActorAssets* as = gegenstandGlmFuer(geschuetzModellPfad(*l))) { m = &as->model; }
    }
    if (m == nullptr || k.flaeche < 0 || static_cast<std::size_t>(k.flaeche) >= m->surfaces.size()) { return {}; }
    return m->surfaces[static_cast<std::size_t>(k.flaeche)].name;
}

}  // namespace

GeschuetzWelt geschuetzWeltJetzt() {
    GeschuetzWelt w;
    w.spur = [](double ms, const float start[3], const float mins[3], const float maxs[3], const float ende[3], int ignoriere, bool g2) {
        SchussSpurArt art;
        if (ignoriere >= kSchussFigur) {
            art.selbstFigur = ignoriere - kSchussFigur;
        } else if (ignoriere >= 0) {
            art.selbst = static_cast<std::size_t>(ignoriere);
        }
        std::vector<G2Kollision> koll;
        art.g2 = g2;
        art.lodGrob = g2;   // traceLod 10: G2_DecideTraceLod nimmt die groebste Stufe
        if (g2) { art.kollisionen = &koll; }
        return schussSpurBei(ms, start, mins, maxs, ende, art);
    };
    w.pvs = [](const float a[3], const float b[3]) { return geschuetzInPvs(a, b); };
    // Die Zeit ist die des laufenden Bilds.
    w.inhalt = [](const float p[3], int ignoriere) { return geschuetzInhalt(g_app->geschuetzBildMs, p, ignoriere); };
    w.bolzen = [](std::size_t i, double ms, const GeschuetzKnochen& k, const char* name, float m[3][4]) {
        const std::vector<GeschuetzLauf>& alle = g_app->geschuetzSim.laeufe();
        return i < alle.size() && geschuetzBolzenMatrix(alle[i], ms, k, name, m);
    };
    w.trefferOrt = [](int ziel, double ms, const std::string& flaeche, const float punkt[3]) {
        return geschuetzTrefferOrt(ziel, ms, flaeche, punkt);
    };
    w.nimmtSchaden = [](int entity, double) { return g_app->schadenZiele.count(entity) != 0U; };
    return w;
}

namespace {

// Die Effekte der Simulation als feste Instanzen (G_PlayEffect): Muendungsblitz,
// Einschlag (CG_MissileHitWall/CG_MissileHitPlayer), Funken und Explosion.
void geschuetzEffekteSetzen() {
    g_app->effectRunners.erase(std::remove_if(g_app->effectRunners.begin(), g_app->effectRunners.end(),
                                              [](const EffectInstance& x) { return x.ausGeschuetz; }),
                               g_app->effectRunners.end());
    g_app->geschuetzSchuesse = 0;
    g_app->geschuetzEinschlaege = 0;
    const auto setze = [&](const std::string& name, double ms, const float ort[3], const float richtung[3], unsigned saat) {
        const std::string pfad = "effects/" + name + ".efx";
        if (!ladeEffektDatei(pfad, name)) { return false; }
        EffectInstance x;
        x.effect = &g_app->effects[pfad];
        x.startMs = ms;
        x.seed = saat | 1U;
        x.ausGeschuetz = true;
        for (int q = 0; q < 3; ++q) {
            x.origin[q] = ort[q];
            x.normal[q] = richtung[q];
        }
        g_app->effectRunners.push_back(x);
        return true;
    };
    unsigned n = 1U;
    for (const GeschuetzLauf& l : g_app->geschuetzSim.laeufe()) {
        for (const GeschuetzSchuss& s : l.schuesse) {
            if (setze(s.effekt, s.ms, s.ort, s.richtung, n++ * 2654435761U)) { ++g_app->geschuetzSchuesse; }
        }
        for (const GeschuetzEffekt& e : l.effekte) { (void)setze(e.effekt, e.ms, e.ort, e.richtung, n++ * 2654435761U); }
    }
    for (const Geschoss& g : g_app->geschuetzSim.geschosse()) {
        if (!g.einschlag) { continue; }
        // WP_TIE_FIGHTER hat keinen Fall in CG_MissileHitWall/CG_MissileHitPlayer.
        const char* name = nullptr;
        if (g.waffe == 0) { name = g.fleisch ? "blaster/flesh_impact" : "blaster/wall_impact"; }
        if (g.waffe == 1) { name = g.fleisch ? "turret/flesh_impact" : "turret/wall_impact"; }
        if (name != nullptr && setze(name, g.endeMs, g.ende, g.normale, n++ * 2654435761U)) { ++g_app->geschuetzEinschlaege; }
    }
}

}  // namespace

// Was die Simulation der Zeichnung und den Selbsttests hinterlaesst.
void geschuetzErgebnisUebernehmen() {
    g_app->pasSchwenks.clear();
    for (const GeschuetzLauf& l : g_app->geschuetzSim.laeufe()) {
        if (l.daten.art != GeschuetzArt::Wache) { continue; }
        const auto i = static_cast<std::size_t>(l.daten.nummer);
        std::vector<KnochenAnim>& anims = g_app->geschuetzAnims[i];
        anims.clear();
        if (!l.aufklappen.empty()) { anims.push_back(KnochenAnim{l.aufklappen.front().first, 0, 11, 0.8F}); }
        std::vector<PasSchwenk>& p = g_app->pasSchwenks[i];
        if (l.aufklappen.empty()) { continue; }
        for (const GeschuetzStand& s : l.staende) {
            if (s.gedacht && s.ms > l.aufklappen.front().first) { p.push_back(PasSchwenk{s.ms, s.pasGier}); }
        }
    }
    geschuetzEffekteSetzen();
}

// Was sonst Schaden nimmt und zerbricht.
std::vector<BruchZiel> bruchZieleDerKarte() {
    std::vector<BruchZiel> bruchziele;
    const MapData& karte = g_app->map;
    const auto wert = [](const MapEntity& e, const char* key) {
        const std::string* v = e.find(key);
        return v != nullptr ? std::atoi(v->c_str()) : 0;
    };
    for (const FallenBrush& b : g_fallenWelt.brushes) {
        const MapEntity& e = karte.entities[b.entity];
        if (e.classname != "func_breakable" || static_cast<std::size_t>(b.modell) >= g_app->geo.models.size()) { continue; }
        const int sf = wert(e, "spawnflags");
        BruchZiel z;
        z.entity = static_cast<int>(b.entity);
        z.health = wert(e, "health");
        if ((sf & 1) == 0 && z.health == 0) { z.health = 10; }
        if (z.health == 0) { continue; }
        z.nurSchwert = (sf & 16) != 0;
        z.nurSchwer = (sf & 16) == 0 && (sf & 32) != 0;
        const BspGeometry::SubModel& sm = g_app->geo.models[static_cast<std::size_t>(b.modell)];
        for (int q = 0; q < 3; ++q) {
            z.lo[q] = b.ort0[q] + sm.mins[q] - 1.0F;
            z.hi[q] = b.ort0[q] + sm.maxs[q] + 1.0F;
            z.splashOrt[q] = z.lo[q] + 0.5F * (z.hi[q] - z.lo[q]);   // funcBBrushDieGo: VectorMA(absmin, 0.5, size, org)
        }
        z.splashSchaden = wert(e, "splashDamage");
        z.splashWeite = static_cast<float>(wert(e, "splashRadius"));
        z.bruchVerzug = wert(e, "delay") * 1000.0;
        bruchziele.push_back(z);
    }
    for (const FallenModell& m : g_fallenWelt.modelle) {
        const MapEntity& e = karte.entities[m.entity];
        BruchZiel z;
        z.entity = static_cast<int>(m.entity);
        z.health = wert(e, "health");
        if (z.health == 0) { continue; }
        float o[3] = {0.0F, 0.0F, 0.0F};
        if (!e.origin.empty()) { (void)std::sscanf(e.origin.c_str(), "%f %f %f", &o[0], &o[1], &o[2]); }
        for (int q = 0; q < 3; ++q) {
            z.lo[q] = m.mins[q] - 1.0F;
            z.hi[q] = m.maxs[q] + 1.0F;
            z.splashOrt[q] = o[q];
        }
        // misc_model_breakable_die: "origin is going to be below it or very very low in the model"
        if (m.mins[2] - o[2] > -4.0F) { z.splashOrt[2] = m.mins[2] + (m.maxs[2] - m.mins[2]) / 2.0F; }
        z.splashSchaden = wert(e, "splashDamage");
        z.splashWeite = static_cast<float>(wert(e, "splashRadius"));
        bruchziele.push_back(z);
    }
    for (const FallenMine& m : g_fallenWelt.minen) {
        BruchZiel z;
        z.entity = static_cast<int>(m.entity);
        z.health = 15;
        z.mine = true;
        z.abMs = m.schadenAb;
        z.bisMs = m.wegMs;
        for (int q = 0; q < 3; ++q) {
            z.lo[q] = m.ort[q] - 5.0F;
            z.hi[q] = m.ort[q] + 5.0F;
        }
        bruchziele.push_back(z);
    }
    return bruchziele;
}

// Die Geschuetze neu aufstellen (die uses der Mover-Simulation) und ohne Ziele laufen
// lassen: so stehen sie auch ohne Figuren in der Szene (hurtTodeBerechnen rechnet mit
// Figuren noch einmal).
void geschuetzeStarten() {
    g_app->geschuetzSim.setze(geschuetzDatenDerKarte(), geschuetzRegelwerk(), kNavKartenSpawnMs, !regelwerkMovieDuels());
}

void geschuetzeOhneZieleRechnen() {
    geschuetzeStarten();
    if (g_app->geschuetzSim.laeufe().empty()) {
        geschuetzEffekteSetzen();
        return;
    }
    const double ende = std::max(g_app->camTrack.durationMs, g_app->timeline.practicalEndMs) + 5000.0;
    std::vector<SchadenAuftrag> auftraege;
    for (double t = kNavKartenSpawnMs; t <= ende + 1e-6; t += 50.0) { g_app->geschuetzSim.bild(t, {}, GeschuetzWelt{}, auftraege); }
    geschuetzErgebnisUebernehmen();
}

// Die Geschosse in der Luft: FX_BlasterProjectileThink und FX_TurretProjectileThink
// spielen ihren Effekt in JEDEM Bild an lerpOrigin, die Richtung in den ersten
// 75 ms ab trTime verkuerzt (FX_Blaster.cpp:57-69, FX_Emplaced.cpp:150-163 OpenJK).
void geschuetzGeschosseZeichnen(double jetzt, std::vector<EffectInstance>& aus) {
    unsigned n = 1U;
    for (const Geschoss& g : g_app->geschuetzSim.geschosse()) {
        ++n;
        if (jetzt < g.abMs || (g.endeMs >= 0.0 && jetzt >= g.endeMs) || g.bahnen.empty()) { continue; }
        const GeschossBahn* b = &g.bahnen.front();
        for (const GeschossBahn& x : g.bahnen) {
            if (x.abMs > jetzt) { break; }
            b = &x;
        }
        const char* name = g.waffe == 0 ? "blaster/NPCshot" : g.waffe == 1 ? "turret/shot" : "ships/imp_blastershot";
        const efx::Effect* fx = effectByName(name);
        if (fx == nullptr) { continue; }
        EffectInstance x;
        x.effect = fx;
        x.startMs = jetzt;
        x.seed = (n * 2654435761U) | 1U;
        const auto d = static_cast<float>((jetzt - b->zeit) * 0.001);
        float laenge = 0.0F;
        for (int q = 0; q < 3; ++q) {
            x.origin[q] = b->basis[q] + d * b->delta[q];
            x.normal[q] = b->delta[q];
            laenge += b->delta[q] * b->delta[q];
        }
        laenge = std::sqrt(laenge);
        if (laenge == 0.0F) {
            x.normal[2] = 1.0F;
        } else {
            for (float& v : x.normal) { v /= laenge; }
        }
        int dif = static_cast<int>(jetzt - b->zeit);
        if (dif < 75) {
            if (dif < 0) { dif = 0; }
            const float skala = (static_cast<float>(dif) / 75.0F) * 0.95F + 0.05F;
            for (float& v : x.normal) { v *= skala; }
        }
        aus.push_back(x);
    }
}
