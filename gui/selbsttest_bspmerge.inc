// selbsttest_bspmerge.inc - rc609 B1: Brush-Entities in fertige Karten einfuegen (BHED_EDITORTEST=bspmerge;
// die Bilder des Handbuchs mit BHED_EDITORTEST=handbuch BHED_HB_NUR=bspmerge)
//
// Eingebunden an genau dieser Stelle von gui/selbsttest.cpp (im selben Namensraum, nach den Helfern der
// Karten-Editor-Laeufe: stufe2::testPinsel, stufe3::q3map2Bereitstellen, stufe8::warteSek, hb::*).
//
// Eine fertige Karte aus den Spielordnern (BHED_BSPMERGE_KARTE, sonst maps/mp/duel1.bsp) als lose Datei in
// <Lauf>\bspmerge\mod\maps\...\bm_<name>.bsp:
//   A  Karten-Editor: File > Open .bsp... mit der losen Datei
//   B  ein trigger_multiple (textures/system/trigger) 160 vor einem Spielerstart mit freiem Weg (CM-Port,
//      Kasten des Spielers) und sein Ziel, ein target_print - 1 neue Brush-Entity
//   C  Build > In fertige Karte einfuegen: q3map2 -bsp baut den Teil, die lose Datei bekommt ein neues *N,
//      .bak ist der Stand vorher, die Welt-Lumps Byte-gleich, Entity-Lump mit "model" "*N"
//   D  ein zweites Mal: nichts mehr neu
//   E  Mission-Tab: die Datei laden - trigger_multiple mit *N in der Entity-Liste, das Modell der Vorschau
//      (geo.models) liegt am Kasten des Triggers, der CM-Port hat dort CONTENTS_TRIGGER
//   F  Karten-Editor: die gespeicherte Datei wieder oeffnen - ein trigger_multiple mehr als vorher, keiner neu
// Fuer den Spielvergleich (Movie Duels per Popen, nicht im Programm): <Lauf>\bspmerge\spiel.txt mit Karte und Startpunkt.
namespace bm {

namespace ke = karteneditor;

struct Zustand {
    std::string karte;          // maps/... im Spiel
    std::string relativ;        // maps/.../bm_<name>.bsp
    std::string wurzel;
    std::string lose;           // <wurzel>\mod\<relativ>
    std::string vorher;         // die lose Datei vor dem Einfuegen
    bool bereit = false;
    float start[3]{};
    float yaw = 0.0F;
    float tmin[3]{};
    float tmax[3]{};
    int modelleVorher = 0;
    int triggerVorher = 0;
};

std::string lies(const std::string& p) {
    std::ifstream f(::bhed::utf8Pfad(p), std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

int zaehleKlasse(const std::string& klasse) {
    int n = 0;
    for (const auto& e : ke::editor().karte.entitaeten()) { n += e->klassenname() == klasse ? 1 : 0; }
    return n;
}

// Ein Spielerstart (info_player_*), von dem aus der Kasten des Spielers (bg_public.h: -15 -15 DEFAULT_MINS_2 -24 bis
// 15 15 40) in einer der acht Richtungen ab seinem Blick 256 weit frei laufen kann (CM_BoxTrace mit CONTENTS_SOLID |
// CONTENTS_PLAYERCLIP). Gestartet wird 9 ueber dem Ursprung wie im Spiel (SelectSpawnPoint: origin[2] += 9, MD9
// code/game/g_client.cpp:543, codemp/game/g_client.c:733). Der Trigger liegt 160 voraus (80 x 80 x 104, ganzzahlig).
bool stelleSuchen(const std::string& bsp, Zustand& z) {
    KollisionsKarte k;
    karte::BspDatei d;
    if (!leseKollision(bsp, k) || !karte::bspLesen(bsp, d)) { return false; }
    z.modelleVorher = static_cast<int>(d.lumps[7].size() / 40);
    std::vector<MapEntity> ents;
    parseEntities(karte::bspEntitaetenText(d), ents);
    for (const MapEntity& e : ents) {
        if (e.classname.rfind("info_player_", 0) != 0 || e.classname == "info_player_intermission") { continue; }
        float o[3] = {0.0F, 0.0F, 0.0F};
        if (std::sscanf(e.origin.c_str(), "%f %f %f", &o[0], &o[1], &o[2]) != 3) { continue; }
        o[2] += 9.0F;
        float blick = 0.0F;
        if (const std::string* a = e.find("angle")) { blick = static_cast<float>(std::atof(a->c_str())); }
        for (int r = 0; r < 8; ++r) {
            const float yaw = blick + 45.0F * static_cast<float>(r);
            const float rad = yaw * 3.14159265F / 180.0F;
            const float dir[3] = {std::cos(rad), std::sin(rad), 0.0F};
            const float mins[3] = {-15.0F, -15.0F, -24.0F};
            const float maxs[3] = {15.0F, 15.0F, 40.0F};
            const float ende[3] = {o[0] + dir[0] * 256.0F, o[1] + dir[1] * 256.0F, o[2]};
            const KollisionsSpur s = kollisionsSpur(k, o, mins, maxs, ende, 0x00000011U);
            if (s.startsolid || s.allsolid || s.fraction < 1.0F) { continue; }
            const float c[3] = {std::round(o[0] + dir[0] * 160.0F), std::round(o[1] + dir[1] * 160.0F), std::round(o[2])};
            for (int i = 0; i < 3; ++i) { z.start[i] = o[i]; }
            z.yaw = yaw;
            z.tmin[0] = c[0] - 40.0F;
            z.tmin[1] = c[1] - 40.0F;
            z.tmin[2] = c[2] - 40.0F;
            z.tmax[0] = c[0] + 40.0F;
            z.tmax[1] = c[1] + 40.0F;
            z.tmax[2] = c[2] + 64.0F;
            return true;
        }
    }
    return false;
}

// Die Karte aus den Spielordnern als lose Datei im Laufordner; Stelle fuer den Trigger.
void vorbereiten(Zustand& z) {
    const char* kk = std::getenv("BHED_BSPMERGE_KARTE");
    z.karte = kk != nullptr && *kk != 0 ? kk : "maps/mp/duel1.bsp";
    const std::size_t s = z.karte.rfind('/');
    z.relativ = z.karte.substr(0, s + 1) + "bm_" + z.karte.substr(s + 1);
    std::error_code ec;
    z.wurzel = (std::filesystem::current_path() / "bspmerge").string();
    std::filesystem::remove_all(::bhed::utf8Pfad(z.wurzel), ec);
    // mit '/' wie die Herkunft im Karten-Editor (bspOeffnenDatei: mitSchraeg)
    z.lose = z.wurzel + "/mod/" + z.relativ;
    std::replace(z.lose.begin(), z.lose.end(), '\\', '/');
    std::filesystem::create_directories(std::filesystem::path(::bhed::utf8Pfad(z.lose)).parent_path(), ec);
    std::string bytes;
    if (!readFromArchives(z.karte, bytes)) {
        diag::detail("Knopftest: B1 - " + z.karte + " nicht in den Spielordnern");
        return;
    }
    std::ofstream(::bhed::utf8Pfad(z.lose), std::ios::binary) << bytes;
    z.bereit = stelleSuchen(bytes, z);
    diag::detail("Knopftest: B1 - " + z.karte + ", " + std::to_string(z.modelleVorher) + " Modelle, Trigger " + std::to_string(z.tmin[0]) + " " +
                 std::to_string(z.tmin[1]) + " " + std::to_string(z.tmin[2]) + " .. " + std::to_string(z.tmax[0]) + " " +
                 std::to_string(z.tmax[1]) + " " + std::to_string(z.tmax[2]));
}

// Karten-Editor auf, Spieldaten, bhed_q3map2 neben das Programm, die lose Datei oeffnen.
void editorOeffnen(const Zustand& z) {
    g_app->leftMode = 3;
    stufe3::q3map2Bereitstellen();
    (void)ke::bspOeffnenDatei(z.lose);
}

// Trigger und Ziel bauen (wie im Editor: neue Brush-Entity mit einem Brush, Punkt-Entity).
void triggerBauen(const Zustand& z) {
    ke::Editor& e = ke::editor();
    e.karte.auswahl.leeren();
    auto trig = e.karte.neueEntitaet("trigger_multiple", true);
    e.karte.entitaetEinfuegen(trig);
    trig->setzen("target", "bm_drucker");
    trig->setzen("wait", "1");
    (void)stufe2::testPinsel(karte::Vektor3f(z.tmin[0], z.tmin[1], z.tmin[2]), karte::Vektor3f(z.tmax[0], z.tmax[1], z.tmax[2]),
                             "textures/system/trigger", trig);
    auto druck = e.karte.neueEntitaet("target_print", false);
    e.karte.entitaetEinfuegen(druck);
    druck->setzen("targetname", "bm_drucker");
    druck->setzen("message", "bspmerge trigger");
    char o[96];
    std::snprintf(o, sizeof(o), "%g %g %g", (z.tmin[0] + z.tmax[0]) * 0.5F, (z.tmin[1] + z.tmax[1]) * 0.5F, z.tmax[2] + 16.0F);
    druck->setzen("origin", o);
    e.karte.auswahl.waehlen(trig, trig->kinder().front());
    ke::auswahlGeaendert();
    e.kamera.setzeUrsprung(karte::Vektor3f(z.start[0], z.start[1], z.start[2] + 48.0F));
    e.kamera.setzeWinkel(karte::Vektor3f(-10.0F, z.yaw, 0.0F));
    ++e.sichtStand;
}

}  // namespace bm

std::vector<Schritt> bspmergeSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<bm::Zustand>();
    namespace ke = karteneditor;

    add(tu("B1: Karte als lose Datei in den Laufordner, Stelle fuer den Trigger", [z] { bm::vorbereiten(*z); }));
    add(pruefe("B1: Karte da, Spielerstart mit freiem Weg gefunden", [z] { return z->bereit; }));
    add(tu("B1: Karten-Editor, Spielordner", [] {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        g_app->leftMode = 3;
    }));
    add(warteBis("B1: Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    add(tu("B1 A: Karten-Editor, File > Open .bsp... mit der losen Datei", [z] {
        if (z->bereit) { bm::editorOeffnen(*z); }
    }));
    add(warteBis("B1 A: geoeffnet", [z] { return !z->bereit || (ke::editor().bspHerkunft.aktiv && ke::editor().bspHerkunft.quelle == z->lose); }, 3600));
    add(stufe8::warteSek("B1 A: Rueckgewinnung fertig", [] { return ke::bspRueckStand().stand == 0 && !ke::bauLaeuft(); }, 1500));
    add(pruefe("B1 A: die lose Datei ist offen, keine Brush-Entity neu", [z] {
        z->triggerVorher = bm::zaehleKlasse("trigger_multiple");
        diag::detail("Knopftest: B1 A - " + std::to_string(ke::editor().karte.entitaeten().size()) + " Entities, trigger_multiple " +
                     std::to_string(z->triggerVorher));
        return ke::editor().bspHerkunft.aktiv && ke::bspNeuePinselEntitaeten() == 0;
    }));
    add(tu("B1 B: trigger_multiple vor dem Spielerstart, target_print als Ziel", [z] { bm::triggerBauen(*z); }));
    add(pruefe("B1 B: eine Brush-Entity neu", [] { return ke::bspNeuePinselEntitaeten() == 1; }));
    add(tu("B1 C: Stand vorher merken", [z] {
        z->vorher = bm::lies(z->lose);
        ke::editor().bspEinfuegenFertig.clear();
    }));
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.3));
    add(foto("bspmerge_baumenue"));
    add(klick(std::string(ke::trK(Str::KeBmEinfuegen)) + "##baueinfuegen", "", 0, true));
    add(warteBis("B1 C: eingefuegt und gespeichert", [] { return !ke::bauLaeuft() && !ke::editor().bspEinfuegenFertig.empty(); }, 3600));
    add(pruefe("B1 C: lose Datei mit neuem *N, .bak = vorher, Welt-Lumps Byte-gleich, Entity-Lump mit model *N", [z] {
        const std::string lose = bm::lies(z->lose);
        const std::string bak = bm::lies(z->lose + ".bak");
        karte::BspDatei a;
        karte::BspDatei b;
        if (!karte::bspLesen(z->vorher, a) || !karte::bspLesen(lose, b)) { return false; }
        int gleich = 0;
        for (std::size_t i : {3U, 5U, 10U, 11U, 12U, 13U, 14U, 15U, 16U, 17U}) { gleich += i < a.lumps.size() && a.lumps[i] == b.lumps[i] ? 1 : 0; }
        const int modelle = static_cast<int>(b.lumps[7].size() / 40);
        const std::string ent = karte::bspEntitaetenText(b);
        const std::string n = "*" + std::to_string(z->modelleVorher);
        const std::string& h = ke::editor().bspHerkunft.ordner;
        const bool kopie = bm::lies(h + "/" + ke::editor().bspHerkunft.stamm + "_original.bsp") == lose;
        std::string meld;
        for (const std::string& m : ke::editor().bspEinfuegenMeldungen) { meld += " | " + m; }
        diag::detail("Knopftest: B1 C - " + ke::editor().bspEinfuegenFertig + ", Modelle " + std::to_string(z->modelleVorher) + " -> " +
                     std::to_string(modelle) + ", Welt-Lumps gleich " + std::to_string(gleich) + "/10, lose " + std::to_string(lose.size()) +
                     " Bytes, .bak " + std::to_string(bak.size()) + meld);
        return bak == z->vorher && gleich == 10 && modelle == z->modelleVorher + 1 && ke::editor().bspEinfuegenFertig == n + " " + n &&
               ent.find("\"classname\" \"trigger_multiple\"\n\"target\" \"bm_drucker\"") != std::string::npos &&
               ent.find("\"model\" \"" + n + "\"") != std::string::npos && ent.find("\"targetname\" \"bm_drucker\"") != std::string::npos && kopie &&
               std::none_of(ke::editor().bspEinfuegenMeldungen.begin(), ke::editor().bspEinfuegenMeldungen.end(),
                            [](const std::string& m) { return m.rfind("fehler", 0) == 0; });
    }));
    add(pruefe("B1 D: danach ist nichts mehr neu (kein zweites Einfuegen derselben Entities)", [] { return ke::bspNeuePinselEntitaeten() == 0; }));

    // --- E: Mission-Tab ---------------------------------------------------------------------------
    add(tu("B1 E: Mission-Tab, die gespeicherte Datei laden", [z] {
        g_app->leftMode = 1;
        karteDateiLaden(z->lose);
    }));
    add(warteBis("B1 E: geladen", [z] { return g_app->map.path == z->lose && !g_app->geo.empty(); }, 1800));
    add(pruefe("B1 E: trigger_multiple mit *N in der Entity-Liste, Modell der Vorschau am Kasten, CM-Port CONTENTS_TRIGGER", [z] {
        const std::string n = "*" + std::to_string(z->modelleVorher);
        bool inListe = false;
        for (const MapEntity& e : g_app->map.entities) {
            const std::string* m = e.find("model");
            inListe = inListe || (e.classname == "trigger_multiple" && m != nullptr && *m == n);
        }
        const auto nr = static_cast<std::size_t>(z->modelleVorher);
        bool ort = g_app->geo.models.size() == nr + 1;
        float d = 0.0F;
        for (int i = 0; ort && i < 3; ++i) {
            d = std::max({d, std::fabs(g_app->geo.models[nr].mins[i] - z->tmin[i]), std::fabs(g_app->geo.models[nr].maxs[i] - z->tmax[i])});
        }
        KollisionsKarte k;
        const float mitte[3] = {(z->tmin[0] + z->tmax[0]) * 0.5F, (z->tmin[1] + z->tmax[1]) * 0.5F, (z->tmin[2] + z->tmax[2]) * 0.5F};
        const std::uint32_t c = leseKollision(bm::lies(z->lose), k) ? kollisionsInhalt(k, mitte, z->modelleVorher) : 0U;
        diag::detail("Knopftest: B1 E - Liste " + std::string(inListe ? "ja" : "nein") + ", Modelle " + std::to_string(g_app->geo.models.size()) +
                     ", Abweichung Kasten " + std::to_string(d) + ", Inhalt " + std::to_string(c));
        return inListe && ort && d <= 0.01F && (c & 0x400U) != 0U;
    }));
    add(tu("B1: Daten fuer den Spielvergleich (spiel.txt)", [z] {
        std::ofstream f(::bhed::utf8Pfad(z->wurzel + "\\spiel.txt"), std::ios::binary);
        char t[512];
        // setviewpos zieht 25 fuer die Augenhoehe ab (Cmd_SetViewpos_f, MD9 code/game/g_cmds.cpp): der Spieler steht am Start (+9)
        std::snprintf(t, sizeof(t), "karte %s\nsetviewpos %g %g %g %g\ntrigger %g %g %g %g %g %g\nmodell %d\n", z->relativ.c_str(), z->start[0],
                      z->start[1], z->start[2] + 25.0F, z->yaw, z->tmin[0], z->tmin[1], z->tmin[2], z->tmax[0], z->tmax[1], z->tmax[2], z->modelleVorher);
        f << t;
    }));

    // --- F: wieder im Karten-Editor oeffnen ---------------------------------------------------------
    add(tu("B1 F: Karten-Editor, die gespeicherte Datei wieder oeffnen", [z] {
        g_app->leftMode = 3;
        (void)ke::bspOeffnenDatei(z->lose);
    }));
    add(warteBis("B1 F: geoeffnet", [z] { return ke::editor().bspHerkunft.aktiv && ke::editor().bspHerkunft.quelle == z->lose; }, 3600));
    add(stufe8::warteSek("B1 F: Rueckgewinnung fertig", [] { return ke::bspRueckStand().stand == 0 && !ke::bauLaeuft(); }, 1500));
    add(pruefe("B1 F: ein trigger_multiple mehr als vorher, keiner neu", [z] {
        const int n = bm::zaehleKlasse("trigger_multiple");
        diag::detail("Knopftest: B1 F - trigger_multiple " + std::to_string(z->triggerVorher) + " -> " + std::to_string(n));
        return n == z->triggerVorher + 1 && ke::bspNeuePinselEntitaeten() == 0;
    }));
    add(tu("B1: zurueck in den Mission-Tab", [] { g_app->leftMode = 1; }));
    return s;
}

// Die Bilder der Seite "Brush-Entities in fertige Karten einfuegen" (BHED_HB_NUR=bspmerge): die Karte als lose
// Datei im Laufordner (kein Benutzerpfad im Bild), der neue Trigger in Kamera und Draufsicht, das Build-Menue mit
// dem Eintrag, die Konsole nach dem Einfuegen.
std::vector<Schritt> bspmergeHandbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<bm::Zustand>();
    namespace ke = karteneditor;
    const auto editor = [] {
        ImGuiWindow* w = karteFenster();
        const ImGuiWindow* haupt = ImGui::FindWindowByName("###main");
        if (w == nullptr) { return ImRect{}; }
        return ImRect{haupt != nullptr ? haupt->Pos.x : w->Pos.x, haupt != nullptr ? haupt->Pos.y : w->Pos.y, w->Rect().Max.x, w->Rect().Max.y};
    };
    add(tu("Handbuch B1: Spielordner, Karten-Editor", [] {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        g_app->leftMode = 3;
    }));
    add(warteBis("Handbuch B1: Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    add(tu("Handbuch B1: Karte als lose Datei, oeffnen", [z] {
        bm::vorbereiten(*z);
        if (z->bereit) { bm::editorOeffnen(*z); }
    }));
    add(warteBis("Handbuch B1: geoeffnet", [z] { return !z->bereit || ke::editor().bspHerkunft.aktiv; }, 3600));
    add(stufe8::warteSek("Handbuch B1: Rueckgewinnung fertig", [] { return ke::bspRueckStand().stand == 0 && !ke::bauLaeuft(); }, 1500));
    add(tu("Handbuch B1: Trigger und Ziel bauen, Draufsicht auf den Trigger", [z] {
        bm::triggerBauen(*z);
        ke::Editor& e = ke::editor();
        e.xy[0].ansicht.typ = karte::Sicht::XY;
        e.xy[0].ansicht.setzeUrsprung(karte::Vektor3f((z->tmin[0] + z->tmax[0]) * 0.5F, (z->tmin[1] + z->tmax[1]) * 0.5F, 0.0F));
        e.xy[0].ansicht.setzeMassstab(0.5F);
        ++e.sichtStand;
    }));
    add(pause(1.0));
    add(hb::fotoTeil("h_bspmerge_trigger", editor));
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.4));
    hb::markiere("h_bspmerge_menue", [] {
        return std::vector<hb::Marke>{hb::Marke(hb::gruppe({std::string(ke::trK(Str::KeBmEinfuegen)) + "##baueinfuegen"}, ""), 1)};
    });
    add(hb::fotoTeil("h_bspmerge_menue", editor));
    add(klick(std::string(ke::trK(Str::KeBmEinfuegen)) + "##baueinfuegen", "", 0, true));
    add(warteBis("Handbuch B1: eingefuegt", [] { return !ke::bauLaeuft() && !ke::editor().bspEinfuegenFertig.empty(); }, 3600));
    add(pause(0.6));
    add(hb::fotoTeil("h_bspmerge_fertig", editor));
    add(tu("Handbuch B1: zurueck in den Mission-Tab", [] { g_app->leftMode = 1; }));
    return s;
}
