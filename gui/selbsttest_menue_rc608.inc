// selbsttest_menue_rc608.inc - Menue-Editor rc608 Teil 2: gezielte Pruefungen (BHED_MENUE_NUR=...).
// Eingebunden in namespace menuetest (selbsttest_menue.inc).

// Die Mitte des ersten aktiven Fensters, dessen Name `teil` enthaelt
ImVec2 fensterMitte(const std::string& teil) {
    for (ImGuiWindow* w : GImGui->Windows) {
        if (w->WasActive && std::string(w->Name).find(teil) != std::string::npos) { return ImVec2{w->Pos.x + w->Size.x * 0.5F, w->Pos.y + w->Size.y * 0.5F}; }
    }
    diag::detail("Knopftest: kein Fenster mit " + teil);
    return ImVec2{};
}

// Die Reiter der rechten Spalte: (Name, Rechteck); ganz = kein Reiter ist gestaucht
struct ReiterLage {
    std::vector<std::pair<std::string, ImRect>> reiter;
    bool ganz = true;
    float rechts = 0;      // rechter Rand des Felds
};
ReiterLage reiterLage() {
    ReiterLage r;
    for (int i = 0; i < GImGui->TabBars.GetMapSize(); ++i) {
        ImGuiTabBar* b = GImGui->TabBars.TryGetMapData(i);
        if (b == nullptr || b->Tabs.Size != 4 || b->CurrFrameVisible < GImGui->FrameCount - 1) { continue; }
        const auto name = [&](ImGuiTabItem& t) {
            const char* n = ImGui::TabBarGetTabName(b, &t);
            return std::string(n, ImGui::FindRenderedTextEnd(n));
        };
        bool unsere = false;
        for (ImGuiTabItem& t : b->Tabs) { unsere = unsere || name(t) == tr(Str::MeReiterHierarchie); }
        if (!unsere) { continue; }
        r.rechts = b->BarRect.Max.x;
        for (ImGuiTabItem& t : b->Tabs) {
            r.reiter.emplace_back(name(t), ImRect(b->BarRect.Min.x + t.Offset, b->BarRect.Min.y, b->BarRect.Min.x + t.Offset + t.Width, b->BarRect.Max.y));
            r.ganz = r.ganz && t.Width + 0.5F >= t.ContentWidth;
        }
    }
    return r;
}

// rc608 Punkt 1 (shank: "das muss nicht Text sondern Text editor heissen"), in allen Sprachen und ungestaucht
std::vector<Schritt> schritteReiter() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("rc608 Reiter: Probe oeffnen", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneText("reiter.menu", "menuDef\n{\n  name \"reiter\"\n  rect 0 0 640 480\n  visible 1\n}\n");
        me::setzeReiter(1);
    }));
    add(pause(0.3));
    static const std::map<std::string, std::string> kSoll = {{"en", "Text editor"},  {"de", "Text-Editor"},     {"zh-Hans", "文本编辑器"},
                                                             {"ja", "テキストエディター"}, {"es", "Editor de texto"}, {"cs", "Textový editor"}};
    const i18n::Language vorher = i18n::currentLanguage();
    for (const i18n::LanguageInfo& l : i18n::languages()) {
        const i18n::Language sprache = l.language;
        const std::string kurz = l.code;
        add(tu("rc608 Reiter: Sprache " + kurz, [sprache] { i18n::setLanguage(sprache); }));
        add(pause(0.3));
        add(pruefe("rc608 Reiter: " + kurz + " heisst der zweite Reiter wie gewuenscht, nichts gestaucht", [kurz] {
            const ReiterLage r = reiterLage();
            const auto soll = kSoll.find(kurz);
            bool gut = r.reiter.size() == 4 && soll != kSoll.end() && r.reiter[1].first == soll->second && r.ganz;
            for (const auto& [name, rect] : r.reiter) {
                gut = gut && rect.Max.x <= r.rechts + 0.5F;
                diag::info("Knopftest: Reiter " + kurz + " " + name + " Breite " + std::to_string(rect.GetWidth()));
            }
            return gut;
        }));
    }
    add(foto("menue_reiter"));
    add(tu("rc608 Reiter: aufraeumen", [vorher] {
        i18n::setLanguage(vorher);
        me::schliesseAlleOhneSpeichern();
        me::setzeReiter(0);
    }));
    return s;
}

// rc608 Punkt 17 (Tester: "I need a replace all option when I do CTRL+F. Like how it is in the scripts section")
std::vector<Schritt> schritteErsetzen() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    static const char* const kText = "menuDef\n{\n  name \"ers\"\n  rect 0 0 640 480\n  visible 1\n"
                                     "  itemDef\n  {\n    name \"a\"\n    type ITEM_TYPE_TEXT\n    rect 10 10 100 30\n    text \"Probe\"\n    visible 1\n  }\n"
                                     "  itemDef\n  {\n    name \"b\"\n    type ITEM_TYPE_TEXT\n    rect 10 50 100 30\n    text \"Probe\"\n    visible 1\n  }\n"
                                     "}\n";
    const auto zaehle = [](const std::string& t, const std::string& w) {
        int n = 0;
        for (std::size_t p = t.find(w); p != std::string::npos; p = t.find(w, p + w.size())) { ++n; }
        return n;
    };
    auto tiefe = std::make_shared<int>(0);
    add(tu("rc608 Ersetzen: zwei Probedateien, Reiter Text", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneText("zwei.menu", kText);
        (void)me::oeffneText("eins.menu", kText);
        me::setzeReiter(1);
    }));
    add(pause(0.4));
    add(klickAn("rc608 Ersetzen: in den Text-Editor", [] { return fensterMitte("/##code_"); }));
    add(pause(0.2));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_F, "Strg+F"));
    add(pause(0.2));
    add(foto("menue_suchen_strgf"));
    add(pruefe("rc608 Ersetzen: Strg+F oeffnet nicht das Suchfenster der Skripte", [] { return !g_app->findOpen; }));
    add(tippe("Probe"));
    add(klick("##ersatz"));
    add(tippe("Neu"));
    add(tu("rc608 Ersetzen: Undo-Tiefe merken", [tiefe] { *tiefe = me::stand().undoSchritte; }));
    add(klick(tr(Str::FindReplaceAll)));
    add(pause(0.2));
    add(pruefe("rc608 Ersetzen: Alle ersetzen hinter Strg+F ersetzt alle Treffer der Datei", [zaehle] {
        const std::string t = me::stand().text;
        return zaehle(t, "\"Probe\"") == 0 && zaehle(t, "\"Neu\"") == 2;
    }));
    add(pruefe("rc608 Ersetzen: ein Undo-Schritt", [tiefe] { return me::stand().undoSchritte == *tiefe + 1; }));
    add(pruefe("rc608 Ersetzen: meldet die Anzahl", [] {
        char m[120];
        std::snprintf(m, sizeof(m), tr(Str::FindReplacedN), 2);
        return me::letzteMeldung() == m;
    }));
    add(foto("menue_suchen_ersetzt"));
    add(tu("rc608 Ersetzen: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
    add(pause(0.2));
    add(pruefe("rc608 Ersetzen: Rueckgaengig stellt alles her", [zaehle] {
        const std::string t = me::stand().text;
        return zaehle(t, "\"Probe\"") == 2 && zaehle(t, "\"Neu\"") == 0;
    }));
    add(klick(tr(Str::FindReplaceBtn)));
    add(pause(0.2));
    add(pruefe("rc608 Ersetzen: Ersetzen nimmt einen Treffer", [zaehle] { return zaehle(me::stand().text, "\"Neu\"") == 1; }));
    add(tu("rc608 Ersetzen: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
    add(pause(0.2));
    add(klick(tr(Str::FindReplaceAllDocs)));
    add(pause(0.2));
    add(pruefe("rc608 Ersetzen: in allen offenen Dateien", [zaehle] {
        char m[160];
        std::snprintf(m, sizeof(m), trN(Str::FindReplacedDocs, 2), 4, 2);
        return zaehle(me::stand().text, "\"Neu\"") == 2 && me::letzteMeldung() == m;
    }));
    add(tu("rc608 Ersetzen: aufraeumen", [] {
        me::schliesseAlleOhneSpeichern();
        me::setzeReiter(0);
    }));
    return s;
}

// rc608 Punkt 15 (Tester: "Noticed it doesn't display shaders"): Bilder werden aufgeloest wie im Spiel.
std::vector<Schritt> schritteShader() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("rc608 Shader: Forcemenue aus dem Spiel oeffnen", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneAusSpiel("ui/ingameforceselect.menu");
    }));
    add(pause(2.0));
    add(foto("menue_forceselect"));
    add(pruefe("rc608 Shader: keine Bild-Warnung im Forcemenue", [] {
        const auto x = me::stand();
        if (x.menus == 0) { return true; }   // Jedi Outcast hat dieses Menue nicht
        int bilder = 0;
        for (const std::string& d : x.diagnosen) {
            if (d.find("gfx/") != std::string::npos) {
                diag::info("Menue-Editor Diagnose: " + d);
                ++bilder;
            }
        }
        diag::info("Knopftest: Forcemenue Warnungen " + std::to_string(x.diagWarnungen) + ", davon Bilder " + std::to_string(bilder) + ", Bilder geladen " + std::to_string(x.bilderGeladen));
        return bilder == 0 && x.bilderGeladen >= 20;
    }));
    // Tabelle ueber alle Originalmenues der eingestellten Spieldaten. Was fehlt, muss genau das sein, was ein unabhaengiges
    // Skript nach den Regeln der Engine auch nicht findet (SP/rc608/bildtabelle.py, Vergleich der Zeilen ausserhalb)
    add(pruefe("rc608 Shader: Tabelle ueber alle Originalmenues", [] {
        const std::vector<me::BildZeile> tab = me::bildTabelle();
        int da = 0;
        for (const me::BildZeile& b : tab) {
            if (b.da) {
                ++da;
            } else {
                diag::info("Bildtabelle FEHLT " + b.datei + ":" + std::to_string(b.zeile) + " " + b.art + " " + b.name);
            }
        }
        diag::info("Knopftest: Bildtabelle " + std::to_string(tab.size()) + " Eintraege, " + std::to_string(da) + " Treffer, " + std::to_string(tab.size() - static_cast<std::size_t>(da)) + " fehlen");
        return !tab.empty();
    }));
    return s;
}
