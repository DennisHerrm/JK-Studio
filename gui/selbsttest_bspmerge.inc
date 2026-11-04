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
//   G  rc609 B13: ein Clip (textures/system/clip) in der Welt hinter dem Trigger und ein func_door mit dem haeufigsten
//      Lightmap-Shader der Karte in einer zweiten freien Richtung; In fertige Karte einfuegen (zwei Durchgaenge: der Teil, dann
//      der Licht-Teil mit der Welt als Schattenwerfer) - Modell 0 hat einen Brush mehr, die Tuer ein *N mit Flaechen und
//      Lightmaps, Baum/VIS/Lichtgitter Byte-gleich, Welt-Lightmaps vorn Byte-gleich, CM-Port PLAYERCLIP im Clip
//   H  Vergleichsbild: Karten-Editor und Vorschau (Mission-Tab, die gespeicherte Datei neu geladen) aus dem Auge des Spielers
//      am Start, Blick auf die Tuer; die Vorschau hat die Tuer mit Lightmap (Luxel nicht schwarz)
//   I  Karten-Editor: die Datei wieder oeffnen - der Clip ist ein Welt-Brush, die Tuer da, nichts neu
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
    // rc609 B13
    float yaw2 = 0.0F;          // zweite freie Richtung (Tuer)
    float boden = 0.0F;         // Boden unter dem Start (Kasten des Spielers nach unten)
    float cmin[3]{};            // Clip
    float cmax[3]{};
    float dmin[3]{};            // Tuer
    float dmax[3]{};
    std::string tuerShader;
    std::string vorher2;
    int tuerModell = 0;
    int tuerenVorher = 0;
    int clipsVorher = 0;
};

std::string lies(const std::string& p) {
    std::ifstream f(::bhed::utf8Pfad(p), std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

// Welt-Brushes, deren Seiten alle textures/system/clip tragen
int clipPinsel() {
    int n = 0;
    for (const auto& p : ke::editor().karte.welt()->kinder()) {
        if (p->art() != karte::Primitiv::Art::Pinsel) { continue; }
        const auto& pin = static_cast<const karte::Pinsel&>(*p);
        bool alle = !pin.flaechen().empty();
        for (const auto& f : pin.flaechen()) { alle = alle && f->shader() == "textures/system/clip"; }
        n += alle ? 1 : 0;
    }
    return n;
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
    // rc609 B13: der Shader der Tuer - der mit den meisten Lightmap-Flaechen der Welt (planar, lightmapNum[0] >= 0), kein Himmel
    {
        std::map<int, int> zahl;
        const std::string& fl = d.lumps[13];
        for (std::size_t i = 0; i + 148 <= fl.size(); i += 148) {
            const auto wert = [&fl, i](std::size_t o) {
                std::uint32_t v = 0;
                for (std::size_t b = 0; b < 4; ++b) { v |= static_cast<std::uint32_t>(static_cast<unsigned char>(fl[i + o + b])) << (8 * b); }
                return static_cast<int>(v);
            };
            if (wert(8) == 1 && wert(36) >= 0) { ++zahl[wert(0)]; }
        }
        int beste = 0;
        for (const auto& [sh, n] : zahl) {
            const std::string name = d.lumps[1].substr(static_cast<std::size_t>(sh) * 72, 64).c_str();
            if (n > beste && name.find("sky") == std::string::npos) {
                beste = n;
                z.tuerShader = name;
            }
        }
    }
    std::vector<MapEntity> ents;
    parseEntities(karte::bspEntitaetenText(d), ents);
    const float mins[3] = {-15.0F, -15.0F, -24.0F};
    const float maxs[3] = {15.0F, 15.0F, 40.0F};
    for (const MapEntity& e : ents) {
        if (e.classname.rfind("info_player_", 0) != 0 || e.classname == "info_player_intermission") { continue; }
        float o[3] = {0.0F, 0.0F, 0.0F};
        if (std::sscanf(e.origin.c_str(), "%f %f %f", &o[0], &o[1], &o[2]) != 3) { continue; }
        o[2] += 9.0F;
        float blick = 0.0F;
        if (const std::string* a = e.find("angle")) { blick = static_cast<float>(std::atof(a->c_str())); }
        std::vector<float> frei;
        for (int r = 0; r < 8; ++r) {
            const float yaw = blick + 45.0F * static_cast<float>(r);
            const float rad = yaw * 3.14159265F / 180.0F;
            const float ende[3] = {o[0] + std::cos(rad) * 256.0F, o[1] + std::sin(rad) * 256.0F, o[2]};
            const KollisionsSpur s = kollisionsSpur(k, o, mins, maxs, ende, 0x00000011U);
            if (!s.startsolid && !s.allsolid && s.fraction >= 1.0F) { frei.push_back(yaw); }
        }
        if (frei.size() < 2) { continue; }   // rc609 B13: Trigger und Clip in der ersten, die Tuer in einer zweiten Richtung
        const float unten[3] = {o[0], o[1], o[2] - 512.0F};
        const KollisionsSpur b = kollisionsSpur(k, o, mins, maxs, unten, 0x00000011U);
        z.boden = std::round(b.endpos[2] + mins[2]);
        const float yaw = frei[0];
        const float rad = yaw * 3.14159265F / 180.0F;
        const float dir[3] = {std::cos(rad), std::sin(rad), 0.0F};
        const float c[3] = {std::round(o[0] + dir[0] * 160.0F), std::round(o[1] + dir[1] * 160.0F), std::round(o[2])};
        for (int i = 0; i < 3; ++i) { z.start[i] = o[i]; }
        z.yaw = yaw;
        z.tmin[0] = c[0] - 40.0F;
        z.tmin[1] = c[1] - 40.0F;
        z.tmin[2] = c[2] - 40.0F;
        z.tmax[0] = c[0] + 40.0F;
        z.tmax[1] = c[1] + 40.0F;
        z.tmax[2] = c[2] + 64.0F;
        // Clip 232 voraus (hinter dem Trigger), 64 x 64, 128 hoch ab dem Boden
        const float cc[2] = {std::round(o[0] + dir[0] * 232.0F), std::round(o[1] + dir[1] * 232.0F)};
        z.cmin[0] = cc[0] - 32.0F;
        z.cmin[1] = cc[1] - 32.0F;
        z.cmin[2] = z.boden;
        z.cmax[0] = cc[0] + 32.0F;
        z.cmax[1] = cc[1] + 32.0F;
        z.cmax[2] = z.boden + 128.0F;
        // Tuer 112 voraus in der zweiten Richtung: 16 dick quer zum Weg, 64 breit, 96 hoch
        z.yaw2 = frei[1];
        const float rad2 = z.yaw2 * 3.14159265F / 180.0F;
        const float dc[2] = {std::round(o[0] + std::cos(rad2) * 112.0F), std::round(o[1] + std::sin(rad2) * 112.0F)};
        const bool querX = std::fabs(std::cos(rad2)) >= std::fabs(std::sin(rad2));
        z.dmin[0] = dc[0] - (querX ? 8.0F : 32.0F);
        z.dmax[0] = dc[0] + (querX ? 8.0F : 32.0F);
        z.dmin[1] = dc[1] - (querX ? 32.0F : 8.0F);
        z.dmax[1] = dc[1] + (querX ? 32.0F : 8.0F);
        z.dmin[2] = z.boden;
        z.dmax[2] = z.boden + 96.0F;
        return !z.tuerShader.empty();
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

    // --- G: rc609 B13 - Clip in die Welt, sichtbare Tuer mit Licht ------------------------------------
    add(tu("B13 G: Clip (textures/system/clip) hinter dem Trigger in die Welt, func_door in der zweiten Richtung", [z] {
        ke::Editor& e = ke::editor();
        z->tuerenVorher = bm::zaehleKlasse("func_door");
        z->clipsVorher = bm::clipPinsel();
        e.karte.auswahl.leeren();
        (void)stufe2::testPinsel(karte::Vektor3f(z->cmin[0], z->cmin[1], z->cmin[2]), karte::Vektor3f(z->cmax[0], z->cmax[1], z->cmax[2]),
                                 "textures/system/clip");
        auto tuer = e.karte.neueEntitaet("func_door", true);
        e.karte.entitaetEinfuegen(tuer);
        tuer->setzen("angle", "-1");   // hoch (SP_func_door: MOVER_UP fuer angle -1, G_SetMovedir)
        tuer->setzen("wait", "3");
        (void)stufe2::testPinsel(karte::Vektor3f(z->dmin[0], z->dmin[1], z->dmin[2]), karte::Vektor3f(z->dmax[0], z->dmax[1], z->dmax[2]), z->tuerShader, tuer);
        ke::auswahlGeaendert();
        ++e.sichtStand;
        diag::detail("Knopftest: B13 G - Shader " + z->tuerShader + ", Tuer " + std::to_string(z->dmin[0]) + " " + std::to_string(z->dmin[1]) + " " +
                     std::to_string(z->dmin[2]) + " .. " + std::to_string(z->dmax[0]) + " " + std::to_string(z->dmax[1]) + " " + std::to_string(z->dmax[2]) +
                     ", Clip " + std::to_string(z->cmin[0]) + " " + std::to_string(z->cmin[1]) + " .. " + std::to_string(z->cmax[0]) + " " +
                     std::to_string(z->cmax[1]));
    }));
    add(pruefe("B13 G: ein Welt-Brush und eine Brush-Entity neu", [] { return ke::bspNeueWeltPinsel() == 1 && ke::bspNeuePinselEntitaeten() == 1; }));
    add(tu("B13 G: Stand vorher merken", [z] {
        z->vorher2 = bm::lies(z->lose);
        karte::BspDatei d;
        z->tuerModell = karte::bspLesen(z->vorher2, d) ? static_cast<int>(d.lumps[7].size() / 40) : 0;
        ke::editor().bspEinfuegenFertig.clear();
    }));
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeBmEinfuegen)) + "##baueinfuegen", "", 0, true));
    add(warteBis("B13 G: Teil und Licht-Teil gebaut, eingefuegt und gespeichert", [] { return !ke::bauLaeuft() && !ke::editor().bspEinfuegenFertig.empty(); },
                 3600));
    add(pruefe("B13 G: Modell 0 + 1 Brush, Tuer *N mit Flaechen und Lightmaps, Baum/VIS/Lichtgitter Byte-gleich, Welt-Lightmaps vorn gleich, "
               "CM-Port PLAYERCLIP im Clip",
               [z] {
                   const std::string lose = bm::lies(z->lose);
                   karte::BspDatei a;
                   karte::BspDatei b;
                   if (!karte::bspLesen(z->vorher2, a) || !karte::bspLesen(lose, b)) { return false; }
                   const auto i32 = [](const std::string& s, std::size_t o) {
                       std::uint32_t v = 0;
                       for (std::size_t i = 0; i < 4; ++i) { v |= static_cast<std::uint32_t>(static_cast<unsigned char>(s[o + i])) << (8 * i); }
                       return static_cast<int>(v);
                   };
                   int gleich = 0;
                   for (std::size_t i : {3U, 15U, 16U, 17U}) { gleich += a.lumps[i] == b.lumps[i] ? 1 : 0; }
                   int vorn = 0;
                   for (std::size_t i : {10U, 11U, 13U, 14U}) {
                       vorn += b.lumps[i].size() > a.lumps[i].size() && b.lumps[i].compare(0, a.lumps[i].size(), a.lumps[i]) == 0 ? 1 : 0;
                   }
                   const auto n = static_cast<std::size_t>(z->tuerModell);
                   const bool modelle = b.lumps[7].size() / 40 == n + 1 && i32(b.lumps[7], 36) == i32(a.lumps[7], 36) + 1;
                   const int flaechen = modelle ? i32(b.lumps[7], n * 40 + 28) : 0;
                   const int seitenA = static_cast<int>(a.lumps[14].size() / 49152);
                   const int seitenB = static_cast<int>(b.lumps[14].size() / 49152);
                   bool lm = flaechen > 0;
                   for (int s = 0; lm && s < flaechen; ++s) {
                       const int nr = i32(b.lumps[7], n * 40 + 24) + s;
                       const int l = i32(b.lumps[13], static_cast<std::size_t>(nr) * 148 + 36);
                       lm = l >= seitenA && l < seitenB;
                   }
                   KollisionsKarte k;
                   const float mitte[3] = {(z->cmin[0] + z->cmax[0]) * 0.5F, (z->cmin[1] + z->cmax[1]) * 0.5F, (z->cmin[2] + z->cmax[2]) * 0.5F};
                   const std::uint32_t c = leseKollision(lose, k) ? kollisionsInhalt(k, mitte) : 0U;
                   std::string meld;
                   for (const std::string& m : ke::editor().bspEinfuegenMeldungen) { meld += " | " + m; }
                   char t[256];
                   std::snprintf(t, sizeof t, "Knopftest: B13 G - %s, Modelle %d -> %d, Welt-Brushes %d -> %d, Tuer %d Flaechen, Lightmap-Seiten %d -> %d, "
                                              "gleich %d/4, vorn %d/4, Clip 0x%x",
                                 ke::editor().bspEinfuegenFertig.c_str(), z->tuerModell, static_cast<int>(b.lumps[7].size() / 40), i32(a.lumps[7], 36),
                                 i32(b.lumps[7], 36), flaechen, seitenA, seitenB, gleich, vorn, c);
                   diag::detail(t + meld);
                   const std::string nn = "*" + std::to_string(z->tuerModell);
                   return modelle && lm && gleich == 4 && vorn == 4 && (c & 0x10U) != 0U && ke::editor().bspEinfuegenFertig == nn + " " + nn + " clips 1" &&
                          std::none_of(ke::editor().bspEinfuegenMeldungen.begin(), ke::editor().bspEinfuegenMeldungen.end(),
                                       [](const std::string& m) { return m.rfind("fehler", 0) == 0; });
               }));
    // --- H: Vergleichsbild Editor gegen Vorschau, aus dem Auge des Spielers am Start (Blick auf die Tuer) --------
    // Augenhoehe: playerMaxs[2] 40 + STANDARD_VIEWHEIGHT_OFFSET -4 (MD9 code/game/bg_public.h, g_client.cpp ps.viewheight) ueber dem Ursprung
    add(tu("B13 H: Karten-Editor - Kamera im Auge des Spielers, Blick auf die Tuer", [z] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        ke::auswahlGeaendert();
        e.kamera.setzeUrsprung(karte::Vektor3f(z->start[0], z->start[1], z->start[2] - 9.0F + 36.0F));
        e.kamera.setzeWinkel(karte::Vektor3f(0.0F, z->yaw2, 0.0F));
        ++e.sichtStand;
    }));
    add(pause(1.0));
    add(foto("bspmerge_tuer_editor"));
    add(tu("B13 H: Mission-Tab, die gespeicherte Datei neu laden", [z] {
        g_app->leftMode = 1;
        karteDateiLaden(z->lose);
    }));
    add(warteBis("B13 H: geladen", [z] { return g_app->map.path == z->lose && g_app->geo.models.size() == static_cast<std::size_t>(z->tuerModell) + 1; }, 1800));
    add(pruefe("B13 H: die Vorschau hat die Tuer mit Lightmap (Luxel an den Ecken nicht schwarz) und func_door mit *N", [z] {
        const auto n = static_cast<std::size_t>(z->tuerModell);
        const auto& m = g_app->geo.models[n];
        double hell = 0.0;
        int ecken = 0;
        bool lm = m.numSurfaces > 0;
        for (int s = m.firstSurface; lm && s < m.firstSurface + m.numSurfaces; ++s) {
            const BspSurface& f = g_app->geo.surfaces[static_cast<std::size_t>(s)];
            lm = f.lightmap >= 0 && static_cast<std::size_t>(f.lightmap) < g_app->geo.lightmaps.size();
            for (int v = 0; lm && v < f.numVerts; ++v) {
                const BspVertex& e = g_app->geo.verts[static_cast<std::size_t>(f.firstVert + v)];
                const auto& bild = g_app->geo.lightmaps[static_cast<std::size_t>(f.lightmap)];
                const int w = g_app->geo.lightmapW(static_cast<std::size_t>(f.lightmap));
                const int h = g_app->geo.lightmapH(static_cast<std::size_t>(f.lightmap));
                const int x = std::clamp(static_cast<int>(e.lightmap[0] * static_cast<float>(w)), 0, w - 1);
                const int y = std::clamp(static_cast<int>(e.lightmap[1] * static_cast<float>(h)), 0, h - 1);
                const std::size_t p = (static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)) * 3;
                if (p + 2 < bild.size()) {
                    hell += (bild[p] + bild[p + 1] + bild[p + 2]) / 3.0;
                    ++ecken;
                }
            }
        }
        bool inListe = false;
        for (const MapEntity& e : g_app->map.entities) {
            const std::string* mm = e.find("model");
            inListe = inListe || (e.classname == "func_door" && mm != nullptr && *mm == "*" + std::to_string(z->tuerModell));
        }
        char t[200];
        std::snprintf(t, sizeof t, "Knopftest: B13 H - Tuer *%d: %d Flaechen, Lightmap %s, Helligkeit an %d Ecken %.1f, Liste %s", z->tuerModell, m.numSurfaces,
                      lm ? "ja" : "nein", ecken, ecken > 0 ? hell / ecken : 0.0, inListe ? "ja" : "nein");
        diag::detail(t);
        return lm && ecken > 0 && hell / ecken > 1.0 && inListe;
    }));
    add(tu("B13 H: Vorschau - Kamera im Auge des Spielers, Blick auf die Tuer", [z] {
        g_app->cam.pos[0] = z->start[0];
        g_app->cam.pos[1] = z->start[1];
        g_app->cam.pos[2] = z->start[2] - 9.0F + 36.0F;
        g_app->cam.angles[0] = 0.0F;
        g_app->cam.angles[1] = z->yaw2;
        g_app->cam.angles[2] = 0.0F;
        g_app->throughCamera = false;
        g_app->cameraSelected = false;
        g_app->mapDirty = true;
    }));
    add(pause(1.5));
    add(foto("bspmerge_tuer_vorschau"));
    add(tu("B1: Daten fuer den Spielvergleich (spiel.txt)", [z] {
        std::ofstream f(::bhed::utf8Pfad(z->wurzel + "\\spiel.txt"), std::ios::binary);
        char t[1024];
        // setviewpos zieht 25 fuer die Augenhoehe ab (Cmd_SetViewpos_f, MD9 code/game/g_cmds.cpp): der Spieler steht am Start (+9)
        std::snprintf(t, sizeof(t),
                      "karte %s\nsetviewpos %g %g %g %g\ntrigger %g %g %g %g %g %g\nmodell %d\ntuerblick %g %g %g %g\nclip %g %g %g %g %g %g\ntuer %g %g %g %g %g %g\n"
                      "tuermodell %d\n",
                      z->relativ.c_str(), z->start[0], z->start[1], z->start[2] + 25.0F, z->yaw, z->tmin[0], z->tmin[1], z->tmin[2], z->tmax[0], z->tmax[1],
                      z->tmax[2], z->modelleVorher, z->start[0], z->start[1], z->start[2] + 25.0F, z->yaw2, z->cmin[0], z->cmin[1], z->cmin[2], z->cmax[0],
                      z->cmax[1], z->cmax[2], z->dmin[0], z->dmin[1], z->dmin[2], z->dmax[0], z->dmax[1], z->dmax[2], z->tuerModell);
        f << t;
    }));
    // --- I: wieder im Karten-Editor oeffnen ---------------------------------------------------------
    add(tu("B13 I: Karten-Editor, die gespeicherte Datei wieder oeffnen", [z] {
        g_app->leftMode = 3;
        (void)ke::bspOeffnenDatei(z->lose);
    }));
    add(warteBis("B13 I: geoeffnet", [z] { return ke::editor().bspHerkunft.aktiv && ke::editor().bspHerkunft.quelle == z->lose; }, 3600));
    add(stufe8::warteSek("B13 I: Rueckgewinnung fertig", [] { return ke::bspRueckStand().stand == 0 && !ke::bauLaeuft(); }, 1500));
    add(pruefe("B13 I: ein func_door mehr, der Clip ist ein Welt-Brush (textures/system/clip), nichts neu", [z] {
        const int tueren = bm::zaehleKlasse("func_door");
        const int clips = bm::clipPinsel();
        diag::detail("Knopftest: B13 I - func_door " + std::to_string(z->tuerenVorher) + " -> " + std::to_string(tueren) + ", Clip-Brushes in der Welt " +
                     std::to_string(z->clipsVorher) + " -> " + std::to_string(clips) + ", neu " + std::to_string(ke::bspNeuePinselEntitaeten()) + "/" + std::to_string(ke::bspNeueWeltPinsel()));
        return tueren == z->tuerenVorher + 1 && clips == z->clipsVorher + 1 && ke::bspNeuePinselEntitaeten() == 0 && ke::bspNeueWeltPinsel() == 0;
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
    // rc609 B13: Clip und Tuer (Stufe 2 und 3), der Blick vom Start auf beide, dann die Vorschau mit der beleuchteten Tuer
    add(tu("Handbuch B13: Clip hinter dem Trigger und func_door", [z] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        (void)stufe2::testPinsel(karte::Vektor3f(z->cmin[0], z->cmin[1], z->cmin[2]), karte::Vektor3f(z->cmax[0], z->cmax[1], z->cmax[2]),
                                 "textures/system/clip");
        auto tuer = e.karte.neueEntitaet("func_door", true);
        e.karte.entitaetEinfuegen(tuer);
        tuer->setzen("angle", "-1");
        (void)stufe2::testPinsel(karte::Vektor3f(z->dmin[0], z->dmin[1], z->dmin[2]), karte::Vektor3f(z->dmax[0], z->dmax[1], z->dmax[2]), z->tuerShader, tuer);
        ke::auswahlGeaendert();
        ++e.sichtStand;
    }));
    add(pause(0.5));
    add(tu("Handbuch B13: Kamera im Auge des Spielers am Start, Blick auf die Tuer (wie B13 H)", [z] {
        ke::Editor& e = ke::editor();
        e.kamera.setzeUrsprung(karte::Vektor3f(z->start[0], z->start[1], z->start[2] - 9.0F + 36.0F));
        e.kamera.setzeWinkel(karte::Vektor3f(0.0F, z->yaw2, 0.0F));
        ++e.sichtStand;
    }));
    add(pause(1.0));
    add(hb::fotoTeil("h_bspmerge_tuer", editor));
    add(tu("Handbuch B13: In fertige Karte einfuegen (Teil und Licht-Teil)", [] { ke::editor().bspEinfuegenFertig.clear(); }));
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeBmEinfuegen)) + "##baueinfuegen", "", 0, true));
    add(warteBis("Handbuch B13: eingefuegt", [] { return !ke::bauLaeuft() && !ke::editor().bspEinfuegenFertig.empty(); }, 3600));
    add(tu("Handbuch B13: Mission-Tab, die gespeicherte Karte, Kamera im Auge des Spielers", [z] {
        g_app->leftMode = 1;
        karteDateiLaden(z->lose);
    }));
    add(warteBis("Handbuch B13: geladen", [z] { return g_app->map.path == z->lose && !g_app->geo.empty(); }, 1800));
    add(tu("Handbuch B13: Kamera", [z] {
        g_app->cam.pos[0] = z->start[0];
        g_app->cam.pos[1] = z->start[1];
        g_app->cam.pos[2] = z->start[2] - 9.0F + 36.0F;
        g_app->cam.angles[0] = 0.0F;
        g_app->cam.angles[1] = z->yaw2;
        g_app->cam.angles[2] = 0.0F;
        g_app->throughCamera = false;
        g_app->cameraSelected = false;
        g_app->mapDirty = true;
    }));
    add(pause(1.5));
    add(hb::fotoTeil("h_bspmerge_vorschau", [] {
        const ImGuiWindow* haupt = ImGui::FindWindowByName("###main");
        return haupt != nullptr ? haupt->Rect() : ImRect{};
    }));
    add(tu("Handbuch B1: zurueck in den Mission-Tab", [] { g_app->leftMode = 1; }));
    return s;
}
