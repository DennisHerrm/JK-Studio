// selbsttest_wegerzeugung.inc - rc609 W3a: Wegpunkt-Erzeugung in Innenraeumen und auf Treppen (BHED_EDITORTEST=wegerzeugung)
//
// Eingebunden an genau dieser Stelle von gui/selbsttest.cpp (im selben Namensraum, mit allen Helfern davor).
//
// Mike (03.11.): "Probably had the most trouble with the Jedi Temple Garden as it ignored all the interiors for some
// reason" und "It avoids adding waypoints to stairs/slopes ... Maybe it can try to connect the waypoints at the top and
// bottom of stairs/slopes." Die Karte kommt aus der Umgebung:
//   BHED_WE_PK3    eine .pk3 ausserhalb der Spielordner (Kopie im Laufordner), sonst aus den Spielordnern
//   BHED_WE_KARTE  Name unter maps/ ohne .bsp (Vorgabe jeditemple_garden), z. B. mp/duel5
// Gemessen wird unabhaengig von der Erzeugung, aus der Karte:
//   - Innenraum: begehbarer Boden (Kasten des Spiels passt), ueber dem ein Strahl nach oben zuerst eine Flaeche trifft,
//     die kein Himmel ist, und die Kollision dort fest ist (kein Laub ohne Koerper)
//   - Dach: begehbarer Boden unter Himmel, unter dem in derselben Spalte ein Innenraum liegt
//   - Treppe: eine gerade Linie ueber den Boden, die in Stufen bis STEPSIZE (18) mindestens um MAX_EDGE_FLOOR_DIST
//     (60) steigt, mit Absatz oben und unten
// Geprueft: Punkte in den Innenraeumen, oben und unten an jeder Treppe ein Punkt und beide ueber das Netz verbunden,
// jede erzeugte Kante besteht die Kantenpruefung des Spiels (NavKantenPruefer).
namespace we {

constexpr float kStufe = 18.0F;        // STEPSIZE (bg_public.h)
constexpr float kEtage = 60.0F;        // MAX_EDGE_FLOOR_DIST (g_navigator.cpp)
constexpr float kBodenNormale = 0.7F;  // MIN_WALK_NORMAL (bg_public.h)
constexpr float kProbe = 64.0F;        // Raster der Messung (ein Drittel der Rasterweite der Erzeugung)
const char* const kFelder[4] = {"target", "target2", "target3", "target4"};

struct Probe {
    std::array<float, 3> boden{};
    bool innen = false;
    bool dach = false;
    int gebiet = -1;       // Area des Blatts (getrennt durch Area-Portale)
    int ix = 0;            // Spalte im Messraster
    int iy = 0;
    bool erreichbar = false;   // zu Fuss vom Spielerstart oder einer Figur aus (Tueren offen)
    float normaleZ = 1.0F;
};
struct Treppe {
    std::array<float, 3> unten{};
    std::array<float, 3> oben{};
};
struct Messung {
    std::vector<Probe> proben;
    std::vector<Treppe> treppen;
    int startGebiet = -1;   // Gebiet des Spielerstarts
};

TraceTreffer strahl(const std::array<float, 3>& a, const std::array<float, 3>& b) { return traceRay(g_app->geo, a.data(), b.data()); }

// Die Kollision des Spiels, Tueren offen (sie oeffnen sich vor der Figur): fuer die Messung, was zu Fuss erreichbar ist.
struct Welt {
    NavBauHilfe h;
    NavVersetzt ohneTueren;
};
Welt welt() {
    Welt w;
    w.h = navBauHilfeKarte();
    for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
        if (g_app->map.entities[i].classname == "func_door") { w.ohneTueren.push_back(NavVersetztEintrag{static_cast<int>(i), {}, false}); }
    }
    return w;
}
// Steht der Kasten einer Figur (DEFAULT_MINS/MAXS) auf diesem Boden, ohne im Festen zu stecken?
bool stehtFrei(const Welt& w, const std::array<float, 3>& b) {
    // Der Kasten kommt von einer Stufe darueber herab und liegt auf (auf einer Treppe auf der Stufenkante unter ihm).
    const NavVec o{b[0], b[1], b[2] + 24.0F + kStufe};
    const NavVec u{b[0], b[1], b[2] + 24.0F - kStufe};
    const NavVec mi{-16.0F, -16.0F, -24.0F};
    const NavVec ma{16.0F, 16.0F, 40.0F};
    const NavSpur s = w.h.spurVersetzt ? w.h.spurVersetzt(o, mi, ma, u, kNavKeineEntity, kNavMaskNpcSolid, w.ohneTueren)
                                       : w.h.spur(o, mi, ma, u, kNavKeineEntity, kNavMaskNpcSolid);
    return !s.startsolid && !s.allsolid && s.fraction < 1.0F;
}

// Alle Boeden einer Spalte von oben nach unten.
std::vector<std::array<float, 4>> boeden(float x, float y, float zOben, float zUnten) {
    std::vector<std::array<float, 4>> aus;
    float z = zOben;
    for (int runde = 0; runde < 32 && z > zUnten; ++runde) {
        const TraceTreffer t = strahl({x, y, z}, {x, y, zUnten});
        if (!t.hit) { break; }
        if (t.normal[2] >= kBodenNormale) { aus.push_back({t.point[0], t.point[1], t.point[2], t.normal[2]}); }
        z = t.point[2] - 2.0F;
    }
    return aus;
}

// Ueber dem Boden zuerst eine Flaeche ohne Himmel, und dort ist die Kollision fest?
bool unterDach(const Welt& w, const std::array<float, 3>& b) {
    const std::array<float, 3> von{b[0], b[1], b[2] + 40.0F};
    const std::array<float, 3> bis{b[0], b[1], b[2] + 8192.0F};
    const TraceTreffer t = strahl(von, bis);
    if (!t.hit || t.surface < 0) { return false; }
    const int sh = g_app->geo.surfaces[static_cast<std::size_t>(t.surface)].shader;
    if (sh >= 0 && sh < static_cast<int>(g_app->geo.shaders.size()) && g_app->geo.shaders[static_cast<std::size_t>(sh)].isSky()) {
        return false;
    }
    const NavSpur s = w.h.spur({von[0], von[1], von[2]}, {0, 0, 0}, {0, 0, 0}, {t.point[0], t.point[1], t.point[2] + 16.0F},
                             kNavKeineEntity, kNavSolid);
    return s.fraction < 1.0F || s.startsolid;
}

// Der Boden in der Naehe von zNah an (x, y): der hoechste nicht ueber zNah + Stufe.
bool bodenBei(float x, float y, float zNah, std::array<float, 3>& aus) {
    const TraceTreffer t = strahl({x, y, zNah + kStufe + 1.0F}, {x, y, zNah - kStufe - 1.0F});
    if (!t.hit || t.normal[2] < kBodenNormale) { return false; }
    aus = {t.point[0], t.point[1], t.point[2]};
    return true;
}

// Eine Treppe ab b in Richtung (dx, dy): gerade Linie in Schritten von 4, jede Hoehenaenderung bis STEPSIZE, nur
// aufwaerts, mindestens drei echte Stufen (ueber 4 hoch) und zusammen mindestens MAX_EDGE_FLOOR_DIST; davor und
// danach ein ebener Absatz von 32.
bool treppeAb(const Welt& h, const std::array<float, 3>& b, float dx, float dy, Treppe& t) {
    constexpr float kSchritt = 4.0F;
    constexpr float kAbsatz = 32.0F;
    // Absatz unten: eben von b bis zur ersten Stufe, die Stufe beginnt in dieser Messzelle.
    std::array<float, 3> p = b;
    float s = 0.0F;
    float ebenVorher = 0.0F;
    for (;;) {
        std::array<float, 3> q{};
        if (s + kSchritt > kProbe || !bodenBei(b[0] + dx * (s + kSchritt), b[1] + dy * (s + kSchritt), p[2], q)) { return false; }
        if (std::fabs(q[2] - p[2]) > 1.0F) { break; }
        s += kSchritt;
        ebenVorher += kSchritt;
        p = q;
    }
    if (ebenVorher < kAbsatz) { return false; }
    const std::array<float, 3> unten = p;
    int stufen = 0;
    float eben = 0.0F;
    std::array<float, 3> letzteStufe = p;
    for (s += kSchritt; s <= 1024.0F; s += kSchritt) {
        std::array<float, 3> q{};
        if (!bodenBei(b[0] + dx * s, b[1] + dy * s, p[2], q)) { return false; }
        const float dz = q[2] - p[2];
        if (dz < -1.0F) { return false; }
        if (dz > 4.0F) {
            ++stufen;
            eben = 0.0F;
            letzteStufe = q;
        } else if (dz > 1.0F) {
            return false;   // eine Schraege, keine Stufe
        } else {
            eben += kSchritt;
        }
        p = q;
        if (stufen > 0 && eben >= kAbsatz) {
            if (stufen < 3 || letzteStufe[2] - unten[2] < kEtage) { return false; }
            t.unten = unten;
            t.oben = p;
            return stehtFrei(h, t.unten) && stehtFrei(h, t.oben);
        }
    }
    return false;
}

// Von Probe a zur Nachbarprobe b zu Fuss: alle 4 Einheiten der Boden hoechstens eine Stufe hoeher oder tiefer, alle 16
// steht der Kasten frei, am Ende derselbe Boden wie b.
bool gehbar(const Welt& w, const Probe& a, const Probe& b) {
    std::array<float, 3> p = a.boden;
    const float dx = b.boden[0] - a.boden[0];
    const float dy = b.boden[1] - a.boden[1];
    const float lang = std::hypot(dx, dy);
    for (float s = 4.0F; s <= lang + 0.01F; s += 4.0F) {
        std::array<float, 3> q{};
        if (!bodenBei(a.boden[0] + dx * s / lang, a.boden[1] + dy * s / lang, p[2], q)) { return false; }
        p = q;
        if (std::fmod(s, 16.0F) < 0.01F && !stehtFrei(w, p)) { return false; }
    }
    return std::fabs(p[2] - b.boden[2]) <= 1.0F;
}

Messung messen() {
    Messung m;
    const Welt h = welt();
    if (!h.h.spur || g_app->geo.models.empty()) { return m; }
    const auto& w = g_app->geo.models.front();
    std::map<std::pair<int, int>, std::vector<int>> spalten;
    int ix = 0;
    for (float x = w.mins[0] + kProbe * 0.5F; x < w.maxs[0]; x += kProbe, ++ix) {
        int iy = 0;
        for (float y = w.mins[1] + kProbe * 0.5F; y < w.maxs[1]; y += kProbe, ++iy) {
            const std::vector<std::array<float, 4>> spalte = boeden(x, y, w.maxs[2] - 1.0F, w.mins[2] + 1.0F);
            bool innenDarunter = false;
            std::vector<Probe> hier;
            for (auto it = spalte.rbegin(); it != spalte.rend(); ++it) {   // von unten nach oben
                const std::array<float, 3> bo{(*it)[0], (*it)[1], (*it)[2]};
                if (!stehtFrei(h, bo)) { continue; }
                Probe p;
                p.boden = bo;
                p.normaleZ = (*it)[3];
                p.innen = unterDach(h, bo);
                const float mitte[3] = {it->at(0), it->at(1), it->at(2) + 25.0F};
                const int blatt = g_app->geo.leafAt(mitte);
                p.gebiet = blatt >= 0 ? g_app->geo.leafs[static_cast<std::size_t>(blatt)].area : -1;
                p.dach = !p.innen && innenDarunter;
                p.ix = ix;
                p.iy = iy;
                innenDarunter = innenDarunter || p.innen;
                hier.push_back(p);
            }
            for (const Probe& p : hier) {
                spalten[{ix, iy}].push_back(static_cast<int>(m.proben.size()));
                m.proben.push_back(p);
                static const float kRicht[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
                for (const auto& r : kRicht) {
                    Treppe t;
                    if (!treppeAb(h, p.boden, r[0], r[1], t)) { continue; }
                    bool neu = true;   // dieselbe Treppe aus einer Nachbarspalte
                    for (const Treppe& a : m.treppen) {
                        const float du = std::hypot(a.unten[0] - t.unten[0], a.unten[1] - t.unten[1]);
                        const float dob = std::hypot(a.oben[0] - t.oben[0], a.oben[1] - t.oben[1]);
                        if ((du < 256.0F && std::fabs(a.unten[2] - t.unten[2]) < kStufe) ||
                            (dob < 256.0F && std::fabs(a.oben[2] - t.oben[2]) < kStufe)) {
                            neu = false;
                        }
                    }
                    if (neu) { m.treppen.push_back(t); }
                }
            }
        }
    }
    // Zu Fuss erreichbar: vom Spielerstart und den Figuren der Karte aus (wie bei Mike: Startpunkte der Karte).
    std::deque<int> offen;
    for (const MapEntity& e : g_app->map.entities) {
        float o[3];
        const bool start = e.classname.rfind("info_player", 0) == 0 || e.classname.rfind("NPC_", 0) == 0 || e.classname.rfind("npc_", 0) == 0;
        if (!start || !entityOrt(e, o)) { continue; }
        int best = -1;
        float bestD = 96.0F;
        for (std::size_t i = 0; i < m.proben.size(); ++i) {
            const auto& b = m.proben[i].boden;
            const float d = std::hypot(std::hypot(b[0] - o[0], b[1] - o[1]), b[2] - (o[2] - 24.0F));
            if (d < bestD) {
                bestD = d;
                best = static_cast<int>(i);
            }
        }
        if (best < 0 || m.proben[static_cast<std::size_t>(best)].erreichbar) { continue; }
        m.proben[static_cast<std::size_t>(best)].erreichbar = true;
        if (e.classname == "info_player_start" || m.startGebiet < 0) { m.startGebiet = m.proben[static_cast<std::size_t>(best)].gebiet; }
        offen.push_back(best);
    }
    static const int kNachbar[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    while (!offen.empty()) {
        const int i = offen.front();
        offen.pop_front();
        const Probe a = m.proben[static_cast<std::size_t>(i)];
        for (const auto& r : kNachbar) {
            const auto it = spalten.find({a.ix + r[0], a.iy + r[1]});
            if (it == spalten.end()) { continue; }
            for (const int j : it->second) {
                Probe& b = m.proben[static_cast<std::size_t>(j)];
                if (b.erreichbar || std::fabs(b.boden[2] - a.boden[2]) > kProbe + kStufe || !gehbar(h, a, b)) { continue; }
                b.erreichbar = true;
                offen.push_back(j);
            }
        }
    }
    return m;
}

// Die Wegpunkte (waypoint, waypoint_small) mit Bodenpunkt (origin - 25: der Kasten beginnt 1 ueber dem Boden).
struct Punkt {
    int entity = -1;
    std::array<float, 3> ort{};
};
std::vector<Punkt> punkte() {
    std::vector<Punkt> aus;
    for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
        const MapEntity& e = g_app->map.entities[i];
        float o[3];
        if ((e.classname == "waypoint" || e.classname == "waypoint_small") && entityOrt(e, o)) {
            aus.push_back(Punkt{static_cast<int>(i), {o[0], o[1], o[2]}});
        }
    }
    return aus;
}
// Ein Wegpunkt liegt bei einer Probe: waagerecht im halben Messraster, Boden bis eine Stufe darueber oder darunter.
bool beiProbe(const Punkt& w, const std::array<float, 3>& b, float waagerecht) {
    return std::hypot(w.ort[0] - b[0], w.ort[1] - b[1]) <= waagerecht && std::fabs(w.ort[2] - 25.0F - b[2]) <= kStufe + 8.0F;
}

// Nachbarn ueber target..target4 (das Spiel verbindet in beide Richtungen).
std::map<int, std::vector<int>> netz() {
    std::map<std::string, int> name;
    for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
        if (!g_app->map.entities[i].targetname.empty()) { name[g_app->map.entities[i].targetname] = static_cast<int>(i); }
    }
    std::map<int, std::vector<int>> n;
    for (const Punkt& p : punkte()) {
        for (const char* f : kFelder) {
            const std::string* t = g_app->map.entities[static_cast<std::size_t>(p.entity)].find(f);
            if (t == nullptr) { continue; }
            const auto it = name.find(*t);
            if (it == name.end()) { continue; }
            n[p.entity].push_back(it->second);
            n[it->second].push_back(p.entity);
        }
    }
    return n;
}
// Kuerzester Weg in Kanten zwischen a und b (-1: keiner).
int schritte(const std::map<int, std::vector<int>>& n, int a, int b) {
    std::map<int, int> tiefe{{a, 0}};
    std::deque<int> offen{a};
    while (!offen.empty()) {
        const int i = offen.front();
        offen.pop_front();
        if (i == b) { return tiefe[i]; }
        const auto it = n.find(i);
        if (it == n.end()) { continue; }
        for (const int j : it->second) {
            if (tiefe.count(j) == 0U) {
                tiefe[j] = tiefe[i] + 1;
                offen.push_back(j);
            }
        }
    }
    return -1;
}

std::string zahl(float v) { return std::to_string(static_cast<int>(std::lround(v))); }
std::string ortText(const std::array<float, 3>& a) { return zahl(a[0]) + " " + zahl(a[1]) + " " + zahl(a[2]); }

}  // namespace we

std::vector<Schritt> wegerzeugungSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const char* const kk = std::getenv("BHED_WE_KARTE");
    const std::string karte = (kk != nullptr && *kk != 0) ? kk : "jeditemple_garden";
    const char* const pk = std::getenv("BHED_WE_PK3");
    const std::string pk3 = pk != nullptr ? pk : "";
    add(tu("WE: Karte " + karte, [=] {
        g_app->leftMode = 1;
        g_app->missionPickOpen = false;
        mission1::ungespeicherteEntitiesVergessen();
        if (!pk3.empty()) {
            missionenAusArchiv(pk3, karte);
        } else if (!loadMapFromArchive("maps/" + karte + ".bsp")) {
            meldeFehler("WE: maps/" + karte + ".bsp nicht geladen");
        }
    }));
    add(warteBis("WE: Karte geladen", [=] { return g_app->map.path == "maps/" + karte + ".bsp" && !g_app->geo.empty(); }, 6000));
    auto m = std::make_shared<we::Messung>();
    add(tu("WE: Innenraeume, Daecher und Treppen aus der Karte messen", [=] {
        g_app->throughCamera = false;
        g_app->playing = false;
        g_app->wegFensterOffen = true;
        g_app->zeigeWegnetz = true;
        const auto t0 = std::chrono::steady_clock::now();
        *m = we::messen();
        int innen = 0;
        int dach = 0;
        int erreichbar = 0;
        for (const we::Probe& p : m->proben) {
            innen += p.innen ? 1 : 0;
            dach += p.dach ? 1 : 0;
            erreichbar += p.erreichbar ? 1 : 0;
        }
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        diag::detail("Knopftest: WE - " + std::to_string(m->proben.size()) + " Bodenproben (Raster 64), zu Fuss erreichbar " + std::to_string(erreichbar) + ", unter Dach " +
                     std::to_string(innen) + ", Dach " + std::to_string(dach) + ", Treppen " + std::to_string(m->treppen.size()) +
                     ", " + std::to_string(static_cast<int>(ms)) + " ms");
        for (const we::Treppe& t : m->treppen) {
            diag::detail("Knopftest: WE - Treppe unten " + we::ortText(t.unten) + ", oben " + we::ortText(t.oben));
        }
    }));
    add(pruefe("WE: die Karte hat zu Fuss erreichbare Innenraeume (gemessen)", [=] {
        return std::any_of(m->proben.begin(), m->proben.end(), [&](const we::Probe& p) { return p.erreichbar && p.innen; });
    }));
    // Startpunkte wie bei Mike: die der Karte. Die Kamera zaehlt bei der Erzeugung mit - sie steht auf dem Spielerstart.
    add(tu("WE: Kamera auf den Spielerstart", [] {
        for (const MapEntity& e : g_app->map.entities) {
            float o[3];
            if (e.classname == "info_player_start" && entityOrt(e, o)) {
                for (int k = 0; k < 3; ++k) { g_app->cam.pos[k] = o[k]; }
                break;
            }
        }
    }));
    add(pause(0.3));
    add(klick(tr(Str::WegErzeugen), "###wegpunkte"));
    add(pause(0.5));
    add(tu("WE: Ergebnis", [=] { diag::detail("Knopftest: WE - " + g_app->wegMeldung); }));
    add(tu("WE: Abdeckung melden", [=] {
        // Abgedeckt: ein Wegpunkt hoechstens eine Rasterweite entfernt. Nur gemeldet, keine Schwelle (der Abstand
        // waechst mit der Punktgrenze des Spiels, die Messung des Bodens ist eine eigene).
        const std::vector<we::Punkt> wp = we::punkte();
        const float S = g_app->wegAbstandGenutzt;
        const auto bedeckt = [&](const we::Probe& p) {
            for (const we::Punkt& w : wp) {
                const float d = std::hypot(std::hypot(w.ort[0] - p.boden[0], w.ort[1] - p.boden[1]), w.ort[2] - 25.0F - p.boden[2]);
                if (d <= S) { return true; }
            }
            return false;
        };
        // Gruppen: je Innenraum (Gebiet hinter Area-Portalen, nicht das des Spielerstarts, unter Dach), unter Dach im
        // Startgebiet (-2), geneigter Boden (-3), ebener Boden draussen (-1).
        std::map<int, std::array<int, 2>> gruppe;   // -> (erreichbare Proben, abgedeckt)
        for (const we::Probe& p : m->proben) {
            if (!p.erreichbar) { continue; }
            const bool b = bedeckt(p);
            const int g = p.gebiet != m->startGebiet ? (p.innen ? p.gebiet : -4) : p.innen ? -2 : p.normaleZ >= 0.999F ? -1 : -4;
            if (g != -4) {
                ++gruppe[g][0];
                gruppe[g][1] += b ? 1 : 0;
            }
            if (p.normaleZ < 0.999F) {
                ++gruppe[-3][0];
                gruppe[-3][1] += b ? 1 : 0;
            }
        }
        // Das Dach ueber den Innenraeumen (auch ohne Weg zu Fuss): ob es Punkte bekommt, nur gemeldet.
        for (const we::Probe& p : m->proben) {
            if (!p.dach) { continue; }
            ++gruppe[-5][0];
            gruppe[-5][1] += bedeckt(p) ? 1 : 0;
        }
        for (const auto& [g, r] : gruppe) {
            const char* was = g == -1   ? "ebener Boden draussen"
                              : g == -2 ? "unter Dach im Startgebiet"
                              : g == -3 ? "geneigter Boden"
                              : g == -5 ? "Dach (alle Proben)"
                                        : "Innenraum Gebiet";
            char z[200];
            std::snprintf(z, sizeof(z), "Knopftest: WE - %s %d: %d erreichbare Proben, abgedeckt %d (%.0f %%)", was, g, r[0], r[1],
                          100.0 * r[1] / std::max(1, r[0]));
            diag::detail(z);
        }
        diag::detail("Knopftest: WE - Wegpunkte " + std::to_string(wp.size()));
    }));
    add(tu("WE: Teile, die die Erzeugung nicht anbinden konnte", [] {
        for (const WegErzeugungInfo::Luecke& l : g_app->wegLuecken) {
            diag::detail("Knopftest: WE - nicht angebunden: letzter Punkt " + we::ortText(l.anker) + ", danach Laufgitter " + we::ortText(l.weiter) +
                         "; auf dem Stueck kein Punkt " + std::to_string(l.ohnePunkt) + ", nicht gerade zu Fuss " + std::to_string(l.ohneGehen) +
                         ", Kantenpruefung des Spiels nein " + std::to_string(l.ohneKante));
        }
    }));
    add(pruefe("WE: jeder zu Fuss erreichbare Innenraum hat einen Wegpunkt in Reichweite, den die Wegfindung des Spiels vom Spielerstart erreicht", [=] {
        // Mikes Fall: Raeume ganz ausgelassen. Das Netz so, wie das Spiel es beim Laden baut (baueNavNetz: SP_waypoint
        // ... NAV::TestEdge), dann NAV::FindPath (navAStar) vom Knoten nahe dem Spielerstart zu den Knoten je Raum.
        const std::shared_ptr<NavNetz> netz = baueNavNetz(g_app->map.entities, navBauHilfeKarte());
        float st[3] = {0.0F, 0.0F, 0.0F};
        bool startDa = false;
        for (const MapEntity& e : g_app->map.entities) {
            if (!startDa && e.classname == "info_player_start" && entityOrt(e, st)) { startDa = true; }
        }
        for (const MapEntity& e : g_app->map.entities) {
            if (!startDa && e.classname.rfind("info_player", 0) == 0 && entityOrt(e, st)) { startDa = true; }
        }
        int von = -1;
        float bestD = 1.0e9F;
        for (std::size_t k = 1; k < netz->knoten.size(); ++k) {
            const NavVec& p = netz->knoten[k].punkt;
            const float d = std::hypot(std::hypot(p[0] - st[0], p[1] - st[1]), p[2] - st[2]);
            if (d < bestD) {
                bestD = d;
                von = static_cast<int>(k);
            }
        }
        // Je Raum die Knoten in Reichweite seiner erreichbaren Bodenproben (hoechstens eine Rasterweite, wie die Abdeckung).
        const float S = g_app->wegAbstandGenutzt;
        std::map<int, std::set<int>> raumKnoten;   // Gebiet -> Knoten
        std::set<int> raeume;
        for (const we::Probe& p : m->proben) {
            if (!p.erreichbar || !p.innen || p.gebiet == m->startGebiet) { continue; }
            raeume.insert(p.gebiet);
            for (std::size_t k = 1; k < netz->knoten.size(); ++k) {
                const NavVec& q = netz->knoten[k].punkt;
                if (std::hypot(std::hypot(q[0] - p.boden[0], q[1] - p.boden[1]), q[2] - 25.0F - p.boden[2]) <= S) {
                    raumKnoten[p.gebiet].insert(static_cast<int>(k));
                }
            }
        }
        int erreicht = 0;
        const std::map<int, std::vector<int>> kanten = we::netz();
        for (const int g : raeume) {
            const auto it = raumKnoten.find(g);
            const std::vector<int> liste = it != raumKnoten.end() ? std::vector<int>(it->second.begin(), it->second.end()) : std::vector<int>{};
            const int zahl = static_cast<int>(liste.size());
            bool ja = false;
            for (int i = 0; i < zahl && !ja && von > 0; ++i) { ja = liste[static_cast<std::size_t>(i)] == von || navAStar(*netz, von, liste[static_cast<std::size_t>(i)]); }
            // Zum Vergleich: ueber die target-Felder verbunden (ohne die Regeln der Laufzeit fuer Tueren)?
            int ueberZiele = -1;
            for (int i = 0; i < zahl && ueberZiele < 0 && von > 0; ++i) {
                ueberZiele = we::schritte(kanten, netz->knoten[static_cast<std::size_t>(von)].entity,
                                          netz->knoten[static_cast<std::size_t>(liste[static_cast<std::size_t>(i)])].entity);
            }
            erreicht += ja ? 1 : 0;
            diag::detail("Knopftest: WE - Innenraum Gebiet " + std::to_string(g) + ": " + std::to_string(zahl) + " Knoten in Reichweite, Wegfindung vom Spielerstart " +
                         (ja ? "ja" : "nein") + ", ueber die target-Felder " + std::to_string(ueberZiele) + " Kanten");
        }
        diag::detail("Knopftest: WE - Wegfindung des Spiels: " + std::to_string(netz->knoten.size() - 1U) + " Knoten, " +
                     std::to_string(netz->zahlKanten()) + " Kanten, Innenraeume erreicht " + std::to_string(erreicht) + " von " +
                     std::to_string(raeume.size()));
        return erreicht == static_cast<int>(raeume.size());
    }));
    add(pruefe("WE: oben und unten an jeder Treppe ein Wegpunkt in Reichweite, die Wegfindung des Spiels kommt von unten nach oben", [=] {
        // Gleicher Massstab wie bei den Innenraeumen: in Reichweite = hoechstens eine Rasterweite vom Boden des Absatzes;
        // das Netz so, wie das Spiel es baut, und NAV::FindPath (navAStar). Zum Vergleich gemeldet: Punkte in einer halben
        // Rasterweite und Kanten ueber die target-Felder.
        const std::shared_ptr<NavNetz> netz = baueNavNetz(g_app->map.entities, navBauHilfeKarte());
        const float S = g_app->wegAbstandGenutzt;
        const auto inReichweite = [&](const std::array<float, 3>& b) {
            std::vector<int> k;
            for (std::size_t i = 1; i < netz->knoten.size(); ++i) {
                const NavVec& p = netz->knoten[i].punkt;
                if (std::hypot(std::hypot(p[0] - b[0], p[1] - b[1]), p[2] - 25.0F - b[2]) <= S) { k.push_back(static_cast<int>(i)); }
            }
            return k;
        };
        const std::vector<we::Punkt> wp = we::punkte();
        const std::map<int, std::vector<int>> n = we::netz();
        int gut = 0;
        for (const we::Treppe& t : m->treppen) {
            const std::vector<int> ku = inReichweite(t.unten);
            const std::vector<int> ko = inReichweite(t.oben);
            bool ja = false;
            for (std::size_t a = 0; a < ku.size() && !ja; ++a) {
                for (std::size_t b = 0; b < ko.size() && !ja; ++b) { ja = ku[a] == ko[b] || navAStar(*netz, ku[a], ko[b]); }
            }
            int nahU = 0;
            int nahO = 0;
            int kanten = -1;
            for (const we::Punkt& w : wp) {
                nahU += we::beiProbe(w, t.unten, S * 0.5F) ? 1 : 0;
                nahO += we::beiProbe(w, t.oben, S * 0.5F) ? 1 : 0;
            }
            for (const we::Punkt& a : wp) {
                if (!we::beiProbe(a, t.unten, S * 0.5F)) { continue; }
                for (const we::Punkt& b : wp) {
                    if (!we::beiProbe(b, t.oben, S * 0.5F)) { continue; }
                    const int k = we::schritte(n, a.entity, b.entity);
                    if (k >= 0 && (kanten < 0 || k < kanten)) { kanten = k; }
                }
            }
            gut += ja ? 1 : 0;
            diag::detail("Knopftest: WE - Treppe " + we::ortText(t.unten) + " -> " + we::ortText(t.oben) + ": in Reichweite unten " +
                         std::to_string(ku.size()) + ", oben " + std::to_string(ko.size()) + ", Wegfindung " + (ja ? "ja" : "nein") +
                         "; in halber Rasterweite unten " + std::to_string(nahU) + ", oben " + std::to_string(nahO) + ", target-Kanten " +
                         std::to_string(kanten));
        }
        diag::detail("Knopftest: WE - Treppen erreicht " + std::to_string(gut) + " von " + std::to_string(m->treppen.size()));
        return gut == static_cast<int>(m->treppen.size());
    }));
    add(pruefe("WE: jede erzeugte Kante besteht die Kantenpruefung des Spiels", [=] {
        const NavBauHilfe h = navBauHilfeKarte();
        NavKantenPruefer p(g_app->map.entities, h);
        std::map<std::string, int> name;
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            if (!g_app->map.entities[i].targetname.empty()) { name[g_app->map.entities[i].targetname] = static_cast<int>(i); }
        }
        int kanten = 0;
        int schlecht = 0;
        for (const we::Punkt& w : we::punkte()) {
            const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(w.entity)];
            if (e.targetname.rfind("wpauto_", 0) != 0) { continue; }
            for (const char* f : we::kFelder) {
                const std::string* t = e.find(f);
                if (t == nullptr) { continue; }
                const auto it = name.find(*t);
                ++kanten;
                if (it == name.end() || !p.pruefe(w.entity, it->second).begehbar) {
                    if (++schlecht <= 5) { diag::detail("Knopftest: WE - Kante " + e.targetname + " -> " + *t + " besteht nicht"); }
                }
            }
        }
        diag::detail("Knopftest: WE - erzeugte Kanten " + std::to_string(kanten) + ", nicht begehbar " + std::to_string(schlecht));
        return kanten > 0 && schlecht == 0;
    }));
    return s;
}
