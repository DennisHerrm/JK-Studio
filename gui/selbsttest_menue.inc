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
        me::VideoImport o;   // klein und schnell; Verwendung cinematic wie in M2
        o.breite = o.hoehe = 64;
        o.guete = 60;
        o.fps = 10;
        o.verwendung = 2;
        const std::string r = me::importiereVideo(pfad, o);
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
        diag::info("Menue-Editor Modell: " + std::to_string(x.modellAufrufe) + " Zeichenaufrufe");
        return x.modellAufrufe > 0;
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
    // Ebenenliste: drei uebereinanderliegende Items (eigene Namen, eine Gruppe), Kommentar dazwischen
    const std::string ebenen = "{\n\tmenuDef\n\t{\n\t\tname stapel\n\t\trect 0 0 640 480\n\t\tvisible 1\n"
                               "\t\titemDef { name unten style WINDOW_STYLE_FILLED backcolor 1 0 0 1 rect 50 50 200 200 visible 1 }\n"
                               "\t\t// zwischen den Bloecken\n"
                               "\t\titemDef { name mitte group paar style WINDOW_STYLE_FILLED backcolor 0 1 0 1 rect 80 80 200 200 visible 1 }\n"
                               "\t\titemDef { name oben group paar style WINDOW_STYLE_FILLED backcolor 0 0 1 1 rect 110 110 200 200 visible 1 }\n"
                               "\t}\n}\n";
    add(tu("Ebenen: Stapel oeffnen", [ebenen] { (void)me::oeffneText("ui/stapel.menu", ebenen); }));
    add(pause(0.2));
    add(pruefe("Ebenen: Alt+Klick waehlt die Items unter dem Zeiger der Reihe nach (oben, mitte, unten, oben)", [] {
        std::vector<int> folge;
        me::waehle(-1, false);
        for (int k = 0; k < 4; ++k) {
            me::altKlickVorschau(150, 150);
            const auto x = me::stand();
            folge.push_back(x.auswahl.size() == 1 ? x.auswahl[0] : -9);
        }
        return folge == std::vector<int>{2, 1, 0, 2};
    }));
    add(tu("Ebenen: oberstes nur in der Vorschau ausblenden", [] { me::verbergeItem(2, true); }));
    add(pause(0.2));
    add(pruefe("Ebenen: Auge aendert den Text nicht, Alt+Klick ueberspringt das verborgene", [ebenen] {
        me::waehle(-1, false);
        me::altKlickVorschau(150, 150);
        const auto x = me::stand();
        return x.text == ebenen && x.auswahl.size() == 1 && x.auswahl[0] == 1;
    }));
    add(tu("Ebenen: wieder einblenden, unten nach ganz vorn ziehen", [] {
        me::verbergeItem(2, false);
        (void)me::ebeneAnStelle(0, 2);
    }));
    add(pruefe("Ebenen: Reihenfolge -> Text (mitte, oben, unten), Kommentar bleibt, ein Rueckgaengig-Schritt", [ebenen] {
        const std::string t = me::stand().text;
        const std::size_t a = t.find("name mitte"), b = t.find("name oben"), c = t.find("name unten");
        const bool reihe = a < b && b < c && c != std::string::npos && t.find("// zwischen den Bloecken") != std::string::npos;
        (void)me::rueckgaengig();
        return reihe && me::stand().text == ebenen;
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
    // Layout: rechte Spalte mit Reitern (Hierarchie / Text / Eigenschaften), Flaeche in voller Hoehe
    add(tu("M2 Layout: Reiter Text", [] { me::setzeReiter(1); }));
    add(pause(0.3));
    add(pruefe("M2 Layout: Reiter Text gewaehlt und gemerkt", [] { return me::reiter() == 1; }));
    add(foto("menue_reiter_text"));
    add(tu("M2 Layout: Reiter Hierarchie", [] { me::setzeReiter(0); }));
    add(pause(0.3));
    add(pruefe("M2 Layout: Reiter Hierarchie gewaehlt", [] { return me::reiter() == 0; }));
    add(foto("menue_reiter_hierarchie"));
    // Live-Diagnose: ein absichtlich kaputtes Menue (fehlende }, Tippfehler im Schluesselwort,
    // show auf ein Item, das es nicht gibt)
    add(tu("M2 Diagnose: kaputtes Menue oeffnen", [] {
        (void)me::oeffneText("ui/kaputt.menu",
                             "{\n\tmenuDef\n\t{\n\t\tname kaputt\n\t\trect 0 0 640 480\n\t\tvisible 1\n"
                             "\t\titemDef\n\t\t{\n\t\t\tname knopf\n\t\t\trect 100 80 200 40\n\t\t\tstyle WINDOW_STYLE_FILLED\n"
                             "\t\t\tbackcolr 1 0 0 1\n\t\t\tvisible 1\n\t\t\taction { show knopp }\n\t}\n}\n");
        me::setzeReiter(1);
    }));
    add(pause(1.0));
    add(foto("menue_diagnose"));
    add(pruefe("M2 Diagnose: im Hintergrund gelaufen, Fehler und Warnungen gemeldet", [st] {
        const auto x = st();
        std::string t;
        for (const std::string& z : x.diagnosen) { t += " | " + z; }
        diag::info("Menue-Editor Diagnose: " + std::to_string(x.diagFehler) + " Fehler, " + std::to_string(x.diagWarnungen) + " Warnungen, " +
                   std::to_string(x.diagHinweise) + " Hinweise, " + std::to_string(x.diagLaeufe) + " Laeufe" + t);
        return x.diagAktuell && x.diagFehler >= 1 && x.diagWarnungen >= 1 && x.diagLaeufe >= 1;
    }));
    add(tu("M2 Diagnose: Schnellkorrektur 1", [] {
        for (int i = 0; i < 20; ++i) {
            if (me::korrigiereDiagnose(i)) { break; }
        }
    }));
    add(pause(0.8));
    add(tu("M2 Diagnose: Schnellkorrektur 2", [] {
        for (int i = 0; i < 20; ++i) {
            if (me::korrigiereDiagnose(i)) { break; }
        }
    }));
    add(pause(0.8));
    add(tu("M2 Diagnose: Schnellkorrektur 3", [] {
        for (int i = 0; i < 20; ++i) {
            if (me::korrigiereDiagnose(i)) { break; }
        }
    }));
    add(pause(0.8));
    add(pruefe("M2 Diagnose: nach den Korrekturen weniger Meldungen (Klammer und Schluesselwort behoben)", [st] {
        const auto x = st();
        std::string t;
        for (const std::string& z : x.diagnosen) { t += " | " + z; }
        diag::info("Menue-Editor Diagnose nach Korrektur: " + std::to_string(x.diagFehler) + " Fehler, " + std::to_string(x.diagWarnungen) +
                   " Warnungen" + t);
        return x.diagAktuell && x.diagFehler == 0 && x.text.find("backcolor 1 0 0 1") != std::string::npos;
    }));
    // Punkt 6: eigene Texte in zwei Sprachen, Vorschau in der gewaehlten Sprache
    add(tu("M2 Texte: Menue mit @MYMENU_START, Text anlegen", [] {
        (void)me::oeffneText("ui/texte.menu",
                             "{\n\tmenuDef\n\t{\n\t\tname texte\n\t\trect 0 0 640 480\n\t\tvisible 1\n"
                             "\t\titemDef\n\t\t{\n\t\t\tname start\n\t\t\trect 100 100 200 40\n\t\t\ttext @MYMENU_START\n"
                             "\t\t\ttextscale 1\n\t\t\tforecolor 1 1 1 1\n\t\t\tvisible 1\n\t\t}\n\t}\n}\n");
        (void)me::legeTextAn("START", {{"English", "Start"}, {"German", "Los"}});
        me::setzeVorschauSprache("German");
    }));
    add(pause(0.4));
    add(foto("menue_texte_deutsch"));
    add(pruefe("M2 Texte: Vorschau Deutsch zeigt den eigenen Text", [] { return me::loeseText("@MYMENU_START") == "Los"; }));
    add(tu("M2 Texte: Vorschau Englisch", [] { me::setzeVorschauSprache("English"); }));
    add(pruefe("M2 Texte: Englisch, .str im Paket", [st] {
        const auto x = st();
        bool str = false;
        for (const std::string& p : x.paket) { str = str || p == "strings/English/MYMENU.str"; }
        return me::loeseText("@MYMENU_START") == "Start" && str && me::textPaket() == "MYMENU";
    }));
    // Punkt 9: Schrift aus einer TTF des Systems
    add(tu("M2 Schrift: arial.ttf nach fonts/probe", [] {
        (void)me::baueSchriftAusTtf("C:\\Windows\\Fonts\\arial.ttf", "probe", 20);
    }));
    add(pruefe("M2 Schrift: .fontdat und .tga im Paket (fehlt die TTF, gilt das als bestanden)", [st] {
        std::error_code ec;
        if (!std::filesystem::exists("C:\\Windows\\Fonts\\arial.ttf", ec)) { return true; }
        const auto x = st();
        int n = 0;
        for (const std::string& p : x.paket) { n += (p == "fonts/probe.fontdat" || p == "fonts/probe.tga") ? 1 : 0; }
        return n == 2;
    }));
    // Punkt 7: fuer das Spiel pruefen, nur Aenderungen exportieren (ausserhalb des Spiels)
    add(pruefe("M2 Pruefung: fuer JA, JK2 und Movie Duels", [] {
        const int n = me::pruefeFuerSpiel(true, true, true);
        diag::info("Menue-Editor Pruefung fuer das Spiel: " + std::to_string(n) + " Befunde");
        return n >= 0;
    }));
    add(pruefe("M2 Export: nur Aenderungen (offene, geaenderte Menues und Paket)", [einst] {
        const std::string ziel = einst + "\\m2_aenderungen.pk3";
        const bool ok = me::exportiereAenderungen(ziel);
        diag::info("Menue-Editor Export nur Aenderungen: " + std::to_string(me::letzterExport()) + " Dateien");
        std::error_code ec;
        return ok && me::letzterExport() >= 3 && std::filesystem::exists(ziel, ec);
    }));
    // Punkt 8: Testpaket und Befehlszeile - nie gestartet (BHED_EDITORTEST)
    // M5b: ohne fs_homepath (jasp, jk2sp) der eigene Ordner als fs_basepath mit den Spieldaten ueber fs_cdpath
    add(pruefe("M2 Im Spiel testen: Paket im eigenen Ordner, Befehlszeile mit fs_homepath bzw. fs_basepath + fs_cdpath", [] {
        const std::string b = me::imSpielTesten();
        diag::info("Menue-Editor Im Spiel testen: " + (b.empty() ? std::string("(kein Programm)") : b));
        return b.empty() || b.find("fs_homepath") != std::string::npos || b.find("fs_game") != std::string::npos ||
               (b.find("fs_basepath") != std::string::npos && b.find("fs_cdpath") != std::string::npos);
    }));
    return s;
}

// --- M3: asiatische Kodierungen, HUD, Saebelstile, Klaenge, Haltung ---
std::vector<Schritt> schritteM3() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = [] { return me::stand(); };
    // Punkt 7: getippt wird UTF-8; ins Paket kommt die Kodierung, in der die Schrift der
    // Engine je se_language liest (KSC5601-Hangul, Shift-JIS, GB2312, Big5)
    const std::string annyeong = "\xEC\x95\x88\xEB\x85\x95";                                 // U+C548 U+B155
    const std::string konnichiwa = "\xE3\x81\x93\xE3\x82\x93\xE3\x81\xAB\xE3\x81\xA1\xE3\x81\xAF";
    const std::string nihao = "\xE4\xBD\xA0\xE5\xA5\xBD";                                    // U+4F60 U+597D
    add(tu("M3 Texte: Menue mit @MYMENU_HALLO, Text auf Koreanisch/Japanisch/Chinesisch/Taiwanesisch", [=] {
        (void)me::oeffneText("ui/asien.menu",
                             "{\n\tmenuDef\n\t{\n\t\tname asien\n\t\trect 0 0 640 480\n\t\tvisible 1\n"
                             "\t\titemDef\n\t\t{\n\t\t\tname hallo\n\t\t\trect 100 100 400 60\n\t\t\ttext @MYMENU_HALLO\n"
                             "\t\t\ttextscale 1\n\t\t\tforecolor 1 1 1 1\n\t\t\tvisible 1\n\t\t}\n\t}\n}\n");
        (void)me::legeTextAn("HALLO", {{"English", "Hello"}, {"Korean", annyeong + " \xEB\x98\xA0"}, {"Japanese", konnichiwa + "\xEF\xBD\xB1"},
                                       {"Chinese", nihao}, {"Taiwanese", nihao}});
    }));
    add(pruefe("M3 Texte: Paket in KSC5601, Shift-JIS, GB2312 und Big5 (nicht Darstellbares als ?)", [] {
        const std::string ko = me::paketInhalt("strings/Korean/MYMENU.str");
        const std::string ja = me::paketInhalt("strings/Japanese/MYMENU.str");
        const std::string zh = me::paketInhalt("strings/Chinese/MYMENU.str");
        const std::string tw = me::paketInhalt("strings/Taiwanese/MYMENU.str");
        diag::info("Menue-Editor Texte asiatisch: " + std::to_string(ko.size()) + "/" + std::to_string(ja.size()) + "/" +
                   std::to_string(zh.size()) + "/" + std::to_string(tw.size()) + " Bytes");
        return ko.find("LANG_KOREAN    \"\xBE\xC8\xB3\xE7 ?\"") != std::string::npos &&
               ja.find("LANG_JAPANESE    \"\x82\xB1\x82\xF1\x82\xC9\x82\xBF\x82\xCD?\"") != std::string::npos &&
               zh.find("LANG_CHINESE    \"\xC4\xE3\xBA\xC3\"") != std::string::npos &&
               tw.find("LANG_TAIWANESE    \"\xA7\x41\xA6\x6E\"") != std::string::npos;
    }));
    add(tu("M3 Texte: Vorschau auf Koreanisch", [] { me::setzeVorschauSprache("Korean"); }));
    add(pause(0.4));
    add(foto("menue_texte_koreanisch"));
    add(pruefe("M3 Texte: Vorschau zeichnet Hangul aus fonts/kor_32_1024_*", [st] {
        const auto x = st();
        diag::info("Menue-Editor Texte Koreanisch: " + std::to_string(x.asienGlyphen) + " asiatische Zeichen");
        return me::loeseText("@MYMENU_HALLO") == "\xBE\xC8\xB3\xE7 ?" && (g_app->settings.gamePaths.empty() || x.asienGlyphen == 2);
    }));
    add(tu("M3 Texte: Vorschau Englisch", [] { me::setzeVorschauSprache("English"); }));
    // Punkt 3: Klaenge beim Oeffnen des Hauptmenues wie im Spiel (s_show 1 bzw. developer 1, stumm):
    // JKA base - setfocus newgamebutton spielt den FocusSound sound/interface/menuroam (.wav fehlt,
    // .mp3); Movie Duels - exec "music music/mainMenu/main/CTCMusic" (S_StartBackgroundTrack)
    add(tu("M3 Klaenge: ui/main.menu aus dem Spiel im Bedienmodus oeffnen", [] {
        me::setzeInteraktion(false);
        (void)me::oeffneAusSpiel("ui/main.menu");
        me::klangListeLeeren();
        me::setzeInteraktion(true);
    }));
    add(pause(0.5));
    add(pruefe("M3 Klaenge: dieselben Klaenge wie im Spiel", [st] {
        const auto x = st();
        std::string l;
        for (const std::string& k : x.klangListe) { l += k + " "; }
        diag::info("Menue-Editor Klaenge Hauptmenue: " + l);
        if (g_app->settings.gamePaths.empty()) { return true; }
        const std::vector<std::string> erwartet =
            x.spiel == 2 ? std::vector<std::string>{"musik:music/mainMenu/main/CTCMusic"} : std::vector<std::string>{"sound/interface/menuroam.wav"};
        return x.klangListe == erwartet;
    }));
    add(tu("M3 Klaenge: Bedienmodus aus", [] { me::setzeInteraktion(false); }));
    // Punkt 5: HUD-Editor - ui/hud.menu aus dem Spiel mit Beispielwerten wie CG_DrawHUD
    add(tu("M3 HUD: ui/hud.menu, Gesundheit 70, Ruestung 30, Macht 100, Saebel mittel", [] {
        if (!me::oeffneAusSpiel("ui/hud.menu")) {   // ohne Spieldaten: ein kleines HUD wie Ravens
            (void)me::oeffneText("ui/hud.menu", R"({
	menuDef
	{
		name lefthud
		rect 0 380 120 100
		visible 1
		itemDef { name healthamount rect 50 440 8 12 forecolor 1 1 1 1 }
		itemDef { name armoramount rect 50 455 8 12 forecolor 1 1 1 1 }
	}
	menuDef
	{
		name righthud
		rect 520 380 120 100
		visible 1
		itemDef { name forceamount rect 560 440 8 12 forecolor 1 1 1 1 }
	}
}
)");
        }
        bhed::menue::HudWerte w;
        w.gesundheit = 70;
        w.ruestung = 30;
        me::setzeHudWerte(w);
    }));
    add(pause(0.4));
    add(foto("menue_hud"));
    add(pruefe("M3 HUD: Zahlenfelder 70, 30, 100 und das Fadenkreuz", [st] {
        const auto x = st();
        diag::info("Menue-Editor HUD: " + std::to_string(x.hudBilder) + " Zahlen/Fadenkreuz, " + std::to_string(x.prims) + " Zeichenbefehle" +
                   (x.hudNachgebildet ? "" : " (Movie Duels/MP: nur Menu_Paint)"));
        return x.hudNachgebildet ? x.hudBilder == 2 + 2 + 3 + 1 : x.hudBilder == 0;
    }));
    add(tu("M3 HUD: Munition statt Saebel (Gewehr), 0 Schuss", [] {
        bhed::menue::HudWerte w;
        w.saebel = false;
        w.munition = 0;
        me::setzeHudWerte(w);
    }));
    add(pause(0.3));
    add(foto("menue_hud_munition"));
    add(pruefe("M3 HUD: Munition 0 gezeichnet (100, 100, 100, 0 und Fadenkreuz)", [st] {
        const auto x = st();
        if (!x.hudNachgebildet) { return x.hudBilder == 0; }
        return x.hudBilder == 3 + 3 + 3 + 1 + (g_app->settings.gamePaths.empty() ? 0 : 1);
    }));
    // Punkt 4: echter Start - das Hauptmenue mit einer sichtbaren Aenderung ins Testpaket; der
    // Start selbst geschieht ausserhalb (nie im Selbsttest), mit genau dieser Befehlszeile
    add(tu("M3 Im Spiel testen: Hauptmenue mit sichtbarer Aenderung", [] {
        me::setzeInteraktion(false);
        (void)me::oeffneAusSpiel("ui/main.menu");
        me::fuegeItemsEin("itemDef\n{\n\tname jkstudio_test\n\ttype ITEM_TYPE_TEXT\n\trect 120 400 400 40\n\ttext \"JK STUDIO TEST\"\n"
                          "\ttextscale 1.5\n\tforecolor 1 0.2 0.2 1\n\tvisible 1\n\tdecoration\n}\n");
    }));
    add(pruefe("M3 Im Spiel testen: Paket im eigenen Ordner, Befehlszeile mit fs_homepath bzw. fs_basepath + fs_cdpath", [st] {
        const std::string b = me::imSpielTesten();
        diag::info("Menue-Editor Im Spiel testen M3: " + b);
        return st().text.find("JK STUDIO TEST") != std::string::npos &&
               (b.empty() || b.find("fs_homepath") != std::string::npos ||
                (b.find("fs_basepath") != std::string::npos && b.find("fs_cdpath") != std::string::npos));
    }));
    return s;
}

// --- M4: Video-Import (Groesse, Ton, fehlende Decoder, Shader), Ton-Schalter ---
// Die Probevideos (ffmpeg, siehe Notizen) liegen in BHED_MENUE_VIDEOS; ohne werden die
// Schritte mit Dateien uebersprungen.
std::vector<Schritt> schritteM4() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = [] { return me::stand(); };
    const char* vo = std::getenv("BHED_MENUE_VIDEOS");
    const std::string videos = vo != nullptr ? vo : "";
    const auto datei = [videos](const char* n) { return (std::filesystem::path(videos) / n).string(); };
    const std::string einst = platform::settingsDirectory();
    add(tu("M4: offene Frage von Im Spiel testen (M2/M3 mit JKA-Daten) schliessen", [] { me::schliesseTestFrage(); }));
    add(pause(0.2));
    add(pruefe("M4 Video: Vorgaben 512 x 512, 30 Bilder/s, Ton aus (Menues spielen stumm)", [] {
        const me::VideoImport o;
        return o.breite == 512 && o.hoehe == 512 && o.fps == 30 && !o.ton && o.einpassen == 0;
    }));
    add(tu("M4 Video: Hauptmenue des Spiels oeffnen", [] {
        if (!me::oeffneAusSpiel("ui/main.menu")) {
            (void)me::oeffneText("ui/main.menu", "{\n\tmenuDef\n\t{\n\t\tname mainMenu\n\t\trect 0 0 640 480\n\t\tvisible 1\n"
                                                 "\t\titemDef\n\t\t{\n\t\t\tname background_video\n\t\t\tstyle WINDOW_STYLE_SHADER\n"
                                                 "\t\t\trect 200 144 256 256\n\t\t\tbackground \"gfx/menus/videologo\"\n\t\t\tvisible 1\n"
                                                 "\t\t\tdecoration\n\t\t}\n\t}\n}\n");
        }
    }));
    auto kinoVorher = std::make_shared<int>(0);
    add(pruefe("M4 Video: H.264 1920x1080 -> RoQ 512x512, Shader menu/video/probe_h264 in shaders/mymenu.shader, Hintergrund des Elements",
               [datei, kinoVorher] {
                   if (std::getenv("BHED_MENUE_VIDEOS") == nullptr) { return true; }
                   me::VideoImport o;
                   o.fps = 15;   // halbe Kodierzeit; die Groesse bleibt die Vorgabe
                   std::string fehler, r;
                   int items = 0;
                   for (const char* name : {"background_video", "background_CTCvideo", "background_mainvideo"}) {   // JKA, Movie Duels
                       const int i = me::itemNummer(name);
                       if (i < 0) { continue; }
                       me::waehle(i, false);
                       r = me::importiereVideo(datei("probe_h264.mp4"), o, &fehler);
                       ++items;
                   }
                   const std::string roq = me::paketInhalt("video/probe_h264.roq");
                   bhed::menue::RoqSpieler sp;
                   const bool kopf = sp.start(roq, 0, false) && (sp.lauf(0), sp.breite() == 512 && sp.hoehe() == 512 && sp.fps() == 15);
                   const std::string sh = me::paketInhalt("shaders/mymenu.shader");
                   bhed::ShaderMap sm;
                   bhed::parseShaderScript(sh, sm);
                   const auto it = sm.find("menu/video/probe_h264");
                   const bool shaderOk = it != sm.end() && it->second.stages.size() == 1 && it->second.stages[0].video == "video/probe_h264.roq";
                   const auto x = me::stand();
                   const bool hintergrund = x.text.find("\"menu/video/probe_h264\"") != std::string::npos;
                   *kinoVorher = x.kinoBilder;
                   diag::info("Menue-Editor M4 Video: " + r + " " + std::to_string(roq.size()) + " Bytes, " + std::to_string(sp.breite()) + "x" +
                              std::to_string(sp.hoehe()) + " " + std::to_string(sp.fps()) + " fps, an " + std::to_string(items) + " Items, Shader " +
                              (shaderOk ? "ok" : "FEHLT") + (fehler.empty() ? "" : ", Meldung: " + fehler));
                   return items >= 1 && r == "video/probe_h264.roq" && kopf && shaderOk && hintergrund && fehler.empty();
               }));
    add(pause(1.2));
    add(foto("menue_video_m4"));
    add(pruefe("M4 Video: das Element zeigt das Video in der Vorschau (videoMap, RoQ-Bilder kommen)", [st, kinoVorher] {
        if (std::getenv("BHED_MENUE_VIDEOS") == nullptr) { return true; }
        const auto x = st();
        diag::info("Menue-Editor M4 Video: " + std::to_string(x.kinoBilder - *kinoVorher) + " Videobilder seit dem Import");
        return x.kinoBilder - *kinoVorher >= 3;
    }));
    add(pruefe("M4 Video: Export-pk3 enthaelt Menue, Video und Shader", [einst] {
        if (std::getenv("BHED_MENUE_VIDEOS") == nullptr) { return true; }
        const std::string ziel = (std::filesystem::path(einst) / "menue_video_m4.pk3").string();
        if (!me::exportierePk3(ziel)) { return false; }
        bhed::Pk3 pk;
        const bool ok = bhed::readPk3Directory(ziel, pk) && pk.find("ui/main.menu") != nullptr && pk.find("video/probe_h264.roq") != nullptr &&
                        pk.find("shaders/mymenu.shader") != nullptr;
        std::error_code ec;
        std::filesystem::remove(ziel, ec);
        return ok;
    }));
    add(pruefe("M4 Video: Im Spiel testen - Testpaket und Befehlszeile (nicht gestartet)", [] {
        // nur mit Movie-Duels-Daten: die Original-jasp.exe kennt kein fs_homepath, dort fragt
        // ein modales Fenster nach dem Mod-Ordner (M2) - das wuerde die Klicks danach abfangen
        if (std::getenv("BHED_MENUE_VIDEOS") == nullptr || me::stand().spiel != 2) { return true; }
        const std::string b = me::imSpielTesten();
        diag::info("Menue-Editor M4 Im Spiel testen: " + (b.empty() ? std::string("(kein Programm)") : b));
        return b.empty() || b.find("fs_homepath") != std::string::npos;
    }));
    add(pruefe("M4 Ton: Tonspur (440 Hz, 3 s) kommt als ZA_SOUND_STEREO im RoQ an", [datei, einst] {
        if (std::getenv("BHED_MENUE_VIDEOS") == nullptr) { return true; }
        const std::string kopie = (std::filesystem::path(einst) / "probe_ton.mp4").string();
        std::error_code ec;
        std::filesystem::copy_file(datei("probe_h264.mp4"), kopie, std::filesystem::copy_options::overwrite_existing, ec);
        me::VideoImport o;
        o.breite = o.hoehe = 64;
        o.fps = 10;
        o.ton = true;
        o.verwendung = 3;
        std::string fehler;
        const std::string r = me::importiereVideo(kopie, o, &fehler);
        int kanaele = 0;
        const std::vector<std::int16_t> pcm = bhed::menue::roqTonDekodieren(me::paketInhalt("video/probe_ton.roq"), &kanaele);
        int wechsel = 0;
        for (std::size_t i = 2; i < pcm.size(); i += 2) { wechsel += (pcm[i - 2] < 0) != (pcm[i] < 0) ? 1 : 0; }
        const double sek = static_cast<double>(pcm.size() / 2) / bhed::menue::kRoqTonRate;
        const double hz = sek > 0 ? wechsel / (2.0 * sek) : 0.0;
        diag::info("Menue-Editor M4 Ton: " + std::to_string(pcm.size() / 2) + " Proben, " + std::to_string(sek) + " s, " + std::to_string(hz) + " Hz");
        return r == "video/probe_ton.roq" && fehler.empty() && kanaele == 2 && std::abs(sek - 3.0) < 0.1 && std::abs(hz - 440.0) < 5.0;
    }));
    add(pruefe("M4 Ton: Video ohne Tonspur -> Hinweis, Video trotzdem im Paket", [datei] {
        if (std::getenv("BHED_MENUE_VIDEOS") == nullptr) { return true; }
        me::VideoImport o;
        o.breite = o.hoehe = 64;
        o.fps = 10;
        o.ton = true;
        o.verwendung = 3;
        std::string fehler;
        const std::string r = me::importiereVideo(datei("probe_stumm.mp4"), o, &fehler);
        return r == "video/probe_stumm.roq" && fehler == tr(Str::MeVideoOhneTon);
    }));
    add(pruefe("M4 Codec: nicht dekodierbares AVI (ZZZZ) -> klare Meldung, kein leeres Video", [datei] {
        if (std::getenv("BHED_MENUE_VIDEOS") == nullptr) { return true; }
        me::VideoImport o;
        o.verwendung = 3;
        std::string fehler, fehler2;
        const std::string r = me::importiereVideo(datei("probe_zzzz.avi"), o, &fehler);
        const std::string r2 = me::importiereVideo(datei("kaputt.mp4"), o, &fehler2);
        char soll[600];
        std::snprintf(soll, sizeof(soll), tr(Str::MeVideoCodecAnders), "ZZZZ");
        diag::info("Menue-Editor M4 Codec: " + fehler + " | " + fehler2);
        return r.empty() && fehler == soll && me::paketInhalt("video/probe_zzzz.roq").empty() && r2.empty() && fehler2 == tr(Str::MeVideoNichtLesbar);
    }));
    add(pruefe("M4 Codec: HEVC, VP9, AV1, Xvid - dekodiert oder die passende Meldung", [datei] {
        if (std::getenv("BHED_MENUE_VIDEOS") == nullptr) { return true; }
        struct Fall {
            const char* datei;
            Str meldung;
        };
        const Fall faelle[] = {{"probe_hevc.mp4", Str::MeVideoCodecHevc}, {"probe_vp9.webm", Str::MeVideoCodecVp},
                               {"probe_av1.mp4", Str::MeVideoCodecAv1}, {"probe_xvid.avi", Str::MeVideoCodecDivx}};
        bool ok = true;
        for (const Fall& f : faelle) {
            me::VideoImport o;
            o.breite = o.hoehe = 64;
            o.fps = 5;
            o.verwendung = 3;
            std::string fehler;
            const std::string r = me::importiereVideo(datei(f.datei), o, &fehler);
            diag::info(std::string("Menue-Editor M4 Codec ") + f.datei + ": " + (r.empty() ? fehler : "dekodiert -> " + r));
            ok = ok && (r.empty() ? fehler == tr(f.meldung) : fehler.empty());
        }
        return ok;
    }));
    add(pruefe("M4 Video: vorhandenes RoQ unveraendert uebernommen (ohne Neukodieren)", [einst] {
        if (std::getenv("BHED_MENUE_VIDEOS") == nullptr) { return true; }
        const std::string roq = me::paketInhalt("video/probe_h264.roq");
        const std::string pfad = (std::filesystem::path(einst) / "direkt.roq").string();
        {
            std::ofstream f(pfad, std::ios::binary);
            f.write(roq.data(), static_cast<std::streamsize>(roq.size()));
        }
        me::VideoImport o;
        o.breite = o.hoehe = 64;   // gilt fuer ein RoQ nicht
        o.verwendung = 3;
        const std::string r = me::importiereVideo(pfad, o);
        return r == "video/direkt.roq" && !roq.empty() && me::paketInhalt("video/direkt.roq") == roq;
    }));
    // der Dialog (Handbuchbild): Quelle, Groesse, Einpassen, Ton, Verwendung, geschaetzte Groesse
    add(tu("M4 Video: Import-Dialog fuer das H.264-Video zeigen", [datei] {
        if (std::getenv("BHED_MENUE_VIDEOS") != nullptr) { me::zeigeVideoImport(datei("probe_h264.mp4")); }
    }));
    add(pause(4.0));   // die Schaetzung kodiert im Hintergrund die ersten Takte
    add(foto("menue_video_dialog"));
    add(tu("M4 Video: Dialog schliessen", [] { me::zeigeVideoImport(""); }));
    // Ton-Schalter der Leiste: gespeichert, aus = keine laufenden Klaenge
    add(pruefe("M4 Ton-Schalter: Einstellung menueTon wird gespeichert und gelesen", [] {
        bhed::Settings a;
        a.menueTon = false;
        bhed::Settings b;
        return bhed::readSettings(bhed::writeSettings(a), b) && !b.menueTon && bhed::Settings{}.menueTon;
    }));
    add(tu("M4 Ton-Schalter: an", [] { g_app->settings.menueTon = true; }));
    add(pause(0.2));
    add(klick(tr(Str::MeTonAn)));
    add(pause(0.2));
    add(pruefe("M4 Ton-Schalter: Klick schaltet aus, keine laufenden Klaenge", [st] { return !g_app->settings.menueTon && st().klangStimmen == 0; }));
    add(klick(tr(Str::MeTonAus)));
    add(pause(0.2));
    add(pruefe("M4 Ton-Schalter: zweiter Klick schaltet wieder an", [] { return g_app->settings.menueTon; }));
    // Punkt 3: Lesbarkeit - Augen-/Schloss-Symbole, Leiste mit Beschriftung, Mindestschrift,
    // Fotos bei 100 % und 150 % Oberflaechengroesse
    auto skala = std::make_shared<float>(1.0F);
    add(tu("M4 Oberflaeche: Groesse merken, 100 %, Reiter Hierarchie", [skala] {
        *skala = g_app->uiScale;
        g_app->uiScale = 1.0F;
        me::setzeReiter(0);   // die Ebenenliste (nach M2/M3 steht ein anderer Reiter vorn)
    }));
    add(pause(0.4));
    add(klick("##auge"));
    add(pause(0.2));
    add(pruefe("M4 Oberflaeche: Auge der Ebenenliste blendet das Item in der Vorschau aus", [] { return me::stand().verborgen == 1; }));
    add(klick("##auge"));
    add(klick("##schloss"));
    add(pause(0.2));
    add(pruefe("M4 Oberflaeche: Schloss der Ebenenliste sperrt das Item", [] { return me::stand().verborgen == 0 && me::stand().gesperrt == 1; }));
    add(klick("##schloss"));
    add(pause(0.3));
    add(foto("menue_oberflaeche_100"));
    add(tu("M4 Oberflaeche: 150 %", [] { g_app->uiScale = 1.5F; }));
    add(pause(0.6));
    add(foto("menue_oberflaeche_150"));
    add(tu("M4 Oberflaeche: Leistenknoepfe mit Beschriftung", [] {
        g_app->uiScale = 1.0F;
        g_app->settings.menueLeisteText = true;
    }));
    add(pause(0.4));
    add(foto("menue_leiste_text"));
    add(pruefe("M4 Oberflaeche: mit Beschriftung zeigt der Knopf Symbol und Text", [] {
        const Element* e = finde(tr(Str::MeVideoImport), "", 0, false, "");
        return e != nullptr && e->rect.GetWidth() > e->rect.GetHeight() * 2.0F;
    }));
    add(tu("M4 Oberflaeche: Leiste wieder nur Symbole, Groesse 50 % versuchen", [] {
        g_app->settings.menueLeisteText = false;
        setUiScale(g_app, 0.5F);   // wie aus den Einstellungen (Laden und setUiScale begrenzen)
    }));
    add(pause(0.4));
    add(pruefe("M4 Oberflaeche: Mindestschrift - nie unter 75 % (12 px)", [] {
        diag::info("Menue-Editor M4 Oberflaeche: Groesse " + std::to_string(g_app->uiScale) + ", Schrift " + std::to_string(ImGui::GetFontSize()) + " px");
        return g_app->uiScale >= kUiScaleMin - 1e-4F && ImGui::GetFontSize() >= 12.0F * g_app->dpiScale - 0.01F;
    }));
    add(pruefe("M4 Oberflaeche: ohne Beschriftung nur das Symbol (quadratisch)", [] {
        const Element* e = finde(tr(Str::MeVideoImport), "", 0, false, "");
        return e != nullptr && e->rect.GetWidth() < e->rect.GetHeight() * 1.2F;
    }));
    add(tu("M4 Oberflaeche: Groesse zurueck", [skala] { g_app->uiScale = *skala; }));
    add(pause(0.3));
    return s;
}

// M5 Punkt 1: Layouts wie Qt - Aufgabentest ohne Text-Reiter ("drei Knoepfe nebeneinander im Rahmen"),
// Spacer, Rahmen verschieben, Rueckgaengig, Aufloesen; die Rundlaeufe prueft ctest menue_layout.
const char* const kLayoutProbe =
    "{\n\tmenuDef\n\t{\n\t\tname layoutprobe\n\t\trect 0 0 640 480\n\t\tvisible 1\n"
    "\t\titemDef\n\t\t{\n\t\t\tname panel\n\t\t\tstyle WINDOW_STYLE_FILLED\n\t\t\tbackcolor 0 0 0.4 1\n\t\t\trect 100 100 440 60\n\t\t\tvisible 1\n\t\t}\n"
    "\t\titemDef\n\t\t{\n\t\t\tname knopf1\n\t\t\ttype ITEM_TYPE_BUTTON\n\t\t\ttext \"Eins\"\n\t\t\trect 120 110 60 20\n\t\t\tvisible 1\n\t\t}\n"
    "\t\titemDef\n\t\t{\n\t\t\tname knopf2\n\t\t\ttype ITEM_TYPE_BUTTON\n\t\t\ttext \"Zwei\"\n\t\t\trect 200 130 60 20\n\t\t\tvisible 1\n\t\t}\n"
    "\t\titemDef\n\t\t{\n\t\t\tname knopf3\n\t\t\ttype ITEM_TYPE_BUTTON\n\t\t\ttext \"Drei\"\n\t\t\trect 300 125 60 20\n\t\t\tvisible 1\n\t\t}\n"
    "\t}\n}\n";

std::vector<Schritt> schritteM5() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = [] { return me::stand(); };
    auto rect = [](const std::string& text, const std::string& name) {
        // rect des itemDef mit diesem Namen aus dem Text (wie ihn das Spiel liest)
        const std::size_t n = text.find("name " + name + "\n");
        const std::size_t r = n == std::string::npos ? n : text.find("rect ", n);
        if (r == std::string::npos) { return std::string(); }
        return text.substr(r + 5, text.find('\n', r) - r - 5);
    };
    auto schritte = std::make_shared<int>(0);
    add(tu("M5 Layout: Probe oeffnen, Reiter Eigenschaften", [] {
        (void)me::oeffneText("ui/layoutprobe.menu", kLayoutProbe);
        me::setzeReiter(2);
    }));
    add(pause(0.3));
    add(pruefe("M5 Layout: ohne Auswahl der leere Zustand mit Zeig mir wie", [] { return finde(tr(Str::MeLayoutZeigMir), "", 0, false, "") != nullptr; }));
    // Aufgabe: Rahmen und drei Knoepfe waehlen (4 Klicks wie Strg+Klick), dann ein Klick auf Nebeneinander
    add(tu("M5 Layout Aufgabe: panel, knopf1..3 waehlen (Strg+Klick)", [schritte] {
        for (const char* n : {"panel", "knopf1", "knopf2", "knopf3"}) {
            me::waehle(me::itemNummer(n), true);
            ++*schritte;
        }
    }));
    add(pause(0.2));
    add(klick(tr(Str::MeLayoutWaagrecht)));
    add(tu("M5 Layout Aufgabe: Schritt zaehlen", [schritte] { ++*schritte; }));
    add(pause(0.3));
    add(pruefe("M5 Layout Aufgabe: Rahmen = panel, Rand 8, Abstand 8, nebeneinander - in 5 Schritten ohne Text-Reiter", [st, rect, schritte] {
        const auto x = st();
        const bool zeile = x.text.find("// jkstudio:layout name=layout1 type=hbox frame=panel spacing=8 margins=8,8,8,8 items=knopf1,knopf2,knopf3") !=
                           std::string::npos;
        char b[200];
        std::snprintf(b, sizeof(b), "Menue-Editor M5 Aufgabe Layout: %d Schritte, knopf1 %s, knopf3 %s", *schritte, rect(x.text, "knopf1").c_str(),
                      rect(x.text, "knopf3").c_str());
        diag::info(b);
        return zeile && rect(x.text, "knopf1") == "108 108 60 20" && rect(x.text, "knopf2") == "176 108 60 20" &&
               rect(x.text, "knopf3") == "244 108 60 20" && *schritte <= 5;
    }));
    add(foto("menue_layout"));
    // Spacer hinter knopf2: knopf3 rutscht an den rechten Rand (108 + 440 - 16 = 532 -> x 472)
    add(tu("M5 Layout: knopf2 waehlen", [] { me::waehle(me::itemNummer("knopf2"), false); }));
    add(pause(0.2));
    add(klick(tr(Str::MeLayoutSpacerNeu)));
    add(pause(0.3));
    add(pruefe("M5 Layout: Spacer schiebt knopf3 an den rechten Rand", [st, rect] { return rect(st().text, "knopf3") == "472 108 60 20"; }));
    // Rahmen verschieben: die Kinder ziehen mit (eine Ersetzung, ein Rueckgaengig-Schritt)
    add(tu("M5 Layout: panel waehlen und 10 nach unten", [] {
        me::waehle(me::itemNummer("panel"), false);
        me::verschiebeAuswahl(0, 10);
    }));
    add(pause(0.3));
    add(pruefe("M5 Layout: Kinder folgen dem Rahmen", [st, rect] {
        const auto x = st();
        return rect(x.text, "panel") == "100 110 440 60" && rect(x.text, "knopf1") == "108 118 60 20" && rect(x.text, "knopf3") == "472 118 60 20";
    }));
    add(tu("M5 Layout: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
    add(pause(0.2));
    add(pruefe("M5 Layout: ein Schritt zurueck stellt Rahmen und Kinder wieder her", [st, rect] {
        const auto x = st();
        return rect(x.text, "panel") == "100 100 440 60" && rect(x.text, "knopf1") == "108 108 60 20";
    }));
    // Aufloesen: Kommentar weg, rects bleiben
    add(tu("M5 Layout: knopf1 waehlen", [] { me::waehle(me::itemNummer("knopf1"), false); }));
    add(pause(0.2));
    add(klick(tr(Str::MeLayoutAufloesen)));
    add(pause(0.3));
    add(pruefe("M5 Layout: Aufloesen entfernt nur die Kommentarzeile", [st, rect] {
        const auto x = st();
        return x.text.find("jkstudio:layout") == std::string::npos && rect(x.text, "knopf3") == "472 108 60 20";
    }));
    add(tu("M5 Layout: Rueckgaengig bis zum Original", [] {
        for (int i = 0; i < 4; ++i) { (void)me::rueckgaengig(); }
    }));
    add(pause(0.2));
    add(pruefe("M5 Layout: nach Rueckgaengig byte-gleich mit dem Original", [st] { return st().text == kLayoutProbe; }));
    // M5 Punkt 2, Aufgabe: "knopf1 soll beim Hover leuchten und beim Klick klicken" - ohne Text-Reiter
    auto ev = std::make_shared<int>(0);
    add(tu("M5 Ereignisse Aufgabe: knopf1 waehlen, Reiter Ereignisse", [ev] {
        me::waehle(me::itemNummer("knopf1"), false);
        me::setzeReiter(3);
        *ev = 2;   // Auswahl und Reiter
    }));
    add(pause(0.3));
    add(klick(tr(Str::MeEvLeuchten)));
    add(pause(0.3));
    add(klick(tr(Str::MeEvKlickKlang)));
    add(tippe("sound/interface/button1.wav"));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(tu("M5 Ereignisse Aufgabe: Schritte zaehlen (Leuchten, Feld, Tippen, Enter)", [ev] { *ev += 4; }));
    add(pause(0.3));
    add(pruefe("M5 Ereignisse Aufgabe: Glow-Element, Hover show/hide, Klick spielt - in 6 Schritten; das Spiel liest es ohne Meldung", [st, ev] {
        const auto x = st();
        bhed::menue::LeseOptionen o;
        o.dialekt = bhed::menue::Dialekt::SP;
        const bhed::menue::Datei d = bhed::menue::lies("ui/layoutprobe.menu", x.text, o);
        if (!d.meldungen.empty() || d.menus.size() != 1) { return false; }
        const auto& m = d.menus[0];
        int glow = -1, knopf = -1;
        for (std::size_t i = 0; i < m.items.size(); ++i) {
            if (m.items[i].window.name == "knopf1_glow") { glow = static_cast<int>(i); }
            if (m.items[i].window.name == "knopf1") { knopf = static_cast<int>(i); }
        }
        if (glow < 0 || knopf < 0 || glow > knopf) { return false; }
        // wie im Spiel: Maus auf den Knopf -> mouseEnter zeigt das Glow-Element
        bhed::menue::Laufzeit lz;
        lz.start(d);
        lz.oeffne("layoutprobe");
        const bool vorher = (lz.items[0][static_cast<std::size_t>(glow)].flags & bhed::menue::WINDOW_VISIBLE) != 0;
        const auto& r = m.items[static_cast<std::size_t>(knopf)].window.rect;
        lz.maus(0, r.x + r.w / 2, r.y + r.h / 2);
        const bool hover = (lz.items[0][static_cast<std::size_t>(glow)].flags & bhed::menue::WINDOW_VISIBLE) != 0;
        const bool klang = m.items[static_cast<std::size_t>(knopf)].action.find("sound/interface/button1.wav") != std::string::npos;
        char b[160];
        std::snprintf(b, sizeof(b), "Menue-Editor M5 Aufgabe Ereignisse: %d Schritte, Glow vorher %d, beim Hover %d, Klick-Klang %d", *ev, vorher, hover,
                      klang);
        diag::info(b);
        return !vorher && hover && klang && *ev <= 6;
    }));
    add(foto("menue_ereignisse"));
    add(tu("M5 Ereignisse: zurueck zum Reiter Eigenschaften", [] { me::setzeReiter(2); }));
    add(tu("M5 Ereignisse: zweimal Rueckgaengig", [] {
        (void)me::rueckgaengig();
        (void)me::rueckgaengig();
    }));
    add(pause(0.2));
    add(pruefe("M5 Ereignisse: Rueckgaengig stellt das Original her", [st] { return st().text == kLayoutProbe; }));
    return s;
}

// M5b Punkt 1: show/hide je Zustand - knopf2 ist nach dem Oeffnen versteckt, "zeig" blendet ihn ein,
// "weg" wieder aus. Mit "Versteckte Elemente geben ihren Platz frei" bekommen beide Skripte (und
// onOpen) setitemrect; eine Laufzeit wie das Spiel legt die Knoepfe nach jedem Klick wie das Layout.
const char* const kZustandProbe =
    "{\n\tmenuDef\n\t{\n\t\tname zustandprobe\n\t\trect 0 0 640 480\n\t\tvisible 1\n\t\tonOpen { hide knopf2 }\n"
    "\t\titemDef\n\t\t{\n\t\t\tname panel\n\t\t\tstyle WINDOW_STYLE_FILLED\n\t\t\tbackcolor 0 0 0.4 1\n\t\t\trect 100 100 440 60\n\t\t\tvisible 1\n\t\t}\n"
    "\t\titemDef\n\t\t{\n\t\t\tname knopf1\n\t\t\ttype ITEM_TYPE_BUTTON\n\t\t\ttext \"Eins\"\n\t\t\trect 120 110 60 20\n\t\t\tvisible 1\n\t\t}\n"
    "\t\titemDef\n\t\t{\n\t\t\tname knopf2\n\t\t\ttype ITEM_TYPE_BUTTON\n\t\t\ttext \"Zwei\"\n\t\t\trect 200 130 60 20\n\t\t\tvisible 1\n\t\t}\n"
    "\t\titemDef\n\t\t{\n\t\t\tname knopf3\n\t\t\ttype ITEM_TYPE_BUTTON\n\t\t\ttext \"Drei\"\n\t\t\trect 300 125 60 20\n\t\t\tvisible 1\n\t\t}\n"
    "\t\titemDef\n\t\t{\n\t\t\tname zeig\n\t\t\ttype ITEM_TYPE_BUTTON\n\t\t\ttext \"Zeig\"\n\t\t\trect 100 300 60 20\n\t\t\tvisible 1\n"
    "\t\t\taction\n\t\t\t{\n\t\t\t\tshow knopf2 ;\n\t\t\t}\n\t\t}\n"
    "\t\titemDef\n\t\t{\n\t\t\tname weg\n\t\t\ttype ITEM_TYPE_BUTTON\n\t\t\ttext \"Weg\"\n\t\t\trect 200 300 60 20\n\t\t\tvisible 1\n"
    "\t\t\taction { hide knopf2 }\n\t\t}\n"
    "\t}\n}\n";

// M5b Punkt 3: Projektordner + Projektdatei, Sicherung und Wiederherstellen nach einem "Absturz",
// Export, Doppelklick (derselbe Weg wie eine zweite Instanz bzw. die Kommandozeile), Frage zur
// Dateizuordnung (nur mit Nein beantwortet - der Test aendert nie die echte Windows-Zuordnung).
std::string pfadText(const std::filesystem::path& p) {
    const std::u8string t = p.u8string();
    return std::string(t.begin(), t.end());
}

std::vector<Schritt> schritteM5bProjekt() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = [] { return me::stand(); };
    namespace fs = std::filesystem;
    const auto ordner = std::make_shared<fs::path>(bhed::utf8Pfad(platform::executableDirectory()) / "projekt_probe");
    const auto datei = std::make_shared<std::string>();
    const auto geaendert = std::make_shared<std::string>();
    const auto zuordnungVorher = std::make_shared<std::string>(platform::liesBenutzerRegistry("Software\\Classes\\.jksmenu"));
    add(tu("M5b Projekt: Menue und ein Bild oeffnen, neues Projekt anlegen", [ordner, datei] {
        std::error_code ec;
        fs::remove_all(*ordner, ec);
        fs::create_directories(*ordner, ec);
        me::schliesseAlleOhneSpeichern();
        (void)me::oeffneText("ui/projektprobe.menu", kLayoutProbe);
        me::legeInsPaket("gfx/menus/probe.tga", std::string("TGA-Probe"));
        *datei = pfadText(*ordner / "probe.jksmenu");
        (void)me::legeProjektAn(*datei);
    }));
    add(pause(0.3));
    add(pruefe("M5b Projekt: Ordner wie die pk3, Menue und Bild darin, Projektdatei nennt beide", [st, ordner, datei] {
        std::error_code ec;
        bool ok = me::projektDatei() == *datei;
        for (const char* u : {"ui", "gfx", "video", "shaders", "strings"}) { ok = ok && fs::is_directory(*ordner / u, ec); }
        std::ifstream f(bhed::utf8Pfad(*datei), std::ios::binary);
        std::ostringstream t;
        t << f.rdbuf();
        const std::string txt = t.str();
        ok = ok && fs::is_regular_file(*ordner / "ui" / "projektprobe.menu", ec) && fs::is_regular_file(*ordner / "gfx" / "menus" / "probe.tga", ec);
        diag::info("Menue-Editor M5b Projekt: " + txt.substr(0, 300));
        return ok && txt.find("menu ui/projektprobe.menu") != std::string::npos &&
               txt.find("asset ui/projektprobe.menu gfx/menus/probe.tga") != std::string::npos && st().dokumente == 1;
    }));
    // aendern, sichern, "Absturz": alles zu ohne Speichern, Projekt wieder auf -> Frage, Wiederherstellen
    add(tu("M5b Projekt: knopf1 verschieben und sofort sichern", [geaendert] {
        me::waehle(me::itemNummer("knopf1"), false);
        me::verschiebeAuswahl(4, 0);
        *geaendert = me::stand().text;
        me::sichereJetzt();
    }));
    add(pruefe("M5b Projekt: Sicherung im Projektordner (.jksautosave) mit dem geaenderten Text", [ordner, geaendert] {
        std::ifstream f(*ordner / ".jksautosave" / "1.menu", std::ios::binary);
        std::ostringstream t;
        t << f.rdbuf();
        return !geaendert->empty() && *geaendert != kLayoutProbe && t.str() == *geaendert;
    }));
    add(tu("M5b Projekt: wie nach einem Absturz alles zu, Projekt wieder oeffnen", [datei] {
        me::schliesseAlleOhneSpeichern();
        (void)me::oeffneProjekt(*datei);
    }));
    add(pause(0.4));
    add(pruefe("M5b Projekt: Frage nach der Sicherung erscheint", [] { return finde(tr(Str::MePrjWiederherstellen), "", 0, false, "") != nullptr; }));
    add(foto("menue_projekt_sicherung"));
    add(klick(tr(Str::MePrjWiederherstellen)));
    add(pause(0.3));
    add(pruefe("M5b Projekt: Wiederherstellen bringt den geaenderten Text (ungespeichert), Sicherung weg", [st, ordner, geaendert] {
        std::error_code ec;
        return st().dokumente == 1 && st().text == *geaendert && !fs::exists(*ordner / ".jksautosave", ec);
    }));
    add(tu("M5b Projekt: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
    add(pruefe("M5b Projekt: Rueckgaengig fuehrt zur gespeicherten Fassung", [st] { return st().text == kLayoutProbe; }));
    add(tu("M5b Projekt: Wiederholen, Projekt speichern, als pk3 ausgeben", [] {
        (void)me::wiederholen();
        (void)me::speichereProjektJetzt();
        (void)me::exportiereProjektJetzt();
    }));
    add(pruefe("M5b Projekt: gespeichert (Menue auf der Platte geaendert), pk3 im Projektordner mit Menue und Bild", [ordner, geaendert] {
        std::error_code ec;
        std::ifstream f(*ordner / "ui" / "projektprobe.menu", std::ios::binary);
        std::ostringstream t;
        t << f.rdbuf();
        return t.str() == *geaendert && fs::is_regular_file(*ordner / "zz_menu.pk3", ec) && me::letzterExport() == 2;
    }));
    // Doppelklick: derselbe Weg wie eine zweite Instanz (WM_COPYDATA) bzw. die Kommandozeile (loadPath)
    add(tu("M5b Projekt: alles zu, Skript-Modus, Projekt per Doppelklick-Weg", [datei] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 0;
        bhed::gui::dateiVonAussen(*datei);
    }));
    add(pause(0.6));
    add(pruefe("M5b Projekt: Doppelklick oeffnet den Menue-Editor mit dem Projekt", [st, datei, geaendert] {
        return g_app->leftMode == 4 && me::projektDatei() == *datei && st().dokumente == 1 && st().text == *geaendert;
    }));
    // Dateizuordnung: nur nach Rueckfrage; der Test antwortet Nein
    add(tu("M5b Projekt: Frage zur Dateizuordnung", [] { me::fragZuordnung(); }));
    add(pause(0.3));
    add(pruefe("M5b Projekt: Rueckfrage zeigt Ja und Nein", [] {
        return finde(tr(Str::MePrjZuordnenJa), "", 0, false, "") != nullptr && finde(tr(Str::MePrjZuordnenNein), "", 0, false, "") != nullptr;
    }));
    add(klick(tr(Str::MePrjZuordnenNein)));
    add(pause(0.2));
    add(pruefe("M5b Projekt: nach Nein ist die Windows-Zuordnung unveraendert", [zuordnungVorher] {
        return finde(tr(Str::MePrjZuordnenJa), "", 0, false, "") == nullptr &&
               platform::liesBenutzerRegistry("Software\\Classes\\.jksmenu") == *zuordnungVorher;
    }));
    add(tu("M5b Projekt: Projekt schliessen", [] { me::schliesseAlleOhneSpeichern(); }));
    // M5b Punkt 4: Mod-Profil aus dem Profilordner; Menue mit Schluessel und Ownerdraw der Mod
    const auto profilOrdner = std::make_shared<fs::path>(bhed::utf8Pfad(platform::settingsDirectory()) / "menue_profile");
    add(tu("M5b Profil: Profil der Mod ablegen, Menue mit Mod-Schluessel oeffnen", [profilOrdner] {
        std::error_code ec;
        fs::create_directories(*profilOrdner, ec);
        std::ofstream f(*profilOrdner / "selbsttest_mod.jksprofile", std::ios::binary);
        f << "jksprofile 1\nname \"Selbsttest Mod\"\ngame ja\nitemkey glScale float\nownerdraw 300 GL_MAP_PREVIEW\n";
        f.close();
        (void)me::oeffneText("ui/modprobe.menu",
                             "{\n menuDef\n {\n  name modprobe\n  rect 0 0 640 480\n  visible 1\n"
                             "  itemDef\n  {\n   name a\n   glScale 1.5\n   rect 10 10 50 20\n   visible 1\n  }\n"
                             "  itemDef\n  {\n   name b\n   type ITEM_TYPE_OWNERDRAW\n   ownerdraw 300\n   rect 100 10 80 40\n   visible 1\n  }\n }\n}\n");
        me::setzeReiter(2);
    }));
    add(pause(0.3));
    add(pruefe("M5b Profil: ohne Profil (das eingebaute des Spiels) meldet der Parser glScale wie die Engine", [st] {
        const char* const spiele[] = {"Jedi Academy", "Jedi Outcast", "Movie Duels"};
        const auto x = st();
        return x.meldungen > 0 && x.spiel >= 0 && x.spiel < 3 && me::modProfil() == spiele[x.spiel];
    }));
    add(tu("M5b Profil: Profil Selbsttest Mod waehlen, a waehlen", [] {
        me::setzeModProfil("Selbsttest Mod");
        me::waehle(me::itemNummer("a"), false);
    }));
    add(pause(0.3));
    add(pruefe("M5b Profil: mit Profil keine Meldung, Mod-Schluessel beschriftet in den Eigenschaften", [st] {
        char kopf[200];
        std::snprintf(kopf, sizeof(kopf), tr(Str::MeProfilSchluessel), "Selbsttest Mod");
        return me::modProfil() == "Selbsttest Mod" && st().meldungen == 0 && finde(kopf, "", 0, false, "") != nullptr;
    }));
    add(foto("menue_profil"));
    add(tu("M5b Profil: Vorlage fuer Galactic Legacy anlegen", [profilOrdner] {
        (void)me::legeProfilVorlageAn(pfadText(*profilOrdner / "galactic_legacy.jksprofile"));
        me::setzeModProfil("galactic_legacy");
    }));
    add(pruefe("M5b Profil: Vorlage geschrieben und als Profil waehlbar", [profilOrdner] {
        std::error_code ec;
        return fs::is_regular_file(*profilOrdner / "galactic_legacy.jksprofile", ec) && me::modProfil() == "galactic_legacy";
    }));
    add(tu("M5b Profil: zurueck zum Profil des Spiels, schliessen", [] {
        me::setzeModProfil("");
        me::schliesseAlleOhneSpeichern();
    }));
    return s;
}

// M5b Punkt 6: Tour "Dein erstes eigenes Menue" (tut-menue) mit ECHTEN Aktionen; vor jeder Aktion
// die Gegenprobe (der Schritt ist noch nicht erledigt, auch nicht durch eine andere Aktion), danach
// muss die Tour selbst weiterschalten. Die Bilder h_tut_menue_1..6 sind die des Tutorials.
std::vector<Schritt> schritteTourMenue() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    namespace fs = std::filesystem;
    const auto datei = std::make_shared<std::string>(pfadText(bhed::utf8Pfad(platform::executableDirectory()) / "tour_projekt" / "erstes.jksmenu"));
    auto erledigt = [&](int n) {
        add(warteBis("M5b Tour: Schritt " + std::to_string(n) + " hat sich selbst erledigt", [n] {
            const TourSicht t = tourSicht();
            return t.id == "tut-menue" && (t.fertig || t.schritt >= n);
        }, 300));
        add(pause(0.3));
    };
    auto offen = [&](int n, const char* was) {
        add(pruefe(std::string("M5b Tour Gegenprobe: Schritt ") + std::to_string(n + 1) + " noch offen (" + was + ")", [n] {
            const TourSicht t = tourSicht();
            return t.aktiv && !t.fertig && t.schritt == n;
        }));
    };
    add(tu("M5b Tour: alles zu, Skript-Modus, Tour tut-menue starten", [datei] {
        std::error_code ec;
        fs::remove_all(fs::path(bhed::utf8Pfad(*datei)).parent_path(), ec);
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 0;
        (void)tourStarten("tut-menue", 0);
    }));
    add(pause(0.4));
    add(pruefe("M5b Tour: laeuft mit 6 Schritten", [] {
        const TourSicht t = tourSicht();
        return t.aktiv && t.id == "tut-menue" && t.anzahl == 6;
    }));
    offen(0, "Skript-Modus");
    add(klick(tr(Str::ViewMenuEditor), "###main"));
    erledigt(1);
    add(foto("h_tut_menue_1"));
    offen(1, "Menue-Editor ohne Projekt");
    add(tu("M5b Tour: Neues Projekt (wie der Knopf nach dem Dateidialog)", [datei] {
        std::error_code ec;
        fs::create_directories(fs::path(bhed::utf8Pfad(*datei)).parent_path(), ec);
        (void)me::legeProjektAn(*datei);
    }));
    erledigt(2);
    add(foto("h_tut_menue_2"));
    add(pruefe("M5b Tour: das Projekt hat ein leeres Vollbild-Menue (ein Element: der Hintergrund)", [] {
        return !me::projektDatei().empty() && me::tourWerte().elemente == 1;
    }));
    offen(2, "noch kein Knopf");
    add(klick(tr(Str::MeWButton)));
    erledigt(3);
    add(klick(tr(Str::MeWButton)));
    add(pause(0.2));
    add(klick(tr(Str::MeWButton)));
    add(pause(0.3));
    add(tu("M5b Tour: Reiter Eigenschaften", [] { me::setzeReiter(2); }));
    add(foto("h_tut_menue_3"));
    add(tu("M5b Tour: die drei Knoepfe waehlen (Strg+Klick)", [] {
        me::waehle(1, false);
        me::waehle(2, true);
        me::waehle(3, true);
    }));
    add(pause(0.3));
    offen(3, "nur gewaehlt, noch kein Layout");
    add(klick(tr(Str::MeLayoutWaagrecht)));
    erledigt(4);
    add(pruefe("M5b Tour: die drei (vorher uebereinander liegenden) Knoepfe stehen nebeneinander, ohne sich zu ueberdecken", [] {
        const auto x = me::stand();
        bhed::menue::LeseOptionen o;
        o.dialekt = bhed::menue::Dialekt::SP;
        const bhed::menue::Datei d = bhed::menue::lies("ui/mein_menue.menu", x.text, o);
        if (d.menus.size() != 1 || d.menus[0].items.size() != 4) { return false; }
        const auto& it = d.menus[0].items;
        const auto& a = it[1].window.rectClient;
        const auto& b = it[2].window.rectClient;
        const auto& c = it[3].window.rectClient;
        diag::info("Menue-Editor M5b Tour Layout: " + std::to_string(a.x) + "/" + std::to_string(a.w) + " " + std::to_string(b.x) + "/" +
                   std::to_string(b.w) + " " + std::to_string(c.x) + "/" + std::to_string(c.w));
        return a.w > 0 && b.x >= a.x + a.w + 8 - 0.5F && c.x >= b.x + b.w + 8 - 0.5F && std::fabs(a.w - b.w) < 0.5F && a.x >= 0 &&
               c.x + c.w <= 640.5F;
    }));
    add(foto("h_tut_menue_4"));
    add(tu("M5b Tour: ersten Knopf waehlen, Reiter Ereignisse", [] {
        me::waehle(1, false);
        me::setzeReiter(3);
    }));
    add(pause(0.3));
    offen(4, "Reiter Ereignisse offen, noch kein Effekt");
    add(klick(tr(Str::MeEvLeuchten)));
    erledigt(5);
    add(foto("h_tut_menue_5"));
    add(tu("M5b Tour: zurueck zum Reiter Eigenschaften", [] { me::setzeReiter(2); }));
    add(pause(0.2));
    offen(5, "noch nicht gespeichert");
    add(klick(tr(Str::MeSpeichern)));
    erledigt(6);
    add(pruefe("M5b Tour: ganz durch; Projektdatei und Menue mit Layout und Leuchten auf der Platte", [datei] {
        const fs::path menu = fs::path(bhed::utf8Pfad(*datei)).parent_path() / "ui" / "mein_menue.menu";
        std::ifstream f(menu, std::ios::binary);
        std::ostringstream t;
        t << f.rdbuf();
        std::error_code ec;
        return tourSicht().fertig && fs::is_regular_file(bhed::utf8Pfad(*datei), ec) && t.str().find("jkstudio:layout") != std::string::npos &&
               t.str().find("_glow") != std::string::npos;
    }));
    add(foto("h_tut_menue_6"));
    add(tu("M5b Tour: schliessen, Projekt zu", [] {
        tourSchliessen();
        me::schliesseAlleOhneSpeichern();
    }));
    return s;
}

std::vector<Schritt> schritteM5b() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = [] { return me::stand(); };
    add(tu("M5b Zustaende: Probe oeffnen, Reiter Eigenschaften", [] {
        (void)me::oeffneText("ui/zustandprobe.menu", kZustandProbe);
        me::setzeReiter(2);
    }));
    add(pause(0.3));
    add(tu("M5b Zustaende: panel, knopf1..3 waehlen", [] {
        for (const char* n : {"panel", "knopf1", "knopf2", "knopf3"}) { me::waehle(me::itemNummer(n), true); }
    }));
    add(pause(0.2));
    add(klick(tr(Str::MeLayoutWaagrecht)));
    add(pause(0.3));
    add(klick(tr(Str::MeLayoutVersteckt)));
    add(pause(0.3));
    add(pruefe("M5b Zustaende: zeig, weg und onOpen bekommen setitemrect; nach jedem Klick liegen die Knoepfe wie im Layout", [st] {
        const auto x = st();
        bhed::menue::LeseOptionen o;
        o.dialekt = bhed::menue::Dialekt::SP;
        const bhed::menue::Datei d = bhed::menue::lies("ui/zustandprobe.menu", x.text, o);
        if (!d.meldungen.empty() || d.menus.size() != 1 || d.menus[0].items.size() != 6) { return false; }
        const auto& m = d.menus[0];
        bhed::menue::Laufzeit lz;
        lz.start(d);
        lz.oeffne("zustandprobe");
        auto lage = [&lz](std::size_t i) {
            const auto& r = lz.rects[0][i];
            return std::to_string(static_cast<int>(r.x)) + " " + std::to_string(static_cast<int>(r.y));
        };
        const std::string grund = lage(1) + "|" + lage(3);
        lz.fuehreAus(0, 4, m.items[4].action);
        const std::string mit = lage(1) + "|" + lage(2) + "|" + lage(3);
        lz.fuehreAus(0, 5, m.items[5].action);
        const std::string ohne = lage(3);
        diag::info("Menue-Editor M5b Zustaende: Grund " + grund + ", nach zeig " + mit + ", nach weg knopf3 " + ohne);
        return grund == "108 108|176 108" && mit == "108 108|176 108|244 108" && ohne == "176 108" && m.onOpen.find("setitemrect") != std::string::npos;
    }));
    add(foto("menue_zustaende"));
    add(tu("M5b Zustaende: Rueckgaengig bis zum Original", [] {
        for (int i = 0; i < 2; ++i) { (void)me::rueckgaengig(); }
    }));
    add(pause(0.2));
    add(pruefe("M5b Zustaende: nach zwei Rueckgaengig byte-gleich mit dem Original", [st] { return st().text == kZustandProbe; }));
    for (auto& x : schritteM5bProjekt()) { add(std::move(x)); }
    for (auto& x : schritteTourMenue()) { add(std::move(x)); }
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
    // M3: eigene Liste BHED_MENUE_VERGLEICH_MENUES="ui/jamp/ingame.menu=vgl_ingame,..." (leer: keine)
    std::vector<std::pair<std::string, std::string>> menues = {{"ui/main.menu", "vergleich_sp"},
                                                               {"ui/jamp/main.menu", "vergleich_mp"},
                                                               {"ui/character.menu", "vergleich_figur"},
                                                               {"ui/saber.menu", "vergleich_schwert"}};
    if (const char* eigene = std::getenv("BHED_MENUE_VERGLEICH_MENUES")) {
        menues.clear();
        const std::string l = eigene;
        for (std::size_t a = 0; a < l.size();) {
            std::size_t e = l.find(',', a);
            if (e == std::string::npos) { e = l.size(); }
            const std::string eintrag = l.substr(a, e - a);
            a = e + 1;
            const std::size_t gl = eintrag.find('=');
            if (gl != std::string::npos) { menues.emplace_back(eintrag.substr(0, gl), eintrag.substr(gl + 1)); }
        }
    }
    for (const auto& m : menues) {
        const std::string pfad = m.first, name = m.second;
        // im Bedienmodus: onOpen laeuft (setfocus), Zeit und Videos wie im Spiel
        add(tu("Vergleich: " + pfad, [pfad] {
            me::setzeInteraktion(false);
            // wie +set im Spiel: BHED_MENUE_VERGLEICH_CVAR=name=wert (z. B. cg_SFXSabers=0)
            if (const char* c = std::getenv("BHED_MENUE_VERGLEICH_CVAR")) {
                const std::string nw = c;
                const std::size_t gl = nw.find('=');
                if (gl != std::string::npos) { me::setzeCvar(nw.substr(0, gl), nw.substr(gl + 1)); }
            }
            // M4 Punkt 2: "ui/x.menu|<datei>" = das Menue mit dem Text aus der Datei (Bedienzustand:
            // onOpen fuehrt die Aktionen aus, das Spiel bekommt denselben Text als Testpaket)
            if (const std::size_t st = pfad.find('|'); st != std::string::npos) {
                std::ifstream f(std::filesystem::path(pfad.substr(st + 1)), std::ios::binary);
                std::ostringstream t;
                t << f.rdbuf();
                (void)me::oeffneText(pfad.substr(0, st), t.str());
            } else {
                (void)me::oeffneAusSpiel(pfad);
            }
            me::setzeInteraktion(true);
        }));
        add(pause(2.5));
        add(foto(name));
        add(pruefe("Vergleich: " + pfad + " gezeichnet", [name] {
            const auto x = me::stand();
            char b[300];
            std::snprintf(b, sizeof(b), "Menue-Editor Vergleich %s: Flaeche %.1f %.1f %.1f %.1f, %d Zeichenbefehle, %d Texturen, Bildzeit %.2f ms, %d Figurenaufrufe %s",
                          name.c_str(), static_cast<double>(x.flaeche[0]), static_cast<double>(x.flaeche[1]), static_cast<double>(x.flaeche[2]),
                          static_cast<double>(x.flaeche[3]), x.prims, x.bilderGeladen, x.bildzeitMs, x.modellAufrufe, x.modellFehler.c_str());
            diag::info(b);
            return x.prims > 0;
        }));
    }
    // M3 Punkt 6: Saebelmenue je Klingenart und Farbe wie im Spiel per Konsole gesetzt:
    // BHED_MENUE_VERGLEICH_SAEBEL="4:blue,1:blue,4:black" (cg_SFXSabers:ui_saber_color)
    if (const char* liste = std::getenv("BHED_MENUE_VERGLEICH_SAEBEL")) {
        std::string l = liste;
        for (std::size_t a = 0; a < l.size();) {
            std::size_t e = l.find(',', a);
            if (e == std::string::npos) { e = l.size(); }
            const std::string eintrag = l.substr(a, e - a);
            a = e + 1;
            const std::size_t dp = eintrag.find(':');
            if (dp == std::string::npos) { continue; }
            const std::string art = eintrag.substr(0, dp), farbe = eintrag.substr(dp + 1);
            add(tu("Vergleich Saebel: cg_SFXSabers " + art + ", " + farbe, [art, farbe] {
                me::setzeInteraktion(false);
                me::setzeCvar("cg_SFXSabers", art);
                me::setzeCvar("g_saber_color", farbe);   // das Menue holt die Farbe beim Oeffnen daraus
                me::setzeCvar("ui_saber_color", farbe);
                (void)me::oeffneAusSpiel("ui/saber.menu");
                me::setzeInteraktion(true);
            }));
            add(pause(2.0));
            add(foto("vergleich_saebel_" + art + "_" + farbe));
            add(pruefe("Vergleich Saebel " + art + " " + farbe + ": Klingen gezeichnet", [] {
                const auto x = me::stand();
                char b[200];
                std::snprintf(b, sizeof(b), "Menue-Editor Vergleich Saebel: Flaeche %.1f %.1f %.1f %.1f, %d Aufrufe ", static_cast<double>(x.flaeche[0]),
                              static_cast<double>(x.flaeche[1]), static_cast<double>(x.flaeche[2]), static_cast<double>(x.flaeche[3]), x.modellAufrufe);
                diag::info(b + x.modellFehler);
                return x.modellFehler.empty() || x.modellFehler.find("blendFunc") == std::string::npos;
            }));
        }
    }
    add(tu("Vergleich: zurueck", [] {
        me::setzeInteraktion(false);
        g_app->leftMode = 0;
    }));
    return s;
}

// M6: Menuefluss (Punkt 1), Tab-Reihenfolge (Punkt 2), Referenzbild (Punkt 7), Rueckfrage beim
// Schliessen (Rest M5b). Hauptmenue -> optionen (mit Zurueck) und credits (Sackgasse), waise nie geoeffnet.
const char* const kFlussProbe =
    "{\n\tmenuDef\n\t{\n\t\tname mainMenu\n\t\trect 0 0 640 480\n\t\tvisible 1\n"
    "\t\titemDef\n\t\t{\n\t\t\tname opt\n\t\t\ttype ITEM_TYPE_BUTTON\n\t\t\ttext \"Optionen\"\n\t\t\trect 100 100 120 30\n\t\t\tvisible 1\n"
    "\t\t\taction { open optionen }\n\t\t}\n"
    "\t\titemDef\n\t\t{\n\t\t\tname cred\n\t\t\ttype ITEM_TYPE_BUTTON\n\t\t\ttext \"Credits\"\n\t\t\trect 100 150 120 30\n\t\t\tvisible 1\n"
    "\t\t\taction { open credits }\n\t\t}\n\t}\n"
    "\tmenuDef\n\t{\n\t\tname optionen\n\t\trect 0 0 640 480\n\t\tvisible 1\n"
    "\t\titemDef\n\t\t{\n\t\t\tname zurueck\n\t\t\ttype ITEM_TYPE_BUTTON\n\t\t\ttext \"Zurueck\"\n\t\t\trect 40 400 120 30\n\t\t\tvisible 1\n"
    "\t\t\taction { close optionen }\n\t\t}\n"
    "\t\titemDef\n\t\t{\n\t\t\tname a\n\t\t\ttype ITEM_TYPE_BUTTON\n\t\t\ttext \"A\"\n\t\t\trect 100 100 120 30\n\t\t\tvisible 1\n"
    "\t\t\taction { play \"sound/interface/button1.wav\" }\n\t\t}\n"
    "\t\titemDef\n\t\t{\n\t\t\tname b\n\t\t\ttype ITEM_TYPE_BUTTON\n\t\t\ttext \"B\"\n\t\t\trect 100 160 120 30\n\t\t\tvisible 1\n"
    "\t\t\taction { play \"sound/interface/button1.wav\" }\n\t\t}\n"
    "\t\titemDef\n\t\t{\n\t\t\tname c\n\t\t\ttype ITEM_TYPE_BUTTON\n\t\t\ttext \"C\"\n\t\t\trect 100 220 120 30\n\t\t\tvisible 1\n"
    "\t\t\taction { play \"sound/interface/button1.wav\" }\n\t\t}\n\t}\n"
    "\tmenuDef\n\t{\n\t\tname credits\n\t\trect 0 0 640 480\n\t\tvisible 1\n"
    "\t\titemDef\n\t\t{\n\t\t\tname weiter\n\t\t\ttype ITEM_TYPE_BUTTON\n\t\t\ttext \"Weiter\"\n\t\t\trect 100 100 120 30\n\t\t\tvisible 1\n"
    "\t\t\taction { play \"sound/interface/button1.wav\" }\n\t\t}\n\t}\n"
    "\tmenuDef\n\t{\n\t\tname waise\n\t\trect 0 0 640 480\n\t\tvisible 1\n"
    "\t\titemDef\n\t\t{\n\t\t\tname zu\n\t\t\ttype ITEM_TYPE_BUTTON\n\t\t\ttext \"Zu\"\n\t\t\trect 100 100 120 30\n\t\t\tvisible 1\n"
    "\t\t\taction { close waise }\n\t\t}\n\t}\n"
    "}\n";

// Echter Mausklick auf die Flaeche an (vx, vy) im 640x480-Bildschirm
Schritt klickFlaeche(const std::string& was, float vx, float vy) {
    return Schritt{was, [=](int b) {
                       ImGuiIO& io = ImGui::GetIO();
                       if (b == 0) {
                           const auto x = me::stand();
                           setzeMaus(ImVec2{x.flaeche[0] + vx / 640.0F * x.flaeche[2], x.flaeche[1] + vy / 480.0F * x.flaeche[3]});
                           return false;
                       }
                       if (b == 2) {
                           io.AddMouseButtonEvent(0, true);
                           return false;
                       }
                       if (b == 3) {
                           io.AddMouseButtonEvent(0, false);
                           return false;
                       }
                       return b >= 6;
                   }};
}

std::vector<Schritt> schritteM6() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = [] { return me::stand(); };
    add(tu("M6 Fluss: Probe oeffnen", [] {
        me::schliesseAlleOhneSpeichern();
        (void)me::oeffneText("ui/flussprobe.menu", kFlussProbe);
    }));
    add(pause(0.3));
    // Aufgabe 1 "Finde das Menue ohne Ausgang": Fluss oeffnen, auf die Sackgasse klicken (2 Schritte, ohne Text-Reiter)
    add(klick(tr(Str::MeFluss)));
    add(pause(0.4));
    add(pruefe("M6 Fluss: Fenster offen, 4 Menues, waise unerreichbar, credits Sackgasse, optionen mit Zurueck", [] {
        const auto f = me::flussInfo();
        char b[200];
        std::snprintf(b, sizeof(b), "Menue-Editor M6 Fluss: %d Menues, %d unerreichbar, %d Sackgassen, %d ohne Zurueck, %d fehlen, %d Kanten", f.menues,
                      f.unerreichbar, f.sackgassen, f.ohneZurueck, f.fehlend, f.kanten);
        diag::info(b);
        return f.fensterOffen && f.menues == 4 && f.unerreichbar == 1 && f.sackgassen == 1 && f.ohneZurueck == 0 && f.fehlend == 0 && f.kanten == 2;
    }));
    add(foto("menue_fluss"));
    add(klick("credits", "menuefluss"));
    add(pause(0.3));
    add(pruefe("M6 Fluss: Klick auf credits zeigt das Menue (2 Schritte, ohne Text-Reiter)", [] { return me::flussInfo().aktivesMenue == "credits"; }));
    add(tu("M6 Fluss: credits bekommt einen Zurueck-Knopf", [] {
        me::fuegeItemsEin("itemDef { name zurueck type ITEM_TYPE_BUTTON text \"Zurueck\" rect 40 400 120 30 visible 1 action { close credits } }");
    }));
    add(pause(0.3));
    add(pruefe("M6 Fluss: nach dem Zurueck-Knopf keine Sackgasse mehr", [] { return me::flussInfo().sackgassen == 0; }));
    // Aufgabe 2 "Tab-Reihenfolge c, b, a, zurueck": optionen, Schalter, vier Klicks auf die Flaeche
    add(klick("optionen", "menuefluss"));
    add(pause(0.3));
    add(klick(tr(Str::MeFluss)));   // Fenster wieder zu: die Flaeche frei fuer die Klicks
    add(pause(0.2));
    add(klick(tr(Str::MeTabFolge)));
    add(pause(0.3));
    add(pruefe("M6 Tab: optionen, Schalter an, Reihenfolge wie Menu_SetNextCursorItem (a b c zurueck)", [] {
        const auto t = me::tabFolge();
        std::string z;
        for (const auto& n : t) { z += n + " "; }
        diag::info("Menue-Editor M6 Tab-Folge: " + z);
        return me::flussInfo().aktivesMenue == "optionen" && me::tabFolgeAn() && t == std::vector<std::string>{"a", "b", "c", "zurueck"};
    }));
    add(foto("menue_tabfolge"));
    auto vorher = std::make_shared<std::string>();
    add(tu("M6 Tab: Text vorher merken", [vorher] { *vorher = me::stand().text; }));
    add(klickFlaeche("M6 Tab: Klick auf c", 160, 235));
    add(klickFlaeche("M6 Tab: Klick auf b", 160, 175));
    add(klickFlaeche("M6 Tab: Klick auf a", 160, 115));
    add(klickFlaeche("M6 Tab: Klick auf zurueck", 100, 415));
    add(pause(0.3));
    add(pruefe("M6 Tab: neue Reihenfolge c b a zurueck, Text geaendert, Parser ohne Meldung", [st, vorher] {
        const auto t = me::tabFolge();
        std::string z;
        for (const auto& n : t) { z += n + " "; }
        diag::info("Menue-Editor M6 Tab-Folge neu: " + z);
        return t == std::vector<std::string>{"c", "b", "a", "zurueck"} && st().text != *vorher && st().meldungen == 0;
    }));
    add(tu("M6 Tab: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
    add(pause(0.2));
    add(pruefe("M6 Tab: ein Rueckgaengig-Schritt stellt den Text byte-gleich zurueck", [st, vorher] { return st().text == *vorher; }));
    add(klick(tr(Str::MeTabFolge)));
    add(pause(0.2));
    add(pruefe("M6 Tab: Schalter aus", [] { return !me::tabFolgeAn(); }));
    // Referenzbild: kleines TGA (24 Bit, unkomprimiert) laden; der Menuetext bleibt unberuehrt
    add(tu("M6 Referenzbild: TGA schreiben und laden", [vorher] {
        *vorher = me::stand().text;
        std::string tga(18, '\0');
        tga[2] = 2;
        tga[12] = 64;
        tga[14] = 48;
        tga[16] = 24;
        for (int i = 0; i < 64 * 48; ++i) {
            tga += static_cast<char>(0x30);
            tga += static_cast<char>((i % 64) * 4);
            tga += static_cast<char>(0xc0);
        }
        const std::string p = pfadText(bhed::utf8Pfad(platform::executableDirectory()) / "referenz_probe.tga");
        (void)spewDatei(p, tga);
        me::ladeReferenzbild(p);
    }));
    add(pause(0.3));
    add(pruefe("M6 Referenzbild: geladen, Menuetext unveraendert", [st, vorher] { return me::referenzbildGeladen() && st().text == *vorher; }));
    add(foto("menue_referenzbild"));
    // Rest M5b: ein geaenderter Reiter fragt beim Schliessen
    add(tu("M6 Schliessen: Text aendern, Reiter schliessen", [] {
        me::waehle(me::itemNummer("a"), false);
        me::verschiebeAuswahl(4, 0);
        me::schliesseReiter(0);
    }));
    add(pause(0.3));
    add(pruefe("M6 Schliessen: Rueckfrage offen, Reiter noch da", [st] { return me::schliessenFrageOffen() && st().dokumente == 1; }));
    add(klick(tr(Str::MeSchliessenVerwerfen)));
    add(pause(0.3));
    add(pruefe("M6 Schliessen: ohne Speichern geschlossen", [st] { return !me::schliessenFrageOffen() && st().dokumente == 0; }));
    // Punkt 5: Vorlage "Einstellungsseite mit Reitern" ueber das + der Seiten (2 Klicks)
    add(tu("M6 Vorlagen: Probe oeffnen", [] { (void)me::oeffneText("ui/vorlagenprobe.menu", kFlussProbe); }));
    add(pause(0.3));
    add(klick("+"));
    add(pause(0.2));
    add(klick(tr(Str::MeVorlageEinst)));
    add(pause(0.3));
    add(pruefe("M6 Vorlagen: settingsMenu mit drei Reitern angelegt, Parser ohne Meldung", [st] {
        const auto x = st();
        return me::menuesImDokument() == 5 && me::flussInfo().aktivesMenue == "settingsMenu" && x.meldungen == 0 && x.seiten >= 7;
    }));
    add(foto("menue_vorlage"));
    // Punkt 4: Stile-Fenster
    add(klick(tr(Str::MeStile)));
    add(pause(0.3));
    add(pruefe("M6 Stile: Fenster offen", [] { return me::flussInfo().stileOffen; }));
    add(foto("menue_stile"));
    add(klick(tr(Str::MeStile)));
    add(pause(0.2));
    // Punkt 6: Assistent - zwei Einstellungen aus dem Katalog der Spielmenues, Seite anlegen
    add(klick(tr(Str::MeAssistent)));
    add(pause(0.5));
    auto zwei = std::make_shared<std::vector<std::string>>();
    std::vector<char> anlegen(128);
    std::snprintf(anlegen.data(), anlegen.size(), tr(Str::MeAssistentAnlegen), 2);
    const std::string anlegenText = anlegen.data();
    add(pruefe("M6 Assistent: Katalog aus den Spielmenues (Schieber, Ja/Nein, Auswahl)", [zwei] {
        const auto k = me::assistentKatalog();
        diag::info("Menue-Editor M6 Assistent: " + std::to_string(k.size()) + " Cvars mit Steuerung");
        if (k.size() >= 2) { *zwei = {k[0], k[1]}; }
        return me::flussInfo().assistentOffen && k.size() >= 2;
    }));
    add(tu("M6 Assistent: zwei Einstellungen ankreuzen", [zwei] {
        for (const auto& c : *zwei) { me::assistentWaehle(c); }
    }));
    add(pause(0.2));
    add(foto("menue_assistent"));
    add(klick(anlegenText));
    add(pause(0.3));
    add(pruefe("M6 Assistent: neue Seite mit den zwei Einstellungen, Parser ohne Meldung", [st] {
        return me::menuesImDokument() == 6 && me::flussInfo().aktivesMenue == "settingsMenu2" && st().meldungen == 0;
    }));
    add(klick(tr(Str::MeAssistent)));
    add(pause(0.2));
    // Punkt 8: Element aus dem Spiel verschieben, Unterschied sehen, auf Original zuruecksetzen
    auto original = std::make_shared<std::string>();
    add(tu("M6 Original: Hauptmenue aus dem Spiel, erstes benanntes Element waehlen und verschieben", [original] {
        me::schliesseAlleOhneSpeichern();
        (void)me::oeffneAusSpiel("ui/main.menu");
        me::setzeReiter(2);
        *original = me::stand().text;
        for (int i = 0; i < 200; ++i) {
            if (me::itemNameBei(i).empty()) { continue; }
            me::waehle(i, false);
            break;
        }
        me::verschiebeAuswahl(6, 0);
    }));
    add(pause(0.4));
    add(pruefe("M6 Original: Unterschied rect erkannt", [] {
        const auto u = me::originalUnterschiede();
        return u == std::vector<std::string>{"rect"};
    }));
    add(foto("menue_vergleich"));
    add(klick(tr(Str::MeOriginalZuruecksetzen)));
    add(pause(0.3));
    add(pruefe("M6 Original: zurueckgesetzt, Text wieder wie im Spiel", [st, original] { return st().text == *original && me::originalUnterschiede().empty(); }));
    add(tu("M6: aufraeumen", [] { me::schliesseAlleOhneSpeichern(); }));
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
    // BHED_MENUE_NUR=m3: nur die Schritte von M3 (gezielter Rot/Gruen-Test)
    if (const char* nur = std::getenv("BHED_MENUE_NUR"); nur != nullptr && std::strcmp(nur, "m3") == 0) {
        for (auto& x : schritteM3()) { add(std::move(x)); }
        add(tu("Zurueck zum Skript-Modus", [] { g_app->leftMode = 0; }));
        return s;
    }
    // BHED_MENUE_NUR=m5: nur die Schritte von M5
    if (const char* nur = std::getenv("BHED_MENUE_NUR"); nur != nullptr && std::strcmp(nur, "m5") == 0) {
        for (auto& x : schritteM5()) { add(std::move(x)); }
        add(tu("Zurueck zum Skript-Modus", [] { g_app->leftMode = 0; }));
        return s;
    }
    // BHED_MENUE_NUR=jk2test (M5b Punkt 5, mit Spieldaten von Jedi Outcast): Hauptmenue mit einem
    // auffaelligen Text, Testpaket und Befehlszeile von "Im Spiel testen" (gestartet wird von aussen)
    if (const char* nur = std::getenv("BHED_MENUE_NUR"); nur != nullptr && std::strcmp(nur, "jk2test") == 0) {
        add(tu("M5b JK2: Hauptmenue aus dem Spiel, Text JK STUDIO TEST einfuegen", [] {
            (void)me::oeffneAusSpiel("ui/main.menu");
            me::fuegeItemsEin("itemDef { name jkstudio_test text \"JK STUDIO TEST\" rect 200 300 240 40 textscale 1 forecolor 1 0.2 0.2 1 "
                              "decoration visible 1 }");
        }));
        add(pause(0.3));
        add(pruefe("M5b JK2: Jedi Outcast geladen, Im Spiel testen liefert Testpaket und Befehlszeile ueber fs_basepath + fs_cdpath", [] {
            const std::string b = me::imSpielTesten();
            diag::info("Menue-Editor M5b JK2 Befehl: " + b);
            return me::stand().spiel == 1 && b.find("jk2sp.exe") != std::string::npos && b.find("+set fs_basepath") != std::string::npos &&
                   b.find("+set fs_cdpath") != std::string::npos;
        }));
        add(tu("Zurueck zum Skript-Modus", [] { g_app->leftMode = 0; }));
        return s;
    }
    // BHED_MENUE_NUR=m5b: nur die Schritte von M5b
    if (const char* nur = std::getenv("BHED_MENUE_NUR"); nur != nullptr && std::strcmp(nur, "m5b") == 0) {
        for (auto& x : schritteM5b()) { add(std::move(x)); }
        add(tu("Zurueck zum Skript-Modus", [] { g_app->leftMode = 0; }));
        return s;
    }
    // BHED_MENUE_NUR=m6: nur die Schritte von M6
    if (const char* nur = std::getenv("BHED_MENUE_NUR"); nur != nullptr && std::strcmp(nur, "m6") == 0) {
        for (auto& x : schritteM6()) { add(std::move(x)); }
        add(tu("Zurueck zum Skript-Modus", [] { g_app->leftMode = 0; }));
        return s;
    }
    // BHED_MENUE_NUR=m4: nur die Schritte von M4
    if (const char* nur = std::getenv("BHED_MENUE_NUR"); nur != nullptr && std::strcmp(nur, "m4") == 0) {
        for (auto& x : schritteM4()) { add(std::move(x)); }
        add(tu("Zurueck zum Skript-Modus", [] { g_app->leftMode = 0; }));
        return s;
    }
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
    // Echte Eingabe in den Code-Editor: Reiter Text (rechte Spalte), Klick in die Flaeche, Zeichen, Strg+Z
    add(tu("Reiter Text", [] { me::setzeReiter(1); }));
    add(pause(0.2));
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
    for (auto& x : schritteM3()) { add(std::move(x)); }
    for (auto& x : schritteM4()) { add(std::move(x)); }
    for (auto& x : schritteM5()) { add(std::move(x)); }
    for (auto& x : schritteM5b()) { add(std::move(x)); }
    for (auto& x : schritteM6()) { add(std::move(x)); }
    add(tu("Zurueck zum Skript-Modus", [] { g_app->leftMode = 0; }));
    return s;
}

}  // namespace menuetest
