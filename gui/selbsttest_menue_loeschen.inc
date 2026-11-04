// selbsttest_menue_loeschen.inc - rc609 ME1: Loeschen in der Mitte des Menues (BHED_EDITORTEST=menueloeschen).
// Eingebunden in namespace menuetest (selbsttest_menue.inc). Tester: "it crashed when I deleted a button".
// Jeder Weg (Entf auf der Flaeche, Entf in der Hierarchie, Rechtsklick, Knopf der Leiste, Umschalt-Auswahl) loescht
// in der Mitte; Schloss und Auge bleiben an ihren Items, auch nach Rueckgaengig und Wiederholen.

// Zehn Items: unbenannte, doppelte Namen, eine Gruppe; der Text T0..T9 kennzeichnet jedes
const char* const kLoeschText =
    "menuDef\n{\n  name \"loeschen\"\n  rect 0 0 640 480\n  fullScreen 1\n  visible 1\n"
    "  itemDef\n  {\n    name \"eins\"\n    type ITEM_TYPE_BUTTON\n    rect 20 20 100 30\n    text \"T0\"\n    visible 1\n  }\n"
    "  itemDef\n  {\n    group \"g1\"\n    type ITEM_TYPE_BUTTON\n    rect 140 20 100 30\n    text \"T1\"\n    visible 1\n  }\n"
    "  itemDef\n  {\n    name \"zwei\"\n    group \"g1\"\n    type ITEM_TYPE_BUTTON\n    rect 260 20 100 30\n    text \"T2\"\n    visible 1\n  }\n"
    "  itemDef\n  {\n    name \"mitte\"\n    group \"g1\"\n    type ITEM_TYPE_BUTTON\n    rect 380 20 100 30\n    text \"T3\"\n    visible 1\n  }\n"
    "  itemDef\n  {\n    type ITEM_TYPE_BUTTON\n    rect 20 120 100 30\n    text \"T4\"\n    visible 1\n  }\n"
    "  itemDef\n  {\n    name \"doppel\"\n    type ITEM_TYPE_BUTTON\n    rect 140 120 100 30\n    text \"T5\"\n    visible 1\n  }\n"
    "  itemDef\n  {\n    name \"drei\"\n    type ITEM_TYPE_BUTTON\n    rect 260 120 100 30\n    text \"T6\"\n    visible 1\n  }\n"
    "  itemDef\n  {\n    name \"doppel\"\n    type ITEM_TYPE_BUTTON\n    rect 380 120 100 30\n    text \"T7\"\n    visible 1\n  }\n"
    "  itemDef\n  {\n    type ITEM_TYPE_BUTTON\n    rect 20 220 100 30\n    text \"T8\"\n    visible 1\n  }\n"
    "  itemDef\n  {\n    name \"vier\"\n    type ITEM_TYPE_BUTTON\n    rect 140 220 100 30\n    text \"T9\"\n    visible 1\n  }\n"
    "}\n";

// Ein Item als Zeile: Name, Gruppe, Text, Schloss (z), Auge (v)
std::string merkerZeile(const me::ItemMerker& x) {
    return x.name + "|" + x.gruppe + "|" + x.text + (x.gesperrt ? "|z" : "") + (x.verborgen ? "|v" : "");
}
std::string merkerText(const std::vector<me::ItemMerker>& v) {
    std::string s;
    for (const me::ItemMerker& x : v) { s += merkerZeile(x) + "; "; }
    return s;
}

// Die k-te Zeile der Hierarchie, deren Beschriftung mit dem Namen beginnt (von oben)
ImVec2 zeileNr(const std::string& name, int k) {
    for (const Element& e : elemente::alle()) {
        if (e.label.rfind(name + "  ", 0) == 0 && k-- == 0) { return e.rect.GetCenter(); }
    }
    return ImVec2{-1.0F, -1.0F};
}

// Punkt 2 (Tester: "not sure why the open menu button isn't displayed nicely with the create and open project buttons"):
// "Menue oeffnen" als dritte Karte des Startbildschirms; ohne Projekt oeffnen, bearbeiten, an den Ort speichern
std::vector<Schritt> schritteStartMenue() {
    namespace fs = std::filesystem;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto datei = std::make_shared<fs::path>(bhed::utf8Pfad(platform::executableDirectory()) / "me1_start" / "probe.menu");
    const auto leer = [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
    };
    add(tu("ME1 Start: kein Menue offen, die Probe liegt auf der Platte", [datei, leer] {
        std::error_code ec;
        fs::remove_all(datei->parent_path(), ec);
        fs::create_directories(datei->parent_path(), ec);
        std::ofstream(*datei, std::ios::binary) << kLoeschText;
        me::setzeMenueDialog([datei] { return pfadText(*datei); });
        leer();
    }));
    add(pause(0.5));
    add(pruefe("ME1 Start: Menue oeffnen ist die dritte Karte, so gross wie die Projekte und in ihrer Reihe", [] {
        const Element* n = finde(tr(Str::MePrjNeu), "##menuestart", 0, false);
        const Element* o = finde(tr(Str::MePrjOeffnen), "##menuestart", 0, false);
        const Element* m = finde(tr(Str::MeStartMenueOeffnen), "##menuestart", 0, false);
        if (n == nullptr || o == nullptr || m == nullptr) { return false; }
        return std::fabs(m->rect.Min.y - n->rect.Min.y) < 0.5F && std::fabs(m->rect.GetHeight() - n->rect.GetHeight()) < 0.5F &&
               std::fabs(m->rect.GetWidth() - n->rect.GetWidth()) < 0.5F && m->rect.Min.x > o->rect.Max.x;
    }));
    add(pruefe("ME1 Start: die Knoepfe oben in der Leiste bleiben", [] {
        return finde(tr(Str::MeDateiOeffnen), "", 0, false) != nullptr && finde(tr(Str::MeAusSpiel), "", 0, false) != nullptr;
    }));
    add(klick(tr(Str::MeStartMenueOeffnen), "##menuestart"));
    add(pause(0.3));
    add(foto("menue_start_oeffnen"));
    add(klick(tr(Str::MeDateiOeffnen), "##Popup"));
    add(pause(0.3));
    add(pruefe("ME1 Start: die Datei von der Platte ist offen, ohne Projekt", [] {
        return me::stand().dokumente == 1 && me::projektDatei().empty() && me::itemMerker().size() == 10;
    }));
    add(tu("ME1 Start: bearbeiten (mitte loeschen)", [] {
        me::waehle(me::itemNummer("mitte"), false);
        me::loescheAuswahl();
    }));
    add(pause(0.2));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_S, "Strg+S"));
    add(pause(0.3));
    add(pruefe("ME1 Start: Strg+S schreibt in dieselbe Datei", [datei] {
        std::ifstream f(*datei, std::ios::binary);
        const std::string inhalt((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        return inhalt == me::stand().text && inhalt.find("\"mitte\"") == std::string::npos && inhalt.find("\"zwei\"") != std::string::npos;
    }));
    add(tu("ME1 Start: wieder ohne Menue", leer));
    add(pause(0.3));
    add(klick(tr(Str::MeStartMenueOeffnen), "##menuestart"));
    add(pause(0.3));
    add(klick(tr(Str::MeAusSpiel), "##Popup"));
    add(pause(0.3));
    add(tippe("ui/main.menu"));
    add(pause(0.3));
    add(klick("ui/main.menu", "##Popup"));
    add(pause(0.5));
    add(pruefe("ME1 Start: Aus dem Spiel oeffnet das Menue des Spiels", [] { return me::stand().dokumente == 1 && me::dokumentName() == "ui/main.menu"; }));
    add(tu("ME1 Start: aufraeumen", [datei, leer] {
        me::setzeMenueDialog(nullptr);
        leer();
        std::error_code ec;
        fs::remove_all(datei->parent_path(), ec);
    }));
    add(pause(0.3));
    return s;
}

std::vector<Schritt> schritteSpeichernOrt();   // rc609 ME2 (selbsttest_menue_speichern.inc)
std::vector<Schritt> schritteSpielbeleg();     // rc609 Endrunde 1 (selbsttest_menue_speichern.inc)

// Druck auf die Flaeche bei (vx, vy) im virtuellen Schirm, ziehen, mitten im Zug Entf, weiter ziehen, loslassen
Schritt zugMitEntf(const std::string& was, float vx, float vy, float dx, float dy) {
    auto start = std::make_shared<ImVec2>();
    return Schritt{was, [=](int b) {
                       ImGuiIO& io = ImGui::GetIO();
                       const auto x = me::stand();
                       const float sx = x.flaeche[2] / 640.0F, sy = x.flaeche[3] / 480.0F;
                       if (b == 0) {
                           *start = ImVec2{x.flaeche[0] + vx * sx, x.flaeche[1] + vy * sy};
                           setzeMaus(*start);
                           return false;
                       }
                       if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
                       if (b >= 3 && b <= 6) {
                           const float t = static_cast<float>(b - 2) / 8.0F;
                           setzeMaus(ImVec2{start->x + dx * sx * t, start->y + dy * sy * t});
                           return false;
                       }
                       if (b == 7) { io.AddKeyEvent(ImGuiKey_Delete, true); return false; }
                       if (b == 8) { io.AddKeyEvent(ImGuiKey_Delete, false); return false; }
                       if (b >= 9 && b <= 12) {
                           const float t = static_cast<float>(b - 4) / 8.0F;
                           setzeMaus(ImVec2{start->x + dx * sx * t, start->y + dy * sy * t});
                           return false;
                       }
                       if (b == 13) { io.AddMouseButtonEvent(0, false); return false; }
                       return b >= 18;
                   }};
}

struct LoeschStand {
    std::vector<me::ItemMerker> vorher, nachher;
    std::string textVorher, textNachher;
    std::vector<int> weg;   // was das Loeschen nehmen soll: die Auswahl ohne gesperrte
};

std::vector<Schritt> schritteLoeschen() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(pause(1.0));
    add(klick(tr(Str::ViewMenuEditor), "###main"));
    add(pause(0.2));
    // Die Probe: Schloss auf T4 (unbenannt) und T7 (zweites "doppel"), Auge zu auf T8 (unbenannt) und T9
    const auto probe = [&](const std::string& was) {
        add(tu("ME1 " + was + ": Probe oeffnen", [] {
            me::schliesseAlleOhneSpeichern();
            g_app->leftMode = 4;
            tourSchliessen();
            (void)me::oeffneText("ui/loeschen.menu", kLoeschText);
            me::setzeInteraktion(false);
            me::setzeSpalten(0.0F, 0.0F);
            me::setzeReiter(2);   // die Properties sind offen
            me::leisteVorgabe();
        }));
        add(pause(0.2));
        add(tu("ME1 " + was + ": Schloss und Auge setzen", [] {
            me::sperreItem(4, true);
            me::sperreItem(7, true);
            me::verbergeItem(8, true);
            me::verbergeItem(9, true);
        }));
        add(pause(0.1));
    };
    // Vor dem Loeschen festhalten: Items, Text, Auswahl
    const auto merken = [](std::shared_ptr<LoeschStand> z) {
        return tu("ME1: Stand vor dem Loeschen", [z] {
            z->vorher = me::itemMerker();
            z->textVorher = me::stand().text;
            z->weg.clear();
            for (const int i : me::stand().auswahl) {
                if (i >= 0 && static_cast<std::size_t>(i) < z->vorher.size() && !z->vorher[static_cast<std::size_t>(i)].gesperrt) { z->weg.push_back(i); }
            }
            std::sort(z->weg.begin(), z->weg.end());
            diag::info("Knopftest: ME1 vorher " + merkerText(z->vorher) + " Auswahl " + std::to_string(me::stand().auswahl.size()));
        });
    };
    // Danach: genau die Auswahl ohne Gesperrte ist weg, alle uebrigen Items behalten Schloss und Auge;
    // Rueckgaengig stellt Text und Merker wieder her, Wiederholen den Stand nach dem Loeschen
    const auto pruefen = [&](const std::string& was, std::shared_ptr<LoeschStand> z, bool undoTasten) {
        add(pause(0.2));
        add(pruefe("ME1 " + was + ": geloescht, Schloss und Auge an denselben Items", [z, was] {
            std::vector<me::ItemMerker> soll;
            for (std::size_t i = 0; i < z->vorher.size(); ++i) {
                if (!std::binary_search(z->weg.begin(), z->weg.end(), static_cast<int>(i))) { soll.push_back(z->vorher[i]); }
            }
            z->nachher = me::itemMerker();
            z->textNachher = me::stand().text;
            const bool ok = !z->weg.empty() && merkerText(z->nachher) == merkerText(soll);
            if (!ok) { diag::info("Knopftest: ME1 " + was + " soll " + merkerText(soll) + " ist " + merkerText(z->nachher)); }
            return ok;
        }));
        if (undoTasten) {
            add(klickAn("ME1: Fokus auf die Flaeche", [] {
                const auto x = me::stand();
                return ImVec2{x.flaeche[0] + 2.0F, x.flaeche[1] + x.flaeche[3] - 2.0F};
            }));
            add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
        } else {
            add(tu("ME1: Rueckgaengig", [] { (void)me::rueckgaengig(); }));
        }
        add(pause(0.2));
        add(pruefe("ME1 " + was + ": Rueckgaengig: Text byte-gleich, Schloss und Auge wie vorher", [z, was] {
            const std::vector<me::ItemMerker> ist = me::itemMerker();
            const bool ok = me::stand().text == z->textVorher && merkerText(ist) == merkerText(z->vorher);
            if (!ok) { diag::info("Knopftest: ME1 " + was + " Rueckgaengig ist " + merkerText(ist)); }
            return ok;
        }));
        if (undoTasten) {
            add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Y, "Strg+Y"));
        } else {
            add(tu("ME1: Wiederholen", [] { (void)me::wiederholen(); }));
        }
        add(pause(0.2));
        add(pruefe("ME1 " + was + ": Wiederholen: wie nach dem Loeschen", [z, was] {
            const std::vector<me::ItemMerker> ist = me::itemMerker();
            const bool ok = me::stand().text == z->textNachher && merkerText(ist) == merkerText(z->nachher);
            if (!ok) { diag::info("Knopftest: ME1 " + was + " Wiederholen ist " + merkerText(ist)); }
            return ok;
        }));
    };
    const auto zeile = [](const std::string& name, int k) { return [name, k] { return zeileNr(name, k); }; };

    // 1. Entf auf der Flaeche: "mitte" (T3, in der Gruppe)
    {
        auto z = std::make_shared<LoeschStand>();
        probe("Flaeche");
        add(ziehFlaeche("ME1: Klick auf die leere Flaeche", 600, 470, 0, 0, 0, 0));
        add(tu("ME1: mitte waehlen", [] { me::waehle(me::itemNummer("mitte"), false); }));
        add(merken(z));
        add(taste(ImGuiKey_Delete, "Entf"));
        pruefen("Entf auf der Flaeche", z, true);
    }
    // 2. Entf in der Hierarchie: das erste "doppel" (T5); oben steht das zweite (T7, gesperrt)
    {
        auto z = std::make_shared<LoeschStand>();
        probe("Hierarchie");
        add(klickAn("ME1: Zeile doppel (T5)", zeile("doppel", 1)));
        add(pause(0.1));
        add(merken(z));
        add(taste(ImGuiKey_Delete, "Entf"));
        pruefen("Entf in der Hierarchie", z, false);
    }
    // 3. Rechtsklick in der Hierarchie, Loeschen: "zwei" (T2, in der Gruppe)
    {
        auto z = std::make_shared<LoeschStand>();
        probe("Kontext");
        add(klickAn("ME1: Rechtsklick auf zwei", zeile("zwei", 0), 1));
        add(pause(0.2));
        add(merken(z));
        add(klick(tr(Str::ActDelete)));
        pruefen("Rechtsklick Loeschen", z, false);
    }
    // 4. Knopf der Leiste bei offenen Properties: "drei" (T6)
    {
        auto z = std::make_shared<LoeschStand>();
        probe("Leiste");
        add(klickAn("ME1: Zeile drei", zeile("drei", 0)));
        add(pause(0.2));
        add(merken(z));
        add(klick("##k_weg", "##karte"));
        pruefen("Knopf der Leiste", z, false);
    }
    // 5. Umschalt-Auswahl in der Hierarchie von vier (T9, verborgen) bis drei (T6): T7 bleibt (gesperrt)
    {
        auto z = std::make_shared<LoeschStand>();
        probe("Umschalt");
        add(klickAn("ME1: Zeile vier", zeile("vier", 0)));
        add(pause(0.1));
        add(halteTaste(ImGuiMod_Shift, true));
        add(klickAn("ME1: Umschalt+Klick auf drei", zeile("drei", 0)));
        add(halteTaste(ImGuiMod_Shift, false));
        add(pause(0.1));
        add(pruefe("ME1 Umschalt: vier bis drei gewaehlt", [] { return me::stand().auswahl.size() == 4; }));
        add(merken(z));
        add(taste(ImGuiKey_Delete, "Entf"));
        pruefen("Umschalt-Auswahl", z, true);
    }
    // 6. Umschalt-Auswahl in die Gruppe hinein (doppel T5 bis zwei T2), Rechtsklick in die Auswahl, Loeschen
    {
        auto z = std::make_shared<LoeschStand>();
        probe("Gruppe");
        add(klickAn("ME1: Zeile doppel (T5)", zeile("doppel", 1)));
        add(pause(0.1));
        add(halteTaste(ImGuiMod_Shift, true));
        add(klickAn("ME1: Umschalt+Klick auf zwei", zeile("zwei", 0)));
        add(halteTaste(ImGuiMod_Shift, false));
        add(pause(0.1));
        add(klickAn("ME1: Rechtsklick auf zwei", zeile("zwei", 0), 1));
        add(pause(0.2));
        add(merken(z));
        add(klick(tr(Str::ActDelete)));
        pruefen("Umschalt in die Gruppe", z, false);
    }
    // 7. Entf, waehrend die Maus noch zieht: Groesse am Griff, Verschieben, Klick in eine Mehrfachauswahl (ohne Ziehen).
    // Das Loeschen gilt, der Zug endet ohne Wirkung.
    for (int art = 0; art < 3; ++art) {
        auto z = std::make_shared<LoeschStand>();
        const char* const arten[] = {"Entf beim Skalieren", "Entf beim Verschieben", "Entf beim Klick in die Mehrfachauswahl"};
        const std::string was = arten[art];
        probe("Zug");
        add(tu("ME1 " + was + ": waehlen", [art] {
            me::verbergeItem(me::itemNummer("vier"), false);   // das letzte Element (T9) wieder sichtbar und greifbar
            me::waehle(me::itemNummer("vier"), false);
            if (art == 2) { me::waehle(me::itemNummer("drei"), true); }
        }));
        add(pause(0.2));
        add(merken(z));
        // vier: rect 140 220 100 30; der Griff unten rechts, die Mitte
        if (art == 0) { add(zugMitEntf("ME1: am Griff ziehen, Entf", 240.0F, 250.0F, 30.0F, 20.0F)); }
        if (art == 1) { add(zugMitEntf("ME1: verschieben, Entf", 190.0F, 235.0F, 40.0F, 30.0F)); }
        if (art == 2) { add(zugMitEntf("ME1: Klick ohne Ziehen, Entf", 190.0F, 235.0F, 0.0F, 0.0F)); }
        pruefen(was, z, false);
        add(pruefe("ME1 " + was + ": die Auswahl zeigt auf vorhandene Elemente", [] {
            const auto x = me::stand();
            const std::size_t n = me::itemMerker().size();
            return std::all_of(x.auswahl.begin(), x.auswahl.end(), [n](int i) { return i >= 0 && static_cast<std::size_t>(i) < n; });
        }));
    }
    // 8. Echte Menues des Spiels (Einzelspieler und Mehrspieler mit #include): ein Element mit eigenem Namen nahe der Mitte,
    // Schloss und Auge dahinter und am Ende; jeder Weg, dazu mit Events bzw. Text-Editor rechts und im Bedienmodus
    for (const char* datei : {"ui/main.menu", "ui/ingame.menu", "ui/jamp/main.menu"}) {
        const std::string d = datei;
        for (int weg = 0; weg < 5; ++weg) {
            auto z = std::make_shared<LoeschStand>();
            auto ziel = std::make_shared<std::string>();
            const char* const wege[] = {" Flaeche Entf", " Leiste", " Hierarchie Rechtsklick", " Strg+X mit Events", " Hierarchie Entf im Bedienmodus"};
            const std::string was = d + wege[weg];
            add(tu("ME1 " + was + ": oeffnen", [d, weg] {
                me::schliesseAlleOhneSpeichern();
                g_app->leftMode = 4;
                tourSchliessen();
                (void)me::oeffneAusSpiel(d);
                me::setzeInteraktion(weg == 4);
                me::setzeSpalten(0.0F, 0.0F);
                me::setzeReiter(weg == 3 ? 3 : weg == 4 ? 1 : 2);
                me::leisteVorgabe();
            }));
            add(pause(0.3));
            add(tu("ME1 " + was + ": Element waehlen, Schloss und Auge dahinter", [ziel] {
                const std::vector<me::ItemMerker> v = me::itemMerker();
                const int n = static_cast<int>(v.size());
                int j = -1;
                for (int k = 0; k < n && j < 0; ++k) {
                    for (const int c : {n / 2 + k, n / 2 - k}) {
                        if (j >= 0 || c < 0 || c >= n || v[static_cast<std::size_t>(c)].name.empty()) { continue; }
                        const auto gleich = std::count_if(v.begin(), v.end(), [&](const me::ItemMerker& x) { return x.name == v[static_cast<std::size_t>(c)].name; });
                        if (gleich == 1) { j = c; }
                    }
                }
                *ziel = j >= 0 ? v[static_cast<std::size_t>(j)].name : std::string();
                me::sperreItem(j + 1, true);
                me::verbergeItem(j + 2, true);
                me::sperreItem(n - 1, true);
                me::waehle(j, false);
            }));
            add(pause(0.2));
            add(pruefe("ME1 " + was + ": Menue mit Items und einem Ziel", [ziel] { return me::itemMerker().size() >= 6 && !ziel->empty(); }));
            if (weg == 0 || weg == 3) {
                add(klickAn("ME1: Fokus auf die Flaeche", [] {
                    const auto x = me::stand();
                    return ImVec2{x.flaeche[0] + 2.0F, x.flaeche[1] + x.flaeche[3] - 2.0F};
                }));
                add(tu("ME1: Ziel wieder waehlen", [ziel] { me::waehle(me::itemNummer(*ziel), false); }));
                add(merken(z));
                add(weg == 0 ? taste(ImGuiKey_Delete, "Entf") : tasteMit(ImGuiMod_Ctrl, ImGuiKey_X, "Strg+X"));
            } else if (weg == 1) {
                add(merken(z));
                add(klick("##k_weg", "##karte"));
            } else if (weg == 2) {
                add(klickAn("ME1: Rechtsklick auf die Zeile", [ziel] { return zeileNr(*ziel, 0); }, 1));
                add(pause(0.2));
                add(merken(z));
                add(klick(tr(Str::ActDelete)));
            } else {
                add(klickAn("ME1: Klick auf die Zeile", [ziel] { return zeileNr(*ziel, 0); }));
                add(pause(0.2));
                add(merken(z));
                add(taste(ImGuiKey_Delete, "Entf"));
            }
            pruefen(was, z, weg == 0);
        }
    }
    add(tu("ME1: Bearbeiten statt Bedienen", [] { me::setzeInteraktion(false); }));
    for (auto& x : schritteStartMenue()) { add(std::move(x)); }
    for (auto& x : schritteSpeichernOrt()) { add(std::move(x)); }   // rc609 ME2
    add(tu("Zurueck zum Skript-Modus", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 0;
    }));
    return s;
}
