// selbsttest_menue_pruefung.inc - rc608 Teil 3b: Gesamtpruefung des Menue-Editors mit echten Klicks (BHED_MENUE_NUR=kontext, datei, seiten ...).
// Eingebunden in namespace menuetest (selbsttest_menue.inc). Geprueft wird das Ergebnis: Text der Datei, Auswahl, Datei auf der Platte.

const char* const kKontextText =
    "menuDef\n{\n  name \"kontext\"\n  rect 0 0 640 480\n  fullScreen 1\n  visible 1\n"
    "  itemDef\n  {\n    name \"grund\"\n    rect 0 0 640 480\n    style WINDOW_STYLE_FILLED\n    backcolor 0.1 0.1 0.2 1\n    decoration\n    visible 1\n  }\n"
    "  itemDef\n  {\n    name \"a\"\n    type ITEM_TYPE_BUTTON\n    rect 100 100 100 40\n    text \"A\"\n    textscale 0.7\n    visible 1\n  }\n"
    "  itemDef\n  {\n    name \"b\"\n    type ITEM_TYPE_BUTTON\n    rect 300 100 100 40\n    text \"B\"\n    textscale 0.7\n    visible 1\n  }\n"
    "  itemDef\n  {\n    name \"c\"\n    type ITEM_TYPE_BUTTON\n    rect 100 300 100 40\n    text \"C\"\n    textscale 0.7\n    visible 1\n  }\n"
    "}\n"
    "menuDef\n{\n  name \"zweite\"\n  rect 0 0 640 480\n  visible 1\n"
    "  itemDef\n  {\n    name \"x\"\n    type ITEM_TYPE_BUTTON\n    rect 50 50 100 40\n    text \"X\"\n    textscale 0.7\n    action { close kontext ; open kontext }\n    visible 1\n  }\n"
    "  itemDef\n  {\n    name \"y\"\n    type ITEM_TYPE_BUTTON\n    rect 50 150 100 40\n    text \"Y\"\n    textscale 0.7\n    visible 1\n  }\n"
    "}\n";

Schritt zieheVirt(const std::string& was, float x0, float y0, float x1, float y1) { return zieheFlaecheVon(was, x0, y0, x1, y1); }

Schritt klickVirt(const std::string& was, float x, float y, int taste = 0) {
    return klickAn(was, [x, y] {
        const auto st = me::stand();
        return ImVec2{st.flaeche[0] + x / 640.0F * st.flaeche[2], st.flaeche[1] + y / 480.0F * st.flaeche[3]};
    }, taste);
}

// Das erste bedienbare Element, dessen Beschriftung den Text enthaelt
ImVec2 findeMit(const std::string& teil) {
    for (const elemente::Element& e : elemente::alle()) {
        if (e.label.find(teil) != std::string::npos && e.rect.GetHeight() > 0.0F) { return e.rect.GetCenter(); }
    }
    return ImVec2{-1.0F, -1.0F};
}

std::string namenDerAuswahl() {
    std::string r;
    for (const int i : me::stand().auswahl) { r += (r.empty() ? "" : ",") + me::itemNameBei(i); }
    return r;
}

std::vector<std::string> namenInFolge() {
    std::vector<std::string> r;
    for (const me::ItemLage& l : me::itemLagen()) { r.push_back(l.name); }
    return r;
}

std::string folgeText() {
    std::string r;
    for (const std::string& n : namenInFolge()) { r += (r.empty() ? "" : ",") + n; }
    return r;
}

std::vector<Schritt> schritteKontext() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto probe = [](const char* was) {
        return tu(std::string("3b Kontext: Probe, ") + was, [] {
            me::schliesseAlleOhneSpeichern();
            g_app->leftMode = 4;
            tourSchliessen();
            (void)me::oeffneText("ui/kontext.menu", kKontextText);
            me::setzeInteraktion(false);
            me::setzeSpalten(0.0F, 0.0F);
            me::setzeReiter(2);
        });
    };
    auto vor = std::make_shared<std::string>();
    auto wahl = std::make_shared<std::string>();
    auto tiefe = std::make_shared<int>(0);
    const auto merken = [=] {
        return tu("3b Kontext: merken", [=] {
            *vor = me::stand().text;
            *wahl = namenDerAuswahl();
            *tiefe = me::stand().undoSchritte;
        });
    };
    // Ein Schritt, Rueckgaengig byte-gleich samt Auswahl, Wiederholen
    const auto einSchritt = [&](const std::string& was) {
        auto nach = std::make_shared<std::string>();
        auto wahlNach = std::make_shared<std::string>();
        add(pause(0.15));
        add(pruefe("3b Kontext: " + was + ": genau ein Schritt", [=] {
            const auto x = me::stand();
            *nach = x.text;
            *wahlNach = namenDerAuswahl();
            if (x.undoSchritte != *tiefe + 1) { diag::info("Knopftest: " + was + ": " + std::to_string(x.undoSchritte - *tiefe) + " Schritte"); }
            return x.undoSchritte == *tiefe + 1 && x.text != *vor;
        }));
        add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
        add(pause(0.15));
        add(pruefe("3b Kontext: " + was + ": Rueckgaengig byte-gleich mit Auswahl", [=] {
            if (namenDerAuswahl() != *wahl) { diag::info("Knopftest: Auswahl " + namenDerAuswahl() + " statt " + *wahl); }
            return me::stand().text == *vor && namenDerAuswahl() == *wahl;
        }));
        add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Y, "Strg+Y"));
        add(pause(0.15));
        add(pruefe("3b Kontext: " + was + ": Wiederholen byte-gleich", [=] { return me::stand().text == *nach && namenDerAuswahl() == *wahlNach; }));
        add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
        add(pause(0.15));
    };

    // --- Hintergrund laesst Klicks durch
    add(probe("Hintergrund"));
    add(pause(0.5));
    add(klickVirt("3b Kontext: Klick auf den Hintergrund", 500, 400));
    add(pause(0.2));
    add(pruefe("3b Kontext: Der Hintergrund bleibt beim Klick ungewaehlt", [] { return namenDerAuswahl().empty(); }));
    add(zieheVirt("3b Kontext: Zug auf dem Hintergrund", 500, 400, 560, 450));
    add(pause(0.2));
    add(pruefe("3b Kontext: Der Hintergrund bewegt sich nicht", [] { return rectVon("grund") == "0 0 640 480" && me::stand().undoSchritte == 0; }));
    add(tu("3b Kontext: Hierarchie waehlt ihn", [] { me::waehle(0, false); }));
    add(pause(0.1));
    add(pruefe("3b Kontext: Aus der Hierarchie ist er gewaehlt und bearbeitbar", [] { return namenDerAuswahl() == "grund"; }));
    add(zieheVirt("3b Kontext: Zug am gewaehlten Hintergrund", 500, 400, 510, 410));
    add(pause(0.2));
    add(pruefe("3b Kontext: Gewaehlt laesst er sich ziehen", [] { return rectVon("grund") == "10 10 640 480"; }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));

    // --- Rahmen aufziehen
    add(probe("Rahmen"));
    add(pause(0.5));
    add(tu("3b Kontext: nichts gewaehlt", [] { me::waehle(-1, false); }));
    add(merken());
    add(zieheVirt("3b Kontext: Rahmen um a und b", 80, 80, 420, 150));
    add(pause(0.2));
    add(foto("menue_rahmen_auswahl"));
    add(pruefe("3b Kontext: Der Rahmen waehlt a und b, nicht den Hintergrund, kein Undo-Schritt", [=] {
        if (namenDerAuswahl() != "a,b") { diag::info("Knopftest: Rahmen waehlt " + namenDerAuswahl()); }
        return namenDerAuswahl() == "a,b" && me::stand().undoSchritte == *tiefe && me::stand().text == *vor;
    }));
    add(halteTaste(ImGuiMod_Shift, true));
    add(zieheVirt("3b Kontext: Umschalt+Rahmen um c", 80, 280, 220, 360));
    add(halteTaste(ImGuiMod_Shift, false));
    add(pause(0.2));
    add(pruefe("3b Kontext: Umschalt nimmt c dazu", [] { return namenDerAuswahl() == "a,b,c"; }));
    add(zieheVirt("3b Kontext: Die Auswahl gemeinsam ziehen", 150, 120, 170, 130));
    add(pause(0.2));
    add(pruefe("3b Kontext: Alle drei sind gewandert", [] {
        return rectVon("a") == "120 110 100 40" && rectVon("b") == "320 110 100 40" && rectVon("c") == "120 310 100 40" && rectVon("grund") == "0 0 640 480";
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(pruefe("3b Kontext: Ein Schritt nimmt alle drei zurueck", [=] { return me::stand().text == *vor; }));
    add(klickVirt("3b Kontext: Klick ins Leere", 500, 400));
    add(pause(0.2));
    add(pruefe("3b Kontext: Ein Klick ins Leere leert die Auswahl", [] { return namenDerAuswahl().empty(); }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(pause(0.2));
    add(pruefe("3b Kontext: Strg+A waehlt alle ausser dem Hintergrund", [] { return namenDerAuswahl() == "a,b,c"; }));

    // --- Rechtsklick
    add(probe("Rechtsklick"));
    add(pause(0.5));
    add(tu("3b Kontext: nichts gewaehlt", [] { me::waehle(-1, false); }));
    add(klickVirt("3b Kontext: Rechtsklick auf a", 150, 120, 1));
    add(pause(0.2));
    add(foto("menue_kontext"));
    add(pruefe("3b Kontext: Rechtsklick waehlt a und oeffnet das Menue mit allen Befehlen", [] {
        bool alle = namenDerAuswahl() == "a";
        for (const Str t : {Str::ActCut, Str::ActCopy, Str::ActPaste, Str::EntDuplicate, Str::ActDelete, Str::MkGanzVorn, Str::MkGanzHinten, Str::MkSperren, Str::MkAusblenden, Str::MkAlsVorlage}) {
            if (finde(tr(t), "", 0, false) == nullptr) {
                diag::info(std::string("Knopftest: Rechtsklick ohne ") + tr(t));
                alle = false;
            }
        }
        return alle;
    }));
    add(merken());
    add(klick(tr(Str::MkGanzVorn)));
    einSchritt("Ganz nach vorn");
    add(pruefe("3b Kontext: nach Rueckgaengig steht die alte Folge", [] { return folgeText() == "grund,a,b,c"; }));
    add(klickVirt("3b Kontext: Rechtsklick auf b", 350, 120, 1));
    add(pause(0.2));
    add(klick(tr(Str::MkGanzHinten)));
    add(pause(0.2));
    add(pruefe("3b Kontext: b liegt ganz hinten, auch unter dem Hintergrund", [] { return folgeText() == "b,grund,a,c" && namenDerAuswahl() == "b"; }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    // Mehrere zugleich: ihre Folge bleibt, ein Schritt
    add(tu("3b Kontext: a und c gewaehlt", [] {
        me::waehle(1, false);
        me::waehle(3, true);
    }));
    add(pause(0.1));
    add(merken());
    add(klickVirt("3b Kontext: Rechtsklick auf die Auswahl", 150, 120, 1));
    add(pause(0.2));
    add(pruefe("3b Kontext: Rechtsklick auf Gewaehltes laesst die Auswahl", [] { return namenDerAuswahl() == "a,c"; }));
    add(klick(tr(Str::MkGanzVorn)));
    add(pause(0.2));
    add(pruefe("3b Kontext: a und c ganz vorn, ihre Folge bleibt", [] { return folgeText() == "grund,b,a,c" && namenDerAuswahl() == "a,c"; }));
    einSchritt("Mehrere nach vorn");

    add(probe("Kopieren"));
    add(pause(0.5));
    add(tu("3b Kontext: nichts gewaehlt", [] { me::waehle(-1, false); }));
    add(klickVirt("3b Kontext: Rechtsklick auf a", 150, 120, 1));
    add(pause(0.2));
    add(merken());
    add(klick(tr(Str::EntDuplicate)));
    einSchritt("Duplizieren");
    add(klickVirt("3b Kontext: Rechtsklick auf a", 150, 120, 1));
    add(pause(0.2));
    add(klick(tr(Str::ActCopy)));
    add(pause(0.2));
    add(pruefe("3b Kontext: Kopieren aendert nichts", [=] { return me::stand().text == *vor; }));
    add(klickVirt("3b Kontext: Rechtsklick ins Leere", 500, 400, 1));
    add(pause(0.2));
    add(foto("menue_kontext_leer"));
    add(pruefe("3b Kontext: Im Leeren: Hintergrund steht in der Liste, die Auswahl bleibt", [] {
        return finde("grund  (ITEM_TYPE_TEXT)", "", 0, true) != nullptr || finde("grund", "", 0, true) != nullptr;
    }));
    add(merken());
    add(klick(tr(Str::ActPaste)));
    einSchritt("Einfuegen");
    add(klickVirt("3b Kontext: Rechtsklick auf b", 350, 120, 1));
    add(pause(0.2));
    add(merken());
    add(klick(tr(Str::ActCut)));
    einSchritt("Ausschneiden");
    add(klickVirt("3b Kontext: Rechtsklick auf b", 350, 120, 1));
    add(pause(0.2));
    add(merken());
    add(klick(tr(Str::ActDelete)));
    einSchritt("Loeschen");

    // Sperren und Ausblenden: kein Schritt, und sie gehoeren zum Element
    add(probe("Sperren"));
    add(pause(0.5));
    add(tu("3b Kontext: nichts gewaehlt", [] { me::waehle(-1, false); }));
    add(klickVirt("3b Kontext: Rechtsklick auf a", 150, 120, 1));
    add(pause(0.2));
    add(merken());
    add(klick(tr(Str::MkSperren)));
    add(pause(0.2));
    add(pruefe("3b Kontext: Sperren ist kein Schritt", [=] { return me::stand().gesperrt == 1 && me::stand().undoSchritte == *tiefe && me::stand().text == *vor; }));
    add(zieheVirt("3b Kontext: Zug am Gesperrten", 150, 120, 200, 200));
    add(pause(0.2));
    add(pruefe("3b Kontext: Gesperrtes bleibt liegen", [] { return rectVon("a") == "100 100 100 40"; }));
    add(klick("##k_vor", "##karte"));
    add(pause(0.2));
    add(pruefe("3b Kontext: Das Schloss wandert mit dem Element", [] {
        if (folgeText() != "grund,b,a,c") { diag::info("Knopftest: Folge " + folgeText()); }
        return folgeText() == "grund,b,a,c" && me::stand().gesperrt == 1;
    }));
    add(tu("3b Kontext: b waehlen", [] { me::waehle(1, false); }));
    add(pause(0.1));
    add(zieheVirt("3b Kontext: Zug an b", 350, 120, 360, 130));
    add(pause(0.2));
    add(pruefe("3b Kontext: b ist frei, das Schloss blieb nicht an der Nummer", [] { return rectVon("b") == "310 110 100 40"; }));
    add(tu("3b Kontext: zweite Seite", [] { me::setzeSeite(1); }));
    add(pause(0.2));
    add(pruefe("3b Kontext: Das Schloss gilt nur im eigenen Menue", [] { return me::stand().gesperrt == 0; }));
    add(tu("3b Kontext: erste Seite", [] { me::setzeSeite(0); }));
    add(pause(0.2));
    add(pruefe("3b Kontext: Zurueck: es ist wieder da", [] { return me::stand().gesperrt == 1; }));
    add(tu("3b Kontext: a waehlen", [] { me::waehle(2, false); }));
    add(pause(0.1));
    add(klickVirt("3b Kontext: Rechtsklick auf a", 150, 120, 1));
    add(pause(0.2));
    add(klick(tr(Str::MkEntsperren)));
    add(pause(0.2));
    add(pruefe("3b Kontext: Entsperren", [] { return me::stand().gesperrt == 0; }));
    add(klickVirt("3b Kontext: Rechtsklick auf a", 150, 120, 1));
    add(pause(0.2));
    add(klick(tr(Str::MkAusblenden)));
    add(pause(0.2));
    add(pruefe("3b Kontext: Ausblenden gilt nur in der Vorschau", [] { return me::stand().verborgen == 1; }));
    add(zieheVirt("3b Kontext: Rahmen ueber das Verborgene", 80, 80, 220, 150));
    add(pause(0.2));
    add(pruefe("3b Kontext: Der Rahmen nimmt Verborgenes nicht", [] { return namenDerAuswahl().empty(); }));

    // Hierarchie: dasselbe Menue
    add(probe("Hierarchie"));
    add(pause(0.5));
    add(klickAn("3b Kontext: Rechtsklick auf die Zeile c", [] { return zeileVon("c"); }, 1));
    add(pause(0.2));
    add(foto("menue_kontext_hierarchie"));
    add(pruefe("3b Kontext: Die Hierarchie zeigt dasselbe Menue", [] { return namenDerAuswahl() == "c" && finde(tr(Str::MkGanzVorn), "", 0, false) != nullptr; }));
    add(merken());
    add(klick(tr(Str::MkGanzHinten)));
    einSchritt("Hierarchie ganz nach hinten");

    // Zwischen Seiten und Dateien
    add(probe("Zwischen Seiten"));
    add(pause(0.5));
    add(tu("3b Kontext: a waehlen", [] { me::waehle(1, false); }));
    add(pause(0.1));
    add(klickVirt("3b Kontext: Fokus auf die Flaeche", 150, 120));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_C, "Strg+C"));
    add(tu("3b Kontext: zweite Seite", [] { me::setzeSeite(1); }));
    add(pause(0.2));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_V, "Strg+V"));
    add(pause(0.2));
    add(pruefe("3b Kontext: Einfuegen auf einer anderen Seite", [] {
        diag::info("Knopftest: Folge " + folgeText() + ", Auswahl " + namenDerAuswahl() + ", a " + rectVon("a"));
        return folgeText() == "x,y,a" && namenDerAuswahl() == "a" && rectVon("a") == "100 100 100 40";
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_X, "Strg+X"));
    add(pause(0.2));
    add(pruefe("3b Kontext: Strg+X nimmt es wieder heraus", [] { return folgeText() == "x,y"; }));
    add(tu("3b Kontext: andere Datei, Mehrspieler", [] {
        // Endrunde 4: wie die MP-Menues des Spiels "ui/jamp/menudef.h" - "ui/menudef.h" gibt es nur in Movie Duels; mit den
        // Daten des Original-JKA (menueeditorja) meldete der Parser "file ui/menudef.h not found"
        (void)me::oeffneText("ui/jamp/andere.menu", "#include \"ui/jamp/menudef.h\"\n{\n  menuDef\n  {\n    name \"andere\"\n    rect 0 0 640 480\n    visible 1\n  }\n}\n");
    }));
    add(pause(0.2));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_V, "Strg+V"));
    add(pause(0.2));
    add(pruefe("3b Kontext: Einfuegen in eine andere Datei, auch von Einzel- nach Mehrspieler", [] {
        const auto x = me::stand();
        for (const std::string& z : x.diagnosen) { diag::info("Knopftest: Meldung " + z); }
        diag::info("Knopftest: Folge " + folgeText() + ", MP " + std::to_string(me::dokumentMp()));
        return me::dokumentMp() && folgeText() == "a" && x.diagFehler == 0 && me::menuesImDokument() == 1 && zeileDa(x.text, "\"a\"");
    }));
    add(tu("3b Kontext: aufraeumen", [] { me::schliesseAlleOhneSpeichern(); }));
    return s;
}

// Speichern, Speichern unter, Aenderung von aussen, Statuszeile
std::vector<Schritt> schritteDatei() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    namespace fs = std::filesystem;
    const auto pfad = [](const char* name) {
        const char* laufOrdner = std::getenv("BHED_EINSTELLUNGEN");
        return (fs::path(laufOrdner != nullptr ? laufOrdner : ".") / name).string();
    };
    const auto liesDatei = [](const std::string& p) {
        std::ifstream f(p, std::ios::binary);
        return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    };
    const auto schreibDatei = [](const std::string& p, const std::string& t) {
        std::ofstream f(p, std::ios::binary);
        f.write(t.data(), static_cast<std::streamsize>(t.size()));
    };
    const std::string p0 = pfad("3b_eins.menu"), p1 = pfad("3b_zwei.menu"), p2 = pfad("3b_drei.menu");
    add(tu("3b Datei: Probe auf der Platte", [=] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        schreibDatei(p0, kKontextText);
        (void)me::oeffneDatei(p0);
        me::setzeInteraktion(false);
        me::setzeReiter(2);
        me::waehle(1, false);
    }));
    add(pause(0.5));
    add(pruefe("3b Datei: Die Statuszeile nennt das Befehlsmodell nicht", [] {
        for (const std::string& t : g_app->statusTeile) {
            if (t.find("signatures") != std::string::npos || (!g_app->statusModell.empty() && t == g_app->statusModell)) {
                diag::info("Knopftest: Statuszeile " + t);
                return false;
            }
        }
        return !g_app->statusModell.empty();
    }));
    add(klickVirt("3b Datei: Fokus Flaeche", 150, 120));
    add(taste(ImGuiKey_RightArrow, std::string("Pfeil rechts")));
    add(pause(0.1));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_S, "Strg+S"));
    add(pause(0.2));
    add(pruefe("3b Datei: Strg+S schreibt die Datei", [=] {
        const std::string t = liesDatei(p0);
        if (t != me::stand().text) { diag::info("Knopftest: Datei hat " + std::to_string(t.size()) + " Bytes, Text " + std::to_string(me::stand().text.size())); }
        return t == me::stand().text && zeileDa(t, "rect 101 100 100 40");
    }));
    // Im Text-Editor gilt es ebenso
    add(tu("3b Datei: Reiter Text", [] { me::setzeReiter(1); }));
    add(pause(0.2));
    add(klick("##codeflaeche"));
    add(pause(0.1));
    add(tu("3b Datei: Zeichen tippen", [] { ImGui::GetIO().AddInputCharactersUTF8("//z"); }));
    add(pause(0.2));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_S, "Strg+S"));
    add(pause(0.2));
    add(pruefe("3b Datei: Strg+S auch im Text-Editor", [=] { return liesDatei(p0) == me::stand().text && zeileDa(liesDatei(p0), "//z"); }));
    add(tu("3b Datei: Text-Editor verlassen", [] { me::setzeReiter(2); }));
    add(pause(0.2));
    add(klickVirt("3b Datei: Fokus Flaeche", 150, 120));
    // Speichern unter: Knopf und Tasten
    add(tu("3b Datei: Dialog ersetzen", [=] { me::setzeSpeichernDialog([=] { return p1; }); }));
    add(klick(tr(Str::ActSaveAs)));
    add(pause(0.2));
    add(pruefe("3b Datei: Speichern unter schreibt die neue Datei, die alte bleibt", [=] {
        return liesDatei(p1) == me::stand().text && me::dokumentName() == "ui/3b_zwei.menu";
    }));
    add(foto("menue_speichern_unter"));
    add(tu("3b Datei: naechstes Ziel, Abbruch", [=] { me::setzeSpeichernDialog([] { return std::string(); }); }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S, "Strg+Umschalt+S"));
    add(pause(0.2));
    add(pruefe("3b Datei: Abbrechen aendert nichts", [] { return me::dokumentName() == "ui/3b_zwei.menu"; }));
    add(tu("3b Datei: drittes Ziel", [=] { me::setzeSpeichernDialog([=] { return p2; }); }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S, "Strg+Umschalt+S"));
    add(pause(0.2));
    add(pruefe("3b Datei: Strg+Umschalt+S ist Speichern unter", [=] { return liesDatei(p2) == me::stand().text && me::dokumentName() == "ui/3b_drei.menu"; }));
    // Aenderung von aussen: Knopf Laden ist ein Schritt, Behalten laesst den Text
    auto mein = std::make_shared<std::string>();
    auto tiefe = std::make_shared<int>(0);
    add(pause(1.2));   // die Pruefung kennt jetzt den eigenen Stand
    add(tu("3b Datei: Ein anderes Programm schreibt", [=] {
        *mein = me::stand().text;
        *tiefe = me::stand().undoSchritte;
        schreibDatei(p2, *mein + "// von aussen\n");
    }));
    add(pause(1.5));
    add(foto("menue_fremd"));
    add(pruefe("3b Datei: Das Programm meldet die Aenderung, der Text bleibt", [=] { return finde(tr(Str::MeFremdLaden), "", 0, false) != nullptr && me::stand().text == *mein; }));
    add(klick(tr(Str::MeFremdBehalten)));
    add(pause(1.5));
    add(pruefe("3b Datei: Behalten: Die Meldung kommt nicht wieder, der Text bleibt", [=] { return finde(tr(Str::MeFremdLaden), "", 0, false) == nullptr && me::stand().text == *mein; }));
    add(tu("3b Datei: Wieder von aussen", [=] { schreibDatei(p2, *mein + "// noch einmal\n"); }));
    add(pause(1.5));
    add(klick(tr(Str::MeFremdLaden)));
    add(pause(0.3));
    add(pruefe("3b Datei: Laden: der Text von der Platte, ein Schritt", [=] {
        const auto x = me::stand();
        return zeileDa(x.text, "// noch einmal") && x.undoSchritte == *tiefe + 1;
    }));
    add(tu("3b Datei: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
    add(pause(0.2));
    add(pruefe("3b Datei: Rueckgaengig holt die eigene Fassung", [=] { return me::stand().text == *mein; }));
    // Eigenes Speichern loest die Meldung nicht aus
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_S, "Strg+S"));
    add(pause(1.5));
    add(pruefe("3b Datei: Eigenes Speichern meldet nichts", [=] { return finde(tr(Str::MeFremdLaden), "", 0, false) == nullptr && liesDatei(p2) == *mein; }));
    add(tu("3b Datei: aufraeumen", [=] {
        me::setzeSpeichernDialog(nullptr);
        me::schliesseAlleOhneSpeichern();
        std::error_code ec;
        fs::remove(p0, ec);
        fs::remove(p1, ec);
        fs::remove(p2, ec);
    }));
    return s;
}

// Seiten: Rechtsklick auf den Reiter
std::vector<Schritt> schritteSeiten() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto vor = std::make_shared<std::string>();
    auto tiefe = std::make_shared<int>(0);
    const auto merken = tu("3b Seiten: merken", [=] {
        *vor = me::stand().text;
        *tiefe = me::stand().undoSchritte;
    });
    const auto reiter = [](const std::string& name) {
        return klickAn("Rechtsklick auf den Reiter " + name, [name] {
            const Element* e = finde(name, "", 0, true);
            return e != nullptr ? e->rect.GetCenter() : ImVec2{-1.0F, -1.0F};
        }, 1);
    };
    const auto einSchritt = [&](const std::string& was) {
        add(pause(0.2));
        add(pruefe("3b Seiten: " + was + ": ein Schritt", [=] { return me::stand().undoSchritte == *tiefe + 1 && me::stand().text != *vor; }));
    };
    const auto zurueck = [&](const std::string& was) {
        add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
        add(pause(0.2));
        add(pruefe("3b Seiten: " + was + ": Rueckgaengig byte-gleich", [=] { return me::stand().text == *vor; }));
    };
    add(tu("3b Seiten: Probe", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneText("ui/kontext.menu", kKontextText);
        me::setzeInteraktion(false);
        me::setzeSpalten(0.0F, 0.0F);
        me::setzeReiter(2);
    }));
    add(pause(0.5));
    add(klickVirt("3b Seiten: Fokus Flaeche", 500, 400));
    add(merken);
    add(reiter("kontext"));
    add(pause(0.2));
    add(foto("menue_seite_kontext"));
    add(pruefe("3b Seiten: Das Menue des Reiters hat alle Befehle", [] {
        bool alle = true;
        for (const Str t : {Str::MsDuplizieren, Str::MsLoeschen, Str::MsNachLinks, Str::MsNachRechts}) { alle = alle && finde(tr(t), "", 0, false) != nullptr; }
        return alle && ImGui::GetIO().WantTextInput;
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(tippe("zweite"));
    add(taste(ImGuiKey_Enter, std::string("Enter")));
    add(pause(0.2));
    add(pruefe("3b Seiten: Ein Name, den es schon gibt, wird abgelehnt und erklaert", [=] { return me::stand().text == *vor && finde("##seitename", "", 0, false) != nullptr; }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(tippe("neuername"));
    add(taste(ImGuiKey_Enter, std::string("Enter")));
    einSchritt("Umbenennen");
    add(pruefe("3b Seiten: Umbenennen zieht open und close mit", [] {
        const std::string t = me::stand().text;
        return zeileDa(t, "name \"neuername\"") && zeileDa(t, "close neuername ; open neuername") && !zeileDa(t, "kontext\"");
    }));
    zurueck("Umbenennen");
    add(reiter("kontext"));
    add(pause(0.2));
    add(klick(tr(Str::MsDuplizieren)));
    einSchritt("Duplizieren");
    add(pruefe("3b Seiten: Die Kopie hat einen freien Namen und alle Elemente; die Seite zeigt sie", [] {
        return me::menuesImDokument() == 3 && zeileDa(me::stand().text, "name \"kontext2\"") && folgeText() == "grund,a,b,c";
    }));
    zurueck("Duplizieren");
    add(reiter("kontext"));
    add(pause(0.2));
    add(klick(tr(Str::MsNachRechts)));
    einSchritt("Nach rechts");
    add(pruefe("3b Seiten: Menues getauscht, die Seite folgt ihrem Menue", [] {
        const std::string t = me::stand().text;
        return t.find("\"zweite\"") < t.find("\"kontext\"") && folgeText() == "grund,a,b,c" && t.size() == std::string(kKontextText).size();
    }));
    zurueck("Nach rechts");
    add(reiter("zweite"));
    add(pause(0.2));
    add(klick(tr(Str::MsNachLinks)));
    einSchritt("Nach links");
    add(pruefe("3b Seiten: Nach links", [] { const std::string t = me::stand().text; return t.find("\"zweite\"") < t.find("\"kontext\""); }));
    zurueck("Nach links");
    add(reiter("zweite"));
    add(pause(0.2));
    add(klick(tr(Str::MsLoeschen)));
    einSchritt("Loeschen");
    add(pruefe("3b Seiten: Das Menue ist weg, der Rest unberuehrt", [] {
        const std::string t = me::stand().text;
        return me::menuesImDokument() == 1 && t.find("\"zweite\"") == std::string::npos && t.find("\"kontext\"") != std::string::npos;
    }));
    add(tu("3b Seiten: gemerkt", [=] { *tiefe = me::stand().undoSchritte; }));
    add(reiter("kontext"));
    add(pause(0.2));
    add(klickAn("3b Seiten: Klick auf den gesperrten Eintrag", [] {
        const Element* e = finde(tr(Str::MsLoeschen), "", 0, false);
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-1.0F, -1.0F};
    }));
    add(pause(0.2));
    add(pruefe("3b Seiten: Das letzte Menue laesst sich nicht loeschen", [=] { return me::menuesImDokument() == 1 && me::stand().undoSchritte == *tiefe; }));
    add(taste(ImGuiKey_Escape, std::string("Esc")));
    add(pause(0.2));
    add(tu("3b Seiten: wieder der Stand vor dem Loeschen", [=] { *tiefe = me::stand().undoSchritte - 1; }));
    zurueck("Loeschen");
    add(tu("3b Seiten: aufraeumen", [] { me::schliesseAlleOhneSpeichern(); }));
    return s;
}

// Ein neues Menue erscheint im Spiel: Menuesatz, Pruefung, Testpaket
std::vector<Schritt> schritteSatz() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto zeileMit = [](const std::string& teil) {
        for (const std::string& z : me::pruefZeilen()) {
            if (z.find(teil) != std::string::npos) { return true; }
        }
        return false;
    };
    for (const bool mp : {false, true}) {
        const std::string was = mp ? "MP" : "SP";
        const std::string datei = mp ? (me::stand().spiel == 2 ? "ui/MD_MP/eigen.menu" : me::stand().spiel == 1 ? "ui/jk2mp/eigen.menu" : "ui/jamp/eigen.menu") : "ui/eigen.menu";
        add(tu("3b Satz: neues Menue " + was, [datei] {
            me::schliesseAlleOhneSpeichern();
            g_app->leftMode = 4;
            tourSchliessen();
            (void)me::oeffneText(datei, "{\n  menuDef\n  {\n    name \"eigen\"\n    fullScreen 1\n    rect 0 0 640 480\n    visible 1\n  }\n}\n");
        }));
        add(pause(0.3));
        add(pruefe("3b Satz: " + was + ": Das Paket traegt die Datei in die Liste des Spiels ein", [datei, mp] {
            const auto liste = [mp] {
                for (const char* n : {"ui/MovieDuels-MP-Menus.txt", "ui/jampmenus.txt", "ui/jk2mpmenus.txt"}) {
                    if (mp) {
                        const std::string t = me::paketDatei(n, false);
                        if (!t.empty()) { return t; }
                    }
                }
                return mp ? std::string() : me::paketDatei("ui/menus.txt", false);
            };
            const std::string t = liste();
            if (t.find(datei) == std::string::npos) { diag::info("Knopftest: Liste ohne " + datei + ": " + std::to_string(t.size()) + " Bytes"); }
            return t.find("loadMenu { \"" + datei + "\" }") != std::string::npos && t.find("main.menu") != std::string::npos;
        }));
        add(pruefe("3b Satz: " + was + ": Nur das Testpaket oeffnet das neue Menue gleich beim Start", [mp] {
            const std::string pfad = mp ? (me::stand().spiel == 2 ? "ui/MD_MP/main.menu" : me::stand().spiel == 1 ? "ui/jk2mp/main.menu" : "ui/jamp/main.menu") : "ui/main.menu";
            const std::string test = me::paketDatei(pfad, true);
            if (test.find("open \"eigen\"") == std::string::npos) { diag::info("Knopftest: Testpaket ohne open: " + pfad + " " + std::to_string(test.size())); }
            return test.find("open \"eigen\"") != std::string::npos && me::paketDatei(pfad, false).empty();
        }));
        add(klick(tr(Str::MePruefen)));
        add(pause(0.3));
        add(foto("menue_pruefung_niemand_" + was));
        add(pruefe("3b Satz: " + was + ": Die Pruefung sagt, dass niemand das Menue oeffnet", [zeileMit] {
            // Endrunde 5: die Meldung in der Sprache des Laufs (ja/zh/de setzen den Namen in andere Anfuehrungszeichen als '...')
            std::vector<char> b(1024);
            std::snprintf(b.data(), b.size(), tr(Str::MeFlussUnerreichbar), "eigen", "eigen");
            return zeileMit(b.data());
        }));
        add(tu("3b Satz: Pruefung zu, ein Menue oeffnet es", [mp] {
            (void)me::oeffneText(mp ? "ui/jamp/main.menu" : "ui/main.menu", std::string("{\n  menuDef\n  {\n    name \"") + (mp ? "main" : "mainMenu") + "\"\n    rect 0 0 640 480\n    visible 1\n    onOpen { open eigen }\n  }\n}\n");
        }));
        add(pause(0.5));
        add(pruefe("3b Satz: " + was + ": Mit einem Weg dorthin ist die Warnung weg", [zeileMit] { return !zeileMit("'eigen'"); }));
    }
    add(tu("3b Satz: aufraeumen", [] { me::schliesseAlleOhneSpeichern(); }));
    return s;
}

// Zwei Videos zugleich: Im Spiel halten sie einander an (rc608 Teil 3a, Foto); die Vorschau wie die Engine
std::vector<Schritt> schritteZweiVideos() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto videos = std::make_shared<std::vector<std::string>>();
    add(tu("3b Videos: Probe mit zwei Rahmen", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneText("ui/zwei.menu", "menuDef\n{\n  name \"zwei\"\n  fullScreen 1\n  rect 0 0 640 480\n  visible 1\n"
                             "  itemDef\n  {\n    name \"eins\"\n    style WINDOW_STYLE_SHADER\n    rect 0 0 300 300\n    decoration\n    visible 1\n    // jkstudio:rahmen video\n  }\n"
                             "  itemDef\n  {\n    name \"zwei\"\n    style WINDOW_STYLE_SHADER\n    rect 320 0 300 300\n    decoration\n    visible 1\n    // jkstudio:rahmen video\n  }\n}\n");
        me::setzeInteraktion(false);
        me::setzeReiter(2);
    }));
    add(pause(0.3));
    for (const int nr : {0, 1}) {
        add(tu("3b Videos: Rahmen waehlen", [nr] { me::waehle(nr, false); }));
        add(pause(0.2));
        add(klick(tr(Str::MlWaehlen), "##inhalt"));
        add(pause(0.4));
        add(Schritt{"3b Videos: Ein Video anklicken", [videos, nr](int b) {
                        ImGuiIO& io = ImGui::GetIO();
                        if (b == 0) {
                            const Element* e = finde("##bw_", "##bw_liste", nr, true);
                            if (e == nullptr) { return true; }
                            videos->push_back(e->label.substr(5));
                            setzeMaus(e->rect.GetCenter());
                            return false;
                        }
                        if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
                        if (b == 3) { io.AddMouseButtonEvent(0, false); return false; }
                        return b >= 6;
                    }});
        add(pause(0.3));
    }
    add(tu("3b Videos: nichts gewaehlt", [] { me::waehle(-1, false); }));
    auto vorher = std::make_shared<int>(0);
    add(pause(1.0));
    add(tu("3b Videos: merken", [vorher] { *vorher = me::stand().kinoBilder; }));
    add(pause(1.5));
    add(foto("menue_zwei_videos"));
    add(pruefe("3b Videos: Zwei zugleich: keines laeuft weiter, und das Programm sagt es", [videos, vorher] {
        const int neu = me::stand().kinoBilder - *vorher;
        diag::info("Knopftest: zwei Videos: " + std::to_string(neu) + " neue Bilder in 1,5 s");
        return videos->size() >= 2U && neu == 0 && me::zweiVideosGemeldet();
    }));
    add(tu("3b Videos: das zweite ausblenden", [] { me::verbergeItem(1, true); }));
    add(pause(0.5));
    add(tu("3b Videos: merken", [vorher] { *vorher = me::stand().kinoBilder; }));
    add(pause(1.5));
    add(pruefe("3b Videos: Eines allein laeuft", [vorher] {
        const int neu = me::stand().kinoBilder - *vorher;
        diag::info("Knopftest: ein Video: " + std::to_string(neu) + " neue Bilder in 1,5 s");
        return neu >= 3 && !me::zweiVideosGemeldet();
    }));
    add(tu("3b Videos: aufraeumen", [] { me::schliesseAlleOhneSpeichern(); }));
    return s;
}

// Jede Kachel, SP und MP: einfuegen, ziehen, skalieren, kopieren, duplizieren, loeschen, sperren, ausblenden, speichern, neu laden, alles zurueck
std::vector<Schritt> schritteWidgets() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    struct Kachel { Str name; const char* kurz; };
    static const Kachel kKacheln[] = {{Str::MlTypText, "text"}, {Str::MlTypButton, "button"}, {Str::MlTypBild, "bild"}, {Str::MlTypVideo, "video"}, {Str::MlTypModell, "modell"},
                                      {Str::MlTypJaNein, "janein"}, {Str::MlTypMulti, "multi"}, {Str::MlTypSchieber, "schieber"}, {Str::MlTypEingabe, "eingabe"}, {Str::MlTypListe, "liste"}, {Str::MlTypTaste, "taste"},
                                      {Str::MlBtLautstaerke, "lautstaerke"}, {Str::MlBtReiter, "reiter"}, {Str::MlBtDetail, "detail"}, {Str::MlBtSpieler, "spieler"}};
    namespace fs = std::filesystem;
    const auto pfad = [](const std::string& name) {
        const char* laufOrdner = std::getenv("BHED_EINSTELLUNGEN");
        return (fs::path(laufOrdner != nullptr ? laufOrdner : ".") / name).string();
    };
    const auto mitte = [] {
        const auto a = me::stand().auswahl;
        const auto lagen = me::itemLagen();
        if (a.empty() || static_cast<std::size_t>(a[0]) >= lagen.size()) { return ImVec2{-1.0F, -1.0F}; }
        return ImVec2{lagen[static_cast<std::size_t>(a[0])].x + lagen[static_cast<std::size_t>(a[0])].b * 0.5F, lagen[static_cast<std::size_t>(a[0])].y + lagen[static_cast<std::size_t>(a[0])].h * 0.5F};
    };
    const auto rechtUnten = [] {
        const auto a = me::stand().auswahl;
        const auto lagen = me::itemLagen();
        if (a.empty() || static_cast<std::size_t>(a[0]) >= lagen.size()) { return ImVec2{-1.0F, -1.0F}; }
        return ImVec2{lagen[static_cast<std::size_t>(a[0])].x + lagen[static_cast<std::size_t>(a[0])].b, lagen[static_cast<std::size_t>(a[0])].y + lagen[static_cast<std::size_t>(a[0])].h};
    };
    const auto rectDerAuswahl = [] {
        const auto a = me::stand().auswahl;
        const auto lagen = me::itemLagen();
        if (a.empty() || static_cast<std::size_t>(a[0]) >= lagen.size()) { return std::array<float, 4>{}; }
        const me::ItemLage& l = lagen[static_cast<std::size_t>(a[0])];
        return std::array<float, 4>{l.x, l.y, l.b, l.h};
    };
    for (const bool mp : {false, true}) {
        const std::string dl = mp ? "MP " : "SP ";
        for (const Kachel& k : kKacheln) {
            const std::string name = dl + k.kurz;
            const std::string datei = pfad(std::string("3b_w_") + (mp ? "mp_" : "sp_") + k.kurz + ".menu");
            auto start = std::make_shared<std::string>();
            auto eingefuegt = std::make_shared<std::string>();
            const bool einzeln = k.name != Str::MlBtReiter && k.name != Str::MlBtDetail;
            add(tu("3b Widgets " + name + ": leeres Menue", [mp, start] {
                me::schliesseAlleOhneSpeichern();
                g_app->leftMode = 4;
                tourSchliessen();
                const std::string kopf = "menuDef\n{\n  name \"probe\"\n  fullScreen 1\n  rect 0 0 640 480\n  visible 1\n}\n";
                (void)me::oeffneText(mp ? (me::stand().spiel == 2 ? "ui/MD_MP/probe.menu" : me::stand().spiel == 1 ? "ui/jk2mp/probe.menu" : "ui/jamp/probe.menu") : "ui/probe.menu", mp ? "{\n" + kopf + "}\n" : kopf);
                me::setzeInteraktion(false);
                me::setzeSpalten(0.0F, 0.0F);
                me::setzeReiter(2);
                me::leisteVorgabe();
                *start = me::stand().text;
            }));
            add(pause(0.3));
            add(rolleZuKachel(tr(k.name)));
            add(klickAn("3b Widgets: Doppelklick auf die Kachel " + name, [k] {
                const Element* e = finde(tr(k.name), "##links", 0, false);
                return e != nullptr ? e->rect.GetCenter() : ImVec2{-1.0F, -1.0F};
            }, 0, true));
            add(pause(0.4));
            add(foto("menue_w_" + std::string(mp ? "mp_" : "sp_") + k.kurz));
            add(pruefe("3b Widgets " + name + ": eingefuegt, gewaehlt, ein Schritt, das Spiel liest es", [=] {
                const auto x = me::stand();
                *eingefuegt = x.text;
                for (const std::string& z : x.diagnosen) { diag::info("Knopftest: " + name + ": " + z); }
                const int rahmen = k.name == Str::MlTypBild || k.name == Str::MlTypVideo ? 1 : 0;   // "noch kein Bild gewaehlt" gehoert zum leeren Rahmen
                return x.undoSchritte == 1 && !x.auswahl.empty() && x.items >= 1 && x.abbruchZeile == 0 && x.meldungen == 0 && x.diagFehler == rahmen;
            }));
            add(pruefe("3b Widgets " + name + ": liegt ganz auf dem Schirm", [=] {
                for (const me::ItemLage& l : me::itemLagen()) {
                    if (l.x < 0.0F || l.y < 0.0F || l.x + l.b > 640.0F || l.y + l.h > 480.0F || l.b <= 0.0F || l.h <= 0.0F) {
                        diag::info("Knopftest: " + name + ": " + l.name + " " + std::to_string(l.x) + " " + std::to_string(l.y) + " " + std::to_string(l.b) + " " + std::to_string(l.h));
                        return false;
                    }
                }
                return true;
            }));
            if (einzeln) {
                auto r0 = std::make_shared<std::array<float, 4>>();
                add(tu("3b Widgets: Lage merken", [=] { *r0 = rectDerAuswahl(); }));
                add(Schritt{"3b Widgets " + name + ": ziehen", [=](int b) {
                                static std::shared_ptr<Schritt> innen;
                                if (b == 0) {
                                    const ImVec2 c = mitte();
                                    innen = std::make_shared<Schritt>(zieheVirt("ziehen", c.x, c.y, c.x + 20.0F, c.y + 10.0F));
                                }
                                return innen->tun(b);
                            }});
                add(pause(0.2));
                add(pruefe("3b Widgets " + name + ": gezogen um 20, 10", [=] {
                    const auto r = rectDerAuswahl();
                    if (r[0] != (*r0)[0] + 20.0F || r[1] != (*r0)[1] + 10.0F) { diag::info("Knopftest: " + name + " liegt auf " + std::to_string(r[0]) + " " + std::to_string(r[1]) + " statt " + std::to_string((*r0)[0] + 20.0F) + " " + std::to_string((*r0)[1] + 10.0F)); }
                    return r[0] == (*r0)[0] + 20.0F && r[1] == (*r0)[1] + 10.0F && r[2] == (*r0)[2] && me::stand().undoSchritte == 2;
                }));
                add(Schritt{"3b Widgets " + name + ": skalieren", [=](int b) {
                                static std::shared_ptr<Schritt> innen;
                                if (b == 0) {
                                    const ImVec2 c = rechtUnten();
                                    innen = std::make_shared<Schritt>(zieheVirt("skalieren", c.x, c.y, c.x + 16.0F, c.y + 12.0F));
                                }
                                return innen->tun(b);
                            }});
                add(pause(0.2));
                add(pruefe("3b Widgets " + name + ": am Griff um 16, 12 groesser", [=] {
                    const auto r = rectDerAuswahl();
                    const bool getroffen = std::fabs(r[2] - (*r0)[2] - 16.0F) <= 1.0F && std::fabs(r[3] - (*r0)[3] - 12.0F) <= 1.0F;   // die Maus steht auf Bildpunkten
                    if (!getroffen) { diag::info("Knopftest: " + name + " Groesse " + std::to_string(r[2]) + " " + std::to_string(r[3]) + " statt " + std::to_string((*r0)[2] + 16.0F) + " " + std::to_string((*r0)[3] + 12.0F)); }
                    return getroffen && me::stand().undoSchritte == 3;
                }));
                add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_D, "Strg+D"));
                add(pause(0.2));
                add(pruefe("3b Widgets " + name + ": dupliziert, gewaehlt ist die Kopie", [=] { return me::stand().items == 2 && me::stand().auswahl == std::vector<int>{1} && me::stand().undoSchritte == 4; }));
                add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_C, "Strg+C"));
                add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_V, "Strg+V"));
                add(pause(0.2));
                add(pruefe("3b Widgets " + name + ": eingefuegt mit freiem Namen", [=] {
                    const auto n = namenInFolge();
                    return n.size() == 3U && n[0] != n[1] && n[1] != n[2] && n[0] != n[2] && me::stand().undoSchritte == 5;
                }));
                add(taste(ImGuiKey_Delete, std::string("Entf")));
                add(pause(0.2));
                add(pruefe("3b Widgets " + name + ": geloescht", [=] { return me::stand().items == 2 && me::stand().auswahl.empty() && me::stand().undoSchritte == 6; }));
                add(tu("3b Widgets: erstes waehlen und sperren", [] {
                    me::waehle(0, false);
                    me::sperreItem(0, true);
                }));
                add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
                add(pause(0.1));
                add(pruefe("3b Widgets " + name + ": Strg+A laesst Gesperrtes aus", [] { return me::stand().auswahl == std::vector<int>{1}; }));
                add(tu("3b Widgets: entsperren", [] { me::sperreItem(0, false); }));
            }
            // Speichern und neu laden
            add(tu("3b Widgets " + name + ": speichern und neu laden", [=] {
                const std::string t = me::stand().text;
                (void)me::speichereUnter(datei);
                const std::string spielName = me::dokumentName();
                me::schliesseAlleOhneSpeichern();
                (void)me::oeffneText(spielName, t);
            }));
            add(pause(0.3));
            add(pruefe("3b Widgets " + name + ": auf der Platte wie im Editor, neu gelesen ohne Meldung", [=] {
                std::ifstream f(datei, std::ios::binary);
                const std::string platte((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                const auto x = me::stand();
                const int rahmen = k.name == Str::MlTypBild || k.name == Str::MlTypVideo ? x.items : 0;
                return platte == x.text && x.items >= 1 && x.abbruchZeile == 0 && x.meldungen == 0 && x.diagFehler == rahmen;
            }));
            add(tu("3b Widgets " + name + ": Datei weg", [=] {
                std::error_code ec;
                fs::remove(datei, ec);
            }));
        }
    }
    add(tu("3b Widgets: aufraeumen", [] { me::schliesseAlleOhneSpeichern(); }));
    return s;
}

// Jedes Feld mit leeren und unsinnigen Werten: Das Menue bleibt lesbar, das Element bleibt, der Text sagt, was drin steht
std::vector<Schritt> schritteUnsinn() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    struct Feld { const char* id; const char* gruppe; const char* item; };
    static const Feld kFelder[] = {{"##name", nullptr, "knopf"}, {"##text", "inhalt", "knopf"}, {"##group", "logik", "knopf"}, {"##cvartest", "logik", "knopf"}, {"##showcvar", "logik", "knopf"},
                                   {"##cvar", "wert", "schieb"}, {"##background", "bildw", "bild"}, {"##mname", "menue", ""}, {"##mbild", "menue", ""},
                                   {"##asset_model", "modell", "figur"}, {"##model_g2skin", "modell", "figur"}};
    static const char* const kWerte[] = {"", " ", "\"", "x\"y", "{", "}", "};", "//", "/*", "\\", "%s%n", "Grüße", "日本語", "$(x)", "a b c"};
    const std::string kProbe =
        "menuDef\n{\n  name \"probe\"\n  fullScreen 1\n  rect 0 0 640 480\n  visible 1\n"
        "  itemDef\n  {\n    name \"knopf\"\n    type ITEM_TYPE_BUTTON\n    rect 100 100 200 40\n    text \"Back\"\n    group \"g\"\n    cvarTest \"ui_tab\"\n    showCvar { \"0\" }\n    visible 1\n    action { close probe }\n  }\n"
        "  itemDef\n  {\n    name \"schieb\"\n    type ITEM_TYPE_SLIDER\n    rect 100 200 200 40\n    cvarfloat \"s_volume\" 0.8 0 1\n    visible 1\n  }\n"
        "  itemDef\n  {\n    name \"bild\"\n    style WINDOW_STYLE_SHADER\n    rect 300 100 200 100\n    background \"gfx/menus/menu_box1\"\n    decoration\n    visible 1\n  }\n"
        "  itemDef\n  {\n    name \"figur\"\n    type ITEM_TYPE_MODEL\n    rect 300 240 200 200\n    asset_model \"models/players/kyle/model.glm\"\n    model_g2skin \"models/players/kyle/model_default.skin\"\n    decoration\n    visible 1\n  }\n"
        "}\n";
    for (const bool mp : {false, true}) {
        for (const Feld& f : kFelder) {
            for (const char* w : kWerte) {
                const std::string fall = std::string(mp ? "MP " : "SP ") + f.id + " = [" + w + "]";
                add(tu("3b Unsinn " + fall + ": Probe", [=] {
                    me::schliesseAlleOhneSpeichern();
                    g_app->leftMode = 4;
                    tourSchliessen();
                    (void)me::oeffneText(mp ? "ui/jamp/probe.menu" : "ui/probe.menu", mp ? "{\n" + kProbe + "}\n" : kProbe);
                    me::setzeInteraktion(false);
                    me::setzeReiter(2);
                    me::leisteAlleOffen();
                    me::setzeSpalten(0.0F, 0.0F);
                    me::waehle(f.item[0] != 0 ? me::itemNummer(f.item) : -1, false);
                    if (ImGuiWindow* fenster = inhaltFenster()) { ImGui::SetScrollY(fenster, 0.0F); }
                }));
                add(pause(0.15));
                add(Schritt{"3b Unsinn: das Feld ins Bild rollen", [f](int b) {
                                if (b < 2) { return false; }
                                const Element* e = finde(f.id, "", 0, false);
                                ImGuiWindow* fenster = inhaltFenster();
                                if (e != nullptr && fenster != nullptr && (e->rect.Max.y > fenster->InnerRect.Max.y || e->rect.Min.y < fenster->InnerRect.Min.y)) {
                                    ImGui::SetScrollY(fenster, fenster->Scroll.y + e->rect.Min.y - fenster->InnerRect.Min.y - 20.0F);
                                }
                                return b >= 5;
                            }});
                add(klick(f.id));
                add(pause(0.05));
                add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
                add(taste(ImGuiKey_Delete, std::string("Entf")));
                if (w[0] != 0) { add(tippe(w)); }
                add(taste(ImGuiKey_Enter, std::string("Enter")));
                add(pause(0.1));
                add(pruefe("3b Unsinn " + fall + ": Das Menue bleibt ganz", [=] {
                    const auto x = me::stand();
                    const bool ok = x.abbruchZeile == 0 && x.menus == 1 && x.items == 4;
                    int auf = 0, zu = 0;
                    bool inString = false;
                    bool escape = false;
                    for (const char c : x.text) {
                        if (escape) {
                            escape = false;
                            continue;
                        }
                        if (mp && inString && c == '\\') {
                            escape = true;
                            continue;
                        }
                        if (c == '"') { inString = !inString; }
                        if (!inString && c == '{') { ++auf; }
                        if (!inString && c == '}') { ++zu; }
                    }
                    diag::detail("Knopftest: Unsinn | " + fall + " | " + (ok && auf == zu && !inString ? "gut" : "ROT") + " | Menues " + std::to_string(x.menus) + ", Elemente " + std::to_string(x.items));
                    return ok && auf == zu && !inString;
                }));
            }
        }
    }
    add(tu("3b Unsinn: aufraeumen", [] { me::schliesseAlleOhneSpeichern(); }));
    return s;
}

// Umlaute und CJK-Zeichen in Feldern: wie im Spiel gespeichert (Kodierung der Sprache), nicht als UTF-8-Bytes
std::vector<Schritt> schritteKodierung() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("3b Kodierung: Probe", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneText("ui/probe.menu", "menuDef\n{\n  name \"probe\"\n  fullScreen 1\n  rect 0 0 640 480\n  visible 1\n  itemDef\n  {\n    name \"knopf\"\n    type ITEM_TYPE_BUTTON\n    rect 100 100 200 40\n    text \"Back\"\n    visible 1\n  }\n}\n");
        me::setzeInteraktion(false);
        me::setzeReiter(2);
        me::leisteVorgabe();
        me::waehle(0, false);
    }));
    add(pause(0.3));
    add(klick("##text"));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(tippe("Grüße"));
    add(taste(ImGuiKey_Enter, std::string("Enter")));
    add(pause(0.2));
    add(pruefe("3b Kodierung: Text in Properties: Umlaute in der Kodierung der Datei", [] {
        const std::string t = me::stand().text;
        diag::info("Knopftest: Text " + t.substr(t.find("text"), 20));
        return t.find("text\t") != std::string::npos ? t.find("Gr\xFC\xDF" "e") != std::string::npos : t.find("Gr\xFC\xDF" "e") != std::string::npos;
    }));
    add(pruefe("3b Kodierung: In Properties steht es wieder lesbar", [] { return textVon("knopf").find("Gr\xFC\xDF" "e") != std::string::npos; }));
    add(tu("3b Kodierung: aufraeumen", [] { me::schliesseAlleOhneSpeichern(); }));
    return s;
}

// Anfaenger: Jedes Bedienelement ohne sichtbaren Namen erklaert sich beim Zeigen (Tooltip oder Hilfezeile); die Leiste oben ebenso
bool tooltipOffen() {
    for (ImGuiWindow* w : GImGui->Windows) {
        if (w->Active && std::strncmp(w->Name, "##Tooltip", 9) == 0) { return true; }
    }
    return false;
}

Schritt allesErklaert(const std::string& wo, const std::string& fenster, bool auchMitNamen) {
    struct Lauf {
        std::vector<std::pair<std::string, ImVec2>> liste;
        std::size_t k = 0;
        int warte = 0;
        int ohne = 0;
        std::string hilfe0;
    };
    auto l = std::make_shared<Lauf>();
    return Schritt{"3b Anfaenger: " + wo, [=](int b) {
                       if (b == 0) {
                           *l = Lauf{};
                           std::set<std::string> gesehen;
                           for (const Element& e : elemente::alle()) {
                               if (fenster.empty()) {   // Die Werkzeugleiste steht im Hauptfenster ueber den Dateireitern
                                   const float y1 = me::stand().flaeche[1] - ImGui::GetFontSize() * 6.0F;
                                   if (e.rect.Max.y > y1 || e.rect.Min.y < ImGui::GetFrameHeight() || e.label.find(".menu") != std::string::npos || e.innen != e.fenster) { continue; }
                               } else if (e.innen.find(fenster) == std::string::npos && e.fenster.find(fenster) == std::string::npos) {
                                   continue;
                               }
                               if (e.rect.GetWidth() < 2.0F || e.rect.GetHeight() < 2.0F) { continue; }
                               const bool namenlos = e.label.empty() || e.label.rfind("##", 0) == 0;
                               if (!namenlos && !auchMitNamen) { continue; }
                               // Gruppenkoepfe und Schrittzeilen zeigen ihren Text, Suchfelder ihren Hinweis, Feldzeilen die Beschriftung davor
                               if (e.label.rfind("##gr_", 0) == 0 || e.label.rfind("##st", 0) == 0 || e.label.find("_suche") != std::string::npos || (e.label.size() == 4 && e.label.rfind("##a", 0) == 0)) { continue; }
                               if (e.label.empty() || !gesehen.insert(e.label).second) { continue; }
                               if (!ImRect(e.clip).Contains(e.rect.GetCenter())) { continue; }
                               l->liste.emplace_back(e.label, e.rect.GetCenter());
                           }
                           GImGui->Style.HoverDelayNormal = 0.0F;
                           GImGui->Style.HoverDelayShort = 0.0F;
                           GImGui->Style.HoverStationaryDelay = 0.0F;
                           l->hilfe0 = me::leisteStand().hilfe;
                           diag::detail("Knopftest: " + wo + ": " + std::to_string(l->liste.size()) + " Elemente");
                           return false;
                       }
                       if (l->k >= l->liste.size()) {
                           GImGui->Style.HoverDelayNormal = 0.40F;
                           GImGui->Style.HoverDelayShort = 0.15F;
                           GImGui->Style.HoverStationaryDelay = 0.15F;
                           if (l->ohne > 0) { meldeFehler(wo + ": " + std::to_string(l->ohne) + " ohne Erklaerung"); }
                           return true;
                       }
                       const auto& [name, ort] = l->liste[l->k];
                       if (l->warte == 0) { setzeMaus(ort); }
                       ++l->warte;
                       if (l->warte < 5) { return false; }
                       const std::string hilfe = me::leisteStand().hilfe;
                       const bool erklaert = tooltipOffen() || (!hilfe.empty() && hilfe != l->hilfe0);
                       if (!erklaert) {
                           ++l->ohne;
                           diag::info("Knopftest: Ohne Erklaerung | " + wo + " | " + name);
                       }
                       ++l->k;
                       l->warte = 0;
                       return false;
                   }};
}

std::vector<Schritt> schritteAnfaenger() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("3b Anfaenger: Probe", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneText("ui/leiste.menu", kLeisteText);
        me::setzeInteraktion(false);
        me::setzeSpalten(0.0F, 0.0F);
    }));
    add(pause(0.4));
    add(allesErklaert("Werkzeugleiste", "", true));
    add(allesErklaert("Linke Leiste", "##links", false));
    struct Fall { const char* wo; int reiter; const char* element; };
    static const Fall kFaelle[] = {{"Properties Knopf", 2, "knopf"}, {"Properties Text", 2, "titel"}, {"Properties Menue", 2, ""}, {"Events Knopf", 3, "knopf"}, {"Events Menue", 3, ""}};
    for (const Fall& f : kFaelle) {
        add(tu(std::string("3b Anfaenger: ") + f.wo, [f] {
            me::setzeReiter(f.reiter);
            me::leisteAlleOffen();
            me::waehle(f.element[0] != 0 ? me::itemNummer(f.element) : -1, false);
            if (ImGuiWindow* w = inhaltFenster()) { ImGui::SetScrollY(w, 0.0F); }
        }));
        add(pause(0.3));
        add(allesErklaert(f.wo, "##rechts", false));
    }
    add(tu("3b Anfaenger: aufraeumen", [] { me::schliesseAlleOhneSpeichern(); }));
    return s;
}

// Felder, die vorher nur im Text gingen
std::vector<Schritt> schritteFelderNeu() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto probe = [](const char* name) {
        return tu(std::string("3b Felder: Probe, ") + name, [name] {
            me::schliesseAlleOhneSpeichern();
            g_app->leftMode = 4;
            tourSchliessen();
            (void)me::oeffneText("ui/felder.menu",
                "menuDef\n{\n  name \"felder\"\n  fullScreen 1\n  rect 0 0 640 480\n  visible 1\n"
                "  itemDef\n  {\n    name \"knopf\"\n    type ITEM_TYPE_BUTTON\n    rect 100 100 200 40\n    text \"Back\"\n    cvarTest \"ui_tab\"\n    showCvar { \"0\" }\n    visible 1\n  }\n"
                "  itemDef\n  {\n    name \"wahl\"\n    type ITEM_TYPE_MULTI\n    rect 100 200 200 40\n    text \"Mode:\"\n    cvar \"ui_x\"\n    cvarStrList { \"One\" \"a\" \"Two\" \"b\" }\n    visible 1\n  }\n"
                "  itemDef\n  {\n    name \"gemalt\"\n    type ITEM_TYPE_OWNERDRAW\n    rect 300 300 100 40\n    ownerdraw 1\n    visible 1\n  }\n"
                "  itemDef\n  {\n    name \"liste\"\n    type ITEM_TYPE_LISTBOX\n    rect 300 100 200 100\n    feeder 16\n    elementwidth 100\n    elementheight 20\n    visible 1\n  }\n}\n");
            me::setzeInteraktion(false);
            me::setzeSpalten(0.0F, 0.0F);
            me::setzeReiter(2);
            me::leisteAlleOffen();
            me::waehle(me::itemNummer(name), false);
            if (ImGuiWindow* w = inhaltFenster()) { ImGui::SetScrollY(w, 0.0F); }
        });
    };
    const auto insBild = [](const char* id) {
        return Schritt{std::string("3b Felder: ins Bild rollen ") + id, [id](int b) {
                           if (b < 2) { return false; }
                           const Element* e = finde(id, "", 0, false);
                           ImGuiWindow* w = inhaltFenster();
                           if (e != nullptr && w != nullptr && (e->rect.Max.y > w->InnerRect.Max.y - 40.0F || e->rect.Min.y < w->InnerRect.Min.y)) {
                               ImGui::SetScrollY(w, w->Scroll.y + e->rect.Min.y - w->InnerRect.Min.y - 20.0F);
                           }
                           return b >= 5;
                       }};
    };
    auto tiefe = std::make_shared<int>(0);
    const auto merken = tu("3b Felder: merken", [tiefe] { *tiefe = me::stand().undoSchritte; });
    // Bedingung
    add(probe("knopf"));
    add(pause(0.4));
    add(insBild("##showcvar"));
    add(foto("menue_felder_bedingung"));
    add(merken);
    // Ist die Spalte niedrig (voller Lauf), rollt "##showcvar" die Auswahl darueber aus dem Bild: erst zu ihr rollen
    add(insBild("##bedingung"));
    add(pause(0.1));
    add(klickAn("3b Felder: Auswahl Bedingung", [] {
        const auto x = me::leisteFeldStelle("##bedingung");
        return ImVec2{x[0], x[1]};
    }));
    add(pause(0.2));
    add(klick(tr(Str::MlBedVerbergen)));
    add(pause(0.2));
    add(pruefe("3b Felder: Die Art wechselt, showCvar wird hideCvar, der Wert bleibt, ein Schritt", [tiefe] {
        const std::string t = me::stand().text;
        return zeileDa(t, "hideCvar") && !zeileDa(t, "showCvar") && zeileDa(t, "\"0\"") && me::stand().undoSchritte == *tiefe + 1 && me::stand().meldungen == 0;
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(pruefe("3b Felder: Rueckgaengig: wieder showCvar", [] { return zeileDa(me::stand().text, "showCvar"); }));
    // Mehrfachwahl
    add(probe("wahl"));
    add(pause(0.4));
    add(insBild("##wl"));
    add(foto("menue_felder_mehrfach"));
    add(merken);
    add(klick("##wl"));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(tippe("Eins"));
    add(taste(ImGuiKey_Enter, std::string("Enter")));
    add(pause(0.2));
    add(pruefe("3b Felder: Der Text eines Eintrags, die Liste bleibt ganz", [tiefe] {
        const std::string t = me::stand().text;
        diag::info("Knopftest: " + t.substr(t.find("cvarStrList"), 50));
        return zeileDa(t, "cvarStrList { \"Eins\" \"a\" \"Two\" \"b\" }") && me::stand().undoSchritte == *tiefe + 1;
    }));
    add(rolleRechtsZu(tr(Str::MlWerteNeu)));   // es: der Knopf liegt mit dem laengeren Text am unteren Rand
    add(klick(tr(Str::MlWerteNeu)));
    add(pause(0.2));
    add(pruefe("3b Felder: Ein Eintrag mehr", [] { return zeileDa(me::stand().text, "\"2\" }"); }));
    add(klick("##wx"));
    add(pause(0.2));
    add(pruefe("3b Felder: Der erste Eintrag ist weg", [] { return !zeileDa(me::stand().text, "\"Eins\""); }));
    // Ownerdraw: Was das Spiel nicht liest, wird nicht geschrieben
    add(probe("gemalt"));
    add(pause(0.4));
    add(insBild("##wort_ownerdraw"));
    add(foto("menue_felder_ownerdraw"));
    add(merken);
    add(klick("##wort_ownerdraw"));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(tippe("UNSINN"));
    add(taste(ImGuiKey_Enter, std::string("Enter")));
    add(pause(0.2));
    add(pruefe("3b Felder: Unsinn in ownerdraw: bleibt alt, das Programm sagt es", [tiefe] {
        const bool gesagt = me::letzteMeldung().find("UNSINN") != std::string::npos;
        diag::info("Knopftest: Meldung " + me::letzteMeldung());
        return zeileDa(me::stand().text, "ownerdraw 1") && me::stand().undoSchritte == *tiefe && gesagt && me::stand().abbruchZeile == 0;
    }));
    add(klick("##wort_ownerdraw"));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(tippe("2"));
    add(taste(ImGuiKey_Enter, std::string("Enter")));
    add(pause(0.2));
    add(pruefe("3b Felder: ownerdraw 2 wird geschrieben", [tiefe] { return zeileDa(me::stand().text, "ownerdraw 2") && me::stand().undoSchritte == *tiefe + 1; }));
    // Liste
    add(probe("liste"));
    add(pause(0.4));
    add(insBild("##wort_elementheight"));
    add(foto("menue_felder_liste"));
    add(klick("##wort_elementheight"));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(tippe("33"));
    add(taste(ImGuiKey_Enter, std::string("Enter")));
    add(pause(0.2));
    add(pruefe("3b Felder: Zeilenhoehe", [] { return zeileDa(me::stand().text, "elementheight 33"); }));
    add(tu("3b Felder: aufraeumen", [] { me::schliesseAlleOhneSpeichern(); }));
    return s;
}

// Undo ueber mehrere Dateien, nach dem Speichern und ueber Reiterwechsel
std::vector<Schritt> schritteUndoMehr() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    namespace fs = std::filesystem;
    const char* laufOrdner = std::getenv("BHED_EINSTELLUNGEN");
    const std::string pfad = (fs::path(laufOrdner != nullptr ? laufOrdner : ".") / "3b_undo.menu").string();
    auto a0 = std::make_shared<std::string>();
    auto b0 = std::make_shared<std::string>();
    auto a1 = std::make_shared<std::string>();
    auto b1 = std::make_shared<std::string>();
    const auto zuDatei = [](const std::string& name) { return klickAn("Reiter " + name, [name] {
        const Element* e = finde(name, "", 0, true);
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-1.0F, -1.0F};
    }); };
    add(tu("3b Undo mehr: zwei Dateien", [=] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneText("ui/a.menu", kKontextText);
        *a0 = me::stand().text;
        (void)me::oeffneText("ui/b.menu", kKontextText);
        *b0 = me::stand().text;
        me::setzeInteraktion(false);
        me::setzeReiter(2);
    }));
    add(pause(0.4));
    add(klickVirt("3b Undo mehr: b waehlt c", 150, 320));
    add(zieheVirt("3b Undo mehr: b zieht c", 150, 320, 170, 330));
    add(pause(0.2));
    add(tu("3b Undo mehr: gemerkt", [=] { *b1 = me::stand().text; }));
    add(zuDatei("ui/a.menu"));
    add(pause(0.3));
    add(klickVirt("3b Undo mehr: a waehlt a", 150, 120));
    add(zieheVirt("3b Undo mehr: a zieht a", 150, 120, 180, 120));
    add(pause(0.2));
    add(tu("3b Undo mehr: gemerkt", [=] { *a1 = me::stand().text; }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(pruefe("3b Undo mehr: Rueckgaengig trifft nur die offene Datei", [=] { return me::stand().text == *a0 && me::dokumentName() == "ui/a.menu"; }));
    add(zuDatei("ui/b.menu"));
    add(pause(0.3));
    add(pruefe("3b Undo mehr: Die andere Datei hat ihren Schritt noch", [=] { return me::stand().text == *b1 && me::stand().undoSchritte == 1; }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(pruefe("3b Undo mehr: In b zurueck", [=] { return me::stand().text == *b0; }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Y, "Strg+Y"));
    add(pause(0.2));
    add(pruefe("3b Undo mehr: In b wiederholt, auch die Auswahl", [=] { return me::stand().text == *b1 && namenDerAuswahl() == "c"; }));
    add(zuDatei("ui/a.menu"));
    add(pause(0.3));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Y, "Strg+Y"));
    add(pause(0.2));
    add(pruefe("3b Undo mehr: Wiederholen nach dem Reiterwechsel", [=] { return me::stand().text == *a1; }));
    // Nach dem Speichern
    add(tu("3b Undo mehr: speichern", [=] { (void)me::speichereUnter(pfad); }));
    add(pause(0.2));
    add(pruefe("3b Undo mehr: Nach dem Speichern ohne Stern", [] { return me::fensterTitel().find('*') == std::string::npos; }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(pruefe("3b Undo mehr: Rueckgaengig nach dem Speichern, wieder mit Stern", [=] { return me::stand().text == *a0 && me::fensterTitel().find('*') != std::string::npos; }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Y, "Strg+Y"));
    add(pause(0.2));
    add(pruefe("3b Undo mehr: Wiederholen bis zum gespeicherten Stand: kein Stern", [=] { return me::stand().text == *a1 && me::fensterTitel().find('*') == std::string::npos; }));
    // Seiten
    add(tu("3b Undo mehr: zweite Seite", [] { me::setzeSeite(1); }));
    add(pause(0.3));
    add(klickVirt("3b Undo mehr: x waehlen", 100, 70));
    add(zieheVirt("3b Undo mehr: x ziehen", 100, 70, 110, 80));
    add(pause(0.2));
    add(tu("3b Undo mehr: erste Seite", [] { me::setzeSeite(0); }));
    add(pause(0.3));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(pruefe("3b Undo mehr: Rueckgaengig holt die Seite des Schritts", [=] { return me::stand().text == *a1 && me::stand().seite == 1; }));
    add(tu("3b Undo mehr: aufraeumen", [=] {
        me::schliesseAlleOhneSpeichern();
        std::error_code ec;
        fs::remove(pfad, ec);
    }));
    return s;
}

// Wenig Platz: Bei 1920 x 1080 und 200 % bleibt die Flaeche brauchbar, die Spalten lassen sich zuklappen und wieder oeffnen
std::vector<Schritt> schritteEng() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto skala = std::make_shared<float>(1.0F);
    auto groesse = std::make_shared<std::pair<int, int>>();
    add(tu("3b Eng: Probe bei 1920 x 1080, 200 %", [=] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneText("ui/leiste.menu", kLeisteText);
        me::setzeInteraktion(false);
        me::setzeSpalten(0.0F, 0.0F);
        g_app->settings.menueRechtsBreite = 0.0F;
        me::waehle(me::itemNummer("knopf"), false);
        *skala = g_app->uiScale;
        *groesse = platform::hauptfensterGroesse(1920, 1080);
        g_app->uiScale = 2.0F;
    }));
    add(pause(0.8));
    add(foto("menue_eng_zu"));
    add(pruefe("3b Eng: Die linke Spalte ging zu, die Flaeche ist ueber 320 breit", [] {
        const auto x = me::stand();
        diag::info("Knopftest: Flaeche " + std::to_string(x.flaeche[2]) + " x " + std::to_string(x.flaeche[3]) + ", links zu " + std::to_string(me::linkeSpalteZu()));
        return me::linkeSpalteZu() && x.flaeche[2] >= 320.0F;
    }));
    add(klick("##links_zu"));
    add(pause(0.3));
    add(foto("menue_eng_auf"));
    add(pruefe("3b Eng: Der Pfeil oeffnet sie wieder", [] { return !me::linkeSpalteZu() && zeileVon("knopf").x >= 0.0F; }));
    add(klick("##links_zu"));
    add(pause(0.3));
    add(pruefe("3b Eng: ... und schliesst sie", [] { return me::linkeSpalteZu(); }));
    add(klick("##rechts_zu"));
    add(pause(0.3));
    add(foto("menue_eng_beide"));
    add(pruefe("3b Eng: Beide zu: die Flaeche hat allen Platz", [] {
        const auto x = me::stand();
        diag::info("Knopftest: Flaeche ohne Spalten " + std::to_string(x.flaeche[2]) + " x " + std::to_string(x.flaeche[3]));
        return x.flaeche[2] >= 400.0F && finde("##k_weg", "##karte", 0, false) == nullptr;
    }));
    add(klick("##rechts_zu"));
    add(pause(0.3));
    add(pruefe("3b Eng: Rechts wieder da", [] { return finde("##k_weg", "##karte", 0, false) != nullptr; }));
    // In en heisst auch das Menue der Hauptleiste "View" (Treffer 0), in es nicht ("Ver" gegen "Vista"): der letzte Treffer
    add(klickAn("3b Eng: Knopf " + std::string(tr(Str::MeAnsichtKnopf)), [] {
        const Element* e = finde(tr(Str::MeAnsichtKnopf), "", 1, false);
        if (e == nullptr) { e = finde(tr(Str::MeAnsichtKnopf), "", 0, false); }
        return e != nullptr ? e->rect.GetCenter() : ImVec2{};
    }));
    add(pause(0.3));
    add(foto("menue_eng_ansicht"));
    add(pruefe("3b Eng: Hinter dem Knopf stehen alle Ansicht-Schalter", [] {
        return finde(tr(Str::MeRaster), "", 0, false) != nullptr && finde(tr(Str::MeAnRaster), "", 0, false) != nullptr && finde(tr(Str::MeAlleItems), "", 0, false) != nullptr;
    }));
    add(taste(ImGuiKey_Escape, std::string("Esc")));
    add(tu("3b Eng: zurueck", [=] {
        g_app->uiScale = *skala;
        if (groesse->first > 0) { (void)platform::hauptfensterGroesse(groesse->first, groesse->second); }
    }));
    add(pause(0.5));
    add(pruefe("3b Eng: Bei viel Platz ist die linke Spalte wieder da", [] { return !me::linkeSpalteZu(); }));
    add(tu("3b Eng: aufraeumen", [] { me::schliesseAlleOhneSpeichern(); }));
    return s;
}

// Das Tutorial "Dein erstes eigenes Menue" in allen Sprachen, 100 % und 200 %
// Tutorial "Ein Menue aus dem Spiel aendern" (tut-menue) mit echten Klicks von Anfang bis Ende. Die Sprachen mit eigenem
// Layoutrisiko: die laengsten Texte (cs) und die breiten Zeichen (ja), dazu die eingestellte.
std::vector<Schritt> schritteTourSpiel() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    namespace fs = std::filesystem;
    const auto paket = std::make_shared<std::string>(pfadText(bhed::utf8Pfad(platform::executableDirectory()) / "tour_spiel" / "zz_menu.pk3"));
    const i18n::Language sprache = i18n::currentLanguage();
    const auto runde = [&](const std::string& code, i18n::Language lang) {
        i18n::setLanguage(lang);   // die Schritte merken sich die Beschriftungen beim Bauen
        auto erledigt = [&](int n) {
            add(warteBis("Tour Spiel " + code + ": Schritt " + std::to_string(n) + " hat sich selbst erledigt", [n] {
                const TourSicht t = tourSicht();
                return t.id == "tut-menue" && (t.fertig || t.schritt >= n);
            }, 300));
            add(pause(0.3));
        };
        const auto zeigt = [&](int n) {
            add(pause(0.25));
            add(pruefe("Tour Spiel " + code + ": Schritt " + std::to_string(n + 1) + " zeigt auf ein sichtbares Bauteil", [n] {
                const TourSicht t = tourSicht();
                const ImVec2 g = ImGui::GetIO().DisplaySize;
                bool gut = t.aktiv && t.schritt == n && !t.ziele.empty();
                for (const ImRect& r : t.ziele) { gut = gut && r.Min.x >= -1.0F && r.Min.y >= -1.0F && r.Max.x <= g.x + 1.0F && r.Max.y <= g.y + 1.0F; }
                return gut;
            }));
        };
        add(tu("Tour Spiel " + code + ": alles zu, Skript-Modus, Tour starten", [=] {
            i18n::setLanguage(lang);
            std::error_code ec;
            fs::remove_all(fs::path(bhed::utf8Pfad(*paket)).parent_path(), ec);
            fs::create_directories(fs::path(bhed::utf8Pfad(*paket)).parent_path(), ec);
            me::setzeSpeichernDialog([paket] { return *paket; });
            me::schliesseAlleOhneSpeichern();
            g_app->leftMode = 0;
            (void)tourStarten("tut-menue", 0);
        }));
        zeigt(0);
        add(klick(tr(Str::ViewMenuEditor), "###main"));
        erledigt(1);
        zeigt(1);
        add(klick(tr(Str::MeAusSpiel)));
        add(pause(0.4));
        add(tippe("main.menu"));
        add(warteBis("Tour Spiel: main.menu in der Liste", [] { return findeMit("main.menu").x >= 0.0F; }, 900));
        add(klickAn("Tour Spiel: main.menu waehlen", [] { return findeMit("main.menu"); }));
        erledigt(2);
        add(warteBis("Tour Spiel: Menue offen, Elemente da", [] { return me::stand().items > 0; }, 600));
        zeigt(2);
        auto start = std::make_shared<ImVec2>(-1.0F, -1.0F);
        add(tu("Tour Spiel: ein Element auf der Flaeche aussuchen", [start] {
            for (const me::ItemLage& l : me::itemLagen()) {
                if (l.b > 40.0F && l.b < 400.0F && l.h > 10.0F && l.h < 100.0F && l.x > 0.0F && l.y > 0.0F && l.x + l.b + 12.0F < 640.0F) {
                    *start = ImVec2{l.x + l.b * 0.5F, l.y + l.h * 0.5F};
                }
            }
        }));
        add(klickAn("Tour Spiel: Klick auf das Element", [start] {
            const auto st = me::stand();
            return ImVec2{st.flaeche[0] + start->x / 640.0F * st.flaeche[2], st.flaeche[1] + start->y / 480.0F * st.flaeche[3]};
        }));
        add(pruefe("Tour Spiel: genau ein Element gewaehlt", [] { return me::stand().auswahl.size() == 1; }));
        add(zieheFlaecheVon("Tour Spiel: das Element ziehen", start, 8.0F, 0.0F));
        erledigt(3);
        zeigt(3);
        add(klick(tr(Str::MeBedienen)));
        erledigt(4);
        zeigt(4);
        add(klick(tr(Str::MeExportPk3)));
        add(pause(0.3));
        add(klickAn("Tour Spiel: Ausgeben im Aufklappmenue", [] {
            for (const elemente::Element& e : elemente::alle()) {
                if (e.label == tr(Str::MeExportPk3) && e.fenster.find("Popup") != std::string::npos) { return e.rect.GetCenter(); }
            }
            return ImVec2{-1.0F, -1.0F};
        }));
        erledigt(5);
        add(pruefe("Tour Spiel " + code + ": ganz durch, im Paket steht das geaenderte Menue", [paket] {
            std::error_code ec;
            return tourSicht().fertig && fs::file_size(fs::path(bhed::utf8Pfad(*paket)), ec) > 0 && !ec && !bhed::menue::lies(me::dokumentName(), me::stand().text, bhed::menue::LeseOptionen{}).menus.empty();
        }));
        add(klickBearbeiten());
        add(tu("Tour Spiel: zu", [] {
            tourSchliessen();
            me::schliesseAlleOhneSpeichern();
        }));
    };
    runde("eingestellt", sprache);
    for (const i18n::LanguageInfo& l : i18n::languages()) {
        if (std::string(l.code) == "cs" || std::string(l.code) == "ja") { runde(l.code, l.language); }
    }
    i18n::setLanguage(sprache);
    add(tu("Tour Spiel: Sprache und Dialog zurueck", [=] {
        i18n::setLanguage(sprache);
        me::setzeSpeichernDialog({});
    }));
    return s;
}

// Zwei Handbuchbilder fuer sich: In den schmalen Spalten des Bildlaufs liegt der Knopf unter dem Rand, und die Schritte davor
// sind auf die breite Spalte ausgelegt.
std::vector<Schritt> schritteBildNach() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto rollen = [] {
        return Schritt{"Bild: Spalte ans Ende rollen", [](int b) {
                           if (b > 0) { return b >= 3; }
                           for (ImGuiWindow* w : GImGui->Windows) {
                               if (w->WasActive && std::string(w->Name).find("/##inhalt_") != std::string::npos && std::string(w->Name).find("##add_liste") == std::string::npos) { ImGui::SetScrollY(w, w->ScrollMax.y); }
                           }
                           return false;
                       }};
    };
    add(tu("Bild: Rahmen", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneText("ui/rahmen.menu",
                             "menuDef\n{\n  name \"rahmen\"\n  rect 0 0 640 480\n  fullScreen 1\n  visible 1\n"
                             "  itemDef\n  {\n    name \"image\"\n    style WINDOW_STYLE_SHADER\n    rect 220 165 200 150\n    background \"gfx/menus/menu_blue\"\n    decoration\n    visible 1\n  }\n}\n");
        me::setzeInteraktion(false);
        me::waehle(0, false);
        me::setzeReiter(2);
    }));
    add(pause(0.6));
    add(klick(tr(Str::MlWaehlen), "##inhalt"));
    add(pause(0.8));
    add(pruefe("Bild: Die Auswahl ist offen", [] { return me::bildWahlOffen(); }));
    add(foto("menue_rahmen_wahl_sp"));
    add(klick(tr(Str::EditorCancel), "###bildwahl"));
    add(pause(0.3));
    add(tu("Bild: Add step", [] {
        me::schliesseAlleOhneSpeichern();
        (void)me::oeffneText("ui/leiste.menu", kLeisteText);
        me::setzeInteraktion(false);
        me::waehle(me::itemNummer("knopf"), false);
        me::setzeReiter(3);
    }));
    add(pause(0.6));
    add(klick("##tr_action"));
    add(pause(0.3));
    add(rollen());
    add(klick(tr(Str::MlAddStep)));
    add(pause(0.3));
    add(rollen());
    add(pause(0.3));
    add(pruefe("Bild: Die Liste von Add step ist zu sehen", [] { return finde("##neu_oftopen", "", 0, false) != nullptr; }));
    add(foto("menue_leiste_addstep"));
    add(tu("Bild: zu", [] { me::schliesseAlleOhneSpeichern(); }));
    return s;
}

std::vector<Schritt> schritteTourAlle() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto skala = std::make_shared<float>(1.0F);
    auto groesse = std::make_shared<std::pair<int, int>>();
    const i18n::Language sprache = i18n::currentLanguage();
    add(tu("3b Touren: Oberflaeche merken", [=] { *skala = g_app->uiScale; }));
    // Die Sprachen mit eigenem Layoutrisiko: die laengsten Texte (cs), die breiten Zeichen (ja), dazu 200 %. Die Schritte
    // merken sich die Beschriftungen beim Bauen, darum wird die Sprache auch dabei gesetzt.
    const std::pair<const char*, float> runden[] = {{"en", 1.0F}, {"cs", 1.0F}, {"ja", 1.0F}, {"cs", 2.0F}};
    for (const auto& [code, zoom] : runden) {
        const i18n::LanguageInfo* l = i18n::findLanguage(code);
        if (l == nullptr) { continue; }
        const i18n::Language lang = l->language;
        const float z = zoom;
        i18n::setLanguage(lang);
        add(tu(std::string("3b Touren: ") + code + " " + std::to_string(z), [=] {
            i18n::setLanguage(lang);
            g_app->uiScale = z;
            // 200 % ist ein 4K-Schirm. 1920 x 1080 bei 200 % (960 x 540 Punkte) reicht fuer Tutorial und Spalten zugleich nicht,
            // siehe Notizen; dort weicht das Tutorial aus, wo es geht.
            *groesse = z > 1.5F ? platform::hauptfensterGroesse(3840, 2160) : platform::hauptfensterGroesse(1920, 1080);
            g_app->settings.menueRechtsBreite = 0.0F;
        }));
        add(pause(0.6));
        for (auto& x : schritteTourMenue()) { add(std::move(x)); }
    }
    i18n::setLanguage(sprache);
    add(tu("3b Touren: zurueck", [=] {
        i18n::setLanguage(sprache);
        g_app->uiScale = *skala;
        if (groesse->first > 0) { (void)platform::hauptfensterGroesse(groesse->first, groesse->second); }
    }));
    return s;
}
