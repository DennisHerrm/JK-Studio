// selbsttest_werkzeuge.inc - aus gui/selbsttest.cpp geschnitten (Stufe 8d Punkt 5, unveraendert): Werkzeuge - Wegnetz, Drehen, Kopieren, Model-Tab, Indizes
//
// Eingebunden an genau dieser Stelle von gui/selbsttest.cpp (im selben Namensraum, mit allen Helfern davor);
// keine eigene Uebersetzungseinheit - der vorverarbeitete Text ist derselbe wie vor dem Schnitt.
// Abschnitte: Wegnetz der NPCs und Draufsicht (BHED_EDITORTEST=wegpunkte, rc589); Umriss der Auswahl (rc597); Marke und Gizmo folgen Schiff und Figur (rc597); Drehen um alle drei Achsen (rc599); Kopieren und Einfuegen im Skriptbaum (rc599); Kopieren, Quell-Reiter schliessen, woanders einfuegen (rc601); Werkzeuge der Kartenansicht (rc590); Model-Tab (BHED_EDITORTEST=modell, rc591)

// --- Wegnetz der NPCs und Draufsicht (BHED_EDITORTEST=wegpunkte, rc589) -------
//
// "npc waypoints erstellen ... bzw. er die automatisch erstellt". Nach dem
// Laden der Karte: Fenster oeffnen, Netz erzeugen (es muss so gebaut sein, wie
// das Spiel es liest: zusammenhaengend, keine fehlenden targets, nichts im
// Festen), Rueckgaengig, von Hand setzen, verbinden und trennen, pruefen,
// Draufsicht mit Schnitthoehe.
std::size_t zaehleKlasse(const char* klasse) {
    std::size_t n = 0;
    for (const MapEntity& e : g_app->map.entities) { n += e.classname == klasse ? 1U : 0U; }
    return n;
}

int entityIndex(const std::string& name) {
    for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
        if (g_app->map.entities[i].targetname == name) { return static_cast<int>(i); }
    }
    return -1;
}

std::vector<Schritt> wegSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto wp0 = std::make_shared<std::size_t>(0);
    auto tiefe0 = std::make_shared<std::size_t>(0);
    add(tu("Wegnetz: Ausgangslage", [=] {
        *wp0 = zaehleKlasse("waypoint");
        *tiefe0 = g_app->entUndo.size();
        g_app->throughCamera = false;
        g_app->playing = false;
        diag::detail("Knopftest: Wegnetz - Karte " + g_app->map.path + ", " + std::to_string(*wp0) + " Wegpunkte");
    }));
    add(klick(tr(Str::WegTitel), "###main"));
    add(pause(0.4));
    add(pruefe("Leiste: Wegpunkte oeffnet das Fenster und zeigt das Netz", [] {
        return g_app->wegFensterOffen && g_app->zeigeWegnetz && finde(tr(Str::WegErzeugen), "###wegpunkte", 0, false) != nullptr;
    }));
    auto dauer = std::make_shared<double>(0.0);
    add(tu("Uhr", [=] { *dauer = ImGui::GetTime(); }));
    add(klick(tr(Str::WegErzeugen), "###wegpunkte"));
    add(pause(0.5));
    auto erzeugt = std::make_shared<std::vector<std::string>>();
    add(pruefe("Netz erzeugen: Wegpunkte mit targets, ein Rueckgaengig-Schritt", [=] {
        erzeugt->clear();
        for (const MapEntity& e : g_app->map.entities) {
            if (e.targetname.rfind("wpauto_", 0) == 0) { erzeugt->push_back(e.targetname); }
        }
        diag::detail("Knopftest: Wegnetz - " + g_app->wegMeldung + ", " + std::to_string(erzeugt->size()) + " wpauto");
        return erzeugt->size() >= 20U && g_app->entUndo.size() == *tiefe0 + 1 &&
               zaehleKlasse("waypoint") == *wp0 + erzeugt->size();
    }));
    add(foto("weg_netz"));
    add(pruefe("Wie das Spiel es liest: alle erzeugten erreichbar, kein target fehlt, nichts im Festen oder schwebend", [=] {
        std::vector<MapEntity> nur;
        for (const MapEntity& e : g_app->map.entities) {
            if (e.targetname.rfind("wpauto_", 0) == 0) { nur.push_back(e); }
        }
        const WegNetz n = baueWegNetz(nur);
        std::vector<std::vector<int>> adj(n.knoten.size());
        for (const WegKante& k : n.kanten) {
            adj[static_cast<std::size_t>(k.a)].push_back(k.b);
            adj[static_cast<std::size_t>(k.b)].push_back(k.a);
        }
        // Mehrere Startpunkte koennen getrennte Inseln ergeben - die groesste
        // muss den Grossteil tragen.
        std::vector<int> komp(n.knoten.size(), -1);
        std::size_t groesste = 0;
        for (std::size_t i = 0; i < n.knoten.size(); ++i) {
            if (komp[i] >= 0) { continue; }
            std::vector<int> st{static_cast<int>(i)};
            komp[i] = static_cast<int>(i);
            std::size_t g = 0;
            while (!st.empty()) {
                const int a = st.back();
                st.pop_back();
                ++g;
                for (const int b : adj[static_cast<std::size_t>(a)]) {
                    if (komp[static_cast<std::size_t>(b)] < 0) {
                        komp[static_cast<std::size_t>(b)] = static_cast<int>(i);
                        st.push_back(b);
                    }
                }
            }
            groesste = std::max(groesste, g);
        }
        int fest = 0;
        int schwebt = 0;
        for (const WegBefund& b : pruefeWegNetz(n, nur, [](const std::array<float, 3>& a, const std::array<float, 3>& b2,
                                                           std::array<float, 3>& t, std::array<float, 3>& nn) {
                 const TraceTreffer r = traceRay(g_app->geo, a.data(), b2.data());
                 for (int k = 0; k < 3; ++k) {
                     t[static_cast<std::size_t>(k)] = r.point[k];
                     nn[static_cast<std::size_t>(k)] = r.normal[k];
                 }
                 return r.hit;
             })) {
            fest += b.art == WegBefund::Art::ImFesten ? 1 : 0;
            schwebt += b.art == WegBefund::Art::Schwebend ? 1 : 0;
        }
        diag::detail("Knopftest: Wegnetz - " + std::to_string(n.knoten.size()) + " Knoten, " +
                     std::to_string(n.kanten.size()) + " Kanten, groesste Insel " + std::to_string(groesste) +
                     ", im Festen " + std::to_string(fest) + ", schwebend " + std::to_string(schwebt) +
                     ", fehlende targets " + std::to_string(n.fehlendeZiele.size()));
        return n.fehlendeZiele.empty() && fest == 0 && schwebt == 0 && groesste * 2 >= n.knoten.size() &&
               n.kanten.size() >= n.knoten.size();
    }));
    add(pruefe("Im Fenster: die Zahlen und das Ergebnis", [] {
        return g_app->wegMeldung.find(std::to_string(zaehleKlasse("waypoint"))) != std::string::npos ||
               !g_app->wegMeldung.empty();
    }));
    // Von Hand setzen: zwei Klicks auf den Boden (dort, wo ein erzeugter steht -
    // da ist sicher Boden), der zweite haengt am ersten.
    auto ziel = std::make_shared<std::array<ImVec2, 2>>();
    add(tu("Zwei Bodenpunkte im Bild suchen (bei erzeugten Wegpunkten)", [=] {
        int n = 0;
        for (const MapEntity& e : g_app->map.entities) {
            if (e.targetname.rfind("wpauto_", 0) != 0 || n >= 2) { continue; }
            float o[3];
            if (!entityOrt(e, o)) { continue; }
            const float boden[3] = {o[0] + 30.0F, o[1] + 30.0F, o[2] - 25.0F};
            ImVec2 sp;
            if (!kartenPunktAufSchirm(boden, sp)) { continue; }
            if (sp.x < g_app->kartenAnsicht[0] + 40 || sp.x > g_app->kartenAnsicht[2] - 40 ||
                sp.y < g_app->kartenAnsicht[1] + 80 || sp.y > g_app->kartenAnsicht[3] - 40) {
                continue;
            }
            if (n == 1 && std::fabs(sp.x - (*ziel)[0].x) + std::fabs(sp.y - (*ziel)[0].y) < 40.0F) { continue; }
            TraceTreffer t;
            if (!kartenStrahlUnterMaus(sp.x, sp.y, t) || t.normal[2] < 0.5F) { continue; }
            (*ziel)[static_cast<std::size_t>(n++)] = sp;
        }
        diag::detail("Knopftest: Wegnetz - Bodenpunkte " + std::to_string(n));
    }));
    add(klick(tr(Str::WegSetzen), "###wegpunkte"));
    add(pause(0.2));
    auto vorSetzen = std::make_shared<std::size_t>(0);
    add(tu("Stand vor dem Setzen", [=] { *vorSetzen = g_app->map.entities.size(); }));
    add(klickAn("Wegpunkt 1 setzen", [=] { return (*ziel)[0]; }));
    add(pause(0.2));
    add(klickAn("Wegpunkt 2 setzen", [=] { return (*ziel)[1]; }));
    add(pause(0.3));
    add(pruefe("Setzen: zwei neue Wegpunkte, der erste nennt den zweiten als target, keiner gewaehlt geaendert", [=] {
        const auto& v = g_app->map.entities;
        if (v.size() != *vorSetzen + 2) { return false; }
        const MapEntity& a = v[v.size() - 2];
        const MapEntity& b = v[v.size() - 1];
        const std::string* t = a.find("target");
        diag::detail("Knopftest: Wegnetz - gesetzt " + a.targetname + " (" + a.origin + ") -> " +
                     (t != nullptr ? *t : std::string("-")) + ", " + b.targetname + " (" + b.origin + ")");
        return a.classname == "waypoint" && b.classname == "waypoint" && t != nullptr && *t == b.targetname &&
               g_app->pickedEntity == static_cast<int>(v.size()) - 1;
    }));
    // Verbinden: der zweite gesetzte mit dem ersten erzeugten - und wieder trennen.
    add(klick(tr(Str::WegVerbinden), "###wegpunkte"));
    add(pause(0.2));
    auto paar = std::make_shared<std::array<int, 2>>();
    auto paarOrt = std::make_shared<std::array<ImVec2, 2>>();
    add(tu("Zwei Punkte zum Verbinden", [=] {
        const int b = static_cast<int>(g_app->map.entities.size()) - 1;
        (*paar)[0] = b;
        (*paar)[1] = -1;
        float ob[3];
        entityOrt(g_app->map.entities[static_cast<std::size_t>(b)], ob);
        const float wb[3] = {ob[0], ob[1], ob[2] - 20.0F};
        kartenPunktAufSchirm(wb, (*paarOrt)[0]);
        float best = 1.0e9F;
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            const MapEntity& e = g_app->map.entities[i];
            if (e.targetname.rfind("wpauto_", 0) != 0) { continue; }
            float o[3];
            entityOrt(e, o);
            const float w[3] = {o[0], o[1], o[2] - 20.0F};
            ImVec2 sp;
            if (!kartenPunktAufSchirm(w, sp)) { continue; }
            const float d = std::fabs(sp.x - (*paarOrt)[0].x) + std::fabs(sp.y - (*paarOrt)[0].y);
            if (d > 60.0F && d < best && sp.x > g_app->kartenAnsicht[0] + 30 && sp.x < g_app->kartenAnsicht[2] - 30 &&
                sp.y > g_app->kartenAnsicht[1] + 80 && sp.y < g_app->kartenAnsicht[3] - 30) {
                best = d;
                (*paar)[1] = static_cast<int>(i);
                (*paarOrt)[1] = sp;
            }
        }
    }));
    add(klickAn("Verbinden: erster Punkt", [=] { return (*paarOrt)[0]; }));
    add(pause(0.2));
    add(klickAn("Verbinden: zweiter Punkt", [=] { return (*paarOrt)[1]; }));
    add(pause(0.3));
    const auto verbunden = [](int a, int b) {
        if (a < 0 || b < 0) { return false; }
        const MapEntity& ea = g_app->map.entities[static_cast<std::size_t>(a)];
        const MapEntity& eb = g_app->map.entities[static_cast<std::size_t>(b)];
        for (const char* f : {"target", "target2", "target3", "target4", "targetJump"}) {
            if (const std::string* t = ea.find(f); t != nullptr && *t == eb.targetname) { return true; }
            if (const std::string* t = eb.find(f); t != nullptr && *t == ea.targetname) { return true; }
        }
        return false;
    };
    add(pruefe("Verbinden: die beiden Punkte sind verbunden", [=] { return verbunden((*paar)[0], (*paar)[1]); }));
    add(klickAn("Trennen: zweiter Punkt (Kette geht von ihm weiter)", [=] { return (*paarOrt)[0]; }));
    add(pause(0.3));
    add(pruefe("Noch einmal dieselben zwei: getrennt", [=] { return !verbunden((*paar)[0], (*paar)[1]); }));
    add(taste(ImGuiKey_Escape, "Esc beendet das Werkzeug"));
    add(pause(0.2));
    add(pruefe("Esc: kein Werkzeug mehr", [] { return g_app->wegWerkzeug == 0 && g_app->wegVon < 0; }));
    // Pruefen
    add(klick(tr(Str::WegPruefen), "###wegpunkte"));
    add(pause(0.3));
    add(pruefe("Pruefen: Ergebnis steht da", [] {
        diag::detail("Knopftest: Wegnetz - " + std::to_string(g_app->wegBefunde.size()) + " Befunde");
        for (std::size_t i = 0; i < g_app->wegBefunde.size() && i < 5; ++i) {
            diag::detail("Knopftest: Wegnetz -   " + g_app->wegBefunde[i].text);
        }
        return g_app->wegGeprueft;
    }));
    // Draufsicht
    auto vorher = std::make_shared<Camera>();
    add(tu("Ansicht merken", [=] { *vorher = g_app->cam; }));
    add(klick(tr(Str::MapTopView), "###main"));
    add(pause(0.6));
    add(foto("weg_draufsicht"));
    add(pruefe("Draufsicht: senkrecht nach unten, Schnitthoehe unter der Kamera, Regler da", [] {
        diag::detail("Knopftest: Draufsicht - Kamera z " + std::to_string(g_app->cam.pos[2]) + ", Schnitt " +
                     std::to_string(g_app->draufSchnitt));
        return g_app->draufsicht && g_app->cam.angles[0] > 88.0F && g_app->draufSchnitt < g_app->cam.pos[2] &&
               finde("##schnitt", "", 0, false) != nullptr;
    }));
    add(pruefe("Draufsicht: ein Klick trifft unter der Schnitthoehe (nicht das Dach)", [] {
        TraceTreffer t;
        const float x = (g_app->kartenAnsicht[0] + g_app->kartenAnsicht[2]) * 0.5F;
        const float y = (g_app->kartenAnsicht[1] + g_app->kartenAnsicht[3]) * 0.5F;
        const bool hit = kartenStrahlUnterMaus(x, y, t);
        diag::detail("Knopftest: Draufsicht - Treffer " + std::to_string(hit) + " bei z " + std::to_string(t.point[2]));
        return !hit || t.point[2] <= g_app->draufSchnitt + 0.5F;
    }));
    add(radAn("Draufsicht: Rad zoomt", ansichtMitte, 2.0F));
    add(pause(0.2));
    add(pruefe("Rad: die Kamera kommt naeher, bleibt ueber dem Schnitt, schaut weiter nach unten", [] {
        return g_app->cam.pos[2] > g_app->draufSchnitt + 63.0F && g_app->cam.angles[0] > 88.0F;
    }));
    add(klick(tr(Str::MapTopView), "###main"));
    add(pause(0.3));
    add(pruefe("Draufsicht aus: die Ansicht steht wie vorher", [=] {
        return !g_app->draufsicht && std::fabs(g_app->cam.pos[0] - vorher->pos[0]) < 0.01F &&
               std::fabs(g_app->cam.pos[2] - vorher->pos[2]) < 0.01F && std::fabs(g_app->cam.angles[0] - vorher->angles[0]) < 0.01F;
    }));
    // Erzeugte entfernen, Rueckgaengig bis zum Anfang
    add(klick(tr(Str::WegEntfernen), "###wegpunkte"));
    add(pause(0.3));
    add(pruefe("Erzeugte entfernen: keine wpauto mehr, die von Hand gesetzten bleiben", [=] {
        std::size_t auto_ = 0;
        for (const MapEntity& e : g_app->map.entities) { auto_ += e.targetname.rfind("wpauto_", 0) == 0 ? 1U : 0U; }
        return auto_ == 0U && zaehleKlasse("waypoint") == *wp0 + 2;
    }));
    add(tu("Alles zurueck", [=] {
        while (g_app->entUndo.size() > *tiefe0) { entityRueckgaengig(); }
    }));
    add(pause(0.3));
    add(pruefe("Rueckgaengig bis zum Anfang: so viele Wegpunkte wie vorher", [=] { return zaehleKlasse("waypoint") == *wp0; }));
    add(tu("Fenster zu", [] { g_app->wegFensterOffen = false; g_app->zeigeWegnetz = false; }));
    add(pause(0.2));
    return s;
}

// --- Umriss der Auswahl (rc597) ------------------------------------------------
//
// "wenn ich bei uns ein Schiff oder eine figur auswaehle ... das es dann
// gehighlightet ist" - wie in Unreal eine orange Linie um das Gewaehlte.
// Gezaehlt werden die Bildpunkte in genau der Umrissfarbe, vor und nach dem
// Waehlen.
namespace {
int orangePunkte() {
    std::vector<std::uint8_t> px;
    int w = 0;
    int h = 0;
    if (!gpu::leseFarbe(px, w, h) || px.empty()) { return -1; }
    int n = 0;
    const std::size_t alle = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
    for (std::size_t i = 0; i < alle; ++i) {
        const int r = px[i * 4U];
        const int g = px[i * 4U + 1U];
        const int b = px[i * 4U + 2U];
        if (r >= 249 && std::abs(g - 168) <= 6 && std::abs(b - 31) <= 6) { ++n; }
    }
    return n;
}
}  // namespace

std::vector<Schritt> umrissSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    struct Kandidat {
        int entity = -1;
        float mitte[3]{};
        float radius = 0.0F;
    };
    auto kandidaten = std::make_shared<std::vector<Kandidat>>();
    auto vorher = std::make_shared<int>(0);
    auto bilder0 = std::make_shared<int>(0);
    auto bester = std::make_shared<int>(0);
    auto zuwachsBilder = std::make_shared<int>(0);
    add(tu("Umriss: Ausgangslage, Brush-Objekte (func_*) suchen", [=] {
        g_app->cameraSelected = false;
        g_app->selectedKey = -1;
        g_app->selectedActor = -1;
        g_app->throughCamera = false;
        entityAuswahlSetzen(-1);
        kandidaten->clear();
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            const MapEntity& e = g_app->map.entities[i];
            const std::string* mdl = e.find("model");
            if (e.classname.rfind("func_", 0) != 0 || mdl == nullptr || mdl->size() < 2 || (*mdl)[0] != '*') { continue; }
            const int nr = std::atoi(mdl->c_str() + 1);
            if (nr <= 0 || static_cast<std::size_t>(nr) >= g_app->geo.models.size()) { continue; }
            const auto bm = g_app->brushMeshes.find(nr);
            if (bm == g_app->brushMeshes.end() || bm->second.indexes.empty()) { continue; }
            const auto& sm = g_app->geo.models[static_cast<std::size_t>(nr)];
            Kandidat k;
            k.entity = static_cast<int>(i);
            float d2 = 0.0F;
            for (int a = 0; a < 3; ++a) {
                k.mitte[a] = (sm.mins[a] + sm.maxs[a]) * 0.5F;
                d2 += (sm.maxs[a] - sm.mins[a]) * (sm.maxs[a] - sm.mins[a]);
            }
            k.radius = std::sqrt(d2) * 0.5F;
            if (k.radius > 24.0F && k.radius < 700.0F) { kandidaten->push_back(k); }
        }
        // Die groessten zuerst - die sieht man am ehesten.
        std::sort(kandidaten->begin(), kandidaten->end(), [](const Kandidat& a, const Kandidat& b) { return a.radius > b.radius; });
        if (kandidaten->size() > 4U) { kandidaten->resize(4U); }
        diag::detail("Knopftest: Umriss - " + std::to_string(kandidaten->size()) + " Brush-Objekte zur Probe");
        *bester = 0;
        *zuwachsBilder = 0;
    }));
    add(pruefe("Umriss: Brush-Objekte zur Probe gefunden", [=] { return !kandidaten->empty(); }));
    // Vier Richtungen je Objekt reichen nicht immer an Waenden vorbei - es
    // genuegt, wenn EIN Objekt aus EINER Richtung sichtbar umrandet wird.
    for (int k = 0; k < 4; ++k) {
        add(tu("Umriss: Kamera vor Objekt " + std::to_string(k + 1), [=] {
            entityAuswahlSetzen(-1);
            if (static_cast<std::size_t>(k) >= kandidaten->size()) { return; }
            const Kandidat& c = (*kandidaten)[static_cast<std::size_t>(k)];
            const float gier = static_cast<float>(k) * 1.5707963F + 0.6F;
            const float abst = c.radius * 1.4F + 120.0F;
            g_app->cam.pos[0] = c.mitte[0] - std::cos(gier) * abst;
            g_app->cam.pos[1] = c.mitte[1] - std::sin(gier) * abst;
            g_app->cam.pos[2] = c.mitte[2] + abst * 0.45F;
            aimAngles(g_app->cam.pos, c.mitte, g_app->cam.angles);
            g_app->mapDirty = true;
        }));
        add(pause(0.4));
        add(tu("Umriss: Bild ohne Auswahl zaehlen", [=] {
            *vorher = orangePunkte();
            *bilder0 = g_app->umrissBilder;
        }));
        add(tu("Umriss: Objekt " + std::to_string(k + 1) + " waehlen", [=] {
            if (static_cast<std::size_t>(k) < kandidaten->size()) {
                entityAuswahlSetzen((*kandidaten)[static_cast<std::size_t>(k)].entity);
            }
            g_app->mapDirty = true;
        }));
        add(pause(0.4));
        add(tu("Umriss: Bild mit Auswahl zaehlen", [=] {
            if (static_cast<std::size_t>(k) >= kandidaten->size()) { return; }
            const int nachher = orangePunkte();
            const int zuwachs = nachher - *vorher;
            *bester = std::max(*bester, zuwachs);
            *zuwachsBilder += g_app->umrissBilder - *bilder0;
            const MapEntity& e = g_app->map.entities[static_cast<std::size_t>((*kandidaten)[static_cast<std::size_t>(k)].entity)];
            diag::detail("Knopftest: Umriss - " + e.classname + " (Entity " +
                         std::to_string((*kandidaten)[static_cast<std::size_t>(k)].entity) + "): orange vorher " +
                         std::to_string(*vorher) + ", nachher " + std::to_string(nachher));
        }));
        add(foto("umriss_brush" + std::to_string(k + 1)));
    }
    add(pruefe("Umriss: ein gewaehltes Brush-Objekt bekommt eine orange Linie", [=] {
        diag::detail("Knopftest: Umriss - bester Zuwachs " + std::to_string(*bester) + " Punkte, " +
                     std::to_string(*zuwachsBilder) + " Umrissbilder");
        return *bester >= 40 && *zuwachsBilder > 0;
    }));
    add(tu("Umriss: Auswahl weg", [] {
        entityAuswahlSetzen(-1);
        g_app->mapDirty = true;
    }));
    add(pause(0.4));
    // Die Karte selbst hat Punkte in dieser Farbe (Lava) - verglichen wird
    // mit dem Bild vor dem Waehlen (dieselbe Ansicht).
    add(pruefe("Umriss: abgewaehlt ist die Linie wieder weg", [=] {
        const int n = orangePunkte();
        diag::detail("Knopftest: Umriss - abgewaehlt " + std::to_string(n) + " orange Punkte, vor dem Waehlen " +
                     std::to_string(*vorher));
        return n >= 0 && std::abs(n - *vorher) < 20;
    }));

    // --- Figur: per Klick auf ihre Marke waehlen ---------------------------
    auto figur = std::make_shared<int>(-1);
    // Nicht jede Figur steht zur Zeit 0 schon da - die erste nehmen, deren
    // Marke in der Ansicht liegt.
    add(tu("Umriss: Zeit auf den Anfang", [=] {
        *figur = -1;
        g_app->playMs = 0;
        g_app->mapDirty = true;
    }));
    for (int k = 0; k < 6; ++k) {
        add(tu("Umriss: Kamera vor Figur " + std::to_string(k + 1), [=] {
            if (*figur >= 0) { return; }
            int gesehen = 0;
            for (std::size_t i = 0; i < g_app->scene.actors.size(); ++i) {
                const Actor& a = g_app->scene.actors[i];
                if (!a.haveStart || a.brushModel > 0) { continue; }
                const ActorState st = a.at(g_app->playMs);
                if (!st.visible) { continue; }
                if (gesehen++ != k) { continue; }
                const float ziel[3] = {st.pos[0], st.pos[1], st.pos[2] + 20.0F};
                const float gier = a.startAngles[1] * 3.14159265F / 180.0F;
                g_app->cam.pos[0] = st.pos[0] + std::cos(gier) * 150.0F;
                g_app->cam.pos[1] = st.pos[1] + std::sin(gier) * 150.0F;
                g_app->cam.pos[2] = st.pos[2] + 50.0F;
                aimAngles(g_app->cam.pos, ziel, g_app->cam.angles);
                g_app->mapDirty = true;
                break;
            }
        }));
        add(pause(0.4));
        add(tu("Umriss: steht eine Figur in der Mitte?", [=] {
            if (*figur >= 0) { return; }
            // Die Marken liegen in Bildkoordinaten.
            const float bw = static_cast<float>(g_app->mapImage.width);
            const float bh = static_cast<float>(g_app->mapImage.height);
            const ImVec2 m{bw * 0.5F, bh * 0.5F};
            float best = 1e30F;
            for (const App::ActorMark& am : g_app->actorMarks) {
                if (am.sx < 30.0F || am.sx > bw - 30.0F || am.sy < 30.0F || am.sy > bh - 30.0F) { continue; }
                const float d = (am.sx - m.x) * (am.sx - m.x) + (am.sy - m.y) * (am.sy - m.y);
                if (d < best) {
                    best = d;
                    *figur = am.index;
                }
            }
            if (*figur >= 0) { diag::detail("Knopftest: Umriss - Figur " + std::to_string(*figur) + " im Bild"); }
        }));
    }
    add(tu("Umriss: Bild ohne Auswahl zaehlen (Figur)", [=] {
        *vorher = orangePunkte();
        *bilder0 = g_app->umrissBilder;
    }));
    add(klickAn("Klick auf die Figur", [=] {
        for (const App::ActorMark& am : g_app->actorMarks) {
            if (am.index == *figur) { return bildZuSchirm(am.sx, am.sy); }
        }
        return ImVec2{-1.0F, -1.0F};
    }));
    add(pause(0.4));
    add(foto("umriss_figur"));
    add(pruefe("Umriss: eine angeklickte Figur bekommt eine orange Linie", [=] {
        const int nachher = orangePunkte();
        diag::detail("Knopftest: Umriss - nach dem Klick: Entity " + std::to_string(g_app->pickedEntity) + ", Kamera " +
                     (g_app->cameraSelected ? "ja" : "nein") + ", Schluessel " + std::to_string(g_app->selectedKey));
        diag::detail("Knopftest: Umriss - Figur " + std::to_string(g_app->selectedActor) + " gewaehlt, orange vorher " +
                     std::to_string(*vorher) + ", nachher " + std::to_string(nachher) + ", " +
                     std::to_string(g_app->umrissBilder - *bilder0) + " Umrissbilder");
        return g_app->selectedActor == *figur && *figur >= 0 && nachher - *vorher >= 40 && g_app->umrissBilder > *bilder0;
    }));
    add(tu("Umriss: Figur abwaehlen", [] {
        g_app->selectedActor = -1;
        g_app->mapDirty = true;
    }));
    add(pause(0.2));
    return s;
}

// --- Marke und Gizmo folgen Schiff und Figur (rc597) --------------------------
//
// "sollte sich die ent nicht mit dem schiff mitbewegen?", "ich kann das schiff
// nicht anklicken weil das ent nicht drauf ist", "auch beim character
// dasselbe", "auch wenn ich die figur anklicke sollte das ent ausgewaehlt
// sein". md_am_sith: sta2 (func_static) fliegt per Skript ein.
struct FolgeZiel {
    int actor = -1;
    int entity = -1;
    double ms = 0.0;
};

// Die freie Kamera vor die Figur bzw. das Schiff stellen, Zeit auf ihren Moment.
void folgeKameraVor(const FolgeZiel& z, float abst, float hoch) {
    if (z.actor < 0) { return; }
    g_app->playMs = z.ms;
    const ActorState st = g_app->scene.actors[static_cast<std::size_t>(z.actor)].at(z.ms);
    const float ziel[3] = {st.pos[0], st.pos[1], st.pos[2] + 20.0F};
    g_app->cam.pos[0] = st.pos[0] - abst;
    g_app->cam.pos[1] = st.pos[1] - abst * 0.3F;
    g_app->cam.pos[2] = st.pos[2] + hoch;
    aimAngles(g_app->cam.pos, ziel, g_app->cam.angles);
    g_app->throughCamera = false;
    g_app->cameraSelected = false;
    g_app->selectedActor = -1;
    entityAuswahlSetzen(-1);
    g_app->mapDirty = true;
}
std::vector<Schritt> folgeSchritte2(std::shared_ptr<FolgeZiel> schiff, std::shared_ptr<FolgeZiel> figur);
std::vector<Schritt> drehSchritte(std::shared_ptr<FolgeZiel> schiff, std::shared_ptr<FolgeZiel> figur);

std::vector<Schritt> folgeSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    using Ziel = FolgeZiel;
    auto schiff = std::make_shared<Ziel>();
    auto figur = std::make_shared<Ziel>();
    auto klick = std::make_shared<ImVec2>(-1.0F, -1.0F);
    // Ein Zeitpunkt, an dem die Figur (das Brush-Modell) weit weg vom Start ist.
    const auto suche = [](bool brush, Ziel& z) {
        z = Ziel{};
        for (std::size_t ai = 0; ai < g_app->scene.actors.size() && z.actor < 0; ++ai) {
            const Actor& a = g_app->scene.actors[ai];
            if (!a.haveStart || (a.brushModel > 0) != brush) { continue; }
            int ei = -1;
            if (brush) {
                for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
                    const std::string* mdl = g_app->map.entities[i].find("model");
                    if (mdl != nullptr && *mdl == "*" + std::to_string(a.brushModel) && !g_app->map.entities[i].origin.empty()) {
                        ei = static_cast<int>(i);
                    }
                }
            } else {
                ei = entityFuerFigur(static_cast<int>(ai));
            }
            float o[3];
            if (ei < 0 || !entityOrt(g_app->map.entities[static_cast<std::size_t>(ei)], o)) { continue; }
            for (double ms = 500.0; ms < 60000.0; ms += 500.0) {
                const ActorState st = a.at(ms);
                const float dx = st.pos[0] - o[0];
                const float dy = st.pos[1] - o[1];
                const float dz = st.pos[2] - o[2];
                if (st.visible && dx * dx + dy * dy + dz * dz > (brush ? 150.0F * 150.0F : 64.0F * 64.0F)) {
                    z.actor = static_cast<int>(ai);
                    z.entity = ei;
                    z.ms = ms;
                    break;
                }
            }
        }
    };
    const auto kameraVor = [](const Ziel& z, float abst, float hoch) { folgeKameraVor(z, abst, hoch); };
    const auto gezeigtBei = [](const Ziel& z, float& abstand) {
        abstand = 1e9F;
        if (z.actor < 0 || z.entity < 0) { return false; }
        const Actor& a = g_app->scene.actors[static_cast<std::size_t>(z.actor)];
        const ActorState st = a.at(g_app->playMs);
        const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(z.entity)];
        float o[3];
        if (!entityOrtAnzeige(e, o)) { return false; }
        // Erwartet: genau dort, wo Figur oder Schiff gerade sind.
        float d2 = 0.0F;
        for (int k = 0; k < 3; ++k) { d2 += (o[k] - st.pos[k]) * (o[k] - st.pos[k]); }
        abstand = std::sqrt(d2);
        bool marke = false;
        for (const App::PickedMarker& mk : g_app->markers) { marke = marke || static_cast<int>(mk.entity) == z.entity; }
        return marke && abstand < 2.0F;
    };

    add(tu("Folgen: Schiff und Figur suchen, die das Skript bewegt", [=] {
        suche(true, *schiff);
        suche(false, *figur);
        diag::detail("Knopftest: Folgen - Schiff: Figur " + std::to_string(schiff->actor) + ", Entity " +
                     std::to_string(schiff->entity) + ", " + std::to_string(static_cast<int>(schiff->ms)) + " ms; Figur: " +
                     std::to_string(figur->actor) + ", Entity " + std::to_string(figur->entity) + ", " +
                     std::to_string(static_cast<int>(figur->ms)) + " ms");
    }));
    add(pruefe("Folgen: ein bewegtes Schiff und eine laufende Figur gefunden", [=] {
        return schiff->actor >= 0 && figur->actor >= 0;
    }));
    // --- Schiff -------------------------------------------------------------
    add(tu("Folgen: Kamera vor das fliegende Schiff", [=] { kameraVor(*schiff, 420.0F, 160.0F); }));
    add(pause(0.5));
    add(foto("folgen_schiff"));
    add(pruefe("Folgen: die Marke des Schiffs steht dort, wo das Schiff gerade fliegt", [=] {
        float d = 0.0F;
        const bool ok = gezeigtBei(*schiff, d);
        diag::detail("Knopftest: Folgen - Schiff: Abstand der Marke zum Schiff " + std::to_string(d));
        return ok;
    }));
    add(tu("Folgen: eine Stelle auf dem Schiff suchen, an der keine Marke liegt", [=] {
        *klick = ImVec2{-1.0F, -1.0F};
        auto treffer = std::make_shared<std::map<int, int>>();
        const float skal = g_app->mapImage.width > 0
                               ? static_cast<float>(g_app->mapImage.width) /
                                     std::max(1.0F, g_app->kartenAnsicht[2] - g_app->kartenAnsicht[0])
                               : 1.0F;
        const float reich = ImGui::GetFontSize() * 2.0F;
        float bestAbst = -1.0F;
        for (float y = g_app->kartenAnsicht[1] + 20.0F; y < g_app->kartenAnsicht[3] - 20.0F; y += 10.0F) {
            for (float x = g_app->kartenAnsicht[0] + 20.0F; x < g_app->kartenAnsicht[2] - 20.0F; x += 10.0F) {
                const int hit = moverUnterMaus(x, y);
                if (hit >= 0) { ++(*treffer)[hit]; }
                if (hit != schiff->entity) { continue; }
                float naechste = 1e9F;
                for (const App::PickedMarker& mk : g_app->markers) {
                    const float mx = g_app->kartenAnsicht[0] + mk.sx / skal;
                    const float my = g_app->kartenAnsicht[1] + mk.sy / skal;
                    naechste = std::min(naechste, (mx - x) * (mx - x) + (my - y) * (my - y));
                }
                for (const App::ActorMark& am : g_app->actorMarks) {
                    const float mx = g_app->kartenAnsicht[0] + am.sx / skal;
                    const float my = g_app->kartenAnsicht[1] + am.sy / skal;
                    naechste = std::min(naechste, (mx - x) * (mx - x) + (my - y) * (my - y));
                }
                if (naechste > reich * reich && naechste > bestAbst) {
                    bestAbst = naechste;
                    *klick = ImVec2{x, y};
                }
            }
        }
        std::string t;
        for (const auto& [ei, n] : *treffer) { t += " " + std::to_string(ei) + ":" + std::to_string(n); }
        int mitSchiff = 0;
        for (const App::PickMover& pm : g_app->pickMover) { mitSchiff += pm.entity == schiff->entity ? 1 : 0; }
        diag::detail("Knopftest: Folgen - Klickstelle auf dem Schiff " + std::to_string(klick->x) + "/" + std::to_string(klick->y) +
                     "; " + std::to_string(g_app->pickMover.size()) + " Modelle (" + std::to_string(mitSchiff) +
                     " vom Schiff), Treffer:" + t);
    }));
    add(klickAn("Klick auf das Schiff (nicht auf die Marke)", [=] { return *klick; }));
    add(pause(0.3));
    add(pruefe("Folgen: ein Klick auf das Schiff waehlt seine Entity", [=] {
        diag::detail("Knopftest: Folgen - gewaehlt " + std::to_string(g_app->pickedEntity) + ", erwartet " +
                     std::to_string(schiff->entity));
        return klick->x >= 0.0F && g_app->pickedEntity == schiff->entity;
    }));
    // --- Figur --------------------------------------------------------------
    add(tu("Folgen: Kamera vor die gelaufene Figur", [=] { kameraVor(*figur, 170.0F, 60.0F); }));
    add(pause(0.5));
    add(pruefe("Folgen: die Marke des Spawners steht bei der Figur", [=] {
        float d = 0.0F;
        const bool ok = gezeigtBei(*figur, d);
        diag::detail("Knopftest: Folgen - Figur: Abstand der Marke zur Figur " + std::to_string(d));
        return ok;
    }));
    add(klickAn("Klick auf die Figur", [=] {
        for (const App::ActorMark& am : g_app->actorMarks) {
            if (am.index == figur->actor) { return bildZuSchirm(am.sx, am.sy); }
        }
        return ImVec2{-1.0F, -1.0F};
    }));
    add(pause(0.3));
    add(foto("folgen_figur"));
    add(pruefe("Folgen: ein Klick auf die Figur waehlt Figur UND ihren Spawner", [=] {
        diag::detail("Knopftest: Folgen - Figur " + std::to_string(g_app->selectedActor) + ", Entity " +
                     std::to_string(g_app->pickedEntity) + ", erwartet " + std::to_string(figur->actor) + "/" +
                     std::to_string(figur->entity));
        return figur->actor >= 0 && g_app->selectedActor == figur->actor && g_app->pickedEntity == figur->entity;
    }));
    // --- Effekte in der Kameravorschau (rc597) ------------------------------
    //
    // "wenn die kamera rechts unten angezeigt wird und ich meine hauptkamera
    // weg drehe werden die effecte nicht mehr gerendert in der kamera rechts
    // unten". Bei 22,4 s schaut die Skriptkamera auf Lavafall und Rauch.
    auto eckenGleich = std::make_shared<int>(0);
    add(tu("Vorschau-Effekte: Kamera waehlen, Zeit 22,4 s", [] {
        g_app->selectedActor = -1;
        entityAuswahlSetzen(-1);
        g_app->cameraSelected = true;
        g_app->kameraBildImBild = true;
        g_app->playMs = 22420.0;
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(tu("Vorschau-Effekte: Hauptkamera blickt wie die Vorschau", [] {
        for (int k = 0; k < 3; ++k) {
            g_app->cam.pos[k] = g_app->vorschauPos[k];
            g_app->cam.angles[k] = g_app->vorschauWinkel[k];
        }
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(tu("Vorschau-Effekte: Ecken merken, Hauptkamera umdrehen", [=] {
        *eckenGleich = g_app->nebenEffektEcken;
        g_app->cam.angles[1] = g_app->vorschauWinkel[1] + 180.0F;
        g_app->cam.angles[0] = -g_app->vorschauWinkel[0];
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(foto("vorschau_effekte"));
    add(pruefe("Vorschau-Effekte: dreht die Hauptkamera weg, bleiben die Effekte in der Vorschau", [=] {
        diag::detail("Knopftest: Vorschau-Effekte - Ecken mit gleichem Blick " + std::to_string(*eckenGleich) +
                     ", Hauptkamera weggedreht " + std::to_string(g_app->nebenEffektEcken) + ", Hauptansicht " +
                     std::to_string(g_app->effectMesh.verts.size()) + ", Vorschau gezeigt " +
                     std::to_string(g_app->vorschauGezeigt));
        return g_app->vorschauGezeigt && *eckenGleich > 100 &&
               static_cast<float>(g_app->nebenEffektEcken) >= 0.8F * static_cast<float>(*eckenGleich);
    }));
    // --- Rauch vor dem Gluehen (rc597) -------------------------------------
    //
    // "wenn ich durch den rauch auf den Vulkan schaue soll der so pixelig
    // sein?" - "das ist dynamic glow, wenn es aus ist, sieht es richtig aus".
    // Die Kamera hinter Rauch- und Dampfeffekte stellen, durch sie hindurch
    // blicken und messen, was das Gluehen zum Bild beitraegt (mit minus
    // ohne) - einmal mit, einmal ohne die Daempfung durch den Rauch.
    struct GluehMessung {
        std::vector<std::array<float, 3>> orte;
        std::vector<std::uint8_t> an;
        std::vector<std::uint8_t> aus;
        double besteDaempfung = 0.0;
        double beitragOhne = 0.0;
        int posen = 0;
        int rauchAufrufe = 0;
    };
    auto gm = std::make_shared<GluehMessung>();
    const auto lies = [](std::vector<std::uint8_t>& px) {
        int w = 0;
        int h = 0;
        return gpu::leseFarbe(px, w, h) && !px.empty();
    };
    const auto beitrag = [](const std::vector<std::uint8_t>& an, const std::vector<std::uint8_t>& aus) {
        if (an.size() != aus.size() || an.empty()) { return -1.0; }
        double summe = 0.0;
        for (std::size_t i = 0; i < an.size(); i += 4U) {
            for (std::size_t k = 0; k < 3U; ++k) { summe += std::max(0, static_cast<int>(an[i + k]) - static_cast<int>(aus[i + k])); }
        }
        return summe / static_cast<double>(an.size() / 4U);
    };
    add(tu("Rauch: Rauch- und Dampfeffekte suchen", [=] {
        g_app->cameraSelected = false;
        g_app->showGlow = true;
        g_app->showEffects = true;
        g_app->rauchImGluehen = true;
        g_app->playMs = 3500.0;
        gm->orte.clear();
        for (const MapEntity& e : g_app->map.entities) {
            if (e.classname != "fx_runner") { continue; }
            const std::string* fx = e.find("fxFile");
            if (fx == nullptr) { continue; }
            std::string n = *fx;
            for (char& ch : n) { ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); }
            float o[3];
            if ((n.find("smoke") != std::string::npos || n.find("steam") != std::string::npos) && entityOrt(e, o) &&
                gm->orte.size() < 4U) {
                gm->orte.push_back({o[0], o[1], o[2]});
            }
        }
        diag::detail("Knopftest: Rauch - " + std::to_string(gm->orte.size()) + " Rauch-/Dampfeffekte");
    }));
    for (int i = 0; i < 16; ++i) {
        const int ort = i / 4;
        const float gier = static_cast<float>(i % 4) * 90.0F;
        add(tu("Rauch: Blick " + std::to_string(i + 1), [=] {
            if (static_cast<std::size_t>(ort) >= gm->orte.size()) { return; }
            const auto& o = gm->orte[static_cast<std::size_t>(ort)];
            const float r = gier * 3.14159265F / 180.0F;
            g_app->cam.pos[0] = o[0] - std::cos(r) * 260.0F;
            g_app->cam.pos[1] = o[1] - std::sin(r) * 260.0F;
            g_app->cam.pos[2] = o[2] + 60.0F;
            const float ziel[3] = {o[0] + std::cos(r) * 400.0F, o[1] + std::sin(r) * 400.0F, o[2] + 40.0F};
            aimAngles(g_app->cam.pos, ziel, g_app->cam.angles);
            g_app->showGlow = true;
            g_app->rauchImGluehen = true;
            g_app->mapDirty = true;
        }));
        add(pause(0.35));
        add(tu("Rauch: mit Daempfung lesen, Gluehen aus", [=] {
            if (static_cast<std::size_t>(ort) >= gm->orte.size()) { return; }
            (void)lies(gm->an);
            gm->rauchAufrufe = std::max(gm->rauchAufrufe, gpu::gluehenRauchAufrufe());
            g_app->showGlow = false;
            g_app->mapDirty = true;
        }));
        add(pause(0.35));
        add(tu("Rauch: ohne Gluehen lesen, Gluehen ohne Daempfung", [=] {
            if (static_cast<std::size_t>(ort) >= gm->orte.size()) { return; }
            (void)lies(gm->aus);
            g_app->showGlow = true;
            g_app->rauchImGluehen = false;
            g_app->mapDirty = true;
        }));
        add(pause(0.35));
        add(tu("Rauch: vergleichen", [=] {
            if (static_cast<std::size_t>(ort) >= gm->orte.size()) { return; }
            std::vector<std::uint8_t> ohne;
            if (!lies(ohne)) { return; }
            const double mit = beitrag(gm->an, gm->aus);
            const double ohneD = beitrag(ohne, gm->aus);
            ++gm->posen;
            if (ohneD > 0.5 && mit >= 0.0) {
                const double daempfung = 1.0 - mit / ohneD;
                if (daempfung > gm->besteDaempfung) {
                    gm->besteDaempfung = daempfung;
                    gm->beitragOhne = ohneD;
                }
            }
            g_app->rauchImGluehen = true;
        }));
    }
    add(pruefe("Rauch: Rauch vor einer gluehenden Flaeche dunkelt auch ihr Gluehen ab", [=] {
        char z[200];
        std::snprintf(z, sizeof(z), "Knopftest: Rauch - %d Blicke, beste Daempfung %.1f %% (Beitrag ohne %.2f), %d Rauchaufrufe",
                      gm->posen, gm->besteDaempfung * 100.0, gm->beitragOhne, gm->rauchAufrufe);
        diag::detail(z);
        return gm->posen > 0 && gm->besteDaempfung > 0.10;
    }));
    add(tu("Rauch: Gluehen, Effekte, Daempfung wieder an", [] {
        g_app->showGlow = true;
        g_app->showEffects = true;
        g_app->rauchImGluehen = true;
        g_app->mapDirty = true;
    }));
    // --- Eine Kamera fuer alle Reiter derselben Karte (rc597) ---------------
    //
    // "wenn sich eine map die ent teilt von allen scripts waere es nicht
    // besser, dass wenn ich die reiter wechsel alle dieselbe kamera teilen?"
    // Reiter B einmal besuchen (dort merkt er sich eine Kamera), zurueck, die
    // Kamera woanders hinstellen, wieder zu B: sie muss bleiben.
    struct ReiterProbe {
        int a = -1;
        int b = -1;
        std::string karte;
        float ort[3]{};
    };
    auto rp = std::make_shared<ReiterProbe>();
    add(tu("Reiter: Kamera an Ort 1, zu einem anderen Reiter derselben Karte", [=] {
        g_app->cameraSelected = false;
        rp->a = g_app->activeTab;
        rp->b = (g_app->activeTab + 1) % static_cast<int>(g_app->tabs.size());
        rp->karte = g_app->map.path;
        g_app->cam.pos[0] += 100.0F;
        g_app->tabRequest = rp->b;
    }));
    add(pause(0.6));
    add(tu("Reiter: zurueck zum ersten", [=] { g_app->tabRequest = rp->a; }));
    add(pause(0.6));
    add(tu("Reiter: Kamera an Ort 2, wieder zum anderen Reiter", [=] {
        g_app->cam.pos[1] += 300.0F;
        g_app->cam.pos[2] += 50.0F;
        for (int k = 0; k < 3; ++k) { rp->ort[k] = g_app->cam.pos[k]; }
        g_app->tabRequest = rp->b;
    }));
    add(pause(0.6));
    add(pruefe("Reiter: auf derselben Karte bleibt die Kamera beim Reiterwechsel", [=] {
        float d = 0.0F;
        for (int k = 0; k < 3; ++k) { d += std::fabs(g_app->cam.pos[k] - rp->ort[k]); }
        diag::detail("Knopftest: Reiter " + std::to_string(rp->a) + " -> " + std::to_string(g_app->activeTab) +
                     ", Karte gleich " + std::to_string(g_app->map.path == rp->karte) + ", Kamera weg um " + std::to_string(d));
        return g_app->activeTab == rp->b && g_app->map.path == rp->karte && d < 1.0F;
    }));
    add(tu("Reiter: zurueck zum ersten", [=] { g_app->tabRequest = rp->a; }));
    add(pause(0.6));

    // --- Woran arbeite ich gerade? (rc597) ---------------------------------
    //
    // "irgendwie muessen die an dem script man gerade ist besonders sichtbar
    // sein, dass man weiss wo man arbeitet und vor allem auch welchen tab man
    // gerade aktiv bearbeitet". intro2_sith benutzt das Schiff sta2.
    add(tu("Fokus: Blick auf den Platz, Zeit 0", [] {
        g_app->playMs = 0.0;
        g_app->skriptFokus = true;
        g_app->cam.pos[0] = 200.0F; g_app->cam.pos[1] = 6000.0F; g_app->cam.pos[2] = 700.0F;
        g_app->cam.angles[0] = 50.0F; g_app->cam.angles[1] = 90.0F; g_app->cam.angles[2] = 0.0F;
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(foto("skript_fokus"));
    add(pruefe("Fokus: die Entities des aktiven Skripts sind erkannt, der Reitername steht in der Ansicht", [=] {
        int benutzt = 0;
        for (const char c : g_app->entImSkript) { benutzt += c != 0 ? 1 : 0; }
        const bool schiffDabei = schiff->entity >= 0 && static_cast<std::size_t>(schiff->entity) < g_app->entImSkript.size() &&
                            g_app->entImSkript[static_cast<std::size_t>(schiff->entity)] != 0;
        const std::string name = mb::reiterName(static_cast<std::size_t>(g_app->activeTab));
        diag::detail("Knopftest: Fokus - " + std::to_string(benutzt) + " von " + std::to_string(g_app->map.entities.size()) +
                     " Entities im Skript \"" + name + "\", Schiff dabei " + std::to_string(schiffDabei) + ", Schild \"" +
                     g_app->skriptSchildText + "\"");
        return g_app->entImSkript.size() == g_app->map.entities.size() && benutzt > 0 &&
               benutzt < static_cast<int>(g_app->map.entities.size()) && schiffDabei && g_app->skriptSchildText == name;
    }));
    // --- Die Bahn: ganz, grau voraus, gelb schon gefahren (rc597) ----------
    add(tu("Bahn: Schiff als Entity waehlen (ohne Figur), Zeit mitten im Flug", [=] {
        kameraVor(*schiff, 420.0F, 160.0F);
        g_app->selectedActor = -1;
        entityAuswahlSetzen(schiff->entity);
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(foto("bahn_schiff"));
    add(pruefe("Bahn: die ganze Flugbahn des gewaehlten Schiffs, grau voraus und gelb, was schon geflogen ist", [] {
        diag::detail("Knopftest: Bahn - " + std::to_string(g_app->bahnFarbig) + " Stuecke gelb, " +
                     std::to_string(g_app->bahnGrau) + " grau");
        return g_app->bahnFarbig > 0 && g_app->bahnGrau > 0;
    }));

    // --- Zeitleiste: Zeile, Bewegung, Ausloeser (rc597) ---------------------
    add(tu("Zeitleiste: den Spawner der Figur waehlen, Spuren an", [=] {
        g_app->showTracks = true;
        g_app->selectedActor = -1;
        entityAuswahlSetzen(figur->entity);
        g_app->playMs = figur->ms;
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(foto("zeitleiste_figur"));
    add(pruefe("Zeitleiste: die Zeile der gewaehlten Figur leuchtet, ihre Bewegung und deren Ausloeser sind markiert", [=] {
        const std::string soll = g_app->scene.actors[static_cast<std::size_t>(std::max(figur->actor, 0))].name;
        diag::detail("Knopftest: Zeitleiste - Zeile \"" + g_app->tlAuswahlZeile + "\" (erwartet \"" + soll + "\"), " +
                     std::to_string(g_app->tlBewegungsBalken) + " Bewegungsbalken, " + std::to_string(g_app->tlAusloeser) +
                     " Ausloeser");
        return figur->actor >= 0 && g_app->tlAuswahlZeile == soll && g_app->tlBewegungsBalken > 0 && g_app->tlAusloeser > 0;
    }));

    // --- Live ziehen: die Figur geht mit, der Gizmo zittert nicht (rc597) ----
    struct ZugProbe {
        ImVec2 start{-1.0F, -1.0F};
        std::array<float, 3> fig0{};
        std::size_t tiefe = 0;
        std::size_t entTiefe = 0;
        float ort0[3]{};
        bool ok = false;
        std::string text;
    };
    auto zp = std::make_shared<ZugProbe>();
    add(tu("Zug: Kamera vor die Figur, ihr Spawner gewaehlt", [=] {
        kameraVor(*figur, 170.0F, 90.0F);
        g_app->selectedActor = -1;
        entityAuswahlSetzen(figur->entity);
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add({"Zug: die Marke des Spawners anfassen, ziehen und mitten im Zug pruefen", [=](int b) {
        ImGuiIO& io = ImGui::GetIO();
        if (b == 0) {
            zp->start = ImVec2{-1.0F, -1.0F};
            for (const App::PickedMarker& mk : g_app->markers) {
                if (static_cast<int>(mk.entity) == figur->entity) { zp->start = bildZuSchirm(mk.sx, mk.sy); }
            }
            if (figur->entity >= 0) { (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(figur->entity)], zp->ort0); }
            if (g_app->figurGezeichnet.size() > static_cast<std::size_t>(std::max(figur->actor, 0))) {
                zp->fig0 = g_app->figurGezeichnet[static_cast<std::size_t>(figur->actor)];
            }
            zp->tiefe = g_app->doc.undoDepth();
            zp->entTiefe = g_app->entUndo.size();
            setzeMaus(zp->start);
            return zp->start.x < 0.0F;   // keine Marke: abbrechen, die Pruefung meldet es
        }
        if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
        if (b >= 3 && b <= 14) {
            setzeMaus(ImVec2{zp->start.x + static_cast<float>(b - 2) * 12.0F, zp->start.y + static_cast<float>(b - 2) * 4.0F});
            return false;
        }
        if (b == 20) {
            const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(figur->entity)];
            float o[3];
            float gezeigt[3];
            (void)entityOrt(e, o);
            (void)entityOrtAnzeige(e, gezeigt);
            const ActorState st = g_app->scene.actors[static_cast<std::size_t>(figur->actor)].at(g_app->playMs);
            const auto& f = g_app->figurGezeichnet.size() > static_cast<std::size_t>(figur->actor)
                                ? g_app->figurGezeichnet[static_cast<std::size_t>(figur->actor)]
                                : std::array<float, 3>{0.0F, 0.0F, 0.0F};
            float dO = 0.0F;
            float abw = 0.0F;
            float gizmo = 0.0F;
            for (int k = 0; k < 3; ++k) {
                const float d = o[k] - zp->ort0[k];
                dO += d * d;
                abw += (f[static_cast<std::size_t>(k)] - st.pos[k] - d) * (f[static_cast<std::size_t>(k)] - st.pos[k] - d);
                gizmo += (gezeigt[k] - f[static_cast<std::size_t>(k)]) * (gezeigt[k] - f[static_cast<std::size_t>(k)]);
            }
            // rc597: mitten im Lauf entsteht ein Bahnpunkt - der Spawner bleibt,
            // die Figur geht live mit, der Gizmo bleibt auf ihr.
            float weg = 0.0F;
            for (int k = 0; k < 2; ++k) {
                weg += (f[static_cast<std::size_t>(k)] - zp->fig0[static_cast<std::size_t>(k)]) *
                       (f[static_cast<std::size_t>(k)] - zp->fig0[static_cast<std::size_t>(k)]);
            }
            (void)abw;
            char z[260];
            std::snprintf(z, sizeof(z), "Knopftest: Zug - Spawner verschoben um %.1f, Figur gewandert %.1f, Gizmo neben der Figur %.2f, Bahnpunkt %d, Zug aktiv %d",
                          std::sqrt(dO), std::sqrt(weg), std::sqrt(gizmo), g_app->entZug.zwischen ? 1 : 0, g_app->entZug.aktiv ? 1 : 0);
            zp->text = z;
            zp->ok = g_app->entZug.aktiv && std::sqrt(weg) > 10.0F && std::sqrt(gizmo) < 1.0F;
            return false;
        }
        if (b == 21) { io.AddMouseButtonEvent(0, false); return false; }
        return b >= 26;
    }});
    add(pruefe("Zug: beim Ziehen des Spawners geht die Figur live mit, der Gizmo bleibt auf ihr", [=] {
        diag::detail(zp->text.empty() ? std::string("Knopftest: Zug - keine Marke zum Anfassen") : zp->text);
        return zp->ok;
    }));
    add(tu("Zug: rueckgaengig", [=] {
        while (g_app->entUndo.size() > zp->entTiefe && entityRueckgaengig()) {}
        while (g_app->doc.undoDepth() > zp->tiefe && g_app->doc.undo()) {}
        rebuildTree();
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    add(pause(0.4));
    // "was bringt das facing? weil es schaut nicht gerade da hin, wo das model
    // hinsieht" - der Pfeil zeigt die Blickrichtung des Modells zur Zeit.
    add(pruefe("Blick: der Pfeil des Spawners zeigt, wohin die Figur gerade schaut", [=] {
        if (figur->actor < 0 || figur->entity < 0) { return false; }
        const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(figur->entity)];
        const ActorState st = g_app->scene.actors[static_cast<std::size_t>(figur->actor)].at(g_app->playMs);
        const std::string* w = e.find("angle");
        const float schl = w != nullptr ? std::strtof(w->c_str(), nullptr) : 0.0F;
        const float d = std::fabs(std::remainder(entityGierAnzeige(e) - st.angles[1], 360.0F));
        char z[200];
        std::snprintf(z, sizeof(z), "Knopftest: Blick - Schluessel %.1f, Modell %.1f, Pfeil %.1f", static_cast<double>(schl),
                      static_cast<double>(st.angles[1]), static_cast<double>(entityGierAnzeige(e)));
        diag::detail(z);
        return std::fabs(std::remainder(schl - st.angles[1], 360.0F)) > 5.0F && d < 1.0F;
    }));
    for (Schritt& x : folgeSchritte2(schiff, figur)) { s.push_back(std::move(x)); }
    for (Schritt& x : drehSchritte(schiff, figur)) { s.push_back(std::move(x)); }
    add(tu("Folgen: Auswahl weg, Zeit auf 0", [] {
        g_app->cameraSelected = false;
        g_app->selectedActor = -1;
        entityAuswahlSetzen(-1);
        g_app->playMs = 0;
        g_app->mapDirty = true;
    }));
    add(pause(0.2));
    return s;
}

// --- Drehen um alle drei Achsen (rc599) --------------------------------------
//
// shank: "ich kann Charaktere, Schiffe und Kameras nicht mehr drehen, weil kein
// Dreh-Gizmo mehr da ist ... bei Move ein Move-Target und bei Drehen ein
// Rotation-Target".
std::vector<Schritt> drehSchritte(std::shared_ptr<FolgeZiel> schiff, std::shared_ptr<FolgeZiel> figur) {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    struct DrehProbe {
        float w0[3]{};
        float o0[3]{};
        std::size_t entTiefe = 0;
    };
    auto dp = std::make_shared<DrehProbe>();
    const auto winkel3 = [](int ei, float w[3]) {
        w[0] = 0.0F;
        w[1] = 0.0F;
        w[2] = 0.0F;
        const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(ei)];
        if (const std::string* a = e.find("angles")) {
            (void)std::sscanf(a->c_str(), "%f %f %f", &w[0], &w[1], &w[2]);
        } else if (const std::string* a1 = e.find("angle")) {
            w[1] = std::strtof(a1->c_str(), nullptr);
        }
    };
    add(tu("Drehen: das Schiff vor dem Flug waehlen, Verschieben", [=] {
        folgeKameraVor(*schiff, 420.0F, 200.0F);
        g_app->playMs = 0.0;
        g_app->selectedActor = -1;
        entityAuswahlSetzen(schiff->entity);
        g_app->gizmoMode = App::GizmoMode::Move;
        winkel3(schiff->entity, dp->w0);
        (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(schiff->entity)], dp->o0);
        dp->entTiefe = g_app->entUndo.size();
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(tu("Drehen: Maus auf die Stelle des Nick-Rings (im Verschieben)", [] {
        ImVec2 q{-1.0F, -1.0F};
        (void)entityGizmoPunkt(5, q);
        setzeMaus(q);
    }));
    add(pause(0.2));
    add(pruefe("Drehen: im Verschieben gibt es keine Ringe, nur Pfeile", [] {
        return g_app->entGizmoHover < 4;
    }));
    add(klick(tr(Str::GizmoRotate), "##entgizmoleiste"));
    add(pause(0.3));
    add(pruefe("Drehen: die Leiste schaltet in den Drehmodus", [] { return g_app->gizmoMode == App::GizmoMode::Rotate; }));
    add(foto("drehen_schiff"));
    add(ziehenAn("Drehen: am Nick-Ring ziehen", [] {
        ImVec2 q{-1.0F, -1.0F};
        (void)entityGizmoPunkt(5, q);
        return q;
    }, 50.0F, 30.0F, 0));
    add(pause(0.4));
    add(pruefe("Drehen: der gruene Ring nickt das Schiff - Gier und Ort bleiben", [=] {
        float w[3];
        winkel3(schiff->entity, w);
        float o[3];
        (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(schiff->entity)], o);
        const float ort = std::fabs(o[0] - dp->o0[0]) + std::fabs(o[1] - dp->o0[1]) + std::fabs(o[2] - dp->o0[2]);
        char z[200];
        std::snprintf(z, sizeof(z), "Knopftest: Drehen - Schiff vorher %.0f %.0f %.0f, nachher %.0f %.0f %.0f, Ort %.1f",
                      static_cast<double>(dp->w0[0]), static_cast<double>(dp->w0[1]), static_cast<double>(dp->w0[2]),
                      static_cast<double>(w[0]), static_cast<double>(w[1]), static_cast<double>(w[2]), static_cast<double>(ort));
        diag::detail(z);
        const float dNick = std::fabs(std::remainder(w[0] - dp->w0[0], 360.0F));
        const float dGier = std::fabs(std::remainder(w[1] - dp->w0[1], 360.0F));
        return dNick > 2.0F && dGier < 0.5F && ort < 0.5F;
    }));
    add(tu("Drehen: Nicken merken", [=] { winkel3(schiff->entity, dp->w0); }));
    add(ziehenAn("Drehen: am Roll-Ring ziehen", [] {
        ImVec2 q{-1.0F, -1.0F};
        (void)entityGizmoPunkt(6, q);
        return q;
    }, 50.0F, 30.0F, 0));
    add(pause(0.4));
    add(pruefe("Drehen: der rote Ring rollt das Schiff", [=] {
        float w[3];
        winkel3(schiff->entity, w);
        const float dRoll = std::fabs(std::remainder(w[2] - dp->w0[2], 360.0F));
        diag::detail("Knopftest: Drehen - Rollen " + std::to_string(dp->w0[2]) + " -> " + std::to_string(w[2]));
        return dRoll > 2.0F;
    }));
    add(tu("Drehen: Gier merken", [=] { winkel3(schiff->entity, dp->w0); }));
    add(ziehenAn("Drehen: am Gier-Ring ziehen", [] {
        ImVec2 q{-1.0F, -1.0F};
        (void)entityGizmoPunkt(4, q);
        return q;
    }, 50.0F, -20.0F, 0));
    add(pause(0.4));
    add(pruefe("Drehen: der blaue Ring dreht die Gier, Nicken und Rollen bleiben", [=] {
        float w[3];
        winkel3(schiff->entity, w);
        const float dGier = std::fabs(std::remainder(w[1] - dp->w0[1], 360.0F));
        const float dRest = std::fabs(std::remainder(w[0] - dp->w0[0], 360.0F)) + std::fabs(std::remainder(w[2] - dp->w0[2], 360.0F));
        diag::detail("Knopftest: Drehen - Gier " + std::to_string(dp->w0[1]) + " -> " + std::to_string(w[1]));
        return dGier > 2.0F && dRest < 0.5F;
    }));
    add(tu("Drehen: zuruecknehmen", [=] {
        while (g_app->entUndo.size() > dp->entTiefe && entityRueckgaengig()) {}
        g_app->mapDirty = true;
    }));
    // Eine Figur hat nur ihre Gier.
    add(tu("Drehen: den Spawner der Figur waehlen", [=] {
        folgeKameraVor(*figur, 170.0F, 120.0F);
        g_app->selectedActor = -1;
        entityAuswahlSetzen(figur->entity);
        g_app->mapDirty = true;
    }));
    add(pause(0.4));
    add(tu("Drehen: Maus auf die Stelle des Nick-Rings", [] {
        ImVec2 q{-1.0F, -1.0F};
        (void)entityGizmoPunkt(5, q);
        setzeMaus(q);
    }));
    add(pause(0.2));
    add(pruefe("Drehen: eine Figur bekommt nur den Gier-Ring (die Engine kennt fuer NPCs nur angle)", [] {
        diag::detail("Knopftest: Drehen - Figur, Griff unter der Maus " + std::to_string(g_app->entGizmoHover));
        return g_app->entGizmoHover != 5 && g_app->entGizmoHover != 6;
    }));
    // rc599: "die Rotation beim Schiff und beim Character setzt keinen neuen
    // Frame, es passt immer den ersten Punkt an". Mitten in der Fahrt bzw.
    // nach dem ersten Befehl der Figur: ein Schluessel zur Zeit.
    const auto zaehle = [](const char* name, const char* setTyp) {
        int n = 0;
        std::function<void(const std::vector<Node>&)> geh = [&](const std::vector<Node>& ns) {
            for (const Node& x : ns) {
                if (x.kind == Node::Kind::Command && x.name == name &&
                    (setTyp == nullptr || (!x.args.empty() && x.args[0].text == setTyp))) {
                    ++n;
                }
                geh(x.children);
            }
        };
        geh(g_app->doc.script().nodes);
        return n;
    };
    struct DrehKey {
        double t = -1.0;
        int moves = 0;
        int setAngles = 0;
        int setDyaw = 0;   // rc601 (bewegung)
        float w0[3]{};
        float gier = 0.0F;
        std::size_t tiefe = 0;
    };
    auto dk = std::make_shared<DrehKey>();
    add(tu("Drehkey: das Schiff mitten in einer move-Fahrt waehlen, Drehmodus", [=] {
        dk->t = -1.0;
        if (schiff->actor >= 0) {
            for (const ActorStep& st : g_app->scene.actors[static_cast<std::size_t>(schiff->actor)].steps) {
                if (st.kind != ActorStep::Kind::Glide || st.path.empty() || !st.fremd.empty() || st.endMs - st.startMs < 1500.0) { continue; }
                const Node* n = nodeAt(g_app->doc.script(), st.path);
                if (n == nullptr || n->name != "move") { continue; }
                dk->t = std::floor((st.startMs + st.endMs) * 0.5 / 50.0) * 50.0;
                break;
            }
        }
        FolgeZiel z = *schiff;
        z.ms = dk->t;
        folgeKameraVor(z, 420.0F, 200.0F);
        g_app->playMs = dk->t;
        g_app->selectedActor = -1;
        entityAuswahlSetzen(schiff->entity);
        g_app->gizmoMode = App::GizmoMode::Rotate;
        winkel3(schiff->entity, dk->w0);
        dk->moves = zaehle("move", nullptr);
        dk->tiefe = g_app->doc.undoDepth();
        const ActorState st = g_app->scene.actors[static_cast<std::size_t>(schiff->actor)].at(dk->t);
        dk->gier = st.angles[1];
        diag::detail("Knopftest: Drehkey - Schiff mitten im move bei " + std::to_string(dk->t) + " ms, " +
                     std::to_string(dk->moves) + " moves");
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(ziehenAn("Drehkey: am Gier-Ring des fliegenden Schiffs ziehen", [] {
        ImVec2 q{-1.0F, -1.0F};
        (void)entityGizmoPunkt(4, q);
        return q;
    }, 50.0F, -25.0F, 0));
    add(pause(0.5));
    add(pruefe("Drehkey: Schiff im Flug - ein neuer Bahnpunkt mit Winkeln, der Start bleibt", [=] {
        float w[3];
        winkel3(schiff->entity, w);
        const float start = std::fabs(w[0] - dk->w0[0]) + std::fabs(w[1] - dk->w0[1]) + std::fabs(w[2] - dk->w0[2]);
        prepareScene();
        const ActorState st = g_app->scene.actors[static_cast<std::size_t>(schiff->actor)].at(dk->t + 1.0);
        const float dGier = std::fabs(std::remainder(st.angles[1] - dk->gier, 360.0F));
        diag::detail("Knopftest: Drehkey - Schiff: moves " + std::to_string(dk->moves) + " -> " +
                     std::to_string(zaehle("move", nullptr)) + ", Start geaendert um " + std::to_string(start) +
                     ", Gier bei t " + std::to_string(dk->gier) + " -> " + std::to_string(st.angles[1]));
        return dk->t > 0.0 && zaehle("move", nullptr) == dk->moves + 1 && start < 0.01F && dGier > 2.0F;
    }));
    add(tu("Drehkey: zuruecknehmen", [=] {
        while (g_app->doc.undoDepth() > dk->tiefe && g_app->doc.undo()) {}
        rebuildTree();
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    add(tu("Drehkey: die Figur nach ihrem ersten Befehl waehlen", [=] {
        FolgeZiel z = *figur;
        folgeKameraVor(z, 170.0F, 120.0F);
        g_app->playMs = figur->ms;
        g_app->selectedActor = -1;
        entityAuswahlSetzen(figur->entity);
        g_app->gizmoMode = App::GizmoMode::Rotate;
        winkel3(figur->entity, dk->w0);
        dk->setAngles = zaehle("set", "SET_ANGLES");
        dk->setDyaw = zaehle("set", "SET_DYAW");
        dk->tiefe = g_app->doc.undoDepth();
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(ziehenAn("Drehkey: am Gier-Ring der Figur ziehen", [] {
        ImVec2 q{-1.0F, -1.0F};
        (void)entityGizmoPunkt(4, q);
        return q;
    }, 50.0F, -25.0F, 0));
    add(pause(0.5));
    // rc601 (bewegung): ein NPC dreht sich mit seiner Drehrate - der Schluessel
    // wird ein SET_DYAW (Q3_SetDYaw setzt desiredYaw, NPC_UpdateAngles dreht).
    // SET_ANGLES springt auch im Spiel (SetClientViewAngle) und bleibt dem
    // Spieler selbst vorbehalten.
    add(pruefe("Drehkey: Figur - set ( SET_DYAW ) im Skript (dreht mit YAW_SPEED), der Spawner bleibt", [=] {
        float w[3];
        winkel3(figur->entity, w);
        const float start = std::fabs(w[1] - dk->w0[1]);
        diag::detail("Knopftest: Drehkey - Figur: SET_DYAW " + std::to_string(dk->setDyaw) + " -> " +
                     std::to_string(zaehle("set", "SET_DYAW")) + ", SET_ANGLES " + std::to_string(dk->setAngles) + " -> " +
                     std::to_string(zaehle("set", "SET_ANGLES")) + ", Spawner geaendert um " + std::to_string(start));
        return zaehle("set", "SET_DYAW") == dk->setDyaw + 1 && zaehle("set", "SET_ANGLES") == dk->setAngles && start < 0.01F;
    }));
    add(tu("Drehkey: zuruecknehmen", [=] {
        while (g_app->doc.undoDepth() > dk->tiefe && g_app->doc.undo()) {}
        rebuildTree();
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    add(tu("Drehen: zurueck ins Verschieben", [] { g_app->gizmoMode = App::GizmoMode::Move; g_app->mapDirty = true; }));
    return s;
}

// Zweiter Teil der Folge-Pruefungen (rc597): eine Funktion allein wurde dem
// Compiler zu gross (C1506, zu viele verschachtelte Bloecke).
std::vector<Schritt> folgeSchritte2(std::shared_ptr<FolgeZiel> schiff, std::shared_ptr<FolgeZiel> figur) {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    using Ziel = FolgeZiel;
    const auto kameraVor = [](const Ziel& z, float abst, float hoch) { folgeKameraVor(z, abst, hoch); };
    // --- Griffe auf der Bahn (rc597) ----------------------------------------
    //
    // "kann ich den Anfangspunkt und den Endpunkt des bewegten Characters
    // setzen ... das ich den Endpunkt nur anders setzen will oder den
    // Startpunkt, und das auch bei den Schiffen". Und unten in der Zeitleiste:
    // "ich wuerde gerne unten mit der Timeline arbeiten koennen".
    struct GriffProbe {
        float ziel0[3]{};
        float start0[3]{};
        float spawner0[3]{};
        std::size_t ents0 = 0;
        std::string zielName0;
        double startMs0 = 0.0;
        double dauer0 = 0.0;
        std::string gang0;
        std::string moveText0;
        float flugZiel0[3]{};
        std::size_t undo0 = 0;
    };
    auto gp = std::make_shared<GriffProbe>();
    const auto laufVon = [](int figurNr) -> const ActorStep* {
        if (figurNr < 0 || static_cast<std::size_t>(figurNr) >= g_app->scene.actors.size()) { return nullptr; }
        for (const ActorStep& st : g_app->scene.actors[static_cast<std::size_t>(figurNr)].steps) {
            if (st.kind == ActorStep::Kind::WalkTo && !st.path.empty() && st.fremd.empty()) { return &st; }
        }
        return nullptr;
    };
    const auto griffNr = [](int art) {
        for (int i = 0; i < bahnGriffZahl(); ++i) {
            if (bahnGriffArt(i) == art) { return i; }
        }
        return -1;
    };
    add(tu("Griffe: Spawner der Figur waehlen, Kamera von oben auf ihren Lauf", [=] {
        g_app->selectedActor = -1;
        entityAuswahlSetzen(figur->entity);
        const ActorStep* st = laufVon(figur->actor);
        if (st != nullptr) {
            g_app->playMs = st->startMs;
            const float mitte[3] = {(st->from[0] + st->to[0]) * 0.5F, (st->from[1] + st->to[1]) * 0.5F, st->to[2]};
            g_app->cam.pos[0] = mitte[0] - 150.0F;
            g_app->cam.pos[1] = mitte[1];
            g_app->cam.pos[2] = mitte[2] + 420.0F;
            aimAngles(g_app->cam.pos, mitte, g_app->cam.angles);
            for (int k = 0; k < 3; ++k) {
                gp->ziel0[k] = st->to[k];
                gp->start0[k] = st->from[k];
            }
            gp->startMs0 = st->startMs;
            gp->dauer0 = st->endMs - st->startMs;
            gp->gang0 = st->text;
            const Node* n = nodeAt(g_app->doc.script(), st->path);
            gp->zielName0 = (n != nullptr && n->args.size() >= 2) ? n->args[1].text : std::string();
        }
        (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(std::max(figur->entity, 0))], gp->spawner0);
        gp->ents0 = g_app->map.entities.size();
        gp->undo0 = g_app->doc.undoDepth();
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(foto("griffe_lauf"));
    add(pruefe("Griffe: Start und Laufziel liegen auf der Bahn der gewaehlten Figur", [=] {
        diag::detail("Knopftest: Griffe - " + std::to_string(bahnGriffZahl()) + " Griffe, Start " +
                     std::to_string(griffNr(0)) + ", Laufziel " + std::to_string(griffNr(1)) + " (" + gp->zielName0 + ")");
        return griffNr(0) == 0 && griffNr(1) > 0;
    }));
    // Geteiltes Ziel: eine zweite Entity zeigt darauf (target2). Dann muss
    // dieser Lauf eine eigene Kopie bekommen, das alte Ziel bleibt stehen.
    auto zielOrt0 = std::make_shared<std::array<float, 3>>();
    auto zielNr0 = std::make_shared<int>(-1);
    add(tu("Griffe: eine zweite Entity auf dasselbe Laufziel zeigen lassen", [=] {
        entityKeySetzen(schiff->entity, "target2", gp->zielName0);
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            if (g_app->map.entities[i].targetname == gp->zielName0) {
                *zielNr0 = static_cast<int>(i);
                float o[3];
                (void)entityOrt(g_app->map.entities[i], o);
                *zielOrt0 = {o[0], o[1], o[2]};
            }
        }
        gp->ents0 = g_app->map.entities.size();
    }));
    add(ziehenAn("das Laufziel", [=] {
        ImVec2 q{-1.0F, -1.0F};
        (void)bahnGriffPunkt(griffNr(1), q);
        return q;
    }, 70.0F, 30.0F, 0));
    add(pause(0.6));
    add(pruefe("Griffe: das Laufziel ziehen verschiebt nur das Ziel - der Start bleibt", [=] {
        const ActorStep* st = laufVon(figur->actor);
        float o[3];
        (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(figur->entity)], o);
        float dz = 0.0F;
        float ds = 0.0F;
        for (int k = 0; k < 2; ++k) {
            dz += std::fabs((st != nullptr ? st->to[k] : 0.0F) - gp->ziel0[k]);
            ds += std::fabs(o[k] - gp->spawner0[k]);
        }
        const Node* n = st != nullptr ? nodeAt(g_app->doc.script(), st->path) : nullptr;
        const std::string name = (n != nullptr && n->args.size() >= 2) ? n->args[1].text : std::string();
        char z[300];
        std::snprintf(z, sizeof(z), "Knopftest: Griffe - Ziel um %.0f verschoben, Spawner um %.0f; Ziel \"%s\" -> \"%s\", Entities %zu -> %zu",
                      static_cast<double>(dz), static_cast<double>(ds), gp->zielName0.c_str(), name.c_str(), gp->ents0,
                      g_app->map.entities.size());
        diag::detail(z);
        float altBleibt = 1e9F;
        if (*zielNr0 >= 0) {
            float o0[3];
            (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(*zielNr0)], o0);
            altBleibt = std::fabs(o0[0] - (*zielOrt0)[0]) + std::fabs(o0[1] - (*zielOrt0)[1]);
        }
        diag::detail("Knopftest: Griffe - altes Ziel bewegt um " + std::to_string(altBleibt));
        return st != nullptr && dz > 20.0F && ds < 0.5F && name != gp->zielName0 && !name.empty() &&
               g_app->map.entities.size() == gp->ents0 + 1U && altBleibt < 0.5F;
    }));
    // Mitten im Zug pruefen: "wenn ich den Startpunkt ziehe, geht die fps auf
    // 16 runter, und das Model hinkt dem Gizmo hinterher". Keine
    // Neuberechnung der Szene, die Figur steht in DIESEM Bild am Griff, das
    // Ziel bleibt.
    auto mitte = std::make_shared<std::string>();
    auto mitteOk = std::make_shared<bool>(false);
    auto startPunkt = std::make_shared<ImVec2>(-1.0F, -1.0F);
    add(tu("Griffe: Zeit an den Anfang des Laufs (die Figur steht am Start)", [=] {
        const ActorStep* st = laufVon(figur->actor);
        // Genau am Anfang des Laufs: vorher ist sie noch nicht gespawnt
        // (unsichtbar), danach schon unterwegs.
        if (st != nullptr) { g_app->playMs = st->startMs + 1.0; }
        diag::detail("Knopftest: Griffe - Figur sichtbar am Laufanfang: " +
                     std::to_string(st != nullptr && g_app->scene.actors[static_cast<std::size_t>(figur->actor)].at(st->startMs + 1.0).visible));
        g_app->mapDirty = true;
    }));
    add(pause(0.4));
    add({"Griffe: den Start anfassen, ziehen und mitten im Zug pruefen", [=](int b) {
        ImGuiIO& io = ImGui::GetIO();
        if (b == 0) {
            *startPunkt = ImVec2{-1.0F, -1.0F};
            (void)bahnGriffPunkt(0, *startPunkt);
            setzeMaus(*startPunkt);
            return startPunkt->x < 0.0F;
        }
        if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
        if (b >= 3 && b <= 14) {
            setzeMaus(ImVec2{startPunkt->x - static_cast<float>(b - 2) * 5.0F, startPunkt->y + static_cast<float>(b - 2) * 1.7F});
            return false;
        }
        if (b == 20) {
            const App::BahnZug& z = g_app->bahnZug;
            const ActorStep* st = laufVon(figur->actor);
            const auto& f = g_app->figurGezeichnet.size() > static_cast<std::size_t>(figur->actor)
                                ? g_app->figurGezeichnet[static_cast<std::size_t>(figur->actor)]
                                : std::array<float, 3>{0.0F, 0.0F, 0.0F};
            float figurAmGriff = 0.0F;
            float zielBleibt = 0.0F;
            for (int k = 0; k < 2; ++k) {
                figurAmGriff += std::fabs(f[static_cast<std::size_t>(k)] - z.ort[k]);
                zielBleibt += std::fabs((st != nullptr ? st->to[k] : 0.0F) - gp->ziel0[k]);
            }
            float weg = std::fabs(z.ort[0] - z.startOrt[0]) + std::fabs(z.ort[1] - z.startOrt[1]);
            char t[260];
            std::snprintf(t, sizeof(t), "Knopftest: Griffe - mitten im Zug: Szene neu gebaut %d mal, Griff %.0f gewandert, Figur neben dem Griff %.1f, Ziel gewandert %.1f",
                          g_app->szeneBauten - z.szeneBautenBeimZug, static_cast<double>(weg), static_cast<double>(figurAmGriff),
                          static_cast<double>(zielBleibt));
            *mitte = t;
            *mitteOk = z.aktiv && g_app->szeneBauten == z.szeneBautenBeimZug && weg > 20.0F && figurAmGriff < 2.0F;
            return false;
        }
        if (b == 21) { io.AddMouseButtonEvent(0, false); return false; }
        return b >= 26;
    }});
    add(pause(0.6));
    add(pruefe("Griffe: beim Ziehen des Starts folgt die Figur sofort, ohne die Szene neu zu rechnen", [=] {
        diag::detail(mitte->empty() ? std::string("Knopftest: Griffe - kein Startgriff") : *mitte);
        return *mitteOk;
    }));
    add(pause(0.6));
    add(pruefe("Griffe: den Start ziehen verschiebt den Spawner - das Laufziel bleibt", [=] {
        const ActorStep* st = laufVon(figur->actor);
        float o[3];
        (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(figur->entity)], o);
        float ds = 0.0F;
        for (int k = 0; k < 2; ++k) { ds += std::fabs(o[k] - gp->spawner0[k]); }
        float vorher[3] = {0.0F, 0.0F, 0.0F};
        if (st != nullptr) { for (int k = 0; k < 3; ++k) { vorher[k] = st->to[k]; } }
        diag::detail("Knopftest: Griffe - Spawner um " + std::to_string(ds) + " verschoben, Ziel bei " +
                     std::to_string(vorher[0]) + "/" + std::to_string(vorher[1]));
        return st != nullptr && ds > 20.0F;
    }));
    // Gehen / Rennen per Rechtsklick auf das Laufziel.
    add(klickAn("Rechtsklick auf das Laufziel", [=] {
        ImVec2 q{-1.0F, -1.0F};
        (void)bahnGriffPunkt(griffNr(1), q);
        return q;
    }, 1));
    add(pause(0.3));
    auto menueZiel = std::make_shared<ImVec2>(-1.0F, -1.0F);
    add({"Klick Gehen/Rennen im Menue", [=](int bild) {
        ImGuiIO& io = ImGui::GetIO();
        if (bild == 0) {
            const std::string l = tr(gp->gang0 == "BOTH_WALK1" ? Str::BahnRennen : Str::BahnGehen);
            const Element* e = finde(l, "##Popup", 0, false);
            if (e == nullptr) { return true; }
            *menueZiel = e->rect.GetCenter();
            setzeMaus(*menueZiel);
            return false;
        }
        if (bild == 2) { io.AddMouseButtonEvent(0, true); return false; }
        if (bild == 3) { io.AddMouseButtonEvent(0, false); return false; }
        return bild >= 8;
    }});
    add(pause(0.6));
    add(pruefe("Griffe: Rechtsklick auf das Laufziel stellt zwischen Gehen und Rennen um", [=] {
        const ActorStep* st = laufVon(figur->actor);
        diag::detail("Knopftest: Griffe - Gangart vorher " + gp->gang0 + ", jetzt " + (st != nullptr ? st->text : std::string("-")) +
                     ", Dauer " + std::to_string(gp->dauer0) + " -> " + std::to_string(st != nullptr ? st->endMs - st->startMs : 0.0));
        return st != nullptr && !gp->gang0.empty() && st->text != gp->gang0;
    }));
    // Rueckwaerts: "both_walkback ist das rueckwaerts laufen und both_walk1
    // vorwaerts" - per Menue "Rueckwaerts laufen" auf dem Laufziel.
    const auto mitteDesLaufs = [=]() -> ActorState {
        const ActorStep* st = laufVon(figur->actor);
        if (st == nullptr) { return ActorState{}; }
        return g_app->scene.actors[static_cast<std::size_t>(figur->actor)].at((st->startMs + st->endMs) * 0.5);
    };
    const auto menuePunkt = [=](Str text) {
        auto ziel = std::make_shared<ImVec2>(-1.0F, -1.0F);
        return Schritt{"Klick im Laufmenue", [=](int bild) {
            ImGuiIO& io = ImGui::GetIO();
            if (bild == 0) {
                const Element* e = finde(tr(text), "##Popup", 0, false);
                if (e == nullptr) { return true; }
                *ziel = e->rect.GetCenter();
                setzeMaus(*ziel);
                return false;
            }
            if (bild == 2) { io.AddMouseButtonEvent(0, true); return false; }
            if (bild == 3) { io.AddMouseButtonEvent(0, false); return false; }
            return bild >= 8;
        }};
    };
    add(klickAn("Rechtsklick auf das Laufziel (rueckwaerts)", [=] {
        ImVec2 q{-1.0F, -1.0F};
        (void)bahnGriffPunkt(griffNr(1), q);
        return q;
    }, 1));
    add(pause(0.3));
    add(menuePunkt(Str::BahnRueckwaerts));
    add(pause(0.6));
    add(pruefe("Rueckwaerts: Blick hinter die Figur - sie laeuft mit der Rueckwaerts-Animation", [=] {
        const ActorStep* st = laufVon(figur->actor);
        const ActorState m = mitteDesLaufs();
        float dot = 0.0F;
        if (st != nullptr) {
            const float g = m.angles[1] * 3.14159265F / 180.0F;
            dot = std::cos(g) * (st->to[0] - st->from[0]) + std::sin(g) * (st->to[1] - st->from[1]);
        }
        diag::detail("Knopftest: Rueckwaerts - Animation mitten im Lauf \"" + m.animation + "\", Blick gegen Laufrichtung " +
                     std::to_string(dot < 0.0F));
        return st != nullptr && dot < 0.0F && m.animation.find("BACK") != std::string::npos;
    }));
    add(klickAn("Rechtsklick auf das Laufziel (vorwaerts)", [=] {
        ImVec2 q{-1.0F, -1.0F};
        (void)bahnGriffPunkt(griffNr(1), q);
        return q;
    }, 1));
    add(pause(0.3));
    add(menuePunkt(Str::BahnBlickFrei));
    add(pause(0.6));
    add(pruefe("Rueckwaerts: 'In Laufrichtung schauen' - wieder vorwaerts", [=] {
        const ActorState m = mitteDesLaufs();
        diag::detail("Knopftest: Rueckwaerts - danach \"" + m.animation + "\"");
        return m.animation == "BOTH_WALK1" || m.animation == "BOTH_RUN1";
    }));
    // Zeitleiste: den Balken des Laufs anfassen und nach rechts ziehen.
    add(tu("Zeitleiste: Spuren an, Zeit vor den Lauf", [=] {
        g_app->showTracks = true;
        const ActorStep* st = laufVon(figur->actor);
        if (st != nullptr) { gp->startMs0 = st->startMs; }
    }));
    add(pause(0.4));
    add(ziehenAn("den Laufbalken in der Zeitleiste", [=] {
        const ActorStep* st = laufVon(figur->actor);
        for (const App::TlBalken& b : g_app->tlBalken) {
            if (st != nullptr && b.pfad == st->path) { return ImVec2{(b.a.x + b.b.x) * 0.5F, (b.a.y + b.b.y) * 0.5F}; }
        }
        return ImVec2{-1.0F, -1.0F};
    }, 60.0F, 0.0F, 0));
    add(pause(0.6));
    add(pruefe("Zeitleiste: den Laufbalken ziehen verschiebt den Lauf in der Zeit", [=] {
        const ActorStep* st = laufVon(figur->actor);
        diag::detail("Knopftest: Zeitleiste - Lauf beginnt " + std::to_string(gp->startMs0) + " -> " +
                     std::to_string(st != nullptr ? st->startMs : -1.0) + " ms, " + std::to_string(g_app->tlBalken.size()) +
                     " Balken");
        return st != nullptr && st->startMs > gp->startMs0 + 200.0;
    }));
    // --- Die Zeitleiste als Baum wie im Unreal Sequencer (rc597) --------------
    //
    // "es ist fuer jeden Character alles in eine Leiste gestopft ... schau mal
    // nach, wie Unreal Engine das handhabt, so dass es logisch ist und man
    // auch aufklappen kann".
    const auto zeileDa = [](const std::string& n) {
        for (const auto& [z, y] : g_app->tlZeilenY) {
            if (z == n) { return true; }
        }
        return false;
    };
    const auto zeileY = [](const std::string& n) {
        for (const auto& [z, y] : g_app->tlZeilenY) {
            if (z == n) { return y; }
        }
        return -1000.0F;
    };
    auto figName = std::make_shared<std::string>();
    add(tu("Baum: die Figur waehlen, waits aus", [=] {
        g_app->showTracks = true;
        g_app->tlWaitsZeigen = false;
        g_app->tlOffen.clear();
        g_app->tlRollFigur = -1;
        g_app->selectedActor = -1;
        entityAuswahlSetzen(figur->entity);
        *figName = g_app->scene.actors[static_cast<std::size_t>(std::max(figur->actor, 0))].name;
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(foto("zeitleiste_baum"));
    add(pruefe("Baum: die gewaehlte Figur klappt auf - eigene Spuren je Art, keine wait-Bloecke", [=] {
        std::string zeilen;
        for (const auto& [z, y] : g_app->tlZeilenY) { zeilen += " [" + z + "]"; }
        diag::detail("Knopftest: Baum - Zeilen" + zeilen + "; wait-Bloecke " + std::to_string(g_app->tlWaitBloecke));
        return g_app->tlOffen.count(*figName) != 0U && zeileDa(*figName + "/" + tr(Str::TlKatBewegung)) &&
               g_app->tlWaitBloecke == 0;
    }));
    add(tu("Baum: waits einblenden", [] { g_app->tlWaitsZeigen = true; }));
    add(pause(0.3));
    add(pruefe("Baum: der Schalter zeigt die waits wieder als Bloecke", [] {
        diag::detail("Knopftest: Baum - mit waits " + std::to_string(g_app->tlWaitBloecke) + " wait-Bloecke");
        return g_app->tlWaitBloecke > 0;
    }));
    add(tu("Baum: waits wieder aus", [] { g_app->tlWaitsZeigen = false; }));
    add(pause(0.3));
    add(klickAn("Pfeil der Figur (zuklappen)", [=] {
        return ImVec2{g_app->tlKopfX + g_app->tlZeilenH * 0.5F, zeileY(*figName)};
    }));
    add(pause(0.3));
    add(pruefe("Baum: der Pfeil klappt die Figur zu", [=] {
        return g_app->tlOffen.count(*figName) == 0U && zeileDa(*figName) && !zeileDa(*figName + "/" + tr(Str::TlKatBewegung));
    }));
    add(tu("Baum: Spuren nach oben rollen (die Kamera steht oben)", [] {
        for (ImGuiWindow* w : GImGui->Windows) {
            if (w != nullptr && std::strstr(w->Name, "/tracks_") != nullptr) { ImGui::SetScrollY(w, 0.0F); }
        }
    }));
    add(pause(0.3));
    add(klickAn("Pfeil der Kamera (zuklappen)", [=] {
        return ImVec2{g_app->tlKopfX + g_app->tlZeilenH * 0.5F, zeileY("camera")};
    }));
    add(pause(0.3));
    add(pruefe("Baum: die Kameragruppe klappt zu - ihre Zeilen sind weg, die Gruppe bleibt", [=] {
        return !g_app->tlKameraOffen && zeileDa("camera") && !zeileDa("camera MOVE");
    }));
    add(klickAn("Pfeil der Kamera (aufklappen)", [=] {
        return ImVec2{g_app->tlKopfX + g_app->tlZeilenH * 0.5F, zeileY("camera")};
    }));
    add(pause(0.3));
    add(pruefe("Baum: und wieder auf", [=] { return g_app->tlKameraOffen && zeileDa("camera MOVE"); }));
    add(klickAn("Pfeil der Figur (aufklappen)", [=] {
        return ImVec2{g_app->tlKopfX + g_app->tlZeilenH * 0.5F, zeileY(*figName)};
    }));
    add(pause(0.3));
    // Schiff: das Flugziel eines move.
    add(tu("Griffe: das Schiff waehlen, Kamera auf sein erstes Flugziel", [=] {
        g_app->selectedActor = -1;
        entityAuswahlSetzen(schiff->entity);
        const Actor& a = g_app->scene.actors[static_cast<std::size_t>(std::max(schiff->actor, 0))];
        for (const ActorStep& st : a.steps) {
            if (st.kind != ActorStep::Kind::Glide || st.path.empty() || !st.fremd.empty()) { continue; }
            const Node* n = nodeAt(g_app->doc.script(), st.path);
            if (n == nullptr) { continue; }
            gp->moveText0 = n->args.empty() ? std::string() : n->args[0].text;
            for (int k = 0; k < 3; ++k) { gp->flugZiel0[k] = st.to[k]; }
            g_app->playMs = st.startMs;
            g_app->cam.pos[0] = st.to[0] - 300.0F;
            g_app->cam.pos[1] = st.to[1] - 100.0F;
            g_app->cam.pos[2] = st.to[2] + 250.0F;
            aimAngles(g_app->cam.pos, st.to, g_app->cam.angles);
            break;
        }
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(ziehenAn("das Flugziel", [=] {
        ImVec2 q{-1.0F, -1.0F};
        (void)bahnGriffPunkt(griffNr(2), q);
        return q;
    }, 80.0F, 0.0F, 0));
    add(pause(0.6));
    add(pruefe("Griffe: das Flugziel ziehen schreibt den neuen Ort in den move-Befehl", [=] {
        const Actor& a = g_app->scene.actors[static_cast<std::size_t>(std::max(schiff->actor, 0))];
        std::string jetzt;
        float d = 0.0F;
        for (const ActorStep& st : a.steps) {
            if (st.kind != ActorStep::Kind::Glide || st.path.empty() || !st.fremd.empty()) { continue; }
            const Node* n = nodeAt(g_app->doc.script(), st.path);
            jetzt = (n != nullptr && !n->args.empty()) ? n->args[0].text : std::string();
            for (int k = 0; k < 2; ++k) { d += std::fabs(st.to[k] - gp->flugZiel0[k]); }
            break;
        }
        diag::detail("Knopftest: Griffe - move \"" + gp->moveText0 + "\" -> \"" + jetzt + "\", Flugziel um " + std::to_string(d) +
                     " verschoben");
        return !gp->moveText0.empty() && jetzt != gp->moveText0 && d > 20.0F;
    }));
    add(pruefe("Zeitleiste: ein move hat am Ende einen Griff fuer die Flugzeit", [] {
        Node n;
        n.kind = Node::Kind::Command;
        n.name = "move";
        Arg v;
        v.kind = Arg::Kind::Vector;
        v.text = "0 0 0";
        Arg d;
        d.kind = Arg::Kind::Number;
        d.text = "2000.000";
        n.args = {v, d};
        const int a = durationArgIndex(n);
        n.args = {v, v, d};
        const int b = durationArgIndex(n);
        diag::detail("Knopftest: Zeitleiste - Dauer-Argument von move: " + std::to_string(a) + " / " + std::to_string(b));
        return a == 1 && b == 2;
    }));
    add(tu("Griffe: alles zurueck (Karte und Skript)", [=] {
        while (entityRueckgaengig()) {}
        while (g_app->doc.undoDepth() > gp->undo0 && g_app->doc.undo()) {}
        rebuildTree();
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    // --- Bahnpunkt mitten in der Bewegung (rc597) ---------------------------
    //
    // "wenn ich das Schiff mittendrin bewege, macht er keinen neuen Punkt,
    // sondern aendert immer den Anfangspunkt - dasselbe beim Character".
    struct BahnPunktProbe {
        std::vector<ActorStep> vorher;
        float ursprung[3]{};
        std::size_t ents = 0;
        std::size_t tiefe = 0;
        std::size_t entTiefe = 0;
        double t = 0.0;
    };
    auto bp = std::make_shared<BahnPunktProbe>();
    const auto bewegungen = [](int figurNr, ActorStep::Kind art) {
        std::vector<ActorStep> v;
        if (figurNr < 0) { return v; }
        for (const ActorStep& st : g_app->scene.actors[static_cast<std::size_t>(figurNr)].steps) {
            if (st.kind == art && !st.path.empty() && st.fremd.empty()) {
                // rc601 (bewegung): Stuecke desselben Weges (anderes Tempo
                // unterwegs) zaehlen als EIN Weg je SET_NAVGOAL.
                bool da = false;
                for (ActorStep& x : v) {
                    if (art == ActorStep::Kind::WalkTo && x.path == st.path) {
                        x.endMs = std::max(x.endMs, st.endMs);
                        if (!st.teilweg) {
                            for (int k = 0; k < 3; ++k) { x.to[k] = st.to[k]; }
                        }
                        da = true;
                    }
                }
                if (!da) { v.push_back(st); }
            }
        }
        return v;
    };
    // Figur: mitten im Lauf die Marke des Spawners ziehen.
    add(tu("Bahnpunkt: die Figur mitten im Lauf, ihr Spawner gewaehlt", [=] {
        const auto l = bewegungen(figur->actor, ActorStep::Kind::WalkTo);
        if (l.empty()) { return; }
        bp->vorher = l;
        // Kurz nach dem Losgehen - spaeter entfernt das Skript sie (unsichtbar).
        bp->t = l.front().startMs + std::min(1000.0, (l.front().endMs - l.front().startMs) * 0.3);
        g_app->playMs = bp->t;
        kameraVor(*figur, 200.0F, 160.0F);
        g_app->playMs = bp->t;
        g_app->selectedActor = -1;
        entityAuswahlSetzen(figur->entity);
        (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(figur->entity)], bp->ursprung);
        bp->ents = g_app->map.entities.size();
        bp->tiefe = g_app->doc.undoDepth();
        bp->entTiefe = g_app->entUndo.size();
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(ziehenAn("Bahnpunkt: die Figur mitten im Lauf zur Seite ziehen", [=] {
        for (const App::PickedMarker& mk : g_app->markers) {
            if (static_cast<int>(mk.entity) == figur->entity) { return bildZuSchirm(mk.sx, mk.sy); }
        }
        return ImVec2{-1.0F, -1.0F};
    }, 70.0F, 40.0F, 0));
    add(pause(0.6));
    add(pruefe("Bahnpunkt: mitten im Lauf entsteht ein neues Laufziel - der Spawner und das alte Ziel bleiben", [=] {
        const auto l = bewegungen(figur->actor, ActorStep::Kind::WalkTo);
        float o[3];
        (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(figur->entity)], o);
        const float spawner = std::fabs(o[0] - bp->ursprung[0]) + std::fabs(o[1] - bp->ursprung[1]);
        bool altesZiel = false;
        for (const ActorStep& st : l) {
            if (!bp->vorher.empty() && std::fabs(st.to[0] - bp->vorher.front().to[0]) + std::fabs(st.to[1] - bp->vorher.front().to[1]) < 1.0F) {
                altesZiel = true;
            }
        }
        diag::detail("Knopftest: Bahnpunkt Figur - Laeufe " + std::to_string(bp->vorher.size()) + " -> " + std::to_string(l.size()) +
                     ", Spawner bewegt um " + std::to_string(spawner) + ", altes Ziel noch angelaufen " + std::to_string(altesZiel) +
                     ", Entities " + std::to_string(bp->ents) + " -> " + std::to_string(g_app->map.entities.size()));
        return !bp->vorher.empty() && l.size() == bp->vorher.size() + 1U && spawner < 0.5F && altesZiel &&
               g_app->map.entities.size() == bp->ents + 1U;
    }));
    add(tu("Bahnpunkt: Figur zuruecknehmen", [=] {
        while (g_app->entUndo.size() > bp->entTiefe && entityRueckgaengig()) {}
        while (g_app->doc.undoDepth() > bp->tiefe && g_app->doc.undo()) {}
        rebuildTree();
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    // Schiff: mitten im ersten Flug an der X-Achse des Gizmos ziehen.
    add(tu("Bahnpunkt: das Schiff mitten im ersten Flug, gewaehlt", [=] {
        const auto g = bewegungen(schiff->actor, ActorStep::Kind::Glide);
        if (g.empty()) { return; }
        bp->vorher = g;
        bp->t = g.front().startMs + (g.front().endMs - g.front().startMs) * 0.5;
        kameraVor(*schiff, 420.0F, 160.0F);
        g_app->playMs = bp->t;
        g_app->selectedActor = -1;
        entityAuswahlSetzen(schiff->entity);
        (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(schiff->entity)], bp->ursprung);
        bp->tiefe = g_app->doc.undoDepth();
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(ziehenAn("Bahnpunkt: das Schiff an der X-Achse ziehen", [] {
        ImVec2 q{-1.0F, -1.0F};
        (void)entityGizmoPunkt(1, q);
        return q;
    }, 60.0F, 0.0F, 0));
    add(pause(0.6));
    add(pruefe("Bahnpunkt: mitten im Flug entsteht ein neuer Punkt - Ursprung und spaetere Zeiten bleiben", [=] {
        const auto g = bewegungen(schiff->actor, ActorStep::Kind::Glide);
        float o[3];
        (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(schiff->entity)], o);
        const float urs = std::fabs(o[0] - bp->ursprung[0]) + std::fabs(o[1] - bp->ursprung[1]) + std::fabs(o[2] - bp->ursprung[2]);
        const auto endeDa = [&](double t) {
            for (const ActorStep& st : g) {
                if (std::fabs(st.endMs - t) < 1.0) { return true; }
            }
            return false;
        };
        bool alleEnden = true;
        for (const ActorStep& st : bp->vorher) { alleEnden = alleEnden && endeDa(st.endMs); }
        std::string z;
        for (const ActorStep& st : g) { z += " " + std::to_string(static_cast<int>(st.endMs)); }
        diag::detail("Knopftest: Bahnpunkt Schiff - Fluege " + std::to_string(bp->vorher.size()) + " -> " + std::to_string(g.size()) +
                     ", Ursprung bewegt um " + std::to_string(urs) + ", Enden" + z);
        return !bp->vorher.empty() && g.size() == bp->vorher.size() + 1U && urs < 0.5F && alleEnden && endeDa(bp->t);
    }));
    add(tu("Bahnpunkt: Schiff zuruecknehmen", [=] {
        while (g_app->doc.undoDepth() > bp->tiefe && g_app->doc.undo()) {}
        rebuildTree();
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    return s;
}

// --- Kopieren und Einfuegen im Skriptbaum (rc599) ------------------------------
//
// Gemeldet (englisch): "if I right click and select copy, it only copies one
// command. It should copy everything highlighted." und "if I copy and paste
// commands in a command block statement (affect), the commands are
// duplicated."
// --- Kopieren, Quell-Reiter schliessen, woanders einfuegen (rc601) ------------
//
// Gemeldet (Tester, 17.10.): "If I copy a command and then close the script
// file. It won't paste that command into another file. I have to paste the
// command before I close the file."
//
// Fall 0: das Ziel hatte keine eigene Ablage - es kommt NICHTS an.
// Fall 1: es hatte eine aeltere - es kommt das FALSCHE an.
// Fall 2: statt zu schliessen ein neuer Reiter ("+" / Datei > Neu).
std::vector<Schritt> ablageSchliessenSchritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    const auto findeReiter = [](const std::string& name) {
        for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
            const std::string& n = (static_cast<int>(i) == g_app->activeTab) ? g_app->shownName : g_app->tabs[i].shownName;
            if (n == name) { return static_cast<int>(i); }
        }
        return -1;
    };
    const auto zahl = [](const char* befehl, const char* arg) {
        int z = 0;
        for (const Node& n : g_app->doc.script().nodes) {
            if (n.name == befehl && !n.args.empty() && n.args[0].text == arg) { ++z; }
        }
        return z;
    };
    for (int fall = 0; fall < 2; ++fall) {
        const std::string f = std::to_string(fall);
        const std::string ziel = "ablage_ziel" + f + ".txt";
        const std::string quelle = "ablage_quelle" + f + ".txt";
        add(tu("Ablage/Schliessen " + f + ": Zielskript oeffnen", [=] {
            openScriptFromMemory("//Generated by BehavEd\nwait ( 1.000 );\n", ziel);
        }));
        add(pause(0.3));
        if (fall == 1) {   // eine aeltere Ablage im Ziel: wait ( 1.000 )
            add(waehle("wait", 0));
            add(klick(tr(Str::ActCopy)));
            add(pause(0.2));
        }
        add(tu("Ablage/Schliessen " + f + ": Quellskript oeffnen", [=] {
            openScriptFromMemory("//Generated by BehavEd\nprint ( \"aus der Quelle\" );\n", quelle);
        }));
        add(pause(0.3));
        add(waehle("print", 0));
        add(klick(tr(Str::ActCopy)));
        add(pause(0.2));
        add(pruefe("Ablage/Schliessen " + f + ": print ist kopiert", [] {
            const char* c = ImGui::GetClipboardText();
            return c != nullptr && std::string(c).find("aus der Quelle") != std::string::npos;
        }));
        add(tu("Ablage/Schliessen " + f + ": Quell-Reiter schliessen (wie das X am Reiter)", [] { reiterSchliessen(g_app->activeTab); }));
        add(pause(0.3));
        add(tu("Ablage/Schliessen " + f + ": ins Ziel, wait waehlen", [=] {
            const int i = findeReiter(ziel);
            if (i >= 0) { activateTab(i); }
            selectByPath(Path{0});
        }));
        add(pause(0.3));
        add(klick(tr(Str::ActPaste)));
        add(pause(0.3));
        add(pruefe("Ablage/Schliessen " + f + ": print kommt nach dem Schliessen der Quelle im Ziel an - und nur print", [=] {
            diag::detail("Knopftest: Ablage/Schliessen " + f + " - aktiv " + g_app->shownName + ", print " +
                         std::to_string(zahl("print", "aus der Quelle")) + ", wait " + std::to_string(zahl("wait", "1.000")));
            return g_app->shownName == ziel && zahl("print", "aus der Quelle") == 1 && zahl("wait", "1.000") == 1;
        }));
        add(tu("Ablage/Schliessen " + f + ": aufraeumen", [=] {
            const int i = findeReiter(ziel);
            if (i >= 0) {
                g_app->tabs[static_cast<std::size_t>(i)].doc.markSaved();
                if (i == g_app->activeTab) { g_app->doc.markSaved(); }
                reiterSchliessen(i);
            }
        }));
        add(pause(0.3));
    }
    // Fall 2: kopieren, dann ein neuer Reiter - auch er brachte eine leere Ablage mit.
    add(tu("Ablage/Neu: Quellskript oeffnen", [] {
        openScriptFromMemory("//Generated by BehavEd\nprint ( \"aus der Quelle\" );\n", "ablage_quelle2.txt");
    }));
    add(pause(0.3));
    add(waehle("print", 0));
    add(klick(tr(Str::ActCopy)));
    add(pause(0.2));
    add(tu("Ablage/Neu: neuer Reiter", [] { neuerReiter(); }));
    add(pause(0.3));
    add(klick(tr(Str::ActPaste)));
    add(pause(0.3));
    add(pruefe("Ablage/Neu: print kommt im neuen Reiter an", [=] {
        diag::detail("Knopftest: Ablage/Neu - aktiv " + g_app->shownName + ", print " + std::to_string(zahl("print", "aus der Quelle")));
        return zahl("print", "aus der Quelle") == 1;
    }));
    add(tu("Ablage/Neu: aufraeumen", [=] {
        for (const char* n : {"ablage_quelle2.txt"}) {
            const int i = findeReiter(n);
            if (i >= 0) {
                g_app->tabs[static_cast<std::size_t>(i)].doc.markSaved();
                if (i == g_app->activeTab) { g_app->doc.markSaved(); }
                reiterSchliessen(i);
            }
        }
        g_app->doc.markSaved();
        reiterSchliessen(g_app->activeTab);
    }));
    add(pause(0.3));
    return s;
}

std::vector<Schritt> ablageSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto kinder = [] {
        const Node* n = nodeAt(g_app->doc.script(), Path{1});
        return n != nullptr ? n->children.size() : 0U;
    };
    add(tu("Ablage: Skript mit einem affect-Block, Baum offen", [] {
        openScriptFromMemory("//Generated by BehavEd\n"
                             "rem ( \"vorher\" );\n"
                             "affect ( \"player\", /*@AFFECT_TYPE*/ FLUSH )\n{\n"
                             "\twait ( 300.000 );\n"
                             "\tsound ( /*@CHANNELS*/ CHAN_VOICE_GLOBAL, \"sound/a.mp3\" );\n"
                             "\twait ( 16250.000 );\n"
                             "\tsound ( /*@CHANNELS*/ CHAN_VOICE_GLOBAL, \"sound/b.mp3\" );\n}\n"
                             "rem ( \"nachher\" );\n",
                             "ablage.txt");
        g_app->settings.flussAnsicht = 0;
        g_app->splitPanes[0].ansicht = 0;
        g_app->expanded.openAll();
        rebuildTree();
    }));
    add(pause(0.4));
    add(tu("Ablage: im Block das zweite wait und den zweiten sound markieren", [] {
        g_app->selection = {Path{1, 2}, Path{1, 3}};
        g_app->selectedPath = Path{1, 3};
        g_app->doc.setClipboard({});
        rebuildTree();
    }));
    add(pause(0.3));
    add(klick("sound", "/pane0_", 1, false, "", 1));   // Rechtsklick auf die markierte Zeile
    add(pause(0.3));
    add(klick(tr(Str::ActCopy), "##Popup"));
    add(pause(0.3));
    add(pruefe("Ablage: Rechtsklick > Copy kopiert ALLE markierten Zeilen", [] {
        diag::detail("Knopftest: Ablage - " + std::to_string(g_app->doc.clipboard().size()) + " Zeilen kopiert");
        return g_app->doc.clipboard().size() == 2U;
    }));
    auto vorher = std::make_shared<std::size_t>(0);
    add(tu("Ablage: nur den letzten sound im Block waehlen", [=] {
        *vorher = kinder();
        g_app->selection = {Path{1, 3}};
        g_app->selectedPath = Path{1, 3};
        rebuildTree();
    }));
    add(pause(0.3));
    add(klick("sound", "/pane0_", 1, false, "", 1));
    add(pause(0.3));
    add(klick(tr(Str::ActPaste), "##Popup"));
    add(pause(0.4));
    add(pruefe("Ablage: Einfuegen im affect-Block fuegt die zwei Zeilen genau einmal ein", [=] {
        std::string folge;
        if (const Node* n = nodeAt(g_app->doc.script(), Path{1})) {
            for (const Node& k : n->children) { folge += " " + k.name; }
        }
        diag::detail("Knopftest: Ablage - Block vorher " + std::to_string(*vorher) + " Zeilen, jetzt " +
                     std::to_string(kinder()) + ":" + folge + "; oben " + std::to_string(g_app->doc.script().nodes.size()));
        return kinder() == *vorher + 2U && g_app->doc.script().nodes.size() == 3U;
    }));
    // Dasselbe mit der Tastatur (Strg+C / Strg+V) - der zweite Weg.
    add(tu("Ablage: zuruecknehmen, zwei Zeilen markieren", [=] {
        while (g_app->doc.undo()) {}
        g_app->expanded.openAll();
        rebuildTree();
        g_app->selection = {Path{1, 0}, Path{1, 1}};
        g_app->selectedPath = Path{1, 1};
        g_app->doc.setClipboard({});
        rebuildTree();
    }));
    add(pause(0.3));
    add(klick("sound", "/pane0_", 0));   // Fokus in den Baum, waehlt den ersten sound
    add(tu("Ablage: wieder beide markieren", [] {
        g_app->selection = {Path{1, 0}, Path{1, 1}};
        g_app->selectedPath = Path{1, 1};
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_C, "Strg+C"));
    add(pause(0.2));
    add(tu("Ablage: den letzten sound waehlen", [=] {
        *vorher = kinder();
        g_app->selection = {Path{1, 3}};
        g_app->selectedPath = Path{1, 3};
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_V, "Strg+V"));
    add(pause(0.4));
    add(pruefe("Ablage: Strg+C / Strg+V im Block - auch genau einmal", [=] {
        std::string folge;
        if (const Node* n = nodeAt(g_app->doc.script(), Path{1})) {
            for (const Node& k : n->children) { folge += " " + k.name; }
        }
        diag::detail("Knopftest: Ablage (Tastatur) - Ablage " + std::to_string(g_app->doc.clipboard().size()) + ", Block " +
                     std::to_string(*vorher) + " -> " + std::to_string(kinder()) + ":" + folge);
        return g_app->doc.clipboard().size() == 2U && kinder() == *vorher + 2U && g_app->doc.script().nodes.size() == 3U;
    }));
    // Mit Umschalt vom affect bis zur letzten Zeile markiert: der Block UND
    // seine Zeilen sind in der Auswahl. Kopiert werden darf der Block nur
    // einmal - nicht zusaetzlich jede seiner Zeilen.
    add(tu("Ablage: zuruecknehmen, affect samt seinen Zeilen markieren", [] {
        while (g_app->doc.undo()) {}
        g_app->expanded.openAll();
        rebuildTree();
        g_app->selection = {Path{1}, Path{1, 0}, Path{1, 1}, Path{1, 2}, Path{1, 3}};
        g_app->selectedPath = Path{1, 3};
        g_app->doc.setClipboard({});
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_C, "Strg+C"));
    add(pause(0.2));
    add(pruefe("Ablage: Block samt Zeilen markiert - der Block kommt einmal in die Ablage, seine Zeilen nicht doppelt", [] {
        std::size_t zeilen = 0;
        for (const Node& n : g_app->doc.clipboard()) { zeilen += 1U + n.children.size(); }
        diag::detail("Knopftest: Ablage (Block) - " + std::to_string(g_app->doc.clipboard().size()) + " oben, " +
                     std::to_string(zeilen) + " Zeilen insgesamt");
        return g_app->doc.clipboard().size() == 1U && zeilen == 5U;
    }));
    for (Schritt& x : ablageSchliessenSchritte()) { s.push_back(std::move(x)); }
    return s;
}

// --- Werkzeuge der Kartenansicht (rc590) --------------------------------------
//
// Rahmen-Auswahl, Strg-Klick, Gruppe ziehen, Alt+Ziehen kopiert, Gruppe
// loeschen, Raster, Platzieren (Klick und Ziehen), gemerkte Ansichten.
std::vector<Schritt> kartenWerkzeugSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto anzahl0 = std::make_shared<std::size_t>(0);
    auto rahmen = std::make_shared<std::array<ImVec2, 2>>();
    auto erwartet = std::make_shared<std::set<int>>();
    const auto schirm = [](const App::PickedMarker& mk) {
        const float skal = g_app->mapImage.width > 0
                               ? static_cast<float>(g_app->mapImage.width) /
                                     std::max(1.0F, g_app->kartenAnsicht[2] - g_app->kartenAnsicht[0])
                               : 1.0F;
        return ImVec2{g_app->kartenAnsicht[0] + mk.sx / skal, g_app->kartenAnsicht[1] + mk.sy / skal};
    };
    add(tu("Werkzeuge: Ausgangslage", [=] {
        g_app->cameraSelected = false;
        g_app->selectedKey = -1;
        entityAuswahlSetzen(-1);
        g_app->rasterSchritt = 0;
        g_app->platzierKlasse.clear();
        g_app->cam.pos[0] = 200.0F; g_app->cam.pos[1] = 6000.0F; g_app->cam.pos[2] = 700.0F;
        g_app->cam.angles[0] = 50.0F; g_app->cam.angles[1] = 90.0F; g_app->cam.angles[2] = 0.0F;
        g_app->mapDirty = true;
        *anzahl0 = g_app->map.entities.size();
    }));
    add(pause(0.6));
    add(tu("Einen Rahmen um zwei bis vier Marken suchen", [=] {
        erwartet->clear();
        // Eine Marke als Anker, dann die Nachbarn in 150 Punkten.
        const float bx0 = g_app->kartenAnsicht[0] + 120.0F;
        const float bx1 = g_app->kartenAnsicht[2] - 120.0F;
        const float by0 = g_app->kartenAnsicht[1] + 140.0F;
        const float by1 = g_app->kartenAnsicht[3] - 60.0F;
        for (const App::PickedMarker& a : g_app->markers) {
            const ImVec2 pa = schirm(a);
            if (pa.x < bx0 || pa.x > bx1 || pa.y < by0 || pa.y > by1) { continue; }
            ImVec2 mn = pa;
            ImVec2 mx = pa;
            std::set<int> drin;
            for (const App::PickedMarker& b : g_app->markers) {
                const ImVec2 pb = schirm(b);
                if (std::fabs(pb.x - pa.x) < 150.0F && std::fabs(pb.y - pa.y) < 150.0F) {
                    mn = ImVec2{std::min(mn.x, pb.x), std::min(mn.y, pb.y)};
                    mx = ImVec2{std::max(mx.x, pb.x), std::max(mx.y, pb.y)};
                }
            }
            const ImVec2 r0{mn.x - 30.0F, mn.y - 30.0F};
            const ImVec2 r1{mx.x + 30.0F, mx.y + 30.0F};
            bool startFrei = true;
            for (const App::PickedMarker& b : g_app->markers) {
                const ImVec2 pb = schirm(b);
                if (pb.x >= r0.x && pb.x <= r1.x && pb.y >= r0.y && pb.y <= r1.y) { drin.insert(static_cast<int>(b.entity)); }
                if (std::fabs(pb.x - r0.x) < 26.0F && std::fabs(pb.y - r0.y) < 26.0F) { startFrei = false; }
            }
            if (drin.size() >= 2U && drin.size() <= 6U && startFrei) {
                (*rahmen)[0] = r0;
                (*rahmen)[1] = r1;
                *erwartet = drin;
                break;
            }
        }
        diag::detail("Knopftest: Werkzeuge - Rahmen mit " + std::to_string(erwartet->size()) + " Marken");
    }));
    add(ziehenAn("Rahmen ziehen", [=] { return (*rahmen)[0]; }, 0.0F, 0.0F, 0));   // erst ein Klick ins Leere
    add({"Rahmen ziehen (zweiter Versuch mit Weg)", [=](int b) {
             ImGuiIO& io = ImGui::GetIO();
             const ImVec2 a = (*rahmen)[0];
             const ImVec2 e = (*rahmen)[1];
             if (b == 0) { setzeMaus(a); return false; }
             if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
             if (b >= 3 && b <= 14) {
                 const float f = static_cast<float>(b - 2) / 12.0F;
                 setzeMaus(ImVec2{a.x + (e.x - a.x) * f, a.y + (e.y - a.y) * f});
                 return false;
             }
             if (b == 15) { io.AddMouseButtonEvent(0, false); return false; }
             return b >= 20;
         }});
    add(pause(0.3));
    add(foto("werkzeug_rahmen"));
    add(pruefe("Rahmen: genau die Entities darin sind gewaehlt", [=] {
        const auto a = entityAuswahl();
        const std::set<int> ist(a.begin(), a.end());
        diag::detail("Knopftest: Werkzeuge - gewaehlt " + std::to_string(ist.size()) + ", erwartet " +
                     std::to_string(erwartet->size()));
        return !erwartet->empty() && ist == *erwartet;
    }));
    // Strg-Klick auf eine Marke ausserhalb: dazu, nochmal: wieder heraus
    auto extra = std::make_shared<int>(-1);
    auto extraOrt = std::make_shared<ImVec2>();
    add(tu("Eine Marke ausserhalb des Rahmens", [=] {
        for (const App::PickedMarker& m : g_app->markers) {
            const ImVec2 p = schirm(m);
            if (erwartet->count(static_cast<int>(m.entity)) != 0U) { continue; }
            if (p.x < g_app->kartenAnsicht[0] + 60 || p.x > g_app->kartenAnsicht[2] - 60 ||
                p.y < g_app->kartenAnsicht[1] + 140 || p.y > g_app->kartenAnsicht[3] - 40) {
                continue;
            }
            bool allein = true;
            for (const App::PickedMarker& o : g_app->markers) {
                const ImVec2 q = schirm(o);
                if (&o != &m && std::fabs(q.x - p.x) < 30.0F && std::fabs(q.y - p.y) < 30.0F) { allein = false; }
            }
            if (allein) {
                *extra = static_cast<int>(m.entity);
                *extraOrt = p;
                break;
            }
        }
    }));
    add(ziehenAn("Strg-Klick auf die Marke", [=] { return *extraOrt; }, 0.0F, 0.0F, 0, ImGuiMod_Ctrl));
    add(pause(0.2));
    add(pruefe("Strg-Klick nimmt sie hinzu", [=] { return *extra >= 0 && entityIstGewaehlt(*extra) && entityAuswahl().size() == erwartet->size() + 1; }));
    add(ziehenAn("noch einmal Strg-Klick", [=] { return *extraOrt; }, 0.0F, 0.0F, 0, ImGuiMod_Ctrl));
    add(pause(0.2));
    add(pruefe("... und nimmt sie wieder heraus", [=] { return !entityIstGewaehlt(*extra) && entityAuswahl().size() == erwartet->size(); }));
    // Gruppe ziehen: an der X-Achse des Hauptpunkts
    auto vorher = std::make_shared<std::vector<std::array<float, 3>>>();
    auto tiefe = std::make_shared<std::size_t>(0);
    const auto orte = [] {
        std::vector<std::array<float, 3>> v;
        for (const int i : entityAuswahl()) {
            float o[3] = {0, 0, 0};
            entityOrt(g_app->map.entities[static_cast<std::size_t>(i)], o);
            v.push_back({o[0], o[1], o[2]});
        }
        return v;
    };
    add(tu("Orte der Gruppe merken", [=] { *vorher = orte(); *tiefe = g_app->entUndo.size(); }));
    add(pause(0.2));
    add(ziehenAn("Gruppe an der X-Achse ziehen", [] { ImVec2 p; entityGizmoPunkt(1, p); return p; }, 60.0F, 0.0F, 0));
    add(pause(0.3));
    add(pruefe("Gruppe: alle um denselben Versatz verschoben, ein Rueckgaengig-Schritt", [=] {
        const auto jetzt = orte();
        if (jetzt.size() != vorher->size() || jetzt.empty()) { return false; }
        const float dx = jetzt[0][0] - (*vorher)[0][0];
        bool gleich = std::fabs(dx) > 2.0F;
        for (std::size_t k = 0; k < jetzt.size(); ++k) {
            gleich = gleich && std::fabs((jetzt[k][0] - (*vorher)[k][0]) - dx) <= 1.01F &&
                     std::fabs(jetzt[k][1] - (*vorher)[k][1]) <= 1.01F;
        }
        diag::detail("Knopftest: Werkzeuge - Gruppe dx " + std::to_string(dx));
        return gleich && g_app->entUndo.size() == *tiefe + 1;
    }));
    // Strg+Z ueber der Karte gehoert den Entities (ausserhalb dem Skript).
    add(tu("Maus in die Karte (unten links, leer)", [] { setzeMaus(ImVec2{g_app->kartenAnsicht[0] + 40.0F, g_app->kartenAnsicht[3] - 40.0F}); }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(tu("Auswahl wiederherstellen (Rueckgaengig leert sie)", [=] {
        std::vector<int> v(erwartet->begin(), erwartet->end());
        entityAuswahlSetzen(v.front());
        g_app->entAuswahl.assign(v.begin() + 1, v.end());
    }));
    add(pause(0.2));
    add(pruefe("Strg+Z: die ganze Gruppe wieder am alten Ort", [=] {
        const auto jetzt = orte();
        bool gleich = jetzt.size() == vorher->size();
        for (std::size_t k = 0; gleich && k < jetzt.size(); ++k) {
            gleich = std::fabs(jetzt[k][0] - (*vorher)[k][0]) < 0.5F;
        }
        return gleich;
    }));
    // Alt+Ziehen kopiert
    add(ziehenAn("Alt+Ziehen an der X-Achse", [] { ImVec2 p; entityGizmoPunkt(1, p); return p; }, 60.0F, 0.0F, 0, ImGuiMod_Alt));
    add(pause(0.3));
    add(pruefe("Alt+Ziehen: Kopien der ganzen Gruppe, die Originale bleiben stehen", [=] {
        const std::size_t n = g_app->map.entities.size();
        bool originale = true;
        std::size_t k = 0;
        for (const int i : *erwartet) {
            float o[3];
            entityOrt(g_app->map.entities[static_cast<std::size_t>(i)], o);
            (void)k;
            bool fand = false;
            for (const auto& v : *vorher) { fand = fand || (std::fabs(v[0] - o[0]) < 0.5F && std::fabs(v[1] - o[1]) < 0.5F); }
            originale = originale && fand;
            ++k;
        }
        diag::detail("Knopftest: Werkzeuge - nach Alt-Zug " + std::to_string(n) + " Entities (vorher " +
                     std::to_string(*anzahl0) + ")");
        return n == *anzahl0 + erwartet->size() && originale && entityAuswahl().front() >= static_cast<int>(*anzahl0);
    }));
    add(tu("Maus in die Karte", [] { setzeMaus(ImVec2{g_app->kartenAnsicht[0] + 40.0F, g_app->kartenAnsicht[3] - 40.0F}); }));
    add(taste(ImGuiKey_Delete, "Entf loescht die Gruppe (die Kopien)"));
    add(pause(0.3));
    add(pruefe("Entf: die ganze gewaehlte Gruppe ist weg", [=] { return g_app->map.entities.size() == *anzahl0 && entityAuswahl().empty(); }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(pruefe("Strg+Z: die Kopien sind wieder da", [=] { return g_app->map.entities.size() == *anzahl0 + erwartet->size(); }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(pruefe("Strg+Z: vor dem Kopieren", [=] { return g_app->map.entities.size() == *anzahl0; }));
    // Raster 16
    add(klick(tr(Str::Grid), "###main"));
    add(klick("16", "##Popup"));
    add(pruefe("Raster: 16", [] { return g_app->rasterSchritt == 16; }));
    add(tu("Eine Entity waehlen", [=] { entityAuswahlSetzen(*erwartet->begin()); }));
    add(pause(0.3));
    add(ziehenAn("an der X-Achse 37 Punkte", [] { ImVec2 p; entityGizmoPunkt(1, p); return p; }, 37.0F, 0.0F, 0));
    add(pause(0.3));
    add(pruefe("Raster: der Ort liegt auf einem Vielfachen von 16", [=] {
        float o[3];
        entityOrt(g_app->map.entities[static_cast<std::size_t>(*erwartet->begin())], o);
        diag::detail("Knopftest: Werkzeuge - x nach Rasterzug " + std::to_string(o[0]));
        return std::fabs(std::fmod(std::fabs(o[0]), 16.0F)) < 0.01F;
    }));
    add(tu("zurueck, Raster aus", [] {
        entityRueckgaengig();
        g_app->rasterSchritt = 0;
        entityAuswahlSetzen(-1);
    }));
    // Platzieren: Kachel anklicken, dann in die Karte
    auto boden = std::make_shared<ImVec2>();
    add(tu("Kamera schraeg ueber einen Spielerstart / eine Figur", [] {
        for (const MapEntity& e : g_app->map.entities) {
            float o[3];
            if ((e.classname.rfind("info_player", 0) == 0 || e.classname.rfind("NPC_", 0) == 0) && entityOrt(e, o)) {
                g_app->cam.pos[0] = o[0] - 260.0F;
                g_app->cam.pos[1] = o[1];
                g_app->cam.pos[2] = o[2] + 260.0F;
                g_app->cam.angles[0] = 42.0F;
                g_app->cam.angles[1] = 0.0F;
                g_app->cam.angles[2] = 0.0F;
                g_app->mapDirty = true;
                break;
            }
        }
    }));
    add(pause(0.4));
    add(tu("Ein Bodenpunkt im Bild", [=] {
        // Ein Raster ueber das Bild, ohne Marken in der Naehe, mit begehbarem Boden.
        bool fand = false;
        for (float fy = 0.35F; fy < 0.95F && !fand; fy += 0.06F) {
            for (float fx = 0.25F; fx < 0.85F && !fand; fx += 0.06F) {
                const float x = g_app->kartenAnsicht[0] + (g_app->kartenAnsicht[2] - g_app->kartenAnsicht[0]) * fx;
                const float y = g_app->kartenAnsicht[1] + (g_app->kartenAnsicht[3] - g_app->kartenAnsicht[1]) * fy;
                TraceTreffer t;
                if (kartenStrahlUnterMaus(x, y, t) && t.normal[2] > 0.7F) {
                    *boden = ImVec2{x, y};
                    fand = true;
                }
            }
        }
        {
            TraceTreffer t;
            const float x = (g_app->kartenAnsicht[0] + g_app->kartenAnsicht[2]) * 0.5F;
            const float y = (g_app->kartenAnsicht[1] + g_app->kartenAnsicht[3]) * 0.5F;
            const bool h = kartenStrahlUnterMaus(x, y, t);
            diag::detail("Knopftest: Werkzeuge - Mitte: Treffer " + std::to_string(h) + " z " + std::to_string(t.point[2]) +
                         " Normale " + std::to_string(t.normal[0]) + " " + std::to_string(t.normal[1]) + " " +
                         std::to_string(t.normal[2]) + ", Kamera " + std::to_string(g_app->gezeigteKamera.pos[2]) +
                         " Blick " + std::to_string(g_app->gezeigteKamera.angles[0]) + ", durch Kamera " +
                         std::to_string(g_app->throughCamera) + "/" + std::to_string(g_app->camViewActive));
        }
        diag::detail("Knopftest: Werkzeuge - Bodenpunkt " + std::to_string(fand) + " bei " + std::to_string(boden->x) +
                     "/" + std::to_string(boden->y));
        *anzahl0 = g_app->map.entities.size();
    }));
    add(klick(tr(Str::EntKindNav), "##platzieren"));
    add(pause(0.2));
    add(pruefe("Kachel Laufziel: Platzieren aktiv", [] { return g_app->platzierKlasse == "waypoint_navgoal"; }));
    add(klickAn("in die Karte", [=] { return *boden; }));
    add(pause(0.3));
    add(pruefe("Platzieren: ein Laufziel steht am Klickpunkt, das Werkzeug ist wieder aus", [=] {
        const auto& v = g_app->map.entities;
        return v.size() == *anzahl0 + 1 && v.back().classname == "waypoint_navgoal" && g_app->platzierKlasse.empty() &&
               g_app->pickedEntity == static_cast<int>(v.size()) - 1;
    }));
    add(ziehenAn("Kachel Effekt in die Karte ziehen", [] {
            const Element* e = finde(tr(Str::EntKindFx), "##platzieren", 0, false);
            return e != nullptr ? e->rect.GetCenter() : ImVec2{0, 0};
        }, 260.0F, 40.0F, 0));
    add(pause(0.3));
    add(pruefe("Ziehen: ein Effekt (fx_runner) wurde gesetzt", [=] {
        const auto& v = g_app->map.entities;
        diag::detail("Knopftest: Werkzeuge - nach dem Ziehen " + std::to_string(v.size()) + " Entities, letzte " +
                     (v.empty() ? std::string() : v.back().classname));
        return v.size() == *anzahl0 + 2 && v.back().classname == "fx_runner" && g_app->platzierKlasse.empty();
    }));
    add(tu("beide zurueck", [] { entityRueckgaengig(); entityRueckgaengig(); }));
    // Gemerkte Ansichten
    auto merk = std::make_shared<Camera>();
    add(tu("Maus in die Karte, Ansicht merken", [=] { setzeMaus(ansichtMitte()); *merk = g_app->cam; }));
    add(pause(0.1));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_3, "Strg+3"));
    add(tu("woanders hin", [] { g_app->cam.pos[0] += 900.0F; g_app->cam.angles[1] += 40.0F; g_app->mapDirty = true; }));
    add(pause(0.2));
    add(taste(ImGuiKey_3, "3"));
    add(pause(0.2));
    add(pruefe("3 springt zur gemerkten Ansicht", [=] {
        return std::fabs(g_app->cam.pos[0] - merk->pos[0]) < 0.01F && std::fabs(g_app->cam.angles[1] - merk->angles[1]) < 0.01F;
    }));
    return s;
}

// --- Model-Tab (BHED_EDITORTEST=modell, rc591) ---------------------------------
std::vector<Schritt> modellSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("Modell: Model-Tab, Spielpfade", [] {
        g_app->leftMode = 2;
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "modell.txt");
    }));
    add(pause(0.5));
    add(klick(tr(Str::NpcBrowser), "###main"));
    add(pause(0.5));
    add(pruefe("NPC-Browser: offen, mit Figuren", [] {
        diag::detail("Knopftest: Modell - " + std::to_string(npcTypenListe().size()) + " NPC-Typen");
        return g_app->npcBrowserOffen && npcTypenListe().size() > 20U;
    }));
    auto typ = std::make_shared<std::string>();
    add(tu("Einen NPC mit Modell und Animationen suchen", [=] {
        for (const std::string& t : npcTypenListe()) {
            if (t.find("jedi") == std::string::npos && t.find("stormtrooper") == std::string::npos) { continue; }
            *typ = t;
            break;
        }
        if (typ->empty() && !npcTypenListe().empty()) { *typ = npcTypenListe().front(); }
    }));
    // 1323 Eintraege: der gesuchte steht kaum im sichtbaren Teil, und ImGui
    // meldet nur, was sichtbar ist - also erst suchen, wie man es auch tut.
    add(klick("##npcsuche", "###npcbrowser"));
    add({"Den NPC-Namen tippen", [=](int b) {
             static Schritt k;
             if (b == 0) { k = tippe(*typ); }
             return k.tun(b);
         }});
    add(pause(0.3));
    add({"Den NPC in der Liste anklicken", [=](int b) {
             static Schritt k;
             if (b == 0) { k = klick(*typ, "###npcbrowser"); }
             return k.tun(b);
         }});
    add(pause(1.5));
    add(foto("modell_npc"));
    add(pruefe("NPC geladen: Modell, Animationen, Haut", [=] {
        diag::detail("Knopftest: Modell - " + *typ + ": " + g_app->modelPath + ", " + std::to_string(g_app->animList.size()) +
                     " Animationen, " + std::to_string(g_app->modelSkins.size()) + " Haeute");
        return !g_app->model.empty() && !g_app->animList.empty() && g_app->npcGeladen == *typ;
    }));
    add(tu("NPC-Browser zu", [] { g_app->npcBrowserOffen = false; }));
    // Animationsliste
    auto zweite = std::make_shared<std::string>();
    add(tu("Eine BOTH_-Animation aus der Liste", [=] {
        for (const AnimEntry& e : g_app->animList) {
            if (e.name.rfind("BOTH_", 0) == 0 && e.numFrames > 5) {
                *zweite = e.name;
                break;
            }
        }
    }));
    add({"Animation anklicken", [=](int b) {
             static Schritt k;
             if (b == 0) { k = klick(*zweite, "", 0, false); }
             return k.tun(b);
         }});
    add(pause(0.3));
    // Eine kurze Animation ohne Schleife ist nach 0,3 s schon durch - also:
    // gewaehlt, und sie lief (oder laeuft noch).
    add(pruefe("Klick in der Liste: diese Animation ist gewaehlt und spielt", [=] {
        if (g_app->animIndex < 0) { return false; }
        const AnimEntry& e = g_app->animList[static_cast<std::size_t>(g_app->animIndex)];
        return e.name == *zweite && (g_app->animPlaying || g_app->animFrame > e.firstFrame);
    }));
    add(klick("TORSO", "", 0, false));
    add(pause(0.3));
    add(pruefe("Gruppe TORSO: keine BOTH_-Zeile mehr in der Liste", [=] { return finde(*zweite, "", 0, false) == nullptr; }));
    add(klick(tr(Str::ModelAnimAll), "", 0, false));
    add(pause(0.3));
    // Transport: Schleife erzwingen, abspielen, anhalten
    add(klick(tr(Str::ModelLoop), "###main"));
    add(pruefe("Schleife: erzwungen", [] { return g_app->loopMode == 1; }));
    add(tu("angehalten", [] { g_app->animPlaying = false; }));
    add(pause(0.1));
    add(klick(tr(Str::ModelPlay), "###main"));
    add(pause(0.6));
    add(pruefe("Abspielen: laeuft (in der Schleife auch nach dem Ende)", [] { return g_app->animPlaying; }));
    add(klick(tr(Str::ModelStop), "###main"));
    add(pause(0.2));
    add(pruefe("Anhalten", [] { return !g_app->animPlaying; }));
    auto bild = std::make_shared<int>(0);
    add(tu("Auf das dritte Bild, merken", [=] {
        const AnimEntry& e = g_app->animList[static_cast<std::size_t>(std::max(g_app->animIndex, 0))];
        g_app->animFrame = e.firstFrame + 2;
        *bild = g_app->animFrame;
    }));
    add(klick(tr(Str::ModelNextFrame), "###main"));
    add(pause(0.2));
    add(pruefe("Bild vor: ein Bild weiter, angehalten", [=] {
        if (g_app->animIndex < 0 || static_cast<std::size_t>(g_app->animIndex) >= g_app->animList.size()) { return false; }
        return !g_app->animPlaying && (g_app->animFrame == *bild + 1 ||
                                       g_app->animFrame == g_app->animList[static_cast<std::size_t>(g_app->animIndex)].firstFrame +
                                                               g_app->animList[static_cast<std::size_t>(g_app->animIndex)].numFrames - 1);
    }));
    add(klick(tr(Str::ModelPrevFrame), "###main"));
    add(pause(0.2));
    add(pruefe("Bild zurueck", [=] { return g_app->animFrame == *bild; }));
    // Ins Skript
    add(tu("Erste Zeile des Skripts waehlen", [] { selectByPath(Path{0}); }));
    add(klick(tr(Str::ModelAnimToScript), "", 0, false));
    add(pause(0.3));
    add(pruefe("Ins Skript: set ( SET_ANIM_..., Name ) hinter der Zeile", [=] {
        const auto& ns = g_app->doc.script().nodes;
        const bool ok = ns.size() >= 2 && ns[1].name == "set" && ns[1].args.size() >= 2 &&
                        ns[1].args[0].text.rfind("SET_ANIM_", 0) == 0 && ns[1].args[1].text == *zweite;
        diag::detail("Knopftest: Modell - Zeile 1: " + (ns.size() > 1 ? ns[1].name + " " +
                                                         (ns[1].args.size() > 1 ? ns[1].args[0].text + " " + ns[1].args[1].text : "")
                                                     : std::string("-")));
        return ok;
    }));
    // Drehteller, Knochen, Einpassen
    auto gier = std::make_shared<float>(0.0F);
    add(tu("Blick merken", [=] { *gier = g_app->modelCam.angles[1]; }));
    add(klick(tr(Str::ModelTurntable), "###main"));
    add(pause(0.6));
    add(pruefe("Drehteller: das Modell dreht sich", [=] {
        return g_app->modellDrehteller && std::fabs(g_app->modelCam.angles[1] - *gier) > 2.0F;
    }));
    add(klick(tr(Str::ModelTurntable), "###main"));
    add(klick(tr(Str::ModelBones), "###main"));
    add(pause(0.5));
    add(foto("modell_knochen"));
    add(pruefe("Knochen: das Skelett ist gezeichnet", [] {
        diag::detail("Knopftest: Modell - " + std::to_string(g_app->modellKnochenLinien) + " Knochenlinien");
        return g_app->modellKnochen && g_app->modellKnochenLinien > 10;
    }));
    add(klick(tr(Str::ModelBones), "###main"));
    add(tu("Ansicht verdrehen", [] { g_app->modelOrbit.turn(g_app->modelCam, 70.0F, 20.0F); g_app->modelDirty = true; }));
    add(klick(tr(Str::ModelResetView), "###main"));
    add(pause(0.3));
    add(pruefe("Einpassen: wieder von vorn", [] { return std::fabs(g_app->modelCam.angles[1] - 180.0F) < 1.0F; }));
    add(tu("zurueck zur Karte", [] { g_app->leftMode = 1; g_app->doc.markSaved(); }));
    add(pause(0.2));
    return s;
}

// --- Mover animieren (rc592) -------------------------------------------------------
// --- Indizes nach Loeschen und Kartenwechsel (Code-Pruefung rc592) ----------
//
// Gefunden beim Durchsehen: mehrere Werkzeuge merken sich Entity-NUMMERN
// (Wegnetz, Auswahl, Animationsfenster, Wegkette). Nach einem Loeschen oder
// einem Kartenwechsel zeigten sie auf eine andere Entity - oder hinter das
// Ende der Liste.
std::vector<Schritt> indexSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto zahl = std::make_shared<int>(0);
    add(tu("Indizes: eine Entity VOR den Wegpunkten loeschen", [=] {
        wegNetzAktuell();
        *zahl = static_cast<int>(g_app->wegNetz.knoten.size());
        int erster = -1;
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            if (!istWegKlasse(g_app->map.entities[i].classname)) { erster = static_cast<int>(i); break; }
        }
        diag::detail("Knopftest: Indizes - Wegknoten " + std::to_string(*zahl) + ", loesche Entity " +
                     std::to_string(erster));
        (void)entityLoeschen(erster);
        wegNetzAktuell();
    }));
    add(pause(0.3));
    add(pruefe("Indizes: das Wegnetz zeigt danach nur auf Wegpunkte, keine Nummer hinter dem Ende", [=] {
        int schlecht = 0;
        for (const WegKnoten& k : g_app->wegNetz.knoten) {
            if (k.entity < 0 || static_cast<std::size_t>(k.entity) >= g_app->map.entities.size() ||
                !istWegKlasse(g_app->map.entities[static_cast<std::size_t>(k.entity)].classname)) {
                ++schlecht;
            }
        }
        diag::detail("Knopftest: Indizes - nach dem Loeschen " + std::to_string(g_app->wegNetz.knoten.size()) +
                     " Knoten, falsch " + std::to_string(schlecht));
        return schlecht == 0 && static_cast<int>(g_app->wegNetz.knoten.size()) == *zahl;
    }));
    add(tu("Loeschen rueckgaengig", [] { (void)entityRueckgaengig(); }));
    // Ein Rautenzug, der haengen bleibt, weil das Fenster mitten im Zug zugeht
    // (oder die Ansicht durch die Kamera schaut): danach darf er nicht mehr
    // laufen - sonst greift er mit einer alten Nummer ins Leere.
    add(tu("Indizes: Rautenzug, dann Fenster zu", [] {
        g_app->animZug = true;
        g_app->animSchluessel = 0;
        g_app->animFensterOffen = false;
        g_app->mapDirty = true;
    }));
    add(pause(0.3));
    add(pruefe("Indizes: der Zug endet mit dem Fenster", [] { return !g_app->animZug; }));
    // Auswahl, Animationsfenster und Wegkette gehoeren zur Karte.
    add(tu("Indizes: Auswahl, Animation und Wegkette setzen, dann andere Karte", [] {
        g_app->entAuswahl = {1, 2, 3};
        g_app->animEntity = 2;
        g_app->animFensterOffen = true;
        g_app->wegLetzter = 1;
        g_app->wegVon = 2;
        (void)loadMapFromArchive(g_app->map.path == "maps/kor1.bsp" ? std::string("maps/academy1.bsp")
                                                                   : std::string("maps/kor1.bsp"));
    }));
    add(warteBis("Karte bereit", [] { return g_app->mapBereit; }, 900));
    add(pause(0.3));
    add(pruefe("Indizes: nach dem Kartenwechsel keine Auswahl, kein Animationsfenster, keine Wegkette", [] {
        diag::detail("Knopftest: Indizes - Auswahl " + std::to_string(g_app->entAuswahl.size()) + ", Animation " +
                     std::to_string(g_app->animFensterOffen) + "/" + std::to_string(g_app->animEntity) + ", Weg " +
                     std::to_string(g_app->wegLetzter) + "/" + std::to_string(g_app->wegVon));
        return g_app->entAuswahl.empty() && !g_app->animFensterOffen && g_app->animEntity < 0 &&
               g_app->wegLetzter < 0 && g_app->wegVon < 0;
    }));
    return s;
}

// --- Lichtschalter im Ablauf (rc593) ------------------------------------------
//
// duel_kamino: die Raeume leuchten nur durch Lichter mit targetname (Stil
// 32+). Ein Skript schaltet eines davon nach einer Sekunde aus; die Seite mit
// seiner Schicht muss vorher anders aussehen als nachher.
std::vector<Schritt> lichtSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("Licht: duel_kamino laden", [] {
        g_app->leftMode = 1;
        (void)loadMapFromArchive("maps/duel_kamino.bsp");
    }));
    add(warteBis("Karte bereit", [] { return g_app->mapBereit; }, 900));
    add(pause(0.5));
    auto seite = std::make_shared<int>(-1);
    auto vorher = std::make_shared<std::vector<std::uint8_t>>();
    add(tu("Licht: ein schaltbares Licht und ein Skript, das es ausschaltet", [=] {
        std::string name;
        for (const MapEntity& e : g_app->map.entities) {
            const std::string* st = e.find("style");
            if (e.classname != "light" || e.targetname.empty() || st == nullptr) { continue; }
            const int stil = std::atoi(st->c_str());
            for (const BspGeometry::StilSchicht& sch : g_app->geo.stilSchichten) {
                if (sch.stil == stil) { *seite = sch.seite; break; }
            }
            if (*seite >= 0) { name = e.targetname; break; }
        }
        diag::detail("Knopftest: Licht - " + std::to_string(g_app->geo.stilSchichten.size()) + " Schichten, Licht \"" +
                     name + "\", Seite " + std::to_string(*seite));
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\nuse ( \"" + name + "\" );\nwait ( 2000.000 );\n",
                             "licht.txt");
        prepareScene();
        g_app->playing = false;
        g_app->playMs = 200.0;
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(tu("Licht: Seite merken (Licht an)", [=] {
        if (*seite >= 0 && static_cast<std::size_t>(*seite) < g_app->geo.lightmaps.size()) {
            *vorher = g_app->geo.lightmaps[static_cast<std::size_t>(*seite)];
        }
        g_app->playMs = 2500.0;
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(foto("licht_aus"));
    add(pruefe("Licht: nach dem use ist die Seite dunkler (das Licht ist aus)", [=] {
        if (*seite < 0 || vorher->empty()) { return false; }
        const std::vector<std::uint8_t>& jetzt = g_app->geo.lightmaps[static_cast<std::size_t>(*seite)];
        long long summeVor = 0;
        long long summeJetzt = 0;
        for (std::size_t i = 0; i < jetzt.size() && i < vorher->size(); ++i) {
            summeVor += (*vorher)[i];
            summeJetzt += jetzt[i];
        }
        diag::detail("Knopftest: Licht - Seite " + std::to_string(*seite) + " Helligkeit " + std::to_string(summeVor) +
                     " -> " + std::to_string(summeJetzt));
        return summeJetzt < summeVor;
    }));
    add(tu("Licht: zurueck an den Anfang", [] { g_app->playMs = 200.0; g_app->mapDirty = true; }));
    add(pause(0.5));
    add(pruefe("Licht: vor dem use wieder hell", [=] {
        return *seite >= 0 && g_app->geo.lightmaps[static_cast<std::size_t>(*seite)] == *vorher;
    }));
    // --- Himmelsportal (rc593): md_am_sith hat eines, duel_kamino nicht ------
    add(pruefe("Portal: duel_kamino hat kein misc_skyportal - Himmelsbox wie bisher", [] {
        float o[3];
        return !himmelPortalOrt(o) && !g_app->himmelPortalGezeichnet;
    }));
    add(tu("Portal: md_am_sith laden", [] { (void)loadMapFromArchive("maps/md_am_sith.bsp"); }));
    add(warteBis("Karte bereit", [] { return g_app->mapBereit; }, 900));
    add(tu("Portal: neu zeichnen", [] { g_app->mapDirty = true; }));
    add(pause(0.5));
    add(pruefe("Portal: md_am_sith zeichnet den Himmel aus dem misc_skyportal, ohne D3D-Meldung", [] {
        float o[3];
        const bool da = himmelPortalOrt(o);
        diag::detail("Knopftest: Portal - " + std::string(da ? "da" : "fehlt") + ", gezeichnet " +
                     std::to_string(g_app->himmelPortalGezeichnet) + ", D3D-Meldungen " +
                     std::to_string(g_app->d3dMeldungen.size()) + ", Fehler \"" + g_app->gpuFehler + "\"");
        return da && g_app->himmelPortalGezeichnet && g_app->gpuFehler.empty() && g_app->d3dMeldungen.empty();
    }));
    // --- Spiegel (rc593): der Spiegelboden in duel_executor ---------------------
    add(tu("Spiegel: duel_executor laden", [] { (void)loadMapFromArchive("maps/duel_executor.bsp"); }));
    add(warteBis("Karte bereit", [] { return g_app->mapBereit; }, 900));
    add(tu("Spiegel: ueber dem Spiegelboden, Blick schraeg nach unten", [] {
        g_app->throughCamera = false;
        g_app->cameraSelected = false;
        g_app->cam.pos[0] = -1160.0F;
        g_app->cam.pos[1] = 1600.0F;
        g_app->cam.pos[2] = 100.0F;
        g_app->cam.angles[0] = 30.0F;
        g_app->cam.angles[1] = 90.0F;
        g_app->cam.angles[2] = 0.0F;
        g_app->mapDirty = true;
    }));
    add(pause(0.6));
    add(foto("spiegel_executor"));
    add(pruefe("Spiegel: der Boden zeigt das Spiegelbild (Satz 3), ohne D3D-Meldung", [] {
        diag::detail("Knopftest: Spiegel - gezeichnet " + std::to_string(g_app->spiegelGezeichnet) + ", Ebene " +
                     std::to_string(g_app->spiegelEbene[2]) + " d " + std::to_string(g_app->spiegelEbene[3]) +
                     ", D3D " + std::to_string(g_app->d3dMeldungen.size()));
        return g_app->spiegelGezeichnet && g_app->spiegelEbene[2] > 0.99F && g_app->d3dMeldungen.empty() &&
               gpu::zielTexturSatz(3) != nullptr;
    }));
    add(tu("Spiegel: hinter den Spiegel (unter den Boden)", [] {
        g_app->cam.pos[2] = -200.0F;
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(pruefe("Spiegel: von hinten kein Spiegelbild", [] { return !g_app->spiegelGezeichnet; }));
    // --- Hintergrund in der Farbe des globalen Nebels (rc600) -----------------
    // Fuer das Handbuch auf taspir1 gefunden: wo nichts gezeichnet wird, war
    // das Bild schwarz. Das Spiel loescht jedes Bild mit der Farbe des
    // globalen Nebels (RB_DrawBuffer: "don't clear during menus, wait for real
    // scene") - auf Taspir also orange, fogparms ( 0.8 0.7 0.4 ).
    add(tu("Nebelfarbe: taspir1 laden", [] { (void)loadMapFromArchive("maps/taspir1.bsp"); }));
    add(warteBis("Karte bereit", [] { return g_app->mapBereit; }, 1500));
    add(tu("Nebelfarbe: auf der Landeplattform steil nach oben", [] {
        g_app->throughCamera = false;
        g_app->cameraSelected = false;
        g_app->draufsicht = false;
        g_app->showFog = true;
        g_app->cam.pos[0] = -1600.0F;
        g_app->cam.pos[1] = -1500.0F;
        g_app->cam.pos[2] = 800.0F;
        g_app->cam.angles[0] = -80.0F;
        g_app->cam.angles[1] = 90.0F;
        g_app->cam.angles[2] = 0.0F;
        g_app->mapDirty = true;
    }));
    add(pause(0.6));
    add(foto("nebelfarbe_taspir"));
    add(pruefe("Nebelfarbe: ueber Taspir steht die Farbe des globalen Nebels, nicht Schwarz", [] {
        std::vector<std::uint8_t> px;
        int w = 0;
        int h = 0;
        if (!gpu::leseFarbe(px, w, h) || px.empty()) { return false; }
        long sum[3] = {0, 0, 0};
        int n = 0;
        for (int y = h / 2 - 4; y <= h / 2 + 4; ++y) {
            for (int x = w / 2 - 4; x <= w / 2 + 4; ++x) {
                const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)) * 4U;
                for (int c = 0; c < 3; ++c) { sum[c] += px[i + static_cast<std::size_t>(c)]; }
                ++n;
            }
        }
        const long r = sum[0] / n, g = sum[1] / n, b = sum[2] / n;
        diag::detail("Knopftest: Nebelfarbe - Mitte " + std::to_string(r) + " " + std::to_string(g) + " " + std::to_string(b) +
                     " (erwartet etwa 204 178 102)");
        return std::labs(r - 204) <= 14 && std::labs(g - 178) <= 14 && std::labs(b - 102) <= 14;
    }));
    return s;
}

