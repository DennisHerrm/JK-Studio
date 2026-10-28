// selbsttest_programm.inc - aus gui/selbsttest.cpp geschnitten (Stufe 8d Punkt 5, unveraendert): Programm - Updater, Reiter, Speichern, Code-Pruefung, Missionen, Jedi Outcast
//
// Eingebunden an genau dieser Stelle von gui/selbsttest.cpp (im selben Namensraum, mit allen Helfern davor);
// keine eigene Uebersetzungseinheit - der vorverarbeitete Text ist derselbe wie vor dem Schnitt.
// Abschnitte: Der Auto-Updater gegen das echte GitHub (BHED_EDITORTEST=update); Undo: Meldung unten, Sprung zum Befehl, Undo-Liste wie in 3ds Max; Reiter schliessen darf die Arbeit im aktiven Reiter nicht verlieren; Mausrad ueber der Reiterleiste (rc601); Beenden nach gescheitertem "Ja, speichern"; Nur ein behaved: ein zweiter Start reicht an den laufenden weiter; Die Kopfzeile eines Feldes (Skriptname) bleibt beim Rollen stehen; Nach dem Laden einer Mission steht ihr ERSTES Skript vorn

// --- Der Auto-Updater gegen das echte GitHub (BHED_EDITORTEST=update) -------
//
// Nur aus einer KOPIE des Programmordners starten: der Test installiert
// wirklich (mit BHED_UPDATE_LOKAL=rc500 gilt das aktuelle Release als neuer).
// Neu gestartet wird nicht.
std::vector<Schritt> updateSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("Update: nachsehen", [] { updater::pruefen(false); }));
    add(warteBis("GitHub hat geantwortet", [] { return updater::zustand().stand != updater::Stand::Pruefe; }, 3000));
    add(pause(0.3));
    add(foto("update_gefunden"));
    add(pruefe("Update: ein neueres Release gefunden (gegen BHED_UPDATE_LOKAL)", [] {
        const updater::Zustand z = updater::zustand();
        diag::detail("Knopftest: Update - Stand " + std::to_string(static_cast<int>(z.stand)) + ", Release " +
                     z.release.tag + ", Meldung " + z.meldung + ", " + std::to_string(z.release.assets.size()) +
                     " Dateien");
        return z.stand == updater::Stand::Verfuegbar && !z.release.tag.empty() &&
               bhed::update::zipAsset(z.release) != nullptr;
    }));
    add(tu("Update: installieren", [] { updater::installieren(); }));
    // rc601: das Zip wuchs von 14 auf ~110 MB (Handbuch in vier Sprachen mit
    // Bildern) - mit 6000 Bildern (in Wahrheit 4000, der Deckel je Schritt)
    // brach der Test beim Laden ab ("haengt", 0 Dateien). Darum Sekunden.
    auto updateStart = std::make_shared<std::chrono::steady_clock::time_point>();
    Schritt warteUpdate{"Warte: Update installiert", [updateStart](int b) {
        if (b == 0) { *updateStart = std::chrono::steady_clock::now(); }
        const auto st = updater::zustand().stand;
        const double sek = std::chrono::duration<double>(std::chrono::steady_clock::now() - *updateStart).count();
        if (st == updater::Stand::Fertig || st == updater::Stand::Fehler) {
            diag::detail("Knopftest: Update installiert nach " + std::to_string(sek) + " s");
            return true;
        }
        if (sek > 600.0) {
            meldeFehler("Update installiert: nicht nach 600 s");
            return true;
        }
        return false;
    }, true};
    add(warteUpdate);
    add(pause(0.3));
    add(foto("update_fertig"));
    add(pruefe("Update: installiert, die alte .exe beiseitegelegt, data/ da", [] {
        const updater::Zustand z = updater::zustand();
        const std::string ordner = platform::executableDirectory();
        const bool alt = std::filesystem::exists(::bhed::utf8Pfad(ordner + "/behaved.exe.alt"));
        const bool exe = std::filesystem::exists(::bhed::utf8Pfad(ordner + "/behaved.exe"));
        const bool daten = std::filesystem::exists(::bhed::utf8Pfad(ordner + "/data/base/anims.h"));
        diag::detail("Knopftest: Update - Stand " + std::to_string(static_cast<int>(z.stand)) + ", " +
                     std::to_string(z.dateien) + " Dateien, alt " + (alt ? "ja" : "nein") + ", exe " +
                     (exe ? "ja" : "nein") + ", data " + (daten ? "ja" : "nein") + ", Meldung " + z.meldung);
        return z.stand == updater::Stand::Fertig && z.dateien > 0 && alt && exe && daten;
    }));
    return s;
}

// --- Undo: Meldung unten, Sprung zum Befehl, Undo-Liste wie in 3ds Max ----
// --- Reiter schliessen darf die Arbeit im aktiven Reiter nicht verlieren ---
//
// shank, 02.10.: "I closed some scripts/tabs I was no longer using and went
// back to continue working on the script but it had reset. All my changes
// were lost with no undo/redo history." Nachgestellt: drei Reiter, im
// mittleren bearbeiten, dann einen anderen (unveraenderten) schliessen -
// links und rechts davon.
std::vector<Schritt> reiterSchliessenSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto findeReiter = [](const std::string& name) {
        // Der aktive Reiter lebt in g_app, nicht in seiner geparkten Kopie.
        for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
            const std::string& n = (static_cast<int>(i) == g_app->activeTab) ? g_app->shownName
                                                                             : g_app->tabs[i].shownName;
            if (n == name) { return static_cast<int>(i); }
        }
        return -1;
    };
    const auto aktivName = [] { return g_app->shownName; };
    const auto aendern = [](const std::string& wert) {
        if (const Node* n = nodeAt(g_app->doc.script(), Path{0}); n != nullptr && !n->args.empty()) {
            Node neu = *n;
            neu.args[0].text = wert;
            (void)g_app->doc.replaceAt(Path{0}, neu);
            rebuildTree();
        }
    };
    const auto wertJetzt = [] {
        const Node* n = nodeAt(g_app->doc.script(), Path{0});
        return (n != nullptr && !n->args.empty()) ? n->args[0].text : std::string("?");
    };
    for (int richtung = 0; richtung < 2; ++richtung) {
        const std::string z = std::to_string(richtung);
        const std::string links = "reiter_links" + z + ".txt";
        const std::string mitte = "reiter_mitte" + z + ".txt";
        const std::string rechts = "reiter_rechts" + z + ".txt";
        add(tu("Reiter " + z + ": drei Skripte oeffnen, im mittleren arbeiten", [=] {
            openScriptFromMemory("//Generated by BehavEd\nwait ( 1.000 );\n", links);
            openScriptFromMemory("//Generated by BehavEd\nwait ( 2.000 );\n", mitte);
            openScriptFromMemory("//Generated by BehavEd\nwait ( 3.000 );\n", rechts);
            std::string liste;
            for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
                liste += " [" + std::to_string(i) + "]" +
                         (static_cast<int>(i) == g_app->activeTab ? "*" + g_app->shownName : g_app->tabs[i].shownName);
            }
            diag::detail("Knopftest: Reiter - Liste" + liste + ", mitte bei " + std::to_string(findeReiter(mitte)));
            activateTab(findeReiter(mitte));
            diag::detail("Knopftest: Reiter - nach activateTab aktiv " + std::to_string(g_app->activeTab) + " " +
                         g_app->shownName);
        }));
        add(pause(0.3));
        add(tu("Reiter " + z + ": den mittleren zweimal aendern", [=] {
            aendern("222.000");
            aendern("333.000");
        }));
        add(pause(0.3));
        add(tu("Reiter " + z + ": einen ANDEREN, unveraenderten Reiter schliessen", [=] {
            const int weg = findeReiter(richtung == 0 ? links : rechts);
            diag::detail("Knopftest: Reiter - schliesse " + (richtung == 0 ? links : rechts) + " (" +
                         std::to_string(weg) + "), aktiv " + std::to_string(g_app->activeTab) + " " + aktivName());
            reiterSchliessen(weg);
        }));
        add(pause(0.3));
        add(pruefe("Reiter " + z + ": der aktive Reiter ist noch derselbe, mit beiden Aenderungen und Undo", [=] {
            diag::detail("Knopftest: Reiter - danach aktiv " + aktivName() + ", Wert " + wertJetzt() + ", Undo " +
                         std::to_string(g_app->doc.undoDepth()));
            return aktivName() == mitte && wertJetzt() == "333.000" && g_app->doc.undoDepth() == 2;
        }));
        add(tu("Reiter " + z + ": weg- und zurueckwechseln", [=] {
            activateTab(findeReiter(richtung == 0 ? rechts : links));
            activateTab(findeReiter(mitte));
        }));
        add(pause(0.3));
        add(pruefe("Reiter " + z + ": nach dem Hin- und Herwechseln immer noch alles da", [=] {
            return aktivName() == mitte && wertJetzt() == "333.000" && g_app->doc.undoDepth() == 2;
        }));
        add(tu("Reiter " + z + ": aufraeumen", [=] {
            for (const std::string& n : {links, mitte, rechts}) {
                const int i = findeReiter(n);
                if (i >= 0) {
                    g_app->tabs[static_cast<std::size_t>(i)].doc.markSaved();
                    if (i == g_app->activeTab) { g_app->doc.markSaved(); }
                }
            }
            for (const std::string& n : {links, mitte, rechts}) {
                const int i = findeReiter(n);
                if (i >= 0) { reiterSchliessen(i); }
            }
        }));
        add(pause(0.3));
    }

    // ---- shank, 03.10.: "it was like I just opened the file again" ---------
    //
    // (1) Dieselbe Datei noch einmal oeffnen darf keinen zweiten Reiter mit
    //     dem Stand von der Platte anlegen.
    auto datei = std::make_shared<std::string>(platform::executableDirectory() + "/selbsttest_doppelt.txt");
    auto reiterVorher = std::make_shared<std::size_t>(0);
    add(tu("Doppelt: Datei schreiben, oeffnen, zweimal aendern", [=] {
        std::ofstream f(::bhed::utf8Pfad(*datei), std::ios::binary);
        f << "//Generated by BehavEd\r\nwait ( 7.000 );\r\n";
        f.close();
        ladeSkriptDatei(*datei);
        aendern("71.000");
        aendern("72.000");
        *reiterVorher = g_app->tabs.size();
    }));
    add(pause(0.3));
    add(tu("Doppelt: weg in einen anderen Reiter, dann dieselbe Datei noch einmal oeffnen", [=] {
        openScriptFromMemory("//Generated by BehavEd\nwait ( 8.000 );\n", "doppelt_anderer.txt");
        ladeSkriptDatei(*datei);
    }));
    add(pause(0.3));
    add(pruefe("Doppelt: kein zweiter Reiter, zurueck im alten - mit Aenderungen und Undo", [=] {
        diag::detail("Knopftest: Doppelt - Reiter " + std::to_string(g_app->tabs.size()) + " (vorher " +
                     std::to_string(*reiterVorher) + " + 1 anderer), Wert " + wertJetzt() + ", Undo " +
                     std::to_string(g_app->doc.undoDepth()) + ", Pfad " + g_app->path);
        return g_app->tabs.size() == *reiterVorher + 1 && wertJetzt() == "72.000" &&
               g_app->doc.undoDepth() == 2 && g_app->homeTab == g_app->activeTab;
    }));
    // (2) Ein Skript aus der Mission (Archiv) zum zweiten Mal laden.
    add(tu("Doppelt: Mission-Skript laden und aendern, dann noch einmal laden", [=] {
        oeffneSkriptAusSpeicher("//Generated by BehavEd\nwait ( 9.000 );\n", "md_selbst/doppelt_mission.txt");
        aendern("91.000");
        activateTab(findeReiter("doppelt_anderer.txt"));
        oeffneSkriptAusSpeicher("//Generated by BehavEd\nwait ( 9.000 );\n", "md_selbst/doppelt_mission.txt");
    }));
    add(pause(0.3));
    add(pruefe("Doppelt: Mission-Skript nur einmal offen, Aenderung noch da", [=] {
        int anzahl = 0;
        for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
            const std::string& n = (static_cast<int>(i) == g_app->activeTab) ? g_app->shownName
                                                                             : g_app->tabs[i].shownName;
            if (n == "doppelt_mission.txt") { ++anzahl; }
        }
        diag::detail("Knopftest: Doppelt - Mission-Skript " + std::to_string(anzahl) + " mal offen, aktiv " +
                     g_app->shownName + ", Wert " + wertJetzt());
        return anzahl == 1 && g_app->shownName == "doppelt_mission.txt" && wertJetzt() == "91.000" &&
               g_app->doc.undoDepth() == 1;
    }));

    // (3) Einen ANDEREN, UNGESICHERTEN Reiter schliessen und "Nein" sagen:
    //     danach muss man wieder im Arbeitsreiter stehen, nicht im Nachbarn.
    for (int antwort = 0; antwort < 2; ++antwort) {
        const std::string z = std::to_string(antwort);
        const std::string arbeit = "arbeit" + z + ".txt";
        const std::string fremd = "fremd" + z + ".txt";
        const std::string nachbar = "nachbar" + z + ".txt";
        add(tu("Ungesichert " + z + ": drei Reiter, der mittlere wird geaendert", [=] {
            openScriptFromMemory("//Generated by BehavEd\nwait ( 1.000 );\n", arbeit);
            openScriptFromMemory("//Generated by BehavEd\nwait ( 2.000 );\n", fremd);
            aendern("22.000");
            openScriptFromMemory("//Generated by BehavEd\nwait ( 3.000 );\n", nachbar);
            activateTab(findeReiter(arbeit));
            aendern("11.000");
            aendern("12.000");
        }));
        add(pause(0.3));
        add(tu("Ungesichert " + z + ": den geaenderten fremden Reiter schliessen", [=] {
            reiterSchliessen(findeReiter(fremd));
        }));
        add(pause(0.3));
        add(pruefe("Ungesichert " + z + ": die Frage kommt", [] { return g_app->askSaveOpen; }));
        add(klick(tr(antwort == 0 ? Str::AnswerNo : Str::EditorCancel), tr(Str::AppTitle)));
        add(pause(0.3));
        add(pruefe(std::string("Ungesichert ") + z + (antwort == 0 ? ": nach Nein" : ": nach Abbrechen") +
                       " wieder im Arbeitsreiter, mit beiden Aenderungen und Undo",
                   [=] {
                       diag::detail("Knopftest: Ungesichert - aktiv " + g_app->shownName + " (" +
                                    std::to_string(g_app->activeTab) + "), Band " + std::to_string(g_app->homeTab) +
                                    ", Wert " + wertJetzt() + ", Undo " + std::to_string(g_app->doc.undoDepth()) +
                                    ", fremder noch offen: " + (findeReiter(fremd) >= 0 ? "ja" : "nein"));
                       const bool fremdOffen = findeReiter(fremd) >= 0;
                       return g_app->shownName == arbeit && wertJetzt() == "12.000" &&
                              g_app->doc.undoDepth() == 2 && g_app->homeTab == g_app->activeTab &&
                              fremdOffen == (antwort == 1);
                   }));
        add(tu("Ungesichert " + z + ": aufraeumen", [=] {
            for (const std::string& n : {arbeit, fremd, nachbar}) {
                const int i = findeReiter(n);
                if (i >= 0) {
                    g_app->tabs[static_cast<std::size_t>(i)].doc.markSaved();
                    if (i == g_app->activeTab) { g_app->doc.markSaved(); }
                }
            }
            for (const std::string& n : {arbeit, fremd, nachbar}) {
                const int i = findeReiter(n);
                if (i >= 0) { reiterSchliessen(i); }
            }
        }));
        add(pause(0.3));
    }
    add(tu("Doppelt: aufraeumen", [=] {
        for (const std::string& n : {std::string("selbsttest_doppelt.txt"), std::string("doppelt_anderer.txt"),
                                     std::string("doppelt_mission.txt")}) {
            for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
                const bool aktiv = static_cast<int>(i) == g_app->activeTab;
                const std::string& pfad = aktiv ? g_app->path : g_app->tabs[i].path;
                const std::string& nm = aktiv ? g_app->shownName : g_app->tabs[i].shownName;
                if (nm == n || (!pfad.empty() && pfad.find(n) != std::string::npos)) {
                    g_app->tabs[i].doc.markSaved();
                    if (aktiv) { g_app->doc.markSaved(); }
                    reiterSchliessen(static_cast<int>(i));
                    break;
                }
            }
        }
        std::error_code ec;
        std::filesystem::remove(::bhed::utf8Pfad(*datei), ec);
    }));
    add(pause(0.3));

    // ======== Code-Pruefung 03.10.: weitere Wege zum Datenverlust ==========
    const auto befehl = [](const char* name) -> const Command* {
        for (const Command& c : g_app->db.commands) {
            if (c.name == name) { return &c; }
        }
        return nullptr;
    };
    const auto knotenZahl = [] { return g_app->doc.script().nodes.size(); };

    // (4) Speichern, Strg+Z, eine ANDERE Aenderung: das Skript ist
    //     ungesichert - Stern, und Schliessen fragt nach.
    auto dirtyDatei = std::make_shared<std::string>(platform::executableDirectory() + "/selbsttest_stern.txt");
    add(tu("Stern: Datei oeffnen, print einfuegen", [=] {
        std::ofstream f(::bhed::utf8Pfad(*dirtyDatei), std::ios::binary);
        f << "//Generated by BehavEd\r\nwait ( 5.000 );\r\n";
        f.close();
        ladeSkriptDatei(*dirtyDatei);
        if (const Command* c = befehl("print")) { fuegeBefehlEin(*c); }
        diag::detail("Knopftest: Stern - nach print " + std::to_string(knotenZahl()) + " Befehle, ungesichert " +
                     (g_app->doc.dirty() ? "ja" : "nein"));
    }));
    add(pause(0.3));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_S, "Strg+S"));
    add(pause(0.3));
    add(pruefe("Stern: nach Strg+S gesichert", [] { return !g_app->doc.dirty(); }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(tu("Stern: statt des print den wait-Wert aendern (andere Aenderung)", [=] {
        aendern("55.000");
        diag::detail("Knopftest: Stern - jetzt " + std::to_string(knotenZahl()) + " Befehle, Wert " + wertJetzt() +
                     ", Undo " + std::to_string(g_app->doc.undoDepth()) + ", ungesichert " +
                     (g_app->doc.dirty() ? "ja" : "nein"));
    }));
    add(pause(0.3));
    add(pruefe("Stern: Speichern - Strg+Z - andere Aenderung gilt als UNGESICHERT", [] {
        return g_app->doc.dirty();
    }));
    add(tu("Stern: den Reiter schliessen (wie das X)", [] { reiterSchliessen(g_app->activeTab); }));
    add(pause(0.3));
    add(pruefe("Stern: Schliessen fragt nach, statt die Aenderung wegzuwerfen", [] { return g_app->askSaveOpen; }));
    add(tu("Stern: falls gefragt - Abbrechen", [] {
        if (g_app->askSaveOpen) {
            schritteEinfuegen({klick(tr(Str::EditorCancel), tr(Str::AppTitle)), pause(0.3)});
        }
    }));
    add(pruefe("Stern: das Skript ist noch offen, mit der Aenderung", [=] {
        const int i = findeReiter("selbsttest_stern.txt");
        bool offen = false;
        for (std::size_t k = 0; k < g_app->tabs.size(); ++k) {
            const bool aktiv = static_cast<int>(k) == g_app->activeTab;
            const std::string& pfad = aktiv ? g_app->path : g_app->tabs[k].path;
            if (pfad.find("selbsttest_stern.txt") != std::string::npos) { offen = true; }
        }
        diag::detail(std::string("Knopftest: Stern - noch offen: ") + (offen ? "ja" : "nein") + " (" +
                     std::to_string(i) + "), Wert " + wertJetzt());
        return offen && wertJetzt() == "55.000";
    }));
    add(tu("Stern: aufraeumen", [=] {
        for (std::size_t k = 0; k < g_app->tabs.size(); ++k) {
            const bool aktiv = static_cast<int>(k) == g_app->activeTab;
            const std::string& pfad = aktiv ? g_app->path : g_app->tabs[k].path;
            if (pfad.find("selbsttest_stern.txt") != std::string::npos) {
                g_app->tabs[k].doc.markSaved();
                if (aktiv) { g_app->doc.markSaved(); }
                reiterSchliessen(static_cast<int>(k));
                break;
            }
        }
        std::error_code ec;
        std::filesystem::remove(::bhed::utf8Pfad(*dirtyDatei), ec);
        std::filesystem::remove_all(
            ::bhed::utf8Pfad(platform::executableDirectory() + "/backup/selbsttest_stern"), ec);
    }));
    add(pause(0.3));

    // (5) Geteilte Ansicht: der Fokus steht im FREMDEN Reiter, der wird
    //     geschlossen - danach muss man im Arbeitsreiter stehen.
    add(tu("Geteilt: Arbeitsreiter und zwei weitere, im Arbeitsreiter aendern", [=] {
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1.000 );\n", "g_arbeit.txt");
        openScriptFromMemory("//Generated by BehavEd\nwait ( 2.000 );\n", "g_fremd.txt");
        openScriptFromMemory("//Generated by BehavEd\nwait ( 3.000 );\n", "g_dritter.txt");
        activateTab(findeReiter("g_arbeit.txt"));
        aendern("41.000");
        aendern("42.000");
    }));
    add(pause(0.3));
    splitMenue(s, 2);
    add(tu("Geteilt: zweites Feld zeigt den fremden Reiter, Fokus dorthin", [=] {
        g_app->splitPanes[1].tab = findeReiter("g_fremd.txt");
        g_app->focusRequest = 1;
    }));
    add(pause(0.4));
    add(pruefe("Geteilt: der fremde Reiter ist jetzt aktiv, das Band zeigt den Arbeitsreiter", [=] {
        diag::detail("Knopftest: Geteilt - aktiv " + g_app->shownName + " (" + std::to_string(g_app->activeTab) +
                     "), Band " + std::to_string(g_app->homeTab) + ", Felder " + std::to_string(g_app->splitCount));
        return g_app->shownName == "g_fremd.txt" && g_app->homeTab == findeReiter("g_arbeit.txt");
    }));
    add(tu("Geteilt: den fremden (fokussierten) Reiter schliessen", [] { reiterSchliessen(g_app->activeTab); }));
    add(pause(0.4));
    add(pruefe("Geteilt: danach im Arbeitsreiter, mit Aenderungen und Undo", [=] {
        diag::detail("Knopftest: Geteilt - danach aktiv " + g_app->shownName + " (" +
                     std::to_string(g_app->activeTab) + "), Band " + std::to_string(g_app->homeTab) + ", Wert " +
                     wertJetzt() + ", Undo " + std::to_string(g_app->doc.undoDepth()));
        return g_app->shownName == "g_arbeit.txt" && wertJetzt() == "42.000" && g_app->doc.undoDepth() == 2 &&
               g_app->homeTab == g_app->activeTab;
    }));
    splitMenue(s, 1);
    add(tu("Geteilt: aufraeumen", [=] {
        for (const char* n : {"g_arbeit.txt", "g_fremd.txt", "g_dritter.txt"}) {
            const int i = findeReiter(n);
            if (i >= 0) {
                g_app->tabs[static_cast<std::size_t>(i)].doc.markSaved();
                if (i == g_app->activeTab) { g_app->doc.markSaved(); }
                reiterSchliessen(i);
            }
        }
    }));
    add(pause(0.3));

    add(pause(0.3));
    return s;
}

// --- Mausrad ueber der Reiterleiste (rc601) -----------------------------------
//
// Gemeldet (Tester, 17.10.): "it would be nice if I could use the scroll wheel
// on the file tabs to move left and right so I can view all the files."
// ImGui rollt die Leiste nur mit dem waagerechten Rad oder Umschalt+Rad.
//
// Gemessen wird am Zustand der Leiste selbst (ImGuiTabBar): die Elementliste
// trug fuer Reiter das Rechteck des VORIGEN Elements (TabItemEx meldet die
// Beschriftung vor dem ItemAdd) - bis rc601, siehe tourRegisterSchritte.
ImGuiTabBar* reiterLeiste() {
    ImGuiWindow* w = ImGui::FindWindowByName("###main");
    return w != nullptr ? GImGui->TabBars.GetByKey(ImHashStr("##skripte", 0, w->ID)) : nullptr;
}

// Ist Reiter i (Kennung "###tab<i>", app.cpp) ganz zu sehen - gemessen am ZIEL
// des Rollens, damit das Ergebnis nicht von der Bildrate abhaengt.
bool reiterGanzSichtbar(int i) {
    ImGuiTabBar* l = reiterLeiste();
    if (l == nullptr || i < 0) { return false; }
    const std::string k = "###tab" + std::to_string(i);
    const ImGuiTabItem* t = ImGui::TabBarFindTabByID(l, ImHashStr(k.c_str(), 0, l->ID));
    if (t == nullptr) { return false; }
    const float x1 = l->BarRect.Min.x + t->Offset - l->ScrollingTarget;
    return x1 >= l->ScrollingRectMinX - 1.5F && x1 + t->Width <= l->ScrollingRectMaxX + 1.5F;
}

// =============================================================================
// Paket tour (rc601): Element-Register - jeder Reiter und das "+" mit seinem
// EIGENEN Rechteck
// =============================================================================
//
// Grundlage der Tutorials im Programm: eine Tour zeigt auf einen Knopf, also
// muss das Register wissen, wo er steht. ImGuis TabItemEx meldet aber die
// Beschriftung (IMGUI_TEST_ENGINE_ITEM_INFO, imgui_widgets.cpp:10711) VOR
// seinem ItemAdd (imgui_widgets.cpp:10827). selbsttestMerkeText fand die
// Kennung dann noch nicht und nahm ctx->LastItemData.Rect - das Rechteck des
// VORIGEN Elements; das richtige kam als zweiter Eintrag ohne Beschriftung,
// den finde() ueberspringt. Zu sehen im ausgelieferten Handbuchbild
// s_neu_plus.webp ("The + at the end of the tab bar opens an empty script"):
// der gelbe Rahmen sitzt auf dem letzten Reiter, nicht auf "+".
std::vector<Schritt> tourRegisterSchritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto vorn = std::make_shared<int>(-1);
    auto zahl = std::make_shared<std::size_t>(0);
    add(tu("Register: zwei leere Reiter", [=] {
        *vorn = g_app->activeTab;
        *zahl = g_app->tabs.size();
        neuerReiter();
        neuerReiter();
    }));
    add(pause(0.5));
    add(pruefe("Register: jeder Reiter und das \"+\" hat genau EIN Rechteck - beschrifteter und "
               "unbeschrifteter Eintrag einer Kennung stimmen ueberein", [] {
        ImGuiTabBar* l = reiterLeiste();
        if (l == nullptr || l->Tabs.Size < 3) { return false; }
        bool ok = true;
        for (const ImGuiTabItem& t : l->Tabs) {
            const Element* beschriftet = nullptr;
            int unbeschriftet = 0;
            int falsch = 0;
            for (const Element& e : g_elemente) {
                if (e.id != t.ID) { continue; }
                if (!e.label.empty()) {
                    if (beschriftet == nullptr) { beschriftet = &e; }
                    continue;
                }
                ++unbeschriftet;
            }
            if (beschriftet == nullptr) {
                diag::detail("Knopftest: Register - Reiter ohne Beschriftung im Register");
                ok = false;
                continue;
            }
            for (const Element& e : g_elemente) {
                if (e.id != t.ID || &e == beschriftet) { continue; }
                if (std::fabs(e.rect.Min.x - beschriftet->rect.Min.x) > 0.5F ||
                    std::fabs(e.rect.Max.x - beschriftet->rect.Max.x) > 0.5F) {
                    ++falsch;
                }
            }
            // Die Breite rechnet die Reiterleiste selbst aus (ImGuiTabItem::Width).
            const bool breiteStimmt = std::fabs(beschriftet->rect.GetWidth() - t.Width) < 1.5F;
            if (falsch > 0 || !breiteStimmt) {
                char z[320];
                std::snprintf(z, sizeof(z),
                              "Knopftest: Register - \"%s\" bei %.0f..%.0f (Breite %.0f, Leiste rechnet %.0f), "
                              "%d Eintraege ohne Beschriftung, %d davon mit anderem Rechteck",
                              beschriftet->label.c_str(), static_cast<double>(beschriftet->rect.Min.x),
                              static_cast<double>(beschriftet->rect.Max.x),
                              static_cast<double>(beschriftet->rect.GetWidth()), static_cast<double>(t.Width),
                              unbeschriftet, falsch);
                diag::detail(z);
                ok = false;
            }
        }
        return ok;
    }));
    add(pruefe("Register: das \"+\" der Reiterleiste ist schmal - nicht das Rechteck des letzten Reiters", [] {
        const float h = ImGui::GetFrameHeight();
        const Element* plus = nullptr;
        for (int k = 0;; ++k) {
            const Element* e = finde("+", "###main", k, false);
            if (e == nullptr) { break; }
            if (e->rect.Min.y < h * 3.0F && (plus == nullptr || e->rect.Min.x > plus->rect.Min.x)) { plus = e; }
        }
        if (plus == nullptr) {
            diag::detail("Knopftest: Register - kein \"+\" oben gefunden");
            return false;
        }
        char z[200];
        std::snprintf(z, sizeof(z), "Knopftest: Register - \"+\" bei %.0f..%.0f, Breite %.0f (Zeilenhoehe %.0f)",
                      static_cast<double>(plus->rect.Min.x), static_cast<double>(plus->rect.Max.x),
                      static_cast<double>(plus->rect.GetWidth()), static_cast<double>(h));
        diag::detail(z);
        return plus->rect.GetWidth() > 1.0F && plus->rect.GetWidth() < h * 2.0F;
    }));
    // Die Stufe "Gefiltert" (gui/elemente.h), mit der die Tutorials im
    // normalen Betrieb laufen: nur angemeldete Beschriftungen, und ein Reiter
    // (Beschriftung vor dem ItemAdd) bekommt dasselbe Rechteck wie in "Voll".
    const auto plusOben = [] {
        const Element* plus = nullptr;
        for (int k = 0;; ++k) {
            const Element* e = finde("+", "###main", k, false);
            if (e == nullptr) { break; }
            if (e->rect.Min.y < ImGui::GetFrameHeight() * 3.0F && (plus == nullptr || e->rect.Min.x > plus->rect.Min.x)) {
                plus = e;
            }
        }
        return plus;
    };
    auto plusVoll = std::make_shared<ImRect>();
    add(tu("Register: Stufe Gefiltert mit \"+\" und dem Datei-Menue anmelden", [=] {
        if (const Element* p = plusOben()) { *plusVoll = p->rect; }
        g_registerFilter = {"+", tr(Str::MenuFile)};
    }));
    add(pause(0.2));
    add(pruefe("Register: Stufe Gefiltert - nur die angemeldeten Beschriftungen, das \"+\" mit demselben "
               "Rechteck wie in Stufe Voll", [=] {
        int fremd = 0;
        for (const Element& e : g_elemente) {
            if (e.label.rfind("+", 0) != 0 && e.label.rfind(tr(Str::MenuFile), 0) != 0) { ++fremd; }
        }
        const Element* p = plusOben();
        char z[260];
        std::snprintf(z, sizeof(z),
                      "Knopftest: Register gefiltert - Stufe %d, %d Eintraege, %d fremde, \"+\" %s bei %.0f..%.0f "
                      "(Voll: %.0f..%.0f)",
                      static_cast<int>(elemente::modus()), static_cast<int>(g_elemente.size()), fremd,
                      p != nullptr ? "da" : "fehlt", p != nullptr ? static_cast<double>(p->rect.Min.x) : 0.0,
                      p != nullptr ? static_cast<double>(p->rect.Max.x) : 0.0, static_cast<double>(plusVoll->Min.x),
                      static_cast<double>(plusVoll->Max.x));
        diag::detail(z);
        return elemente::modus() == elemente::Modus::Gefiltert && fremd == 0 && !g_elemente.empty() &&
               g_elemente.size() < 40 && p != nullptr && plusVoll->GetWidth() > 1.0F &&
               std::fabs(p->rect.Min.x - plusVoll->Min.x) < 0.5F && std::fabs(p->rect.Max.x - plusVoll->Max.x) < 0.5F &&
               finde(tr(Str::MenuFile), "###main", 0, false) != nullptr;
    }));
    add(tu("Register: wieder Stufe Voll", [] { g_registerFilter.clear(); }));
    add(pause(0.2));
    add(pruefe("Register: nach dem Abmelden wieder Stufe Voll", [] {
        return elemente::modus() == elemente::Modus::Voll && g_elemente.size() > 100;
    }));
    add(tu("Register: aufraeumen", [=] {
        for (std::size_t i = g_app->tabs.size(); i-- > *zahl;) {
            g_app->tabs[i].doc.markSaved();
            if (static_cast<int>(i) == g_app->activeTab) { g_app->doc.markSaved(); }
            reiterSchliessen(static_cast<int>(i));
        }
        if (*vorn >= 0 && *vorn < static_cast<int>(g_app->tabs.size())) { activateTab(*vorn); }
    }));
    add(pause(0.3));
    return s;
}

// =============================================================================
// Paket tour (rc601): die Tutorials im Programm, mit echter Bedienung
// =============================================================================
//
// Ein Durchgang durch "k-neues-skript" (handbuch/<sprache>/touren.json, neben
// behaved.exe oder bis drei Ebenen hoeher): ueber das Help-Menue oeffnen,
// starten, das hervorgehobene "+" klicken, Zurueck (ohne sofortiges
// Weiterschalten), Weiter, Zeig es mir, Schweben, Mach es, den Oeffner (die
// Zeile vor dem Editor), Tippen und Ok, Ueberspringen mit Einrichten, Weiter
// bei einem Schritt zum Ansehen, Schliessen, Fortsetzen. Dazu die Menuekette
// (erst "File", aufgeklappt "New") und ein zweites Tutorial mit zwei
// nummerierten Zielen und einem echten Doppelklick in der Ereignisliste.
std::vector<Schritt> tourSchritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto vorn = std::make_shared<int>(-1);
    auto zahl = std::make_shared<std::size_t>(0);
    auto knotenVorher = std::make_shared<int>(0);
    auto schwebAb = std::make_shared<double>(0.0);
    const auto gleich = [](const ImRect& a, const ImRect& b) {
        return std::fabs(a.Min.x - b.Min.x) < 1.5F && std::fabs(a.Min.y - b.Min.y) < 1.5F &&
               std::fabs(a.Max.x - b.Max.x) < 1.5F && std::fabs(a.Max.y - b.Max.y) < 1.5F;
    };
    const auto sichtbar = [](const Element* e) {
        if (e == nullptr) { return ImRect{}; }
        ImRect c = e->rect;
        c.ClipWithFull(e->clip);
        return (c.GetWidth() > 1.0F && c.GetHeight() > 1.0F) ? c : e->rect;
    };
    const auto zielMitte = [](std::size_t i) {
        const TourSicht v = tourSicht();
        return i < v.ziele.size() ? v.ziele[i].GetCenter() : ImVec2{-1.0F, -1.0F};
    };
    const auto melde = [](const std::string& was) {
        const TourSicht v = tourSicht();
        std::string z = "Knopftest: Tour - " + was + ": aktiv " + std::to_string(v.aktiv) + ", " + v.id + " Schritt " +
                        std::to_string(v.schritt + 1) + "/" + std::to_string(v.anzahl) + ", Ziele";
        for (const ImRect& r : v.ziele) {
            char b[80];
            std::snprintf(b, sizeof(b), " [%.0f,%.0f-%.0f,%.0f]", static_cast<double>(r.Min.x), static_cast<double>(r.Min.y),
                          static_cast<double>(r.Max.x), static_cast<double>(r.Max.y));
            z += b;
        }
        z += v.zeigtOeffner ? ", Oeffner" : "";
        diag::detail(z);
    };
    const auto oberesPlus = [] {
        const Element* best = nullptr;
        for (int k = 0;; ++k) {
            const Element* e = finde("+", "###main", k, false);
            if (e == nullptr) { break; }
            if (e->rect.Min.y < ImGui::GetFrameHeight() * 3.0F && (best == nullptr || e->rect.Min.x > best->rect.Min.x)) { best = e; }
        }
        return best;
    };
    const auto menue = [](const std::string& label, const char* fenster) {
        for (int k = 0;; ++k) {
            const Element* e = finde(label, fenster, k, false);
            if (e == nullptr) { return static_cast<const Element*>(nullptr); }
            if ((e->flags & ImGuiItemStatusFlags_Openable) != 0 || std::strcmp(fenster, "##Menu") == 0) { return e; }
        }
    };
    const auto daneben = [] { return klickAn("daneben", [] { return ImVec2{12.0F, ImGui::GetIO().DisplaySize.y - 6.0F}; }); };
    // wait in allen Ebenen (ein Einfuegen hinter eine Zeile im Block landet im Block).
    const auto waitsUeberall = [] {
        std::function<int(const std::vector<Node>&)> z = [&z](const std::vector<Node>& ns) {
            int n = 0;
            for (const Node& k : ns) { n += (k.kind == Node::Kind::Command && k.name == "wait" ? 1 : 0) + z(k.children); }
            return n;
        };
        return z(g_app->doc.script().nodes);
    };
    const auto ueberHelp = [&](const std::string& was) {
        add(tu("Tour: " + was, [] {}));
        add(klick(tr(Str::MenuHelp), "###main"));
        add(pause(0.3));
        add(klick(tr(Str::TourMenu), "##Menu"));
        add(pause(0.4));
    };

    // Einen Schritt nur ausfuehren, wenn die Bedingung beim Beginn gilt.
    const auto wenn = [](std::function<bool()> bedingung, Schritt x) {
        auto an = std::make_shared<bool>(false);
        return Schritt{x.text, [=](int b) {
                           if (b == 0) { *an = bedingung(); }
                           return *an ? x.tun(b) : true;
                       }};
    };
    const auto ohneKarte = [] { return g_app->geo.empty(); };
    // Im normalen Betrieb laeuft das Register nur fuer die Beschriftungen der
    // Tour (Stufe Gefiltert). Hier meldet der Selbsttest kurz nichts Eigenes an
    // (kein Element heisst "\x01") - die Tour muss dieselben Ziele finden wie
    // in Stufe Voll.
    auto vollZiele = std::make_shared<std::vector<ImRect>>();
    const auto gefiltertGleich = [&](const std::string& wann) {
        add(tu("Tour: " + wann + " - Ziele merken, dann nur die Tour anmelden lassen", [=] {
            *vollZiele = tourSicht().ziele;
            g_registerFilter = {"\x01"};
        }));
        add(pause(0.3));
        add(pruefe("Tour: " + wann + " in Stufe Gefiltert (nur die Beschriftungen der Tour) - dieselben Ziele wie in Stufe Voll", [=] {
            melde(wann + ", gefiltert");
            const TourSicht v = tourSicht();
            bool gleichAlle = !vollZiele->empty() && v.ziele.size() == vollZiele->size();
            for (std::size_t i = 0; gleichAlle && i < v.ziele.size(); ++i) { gleichAlle = gleich(v.ziele[i], (*vollZiele)[i]); }
            diag::detail("Knopftest: Tour - Stufe " + std::to_string(static_cast<int>(elemente::modus())) + ", " +
                         std::to_string(g_elemente.size()) + " Eintraege im Register");
            return elemente::modus() == elemente::Modus::Gefiltert && v.angemeldet && gleichAlle && g_elemente.size() < 80;
        }));
        add(tu("Tour: wieder Stufe Voll", [] { g_registerFilter.clear(); }));
        add(pause(0.2));
    };

    add(tu("Tour: Stand merken, Classic und Events", [=] {
        *vorn = g_app->activeTab;
        *zahl = g_app->tabs.size();
        g_app->leftMode = 0;
        g_app->settings.flussAnsicht = 0;
        for (const char* id : {"k-neues-skript", "k-befehl-einfuegen", "k-zeilen-ordnen", "k-uebersetzen"}) {
            g_app->settings.tourStand.erase(id);
        }
    }));
    // Der Einstieg auf der Startflaeche (Map-Tab ohne Karte, Zeile "New here?").
    add(tu("Tour: Startflaeche (Map-Tab ohne Karte)", [=] {
        if (ohneKarte()) { g_app->leftMode = 1; }
        diag::detail(std::string("Knopftest: Tour - Startflaeche ") + (ohneKarte() ? "zu sehen" : "nicht zu sehen (Karte geladen)"));
    }));
    add(pause(0.5));
    add(wenn(ohneKarte, klick(tr(Str::TourStartGuide), "###main")));
    add(pause(0.3));
    add(pruefe("Tour: \"Interactive tour\" auf der Startflaeche oeffnet das Tutorial-Fenster", [=] {
        return !ohneKarte() || tourSicht().fensterOffen;
    }));
    add(wenn(ohneKarte, klick(tr(Str::TourClose), "###tour")));
    add(tu("Tour: zurueck zu Events", [] { g_app->leftMode = 0; }));
    add(pause(0.3));
    ueberHelp("Fenster ueber Help -> Interactive tutorials");
    add(pruefe("Tour: das Fenster ist offen und hat die Tutorials aus touren.json geladen", [] {
        const TourSicht v = tourSicht();
        diag::detail("Knopftest: Tour - Ordner \"" + v.ordner + "\", " + std::to_string(v.touren) + " Tutorials" +
                     (v.fehler.empty() ? std::string() : ", Fehler: " + v.fehler));
        return v.fensterOffen && v.fehler.empty() && v.touren >= 5;
    }));
    add(klick(std::string(tr(Str::TourStart)) + "##k-neues-skript", "###tour"));
    add(pause(0.5));
    add(pruefe("Tour: Schritt 1 zeigt auf das \"+\" der Reiterleiste (1) und auf File (2)", [=] {
        melde("Schritt 1");
        const TourSicht v = tourSicht();
        const Element* plus = oberesPlus();
        const Element* datei = menue(tr(Str::MenuFile), "###main");
        return v.aktiv && v.id == "k-neues-skript" && v.schritt == 0 && v.overlay && v.ziele.size() == 2 && plus != nullptr &&
               datei != nullptr && gleich(v.ziele[0], sichtbar(plus)) && gleich(v.ziele[1], sichtbar(datei));
    }));
    gefiltertGleich("Schritt 1 (+ und File)");
    // Die Menuekette zeigt ihr tiefstes offenes Glied.
    add(klick(tr(Str::MenuFile), "###main"));
    add(pause(0.3));
    add(pruefe("Tour: File aufgeklappt - die Menuekette zeigt jetzt auf New im Menue", [=] {
        melde("File offen");
        const TourSicht v = tourSicht();
        const Element* neu = menue(tr(Str::FileNew), "##Menu");
        return v.ziele.size() == 2 && neu != nullptr && gleich(v.ziele[1], sichtbar(neu));
    }));
    gefiltertGleich("Schritt 1 mit offenem File-Menue (New)");
    add(daneben());
    add(pause(0.3));
    add(klickAn("Tour: das hervorgehobene \"+\"", [=] { return zielMitte(0); }));
    add(warteBis("Tour bei Schritt 2", [] { return tourSicht().schritt == 1; }, 4000));
    add(pause(0.2));
    add(pruefe("Tour: Schritt 2 zeigt auf affect in der Ereignisliste", [=] {
        melde("Schritt 2");
        const TourSicht v = tourSicht();
        const Element* e = finde("affect  ", "", 0, true);
        return v.ziele.size() == 1 && e != nullptr && gleich(v.ziele[0], sichtbar(e));
    }));
    gefiltertGleich("Schritt 2 (affect in der Ereignisliste)");
    add(klick(tr(Str::TourBack), "###tour"));
    add(pause(0.8));
    add(pruefe("Tour: Zurueck - wieder Schritt 1, er schaltet NICHT sofort wieder weiter (der Reiter ist schon leer)", [] {
        const TourSicht v = tourSicht();
        return v.schritt == 0 && !v.erfuellt;
    }));
    add(pruefe("Tour: nach Zurueck steht Weiter statt Ueberspringen da", [] {
        return finde(tr(Str::TourNext), "###tour", 0, false) != nullptr && finde(tr(Str::TourSkip), "###tour", 0, false) == nullptr;
    }));
    add(klick(tr(Str::TourNext), "###tour"));
    add(pause(0.3));
    add(klick(tr(Str::TourShowMe), "###tour"));
    add(pause(0.2));
    add(pruefe("Tour: Zeig es mir - der Geisterzeiger laeuft, geklickt wird nichts", [] {
        const TourSicht v = tourSicht();
        return v.geist && v.schritt == 1 && anzahl("affect") == 0;
    }));
    add({"Tour: die Maus auf affect stehen lassen", [=](int b) {
             if (b == 0) { *schwebAb = ImGui::GetTime(); }
             setzeMaus(zielMitte(0));
             return ImGui::GetTime() - *schwebAb > 1.0;
         }});
    add(warteBis("Tour bei Schritt 3 (affect lange genug gezeigt)", [] { return tourSicht().schritt == 2; }, 4000));
    add(pause(0.2));
    add(klick(tr(Str::TourDoIt), "###tour"));
    add(warteBis("Tour bei Schritt 4 (Mach es hat affect eingefuegt)", [] { return tourSicht().schritt == 3; }, 4000));
    add(pause(0.3));
    add(pruefe("Tour: Mach es - genau ein affect im Skript", [] { return anzahl("affect") == 1; }));
    add(pruefe("Tour: Schritt 4 bei geschlossenem Editor - die Tour zeigt auf den Oeffner, die affect-Zeile", [=] {
        melde("Schritt 4, Editor zu");
        const TourSicht v = tourSicht();
        const Element* e = finde("affect", "pane0", 0, false);
        return v.zeigtOeffner && v.ziele.size() == 1 && e != nullptr && v.ziele[0].Contains(e->rect.GetCenter());
    }));
    add(klickAn("Tour: Doppelklick auf die hervorgehobene Zeile", [] {
        const TourSicht v = tourSicht();
        if (v.ziele.empty()) { return ImVec2{-1.0F, -1.0F}; }
        return ImVec2{v.ziele[0].Min.x + ImGui::GetFontSize() * 2.0F, v.ziele[0].GetCenter().y};
    }, 0, true));
    add(warteBis("Tour: der Editor ist offen", [] { return g_app->editorOpen; }, 2000));
    add(pause(0.4));
    add(pruefe("Tour: Editor offen - die Tour zeigt auf das erste Feld (1) und Ok (2)", [=] {
        melde("Schritt 4, Editor offen");
        const TourSicht v = tourSicht();
        const Element* ok = finde(tr(Str::EditorOk), "###editor", 0, false);
        return !v.zeigtOeffner && v.ziele.size() == 2 && ok != nullptr && gleich(v.ziele[1], sichtbar(ok));
    }));
    add(klickAn("Tour: das erste Feld", [=] { return zielMitte(0); }));
    add(tippe("kyle"));
    add(klick(tr(Str::EditorOk), "###editor"));
    add(warteBis("Tour bei Schritt 5 (affect heisst kyle)", [] { return tourSicht().schritt == 4; }, 4000));
    add(pruefe("Tour: das erste Feld von affect ist jetzt kyle", [] { return argText("affect", 0, 0) == "kyle"; }));
    // rc601 tour (Review): Ueberspringen aendert das Skript nicht mehr (es
    // setzte vorher ein wait ein) - das wait in den Block setzt "Mach es".
    add(klick(tr(Str::TourDoIt), "###tour"));
    add(warteBis("Tour bei Schritt 6 (Mach es hat wait eingefuegt)", [] { return tourSicht().schritt == 5; }, 4000));
    add(pruefe("Tour: Mach es - Schritt 6, und die Tour hat wait IN den affect-Block gesetzt", [] {
        const Node* a = knoten("affect", 0);
        bool drin = false;
        if (a != nullptr) {
            for (const Node& c : a->children) {
                if (c.kind == Node::Kind::Command && c.name == "wait") { drin = true; }
            }
        }
        return tourSicht().schritt == 5 && drin;
    }));
    add(pruefe("Tour: Schritt 6 ist zum Ansehen - Weiter statt Ueberspringen", [] {
        return finde(tr(Str::TourNext), "###tour", 0, false) != nullptr && finde(tr(Str::TourSkip), "###tour", 0, false) == nullptr;
    }));
    add(klick(tr(Str::TourNext), "###tour"));
    add(pause(0.4));
    add(pruefe("Tour: Schritt 7 zeigt auf Save - ohne \"Mach es\" (Speichern oeffnet einen Dateidialog)", [=] {
        melde("Schritt 7");
        const TourSicht v = tourSicht();
        const Element* sp = finde(tr(Str::FileSave), "", 0, false);
        return v.schritt == 6 && v.ziele.size() == 1 && sp != nullptr && gleich(v.ziele[0], sichtbar(sp)) &&
               finde(tr(Str::TourDoIt), "###tour", 0, false) == nullptr;
    }));
    add(klick(tr(Str::TourClose), "###tour"));
    add(pause(0.3));
    add(pruefe("Tour: geschlossen - keine Hervorhebung, nichts mehr im Register angemeldet, Fortschritt 6/7 gemerkt", [] {
        const TourSicht v = tourSicht();
        const auto it = g_app->settings.tourStand.find("k-neues-skript");
        diag::detail("Knopftest: Tour - Fortschritt " + (it != g_app->settings.tourStand.end() ? it->second : std::string("-")));
        return !v.aktiv && !v.fensterOffen && !v.overlay && !v.angemeldet && it != g_app->settings.tourStand.end() &&
               it->second == "6/7";
    }));
    ueberHelp("Fortsetzen ueber die Liste");
    auto weiterKlick = std::make_shared<Schritt>();
    add(Schritt{"Tour: Fortsetzen (Schritt 7) anklicken", [=](int b) {
                    // Die Beschriftung baut das Fenster genau so: tr(TourContinue) mit der
                    // Nummer, dazu "##<id>" (die Sprache steht erst zur Laufzeit fest).
                    if (b == 0) {
                        char c[96];
                        std::snprintf(c, sizeof(c), tr(Str::TourContinue), 7);
                        *weiterKlick = klick(std::string(c) + "##k-neues-skript", "###tour");
                    }
                    return weiterKlick->tun(b);
                }});
    add(pause(0.4));
    add(pruefe("Tour: Fortsetzen - wieder bei Schritt 7", [] { return tourSicht().aktiv && tourSicht().schritt == 6; }));
    add(klick(tr(Str::TourAllList), "###tour"));
    add(pause(0.3));
    // Die Suche im Handbuch (suche.js) im selben Fenster.
    add(klick("##toursuche", "###tour"));
    add(tippe("affect"));
    add(pause(0.3));
    add(pruefe("Tour: die Suche im Handbuch findet Abschnitte zu affect", [] {
        return finde(tr(Str::TourSearchGuide), "###tour", 0, false) != nullptr;
    }));
    add(klick(tr(Str::TourSearchGuide), "###tour"));
    add(pause(0.2));
    add(pruefe("Tour: \"Im Handbuch oeffnen\" fragt die Seite an (im Selbsttest nur in der Statuszeile)", [] {
        const std::string z = g_app->statusLines.empty() ? std::string() : g_app->statusLines.back();
        diag::detail("Knopftest: Tour - Statuszeile: " + z);
        return z.find("Handbuch angefragt") != std::string::npos && z.find(".html") != std::string::npos;
    }));
    add(klick("##toursuche", "###tour"));
    add(tippe(""));
    add(pause(0.2));
    // Ein zweites Tutorial: zwei nummerierte Ziele, ein echter Doppelklick.
    add(klick(std::string(tr(Str::TourStart)) + "##k-befehl-einfuegen", "###tour"));
    add(pause(0.6));
    add(pruefe("Tour: k-befehl-einfuegen Schritt 1 - zwei Ziele: der Baum (1) und wait in der Liste (2), sichtbar gerollt", [=] {
        melde("Einfuegen 1");
        const TourSicht v = tourSicht();
        const Element* w = finde("wait  ", "", 0, true);
        return v.aktiv && v.id == "k-befehl-einfuegen" && v.ziele.size() == 2 && w != nullptr && gleich(v.ziele[1], sichtbar(w)) &&
               ganzSichtbar(*w);
    }));
    add(tu("Tour: Befehle zaehlen", [=] { *knotenVorher = waitsUeberall(); }));
    add(klickAn("Tour: Doppelklick auf wait (Ziel 2)", [=] { return zielMitte(1); }, 0, true));
    add(warteBis("Tour: k-befehl-einfuegen bei Schritt 2", [] { return tourSicht().schritt == 1; }, 4000));
    add(pruefe("Tour: der Doppelklick hat ein wait eingefuegt", [=] { return waitsUeberall() == *knotenVorher + 1; }));
    add(klick(tr(Str::TourClose), "###tour"));
    add(pause(0.3));

    // k-zeilen-ordnen ganz durch, jeder Schritt mit seinem Knopf in der
    // Aktionsspalte: auswahl, aenderung+, knoten+ (zweimal), rem+.
    add(tu("Tour: drei Zeilen fuer k-zeilen-ordnen", [] {
        openScriptFromMemory("//Generated by BehavEd\r\n\r\nwait ( 1.000 );\r\nwait ( 2.000 );\r\nwait ( 3.000 );\r\n", "tour_ordnen.txt");
        g_app->leftMode = 0;
    }));
    add(pause(0.4));
    ueberHelp("Liste fuer k-zeilen-ordnen");
    add(klick(std::string(tr(Str::TourStart)) + "##k-zeilen-ordnen", "###tour"));
    add(pause(0.4));
    add(klickAn("Tour: eine Zeile im Baum waehlen", [] {
        const Element* e = finde("wait", "pane0", 0, false);
        return e != nullptr ? ImVec2{e->rect.Min.x + ImGui::GetFontSize() * 2.0F, e->rect.GetCenter().y} : ImVec2{-1.0F, -1.0F};
    }));
    add(warteBis("Tour: k-zeilen-ordnen bei Schritt 2 (Zeile gewaehlt)", [] { return tourSicht().schritt == 1; }, 4000));
    add(pruefe("Tour: Schritt 2 zeigt Move up und Move down als einen Kasten", [=] {
        melde("Ordnen 2");
        const TourSicht v = tourSicht();
        const Element* hoch = finde(tr(Str::ActMoveUp), "###main", 0, false);
        const Element* runter = finde(tr(Str::ActMoveDown), "###main", 0, false);
        return v.ziele.size() == 1 && hoch != nullptr && runter != nullptr && v.ziele[0].Contains(hoch->rect.GetCenter()) &&
               v.ziele[0].Contains(runter->rect.GetCenter());
    }));
    add(klick(tr(Str::ActMoveDown), "###main"));
    add(warteBis("Tour: k-zeilen-ordnen bei Schritt 3 (verschoben)", [] { return tourSicht().schritt == 2; }, 4000));
    add(klick(tr(Str::ActClone), "###main"));
    add(warteBis("Tour: k-zeilen-ordnen bei Schritt 4 (dupliziert)", [] { return tourSicht().schritt == 3; }, 4000));
    add(pruefe("Tour: Schritt 4 zeigt auf Copy (1) und Paste (2)", [=] {
        melde("Ordnen 4");
        return tourSicht().ziele.size() == 2;
    }));
    add(klick(tr(Str::ActCopy), "###main"));
    add(pause(0.2));
    add(klick(tr(Str::ActPaste), "###main"));
    add(warteBis("Tour: k-zeilen-ordnen bei Schritt 5 (eingefuegt)", [] { return tourSicht().schritt == 4; }, 4000));
    add(klick(tr(Str::ActRem), "###main"));
    add(warteBis("Tour: k-zeilen-ordnen ganz durch (REM)", [] { return tourSicht().fertig; }, 4000));
    add(pause(0.3));
    add(pruefe("Tour: fertig - \"Tutorial finished\", Fortschritt \"fertig\", keine Hervorhebung mehr", [] {
        const auto it = g_app->settings.tourStand.find("k-zeilen-ordnen");
        return tourSicht().fertig && !tourSicht().overlay && it != g_app->settings.tourStand.end() && it->second == "fertig";
    }));
    add(klick(tr(Str::TourClose), "###tour"));
    add(pause(0.3));

    // k-uebersetzen: ein Skript mit Fehler (ein Uebersetzen, das scheitert,
    // fragt nicht nach einem Dateinamen - kein Dialog) und einem falsch
    // geschriebenen Wert (eine Meldung fuer die Zaehler der Statuszeile).
    add(tu("Tour: Skript mit Fehler fuer k-uebersetzen", [] {
        openScriptFromMemory("//Generated by BehavEd\r\n\r\nif ( $get( FLOAT, \"seen\")$, $~$, $0$ )\r\n{\r\n\twait ( 1000.000 );\r\n}\r\n\r\n"
                             "set ( /*@SET_TYPES*/ \"SET_WALKNG\", \"true\" );\r\n",
                             "tour_uebersetzen.txt");
    }));
    add(pause(0.5));
    ueberHelp("Liste fuer k-uebersetzen");
    add(klick(std::string(tr(Str::TourStart)) + "##k-uebersetzen", "###tour"));
    add(pause(0.4));
    add(pruefe("Tour: k-uebersetzen Schritt 1 zeigt auf Compile", [=] {
        melde("Uebersetzen 1");
        const Element* k = finde(tr(Str::Compile), "###main", 0, false);
        return tourSicht().ziele.size() == 1 && k != nullptr && gleich(tourSicht().ziele[0], sichtbar(k));
    }));
    add(klick(tr(Str::Compile), "###main"));
    add(warteBis("Tour: k-uebersetzen bei Schritt 2 (uebersetzt, gescheitert)", [] { return tourSicht().schritt == 1; }, 4000));
    add(klick(tr(Str::TourNext), "###tour"));
    add(pause(0.4));
    add(pruefe("Tour: Schritt 3 zeigt auf den Zaehler in der Statuszeile", [=] {
        melde("Uebersetzen 3");
        const TourSicht v = tourSicht();
        return v.schritt == 2 && v.ziele.size() == 1 && v.ziele[0].Min.y > ImGui::GetIO().DisplaySize.y * 0.8F;
    }));
    add(klickAn("Tour: der hervorgehobene Zaehler", [=] { return zielMitte(0); }));
    add(warteBis("Tour: k-uebersetzen bei Schritt 4 (Meldungen offen)", [] { return tourSicht().schritt == 3; }, 4000));
    add(pruefe("Tour: das Meldungsfenster ist offen", [] { return g_app->messagesOpen; }));
    add(klick(tr(Str::TourClose), "###tour"));
    add(tu("Tour: Meldungen zu", [] { g_app->messagesOpen = false; }));
    add(pause(0.3));
    add(tu("Tour: aufraeumen", [=] {
        for (std::size_t i = g_app->tabs.size(); i-- > *zahl;) {
            g_app->tabs[i].doc.markSaved();
            if (static_cast<int>(i) == g_app->activeTab) { g_app->doc.markSaved(); }
            reiterSchliessen(static_cast<int>(i));
        }
        if (*vorn >= 0 && *vorn < static_cast<int>(g_app->tabs.size())) { activateTab(*vorn); }
    }));
    add(pause(0.3));
    return s;
}

// =============================================================================
// Paket tour (rc601, Code-Pruefung): was die Pruefung der Tutorials fand
// =============================================================================
//
// Je Befund ein Abschnitt, der auf dem Stand vor der Behebung fehlschlug:
//   1. k-werte-bearbeiten: Schritte 2-6 sind "manuell" und brauchen den
//      offenen Ereigniseditor - der ist ein Modal (BeginPopupModal), und ImGui
//      laesst keine Maus an Fenster ausserhalb seines Begin-Stapels
//      (imgui.cpp:5689, IsWindowWithinBeginStackOf). "Weiter" im
//      Tutorial-Fenster war mit offenem Editor nicht anklickbar.
//   2. Ueberspringen fuehrte immer `einrichten` aus - bei k-befehl-einfuegen
//      ein streunendes `wait` im aktiven Skript.
//   3. "Mach es" bei k-neues-skript Schritt 2 (nur zeigen/schweben) konnte
//      den Schritt nie erledigen.
//   4. "aenderung+" zaehlte Document::undoDepth() - die Liste endet bei
//      kMaxUndo (200); danach zaehlte kein Verschieben mehr.
//   5. Ein gemerkter Stand hinter dem Ende einer (kuerzer gewordenen) Tour:
//      "Fortsetzen" tat still nichts.
//   6. tourStarten stellte die Skriptansicht fuer immer auf Classic.
//   7. Die Abdunklung lag ueber Hinweisen, Menues und fremden Modals.
std::vector<Schritt> tourPruefungSchritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto vorn = std::make_shared<int>(-1);
    auto zahl = std::make_shared<std::size_t>(0);
    auto zaehler = std::make_shared<int>(0);
    auto statusVorher = std::make_shared<std::string>();
    auto gestartet = std::make_shared<bool>(true);
    const auto melde = [](const std::string& was) {
        const TourSicht v = tourSicht();
        std::string z = "Knopftest: Tour-Pruefung - " + was + ": aktiv " + std::to_string(v.aktiv) + ", " + v.id +
                        " Schritt " + std::to_string(v.schritt + 1) + "/" + std::to_string(v.anzahl) + ", Ziele";
        for (const ImRect& r : v.ziele) {
            char b[80];
            std::snprintf(b, sizeof(b), " [%.0f,%.0f-%.0f,%.0f]", static_cast<double>(r.Min.x), static_cast<double>(r.Min.y),
                          static_cast<double>(r.Max.x), static_cast<double>(r.Max.y));
            z += b;
        }
        z += ", Loecher " + std::to_string(v.loecher.size()) + (v.overlay ? ", abgedunkelt" : ", nicht abgedunkelt");
        diag::detail(z);
    };
    const auto waitsUeberall = [] {
        std::function<int(const std::vector<Node>&)> z = [&z](const std::vector<Node>& ns) {
            int n = 0;
            for (const Node& k : ns) { n += (k.kind == Node::Kind::Command && k.name == "wait" ? 1 : 0) + z(k.children); }
            return n;
        };
        return z(g_app->doc.script().nodes);
    };
    // Liegt der Punkt in einem Loch der Abdunklung (also nicht abgedunkelt)?
    const auto imLoch = [](ImVec2 p) {
        for (const ImRect& r : tourSicht().loecher) {
            if (r.Contains(p)) { return true; }
        }
        return false;
    };
    const auto rechteckImLoch = [](const ImRect& q) {
        for (const ImRect& r : tourSicht().loecher) {
            if (r.Contains(q)) { return true; }
        }
        return false;
    };
    const auto anzeigeRang = [](const char* name) {
        ImGuiWindow* w = ImGui::FindWindowByName(name);
        return w != nullptr ? ImGui::FindWindowDisplayIndex(w) : -1;
    };
    const auto schliessen = [&](const std::string& wann) {
        add(tu("Tour-Pruefung: " + wann + " - Tour zu", [] {
            if (tourSicht().fensterOffen || tourSicht().aktiv) { tourSchliessen(); }
        }));
        add(pause(0.2));
    };
    const auto dreiWaits = [](const char* name) {
        openScriptFromMemory("//Generated by BehavEd\r\n\r\nwait ( 1.000 );\r\nwait ( 2.000 );\r\nwait ( 3.000 );\r\n", name);
        g_app->leftMode = 0;
    };

    add(tu("Tour-Pruefung: Stand merken, Classic und Events", [=] {
        *vorn = g_app->activeTab;
        *zahl = g_app->tabs.size();
        g_app->leftMode = 0;
        g_app->settings.flussAnsicht = 0;
        for (const char* id : {"k-neues-skript", "k-befehl-einfuegen", "k-werte-bearbeiten", "k-zeilen-ordnen", "k-uebersetzen"}) {
            g_app->settings.tourStand.erase(id);
        }
    }));

    // --- 1. k-werte-bearbeiten mit offenem Ereigniseditor -----------------------
    add(tu("Tour-Pruefung: ein set fuer k-werte-bearbeiten, Tour starten", [] {
        openScriptFromMemory("//Generated by BehavEd\r\n\r\nset ( /*@SET_TYPES*/ \"SET_WALKING\", \"true\" );\r\n", "tour_werte.txt");
        g_app->leftMode = 0;
        (void)tourStarten("k-werte-bearbeiten");
    }));
    add(pause(0.5));
    add(klickAn("Tour-Pruefung: Doppelklick auf die set-Zeile (oeffnet den Editor)", [] {
        const Element* e = finde("set", "pane0", 0, false);
        return e != nullptr ? ImVec2{e->rect.Min.x + ImGui::GetFontSize() * 2.0F, e->rect.GetCenter().y} : ImVec2{-1.0F, -1.0F};
    }, 0, true));
    add(warteBis("Tour-Pruefung: Editor offen, k-werte-bearbeiten bei Schritt 2",
                 [] { return g_app->editorOpen && tourSicht().id == "k-werte-bearbeiten" && tourSicht().schritt == 1; }, 4000));
    add(pause(0.4));
    add(pruefe("Tour-Pruefung: mit offenem Editor liegt das Tutorial-Fenster VOR dem Editor (nimmt Klicks an)", [=] {
        melde("Werte 2, Editor offen");
        const int t = anzeigeRang("###tour");
        const int e = anzeigeRang("###editor");
        diag::detail("Knopftest: Tour-Pruefung - Anzeigerang Tutorial " + std::to_string(t) + ", Editor " + std::to_string(e));
        return g_app->editorOpen && t >= 0 && e >= 0 && t > e;
    }));
    for (int nach = 2; nach <= 6; ++nach) {
        add(klick(tr(Str::TourNext), "###tour"));
        add(pause(0.25));
        add(pruefe("Tour-Pruefung: \"Weiter\" mit offenem Editor - Schritt " + std::to_string(nach + 1), [nach] {
            return g_app->editorOpen && tourSicht().schritt == nach;
        }));
    }
    add(pruefe("Tour-Pruefung: Schritt 7 zeigt auf Ok (1) und Cancel (2) im Editor", [=] {
        melde("Werte 7");
        const TourSicht v = tourSicht();
        const Element* ok = finde(tr(Str::EditorOk), "###editor", 0, false);
        return v.schritt == 6 && v.ziele.size() == 2 && ok != nullptr && v.ziele[0].Contains(ok->rect.GetCenter());
    }));
    add(klick(tr(Str::TourDoIt), "###tour"));
    add(warteBis("Tour-Pruefung: \"Mach es\" schliesst den Editor - Tutorial fertig", [] { return tourSicht().fertig; }, 600));
    add(pause(0.3));
    add(pruefe("Tour-Pruefung: Editor zu und auch aus ImGuis Klappfenster-Stapel (kein Modal sperrt mehr die Maus)", [] {
        return !g_app->editorOpen && ImGui::GetTopMostPopupModal() == nullptr;
    }));
    add(klick(tr(Str::TourClose), "###tour"));
    add(pause(0.3));
    add(pruefe("Tour-Pruefung: Schliessen kam an", [] { return !tourSicht().fensterOffen; }));

    // Ging oben etwas schief, steht der Editor noch offen und naehme den
    // folgenden Abschnitten die Maus - also sicher schliessen.
    add(tu("Tour-Pruefung: Editor und Tour sicher zu", [] {
        g_app->editorOpen = false;
        if (tourSicht().fensterOffen || tourSicht().aktiv) { tourSchliessen(); }
    }));
    add(pause(0.3));

    // --- 2. Ueberspringen setzt keinen Befehl, "Mach es" schon --------------------
    add(tu("Tour-Pruefung: drei waits, k-befehl-einfuegen", [=] {
        dreiWaits("tour_pruefung.txt");
        (void)tourStarten("k-befehl-einfuegen");
    }));
    add(pause(0.5));
    add(tu("Tour-Pruefung: waits zaehlen", [=] { *zaehler = waitsUeberall(); }));
    add(klick(tr(Str::TourSkip), "###tour"));
    add(pause(0.4));
    add(pruefe("Tour-Pruefung: Ueberspringen bei k-befehl-einfuegen - Schritt 2, aber KEIN neues wait im Skript", [=] {
        diag::detail("Knopftest: Tour-Pruefung - waits vorher " + std::to_string(*zaehler) + ", jetzt " +
                     std::to_string(waitsUeberall()));
        return tourSicht().schritt == 1 && waitsUeberall() == *zaehler;
    }));
    add(tu("Tour-Pruefung: waits zaehlen", [=] { *zaehler = waitsUeberall(); }));
    add(klick(tr(Str::TourDoIt), "###tour"));
    add(warteBis("Tour-Pruefung: \"Mach es\" erledigt Schritt 2", [] { return tourSicht().schritt == 2; }, 4000));
    add(pruefe("Tour-Pruefung: \"Mach es\" hat genau ein wait eingefuegt", [=] { return waitsUeberall() == *zaehler + 1; }));
    schliessen("Einfuegen");

    // --- 3. Kein "Mach es", wo es den Schritt nicht erledigen kann -------------------
    add(tu("Tour-Pruefung: k-neues-skript ab Schritt 2", [] { (void)tourStarten("k-neues-skript", 1); }));
    add(pause(0.4));
    add(pruefe("Tour-Pruefung: k-neues-skript Schritt 2 (affect zeigen/schweben) bietet kein \"Mach es\" an", [] {
        return tourSicht().schritt == 1 && finde(tr(Str::TourDoIt), "###tour", 0, false) == nullptr &&
               finde(tr(Str::TourSkip), "###tour", 0, false) != nullptr;
    }));
    schliessen("Schritt 2");

    // --- 4. "aenderung+" mit voller Rueckgaengig-Liste --------------------------------
    add(tu("Tour-Pruefung: drei waits und mehr Aenderungen, als die Rueckgaengig-Liste haelt", [=] {
        dreiWaits("tour_aenderung.txt");
        // Das erste wait (vor ihm kann die Kopfzeile "//Generated by
        // BehavEd" als eigener Knoten stehen - Path{0} traf sie, und die
        // Schleife brach ohne eine Aenderung ab: "Rueckgaengig-Tiefe 0").
        Path p{0};
        const std::vector<Node>& ns = g_app->doc.script().nodes;
        for (std::size_t i = 0; i < ns.size(); ++i) {
            if (ns[i].kind == Node::Kind::Command && ns[i].name == "wait") {
                p = Path{i};
                break;
            }
        }
        for (std::size_t i = 0; i < Document::kMaxUndo + 10; ++i) {
            const Node* alt = nodeAt(g_app->doc.script(), p);
            if (alt == nullptr || alt->args.empty()) { break; }
            Node neu = *alt;
            neu.args[0].text = (i % 2 == 0) ? "4.000" : "1.000";
            (void)g_app->doc.replaceAt(p, std::move(neu));
        }
        rebuildTree();
        selectByPath(p);
    }));
    add(pause(0.3));
    add(pruefe("Tour-Pruefung: die Rueckgaengig-Liste ist am Anschlag (kMaxUndo)", [] {
        diag::detail("Knopftest: Tour-Pruefung - Rueckgaengig-Tiefe " + std::to_string(g_app->doc.undoDepth()));
        return g_app->doc.undoDepth() == Document::kMaxUndo;
    }));
    add(tu("Tour-Pruefung: k-zeilen-ordnen ab Schritt 2 (Move up / Move down)", [] { (void)tourStarten("k-zeilen-ordnen", 1); }));
    add(pause(0.4));
    add(klick(tr(Str::ActMoveDown), "###main"));
    add(warteBis("Tour-Pruefung: Verschieben zaehlt auch mit voller Rueckgaengig-Liste - Schritt 3",
                 [] { return tourSicht().schritt == 2; }, 600));
    schliessen("Verschieben");

    // --- 5. Gemerkter Stand hinter dem Ende der Tour ----------------------------------
    add(tu("Tour-Pruefung: Fortschritt 9/12 fuer k-neues-skript (die Tour hat heute 7 Schritte), Liste oeffnen", [] {
        g_app->settings.tourStand["k-neues-skript"] = "9/12";
        tourFensterOeffnen();
    }));
    add(pause(0.4));
    add(pruefe("Tour-Pruefung: kein \"Fortsetzen (Schritt 10)\" bei einer Tour mit 7 Schritten - nur Starten", [] {
        char c[96];
        std::snprintf(c, sizeof(c), tr(Str::TourContinue), 10);
        const bool fortsetzen = finde(std::string(c) + "##k-neues-skript", "###tour", 0, false) != nullptr;
        const bool starten = finde(std::string(tr(Str::TourStart)) + "##k-neues-skript", "###tour", 0, false) != nullptr;
        diag::detail(std::string("Knopftest: Tour-Pruefung - Fortsetzen ") + (fortsetzen ? "da" : "nicht da") + ", Starten " +
                     (starten ? "da" : "nicht da"));
        return !fortsetzen && starten;
    }));
    add(tu("Tour-Pruefung: tourStarten hinter dem Ende", [=] {
        *statusVorher = g_app->statusLines.empty() ? std::string() : g_app->statusLines.back();
        *gestartet = tourStarten("k-neues-skript", 9);
    }));
    add(pause(0.2));
    add(pruefe("Tour-Pruefung: ein Start hinter dem Ende scheitert MIT Meldung in der Statuszeile", [=] {
        const std::string z = g_app->statusLines.empty() ? std::string() : g_app->statusLines.back();
        diag::detail("Knopftest: Tour-Pruefung - Statuszeile: " + z);
        return !*gestartet && !tourSicht().aktiv && z != *statusVorher && z.find("k-neues-skript") != std::string::npos;
    }));
    add(tu("Tour-Pruefung: Fortschritt zuruecksetzen", [] { g_app->settings.tourStand.erase("k-neues-skript"); }));
    schliessen("Liste");

    // --- 6. Die Skriptansicht kommt nach der Tour zurueck --------------------------------
    add(tu("Tour-Pruefung: Ansicht Graph, k-zeilen-ordnen starten", [] {
        g_app->settings.flussAnsicht = 1;
        (void)tourStarten("k-zeilen-ordnen");
    }));
    add(pause(0.3));
    add(pruefe("Tour-Pruefung: die Tour schaltet auf Classic", [] { return g_app->settings.flussAnsicht == 0; }));
    add(klick(tr(Str::TourClose), "###tour"));
    add(pause(0.3));
    add(pruefe("Tour-Pruefung: nach Schliessen wieder Graph", [] {
        diag::detail("Knopftest: Tour-Pruefung - Ansicht nach Schliessen " + std::to_string(g_app->settings.flussAnsicht));
        return g_app->settings.flussAnsicht == 1;
    }));
    add(tu("Tour-Pruefung: Ansicht Tracks, k-zeilen-ordnen starten", [] {
        g_app->settings.flussAnsicht = 2;
        (void)tourStarten("k-zeilen-ordnen");
    }));
    add(pause(0.3));
    add(klick(tr(Str::TourAllList), "###tour"));
    add(pause(0.3));
    add(pruefe("Tour-Pruefung: nach \"Alle Tutorials\" wieder Tracks", [] { return g_app->settings.flussAnsicht == 2; }));
    schliessen("Liste nach Alle Tutorials");
    add(tu("Tour-Pruefung: Ansicht Graph, k-zeilen-ordnen ab dem letzten Schritt", [] {
        g_app->settings.flussAnsicht = 1;
        (void)tourStarten("k-zeilen-ordnen", 4);
    }));
    add(pause(0.3));
    add(klick(tr(Str::TourSkip), "###tour"));
    add(pause(0.3));
    add(pruefe("Tour-Pruefung: Tour fertig (letzter Schritt uebersprungen) - wieder Graph", [] {
        return tourSicht().fertig && g_app->settings.flussAnsicht == 1;
    }));
    schliessen("fertig");
    add(tu("Tour-Pruefung: Ansicht Graph, Tour starten, waehrend der Tour Tracks waehlen", [] {
        g_app->settings.flussAnsicht = 1;
        (void)tourStarten("k-zeilen-ordnen");
        g_app->settings.flussAnsicht = 2;
    }));
    add(pause(0.2));
    schliessen("eigene Wahl");
    add(pruefe("Tour-Pruefung: eine eigene Wahl waehrend der Tour bleibt (Tracks)", [] { return g_app->settings.flussAnsicht == 2; }));
    add(tu("Tour-Pruefung: zurueck auf Classic", [] { g_app->settings.flussAnsicht = 0; }));

    // --- 7. Die Abdunklung laesst Menues, Hinweise und fremde Modals frei -----------------
    add(tu("Tour-Pruefung: k-uebersetzen (zeigt auf Compile)", [] { (void)tourStarten("k-uebersetzen"); }));
    add(pause(0.5));
    add(pruefe("Tour-Pruefung: Hervorhebung steht", [=] {
        melde("Uebersetzen 1");
        return tourSicht().overlay;
    }));
    add(klick(tr(Str::MenuHelp), "###main"));
    add(pause(0.3));
    add(pruefe("Tour-Pruefung: ein offenes Menue ohne Ziel wird nicht abgedunkelt", [=] {
        melde("Help offen");
        const Element* e = finde(tr(Str::TourMenu), "##Menu", 0, false);
        return tourSicht().overlay && e != nullptr && imLoch(e->rect.GetCenter());
    }));
    add(klickAn("daneben", [] { return ImVec2{12.0F, ImGui::GetIO().DisplaySize.y - 6.0F}; }));
    add(pause(0.3));
    auto schwebAb = std::make_shared<double>(0.0);
    add({"Tour-Pruefung: die Maus auf \"Zeig es mir\" stehen lassen (Hinweis)", [=](int b) {
             const Element* e = finde(tr(Str::TourShowMe), "###tour", 0, false);
             if (e == nullptr) {
                 meldeFehler("Tour-Pruefung: \"Zeig es mir\" nicht gefunden");
                 return true;
             }
             if (b == 0) { *schwebAb = ImGui::GetTime(); }
             setzeMaus(e->rect.GetCenter());
             return ImGui::GetTime() - *schwebAb > 0.8;
         }});
    add(pruefe("Tour-Pruefung: ein Hinweis (Tooltip) wird nicht abgedunkelt", [=] {
        melde("Hinweis");
        int hinweise = 0;
        bool frei = true;
        for (ImGuiWindow* w : GImGui->Windows) {
            if ((w->Flags & ImGuiWindowFlags_Tooltip) == 0 || !w->Active || w->Hidden) { continue; }
            ++hinweise;
            if (!rechteckImLoch(w->Rect())) { frei = false; }
        }
        diag::detail("Knopftest: Tour-Pruefung - " + std::to_string(hinweise) + " Hinweisfenster");
        return hinweise > 0 && frei;
    }));
    add(tu("Tour-Pruefung: Maus weg, About oeffnen (ein fremdes Modal)", [] {
        setzeMaus(ImVec2{ImGui::GetIO().DisplaySize.x * 0.5F, ImGui::GetIO().DisplaySize.y * 0.5F});
        g_app->aboutOpen = true;
    }));
    add(pause(0.4));
    add(pruefe("Tour-Pruefung: waehrend About offen ist, dunkelt die Tour nichts ab", [=] {
        melde("About offen");
        return !tourSicht().overlay;
    }));
    add(tu("Tour-Pruefung: About zu", [] { g_app->aboutOpen = false; }));
    add(pause(0.4));
    add(pruefe("Tour-Pruefung: About zu - die Hervorhebung ist wieder da", [] { return tourSicht().overlay; }));
    schliessen("Abdunklung");

    add(tu("Tour-Pruefung: aufraeumen", [=] {
        g_app->editorOpen = false;
        g_app->aboutOpen = false;
        g_app->settings.flussAnsicht = 0;
        for (std::size_t i = g_app->tabs.size(); i-- > *zahl;) {
            g_app->tabs[i].doc.markSaved();
            if (static_cast<int>(i) == g_app->activeTab) { g_app->doc.markSaved(); }
            reiterSchliessen(static_cast<int>(i));
        }
        if (*vorn >= 0 && *vorn < static_cast<int>(g_app->tabs.size())) { activateTab(*vorn); }
    }));
    add(pause(0.3));
    return s;
}

std::vector<Schritt> reiterRadSchritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    const auto name = [](int k) {
        char b[48];
        std::snprintf(b, sizeof(b), "mausrad_reiter_%02d.txt", k);
        return std::string(b);
    };
    const auto findeReiter = [](const std::string& n) {
        for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
            const std::string& m = (static_cast<int>(i) == g_app->activeTab) ? g_app->shownName : g_app->tabs[i].shownName;
            if (m == n) { return static_cast<int>(i); }
        }
        return -1;
    };
    const auto mitte = [] {
        ImGuiTabBar* l = reiterLeiste();
        if (l == nullptr) { return ImVec2{-1.0F, -1.0F}; }
        return ImVec2{(l->ScrollingRectMinX + l->ScrollingRectMaxX) * 0.5F, l->BarRect.GetCenter().y};
    };
    const auto ziel = [] {
        ImGuiTabBar* l = reiterLeiste();
        return l != nullptr ? l->ScrollingTarget : -1.0F;
    };
    auto vorherName = std::make_shared<std::string>();
    auto vorher = std::make_shared<float>(0.0F);
    auto raste = std::make_shared<float>(0.0F);
    auto wahl = std::make_shared<std::pair<int, int>>();
    add(tu("Reiterrad: 30 Skripte oeffnen", [=] {
        *vorherName = g_app->shownName;
        for (int k = 0; k < 30; ++k) { openScriptFromMemory("//Generated by BehavEd\nwait ( 1.000 );\n", name(k)); }
    }));
    add(pause(0.3));
    add(tu("Reiterrad: den ersten Reiter waehlen", [] { activateTab(0); }));
    add(pause(0.8));
    add(pruefe("Reiterrad: Vorbedingung - die Leiste laeuft ueber, steht links, der letzte Reiter ist verdeckt", [=] {
        ImGuiTabBar* l = reiterLeiste();
        if (l == nullptr) { return false; }
        diag::detail("Knopftest: Reiterrad - alle " + std::to_string(l->WidthAllTabs) + ", Leiste " + std::to_string(l->BarRect.GetWidth()) +
                     ", Bereich " + std::to_string(l->ScrollingRectMinX) + ".." + std::to_string(l->ScrollingRectMaxX) + ", Ziel " +
                     std::to_string(l->ScrollingTarget));
        return l->ScrollButtonEnabled && l->ScrollingTarget < 0.5F && reiterGanzSichtbar(0) && !reiterGanzSichtbar(findeReiter(name(29)));
    }));
    add(tu("Reiterrad: Stand merken", [=] { *vorher = ziel(); *wahl = {g_app->homeTab, g_app->activeTab}; }));
    add(radAn("ueber der Reiterleiste, eine Raste nach unten", mitte, -1.0F));
    add(pause(0.2));
    add(pruefe("Reiterrad: eine Raste nach unten rollt die Leiste nach rechts", [=] {
        *raste = ziel() - *vorher;
        diag::detail("Knopftest: Reiterrad - eine Raste: " + std::to_string(*raste));
        return *raste > 1.0F;
    }));
    add(radAn("ueber der Reiterleiste, weit nach unten", mitte, -20.0F));
    add(pause(0.6));
    add(pruefe("Reiterrad: ganz rechts - der letzte Reiter ist zu sehen, der erste nicht", [=] {
        return reiterGanzSichtbar(findeReiter(name(29))) && !reiterGanzSichtbar(0);
    }));
    add(pruefe("Reiterrad: das Rad waehlt keinen anderen Reiter", [=] { return g_app->homeTab == wahl->first && g_app->activeTab == wahl->second; }));
    add(tu("Reiterrad: Stand vor dem Rad ueber dem Skript", [=] { *vorher = ziel(); }));
    add(radAn("ueber dem Skriptfeld", [] {
        const ImGuiViewport* v = ImGui::GetMainViewport();
        return ImVec2{v->Pos.x + v->Size.x * 0.5F, v->Pos.y + v->Size.y * 0.6F};
    }, 3.0F));
    add(pause(0.2));
    add(pruefe("Reiterrad: das Rad anderswo bewegt die Leiste nicht", [=] { return std::fabs(ziel() - *vorher) < 0.5F; }));
    add(radAn("ueber der Reiterleiste, weit nach oben", mitte, 20.0F));
    add(pause(0.6));
    add(pruefe("Reiterrad: ganz links - der erste Reiter ist wieder zu sehen", [=] { return ziel() < 0.5F && reiterGanzSichtbar(0); }));
    add(tu("Reiterrad: Stand vor Umschalt+Rad", [=] { *vorher = ziel(); }));
    add({"Umschalt+Rad ueber der Reiterleiste", [=](int b) {
             ImGuiIO& io = ImGui::GetIO();
             if (b == 0) { setzeMaus(mitte()); return false; }
             if (b == 1) { io.AddKeyEvent(ImGuiMod_Shift, true); return false; }
             if (b == 3) { io.AddMouseWheelEvent(0.0F, -1.0F); return false; }
             if (b == 5) { io.AddKeyEvent(ImGuiMod_Shift, false); return false; }
             return b >= 8;
         }});
    add(pause(0.2));
    add(pruefe("Reiterrad: Umschalt+Rad rollt genau eine Raste - nicht doppelt", [=] {
        const float d = ziel() - *vorher;
        diag::detail("Knopftest: Reiterrad - Umschalt+Rad: " + std::to_string(d) + ", Raste " + std::to_string(*raste));
        return d > 1.0F && std::fabs(d - *raste) < 1.0F;
    }));
    add(tu("Reiterrad: aufraeumen", [=] {
        for (int k = 0; k < 30; ++k) {
            const int i = findeReiter(name(k));
            if (i < 0) { continue; }
            g_app->tabs[static_cast<std::size_t>(i)].doc.markSaved();
            if (i == g_app->activeTab) { g_app->doc.markSaved(); }
            reiterSchliessen(i);
        }
        const int i = findeReiter(*vorherName);
        if (i >= 0) { activateTab(i); }
    }));
    add(pause(0.3));
    return s;
}

// --- Beenden nach gescheitertem "Ja, speichern" ------------------------------
//
// Eigene Funktion, nur im Modus `reiter`: der Test setzt alle anderen Reiter
// auf gesichert, damit nur nach seiner Datei gefragt wird - im Gesamtlauf
// braucht das Beenden am Ende aber das geaenderte Knopftest-Skript.
std::vector<Schritt> schutzBeendenSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto findeReiter = [](const std::string& name) {
        for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
            const std::string& n = (static_cast<int>(i) == g_app->activeTab) ? g_app->shownName
                                                                             : g_app->tabs[i].shownName;
            if (n == name) { return static_cast<int>(i); }
        }
        return -1;
    };
    const auto aendern = [](const std::string& wert) {
        if (const Node* n = nodeAt(g_app->doc.script(), Path{0}); n != nullptr && !n->args.empty()) {
            Node neu = *n;
            neu.args[0].text = wert;
            (void)g_app->doc.replaceAt(Path{0}, neu);
            rebuildTree();
        }
    };
    // (6) Beenden: "Ja" bei einer schreibgeschuetzten Datei, den Schutz NICHT
    //     aufheben - das Speichern ging nicht. Ein zweites Beenden muss
    //     wieder fragen (vorher: die Kette blieb hinter dem Reiter stehen, und
    //     beim naechsten Beenden schloss das Programm ungefragt).
    //     Als LETZTES: mit dem alten Fehler endet hier das Programm.
    auto schutzDatei = std::make_shared<std::string>(platform::executableDirectory() + "/selbsttest_schutz.txt");
    add(tu("Schutz: Arbeitsreiter, dazu eine schreibgeschuetzte Datei mit Aenderung", [=] {
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1.000 );\n", "s_arbeit.txt");
        std::error_code ec;
        std::filesystem::permissions(::bhed::utf8Pfad(*schutzDatei), std::filesystem::perms::owner_write,
                                     std::filesystem::perm_options::add, ec);
        std::ofstream f(::bhed::utf8Pfad(*schutzDatei), std::ios::binary);
        f << "//Generated by BehavEd\r\nwait ( 6.000 );\r\n";
        f.close();
        ladeSkriptDatei(*schutzDatei);
        aendern("66.000");
        // Unter Windows wird eine Datei erst schreibgeschuetzt, wenn ALLE
        // Schreibrechte weg sind (MSVC setzt dann FILE_ATTRIBUTE_READONLY).
        std::filesystem::permissions(::bhed::utf8Pfad(*schutzDatei),
                                     std::filesystem::perms::owner_write | std::filesystem::perms::group_write |
                                         std::filesystem::perms::others_write,
                                     std::filesystem::perm_options::remove, ec);
        activateTab(findeReiter("s_arbeit.txt"));
        // Alle uebrigen Reiter sauber, damit nur nach diesem gefragt wird.
        for (std::size_t k = 0; k < g_app->tabs.size(); ++k) {
            const bool aktiv = static_cast<int>(k) == g_app->activeTab;
            const std::string& pfad = aktiv ? g_app->path : g_app->tabs[k].path;
            if (pfad.find("selbsttest_schutz.txt") == std::string::npos) {
                g_app->tabs[k].doc.markSaved();
                if (aktiv) { g_app->doc.markSaved(); }
            }
        }
        g_app->settings.queryOnDiscard = false;   // ohne die Frage "Exit?"
    }));
    add(pause(0.3));
    add(tu("Schutz: Beenden anfordern", [] { g_app->wantQuit = true; }));
    add(pause(0.4));
    add(pruefe("Schutz: es wird nach der geschuetzten Datei gefragt", [] {
        diag::detail("Knopftest: Schutz - Frage offen " + std::string(g_app->askSaveOpen ? "ja" : "nein") +
                     ", aktiv " + g_app->shownName + " " + g_app->path);
        return g_app->askSaveOpen && g_app->path.find("selbsttest_schutz.txt") != std::string::npos;
    }));
    add(klick(tr(Str::AnswerYes), tr(Str::AppTitle)));
    add(pause(0.4));
    add(pruefe("Schutz: behaved bietet an, den Schreibschutz aufzuheben", [] { return g_app->frageOffen; }));
    add(klick(tr(Str::AnswerNo), "###frage"));
    add(pause(0.4));
    add(pruefe("Schutz: nach gescheitertem Speichern ist die Kette zurueckgesetzt, man steht im Arbeitsreiter", [] {
        diag::detail("Knopftest: Schutz - quitAskFrom " + std::to_string(g_app->quitAskFrom) + ", aktiv " +
                     g_app->shownName + " (" + std::to_string(g_app->activeTab) + "), Band " +
                     std::to_string(g_app->homeTab));
        return g_app->quitAskFrom == 0 && g_app->shownName == "s_arbeit.txt" &&
               g_app->homeTab == g_app->activeTab;
    }));
    add(tu("Schutz: noch einmal beenden", [] { g_app->wantQuit = true; }));
    add(pause(0.4));
    add(pruefe("Schutz: das zweite Beenden fragt WIEDER nach der Datei (statt still zu schliessen)", [] {
        diag::detail("Knopftest: Schutz - zweites Beenden, Frage offen " +
                     std::string(g_app->askSaveOpen ? "ja" : "nein"));
        return g_app->askSaveOpen;
    }));
    add(klick(tr(Str::EditorCancel), tr(Str::AppTitle)));
    add(pause(0.3));
    add(tu("Schutz: aufraeumen", [=] {
        g_app->settings.queryOnDiscard = true;
        g_app->quitAskFrom = 0;
        for (std::size_t k = 0; k < g_app->tabs.size(); ++k) {
            const bool aktiv = static_cast<int>(k) == g_app->activeTab;
            const std::string& pfad = aktiv ? g_app->path : g_app->tabs[k].path;
            const std::string& nm = aktiv ? g_app->shownName : g_app->tabs[k].shownName;
            if (pfad.find("selbsttest_schutz.txt") != std::string::npos || nm == "s_arbeit.txt") {
                g_app->tabs[k].doc.markSaved();
                if (aktiv) { g_app->doc.markSaved(); }
            }
        }
        for (const char* n : {"s_arbeit.txt"}) {
            const int i = findeReiter(n);
            if (i >= 0) { reiterSchliessen(i); }
        }
        for (std::size_t k = 0; k < g_app->tabs.size(); ++k) {
            const bool aktiv = static_cast<int>(k) == g_app->activeTab;
            const std::string& pfad = aktiv ? g_app->path : g_app->tabs[k].path;
            if (pfad.find("selbsttest_schutz.txt") != std::string::npos) {
                reiterSchliessen(static_cast<int>(k));
                break;
            }
        }
        std::error_code ec;
        std::filesystem::permissions(::bhed::utf8Pfad(*schutzDatei), std::filesystem::perms::owner_write,
                                     std::filesystem::perm_options::add, ec);
        std::filesystem::remove(::bhed::utf8Pfad(*schutzDatei), ec);
    }));
    add(pause(0.3));
    return s;
}

// --- Nur ein behaved: ein zweiter Start reicht an den laufenden weiter --------
//
// BHED_EDITORTEST=einzel mit BHED_EINZEL=<Name>: diese Instanz startet ihre
// eigene exe noch einmal (ohne BHED_EDITORTEST, wie ein Doppelklick im
// Explorer). Die zweite muss sofort mit 0 enden, und die Datei muss HIER
// ankommen - ohne zweiten Reiter, ohne die Aenderungen zu verlieren.
std::vector<Schritt> einzelSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto datei = std::make_shared<std::string>(platform::executableDirectory() + "\\selbsttest_einzel.txt");
    auto anfang = std::make_shared<std::size_t>(0);
    auto griff = std::make_shared<std::uintptr_t>(0);
    auto start = std::make_shared<std::chrono::steady_clock::time_point>();
    auto code = std::make_shared<long>(-3);
    const auto wertJetzt = [] {
        const Node* n = nodeAt(g_app->doc.script(), Path{0});
        return (n != nullptr && !n->args.empty()) ? n->args[0].text : std::string("?");
    };
    const auto aendern = [](const std::string& wert) {
        if (const Node* n = nodeAt(g_app->doc.script(), Path{0}); n != nullptr && !n->args.empty()) {
            Node neu = *n;
            neu.args[0].text = wert;
            (void)g_app->doc.replaceAt(Path{0}, neu);
            rebuildTree();
        }
    };
    const auto zweiterStart = [&](const std::string& was, std::function<std::string()> argument) {
        add(tu("Einzel: zweiter Start " + was, [=] {
            *griff = platform::starteZweiteInstanz(argument());
            *start = std::chrono::steady_clock::now();
            *code = -3;
            diag::detail("Knopftest: Einzel - zweiter Start " + was + ", Griff " + (*griff != 0 ? "da" : "FEHLT"));
        }));
        add(warteBis("Einzel: zweiter Start endet", [=] {
            const long st = platform::zweiteInstanzStand(*griff);
            const double sek = std::chrono::duration<double>(std::chrono::steady_clock::now() - *start).count();
            if (st != -1 || sek > 12.0) {
                *code = st;
                platform::zweiteInstanzSchliessen(*griff, true);   // laeuft sie noch: nur DIESEN Kindprozess
                *griff = 0;
                diag::detail("Knopftest: Einzel - zweiter Start nach " + std::to_string(sek) + " s: " +
                             (st == -1 ? std::string("lief noch, beendet") : "Code " + std::to_string(st)));
                return true;
            }
            return false;
        }, 100000));
        add(pause(0.5));
        add(pruefe("Einzel: der zweite Start " + was + " endet sofort von selbst (Code 0)",
                   [=] { return *code == 0; }));
    };
    add(tu("Einzel: Testdatei schreiben, Reiter zaehlen", [=] {
        std::ofstream f(::bhed::utf8Pfad(*datei), std::ios::binary);
        f << "//Generated by BehavEd\r\nwait ( 4.000 );\r\n";
        f.close();
        *anfang = g_app->tabs.size();
        diag::detail("Knopftest: Einzel - " + std::to_string(*anfang) + " Reiter am Anfang");
    }));
    zweiterStart("mit der Datei", [=] { return *datei; });
    add(pruefe("Einzel: die Datei ist im laufenden Fenster als neuer Reiter offen", [=] {
        diag::detail("Knopftest: Einzel - Reiter " + std::to_string(g_app->tabs.size()) + ", aktiv " + g_app->path);
        return g_app->tabs.size() == *anfang + 1 && g_app->path.find("selbsttest_einzel.txt") != std::string::npos;
    }));
    add(tu("Einzel: dort zweimal aendern, dann in einen anderen Reiter", [=] {
        aendern("44.000");
        aendern("45.000");
        activateTab(0);
    }));
    add(pause(0.3));
    zweiterStart("mit derselben Datei", [=] { return *datei; });
    add(pruefe("Einzel: kein zweiter Reiter - zurueck im alten, mit Aenderungen und Undo", [=] {
        diag::detail("Knopftest: Einzel - Reiter " + std::to_string(g_app->tabs.size()) + ", aktiv " + g_app->path +
                     ", Wert " + wertJetzt() + ", Undo " + std::to_string(g_app->doc.undoDepth()));
        return g_app->tabs.size() == *anfang + 1 &&
               g_app->path.find("selbsttest_einzel.txt") != std::string::npos && wertJetzt() == "45.000" &&
               g_app->doc.undoDepth() == 2;
    }));
    zweiterStart("ohne Datei", [] { return std::string(); });
    add(pruefe("Einzel: ohne Datei kommt kein Reiter dazu", [=] { return g_app->tabs.size() == *anfang + 1; }));
    add(tu("Einzel: aufraeumen", [=] {
        for (std::size_t k = 0; k < g_app->tabs.size(); ++k) {
            const bool aktiv = static_cast<int>(k) == g_app->activeTab;
            const std::string& pfad = aktiv ? g_app->path : g_app->tabs[k].path;
            if (pfad.find("selbsttest_einzel.txt") != std::string::npos) {
                g_app->tabs[k].doc.markSaved();
                if (aktiv) { g_app->doc.markSaved(); }
                reiterSchliessen(static_cast<int>(k));
                break;
            }
        }
        std::error_code ec;
        std::filesystem::remove(::bhed::utf8Pfad(*datei), ec);
    }));
    add(pause(0.3));
    return s;
}

// --- Die Kopfzeile eines Feldes (Skriptname) bleibt beim Rollen stehen -----
//
// shank, 03.10.: "wenn ich compare offen habe und runter scrolle sehe ich
// nicht welches script ich gerade arbeite ... immer oben anzeigen lassen".
std::vector<Schritt> kopfSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto sichtbar = [](const std::string& name) {
        const Element* e = finde(name, "", 0, false);
        if (e == nullptr) {
            diag::detail("Knopftest: Kopf - " + name + " nicht gefunden");
            return false;
        }
        const bool ja = e->rect.Min.y >= e->clip.Min.y - 0.5F && e->rect.Max.y <= e->clip.Max.y + 0.5F;
        char z[240];
        std::snprintf(z, sizeof(z), "Knopftest: Kopf - %s bei y %.0f..%.0f, sichtbar %.0f..%.0f -> %s",
                      name.c_str(), static_cast<double>(e->rect.Min.y), static_cast<double>(e->rect.Max.y),
                      static_cast<double>(e->clip.Min.y), static_cast<double>(e->clip.Max.y), ja ? "ja" : "NEIN");
        diag::detail(z);
        return ja;
    };
    const auto findeReiter = [](const std::string& name) {
        for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
            const std::string& n = (static_cast<int>(i) == g_app->activeTab) ? g_app->shownName
                                                                             : g_app->tabs[i].shownName;
            if (n == name) { return static_cast<int>(i); }
        }
        return -1;
    };
    add(tu("Kopf: zwei lange Skripte", [=] {
        openScriptFromMemory(langesSkript(1000).c_str(), "kopf_a.txt");
        openScriptFromMemory(langesSkript(2000).c_str(), "kopf_b.txt");
        activateTab(findeReiter("kopf_a.txt"));
    }));
    add(pause(0.3));
    splitMenue(s, 2);
    add(tu("Kopf: Vergleichsfeld zeigt das zweite", [=] {
        g_app->splitPanes[1].tab = findeReiter("kopf_b.txt");
        g_app->splitPanes[1].dirty = true;
    }));
    add(pause(0.5));
    add(pruefe("Kopf: vor dem Rollen sind beide Skriptnamen sichtbar",
               [=] { return sichtbar("##splitpick0") && sichtbar("##splitpick1"); }));
    add(rolleBaum("/pane1_", 1.0F));
    add(rolleBaum("/pane0_", 1.0F));
    add(pause(0.5));
    add(pruefe("Kopf: im Vergleichsfeld bleibt der Skriptname nach dem Rollen oben sichtbar",
               [=] { return sichtbar("##splitpick1"); }));
    add(pruefe("Kopf: im bearbeiteten Feld ebenso",
               [=] { return sichtbar("##splitpick0"); }));
    add(foto("kopf_gerollt"));
    splitMenue(s, 1);
    add(tu("Kopf: aufraeumen", [=] {
        for (const char* n : {"kopf_a.txt", "kopf_b.txt"}) {
            const int i = findeReiter(n);
            if (i >= 0) {
                g_app->tabs[static_cast<std::size_t>(i)].doc.markSaved();
                if (i == g_app->activeTab) { g_app->doc.markSaved(); }
                reiterSchliessen(i);
            }
        }
    }));
    add(pause(0.3));
    return s;
}

// --- Nach dem Laden einer Mission steht ihr ERSTES Skript vorn --------------
//
// Code-Pruefung 03.10.: wurde das erste Skript in einen leeren Reiter geladen
// (statt in einen neuen), rechnete loadMission "Reiter minus geladene" und
// landete auf dem ZWEITEN Skript.
// --- Code-Pruefung 03.10.: jeder Fund im laufenden Programm nachgestellt ---
//
// Modus "pruefung". Jede Probe schlaegt mit dem Stand VOR dem Fix fehl und
// geht mit dem Fix durch (so gegengeprueft). Zum Schluss der Neustart nach
// einem Update - mit einem ungesicherten Reiter.
std::vector<Schritt> pruefungSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const std::string ordner = platform::executableDirectory();
    const std::string ed = "###editor";
    const auto schreibe = [](const std::string& pfad, const std::string& text) {
        std::error_code ec;
        std::filesystem::create_directories(::bhed::utf8Pfad(pfad).parent_path(), ec);
        std::filesystem::permissions(::bhed::utf8Pfad(pfad), std::filesystem::perms::owner_write,
                                     std::filesystem::perm_options::add, ec);
        std::ofstream f(::bhed::utf8Pfad(pfad), std::ios::binary);
        f << text;
    };
    // Reiter, dessen Pfad auf `ende` endet (-1: keiner).
    const auto reiterVon = [](const std::string& ende) {
        for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
            const std::string& p = (static_cast<int>(i) == g_app->activeTab) ? g_app->path : g_app->tabs[i].path;
            if (p.size() >= ende.size() && p.compare(p.size() - ende.size(), ende.size(), ende) == 0) {
                return static_cast<int>(i);
            }
        }
        return -1;
    };
    const auto docVon = [reiterVon](const std::string& ende) -> const Document* {
        const int i = reiterVon(ende);
        if (i < 0) { return nullptr; }
        return i == g_app->activeTab ? &g_app->doc : &g_app->tabs[static_cast<std::size_t>(i)].doc;
    };
    const auto ersterWert = [docVon](const std::string& ende) {
        const Document* d = docVon(ende);
        if (d == nullptr) { return std::string("(nicht offen)"); }
        for (const Node& n : d->script().nodes) {
            if (n.kind == Node::Kind::Command && !n.args.empty()) { return n.args[0].text; }
        }
        return std::string("?");
    };
    const auto aendern = [](const std::string& wert) {
        for (std::size_t i = 0; i < g_app->doc.script().nodes.size(); ++i) {
            const Node& n = g_app->doc.script().nodes[i];
            if (n.kind != Node::Kind::Command || n.args.empty()) { continue; }
            Node neu = n;
            neu.args[0].text = wert;
            (void)g_app->doc.replaceAt(Path{i}, neu);
            rebuildTree();
            return;
        }
    };
    // In den Reiter mit diesem Pfad - fehlt er (eine Probe davor ist
    // gescheitert und hat ihn umbenannt), die Datei frisch laden. Sonst
    // liefe die naechste Probe in einem fremden Reiter weiter.
    const auto nach = [reiterVon](const std::string& ende, const std::string& pfad) {
        const int i = reiterVon(ende);
        if (i >= 0) {
            activateTab(i);
        } else {
            ladePfad(pfad);
        }
    };
    const auto sauber = [reiterVon](const std::string& ende) {
        const int i = reiterVon(ende);
        if (i < 0) { return; }
        g_app->tabs[static_cast<std::size_t>(i)].doc.markSaved();
        if (i == g_app->activeTab) { g_app->doc.markSaved(); }
    };
    const auto schliesse = [reiterVon, sauber](const std::string& ende) {
        sauber(ende);
        const int i = reiterVon(ende);
        if (i >= 0) { reiterSchliessen(i); }
    };

    // ---- 0. Ereigniseditor mit vielen Feldern passt ins Fenster ----------
    //
    // shank (11.10.): "when viewing some commands with a lot of parameters
    // like camera pan, it doesn't fit in the window". Bei 150 % ragte der
    // Editor fuer camera PAN links und rechts hinaus.
    {
        auto zoomVorher = std::make_shared<float>(1.0F);
        auto groesseVorher = std::make_shared<std::pair<int, int>>(0, 0);
        add(tu("Breite: camera PAN, Fenster 1360 x 900 wie bei shank", [=] {
            *zoomVorher = g_app->uiScale;
            *groesseVorher = platform::hauptfensterGroesse(1360, 900);
            openScriptFromMemory("//Generated by BehavEd\ncamera ( PAN, < -8.000 47.000 0.000 >, "
                                 "< 0.000 0.000 0.000 >, 0 );\n"
                                 "camera ( MOVE, < -3158.000 4736.000 78.000 >, 0 );\n"
                                 "wait ( -1.500 );\nwait ( 3000.000 );\n"
                                 "camera ( FADE, < 1.000 1.000 1.000 >, 0.000, < 1.000 1.000 1.000 >, 1.000, 2000 );\n",
                                 "pruef_breite.txt");
        }));
        add(pause(0.5));
        add(foto("schrift_100"));   // die Schrift im Baum, 100 %
        add(tu("Breite: Anzeige 150 %", [=] { g_app->uiScale = 1.5F; }));
        add(pause(0.5));
        add(oeffneWeg(Path{0}, "camera PAN"));
        add(pause(0.5));
        add(foto("editor_pan_150"));
        add(pruefe("Breite: der Editor fuer camera PAN passt bei 150 % ins Fenster", [] {
            const ImGuiWindow* w = ImGui::FindWindowByName("###editor");
            const ImGuiViewport* vp = ImGui::GetMainViewport();
            if (w == nullptr) { return false; }
            diag::detail("Knopftest: Breite - Editor " + std::to_string(static_cast<int>(w->Pos.x)) + ".." +
                         std::to_string(static_cast<int>(w->Pos.x + w->Size.x)) + ", Fenster " +
                         std::to_string(static_cast<int>(vp->WorkPos.x)) + ".." +
                         std::to_string(static_cast<int>(vp->WorkPos.x + vp->WorkSize.x)));
            return w->Pos.x >= vp->WorkPos.x && w->Pos.x + w->Size.x <= vp->WorkPos.x + vp->WorkSize.x;
        }));
        add(klick(tr(Str::EditorCancel), "###editor"));
        add(tu("Breite: aufraeumen", [=] {
            g_app->uiScale = *zoomVorher;
            if (groesseVorher->first > 0) {
                (void)platform::hauptfensterGroesse(groesseVorher->first, groesseVorher->second);
            }
            g_app->doc.markSaved();
            reiterSchliessen(g_app->activeTab);
        }));
        add(pause(0.5));
    }

    // ---- 0a. Changelog-Fenster: Datei-Menue, jede Sprache ----------------
    {
        auto spracheVorher = std::make_shared<i18n::Language>(i18n::Language::English);
        add(tu("Changelog: Sprache merken", [=] { *spracheVorher = i18n::currentLanguage(); }));
        add(klick(tr(Str::MenuHelp)));
        add(klick(tr(Str::ChangelogMenu), "##Menu"));
        add(pause(0.5));
        add(pruefe("Changelog: das Fenster ist offen und hat Eintraege", [] {
            const ImGuiWindow* w = ImGui::FindWindowByName("###changelog");
            diag::detail("Knopftest: Changelog - offen " + std::string(g_app->changelogOpen ? "ja" : "nein") +
                         ", Fenster " + (w != nullptr && w->Active ? "da" : "fehlt") + ", Eintraege " +
                         std::to_string(changelog::anzahl()));
            return g_app->changelogOpen && w != nullptr && w->Active && changelog::anzahl() > 10;
        }));
        add(foto("changelog_en"));
        add(tu("Changelog: auf Japanisch", [] { i18n::setLanguage(i18n::Language::Japanese); }));
        add(pause(0.5));
        add(foto("changelog_ja"));
        add(tu("Changelog: Sprache zurueck, schliessen", [=] {
            i18n::setLanguage(*spracheVorher);
            g_app->changelogOpen = false;
        }));
        add(pause(0.3));
    }

    // ---- 0c. Zielspiel Multiplayer: Menue, Anmerkungen ------------------
    {
        const auto mitCode = [](const char* code) {
            for (const Issue& i : g_app->issues) {
                if (i.code == code) { return true; }
            }
            return false;
        };
        add(tu("MP: Skript mit camera, Zielspiel Singleplayer", [] {
            g_app->settings.mpZiel = false;
            openScriptFromMemory("//Generated by BehavEd\ncamera ( ENABLE );\nprint ( \"hallo\" );\n", "pruef_mp.txt");
        }));
        add(pause(0.3));
        add(pruefe("MP: im Singleplayer keine MP-Anmerkung", [=] { return !mitCode("M001") && !mitCode("M007"); }));
        add(klick(tr(Str::MenuView)));
        add(klick(tr(Str::ViewTarget), "##Menu"));
        add(klick(tr(Str::TargetMp), "##Menu"));
        add(pause(0.4));
        add(pruefe("MP: ueber View > Target game > Multiplayer melden camera (M001) und print ohne ! (M007)", [=] {
            diag::detail("Knopftest: MP - Ziel " + std::string(g_app->settings.mpZiel ? "MP" : "SP") + ", " +
                         std::to_string(g_app->issues.size()) + " Anmerkungen");
            return g_app->settings.mpZiel && mitCode("M001") && mitCode("M007");
        }));
        add(foto("mp_anmerkungen"));
        add(tu("MP: zurueck auf Singleplayer, aufraeumen", [=] {
            g_app->settings.mpZiel = false;
            rebuildTree();
            schliesse("pruef_mp.txt");
            g_app->doc.markSaved();
            reiterSchliessen(g_app->activeTab);
        }));
        add(pause(0.3));
    }

    // ---- 0a2. Menue Help: About, Changelog, Updates -----------------------
    add(klick(tr(Str::MenuHelp)));
    add(pause(0.3));
    add(pruefe("Help: das Menue hat About, Changelog, Nach Updates suchen und Beim Start suchen", [] {
        const bool da = finde(tr(Str::HelpAbout), "##Menu", 0, false) != nullptr &&
                        finde(tr(Str::ChangelogMenu), "##Menu", 0, false) != nullptr &&
                        finde(tr(Str::UpdMenu), "##Menu", 0, false) != nullptr &&
                        finde(tr(Str::UpdAutoCheck), "##Menu", 0, false) != nullptr;
        const bool nichtMehrInDatei = true;   // siehe naechste Probe
        return da && nichtMehrInDatei;
    }));
    add(klick(tr(Str::HelpAbout), "##Menu"));
    add(pause(0.4));
    add(pruefe("Help: About oeffnet das About-Fenster", [] { return g_app->aboutOpen; }));
    add(klick(tr(Str::AboutClose), tr(Str::AboutTitle)));
    add(pause(0.3));
    add(klick(tr(Str::MenuFile)));
    add(pause(0.3));
    add(pruefe("Help: im Datei-Menue stehen Update und Changelog nicht mehr", [] {
        return finde(tr(Str::UpdMenu), "##Menu", 0, false) == nullptr &&
               finde(tr(Str::ChangelogMenu), "##Menu", 0, false) == nullptr;
    }));
    add(tasteMit(ImGuiKey_None, ImGuiKey_Escape, "Escape (Menue zu)"));
    add(pause(0.3));

    // ---- 0b. Lesefehler beim Oeffnen stehen in der Statusliste -----------
    //
    // @Seph (MP-Bericht, 08.10.): "Parse warnings on open are discarded" -
    // wer eine kaputte Zeile hat, erfuhr es erst (vielleicht) beim
    // Uebersetzen.
    {
        auto vorher = std::make_shared<std::size_t>(0);
        add(tu("Lesefehler: Datei mit kaputter Zeile oeffnen", [=] {
            schreibe(ordner + "\\pruef_kaputt.txt",
                     "//Generated by BehavEd\r\nwait ( 1.000 );\r\nwait ( 2.000 ;\r\n");
            *vorher = g_app->statusLines.size();
            ladePfad(ordner + "\\pruef_kaputt.txt");
        }));
        add(pause(0.3));
        add(pruefe("Lesefehler: die Statusliste nennt die kaputte Zeile beim Oeffnen", [=] {
            bool gefunden = false;
            for (std::size_t k = *vorher; k < g_app->statusLines.size(); ++k) {
                diag::detail("Knopftest: Lesefehler - Status \"" + g_app->statusLines[k] + "\"");
                if (g_app->statusLines[k].find("wait ( 2.000 ;") != std::string::npos) { gefunden = true; }
            }
            return gefunden;
        }));
        add(tu("Lesefehler: aufraeumen", [=] { schliesse("pruef_kaputt.txt"); }));
        add(pause(0.3));
    }

    // ---- 1. Doppelklick auf eine Makrozeile klappt sie um ----------------
    //
    // Der Doppelklick ruft openEditorForNode, das bei einem Makro den Baum
    // NEU BAUT - mitten in der Schleife ueber genau diese Zeilen. Danach las
    // die Schleife weiter aus `r` (Argumente, Linien, Kontextmenue): Zugriff
    // auf freigegebenen Speicher, sobald der Vektor waechst. Sichtbar macht
    // das die ASan-Fassung (build-asan); hier wird nur das Umklappen geprueft.
    {
        std::string text = "//Generated by BehavEd\r\n//$\"pruefMakro\"@300\r\n";
        for (int k = 0; k < 300; ++k) { text += "set ( \"SET_WALKING\", \"false\" );\r\n"; }
        for (int k = 0; k < 60; ++k) { text += "wait ( " + std::to_string(k + 1) + ".000 );\r\n"; }
        auto offenVorher = std::make_shared<bool>(false);
        add(tu("Makro: Skript mit grossem Makro oben", [=] {
            openScriptFromMemory(text, "pruef_makro.txt");
            g_app->doc.vergibKennungen();
            const Node* m = nodeAt(g_app->doc.script(), Path{0});
            if (m != nullptr) { g_app->expanded.setOpen(m->kennung, false); }
            rebuildTree();
        }));
        add(pause(0.4));
        const auto makroZeile = [] {
            const Element* e = finde("pruefMakro", "", 0, true);
            return e != nullptr ? ImVec2{e->rect.Min.x + 20.0F, (e->rect.Min.y + e->rect.Max.y) * 0.5F}
                                : ImVec2{-1.0F, -1.0F};
        };
        for (int runde = 0; runde < 2; ++runde) {
            add(tu("Makro: Zustand merken", [=] {
                g_app->doc.vergibKennungen();
                const Node* m = nodeAt(g_app->doc.script(), Path{0});
                *offenVorher = m != nullptr && g_app->expanded.isOpen(m->kennung);
            }));
            add(klickAn("Doppelklick auf pruefMakro", makroZeile, 0, true));
            add(pause(0.6));
            add(pruefe("Makro: Doppelklick klappt um (" + std::to_string(runde + 1) + ")", [=] {
                const Node* m = nodeAt(g_app->doc.script(), Path{0});
                const bool offen = m != nullptr && g_app->expanded.isOpen(m->kennung);
                diag::detail("Knopftest: Makro - vorher " + std::string(*offenVorher ? "offen" : "zu") + ", jetzt " +
                             (offen ? "offen" : "zu") + ", Zeilen " + std::to_string(g_app->rows.size()));
                return offen != *offenVorher;
            }));
        }
        add(tu("Makro: aufraeumen", [=] {
            for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
                const std::string& n = (static_cast<int>(i) == g_app->activeTab) ? g_app->shownName
                                                                                 : g_app->tabs[i].shownName;
                if (n == "pruef_makro.txt") {
                    g_app->tabs[i].doc.markSaved();
                    if (static_cast<int>(i) == g_app->activeTab) { g_app->doc.markSaved(); }
                    reiterSchliessen(static_cast<int>(i));
                    break;
                }
            }
        }));
        add(pause(0.3));
    }

    // ---- 1b. Langer Text im Ereigniseditor ------------------------------
    //
    // Das Textfeld hatte einen festen Puffer von 256 Byte: wer in einem
    // langen print auch nur ein Zeichen loeschte, bekam nur die ersten 254
    // zurueck ins Skript (und anhaengen ging gar nicht - der Puffer war voll).
    {
        const std::string lang = std::string(300, 'a') + "ENDE";
        add(tu("Langtext: print mit 304 Zeichen", [=] {
            openScriptFromMemory("//Generated by BehavEd\nprint ( \"" + lang + "\" );\n", "pruef_lang.txt");
        }));
        add(pause(0.3));
        add(oeffneWeg(Path{0}, "langes print"));
        // Das Feld ist breiter als der Bildschirm - in seinen sichtbaren Teil.
        add(klickAn("in das lange Feld", [ed] {
            const Element* e = finde("##v", ed, 0, false);
            if (e == nullptr) { return ImVec2{-1.0F, -1.0F}; }
            return ImVec2{std::max(e->rect.Min.x, e->clip.Min.x) + 40.0F, (e->rect.Min.y + e->rect.Max.y) * 0.5F};
        }));
        add(tasteMit(ImGuiKey_None, ImGuiKey_End, "Ende"));
        add(tasteMit(ImGuiKey_None, ImGuiKey_Backspace, "Ruecktaste"));
        add(klick(tr(Str::EditorOk), ed));
        add(pause(0.4));
        add(pruefe("Langtext: ein Zeichen am Ende geloescht - der Rest (303 Zeichen) steht im Skript", [=] {
            const Node* n = nodeAt(g_app->doc.script(), Path{0});
            const std::string t = (n != nullptr && !n->args.empty()) ? n->args[0].text : std::string();
            diag::detail("Knopftest: Langtext - " + std::to_string(t.size()) + " Zeichen, Ende \"" +
                         (t.size() >= 6 ? t.substr(t.size() - 6) : t) + "\"");
            return t == lang.substr(0, lang.size() - 1);
        }));
        add(tu("Langtext: aufraeumen", [=] {
            g_app->doc.markSaved();
            reiterSchliessen(g_app->activeTab);
        }));
        add(pause(0.3));
    }

    // ---- 2. Datei von aussen, waehrend der Editor offen ist ---------------
    //
    // rc580 (eine Instanz): ein Doppelklick im Explorer oeffnet die Datei im
    // laufenden Fenster. Stand dort der Ereigniseditor offen, wechselte der
    // Reiter darunter weg - und "Ok" schrieb den Wert in das NEUE Skript.
    const std::string pa = ordner + "\\pruef_a.txt";
    const std::string pb = ordner + "\\pruef_b.txt";
    add(tu("Aussen: zwei Dateien, A laden", [=] {
        schreibe(pa, "//Generated by BehavEd\r\nprint ( \"A\" );\r\n");
        schreibe(pb, "//Generated by BehavEd\r\nprint ( \"B\" );\r\n");
        ladePfad(pa);
    }));
    add(pause(0.3));
    add(oeffneWeg(Path{0}, "print in A"));
    add(tu("Aussen: B kommt von einer zweiten Instanz", [=] { dateiVonAussen(pb); }));
    add(pause(0.5));
    add(klick("##v", ed));
    add(tippe("Neu"));
    add(klick(tr(Str::EditorOk), ed));
    add(pause(0.6));
    add(pruefe("Aussen: Ok im Editor aendert A, nicht das von aussen geoeffnete B", [=] {
        diag::detail("Knopftest: Aussen - A \"" + ersterWert("pruef_a.txt") + "\", B \"" + ersterWert("pruef_b.txt") +
                     "\", aktiv " + g_app->path);
        return ersterWert("pruef_a.txt") == "Neu" && ersterWert("pruef_b.txt") == "B";
    }));
    add(pruefe("Aussen: B ist nach dem Schliessen des Editors offen", [=] { return reiterVon("pruef_b.txt") >= 0; }));

    // ---- 3. Save As auf einen Pfad, der nicht geht ------------------------
    add(tu("Save As: in A wechseln, Save As in einen Ordner, den es nicht gibt", [=] {
        nach("pruef_a.txt", pa);
        aendern("A1");
        (void)speichernUnterPfad(ordner + "\\gibt_es_nicht\\x.txt");
    }));
    add(pause(0.3));
    add(pruefe("Save As: gescheitert - der Reiter behaelt seinen Pfad und bleibt ungesichert", [=] {
        bool inListe = false;
        for (const std::string& r : g_app->settings.recent) {
            if (r.find("gibt_es_nicht") != std::string::npos) { inListe = true; }
        }
        diag::detail("Knopftest: Save As - Pfad " + g_app->path + ", ungesichert " +
                     (g_app->doc.dirty() ? "ja" : "nein") + ", in Zuletzt " + (inListe ? "ja" : "nein"));
        return g_app->path.find("pruef_a.txt") != std::string::npos && g_app->doc.dirty() && !inListe;
    }));

    // ---- 4. Save As auf die Datei eines anderen offenen Reiters -----------
    add(tu("Save As: A unter dem Namen von B speichern (B ist offen)", [=] {
        (void)speichernUnterPfad(pb);
    }));
    add(pause(0.3));
    add(pruefe("Save As: abgelehnt - B auf der Platte unveraendert, kein Pfad doppelt", [=] {
        int mitB = 0;
        for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
            const std::string& p = (static_cast<int>(i) == g_app->activeTab) ? g_app->path : g_app->tabs[i].path;
            if (p.find("pruef_b.txt") != std::string::npos) { ++mitB; }
        }
        const std::string platte = slurp(pb);
        diag::detail("Knopftest: Save As - " + std::to_string(mitB) + " Reiter mit pruef_b.txt, B auf der Platte " +
                     (platte.find("\"B\"") != std::string::npos ? "unveraendert" : "UEBERSCHRIEBEN"));
        return mitB == 1 && platte.find("\"B\"") != std::string::npos && g_app->path.find("pruef_a.txt") != std::string::npos;
    }));

    // ---- 5. Uebersetzen: Skript ohne Endung in einem Ordner mit Punkt -----
    const std::string punktOrdner = ordner + "\\pruef.ordner";
    add(tu("Uebersetzen: pruef.ordner\\skript (ohne Endung) laden und uebersetzen", [=] {
        std::error_code ec;
        std::filesystem::remove(::bhed::utf8Pfad(ordner + "\\pruef.ibi"), ec);
        schreibe(punktOrdner + "\\skript", "//Generated by BehavEd\r\nprint ( \"X\" );\r\n");
        ladePfad(punktOrdner + "\\skript");
        uebersetzen();
    }));
    add(pause(0.3));
    add(pruefe("Uebersetzen: die .ibi liegt neben dem Skript, nicht als pruef.ibi eine Ebene hoeher", [=] {
        const bool daneben = std::filesystem::exists(::bhed::utf8Pfad(punktOrdner + "\\skript.ibi"));
        const bool falsch = std::filesystem::exists(::bhed::utf8Pfad(ordner + "\\pruef.ibi"));
        diag::detail("Knopftest: Uebersetzen - daneben " + std::string(daneben ? "ja" : "nein") + ", pruef.ibi " +
                     (falsch ? "JA" : "nein"));
        return daneben && !falsch;
    }));
    add(tu("Uebersetzen: aufraeumen", [=] { schliesse("skript.txt"); schliesse("skript"); }));

    // ---- 6. Save all mit einer schreibgeschuetzten Datei im Hintergrund ---
    const std::string pc = ordner + "\\pruef_c.txt";
    add(tu("Schutz: C laden, aendern, schreibschuetzen; zurueck nach A, A aendern", [=] {
        schreibe(pc, "//Generated by BehavEd\r\nprint ( \"C\" );\r\n");
        ladePfad(pc);
        aendern("C2");
        std::error_code ec;
        std::filesystem::permissions(::bhed::utf8Pfad(pc),
                                     std::filesystem::perms::owner_write | std::filesystem::perms::group_write |
                                         std::filesystem::perms::others_write,
                                     std::filesystem::perm_options::remove, ec);
        nach("pruef_a.txt", pa);
        aendern("A2");
        alleSpeichern();
    }));
    add(pause(0.4));
    add(pruefe("Schutz: Save all fragt nach dem Entsperren", [] { return g_app->frageOffen; }));
    add(klick(tr(Str::AnswerYes), "###frage"));
    add(pause(0.5));
    add(pruefe("Schutz: nach Ja steht C auf der Platte (nicht nur A gespeichert)", [=] {
        const std::string platte = slurp(pc);
        const Document* c = docVon("pruef_c.txt");
        diag::detail("Knopftest: Schutz - C auf der Platte " +
                     std::string(platte.find("C2") != std::string::npos ? "neu" : "ALT") + ", C ungesichert " +
                     (c != nullptr && c->dirty() ? "JA" : "nein") + ", aktiv " + g_app->path);
        return platte.find("C2") != std::string::npos && c != nullptr && !c->dirty() &&
               g_app->path.find("pruef_a.txt") != std::string::npos;
    }));
    add(tu("Schutz: aufraeumen", [=] {
        std::error_code ec;
        std::filesystem::permissions(::bhed::utf8Pfad(pc), std::filesystem::perms::owner_write,
                                     std::filesystem::perm_options::add, ec);
        schliesse("pruef_c.txt");
    }));
    add(pause(0.3));

    // ---- 7. Save all mit geteilter Ansicht -------------------------------
    const std::string s1 = ordner + "\\pruef_s1.txt";
    const std::string s2 = ordner + "\\pruef_s2.txt";
    const std::string s3 = ordner + "\\pruef_s3.txt";
    auto vorher = std::make_shared<std::string>();
    const auto lage = [] {
        const auto name = [](int i) {
            if (i < 0 || i >= static_cast<int>(g_app->tabs.size())) { return std::string("?"); }
            const std::string& p = (i == g_app->activeTab) ? g_app->path : g_app->tabs[static_cast<std::size_t>(i)].path;
            return p.substr(p.find_last_of("/\\") + 1);
        };
        return "aktiv " + name(g_app->activeTab) + ", Band " + name(g_app->homeTab) + ", Felder " +
               std::to_string(g_app->splitCount) + ", Feld 1 " + name(g_app->splitPanes[1].tab);
    };
    add(tu("Geteilt: drei Dateien, alle geaendert, s1 vorn", [=] {
        for (const std::string& p : {s1, s2, s3}) {
            schreibe(p, "//Generated by BehavEd\r\nprint ( \"s\" );\r\n");
            ladePfad(p);
            aendern("s neu");
        }
        nach("pruef_s1.txt", s1);
    }));
    add(pause(0.3));
    splitMenue(s, 2);
    add(tu("Geteilt: zweites Feld zeigt s2", [=] {
        g_app->splitPanes[1].tab = reiterVon("pruef_s2.txt");
        g_app->splitPanes[1].dirty = true;
    }));
    add(pause(0.3));
    add(tu("Geteilt: Lage merken, Save all", [=] {
        *vorher = lage();
        alleSpeichern();
    }));
    add(pause(0.4));
    add(pruefe("Geteilt: Save all laesst die Ansicht, wie sie war", [=] {
        diag::detail("Knopftest: Geteilt - vorher " + *vorher + " / nachher " + lage());
        return lage() == *vorher;
    }));
    splitMenue(s, 1);
    add(tu("Geteilt: aufraeumen", [=] {
        for (const char* n : {"pruef_s1.txt", "pruef_s2.txt", "pruef_s3.txt"}) { schliesse(n); }
    }));
    add(pause(0.3));

    // ---- 8. Sicherungen zweier Skripte mit gleichem Namen ----------------
    const std::string g1 = ordner + "\\pruef_d1\\gleich.txt";
    const std::string g2 = ordner + "\\pruef_d2\\gleich.txt";
    const auto sicherungenMit = [ordner](const std::string& inhalt) {
        // Ordner backup/gleich* durchsuchen: steht `inhalt` in einer backup1.txt?
        std::error_code ec;
        for (const auto& e : std::filesystem::directory_iterator(::bhed::utf8Pfad(ordner + "\\backup"), ec)) {
            if (e.path().filename().string().rfind("gleich", 0) != 0) { continue; }
            if (slurp((e.path() / "backup1.txt").string()).find(inhalt) != std::string::npos) { return true; }
        }
        return false;
    };
    add(tu("Sicherung: zwei Ordner mit gleich.txt, beide aendern und speichern", [=] {
        std::error_code ec;
        for (const auto& e : std::filesystem::directory_iterator(::bhed::utf8Pfad(ordner + "\\backup"), ec)) {
            if (e.path().filename().string().rfind("gleich", 0) == 0) { std::filesystem::remove_all(e.path(), ec); }
        }
        schreibe(g1, "//Generated by BehavEd\r\nprint ( \"eins\" );\r\n");
        schreibe(g2, "//Generated by BehavEd\r\nprint ( \"zwei\" );\r\n");
        ladePfad(g1);
        aendern("eins neu");
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_S, "Strg+S (d1)"));
    add(pause(0.2));
    add(tu("Sicherung: d2 laden und aendern", [=] {
        ladePfad(g2);
        aendern("zwei neu");
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_S, "Strg+S (d2)"));
    add(pause(0.3));
    add(pruefe("Sicherung: jedes der beiden Skripte hat seine eigene neueste Sicherung", [=] {
        const bool eins = sicherungenMit("eins neu");
        const bool zwei = sicherungenMit("zwei neu");
        diag::detail("Knopftest: Sicherung gleich.txt - eins " + std::string(eins ? "ja" : "NEIN") + ", zwei " +
                     (zwei ? "ja" : "NEIN"));
        return eins && zwei;
    }));
    add(tu("Sicherung: aufraeumen", [=] {
        schliesse("pruef_d1\\gleich.txt");
        schliesse("pruef_d2\\gleich.txt");
        std::error_code ec;
        for (const auto& e : std::filesystem::directory_iterator(::bhed::utf8Pfad(ordner + "\\backup"), ec)) {
            if (e.path().filename().string().rfind("gleich", 0) == 0) { std::filesystem::remove_all(e.path(), ec); }
        }
    }));
    add(pause(0.3));
    add(tu("Aussen: aufraeumen", [=] { schliesse("pruef_a.txt"); schliesse("pruef_b.txt"); }));
    add(pause(0.3));
    return s;
}

// Zum Schluss des Modus "pruefung": "Neu starten" nach einem Update, mit
// einem ungesicherten Reiter. Die Speichernfrage wird mit Nein beantwortet;
// danach MUSS das Programm neu starten. Der neue Prozess legt im Testmodus
// nur die Marke neustart_angekommen.txt an (main_win32.cpp) - die prueft
// das Laufskript, nachdem dieser Prozess zu ist.
std::vector<Schritt> neustartSchritte() {
    std::vector<Schritt> s;
    s.push_back(tu("Neustart: Marke weg, ein Reiter ungesichert", [] {
        std::error_code ec;
        std::filesystem::remove(::bhed::utf8Pfad(platform::executableDirectory() + "\\neustart_angekommen.txt"),
                                ec);
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1.000 );\n", "pruef_neustart.txt");
        if (const Node* n = nodeAt(g_app->doc.script(), Path{0}); n != nullptr && !n->args.empty()) {
            Node neu = *n;
            neu.args[0].text = "2.000";
            (void)g_app->doc.replaceAt(Path{0}, neu);
            rebuildTree();
        }
    }));
    s.push_back(tu("Zusammenfassung", [] {
        g_app->settings.queryOnDiscard = g_queryVorher;
        if (g_flussVorher >= 0) { g_app->settings.flussAnsicht = g_flussVorher; }
        char z[120];
        std::snprintf(z, sizeof(z), "Knopftest fertig: %d OK, %d FEHLER", g_ok, g_fehler);
        diag::detail(z);
        diag::detail("Knopftest: Neustart wird angefordert (Marke: neustart_angekommen.txt)");
    }));
    s.push_back(tu("Neustart wie nach einem Update", [] { updater::neuStarten(); }));
    s.push_back(antworteNein());
    return s;
}

// --- Zuletzt geoeffnete Missionen (rc599, BHED_EDITORTEST=letzte) -------------
//
// shank: "ich wuerde gerne ein recent mission tab haben im editor statt recent
// file, so dass man ein komplettes Projekt oeffnen kann".
//   BHED_MISSION_ARCHIV, BHED_MISSION_NAME wie bei missionfoto
std::vector<Schritt> letzteMissionSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const std::string archiv = std::getenv("BHED_MISSION_ARCHIV") ? std::getenv("BHED_MISSION_ARCHIV") : "";
    const std::string name = std::getenv("BHED_MISSION_NAME") ? std::getenv("BHED_MISSION_NAME") : "";
    add(tu("Letzte: Liste leer, Mission aus der pk3 laden", [=] {
        g_app->settings.recentMissions.clear();
        g_app->leftMode = 1;
        missionenAusArchiv(archiv);
    }));
    add(pause(0.5));
    add(tu("Letzte: Auswahl oder gleich geladen", [=] {
        if (g_app->missionPickOpen) { schritteEinfuegen({waehleInListe(name, false, 0, "##missionpick")}); }
    }));
    add(warteBis("Letzte: Mission geladen", [] { return !g_app->map.empty() && g_app->camTrackValid; }, 1500));
    auto merk = std::make_shared<std::pair<std::string, std::size_t>>();
    add(pruefe("Letzte: die Mission steht vorn in der Liste (Archiv und Name)", [=] {
        const std::string soll = "pk3|" + g_app->missionArchive + "|" + name;
        diag::detail("Knopftest: Letzte - " + std::string(g_app->settings.recentMissions.empty() ? "-" : g_app->settings.recentMissions[0]) +
                     ", Reiter " + std::to_string(g_app->tabs.size()) + ", Karte " + g_app->map.path);
        merk->first = g_app->map.path;
        merk->second = g_app->tabs.size();
        return !g_app->settings.recentMissions.empty() && g_app->settings.recentMissions[0] == soll;
    }));
    add(tu("Letzte: ein neuer, leerer Reiter", [] { neuerReiter(); }));
    add(pause(0.5));
    add(pruefe("Letzte: im neuen Reiter kein Skript", [] { return g_app->doc.script().nodes.size() <= 1U; }));
    add(klick(tr(Str::MenuFile), "###main"));
    add(klick(tr(Str::RecentMissions), "##Menu"));
    add(klick(name, "##Menu"));
    add(warteBis("Letzte: wieder geladen", [] { return g_app->camTrackValid && g_app->doc.script().nodes.size() > 1U; }, 1500));
    add(pruefe("Letzte: File > Recent missions oeffnet das ganze Projekt - Karte und die Reiter der Mission, das erste vorn", [=] {
        diag::detail("Knopftest: Letzte - danach aktiv " + g_app->shownName + ", erstes Skript " + g_app->missionErstesSkript +
                     ", Karte " + g_app->map.path + ", Reiter " + std::to_string(g_app->tabs.size()));
        return g_app->map.path == merk->first && g_app->tabs.size() <= merk->second + 1U &&
               g_app->doc.script().nodes.size() > 1U && g_app->missionAktiv;
    }));
    add(tu("Letzte: einen Eintrag, den es nicht mehr gibt, oeffnen", [] {
        g_app->settings.noteRecentMission("ordner|C:\\gibt\\es\\nicht\\md_weg");
        letzteMissionOeffnen("ordner|C:\\gibt\\es\\nicht\\md_weg");
    }));
    add(pruefe("Letzte: ein verschwundener Eintrag faellt aus der Liste", [] {
        for (const std::string& e : g_app->settings.recentMissions) {
            if (e.find("md_weg") != std::string::npos) { return false; }
        }
        return true;
    }));
    return s;
}

// --- rc603: Load mission zeigt die Missionen des ganzen Spiels (BHED_EDITORTEST=missionsliste) ---
//
// Gemeldet: mit den Daten des reinen JKA (base) zeigt "Load mission" keine
// Mission zur Auswahl. Karten, Entities und Skripte liegen in JKA (und JK2)
// ueber assets0..assets3 verteilt; die Auswahl suchte nur im EINEN gewaehlten
// Archiv - nur assets0 ergab Missionen, assets1 eine leere Liste. Dazu nahm
// "Load mission" den Ordner des Archivs DAUERHAFT in die Spielordner auf.
//
// Nachgestellt mit dem reinen JKA (BHED_JKA_BASIS, sonst der Steam-Ordner
// "GameData original/base"): assets1.pk3 waehlen muss t1_*, t2_*, t3_*, kor1,
// yavin1, taspir1 ... nennen, ohne die Spielordner der Einstellungen zu
// aendern und ohne Movie-Duels-Missionen dazwischen. Danach eine pk3 aus
// einem eingerichteten Spielordner (Movie Duels base): dann die Missionen von
// Movie Duels UND die des Grundspiels darin.
namespace missionsliste {
struct Zustand {
    std::string jka;                    // Ordner des reinen JKA (base)
    std::string mdBasis;                // eingerichteter Ordner mit assets1.pk3
    std::vector<std::string> vorher;    // settings.gamePaths vor dem Test
};
bool hat(const std::string& name, bool spielbar) {
    for (const Mission& m : g_app->missions) {
        if (m.name == name) { return !spielbar || m.playable(); }
    }
    return false;
}
std::string liste() {
    int spielbar = 0;
    for (const Mission& m : g_app->missions) {
        if (m.playable()) { ++spielbar; }
    }
    std::string s = std::to_string(g_app->missions.size()) + " Karten, " + std::to_string(spielbar) + " spielbar:";
    int n = 0;
    for (const Mission& m : g_app->missions) {
        if (++n > 14) {
            s += " ...";
            break;
        }
        s += " " + m.name;
    }
    return s;
}
std::string ordnerListe(const std::vector<std::string>& v) {
    std::string s;
    for (const std::string& o : v) { s += " [" + o + "]"; }
    return s.empty() ? std::string(" (keine)") : s;
}
std::vector<std::string> aktiveOrdner() {
    std::vector<std::string> aktiv;
    for (const GamePath& gp : g_app->gamePaths) { aktiv.push_back(gp.directory); }
    return aktiv;
}
std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}
// Die Skripte einer Mission (aus der Auswahl) in der Reihenfolge ihrer Reiter.
std::vector<std::string> reiterFolge(const std::string& mission) {
    std::vector<std::pair<int, std::string>> paare;
    for (const Mission& m : g_app->missions) {
        if (m.name != mission) { continue; }
        for (const Mission::Script& sc : m.scripts) {
            const int r = reiterFuerSkriptRef(sc.ref);
            if (r >= 0) { paare.emplace_back(r, sc.ref); }
        }
    }
    std::sort(paare.begin(), paare.end());
    std::vector<std::string> out;
    for (const auto& [r, ref] : paare) {
        if (out.empty() || klein(out.back()) != klein(ref)) { out.push_back(ref); }
    }
    return out;
}
}  // namespace missionsliste

// --- rc604: Jedi Outcast (BHED_EDITORTEST=jk2) -------------------------------
//
// Betreuer: "echtes Jedi Outcast" als drittes Regelwerk. Nachgestellt mit den
// Original-Daten von JK2 (BHED_JK2_BASIS, sonst der Steam-Ordner "Jedi
// Outcast/GameData/base"): Load mission mit assets0.pk3, kejim_post. Die
// Spielordner der Einstellungen bleiben unberuehrt (rc603: die JK2-Ordner
// gelten nur fuer die Sitzung).
//   1. Vorher - Regelwerk OpenJK erzwungen = wie behaved bis rc603 mit
//      JK2-Daten: keine NPC-Liste (nur NPCs.cfg), menschliche Figuren auf das
//      JKA-Skelett umgelegt (53 Knochen gegen eine .gla mit 72).
//   2. Nachher - automatisch: Jedi Outcast erkannt, Befehlssatz data/jk2,
//      jede Figur mit NPC_type hat ihr Modell, menschliche Figuren auf dem
//      JK2-Skelett (72 Knochen), Schwerter mit Griff und Farbe aus NPCs.cfg.
//   3. Danach zurueck zu den eingerichteten Ordnern: Jedi Academy / Movie
//      Duels wie zuvor, eine Mission von dort mit 53-Knochen-Skelett.
namespace jk2probe {
struct Befund {
    int mitTyp = 0;          // Figuren mit NPC_type
    int ohneModell = 0;      // davon ohne Modell (keine Beschreibung oder kein .glm)
    int menschen = 0;        // Modelle mit "_humanoid"
    int passend = 0;         // davon Knochen von Modell und Skelett gleich
    int jk2Umgelegt = 0;     // JK2-Modell auf das JKA-Skelett umgelegt
    std::size_t skelettKnochen = 0;
    bool rhang = false;      // das Skelett kennt rhang_tag_bone (Schwert in der Hand)
    std::string text;
};
Befund befund() {
    Befund b;
    std::string ohne;
    for (std::size_t i = 0; i < g_app->scene.actors.size(); ++i) {
        const Actor& a = g_app->scene.actors[i];
        if (a.npcType.empty()) { continue; }
        ++b.mitTyp;
        std::string k = a.npcType;
        for (char& c : k) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        const auto it = g_app->npcMap.find(k);
        const bool md3 = it != g_app->npcMap.end() && it->second.playerModel.empty() && !it->second.legsModel.empty();
        const bool modell = i < g_app->actorAssets.size() && g_app->actorAssets[i] != nullptr &&
                            !g_app->actorAssets[i]->model.empty();
        if (!modell && !md3) {
            ++b.ohneModell;
            if (ohne.size() < 200) { ohne += " " + a.name + "(" + a.npcType + ")"; }
            continue;
        }
        if (!modell) { continue; }
        const App::ActorAssets& aa = *g_app->actorAssets[i];
        if (aa.model.animFile.find("_humanoid") == std::string::npos) { continue; }
        ++b.menschen;
        b.jk2Umgelegt += aa.model.jk2Umgewandelt ? 1 : 0;
        if (aa.skeleton != nullptr) {
            b.skelettKnochen = aa.skeleton->anim.bones.size();
            for (const GlaBone& kn : aa.skeleton->anim.bones) { b.rhang = b.rhang || kn.name == "rhang_tag_bone"; }
            if (static_cast<int>(aa.skeleton->anim.bones.size()) == aa.model.numBones) { ++b.passend; }
        }
    }
    b.text = std::to_string(g_app->scene.actors.size()) + " Figuren, " + std::to_string(b.mitTyp) + " mit NPC_type, " +
             std::to_string(b.ohneModell) + " ohne Modell" + (ohne.empty() ? std::string() : " [" + ohne + " ]") + ", " +
             std::to_string(b.menschen) + " menschlich, davon " + std::to_string(b.passend) +
             " mit passendem Skelett, " + std::to_string(b.jk2Umgelegt) + " auf JKA umgelegt, Skelett " +
             std::to_string(b.skelettKnochen) + " Knochen" + (b.rhang ? " (rhang_tag_bone)" : "") + ", NPC-Typen " +
             std::to_string(g_app->npcMap.size());
    return b;
}
// mitModell: mindestens eine Figur hat ihr Modell (unter OpenJK-Regeln mit
// JK2-Daten hat keine eines - das ist der nachgestellte Fehler).
bool figurenGeladen(bool mitModell = true) {
    if (g_app->map.empty() || !g_app->camTrackValid || g_app->scene.actors.empty() ||
        g_app->actorAssets.size() != g_app->scene.actors.size() || !g_app->npcMapRead) {
        return false;
    }
    if (!mitModell) { return true; }
    for (const auto& a : g_app->actorAssets) {
        if (a != nullptr && !a->model.empty()) { return true; }
    }
    return false;
}
bool hatZiel(const char* wert) {
    const auto it = g_app->db.typesets.find("OBJECTIVES");
    if (it == g_app->db.typesets.end()) { return false; }
    for (const auto& e : it->second.entries) {
        if (e.name == wert) { return true; }
    }
    return false;
}
struct Zustand {
    std::string jk2;
    std::string jkaArchiv;               // eine pk3 aus den eingerichteten Ordnern (Movie Duels base)
    std::vector<std::string> vorherJka;  // settings.gamePaths
    std::vector<std::string> vorherJk2;  // settings.gamePathsJk2
    int vorherRegel = 0;
    Befund rot;
    int jk2Reiter = -1;   // rc604: Reiter mit kejim_post
    int jkaReiter = -1;   // rc604: Reiter mit md_am_sith
};
}  // namespace jk2probe

std::vector<Schritt> jk2Schritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<jk2probe::Zustand>();
    add(tu("JK2: Ausgangslage (Original-Jedi-Outcast, eingerichtete Spielordner)", [=] {
        const char* e = std::getenv("BHED_JK2_BASIS");
        z->jk2 = e != nullptr ? e : "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Jedi Outcast\\GameData\\base";
        z->vorherJka = g_app->settings.gamePaths;
        z->vorherJk2 = g_app->settings.gamePathsJk2;
        z->vorherRegel = g_app->settings.regelwerk;
        for (const std::string& o : g_app->settings.gamePaths) {
            std::error_code ec;
            if (z->jkaArchiv.empty() && std::filesystem::is_regular_file(::bhed::utf8Pfad(o + "\\assets1.pk3"), ec)) {
                z->jkaArchiv = o + "\\assets1.pk3";
            }
        }
        g_app->missionPickOpen = false;
        diag::detail("Knopftest: JK2 - Spiel \"" + z->jk2 + "\", Regelwerk vorher " +
                     regelwerkSchluessel(z->vorherRegel) + ", JKA-Archiv \"" + z->jkaArchiv + "\"");
    }));
    add(pruefe("JK2: Original-Daten (assets0.pk3) vorhanden", [=] {
        std::error_code ec;
        return std::filesystem::is_regular_file(::bhed::utf8Pfad(z->jk2 + "\\assets0.pk3"), ec);
    }));
    // --- 1. Vorher: OpenJK-Regeln erzwungen (behaved bis rc603) -------------
    add(tu("JK2 vorher: Regelwerk OpenJK erzwungen, Load mission assets0.pk3", [=] {
        g_app->settings.regelwerk = kRegelwerkOpenJK;
        (void)regelwerkPruefen();
        missionenAusArchiv(z->jk2 + "\\assets0.pk3");
    }));
    add(warteBis("JK2: kejim_post in der Auswahl", [] { return missionsliste::hat("kejim_post", true); }, 3500));
    add(pruefe("JK2: Load mission nennt 25 spielbare Missionen von Jedi Outcast", [] {
        diag::detail("Knopftest: JK2 - Missionen: " + missionsliste::liste());
        int spielbar = 0;
        for (const Mission& m : g_app->missions) { spielbar += m.playable() ? 1 : 0; }
        return spielbar == 25 && missionsliste::hat("yavin_final", true);
    }));
    add(waehleInListe("kejim_post", false, 0, "##missionpick"));
    add(warteBis("JK2 vorher: kejim_post geladen, Figuren aufgebaut", [] {
        return g_app->map.path.find("kejim_post") != std::string::npos && jk2probe::figurenGeladen(false);
    }, 3000));
    add(pruefe("JK2 vorher (nachgestellt): mit OpenJK-Regeln fehlen NPC-Modelle und das Skelett passt nicht", [=] {
        z->rot = jk2probe::befund();
        diag::detail("Knopftest: JK2 vorher (OpenJK-Regeln) - " + z->rot.text + ", Befehlssatz " +
                     (jk2probe::hatZiel("ARTUS_DETENTION_OBJ1") ? "JK2" : "JKA"));
        return !regelwerkJediOutcast() && (z->rot.ohneModell > 0 || z->rot.passend < z->rot.menschen);
    }));
    // --- 2. Nachher: automatisch -> Jedi Outcast ----------------------------
    add(tu("JK2: Regelwerk automatisch", [] {
        g_app->settings.regelwerk = kRegelwerkAuto;
        (void)regelwerkPruefen();
    }));
    add(warteBis("JK2: Jedi Outcast angewandt, Figuren neu geladen", [] {
        return regelwerkJediOutcast() && jk2probe::figurenGeladen() && g_app->npcMapRead;
    }, 3000));
    add(pruefe("JK2: Regelwerk Jedi Outcast erkannt (automatisch, am Ordner der Mission)", [] {
        diag::detail("Knopftest: JK2 - " + regelwerkStatusZeile() + ", Ghoul2 JK2 " + std::to_string(ghoul2Jk2()));
        return regelwerkJediOutcast() && ghoul2Jk2() && regelwerkSpielErmitteln() == kRegelwerkJediOutcast;
    }));
    add(pruefe("JK2: Befehlssatz data/jk2 (JK2-Missionsziele in OBJECTIVES)", [] {
        diag::detail("Knopftest: JK2 - Befehlssatz " + g_app->befehlssatzOrdner);
        return jk2probe::hatZiel("ARTUS_DETENTION_OBJ1") && g_app->befehlssatzOrdner.size() >= 3 &&
               g_app->befehlssatzOrdner.compare(g_app->befehlssatzOrdner.size() - 3, 3, "jk2") == 0;
    }));
    add(pruefe("JK2: jede Figur mit NPC_type hat ihr Modell (NPCs.cfg, kein \"ohne Modell\")", [=] {
        const jk2probe::Befund b = jk2probe::befund();
        diag::detail("Knopftest: JK2 nachher - " + b.text);
        return b.mitTyp > 0 && b.ohneModell == 0 && g_app->npcMap.count("kyle") == 1;
    }));
    add(pruefe("JK2: menschliche Figuren auf dem JK2-Skelett (72 Knochen, rhang_tag_bone), keine umgelegt", [] {
        const jk2probe::Befund b = jk2probe::befund();
        return b.menschen > 0 && b.passend == b.menschen && b.jk2Umgelegt == 0 && b.skelettKnochen == 72 && b.rhang;
    }));
    add(pruefe("JK2: der Spieler ist kyle (Modell kyle, CLASS_KYLE)", [] {
        const auto it = g_app->npcMap.find("player");
        return it != g_app->npcMap.end() && it->second.playerModel == "kyle" && it->second.klasse == "CLASS_KYLE";
    }));
    add(pruefe("JK2: Schwerter ohne .sab - Griff im Archiv, Laenge und Farbe aus Klasse und NPCs.cfg", [] {
        std::string bericht;
        bool ok = true;
        for (const char* typ : {"desann", "tavion", "luke", "kyle", "reborn", "rebornboss", "shadowtrooper", "player"}) {
            const auto it = g_app->npcMap.find(typ);
            if (it == g_app->npcMap.end()) {
                bericht += std::string(" ") + typ + ":fehlt";
                ok = false;
                continue;
            }
            Actor a;
            a.name = typ;
            a.npcType = typ;
            const int art = saberArtJk2(a, &it->second);
            const SaberArt& sa = a.saberArten[static_cast<std::size_t>(art)];
            const std::string griff = a.modelle[static_cast<std::size_t>(sa.modell)];
            std::string daten;
            const bool griffDa = readFromArchives(griff, daten);
            std::string soll = it->second.saberFarbe[0][0];
            for (char& c : soll) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
            if (soll.empty()) { soll = "red"; }
            const bool farbeOk = soll == "random" || sa.klinge[0].farbe == soll;
            ok = ok && griffDa && farbeOk;
            bericht += std::string(" ") + typ + ":" + griff.substr(griff.find("weapons2/") + 9) + "/" +
                       std::to_string(static_cast<int>(sa.klinge[0].max)) + "/" + sa.klinge[0].farbe +
                       (griffDa ? "" : "(Griff fehlt)") + (farbeOk ? "" : "(Farbe falsch)");
        }
        diag::detail("Knopftest: JK2 Schwerter -" + bericht);
        return ok;
    }));
    // rc604 fix604 (JK2 Stufe 3): das Wegnetz aus maps/kejim_post.nav und ein
    // Weg darueber mit den Spuren der Vorschau (Welt, Brush-Entities, PVS).
    add(pruefe("JK2: Wegnetz aus kejim_post.nav, eine Figur laeuft ueber das Netz (NAVNEW_MoveToGoal)", [] {
        const std::shared_ptr<const NavWelt> welt = navWeltVorschau();
        const NavNetzJk2* n = (welt && welt->netz) ? welt->netz->jk2.get() : nullptr;
        if (n == nullptr || n->knoten.size() < 194U) {
            diag::detail("Knopftest: JK2 Wegnetz fehlt");
            return false;
        }
        LaufZustand z;
        z.ort = n->knoten[1].punkt;
        z.denkNullMs = 0.0;
        LaufWerte w;
        w.zielGesetzt = true;
        w.ziel = n->knoten[193].punkt;
        w.maxTempo = 150.0F;
        w.gehTempo = 50.0F;
        w.verhaltenTaktMs = 100;
        w.fallen = true;
        const LaufPlan p = planeLauf(*welt, z, w, 0.0, 60000.0);
        int ersterKnoten = 0;
        for (const LaufTakt& t : p.takte) {
            if (ersterKnoten == 0 && t.tempo > 0) { ersterKnoten = t.stueck; }
        }
        diag::detail("Knopftest: JK2 Wegnetz - " + std::to_string(n->knoten.size()) + " Knoten" +
                     (n->ausDatei ? " aus der .nav" : "") + ", Weg 1 -> 193: erster Knoten " + std::to_string(ersterKnoten - 1) +
                     ", angekommen " + std::to_string(static_cast<int>(p.ankunftMs)) + " ms");
        return n->ausDatei && n->knoten.size() == 239U && p.ankunftMs > 0.0 && ersterKnoten > 0;
    }));
    add(pruefe("JK2: die Spielordner der Einstellungen bleiben unberuehrt", [=] {
        return g_app->settings.gamePaths == z->vorherJka && g_app->settings.gamePathsJk2 == z->vorherJk2;
    }));
    // --- rc604: zwei Missionsreiter verschiedener Spiele -------------------
    // Die Spielordner der Sitzung waren EINE Liste fuer alle Reiter: lud man
    // in einem zweiten Reiter eine Mission von Jedi Academy, zeigte der
    // JK2-Reiter danach Dateien und Regeln von Jedi Academy. Jeder Reiter mit
    // Karte behaelt jetzt die Spielordner seines Spiels.
    add(tu("Reiter: neuer leerer Reiter, Load mission (Jedi Academy)", [=] {
        z->jk2Reiter = g_app->activeTab;
        neuerReiter();
        missionenAusArchiv(z->jkaArchiv);
    }));
    add(warteBis("Reiter: md_am_sith in der Auswahl", [] { return missionsliste::hat("md_am_sith", true); }, 3500));
    add(waehleInListe("md_am_sith", false, 0, "##missionpick"));
    add(warteBis("Reiter: md_am_sith geladen, Jedi Academy", [] {
        return g_app->map.path.find("md_am_sith") != std::string::npos && !regelwerkJediOutcast();
    }, 3000));
    add(tu("Reiter: zurueck zum JK2-Reiter", [=] {
        z->jkaReiter = g_app->activeTab;
        activateTab(z->jk2Reiter);
    }));
    add(warteBis("Reiter: JK2-Reiter zeigt kejim_post", [] { return g_app->map.path.find("kejim_post") != std::string::npos; }, 600));
    add(pause(0.5));
    add(pruefe("Reiter: der JK2-Reiter behaelt die Spielordner und Regeln von Jedi Outcast", [=] {
        bool jk2Ordner = false;
        for (const std::string& o : aktiveSpielordner()) { jk2Ordner = jk2Ordner || jediOutcastOrdner(o); }
        diag::detail("Knopftest: Reiter JK2 - " + regelwerkStatusZeile() + ", JK2-Ordner " + std::to_string(jk2Ordner) +
                     ", Reiter " + std::to_string(z->jk2Reiter) + "/" + std::to_string(z->jkaReiter));
        return jk2Ordner && regelwerkJediOutcast();
    }));
    add(tu("Reiter: zum Reiter von Jedi Academy", [=] { activateTab(z->jkaReiter); }));
    add(warteBis("Reiter: md_am_sith wieder da", [] { return g_app->map.path.find("md_am_sith") != std::string::npos; }, 600));
    add(pause(0.5));
    add(pruefe("Reiter: der Reiter von Jedi Academy behaelt dessen Spielordner und Regeln", [=] {
        bool jk2Ordner = false;
        for (const std::string& o : aktiveSpielordner()) { jk2Ordner = jk2Ordner || jediOutcastOrdner(o); }
        diag::detail("Knopftest: Reiter JKA - " + regelwerkStatusZeile() + ", JK2-Ordner " + std::to_string(jk2Ordner));
        return !jk2Ordner && !regelwerkJediOutcast();
    }));
    add(tu("Reiter: zurueck zum JK2-Reiter", [=] { activateTab(z->jk2Reiter); }));
    add(pause(0.5));
    // --- 3. Danach: zurueck zu Jedi Academy / Movie Duels -------------------
    add(tu("JK2: zurueck zu den eingerichteten Spielordnern", [] {
        g_app->missionPickOpen = false;
        setzeSitzungsSpielordner({});
        (void)regelwerkPruefen();
    }));
    add(warteBis("JK2: wieder Jedi Academy", [] { return !regelwerkJediOutcast(); }, 600));
    add(pruefe("JKA danach: Regelwerk nicht JK2, Befehlssatz data/base, Ghoul2 wie JKA, eingerichtete Ordner", [=] {
        diag::detail("Knopftest: JK2 danach - " + regelwerkStatusZeile() + ", Befehlssatz " + g_app->befehlssatzOrdner);
        return !regelwerkJediOutcast() && !ghoul2Jk2() && !jk2probe::hatZiel("ARTUS_DETENTION_OBJ1") &&
               &aktiveSpielordner() == &g_app->settings.gamePaths;
    }));
    add(tu("JKA danach: Load mission aus dem eingerichteten Ordner (md_am_sith)", [=] {
        if (!z->jkaArchiv.empty()) { missionenAusArchiv(z->jkaArchiv); }
    }));
    add(warteBis("JKA danach: md_am_sith in der Auswahl", [] { return missionsliste::hat("md_am_sith", true); }, 3500));
    add(waehleInListe("md_am_sith", false, 0, "##missionpick"));
    add(warteBis("JKA danach: md_am_sith geladen, Figuren mit Modell", [] {
        return g_app->map.path.find("md_am_sith") != std::string::npos && jk2probe::figurenGeladen();
    }, 3000));
    add(pruefe("JKA danach: menschliche Figuren auf dem JKA-Skelett (53 Knochen), Regelwerk nicht JK2", [] {
        const jk2probe::Befund b = jk2probe::befund();
        diag::detail("Knopftest: JKA danach - " + b.text + ", " + regelwerkStatusZeile());
        return !regelwerkJediOutcast() && b.menschen > 0 && b.passend == b.menschen && b.skelettKnochen == 53;
    }));
    add(tu("JK2: Regelwerk wie vorher", [=] {
        g_app->missionPickOpen = false;
        g_app->settings.regelwerk = z->vorherRegel;
    }));
    return s;
}

std::vector<Schritt> missionsListeSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<missionsliste::Zustand>();
    add(tu("Missionsliste: Ausgangslage (reines JKA, eingerichtete Spielordner)", [=] {
        const char* e = std::getenv("BHED_JKA_BASIS");
        z->jka = e != nullptr ? e : "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Jedi Academy\\GameData original\\base";
        z->vorher = g_app->settings.gamePaths;
        for (const std::string& o : g_app->settings.gamePaths) {
            std::error_code ec;
            if (z->mdBasis.empty() && std::filesystem::is_regular_file(::bhed::utf8Pfad(o + "\\assets1.pk3"), ec)) {
                z->mdBasis = o;
            }
        }
        g_app->missionPickOpen = false;
        diag::detail("Knopftest: Missionsliste - reines JKA \"" + z->jka + "\", eingerichtet" +
                     missionsliste::ordnerListe(z->vorher) + ", assets1.pk3 in \"" + z->mdBasis + "\"");
    }));
    add(pruefe("Missionsliste: reines JKA (assets1.pk3) und ein eingerichteter Ordner mit assets1.pk3 vorhanden", [=] {
        std::error_code ec;
        return std::filesystem::is_regular_file(::bhed::utf8Pfad(z->jka + "\\assets1.pk3"), ec) && !z->mdBasis.empty();
    }));
    // --- 1. Eine pk3 OHNE Karten aus dem reinen JKA ------------------------
    add(tu("Missionsliste: Load mission mit assets1.pk3 des reinen JKA", [=] { missionenAusArchiv(z->jka + "\\assets1.pk3"); }));
    add(warteBis("Missionsliste: t1_sour in der Auswahl", [] { return missionsliste::hat("t1_sour", true); }, 3500));
    add(pruefe("Missionsliste: assets1.pk3 (ohne Karten) nennt die Missionen von JKA - t1_sour, t2_trip, t3_bounty, kor1, yavin1, taspir1", [] {
        diag::detail("Knopftest: Missionsliste - reines JKA: " + missionsliste::liste());
        bool alle = g_app->missionPickOpen;
        for (const char* n : {"t1_sour", "t2_trip", "t3_bounty", "kor1", "yavin1", "taspir1", "t1_inter", "vjun3"}) {
            if (!missionsliste::hat(n, true)) {
                diag::detail(std::string("Knopftest: Missionsliste - fehlt: ") + n);
                alle = false;
            }
        }
        return alle;
    }));
    // Dieselben Skriptverweise wie der bisherige Weg (findMissions ueber das
    // EINE Archiv, readBsp im Hauptfaden): die Hintergrundsuche liest Lump 0
    // in Arbeitsfaeden (readBspEntityLump) - fuer jede Karte in assets0.pk3
    // muss dasselbe herauskommen.
    add(pruefe("Missionsliste: Hintergrundsuche liefert je Karte dieselben Skriptverweise wie findMissions mit readBsp (assets0.pk3)", [=] {
        Pk3 a0;
        if (!readPk3Directory(z->jka + "\\assets0.pk3", a0)) { return false; }
        std::vector<std::string> namen;
        for (const Pk3Entry& e : a0.entries) { namen.push_back(e.name); }
        const std::vector<Mission> alt = findMissions(namen, [](const std::string& n, std::string& out) { return readFromArchives(n, out); });
        int gleich = 0;
        int verschieden = 0;
        for (const Mission& ma : alt) {
            const Mission* mn = nullptr;
            for (const Mission& m : g_app->missions) {
                if (m.name == ma.name) { mn = &m; }
            }
            bool ok = mn != nullptr && mn->scripts.size() == ma.scripts.size() && mn->entFile == ma.entFile;
            for (std::size_t i = 0; ok && i < ma.scripts.size(); ++i) {
                ok = mn->scripts[i].ref == ma.scripts[i].ref && mn->scripts[i].key == ma.scripts[i].key;
            }
            (ok ? gleich : verschieden) += 1;
            if (!ok) { diag::detail("Knopftest: Missionsliste - anders als findMissions: " + ma.name); }
        }
        diag::detail("Knopftest: Missionsliste - Vergleich mit findMissions: " + std::to_string(gleich) + " gleich, " +
                     std::to_string(verschieden) + " verschieden");
        return verschieden == 0 && gleich == static_cast<int>(alt.size()) && gleich > 50;
    }));
    add(pruefe("Missionsliste: kein Movie-Duels-Archiv dazwischen (nur das Spiel des Archivs)", [] {
        for (const Mission& m : g_app->missions) {
            if (m.name.rfind("md_", 0) == 0) { return false; }
        }
        return true;
    }));
    add(pruefe("Missionsliste: die Spielordner der Einstellungen bleiben, wie sie waren", [=] {
        diag::detail("Knopftest: Missionsliste - Einstellungen jetzt" + missionsliste::ordnerListe(g_app->settings.gamePaths) +
                     ", aktiv" + missionsliste::ordnerListe(missionsliste::aktiveOrdner()));
        return g_app->settings.gamePaths == z->vorher;
    }));
    add(tu("Missionsliste: Auswahl zu", [] { g_app->missionPickOpen = false; }));
    add(pause(0.3));
    // --- 2. Eine pk3 aus einem EINGERICHTETEN Spielordner -------------------
    add(tu("Missionsliste: Load mission mit assets1.pk3 aus dem eingerichteten Spielordner (Movie Duels base)",
           [=] { missionenAusArchiv(z->mdBasis + "\\assets1.pk3"); }));
    add(warteBis("Missionsliste: md_am_sith in der Auswahl", [] { return missionsliste::hat("md_am_sith", true); }, 3500));
    add(pruefe("Missionsliste: Missionen von Movie Duels (md_am_sith, md_ga_jedi) UND des Grundspiels (t1_sour, kor1)", [] {
        diag::detail("Knopftest: Missionsliste - eingerichtet: " + missionsliste::liste());
        return missionsliste::hat("md_am_sith", true) && missionsliste::hat("md_ga_jedi", true) &&
               missionsliste::hat("t1_sour", true) && missionsliste::hat("kor1", true);
    }));
    add(pruefe("Missionsliste: es gelten wieder genau die eingerichteten Spielordner", [=] {
        const std::vector<std::string> aktiv = missionsliste::aktiveOrdner();
        diag::detail("Knopftest: Missionsliste - aktiv" + missionsliste::ordnerListe(aktiv));
        return aktiv == z->vorher && g_app->settings.gamePaths == z->vorher;
    }));
    add(tu("Missionsliste: Auswahl zu", [] { g_app->missionPickOpen = false; }));
    add(pause(0.3));
    // --- 3. t1_sour aus dem reinen JKA laden -------------------------------
    add(tu("Missionsliste: noch einmal assets1.pk3 des reinen JKA", [=] { missionenAusArchiv(z->jka + "\\assets1.pk3"); }));
    add(warteBis("Missionsliste: t1_sour wieder in der Auswahl", [] { return missionsliste::hat("t1_sour", true); }, 3500));
    add(waehleInListe("t1_sour", false, 0, "##missionpick"));
    add(warteBis("Missionsliste: t1_sour geladen", [] {
        return g_app->map.path == "maps/t1_sour.bsp" && g_app->camTrackValid && g_app->missionAktiv;
    }, 3000));
    add(pruefe("Missionsliste: t1_sour mit Entities und seinen Skripten aus dem reinen JKA, Einstellungen unveraendert", [=] {
        int t1 = 0;
        for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
            const bool aktiv = static_cast<int>(i) == g_app->activeTab;
            const std::string& n = aktiv ? g_app->path : g_app->tabs[i].path;
            const std::string& sp = aktiv ? g_app->skriptPfad : g_app->tabs[i].skriptPfad;
            if (n.find("t1_sour") != std::string::npos || sp.find("t1_sour") != std::string::npos) { ++t1; }
        }
        diag::detail("Knopftest: Missionsliste - geladen " + g_app->map.path + ", " + std::to_string(g_app->map.entities.size()) +
                     " Entities, " + std::to_string(t1) + " Reiter mit t1_sour-Skripten, aktiv" +
                     missionsliste::ordnerListe(missionsliste::aktiveOrdner()));
        return g_app->map.path == "maps/t1_sour.bsp" && g_app->map.entities.size() > 100 && t1 >= 5 &&
               g_app->settings.gamePaths == z->vorher;
    }));
    // --- 4. rc603: die Reiter in der Reihenfolge, in der das Spiel startet ---
    //
    // Der Betreuer: "kann er die scripts bei load mission in der Reihenfolge
    // als Tab oeffnen, wie sie ingame abgespielt werden?" - Vorher in der
    // Reihenfolge der Entities. t2_trip: info_player_start (Entity 616)
    // zielt auf t534 - zwei target_scriptrunner, t2_Trip/intro (612) und
    // T2_Trip/playerspawn (621); G_UseTargets (g_client.cpp:2432) findet sie
    // in Entity-Reihenfolge. Vorn standen aber common/use_turbo (39) und
    // common/wrong_way (69), die erst ein Trigger des Spielers startet.
    add(tu("Reihenfolge: t2_trip aus dem reinen JKA", [=] { missionenAusArchiv(z->jka + "\\assets1.pk3"); }));
    add(warteBis("Reihenfolge: t2_trip in der Auswahl", [] { return missionsliste::hat("t2_trip", true); }, 3500));
    add(waehleInListe("t2_trip", false, 0, "##missionpick"));
    add(warteBis("Reihenfolge: t2_trip geladen", [] {
        return g_app->map.path == "maps/t2_trip.bsp" && g_app->camTrackValid && g_app->missionAktiv;
    }, 3000));
    add(pruefe("Reihenfolge: t2_trip - vorn t2_Trip/intro, dann T2_Trip/playerspawn (beide vom Spielerstart), das erste aktiv", [] {
        const std::vector<std::string> folge = missionsliste::reiterFolge("t2_trip");
        std::string text;
        for (const std::string& r : folge) { text += " " + r; }
        diag::detail("Knopftest: Reihenfolge - t2_trip, Reiter:" + text + ", aktiv " + g_app->shownName);
        return folge.size() >= 2 && missionsliste::klein(folge[0]) == "t2_trip/intro" &&
               missionsliste::klein(folge[1]) == "t2_trip/playerspawn" &&
               missionsliste::klein(g_app->shownName).rfind("intro.", 0) == 0;
    }));
    // --- Aufraeumen: zurueck zu den eingerichteten Spielordnern ------------
    add(tu("Missionsliste: zurueck - eine pk3 des eingerichteten Spielordners", [=] { missionenAusArchiv(z->mdBasis + "\\assets1.pk3"); }));
    add(warteBis("Missionsliste: eingerichtete Auswahl", [] { return missionsliste::hat("md_am_sith", true); }, 3500));
    add(tu("Missionsliste: Auswahl zu", [] { g_app->missionPickOpen = false; }));
    add(pruefe("Missionsliste: danach gelten die eingerichteten Spielordner", [=] {
        return missionsliste::aktiveOrdner() == z->vorher && g_app->settings.gamePaths == z->vorher;
    }));
    // --- rc603 (Durchsicht): ein eingerichteter Spielordner ohne .pk3 -------
    //
    // setzeSitzungsSpielordner verglich die geltenden Ordner mit den
    // EINGELESENEN (g_app->gamePaths) - ein Ordner ohne .pk3 (eine Mod nur mit
    // losen Dateien, ein leerer "base") fehlt dort. Dann las JEDES "Load
    // mission" alle Spielordner neu: rescanGamePaths leert alle
    // Zwischenspeicher und laedt die Texturen der offenen Karte neu. Vorher
    // (addGamePathDirectory) geschah bei einem eingerichteten Ordner nichts.
    auto stand = std::make_shared<int>(0);
    add(tu("Ordner ohne pk3: einen leeren Spielordner einrichten", [] {
        std::error_code ec;
        const std::filesystem::path leer = std::filesystem::current_path(ec) / "leerer_spielordner";
        std::filesystem::create_directories(leer, ec);
        g_app->settings.gamePaths.push_back(leer.string());
        rescanGamePaths();
        refreshPk3List();
    }));
    add(tu("Ordner ohne pk3: Load mission (eingerichteter Ordner)", [=] { missionenAusArchiv(z->mdBasis + "\\assets1.pk3"); }));
    add(warteBis("Ordner ohne pk3: Auswahl", [] { return missionsliste::hat("md_am_sith", true); }, 3500));
    add(tu("Ordner ohne pk3: Auswahl zu, Einlesestand merken", [=] {
        g_app->missionPickOpen = false;
        *stand = g_app->texCacheStand;
    }));
    add(pause(0.3));
    add(tu("Ordner ohne pk3: noch einmal Load mission", [=] { missionenAusArchiv(z->mdBasis + "\\assets1.pk3"); }));
    add(pruefe("Ordner ohne pk3: ein zweites Load mission mit denselben Spielordnern liest sie nicht neu ein", [=] {
        diag::detail("Knopftest: Missionsliste - Einlesestand vorher " + std::to_string(*stand) + ", nachher " +
                     std::to_string(g_app->texCacheStand) + ", eingelesen " + std::to_string(g_app->gamePaths.size()) +
                     " von" + missionsliste::ordnerListe(g_app->settings.gamePaths));
        return g_app->texCacheStand == *stand;
    }));
    add(tu("Ordner ohne pk3: Auswahl zu, Ordner wieder weg", [=] {
        g_app->missionPickOpen = false;
        g_app->settings.gamePaths = z->vorher;
        rescanGamePaths();
        refreshPk3List();
    }));
    return s;
}

// --- Eine eigene Mission wie ein Anwender laden (BHED_EDITORTEST=missionfoto) ---
//
// Map-Tab -> "Load mission..." -> Mission aus einer pk3 waehlen, dann die
// Zwischensequenz an mehreren Zeitpunkten fotografieren. Fuer den Vergleich mit
// dem Spiel (eigene Mission md_tpa_jedi, 11.10.).
//   BHED_MISSION_ARCHIV  Pfad der pk3
//   BHED_MISSION_NAME    Name in der Missionsliste (z. B. md_tpa_jedi)
//   BHED_ZEIT            Zeitpunkte in ms ("5000 12000 ...")
std::vector<Schritt> missionFotoSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const std::string archiv = std::getenv("BHED_MISSION_ARCHIV") ? std::getenv("BHED_MISSION_ARCHIV") : "";
    const std::string name = std::getenv("BHED_MISSION_NAME") ? std::getenv("BHED_MISSION_NAME") : "";
    std::vector<double> zeiten;
    if (const char* z = std::getenv("BHED_ZEIT")) {
        std::istringstream is(z);
        double t = 0.0;
        while (is >> t) { zeiten.push_back(t); }
    }
    add(tu("Missionfoto: Kartenansicht, Missionen der pk3", [=] {
        g_app->leftMode = 1;
        missionenAusArchiv(archiv);
    }));
    add(pause(0.5));
    // Hat die pk3 nur EINE spielbare Mission, laedt "Load mission..." sie
    // gleich (missionenAusArchiv) - dann gibt es keine Auswahl.
    add(tu("Missionfoto: Auswahl oder gleich geladen", [=] {
        if (g_app->missionPickOpen) {
            schritteEinfuegen({waehleInListe(name, false, 0, "##missionpick")});
            return;
        }
        diag::detail("Knopftest: Missionfoto - keine Auswahl, Mission direkt geladen");
    }));
    add(warteBis("Missionfoto: Mission geladen", [] { return !g_app->map.empty() && g_app->camTrackValid; }, 1500));
    add(pause(1.0));
    add(tu("Missionfoto: Bestand", [] {
        int spawner = 0;
        for (const auto& e : g_app->map.entities) {
            if (e.classname == "NPC_spawner") { ++spawner; }
        }
        diag::detail("Knopftest: Missionfoto - aktiv " + g_app->shownName + ", Reiter " +
                     std::to_string(g_app->tabs.size()) + ", Entities " + std::to_string(g_app->map.entities.size()) +
                     ", NPC_spawner " + std::to_string(spawner) + ", Figuren " +
                     std::to_string(g_app->scene.actors.size()));
        for (const auto& a : g_app->scene.actors) {
            diag::detail("Knopftest: Missionfoto - Figur \"" + a.name + "\" Typ \"" + a.npcType + "\"");
        }
    }));
    // Einen bestimmten Reiter (BHED_REITER, Teil des Namens) aktivieren und
    // je Lauf protokollieren, welche Beinanimation mitten im Lauf spielt -
    // so laesst sich an bestehenden Skripten pruefen, ob behaved rueckwaerts
    // laufen erkennt (rc597, Vader in md_dd/cin1_jedi).
    const std::string reiter = std::getenv("BHED_REITER") ? std::getenv("BHED_REITER") : "";
    if (!reiter.empty()) {
        add(tu("Missionfoto: Reiter " + reiter, [=] {
            for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
                const bool aktiv = static_cast<int>(i) == g_app->activeTab;
                const std::string& n = aktiv ? g_app->shownName : g_app->tabs[i].shownName;
                const std::string& pf = aktiv ? g_app->path : g_app->tabs[i].path;
                const std::string name = !n.empty() ? n : fileName(pf);
                if (name.find(reiter) != std::string::npos) { g_app->tabRequest = static_cast<int>(i); }
            }
        }));
        add(pause(1.0));
        add(warteBis("Missionfoto: Szene des Reiters", [] { return g_app->camTrackValid; }, 600));
        add(tu("Missionfoto: Laeufe und ihre Animation", [] {
            diag::detail("Knopftest: Missionfoto - Reiter " + g_app->shownName);
            for (const Actor& a : g_app->scene.actors) {
                for (const ActorStep& st : a.steps) {
                    if (st.kind != ActorStep::Kind::WalkTo) { continue; }
                    const ActorState m = a.at((st.startMs + st.endMs) * 0.5);
                    char z[240];
                    std::snprintf(z, sizeof(z), "Knopftest: Missionfoto - Lauf %s %.1f-%.1f s: %s", a.name.c_str(),
                                  st.startMs / 1000.0, st.endMs / 1000.0, m.animation.c_str());
                    diag::detail(z);
                }
            }
        }));
    }
    add(foto("missionfoto_start"));
    for (const double t : zeiten) {
        add(tu("Missionfoto: zu " + std::to_string(static_cast<int>(t)) + " ms", [=] {
            g_app->playing = false;
            g_app->throughCamera = true;
            g_app->playMs = t;
            g_app->mapDirty = true;
        }));
        add(pause(0.6));
        add(foto("missionfoto_" + std::to_string(static_cast<int>(t))));
    }
    return s;
}

// --- Missionsbau im Map-Tab (BHED_EDITORTEST=missionsbau) -------------------
//
// Gewuenscht (11.10.): "alle kombinationen testen die es gibt jeder knopf wo
// man was affecten kann miteinander" - eine Fehleranalyse fuer Bugs und
// Abstuerze. Drei Teile:
//
//   1. Der Weg eines Anwenders, Knopf fuer Knopf: New mission (Fenster),
//      + Entity, Rechtsklick "New entity here", Spawnflags, NPC-Auswahl,
//      Auf den Boden, Ziehen in der Ansicht, Strg+Z/Strg+Y/Entf ueber der
//      Karte, Duplizieren, Loeschen, Skript oeffnen, Save mission (Fenster),
//      Reiterwechsel, Mission aus der gespeicherten pk3 wieder laden,
//      Rueckfragen bei Ungespeichertem.
//   2. Zufaellige Kombinationen (feste Saat, 400 Schritte) aller Handlungen,
//      nach jedem Bild Invarianten - und zwischendurch speichern und laden.
//   3. Was gespeichert ist, kommt genau so wieder.
//
// Schreibt nur in den Laufordner (BHED_MISSIONEN, Spielordner "spiel").
namespace mb {

std::string laufOrdner() { return platform::executableDirectory(); }

// Wie der Reiter heisst: aus dem Archiv shownName, von der Platte der Dateiname.
std::string reiterName(std::size_t i) {
    const bool aktiv = static_cast<int>(i) == g_app->activeTab;
    const std::string& n = aktiv ? g_app->shownName : g_app->tabs[i].shownName;
    const std::string& p = aktiv ? g_app->path : g_app->tabs[i].path;
    return !n.empty() ? n : fileName(p);
}

// Den Inspektor rollen (0 = oben, 1 = unten) - Knoepfe weiter unten sind
// sonst abgeschnitten und nicht anklickbar.
void rolleInspektor(float anteil) {
    for (ImGuiWindow* w : GImGui->Windows) {
        if (w != nullptr && std::strstr(w->Name, "entinspektor") != nullptr) {
            ImGui::SetScrollY(w, w->ScrollMax.y * anteil);
        }
    }
}

void zumReiter(const std::string& name) {
    for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
        if (reiterName(i) == name) {
            activateTab(static_cast<int>(i));
            return;
        }
    }
}

std::string verbinde(const std::string& a, const std::string& b) {
    std::string r = a;
    if (!r.empty() && r.back() != '\\' && r.back() != '/') { r += '\\'; }
    return r + b;
}

ImVec2 ansichtMitte() {
    return ImVec2{(g_app->kartenAnsicht[0] + g_app->kartenAnsicht[2]) * 0.5F,
                  (g_app->kartenAnsicht[1] + g_app->kartenAnsicht[3]) * 0.5F};
}

int zaehle(const char* klasse) {
    int n = 0;
    for (const MapEntity& e : g_app->map.entities) { n += e.classname == klasse ? 1 : 0; }
    return n;
}

const MapEntity* gewaehlt() {
    if (g_app->pickedEntity < 0 || static_cast<std::size_t>(g_app->pickedEntity) >= g_app->map.entities.size()) {
        return nullptr;
    }
    return &g_app->map.entities[static_cast<std::size_t>(g_app->pickedEntity)];
}

std::string wert(const MapEntity* e, const char* k) {
    if (e == nullptr) { return "<keine>"; }
    const std::string* v = e->find(k);
    return v != nullptr ? *v : std::string();
}

// Fuesse ueber dem Boden (wie der Inspektor), 9999 ohne Boden.
float fuesse(const MapEntity& e) {
    float o[3];
    if (!entityOrt(e, o) || g_app->geo.empty()) { return 9999.0F; }
    const ent::Def* d = ent::def(e.classname);
    const float unten = (d != nullptr && !d->brush) ? d->mins[2] : 0.0F;
    const float oben = (d != nullptr && !d->brush) ? d->maxs[2] : 8.0F;
    const float a[3] = {o[0], o[1], o[2] + std::max(oben, 1.0F)};
    const float b[3] = {o[0], o[1], o[2] - 8192.0F};
    const TraceTreffer t = traceRay(g_app->geo, a, b);
    return t.hit ? (o[2] + unten) - t.point[2] : 9999.0F;
}

// Die Invarianten nach jeder Handlung.
std::string invariantenFehler() {
    const int n = static_cast<int>(g_app->map.entities.size());
    if (g_app->pickedEntity < -1 || g_app->pickedEntity >= n) {
        return "pickedEntity " + std::to_string(g_app->pickedEntity) + " bei " + std::to_string(n);
    }
    if (g_app->entUndo.size() > 200) { return "Verlauf zu lang"; }
    for (const std::size_t h : g_app->hiddenEntities) {
        if (h >= static_cast<std::size_t>(n)) { return "hiddenEntities zeigt hinaus"; }
    }
    // Die gemerkten Entities muessen dem Stand entsprechen - sonst bringt der
    // naechste Reiterwechsel einen alten Stand zurueck.
    if (const auto it = g_app->entitiesJeKarte.find(g_app->map.path); it != g_app->entitiesJeKarte.end()) {
        if (it->second.size() != g_app->map.entities.size()) {
            return "Merkliste " + std::to_string(it->second.size()) + " != " + std::to_string(n);
        }
        for (std::size_t i = 0; i < it->second.size(); ++i) {
            if (it->second[i].keys != g_app->map.entities[i].keys) { return "Merkliste weicht ab bei " + std::to_string(i); }
        }
    }
    for (const MapEntity& e : g_app->map.entities) {
        // Abkuerzungen in Takt mit den Schluesseln.
        const std::string* c = e.find("classname");
        if ((c == nullptr ? std::string() : *c) != e.classname) { return "classname nicht in Takt"; }
    }
    return {};
}

std::vector<std::pair<std::string, std::string>> pk3Inhalt(const std::string& pfad) {
    std::vector<std::pair<std::string, std::string>> r;
    Pk3 arc;
    if (!readPk3Directory(pfad, arc)) { return r; }
    for (const Pk3Entry& e : arc.entries) {
        std::string t;
        if (readPk3File(arc, e, t)) { r.emplace_back(e.name, t); }
    }
    return r;
}

const std::string* datei(const std::vector<std::pair<std::string, std::string>>& v, const std::string& n) {
    for (const auto& [k, t] : v) {
        if (k == n) { return &t; }
    }
    return nullptr;
}

struct Zustand {
    std::string karte;          // Grundkarte
    std::string pk3;            // gespeicherte Mission
    std::string spiel;          // Spielordner im Laufordner
    int vorher = 0;
    int npc = -1;
    std::size_t undoTiefe = 0;
    std::size_t skriptUndo = 0;
    float ortVorher[3] = {0, 0, 0};
    std::vector<MapEntity> gespeichert;
    std::uint32_t saat = 20261011U;
    int fehlerZufall = 0;
    std::map<std::string, int> handlungen;
};

std::uint32_t zufall(Zustand& z) {
    z.saat = z.saat * 1664525U + 1013904223U;
    return z.saat >> 8U;
}

}  // namespace mb

// --- rc601 saber: Lichtschwerter an Waenden und gegeneinander ----------------
//
// Gemeldet: "There are no saber collisions". Nachgestellt in der geladenen
// Karte (Modus missionsbau: die neue Mission auf duel_mandalore; sonst laedt
// der Schritt sie selbst) mit einem NPC-Typ aus den Spieldaten, dessen
// Schwert weder noIdleEffect noch noWallMarks hat:
//   A) die Klinge ragt in eine Wand - im Spiel wird sie gekuerzt, es
//      spruehen Funken (sparks/spark_nosnd), es klingt saberhitwall;
//   B) zwei Klingen in BS_CINEMATIC mit SET_SABERCOLLISIONS liegen
//      aufeinander - im Spiel klirrt es (saber/saber_block, saberblock%d);
//   C) dieselben ohne SET_SABERCOLLISIONS klirren nicht.
// Stumm (g_app->stumm); die Klaenge stehen in der Mitschrift des Pakets ton.
namespace {
struct SaberTest {
    bool bereit = false;
    std::string npcTyp;
    std::vector<std::string> kandidaten;
    std::size_t naechster = 0;
    int kandidatBild = 0;
    std::string kartenVorher;
    std::vector<MapEntity> entities;
    bool getauscht = false;
    int reiter = 0;
    Camera cam;
    float ort[3]{};          // freie Stelle (info_player_start)
    float gier = 0.0F;
    bool gemessen = false;
    float wurzel[3]{};
    float spitze[3]{};
    bool wand = false;
    float wandPunkt[3]{};
    float wandNormale[3]{};
    float ort2[3]{};
    float gier2 = 0.0F;
};

std::string saberVek(const float p[3]) {
    char b[96];
    std::snprintf(b, sizeof(b), "< %.3f %.3f %.3f >", static_cast<double>(p[0]), static_cast<double>(p[1]),
                  static_cast<double>(p[2]));
    return b;
}

// Die Figuren als NPC_spawner (ohne targetname: sie stehen ab dem Start).
void saberSpawnerSetzen(SaberTest& t, const std::vector<std::pair<std::string, std::array<float, 4>>>& figuren) {
    if (!t.getauscht) {
        t.entities = g_app->map.entities;
        t.getauscht = true;
    }
    g_app->map.entities = t.entities;
    for (const auto& [name, o] : figuren) {
        char ort[96];
        char winkel[32];
        std::snprintf(ort, sizeof(ort), "%.3f %.3f %.3f", static_cast<double>(o[0]), static_cast<double>(o[1]),
                      static_cast<double>(o[2]));
        std::snprintf(winkel, sizeof(winkel), "%.3f", static_cast<double>(o[3]));
        MapEntity e;
        e.classname = "NPC_spawner";
        e.origin = ort;
        e.keys = {{"classname", "NPC_spawner"}, {"NPC_type", t.npcTyp}, {"NPC_targetname", name},
                  {"origin", ort}, {"angle", winkel}};
        g_app->map.entities.push_back(e);
    }
}

// Die Kamera schaut auf `ziel` (sonst blickt sie nach +x und die Figur
// stuende hinter ihr).
std::string saberSkript(const std::vector<std::string>& namen, bool kollisionen, const float kamera[3],
                        const float ziel[3]) {
    std::string s = "//Generated by BehavEd\n";
    for (const std::string& n : namen) {
        s += "affect ( \"" + n + "\", /*@AFFECT_TYPE*/ FLUSH )\n{\n";
        s += "\tset ( /*@SET_TYPES*/ \"SET_BEHAVIOR_STATE\", \"BS_CINEMATIC\" );\n";
        if (kollisionen) {
            s += "\tset ( /*@SET_TYPES*/ \"SET_SABERCOLLISIONS\", \"true\" );\n";
        }
        s += "\tset ( /*@SET_TYPES*/ \"SET_SABERACTIVE\", /*@BOOL_TYPES*/ \"true\" );\n}\n";
    }
    s += "camera ( /*@CAMERA_COMMANDS*/ ENABLE );\n";
    s += "camera ( /*@CAMERA_COMMANDS*/ MOVE, " + saberVek(kamera) + ", 0.000 );\n";
    {
        const float dx = ziel[0] - kamera[0];
        const float dy = ziel[1] - kamera[1];
        const float dz = ziel[2] - kamera[2];
        const float gier = std::atan2(dy, dx) * 180.0F / 3.14159265F;
        const float nick = -std::atan2(dz, std::sqrt(dx * dx + dy * dy)) * 180.0F / 3.14159265F;
        const float w[3] = {nick, gier, 0.0F};
        s += "camera ( /*@CAMERA_COMMANDS*/ PAN, " + saberVek(w) + ", < 0.000 0.000 0.000 >, 0.000 );\n";
    }
    s += "camera ( /*@CAMERA_COMMANDS*/ MOVE, " + saberVek(kamera) + ", 4000.000 );\n";
    s += "wait ( 4000.000 );\n";
    return s;
}

void saberSzene(SaberTest& t, const std::string& skript, const std::string& name) {
    g_app->playing = false;
    g_app->audio.stopAll();
    g_app->laufendeSchleifen.clear();
    tonZuruecksetzen();
    g_app->doc.markSaved();
    g_app->camTrackValid = false;   // damit "Szene aufgebaut" auf DIESE Szene wartet
    openScriptFromMemory(skript, name);
    (void)t;
    g_app->leftMode = 1;
    g_app->playAudio = true;
    g_app->abspielTempo = 1.0F;
    g_app->followCam = true;
    g_app->throughCamera = false;
    g_app->uhrAussetzen = 0;
    g_app->playMs = 0.0;
    g_app->audioUpTo = -1.0;
    klangMitschrift().clear();
    klangMitschriftAn(true);
}

int saberFigurIndex(const std::string& name) {
    for (std::size_t i = 0; i < g_app->scene.actors.size(); ++i) {
        if (g_app->scene.actors[i].name == name) { return static_cast<int>(i); }
    }
    return -1;
}

// Abspielen in 50-ms-Schritten bis bisMs (wie tonAbspielen).
Schritt saberAbspielen(const std::string& was, double bisMs) {
    return {"Schwerter: " + was + " abspielen bis " + std::to_string(static_cast<int>(bisMs)) + " ms", [=](int b) {
                g_app->playing = true;
                if (b == 0) {
                    g_app->playMs = 0.0;
                    g_app->audioUpTo = -1.0;
                    return false;
                }
                const double tt = static_cast<double>(b) * 50.0;
                if (tt > bisMs) {
                    g_app->playing = false;
                    return true;
                }
                g_app->playMs = tt;
                return false;
            }};
}

bool saberMitschriftHat(const char* teil) {
    for (const KlangMitschrift& k : klangMitschrift()) {
        if (k.art == 'E' && k.datei.find(teil) != std::string::npos) { return true; }
    }
    return false;
}
}  // namespace

std::vector<Schritt> saberSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto t = std::make_shared<SaberTest>();
    add(tu("Schwerter: Karte, NPC-Typ und freie Stelle suchen", [=] {
        t->reiter = static_cast<int>(g_app->tabs.size());
        t->cam = g_app->cam;
        t->kartenVorher = g_app->map.path;
        if (g_app->geo.empty()) {
            if (!loadMapFromArchive("maps/duel_mandalore.bsp")) {
                diag::detail("Knopftest: Schwerter - keine Karte ladbar, Probe ohne Aussage");
                return;
            }
        }
        // Ein Jedi mit Lichtschwert und Modell, dessen .sab weder
        // noIdleEffect noch noWallMarks setzt (sonst spruehen im Leerlauf
        // keine Funken - im Spiel auch nicht).
        std::vector<std::string> kandidaten;
        for (const auto& [typ, def] : g_app->npcMap) {
            if (def.saber.empty() || def.playerModel.empty()) { continue; }
            std::string w = def.weapon;
            for (char& c : w) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
            if (w != "WP_SABER") { continue; }
            std::string sk = def.saber;
            for (char& c : sk) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
            const auto sab = g_app->saberMap.find(sk);
            if (sab == g_app->saberMap.end()) { continue; }
            const SaberKontaktWerte& kw = sab->second.kontakt;
            if (kw.keinLeerlauf || kw.keineWandspuren || sab->second.numBlades != 1 || sab->second.noBlade) { continue; }
            kandidaten.push_back(typ);
        }
        // Jedi zuerst, sonst nach Namen - und der Reihe nach probieren: ein
        // Griff ohne *blade-/*flash-Bolzen zeigt keine Klinge (klingenStrecke).
        std::sort(kandidaten.begin(), kandidaten.end(), [](const std::string& x, const std::string& y) {
            const bool jx = x.find("jedi") != std::string::npos;
            const bool jy = y.find("jedi") != std::string::npos;
            return jx != jy ? jx : x < y;
        });
        t->kandidaten = kandidaten;
        if (!kandidaten.empty()) { t->npcTyp = kandidaten.front(); }
        for (const MapEntity& e : g_app->map.entities) {
            float o[3];
            if (e.classname == "info_player_start" && leseVektor(e.origin, o)) {
                for (int k = 0; k < 3; ++k) { t->ort[k] = o[k]; }
                if (const std::string* a = e.find("angle")) { t->gier = std::strtof(a->c_str(), nullptr); }
                t->bereit = true;
                break;
            }
        }
        diag::detail("Knopftest: Schwerter - Karte " + g_app->map.path + ", NPC-Typ \"" + t->npcTyp + "\" (" +
                     std::to_string(kandidaten.size()) + " passende), Stelle " + saberVek(t->ort));
        t->bereit = t->bereit && !t->npcTyp.empty();
    }));
    add(pruefe("Schwerter: Karte, NPC-Typ mit Lichtschwert und Startstelle gefunden", [=] { return t->bereit; }));

    // --- A) Die Klinge ragt in eine Wand -------------------------------------
    add({"Schwerter A: Klinge frei messen (NPC-Typen der Reihe nach)", [=](int) {
             if (!t->bereit) { return true; }
             if (t->kandidatBild == 0) {
                 t->npcTyp = t->kandidaten[t->naechster];
                 saberSpawnerSetzen(*t, {{"sabtest", {t->ort[0], t->ort[1], t->ort[2], t->gier}}});
                 const float kam[3] = {t->ort[0] + 120.0F, t->ort[1], t->ort[2] + 40.0F};
                 const float ziel[3] = {t->ort[0], t->ort[1], t->ort[2] + 20.0F};
                 saberSzene(*t, saberSkript({"sabtest"}, false, kam, ziel), "saber_frei.txt");
                 g_app->playMs = 2500.0;
             }
             ++t->kandidatBild;
             const int fi = saberFigurIndex("sabtest");
             if (t->kandidatBild > 3) {
                 for (const SaberGezeichnet& k : saberGezeichnetesBild()) {
                     if (k.figur == fi && k.fach == 0 && k.klinge == 0) {
                         for (int i = 0; i < 3; ++i) {
                             t->wurzel[i] = k.wurzel[i];
                             t->spitze[i] = k.spitze[i];
                         }
                         t->gemessen = true;
                         diag::detail("Knopftest: Schwerter A - NPC-Typ \"" + t->npcTyp + "\" zeigt seine Klinge, Griff " +
                                      saberVek(t->wurzel) + ", Spitze " + saberVek(t->spitze));
                         return true;
                     }
                 }
             }
             if (t->kandidatBild > 90) {
                 diag::detail("Knopftest: Schwerter A - NPC-Typ \"" + t->npcTyp + "\" zeigt keine Klinge, naechster");
                 t->kandidatBild = 0;
                 ++t->naechster;
                 if (t->naechster >= std::min<std::size_t>(t->kandidaten.size(), 30)) { return true; }
             }
             return false;
         }});
    add(pruefe("Schwerter A: eine Klinge frei gemessen", [=] { return t->gemessen; }));
    // rc602 kartenpruefung: die Leuchtklinge (gfx/effects/sabers/<farbe>_glow,
    // Stufe mit "glow" in sabers.shader) geht im Spiel in den Gluehpuffer
    // (RB_DrawSurfs, g_bRenderGlowingObjects) - behaved zeichnete die Klingen
    // nur ins Bild.
    auto glowVorher = std::make_shared<bool>(true);
    add(tu("Schwerter A: Gluehen an", [=] {
        *glowVorher = g_app->showGlow;
        g_app->showGlow = true;
        g_app->mapDirty = true;
    }));
    add(warteBis("gezeichnet", [] { return !g_app->mapDirty; }, 600));
    add(pause(0.2));
    add(pruefe("Schwerter A: die Leuchtklinge geht ins Gluehen", [=] {
        const int n = gpu::gluehenKlingenAufrufe();
        diag::detail("Knopftest: Schwerter A - Klingen im Gluehen " + std::to_string(n));
        g_app->showGlow = *glowVorher;
        g_app->mapDirty = true;
        return !t->gemessen || n > 0;
    }));
    add(tu("Schwerter A: Stand ins Protokoll", [=] {
        const int fi = saberFigurIndex("sabtest");
        std::string z = "Knopftest: Schwerter A - Figur " + std::to_string(fi) + " von " +
                        std::to_string(g_app->scene.actors.size()) + ", gezeichnete Klingen " +
                        std::to_string(saberGezeichnetesBild().size()) + ", Bild " + std::to_string(ImGui::GetFrameCount());
        if (fi >= 0) {
            const ActorState st = g_app->scene.actors[static_cast<std::size_t>(fi)].at(g_app->playMs);
            z += ", sichtbar " + std::to_string(st.visible) + ", Waffe " + st.waffe + ", haeltSaber " +
                 std::to_string(haeltSaber(st)) + ", Klinge " + std::to_string(klingenLaenge(st, 0, 0, g_app->playMs)) +
                 ", Start " + std::to_string(g_app->scene.actors[static_cast<std::size_t>(fi)].haveStart) +
                 ", Assets " + std::to_string(g_app->actorAssets.size()) + ", showActors " +
                 std::to_string(g_app->showActors) + ", Ort " + saberVek(st.pos) + ", gezeichnet bei " +
                 (static_cast<std::size_t>(fi) < g_app->figurGezeichnet.size()
                      ? saberVek(g_app->figurGezeichnet[static_cast<std::size_t>(fi)].data())
                      : std::string("-")) +
                 ", Modell leer " + std::to_string(g_app->actorAssets[static_cast<std::size_t>(fi)]->model.empty()) +
                 ", Kamera " + saberVek(g_app->cam.pos);
        }
        for (const SaberGezeichnet& k : saberGezeichnetesBild()) {
            z += " [" + std::to_string(k.figur) + "/" + std::to_string(k.fach) + "/" + std::to_string(k.klinge) + "]";
        }
        diag::detail(z);
    }));
    add(tu("Schwerter A: Wand suchen und die Figur so stellen, dass die Wand die Klinge in der Mitte schneidet", [=] {
        if (!t->gemessen) { return; }
        // Eine Wand auf Hoehe des Griffs, 16 Richtungen, 96..600 weit, mit
        // waagerechter Normale (traceRay gegen die gezeichneten Flaechen).
        std::string versuche;
        for (int r = 0; r < 16 && !t->wand; ++r) {
            const float w = static_cast<float>(r) * 3.14159265F / 8.0F;
            const float a[3] = {t->ort[0], t->ort[1], t->wurzel[2]};
            const float b[3] = {a[0] + std::cos(w) * 2000.0F, a[1] + std::sin(w) * 2000.0F, a[2]};
            const TraceTreffer h = traceRay(g_app->geo, a, b);
            char vz[80];
            std::snprintf(vz, sizeof(vz), " [%d: %s %.0f nz %.2f]", r, h.hit ? "Treffer" : "frei",
                          static_cast<double>(h.fraction * 2000.0F), static_cast<double>(h.normal[2]));
            versuche += vz;
            if (!h.hit || h.fraction * 2000.0F < 64.0F || std::fabs(h.normal[2]) > 0.3F) { continue; }
            for (int i = 0; i < 3; ++i) {
                t->wandPunkt[i] = h.point[i];
                t->wandNormale[i] = h.normal[i];
            }
            t->wand = true;
        }
        if (!t->wand) {
            diag::detail("Knopftest: Schwerter A - keine Wand gefunden:" + versuche);
            return;
        }
        // Die Klinge relativ zur Figur (waagerecht), gedreht auf die Wand zu.
        const float rad = 3.14159265F / 180.0F;
        const float dx = t->spitze[0] - t->wurzel[0];
        const float dy = t->spitze[1] - t->wurzel[1];
        const float lh = std::sqrt(dx * dx + dy * dy);
        const float klingeWelt = std::atan2(dy, dx);
        const float nl = std::sqrt(t->wandNormale[0] * t->wandNormale[0] + t->wandNormale[1] * t->wandNormale[1]);
        const float n[2] = {t->wandNormale[0] / nl, t->wandNormale[1] / nl};
        const float zielGier = std::atan2(-n[1], -n[0]);
        const float dreh = zielGier - klingeWelt;
        t->gier2 = t->gier + dreh / rad;
        // Der Griff relativ zum Ursprung, mitgedreht.
        const float rx = t->wurzel[0] - t->ort[0];
        const float ry = t->wurzel[1] - t->ort[1];
        const float rx2 = std::cos(dreh) * rx - std::sin(dreh) * ry;
        const float ry2 = std::sin(dreh) * rx + std::cos(dreh) * ry;
        // Der Griff 45 % der waagerechten Klinge vor der Wand, mindestens so
        // weit, dass der Kasten der Figur (16) frei bleibt.
        float vor = 0.45F * lh;
        const float griffZurWand = -(rx2 * n[0] + ry2 * n[1]);   // >0: Griff vor dem Ursprung zur Wand
        if (vor + griffZurWand < 20.0F) { vor = 20.0F - griffZurWand; }
        t->ort2[0] = t->wandPunkt[0] + n[0] * vor - rx2;
        t->ort2[1] = t->wandPunkt[1] + n[1] * vor - ry2;
        t->ort2[2] = t->ort[2];
        char z[260];
        std::snprintf(z, sizeof(z),
                      "Knopftest: Schwerter A - Klinge waagerecht %.1f lang, Wand bei %s Normale %s, Figur nach %s Gier %.1f, Griff %.1f vor der Wand",
                      static_cast<double>(lh), saberVek(t->wandPunkt).c_str(), saberVek(t->wandNormale).c_str(),
                      saberVek(t->ort2).c_str(), static_cast<double>(t->gier2), static_cast<double>(vor));
        diag::detail(z);
        saberSpawnerSetzen(*t, {{"sabtest", {t->ort2[0], t->ort2[1], t->ort2[2], t->gier2}}});
        const float kam[3] = {t->ort2[0] + n[0] * 150.0F, t->ort2[1] + n[1] * 150.0F, t->ort2[2] + 40.0F};
        const float ziel[3] = {t->ort2[0], t->ort2[1], t->ort2[2] + 20.0F};
        saberSzene(*t, saberSkript({"sabtest"}, false, kam, ziel), "saber_wand.txt");
    }));
    add(warteBis("Schwerter A: Szene an der Wand aufgebaut", [] { return g_app->camTrackValid; }, 900));
    add(saberAbspielen("Klinge an der Wand", 1500.0));
    add(tu("Schwerter A: Zeitmarke 2500 ms", [] { g_app->playMs = 2500.0; }));
    add(pause(0.3));
    add(pruefe("Schwerter A: die gezeichnete Klinge steckt nicht in der Wand (CG_AddSaberBladeGo kuerzt sie)", [=] {
        if (!t->wand) { return false; }
        const int fi = saberFigurIndex("sabtest");
        int gesehen = 0;
        bool drin = false;
        std::string log;
        for (const SaberGezeichnet& k : saberGezeichnetesBild()) {
            if (k.figur != fi) { continue; }
            ++gesehen;
            const TraceTreffer h = traceRay(g_app->geo, k.wurzel, k.spitze);
            const float l = std::sqrt((k.spitze[0] - k.wurzel[0]) * (k.spitze[0] - k.wurzel[0]) +
                                      (k.spitze[1] - k.wurzel[1]) * (k.spitze[1] - k.wurzel[1]) +
                                      (k.spitze[2] - k.wurzel[2]) * (k.spitze[2] - k.wurzel[2]));
            char z[200];
            std::snprintf(z, sizeof(z), " [Klinge %d: Laenge %.1f, %s]", k.klinge, static_cast<double>(l),
                          h.hit ? ("trifft die Wand bei " + std::to_string(h.fraction * l)).c_str() : "frei");
            log += z;
            if (h.hit) { drin = true; }
        }
        diag::detail("Knopftest: Schwerter A - gezeichnet:" + log + " | " + saberLeistung());
        return gesehen > 0 && !drin;
    }));
    add(pruefe("Schwerter A: Funken an der Wand (sparks/spark_nosnd) und Brandspur", [=] {
        int funken = 0;
        int spuren = 0;
        for (const SaberWandEreignis& e : saberWandEreignisseBis(2500.0)) {
            funken += e.art == SaberWandEreignis::Art::Funken ? 1 : 0;
            spuren += e.art == SaberWandEreignis::Art::Spur ? 1 : 0;
        }
        diag::detail("Knopftest: Schwerter A - Funken " + std::to_string(funken) + ", Spuren " + std::to_string(spuren) +
                     ", Effekt geladen: " + (effectByName("sparks/spark_nosnd") != nullptr ? "ja" : "nein"));
        return funken > 0 && spuren > 0 && effectByName("sparks/spark_nosnd") != nullptr;
    }));
    add(pruefe("Schwerter A: saberhitwall klingt (Mitschrift)", [] {
        return saberMitschriftHat("saberhitwall") || saberMitschriftHat("saberstrikewall");
    }));
    // rc601 saber review 2a: die Brandspuren werden gezeichnet - CG_CreateSaberMarks
    // schneidet sie mit R_MarkFragments auf die Wand, CG_AddMarks zeichnet sie
    // (rivetmark 10 s, saberDamageGlow 1,5 s). Vorher standen sie nur als
    // Ereignis in der Liste (NOTIZEN_saber.md 9.4).
    add(tu("Schwerter A: Brandspurzaehler auf 0, neu zeichnen", [] {
        g_saberSpurenGezeichnet = 0;
        g_app->mapDirty = true;
    }));
    add(pause(0.3));
    add(pruefe("Schwerter A: Brandspuren an der Wand gezeichnet (CG_CreateSaberMarks, R_MarkFragments, CG_AddMarks)", [=] {
        if (!t->wand) { return false; }
        const std::vector<SpurPoly> m = saberSpurenBei(2500.0);
        int brand = 0;
        int glut = 0;
        float weit = 0.0F;
        for (const SpurPoly& p : m) {
            (p.shader == 0 ? brand : glut) += 1;
            for (int j = 0; j < p.zahl; ++j) {
                float d = 0.0F;
                for (int k = 0; k < 3; ++k) { d += (p.xyz[j][k] - t->wandPunkt[k]) * t->wandNormale[k]; }
                weit = std::max(weit, std::fabs(d));
            }
        }
        char z[220];
        std::snprintf(z, sizeof(z),
                      "Knopftest: Schwerter A - Brandspuren bei 2500 ms: %d rivetmark, %d saberDamageGlow, groesster "
                      "Abstand zur Wandebene %.3f, gezeichnet %d | ",
                      brand, glut, static_cast<double>(weit), g_saberSpurenGezeichnet);
        diag::detail(z + saberLeistung());
        return brand > 0 && glut > 0 && weit < 2.0F && g_saberSpurenGezeichnet > 0;
    }));
    // rc601 saber review: Vorspulen = Abspielen. Einmal Bild fuer Bild bis 13 s
    // (wie das Abspielen, Schritt 16 ms), dann ein Sprung auf 20 s und zurueck
    // auf 13 s (die Rechnung setzt dann an einem Schnappschuss vor dem Fenster
    // der Brandspuren auf). Gleich sein muessen die gezeichnete Laenge, die
    // Wandereignisse der letzten Sekunde und jede lebende Brandspur - Bit fuer
    // Bit (eine fast stehende Klinge dreht ihre Spur sonst nach Rundungsrauschen).
    // rc603 fix603: der Speicher behaelt jetzt alle gerechneten Bilder - damit
    // der Sprung zurueck trotzdem am Schnappschuss neu rechnet, haelt er hier
    // nur 1000 Bilder (saberObergrenze; vorn faellt, was vor 20 s - 11 s liegt).
    add(pruefe("Schwerter A: Vorspulen gleich Abspielen bei 13 s (Laenge, Funken, Brandspuren)", [=] {
        const int fi = saberFigurIndex("sabtest");
        constexpr double kZiel = 13000.0;
        const auto stand = [fi](std::vector<SpurPoly>& marken, float& laenge, int& ereignisse) {
            marken = saberSpurenBei(kZiel);
            laenge = saberZeichenLaengeBei(fi, 0, 0, kZiel, -1.0F);
            ereignisse = 0;
            for (const SaberWandEreignis& e : saberWandEreignisseBis(kZiel)) {
                ereignisse += (e.ms > kZiel - 1000.0) ? 1 : 0;
            }
        };
        saberVergessen();
        for (double ms = 0.0; ms < kZiel; ms += 16.0) { saberRechneBis(ms); }
        std::vector<SpurPoly> a;
        float la = 0.0F;
        int ea = 0;
        stand(a, la, ea);
        saberObergrenze(1000);
        saberRechneBis(20000.0);   // Sprung nach vorn ...
        std::vector<SpurPoly> b;
        float lb = 0.0F;
        int eb = 0;
        stand(b, lb, eb);          // ... und zurueck
        saberObergrenze(0);
        bool gleich = a.size() == b.size();
        for (std::size_t i = 0; gleich && i < a.size(); ++i) {
            gleich = a[i].zeit == b[i].zeit && a[i].shader == b[i].shader && a[i].zahl == b[i].zahl &&
                     a[i].farbe[0] == b[i].farbe[0] && a[i].farbe[2] == b[i].farbe[2];
            for (int j = 0; gleich && j < a[i].zahl; ++j) {
                for (int k = 0; k < 3; ++k) { gleich = gleich && std::fabs(a[i].xyz[j][k] - b[i].xyz[j][k]) < 0.001F; }
            }
            if (!gleich) {
                char d[260];
                std::snprintf(d, sizeof(d),
                              "Knopftest: Schwerter A - erste Abweichung bei Marke %zu: Zeit %d/%d, Shader %d/%d, Ecken "
                              "%d/%d, Farbe %d/%d, Ecke0 %.3f %.3f %.3f / %.3f %.3f %.3f",
                              i, a[i].zeit, b[i].zeit, a[i].shader, b[i].shader, a[i].zahl, b[i].zahl, a[i].farbe[0],
                              b[i].farbe[0], static_cast<double>(a[i].xyz[0][0]), static_cast<double>(a[i].xyz[0][1]),
                              static_cast<double>(a[i].xyz[0][2]), static_cast<double>(b[i].xyz[0][0]),
                              static_cast<double>(b[i].xyz[0][1]), static_cast<double>(b[i].xyz[0][2]));
                diag::detail(d);
            }
        }
        char z[240];
        std::snprintf(z, sizeof(z),
                      "Knopftest: Schwerter A - bei 13 s abgespielt: Laenge %.3f, %d Ereignisse, %zu Marken | "
                      "nach 20 s und zurueck: Laenge %.3f, %d Ereignisse, %zu Marken",
                      static_cast<double>(la), ea, a.size(), static_cast<double>(lb), eb, b.size());
        diag::detail(z);
        saberVergessen();
        return la > 0.0F && la == lb && ea == eb && !a.empty() && gleich;
    }));

    // --- B) Zwei Klingen aufeinander, mit SET_SABERCOLLISIONS ------------------
    // rc601 integ: SET_SABERCOLLISIONS gibt es nur in Movie Duels - B laeuft
    // fest unter Movie Duels, B2 prueft dieselbe Szene unter OpenJK.
    auto regelVorher = std::make_shared<int>(kRegelwerkAuto);
    add(tu("Schwerter B: zwei Figuren an derselben Stelle, BS_CINEMATIC und SET_SABERCOLLISIONS", [=] {
        *regelVorher = g_app->settings.regelwerk;
        g_app->settings.regelwerk = kRegelwerkMovieDuels;
        (void)regelwerkPruefen();
        if (!t->bereit) { return; }
        saberSpawnerSetzen(*t, {{"sabklirr1", {t->ort[0], t->ort[1], t->ort[2], t->gier}},
                                {"sabklirr2", {t->ort[0], t->ort[1], t->ort[2], t->gier}}});
        const float kam[3] = {t->ort[0] + 100.0F, t->ort[1], t->ort[2] + 40.0F};
        const float ziel[3] = {t->ort[0], t->ort[1], t->ort[2] + 20.0F};
        saberSzene(*t, saberSkript({"sabklirr1", "sabklirr2"}, true, kam, ziel), "saber_klirren.txt");
    }));
    add(warteBis("Schwerter B: Szene aufgebaut", [] { return g_app->camTrackValid; }, 900));
    add(tu("Schwerter B: Blitzzaehler auf 0", [] {
        g_saberBlitze = 0;
        g_saberBlitzeNetz = 0;
        g_saberBlitze2D = 0;
        g_saberBlitzeNachBalken = 0;
    }));
    add(saberAbspielen("Klirren", 1500.0));
    add(pruefe("Schwerter B: die Klingen klirren (WP_SaberDamageTrace, saber/saber_block)", [=] {
        const int a = saberFigurIndex("sabklirr1");
        const int b = saberFigurIndex("sabklirr2");
        int zahl = 0;
        double erstes = -1.0;
        std::string beispiel;
        for (const SaberKlirren& k : saberKlirrenBis(1500.0)) {
            if (k.figur == k.gegner || (k.figur != a && k.figur != b)) { continue; }
            ++zahl;
            if (erstes < 0.0) {
                erstes = k.ms;
                beispiel = k.effekt + " / " + k.klang;
            }
        }
        diag::detail("Knopftest: Schwerter B - " + std::to_string(zahl) + " Klirren (im Speicher), erstes bei " +
                     std::to_string(static_cast<int>(erstes)) + " ms: " + beispiel + ", Effekt geladen: " +
                     (effectByName("saber/saber_block") != nullptr ? "ja" : "nein"));
        return zahl > 0 && effectByName("saber/saber_block") != nullptr;
    }));
    add(pruefe("Schwerter B: saberblock klingt (Mitschrift, G_Sound)", [] {
        return saberMitschriftHat("saberblock") || saberMitschriftHat("classicblock");
    }));
    add(pruefe("Schwerter B: der Klirrblitz wird gezeichnet (CG_SaberClashFlare, gfx/effects/saberFlare)", [] {
        diag::detail("Knopftest: Schwerter B - Klirrblitze gezeichnet: " + std::to_string(g_saberBlitze));
        return g_saberBlitze > 0;
    }));
    // rc601 saber review 2c: im Spiel ist der Blitz ein 2D-Bild, das CG_Draw2D
    // NACH den Kinobalken zeichnet ("still draw the saber clash flare, but
    // nothing else", cg_draw.cpp:10204 MD, 3964 OJ) - also ueber den Balken,
    // nicht im 3D-Bild darunter.
    add(pruefe("Schwerter B: der Klirrblitz liegt als 2D-Bild ueber den Kinobalken (CG_Draw2D)", [] {
        diag::detail("Knopftest: Schwerter B - Klirrblitze im Effektnetz " + std::to_string(g_saberBlitzeNetz) +
                     ", als 2D-Bild " + std::to_string(g_saberBlitze2D) + ", davon nach den Balken " +
                     std::to_string(g_saberBlitzeNachBalken));
        return g_saberBlitzeNetz == 0 && g_saberBlitze2D > 0 && g_saberBlitzeNachBalken > 0;
    }));
    // rc603 fix603 (Durchsicht Zurueckspulen): ueber der Obergrenze faellt ein
    // Clientbild samt den Klirren seiner Serverbilder (bildEntfernen). Setzt die
    // Rechnung spaeter an einem Schnappschuss auf, dessen Bild schon fort war,
    // kommt das Bild aus dem Schnappschuss zurueck - das Klirren des
    // Serverbildes, das IN diesem Bild lief, nicht: zur vollen Sekunde t*8
    // (t = 125, 250, ...) laeuft immer ein Serverbild. Hier klirrt jedes
    // Serverbild. Vorwaerts bis 14 s mit 1000 Bildern im Speicher (Bild 125 bei
    // 1 s faellt), zurueck auf 12 s (setzt am Schnappschuss 125 auf) und auf
    // 1,1 s: dieselben Klirren und dieselben Effekte bei 1,1 s wie ohne
    // Obergrenze.
    add(pruefe("Schwerter B: Klirren am Schnappschuss nach dem Zurueckspulen mit Obergrenze gleich", [=] {
        const auto lauf = [](std::size_t grenze, std::vector<double>& klirren, std::size_t& effekte) {
            saberVergessen();
            saberObergrenze(grenze);
            for (double ms = 0.0; ms <= 14000.0; ms += 16.0) { saberRechneBis(ms); }
            saberRechneBis(12000.0);
            effekte = saberEffekteBei(1100.0).size();
            klirren.clear();
            for (const SaberKlirren& k : saberKlirrenBis(3000.0)) { klirren.push_back(k.ms); }
            saberObergrenze(0);
            saberVergessen();
        };
        std::vector<double> ohne;
        std::vector<double> mit;
        std::size_t effOhne = 0;
        std::size_t effMit = 0;
        lauf(0, ohne, effOhne);
        lauf(1000, mit, effMit);
        const auto bei = [](const std::vector<double>& v, double ms) {
            return static_cast<int>(std::count(v.begin(), v.end(), ms));
        };
        std::string fehlt;
        for (double ms = 0.0; ms <= 3000.0; ms += 50.0) {
            if (bei(ohne, ms) != bei(mit, ms)) {
                fehlt += " " + std::to_string(static_cast<int>(ms)) + " ms (" + std::to_string(bei(ohne, ms)) + "/" +
                         std::to_string(bei(mit, ms)) + ")";
            }
        }
        diag::detail("Knopftest: Schwerter B - Klirren bis 3 s ohne Obergrenze " + std::to_string(ohne.size()) +
                     " (bei 1000 ms " + std::to_string(bei(ohne, 1000.0)) + "), mit Obergrenze 1000 " +
                     std::to_string(mit.size()) + "; Effekte bei 1,1 s " + std::to_string(effOhne) + " / " +
                     std::to_string(effMit) + (fehlt.empty() ? std::string() : "; verschieden bei" + fehlt));
        return bei(ohne, 1000.0) > 0 && ohne == mit && effOhne == effMit;
    }));
    // --- B2) rc601 integ: dieselbe Szene unter OpenJK - G_InCinematicSaberAnim
    //     kennt dort nur BOTH_CIN_16/17 (wp_saber.cpp:2434-2443 OJ), den Befehl
    //     SET_SABERCOLLISIONS gar nicht: kein Klirren.
    add(tu("Schwerter B2: dieselbe Szene unter OpenJK", [=] {
        g_app->settings.regelwerk = kRegelwerkOpenJK;
        (void)regelwerkPruefen();
        g_app->camTrackValid = false;
    }));
    add(warteBis("Schwerter B2: Szene aufgebaut", [] { return g_app->camTrackValid; }, 900));
    add(saberAbspielen("Klirren unter OpenJK", 1500.0));
    add(pruefe("Schwerter B2: unter OpenJK klirren dieselben Klingen nicht (kein SET_SABERCOLLISIONS)", [=] {
        int zahl = 0;
        for (const SaberKlirren& k : saberKlirrenBis(1500.0)) { zahl += k.figur != k.gegner ? 1 : 0; }
        diag::detail("Knopftest: Schwerter B2 - " + std::to_string(zahl) + " Klirren unter OpenJK");
        const bool ok = !t->bereit || zahl == 0;
        g_app->settings.regelwerk = kRegelwerkMovieDuels;
        (void)regelwerkPruefen();
        return ok;
    }));
    // --- C) Ohne SET_SABERCOLLISIONS kein Klirren --------------------------
    add(tu("Schwerter C: dieselben ohne SET_SABERCOLLISIONS", [=] {
        if (!t->bereit) { return; }
        const float kam[3] = {t->ort[0] + 100.0F, t->ort[1], t->ort[2] + 40.0F};
        const float ziel[3] = {t->ort[0], t->ort[1], t->ort[2] + 20.0F};
        saberSzene(*t, saberSkript({"sabklirr1", "sabklirr2"}, false, kam, ziel), "saber_still.txt");
    }));
    add(warteBis("Schwerter C: Szene aufgebaut", [] { return g_app->camTrackValid; }, 900));
    add(saberAbspielen("ohne Kollisionen", 1500.0));
    add(pruefe("Schwerter C: ohne SET_SABERCOLLISIONS klirrt nichts (G_InCinematicSaberAnim)", [] {
        return saberKlirrenBis(1500.0).empty() && !saberMitschriftHat("saberblock");
    }));
    // --- D) rc601 saber review: nur Figuren im Snapshot (PVS der Kamera) ------
    // Der Client zeichnet - und spurt, kuerzt, spruehrt, klingt, brennt - nur
    // Figuren, die der Server in den Snapshot legt: sichtbar vom Kameraort aus
    // (SV_BuildClientSnapshot: "if in camera mode use camera position instead",
    // sv_snapshot.cpp:550 OJ). Steht die Kamera in einem Raum, der die Figur
    // nicht sieht, gibt es an der Wand weder Funken noch Brandspur noch Klang.
    add(tu("Schwerter D: dieselbe Figur an der Wand, Kamera ausserhalb ihres PVS", [=] {
        if (!t->wand) { return; }
        const float mn[3] = {t->ort2[0] - 17.0F, t->ort2[1] - 17.0F, t->ort2[2] - 25.0F};
        const float mx[3] = {t->ort2[0] + 17.0F, t->ort2[1] + 17.0F, t->ort2[2] + 41.0F};
        float kam[3] = {0.0F, 0.0F, 0.0F};
        bool gefunden = false;
        for (const BspGeometry::Leaf& l : g_app->geo.leafs) {
            if (l.cluster < 0) { continue; }
            const float c[3] = {0.5F * (l.mins[0] + l.maxs[0]), 0.5F * (l.mins[1] + l.maxs[1]),
                                0.5F * (l.mins[2] + l.maxs[2])};
            if (!kastenImSnapshot(g_app->geo, c, mn, mx)) {
                for (int k = 0; k < 3; ++k) { kam[k] = c[k]; }
                gefunden = true;
                break;
            }
        }
        diag::detail("Knopftest: Schwerter D - Kamera ausserhalb des PVS " + std::string(gefunden ? "bei " + saberVek(kam) : "nicht gefunden"));
        if (!gefunden) { return; }
        klangMitschrift().clear();
        saberSpawnerSetzen(*t, {{"sabtest", {t->ort2[0], t->ort2[1], t->ort2[2], t->gier2}}});
        const float ziel[3] = {t->ort2[0], t->ort2[1], t->ort2[2] + 20.0F};
        saberSzene(*t, saberSkript({"sabtest"}, false, kam, ziel), "saber_pvs.txt");
    }));
    add(warteBis("Schwerter D: Szene aufgebaut", [] { return g_app->camTrackValid; }, 900));
    add(saberAbspielen("Klinge an der Wand ausserhalb des PVS", 1500.0));
    add(pruefe("Schwerter D: ausserhalb des Snapshots keine Funken, Brandspuren oder Wandklaenge (CG_Player nur fuer Snapshot-Entities)", [] {
        int ereignisse = 0;
        for (const SaberWandEreignis& e : saberWandEreignisseBis(1500.0)) {
            ereignisse += (e.art == SaberWandEreignis::Art::Funken || e.art == SaberWandEreignis::Art::Spur ||
                           e.art == SaberWandEreignis::Art::Klang)
                              ? 1
                              : 0;
        }
        const std::size_t marken = saberSpurenBei(1500.0).size();
        const bool klang = saberMitschriftHat("saberhitwall") || saberMitschriftHat("saberstrikewall");
        diag::detail("Knopftest: Schwerter D - Wandereignisse " + std::to_string(ereignisse) + ", Brandspuren " +
                     std::to_string(marken) + ", Wandklang " + (klang ? "ja" : "nein"));
        return g_app->camTrackValid && ereignisse == 0 && marken == 0U && !klang;
    }));
    add(tu("Schwerter: aufraeumen", [=] {
        g_app->settings.regelwerk = *regelVorher;   // rc601 integ
        g_app->playing = false;
        g_app->audio.stopAll();
        g_app->laufendeSchleifen.clear();
        tonZuruecksetzen();
        klangMitschriftAn(false);
        klangMitschrift().clear();
        if (t->getauscht) {
            g_app->map.entities = t->entities;
            t->getauscht = false;
        }
        g_app->cam = t->cam;
        g_app->doc.markSaved();
        while (static_cast<int>(g_app->tabs.size()) > std::max(1, t->reiter)) {
            reiterSchliessen(static_cast<int>(g_app->tabs.size()) - 1);
        }
        g_app->camTrackValid = false;
        diag::detail("Knopftest: Schwerter - " + saberLeistung());
    }));
    add(pause(0.3));
    return s;
}
// Paket bewegung (rc601): Schwerkraft im Programm (steht vor szenenSchritte).
std::vector<Schritt> schwerkraftSchritte();

std::vector<Schritt> missionsbauSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<mb::Zustand>();

    // ---------------- Vorbereitung ----------------
    add(tu("Missionsbau: Laufordner als Missionswurzel, Grundkarte suchen", [=] {
        const std::string wurzel = mb::verbinde(mb::laufOrdner(), "missionen");
        _putenv_s("BHED_MISSIONEN", wurzel.c_str());
        z->spiel = mb::verbinde(mb::laufOrdner(), "spiel");
        std::error_code ec;
        std::filesystem::create_directories(::bhed::utf8Pfad(z->spiel), ec);
        for (const GamePath& gp : g_app->gamePaths) {
            for (const Pk3& arc : gp.archives) {
                if (arc.find("maps/duel_mandalore.bsp") != nullptr) { z->karte = "maps/duel_mandalore.bsp"; }
                for (const Pk3Entry& e : arc.entries) {
                    if (z->karte.empty() && e.name.rfind("maps/", 0) == 0 && e.name.size() > 9 &&
                        e.name.compare(e.name.size() - 4, 4, ".bsp") == 0 && e.name.find("/mp/") == std::string::npos) {
                        z->karte = e.name;
                    }
                }
            }
        }
        diag::detail("Knopftest: Missionsbau - Wurzel " + wurzel + ", Grundkarte " + z->karte);
        g_app->leftMode = 1;
    }));
    add(pruefe("Missionsbau: Grundkarte gefunden", [=] { return !z->karte.empty(); }));
    add(pause(0.5));
    // Ohne Karte: die Startflaeche statt "No map loaded" (rc586).
    add(foto("mb_start"));
    add(pruefe("Startflaeche: drei Wege hinein sichtbar", [] {
        return g_app->geo.empty() && finde(tr(Str::StartCardNew), "", 0, false) != nullptr &&
               finde(tr(Str::StartCardLoad), "", 0, false) != nullptr && finde(tr(Str::StartCardMap), "", 0, false) != nullptr;
    }));
    // rc600: "Neu hier?" - Erste Schritte und Tutorials oeffnen das Handbuch
    // an der richtigen Seite (im Selbsttest nur eine Statuszeile).
    const auto handbuchVerlangt = [](const char* seite) {
        for (const std::string& z : g_app->statusLines) {
            if (z.find(std::string("Handbuch angefragt: ") + seite) != std::string::npos) { return true; }
        }
        return false;
    };
    add(tu("Startflaeche: Status leeren", [] { g_app->statusLines.clear(); }));
    add(klick(tr(Str::StartGuideFirst), "##startflaeche"));
    add(pause(0.2));
    add(pruefe("Startflaeche: Erste Schritte oeffnet das Handbuch", [=] { return handbuchVerlangt("erste-schritte.html"); }));
    add(klick(tr(Str::StartGuideTut), "##startflaeche"));
    add(pause(0.2));
    add(pruefe("Startflaeche: Tutorials oeffnet die Tutorial-Seite", [=] { return handbuchVerlangt("tutorials.html"); }));
    add(pruefe("Modusleiste: der Karten-Tab heisst Mission", [] {
        return std::string(tr(Str::ViewMap)) != "Map" && finde(tr(Str::ViewMap), "", 0, false) != nullptr;
    }));

    // ---------------- New mission ueber das Fenster ----------------
    add(klick(tr(Str::MissionNew)));
    add(pause(0.4));
    add(pruefe("New mission: Fenster offen", [] { return g_app->neueMissionOffen; }));
    add(klick("##kartensuche", "##neuemission"));
    add(tu("Kartensuche: Namen der Grundkarte tippen", [=] {
        schritteEinfuegen({tippe(fileName(z->karte).substr(0, 10))});
    }));
    add(pause(0.3));
    add(tu("Karte in der Liste waehlen", [=] {
        schritteEinfuegen({klick(z->karte, "##neuemission")});
    }));
    add(pause(0.3));
    add(klick(tr(Str::MissionCreate), "##neuemission"));
    add(warteBis("Mission angelegt", [] { return g_app->missionAktiv && !g_app->neueMissionOffen; }, 600));
    add(warteBis("Szene der neuen Mission", [] { return g_app->camTrackValid; }, 1500));
    add(pruefe("New mission: Karte, Ordner, Intro-Reiter, Scriptrunner", [=] {
        const App::MissionsProjekt& m = g_app->mission;
        const std::string intro = mb::verbinde(m.projektOrdner, "scripts\\" + m.ordner + "\\intro.txt");
        const std::string ent = mb::verbinde(m.projektOrdner, "maps\\" + m.name + ".ent");
        bool runner = false;
        bool start = false;
        for (const MapEntity& e : g_app->map.entities) {
            if (e.classname == "target_scriptrunner" && mb::wert(&e, "usescript") == m.ordner + "/intro") { runner = true; }
            if (e.classname == "info_player_start" && !mb::wert(&e, "target").empty()) { start = true; }
        }
        std::error_code ec;
        diag::detail("Knopftest: Missionsbau - " + m.name + " / " + m.ordner + ", Karte " + g_app->map.path +
                     ", Reiter " + mb::reiterName(static_cast<std::size_t>(g_app->activeTab)) + ", Ordner " + m.projektOrdner + ", Ziel " + m.zielPk3);
        return g_app->map.path == "maps/" + m.name + ".bsp" && !g_app->geo.empty() &&
               std::filesystem::exists(::bhed::utf8Pfad(intro), ec) &&
               std::filesystem::exists(::bhed::utf8Pfad(ent), ec) && mb::reiterName(static_cast<std::size_t>(g_app->activeTab)) == "intro.txt" &&
               runner && start && g_app->entitiesUngespeichert.empty();
    }));
    add(pruefe("New mission: das Intro hat eine Kamerafahrt", [] {
        return g_app->camTrackValid && !g_app->camTrack.segments.empty();
    }));
    add(tu("New mission: derselbe Name noch einmal, ein ungueltiger", [=] {
        std::string f1;
        std::string f2;
        const std::size_t reiter = g_app->tabs.size();
        const bool a = neueMissionAnlegen(z->karte, g_app->mission.name, g_app->mission.ordner, &f1);
        const bool b = neueMissionAnlegen(z->karte, "md bad", "md_bad", &f2);
        diag::detail("Knopftest: Missionsbau - doppelt: " + std::string(a ? "angelegt" : "abgewiesen") + " (" + f1 +
                     "), ungueltig: " + (b ? "angelegt" : "abgewiesen") + " (" + f2 + "), Reiter " +
                     std::to_string(reiter) + " -> " + std::to_string(g_app->tabs.size()));
        z->vorher = (!a && !b && !f1.empty() && !f2.empty() && reiter == g_app->tabs.size()) ? 1 : 0;
    }));
    add(pruefe("New mission: doppelter und ungueltiger Name abgewiesen, kein neuer Reiter", [=] { return z->vorher == 1; }));

    // ---------------- + Entity ----------------
    add(tu("+ Entity: Anzahl merken", [=] { z->vorher = mb::zaehle("NPC_spawner"); }));
    add(klick(tr(Str::EntNew)));
    add(pause(0.3));
    add(klick(tr(Str::EntKindNpc)));
    add(pause(0.5));
    add(foto("mb_inspektor"));
    add(pruefe("+ Entity: Figur gesetzt, gewaehlt, mit Namen, auf dem Boden", [=] {
        const MapEntity* e = mb::gewaehlt();
        const float f = e != nullptr ? mb::fuesse(*e) : 9999.0F;
        diag::detail("Knopftest: Missionsbau - neue Figur " + mb::wert(e, "NPC_targetname") + " bei " +
                     mb::wert(e, "origin") + ", Fuesse " + std::to_string(f));
        z->npc = g_app->pickedEntity;
        return e != nullptr && e->classname == "NPC_spawner" && mb::zaehle("NPC_spawner") == z->vorher + 1 &&
               !mb::wert(e, "NPC_targetname").empty() && f > -0.5F && f < 3.0F &&
               g_app->entitiesUngespeichert.count(g_app->map.path) == 1U;
    }));
    // Spawnflags als Haekchen
    add(klick("CINEMATIC", "entinspektor"));
    add(pause(0.2));
    add(pruefe("Haekchen CINEMATIC: spawnflags 32", [] { return mb::wert(mb::gewaehlt(), "spawnflags") == "32"; }));
    add(klick("DROPTOFLOOR", "entinspektor"));
    add(pause(0.2));
    add(pruefe("Haekchen DROPTOFLOOR dazu: 48", [] { return mb::wert(mb::gewaehlt(), "spawnflags") == "48"; }));
    add(klick("CINEMATIC", "entinspektor"));
    add(pause(0.2));
    add(pruefe("CINEMATIC wieder aus: 16", [] { return mb::wert(mb::gewaehlt(), "spawnflags") == "16"; }));
    // NPC-Auswahl
    add(klick(tr(Str::EntPickNpc), "entinspektor"));
    add(pause(0.4));
    add(tu("NPC-Auswahl: einen Typ aus der Liste waehlen", [=] {
        // Ein Typ, den es sicher gibt und der in der Liste sichtbar ist.
        std::string typ;
        for (const auto& [t2, d] : g_app->npcMap) {
            (void)d;
            if (finde(t2, "", 0, false) != nullptr) { typ = t2; break; }
        }
        diag::detail("Knopftest: Missionsbau - NPC-Auswahl klickt \"" + typ + "\"");
        if (!typ.empty()) { schritteEinfuegen({klick(typ)}); }
        else { meldeFehler("NPC-Auswahl: kein Eintrag sichtbar"); }
    }));
    add(pause(0.4));
    add(pruefe("NPC_type gesetzt", [] {
        diag::detail("Knopftest: Missionsbau - NPC_type " + mb::wert(mb::gewaehlt(), "NPC_type"));
        return !mb::wert(mb::gewaehlt(), "NPC_type").empty();
    }));
    // Rueckgaengig ueber die Knoepfe
    add(tu("Inspektor nach unten rollen", [] { mb::rolleInspektor(1.0F); }));
    add(pause(0.2));
    add(klick(tr(Str::EditUndo), "entinspektor"));
    add(pause(0.2));
    add(pruefe("Undo (Knopf): NPC_type wieder leer", [] { return mb::wert(mb::gewaehlt(), "NPC_type").empty(); }));
    add(klick(tr(Str::EditRedo), "entinspektor"));
    add(pause(0.2));
    add(pruefe("Redo (Knopf): NPC_type wieder da", [] { return !mb::wert(mb::gewaehlt(), "NPC_type").empty(); }));
    // rc601 bewegung: die Schwerkraft gegen den Boden dieser Karte.
    for (Schritt& x : schwerkraftSchritte()) { add(std::move(x)); }
    // Auf den Boden
    add(tu("Figur 120 hoch heben", [=] {
        const MapEntity* e = mb::gewaehlt();
        float o[3];
        if (e != nullptr && entityOrt(*e, o)) {
            o[2] += 120.0F;
            char t[96];
            std::snprintf(t, sizeof(t), "%d %d %d", static_cast<int>(o[0]), static_cast<int>(o[1]), static_cast<int>(o[2]));
            entityKeySetzen(g_app->pickedEntity, "origin", t);
        }
    }));
    add(pause(0.2));
    add(tu("Inspektor nach oben rollen", [] { mb::rolleInspektor(0.0F); }));
    add(pause(0.2));
    add(klick(tr(Str::EntDropFloor), "entinspektor"));
    add(pause(0.2));
    add(pruefe("Auf den Boden: Fuesse 1 ueber dem Boden", [] {
        const MapEntity* e = mb::gewaehlt();
        const float f = e != nullptr ? mb::fuesse(*e) : 9999.0F;
        diag::detail("Knopftest: Missionsbau - nach Auf den Boden: Fuesse " + std::to_string(f));
        return f > 0.0F && f < 2.0F;
    }));
    // Ziehen in der Ansicht
    add(tu("Zur Figur fliegen (freie Kamera)", [] {
        g_app->throughCamera = false;
        g_app->playing = false;
        zurEntityFliegen(static_cast<std::size_t>(g_app->pickedEntity));
        g_app->mapDirty = true;
    }));
    add(pause(0.6));
    add(tu("Ort vor dem Ziehen merken", [=] {
        const MapEntity* e = mb::gewaehlt();
        if (e != nullptr) { (void)entityOrt(*e, z->ortVorher); }
        z->undoTiefe = g_app->entUndo.size();
    }));
    add(ziehenAn("die gewaehlte Figur in der Ansicht", [] {
        // Ihre Marke (Rasterpunkte -> Bildschirm).
        const float skal = g_app->mapImage.width > 0 && g_app->kartenAnsicht[2] > g_app->kartenAnsicht[0]
                               ? static_cast<float>(g_app->mapImage.width) / (g_app->kartenAnsicht[2] - g_app->kartenAnsicht[0])
                               : 1.0F;
        for (const App::PickedMarker& mk : g_app->markers) {
            if (static_cast<int>(mk.entity) == g_app->pickedEntity) {
                return ImVec2{g_app->kartenAnsicht[0] + mk.sx / skal, g_app->kartenAnsicht[1] + mk.sy / skal};
            }
        }
        return mb::ansichtMitte();
    }, 90.0F, 30.0F, 0));
    add(pause(0.3));
    add(pruefe("Ziehen: verschoben, auf dem Boden, ein Rueckgaengig-Schritt", [=] {
        const MapEntity* e = mb::gewaehlt();
        float o[3] = {0, 0, 0};
        if (e != nullptr) { (void)entityOrt(*e, o); }
        const float weg = std::hypot(o[0] - z->ortVorher[0], o[1] - z->ortVorher[1]);
        const float f = e != nullptr ? mb::fuesse(*e) : 9999.0F;
        diag::detail("Knopftest: Missionsbau - gezogen um " + std::to_string(weg) + ", Fuesse " + std::to_string(f) +
                     ", Verlauf " + std::to_string(z->undoTiefe) + " -> " + std::to_string(g_app->entUndo.size()));
        return e != nullptr && weg > 8.0F && f > -0.5F && f < 3.0F && g_app->entUndo.size() == z->undoTiefe + 1;
    }));
    // Strg+Z / Strg+Y / Entf ueber der Karte - das Skript bleibt unberuehrt
    add(tu("Maus ueber die Ansicht, Stand merken", [=] {
        setzeMaus(mb::ansichtMitte());
        z->skriptUndo = g_app->doc.undoDepth();
        z->undoTiefe = g_app->entUndo.size();
    }));
    add(pause(0.2));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z ueber der Karte"));
    add(pause(0.2));
    add(pruefe("Strg+Z ueber der Karte: Entity zurueck, Skript unberuehrt", [=] {
        const MapEntity* e = mb::gewaehlt();
        float o[3] = {0, 0, 0};
        if (e != nullptr) { (void)entityOrt(*e, o); }
        return g_app->entUndo.size() + 1 == z->undoTiefe && g_app->doc.undoDepth() == z->skriptUndo &&
               std::hypot(o[0] - z->ortVorher[0], o[1] - z->ortVorher[1]) < 2.0F;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Y, "Strg+Y ueber der Karte"));
    add(pause(0.2));
    add(pruefe("Strg+Y ueber der Karte: wieder verschoben", [=] { return g_app->entUndo.size() == z->undoTiefe; }));
    // --- Gizmo und Hervorheben (rc586) ---------------------------------------
    auto winkel0 = std::make_shared<float>(0.0F);
    add(tu("Gizmo: Stand merken", [=] {
        const MapEntity* e = mb::gewaehlt();
        if (e != nullptr) { (void)entityOrt(*e, z->ortVorher); }
        *winkel0 = e != nullptr && e->find("angle") != nullptr ? std::strtof(e->find("angle")->c_str(), nullptr) : 0.0F;
        z->undoTiefe = g_app->entUndo.size();
    }));
    add(pause(0.3));
    add(foto("mb_gizmo"));
    add(ziehenAn("Gizmo-Achse Z nach oben", [] {
        ImVec2 q{0.0F, 0.0F};
        (void)entityGizmoPunkt(3, q);
        return q;
    }, 0.0F, -40.0F, 0));
    add(pause(0.3));
    add(pruefe("Gizmo Z: nur die Hoehe aendert sich, ein Rueckgaengig-Schritt", [=] {
        const MapEntity* e = mb::gewaehlt();
        float o[3] = {0, 0, 0};
        if (e != nullptr) { (void)entityOrt(*e, o); }
        diag::detail("Knopftest: Missionsbau - Gizmo Z: " + std::to_string(z->ortVorher[2]) + " -> " + std::to_string(o[2]) +
                     ", x/y " + std::to_string(o[0] - z->ortVorher[0]) + "/" + std::to_string(o[1] - z->ortVorher[1]) +
                     ", Verlauf " + std::to_string(z->undoTiefe) + " -> " + std::to_string(g_app->entUndo.size()));
        return e != nullptr && o[2] > z->ortVorher[2] + 2.0F && std::fabs(o[0] - z->ortVorher[0]) < 0.5F &&
               std::fabs(o[1] - z->ortVorher[1]) < 0.5F && g_app->entUndo.size() == z->undoTiefe + 1;
    }));
    add(tu("Gizmo: Ort fuer den Ring merken", [=] {
        const MapEntity* e = mb::gewaehlt();
        if (e != nullptr) { (void)entityOrt(*e, z->ortVorher); }
    }));
    // Seit rc599 dreht der Ring nur im Drehmodus (Verschieben: nur Pfeile).
    add(tu("Gizmo: Drehmodus", [] { g_app->gizmoMode = App::GizmoMode::Rotate; g_app->mapDirty = true; }));
    add(pause(0.2));
    add(ziehenAn("Gizmo-Ring drehen", [] {
        ImVec2 q{0.0F, 0.0F};
        (void)entityGizmoPunkt(4, q);
        return q;
    }, 45.0F, -10.0F, 0));
    add(tu("Gizmo: wieder Verschieben", [] { g_app->gizmoMode = App::GizmoMode::Move; g_app->mapDirty = true; }));
    add(pause(0.3));
    add(pruefe("Gizmo-Ring: die Blickrichtung dreht sich, der Ort bleibt", [=] {
        const MapEntity* e = mb::gewaehlt();
        float o[3] = {0, 0, 0};
        if (e != nullptr) { (void)entityOrt(*e, o); }
        const float w = e != nullptr && e->find("angle") != nullptr ? std::strtof(e->find("angle")->c_str(), nullptr) : 0.0F;
        diag::detail("Knopftest: Missionsbau - Ring: Winkel " + std::to_string(*winkel0) + " -> " + std::to_string(w));
        return e != nullptr && std::fabs(w - *winkel0) > 2.0F && std::fabs(o[0] - z->ortVorher[0]) < 0.5F &&
               std::fabs(o[2] - z->ortVorher[2]) < 0.5F;
    }));
    // Ueberfahren einer anderen Entity hebt sie hervor (Kasten + Name).
    auto andere = std::make_shared<int>(-1);
    auto gewaehltVorher = std::make_shared<int>(-1);
    add(tu("Auswahl aufheben, Rundblick (damit andere Entities im Bild sind)", [=] {
        *gewaehltVorher = g_app->pickedEntity;
        g_app->pickedEntity = -1;
        g_app->selectedActor = -1;
        g_app->cam.angles[1] += 180.0F;
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(tu("Maus ueber eine andere Entity", [=] {
        *andere = -1;
        for (const App::PickedMarker& mk : g_app->markers) {
            if (static_cast<int>(mk.entity) != g_app->pickedEntity) {
                *andere = static_cast<int>(mk.entity);
                setzeMaus(bildZuSchirm(mk.sx, mk.sy));
                break;
            }
        }
    }));
    add(pause(0.3));
    add(foto("mb_hover"));
    add(pruefe("Ueberfahren: die Entity unter der Maus ist hervorgehoben", [=] {
        diag::detail("Knopftest: Missionsbau - Ueberfahren: erwartet " + std::to_string(*andere) + ", hervorgehoben " +
                     std::to_string(g_app->entHover));
        return *andere >= 0 && g_app->entHover == *andere;
    }));
    add(tu("Auswahl zurueck", [=] {
        g_app->pickedEntity = *gewaehltVorher;
        g_app->cam.angles[1] -= 180.0F;
        g_app->mapDirty = true;
    }));
    add(pause(0.3));
    // Duplizieren und Entf
    add(tu("Duplizieren", [=] {
        z->vorher = static_cast<int>(g_app->map.entities.size());
        entityDuplizieren(g_app->pickedEntity);
    }));
    add(pause(0.2));
    add(pruefe("Duplikat: eigener Name", [=] {
        const MapEntity* e = mb::gewaehlt();
        const MapEntity& o = g_app->map.entities[static_cast<std::size_t>(z->npc)];
        return static_cast<int>(g_app->map.entities.size()) == z->vorher + 1 && e != nullptr &&
               mb::wert(e, "NPC_targetname") != mb::wert(&o, "NPC_targetname");
    }));
    add(tu("Maus ueber die Ansicht", [] { setzeMaus(mb::ansichtMitte()); }));
    add(pause(0.2));
    add(taste(ImGuiKey_Delete, "Entf ueber der Karte"));
    add(pause(0.2));
    add(pruefe("Entf ueber der Karte: Duplikat geloescht, Skript unberuehrt", [=] {
        return static_cast<int>(g_app->map.entities.size()) == z->vorher && g_app->pickedEntity == -1 &&
               g_app->doc.undoDepth() == z->skriptUndo;
    }));
    // Rechtsklick "New entity here"
    add(tu("Laufziele zaehlen", [=] { z->vorher = mb::zaehle("waypoint_navgoal"); }));
    add(klickAn("rechts in die Ansicht", [] { return mb::ansichtMitte(); }, 1));
    add(pause(0.3));
    add(klick(tr(Str::EntNewHere)));
    add(pause(0.3));
    add(klick(tr(Str::EntKindNav)));
    add(pause(0.3));
    add(pruefe("Rechtsklick > New entity here > Laufziel", [=] {
        const MapEntity* e = mb::gewaehlt();
        return mb::zaehle("waypoint_navgoal") == z->vorher + 1 && e != nullptr && e->classname == "waypoint_navgoal" &&
               !e->targetname.empty();
    }));
    // Alle Schluessel roh, Schluessel dazu und weg
    add(tu("Inspektor nach unten rollen", [] { mb::rolleInspektor(1.0F); }));
    add(pause(0.2));
    add(klick(tr(Str::EntRaw), "entinspektor"));
    add(pause(0.2));
    add(tu("Schluessel radius setzen und wieder entfernen", [=] {
        entityKeySetzen(g_app->pickedEntity, "radius", "24");
        z->vorher = mb::wert(mb::gewaehlt(), "radius") == "24" ? 1 : 0;
        entityKeySetzen(g_app->pickedEntity, "radius", "");
    }));
    add(pause(0.2));
    add(pruefe("Rohansicht: Schluessel gesetzt und entfernt", [=] {
        return z->vorher == 1 && mb::gewaehlt() != nullptr && mb::gewaehlt()->find("radius") == nullptr;
    }));
    add(tu("Inspektor nach unten rollen", [] { mb::rolleInspektor(1.0F); }));
    add(pause(0.2));
    add(klick(tr(Str::EntRaw), "entinspektor"));
    // Ein Scriptrunner mit eigenem Skript: Open legt es im Missionsordner an
    add(tu("Scriptrunner unter das Fadenkreuz", [] { neueEntityHier("target_scriptrunner", false); }));
    add(pause(0.3));
    add(tu("usescript des Runners oeffnen (legt das Skript an)", [=] {
        const MapEntity* e = mb::gewaehlt();
        z->vorher = static_cast<int>(g_app->tabs.size());
        diag::detail("Knopftest: Missionsbau - Runner usescript " + mb::wert(e, "usescript"));
        schritteEinfuegen({klick(tr(Str::EntOpenScript), "entinspektor")});
    }));
    add(pause(0.5));
    add(pruefe("Open: neuer Reiter mit dem Skript, Datei im Missionsordner", [=] {
        std::error_code ec;
        const std::string datei = mb::verbinde(g_app->mission.projektOrdner,
                                               "scripts\\" + g_app->mission.ordner + "\\runner1.txt");
        diag::detail("Knopftest: Missionsbau - Reiter " + std::to_string(z->vorher) + " -> " +
                     std::to_string(g_app->tabs.size()) + ", aktiv " + mb::reiterName(static_cast<std::size_t>(g_app->activeTab)));
        return static_cast<int>(g_app->tabs.size()) == z->vorher + 1 && mb::reiterName(static_cast<std::size_t>(g_app->activeTab)) == "runner1.txt" &&
               std::filesystem::exists(::bhed::utf8Pfad(datei), ec);
    }));
    add(tu("zurueck in den Intro-Reiter", [] { mb::zumReiter("intro.txt"); }));
    add(pause(0.3));

    // ---------------- Reiterwechsel mit anderer Karte ----------------
    add(tu("Entities merken, Reiter mit anderer Karte", [=] {
        z->gespeichert = g_app->map.entities;
        z->vorher = g_app->activeTab;
        neuerReiter();
        (void)loadMapFromArchive(z->karte == "maps/duel_mandalore.bsp" ? std::string("maps/academy1.bsp") : std::string("maps/duel_mandalore.bsp"));
    }));
    add(pause(0.5));
    // rc592: die Leiste zeigte weiter den Namen der Mission, obwohl vorn eine
    // andere Karte lag.
    add(pruefe("Leiste: vorn eine andere Karte - ihr Name, nicht der der Mission", [=] {
        const std::string n = leistenTitel();
        diag::detail("Knopftest: Missionsbau - Leiste zeigt \"" + n + "\" (Karte " + g_app->map.path + ", Mission " +
                     g_app->mission.name + ")");
        return n != g_app->mission.name && (n == "academy1" || n == "duel_mandalore");
    }));
    add(tu("zurueck", [=] { activateTab(z->vorher); }));
    add(pause(0.5));
    add(pruefe("Leiste: wieder die Missionskarte - der Name der Mission", [=] {
        return leistenTitel() == g_app->mission.name;
    }));
    add(pruefe("Reiterwechsel: bearbeitete Entities der neuen Mission sind noch da", [=] {
        bool gleich = g_app->map.entities.size() == z->gespeichert.size();
        for (std::size_t i = 0; gleich && i < z->gespeichert.size(); ++i) { gleich = g_app->map.entities[i].keys == z->gespeichert[i].keys; }
        diag::detail("Knopftest: Missionsbau - zurueck: " + std::to_string(g_app->map.entities.size()) + " Entities, Karte " +
                     g_app->map.path + ", Geometrie " + (g_app->geo.empty() ? "leer" : "da"));
        return gleich && !g_app->geo.empty();
    }));

    // ---------------- Drei Ansichten: Classic, Graph, Spuren (rc587) ----------------
    //
    // "das man wenn man will in alle 3 arbeiten kann durch umschalten": in jeder
    // Ansicht etwas aendern, dazwischen umschalten, Rueckgaengig ueber die
    // Ansichten hinweg - alles auf demselben Dokument.
    {
        auto schl = std::make_shared<std::map<std::string, std::string>>();
        const auto knotenPunkt = [](const std::string& sch, bool ausgang) {
            const App::GraphZustand& gz = g_app->graph();
            const float s = ImGui::GetFontSize() / 13.0F;
            for (const GraphKnoten& k : gz.g.knoten) {
                if (k.schluessel != sch) { continue; }
                const float B = 190.0F * s * gz.zoom;
                const float H = 62.0F * s * gz.zoom;
                const ImVec2 a{gz.ursprung.x + gz.panX + k.x * gz.zoom, gz.ursprung.y + gz.panY + k.y * gz.zoom};
                return ausgang ? ImVec2{a.x + B, a.y + H * 0.5F} : ImVec2{a.x + B * 0.5F, a.y + H * 0.5F};
            }
            return ImVec2{-1.0F, -1.0F};
        };
        const auto kanteDa = [](const std::string& von, const std::string& nach, const std::string& label) {
            const auto& g = g_app->graph().g;
            for (const GraphKante& k : g.kanten) {
                if (g.knoten[static_cast<std::size_t>(k.von)].schluessel == von &&
                    g.knoten[static_cast<std::size_t>(k.nach)].schluessel == nach && k.label == label) {
                    return true;
                }
            }
            return false;
        };
        add(tu("Ansichten: Schluessel der Knoten bestimmen", [=] {
            mb::zumReiter("intro.txt");
            for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
                const MapEntity& e = g_app->map.entities[i];
                if (e.classname == "info_player_start") { (*schl)["start"] = "e:" + std::to_string(i); }
                if (e.classname == "target_scriptrunner" && e.targetname != "runner1") { (*schl)["introStarter"] = "e:" + std::to_string(i); }
                if (e.classname == "target_scriptrunner" && e.targetname == "runner1") { (*schl)["runner1"] = "e:" + std::to_string(i); }
                if (e.classname == "NPC_spawner" && mb::wert(&e, "NPC_targetname") == "npc1") { (*schl)["npc1"] = "e:" + std::to_string(i); }
            }
            std::string o = g_app->mission.ordner;
            for (char& c : o) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
            (*schl)["intro"] = "s:" + o + "/intro";
            (*schl)["runner1Skript"] = "s:" + o + "/runner1";
            std::string t;
            for (const auto& [k, v] : *schl) { t += k + "=" + v + " "; }
            diag::detail("Knopftest: Ansichten - " + t);
        }));
        add(pause(0.3));
        add(klick(tr(Str::FlowGraph)));
        add(pause(0.8));
        add(foto("ans_graph"));
        add(pruefe("Graph: Ansicht aktiv; Start -target-> Starter -usescript-> intro; npc1 als Knoten", [=] {
            diag::detail("Knopftest: Ansichten - Graph " + std::to_string(g_app->graph().g.knoten.size()) + " Knoten, " +
                         std::to_string(g_app->graph().g.kanten.size()) + " Kanten");
            return g_app->settings.flussAnsicht == 1 && kanteDa((*schl)["start"], (*schl)["introStarter"], "target") &&
                   kanteDa((*schl)["introStarter"], (*schl)["intro"], "usescript") &&
                   knotenPunkt((*schl)["npc1"], false).x >= 0.0F;
        }));
        // Verbinden mit der Maus: vom Ausgang des Starters runner1 auf die Figur npc1.
        auto vonP = std::make_shared<ImVec2>();
        auto nachP = std::make_shared<ImVec2>();
        add(tu("Graph: Punkte fuer die Verbindung", [=] {
            *vonP = knotenPunkt((*schl)["runner1"], true);
            *nachP = knotenPunkt((*schl)["npc1"], false);
            diag::detail("Knopftest: Ansichten - ziehe von " + std::to_string(vonP->x) + "/" + std::to_string(vonP->y) +
                         " nach " + std::to_string(nachP->x) + "/" + std::to_string(nachP->y));
        }));
        add({"Ziehe im Graph: runner1 -> npc1", [=](int b) {
                 ImGuiIO& io = ImGui::GetIO();
                 if (b == 0) { setzeMaus(*vonP); return false; }
                 if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
                 if (b >= 3 && b <= 14) {
                     const float f = static_cast<float>(b - 2) / 12.0F;
                     setzeMaus(ImVec2{vonP->x + (nachP->x - vonP->x) * f, vonP->y + (nachP->y - vonP->y) * f});
                     return false;
                 }
                 if (b == 15) { io.AddMouseButtonEvent(0, false); return false; }
                 return b >= 20;
             }});
        add(pause(0.4));
        add(pruefe("Graph-Verbindung mit der Maus: runner1.target = targetname von npc1, die Kante ist da", [=] {
            const MapEntity* r = nullptr;
            const MapEntity* n = nullptr;
            for (const MapEntity& e : g_app->map.entities) {
                if (e.classname == "target_scriptrunner" && e.targetname == "runner1") { r = &e; }
                if (e.classname == "NPC_spawner" && mb::wert(&e, "NPC_targetname") == "npc1") { n = &e; }
            }
            diag::detail("Knopftest: Ansichten - runner1.target \"" + mb::wert(r, "target") + "\", npc1.targetname \"" +
                         mb::wert(n, "targetname") + "\"");
            return r != nullptr && n != nullptr && !n->targetname.empty() && mb::wert(r, "target") == n->targetname &&
                   kanteDa((*schl)["runner1"], (*schl)["npc1"], "target");
        }));
        // Skript -> Entity: ein use ans Ende des Skripts.
        add(tu("Graph: intro -> runner1 verbinden (use)", [=] {
            const std::string t = graphVerbinden((*schl)["intro"], (*schl)["runner1"], std::string());
            diag::detail("Knopftest: Ansichten - " + t);
        }));
        add(pause(0.3));
        add(pruefe("intro hat am Ende use ( \"runner1\" ), die Kante ist da, Ansicht bleibt Graph", [=] {
            const auto& nodes = g_app->doc.script().nodes;
            const bool use = !nodes.empty() && nodes.back().name == "use" && !nodes.back().args.empty() &&
                             nodes.back().args[0].text == "runner1";
            return mb::reiterName(static_cast<std::size_t>(g_app->activeTab)) == "intro.txt" && use &&
                   kanteDa((*schl)["intro"], (*schl)["runner1"], "use") && g_app->settings.flussAnsicht == 1;
        }));
        // --- rc597: rechtwinklige Leitungen, Umlenkpunkte, Ablegen von links ---
        add(tu("Graph: einpassen (rechtwinklig ist voreingestellt)", [] {
            g_app->graph().angepasst = false;
            g_app->graph().rechtwinklig = true;
        }));
        add(pause(0.3));
        add(pruefe("Graph rechtwinklig: keine schraegen Stuecke, keine Leitung quer durch einen Knoten", [] {
            const App::GraphZustand& gz = g_app->graph();
            diag::detail("Knopftest: Graph - " + std::to_string(gz.kantenMitte.size()) + " Kanten, schraeg " +
                         std::to_string(gz.schraegeStuecke) + ", Kreuzungen " + std::to_string(gz.kreuzungen));
            return !gz.kantenMitte.empty() && gz.schraegeStuecke == 0 && gz.kreuzungen == 0;
        }));
        auto kante = std::make_shared<std::string>();
        add(tu("Graph: eine Leitung waehlen", [=] {
            const App::GraphZustand& gz = g_app->graph();
            kante->clear();
            for (const auto& [k, p] : gz.kantenMitte) {
                if (p.x > gz.ursprung.x + 20.0F && p.y > gz.ursprung.y + 20.0F && p.x < gz.ursprung.x + gz.groesse.x - 20.0F &&
                    p.y < gz.ursprung.y + gz.groesse.y - 20.0F) {
                    *kante = k;
                    break;
                }
            }
        }));
        // kantenMitte liegt auf der Linie (die Beschriftung steht darueber).
        add(klickAn("Doppelklick auf die Leitung", [=] {
            const auto& km = g_app->graph().kantenMitte;
            const auto it = km.find(*kante);
            return it != km.end() ? it->second : ImVec2{-1.0F, -1.0F};
        }, 0, true));
        add(pause(0.2));
        add(pruefe("Doppelklick auf eine Leitung setzt einen Umlenkpunkt", [=] {
            const auto& u = g_app->graph().umlenk;
            const auto it = u.find(*kante);
            diag::detail("Knopftest: Umlenkpunkte an \"" + *kante + "\": " + std::to_string(it != u.end() ? it->second.size() : 0U));
            return !kante->empty() && it != u.end() && it->second.size() == 1U;
        }));
        add(klickAn("Rechtsklick auf den Umlenkpunkt", [=] {
            const App::GraphZustand& gz = g_app->graph();
            const auto it = gz.umlenk.find(*kante);
            if (it == gz.umlenk.end() || it->second.empty()) { return ImVec2{-1.0F, -1.0F}; }
            const ImVec2 p = it->second.front();
            return ImVec2{gz.ursprung.x + gz.panX + p.x * gz.zoom, gz.ursprung.y + gz.panY + p.y * gz.zoom};
        }, 1));
        add(pause(0.2));
        add(pruefe("Rechtsklick entfernt den Umlenkpunkt wieder", [=] {
            return g_app->graph().umlenk.find(*kante) == g_app->graph().umlenk.end();
        }));
        // Befehl auf freie Flaeche: neues Skript an der Abwurfstelle. Vorher
        // alle Knoten weit nach rechts schieben, damit die Mitte frei ist.
        auto reiterVorher = std::make_shared<std::size_t>(0);
        auto modusVorher = std::make_shared<int>(0);
        add(tu("Graph: die Mitte freimachen, links die Ereignisliste", [=] {
            *reiterVorher = g_app->tabs.size();
            *modusVorher = g_app->leftMode;
            g_app->leftMode = 0;
            g_app->graph().panX += g_app->graph().groesse.x * 3.0F;
        }));
        add(pause(0.2));
        add(ziehe("flush  ", "", "##graphflaeche", "", 0, 0, 0.5F, true, ImVec2{0.0F, 0.0F}, true));
        add(pause(0.3));
        add(pruefe("Befehl auf freie Flaeche im Graph: neues Skript mit dem Befehl, als Knoten gewaehlt", [=] {
            const App::GraphZustand& gz = g_app->graph();
            const std::string name = mb::reiterName(static_cast<std::size_t>(g_app->activeTab));
            bool knoten = false;
            for (const GraphKnoten& k : gz.g.knoten) { knoten = knoten || k.schluessel == gz.auswahl; }
            diag::detail("Knopftest: Graph-Ablegen - Reiter \"" + name + "\", Auswahl \"" + gz.auswahl + "\", " +
                         std::to_string(g_app->tabs.size()) + " Reiter");
            return g_app->tabs.size() == *reiterVorher + 1U && name.rfind("neu", 0) == 0 &&
                   !g_app->doc.script().nodes.empty() && g_app->doc.script().nodes.front().name == "flush" &&
                   gz.auswahl.rfind("s:", 0) == 0 && gz.auswahl.find("neu") != std::string::npos && knoten;
        }));
        add(tu("Graph: den neuen Reiter wieder schliessen, linke Spalte wie vorher", [=] {
            if (mb::reiterName(static_cast<std::size_t>(g_app->activeTab)).rfind("neu", 0) == 0) {
                g_app->doc.markSaved();
                reiterSchliessen(g_app->activeTab);
            }
            mb::zumReiter("intro.txt");
            g_app->leftMode = *modusVorher;
        }));
        add(pause(0.4));   // das neue Layout einschwingen lassen
        add(tu("Graph: wieder einpassen", [] { g_app->graph().angepasst = false; }));
        add(pause(0.3));
        // Doppelklick auf das Skript runner1 oeffnet seinen Reiter.
        add(tu("Graph: den Knoten runner1 (Skript) in die Mitte holen", [=] {
            App::GraphZustand& gz = g_app->graph();
            for (const GraphKnoten& k : gz.g.knoten) {
                if (k.schluessel == (*schl)["runner1Skript"]) {
                    const float s = ImGui::GetFontSize() / 13.0F;
                    gz.panX = gz.groesse.x * 0.5F - (k.x + 95.0F * s) * gz.zoom;
                    gz.panY = gz.groesse.y * 0.5F - (k.y + 31.0F * s) * gz.zoom;
                }
            }
        }));
        add(pause(0.2));
        add(klickAn("Doppelklick auf runner1 (Skript)", [=] { return knotenPunkt((*schl)["runner1Skript"], false); }, 0, true));
        add(pause(0.4));
        add(pruefe("Doppelklick: der Reiter runner1.txt ist vorn", [] {
            return mb::reiterName(static_cast<std::size_t>(g_app->activeTab)) == "runner1.txt";
        }));
        // --- Spuren ---
        add(tu("zurueck ins intro", [] { mb::zumReiter("intro.txt"); }));
        add(klick(tr(Str::FlowTracks)));
        add(pause(0.8));
        add(foto("ans_spuren"));
        add(pruefe("Spuren: Ansicht aktiv, Bloecke gezeichnet", [] {
            diag::detail("Knopftest: Ansichten - Spuren " + std::to_string(g_app->spurenBloecke.size()) + " Bloecke");
            return g_app->settings.flussAnsicht == 2 && !g_app->spurenBloecke.empty();
        }));
        auto waitPfad = std::make_shared<Path>();
        auto waitRect = std::make_shared<ImRect>();
        add(tu("Spuren: den Block des wait suchen", [=] {
            const auto& nodes = g_app->doc.script().nodes;
            for (std::size_t i = 0; i < nodes.size(); ++i) {
                if (nodes[i].name == "wait") { *waitPfad = Path{i}; }
            }
            for (const auto& [p, r] : g_app->spurenBloecke) {
                if (p == *waitPfad) { *waitRect = r; }
            }
            diag::detail("Knopftest: Ansichten - wait-Block " + std::to_string(waitRect->Min.x) + ".." + std::to_string(waitRect->Max.x));
        }));
        // Die Beschriftung zeigt den Wert aus dem Skript - nicht die Dauer des
        // Nachbaus, die einen Takt laenger ist (Fotolauf rc601: "wait ( 3050 )"
        // auf dem Block, "wait ( 3000 )" im Skript).
        add(pruefe("Spuren: der wait-Block traegt den Wert aus dem Skript", [=] {
            const Node* n = nodeAt(g_app->doc.script(), *waitPfad);
            if (n == nullptr || n->args.empty()) { return false; }
            const std::string soll = "wait ( " + n->args[0].text + " )";
            for (const TimelineTrack& t : g_app->timeline.tracks) {
                for (const TimelineEvent& e : t.events) {
                    if (e.path == *waitPfad) {
                        diag::detail("Knopftest: Ansichten - wait-Block \"" + e.label + "\", Skript \"" + soll + "\"");
                        return e.label == soll;
                    }
                }
            }
            return false;
        }));
        add(klickAn("auf den wait-Block", [=] { return ImVec2{(waitRect->Min.x + waitRect->Max.x) * 0.5F, waitRect->GetCenter().y}; }));
        add(pause(0.3));
        add(pruefe("Spuren: Klick auf den Block waehlt seine Zeile", [=] { return !waitPfad->empty() && g_app->selectedPath == *waitPfad; }));
        add({"Ziehe die rechte Kante des wait um 80 Punkte", [=](int b) {
                 ImGuiIO& io = ImGui::GetIO();
                 const ImVec2 start{waitRect->Max.x - 1.0F, waitRect->GetCenter().y};
                 if (b == 0) { setzeMaus(start); return false; }
                 if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
                 if (b >= 3 && b <= 14) {
                     setzeMaus(ImVec2{start.x + 80.0F * static_cast<float>(b - 2) / 12.0F, start.y});
                     return false;
                 }
                 if (b == 15) { io.AddMouseButtonEvent(0, false); return false; }
                 return b >= 20;
             }});
        add(pause(0.3));
        auto waitNeu = std::make_shared<double>(0.0);
        add(pruefe("Spuren: das wait ist laenger geworden", [=] {
            const Node* n = nodeAt(g_app->doc.script(), *waitPfad);
            *waitNeu = n != nullptr && !n->args.empty() ? std::atof(n->args[0].text.c_str()) : 0.0;
            diag::detail("Knopftest: Ansichten - wait jetzt " + std::to_string(*waitNeu));
            return *waitNeu > 3000.5;
        }));
        // --- Classic: dieselbe Aenderung, Rueckgaengig wirkt ueberall ---
        add(klick(tr(Str::FlowClassic)));
        add(pause(0.4));
        add(pruefe("Classic: Ansicht aktiv, die Zeile ist noch gewaehlt", [=] {
            return g_app->settings.flussAnsicht == 0 && g_app->selectedPath == *waitPfad;
        }));
        add(tu("Maus weg von der Karte", [] { setzeMaus(ImVec2{ImGui::GetIO().DisplaySize.x - 30.0F, 200.0F}); }));
        add(pause(0.2));
        add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z in Classic"));
        add(pause(0.3));
        add(pruefe("Strg+Z nimmt die Aenderung aus den Spuren zurueck", [=] {
            const Node* n = nodeAt(g_app->doc.script(), *waitPfad);
            const double w = n != nullptr && !n->args.empty() ? std::atof(n->args[0].text.c_str()) : 0.0;
            diag::detail("Knopftest: Ansichten - nach Strg+Z wait " + std::to_string(w));
            return std::fabs(w - 3000.0) < 0.5;
        }));
        add(klick(tr(Str::FlowTracks)));
        add(pause(0.4));
        add(pruefe("Spuren zeigen den alten Stand", [=] {
            for (const auto& [p, r] : g_app->spurenBloecke) {
                if (p == *waitPfad) { return std::fabs(r.GetWidth() - waitRect->GetWidth()) < 3.0F; }
            }
            return false;
        }));
        // --- Bearbeiten in Spuren und Graph (rc601) ---
        //
        // shank: "das jedes Tutorial in jedem Modus erstellt werden kann".
        // Vorher liess sich in den Spuren nur waehlen und die Dauer ziehen, im
        // Graphen nur ein Befehl ans Ende eines Skripts werfen. Jetzt: Doppelklick
        // oeffnet den Editor, Rechtsklick das Menue der Classic-Zeile, und ein
        // Befehl aus der Ereignisliste landet dort, wo man ihn fallen laesst.
        auto stand = std::make_shared<std::size_t>(0);
        auto anzahl = std::make_shared<std::size_t>(0);
        const auto blockVon = [](const Path& p) {
            for (const auto& [q, r] : g_app->spurenBloecke) {
                if (q == p) { return r; }
            }
            return ImRect{};
        };
        const auto zurueck = [=](const std::string& was) {
            return tu(was, [=] {
                g_app->editorOpen = false;
                while (g_app->doc.undoDepth() > *stand && g_app->doc.undo()) {}
                rebuildTree();
                g_app->camTrackValid = false;
            });
        };
        // Aus der Ereignisliste an eine Stelle ziehen (Ziel erst im Zug bestimmt).
        const auto ausListe = [](const std::string& befehl, std::function<ImVec2()> ziel) {
            auto start = std::make_shared<ImVec2>();
            return Schritt{"Ziehe " + befehl + " aus der Ereignisliste", [=](int b) {
                               ImGuiIO& io = ImGui::GetIO();
                               if (b == 0) {
                                   const Element* e = finde(befehl + "  ", "", 0, true);
                                   if (e == nullptr) {
                                       meldeFehler(befehl + " nicht in der Ereignisliste");
                                       return true;
                                   }
                                   *start = ImVec2{e->rect.Min.x + 20.0F, e->rect.GetCenter().y};
                                   setzeMaus(*start);
                                   return false;
                               }
                               if (b == 1) { io.AddMouseButtonEvent(0, true); return false; }
                               if (b >= 2 && b <= 6) {
                                   setzeMaus(ImVec2{start->x + 5.0F * static_cast<float>(b - 1), start->y + 2.0F * static_cast<float>(b - 1)});
                                   return false;
                               }
                               if (b >= 7 && b <= 14) {
                                   const ImVec2 z = ziel();
                                   const float f = std::min(1.0F, static_cast<float>(b - 6) / 5.0F);
                                   const ImVec2 a{start->x + 25.0F, start->y + 10.0F};
                                   setzeMaus(ImVec2{a.x + (z.x - a.x) * f, a.y + (z.y - a.y) * f});
                                   return false;
                               }
                               if (b == 15) {
                                   if (!GImGui->DragDropActive) { meldeFehler(befehl + ": kein Zug aktiv vor dem Loslassen"); }
                                   io.AddMouseButtonEvent(0, false);
                                   return false;
                               }
                               return b >= 19;
                           }};
        };
        const auto zeileImGraph = [](const std::string& was) {
            const Element* e = finde(was, "befehlsliste", 0, true);
            return e == nullptr ? ImVec2{-1.0F, -1.0F} : ImVec2{e->rect.Min.x + 30.0F, e->rect.GetCenter().y};
        };
        add(tu("Spuren bearbeiten: Stand merken", [=] {
            *stand = g_app->doc.undoDepth();
            *anzahl = g_app->doc.script().nodes.size();
            g_app->leftMode = 0;
        }));
        add(pause(0.3));
        add(klickAn("Doppelklick auf den wait-Block", [=] { return blockVon(*waitPfad).GetCenter(); }, 0, true));
        add(pause(0.5));
        add(pruefe("Spuren: Doppelklick oeffnet den Editor fuer das wait", [=] {
            return g_app->editorOpen && g_app->selectedPath == *waitPfad;
        }));
        add(tu("Editor zu", [] { g_app->editorOpen = false; }));
        add(pause(0.3));
        add(klickAn("Rechtsklick auf den wait-Block", [=] { return blockVon(*waitPfad).GetCenter(); }, 1));
        add(pause(0.4));
        add(foto("ans_spuren_menue"));
        add(klick(tr(Str::ActClone)));
        add(pause(0.4));
        add(pruefe("Spuren: Rechtsklick > Duplizieren verdoppelt das wait", [=] {
            const auto& ns = g_app->doc.script().nodes;
            return ns.size() == *anzahl + 1 && waitPfad->front() + 1 < ns.size() && ns[waitPfad->front() + 1].name == "wait";
        }));
        add(zurueck("Spuren: Duplizieren zuruecknehmen"));
        add(pause(0.4));
        add(klickAn("Rechtsklick auf den wait-Block", [=] { return blockVon(*waitPfad).GetCenter(); }, 1));
        add(pause(0.4));
        add(klick(tr(Str::ActDelete)));
        add(pause(0.4));
        add(pruefe("Spuren: Rechtsklick > Loeschen nimmt das wait weg", [=] {
            return g_app->doc.script().nodes.size() == *anzahl - 1;
        }));
        add(zurueck("Spuren: Loeschen zuruecknehmen"));
        add(pause(0.4));
        add(ausListe("print", [=] {
            const ImRect r = blockVon(*waitPfad);
            return ImVec2{r.Min.x + std::min(12.0F, r.GetWidth() * 0.3F), r.GetCenter().y};
        }));
        add(pause(0.4));
        add(pruefe("Spuren: print in die Skript-Spur gezogen steht hinter dem wait", [=] {
            const auto& ns = g_app->doc.script().nodes;
            const std::size_t i = waitPfad->front() + 1;
            diag::detail("Knopftest: Ansichten - nach dem Ablegen " + std::to_string(ns.size()) + " Zeilen, hinter wait: " +
                         (i < ns.size() ? ns[i].name : std::string("-")));
            return ns.size() == *anzahl + 1 && i < ns.size() && ns[i].name == "print";
        }));
        add(zurueck("Spuren: print zuruecknehmen"));
        add(pause(0.4));
        // Die Liste rechts: auch was keine Zeit hat (affect, declare ...).
        add(pruefe("Spuren: die Befehlsliste rechts zeigt jede Zeile des Skripts", [=] { return zeileImGraph("wait (").x >= 0.0F; }));
        add(tu("Spuren: leeres Skript in einem neuen Reiter", [] { neuerReiter(); g_app->settings.flussAnsicht = 2; }));
        add(pause(0.5));
        add(foto("ans_spuren_leer"));
        add(ausListe("affect", [] {
            const Element* e = finde("##listenende", "spurenliste", 0, false);
            return e == nullptr ? ImVec2{-1.0F, -1.0F} : e->rect.GetCenter();
        }));
        add(pause(0.4));
        add(pruefe("Spuren: affect in die leere Liste gezogen - das Skript hat eine Zeile", [] {
            const auto& ns = g_app->doc.script().nodes;
            return !ns.empty() && ns.back().name == "affect";
        }));
        add(tu("Spuren: den neuen Reiter verwerfen", [] {
            g_app->doc.markSaved();
            reiterSchliessen(g_app->activeTab);
            mb::zumReiter("intro.txt");
        }));
        add(pause(0.5));
        // Graph: die Befehle des offenen Skripts in den Details.
        add(klick(tr(Str::FlowGraph)));
        add(pause(0.8));
        add(tu("Graph: das intro waehlen", [=] { g_app->graph().auswahl.clear(); }));
        add(pause(0.5));
        add(foto("ans_graph_befehle"));
        add(pruefe("Graph: die Befehle des intro stehen in den Details", [=] { return zeileImGraph("wait (").x >= 0.0F; }));
        add(tu("Graph: Auswahl leeren", [] { g_app->selectedPath.clear(); g_app->selection.clear(); }));
        add(klickAn("Graph: Klick auf die wait-Zeile", [=] { return zeileImGraph("wait ("); }));
        add(pause(0.3));
        add(pruefe("Graph: Klick waehlt die Zeile", [=] { return g_app->selectedPath == *waitPfad; }));
        add(klickAn("Graph: Doppelklick auf die wait-Zeile", [=] { return zeileImGraph("wait ("); }, 0, true));
        add(pause(0.5));
        add(pruefe("Graph: Doppelklick oeffnet den Editor", [=] { return g_app->editorOpen && g_app->selectedPath == *waitPfad; }));
        add(tu("Editor zu", [] { g_app->editorOpen = false; }));
        add(pause(0.3));
        add(klickAn("Graph: Rechtsklick auf die wait-Zeile", [=] { return zeileImGraph("wait ("); }, 1));
        add(pause(0.4));
        add(klick(tr(Str::ActDelete)));
        add(pause(0.4));
        add(pruefe("Graph: Rechtsklick > Loeschen nimmt das wait weg", [=] { return g_app->doc.script().nodes.size() == *anzahl - 1; }));
        add(zurueck("Graph: Loeschen zuruecknehmen"));
        add(pause(0.4));
        add(ausListe("print", [=] { return zeileImGraph("wait ("); }));
        add(pause(0.4));
        add(pruefe("Graph: print auf die wait-Zeile gezogen steht dahinter", [=] {
            const auto& ns = g_app->doc.script().nodes;
            const std::size_t i = waitPfad->front() + 1;
            return ns.size() == *anzahl + 1 && i < ns.size() && ns[i].name == "print";
        }));
        add(zurueck("Graph: print zuruecknehmen"));
        add(pause(0.3));
        add(pruefe("Spuren und Graph: alles zurueckgenommen", [=] {
            return g_app->doc.undoDepth() == *stand && g_app->doc.script().nodes.size() == *anzahl;
        }));
        add(klick(tr(Str::FlowTracks)));
        add(pause(0.4));
        // --- Schnell hin und her ---
        auto vorher = std::make_shared<std::pair<std::size_t, Path>>();
        add(tu("Stand vor dem schnellen Umschalten", [=] {
            *vorher = {g_app->doc.undoDepth(), g_app->selectedPath};
            std::vector<Schritt> folge;
            for (int k = 0; k < 24; ++k) {
                folge.push_back(tu("Umschalten " + std::to_string(k), [k] { g_app->settings.flussAnsicht = (k * 2 + 1) % 3; }));
                folge.push_back(pause(0.03));
            }
            schritteEinfuegen(std::move(folge));
        }));
        add(pause(0.3));
        add(pruefe("24 mal umgeschaltet: Dokument und Auswahl unveraendert", [=] {
            return g_app->doc.undoDepth() == vorher->first && g_app->selectedPath == vorher->second;
        }));
        // --- Entity-Rueckgaengig wirkt auch im Graph ---
        add(tu("Graph, zwei Entity-Schritte zurueck (target und targetname)", [] {
            g_app->settings.flussAnsicht = 1;
            entityRueckgaengig();
            entityRueckgaengig();
        }));
        add(pause(0.4));
        add(pruefe("Graph: die Verbindung runner1 -> npc1 ist wieder weg", [=] {
            return !kanteDa((*schl)["runner1"], (*schl)["npc1"], "target");
        }));
        add(tu("zurueck zu Classic", [] { g_app->settings.flussAnsicht = 0; }));
        add(pause(0.3));
        // --- Geteilte Ansicht: jedes Feld seine eigene Ansicht ---
        //
        // "und koennen die 2 neuen ansichten auch in split view funktionieren?"
        // Links intro in Classic (Fokus), rechts runner1 - dort Spuren, dann
        // Graph. Graph und Spuren folgen dem Skript mit dem Fokus.
        auto teilX = std::make_shared<float>(0.0F);
        add(tu("Zwei Felder: links intro, rechts runner1", [=] {
            mb::zumReiter("intro.txt");
            g_app->leftMode = 0;   // geteilt wird im Events-Modus
            g_app->splitCount = 2;
            g_app->focusPane = 0;
            g_app->splitPanes[0].tab = g_app->activeTab;
            for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
                if (mb::reiterName(i) == "runner1.txt") { g_app->splitPanes[1].tab = static_cast<int>(i); }
            }
            for (auto& pn : g_app->splitPanes) { pn.dirty = true; }
        }));
        add(pause(0.5));
        add(klick(tr(Str::FlowTracks), "", 1));
        add(pause(0.6));
        add(foto("ans_geteilt_spuren"));
        add(pruefe("Geteilt: rechts Spuren, links weiter Classic, Fokus und Reiter unveraendert", [=] {
            // Die linke Kante des rechten Feldes: vor seinem Classic-Knopf steht
            // nur die Ueberschrift.
            const Element* e = finde(tr(Str::FlowClassic), "", 1, false, "");
            *teilX = e != nullptr ? e->rect.Min.x - ImGui::GetFontSize() * 7.0F : 1.0e9F;
            bool rechts = !g_app->spurenBloecke.empty();
            for (const auto& [p, r] : g_app->spurenBloecke) { rechts = rechts && r.Min.x > *teilX; }
            diag::detail("Knopftest: Ansichten - geteilt, Feld 0 " + std::to_string(feldAnsicht(0)) + ", Feld 1 " +
                         std::to_string(feldAnsicht(1)) + ", " + std::to_string(g_app->spurenBloecke.size()) + " Bloecke rechts von " +
                         std::to_string(*teilX));
            return feldAnsicht(0) == 0 && feldAnsicht(1) == 2 && rechts && g_app->focusPane == 0 &&
                   mb::reiterName(static_cast<std::size_t>(g_app->activeTab)) == "intro.txt";
        }));
        add(tu("Geteilt: den Block des wait rechts suchen", [=] {
            for (const auto& [p, r] : g_app->spurenBloecke) {
                if (p == *waitPfad) { *waitRect = r; }
            }
            g_app->selectedPath.clear();
        }));
        add(klickAn("Geteilt: auf den wait-Block rechts", [=] { return waitRect->GetCenter(); }));
        add(pause(0.3));
        add(pruefe("Geteilt: Klick rechts waehlt die Zeile im intro links, ohne Fokuswechsel", [=] {
            return g_app->selectedPath == *waitPfad && g_app->focusPane == 0 &&
                   mb::reiterName(static_cast<std::size_t>(g_app->activeTab)) == "intro.txt";
        }));
        add(klick(tr(Str::FlowGraph), "", 1));
        add(pause(0.6));
        add(klick(tr(Str::FlowGraph), "", 0));
        add(pause(0.6));
        add(foto("ans_geteilt_graph"));
        add(pruefe("Geteilt: zwei Graphen nebeneinander, jeder mit eigener Flaeche", [=] {
            const App::GraphZustand& a = g_app->graphe[0];
            const App::GraphZustand& b = g_app->graphe[1];
            diag::detail("Knopftest: Ansichten - Graph links bei " + std::to_string(a.ursprung.x) + " (" +
                         std::to_string(a.g.knoten.size()) + " Knoten), rechts bei " + std::to_string(b.ursprung.x) + " (" +
                         std::to_string(b.g.knoten.size()) + " Knoten)");
            return feldAnsicht(0) == 1 && feldAnsicht(1) == 1 && !a.g.knoten.empty() &&
                   a.g.knoten.size() == b.g.knoten.size() && b.ursprung.x > a.ursprung.x + a.groesse.x - 1.0F;
        }));
        add(klick(tr(Str::FlowClassic), "", 0));
        add(pause(0.4));
        add(pruefe("Geteilt: links wieder Classic mit dem intro, rechts weiter Graph", [=] {
            return feldAnsicht(0) == 0 && feldAnsicht(1) == 1 && finde("##splitpick0", "", 0, false, "") != nullptr;
        }));
        add(tu("wieder ein Feld, zurueck in die Karte", [] {
            g_app->leftMode = 1;
            g_app->splitPanes[1].ansicht = 0;
            g_app->splitCount = 1;
            g_app->focusPane = 0;
            for (auto& pn : g_app->splitPanes) { pn.dirty = true; }
        }));
        add(pause(0.3));
    }
    // ---------------- Save mission ----------------
    add(klick(tr(Str::MissionSave)));
    add(pause(0.4));
    add(pruefe("Save mission: Fenster mit Dateiliste", [] {
        return g_app->missionSpeichernOffen && finde(tr(Str::MissionSaveNow), "##missionspeichern", 0, false) != nullptr;
    }));
    add(klick(tr(Str::MissionCancel), "##missionspeichern"));
    add(pause(0.3));
    add(tu("Speichern in den Spielordner des Laufs", [=] {
        z->pk3 = mb::verbinde(z->spiel, "zz_" + g_app->mission.name + ".pk3");
        std::string f;
        z->gespeichert = g_app->map.entities;
        z->vorher = missionSpeichernNach(z->pk3, &f) ? 1 : 0;
        diag::detail("Knopftest: Missionsbau - gespeichert nach " + z->pk3 + ": " + (z->vorher != 0 ? "ja" : "nein " + f));
        // Eine Kopie dieses sauberen Stands fuer den Spieltest (vor dem Zufall;
        // andere Endung, damit sie kein Spielarchiv ist).
        std::error_code ec;
        std::filesystem::copy_file(::bhed::utf8Pfad(z->pk3), ::bhed::utf8Pfad(z->pk3 + ".erste"),
                                   std::filesystem::copy_options::overwrite_existing, ec);
    }));
    add(pruefe("Save mission: pk3 mit Karte, .ent, Skripten und .IBI; nichts mehr ungespeichert", [=] {
        const auto inhalt = mb::pk3Inhalt(z->pk3);
        const App::MissionsProjekt& m = g_app->mission;
        const std::string* ent = mb::datei(inhalt, "maps/" + m.name + ".ent");
        const std::string* ibi = mb::datei(inhalt, "scripts/" + m.ordner + "/intro.IBI");
        const std::string* ibi2 = mb::datei(inhalt, "scripts/" + m.ordner + "/runner1.IBI");
        std::vector<MapEntity> gelesen;
        if (ent != nullptr) { parseEntities(*ent, gelesen); }
        bool gleich = gelesen.size() == z->gespeichert.size();
        for (std::size_t i = 0; gleich && i < gelesen.size(); ++i) { gleich = gelesen[i].keys == z->gespeichert[i].keys; }
        bool sauber = true;
        for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
            const bool aktiv = static_cast<int>(i) == g_app->activeTab;
            const std::string n = mb::reiterName(i);
            const bool schmutzig = aktiv ? g_app->doc.dirty() : g_app->tabs[i].doc.dirty();
            if ((n == "intro.txt" || n == "runner1.txt") && schmutzig) { sauber = false; }
        }
        diag::detail("Knopftest: Missionsbau - pk3 " + std::to_string(inhalt.size()) + " Dateien, .ent " +
                     std::to_string(gelesen.size()) + " Entities, Einblendung \"" +
                     (g_app->einblendungen.empty() ? std::string() : g_app->einblendungen.back().text) + "\"");
        const bool eingeblendet = !g_app->einblendungen.empty() &&
                                  g_app->einblendungen.back().text.find("zz_" + m.name) != std::string::npos &&
                                  !g_app->einblendungen.back().ordner.empty();
        return z->vorher == 1 && eingeblendet && mb::datei(inhalt, "maps/" + m.name + ".bsp") != nullptr && gleich && ibi != nullptr &&
               ibi->compare(0, 3, "IBI") == 0 && ibi2 != nullptr && sauber && g_app->entitiesUngespeichert.empty();
    }));
    // Ein kaputtes Skript: Speichern verweigert, das Archiv bleibt
    add(tu("runner1 kaputt machen und speichern", [=] {
        mb::zumReiter("runner1.txt");
        Script kaputt;
        std::vector<Diag> d;
        // Eine Zeile, die sich nicht lesen laesst - sie bleibt wortgetreu
        // stehen, und das Uebersetzen verweigert (C005). Ein fehlendes '}'
        // dagegen ergaenzt das Speichern, wie bei jedem Skript.
        (void)readScript("//Generated by BehavEd\n\nwait ( 1.000\n", kaputt, d);
        g_app->doc = Document{kaputt};
        g_app->doc.markSaved();
        std::error_code ec;
        const auto vorher = std::filesystem::last_write_time(::bhed::utf8Pfad(z->pk3), ec);
        std::string f;
        const bool ok = missionSpeichernNach(z->pk3, &f);
        const auto nachher = std::filesystem::last_write_time(::bhed::utf8Pfad(z->pk3), ec);
        diag::detail("Knopftest: Missionsbau - kaputtes Skript: " + std::string(ok ? "gespeichert" : "verweigert") + " (" + f + ")");
        z->vorher = (!ok && vorher == nachher && !f.empty()) ? 1 : 0;
        // Wieder heil machen.
        Script heil;
        (void)readScript("//Generated by BehavEd\n\nwait ( 500.000 );\n", heil, d);
        g_app->doc = Document{heil};
    }));
    add(pruefe("Kaputtes Skript: Speichern verweigert, pk3 unveraendert", [=] { return z->vorher == 1; }));

    // ---------------- Laden aus der gespeicherten pk3 ----------------
    add(tu("Spielordner des Laufs eintragen, Mission laden", [=] {
        addGamePathDirectory(z->spiel);
        z->gespeichert.clear();
        for (const auto& [n, t] : mb::pk3Inhalt(z->pk3)) {
            if (n.size() > 4 && n.compare(n.size() - 4, 4, ".ent") == 0) { parseEntities(t, z->gespeichert); }
        }
        missionenAusArchiv(z->pk3);
    }));
    // rc603: "Load mission" nennt die Missionen des ganzen Spiels (der
    // Spielordner des Laufs liegt bei den eingerichteten) - die des
    // gewaehlten Archivs stehen vorn; die gespeicherte Mission waehlen.
    add(pruefe("Auswahl: die Mission der gespeicherten pk3 steht vorn (im gewaehlten Archiv)", [] {
        const bool vorn = g_app->missionPickOpen && !g_app->missions.empty() &&
                          g_app->missions[0].name == g_app->mission.name && g_app->missions[0].imGewaehlten;
        diag::detail("Knopftest: Missionsbau - Auswahl " + std::string(g_app->missionPickOpen ? "offen" : "zu") + ", vorn \"" +
                     (g_app->missions.empty() ? std::string("-") : g_app->missions[0].name) + "\", Mission \"" +
                     g_app->mission.name + "\", " + std::to_string(g_app->missions.size()) + " Karten");
        return vorn;
    }));
    add(tu("Auswahl: die gespeicherte Mission waehlen", [] {
        if (g_app->missionPickOpen) {
            schritteEinfuegen({waehleInListe(g_app->mission.name, false, 0, "##missionpick")});
        }
    }));
    add(warteBis("gespeicherte Mission geladen", [] { return g_app->camTrackValid && g_app->missionAktiv; }, 1500));
    add(pruefe("Geladen: genau die gespeicherten Entities", [=] {
        bool gleich = g_app->map.entities.size() == z->gespeichert.size() && !z->gespeichert.empty();
        for (std::size_t i = 0; gleich && i < z->gespeichert.size(); ++i) { gleich = g_app->map.entities[i].keys == z->gespeichert[i].keys; }
        diag::detail("Knopftest: Missionsbau - geladen " + std::to_string(g_app->map.entities.size()) + ", gespeichert " +
                     std::to_string(z->gespeichert.size()) + ", Karte " + g_app->map.path);
        return gleich;
    }));
    // Die Skripte der Mission waren schon offen (aus dem Missionsordner) -
    // das Laden darf sie nicht ein zweites Mal oeffnen (11.10.).
    add(pruefe("Geladen: kein Skript doppelt offen", [=] {
        int intro = 0;
        int runner = 0;
        for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
            intro += mb::reiterName(i) == "intro.txt" ? 1 : 0;
            runner += mb::reiterName(i) == "runner1.txt" ? 1 : 0;
        }
        diag::detail("Knopftest: Missionsbau - nach dem Laden intro.txt " + std::to_string(intro) + " mal, runner1.txt " +
                     std::to_string(runner) + " mal offen");
        return intro == 1 && runner == 1;
    }));
    // Aus dem Missionsordner wieder oeffnen (mission.txt) - wie nach einem
    // Neustart, wenn die Mission noch nicht gepackt war.
    add(tu("Mission aus ihrem Ordner oeffnen", [=] {
        std::string f;
        z->vorher = missionAusOrdnerLaden(g_app->mission.projektOrdner, &f) ? 1 : 0;
        diag::detail("Knopftest: Missionsbau - aus dem Ordner: " + std::string(z->vorher != 0 ? "ja" : "nein " + f));
    }));
    add(warteBis("Mission aus dem Ordner geladen", [] { return g_app->camTrackValid; }, 1500));
    add(pruefe("Aus dem Ordner: dieselben Entities, kein Skript doppelt", [=] {
        bool gleich = g_app->map.entities.size() == z->gespeichert.size();
        for (std::size_t i = 0; gleich && i < z->gespeichert.size(); ++i) { gleich = g_app->map.entities[i].keys == z->gespeichert[i].keys; }
        int intro = 0;
        for (std::size_t i = 0; i < g_app->tabs.size(); ++i) { intro += mb::reiterName(i) == "intro.txt" ? 1 : 0; }
        diag::detail("Knopftest: Missionsbau - Ordner: " + std::to_string(g_app->map.entities.size()) + " Entities (" +
                     std::to_string(z->gespeichert.size()) + "), intro.txt " + std::to_string(intro) + " mal");
        return z->vorher == 1 && gleich && intro == 1 && g_app->missionAktiv;
    }));
    // Ungespeichert + noch einmal laden: Rueckfrage, Nein laesst alles stehen
    add(tu("Aendern, dann dieselbe Mission noch einmal laden", [=] {
        neueEntityHier("waypoint_navgoal", false);
        z->vorher = static_cast<int>(g_app->map.entities.size());
        missionenAusArchiv(z->pk3);
    }));
    add(pause(0.3));
    add(tu("Auswahl: dieselbe Mission waehlen (rc603: Missionen des ganzen Spiels)", [] {
        if (g_app->missionPickOpen) {
            schritteEinfuegen({waehleInListe(g_app->mission.name, false, 0, "##missionpick"), pause(0.3)});
        }
    }));
    add(pruefe("Rueckfrage vor dem Verwerfen", [] { return g_app->frageOffen; }));
    add(klick(tr(Str::AnswerNo), "###frage"));
    add(pause(0.3));
    add(pruefe("Nein: Aenderung bleibt", [=] {
        return static_cast<int>(g_app->map.entities.size()) == z->vorher && !g_app->entitiesUngespeichert.empty();
    }));

    // ---------------- Zufaellige Kombinationen ----------------
    add(tu("Zufall: 400 Handlungen", [=] {
        std::vector<Schritt> folge;
        static const char* kKlassen[] = {"NPC_spawner", "waypoint_navgoal", "ref_tag", "target_position",
                                         "target_scriptrunner", "fx_runner", "target_speaker", "info_player_start"};
        for (int k = 0; k < 400; ++k) {
            folge.push_back(tu("Zufall " + std::to_string(k), [=] {
                const std::uint32_t r = mb::zufall(*z);
                const int n = static_cast<int>(g_app->map.entities.size());
                const int beliebig = n > 0 ? static_cast<int>(mb::zufall(*z) % static_cast<std::uint32_t>(n)) : -1;
                std::string was;
                switch (r % 16U) {
                    case 0: case 1:
                        was = "neu";
                        neueEntityHier(kKlassen[mb::zufall(*z) % 8U], (mb::zufall(*z) & 1U) != 0U);
                        break;
                    case 2: was = "loeschen"; entityLoeschen(beliebig); break;
                    case 3: was = "duplizieren"; entityDuplizieren(beliebig); break;
                    case 4: {
                        was = "feld";
                        static const char* kKeys[] = {"targetname", "NPC_type", "spawnflags", "angle", "usescript", "radius",
                                                      "origin", "target", "count"};
                        static const char* kWerte[] = {"", "a", "x y", "0", "-1", "99999", "1 2 3", "md_test/x", "\"q\""};
                        entityKeySetzen(beliebig, kKeys[mb::zufall(*z) % 9U], kWerte[mb::zufall(*z) % 9U]);
                        break;
                    }
                    case 5: {
                        was = "flag";
                        if (beliebig >= 0) {
                            const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(beliebig)];
                            const int alt = e.find("spawnflags") != nullptr ? std::atoi(e.find("spawnflags")->c_str()) : 0;
                            entityKeySetzen(beliebig, "spawnflags", std::to_string(alt ^ (1 << (mb::zufall(*z) % 10U))));
                        }
                        break;
                    }
                    case 6: case 7: was = "undo"; entityRueckgaengig(); break;
                    case 8: was = "redo"; entityWiederholen(); break;
                    case 9: was = "boden"; entityAufBoden(beliebig); break;
                    case 10: was = "waehlen"; g_app->pickedEntity = beliebig; g_app->selectedActor = -1; break;
                    case 11: {
                        was = "reiter";
                        if (!g_app->tabs.empty()) { activateTab(static_cast<int>(mb::zufall(*z) % g_app->tabs.size())); }
                        break;
                    }
                    case 12: was = "roh"; g_app->entRawAnsicht = !g_app->entRawAnsicht; break;
                    case 13: {
                        was = "figur";
                        g_app->selectedActor = g_app->scene.actors.empty()
                                                   ? -1
                                                   : static_cast<int>(mb::zufall(*z) % g_app->scene.actors.size());
                        g_app->pickedEntity = -1;
                        break;
                    }
                    case 14: {
                        was = "speichern";
                        if ((mb::zufall(*z) % 4U) == 0U && g_app->missionAktiv) {
                            std::string f;
                            (void)missionSpeichernNach(z->pk3, &f);
                        }
                        break;
                    }
                    default: was = "szene"; g_app->camTrackValid = false; break;
                }
                ++z->handlungen[was];
                const std::string fehl = mb::invariantenFehler();
                if (!fehl.empty() && z->fehlerZufall < 10) {
                    ++z->fehlerZufall;
                    meldeFehler("Zufall " + std::to_string(k) + " (" + was + "): " + fehl);
                }
            }));
            if (k % 8 == 7) { folge.push_back(pause(0.05)); }
        }
        schritteEinfuegen(std::move(folge));
    }));
    add(pause(0.5));
    add(tu("Zufall: Zusammenfassung", [=] {
        std::string t;
        for (const auto& [k, v] : z->handlungen) { t += k + " " + std::to_string(v) + ", "; }
        diag::detail("Knopftest: Missionsbau - Zufall: " + t + "Fehler " + std::to_string(z->fehlerZufall) +
                     ", Entities " + std::to_string(g_app->map.entities.size()) + ", Reiter " + std::to_string(g_app->tabs.size()));
    }));
    add(pruefe("Zufall: alle Invarianten gehalten", [=] { return z->fehlerZufall == 0 && mb::invariantenFehler().empty(); }));

    // ---------------- Zum Schluss: speichern, laden, vergleichen ----------------
    add(tu("Missionsreiter vor, speichern", [=] { mb::zumReiter("intro.txt"); }));
    add(pause(0.5));
    add(tu("Endstand speichern", [=] {
        if (g_app->map.path != g_app->mission.karte) {
            diag::detail("Knopftest: Missionsbau - Missionskarte nicht vorn (" + g_app->map.path + ")");
        }
        z->gespeichert = g_app->entitiesJeKarte.count(g_app->mission.karte) != 0U
                             ? g_app->entitiesJeKarte[g_app->mission.karte]
                             : g_app->map.entities;
        std::string f;
        z->vorher = missionSpeichernNach(z->pk3, &f) ? 1 : 0;
        diag::detail("Knopftest: Missionsbau - Endstand gespeichert: " + std::string(z->vorher != 0 ? "ja" : "nein " + f));
    }));
    add(pruefe("Endstand: .ent im Archiv = Entities im Programm", [=] {
        std::vector<MapEntity> gelesen;
        for (const auto& [n, t] : mb::pk3Inhalt(z->pk3)) {
            if (n.size() > 4 && n.compare(n.size() - 4, 4, ".ent") == 0) { parseEntities(t, gelesen); }
        }
        bool gleich = gelesen.size() == z->gespeichert.size();
        for (std::size_t i = 0; gleich && i < gelesen.size(); ++i) { gleich = gelesen[i].keys == z->gespeichert[i].keys; }
        return z->vorher == 1 && gleich;
    }));
    // Beenden mit Ungespeichertem: Rueckfrage, Nein = zurueck mit Speicherfenster
    add(tu("Noch eine Aenderung, dann beenden", [] {
        neueEntityHier("ref_tag", false);
        g_app->entFrageBeantwortet = false;
        g_app->wantQuit = true;
    }));
    add(pause(0.4));
    add(pruefe("Beenden: Rueckfrage wegen der Entities", [] {
        diag::detail("Knopftest: Missionsbau - Frage: " + g_app->frageText);
        return g_app->frageOffen && !g_app->entitiesUngespeichert.empty() &&
               g_app->frageText.find(*g_app->entitiesUngespeichert.begin()) != std::string::npos;
    }));
    add(klick(tr(Str::AnswerNo), "###frage"));
    add(pause(0.4));
    add(pruefe("Nein: laeuft weiter, Save mission offen", [] { return !g_app->quitConfirmed && g_app->missionSpeichernOffen; }));
    add(klick(tr(Str::MissionCancel), "##missionspeichern"));
    add(pause(0.3));
    return s;
}

std::vector<Schritt> missionReiterSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto archiv = std::make_shared<std::string>();
    add(tu("Mission: ein leerer Reiter, DAHINTER noch ein Skript, Archiv suchen", [=] {
        neuerReiter();
        const int leer = g_app->activeTab;
        // Erst in einen gefuellten Reiter - sonst landete das Skript im
        // leeren (der wird wiederverwendet) und es gaebe keinen in der Mitte.
        activateTab(0);
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1.000 );\n", "mission_hinten.txt");
        activateTab(leer);
        for (const GamePath& gp : g_app->gamePaths) {
            for (const Pk3& arc : gp.archives) {
                if (arc.find("maps/md_am_sith.bsp") != nullptr) { *archiv = arc.path; }
            }
        }
        diag::detail("Knopftest: Mission - aktiv leer: " +
                     std::string(g_app->path.empty() && g_app->doc.script().nodes.empty() ? "ja" : "nein") +
                     ", Archiv \"" + *archiv + "\"");
    }));
    add(pruefe("Mission: Archiv gefunden", [=] { return !archiv->empty(); }));
    add(tu("Mission: Missionen des Archivs", [=] { if (!archiv->empty()) { missionenAusArchiv(*archiv); } }));
    add(pruefe("Mission: Auswahl offen", [] { return g_app->missionPickOpen; }));
    add(waehleInListe("md_am_sith", false, 0, "##missionpick"));
    add(warteBis("Mission geladen", [] {
        return !g_app->map.empty() && g_app->tabs.size() > 2 && g_app->camTrackValid;
    }, 1200));
    add(pause(0.5));
    add(pruefe("Mission: nach dem Laden ist das ERSTE Missionsskript aktiv", [] {
        std::string erwartet = g_app->missionErstesSkript;
        const auto strich = erwartet.find_last_of("/\\");
        if (strich != std::string::npos) { erwartet = erwartet.substr(strich + 1); }
        diag::detail("Knopftest: Mission - erstes Skript " + g_app->missionErstesSkript + ", aktiv " +
                     g_app->shownName + " (Reiter " + std::to_string(g_app->activeTab) + " von " +
                     std::to_string(g_app->tabs.size()) + "), Band " + std::to_string(g_app->homeTab));
        return !erwartet.empty() && g_app->shownName == erwartet && g_app->homeTab == g_app->activeTab;
    }));
    // --- Die Entities der .ent ueberleben einen Kartenwechsel -------------
    //
    // Gefunden beim Bau einer eigenen Mission (11.10.): ein Reiter mit einer
    // ANDEREN Karte, dann zurueck - und loadMapFromArchive las nur die .bsp.
    // Die Entities aus der .ent (alle Figuren der Mission) waren fort.
    auto vorher = std::make_shared<std::pair<int, std::size_t>>(-1, 0);
    auto entMission = std::make_shared<std::pair<std::string, std::string>>();   // Archiv, Name
    add(tu("Mission: eine Mission MIT .ent suchen und laden", [=] {
        for (const GamePath& gp : g_app->gamePaths) {
            for (const Pk3& arc : gp.archives) {
                for (const Pk3Entry& e : arc.entries) {
                    const std::string& n = e.name;
                    // md_*: eine Mission mit Skripten, nicht nur eine Karte mit .ent
                    if (!entMission->first.empty() || n.size() <= 9 || n.rfind("maps/md_", 0) != 0 ||
                        n.compare(n.size() - 4, 4, ".ent") != 0) {
                        continue;
                    }
                    const std::string name = n.substr(5, n.size() - 9);
                    const Pk3Entry* bspE = arc.find("maps/" + name + ".bsp");
                    if (bspE == nullptr) { continue; }
                    // Nur aussagekraeftig, wenn .ent und .bsp verschieden viele haben.
                    std::string entText;
                    std::string bspBytes;
                    MapData bspMap;
                    std::vector<MapEntity> entEnts;
                    if (!readPk3File(arc, e, entText) || !readPk3File(arc, *bspE, bspBytes) ||
                        !readBsp(bspBytes, bspMap)) {
                        continue;
                    }
                    parseEntities(entText, entEnts);
                    diag::detail("Knopftest: Kandidat " + name + ": .ent " + std::to_string(entEnts.size()) +
                                 ", .bsp " + std::to_string(bspMap.entities.size()));
                    if (!entEnts.empty() && entEnts.size() != bspMap.entities.size()) {
                        *entMission = {arc.path, name};
                        vorher->second = entEnts.size();
                    }
                }
            }
        }
        diag::detail("Knopftest: Mission mit .ent: \"" + entMission->second + "\" in \"" + entMission->first + "\"");
        if (!entMission->first.empty()) {
            missionenAusArchiv(entMission->first);
            if (g_app->missionPickOpen) {
                schritteEinfuegen({waehleInListe(entMission->second, false, 0, "##missionpick")});
            }
        }
    }));
    add(pruefe("Mission: eine Mission mit .ent gefunden", [=] { return !entMission->first.empty(); }));
    add(warteBis("Mission mit .ent geladen", [=] {
        return g_app->map.path == "maps/" + entMission->second + ".bsp" && g_app->camTrackValid;
    }, 1200));
    add(pruefe("Mission: geladen mit den Entities der .ent", [=] {
        diag::detail("Knopftest: Mission - geladen " + std::to_string(g_app->map.entities.size()) +
                     " Entities, .ent hat " + std::to_string(vorher->second));
        return g_app->map.entities.size() == vorher->second;
    }));
    add(tu("Mission: Entities merken, Reiter mit einer anderen Karte", [=] {
        vorher->first = g_app->activeTab;
        std::string andere;
        for (const GamePath& gp : g_app->gamePaths) {
            for (const Pk3& arc : gp.archives) {
                for (const Pk3Entry& e : arc.entries) {
                    const std::string& n = e.name;
                    if (andere.empty() && n.size() > 9 && n.rfind("maps/", 0) == 0 &&
                        n.compare(n.size() - 4, 4, ".bsp") == 0 && n != g_app->map.path) {
                        andere = n;
                    }
                }
            }
        }
        diag::detail("Knopftest: Mission - " + std::to_string(vorher->second) + " Entities in Reiter " +
                     std::to_string(vorher->first) + " (" + g_app->map.path + "), andere Karte \"" + andere + "\"");
        neuerReiter();
        if (!andere.empty()) { (void)loadMapFromArchive(andere); }
    }));
    add(pause(0.5));
    add(tu("Mission: zurueck in den Missionsreiter", [=] { activateTab(vorher->first); }));
    add(pause(0.5));
    add(pruefe("Mission: nach dem Kartenwechsel sind die Entities der .ent wieder da", [=] {
        diag::detail("Knopftest: Mission - zurueck: " + std::to_string(g_app->map.entities.size()) +
                     " Entities (vorher " + std::to_string(vorher->second) + "), Karte " + g_app->map.path);
        return vorher->second > 0 && g_app->map.entities.size() == vorher->second;
    }));
    // Zweiter Fall: dieselbe Mission noch einmal laden, waehrend ein anderer
    // Reiter vorn ist - alle Skripte sind schon offen (rc579: nicht doppelt),
    // vorn stehen soll trotzdem das erste.
    add(tu("Mission: in einen anderen Reiter, dieselbe Mission noch einmal", [=] {
        for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
            const std::string& n = (static_cast<int>(i) == g_app->activeTab) ? g_app->shownName
                                                                             : g_app->tabs[i].shownName;
            if (n == "mission_hinten.txt") { activateTab(static_cast<int>(i)); }
        }
        if (!archiv->empty()) { missionenAusArchiv(*archiv); }
    }));
    add(pruefe("Mission: Auswahl wieder offen", [] { return g_app->missionPickOpen; }));
    add(waehleInListe("md_am_sith", false, 0, "##missionpick"));
    add(pause(1.5));
    add(pruefe("Mission: auch beim zweiten Laden steht das erste Skript vorn, nichts doppelt", [] {
        std::string erwartet = g_app->missionErstesSkript;
        const auto strich = erwartet.find_last_of("/\\");
        if (strich != std::string::npos) { erwartet = erwartet.substr(strich + 1); }
        int anzahl = 0;
        for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
            const std::string& n = (static_cast<int>(i) == g_app->activeTab) ? g_app->shownName
                                                                             : g_app->tabs[i].shownName;
            if (n == erwartet) { ++anzahl; }
        }
        diag::detail("Knopftest: Mission - zweites Laden, aktiv " + g_app->shownName + ", " + erwartet + " " +
                     std::to_string(anzahl) + " mal offen, Reiter " + std::to_string(g_app->tabs.size()));
        return g_app->shownName == erwartet && anzahl == 1;
    }));
    return s;
}

// --- Die Feldbelegung eines anderen Reiters nach dem Schliessen -----------
//
// Code-Pruefung 03.10.: jeder Reiter merkt sich seine Aufteilung als
// Reiter-NUMMERN (Parked::splitTabs). Wird links davon einer geschlossen,
// rutschen die Nummern - die gemerkten nicht. Zurueck im Reiter zeigt ein
// Feld dann ein fremdes Skript.
std::vector<Schritt> belegungSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto findeReiter = [](const std::string& name) {
        for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
            const std::string& n = (static_cast<int>(i) == g_app->activeTab) ? g_app->shownName
                                                                             : g_app->tabs[i].shownName;
            if (n == name) { return static_cast<int>(i); }
        }
        return -1;
    };
    const auto nameVon = [](int i) {
        if (i < 0 || i >= static_cast<int>(g_app->tabs.size())) { return std::string("?"); }
        return (i == g_app->activeTab) ? g_app->shownName : g_app->tabs[static_cast<std::size_t>(i)].shownName;
    };
    add(tu("Belegung: vier Skripte b_a, b_x, b_y, b_z", [=] {
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1.000 );\n", "b_a.txt");
        openScriptFromMemory("//Generated by BehavEd\nwait ( 2.000 );\n", "b_x.txt");
        openScriptFromMemory("//Generated by BehavEd\nwait ( 3.000 );\n", "b_y.txt");
        openScriptFromMemory("//Generated by BehavEd\nwait ( 4.000 );\n", "b_z.txt");
        activateTab(findeReiter("b_x.txt"));
    }));
    add(pause(0.3));
    splitMenue(s, 2);
    add(tu("Belegung: zweites Feld zeigt b_y", [=] {
        g_app->splitPanes[1].tab = findeReiter("b_y.txt");
        g_app->splitPanes[1].dirty = true;
    }));
    add(pause(0.3));
    add(tu("Belegung: zu b_z wechseln (wie ein Klick auf seinen Reiter)", [=] {
        activateTab(findeReiter("b_z.txt"));
    }));
    add(pause(0.3));
    add(tu("Belegung: b_a schliessen (liegt links von allen)", [=] { reiterSchliessen(findeReiter("b_a.txt")); }));
    add(pause(0.3));
    add(tu("Belegung: zurueck zu b_x", [=] { activateTab(findeReiter("b_x.txt")); }));
    add(pause(0.4));
    add(pruefe("Belegung: in b_x zeigt das zweite Feld weiter b_y", [=] {
        diag::detail("Knopftest: Belegung - aktiv " + g_app->shownName + ", Felder " +
                     std::to_string(g_app->splitCount) + ", Feld 1 zeigt " + nameVon(g_app->splitPanes[1].tab));
        return g_app->shownName == "b_x.txt" && g_app->splitCount == 2 &&
               nameVon(g_app->splitPanes[1].tab) == "b_y.txt";
    }));
    splitMenue(s, 1);
    add(tu("Belegung: aufraeumen", [=] {
        for (const char* n : {"b_a.txt", "b_x.txt", "b_y.txt", "b_z.txt"}) {
            const int i = findeReiter(n);
            if (i >= 0) {
                g_app->tabs[static_cast<std::size_t>(i)].doc.markSaved();
                if (i == g_app->activeTab) { g_app->doc.markSaved(); }
                reiterSchliessen(i);
            }
        }
    }));
    add(pause(0.3));
    return s;
}

