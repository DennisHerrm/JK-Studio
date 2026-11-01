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

// Bildschirmpunkt zu einer Stelle der Flaeche (virtuell, dazu Bildpunkte)
ImVec2 aufFlaeche(float vx, float vy, float px = 0, float py = 0) {
    const auto x = me::stand();
    return ImVec2{x.flaeche[0] + vx / 640.0F * x.flaeche[2] + px, x.flaeche[1] + vy / 480.0F * x.flaeche[3] + py};
}

const char* zeigerName(ImGuiMouseCursor c) {
    switch (c) {
        case ImGuiMouseCursor_Arrow: return "Pfeil";
        case ImGuiMouseCursor_TextInput: return "Text";
        case ImGuiMouseCursor_ResizeAll: return "Verschieben";
        case ImGuiMouseCursor_ResizeNS: return "senkrecht";
        case ImGuiMouseCursor_ResizeEW: return "waagerecht";
        case ImGuiMouseCursor_ResizeNESW: return "NO-SW";
        case ImGuiMouseCursor_ResizeNWSE: return "NW-SO";
        case ImGuiMouseCursor_Hand: return "Hand";
        default: return "anderer";
    }
}

// Maus an die Stelle, zwei Bilder warten, dann muss der gesetzte ImGui-Zeiger stimmen
Schritt zeigerAn(const std::string& was, std::function<ImVec2()> ort, ImGuiMouseCursor soll) {
    return Schritt{"rc608 Zeiger: " + was, [=](int b) {
                       if (b == 0) {
                           setzeMaus(ort());
                           return false;
                       }
                       if (b < 4) { return false; }
                       const ImGuiMouseCursor ist = ImGui::GetMouseCursor();
                       pruefeWahr(ist == soll, "rc608 Zeiger: " + was + " (soll " + zeigerName(soll) + ", ist " + zeigerName(ist) + ")");
                       return true;
                   }};
}

// rc608 Punkt 16: ueber dem Text und in Eingabefeldern der Text-Cursor, im Zeilenrand der Pfeil
std::vector<Schritt> schritteTextZeiger() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("rc608 Zeiger: Probe oeffnen, Reiter Text-Editor", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneText("zeiger.menu", kZugText);
        me::setzeReiter(1);
    }));
    add(pause(0.3));
    const auto codeFenster = [] {
        for (ImGuiWindow* w : GImGui->Windows) {
            if (w->WasActive && std::string(w->Name).find("/##code_") != std::string::npos) { return w; }
        }
        return static_cast<ImGuiWindow*>(nullptr);
    };
    add(zeigerAn("ueber dem Text", [codeFenster] {
        ImGuiWindow* w = codeFenster();
        return w != nullptr ? ImVec2{w->Pos.x + w->Size.x * 0.4F, w->Pos.y + w->Size.y * 0.4F} : ImVec2{};
    }, ImGuiMouseCursor_TextInput));
    add(zeigerAn("im Zeilenrand", [codeFenster] {
        ImGuiWindow* w = codeFenster();
        return w != nullptr ? ImVec2{w->Pos.x + 6.0F, w->Pos.y + w->Size.y * 0.4F} : ImVec2{};
    }, ImGuiMouseCursor_Arrow));
    add(klickAn("rc608 Zeiger: in den Text", [codeFenster] {
        ImGuiWindow* w = codeFenster();
        return w != nullptr ? ImVec2{w->Pos.x + w->Size.x * 0.4F, w->Pos.y + w->Size.y * 0.4F} : ImVec2{};
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_F, "Strg+F"));
    add(pause(0.2));
    add(pruefe("rc608 Zeiger: das Suchfeld hat den Fokus", [] { return ImGui::GetIO().WantTextInput; }));
    add(zeigerAn("ueber dem Suchfeld", [codeFenster] {
        for (ImGuiWindow* w : GImGui->Windows) {
            if (w->WasActive && std::string(w->Name).find("/##codeBereich_") != std::string::npos) { return ImVec2{w->Pos.x + 30.0F, w->Pos.y + 10.0F}; }
        }
        return ImVec2{};
    }, ImGuiMouseCursor_TextInput));
    add(tu("rc608 Zeiger: aufraeumen", [] {
        me::schliesseAlleOhneSpeichern();
        me::setzeReiter(0);
    }));
    return s;
}

// rc608 Punkt 8 (shank: "wenn ich ueber die raender fahre ... sollte die Maus das skalier symbol bekommen")
std::vector<Schritt> zeigerTabelle(const std::string& stufe) {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto an = [](float vx, float vy, float px = 0, float py = 0) {
        return [=] { return aufFlaeche(vx, vy, px, py); };
    };
    const std::string st = stufe + ": ";
    // Das Element frei: rect 400 300 100 50
    add(tu("rc608 Zeiger: frei waehlen", [] { me::waehle(me::itemNummer("frei"), false); }));
    add(pause(0.1));
    add(zeigerAn(st + "Griff oben links", an(400, 300), ImGuiMouseCursor_ResizeNWSE));
    add(zeigerAn(st + "Griff oben Mitte", an(450, 300), ImGuiMouseCursor_ResizeNS));
    add(zeigerAn(st + "Griff oben rechts", an(500, 300), ImGuiMouseCursor_ResizeNESW));
    add(zeigerAn(st + "Griff links Mitte", an(400, 325), ImGuiMouseCursor_ResizeEW));
    add(zeigerAn(st + "Griff rechts Mitte", an(500, 325), ImGuiMouseCursor_ResizeEW));
    add(zeigerAn(st + "Griff unten links", an(400, 350), ImGuiMouseCursor_ResizeNESW));
    add(zeigerAn(st + "Griff unten Mitte", an(450, 350), ImGuiMouseCursor_ResizeNS));
    add(zeigerAn(st + "Griff unten rechts", an(500, 350), ImGuiMouseCursor_ResizeNWSE));
    add(zeigerAn(st + "Rand oben, ohne Griff", an(425, 300), ImGuiMouseCursor_ResizeNS));
    add(zeigerAn(st + "Rand unten, ohne Griff", an(475, 350), ImGuiMouseCursor_ResizeNS));
    add(zeigerAn(st + "Rand links, ohne Griff", an(400, 312), ImGuiMouseCursor_ResizeEW));
    add(zeigerAn(st + "Rand rechts, ohne Griff", an(500, 338), ImGuiMouseCursor_ResizeEW));
    add(zeigerAn(st + "auf dem gewaehlten Element", an(450, 325), ImGuiMouseCursor_ResizeAll));
    add(zeigerAn(st + "auf einem anderen Element", an(150, 150), ImGuiMouseCursor_Arrow));
    add(zeigerAn(st + "auf leerer Flaeche", an(50, 50), ImGuiMouseCursor_Arrow));
    add(tu("rc608 Zeiger: frei sperren", [] { me::sperreItem(me::itemNummer("frei"), true); }));
    add(zeigerAn(st + "gesperrt: am Griff kein Pfeil", an(500, 350), ImGuiMouseCursor_Arrow));
    add(zeigerAn(st + "gesperrt: innen kein Verschieben", an(450, 325), ImGuiMouseCursor_Arrow));
    add(tu("rc608 Zeiger: frei entsperren", [] { me::sperreItem(me::itemNummer("frei"), false); }));
    return s;
}

std::vector<Schritt> schritteZeiger() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("rc608 Zeiger: Probe oeffnen", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneText("zeiger.menu", kZugText);
        me::setzeInteraktion(false);
    }));
    add(pause(0.5));
    for (auto& x : zeigerTabelle("100 %")) { add(std::move(x)); }
    add(foto("menue_griffe"));

    // Beim Ziehen bleibt der Zeiger, auch wenn die Maus den Griff verlaesst
    auto start = std::make_shared<std::string>();
    add(tu("rc608 Zeiger: frei waehlen", [start] {
        me::waehle(me::itemNummer("frei"), false);
        *start = rectVon("frei");
    }));
    add(pause(0.1));
    add(Schritt{"rc608 Zeiger: Griff rechts unten ziehen, Zeiger bleibt", [start](int b) {
                    ImGuiIO& io = ImGui::GetIO();
                    const ImVec2 p = aufFlaeche(500, 350);
                    if (b == 0) { setzeMaus(p); return false; }
                    if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
                    if (b == 4) { setzeMaus(ImVec2{p.x + 40, p.y + 20}); return false; }
                    if (b == 7) { setzeMaus(ImVec2{p.x + 80, p.y + 60}); return false; }
                    if (b == 10) {
                        pruefeWahr(ImGui::GetMouseCursor() == ImGuiMouseCursor_ResizeNWSE, std::string("rc608 Zeiger: waehrend des Ziehens NW-SO, ist ") + zeigerName(ImGui::GetMouseCursor()));
                        io.AddMouseButtonEvent(0, false);
                        return false;
                    }
                    return b >= 13;
                }});
    add(pruefe("rc608 Zeiger: der Griff hat gezogen", [start] { return rectVon("frei") != *start; }));
    add(tu("rc608 Zeiger: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
    add(pause(0.2));

    // Ein Rand ohne Griff skaliert wirklich, auf der Kante, die er zeigt
    add(ziehFlaeche("rc608 Zeiger: Rand rechts (ohne Griff) ziehen", 500, 338, 0, 0, 40, 0));
    add(pruefe("rc608 Zeiger: Rand rechts aendert nur die Breite", [] {
        const std::string r = rectVon("frei");
        return r.rfind("400 300 ", 0) == 0 && r.find(" 50") != std::string::npos && r != "400 300 100 50";
    }));
    add(tu("rc608 Zeiger: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
    add(pause(0.2));

    // Am Rand der Flaeche: ein Element ueber den ganzen Schirm, alle Griffe treffbar und ohne Hilfslinie
    add(tu("rc608 Zeiger: Vollbild-Element", [] {
        (void)me::oeffneText("rand.menu", "menuDef\n{\n  name \"rand\"\n  rect 0 0 640 480\n  visible 1\n"
                                          "  itemDef\n  {\n    name \"voll\"\n    type ITEM_TYPE_BUTTON\n    rect 0 0 640 480\n    style WINDOW_STYLE_FILLED\n    backcolor 0.2 0.2 0.6 1\n    visible 1\n  }\n}\n");
    }));
    add(pause(0.3));
    add(tu("rc608 Zeiger: voll waehlen", [] { me::waehle(me::itemNummer("voll"), false); }));
    add(pause(0.1));
    const auto an = [](float vx, float vy, float px = 0, float py = 0) { return [=] { return aufFlaeche(vx, vy, px, py); }; };
    add(zeigerAn("Rand der Flaeche: x=0, y=0", an(0, 0), ImGuiMouseCursor_ResizeNWSE));
    add(zeigerAn("Rand der Flaeche: x=640, y=480", an(640, 480), ImGuiMouseCursor_ResizeNWSE));
    add(zeigerAn("Rand der Flaeche: x=640, Griff von aussen", an(640, 240, 3), ImGuiMouseCursor_ResizeEW));
    add(zeigerAn("Rand der Flaeche: y=480, Griff von aussen", an(320, 480, 0, 3), ImGuiMouseCursor_ResizeNS));
    add(zeigerAn("Rand der Flaeche: y=0, Griff von oben", an(320, 0, 0, -3), ImGuiMouseCursor_ResizeNS));
    add(foto("menue_griffe_rand"));
    add(ziehFlaeche("rc608 Zeiger: Griff bei y=0 ziehen", 320, 0, 0, -3, 0, 60));
    add(pruefe("rc608 Zeiger: Griff im Lineal zieht das Element, keine Hilfslinie", [] { return rectVon("voll") != "0 0 640 480" && rectVon("voll").rfind("0 ", 0) == 0; }));
    add(tu("rc608 Zeiger: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
    add(pause(0.2));
    add(ziehFlaeche("rc608 Zeiger: Griff bei x=640 von aussen ziehen", 640, 240, 3, 0, -50, 0));
    add(pruefe("rc608 Zeiger: Griff bei x=640 zieht", [] { return rectVon("voll") != "0 0 640 480"; }));
    add(tu("rc608 Zeiger: aufraeumen", [] { me::schliesseAlleOhneSpeichern(); }));
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
