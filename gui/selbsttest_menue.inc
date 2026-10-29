// selbsttest_menue.inc - Menue-Editor M1: jede Funktion mit Wirkungspruefung.
//
// Modus BHED_EDITORTEST=menueeditor. Ohne Spielordner laeuft alles mit einem
// eingebauten SP- und MP-Menue; mit Spielordnern zusaetzlich ein Original
// (ui/main.menu bzw. ui/jamp/main.menu) mit Schriften und Bildern.
// Schreibt nur in den Einstellungsordner des Laufs, nie in Spielordner, und
// startet kein Spiel. Fotos (BHED_FOTOS) nur vom eigenen Fenster.
namespace menuetest {
namespace me = menueeditor;

const char* const kSp =
    "// Probe fuer den Selbsttest\n"
    "{\n"
    "\tmenuDef\n"
    "\t{\n"
    "\t\tname\t\t\"probe\"\n"
    "\t\tfullScreen\t1\n"
    "\t\trect\t\t0 0 640 480\n"
    "\t\tvisible\t\t1\n"
    "\t\tfocusColor\t1 1 0 1\n"
    "\t\tonOpen\t\t{ setcvar ui_tab 1 }\n"
    "\t\titemDef\n"
    "\t\t{\n"
    "\t\t\tname\t\ttitel\n"
    "\t\t\ttype\t\tITEM_TYPE_TEXT\n"
    "\t\t\trect\t\t100 40 200 30\t// Titel\n"
    "\t\t\ttext\t\t\"Probe\"\n"
    "\t\t\tforecolor\t1 1 1 1\n"
    "\t\t\tvisible\t\t1\n"
    "\t\t\tdecoration\n"
    "\t\t}\n"
    "\t\titemDef\n"
    "\t\t{\n"
    "\t\t\tname\t\tjanein\n"
    "\t\t\ttype\t\tITEM_TYPE_YESNO\n"
    "\t\t\trect\t\t100 100 200 30\n"
    "\t\t\ttext\t\t\"Option:\"\n"
    "\t\t\tcvar\t\t\"ui_probe\"\n"
    "\t\t\tvisible\t\t1\n"
    "\t\t}\n"
    "\t\titemDef\n"
    "\t\t{\n"
    "\t\t\tname\t\tseite2\n"
    "\t\t\tcvarTest\tui_tab\n"
    "\t\t\tshowCvar\t{ \"2\" }\n"
    "\t\t\tstyle\t\tWINDOW_STYLE_FILLED\n"
    "\t\t\tbackcolor\t0 0 1 1\n"
    "\t\t\trect\t\t400 100 100 100\n"
    "\t\t\tvisible\t\t1\n"
    "\t\t}\n"
    "\t\titemDef\n"
    "\t\t{\n"
    "\t\t\tname\t\tseite1\n"
    "\t\t\tcvarTest\tui_tab\n"
    "\t\t\tshowCvar\t{ \"1\" }\n"
    "\t\t\tstyle\t\tWINDOW_STYLE_FILLED\n"
    "\t\t\tbackcolor\t1 0 0 1\n"
    "\t\t\trect\t\t400 220 100 100\n"
    "\t\t\tvisible\t\t1\n"
    "\t\t\tfoo\t\t1\n"
    "\t\t}\n"
    "\t}\n"
    "}\n";

// --- M2: Videos, Modelle, Bibliothek, Menuesatz, Klaenge, Texte, Pruefungen, Test im Spiel, Schriften ---
std::vector<Schritt> schritteM2() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = [] { return me::stand(); };
    const std::string einst = platform::settingsDirectory();
    // Punkt 2: ein RoQ aus dem eigenen Kodierer als cinematic abspielen
    add(tu("M2 Video: RoQ kodieren, ins Paket, Menue mit cinematic oeffnen", [] {
        (void)me::oeffneText("ui/kino.menu",
                             "{\n\tmenuDef\n\t{\n\t\tname kino\n\t\trect 0 0 640 480\n\t\tvisible 1\n"
                             "\t\titemDef\n\t\t{\n\t\t\tname film\n\t\t\trect 100 80 320 240\n\t\t\tstyle WINDOW_STYLE_CINEMATIC\n"
                             "\t\t\tcinematic \"probe\"\n\t\t\tvisible 1\n\t\t}\n\t}\n}\n");
        bhed::menue::RoqKodierer k;
        bhed::menue::RoqOptionen o;
        o.fps = 10;
        (void)k.beginne(64, 48, o);
        for (int t = 0; t < 8; ++t) {
            bhed::menue::RoqBild b;
            b.breite = 64;
            b.hoehe = 48;
            b.rgba.assign(64 * 48 * 4, 255);
            for (int i = 0; i < 64 * 48; ++i) {
                b.rgba[static_cast<std::size_t>(i * 4)] = static_cast<std::uint8_t>((i % 64) * 4 + t * 20);
                b.rgba[static_cast<std::size_t>(i * 4 + 1)] = static_cast<std::uint8_t>((i / 64) * 5);
            }
            k.bild(b);
        }
        me::legeInsPaket("video/probe.roq", k.fertig());
    }));
    add(pause(1.2));
    add(foto("menue_video"));
    add(pruefe("M2 Video: cinematic spielt (RoQ-Bilder kommen, cl_cin.cpp-Port)", [st] {
        const auto x = st();
        diag::info("Menue-Editor Video: " + std::to_string(x.kinoBilder) + " RoQ-Bilder gezeigt");
        return x.kinoBilder >= 3;
    }));
    // GIF -> RoQ (Import, Guete einstellbar)
    add(pruefe("M2 Video: GIF importieren -> video/<name>.roq im Paket", [einst] {
        static const unsigned char gif[] = {'G', 'I', 'F', '8', '9', 'a', 1, 0, 1, 0, 0x81, 0, 0, 0xFF, 0, 0, 0, 0xFF, 0, 0, 0, 0xFF, 0, 0, 0,
                                            0x21, 0xF9, 4, 0, 5, 0, 0, 0, 0x2C, 0, 0, 0, 0, 1, 0, 1, 0, 0, 2, 2, 0x44, 0x01, 0,
                                            0x21, 0xF9, 4, 0, 10, 0, 0, 0, 0x2C, 0, 0, 0, 0, 1, 0, 1, 0, 0, 2, 2, 0x4C, 0x01, 0, 0x3B};
        const std::string pfad = (std::filesystem::path(einst) / "probegif.gif").string();
        {
            std::ofstream f(pfad, std::ios::binary);
            f.write(reinterpret_cast<const char*>(gif), sizeof(gif));
        }
        me::waehle(0, false);
        const std::string r = me::importiereVideo(pfad, 60, 10, 64);
        const auto x = me::stand();
        bool imPaket = false;
        for (const std::string& p : x.paket) { imPaket = imPaket || p == "video/probegif.roq"; }
        return r == "video/probegif.roq" && imPaket && x.text.find("cinematic") != std::string::npos;
    }));
    // Punkt 1: 3D-Modell (nur mit Spielordnern)
    add(tu("M2 Modell: ITEM_TYPE_MODEL oeffnen", [] {
        (void)me::oeffneText("ui/modell.menu",
                             "{\n\tmenuDef\n\t{\n\t\tname modell\n\t\trect 0 0 640 480\n\t\tvisible 1\n"
                             "\t\titemDef\n\t\t{\n\t\t\tname character\n\t\t\ttype ITEM_TYPE_MODEL\n\t\t\trect 220 60 200 360\n"
                             "\t\t\tasset_model \"models/players/jedi_hf/model.glm\"\n\t\t\tmodel_rotation 20\n\t\t\tvisible 1\n\t\t}\n\t}\n}\n");
    }));
    add(pause(0.8));
    add(foto("menue_modell"));
    add(pruefe("M2 Modell: Figur gezeichnet (ohne Spielordner uebersprungen)", [st] {
        if (g_app->gamePaths.empty()) { return true; }
        const auto x = st();
        diag::info("Menue-Editor Modell: " + std::to_string(x.modellDreiecke) + " Dreiecke");
        return x.modellDreiecke > 100;
    }));
    // Punkt 4: Bibliothek - Detailbereich per Vorlage, eigene Vorlage speichern
    add(tu("M2 Bibliothek: Probe oeffnen, Detailbereich einsetzen", [] {
        (void)me::oeffneText("ui/bibliothek.menu", "{\n\tmenuDef\n\t{\n\t\tname bib\n\t\trect 0 0 640 480\n\t\tvisible 1\n\t}\n}\n");
        (void)me::fuegeVorlageEin("detail", 20, 40);
    }));
    add(pause(0.3));
    add(foto("menue_detail"));
    add(pruefe("M2 Bibliothek: Detailbereich (76 Items, onOpen setzt ui_tab/ui_sel/ui_tabofs)", [st] {
        const auto x = st();
        return x.items == 76 && x.text.find("onOpen") != std::string::npos && x.text.find("ui_tabofs") != std::string::npos;
    }));
    add(tu("M2 Bibliothek: Bedienen, Klick auf Unterreiter 2", [] {
        me::setzeInteraktion(true);
    }));
    add(pause(0.3));
    add(tu("M2 Bibliothek: Klick", [] { me::klickVorschau(30, 140); }));
    add(pruefe("M2 Bibliothek: Detailbereich bedienbar (ui_sel 0_1)", [] { return me::cvarWert("ui_sel") == "0_1"; }));
    add(tu("M2 Bibliothek: Bedienen aus", [] { me::setzeInteraktion(false); }));
    add(pruefe("M2 Bibliothek: Auswahl als eigene Vorlage speichern", [] {
        me::waehle(0, false);
        if (!me::speichereAuswahlAlsVorlage("probe_vorlage")) { return false; }
        const auto ids = me::vorlagenIds();
        return std::find(ids.begin(), ids.end(), "probe_vorlage") != ids.end();
    }));
    // Punkt 3: Menuesatz ueber zwei Dateien - "open" wechselt die Datei
    add(tu("M2 Menuesatz: zwei Dateien und menus.txt oeffnen", [] {
        (void)me::oeffneText("ui/probe_b.menu", "{ menuDef { name zweites rect 0 0 640 480 visible 0\n"
                                                "  itemDef { name t text \"B\" rect 10 10 100 20 visible 1 } } }\n");
        (void)me::oeffneText("ui/probe_a.menu", "{ menuDef { name erstes rect 0 0 640 480 visible 1\n"
                                                "  itemDef { name k type ITEM_TYPE_BUTTON rect 100 100 200 40 style WINDOW_STYLE_FILLED backcolor 0 0 1 1\n"
                                                "    visible 1 action { open zweites } } } }\n");
        (void)me::oeffneText("ui/probesatz.txt", "{\n loadMenu { \"ui/probe_a.menu\" }\n loadMenu { \"ui/probe_b.menu\" }\n}\n");
        (void)me::oeffneMenuesatz("ui/probesatz.txt");
        me::setzeInteraktion(true);
    }));
    add(pause(0.3));
    add(tu("M2 Menuesatz: Klick auf den Knopf (open zweites)", [] { me::klickVorschau(150, 120); }));
    add(pause(0.3));
    add(pruefe("M2 Menuesatz: open wechselt in die andere Datei des Satzes", [] {
        diag::info("Menue-Editor Menuesatz: " + me::laufInfo());
        return me::letzterSatzWechsel() == "zweites";
    }));
    add(tu("M2 Menuesatz: Bedienen aus", [] { me::setzeInteraktion(false); }));
    return s;
}

// Bildvergleich mit dem Spiel (BHED_MENUE_VERGLEICH=1): JKA-Hauptmenue SP und MP
// in 4:3 fotografieren und die Lage der Flaeche melden (Zuschnitt in
// tools/menue_bildvergleich.py gegen die Fotos aus dem Spiel).
std::vector<Schritt> vergleichSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(pause(1.0));
    add(klick(tr(Str::ViewMenuEditor), "###main"));
    add(tu("Vergleich: 4:3 mit Balken", [] { me::setzeBalken(true); }));
    const char* const menues[][2] = {{"ui/main.menu", "vergleich_sp"},
                                     {"ui/jamp/main.menu", "vergleich_mp"},
                                     {"ui/character.menu", "vergleich_figur"},
                                     {"ui/saber.menu", "vergleich_schwert"}};
    for (const auto& m : menues) {
        const std::string pfad = m[0], name = m[1];
        // im Bedienmodus: onOpen laeuft (setfocus), Zeit und Videos wie im Spiel
        add(tu("Vergleich: " + pfad, [pfad] {
            me::setzeInteraktion(false);
            (void)me::oeffneAusSpiel(pfad);
            me::setzeInteraktion(true);
        }));
        add(pause(2.5));
        add(foto(name));
        add(pruefe("Vergleich: " + pfad + " gezeichnet", [name] {
            const auto x = me::stand();
            char b[300];
            std::snprintf(b, sizeof(b), "Menue-Editor Vergleich %s: Flaeche %.1f %.1f %.1f %.1f, %d Zeichenbefehle, %d Texturen, Bildzeit %.2f ms",
                          name.c_str(), static_cast<double>(x.flaeche[0]), static_cast<double>(x.flaeche[1]), static_cast<double>(x.flaeche[2]),
                          static_cast<double>(x.flaeche[3]), x.prims, x.bilderGeladen, x.bildzeitMs);
            diag::info(b);
            return x.prims > 0;
        }));
    }
    add(tu("Vergleich: zurueck", [] {
        me::setzeInteraktion(false);
        g_app->leftMode = 0;
    }));
    return s;
}

std::vector<Schritt> schritte() {
    if (std::getenv("BHED_MENUE_VERGLEICH") != nullptr) { return vergleichSchritte(); }
    // Sprache fuer die Handbuchbilder (wie handbuchSchritte): BHED_SPRACHE = en / de / zh / ja
    if (const char* sp = std::getenv("BHED_SPRACHE")) {
        if (const i18n::LanguageInfo* l = i18n::findLanguage(sp)) { i18n::setLanguage(l->language); }
    }
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = [] { return me::stand(); };
    auto orig = std::make_shared<std::string>(kSp);
    add(pause(1.0));   // erst das Fenster in Ruhe (ja: der Knopf lag beim ersten Bild noch halb draussen)
    add(klick(tr(Str::ViewMenuEditor), "###main"));
    add(pause(0.2));
    add(pruefe("Menue-Editor: Modus 4 aktiv", [] { return g_app->leftMode == 4 && me::aktiv(); }));
    add(tu("Menue-Editor: Probe oeffnen", [] { (void)me::oeffneText("ui/probe.menu", kSp); }));
    add(pause(0.3));
    add(pruefe("Lesen: 1 Menue, 4 Items, Text byte-gleich", [st] {
        const auto x = st();
        return x.menus == 1 && x.items == 4 && x.text == kSp;
    }));
    add(pruefe("Lesen: Meldung wie die Engine (Unknown item keyword 'foo')", [st] { return st().meldungen >= 1; }));
    add(pruefe("Vorschau: Zeichenbefehle", [st] { return st().prims > 0; }));
    add(foto("menue_uebersicht"));
    // Flaeche -> Code
    add(tu("Auswahl: Item 1 (janein)", [] { me::waehle(1, false); }));
    add(pause(0.2));
    add(pruefe("Auswahl springt zum itemDef im Code", [st] {
        const auto x = st();
        return x.auswahl.size() == 1 && x.auswahl[0] == 1 && x.cursorZeile.find("itemDef") != std::string::npos;
    }));
    // Verschieben aendert nur die Zahlen von rect
    add(tu("Verschieben: +12 x", [] { me::verschiebeAuswahl(12, 0); }));
    add(pruefe("Verschieben: rect 112 100 200 30, sonst nichts geaendert", [st, orig] {
        const auto x = st();
        std::string erwartet = *orig;
        const std::size_t p = erwartet.find("rect\t\t100 100 200 30");
        if (p == std::string::npos) { return false; }
        erwartet.replace(p, 20, "rect\t\t112 100 200 30");
        return x.text == erwartet;
    }));
    add(tu("Rueckgaengig", [] { (void)me::rueckgaengig(); }));
    add(pruefe("Rueckgaengig: wieder wie geladen", [st] { return st().text == kSp; }));
    add(tu("Wiederholen", [] { (void)me::wiederholen(); }));
    add(pruefe("Wiederholen: 112", [st] { return st().text.find("rect\t\t112 100 200 30") != std::string::npos; }));
    // Echte Eingabe in den Code-Editor: Klick in die Flaeche, Zeichen, Strg+Z
    add(klick("##codeflaeche"));
    add({"Code: XYZ tippen", [](int b) {
             if (b == 1) { ImGui::GetIO().AddInputCharactersUTF8("XYZ"); }
             return b >= 4;
         }});
    add(pruefe("Code: getippt (echte Tastatureingabe)", [st] { return st().text.find("XYZ") != std::string::npos; }));
    add(pruefe("Code: Festbreitenschrift (Consolas/Courier New, i und W gleich breit)", [st] { return st().codeFestbreite; }));
    add({"Code: Strg+Z", [](int b) {
             ImGuiIO& io = ImGui::GetIO();
             if (b == 1) {
                 io.AddKeyEvent(ImGuiMod_Ctrl, true);
                 io.AddKeyEvent(ImGuiKey_Z, true);
             }
             if (b == 2) {
                 io.AddKeyEvent(ImGuiKey_Z, false);
                 io.AddKeyEvent(ImGuiMod_Ctrl, false);
             }
             return b >= 5;
         }});
    add(pruefe("Code: Strg+Z nimmt das Tippen zurueck", [st] {
        return st().text.find("XYZ") == std::string::npos && st().text.find("rect\t\t112 100 200 30") != std::string::npos;
    }));
    // Palette
    add(tu("Palette: Knopf einfuegen", [] { me::fuegeEin(bhed::menue::ITEM_TYPE_BUTTON); }));
    add(pause(0.2));
    add(pruefe("Palette: 5 Items, ITEM_TYPE_BUTTON im Text, neues Item gewaehlt", [st] {
        const auto x = st();
        return x.items == 5 && x.text.find("ITEM_TYPE_BUTTON") != std::string::npos && x.auswahl.size() == 1 && x.auswahl[0] == 4;
    }));
    add(foto("menue_palette"));
    add(tu("Duplizieren (Strg+D)", [] { me::dupliziereAuswahl(); }));
    add(pruefe("Duplizieren: 6 Items, button_3 neu und gewaehlt", [st] {
        const auto x = st();
        return x.items == 6 && x.text.find("button_3") != std::string::npos && x.auswahl.size() == 1 && x.auswahl[0] == 5;
    }));
    add(tu("Loeschen (Entf)", [] { me::loescheAuswahl(); }));
    add(pruefe("Loeschen: wieder 5 Items, button_3 weg", [st] {
        const auto x = st();
        return x.items == 5 && x.text.find("button_3") == std::string::npos;
    }));
    // Suchen und Ersetzen
    add(pruefe("Suchen: rect 6x", [] { return me::suche("rect", false) == 6; }));
    add(pruefe("Suchen Regex: ui_\\w+ 4x", [] { return me::suche("ui_\\w+", true) == 4; }));
    auto ersetzt = std::make_shared<int>(-1);
    add(tu("Ersetzen Regex: Probe -> Test", [ersetzt] { *ersetzt = me::ersetzeAlle("\"Probe\"", "\"Test\"", true); }));
    add(pruefe("Ersetzen Regex (Gross/Klein egal): name \"probe\" und text \"Probe\" ersetzt", [st, ersetzt] {
        return *ersetzt == 2 && st().text.find("\"Test\"") != std::string::npos && st().text.find("\"Probe\"") == std::string::npos;
    }));
    add(foto("menue_suchen"));
    // Seiten (ui_tab) und Cvars
    add(pruefe("Seiten: menuDef + zwei ui_tab-Seiten", [st] { return st().seiten == 3; }));
    add(tu("Cvar ui_tab = 2 (Vorschau)", [] { me::setzeCvar("ui_tab", "2"); }));
    add(pause(0.2));
    add(pruefe("Cvar ui_tab = 2 gesetzt (nur Vorschau)", [] { return me::cvarWert("ui_tab") == "2"; }));
    // Bedienen
    add(tu("Bedienen an", [] { me::setzeInteraktion(true); }));
    add(pause(0.3));
    add(pruefe("Bedienen: onOpen setzt ui_tab 1", [] { return me::cvarWert("ui_tab") == "1"; }));
    add(tu("Bedienen: Klick auf Ja/Nein", [] { me::klickVorschau(150, 115); }));
    add(pruefe("Bedienen: ui_probe = 1", [] { return me::cvarWert("ui_probe") == "1"; }));
    add(foto("menue_bedienen"));
    add(tu("Bedienen aus", [] { me::setzeInteraktion(false); }));
    // MP mit Makros
    add(tu("MP-Menue mit #define oeffnen", [] {
        (void)me::oeffneText("ui/jamp/probe_mp.menu",
                             "#define LINKS 30\n{ menuDef { name mp rect 0 0 640 480 visible 1\n"
                             "  itemDef { name a rect LINKS 40 100 20 style 1 backcolor 0 1 0 1 visible 1 } } }\n");
    }));
    add(pause(0.2));
    add(pruefe("MP: Makro aufgeloest, ein Item", [st] { return st().menus == 1 && st().items == 1; }));
    add(tu("MP: Item waehlen und verschieben (rect aus Makro)", [] {
        me::waehle(0, false);
        me::verschiebeAuswahl(5, 0);
    }));
    add(pruefe("MP: rect aus einem Makro bleibt unberuehrt", [st] { return st().text.find("rect LINKS 40 100 20") != std::string::npos; }));
    // Original aus dem Spiel (nur mit Spielordnern)
    add(tu("Original aus dem Spiel oeffnen (wenn vorhanden)", [] {
        if (!me::oeffneAusSpiel("ui/jamp/main.menu")) { (void)me::oeffneAusSpiel("ui/main.menu"); }
    }));
    add(pause(1.0));
    add(foto("menue_original"));
    add(pruefe("Original: gelesen und gezeichnet (ohne Spielordner uebersprungen)", [st] {
        if (g_app->gamePaths.empty()) { return true; }
        const auto x = st();
        diag::info("Menue-Editor Original: " + std::to_string(x.menus) + " Menues, " + std::to_string(x.items) + " Items, " +
                   std::to_string(x.prims) + " Zeichenbefehle, " + std::to_string(x.bilderGeladen) + " Texturen, " +
                   std::to_string(x.meldungen) + " Meldungen");
        return x.menus > 0 && x.prims > 0 && x.bilderGeladen > 0;
    }));
    // Export nie in Spielordner, sonst in den Einstellungsordner
    add(pruefe("Export .pk3 in den Einstellungsordner", [] {
        const std::string ziel = (std::filesystem::path(platform::settingsDirectory()) / "menue_probe.pk3").string();
        const bool ok = me::exportierePk3(ziel);
        std::error_code ec;
        const bool da = std::filesystem::exists(ziel, ec);
        std::filesystem::remove(ziel, ec);
        return ok && da;
    }));
    add(pruefe("Export in einen Spielordner wird abgelehnt", [] {
        if (g_app->settings.gamePaths.empty()) { return true; }
        return !me::exportierePk3((std::filesystem::path(g_app->settings.gamePaths.front()) / "zz_menue_probe.pk3").string());
    }));
    for (auto& x : schritteM2()) { add(std::move(x)); }
    add(tu("Zurueck zum Skript-Modus", [] { g_app->leftMode = 0; }));
    return s;
}

}  // namespace menuetest
