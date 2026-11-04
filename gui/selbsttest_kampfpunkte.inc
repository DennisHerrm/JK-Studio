// selbsttest_kampfpunkte.inc - rc609 W4: Kampfpunkte von Hand, Testroute, Abdeckung, Versetzen (BHED_EDITORTEST=kampfpunkte)
//
// Eingebunden an genau dieser Stelle von gui/selbsttest.cpp (nach selbsttest_wegmalen.inc: wm::Strecke, wm::bei).
//
// Mike (03.11.): point_combat "should be sprinkled around randomly (usually behind cover) and they automatically connect
// to waypoints (no target needed) ... I can go around the map and click areas to add a point_combat." Karte aus
// BHED_WE_PK3/BHED_WE_KARTE wie wegerzeugung. Mit der Maus in der Draufsicht, wie ein Anwender:
//   1. Warnung: ein Kampfpunkt auf einer Karte ohne Wegnetz - das Spiel meldet "has no waypoint" (size_edges() == 0)
//   2. Setzen: drei Klicks, je ein Schritt; Rueckgaengig
//   3. Zuordnung: je Punkt der Knoten aus GetNearestNode (navKampfPunkte gegen navNaechsterKnoten auf der Welt des Spiels),
//      ein freier Punkt ist selbst ein Knoten mit Auto-Kanten; einer an der Wand (kein eigener Knoten) bekommt einen
//      Wegpunkt mit freier Sicht
//   4. Duplizieren (Strg+D), Entf, Strg+Z
//   5. Testroute ueber eine Sprungkante aufs Dach; ohne Sprung die Meldung "kein Weg"
//   6. Abdeckung: eine Flaeche ohne Wegpunkt wird rot, mit Wegpunkt nicht
//   7. Versetzen: ein von Hand gesetzter Wegpunkt an der Wand - ohne Versetzen verwirft ihn das Spiel, mit Versetzen
//      steht er an einer Stelle, die das Spiel behaelt
namespace kp {

bool istWegpunkt(const MapEntity& e) { return e.classname == "waypoint" || e.classname == "waypoint_small"; }

std::vector<int> kampfpunkte() {
    std::vector<int> aus;
    for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
        if (g_app->map.entities[i].classname == "point_combat") { aus.push_back(static_cast<int>(i)); }
    }
    return aus;
}
const NavKampfPunkt* eintrag(int entity) {
    for (const NavKampfPunkt& k : g_app->wegKampf) {
        if (k.entity == entity) { return &k; }
    }
    return nullptr;
}
std::array<float, 3> ortVon(int e) {
    float o[3] = {0, 0, 0};
    (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(e)], o);
    return {o[0], o[1], o[2]};
}
// Haengen die Wegpunkte, die einem Bodenpunkt a und einem Bodenpunkt b am naechsten liegen, ueber target..target4 und
// targetJump zusammen (die Felder, die der Editor schreibt - unabhaengig vom Netzbau des Spiels)?
bool feldVerbunden(const std::array<float, 3>& a, const std::array<float, 3>& b) {
    std::map<std::string, int> name;
    const auto& v = g_app->map.entities;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (!v[i].targetname.empty()) { name[v[i].targetname] = static_cast<int>(i); }
    }
    const auto naechster = [&](const std::array<float, 3>& q) {
        int best = -1;
        float bestD = 1.0e30F;
        for (std::size_t i = 0; i < v.size(); ++i) {
            float o[3];
            if (!istWegpunkt(v[i]) || !entityOrt(v[i], o)) { continue; }
            const float d = std::hypot(std::hypot(o[0] - q[0], o[1] - q[1]), o[2] - 25.0F - q[2]);
            if (d < bestD) {
                bestD = d;
                best = static_cast<int>(i);
            }
        }
        return best;
    };
    std::map<int, std::vector<int>> n;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (!istWegpunkt(v[i])) { continue; }
        for (const char* f : {"target", "target2", "target3", "target4", "targetJump"}) {
            const std::string* t = v[i].find(f);
            const auto it = t != nullptr ? name.find(*t) : name.end();
            if (it == name.end()) { continue; }
            n[static_cast<int>(i)].push_back(it->second);
            n[it->second].push_back(static_cast<int>(i));
        }
    }
    const int von = naechster(a);
    const int nach = naechster(b);
    std::set<int> gesehen{von};
    std::vector<int> st{von};
    while (!st.empty()) {
        const int i = st.back();
        st.pop_back();
        if (i == nach) { return true; }
        for (const int j : n[i]) {
            if (gesehen.insert(j).second) { st.push_back(j); }
        }
    }
    return false;
}
// Draufsicht ueber einem Bodenpunkt, das Wegpunkt-Fenster an den rechten Rand.
void ueber(const std::array<float, 3>& b) {
    draufsichtSchalten(false);
    g_app->cam.pos[0] = b[0];
    g_app->cam.pos[1] = b[1];
    g_app->cam.pos[2] = b[2] + 64.0F;
    draufsichtSchalten(true);
    if (ImGuiWindow* f = ImGui::FindWindowByName("###wegpunkte")) {
        ImGui::SetWindowPos(f, ImVec2{g_app->kartenAnsicht[2] - f->Size.x - 8.0F, g_app->kartenAnsicht[1] + 8.0F});
    }
}
// Stellen am Rand des begehbaren Bodens (eine Nachbarprobe fehlt), nah an `nah` zuerst.
std::vector<std::array<float, 3>> randStellen(const we::Messung& m, const std::array<float, 3>& nah) {
    std::set<std::pair<int, int>> da;
    for (const we::Probe& p : m.proben) {
        if (p.erreichbar) { da.insert({p.ix, p.iy}); }
    }
    std::vector<std::pair<float, std::array<float, 3>>> liste;
    for (const we::Probe& p : m.proben) {
        if (!p.erreichbar || p.normaleZ < 0.999F) { continue; }
        const bool rand = da.count({p.ix + 1, p.iy}) == 0U || da.count({p.ix - 1, p.iy}) == 0U || da.count({p.ix, p.iy + 1}) == 0U ||
                          da.count({p.ix, p.iy - 1}) == 0U;
        if (rand) { liste.emplace_back(std::hypot(p.boden[0] - nah[0], p.boden[1] - nah[1]), p.boden); }
    }
    std::sort(liste.begin(), liste.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    std::vector<std::array<float, 3>> aus;
    for (const auto& [d, b] : liste) { aus.push_back(b); }
    return aus;
}
// Neben einer Wand: der Boden bis 40 zur Seite, bis eine Stelle kommt, an der der Kasten von SpawnedPoint steckt.
std::vector<std::array<float, 3>> wandStellen(const std::vector<std::array<float, 3>>& rand) {
    std::vector<std::array<float, 3>> aus;
    for (const auto& b : rand) {
        for (int r = 0; r < 4; ++r) {
            for (float d = 8.0F; d <= 40.0F; d += 8.0F) {
                const float dx = r == 0 ? d : r == 1 ? -d : 0.0F;
                const float dy = r == 2 ? d : r == 3 ? -d : 0.0F;
                std::array<float, 3> q{};
                if (we::bodenBei(b[0] + dx, b[1] + dy, b[2], q)) { aus.push_back(q); }
            }
        }
        if (aus.size() > 400U) { break; }
    }
    return aus;
}

}  // namespace kp

std::vector<Schritt> kampfpunkteSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const char* const kk = std::getenv("BHED_WE_KARTE");
    const std::string karte = (kk != nullptr && *kk != 0) ? kk : "jeditemple_garden";
    const char* const pk = std::getenv("BHED_WE_PK3");
    const std::string pk3 = pk != nullptr ? pk : "";
    add(tu("KP: Karte " + karte, [=] {
        g_app->leftMode = 1;
        g_app->missionPickOpen = false;
        mission1::ungespeicherteEntitiesVergessen();
        if (!pk3.empty()) {
            missionenAusArchiv(pk3, karte);
        } else if (!loadMapFromArchive("maps/" + karte + ".bsp")) {
            meldeFehler("KP: maps/" + karte + ".bsp nicht geladen");
        }
    }));
    add(warteBis("KP: Karte geladen", [=] { return g_app->map.path == "maps/" + karte + ".bsp" && !g_app->geo.empty(); }, 6000));
    auto m = std::make_shared<we::Messung>();
    auto strecke = std::make_shared<wm::Strecke>();
    auto dach = std::make_shared<std::vector<int>>();
    auto tiefe = std::make_shared<std::size_t>(0);
    auto zahl0 = std::make_shared<std::size_t>(0);
    add(tu("KP: Boden messen, Strecke und Dach suchen, Wegpunkte der Karte weg (der Anfang ohne Netz)", [=] {
        g_app->throughCamera = false;
        g_app->playing = false;
        g_app->wegFensterOffen = true;
        g_app->zeigeWegnetz = true;
        g_app->wegAutoVerbinden = true;
        g_app->wegVersetzen = true;
        g_app->wegRouteSpringen = true;
        g_app->wegAbdeckungAn = false;
        g_app->wegBereiche.clear();
        wegBereicheSpeichern();
        // Ein Anfang ohne ein einziges Wegnetz (wie jeditemple_garden, wie Mike sie bekam): was die Karte an Wegpunkten,
        // Laufzielen und Kampfpunkten mitbringt, kommt weg (ohne Rueckgaengig-Schritt, wie geladen).
        auto& v = g_app->map.entities;
        const std::size_t vor = v.size();
        v.erase(std::remove_if(v.begin(), v.end(), [](const MapEntity& e) { return istWegKlasse(e.classname); }), v.end());
        *m = we::messen();
        *strecke = wm::streckeSuchen(*m);
        *dach = we::groesstesDach(*m);
        g_app->entUndo.clear();
        diag::detail("Knopftest: KP - Karte " + g_app->map.path + ", " + std::to_string(vor - v.size()) + " Netz-Entities entfernt, " +
                     std::to_string(m->proben.size()) + " Bodenproben, Strecke ab " + (strecke->da ? we::ortText(strecke->a) : std::string("-")) +
                     ", groesstes Dach " + std::to_string(dach->size()) + " Proben");
    }));
    add(pruefe("KP: eine gerade Strecke ueber ebenen Boden gefunden", [=] { return strecke->da; }));
    const auto bei = [=](float dx, float dy) { return wm::bei(strecke, dx, dy); };
    const auto stand = [=](const std::string& was) {
        return tu("KP " + was + ": Stand", [=] {
            g_app->wegWerkzeug = 0;   // die Knoepfe der Werkzeuge schalten um: ein zweiter Klick schaltet aus
            *tiefe = g_app->entUndo.size();
            *zahl0 = g_app->map.entities.size();
        });
    };

    // --- 1. Warnung ohne Wegnetz ---------------------------------------------------------------
    add(wm::draufsicht(strecke, 384.0F));
    add(pause(0.6));
    add(stand("Warnung"));
    add(klick(tr(Str::WegKampfSetzen), "###wegpunkte"));
    add(pause(0.2));
    add(klickAn("Kampfpunkt ohne Netz", bei(128.0F, 64.0F)));
    add(pause(0.4));
    add(pruefe("KP Warnung: ein Kampfpunkt auf der Karte ohne Wegnetz - \"has no waypoint\" (das Netz hat keine Kante), im Fenster rot", [=] {
        const std::vector<int> k = kp::kampfpunkte();
        const NavKampfPunkt* e = k.size() == 1U ? kp::eintrag(k.front()) : nullptr;
        char z[200] = "";
        if (e != nullptr) {
            std::snprintf(z, sizeof(z), tr(Str::WegKampfOhneWegpunkt), static_cast<double>(e->ort[0]), static_cast<double>(e->ort[1]),
                          static_cast<double>(e->ort[2]));
        }
        const bool zeile = e != nullptr && finde(z, "###wegpunkte", 0, false) != nullptr;
        diag::detail("Knopftest: KP - Warnung: " + std::to_string(k.size()) + " Kampfpunkte, ohne Wegpunkt " +
                     (e != nullptr && e->ohneWegpunkt ? "ja" : "nein") + ", Zeile im Fenster " + (zeile ? "ja" : "nein") + ", Schritte " +
                     std::to_string(g_app->entUndo.size() - *tiefe));
        return e != nullptr && e->ohneWegpunkt && zeile && g_app->entUndo.size() == *tiefe + 1;
    }));
    add(foto("kp_warnung"));
    add(tu("KP Warnung: Maus in die Ansicht", [] { setzeMaus(w2::ansichtMitte()); }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(pruefe("KP Rueckgaengig: der Kampfpunkt ist weg", [=] { return kp::kampfpunkte().empty() && g_app->entUndo.size() == *tiefe; }));

    // --- 2. Netz erzeugen, drei Kampfpunkte per Klick -----------------------------------------------
    add(klick(tr(Str::WegErzeugen), "###wegpunkte"));
    add(warteBis("KP: Erzeugung fertig", [] { return !wegAuftragLaeuft(); }, 20000));
    add(pause(0.3));
    add(stand("Setzen"));
    add(klick(tr(Str::WegKampfSetzen), "###wegpunkte"));
    add(pause(0.2));
    add(klickAn("Kampfpunkt 1", bei(128.0F, 64.0F)));
    add(pause(0.2));
    add(klickAn("Kampfpunkt 2", bei(384.0F, -64.0F)));
    add(pause(0.2));
    add(klickAn("Kampfpunkt 3", bei(640.0F, 64.0F)));
    add(pause(0.5));
    auto drei = std::make_shared<std::vector<int>>();
    add(pruefe("KP Setzen: drei Klicks - drei point_combat am Boden + 25, ohne target, je ein Schritt", [=] {
        *drei = kp::kampfpunkte();
        int falsch = 0;
        for (const int e : *drei) {
            const auto o = kp::ortVon(e);
            const MapEntity& me = g_app->map.entities[static_cast<std::size_t>(e)];
            falsch += (std::fabs(o[2] - 25.0F - strecke->a[2]) > we::kStufe || me.find("target") != nullptr) ? 1 : 0;
        }
        diag::detail("Knopftest: KP - Setzen: " + std::to_string(drei->size()) + " Kampfpunkte, falsch " + std::to_string(falsch) + ", Schritte " +
                     std::to_string(g_app->entUndo.size() - *tiefe));
        return drei->size() == 3U && falsch == 0 && g_app->entUndo.size() == *tiefe + 3;
    }));

    // --- 3. Zuordnung wie im Spiel ---------------------------------------------------------------------
    add(pruefe("KP Zuordnung: je Punkt der Knoten aus GetNearestNode, frei = der eigene Knoten mit Auto-Kanten zu Wegpunkten", [=] {
        wegKampfAktuell();
        const std::shared_ptr<const NavWelt> w = navWeltSpawn();
        if (!w || !w->netz) { return false; }
        const NavNetz& n = *w->netz;
        int gut = 0;
        for (const int e : *drei) {
            const NavKampfPunkt* k = kp::eintrag(e);
            if (k == nullptr) { continue; }
            const int nn = navNaechsterKnoten(*w, k->ort, 0, 0);
            int wegKanten = 0;
            if (k->knoten > 0) {
                for (const NavNetz::Nachbar& nb : n.links[static_cast<std::size_t>(k->knoten)]) {
                    wegKanten += n.knoten[static_cast<std::size_t>(nb.knoten)].typ == NavKnoten::Wegpunkt ? 1 : 0;
                }
            }
            const bool ok = !k->ohneWegpunkt && k->zuordnung == nn && k->knoten > 0 && k->zuordnung == k->knoten && wegKanten >= 1;
            gut += ok ? 1 : 0;
            diag::detail("Knopftest: KP - Zuordnung Entity " + std::to_string(e) + " bei " + we::ortText(k->ort) + ": Knoten " + std::to_string(k->knoten) +
                         ", GetNearestNode " + std::to_string(nn) + ", Editor " + std::to_string(k->zuordnung) + ", Kanten zu Wegpunkten " +
                         std::to_string(wegKanten));
        }
        return gut == 3;
    }));
    add(foto("kp_zuordnung"));
    auto wand = std::make_shared<std::array<float, 3>>();
    auto wandDa = std::make_shared<bool>(false);
    add(tu("KP Wand: eine Stelle suchen, an der der Kasten von SpawnedPoint steckt, der Punkt selbst aber frei ist", [=] {
        const NavBauHilfe h = navBauHilfeKarte();
        const std::vector<std::array<float, 3>> kandidaten = kp::wandStellen(kp::randStellen(*m, strecke->a));
        std::vector<MapEntity> kopie = g_app->map.entities;
        kopie.emplace_back();
        int geprueft = 0;
        for (const auto& b : kandidaten) {
            if (geprueft++ > 300) { break; }
            const NavVec o{std::round(b[0]), std::round(b[1]), std::round(b[2] + 25.0F)};
            const NavSpur frei = h.spur(o, {0, 0, 0}, {0, 0, 0}, o, kNavKeineEntity, kNavSolid | kNavTerrain | kNavMonsterClip | kNavBotClip);
            if (frei.startsolid) { continue; }
            MapEntity& c = kopie.back();
            c = MapEntity{};
            c.classname = "point_combat";
            c.origin = std::to_string(static_cast<int>(o[0])) + " " + std::to_string(static_cast<int>(o[1])) + " " + std::to_string(static_cast<int>(o[2]));
            c.keys = {{"classname", c.classname}, {"origin", c.origin}};
            NavKantenPruefer p(kopie, h);
            if (!p.hatKnoten(static_cast<int>(kopie.size()) - 1)) {
                *wand = b;
                *wandDa = true;
                break;
            }
        }
        diag::detail("Knopftest: KP - Wand: " + std::to_string(kandidaten.size()) + " Kandidaten, " + std::to_string(geprueft) + " geprueft, " +
                     (*wandDa ? "Stelle " + we::ortText(*wand) : std::string("keine")));
        if (*wandDa) {
            const float b[3] = {(*wand)[0], (*wand)[1], (*wand)[2]};
            (void)wegKampfSetzen(b);
            kp::ueber(*wand);
        }
    }));
    add(pause(0.6));
    add(pruefe("KP Wand: kein eigener Knoten (in solid), zugeordnet ein Wegpunkt mit freier Sicht (GetNearestNode), Linie dorthin", [=] {
        if (!*wandDa) { return false; }
        wegKampfAktuell();
        const NavKampfPunkt* k = kp::eintrag(static_cast<int>(g_app->map.entities.size()) - 1);
        const std::shared_ptr<const NavWelt> w = navWeltSpawn();
        if (k == nullptr || !w) { return false; }
        const int nn = navNaechsterKnoten(*w, k->ort, 0, 0);
        const bool wp = k->zuEntity >= 0 && kp::istWegpunkt(g_app->map.entities[static_cast<std::size_t>(k->zuEntity)]);
        diag::detail("Knopftest: KP - Wand: Knoten " + std::to_string(k->knoten) + ", GetNearestNode " + std::to_string(nn) + ", zugeordnet Entity " +
                     std::to_string(k->zuEntity) + (wp ? " (Wegpunkt)" : "") + " bei " + we::ortText(k->zuOrt));
        return k->knoten == 0 && !k->ohneWegpunkt && k->zuordnung == nn && wp;
    }));
    add(foto("kp_wand"));
    // Fuer den Spielvergleich (BHED_KP_ENT=<Ordner>): dazu zwei Kampfpunkte auf dem Dach (noch ohne Wegpunkte dort) - einer
    // frei, einer an einer Wand. Die Entities als .ent und je Kampfpunkt, ob das Spiel "has no waypoint" melden muss.
    if (const char* ordner = std::getenv("BHED_KP_ENT"); ordner != nullptr && *ordner != 0) {
        const std::string o = ordner;
        add(tu("KP Spielvergleich: zwei Kampfpunkte auf dem Dach, Entities und Erwartung schreiben", [=] {
            std::size_t dazu = 0;
            if (!dach->empty()) {
                const std::array<float, 3> mitte = we::dachMitte(*m, *dach);
                const float b[3] = {mitte[0], mitte[1], mitte[2]};
                (void)wegKampfSetzen(b);
                ++dazu;
                std::vector<std::array<float, 3>> rand;
                for (const int i : *dach) { rand.push_back(m->proben[static_cast<std::size_t>(i)].boden); }
                const NavBauHilfe h = navBauHilfeKarte();
                for (const auto& q : kp::wandStellen(rand)) {
                    std::vector<MapEntity> kopie = g_app->map.entities;
                    MapEntity c;
                    c.classname = "point_combat";
                    c.origin = std::to_string(static_cast<int>(std::round(q[0]))) + " " + std::to_string(static_cast<int>(std::round(q[1]))) + " " +
                               std::to_string(static_cast<int>(std::round(q[2] + 25.0F)));
                    c.keys = {{"classname", c.classname}, {"origin", c.origin}};
                    kopie.push_back(c);
                    NavKantenPruefer p(kopie, h);
                    if (!p.hatKnoten(static_cast<int>(kopie.size()) - 1)) {
                        const float bw[3] = {q[0], q[1], q[2]};
                        (void)wegKampfSetzen(bw);
                        ++dazu;
                        break;
                    }
                }
            }
            g_app->wegKampfWelt.reset();
            wegKampfAktuell();
            std::string erwartet;
            for (const NavKampfPunkt& k : g_app->wegKampf) {
                char z[160];
                std::snprintf(z, sizeof(z), "%d %d %d %s\n", static_cast<int>(k.ort[0]), static_cast<int>(k.ort[1]), static_cast<int>(k.ort[2]),
                              k.ohneWegpunkt ? "ohne" : "mit");
                erwartet += z;
            }
            const auto schreib = [](const std::string& pfad, const std::string& text) {
                std::ofstream f(::bhed::utf8Pfad(pfad), std::ios::binary | std::ios::trunc);
                f << text;
                return static_cast<bool>(f);
            };
            const bool ok = schreib(o + "\\kp.ent", writeEntities(g_app->map.entities)) && schreib(o + "\\kp_erwartet.txt", erwartet);
            diag::detail("Knopftest: KP - Spielvergleich: " + std::to_string(dazu) + " auf dem Dach, " + std::to_string(g_app->wegKampf.size()) +
                         " Kampfpunkte geschrieben (" + (ok ? "ok" : "FEHLER") + "): " + erwartet);
            for (std::size_t i = 0; i < dazu; ++i) { (void)entityRueckgaengig(); }
        }));
    }

    // --- 4. Duplizieren, Entf, Rueckgaengig ---------------------------------------------------------------
    add(tu("KP Duplizieren: einen Kampfpunkt waehlen, Maus in die Ansicht", [=] {
        kp::ueber({strecke->a[0] + 384.0F, strecke->a[1], strecke->a[2]});
        entityAuswahlSetzen(drei->empty() ? -1 : drei->front());
        *tiefe = g_app->entUndo.size();
        *zahl0 = kp::kampfpunkte().size();
        setzeMaus(w2::ansichtMitte());
    }));
    add(pause(0.3));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_D, "Strg+D"));
    add(pause(0.3));
    add(pruefe("KP Duplizieren (Strg+D): ein Kampfpunkt mehr, ein Schritt", [=] {
        return !drei->empty() && kp::kampfpunkte().size() == *zahl0 + 1 && g_app->entUndo.size() == *tiefe + 1;
    }));
    add(taste(ImGuiKey_Delete, "Entf"));
    add(pause(0.3));
    add(pruefe("KP Entf: die Kopie ist weg, ein Schritt", [=] { return kp::kampfpunkte().size() == *zahl0 && g_app->entUndo.size() == *tiefe + 2; }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(pruefe("KP Strg+Z zweimal: wie vorher", [=] { return kp::kampfpunkte().size() == *zahl0 && g_app->entUndo.size() == *tiefe; }));

    // --- 5. Testroute aufs Dach ------------------------------------------------------------------------
    auto dachPunkt = std::make_shared<std::array<float, 3>>();
    auto sprungVor = std::make_shared<int>(0);
    const auto sprungZahl = [] {
        int n = 0;
        for (const MapEntity& e : g_app->map.entities) {
            const std::string* j = e.find("targetJump");
            n += (j != nullptr && !j->empty()) ? 1 : 0;
        }
        return n;
    };
    add(tu("KP Route: Startpunkt auf dem Dach (Sprungkanten)", [=] {
        g_app->wegWerkzeug = 0;
        *sprungVor = sprungZahl();
        if (dach->empty()) { return; }
        *dachPunkt = we::dachMitte(*m, *dach);
        diag::detail("Knopftest: KP - Dach " + we::ortText(*dachPunkt));
        (void)wegAuftragStarten(WegAuftragArt::Start, {*dachPunkt});
    }));
    add(warteBis("KP: Startpunkt fertig", [] { return !wegAuftragLaeuft(); }, 20000));
    add(pause(0.3));
    add(tu("KP Route: Ergebnis des Startpunkts", [=] {
        diag::detail("Knopftest: KP - Startpunkt: " + g_app->wegMeldung + "; Sprungkanten " + std::to_string(*sprungVor) + " -> " + std::to_string(sprungZahl()));
    }));
    add(tu("KP Route: Draufsicht ueber der Strecke", [=] { kp::ueber(strecke->a); }));
    add(pause(0.5));
    add(klick(tr(Str::WegRoute), "###wegpunkte"));
    add(pause(0.2));
    add(klickAn("Route Anfang", bei(0.0F, 0.0F)));
    add(pause(0.2));
    add(tu("KP Route: Draufsicht ueber dem Dach", [=] { kp::ueber(*dachPunkt); }));
    add(pause(0.5));
    add(klickAn("Route Ende (Dach)", [=] { return wm::schirm(*dachPunkt); }));
    add(pause(0.4));
    auto zuFuss = std::make_shared<bool>(false);
    add(pruefe("KP Route: der Weg des Spiels vom Boden aufs Dach (ist das Dach nicht zu Fuss erreichbar: ueber eine Sprungkante), jedes Stueck eine Kante des Netzes", [=] {
        const std::shared_ptr<const NavWelt> w = navWeltSpawn();
        if (dach->empty() || !w || !w->netz) { return false; }
        const NavNetz& n = *w->netz;
        int spruenge = 0;
        int ohneKante = 0;
        for (std::size_t i = 1; i < g_app->wegRouteKnoten.size(); ++i) {
            const int k = n.kanteZwischen(g_app->wegRouteKnoten[i - 1], g_app->wegRouteKnoten[i]);
            ohneKante += k > 0 ? 0 : 1;
            spruenge += k > 0 && (n.kanten[static_cast<std::size_t>(k)].flags & NavKante::Springen) != 0U ? 1 : 0;
        }
        const bool amDach = !g_app->wegRoute.empty() && std::fabs(g_app->wegRoute.back()[2] - (*dachPunkt)[2]) < 2.0F * we::kStufe;
        // Manche Karte hat ein "Dach", das man zu Fuss erreicht (mp/ffa5, W3b) - dann braucht der Weg keinen Sprung.
        *zuFuss = g_app->wegRouteKnoten.size() >= 2U &&
                  navAStar(n, g_app->wegRouteKnoten.front(), g_app->wegRouteKnoten.back(), nullptr, false);
        std::string text;
        for (const auto& q : g_app->wegRoute) { text += " " + we::ortText(q); }
        diag::detail("Knopftest: KP - Route: " + g_app->wegMeldung + ";" + text + "; Spruenge " + std::to_string(spruenge) + ", ohne Kante " +
                     std::to_string(ohneKante) + ", zu Fuss erreichbar " + (*zuFuss ? "ja" : "nein"));
        // Verbinden die Felder (target.., targetJump) den Boden nicht mit dem Dach (der Startpunkt band das Dach an einen
        // anderen, abgetrennten Teil an), zeigt die Testroute keinen Weg und sagt es.
        const bool felder = kp::feldVerbunden(strecke->a, *dachPunkt);
        diag::detail(std::string("Knopftest: KP - Route: Boden und Dach ueber die Felder verbunden ") + (felder ? "ja" : "nein"));
        if (!felder) { return g_app->wegRoute.empty() && g_app->wegMeldung == tr(Str::WegRouteKeinWeg); }
        return g_app->wegRouteKnoten.size() >= 2U && (*zuFuss || spruenge >= 1) && ohneKante == 0 && amDach;
    }));
    add(foto("kp_route"));
    add(klick(tr(Str::WegRouteSpringen), "###wegpunkte"));
    add(pause(0.3));
    add(pruefe("KP Route: eine Figur ohne Sprung kommt nicht hin - kein Weg, Meldung (ist das Dach zu Fuss erreichbar: Weg ohne Sprung)", [=] {
        diag::detail("Knopftest: KP - Route ohne Sprung: " + g_app->wegMeldung);
        if (*zuFuss) { return !g_app->wegRouteSpringen && !g_app->wegRoute.empty(); }
        return !g_app->wegRouteSpringen && g_app->wegRoute.empty() && g_app->wegMeldung == tr(Str::WegRouteKeinWegLaufen);
    }));
    add(klick(tr(Str::WegRouteSpringen), "###wegpunkte"));
    add(tu("KP Route: Werkzeug aus", [] { g_app->wegWerkzeug = 0; }));

    // --- 6. Abdeckung ------------------------------------------------------------------------------------
    auto loch = std::make_shared<std::array<float, 3>>();
    add(tu("KP Abdeckung: die Wegpunkte bis 450 um die Mitte der Strecke weg (ein Schritt)", [=] {
        kp::ueber({strecke->a[0] + 384.0F, strecke->a[1], strecke->a[2]});
        *loch = {strecke->a[0] + 384.0F, strecke->a[1], strecke->a[2]};
        std::vector<MapEntity> vorher = g_app->map.entities;
        auto& v = g_app->map.entities;
        v.erase(std::remove_if(v.begin(), v.end(),
                               [&](const MapEntity& e) {
                                   float o[3];
                                   return kp::istWegpunkt(e) && entityOrt(e, o) && std::hypot(o[0] - (*loch)[0], o[1] - (*loch)[1]) < 450.0F &&
                                          std::fabs(o[2] - 25.0F - (*loch)[2]) < 64.0F;
                               }),
                v.end());
        entitiesGeaendert(std::move(vorher));
    }));
    add(pause(0.3));
    add(klick(tr(Str::WegAbdeckung), "###wegpunkte"));
    add(warteBis("KP: Abdeckung gerechnet", [] { return wegAbdeckungAktuell(); }, 30000));
    add(pause(0.4));
    const auto offenBei = [](const std::array<float, 3>& b, float weit) {
        int n = 0;
        for (const auto& q : g_app->wegAbdeckung.offen) {
            n += (std::hypot(q[0] - b[0], q[1] - b[1]) <= weit && std::fabs(q[2] - b[2]) <= we::kStufe) ? 1 : 0;
        }
        return n;
    };
    add(pruefe("KP Abdeckung: die Flaeche ohne Wegpunkt ist rot, an keinem Wegpunkt eine rote Zelle", [=] {
        int amWegpunkt = 0;
        for (const we::Punkt& w : we::punkte()) { amWegpunkt += offenBei({w.ort[0], w.ort[1], w.ort[2] - 25.0F}, 16.0F); }
        const int imLoch = offenBei(*loch, 64.0F);
        diag::detail("Knopftest: KP - Abdeckung: " + std::to_string(g_app->wegAbdeckung.offen.size()) + " von " + std::to_string(g_app->wegAbdeckung.zellen) +
                     " Zellen offen, im Loch " + std::to_string(imLoch) + ", an Wegpunkten " + std::to_string(amWegpunkt) +
                     ", Rasterweite " + std::to_string(static_cast<int>(g_app->wegAbstand)) + ", volle Erzeugung " +
                     std::to_string(static_cast<int>(g_app->wegAbstandVoll)));
        return g_app->draufsicht && imLoch > 0 && amWegpunkt == 0;
    }));
    add(foto("kp_abdeckung"));
    add(tu("KP Abdeckung: Rueckgaengig (die Wegpunkte wieder da)", [] { (void)entityRueckgaengig(); }));
    add(warteBis("KP: Abdeckung neu gerechnet", [] { return wegAbdeckungAktuell(); }, 30000));
    add(pruefe("KP Abdeckung: mit den Wegpunkten ist die Stelle nicht mehr rot", [=] {
        diag::detail("Knopftest: KP - Abdeckung danach: im Loch " + std::to_string(offenBei(*loch, 64.0F)));
        return offenBei(*loch, 64.0F) == 0;
    }));
    add(klick(tr(Str::WegAbdeckung), "###wegpunkte"));

    // --- 7. Versetzen ------------------------------------------------------------------------------------
    auto stelle = std::make_shared<std::array<float, 3>>();
    auto stelleDa = std::make_shared<bool>(false);
    auto gesetzt = std::make_shared<int>(-1);
    add(tu("KP Versetzen: eine Stelle am Rand, an der das Spiel einen Wegpunkt (Boden + 25) verwirft, Versetzen aus", [=] {
        const std::unique_ptr<WegWeltKarte> k = wegWeltKarte();
        int geprueft = 0;
        for (const auto& b : kp::wandStellen(kp::randStellen(*m, strecke->a))) {
            if (geprueft++ > 300) { break; }
            const std::array<float, 3> o{std::round(b[0]), std::round(b[1]), std::round(b[2] + 25.0F)};
            std::array<float, 3> neu{};
            if (k->welt.knoten(o) || !wegStelleSuchen(b, k->welt, 128.0F, neu)) { continue; }
            *stelle = b;
            *stelleDa = true;
            break;
        }
        diag::detail("Knopftest: KP - Versetzen: " + (*stelleDa ? "Stelle " + we::ortText(*stelle) : std::string("keine Stelle")));
        if (*stelleDa) { kp::ueber(*stelle); }
        g_app->wegVersetzen = false;
        *tiefe = g_app->entUndo.size();
    }));
    add(pause(0.5));
    add(klick(tr(Str::WegSetzen), "###wegpunkte"));
    add(pause(0.2));
    add(klickAn("Wegpunkt an der Wand (ohne Versetzen)", [=] { return wm::schirm(*stelle); }));
    add(pause(0.3));
    const auto spielBehaelt = [](int e) {
        const std::unique_ptr<WegWeltKarte> k = wegWeltKarte();
        const auto o = kp::ortVon(e);
        return k->welt.knoten(o);
    };
    add(pruefe("KP Versetzen: nachgestellt - ohne Versetzen steht der Wegpunkt dort, wo das Spiel ihn verwirft", [=] {
        *gesetzt = static_cast<int>(g_app->map.entities.size()) - 1;
        const bool neu = *stelleDa && kp::istWegpunkt(g_app->map.entities[static_cast<std::size_t>(*gesetzt)]) && g_app->entUndo.size() == *tiefe + 1;
        const bool behaelt = neu && spielBehaelt(*gesetzt);
        diag::detail("Knopftest: KP - ohne Versetzen bei " + (neu ? we::ortText(kp::ortVon(*gesetzt)) : std::string("-")) + ", das Spiel behaelt ihn " +
                     (behaelt ? "ja" : "nein"));
        return neu && !behaelt;
    }));
    add(tu("KP Versetzen: Rueckgaengig, Versetzen an", [=] { (void)entityRueckgaengig(); }));
    add(klick(tr(Str::WegVersetzen), "###wegpunkte"));
    add(pause(0.2));
    add(klickAn("Wegpunkt an der Wand (mit Versetzen)", [=] { return wm::schirm(*stelle); }));
    add(pause(0.3));
    add(pruefe("KP Versetzen: mit Versetzen steht er an einer Stelle, die das Spiel behaelt (bis 128 weit), mit Meldung, wohin", [=] {
        *gesetzt = static_cast<int>(g_app->map.entities.size()) - 1;
        if (!*stelleDa || !kp::istWegpunkt(g_app->map.entities[static_cast<std::size_t>(*gesetzt)])) { return false; }
        const auto o = kp::ortVon(*gesetzt);
        const float d = std::hypot(o[0] - (*stelle)[0], o[1] - (*stelle)[1]);
        const std::string vorne = std::string(tr(Str::WegVersetzt)).substr(0, std::string(tr(Str::WegVersetzt)).find('%'));
        const bool meldung = g_app->wegMeldung.rfind(vorne, 0) == 0;
        diag::detail("Knopftest: KP - mit Versetzen bei " + we::ortText(o) + ", " + std::to_string(static_cast<int>(d)) + " weit; " + g_app->wegMeldung);
        return g_app->wegVersetzen && spielBehaelt(*gesetzt) && d <= 128.0F + 1.0F && meldung && g_app->entUndo.size() == *tiefe + 1;
    }));
    add(foto("kp_versetzt"));
    add(tu("KP: Werkzeug aus, Draufsicht aus", [] {
        g_app->wegWerkzeug = 0;
        draufsichtSchalten(false);
    }));
    return s;
}

// rc609 W4: die Bilder des Abschnitts "Kampfpunkte, Testroute, Abdeckung" (Seite NPC-Wegpunkte), BHED_EDITORTEST=handbuch
// mit BHED_HB_NUR=kampfpunkte; Karte aus BHED_WE_PK3/BHED_WE_KARTE wie kampfpunkte (Jedi Temple Garden).
//   h_weg_kampf      Draufsicht: Kampfpunkte mit ihren Kanten, einer an der Wand mit der Linie zum Wegpunkt; das Fenster
//                    mit einer Warnung (ein Kampfpunkt auf dem Dach ohne Wegpunkt auf demselben Boden)
//   h_weg_route      Draufsicht: die Testroute vom Boden aufs Dach ueber eine Sprungkante
//   h_weg_abdeckung  Draufsicht: rote Zellen, wo die Wegpunkte fehlen
std::vector<Schritt> kampfpunkteHandbuchSchritte() {
    std::vector<Schritt> t;
    const char* const kk = std::getenv("BHED_WE_KARTE");
    const std::string karte = (kk != nullptr && *kk != 0) ? kk : "jeditemple_garden";
    const char* const pk = std::getenv("BHED_WE_PK3");
    const std::string pk3 = pk != nullptr ? pk : "";
    t.push_back(tu("Handbuch Kampfpunkte: Karte " + karte, [=] {
        g_app->leftMode = 1;
        g_app->missionPickOpen = false;
        if (!pk3.empty()) {
            missionenAusArchiv(pk3, karte);
        } else if (!loadMapFromArchive("maps/" + karte + ".bsp")) {
            meldeFehler("Handbuch Kampfpunkte: maps/" + karte + ".bsp nicht geladen");
        }
    }));
    t.push_back(warteBis("Handbuch Kampfpunkte: geladen", [=] { return g_app->map.path == "maps/" + karte + ".bsp" && !g_app->geo.empty(); }, 6000));
    auto m = std::make_shared<we::Messung>();
    auto strecke = std::make_shared<wm::Strecke>();
    auto dachPunkt = std::make_shared<std::array<float, 3>>();
    t.push_back(tu("Handbuch Kampfpunkte: messen, Netz erzeugen, Kampfpunkte setzen", [=] {
        g_app->playing = false;
        g_app->throughCamera = false;
        g_app->cameraSelected = false;
        g_app->selectedActor = -1;
        entityAuswahlSetzen(-1);
        g_app->wegFensterOffen = true;
        g_app->zeigeWegnetz = true;
        g_app->wegAbdeckungAn = false;
        g_app->wegBereiche.clear();
        wegBereicheSpeichern();
        *m = we::messen();
        *strecke = wm::streckeSuchen(*m);
        const std::vector<int> dach = we::groesstesDach(*m);
        if (!strecke->da || dach.empty()) {
            meldeFehler("Handbuch Kampfpunkte: keine Strecke oder kein Dach");
            return;
        }
        *dachPunkt = we::dachMitte(*m, dach);
        g_app->wegAbstand = 192.0F;
        (void)wegAuftragStarten(WegAuftragArt::Voll, {}, -1, false);
        for (const auto& d : std::vector<std::array<float, 2>>{{128.0F, 64.0F}, {384.0F, -64.0F}, {640.0F, 64.0F}}) {
            const float b[3] = {strecke->a[0] + d[0], strecke->a[1] + d[1], strecke->a[2]};
            (void)wegKampfSetzen(b);
        }
        // einer an der Wand (kein eigener Knoten), der naechste an der Strecke
        const NavBauHilfe h = navBauHilfeKarte();
        for (const auto& q : kp::wandStellen(kp::randStellen(*m, strecke->a))) {
            std::vector<MapEntity> kopie = g_app->map.entities;
            MapEntity c;
            c.classname = "point_combat";
            c.origin = std::to_string(static_cast<int>(std::round(q[0]))) + " " + std::to_string(static_cast<int>(std::round(q[1]))) + " " +
                       std::to_string(static_cast<int>(std::round(q[2] + 25.0F)));
            c.keys = {{"classname", c.classname}, {"origin", c.origin}};
            kopie.push_back(c);
            NavKantenPruefer p(kopie, h);
            if (!p.hatKnoten(static_cast<int>(kopie.size()) - 1)) {
                const float b[3] = {q[0], q[1], q[2]};
                (void)wegKampfSetzen(b);
                break;
            }
        }
        // einer auf dem Dach (dort noch kein Wegpunkt): die Warnung im Fenster
        const float b[3] = {(*dachPunkt)[0], (*dachPunkt)[1], (*dachPunkt)[2]};
        (void)wegKampfSetzen(b);
        entityAuswahlSetzen(-1);
    }));
    const auto fensterLinks = [] {
        return tu("Handbuch Kampfpunkte: Fenster links neben die Ansicht", [] {
            if (ImGuiWindow* f = ImGui::FindWindowByName("###wegpunkte")) {
                ImGui::SetWindowPos(f, ImVec2{std::max(4.0F, g_app->kartenAnsicht[0] - f->Size.x - 4.0F), g_app->kartenAnsicht[1]});
            }
        });
    };
    const auto bild = [] { return hb::vereint(hb::karte(), hb::fensterRect("###wegpunkte")); };
    const auto mausWeg = [] {
        return tu("Handbuch Kampfpunkte: Maus aus dem Bild", [] { setzeMaus(ImVec2{4.0F, ImGui::GetIO().DisplaySize.y - 4.0F}); });
    };
    // Das Fenster ist bei 200 % hoeher als die Ansicht und rollt; Elemente ausserhalb zeichnet ImGui nicht. Die Abschnitte
    // Kampfpunkte und Pruefen stehen unten: ganz nach unten rollen.
    const auto rolle = [] {
        return tu("Handbuch Kampfpunkte: Fenster nach unten rollen", [] {
            if (ImGuiWindow* f = ImGui::FindWindowByName("###wegpunkte")) { ImGui::SetScrollY(f, f->ScrollMax.y); }
        });
    };
    // Die Wand-Stelle liegt am Rand nahe der Strecke: die Draufsicht ueber der Mitte der Kampfpunkte am Boden.
    t.push_back(tu("Handbuch Kampfpunkte: Draufsicht ueber den Kampfpunkten", [=] {
        std::array<float, 3> mitte{0, 0, 0};
        const std::vector<int> k = kp::kampfpunkte();
        int n = 0;
        for (std::size_t i = 0; i + 1 < k.size(); ++i) {   // ohne den auf dem Dach
            const auto o = kp::ortVon(k[i]);
            mitte = {mitte[0] + o[0], mitte[1] + o[1], mitte[2] + o[2]};
            ++n;
        }
        if (n > 0) { mitte = {mitte[0] / static_cast<float>(n), mitte[1] / static_cast<float>(n), mitte[2] / static_cast<float>(n) - 25.0F}; }
        kp::ueber(mitte);
        g_app->cam.pos[2] = g_app->draufSchnitt + 700.0F;
    }));
    t.push_back(fensterLinks());
    t.push_back(mausWeg());
    t.push_back(pause(0.5));
    t.push_back(rolle());
    t.push_back(pause(0.5));
    hb::markiere("h_weg_kampf", [] {
        std::string warnung;
        const NavNetz* kn = g_app->wegKampfWelt && g_app->wegKampfWelt->netz ? g_app->wegKampfWelt->netz.get() : nullptr;
        for (const NavKampfPunkt& k : g_app->wegKampf) {
            const bool insel = kn != nullptr && k.knoten > 0 && kn->knoten[static_cast<std::size_t>(k.knoten)].insel;
            if (!k.ohneWegpunkt && !insel) { continue; }
            char z[200];
            std::snprintf(z, sizeof(z), tr(k.ohneWegpunkt ? Str::WegKampfOhneWegpunkt : Str::WegKampfAllein), static_cast<double>(k.ort[0]),
                          static_cast<double>(k.ort[1]), static_cast<double>(k.ort[2]));
            warnung = z;
            break;
        }
        return std::vector<hb::Marke>{hb::Marke(hb::gruppe({tr(Str::WegKampfSetzen)}, "###wegpunkte"), 1),
                                      hb::Marke(hb::gruppe({warnung}, "###wegpunkte"), 2)};
    });
    t.push_back(hb::fotoTeil("h_weg_kampf", bild));
    t.push_back(tu("Handbuch Kampfpunkte: Startpunkt auf dem Dach, Testroute vom Boden aufs Dach", [=] {
        (void)wegAuftragStarten(WegAuftragArt::Start, {*dachPunkt}, -1, false);
        g_app->wegRouteSpringen = true;
        g_app->wegWerkzeug = 8;
        // Kurz genug fuer ein Bild: von drei Kanten vor dem Fuss der hoechsten Sprungkante bis drei Kanten hinter ihrem Kopf.
        std::map<std::string, int> name;
        const auto& v = g_app->map.entities;
        for (std::size_t i = 0; i < v.size(); ++i) {
            if (!v[i].targetname.empty()) { name[v[i].targetname] = static_cast<int>(i); }
        }
        std::map<int, std::vector<int>> nb;
        int fuss = -1;
        int kopf = -1;
        float best = 0.0F;
        for (std::size_t i = 0; i < v.size(); ++i) {
            if (!kp::istWegpunkt(v[i])) { continue; }
            for (const char* f : {"target", "target2", "target3", "target4"}) {
                const std::string* z = v[i].find(f);
                const auto it = z != nullptr ? name.find(*z) : name.end();
                if (it == name.end()) { continue; }
                nb[static_cast<int>(i)].push_back(it->second);
                nb[it->second].push_back(static_cast<int>(i));
            }
            const std::string* j = v[i].find("targetJump");
            const auto it = j != nullptr ? name.find(*j) : name.end();
            if (it == name.end()) { continue; }
            const float dz = kp::ortVon(it->second)[2] - kp::ortVon(static_cast<int>(i))[2];
            if (std::fabs(dz) > best) {
                best = std::fabs(dz);
                fuss = dz > 0.0F ? static_cast<int>(i) : it->second;
                kopf = dz > 0.0F ? it->second : static_cast<int>(i);
            }
        }
        const auto weiter = [&](int von) {
            int letzt = von;
            std::set<int> gesehen{von};
            std::vector<int> rand{von};
            for (int tiefe = 0; tiefe < 3 && !rand.empty(); ++tiefe) {
                std::vector<int> neu;
                for (const int i : rand) {
                    for (const int j : nb[i]) {
                        if (gesehen.insert(j).second && std::fabs(kp::ortVon(j)[2] - kp::ortVon(von)[2]) < 60.0F) { neu.push_back(j); }
                    }
                }
                if (!neu.empty()) { letzt = neu.front(); }
                rand = neu;
            }
            const auto o = kp::ortVon(letzt);
            return std::array<float, 3>{o[0], o[1], o[2] - 25.0F};
        };
        const std::array<float, 3> a = fuss >= 0 ? weiter(fuss) : strecke->a;
        const std::array<float, 3> b = kopf >= 0 ? weiter(kopf) : *dachPunkt;
        g_app->wegRouteKlick = {a, b};
        (void)wegRouteRechnen(a, b);
        // Draufsicht ueber der Mitte des Wegs, der Schnitt ueber dem Dach, so hoch, dass der ganze Weg ins Bild passt
        float x0 = 1.0e9F;
        float y0 = 1.0e9F;
        float x1 = -1.0e9F;
        float y1 = -1.0e9F;
        for (const auto& q : g_app->wegRoute) {
            x0 = std::min(x0, q[0]);
            y0 = std::min(y0, q[1]);
            x1 = std::max(x1, q[0]);
            y1 = std::max(y1, q[1]);
        }
        kp::ueber({(x0 + x1) * 0.5F, (y0 + y1) * 0.5F, (*dachPunkt)[2]});
        g_app->draufSchnitt = (*dachPunkt)[2] + 160.0F;
        g_app->cam.pos[2] = g_app->draufSchnitt + std::max(700.0F, 1.3F * std::max(x1 - x0, y1 - y0));
        g_app->mapDirty = true;
    }));
    t.push_back(fensterLinks());
    t.push_back(mausWeg());
    t.push_back(pause(0.5));
    t.push_back(rolle());
    t.push_back(pause(0.5));
    hb::markiere("h_weg_route", [] {
        return std::vector<hb::Marke>{hb::Marke(hb::gruppe({tr(Str::WegRoute), tr(Str::WegRouteSpringen)}, "###wegpunkte"), 1)};
    });
    t.push_back(hb::fotoTeil("h_weg_route", bild));
    t.push_back(tu("Handbuch Kampfpunkte: Testroute aus, Wegpunkte bis 450 um die Mitte der Strecke weg, Abdeckung an", [=] {
        g_app->wegWerkzeug = 0;
        g_app->wegRouteKlick.clear();
        g_app->wegRoute.clear();
        g_app->wegRouteSprung.clear();
        g_app->wegRouteKnoten.clear();
        // Das Loch weit weg vom Spielerstart (sonst zerfiele das Netz am Hauptteil, und alles stuende violett da): ein gut
        // verbundener Wegpunkt (alle vier Felder) auf dem Boden der Strecke bei drei Vierteln der Entfernungen vom
        // Spielerstart; weg, was naeher liegt als 1,6 Reichweiten der Abdeckung (sonst deckten die Nachbarn das Loch).
        float st[3] = {0, 0, 0};
        for (const MapEntity& e : g_app->map.entities) {
            if (e.classname == "info_player_start" && entityOrt(e, st)) { break; }
        }
        std::vector<std::pair<float, std::array<float, 3>>> kandidaten;
        for (const MapEntity& e : g_app->map.entities) {
            float o[3];
            if (!kp::istWegpunkt(e) || e.find("target4") == nullptr || !entityOrt(e, o) || std::fabs(o[2] - 25.0F - strecke->a[2]) > 64.0F) { continue; }
            kandidaten.emplace_back(std::hypot(o[0] - st[0], o[1] - st[1]), std::array<float, 3>{o[0], o[1], o[2] - 25.0F});
        }
        std::sort(kandidaten.begin(), kandidaten.end(), [](const auto& x, const auto& y) { return x.first < y.first; });
        const std::array<float, 3> loch = kandidaten.empty() ? std::array<float, 3>{strecke->a[0] + 384.0F, strecke->a[1], strecke->a[2]}
                                                            : kandidaten[kandidaten.size() * 3 / 4].second;
        std::vector<MapEntity> vorher = g_app->map.entities;
        auto& v = g_app->map.entities;
        v.erase(std::remove_if(v.begin(), v.end(),
                               [&](const MapEntity& e) {
                                   float o[3];
                                   return kp::istWegpunkt(e) && entityOrt(e, o) &&
                                          std::hypot(o[0] - loch[0], o[1] - loch[1]) < 1.6F * std::max(g_app->wegAbstand, g_app->wegAbstandVoll) &&
                                          std::fabs(o[2] - 25.0F - loch[2]) < 64.0F;
                               }),
                v.end());
        entitiesGeaendert(std::move(vorher));
        kp::ueber(loch);
        g_app->cam.pos[2] = g_app->draufSchnitt + 900.0F;
        g_app->wegAbdeckungAn = true;
    }));
    t.push_back(warteBis("Handbuch Kampfpunkte: Abdeckung gerechnet", [] { return wegAbdeckungAktuell(); }, 30000));
    t.push_back(fensterLinks());
    t.push_back(mausWeg());
    t.push_back(pause(0.5));
    t.push_back(rolle());
    t.push_back(pause(0.5));
    hb::markiere("h_weg_abdeckung", [] { return std::vector<hb::Marke>{hb::Marke(hb::gruppe({tr(Str::WegAbdeckung)}, "###wegpunkte"), 1)}; });
    t.push_back(hb::fotoTeil("h_weg_abdeckung", bild));
    t.push_back(tu("Handbuch Kampfpunkte: aufraeumen", [] {
        g_app->wegAbdeckungAn = false;
        draufsichtSchalten(false);
    }));
    return t;
}
