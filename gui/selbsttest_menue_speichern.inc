// selbsttest_menue_speichern.inc - rc609 ME2: ein loses Menue im Mod-Ordner an seinen Ort speichern
// (BHED_MENUE_NUR=speichern, auch im Modus menueloeschen). Eingebunden in namespace menuetest.
// shank (04.11.): "Der Menue-Editor darf ein Menue, das LOSE in einem Mod-Ordner (oder base) unter GameData liegt, an
// seinen Ort zurueckspeichern - mit Sicherung." Ein Menue in einer .pk3 bleibt bei Speichern unter mit Hinweis.
// Alles in einer KOPIE eines Spielordners neben der exe (me2_spiel/GameData/base und /MD), die der Test als Spielordner
// einstellt und am Ende wieder austraegt - nie in einem echten Spielordner.

std::vector<Schritt> schritteSpeichernOrt() {
    namespace fs = std::filesystem;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const fs::path wurzel = bhed::utf8Pfad(platform::executableDirectory()) / "me2_spiel";
    const fs::path gameData = wurzel / "GameData";
    const fs::path base = gameData / "base";
    const fs::path mod = gameData / "MD";
    const fs::path pk3 = base / "me2probe.pk3";
    const fs::path lose = mod / "ui" / "me2_lose.menu";         // aus dem Spiel geoeffnet
    const fs::path datei = mod / "ui" / "me2_datei.menu";       // ueber Datei oeffnen...
    const fs::path schatten = base / "ui" / "me2_schatten.menu";   // lose, aber dieselbe Datei steht in der .pk3 daneben
    const fs::path programm = gameData / "me2_programm.menu";   // im Programmordner (exe, dll): bleibt gesperrt
    const fs::path kopie = wurzel / "kopien" / "me2_kopie.menu";
    const fs::path kopie2 = wurzel / "kopien" / "me2_kopie2.menu";
    const auto liesDatei = [](const fs::path& p) {
        std::ifstream f(p, std::ios::binary);
        return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    };
    const auto bak = [](fs::path p) { return p += ".bak"; };
    // Die ganze Meldung steht in der Einblendung unten rechts (die Leiste hat nur eine Zeile)
    const auto eingeblendet = [](const char* teil) {
        return std::any_of(g_app->einblendungen.begin(), g_app->einblendungen.end(),
                           [teil](const App::Einblendung& e) { return e.text.find(teil) != std::string::npos; });
    };
    const auto gefragt = std::make_shared<int>(0);
    const auto pk3Vorher = std::make_shared<std::string>();
    const auto ersterStand = std::make_shared<std::string>();
    const auto alteOrdner = std::make_shared<std::vector<std::string>>();
    const auto leer = [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
    };
    const auto dialog = [gefragt](const fs::path& ziel) {
        me::setzeSpeichernDialog([gefragt, t = pfadText(ziel)] {
            ++*gefragt;
            return t;
        });
    };
    add(tu("ME2: Kopie eines Spielordners anlegen und als Spielordner einstellen", [=] {
        std::error_code ec;
        fs::remove_all(wurzel, ec);
        for (const fs::path& p : {lose, schatten, programm, kopie}) { fs::create_directories(p.parent_path(), ec); }
        for (const fs::path& p : {lose, datei, schatten, programm}) { std::ofstream(p, std::ios::binary) << kLoeschText; }
        (void)writePk3(pfadText(pk3), {{"ui/me2_inpk3.menu", kLoeschText}, {"ui/me2_schatten.menu", kLoeschText}});
        *pk3Vorher = liesDatei(pk3);
        *alteOrdner = g_app->settings.gamePaths;
        for (const fs::path& o : {base, mod}) {
            g_app->settings.gamePaths.push_back(pfadText(o));
            GamePath g;
            (void)scanGamePath(pfadText(o), g);
            g.directory = pfadText(o);
            g_app->gamePaths.push_back(std::move(g));
        }
        leer();
    }));
    add(pause(0.3));
    // Strg+S auf dem geoeffneten Menue: die Auswahl loeschen, dann speichern
    const auto bearbeitenUndSpeichern = [&](const char* name) {
        add(tu(std::string("ME2: bearbeiten (") + name + " loeschen)", [n = std::string(name)] {
            me::waehle(me::itemNummer(n), false);
            me::loescheAuswahl();
        }));
        add(pause(0.2));
        add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_S, "Strg+S"));
        add(pause(0.3));
    };

    // 1. Aus dem Spiel: lose im Mod-Ordner (keine .pk3 hat es) - Strg+S schreibt an den Ort, die alte als .bak
    add(tu("ME2: ui/me2_lose.menu aus dem Spiel oeffnen", [=] {
        dialog(kopie);
        *gefragt = 0;
        (void)me::oeffneAusSpiel("ui/me2_lose.menu");
    }));
    add(pause(0.3));
    add(pruefe("ME2: das lose Menue des Mod-Ordners ist offen, ohne Projekt", [] {
        return me::stand().dokumente == 1 && me::projektDatei().empty() && me::itemMerker().size() == 10;
    }));
    bearbeitenUndSpeichern("mitte");
    add(pruefe("ME2 Aus dem Spiel: Strg+S fragt nicht und schreibt an den Ort, byte-gleich zum Text-Editor", [=] {
        const std::string t = liesDatei(lose);
        diag::detail("ME2 lose: gefragt " + std::to_string(*gefragt) + ", " + std::to_string(t.size()) + " Bytes, Meldung: " + me::letzteMeldung());
        *ersterStand = me::stand().text;
        return *gefragt == 0 && t == me::stand().text && t.find("\"mitte\"") == std::string::npos;
    }));
    add(pruefe("ME2 Aus dem Spiel: die alte Datei liegt als .bak daneben, die Meldung nennt sie", [=] {
        return liesDatei(bak(lose)) == kLoeschText && me::letzteMeldung().find(".bak") != std::string::npos && eingeblendet(".bak");
    }));
    bearbeitenUndSpeichern("drei");
    add(pruefe("ME2 Aus dem Spiel: das zweite Strg+S schreibt wieder dorthin, .bak ist der Stand davor", [=] {
        return *gefragt == 0 && liesDatei(lose) == me::stand().text && liesDatei(bak(lose)) == *ersterStand &&
               me::stand().text != *ersterStand;
    }));

    // 2. Datei oeffnen...: dieselbe Regel fuer eine lose Datei im Mod-Ordner
    add(tu("ME2: MD/ui/me2_datei.menu ueber Datei oeffnen", [=] {
        leer();
        *gefragt = 0;
        (void)me::oeffneDatei(pfadText(datei));
    }));
    add(pause(0.3));
    bearbeitenUndSpeichern("mitte");
    add(pruefe("ME2 Datei oeffnen: Strg+S schreibt an den Ort, .bak mit altem Inhalt", [=] {
        diag::detail("ME2 datei: gefragt " + std::to_string(*gefragt) + ", Meldung: " + me::letzteMeldung());
        return *gefragt == 0 && liesDatei(datei) == me::stand().text && liesDatei(bak(datei)) == kLoeschText;
    }));
    add(pruefe("ME2 Datei oeffnen: der Reiter heisst ui/me2_datei.menu (Spielpfad mit Schraegstrich)", [] {
        diag::detail("ME2 datei: Reiter \"" + me::dokumentName() + "\"");
        return me::dokumentName() == "ui/me2_datei.menu";
    }));

    // 3. Lose in base, aber die .pk3 daneben hat dieselbe Datei: an den Ort, mit Hinweis (fs_dirbeforepak 0)
    add(tu("ME2: base/ui/me2_schatten.menu ueber Datei oeffnen", [=] {
        leer();
        *gefragt = 0;
        (void)me::oeffneDatei(pfadText(schatten));
    }));
    add(pause(0.3));
    bearbeitenUndSpeichern("mitte");
    add(pruefe("ME2 verdeckt: Strg+S schreibt an den Ort und sagt, dass das Spiel die .pk3 zuerst liest", [=] {
        diag::detail("ME2 schatten: gefragt " + std::to_string(*gefragt) + ", Meldung: " + me::letzteMeldung());
        return *gefragt == 0 && liesDatei(schatten) == me::stand().text && liesDatei(bak(schatten)) == kLoeschText &&
               me::letzteMeldung().find("me2probe.pk3") != std::string::npos && eingeblendet("me2probe.pk3");
    }));
    add(foto("me2_meldung_verdeckt"));   // die ganze Meldung in der Einblendung

    // 4. Aus dem Spiel, in der .pk3: weiter Speichern unter mit Hinweis, die .pk3 bleibt Byte fuer Byte
    add(tu("ME2: ui/me2_inpk3.menu aus dem Spiel oeffnen", [=] {
        leer();
        *gefragt = 0;
        (void)me::oeffneAusSpiel("ui/me2_inpk3.menu");
    }));
    add(pause(0.3));
    bearbeitenUndSpeichern("mitte");
    add(pruefe("ME2 in der .pk3: Strg+S fragt nach dem Ort der Kopie und nennt die .pk3", [=] {
        diag::detail("ME2 pk3: gefragt " + std::to_string(*gefragt) + ", Meldung: " + me::letzteMeldung());
        return *gefragt == 1 && liesDatei(kopie) == me::stand().text && liesDatei(pk3) == *pk3Vorher &&
               me::letzteMeldung().find("me2probe.pk3") != std::string::npos && eingeblendet("me2probe.pk3");
    }));
    add(foto("me2_meldung_pk3"));

    // 5. Programmordner (GameData selbst): bleibt gesperrt
    add(tu("ME2: GameData/me2_programm.menu ueber Datei oeffnen", [=] {
        leer();
        dialog(kopie2);
        *gefragt = 0;
        (void)me::oeffneDatei(pfadText(programm));
    }));
    add(pause(0.3));
    bearbeitenUndSpeichern("mitte");
    add(pruefe("ME2 Programmordner: Strg+S fragt nach dem Ort, die Datei bleibt, keine .bak", [=] {
        std::error_code ec;
        return *gefragt == 1 && liesDatei(programm) == kLoeschText && liesDatei(kopie2) == me::stand().text &&
               !fs::exists(bak(programm), ec);
    }));

    // Der Startbildschirm nennt die Karte "Menue oeffnen" (MePrjLeer), in jeder Sprache
    add(pruefe("ME2: MePrjLeer nennt die Karte Menue oeffnen, in allen 6 Sprachen", [] {
        bool ok = true;
        for (const i18n::LanguageInfo& l : i18n::languages()) {
            const std::string t = i18n::trIn(l.language, Str::MePrjLeer);
            if (t.find(i18n::trIn(l.language, Str::MeStartMenueOeffnen)) == std::string::npos) {
                diag::detail(std::string("ME2 MePrjLeer ohne Kartennamen: ") + i18n::kurzCode(l.language));
                ok = false;
            }
        }
        return ok && i18n::languages().size() == 6;
    }));
    add(tu("ME2: aufraeumen, Spielordner wie vorher", [=] {
        me::setzeSpeichernDialog(nullptr);
        leer();
        g_app->einblendungen.clear();   // die Einblendungen der Meldungen laegen sonst ueber den Knoepfen der naechsten Schritte
        g_app->settings.gamePaths = *alteOrdner;
        g_app->gamePaths.pop_back();
        g_app->gamePaths.pop_back();
        std::error_code ec;
        fs::remove_all(wurzel, ec);
    }));
    add(pause(0.3));
    return s;
}

// rc609 Endrunde 1 (BHED_MENUE_NUR=spielbeleg, BHED_ME2_MOD=<fs_homepath>\MD mit einer losen Kopie des Menues
// BHED_ME2_MENUE, z. B. ui/quit.menu): das Menue aus dem Spiel oeffnen, BHED_ME2_ALT durch BHED_ME2_NEU ersetzen, Strg+S an
// seinen Ort. Danach startet das Skript des Spielvergleichs Movie Duels mit diesem fs_homepath und fotografiert das Menue.
std::vector<Schritt> schritteSpielbeleg() {
    namespace fs = std::filesystem;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto umgebung = [](const char* n) {
        const char* w = std::getenv(n);
        return std::string(w != nullptr ? w : "");
    };
    const fs::path mod = bhed::utf8Pfad(umgebung("BHED_ME2_MOD"));
    const std::string menue = umgebung("BHED_ME2_MENUE");
    const std::string alt = umgebung("BHED_ME2_ALT");
    const std::string neu = umgebung("BHED_ME2_NEU");
    const fs::path datei = mod / bhed::utf8Pfad(menue);
    const auto liesDatei = [](const fs::path& p) {
        std::ifstream f(p, std::ios::binary);
        return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    };
    const auto vorher = std::make_shared<std::string>();
    const auto ersetzt = std::make_shared<int>(0);
    add(tu("ME2 Spielbeleg: der Mod-Ordner des eigenen fs_homepath als letzter Spielordner, das Menue aus dem Spiel", [=] {
        *vorher = liesDatei(datei);
        g_app->settings.gamePaths.push_back(pfadText(mod));
        GamePath g;
        (void)scanGamePath(pfadText(mod), g);
        g.directory = pfadText(mod);
        g_app->gamePaths.push_back(std::move(g));
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneAusSpiel(menue);
    }));
    add(pause(0.5));
    add(tu("ME2 Spielbeleg: Text ersetzen", [=] { *ersetzt = me::ersetzeAlle(alt, neu, false); }));
    add(pause(0.2));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_S, "Strg+S"));
    add(pause(0.5));
    add(pruefe("ME2 Spielbeleg: an seinen Ort gespeichert, die alte Datei als .bak", [=] {
        const std::string t = liesDatei(datei);
        fs::path b = datei;
        b += ".bak";
        diag::detail("ME2 Spielbeleg: " + std::to_string(*ersetzt) + " ersetzt, " + std::to_string(vorher->size()) + " -> " + std::to_string(t.size()) +
                     " Bytes, Meldung: " + me::letzteMeldung());
        return !vorher->empty() && *ersetzt >= 1 && t.find(neu) != std::string::npos && liesDatei(b) == *vorher;
    }));
    return s;
}
