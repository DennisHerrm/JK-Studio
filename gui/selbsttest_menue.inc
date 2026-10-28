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

std::vector<Schritt> schritte() {
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
    add(tu("Zurueck zum Skript-Modus", [] { g_app->leftMode = 0; }));
    return s;
}

}  // namespace menuetest
