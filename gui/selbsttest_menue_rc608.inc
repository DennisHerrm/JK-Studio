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
        add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_1, "Strg+1"));
        add(pause(0.1));
        add(halteTaste(ImGuiMod_Ctrl, true));
        add(radAn("Strg+Rad " + name, an(450, 325), klicks));
        add(halteTaste(ImGuiMod_Ctrl, false));
        add(pruefe("rc608 Zoom: Stufe " + name + " erreicht", [] {
            diag::info("Knopftest: Zoom " + std::to_string(me::stand().zoomProzent) + " %");
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
    erklaert("bild", 200, 335, mit(Str::MeSchreibenBild, "gfx/menus/menu_blendbox"));
    // Hierarchie zeigt den Text in der Zeile
    add(tu("rc608 Schreiben: Reiter Hierarchie", [] { me::setzeReiter(0); }));
    add(pause(0.2));
    add(klick("knopf  ITEM_TYPE_BUTTON  \"Back\""));
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
    add(klick("zwei_a  ITEM_TYPE_BUTTON"));
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
