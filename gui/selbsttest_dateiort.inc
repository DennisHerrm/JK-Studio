// selbsttest_dateiort.inc - rc609 W2a: Dateien an Ort und Stelle (BHED_EDITORTEST=dateiort)
//
// Eingebunden an genau dieser Stelle von gui/selbsttest.cpp (im selben Namensraum, mit allen Helfern davor).
//
// Nachgebauter Mod-Ordner im eigenen Laufordner (<Lauf>\dateiort\GameData\MD), nie im
// Spielordner; die Karte ist md_am_sith aus den eingerichteten Spielordnern (nur gelesen).
//   A  lose maps/w2a_lose.bsp + lose Skript: oeffnen, aendern, speichern - nur diese Dateien
//      haben einen neuen Stand (dazu die neue .ent daneben und w2a_lose.bsp.bak), keine Kopie
//      unter Dokumente (BHED_MISSIONEN bleibt leer), keine neue .pk3
//   B  "In die BSP uebernehmen" Byte-gleich zu bhed_q3map2 -onlyents auf dieselben Entities
//   C  aus zz_w2a.pk3, Weg 1: Entities und Skript - nur der eine Eintrag ist anders, .pk3.bak
//   D  aus zz_w2a.pk3, Weg 2 (gemerkt): .pk3 Byte-gleich, die lose .ent liegt daneben
//   E  Skriptvorlage nur auf Wunsch, Weg 4 (Missionsordner + Pack-Dialog)
//   F1 Skript nur per run gestartet ist im Missions-pk3; F7 keine mission.txt, keine .pk3 darin
//   F2 Mission ohne gemerkte Entities, andere Karte vorn: die .ent im pk3 ist nicht leer
//   F3 Karte neu laden fragt und leert den Verlauf; F4 .ent oeffnen raeumt Auswahl, Verlauf, Skriptliste
namespace dateiort {

struct Zustand {
    std::string wurzel;
    std::string mod;
    std::string missionen;
    std::string loseBsp;
    std::string loseSkript;
    std::string pk3;
    std::string bsp;                               // md_am_sith, Original
    std::map<std::string, std::string> bestand;    // Datei -> Groesse:Pruefsumme
    std::map<std::string, std::string> eintraege;  // Eintrag der .pk3 -> Inhalt
    std::string pk3Vorher;
    std::vector<std::string> doku;                 // Dokumente\behaved\missions
    std::string projekt;                           // Missionsordner w2a_f1
    std::size_t f2Erwartet = 0;
    std::size_t f2Gefunden = 0;
    bool bereit = false;
};

std::string u8(const std::filesystem::path& p) {
    const auto s = p.generic_u8string();
    return std::string(s.begin(), s.end());
}
std::string lies(const std::string& p) {
    std::ifstream f(::bhed::utf8Pfad(p), std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
void schreib(const std::string& p, const std::string& t) {
    std::error_code ec;
    std::filesystem::create_directories(::bhed::utf8Pfad(p).parent_path(), ec);
    std::ofstream f(::bhed::utf8Pfad(p), std::ios::binary | std::ios::trunc);
    f.write(t.data(), static_cast<std::streamsize>(t.size()));
}
bool da(const std::string& p) {
    std::error_code ec;
    return std::filesystem::exists(::bhed::utf8Pfad(p), ec);
}
std::map<std::string, std::string> bestand(const std::string& wurzel) {
    std::map<std::string, std::string> aus;
    std::error_code ec;
    const std::filesystem::path w = ::bhed::utf8Pfad(wurzel);
    for (auto it = std::filesystem::recursive_directory_iterator(w, ec); !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
        if (!it->is_regular_file(ec)) { continue; }
        const auto rel = std::filesystem::relative(it->path(), w, ec).generic_u8string();
        const std::string inhalt = lies(u8(it->path()));
        aus[std::string(rel.begin(), rel.end())] = std::to_string(inhalt.size()) + ":" + std::to_string(std::hash<std::string>{}(inhalt));
    }
    return aus;
}
// Was in b neu, anders oder weg ist gegenueber a.
std::vector<std::string> unterschied(const std::map<std::string, std::string>& a, const std::map<std::string, std::string>& b) {
    std::vector<std::string> aus;
    for (const auto& [k, v] : b) {
        const auto it = a.find(k);
        if (it == a.end() || it->second != v) { aus.push_back(k); }
    }
    for (const auto& [k, v] : a) {
        if (b.count(k) == 0U) { aus.push_back(k + " (weg)"); }
    }
    return aus;
}
std::string liste(const std::vector<std::string>& v) {
    std::string s;
    for (const std::string& x : v) { s += (s.empty() ? "" : ", ") + x; }
    return s.empty() ? "-" : s;
}
std::map<std::string, std::string> eintraege(const std::string& pk3) {
    std::map<std::string, std::string> aus;
    Pk3 a;
    if (!readPk3Directory(pk3, a)) { return aus; }
    for (const Pk3Entry& e : a.entries) {
        std::string t;
        if (readPk3File(a, e, t)) { aus[e.name] = t; }
    }
    return aus;
}
std::vector<std::string> dokuMissionen() {
    std::vector<std::string> aus;
    const std::string d = platform::dokumenteOrdner();
    std::error_code ec;
    const std::filesystem::path p = ::bhed::utf8Pfad(d) / "behaved" / "missions";
    if (d.empty() || !std::filesystem::is_directory(p, ec)) { return aus; }
    for (const auto& e : std::filesystem::directory_iterator(p, ec)) { aus.push_back(u8(e.path().filename())); }
    return aus;
}
// Eine Zeile an das Skript des aktiven Reiters haengen (wie Einfuegen im Editor).
void zeileAnhaengen(const std::string& zeile) {
    Script sc = g_app->doc.script();
    Script x;
    std::vector<Diag> d;
    (void)readScript(zeile, x, d);
    for (Node& n : x.nodes) { sc.nodes.push_back(std::move(n)); }
    g_app->doc = Document{sc};
    rebuildTree();
}
std::string ibiText(const std::string& ibi) {
    std::vector<IbiBlock> bl;
    std::vector<Diag> d;
    Script sc;
    if (!readIbi(ibi, bl, d) || !decompile(bl, g_app->db, sc, d)) { return {}; }
    return writeScript(sc);
}
bool fragtNachWeg() { return !g_app->speicherFragen.empty(); }
// Spielvergleich (BHED_DO_BELEG=1): ein Wegpunkt 4 Einheiten unter dem Boden am Spielerstart - das Spiel meldet ihn beim
// Laden mit Namen (SP_waypoint: "ERROR: Waypoint <name> at ... in solid!", G_CheckInSolid), also hat es genau diese
// Entities gelesen.
void belegSetzen(const char* name) {
    if (std::getenv("BHED_DO_BELEG") == nullptr) { return; }
    float p[3]{};
    for (const MapEntity& e : g_app->map.entities) {
        if (e.classname == "info_player_start" && entityOrt(e, p)) { break; }
    }
    const float unten[3] = {p[0], p[1], p[2] - 1000.0F};
    const TraceTreffer t = traceRay(g_app->geo, p, unten);
    if (!t.hit) { return; }
    const float o[3] = {t.point[0], t.point[1], t.point[2] - 4.0F};
    const int i = neueEntityAn("waypoint", o);
    if (i >= 0) { (void)entityKeySetzen(i, "targetname", name); }
}
std::string kl(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

}  // namespace dateiort

std::vector<Schritt> dateiortSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<dateiort::Zustand>();
    using namespace dateiort;

    add(tu("Dateiort: Mod-Ordner im Laufordner nachbauen", [z] {
        std::error_code ec;
        z->wurzel = (std::filesystem::current_path() / "dateiort").string();
        std::filesystem::remove_all(::bhed::utf8Pfad(z->wurzel), ec);
        z->mod = z->wurzel + "\\GameData\\MD";
        z->missionen = z->wurzel + "\\missionen";
        std::filesystem::create_directories(::bhed::utf8Pfad(z->missionen), ec);
        _putenv_s("BHED_MISSIONEN", z->missionen.c_str());
        g_app->settings.pk3SpeicherWeg = 0;
        g_app->settings.missionsordnerAblauf = false;
        g_app->settings.skriptVorlage = false;
        if (!readFromArchives("maps/md_am_sith.bsp", z->bsp)) {
            diag::detail("Knopftest: Dateiort - maps/md_am_sith.bsp nicht in den Spielordnern");
            return;
        }
        z->loseBsp = z->mod + "\\maps\\w2a_lose.bsp";
        z->loseSkript = z->mod + "\\scripts\\w2a\\lose.txt";
        z->pk3 = z->mod + "\\zz_w2a.pk3";
        schreib(z->loseBsp, z->bsp);
        schreib(z->loseSkript, "rem ( \"w2a lose\" );\nwait ( 100.000 );\n");
        Script sc;
        std::vector<Diag> d;
        (void)readScript("rem ( \"w2a pk3\" );\nwait ( 200.000 );\n", sc, d);
        std::vector<IbiBlock> bl;
        std::vector<Diag> cd;
        const bool ibi = compile(sc, g_app->db, bl, cd);
        std::string fehler;
        z->bereit = ibi && writePk3(z->pk3, {{"maps/w2a_pk3.bsp", z->bsp}, {"scripts/w2a/pk3skript.IBI", writeIbi(bl)},
                                            {"levelshots/w2a_pk3.jpg", std::string(4096, '\x42')}},
                                    &fehler);
        z->doku = dokuMissionen();
        z->bestand = bestand(z->wurzel);
        diag::detail("Knopftest: Dateiort - " + z->wurzel + ", " + std::to_string(z->bestand.size()) + " Dateien, bsp " +
                     std::to_string(z->bsp.size()) + " Bytes " + fehler);
    }));
    add(pruefe("Dateiort: Mod-Ordner mit loser .bsp, losem Skript und zz_w2a.pk3 angelegt", [z] { return z->bereit; }));

    // --- A: lose Dateien ------------------------------------------------------------------
    add(tu("Dateiort A: lose .bsp oeffnen (Karte laden)", [z] {
        if (z->bereit) { karteDateiLaden(z->loseBsp); }
    }));
    add(warteBis("Dateiort A: Karte geladen", [z] { return !z->bereit || g_app->map.path == z->loseBsp; }, 600));
    add(pruefe("Dateiort A: Herkunft der .bsp ist die lose Datei", [z] {
        return g_app->kartenHerkunft.art == DateiHerkunft::Art::Lose && g_app->kartenHerkunft.pfad == z->loseBsp;
    }));
    add(tu("Dateiort A: worldspawn aendern, Entities speichern, in die BSP uebernehmen", [z] {
        (void)entityKeySetzen(0, "message", "rc609 W2a lose");
        belegSetzen("w2a_beleg_lose");
        (void)entitiesSpeichern();
        (void)entitiesInBspUebernehmen();
    }));
    add(tu("Dateiort A: loses Skript oeffnen, aendern, speichern (Strg+S)", [z] {
        ladeSkriptDatei(z->loseSkript);
        zeileAnhaengen("rem ( \"w2a geaendert\" );\n");
        (void)reiterSpeichernUnter(g_app->activeTab, std::string());
    }));
    add(pause(0.3));
    add(pruefe("Dateiort A: nur die geoeffneten Dateien haben einen neuen Stand (dazu .ent daneben und .bsp.bak)", [z] {
        const auto jetzt = bestand(z->wurzel);
        const std::vector<std::string> u = unterschied(z->bestand, jetzt);
        diag::detail("Knopftest: Dateiort A - neu/anders: " + liste(u));
        const std::vector<std::string> soll = {"GameData/MD/maps/w2a_lose.bsp", "GameData/MD/maps/w2a_lose.bsp.bak", "GameData/MD/maps/w2a_lose.ent",
                                               "GameData/MD/scripts/w2a/lose.txt"};
        std::vector<std::string> sortiert = u;
        std::sort(sortiert.begin(), sortiert.end());
        z->bestand = jetzt;
        return sortiert == soll;
    }));
    add(pruefe("Dateiort A: .bsp.bak ist das Original, .ent und .bsp tragen die Aenderung, das Skript auch", [z] {
        MapData m;
        std::string t;
        const bool bsp = readBsp(lies(z->loseBsp), m) && !m.entities.empty() && m.entities[0].find("message") != nullptr &&
                         *m.entities[0].find("message") == "rc609 W2a lose";
        const bool ent = lies(z->mod + "\\maps\\w2a_lose.ent").find("rc609 W2a lose") != std::string::npos;
        const bool bak = lies(z->loseBsp + ".bak") == z->bsp;
        const bool skript = lies(z->loseSkript).find("w2a geaendert") != std::string::npos;
        diag::detail(std::string("Knopftest: Dateiort A - bsp ") + (bsp ? "ja" : "nein") + ", ent " + (ent ? "ja" : "nein") + ", bak " +
                     (bak ? "ja" : "nein") + ", skript " + (skript ? "ja" : "nein"));
        return bsp && ent && bak && skript;
    }));
    add(pruefe("Dateiort A: keine Kopie unter Dokumente (Missionsordner leer, Dokumente\\behaved\\missions unveraendert), keine neue .pk3", [z] {
        const auto m = bestand(z->missionen);
        int pk3 = 0;
        for (const auto& [k, v] : z->bestand) { pk3 += (k.size() > 4 && kl(k.substr(k.size() - 4)) == ".pk3") ? 1 : 0; }
        return m.empty() && dokuMissionen() == z->doku && pk3 == 1;
    }));

    // --- B: -onlyents ----------------------------------------------------------------------
    add(tu("Dateiort B: bhed_q3map2 -onlyents auf dieselben Entities", [z] {
        const std::string ordner = z->wurzel + "\\q3";
        schreib(ordner + "\\w2a.bsp", z->bsp);
        std::vector<karte::EntitaetPaare> paare;
        for (const MapEntity& e : g_app->map.entities) { paare.push_back(e.keys); }
        schreib(ordner + "\\w2a.map", karte::nurEntitaetenKarte(paare));
        std::error_code ec;
        std::filesystem::create_directories(::bhed::utf8Pfad(ordner + "\\leer\\base"), ec);
        const char* q = std::getenv("BHED_Q3MAP2");
        std::string exe = q != nullptr ? q : platform::executableDirectory() + "\\bhed_q3map2.exe";
        const auto schraeg = [](std::string x) {
            for (char& c : x) { c = c == '\\' ? '/' : c; }
            return x;
        };
        karte::BspDatei b;
        const bool lichter = karte::bspLesen(z->bsp, b) && karte::bspHatLichter(karte::bspEntitaetenText(b));
        const karte::Q3map2Aufruf a{schraeg(exe), "ja", schraeg(ordner + "\\leer") + "/", "", "base"};
        bauplattform::Prozess p;
        std::string fehler;
        int rc = -1;
        if (p.starten(karte::onlyentsBefehl(a, schraeg(ordner + "\\w2a.map"), lichter), ordner, false, fehler) && p.warten(120000)) { rc = p.rueckgabe(); }
        const std::string q3 = rc == 0 ? karte::bspEntitaetenEinsetzen(z->bsp, lies(ordner + "\\w2a.bsp")) : std::string();
        const std::string ich = lies(z->loseBsp);
        diag::detail("Knopftest: Dateiort B - q3map2 rc " + std::to_string(rc) + " " + fehler + ", -onlyents " + std::to_string(q3.size()) +
                     " Bytes, Mission-Tab " + std::to_string(ich.size()) + " Bytes, " + (q3 == ich && !q3.empty() ? "gleich" : "ANDERS"));
        schreib(ordner + "\\ergebnis.txt", q3 == ich && !q3.empty() ? "gleich" : "anders");
    }));
    add(pruefe("Dateiort B: In die BSP uebernehmen ist Byte-gleich zu -onlyents", [z] { return lies(z->wurzel + "\\q3\\ergebnis.txt") == "gleich"; }));

    // --- C: aus der .pk3, Weg 1 --------------------------------------------------------------
    add(tu("Dateiort C: Karte aus zz_w2a.pk3 oeffnen, aendern, Entities speichern", [z] {
        z->pk3Vorher = lies(z->pk3);
        z->eintraege = eintraege(z->pk3);
        if (z->bereit) { (void)karteAusArchivOeffnen(z->pk3, "maps/w2a_pk3.bsp"); }
        (void)entityKeySetzen(0, "message", "rc609 W2a pk3");
        belegSetzen("w2a_beleg_pk3");
        (void)entitiesSpeichern();
    }));
    add(pruefe("Dateiort C: Herkunft .pk3 + Eintrag, es fragt nach dem Weg", [z] {
        return g_app->kartenHerkunft.art == DateiHerkunft::Art::Pk3 && g_app->kartenHerkunft.pfad == z->pk3 && fragtNachWeg();
    }));
    add(pause(0.3));
    add(klick(tr(Str::DoWegPk3), "###speicherweg"));
    add(pause(0.3));
    add(pruefe("Dateiort C: Weg 1 - nur maps/w2a_pk3.ent ist neu, alle anderen Eintraege gleich, .pk3.bak = vorher", [z] {
        const auto jetzt = eintraege(z->pk3);
        std::vector<std::string> anders;
        for (const auto& [k, v] : jetzt) {
            if (z->eintraege.count(k) == 0U || z->eintraege.at(k) != v) { anders.push_back(k); }
        }
        diag::detail("Knopftest: Dateiort C - Eintraege " + std::to_string(z->eintraege.size()) + " -> " + std::to_string(jetzt.size()) +
                     ", anders: " + liste(anders));
        const bool ent = jetzt.count("maps/w2a_pk3.ent") != 0U && jetzt.at("maps/w2a_pk3.ent").find("rc609 W2a pk3") != std::string::npos;
        const bool ok = anders == std::vector<std::string>{"maps/w2a_pk3.ent"} && ent && lies(z->pk3 + ".bak") == z->pk3Vorher && !fragtNachWeg();
        z->eintraege = jetzt;
        z->pk3Vorher = lies(z->pk3);
        return ok;
    }));
    add(tu("Dateiort C: Skript aus zz_w2a.pk3 oeffnen (IBI), aendern, Strg+S", [z] {
        if (z->bereit) { (void)skriptAusArchivOeffnen(z->pk3, "scripts/w2a/pk3skript.IBI"); }
        zeileAnhaengen("wait ( 321.000 );\n");   // rem kommt nicht in die .IBI (wie IBIze)
        (void)skriptAusPk3Speichern();
    }));
    add(pause(0.3));
    add(klick(tr(Str::DoWegPk3), "###speicherweg"));
    add(pause(0.3));
    add(pruefe("Dateiort C: Weg 1 - nur scripts/w2a/pk3skript.IBI ist anders (uebersetzt mit der Aenderung)", [z] {
        const auto jetzt = eintraege(z->pk3);
        std::vector<std::string> anders;
        for (const auto& [k, v] : jetzt) {
            if (z->eintraege.count(k) == 0U || z->eintraege.at(k) != v) { anders.push_back(k); }
        }
        const bool text = jetzt.count("scripts/w2a/pk3skript.IBI") != 0U &&
                          ibiText(jetzt.at("scripts/w2a/pk3skript.IBI")).find("321.000") != std::string::npos;
        diag::detail("Knopftest: Dateiort C Skript - anders: " + liste(anders) + ", Aenderung in der IBI " + (text ? "ja" : "nein"));
        const bool ok = anders == std::vector<std::string>{"scripts/w2a/pk3skript.IBI"} && text && lies(z->pk3 + ".bak") == z->pk3Vorher &&
                        !g_app->doc.dirty();
        z->eintraege = jetzt;
        return ok;
    }));

    // --- D: aus der .pk3, Weg 2 (gemerkt) ----------------------------------------------------
    add(tu("Dateiort D: Karte neu aus der .pk3, aendern, Entities speichern", [z] {
        z->pk3Vorher = lies(z->pk3);
        if (z->bereit) { (void)karteAusArchivOeffnen(z->pk3, "maps/w2a_pk3.bsp"); }
        (void)entityKeySetzen(0, "message", "rc609 W2a lose daneben");
        (void)entitiesSpeichern();
    }));
    add(pause(0.3));
    add(klick(tr(Str::DoWegMerken), "###speicherweg"));
    add(klick(tr(Str::DoWegLose), "###speicherweg"));
    add(pause(0.3));
    add(pruefe("Dateiort D: Weg 2 - .pk3 Byte-gleich, die lose .ent liegt im Mod-Ordner, Wahl gemerkt", [z] {
        const std::string ent = lies(z->mod + "\\maps\\w2a_pk3.ent");
        const bool ok = lies(z->pk3) == z->pk3Vorher && ent.find("rc609 W2a lose daneben") != std::string::npos && g_app->settings.pk3SpeicherWeg == 2;
        diag::detail("Knopftest: Dateiort D - lose .ent " + std::to_string(ent.size()) + " Bytes, gemerkt " + std::to_string(g_app->settings.pk3SpeicherWeg));
        return ok;
    }));
    add(tu("Dateiort D: zweites Speichern geht ohne Frage an die lose Datei", [z] {
        (void)entityKeySetzen(0, "message", "rc609 W2a zweites mal");
        (void)entitiesSpeichern();
    }));
    add(pruefe("Dateiort D: ohne Frage, lose .ent aktualisiert, .pk3 weiter Byte-gleich", [z] {
        return !fragtNachWeg() && lies(z->mod + "\\maps\\w2a_pk3.ent").find("zweites mal") != std::string::npos && lies(z->pk3) == z->pk3Vorher;
    }));

    // --- G: aus der .pk3, Weg 3 (Speichern unter; der Dateidialog antwortet im Selbsttest mit einem Pfad im Laufordner) --
    auto unter = std::make_shared<std::string>();
    add(tu("Dateiort G: Karte neu aus der .pk3, aendern, Entities speichern (Wahl nicht gemerkt)", [z, unter] {
        g_app->settings.pk3SpeicherWeg = 0;
        z->pk3Vorher = lies(z->pk3);
        *unter = z->wurzel + "\\unter\\w2a_unter.ent";
        std::error_code ec;
        std::filesystem::create_directories(utf8Pfad(z->wurzel + "\\unter"), ec);
        if (z->bereit) { (void)karteAusArchivOeffnen(z->pk3, "maps/w2a_pk3.bsp"); }
        (void)entityKeySetzen(0, "message", "rc609 W2a speichern unter");
        (void)entitiesSpeichern();
    }));
    add(pause(0.3));
    add(tu("Dateiort G: der Dateidialog antwortet mit unter\\w2a_unter.ent", [unter] { platform::dialogAntwortImSelbsttest(*unter); }));
    add(klick(tr(Str::DoWegUnter), "###speicherweg"));
    add(pause(0.3));
    add(pruefe("Dateiort G: Weg 3 - der Dialog schlug w2a_pk3.ent vor, die .ent liegt am gewaehlten Ort, .pk3 Byte-gleich", [z, unter] {
        const std::string ent = lies(*unter);
        const std::string vorschlag = platform::dialogVorschlagImSelbsttest();
        diag::detail("Knopftest: Dateiort G - Vorschlag \"" + vorschlag + "\", " + std::to_string(ent.size()) + " Bytes am gewaehlten Ort");
        return vorschlag == "w2a_pk3.ent" && ent.find("rc609 W2a speichern unter") != std::string::npos && lies(z->pk3) == z->pk3Vorher &&
               g_app->settings.pk3SpeicherWeg == 0;
    }));
    add(tu("Dateiort G: zweites Speichern", [] {
        (void)entityKeySetzen(0, "message", "rc609 W2a unter zweites mal");
        (void)entitiesSpeichern();
    }));
    add(pause(0.3));
    add(pruefe("Dateiort G: das zweite Speichern geht ohne Frage an die gewaehlte Datei", [z, unter] {
        return !fragtNachWeg() && lies(*unter).find("unter zweites mal") != std::string::npos && lies(z->pk3) == z->pk3Vorher;
    }));

    // --- F1 / F7 / E: Mission ------------------------------------------------------------------
    add(tu("Dateiort F1: neue Mission w2a_f1 auf der losen Karte, Intro startet teil2 per run", [z] {
        g_app->settings.pk3SpeicherWeg = 0;
        std::string fehler;
        if (!neueMissionAnlegen(z->loseBsp, "w2a_f1", "w2a_f1", &fehler)) { diag::detail("Knopftest: Dateiort F1 - " + fehler); }
        z->projekt = g_app->mission.projektOrdner;
        schreib(z->projekt + "\\scripts\\w2a_f1\\teil2.txt", "rem ( \"teil2\" );\nwait ( 500.000 );\n");
        zeileAnhaengen("run ( \"w2a_f1/teil2\" );\n");
        (void)reiterSpeichernUnter(g_app->activeTab, std::string());
    }));
    add(tu("Dateiort F1: zweimal Als .pk3 packen in den Missionsordner", [z] {
        std::string f;
        (void)missionSpeichernNach(z->projekt + "\\w2a_f1.pk3", &f);
        (void)missionSpeichernNach(z->projekt + "\\w2a_f1.pk3", &f);
    }));
    add(pruefe("Dateiort F1: scripts/w2a_f1/teil2.IBI (nur per run gestartet) ist im Missions-pk3", [z] {
        const auto e = eintraege(z->projekt + "\\w2a_f1.pk3");
        std::string namen;
        for (const auto& [k, v] : e) { namen += k + " "; }
        diag::detail("Knopftest: Dateiort F1 - Eintraege: " + namen);
        return e.count("scripts/w2a_f1/teil2.IBI") != 0U;
    }));
    add(pruefe("Dateiort F7: keine mission.txt (Plattenpfade) und keine .pk3 im veroeffentlichten pk3", [z] {
        for (const auto& [k, v] : eintraege(z->projekt + "\\w2a_f1.pk3")) {
            const std::string l = kl(k);
            if (l == "mission.txt" || (l.size() > 4 && l.substr(l.size() - 4) == ".pk3")) { return false; }
        }
        return true;
    }));
    add(tu("Dateiort E: neues Skript anlegen (Vorlage aus)", [z] { skriptOeffnen("w2a_f1/neu", true); }));
    add(pruefe("Dateiort E: ohne Vorlage leer und noch keine Datei, Pfad im Missionsordner", [z] {
        const bool leer = g_app->doc.script().nodes.empty();
        const bool keineDatei = !da(z->projekt + "\\scripts\\w2a_f1\\neu.txt");
        diag::detail(std::string("Knopftest: Dateiort E - leer ") + (leer ? "ja" : "nein") + ", Datei " + (keineDatei ? "nein" : "ja") + ", Pfad " + g_app->path);
        return leer && keineDatei && kl(g_app->path).find("neu.txt") != std::string::npos;
    }));
    add(tu("Dateiort E: Skript aus der .pk3, Weg 4 (Missionsordner + neue .pk3)", [z] {
        if (z->bereit) { (void)skriptAusArchivOeffnen(z->pk3, "scripts/w2a/pk3skript.IBI"); }
        zeileAnhaengen("rem ( \"w2a weg4\" );\n");
        (void)skriptAusPk3Speichern();
    }));
    add(pause(0.3));
    add(klick(tr(Str::DoWegMission), "###speicherweg"));
    add(pause(0.3));
    add(pruefe("Dateiort E: Weg 4 - Skript im Missionsordner, der Pack-Dialog ist offen", [z] {
        return lies(z->projekt + "\\scripts\\w2a\\pk3skript.txt").find("w2a weg4") != std::string::npos && g_app->missionSpeichernOffen;
    }));
    add(klick(tr(Str::MissionCancel), "##missionspeichern"));

    // --- F2 ------------------------------------------------------------------------------------
    add(tu("Dateiort F2: Entities der Mission vergessen, .ent weg, andere Karte vorn, packen", [z] {
        std::error_code ec;
        std::filesystem::remove(::bhed::utf8Pfad(z->projekt + "\\maps\\w2a_f1.ent"), ec);
        vergissEntities(g_app->mission.karte);
        neuerReiter();
        if (z->bereit) { (void)karteAusArchivOeffnen(z->pk3, "maps/w2a_pk3.bsp"); }
        std::string bsp;
        MapData m;
        if (kartenBytes(g_app->mission.karte, bsp) && readBsp(bsp, m)) { z->f2Erwartet = m.entities.size(); }
        std::string f;
        (void)missionSpeichernNach(z->wurzel + "\\f2.pk3", &f);
        const auto e = eintraege(z->wurzel + "\\f2.pk3");
        std::vector<MapEntity> ents;
        if (e.count("maps/w2a_f1.ent") != 0U) { parseEntities(e.at("maps/w2a_f1.ent"), ents); }
        z->f2Gefunden = ents.size();
        diag::detail("Knopftest: Dateiort F2 - .ent im pk3: " + std::to_string(z->f2Gefunden) + " Entities, .bsp " + std::to_string(z->f2Erwartet) +
                     " (Karte vorn " + g_app->map.path + ")");
    }));
    add(pruefe("Dateiort F2: die .ent im pk3 ist nicht leer (so viele Entities wie die .bsp)", [z] { return z->f2Erwartet > 0 && z->f2Gefunden == z->f2Erwartet; }));

    // --- F3 ------------------------------------------------------------------------------------
    add(tu("Dateiort F3: lose Karte laden, zweimal aendern, dieselbe Karte neu laden", [z] {
        karteDateiLaden(z->loseBsp);
        (void)entityKeySetzen(1, "w2a_f3", "1");
        (void)entityKeySetzen(1, "w2a_f3", "2");
        karteDateiLaden(z->loseBsp);
    }));
    add(pruefe("Dateiort F3: es fragt vor dem Verwerfen", [] { return g_app->frageOffen; }));
    add(pause(0.3));
    add(klick(tr(Str::AnswerYes)));
    add(pause(0.3));
    add(pruefe("Dateiort F3: nach Ja frisch geladen, Strg+Z holt nichts zurueck", [] {
        const bool frisch = g_app->map.entities.size() > 1 && g_app->map.entities[1].find("w2a_f3") == nullptr;
        diag::detail(std::string("Knopftest: Dateiort F3 - frisch ") + (frisch ? "ja" : "nein") + ", Undo moeglich " + (entityUndoMoeglich() ? "ja" : "nein"));
        return frisch && !entityUndoMoeglich();
    }));

    // --- F4 ------------------------------------------------------------------------------------
    add(tu("Dateiort F4: Mehrfachauswahl {3, 7}, eine Aenderung, dann .ent oeffnen", [z] {
        g_app->entAuswahl = {3, 7};
        (void)entityKeySetzen(2, "w2a_f4", "1");
        std::vector<MapEntity> ents = g_app->map.entities;
        MapEntity n;
        n.classname = "NPC_spawner";
        n.keys = {{"classname", "NPC_spawner"}, {"origin", "0 0 0"}, {"spawnscript", "w2a_f4/neu"}};
        ents.push_back(n);
        schreib(z->wurzel + "\\f4.ent", writeEntities(ents));
        (void)entDateiLaden(z->wurzel + "\\f4.ent");
    }));
    add(pruefe("Dateiort F4: Auswahl leer, kein Rueckgaengig in die alte Liste, Skriptliste aus der neuen .ent", [] {
        bool skript = false;
        for (const MapData::ScriptRef& r : g_app->map.scripts) { skript = skript || r.path == "w2a_f4/neu"; }
        diag::detail("Knopftest: Dateiort F4 - Auswahl " + std::to_string(g_app->entAuswahl.size()) + ", Undo " + (entityUndoMoeglich() ? "ja" : "nein") +
                     ", Skript w2a_f4/neu " + (skript ? "ja" : "nein"));
        return g_app->entAuswahl.empty() && !entityUndoMoeglich() && skript;
    }));
    add(tu("Dateiort: Einstellungen zurueck", [] {
        g_app->settings.pk3SpeicherWeg = 0;
        g_app->entitiesUngespeichert.clear();
    }));
    return s;
}
