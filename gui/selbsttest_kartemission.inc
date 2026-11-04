// selbsttest_kartemission.inc - rc609 B2: Mission <-> Map ohne Verlust (BHED_EDITORTEST=kartemission)
//
// Eingebunden an genau dieser Stelle von gui/selbsttest.cpp (nach selbsttest_karte3bis5.inc:
// stufe3::raumKarte; nach selbsttest_dateiort.inc: dateiort::lies/schreib/zeileAnhaengen).
//
// Alles im eigenen Laufordner (<Lauf>\kartemission\GameData\MD), nie im Spielordner.
//   A  b2raum: Quelle maps/b2raum.map neben maps/b2raum.bsp (lose). Mission mit ungespeicherten
//      Aenderungen (Skripttext, eigene Figur, geaenderter Spielerstart, Kamera, Zeitleiste, Auswahl, offenes
//      Fenster) -> Map-Tab -> "Karte der Mission bearbeiten" -> Saeule ergaenzen, Tuer "tuer" in "tor"
//      umbenennen (absichtlich gebrochener Bezug) -> bauen -> zurueck. Strahl trifft die Saeule, alle
//      Aenderungen sind noch da, der Bezug "tuer" wird gemeldet. Wechseln behaelt beide Staende.
//   A2 dieselbe Saeule versetzt (gleiche Form): Kollision und die Wege der Vorschau (navWeltVorschau) folgen.
//   B  Gegenprobe b2zwei: nur als .bsp in zz_b2.pk3 (keine Quelle) - zurueckgewandelt, gebaut, in den
//      Eintrag der .pk3 geschrieben (die anderen Eintraege Byte-gleich, .pk3.bak), nachgeladen.
namespace kartemission {

namespace ke = karteneditor;

struct Zustand {
    std::string wurzel;
    std::string mod;
    std::string karteA;     // maps/b2raum.map
    std::string bspA;
    std::string skriptA;
    std::string pk3;
    std::string bspB;       // die gebaute b2zwei.bsp (vor dem Packen)
    std::map<std::string, std::string> eintraege;
    bool bereit = false;
    // Stand der Mission vor dem Wechsel
    Camera cam;
    double playMs = 0.0;
    std::string eigeneName;
    int start = -1;
    std::size_t flaechenVorher = 0;
    // Stand des Map-Tabs
    karte::Vektor3f kameraKarte{0, 0, 0};
    std::size_t auswahlKarte = 0;
    std::shared_ptr<karte::Pinsel> saeuleA;
    std::size_t flaechenA = 0;
};

std::string mapMitRelais(const std::string& extra) {
    // raumKarte (Stufe 3): Raum mit Mittelwand, Saeule, Patch, func_door "tuer"; dazu ein Relais auf "tuer"
    return stufe3::raumKarte(false) + "// entity 5\n{\n\"classname\" \"target_relay\"\n\"targetname\" \"relais\"\n\"target\" \"tuer\"\n" +
           "\"origin\" \"-400 200 24\"\n}\n" + extra;
}

int bauIndex(const char* name) {
    const auto& v = ke::editor().bauProjekt.eintraege;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (v[i].name == name) { return static_cast<int>(i); }
    }
    return -1;
}

void bauen() {
    ke::Editor& e = ke::editor();
    e.bau.ueberwachen = false;   // kein Port 39000 (parallele Laeufe)
    e.bau.spielStarten = false;
    e.optimierer.nachBau = false;
    ke::bauMenueSicherstellen();
    if (const int i = bauIndex("[Test] Quick and Crude"); i >= 0) { ke::bauStarten(static_cast<std::size_t>(i)); }
}

// Ein Kasten in der Welt des Map-Tabs (wie Brush > Brush ziehen: ein Undo-Schritt).
std::shared_ptr<karte::Pinsel> saeule(const karte::Vektor3f& mitte, const karte::Vektor3f& halb) {
    ke::Editor& e = ke::editor();
    e.karte.undo.start();
    karte::TexturProjektion proj;
    karte::texdefStandard(karte::TexdefArt::Quake, proj);
    auto p = std::make_shared<karte::Pinsel>();
    e.karte.welt()->kindEinfuegen(p);
    p->quaderBauen(karte::Kasten(mitte, halb), "test/neu", proj, e.karte.kontext);
    e.karte.undo.finish("brushDragNew");
    e.karte.auswahl.leeren();
    e.karte.auswahl.waehlen(e.karte.welt(), p);
    ke::auswahlGeaendert();
    return p;
}

// Denselben Kasten an anderer Stelle (gleiche Form, ein Undo-Schritt).
void saeuleVersetzen(const std::shared_ptr<karte::Pinsel>& p, const karte::Vektor3f& mitte, const karte::Vektor3f& halb) {
    if (!p) { return; }
    ke::Editor& e = ke::editor();
    e.karte.undo.start();
    karte::TexturProjektion proj;
    karte::texdefStandard(karte::TexdefArt::Quake, proj);
    p->quaderBauen(karte::Kasten(mitte, halb), "test/neu", proj, e.karte.kontext);
    e.karte.undo.finish("brushResize");
    ke::auswahlGeaendert();
}

// Dasselbe ueber die Wege der Vorschau (navWeltVorschau: die Spur, mit der die Figuren laufen).
float wegeX(float y, float z) {
    const std::shared_ptr<const NavWelt> w = navWeltVorschau();
    if (!w || !w->spur) { return -1.0F; }
    const NavSpur s = w->spur({100.0F, y, z}, {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F}, {600.0F, y, z}, kNavKeineEntity, kNavSolid);
    return s.fraction < 1.0F ? s.endpos[0] : 9999.0F;
}

// Waere eine Wegpunkt-Kante von x 150 nach x 400 (y 0) begehbar (NavKantenPruefer der geladenen Karte)?
bool kanteFrei() {
    const std::unique_ptr<WegWeltKarte> w = wegWeltKarte();
    return w && w->pruefer && w->pruefer->pruefeOrte({150.0F, 0.0F, 24.0F}, {400.0F, 0.0F, 24.0F}).begehbar;
}

// Die Tuer "tuer" im Map-Tab umbenennen (wie im Entity-Inspektor).
bool tuerUmbenennen(const std::string& neu) {
    ke::Editor& e = ke::editor();
    for (const auto& ent : e.karte.entitaeten()) {
        if (ent->wert("targetname") == "tuer") {
            e.karte.undo.start();
            ent->setzen("targetname", neu);
            e.karte.undo.finish("entitySetKeyValue");
            return true;
        }
    }
    return false;
}

// Wo ein Strahl auf Hoehe z von x=100 nach x=600 die Welt trifft (CM_BoxTrace, CONTENTS_SOLID).
float strahlX(float y, float z) {
    const auto k = geladeneKollision(g_app->map.path);
    if (!k) { return -1.0F; }
    const float a[3] = {100.0F, y, z};
    const float b[3] = {600.0F, y, z};
    const float n[3] = {0.0F, 0.0F, 0.0F};
    const KollisionsSpur s = kollisionsSpur(*k, a, n, n, b, 1U);
    return s.fraction < 1.0F ? s.endpos[0] : 9999.0F;
}

int findeEntity(const char* klasse, const char* schluessel, const std::string& wert) {
    for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
        const MapEntity& e = g_app->map.entities[i];
        const std::string* w = e.find(schluessel);
        if (e.classname == klasse && w != nullptr && *w == wert) { return static_cast<int>(i); }
    }
    return -1;
}

int zaehleEntity(const char* klasse, const char* schluessel, const std::string& wert) {
    int n = 0;
    for (const MapEntity& e : g_app->map.entities) {
        const std::string* w = e.find(schluessel);
        n += (e.classname == klasse && w != nullptr && *w == wert) ? 1 : 0;
    }
    return n;
}

bool gemeldet(const std::string& name) {
    for (const App::Einblendung& b : g_app->einblendungen) {
        if (b.text.find(name) != std::string::npos) { return true; }
    }
    return false;
}

bool camGleich(const Camera& a, const Camera& b) {
    for (int k = 0; k < 3; ++k) {
        if (std::fabs(a.pos[k] - b.pos[k]) > 0.01F || std::fabs(a.angles[k] - b.angles[k]) > 0.01F) { return false; }
    }
    return true;
}

// Der Spielerstart der Testkarten (q3map2 nimmt die Lichter aus dem Entity-Lump, der Start bleibt).
int spielerstart() { return findeEntity("info_player_start", "origin", "-384 128 24"); }

// Mission-Tab: ungespeicherte Aenderungen anlegen und merken.
void missionAendern(Zustand& z, const char* figur, const char* name, const float ort[3]) {
    dateiort::zeileAnhaengen("wait ( 777.000 );\n");
    const int neu = neueEntityAn(figur, ort);
    if (neu >= 0) { (void)entityKeySetzen(neu, "NPC_targetname", name); }
    z.eigeneName = name;
    z.start = spielerstart();
    if (z.start >= 0) { (void)entityKeySetzen(z.start, "angle", "135"); }
    entityAuswahlSetzen(findeEntity(figur, "NPC_targetname", name));
    if (z.start >= 0) { entityAuswahlUmschalten(z.start); }
    g_app->cam.pos[0] = 96.0F;
    g_app->cam.pos[1] = -48.0F;
    g_app->cam.pos[2] = 128.0F;
    g_app->cam.angles[0] = 10.0F;
    g_app->cam.angles[1] = 33.0F;
    g_app->playMs = 400.0;
    g_app->settings.entTextOffen = true;
    z.cam = g_app->cam;
    z.playMs = g_app->playMs;
    z.flaechenVorher = g_app->geo.surfaces.size();
}

// Nach der Rueckkehr: alle Aenderungen der Mission noch da?
std::vector<Schritt> missionPruefen(const std::shared_ptr<Zustand>& z, const std::string& teil, const char* figur) {
    std::vector<Schritt> s;
    s.push_back(pruefe("Kartemission " + teil + ": Skripttext noch da (wait 777), ungespeichert", [] {
        const std::string t = writeScript(g_app->doc.script());
        return t.find("777.000") != std::string::npos;
    }));
    s.push_back(pruefe("Kartemission " + teil + ": eigene Figur genau einmal, Spielerstart angle 135, Entities ungespeichert", [z, figur] {
        const int l = spielerstart();
        const std::string* w = l >= 0 ? g_app->map.entities[static_cast<std::size_t>(l)].find("angle") : nullptr;
        diag::detail("Knopftest: Kartemission - Figur " + std::to_string(zaehleEntity(figur, "NPC_targetname", z->eigeneName)) + "x, Spielerstart angle " +
                     (w != nullptr ? *w : std::string("-")) + ", ungespeichert " + (g_app->entitiesUngespeichert.count(g_app->map.path) != 0U ? "ja" : "nein"));
        return zaehleEntity(figur, "NPC_targetname", z->eigeneName) == 1 && w != nullptr && *w == "135" &&
               g_app->entitiesUngespeichert.count(g_app->map.path) != 0U;
    }));
    s.push_back(pruefe("Kartemission " + teil + ": Kamera und Zeitleiste wie vorher", [z] {
        diag::detail("Knopftest: Kartemission - Kamera " + std::to_string(g_app->cam.pos[0]) + " " + std::to_string(g_app->cam.pos[1]) + " " +
                     std::to_string(g_app->cam.pos[2]) + ", Zeit " + std::to_string(g_app->playMs));
        return camGleich(g_app->cam, z->cam) && std::fabs(g_app->playMs - z->playMs) < 0.01;
    }));
    s.push_back(pruefe("Kartemission " + teil + ": Auswahl wie vorher (Figur, dazu der Spielerstart), Entity-Text offen", [z, figur] {
        const int f = findeEntity(figur, "NPC_targetname", z->eigeneName);
        const int l = spielerstart();
        const std::vector<int> a = entityAuswahl();
        diag::detail("Knopftest: Kartemission - Auswahl " + std::to_string(a.size()) + " (gewaehlt " + std::to_string(g_app->pickedEntity) + ", Figur " +
                     std::to_string(f) + ", Spielerstart " + std::to_string(l) + ")");
        return f >= 0 && l >= 0 && g_app->pickedEntity == f && a.size() == 2 && a[1] == l && g_app->settings.entTextOffen;
    }));
    return s;
}

}  // namespace kartemission

// "Karte der Mission bearbeiten" im Menue File des Map-Tabs (Punkt 2)
std::vector<Schritt> karteDerMissionSchritte(const std::string& teil) {
    namespace ke = karteneditor;
    std::vector<Schritt> s;
    s.push_back(klick(ke::trK(Str::KeLFile), "###main"));
    s.push_back(pause(0.3));
    s.push_back(klick(ke::trK(Str::KmMenue)));
    s.push_back(pause(0.5));
    s.push_back(pruefe("Kartemission " + teil + ": Map-Tab mit der Mission verbunden (Konsole)", [] {
        const std::string muster = ke::trK(Str::KmVerbunden);
        const std::string anfang = muster.substr(0, muster.find("%s"));
        bool da = false;
        for (const ke::KonsolenZeile& z : ke::editor().konsole) { da = da || z.text.rfind(anfang, 0) == 0; }
        return da;
    }));
    return s;
}

// handbuch: BHED_EDITORTEST=handbuch mit BHED_HB_NUR=kartemission - Teil A bis zur Liste, mit den Bildern
std::vector<Schritt> kartemissionSchritte(bool handbuch) {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<kartemission::Zustand>();
    using namespace kartemission;
    const auto fertig = [](const char* was) { return warteBis(was, [] { return !ke::bauLaeuft(); }, 9000); };

    add(tu("Kartemission: Mod-Ordner im Laufordner, Quelle maps/b2raum.map", [z] {
        std::error_code ec;
        z->wurzel = (std::filesystem::current_path() / "kartemission").string();
        std::filesystem::remove_all(::bhed::utf8Pfad(z->wurzel), ec);
        z->mod = z->wurzel + "\\GameData\\MD";
        const std::string missionen = z->wurzel + "\\missionen";
        std::filesystem::create_directories(::bhed::utf8Pfad(missionen), ec);
        _putenv_s("BHED_MISSIONEN", missionen.c_str());
        g_app->settings.pk3SpeicherWeg = 0;
        g_app->settings.missionsordnerAblauf = false;
        z->karteA = z->mod + "\\maps\\b2raum.map";
        z->bspA = z->mod + "\\maps\\b2raum.bsp";
        z->skriptA = z->mod + "\\scripts\\b2\\intro.txt";
        z->pk3 = z->mod + "\\zz_b2.pk3";
        dateiort::schreib(z->karteA, mapMitRelais(std::string()));
        dateiort::schreib(z->skriptA, "use ( \"tuer\" );\nuse ( \"relais\" );\n");
    }));
    add(klick(tr(Str::ViewMapEditor), "###main"));
    add(pause(0.5));
    add(tu("Kartemission A: b2raum.map oeffnen und bauen ([Test] Quick and Crude)", [z] {
        (void)ke::karteOeffnen(z->karteA);
        ke::editor().bau.zielOrdner.clear();
        bauen();
    }));
    add(fertig("Kartemission A: erster Bau fertig"));
    add(pruefe("Kartemission A: maps/b2raum.bsp gebaut", [z] {
        z->bereit = dateiort::da(z->bspA);
        return z->bereit;
    }));

    // --- Mission mit ungespeicherten Aenderungen -----------------------------------------
    add(klick(tr(Str::ViewMap), "###main"));
    add(tu("Kartemission A: Mission oeffnen (lose .bsp und Skript)", [z] {
        if (!z->bereit) { return; }
        karteDateiLaden(z->bspA);
        ladeSkriptDatei(z->skriptA);
    }));
    add(warteBis("Kartemission A: Karte geladen", [z] { return !z->bereit || g_app->map.path == z->bspA; }, 600));
    add(pause(0.5));
    add(tu("Kartemission A: Skript, eigene Figur, Spielerstart, Kamera, Zeitleiste, Auswahl, Fenster aendern", [z] {
        const float ort[3] = {-400.0F, -150.0F, 24.0F};
        missionAendern(*z, "NPC_Kyle", "b2kyle", ort);
    }));
    add(pause(0.3));
    add(pruefe("Kartemission A: vorher - Strahl und Wege treffen die Ostwand (x 512), Kante x 150 -> 400 frei, Bezug tuer heil", [] {
        const float x = strahlX(0.0F, 100.0F);
        const float w = wegeX(0.0F, 100.0F);
        const bool k = kanteFrei();
        diag::detail("Knopftest: Kartemission A vorher - Strahl bei x " + std::to_string(x) + ", Wege " + std::to_string(w) + ", Kante " + (k ? "frei" : "zu"));
        return std::fabs(x - 512.0F) < 1.0F && std::fabs(w - 512.0F) < 1.0F && k && findeEntity("func_door", "targetname", "tuer") >= 0;
    }));

    // --- Map-Tab: Karte der Mission, Saeule, Bezug brechen, bauen -------------------------
    add(klick(tr(Str::ViewMapEditor), "###main"));
    add(pause(0.4));
    for (Schritt& x : karteDerMissionSchritte("A")) { add(std::move(x)); }
    add(pruefe("Kartemission A: die Quelle maps/b2raum.map ist offen (neben der .bsp der Mission)", [z] { return ke::editor().name == z->karteA; }));
    if (handbuch) {
        add(klick(ke::trK(Str::KeLFile), "###main"));
        add(pause(0.6));
        hb::markiere("h_kartemission_menue", [] { return std::vector<hb::Marke>{hb::Marke(hb::gruppe({ke::trK(Str::KmMenue)}))}; });
        add(hb::fotoTeil("h_kartemission_menue", [] {   // vom linken Rand bis hinter das Menue: mindestens 1920 breit
            const ImGuiWindow* haupt = ImGui::FindWindowByName("###main");
            const ImRect m = obersterPopupRahmen();
            const float x0 = haupt != nullptr ? haupt->Pos.x : 0.0F;
            const float y0 = haupt != nullptr ? haupt->Pos.y : 0.0F;
            return ImRect{x0, y0, std::max(m.Max.x + 220.0F, x0 + 1920.0F), std::max(m.Max.y + 20.0F, y0 + 900.0F)};
        }));
        add(taste(ImGuiKey_Escape, "Esc"));
        add(pause(0.3));
    }
    add(tu("Kartemission A: Map-Tab - Kamera setzen, Logik-Fenster offen, Saeule ergaenzen, Tuer umbenennen", [z] {
        ke::Editor& e = ke::editor();
        e.kamera.ursprung = karte::Vektor3f(-64, 32, 96);
        ++e.sichtStand;
        e.logik.offen = true;
        z->saeuleA = saeule(karte::Vektor3f(232, 0, 100), karte::Vektor3f(32, 32, 100));
        (void)tuerUmbenennen("tor");
        z->kameraKarte = e.kamera.ursprung;
        z->auswahlKarte = e.karte.auswahl.anzahl();
    }));
    add(pause(0.3));
    add(tu("Kartemission A: Logik-Fenster klein nach rechts unten (die Modusleiste bleibt frei)", [] {
        const ImVec2 d = ImGui::GetIO().DisplaySize;
        const float fs = ImGui::GetFontSize();
        ImGui::SetWindowSize("###kartelogik", ImVec2{20.0F * fs, 12.0F * fs});
        ImGui::SetWindowPos("###kartelogik", ImVec2{d.x - 21.0F * fs, d.y - 14.0F * fs});
    }));
    add(pause(0.3));
    add(tu("Kartemission A: bauen", [] { bauen(); }));
    add(fertig("Kartemission A: Bau mit Saeule fertig"));

    // --- zurueck in den Mission-Tab -----------------------------------------------------------
    add(klick(tr(Str::ViewMap), "###main"));
    add(pause(0.6));
    add(pruefe("Kartemission A: neue Geometrie - Strahl und Wege treffen die Saeule (x 200), die Wegpunkt-Kante ist zu", [z] {
        const float x = strahlX(0.0F, 100.0F);
        const float w = wegeX(0.0F, 100.0F);
        const bool k = kanteFrei();
        z->flaechenA = g_app->geo.surfaces.size();
        diag::detail("Knopftest: Kartemission A - Strahl bei x " + std::to_string(x) + ", Wege " + std::to_string(w) + ", Kante " + (k ? "frei" : "zu") +
                     ", Flaechen " + std::to_string(z->flaechenVorher) + " -> " + std::to_string(g_app->geo.surfaces.size()) + ", Vorschau " +
                     (g_app->mapBereit ? "bereit" : "nicht bereit"));
        return std::fabs(x - 200.0F) < 1.0F && std::fabs(w - 200.0F) < 1.0F && !k && g_app->geo.surfaces.size() > z->flaechenVorher && g_app->mapBereit;
    }));
    add(pruefe("Kartemission A: Entity-Liste der neuen Karte - Tuer heisst jetzt tor", [] { return findeEntity("func_door", "targetname", "tor") >= 0; }));
    for (Schritt& x : missionPruefen(z, "A", "NPC_Kyle")) { add(std::move(x)); }
    add(pruefe("Kartemission A: gebrochener Bezug gemeldet - Skript use(\"tuer\") und Relais target tuer", [] { return gemeldet("tuer"); }));
    add(pruefe("Kartemission A: die Liste nennt beide Stellen (Skript und Relais), Fenster offen", [] {
        const KarteMissionStand& st = karteMissionStand();
        int stellen = 0;
        for (const KarteMissionBefund& b : st.befunde) {
            stellen += (b.art == KarteMissionBefund::Art::NameFehlt && b.was == "tuer") ? 1 : 0;
            diag::detail("Knopftest: Kartemission A - Befund " + b.was + " @ " + b.wo);
        }
        return stellen == 2 && st.fensterOffen;
    }));
    add(pruefe("Kartemission A: die Liste liegt in der Mitte (Modusleiste links und Menue oben bleiben frei)", [] {
        const ImGuiWindow* w = ImGui::FindWindowByName("###kartemission");
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        if (w != nullptr) {
            diag::detail("Knopftest: Kartemission A - Liste " + std::to_string(w->Pos.x) + " " + std::to_string(w->Pos.y) + " " + std::to_string(w->Size.x) + " x " +
                         std::to_string(w->Size.y) + ", Hauptfenster " + std::to_string(vp->WorkSize.x) + " x " + std::to_string(vp->WorkSize.y));
        }
        return w != nullptr && w->Pos.x >= vp->WorkPos.x + vp->WorkSize.x * 0.2F && w->Pos.y >= vp->WorkPos.y + vp->WorkSize.y * 0.2F &&
               w->Pos.x + w->Size.x <= vp->WorkPos.x + vp->WorkSize.x * 0.8F && w->Pos.y + w->Size.y <= vp->WorkPos.y + vp->WorkSize.y * 0.8F;
    }));
    add(pruefe("Kartemission A: .bsp.bak ist die alte Karte", [z] { return dateiort::da(z->bspA + ".bak"); }));
    if (handbuch) {
        add(tu("Handbuch: Entity-Text zu (die 3D-Ansicht frei), Maus auf die Statuszeile", [] {
            g_app->settings.entTextOffen = false;
            const ImGuiViewport* vp = ImGui::GetMainViewport();
            ImGui::GetIO().AddMousePosEvent(vp->WorkPos.x + vp->WorkSize.x * 0.3F, vp->WorkPos.y + vp->WorkSize.y - 4.0F);
        }));
        add(pause(1.0));
        add(hb::fotoTeil("h_kartemission_liste", [] {   // das ganze Hauptfenster: Ansicht, Modusleiste und die Liste
            const ImGuiViewport* vp = ImGui::GetMainViewport();
            return ImRect{vp->WorkPos, ImVec2{vp->WorkPos.x + vp->WorkSize.x, vp->WorkPos.y + vp->WorkSize.y}};
        }));
        return s;
    }

    // --- Punkt 4: Wechseln behaelt beide Staende -------------------------------------------------
    add(klick(tr(Str::ViewMapEditor), "###main"));
    add(pause(0.4));
    add(pruefe("Kartemission: Map-Tab nach dem Hin und Her - Kamera, Auswahl, Logik-Fenster wie vorher", [z] {
        const ke::Editor& e = ke::editor();
        const karte::Vektor3f d = e.kamera.ursprung - z->kameraKarte;
        diag::detail("Knopftest: Kartemission - Map-Tab Kamera " + std::to_string(e.kamera.ursprung.x()) + " " + std::to_string(e.kamera.ursprung.y()) + " " +
                     std::to_string(e.kamera.ursprung.z()) + ", Auswahl " + std::to_string(e.karte.auswahl.anzahl()));
        return std::fabs(d.x()) < 0.01F && std::fabs(d.y()) < 0.01F && std::fabs(d.z()) < 0.01F && e.karte.auswahl.anzahl() == z->auswahlKarte && e.logik.offen;
    }));
    add(klick(tr(Str::ViewMap), "###main"));
    add(pause(0.4));
    add(pruefe("Kartemission: Mission-Tab nach dem zweiten Wechsel - Kamera und Zeit wie vorher", [z] {
        return camGleich(g_app->cam, z->cam) && std::fabs(g_app->playMs - z->playMs) < 0.01;
    }));

    // --- A2: dieselbe Saeule versetzt (gleiche Form) - Geometrie und Wege folgen ---------------
    add(klick(tr(Str::ViewMapEditor), "###main"));
    add(pause(0.3));
    add(tu("Kartemission A2: Saeule nach x 296..360 versetzen, bauen", [z] {
        saeuleVersetzen(z->saeuleA, karte::Vektor3f(328, 0, 100), karte::Vektor3f(32, 32, 100));
        bauen();
    }));
    add(fertig("Kartemission A2: Bau fertig"));
    add(klick(tr(Str::ViewMap), "###main"));
    add(pause(0.6));
    add(pruefe("Kartemission A2: Strahl und Wege der Vorschau treffen die versetzte Saeule (x 296)", [z] {
        const float x = strahlX(0.0F, 100.0F);
        const float w = wegeX(0.0F, 100.0F);
        diag::detail("Knopftest: Kartemission A2 - Strahl bei x " + std::to_string(x) + ", Wege " + std::to_string(w) + ", Flaechen " +
                     std::to_string(z->flaechenA) + " -> " + std::to_string(g_app->geo.surfaces.size()));
        return std::fabs(x - 296.0F) < 1.0F && std::fabs(w - 296.0F) < 1.0F;
    }));

    // --- B: Gegenprobe - Karte nur als .bsp in einer .pk3 ---------------------------------------
    add(klick(tr(Str::ViewMapEditor), "###main"));
    add(pause(0.3));
    add(tu("Kartemission B: zweite Karte b2zwei bauen (Quelle ausserhalb des Mod-Ordners)", [z] {
        const std::string q = z->wurzel + "\\quelle\\b2zwei.map";
        dateiort::schreib(q, mapMitRelais("// entity 6\n{\n\"classname\" \"light\"\n\"origin\" \"256 -160 160\"\n\"light\" \"250\"\n}\n"));
        z->bspB = z->wurzel + "\\quelle\\b2zwei.bsp";
        ke::editor().logik.offen = false;
        (void)ke::karteOeffnen(q);
        bauen();
    }));
    add(fertig("Kartemission B: b2zwei gebaut"));
    add(tu("Kartemission B: zz_b2.pk3 packen (maps/b2zwei.bsp, levelshot, Skript)", [z] {
        const std::string bsp = dateiort::lies(z->bspB);
        std::string fehler;
        z->bereit = !bsp.empty() && writePk3(z->pk3, {{"maps/b2zwei.bsp", bsp}, {"levelshots/b2zwei.jpg", std::string(4096, '\x42')},
                                                       {"scripts/b2/zwei.txt", "use ( \"tuer\" );\n"}},
                                             &fehler);
        z->eintraege = dateiort::eintraege(z->pk3);
        diag::detail("Knopftest: Kartemission B - pk3 " + std::to_string(z->eintraege.size()) + " Eintraege " + fehler);
    }));
    add(klick(tr(Str::ViewMap), "###main"));
    add(tu("Kartemission B: Mission aus zz_b2.pk3 oeffnen", [z] {
        if (z->bereit) { (void)karteAusArchivOeffnen(z->pk3, "maps/b2zwei.bsp"); }
    }));
    add(warteBis("Kartemission B: Karte geladen", [z] { return !z->bereit || g_app->map.path == "maps/b2zwei.bsp"; }, 600));
    add(pause(0.5));
    add(tu("Kartemission B: ungespeicherte Aenderungen", [z] {
        const float ort[3] = {-400.0F, 150.0F, 24.0F};
        missionAendern(*z, "NPC_Jan", "b2jan", ort);
    }));
    add(pruefe("Kartemission B: vorher - Strahl bei y -150 trifft die Ostwand", [] { return std::fabs(strahlX(-150.0F, 150.0F) - 512.0F) < 1.0F; }));
    add(klick(tr(Str::ViewMapEditor), "###main"));
    add(pause(0.3));
    for (Schritt& x : karteDerMissionSchritte("B")) { add(std::move(x)); }
    add(fertig("Kartemission B: zurueckgewandelt"));
    add(pruefe("Kartemission B: die .bsp aus der .pk3 ist zurueckgewandelt offen", [] {
        return ke::editor().bspHerkunft.aktiv && ke::editor().bspHerkunft.quelle == "maps/b2zwei.bsp";
    }));
    add(tu("Kartemission B: misc_model-Suche abbrechen (der Raum hat keine Modelle)", [] { ke::bspRueckAbbrechen(); }));
    add(warteBis("Kartemission B: misc_model-Suche beendet", [] { return !ke::bspRueckLaeuft(); }, 1200));
    add(tu("Kartemission B: Saeule bei x 300..340, y -182..-118 ergaenzen, bauen", [] {
        saeule(karte::Vektor3f(320, -150, 100), karte::Vektor3f(20, 32, 100));
        bauen();
    }));
    add(fertig("Kartemission B: Bau fertig"));
    add(klick(tr(Str::ViewMap), "###main"));
    add(pause(0.6));
    add(pruefe("Kartemission B: neue Geometrie - Strahl trifft die Saeule (x 300)", [] {
        const float x = strahlX(-150.0F, 150.0F);
        diag::detail("Knopftest: Kartemission B - Strahl bei x " + std::to_string(x));
        return std::fabs(x - 300.0F) < 1.0F;
    }));
    add(pruefe("Kartemission B: in den Eintrag der .pk3 gebaut - nur maps/b2zwei.bsp anders, .pk3.bak", [z] {
        const auto jetzt = dateiort::eintraege(z->pk3);
        std::vector<std::string> anders;
        for (const auto& [k, v] : jetzt) {
            if (z->eintraege.count(k) == 0U || z->eintraege.at(k) != v) { anders.push_back(k); }
        }
        diag::detail("Knopftest: Kartemission B - anders: " + dateiort::liste(anders));
        return anders == std::vector<std::string>{"maps/b2zwei.bsp"} && jetzt.size() == z->eintraege.size() && dateiort::da(z->pk3 + ".bak");
    }));
    for (Schritt& x : missionPruefen(z, "B", "NPC_Jan")) { add(std::move(x)); }
    return s;
}
