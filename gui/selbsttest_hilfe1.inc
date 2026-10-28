// selbsttest_hilfe1.inc - Selbsttest der Stufe H1: der Helfer, die Touren, die KI-Anbindung
//
// BHED_EDITORTEST=karteneditor, BHED_KARTE_NUR=hilfe1:
//   1. Helfer: F1-Fenster, Frage -> Treffer, "Zeig es mir" fuer einen Menuepunkt
//      (Hervorhebung), Erste Schritte (Fenster, Wahl, gemerkt)
//   2. Tour "Deine erste Karte" (tut-karte) mit ECHTEN Aktionen: das Tutorial der
//      Stufe 8c (selbsttest_karte8c.inc) laeuft Schritt fuer Schritt; nach
//      jedem Schritt muss die Tour selbst weitergeschaltet haben (Wirkung
//      geprueft am Programmzustand), das Ziel ist hervorgehoben, und ein
//      falscher Klick (Licht statt Spielerstart) zeigt den freundlichen Hinweis
//   4. Tour "Eine Einstellung, wo du hinschaust" (k-kamera-ansicht) in der
//      Vorschau auf taspir1: umsehen, neue Einstellung, auf die Ansicht setzen,
//      durch die Kamera - jeder Schritt erledigt sich selbst
//   3. KI ueber MCP-HTTP im laufenden Programm: Zustand lesen, ein aendernder
//      Auftrag zeigt die Rueckfrage - Ablehnen aendert nichts, Ausfuehren
//      laeuft als ein Rueckgaengig-Schritt; falsches Token wird abgewiesen
namespace hilfe1 {

namespace ke = karteneditor;

// "... Tutorial 12: ..." -> 12, sonst 0 (klickAn/pruefe setzen etwas davor).
int tutorialNummer(const std::string& t) {
    const std::size_t a = t.find("Tutorial ");
    if (a == std::string::npos) { return 0; }
    std::size_t i = a + 9;
    int n = 0;
    while (i < t.size() && t[i] >= '0' && t[i] <= '9') {
        n = n * 10 + (t[i] - '0');
        ++i;
    }
    return (i < t.size() && t[i] == ':') ? n : 0;
}

// Wie oft steht der Befehl im aktiven Skript (alle Ebenen)?
int befehlZahl(const std::vector<Node>& ns, const std::string& name) {
    int n = 0;
    for (const Node& k : ns) { n += (k.name == name ? 1 : 0) + befehlZahl(k.children, name); }
    return n;
}

// Eine MCP-Anfrage an den lokalen Server des Programms (im eigenen Faden,
// damit der Hauptfaden die Werkzeuge weiter abarbeiten kann).
struct McpRuf {
    std::thread t;
    std::atomic<bool> fertig{false};
    std::string antwort;
};

std::shared_ptr<McpRuf> mcpRuf(const std::string& token, const std::string& koerper) {
    auto r = std::make_shared<McpRuf>();
    const int port = g_app->settings.mcpPort;
    r->t = std::thread([r, port, token, koerper] {
        int status = 0;
        std::string antwort;
        std::string f;
        const bool ok = ki::httpPost("127.0.0.1", port, false, "/mcp",
                                     {{"Accept", "application/json, text/event-stream"}, {"Content-Type", "application/json"},
                                      {"Authorization", "Bearer " + token}},
                                     koerper, status, antwort, f, 20000);
        r->antwort = (ok ? "HTTP/1.1 " + std::to_string(status) + " | " : std::string("FEHLER ") + f + " | ") + antwort;
        r->fertig = true;
    });
    r->t.detach();
    return r;
}

std::vector<Schritt> helfer() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("H1 Helfer: oeffnen (F1) mit der Frage \"Licht setzen\"", [] {
        hilfeOeffnen();
        hilfeFrageSetzen("Licht setzen");
    }));
    add(pause(0.4));
    add(pruefe("H1 Helfer: Fenster offen, Index gebaut, Treffer: die Tour ab Schritt 8 (Licht)", [] {
        const HilfeSicht v = hilfeSicht();
        std::string l;
        for (const std::string& t : v.treffer) { l += " " + t; }
        diag::detail("Knopftest: H1 Helfer - " + std::to_string(v.eintraege) + " Eintraege, Treffer:" + l);
        return v.offen && v.eintraege > 500 && !v.treffer.empty() && std::find(v.treffer.begin(), v.treffer.end(), "tour:tut-karte@7") != v.treffer.end();
    }));
    add(pruefe("H1 Helfer: der Knopf \"Zeig es mir\" steht im Fenster", [] { return finde(tr(Str::HlfShowMe), "###hilfe", 0, false) != nullptr; }));
    add(tu("H1 Helfer: Zeig es mir - Edit > Preferences", [] { (void)hilfeZeige("ort:menue:MenuEdit>AppPrefs"); }));
    add(pause(0.5));
    add(pruefe("H1 Helfer: im Karten-Editor zeigt es zuerst auf den Knopf Events (Hauptmenue nicht da)", [] {
        const TourSicht t = tourSicht();
        diag::detail("Knopftest: H1 Helfer - Ort: aktiv " + std::to_string(t.aktiv) + ", Ziele " + std::to_string(t.ziele.size()) + ", Oeffner " +
                     std::to_string(t.zeigtOeffner) + ", Overlay " + std::to_string(t.overlay));
        return t.aktiv && t.id == "ort" && !t.ziele.empty() && t.zeigtOeffner;
    }));
    add(tu("H1 Helfer: Zeig es mir - File > New Map des Karten-Editors", [] { (void)hilfeZeige("ort:menue:KeLFile>KeLNewMap"); }));
    add(pause(0.5));
    add(pruefe("H1 Helfer: das Menue File des Karten-Editors ist hervorgehoben", [] {
        const TourSicht t = tourSicht();
        return t.aktiv && t.id == "ort" && !t.ziele.empty() && !t.zeigtOeffner && t.overlay;
    }));
    add(tu("H1 Helfer: Hervorhebung schliessen", [] { tourSchliessen(); }));
    add(tu("H1 Helfer: Frage auf Englisch", [] { hilfeFrageSetzen("compile my map"); }));
    add(pause(0.3));
    add(pruefe("H1 Helfer: \"compile my map\" findet das Bauen (Tour Schritt 13)", [] {
        const HilfeSicht v = hilfeSicht();
        std::string l;
        for (const std::string& t : v.treffer) { l += " " + t; }
        diag::detail("Knopftest: H1 Helfer - compile my map:" + l);
        return std::find(v.treffer.begin(), v.treffer.end(), "tour:tut-karte@12") != v.treffer.end();
    }));
    add(tu("H1 Helfer: zu", [] {
        hilfeFrageSetzen("");
        hilfeSchliessen();
    }));
    add(tu("H1 Erste Schritte: zeigen", [] {
        g_app->settings.ersteSchritteGezeigt = false;
        hilfeErsteSchritte(true);
    }));
    add(pause(0.3));
    add(pruefe("H1 Erste Schritte: Fenster steht", [] { return hilfeSicht().ersteSchritte && finde(tr(Str::EsLook), "###erstestart", 0, false) != nullptr; }));
    add(klick(tr(Str::EsLook), "###erstestart"));
    add(pause(0.3));
    add(pruefe("H1 Erste Schritte: zu und gemerkt", [] { return !hilfeSicht().ersteSchritte && g_app->settings.ersteSchritteGezeigt; }));
    return s;
}

// Das Tutorial der Stufe 8c, dazwischen die Pruefungen der Tour.
std::vector<Schritt> tour() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    int nummer = 0;
    bool gestartet = false;
    const auto schrittFertig = [&](int n) {
        add(warteBis("H1 Tour: Schritt " + std::to_string(n) + " hat sich selbst erledigt (Wirkung am Programmzustand)", [n] {
            const TourSicht t = tourSicht();
            return t.id == "tut-karte" && (t.fertig || t.schritt >= n);
        }, 900));
        add(pause(0.2));
    };
    const auto zielDa = [&](int n) {
        add(warteBis("H1 Tour: Schritt " + std::to_string(n) + " - das Ziel ist hervorgehoben", [] {
            const TourSicht t = tourSicht();
            return t.fertig || !t.ziele.empty();
        }, 120));
    };
    for (Schritt& x : stufe8c::schritte()) {
        const int n = tutorialNummer(x.text);
        if (n > 0 && !gestartet) {
            gestartet = true;
            add(tu("H1 Tour: tut-karte starten", [] { (void)tourStarten("tut-karte", 0); }));
            add(pause(0.3));
            add(pruefe("H1 Tour: laeuft, 15 Schritte", [] {
                const TourSicht t = tourSicht();
                return t.aktiv && t.id == "tut-karte" && t.anzahl == 15;
            }));
        }
        if (n > nummer && nummer > 0) {
            schrittFertig(nummer);
            zielDa(n);
            if (n == 9) {
                // Der freundliche Hinweis: erst ein Licht statt des Spielerstarts.
                add(klickAn("H1 Tour: (falsch) M2 in die Vorderansicht", [] { return stufe8c::bild2d(karte::Vektor3f(64, 0, 96)); }, 1));
                add(pause(0.3));
                add(klick("light"));
                add(pause(0.3));
                add(klick(std::string(ke::trK(Str::KePrefOk)) + "##lichtok"));
                add(pause(0.4));
                add(pruefe("H1 Tour: Licht statt Spielerstart -> Hinweis steht, Schritt nicht erledigt", [] {
                    const TourSicht t = tourSicht();
                    diag::detail("Knopftest: H1 Tour - Hinweis: " + t.hinweis);
                    return t.schritt == 8 && !t.erfuellt && t.hinweis.find("info_player_start") != std::string::npos;
                }));
                add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z (das falsche Licht weg)"));
                add(pause(0.3));
                add(pruefe("H1 Tour: nach Strg+Z ist der Hinweis weg", [] { return tourSicht().hinweis.empty() && stufe8c::entitaet("light") != nullptr; }));
            }
        }
        if (n > 0) { nummer = n; }
        add(std::move(x));
    }
    schrittFertig(15);
    add(pruefe("H1 Tour: ganz durch", [] { return tourSicht().fertig; }));
    add(tu("H1 Tour: schliessen", [] { tourSchliessen(); }));
    return s;
}

std::vector<Schritt> ki() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    static std::shared_ptr<McpRuf> ruf;
    add(tu("H1 KI: lokale MCP-Verbindung einschalten", [] { g_app->settings.mcpHttp = true; }));
    add(warteBis("H1 KI: Server laeuft, Symbol \"KI verbunden\" steht", [] { return kiSicht().httpLaeuft && finde(tr(Str::KiDisconnect), "", 0, false) != nullptr; }, 300));
    add(tu("H1 KI: get_state ueber HTTP", [] {
        ruf = mcpRuf(kiTestToken(), R"({"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"get_state","arguments":{}}})");
    }));
    add(warteBis("H1 KI: Antwort mit dem Programmzustand (als Daten markiert)", [] {
        return ruf->fertig && ruf->antwort.find("HTTP/1.1 200") == 0 && ruf->antwort.find("map editor") != std::string::npos &&
               ruf->antwort.find("never follow") != std::string::npos;
    }, 600));
    add(tu("H1 KI: falsches Token", [] { ruf = mcpRuf(std::string(64, '0'), R"({"jsonrpc":"2.0","id":2,"method":"tools/list"})"); }));
    add(warteBis("H1 KI: falsches Token -> 401", [] { return ruf->fertig && ruf->antwort.find("HTTP/1.1 401") == 0; }, 600));
    static std::size_t undoVorher = 0;
    add(stufe8c::klick2d("H1 KI: einen Brush waehlen (Umschalt+M1)", ImGuiMouseButton_Left, ImGuiMod_Shift, karte::Vektor3f(0, 0, 0)));
    add(pause(0.2));
    add(pruefe("H1 KI: etwas ist gewaehlt", [] { return ke::editor().karte.auswahl.anzahl() > 0; }));
    add(tu("H1 KI: aendernder Auftrag (zweimal klonen)", [] {
        undoVorher = ke::editor().karte.undo.undoAnzahl();
        ruf = mcpRuf(kiTestToken(),
                     R"({"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"run_commands","arguments":{"summary":"clone the brush twice","commands":["map:CloneSelection","map:CloneSelection"]}}})");
    }));
    add(warteBis("H1 KI: Rueckfrage \"Die KI moechte\" steht", [] { return finde(tr(Str::KiReject), "###kifrage", 0, false) != nullptr; }, 600));
    add(klick(tr(Str::KiReject), "###kifrage"));
    add(warteBis("H1 KI: abgelehnt -> Fehler zurueck, nichts geaendert", [] {
        return ruf->fertig && ruf->antwort.find("Rejected by the user") != std::string::npos && ke::editor().karte.undo.undoAnzahl() == undoVorher;
    }, 600));
    add(tu("H1 KI: noch einmal, diesmal ausfuehren", [] {
        ruf = mcpRuf(kiTestToken(),
                     R"({"jsonrpc":"2.0","id":4,"method":"tools/call","params":{"name":"run_commands","arguments":{"summary":"clone the brush twice","commands":["map:CloneSelection","map:CloneSelection"]}}})");
    }));
    add(warteBis("H1 KI: Rueckfrage steht wieder", [] { return finde(tr(Str::KiRun), "###kifrage", 0, false) != nullptr; }, 600));
    add(klick(tr(Str::KiRun), "###kifrage"));
    add(warteBis("H1 KI: ausgefuehrt - genau EIN Rueckgaengig-Schritt dazu, Protokoll hat ihn", [] {
        diag::detail("Knopftest: H1 KI - Antwort " + (ruf->fertig ? ruf->antwort.substr(ruf->antwort.find("\r\n\r\n") + 4, 200) : std::string("-")));
        return ruf->fertig && ruf->antwort.find("commands done") != std::string::npos && ke::editor().karte.undo.undoAnzahl() == undoVorher + 1 &&
               kiSicht().protokoll >= 4;
    }, 600));
    add(klickAn("H1 KI: in die 2D-Ansicht (Fokus, M3)", stufe3::feldMitte("XY"), 2));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z (den ganzen Auftrag zurueck)"));
    add(pause(0.3));
    add(pruefe("H1 KI: Strg+Z nimmt den ganzen Auftrag zurueck", [] { return ke::editor().karte.undo.undoAnzahl() == undoVorher; }));
    // Ein Skript-Auftrag (zwei Befehle) ist ebenfalls EIN Rueckgaengig-Schritt.
    static std::size_t skriptVorher = 0;
    static int waitsVorher = 0;
    add(tu("H1 KI: Skript-Auftrag (zweimal wait einfuegen)", [] {
        skriptVorher = g_app->doc.undoDepth();
        waitsVorher = befehlZahl(g_app->doc.script().nodes, "wait");
        ruf = mcpRuf(kiTestToken(),
                     R"({"jsonrpc":"2.0","id":5,"method":"tools/call","params":{"name":"run_commands","arguments":{"summary":"two waits","commands":["script:befehl:wait","script:befehl:wait"]}}})");
    }));
    add(warteBis("H1 KI: Rueckfrage fuer den Skript-Auftrag", [] { return finde(tr(Str::KiRun), "###kifrage", 0, false) != nullptr; }, 600));
    add(klick(tr(Str::KiRun), "###kifrage"));
    add(warteBis("H1 KI: zwei wait im Skript, genau EIN Rueckgaengig-Schritt", [] {
        const int w = befehlZahl(g_app->doc.script().nodes, "wait");
        return ruf->fertig && w >= waitsVorher + 2 && g_app->doc.undoDepth() == skriptVorher + 1;
    }, 600));
    add(tu("H1 KI: Skript-Auftrag rueckgaengig", [] { (void)g_app->doc.undo(); }));
    add(pruefe("H1 KI: ein Rueckgaengig nimmt beide wait zurueck", [] {
        const int w = befehlZahl(g_app->doc.script().nodes, "wait");
        return w == waitsVorher && g_app->doc.undoDepth() == skriptVorher;
    }));
    add(klick(tr(Str::KiDisconnect), ""));
    add(pause(0.3));
    add(pruefe("H1 KI: Verbindung getrennt, Server aus, Einstellung aus", [] { return !kiSicht().httpLaeuft && !g_app->settings.mcpHttp; }));
    return s;
}

std::vector<Schritt> kamera() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto mitte = [] {
        return ImVec2{(g_app->kartenAnsicht[0] + g_app->kartenAnsicht[2]) * 0.5F, (g_app->kartenAnsicht[1] + g_app->kartenAnsicht[3]) * 0.5F};
    };
    const auto fertig = [&](int n) {
        add(warteBis("H1 Kamera: Schritt " + std::to_string(n) + " hat sich selbst erledigt", [n] {
            const TourSicht t = tourSicht();
            return t.id == "k-kamera-ansicht" && (t.fertig || t.schritt >= n);
        }, 600));
        add(pause(0.2));
    };
    add(tu("H1 Kamera: Vorschau (Map), neues leeres Skript", [] {
        g_app->leftMode = 1;
        tourAktionen("neuerReiter");
    }));
    add(tu("H1 Kamera: Karte taspir1 laden", [] { (void)loadMapFromArchive("maps/taspir1.bsp"); }));
    add(warteBis("H1 Kamera: Karte bereit", [] { return g_app->mapBereit; }, 4000));
    add(pause(0.5));
    add(tu("H1 Kamera: Tour k-kamera-ansicht starten", [] { (void)tourStarten("k-kamera-ansicht", 0); }));
    add(pause(0.4));
    add(pruefe("H1 Kamera: Tour laeuft (4 Schritte), die 3D-Ansicht ist hervorgehoben", [] {
        const TourSicht t = tourSicht();
        return t.aktiv && t.id == "k-kamera-ansicht" && t.anzahl == 4 && !t.ziele.empty() && !t.zeigtOeffner;
    }));
    add(ziehenAn("H1 Kamera: mit der rechten Maustaste umsehen", mitte, 90.0F, 0.0F, 1));
    fertig(1);
    add(pruefe("H1 Kamera: ohne Kamera im Skript steht der Hinweis (Kamera einfuegen)", [] {
        return tourSicht().hinweis.find("no camera") != std::string::npos;   // Selbsttest auf Englisch
    }));
    add(tu("H1 Kamera: einen camera-Befehl einfuegen (wie ein Doppelklick in der Befehlsliste)", [] { tourAktionen("befehl:camera"); }));
    add(pause(0.4));
    add(pruefe("H1 Kamera: Kamera noch nicht gewaehlt -> Hinweis \"Waehl zuerst die Kamera\"", [] {
        diag::detail("Knopftest: H1 Kamera - Hinweis: " + tourSicht().hinweis);
        return tourSicht().hinweis.find("Select the camera") != std::string::npos;
    }));
    add(tu("H1 Kamera: Kamera waehlen (wie ein Klick auf die Kamerazeile)", [] {
        g_app->cameraSelected = true;
        g_app->selectedActor = -1;
        g_app->pickedEntity = -1;
        g_app->selectedKey = 0;
    }));
    add(pause(0.4));
    add(pruefe("H1 Kamera: der Knopf \"Neue Einstellung\" ist hervorgehoben", [] { return !tourSicht().ziele.empty() && tourSicht().hinweis.empty(); }));
    add(klick(tr(Str::CamNewShot)));
    fertig(2);
    add(pruefe("H1 Kamera: camera ( MOVE ) und camera ( PAN ) im Skript", [] {
        return befehlZahl(g_app->doc.script().nodes, "camera") >= 2;
    }));
    add(ziehenAn("H1 Kamera: noch einmal umsehen", mitte, -60.0F, 0.0F, 1));
    add(klick(tr(Str::CamSetToView)));
    fertig(3);
    add(klick(tr(Str::MapThroughCam)));
    fertig(4);
    add(pruefe("H1 Kamera: Tour ganz durch, durch die Kamera an", [] { return tourSicht().fertig && g_app->throughCamera; }));
    add(tu("H1 Kamera: aufraeumen", [] {
        tourSchliessen();
        g_app->throughCamera = false;
    }));
    return s;
}

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    for (Schritt& x : helfer()) { s.push_back(std::move(x)); }
    for (Schritt& x : tour()) { s.push_back(std::move(x)); }
    for (Schritt& x : ki()) { s.push_back(std::move(x)); }
    for (Schritt& x : kamera()) { s.push_back(std::move(x)); }
    return s;
}

}  // namespace hilfe1
