// selbsttest_karte1bis2.inc - aus gui/selbsttest.cpp geschnitten (Stufe 8d Punkt 5, unveraendert): Karten-Editor Stufe 1b und 2
//
// Eingebunden an genau dieser Stelle von gui/selbsttest.cpp (im selben Namensraum, mit allen Helfern davor);
// keine eigene Uebersetzungseinheit - der vorverarbeitete Text ist derselbe wie vor dem Schnitt.
// Abschnitte: Karten-Editor (Stufe 1b, BHED_EDITORTEST=karteneditor); Punkt 11: jeder nachgebaute Befehl, Fotos zum Vergleich mit NRC; Stufe 2 des Karten-Editors: die Bauwerkzeuge

// --- Karten-Editor (Stufe 1b, BHED_EDITORTEST=karteneditor) ---------------------
//
// Der Arbeitsbereich nach NetRadiant-Custom mit echten Klicks: Modusleiste,
// File-Menue (New Map, zuletzt geoeffnet), Rueckfrage bei geaenderter Karte
// (ConfirmModified: Cancel, Discard), Oeffnen und Speichern unter mit
// bytegleichem Ergebnis. Dateidialoge kommen im Test nicht vor: Oeffnen und
// Speichern unter laufen ueber die Funktionen hinter dem Dialog.
std::string karteTestPfad(const char* name) { return (std::filesystem::current_path() / name).string(); }

std::string dateiInhalt(const std::string& pfad) {
    std::ifstream f(std::filesystem::path(pfad), std::ios::binary);
    std::ostringstream o;
    o << f.rdbuf();
    return o.str();
}

// Punkt 3: Rahmen wie NRC - jedes Menue mit allen Eintraegen, die
// Werkzeugleisten mit NRCs standardmaessig versteckten Knoepfen, das
// Kontextmenue der Leisten, die vier Fensteranordnungen ueber die
// Einstellungen, GUI- und Viewports-Thema, "Begriffe wie NetRadiant".
// Das Kindfenster des Karten-Editors ("...###main/##karteneditor_XXXX").
ImGuiWindow* karteFenster() {
    for (ImGuiWindow* k : GImGui->Windows) {
        if (std::strstr(k->Name, "/##karteneditor_") != nullptr) { return k; }
    }
    return nullptr;
}

// Was ein Fenster zeichnet, liegt in seinem Rechteck (2 Punkte Rand fuer die
// Kantenglaettung). Findet z. B. Umrisse, deren Flags als Dicke ankamen (ImGui
// 1.92.8 tauschte bei AddRect thickness und flags). namensTeil: das Ende des
// ImGui-Namens bis vor die Kennung ("/##gruppeinnen_") - nur das Fenster
// selbst, nicht seine Kinder.
bool zeichnungImFenster(const char* namensTeil, std::string* bericht) {
    for (ImGuiWindow* w : GImGui->Windows) {
        const char* wo = std::strstr(w->Name, namensTeil);
        if (!(w->Active || w->WasActive) || wo == nullptr || std::strchr(wo + std::strlen(namensTeil), '/') != nullptr) { continue; }
        ImRect aussen = w->Rect();
        aussen.Expand(2.0F);
        int raus = 0;
        ImVec2 weitester = aussen.GetCenter();
        float abstand = 0.0F;
        for (const ImDrawVert& v : w->DrawList->VtxBuffer) {
            if (aussen.Contains(v.pos)) { continue; }
            ++raus;
            const float d = std::max({aussen.Min.x - v.pos.x, v.pos.x - aussen.Max.x, aussen.Min.y - v.pos.y, v.pos.y - aussen.Max.y});
            if (d > abstand) {
                abstand = d;
                weitester = v.pos;
            }
        }
        if (bericht != nullptr) {
            char z[200];
            std::snprintf(z, sizeof(z), "%d Punkte, %d ausserhalb (am weitesten %.0f/%.0f, %.0f Pixel draussen)", w->DrawList->VtxBuffer.Size,
                          raus, weitester.x, weitester.y, abstand);
            *bericht = z;
        }
        return w->DrawList->VtxBuffer.Size > 0 && raus == 0;
    }
    if (bericht != nullptr) { *bericht = "kein Fenster"; }
    return false;
}

// Ein Rechteck des Karten-Editors aus dem letzten Bild (logische Punkte).
const karteneditor::Editor::Rechteck* karteFeld(const std::string& name) {
    for (const auto& r : karteneditor::editor().felderImBild) {
        if (r.name == name) { return &r; }
    }
    return nullptr;
}

std::vector<Schritt> karteneditorRahmenSchritte() {
    namespace ke = karteneditor;
    namespace kb = karte::befehle;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    // Offene Menues schliessen: ein Klick in die Statuszeile des Editors
    // (ImGui schliesst mit der Maus geoeffnete Menues nicht mit Escape).
    auto menuesZu = [] {
        return klickAn("in die Statuszeile (Menues zu)", [] {
            ImGuiWindow* w = karteFenster();
            if (w == nullptr) { return ImVec2{5, 5}; }
            // Stufe 9: ganz links - das Build-Menue reicht mit "Test in game" bis an die Statuszeile
            return ImVec2{w->Pos.x + 12.0F, w->Pos.y + w->Size.y - ImGui::GetTextLineHeight() * 0.5F - 2.0F};
        });
    };
    // 1. Jedes Menue oeffnen; alle Eintraege der ersten Ebene muessen da sein,
    //    jedes Untermenue wird geoeffnet und seine Eintraege geprueft.
    const auto& liste = kb::menues();
    for (std::size_t i = 0; i < liste.size(); ++i) {
        if (liste[i].art != kb::MenueArt::Menue) { continue; }
        const std::string titel = ke::trK(liste[i].str);
        std::vector<std::string> punkte;
        std::vector<std::pair<std::string, std::vector<std::string>>> unter;
        int tiefe = 0;
        for (std::size_t k = i + 1; k < liste.size(); ++k) {
            const kb::MenueEintrag& m = liste[k];
            if (m.art == kb::MenueArt::Ende) {
                if (tiefe == 0) { break; }
                --tiefe;
                continue;
            }
            if (!kb::bedingungGilt(m.bedingung, kb::Anordnung::Regular, false)) {
                if (m.art == kb::MenueArt::Unter) { ++tiefe; }
                continue;
            }
            if (m.art == kb::MenueArt::Unter) {
                if (tiefe == 0) { unter.push_back({ke::trK(m.str), {}}); }
                ++tiefe;
                continue;
            }
            if (m.art != kb::MenueArt::Punkt && m.art != kb::MenueArt::Haken && m.art != kb::MenueArt::Ruf) { continue; }
            if (tiefe == 0) {
                punkte.emplace_back(ke::trK(m.str));
            } else if (tiefe == 1 && !unter.empty()) {
                unter.back().second.emplace_back(ke::trK(m.str));
            }
        }
        add(klick(titel, "###main"));
        add(pause(0.2));
        add(pruefe("Menue " + titel + ": " + std::to_string(punkte.size()) + " Eintraege da", [punkte, titel] {
            std::string fehlt;
            for (const std::string& p : punkte) {
                if (finde(p, "", 0, false) == nullptr) { fehlt += " \"" + p + "\""; }
            }
            if (!fehlt.empty()) { diag::detail("Knopftest: Menue " + titel + " - es fehlt" + fehlt); }
            return fehlt.empty();
        }));
        for (const auto& [name, eintraege] : unter) {
            add(klick(name, "", 0, false));
            add(pause(0.2));
            const std::string un = name;
            const std::vector<std::string> ei = eintraege;
            add(pruefe("Untermenue " + titel + " > " + un + ": " + std::to_string(ei.size()) + " Eintraege da", [ei, un] {
                std::string fehlt;
                for (const std::string& p : ei) {
                    if (finde(p, "", 0, false) == nullptr) { fehlt += " \"" + p + "\""; }
                }
                if (!fehlt.empty()) { diag::detail("Knopftest: Untermenue " + un + " - es fehlt" + fehlt); }
                return fehlt.empty();
            }));
        }
        add(menuesZu());
        add(pause(0.1));
    }
    // 1b. Plugins wie NRC 20260114 (Foto menu_Plugins): sieben Untermenues in
    //     Ladereihenfolge, Eintraege von hinten nach vorn (PlugInMenu_Add).
    add(klick(ke::trK(Str::KeLPlugins), "###main"));
    add(pause(0.2));
    add(pruefe("Plugins: MeshTex, ShaderPlug, SunPlug, UFO:AI, bobToolz, brushexport2, prtview", [] {
        float y = -1.0F;
        for (const char* n : {"MeshTex", "ShaderPlug", "SunPlug", "UFO:AI", "bobToolz", "brushexport2", "prtview"}) {
            const Element* el = finde(n, "", 0, false);
            if (el == nullptr || el->rect.Min.y <= y) { return false; }
            y = el->rect.Min.y;
        }
        return true;
    }));
    add(klick("bobToolz", "", 0, false));
    add(pause(0.2));
    // Seit Stufe 4 Punkt 3a ist bobToolz nachgebaut (vorher ausgegraut).
    add(pruefe("Plugins > bobToolz: Flip terrain oben, About... unten, nachgebaut (Stufe 4)", [] {
        const Element* oben = finde("Flip terrain", "", 0, false);
        const Element* unten = finde("About...", "", 0, false);
        return oben != nullptr && unten != nullptr && oben->rect.Min.y < unten->rect.Min.y &&
               ke::nachgebaut("bobToolz::FlipTerrain") && ke::nachgebaut("bobToolz::About");
    }));
    add(foto("karteneditor_plugins"));
    add(menuesZu());
    add(pause(0.1));
    // 2. Werkzeugleisten: genau die Knoepfe, die NRC zeigt (gtkmisc.cpp:147).
    auto erwartet = [] {
        std::vector<std::string> v;
        for (const auto* l : {&kb::werkzeugleiste(), &kb::filterleiste(), &kb::pluginleiste()}) {
            for (const kb::Knopf& k : *l) {
                if (k.trenner || !kb::bedingungGilt(k.bedingung, ke::editor().anordnung, ke::editor().gruppenDialogEingebaut)) {
                    continue;
                }
                if (ke::editor().versteckteKnoepfe.count(k.befehl) != 0U) { continue; }
                v.emplace_back(k.befehl);
            }
        }
        return v;
    };
    add(pruefe("Werkzeugleisten: NRCs sichtbare Knoepfe (versteckt: OpenMap, SaveMap, Undo, Redo, ...)", [erwartet] {
        const auto soll = erwartet();
        const auto& ist = ke::editor().knoepfeImBild;
        // Was nicht passt, liegt wie in Qt hinter dem Erweiterungsknopf seiner
        // Leiste: gezeichnet ist eine Teilfolge der erwarteten Knoepfe.
        std::size_t j = 0;
        for (const std::string& k : soll) {
            if (j < ist.size() && ist[j] == k) { ++j; }
        }
        const bool teilfolge = j == ist.size();
        const bool erweitert = karteFeld("Erweiterung:filter") != nullptr || karteFeld("Erweiterung:plugin") != nullptr;
        diag::detail("Knopftest: Karten-Editor - " + std::to_string(ist.size()) + " Knoepfe gezeichnet, erwartet " +
                     std::to_string(soll.size()) + (erweitert ? " (Rest hinter Erweiterungsknoepfen)" : ""));
        return teilfolge && (ist.size() == soll.size() || erweitert) && std::find(ist.begin(), ist.end(), "OpenMap") == ist.end() &&
               std::find(ist.begin(), ist.end(), "MouseDrag") != ist.end() &&
               std::find(ist.begin(), ist.end(), "FilterWorldBrushes") != ist.end();
    }));
    // 2b. Jedes Symbol der Leisten, der Statuszeile und der Layout-Seite hat
    //     eine eigene Vektorzeichnung (shank: moderner Stil, NRCs Bedeutung) -
    //     keine fehlt, und keine zwei sehen gleich aus (ausser Entities und
    //     Patches, die NRC selbst an zwei Stellen gleich zeigt).
    add(pruefe("Knopfsymbole: alle als eigene Vektorzeichnung, keine doppelt", [] {
        std::vector<std::string> namen;
        for (const auto* l : {&kb::werkzeugleiste(), &kb::filterleiste(), &kb::texturKnoepfe(), &kb::pluginleiste()}) {
            for (const kb::Knopf& k : *l) {
                if (!k.trenner && k.bild[0] != 0) { namen.emplace_back(k.bild); }
            }
        }
        for (const char* n : {"status_brush", "status_patch", "status_entity", "window1", "window2", "window3", "window4"}) {
            namen.emplace_back(n);
        }
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        std::map<std::uint64_t, std::string> gesehen;
        std::string fehlt;
        std::string doppelt;
        const auto gleichErlaubt = [](const std::string& a, const std::string& b) {
            const auto gruppe = [](const std::string& n) {
                if (n == "entities" || n == "f-entities" || n == "status_entity") { return 1; }
                if (n == "f-patches" || n == "status_patch") { return 2; }
                return 0;
            };
            return a == b || (gruppe(a) != 0 && gruppe(a) == gruppe(b));
        };
        for (const std::string& n : namen) {
            const int vorher = dl->VtxBuffer.Size;
            const ImVec2 pos{-4000.0F, -4000.0F};
            if (!ke::karteIcon(dl, n.c_str(), pos, 32.0F, IM_COL32_WHITE)) {
                fehlt += " " + n;
                continue;
            }
            std::uint64_t h = 1469598103934665603ULL;
            for (int v = vorher; v < dl->VtxBuffer.Size; ++v) {
                const ImDrawVert& q = dl->VtxBuffer[v];
                const auto x = static_cast<std::int64_t>(std::lround((q.pos.x - pos.x) * 4.0F));
                const auto y = static_cast<std::int64_t>(std::lround((q.pos.y - pos.y) * 4.0F));
                h = (h ^ static_cast<std::uint64_t>(x * 7919 + y)) * 1099511628211ULL;
            }
            const auto [it, neu] = gesehen.emplace(h, n);
            if (!neu && !gleichErlaubt(it->second, n)) { doppelt += " " + it->second + "=" + n; }
        }
        diag::detail("Knopftest: Karten-Editor - " + std::to_string(namen.size()) + " Symbole, fehlen:" +
                     (fehlt.empty() ? std::string(" keine") : fehlt) +
                     ", doppelt:" + (doppelt.empty() ? std::string(" keine") : doppelt));
        return fehlt.empty() && doppelt.empty();
    }));
    // 3. Kontextmenue der Leisten: MouseUV verstecken und wieder zeigen.
    auto knopfOrt = [](const std::string& id) {
        return [id]() {
            const Element* el = finde(id, "", 0, true);
            return el != nullptr ? el->rect.GetCenter() : ImVec2{0, 0};
        };
    };
    add(klickAn("Rechtsklick auf den Knopf Resize", knopfOrt("##knopfMouseDrag"), 1));
    add(pause(0.2));
    add(klick("MouseUV##lm"));
    add(pause(0.2));
    add(pruefe("Kontextmenue: MouseUV versteckt, das Menue bleibt offen (gtkmisc.cpp:252 \"prevent closing the menu on clicks\")", [] {
        const auto& ist = ke::editor().knoepfeImBild;
        return ke::editor().versteckteKnoepfe.count("MouseUV") != 0U && std::find(ist.begin(), ist.end(), "MouseUV") == ist.end() &&
               finde("MouseUV##lm", "", 0, false) != nullptr;
    }));
    add(klickAn("Rechtsklick auf den Knopf Resize", knopfOrt("##knopfMouseDrag"), 1));
    add(pause(0.2));
    add(klick("MouseUV##lm"));
    add(pause(0.2));
    add(pruefe("Kontextmenue: MouseUV wieder da", [] { return ke::editor().versteckteKnoepfe.count("MouseUV") == 0U; }));
    add(taste(ImGuiKey_Escape, "Escape"));
    // 4. Die vier Fensteranordnungen ueber Edit > Preferences > Layout.
    struct Lay {
        Str name;
        kb::Anordnung a;
        const char* foto;
    };
    for (const Lay& l : {Lay{Str::KeLayoutSplit, kb::Anordnung::Split, "karteneditor_split"},
                         Lay{Str::KeLayoutFloating, kb::Anordnung::Floating, "karteneditor_floating"},
                         Lay{Str::KeLayoutRegularLeft, kb::Anordnung::RegularLeft, "karteneditor_regularleft"},
                         Lay{Str::KeLayoutRegular, kb::Anordnung::Regular, "karteneditor_regular"}}) {
        add(klick(ke::trK(Str::KeLEdit), "###main"));
        add(klick(ke::trK(Str::KeLPreferences)));
        add(pause(0.3));
        add(klick(ke::trK(Str::KePgLayout), "###karteeinstellungen"));
        add(pause(0.2));
        // Radioknoepfe ohne Text unter den Bildern (appendRadioIcons, Foto pref_Layout.png)
        add(klick("##layout" + std::to_string(static_cast<int>(l.a)), "###karteeinstellungen"));
        add(klick(ke::trK(Str::KePrefOk), "###karteeinstellungen"));
        add(pause(0.4));
        const kb::Anordnung soll = l.a;
        add(pruefe(std::string("Anordnung ") + ke::trK(l.name), [soll] {
            return ke::editor().anordnung == soll && !ke::editor().einstellungenOffen;
        }));
        add(foto(l.foto));
        if (soll == kb::Anordnung::Floating) {
            // View > Camera View / XY / XZ / YZ (nur Floating): Fenster aus und an.
            for (const auto& [str, nr] : {std::pair<Str, int>{Str::KeLCameraView, 3}, {Str::KeLXYTopView, 2},
                                          {Str::KeLXZFrontView, 1}, {Str::KeLYZSideView, 0}}) {
                for (int runde = 0; runde < 2; ++runde) {
                    add(klick(ke::trK(Str::KeLView), "###main"));
                    add(klick(ke::trK(str)));
                    add(pause(0.2));
                    const bool sichtbar = runde == 1;
                    const int n = nr;
                    add(pruefe(std::string("Floating: ") + ke::trK(str) + (sichtbar ? " wieder an" : " aus"),
                               [n, sichtbar] { return ke::editor().schwebendSichtbar[static_cast<std::size_t>(n)] == sichtbar; }));
                }
            }
        }
    }
    // Misc > Maximize view: das Feld unter der Maus (XY) fuellt alles, nochmal zurueck.
    add(klickAn("Maus ueber das XY-Feld", [] {
        ImGuiWindow* w = karteFenster();
        if (w == nullptr) { return ImVec2{0, 0}; }
        return ImVec2{w->Pos.x + w->Size.x * 0.2F, w->Pos.y + w->Size.y * 0.4F};
    }));
    add(pause(0.2));
    add(tu("MaximizeView (F12) per Befehl", [] { (void)ke::ausfuehren("MaximizeView"); }));
    add(pause(0.2));
    // mainframe.cpp:1563-1578: setSizes {9999,0} je Teiler - XY fuellt die
    // Mitte bis auf den Griff, Kamera-Spalte 0 breit, Konsole 0 hoch.
    add(pruefe("Maximize view: XY fuellt die Mitte, die anderen Felder 0, Griffe bleiben", [] {
        const auto* xy = karteFeld("XY");
        const auto* kam = karteFeld("Kamera");
        const auto* kon = karteFeld("Konsole");
        const auto* gr = karteFeld("Griff:h:1");
        const auto* fl = karteFeld("Filterleiste");
        if (xy == nullptr || kam == nullptr || kon == nullptr || gr == nullptr || fl == nullptr) { return false; }
        diag::detail("Knopftest: Karten-Editor - maximiert XY " + std::to_string(xy->b) + "x" + std::to_string(xy->h) +
                     ", Kamera " + std::to_string(kam->b) + ", Konsole " + std::to_string(kon->h));
        return ke::editor().maximiert && xy->b == fl->x - 4 && kam->b == 0 && kon->h == 0 && gr->x == xy->b;
    }));
    add(foto("karteneditor_maximiert"));
    add(klick(ke::trK(Str::KeLMisc), "###main"));
    add(klick(ke::trK(Str::KeLMaximizeView)));
    add(pause(0.2));
    add(pruefe("Maximize view ueber das Menue: wieder alle Felder", [] { return !ke::editor().maximiert; }));
    add(pruefe("Split: NextView-Knopf fehlt, Console/Texture Browser im Menue View (Bedingungen)", [] {
        return kb::bedingungGilt(kb::kNichtRegular, kb::Anordnung::Split, false) &&
               !kb::bedingungGilt(kb::kNichtSplit, kb::Anordnung::Split, false);
    }));
    // 5. Farben: aus dem Farbschema des Programms (Entscheidung shank). Die
    //    NRC-Eintraege unter Misc > Theming stehen da, ausgegraut.
    add(klick(ke::trK(Str::KeLMisc), "###main"));
    add(klick(ke::trK(Str::KeLTheming)));
    add(pause(0.2));
    add(klick(ke::trK(Str::KeLGUITheme)));
    add(pause(0.2));
    add(pruefe("Misc > Theming > GUI Theme: NRC 20260114 hat vier Themen (Default, Fusion, Dark, Darker)", [] {
        return finde("Darker", "", 0, false) != nullptr && finde("Fusion", "", 0, false) != nullptr &&
               finde("BlenderLight", "", 0, false) == nullptr;
    }));
    add(menuesZu());
    auto themaVorher = std::make_shared<std::string>();
    for (const char* id : {"dark", "light"}) {
        const std::string tid = id;
        add(tu(std::string("Farbschema des Programms: ") + id, [tid, themaVorher] {
            if (themaVorher->empty() && g_app->activeTheme != nullptr) { *themaVorher = g_app->activeTheme->id; }
            if (const theme::Theme* t = theme::findTheme(tid)) {   // wie setThemeById (app.cpp)
                g_app->activeTheme = t;
                g_app->themePending = true;
            }
        }));
        add(pause(0.4));
        add(pruefe(std::string("Farbrollen folgen dem Farbschema ") + id, [tid] {
            const ke::Farben& f = ke::editor().farben;
            const ImVec4 hinten = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg).w > 0.5F ? ImGui::GetStyleColorVec4(ImGuiCol_ChildBg)
                                                                                     : ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
            const float hell = f.rasterHinten[0] + f.rasterHinten[1] + f.rasterHinten[2];
            return std::fabs(f.rasterHinten[0] - hinten.x) < 1e-4F && (tid == "dark" ? hell < 1.5F : hell > 1.5F);
        }));
        add(foto(std::string("karteneditor_thema_") + id));
    }
    add(tu("Farbschema zurueck", [themaVorher] {
        if (const theme::Theme* t = theme::findTheme(*themaVorher)) {
            g_app->activeTheme = t;
            g_app->themePending = true;
        }
    }));
    // 6. "Begriffe wie NetRadiant": auf Deutsch heisst das Menue "Datei",
    //    eingeschaltet wieder "File".
    auto sprache = std::make_shared<i18n::Language>(i18n::Language::English);
    add(tu("Sprache Deutsch", [sprache] {
        *sprache = i18n::currentLanguage();
        i18n::setLanguage(i18n::Language::German);
        ke::setzeNrcBegriffe(false);
    }));
    add(pause(0.3));
    add(pruefe("Deutsch: Menue \"Datei\", kein \"File\"", [] {
        return finde("Datei", "###main", 0, false) != nullptr && finde("File", "###main", 0, false) == nullptr;
    }));
    add(klick("Bearbeiten", "###main"));
    add(klick("Einstellungen..."));
    add(pause(0.3));
    add(klick("Oberfläche", "###karteeinstellungen"));
    add(pause(0.2));
    add(klick("Begriffe wie NetRadiant (Englisch)", "###karteeinstellungen", 0, false, "", 0, 0.05F));
    add(klick("OK", "###karteeinstellungen"));
    add(pause(0.3));
    add(pruefe("Begriffe wie NetRadiant: Menue \"File\", \"Edit\" trotz Deutsch", [] {
        return ke::nrcBegriffe() && finde("File", "###main", 0, false) != nullptr &&
               finde("Edit", "###main", 0, false) != nullptr && finde("Datei", "###main", 0, false) == nullptr;
    }));
    add(foto("karteneditor_nrcbegriffe"));
    add(tu("Sprache zurueck, Begriffe aus", [sprache] {
        i18n::setLanguage(*sprache);
        ke::setzeNrcBegriffe(false);
    }));
    return s;
}

// Punkt 4: Tastenkuerzel wie NRC/Qt mit einer deutschen Tastatur (feste
// Belegung statt der von Windows): "[" ist AltGr+8, "]" AltGr+9, Strg+AltGr+8
// bleibt "[" (wie in NRC), Umschalt+AltGr+8 ist "Shift+["; Kuerzel nur im
// Karten-Editor und nicht bei gedrueckter Maustaste; der Dialog "Mapped
// Commands" (Ctrl+Shift+P): suchen, Edit mit neuer Taste, Konflikt mit No
// (doppelt = mehrdeutig, nichts geschieht), Reset, Reset All.
bool deutscheTastatur(int k, karte::befehle::TastenBelegung& out, bool& ziffernblock, bool& erweitert) {
    ziffernblock = false;
    erweitert = false;
    out = karte::befehle::TastenBelegung{};
    // Zeichen ohne, mit Umschalt und mit AltGr (Strg+Alt) - deutsche Belegung.
    static const char* const kZiffern[10][3] = {{"0", "=", "}"}, {"1", "!", ""}, {"2", "\"", "\xB2"}, {"3", "\xA7", "\xB3"},
                                                {"4", "$", ""},  {"5", "%", ""}, {"6", "&", ""},     {"7", "/", "{"},
                                                {"8", "(", "["}, {"9", ")", "]"}};
    if (k >= ImGuiKey_0 && k <= ImGuiKey_9) {
        const auto& z = kZiffern[k - ImGuiKey_0];
        const auto c = [](const char* t) { return static_cast<std::uint32_t>(static_cast<unsigned char>(t[0])); };
        const std::uint32_t d = c(z[0]);
        const std::uint32_t sh = c(z[1]);
        const std::uint32_t ag = z[2][0] != 0 ? c(z[2]) : sh;   // AltGr ohne Zeichen: wie Alt+Umschalt? - Qt nimmt dann den Rueckfall
        out.qtKey = {d, sh, d, sh, d, sh, z[2][0] != 0 ? ag : d, sh, d};
        out.vorhanden = true;
        return true;
    }
    if (k >= ImGuiKey_A && k <= ImGuiKey_Z) {
        const auto l = static_cast<std::uint32_t>('A' + (k - ImGuiKey_A));
        out.qtKey = {l, l, l, l, l, l, (k == ImGuiKey_Q) ? std::uint32_t{'@'} : l, l, l};
        out.vorhanden = true;
        return true;
    }
    return false;
}

std::vector<Schritt> karteneditorTastenSchritte() {
    namespace ke = karteneditor;
    namespace kb = karte::befehle;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("Deutsche Tastatur einsetzen, Raster 16", [] {
        ke::tastaturErsatz = deutscheTastatur;
        ke::editor().raster.setzePotenz(4);
        ke::editor().letzterBefehl.clear();
    }));
    auto raster = [](float soll, const std::string& was) {
        return pruefe(was, [soll] {
            diag::detail("Knopftest: Karten-Editor - Raster " + std::to_string(ke::editor().raster.groesse()) + ", zuletzt " +
                         ke::editor().letzterBefehl);
            return ke::editor().raster.groesse() == soll;
        });
    };
    add(tasteKombi(ImGuiKey_3, "3"));
    add(raster(4.0F, "Taste 3 = SetGrid4: Raster 4"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_9, "AltGr+9"));
    add(raster(8.0F, "AltGr+9 = \"]\" = GridUp: Raster 8"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_8, "AltGr+8"));
    add(raster(4.0F, "AltGr+8 = \"[\" = GridDown: Raster 4"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_8, "Strg+AltGr+8"));
    add(raster(2.0F, "Strg+AltGr+8 bleibt \"[\" (Strg+[ ist auf deutscher Tastatur nicht erreichbar, wie NRC): Raster 2"));
    add(tu("zuletzt vergessen", [] { ke::editor().letzterBefehl.clear(); }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiMod_Shift | ImGuiKey_8, "Umschalt+AltGr+8"));
    add(pruefe("Umschalt+AltGr+8 = \"Shift+[\" = CameraModePrev (Stufe 1b, Punkt 6): Raster bleibt 2", [] {
        return ke::editor().raster.groesse() == 2.0F && ke::editor().statusBefehl == "CameraModePrev";
    }));
    add(tasteKombi(ImGuiKey_5, "5"));
    add(raster(16.0F, "Taste 5 = SetGrid16"));
    add(tu("Modus Events", [] { g_app->leftMode = 0; }));
    add(pause(0.2));
    add(tasteKombi(ImGuiKey_3, "3 im Modus Events"));
    add(raster(16.0F, "Kuerzel gelten nur im Karten-Editor: Raster bleibt 16"));
    add(tu("zurueck in den Karten-Editor", [] { g_app->leftMode = 3; }));
    add(pause(0.3));
    // Gedrueckte Maustaste: keine Kuerzel (RadiantQMainWindow::event)
    add({"Taste 3 bei gedrueckter rechter Maustaste", [](int b) {
             ImGuiIO& io = ImGui::GetIO();
             if (b == 0) {
                 ImGuiWindow* w = karteFenster();
                 if (w != nullptr) { setzeMaus(ImVec2{w->Pos.x + w->Size.x * 0.3F, w->Pos.y + w->Size.y * 0.5F}); }
             }
             if (b == 1) { io.AddMouseButtonEvent(2, true); }
             if (b == 3) { io.AddKeyEvent(ImGuiKey_3, true); }
             if (b == 4) { io.AddKeyEvent(ImGuiKey_3, false); }
             if (b == 6) { io.AddMouseButtonEvent(2, false); }
             return b >= 9;
         }});
    add(raster(16.0F, "bei gedrueckter Maustaste kein Kuerzel: Raster bleibt 16"));
    add(tasteKombi(ImGuiKey_P, "P"));
    add(pruefe("P = Preferences", [] { return ke::editor().einstellungenOffen; }));
    add(tu("Einstellungen zu", [] { ke::editor().einstellungenOffen = false; }));
    add(pause(0.2));
    add(tasteKombi(ImGuiKey_N, "N"));
    add(pruefe("N = ToggleEntityInspector: Gruppendialog auf Entities", [] {
        return ke::editor().gruppenDialogOffen && ke::editor().gruppenSeite == 0;
    }));
    add(tasteKombi(ImGuiKey_N, "N"));
    add(pruefe("N nochmal: Gruppendialog zu (GroupDialog_showPage)", [] { return !ke::editor().gruppenDialogOffen; }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P, "Strg+Umschalt+P"));
    add(pause(0.3));
    add(pruefe("Ctrl+Shift+P = Shortcuts: Mapped Commands offen", [] { return ke::editor().tastenDialogOffen; }));
    add(foto("karteneditor_tasten"));
    // Wie das Foto von NRC 20260114: Plugin-Befehle stehen in der Liste,
    // ohne Ruecksicht auf Gross-/Kleinschreibung einsortiert.
    add(pruefe("Mapped Commands: nach AutoCaulkSelected folgen bobToolz::About, bobToolz::ApertureDoor (wie NRC)", [] {
        const Element* a = finde("AutoCaulkSelected##tz", "###kartetasten", 0, false);
        const Element* b = finde("bobToolz::About##tz", "###kartetasten", 0, false);
        const Element* c = finde("bobToolz::ApertureDoor##tz", "###kartetasten", 0, false);
        diag::detail(std::string("Knopftest: Karten-Editor - Zeilen ") + (a ? "A" : "-") + (b ? "B" : "-") + (c ? "C" : "-"));
        return a != nullptr && b != nullptr && c != nullptr && a->rect.Min.y < b->rect.Min.y && b->rect.Min.y < c->rect.Min.y;
    }));
    // Suchen und neue Taste
    add(klick("##suchname", "###kartetasten"));
    add(tippe("GridUp"));
    add(pause(0.3));
    add(klick("GridUp##tz", "###kartetasten", 0, false, "", 0, 0.15F));
    add(klick(ke::trK(Str::KeEdit), "###kartetasten"));
    add(pause(0.2));
    add(pruefe("Edit: nimmt eine Taste auf", [] { return ke::editor().tastenDialogBearbeitet == "GridUp"; }));
    add(tasteKombi(ImGuiKey_F7, "F7"));
    add(pause(0.2));
    add(pruefe("GridUp liegt auf F7, gespeichert", [] {
        const auto it = g_app->settings.karteTasten.find("GridUp");
        return ke::editor().belegung.taste("GridUp") == kb::kKeyF1 + 6 && it != g_app->settings.karteTasten.end() &&
               it->second == "F7";
    }));
    add(tu("Dialog zu", [] { ke::editor().tastenDialogOffen = false; }));
    add(pause(0.2));
    add(tasteKombi(ImGuiKey_F7, "F7"));
    add(raster(32.0F, "F7 = GridUp: Raster 32"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_9, "AltGr+9"));
    add(raster(32.0F, "AltGr+9 (\"]\") tut nichts mehr: Raster bleibt 32"));
    // Konflikt: GridUp auf "1" (SetGrid1) - No laesst beide, dann mehrdeutig.
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P, "Strg+Umschalt+P"));
    add(pause(0.3));
    // DoCommandListDlg baut den Dialog neu: die Suche ist wieder leer.
    add(pruefe("Neu geoeffnet: Suchfeld leer, GridUp ausserhalb der ersten Zeilen", [] {
        const Element* g = finde("GridUp##tz", "###kartetasten", 0, false);
        return g == nullptr || !ganzSichtbar(*g);
    }));
    add(klick("##suchname", "###kartetasten"));
    add(tippe("GridUp"));
    add(pause(0.3));
    add(klick("GridUp##tz", "###kartetasten", 0, false, "", 0, 0.15F));
    add(klick(ke::trK(Str::KeEdit), "###kartetasten"));
    add(tasteKombi(ImGuiKey_1, "1"));
    add(pause(0.3));
    add(pruefe("Konflikt mit SetGrid1: Rueckfrage \"Key already used\"", [] {
        return finde(ke::trK(Str::KeNo), "", 0, false) != nullptr;
    }));
    add(klick(ke::trK(Str::KeNo)));
    add(pause(0.2));
    add(pruefe("No: beide auf \"1\"", [] {
        return ke::editor().belegung.taste("GridUp") == '1' && ke::editor().belegung.taste("SetGrid1") == '1';
    }));
    add(tu("Dialog zu", [] { ke::editor().tastenDialogOffen = false; }));
    add(pause(0.2));
    add(tasteKombi(ImGuiKey_1, "1"));
    add(raster(32.0F, "\"1\" ist mehrdeutig: nichts geschieht (QAction: Ambiguous shortcut overload)"));
    add(pruefe("Konsole meldet die mehrdeutige Taste", [] {
        for (const ke::KonsolenZeile& z : ke::editor().konsole) {
            if (z.text.find("Ambiguous shortcut overload: 1") != std::string::npos) { return true; }
        }
        return false;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P, "Strg+Umschalt+P"));
    add(pause(0.3));
    add(klick(ke::trK(Str::KeResetAll), "###kartetasten"));
    add(pause(0.2));
    add(klick(ke::trK(Str::KeYes)));
    add(pause(0.2));
    add(pruefe("Reset All: alle Vorgaben wieder da, nichts mehr gespeichert", [] {
        return ke::editor().belegung.taste("GridUp") == ']' && ke::editor().belegung.taste("SetGrid1") == '1' &&
               ke::editor().belegung.abweichende().empty() && g_app->settings.karteTasten.empty();
    }));
    add(tu("Dialog zu, echte Tastatur", [] {
        ke::editor().tastenDialogOffen = false;
        ke::tastaturErsatz = nullptr;
    }));
    return s;
}

// Feldgroessen wie NRC: bei Massstab 1 und dem Arbeitsbereich so gross wie
// NRCs Client (1280 x 747 logische Punkte = das Foto bei 2560 x 1494 / 2)
// muessen alle Felder auf den Punkt so liegen wie im Foto von NRC 20260114
// (SP\nrc_fotos\dpi\radiant_dpi_layout_<n>_0.png, gemessen; Statuszeile mit
// leeren Feldern und "G:16  F:BP  C:14  L:OFF  " wie dort).
struct SollFeld {
    const char* name;
    int x;
    int y;
    int b;
    int h;
};

bool feldVergleich(const std::string& was, const std::vector<SollFeld>& soll) {
    std::string bericht;
    bool ok = true;
    for (const SollFeld& f : soll) {
        const auto* r = karteFeld(f.name);
        char z[200];
        if (r == nullptr) {
            std::snprintf(z, sizeof(z), " %s fehlt;", f.name);
            ok = false;
        } else {
            const bool gleich = r->x == f.x && r->y == f.y && r->b == f.b && r->h == f.h;
            ok = ok && gleich;
            std::snprintf(z, sizeof(z), " %s %d,%d %dx%d%s;", f.name, r->x, r->y, r->b, r->h,
                          gleich ? "" : (" SOLL " + std::to_string(f.x) + "," + std::to_string(f.y) + " " + std::to_string(f.b) +
                                         "x" + std::to_string(f.h)).c_str());
        }
        bericht += z;
    }
    diag::detail("Knopftest: Karten-Editor Masse " + was + ":" + bericht);
    return ok;
}

std::vector<Schritt> karteneditorMassSchritte() {
    namespace ke = karteneditor;
    namespace kb = karte::befehle;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto format = std::make_shared<karte::PinselFormat>(karte::PinselFormat::Quake3BP);
    auto nrc = std::make_shared<bool>(false);
    add(tu("Massstab 1, Arbeitsbereich 1280 x 747, Statuszeile wie im Foto, NRC-Begriffe", [format, nrc] {
        ke::Editor& e = ke::editor();
        *nrc = ke::nrcBegriffe();
        ke::setzeNrcBegriffe(true);
        diag::detail("Knopftest: Karten-Editor - dpiScale " + std::to_string(g_app->dpiScale) + ", uiScale " +
                     std::to_string(g_app->uiScale) + ", massstab() " + std::to_string(ke::massstab()));
        {
            ImGui::PushFont(ke::schrift(), 16.0F / (g_app->dpiScale * g_app->uiScale));
            diag::detail(std::string("Knopftest: Karten-Editor - Schrift ") +
                         (ke::schrift() != nullptr ? ke::schrift()->GetDebugName() : "(keine)") + ", Groesse " +
                         std::to_string(ImGui::GetFontSize()) + ", Edit " + std::to_string(ImGui::CalcTextSize("Edit").x) +
                         ", File " + std::to_string(ImGui::CalcTextSize("File").x) + ", Raster " +
                         std::to_string(ImGui::CalcTextSize("G:16  F:BP  C:14  L:OFF  ").x));
            ImGui::PopFont();
        }
        ke::massstabErsatz = 1.0F;
        ke::bereichErsatzB = 1280;
        ke::bereichErsatzH = 747;
        *format = e.karte.kontext.format;
        e.karte.kontext.format = karte::PinselFormat::Quake3BP;
        e.raster.setzePotenz(4);
        e.statusBefehl.clear();
        e.statusPosition.clear();
        e.statusTextur.clear();
        e.hGroessen.clear();
        e.vGroessen.clear();
        e.v2Groessen.clear();
        e.hauptleiste = true;
        e.filterleiste = true;
        e.gruppenDialogEingebaut = false;
        e.versteckteKnoepfe.clear();
        for (const std::string& n : kb::standardVersteckt()) { e.versteckteKnoepfe.insert(n); }
        e.anordnung = kb::Anordnung::Regular;
        e.teilerFuer = -1;
    }));
    add(pause(0.3));
    // Gemeinsam: Menue 21, Werkzeugleiste 39 (erster Knopf 13/25, 31 gross),
    // Filterleiste rechts 39, Statuszeile 21 mit Qts Verteilung.
    const std::vector<SollFeld> rahmen = {
        {"Bereich", 0, 0, 1280, 747},       {"Menue:File", 6, 0, 26, 21},      {"Menue:Edit", 38, 0, 29, 21},
        {"Menue:View", 73, 0, 34, 21},       {"Menue:Help", 468, 0, 33, 21},
        {"Leiste:filter", 1241, 60, 39, 606}, {"Knopf:ResetFilters", 1245, 593, 31, 31},
        {"Erweiterung:filter", 1245, 651, 31, 12}, {"Leiste:plugin", 1241, 666, 39, 60},
        {"Knopf:bobToolz::PolygonBuilder", 1245, 679, 31, 31}, {"Erweiterung:plugin", 1245, 711, 31, 12},
        {"Werkzeugleiste", 0, 21, 1280, 39}, {"Knopf:MirrorSelectionHorizontally", 13, 25, 31, 31},   {"Knopf:CSGSubtract", 145, 25, 31, 31},
        {"Knopf:MouseDrag", 378, 25, 31, 31}, {"Filterleiste", 1241, 60, 39, 606}, {"Knopf:FilterWorldBrushes", 1245, 73, 31, 31},
        {"Statuszeile", 0, 726, 1280, 21},  {"Status:Befehl", 2, 729, 320, 16}, {"Status:Position", 328, 729, 321, 16},
        {"Status:Zaehler", 655, 729, 168, 16}, {"Status:Textur", 829, 729, 320, 16}, {"Status:Raster", 1155, 729, 125, 16},
    };
    add(pruefe("Masse Rahmen: Menue, Werkzeugleiste, Filterleiste, Statuszeile wie NRC", [rahmen] {
        return feldVergleich("Rahmen", rahmen);
    }));
    add(pruefe("Masse Knoepfe: alle der Hauptleiste, Filter bis Reset, Plugin nur Polygon (Rest hinter Erweiterung)", [] {
        const auto& ist = ke::editor().knoepfeImBild;
        const auto da = [&ist](const char* n) { return std::find(ist.begin(), ist.end(), n) != ist.end(); };
        diag::detail("Knopftest: Karten-Editor - " + std::to_string(ist.size()) + " Knoepfe gezeichnet");
        return da("ResetFilters") && !da("RegionSetSelection") && !da("HideSelected") && da("bobToolz::PolygonBuilder") &&
               !da("bobToolz::TreePlanter") && da("MouseDrag") && da("FilterTriggers");
    }));
    add(pruefe("Masse Regular: XY 495 x 629, Konsole 33, Kamera 742 x 414, Texturen 248, Griffe 4", [] {
        return feldVergleich("Regular", {{"XY", 0, 60, 495, 629},
                                         {"Griff:v:1", 0, 689, 495, 4},
                                         {"Konsole", 0, 693, 495, 33},
                                         {"Griff:h:1", 495, 60, 4, 666},
                                         {"Kamera", 499, 60, 742, 414},
                                         {"Griff:v2:1", 499, 474, 742, 4},
                                         {"Texturen", 499, 478, 742, 248}});
    }));
    add(foto("karteneditor_mass_regular"));
    add(tu("Regular Left", [] {
        ke::editor().anordnung = kb::Anordnung::RegularLeft;
        ke::editor().teilerFuer = -1;
    }));
    add(pause(0.3));
    add(pruefe("Masse Regular Left: Kamera-Spalte links 495, XY-Spalte rechts 742", [] {
        return feldVergleich("RegularLeft", {{"Kamera", 0, 60, 495, 414},
                                             {"Griff:v2:1", 0, 474, 495, 4},
                                             {"Texturen", 0, 478, 495, 248},
                                             {"Griff:h:1", 495, 60, 4, 666},
                                             {"XY", 499, 60, 742, 629},
                                             {"Griff:v:1", 499, 689, 742, 4},
                                             {"Konsole", 499, 693, 742, 33}});
    }));
    add(foto("karteneditor_mass_regularleft"));
    add(tu("Split", [] {
        ke::editor().anordnung = kb::Anordnung::Split;
        ke::editor().teilerFuer = -1;
    }));
    add(pause(0.3));
    add(pruefe("Masse Split: vier gleiche Felder 619/618 x 331 (keine Groessen gesetzt)", [] {
        return feldVergleich("Split", {{"Kamera", 0, 60, 619, 331},
                                       {"Griff:v:1", 0, 391, 619, 4},
                                       {"YZ", 0, 395, 619, 331},
                                       {"Griff:h:1", 619, 60, 4, 666},
                                       {"XY", 623, 60, 618, 331},
                                       {"Griff:v2:1", 623, 391, 618, 4},
                                       {"XZ", 623, 395, 618, 331}});
    }));
    add(foto("karteneditor_mass_split"));
    add(tu("Floating", [] {
        ke::Editor& e = ke::editor();
        e.anordnung = kb::Anordnung::Floating;
        e.teilerFuer = -1;
        e.schwebendSichtbar = {true, true, true, true};
        e.schwebendZuruecksetzen = true;
    }));
    add(pause(0.4));
    add(pruefe("Masse Floating: Mitte leer, vier Fenster mit Innenmass 400 x 300", [] {
        // Innenmass 400 x 300 (addWindow); die Lage des Rahmens 50/100,
        // 500/100, 500/450, 50/450 - darunter die Titelleiste von ImGui statt
        // der von Windows (Innenecke daher um deren Hoehe tiefer).
        bool ok = feldVergleich("Floating", {{"Mitte", 0, 60, 1241, 666}});
        const int titel = static_cast<int>(std::lround(ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F));
        (void)titel;
        std::string bericht;
        const struct {
            const char* n;
            int x;
            int y;
        } soll[] = {{"Fenster:Kamera", 50, 100}, {"Fenster:XY", 500, 100}, {"Fenster:XZ", 500, 450}, {"Fenster:YZ", 50, 450}};
        for (const auto& f : soll) {
            const auto* r = karteFeld(f.n);
            if (r == nullptr) {
                ok = false;
                bericht += std::string(" ") + f.n + " fehlt;";
                continue;
            }
            bericht += " " + std::string(f.n) + " " + std::to_string(r->x) + "," + std::to_string(r->y) + " " + std::to_string(r->b) +
                       "x" + std::to_string(r->h) + ";";
            ok = ok && r->b == 400 && r->h == 300 && r->x == f.x && r->y > f.y && r->y < f.y + 40;
        }
        diag::detail("Knopftest: Karten-Editor Masse Floating-Fenster:" + bericht);
        return ok;
    }));
    add(foto("karteneditor_mass_floating"));
    add(tu("Massstab und Bereich wie vorher, Regular", [format, nrc] {
        ke::Editor& e = ke::editor();
        ke::setzeNrcBegriffe(*nrc);
        ke::massstabErsatz = 0.0F;
        ke::bereichErsatzB = 0;
        ke::bereichErsatzH = 0;
        e.karte.kontext.format = *format;
        e.anordnung = kb::Anordnung::Regular;
        e.teilerFuer = -1;
    }));
    add(pause(0.3));
    // Beim echten Massstab (150 % -> Qt 2): Werkzeugleiste 78 Bildpunkte
    // hoch, Knoepfe 62 - wie NRC auf demselben Bildschirm.
    add(pruefe("echter Massstab: Werkzeugleiste und Knoepfe in Bildpunkten wie NRC (Qt rundet den Bildschirmmassstab)", [] {
        const float s = ke::massstab();
        const float qt = std::max(1.0F, std::round(g_app->dpiScale));
        const auto* l = karteFeld("Werkzeugleiste");
        const auto* b = karteFeld("Bereich");
        const auto* xy = karteFeld("XY");
        diag::detail("Knopftest: Karten-Editor - massstab " + std::to_string(s) + ", Werkzeugleiste " +
                     std::to_string(l != nullptr ? static_cast<float>(l->h) * s : 0.0F) + " Bildpunkte, Bereich " +
                     (b != nullptr ? std::to_string(b->b) + "x" + std::to_string(b->h) : std::string("-")) + ", XY " +
                     (xy != nullptr ? std::to_string(xy->b) + "x" + std::to_string(xy->h) : std::string("-")));
        // Stufe 3 Punkt 0b: alle Felder beim echten Massstab ins Protokoll (Tabelle
        // in NOTIZEN_karte3.md, Fenster des Anwenders etwa 2559 x 1521).
        std::string alle;
        for (const auto& r : ke::editor().felderImBild) {
            if (r.name.rfind("Knopf:", 0) == 0 || r.name.rfind("Menue:", 0) == 0 || r.name.rfind("Status:", 0) == 0) { continue; }
            alle += " " + r.name + "=" + std::to_string(r.x) + "," + std::to_string(r.y) + "," + std::to_string(r.b) + "x" + std::to_string(r.h);
        }
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        diag::detail("Knopftest: Karten-Editor echte Felder (Fenster " + std::to_string(static_cast<int>(vp->Size.x)) + "x" +
                     std::to_string(static_cast<int>(vp->Size.y)) + "):" + alle);
        return l != nullptr && l->h == 39 && s == qt * g_app->uiScale && b != nullptr && b->h > 300 && xy != nullptr &&
               xy->h > 200;
    }));
    add(foto("karteneditor_mass_echt"));
    return s;
}

// Punkt 5: die 2D-Ansicht mit echten Ereignissen (Maus, Rad, Tasten). Die
// Testkarte: worldspawn mit einem Wuerfel -64..64 x -64..64 x 0..64, ein
// Licht bei 0 0 128. Sollwerte aus xywindow.cpp (siehe bhed/karte/xyansicht.h).
std::vector<Schritt> karteneditorXySchritte() {
    namespace ke = karteneditor;
    namespace kb = karte::befehle;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    // Bildpunkt in der XY-Ansicht (Ursprung links oben der Ansicht)
    auto xyPunkt = [](float fx, float fy) {
        const ke::XyFenster& f = ke::editor().xy[0];
        return ImVec2{f.bildX + static_cast<float>(f.ansicht.breite) * fx, f.bildY + static_cast<float>(f.ansicht.hoehe) * fy};
    };
    // Maus ziehen: Taste druecken, in Schritten bewegen, loslassen.
    // von: Anteil der Ansicht; weg: Bildpunkte (erst beim Ziehen ausgewertet).
    auto ziehen = [xyPunkt](const std::string& was, int taste, ImGuiKeyChord mods, ImVec2 von, ImVec2 weg) {
        return Schritt{was, [=](int b) {
                           ImGuiIO& io = ImGui::GetIO();
                           const ImVec2 a = xyPunkt(von.x, von.y);
                           const ImVec2 z{a.x + weg.x, a.y + weg.y};
                           if (b == 0) {
                               setzeMaus(a);
                               if ((mods & ImGuiMod_Ctrl) != 0) { io.AddKeyEvent(ImGuiMod_Ctrl, true); }
                               if ((mods & ImGuiMod_Shift) != 0) { io.AddKeyEvent(ImGuiMod_Shift, true); }
                               if ((mods & ImGuiMod_Alt) != 0) { io.AddKeyEvent(ImGuiMod_Alt, true); }
                               return false;
                           }
                           if (b == 1) {
                               io.AddMouseButtonEvent(taste, true);
                               return false;
                           }
                           if (b >= 2 && b <= 11) {
                               const float t = static_cast<float>(b - 1) / 10.0F;
                               setzeMaus(ImVec2{a.x + (z.x - a.x) * t, a.y + (z.y - a.y) * t});
                               return false;
                           }
                           if (b == 12) {
                               io.AddMouseButtonEvent(taste, false);
                               return false;
                           }
                           if (b == 13) {
                               if ((mods & ImGuiMod_Ctrl) != 0) { io.AddKeyEvent(ImGuiMod_Ctrl, false); }
                               if ((mods & ImGuiMod_Shift) != 0) { io.AddKeyEvent(ImGuiMod_Shift, false); }
                               if ((mods & ImGuiMod_Alt) != 0) { io.AddKeyEvent(ImGuiMod_Alt, false); }
                           }
                           return b >= 15;
                       }};
    };
    auto zustand = std::make_shared<karte::XyAnsicht>();
    add(tu("Regular, Massstab wie gewohnt, Testkarte neu laden, Standardanzeige", [] {
        ke::Editor& e = ke::editor();
        e.anordnung = kb::Anordnung::Regular;
        e.teilerFuer = -1;
        e.anzeige = ke::Anzeige{};
        e.aktivXy = 0;
        e.xy[0].ansicht.typ = karte::Sicht::XY;
        e.xy[0].ansicht.massstab = 1;
        (void)ke::karteOeffnen(karteTestPfad("karte_kopie.map"));
    }));
    add(pause(0.4));
    add(pruefe("Startposition: kein info_player_start -> Kamera und XY auf 0 0 0, Massstab 1", [] {
        const ke::Editor& e = ke::editor();
        return e.kamera.ursprung == karte::Vektor3f(0, 0, 0) && e.xy[0].ansicht.ursprung == karte::Vektor3f(0, 0, 0) &&
               e.xy[0].ansicht.massstab == 1 && e.xy[0].ansicht.breite > 100;
    }));
    add(foto("karteneditor_xy"));
    // Mausrad ueber der Ansicht: ein Schritt 5/4, der Punkt unter dem Zeiger bleibt.
    add({"Mausrad hoch ueber XY (rechts oben)", [xyPunkt, zustand](int b) {
             if (b == 0) { setzeMaus(xyPunkt(0.75F, 0.25F)); }
             if (b == 2) {
                 *zustand = ke::editor().xy[0].ansicht;
                 ImGui::GetIO().AddMouseWheelEvent(0.0F, 1.0F);
             }
             return b >= 5;
         }});
    add(pruefe("Mausrad: Massstab x 5/4, Weltpunkt unter dem Zeiger bleibt", [zustand, xyPunkt] {
        const karte::XyAnsicht& v = ke::editor().xy[0].ansicht;
        const ke::XyFenster& f = ke::editor().xy[0];
        const ImVec2 p = xyPunkt(0.75F, 0.25F);
        const int mx = static_cast<int>(std::lround(std::floor(p.x) - f.bildX));
        const int my = static_cast<int>(std::lround(std::floor(p.y) - f.bildY));
        const karte::Vektor3f a = zustand->zuPunkt(mx, my);
        const karte::Vektor3f b = v.zuPunkt(mx, my);
        diag::detail("Knopftest: Karten-Editor XY - Massstab " + std::to_string(v.massstab) + ", Punkt vorher " +
                     std::to_string(a[0]) + " nachher " + std::to_string(b[0]));
        return std::fabs(v.massstab - 1.25F) < 1e-4F && std::fabs(a[0] - b[0]) < 1.0F && std::fabs(a[1] - b[1]) < 1.0F;
    }));
    // M2 ziehen: Pan (Welt klebt am Zeiger, Deltas in logischen Qt-Punkten)
    add(tu("Zustand merken", [zustand] { *zustand = ke::editor().xy[0].ansicht; }));
    add(ziehen("M2 ziehen: 200 Bildpunkte nach rechts", ImGuiMouseButton_Right, 0, ImVec2{0.4F, 0.5F}, ImVec2{200.0F, 0.0F}));
    add(pruefe("Pan: Ursprung x um 200 / Qt-Massstab / Massstab nach links", [zustand] {
        const karte::XyAnsicht& v = ke::editor().xy[0].ansicht;
        const float qt = std::max(1.0F, std::round(g_app->dpiScale));
        const float soll = zustand->ursprung[0] - 200.0F / qt / v.massstab;
        diag::detail("Knopftest: Karten-Editor XY - Pan x " + std::to_string(zustand->ursprung[0]) + " -> " +
                     std::to_string(v.ursprung[0]) + " (soll " + std::to_string(soll) + ")");
        return std::fabs(v.ursprung[0] - soll) < 1.5F / v.massstab && v.ursprung[1] == zustand->ursprung[1] &&
               !ke::editor().xy[0].verschieben;
    }));
    // Alt+M2 ziehen: je 16 logische Punkte nach unten ein Schritt hinaus.
    add(tu("Zustand merken", [zustand] { *zustand = ke::editor().xy[0].ansicht; }));
    add(ziehen("Alt+M2 ziehen: 80 Bildpunkte nach unten", ImGuiMouseButton_Right, ImGuiMod_Alt, ImVec2{0.5F, 0.3F},
               ImVec2{0.0F, 80.0F}));
    add(pruefe("Alt+M2: 80 Bildpunkte = 40 logische nach unten: zwei Schritte hinaus (je ueber 16)", [zustand] {
        const karte::XyAnsicht& v = ke::editor().xy[0].ansicht;
        const float qt = std::max(1.0F, std::round(g_app->dpiScale));
        const int schritte = static_cast<int>(80.0F / qt) / 17;   // je > 16
        float soll = zustand->massstab;
        for (int i = 0; i < schritte; ++i) { soll *= 0.8F; }
        diag::detail("Knopftest: Karten-Editor XY - Drag-Zoom " + std::to_string(zustand->massstab) + " -> " +
                     std::to_string(v.massstab) + " (soll " + std::to_string(soll) + ")");
        return std::fabs(v.massstab - soll) < 1e-4F;
    }));
    // M3: Blickrichtung der Kamera auf den Zeiger; Ctrl+M3: Kamera dorthin.
    add(ziehen("M3 rechts der Kamera", ImGuiMouseButton_Middle, 0, ImVec2{0.9F, 0.5F}, ImVec2{0.0F, 0.0F}));
    add(pruefe("M3: yaw zeigt zum Zeiger (atan2)", [xyPunkt] {
        const ke::Editor& e = ke::editor();
        const ke::XyFenster& f = e.xy[0];
        // ImGui rundet die Mauslage ab (AddMousePosEvent: ImFloorSigned)
        const ImVec2 p = xyPunkt(0.9F, 0.5F);
        const karte::Vektor3f w = f.ansicht.zuPunkt(static_cast<int>(std::lround(std::floor(p.x) - f.bildX)),
                                                    static_cast<int>(std::lround(std::floor(p.y) - f.bildY)));
        double soll = std::atan2(w[1] - e.kamera.ursprung[1], w[0] - e.kamera.ursprung[0]) * 180.0 / 3.14159265358979;
        if (soll < 0) { soll += 360; }
        diag::detail("Knopftest: Karten-Editor XY - yaw " + std::to_string(e.kamera.winkel[1]) + " soll " + std::to_string(soll) +
                     " pitch " + std::to_string(e.kamera.winkel[0]));
        return std::fabs(e.kamera.winkel[1] - soll) < 0.01;
    }));
    add(ziehen("Ctrl+M3 links oben", ImGuiMouseButton_Middle, ImGuiMod_Ctrl, ImVec2{0.2F, 0.2F}, ImVec2{0.0F, 0.0F}));
    add(pruefe("Ctrl+M3: Kamera an den Zeiger (eingerastet), z bleibt", [xyPunkt] {
        const ke::Editor& e = ke::editor();
        const ke::XyFenster& f = e.xy[0];
        const ImVec2 p = xyPunkt(0.2F, 0.2F);
        const karte::Vektor3f w = f.ansicht.zuPunkt(static_cast<int>(std::lround(std::floor(p.x) - f.bildX)),
                                                    static_cast<int>(std::lround(std::floor(p.y) - f.bildY)), e.raster.einrastGroesse());
        return e.kamera.ursprung[0] == w[0] && e.kamera.ursprung[1] == w[1] && e.kamera.ursprung[2] == 0;
    }));
    add(pruefe("Statuszeile: Position \"x: %6.1f  y: %6.1f  z: %6.1f\"", [] {
        const std::string& t = ke::editor().statusPosition;
        return t.rfind("x: ", 0) == 0 && t.find("  y: ") != std::string::npos && t.find("  z:    0.0") != std::string::npos;
    }));
    // Tasten (NRC-Standard): Ctrl+Tab, Num7, Insert, `, Ctrl+Shift+Tab, J, Shift+X, 0
    add(tu("deutsche Tastatur", [] { ke::tastaturErsatz = deutscheTastatur; }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Tab, "Strg+Tab"));
    add(pruefe("Ctrl+Tab = NextView: XY -> XZ", [] { return ke::editor().xy[0].ansicht.typ == karte::Sicht::XZ; }));
    add(foto("karteneditor_xz"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Tab, "Strg+Tab"));
    add(pruefe("Ctrl+Tab: XZ -> YZ", [] { return ke::editor().xy[0].ansicht.typ == karte::Sicht::YZ; }));
    add(tasteKombi(ImGuiKey_Keypad7, "Num 7"));
    add(pruefe("Num 7 = ViewTop: XY", [] { return ke::editor().xy[0].ansicht.typ == karte::Sicht::XY; }));
    add(tu("Zustand merken", [zustand] { *zustand = ke::editor().xy[0].ansicht; }));
    add(tasteKombi(ImGuiKey_Insert, "Einfg"));
    add(pruefe("Insert = ZoomOut: Massstab x 4/5", [zustand] {
        return std::fabs(ke::editor().xy[0].ansicht.massstab - zustand->massstab * 0.8F) < 1e-4F;
    }));
    add(tu("Wuerfel auswaehlen", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        const auto w = e.karte.welt();
        if (w && !w->kinder().empty()) { e.karte.auswahl.waehlen(w, w->kinder().front()); }
    }));
    // "`" liegt auf deutscher Tastatur als Tottaste auf Umschalt+´ (wie in
    // NRC nicht als Kuerzel erreichbar) - hier ueber den Befehl.
    add(tu("XYFocusOnSelected", [] { (void)ke::ausfuehren("XYFocusOnSelected"); }));
    add(pause(0.2));
    add(pruefe("XYFocusOnSelected: Mitte des Wuerfels, Massstab min(W/384, H/384)", [] {
        const karte::XyAnsicht& v = ke::editor().xy[0].ansicht;
        const float soll = std::min(static_cast<float>(v.breite) / 384.0F, static_cast<float>(v.hoehe) / 384.0F);
        diag::detail("Knopftest: Karten-Editor XY - Fokus " + std::to_string(v.ursprung[0]) + "," + std::to_string(v.ursprung[1]) +
                     "," + std::to_string(v.ursprung[2]) + " Massstab " + std::to_string(v.massstab));
        return v.ursprung == karte::Vektor3f(0, 0, 32) && std::fabs(v.massstab - std::min(64.0F, soll)) < 1e-3F;
    }));
    add(foto("karteneditor_xy_auswahl"));
    add(tasteKombi(ImGuiKey_J, "J"));
    add(pruefe("J = ShowSize2d aus", [] { return !ke::editor().anzeige.groesse2d; }));
    add(tasteKombi(ImGuiKey_J, "J"));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_X, "Umschalt+X"));
    add(pruefe("Shift+X = ToggleCrosshairs an", [] { return ke::editor().anzeige.fadenkreuz; }));
    add(tasteKombi(ImGuiKey_0, "0"));
    add(pruefe("0 = ToggleGrid aus", [] { return !ke::editor().anzeige.raster; }));
    add(foto("karteneditor_xy_ohne_raster"));
    add(tasteKombi(ImGuiKey_0, "0"));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_X, "Umschalt+X"));
    add(tu("Auswahl leeren, echte Tastatur", [] {
        ke::editor().karte.auswahl.leeren();
        ke::tastaturErsatz = nullptr;
    }));
    return s;
}

// Punkt 6: die 3D-Kamera wie NRC (camwindow.cpp) - Bild (Direct3D 11, Textur
// aus qer_editorimage, Licht aus Blickrichtung), Freelook mit M2, diskrete
// Tasten, Fokus (Tab), Center (End), Etagen (PgUp/PgDown), Cubic Clip,
// Darstellungsarten (Shift+] / Shift+[), Mausrad, gehaltenes W im Freelook,
// Tempo (Shift+Num+), Auswahl getoent.
std::vector<Schritt> karteneditorKameraSchritte() {
    namespace ke = karteneditor;
    namespace kb = karte::befehle;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto kamPunkt = [](float fx, float fy) {
        const ke::KameraFenster& f = ke::editor().kamFenster;
        return ImVec2{f.bildX + static_cast<float>(f.breite) * fx, f.bildY + static_cast<float>(f.hoehe) * fy};
    };
    // Ein Bildpunkt der Kamera (Geraetepunkte im Fenster)
    auto pixel = [](float fx, float fy, unsigned char* rgba) {
        const ke::KameraFenster& f = ke::editor().kamFenster;
        return ke::kameraPixel(static_cast<int>(static_cast<float>(f.breite) * fx), static_cast<int>(static_cast<float>(f.hoehe) * fy), rgba);
    };
    auto pixelText = [](const unsigned char* p) {
        return std::to_string(p[0]) + "," + std::to_string(p[1]) + "," + std::to_string(p[2]);
    };
    // Kurzer Klick mit der rechten Maustaste in die Kamera (unter 300 ms)
    auto rechtsKlick = [kamPunkt](const std::string& was) {
        return Schritt{was, [kamPunkt](int b) {
                           ImGuiIO& io = ImGui::GetIO();
                           if (b == 0) { setzeMaus(kamPunkt(0.5F, 0.5F)); }
                           if (b == 1) { io.AddMouseButtonEvent(ImGuiMouseButton_Right, true); }
                           if (b == 2) { io.AddMouseButtonEvent(ImGuiMouseButton_Right, false); }
                           return b >= 4;
                       }};
    };
    auto merk = std::make_shared<karte::Kamera>();
    auto farbe = std::make_shared<std::array<unsigned char, 4>>();
    add(tu("Kamera: Regular, Testkarte, NRC-Standardwerte, Kamera vor dem Wuerfel (-256 0 32), Blick +x", [] {
        ke::Editor& e = ke::editor();
        e.anordnung = kb::Anordnung::Regular;
        e.teilerFuer = -1;
        e.anzeige = ke::Anzeige{};
        e.kameraE = karte::KameraEinstellungen{};
        (void)ke::karteOeffnen(karteTestPfad("karte_kopie.map"));
        e.karte.auswahl.leeren();
        e.kamera.setzeUrsprung(karte::Vektor3f(-256, 0, 32));
        e.kamera.setzeWinkel(karte::Vektor3f(0, 0, 0));
        ke::tastaturErsatz = deutscheTastatur;
    }));
    add(pause(0.3));
    add(warteBis("Kamera: Texturen geladen (qer_editorimage im Hintergrund)", [] { return ke::kameraBilderFertig(); }, 3000));
    add(pause(0.3));
    add(pruefe("Kamera gezeichnet: Mitte zeigt den Wuerfel, Ecke oben den Hintergrund (color_cameraback)", [pixel, pixelText, farbe] {
        unsigned char m[4] = {};
        unsigned char ecke[4] = {};
        if (!pixel(0.5F, 0.5F, m) || !pixel(0.02F, 0.02F, ecke)) { return false; }
        const auto& h = ke::editor().farben.kameraHinten;
        const auto nahe = [](int a, float b) { return std::abs(a - static_cast<int>(std::lround(b * 255.0F))) <= 1; };
        diag::detail("Knopftest: Karten-Editor Kamera - Mitte " + pixelText(m) + ", Ecke " + pixelText(ecke) + ", Hintergrund " +
                     std::to_string(h[0]) + " Fenster " + std::to_string(ke::editor().kamFenster.breite) + "x" +
                     std::to_string(ke::editor().kamFenster.hoehe));
        (*farbe)[0] = m[0];
        (*farbe)[1] = m[1];
        (*farbe)[2] = m[2];
        const bool eckeHinten = nahe(ecke[0], h[0]) && nahe(ecke[1], h[1]) && nahe(ecke[2], h[2]);
        const bool mitteWuerfel = !(nahe(m[0], h[0]) && nahe(m[1], h[1]) && nahe(m[2], h[2]));
        return eckeHinten && mitteWuerfel && ke::editor().kamFenster.breite > 100;
    }));
    add(foto("karteneditor_kamera"));
    // Freelook: kurzer M2 schaltet ein, Maus 100 Bildpunkte nach rechts dreht
    // um 0,1 Grad je logischem Punkt nach rechts, kurzer M2 schaltet aus.
    add(tu("Kamera merken", [merk] { *merk = ke::editor().kamera; }));
    add(rechtsKlick("M2 kurz in die Kamera"));
    add(pruefe("Freelook an (kurzer M2 = Umschalter)", [] { return ke::editor().kamFenster.frei; }));
    add({"Maus im Freelook 100 Bildpunkte nach rechts", [kamPunkt](int b) {
             const ImVec2 a = kamPunkt(0.5F, 0.5F);
             if (b >= 1 && b <= 10) { setzeMaus(ImVec2{a.x + 10.0F * static_cast<float>(b), a.y}); }
             return b >= 13;
         }});
    add(pruefe("Freelook: Yaw um 0,1 Grad je logischem Punkt kleiner (100 / Qt-Massstab)", [merk] {
        const float qt = std::max(1.0F, std::round(g_app->dpiScale));
        float soll = merk->winkel[1] - (100.0F / qt) * 0.0333333F * 3.0F;
        if (soll <= 0) { soll += 360; }
        diag::detail("Knopftest: Karten-Editor Kamera - Yaw " + std::to_string(ke::editor().kamera.winkel[1]) + " soll " + std::to_string(soll));
        return std::fabs(ke::editor().kamera.winkel[1] - soll) < 1e-3F && ke::editor().kamera.ursprung == merk->ursprung;
    }));
    add(foto("karteneditor_kamera_freelook"));
    // mehr als 250 ms nach dem ersten M2: sonst ist es der Doppel-M2 des Entitaetenmenues (context_menu_show)
    add(pause(0.3));
    add(rechtsKlick("M2 kurz im Freelook"));
    add(pruefe("Freelook aus (kurzer M2 im Freelook)", [] { return !ke::editor().kamFenster.frei; }));
    // Diskrete Tasten (CamWnd_Move_Discrete_Enable): Up 32 vorwaerts in der
    // Ebene, Left 22,5 Grad, D 32 nach rechts
    add(tu("Blick +x", [] { ke::editor().kamera.setzeWinkel(karte::Vektor3f(0, 0, 0)); }));
    add(tu("Kamera merken", [merk] { *merk = ke::editor().kamera; }));
    add(tasteKombi(ImGuiKey_UpArrow, "Pfeil hoch"));
    add(pruefe("Pfeil hoch = CameraForward diskret: +32 in x", [merk] {
        const karte::Vektor3f d = ke::editor().kamera.ursprung - merk->ursprung;
        return std::fabs(d[0] - 32.0F) < 1e-3F && std::fabs(d[1]) < 1e-3F && d[2] == 0;
    }));
    add(tasteKombi(ImGuiKey_D, "D"));
    add(pruefe("D = CameraStrafeRight diskret: 32 nach rechts (-y bei Yaw 0)", [merk] {
        const karte::Vektor3f d = ke::editor().kamera.ursprung - merk->ursprung;
        return std::fabs(d[1] + 32.0F) < 1e-3F;
    }));
    add(tasteKombi(ImGuiKey_LeftArrow, "Pfeil links"));
    add(pruefe("Pfeil links = CameraLeft diskret: Yaw +22,5", [] { return std::fabs(ke::editor().kamera.winkel[1] - 22.5F) < 1e-3F; }));
    add(tu("Pitch 30, Yaw 100", [] { ke::editor().kamera.setzeWinkel(karte::Vektor3f(30, 100, 0)); }));
    add(tasteKombi(ImGuiKey_End, "Ende"));
    add(pruefe("Ende = CenterView: Pitch 0, Yaw 90", [] {
        const karte::Vektor3f w = ke::editor().kamera.winkel;
        return w[0] == 0 && w[1] == 90;
    }));
    // Fokus auf die Auswahl
    add(tu("Wuerfel auswaehlen, Blick +x von weit weg (Werkzeug Resize: kein Manipulator ueber der Mitte)", [] {
        ke::Editor& e = ke::editor();
        if (e.bearbeitung.modus() != karte::ManipModus::Ziehen) { (void)ke::ausfuehren("MouseDrag"); }
        e.karte.auswahl.leeren();
        const auto w = e.karte.welt();
        if (w && !w->kinder().empty()) { e.karte.auswahl.waehlen(w, w->kinder().front()); }
        e.kamera.setzeUrsprung(karte::Vektor3f(-2000, 300, 500));
        e.kamera.setzeWinkel(karte::Vektor3f(-10, 20, 0));
    }));
    add(pause(0.2));
    add(tasteKombi(ImGuiKey_Tab, "Tab"));
    add(pruefe("Tab = CameraFocusOnSelected: Wuerfel ganz im Bild, Winkel bleiben", [] {
        const ke::Editor& e = ke::editor();
        const karte::Kasten w(karte::Vektor3f(0, 0, 32), karte::Vektor3f(64, 64, 32));
        const karte::Vektor3f soll = e.kamera.fokusPosition(w);
        diag::detail("Knopftest: Karten-Editor Kamera - Fokus " + std::to_string(e.kamera.ursprung[0]) + "," +
                     std::to_string(e.kamera.ursprung[1]) + "," + std::to_string(e.kamera.ursprung[2]));
        return karte::laenge(e.kamera.ursprung - soll) < 0.01 && e.kamera.winkel == karte::Vektor3f(-10, 20, 0);
    }));
    add(pause(0.3));
    add(tu("getoente Mitte merken", [pixel, farbe] {
        unsigned char m[4] = {};
        (void)pixel(0.5F, 0.5F, m);
        (*farbe)[0] = m[0];
        (*farbe)[1] = m[1];
        (*farbe)[2] = m[2];
    }));
    add(foto("karteneditor_kamera_auswahl"));
    add(tu("Auswahl leeren", [] { ke::editor().karte.auswahl.leeren(); }));
    add(pause(0.3));
    add(pruefe("Ausgewaehlt war die Mitte getoent: 0,7 x ohne + 0,3 x color_selbrushes3d ($CAM_HIGHLIGHT)", [pixel, pixelText, farbe] {
        unsigned char m[4] = {};
        if (!pixel(0.5F, 0.5F, m)) { return false; }
        const auto& sel = ke::editor().farben.auswahl3d;
        bool gut = true;
        std::string soll;
        for (int i = 0; i < 3; ++i) {
            const float erwartet = (0.7F * static_cast<float>(m[i])) + (0.3F * sel[static_cast<std::size_t>(i)] * 255.0F);
            soll += std::to_string(static_cast<int>(erwartet)) + " ";
            gut = gut && std::fabs(static_cast<float>((*farbe)[static_cast<std::size_t>(i)]) - erwartet) <= 3.0F;
        }
        diag::detail("Knopftest: Karten-Editor Kamera - ohne Auswahl " + pixelText(m) + ", getoent " +
                     pixelText(farbe->data()) + ", soll " + soll);
        return gut;
    }));
    // Etagen: Kamera ueber dem Wuerfel (Oberkante 64) unter dem Licht (120..136)
    add(tu("Auswahl leeren, Kamera auf 0 0 32", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.kamera.setzeUrsprung(karte::Vektor3f(0, 0, 32));
        e.kamera.setzeWinkel(karte::Vektor3f(0, 0, 0));
    }));
    auto etage = [](float soll, const std::string& was) {
        return pruefe(was, [soll] {
            diag::detail("Knopftest: Karten-Editor Kamera - z " + std::to_string(ke::editor().kamera.ursprung[2]));
            return ke::editor().kamera.ursprung[2] == soll;
        });
    };
    add(tasteKombi(ImGuiKey_PageUp, "Bild hoch"));
    add(etage(96, "PgUp = UpFloor: Oberkante Wuerfel 64 + 32"));
    add(tasteKombi(ImGuiKey_PageUp, "Bild hoch"));
    add(etage(168, "PgUp: Oberkante Licht 136 + 32"));
    add(tasteKombi(ImGuiKey_PageDown, "Bild runter"));
    add(etage(96, "PgDown = DownFloor: 96"));
    add(tasteKombi(ImGuiKey_PageDown, "Bild runter"));
    add(etage(-16, "PgDown: keine tiefere Etage - Unterkante 0 - 16"));
    // Cubic Clip: Strg+\\, Strg+[ und Strg+] sind auf deutscher Tastatur
    // nicht erreichbar (AltGr) - ueber die Befehle; die Statuszeile zeigt C:
    add(tu("ToggleCubicClip, CubicClipZoomIn x2", [] {
        (void)ke::ausfuehren("ToggleCubicClip");
        (void)ke::ausfuehren("CubicClipZoomIn");
        (void)ke::ausfuehren("CubicClipZoomIn");
    }));
    add(pause(0.2));
    add(pruefe("Cubic Clip an, Skala 12, Fernebene 2^9.5, Statuszeile \"C:12\"", [] {
        const ke::Editor& e = ke::editor();
        const float fern = karte::fernEbene(e.kameraE);
        const float p10 = e.kamera.projektion[10];
        const float soll10 = -(fern + 1.0F) / (fern - 1.0F);
        return e.kameraE.kubisch && e.kameraE.kubikSkala == 12 && ke::angehakt("ToggleCubicClip") &&
               std::fabs(fern - 724.0773F) < 0.01F && std::fabs(p10 - soll10) < 1e-6F;
    }));
    add(tu("Cubic Clip zurueck", [] {
        (void)ke::ausfuehren("CubicClipZoomOut");
        (void)ke::ausfuehren("CubicClipZoomOut");
        (void)ke::ausfuehren("ToggleCubicClip");
    }));
    // Darstellungsarten: Umschalt+AltGr+9 = "Shift+]" = CameraModeNext
    add(tu("Kamera vor dem Wuerfel, Blick +x", [] {
        ke::Editor& e = ke::editor();
        e.kamera.setzeUrsprung(karte::Vektor3f(-256, 0, 32));
        e.kamera.setzeWinkel(karte::Vektor3f(0, 0, 0));
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiMod_Shift | ImGuiKey_9, "Umschalt+AltGr+9"));
    add(pruefe("Shift+] = CameraModeNext: Textured -> Textured+Wire",
               [] { return ke::editor().kameraE.modus == karte::KameraModus::TexturDraht; }));
    add(pause(0.2));
    add(foto("karteneditor_kamera_texturdraht"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiMod_Shift | ImGuiKey_9, "Umschalt+AltGr+9"));
    add(pruefe("Shift+]: Textured+Wire -> Wireframe (ohne Lighting wie Quake 3)",
               [] { return ke::editor().kameraE.modus == karte::KameraModus::Draht; }));
    add(pause(0.2));
    add(pruefe("Wireframe: in der Flaeche nur Hintergrund (Polygone als Linien)", [pixel, pixelText] {
        unsigned char m[4] = {};
        if (!pixel(0.5F, 0.5F, m)) { return false; }
        const auto& h = ke::editor().farben.kameraHinten;
        diag::detail("Knopftest: Karten-Editor Kamera - Draht Mitte " + pixelText(m));
        return std::abs(m[0] - static_cast<int>(std::lround(h[0] * 255.0F))) <= 1;
    }));
    add(foto("karteneditor_kamera_draht"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiMod_Shift | ImGuiKey_9, "Umschalt+AltGr+9"));
    add(pruefe("Shift+]: Wireframe -> Flatshade", [] { return ke::editor().kameraE.modus == karte::KameraModus::Flach; }));
    add(pause(0.2));
    add(pruefe("Flatshade: Flaeche zur Kamera = Durchschnittsfarbe der Textur x 1,0 (Licht aus Blickrichtung)", [pixel, pixelText] {
        unsigned char m[4] = {};
        float c[3] = {};
        if (!pixel(0.5F, 0.5F, m) || !ke::kameraTexturFarbe("textures/common/caulk", c)) { return false; }
        diag::detail("Knopftest: Karten-Editor Kamera - flach " + pixelText(m) + " soll " + std::to_string(c[0] * 255.0F) + "," +
                     std::to_string(c[1] * 255.0F) + "," + std::to_string(c[2] * 255.0F));
        bool gut = true;
        for (int i = 0; i < 3; ++i) { gut = gut && std::abs(m[i] - static_cast<int>(std::lround(c[i] * 255.0F))) <= 1; }
        return gut;
    }));
    add(foto("karteneditor_kamera_flach"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiMod_Shift | ImGuiKey_9, "Umschalt+AltGr+9"));
    add(pruefe("Shift+]: Flatshade -> Textured", [] { return ke::editor().kameraE.modus == karte::KameraModus::Textur; }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiMod_Shift | ImGuiKey_8, "Umschalt+AltGr+8"));
    add(pruefe("Shift+[ = CameraModePrev: Textured -> Flatshade", [] { return ke::editor().kameraE.modus == karte::KameraModus::Flach; }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiMod_Shift | ImGuiKey_9, "Umschalt+AltGr+9"));
    // Mausrad: ein Schritt (100) je Ereignis Richtung Zeiger (Bildmitte = vorwaerts)
    add(tu("Kamera merken", [merk] { *merk = ke::editor().kamera; }));
    add({"Mausrad hoch in der Bildmitte der Kamera", [kamPunkt](int b) {
             if (b == 0) { setzeMaus(kamPunkt(0.5F, 0.5F)); }
             if (b == 2) { ImGui::GetIO().AddMouseWheelEvent(0.0F, 1.0F); }
             return b >= 5;
         }});
    add(pruefe("Mausrad: 100 vorwaerts (Zoom to Mouse pointer)", [merk] {
        const karte::Vektor3f d = ke::editor().kamera.ursprung - merk->ursprung;
        diag::detail("Knopftest: Karten-Editor Kamera - Rad " + std::to_string(d[0]) + "," + std::to_string(d[1]) + "," +
                     std::to_string(d[2]));
        return std::fabs(static_cast<float>(karte::laenge(d)) - 100.0F) < 0.01F && d[0] > 99.0F;
    }));
    // Freelook mit gehaltenem W: kontinuierlich, beschleunigt (500 u/s nach 200 ms)
    add(rechtsKlick("M2 kurz: Freelook an"));
    add(tu("Kamera merken", [merk] { *merk = ke::editor().kamera; }));
    add({"W 0,4 s halten", [](int b) {
             static double start = 0;
             ImGuiIO& io = ImGui::GetIO();
             if (b == 0) {
                 io.AddKeyEvent(ImGuiKey_W, true);
                 start = ImGui::GetTime();
                 return false;
             }
             if (ImGui::GetTime() - start < 0.4) { return false; }
             io.AddKeyEvent(ImGuiKey_W, false);
             return true;
         }});
    add(pause(0.2));
    add(pruefe("W im Freelook: kontinuierlich vorwaerts (mehr als 32), danach steht die Kamera", [merk] {
        const ke::Editor& e = ke::editor();
        const karte::Vektor3f d = e.kamera.ursprung - merk->ursprung;
        diag::detail("Knopftest: Karten-Editor Kamera - W " + std::to_string(d[0]) + " Tempo " + std::to_string(e.kamera.tempo));
        return d[0] > 40.0F && d[0] < 260.0F && e.kamera.bewegung == 0U && e.kamFenster.frei;
    }));
    add(pause(0.3));   // kein Doppel-M2 (Entitaetenmenue)
    add(rechtsKlick("M2 kurz: Freelook aus"));
    add(pruefe("Freelook aus", [] { return !ke::editor().kamFenster.frei; }));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_KeypadAdd, "Umschalt+Num+"));
    add(pruefe("Shift+Num+ = CameraSpeedInc: 750, Konsole \" ++Camera Move Speed: 750\" (Text Ausgabe, Zahl Warnung, eine Zeile)", [] {
        const ke::Editor& e = ke::editor();
        bool zeile = false;
        for (const ke::KonsolenZeile& z : e.konsole) {
            zeile = zeile || (z.text == " ++Camera Move Speed: 750" && z.teile.size() == 2 &&
                              z.teile[0].art == ke::KonsolenZeile::Art::Ausgabe && z.teile[0].laenge == 22 &&
                              z.teile[1].art == ke::KonsolenZeile::Art::Warnung);
        }
        return e.kameraE.tempo == 750 && zeile;
    }));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_KeypadSubtract, "Umschalt+Num-"));
    add(pruefe("Shift+Num- = CameraSpeedDec: 500", [] { return ke::editor().kameraE.tempo == 500; }));
    // Die uebrigen diskreten Befehle: Down, Right, A; CameraUp/Down/AngleUp/
    // AngleDown haben keine Taste (ueber den Befehl).
    add(tu("Kamera auf 0 0 0, Blick +x, merken", [merk] {
        ke::editor().kamera.setzeUrsprung(karte::Vektor3f(0, 0, 0));
        ke::editor().kamera.setzeWinkel(karte::Vektor3f(0, 0, 0));
        *merk = ke::editor().kamera;
    }));
    add(tasteKombi(ImGuiKey_DownArrow, "Pfeil runter"));
    add(pruefe("Pfeil runter = CameraBack diskret: -32 in x", [] { return std::fabs(ke::editor().kamera.ursprung[0] + 32.0F) < 1e-3F; }));
    add(tasteKombi(ImGuiKey_A, "A"));
    add(pruefe("A = CameraStrafeLeft diskret: +32 in y", [] { return std::fabs(ke::editor().kamera.ursprung[1] - 32.0F) < 1e-3F; }));
    add(tasteKombi(ImGuiKey_RightArrow, "Pfeil rechts"));
    add(pruefe("Pfeil rechts = CameraRight diskret: Yaw -22,5", [] { return std::fabs(ke::editor().kamera.winkel[1] + 22.5F) < 1e-3F; }));
    add(tu("CameraUp, CameraAngleUp", [] {
        (void)ke::ausfuehren("CameraUp");
        (void)ke::ausfuehren("CameraAngleUp");
    }));
    add(pruefe("CameraUp: z +32, CameraAngleUp: Pitch +22,5", [] {
        return ke::editor().kamera.ursprung[2] == 32 && std::fabs(ke::editor().kamera.winkel[0] - 22.5F) < 1e-3F;
    }));
    add(tu("CameraDown, CameraAngleDown", [] {
        (void)ke::ausfuehren("CameraDown");
        (void)ke::ausfuehren("CameraAngleDown");
    }));
    add(pruefe("CameraDown: z 0, CameraAngleDown: Pitch 0", [] {
        return ke::editor().kamera.ursprung[2] == 0 && std::fabs(ke::editor().kamera.winkel[0]) < 1e-3F;
    }));
    // Freelook: alle KeyEvents (CamWnd_Add_Handlers_FreeMove); FreeMoveUp/Down
    // haben keine Taste - fuer den Test F9 / F10.
    add(tu("Kamera auf 0 0 0, Blick +x; F9/F10 fuer CameraFreeMoveUp/Down", [] {
        ke::Editor& e = ke::editor();
        e.kamera.setzeUrsprung(karte::Vektor3f(0, 0, 0));
        e.kamera.setzeWinkel(karte::Vektor3f(0, 0, 0));
        e.belegung.setzen("CameraFreeMoveUp", kb::tasteAusText("F9"));
        e.belegung.setzen("CameraFreeMoveDown", kb::tasteAusText("F10"));
    }));
    add(rechtsKlick("M2 kurz: Freelook an"));
    auto halten = [merk](ImGuiKey taste, const std::string& name) {
        return Schritt{"Freelook: " + name + " 0,15 s halten", [merk, taste](int b) {
                           static double start = 0;
                           ImGuiIO& io = ImGui::GetIO();
                           if (b == 0) {
                               *merk = ke::editor().kamera;
                               io.AddKeyEvent(taste, true);
                               start = ImGui::GetTime();
                               return false;
                           }
                           if (ImGui::GetTime() - start < 0.15) { return false; }
                           io.AddKeyEvent(taste, false);
                           return true;
                       }};
    };
    auto richtung = [merk](int achse, float vz, const std::string& was) {
        return pruefe(was, [merk, achse, vz] {
            const ke::Editor& e = ke::editor();
            const float d = e.kamera.ursprung[static_cast<std::size_t>(achse)] - merk->ursprung[static_cast<std::size_t>(achse)];
            diag::detail("Knopftest: Karten-Editor Kamera - Achse " + std::to_string(achse) + " " + std::to_string(d));
            return d * vz > 5.0F && e.kamera.bewegung == 0U;
        });
    };
    struct Freitaste {
        ImGuiKey taste;
        const char* name;
        int achse;
        float vz;
        const char* was;
    };
    static const Freitaste kFrei[] = {
        {ImGuiKey_S, "S", 0, -1, "S = CameraFreeMoveBack: zurueck"},
        {ImGuiKey_A, "A", 1, 1, "A = CameraFreeMoveLeft: nach links (+y)"},
        {ImGuiKey_D, "D", 1, -1, "D = CameraFreeMoveRight: nach rechts (-y)"},
        {ImGuiKey_UpArrow, "Pfeil hoch", 0, 1, "Pfeil hoch = CameraFreeMoveForward2: vor"},
        {ImGuiKey_DownArrow, "Pfeil runter", 0, -1, "Pfeil runter = CameraFreeMoveBack2: zurueck"},
        {ImGuiKey_LeftArrow, "Pfeil links", 1, 1, "Pfeil links = CameraFreeMoveLeft2: links"},
        {ImGuiKey_RightArrow, "Pfeil rechts", 1, -1, "Pfeil rechts = CameraFreeMoveRight2: rechts"},
        {ImGuiKey_F9, "F9", 2, 1, "F9 = CameraFreeMoveUp (Testbelegung): hoch in Welt-z"},
        {ImGuiKey_F10, "F10", 2, -1, "F10 = CameraFreeMoveDown (Testbelegung): runter"},
    };
    for (const Freitaste& t : kFrei) {
        add(halten(t.taste, t.name));
        add(pause(0.1));
        add(richtung(t.achse, t.vz, t.was));
    }
    add(tu("Wuerfel auswaehlen", [] {
        ke::Editor& e = ke::editor();
        const auto w = e.karte.welt();
        if (w && !w->kinder().empty()) { e.karte.auswahl.waehlen(w, w->kinder().front()); }
    }));
    add(pause(0.2));
    add(halten(ImGuiKey_Tab, "Tab"));
    add(pause(0.1));
    add(pruefe("Tab im Freelook = CameraFreeFocus: jeder Schritt setzt die Kamera auf den Fokus", [] {
        const ke::Editor& e = ke::editor();
        const karte::Vektor3f soll = e.kamera.fokusPosition(e.workzone);
        return karte::laenge(e.kamera.ursprung - soll) < 0.01 && e.kamera.bewegung == 0U && e.kamera.fokusVersatz == 0;
    }));
    add(pause(0.3));   // kein Doppel-M2 (Entitaetenmenue)
    add(rechtsKlick("M2 kurz: Freelook aus"));
    add(tu("Testbelegung zuruecksetzen, Auswahl leeren", [] {
        ke::Editor& e = ke::editor();
        e.belegung.zuruecksetzen("CameraFreeMoveUp");
        e.belegung.zuruecksetzen("CameraFreeMoveDown");
        e.karte.auswahl.leeren();
    }));
    // Einstellungsseite "Camera": "Discrete movement" aus -> die Pfeile sind
    // KeyEvents (kontinuierlich, CamWnd_Move_Enable).
    add(tu("Einstellungen oeffnen", [] { (void)ke::ausfuehren("Preferences"); }));
    add(pause(0.3));
    // Vergleich mit den Fotos SP\nrc_fotos\dialoge\pref_Layout.png / pref_Interface.png
    add(klick(ke::trK(Str::KePgLayout), "###karteeinstellungen"));
    add(pause(0.2));
    add(foto("karteneditor_einstellungen_layout"));
    add(klick(ke::trK(Str::KePgInterface), "###karteeinstellungen"));
    add(pause(0.2));
    add(foto("karteneditor_einstellungen_interface"));
    add(klick(ke::trK(Str::KePgCamera)));
    add(pause(0.2));
    add(foto("karteneditor_einstellungen_kamera"));
    add(klick(ke::trK(Str::KePrCamDiscrete)));
    add(klick(ke::trK(Str::KePrefOk)));
    add(pause(0.2));
    add(pruefe("Einstellungen: Discrete movement aus", [] { return !ke::editor().kameraE.diskret; }));
    add(tu("Kamera auf 0 0 0, Blick +x", [] {
        ke::editor().kamera.setzeUrsprung(karte::Vektor3f(0, 0, 0));
        ke::editor().kamera.setzeWinkel(karte::Vektor3f(0, 0, 0));
    }));
    add(halten(ImGuiKey_UpArrow, "Pfeil hoch (nicht diskret)"));
    add(pause(0.1));
    add(richtung(0, 1, "Pfeil hoch ohne Discrete movement = CameraForward als KeyEvent: kontinuierlich vor"));
    add(halten(ImGuiKey_LeftArrow, "Pfeil links (nicht diskret)"));
    add(pause(0.1));
    add(pruefe("Pfeil links = CameraLeft als KeyEvent: dreht (75 Grad/s)", [] {
        const float y = ke::editor().kamera.winkel[1];
        diag::detail("Knopftest: Karten-Editor Kamera - Yaw " + std::to_string(y));
        return y > 5.0F && y < 40.0F;
    }));
    // View > Show: Renderer Stats, 3D Workzone, 3D Size Info
    add(tu("ShowStats an, Wuerfel auswaehlen", [] {
        (void)ke::ausfuehren("ShowStats");
        ke::Editor& e = ke::editor();
        e.kamera.setzeUrsprung(karte::Vektor3f(-256, -128, 128));
        e.kamera.setzeWinkel(karte::Vektor3f(-20, 25, 0));
        const auto w = e.karte.welt();
        if (w && !w->kinder().empty()) { e.karte.auswahl.waehlen(w, w->kinder().front()); }
    }));
    add(pause(0.3));
    add(pruefe("ShowStats, ShowWorkzone3d, ShowSize3d angehakt", [] {
        return ke::angehakt("ShowStats") && ke::angehakt("ShowWorkzone3d") && ke::angehakt("ShowSize3d");
    }));
    add(foto("karteneditor_kamera_statistik"));
    add(tu("ShowWorkzone3d und ShowSize3d aus", [] {
        (void)ke::ausfuehren("ShowWorkzone3d");
        (void)ke::ausfuehren("ShowSize3d");
    }));
    add(pause(0.2));
    add(pruefe("ShowWorkzone3d / ShowSize3d aus", [] { return !ke::angehakt("ShowWorkzone3d") && !ke::angehakt("ShowSize3d"); }));
    add(foto("karteneditor_kamera_ohne_workzone"));
    add(tu("Kamera: Standardwerte, echte Tastatur", [] {
        ke::editor().kameraE = karte::KameraEinstellungen{};
        ke::editor().karte.auswahl.leeren();
        ke::tastaturErsatz = nullptr;
    }));
    return s;
}

// Punkt 7: Auswaehlen und Bearbeiten wie NRC - Klick (Tunnel), Ziehen,
// Groesse aendern, die Werkzeuge W / R / Q, Klonen, Loeschen, Rechteck,
// Verstecken, Schubsen, Raster, Kopieren/Einfuegen, Rueckgaengig mit Namen,
// neuer Pinsel in 3D.
std::vector<Schritt> karteneditorAuswahlSchritte() {
    namespace ke = karteneditor;
    namespace kb = karte::befehle;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    // Weltpunkt (x, y) der XY-Ansicht als Bildschirmpunkt
    auto welt = [](float x, float y) {
        const ke::XyFenster& f = ke::editor().xy[0];
        const auto b = f.ansicht.zuBild(karte::Vektor3f(x, y, 0));
        return ImVec2{f.bildX + b[0], f.bildY + b[1]};
    };
    // Maus ziehen in der XY-Ansicht: Zusatztasten, Druck, zehn Schritte, Loslassen.
    auto ziehen = [welt](const std::string& was, int taste, ImGuiKeyChord mods, ImVec2 von, ImVec2 nach) {
        return Schritt{was, [=](int b) {
                           ImGuiIO& io = ImGui::GetIO();
                           const ImVec2 a = welt(von.x, von.y);
                           const ImVec2 z = welt(nach.x, nach.y);
                           if (b == 0) {
                               setzeMaus(a);
                               if ((mods & ImGuiMod_Ctrl) != 0) { io.AddKeyEvent(ImGuiMod_Ctrl, true); }
                               if ((mods & ImGuiMod_Shift) != 0) { io.AddKeyEvent(ImGuiMod_Shift, true); }
                               if ((mods & ImGuiMod_Alt) != 0) { io.AddKeyEvent(ImGuiMod_Alt, true); }
                               return false;
                           }
                           if (b == 1) {
                               io.AddMouseButtonEvent(taste, true);
                               return false;
                           }
                           if (b >= 2 && b <= 11) {
                               const float t = static_cast<float>(b - 1) / 10.0F;
                               setzeMaus(ImVec2{a.x + (z.x - a.x) * t, a.y + (z.y - a.y) * t});
                               return false;
                           }
                           if (b == 12) {
                               io.AddMouseButtonEvent(taste, false);
                               return false;
                           }
                           if (b == 13) {
                               if ((mods & ImGuiMod_Ctrl) != 0) { io.AddKeyEvent(ImGuiMod_Ctrl, false); }
                               if ((mods & ImGuiMod_Shift) != 0) { io.AddKeyEvent(ImGuiMod_Shift, false); }
                               if ((mods & ImGuiMod_Alt) != 0) { io.AddKeyEvent(ImGuiMod_Alt, false); }
                           }
                           return b >= 15;
                       }};
    };
    // Klick ohne Bewegung (Tunnel-Auswahl)
    auto klickWelt = [welt](const std::string& was, int taste, ImGuiKeyChord mods, ImVec2 an) {
        return Schritt{was, [=](int b) {
                           ImGuiIO& io = ImGui::GetIO();
                           if (b == 0) {
                               setzeMaus(welt(an.x, an.y));
                               if ((mods & ImGuiMod_Shift) != 0) { io.AddKeyEvent(ImGuiMod_Shift, true); }
                               return false;
                           }
                           if (b == 2) {
                               io.AddMouseButtonEvent(taste, true);
                               return false;
                           }
                           if (b == 3) {
                               io.AddMouseButtonEvent(taste, false);
                               return false;
                           }
                           if (b == 4 && (mods & ImGuiMod_Shift) != 0) { io.AddKeyEvent(ImGuiMod_Shift, false); }
                           return b >= 6;
                       }};
    };
    auto pinsel = [] {
        const auto w = ke::editor().karte.welt();
        return (w && !w->kinder().empty()) ? std::dynamic_pointer_cast<karte::Pinsel>(w->kinder().front()) : nullptr;
    };
    auto grenzen = [pinsel] {
        const auto p = pinsel();
        return p ? p->grenzen(ke::editor().karte.kontext) : karte::Kasten();
    };
    auto nah = [](float a, float b) { return std::fabs(a - b) < 0.01F; };
    auto rueckgaengig = [](const std::string& name) {
        return pruefe("Rueckgaengig-Name: " + name, [name] {
            diag::detail("Knopftest: Karten-Editor Auswahl - Rueckgaengig \"" + ke::editor().karte.undo.undoName() + "\"");
            return ke::editor().karte.undo.undoName().rfind(name, 0) == 0;
        });
    };
    add(tu("Auswahl: Regular, Testkarte, XY Massstab 1 um (0 0 0), Raster 8, Werkzeug Resize", [] {
        ke::Editor& e = ke::editor();
        e.anordnung = kb::Anordnung::Regular;
        e.teilerFuer = -1;
        e.anzeige = ke::Anzeige{};
        (void)ke::karteOeffnen(karteTestPfad("karte_kopie.map"));
        e.karte.auswahl.leeren();
        e.aktivXy = 0;
        e.xy[0].ansicht.typ = karte::Sicht::XY;
        e.xy[0].ansicht.setzeMassstab(1);
        e.xy[0].ansicht.setzeUrsprung(karte::Vektor3f(0, 0, 0));
        e.raster.setzePotenz(3);
        e.kamera.setzeUrsprung(karte::Vektor3f(-256, 0, 32));
        e.kamera.setzeWinkel(karte::Vektor3f(0, 0, 0));
        ke::tastaturErsatz = deutscheTastatur;
        if (e.bearbeitung.modus() != karte::ManipModus::Ziehen) { (void)ke::ausfuehren("MouseDrag"); }
    }));
    add(pause(0.4));
    add(pruefe("Standardwerkzeug Resize (Q) gedrueckt (g_defaultToolMode = DragMode)",
               [] { return ke::angehakt("MouseDrag") && !ke::angehakt("MouseTranslate"); }));
    add(klickWelt("M1-Klick auf den Pinsel (32 32)", ImGuiMouseButton_Left, 0, ImVec2{32, 32}));
    add(pruefe("Tunnel-Auswahl: der Pinsel ist gewaehlt", [pinsel] {
        const ke::Editor& e = ke::editor();
        return e.karte.auswahl.anzahl() == 1 && e.karte.auswahl.gewaehlt(e.karte.welt().get(), pinsel().get());
    }));
    add(ziehen("Resize: im Pinsel ziehen (32 32) -> (64 48)", ImGuiMouseButton_Left, 0, ImVec2{32, 32}, ImVec2{64, 48}));
    add(pruefe("gezogen um (32 16)", [grenzen, nah] {
        const karte::Kasten b = grenzen();
        diag::detail("Knopftest: Karten-Editor Auswahl - Mitte " + std::to_string(b.origin[0]) + " " + std::to_string(b.origin[1]));
        return nah(b.origin[0], 32) && nah(b.origin[1], 16) && nah(b.extents[0], 64);
    }));
    add(rueckgaengig("dragTool"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pruefe("Strg+Z: zurueck, Konsole \"Undo: dragTool\"", [grenzen, nah] {
        const auto& k = ke::editor().konsole;
        return nah(grenzen().origin[0], 0) && !k.empty() && k.back().text == "Undo: dragTool";
    }));
    add(ziehen("Resize: neben der +x-Seite (84 0) -> (100 0)", ImGuiMouseButton_Left, 0, ImVec2{84, 0}, ImVec2{100, 0}));
    add(pruefe("Groesse: +x-Seite auf 80, danach keine Flaechen gewaehlt", [grenzen, nah] {
        const karte::Kasten b = grenzen();
        return nah(b.origin[0] + b.extents[0], 80) && nah(b.origin[0] - b.extents[0], -64) && ke::editor().karte.auswahl.flaechen().empty();
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tasteKombi(ImGuiKey_W, "W"));
    add(pruefe("W: Translate, Status wie NRC", [] {
        const ke::Editor& e = ke::editor();
        return e.bearbeitung.modus() == karte::ManipModus::Verschieben && ke::angehakt("MouseTranslate") &&
               e.statusBefehl == "Translate Tool: translate objects and components";
    }));
    add(pause(0.2));
    add(foto("karteneditor_verschieben"));
    add(ziehen("x-Pfeil ziehen (30 0) -> (62 0)", ImGuiMouseButton_Left, 0, ImVec2{30, 0}, ImVec2{62, 4}));
    add(pruefe("x-Pfeil: 32 nach rechts, y bleibt", [grenzen, nah] {
        const karte::Kasten b = grenzen();
        return nah(b.origin[0], 32) && nah(b.origin[1], 0);
    }));
    add(rueckgaengig("translateTool -xyz 32 0 0"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tasteKombi(ImGuiKey_R, "R"));
    add(pause(0.2));
    add(foto("karteneditor_drehen"));
    add(ziehen("Bildkreis ziehen (52 52) -> (-52 52)", ImGuiMouseButton_Left, 0, ImVec2{52, 52}, ImVec2{-52, 52}));
    add(rueckgaengig("rotateTool -eulerXYZ"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tasteKombi(ImGuiKey_Q, "Q"));
    add(pruefe("Q nach Rotate: Resize (ToggleDragSkewModes)", [] { return ke::editor().bearbeitung.modus() == karte::ManipModus::Ziehen; }));
    add(tasteKombi(ImGuiKey_Q, "Q"));
    add(pruefe("Q nach Resize: Transform", [] {
        return ke::editor().bearbeitung.modus() == karte::ManipModus::Verzerren && ke::angehakt("MouseTransform");
    }));
    add(pause(0.2));
    add(foto("karteneditor_transform"));
    add(tasteKombi(ImGuiKey_Q, "Q"));
    add(tasteKombi(ImGuiKey_Space, "Leertaste"));
    add(pruefe("Leertaste: geklont, der Klon gewaehlt", [] {
        const ke::Editor& e = ke::editor();
        return e.karte.welt()->kinder().size() == 2 && e.karte.auswahl.anzahl() == 1 &&
               e.karte.auswahl.gewaehlt(e.karte.welt().get(), e.karte.welt()->kinder().back().get());
    }));
    add(rueckgaengig("cloneSelected"));
    add(tasteKombi(ImGuiKey_Delete, "Entf"));
    add(pruefe("Entf: der Klon ist geloescht", [] {
        const ke::Editor& e = ke::editor();
        return e.karte.welt()->kinder().size() == 1 && e.karte.auswahl.anzahl() == 0;
    }));
    add(rueckgaengig("deleteSelected"));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    // Rechteck mit Umschalt+M2: ziehen, halb zeigen (Foto), loslassen
    add({"Umschalt+M2 druecken und halb ziehen", [welt](int b) {
             ImGuiIO& io = ImGui::GetIO();
             const ImVec2 a = welt(-100, -100);
             const ImVec2 z = welt(100, 100);
             if (b == 0) {
                 setzeMaus(a);
                 io.AddKeyEvent(ImGuiMod_Shift, true);
             }
             if (b == 1) { io.AddMouseButtonEvent(ImGuiMouseButton_Right, true); }
             if (b >= 2 && b <= 7) {
                 const float t = static_cast<float>(b - 1) / 10.0F;
                 setzeMaus(ImVec2{a.x + (z.x - a.x) * t, a.y + (z.y - a.y) * t});
             }
             return b >= 9;
         }});
    add(pruefe("Auswahlrechteck sichtbar (rect_t eSelect)", [] {
        const karte::AuswahlRechteck& r = ke::editor().beobachter[0].rechteck;
        return r.max[0] > r.min[0] && r.max[1] > r.min[1] && r.art == karte::AuswahlRechteck::Art::Auswaehlen;
    }));
    add(foto("karteneditor_rechteck"));
    add({"fertig ziehen, loslassen", [welt](int b) {
             ImGuiIO& io = ImGui::GetIO();
             if (b == 0) { setzeMaus(welt(100, 100)); }
             if (b == 1) { io.AddMouseButtonEvent(ImGuiMouseButton_Right, false); }
             if (b == 2) { io.AddKeyEvent(ImGuiMod_Shift, false); }
             return b >= 4;
         }});
    add(pruefe("Rechteck: Pinsel und Licht gewaehlt", [] { return ke::editor().karte.auswahl.anzahl() == 2; }));
    add(tasteKombi(ImGuiKey_H, "H"));
    add(pruefe("H: versteckt und abgewaehlt, Knopf gedrueckt", [pinsel] {
        const ke::Editor& e = ke::editor();
        return pinsel()->versteckt && e.karte.auswahl.anzahl() == 0 && ke::angehakt("HideSelected");
    }));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_H, "Umschalt+H"));
    add(pruefe("Umschalt+H: wieder sichtbar", [pinsel] { return !pinsel()->versteckt && !ke::angehakt("HideSelected"); }));
    add(klickWelt("M1-Klick auf den Pinsel", ImGuiMouseButton_Left, 0, ImVec2{32, 32}));
    add(tasteKombi(ImGuiMod_Alt | ImGuiKey_RightArrow, "Alt+Rechts"));
    add(pruefe("Alt+Rechts: um ein Raster (8) nach rechts", [grenzen, nah] { return nah(grenzen().origin[0], 8); }));
    add(rueckgaengig("nudgeSelectedRight"));
    add(tu("Raster 16", [] { ke::editor().raster.setzePotenz(4); }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_G, "Strg+G"));
    add(pruefe("Strg+G: Ecken aufs Raster 16 (-56 -> -64, 72 -> 64)", [grenzen, nah] {
        const karte::Kasten b = grenzen();
        return nah(b.origin[0] - b.extents[0], -64) && nah(b.origin[0] + b.extents[0], 64);
    }));
    add(rueckgaengig("snapSelected -grid 16"));
    add(tu("Raster 8", [] { ke::editor().raster.setzePotenz(3); }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_C, "Strg+C"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_V, "Strg+V"));
    add(pruefe("Strg+C, Strg+V: eingefuegt und gewaehlt", [] {
        const ke::Editor& e = ke::editor();
        return e.karte.welt()->kinder().size() == 2 && e.karte.auswahl.anzahl() == 1;
    }));
    add(rueckgaengig("paste"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Y, "Strg+Y"));
    add(pruefe("Strg+Y: Wiederholen (Redo2), Konsole \"Redo: paste\"", [] {
        const ke::Editor& e = ke::editor();
        return e.karte.welt()->kinder().size() == 2 && !e.konsole.empty() && e.konsole.back().text == "Redo: paste";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    // 3D: Klick waehlt, Ziehen ohne Auswahl baut einen Pinsel
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add({"Kamera: M1-Klick in die Mitte (der Wuerfel)", [](int b) {
             const ke::KameraFenster& f = ke::editor().kamFenster;
             ImGuiIO& io = ImGui::GetIO();
             if (b == 0) { setzeMaus(ImVec2{f.bildX + static_cast<float>(f.breite) * 0.5F, f.bildY + static_cast<float>(f.hoehe) * 0.5F}); }
             if (b == 2) { io.AddMouseButtonEvent(ImGuiMouseButton_Left, true); }
             if (b == 3) { io.AddMouseButtonEvent(ImGuiMouseButton_Left, false); }
             return b >= 5;
         }});
    add(pruefe("Kamera: Tunnel-Auswahl waehlt den Pinsel", [pinsel] {
        const ke::Editor& e = ke::editor();
        return e.karte.auswahl.anzahl() == 1 && e.karte.auswahl.gewaehlt(e.karte.welt().get(), pinsel().get());
    }));
    add(tasteKombi(ImGuiKey_W, "W"));
    add(pause(0.3));
    add(foto("karteneditor_kamera_verschieben"));
    add(tasteKombi(ImGuiKey_W, "W (zurueck zu Resize)"));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(tu("Kamera 30 Grad nach unten (unter dem Wuerfel ist nichts)", [] { ke::editor().kamera.setzeWinkel(karte::Vektor3f(-30, 0, 0)); }));
    add(pause(0.2));
    add({"Kamera: M1 ziehen ohne Auswahl", [](int b) {
             const ke::KameraFenster& f = ke::editor().kamFenster;
             ImGuiIO& io = ImGui::GetIO();
             const ImVec2 a{f.bildX + static_cast<float>(f.breite) * 0.5F, f.bildY + static_cast<float>(f.hoehe) * 0.5F};
             const ImVec2 z{a.x + 120.0F, a.y - 60.0F};
             if (b == 0) { setzeMaus(a); }
             if (b == 1) { io.AddMouseButtonEvent(ImGuiMouseButton_Left, true); }
             if (b >= 2 && b <= 11) {
                 const float t = static_cast<float>(b - 1) / 10.0F;
                 setzeMaus(ImVec2{a.x + (z.x - a.x) * t, a.y + (z.y - a.y) * t});
             }
             if (b == 12) { io.AddMouseButtonEvent(ImGuiMouseButton_Left, false); }
             return b >= 14;
         }});
    add(pruefe("Kamera: neuer Pinsel (DragNewBrush), gewaehlt, Rueckgaengig \"dragTool\"", [] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Karten-Editor Auswahl - Pinsel " + std::to_string(e.karte.welt()->kinder().size()) + ", " +
                     e.karte.undo.undoName());
        return e.karte.welt()->kinder().size() == 2 && e.karte.auswahl.anzahl() == 1 && e.karte.undo.undoName() == "dragTool";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tu("Auswahl: Kamera zurueck, nichts gewaehlt", [] {
        ke::Editor& e = ke::editor();
        e.kamera.setzeWinkel(karte::Vektor3f(0, 0, 0));
        e.karte.auswahl.leeren();
    }));
    // Die uebrigen Befehle von Punkt 7 ueber ihre Knoepfe, Tasten und das Menue.
    auto knopfOrt = [](const std::string& id) {
        return [id]() {
            const Element* el = finde(id, "", 0, true);
            return el != nullptr ? el->rect.GetCenter() : ImVec2{0, 0};
        };
    };
    auto licht = []() -> karte::Entitaet* {
        for (const auto& ent : ke::editor().karte.entitaeten()) {
            if (ent->klassenname() == "light") { return ent.get(); }
        }
        return nullptr;
    };
    auto kinder = [] { return ke::editor().karte.welt()->kinder().size(); };
    auto anzahl = [] { return ke::editor().karte.auswahl.anzahl(); };
    auto konsoleZuletzt = [](const std::string& text) {
        const auto& k = ke::editor().konsole;
        return !k.empty() && k.back().text == text;
    };
    add(klickWelt("M1-Klick auf den Pinsel", ImGuiMouseButton_Left, 0, ImVec2{32, 32}));
    struct Werkzeug {
        const char* befehl;
        karte::ManipModus modus;
    };
    for (const Werkzeug& w : {Werkzeug{"MouseTranslate", karte::ManipModus::Verschieben}, Werkzeug{"MouseRotate", karte::ManipModus::Drehen},
                              Werkzeug{"MouseScale", karte::ManipModus::Skalieren}, Werkzeug{"MouseTransform", karte::ManipModus::Verzerren},
                              Werkzeug{"MouseDrag", karte::ManipModus::Ziehen}}) {
        const std::string b = w.befehl;
        const karte::ManipModus m = w.modus;
        add(klickAn(std::string("Knopf ") + b, knopfOrt("##knopf" + b)));
        add(pruefe("Knopf " + b + ": Werkzeug gewaehlt, Knopf gedrueckt",
                   [b, m] { return ke::editor().bearbeitung.modus() == m && ke::angehakt(b); }));
    }
    add(klickAn("Knopf MouseScale", knopfOrt("##knopfMouseScale")));
    add(klickAn("Knopf MouseScale noch einmal", knopfOrt("##knopfMouseScale")));
    add(pruefe("Dasselbe Werkzeug noch einmal: zurueck zu Resize (g_defaultToolMode)",
               [] { return ke::editor().bearbeitung.modus() == karte::ManipModus::Ziehen && ke::angehakt("MouseDrag"); }));
    add(tu("MouseRotateOrScale", [] { (void)ke::ausfuehren("MouseRotateOrScale"); }));
    add(pruefe("MouseRotateOrScale aus Resize: Rotate", [] { return ke::editor().bearbeitung.modus() == karte::ManipModus::Drehen; }));
    add(tu("MouseRotateOrScale", [] { (void)ke::ausfuehren("MouseRotateOrScale"); }));
    add(pruefe("MouseRotateOrScale aus Rotate: Scale", [] { return ke::editor().bearbeitung.modus() == karte::ManipModus::Skalieren; }));
    add(tu("MouseDrag", [] { (void)ke::ausfuehren("MouseDrag"); }));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_Space, "Umschalt+Leertaste"));
    add(pruefe("Umschalt+Leertaste: geklont (CloneSelectionAndMakeUnique)", [kinder, anzahl] { return kinder() == 2 && anzahl() == 1; }));
    add(rueckgaengig("cloneSelectedMakeUnique"));
    // Undo und Redo stehen in NRCs g_toolbarHiddenButtons (gtkmisc.cpp:147-160):
    // erst ueber das Leistenmenue zeigen (dort schon geprueft), danach wieder weg.
    add(tu("Knoepfe Undo und Redo zeigen", [] {
        ke::editor().versteckteKnoepfe.erase("Undo");
        ke::editor().versteckteKnoepfe.erase("Redo");
    }));
    add(pause(0.2));
    add(klickAn("Knopf Undo", knopfOrt("##knopfUndo")));
    add(pruefe("Knopf Undo: Klon weg, Konsole \"Undo: cloneSelectedMakeUnique\"",
               [kinder, konsoleZuletzt] { return kinder() == 1 && konsoleZuletzt("Undo: cloneSelectedMakeUnique"); }));
    add(klickAn("Knopf Redo", knopfOrt("##knopfRedo")));
    add(pruefe("Knopf Redo: Klon wieder da, Konsole \"Redo: cloneSelectedMakeUnique\"",
               [kinder, konsoleZuletzt] { return kinder() == 2 && konsoleZuletzt("Redo: cloneSelectedMakeUnique"); }));
    add(tu("Knoepfe Undo und Redo wieder verstecken", [] {
        ke::editor().versteckteKnoepfe.insert("Undo");
        ke::editor().versteckteKnoepfe.insert("Redo");
    }));
    add(klickWelt("M1-Klick auf den Pinsel", ImGuiMouseButton_Left, 0, ImVec2{32, 32}));
    add(tasteKombi(ImGuiKey_Backspace, "Ruecktaste"));
    add(pruefe("Ruecktaste: geloescht (DeleteSelection2)", [kinder, anzahl] { return kinder() == 1 && anzahl() == 0; }));
    add(rueckgaengig("deleteSelected"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, "Strg+Umschalt+Z"));
    add(pruefe("Strg+Umschalt+Z: Wiederholen (Redo), Konsole \"Redo: deleteSelected\"",
               [kinder, konsoleZuletzt] { return kinder() == 1 && konsoleZuletzt("Redo: deleteSelected"); }));
    add(klickWelt("M1-Klick auf den Pinsel", ImGuiMouseButton_Left, 0, ImVec2{32, 32}));
    add(tasteKombi(ImGuiKey_Z, "Z"));
    add(pruefe("Z: geloescht (DeleteSelection)", [kinder] { return kinder() == 0; }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(klickWelt("M1-Klick auf den Pinsel", ImGuiMouseButton_Left, 0, ImVec2{32, 32}));
    add(pruefe("Pinsel wieder da und gewaehlt", [kinder, anzahl] { return kinder() == 1 && anzahl() == 1; }));
    add(tasteKombi(ImGuiKey_C, "C"));
    add(pruefe("C: nichts gewaehlt (UnSelectSelection)", [anzahl] { return anzahl() == 0; }));
    add(tasteKombi(ImGuiKey_I, "I"));
    add(pruefe("I: Pinsel und Licht gewaehlt (InvertSelection)", [anzahl] { return anzahl() == 2; }));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(pruefe("Esc: nichts gewaehlt (UnSelectSelection2)", [anzahl] { return anzahl() == 0; }));
    add(klick(ke::trK(Str::KeLEdit), "###main"));
    add(klick(ke::trK(Str::KeLInvertSelection)));
    add(pause(0.2));
    add(pruefe("Edit > Invert Selection: Pinsel und Licht gewaehlt", [anzahl] { return anzahl() == 2; }));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(klickWelt("M1-Klick auf das Licht (0 0)", ImGuiMouseButton_Left, 0, ImVec2{0, 0}));
    add(pruefe("Licht gewaehlt (Punkt-Entities in 2D bevorzugt)",
               [licht] { return ke::editor().karte.auswahl.anzahl() == 1 && ke::editor().karte.auswahl.gewaehlt(licht()); }));
    add(tasteKombi(ImGuiKey_Space, "Leertaste"));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_A, "Umschalt+A"));
    add(pruefe("Umschalt+A: beide Lichter gewaehlt (SelectAllOfType)",
               [anzahl] { return ke::editor().karte.entitaeten().size() == 3 && anzahl() == 2; }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_V, "Umschalt+V"));
    add(pruefe("Umschalt+V: eingefuegt bei der Kamera (-256 0 32), gewaehlt (PasteToCamera)", [kinder, anzahl, nah] {
        const ke::Editor& e = ke::editor();
        const auto p = std::dynamic_pointer_cast<karte::Pinsel>(e.karte.welt()->kinder().back());
        const karte::Kasten b = p ? p->grenzen(e.karte.kontext) : karte::Kasten();
        return kinder() == 2 && anzahl() == 1 && nah(b.origin[0], -256) && nah(b.origin[1], 0) && nah(b.origin[2], 32);
    }));
    add(rueckgaengig("pasteToCamera"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(klickWelt("M1-Klick auf den Pinsel", ImGuiMouseButton_Left, 0, ImVec2{32, 32}));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_V, "Strg+Umschalt+V"));
    add(pruefe("Strg+Umschalt+V: zur Kamera verschoben (MoveToCamera)", [grenzen, nah] { return nah(grenzen().origin[0], -256); }));
    add(rueckgaengig("moveToCamera"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tasteKombi(ImGuiMod_Alt | ImGuiKey_LeftArrow, "Alt+Links"));
    add(pruefe("Alt+Links: 8 nach links", [grenzen, nah] { return nah(grenzen().origin[0], -8); }));
    add(rueckgaengig("nudgeSelectedLeft"));
    add(tasteKombi(ImGuiMod_Alt | ImGuiKey_UpArrow, "Alt+Hoch"));
    add(pruefe("Alt+Hoch: 8 nach oben (+y)", [grenzen, nah] { return nah(grenzen().origin[1], 8); }));
    add(rueckgaengig("nudgeSelectedUp"));
    add(tasteKombi(ImGuiMod_Alt | ImGuiKey_DownArrow, "Alt+Runter"));
    add(pruefe("Alt+Runter: zurueck auf y 0", [grenzen, nah] { return nah(grenzen().origin[1], 0); }));
    add(rueckgaengig("nudgeSelectedDown"));
    add(tasteKombi(ImGuiKey_KeypadAdd, "Ziffernblock +"));
    add(pruefe("Ziffernblock +: 8 hoch (MoveSelectionUP)", [grenzen, nah] { return nah(grenzen().origin[2], 40); }));
    add(rueckgaengig("nudgeSelected -axis z -amount 8"));
    add(tasteKombi(ImGuiKey_KeypadSubtract, "Ziffernblock -"));
    add(pruefe("Ziffernblock -: 8 runter (MoveSelectionDOWN)", [grenzen, nah] { return nah(grenzen().origin[2], 32); }));
    add(rueckgaengig("nudgeSelected -axis z -amount -8"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pruefe("fuenfmal Strg+Z: wieder bei (0 0 32)", [grenzen, nah] {
        const karte::Kasten b = grenzen();
        return nah(b.origin[0], 0) && nah(b.origin[1], 0) && nah(b.origin[2], 32);
    }));
    // Hide Selected ist der letzte Filterknopf und steht bei dieser Fensterhoehe
    // (wie in NRC) hinter dem Erweiterungsknopf: die Filterknoepfe davor
    // verstecken (Leistenmenue), dann ist er sichtbar.
    add(tu("Filterknoepfe vor Hide Selected verstecken", [] {
        for (const kb::Knopf& k : kb::filterleiste()) {
            if (!k.trenner && k.befehl[0] != 0 && std::string(k.befehl) != "HideSelected") { ke::editor().versteckteKnoepfe.insert(k.befehl); }
        }
    }));
    add(pause(0.2));
    add(klickAn("Filterleiste: Knopf Hide Selected", knopfOrt("##knopfHideSelected")));
    add(pruefe("Knopf Hide Selected: versteckt, Knopf gedrueckt", [pinsel] { return pinsel()->versteckt && ke::angehakt("HideSelected"); }));
    add(klickAn("Filterleiste: Rechtsklick auf Hide Selected", knopfOrt("##knopfHideSelected"), 1));
    add(pruefe("Rechtsklick: Show Hidden (FilterToolbarHandler)", [pinsel] { return !pinsel()->versteckt && !ke::angehakt("HideSelected"); }));
    add(tu("Filterknoepfe wieder zeigen, nichts gewaehlt", [] {
        for (const kb::Knopf& k : kb::filterleiste()) {
            if (!k.trenner && k.befehl[0] != 0) { ke::editor().versteckteKnoepfe.erase(k.befehl); }
        }
        ke::editor().karte.auswahl.leeren();
    }));
    // Einstellungsseite "Selection" (Vergleich mit SP\nrc_fotos\dialoge\pref_Selection.png)
    add(tu("Einstellungen oeffnen", [] { (void)ke::ausfuehren("Preferences"); }));
    add(pause(0.3));
    add(klick(ke::trK(Str::KePgSelection), "###karteeinstellungen"));
    add(pause(0.2));
    add(pruefe("Seite Selection: vier Felder, Vorgaben wie NRC (12, an, an, XY plane + Z with Alt)", [] {
        const ke::Editor& e = ke::editor();
        // Die Beschriftungen links sind Text (kein Element), das Klappfeld meldet
        // ImGui nicht (BeginCombo ohne IMGUI_TEST_ENGINE_ITEM_INFO): es wird
        // unten ueber seine Zeile geprueft.
        const bool felder = finde("##waehlergroesse", "", 0, true) != nullptr &&
                            finde(ke::trK(Str::KePrSelPreferPoint), "", 0, false) != nullptr &&
                            finde(ke::trK(Str::KePrSel3dBrushes), "", 0, false) != nullptr;
        diag::detail(std::string("Knopftest: Karten-Editor Auswahl - Seite Selection, Felder ") + (felder ? "da" : "fehlen") + ", " +
                     std::to_string(e.bearbeitung.waehlEpsilon) + " " + std::to_string(e.bearbeitung.einstellung.punktEntitaetenIn2d) +
                     std::to_string(e.bearbeitung.pinselIn3d) + " " + std::to_string(e.bearbeitung.bewegStil3d));
        return felder && e.bearbeitung.waehlEpsilon == 12.0F && e.bearbeitung.einstellung.punktEntitaetenIn2d &&
               e.bearbeitung.pinselIn3d && e.bearbeitung.bewegStil3d == 0;
    }));
    add(foto("karteneditor_einstellungen_auswahl"));
    // Das Klappfeld "Move style in 3D" liegt eine Zeile unter "Create brushes in 3D".
    add(klickAn("Klappfeld Move style in 3D", [] {
        const Element* a = finde(ke::trK(Str::KePrSelPreferPoint), "", 0, false);
        const Element* b = finde(ke::trK(Str::KePrSel3dBrushes), "", 0, false);
        if (a == nullptr || b == nullptr) { return ImVec2{0, 0}; }
        const ImVec2 c = b->rect.GetCenter();
        return ImVec2{c.x, c.y + (c.y - a->rect.GetCenter().y)};
    }));
    add(pause(0.2));
    add(klick(ke::trK(Str::KePrSelMoveView)));
    add(klick(ke::trK(Str::KePrSel3dBrushes)));
    add(klick(ke::trK(Str::KePrefOk)));
    add(pause(0.2));
    add(pruefe("Einstellungen: Move style in 3D = View plane + Forward with Alt", [] { return ke::editor().bearbeitung.bewegStil3d == 1; }));
    add(pruefe("Einstellungen: Create brushes in 3D aus", [] { return !ke::editor().bearbeitung.pinselIn3d; }));
    add({"Kamera: M1 ziehen ohne Auswahl", [](int b) {
             const ke::KameraFenster& f = ke::editor().kamFenster;
             ImGuiIO& io = ImGui::GetIO();
             const ImVec2 a{f.bildX + static_cast<float>(f.breite) * 0.5F, f.bildY + static_cast<float>(f.hoehe) * 0.5F};
             const ImVec2 z{a.x + 120.0F, a.y - 60.0F};
             if (b == 0) {
                 ke::editor().kamera.setzeWinkel(karte::Vektor3f(-30, 0, 0));
                 setzeMaus(a);
             }
             if (b == 1) { io.AddMouseButtonEvent(ImGuiMouseButton_Left, true); }
             if (b >= 2 && b <= 11) {
                 const float t = static_cast<float>(b - 1) / 10.0F;
                 setzeMaus(ImVec2{a.x + (z.x - a.x) * t, a.y + (z.y - a.y) * t});
             }
             if (b == 12) { io.AddMouseButtonEvent(ImGuiMouseButton_Left, false); }
             return b >= 14;
         }});
    add(pruefe("Ohne Create brushes in 3D: kein neuer Pinsel in der Kamera (g_3DCreateBrushes)", [kinder] { return kinder() == 1; }));
    add(tu("Create brushes in 3D wieder an, Move style XY, Kamera zurueck, nichts gewaehlt", [] {
        ke::Editor& e = ke::editor();
        e.bearbeitung.pinselIn3d = true;
        e.bearbeitung.bewegStil3d = 0;
        e.kamera.setzeWinkel(karte::Vektor3f(0, 0, 0));
        e.karte.auswahl.leeren();
    }));
    return s;
}

// Punkt 8: der Texturbrowser (texwindow.cpp) - Lagen, Baum, Klick (M1 setzt
// den Shader, M3 waehlt nur), Filter, View-Menue, U / Ctrl+A, Rad, M2-Ziehen,
// Kontextmenue, Shader Info / Text, Doppelklick, Refresh, Find / Replace,
// Einstellungsseite, Teiler.
std::vector<Schritt> karteneditorTexturSchritte() {
    namespace ke = karteneditor;
    namespace kb = karte::befehle;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto feldVon = [](const std::string& name) -> const ke::Editor::Rechteck* {
        for (const auto& r : ke::editor().felderImBild) {
            if (r.name == name) { return &r; }
        }
        return nullptr;
    };
    auto knopfOrt = [](const std::string& id) {
        return [id]() {
            const Element* el = finde(id, "", 0, true);
            return el != nullptr ? el->rect.GetCenter() : ImVec2{0, 0};
        };
    };
    auto texturOrt = [](const std::string& shader) {
        return [shader]() {
            ImVec2 p{0, 0};
            (void)ke::texturMitte(shader, p);
            return p;
        };
    };
    auto pinsel = [] {
        const auto w = ke::editor().karte.welt();
        return (w && !w->kinder().empty()) ? std::dynamic_pointer_cast<karte::Pinsel>(w->kinder().front()) : nullptr;
    };
    auto namen = [] {
        std::vector<std::string> v;
        ke::Editor& e = ke::editor();
        for (const auto& l : e.texBro.anordnen(e.aktiveShader)) { v.push_back(l.shader->name); }
        return v;
    };
    auto konsoleHat = [](const std::string& anfang) {
        for (const auto& z : ke::editor().konsole) {
            if (z.text.rfind(anfang, 0) == 0) { return true; }
        }
        return false;
    };
    add(tu("Texturen: Regular, Testkarte neu, nichts gewaehlt", [] {
        ke::Editor& e = ke::editor();
        e.anordnung = kb::Anordnung::Regular;
        e.gruppenDialogEingebaut = false;
        e.teilerFuer = -1;
        (void)ke::karteOeffnen(karteTestPfad("karte_kopie.map"));
        e.karte.auswahl.leeren();
        e.texGroessen.clear();
        e.texTeiler = karte::Teiler{};
        e.konsole.clear();
    }));
    add(pause(0.4));
    add(pruefe("Lagen wie NRC: links 100 breit ({ 100, 800 }), Leiste 25, Knoepfe 21 bei 2 / 24 / 46, Filter 22, Rollbalken 7",
               [feldVon] {
                   const auto* l = feldVon("Texturen:Links");
                   const auto* k0 = feldVon("Texturen:Knopf:texbro_view");
                   const auto* k1 = feldVon("Texturen:Knopf:FindReplaceTextures");
                   const auto* k2 = feldVon("Texturen:Knopf:RefreshShaders");
                   const auto* f = feldVon("Texturen:Filter");
                   const auto* r = feldVon("Texturen:Rollbalken");
                   const auto* a = feldVon("Texturen:Ansicht");
                   if (l == nullptr || k0 == nullptr || k1 == nullptr || k2 == nullptr || f == nullptr || r == nullptr || a == nullptr) {
                       return false;
                   }
                   diag::detail("Knopftest: Texturen - links " + std::to_string(l->b) + ", Ansicht " + std::to_string(a->x) + " " +
                                std::to_string(a->b) + "x" + std::to_string(a->h) + ", Rollbalken " + std::to_string(r->b));
                   return l->b == 100 && k0->x == l->x + 2 && k1->x == l->x + 24 && k2->x == l->x + 46 && k0->b == 21 &&
                          k0->y == l->y + 2 && f->y == l->y + 25 && f->h == 22 && r->b == 7 && a->x == l->x + 104;
               }));
    add(pruefe("Nach dem Laden: textures/ (gewaehlt) und der Shader der Karte, in Gebrauch", [namen] {
        ke::Editor& e = ke::editor();
        const auto n = namen();
        const karte::AktiverShader* c = e.aktiveShader.finden("textures/common/caulk");
        std::string t;
        for (const auto& x : n) { t += " " + x; }
        diag::detail("Knopftest: Texturen - gezeigt:" + t);
        return n.size() == 2 && n[0] == "textures/" && n[1] == "textures/common/caulk" && c != nullptr && c->imGebrauch &&
               e.texBro.shader == "textures/" && ke::texturTitel() == "Textures: all";
    }));
    add(warteBis("Texturen: Bilder geladen", [] { return ke::kameraBilderFertig(); }, 2000));
    add(foto("karteneditor_texturen_start"));
    // Verzeichnisbaum: Doppelklick auf den ersten Ordner (activated, Fusion)
    add({"Baum: Doppelklick auf den ersten Ordner", [](int b) {
             static Schritt k;
             static std::string pfad;
             if (b == 0) {
                 pfad.clear();
                 for (int i = 0; i < 40; ++i) {
                     const Element* el = finde("##tb", "", i, true);
                     if (el == nullptr) { break; }
                     if (el->label.rfind("##tb#", 0) != 0) {
                         pfad = el->label.substr(4);
                         const ImVec2 m = el->rect.GetCenter();
                         k = klickAn("Ordner " + pfad, [m] { return m; }, 0, true);
                         break;
                     }
                 }
                 diag::detail("Knopftest: Texturen - erster Ordner " + pfad);
                 if (pfad.empty()) { return true; }
             }
             return k.tun(b);
         }});
    add(pause(0.2));
    add(pruefe("Doppelklick: Verzeichnis gezeigt, Konsole \"Showing N shaders.\", Titel", [konsoleHat] {
        ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Texturen - Verzeichnis " + e.texBro.verzeichnis + ", " + ke::texturTitel());
        return !e.texBro.verzeichnis.empty() && e.texBro.verzeichnis.back() == '/' && konsoleHat("Showing ") &&
               ke::texturTitel() == "Textures: " + e.texBro.verzeichnis;
    }));
    add(tu("system/ zeigen (TextureBrowser_ShowDirectory)", [] { ke::texturVerzeichnisZeigen("system/"); }));
    add(pause(0.3));
    add(pruefe("system/: Shader aus shaders/system.shader, caulk dabei", [namen] {
        const auto n = namen();
        diag::detail("Knopftest: Texturen - system/: " + std::to_string(n.size()) + " Texturen");
        return std::find(n.begin(), n.end(), "textures/system/caulk") != n.end() &&
               std::all_of(n.begin(), n.end(), [](const std::string& x) { return x.rfind("textures/system/", 0) == 0; });
    }));
    add(warteBis("Texturen: Bilder von system/ geladen", [] { return ke::kameraBilderFertig(); }, 2000));
    add(foto("karteneditor_texturen"));
    // M1 auf caulk mit gewaehltem Pinsel: Shader gesetzt, Rueckgaengig
    add(tu("Pinsel waehlen", [pinsel] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), pinsel());
    }));
    add(klickAn("M1 auf system/caulk", texturOrt("textures/system/caulk")));
    add(pruefe("M1: gewaehlt, auf den Pinsel gesetzt (textureNameSetSelected), Status \"system/caulk W: ...\"", [pinsel] {
        ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Texturen - Status " + e.statusTextur + ", Rueckgaengig " + e.karte.undo.undoName());
        return e.texBro.shader == "textures/system/caulk" && pinsel()->flaechen()[0]->shader() == "textures/system/caulk" &&
               e.karte.undo.undoName() == "textureNameSetSelected" && e.statusTextur.rfind("system/caulk W: ", 0) == 0;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pruefe("Strg+Z: wieder common/caulk", [pinsel] { return pinsel()->flaechen()[0]->shader() == "textures/common/caulk"; }));
    add({"M3 auf eine andere Textur (waehlt nur)", [namen](int b) {
             static Schritt k;
             if (b == 0) {
                 std::string ziel;
                 for (const auto& n : namen()) {
                     if (n != "textures/system/caulk") {
                         ziel = n;
                         break;
                     }
                 }
                 k = klickAn("M3 auf " + ziel, [ziel] {
                     ImVec2 p{0, 0};
                     (void)ke::texturMitte(ziel, p);
                     return p;
                 }, 2);
             }
             return k.tun(b);
         }});
    add(pruefe("M3: gewaehlt, der Pinsel behaelt seinen Shader", [pinsel] {
        ke::Editor& e = ke::editor();
        return e.texBro.shader != "textures/system/caulk" && e.texBro.shader.rfind("textures/system/", 0) == 0 &&
               pinsel()->flaechen()[0]->shader() == "textures/common/caulk";
    }));
    // Filterfeld: Fokus unter der Maus, Tippen filtert, Esc leert
    add({"Maus ueber das Filterfeld", [](int b) {
             const Element* el = finde("##texfilter", "", 0, true);
             if (b == 0 && el != nullptr) { setzeMaus(el->rect.GetCenter()); }
             return b >= 4;
         }});
    add({"Tippe \"caulk\"", [](int b) {
             if (b == 0) { ImGui::GetIO().AddInputCharactersUTF8("caulk"); }
             return b >= 4;
         }});
    add(pruefe("Filter \"caulk\": nur Namen mit caulk", [namen] {
        const auto n = namen();
        return !n.empty() && ke::editor().texBro.filter == "caulk" &&
               std::all_of(n.begin(), n.end(), [](const std::string& x) { return x.find("caulk") != std::string::npos; });
    }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pruefe("Esc im Filterfeld: leer (ShortcutOverride), keine Auswahl aufgehoben", [] {
        ke::Editor& e = ke::editor();
        return e.texBro.filter.empty() && e.texFilterText.empty() && e.karte.auswahl.anzahl() == 1;
    }));
    add(klickAn("Suchart umschalten (Lupe)", knopfOrt("##texfiltermodus")));
    add(pruefe("SearchFromStart an", [] { return ke::angehakt("SearchFromStart"); }));
    add(klickAn("Suchart umschalten (Lupe)", knopfOrt("##texfiltermodus")));
    add(pruefe("SearchFromStart aus", [] { return !ke::angehakt("SearchFromStart"); }));
    // View-Menue
    add(klickAn("Knopf View", knopfOrt("##texknopftexbro_view")));
    add(pause(0.2));
    add(klick(ke::trK(Str::KeLHideUnused)));
    add(pause(0.2));
    add(pruefe("View > Hide Unused: in system/ ist nichts benutzt", [namen] { return ke::angehakt("ShowInUse") && namen().empty(); }));
    add(tasteKombi(ImGuiKey_U, "U"));
    add(pruefe("U: wieder alle", [namen] { return !ke::angehakt("ShowInUse") && !namen().empty(); }));
    // Die uebrigen Schalter des View-Menues, je ueber das Menue an und wieder aus
    struct Schalter {
        Str text;
        const char* befehl;
    };
    static int vorher = 0;
    for (const Schalter& sw : {Schalter{Str::KeLShowShaders, "ToggleShowShaders"}, Schalter{Str::KeLShowTextures, "ToggleShowTextures"},
                               Schalter{Str::KeLHideImageMissing, "FilterNotex"}, Schalter{Str::KeLFixedSize, "FixedSize"},
                               Schalter{Str::KeLTransparency, "EnableAlpha"}, Schalter{Str::KeLShaderListOnly, "ToggleShowShaderlistOnly"}}) {
        const std::string befehl = sw.befehl;
        const bool an = befehl == "ToggleShowShaders" || befehl == "ToggleShowTextures" || befehl == "FixedSize";   // Vorgaben
        add(tu("merken (" + befehl + ")", [befehl] {
            ke::Editor& e = ke::editor();
            if (befehl == "FixedSize") {
                const karte::AktiverShader* v = e.aktiveShader.finden("textures/system/caulk");
                vorher = v != nullptr ? e.texBro.texturMass(*v).first : -1;
            } else {
                vorher = static_cast<int>(e.texBro.anordnen(e.aktiveShader).size());
            }
        }));
        add(klickAn("Knopf View", knopfOrt("##texknopftexbro_view")));
        add(pause(0.2));
        add(klick(ke::trK(sw.text)));
        add(pause(0.2));
        add(pruefe("View > " + std::string(ke::trK(sw.text)) + ": umgeschaltet, Wirkung", [befehl, an] {
            ke::Editor& e = ke::editor();
            const karte::TexturBrowserEinstellungen& t = e.texBro.e;
            const int n = static_cast<int>(e.texBro.anordnen(e.aktiveShader).size());
            diag::detail("Knopftest: Texturen - " + befehl + ": vorher " + std::to_string(vorher) + ", nachher " + std::to_string(n));
            if (ke::angehakt(befehl) == an) { return false; }
            const auto alle = [&e](auto bed) {
                const auto l = e.texBro.anordnen(e.aktiveShader);
                return std::all_of(l.begin(), l.end(), [&](const karte::TexturLage& x) { return bed(*x.shader); });
            };
            if (befehl == "ToggleShowShaders") {   // nur Bilder ohne Shader
                return !t.shaderZeigen && n < vorher && alle([](const karte::AktiverShader& s) { return s.standard; });
            }
            if (befehl == "ToggleShowTextures") {
                return !t.texturenZeigen && n <= vorher && alle([](const karte::AktiverShader& s) { return !s.standard; });
            }
            if (befehl == "FilterNotex") { return t.ohneNotex && n < vorher && alle([](const karte::AktiverShader& s) { return !s.bildFehlt; }); }
            if (befehl == "FixedSize") {   // caulk 64 x 64: mit Fixed Size 48 breit, ohne 32 (50 %)
                const karte::AktiverShader* v = e.aktiveShader.finden("textures/system/caulk");
                return !t.festeGroesse && v != nullptr && vorher == 48 && e.texBro.texturMass(*v).first == 32;
            }
            if (befehl == "EnableAlpha") { return t.transparenz; }
            return t.nurShaderliste;
        }));
        if (befehl == "EnableAlpha") { add(foto("karteneditor_texturen_transparenz")); }
        add(tu("zurueck (" + befehl + ")", [befehl] { (void)ke::ausfuehren(befehl); }));
    }
    // T (ToggleTextures): in Regular ohne eingebauten Gruppendialog zeigt
    // GroupDialog_showPage( nullptr ) nur das Fenster.
    add(tasteKombi(ImGuiKey_T, "T"));
    add(pruefe("T in Regular: der Gruppendialog ist offen", [] { return ke::editor().gruppenDialogOffen; }));
    add(tu("Gruppendialog zu", [] { ke::editor().gruppenDialogOffen = false; }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_A, "Strg+A"));
    add(pruefe("Strg+A (Show All) einmal: alle Verzeichnisse, nur benutzte", [namen] {
        const auto n = namen();
        return ke::editor().texBro.verzeichnis.empty() && n.size() == 1 && n[0] == "textures/common/caulk";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_A, "Strg+A"));
    add(pruefe("Strg+A zweimal: alle aktiven Shader", [namen] { return namen().size() > 3; }));
    // Rad und M2-Ziehen
    add({"Rad nach unten ueber der Ansicht", [](int b) {
             const ke::TexturBrowserLage& l = ke::texturBrowserLage();
             if (b == 0) { setzeMaus(ImVec2{l.ansichtX + 20.0F, l.ansichtY + static_cast<float>(l.hoehe) * 0.5F}); }
             if (b == 2) { ImGui::GetIO().AddMouseWheelEvent(0.0F, -1.0F); }
             return b >= 5;
         }});
    add(pruefe("Rad: um 64 gerollt (geklemmt)", [] {
        ke::Editor& e = ke::editor();
        const int gesamt = e.texBro.gesamtHoehe(e.aktiveShader);
        const int soll = std::clamp(-64, std::min(e.texBro.hoehe - gesamt, 0), 0);
        diag::detail("Knopftest: Texturen - Ursprung " + std::to_string(e.texBro.ursprungY(e.aktiveShader)) + ", soll " +
                     std::to_string(soll) + ", gesamt " + std::to_string(gesamt) + ", hoehe " + std::to_string(e.texBro.hoehe));
        return e.texBro.ursprungY(e.aktiveShader) == soll;
    }));
    static int vorZiehen = 0;
    add(tu("Ursprung merken", [] { vorZiehen = ke::editor().texBro.ursprungY(ke::editor().aktiveShader); }));
    add(ziehenAn("M2 nach unten ziehen (40 Bildpunkte)", [] {
        const ke::TexturBrowserLage& l = ke::texturBrowserLage();
        return ImVec2{l.ansichtX + 20.0F, l.ansichtY + 10.0F};
    }, 0.0F, 40.0F, 1));
    add(pause(0.2));
    add(pruefe("M2 gezogen: nach oben gerollt (originy + dy), kein Kontextmenue (Weg >= 16)", [] {
        ke::Editor& e = ke::editor();
        const int nach = e.texBro.ursprungY(e.aktiveShader);
        diag::detail("Knopftest: Texturen - M2 gezogen " + std::to_string(vorZiehen) + " -> " + std::to_string(nach));
        return (nach > vorZiehen || nach == 0) && !ImGui::IsPopupOpen("##texmenue", ImGuiPopupFlags_AnyPopupId);
    }));
    add(tu("common/caulk ins Bild (TextureBrowser_Focus)", [] { ke::editor().texBro.fokus("textures/common/caulk", ke::editor().aktiveShader); }));
    add(pause(0.2));
    // Kontextmenue
    add(klickAn("M2 auf common/caulk", texturOrt("textures/common/caulk"), 1));
    add(pause(0.2));
    add(klick(ke::trK(Str::KeTexSelObjects)));
    add(pause(0.2));
    add(pruefe("Select Textured Objects: der Pinsel gewaehlt", [pinsel] {
        ke::Editor& e = ke::editor();
        return e.karte.auswahl.gewaehlt(e.karte.welt().get(), pinsel().get());
    }));
    add(tu("nichts gewaehlt", [] { ke::editor().karte.auswahl.leeren(); }));
    add(klickAn("M2 auf common/caulk", texturOrt("textures/common/caulk"), 1));
    add(pause(0.2));
    add(klick(ke::trK(Str::KeTexSelSurfaces)));
    add(pause(0.2));
    add(pruefe("Select Textured Surfaces: sechs Flaechen", [] { return ke::editor().karte.auswahl.flaechen().size() == 6; }));
    add(tu("keine Flaechen", [] { ke::editor().karte.auswahl.flaechenLeeren(); }));
    add(klickAn("M2 auf common/caulk", texturOrt("textures/common/caulk"), 1));
    add(pause(0.2));
    add(pruefe("Kontextmenue: alle Eintraege da (Name, Open Shader, Shader Info, Select ..., Recent, Shader's Folder, system/)", [] {
        for (const char* t : {ke::trK(Str::KeTexOpenShader), ke::trK(Str::KeTexOpenShaderExt), ke::trK(Str::KeTexShaderInfo),
                              ke::trK(Str::KeTexSelObjects), ke::trK(Str::KeTexSelSurfaces), ke::trK(Str::KeTexRecent),
                              ke::trK(Str::KeTexShaderFolder)}) {
            if (finde(t, "", 0, false) == nullptr) {
                diag::detail(std::string("Knopftest: Texturen - im Kontextmenue fehlt ") + t);
                return false;
            }
        }
        return finde("textures/common/caulk##texkopieren", "", 0, true) != nullptr ||
               finde("     textures/common/caulk##texkopieren", "", 0, false) != nullptr;
    }));
    add(klick("-> system/##texcommon"));
    add(pause(0.2));
    add(pruefe("-> system/: das Verzeichnis", [] { return ke::editor().texBro.verzeichnis == "system/"; }));
    add({"M2 auf einen Shader mit Datei (system/)", [](int b) {
             static Schritt k;
             if (b == 0) {
                 ke::Editor& e = ke::editor();
                 std::string ziel;
                 for (const auto& l : e.texBro.anordnen(e.aktiveShader)) {
                     if (!l.shader->standard) {
                         ziel = l.shader->name;
                         break;
                     }
                 }
                 k = klickAn("M2 auf " + ziel, [ziel] {
                     ImVec2 p{0, 0};
                     (void)ke::texturMitte(ziel, p);
                     return p;
                 }, 1);
             }
             return k.tun(b);
         }});
    add(pause(0.2));
    add(klick(ke::trK(Str::KeTexShaderInfo)));
    add(pause(0.2));
    add(pruefe("Shader Info: Name und Datei shaders/system.shader", [] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Texturen - Shader Info " + e.shaderInfoName + " in " + e.shaderInfoDatei);
        return e.shaderInfoName.rfind("textures/system/", 0) == 0 && e.shaderInfoDatei == "shaders/system.shader";
    }));
    add(foto("karteneditor_texturen_shaderinfo"));
    add(klick(ke::trK(Str::KePrefOk)));
    add(pause(0.2));
    add({"Umschalt+M1 auf einen Shader (Text ansehen)", [](int b) {
             static Schritt k;
             ImGuiIO& io = ImGui::GetIO();
             if (b == 0) {
                 ke::Editor& e = ke::editor();
                 std::string ziel;
                 for (const auto& l : e.texBro.anordnen(e.aktiveShader)) {
                     if (!l.shader->standard) {
                         ziel = l.shader->name;
                         break;
                     }
                 }
                 io.AddKeyEvent(ImGuiMod_Shift, true);
                 k = klickAn("Umschalt+M1 auf " + ziel, [ziel] {
                     ImVec2 p{0, 0};
                     (void)ke::texturMitte(ziel, p);
                     return p;
                 });
             }
             const bool fertig = k.tun(b);
             if (fertig) { io.AddKeyEvent(ImGuiMod_Shift, false); }
             return fertig;
         }});
    add(pause(0.3));
    add(pruefe("Shader-Text: offen, Titel <Archiv>::shaders/system.shader (nur lesen) bzw. <Ordner>shaders/system.shader "
               "(lose Datei: bearbeitbar), zur Definition gerollt",
               [] {
                   const ke::Editor::ShaderText& t = ke::editor().shaderText;
                   diag::detail("Knopftest: Texturen - Shader-Text " + t.datei + ", Cursor " + std::to_string(t.cursor) +
                                (t.editierbar ? ", bearbeitbar" : ", nur lesen"));
                   const bool imArchiv = t.datei.find("::shaders/system.shader") != std::string::npos;
                   const bool lose = t.datei.size() > 21 && t.datei.compare(t.datei.size() - 21, 21, "shaders/system.shader") == 0;
                   return t.offen && t.cursor > 0 && ((imArchiv && !t.editierbar) || (!imArchiv && lose && t.editierbar));
               }));
    add(foto("karteneditor_texturen_shadertext"));
    add(tu("Shader-Text schliessen", [] { ke::editor().shaderText.offen = false; }));
    add(pause(0.2));
    // Doppelklick: das Verzeichnis des gewaehlten Shaders
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_A, "Strg+A"));
    add(tu("alle zeigen, textures/ gewaehlt", [] {
        ke::Editor& e = ke::editor();
        if (e.texBro.unbenutzteVerstecken) { e.texBro.unbenutzteSetzen(false); }
        ke::texturShaderWaehlen("textures/common/caulk");
    }));
    add(pause(0.2));
    add(klickAn("Doppelklick (M1) auf common/caulk", texturOrt("textures/common/caulk"), 0, true));
    add(pause(0.2));
    add(pruefe("Doppelklick: Verzeichnis des Shaders (common/)", [] { return ke::editor().texBro.verzeichnis == "common/"; }));
    // Flush & Reload Shaders
    add(klickAn("Knopf Flush & Reload Shaders", knopfOrt("##texknopfRefreshShaders")));
    add(pause(0.3));
    add(pruefe("RefreshShaders: Verzeichnis leer, Shader der Karte wieder aktiv", [] {
        ke::Editor& e = ke::editor();
        return e.texBro.verzeichnis.empty() && e.aktiveShader.finden("textures/common/caulk") != nullptr &&
               e.aktiveShader.finden("textures/system/caulk") == nullptr;
    }));
    // Find / Replace
    add(klickAn("Knopf Find / Replace", knopfOrt("##texknopfFindReplaceTextures")));
    add(pause(0.2));
    add(pruefe("Find / Replace offen", [] { return ke::editor().findenOffen; }));
    add(tu("Find common/caulk, Replace system/nodraw", [] {
        ke::Editor& e = ke::editor();
        e.findenText = "common/caulk";
        e.ersetzenText = "system/nodraw";
        e.findenNurAuswahl = false;
    }));
    add(foto("karteneditor_texturen_finden"));
    add(klick(ke::trK(Str::KeFrApply)));
    add(pause(0.2));
    add(pruefe("Apply: ersetzt, Rueckgaengig textureFindReplace", [pinsel] {
        ke::Editor& e = ke::editor();
        return pinsel()->flaechen()[0]->shader() == "textures/system/nodraw" &&
               e.karte.undo.undoName() == "textureFindReplace -find textures/common/caulk -replace textures/system/nodraw";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tu("Find aktiv, system/ zeigen", [] {
        ke::Editor& e = ke::editor();
        e.findenAktiv = true;
        ke::texturVerzeichnisZeigen("system/");
    }));
    add(pause(0.3));
    add(klickAn("M1 auf system/caulk bei offenem Dialog", texturOrt("textures/system/caulk")));
    add(pruefe("bei offenem Find-Dialog: Find-Feld = system/caulk, nichts gesetzt", [pinsel] {
        ke::Editor& e = ke::editor();
        return e.findenText == "system/caulk" && pinsel()->flaechen()[0]->shader() == "textures/common/caulk";
    }));
    add(klick(ke::trK(Str::KeFrClose)));
    add(pause(0.2));
    add(pruefe("Close: Dialog zu", [] { return !ke::editor().findenOffen; }));
    // Einstellungsseite "Texture Browser" (Foto pref_Texture_Browser.png)
    add(tu("Einstellungen oeffnen", [] { (void)ke::ausfuehren("Preferences"); }));
    add(pause(0.3));
    add(klick(ke::trK(Str::KePgTexBro), "###karteeinstellungen"));
    add(pause(0.2));
    add(pruefe("Seite Texture Browser: Felder, Vorgaben wie NRC (50 %, 160, 48, 64, None, an)", [] {
        const ke::Editor& e = ke::editor();
        const karte::TexturBrowserEinstellungen& t = e.texBro.e;
        const bool felder = finde(ke::trK(Str::KePrTbScrollbar), "", 0, false) != nullptr && finde("##texmax", "", 0, true) != nullptr &&
                            finde("##texmin", "", 0, true) != nullptr && finde("##texrad", "", 0, true) != nullptr;
        return felder && t.rollbalken && t.skala == 50 && t.einheitsGroesse == 160 && t.einheitsMin == 48 && t.radSchritt == 64 &&
               t.startShader == 0 && t.commonOhneTexturen;
    }));
    add(foto("karteneditor_einstellungen_texturen"));
    // Die Klappfelder meldet ImGui nicht: Lage ueber die Zeilen darum.
    add(klickAn("Klappfeld Texture Thumbnail Scale", [] {
        const Element* a = finde(ke::trK(Str::KePrTbScrollbar), "", 0, false);
        const Element* b = finde("##texmax", "", 0, true);
        if (a == nullptr || b == nullptr) { return ImVec2{0, 0}; }
        return ImVec2{b->rect.GetCenter().x, (a->rect.GetCenter().y + b->rect.GetCenter().y) * 0.5F};
    }));
    add(pause(0.2));
    add(klick("100%"));
    add(pause(0.2));
    add(klickAn("Klappfeld Load Shaders at Startup", [] {
        const Element* a = finde("##texmin", "", 0, true);
        const Element* b = finde("##texrad", "", 0, true);
        if (a == nullptr || b == nullptr) { return ImVec2{0, 0}; }
        return ImVec2{b->rect.GetCenter().x, b->rect.GetCenter().y + (b->rect.GetCenter().y - a->rect.GetCenter().y)};
    }));
    add(pause(0.2));
    add(klick("System"));
    add(pause(0.2));
    add(klick(ke::trK(Str::KePrefOk)));
    add(pause(0.3));
    add(pruefe("Skala 100 %: caulk (64 x 64) 64 breit; Load Shaders at Startup = System", [] {
        ke::Editor& e = ke::editor();
        const karte::AktiverShader* c = e.aktiveShader.finden("textures/common/caulk");
        return e.texBro.e.skala == 100 && c != nullptr && e.texBro.texturMass(*c).first == 64 && e.texBro.e.startShader == 1;
    }));
    add(tu("GL-Flaeche neu (initializeGL), Verzeichnis leer", [] {
        ke::editor().texBro.verzeichnis.clear();
        ke::texturGlNeu();
    }));
    add(pause(0.3));
    add(pruefe("initializeGL mit Load Shaders System: system/ gezeigt", [] { return ke::editor().texBro.verzeichnis == "system/"; }));
    add(tu("Skala 50 %, Load Shaders None", [] {
        ke::Editor& e = ke::editor();
        e.texBro.e.skala = 50;
        e.texBro.e.startShader = 0;
        e.texBro.hoeheGeaendert();
    }));
    add(tu("Einstellungen oeffnen", [] { (void)ke::ausfuehren("Preferences"); }));
    add(pause(0.3));
    add(klick(ke::trK(Str::KePrTbScrollbar)));
    add(klick(ke::trK(Str::KePrefOk)));
    add(pause(0.3));
    add(pruefe("Texture scrollbar aus: kein Rollbalken, die Ansicht reicht bis zum Rand", [feldVon] {
        return !ke::editor().texBro.e.rollbalken && feldVon("Texturen:Rollbalken") == nullptr;
    }));
    add(tu("Rollbalken wieder an", [] { ke::editor().texBro.e.rollbalken = true; }));
    // Teiler ziehen und merken
    add({"Teiler des Texturbrowsers 20 logische Punkte nach rechts", [](int b) {
             static Schritt k;
             if (b == 0) {
                 const Element* el = finde("##grifftex1", "", 0, true);
                 const ImVec2 m = el != nullptr ? el->rect.GetCenter() : ImVec2{0, 0};
                 k = ziehenAn("Griff", [m] { return m; }, 20.0F * ke::massstab(), 0.0F, 0);
             }
             return k.tun(b);
         }});
    add(pause(0.3));
    add(pruefe("Teiler gezogen: links breiter, gemerkt (TextureBrowser/splitter)", [feldVon] {
        const auto* l = feldVon("Texturen:Links");
        const auto& g = ke::editor().texGroessen;
        diag::detail("Knopftest: Texturen - links nach dem Ziehen " + std::to_string(l != nullptr ? l->b : -1));
        return l != nullptr && l->b > 100 && !g.empty() && g[0] == l->b;
    }));
    // TexManipulator (M3): in der Kamera kopieren, mit Alt auftragen; in 2D Alt+M3
    add(tu("Kamera auf den Pinsel, Browser-Shader system/nodraw, nichts gewaehlt", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.kamera.setzeUrsprung(karte::Vektor3f(-256, 0, 32));
        e.kamera.setzeWinkel(karte::Vektor3f(0, 0, 0));
        e.aktivXy = 0;
        e.xy[0].ansicht.typ = karte::Sicht::XY;
        e.xy[0].ansicht.setzeMassstab(1);
        e.xy[0].ansicht.setzeUrsprung(karte::Vektor3f(0, 0, 0));
        ke::texturShaderWaehlen("textures/system/nodraw");
    }));
    add(pause(0.3));
    auto kameraMitte = [] {
        const ke::KameraFenster& f = ke::editor().kamFenster;
        return ImVec2{f.bildX + static_cast<float>(f.breite) * 0.5F, f.bildY + static_cast<float>(f.hoehe) * 0.5F};
    };
    add(klickAn("M3 in die Kamera (Pinsel)", kameraMitte, 2));
    add(pause(0.2));
    add(pruefe("M3 (Scene_copyClosestTexture): der Shader der Flaeche im Browser, Status", [] {
        const ke::Editor& e = ke::editor();
        return e.texBro.shader == "textures/common/caulk" && e.statusTextur.rfind("common/caulk W: ", 0) == 0;
    }));
    add(tu("Browser-Shader system/nodraw", [] { ke::texturShaderWaehlen("textures/system/nodraw"); }));
    add({"Alt+M3 in die Kamera (auftragen)", [kameraMitte](int b) {
             static Schritt k;
             ImGuiIO& io = ImGui::GetIO();
             if (b == 0) {
                 io.AddKeyEvent(ImGuiMod_Alt, true);
                 k = klickAn("Alt+M3", kameraMitte, 2);
             }
             const bool fertig = k.tun(b);
             if (fertig) { io.AddKeyEvent(ImGuiMod_Alt, false); }
             return fertig;
         }});
    add(pause(0.2));
    add(pruefe("Alt+M3: die Flaeche zur Kamera hat system/nodraw, Rueckgaengig \"paintTexture\"", [pinsel] {
        ke::Editor& e = ke::editor();
        int neu = 0;
        for (const auto& f : pinsel()->flaechen()) { neu += f->shader() == "textures/system/nodraw" ? 1 : 0; }
        diag::detail("Knopftest: Texturen - Alt+M3: " + std::to_string(neu) + " Flaechen, " + e.karte.undo.undoName());
        return neu == 1 && e.karte.undo.undoName() == "paintTexture";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add({"Alt+M3 in der XY-Ansicht (32 32)", [](int b) {
             static Schritt k;
             ImGuiIO& io = ImGui::GetIO();
             if (b == 0) {
                 io.AddKeyEvent(ImGuiMod_Alt, true);
                 k = klickAn("Alt+M3 XY", [] {
                     const ke::XyFenster& f = ke::editor().xy[0];
                     const auto p = f.ansicht.zuBild(karte::Vektor3f(32, 32, 0));
                     return ImVec2{f.bildX + p[0], f.bildY + p[1]};
                 }, 2);
             }
             const bool fertig = k.tun(b);
             if (fertig) { io.AddKeyEvent(ImGuiMod_Alt, false); }
             return fertig;
         }});
    add(pause(0.2));
    add(pruefe("Alt+M3 in 2D: die Oberseite hat system/nodraw", [pinsel] {
        for (const auto& f : pinsel()->flaechen()) {
            if (f->ebene().normal()[2] > 0.5) { return f->shader() == "textures/system/nodraw"; }
        }
        return false;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tu("Texturen: zurueck", [] {
        ke::Editor& e = ke::editor();
        e.texGroessen.clear();
        e.texTeiler = karte::Teiler{};
        e.karte.auswahl.leeren();
    }));
    return s;
}

// Punkt 10: die Konsole wie NRC - Meldungen beim Laden des Spiels (vfs,
// Shader, Entitaetsklassen), Karte laden/speichern ("Open file X for
// read...success", ScopeTimer), Zeilen aus Stuecken verschiedener Art,
// Umbruch an der Breite, Auswahl mit der Maus, Kontextmenue (Copy, Select
// All, Copy All, Clear), O in Split (Seite Console des Gruppendialogs).
std::vector<Schritt> karteneditorKonsoleSchritte() {
    namespace ke = karteneditor;
    namespace kb = karte::befehle;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto zeileMit = [](const std::string& anfang) -> const ke::KonsolenZeile* {
        for (const auto& z : ke::editor().konsole) {
            if (z.text.rfind(anfang, 0) == 0) { return &z; }
        }
        return nullptr;
    };
    auto feldVon = [](const std::string& name) -> const ke::Editor::Rechteck* {
        for (const auto& r : ke::editor().felderImBild) {
            if (r.name == name) { return &r; }
        }
        return nullptr;
    };
    add(pruefe("Spiel geladen: Konsole wie NRC (vfs directory, pak file, filesystem initialised, Shader, ScanFile)", [zeileMit] {
        const auto& sp = ke::editor().spiel;
        if (!sp) { return false; }
        // Das Protokoll des Spiels steht in der Konsole (sie wurde dazwischen geleert): hier das Protokoll selbst
        bool vfs = false;
        bool init = false;
        bool scan = false;
        bool shader = false;
        bool suche = false;
        for (const auto& teil : sp->protokoll) {
            vfs = vfs || teil.text.rfind("vfs directory: ", 0) == 0;
            init = init || teil.text == "filesystem initialised\n";
            scan = scan || teil.text.rfind("ScanFile: ", 0) == 0;
            shader = shader || teil.text.rfind("Parsing shaderfile shaders/", 0) == 0 || teil.text.rfind("No shaderlist.txt found", 0) == 0;
            suche = suche || teil.text.rfind("EntityClass: searching \"", 0) == 0;
        }
        diag::detail("Knopftest: Konsole - Protokoll des Spiels " + std::to_string(sp->protokoll.size()) + " Teile");
        (void)zeileMit;
        return (vfs || sp->ordner.empty()) && init && scan && shader && suche;
    }));
    add(tu("Konsole leeren, Karte oeffnen", [] {
        ke::konsoleLeeren();
        (void)ke::karteOeffnen(karteTestPfad("karte_kopie.map"));
    }));
    add(pruefe("Karte laden: Loading map from, Open file ... for read...success (eine Zeile), map load timer, --- LoadMapFile ---", [] {
        const auto& k = ke::editor().konsole;
        std::string pfad = karteTestPfad("karte_kopie.map");
        std::replace(pfad.begin(), pfad.end(), '\\', '/');
        for (const auto& z : k) { diag::detail("Knopftest: Konsole - " + z.text); }
        if (k.size() < 8) { return false; }
        // readGraph meldet das erkannte Format als Warnung (mapq3/plugin.cpp:263)
        const bool format = k[2].text == "detectedFormat = eBrushTypeQuake3" && k[2].art == ke::KonsolenZeile::Art::Warnung;
        const bool zeitOk = k[3].text.rfind("map load timer: ", 0) == 0 && k[3].text.find(" second(s) elapsed") != std::string::npos &&
                            k[3].text.size() == std::string("map load timer:  0.00 second(s) elapsed").size();
        return k[0].text == "Loading map from " + pfad && k[1].text == "Open file " + pfad + " for read...success" &&
               k[1].teile.size() == 1 && format && zeitOk && k[4].text == "--- LoadMapFile ---" && k[5].text == pfad &&
               k[6].text == "1 primitives" && k[7].text == "2 entities";
    }));
    add(tu("Konsole leeren, Welt-Pinsel waehlen, aendern und speichern", [] {
        ke::Editor& e = ke::editor();
        ke::konsoleLeeren();
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), e.karte.welt()->kinder().front());
        karte::schluesselSetzenRueckgaengig(e.karte, "message", "konsole");
        (void)ke::karteSpeichern();
    }));
    add(pruefe("Speichern: der Befehl, Open file ... for write...success, map save timer", [] {
        const auto& k = ke::editor().konsole;
        std::string pfad = karteTestPfad("karte_kopie.map");
        std::replace(pfad.begin(), pfad.end(), '\\', '/');
        for (const auto& z : k) { diag::detail("Knopftest: Konsole - " + z.text); }
        return k.size() >= 3 && k[0].text == "entitySetKeyValue -key \"message\" -value \"konsole\"" &&
               k[1].text == "Open file " + pfad + " for write...success" && k[2].text.rfind("map save timer: ", 0) == 0;
    }));
    add(tu("Zurueck: alte Meldung, gespeichert", [] {
        ke::Editor& e = ke::editor();
        karte::schluesselSetzenRueckgaengig(e.karte, "message", "karteneditor");
        (void)ke::karteSpeichern();
        e.karte.auswahl.leeren();
    }));
    add(tu("Stuecke: Ausgabe ohne Zeilenende, dann Fehler mit Zeilenende", [] {
        ke::konsoleLeeren();
        ke::konsoleSchreiben(ke::KonsolenZeile::Art::Ausgabe, "Open file x for read...");
        ke::konsoleSchreiben(ke::KonsolenZeile::Art::Fehler, "failure\n");
        ke::konsoleSchreiben(ke::KonsolenZeile::Art::Warnung, "zwei\nZeilen\n");
    }));
    add(pruefe("Sys_Print: eine Zeile aus zwei Stuecken (Ausgabe, Fehler), '\\n' trennt Zeilen", [] {
        const auto& k = ke::editor().konsole;
        return k.size() == 3 && k[0].text == "Open file x for read...failure" && k[0].teile.size() == 2 &&
               k[0].teile[1].art == ke::KonsolenZeile::Art::Fehler && k[0].teile[1].von == 23 && k[1].text == "zwei" &&
               k[2].text == "Zeilen" && !k[2].offen;
    }));
    // In Split ist die Konsole eine Seite des Gruppendialogs: O
    add(tu("Split, Gruppendialog zu", [] {
        ke::Editor& e = ke::editor();
        e.anordnung = kb::Anordnung::Split;
        e.gruppenDialogEingebaut = false;
        e.teilerFuer = -1;
    }));
    add(pause(0.3));
    add(tu("Gruppendialog zu", [] { ke::editor().gruppenDialogOffen = false; }));
    add(tasteKombi(ImGuiKey_O, "O"));
    add(pause(0.3));
    add(pruefe("O (ToggleConsole) in Split: Gruppendialog auf der Seite Console", [] {
        return ke::editor().gruppenDialogOffen && ke::editor().gruppenSeite == 1;
    }));
    add(tu("Eine lange Zeile (Umbruch an der Breite)", [] {
        std::string lang;
        for (int i = 0; i < 40; ++i) { lang += "wort" + std::to_string(i) + " "; }
        ke::ausgabe(lang);
    }));
    add(pause(0.2));
    add(foto("karteneditor_konsole"));
    add({"Doppelklick auf das erste Wort (\"Open\")", [](int b) {
             static Schritt k;
             if (b == 0) {
                 k = klickAn("Doppelklick", [] {
                     const Element* el = finde("##konsoleText", "", 0, true);
                     const ImGuiWindow* w = ImGui::FindWindowByName("##konsoleText");
                     (void)el;
                     if (w == nullptr) {
                         for (ImGuiWindow* x : ImGui::GetCurrentContext()->Windows) {
                             if (std::string(x->Name).find("##konsoleText") != std::string::npos && x->Active) { w = x; }
                         }
                     }
                     const float s = ke::massstab();
                     return w != nullptr ? ImVec2{w->Pos.x + 7.0F * s, w->Pos.y + 5.0F * s + 6.0F * s - w->Scroll.y}
                                         : ImVec2{0, 0};
                 }, 0, true);
             }
             return k.tun(b);
         }});
    add(pause(0.2));
    add(pruefe("Doppelklick: das Wort ist gewaehlt", [] {
        diag::detail("Knopftest: Konsole - Auswahl \"" + ke::konsoleAuswahl() + "\"");
        return ke::konsoleAuswahl() == "Open";
    }));
    add({"Rechtsklick in die Konsole", [](int b) {
             static Schritt k;
             if (b == 0) {
                 k = klickAn("Rechtsklick", [] {
                     for (ImGuiWindow* x : ImGui::GetCurrentContext()->Windows) {
                         if (std::string(x->Name).find("##konsoleText") != std::string::npos && x->Active) {
                             return ImVec2{x->Pos.x + x->Size.x * 0.5F, x->Pos.y + x->Size.y * 0.5F};
                         }
                     }
                     return ImVec2{0, 0};
                 }, 1);
             }
             return k.tun(b);
         }});
    add(pause(0.2));
    add(pruefe("Kontextmenue: Copy, Select All, Copy All, Clear", [] {
        return finde(std::string(ke::trK(Str::KeKoCopy)) + "##konsolecopy", "", 0, false) != nullptr &&
               finde(std::string(ke::trK(Str::KeKoSelectAll)) + "##konsoleselectall", "", 0, false) != nullptr &&
               finde(std::string(ke::trK(Str::KeKoCopyAll)) + "##konsolecopyall", "", 0, false) != nullptr &&
               finde(std::string(ke::trK(Str::KeKoClear)) + "##konsoleclear", "", 0, false) != nullptr;
    }));
    add(foto("karteneditor_konsole_menue"));
    add(klick(std::string(ke::trK(Str::KeKoCopy)) + "##konsolecopy"));
    add(pause(0.2));
    add(pruefe("Copy: \"Open\" in der Zwischenablage", [] {
        const char* c = ImGui::GetClipboardText();
        return c != nullptr && std::string(c) == "Open";
    }));
    add({"Rechtsklick in die Konsole", [](int b) {
             static Schritt k;
             if (b == 0) {
                 k = klickAn("Rechtsklick", [] {
                     for (ImGuiWindow* x : ImGui::GetCurrentContext()->Windows) {
                         if (std::string(x->Name).find("##konsoleText") != std::string::npos && x->Active) {
                             return ImVec2{x->Pos.x + x->Size.x * 0.5F, x->Pos.y + x->Size.y * 0.5F};
                         }
                     }
                     return ImVec2{0, 0};
                 }, 1);
             }
             return k.tun(b);
         }});
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KeKoSelectAll)) + "##konsoleselectall"));
    add(pause(0.2));
    add(pruefe("Select All: alles gewaehlt (vier Zeilen)", [] {
        const std::string a = ke::konsoleAuswahl();
        return a.rfind("Open file x for read...failure\nzwei\nZeilen\nwort0 ", 0) == 0;
    }));
    add({"Rechtsklick in die Konsole", [](int b) {
             static Schritt k;
             if (b == 0) {
                 k = klickAn("Rechtsklick", [] {
                     for (ImGuiWindow* x : ImGui::GetCurrentContext()->Windows) {
                         if (std::string(x->Name).find("##konsoleText") != std::string::npos && x->Active) {
                             return ImVec2{x->Pos.x + x->Size.x * 0.5F, x->Pos.y + x->Size.y * 0.5F};
                         }
                     }
                     return ImVec2{0, 0};
                 }, 1);
             }
             return k.tun(b);
         }});
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KeKoCopyAll)) + "##konsolecopyall"));
    add(pause(0.2));
    add(pruefe("Copy All: der ganze Text (toPlainText)", [] {
        const char* c = ImGui::GetClipboardText();
        return c != nullptr && std::string(c).rfind("Open file x for read...failure\nzwei\nZeilen\nwort0 ", 0) == 0;
    }));
    add({"Rechtsklick in die Konsole", [](int b) {
             static Schritt k;
             if (b == 0) {
                 k = klickAn("Rechtsklick", [] {
                     for (ImGuiWindow* x : ImGui::GetCurrentContext()->Windows) {
                         if (std::string(x->Name).find("##konsoleText") != std::string::npos && x->Active) {
                             return ImVec2{x->Pos.x + x->Size.x * 0.5F, x->Pos.y + x->Size.y * 0.5F};
                         }
                     }
                     return ImVec2{0, 0};
                 }, 1);
             }
             return k.tun(b);
         }});
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KeKoClear)) + "##konsoleclear"));
    add(pause(0.2));
    add(pruefe("Clear: die Konsole ist leer", [] { return ke::editor().konsole.empty(); }));
    add(tu("Zurueck: Regular, Gruppendialog zu", [] {
        ke::Editor& e = ke::editor();
        e.anordnung = kb::Anordnung::Regular;
        e.teilerFuer = -1;
        e.gruppenDialogOffen = false;
    }));
    add(pause(0.3));
    (void)feldVon;
    return s;
}

// Fuer den Selbsttest der Entitaeten: die nr-te Entitaet einer Klasse und
// ihre Schluessel - ohne Absturz, wenn es sie (im roten Lauf) nicht gibt.
std::shared_ptr<karte::Entitaet> entitaetNr(const std::string& klasse, std::size_t nr) {
    for (const auto& ent : karteneditor::editor().karte.entitaeten()) {
        if (ent->klassenname() == klasse && nr-- == 0) { return ent; }
    }
    return nullptr;
}
std::string entWert(const std::string& klasse, std::size_t nr, const std::string& schluessel) {
    const auto e = entitaetNr(klasse, nr);
    return e ? e->wert(schluessel) : std::string("<fehlt>");
}
bool entOhne(const std::string& klasse, std::size_t nr, const std::string& schluessel) {
    const auto e = entitaetNr(klasse, nr);
    return e && !e->hat(schluessel);
}
void waehleEnt(const std::string& klasse, std::size_t nr) {
    if (const auto e = entitaetNr(klasse, nr)) { karteneditor::editor().karte.auswahl.waehlen(e); }
}

// Punkt 9: Entitaeten - die Seite "Entities" des Gruppendialogs (Reiter unten,
// Teiler { 55, 175, 255, 255 }), Schluessel/Werte, Spawnflags, Attribute,
// Klassenliste, das Entitaetenmenue der 2D-Ansicht (M2 ohne Bewegung, Strg
// erzeugt ohne zu schliessen) und der Kamera (Doppel-M2), Lichtstaerke,
// Verbinden (Strg+K), Farbe (K, Choose Color), Normalize, Entity List
// (Umschalt+L), Auswahl nach Schluessel, Shift+E, Strg+Umschalt+E, die
// Einstellungsseite Entities und Reload Entity Definitions.
std::vector<Schritt> karteneditorEntitaetenSchritte() {
    namespace ke = karteneditor;
    namespace kb = karte::befehle;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto feldVon = [](const std::string& name) -> const ke::Editor::Rechteck* {
        for (const auto& r : ke::editor().felderImBild) {
            if (r.name == name) { return &r; }
        }
        return nullptr;
    };
    auto knopfOrt = [](const std::string& id) {
        return [id]() {
            const Element* el = finde(id, "", 0, true);
            return el != nullptr ? el->rect.GetCenter() : ImVec2{0, 0};
        };
    };
    auto xyOrt = [](float x, float y) {
        return [x, y]() {
            const ke::XyFenster& f = ke::editor().xy[0];
            const auto p = f.ansicht.zuBild(karte::Vektor3f(x, y, 0));
            return ImVec2{f.bildX + p[0], f.bildY + p[1]};
        };
    };
    auto zahl = [](const std::string& klasse) {
        std::size_t n = 0;
        for (const auto& ent : ke::editor().karte.entitaeten()) { n += ent->klassenname() == klasse ? 1U : 0U; }
        return n;
    };
    auto zeigerText = [](const void* p) {
        char k[32];
        std::snprintf(k, sizeof(k), "%p", p);
        return std::string(k);
    };
    auto tippe = [](const std::string& was, const std::string& text) {
        return Schritt{"Tippe \"" + text + "\" (" + was + ")", [text](int b) {
                           if (b == 0) { ImGui::GetIO().AddInputCharactersUTF8(text.c_str()); }
                           return b >= 3;
                       }};
    };
    auto konsoleHat = [](const std::string& text) {
        for (const auto& z : ke::editor().konsole) {
            if (z.text == text) { return true; }
        }
        return false;
    };
    auto rechtsKamera = [] {
        return Schritt{"M2 kurz in die Kamera", [](int b) {
                           ImGuiIO& io = ImGui::GetIO();
                           const ke::KameraFenster& f = ke::editor().kamFenster;
                           if (b == 0) { setzeMaus(ImVec2{f.bildX + static_cast<float>(f.breite) * 0.5F, f.bildY + static_cast<float>(f.hoehe) * 0.5F}); }
                           if (b == 1) { io.AddMouseButtonEvent(ImGuiMouseButton_Right, true); }
                           if (b == 2) { io.AddMouseButtonEvent(ImGuiMouseButton_Right, false); }
                           return b >= 3;
                       }};
    };
    // Strg gehalten waehrend eines Klicks auf einen Menueeintrag
    auto strgKlick = [](const std::string& label) {
        return Schritt{"Strg+Klick \"" + label + "\"", [label](int b) {
                           ImGuiIO& io = ImGui::GetIO();
                           static ImVec2 m;
                           if (b == 0) {
                               const Element* el = finde(label, "", 0, false);
                               m = el != nullptr ? el->rect.GetCenter() : ImVec2{0, 0};
                               setzeMaus(m);
                               io.AddKeyEvent(ImGuiMod_Ctrl, true);
                               io.AddKeyEvent(ImGuiKey_LeftCtrl, true);
                           }
                           if (b == 2) { io.AddMouseButtonEvent(0, true); }
                           if (b == 3) { io.AddMouseButtonEvent(0, false); }
                           if (b == 5) {
                               io.AddKeyEvent(ImGuiKey_LeftCtrl, false);
                               io.AddKeyEvent(ImGuiMod_Ctrl, false);
                           }
                           return b >= 7;
                       }};
    };
    static float wzZ = 0.0F;
    static std::size_t anzahlVorher = 0;
    add(tu("Entitaeten: Regular, Testkarte, nichts gewaehlt, Gruppendialog zu", [] {
        ke::Editor& e = ke::editor();
        e.anordnung = kb::Anordnung::Regular;
        e.gruppenDialogEingebaut = false;
        e.teilerFuer = -1;
        (void)ke::karteOeffnen(karteTestPfad("karte_kopie.map"));
        e.karte.auswahl.leeren();
        e.ent.groessen.clear();
        e.ent.teiler = karte::Teiler{};
        e.ent.autoFokus = false;
        e.ent.listeOffen = false;
        e.ent.lichtNichtZeigen = false;
        e.ent.lichtLetzte = 300;
        e.ent.zielnamen = false;
        e.ent.namenAbstand = 512;
        e.konsole.clear();
        ke::tastaturErsatz = deutscheTastatur;
    }));
    add(pause(0.4));
    add(pruefe("Regular ohne eingebauten Gruppendialog: zu (MainFrame::Create)", [] { return !ke::editor().gruppenDialogOffen; }));
    add(tasteKombi(ImGuiKey_N, "N"));
    add(pause(0.4));
    add(pruefe("N: Gruppendialog auf Entities (Reiter gezeichnet)", [feldVon] {
        return ke::editor().gruppenDialogOffen && ke::editor().gruppenSeite == 0 && feldVon("Gruppe:Reiter:0") != nullptr;
    }));
    add(pruefe("Lagen wie NRC: Reiter unten 25 hoch ab 0 (Text + 24), Inhalt 2 innen 440 x 750, Teiler 55 / 175 / 255 / 255 mit Griffen 4",
               [feldVon] {
                   const auto* r0 = feldVon("Gruppe:Reiter:0");
                   const auto* r3 = feldVon("Gruppe:Reiter:3");
                   const auto* r4 = feldVon("Gruppe:Reiter:4");
                   const auto* in = feldVon("Gruppe:Inhalt");
                   const auto* k = feldVon("Entities:Klassen");
                   const auto* c = feldVon("Entities:Kommentar");
                   const auto* b = feldVon("Entities:Behaelter");
                   const auto* a = feldVon("Entities:Attribute");
                   if (r0 == nullptr || r3 == nullptr || r4 == nullptr || in == nullptr || k == nullptr || c == nullptr || b == nullptr ||
                       a == nullptr) {
                       return false;
                   }
                   diag::detail("Knopftest: Entitaeten - Reiter " + std::to_string(r0->x) + "/" + std::to_string(r0->y) + " " +
                                std::to_string(r0->b) + "x" + std::to_string(r0->h) + ", Models bei " + std::to_string(r3->x) + " " +
                                std::to_string(r3->b) + ", Layers bei " + std::to_string(r4->x) + " " + std::to_string(r4->b) +
                                "; Inhalt " + std::to_string(in->x) + "/" + std::to_string(in->y) + " " + std::to_string(in->b) + "x" +
                                std::to_string(in->h) + "; Teiler " + std::to_string(k->h) + "/" + std::to_string(c->h) + "/" +
                                std::to_string(b->h) + "/" + std::to_string(a->h) + " bei " + std::to_string(k->y) + "/" +
                                std::to_string(c->y) + "/" + std::to_string(b->y) + "/" + std::to_string(a->y));
                   const bool reiter = r0->x == 0 && r0->h == 25 && r0->y == 777 - 25 && r3->x == r0->b && r4->x == r3->x + r3->b;
                   const bool inhalt = in->x == 2 && in->y == 2 && in->b == 440 && in->h == 750;
                   const bool summe = k->h + c->h + b->h + a->h + 3 * 4 == 750;
                   const bool nahe = std::abs(k->h - 55) <= 1 && std::abs(c->h - 175) <= 1 && std::abs(b->h - 255) <= 1 &&
                                     std::abs(a->h - 255) <= 2 && c->y == k->y + k->h + 4 && b->y == c->y + c->h + 4;
                   return reiter && inhalt && summe && nahe;
               }));
    // Punkt 13 (Handbuchbild): die Umrisse der Reiter kamen mit der Dicke 192
    // (ImDrawFlags_RoundCornersBottom als thickness) und ueberdeckten die Reiter.
    add(pruefe("Reiter: Umrisse 1 Punkt dick, alles im Gruppendialog", [] {
        std::string bericht;
        const bool ok = zeichnungImFenster("/##gruppeinnen_", &bericht);
        diag::detail("Knopftest: Gruppendialog - " + bericht);
        return ok;
    }));
    add(pruefe("Behaelter ohne Auswahl: Liste, Key/Value 22 hoch (Abstand 6), Knoepfe 22; Clear All und Delete Key gleich breit",
               [feldVon] {
                   const auto* l = feldVon("Entities:Schluessel");
                   const auto* key = feldVon("Entities:Key");
                   const auto* val = feldVon("Entities:Value");
                   const auto* ca = feldVon("Entities:ClearAll");
                   const auto* dk = feldVon("Entities:DeleteKey");
                   const auto* st = feldVon("Entities:SelectTargeting");
                   const auto* sc = feldVon("Entities:SelectConnected");
                   const auto* af = feldVon("Entities:AutoFocus");
                   const auto* b = feldVon("Entities:Behaelter");
                   if (l == nullptr || key == nullptr || val == nullptr || ca == nullptr || dk == nullptr || st == nullptr || sc == nullptr ||
                       af == nullptr || b == nullptr) {
                       return false;
                   }
                   diag::detail("Knopftest: Entitaeten - Liste " + std::to_string(l->h) + ", Key " + std::to_string(key->x) + " " +
                                std::to_string(key->b) + "x" + std::to_string(key->h) + ", Clear All " + std::to_string(ca->x) + " " +
                                std::to_string(ca->b) + ", Delete Key " + std::to_string(dk->b) + ", Pfeile " + std::to_string(st->b) + "/" +
                                std::to_string(sc->b) + ", Augen " + std::to_string(af->b));
                   return key->h == 22 && val->y == key->y + 28 && ca->h == 22 && ca->y == val->y + 28 && std::abs(ca->b - dk->b) <= 1 &&
                          st->b == 21 && sc->b == 37 && ca->x == b->x + 4 && af->x + af->b == b->x + b->b - 4 && l->y == b->y &&
                          l->h + 6 + 50 + 6 + 22 == b->h;
               }));
    add(foto("karteneditor_entities"));
    // Ein Licht waehlen: Klasse, Schluessel, Spawnflags, Attribute
    add(tu("Das Licht waehlen", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        waehleEnt("light", 0);
    }));
    add(pause(0.3));
    add(pruefe("Licht gewaehlt: Klassenliste auf light, Zeilen classname / light / origin, Kommentar der Klasse", [] {
        ke::Editor& e = ke::editor();
        const auto& w = e.ent.werte.werte;
        diag::detail("Knopftest: Entitaeten - Klasse " + e.ent.klasse + ", " + std::to_string(w.size()) + " Schluessel");
        return e.ent.klasse == "light" && e.ent.kommentarKlasse == "light" && w.size() == 3 && w.count("light") != 0U &&
               finde("##ezlight", "", 0, true) != nullptr && finde("##eklight", "", 0, true) != nullptr;
    }));
    add(foto("karteneditor_entities_licht"));
    add(klick("##ezlight"));
    add(pause(0.2));
    add(pruefe("Zeile light gedrueckt: Key = light, Value = 300 (EntityProperties_selection_changed)", [] {
        const ke::Editor& e = ke::editor();
        return e.ent.schluessel == "light" && e.ent.wert == "300" && e.ent.zeile == "light";
    }));
    add(klick("##eivalue"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_A, "Strg+A"));
    add(tippe("Value", "500"));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.2));
    add(pruefe("Value 500 + Enter: light = 500, Rueckgaengig entitySetKeyValue -key \"light\" -value \"500\"", [] {
        ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Entitaeten - light " + entWert("light", 0, "light") + ", Rueckgaengig " + e.karte.undo.undoName());
        return entWert("light", 0, "light") == "500" && e.karte.undo.undoName() == "entitySetKeyValue -key \"light\" -value \"500\"";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(pruefe("Strg+Z: light wieder 300", [] { return entWert("light", 0, "light") == "300"; }));
    add(klick("##ezlight"));
    add(klick(ke::trK(Str::KeEiDeleteKey)));
    add(pause(0.2));
    add(pruefe("Delete Key: light weg, entityDeleteKey -key light", [] {
        return entOhne("light", 0, "light") && ke::editor().karte.undo.undoName() == "entityDeleteKey -key light";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(pruefe("Spawnflags des Lichts: linear, noincidence, start_off (LowerCase)", [] {
        return finde("linear##eiflag0", "", 0, false) != nullptr && finde("noincidence##eiflag1", "", 0, false) != nullptr &&
               finde("start_off##eiflag2", "", 0, false) != nullptr;
    }));
    add(klick("linear##eiflag0"));
    add(pause(0.2));
    add(pruefe("Spawnflag linear: spawnflags 1, entitySetSpawnflags -flags 1", [] {
        return entWert("light", 0, "spawnflags") == "1" && ke::editor().karte.undo.undoName() == "entitySetSpawnflags -flags 1";
    }));
    add(klick("linear##eiflag0"));
    add(pause(0.2));
    add(pruefe("Spawnflag aus: der Schluessel ist weg (\"\" bei 0)", [] {
        return entOhne("light", 0, "spawnflags") && ke::editor().karte.undo.undoName() == "entitySetSpawnflags -flags 0";
    }));
    // Attribute (eclass_def.cpp: angle "Yaw Angle", model, noise)
    add({"Attribut angle: Feld anklicken", [](int b) {
             static Schritt k;
             if (b == 0) {
                 const karte::EntitaetsKlasse* kl = ke::editor().karte.klasseFuer("light", false);
                 int index = -1;
                 for (std::size_t i = 0; i < kl->attribute.size(); ++i) {
                     if (kl->attribute[i].first == "angle") { index = static_cast<int>(i); }
                 }
                 diag::detail("Knopftest: Entitaeten - angle ist Attribut " + std::to_string(index) + " (" +
                              (index >= 0 ? kl->attribute[static_cast<std::size_t>(index)].second.typ : std::string("-")) + ")");
                 k = klick("##eiattr" + std::to_string(index) + "w");
             }
             return k.tun(b);
         }});
    add(tippe("angle", "450"));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.2));
    add(pruefe("angle 450 + Enter: angle_normalised -> 90", [] {
        diag::detail("Knopftest: Entitaeten - angle " + entWert("light", 0, "angle"));
        return entWert("light", 0, "angle") == "90";
    }));
    add(tu("Kamera: Blick nach yaw 30", [] { ke::editor().kamera.setzeWinkel(karte::Vektor3f(0, 30, 0)); }));
    add({"<-cam beim Winkel", [](int b) {
             static Schritt k;
             if (b == 0) {
                 const Element* el = finde(std::string(ke::trK(Str::KeEiCam)) + "##eiattr", "", 0, true);
                 k = klick(el != nullptr ? el->label : std::string("<-cam"));
             }
             return k.tun(b);
         }});
    add(pause(0.2));
    add(pruefe("<-cam: angle = Gierwinkel der Kamera (30)", [] { return entWert("light", 0, "angle") == "30"; }));
    add(klick(ke::trK(Str::KeEiClearAll)));
    add(pause(0.2));
    add(pruefe("Clear All: nur classname und origin bleiben, entityClear", [] {
        const auto l = entitaetNr("light", 0);
        return l && l->schluessel().size() == 2 && l->hat("origin") && ke::editor().karte.undo.undoName() == "entityClear";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(pruefe("Dreimal Strg+Z: light 300, kein angle", [] {
        const auto l = entitaetNr("light", 0);
        return l && l->wert("light") == "300" && !l->hat("angle");
    }));
    // Auswahl nach Schluessel (Select_EntitiesByKeyValue)
    add(tu("nichts gewaehlt, Key = light", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.ent.schluessel = "light";
        e.ent.wert = "300";
    }));
    add(klickAn("Select by key", knopfOrt("##eiselkey")));
    add(pruefe("Select by key: das Licht gewaehlt", [] {
        const ke::Editor& e = ke::editor();
        return e.karte.auswahl.anzahl() == 1 && e.karte.auswahl.gewaehlt(entitaetNr("light", 0).get());
    }));
    add(tu("nichts gewaehlt", [] { ke::editor().karte.auswahl.leeren(); }));
    add(klickAn("Select by key + value", knopfOrt("##eiselkeyvalue")));
    add(pruefe("Select by key + value: das Licht gewaehlt", [] { return ke::editor().karte.auswahl.gewaehlt(entitaetNr("light", 0).get()); }));
    // Klassenliste: Doppelklick wandelt um (Welt-Pinsel -> func_group)
    add(tu("Den Welt-Pinsel waehlen", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), e.karte.welt()->kinder().front());
    }));
    add(pause(0.2));
    add(pruefe("Welt-Pinsel gewaehlt: Klassenliste auf worldspawn", [] { return ke::editor().ent.klasse == "worldspawn"; }));
    add(tu("func_group in der Klassenliste zeigen (current item, scrollTo)", [] {
        ke::Editor& e = ke::editor();
        e.ent.klasse = "func_group";
        e.ent.kommentarKlasse = "func_group";
        e.ent.klasseRollen = true;
    }));
    add(pause(0.3));
    add(klickAn("Doppelklick auf func_group", knopfOrt("##ekfunc_group"), 0, true));
    add(pause(0.2));
    add(pruefe("Doppelklick: der Pinsel in einer neuen func_group, gewaehlt, entitySetClass -class func_group", [] {
        ke::Editor& e = ke::editor();
        const auto g = entitaetNr("func_group", 0);
        return g && g->kinder().size() == 1 && e.karte.welt()->kinder().empty() && e.karte.auswahl.gewaehlt(g.get(), g->kinder()[0].get()) &&
               e.karte.undo.undoName() == "entitySetClass -class func_group";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(pruefe("Strg+Z: der Pinsel wieder in der Welt", [zahl] { return zahl("func_group") == 0 && ke::editor().karte.welt()->kinder().size() == 1; }));
    // Entity > Move Primitives (Strg+M), EntityMovePrimitivesToFirst, EntityUngroup, EntityUngroupPrimitives
    add(tu("Welt-Pinsel in eine func_group, ein zweiter Pinsel in der Welt", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), e.karte.welt()->kinder().front());
        karte::klasseSetzenAuswahl(e.karte, "func_group");
        e.karte.undo.start();
        karte::TexturProjektion proj;
        karte::texdefStandard(karte::TexdefArt::Quake, proj);
        auto p = std::make_shared<karte::Pinsel>();
        e.karte.welt()->kindEinfuegen(p);
        p->quaderBauen(karte::Kasten(karte::Vektor3f(256, 0, 32), karte::Vektor3f(32, 32, 32)), "textures/common/caulk", proj, e.karte.kontext);
        e.karte.undo.finish("brushDragNew");
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), p);
        const auto g = entitaetNr("func_group", 0);
        if (g && !g->kinder().empty()) { e.karte.auswahl.waehlen(g, g->kinder().front()); }
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_M, "Strg+M"));
    add(pause(0.2));
    add(pruefe("Strg+M (Move Primitives to Entity): der Welt-Pinsel in der func_group des zuletzt Gewaehlten", [] {
        const ke::Editor& e = ke::editor();
        const auto g = entitaetNr("func_group", 0);
        return g && g->kinder().size() == 2 && e.karte.welt()->kinder().empty() &&
               e.karte.undo.undoName() == "movePrimitivesToEntity \"func_group\"";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tu("func_group-Pinsel zuerst, dann der Welt-Pinsel: EntityMovePrimitivesToFirst", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        const auto g = entitaetNr("func_group", 0);
        if (g && !g->kinder().empty()) { e.karte.auswahl.waehlen(g, g->kinder().front()); }
        if (!e.karte.welt()->kinder().empty()) { e.karte.auswahl.waehlen(e.karte.welt(), e.karte.welt()->kinder().front()); }
        (void)ke::ausfuehren("EntityMovePrimitivesToFirst");
    }));
    add(pruefe("EntityMovePrimitivesToFirst: in die func_group des zuerst Gewaehlten", [] {
        const auto g = entitaetNr("func_group", 0);
        return g && g->kinder().size() == 2 && ke::editor().karte.welt()->kinder().empty();
    }));
    add(tu("Einen Pinsel der func_group waehlen: EntityUngroupPrimitives", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        const auto g = entitaetNr("func_group", 0);
        if (g && !g->kinder().empty()) { e.karte.auswahl.waehlen(g, g->kinder().front()); }
        (void)ke::ausfuehren("EntityUngroupPrimitives");
    }));
    add(pruefe("EntityUngroupPrimitives: der Pinsel in der Welt, ungroupSelectedPrimitives", [] {
        const ke::Editor& e = ke::editor();
        const auto g = entitaetNr("func_group", 0);
        return g && g->kinder().size() == 1 && e.karte.welt()->kinder().size() == 1 && e.karte.undo.undoName() == "ungroupSelectedPrimitives";
    }));
    add(tu("Die func_group waehlen: EntityUngroup", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        waehleEnt("func_group", 0);
        (void)ke::ausfuehren("EntityUngroup");
    }));
    add(pruefe("EntityUngroup: Gruppe aufgeloest, beide Pinsel in der Welt, ungroupSelectedEntities", [zahl] {
        const ke::Editor& e = ke::editor();
        return zahl("func_group") == 0 && e.karte.welt()->kinder().size() == 2 && e.karte.undo.undoName() == "ungroupSelectedEntities";
    }));
    add(tu("Zurueck: Karte neu, nichts gewaehlt", [] {
        ke::Editor& e = ke::editor();
        (void)ke::karteOeffnen(karteTestPfad("karte_kopie.map"));
        e.karte.auswahl.leeren();
    }));
    add(pause(0.2));
    // Entitaetenmenue der 2D-Ansicht (der schwebende Gruppendialog ist dafuer zu)
    add(tu("nichts gewaehlt, Workzone merken, Gruppendialog zu", [] {
        ke::Editor& e = ke::editor();
        e.gruppenDialogOffen = false;
        e.karte.auswahl.leeren();
        const karte::Kasten& wz = e.workzone;
        wzZ = karte::zahlEingerastet(karte::zahlMitte(wz.origin[2] - wz.extents[2], wz.origin[2] + wz.extents[2]), e.raster.groesse());
        anzahlVorher = e.karte.entitaeten().size();
    }));
    add(klickAn("M2 in die XY-Ansicht bei 64 64 (ohne Bewegung)", xyOrt(64, 64), 1));
    add(pause(0.3));
    add(pruefe("M2 ohne Bewegung: das Entitaetenmenue (Untermenues info, ...)", [] {
        diag::detail("Knopftest: Entitaeten - Menue " + std::to_string(ke::entitaetenMenuePunkte().size()) + " Punkte oben");
        return finde("info", "", 0, false) != nullptr && !ke::editor().ent.kameraMenue;
    }));
    add(foto("karteneditor_entitaetenmenue"));
    add(klick("info"));
    add(pause(0.3));
    add(klick("info_null"));
    add(pause(0.3));
    add(pruefe("info_null erzeugt bei 64 64 (Mitte der Workzone auf dem Raster), gewaehlt, entityCreate -class info_null", [] {
        ke::Editor& e = ke::editor();
        const auto n = entitaetNr("info_null", 0);
        char soll[64];
        std::snprintf(soll, sizeof(soll), "64 64 %g", static_cast<double>(wzZ));
        diag::detail("Knopftest: Entitaeten - info_null bei " + (n ? n->wert("origin") : std::string("-")) + ", soll " + soll);
        return n && n->wert("origin") == soll && e.karte.auswahl.gewaehlt(n.get()) && e.karte.undo.undoName() == "entityCreate -class info_null";
    }));
    add(klickAn("M2 in die XY-Ansicht bei 128 64", xyOrt(128, 64), 1));
    add(pause(0.3));
    add(klick("info"));
    add(pause(0.3));
    add(strgKlick("info_notnull"));
    add(pause(0.2));
    add(pruefe("Strg+Klick: info_notnull erzeugt, das Menue bleibt offen", [] {
        return entitaetNr("info_notnull", 0) != nullptr && finde("info_notnull", "", 0, false) != nullptr;
    }));
    add(strgKlick("info_notnull"));
    add(pause(0.2));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    add(pruefe("Zweimal: der zweite um max( 8, Raster ) weiter auf der x-Achse (g_entityCreationOffset)", [] {
        const auto a = entitaetNr("info_notnull", 0);
        const auto b = entitaetNr("info_notnull", 1);
        char sa[64];
        char sb[64];
        std::snprintf(sa, sizeof(sa), "128 64 %g", static_cast<double>(wzZ));
        std::snprintf(sb, sizeof(sb), "136 64 %g", static_cast<double>(wzZ));
        diag::detail("Knopftest: Entitaeten - info_notnull " + (a ? a->wert("origin") : std::string("-")) + " / " +
                     (b ? b->wert("origin") : std::string("-")));
        return a && b && a->wert("origin") == sa && b->wert("origin") == sb;
    }));
    add(pruefe("Esc schliesst Untermenue und Menue, ohne Kuerzel (UnSelectSelection2): der zweite bleibt gewaehlt", [] {
        const ke::Editor& e = ke::editor();
        return !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel) && e.karte.auswahl.anzahl() == 1 &&
               e.karte.auswahl.gewaehlt(entitaetNr("info_notnull", 1).get());
    }));
    // Licht mit der Frage nach der Staerke
    add(tu("nichts gewaehlt", [] { ke::editor().karte.auswahl.leeren(); }));
    add(klickAn("M2 in die XY-Ansicht bei -64 -64", xyOrt(-64, -64), 1));
    add(pause(0.3));
    add(pruefe("M2 bei -64 -64: das Entitaetenmenue mit light", [xyOrt] {
        const ImVec2 m = xyOrt(-64, -64)();
        const ke::XyFenster& f = ke::editor().xy[0];
        char z[200];
        std::snprintf(z, sizeof(z), "Knopftest: Entitaeten - M2 bei %.0f/%.0f, XY ab %.0f/%.0f, Popup %d, info %d, light %d",
                      static_cast<double>(m.x), static_cast<double>(m.y), static_cast<double>(f.bildX), static_cast<double>(f.bildY),
                      ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel) ? 1 : 0,
                      finde("info", "", 0, false) != nullptr ? 1 : 0, finde("light", "", 0, false) != nullptr ? 1 : 0);
        diag::detail(z);
        return finde("light", "", 0, false) != nullptr;
    }));
    add(foto("karteneditor_entitaetenmenue_licht"));
    add(klick("light"));
    add(pause(0.3));
    add(pruefe("light: Set Light intensity offen (300 vorgegeben)", [] {
        return ke::editor().ent.lichtOffen && ke::editor().ent.lichtWert == 300 && finde(ke::trK(Str::KeLiDontShow), "", 0, false) != nullptr;
    }));
    add(foto("karteneditor_lichtstaerke"));
    add(ziehenAn("Intensity: (SpinBoxLabel) 100 Bildpunkte nach rechts", knopfOrt("##lichtziehen"), 100.0F, 0.0F, 0));
    add(pruefe("SpinBoxLabel: je 20 logische Punkte ein Schritt", [] {
        const int soll = 300 + static_cast<int>((100.0F / std::max(1.0F, std::round(g_app->dpiScale))) / 20.0F);
        diag::detail("Knopftest: Entitaeten - Lichtstaerke " + std::to_string(ke::editor().ent.lichtWert) + ", soll " + std::to_string(soll));
        return ke::editor().ent.lichtWert == soll;
    }));
    add(klick(std::string(ke::trK(Str::KePrefOk)) + "##lichtok"));
    add(pause(0.2));
    add(pruefe("OK: das neue Licht hat diese Staerke, ein Schritt entityCreate -class light", [zahl] {
        const auto l = entitaetNr("light", 1);
        const int soll = 300 + static_cast<int>((100.0F / std::max(1.0F, std::round(g_app->dpiScale))) / 20.0F);
        return zahl("light") == 2 && l && l->wert("light") == std::to_string(soll) && ke::editor().ent.lichtLetzte == soll &&
               ke::editor().karte.undo.undoName() == "entityCreate -class light";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(pruefe("Strg+Z: das Licht samt Staerke weg", [zahl] { return zahl("light") == 1; }));
    // Gruppe ohne Pinsel: Fehler
    add(klickAn("M2 in die XY-Ansicht bei 0 128", xyOrt(0, 128), 1));
    add(pause(0.3));
    add(klick("func"));
    add(pause(0.3));
    add(klick("func_group"));
    add(pause(0.2));
    add(pruefe("func_group ohne Pinsel: Konsole \"failed to create a group entity - no brushes are selected\"", [konsoleHat, zahl] {
        return konsoleHat("failed to create a group entity - no brushes are selected") && zahl("func_group") == 0;
    }));
    // Kamera: Doppel-M2
    add(tu("Kamera bei 0 0 64, Blick +x", [] {
        ke::Editor& e = ke::editor();
        e.kamera.setzeUrsprung(karte::Vektor3f(0, 0, 64));
        e.kamera.setzeWinkel(karte::Vektor3f(0, 0, 0));
        e.karte.auswahl.leeren();
    }));
    add(rechtsKamera());
    add(rechtsKamera());
    add(pause(0.3));
    add(pruefe("Doppel-M2 in der Kamera: das Entitaetenmenue (g_bCamEntityMenu), Freelook aus", [] {
        return finde("info", "", 0, false) != nullptr && ke::editor().ent.kameraMenue && !ke::editor().kamFenster.frei;
    }));
    add(klick("info"));
    add(pause(0.3));
    add(klick("info_null"));
    add(pause(0.3));
    add(pruefe("Aus der Kamera: info_null 64 vor der Kamera (-viewvector), eingerastet", [] {
        const auto n = entitaetNr("info_null", 1);
        diag::detail("Knopftest: Entitaeten - info_null aus der Kamera bei " + (n ? n->wert("origin") : std::string("-")));
        return n && n->wert("origin") == "64 0 64";
    }));
    // Verbinden (Strg+K)
    add(tu("info_null und dann info_notnull waehlen", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        waehleEnt("info_null", 0);
        waehleEnt("info_notnull", 0);
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_K, "Strg+K"));
    add(pause(0.2));
    add(pruefe("Strg+K: target = targetname = info_notnull1, entityConnectSelected", [] {
        const auto a = entitaetNr("info_null", 0);
        const auto b = entitaetNr("info_notnull", 0);
        return a && b && a->wert("target") == "info_notnull1" && b->wert("targetname") == "info_notnull1" &&
               ke::editor().karte.undo.undoName() == "entityConnectSelected";
    }));
    add(tu("info_null waehlen", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        waehleEnt("info_null", 0);
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_E, "Strg+Umschalt+E"));
    add(pause(0.2));
    add(pruefe("Strg+Umschalt+E (Select Connected Entities): info_notnull dazu", [] {
        const ke::Editor& e = ke::editor();
        return e.karte.auswahl.gewaehlt(entitaetNr("info_null", 0).get()) && e.karte.auswahl.gewaehlt(entitaetNr("info_notnull", 0).get());
    }));
    add(tu("info_notnull waehlen, Gruppendialog auf Entities", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        waehleEnt("info_notnull", 0);
        e.gruppenDialogOffen = true;
        e.gruppenSeite = 0;
    }));
    add(pause(0.3));
    add(klickAn("Select targeting entities", knopfOrt("##eiseltargeting")));
    add(pruefe("<- (Select targeting entities): nur info_null", [] {
        const ke::Editor& e = ke::editor();
        return e.karte.auswahl.anzahl() == 1 && e.karte.auswahl.gewaehlt(entitaetNr("info_null", 0).get());
    }));
    add(klickAn("Select targets", knopfOrt("##eiseltargets")));
    add(pruefe("-> (Select targets): nur info_notnull", [] {
        const ke::Editor& e = ke::editor();
        return e.karte.auswahl.anzahl() == 1 && e.karte.auswahl.gewaehlt(entitaetNr("info_notnull", 0).get());
    }));
    // Farbe (K) und Normalize
    add(tu("Das Licht waehlen", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        waehleEnt("light", 0);
    }));
    add(tasteKombi(ImGuiKey_K, "K"));
    add(pause(0.3));
    add(pruefe("K: Choose Color offen (Grundfarben, Pick Screen Color, Custom colors)", [] {
        return ke::editor().ent.farbDialog.offen && finde("##cdgrund", "", 0, true) != nullptr &&
               finde(std::string(ke::trK(Str::KeCdPick)) + "##cdpick", "", 0, false) != nullptr;
    }));
    add(foto("karteneditor_farbe"));
    add(klickAn("Grundfarbe Rot (Zeile 3, Spalte 1)", [] {
        const Element* el = finde("##cdgrund", "", 0, true);
        const float s = ke::massstab();
        return el != nullptr ? ImVec2{el->rect.Min.x + (28.0F + 14.0F) * s, el->rect.Min.y + (3.0F * 24.0F + 12.0F) * s} : ImVec2{0, 0};
    }));
    add(pause(0.2));
    add(pruefe("Rot gewaehlt: HTML #ff0000", [] { return ke::editor().ent.farbDialog.html == "#ff0000"; }));
    add(klick(std::string(ke::trK(Str::KePrefOk)) + "##cdok"));
    add(pause(0.2));
    add(pruefe("OK: _color 1 0 0, entitySetColour 1 0 0", [] {
        return entWert("light", 0, "_color") == "1 0 0" && ke::editor().karte.undo.undoName() == "entitySetColour 1 0 0";
    }));
    add(tu("_color 0.5 0.25 0 setzen", [] { karte::schluesselSetzenRueckgaengig(ke::editor().karte, "_color", "0.5 0.25 0"); }));
    add(tu("Entity > Normalize Color", [] { (void)ke::ausfuehren("EntityColorNormalize"); }));
    add(pruefe("Normalize Color: 1 0.5 0, entityNormalizeColour 1 0.5 0", [] {
        return entWert("light", 0, "_color") == "1 0.5 0" && ke::editor().karte.undo.undoName() == "entityNormalizeColour 1 0.5 0";
    }));
    // Entity List (Umschalt+L)
    add(tu("nichts gewaehlt", [] { ke::editor().karte.auswahl.leeren(); }));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_L, "Umschalt+L"));
    add(pause(0.4));
    add(tu("Gruppendialog zu", [] { ke::editor().gruppenDialogOffen = false; }));
    add(pruefe("Umschalt+L: Entity List offen, Wurzel aufgeklappt, Zeilen der Entities", [zeigerText, feldVon] {
        const ke::Editor& e = ke::editor();
        return e.ent.listeOffen && finde("##elkarte", "", 0, true) != nullptr &&
               finde("##el" + zeigerText(entitaetNr("light", 0).get()), "", 0, true) != nullptr && feldVon("EntityList:Filter") != nullptr;
    }));
    add(foto("karteneditor_entityliste"));
    add({"Klick auf die Zeile des Lichts", [zeigerText](int b) {
             static Schritt k;
             if (b == 0) { k = klick("##el" + zeigerText(entitaetNr("light", 0).get())); }
             return k.tun(b);
         }});
    add(pause(0.2));
    add(pruefe("Klick in der Liste: das Licht gewaehlt", [] {
        const ke::Editor& e = ke::editor();
        return e.karte.auswahl.anzahl() == 1 && e.karte.auswahl.gewaehlt(entitaetNr("light", 0).get());
    }));
    add({"Maus ueber das Filterfeld, anklicken", [](int b) {
             static Schritt k;
             if (b == 0) { k = klick("##elfilter"); }
             return k.tun(b);
         }});
    add(tippe("Filter", "notnull"));
    add(pause(0.2));
    add(pruefe("Filter \"notnull\": info_notnull sichtbar, das Licht nicht", [zeigerText] {
        return finde("##el" + zeigerText(entitaetNr("info_notnull", 0).get()), "", 0, true) != nullptr &&
               finde("##el" + zeigerText(entitaetNr("light", 0).get()), "", 0, true) == nullptr;
    }));
    add(tu("Entity List zu", [] { ke::editor().ent.listeOffen = false; }));
    // Shift+E
    add(tu("Den Welt-Pinsel waehlen", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), e.karte.welt()->kinder().front());
    }));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_E, "Umschalt+E"));
    add(pruefe("Umschalt+E (Expand Selection To Entities): der Pinsel bleibt, worldspawn selbst nicht", [] {
        const ke::Editor& e = ke::editor();
        return e.karte.auswahl.anzahl() == 1 && !e.karte.auswahl.gewaehlt(e.karte.welt().get());
    }));
    // Entity-Menue: die Punkte sind da und nicht ausgegraut
    add(pruefe("Entity-Menue: Connect, Move Primitives, Select Color, Normalize, Reload nachgebaut", [] {
        for (const char* b : {"EntitiesConnect", "EntityMovePrimitivesToLast", "EntityColorSet", "EntityColorNormalize",
                              "EntityReloadDefinitions", "ToggleEntityList", "ExpandSelectionToEntities", "SelectConnectedEntities"}) {
            if (!ke::nachgebaut(b)) { return false; }
        }
        return true;
    }));
    // Einstellungsseite Entities
    add(tu("Einstellungen auf, Seite Entities", [] { ke::editor().einstellungenOffen = true; }));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KePgEntities)) + "##seite9"));
    add(pause(0.2));
    add(pruefe("Seite Entities: Names Display Distance (3D) 512, Ratio (2D) 64, Entity Names = Targetnames", [] {
        return finde("##namenabstand", "", 0, true) != nullptr && finde("##namenverhaeltnis", "", 0, true) != nullptr &&
               finde(ke::trK(Str::KePrEnTargetnames), "", 0, false) != nullptr;
    }));
    add(foto("karteneditor_einstellungen_entities"));
    add(klick(ke::trK(Str::KePrEnTargetnames)));
    add(klick(ke::trK(Str::KePrefOk)));
    add(pause(0.2));
    add(pruefe("OK: Entity Names = Targetnames an (Namen zeigen targetname)", [] { return ke::editor().ent.zielnamen; }));
    add(tu("Targetnames wieder aus", [] {
        ke::editor().ent.zielnamen = false;
        ke::einstellungenSpeichern();
    }));
    // Reload Entity Definitions: Rueckfrage, Discard, Karte neu
    add(tu("Entity > Reload Entity Definitions", [] { (void)ke::ausfuehren("EntityReloadDefinitions"); }));
    add(pause(0.3));
    add(pruefe("Reload bei geaenderter Karte: Rueckfrage \"Reload Entity Definitions\"", [] {
        return ke::editor().frageOffen && ke::editor().frageTitel == "Reload Entity Definitions";
    }));
    add(klick(tr(Str::KeDiscard)));
    add(pause(0.4));
    add(pruefe("Discard: Klassen neu, Karte aus der Datei (2 Entities, nicht geaendert)", [] {
        const ke::Editor& e = ke::editor();
        return !e.frageOffen && e.ent.klassenStand > 0 && e.karte.entitaeten().size() == 2 && !ke::geaendert() && e.karteSpiel &&
               e.karteSpiel->klassen.finden("light") != nullptr;
    }));
    add(tu("Entitaeten: zurueck", [] {
        ke::Editor& e = ke::editor();
        e.gruppenDialogOffen = false;
        e.karte.auswahl.leeren();
        e.ent.listeOffen = false;
    }));
    (void)anzahlVorher;
    return s;
}

// --- Punkt 11: jeder nachgebaute Befehl, Fotos zum Vergleich mit NRC ----------
//
// Alle Dialoge und Popups des Karten-Editors schliessen (nach einem Befehl,
// der einen Dialog oeffnet). Die Rueckfrage "Karte geaendert" beantwortet
// "Discard" - Reload Entity Definitions laedt dann wirklich neu.
void karteDialogeZu() {
    namespace ke = karteneditor;
    ke::Editor& e = ke::editor();
    e.einstellungenOffen = false;
    e.tastenDialogOffen = false;
    e.findenOffen = false;
    e.shaderInfoOffen = false;
    e.shaderText.offen = false;
    e.ueberOffen = false;
    e.ent.listeOffen = false;
    e.ent.farbDialog.offen = false;
    e.ent.lichtOffen = false;
    e.bauDialogOffen = false;
    e.portalLadenOffen = false;       // Stufe 3 Punkt 5: prtview
    e.portalKonfigOffen = false;
    ke::terrainSchliessen();          // Stufe 8b Punkt 1: Terrain Generator
    ke::zusammenDialogSchliessen();   // Stufe 8a: Add to compiled map (lag sonst ueber der XY-Ansicht der Tests danach)
    e.logik.offen = false;            // Stufe 8b: Logik-Ansicht
    e.undoListeOffen = false;         // Stufe 8b: Undo-Verlauf
    ke::bauAbbrechen();               // Stufe 3 Punkt 4: kein q3map2 in den naechsten Schritt
    e.bauRueckmeldungOffen = false;   // Q3Map debug window
    e.meldung.offen = false;          // qtMeldung (sonst offen, aber ohne Popup)
    e.meldung.danach = nullptr;
    if (e.frageOffen) {
        std::function<void()> danach = std::move(e.nachFrage);
        e.nachFrage = nullptr;
        e.frageOffen = false;
        if (danach) { danach(); }
    }
    if (GImGui->OpenPopupStack.Size > 0) { ImGui::ClosePopupToLevel(0, true); }
}

// Das Rechteck des obersten offenen Popups (Menue, Untermenue).
ImRect obersterPopupRahmen() {
    const ImGuiContext& g = *GImGui;
    for (int i = g.OpenPopupStack.Size - 1; i >= 0; --i) {
        if (const ImGuiWindow* w = g.OpenPopupStack[i].Window) { return w->Rect(); }
    }
    return ImRect{};
}

// Das Rechteck eines Fensters ("###kartetasten" ...), leer wenn es nicht da ist.
ImRect fensterRahmen(const std::string& name) {
    const ImGuiWindow* w = ImGui::FindWindowByName(name.c_str());
    return (w != nullptr && (w->Active || w->WasActive)) ? w->Rect() : ImRect{};
}

// Foto wie foto(), dazu <name>.rect (x0 y0 x1 y1 in Bildpunkten): den
// Ausschnitt schneidet der Vergleich mit dem NRC-Foto aus.
Schritt fotoRahmen(const std::string& name, std::function<ImRect()> rahmen) {
    return {"Foto " + name, [=](int b) {
                const char* ordner = std::getenv("BHED_FOTOS");
                if (b == 2) {
                    const ImRect r = rahmen();
                    if (r.GetWidth() < 2.0F || r.GetHeight() < 2.0F) {
                        meldeFehler("Foto " + name + ": nichts zu sehen");
                    } else if (ordner != nullptr) {
                        std::ofstream f(std::string(ordner) + "\\" + name + ".rect");
                        f << static_cast<int>(r.Min.x) << " " << static_cast<int>(r.Min.y) << " " << static_cast<int>(r.Max.x)
                          << " " << static_cast<int>(r.Max.y) << "\n";
                    }
                    if (ordner != nullptr) { render::fotoAnfordern(std::string(ordner) + "\\" + name + ".bmp"); }
                }
                return b >= 5;
            }};
}

// Jeder nachgebaute Befehl einmal (GlobalCommands_insert / GlobalToggles_insert):
// mit gewaehltem Pinsel und Licht, Umschalter ein zweites Mal (zurueck),
// danach alle Dialoge zu. Nicht hier: Exit (beendet), OpenMap und SaveMapAs
// (Windows-Dateidialog - die Funktionen dahinter pruefen die Schritte oben).
// New Map kommt zuletzt. Raster, Kamera, Anzeige, 2D-Ansichten, Texturbrowser,
// Fenster und Werkzeug kommen danach zurueck, die Karte wird neu geladen.
// --- Stufe 2 des Karten-Editors: die Bauwerkzeuge -----------------------------
//
// Je Punkt der Stufe 2 Wirkungspruefungen in der echten Oberflaeche (Maus und
// Tasten wie ein Mensch); die Geometrie rechnen die ctests (kartekomponenten
// usw.) exakt nach. BHED_KARTE_NUR=stufe2 laesst nur diese Schritte laufen.
namespace stufe2 {

namespace ke = karteneditor;

ImVec2 weltXy(float x, float y) {
    const ke::XyFenster& f = ke::editor().xy[0];
    const auto b = f.ansicht.zuBild(karte::Vektor3f(x, y, 0));
    return ImVec2{f.bildX + b[0], f.bildY + b[1]};
}

// Maus ziehen in der XY-Ansicht: Zusatztasten, Druck, zehn Schritte, Loslassen.
Schritt ziehenXy(const std::string& was, int taste, ImGuiKeyChord mods, ImVec2 von, ImVec2 nach) {
    return Schritt{was, [=](int b) {
                       ImGuiIO& io = ImGui::GetIO();
                       const ImVec2 a = weltXy(von.x, von.y);
                       const ImVec2 z = weltXy(nach.x, nach.y);
                       if (b == 0) {
                           setzeMaus(a);
                           if ((mods & ImGuiMod_Ctrl) != 0) { io.AddKeyEvent(ImGuiMod_Ctrl, true); }
                           if ((mods & ImGuiMod_Shift) != 0) { io.AddKeyEvent(ImGuiMod_Shift, true); }
                           if ((mods & ImGuiMod_Alt) != 0) { io.AddKeyEvent(ImGuiMod_Alt, true); }
                           return false;
                       }
                       if (b == 1) {
                           io.AddMouseButtonEvent(taste, true);
                           return false;
                       }
                       if (b >= 2 && b <= 11) {
                           const float t = static_cast<float>(b - 1) / 10.0F;
                           setzeMaus(ImVec2{a.x + (z.x - a.x) * t, a.y + (z.y - a.y) * t});
                           return false;
                       }
                       if (b == 12) {
                           io.AddMouseButtonEvent(taste, false);
                           return false;
                       }
                       if (b == 13) {
                           if ((mods & ImGuiMod_Ctrl) != 0) { io.AddKeyEvent(ImGuiMod_Ctrl, false); }
                           if ((mods & ImGuiMod_Shift) != 0) { io.AddKeyEvent(ImGuiMod_Shift, false); }
                           if ((mods & ImGuiMod_Alt) != 0) { io.AddKeyEvent(ImGuiMod_Alt, false); }
                       }
                       return b >= 15;
                   }};
}

Schritt klickXy(const std::string& was, int taste, ImVec2 an) {
    return Schritt{was, [=](int b) {
                       ImGuiIO& io = ImGui::GetIO();
                       if (b == 0) {
                           setzeMaus(weltXy(an.x, an.y));
                           return false;
                       }
                       if (b == 2) {
                           io.AddMouseButtonEvent(taste, true);
                           return false;
                       }
                       if (b == 3) {
                           io.AddMouseButtonEvent(taste, false);
                           return false;
                       }
                       return b >= 6;
                   }};
}

std::shared_ptr<karte::Pinsel> pinsel() {
    const auto w = ke::editor().karte.welt();
    return (w && !w->kinder().empty()) ? std::dynamic_pointer_cast<karte::Pinsel>(w->kinder().front()) : nullptr;
}

std::size_t beitragend() {
    const auto p = pinsel();
    if (!p) { return 0; }
    p->bRep(ke::editor().karte.kontext);
    std::size_t n = 0;
    for (const auto& f : p->flaechen()) {
        if (f->traegtBei()) { ++n; }
    }
    return n;
}

bool hatEcke(const karte::Vektor3d& v) {
    const auto p = pinsel();
    if (!p) { return false; }
    for (const karte::FlaechenPunkt& fp : p->ecken(ke::editor().karte.kontext)) {
        if (karte::laengeQuadrat(p->eckpunkt(fp) - v) < 1e-6) { return true; }
    }
    return false;
}

Schritt rueckgaengigName(const std::string& name) {
    return pruefe("Rueckgaengig-Name: " + name, [name] {
        diag::detail("Knopftest: Stufe 2 - Rueckgaengig \"" + ke::editor().karte.undo.undoName() + "\"");
        return ke::editor().karte.undo.undoName().rfind(name, 0) == 0;
    });
}

// Grundzustand: Regular, Testkarte (Pinsel -64..64 x -64..64 x 0..64), XY
// Massstab 1 um (0 0 0), Raster 8, Werkzeug Resize, deutsche Tastatur.
Schritt grundzustand(const std::string& was) {
    return tu(was, [] {
        ke::Editor& e = ke::editor();
        e.anordnung = karte::befehle::Anordnung::Regular;
        e.teilerFuer = -1;
        e.anzeige = ke::Anzeige{};
        (void)ke::karteOeffnen(karteTestPfad("karte_kopie.map"));
        e.karte.auswahl.leeren();
        e.bearbeitung.einstellung.modus = karte::AuswahlModus::Primitiv;
        e.bearbeitung.einstellung.komponente = karte::KomponentenModus::Standard;
        e.aktivXy = 0;
        e.xy[0].ansicht.typ = karte::Sicht::XY;
        e.xy[0].ansicht.setzeMassstab(1);
        e.xy[0].ansicht.setzeUrsprung(karte::Vektor3f(0, 0, 0));
        e.raster.setzePotenz(3);
        ke::tastaturErsatz = deutscheTastatur;
        if (e.bearbeitung.modus() != karte::ManipModus::Ziehen) { (void)ke::ausfuehren("MouseDrag"); }
    });
}

// Punkt 1: Komponenten (V/E/F), Ecken ziehen mit Huellenbau, Texture Lock, Build-Werkzeug
std::vector<Schritt> komponenten() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(grundzustand("Stufe 2/1: Grundzustand"));
    add(pause(0.4));
    add(klickXy("M1 auf den Pinsel", ImGuiMouseButton_Left, ImVec2{32, 32}));
    add(pruefe("Pinsel gewaehlt", [] { return ke::editor().karte.auswahl.anzahl() == 1; }));
    add(tasteKombi(ImGuiKey_V, "V"));
    add(pruefe("V: Ecken-Modus (DragVertices angehakt), 8 Ecken gezeichnet ($POINT colour_vertex)", [] {
        const ke::Editor& e = ke::editor();
        const ke::KomponentenZeichnung z = ke::komponentenZeichnung(e.blicke[0]);
        diag::detail("Knopftest: Stufe 2 - Ecken gezeichnet " + std::to_string(z.punkte.size()));
        return ke::angehakt("DragVertices") && e.bearbeitung.einstellung.modus == karte::AuswahlModus::Komponente && z.punkte.size() == 8;
    }));
    add(pause(0.2));
    add(ziehenXy("Ecke (64 64) nach (80 64) ziehen", ImGuiMouseButton_Left, 0, ImVec2{64, 64}, ImVec2{80, 64}));
    add(pruefe("Huelle: 7 Flaechen, Ecke (80 64 64), die Ecke bleibt gewaehlt", [] {
        const ke::KomponentenZeichnung z = ke::komponentenZeichnung(ke::editor().blicke[0]);
        // FaceInstance::iterate_selected: je Flaeche um die Ecke ein $SELPOINT, alle an derselben Stelle
        bool alleDort = !z.gewaehlt.empty();
        for (const karte::Linie& l : z.gewaehlt) { alleDort = alleDort && l.a == karte::Vektor3f(80, 64, 64); }
        diag::detail("Knopftest: Stufe 2 - Flaechen " + std::to_string(beitragend()) + ", gewaehlte Punkte " + std::to_string(z.gewaehlt.size()) +
                     (alleDort ? " bei (80 64 64)" : " verstreut") + ", Ecken " + (hatEcke(karte::Vektor3d(80, 64, 64)) ? "a" : "-") +
                     (hatEcke(karte::Vektor3d(64, 64, 0)) ? "b" : "-"));
        return beitragend() == 7 && hatEcke(karte::Vektor3d(80, 64, 64)) && hatEcke(karte::Vektor3d(64, 64, 0)) && alleDort;
    }));
    add(rueckgaengigName("dragTool"));
    add(foto("karteneditor_ecken"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pruefe("Strg+Z: wieder 6 Flaechen", [] { return beitragend() == 6 && hatEcke(karte::Vektor3d(64, 64, 64)); }));
    add(tasteKombi(ImGuiKey_E, "E"));
    add(pruefe("E: Kanten-Modus, 12 Kantenmitten", [] {
        const ke::KomponentenZeichnung z = ke::komponentenZeichnung(ke::editor().blicke[0]);
        return ke::angehakt("DragEdges") && !ke::angehakt("DragVertices") && z.punkte.size() == 12;
    }));
    add(ziehenXy("Kante (64 0 64) nach (80 0) ziehen", ImGuiMouseButton_Left, 0, ImVec2{64, 0}, ImVec2{80, 0}));
    add(pruefe("Kante: oben x=80, unten x=64, 6 Flaechen", [] {
        return beitragend() == 6 && hatEcke(karte::Vektor3d(80, 64, 64)) && hatEcke(karte::Vektor3d(80, -64, 64)) && hatEcke(karte::Vektor3d(64, 64, 0));
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tasteKombi(ImGuiKey_F, "F"));
    add(pruefe("F: Flaechen-Modus, 6 Schwerpunkte", [] {
        const ke::KomponentenZeichnung z = ke::komponentenZeichnung(ke::editor().blicke[0]);
        return ke::angehakt("DragFaces") && z.punkte.size() == 6;
    }));
    add(tasteKombi(ImGuiKey_F, "F"));
    add(pruefe("F nochmal: zurueck in den Primitivmodus", [] {
        return !ke::angehakt("DragFaces") && ke::editor().bearbeitung.einstellung.modus == karte::AuswahlModus::Primitiv;
    }));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_T, "Umschalt+T"));
    add(pause(0.2));
    add(pruefe("Umschalt+T: Texture Lock an, Statuszeile L:ON", [] {
        return ke::angehakt("TogTexLock") && ke::editor().karte.kontext.texturLock;
    }));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_T, "Umschalt+T"));
    add(pruefe("Umschalt+T: Texture Lock aus", [] { return !ke::angehakt("TogTexLock") && !ke::editor().karte.kontext.texturLock; }));
    add(tu("TogTexVertexLock", [] { (void)ke::ausfuehren("TogTexVertexLock"); }));
    add(pruefe("Texture Vertex Lock an", [] { return ke::angehakt("TogTexVertexLock") && ke::editor().bearbeitung.texturEckenSperre; }));
    add(tu("TogTexVertexLock", [] { (void)ke::ausfuehren("TogTexVertexLock"); }));
    add(tasteKombi(ImGuiKey_B, "B"));
    add(pruefe("B: Build-Werkzeug, Status wie NRC", [] {
        const ke::Editor& e = ke::editor();
        return ke::angehakt("MouseBuild") && e.bearbeitung.modus() == karte::ManipModus::Bauen &&
               e.statusBefehl == "Build Tool: extrude, build chains, clone";
    }));
    add(klickXy("Build: M1 neben den Pinsel", ImGuiMouseButton_Left, ImVec2{200, 200}));
    add(pruefe("Build: der Klick waehlt nicht ab (testSelect_simpleM1 nicht im Build-Modus)", [] {
        return ke::editor().karte.auswahl.anzahl() == 1;
    }));
    add(tasteKombi(ImGuiKey_B, "B"));
    add(pruefe("B nochmal: Resize", [] { return ke::angehakt("MouseDrag"); }));
    // UV-Werkzeug (G): mit gewaehlter Deckflaeche zeichnet die Kamera das Texturgitter
    add(tu("Deckflaeche des Pinsels waehlen", [] {
        ke::Editor& e = ke::editor();
        const auto p = pinsel();
        if (!p) { return; }
        p->bRep(e.karte.kontext);
        for (const auto& f : p->flaechen()) {
            if (f->ebene().normal()[2] > 0.9) { e.karte.auswahl.flaecheWaehlen(p, f); }
        }
        e.kamera.setzeUrsprung(karte::Vektor3f(0, -160, 224));
        e.kamera.setzeWinkel(karte::Vektor3f(-50, 90, 0));
    }));
    add(tasteKombi(ImGuiKey_G, "G"));
    add(pause(0.3));
    add(pruefe("G: UV-Werkzeug, Status wie NRC, Texturgitter in der Kamera", [] {
        const ke::Editor& e = ke::editor();
        const karte::ManipZeichnung mz = ke::manipulatorZeichnung(3);
        diag::detail("Knopftest: Stufe 2 - UV-Gitter " + std::to_string(mz.blendLinien.size()) + " Linien, Pivot " +
                     std::to_string(mz.pivotLinien.size()));
        return ke::angehakt("MouseUV") && e.bearbeitung.modus() == karte::ManipModus::UV &&
               e.statusBefehl == "UV Tool: edit texture alignment" && mz.blendLinien.size() > 8 && mz.pivotLinien.size() == 3;
    }));
    add(foto("karteneditor_uv"));
    add(tasteKombi(ImGuiKey_G, "G"));
    add(pruefe("G nochmal: Resize", [] { return ke::angehakt("MouseDrag"); }));
    return s;
}

Schritt klickXyMit(const std::string& was, ImGuiKeyChord mods, ImVec2 an) {
    return Schritt{was, [=](int b) {
                       ImGuiIO& io = ImGui::GetIO();
                       if (b == 0) {
                           setzeMaus(weltXy(an.x, an.y));
                           if ((mods & ImGuiMod_Ctrl) != 0) { io.AddKeyEvent(ImGuiMod_Ctrl, true); }
                           return false;
                       }
                       if (b == 2) {
                           io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
                           return false;
                       }
                       if (b == 3) {
                           io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
                           return false;
                       }
                       if (b == 4 && (mods & ImGuiMod_Ctrl) != 0) { io.AddKeyEvent(ImGuiMod_Ctrl, false); }
                       return b >= 6;
                   }};
}

// Zwei Klicks kurz nacheinander (Clipper_tryDoubleclick: unter 200 ms)
Schritt doppelklickXy(const std::string& was, ImVec2 an) {
    return Schritt{was, [=](int b) {
                       ImGuiIO& io = ImGui::GetIO();
                       if (b == 0) {
                           setzeMaus(weltXy(an.x, an.y));
                           return false;
                       }
                       if (b == 1 || b == 3) {
                           io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
                           return false;
                       }
                       if (b == 2 || b == 4) {
                           io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
                           return false;
                       }
                       return b >= 7;
                   }};
}

std::size_t pinselAnzahl() {
    const auto w = ke::editor().karte.welt();
    return w ? w->kinder().size() : 0;
}

karte::Kasten pinselGrenzen(std::size_t i) {
    const auto w = ke::editor().karte.welt();
    if (!w || i >= w->kinder().size()) { return karte::Kasten(); }
    return std::static_pointer_cast<karte::Pinsel>(w->kinder()[i])->grenzen(ke::editor().karte.kontext);
}

// Punkt 2: Clipper (X, Return, Shift+Return, Ctrl+Return, Strg+M1 in 2D, Doppelklick)
std::vector<Schritt> clipper() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(grundzustand("Stufe 2/2: Grundzustand"));
    add(pause(0.4));
    add(klickXy("M1 auf den Pinsel", ImGuiMouseButton_Left, ImVec2{32, 32}));
    add(tasteKombi(ImGuiKey_X, "X"));
    add(pruefe("X: Clipper, Status wie NRC", [] {
        const ke::Editor& e = ke::editor();
        return ke::angehakt("ToggleClipper") && e.bearbeitung.modus() == karte::ManipModus::Clipper &&
               e.statusBefehl == "Clipper Tool: apply clip planes to brushes";
    }));
    add(klickXy("Clipper: Punkt 1 bei (0 -96)", ImGuiMouseButton_Left, ImVec2{0, -96}));
    add(klickXy("Clipper: Punkt 2 bei (0 96)", ImGuiMouseButton_Left, ImVec2{0, 96}));
    add(pause(0.2));
    add(pruefe("zwei Punkte, die Schnittebene liegt im Pinsel (BrushClipPlane)", [] {
        const ke::Editor& e = ke::editor();
        const std::vector<ke::ClipperFlaeche> f = ke::clipperFlaechen();
        return e.bearbeitung.clipperPunkte.anzahl == 2 && f.size() == 1 && f[0].punkte.size() == 4;
    }));
    add(foto("karteneditor_clipper"));
    add(tasteKombi(ImGuiKey_Enter, "Return"));
    add(pruefe("Return: Clip, es bleibt x 0..64 (Normale (p2-p0) x (p1-p0) = -x)", [] {
        const karte::Kasten b = pinselGrenzen(0);
        diag::detail("Knopftest: Stufe 2 - Clip x " + std::to_string(b.origin[0] - b.extents[0]) + ".." + std::to_string(b.origin[0] + b.extents[0]));
        return pinselAnzahl() == 1 && std::fabs(b.origin[0] - b.extents[0]) < 0.01F && std::fabs(b.origin[0] + b.extents[0] - 64) < 0.01F;
    }));
    add(rueckgaengigName("clipperClip"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(klickXy("Clipper: Punkt 1 bei (0 -96)", ImGuiMouseButton_Left, ImVec2{0, -96}));
    add(klickXy("Clipper: Punkt 2 bei (0 96)", ImGuiMouseButton_Left, ImVec2{0, 96}));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Enter, "Strg+Return"));
    add(pruefe("Strg+Return: Flip", [] { return ke::editor().bearbeitung.clipperUmgedreht; }));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_Enter, "Umschalt+Return"));
    add(pruefe("Umschalt+Return: Split in zwei Pinsel", [] { return pinselAnzahl() == 2 && ke::editor().karte.auswahl.anzahl() == 2; }));
    add(rueckgaengigName("clipperSplit"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    // Doppelklick auf einen gesetzten Punkt: teilt ("On DoubleClick do: Split")
    add(klickXy("Clipper: Punkt 1 bei (0 -96)", ImGuiMouseButton_Left, ImVec2{0, -96}));
    add(klickXy("Clipper: Punkt 2 bei (0 96)", ImGuiMouseButton_Left, ImVec2{0, 96}));
    add(doppelklickXy("Doppelklick auf Punkt 2", ImVec2{0, 96}));
    add(pruefe("Doppelklick: Split", [] { return pinselAnzahl() == 2; }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tasteKombi(ImGuiKey_X, "X"));
    add(pruefe("X nochmal: Resize", [] { return ke::angehakt("MouseDrag"); }));
    // Strg+M1 in 2D: der Clipper fuer einen Schnitt (ClipperModeQuick)
    add(klickXy("M1 auf den Pinsel", ImGuiMouseButton_Left, ImVec2{32, 32}));
    add(klickXyMit("Strg+M1 bei (32 -96)", ImGuiMod_Ctrl, ImVec2{32, -96}));
    add(pruefe("Strg+M1: Clipper eingeschaltet (schnell)", [] {
        const ke::Editor& e = ke::editor();
        return e.bearbeitung.modus() == karte::ManipModus::Clipper && e.bearbeitung.clipperSchnell && e.bearbeitung.clipperPunkte.anzahl == 1;
    }));
    add(klickXyMit("Strg+M1 bei (32 96)", ImGuiMod_Ctrl, ImVec2{32, 96}));
    add(tasteKombi(ImGuiKey_Enter, "Return"));
    add(pruefe("schneller Clip: x 32..64, danach wieder Resize", [] {
        const karte::Kasten b = pinselGrenzen(0);
        return ke::angehakt("MouseDrag") && !ke::editor().bearbeitung.clipperSchnell && std::fabs(b.origin[0] - b.extents[0] - 32) < 0.01F;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    return s;
}

}  // namespace stufe2

namespace stufe2 {

std::function<ImVec2()> elementOrt(const std::string& id) {
    return [id]() {
        const Element* el = finde(id, "", 0, true);
        return el != nullptr ? el->rect.GetCenter() : ImVec2{0, 0};
    };
}

std::size_t pinselZahl() {
    const auto w = ke::editor().karte.welt();
    return w ? w->kinder().size() : 0;
}

karte::Kasten grenzenVon(std::size_t i) {
    const auto w = ke::editor().karte.welt();
    if (!w || i >= w->kinder().size()) { return karte::Kasten(); }
    return std::static_pointer_cast<karte::Pinsel>(w->kinder()[i])->grenzen(ke::editor().karte.kontext);
}

bool kastenGleich(const karte::Kasten& b, const karte::Vektor3f& mn, const karte::Vektor3f& mx) {
    for (std::size_t i = 0; i < 3; ++i) {
        if (std::fabs(b.origin[i] - b.extents[i] - mn[i]) > 0.01F || std::fabs(b.origin[i] + b.extents[i] - mx[i]) > 0.01F) { return false; }
    }
    return true;
}

// Ein zweiter Pinsel (32 -16 16)..(96 16 48), durch die +x-Seite des Testpinsels
Schritt schneiderDazu(const std::string& was, bool beideWaehlen) {
    return tu(was, [beideWaehlen] {
        ke::Editor& e = ke::editor();
        e.karte.undo.start();
        karte::TexturProjektion proj;
        karte::texdefStandard(karte::TexdefArt::Quake, proj);
        auto p = std::make_shared<karte::Pinsel>();
        e.karte.welt()->kindEinfuegen(p);
        p->quaderBauen(karte::Kasten(karte::Vektor3f(64, 0, 32), karte::Vektor3f(32, 16, 16)), "textures/common/caulk", proj, e.karte.kontext);
        e.karte.undo.finish("brushDragNew");
        e.karte.auswahl.leeren();
        if (beideWaehlen) { e.karte.auswahl.waehlen(e.karte.welt(), e.karte.welt()->kinder().front()); }
        e.karte.auswahl.waehlen(e.karte.welt(), p);
        ke::auswahlGeaendert();
    });
}

// Punkt 3: CSG (Shift+U, Strg+U, Strg+Umschalt+U, Make Room, CSG Tool, Alt+D/Alt+S)
std::vector<Schritt> csg() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(grundzustand("Stufe 2/3: Grundzustand"));
    add(pause(0.4));
    add(klickXy("M1 auf den Pinsel", ImGuiMouseButton_Left, ImVec2{32, 32}));
    add(klickAn("Knopf Room", elementOrt("##knopfCSGroom")));
    add(pruefe("Room: 6 Waende 8 dick (Raster), +x-Wand x 64..72", [] {
        bool plusX = false;
        for (std::size_t i = 0; i < pinselZahl(); ++i) {
            plusX = plusX || kastenGleich(grenzenVon(i), karte::Vektor3f(64, -64, 0), karte::Vektor3f(72, 64, 64));
        }
        diag::detail("Knopftest: Stufe 2 - Room " + std::to_string(pinselZahl()) + " Pinsel");
        return pinselZahl() == 6 && plusX;
    }));
    add(rueckgaengigName("makeRoom"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pruefe("Room rueckgaengig: ein Pinsel", [] { return pinselZahl() == 1; }));
    // CSG Tool
    add(klickAn("Knopf CSG Tool", elementOrt("##knopfCSGTool")));
    add(pause(0.3));
    add(pruefe("CSG Tool offen, Vorgaben wie NRC (16, -faces, Caulk, Innen weg)", [] {
        const ke::Editor::CsgWerkzeug& w = ke::editor().csgWerkzeug;
        return w.offen && finde("##csgpull", "", 0, true) != nullptr && w.dicke == 16.0 && w.ausschluss == 0 && w.caulk && w.innenWeg;
    }));
    add(foto("karteneditor_csgtool"));
    add(klickXy("M1 auf den Pinsel", ImGuiMouseButton_Left, ImVec2{32, 32}));
    add(klickAn("Hollow::pull faces", elementOrt("##csgpull")));
    add(pruefe("Pull: 6 Platten 16 dick, das Innere weg", [] {
        bool plusX = false;
        for (std::size_t i = 0; i < pinselZahl(); ++i) {
            plusX = plusX || kastenGleich(grenzenVon(i), karte::Vektor3f(64, -64, 0), karte::Vektor3f(80, 64, 64));
        }
        return pinselZahl() == 6 && plusX;
    }));
    add(rueckgaengigName("brushHollow::Pull"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(klickAn("Grid-> (ohne Ziehen: das Raster)", elementOrt("##csggitter")));
    add(pruefe("Grid->: Dicke und Schritt = Raster 8", [] {
        const ke::Editor::CsgWerkzeug& w = ke::editor().csgWerkzeug;
        return w.dicke == 8.0 && w.schritt == 8.0;
    }));
    add(klickAn("Pfeil hoch", elementOrt("##csgdickehoch")));
    add(pruefe("Pfeil hoch: 16", [] { return ke::editor().csgWerkzeug.dicke == 16.0; }));
    add(klickAn("-proj", elementOrt(std::string(ke::trK(Str::KeCsgMinusProj)) + "##csgminusproj")));
    add(klickAn("Remove inner brush aus", elementOrt("##csginnenweg")));
    add(pruefe("-proj gewaehlt, Innen weg aus", [] {
        const ke::Editor::CsgWerkzeug& w = ke::editor().csgWerkzeug;
        return w.ausschluss == 2 && !w.innenWeg;
    }));
    add(klickXy("M1 auf den Pinsel", ImGuiMouseButton_Left, ImVec2{32, 32}));
    add(klickAn("Hollow::extrude faces", elementOrt("##csgextrude")));
    add(pruefe("Extrude mit -proj (XY): 4 Seitenplatten, das Original bleibt", [] {
        diag::detail("Knopftest: Stufe 2 - Extrude -proj " + std::to_string(pinselZahl()) + " Pinsel");
        return pinselZahl() == 5 && kastenGleich(grenzenVon(0), karte::Vektor3f(-64, -64, 0), karte::Vektor3f(64, 64, 64));
    }));
    add(rueckgaengigName("brushHollow::Extrude"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(klickAn("Expand brush", elementOrt("##csgerweitern")));
    add(pruefe("Expand mit -proj: nur die Seiten 16 nach aussen", [] {
        return kastenGleich(grenzenVon(0), karte::Vektor3f(-80, -80, 0), karte::Vektor3f(80, 80, 64)) && ke::editor().karte.undo.undoName() == "Expand brush";
    }));
    add(klickAn("Shrink brush", elementOrt("##csgschrumpfen")));
    add(pruefe("Shrink: zurueck", [] {
        return kastenGleich(grenzenVon(0), karte::Vektor3f(-64, -64, 0), karte::Vektor3f(64, 64, 64)) && ke::editor().karte.undo.undoName() == "Shrink brush";
    }));
    add(klickAn("-faces", elementOrt(std::string(ke::trK(Str::KeCsgMinusFaces)) + "##csgminusfaces")));
    add(klickAn("Remove inner brush an", elementOrt("##csginnenweg")));
    add(tu("CSG Tool zu", [] { ke::editor().csgWerkzeug.offen = false; }));
    // Subtract (Umschalt+U): Schneider durch die +x-Seite; Z vor Y vor X
    add(schneiderDazu("Schneider dazu, nur er gewaehlt", false));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_U, "Umschalt+U"));
    add(pruefe("Subtract: 5 Teile, zuerst die Platte z 48..64", [] {
        diag::detail("Knopftest: Stufe 2 - Subtract " + std::to_string(pinselZahl()) + " Pinsel");
        return pinselZahl() == 6 && kastenGleich(grenzenVon(1), karte::Vektor3f(-64, -64, 48), karte::Vektor3f(64, 64, 64)) &&
               ke::editor().karte.undo.undoName() == "brushSubtract";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    // Intersect (Strg+Umschalt+U): der zuletzt gewaehlte bleibt
    add(schneiderDazu("Schneider dazu, beide gewaehlt", true));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_U, "Strg+Umschalt+U"));
    add(pruefe("Intersect: ein Pinsel (32 -16 16)..(64 16 48)", [] {
        return pinselZahl() == 1 && kastenGleich(grenzenVon(0), karte::Vektor3f(32, -16, 16), karte::Vektor3f(64, 16, 48)) &&
               ke::editor().karte.undo.undoName() == "brushIntersect";
    }));
    // Rueckgaengig waehlt die wiederhergestellten Pinsel nicht (NRC: neue
    // Instanzen sind nicht gewaehlt), also den Schneider neu
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pruefe("Intersect rueckgaengig: beide da, nur der Schneider gewaehlt", [] {
        return pinselZahl() == 2 && ke::editor().karte.auswahl.anzahl() == 1;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    // Wrap Merge (Strg+U): die Huelle beider
    add(schneiderDazu("Schneider dazu, beide gewaehlt", true));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_U, "Strg+U"));
    add(pruefe("Wrap Merge: ein Pinsel x -64..96 mit schraegen Flaechen", [] {
        const auto w = ke::editor().karte.welt();
        const std::size_t n = pinselZahl() == 1 ? std::static_pointer_cast<karte::Pinsel>(w->kinder()[0])->anzahl() : 0;
        diag::detail("Knopftest: Stufe 2 - Wrap Merge " + std::to_string(n) + " Flaechen");
        return pinselZahl() == 1 && kastenGleich(grenzenVon(0), karte::Vektor3f(-64, -64, 0), karte::Vektor3f(96, 64, 64)) && n > 6 &&
               ke::editor().karte.undo.undoName() == "brushWrapMerge";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    // Merge (Brush > CSG > CSG Merge): nicht konvex -> abgelehnt
    add(schneiderDazu("Schneider dazu, beide gewaehlt", true));
    add(tu("CSG Merge (Menue Brush > CSG)", [] { (void)ke::ausfuehren("CSGMerge"); }));
    add(pruefe("Merge abgelehnt (nicht konvex), beide bleiben, kein Schritt", [] { return pinselZahl() == 2 && ke::editor().karte.undo.undoName() != "brushMerge"; }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    // Make detail / Make structural
    add(klickXy("M1 auf den Pinsel", ImGuiMouseButton_Left, ImVec2{-32, -32}));
    add(tasteKombi(ImGuiMod_Alt | ImGuiKey_D, "Alt+D"));
    add(pruefe("Alt+D: Make detail", [] {
        const auto w = ke::editor().karte.welt();
        return pinselZahl() == 1 && std::static_pointer_cast<karte::Pinsel>(w->kinder()[0])->istDetail() &&
               ke::editor().karte.undo.undoName() == "brushSetDetail";
    }));
    add(tasteKombi(ImGuiMod_Alt | ImGuiKey_S, "Alt+S"));
    add(pruefe("Alt+S: Make structural", [] {
        const auto w = ke::editor().karte.welt();
        return pinselZahl() == 1 && !std::static_pointer_cast<karte::Pinsel>(w->kinder()[0])->istDetail() &&
               ke::editor().karte.undo.undoName() == "brushClearDetail";
    }));
    return s;
}

std::size_t flaechenVon(std::size_t i) {
    const auto w = ke::editor().karte.welt();
    if (!w || i >= w->kinder().size()) { return 0; }
    return std::static_pointer_cast<karte::Pinsel>(w->kinder()[i])->anzahl();
}

// Punkt 4: Grundformen (Strg+3..9, Prism..., Cone..., Sphere..., Rock..., Icosahedron...)
std::vector<Schritt> grundformen() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(grundzustand("Stufe 2/4: Grundzustand"));
    add(pause(0.4));
    add(klickXy("M1 auf den Pinsel", ImGuiMouseButton_Left, ImVec2{32, 32}));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_5, "Strg+5"));
    add(pruefe("Strg+5: Prisma mit 5 Seiten (7 Flaechen), Achse z", [] {
        return flaechenVon(0) == 7 && ke::editor().karte.undo.undoName() == "brushPrism -sides 5 -axis 2";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tu("Brush > Prism...", [] { (void)ke::ausfuehren("BrushPrism"); }));
    add(pause(0.3));
    add(pruefe("Arbitrary sides: Sides 8 (Vorgabe), ohne Truncate", [] {
        const ke::Editor::SeitenDialog& d = ke::editor().seitenDialog;
        return d.offen && d.wert == 8 && finde("##seitenwert", "", 0, true) != nullptr && finde("##seitenstutzen", "", 0, true) == nullptr;
    }));
    add(foto("karteneditor_seiten_prism"));
    add(klickAn("Pfeil hoch", elementOrt("##seitenhoch")));
    add(klickAn("OK", elementOrt(std::string(ke::trK(Str::KePrefOk)) + "##seitenok")));
    add(pruefe("OK: Prisma mit 9 Seiten (11 Flaechen)", [] {
        return !ke::editor().seitenDialog.offen && flaechenVon(0) == 11 && ke::editor().karte.undo.undoName() == "brushPrism -sides 9 -axis 2";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tu("Brush > Icosahedron...", [] { (void)ke::ausfuehren("BrushIcosahedron"); }));
    add(pause(0.3));
    add(pruefe("Icosahedron: Subdivisions 1, Truncate sichtbar", [] {
        const ke::Editor::SeitenDialog& d = ke::editor().seitenDialog;
        return d.offen && d.wert == 1 && finde(std::string(ke::trK(Str::KeSidesTruncate)) + "##seitenstutzen", "", 0, true) != nullptr;
    }));
    add(foto("karteneditor_seiten_ikosaeder"));
    add(klickAn("Pfeil runter", elementOrt("##seitenrunter")));
    add(klickAn("Truncate", elementOrt(std::string(ke::trK(Str::KeSidesTruncate)) + "##seitenstutzen")));
    add(klickAn("OK", elementOrt(std::string(ke::trK(Str::KePrefOk)) + "##seitenok")));
    add(pruefe("Icosahedron 0 gestutzt: 32 Flaechen", [] {
        return flaechenVon(0) == 32 && ke::editor().karte.undo.undoName() == "brushIcosahedron -subdivisions 0";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tu("Brush > Rock...", [] { (void)ke::ausfuehren("BrushRock"); }));
    add(pause(0.3));
    add(pruefe("Rock: Sides 32", [] { return ke::editor().seitenDialog.offen && ke::editor().seitenDialog.wert == 32; }));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(pruefe("Esc: abgebrochen, der Pinsel bleibt ein Quader", [] { return !ke::editor().seitenDialog.offen && flaechenVon(0) == 6; }));
    add(tu("Brush > Sphere...", [] { (void)ke::ausfuehren("BrushSphere"); }));
    add(pause(0.3));
    add(tasteKombi(ImGuiKey_Enter, "Return"));
    add(pruefe("Return: Kugel mit 8 Seiten (64 Ebenen)", [] {
        return flaechenVon(0) == 64 && ke::editor().karte.undo.undoName() == "brushSphere -sides 8";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tu("Brush > Cone...", [] { (void)ke::ausfuehren("BrushCone"); }));
    add(pause(0.3));
    add(tasteKombi(ImGuiKey_Enter, "Return"));
    add(pruefe("Return: Kegel mit 8 Seiten (9 Flaechen)", [] {
        return flaechenVon(0) == 9 && ke::editor().karte.undo.undoName() == "brushCone -sides 8 -axis 2";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    return s;
}

karte::Kasten letzterGrenzen() {
    const auto w = ke::editor().karte.welt();
    if (!w || w->kinder().empty()) { return karte::Kasten(); }
    return std::static_pointer_cast<karte::Pinsel>(w->kinder().back())->grenzen(ke::editor().karte.kontext);
}

// Punkt 5: Drehen, Spiegeln, Arbitrary rotation/scale, Repeat/Reset Transforms
std::vector<Schritt> transformationen() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto nurSchneider = [] {
        return tu("nur den Schneider waehlen", [] {
            ke::Editor& e = ke::editor();
            e.karte.auswahl.leeren();
            e.karte.auswahl.waehlen(e.karte.welt(), e.karte.welt()->kinder().back());
            ke::auswahlGeaendert();
        });
    };
    add(grundzustand("Stufe 2/5: Grundzustand"));
    add(pause(0.4));
    add(schneiderDazu("Schneider (32 -16 16)..(96 16 48), nur er gewaehlt", false));
    add(klickAn("Knopf Rotate Clockwise", elementOrt("##knopfRotateSelectionClockwise")));
    add(pruefe("Rotate Clockwise (XY): um z -90 um die Mitte (64 0 32)", [] {
        return kastenGleich(letzterGrenzen(), karte::Vektor3f(48, -32, 16), karte::Vektor3f(80, 32, 48)) &&
               ke::editor().karte.undo.undoName() == "rotateSelected Clockwise 90";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tu("beide waehlen", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        for (const auto& p : e.karte.welt()->kinder()) { e.karte.auswahl.waehlen(e.karte.welt(), p); }
        ke::auswahlGeaendert();
    }));
    add(klickAn("Knopf Flip Horizontally", elementOrt("##knopfMirrorSelectionHorizontally")));
    add(pruefe("Flip Horizontally (XY): an x 16 gespiegelt, der Schneider bei -64..0", [] {
        return kastenGleich(letzterGrenzen(), karte::Vektor3f(-64, -16, 16), karte::Vektor3f(0, 16, 48)) &&
               ke::editor().karte.undo.undoName() == "mirrorSelected -axis x";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(nurSchneider());
    // Arbitrary rotation (Umschalt+R)
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_R, "Umschalt+R"));
    add(pause(0.3));
    add(pruefe("Arbitrary rotation offen, 0 0 0", [] {
        const auto& d = ke::editor().drehDialog;
        return d.offen && d.werte[0] == 0.0 && d.werte[2] == 0.0 && finde("##kartedrehenwert2", "", 0, true) != nullptr;
    }));
    add(foto("karteneditor_drehen"));
    add(klickAn("Feld Z", elementOrt("##kartedrehenwert2")));
    add(tippe("90"));
    add(klickAn("Apply", elementOrt(std::string(ke::trK(Str::KeArbApply)) + "##kartedrehenanwenden")));
    add(pruefe("Apply: um z 90, der Dialog bleibt offen", [] {
        return ke::editor().drehDialog.offen && kastenGleich(letzterGrenzen(), karte::Vektor3f(48, -32, 16), karte::Vektor3f(80, 32, 48)) &&
               ke::editor().karte.undo.undoName() == "rotateSelectedEulerXYZ -x 0 -y 0 -z 90";
    }));
    add(klickAn("Feld X", elementOrt("##kartedrehenwert0")));
    add(tasteKombi(ImGuiKey_Enter, "Return"));
    add(pruefe("Return: OK - noch einmal 90, zu, die Werte bleiben", [] {
        const auto& d = ke::editor().drehDialog;
        return !d.offen && d.werte[2] == 90.0 && kastenGleich(letzterGrenzen(), karte::Vektor3f(32, -16, 16), karte::Vektor3f(96, 16, 48));
    }));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_R, "Umschalt+R"));
    add(pause(0.2));
    add(klickAn("Feld X", elementOrt("##kartedrehenwert0")));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(pruefe("Esc: Cancel - zu, Werte 0, die Auswahl bleibt", [] {
        const auto& d = ke::editor().drehDialog;
        return !d.offen && d.werte[2] == 0.0 && ke::editor().karte.auswahl.anzahl() == 1;
    }));
    // Arbitrary scale (Strg+Umschalt+S)
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S, "Strg+Umschalt+S"));
    add(pause(0.3));
    add(foto("karteneditor_skalieren"));
    add(klickAn("Feld X", elementOrt("##karteskalierenwert0")));
    add(tippe("2"));
    add(klickAn("OK", elementOrt(std::string(ke::trK(Str::KePrefOk)) + "##karteskalierenok")));
    add(pruefe("Arbitrary scale 2 1 1: x 0..128", [] {
        return !ke::editor().skalierDialog.offen && kastenGleich(letzterGrenzen(), karte::Vektor3f(0, -16, 16), karte::Vektor3f(128, 16, 48)) &&
               ke::editor().karte.undo.undoName() == "scaleSelected -x 2 -y 1 -z 1";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    // Repeat / Reset Transforms
    add(tasteKombi(ImGuiMod_Alt | ImGuiKey_R, "Alt+R"));
    add(pruefe("Alt+R: alles zurueckgesetzt (Texte mit Leerzeichen vorn)", [] {
        const auto& b = ke::editor().bearbeitung;
        return b.wiederholbar().eins() && b.transformText(0) == "  Translate 0 0 0" && b.transformText(2) == "  Scale 1 1 1";
    }));
    add(tasteKombi(ImGuiMod_Alt | ImGuiKey_RightArrow, "Alt+Rechts"));
    add(pruefe("Nudge rechts: 'x Translate 8 0 0'", [] { return ke::editor().bearbeitung.transformText(0) == "x Translate 8 0 0"; }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_R, "Strg+R"));
    add(pruefe("Strg+R: Kopie 8 weiter, die Kopie gewaehlt", [] {
        return pinselZahl() == 3 && kastenGleich(letzterGrenzen(), karte::Vektor3f(48, -16, 16), karte::Vektor3f(112, 16, 48)) &&
               ke::editor().karte.undo.undoName() == "repeatTransforms" && ke::editor().karte.auswahl.anzahl() == 1;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    return s;
}

std::shared_ptr<karte::Patch> patchNr(std::size_t i) {
    const auto w = ke::editor().karte.welt();
    std::size_t n = 0;
    for (const auto& c : w->kinder()) {
        if (c->art() == karte::Primitiv::Art::Patch) {
            if (n == i) { return std::static_pointer_cast<karte::Patch>(c); }
            ++n;
        }
    }
    return nullptr;
}

std::size_t patchZahl() {
    std::size_t n = 0;
    for (const auto& c : ke::editor().karte.welt()->kinder()) { n += c->art() == karte::Primitiv::Art::Patch ? 1U : 0U; }
    return n;
}

// Punkt 6: das Curve-Menue und seine Dialoge
std::vector<Schritt> patches() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(grundzustand("Stufe 2/6: Grundzustand"));
    add(pause(0.4));
    add(klickXy("M1 auf den Pinsel", ImGuiMouseButton_Left, ImVec2{32, 32}));
    add(tu("Curve > Cylinder (9x3)", [] { (void)ke::ausfuehren("PatchCylinder"); }));
    add(pruefe("Cylinder: 9x3 statt des Pinsels, gewaehlt", [] {
        const auto p = patchNr(0);
        return p && p->breite() == 9 && p->hoehe() == 3 && pinselZahl() == 1 && ke::editor().karte.auswahl.anzahl() == 1 &&
               ke::editor().karte.undo.undoName() == "patchCreateCylinder";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_KeypadAdd, "Strg+Umschalt+Num+"));
    add(pruefe("Insert (2) First Columns: 11 breit", [] {
        return patchNr(0)->breite() == 11 && ke::editor().karte.undo.undoName() == "patchInsertFirstColumns";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_M, "Strg+Umschalt+M"));
    add(pruefe("Transpose: 3 x 11", [] { return patchNr(0)->breite() == 3 && patchNr(0)->hoehe() == 11; }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_I, "Strg+I"));
    add(pruefe("Invert Matrix", [] { return ke::editor().karte.undo.undoName() == "patchInvert"; }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_N, "Strg+N"));
    add(pruefe("Naturalize", [] { return ke::editor().karte.undo.undoName() == "patchNaturalTexture"; }));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_I, "Umschalt+I"));
    add(pruefe("Invert Y", [] { return ke::editor().karte.undo.undoName() == "patchFlipTextureV"; }));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_N, "Umschalt+N"));
    add(pruefe("Reset Texture", [] { return ke::editor().karte.undo.undoName() == "textureReset/Cap"; }));
    for (int i = 0; i < 6; ++i) { add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z")); }
    add(pruefe("zurueck: Zylinder 9x3", [] { return patchNr(0) && patchNr(0)->breite() == 9 && patchNr(0)->hoehe() == 3; }));
    // Cap (Umschalt+C)
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_C, "Umschalt+C"));
    add(pause(0.3));
    add(pruefe("Cap: Dialog mit Bevel gewaehlt", [] {
        const auto& d = ke::editor().patchDialog;
        return d.art == 3 && d.kappe == 0 && finde(std::string(ke::trK(Str::KePdCapCylinder)) + "##patchkappe4", "", 0, true) != nullptr;
    }));
    add(foto("karteneditor_patch_cap"));
    add(klickAn("Cylinder", elementOrt(std::string(ke::trK(Str::KePdCapCylinder)) + "##patchkappe4")));
    add(klickAn("OK", elementOrt(std::string(ke::trK(Str::KePrefOk)) + "##patchok")));
    add(pruefe("Cylinder-Kappen: zwei 3x5, alle gewaehlt", [] {
        const auto c = patchNr(1);
        return patchZahl() == 3 && c && c->breite() == 3 && c->hoehe() == 5 && ke::editor().karte.auswahl.anzahl() == 3 &&
               ke::editor().karte.undo.undoName() == "patchPutCaps";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tu("nur den Zylinder waehlen", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), patchNr(0));
        ke::auswahlGeaendert();
    }));
    // Thicken (Strg+T)
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_T, "Strg+T"));
    add(pause(0.3));
    add(pruefe("Thicken: Dicke = Raster 8, Normal, Side walls", [] {
        const auto& d = ke::editor().patchDialog;
        return d.art == 4 && d.wert == 8 && d.achse == 3 && d.waende;
    }));
    add(foto("karteneditor_patch_thicken"));
    add(klickAn("OK", elementOrt(std::string(ke::trK(Str::KePrefOk)) + "##patchok")));
    add(pruefe("Thicken am Zylinder: Gegenseite + 2 Waende (Spalten geschlossen)", [] {
        diag::detail("Knopftest: Stufe 2 - Thicken " + std::to_string(patchZahl()) + " Patches");
        return patchZahl() == 4 && ke::editor().karte.undo.undoName() == "patchThicken";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    // Deform: Esc
    add(tu("Curve > Deform...", [] { (void)ke::ausfuehren("PatchDeform"); }));
    add(pause(0.3));
    add(pruefe("Deform: Max deform 64, Z", [] { return ke::editor().patchDialog.art == 2 && ke::editor().patchDialog.wert == 64; }));
    add(foto("karteneditor_patch_deform"));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(pruefe("Esc: abgebrochen", [] { return ke::editor().patchDialog.art == 0 && patchZahl() == 1; }));
    // Simple Patch Mesh (Umschalt+P)
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_P, "Umschalt+P"));
    add(pause(0.3));
    add(pruefe("Patch density: 3 x 3, ohne Square", [] {
        const auto& d = ke::editor().patchDialog;
        return d.art == 1 && d.spalten == 0 && d.zeilen == 0 && finde("##patchquadrat", "", 0, true) == nullptr;
    }));
    add(foto("karteneditor_patch_dichte"));
    add(tasteKombi(ImGuiKey_Enter, "Return"));
    add(pruefe("Return: Ebene 3x3 statt des Zylinders", [] {
        const auto p = patchNr(0);
        return patchZahl() == 1 && p && p->breite() == 3 && p->hoehe() == 3 && ke::editor().karte.undo.undoName() == "patchCreatePlane";
    }));
    add(tu("Curve > Exact Cylinder...", [] { (void)ke::ausfuehren("PatchXactCylinder"); }));
    add(pause(0.3));
    add(pruefe("Exact Cylinder: 13 x 3 vorgewaehlt, Square da", [] {
        const auto& d = ke::editor().patchDialog;
        return d.art == 1 && d.spalten == 3 && d.zeilen == 0 && finde(std::string(ke::trK(Str::KePdSquare)) + "##patchquadrat", "", 0, true) != nullptr;
    }));
    add(foto("karteneditor_patch_exakt"));
    add(klickAn("OK", elementOrt(std::string(ke::trK(Str::KePrefOk)) + "##patchok")));
    add(pruefe("Exact Cylinder 13x3", [] {
        const auto p = patchNr(0);
        return p && p->breite() == 13 && p->hoehe() == 3 && ke::editor().karte.undo.undoName() == "patchCreateXactCylinder";
    }));
    return s;
}

karte::Texdef ersteFlaecheSsr() {
    const auto w = ke::editor().karte.welt();
    karte::Texdef t;
    if (!w || w->kinder().empty() || w->kinder()[0]->art() != karte::Primitiv::Art::Pinsel) { return t; }
    const auto p = std::static_pointer_cast<karte::Pinsel>(w->kinder()[0]);
    karte::schiebSkalDrehAusFlaeche(ke::editor().karte.kontext.art(), t, p->flaechen()[0]->texdefHolen(ke::editor().karte.kontext));
    return t;
}

// Punkt 7: Surface Inspector, Textur-Tasten
std::vector<Schritt> texturen() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(grundzustand("Stufe 2/7: Grundzustand"));
    add(pause(0.4));
    add(klickXy("M1 auf den Pinsel", ImGuiMouseButton_Left, ImVec2{32, 32}));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_UpArrow, "Umschalt+Hoch"));
    add(pruefe("Shift+Up: shift t + 8 (Schritt 8)", [] {
        return std::fabs(ersteFlaecheSsr().shift[1] - 8.0F) < 0.01F && ke::editor().karte.undo.undoName() == "brushShiftTexture -x 0 -y 8";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tasteKombi(ImGuiKey_S, "S"));
    add(pause(0.3));
    add(pruefe("S: Surface Inspector offen", [] { return ke::editor().siOffen && finde("##sitextur", "", 0, true) != nullptr; }));
    add(foto("karteneditor_surface_inspector"));
    add(klickAn("Horizontal shift: Pfeil hoch", elementOrt("##sihoch0")));
    add(pruefe("Pfeil hoch: hShift 8 sofort (NonModalSpinner::stepBy)", [] {
        return std::fabs(ersteFlaecheSsr().shift[0] - 8.0F) < 0.01F && ke::editor().karte.undo.undoName() == "textureProjectionSetSelected -hShift 8";
    }));
    add(klickAn("Feld Rotate", elementOrt("##siwert4")));
    add(tippe("30"));
    add(tasteKombi(ImGuiKey_Enter, "Return"));
    add(pruefe("Rotate 30 mit Return", [] {
        return std::fabs(ersteFlaecheSsr().rotate - 30.0F) < 0.01F && ke::editor().karte.undo.undoName() == "textureProjectionSetSelected -rotation 30";
    }));
    add(klickAn("Reset", elementOrt(std::string(ke::trK(Str::KeSiReset)) + "##sireset")));
    add(pruefe("Reset: textureReset/Cap, rotate 0", [] {
        return std::fabs(ersteFlaecheSsr().rotate) < 0.01F && ke::editor().karte.undo.undoName() == "textureReset/Cap";
    }));
    add(klickAn("Fit", elementOrt(std::string(ke::trK(Str::KeSiFit)) + "##sifit")));
    add(pruefe("Fit 1 x 1: textureAutoFit", [] { return ke::editor().karte.undo.undoName() == "textureAutoFit"; }));
    add(klickAn("Axial", elementOrt(std::string(ke::trK(Str::KeSiAxial)) + "##siaxial")));
    add(pruefe("Axial: textureProjectAxial", [] { return ke::editor().karte.undo.undoName() == "textureProjectAxial"; }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_F, "Strg+F"));
    add(pruefe("Strg+F: FitTexture", [] { return ke::editor().karte.undo.undoName() == "textureAutoFit"; }));
    add(klickAn("Match Grid", elementOrt(std::string(ke::trK(Str::KeSiMatchGrid)) + "##simatchgrid")));
    add(pruefe("Match Grid: Schritt = Raster / Skalierung", [] {
        const karte::Texdef t = ersteFlaecheSsr();
        const float soll = static_cast<float>(karte::zuGanzzahl(8.0F / t.scale[0]));
        diag::detail("Knopftest: Stufe 2 - Match Grid " + std::to_string(ke::editor().siWerte.verschiebung[0]) + " soll " + std::to_string(soll));
        return std::fabs(ke::editor().siWerte.verschiebung[0] - soll) < 0.01F;
    }));
    add(tasteKombi(ImGuiKey_S, "S"));
    add(pruefe("S: Surface Inspector zu", [] { return !ke::editor().siOffen; }));
    add(tu("Schritte zurueck auf 8", [] { ke::editor().siWerte = karte::SiWerte{}; }));
    // Tags des Texturbrowsers
    add(tu("View > Tags GUI", [] { (void)ke::ausfuehren("TagsToggleGui"); }));
    add(pause(0.3));
    add(pruefe("Tags GUI an: Reiterleiste da", [] { return ke::editor().tags.gui && finde("##tagreiter1", "", 0, true) != nullptr; }));
    add(klickAn("Reiter Tags", elementOrt("##tagreiter1")));
    add(pruefe("Reiter Tags gewaehlt", [] { return ke::editor().tags.reiter == 1; }));
    add(tu("Tags > Add tag", [] { (void)ke::ausfuehren("TagAdd"); }));
    add(pause(0.2));
    add(tippe("Holz"));
    add(tasteKombi(ImGuiKey_Enter, "Return"));
    add(pruefe("Add tag: NewTag in Holz umbenannt", [] {
        const auto& t = ke::editor().tags;
        return t.alle.count("Holz") != 0U && t.alle.count("NewTag") == 0U && t.bearbeitet.empty();
    }));
    add(tu("Shader common/caulk waehlen", [] { ke::texturShaderWaehlen("textures/common/caulk"); }));
    add(klickAn("Haken vor Holz", [] {
        const Element* el = finde("##tagHolz", "", 0, true);
        return el != nullptr ? ImVec2{el->rect.Min.x + 7.0F, el->rect.GetCenter().y} : ImVec2{0, 0};
    }));
    add(pruefe("Haken: Holz an textures/common/caulk (Eintrag in custom)", [] {
        const auto& t = ke::editor().tags;
        return t.datei.hatTag("textures/common/caulk", "Holz") && t.haken.count("Holz") != 0U;
    }));
    add(tu("Tags > Search tag", [] { (void)ke::ausfuehren("TagSearch"); }));
    add(pruefe("Search tag: nur die gefundenen, Titel [TAGS] Holz", [] {
        const auto& tb = ke::editor().texBro;
        return tb.tagSuche && tb.gefundeneShader.count("textures/common/caulk") != 0U && tb.verzeichnis == "[TAGS] Holz";
    }));
    add(tu("Tags > Delete tag", [] { (void)ke::ausfuehren("TagDelete"); }));
    add(pause(0.3));
    add(pruefe("Delete tag: Rueckfrage offen", [] { return ke::editor().meldung.offen; }));
    add(foto("karteneditor_tags_frage"));
    add(tasteKombi(ImGuiKey_Enter, "Return"));
    add(pruefe("Ja: Holz ueberall geloescht", [] {
        const auto& t = ke::editor().tags;
        return !ke::editor().meldung.offen && t.alle.empty() && !t.datei.hatTag("textures/common/caulk", "Holz");
    }));
    add(tu("View > Tags GUI aus, Show All", [] {
        (void)ke::ausfuehren("TagsToggleGui");
        (void)ke::ausfuehren("ShowAllTextures");
    }));
    add(pruefe("Tags GUI aus", [] { return !ke::editor().tags.gui && !ke::editor().texBro.tagSuche; }));
    // Einstellungsseite Textures
    add(tu("Einstellungen auf", [] { ke::editor().einstellungenOffen = true; }));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KePgTextures)) + "##seite7"));
    add(pause(0.2));
    add(pruefe("Seite Textures: Quality, Gamma, Render Mode, Compression, Anisotropy", [] {
        const bool q = finde("50%##texqual1", "", 0, true) != nullptr;
        const bool g = finde("##texgamma", "", 0, true) != nullptr;
        const bool m = finde("##texmodus", "", 0, true) != nullptr;
        const bool c = finde("##texkompression", "", 0, true) != nullptr;
        diag::detail(std::string("Knopftest: Stufe 2 - Seite Textures ") + (q ? "q" : "-") + (g ? "g" : "-") + (m ? "m" : "-") + (c ? "c" : "-"));
        return q && g;   // die Klapplisten meldet die Elementliste nicht unter ihrer Kennung
    }));
    add(klick("50%##texqual1"));
    add(klick(ke::trK(Str::KePrefOk)));
    add(pause(0.2));
    add(pruefe("OK: Texture Quality 50% (Mip-Stufe 1)", [] { return ke::editor().texQual.mipStufe == 1 && !ke::editor().einstellungenOffen; }));
    add(tu("Texture Quality zurueck auf 100%", [] {
        ke::editor().texQual.mipStufe = 0;
        ke::texturBilderNeuLaden();
        ke::einstellungenSpeichern();
    }));
    return s;
}

}  // namespace stufe2

namespace stufe2 {

bool konsoleHat(const std::string& text) {
    for (const ke::KonsolenZeile& z : ke::editor().konsole) {
        if (z.text.find(text) != std::string::npos) { return true; }
    }
    return false;
}

std::shared_ptr<karte::Entitaet> lichtEntitaet() {
    for (const auto& e : ke::editor().karte.entitaeten()) {
        if (e->klassenname() == "light") { return e; }
    }
    return nullptr;
}

// Ein grosser Pinsel (-200..200)^3 dazu, allein gewaehlt
Schritt grosserDazu(const std::string& was) {
    return tu(was, [] {
        ke::Editor& e = ke::editor();
        e.karte.undo.start();
        karte::TexturProjektion proj;
        karte::texdefStandard(karte::TexdefArt::Quake, proj);
        auto p = std::make_shared<karte::Pinsel>();
        e.karte.welt()->kindEinfuegen(p);
        p->quaderBauen(karte::Kasten(karte::Vektor3f(0, 0, 0), karte::Vektor3f(200, 200, 200)), "textures/common/caulk", proj, e.karte.kontext);
        e.karte.undo.finish("brushDragNew");
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), p);
        ke::auswahlGeaendert();
    });
}

Schritt nurPinselWaehlen(const std::string& was, std::size_t i) {
    return tu(was, [i] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), e.karte.welt()->kinder()[i]);
        ke::auswahlGeaendert();
    });
}

bool pinselSichtbar(std::size_t i) { return ke::editor().karte.welt()->kinder()[i]->sichtbar(); }

std::string laufDatei(const char* name) { return (std::filesystem::current_path() / name).string(); }

bool dateiDa(const std::string& p) {
    std::error_code ec;
    return std::filesystem::exists(std::filesystem::path(p), ec);
}

std::string dateiText(const std::string& p) {
    std::ifstream f(std::filesystem::path(p), std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

// Punkt 8: Auswahl und Datei (Select inside/touching, Strg+E, Region, Find
// brush, Map Info, Import, Save selected/region, Autosave)
std::vector<Schritt> auswahlDatei() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(grundzustand("Stufe 2/8: Grundzustand"));
    add(pause(0.4));
    add(schneiderDazu("zweiter Pinsel (32 -16 16)..(96 16 48), allein gewaehlt", false));
    add(tu("Edit > Select touching", [] { (void)ke::ausfuehren("SelectTouching"); }));
    add(pruefe("Select touching: beide Pinsel, das Licht nicht", [] {
        const ke::Editor& e = ke::editor();
        return e.karte.auswahl.anzahl() == 2 && !e.karte.auswahl.gewaehlt(lichtEntitaet().get());
    }));
    add(grosserDazu("grosser Pinsel (-200..200), allein gewaehlt"));
    add(tu("Edit > Select inside", [] { (void)ke::ausfuehren("SelectInside"); }));
    add(pruefe("Select inside: der grosse geloescht, beide Pinsel und das Licht gewaehlt", [] {
        const ke::Editor& e = ke::editor();
        return pinselZahl() == 2 && e.karte.auswahl.anzahl() == 3 && e.karte.auswahl.gewaehlt(lichtEntitaet().get()) &&
               e.karte.undo.undoName() == "deleteSelected";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pruefe("Strg+Z: der grosse wieder da", [] { return pinselZahl() == 3; }));
    add(nurPinselWaehlen("nur den Testpinsel waehlen", 0));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_E, "Strg+E"));
    add(pruefe("Strg+E: alle drei Weltpinsel, das Licht nicht", [] {
        const ke::Editor& e = ke::editor();
        return e.karte.auswahl.anzahl() == 3 && !e.karte.auswahl.gewaehlt(lichtEntitaet().get());
    }));
    // Region
    add(nurPinselWaehlen("nur den zweiten Pinsel waehlen", 1));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_R, "Strg+Umschalt+R"));
    add(pruefe("Region Set Selection: angehakt, nur der zweite sichtbar, Auswahl leer", [] {
        const ke::Editor& e = ke::editor();
        return ke::angehakt("RegionSetSelection") && e.region.aktiv && !pinselSichtbar(0) && pinselSichtbar(1) && !pinselSichtbar(2) &&
               !lichtEntitaet()->sichtbar() && e.karte.auswahl.anzahl() == 0 && std::fabs(e.region.min[0] - 32.0F) < 0.01F;
    }));
    add(pause(0.3));
    add(foto("karteneditor_region"));
    add(tu("File > Save region...", [] { (void)ke::regionSpeichernUnter(laufDatei("stufe2_region.map")); }));
    add(pruefe("Save region: der zweite, sechs Waende, info_player_start; das Licht fehlt", [] {
        const std::string t = dateiText(laufDatei("stufe2_region.map"));
        karte::Karte k;
        (void)karte::karteLesen(karte::textmodusLesen(t), k);
        const auto w = k.welt();
        return w && w->kinder().size() == 7 && k.entitaeten().size() == 2 && k.entitaeten().back()->klassenname() == "info_player_start" &&
               t.find("\r\n") != std::string::npos && konsoleHat("for write...success");
    }));
    add(tu("Region > Off", [] { (void)ke::ausfuehren("RegionOff"); }));
    add(pruefe("Region Off: alles sichtbar", [] {
        return !ke::editor().region.aktiv && pinselSichtbar(0) && pinselSichtbar(2) && lichtEntitaet()->sichtbar() && !ke::angehakt("RegionSetSelection");
    }));
    add(tu("XY: Massstab 1 um (0 0 0), Region > Set XY", [] {
        ke::Editor& e = ke::editor();
        e.aktivXy = 0;
        e.xy[0].ansicht.setzeMassstab(1);
        e.xy[0].ansicht.setzeUrsprung(karte::Vektor3f(0, 0, 0));
        (void)ke::ausfuehren("RegionSetXY");
    }));
    add(pruefe("Set XY: Bereich = die sichtbare Flaeche der XY-Ansicht, Z -65472..65472", [] {
        const ke::Editor& e = ke::editor();
        const karte::XyAnsicht& v = e.xy[0].ansicht;
        const float hb = 0.5F * static_cast<float>(v.breite);
        diag::detail("Knopftest: Stufe 2 - Set XY " + std::to_string(e.region.min[0]) + " .. " + std::to_string(e.region.max[0]));
        return e.region.aktiv && std::fabs(e.region.min[0] + hb) < 0.01F && std::fabs(e.region.max[0] - hb) < 0.01F &&
               std::fabs(e.region.min[2] + 65472.0F) < 0.01F && pinselSichtbar(0) && lichtEntitaet()->sichtbar();
    }));
    add(tu("Region > Off", [] { (void)ke::ausfuehren("RegionOff"); }));
    add(nurPinselWaehlen("nur den zweiten Pinsel waehlen", 1));
    add(tu("Region > Set Brush", [] { (void)ke::ausfuehren("RegionSetBrush"); }));
    add(pruefe("Set Brush: der zweite bestimmt den Bereich und ist geloescht, das Licht ausgeschlossen", [] {
        const ke::Editor& e = ke::editor();
        return e.region.aktiv && pinselZahl() == 2 && e.karte.undo.undoName() == "deleteSelected" && pinselSichtbar(0) &&
               !lichtEntitaet()->sichtbar() && std::fabs(e.region.max[0] - 96.0F) < 0.01F;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tu("Region > Off", [] { (void)ke::ausfuehren("RegionOff"); }));
    add(tu("Region Set Selection ohne Auswahl", [] {
        ke::editor().karte.auswahl.leeren();
        (void)ke::ausfuehren("RegionSetSelection");
    }));
    add(pruefe("ohne Auswahl: Region Off", [] { return !ke::editor().region.aktiv && pinselSichtbar(0); }));
    // Find brush
    add(tu("Misc > Find brush...", [] { (void)ke::ausfuehren("FindBrush"); }));
    add(pause(0.3));
    add(pruefe("Find Brush: offen mit 0 / 0", [] {
        const auto& d = ke::editor().pinselSuche;
        return d.offen && d.entitaet == 0 && d.pinsel == 0 && finde("##suchpinsel", "", 0, true) != nullptr;
    }));
    add(foto("karteneditor_find_brush"));
    // Stufe 3 Punkt 0: Textbreiten wie Qt (DirectWrite in Entwurfsmassen). Am
    // Foto dlg_Misc_Find_brush_0.png (2x): die Felder beginnen bei 86 =
    // 11 + 69 ("Entity number" in Tahoma 11 nach Qt) + 6, Innenraum 263 breit.
    auto nrcVorher = std::make_shared<bool>(false);
    add(tu("Find Brush: Begriffe wie NetRadiant", [nrcVorher] {
        *nrcVorher = ke::nrcBegriffe();
        ke::setzeNrcBegriffe(true);
    }));
    add(pause(0.2));
    add(pruefe("Find Brush: Felder bei 86, Innenraum 263 breit wie das NRC-Foto", [] {
        const ImGuiWindow* w = ImGui::FindWindowByName("###kartepinselsuche");
        const Element* el = finde("##suchpinsel", "", 0, true);
        if (w == nullptr || el == nullptr) { return false; }
        const float sk = ke::massstab();
        const long x = std::lround((el->rect.Min.x - w->Pos.x) / sk);
        const long b = std::lround(w->Size.x / sk);
        diag::detail("Knopftest: Find Brush - Feld ab " + std::to_string(x) + ", Innenraum " + std::to_string(b) + " breit");
        return x == 86 && b == 263;
    }));
    add(tu("Find Brush: Begriffe zurueck", [nrcVorher] { ke::setzeNrcBegriffe(*nrcVorher); }));
    add(klickAn("Feld Brush number", elementOrt("##suchpinsel")));
    add(tippe("1"));
    add(tasteKombi(ImGuiKey_Enter, "Return"));
    add(pause(0.2));
    add(pruefe("Find: Pinsel 1 (der zweite) gewaehlt, die 2D-Ansichten auf seine Mitte (64 0 32)", [] {
        const ke::Editor& e = ke::editor();
        return !e.pinselSuche.offen && e.karte.auswahl.anzahl() == 1 && e.karte.auswahl.gewaehlt(e.karte.welt().get(), e.karte.welt()->kinder()[1].get()) &&
               std::fabs(e.xy[0].ansicht.ursprung[0] - 64.0F) < 0.01F && std::fabs(e.xy[1].ansicht.ursprung[2] - 32.0F) < 0.01F;
    }));
    add(tu("Misc > Find brush... nochmal", [] { (void)ke::ausfuehren("FindBrush"); }));
    add(pause(0.3));
    add(pruefe("Find Brush: die Auswahl vorbelegt (Entity 0, Brush 1)", [] {
        const auto& d = ke::editor().pinselSuche;
        return d.offen && d.entitaet == 0 && d.pinsel == 1;
    }));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(pruefe("Esc: zu", [] { return !ke::editor().pinselSuche.offen; }));
    // Map Info
    add(tasteKombi(ImGuiKey_M, "M"));
    add(pause(0.3));
    add(pruefe("M: Map Info mit 3 Pinseln, 0 Patches, 2 Entitaeten, ingame 1", [] {
        const auto& d = ke::editor().kartenInfo;
        return d.offen && d.info.pinsel == 3 && d.info.patches == 0 && d.info.entitaeten == 2 && d.info.imSpiel == 1 && d.info.gruppen == 1 &&
               d.info.aufschluesselung.size() == 2;
    }));
    add(foto("karteneditor_map_info"));
    add(klickAn("Kopf Count", elementOrt("##infokopf1")));
    add(pruefe("Kopf Count: nach Anzahl sortiert", [] { return ke::editor().kartenInfo.sortSpalte == 1 && !ke::editor().kartenInfo.absteigend; }));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(pruefe("Esc: Map Info zu", [] { return !ke::editor().kartenInfo.offen; }));
    // Save selected und Import
    add(tu("das Licht waehlen, File > Save selected...", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(lichtEntitaet());
        (void)ke::auswahlSpeichernUnter(laufDatei("stufe2_auswahl.map"));
    }));
    add(pruefe("Save selected: nur das Licht (IncludeSelectedWalker: worldspawn ohne gewaehlten Pinsel nicht)", [] {
        karte::Karte k;
        (void)karte::karteLesen(karte::textmodusLesen(dateiText(laufDatei("stufe2_auswahl.map"))), k);
        return k.entitaeten().size() == 1 && k.entitaeten().front()->klassenname() == "light";
    }));
    add(tu("File > Import...", [] { (void)ke::karteImportierenAus(laufDatei("stufe2_auswahl.map")); }));
    add(pruefe("Import: ein zweites Licht, gewaehlt mit dem ersten, \"mapImport\"", [] {
        const ke::Editor& e = ke::editor();
        return e.karte.entitaeten().size() == 3 && e.karte.auswahl.anzahl() == 2 && e.karte.undo.undoName() == "mapImport" &&
               konsoleHat("for read...success");
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pruefe("Strg+Z: wieder ein Licht", [] { return ke::editor().karte.entitaeten().size() == 2; }));
    // Autosave: Einstellungsseite und die Uhr
    add(tu("Einstellungen auf", [] { ke::editor().einstellungenOffen = true; }));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KePgAutosave)) + "##seite22"));
    add(pause(0.2));
    add(pruefe("Seite Autosave: Enable, Frequency, Snapshots", [] {
        return finde(std::string(ke::trK(Str::KePrAutosaveEnable)) + "##autosavean", "", 0, true) != nullptr &&
               finde("##autosaveminuten", "", 0, true) != nullptr;
    }));
    add(foto("karteneditor_pref_autosave"));
    add(klick(std::string(ke::trK(Str::KePrAutosaveSnapshots)) + "##schnappschuesse"));
    add(klick(ke::trK(Str::KePrefOk)));
    add(pause(0.2));
    add(pruefe("OK: Save Snapshots an", [] { return ke::editor().autosave.schnappschuesse && !ke::editor().einstellungenOffen; }));
    add(tu("Schnappschuesse wieder aus, Uhr starten (eine Aenderung)", [] {
        ke::Editor& e = ke::editor();
        e.autosave.schnappschuesse = false;
        ke::einstellungenSpeichern();
        e.autosave.leeren();
        e.autosaveVersatz = 0;
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(laufDatei("karte_kopie.autosave.map")), ec);
        std::filesystem::remove_all(std::filesystem::path(laufDatei("snapshots")), ec);
        (void)ke::ausfuehren("SelectNudgeRight");
    }));
    add(pause(1.3));
    add(tu("die Uhr 16 Minuten vor", [] { ke::editor().autosaveVersatz = 16 * 60; }));
    add(pause(1.3));
    add(pruefe("Autosave: \"Autosaving...\" und karte_kopie.autosave.map", [] {
        return konsoleHat("Autosaving...") && dateiDa(laufDatei("karte_kopie.autosave.map"));
    }));
    add(tu("Snapshots an, noch eine Aenderung", [] {
        ke::editor().autosave.schnappschuesse = true;
        (void)ke::ausfuehren("SelectNudgeRight");
    }));
    add(pause(1.3));
    add(tu("die Uhr 32 Minuten vor", [] { ke::editor().autosaveVersatz = 32 * 60; }));
    add(pause(1.3));
    add(pruefe("Snapshot: \"Autosaving snapshot...\" und snapshots/karte_kopie.0.map", [] {
        return konsoleHat("Autosaving snapshot...") && dateiDa(laufDatei("snapshots/karte_kopie.0.map"));
    }));
    add(tu("Autosave aus, noch eine Aenderung", [] {
        ke::editor().autosave.an = false;
        ke::editor().autosave.schnappschuesse = false;
        (void)ke::ausfuehren("SelectNudgeRight");
    }));
    add(pause(1.3));
    add(tu("die Uhr 48 Minuten vor", [] { ke::editor().autosaveVersatz = 48 * 60; }));
    add(pause(1.3));
    add(pruefe("Autosave aus: \"Autosave skipped...\"", [] { return konsoleHat("Autosave skipped..."); }));
    add(tu("Autosave zurueck", [] {
        ke::Editor& e = ke::editor();
        e.autosave = karte::AutoSpeichern{};
        e.autosaveVersatz = 0;
        ke::einstellungenSpeichern();
    }));
    return s;
}

bool alleFlaechen(const char* shader) {
    const auto p = pinsel();
    if (!p) { return false; }
    for (const auto& f : p->flaechen()) {
        if (f->shader() != shader) { return false; }
    }
    return true;
}

// Punkt 9: Filter (Menue, Tasten, Filterleiste mit Rechtsklick, Invert/Reset)
std::vector<Schritt> filter() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(grundzustand("Stufe 2/9: Grundzustand"));
    add(tu("Filter zuruecksetzen", [] { (void)ke::ausfuehren("ResetFilters"); }));
    add(pause(0.4));
    add(tasteKombi(ImGuiMod_Alt | ImGuiKey_6, "Alt+6"));
    add(pause(0.2));
    add(pruefe("Alt+6: Caulk-Filter an, der Caulk-Pinsel unsichtbar, Licht sichtbar", [] {
        return ke::angehakt("FilterCaulk") && !pinsel()->sichtbar() && pinsel()->gefiltert && lichtEntitaet()->sichtbar();
    }));
    add(foto("karteneditor_filter_caulk"));
    add(klickXy("M1 auf den gefilterten Pinsel", ImGuiMouseButton_Left, ImVec2{32, 32}));
    add(pruefe("gefiltert: nicht waehlbar", [] { return ke::editor().karte.auswahl.anzahl() == 0; }));
    add(tasteKombi(ImGuiMod_Alt | ImGuiKey_6, "Alt+6"));
    add(pruefe("Alt+6: wieder sichtbar", [] { return !ke::angehakt("FilterCaulk") && pinsel()->sichtbar(); }));
    add(tu("Filter > Lights", [] { (void)ke::ausfuehren("FilterLights"); }));
    add(pruefe("Lights: das Licht gefiltert", [] { return ke::angehakt("FilterLights") && !lichtEntitaet()->sichtbar() && pinsel()->sichtbar(); }));
    add(tu("Filter > Invert filters", [] { (void)ke::ausfuehren("InvertFilters"); }));
    add(pruefe("Invert: Lights aus, alle anderen an - World und Entities filtern alles", [] {
        const ke::Editor& e = ke::editor();
        return !ke::angehakt("FilterLights") && ke::angehakt("FilterWorldBrushes") && ke::angehakt("FilterSky") && !pinsel()->sichtbar() &&
               !lichtEntitaet()->sichtbar() && e.filterMaske == (karte::filterUmkehren(0) & ~static_cast<unsigned int>(karte::EXCLUDE_LIGHTS));
    }));
    // ResetFilters steht bei dieser Fensterhoehe (wie in NRC) hinter dem Erweiterungsknopf
    add(tu("Filter > Reset filters", [] { (void)ke::ausfuehren("ResetFilters"); }));
    add(pruefe("ResetFilters: nichts gefiltert", [] { return ke::editor().filterMaske == 0 && pinsel()->sichtbar() && lichtEntitaet()->sichtbar(); }));
    add(klickAn("Filterleiste: Knopf Patches", elementOrt("##knopfFilterPatches")));
    add(pruefe("Knopf Patches: FilterPatches an (SI_Exclude gespeichert)", [] {
        const auto it = g_app->settings.karte.find("SI_Exclude");
        return ke::angehakt("FilterPatches") && it != g_app->settings.karte.end() && it->second == std::to_string(karte::EXCLUDE_CURVES);
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_P, "Strg+P"));
    add(pruefe("Strg+P: FilterPatches aus", [] { return !ke::angehakt("FilterPatches"); }));
    // Rechtsklick-Handlungen der Filterleiste
    add(klickXy("M1 auf den Pinsel", ImGuiMouseButton_Left, ImVec2{32, 32}));
    add(klickAn("Rechtsklick Translucent", elementOrt("##knopfFilterTranslucent"), 1));
    // Stufe 6 (NRC#145): GetCommonShader mit den Namen aus system.shader von JA/JK2 - "nodraw" ist dort
    // system/nodraw_solid (fest), "nodrawnonsolid" ist system/nodraw (NRC: system/nodraw, system/nodrawnonsolid)
    add(pruefe("Rechtsklick Translucent: system/nodraw_solid, \"textureNameSetSelected\"", [] {
        return alleFlaechen("textures/system/nodraw_solid") && ke::editor().karte.undo.undoName() == "textureNameSetSelected";
    }));
    add(klickAn("Rechtsklick Translucent nochmal", elementOrt("##knopfFilterTranslucent"), 1));
    add(pruefe("zweiter Rechtsklick: system/nodraw (nicht fest)", [] { return alleFlaechen("textures/system/nodraw"); }));
    add(klickAn("Rechtsklick Translucent ein drittes Mal", elementOrt("##knopfFilterTranslucent"), 1));
    add(pruefe("dritter Rechtsklick: wieder system/nodraw_solid", [] { return alleFlaechen("textures/system/nodraw_solid"); }));
    add(klickAn("Rechtsklick Translucent ein viertes Mal", elementOrt("##knopfFilterTranslucent"), 1));
    add(tu("Maus aus der Filterleiste", [] { setzeMaus(weltXy(0, 0)); }));
    add(pause(0.3));
    add(klickAn("Rechtsklick Translucent nach dem Betreten", elementOrt("##knopfFilterTranslucent"), 1));
    add(pruefe("nach QEvent::Enter: von vorn (system/nodraw_solid)", [] { return alleFlaechen("textures/system/nodraw_solid"); }));
    add(klickAn("Rechtsklick Caulk", elementOrt("##knopfFilterCaulk"), 1));
    add(pruefe("Rechtsklick Caulk: system/caulk", [] { return alleFlaechen("textures/system/caulk"); }));
    add(klickAn("Rechtsklick Details", elementOrt("##knopfFilterDetails"), 1));
    add(pruefe("Rechtsklick Details: MakeDetail", [] { return pinsel()->istDetail(); }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_D, "Strg+D"));
    add(pruefe("Strg+D: der Detail-Pinsel gefiltert", [] { return ke::angehakt("FilterDetails") && !pinsel()->sichtbar(); }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_D, "Strg+D"));
    add(klickAn("Rechtsklick Structural", elementOrt("##knopfFilterStructural"), 1));
    add(pruefe("Rechtsklick Structural: MakeStructural", [] { return !pinsel()->istDetail(); }));
    add(klickAn("Rechtsklick Func_Groups", elementOrt("##knopfFilterFuncGroups"), 1));
    add(pruefe("Rechtsklick Func_Groups: func_group aus der Auswahl", [] {
        const ke::Editor& e = ke::editor();
        bool gruppe = false;
        for (const auto& x : e.karte.entitaeten()) { gruppe = gruppe || (x->klassenname() == "func_group" && x->kinder().size() == 1); }
        return gruppe && e.karte.undo.undoName() == "entityCreate -class func_group";
    }));
    add(tu("Filter > FuncGroups", [] { (void)ke::ausfuehren("FilterFuncGroups"); }));
    add(pruefe("FuncGroups: die Gruppe (mit dem Pinsel) gefiltert", [] {
        for (const auto& x : ke::editor().karte.entitaeten()) {
            if (x->klassenname() == "func_group") { return !x->sichtbar(); }
        }
        return false;
    }));
    add(tu("Filter zuruecksetzen", [] { (void)ke::ausfuehren("ResetFilters"); }));
    return s;
}

// Eine Spalte einer Zeile des Ebenen-Browsers: von rechts (select 42, visible 14)
std::function<ImVec2()> ebenenSpalte(int ebene, float vonRechts) {
    return [ebene, vonRechts]() {
        const Element* el = finde("##ebene" + std::to_string(ebene), "", 0, true);
        if (el == nullptr) { return ImVec2{0, 0}; }
        return ImVec2{el->rect.Max.x - std::round(vonRechts * ke::massstab()), el->rect.GetCenter().y};
    };
}

// Punkt 10a: Ebenen (Browser, Kontextmenue, Strg+Q, Umschalt+Q, Ziehen und Ablegen)
std::vector<Schritt> ebenen() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(grundzustand("Stufe 2/10a: Grundzustand"));
    add(tu("View > Layers Browser", [] {
        ke::editor().gruppenSeite = 0;
        (void)ke::ausfuehren("ToggleLayersBrowser");
    }));
    add(pause(0.3));
    // 444 x 777 (GroupDlg/geometry) passt bei 1472 Bildpunkten Hoehe nicht ganz auf den Schirm
    add(tu("Gruppendialog 444 x 600 rechts (die XY-Ansicht bleibt frei)", [] {
        const float s = ke::massstab();
        ImGui::SetWindowPos("###kartegruppe", ImVec2{ImGui::GetMainViewport()->Size.x - 520.0F * s, 40.0F * s});
        ImGui::SetWindowSize("###kartegruppe", ImVec2{444.0F * s, 600.0F * s});
    }));
    add(pause(0.3));
    add(pruefe("Seite Layers: eine Zeile \"0\", aktuell", [] {
        return ke::editor().gruppenDialogOffen && ke::editor().gruppenSeite == 4 && finde("##ebene0", "", 0, true) != nullptr &&
               ke::editor().karte.schichten.size() == 1;
    }));
    add(klickAn("New Layer", elementOrt("##ebenenknopf0")));
    add(pause(0.3));
    add(pruefe("Layer Name: Dialog mit UnnamedpLayer", [] { return ke::editor().eingabe.offen && std::string(ke::editor().eingabe.puffer) == "UnnamedpLayer"; }));
    add(foto("karteneditor_ebene_name"));
    add(tippe("Wald\"1"));
    add(tasteKombi(ImGuiKey_Enter, "Return"));
    add(pause(0.2));
    add(pruefe("neue Ebene Wald1 (ohne '\"'), aktuell", [] {
        const auto& k = ke::editor().karte;
        return !ke::editor().eingabe.offen && k.schichten.size() == 2 && k.schichten[1].name == "Wald1" && k.aktuelleSchicht == 1;
    }));
    add(foto("karteneditor_ebenen"));
    add(klickXy("M1 auf den Pinsel", ImGuiMouseButton_Left, ImVec2{32, 32}));
    add(klickAn("Rechtsklick auf Wald1", elementOrt("##ebene1"), 1));
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KeLayMoveSel)) + "##ebenezuordnen"));
    add(pruefe("Move Selection to This Layer: der Pinsel auf Wald1", [] { return pinsel()->schicht == 1; }));
    add(klickAn("Auge von Wald1", ebenenSpalte(1, 14.0F)));
    add(pruefe("Auge: Wald1 versteckt, der Pinsel unsichtbar und abgewaehlt", [] {
        return !pinsel()->sichtbar() && pinsel()->ebeneVersteckt && ke::editor().karte.auswahl.anzahl() == 0 && ke::editor().ebenenBro.sichtbar[1] == 0;
    }));
    add(klickAn("Auge von Wald1 nochmal", ebenenSpalte(1, 14.0F)));
    add(pruefe("Auge: wieder sichtbar", [] { return pinsel()->sichtbar(); }));
    add(klickAn("Auswahlsymbol von 0", ebenenSpalte(0, 42.0F)));
    add(pruefe("Select 0: nur das Licht", [] {
        const auto& k = ke::editor().karte;
        return k.auswahl.anzahl() == 1 && k.auswahl.gewaehlt(lichtEntitaet().get());
    }));
    add(ziehenAn("Umschalt+Klick auf das Auswahlsymbol von Wald1", ebenenSpalte(1, 42.0F), 0.0F, 0.0F, 0, ImGuiMod_Shift));
    add(pruefe("Umschalt: das Licht auf Wald1", [] { return lichtEntitaet()->schicht == 1; }));
    add(tu("Auswahl: nur der Pinsel", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), e.karte.welt()->kinder().front());
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Q, "Strg+Q"));
    add(pruefe("Strg+Q: alles auf Wald1 (Pinsel und Licht)", [] { return ke::editor().karte.auswahl.anzahl() == 2; }));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_Q, "Umschalt+Q"));
    add(pause(0.2));
    add(pruefe("Umschalt+Q: Menue mit -> 0 und -> Wald1", [] {
        return ke::editor().ebenenBro.menueOffen && finde("-> 0##ebenezu0", "", 0, true) != nullptr;
    }));
    add(foto("karteneditor_ebenen_menue"));
    add(klick("-> 0##ebenezu0"));
    add(pruefe("-> 0: beide auf 0", [] { return pinsel()->schicht == 0 && lichtEntitaet()->schicht == 0; }));
    // Ziehen und Ablegen: eine zweite Ebene unter Wald1
    add(tu("New Layer B (direkt)", [] {
        (void)karte::ebeneNeu(ke::editor().karte, -1, "B");
    }));
    add(pause(0.2));
    add(ziehenAn("Zeile B auf die Mitte von Wald1", elementOrt("##ebene2"), 0.0F, -std::round(22.0F * ke::massstab()), 0));
    add(pause(0.2));
    add(pruefe("abgelegt: B ist Kind von Wald1", [] {
        const auto& k = ke::editor().karte;
        return k.schichten.size() == 3 && k.schichten[2].name == "B" && k.schichten[2].eltern == 1;
    }));
    add(tu("Pinsel auf B", [] { pinsel()->schicht = 2; }));
    add(klickAn("Rechtsklick auf Wald1", elementOrt("##ebene1"), 1));
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KeLayDelete)) + "##ebeneloeschen"));
    add(pruefe("Delete Wald1: B mit, der Pinsel auf 0 (B war aktuell -> erste oberste)", [] {
        const auto& k = ke::editor().karte;
        return k.schichten.size() == 1 && pinsel()->schicht == 0 && k.aktuelleSchicht == 0;
    }));
    add(tu("Gruppendialog zu, Seite Entities", [] {
        ke::editor().gruppenDialogOffen = false;
        ke::editor().gruppenSeite = 0;
    }));
    return s;
}

std::shared_ptr<karte::Entitaet> modellEntitaet(const char* modell) {
    for (const auto& e : ke::editor().karte.entitaeten()) {
        if (e->wert("model") == modell) { return e; }
    }
    return nullptr;
}

Schritt modellDazu(const std::string& was, const char* modell, const char* ursprung) {
    return tu(was, [modell, ursprung] {
        ke::Editor& e = ke::editor();
        e.karte.undo.start();
        auto ent = e.karte.neueEntitaet("misc_model", false);
        ent->setzen("origin", ursprung);
        ent->setzen("model", modell);
        e.karte.entitaetEinfuegen(ent);
        e.karte.undo.finish("entityCreate -class misc_model");
        e.karte.auswahl.leeren();
        ke::auswahlGeaendert();
    });
}

// Punkt 10b: misc_model mit Modell (md3, glm), Grenzen, Wahl, Filter, Meldungen
std::vector<Schritt> modelle() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(grundzustand("Stufe 2/10b: Grundzustand"));
    add(pause(0.4));
    add(modellDazu("misc_model bespin/panels.md3 bei (200 0 32)", "models/map_objects/bespin/panels.md3", "200 0 32"));
    add(pause(0.3));
    add(pruefe("md3 geladen: \"Loaded Model:\", Flaechen, kein NullModel", [] {
        const auto ent = modellEntitaet("models/map_objects/bespin/panels.md3");
        if (!ent) { return false; }
        const karte::ModellAnsicht a = ke::kartenModelle().ansicht(*ent);
        diag::detail("Knopftest: Stufe 2 - panels.md3 Flaechen " + std::to_string(a.modell ? a.modell->flaechen.size() : 0));
        return a.modell && !a.modell->null && !a.modell->flaechen.empty() && konsoleHat("Loaded Model: \"models/map_objects/bespin/panels.md3\"");
    }));
    add(pruefe("Grenzen: worldAABB des Modells (nicht der Klassenkasten)", [] {
        const auto ent = modellEntitaet("models/map_objects/bespin/panels.md3");
        const karte::Kasten b = karte::instanzGrenzen(*ent, ke::editor().karte.kontext);
        const karte::Kasten m = karte::modellWeltKasten(ke::kartenModelle().ansicht(*ent), karte::miscModellMatrix(*ent));
        return karte::kastenGueltig(b) && kastenGleich(b, m.origin - m.extents, m.origin + m.extents);
    }));
    add(klickAn("M1 auf ein Dreieck des Modells (XY)", [] {
        const auto ent = modellEntitaet("models/map_objects/bespin/panels.md3");
        const karte::ModellAnsicht a = ke::kartenModelle().ansicht(*ent);
        const karte::Matrix4 m = karte::miscModellMatrix(*ent);
        // das groesste Dreieck in der XY-Ebene, seine Mitte
        float best = -1;
        karte::Vektor3f mitte;
        for (const auto& f : a.modell->flaechen) {
            for (std::size_t i = 0; i + 2 < f.indizes.size(); i += 3) {
                const karte::Vektor3f p0 = karte::transformierterPunkt(m, f.punkte[f.indizes[i]].ort);
                const karte::Vektor3f p1 = karte::transformierterPunkt(m, f.punkte[f.indizes[i + 1]].ort);
                const karte::Vektor3f p2 = karte::transformierterPunkt(m, f.punkte[f.indizes[i + 2]].ort);
                const float fl = std::fabs((p1[0] - p0[0]) * (p2[1] - p0[1]) - (p2[0] - p0[0]) * (p1[1] - p0[1]));
                if (fl > best) {
                    best = fl;
                    mitte = (p0 + p1 + p2) * (1.0F / 3.0F);
                }
            }
        }
        return weltXy(mitte[0], mitte[1]);
    }));
    add(pruefe("das Modell ist waehlbar: misc_model gewaehlt", [] {
        const auto ent = modellEntitaet("models/map_objects/bespin/panels.md3");
        return ke::editor().karte.auswahl.anzahl() == 1 && ke::editor().karte.auswahl.gewaehlt(ent.get());
    }));
    add(pause(0.3));
    add(foto("karteneditor_misc_model"));
    add(tu("modelscale 2", [] {
        const auto ent = modellEntitaet("models/map_objects/bespin/panels.md3");
        ke::editor().karte.undo.start();
        ent->setzen("modelscale", "2");
        ke::editor().karte.undo.finish("entitySetKeyValue -key modelscale -value 2");
    }));
    add(pruefe("modelscale 2: doppelte Grenzen", [] {
        const auto ent = modellEntitaet("models/map_objects/bespin/panels.md3");
        const karte::ModellAnsicht a = ke::kartenModelle().ansicht(*ent);
        const karte::Kasten b = karte::instanzGrenzen(*ent, ke::editor().karte.kontext);
        return std::fabs(b.extents[2] - 2.0F * a.modell->grenzen.extents[2]) < 0.01F;
    }));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_M, "Umschalt+M"));
    add(pruefe("Umschalt+M: Models-Filter - das misc_model unsichtbar", [] {
        return ke::angehakt("FilterModels") && !modellEntitaet("models/map_objects/bespin/panels.md3")->sichtbar();
    }));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_M, "Umschalt+M"));
    add(modellDazu("misc_model hoth/eweb_model.glm (Grundhaltung, model_default.skin)", "models/map_objects/hoth/eweb_model.glm", "-200 0 32"));
    add(pruefe("glm geladen: Flaechen mit Shadern", [] {
        const auto ent = modellEntitaet("models/map_objects/hoth/eweb_model.glm");
        const karte::ModellAnsicht a = ke::kartenModelle().ansicht(*ent);
        bool shader = false;
        for (const auto& n : a.shader) { shader = shader || !n.empty(); }
        diag::detail("Knopftest: Stufe 2 - eweb_model.glm Flaechen " + std::to_string(a.modell ? a.modell->flaechen.size() : 0));
        return a.modell && !a.modell->null && !a.modell->flaechen.empty() && shader;
    }));
    add(modellDazu("misc_model ohne Datei", "models/gibtsnicht.md3", "0 200 32"));
    add(modellDazu("misc_model mit unbekannter Endung", "models/x.obj", "0 -200 32"));
    add(pause(0.3));
    add(pruefe("Meldungen \"Model load failed\" / \"Model type not supported\", NullModel (8)", [] {
        const auto ent = modellEntitaet("models/gibtsnicht.md3");
        const karte::ModellAnsicht a = ke::kartenModelle().ansicht(*ent);
        return konsoleHat("Model load failed: \"models/gibtsnicht.md3\"") && konsoleHat("Model type not supported: \"models/x.obj\"") && a.modell->null &&
               std::fabs(a.modell->grenzen.extents[0] - 8.0F) < 0.01F;
    }));
    add(tu("Misc > Refresh models", [] {
        ke::konsoleLeeren();
        (void)ke::ausfuehren("RefreshReferences");
    }));
    add(pause(0.3));
    add(pruefe("Refresh models: neu geladen (\"Loaded Model:\" erneut)", [] { return konsoleHat("Loaded Model: \"models/map_objects/bespin/panels.md3\""); }));
    return s;
}

// Die Mitte der Zelle i der Modellansicht (Geraetepunkte ab der linken oberen Ecke)
std::function<ImVec2()> modellZelle(int i) {
    return [i]() {
        const Element* el = finde("##modbroansicht", "", 0, true);
        if (el == nullptr) { return ImVec2{0, 0}; }
        const int zelle = ke::editor().modBroE.zelle;
        const int schrift = static_cast<int>(std::lround(ImGui::GetFontSize()));
        const karte::ModellZellen z(static_cast<int>(el->rect.GetWidth()), zelle, schrift, static_cast<int>(std::lround(ImGui::GetFontSize() * 0.2F)));
        const karte::Vektor3f o = z.ursprung(i);
        return ImVec2{el->rect.Min.x + o[0], el->rect.Min.y - o[2]};
    };
}

// Punkt 10c: der Modell-Browser (Baum, Ordner laden, Zuweisen, Einfuegen, Seite)
std::vector<Schritt> modellBrowser() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(grundzustand("Stufe 2/10c: Grundzustand"));
    add(tu("View > Model Browser", [] {
        ke::editor().gruppenSeite = 0;
        (void)ke::ausfuehren("ToggleModelBrowser");
    }));
    add(pause(0.3));
    add(tu("Gruppendialog 444 x 600 rechts", [] {
        const float s = ke::massstab();
        ImGui::SetWindowPos("###kartegruppe", ImVec2{ImGui::GetMainViewport()->Size.x - 520.0F * s, 40.0F * s});
        ImGui::SetWindowSize("###kartegruppe", ImVec2{444.0F * s, 600.0F * s});
    }));
    add(pause(0.5));
    add(pruefe("Seite Models: Baum mit \"models\"", [] {
        return ke::editor().gruppenSeite == 3 && finde("##mbmodels/", "", 0, true) != nullptr;
    }));
    add(foto("karteneditor_modellbrowser_baum"));
    for (const char* p : {"models/", "models/map_objects/"}) {
        const std::string id = std::string("##mb") + p;
        add(klickAn(std::string("Pfeil vor ") + p, [id]() {
            const Element* el = finde(id, "", 0, true);
            if (el == nullptr) { return ImVec2{0, 0}; }
            const int tiefe = static_cast<int>(std::count(id.begin(), id.end(), '/')) - 1;
            return ImVec2{el->rect.Min.x + (20.0F * static_cast<float>(tiefe) + 8.0F) * ke::massstab(), el->rect.GetCenter().y};
        }));
        add(pause(0.2));
    }
    add(pruefe("aufgeklappt: models/map_objects/bespin/ sichtbar", [] { return finde("##mbmodels/map_objects/bespin/", "", 0, true) != nullptr; }));
    add(klickAn("Doppelklick auf bespin", elementOrt("##mbmodels/map_objects/bespin/"), 0, true));
    add(pause(0.6));
    add(pruefe("bespin geladen: Dateien, Ordnerpfad", [] {
        const auto d = ke::modellBrowserDateien();
        diag::detail("Knopftest: Stufe 2 - bespin " + std::to_string(d.size()) + " Modelle");
        return ke::modellBrowserOrdner() == "models/map_objects/bespin/" && !d.empty() &&
               std::find(d.begin(), d.end(), "panels.md3") != d.end();
    }));
    add(pause(0.5));
    add(foto("karteneditor_modellbrowser"));
    add(modellDazu("misc_model ohne Modell, gewaehlt", "", "300 0 32"));
    add(tu("das misc_model waehlen", [] {
        ke::Editor& e = ke::editor();
        for (const auto& ent : e.karte.entitaeten()) {
            if (ent->klassenname() == "misc_model") { e.karte.auswahl.waehlen(ent); }
        }
    }));
    add(klickAn("Klick auf die erste Zelle", modellZelle(0)));
    add(pruefe("Klick: Modell 0 den gewaehlten zugewiesen (\"entityAssignModel\")", [] {
        const auto d = ke::modellBrowserDateien();
        const std::string soll = "models/map_objects/bespin/" + d[0];
        for (const auto& ent : ke::editor().karte.entitaeten()) {
            if (ent->klassenname() == "misc_model" && ent->wert("model") == soll) {
                return ke::modellBrowserAktuell() == 0 && ke::editor().karte.undo.undoName() == "entityAssignModel";
            }
        }
        return false;
    }));
    add(klickAn("Doppelklick auf die erste Zelle", modellZelle(0), 0, true));
    add(pruefe("Doppelklick: neues misc_model vor der Kamera, gewaehlt (\"insertModel\")", [] {
        const ke::Editor& e = ke::editor();
        int n = 0;
        for (const auto& ent : e.karte.entitaeten()) { n += ent->klassenname() == "misc_model" ? 1 : 0; }
        return n == 2 && e.karte.undo.undoName() == "insertModel" && e.karte.auswahl.anzahl() == 1;
    }));
    // Einstellungsseite
    add(tu("Einstellungen auf", [] { ke::editor().einstellungenOffen = true; }));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KePgModBro)) + "##seite21"));
    add(pause(0.2));
    add(pruefe("Seite Model Browser: Model View Size, Ordnerliste", [] {
        return finde("##modordner", "", 0, true) != nullptr;
    }));
    add(foto("karteneditor_pref_modellbrowser"));
    add(klick(ke::trK(Str::KePrefOk)));
    add(tu("Gruppendialog zu", [] {
        ke::editor().gruppenDialogOffen = false;
        ke::editor().gruppenSeite = 0;
    }));
    return s;
}

// Die Mitte einer Klappliste (QComboBox) des Einstellungsdialogs
std::function<ImVec2()> klappOrt(const std::string& id) {
    return [id]() {
        for (const auto& k : ke::editor().klappOrte) {
            if (k.id == id) { return ImVec2{(k.x0 + k.x1) * 0.5F, (k.y0 + k.y1) * 0.5F}; }
        }
        return ImVec2{0, 0};
    };
}

std::string einstellung(const char* schluessel) {
    const auto it = g_app->settings.karte.find(schluessel);
    return it != g_app->settings.karte.end() ? it->second : std::string("<fehlt>");
}

bool farbeNahe(const unsigned char* p, const ke::Farben::F3& f, int toleranz) {
    for (int i = 0; i < 3; ++i) {
        if (std::abs(static_cast<int>(p[i]) - static_cast<int>(std::lround(f[static_cast<std::size_t>(i)] * 255.0F))) > toleranz) { return false; }
    }
    return true;
}

// Anteil der Bildpunkte entlang der oberen Kante des Testpinsels (y = 64,
// x -48..48) in der XY-Ansicht, die die Auswahlfarbe haben ($XY_OVERLAY, 2 breit)
float auswahlKantenAnteil() {
    const ke::Editor& e = ke::editor();
    const karte::XyAnsicht& v = e.xy[0].ansicht;
    int farbig = 0;
    int alle = 0;
    for (int x = -48; x <= 48; ++x) {
        const auto b = v.zuBild(karte::Vektor3f(static_cast<float>(x), 64, 0));
        bool treffer = false;
        for (int dy = -1; dy <= 1; ++dy) {
            unsigned char p[4] = {};
            if (ke::xyPixel(0, static_cast<int>(std::lround(b[0])), static_cast<int>(std::lround(b[1])) + dy, p) && farbeNahe(p, e.farben.auswahl, 40)) {
                treffer = true;
            }
        }
        farbig += treffer ? 1 : 0;
        ++alle;
    }
    return alle > 0 ? static_cast<float>(farbig) / static_cast<float>(alle) : 0.0F;
}

// Weltpunkt -> Bildpunkt der Kamera (Projektion x Modelview, spaltenweise)
ImVec2 kameraBild(const karte::Vektor3f& w) {
    const ke::Editor& e = ke::editor();
    const karte::Matrix4 m = e.kamera.sicht();
    const float x = m[0] * w[0] + m[4] * w[1] + m[8] * w[2] + m[12];
    const float y = m[1] * w[0] + m[5] * w[1] + m[9] * w[2] + m[13];
    const float q = m[3] * w[0] + m[7] * w[1] + m[11] * w[2] + m[15];
    return ImVec2{(x / q * 0.5F + 0.5F) * static_cast<float>(e.kamFenster.breite), (0.5F - y / q * 0.5F) * static_cast<float>(e.kamFenster.hoehe)};
}

// Ein Bildpunkt in der Auswahlfarbe der Kamera im Umkreis von 2 um den Punkt?
bool kameraAuswahlBei(const karte::Vektor3f& w) {
    const ImVec2 b = kameraBild(w);
    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            unsigned char p[4] = {};
            if (ke::kameraPixel(static_cast<int>(b.x) + dx, static_cast<int>(b.y) + dy, p) && farbeNahe(p, ke::editor().farben.auswahl3d, 30)) {
                return true;
            }
        }
    }
    return false;
}

// Mit M1 in der XY-Ansicht von einem Weltpunkt an den rechten Rand ziehen und
// dort `halte` Bilder stehen bleiben (ChaseMouse).
Schritt randZiehen(const std::string& was, ImVec2 von, int halte, std::shared_ptr<float> ursprungVorher) {
    return Schritt{was, [=](int b) {
                       ImGuiIO& io = ImGui::GetIO();
                       const ke::XyFenster& f = ke::editor().xy[0];
                       if (b == 0) {
                           setzeMaus(weltXy(von.x, von.y));
                           return false;
                       }
                       if (b == 1) {
                           io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
                           return false;
                       }
                       const ImVec2 a = weltXy(von.x, von.y);
                       const ImVec2 z{f.bildX + static_cast<float>(f.ansicht.breite) - 4.0F, a.y};
                       if (b >= 2 && b <= 11) {
                           const float t = static_cast<float>(b - 1) / 10.0F;
                           setzeMaus(ImVec2{a.x + (z.x - a.x) * t, a.y});
                           if (b == 11) { *ursprungVorher = f.ansicht.ursprung[0]; }
                           return false;
                       }
                       if (b == 12 + halte) {
                           io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
                           return false;
                       }
                       return b >= 15 + halte;
                   }};
}

// Punkt 11a: die Einstellungsseiten Settings, Undo, Brush, Grid, Orthographic,
// Build und "Selected faces wire" (Camera) mit ihren Wirkungen
std::vector<Schritt> einstellungen11() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto seite = [&](Str baum, int i) {
        add(tu("Einstellungen auf", [] { ke::editor().einstellungenOffen = true; }));
        add(pause(0.3));
        add(klick(std::string(ke::trK(baum)) + "##seite" + std::to_string(i)));
        add(pause(0.2));
    };
    const auto ok = [&]() {
        add(klick(ke::trK(Str::KePrefOk)));
        add(pause(0.2));
    };
    add(grundzustand("Stufe 2/11a: Grundzustand"));
    // --- Settings: Nudge selected after duplication, Load last map at startup
    seite(Str::KePgSettings, 10);
    add(pruefe("Seite Settings: Nudge selected after duplication, Load last map at startup (an)", [] {
        return finde(std::string(ke::trK(Str::KePrNudgeClone)) + "##nachklonen", "", 0, true) != nullptr &&
               finde(std::string(ke::trK(Str::KePrLoadLastMap)) + "##letztekarte", "", 0, true) != nullptr && ke::editor().letzteKarteLaden &&
               !ke::editor().nachKlonenSchubsen;
    }));
    add(foto("karteneditor_pref_settings"));
    add(klick(std::string(ke::trK(Str::KePrNudgeClone)) + "##nachklonen"));
    add(klick(std::string(ke::trK(Str::KePrLoadLastMap)) + "##letztekarte"));
    ok();
    add(pruefe("OK: Nudge an, Load last map aus, gespeichert (NudgeAfterClone, LoadLastMap)", [] {
        const ke::Editor& e = ke::editor();
        return e.nachKlonenSchubsen && !e.letzteKarteLaden && einstellung("NudgeAfterClone") == "1" && einstellung("LoadLastMap") == "0";
    }));
    add(klickXy("M1 auf den Pinsel", ImGuiMouseButton_Left, ImVec2{32, 32}));
    add(tu("Edit > Duplicate (CloneSelection)", [] { (void)ke::ausfuehren("CloneSelection"); }));
    add(pruefe("Duplicate: der Klon ein Raster (8) nach rechts und unten (NudgeSelection eNudgeRight, eNudgeDown)", [] {
        return pinselZahl() == 2 && kastenGleich(grenzenVon(1), karte::Vektor3f(-56, -72, 0), karte::Vektor3f(72, 56, 64)) &&
               ke::editor().karte.undo.undoName() == "cloneSelected";
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(klickXy("M1 auf den Pinsel (Rueckgaengig nahm den Klon samt Auswahl)", ImGuiMouseButton_Left, ImVec2{32, 32}));
    add(tu("Nudge aus, Duplicate", [] {
        ke::editor().nachKlonenSchubsen = false;
        (void)ke::ausfuehren("CloneSelection");
    }));
    add(pruefe("ohne Nudge: der Klon liegt auf dem Original", [] {
        return pinselZahl() == 2 && kastenGleich(grenzenVon(1), karte::Vektor3f(-64, -64, 0), karte::Vektor3f(64, 64, 64));
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tu("Start mit \"Load last map\" aus (LastMap = karte_quelle.map)", [] {
        ke::Editor& e = ke::editor();
        e.letzteKarte = karteTestPfad("karte_quelle.map");
        e.letzteKarteGeprueft = false;
        ke::letzteKarteBeimStart();
    }));
    add(pruefe("aus: die Karte bleibt karte_kopie.map", [] { return ke::editor().name == karteTestPfad("karte_kopie.map"); }));
    add(tu("Start mit \"Load last map\" an", [] {
        ke::Editor& e = ke::editor();
        e.letzteKarteLaden = true;
        e.letzteKarteGeprueft = false;
        ke::letzteKarteBeimStart();
    }));
    add(pruefe("an: karte_quelle.map geladen (Map_LoadFile, nicht in der MRU-Liste)", [] {
        const ke::Editor& e = ke::editor();
        return e.name == karteTestPfad("karte_quelle.map") && e.karte.entitaeten().size() == 2 &&
               (e.zuletzt.anzahl() == 0 || e.zuletzt.pfade()[0] != karteTestPfad("karte_quelle.map"));
    }));
    add(pruefe("nur einmal: ein zweiter Aufruf laedt nicht neu", [] {
        const std::size_t vorher = ke::editor().ladeZaehler;
        ke::letzteKarteBeimStart();
        return ke::editor().ladeZaehler == vorher;
    }));
    add(pruefe("Beenden: LastMap = die offene Karte; unbenannt bleibt der alte Name (main.cpp:476)", [] {
        ke::Editor& e = ke::editor();
        ke::beimBeenden();
        const bool benannt = einstellung("LastMap") == karteTestPfad("karte_quelle.map");
        const std::string alt = e.name;
        e.name = "unnamed.map";
        ke::beimBeenden();
        e.name = alt;
        return benannt && einstellung("LastMap") == karteTestPfad("karte_quelle.map");
    }));
    add(grundzustand("Stufe 2/11a: Grundzustand"));
    // --- Undo: Undo Queue Size
    seite(Str::KePgUndo, 11);
    add(pruefe("Seite Undo: Undo Queue Size 512", [] { return finde("##undostufen", "", 0, true) != nullptr && ke::editor().karte.undo.stufen() == 512; }));
    add(foto("karteneditor_pref_undo"));
    add(klick("+", "karteeinstellungen"));
    add(pruefe("Pfeil hoch: 512 -> 522 (QSpinBox AdaptiveDecimalStep: eine Zehnerpotenz unter dem Wert)", [] {
        return ke::einstellungsFeld("undostufen") == "522";
    }));
    add(klickAn("Feld Undo Queue Size", elementOrt("##undostufen")));
    add(tippe("100"));
    add(klick("-", "karteeinstellungen"));
    add(pruefe("Pfeil runter bei 100: 99 (Schritt 1 unter 100)", [] { return ke::einstellungsFeld("undostufen") == "99"; }));
    add(klickAn("Feld Undo Queue Size", elementOrt("##undostufen")));
    add(tippe("2"));
    ok();
    add(pruefe("OK: Undo Queue Size 2 (setLevels, UndoLevels)", [] { return ke::editor().karte.undo.stufen() == 2 && einstellung("UndoLevels") == "2"; }));
    add(klickXy("M1 auf den Pinsel", ImGuiMouseButton_Left, ImVec2{32, 32}));
    add(tu("dreimal Alt+Rechts (SelectNudgeRight)", [] {
        for (int i = 0; i < 3; ++i) { (void)ke::ausfuehren("SelectNudgeRight"); }
    }));
    add(tu("dreimal Rueckgaengig", [] {
        for (int i = 0; i < 3; ++i) { (void)ke::ausfuehren("Undo"); }
    }));
    add(pruefe("nur zwei Schritte zurueck: der Pinsel bleibt um 8 verschoben", [] {
        return kastenGleich(grenzenVon(0), karte::Vektor3f(-56, -64, 0), karte::Vektor3f(72, 64, 64));
    }));
    add(tu("Undo Queue Size zurueck auf 512", [] {
        ke::editor().karte.undo.setzeStufen(512);
        ke::einstellungenSpeichern();
    }));
    add(grundzustand("Stufe 2/11a: Grundzustand"));
    // --- Brush: Default texture scale, New map Brush Type, caulk, snap planes
    seite(Str::KePgBrush, 13);
    add(pruefe("Seite Brush: Feld 0.2500, Klappliste, zwei Haken", [] {
        return finde(std::string(ke::trK(Str::KePrBrushCaulk)) + "##immercaulk", "", 0, true) != nullptr &&
               finde(std::string(ke::trK(Str::KePrBrushSnap)) + "##ganzzahlig", "", 0, true) != nullptr &&
               karte::standardTexturSkalierung() == 0.25F && klappOrt("##pinseltyp")().x > 0.0F;
    }));
    add(klickAn("Klappliste New map Brush Type", klappOrt("##pinseltyp")));
    add(pause(0.2));
    add(klick(ke::trK(Str::KePrBrushBp), "##Combo_"));
    add(pause(0.2));
    add(foto("karteneditor_pref_brush"));
    add(klick("+", "karteeinstellungen"));
    add(klick(std::string(ke::trK(Str::KePrBrushCaulk)) + "##immercaulk"));
    add(klick(std::string(ke::trK(Str::KePrBrushSnap)) + "##ganzzahlig"));
    ok();
    add(pruefe("OK: Skala 0.26 (AdaptiveDecimalStep 0.01), Brush Primitives, Caulk, Snap - gespeichert", [] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Stufe 2 - Skala " + std::to_string(karte::standardTexturSkalierung()) + ", Typ " + std::to_string(e.pinselTyp));
        return std::fabs(karte::standardTexturSkalierung() - 0.26F) < 1e-6F && e.pinselTyp == 1 && e.bearbeitung.immerCaulk &&
               karte::ebenenPunkteGanzzahlig() && einstellung("BrushType") == "1" && einstellung("BrushAlwaysCaulk") == "1" &&
               einstellung("BrushSnapPlanes") == "1" && einstellung("TexdefDefaultScale") == "0.26";
    }));
    add(tu("File > New Map", [] { ke::neueKarte(); }));
    add(pause(0.2));
    add(pruefe("New Map: Format Brush Primitives (Map_Free: Brush_unlatchPreferences)", [] {
        const ke::Editor& e = ke::editor();
        return !e.frageOffen && e.name == "unnamed.map" && e.karte.kontext.format == karte::PinselFormat::Quake3BP;
    }));
    add(tu("Testkarte wieder offen (Axial Projection aus der Datei)", [] { (void)ke::karteOeffnen(karteTestPfad("karte_kopie.map")); }));
    add(pruefe("Map_Read erkennt das Format der Datei: Axial Projection", [] {
        return ke::editor().karte.kontext.format == karte::PinselFormat::Quake3;
    }));
    add(ziehenXy("neuer Pinsel (100 100)..(164 164)", ImGuiMouseButton_Left, 0, ImVec2{100, 100}, ImVec2{164, 164}));
    add(pruefe("neuer Pinsel: alle Flaechen common/caulk, Skala 0.26 (Always use caulk, Default texture scale)", [] {
        const auto w = ke::editor().karte.welt();
        if (!w || w->kinder().size() != 2) { return false; }
        const auto p = std::dynamic_pointer_cast<karte::Pinsel>(w->kinder().back());
        if (!p) { return false; }
        bool alle = !p->flaechen().empty();
        for (const auto& f : p->flaechen()) {
            alle = alle && f->shader() == "textures/common/caulk" && std::fabs(f->projektion().texdef.scale[0] - 0.26F) < 1e-6F;
        }
        return alle;
    }));
    add(tu("ein Pinsel mit Bruchteilen (0.25..64.75) dazu", [] {
        ke::Editor& e = ke::editor();
        karte::TexturProjektion proj;
        karte::texdefStandard(karte::TexdefArt::Quake, proj);
        auto p = std::make_shared<karte::Pinsel>();
        p->quaderBauen(karte::Kasten(karte::Vektor3f(32.5F, 32.5F, 300.5F), karte::Vektor3f(32.25F, 32.25F, 32.25F)), "textures/common/caulk", proj,
                       e.karte.kontext);
        e.karte.welt()->kindEinfuegen(p);
    }));
    add(pruefe("Snap plane points: gespeichert ganzzahlig (64.75 -> 65)", [] {
        const std::string t = karte::karteSchreiben(ke::editor().karte);
        return t.find("64.75") == std::string::npos && t.find("( 65 65 333 )") != std::string::npos;
    }));
    add(pruefe("ohne Snap: 64.75 bleibt", [] {
        karte::setzeEbenenPunkteGanzzahlig(false);
        const std::string t = karte::karteSchreiben(ke::editor().karte);
        return t.find("64.75") != std::string::npos;
    }));
    add(tu("Brush zurueck: 0.25, Axial Projection, kein Caulk", [] {
        ke::Editor& e = ke::editor();
        karte::setzeStandardTexturSkalierung(0.25F);
        e.pinselTyp = 0;
        e.bearbeitung.immerCaulk = false;
        ke::einstellungenSpeichern();
    }));
    add(grundzustand("Stufe 2/11a: Grundzustand"));
    // --- Grid: Default grid spacing, Max grid coordinate
    seite(Str::KePgGrid, 15);
    add(pruefe("Seite Grid: zwei Klapplisten", [] { return klappOrt("##rasterstandard")().x > 0.0F && klappOrt("##rastermax")().x > 0.0F; }));
    add(foto("karteneditor_pref_grid"));
    add(klickAn("Klappliste Default grid spacing", klappOrt("##rasterstandard")));
    add(pause(0.2));
    add(klick("64", "##Combo_"));
    add(pause(0.2));
    add(klickAn("Klappliste Max grid coordinate", klappOrt("##rastermax")));
    add(pause(0.2));
    add(klick("4096", "##Combo_"));
    add(pause(0.2));
    ok();
    add(pruefe("OK: GridDefault 9 (64, gilt beim Start), Max 4096 sofort - Region-Grenzen +-4096 (Region_defaultMinMax)", [] {
        const ke::Editor& e = ke::editor();
        karte::Raster r;
        r.standardSetzen(std::atoi(einstellung("GridDefault").c_str()));
        return e.rasterStandard == 9 && einstellung("GridDefault") == "9" && r.groesse() == 64.0F && e.raster.groesse() == 8.0F &&
               e.raster.maxKoord() == 4096.0F && e.region.max[0] == 4096.0F && e.region.min[2] == -4096.0F && einstellung("GridMaxCoordPower") == "0";
    }));
    add(tu("Grid zurueck: 16, 65536", [] {
        ke::Editor& e = ke::editor();
        e.rasterStandard = karte::Raster::kStandard;
        e.rasterMaxPotenz = 4;
        e.raster.setzeMaxKoordPotenz(4);
        karte::regionStandard(e.region, e.raster.maxKoord());
        ke::einstellungenSpeichern();
    }));
    // --- Orthographic: Solid selection boxes, Chase mouse, Zoom to Mouse pointer, MSAA
    seite(Str::KePgOrtho, 18);
    add(pruefe("Seite Orthographic: drei Haken (an), MSAA 8", [] {
        const ke::Editor& e = ke::editor();
        return finde(std::string(ke::trK(Str::KePrOrthoStipple)) + "##keinestrichelung", "", 0, true) != nullptr && e.anzeige.keineStrichelung &&
               e.anzeige.jagen && e.anzeige.zumZeiger && e.anzeige.msaa == 8;
    }));
    add(foto("karteneditor_pref_ortho"));
    add(klick(std::string(ke::trK(Str::KePrOrthoStipple)) + "##keinestrichelung"));
    ok();
    add(pruefe("OK: Solid selection boxes aus (NoStipple 0)", [] { return !ke::editor().anzeige.keineStrichelung && einstellung("NoStipple") == "0"; }));
    add(klickXy("M1 auf den Pinsel", ImGuiMouseButton_Left, ImVec2{32, 32}));
    add(pause(0.3));
    add(pruefe("gestrichelt: an der Kante etwa die Haelfte in der Auswahlfarbe (glLineStipple 3, 0xaaaa)", [] {
        const float a = auswahlKantenAnteil();
        diag::detail("Knopftest: Stufe 2 - Kante gestrichelt " + std::to_string(a));
        return a > 0.3F && a < 0.75F;
    }));
    add(tu("Solid selection boxes an", [] { ke::editor().anzeige.keineStrichelung = true; }));
    add(pause(0.3));
    add(pruefe("durchgezogen: die ganze Kante in der Auswahlfarbe", [] {
        const float a = auswahlKantenAnteil();
        diag::detail("Knopftest: Stufe 2 - Kante durchgezogen " + std::to_string(a));
        return a > 0.95F;
    }));
    {
        auto vorher = std::make_shared<float>(0.0F);
        add(randZiehen("den Pinsel an den rechten Rand ziehen, 20 Bilder halten (Chase mouse an)", ImVec2{0, 0}, 20, vorher));
        add(pruefe("Chase mouse an: die Ansicht rollt nach rechts (ChaseMouse: Scroll)", [vorher] {
            diag::detail("Knopftest: Stufe 2 - Ursprung x " + std::to_string(*vorher) + " -> " + std::to_string(ke::editor().xy[0].ansicht.ursprung[0]));
            return ke::editor().xy[0].ansicht.ursprung[0] > *vorher + 8.0F;
        }));
        add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
        add(tu("Ansicht zurueck, Chase mouse aus", [] {
            ke::editor().xy[0].ansicht.setzeUrsprung(karte::Vektor3f(0, 0, 0));
            ke::editor().anzeige.jagen = false;
        }));
        add(randZiehen("den Pinsel an den rechten Rand ziehen, 20 Bilder halten (Chase mouse aus)", ImVec2{0, 0}, 20, vorher));
        add(pruefe("Chase mouse aus: die Ansicht bleibt", [vorher] { return ke::editor().xy[0].ansicht.ursprung[0] == *vorher; }));
        add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    }
    add(tu("Ansicht zurueck", [] {
        ke::editor().xy[0].ansicht.setzeUrsprung(karte::Vektor3f(0, 0, 0));
        ke::editor().xy[0].ansicht.setzeMassstab(1);
    }));
    add(radAn("hinein bei (200 100)", [] { return weltXy(200, 100); }, 1.0F));
    add(pruefe("Zoom to Mouse pointer an: der Ursprung wandert zur Maus (ZoomInWithMouse)", [] {
        const karte::Vektor3f& o = ke::editor().xy[0].ansicht.ursprung;
        return ke::editor().xy[0].ansicht.massstab > 1.0F && o[0] > 1.0F && o[1] > 1.0F;
    }));
    add(tu("Ansicht zurueck, Zoom to Mouse pointer aus", [] {
        ke::editor().xy[0].ansicht.setzeUrsprung(karte::Vektor3f(0, 0, 0));
        ke::editor().xy[0].ansicht.setzeMassstab(1);
        ke::editor().anzeige.zumZeiger = false;
    }));
    add(radAn("hinein bei (200 100)", [] { return weltXy(200, 100); }, 1.0F));
    add(pruefe("Zoom to Mouse pointer aus: der Ursprung bleibt", [] {
        const karte::Vektor3f& o = ke::editor().xy[0].ansicht.ursprung;
        return ke::editor().xy[0].ansicht.massstab > 1.0F && o[0] == 0.0F && o[1] == 0.0F;
    }));
    seite(Str::KePgOrtho, 18);
    add(klickAn("Klappliste MSAA", klappOrt("##xymsaa")));
    add(pause(0.2));
    add(klick("0", "##Combo_"));
    add(pause(0.2));
    ok();
    add(pause(0.3));
    add(pruefe("MSAA 0: das Renderziel der XY-Ansicht ohne Mehrfachabtastung (XYMSAA 0)", [] {
        diag::detail("Knopftest: Stufe 2 - XY-Abtastungen " + std::to_string(ke::xyAbtastungen(0)));
        return ke::editor().anzeige.msaa == 0 && einstellung("XYMSAA") == "0" && ke::xyAbtastungen(0) == 1;
    }));
    add(tu("MSAA 8", [] { ke::editor().anzeige.msaa = 8; }));
    add(pause(0.3));
    add(pruefe("MSAA 8: wieder mehrfach abgetastet", [] { return ke::xyAbtastungen(0) >= 2; }));
    add(tu("Orthographic zurueck", [] {
        ke::editor().anzeige = ke::Anzeige{};
        ke::einstellungenSpeichern();
    }));
    // --- Build
    seite(Str::KePgBuild, 19);
    add(pruefe("Seite Build: default:jaSP.exe, default:+set sv_pure 0 [+set fs_game MD] +devmap %mapname% (DefaultableString)", [] {
        const karte::SpielBeschreibung& sb = ke::kartenSpiel();
        diag::detail("Knopftest: Stufe 2 - Build " + ke::einstellungsFeld("bauengine") + " / " + ke::einstellungsFeld("bauargumente"));
        const std::string fs = ke::bauSpielName() != sb.basisSpiel ? " +set fs_game " + ke::bauSpielName() : std::string();
        return ke::einstellungsFeld("bauengine") == std::string("default:") + sb.engine &&
               ke::einstellungsFeld("bauargumente") == "default:+set sv_pure 0" + fs + " +devmap %mapname%" &&
               ke::einstellungsFeld("bauenginemp") == std::string("default:") + sb.mpEngine && finde("##bauengine", "", 0, true) != nullptr;
    }));
    add(foto("karteneditor_pref_build"));
    add(klick(std::string(ke::trK(Str::KePrBuildRun)) + "##baustarten"));
    add(klickAn("Feld Engine to Run", elementOrt("##bauengine")));
    add(tippe("openjk.x86.exe"));
    add(klickAn("Feld Region Box Shader", elementOrt("##regionshader")));
    add(tippe("textures/common/nodraw"));
    ok();
    add(pruefe("OK: Run Engine an, eigenes Programm, Region Box Shader - gespeichert", [] {
        const ke::Editor& e = ke::editor();
        return e.bau.spielStarten && e.bau.engine.text == "openjk.x86.exe" && e.bau.argumente.text.empty() &&
               e.regionKastenShader == "textures/common/nodraw" && einstellung("BuildRunGame") == "1" &&
               einstellung("BuildEngineExecutable") == "openjk.x86.exe" && einstellung("BuildEngineArgs").empty() &&
               einstellung("RegionBoxShader") == "textures/common/nodraw";
    }));
    add(tu("den Pinsel waehlen, Region > Set selection, File > Save region...", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), pinsel());
        (void)ke::ausfuehren("RegionSetSelection");
        (void)ke::regionSpeichernUnter(laufDatei("stufe2_region11.map"));
        (void)ke::ausfuehren("RegionOff");
    }));
    add(pruefe("Save region: die Waende mit common/nodraw (ScopeRegionBrushes: g_regionBoxShader)", [] {
        return dateiText(laufDatei("stufe2_region11.map")).find("common/nodraw") != std::string::npos;
    }));
    add(tu("Build zurueck", [] {
        ke::Editor& e = ke::editor();
        e.bau = ke::Editor::BauEinstellungen{};
        e.regionKastenShader.clear();
        ke::einstellungenSpeichern();
    }));
    // --- Camera: Selected faces wire
    add(tu("Kamera vor die -x-Seite, Faerben aus, die Flaeche x = -64 waehlen", [] {
        ke::Editor& e = ke::editor();
        e.kameraE.faerben = false;
        e.kamera.setzeUrsprung(karte::Vektor3f(-256, 0, 32));
        e.kamera.setzeWinkel(karte::Vektor3f(0, 0, 0));
        e.bearbeitung.einstellung.modus = karte::AuswahlModus::Komponente;
        e.bearbeitung.einstellung.komponente = karte::KomponentenModus::Flaeche;
        const auto p = pinsel();
        for (const auto& f : p->flaechen()) {
            if (f->ebene().normal()[0] < -0.5) { e.karte.auswahl.flaecheWaehlen(p, f); }
        }
    }));
    add(pause(0.4));
    add(pruefe("Selected faces wire an: Kante der Flaeche in der Auswahlfarbe ($CAM_FACEWIRE)", [] {
        return ke::editor().karte.auswahl.flaechen().size() == 1 && kameraAuswahlBei(karte::Vektor3f(-64, 0, 64));
    }));
    add(tu("Selected faces wire aus", [] { ke::editor().kameraE.flaechenDraht = false; }));
    add(pause(0.4));
    add(pruefe("Selected faces wire aus: keine Kante in der Auswahlfarbe", [] { return !kameraAuswahlBei(karte::Vektor3f(-64, 0, 64)); }));
    add(tu("Kamera zurueck", [] {
        ke::Editor& e = ke::editor();
        e.kameraE = karte::KameraEinstellungen{};
        e.karte.auswahl.leeren();
        e.bearbeitung.einstellung.modus = karte::AuswahlModus::Primitiv;
        e.bearbeitung.einstellung.komponente = karte::KomponentenModus::Standard;
    }));
    // --- Global, Display: leer wie NRC
    seite(Str::KePgGlobal, 0);
    add(foto("karteneditor_pref_global"));
    add(klick(std::string(ke::trK(Str::KePgDisplay)) + "##seite6"));
    add(pause(0.2));
    add(foto("karteneditor_pref_display"));
    ok();
    return s;
}

bool protokollHat(const karte::Protokoll& p, const std::string& text) {
    for (const karte::ProtokollTeil& t : p) {
        if (t.text.find(text) != std::string::npos) { return true; }
    }
    return false;
}

bool dateiImSpiel(const std::string& pfad) {
    const auto& sp = ke::editor().spiel;
    std::string bytes;
    return sp && sp->fs.dateiLesen(pfad, bytes);
}

void testDateiSchreiben(const std::string& pfad, const std::string& text) {
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(pfad).parent_path(), ec);
    (void)spewDatei(pfad, text);
}

// Punkt 11b: Global > Game ("Select the game", Neustart), der Dialog "Global
// Preferences" beim Start, Game (Mod, fs_game, Mapping mode), Paths (Engine Path,
// Extra Resource Path) - jedes mit dem neu geladenen Spiel nachgeprueft
std::vector<Schritt> einstellungen11b() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto seite = [&](Str baum, int i) {
        add(tu("Einstellungen auf", [] { ke::editor().einstellungenOffen = true; }));
        add(pause(0.3));
        add(klick(std::string(ke::trK(baum)) + "##seite" + std::to_string(i)));
        add(pause(0.2));
    };
    const auto ok = [&]() {
        add(klick(ke::trK(Str::KePrefOk)));
        add(pause(0.3));
    };
    const auto spielWaehlen = [&](const char* name) {
        add(klickAn("Klappliste Select the game", klappOrt("##spielwahl")));
        add(pause(0.2));
        add(klick(name, "##Combo_"));
        add(pause(0.2));
    };
    add(grundzustand("Stufe 2/11b: Grundzustand"));
    // --- Global > Game: Select the game (LatchedValue, Neustart)
    seite(Str::KePgGame, 1);
    add(pruefe("Seite Game (Global): Select the game, Startup / Show Global Preferences", [] {
        return klappOrt("##spielwahl")().x > 0.0F && finde(std::string(ke::trK(Str::KePrShowGlobal)) + "##spielfrage", "", 0, true) != nullptr;
    }));
    add(foto("karteneditor_pref_global_game"));
    spielWaehlen("Jedi Knight II: Jedi Outcast");
    ok();
    add(pruefe("OK: Frage \"Restart is required\" mit \"Selected Game\" (PreferencesDialog_showDialog)", [] {
        const auto& m = ke::editor().meldung;
        return m.offen && m.titel == "Restart is required" && m.text.find("Selected Game") != std::string::npos && m.text.find("Restart now?") != std::string::npos;
    }));
    add(foto("karteneditor_neustart"));
    add(klick(std::string(ke::trK(Str::KeNo)) + "##meldungnein"));
    add(pause(0.2));
    add(pruefe("No: jk2.game gemerkt (gamefile = m_latched), das Spiel bleibt Jedi Academy", [] {
        const ke::Editor& e = ke::editor();
        return e.spielDateiNeu == "jk2.game" && einstellung("gamefile") == "jk2.game" && ke::kartenSpielNummer() != kRegelwerkJediOutcast &&
               e.spiel && e.spiel->spiel != kRegelwerkJediOutcast;
    }));
    seite(Str::KePgGame, 1);
    spielWaehlen("Jedi Knight Jedi Academy");
    ok();
    add(pruefe("wieder Jedi Academy: keine Frage, nichts mehr gemerkt", [] {
        return !ke::editor().meldung.offen && ke::editor().spielDateiNeu.empty() && einstellung("gamefile") != "jk2.game";
    }));
    seite(Str::KePgGame, 1);
    spielWaehlen("Jedi Knight II: Jedi Outcast");
    ok();
    add(klick(std::string(ke::trK(Str::KeYes)) + "##meldungja"));
    add(warteBis("Restart now: Jedi Outcast geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    add(pruefe("Yes (Radiant_Restart): Jedi Outcast mit data/karte/jk2, die Karte wieder offen", [] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Stufe 2 - nach dem Neustart " + (e.spiel ? e.spiel->name + ", " + std::to_string(e.spiel->klassen.anzahl()) + " Klassen" : std::string("-")));
        return e.spiel && e.spiel->spiel == kRegelwerkJediOutcast && e.karteSpiel == e.spiel && std::string(ke::kartenSpiel().typ) == "jk2" &&
               e.name == karteTestPfad("karte_kopie.map") && e.spielDatei == "jk2.game" && e.spiel->klassen.anzahl() > 50;
    }));
    seite(Str::KePgGame, 1);
    spielWaehlen("Jedi Knight Jedi Academy");
    ok();
    add(klick(std::string(ke::trK(Str::KeYes)) + "##meldungja"));
    add(warteBis("Restart now: Jedi Academy geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    add(pruefe("zurueck auf Jedi Academy", [] {
        const ke::Editor& e = ke::editor();
        return e.spiel && e.spiel->spiel != kRegelwerkJediOutcast && e.karteSpiel == e.spiel && e.spielDatei == "ja.game" &&
               e.spiel->klassen.finden("info_player_start") != nullptr;
    }));
    // --- Startup / Show Global Preferences: der Dialog beim Start
    seite(Str::KePgGame, 1);
    add(klick(std::string(ke::trK(Str::KePrShowGlobal)) + "##spielfrage"));
    ok();
    add(pruefe("OK: gamePrompt 1", [] { return ke::editor().spielFrage && einstellung("gamePrompt") == "1"; }));
    add(tu("Start des Editors nachstellen (CGameDialog::Init)", [] { ke::editor().spielFrageGezeigt = false; }));
    add(pause(0.4));
    add(pruefe("Start: \"Global Preferences\" offen (Game settings, Select the game, Startup)", [] {
        return ke::editor().globaleEinstellungenOffen && finde(std::string(ke::trK(Str::KePrShowGlobal)) + "##globalspielfrage", "", 0, true) != nullptr &&
               klappOrt("##globalspielwahl")().x > 0.0F;
    }));
    add(foto("karteneditor_global_preferences"));
    add(klick(std::string(ke::trK(Str::KePrShowGlobal)) + "##globalspielfrage"));
    add(klick(std::string(ke::trK(Str::KePrefOk)) + "##globalok"));
    add(pause(0.3));
    add(pruefe("OK: zu, Show Global Preferences aus (SavePrefs)", [] {
        return !ke::editor().globaleEinstellungenOffen && !ke::editor().spielFrage && einstellung("gamePrompt") == "0";
    }));
    // --- Game: Select mod, fs_game, Mapping mode; Paths: Extra Resource Path
    seite(Str::KePgGame, 2);
    add(pruefe("Seite Game: Select mod (basegame), fs_game \"base\" gesperrt, Mapping mode", [] {
        return ke::einstellungsFeld("fsgame") == "base" && ke::einstellungsFeld("fsgameaktiv") == "0" && klappOrt("##modwahl")().x > 0.0F &&
               klappOrt("##spielmodus")().x > 0.0F;
    }));
    add(foto("karteneditor_pref_game"));
    add(klickAn("Klappliste Select mod", klappOrt("##modwahl")));
    add(pause(0.2));
    add(klick("Custom JA modification", "##Combo_"));
    add(pause(0.2));
    add(pruefe("Custom JA modification: fs_game leer und frei (gamecombo_for_gamename)", [] {
        return ke::einstellungsFeld("fsgame").empty() && ke::einstellungsFeld("fsgameaktiv") == "1";
    }));
    add(klickAn("Feld fs_game", elementOrt("##fsgame")));
    add(tippe("testmod11"));
    add(klickAn("Klappliste Mapping mode", klappOrt("##spielmodus")));
    add(pause(0.2));
    add(klick(ke::trK(Str::KePrMpMode), "##Combo_"));
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KePgPaths)) + "##seite3"));
    add(pause(0.2));
    add(pruefe("Seite Paths: Engine Path = der Ordner ueber dem ersten Spielordner, fuenf Extra Resource Path", [] {
        diag::detail("Knopftest: Stufe 2 - Engine Path " + ke::einstellungsFeld("enginepfad"));
        return !ke::einstellungsFeld("enginepfad").empty() && ke::einstellungsFeld("enginepfad") == ke::enginePfadAnzeige() &&
               finde("##zusatzpfad4", "", 0, true) != nullptr;
    }));
    add(foto("karteneditor_pref_paths"));
    add(tu("Testordner zusatz11 mit maps/zusatz11.txt", [] { testDateiSchreiben(laufDatei("zusatz11/maps/zusatz11.txt"), "zusatz11"); }));
    add(klickAn("Feld Extra Resource Path 1", elementOrt("##zusatzpfad0")));
    add(tippe(laufDatei("zusatz11")));
    ok();
    add(pause(0.3));
    add(pruefe("OK: fs_game testmod11, mp, Extra Resource Path - das Spiel neu geladen, gespeichert", [] {
        const ke::Editor& e = ke::editor();
        const std::string zusatz = karte::ordnerBereinigt(laufDatei("zusatz11"));
        const std::vector<std::string> v = ke::kartenVfsOrdner();
        diag::detail("Knopftest: Stufe 2 - VFS " + (v.empty() ? std::string("-") : v[0]) + " | " + (v.size() > 1 ? v[1] : std::string("-")));
        return e.spielName == "testmod11" && e.spielModus == "mp" && e.zusatzPfade[0] == zusatz && e.spiel && e.spiel->modus == "mp" &&
               v.size() >= 3 && v[0] == zusatz && v[1].size() > 10 && v[1].substr(v[1].size() - 10) == "testmod11/" &&
               einstellung("GameName") == "testmod11" && einstellung("GameMode") == "mp" && einstellung("ExtraResourcePath0") == zusatz;
    }));
    add(pruefe("Extra Resource Path: maps/zusatz11.txt im Spiel (QE_InitVFS)", [] { return dateiImSpiel("maps/zusatz11.txt"); }));
    add(pruefe("Mapping mode mp: sp_entities.def uebergangen (EntityClassesLoadFile)", [] {
        return protokollHat(ke::editor().spiel->klassen.protokoll, "Ignoring 'sp_entities.def'");
    }));
    // --- Paths: Engine Path
    add(tu("Testordner engine11 mit base/maps/engine11.txt und testmod11/maps/mod11.txt", [] {
        testDateiSchreiben(laufDatei("engine11/base/maps/engine11.txt"), "engine11");
        testDateiSchreiben(laufDatei("engine11/testmod11/maps/mod11.txt"), "mod11");
    }));
    seite(Str::KePgPaths, 3);
    add(klickAn("Feld Engine Path", elementOrt("##enginepfad")));
    add(tippe(laufDatei("engine11")));
    ok();
    add(pause(0.3));
    add(pruefe("Engine Path: zusatz11/, engine11/testmod11/, engine11/base/ (QE_InitVFS), beide Dateien gefunden", [] {
        const ke::Editor& e = ke::editor();
        const std::string engine = karte::ordnerBereinigt(laufDatei("engine11"));
        const std::vector<std::string> v = ke::kartenVfsOrdner();
        return e.enginePfad == engine && v.size() == 3 && v[1] == engine + "testmod11/" && v[2] == engine + "base/" && dateiImSpiel("maps/engine11.txt") &&
               dateiImSpiel("maps/mod11.txt") && dateiImSpiel("maps/zusatz11.txt") && einstellung("EnginePath") == engine;
    }));
    add(tu("Spiel zurueck: Engine Path aus den Spielordnern, ohne Mod und Zusatzpfad, sp", [] {
        ke::spielNeuRealisieren("Edit Project Settings", [] {
            ke::Editor& e = ke::editor();
            e.enginePfad.clear();
            e.zusatzPfade = {};
            e.spielName.clear();
            e.spielModus = "sp";
            ke::einstellungenSpeichern();
        });
    }));
    add(pause(0.3));
    add(pruefe("zurueck: sp (mp_entities.def uebergangen), die Spielordner des Programms", [] {
        const ke::Editor& e = ke::editor();
        return e.spiel && e.spiel->modus == "sp" && protokollHat(e.spiel->klassen.protokoll, "Ignoring 'mp_entities.def'") && !dateiImSpiel("maps/zusatz11.txt") &&
               e.spiel->klassen.finden("info_player_start") != nullptr;
    }));
    return s;
}

// Eine unkomprimierte 24-Bit-TGA (4 x 4) in einer Farbe
std::string tgaBild(unsigned char r, unsigned char g, unsigned char b) {
    std::string t(18, '\0');
    t[2] = 2;
    t[12] = 4;
    t[14] = 4;
    t[16] = 24;
    for (int i = 0; i < 16; ++i) {
        t.push_back(static_cast<char>(b));
        t.push_back(static_cast<char>(g));
        t.push_back(static_cast<char>(r));
    }
    return t;
}

ImVec2 menueOben() {   // die Abreisskante des offenen Menues (###Menu_00)
    ImGuiWindow* w = ImGui::FindWindowByName("###Menu_00");
    return w != nullptr ? ImVec2{w->Pos.x + w->Size.x * 0.5F, w->Pos.y + 4.0F * ke::massstab()} : ImVec2{0, 0};
}

// Punkt 11c: Fullscreen, abreissbare Menues, Set 2D Background image, Bug report,
// About, Shader Editor Command, Engine Path Configuration, OpenGL Font
std::vector<Schritt> einstellungen11c() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto rechteck = std::make_shared<std::array<int, 4>>();
    auto aufgezeichnet = std::make_shared<std::string>();
    add(grundzustand("Stufe 2/11c: Grundzustand"));
    // --- Fullscreen (F11)
    add(tu("Fenster merken, Misc > Fullscreen", [rechteck] {
        auto& r = *rechteck;
        (void)platform::hauptfensterRechteck(r[0], r[1], r[2], r[3]);
        (void)ke::ausfuehren("Fullscreen");
    }));
    add(pause(0.5));
    add(pruefe("Fullscreen: das Fenster deckt den Bildschirm (WS_POPUP, Qt::WindowFullScreen)", [] {
        int x = 0, y = 0, b = 0, h = 0, mx = 0, my = 0, mb = 0, mh = 0;
        const bool ok = platform::hauptfensterRechteck(x, y, b, h) && platform::bildschirmRechteck(mx, my, mb, mh);
        diag::detail("Knopftest: Stufe 2 - Vollbild " + std::to_string(b) + "x" + std::to_string(h) + " Bildschirm " + std::to_string(mb) + "x" +
                     std::to_string(mh));
        return ok && platform::istVollbild() && x == mx && y == my && b == mb && h == mh;
    }));
    add(tasteKombi(ImGuiKey_F11, "F11"));
    add(pause(0.5));
    add(pruefe("F11: zurueck auf die alte Lage", [rechteck] {
        int x = 0, y = 0, b = 0, h = 0;
        const auto& r = *rechteck;
        return platform::hauptfensterRechteck(x, y, b, h) && !platform::istVollbild() && x == r[0] && y == r[1] && b == r[2] && h == r[3];
    }));
    add(pause(0.5));
    // --- Abreissbare Menues (QTornOffMenu)
    add(klick(ke::trK(Str::KeLMisc), "###main"));
    add(pause(0.3));
    add(klickAn("Abreisskante des Menues Misc", menueOben));
    add(pause(0.3));
    add(pruefe("Abgerissen: ein Fenster \"Misc\" mit den Eintraegen, das Menue zu", [] {
        const ke::Editor& e = ke::editor();
        ImGuiWindow* m = ImGui::FindWindowByName("###Menu_00");
        diag::detail("Knopftest: Stufe 2 - Abrisse " + std::to_string(e.abrisse.size()) + ", Menue " + (m == nullptr ? "-" : (m->Active ? "aktiv" : "zu")) +
                     ", Map Info " + (finde(std::string(ke::trK(Str::KeLMapInfo)) + "###MapInfo", "abriss", 0, true) != nullptr ? "da" : "fehlt"));
        return e.abrisse.size() == 1 && e.abrisse[0].titel == ke::trK(Str::KeLMisc) && ImGui::FindWindowByName("###Menu_00") != nullptr &&
               !ImGui::FindWindowByName("###Menu_00")->Active &&
               finde(std::string(ke::trK(Str::KeLMapInfo)) + "###MapInfo", "abriss", 0, true) != nullptr;
    }));
    add(foto("karteneditor_abgerissen"));
    add(klick(std::string(ke::trK(Str::KeLMapInfo)) + "###MapInfo", "abriss", 0, true));
    add(pause(0.3));
    add(pruefe("Klick im abgerissenen Fenster: Map Info geht auf, das Fenster bleibt", [] {
        return ke::editor().kartenInfo.offen && ke::editor().abrisse.size() == 1;
    }));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(klick(ke::trK(Str::KeLMisc), "###main"));
    add(pause(0.3));
    add(klickAn("Abreisskante noch einmal (toggleTearOff)", menueOben));
    add(pause(0.3));
    add(pruefe("noch einmal: das abgerissene Fenster ist zu", [] { return ke::editor().abrisse.empty(); }));
    add(tu("Einstellungen auf", [] { ke::editor().einstellungenOffen = true; }));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KePgLayout)) + "##seite5"));
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KePrDetachableMenus)) + "##abreissbar"));
    add(klick(ke::trK(Str::KePrefOk)));
    add(pause(0.3));
    add(pruefe("Detachable Menus aus (DetachableMenus 0)", [] { return !ke::editor().menueAbreissbar && einstellung("DetachableMenus") == "0"; }));
    add(klick(ke::trK(Str::KeLMisc), "###main"));
    add(pause(0.3));
    add(klickAn("oben ins Menue Misc", menueOben));
    add(pause(0.3));
    add(pruefe("ohne Abreisskante reisst nichts ab", [] { return ke::editor().abrisse.empty(); }));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(tu("Detachable Menus wieder an", [] {
        ke::editor().menueAbreissbar = true;
        ke::einstellungenSpeichern();
    }));
    // --- Bug report, About
    add(tu("Help > Bug report (OpenURL im Test aufgezeichnet)", [aufgezeichnet] {
        ke::editor().urlOeffnen = [aufgezeichnet](const std::string& u) { *aufgezeichnet = u; };
        (void)ke::ausfuehren("OpenBugReportURL");
    }));
    add(pruefe("Bug report: OpenURL( https://github.com/Garux/netradiant-custom/issues )", [aufgezeichnet] {
        return *aufgezeichnet == "https://github.com/Garux/netradiant-custom/issues";
    }));
    add(tu("Help > About", [] { (void)ke::ausfuehren("DoAbout"); }));
    add(pause(0.4));
    add(pruefe("About: OK, Credits (gesperrt), Changelog, About ...; Direct3D-Werte", [] {
        const render::GeraeteInfo i = render::geraeteInfo();
        diag::detail("Knopftest: Stufe 2 - About " + i.hersteller + " / " + i.version + " / " + i.karte + " / " + std::to_string(i.merkmale.size()) +
                     " Merkmale");
        return ke::editor().ueberOffen && finde(std::string(ke::trK(Str::KePrefOk)) + "##ueberok", "", 0, true) != nullptr &&
               finde(std::string(ke::trK(Str::KeAboutCredits)) + "##uebercredits", "", 0, true) != nullptr && !i.karte.empty() &&
               i.version.find("Direct3D 11") != std::string::npos && !i.merkmale.empty();
    }));
    add(foto("karteneditor_about"));
    add(klick(std::string(ke::trK(Str::KeAboutQt)) + "##ueberqt"));
    add(pause(0.3));
    add(pruefe("About Dear ImGui: Meldung mit der Version (QApplication::aboutQt)", [] {
        return ke::editor().meldung.offen && ke::editor().meldung.text.find(IMGUI_VERSION) != std::string::npos;
    }));
    add(klick(std::string(ke::trK(Str::KePrefOk)) + "##meldungok"));
    add(klick(std::string(ke::trK(Str::KePrefOk)) + "##ueberok"));
    add(pause(0.2));
    add(pruefe("OK: About zu", [] { return !ke::editor().ueberOffen; }));
    // --- Set 2D Background image, Shader Editor Command (Testordner als Extra Resource Path)
    add(tu("Testordner zusatz12: textures/test12/bild.tga (rot), scripts/test12.shader; als Extra Resource Path", [] {
        testDateiSchreiben(laufDatei("zusatz12/textures/test12/bild.tga"), tgaBild(255, 0, 0));
        testDateiSchreiben(laufDatei("zusatz12/scripts/test12.shader"), "textures/test12/bild\n{\n\tqer_editorimage textures/test12/bild.tga\n}\n");
        ke::spielNeuRealisieren("Edit Project Settings", [] {
            ke::editor().zusatzPfade[0] = karte::ordnerBereinigt(laufDatei("zusatz12"));
        });
    }));
    add(pause(0.3));
    add(tu("ohne Auswahl: Set 2D Background image", [] {
        ke::editor().karte.auswahl.leeren();
        ke::hintergrundBildSetzen(0, "textures/test12/bild.tga");
    }));
    add(pruefe("ohne Auswahl: Meldung \"No selection\"", [] {
        return ke::editor().meldung.offen && ke::editor().meldung.titel == "No selection" && ke::editor().xy[0].hintergrund.bild == nullptr;
    }));
    add(klick(std::string(ke::trK(Str::KePrefOk)) + "##meldungok"));
    add(klickXy("M1 auf den Pinsel", ImGuiMouseButton_Left, ImVec2{32, 32}));
    add(tu("mit Auswahl: Set 2D Background image textures/test12/bild.tga", [] { ke::hintergrundBildSetzen(0, "textures/test12/bild.tga"); }));
    add(tu("Auswahl weg (die Auswahlfarbe soll nicht stoeren)", [] { ke::editor().karte.auswahl.leeren(); }));
    add(pause(0.4));
    add(pruefe("Hintergrund: im Kasten der Auswahl rot (BackgroundImage::render), \"Loaded background texture\"", [] {
        const ke::XyFenster& f = ke::editor().xy[0];
        const auto b = f.ansicht.zuBild(karte::Vektor3f(20, 20, 0));
        int rot = 0;   // zwischen den Rasterlinien: wenigstens ein Punkt rein rot
        for (int dy = -3; dy <= 3; ++dy) {
            for (int dx = -3; dx <= 3; ++dx) {
                unsigned char p[4] = {};
                if (ke::xyPixel(0, static_cast<int>(b[0]) + dx, static_cast<int>(b[1]) + dy, p) && p[0] > 230 && p[1] < 30 && p[2] < 30) { ++rot; }
            }
        }
        diag::detail("Knopftest: Stufe 2 - Hintergrund rot " + std::to_string(rot) + " von 49");
        return rot > 10 && f.hintergrund.xmin == -64.0F && f.hintergrund.ymax == 64.0F && konsoleHat("Loaded background texture textures/test12/bild.tga");
    }));
    add(foto("karteneditor_hintergrundbild"));
    add(tu("Interface: Shader Editor Command auf ein Programm, das es nicht gibt (Q_Exec im Test aufgezeichnet)", [aufgezeichnet] {
        ke::editor().einstellungenOffen = true;
        ke::editor().befehlStarten = [aufgezeichnet](const std::string& z) {
            *aufgezeichnet = z;
            return false;
        };
    }));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KePgInterface)) + "##seite4"));
    add(pause(0.2));
    add(klickAn("Feld Shader Editor Command", elementOrt("##shadereditor")));
    add(tippe("bhed_gibtsnicht11.exe"));
    add(foto("karteneditor_pref_interface"));
    add(klick(ke::trK(Str::KePrefOk)));
    add(pause(0.2));
    add(tu("Open shader in External Editor (scripts/test12.shader)", [] { ke::shaderAnsehen("scripts/test12.shader", "textures/test12/bild", true); }));
    add(pruefe("Launching: <Befehl> \"<Pfad>\", gescheitert: \"Failed to execute\" (DoShaderView)", [aufgezeichnet] {
        const std::string pfad = karte::ordnerBereinigt(laufDatei("zusatz12")) + "scripts/test12.shader";
        diag::detail("Knopftest: Stufe 2 - Q_Exec " + *aufgezeichnet);
        return *aufgezeichnet == "bhed_gibtsnicht11.exe \"" + pfad + "\"" && konsoleHat("Launching: bhed_gibtsnicht11.exe") &&
               konsoleHat("Failed to execute bhed_gibtsnicht11.exe") && einstellung("CustomShaderEditorCommand") == "bhed_gibtsnicht11.exe";
    }));
    add(tu("ohne Befehl: Open shader in External Editor", [aufgezeichnet] {
        ke::editor().shaderEditorBefehl.clear();
        aufgezeichnet->clear();
        ke::shaderAnsehen("scripts/test12.shader", "textures/test12/bild", true);
    }));
    add(pruefe("ohne Befehl: ShellExecute( pathFull ) (aufgezeichnet)", [aufgezeichnet] {
        return *aufgezeichnet == karte::ordnerBereinigt(laufDatei("zusatz12")) + "scripts/test12.shader";
    }));
    // --- Engine Path Configuration (EnginePath_verify)
    add(pruefe("EnginePath_verify: mit vorhandenem Engine-Pfad kein Dialog, mit fehlendem der Dialog", [] {
        ke::Editor& e = ke::editor();
        e.enginePfadGeprueft = false;
        ke::enginePfadPruefen();
        const bool vorhanden = !e.enginePfadDialogOffen;
        const std::string alt = e.enginePfad;
        e.enginePfad = karte::ordnerBereinigt(laufDatei("gibtsnicht12"));
        e.enginePfadGeprueft = false;
        ke::enginePfadPruefen();
        e.enginePfad = alt;
        return vorhanden && e.enginePfadDialogOffen;
    }));
    add(pause(0.3));
    add(pruefe("Dialog \"Engine Path Configuration\": Hinweis, Engine Path, OK", [] {
        return finde("##enginepfaddialog", "", 0, true) != nullptr && finde(std::string(ke::trK(Str::KePrefOk)) + "##enginepfadok", "", 0, true) != nullptr;
    }));
    add(foto("karteneditor_engine_path"));
    add(tu("Testordner engine12/base/maps/engine12.txt", [] { testDateiSchreiben(laufDatei("engine12/base/maps/engine12.txt"), "engine12"); }));
    add(klickAn("Feld Engine Path", elementOrt("##enginepfaddialog")));
    add(tippe(laufDatei("engine12")));
    add(klick(std::string(ke::trK(Str::KePrefOk)) + "##enginepfadok"));
    add(pause(0.3));
    add(pruefe("OK: Engine Path engine12 (setEnginePath), maps/engine12.txt im Spiel", [] {
        return !ke::editor().enginePfadDialogOffen && ke::editor().enginePfad == karte::ordnerBereinigt(laufDatei("engine12")) && dateiImSpiel("maps/engine12.txt");
    }));
    // --- OpenGL Font
    add(tu("Misc > Theming > OpenGL Font...", [] { (void)ke::ausfuehren("OpenGLFont"); }));
    add(pause(0.4));
    add(pruefe("Select Font: Familien (Arial dabei), Segoe UI fuer Myriad Pro, Groesse 8", [] {
        diag::detail("Knopftest: Stufe 2 - Schrift " + ke::einstellungsFeld("schriftfamilie") + " " + ke::einstellungsFeld("schriftgroesse"));
        return ke::editor().schriftDialogOffen && ke::einstellungsFeld("schriftfamilie") == "Segoe UI" && ke::einstellungsFeld("schriftgroesse") == "8" &&
               ("\n" + ke::einstellungsFeld("schriftfamilien")).find("\nArial\n") != std::string::npos;
    }));
    add(foto("karteneditor_select_font"));
    add(klickAn("Feld Font", elementOrt("##schriftfamilie")));
    add(tippe("Arial"));
    add(klickAn("Feld Size", elementOrt("##schriftgroesse")));
    add(tippe("12"));
    add(klick(std::string(ke::trK(Str::KePrefOk)) + "##schriftok"));
    add(pause(0.6));
    add(pruefe("OK: OpenGLFont Arial 12, die Schrift der Ansichten neu gebaut (glfont_create)", [] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Stufe 2 - OpenGL-Schrift " + e.glSchriftGeladen + " " + std::to_string(ke::glSchriftPx()));
        std::string datei = e.glSchriftGeladen;
        for (char& c : datei) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        return e.glSchriftName == "Arial" && e.glSchriftGroesse == 12 && einstellung("OpenGLFont") == "Arial" && einstellung("OpenGLFontSize") == "12" &&
               ke::glSchrift() != ke::schrift() && datei.find("arial.ttf") != std::string::npos && ke::glSchriftPx() > 17.0F && ke::glSchriftPx() < 19.0F;
    }));
    add(foto("karteneditor_opengl_schrift"));
    add(tu("Schrift, Spiel und Hooks zurueck", [] {
        ke::Editor& e = ke::editor();
        ke::glSchriftSetzen("Myriad Pro", 8);
        e.urlOeffnen = nullptr;
        e.befehlStarten = nullptr;
        e.shaderEditorBefehl.clear();
        if (e.xy[0].hintergrund.bild != nullptr) {
            kartegpu::bildFreigeben(e.xy[0].hintergrund.bild);
            e.xy[0].hintergrund = ke::XyFenster::Hintergrund{};
        }
        ke::spielNeuRealisieren("Edit Project Settings", [] {
            ke::editor().enginePfad.clear();
            ke::editor().zusatzPfade = {};
            ke::einstellungenSpeichern();
        });
    }));
    add(pause(0.6));
    add(pruefe("zurueck: Kartenschrift, Spielordner des Programms", [] {
        const ke::Editor& e = ke::editor();
        return ke::glSchrift() == ke::schrift() && e.enginePfad.empty() && e.spiel && e.spiel->klassen.finden("info_player_start") != nullptr &&
               !dateiImSpiel("maps/engine12.txt");
    }));
    return s;
}

// Ein Pinsel (min..max) mit einem Shader in worldspawn (oder in "ent")
std::shared_ptr<karte::Pinsel> testPinsel(const karte::Vektor3f& mn, const karte::Vektor3f& mx, const std::string& shader,
                                          const std::shared_ptr<karte::Entitaet>& ent = nullptr) {
    ke::Editor& e = ke::editor();
    karte::TexturProjektion proj;
    karte::texdefStandard(e.karte.kontext.art(), proj);
    auto p = std::make_shared<karte::Pinsel>();
    p->quaderBauen(karte::kastenAusMinMax(mn, mx), shader, proj, e.karte.kontext);
    (ent ? ent : e.karte.welt())->kindEinfuegen(p);
    return p;
}

std::vector<karte::Vektor3f> patchEcken(const std::shared_ptr<karte::Patch>& p) {
    std::vector<karte::Vektor3f> v;
    if (p) {
        for (const karte::PatchPunkt& q : p->punkte()) { v.push_back(q.vertex); }
    }
    return v;
}

// Punkt 12: die Wirkung jedes Befehls der Stufe 2, der in den Schritten oben
// nicht ueber ausfuehren/Taste/Menue lief (dort pruefte der Kern oder ein
// anderer Weg die Wirkung): je ein Ausloesen ueber ausfuehren mit Pruefung.
std::vector<Schritt> befehlsWirkungen() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto rueck = [&]() { add(tu("Rueckgaengig", [] { (void)ke::ausfuehren("Undo"); })); };
    const auto pinselWaehlen = [&]() {
        add(tu("den Pinsel waehlen", [] {
            ke::Editor& e = ke::editor();
            e.karte.auswahl.leeren();
            e.karte.auswahl.waehlen(e.karte.welt(), pinsel());
            ke::auswahlGeaendert();
        }));
    };
    add(grundzustand("Stufe 2/12: Grundzustand (Befehlswirkungen)"));
    // --- Brush > N sided (BrushMakeSided::set): Prisma mit N Seiten um die Blickachse
    for (const int n : {3, 4, 6, 7, 8, 9}) {
        pinselWaehlen();
        const std::string name = "Brush" + std::to_string(n) + "Sided";
        add(tu("Brush > " + std::to_string(n) + " sided", [name] { (void)ke::ausfuehren(name); }));
        add(pruefe(name + ": " + std::to_string(n + 2) + " Flaechen, \"brushPrism -sides " + std::to_string(n) + " -axis 2\"", [n] {
            return beitragend() == static_cast<std::size_t>(n + 2) &&
                   ke::editor().karte.undo.undoName() == "brushPrism -sides " + std::to_string(n) + " -axis 2";
        }));
        rueck();
    }
    // --- Drehen und Spiegeln: zwei Pinsel A (0 0 0)..(64 32 16), B (64 0 0)..(96 64 32)
    add(tu("Pinsel A und B dazu, beide waehlen", [] {
        ke::Editor& e = ke::editor();
        const auto a = testPinsel(karte::Vektor3f(0, 0, 0), karte::Vektor3f(64, 32, 16), "textures/common/caulk");
        const auto b = testPinsel(karte::Vektor3f(64, 0, 0), karte::Vektor3f(96, 64, 32), "textures/common/caulk");
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), a);
        e.karte.auswahl.waehlen(e.karte.welt(), b);
        ke::auswahlGeaendert();
    }));
    const auto ausdehnung = [](float x, float y, float z) {
        return [x, y, z] {
            const karte::Kasten k = grenzenVon(1);
            return std::fabs(k.extents[0] - x) < 0.01F && std::fabs(k.extents[1] - y) < 0.01F && std::fabs(k.extents[2] - z) < 0.01F;
        };
    };
    for (const auto& [name, x, y, z] : {std::tuple<const char*, float, float, float>{"RotateSelectionX", 32.0F, 8.0F, 16.0F},
                                        {"RotateSelectionY", 8.0F, 16.0F, 32.0F}, {"RotateSelectionZ", 16.0F, 32.0F, 8.0F},
                                        {"RotateSelectionAnticlockwise", 16.0F, 32.0F, 8.0F}}) {
        const std::string n = name;
        add(tu(n, [n] { (void)ke::ausfuehren(n); }));
        add(pruefe(n + ": A um 90 Grad gedreht (Ausdehnung " + std::to_string(static_cast<int>(x)) + " " + std::to_string(static_cast<int>(y)) + " " +
                       std::to_string(static_cast<int>(z)) + ")",
                   ausdehnung(x, y, z)));
        rueck();
    }
    for (const auto& [name, mn, mx] : {std::tuple<const char*, karte::Vektor3f, karte::Vektor3f>{"MirrorSelectionX", {32, 0, 0}, {96, 32, 16}},
                                       {"MirrorSelectionY", {0, 32, 0}, {64, 64, 16}}, {"MirrorSelectionZ", {0, 0, 16}, {64, 32, 32}},
                                       {"MirrorSelectionVertically", {0, 32, 0}, {64, 64, 16}}}) {
        const std::string n = name;
        const karte::Vektor3f a = mn;
        const karte::Vektor3f b = mx;
        add(tu(n, [n] { (void)ke::ausfuehren(n); }));
        add(pruefe(n + ": A um die Mitte der Auswahl gespiegelt", [a, b] { return kastenGleich(grenzenVon(1), a, b); }));
        rueck();
    }
    add(grundzustand("Stufe 2/12: Grundzustand (Patches)"));
    // --- Patches aus der Auswahl (Patch_Bevel, _EndCap, _SquareCylinder, _Cone, _Sphere)
    for (const auto& [name, undo] : {std::pair<const char*, const char*>{"PatchBevel", "patchCreateBevel"}, {"PatchEndCap", "patchCreateEndCap"},
                                     {"PatchSquareCylinder", "patchCreateSquareCylinder"}, {"PatchCone", "patchCreateCone"},
                                     {"PatchSphere", "patchCreateSphere"}}) {
        const std::string n = name;
        const std::string u = undo;
        pinselWaehlen();
        add(tu(n, [n] { (void)ke::ausfuehren(n); }));
        add(pruefe(n + ": ein Patch im Kasten der Auswahl, gewaehlt (\"" + u + "\")", [u] {
            const auto p = patchNr(0);
            return patchZahl() == 1 && p && ke::editor().karte.undo.undoName() == u && ke::editor().karte.auswahl.anzahl() == 1 &&
                   std::fabs(p->grenzen().origin[0]) < 0.01F && p->grenzen().extents[0] > 32.0F;
        }));
        rueck();
    }
    for (const auto& [name, undo] : {std::pair<const char*, const char*>{"PatchXactCone", "patchCreateXactCone"}, {"PatchXactSphere", "patchCreateXactSphere"}}) {
        const std::string n = name;
        const std::string u = undo;
        pinselWaehlen();
        add(tu(n + " (Dialog)", [n] { (void)ke::ausfuehren(n); }));
        add(pause(0.3));
        add(klickAn("OK", elementOrt(std::string(ke::trK(Str::KePrefOk)) + "##patchok")));
        add(pruefe(n + ": exakter Patch (\"" + u + "\")", [u] { return patchZahl() == 1 && ke::editor().karte.undo.undoName() == u; }));
        rueck();
    }
    // --- Zeilen und Spalten an einer 3 x 3-Ebene
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_P, "Umschalt+P"));
    add(pause(0.3));
    add(tasteKombi(ImGuiKey_Enter, "Return"));
    add(pruefe("Ebene 3 x 3 gewaehlt", [] { return patchZahl() == 1 && patchNr(0)->breite() == 3 && patchNr(0)->hoehe() == 3; }));
    for (const auto& [name, b, h] : {std::tuple<const char*, int, int>{"PatchInsertLastColumn", 5, 3}, {"PatchDeleteLastColumn", 3, 3},
                                     {"PatchInsertFirstRow", 3, 5}, {"PatchInsertLastRow", 3, 7}, {"PatchDeleteFirstRow", 3, 5},
                                     {"PatchDeleteLastRow", 3, 3}, {"PatchInsertLastColumn", 5, 3}, {"PatchDeleteFirstColumn", 3, 3}}) {
        const std::string n = name;
        add(tu(n, [n] { (void)ke::ausfuehren(n); }));
        add(pruefe(n + ": " + std::to_string(b) + " x " + std::to_string(h), [b, h] {
            return patchNr(0)->breite() == static_cast<std::size_t>(b) && patchNr(0)->hoehe() == static_cast<std::size_t>(h);
        }));
    }
    add(tu("auf 5 x 5, einen Punkt verschieben", [] {
        (void)ke::ausfuehren("PatchInsertLastColumn");
        (void)ke::ausfuehren("PatchInsertLastRow");
        const auto p = patchNr(0);
        std::vector<karte::PatchPunkt> q = p->punkte();
        q[6].vertex = q[6].vertex + karte::Vektor3f(20, 10, 0);
        p->punkteSetzen(q);
        p->punkteGeaendert();
    }));
    auto vorher = std::make_shared<std::vector<karte::Vektor3f>>();
    auto stVorher = std::make_shared<karte::Vektor2f>();
    for (const char* name : {"RedisperseRows", "RedisperseCols", "SmoothRows", "SmoothCols"}) {
        const std::string n = name;
        add(tu(n, [n, vorher] {
            *vorher = patchEcken(patchNr(0));
            (void)ke::ausfuehren(n);
        }));
        add(pruefe(n + ": Kontrollpunkte neu verteilt bzw. geglaettet", [vorher] { return patchEcken(patchNr(0)) != *vorher; }));
        rueck();
    }
    add(tu("InvertCurveTextureX", [stVorher] {
        *stVorher = patchNr(0)->punkt(0, 0).texcoord;
        (void)ke::ausfuehren("InvertCurveTextureX");
    }));
    add(pruefe("InvertCurveTextureX: s gespiegelt (\"patchFlipTextureU\")", [stVorher] {
        return patchNr(0)->punkt(0, 0).texcoord[0] != (*stVorher)[0] && ke::editor().karte.undo.undoName() == "patchFlipTextureU";
    }));
    add(grundzustand("Stufe 2/12: Grundzustand (Texturen)"));
    // --- Fit, Shift, Scale, Rotate (Surface Inspector-Befehle an der ganzen Auswahl)
    auto ssr = std::make_shared<karte::Texdef>();
    const auto texturBefehl = [&](const std::string& n, std::function<bool(const karte::Texdef&, const karte::Texdef&)> pruef, const std::string& text) {
        pinselWaehlen();
        add(tu(n, [n, ssr] {
            *ssr = ersteFlaecheSsr();
            (void)ke::ausfuehren(n);
        }));
        add(pruefe(n + ": " + text, [ssr, pruef] { return pruef(*ssr, ersteFlaecheSsr()); }));
        rueck();
    };
    const auto anders = [](float a, float b) { return std::fabs(a - b) > 1e-4F; };
    texturBefehl("FitTextureWidth", [anders](const karte::Texdef& a, const karte::Texdef& b) { return anders(a.scale[0], b.scale[0]); }, "Breite eingepasst");
    texturBefehl("FitTextureHeight", [anders](const karte::Texdef& a, const karte::Texdef& b) { return anders(a.scale[1], b.scale[1]); }, "Hoehe eingepasst");
    texturBefehl("FitTextureWidthOnly", [anders](const karte::Texdef& a, const karte::Texdef& b) { return anders(a.scale[0], b.scale[0]) && !anders(a.scale[1], b.scale[1]); },
                 "nur die Breite");
    texturBefehl("FitTextureHeightOnly", [anders](const karte::Texdef& a, const karte::Texdef& b) { return anders(a.scale[1], b.scale[1]) && !anders(a.scale[0], b.scale[0]); },
                 "nur die Hoehe");
    for (const char* n : {"TexShiftLeft", "TexShiftRight"}) {
        texturBefehl(n, [anders](const karte::Texdef& a, const karte::Texdef& b) { return anders(a.shift[0], b.shift[0]) && !anders(a.shift[1], b.shift[1]); },
                     "waagerecht verschoben");
    }
    for (const char* n : {"TexShiftUp", "TexShiftDown"}) {
        texturBefehl(n, [anders](const karte::Texdef& a, const karte::Texdef& b) { return anders(a.shift[1], b.shift[1]) && !anders(a.shift[0], b.shift[0]); },
                     "senkrecht verschoben");
    }
    for (const char* n : {"TexScaleLeft", "TexScaleRight"}) {
        texturBefehl(n, [anders](const karte::Texdef& a, const karte::Texdef& b) { return anders(a.scale[0], b.scale[0]) && !anders(a.scale[1], b.scale[1]); },
                     "waagerecht skaliert");
    }
    for (const char* n : {"TexScaleUp", "TexScaleDown"}) {
        texturBefehl(n, [anders](const karte::Texdef& a, const karte::Texdef& b) { return anders(a.scale[1], b.scale[1]) && !anders(a.scale[0], b.scale[0]); },
                     "senkrecht skaliert");
    }
    for (const char* n : {"TexRotateClock", "TexRotateCounter"}) {
        texturBefehl(n, [](const karte::Texdef& a, const karte::Texdef& b) {
            const float d = std::fmod(std::fabs(b.rotate - a.rotate), 360.0F);
            return std::fabs(d - 45.0F) < 0.01F || std::fabs(d - 315.0F) < 0.01F;
        }, "um 45 Grad gedreht");
    }
    // --- Texture Project: an Axial-Projection-Pinseln nur die Warnung, an Brush Primitives die Projektion
    pinselWaehlen();
    add(tu("TextureProjectAxial (Axial Projection)", [] { (void)ke::ausfuehren("TextureProjectAxial"); }));
    add(pruefe("Axial Projection: Warnung wie NRC, \"textureProjectAxial\"", [] {
        return konsoleHat("function doesn't work for *brushes*, having Axial Projection type") &&
               ke::editor().karte.undo.undoName() == "textureProjectAxial";
    }));
    rueck();
    add(tu("Brush Primitives: New Map, ein Pinsel, gewaehlt, Textur gedreht", [] {
        ke::Editor& e = ke::editor();
        e.pinselTyp = 1;
        ke::neueKarte();
        e.pinselTyp = 0;
        auto w = e.karte.weltHolenOderAnlegen();
        const auto p = testPinsel(karte::Vektor3f(-64, -64, 0), karte::Vektor3f(64, 64, 64), "textures/common/caulk");
        e.karte.auswahl.waehlen(w, p);
        ke::auswahlGeaendert();
        (void)ke::ausfuehren("TexRotateClock");
    }));
    for (const auto& [name, undo] : {std::pair<const char*, const char*>{"TextureProjectAxial", "textureProjectAxial"},
                                     {"TextureProjectOrtho", "textureProjectOrtho"}, {"TextureProjectCam", "textureProjectCam"}}) {
        const std::string n = name;
        const std::string u = undo;
        auto bp = std::make_shared<std::string>();
        add(tu(n + " (Brush Primitives)", [n, bp] {
            *bp = karte::karteSchreiben(ke::editor().karte);
            (void)ke::ausfuehren(n);
        }));
        add(pruefe(n + ": die Projektion der Flaechen neu (\"" + u + "\")", [u, bp] {
            return ke::editor().karte.kontext.format == karte::PinselFormat::Quake3BP && karte::karteSchreiben(ke::editor().karte) != *bp &&
                   ke::editor().karte.undo.undoName() == u;
        }));
        rueck();
    }
    add(grundzustand("Stufe 2/12: Grundzustand (Auswahl, Dateien)"));
    // --- SelectTextured (Select_FacesAndPatchesByShader_)
    add(tu("Browser-Shader common/caulk, Edit > Select All Faces with this Texture", [] {
        ke::editor().karte.auswahl.leeren();
        ke::editor().bearbeitung.browserShader = "textures/common/caulk";
        (void)ke::ausfuehren("SelectTextured");
    }));
    add(pruefe("SelectTextured: die 6 Flaechen des Pinsels gewaehlt", [] { return ke::editor().karte.auswahl.flaechen().size() == 6; }));
    add(tu("Auswahl weg", [] {
        ke::editor().karte.auswahl.leeren();
        ke::auswahlGeaendert();
    }));
    // --- Save selected, Import, Save region ueber die Dateiwahl (im Test ersetzt)
    add(tu("das Licht waehlen, File > Save selected... (Dateiwahl: s12_auswahl.map)", [] {
        ke::Editor& e = ke::editor();
        e.dateiDialog = [](const std::string&) { return laufDatei("s12_auswahl.map"); };
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(lichtEntitaet());
        (void)ke::ausfuehren("SaveSelected");
    }));
    add(pruefe("SaveSelected: s12_auswahl.map mit dem Licht", [] {
        karte::Karte k;
        (void)karte::karteLesen(karte::textmodusLesen(dateiText(laufDatei("s12_auswahl.map"))), k);
        return k.entitaeten().size() == 1 && k.entitaeten().front()->klassenname() == "light";
    }));
    add(tu("File > Import... (Dateiwahl: s12_auswahl.map)", [] { (void)ke::ausfuehren("ImportMap"); }));
    add(pruefe("ImportMap: ein zweites Licht (\"mapImport\")", [] {
        return ke::editor().karte.entitaeten().size() == 3 && ke::editor().karte.undo.undoName() == "mapImport";
    }));
    rueck();
    add(tu("den Pinsel waehlen, Region > Set selection, File > Save region... (s12_region.map)", [] {
        ke::Editor& e = ke::editor();
        e.dateiDialog = [](const std::string&) { return laufDatei("s12_region.map"); };
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), pinsel());
        (void)ke::ausfuehren("RegionSetSelection");
        (void)ke::ausfuehren("SaveRegion");
        (void)ke::ausfuehren("RegionOff");
        e.dateiDialog = nullptr;
    }));
    add(pruefe("SaveRegion: s12_region.map mit Startpunkt und den Waenden", [] {
        const std::string t = dateiText(laufDatei("s12_region.map"));
        return t.find("info_player_start") != std::string::npos && t.find("// brush 6") != std::string::npos;
    }));
    // --- Filter: je ein Gegenstand, der gefiltert wird
    auto ziele = std::make_shared<std::map<std::string, std::shared_ptr<karte::Primitiv>>>();
    auto zielEnt = std::make_shared<std::map<std::string, std::shared_ptr<karte::Entitaet>>>();
    add(tu("Filterziele: Pinsel je Shader, Entitaeten info_player_start, path_corner, trigger_multiple", [ziele, zielEnt] {
        ke::Editor& e = ke::editor();
        auto& z = *ziele;
        float x = 200;
        const auto dazu = [&](const char* filter, const std::string& shader) {
            z[filter] = testPinsel(karte::Vektor3f(x, 0, 0), karte::Vektor3f(x + 16, 16, 16), shader);
            x += 32;
        };
        dazu("FilterClips", "textures/common/clip");
        dazu("FilterBotClips", "textures/common/botclip");
        dazu("FilterLiquids", "textures/liquids/wasser12");
        dazu("FilterHintsSkips", "textures/common/hint");
        dazu("FilterClusterportals", "textures/common/clusterportal");
        dazu("FilterAreaportals", "textures/common/areaportal");
        dazu("FilterLightgrid", "textures/common/lightgrid");
        dazu("FilterDecals", "textures/decals/test12");
        // Sky und Translucent: ein Shader des Spiels mit QER_SKY bzw. QER_TRANS
        std::string himmel;
        std::string durch;
        for (const karte::ShaderEintrag* sh : e.karteSpiel->inventar.shader()) {
            if (himmel.empty() && (sh->flags & karte::QER_SKY) != 0U) { himmel = sh->name; }
            if (durch.empty() && (sh->flags & (karte::QER_TRANS | karte::QER_ALPHATEST)) != 0U && (sh->flags & karte::QER_SKY) == 0U) { durch = sh->name; }
        }
        dazu("FilterSky", himmel);
        dazu("FilterTranslucent", durch);
        dazu("FilterStructural", "textures/common/caulk");
        z["FilterWorldBrushes"] = pinsel();
        auto& ze = *zielEnt;
        const auto ent = [&](const char* filter, const char* klasse) {
            auto n = e.karte.neueEntitaet(klasse, false);
            n->setzen("origin", "0 300 0");
            e.karte.entitaetEinfuegen(n);
            ze[filter] = n;
            return n;
        };
        ent("FilterPointEntities", "info_player_start");
        ent("FilterPaths", "path_corner");
        auto trig = e.karte.neueEntitaet("trigger_multiple", true);
        e.karte.entitaetEinfuegen(trig);
        (void)testPinsel(karte::Vektor3f(0, 400, 0), karte::Vektor3f(32, 432, 32), "textures/common/trigger", trig);
        ze["FilterTriggers"] = trig;
        ze["FilterEntities"] = lichtEntitaet();
        ze["FilterWorldBrushes"] = e.karte.welt();   // filter_entity_world: worldspawn samt Kindern
    }));
    add(pause(0.2));
    for (const char* f : {"FilterClips", "FilterBotClips", "FilterLiquids", "FilterHintsSkips", "FilterClusterportals", "FilterAreaportals",
                          "FilterLightgrid", "FilterDecals", "FilterSky", "FilterTranslucent", "FilterStructural"}) {
        const std::string n = f;
        add(tu(n + " an", [n] { (void)ke::ausfuehren(n); }));
        add(pause(0.15));
        add(pruefe(n + ": der Pinsel ist gefiltert, der Haken an", [n, ziele] {
            const auto p = (*ziele)[n];
            return p && !p->sichtbar() && ke::angehakt(n);
        }));
        add(tu(n + " aus", [n] { (void)ke::ausfuehren(n); }));
        add(pause(0.15));
        add(pruefe(n + " aus: wieder sichtbar", [n, ziele] { return (*ziele)[n] && (*ziele)[n]->sichtbar() && !ke::angehakt(n); }));
    }
    for (const char* f : {"FilterPointEntities", "FilterPaths", "FilterTriggers", "FilterEntities", "FilterWorldBrushes"}) {
        const std::string n = f;
        add(tu(n + " an", [n] { (void)ke::ausfuehren(n); }));
        add(pause(0.15));
        add(pruefe(n + ": die Entitaet ist gefiltert, der Haken an", [n, zielEnt] {
            const auto e = (*zielEnt)[n];
            return e && !e->sichtbar() && ke::angehakt(n);
        }));
        add(tu(n + " aus", [n] { (void)ke::ausfuehren(n); }));
        add(pause(0.15));
        add(pruefe(n + " aus: wieder sichtbar", [n, zielEnt] { return (*zielEnt)[n] && (*zielEnt)[n]->sichtbar() && !ke::angehakt(n); }));
    }
    // --- Tags: Copy, Paste, Rename, Show untagged
    add(tu("Tags GUI an, Tag Stein an common/caulk, caulk im Browser", [] {
        ke::Editor& e = ke::editor();
        if (!e.tags.gui) { (void)ke::ausfuehren("TagsToggleGui"); }
        if (!e.tags.datei.hatShader("textures/common/caulk")) {   // eintragAnlegen
            (void)e.tags.datei.shaderDazu("textures/common/caulk", karte::TagTexturArt::Eigen, karte::TagKnotenArt::Shader);
        }
        (void)e.tags.datei.tagDazu("textures/common/caulk", "Stein");
        e.tags.alle.insert("Stein");
        ke::texturShaderWaehlen("textures/common/caulk");
        (void)ke::ausfuehren("TagCopy");
    }));
    add(pruefe("TagCopy: die Tags von common/caulk gemerkt", [] {
        const auto& k = ke::editor().tags.kopiert;
        return k.size() == 1 && k[0] == "Stein";
    }));
    add(tu("anderen Shader waehlen (common/clip), TagPaste", [] {
        ke::texturShaderWaehlen("textures/common/clip");
        (void)ke::ausfuehren("TagPaste");
    }));
    add(pruefe("TagPaste: common/clip hat Stein", [] { return ke::editor().tags.datei.hatTag("textures/common/clip", "Stein"); }));
    add(tu("Stein markieren, TagRename", [] {
        ke::editor().tags.reiter = 1;
        ke::editor().tags.markiert = {"Stein"};
        (void)ke::ausfuehren("TagRename");
    }));
    add(pause(0.3));
    add(pruefe("TagRename: Stein wird bearbeitet", [] { return ke::editor().tags.bearbeitet == "Stein"; }));
    add(tippe("Fels"));
    add(tasteKombi(ImGuiKey_Enter, "Return"));
    add(pruefe("Return: Fels statt Stein, an beiden Shadern", [] {
        const auto& t = ke::editor().tags;
        return t.alle.count("Fels") != 0U && t.alle.count("Stein") == 0U && t.datei.hatTag("textures/common/caulk", "Fels") &&
               t.datei.hatTag("textures/common/clip", "Fels");
    }));
    add(tu("TagSearchUntagged", [] { (void)ke::ausfuehren("TagSearchUntagged"); }));
    add(pruefe("Show Untagged: die Warnung wie NRC", [] { return ke::editor().meldung.offen && ke::editor().meldung.titel == "Show Untagged"; }));
    add(klick(std::string(ke::trK(Str::KeYes)) + "##meldungja"));
    add(pause(0.2));
    add(pruefe("Yes: der Browser zeigt die Shader ohne Tag (\"Untagged\")", [] {
        const auto& tb = ke::editor().texBro;
        return tb.tagSuche && tb.verzeichnis == "Untagged" && tb.gefundeneShader.count("textures/common/caulk") == 0U;
    }));
    add(tu("Tags weg, Tags GUI aus, Show All", [] {
        ke::Editor& e = ke::editor();
        e.tags.datei.tagUeberallWeg("Fels");
        e.tags.alle.erase("Fels");
        e.tags.markiert.clear();
        (void)ke::ausfuehren("TagsToggleGui");
        (void)ke::ausfuehren("ShowAllTextures");
    }));
    return s;
}

// Punkt 12: was in Stufe 1b offen blieb, weil es eine Dateiwahl von Windows
// oeffnet oder nur in anderen Anordnungen da ist - die versteckten Knoepfe Open,
// Save, Change views, die Eintraege 2 bis 9 der zuletzt-Liste, Texture Browser
// (Menue und Knopf in Split).
std::vector<Schritt> stufe1Reste() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto knopfOrt = [](const std::string& id) {
        return [id]() {
            const Element* el = finde(id, "", 0, true);
            return el != nullptr ? el->rect.GetCenter() : ImVec2{0, 0};
        };
    };
    add(grundzustand("Stufe 2/12: Grundzustand (Reste der Stufe 1b)"));
    add(tu("Knoepfe Open, Save, Change views zeigen (ToolbarHiddenButtons), Kopie s12_knopf.map, Dateiwahl ersetzt", [] {
        ke::Editor& e = ke::editor();
        e.versteckteKnoepfe.erase("OpenMap");
        e.versteckteKnoepfe.erase("SaveMap");
        e.versteckteKnoepfe.erase("NextView");
        (void)spewDatei(laufDatei("s12_knopf.map"), dateiText(karteTestPfad("karte_kopie.map")));
        e.dateiDialog = [](const std::string&) { return laufDatei("s12_knopf.map"); };
    }));
    add(pause(0.3));
    add(klickAn("Knopf Open", knopfOrt("##knopfOpenMap")));
    add(pause(0.3));
    add(pruefe("Knopf Open: s12_knopf.map offen (Dateiwahl)", [] { return ke::editor().name == laufDatei("s12_knopf.map"); }));
    add(tu("Karte aendern (worldspawn message)", [] {
        ke::Editor& e = ke::editor();
        karte::Befehl b(e.karte.undo, "entitySetKeyValue -key \"message\" -value \"knopf12\"");
        e.karte.welt()->setzen("message", "knopf12");
    }));
    add(klickAn("Knopf Save", knopfOrt("##knopfSaveMap")));
    add(pause(0.2));
    add(pruefe("Knopf Save: gespeichert, die Datei hat knopf12", [] {
        return !ke::geaendert() && dateiText(laufDatei("s12_knopf.map")).find("knopf12") != std::string::npos;
    }));
    add(klickAn("Knopf Change views", knopfOrt("##knopfNextView")));
    add(pruefe("Knopf Change views: XY -> XZ", [] { return ke::editor().xy[0].ansicht.typ == karte::Sicht::XZ; }));
    add(tu("Ansicht XY, Knoepfe wieder versteckt", [] {
        ke::Editor& e = ke::editor();
        e.xy[0].ansicht.typ = karte::Sicht::XY;
        for (const char* n : {"OpenMap", "SaveMap", "NextView"}) { e.versteckteKnoepfe.insert(n); }
        e.dateiDialog = nullptr;
    }));
    // zuletzt geoeffnet: 9 Eintraege
    add(tu("neun Karten s12_mru1..9.map in die zuletzt-Liste (1 oben)", [] {
        const std::string text = dateiText(karteTestPfad("karte_kopie.map"));
        for (int i = 9; i >= 1; --i) {
            const std::string p = laufDatei(("s12_mru" + std::to_string(i) + ".map").c_str());
            (void)spewDatei(p, text);
            ke::zuletztEintragen(p);
        }
    }));
    for (int i = 2; i <= 9; ++i) {
        add(klick(ke::trK(Str::KeLFile), "###main"));
        add(pause(0.2));
        add({"zuletzt geoeffnet: Eintrag " + std::to_string(i), [i](int b) {
                 static Schritt k;
                 if (b == 0) { k = klick(karte::ohneMnemonik(ke::editor().zuletzt.beschriftung(static_cast<std::size_t>(i - 1)))); }
                 return k.tun(b);
             }});
        add(pause(0.3));
        // MRU_AddFile beim Oeffnen: der Eintrag rueckt nach oben
        add(pruefe("Eintrag " + std::to_string(i) + ": s12_mru" + std::to_string(i) + ".map offen", [i] {
            const std::string p = laufDatei(("s12_mru" + std::to_string(i) + ".map").c_str());
            return ke::editor().name == p && ke::editor().zuletzt.pfade()[0] == p;
        }));
        add(tu("die Liste wieder 1..9", [] {
            for (int k = 9; k >= 1; --k) { ke::zuletztEintragen(laufDatei(("s12_mru" + std::to_string(k) + ".map").c_str())); }
        }));
    }
    // Texture Browser: Menue und Knopf in Split
    add(tu("Anordnung Split", [] {
        ke::editor().anordnung = karte::befehle::Anordnung::Split;
        ke::editor().teilerFuer = -1;
        ke::editor().gruppenDialogOffen = false;
        ke::editor().versteckteKnoepfe.erase("ToggleTextures");
    }));
    add(pause(0.4));
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.2));
    add(klick(ke::trK(Str::KeLTextureBrowser), "", 0, true));
    add(pause(0.3));
    add(pruefe("View > Texture Browser (Split): Gruppendialog mit dem Texturbrowser", [] {
        return ke::editor().gruppenDialogOffen && ke::editor().gruppenSeite == 2;
    }));
    add(tu("Gruppendialog zu", [] { ke::editor().gruppenDialogOffen = false; }));
    add(pause(0.2));
    add(klickAn("Knopf Texture Browser", knopfOrt("##knopfToggleTextures")));
    add(pause(0.3));
    add(pruefe("Knopf Texture Browser (Split): Texturbrowser", [] { return ke::editor().gruppenDialogOffen && ke::editor().gruppenSeite == 2; }));
    add(tu("zurueck: Regular, Knopf versteckt, Gruppendialog zu", [] {
        ke::Editor& e = ke::editor();
        e.anordnung = karte::befehle::Anordnung::Regular;
        e.teilerFuer = -1;
        e.gruppenDialogOffen = false;
        e.gruppenSeite = 0;
        e.versteckteKnoepfe.insert("ToggleTextures");
    }));
    add(pause(0.3));
    return s;
}

// Punkt 12: jeder Befehl der Stufe 2 wurde in den Schritten oben (Taste, Menue,
// Knopf oder ausfuehren) ausgeloest - die Wirkung pruefen die Schritte dort.
std::vector<Schritt> alleBefehle() {
    std::vector<Schritt> s;
    s.push_back(pruefe("Stufe 2: jeder neue Befehl wurde ausgeloest (befehle::stufe2)", [] {
        std::string fehlt;
        int n = 0;
        for (const std::string& b : karte::befehle::stufe2()) {
            ++n;
            if (ke::editor().befehleAusgefuehrt.count(b) == 0) { fehlt += " " + b; }
        }
        diag::detail("Knopftest: Stufe 2 - " + std::to_string(n) + " Befehle, nicht ausgeloest:" + (fehlt.empty() ? std::string(" -") : fehlt));
        return n > 100 && fehlt.empty();
    }));
    return s;
}

}  // namespace stufe2

