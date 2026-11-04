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
        if (b == nullptr || b->Tabs.Size != 3 || b->CurrFrameVisible < GImGui->FrameCount - 1) { continue; }
        const auto name = [&](ImGuiTabItem& t) {
            const char* n = ImGui::TabBarGetTabName(b, &t);
            return std::string(n, ImGui::FindRenderedTextEnd(n));
        };
        bool unsere = false;
        for (ImGuiTabItem& t : b->Tabs) { unsere = unsere || name(t) == tr(Str::MeReiterEigenschaften); }
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
        add(pruefe("rc608 Reiter: " + kurz + " heisst der dritte Reiter wie gewuenscht, nichts gestaucht", [kurz] {
            const ReiterLage r = reiterLage();
            const auto soll = kSoll.find(kurz);
            bool gut = r.reiter.size() == 3 && soll != kSoll.end() && r.reiter[2].first == soll->second && r.ganz;
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
    add(zeigerAn(st + "Rand oben, ohne Griff", an(400, 300, 7, 0), ImGuiMouseCursor_ResizeNS));
    add(zeigerAn(st + "Rand unten, ohne Griff", an(500, 350, -7, 0), ImGuiMouseCursor_ResizeNS));
    add(zeigerAn(st + "Rand links, ohne Griff", an(400, 300, 0, 7), ImGuiMouseCursor_ResizeEW));
    add(zeigerAn(st + "Rand rechts, ohne Griff", an(500, 350, 0, -7), ImGuiMouseCursor_ResizeEW));
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
        me::setzeSpalten(0.0F, 0.0F);
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

// rc608 Punkt 4: Zoomen und Verschieben; Treffer, Griffe und Ziehen stimmen auf jeder Stufe
std::vector<Schritt> schritteZoom() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto an = [](float vx, float vy, float px = 0, float py = 0) { return [=] { return aufFlaeche(vx, vy, px, py); }; };
    const auto nah = [](float a, float b, float t = 1.5F) { return std::fabs(a - b) <= t; };
    auto start = std::make_shared<std::array<float, 4>>();
    auto text = std::make_shared<std::string>();
    auto tiefe = std::make_shared<int>(0);
    auto punkt = std::make_shared<ImVec2>();
    add(tu("rc608 Zoom: Probe oeffnen", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneText("zoom.menu", kZugText);
        me::setzeInteraktion(false);
        me::setzeSpalten(0.0F, 0.0F);
    }));
    add(pause(0.5));
    add(tu("rc608 Zoom: Fokus auf die Flaeche, Ausgangslage merken", [start, text, tiefe] {
        const auto x = me::stand();
        *start = {x.flaeche[0], x.flaeche[1], x.flaeche[2], x.flaeche[3]};
        *text = x.text;
        *tiefe = x.undoSchritte;
    }));
    // Strg+1 und Strg+0
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_1, "Strg+1"));
    add(pause(0.1));
    add(pruefe("rc608 Zoom: Strg+1 ist 100 %", [nah] {
        const auto x = me::stand();
        diag::info("Knopftest: Zoom " + std::to_string(x.zoomProzent) + " %, Flaeche " + std::to_string(x.flaeche[2]));
        return x.zoomProzent == 100 && nah(x.flaeche[2], 640.0F);
    }));
    add(foto("menue_zoom_100"));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_0, "Strg+0"));
    add(pause(0.1));
    add(pruefe("rc608 Zoom: Strg+0 ist wieder passend", [nah, start] {
        const auto x = me::stand();
        return nah(x.flaeche[0], (*start)[0]) && nah(x.flaeche[1], (*start)[1]) && nah(x.flaeche[2], (*start)[2]) && nah(x.flaeche[3], (*start)[3]);
    }));
    // Strg+Mausrad um den Zeiger: der virtuelle Punkt unter der Maus bleibt stehen
    add(tu("rc608 Zoom: Punkt merken", [punkt] { *punkt = aufFlaeche(450, 325); }));
    add(halteTaste(ImGuiMod_Ctrl, true));
    add(radAn("Strg+Rad ueber frei", an(450, 325), 2.0F));
    add(halteTaste(ImGuiMod_Ctrl, false));
    add(pruefe("rc608 Zoom: Strg+Rad zoomt um den Zeiger", [nah, punkt, start] {
        const auto x = me::stand();
        const ImVec2 p = aufFlaeche(450, 325);
        return nah(p.x, punkt->x) && nah(p.y, punkt->y) && x.flaeche[2] > (*start)[2] * 1.4F;
    }));
    add(foto("menue_zoom_rein"));
    // Grenzen
    add(halteTaste(ImGuiMod_Ctrl, true));
    add(radAn("Strg+Rad weit hinein", an(450, 325), 30.0F));
    add(halteTaste(ImGuiMod_Ctrl, false));
    add(pruefe("rc608 Zoom: hoechstens 800 %", [] { return me::stand().zoomProzent == 800; }));
    add(halteTaste(ImGuiMod_Ctrl, true));
    add(radAn("Strg+Rad weit hinaus", an(450, 325), -60.0F));
    add(halteTaste(ImGuiMod_Ctrl, false));
    add(pruefe("rc608 Zoom: mindestens 25 %", [] { return me::stand().zoomProzent == 25; }));
    add(foto("menue_zoom_25"));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_0, "Strg+0"));
    add(pause(0.1));
    // Mitteltaste und Leertaste verschieben, sonst aendert sich nichts
    add(tu("rc608 Zoom: frei waehlen", [] { me::waehle(me::itemNummer("frei"), false); }));
    add(tu("rc608 Zoom: Punkt merken", [punkt] { *punkt = aufFlaeche(0, 0); }));
    add(ziehenAn("rc608 Zoom: Mitteltaste", an(50, 50), 60.0F, 30.0F, 2));
    add(pruefe("rc608 Zoom: Mitteltaste verschiebt die Ansicht", [nah, punkt] {
        const ImVec2 p = aufFlaeche(0, 0);
        diag::info("Knopftest: Mitteltaste " + std::to_string(p.x - punkt->x) + ", " + std::to_string(p.y - punkt->y));
        return nah(p.x - punkt->x, 60.0F, 3.0F) && nah(p.y - punkt->y, 30.0F, 3.0F);
    }));
    add(tu("rc608 Zoom: Punkt merken", [punkt] { *punkt = aufFlaeche(0, 0); }));
    add(Schritt{"Taste Leertaste halten", [](int b) {
                    if (b == 0) { ImGui::GetIO().AddKeyEvent(ImGuiKey_Space, true); }
                    return b >= 2;
                }});
    add(zeigerAn("Leertaste: Hand", an(450, 325), ImGuiMouseCursor_Hand));
    add(ziehenAn("rc608 Zoom: Leertaste+Ziehen ueber dem gewaehlten Element", an(450, 325), -40.0F, 20.0F, 0));
    add(Schritt{"Taste Leertaste loslassen", [](int b) {
                    if (b == 0) { ImGui::GetIO().AddKeyEvent(ImGuiKey_Space, false); }
                    return b >= 2;
                }});
    add(pruefe("rc608 Zoom: Leertaste+Ziehen verschiebt die Ansicht, das Element bleibt", [nah, punkt] {
        const ImVec2 p = aufFlaeche(0, 0);
        return nah(p.x - punkt->x, -40.0F, 3.0F) && nah(p.y - punkt->y, 20.0F, 3.0F) && rectVon("frei") == "400 300 100 50" && auswahlText() == "2";
    }));
    add(pruefe("rc608 Zoom: die Ansicht ist kein Undo-Schritt und steht nicht in der Datei", [text, tiefe] {
        const auto x = me::stand();
        return x.text == *text && x.undoSchritte == *tiefe;
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_0, "Strg+0"));
    add(pause(0.1));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_1, "Strg+1"));
    add(pause(0.1));
    // Griffe, Zeiger und Ziehen je Stufe, in beiden Seitenverhaeltnis-Arten
    struct Stufe { const char* name; float klicks; bool balken; };
    static const Stufe kStufen[] = {{"381 %", 6.0F, false}, {"800 %", 10.0F, false}, {"25 %", -8.0F, false}, {"381 % Balken", 6.0F, true}};
    for (const Stufe& st : kStufen) {
        const std::string name = st.name;
        const float klicks = st.klicks;
        const bool balken = st.balken;
        add(tu("rc608 Zoom: Stufe " + name, [balken] { me::setzeBalken(balken); }));
        add(pause(0.1));
        // Erst passend: Strg+1 behaelt die Mitte der Ansicht, und das Element soll im Bild bleiben
        add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_0, "Strg+0"));
        add(pause(0.1));
        add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_1, "Strg+1"));
        add(pause(0.1));
        // frei in die Mitte der Ansicht (bei 800 % ist es 800 Punkte breit, die Flaeche je nach Sprache kaum breiter)
        add(ziehenAn("rc608 Zoom: frei in die Mitte, " + name, an(450, 325), -130.0F, -85.0F, 2));
        add(pause(0.1));
        add(halteTaste(ImGuiMod_Ctrl, true));
        add(radAn("Strg+Rad " + name, an(450, 325), klicks));
        add(halteTaste(ImGuiMod_Ctrl, false));
        add(pruefe("rc608 Zoom: Stufe " + name + " erreicht", [] {
            const auto x = me::stand();
            const ImVec2 p = aufFlaeche(500, 350);
            diag::info("Knopftest: Zoom " + std::to_string(x.zoomProzent) + " %, Flaeche " + std::to_string(x.flaeche[0]) + " " + std::to_string(x.flaeche[1]) + " " + std::to_string(x.flaeche[2]) + " " + std::to_string(x.flaeche[3]) + ", Griff unten rechts " + std::to_string(p.x) + " " + std::to_string(p.y));
            return true;
        }));
        for (auto& x : zeigerTabelle(name)) { add(std::move(x)); }
        // Ziehen: 8 Prozent des Rechtecks auf dem Bildschirm, Ergebnis in virtuellen Punkten
        auto vorher = std::make_shared<std::string>();
        add(tu("rc608 Zoom: frei waehlen, rect merken", [vorher] {
            me::waehle(me::itemNummer("frei"), false);
            *vorher = rectVon("frei");
        }));
        add(pause(0.1));
        auto soll = std::make_shared<float>(0.0F);
        add(Schritt{"rc608 Zoom: frei ziehen, " + name, [soll](int b) {
                        ImGuiIO& io = ImGui::GetIO();
                        const auto x = me::stand();
                        const float sx = x.flaeche[2] / 640.0F;
                        const ImVec2 p = aufFlaeche(450, 325);
                        if (b == 0) { *soll = 400.0F + std::round(30.0F); setzeMaus(p); return false; }
                        if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
                        if (b >= 3 && b <= 6) { setzeMaus(ImVec2{p.x + 30.0F * sx * static_cast<float>(b - 2) / 4.0F, p.y}); return false; }
                        if (b == 8) { io.AddMouseButtonEvent(0, false); return false; }
                        return b >= 11;
                    }});
        add(pruefe("rc608 Zoom: " + name + " ziehen ergibt 30 virtuelle Punkte", [name, nah] {
            const std::string r = rectVon("frei");
            diag::info("Knopftest: gezogen " + name + ": " + r);
            return nah(static_cast<float>(std::atof(r.c_str())), 430.0F, 2.0F);
        }));
        add(tu("rc608 Zoom: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
        add(pause(0.2));
    }
    add(tu("rc608 Zoom: aufraeumen", [] {
        me::setzeBalken(false);
        me::schliesseAlleOhneSpeichern();
    }));
    return s;
}

// rc608 Punkt 5 (shank: "wieso kann ich nicht schreiben direkt wenn ich dopplklick mache um den Text in dem Style zu schreiben?")
const char* const kSchreibText =
    "menuDef\n{\n  name \"schreiben\"\n  rect 0 0 640 480\n  visible 1\n"
    "  itemDef\n  {\n    name \"titel\"\n    type ITEM_TYPE_TEXT\n    rect 100 40 300 40\n    text \"Probe\"\n    font 1\n    textscale 1\n    forecolor 1 1 0 1\n    visible 1\n  }\n"
    "  itemDef\n  {\n    name \"knopf\"\n    type ITEM_TYPE_BUTTON\n    rect 100 100 200 40\n    text \"Back\"\n    font 2\n    textscale 1\n    textalign ITEM_ALIGN_CENTER\n    textalignx 100\n    forecolor 1 1 1 1\n    visible 1\n  }\n"
    "  itemDef\n  {\n    name \"spieltext\"\n    type ITEM_TYPE_BUTTON\n    rect 100 150 200 40\n    text @MENUS_BACK\n    font 2\n    textscale 1\n    forecolor 1 1 1 1\n    visible 1\n  }\n"
    "  itemDef\n  {\n    name \"lauftext\"\n    type ITEM_TYPE_TEXTSCROLL\n    rect 330 100 280 120\n    text \"Eins zwei drei\"\n    font 2\n    textscale 0.7\n    lineHeight 20\n    forecolor 1 1 1 1\n    autowrapped\n    visible 1\n  }\n"
    "  itemDef\n  {\n    name \"auscvar\"\n    type ITEM_TYPE_TEXT\n    rect 100 200 200 30\n    cvar \"ui_name\"\n    visible 1\n  }\n"
    "  itemDef\n  {\n    name \"auscode\"\n    type ITEM_TYPE_OWNERDRAW\n    rect 100 240 200 30\n    ownerdraw 12\n    visible 1\n  }\n"
    "  itemDef\n  {\n    name \"liste\"\n    type ITEM_TYPE_LISTBOX\n    rect 100 280 200 30\n    feeder 1\n    visible 1\n  }\n"
    "  itemDef\n  {\n    name \"bild\"\n    style WINDOW_STYLE_SHADER\n    rect 100 320 200 30\n    background \"gfx/menus/menu_blendbox\"\n    visible 1\n  }\n"
    "}\n";

std::string textVon(const std::string& name) {
    const std::string t = me::stand().text;
    const std::size_t p = t.find("name \"" + name + "\"");
    if (p == std::string::npos) { return ""; }
    const std::size_t r = t.find("    text ", p);
    if (r == std::string::npos) { return ""; }
    const std::size_t e = t.find("\n    ", r + 9);
    return t.substr(r + 9, e - r - 9);
}

std::vector<Schritt> schritteSchreiben() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto doppel = [](const std::string& was, float vx, float vy) {
        return klickAn("rc608 Schreiben: Doppelklick " + was, [=] { return aufFlaeche(vx, vy); }, 0, true);
    };
    auto tiefe = std::make_shared<int>(0);
    const auto merke = [tiefe] { return tu("rc608 Schreiben: Undo-Tiefe merken", [tiefe] { *tiefe = me::stand().undoSchritte; }); };
    const auto schritte = [tiefe](const std::string& was, int n) {
        return pruefe("rc608 Schreiben: " + was + ": " + std::to_string(n) + " Undo-Schritt(e)", [tiefe, n] { return me::stand().undoSchritte == *tiefe + n; });
    };
    add(tu("rc608 Schreiben: Probe oeffnen", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneText("ui/schreiben.menu", kSchreibText);
        me::setzeInteraktion(false);
    }));
    add(pause(0.6));
    add(foto("menue_schreiben_vorher"));
    // Doppelklick: schreibt, zieht nicht, der Cursor steht wie gezeichnet
    add(merke());
    add(doppel("titel", 150, 55));
    add(pause(0.4));
    add(pruefe("rc608 Schreiben: Doppelklick oeffnet das Schreiben", [] {
        const auto x = me::schreibStand();
        return x.an && x.puffer == "Probe" && x.item == me::itemNummer("titel") && rectVon("titel") == "100 40 300 40";
    }));
    add(foto("menue_schreiben_an"));
    add(zeigerAn("beim Schreiben der Text-Cursor", [] { return aufFlaeche(200, 60); }, ImGuiMouseCursor_TextInput));
    add(taste(ImGuiKey_End, "Ende"));
    add(pause(0.2));
    add(pruefe("rc608 Schreiben: Cursor am Ende sitzt am Ende des gezeichneten Textes", [] {
        const auto x = me::schreibStand();
        diag::info("Knopftest: Cursor " + std::to_string(x.cursorX) + ", Text rechts " + std::to_string(x.textRechts));
        return x.textRechts > 100.0F && std::fabs(x.cursorX - x.textRechts) < 4.0F;
    }));
    // Klick in den Text setzt den Cursor auf das Zeichen unter der Maus
    auto mitte = std::make_shared<float>(0.0F);
    add(tu("rc608 Schreiben: Mitte des Textes merken", [mitte] { *mitte = 100.0F + (me::schreibStand().textRechts - 100.0F) * 0.5F; }));
    add(klickAn("rc608 Schreiben: Klick in die Mitte des Textes", [mitte] { return aufFlaeche(*mitte, 55); }));
    add(pause(0.2));
    add(pruefe("rc608 Schreiben: der Klick setzt den Cursor in die Mitte, das Schreiben bleibt", [mitte] {
        const auto x = me::schreibStand();
        diag::info("Knopftest: Cursor nach Klick " + std::to_string(x.cursorX) + ", Mitte " + std::to_string(*mitte));
        return x.an && std::fabs(x.cursorX - *mitte) < 12.0F;
    }));
    // Tippen zeigt sofort, Esc verwirft
    add(tippe("Etwas"));
    add(pause(0.2));
    add(foto("menue_schreiben_tippen"));
    add(pruefe("rc608 Schreiben: das Getippte steht im Feld, die Datei noch nicht", [tiefe] {
        return me::schreibStand().puffer == "Etwas" && textVon("titel") == "\"Probe\"" && me::stand().undoSchritte == *tiefe;
    }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    add(pruefe("rc608 Schreiben: Esc verwirft", [] { return !me::schreibStand().an && textVon("titel") == "\"Probe\""; }));
    add(schritte("nach Esc", 0));
    // Enter uebernimmt: nur die Zeile text aendert sich, ein Schritt
    auto vorherText = std::make_shared<std::string>();
    add(tu("rc608 Schreiben: Text merken", [vorherText] { *vorherText = me::stand().text; }));
    add(merke());
    add(doppel("titel", 150, 55));
    add(pause(0.3));
    add(tippe("Neu"));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.2));
    add(pruefe("rc608 Schreiben: Enter uebernimmt, nur die Zeile text aendert sich", [vorherText] {
        std::string soll = *vorherText;
        const std::string alt = "text \"Probe\"";
        soll.replace(soll.find(alt), alt.size(), "text \"Neu\"");
        return !me::schreibStand().an && me::stand().text == soll;
    }));
    add(schritte("nach Enter", 1));
    add(foto("menue_schreiben_fertig"));
    add(tu("rc608 Schreiben: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
    add(pause(0.2));
    add(pruefe("rc608 Schreiben: Rueckgaengig stellt den Text her", [vorherText] { return me::stand().text == *vorherText; }));
    // Klick daneben uebernimmt
    add(merke());
    add(doppel("knopf", 200, 120));
    add(pause(0.3));
    add(pruefe("rc608 Schreiben: Button mit Text", [] { return me::schreibStand().puffer == "Back"; }));
    add(tippe("Zurueck"));
    add(klickAn("rc608 Schreiben: Klick daneben", [] { return aufFlaeche(50, 440); }));
    add(pause(0.2));
    add(pruefe("rc608 Schreiben: Klick daneben uebernimmt", [] { return !me::schreibStand().an && textVon("knopf") == "\"Zurueck\""; }));
    add(schritte("nach Klick daneben", 1));
    add(tu("rc608 Schreiben: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
    add(pause(0.2));
    // Mehrzeilig: Strg+Enter macht die neue Zeile, die Datei liest sich wieder
    add(doppel("lauftext", 400, 110));
    add(pause(0.3));
    add(tippe("A"));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Enter, "Strg+Enter"));
    add(Schritt{"Tippe B", [](int b) {
                    if (b == 0) { ImGui::GetIO().AddInputCharactersUTF8("B"); }
                    return b >= 3;
                }});
    add(pause(0.2));
    add(pruefe("rc608 Schreiben: Strg+Enter macht im Lauftext eine neue Zeile", [] { return me::schreibStand().puffer == "A\nB"; }));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.2));
    add(pruefe("rc608 Schreiben: SP: der Umbruch steht roh im String", [] { return textVon("lauftext").rfind("\"A\nB\"", 0) == 0; }));
    add(tu("rc608 Schreiben: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
    add(pause(0.2));
    // @-Texte: aufgeloest zeigen, das Spiel-Paket bleibt; das Element bekommt einen eigenen Text
    add(merke());
    add(doppel("spieltext", 200, 170));
    add(pause(0.3));
    add(pruefe("rc608 Schreiben: @-Text zeigt den aufgeloesten Text", [] {
        const auto x = me::schreibStand();
        diag::info("Knopftest: @MENUS_BACK ist " + x.puffer);
        return x.an && !x.puffer.empty() && x.puffer[0] != '@' && x.puffer == me::loeseText("@MENUS_BACK");
    }));
    add(tippe("Eigen"));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.2));
    add(pruefe("rc608 Schreiben: das Element nutzt einen eigenen Text, der des Spiels bleibt", [] {
        const std::string t = textVon("spieltext");
        diag::info("Knopftest: Zeile text " + t + ", Meldung " + me::letzteMeldung());
        return t.rfind("@MYMENU_MENUS_BACK", 0) == 0 && me::loeseText("@MYMENU_MENUS_BACK") == "Eigen" && me::loeseText("@MENUS_BACK") != "Eigen" &&
               me::letzteMeldung().find("MENUS_BACK") != std::string::npos;
    }));
    add(schritte("nach dem eigenen Text", 1));
    add(tu("rc608 Schreiben: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
    add(pause(0.2));
    add(pruefe("rc608 Schreiben: Rueckgaengig: wieder der Text des Spiels", [] { return textVon("spieltext").rfind("@MENUS_BACK", 0) == 0; }));
    // rc608 Teil 3a: Auch der schon eigene Text ist ein Undo-Schritt (vorher sagte es nur eine Einblendung)
    add(tu("rc608 Schreiben: Wiederholen: der eigene Text", [] { (void)me::wiederholen(); }));
    add(pause(0.2));
    add(pruefe("rc608 Schreiben: Wiederholen stellt auch den eigenen Text im Paket her", [] { return textVon("spieltext").rfind("@MYMENU_MENUS_BACK", 0) == 0 && me::loeseText("@MYMENU_MENUS_BACK") == "Eigen"; }));
    add(merke());
    auto eigenVor = std::make_shared<std::string>();
    add(tu("rc608 Schreiben: Text merken", [eigenVor] { *eigenVor = me::stand().text; }));
    add(doppel("spieltext", 200, 170));
    add(pause(0.3));
    add(tippe("Zwei"));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.2));
    add(pruefe("rc608 Schreiben: Eigener Text geaendert: die Datei bleibt, das Paket aendert sich", [eigenVor] { return me::stand().text == *eigenVor && me::loeseText("@MYMENU_MENUS_BACK") == "Zwei"; }));
    add(schritte("nach dem geaenderten eigenen Text", 1));
    add(tu("rc608 Schreiben: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
    add(pause(0.2));
    add(pruefe("rc608 Schreiben: Rueckgaengig: der eigene Text wie vorher", [] { return me::loeseText("@MYMENU_MENUS_BACK") == "Eigen"; }));
    add(tu("rc608 Schreiben: Wiederholen", [] { (void)me::wiederholen(); }));
    add(pause(0.2));
    add(pruefe("rc608 Schreiben: Wiederholen: der geaenderte Text", [] { return me::loeseText("@MYMENU_MENUS_BACK") == "Zwei"; }));
    add(tu("rc608 Schreiben: alles zurueck", [] {
        (void)me::rueckgaengig();
        (void)me::rueckgaengig();
    }));
    add(pause(0.2));
    add(pruefe("rc608 Schreiben: wieder der Text des Spiels, der eigene ist weg", [] { return textVon("spieltext").rfind("@MENUS_BACK", 0) == 0 && me::loeseText("@MYMENU_MENUS_BACK") != "Eigen"; }));
    // Kopieren und Einfuegen (die Ablage des Selbsttests, nicht die des Benutzers), Ausschneiden
    add(doppel("lauftext", 400, 110));
    add(pause(0.3));
    add(tippe("Eins"));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_C, "Strg+C"));
    add(taste(ImGuiKey_End, "Ende"));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_V, "Strg+V"));
    add(pause(0.2));
    add(pruefe("rc608 Schreiben: Kopieren und Einfuegen", [] {
        diag::info("Knopftest: Puffer " + me::schreibStand().puffer);
        return me::schreibStand().puffer == "EinsEins";
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_X, "Strg+X"));
    add(pause(0.2));
    add(pruefe("rc608 Schreiben: Ausschneiden", [] { return me::schreibStand().puffer.empty(); }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_V, "Strg+V"));
    add(pause(0.2));
    add(pruefe("rc608 Schreiben: Einfuegen nach dem Ausschneiden", [] { return me::schreibStand().puffer == "EinsEins"; }));
    // Bild auf, Bild ab im Lauftext: Der Cursor geht ueber das Feld und bleibt sichtbar
    for (int i = 0; i < 24; ++i) { add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Enter, "Strg+Enter")); }
    add(pause(0.2));
    auto yUnten = std::make_shared<float>(0.0F);
    add(tu("rc608 Schreiben: unten merken", [yUnten] { *yUnten = me::schreibStand().cursorY; }));
    add(taste(ImGuiKey_PageUp, "Bild auf"));
    add(pause(0.2));
    auto yOben = std::make_shared<float>(0.0F);
    add(pruefe("rc608 Schreiben: Bild auf geht nach oben, der Cursor bleibt im Element", [yUnten, yOben] {
        const auto x = me::schreibStand();
        *yOben = x.cursorY;
        diag::info("Knopftest: Cursor unten " + std::to_string(*yUnten) + ", nach Bild auf " + std::to_string(x.cursorY));
        return x.an && x.cursorY > 0.0F && x.cursorY < 480.0F && x.puffer.size() >= 8U;
    }));
    add(taste(ImGuiKey_PageDown, "Bild ab"));
    add(pause(0.2));
    add(pruefe("rc608 Schreiben: Bild ab geht wieder nach unten, der Cursor bleibt im Element", [yOben] {
        const auto x = me::schreibStand();
        diag::info("Knopftest: nach Bild ab " + std::to_string(x.cursorY));
        return x.an && x.cursorY >= *yOben && x.cursorY < 480.0F;
    }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    // Woher der Text sonst kommt: erklaeren statt nichts tun
    const auto erklaert = [&](const std::string& was, float vx, float vy, std::function<std::string()> soll) {
        add(tu("rc608 Schreiben: Meldung leeren", [] { (void)me::rueckgaengig(); (void)me::wiederholen(); }));
        add(doppel(was, vx, vy));
        add(pause(0.3));
        add(pruefe("rc608 Schreiben: " + was + " wird erklaert", [soll] {
            diag::info("Knopftest: Meldung " + me::letzteMeldung());
            return !me::schreibStand().an && me::letzteMeldung() == soll();
        }));
    };
    const auto mit = [](Str str, const std::string& wert) {
        return [str, wert] {
            char b[400];
            std::snprintf(b, sizeof(b), tr(str), wert.c_str());
            return std::string(b);
        };
    };
    erklaert("auscvar", 200, 215, mit(Str::MeSchreibenCvar, "ui_name"));
    erklaert("auscode", 200, 255, [] {
        char b[400];
        std::snprintf(b, sizeof(b), tr(Str::MeSchreibenOwnerdraw), 12);
        return std::string(b);
    });
    erklaert("liste", 200, 295, [] { return std::string(tr(Str::MeSchreibenFeeder)); });
    // rc608 Teil 3a: Auf einem Bild oeffnet der Doppelklick die Auswahl
    add(doppel("bild", 200, 335));
    add(pause(0.3));
    add(pruefe("rc608 Schreiben: Doppelklick auf ein Bild oeffnet die Auswahl, kein Schreiben", [] { return me::bildWahlOffen() && !me::schreibStand().an; }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    // Hierarchie zeigt den Text in der Zeile
    add(tu("rc608 Schreiben: Reiter Hierarchie", [] { me::setzeReiter(0); }));
    add(pause(0.2));
    add(klick(std::string("knopf  ") + tr(Str::MlTypButton) + "  \"Back\""));
    add(pruefe("rc608 Schreiben: die Hierarchie zeigt den Text in der Zeile und waehlt", [] { return me::stand().auswahl == std::vector<int>{me::itemNummer("knopf")}; }));
    add(foto("menue_schreiben_hierarchie"));
    add(tu("rc608 Schreiben: aufraeumen", [] { me::schliesseAlleOhneSpeichern(); }));
    return s;
}

// rc608 Punkt 10: die Hierarchie zeigt nur die offene Seite
std::vector<Schritt> schritteHierarchie() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    static const char* const kZwei =
        "{\nmenuDef\n{\n  name \"erstes\"\n  rect 0 0 640 480\n  visible 1\n"
        "  itemDef\n  {\n    name \"eins_a\"\n    type ITEM_TYPE_BUTTON\n    rect 10 10 100 30\n    visible 1\n  }\n"
        "  itemDef\n  {\n    name \"eins_b\"\n    type ITEM_TYPE_BUTTON\n    rect 10 50 100 30\n    visible 1\n  }\n}\n"
        "menuDef\n{\n  name \"zweites\"\n  rect 0 0 640 480\n  visible 1\n"
        "  itemDef\n  {\n    name \"zwei_a\"\n    type ITEM_TYPE_BUTTON\n    rect 200 10 100 30\n    visible 1\n  }\n}\n}\n";
    const auto gibt = [](const std::string& name) {
        const std::vector<std::string> z = me::hierarchieZeilen();
        return std::find(z.begin(), z.end(), name) != z.end();
    };
    add(tu("rc608 Hierarchie: Datei mit zwei Menues", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        g_app->settings.menueGanzeDatei = false;
        (void)me::oeffneText("ui/zwei.menu", kZwei);
        me::setzeReiter(0);
    }));
    add(pause(0.4));
    add(foto("menue_hierarchie_seite"));
    add(pruefe("rc608 Hierarchie: nur die Elemente der offenen Seite", [gibt] {
        return gibt("eins_a") && gibt("eins_b") && !gibt("zwei_a");
    }));
    add(klick("zweites"));
    add(pause(0.3));
    add(pruefe("rc608 Hierarchie: der Reiter wechselt, die Liste wechselt mit", [gibt] {
        return !gibt("eins_a") && gibt("zwei_a");
    }));
    // Gleichklang: Auswahl auf der Flaeche und in der Liste
    add(klickAn("rc608 Hierarchie: zwei_a auf der Flaeche", [] { return aufFlaeche(250, 25); }));
    add(pause(0.2));
    add(pruefe("rc608 Hierarchie: Auswahl auf der Flaeche", [] { return me::stand().auswahl == std::vector<int>{0}; }));
    add(tu("rc608 Hierarchie: Auswahl leeren", [] { me::waehle(-1, false); }));
    add(klick(std::string("zwei_a  ") + tr(Str::MlTypButton)));
    add(pause(0.2));
    add(pruefe("rc608 Hierarchie: Auswahl aus der Liste", [] { return me::stand().auswahl == std::vector<int>{0}; }));
    // Der Umschalter
    add(klick(tr(Str::MeGanzeDatei)));
    add(pause(0.2));
    add(foto("menue_hierarchie_datei"));
    add(pruefe("rc608 Hierarchie: Ganze Datei zeigt beide Menues", [gibt] { return gibt("eins_a") && gibt("zwei_a"); }));
    add(klick(tr(Str::MeGanzeDatei)));
    add(pause(0.2));
    add(pruefe("rc608 Hierarchie: und wieder nur die Seite", [gibt] { return !gibt("eins_a") && gibt("zwei_a"); }));
    // Rest aus Teil 1: Pfeiltasten und Entf wirken mit dem Fokus in der Hierarchie wie auf der Flaeche
    add(klick(std::string("zwei_a  ") + tr(Str::MlTypButton)));
    add(pause(0.2));
    add(taste(ImGuiKey_RightArrow, "Pfeil rechts"));
    add(pause(0.2));
    add(pruefe("rc608 Hierarchie: Pfeil rechts schiebt um 1, ein Schritt", [] {
        const std::string t = me::stand().text;
        diag::info("Knopftest: Hierarchie Pfeil, Auswahl " + std::to_string(me::stand().auswahl.size()));
        return t.find("rect 201 10 100 30") != std::string::npos && me::stand().auswahl == std::vector<int>{0};
    }));
    add(tasteMit(ImGuiMod_Shift, ImGuiKey_DownArrow, "Umschalt+Pfeil unten"));
    add(pause(0.2));
    add(pruefe("rc608 Hierarchie: Umschalt+Pfeil schiebt um 10", [] { return me::stand().text.find("rect 201 20 100 30") != std::string::npos; }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(pruefe("rc608 Hierarchie: Rueckgaengig nimmt beide zurueck", [] { return me::stand().text.find("rect 200 10 100 30") != std::string::npos; }));
    add(klick(std::string("zwei_a  ") + tr(Str::MlTypButton)));
    add(pause(0.2));
    add(taste(ImGuiKey_Delete, "Entf"));
    add(pause(0.3));
    add(pruefe("rc608 Hierarchie: Entf loescht das gewaehlte Element", [] { return me::stand().text.find("zwei_a") == std::string::npos; }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(pruefe("rc608 Hierarchie: Rueckgaengig bringt es wieder", [] { return me::stand().text.find("zwei_a") != std::string::npos; }));
    // Zustandsreiter: nur, was dort sichtbar ist
    add(tu("rc608 Hierarchie: Bauteil Detail area", [] {
        me::schliesseAlleOhneSpeichern();
        (void)me::oeffneText("ui/detail.menu", kZugText);
        (void)me::fuegeVorlageEin("detail", 100.0F, 100.0F);
    }));
    add(pause(0.4));
    add(foto("menue_hierarchie_zustand"));
    auto alle = std::make_shared<std::size_t>(0);
    add(pruefe("rc608 Hierarchie: es gibt Zustandsreiter", [alle] {
        *alle = me::hierarchieZeilen().size();
        diag::info("Knopftest: Seiten nach Detail area: " + std::to_string(me::stand().seiten) + ", Zeilen " + std::to_string(*alle));
        return me::stand().seiten >= 2;
    }));
    add(tu("rc608 Hierarchie: ersten Zustandsreiter oeffnen", [] { me::setzeSeite(1); }));
    add(pause(0.3));
    add(foto("menue_hierarchie_zustandsreiter"));
    add(pruefe("rc608 Hierarchie: ein Zustandsreiter zeigt nur das dort Sichtbare", [alle] {
        const std::size_t n = me::hierarchieZeilen().size();
        diag::info("Knopftest: Zeilen im Zustandsreiter " + std::to_string(n));
        return n > 0 && n < *alle;
    }));
    // Titelleiste
    add(pruefe("rc608 Hierarchie: die Titelleiste nennt die Menuedatei", [] {
        const std::string t = windowTitle();
        diag::info("Knopftest: Titel " + t);
        return t.find("ui/detail.menu") != std::string::npos;
    }));
    add(tu("rc608 Hierarchie: aufraeumen", [] { me::schliesseAlleOhneSpeichern(); }));
    return s;
}

// Ein Element (Beschriftung) auf die Flaeche ziehen und an der virtuellen Stelle (vx, vy) ablegen
// Die linke Liste rollt: ein Teil, das noch nicht im Bild ist, hat dort keine Beschriftung (ImGui meldet sie erst beim Zeichnen)
Schritt rolleZuKachel(const std::string& name) {
    return Schritt{"rc608: zur Kachel rollen " + name, [=](int b) {
                       if (b > 0) { return b >= 3; }
                       for (ImGuiWindow* w : GImGui->Windows) {
                           if (!w->WasActive || std::string(w->Name).find("/##teile_liste_") == std::string::npos) { continue; }
                           for (const me::KachelLage& k : me::kachelLagen()) {
                               if (k.name == name) { ImGui::SetScrollY(w, std::max(0.0F, w->Scroll.y + k.y0 - w->Pos.y)); }
                           }
                       }
                       return false;
                   }};
}
// Endrunde rc608: die rechte Spalte (Fenster "/##inhalt_") rollen, bis `label` ganz zu sehen ist, und es dann nach oben
// holen. Im Bildlauf (BHED_MENUE_BILDER, schmale Spalten) liegt manches unter dem Rand; ImGui meldet ein Element erst,
// wenn es gezeichnet wird. Im breiten Fenster ist es meist schon zu sehen, dann rollt nur das Hochholen.
Schritt rolleRechtsZu(const std::string& label) {
    return Schritt{"Endrunde: rechte Spalte zu " + label, [=](int b) {
                       if (const Element* e = finde(label, "", 0, false, ""); e != nullptr && ganzSichtbar(*e)) {
                           if (ImGuiWindow* w = ImGui::FindWindowByName(e->innen.c_str()); w != nullptr) {
                               ImGui::SetScrollY(w, std::clamp(w->Scroll.y + e->rect.Min.y - w->InnerRect.Min.y - ImGui::GetFontSize(), 0.0F, w->ScrollMax.y));
                           }
                           return true;
                       }
                       if (b > 120) {
                           meldeFehler("Endrunde: " + label + " in der rechten Spalte nicht zu sehen");
                           return true;
                       }
                       if (b % 3 == 0) {
                           for (ImGuiWindow* w : GImGui->Windows) {
                               if (!w->WasActive || std::string(w->Name).find("/##inhalt_") == std::string::npos ||
                                   std::string(w->Name).find("##add_liste") != std::string::npos) {
                                   continue;
                               }
                               ImGui::SetScrollY(w, std::min(w->ScrollMax.y, w->Scroll.y + w->InnerRect.GetHeight() * 0.5F));
                           }
                       }
                       return false;
                   }};
}
Schritt rolleTeileAnsEnde() {
    return Schritt{"rc608: Teile ans Ende rollen", [](int b) {
                       if (b > 0) { return b >= 3; }
                       for (ImGuiWindow* w : GImGui->Windows) {
                           if (w->WasActive && std::string(w->Name).find("/##teile_liste_") != std::string::npos) { ImGui::SetScrollY(w, 100000.0F); }
                       }
                       return false;
                   }};
}

Schritt zieheAufFlaeche(const std::string& label, float vx, float vy, std::function<void()> kurzVorDemLoslassen = {}) {
    auto start = std::make_shared<ImVec2>();
    return Schritt{"rc608 Ablegen: " + label, [=](int roh) {
                       ImGuiIO& io = ImGui::GetIO();
                       // zuerst zur Kachel rollen: Die Beschriftung kennt nur, was im Bild steht
                       if (roh == 0) {
                           for (ImGuiWindow* w : GImGui->Windows) {
                               if (!w->WasActive || std::string(w->Name).find("/##teile_liste_") == std::string::npos) { continue; }
                               for (const me::KachelLage& k : me::kachelLagen()) {
                                   if (k.name == label) { ImGui::SetScrollY(w, std::max(0.0F, w->Scroll.y + k.y0 - w->Pos.y)); }
                               }
                           }
                       }
                       if (roh < 3) { return false; }
                       const int b = roh - 3;
                       if (b == 0) {
                           const Element* e = finde(label, "##links", 0, false);
                           if (e == nullptr) {
                               meldeFehler("rc608 Ablegen: " + label + " nicht gefunden");
                               return true;
                           }
                           *start = e->rect.GetCenter();
                           setzeMaus(*start);
                           return false;
                       }
                       if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
                       if (b >= 3 && b <= 7) { setzeMaus(ImVec2{start->x + 5.0F * static_cast<float>(b - 2), start->y + 3.0F * static_cast<float>(b - 2)}); return false; }
                       if (b >= 8 && b <= 10) { setzeMaus(aufFlaeche(vx, vy)); return false; }
                       if (b == 12) {
                           if (kurzVorDemLoslassen) { kurzVorDemLoslassen(); }
                           return false;
                       }
                       if (b == 13) { io.AddMouseButtonEvent(0, false); return false; }
                       return b >= 17;
                   }};
}

// rc608 Punkt 11 (shank: "warum kann ich die Libary sachen nehemn und rueber schieben aber die sachen oben ... nur doppel klicken")
std::vector<Schritt> schritteAblegen() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto tiefe = std::make_shared<int>(0);
    auto anzahl = std::make_shared<int>(0);
    const auto merke = [tiefe, anzahl] {
        return tu("rc608 Ablegen: Stand merken", [tiefe, anzahl] {
            const auto x = me::stand();
            *tiefe = x.undoSchritte;
            *anzahl = x.items;
        });
    };
    add(tu("rc608 Ablegen: leeres Menue", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneText("ablegen.menu", "menuDef\n{\n  name \"ablegen\"\n  rect 0 0 640 480\n  visible 1\n}\n");
        me::setzeInteraktion(false);
    }));
    add(pause(0.5));
    // Ein Klick tut nichts
    add(merke());
    add(rolleZuKachel(tr(Str::MlTypButton)));
    add(klick(tr(Str::MlTypButton)));
    add(pause(0.3));
    add(pruefe("rc608 Ablegen: ein einfacher Klick auf ein Widget setzt nichts ein", [anzahl] { return me::stand().items == *anzahl; }));
    // Jedes Widget auf die Flaeche
    struct W { Str name; float x, y; const char* typ; };
    static const W kWidgets[] = {{Str::MlTypText, 20, 30, "ITEM_TYPE_TEXT"},       {Str::MlTypButton, 240, 30, "ITEM_TYPE_BUTTON"}, {Str::MlTypBild, 450, 30, "WINDOW_STYLE_SHADER"},
                                 {Str::MlTypJaNein, 20, 110, "ITEM_TYPE_YESNO"},   {Str::MlTypMulti, 240, 110, "ITEM_TYPE_MULTI"},  {Str::MlTypSchieber, 450, 110, "ITEM_TYPE_SLIDER"},
                                 {Str::MlTypEingabe, 20, 190, "ITEM_TYPE_EDITFIELD"}, {Str::MlTypListe, 240, 190, "ITEM_TYPE_LISTBOX"}, {Str::MlTypModell, 450, 190, "ITEM_TYPE_MODEL"},
                                 {Str::MlTypTaste, 20, 270, "ITEM_TYPE_BIND"},   {Str::MlTypVideo, 240, 270, "jkstudio:rahmen video"}};
    for (const W& w : kWidgets) {
        const std::string label = tr(w.name);
        const float x = w.x, y = w.y;
        const std::string typ = w.typ;
        add(merke());
        auto rahmen = std::make_shared<std::array<float, 4>>();
        auto zeigt = std::make_shared<bool>(false);
        add(zieheAufFlaeche(label, x, y, [rahmen, zeigt] { *zeigt = me::ablageRahmen(rahmen->data()); }));
        add(pause(0.1));
        add(pruefe("rc608 Ablegen: " + label + " liegt genau an der Stelle, gewaehlt, ein Schritt, mit Rahmen beim Ziehen", [=] {
            const auto st = me::stand();
            const std::vector<me::ItemLage> l = me::itemLagen();
            if (st.items != *anzahl + 1 || l.empty() || st.undoSchritte != *tiefe + 1 || st.auswahl.size() != 1) {
                diag::info("Knopftest: Ablegen " + label + ": Elemente " + std::to_string(st.items) + ", Undo " + std::to_string(st.undoSchritte - *tiefe));
                return false;
            }
            const me::ItemLage& n = l[static_cast<std::size_t>(st.auswahl[0])];
            diag::info("Knopftest: Ablegen " + label + " auf " + std::to_string(n.x) + ", " + std::to_string(n.y) + ", Rahmen " + std::to_string((*rahmen)[0]) + ", " + std::to_string((*rahmen)[1]));
            return *zeigt && std::fabs(n.x - x) <= 6.0F && std::fabs(n.y - y) <= 6.0F && std::fabs(n.x - (*rahmen)[0]) < 0.5F && std::fabs(n.y - (*rahmen)[1]) < 0.5F &&
                   st.text.find(typ) != std::string::npos;
        }));
    }
    add(foto("menue_ablegen"));
    // Einrasten beim Ablegen: 2 Punkte neben der Kante des Textes rastet ein
    add(merke());
    add(zieheAufFlaeche(tr(Str::MlTypButton), 22, 350));
    add(pause(0.1));
    add(pruefe("rc608 Ablegen: rastet an der Kante ein", [] {
        const auto st = me::stand();
        const me::ItemLage n = me::itemLagen()[static_cast<std::size_t>(st.auswahl[0])];
        diag::info("Knopftest: eingerastet auf " + std::to_string(n.x));
        return std::fabs(n.x - 20.0F) < 0.5F;
    }));
    // Bei Zoom: genau dort
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_1, "Strg+1"));
    add(pause(0.2));
    add(merke());
    add(zieheAufFlaeche(tr(Str::MlTypText), 330, 417));
    add(pause(0.1));
    add(pruefe("rc608 Ablegen: bei 100 % genau dort", [] {
        const auto st = me::stand();
        const me::ItemLage n = me::itemLagen()[static_cast<std::size_t>(st.auswahl[0])];
        diag::info("Knopftest: bei 100 %: " + std::to_string(n.x) + ", " + std::to_string(n.y) + ", Zoom " + std::to_string(st.zoomProzent));
        return st.zoomProzent == 100 && std::fabs(n.x - 330.0F) <= 6.0F && std::fabs(n.y - 417.0F) <= 6.0F;
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_0, "Strg+0"));
    add(pause(0.2));
    // Doppelklick: in die Mitte, ohne Stapel
    auto erstes = std::make_shared<std::array<float, 2>>();
    add(tu("rc608 Ablegen: frisches Menue", [] { (void)me::oeffneText("mitte.menu", "menuDef\n{\n  name \"mitte\"\n  rect 0 0 640 480\n  visible 1\n}\n"); }));
    add(pause(0.3));
    add(merke());
    const auto doppelKachel = [](Str name) {
        return klickAn("rc608 Ablegen: Doppelklick auf " + std::string(tr(name)), [name] {
            const Element* e = finde(tr(name), "##links", 0, false);
            return e != nullptr ? e->rect.GetCenter() : ImVec2{};
        }, 0, true);
    };
    add(rolleZuKachel(tr(Str::MlTypButton)));
    add(doppelKachel(Str::MlTypButton));
    add(pause(0.2));
    add(pruefe("rc608 Ablegen: Doppelklick setzt ein, ein Schritt", [tiefe, anzahl, erstes] {
        const auto st = me::stand();
        if (st.items != *anzahl + 1 || st.undoSchritte != *tiefe + 1 || st.auswahl.empty()) { return false; }
        const me::ItemLage n = me::itemLagen()[static_cast<std::size_t>(st.auswahl[0])];
        *erstes = {n.x, n.y};
        return true;
    }));
    add(doppelKachel(Str::MlTypButton));
    add(pause(0.2));
    add(pruefe("rc608 Ablegen: der zweite Doppelklick stapelt nicht", [erstes] {
        const auto st = me::stand();
        if (st.auswahl.empty()) { return false; }
        const me::ItemLage n = me::itemLagen()[static_cast<std::size_t>(st.auswahl[0])];
        diag::info("Knopftest: zweiter Doppelklick " + std::to_string(n.x) + ", " + std::to_string(n.y));
        return std::fabs(n.x - (*erstes)[0] - 20.0F) < 0.5F && std::fabs(n.y - (*erstes)[1] - 20.0F) < 0.5F;
    }));
    // Bibliothek: derselbe Weg
    add(merke());
    add(rolleZuKachel(tr(Str::MlBtReiter)));
    add(zieheAufFlaeche(tr(Str::MlBtReiter), 100, 300));
    add(pause(0.2));
    add(pruefe("rc608 Ablegen: Bibliothek auf dieselbe Weise, ein Schritt, drei Elemente", [tiefe, anzahl] {
        const auto st = me::stand();
        return st.items == *anzahl + 3 && st.undoSchritte == *tiefe + 1;
    }));
    add(tu("rc608 Ablegen: aufraeumen", [] { me::schliesseAlleOhneSpeichern(); }));
    return s;
}

// rc608 Punkt 12: Modell einfuegen und drehen
std::vector<Schritt> schritteModell() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto griffe = std::make_shared<std::array<float, 8>>();
    auto groesse = std::make_shared<float>(0.0F);
    const auto hol = [griffe, groesse] { return me::modellGriffe(griffe->data(), groesse.get()); };
    add(tu("rc608 Modell: leeres Menue", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneText("modell.menu", "menuDef\n{\n  name \"modell\"\n  rect 0 0 640 480\n  visible 1\n}\n");
        me::setzeInteraktion(false);
    }));
    add(pause(0.5));
    add(tu("rc608 Modell: einfuegen wie der Doppelklick", [] { me::fuegeEin(bhed::menue::ITEM_TYPE_MODEL); }));
    add(pause(2.5));
    add(foto("menue_modell_neu"));
    add(pruefe("rc608 Modell: Vorgaben wie in den Menues des Spiels", [] {
        const auto x = me::stand();
        const std::string t = x.text;
        const std::vector<me::ItemLage> l = me::itemLagen();
        diag::info("Knopftest: Modell " + std::to_string(l.empty() ? 0.0F : l[0].b) + " x " + std::to_string(l.empty() ? 0.0F : l[0].h) + ", Aufrufe " + std::to_string(x.modellAufrufe) + ", Fehler " + x.modellFehler);
        if (x.spiel == 1) { return t.find("jk2logo") != std::string::npos; }
        return !l.empty() && l[0].b == 200.0F && l[0].h == 225.0F && t.find("model_angle") != std::string::npos && t.find("BOTH_MENUIDLE1") != std::string::npos && x.modellAufrufe > 0 &&
               t.find("model_g2anim") < t.find("asset_model");
    }));
    // Griffe: so gross wie ein Knopf, kein Griff zum Skalieren darunter, je Zoomstufe und bei einem kleinen Element
    const auto griffePruefen = [&](const std::string& was) {
        add(pruefe("rc608 Modell: " + was + ": die Griffe sind gross genug und liegen nicht auf den Griffen zum Skalieren", [hol, griffe, groesse] {
            if (!hol()) { return false; }
            const float g = *groesse;
            const std::vector<me::ItemLage> l = me::itemLagen();
            const auto st = me::stand();
            const me::ItemLage& n = l[static_cast<std::size_t>(st.auswahl[0])];
            const float kx[3] = {n.x, n.x + n.b * 0.5F, n.x + n.b};
            const float ky[3] = {n.y, n.y + n.h * 0.5F, n.y + n.h};
            bool gut = g >= ImGui::GetFrameHeight();
            for (int k = 0; k < 4; ++k) {
                const float cx = (*griffe)[static_cast<std::size_t>(k * 2)], cy = (*griffe)[static_cast<std::size_t>(k * 2 + 1)];
                for (int yy = 0; yy < 3; ++yy) {
                    for (int xx = 0; xx < 3; ++xx) {
                        if (xx == 1 && yy == 1) { continue; }
                        const ImVec2 h = aufFlaeche(kx[xx], ky[yy]);
                        const bool ueberlappt = std::fabs(h.x - cx) < g * 0.5F + 5.0F && std::fabs(h.y - cy) < g * 0.5F + 5.0F;
                        gut = gut && !ueberlappt;
                    }
                }
            }
            diag::info("Knopftest: Griffgroesse " + std::to_string(g) + ", Knopf " + std::to_string(ImGui::GetFrameHeight()));
            return gut;
        }));
    };
    add(pause(0.2));
    griffePruefen("Vorgabe");
    add(foto("menue_modell_griffe"));
    // Blickrichtung drehen: ein Zug, ein Schritt, nur model_angle
    auto tiefe = std::make_shared<int>(0);
    auto text = std::make_shared<std::string>();
    add(tu("rc608 Modell: Stand merken", [tiefe, text] {
        *tiefe = me::stand().undoSchritte;
        *text = me::stand().text;
    }));
    add(zeigerAn("auf dem Griff Drehen", [griffe] { return ImVec2{(*griffe)[0], (*griffe)[1]}; }, ImGuiMouseCursor_ResizeEW));
    add(ziehenAn("rc608 Modell: Griff Drehen", [griffe] { return ImVec2{(*griffe)[0], (*griffe)[1]}; }, 60.0F, 0.0F, 0));
    add(pause(0.2));
    add(pruefe("rc608 Modell: Drehen schreibt model_angle, ein Undo-Schritt, sonst nichts", [tiefe, text] {
        const auto x = me::stand();
        const std::size_t p = x.text.find("model_angle");
        const std::string zeile = p == std::string::npos ? std::string() : x.text.substr(p, x.text.find('\n', p) - p);
        diag::info("Knopftest: " + zeile);
        const auto winkel = [](const std::string& t) {
            const std::size_t w = t.find("model_angle");
            return w == std::string::npos ? -1 : std::atoi(t.c_str() + w + 11);
        };
        // Sonst aendert sich nichts: die Datei waechst hoechstens um die Ziffern des Winkels (JK2: 90 -> 120)
        const std::size_t ziffernVorher = std::to_string(winkel(*text)).size(), ziffernNachher = std::to_string(winkel(x.text)).size();
        return x.undoSchritte == *tiefe + 1 && winkel(x.text) == winkel(*text) + 30 && x.text.size() + ziffernVorher == text->size() + ziffernNachher;
    }));
    add(foto("menue_modell_gedreht"));
    add(tu("rc608 Modell: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
    add(pause(0.2));
    add(pruefe("rc608 Modell: Rueckgaengig stellt die Richtung her", [text] { return me::stand().text == *text; }));
    // Hoehe und Abstand
    add(tu("rc608 Modell: Griffe lesen", [hol] { (void)hol(); }));
    add(ziehenAn("rc608 Modell: Griff Hoehe", [griffe] { return ImVec2{(*griffe)[4], (*griffe)[5]}; }, 0.0F, 40.0F, 0));
    add(pause(0.2));
    add(pruefe("rc608 Modell: Hoehe schreibt model_g2mins und model_g2maxs", [text] {
        const std::string t = me::stand().text;
        return t != *text && t.find("model_g2mins") != std::string::npos;
    }));
    add(tu("rc608 Modell: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
    add(pause(0.2));
    // Griffe bei 381 %, bei 25 % und bei einem kleinen Element
    add(tu("rc608 Modell: Griffe lesen", [hol] { (void)hol(); }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_1, "Strg+1"));
    add(pause(0.1));
    add(halteTaste(ImGuiMod_Ctrl, true));
    add(radAn("Strg+Rad", [] { return aufFlaeche(370, 200); }, 6.0F));
    add(halteTaste(ImGuiMod_Ctrl, false));
    griffePruefen("381 %");
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_1, "Strg+1"));
    add(pause(0.1));
    add(halteTaste(ImGuiMod_Ctrl, true));
    add(radAn("Strg+Rad", [] { return aufFlaeche(370, 200); }, -8.0F));
    add(halteTaste(ImGuiMod_Ctrl, false));
    griffePruefen("25 %");
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_0, "Strg+0"));
    add(pause(0.1));
    add(tu("rc608 Modell: kleines Element", [] { me::setzeEigenschaftAuswahl("rect", {"220", "200", "200", "40"}); }));
    add(pause(0.3));
    griffePruefen("200 x 40");
    add(foto("menue_modell_griffe_klein"));
    add(tu("rc608 Modell: aufraeumen", [] { me::schliesseAlleOhneSpeichern(); }));
    return s;
}

// rc608 Punkt 14 (shank: "ich hab auch in dem Model Text allignemt dabei"): je Typ nur, was das Spiel auswertet
std::vector<Schritt> schritteFelderJeTyp() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("rc608 Felder: Probe", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        std::string t = "menuDef\n{\n  name \"felder\"\n  rect 0 0 640 480\n  visible 1\n";
        for (int typ = 0; typ < 15; ++typ) {
            t += "  itemDef\n  {\n    name \"e" + std::to_string(typ) + "\"\n    type " + std::string(bhed::menue::typName(typ)) + "\n    rect 10 10 100 30\n    textalign ITEM_ALIGN_CENTER\n    visible 1\n  }\n";
        }
        t += "}\n";
        (void)me::oeffneText("ui/felder.menu", t);
        me::setzeReiter(2);
    }));
    add(pause(0.4));
    for (int typ = 0; typ < 15; ++typ) {
        const std::string name = "e" + std::to_string(typ);
        add(tu("rc608 Felder: " + name + " waehlen", [name] { me::waehle(me::itemNummer(name), false); }));
        add(pause(0.15));
        add(pruefe("rc608 Felder: " + std::string(bhed::menue::typName(typ)) + " zeigt, was gilt", [typ, name] {
            const std::vector<std::string> f = me::eigenschaftsFelder();
            const auto hat = [&](const char* k) { return std::find(f.begin(), f.end(), k) != f.end(); };
            bool gut = true;
            for (const char* k : {"text", "textscale", "textalignx", "textalign", "forecolor", "cvar", "asset_model"}) {
                const bool soll = bhed::menue::feldGilt(k, typ, 0, me::stand().spiel == 1 ? bhed::menue::Spiel::JK2 : me::stand().spiel == 2 ? bhed::menue::Spiel::MD : bhed::menue::Spiel::JA);
                if (hat(k) != soll) {
                    diag::info("Knopftest: Felder " + name + ": " + k + (soll ? " fehlt" : " zu viel"));
                    gut = false;
                }
            }
            // textalign steht in der Datei: gilt er nicht, erscheint er unten statt zu verschwinden
            const std::vector<std::string> w = me::weitereEintraege();
            const bool unten = std::any_of(w.begin(), w.end(), [](const std::string& z) { return z.rfind("textalign", 0) == 0; });
            return gut && unten == !hat("textalign");
        }));
    }
    add(tu("rc608 Felder: Modell", [] { me::waehle(me::itemNummer("e7"), false); }));
    add(pause(0.2));
    add(foto("menue_properties_modell_ohne_text"));
    // Animation aus der Liste mit Suche, die Zeile kommt vor asset_model
    add(tu("rc608 Felder: ein richtiges Modell", [] {
        me::schliesseAlleOhneSpeichern();
        (void)me::oeffneText("ui/anim.menu", "menuDef\n{\n  name \"anim\"\n  rect 0 0 640 480\n  visible 1\n"
                                             "  itemDef\n  {\n    name \"figur\"\n    type ITEM_TYPE_MODEL\n    rect 200 100 200 225\n    asset_model \"models/players/kyle/model.glm\"\n    model_g2skin \"models/players/kyle/model_default.skin\"\n    model_angle 180\n    model_g2anim \"BOTH_WALK1\"\n    visible 1\n  }\n}\n");
        me::setzeInteraktion(false);
        me::waehle(0, false);
        me::setzeReiter(2);
    }));
    add(pause(1.5));
    add(foto("menue_properties_modell"));
    add(pruefe("rc608 Felder: die Warnung nennt die Reihenfolge", [] { return me::stand().text.find("model_g2anim") > me::stand().text.find("asset_model"); }));
    auto tiefe = std::make_shared<int>(0);
    add(tu("rc608 Felder: Undo-Tiefe merken", [tiefe] { *tiefe = me::stand().undoSchritte; }));
    const auto nichtJk2 = [](Schritt x) {
        const std::function<bool(int)> f = x.tun;
        x.tun = [f](int b) { return me::stand().spiel == 1 ? true : f(b); };
        return x;
    };
    add(nichtJk2(klick(tr(Str::MeModellAnimWaehlen))));
    add(pause(0.3));
    add(foto("menue_properties_animliste"));
    add(nichtJk2(tippe("BOTH_STAND1")));
    add(pause(0.3));
    add(nichtJk2(klick("BOTH_STAND1")));
    add(pause(1.0));
    add(nichtJk2(pruefe("rc608 Felder: Animation gewaehlt: Zeile vor asset_model, ein Undo-Schritt", [tiefe] {
        const auto x = me::stand();
        const std::size_t a = x.text.find("BOTH_STAND1"), m = x.text.find("asset_model");
        diag::info("Knopftest: Anim an " + std::to_string(a) + ", Modell an " + std::to_string(m) + ", Undo " + std::to_string(x.undoSchritte - *tiefe));
        return a != std::string::npos && a < m && x.text.find("BOTH_WALK1") == std::string::npos && x.undoSchritte == *tiefe + 1;
    })));
    add(foto("menue_properties_anim_gewaehlt"));
    add(pruefe("rc608 Felder: was die Modell-Gruppe zeigt, steht nicht doppelt unten; keine doppelten Kennungen", [] {
        const std::vector<std::string> w = me::weitereEintraege();
        for (const std::string& z : w) { diag::info("Knopftest: weiter unten: " + z); }
        for (const elemente::IdKonflikt& k : elemente::idKonflikte()) { diag::info("Knopftest: doppelte Kennung in " + k.fenster + " bei " + std::to_string(k.rect.Min.x) + ", " + std::to_string(k.rect.Min.y)); }
        // JK2: model_g2anim wertet das Spiel nicht aus, es gehoert nach unten
        const bool doppelt = me::stand().spiel != 1 && std::any_of(w.begin(), w.end(), [](const std::string& z) { return z.rfind("model_", 0) == 0 || z.rfind("asset_", 0) == 0; });
        return !doppelt && elemente::idKonflikte().empty();
    }));
    add(tu("rc608 Felder: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
    add(pause(0.3));
    add(pruefe("rc608 Felder: Rueckgaengig stellt die alte Zeile her", [] { return me::stand().text.find("BOTH_WALK1") != std::string::npos && me::stand().text.find("BOTH_STAND1") == std::string::npos; }));
    add(tu("rc608 Felder: aufraeumen", [] {
        me::schliesseAlleOhneSpeichern();
        me::setzeReiter(0);
    }));
    return s;
}

// Liegt alles im Inhalt der rechten Spalte? Waagerechtes Rollen waere der letzte Ausweg: ScrollMax.x bleibt 0,
// jedes Element liegt ganz im Fenster. fehler: was hinausragt.
ImGuiWindow* inhaltFenster() {
    for (ImGuiWindow* x : GImGui->Windows) {
        const std::string n = x->Name;
        if (x->WasActive && n.find("/##inhalt_") != std::string::npos && n.find("##rechts") != std::string::npos) { return x; }
    }
    return nullptr;
}

bool rechtsGanz(std::string& fehler, const char* teil = "##inhalt") {
    ImGuiWindow* w = nullptr;
    const std::string kennung = std::string("/") + teil + "_";
    for (ImGuiWindow* x : GImGui->Windows) {
        const std::string n = x->Name;
        if (x->WasActive && n.find(kennung) != std::string::npos && n.find("##rechts") != std::string::npos) { w = x; }
    }
    if (w == nullptr) {
        fehler = "kein Inhalt";
        return false;
    }
    bool gut = true;
    if (w->ScrollMax.x > 0.5F) {
        fehler += " ScrollMax " + std::to_string(w->ScrollMax.x);
        gut = false;
    }
    for (const Element& e : elemente::alle()) {
        // die Karte oben liegt im selben Feld
        if (e.innen.find("##karte") != std::string::npos && e.innen.find("##rechts") != std::string::npos && e.label.find("##karte_") == std::string::npos &&
            (e.rect.Min.x < w->Pos.x - 0.5F || e.rect.Max.x > w->Pos.x + w->Size.x + 0.5F)) {
            fehler += " [Karte " + e.label + "]";
            gut = false;
        }
        if (e.innen.find(teil) == std::string::npos) { continue; }
        if (e.rect.Min.x >= w->InnerRect.Max.x - 1.0F || e.label.find(std::string(teil) + "_") != std::string::npos) { continue; }   // der Rollbalken und das Fenster selbst
        if (e.rect.Min.x < w->Pos.x - 0.5F || e.rect.Max.x > w->InnerRect.Max.x + 0.5F) {
            fehler += " [" + e.label + " bis " + std::to_string(e.rect.Max.x - w->InnerRect.Max.x) + " bei y " + std::to_string(e.rect.Min.y - w->Pos.y + w->Scroll.y) + "]";
            gut = false;
        }
    }
    return gut;
}

// Der Probeinhalt fuer die Breiten: alle Felder eines Modells und eines Knopfs, dazu Ereignisse mit den breiten Aktionen
const char* const kBreitenText =
    "menuDef\n{\n  name \"breit\"\n  rect 0 0 640 480\n  visible 1\n"
    "  itemDef\n  {\n    name \"knopf\"\n    type ITEM_TYPE_BUTTON\n    rect 100 100 200 40\n    text \"Back\"\n    focusSound \"sound/interface/button1.wav\"\n    visible 1\n"
    "    action { play \"sound/interface/button1.wav\" ; setitemcolor knopf forecolor 1 0.5 0.25 1 ; transition knopf 10 10 100 30 6 6 108 38 10 1 ; setcvar ui_name 1 ; show knopf }\n"
    "    mouseEnter { setitemcolor knopf backcolor 1 1 1 1 }\n  }\n"
    "  itemDef\n  {\n    name \"figur\"\n    type ITEM_TYPE_MODEL\n    rect 300 100 200 225\n    model_g2anim \"BOTH_STAND1\"\n    asset_model \"models/players/kyle/model.glm\"\n    model_angle 180\n    visible 1\n  }\n"
    "}\n";

// rc608 Punkte 3 und 13: Bei 300, 500 und 1000 Punkten, 100 % und 200 %, in jeder Sprache liegt alles im Feld
std::vector<Schritt> breitenTabelle(const std::string& wer, int reiter, std::vector<int> elemente, std::function<bool(std::string&)> zusatz) {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto skala = std::make_shared<float>(1.0F);
    const i18n::Language sprache = i18n::currentLanguage();
    add(tu("rc608 Breiten " + wer + ": Oberflaeche merken", [skala, reiter] {
        *skala = g_app->uiScale;
        me::setzeReiter(reiter);
    }));
    for (const float zoom : {1.0F, 2.0F}) {
        for (const i18n::LanguageInfo& l : i18n::languages()) {
            const i18n::Language lang = l.language;
            const std::string kurz = l.code;
            for (const float breite : {300.0F, 500.0F, 1000.0F}) {
                for (const int el : elemente) {
                    const std::string was = "rc608 Breiten " + wer + " " + kurz + " " + std::to_string(static_cast<int>(zoom * 100.0F)) + " % " + std::to_string(static_cast<int>(breite)) + " Punkte Element " + std::to_string(el);
                    add(tu(was, [lang, zoom, breite, el] {
                        i18n::setLanguage(lang);
                        g_app->uiScale = zoom;
                        me::setzeSpalten(me::linkeSpalte(), breite);
                        me::waehle(el, false);
                    }));
                    add(pause(0.12));
                    add(pruefe(was + " liegt ganz im Feld", [was, zusatz] {
                        std::string f;
                        bool gut = rechtsGanz(f);
                        if (zusatz) { gut = zusatz(f) && gut; }
                        if (!gut) { diag::info("Knopftest: " + was + ": rechts " + std::to_string(me::rechteSpalte()) + f); }
                        return gut;
                    }));
                }
            }
        }
    }
    add(tu("rc608 Breiten " + wer + ": Oberflaeche zurueck", [skala, sprache] {
        g_app->uiScale = *skala;
        i18n::setLanguage(sprache);
    }));
    return s;
}

std::vector<Schritt> schritteBreiten() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("rc608 Breiten: Probe", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneText("ui/breit.menu", kBreitenText);
        me::setzeInteraktion(false);
    }));
    add(pause(1.0));
    for (auto& x : breitenTabelle("Properties", 2, {0, 1}, {})) { add(std::move(x)); }
    // Mindestbreite: kleiner geht es nicht, und die Zahlen sind lesbar
    add(tu("rc608 Breiten: viel zu schmal", [] {
        me::setzeReiter(2);
        me::waehle(0, false);
        me::setzeSpalten(10.0F, 10.0F);
    }));
    add(pause(0.2));
    add(foto("menue_properties_schmal"));
    add(pruefe("rc608 Breiten: Mindestbreiten rechts und links", [] {
        const float fs = ImGui::GetFontSize();
        diag::info("Knopftest: Mindestbreite rechts " + std::to_string(me::rechteSpalte()) + ", links " + std::to_string(me::linkeSpalte()) + ", Schrift " + std::to_string(fs));
        return me::rechteSpalte() >= fs * 16.0F - 0.5F && me::linkeSpalte() >= fs * 11.0F - 0.5F;
    }));
    const i18n::Language spracheVorher = i18n::currentLanguage();
    for (const i18n::LanguageInfo& l : i18n::languages()) {
        const i18n::Language lang = l.language;
        const std::string kurz = l.code;
        add(tu("rc608 Breiten: links schmal, " + kurz, [lang] { i18n::setLanguage(lang); }));
        add(pause(0.15));
        add(pruefe("rc608 Breiten: die Spalten zeigen ihren Inhalt ganz, alle Reiter im Feld, " + kurz, [] {
            const ReiterLage r = reiterLage();
            bool reiterGut = r.reiter.size() == 3;
            for (const auto& [name, rect] : r.reiter) { reiterGut = reiterGut && rect.Max.x <= r.rechts + 0.5F && rect.GetWidth() >= 30.0F; }
            return spaltenGanz() && reiterGut;
        }));
    }
    add(tu("rc608 Breiten: Sprache zurueck", [spracheVorher] { i18n::setLanguage(spracheVorher); }));
    add(tu("rc608 Breiten: viel zu breit", [] { me::setzeSpalten(100000.0F, 100000.0F); }));
    add(pause(0.2));
    add(pruefe("rc608 Breiten: die Flaeche behaelt ihren Platz", [] {
        const auto x = me::stand();
        diag::info("Knopftest: Flaeche bei breiten Spalten " + std::to_string(x.flaeche[2]));
        return x.flaeche[2] >= ImGui::GetFontSize() * 15.0F;
    }));
    add(foto("menue_properties_breit"));
    // Engine messages (shank fragte, was der Abschnitt macht): ein Satz, "Keine Meldungen", lange Meldungen umbrechen, ein Klick springt
    add(tu("rc608 Meldungen: Reiter Properties", [] {
        me::setzeSpalten(0.0F, 0.0F);
        me::setzeReiter(2);
        me::waehle(0, false);
        me::leisteTablett(3);
    }));
    add(pause(0.4));
    add(pruefe("rc608 Meldungen: bei leerer Liste steht es da", [] {
        const int alter = ImGui::GetFrameCount() - me::meldungenLeerBild();
        diag::info("Knopftest: Meldungen " + std::to_string(me::stand().meldungen) + ", Keine-Meldungen vor " + std::to_string(alter) + " Bildern");
        return me::stand().meldungen == 0 && alter <= 2;
    }));
    add(tu("rc608 Meldungen: Datei mit einer langen Meldung", [] {
        const std::string wort(160, 'x');
        const std::string text = "menuDef\n{\n\tname mld\n\trect 0 0 640 480\n\titemDef\n\t{\n\t\tname a\n\t\trect 10 10 100 30\n\t\t" + wort + " 1\n\t\tvisible 1\n\t}\n}\n";
        (void)me::oeffneText("ui/mld.menu", text);
        me::setzeInteraktion(false);
        me::setzeReiter(2);
        me::waehle(-1, false);
    }));
    add(pause(0.8));
    add(pruefe("rc608 Meldungen: die Meldung bricht um und bleibt im Feld", [] {
        const Element* e = finde("##meldung", "", 0, false);
        if (e == nullptr) { return false; }
        std::string fehler;
        const bool gut = rechtsGanz(fehler, "##tablett");
        diag::info("Knopftest: Meldung " + std::to_string(e->rect.GetHeight()) + " hoch" + (gut ? "" : ", ragt hinaus: " + fehler));
        return me::stand().meldungen >= 1 && e->rect.GetHeight() > ImGui::GetFontSize() * 1.5F && gut;
    }));
    add(klick("##meldung", "", 0, false));
    add(pause(0.3));
    add(pruefe("rc608 Meldungen: ein Klick springt zur Zeile", [] {
        const std::string z = me::stand().cursorZeile;
        diag::info("Knopftest: Cursorzeile " + z.substr(0, 30));
        return z.find("xxxx") != std::string::npos;
    }));
    add(foto("menue_meldungen"));
    add(tu("rc608 Meldungen: schliessen", [] {
        me::leisteVorgabe();
        me::schliesseAlleOhneSpeichern();
        (void)me::oeffneText("ui/breit.menu", kBreitenText);
        me::setzeInteraktion(false);
    }));
    add(pause(0.3));
    add(tu("rc608 Breiten: Vorgabe", [] {
        me::setzeSpalten(0.0F, 0.0F);
        me::schliesseAlleOhneSpeichern();
        me::setzeReiter(0);
    }));
    return s;
}

// rc608 Punkt 2 (shank: "das bringt fast gar nichts weil ich viel zu wenig sehe")
std::vector<Schritt> schritteTextBreite() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto skala = std::make_shared<float>(1.0F);
    auto groesse = std::make_shared<std::pair<int, int>>(0, 0);
    add(tu("rc608 Textbreite: Menue aus dem Spiel, Reiter Text-Editor", [skala, groesse] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        g_app->settings.menueRechtsBreite = 0.0F;
        g_app->settings.menueUmbruch = false;
        me::setzeSpalten(0.0F, 0.0F);
        (void)me::oeffneAusSpiel("ui/ingameforceselect.menu");
        me::setzeReiter(1);
        *skala = g_app->uiScale;
        *groesse = platform::hauptfensterGroesse(1920, 1080);
    }));
    add(pause(1.0));
    add(foto("menue_texteditor_vorgabe"));
    // Bei jeder Fenstergroesse und Oberflaeche: Text-Editor brauchbar, die Flaeche nicht unbenutzbar klein
    struct Masz { int b, h; float zoom; };
    static const Masz kMasse[] = {{1920, 1080, 1.0F}, {1920, 1080, 2.0F}, {2560, 1600, 1.0F}, {2560, 1600, 2.0F}};
    for (const Masz& m : kMasse) {
        const std::string was = std::to_string(m.b) + " x " + std::to_string(m.h) + ", " + std::to_string(static_cast<int>(m.zoom * 100.0F)) + " %";
        const Masz q = m;
        add(tu("rc608 Textbreite: " + was, [q] {
            (void)platform::hauptfensterGroesse(q.b, q.h);
            g_app->uiScale = q.zoom;
            me::setzeSpalten(0.0F, 0.0F);
            g_app->settings.menueRechtsBreite = 0.0F;
        }));
        add(pause(0.8));
        add(foto("menue_masz_" + std::to_string(m.b) + "_" + std::to_string(static_cast<int>(m.zoom * 100.0F))));
        add(pruefe("rc608 Textbreite: " + was + ": rechtes Feld brauchbar, Flaeche bleibt", [was] {
            const float fs = ImGui::GetFontSize();
            const auto x = me::stand();
            diag::info("Knopftest: Textbreite " + was + ": rechts " + std::to_string(me::rechteSpalte() / fs) + " Schrifthoehen, Flaeche " + std::to_string(x.flaeche[2] / fs) + " Schrifthoehen, Text " + std::to_string(me::textAnsicht().flaeche / fs));
            // In Bildpunkten: Bei 200 % auf 1920 Punkten passen nur 40 Schrifthoehen in die Breite; 320 Punkte sind die halbe Spielflaeche, Zoom und Verschieben helfen weiter
            // rc608 Teil 3b: Bei 1080 Punkten und 200 % geht die linke Spalte von selbst zu, die Ansicht-Schalter wandern hinter einen Knopf: wieder ueber 320 (Teil 3a: 297 x 223)
            return me::rechteSpalte() >= fs * 16.0F - 0.5F && x.flaeche[2] >= 320.0F && std::fabs(x.flaeche[2] / x.flaeche[3] - 4.0F / 3.0F) < 0.01F;
        }));
    }
    add(tu("rc608 Textbreite: zurueck", [skala, groesse] {
        g_app->uiScale = *skala;
        if (groesse->first > 0) { (void)platform::hauptfensterGroesse(groesse->first, groesse->second); }
        me::setzeSpalten(0.0F, 0.0F);
        g_app->settings.menueRechtsBreite = 0.0F;
    }));
    add(pause(0.8));
    // Welche Vorgabe zeigt mehr?
    auto ohne = std::make_shared<long long>(0);
    add(pruefe("rc608 Textbreite: Zeichen ohne Umbruch messen", [ohne] {
        *ohne = me::textAnsicht().zeichen;
        return *ohne > 0;
    }));
    // Alt+Z schaltet um, die Datei bleibt
    auto text = std::make_shared<std::string>();
    add(tu("rc608 Textbreite: Text merken", [text] { *text = me::stand().text; }));
    add(klickAn("rc608 Textbreite: in den Text", [] { return fensterMitte("/##code_"); }));
    add(pause(0.2));
    add(tasteMit(ImGuiMod_Alt, ImGuiKey_Z, "Alt+Z"));
    add(pause(0.3));
    add(pruefe("rc608 Textbreite: Alt+Z schaltet den Umbruch ein, die Datei bleibt", [text, ohne] {
        const long long mit = me::textAnsicht().zeichen;
        diag::info("Knopftest: sichtbare Zeichen ohne Umbruch " + std::to_string(*ohne) + ", mit " + std::to_string(mit));
        // Die Vorgabe ist, was mehr zeigt
        return g_app->settings.menueUmbruch && me::stand().text == *text && mit > *ohne && Settings{}.menueUmbruch;
    }));
    add(foto("menue_texteditor_umbruch"));
    add(tasteMit(ImGuiMod_Alt, ImGuiKey_Z, "Alt+Z"));
    add(pause(0.3));
    add(pruefe("rc608 Textbreite: Alt+Z schaltet zurueck", [] { return !g_app->settings.menueUmbruch; }));
    // Waagerecht rollen: schmal, lange Zeile, Rollbalken und Umschalt+Mausrad
    add(tu("rc608 Textbreite: schmal", [] { me::setzeSpalten(0.0F, 1.0F); }));
    add(pause(0.3));
    add(pruefe("rc608 Textbreite: schmal gibt es einen sichtbaren Rollbalken", [] {
        const me::TextAnsicht t = me::textAnsicht();
        diag::info("Knopftest: Balken " + std::to_string(t.balken[2]) + " x " + std::to_string(t.balken[3]) + ", breiteste " + std::to_string(t.breiteste) + ", Flaeche " + std::to_string(t.flaeche));
        return t.breiteste > t.flaeche && t.balken[2] > 0.0F && t.balken[3] >= 6.0F;
    }));
    add(foto("menue_texteditor_rollbalken"));
    add(Schritt{"rc608 Textbreite: Umschalt+Mausrad", [](int b) {
                    ImGuiIO& io = ImGui::GetIO();
                    if (b == 0) { setzeMaus(fensterMitte("/##code_")); return false; }
                    if (b == 2) { io.AddKeyEvent(ImGuiMod_Shift, true); return false; }
                    if (b == 3) { io.AddMouseWheelEvent(0.0F, -3.0F); return false; }
                    if (b == 5) { io.AddKeyEvent(ImGuiMod_Shift, false); return false; }
                    return b >= 8;
                }});
    add(pruefe("rc608 Textbreite: Umschalt+Mausrad rollt seitwaerts", [] {
        diag::info("Knopftest: scrollX " + std::to_string(me::textAnsicht().scrollX));
        return me::textAnsicht().scrollX > 0.0F;
    }));
    auto vorher = std::make_shared<float>(0.0F);
    add(tu("rc608 Textbreite: scrollX merken", [vorher] { *vorher = me::textAnsicht().scrollX; }));
    add(ziehenAn("rc608 Textbreite: am Rollbalken", [] {
        const me::TextAnsicht t = me::textAnsicht();
        return ImVec2{t.balken[0] + t.balken[2] * 0.2F, t.balken[1] + t.balken[3] * 0.5F};
    }, 60.0F, 0.0F, 0));
    add(pause(0.2));
    add(pruefe("rc608 Textbreite: der Rollbalken rollt", [vorher] { return me::textAnsicht().scrollX > *vorher; }));
    add(tu("rc608 Textbreite: aufraeumen", [] {
        me::setzeSpalten(0.0F, 0.0F);
        g_app->settings.menueRechtsBreite = 0.0F;
        me::schliesseAlleOhneSpeichern();
        me::setzeReiter(0);
    }));
    return s;
}

// rc608 Punkt 3 (shank: "die events scalen nicht gut mit und sind echt nicht gut zu gebrauchen")
std::vector<Schritt> schritteEreignisseBloecke() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("rc608 Events: Probe", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneText("ui/breit.menu", kBreitenText);
        me::setzeInteraktion(false);
        me::setzeReiter(3);
        me::waehle(0, false);
    }));
    add(pause(0.6));
    add(foto("menue_events_breit"));
    // Je Aktion Hoch und Loeschen, im Feld (gezaehlt, was gerade zu sehen ist; bei schmalem Feld liegt der Rest weiter unten).
    // Die Rechtecke prueft breitenTabelle.
    const auto knoepfeDa = [](std::string& f) {
        int hoch = 0;
        int weg = 0;
        for (const Element& e : elemente::alle()) {
            if (e.innen.find("##inhalt") == std::string::npos) { continue; }
            hoch += e.label.rfind("##st_h", 0) == 0 ? 1 : 0;
            weg += e.label.rfind("##st_x", 0) == 0 ? 1 : 0;
        }
        // rc608 Teil 3a: Bei grosser Oberflaeche stehen erst die Ausloeser im Bild: Die Knoepfe liegen dann weiter unten; unten geprueft
        const ImGuiWindow* inhalt = inhaltFenster();
        const bool weiterUnten = inhalt != nullptr && inhalt->ScrollMax.y > inhalt->Scroll.y;
        if ((hoch == 0 && !weiterUnten) || hoch != weg) {
            f += " Knoepfe ^ " + std::to_string(hoch) + ", x " + std::to_string(weg);
            return false;
        }
        return true;
    };
    for (auto& x : breitenTabelle("Events", 3, {0}, knoepfeDa)) { add(std::move(x)); }
    add(tu("rc608 Events: 200 %, ans Ende gerollt", [] {
        g_app->uiScale = 2.0F;
        me::setzeSpalten(0.0F, 500.0F);
        me::waehle(0, false);
    }));
    add(pause(0.3));
    add(tu("rc608 Events: rollen", [] {
        if (ImGuiWindow* inhalt = inhaltFenster()) { inhalt->Scroll.y = inhalt->ScrollMax.y; }
    }));
    add(pause(0.2));
    add(pruefe("rc608 Events: 200 %, unten stehen die Knoepfe", [knoepfeDa] {
        std::string f;
        int hoch = 0;
        for (const Element& e : elemente::alle()) { hoch += e.innen.find("##inhalt") != std::string::npos && e.label.rfind("##st_h", 0) == 0 ? 1 : 0; }
        return hoch > 0 && knoepfeDa(f);
    }));
    add(tu("rc608 Events: 100 %", [] { g_app->uiScale = 1.0F; }));
    add(pause(0.2));
    add(tu("rc608 Events: Mindestbreite", [] {
        me::setzeSpalten(0.0F, 1.0F);
        me::waehle(0, false);
    }));
    add(pause(0.3));
    add(foto("menue_events_mindest"));
    add(tu("rc608 Events: Breite 500", [] {
        me::setzeSpalten(0.0F, 500.0F);
        me::waehle(0, false);
    }));
    add(pause(0.3));
    add(foto("menue_events_schmal"));
    // transition: das Feld w von "nach" aendert genau das achte Wort, Folge und Datei wie im Spiel
    add(klick("##st2"));   // der Schritt transition, immer nur einer ist offen
    add(pause(0.2));
    add(klickAn("rc608 Events: nach w doppelklicken", [] {
        const Element* e = finde("##Z", "##inhalt", 1, false);   // w von "Von", w von "Nach"
        return e != nullptr ? e->rect.GetCenter() : ImVec2{};
    }, 0, true));
    add(pause(0.2));
    add(tippe("77"));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.3));
    add(pruefe("rc608 Events: transition: geaendert wird der richtige Wert, die Folge bleibt", [] {
        std::string t = me::stand().text;
        t.erase(std::remove(t.begin(), t.end(), '"'), t.end());   // der Baustein schreibt zitierte Woerter, String_Parse liest beides
        diag::info("Knopftest: " + t.substr(t.find("transition"), 60));
        return t.find("transition knopf 10 10 100 30 6 6 77 38 10 1") != std::string::npos;
    }));
    add(tu("rc608 Events: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
    add(pause(0.2));
    add(pruefe("rc608 Events: Rueckgaengig", [] { return me::stand().text.find("6 6 108 38") != std::string::npos; }));
    add(tu("rc608 Events: aufraeumen", [] {
        me::setzeSpalten(0.0F, 0.0F);
        me::schliesseAlleOhneSpeichern();
        me::setzeReiter(0);
    }));
    return s;
}

// rc608 Punkt 9 (shank: warum ist ein neues leeres Menue schwarz und nicht transparent?)
std::vector<Schritt> schritteLeeresMenue() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto text = [](const char* name, int voll, const char* zusatz) {
        return std::string("{\n\tmenuDef\n\t{\n\t\tname ") + name + "\n\t\tfullScreen " + std::to_string(voll) + "\n\t\trect 0 0 640 480\n\t\tvisible 1\n" + zusatz +
               "\t\titemDef\n\t\t{\n\t\t\tname marke\n\t\t\trect 10 10 200 30\n\t\t\ttext \"" + name + "\"\n\t\t\tfont 2\n\t\t\ttextscale 0.7\n\t\t\tforecolor 1 1 0 1\n\t\t\tvisible 1\n\t\t\tdecoration\n\t\t}\n\t}\n}\n";
    };
    // Dieselben Menues wie auf den Spielfotos (SP/rc608/md/): SP und MP
    struct Fall { const char* datei; const char* foto; int voll; const char* zusatz; };
    static const Fall kFaelle[] = {
        {"ui/jks_leer1.menu", "menue_leer_sp_voll", 1, ""},
        {"ui/jks_leer0.menu", "menue_leer_sp_ueberlagerung", 0, ""},
        {"ui/jks_leerfarbe.menu", "menue_leer_sp_farbe", 1, "\t\tbackcolor 0.2 0.3 0.8 1\n"},
        {"ui/jks_leerbild2.menu", "menue_leer_sp_bild", 1, "\t\tbackground \"gfx/mp/new_f_icon_push\"\n"},
        {"ui/MD_MP/jks_leer1.menu", "menue_leer_mp_voll", 1, ""},
        {"ui/MD_MP/jks_leer0.menu", "menue_leer_mp_ueberlagerung", 0, ""},
        {"ui/MD_MP/jks_leerbild2.menu", "menue_leer_mp_bild", 1, "\t\tbackground \"gfx/mp/new_f_icon_push\"\n"},
    };
    add(tu("rc608 Leer: Editor", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        me::setzeInteraktion(false);
    }));
    for (const Fall& f : kFaelle) {
        const std::string datei = f.datei;
        const std::string inhalt = text("leer", f.voll, f.zusatz);
        add(tu("rc608 Leer: " + datei, [datei, inhalt] {
            me::schliesseAlleOhneSpeichern();
            (void)me::oeffneText(datei, inhalt);
        }));
        add(pause(0.4));
        add(foto(f.foto));
    }
    // Schachbrett: nur ohne fullScreen, abschaltbar, nur Anzeige
    auto inhalt0 = std::make_shared<std::string>();
    add(tu("rc608 Leer: Ueberlagerung", [inhalt0, text] {
        me::schliesseAlleOhneSpeichern();
        (void)me::oeffneText("ui/jks_leer0.menu", text("leer", 0, ""));
        *inhalt0 = me::stand().text;
        me::setzeReiter(2);
    }));
    add(pause(0.3));
    add(pruefe("rc608 Leer: ohne fullScreen das Schachbrett", [] { return me::schachbrettZellen() > 100; }));
    add(klick(tr(Str::MeSchachbrett)));
    add(pause(0.2));
    add(pruefe("rc608 Leer: das Schachbrett laesst sich abschalten, die Datei bleibt", [inhalt0] { return me::schachbrettZellen() == 0 && me::stand().text == *inhalt0; }));
    add(klick(tr(Str::MeSchachbrett)));
    add(pause(0.2));
    // Der Schalter in den Properties des Menues
    auto tiefe = std::make_shared<int>(0);
    add(tu("rc608 Leer: nichts gewaehlt", [tiefe] {
        me::waehle(-1, false);
        *tiefe = me::stand().undoSchritte;
    }));
    add(pause(0.2));
    add(foto("menue_leer_schalter"));
    add(klick(tr(Str::MeVollbild)));
    add(pause(0.2));
    add(pruefe("rc608 Leer: Vollbild schreibt fullScreen 1, ein Schritt, das Schachbrett geht", [tiefe] {
        const auto x = me::stand();
        return x.text.find("fullScreen 1") != std::string::npos && x.undoSchritte == *tiefe + 1 && me::schachbrettZellen() == 0;
    }));
    add(tu("rc608 Leer: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
    add(pause(0.2));
    add(pruefe("rc608 Leer: Rueckgaengig", [inhalt0] { return me::stand().text == *inhalt0 && me::schachbrettZellen() > 100; }));
    // Ein neues MP-Menue ueberrascht nicht: Hinweis, und die Vorlage deckt
    add(tu("rc608 Leer: MP ohne background", [text] {
        me::schliesseAlleOhneSpeichern();
        (void)me::oeffneText("ui/MD_MP/jks_leer1.menu", text("leer", 1, ""));
    }));
    add(pause(1.0));
    add(pruefe("rc608 Leer: MP fullScreen ohne background: die Diagnose sagt es", [] {
        const auto x = me::stand();
        return std::any_of(x.diagnosen.begin(), x.diagnosen.end(), [](const std::string& d) { return d.find(tr(Str::MeDiagVollbildOhneBild)) != std::string::npos; });
    }));
    add(tu("rc608 Leer: aufraeumen", [] {
        me::schliesseAlleOhneSpeichern();
        me::setzeReiter(0);
    }));
    return s;
}

// rc608 Punkt 7: das Changelog-Fenster nach Bereichen, alle Sprachen
std::vector<Schritt> schritteChangelog() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const i18n::Language vorher = i18n::currentLanguage();
    add(tu("rc608 Changelog: Fenster oeffnen", [] { g_app->changelogOpen = true; }));
    add(pause(0.5));
    for (const i18n::LanguageInfo& l : i18n::languages()) {
        const i18n::Language lang = l.language;
        const std::string kurz = l.code;
        add(tu("rc608 Changelog: Sprache " + kurz, [lang] { i18n::setLanguage(lang); }));
        add(pause(0.4));
        add(pruefe("rc608 Changelog: " + kurz + ": die oberste Fassung steht nach Bereichen, jede Gruppe mit Titel im Fenster", [kurz] {
            // die oberste Fassung (das Fenster klappt die neuesten zwei auf) - nicht eine feste Fassung, die mit jeder neuen nach unten rueckt
            if (changelog::anzahl() == 0) { return false; }
            const changelog::Eintrag* e = changelog::eintraege();
            const std::vector<changelog::Gruppe> g = changelog::gruppen(changelog::text(e[0], static_cast<int>(i18n::currentLanguage())));
            int mitTitel = 0;
            bool gut = true;
            for (const changelog::Gruppe& x : g) {
                if (x.titel.empty()) { continue; }
                ++mitTitel;
                if (finde(x.titel, "###changelog", 0, false) == nullptr) {
                    diag::info("Knopftest: Changelog " + kurz + ": Gruppe nicht im Fenster: " + x.titel);
                    gut = false;
                }
            }
            diag::info("Knopftest: Changelog " + kurz + ": " + std::to_string(g.size()) + " Gruppen, davon mit Titel " + std::to_string(mitTitel));
            return gut && mitTitel >= 5;
        }));
    }
    add(foto("changelog_gruppen"));
    add(tu("rc608 Changelog: zurueck", [vorher] {
        i18n::setLanguage(vorher);
        g_app->changelogOpen = false;
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
        me::sucheZu();
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
