// selbsttest_numpad.inc - rc612 NUMPAD: der Ziffernblock setzt im Karten-Editor das Raster (karteneditor, BHED_KARTE_NUR=numpad).
//
// Wunsch eines Mappers (shank 09.11.): "the NUMPAD numbers should change grids in mapping". Vorgabe: Num 0-9 tun, was die
// obere Ziffernreihe tut (NRC: 1..9 = SetGrid1..SetGrid256, 0 = ToggleGrid; grid.cpp:192-200, xywindow.cpp:2216).
// Der Schalter "Numpad sets the grid" (Menue Grid und Preferences > Grid) stellt ausgeschaltet NRCs Belegung her: Num 1
// ViewFront, Num 3 ViewSide, Num 7 ViewTop (xywindow.cpp:2273-2275).

namespace numpad {

namespace ke = karteneditor;

// Die Raster der Ziffern 1..9 der oberen Reihe (SetGrid1 .. SetGrid256)
float rasterDerZiffer(int d) { return static_cast<float>(1 << (d - 1)); }

// Was gespeichert ist ("-" = nichts): eine Einstellung des Karten-Editors, eine eigene Taste
std::string gespeichert(const char* name) {
    const auto it = g_app->settings.karte.find(name);
    return it != g_app->settings.karte.end() ? it->second : std::string("-");
}
std::string gespeicherteTaste(const char* name) {
    const auto it = g_app->settings.karteTasten.find(name);
    return it != g_app->settings.karteTasten.end() ? it->second : std::string("-");
}

// Die Maus auf ein Element stellen und stehen lassen, bis sein Hinweis (Tooltip) da ist
Schritt schweben(const std::string& was, const std::string& label, const std::string& fenster, bool praefix) {
    auto ab = std::make_shared<double>(0.0);
    return {was, [=](int b) {
                const Element* e = finde(label, fenster, 0, praefix);
                if (e == nullptr) {
                    meldeFehler(was + ": nicht gefunden");
                    return true;
                }
                if (b == 0) { *ab = ImGui::GetTime(); }
                setzeMaus(e->rect.GetCenter());
                return ImGui::GetTime() - *ab > 0.8;
            }};
}

// Steht ein Hinweis, und zwar ganz im Programmfenster?
bool hinweisImBild() {
    const ImVec2 g = ImGui::GetIO().DisplaySize;
    for (const ImGuiWindow* w : GImGui->Windows) {
        if ((w->Flags & ImGuiWindowFlags_Tooltip) == 0 || !w->Active || w->Hidden) { continue; }
        diag::detail("Knopftest: Numpad - Hinweis " + std::to_string(static_cast<int>(w->Size.x)) + " x " + std::to_string(static_cast<int>(w->Size.y)) +
                     " bei " + std::to_string(static_cast<int>(w->Pos.x)) + "/" + std::to_string(static_cast<int>(w->Pos.y)) + ", Programm " +
                     std::to_string(static_cast<int>(g.x)) + " x " + std::to_string(static_cast<int>(g.y)));
        return w->Pos.x >= 0.0F && w->Pos.y >= 0.0F && w->Pos.x + w->Size.x <= g.x && w->Pos.y + w->Size.y <= g.y;
    }
    return false;
}

// Die Maus eine Zeile entlangfuehren, bis ein Hinweis erscheint (die Markierung "(x)" hinter einer geaenderten Taste)
Schritt hinweisInZeile(const std::string& was, const std::string& label, const std::string& fenster) {
    return {was, [=](int b) {
                const Element* e = finde(label, fenster, 0, false);
                if (e == nullptr) {
                    meldeFehler(was + ": nicht gefunden");
                    return true;
                }
                if (b > 0) {
                    for (const ImGuiWindow* w : GImGui->Windows) {
                        if ((w->Flags & ImGuiWindowFlags_Tooltip) != 0 && w->Active && !w->Hidden) { return true; }
                    }
                }
                const float x = e->rect.Min.x + 4.0F * static_cast<float>(b);
                if (x > e->rect.Max.x) {
                    meldeFehler(was + ": kein Hinweis in der Zeile");
                    return true;
                }
                setzeMaus(ImVec2{x, e->rect.GetCenter().y});
                return false;
            }};
}

// Der Zustand vor dem Teil (der volle Lauf geht danach weiter)
struct Vorher {
    int potenz = 0;
    karte::Sicht sicht = karte::Sicht::XY;
    bool raster = true;
};

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto vorher = std::make_shared<Vorher>();
    add(tu("Numpad: Raster 16, Ansicht XY, Raster sichtbar", [vorher] {
        ke::Editor& e = ke::editor();
        *vorher = Vorher{e.raster.potenz(), e.xy[0].ansicht.typ, e.anzeige.raster};
        e.raster.setzePotenz(4);
        e.xy[0].ansicht.typ = karte::Sicht::XY;
        e.anzeige.raster = true;
        ++e.sichtStand;
        e.letzterBefehl.clear();
    }));
    add(pause(0.3));
    // --- Punkt 1: jede Ziffer des Ziffernblocks setzt das Raster der gleichen Ziffer oben (rot bis rc611 fuer 1/3/7: Ansicht)
    for (int d = 1; d <= 9; ++d) {
        add(tasteKombi(static_cast<ImGuiKey>(ImGuiKey_Keypad0 + d), "Num " + std::to_string(d)));
        add(pruefe("Numpad Vorgabe: Num " + std::to_string(d) + " = SetGrid" + std::to_string(1 << (d - 1)) + ", Ansicht bleibt XY", [d] {
            const ke::Editor& e = ke::editor();
            diag::detail("Knopftest: Numpad - Raster " + std::to_string(e.raster.groesse()) + ", Ansicht " +
                         std::to_string(static_cast<int>(e.xy[0].ansicht.typ)) + ", zuletzt " + e.letzterBefehl);
            return e.raster.groesse() == rasterDerZiffer(d) && e.xy[0].ansicht.typ == karte::Sicht::XY;
        }));
    }
    add(tasteKombi(ImGuiKey_Keypad0, "Num 0"));
    add(pruefe("Numpad Vorgabe: Num 0 = ToggleGrid wie die obere 0 (Raster aus)", [] {
        diag::detail("Knopftest: Numpad - zuletzt " + ke::editor().letzterBefehl);
        return !ke::editor().anzeige.raster && ke::editor().letzterBefehl == "ToggleGrid";
    }));
    add(tasteKombi(ImGuiKey_Keypad0, "Num 0"));
    add(pruefe("Numpad Vorgabe: Num 0 nochmal - Raster wieder an", [] { return ke::editor().anzeige.raster; }));

    // --- Punkt 2: der Schalter. Menue Grid: am Ende "Numpad sets the grid" (angehakt), mit Hinweis
    add(tu("Numpad: Raster 16, deutsche Tastatur", [] {
        ke::editor().raster.setzePotenz(4);
        ke::tastaturErsatz = deutscheTastatur;
    }));
    add(klick(ke::trK(Str::KeLGrid), "###main"));
    add(pause(0.3));
    add(schweben("Numpad: Maus auf Grid > Numpad sets the grid", ke::trK(Str::KeZbRaster), "##Menu_", false));
    add(pruefe("Numpad: das Menue Grid endet mit \"Numpad sets the grid\" (an), sein Hinweis steht ganz im Bild", [] {
        const Element* e = finde(ke::trK(Str::KeZbRaster), "##Menu_", 0, false);
        const Element* g = finde(ke::trK(Str::KeLGrid1024), "##Menu_", 0, false);
        return e != nullptr && g != nullptr && e->rect.Min.y >= g->rect.Max.y - 1.0F && ke::editor().belegung.ziffernblockRaster() && hinweisImBild();
    }));
    add(foto("numpad_menue_grid"));
    add(klick(ke::trK(Str::KeZbRaster), "##Menu_", 0, false));
    add(pause(0.3));
    add(pruefe("Numpad: Grid > Numpad sets the grid aus - wie NetRadiant-Custom, gespeichert (BhedNumpad=views)", [] {
        const auto& b = ke::editor().belegung;
        diag::detail("Knopftest: Numpad - BhedNumpad " + gespeichert("BhedNumpad") + ", ViewFront \"" + b.text("ViewFront") + "\", SetGrid1 \"" +
                     b.text("SetGrid1") + "\"");
        return !b.ziffernblockRaster() && gespeichert("BhedNumpad") == "views" && b.text("ViewFront") == "Num+1" && b.text("ViewSide") == "Num+3" &&
               b.text("ViewTop") == "Num+7" && b.text("SetGrid1") == "1" && b.text("ToggleGrid") == "0" && g_app->settings.karteTasten.empty();
    }));
    // wie NRC: Num 1 Vorne, Num 3 Seite, Num 7 Oben; das Raster bleibt, Num 5 hat keinen Befehl
    const struct {
        ImGuiKey taste;
        const char* name;
        karte::Sicht sicht;
    } kNrc[3] = {{ImGuiKey_Keypad1, "Num 1 = ViewFront (XZ)", karte::Sicht::XZ},
                 {ImGuiKey_Keypad3, "Num 3 = ViewSide (YZ)", karte::Sicht::YZ},
                 {ImGuiKey_Keypad7, "Num 7 = ViewTop (XY)", karte::Sicht::XY}};
    for (const auto& n : kNrc) {
        add(tasteKombi(n.taste, n.name));
        const karte::Sicht soll = n.sicht;
        add(pruefe(std::string("Numpad wie NRC: ") + n.name + ", Raster bleibt 16", [soll] {
            const ke::Editor& e = ke::editor();
            return e.xy[0].ansicht.typ == soll && e.raster.groesse() == 16.0F;
        }));
    }
    add(tu("Numpad: letzten Befehl leeren", [] { ke::editor().letzterBefehl.clear(); }));
    add(tasteKombi(ImGuiKey_Keypad5, "Num 5"));
    add(pruefe("Numpad wie NRC: Num 5 hat keinen Befehl - Raster bleibt 16", [] {
        return ke::editor().raster.groesse() == 16.0F && ke::editor().letzterBefehl.empty();
    }));
    add(tasteKombi(ImGuiKey_3, "3"));
    add(pruefe("Numpad wie NRC: die obere Reihe setzt das Raster weiter (3 = Raster 4)", [] { return ke::editor().raster.groesse() == 4.0F; }));
    // im Menue stehen bei den Ansichten jetzt die Num-Tasten
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.3));
    add(klick(ke::trK(Str::KeLOrthographic), "##Menu_", 0, false));
    add(pause(0.3));
    add(pruefe("Numpad wie NRC: View > Orthographic offen (XY (Top), XZ (Front), YZ (Side))", [] {
        return finde(ke::trK(Str::KeLXYTop), "##Menu_", 0, false) != nullptr && finde(ke::trK(Str::KeLXZFront), "##Menu_", 0, false) != nullptr &&
               finde(ke::trK(Str::KeLYZSide), "##Menu_", 0, false) != nullptr;
    }));
    add(foto("numpad_menue_ansicht_nrc"));
    add(klick(ke::trK(Str::KeLXZFront), "##Menu_", 0, false));
    add(pause(0.3));
    add(pruefe("Numpad wie NRC: View > Orthographic > XZ (Front) - XZ", [] { return ke::editor().xy[0].ansicht.typ == karte::Sicht::XZ; }));
    // Preferences > Grid: derselbe Schalter mit Hinweis; OK stellt die Vorgabe wieder her
    add(tu("Numpad: Preferences oeffnen", [] { ke::editor().einstellungenOffen = true; }));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KePgGrid)) + "##seite15"));
    add(pause(0.3));
    add(schweben("Numpad: Maus auf Preferences > Grid > Numpad sets the grid", ke::trK(Str::KeZbRaster), "###karteeinstellungen", true));
    add(pruefe("Numpad: Preferences > Grid zeigt \"Numpad sets the grid\" (aus), sein Hinweis steht ganz im Bild", [] {
        return finde(ke::trK(Str::KeZbRaster), "###karteeinstellungen", 0, true) != nullptr && !ke::editor().belegung.ziffernblockRaster() &&
               hinweisImBild();
    }));
    add(foto("numpad_pref_grid"));
    add(klick(ke::trK(Str::KeZbRaster), "###karteeinstellungen", 0, true));
    add(pause(0.2));
    add(pruefe("Numpad: der Haken gilt erst mit OK (noch wie NRC)", [] { return !ke::editor().belegung.ziffernblockRaster(); }));
    add(klick(ke::trK(Str::KePrefOk), "###karteeinstellungen"));
    add(pause(0.4));
    add(pruefe("Numpad: Preferences > Grid > Numpad sets the grid an, OK - die Vorgabe ist wieder da, gespeichert (BhedNumpad=grid)", [] {
        const auto& b = ke::editor().belegung;
        return b.ziffernblockRaster() && gespeichert("BhedNumpad") == "grid" && b.text("SetGrid1") == "1 / Num+1" && b.text("ViewFront").empty() &&
               g_app->settings.karteTasten.empty();
    }));
    add(tasteKombi(ImGuiKey_Keypad7, "Num 7"));
    add(pruefe("Numpad: Num 7 setzt wieder das Raster 64, die Ansicht bleibt XZ", [] {
        return ke::editor().raster.groesse() == 64.0F && ke::editor().xy[0].ansicht.typ == karte::Sicht::XZ;
    }));
    // Tastenbelegung (Ctrl+Shift+P): die Suche nach der Taste kennt die Zweittaste
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P, "Strg+Umschalt+P"));
    add(pause(0.3));
    add(klick("##suchtaste", "###kartetasten"));
    add(tippe("Num+5"));
    add(pause(0.3));
    add(pruefe("Numpad: Shortcuts, Suche nach der Taste \"Num+5\" findet SetGrid16 (und nicht SetGrid32)", [] {
        return finde("SetGrid16##tz", "###kartetasten", 0, false) != nullptr && finde("SetGrid32##tz", "###kartetasten", 0, false) == nullptr;
    }));
    add(tu("Numpad: Dialog zu", [] { ke::editor().tastenDialogOffen = false; }));
    add(pause(0.2));
    // eine Num-Taste ist vergeben - Rueckfrage; Yes nimmt sie nur dem Raster-Befehl
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P, "Strg+Umschalt+P"));
    add(pause(0.3));
    add(klick("##suchname", "###kartetasten"));
    add(tippe("Zoom100"));
    add(pause(0.3));
    add(klick("Zoom100##tz", "###kartetasten", 0, false, "", 0, 0.15F));
    add(klick(ke::trK(Str::KeEdit), "###kartetasten"));
    add(pause(0.2));
    add(tasteKombi(ImGuiKey_Keypad5, "Num 5"));
    add(pause(0.3));
    add(pruefe("Numpad: Zoom100 auf Num 5 - Rueckfrage \"Key already used\" (Yes, No), noch nichts geaendert", [] {
        return finde(ke::trK(Str::KeNo), "", 0, false) != nullptr && finde(ke::trK(Str::KeYes), "", 0, false) != nullptr &&
               ke::editor().belegung.text("Zoom100").empty() && ke::editor().belegung.text("SetGrid16") == "5 / Num+5";
    }));
    add(foto("numpad_konflikt"));
    add(klick(ke::trK(Str::KeYes)));
    add(pause(0.2));
    add(pruefe("Numpad: Yes - Zoom100 auf Num+5, SetGrid16 behaelt \"5\"; gespeichert (Zoom100=Num+5, SetGrid16#Num=)", [] {
        const auto& b = ke::editor().belegung;
        diag::detail("Knopftest: Numpad - Zoom100 \"" + b.text("Zoom100") + "\", SetGrid16 \"" + b.text("SetGrid16") + "\", gespeichert Zoom100=" +
                     gespeicherteTaste("Zoom100") + ", SetGrid16#Num=" + gespeicherteTaste("SetGrid16#Num"));
        return b.text("Zoom100") == "Num+5" && b.text("SetGrid16") == "5" && gespeicherteTaste("Zoom100") == "Num+5" &&
               gespeicherteTaste("SetGrid16#Num").empty() && g_app->settings.karteTasten.size() == 2;
    }));
    add(tu("Numpad: Dialog zu, Raster 4", [] {
        ke::editor().tastenDialogOffen = false;
        ke::editor().raster.setzePotenz(2);
        ke::editor().letzterBefehl.clear();
    }));
    add(pause(0.2));
    add(tasteKombi(ImGuiKey_Keypad5, "Num 5"));
    add(pruefe("Numpad: Num 5 loest jetzt Zoom100 aus, das Raster bleibt 4", [] {
        return ke::editor().letzterBefehl == "Zoom100" && ke::editor().raster.groesse() == 4.0F;
    }));
    add(tasteKombi(ImGuiKey_5, "5"));
    add(pruefe("Numpad: die obere 5 setzt weiter das Raster 16", [] { return ke::editor().raster.groesse() == 16.0F; }));
    // Der Schalter mit einer eigenen Num-Taste: sie bleibt, die Vorgabe weicht ihr (nicht mehrdeutig)
    add(tu("Numpad: Schalter aus und wieder an", [] {
        ke::ziffernblockSetzen(false);
        ke::ziffernblockSetzen(true);
        ke::editor().letzterBefehl.clear();
    }));
    add(tasteKombi(ImGuiKey_Keypad5, "Num 5"));
    add(pruefe("Numpad: Schalter aus und an - Num 5 bleibt Zoom100 (nicht mehrdeutig), gespeichert wie vorher", [] {
        const ke::Editor& e = ke::editor();
        return e.letzterBefehl == "Zoom100" && e.belegung.text("SetGrid16") == "5" && gespeicherteTaste("Zoom100") == "Num+5" &&
               gespeicherteTaste("SetGrid16#Num").empty() && gespeichert("BhedNumpad") == "grid";
    }));
    add(tasteKombi(ImGuiKey_Keypad6, "Num 6"));
    add(pruefe("Numpad: Schalter aus und an - Num 6 setzt das Raster 32", [] { return ke::editor().raster.groesse() == 32.0F; }));
    // Einstellungen von vor rc612 (kein BhedNumpad, eigene Taste Zoom100=Num+5): die eigene Taste behaelt ihre Wirkung
    add(tu("Numpad: Tastenbelegung wie von rc611 laden (Zoom100=Num+5, kein BhedNumpad)", [] {
        g_app->settings.karte.erase("BhedNumpad");
        g_app->settings.karteTasten.clear();
        g_app->settings.karteTasten["Zoom100"] = "Num+5";
        ke::editor().belegung = karte::befehle::Belegung();   // wie nach dem Programmstart
        ke::belegungLaden();
        ke::editor().letzterBefehl.clear();
    }));
    add(tasteKombi(ImGuiKey_Keypad5, "Num 5"));
    add(pruefe("Numpad: Einstellungen von rc611 - Vorgabe Raster, Num 5 bleibt Zoom100", [] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Numpad - SetGrid16 \"" + e.belegung.text("SetGrid16") + "\", SetGrid4 \"" + e.belegung.text("SetGrid4") +
                     "\", ViewSide \"" + e.belegung.text("ViewSide") + "\", zuletzt " + e.letzterBefehl);
        return e.belegung.ziffernblockRaster() && e.letzterBefehl == "Zoom100" && e.belegung.text("Zoom100") == "Num+5" &&
               e.belegung.text("SetGrid16") == "5" && e.belegung.text("SetGrid4") == "3 / Num+3" && e.belegung.text("ViewSide").empty();
    }));
    add(tasteKombi(ImGuiKey_Keypad3, "Num 3"));
    add(pruefe("Numpad: Einstellungen von rc611 - Num 3 setzt das Raster 4, die Ansicht bleibt XZ", [] {
        return ke::editor().raster.groesse() == 4.0F && ke::editor().xy[0].ansicht.typ == karte::Sicht::XZ;
    }));
    add(tu("Numpad: speichern", [] { ke::einstellungenSpeichern(); }));
    add(pruefe("Numpad: danach gespeichert BhedNumpad=grid, Zoom100=Num+5, SetGrid16#Num=", [] {
        return gespeichert("BhedNumpad") == "grid" && gespeicherteTaste("Zoom100") == "Num+5" && gespeicherteTaste("SetGrid16#Num").empty() &&
               g_app->settings.karteTasten.size() == 2;
    }));
    // Clear leert beide Tasten; Reset holt beide zurueck und fragt, weil Num+5 vergeben ist; Yes nimmt sie Zoom100
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P, "Strg+Umschalt+P"));
    add(pause(0.3));
    add(klick("##suchname", "###kartetasten"));
    add(tippe("SetGrid16"));
    add(pause(0.3));
    add(hinweisInZeile("Numpad: Maus auf die Markierung (x) hinter der geaenderten Taste von SetGrid16", "SetGrid16##tz", "###kartetasten"));
    add(pruefe("Numpad: Shortcuts zeigt SetGrid16 als geaendert, der Hinweis mit der Vorgabe steht ganz im Bild", [] {
        return ke::editor().belegung.geaendert("SetGrid16") && !ke::editor().belegung.vorgabeWieNrc("SetGrid16") &&
               ke::editor().belegung.standardText("SetGrid16") == "5 / Num+5" && hinweisImBild();
    }));
    add(foto("numpad_tasten"));
    add(klick("SetGrid16##tz", "###kartetasten", 0, false, "", 0, 0.15F));
    add(klick(ke::trK(Str::KeClear), "###kartetasten"));
    add(pause(0.2));
    add(pruefe("Numpad: Clear - SetGrid16 ohne Taste (gespeichert SetGrid16= und SetGrid16#Num=)", [] {
        return ke::editor().belegung.text("SetGrid16").empty() && gespeicherteTaste("SetGrid16").empty() && gespeicherteTaste("SetGrid16#Num").empty();
    }));
    add(klick(ke::trK(Str::KeReset), "###kartetasten"));
    add(pause(0.3));
    add(pruefe("Numpad: Reset - Rueckfrage, weil die Vorgabe Num+5 bei Zoom100 liegt; noch nichts geaendert", [] {
        return finde(ke::trK(Str::KeNo), "", 0, false) != nullptr && finde(ke::trK(Str::KeYes), "", 0, false) != nullptr &&
               ke::editor().belegung.text("SetGrid16").empty();
    }));
    add(klick(ke::trK(Str::KeYes)));
    add(pause(0.2));
    add(pruefe("Numpad: Yes - SetGrid16 wieder \"5 / Num+5\", Zoom100 ohne Taste, nichts mehr gespeichert", [] {
        const auto& b = ke::editor().belegung;
        diag::detail("Knopftest: Numpad - SetGrid16 \"" + b.text("SetGrid16") + "\", Zoom100 \"" + b.text("Zoom100") + "\", " +
                     std::to_string(g_app->settings.karteTasten.size()) + " gespeichert");
        return b.text("SetGrid16") == "5 / Num+5" && b.text("Zoom100").empty() && b.abweichende().empty() && g_app->settings.karteTasten.empty();
    }));
    // Reset All stellt auch die Zweittasten her
    add(tu("Numpad: Num 0 von ToggleGrid loesen, NextView auf Q", [] {
        ke::editor().belegung.loesen("ToggleGrid", karte::befehle::kNum | '0');
        ke::editor().belegung.setzen("NextView", 'Q');
        ke::einstellungenSpeichern();
    }));
    add(klick(ke::trK(Str::KeResetAll), "###kartetasten"));
    add(pause(0.2));
    add(klick(ke::trK(Str::KeYes)));
    add(pause(0.2));
    add(pruefe("Numpad: Reset All - ToggleGrid wieder \"0 / Num+0\", NextView \"Ctrl+Tab\", nichts gespeichert", [] {
        const auto& b = ke::editor().belegung;
        return b.text("ToggleGrid") == "0 / Num+0" && b.text("NextView") == "Ctrl+Tab" && b.abweichende().empty() &&
               g_app->settings.karteTasten.empty();
    }));
    add(tu("Numpad: Dialog zu", [] { ke::editor().tastenDialogOffen = false; }));
    add(pause(0.2));

    // --- Punkt 3: die Anzeige im Menue (Grid mit den Num-Tasten, die Ansichten ohne) und die Ansichten ohne Ziffernblock
    add(pruefe("Numpad Vorgabe: Grid zeigt beide Tasten (Grid1 \"1 / Num+1\", Grid256 \"9 / Num+9\", Show Grid \"0 / Num+0\"), "
               "View > Orthographic ohne Num-Tasten, Next \"Ctrl+Tab\"", [] {
        const auto& b = ke::editor().belegung;
        diag::detail("Knopftest: Numpad - SetGrid1 \"" + b.text("SetGrid1") + "\", SetGrid256 \"" + b.text("SetGrid256") + "\", ToggleGrid \"" +
                     b.text("ToggleGrid") + "\", ViewFront \"" + b.text("ViewFront") + "\", NextView \"" + b.text("NextView") + "\"");
        return b.ziffernblockRaster() && b.text("SetGrid1") == "1 / Num+1" && b.text("SetGrid256") == "9 / Num+9" &&
               b.text("ToggleGrid") == "0 / Num+0" && b.text("ViewFront").empty() && b.text("ViewSide").empty() && b.text("ViewTop").empty() &&
               b.text("NextView") == "Ctrl+Tab";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Tab, "Strg+Tab"));
    add(pruefe("Numpad Vorgabe: Strg+Tab wechselt die Ansicht weiter (XZ -> YZ)", [] { return ke::editor().xy[0].ansicht.typ == karte::Sicht::YZ; }));
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.3));
    add(klick(ke::trK(Str::KeLOrthographic), "##Menu_", 0, false));
    add(pause(0.3));
    add(foto("numpad_menue_ansicht"));
    add(klick(ke::trK(Str::KeLXYTop), "##Menu_", 0, false));
    add(pause(0.3));
    add(pruefe("Numpad Vorgabe: View > Orthographic > XY (Top) - wieder XY", [] { return ke::editor().xy[0].ansicht.typ == karte::Sicht::XY; }));
    add(tu("Numpad: Zustand wie vorher, echte Tastatur", [vorher] {
        ke::Editor& e = ke::editor();
        e.raster.setzePotenz(vorher->potenz);
        e.xy[0].ansicht.typ = vorher->sicht;
        e.anzeige.raster = vorher->raster;
        ++e.sichtStand;
        ke::tastaturErsatz = nullptr;
    }));
    return s;
}

}  // namespace numpad
