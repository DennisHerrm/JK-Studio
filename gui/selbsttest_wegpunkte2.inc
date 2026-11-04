// selbsttest_wegpunkte2.inc - rc609 W1: Wegpunkte loeschen, verbinden, duplizieren (BHED_EDITORTEST=wegpunkte2)
//
// Eingebunden an genau dieser Stelle von gui/selbsttest.cpp (im selben Namensraum, mit allen Helfern davor).
//
// Mike (03.11.): "if I delete a waypoint, it would be nice if everything that
// referenced it were also removed from its target", "If I add some waypoints
// manually, it would be nice if they automatically connected to nearby
// waypoints", "I had to awkwardly duplicate and rename everything". Nach dem
// Laden von md_am_sith (die ersten Schritte von karteSchritte):
//   1. Startpunkt: ein von Hand gesetzter Wegpunkt an einer Stelle, die das
//      Fluten von den anderen Startpunkten aus nicht erreicht - die Erzeugung
//      flutet auch von ihm aus und verbindet ihn
//   2. Loeschen (Entf): kein Verweis bleibt; Luecke schliessen; eine andere
//      Entity mit Verweisen fragt (Ja entfernt, Nein laesst stehen)
//   3. Setzen: der neue Punkt ist mit Nachbarn verbunden, jede Kante besteht
//      die Kantenpruefung des Spiels
//   4. Ziehen hinter eine Wand: die Kante, die das Spiel verwerfen wuerde, ist weg
//   5. Duplizieren (Strg+D): Kopie in sich verbunden, Namen frei; Umbenennen
//      nach Muster; Kontextmenue "Loeschen"
// Je Aktion genau ein Rueckgaengig-Schritt, Wiederholen stellt wieder her.
namespace w2 {

const char* const kFelder[5] = {"target", "target2", "target3", "target4", "targetJump"};

std::string kl(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}
int indexVon(const std::string& name) {
    for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
        if (g_app->map.entities[i].targetname == name) { return static_cast<int>(i); }
    }
    return -1;
}
int verweiseAuf(const std::string& name) {
    int n = 0;
    for (const MapEntity& e : g_app->map.entities) {
        for (const char* f : kFelder) {
            if (const std::string* t = e.find(f); t != nullptr && !name.empty() && kl(*t) == kl(name)) { ++n; }
        }
    }
    return n;
}
bool istWp(int i) {
    if (i < 0 || static_cast<std::size_t>(i) >= g_app->map.entities.size()) { return false; }
    const std::string& c = g_app->map.entities[static_cast<std::size_t>(i)].classname;
    return c == "waypoint" || c == "waypoint_small";
}
// Kante a <-> b ueber target..target4 (in einer der Richtungen).
bool kante(const std::vector<MapEntity>& v, int a, int b) {
    const MapEntity& ea = v[static_cast<std::size_t>(a)];
    const MapEntity& eb = v[static_cast<std::size_t>(b)];
    for (int z = 0; z < 4; ++z) {
        if (const std::string* t = ea.find(kFelder[z]); t != nullptr && !eb.targetname.empty() && *t == eb.targetname) { return true; }
        if (const std::string* t = eb.find(kFelder[z]); t != nullptr && !ea.targetname.empty() && *t == ea.targetname) { return true; }
    }
    return false;
}
bool kante(int a, int b) { return kante(g_app->map.entities, a, b); }
// Die Wegpunkt-Nachbarn von i.
std::vector<int> nachbarn(const std::vector<MapEntity>& v, int i) {
    std::vector<int> aus;
    for (std::size_t j = 0; j < v.size(); ++j) {
        const std::string& c = v[j].classname;
        if (static_cast<int>(j) != i && (c == "waypoint" || c == "waypoint_small") && kante(v, i, static_cast<int>(j))) {
            aus.push_back(static_cast<int>(j));
        }
    }
    return aus;
}
std::vector<int> nachbarn(int i) { return nachbarn(g_app->map.entities, i); }
// Besteht die Kante a -> b (a traegt das Feld) die Kantenpruefung des Spiels?
bool begehbar(const std::vector<MapEntity>& v, int a, int b) {
    const NavBauHilfe h = navBauHilfeKarte();
    NavKantenPruefer p(v, h);
    const MapEntity& ea = v[static_cast<std::size_t>(a)];
    const MapEntity& eb = v[static_cast<std::size_t>(b)];
    for (int z = 0; z < 4; ++z) {
        if (const std::string* t = ea.find(kFelder[z]); t != nullptr && *t == eb.targetname) { return p.pruefe(a, b).begehbar; }
    }
    return p.pruefe(b, a).begehbar;
}
ImVec2 aufSchirm(int i, float dz = 0.0F) {
    float o[3] = {0, 0, 0};
    if (i < 0 || !entityOrt(g_app->map.entities[static_cast<std::size_t>(i)], o)) { return ImVec2{-1, -1}; }
    o[2] += dz;
    ImVec2 s{-1, -1};
    if (!kartenPunktAufSchirm(o, s)) { return ImVec2{-1, -1}; }
    return s;
}
bool imBild(ImVec2 s, float rand = 60.0F) {
    return s.x > g_app->kartenAnsicht[0] + rand && s.x < g_app->kartenAnsicht[2] - rand && s.y > g_app->kartenAnsicht[1] + rand + 40.0F &&
           s.y < g_app->kartenAnsicht[3] - rand;
}
// Ein Punkt in der Kartenansicht, den kein schwebendes Fenster (das Wegpunkt-
// Fenster) verdeckt - dort hat die Ansicht den Fokus fuer Entf und Strg+D.
ImVec2 ansichtMitte() {
    ImRect fenster;
    for (ImGuiWindow* w : GImGui->Windows) {
        if (w->WasActive && !w->Hidden && std::strstr(w->Name, "###wegpunkte") != nullptr) { fenster = w->Rect(); }
    }
    const float x0 = g_app->kartenAnsicht[0];
    const float y0 = g_app->kartenAnsicht[1];
    const float x1 = g_app->kartenAnsicht[2];
    const float y1 = g_app->kartenAnsicht[3];
    for (const float fy : {0.5F, 0.7F, 0.3F, 0.85F}) {
        for (const float fx : {0.5F, 0.3F, 0.7F, 0.15F, 0.85F}) {
            const ImVec2 p{x0 + (x1 - x0) * fx, y0 + (y1 - y0) * fy};
            if (!fenster.Contains(p)) { return p; }
        }
    }
    return ImVec2{(x0 + x1) * 0.5F, (y0 + y1) * 0.5F};
}
std::vector<int> erzeugte() {
    std::vector<int> aus;
    for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
        if (g_app->map.entities[i].targetname.rfind("wpauto_", 0) == 0) { aus.push_back(static_cast<int>(i)); }
    }
    return aus;
}
float abstand3(const std::array<float, 3>& a, const float b[3]) {
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}

}  // namespace w2

std::vector<Schritt> wegpunkte2Schritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto tiefe0 = std::make_shared<std::size_t>(0);
    auto wp0 = std::make_shared<std::size_t>(0);
    auto tiefe = std::make_shared<std::size_t>(0);   // Stand vor einer Aktion
    add(tu("W2: Ausgangslage", [=] {
        g_app->throughCamera = false;
        g_app->playing = false;
        g_app->wegFensterOffen = true;
        g_app->zeigeWegnetz = true;
        g_app->wegAutoVerbinden = true;
        g_app->wegLueckeSchliessen = false;
        *tiefe0 = g_app->entUndo.size();
        *wp0 = 0;
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) { *wp0 += w2::istWp(static_cast<int>(i)) ? 1U : 0U; }
        diag::detail("Knopftest: W2 - Karte " + g_app->map.path + ", " + std::to_string(*wp0) + " Wegpunkte");
    }));
    add(pause(0.4));
    add(klick(tr(Str::WegErzeugen), "###wegpunkte"));
    add(pause(0.5));
    add(pruefe("W2: Netz erzeugt (mindestens 20 Wegpunkte)", [] {
        diag::detail("Knopftest: W2 - " + g_app->wegMeldung);
        return w2::erzeugte().size() >= 20U;
    }));

    // --- 1. Startpunkt: ein Wegpunkt, den das Fluten sonst nicht erreicht ---------
    auto stelle = std::make_shared<std::array<float, 3>>();
    auto flut = std::make_shared<std::vector<std::array<float, 3>>>();
    add(tu("W2 Start: eine Stelle suchen, die das Fluten nicht erreicht", [=] {
        flut->clear();
        std::vector<std::array<float, 3>> netz;
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            float o[3];
            if (istWegKlasse(g_app->map.entities[i].classname) && entityOrt(g_app->map.entities[i], o)) { netz.push_back({o[0], o[1], o[2]}); }
        }
        const float S = g_app->wegAbstand;
        const auto& m = g_app->geo.models.front();
        WegErzeugung p;
        p.abstand = S;
        p.maxKnoten = 200;
        const std::unique_ptr<WegWeltKarte> welt = wegWeltKarte();
        // rc609 W3a: Die Erzeugung kommt jetzt zu Fuss durch Tueren und ueber Treppen. Mit der Suche nur auf der obersten
        // Flaeche jeder Spalte (bis W1) blieb nichts mehr: Lauf we7w2 fand 779 Stellen weit vom Netz, von keiner entstand
        // ein Punkt (der Kasten steckt dort im Festen, ausserhalb der spielbaren Huelle). Gesucht wird darum auf allen
        // Boeden jeder Spalte nach einer Stelle, die zu Fuss nicht ans Netz kommt: kein Netzpunkt auf ihrer Hoehe (eine
        // Stufe) in einer halben Rasterweite, und bei keinem der Punkte, die von ihr aus entstehen.
        const auto amNetz = [&](const float q[3]) {
            for (const auto& n : netz) {
                if (std::hypot(n[0] - q[0], n[1] - q[1]) <= S * 0.5F && std::fabs(n[2] - q[2]) <= 18.0F) { return true; }
            }
            return false;
        };
        std::array<int, 4> zahl{0, 0, 0, 0};   // Boden, fern vom Netz, Flut mit Punkten, abgetrennt
        for (float x = m.mins[0] + S * 0.5F; x < m.maxs[0] && flut->empty(); x += S) {
            for (float y = m.mins[1] + S * 0.5F; y < m.maxs[1] && flut->empty(); y += S) {
                float zOben = m.maxs[2] - 1.0F;
                for (int runde = 0; runde < 16 && flut->empty() && zOben > m.mins[2]; ++runde) {
                    const float a[3] = {x, y, zOben};
                    const float b[3] = {x, y, m.mins[2] + 1.0F};
                    const TraceTreffer t = traceRay(g_app->geo, a, b);
                    if (!t.hit) { break; }
                    zOben = t.point[2] - 2.0F;
                    if (t.normal[2] < 0.7F) { continue; }
                    ++zahl[0];
                    const float q[3] = {t.point[0], t.point[1], t.point[2] + 25.0F};
                    if (amNetz(q)) { continue; }
                    ++zahl[1];
                    const std::vector<NeuerWegpunkt> f = erzeugeWegpunkte({{t.point[0], t.point[1], t.point[2] + 1.0F}}, welt->welt, p);
                    if (f.size() < 3U) { continue; }
                    ++zahl[2];
                    bool getrennt = true;
                    for (const NeuerWegpunkt& k : f) { getrennt = getrennt && !amNetz(k.ort.data()); }
                    if (!getrennt) { continue; }
                    ++zahl[3];
                    *stelle = {t.point[0], t.point[1], t.point[2]};
                    for (const NeuerWegpunkt& k : f) { flut->push_back(k.ort); }
                }
            }
        }
        diag::detail("Knopftest: W2 Start - Stelle " + std::to_string((*stelle)[0]) + " " + std::to_string((*stelle)[1]) + " " +
                     std::to_string((*stelle)[2]) + ", von dort " + std::to_string(flut->size()) + " Rasterpunkte; gesucht: Boden " +
                     std::to_string(zahl[0]) + ", fern vom Netz " + std::to_string(zahl[1]) + ", mit Flut " + std::to_string(zahl[2]) +
                     ", abgetrennt " + std::to_string(zahl[3]));
    }));
    add(pruefe("W2 Start: eine abgetrennte Stelle mit Boden gefunden", [=] { return !flut->empty(); }));
    add(tu("W2 Start: Erzeugung zurueck, dort von Hand einen Wegpunkt setzen", [=] {
        while (g_app->entUndo.size() > *tiefe0) { (void)entityRueckgaengig(); }
        const float boden[3] = {(*stelle)[0], (*stelle)[1], (*stelle)[2]};
        (void)wegSetzen(boden, -1);
    }));
    add(klick(tr(Str::WegErzeugen), "###wegpunkte"));
    add(pause(0.5));
    add(pruefe("W2 Start: die Erzeugung flutet auch vom gesetzten Wegpunkt aus und verbindet ihn", [=] {
        int dort = 0;
        for (const int i : w2::erzeugte()) {
            float o[3];
            if (!entityOrt(g_app->map.entities[static_cast<std::size_t>(i)], o)) { continue; }
            for (const auto& f : *flut) {
                if (w2::abstand3(f, o) < g_app->wegAbstand * 0.5F) {
                    ++dort;
                    break;
                }
            }
        }
        int handNachbarn = 0;
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            float o[3];
            const MapEntity& e = g_app->map.entities[i];
            if (e.classname != "waypoint" || e.targetname.rfind("wpauto_", 0) == 0 || !entityOrt(e, o)) { continue; }
            const float b[3] = {(*stelle)[0], (*stelle)[1], (*stelle)[2] + 25.0F};
            const std::array<float, 3> oa{o[0], o[1], o[2]};
            if (w2::abstand3(oa, b) < 2.0F) { handNachbarn = static_cast<int>(w2::nachbarn(static_cast<int>(i)).size()); }
        }
        diag::detail("Knopftest: W2 Start - erzeugt im abgetrennten Bereich " + std::to_string(dort) + " von " +
                     std::to_string(flut->size()) + ", Nachbarn des gesetzten " + std::to_string(handNachbarn));
        return dort >= 2 && handNachbarn >= 1;
    }));
    add(tu("W2 Start: alles zurueck, Netz neu", [=] {
        while (g_app->entUndo.size() > *tiefe0) { (void)entityRueckgaengig(); }
    }));
    add(klick(tr(Str::WegErzeugen), "###wegpunkte"));
    add(pause(0.5));
    auto tiefeNetz = std::make_shared<std::size_t>(0);
    add(tu("W2: Stand mit Netz", [=] { *tiefeNetz = g_app->entUndo.size(); }));

    // --- 2. Loeschen ----------------------------------------------------------------
    auto name = std::make_shared<std::string>();
    auto verweise = std::make_shared<int>(0);
    add(tu("W2 Loeschen: einen erzeugten Wegpunkt waehlen, auf den andere zeigen", [=] {
        name->clear();
        for (const int i : w2::erzeugte()) {
            const std::string& n = g_app->map.entities[static_cast<std::size_t>(i)].targetname;
            if (w2::verweiseAuf(n) >= 2) {
                *name = n;
                entityAuswahlSetzen(i);
                break;
            }
        }
        *verweise = w2::verweiseAuf(*name);
        *tiefe = g_app->entUndo.size();
        diag::detail("Knopftest: W2 Loeschen - " + *name + " mit " + std::to_string(*verweise) + " Verweisen");
        setzeMaus(w2::ansichtMitte());
    }));
    add(pause(0.3));
    add(taste(ImGuiKey_Delete, "Entf"));
    add(pause(0.3));
    add(pruefe("W2 Loeschen (Entf): Wegpunkt weg, kein target zeigt mehr auf ihn, ein Schritt", [=] {
        diag::detail("Knopftest: W2 Loeschen - danach " + std::to_string(w2::verweiseAuf(*name)) + " Verweise, Schritte +" +
                     std::to_string(g_app->entUndo.size() - *tiefe));
        return !name->empty() && w2::indexVon(*name) < 0 && w2::verweiseAuf(*name) == 0 && g_app->entUndo.size() == *tiefe + 1;
    }));
    add(tu("W2 Loeschen: Rueckgaengig", [] { (void)entityRueckgaengig(); }));
    add(pruefe("W2 Loeschen: Rueckgaengig bringt Punkt und alle Verweise zurueck", [=] {
        return w2::indexVon(*name) >= 0 && w2::verweiseAuf(*name) == *verweise && g_app->entUndo.size() == *tiefe;
    }));
    add(tu("W2 Loeschen: Wiederholen", [] { (void)entityWiederholen(); }));
    add(pruefe("W2 Loeschen: Wiederholen loescht wieder samt Verweisen", [=] {
        return w2::indexVon(*name) < 0 && w2::verweiseAuf(*name) == 0 && g_app->entUndo.size() == *tiefe + 1;
    }));
    add(foto("w2_loeschen"));

    // Luecke schliessen
    auto paare = std::make_shared<std::vector<std::pair<std::string, std::string>>>();
    add(tu("W2 Luecke: einschalten, einen Wegpunkt mit drei Nachbarn waehlen", [=] {
        g_app->wegLueckeSchliessen = true;
        paare->clear();
        name->clear();
        for (const int i : w2::erzeugte()) {
            const std::vector<int> nb = w2::nachbarn(i);
            if (nb.size() < 3U) { continue; }
            *name = g_app->map.entities[static_cast<std::size_t>(i)].targetname;
            for (std::size_t a = 0; a < nb.size(); ++a) {
                for (std::size_t b = a + 1; b < nb.size(); ++b) {
                    if (!w2::kante(nb[a], nb[b])) {
                        paare->emplace_back(g_app->map.entities[static_cast<std::size_t>(nb[a])].targetname,
                                            g_app->map.entities[static_cast<std::size_t>(nb[b])].targetname);
                    }
                }
            }
            if (paare->empty()) { continue; }
            entityAuswahlSetzen(i);
            break;
        }
        *tiefe = g_app->entUndo.size();
        diag::detail("Knopftest: W2 Luecke - " + *name + ", " + std::to_string(paare->size()) + " Nachbarpaare ohne Kante");
        setzeMaus(w2::ansichtMitte());
    }));
    add(pause(0.3));
    add(taste(ImGuiKey_Delete, "Entf"));
    add(pause(0.3));
    add(pruefe("W2 Luecke: die Nachbarn sind verbunden, wo das Spiel die Kante behaelt, alles in einem Schritt", [=] {
        int neu = 0;
        int falsch = 0;
        for (const auto& [na, nb] : *paare) {
            const int a = w2::indexVon(na);
            const int b = w2::indexVon(nb);
            if (a < 0 || b < 0 || !w2::kante(a, b)) { continue; }
            ++neu;
            falsch += w2::begehbar(g_app->map.entities, a, b) ? 0 : 1;
        }
        diag::detail("Knopftest: W2 Luecke - " + std::to_string(neu) + " neue Kanten unter den Nachbarn, " + std::to_string(falsch) +
                     " davon nicht begehbar");
        g_app->wegLueckeSchliessen = false;
        return !name->empty() && w2::indexVon(*name) < 0 && w2::verweiseAuf(*name) == 0 && neu >= 1 && falsch == 0 &&
               g_app->entUndo.size() == *tiefe + 1;
    }));

    // Andere Entity mit Verweisen: Rueckfrage im Programm
    auto anderer = std::make_shared<std::string>();
    auto klasse = std::make_shared<std::string>();
    const auto andereWaehlen = [=] {
        anderer->clear();
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            const MapEntity& e = g_app->map.entities[i];
            if (e.targetname.empty() || istWegKlasse(e.classname) || e.classname == "worldspawn") { continue; }
            int gleich = 0;
            for (const MapEntity& x : g_app->map.entities) { gleich += w2::kl(x.targetname) == w2::kl(e.targetname) ? 1 : 0; }
            if (gleich == 1 && w2::verweiseAuf(e.targetname) >= 1) {
                *anderer = e.targetname;
                *klasse = e.classname;
                entityAuswahlSetzen(static_cast<int>(i));
                break;
            }
        }
        *verweise = w2::verweiseAuf(*anderer);
        *tiefe = g_app->entUndo.size();
        diag::detail("Knopftest: W2 Frage - " + *klasse + " " + *anderer + " mit " + std::to_string(*verweise) + " Verweisen");
        setzeMaus(w2::ansichtMitte());
    };
    add(tu("W2 Frage: eine andere Entity waehlen, auf die ein target zeigt", andereWaehlen));
    add(pause(0.3));
    add(taste(ImGuiKey_Delete, "Entf"));
    add(pause(0.3));
    add(pruefe("W2 Frage: Entf fragt im Programm (noch nichts geloescht)", [=] {
        return !anderer->empty() && g_app->frageOffen && w2::indexVon(*anderer) >= 0;
    }));
    add(foto("w2_frage"));
    add(klick(tr(Str::AnswerYes), "###frage"));
    add(pause(0.3));
    add(pruefe("W2 Frage: Ja loescht samt Verweisen, ein Schritt", [=] {
        return w2::indexVon(*anderer) < 0 && w2::verweiseAuf(*anderer) == 0 && g_app->entUndo.size() == *tiefe + 1;
    }));
    add(tu("W2 Frage: Rueckgaengig", [] { (void)entityRueckgaengig(); }));
    add(pruefe("W2 Frage: Rueckgaengig bringt Entity und Verweise zurueck", [=] {
        return w2::indexVon(*anderer) >= 0 && w2::verweiseAuf(*anderer) == *verweise;
    }));
    add(tu("W2 Frage: noch einmal waehlen", andereWaehlen));
    add(pause(0.3));
    add(taste(ImGuiKey_Delete, "Entf"));
    add(pause(0.3));
    add(klick(tr(Str::AnswerNo), "###frage"));
    add(pause(0.3));
    add(pruefe("W2 Frage: Nein loescht nur die Entity, die Verweise bleiben", [=] {
        return w2::indexVon(*anderer) < 0 && w2::verweiseAuf(*anderer) == *verweise && g_app->entUndo.size() == *tiefe + 1;
    }));
    add(tu("W2 Frage: Rueckgaengig", [] { (void)entityRueckgaengig(); }));

    // --- 3. Setzen ------------------------------------------------------------------
    auto klickOrt = std::make_shared<ImVec2>();
    add(tu("W2 Setzen: eine Bodenstelle zwischen erzeugten Wegpunkten im Bild", [=] {
        *klickOrt = ImVec2{-1, -1};
        for (const int i : w2::erzeugte()) {
            float o[3];
            if (!entityOrt(g_app->map.entities[static_cast<std::size_t>(i)], o) || w2::nachbarn(i).size() < 3U) { continue; }
            const float b[3] = {o[0] + g_app->wegAbstand * 0.4F, o[1] + g_app->wegAbstand * 0.3F, o[2] - 25.0F};
            ImVec2 sp;
            if (!kartenPunktAufSchirm(b, sp) || !w2::imBild(sp)) { continue; }
            TraceTreffer t;
            if (!kartenStrahlUnterMaus(sp.x, sp.y, t) || t.normal[2] < 0.7F || std::fabs(t.point[2] - b[2]) > 8.0F) { continue; }
            // Nur wo das Spiel den Wegpunkt behaelt (nicht im Festen, G_CheckInSolid).
            std::vector<MapEntity> v = g_app->map.entities;
            MapEntity probe;
            char ot[96];
            std::snprintf(ot, sizeof(ot), "%d %d %d", static_cast<int>(std::lround(t.point[0])), static_cast<int>(std::lround(t.point[1])),
                          static_cast<int>(std::lround(t.point[2] + 25.0F)));
            setKey(probe, "classname", "waypoint");
            setKey(probe, "origin", ot);
            v.push_back(probe);
            const NavBauHilfe h = navBauHilfeKarte();
            NavKantenPruefer p(v, h);
            if (!p.hatKnoten(static_cast<int>(v.size()) - 1)) { continue; }
            *klickOrt = sp;
            break;
        }
        *tiefe = g_app->entUndo.size();
        diag::detail("Knopftest: W2 Setzen - Klick bei " + std::to_string(klickOrt->x) + " " + std::to_string(klickOrt->y));
    }));
    add(klick(tr(Str::WegSetzen), "###wegpunkte"));
    add(pause(0.2));
    auto gesetzt = std::make_shared<std::string>();
    add(klickAn("W2 Setzen: auf den Boden", [=] { return *klickOrt; }));
    add(pause(0.3));
    add(taste(ImGuiKey_Escape, "Esc beendet das Werkzeug"));
    add(pruefe("W2 Setzen: der neue Wegpunkt ist mit Nachbarn verbunden, jede Kante wie im Spiel, ein Schritt", [=] {
        const int n = static_cast<int>(g_app->map.entities.size()) - 1;
        if (!w2::istWp(n)) { return false; }
        *gesetzt = g_app->map.entities[static_cast<std::size_t>(n)].targetname;
        const std::vector<int> nb = w2::nachbarn(n);
        int falsch = 0;
        for (const int j : nb) { falsch += w2::begehbar(g_app->map.entities, n, j) ? 0 : 1; }
        diag::detail("Knopftest: W2 Setzen - " + *gesetzt + " (" + g_app->map.entities[static_cast<std::size_t>(n)].origin + ") mit " +
                     std::to_string(nb.size()) + " Nachbarn, " + std::to_string(falsch) + " nicht begehbar");
        {   // Was die Kantenpruefung zu den naechsten erzeugten sagt
            const NavBauHilfe h = navBauHilfeKarte();
            NavKantenPruefer p(g_app->map.entities, h);
            std::string z = std::string("Knopftest: W2 Setzen - Spur ") + (h.spur ? "da" : "fehlt") + ", Knoten " + (p.hatKnoten(n) ? "da" : "im Festen");
            int k = 0;
            for (const int j : w2::erzeugte()) {
                float a3[3];
                float b3[3];
                if (!entityOrt(g_app->map.entities[static_cast<std::size_t>(j)], a3) || !entityOrt(g_app->map.entities[static_cast<std::size_t>(n)], b3)) { continue; }
                const float d = std::sqrt((a3[0] - b3[0]) * (a3[0] - b3[0]) + (a3[1] - b3[1]) * (a3[1] - b3[1]) + (a3[2] - b3[2]) * (a3[2] - b3[2]));
                if (d > g_app->wegAbstand || k++ > 4) { continue; }
                z += "; " + g_app->map.entities[static_cast<std::size_t>(j)].targetname + " " + std::to_string(static_cast<int>(d)) + ": " +
                     (p.pruefe(n, j).begehbar ? "begehbar" : "nein") + "/" + (p.pruefe(j, n).begehbar ? "begehbar" : "nein");
            }
            diag::detail(z);
        }
        return nb.size() >= 2U && falsch == 0 && g_app->entUndo.size() == *tiefe + 1;
    }));
    add(foto("w2_setzen"));
    add(tu("W2 Setzen: Rueckgaengig", [] { (void)entityRueckgaengig(); }));
    add(pruefe("W2 Setzen: Rueckgaengig nimmt Punkt und Kanten zurueck", [=] {
        return w2::indexVon(*gesetzt) < 0 && w2::verweiseAuf(*gesetzt) == 0 && g_app->entUndo.size() == *tiefe;
    }));
    add(tu("W2 Setzen: Wiederholen", [] { (void)entityWiederholen(); }));
    add(pruefe("W2 Setzen: Wiederholen stellt Punkt und Kanten wieder her", [=] {
        const int n = w2::indexVon(*gesetzt);
        return n >= 0 && w2::nachbarn(n).size() >= 2U && g_app->entUndo.size() == *tiefe + 1;
    }));

    // --- 4. Ziehen hinter eine Wand -------------------------------------------------
    auto ziel = std::make_shared<ImVec2>();
    auto zugVon = std::make_shared<ImVec2>();
    auto vorZug = std::make_shared<std::vector<int>>();
    auto vorOrt = std::make_shared<std::string>();
    // rc609 W3a: Das neue Netz ist dichter; der Griff traf einen anderen Wegpunkt davor (Lauf wecw2f/wedw2: wpauto_131
    // gewaehlt, 0 Schritte). Darum sind die anderen Wegpunkte beim Ziehen ausgeblendet (nicht greifbar; ihre Kanten
    // prueft das Ziehen trotzdem), das schwebende Wegpunkt-Fenster zu und die Kamera auf den Punkt gerichtet.
    add(tu("W2 Ziehen: nur den gesetzten Punkt greifbar, im Bild", [=] {
        g_app->wegFensterOffen = false;
        const int n = w2::indexVon(*gesetzt);
        g_app->hiddenEntities.clear();
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            if (static_cast<int>(i) != n && istWegKlasse(g_app->map.entities[i].classname)) { g_app->hiddenEntities.insert(i); }
        }
        float o[3];
        if (n >= 0 && entityOrt(g_app->map.entities[static_cast<std::size_t>(n)], o)) {
            aimAngles(g_app->cam.pos, o, g_app->cam.angles);
            g_app->mapDirty = true;
        }
    }));
    add(pause(0.3));
    add(tu("W2 Ziehen: eine Stelle im Bild suchen, an der eine Kante des Punkts nicht mehr begehbar ist", [=] {
        *ziel = ImVec2{-1, -1};
        const int n = w2::indexVon(*gesetzt);
        if (n < 0) { return; }
        entityAuswahlSetzen(n);
        *vorZug = w2::nachbarn(n);
        *vorOrt = g_app->map.entities[static_cast<std::size_t>(n)].origin;
        // Gegriffen am Marker der Entity (entityZiehen greift nur hoechstens 1,2 Schriftgroessen vom Marker,
        // app_mission.cpp; rc609 W3a: vorher 20 unter dem Ursprung - aus der Naehe mehr Bildpunkte als das, Laeufe
        // wecw2..weew2: 0 Schritte), gezielt auf dieselbe Hoehe ueber dem Ziel: das Ziehen setzt den Punkt auf die
        // Flaeche unter der Maus mit dem Griffversatz (entityZiehen, Modus 0) und auf das Raster.
        const ImVec2 von = w2::aufSchirm(n);
        diag::detail("Knopftest: W2 Ziehen - Griff bei " + std::to_string(von.x) + " " + std::to_string(von.y) + (w2::imBild(von) ? " im Bild" : " nicht im Bild"));
        float best = 1.0e9F;
        for (const int i : w2::erzeugte()) {
            float o[3];
            if (!entityOrt(g_app->map.entities[static_cast<std::size_t>(i)], o)) { continue; }
            const float q[3] = {o[0] + 30.0F, o[1] + 30.0F, o[2]};
            const float qs[3] = {q[0], q[1], q[2]};
            ImVec2 sp;
            if (!kartenPunktAufSchirm(qs, sp) || !w2::imBild(sp)) { continue; }
            const float d = std::fabs(sp.x - von.x) + std::fabs(sp.y - von.y);
            if (d >= best || d < 40.0F) { continue; }
            // rc609 W3a: ueber die Geometrie, nicht ueber die Lage erzeugter Punkte (die die neue Erzeugung anders setzt):
            // vom Zielpunkt und seiner Umgebung (Griffversatz, Raster) aus steht zu mindestens einem bisherigen Nachbarn
            // eine Wand im Weg - ein Strahl auf Brusthoehe trifft sie. Dort bricht die Kante im Spiel sicher.
            bool ueberall = true;
            for (const auto& [dx, dy] : {std::pair{0.0F, 0.0F}, {24.0F, 24.0F}, {-24.0F, 24.0F}, {24.0F, -24.0F}, {-24.0F, -24.0F}}) {
                int wand = 0;
                for (const int j : *vorZug) {
                    float oj[3];
                    if (!entityOrt(g_app->map.entities[static_cast<std::size_t>(j)], oj)) { continue; }
                    const float a[3] = {q[0] + dx, q[1] + dy, q[2] + 16.0F};
                    const float b[3] = {oj[0], oj[1], oj[2] + 16.0F};
                    wand += traceRay(g_app->geo, a, b).hit ? 1 : 0;
                }
                ueberall = ueberall && wand > 0;
            }
            if (!ueberall) { continue; }
            best = d;
            *ziel = sp;
        }
        *tiefe = g_app->entUndo.size();
        diag::detail("Knopftest: W2 Ziehen - " + std::to_string(vorZug->size()) + " Nachbarn, Ziel " + std::to_string(ziel->x) + " " +
                     std::to_string(ziel->y));
    }));
    add(pruefe("W2 Ziehen: Stelle hinter einer Wand gefunden", [=] { return ziel->x >= 0.0F; }));
    add({"Ziehe W2: den Wegpunkt hinter die Wand", [=](int b) {
             ImGuiIO& io = ImGui::GetIO();
             if (b == 0) {
                 // An der Mitte der Entity (dort greift entityZiehen sie zum Ziehen ueber den Boden), aber neben den
                 // Pfeilen des Gizmos: die greifen ab einem Viertel ihrer Laenge 0,45 Schriftgroessen breit
                 // (gizmoGriff, app_mission.cpp). Lauf wegw2: genau in der Mitte lag der verkuerzte x-Pfeil
                 // (Griff 1), der Punkt ging nur entlang x. Daher der Punkt in 0,8 Schriftgroessen um die Mitte, der
                 // von allen drei Pfeilen am weitesten weg ist (die Entity greift bis 1,2 Schriftgroessen).
                 const int n = w2::indexVon(*gesetzt);
                 const ImVec2 mitte = w2::aufSchirm(n);
                 float o[3] = {0.0F, 0.0F, 0.0F};
                 if (n >= 0) { (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(n)], o); }
                 ImVec2 pfeil[3];
                 for (int k = 0; k < 3; ++k) {
                     float e[3] = {o[0], o[1], o[2]};
                     e[k] += 8.0F;
                     ImVec2 se{mitte.x, mitte.y};
                     (void)kartenPunktAufSchirm(e, se);
                     const float dx = se.x - mitte.x;
                     const float dy = se.y - mitte.y;
                     const float l = std::max(1.0e-3F, std::hypot(dx, dy));
                     pfeil[k] = ImVec2{dx / l, dy / l};
                 }
                 const float r = ImGui::GetFontSize() * 0.8F;
                 float bestAbstand = -1.0F;
                 *zugVon = mitte;
                 for (int w = 0; w < 16; ++w) {
                     const float wink = static_cast<float>(w) * 6.2831853F / 16.0F;
                     const ImVec2 c{mitte.x + std::cos(wink) * r, mitte.y + std::sin(wink) * r};
                     float minWinkel = 10.0F;
                     for (const ImVec2& pf : pfeil) {
                         const float cw = (std::cos(wink) * pf.x + std::sin(wink) * pf.y);
                         minWinkel = std::min(minWinkel, std::acos(std::clamp(cw, -1.0F, 1.0F)));
                     }
                     if (minWinkel > bestAbstand) {
                         bestAbstand = minWinkel;
                         *zugVon = c;
                     }
                 }
                 setzeMaus(*zugVon);
                 return false;
             }
             if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
             if (b == 3) {
                 const int z = g_app->pickedEntity;
                 diag::detail("Knopftest: W2 Ziehen - beim Druck: gewaehlt " +
                              (z >= 0 ? g_app->map.entities[static_cast<std::size_t>(z)].targetname : std::string("-")) + ", unter der Maus " +
                              (g_app->entHover >= 0 ? g_app->map.entities[static_cast<std::size_t>(g_app->entHover)].targetname : std::string("-")) +
                              ", Zug " + (g_app->entZug.kandidat ? "ja" : "nein") + ", Gizmo-Achse " + std::to_string(g_app->gizmoAxis) +
                              ", Griff " + std::to_string(g_app->entGizmoHover));
             }
             if (b >= 3 && b <= 14) {
                 const float f = static_cast<float>(b - 2) / 12.0F;
                 setzeMaus(ImVec2{zugVon->x + (ziel->x - zugVon->x) * f, zugVon->y + (ziel->y - zugVon->y) * f});
                 return false;
             }
             if (b == 15) { io.AddMouseButtonEvent(0, false); return false; }
             return b >= 20;
         }});
    add(pause(0.3));
    add(pruefe("W2 Ziehen: verschoben, die nicht mehr begehbaren Kanten sind weg, jede Kante wie im Spiel, ein Schritt", [=] {
        const int n = w2::indexVon(*gesetzt);
        if (n < 0) { return false; }
        const std::vector<int> nb = w2::nachbarn(n);
        int falsch = 0;
        for (const int j : nb) { falsch += w2::begehbar(g_app->map.entities, n, j) ? 0 : 1; }
        int weg = 0;
        for (const int j : *vorZug) { weg += std::find(nb.begin(), nb.end(), j) == nb.end() ? 1 : 0; }
        // Was das Ziehen bewegt hat (gegen den Stand vor dem letzten Rueckgaengig-Schritt).
        std::string bewegt;
        if (g_app->entUndo.size() > *tiefe && g_app->entUndo.back().size() == g_app->map.entities.size()) {
            for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
                if (g_app->entUndo.back()[i].origin != g_app->map.entities[i].origin) { bewegt += " " + g_app->map.entities[i].targetname; }
            }
        }
        diag::detail("Knopftest: W2 Ziehen - Schritte " + std::to_string(g_app->entUndo.size() - *tiefe) + ", bewegt:" + bewegt);
        diag::detail("Knopftest: W2 Ziehen - von " + *vorOrt + " nach " + g_app->map.entities[static_cast<std::size_t>(n)].origin + ", " +
                     std::to_string(weg) + " Kanten getrennt, jetzt " + std::to_string(nb.size()) + " Nachbarn, " + std::to_string(falsch) +
                     " nicht begehbar");
        return g_app->map.entities[static_cast<std::size_t>(n)].origin != *vorOrt && weg >= 1 && falsch == 0 &&
               g_app->entUndo.size() == *tiefe + 1;
    }));
    add(foto("w2_ziehen"));
    add(tu("W2 Ziehen: Rueckgaengig", [] { (void)entityRueckgaengig(); }));
    add(pruefe("W2 Ziehen: Rueckgaengig stellt Ort und Kanten wieder her", [=] {
        const int n = w2::indexVon(*gesetzt);
        return n >= 0 && g_app->map.entities[static_cast<std::size_t>(n)].origin == *vorOrt && w2::nachbarn(n) == *vorZug;
    }));
    add(tu("W2 Ziehen: Wiederholen", [] { (void)entityWiederholen(); }));
    add(pruefe("W2 Ziehen: Wiederholen verschiebt wieder", [=] {
        const int n = w2::indexVon(*gesetzt);
        return n >= 0 && g_app->map.entities[static_cast<std::size_t>(n)].origin != *vorOrt && g_app->entUndo.size() == *tiefe + 1;
    }));
    add(tu("W2 Ziehen: Fenster wieder auf, alle Wegpunkte sichtbar", [] {
        g_app->wegFensterOffen = true;
        g_app->hiddenEntities.clear();
    }));

    // --- 5. Duplizieren, Umbenennen, Kontextmenue -----------------------------------
    auto quellen = std::make_shared<std::vector<std::string>>();
    auto vorZahl = std::make_shared<std::size_t>(0);
    add(tu("W2 Duplizieren: drei verbundene Wegpunkte waehlen (eine Kette)", [=] {
        quellen->clear();
        for (const int a : w2::erzeugte()) {
            for (const int b : w2::nachbarn(a)) {
                for (const int c : w2::nachbarn(b)) {
                    if (c == a || quellen->size() == 3U) { continue; }
                    *quellen = {g_app->map.entities[static_cast<std::size_t>(a)].targetname,
                                g_app->map.entities[static_cast<std::size_t>(b)].targetname,
                                g_app->map.entities[static_cast<std::size_t>(c)].targetname};
                }
                if (quellen->size() == 3U) { break; }
            }
            if (quellen->size() == 3U) { break; }
        }
        entityAuswahlSetzen(-1);
        for (const std::string& q : *quellen) { entityAuswahlUmschalten(w2::indexVon(q)); }
        *vorZahl = g_app->map.entities.size();
        *tiefe = g_app->entUndo.size();
        diag::detail("Knopftest: W2 Duplizieren - " + std::to_string(quellen->size()) + " Quellen");
        setzeMaus(w2::ansichtMitte());
    }));
    add(pause(0.3));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_D, "Strg+D"));
    add(pause(0.3));
    auto kopien = std::make_shared<std::vector<std::string>>();
    add(pruefe("W2 Duplizieren (Strg+D): drei Kopien, Namen frei, in sich verbunden wie die Quellen, ein Schritt", [=] {
        if (quellen->size() != 3U || g_app->map.entities.size() != *vorZahl + 3U) { return false; }
        kopien->clear();
        for (std::size_t k = 0; k < 3; ++k) { kopien->push_back(g_app->map.entities[*vorZahl + k].targetname); }
        int doppelt = 0;   // ein Name einer Kopie, den noch eine andere Entity traegt
        for (const std::string& k : *kopien) {
            int gleich = 0;
            for (const MapEntity& e : g_app->map.entities) { gleich += w2::kl(e.targetname) == w2::kl(k) ? 1 : 0; }
            doppelt += (k.empty() || gleich != 1) ? 1 : 0;
        }
        int gleich = 0;
        int innen = 0;
        int altVerweis = 0;
        for (int a = 0; a < 3; ++a) {
            for (int b = a + 1; b < 3; ++b) {
                const bool q = w2::kante(w2::indexVon((*quellen)[static_cast<std::size_t>(a)]), w2::indexVon((*quellen)[static_cast<std::size_t>(b)]));
                const bool c = w2::kante(w2::indexVon((*kopien)[static_cast<std::size_t>(a)]), w2::indexVon((*kopien)[static_cast<std::size_t>(b)]));
                gleich += q == c ? 1 : 0;
                innen += c ? 1 : 0;
            }
        }
        for (const std::string& k : *kopien) {
            const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(w2::indexVon(k))];
            for (const char* f : w2::kFelder) {
                const std::string* t = e.find(f);
                if (t == nullptr) { continue; }
                for (std::size_t q = 0; q < quellen->size(); ++q) {
                    // eine Kante innerhalb der Quellen darf in der Kopie nicht auf die Quelle zeigen
                    altVerweis += *t == (*quellen)[q] ? 1 : 0;
                }
            }
        }
        diag::detail("Knopftest: W2 Duplizieren - Kopien " + (*kopien)[0] + " " + (*kopien)[1] + " " + (*kopien)[2] + ", innen " +
                     std::to_string(innen) + " Kanten (gleich wie Quellen: " + std::to_string(gleich) + "/3), auf Quellen " +
                     std::to_string(altVerweis) + ", doppelte Namen " + std::to_string(doppelt));
        return doppelt == 0 && gleich == 3 && innen >= 2 && altVerweis == 0 && g_app->entUndo.size() == *tiefe + 1;
    }));
    add(foto("w2_duplizieren"));
    add(tu("W2 Duplizieren: Rueckgaengig", [] { (void)entityRueckgaengig(); }));
    add(pruefe("W2 Duplizieren: Rueckgaengig nimmt die Kopien weg", [=] {
        return g_app->map.entities.size() == *vorZahl && w2::indexVon((*kopien)[0]) < 0;
    }));
    add(tu("W2 Duplizieren: Wiederholen", [] { (void)entityWiederholen(); }));
    add(pruefe("W2 Duplizieren: Wiederholen bringt die Kopien zurueck", [=] {
        return g_app->map.entities.size() == *vorZahl + 3U && w2::indexVon((*kopien)[2]) >= 0;
    }));
    add(tu("W2 Umbenennen: die Kopien waehlen, Muster w2kopie_#", [=] {
        entityAuswahlSetzen(-1);
        for (const std::string& k : *kopien) { entityAuswahlUmschalten(w2::indexVon(k)); }
        g_app->wegMuster = "w2kopie_#";
        *tiefe = g_app->entUndo.size();
    }));
    add(klick(tr(Str::WegUmbenennen), "###wegpunkte"));
    add(pause(0.3));
    add(pruefe("W2 Umbenennen: neue Namen nach Muster, die Verweise ziehen mit, ein Schritt", [=] {
        int alt = 0;
        for (const std::string& k : *kopien) { alt += w2::verweiseAuf(k) + (w2::indexVon(k) >= 0 ? 1 : 0); }
        const int a = w2::indexVon("w2kopie_1");
        const int b = w2::indexVon("w2kopie_2");
        const int c = w2::indexVon("w2kopie_3");
        const int innen = (a >= 0 && b >= 0 && w2::kante(a, b) ? 1 : 0) + (b >= 0 && c >= 0 && w2::kante(b, c) ? 1 : 0) +
                          (a >= 0 && c >= 0 && w2::kante(a, c) ? 1 : 0);
        diag::detail("Knopftest: W2 Umbenennen - alte Namen noch " + std::to_string(alt) + ", innen " + std::to_string(innen) + " Kanten");
        return a >= 0 && b >= 0 && c >= 0 && alt == 0 && innen >= 2 && g_app->entUndo.size() == *tiefe + 1;
    }));
    // Kontextmenue: Loeschen
    add(tu("W2 Kontextmenue: die drei Kopien bleiben gewaehlt", [=] {
        *tiefe = g_app->entUndo.size();
        *vorZahl = g_app->map.entities.size();
    }));
    add(klickAn("W2 Kontextmenue: Rechtsklick in die Ansicht", [] { return w2::ansichtMitte(); }, 1));
    add(pause(0.3));
    add(foto("w2_kontextmenue"));
    add(klick(tr(Str::EntDelete), "##Popup"));
    add(pause(0.3));
    add(pruefe("W2 Kontextmenue: Loeschen nimmt die drei samt Verweisen weg, ein Schritt", [=] {
        return g_app->map.entities.size() + 3U == *vorZahl && w2::indexVon("w2kopie_1") < 0 && w2::verweiseAuf("w2kopie_2") == 0 &&
               g_app->entUndo.size() == *tiefe + 1;
    }));

    add(tu("W2: alles zurueck", [=] {
        while (g_app->entUndo.size() > *tiefe0) { (void)entityRueckgaengig(); }
    }));
    add(pause(0.3));
    add(pruefe("W2: Rueckgaengig bis zum Anfang - so viele Wegpunkte wie vorher", [=] {
        std::size_t n = 0;
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) { n += w2::istWp(static_cast<int>(i)) ? 1U : 0U; }
        return n == *wp0;
    }));
    add(tu("W2: Fenster zu", [] {
        g_app->wegFensterOffen = false;
        g_app->zeigeWegnetz = false;
    }));
    return s;
}
