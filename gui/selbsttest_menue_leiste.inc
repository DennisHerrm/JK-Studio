// selbsttest_menue_leiste.inc - rc608 Teil 3a: die Seitenleisten des Menue-Editors (BHED_MENUE_NUR=leiste, leistebreit, rahmen).
// Eingebunden in namespace menuetest (selbsttest_menue.inc). Alles mit echten Klicks; geprueft wird der Text der Datei.

const char* const kLeisteText =
    "menuDef\n{\n  name \"leiste\"\n  rect 0 0 640 480\n  fullScreen 1\n  visible 1\n"
    "  onOpen { setcvar ui_tab \"0\" }\n"
    "  itemDef\n  {\n    name \"knopf\"\n    type ITEM_TYPE_BUTTON\n    rect 100 100 200 40\n    text \"Back\"\n    textscale 0.7\n    forecolor 1 1 1 1\n    focusSound \"sound/interface/button1.wav\"\n    visible 1\n"
    "    action { play \"sound/interface/button1.wav\" ; setitemcolor knopf forecolor 1 0.5 0.25 1 ; transition knopf 10 10 100 30 6 6 108 38 10 1 ; setcvar ui_name 1 ; show knopf }\n"
    "    mouseEnter { setitemcolor knopf backcolor 1 1 1 1 }\n    onFocus { show fehlt }\n  }\n"
    "  itemDef\n  {\n    name \"titel\"\n    type ITEM_TYPE_TEXT\n    rect 120 30 400 50\n    text \"Titel\"\n    textscale 1\n    decoration\n    visible 1\n  }\n"
    "  itemDef\n  {\n    name \"zweit\"\n    type ITEM_TYPE_BUTTON\n    rect 100 200 200 40\n    text \"Zwei\"\n    textscale 0.55\n    visible 1\n    action { show knopf ; hide titel }\n  }\n"
    "}\n";

// Das erste Feld mit dieser Kennung unter dem Kopf einer Gruppe
ImVec2 feldUnterGruppe(const std::string& gruppe, const std::string& feld) {
    const Element* g = finde("##gr_" + gruppe, "", 0, false);
    if (g == nullptr) { return ImVec2{-1.0F, -1.0F}; }
    for (int k = 0;; ++k) {
        const Element* e = finde(feld, "##inhalt", k, false);
        if (e == nullptr) { break; }
        if (e->rect.Min.y > g->rect.Min.y) { return e->rect.GetCenter(); }
    }
    return ImVec2{-1.0F, -1.0F};
}

// Ein Zug mit gedrueckter Taste von einer Stelle aus
Schritt ziehenAb(const std::string& was, std::function<ImVec2()> ort, float dx, float dy) {
    auto start = std::make_shared<ImVec2>();
    return Schritt{was, [=](int b) {
                       ImGuiIO& io = ImGui::GetIO();
                       if (b == 0) {
                           *start = ort();
                           setzeMaus(*start);
                           return false;
                       }
                       if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
                       if (b >= 3 && b <= 8) {
                           const float t = static_cast<float>(b - 2) / 6.0F;
                           setzeMaus(ImVec2{start->x + dx * t, start->y + dy * t});
                           return false;
                       }
                       if (b == 9) { io.AddMouseButtonEvent(0, false); return false; }
                       return b >= 12;
                   }};
}

bool zeileDa(const std::string& text, const std::string& teil) { return text.find(teil) != std::string::npos; }

// Wie die Datei aussieht, wenn eine Zeile mit dem Schluessel im genannten Element fehlt
std::string ohneZeile(std::string t, const std::string& zeile) {
    const std::size_t p = t.find(zeile);
    if (p == std::string::npos) { return t; }
    std::size_t a = p;
    while (a > 0 && t[a - 1] != '\n') { --a; }
    std::size_t e = t.find('\n', p);
    return t.erase(a, e == std::string::npos ? std::string::npos : e + 1 - a);
}

std::vector<Schritt> schritteLeiste() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto probe = [](const char* name) {
        return tu(std::string("rc608 Leiste: Probe, ") + name, [name] {
            me::schliesseAlleOhneSpeichern();
            g_app->leftMode = 4;
            tourSchliessen();
            (void)me::oeffneText("ui/leiste.menu", kLeisteText);
            me::setzeInteraktion(false);
            me::setzeSpalten(0.0F, 0.0F);
            me::waehle(me::itemNummer(name), false);
            me::setzeReiter(2);
            me::leisteVorgabe();
            if (ImGuiWindow* w = inhaltFenster()) { ImGui::SetScrollY(w, 0.0F); }   // gerollt bleibt das Feld stehen, auch ueber Dateien hinweg
        });
    };
    auto vor = std::make_shared<std::string>();
    auto tiefe = std::make_shared<int>(0);
    const auto merken = [=] { return tu("rc608 Leiste: merken", [=] { *vor = me::stand().text; *tiefe = me::stand().undoSchritte; }); };
    const auto einSchritt = [=](const std::string& was) {
        return pruefe("rc608 Leiste: " + was + ": ein Undo-Schritt", [=] {
            const auto x = me::stand();
            if (x.undoSchritte != *tiefe + 1) { diag::info("Knopftest: " + was + ": " + std::to_string(x.undoSchritte - *tiefe) + " Schritte"); }
            return x.undoSchritte == *tiefe + 1 && x.text != *vor;
        });
    };
    const auto zurueck = [=](const std::string& was) {
        std::vector<Schritt> l;
        l.push_back(tu("rc608 Leiste: " + was + ": Rueckgaengig", [] { (void)me::rueckgaengig(); }));
        l.push_back(pause(0.15));
        l.push_back(pruefe("rc608 Leiste: " + was + ": Rueckgaengig byte-gleich", [=] { return me::stand().text == *vor; }));
        return l;
    };
    const auto zurueckAdd = [&](const std::string& was) {
        for (auto& x : zurueck(was)) { add(std::move(x)); }
    };

    // --- Reiter und Karte
    add(probe("knopf"));
    add(pause(0.5));
    add(foto("menue_leiste_properties"));
    add(pruefe("rc608 Leiste: Reiter Properties, Events, Text editor; die Hierarchie steht links", [] {
        const ReiterLage r = reiterLage();
        const bool reihe = r.reiter.size() == 3 && r.reiter[0].first == tr(Str::MeReiterEigenschaften) && r.reiter[1].first == tr(Str::MeEreignisse) && r.reiter[2].first == tr(Str::MeReiterText);
        return reihe && zeileVon("knopf").x >= 0.0F && zeileVon("knopf").x < me::stand().flaeche[0];
    }));
    add(pruefe("rc608 Leiste: Karte mit allen Knoepfen, ausserhalb des rollenden Teils", [] {
        for (const char* k : {"##k_sichtbar", "##k_sperren", "##k_vor", "##k_hin", "##k_dup", "##k_weg", "##name"}) {
            const Element* e = finde(k, "##karte", 0, false);
            if (e == nullptr || e->innen.find("##inhalt") != std::string::npos) {
                diag::info(std::string("Knopftest: Karte ohne ") + k);
                return false;
            }
        }
        return true;
    }));
    auto kartenY = std::make_shared<float>(0.0F);
    add(tu("rc608 Leiste: rollen", [kartenY] {
        const Element* e = finde("##k_weg", "##karte", 0, false);
        *kartenY = e != nullptr ? e->rect.Min.y : -1.0F;
        for (ImGuiWindow* x : GImGui->Windows) {
            const std::string n = x->Name;
            if (x->WasActive && n.find("/##inhalt_") != std::string::npos) { ImGui::SetScrollY(x, 200.0F); }
        }
    }));
    add(pause(0.15));
    add(pruefe("rc608 Leiste: die Karte bleibt beim Rollen stehen", [kartenY] {
        const Element* e = finde("##k_weg", "##karte", 0, false);
        return e != nullptr && std::fabs(e->rect.Min.y - *kartenY) < 0.5F;
    }));

    // --- Gruppen, Suche, File keys
    add(probe("knopf"));
    add(pause(0.3));
    // Was nicht im Bild steht, hat keine Beschriftung: oben die ersten Gruppen, am Ende des Felds die letzten
    const auto inhaltRollen = [](bool ende) {
        return tu("rc608 Leiste: Inhalt rollen", [ende] {
            if (ImGuiWindow* w = inhaltFenster()) { ImGui::SetScrollY(w, ende ? w->ScrollMax.y : 0.0F); }
        });
    };
    add(pruefe("rc608 Leiste: oben die ersten Gruppen und der Text", [] {
        for (const char* g : {"##gr_inhalt", "##gr_lage", "##gr_aussehen"}) {
            if (finde(g, "##inhalt", 0, false) == nullptr) {
                diag::info(std::string("Knopftest: Gruppe fehlt ") + g);
                return false;
            }
        }
        return finde("##text", "##inhalt", 0, false) != nullptr;
    }));
    add(inhaltRollen(true));
    add(pause(0.15));
    add(pruefe("rc608 Leiste: unten Logik und weitere Eintraege, zugeklappt", [] {
        for (const char* g : {"##gr_logik", "##gr_mehr"}) {
            if (finde(g, "##inhalt", 0, false) == nullptr) {
                diag::info(std::string("Knopftest: Gruppe fehlt ") + g);
                return false;
            }
        }
        return finde("##group", "##inhalt", 0, false) == nullptr;
    }));
    add(klick("##gr_logik"));
    add(pause(0.15));
    add(inhaltRollen(true));
    add(pause(0.15));
    add(pruefe("rc608 Leiste: Klick klappt die Gruppe auf", [] { return finde("##group", "##inhalt", 0, false) != nullptr; }));
    add(klick("##gr_logik"));
    add(pause(0.15));
    add(pruefe("rc608 Leiste: Klick klappt sie wieder zu, die Felder bleiben angemeldet", [] {
        const std::vector<std::string> f = me::eigenschaftsFelder();
        return finde("##group", "##inhalt", 0, false) == nullptr && std::find(f.begin(), f.end(), "group") != f.end();
    }));
    add(inhaltRollen(false));
    add(pause(0.15));
    add(klick("##feldsuche"));
    add(tippe("textalignx"));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: Suche nach dem Schluessel findet das Feld, andere Gruppen fallen weg", [] {
        return finde("##gr_inhalt", "##inhalt", 0, false) != nullptr && finde("##gr_lage", "##inhalt", 0, false) == nullptr && finde("##text", "##inhalt", 0, false) == nullptr &&
               finde("##a_mitte", "##inhalt", 0, false) != nullptr;
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(tippe("zzzzz"));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: Suche ohne Treffer: keine Gruppe mehr", [] { return finde("##gr_inhalt", "##inhalt", 0, false) == nullptr && finde("##gr_mehr", "##inhalt", 0, false) == nullptr; }));
    add(foto("menue_leiste_suche_leer"));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(taste(ImGuiKey_Delete, "Entf"));
    add(pause(0.2));
    add(klick(tr(Str::MlDateiSchluessel)));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: File keys zeigt die Schluessel", [] { return g_app->settings.menueDateiSchluessel && me::leisteStand().schluesselZeilen > 3; }));
    add(foto("menue_leiste_schluessel"));
    add(klick(tr(Str::MlDateiSchluessel)));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: File keys wieder aus", [] { return !g_app->settings.menueDateiSchluessel && me::leisteStand().schluesselZeilen == 0; }));

    // --- Zahlen: Feld anklicken tippt, Ziehen an der Beschriftung veraendert
    add(klickAn("rc608 Leiste: ein Klick in die Zahl", [] { return feldUnterGruppe("lage", "##Y"); }));
    add(pause(0.15));
    add(pruefe("rc608 Leiste: ein Klick in die Zahl tippt", [] { return ImGui::GetIO().WantTextInput; }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.15));
    add(merken());
    add(ziehenAb("rc608 Leiste: an der Beschriftung X ziehen", [] { return feldUnterGruppe("lage", "##ziehX"); }, 30.0F, 0.0F));
    add(pause(0.15));
    add(pruefe("rc608 Leiste: Ziehen an der Beschriftung veraendert die Zahl", [] {
        diag::info("Knopftest: rect nach Ziehen " + rectVon("knopf"));
        return rectVon("knopf").rfind("130 100", 0) == 0;
    }));
    add(einSchritt("Ziehen an der Beschriftung"));
    zurueckAdd("Ziehen an der Beschriftung");

    // --- Byte-gleich gegen den alten Weg
    struct Fall {
        const char* name;
        std::function<void(std::vector<Schritt>&)> tun;
        std::vector<std::pair<std::string, std::vector<std::string>>> alt;
    };
    const auto tippeIn = [](const std::string& gruppe, const std::string& feld, const std::string& wert) {
        return [=](std::vector<Schritt>& l) {
            l.push_back(klickAn("in " + feld, [=] { return feldUnterGruppe(gruppe, feld); }));
            l.push_back(pause(0.1));
            l.push_back(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
            l.push_back(tippe(wert));
            l.push_back(taste(ImGuiKey_Enter, "Enter"));
        };
    };
    const std::vector<Fall> faelle = {
        {"rect", tippeIn("lage", "##X", "123"), {{"rect", {"123", "100", "200", "40"}}}},
        {"text", tippeIn("inhalt", "##text", "Neu"), {{"text", {"\"Neu\""}}}},
        {"textscale", tippeIn("inhalt", "##textscale", "0.9"), {{"textscale", {"0.9"}}}},
        // es/cs: mit den laengeren Texten liegt die Fuellung unter dem Rand der Spalte - erst hinrollen wie ein Anwender
        {"style", [](std::vector<Schritt>& l) {
             l.push_back(rolleRechtsZu(tr(Str::MlFuellFarbe)));
             l.push_back(klick(tr(Str::MlFuellFarbe), "##rechts"));
         }, {{"style", {"WINDOW_STYLE_FILLED"}}}},
    };
    for (const Fall& f : faelle) {
        auto neuWeg = std::make_shared<std::string>();
        const std::string n = f.name;
        add(probe("knopf"));
        add(pause(0.25));
        add(merken());
        std::vector<Schritt> l;
        f.tun(l);
        for (Schritt& x : l) { add(std::move(x)); }
        add(pause(0.2));
        add(einSchritt(n));
        add(tu("rc608 Leiste: " + n + ": Ergebnis merken", [neuWeg] { *neuWeg = me::stand().text; }));
        zurueckAdd(n);
        add(pause(0.1));
        const auto alt = f.alt;
        add(tu("rc608 Leiste: " + n + ": alter Weg", [alt] {
            for (const auto& [k, w] : alt) { me::setzeEigenschaftAuswahl(k, w); }
        }));
        add(pause(0.15));
        add(pruefe("rc608 Leiste: " + n + ": Datei byte-gleich gegen den alten Weg", [neuWeg, n] {
            const bool gleich = me::stand().text == *neuWeg;
            if (!gleich) { diag::info("Knopftest: neu " + neuWeg->substr(0, 300) + " | alt " + me::stand().text.substr(0, 300)); }
            return gleich;
        }));
    }

    // --- Farbe: eine Zeile, RGBA erst nach dem Klick auf das Farbfeld, zuletzt benutzte Farben
    add(probe("knopf"));
    add(pause(0.25));
    add(tu("rc608 Leiste: keine alten Farben", [] { g_app->settings.menueLetzteFarben.clear(); }));
    add(pruefe("rc608 Leiste: Farbe zu: kein RGBA-Feld", [] { return finde("##deck", "##inhalt", 0, false) == nullptr; }));
    add(klick("##tupfer", "##inhalt"));
    add(pause(0.15));
    add(pruefe("rc608 Leiste: Klick auf das Farbfeld oeffnet RGBA", [] { return finde("##deck", "##inhalt", 0, false) != nullptr && me::leisteStand().farbeOffen == "forecolor"; }));
    add(foto("menue_leiste_farbe"));
    add(merken());
    add(klickAn("rc608 Leiste: R", [] { return feldUnterGruppe("inhalt", "##X"); }));
    add(pause(0.1));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(tippe("0.5"));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: die Farbe steht in der Datei", [] { return zeileDa(me::stand().text, "forecolor 0.5 1 1 1"); }));
    add(einSchritt("Farbe"));
    add(pruefe("rc608 Leiste: die Farbe steht unter den zuletzt benutzten", [] { return g_app->settings.menueLetzteFarben.rfind("0.5 1 1 1", 0) == 0 && finde("##letzte", "##inhalt", 0, false) != nullptr; }));
    zurueckAdd("Farbe");
    add(tu("rc608 Leiste: anderes Element", [] { me::waehle(me::itemNummer("zweit"), false); }));
    add(pause(0.2));
    add(klick("##tupfer", "##inhalt"));
    add(pause(0.15));
    add(merken());
    add(klick("##letzte", "##inhalt"));
    add(pause(0.25));
    add(pruefe("rc608 Leiste: ein Klick auf eine zuletzt benutzte Farbe setzt sie", [] {
        const std::string t = me::stand().text;
        return zeileDa(t.substr(t.find("name \"zweit\"")), "forecolor 0.5 1 1 1");
    }));
    add(einSchritt("zuletzt benutzte Farbe"));
    zurueckAdd("zuletzt benutzte Farbe");

    // --- Ausrichtung bleibt beim Skalieren, auch senkrecht
    add(probe("knopf"));
    add(pause(0.25));
    add(merken());
    add(klick("##a_mitte"));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: Mitte schreibt textalign und textalignx", [] {
        const std::string t = me::stand().text;
        diag::info("Knopftest: Mitte: " + t.substr(t.find("name \"knopf\""), 240));
        return zeileDa(t, "ITEM_ALIGN_CENTER") && zeileDa(t, "textalignx") && zeileDa(t, "100");
    }));
    add(einSchritt("Ausrichtung Mitte"));
    add(pruefe("rc608 Leiste: Datei hat nur die Schluessel des Spiels", [] { return !zeileDa(me::stand().text, "jkstudio"); }));
    add(ziehFlaeche("rc608 Leiste: Griff rechts Mitte ziehen", 300, 120, 0, 0, 60, 0));
    add(pause(0.15));
    add(pruefe("rc608 Leiste: Mitte bleibt beim Skalieren: textalignx ist die halbe Breite", [] {
        const me::ItemLage* l = nullptr;
        const std::vector<me::ItemLage> v = me::itemLagen();
        for (const me::ItemLage& x : v) {
            if (x.name == "knopf") { l = &x; }
        }
        const std::string t = me::stand().text;
        const std::string soll = bhed::menue::zahlText(std::round((l != nullptr ? l->b : 0.0F) * 0.5F));
        diag::info("Knopftest: Breite " + std::to_string(l != nullptr ? l->b : 0.0F) + ", soll textalignx " + soll);
        return l != nullptr && l->b > 205.0F && zeileDa(t, "textalignx") && zeileDa(t, soll);
    }));
    add(klick("##v_mitte"));
    add(pause(0.2));
    auto yMitte = std::make_shared<float>(0.0F);
    add(pruefe("rc608 Leiste: Mitte senkrecht: textaligny = (Hoehe - Schrift) / 2", [yMitte] {
        const auto l = me::leisteStand();
        const float th = l.textOben + l.textUnten;
        const float soll = std::round((40.0F - th) * 0.5F);
        *yMitte = soll;
        const std::string t = me::stand().text;
        diag::info("Knopftest: Textaligny soll " + std::to_string(soll) + " bei Schrift " + std::to_string(th));
        return th > 1.0F && zeileDa(t, "textaligny") && zeileDa(t.substr(t.find("textaligny")), bhed::menue::zahlText(soll));
    }));
    add(ziehFlaeche("rc608 Leiste: Griff unten Mitte ziehen", 200, 140, 0, 0, 0, 40));
    add(pause(0.15));
    add(pruefe("rc608 Leiste: senkrecht bleibt die Mitte beim Skalieren", [] {
        const std::vector<me::ItemLage> v = me::itemLagen();
        float h = 0.0F;
        for (const me::ItemLage& x : v) {
            if (x.name == "knopf") { h = x.h; }
        }
        const float soll = std::round((h - me::leisteStand().textOben - me::leisteStand().textUnten) * 0.5F);
        const std::string t = me::stand().text;
        diag::info("Knopftest: Hoehe " + std::to_string(h) + ", soll textaligny " + std::to_string(soll));
        const std::size_t p = t.find("textaligny");
        return h > 45.0F && p != std::string::npos && t.compare(t.find_first_not_of(" \t", p + 10), bhed::menue::zahlText(soll).size(), bhed::menue::zahlText(soll)) == 0;
    }));
    add(klickAn("rc608 Leiste: eigene Zahl bei Y", [] { return feldUnterGruppe("inhalt", "##Y"); }));
    add(pause(0.1));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(tippe("3"));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.2));
    add(ziehFlaeche("rc608 Leiste: noch einmal skalieren", 200, 180, 0, 0, 0, 30));
    add(pause(0.15));
    add(pruefe("rc608 Leiste: Wer die Zahl selbst aendert, schaltet das Folgen ab", [] {
        const std::string t = me::stand().text;
        const std::size_t p = t.find("textaligny");
        return p != std::string::npos && t.compare(t.find_first_not_of(" \t", p + 10), 1, "3") == 0;
    }));

    // --- Fuellung: Felder nur, wenn sie wirken
    add(probe("knopf"));
    add(pause(0.25));
    add(pruefe("rc608 Leiste: keine Fuellung: weder Fuellfarbe noch Bildfeld", [] {
        const std::vector<std::string> f = me::eigenschaftsFelder();
        return std::find(f.begin(), f.end(), "backcolor") == f.end() && std::find(f.begin(), f.end(), "background") == f.end() && std::find(f.begin(), f.end(), "bordercolor") == f.end();
    }));
    add(rolleRechtsZu(tr(Str::MlFuellFarbe)));
    add(klick(tr(Str::MlFuellFarbe), "##rechts"));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: Farbe: die Fuellfarbe erscheint", [] {
        const std::vector<std::string> f = me::eigenschaftsFelder();
        return std::find(f.begin(), f.end(), "backcolor") != f.end() && zeileDa(me::stand().text, "WINDOW_STYLE_FILLED");
    }));
    add(rolleRechtsZu(tr(Str::MlFuellBild)));
    add(klick(tr(Str::MlFuellBild), "##rechts"));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: Bild: das Bildfeld erscheint, die Fuellfarbe faellt weg", [] {
        const std::vector<std::string> f = me::eigenschaftsFelder();
        return std::find(f.begin(), f.end(), "backcolor") == f.end() && std::find(f.begin(), f.end(), "background") != f.end() && zeileDa(me::stand().text, "WINDOW_STYLE_SHADER");
    }));

    // --- Hilfezeile
    add(probe("knopf"));
    add(pause(0.25));
    add(Schritt{"rc608 Leiste: Zeiger auf die Textgroesse", [](int b) {
                    if (b == 0) { setzeMaus(feldUnterGruppe("inhalt", "##textscale")); }
                    return b >= 3;
                }});
    add(pruefe("rc608 Leiste: die Hilfezeile sagt sofort, was das Feld bewirkt", [] {
        const std::string h = me::leisteStand().hilfe;
        diag::info("Knopftest: Hilfezeile " + h);
        return zeileDa(h, "textscale") && zeileDa(h, tr(Str::MlFGroesseTip));
    }));
    add(Schritt{"rc608 Leiste: Zeiger auf die Flaeche", [](int b) {
                    if (b == 0) { setzeMaus(aufFlaeche(20, 460)); }
                    return b >= 3;
                }});
    add(pruefe("rc608 Leiste: sonst der Hinweis", [] { return me::leisteStand().hilfe == tr(Str::MlHilfeStart); }));

    // --- Geaenderte Werte markieren, einzeln zuruecksetzen
    add(probe("knopf"));
    add(pause(0.25));
    add(pruefe("rc608 Leiste: textscale 0.7 weicht ab und traegt eine Marke", [] { return finde("##zurueck", "##inhalt", 0, false) != nullptr; }));
    add(foto("menue_leiste_marke"));
    add(merken());
    add(klick("##zurueck", "##inhalt", 0));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: Zuruecksetzen entfernt die Zeile, Datei byte-gleich mit dem Stand ohne sie", [=] {
        const std::string soll = ohneZeile(*vor, "textscale 0.7");
        const bool gut = me::stand().text == soll;
        if (!gut) { diag::info("Knopftest: Zuruecksetzen ergab " + me::stand().text.substr(0, 320)); }
        return gut;
    }));
    add(einSchritt("Zuruecksetzen"));
    zurueckAdd("Zuruecksetzen");
    add(tu("rc608 Leiste: das Element mit dem Wert der Vorgabe", [] { me::waehle(me::itemNummer("zweit"), false); }));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: textscale 0.55 ist die Vorgabe des Spiels: keine Marke", [] { return finde("##zurueck", "##inhalt", 0, false) == nullptr; }));
    add(tu("rc608 Leiste: CRLF", [] {
        std::string t = kLeisteText;
        std::string c;
        for (const char x : t) {
            if (x == '\n') { c += '\r'; }
            c += x;
        }
        (void)me::oeffneText("ui/leistecrlf.menu", c);
        me::waehle(me::itemNummer("knopf"), false);
    }));
    add(pause(0.3));
    add(merken());
    add(klick("##zurueck", "##inhalt", 0));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: auch mit CRLF faellt genau die Zeile weg", [=] {
        std::string soll = *vor;
        const std::size_t p = soll.find("    textscale 0.7\r\n");
        if (p != std::string::npos) { soll.erase(p, std::strlen("    textscale 0.7\r\n")); }
        return me::stand().text == soll;
    }));

    // --- Events
    add(probe("knopf"));
    add(tu("rc608 Leiste: Reiter Events", [] { me::setzeReiter(3); }));
    add(pause(0.3));
    add(foto("menue_leiste_events"));
    add(pruefe("rc608 Leiste: Auslöser Klick mit fuenf Schritten, der erste ist offen", [] {
        const auto l = me::leisteStand();
        return l.ausloeser == "action" && l.offenerSchritt == 0 && finde("##st4", "##inhalt", 0, false) != nullptr && finde("##st5", "##inhalt", 0, false) == nullptr;
    }));
    add(pruefe("rc608 Leiste: Hoch, Runter, Entfernen stehen an fester Stelle", [] {
        float x = -1.0F;
        for (int k = 0; k < 5; ++k) {
            const Element* e = finde("##st_x" + std::to_string(k), "##inhalt", 0, false);
            if (e == nullptr || (x >= 0.0F && std::fabs(e->rect.Min.x - x) > 0.5F)) { return false; }
            x = e->rect.Min.x;
        }
        return true;
    }));
    add(klick("##st2"));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: immer nur ein Schritt offen", [] { return me::leisteStand().offenerSchritt == 2 && finde(tr(Str::MlProbeSchritt), "##inhalt", 0, false) != nullptr; }));
    add(foto("menue_leiste_transition"));
    add(merken());
    add(klick(tr(Str::MlJetzigeLage), "##inhalt", 0));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: Use current nimmt die jetzige Lage", [] {
        std::string t = me::stand().text;
        t.erase(std::remove(t.begin(), t.end(), '"'), t.end());
        diag::info("Knopftest: " + t.substr(t.find("transition"), 60));
        return zeileDa(t, "transition knopf 100 100 200 40 6 6 108 38 10 1");
    }));
    add(einSchritt("Use current"));
    zurueckAdd("Use current");
    // Probieren: die Datei bleibt, kein Schritt, die Flaeche zeigt es
    add(merken());
    auto proben = std::make_shared<int>(0);
    add(tu("rc608 Leiste: Proben merken", [proben] { *proben = me::leisteStand().proben; }));
    add(klick(tr(Str::MlProbeSchritt), "##inhalt"));
    add(pause(0.3));
    add(pruefe("rc608 Leiste: Try this step laeuft im Interaktionsmodus", [proben] { return me::leisteStand().probeLaeuft && me::leisteStand().proben == *proben + 1 && me::stand().interaktion; }));
    add(foto("menue_leiste_probe"));
    add(pause(2.0));
    add(pruefe("rc608 Leiste: danach steht alles wieder, kein Schritt, die Datei bleibt", [=] {
        const auto x = me::stand();
        return !me::leisteStand().probeLaeuft && !x.interaktion && x.text == *vor && x.undoSchritte == *tiefe;
    }));
    add(tu("rc608 Leiste: Klaenge zaehlen", [] { me::klangListeLeeren(); }));
    add(klick(tr(Str::MlProbeAlle), "##inhalt"));
    add(pause(0.3));
    add(pruefe("rc608 Leiste: Try on canvas spielt alle Schritte, der Klang laeuft ueber den vorhandenen Weg", [] {
        const auto x = me::stand();
        const bool gut = std::find(x.klangListe.begin(), x.klangListe.end(), "sound/interface/button1.wav") != x.klangListe.end() && zeileDa(me::leisteStand().hilfe, "button1");
        diag::info("Knopftest: Hilfezeile beim Probieren: " + me::leisteStand().hilfe);
        return gut;
    }));
    add(pause(2.0));
    add(pruefe("rc608 Leiste: Try on canvas: danach unveraendert", [=] { return !me::stand().interaktion && me::stand().text == *vor && me::stand().undoSchritte == *tiefe; }));
    // Reihenfolge und Entfernen
    add(merken());
    add(klick("##st_r0"));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: Runter tauscht zwei Schritte", [] {
        std::string t = me::stand().text;
        t.erase(std::remove(t.begin(), t.end(), '"'), t.end());
        return t.find("setitemcolor knopf forecolor") < t.find("play sound");
    }));
    add(einSchritt("Runter"));
    zurueckAdd("Runter");
    add(merken());
    add(rolleRechtsZu("##st_x4"));   // es: die Schrittliste ist laenger als die Spalte
    add(klick("##st_x4"));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: Entfernen nimmt den Schritt", [] {
        std::string t = me::stand().text;
        t.erase(std::remove(t.begin(), t.end(), '"'), t.end());
        return zeileDa(t, "setcvar ui_name 1 }") && !zeileDa(t, "; show knopf");
    }));
    add(einSchritt("Entfernen"));
    zurueckAdd("Entfernen");
    // Add step
    add(merken());
    add(klick(tr(Str::MlAddStep)));
    add(pause(0.2));
    add(foto("menue_leiste_addstep"));
    add(pruefe("rc608 Leiste: Add step: am haeufigsten oben, im SP-Menue kein playlooped", [] { return finde("##neu_oftopen", "##add_liste", 0, false) != nullptr && finde("##neu_alleplaylooped", "##add_liste", 0, false) == nullptr; }));
    // Der Name in der Sprache des Laufs ("menu" stand nur im englischen Namen; es "Abrir menú", cs "Otevřít nabídku")
    add(tippe(tr(Str::MlSOeffnen)));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: die Suche findet Namen und Befehl", [] { return finde("##neu_alleopen", "##add_liste", 0, false) != nullptr && finde("##neu_alleplay", "##add_liste", 0, false) == nullptr; }));
    add(klick("##neu_alleopen"));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: der neue Schritt steht hinten und ist offen", [] {
        const auto l = me::leisteStand();
        return l.offenerSchritt == 5 && zeileDa(me::stand().text, "; open");
    }));
    add(einSchritt("Add step"));
    add(klick("##a0", "##inhalt"));
    add(pause(0.1));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(tippe("frei_geschrieben"));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.25));
    add(pruefe("rc608 Leiste: freies Schreiben bleibt", [] { return zeileDa(me::stand().text, "open frei_geschrieben") || zeileDa(me::stand().text, "open \"frei_geschrieben\""); }));
    add(klick(std::string(tr(Str::MlVorschlaege)) + "##a0"));
    add(pause(0.2));
    add(foto("menue_leiste_vorschlag"));
    add(tippe("leiste"));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: Die Liste nennt die Menues", [] { return finde("leiste", "##Popup_", 0, false) != nullptr; }));
    add(klick("leiste", "##Popup_"));
    add(pause(0.25));
    add(pruefe("rc608 Leiste: der Vorschlag ist eingetragen", [] { return zeileDa(me::stand().text, "open leiste") || zeileDa(me::stand().text, "open \"leiste\""); }));
    add(tu("rc608 Leiste: Rueckgaengig", [] { (void)me::rueckgaengig(); (void)me::rueckgaengig(); (void)me::rueckgaengig(); }));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: zurueck zum Stand vor Add step", [=] { return me::stand().text == *vor; }));
    // Hover: Effekt legt den Rueckweg an
    add(klick("##tr_mouseEnter"));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: bei Hover stehen Effekte und Klang", [] { return finde(tr(Str::MeEvGroesser), "##inhalt", 0, false) != nullptr && finde("##hoverklang", "##inhalt", 0, false) != nullptr && finde("##klickklang", "##inhalt", 0, false) == nullptr; }));
    add(merken());
    add(klick(tr(Str::MeEvGroesser)));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: Bigger legt den Rueckweg bei Hover ends an", [] {
        const std::string t = me::stand().text;
        return zeileDa(t, "mouseExit") && zeileDa(t.substr(t.find("mouseExit")), "transition knopf 96 96 208 48 100 100 200 40 10 1");
    }));
    add(einSchritt("Bigger"));
    zurueckAdd("Bigger");
    // Fehlendes Ziel: am Schritt, am Ausloeser und in der Pruefung
    add(klick("##tr_onFocus"));
    add(pause(0.3));
    add(foto("menue_leiste_ziel_fehlt"));
    add(pruefe("rc608 Leiste: Das fehlende Ziel steht in der Pruefung", [] {
        const auto x = me::stand();
        const bool gut = std::any_of(x.diagnosen.begin(), x.diagnosen.end(), [](const std::string& d) { return zeileDa(d, "fehlt"); });
        if (!gut) { for (const std::string& d : x.diagnosen) { diag::info("Knopftest: Diagnose " + d); } }
        return gut;
    }));
    // Umbenennen zieht die Schritte mit, in allen Elementen, und ein Schritt
    add(merken());
    add(klick("##name", "##karte"));
    add(pause(0.1));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(tippe("zurueck"));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.3));
    add(pruefe("rc608 Leiste: Umbenennen: Schritte, die auf das Element zeigen, ziehen mit", [] {
        const std::string t = me::stand().text;
        diag::info("Knopftest: nach Umbenennen " + t.substr(t.find("action")));
        return zeileDa(t, "name \"zurueck\"") && zeileDa(t, "setitemcolor zurueck forecolor") && zeileDa(t, "transition zurueck 10") && zeileDa(t, "show zurueck ; hide titel") && !zeileDa(t, "knopf");
    }));
    add(einSchritt("Umbenennen"));
    zurueckAdd("Umbenennen");
    // Menue selbst: keine Auswahl
    add(tu("rc608 Leiste: Auswahl leeren", [] { me::waehle(-1, false); }));
    add(pause(0.3));
    add(foto("menue_leiste_menue_events"));
    add(pruefe("rc608 Leiste: Keine Auswahl: die Ausloeser des Menues", [] {
        return me::leisteStand().ausloeser == "onOpen" && finde("##tr_onClose", "##inhalt", 0, false) != nullptr && finde("##tr_onAccept", "##inhalt", 0, false) != nullptr && finde("##tr_onESC", "##inhalt", 0, false) != nullptr && finde("##st0", "##inhalt", 0, false) != nullptr;
    }));
    add(klick("##tr_onESC"));
    add(pause(0.15));
    add(merken());
    add(klick(tr(Str::MlAddStep)));
    add(pause(0.15));
    add(klick("##neu_oftclose"));
    add(pause(0.25));
    add(pruefe("rc608 Leiste: Das Menue bekommt onESC vor seinen Elementen", [] {
        const std::string t = me::stand().text;
        const std::size_t e = t.find("onESC");
        diag::info("Knopftest: " + t.substr(0, 180));
        return e != std::string::npos && e < t.find("itemDef");
    }));
    add(einSchritt("Schritt im Menue"));
    zurueckAdd("Schritt im Menue");
    add(tu("rc608 Leiste: Reiter Properties", [] { me::setzeReiter(2); }));
    add(pause(0.3));
    add(foto("menue_leiste_menue"));
    add(pruefe("rc608 Leiste: Keine Auswahl: Name, Anzeige, Hintergrund des Menues", [] { return finde("##mname", "##inhalt", 0, false) != nullptr && finde("##mbild", "##inhalt", 0, false) != nullptr && finde(tr(Str::MeVollbild), "##inhalt", 0, false) != nullptr; }));

    // --- Zwei oder mehr gewaehlt
    add(tu("rc608 Leiste: zwei waehlen", [] {
        me::waehle(me::itemNummer("knopf"), false);
        me::waehle(me::itemNummer("titel"), true);
    }));
    add(pause(0.3));
    add(foto("menue_leiste_mehrere"));
    add(merken());
    add(pruefe("rc608 Leiste: Change all zeigt gemischte Werte, Anordnen ist da", [] { return finde("##gr_alle", "##inhalt", 0, false) != nullptr && finde("##gr_ordnen", "##inhalt", 0, false) != nullptr && me::leisteStand().gemischt > 0; }));
    add(klickAn("rc608 Leiste: Textgroesse fuer alle", [] { return feldUnterGruppe("alle", "##alle_scale"); }));
    add(pause(0.1));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(tippe("0.8"));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.25));
    add(pruefe("rc608 Leiste: beide haben den neuen Wert", [] {
        const std::string t = me::stand().text;
        const std::size_t a = t.find("textscale 0.8");
        return a != std::string::npos && t.find("textscale 0.8", a + 5) != std::string::npos;
    }));
    add(einSchritt("Change all"));
    zurueckAdd("Change all");
    add(merken());
    add(klick(tr(Str::MeLinks), "##inhalt"));
    add(pause(0.25));
    add(einSchritt("Anordnen"));
    zurueckAdd("Anordnen");

    // --- Fuss: Component, Preview cvars, Engine messages; die Funktionen bleiben
    add(probe("titel"));
    add(pause(0.3));
    add(klick(tr(Str::MeKomp), "##rechts"));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: Component liegt im Fuss, das Feld fuer den Namen ist da", [] { return me::leisteStand().tablett == 1 && finde("##kompname", "##tablett", 0, false) != nullptr; }));
    add(foto("menue_leiste_komponente"));
    add(klick(tr(Str::MeKompAnlegen)));
    add(pause(0.3));
    add(klick(tr(Str::MeKompKopie)));
    add(pause(0.3));
    add(pruefe("rc608 Leiste: Verknuepfte Kopie ist erreichbar", [] { return zeileDa(me::stand().text, "jkstudio:instanz"); }));
    add(tu("rc608 Leiste: die Kopie waehlen", [] { me::waehle(me::itemNummer("titel_2"), false); }));
    add(pause(0.3));
    add(pruefe("rc608 Leiste: Original waehlen ist erreichbar", [] { return finde(tr(Str::MeKompZumOriginal), "##tablett", 0, false) != nullptr; }));
    add(klick(tr(Str::MeKompZumOriginal)));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: Original waehlen springt", [] { return me::itemNameBei(me::stand().auswahl.empty() ? -1 : me::stand().auswahl[0]) == "titel"; }));
    add(klick(tr(Str::MlVorschauCvars)));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: Preview cvars im Fuss", [] { return me::leisteStand().tablett == 2 && finde(tr(Str::MeCvarsZuruecksetzen), "##tablett", 0, false) != nullptr; }));
    add(foto("menue_leiste_cvars"));
    add(klick(std::string(tr(Str::MeMeldungen)) + ": 0"));
    add(pause(0.2));
    add(pruefe("rc608 Leiste: Engine messages im Fuss", [] { return me::leisteStand().tablett == 3 && ImGui::GetFrameCount() - me::meldungenLeerBild() <= 2; }));
    add(klick(std::string(tr(Str::MeMeldungen)) + ": 0"));
    add(tu("rc608 Leiste: aufraeumen", [] {
        me::schliesseAlleOhneSpeichern();
        me::setzeSpalten(0.0F, 0.0F);
        me::setzeReiter(2);
    }));
    return s;
}

// Jeder Elementtyp, dazu Bild und Video: bei 300, 500, 1000 Punkten, 100 % und 200 %, 6 Sprachen liegt alles im Feld
std::vector<Schritt> schritteLeisteBreit() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    static std::string text;
    text = "menuDef\n{\n  name \"breit\"\n  rect 0 0 640 480\n  visible 1\n";
    std::vector<int> nummern;
    for (int typ = 0; typ < 15; ++typ) {
        text += "  itemDef\n  {\n    name \"e" + std::to_string(typ) + "\"\n    type " + std::string(bhed::menue::typName(typ)) + "\n    rect 10 10 100 30\n    text \"Ein ziemlich langer Text, der zeigt, wie die Zusammenfassung abschneidet\"\n    textscale 0.8\n    border 1\n    style WINDOW_STYLE_FILLED\n    background \"gfx/menus/menu_buttonback\"\n    cvar \"ui_lang\"\n    cvarTest \"ui_tab\"\n    showCvar { \"1\" }\n    group \"gruppe\"\n    decoration\n    visible 1\n"
                "    action { play \"sound/interface/button1.wav\" ; setitemcolor e0 forecolor 1 0.5 0.25 1 ; transition e0 10 10 100 30 6 6 108 38 10 1 ; setcvar ui_name 1 ; open breit }\n    mouseEnter { setcolor backcolor 1 1 1 1 }\n  }\n";
        nummern.push_back(typ);
    }
    text += "  itemDef\n  {\n    name \"bild\"\n    style WINDOW_STYLE_SHADER\n    rect 10 10 200 100\n    visible 1\n  }\n";
    text += "  itemDef\n  {\n    name \"video\"\n    style WINDOW_STYLE_SHADER\n    rect 10 10 200 100\n    // jkstudio:rahmen video\n    visible 1\n  }\n";
    text += "}\n";
    nummern.push_back(15);
    nummern.push_back(16);
    add(tu("rc608 Leiste breit: Probe, alle Gruppen offen", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneText("ui/breit.menu", text);
        me::setzeInteraktion(false);
        me::leisteAlleOffen();
        g_app->settings.menueDateiSchluessel = true;
    }));
    add(pause(0.5));
    const auto farbeAuf = [](std::string& f) {
        (void)f;
        return true;
    };
    for (auto& x : breitenTabelle("Leiste Properties", 2, nummern, farbeAuf)) { add(std::move(x)); }
    for (auto& x : breitenTabelle("Leiste Events", 3, {0, 7}, farbeAuf)) { add(std::move(x)); }
    add(tu("rc608 Leiste breit: aufraeumen", [] {
        g_app->settings.menueDateiSchluessel = false;
        me::setzeSpalten(0.0F, 0.0F);
        me::schliesseAlleOhneSpeichern();
        me::setzeReiter(2);
    }));
    return s;
}

// rc608 Teil 3a: Zeigt der Modus etwas, was das Spiel nie zeigt, ist das ein Fehler. Treffer und Griffe bei mehreren Stufen
std::vector<Schritt> schritteVerzerrt() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("rc608 Flaeche: Probe", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneText("zug.menu", kZugText);
        me::setzeInteraktion(false);
        me::setzeAufloesung(0);
        me::setzeBalken(false);
    }));
    add(pause(0.4));
    for (const float rechts : {0.0F, 500.0F, 1000.0F, 100000.0F}) {
        const std::string was = "rechts " + std::to_string(static_cast<int>(rechts));
        add(tu("rc608 Flaeche: " + was, [rechts] { me::setzeSpalten(0.0F, rechts); }));
        add(pause(0.15));
        add(pruefe("rc608 Flaeche: " + was + ": ohne Spielfenster 4:3, nie hochkant", [was] {
            const auto x = me::stand();
            diag::info("Knopftest: Flaeche " + was + ": " + std::to_string(x.flaeche[2]) + " x " + std::to_string(x.flaeche[3]));
            return std::fabs(x.flaeche[2] / x.flaeche[3] - 4.0F / 3.0F) < 0.01F;
        }));
    }
    add(tu("rc608 Flaeche: 1920x1080", [] {
        me::setzeSpalten(0.0F, 0.0F);
        me::setzeAufloesung(7);
    }));
    add(pause(0.2));
    add(pruefe("rc608 Flaeche: 1920x1080: das Seitenverhaeltnis des Spielfensters", [] {
        const auto x = me::stand();
        return std::fabs(x.flaeche[2] / x.flaeche[3] - 16.0F / 9.0F) < 0.01F;
    }));
    add(tu("rc608 Flaeche: 4:3", [] { me::setzeAufloesung(0); }));
    add(pause(0.2));
    add(foto("menue_flaeche_43"));
    // Treffer und Griffe je Zoomstufe
    for (const int stufe : {0, 2, 4}) {
        const std::string st = "Stufe " + std::to_string(stufe);
        add(tu("rc608 Flaeche: " + st, [] { me::waehle(me::itemNummer("frei"), false); }));
        add(pause(0.1));
        if (stufe > 0) {
            add(Schritt{"rc608 Flaeche: heran " + st, [stufe](int b) {
                            ImGuiIO& io = ImGui::GetIO();
                            if (b == 0) { setzeMaus(aufFlaeche(450, 325)); return false; }
                            if (b == 2) { io.AddKeyEvent(ImGuiMod_Ctrl, true); return false; }
                            if (b == 3) { io.AddMouseWheelEvent(0.0F, static_cast<float>(stufe)); return false; }
                            if (b == 5) { io.AddKeyEvent(ImGuiMod_Ctrl, false); return false; }
                            return b >= 8;
                        }});
        }
        add(zeigerAn("4:3 " + st + " Griff rechts unten", [] { return aufFlaeche(500, 350); }, ImGuiMouseCursor_ResizeNWSE));
        add(zeigerAn("4:3 " + st + " auf dem Element", [] { return aufFlaeche(450, 325); }, ImGuiMouseCursor_ResizeAll));
        auto vor = std::make_shared<std::string>();
        add(tu("rc608 Flaeche: merken", [vor] { *vor = me::stand().text; }));
        add(ziehFlaeche("rc608 Flaeche: Griff ziehen " + st, 500, 350, 0, 0, 40, 30));
        add(pruefe("rc608 Flaeche: der Griff zieht " + st, [vor] { return me::stand().text != *vor && rectVon("frei").rfind("400 300 ", 0) == 0; }));
        add(tu("rc608 Flaeche: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
        add(pause(0.1));
    }
    add(tu("rc608 Flaeche: aufraeumen", [] {
        me::setzeAufloesung(0);
        me::schliesseAlleOhneSpeichern();
    }));
    return s;
}

// rc608 Teil 3a (shank: "kann man den Text editor scalen und raus zu zoomen und naeher ran zu gehen?")
std::vector<Schritt> schritteTextZoom() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto rad = [](const std::string& was, std::function<ImVec2()> ort, float raster) {
        return Schritt{was, [=](int b) {
                           ImGuiIO& io = ImGui::GetIO();
                           if (b == 0) { setzeMaus(ort()); return false; }
                           if (b == 2) { io.AddKeyEvent(ImGuiMod_Ctrl, true); return false; }
                           if (b == 3) { io.AddMouseWheelEvent(0.0F, raster); return false; }
                           if (b == 5) { io.AddKeyEvent(ImGuiMod_Ctrl, false); return false; }
                           return b >= 8;
                       }};
    };
    const auto ueberText = [] { return fensterMitte("/##code_"); };
    const auto ueberFlaeche = [] { return aufFlaeche(320, 240); };
    auto hoehe100 = std::make_shared<float>(0.0F);
    auto vor = std::make_shared<std::string>();
    auto tiefe = std::make_shared<int>(0);
    add(tu("rc608 Textzoom: Probe", [=] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        g_app->settings.menueTextZoom = 100;
        g_app->settings.menueUmbruch = false;
        (void)me::oeffneText("zug.menu", kZugText);
        me::setzeInteraktion(false);
        me::setzeReiter(1);
    }));
    add(pause(0.4));
    add(tu("rc608 Textzoom: merken", [=] {
        *hoehe100 = me::textAnsicht().zeilenHoehe;
        *vor = me::stand().text;
        *tiefe = me::stand().undoSchritte;
    }));
    auto flaecheVor = std::make_shared<int>(0);
    add(tu("rc608 Textzoom: Zoom der Flaeche merken", [flaecheVor] { *flaecheVor = me::stand().zoomProzent; }));
    add(rad("rc608 Textzoom: Strg+Rad ueber dem Text", ueberText, 2.0F));
    add(pruefe("rc608 Textzoom: Strg+Rad aendert die Schrift des Text-Editors, die Flaeche bleibt", [=] {
        const auto t = me::textAnsicht();
        diag::info("Knopftest: Textzoom " + std::to_string(t.zoom) + ", Zeile " + std::to_string(t.zeilenHoehe) + " statt " + std::to_string(*hoehe100));
        return t.zoom == 110 && t.zeilenHoehe > *hoehe100 && me::stand().zoomProzent == *flaecheVor;
    }));
    add(foto("menue_textzoom_110"));
    add(rad("rc608 Textzoom: Strg+Rad ueber der Flaeche", ueberFlaeche, 2.0F));
    add(pruefe("rc608 Textzoom: ueber der Flaeche zoomt die Flaeche, der Text bleibt", [=] { return me::textAnsicht().zoom == 110 && me::stand().zoomProzent != *flaecheVor; }));
    add(Schritt{"rc608 Textzoom: Zeiger ueber die Flaeche", [=](int b) {
                    if (b == 0) { setzeMaus(ueberFlaeche()); }
                    return b >= 3;
                }});
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_0, "Strg+0"));
    add(pause(0.15));
    add(pruefe("rc608 Textzoom: Strg+0 ueber der Flaeche passt sie ein, der Text bleibt", [=] { return me::textAnsicht().zoom == 110 && me::stand().zoomProzent == *flaecheVor; }));
    add(Schritt{"rc608 Textzoom: Zeiger ueber den Text", [=](int b) {
                    if (b == 0) { setzeMaus(ueberText()); }
                    return b >= 3;
                }});
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_0, "Strg+0"));
    add(pause(0.15));
    add(pruefe("rc608 Textzoom: Strg+0 ueber dem Text: Vorgabe", [=] { return me::textAnsicht().zoom == 100 && std::fabs(me::textAnsicht().zeilenHoehe - *hoehe100) < 0.5F; }));
    for (int i = 0; i < 30; ++i) { add(rad("rc608 Textzoom: kleiner", ueberText, -1.0F)); }
    add(pruefe("rc608 Textzoom: unten bei 50 %", [] { return me::textAnsicht().zoom == 50; }));
    add(foto("menue_textzoom_50"));
    // Treffer, Cursor, Umbruch, Rollbalken je Stufe, 100 % und 200 % Oberflaeche
    auto skala = std::make_shared<float>(1.0F);
    add(tu("rc608 Textzoom: Oberflaeche merken", [skala] { *skala = g_app->uiScale; }));
    for (const float ober : {1.0F, 2.0F}) {
        for (const int stufe : {50, 100, 170, 300}) {
            const std::string st = std::to_string(stufe) + " % bei Oberflaeche " + std::to_string(static_cast<int>(ober * 100.0F));
            add(tu("rc608 Textzoom: " + st, [=] {
                g_app->uiScale = ober;
                me::sucheZu();
                g_app->settings.menueTextZoom = stufe;
                g_app->settings.menueUmbruch = false;
                me::setzeTextScroll(0.0F);
            }));
            add(pause(0.25));
            // Klick in Zeile 3 (name "zug" ...) auf das dritte Zeichen, tippen: Der Buchstabe steht genau dort
            const bool riesig = static_cast<float>(stufe) * ober > 350.0F;   // bei 600 % steht Zeile 3 nicht im Bild, rechts vom Rand bleiben gut zwei Zeichen
            const float reihe = riesig ? 0.5F : 2.5F;
            const float spalte = riesig ? 1.0F : 2.5F;
            const std::size_t soll = riesig ? 1 : 2;
            add(klickAn("rc608 Textzoom: Klick in eine Zeile " + st, [reihe, spalte] {
                const auto t = me::textAnsicht();
                return ImVec2{t.textX + t.leer * spalte, t.textY + t.zeilenHoehe * reihe};
            }));
            add(pause(0.1));
            add(tu("rc608 Textzoom: Zeichen tippen", [] { ImGui::GetIO().AddInputCharactersUTF8("#"); }));
            add(pause(0.15));
            add(pruefe("rc608 Textzoom: Treffer und Cursor stimmen " + st, [st, soll] {
                const std::string z = me::stand().cursorZeile;
                const auto tv = me::textAnsicht();
                diag::info("Knopftest: " + st + ": Zeile [" + z + "], textX " + std::to_string(tv.textX) + ", textY " + std::to_string(tv.textY) + ", leer " + std::to_string(tv.leer) + ", Zeilenhoehe " + std::to_string(tv.zeilenHoehe) + ", Feld " + std::to_string(tv.flaeche));
                return z.size() > 3 && z[soll] == '#';
            }));
            add(tu("rc608 Textzoom: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
            add(pause(0.1));
            add(pruefe("rc608 Textzoom: Treffer im Rand, Balken und Zeilenbreite " + st, [st] {
                const auto t = me::textAnsicht();
                diag::info("Knopftest: " + st + ": breiteste " + std::to_string(t.breiteste) + ", Feld " + std::to_string(t.flaeche) + ", Balken " + std::to_string(t.balken[2]));
                // ohne Umbruch: Die breiteste Zeile waechst mit der Schrift, und der Balken erscheint, sobald sie nicht passt
                return t.breiteste <= t.flaeche || t.balken[2] > 0.0F;
            }));
            add(tu("rc608 Textzoom: Umbruch an", [] { g_app->settings.menueUmbruch = true; }));
            add(pause(0.2));
            add(pruefe("rc608 Textzoom: mit Umbruch nichts breiter als das Feld " + st, [] {
                const auto t = me::textAnsicht();
                return t.breiteste <= t.flaeche + t.leer;
            }));
            add(tu("rc608 Textzoom: nach oben", [] { me::setzeTextScroll(0.0F); }));
            add(pause(0.1));
            add(klickAn("rc608 Textzoom: Klick mit Umbruch " + st, [] {
                const auto t = me::textAnsicht();
                return ImVec2{t.textX + t.leer * 2.5F, t.textY + t.zeilenHoehe * 0.5F};
            }));
            add(pause(0.15));
            add(pruefe("rc608 Textzoom: mit Umbruch trifft der Klick die erste Zeile " + st, [] { return me::stand().cursorZeile.rfind("menuDef", 0) == 0; }));
        }
    }
    add(tu("rc608 Textzoom: Oberflaeche zurueck", [skala] {
        g_app->uiScale = *skala;
        g_app->settings.menueTextZoom = 100;
    }));
    // Suchen: Treffer ueber dieselbe Reihenrechnung; nur Ansicht: kein Undo-Schritt, die Datei bleibt
    add(pause(0.2));
    add(tu("rc608 Textzoom: 150", [] { g_app->settings.menueTextZoom = 150; }));
    add(pause(0.2));
    add(pruefe("rc608 Textzoom: Suchen findet bei jeder Groesse", [] { return me::suche("WINDOW_STYLE_FILLED", false) == 3; }));
    add(foto("menue_textzoom_suche"));
    add(pruefe("rc608 Textzoom: kein Undo-Schritt, die Datei bleibt", [=] { return me::stand().text == *vor; }));
    add(tu("rc608 Textzoom: aufraeumen", [] {
        g_app->settings.menueTextZoom = 100;
        g_app->settings.menueUmbruch = true;
        me::schliesseAlleOhneSpeichern();
        me::setzeReiter(2);
    }));
    return s;
}

// rc608 Teil 3a (shank: "bei add image oeffnet sich gleich immer der image folder ... gilt das nicht auch fuer Videos?"):
// erst der Rahmen, dann die Auswahl; beides ein Schritt, beide byte-gleich zurueck, auch die Paketeintraege
std::vector<Schritt> schritteRahmen() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto vor = std::make_shared<std::string>();
    auto tiefe = std::make_shared<int>(0);
    auto anzahl = std::make_shared<int>(0);
    auto paket = std::make_shared<std::vector<std::string>>();
    auto dialoge = std::make_shared<int>(0);
    const std::string bildPfad = (std::filesystem::path(platform::settingsDirectory()) / "probebild.tga").string();
    const auto merke = [=] {
        return tu("rc608 Rahmen: merken", [=] {
            const auto x = me::stand();
            *vor = x.text;
            *tiefe = x.undoSchritte;
            *anzahl = x.items;
            *paket = x.paket;
        });
    };
    const auto doppelKachel = [](Str name) {
        return klickAn("rc608 Rahmen: Doppelklick auf " + std::string(tr(name)), [name] {
            const Element* e = finde(tr(name), "##links", 0, false);
            return e != nullptr ? e->rect.GetCenter() : ImVec2{};
        }, 0, true);
    };
    const auto rueckUndVor = [&](const std::string& was) {
        auto nachher = std::make_shared<std::string>();
        auto paketNach = std::make_shared<std::vector<std::string>>();
        add(tu("rc608 Rahmen: " + was + ": Ergebnis merken, Rueckgaengig", [=] {
            *nachher = me::stand().text;
            *paketNach = me::stand().paket;
            (void)me::rueckgaengig();
        }));
        add(pause(0.2));
        add(pruefe("rc608 Rahmen: " + was + ": Rueckgaengig byte-gleich, das Paket auch", [=] { return me::stand().text == *vor && me::stand().paket == *paket; }));
        add(tu("rc608 Rahmen: " + was + ": Wiederholen", [] { (void)me::wiederholen(); }));
        add(pause(0.2));
        add(pruefe("rc608 Rahmen: " + was + ": Wiederholen byte-gleich, das Paket auch", [=] { return me::stand().text == *nachher && me::stand().paket == *paketNach; }));
    };
    for (const bool mp : {false, true}) {
        const std::string d = mp ? "MP " : "SP ";
        add(tu("rc608 Rahmen: " + d + "leeres Menue, Dialoge zaehlen", [mp, dialoge, bildPfad] {
            me::schliesseAlleOhneSpeichern();
            g_app->leftMode = 4;
            tourSchliessen();
            (void)me::oeffneText(mp ? "ui/jamp/rahmen.menu" : "ui/rahmen.menu", "menuDef\n{\n  name \"rahmen\"\n  rect 0 0 640 480\n  visible 1\n  itemDef\n  {\n    name \"davor\"\n    rect 10 10 50 50\n    style 1\n    backcolor 1 0 0 1\n    visible 1\n  }\n}\n");
            me::setzeInteraktion(false);
            me::setzeReiter(2);
            me::setzeSpalten(0.0F, 0.0F);
            me::leisteVorgabe();
            *dialoge = 0;
            std::ofstream f(bildPfad, std::ios::binary);
            const unsigned char tga[] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 24, 0, 255, 255, 255};
            f.write(reinterpret_cast<const char*>(tga), sizeof(tga));
            f.close();
            me::setzeBildDialog([dialoge, bildPfad] {
                ++*dialoge;
                return bildPfad;
            });
        }));
        add(pause(0.4));
        // Ein einfacher Klick tut nichts, ein Doppelklick setzt den Rahmen ohne Dateidialog
        add(merke());
        add(klick(tr(Str::MlTypBild), "##links"));
        add(pause(0.2));
        add(pruefe("rc608 Rahmen: " + d + "ein Klick tut nichts", [=] { return me::stand().items == *anzahl; }));
        add(doppelKachel(Str::MlTypBild));
        add(pause(0.3));
        add(pruefe("rc608 Rahmen: " + d + "Bild: ein Rahmen, gewaehlt, ein Schritt, kein Dialog, ohne Bild und ohne Platzhalter in der Datei", [=] {
            const auto x = me::stand();
            const std::string neu = x.text.substr(x.text.rfind("itemDef"));
            diag::info("Knopftest: Rahmen: " + neu);
            // Der Name bleibt, wenn er im Menue frei ist (freierName, seit 433cd7dc): "image", nicht "image_2"
            const std::vector<me::ItemLage> l = me::itemLagen();
            const bool name = x.auswahl.size() == 1 && static_cast<std::size_t>(x.auswahl[0]) < l.size() && l[static_cast<std::size_t>(x.auswahl[0])].name == "image";
            return name && x.items == *anzahl + 1 && x.undoSchritte == *tiefe + 1 && *dialoge == 0 && !me::bildWahlOffen() && zeileDa(neu, "WINDOW_STYLE_SHADER") && !zeileDa(neu, "background") && !zeileDa(neu, "menu_box1");
        }));
        add(foto("menue_rahmen_bild_" + std::string(mp ? "mp" : "sp")));
        rueckUndVor(d + "Bild einfuegen");
        // Die Pruefung nennt ihn, der Export warnt
        add(pruefe("rc608 Rahmen: " + d + "Die Pruefung nennt den Rahmen ohne Bild", [] {
            const auto x = me::stand();
            const bool gut = std::any_of(x.diagnosen.begin(), x.diagnosen.end(), [](const std::string& z) { return zeileDa(z, "image") || zeileDa(z, "Bild") || zeileDa(z, "図") || zeileDa(z, "图"); });
            if (!gut) { for (const std::string& z : x.diagnosen) { diag::info("Knopftest: Diagnose " + z); } }
            return me::pruefeFuerSpiel(true, false, true) >= 1;
        }));
        // Doppelklick auf den Rahmen oeffnet die Auswahl und zieht nie; Abbrechen laesst alles stehen
        auto lage = std::make_shared<std::string>();
        add(merke());
        add(tu("rc608 Rahmen: Lage merken", [lage] { *lage = lageVon("image"); }));
        add(klickAn("rc608 Rahmen: " + d + "Doppelklick auf den Rahmen", [] {
            for (const me::ItemLage& l : me::itemLagen()) {
                if (l.name == "image") { return aufFlaeche(l.x + l.b * 0.5F, l.y + l.h * 0.5F); }
            }
            return ImVec2{};
        }, 0, true));
        add(pause(0.3));
        add(pruefe("rc608 Rahmen: " + d + "der Doppelklick oeffnet die Auswahl und zieht nicht", [=] {
            diag::info("Knopftest: Meldungen " + std::to_string(me::stand().meldungen) + ", Auswahl offen " + std::to_string(me::bildWahlOffen() ? 1 : 0) + ", Lage " + *lage + " -> " +
                       lageVon("image") + ", Schritte " + std::to_string(me::stand().undoSchritte) + " (vorher " + std::to_string(*tiefe) + ")");
            return me::bildWahlOffen() && !lage->empty() && lageVon("image") == *lage && me::stand().undoSchritte == *tiefe;
        }));
        add(foto("menue_rahmen_wahl_" + std::string(mp ? "mp" : "sp")));
        add(klick(tr(Str::EditorCancel), "###bildwahl"));
        add(pause(0.2));
        add(pruefe("rc608 Rahmen: " + d + "Abbrechen laesst den Rahmen stehen, kein Schritt", [=] { return !me::bildWahlOffen() && me::stand().text == *vor && me::stand().undoSchritte == *tiefe; }));
        // Der Knopf in Properties: aus den Spieldaten waehlen
        add(klick(tr(Str::MlWaehlen), "##inhalt"));
        add(pause(0.4));
        add(klick("##bw_suche"));
        add(tippe("menu_"));
        add(pause(0.3));
        auto gewaehlt = std::make_shared<std::string>();
        add(Schritt{"rc608 Rahmen: " + d + "Das erste Bild der Liste anklicken", [gewaehlt](int b) {
                        ImGuiIO& io = ImGui::GetIO();
                        if (b == 0) {
                            const Element* e = finde("##bw_", "##bw_liste", 2, true);
                            if (e == nullptr) { return true; }
                            *gewaehlt = e->label.substr(5);
                            setzeMaus(e->rect.GetCenter());
                            return false;
                        }
                        if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
                        if (b == 3) { io.AddMouseButtonEvent(0, false); return false; }
                        return b >= 6;
                    }});
        add(pruefe("rc608 Rahmen: " + d + "Das Bild steht im Rahmen, ein Schritt, nur eine Zeile dazu", [=] {
            const auto x = me::stand();
            diag::info("Knopftest: gewaehlt " + *gewaehlt);
            return !gewaehlt->empty() && !me::bildWahlOffen() && x.undoSchritte == *tiefe + 1 && zeileDa(x.text, "background") && zeileDa(x.text, *gewaehlt) && lageVon("image") == *lage;
        }));
        rueckUndVor(d + "Bild waehlen");
        add(tu("rc608 Rahmen: wieder ohne", [] { (void)me::rueckgaengig(); }));
        add(pause(0.2));
        // Von Datei: ins Paket wie bisher, zusammen ein Schritt
        add(merke());
        add(klick(tr(Str::MlWaehlen), "##inhalt"));
        add(pause(0.3));
        add(klick(tr(Str::MlVonDatei), "###bildwahl"));
        add(pause(0.4));
        add(pruefe("rc608 Rahmen: " + d + "Von Datei: ins Paket, in den Rahmen, ein Schritt", [=] {
            const auto x = me::stand();
            return *dialoge == 1 && x.undoSchritte == *tiefe + 1 && zeileDa(x.text, "gfx/menus/probebild") && std::find(x.paket.begin(), x.paket.end(), "gfx/menus/probebild.tga") != x.paket.end();
        }));
        rueckUndVor(d + "Von Datei");
        // Vollbild ganz hinten
        add(merke());
        add(klick(tr(Str::MlVollbildHinten), "##inhalt"));
        add(pause(0.3));
        add(pruefe("rc608 Rahmen: " + d + "Ganzer Schirm, ganz hinten, ein Schritt", [=] {
            const auto x = me::stand();
            const std::vector<me::ItemLage> l = me::itemLagen();
            return x.undoSchritte == *tiefe + 1 && !l.empty() && l[0].name == "image" && l[0].b == 640.0F && l[0].h == 480.0F && x.auswahl == std::vector<int>{0};
        }));
        rueckUndVor(d + "Ganzer Schirm");
        // Video: ein Rahmen mit Kennung, die Auswahl nennt die Videos des Spiels, nur der Shader kommt ins Paket
        add(merke());
        add(doppelKachel(Str::MlTypVideo));
        add(pause(0.3));
        add(pruefe("rc608 Rahmen: " + d + "Video: ein Rahmen, ein Schritt, kein Dialog", [=] {
            const auto x = me::stand();
            return x.items == *anzahl + 1 && x.undoSchritte == *tiefe + 1 && *dialoge == 1 && zeileDa(x.text, "jkstudio:rahmen video") && x.auswahl.size() == 1;
        }));
        add(foto("menue_rahmen_video_" + std::string(mp ? "mp" : "sp")));
        add(tu("rc608 Rahmen: Video merken", [=] {
            *vor = me::stand().text;
            *tiefe = me::stand().undoSchritte;
            *paket = me::stand().paket;
        }));
        add(klick(tr(Str::MlWaehlen), "##inhalt"));
        add(pause(0.4));
        add(foto("menue_rahmen_videowahl_" + std::string(mp ? "mp" : "sp")));
        auto video = std::make_shared<std::string>();
        add(Schritt{"rc608 Rahmen: " + d + "Das erste Video anklicken", [video](int b) {
                        ImGuiIO& io = ImGui::GetIO();
                        if (b == 0) {
                            const Element* e = finde("##bw_", "##bw_liste", 0, true);
                            if (e == nullptr) { return true; }
                            *video = e->label.substr(5);
                            setzeMaus(e->rect.GetCenter());
                            return false;
                        }
                        if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
                        if (b == 3) { io.AddMouseButtonEvent(0, false); return false; }
                        return b >= 6;
                    }});
        add(pruefe("rc608 Rahmen: " + d + "Video gewaehlt: Shader mit videoMap im Paket, das Video bleibt im Spiel, Kennung weg, ein Schritt", [=] {
            const auto x = me::stand();
            diag::info("Knopftest: Video " + *video + ", Paket " + std::to_string(x.paket.size()) + ", Text " + x.text.substr(x.text.rfind("itemDef")));
            if (video->empty()) { return false; }
            const std::string shader = me::paketInhalt("shaders/mymenu.shader");
            return x.undoSchritte == *tiefe + 1 && zeileDa(x.text, "menu/video/" + *video) && !zeileDa(x.text, "jkstudio:rahmen") && zeileDa(shader, "videoMap video/" + *video + ".roq") &&
                   std::find(x.paket.begin(), x.paket.end(), "video/" + *video + ".roq") == x.paket.end();
        }));
        add(foto("menue_rahmen_video_laeuft_" + std::string(mp ? "mp" : "sp")));
        rueckUndVor(d + "Video waehlen");
    }
    add(tu("rc608 Rahmen: aufraeumen", [bildPfad] {
        me::setzeBildDialog({});
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(bildPfad), ec);
        me::schliesseAlleOhneSpeichern();
    }));
    return s;
}

// rc608 Teil 3a: die linke Leiste
std::vector<Schritt> schritteLinks() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto probe = [] {
        return tu("rc608 Links: Probe", [] {
            me::schliesseAlleOhneSpeichern();
            g_app->leftMode = 4;
            tourSchliessen();
            (void)me::oeffneText("ui/links.menu", kLeisteText);
            me::setzeInteraktion(false);
            me::setzeSpalten(0.0F, 0.0F);
            me::leisteVorgabe();
            g_app->settings.menueTeileHinweisWeg = false;
        });
    };
    add(probe());
    add(pause(0.5));
    add(foto("menue_links"));
    add(rolleTeileAnsEnde());
    add(pruefe("rc608 Links: Gruppen, Suche, Hierarchie, Hinweis", [] {
        const auto x = me::kachelLagen();
        std::string namen;
        int knopf = 0;
        for (const me::KachelLage& k : x) {
            namen += k.name + ", ";
            knopf += k.name == tr(Str::MlTypButton) ? 1 : 0;
        }
        diag::info("Knopftest: Kacheln " + namen);
        return finde("##gr_l_fertig", "##links", 0, false) != nullptr &&
               finde("##gr_l_vorlagen", "##links", 0, false) != nullptr && finde("##gr_l_hier", "##links", 0, false) != nullptr && finde("##teile_suche", "##links", 0, false) != nullptr && knopf == 1 &&
               x.size() >= 15U;
    }));
    // Suche
    add(klick("##teile_suche"));
    add(tippe(tr(Str::MlBtLautstaerke)));
    add(pause(0.2));
    add(pruefe("rc608 Links: Die Suche zeigt das passende Teil", [] {
        const auto x = me::kachelLagen();
        return x.size() == 1U && x[0].name == tr(Str::MlBtLautstaerke);
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(taste(ImGuiKey_Delete, "Entf"));
    add(pause(0.2));
    // Der Knopf hat eine sichtbare Flaeche; der Hinweis verschwindet
    add(klickAn("rc608 Links: Doppelklick auf den Knopf", [] {
        const Element* e = finde(tr(Str::MlTypButton), "##links", 0, false);
        return e != nullptr ? e->rect.GetCenter() : ImVec2{};
    }, 0, true));
    add(pause(0.3));
    add(foto("menue_links_knopf"));
    add(pruefe("rc608 Links: Der Knopf hat eine Flaeche, die Mitte senkrecht stimmt; der Hinweis ist weg", [] {
        const auto x = me::stand();
        const std::string neu = x.text.substr(x.text.rfind("itemDef"));
        diag::info("Knopftest: Knopf " + neu);
        return zeileDa(neu, "WINDOW_STYLE_FILLED") && zeileDa(neu, "backcolor") && zeileDa(neu, "textaligny") && g_app->settings.menueTeileHinweisWeg;
    }));
    // Hierarchie: Strg, Umschalt, Suche, zuklappen
    add(probe());
    add(pause(0.4));
    add(klickAn("rc608 Links: Klick auf titel", [] { return zeileVon("titel"); }));
    add(pause(0.1));
    add(Schritt{"rc608 Links: Strg+Klick auf zweit", [](int b) {
                    ImGuiIO& io = ImGui::GetIO();
                    if (b == 0) { setzeMaus(zeileVon("zweit")); return false; }
                    if (b == 2) { io.AddKeyEvent(ImGuiMod_Ctrl, true); return false; }
                    if (b == 3) { io.AddMouseButtonEvent(0, true); return false; }
                    if (b == 4) { io.AddMouseButtonEvent(0, false); return false; }
                    if (b == 5) { io.AddKeyEvent(ImGuiMod_Ctrl, false); return false; }
                    return b >= 8;
                }});
    add(pruefe("rc608 Links: Strg nimmt hinzu", [] { return me::stand().auswahl.size() == 2U; }));
    add(pause(0.5));   // sonst waere der naechste Klick auf dieselbe Zeile ein Doppelklick (umbenennen)
    add(klickAn("rc608 Links: Klick auf zweit", [] { return zeileVon("zweit"); }));
    add(pause(0.5));
    add(Schritt{"rc608 Links: Umschalt+Klick auf knopf", [](int b) {
                    ImGuiIO& io = ImGui::GetIO();
                    if (b == 0) { setzeMaus(zeileVon("knopf")); return false; }
                    if (b == 2) { io.AddKeyEvent(ImGuiMod_Shift, true); return false; }
                    if (b == 3) { io.AddMouseButtonEvent(0, true); return false; }
                    if (b == 4) { io.AddMouseButtonEvent(0, false); return false; }
                    if (b == 5) { io.AddKeyEvent(ImGuiMod_Shift, false); return false; }
                    return b >= 8;
                }});
    add(pruefe("rc608 Links: Umschalt waehlt den Bereich", [] {
        diag::info("Knopftest: Auswahl " + std::to_string(me::stand().auswahl.size()));
        return me::stand().auswahl.size() == 3U;
    }));
    add(klick("##hier_suche"));
    add(tippe("titel"));
    add(pause(0.2));
    add(pruefe("rc608 Links: Die Suche filtert die Hierarchie", [] {
        const std::vector<std::string> z = me::hierarchieZeilen();
        return z.size() == 1U && z[0] == "titel";
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(taste(ImGuiKey_Delete, "Entf"));
    add(pause(0.2));
    add(klick("##gr_l_hier"));
    add(pause(0.2));
    add(pruefe("rc608 Links: die Hierarchie klappt zu, und die Teile bekommen den Platz", [] { return me::hierarchieZeilen().empty() || finde("##hier_suche", "##links", 0, false) == nullptr; }));
    add(klick("##gr_l_hier"));
    add(pause(0.2));
    // Eigene Vorlagen: nur mit Auswahl, Name erst danach, Vorschaubild, umbenennen, loeschen und zurueckholen
    add(tu("rc608 Links: keine Auswahl", [] { me::waehle(-1, false); }));
    add(pause(0.2));
    add(rolleTeileAnsEnde());
    add(klickAn("rc608 Links: Klick auf den grauen Knopf", [] {
        const Element* e = finde(tr(Str::MeVorlageSpeichern), "##links", 0, false);
        return e != nullptr ? e->rect.GetCenter() : ImVec2{};
    }));
    add(pause(0.2));
    add(pruefe("rc608 Links: ohne Auswahl fragt nichts nach einem Namen", [] { return finde("##vorlagename", "##links", 0, false) == nullptr; }));
    add(tu("rc608 Links: titel waehlen", [] { me::waehle(me::itemNummer("titel"), false); }));
    add(pause(0.2));
    add(klick(tr(Str::MeVorlageSpeichern), "##links"));
    add(pause(0.2));
    add(rolleTeileAnsEnde());
    add(pruefe("rc608 Links: mit Auswahl erst jetzt der Name", [] { return ImGui::GetIO().WantTextInput && finde("##vorlagename", "##links", 0, false) != nullptr; }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(tippe("probevorlage"));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.3));
    const auto hatVorlage = [](const char* n) {
        const std::vector<std::string> v = me::vorlagenIds();
        return std::find(v.begin(), v.end(), n) != v.end();
    };
    add(pruefe("rc608 Links: die Vorlage ist eine Kachel", [hatVorlage] {
        const auto x = me::kachelLagen();
        return hatVorlage("probevorlage") && std::any_of(x.begin(), x.end(), [](const me::KachelLage& k) { return k.name == "probevorlage"; });
    }));
    add(rolleTeileAnsEnde());
    add(foto("menue_links_vorlage"));
    add(klick("##v_umbenennen", "##links"));
    add(pause(0.2));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(tippe("neuername"));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.3));
    add(pruefe("rc608 Links: umbenannt", [hatVorlage] { return hatVorlage("neuername") && !hatVorlage("probevorlage"); }));
    add(rolleTeileAnsEnde());
    add(klickAn("rc608 Links: Doppelklick fuegt die Vorlage ein", [] {
        const Element* e = finde("neuername", "##links", 0, false);
        return e != nullptr ? ImVec2{e->rect.Min.x + e->rect.GetWidth() * 0.3F, e->rect.GetCenter().y + 8.0F} : ImVec2{};
    }, 0, true));
    add(pause(0.3));
    add(pruefe("rc608 Links: die Vorlage setzt ein", [] { return me::itemNummer("titel_2") >= 0; }));
    add(rolleTeileAnsEnde());
    add(klick("##v_loeschen", "##links"));
    add(pause(0.3));
    add(pruefe("rc608 Links: geloescht, und es laesst sich zuruecknehmen", [hatVorlage] { return !hatVorlage("neuername") && finde(tr(Str::MlVorlageZurueck), "##links", 0, false) != nullptr; }));
    add(rolleTeileAnsEnde());
    add(foto("menue_links_geloescht"));
    add(klick(tr(Str::MlVorlageZurueck), "##links"));
    add(pause(0.3));
    add(pruefe("rc608 Links: zurueckgeholt", [hatVorlage] { return hatVorlage("neuername"); }));
    // Rechtecke: 6 Sprachen, 100 % und 200 %, schmal und breit, viele Vorlagen: das Feld rollt, nichts wird abgeschnitten
    add(tu("rc608 Links: viele Vorlagen", [] {
        me::waehle(me::itemNummer("titel"), false);
        for (int i = 0; i < 30; ++i) { (void)me::speichereAuswahlAlsVorlage("vorlage_mit_einem_langen_namen_" + std::to_string(i)); }
    }));
    add(pause(0.3));
    auto skala = std::make_shared<float>(1.0F);
    const i18n::Language sprache = i18n::currentLanguage();
    add(tu("rc608 Links: Oberflaeche merken", [skala] { *skala = g_app->uiScale; }));
    for (const float zoom : {1.0F, 2.0F}) {
        for (const i18n::LanguageInfo& l : i18n::languages()) {
            const i18n::Language lang = l.language;
            const std::string kurz = l.code;
            for (const float breite : {10.0F, 400.0F, 900.0F}) {
                const std::string was = "rc608 Links: " + kurz + " " + std::to_string(static_cast<int>(zoom * 100.0F)) + " % Breite " + std::to_string(static_cast<int>(breite));
                add(tu(was, [lang, zoom, breite] {
                    i18n::setLanguage(lang);
                    g_app->uiScale = zoom;
                    me::setzeSpalten(breite, 0.0F);
                }));
                add(pause(0.12));
                // Seit 433cd7dc (rc608 Teil 3b) geht die linke Spalte zu, wenn es eng wird (200 %); wer sie braucht, oeffnet
                // sie mit dem Pfeil. Geprueft wird die offene Spalte.
                add(tu(was + ": linke Spalte offen", [] {
                    if (me::linkeSpalteZu()) { me::oeffneLinkeSpalte(); }
                }));
                add(pause(0.12));
                add(pruefe(was + ": nichts abgeschnitten, kein waagerechtes Rollen", [was] {
                    bool gut = me::kachelnScrollMaxX() < 0.5F;
                    const auto x = me::kachelLagen();
                    for (const me::KachelLage& k : x) {
                        if (k.textBreite > k.x1 - k.x0 + 0.5F || k.textUnten > k.y1 + 0.5F) {
                            diag::info("Knopftest: " + was + ": " + k.name + " Text " + std::to_string(k.textBreite) + " in " + std::to_string(k.x1 - k.x0) + ", unten " + std::to_string(k.textUnten - k.y1));
                            gut = false;
                        }
                    }
                    if (!gut || x.size() < 15U) { diag::info("Knopftest: " + was + ": waagerecht " + std::to_string(me::kachelnScrollMaxX()) + ", Kacheln " + std::to_string(x.size())); }
                    return gut && x.size() >= 15U;
                }));
            }
        }
    }
    add(foto("menue_links_viele"));
    add(tu("rc608 Links: aufraeumen", [skala, sprache] {
        g_app->uiScale = *skala;
        i18n::setLanguage(sprache);
        me::setzeSpalten(0.0F, 0.0F);
        std::error_code ec;
        std::filesystem::remove_all(std::filesystem::path(platform::settingsDirectory()) / "menue_vorlagen", ec);
        me::schliesseAlleOhneSpeichern();
    }));
    return s;
}

// rc608 Teil 3a: Probe fuer den Vergleich mit dem laufenden Spiel. Was die Knoepfe des Programms in die Datei schreiben, geht unveraendert ins Spiel:
// BHED_PROBE_AUS ist der Ordner fuer die Menues und den Shader; Fotos der Vorschau wie sonst (BHED_FOTOS).
std::vector<Schritt> schritteSpielprobe() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto ablegen = [](const std::string& datei) {
        const char* aus = std::getenv("BHED_PROBE_AUS");
        if (aus == nullptr) { return; }
        std::ofstream(std::filesystem::path(aus) / (datei + ".menu"), std::ios::binary) << me::stand().text;
        const std::string shader = me::paketInhalt("shaders/mymenu.shader");
        if (!shader.empty()) { std::ofstream(std::filesystem::path(aus) / "mymenu.shader", std::ios::binary) << shader; }
    };
    const auto doppelKachel = [](Str name) {
        return klickAn("rc608 Spielprobe: Doppelklick auf " + std::string(tr(name)), [name] {
            const Element* e = finde(tr(name), "##links", 0, false);
            return e != nullptr ? e->rect.GetCenter() : ImVec2{};
        }, 0, true);
    };
    for (const bool mp : {false, true}) {
        const std::string d = mp ? "mp" : "sp";
        // Ausrichtung: Kaesten mit Rand, 4 Schriften mal 3 Groessen, senkrecht in die Mitte ueber den Knopf des Programms
        std::string a = "menuDef\n{\n  name \"t3a_" + d + "\"\n  rect 0 0 640 480\n  fullScreen 1\n  style 1\n  backcolor 0 0 0 1\n  visible 1\n";
        if (mp) { a += "  itemDef\n  {\n    name \"schwarz\"\n    rect 0 0 640 480\n    style 1\n    backcolor 0 0 0 1\n    decoration\n    visible 1\n  }\n"; }
        int n = 0;
        for (int f = 1; f <= 4; ++f) {
            int sp = 0;
            for (const char* g : {"0.5", "0.7", "1"}) {
                const std::string nm = "b" + std::to_string(n);
                a += "  itemDef\n  {\n    name \"" + nm + "\"\n    rect " + std::to_string(20 + sp * 205) + " " + std::to_string(20 + (f - 1) * 110) + " 190 80\n    border 1\n    bordercolor 1 1 0 1\n    text \"Ag Mitte\"\n    textscale " + g + "\n    font " + std::to_string(f) +
                     "\n    forecolor 1 1 1 1\n    decoration\n    visible 1\n  }\n";
                ++n;
                ++sp;
            }
        }
        a += "}\n";
        const std::string mpOrdner = std::getenv("BHED_PROBE_MPORDNER") != nullptr ? std::getenv("BHED_PROBE_MPORDNER") : "ui/jamp/";
        const std::string pfadA = mp ? mpOrdner + "t3a_mp.menu" : "ui/t3a_sp.menu";
        add(tu("rc608 Spielprobe: Ausrichtung " + d, [a, pfadA] {
            me::schliesseAlleOhneSpeichern();
            g_app->leftMode = 4;
            tourSchliessen();
            (void)me::oeffneText(pfadA, a);
            me::setzeInteraktion(false);
            me::setzeReiter(2);
            me::leisteVorgabe();
        }));
        add(pause(0.5));
        for (int i = 0; i < 12; ++i) {
            const std::string nm = "b" + std::to_string(i);
            add(tu("rc608 Spielprobe: " + nm + " waehlen", [nm] { me::waehle(me::itemNummer(nm), false); }));
            add(pause(0.12));
            add(klick("##a_mitte"));
            add(pause(0.1));
            add(klick("##v_mitte"));
            add(pause(0.1));
        }
        add(tu("rc608 Spielprobe: keine Auswahl, ablegen", [ablegen, d] {
            me::waehle(-1, false);
            ablegen("t3a_" + d);
        }));
        add(pause(0.3));
        add(foto("t3a_" + d));
        // Teile aus der linken Leiste, wie eingefuegt
        const std::string pfadB = mp ? mpOrdner + "t3b_mp.menu" : "ui/t3b_sp.menu";
        add(tu("rc608 Spielprobe: Teile " + d, [mp, d, pfadB] {
            me::schliesseAlleOhneSpeichern();
            std::string b = "menuDef\n{\n  name \"t3b_" + d + "\"\n  rect 0 0 640 480\n  fullScreen 1\n  visible 1\n";
            if (mp) { b += "  itemDef\n  {\n    name \"schwarz\"\n    rect 0 0 640 480\n    style 1\n    backcolor 0 0 0 1\n    decoration\n    visible 1\n  }\n"; }
            (void)me::oeffneText(pfadB, b + "}\n");
            me::setzeInteraktion(false);
            me::setzeReiter(2);
        }));
        add(pause(0.4));
        const auto lage = [](const char* wert) { return tu(std::string("rc608 Spielprobe: Lage ") + wert, [wert] { me::setzeEigenschaftAuswahl("rect", {wert}); }); };
        add(doppelKachel(Str::MlTypButton));
        add(pause(0.3));
        add(lage("20 20 160 36"));
        add(pause(0.2));
        add(doppelKachel(Str::MlTypBild));
        add(pause(0.3));
        add(lage("20 80 200 150"));
        add(pause(0.2));
        add(doppelKachel(Str::MlTypBild));
        add(pause(0.3));
        add(lage("20 250 200 150"));
        add(pause(0.2));
        add(klick(tr(Str::MlWaehlen), "##inhalt"));
        add(pause(0.4));
        add(klick("##bw_suche"));
        add(tippe("menu_buttonback"));
        add(pause(0.3));
        add(klick("##bw_gfx/menus/menu_buttonback"));
        add(pause(0.3));
        add(doppelKachel(Str::MlTypVideo));
        add(pause(0.3));
        add(lage("240 20 256 192"));
        add(pause(0.2));
        add(klick(tr(Str::MlWaehlen), "##inhalt"));
        add(pause(0.4));
        add(klick("##bw_suche"));
        add(tippe("ep1video"));
        add(pause(0.3));
        add(klick("##bw_ep1video"));
        add(pause(0.3));
        add(doppelKachel(Str::MlTypModell));
        add(pause(0.5));
        add(lage("400 230 200 225"));
        add(pause(1.0));
        add(tu("rc608 Spielprobe: keine Auswahl, ablegen", [ablegen, d] {
            me::waehle(-1, false);
            ablegen("t3b_" + d);
        }));
        add(pause(0.5));
        add(foto("t3b_" + d));
    }
    add(tu("rc608 Spielprobe: aufraeumen", [] { me::schliesseAlleOhneSpeichern(); }));
    return s;
}

// rc608 Teil 3a: eine selbst gebaute animation.cfg: Die Liste zeigt ihre Namen, und das Gewaehlte steht in der Datei
std::vector<Schritt> schritteEigeneAnimation() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto ordner = std::filesystem::path(platform::settingsDirectory()) / "eigene_anim";
    const std::string ordnerText = ordner.string();
    add(tu("rc608 Eigene Animation: lose Datei ueber das Spiel legen", [ordnerText] {
        std::error_code ec;
        const auto ziel = std::filesystem::path(ordnerText) / "models" / "players" / "_humanoid";
        std::filesystem::create_directories(ziel, ec);
        std::ofstream(ziel / "animation.cfg", std::ios::binary) << "BOTH_STAND1 0 40 -1 20\nBOTH_WALK1 100 20 -1 20\nBOTH_MENUIDLE1 200 30 -1 20\n";
        GamePath g;
        g.directory = ordnerText;
        g_app->gamePaths.push_back(g);
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneText("ui/anim2.menu", "menuDef\n{\n  name \"anim2\"\n  rect 0 0 640 480\n  visible 1\n  itemDef\n  {\n    name \"figur\"\n    type ITEM_TYPE_MODEL\n    rect 200 100 200 225\n    model_g2anim \"BOTH_STAND1\"\n    asset_model \"models/players/kyle/model.glm\"\n    model_angle 180\n    visible 1\n  }\n}\n");
        me::setzeInteraktion(false);
        me::waehle(0, false);
        me::setzeReiter(2);
    }));
    add(pause(2.0));
    add(klick(tr(Str::MeModellAnimWaehlen)));
    add(pause(0.4));
    add(foto("menue_eigene_anim"));
    add(pruefe("rc608 Eigene Animation: die Liste zeigt genau die Namen der eigenen Datei", [] {
        const Element* a = finde("BOTH_WALK1", "##Popup_", 0, false);
        const Element* b = finde("BOTH_MENUIDLE1", "##Popup_", 0, false);
        const Element* c = finde("BOTH_RUN1", "##Popup_", 0, false);
        return a != nullptr && b != nullptr && c == nullptr;
    }));
    add(klick("BOTH_MENUIDLE1", "##Popup_"));
    add(pause(0.4));
    add(pruefe("rc608 Eigene Animation: die gewaehlte steht vor asset_model in der Datei", [] {
        const std::string t = me::stand().text;
        return t.find("BOTH_MENUIDLE1") != std::string::npos && t.find("BOTH_MENUIDLE1") < t.find("asset_model") && t.find("BOTH_STAND1") == std::string::npos;
    }));
    add(tu("rc608 Eigene Animation: aufraeumen", [ordnerText] {
        g_app->gamePaths.pop_back();
        me::schliesseAlleOhneSpeichern();
        std::error_code ec;
        std::filesystem::remove_all(ordnerText, ec);
    }));
    return s;
}

// rc608 Teil 3a: im Spielfoto: Movie Duels MP liest ui/MovieDuels-MP-Menus.txt, dort steht font 2 = albertus_bold und bigFont = ergoec
std::vector<Schritt> schritteMpSchrift() {
    std::vector<Schritt> s;
    s.push_back(tu("rc608 MP-Schriften: ein Menue oeffnen", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneText("ui/jamp/mp_schrift.menu", "menuDef\n{\n  name \"mp_schrift\"\n  rect 0 0 640 480\n  visible 1\n}\n");
    }));
    s.push_back(pause(0.3));
    s.push_back(pruefe("rc608 MP-Schriften: die Nummern aus dem Menuesatz des Spiels", [] {
        const std::string f2 = me::schriftNameMp(2);
        const std::string f3 = me::schriftNameMp(3);
        const std::string fehler = "spiel " + std::to_string(me::stand().spiel) + ": font 2 = " + f2 + ", font 3 = " + f3;
        diag::info(fehler);
        if (me::stand().spiel == 2) { return f2 == "albertus_bold" && f3 == "ergoec"; }
        return me::stand().spiel == 0 ? (f2 == "ergoec" && f3 == "anewhope") : !f2.empty();
    }));
    return s;
}

// rc608 Teil 3a (shank, mit Foto): Rahmen genau uebereinander: "Double-click: choose image" und "... video" uebereinander, unlesbar.
// Beim Ablegen rastete die Ecke exakt auf die eines gleich grossen Elements. Einfuegen und Duplizieren sind derselbe Fall.
std::vector<Schritt> schritteStapel() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto gleich = [](const std::vector<int>& a, std::initializer_list<int> b) { return a == std::vector<int>(b); };
    static const char* const kRahmenText =
        "menuDef\n{\n  name \"stapel\"\n  rect 0 0 640 480\n  visible 1\n"
        "  itemDef\n  {\n    name \"image\"\n    style WINDOW_STYLE_SHADER\n    rect 11 11 200 100\n    visible 1\n  }\n"
        "  itemDef\n  {\n    name \"video\"\n    style WINDOW_STYLE_SHADER\n    rect 11 11 200 100\n    // jkstudio:rahmen video\n    visible 1\n  }\n"
        "  itemDef\n  {\n    name \"neben\"\n    style WINDOW_STYLE_SHADER\n    rect 300 11 200 100\n    visible 1\n  }\n"
        "  itemDef\n  {\n    name \"ecke\"\n    style WINDOW_STYLE_SHADER\n    rect 480 100 100 100\n    visible 1\n  }\n"
        "}\n";
    add(tu("rc608 Stapel: zwei Rahmen genau uebereinander", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneText("ui/stapel.menu", kRahmenText);
        me::setzeInteraktion(false);
    }));
    add(pause(0.4));
    add(foto("menue_stapel_texte"));
    add(pruefe("rc608 Stapel: der Text steht nur fuer den obersten Rahmen", [gleich] {
        std::string liste;
        for (const int i : me::rahmenTexte()) { liste += std::to_string(i) + " "; }
        diag::info("Knopftest: Platzhalter fuer " + liste);
        return gleich(me::rahmenTexte(), {1, 2, 3});   // neben: "ecke" beruehrt nur seine Ecke, nicht den Text
    }));
    add(pruefe("rc608 Stapel: die Datei bleibt unberuehrt", [] { return me::stand().text == kRahmenText; }));

    // Ablegen durch Ziehen: leeres Menue, zwei gleich grosse Bilder, das zweite knapp neben dem ersten
    auto tiefe = std::make_shared<int>(0);
    auto rahmen = std::make_shared<std::array<float, 4>>();
    const auto neues = [] {
        const auto st = me::stand();
        return me::itemLagen()[static_cast<std::size_t>(st.auswahl[0])];
    };
    add(tu("rc608 Stapel: leeres Menue", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneText("ui/stapel2.menu", "menuDef\n{\n  name \"stapel2\"\n  rect 0 0 640 480\n  visible 1\n}\n");
        me::setzeInteraktion(false);
    }));
    add(pause(0.4));
    add(zieheAufFlaeche(tr(Str::MlTypBild), 100, 100));
    add(pause(0.1));
    add(tu("rc608 Stapel: Stand merken", [tiefe] { *tiefe = me::stand().undoSchritte; }));
    add(zieheAufFlaeche(tr(Str::MlTypBild), 102, 102, [rahmen] { (void)me::ablageRahmen(rahmen->data()); }));
    add(pause(0.1));
    add(foto("menue_stapel_abgelegt"));
    add(pruefe("rc608 Stapel: Ablegen landet nicht deckungsgleich, der Rahmen beim Ziehen zeigt die echte Stelle, ein Schritt", [tiefe, rahmen, neues] {
        const auto n = neues();
        const auto st = me::stand();
        diag::info("Knopftest: Stapel abgelegt auf " + std::to_string(n.x) + ", " + std::to_string(n.y) + ", Rahmen " + std::to_string((*rahmen)[0]) + ", " + std::to_string((*rahmen)[1]) + ", Undo +" + std::to_string(st.undoSchritte - *tiefe));
        return !(std::fabs(n.x - 100.0F) < 1.0F && std::fabs(n.y - 100.0F) < 1.0F) && std::fabs(n.x - (*rahmen)[0]) < 0.5F && std::fabs(n.y - (*rahmen)[1]) < 0.5F && st.undoSchritte == *tiefe + 1;
    }));
    add(tu("rc608 Stapel: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
    add(pause(0.1));
    add(pruefe("rc608 Stapel: ein Rueckgaengig nimmt das Abgelegte weg", [] { return me::stand().items == 1; }));
    // Alt schaltet das Einrasten aus: dort, wohin man zieht
    add(tu("rc608 Stapel: Alt halten", [] { ImGui::GetIO().AddKeyEvent(ImGuiMod_Alt, true); }));
    add(zieheAufFlaeche(tr(Str::MlTypBild), 102, 102));
    add(tu("rc608 Stapel: Alt los", [] { ImGui::GetIO().AddKeyEvent(ImGuiMod_Alt, false); }));
    add(pause(0.1));
    add(pruefe("rc608 Stapel: mit Alt kein Einrasten und kein Ruecken", [neues] {
        const auto n = neues();
        diag::info("Knopftest: Stapel mit Alt auf " + std::to_string(n.x) + ", " + std::to_string(n.y));
        return std::fabs(n.x - 102.0F) <= 3.0F && std::fabs(n.y - 102.0F) <= 3.0F && !(std::fabs(n.x - 100.0F) < 1.0F && std::fabs(n.y - 100.0F) < 1.0F);
    }));
    add(tu("rc608 Stapel: Rueckgaengig Alt", [] { (void)me::rueckgaengig(); }));
    add(pause(0.1));

    // Schnell zweimal zugegriffen und gezogen: Der erste Griff ist ein Klick, der zweite zieht: genau ein neues Element, dort, wo man loslaesst
    add(tu("rc608 Stapel: leeres Menue fuer das schnelle Ziehen", [] {
        (void)me::oeffneText("ui/stapel3.menu", "menuDef\n{\n  name \"stapel3\"\n  rect 0 0 640 480\n  visible 1\n}\n");
        me::setzeInteraktion(false);
    }));
    add(pause(0.3));
    add(rolleZuKachel(tr(Str::MlTypBild)));
    add(klick(tr(Str::MlTypBild)));
    add(zieheAufFlaeche(tr(Str::MlTypBild), 300, 300));
    add(pause(0.1));
    add(pruefe("rc608 Stapel: schnelles Zweitgreifen setzt genau ein Element, wo man loslaesst", [neues] {
        const auto st = me::stand();
        if (st.items != 1 || st.auswahl.empty()) {
            diag::info("Knopftest: schnelles Ziehen: " + std::to_string(st.items) + " Elemente");
            return false;
        }
        const auto n = neues();
        return std::fabs(n.x - 300.0F) <= 6.0F && std::fabs(n.y - 300.0F) <= 6.0F;
    }));
    add(tu("rc608 Stapel: zurueck zum zweiten Menue", [] {
        me::schliesseAlleOhneSpeichern();
        (void)me::oeffneText("ui/stapel2.menu", "menuDef\n{\n  name \"stapel2\"\n  rect 0 0 640 480\n  visible 1\n  itemDef\n  {\n    name \"image\"\n    style WINDOW_STYLE_SHADER\n    rect 100 100 200 150\n    visible 1\n  }\n}\n");
        me::setzeInteraktion(false);
    }));
    add(pause(0.3));

    // Duplizieren und Einfuegen
    add(tu("rc608 Stapel: Duplizieren", [tiefe] {
        me::waehle(0, false);
        *tiefe = me::stand().undoSchritte;
        me::dupliziereAuswahl();
    }));
    add(pause(0.2));
    add(pruefe("rc608 Stapel: Duplizieren liegt nicht auf dem Original, ein Schritt", [tiefe, neues] {
        const auto l = me::itemLagen();
        const auto n = neues();
        return l.size() == 2 && !(std::fabs(n.x - l[0].x) < 1.0F && std::fabs(n.y - l[0].y) < 1.0F) && me::stand().undoSchritte == *tiefe + 1;
    }));
    add(tu("rc608 Stapel: Rueckgaengig Duplizieren", [] { (void)me::rueckgaengig(); }));
    add(pause(0.1));
    add(tu("rc608 Stapel: Einfuegen", [tiefe] {
        me::waehle(0, false);
        *tiefe = me::stand().undoSchritte;
        me::einfuegenOhneDeckung(me::kopiereAuswahl());
    }));
    add(pause(0.2));
    add(pruefe("rc608 Stapel: Einfuegen liegt nicht auf dem Original, ein Schritt", [tiefe, neues] {
        const auto l = me::itemLagen();
        const auto n = neues();
        return l.size() == 2 && !(std::fabs(n.x - l[0].x) < 1.0F && std::fabs(n.y - l[0].y) < 1.0F) && me::stand().undoSchritte == *tiefe + 1;
    }));
    add(tu("rc608 Stapel: zweimal Einfuegen", [] {
        me::waehle(0, false);
        me::einfuegenOhneDeckung(me::kopiereAuswahl());
    }));
    add(pause(0.2));
    add(pruefe("rc608 Stapel: auch das zweite Mal frei", [] {
        const auto l = me::itemLagen();
        for (std::size_t a = 0; a < l.size(); ++a) {
            for (std::size_t b = a + 1; b < l.size(); ++b) {
                if (std::fabs(l[a].x - l[b].x) < 1.0F && std::fabs(l[a].y - l[b].y) < 1.0F) { return false; }
            }
        }
        return l.size() == 3;
    }));
    add(tu("rc608 Stapel: aufraeumen", [] { me::schliesseAlleOhneSpeichern(); }));
    return s;
}
