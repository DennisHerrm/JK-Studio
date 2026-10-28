// app_ablage.inc - aus gui/app.cpp geschnitten (Stufe 8d Punkt 5, unveraendert): Zwischenablage, Dateigrenzen, Berichte, Himmelsshader
//
// Eingebunden an genau dieser Stelle von gui/app.cpp (im selben Namensraum, mit allen Helfern davor);
// keine eigene Uebersetzungseinheit - der vorverarbeitete Text ist derselbe wie vor dem Schnitt.
// Abschnitte: Zwischenablage wie im Original; Ueber Dateigrenzen hinweg sichtbar; Ein Bericht, den man anhaengt; Die Wolkenschichten eines Himmelsshaders (rc592)


// Der Zugang von aussen - siehe app_internal.h.
//
// MUSS ausserhalb des namenlosen Namensraums stehen. Darin waere die
// Funktion nur in dieser Datei sichtbar, und app_view3d.cpp bekaeme sie
// beim Binden nicht - genau das hat lint_link.py gerade gemeldet, bevor
// der Binder es tat. Derselbe Fallstrick wie bei loadMapFromArchive.
//
// Setzt auch den Arbeitsbereich: wer von aussen einen Reiter waehlt, meint
// ihn als Ganzes und nicht nur das bearbeitete Skript.
void activateTab(int index) {
    switchTab(index);
    g_app->homeTab = index;
}

// Fuer den Selbsttest (gui/selbsttest.cpp): derselbe Weg wie ein Klick in
// die Ereignisliste bzw. wie "Open" nach dem Dateidialog.
void fuegeBefehlEin(const Command& c) { insertCommand(c); }
void ladeSkriptDatei(const std::string& pfad) { loadPath(pfad); }

// Von einer zweiten Instanz weitergereicht (gui/main_win32.cpp,
// WM_COPYDATA). Gemerkt und im naechsten Bild geoeffnet - nicht mitten in
// der Nachrichtenverarbeitung, und nicht, solange eine Frage offen ist.
std::vector<std::string>& dateienVonAussen() {
    static std::vector<std::string> liste;
    return liste;
}
void dateiVonAussen(const std::string& pfad) {
    dateienVonAussen().push_back(pfad);
    diag::info("Von einer zweiten Instanz: " + (pfad.empty() ? std::string("(nur nach vorn)") : pfad));
}
// Stufe 9 Punkt 3: aufs Fenster gezogene Dateien - im Karten-Editor werden Modelle
// (.obj/.fbx/.glb/.gltf) eingesetzt, alles andere geht wie eine Datei von aussen auf.
struct AbgelegteDatei {
    std::string pfad;
    float x = 0;
    float y = 0;
};
std::vector<AbgelegteDatei>& abgelegteDateien() {
    static std::vector<AbgelegteDatei> liste;
    return liste;
}
void dateiAbgelegt(const std::string& pfad, float x, float y) {
    abgelegteDateien().push_back({pfad, x, y});
    diag::info("Abgelegt: " + pfad);
}
void abgelegteDateienVerarbeiten() {
    if (g_app == nullptr || abgelegteDateien().empty()) { return; }
    const std::vector<AbgelegteDatei> liste = std::move(abgelegteDateien());
    abgelegteDateien().clear();
    for (const AbgelegteDatei& a : liste) {
        if (g_app->leftMode == 3 && karteneditor::istModellDatei(a.pfad)) {
            karteneditor::modellAbgelegt(a.pfad, ImVec2(a.x, a.y));
        } else {
            dateiVonAussen(a.pfad);
        }
    }
}
void dateienVonAussenOeffnen() {
    if (g_app == nullptr || dateienVonAussen().empty()) {
        return;
    }
    // Warten, solange etwas offen ist, das am AKTIVEN Reiter haengt: der
    // Ereigniseditor merkt sich nur den Weg im Skript - wechselte der Reiter
    // darunter weg, schrieb "Ok" in das neu geoeffnete Skript (Code-Pruefung
    // 03.10., im Selbsttest "pruefung" nachgestellt). Ebenso Fragen, Dialoge,
    // offene Menues und ein laufendes Ziehen. Die Datei bleibt vorgemerkt und
    // geht auf, sobald alles zu ist.
    if (g_app->askSaveOpen || g_app->frageOffen || g_app->editorOpen || g_app->prefsOpen ||
        g_app->missionPickOpen || g_app->dragging ||
        ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel)) {
        return;
    }
    std::vector<std::string> liste;
    liste.swap(dateienVonAussen());
    for (const std::string& p : liste) {
        if (!p.empty()) {
            loadPath(p);   // schon offen? Dann dorthin (reiterMitPfad)
        }
    }
}
// Fuer app_view3d.cpp und den Selbsttest - die eigentlichen Funktionen
// stehen im namenlosen Namensraum bei closeTab.
int reiterMitPfad(const std::string& p) { return reiterMitPfadI(p); }
int reiterMitSkript(const std::string& name) { return reiterMitSkriptI(name); }
void oeffneSkriptAusSpeicher(const std::string& data, const std::string& name) {
    oeffneSkriptAusSpeicherI(data, name);
}
// Ein neuer, leerer Reiter wie "New" - ohne die Rueckfrage und ohne das
// Wiederverwenden eines leeren Reiters, die doNew() davorschaltet.
void neuerReiter() { neuesSkript(); }
std::vector<Node> knotenAusNeuNutzlast(const void* daten, int groesse) { return knotenAusNeuNutzlastIntern(daten, groesse); }
// Aus main: nach dem Neustart eines Updates einmal zeigen, was neu ist.
void changelogZeigen() {
    if (g_app != nullptr) { g_app->changelogOpen = true; }
}
// Fuer den Selbsttest (Modus "pruefung").
void ladePfad(const std::string& p) { loadPath(p); }
void uebersetzen() { doCompile(); }
void alleSpeichern() { doSaveAll(); }
bool speichernUnterPfad(const std::string& p) { return speichernUnterPfadI(p); }

bool reiterSpeichernUnter(int reiter, const std::string& pfad) {
    if (reiter < 0 || reiter >= static_cast<int>(g_app->tabs.size())) {
        return false;
    }
    const auto pfadVon = [reiter]() -> std::string& {
        return reiter == g_app->activeTab ? g_app->path : g_app->tabs[static_cast<std::size_t>(reiter)].path;
    };
    const std::string vorher = pfadVon();
    if (!pfad.empty()) {
        pfadVon() = pfad;
    }
    if (pfadVon().empty()) {
        return false;
    }
    const bool ok = speichereReiterI(reiter);
    if (!ok && !g_app->frageOffen) {
        pfadVon() = vorher;
    }
    return ok;
}


// Eine Zeile ueber ihren WEG auswaehlen und sichtbar machen.
//
// AUSSERHALB des anonymen Namensraums, weil app_view3d.cpp sie ruft.
// Beim ersten Anlauf stand sie darin - dann ist sie TU-lokal, und das
// Binden bricht ab:
//
//     LNK2019: Verweis auf nicht aufgeloestes externes Symbol
//              "void __cdecl bhed::gui::selectByPath(...)"
//
// Dasselbe Muster wie in rc174, nur andersherum: dort fehlte die
// Definition im anonymen Namensraum, hier steckt sie faelschlich darin.
//
// Dasselbe Muster stand dreimal ausgeschrieben da: beim Suchen, beim
// Anklicken eines Kameraschluessels, und ab rc178 beim Anklicken eines
// Blocks in der Zeitleiste. Drei Abschriften laufen auseinander - eine
// davon vergisst scrollToSelected, und dann springt die Ansicht nicht mit.
void selectByPath(const Path& p) {
    if (p.empty()) {
        return;
    }
    // Das Ziel SICHTBAR machen: alle Bloecke darueber aufklappen.
    //
    // Gefunden vom Selbsttest (rc568): die Suche fand sechs `wait`, aber
    // nach jedem "Find next" war die Auswahl leer. Fuenf davon stehen in
    // zugeklappten Bloecken, und rebuildTree verwirft eine Auswahl, deren
    // Zeile nicht im Baum steht ("Knoten ist weg"). Das Original klappt
    // den Baum bis zum Treffer auf; jetzt auch behaved - fuer jeden, der
    // hierher springt.
    for (std::size_t k = 1; k < p.size(); ++k) {
        g_app->expanded.setOpen(kennungFuer(Path(p.begin(), p.begin() + static_cast<std::ptrdiff_t>(k))), true);
    }
    // Ebenso ein gefaltetes Makro, zu dem die Zeile gehoert: seine Befehle
    // stehen in der Datei flach dahinter (tree.cpp, "die naechsten count
    // Befehle gehoeren zum Makro").
    {
        const std::vector<Node>& ns = g_app->doc.script().nodes;
        const std::size_t oben = p.front();
        for (std::size_t j = oben; j-- > 0;) {
            if (ns[j].kind != Node::Kind::Macro) { continue; }
            int befehle = 0;
            for (std::size_t q = j + 1; q <= oben && q < ns.size(); ++q) {
                if (ns[q].kind == Node::Kind::Command) { ++befehle; }
            }
            if (befehle <= ns[j].count) {
                g_app->expanded.setOpen(kennungFuer(Path{j}), true);
            }
            break;
        }
    }
    g_app->selectedPath = p;
    g_app->selection.clear();
    g_app->selection.push_back(p);
    rebuildTree();
    g_app->scrollToSelected = true;
    g_app->mapDirty = true;
}


// Ein Auswahlfeld, in das man auch TIPPEN kann.
//
// Gebaut auf ImGuis eigenem BeginCombo, nicht auf einem selbstgebauten
// Popup. Der erste Anlauf tat Letzteres und hatte drei Fehler, die alle
// dieselbe Wurzel hatten - er kaempfte gegen die Bibliothek:
//
//   * Der zweite Klick auf den Pfeil oeffnete nichts. ImGui schliesst ein
//     Popup beim Klick daneben - und der Pfeil IST daneben. Erst schloss es,
//     dann rief mein Code OpenPopup, und beides hob sich auf.
//   * Die Liste erschien HINTER dem Editorfenster. Popups innerhalb eines
//     modalen Fensters gehoeren auf dessen Stapel; ein von Hand geoeffnetes
//     landet daneben.
//   * Sie liess sich nicht rollen, weil OpenPopup in jedem Bild gerufen
//     wurde. Die Anleitung sagt es ausdruecklich: "don't call every frame!"
//
// BeginCombo macht all das selbst: Auf- und Zuklappen, Stapelordnung,
// Rollbalken, Tastaturbedienung. Das Tippen kommt als Textfeld INNERHALB
// der Liste dazu - so wie es Ravens BehavEd auch nicht konnte, aber wir
// brauchen es fuer Movie Duels' eigene Animationen.
//
// Bedienung:
//   aufklappen   ganze Liste, Filter leer
//   tippen       filtert
//   Eintrag      uebernimmt ihn
//   Eingabe      uebernimmt den GETIPPTEN Text, auch wenn er nicht in der
//                Liste steht
ComboEditResult comboEdit(const char* id, std::string& value,
                          const std::vector<TypeEntry>* entries, float width) {
    ComboEditResult out;
    ImGui::PushID(id);
    ImGui::SetNextItemWidth(width);

    // Der Filtertext lebt so lange, wie die Liste offen ist.
    static std::map<ImGuiID, std::string> filters;
    // Merker fuer "im naechsten Bild zum aktuellen Eintrag rollen" - siehe
    // die Begruendung weiter unten bei SetScrollY.
    static std::map<ImGuiID, int> rollNoch;
    const ImGuiID key = ImGui::GetID("##combo");

    // "Alpha-sort edit pulldowns" (Dialog 131): die Eintraege alphabetisch.
    // Der Schalter wurde bisher nur gespeichert und nirgends ausgewertet.
    //
    // Sortiert wird EINMAL je Liste (die Typmengen aendern sich nicht,
    // solange die .bhc geladen ist) - nicht in jedem Bild. Beide Durchlaeufe
    // unten, Hoehe und Zeilen, benutzen dieselbe Reihenfolge. Abschnitts-
    // ueberschriften entfallen dann: sortiert stuenden sie zwischen fremden
    // Eintraegen.
    const bool alpha = g_app->settings.alphaSortPulldowns;
    std::vector<std::size_t> reihenfolgeNatur;
    const std::vector<std::size_t>* reihenfolge = &reihenfolgeNatur;
    if (entries != nullptr) {
        if (alpha) {
            static std::map<const void*, std::vector<std::size_t>> sortiert;
            std::vector<std::size_t>& s = sortiert[entries];
            if (s.size() != entries->size()) {
                s.resize(entries->size());
                for (std::size_t q = 0; q < s.size(); ++q) { s[q] = q; }
                auto klein = [](const std::string& x) {
                    std::string y = x;
                    for (char& ch : y) {
                        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                    }
                    return y;
                };
                std::stable_sort(s.begin(), s.end(), [&](std::size_t l, std::size_t r) {
                    return klein((*entries)[l].name) < klein((*entries)[r].name);
                });
            }
            reihenfolge = &s;
        } else {
            reihenfolgeNatur.resize(entries->size());
            for (std::size_t q = 0; q < reihenfolgeNatur.size(); ++q) {
                reihenfolgeNatur[q] = q;
            }
        }
    }

    // Die Liste soll so hoch sein wie ihr Inhalt, nicht wie die Vorgabe.
    //
    // ImGuiComboFlags_HeightLarge setzt eine feste Hoehe von zwanzig
    // Zeilen. Bleiben nach dem Filtern nur sechzehn uebrig, klafft darunter
    // leerer Raum - genau das war zu sehen.
    //
    // Nachgesehen in imgui_widgets.cpp, BeginComboPopup():
    //
    //     if (g.NextWindowData.HasFlags & ImGuiNextWindowDataFlags_HasSizeConstraint)
    //         ... (nur die Mindestbreite wird angehoben)
    //     else
    //         SetNextWindowSizeConstraints(constraint_min, constraint_max);
    //
    // Eine eigene Vorgabe wird also uebernommen. Gezaehlt wird mit dem
    // Filter des VORIGEN Bildes - der liegt schon vor, und ein Bild
    // Verzoegerung sieht niemand.
    // Die Hoehe, die die LISTE bekommt - ohne Suchfeld und Trenner.
    //
    // Sie muss ausdruecklich gerechnet werden. Das Klappfenster waechst mit
    // seinem Inhalt; ein Kindfenster mit Hoehe 0 nimmt "die restliche
    // Elterngroesse" - und beides zusammen ist eine Rueckkopplung. ImGui
    // fuehrt sie selbst als bekanntes Problem ("auto-fit feedback loop"),
    // und zu einem selbstwachsenden Fenster mit selbstwachsendem Kind
    // heisst der erste empfohlene Ausweg: die Inhaltsgroesse von Hand
    // rechnen.
    //
    // Genau die Zahlen liegen hier schon vor - `wanted`, `cap` und `extra`
    // bestimmen ohnehin die Fenstergrenze.
    float listenH = 0.0F;
    {
        const auto had = filters.find(key);
        const std::string prev = (had == filters.end()) ? std::string{} : had->second;
        // GENAU so zaehlen, wie die Liste ihre Zeilen baut: Eintraege PLUS
        // Ueberschriften. Die Ueberschriften vergessen zu haben war der
        // Grund, warum unten trotzdem noch Luft blieb.
        int visible = 0;
        if (entries != nullptr) {
            std::string needle = prev;
            for (char& c : needle) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            const std::string* lastSection = nullptr;
            for (const std::size_t ei : *reihenfolge) {
                const TypeEntry& e = (*entries)[ei];
                if (!needle.empty()) {
                    std::string low = e.name;
                    for (char& c : low) {
                        c = static_cast<char>(
                            std::tolower(static_cast<unsigned char>(c)));
                    }
                    if (low.find(needle) == std::string::npos) {
                        continue;
                    }
                }
                if (!alpha && !e.section.empty() &&
                    (lastSection == nullptr || *lastSection != e.section)) {
                    lastSection = &e.section;
                    ++visible;   // die Ueberschrift ist auch eine Zeile
                }
                ++visible;
            }
            if (!prev.empty()) {
                ++visible;   // die Zeile "... uebernehmen"
            }
        }
        const float line = ImGui::GetTextLineHeightWithSpacing();
        const float extra = ImGui::GetFrameHeightWithSpacing() +     // Filterfeld
                            ImGui::GetStyle().WindowPadding.y * 2.0F +
                            ImGui::GetStyle().ItemSpacing.y * 2.0F;  // Trenner
        const float wanted = static_cast<float>(std::max(visible, 1)) * line + extra;
        float cap = 20.0F * line + extra;   // wie HeightLarge

        // Und jetzt der Platz, der WIRKLICH da ist.
        //
        // Zwanzig Zeilen sind eine Obergrenze, kein Versprechen: steht das
        // Feld weit unten im Fenster, laeuft die Liste trotzdem hinaus und
        // die letzten Eintraege sind nicht erreichbar. Genau das war bei
        // SET zu sehen - die Liste reichte ueber den Fensterrand hinaus.
        //
        // ImGui klappt ein Kombifeld von selbst nach oben, wenn unten kein
        // Platz ist. Es kann das aber nur, wenn die verlangte Hoehe
        // ueberhaupt irgendwo hinpasst. Also nehmen wir die groessere der
        // beiden Seiten als Grenze - dann passt sie immer auf eine davon.
        {
            const ImGuiViewport* vp = ImGui::GetMainViewport();
            const float rand = ImGui::GetStyle().WindowPadding.y * 2.0F;
            const ImVec2 hier = ImGui::GetCursorScreenPos();
            const float unten = (vp->Pos.y + vp->Size.y) -
                                (hier.y + ImGui::GetFrameHeight()) - rand;
            const float oben = hier.y - vp->Pos.y - rand;
            const float platz = std::max(unten, oben);
            if (platz > line * 3.0F) {
                cap = std::min(cap, platz);
            }
        }
        // --- Auch die BREITE festlegen -------------------------------
        //
        // shank: "beim Runterscrollen werden die ploetzlich breiter. Ich
        // denke, man muss es generell so breit machen wie das breiteste
        // Wort darin."
        //
        // Genau so ist es. ImGui misst das Klappfenster an den GERADE
        // SICHTBAREN Eintraegen aus - beim Scrollen kommen laengere dazu,
        // und es springt. Bei "SET_RAILCENTERTRACKUNLOCKED" faellt das auf.
        //
        // Der breiteste Eintrag steht von Anfang an fest; er wird einmal
        // gemessen und als Mindestbreite vorgegeben. Dann springt nichts
        // mehr, egal wohin man scrollt.
        //
        // Auch die Abschnittsueberschriften zaehlen mit ("Standard strings"
        // im Bild) - sie stehen in derselben Liste.
        float breitester = 0.0F;
        if (entries != nullptr) {
            std::string letzterAbschnitt;
            for (const TypeEntry& e : *entries) {
                breitester = std::max(breitester,
                                      ImGui::CalcTextSize(e.name.c_str()).x);
                if (e.section != letzterAbschnitt) {
                    letzterAbschnitt = e.section;
                    breitester =
                        std::max(breitester,
                                 ImGui::CalcTextSize(e.section.c_str()).x);
                }
            }
        }
        // Gedeckelt auf die Arbeitsflaeche.
        //
        // Seit rc511 setzt die Liste ihre Mindestbreite selbst. Bei sehr
        // langen Eintraegen kann die groesser werden als der Bildschirm -
        // ImGui schiebt das Fenster dann zurecht, aber der rechte Rand
        // liegt trotzdem draussen.
        //
        // Eine MINDESTbreite, die groesser ist als der Platz, ist keine
        // Mindestbreite mehr, sondern ein Widerspruch.
        const float platzB =
            ImGui::GetMainViewport()->WorkSize.x -
            ImGui::GetStyle().WindowPadding.x * 4.0F;
        const float minB =
            (breitester > 0.0F)
                ? breitester + ImGui::GetStyle().FramePadding.x * 2.0F +
                      ImGui::GetStyle().ScrollbarSize +
                      ImGui::GetStyle().WindowPadding.x * 2.0F
                : 0.0F;
        const float minBGedeckelt = std::min(minB, std::max(platzB, 0.0F));
        // Je Liste EINMAL ins Protokoll.
        //
        // Meine erste Fassung verglich mit EINER gemerkten Zahl. Im
        // Ereignisfenster wechseln sich aber mehrere Klapplisten ab (261
        // Eintraege und 3), und jede sah den Wert der anderen als
        // "Aenderung" - shanks Protokoll hat dadurch 4248 gleiche Zeilen.
        //
        // Ein Protokoll, das man nicht lesen kann, ist keines. Also je
        // (Anzahl, Breite) nur einmal.
        static std::set<std::pair<int, int>> schonGemeldet;
        const std::pair<int, int> schluessel{
            entries != nullptr ? static_cast<int>(entries->size()) : 0,
            static_cast<int>(minB)};
        if (schonGemeldet.insert(schluessel).second) {
            char zec[160];
            std::snprintf(zec, sizeof(zec),
                          "Klappfenster: %d Eintraege, breitester Text %.0f, "
                          "Mindestbreite %.0f (Platz %.0f%s)",
                          entries != nullptr
                              ? static_cast<int>(entries->size())
                              : 0,
                          static_cast<double>(breitester),
                          static_cast<double>(minBGedeckelt),
                          static_cast<double>(platzB),
                          minB > platzB ? ", GEDECKELT" : "");
            diag::detail(zec);
        }
        g_app->dbgComboMinB = minBGedeckelt;
        const float fensterH = std::min(wanted, cap);
        // Untergrenze EINE Zeile, nicht drei.
        //
        // Drei war meine erste Wahl ("sonst unbedienbar") und holte genau
        // den Fehler zurueck, den die Hoehenrechnung weiter oben schon
        // einmal beseitigt hat: filtert man auf einen Treffer herunter,
        // klafft darunter leerer Raum. Bei einem Treffer ergibt die
        // Rechnung eine Zeile - und `visible` wird ohnehin auf mindestens
        // 1 angehoben, also passt auch die Zeile "kein Treffer" hinein.
        listenH = std::max(line, fensterH - extra);
        g_app->dbgComboListenH = listenH;
        ImGui::SetNextWindowSizeConstraints(
            ImVec2{minBGedeckelt, 0.0F},
            ImVec2{FLT_MAX, fensterH});
    }

    const bool comboOffen = ImGui::BeginCombo("##combo", value.c_str());
    // BeginCombo meldet seine Beschriftung der Testumgebung nicht (nur beim
    // Aufklappen). Der Selbsttest findet Klapplisten sonst nicht.
    if (GImGui->TestEngineHookItems) {
        selbsttestMerkeText(GImGui, key, "##combo");
    }
    if (!comboOffen) {
        // Zugeklappt: den Filtertext vergessen. Sonst rechnet das naechste
        // Aufklappen die Hoehe einmal mit dem alten Text - und die Liste
        // springt im ersten Bild.
        filters.erase(key);
        rollNoch.erase(key);
        // Der Meldemerker steht in comboEdit weiter unten; hier ist er
        // nicht sichtbar. Das Aufraeumen geschieht dort ueber
        // IsWindowAppearing() - eine Stelle, ein Mechanismus.
    } else {
        std::string& filter = filters[key];

        char buf[256];
        std::snprintf(buf, sizeof(buf), "%s", filter.c_str());
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::IsWindowAppearing()) {
            filter.clear();
            buf[0] = '\0';
            ImGui::SetKeyboardFocusHere();
        }
        const bool entered = ImGui::InputTextWithHint(
            "##filter", tr(Str::FilterHint), buf, sizeof(buf),
            ImGuiInputTextFlags_EnterReturnsTrue);
        filter = buf;
        // Escape schliesst die Liste - mit EINEM Druck, wie eine
        // Windows-Klappliste im Original. Das Suchfeld behielt die Taste
        // bisher fuer sich (verlassen), und ein zweites Escape bewirkte gar
        // nichts: ImGui schliesst Klappfenster mit Escape nur ueber die
        // Tastaturnavigation, und die ist aus. Gefunden vom Selbsttest.
        if ((ImGui::IsItemDeactivated() || !ImGui::IsAnyItemActive()) &&
            ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            ImGui::CloseCurrentPopup();
        }

        // Eingabetaste: den getippten Text uebernehmen, auch wenn er in der
        // Liste nicht vorkommt. Genau dafuer ist das Feld da.
        if (entered && !filter.empty()) {
            value = filter;
            out.edited = true;
            out.picked = true;
            ImGui::CloseCurrentPopup();
        }

        std::string needle = filter;
        for (char& ch : needle) {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }

        // --- Ab hier ein eigenes Kindfenster ------------------------------
        //
        // shank: "die suchleiste die nicht mit Scrollt ... das die suchliste
        // immer oben visible bleibt und beim runter scrollen nicht weg ist."
        //
        // Bisher standen Suchfeld und Liste im SELBEN Rollbereich, also
        // rollte das Feld mit hinaus. Ein eigenes Kindfenster fuer die Liste
        // trennt beides: das Klappfenster selbst rollt dann gar nicht mehr,
        // nur noch das Kind.
        //
        // Die Hoehe kommt von oben, ausdruecklich gerechnet - siehe dort.
        // Die Breite darf 0 sein ("restliche Elterngroesse"), weil die
        // Mindestbreite des Klappfensters bereits am breitesten Eintrag
        // haengt; sie kann also nicht zusammenfallen.
        //
        // `EndChild()` muss in JEDEM Fall gerufen werden, auch wenn
        // `BeginChild` false liefert - das steht so in der Anleitung.
        ImGui::Separator();
        // Einmal je Aufklappen: was gerechnet wurde und was daraus wurde.
        //
        // shank: "adde das ins debug log, das ich dir das schicken kann und
        // du kontrollieren kannst das alles geht." Genau dafuer. Ohne diese
        // Zeilen kann ich von hier aus nicht pruefen, ob das Kindfenster
        // seine Hoehe bekommt - gui/app.cpp laesst sich ohne ImGui nicht
        // einmal uebersetzen.
        // NICHT im ersten Bild messen.
        //
        // rc542 tat genau das und schrieb `Fensterhoehe 24`, waehrend die
        // Liste 600 hoch war - kleiner als das Suchfeld allein. Die Zahl
        // war falsch, nicht der Aufbau: ein selbstwachsendes Fenster kennt
        // seine Groesse im Bild des Auftauchens noch nicht. Dieselbe
        // Ursache wie beim geklemmten SetScrollY in rc541, nur eine
        // Messung statt einer Wirkung.
        //
        // Also im ZWEITEN Bild, einmal je Aufklappen.
        static std::set<ImGuiID> schonGemeldetAuf;
        if (ImGui::IsWindowAppearing()) {
            schonGemeldetAuf.erase(key);
        } else if (schonGemeldetAuf.insert(key).second) {
            char z[220];
            std::snprintf(z, sizeof(z),
                          "Klappliste auf: %d Eintraege, Listenhoehe %.0f, "
                          "Fensterhoehe %.0f, Suchfeld %.0f hoch",
                          entries != nullptr
                              ? static_cast<int>(entries->size())
                              : 0,
                          static_cast<double>(listenH),
                          static_cast<double>(ImGui::GetWindowSize().y),
                          static_cast<double>(
                              ImGui::GetFrameHeightWithSpacing()));
            diag::detail(z);
        }
        ImGui::BeginChild("##liste", ImVec2{0.0F, listenH});

        // --- Der eigene Wert, sichtbar ------------------------------------
        //
        // Uebernehmen liess sich getippter Text schon vorher - mit der
        // Eingabetaste. Nur stand am Feld "Tippen zum Filtern", und damit
        // kam niemand auf die Idee. Movie Duels bringt eigene Animationen
        // mit, die in keiner .bhc stehen; genau dafuer ist das da.
        //
        // Jetzt steht die Moeglichkeit als anklickbare Zeile ganz oben,
        // sobald der getippte Text nicht schon genau ein Eintrag ist. Damit
        // ist auch "kein Treffer" keine Sackgasse mehr, sondern der Ort, wo
        // man den eigenen Namen bestaetigt.
        //
        // Der Wert bleibt erhalten: der Pruefer meldet ihn als Anmerkung
        // V004 ("steht nicht in ..., bleibt unveraendert erhalten"), nicht
        // als Fehler. Geschrieben wird er unveraendert.
        bool exactMatch = false;
        if (entries != nullptr && !filter.empty()) {
            for (const TypeEntry& e : *entries) {
                if (e.name == filter) {
                    exactMatch = true;
                    break;
                }
            }
        }
        if (!filter.empty() && !exactMatch) {
            char label[320];
            // "###eigen" haengt die Kennung fest: sonst leitet ImGui sie aus
            // dem Text ab, und ein getipptes "##" wuerde die Beschriftung
            // abschneiden und die Kennung wandern lassen.
            std::snprintf(label, sizeof(label), tr(Str::UseTyped), filter.c_str());
            const std::string row = std::string(label) + "###eigen";
            if (ImGui::Selectable(row.c_str())) {
                value = filter;
                out.edited = true;
                out.picked = true;
                ImGui::CloseCurrentPopup();
            }
        }

        if (entries != nullptr) {
            // Erst die Anzeigeliste bauen: Ueberschriften und Eintraege
            // gemischt, als Nummern. Danach kann der Clipper darueber
            // laufen.
            //
            // Der Clipper (ImGuiListClipper) reicht nur die SICHTBAREN
            // Zeilen durch. Vorher schnitt ich bei 500 Treffern ab und
            // schrieb "..." - eine Kruecke, weil 1543 Animationsnamen
            // untereinander sonst jedes Bild gekostet haetten. Mit dem
            // Clipper darf die Liste vollstaendig sein.
            struct DisplayRow {
                int entry;         // Nummer im Eintragsfeld, -1 = Ueberschrift
                const std::string* section;
            };
            static std::vector<DisplayRow> rows;
            rows.clear();
            rows.reserve(entries->size());
            const std::string* lastSection = nullptr;
            int currentRow = -1;
            for (const std::size_t i : *reihenfolge) {
                const TypeEntry& e = (*entries)[i];
                if (!needle.empty()) {
                    std::string low = e.name;
                    for (char& ch : low) {
                        ch = static_cast<char>(
                            std::tolower(static_cast<unsigned char>(ch)));
                    }
                    if (low.find(needle) == std::string::npos) {
                        continue;
                    }
                }
                if (!alpha && !e.section.empty() &&
                    (lastSection == nullptr || *lastSection != e.section)) {
                    lastSection = &e.section;
                    rows.push_back(DisplayRow{-1, lastSection});
                }
                if (e.name == value) {
                    currentRow = static_cast<int>(rows.size());
                }
                rows.push_back(DisplayRow{static_cast<int>(i), nullptr});
            }

            if (rows.empty()) {
                // Keine Sackgasse mehr: die Zeile "... uebernehmen" steht
                // schon oben. Hier nur noch der Hinweis, dass die Liste
                // nichts hergibt.
                ImGui::TextDisabled("%s", tr(Str::MsgNoMatch));
            } else {
                // Beim Aufklappen zum aktuellen Eintrag rollen. Mit einem
                // Clipper geht SetItemDefaultFocus nicht mehr - der Eintrag
                // wird ja gar nicht abgeschickt, wenn er ausserhalb liegt.
                // Deshalb ueber SetScrollY, das die Nummer kennt.
                //
                // Aber NICHT im ersten Bild. Das stand hier und wirkte
                // nicht: `SetScrollY` setzt nur ein Ziel, und ImGui klemmt
                // es beim naechsten `Begin()` gegen `ScrollMax`. Der Wert
                // stammt aus der Inhaltshoehe des VORIGEN Bildes - und die
                // ist beim frisch aufgeklappten Fenster null. Das Ziel
                // wurde also auf 0 geklemmt, und die Liste stand oben.
                //
                // shank zu rc540, mit zwei Bildern: die Klappliste geht bei
                // SET_ORIGIN auf und zeigt SET_PARM1 ganz oben, statt zum
                // aktuellen Eintrag zu rollen.
                //
                // Also im ZWEITEN Bild rollen. Dann steht die Inhaltshoehe
                // fest, weil der Clipper sie im ersten gesetzt hat. Ein
                // Bild Verzoegerung sieht niemand - dieselbe Ueberlegung
                // wie bei der Hoehenrechnung weiter oben.
                if (ImGui::IsWindowAppearing()) {
                    rollNoch[key] = 1;
                } else {
                    const auto rn = rollNoch.find(key);
                    if (rn != rollNoch.end()) {
                        const float ziel =
                            static_cast<float>(std::max(currentRow, 0)) *
                            ImGui::GetTextLineHeightWithSpacing();
                        if (currentRow >= 0) {
                            ImGui::SetScrollY(ziel);
                        }
                        // Was gewollt war, was moeglich ist, was schon
                        // steht. Nur diese eine Zeile je Aufklappen.
                        char z[200];
                        std::snprintf(
                            z, sizeof(z),
                            "Klappliste rollt: Zeile %d, Ziel %.0f, "
                            "ScrollMax %.0f, Kindhoehe %.0f%s",
                            currentRow, static_cast<double>(ziel),
                            static_cast<double>(ImGui::GetScrollMaxY()),
                            static_cast<double>(ImGui::GetWindowSize().y),
                            currentRow < 0 ? "  (Wert nicht in der Liste)"
                                           : "");
                        diag::detail(z);
                        rollNoch.erase(rn);
                    }
                }
                // Die Zeilenhoehe AUSDRUECKLICH mitgeben.
                //
                // Der Clipper setzt gleich hohe Zeilen voraus, und ohne
                // Angabe nimmt er die Hoehe der ERSTEN als Mass fuer alle
                // (ocornut/imgui#6042: "the ImGuiListClipper assumes that
                // all the rows are the same height").
                //
                // Unsere erste Zeile ist eine Ueberschrift, und die ist
                // hoeher als ein Eintrag. Also rechnete er die ganze Liste
                // mit Ueberschriftshoehe - bei siebzehn Zeilen ergab das
                // mehrere Zeilen Luft am unteren Rand, die niemand fuellt.
                // Genau der Abstand, der zu sehen war.
                //
                // Deshalb sind unten AUCH die Ueberschriften genau eine
                // Textzeile hoch. Beides zusammen - gleiche Hoehe und
                // angesagte Hoehe - macht die Rechnung exakt.
                ImGuiListClipper clipper;
                clipper.Begin(static_cast<int>(rows.size()),
                              ImGui::GetTextLineHeightWithSpacing());
                while (clipper.Step()) {
                    for (int row = clipper.DisplayStart; row < clipper.DisplayEnd;
                         ++row) {
                        const DisplayRow& d = rows[static_cast<std::size_t>(row)];
                        if (d.entry < 0) {
                            // Eine Ueberschrift, genau EINE Textzeile hoch.
                            //
                            // ImGui::SeparatorText ist hoeher - es setzt
                            // Abstand darueber und darunter. Das bringt den
                            // Clipper aus dem Tritt (siehe oben). Also der
                            // Text gedaempft, und der Strich daneben von
                            // Hand gezogen: sieht gleich aus, ist aber so
                            // hoch wie jeder andere Eintrag.
                            const ImVec2 wo = ImGui::GetCursorScreenPos();
                            ImGui::TextDisabled("%s", d.section->c_str());
                            const float th = ImGui::GetTextLineHeight();
                            const float breite =
                                ImGui::GetContentRegionAvail().x;
                            const float hinter =
                                wo.x + ImGui::CalcTextSize(d.section->c_str()).x +
                                ImGui::GetStyle().ItemSpacing.x;
                            const float rechts = wo.x + breite;
                            if (rechts > hinter) {
                                ImGui::GetWindowDrawList()->AddLine(
                                    ImVec2{hinter, wo.y + th * 0.5F},
                                    ImVec2{rechts, wo.y + th * 0.5F},
                                    ImGui::GetColorU32(ImGuiCol_Separator));
                            }
                            continue;
                        }
                        const TypeEntry& e =
                            (*entries)[static_cast<std::size_t>(d.entry)];
                        if (ImGui::Selectable(e.name.c_str(), e.name == value)) {
                            value = e.name;
                            out.picked = true;
                            // Selbst zuklappen.
                            //
                            // Normalerweise erledigt ImGui das: "By default,
                            // Selectable()/MenuItem() are calling
                            // CloseCurrentPopup()." Das haengt aber am
                            // AKTUELLEN Fenster - und seit rc542 steht die
                            // Liste in einem Kindfenster, und ein Kind ist
                            // kein Klappfenster. Also blieb die Liste offen.
                            //
                            // shank zu rc542: "die dropdown liste geht nicht
                            // mehr automatisch zu, wenn man was ausgewaehlt
                            // hat."
                            //
                            // Die Zeile "eigenen Wert uebernehmen" weiter
                            // oben tat es von Anfang an selbst - deshalb ist
                            // die nie aufgefallen.
                            ImGui::CloseCurrentPopup();
                        }
                        if (ImGui::IsItemHovered() && !e.desc.empty()) {
                            ImGui::SetTooltip("%s", e.desc.c_str());
                        }
                    }
                }
            }
        }
        ImGui::EndChild();
        ImGui::EndCombo();
    }
    ImGui::PopID();
    return out;
}

// Einen Ordner als Spielordner aufnehmen, falls er noch fehlt.
//
// rc603: nur noch auf ausdruecklichen Wunsch (Spielordner-Fenster) - "Load
// mission" nimmt nichts mehr dauerhaft auf. Wer die eingerichteten Ordner
// aendert, will mit ihnen arbeiten: die Ordner einer Sitzung enden dann.
//
// rc604: ein Ordner von Jedi Outcast (jediOutcastOrdner) kommt in die eigene
// Liste (gamePathsJk2) - nie zu den Ordnern von Jedi Academy.
void addGamePathDirectory(const std::string& dir) {
    if (dir.empty()) {
        return;
    }
    std::vector<std::string>& liste =
        jediOutcastOrdner(dir) ? g_app->settings.gamePathsJk2 : g_app->settings.gamePaths;
    for (const std::string& g : liste) {
        if (g == dir) {
            if (!g_app->sitzungsSpielordner.empty()) {
                setzeSitzungsSpielordner({});
            }
            return;
        }
    }
    liste.push_back(dir);
    g_app->sitzungsSpielordner.clear();
    rescanGamePaths();
    refreshPk3List();
}

const std::vector<std::string>& aktiveSpielordner() {
    // rc604: die eingerichteten des Regelwerks (JK2 oder JKA/Movie Duels).
    return g_app->sitzungsSpielordner.empty() ? eingerichteteSpielordner() : g_app->sitzungsSpielordner;
}

void setzeSitzungsSpielordner(std::vector<std::string> ordner) {
    // Gleichen die neuen den eingerichteten, gilt keine Sitzung.
    const auto gleich = [](const std::vector<std::string>& a, const std::vector<std::string>& b) {
        if (a.size() != b.size()) {
            return false;
        }
        for (std::size_t i = 0; i < a.size(); ++i) {
            if (!gleicherOrdner(a[i], b[i])) {
                return false;
            }
        }
        return true;
    };
    if (gleich(ordner, eingerichteteSpielordner())) {   // rc604: die des Regelwerks
        ordner.clear();
    }
    g_app->sitzungsSpielordner = std::move(ordner);
    // Neu einlesen nur, wenn die geltenden Ordner andere sind als die zuletzt
    // eingelesenen - rescanGamePaths leert alle Zwischenspeicher.
    // rc603 (Durchsicht): verglichen wird mit der Liste, die rescanGamePaths
    // zuletzt durchsucht hat (eingeleseneSpielordner), nicht mit den Ordnern
    // in g_app->gamePaths - dort fehlt ein Ordner ohne .pk3 (eine Mod nur mit
    // losen Dateien), und dann las JEDES "Load mission" alle Spielordner neu
    // (Selbsttest missionsliste, "Ordner ohne pk3": Einlesestand 7 -> 8, je
    // rund 0,5 s, dazu alle Texturen der offenen Karte).
    if (g_app->spielordnerEingelesen && gleich(g_app->eingeleseneSpielordner, aktiveSpielordner())) {
        return;
    }
    if (g_app->sitzungsSpielordner.empty()) {
        diag::info("Spielordner: wieder die eingerichteten");
    } else {
        std::string liste;
        for (const std::string& o : g_app->sitzungsSpielordner) {
            liste += (liste.empty() ? "" : "; ") + o;
        }
        diag::info("Spielordner nur fuer diese Sitzung: " + liste);
        char z[1200];
        std::snprintf(z, sizeof(z), tr(Str::MissionSessionStatus), liste.c_str());
        addStatus(z);
    }
    rescanGamePaths();
    refreshPk3List();
}

// Vor jedem Wegwerfen fragen. Das Original zeigt denselben Dialog; ohne ihn
// verliert man die Arbeit beim versehentlichen Klick auf "New".
// Vor dem Wegwerfen fragen - und DANACH weitermachen.
//
// Die alte Fassung gab ein bool zurueck, weil eine Windows-MessageBox
// sofort antwortet. Ein Fenster im eigenen Stil tut das nicht: es steht ein
// paar Bilder lang da und wartet. Also wird mitgegeben, was danach geschehen
// soll.
//
// Ist nichts zu sichern, laeuft es unveraendert durch - dann ist der Aufruf
// so gut wie kostenlos.
void withUnsaved(std::function<void()> then, std::function<void()> onCancel) {
    // Ungesicherte Aenderungen: IMMER fragen, wie das Original.
    //
    // Vorher hiess "Yes/No query on Open/New/Exit" abgeschaltet: gar nicht
    // fragen - und die Aenderungen gingen still verloren. Am laufenden
    // BehavEd.exe beobachtet: "Current script has unsaved changes. Save
    // this file first?" kommt auch mit ausgeschaltetem Schalter. Der steuert
    // dort nur die ZUSAETZLICHEN Fragen "Open?", "New?", "Exit?" (doOpen,
    // doNew, confirmQuit).
    if (!g_app->doc.dirty()) {
        then();
        return;
    }
    (void)onCancel;
    g_app->askSaveName = !g_app->path.empty()  ? fileName(g_app->path)
                         : !g_app->shownName.empty() ? g_app->shownName
                                                     : std::string("unnamed.txt");
    g_app->askSaveThen = std::move(then);
    g_app->askSaveCancel = std::move(onCancel);
    g_app->askSaveOpen = true;
}

void frage(const std::string& text, std::function<void()> ja,
           std::function<void()> nein) {
    g_app->frageText = text;
    g_app->frageJa = std::move(ja);
    g_app->frageNein = std::move(nein);
    g_app->frageOffen = true;
    diag::detail("Frage: " + text);
}

// Die Ja/Nein-Frage - dieselbe Form wie die Speichernfrage darunter.
// Eingabe heisst Ja, Escape Nein, wie bei einer MessageBox.
void drawFrage() {
    if (!g_app->frageOffen) {
        return;
    }
    const std::string titel = std::string(tr(Str::AppTitle)) + "###frage";
    ImGui::OpenPopup(titel.c_str());
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        ImVec2{vp->Pos.x + vp->Size.x * 0.5F, vp->Pos.y + vp->Size.y * 0.5F},
        ImGuiCond_Appearing, ImVec2{0.5F, 0.5F});
    if (!ImGui::BeginPopupModal(titel.c_str(), nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize |
                                    ImGuiWindowFlags_NoSavedSettings)) {
        return;
    }
    ImGui::TextUnformatted(g_app->frageText.c_str());
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    const float knopfB = std::max(bhed::gui::dx(kEdOkW),
                                  ImGui::CalcTextSize(tr(Str::AnswerYes)).x +
                                      ImGui::GetStyle().FramePadding.x * 2.0F);
    const float reihe = knopfB * 2.0F + ImGui::GetStyle().ItemSpacing.x;
    ImGui::SetCursorPosX(std::max(ImGui::GetStyle().WindowPadding.x,
                                  (ImGui::GetWindowWidth() - reihe) * 0.5F));
    int antwort = 0;   // 1 = Ja, 2 = Nein
    if (ImGui::Button(tr(Str::AnswerYes), ImVec2{knopfB, 0.0F}) ||
        ImGui::Shortcut(ImGuiKey_Enter) || ImGui::Shortcut(ImGuiKey_KeypadEnter)) {
        antwort = 1;
    }
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::AnswerNo), ImVec2{knopfB, 0.0F}) ||
        ImGui::Shortcut(ImGuiKey_Escape)) {
        antwort = 2;
    }
    if (antwort != 0) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
    if (antwort != 0) {
        // Erst schliessen, dann ausfuehren: die Antwort darf selbst wieder
        // fragen (Einfuegen nach "Proceed?", Beenden nach "Exit?").
        std::function<void()> dann =
            std::move(antwort == 1 ? g_app->frageJa : g_app->frageNein);
        g_app->frageOffen = false;
        g_app->frageJa = nullptr;
        g_app->frageNein = nullptr;
        diag::detail(std::string("Frage beantwortet: ") + (antwort == 1 ? "Ja" : "Nein"));
        if (dann) { dann(); }
    }
}

// --- Zwischenablage wie im Original ---------------------------------------
//
// Das Original legt kopierte Zeilen als TEXT in die Windows-Zwischenablage:
// "//(BHVD)", Zeilenumbruch, dann die Skriptzeilen - beobachtet:
//
//     //(BHVD)
//     declare ( /*@DECLARE_TYPE*/ FLOAT, "rand" );
//
// Damit tauscht man mit Notepad, Foren, einer zweiten Instanz und dem
// Original selbst. behaved hatte bisher nur eine interne Ablage.
//
// Die interne Ablage bleibt: steht in Windows noch genau der Text, den wir
// selbst hineingelegt haben, wird sie benutzt. Sonst wird der Text gelesen
// wie eine Skriptdatei.
//
// ImGui::SetClipboardText schreibt Unicode-Text; Windows liefert daraus
// auch CF_TEXT, das Original liest ihn also.
void ablageNachWindows() {
    const std::vector<Node>& c = g_app->doc.clipboard();
    if (c.empty()) {
        return;
    }
    g_app->ablage = c;   // gehoert dem Anwender, nicht dem Reiter
    std::string text = "//(BHVD)\r\n";
    text += writeNodes(c, 0);
    ImGui::SetClipboardText(text.c_str());
    g_app->ablageText = text;
}

void kopieren(bool ausschneiden) {
    const Path sel = g_app->selectedPath;
    const std::vector<Path> m = selectionOrCurrent();
    if (m.empty()) {
        // Original: "No line selected, Copy entire script?" - beim
        // Ausschneiden ohne Auswahl tut es nichts.
        if (!ausschneiden && !g_app->doc.script().nodes.empty()) {
            frage(tr(Str::AskCopyAll), [] {
                g_app->doc.setClipboard(g_app->doc.script().nodes);
                ablageNachWindows();
                setStatus(tr(Str::MsgCopied),
                          static_cast<int>(g_app->doc.script().nodes.size()), 0);
            });
        }
        return;
    }
    if (ausschneiden) {
        const bool ok = m.size() > 1 ? g_app->doc.cutAll(m) : g_app->doc.cutAt(sel);
        if (ok) {
            ablageNachWindows();
            g_app->selection.clear();
            g_app->selectedPath.clear();
            rebuildTree();
        }
    } else {
        const bool ok = m.size() > 1 ? g_app->doc.copyAll(m) : g_app->doc.copyAt(sel);
        if (ok) { ablageNachWindows(); }
    }
}

void kopiereWeg(const Path& weg, bool ausschneiden) {
    if (ausschneiden) {
        if (g_app->doc.cutAt(weg)) {
            ablageNachWindows();
            rebuildTree();
        }
    } else if (g_app->doc.copyAt(weg)) {
        ablageNachWindows();
    }
}

void einfuegen(const Path& hinter) {
    const char* roh = ImGui::GetClipboardText();
    const std::string text = (roh != nullptr) ? std::string(roh) : std::string();
    const Path at = hinter;
    // In einen angeklickten Block ans Ende (siehe zielBlock) - nur, wenn das
    // Ziel die Auswahl ist; das Kontextmenue einer anderen Zeile meint
    // "hinter diese Zeile".
    const Path block = (hinter == g_app->selectedPath) ? zielBlock() : Path{};
    auto rein = [at, block] {
        if (!block.empty()) {
            if (g_app->doc.pasteInto(block)) {
                g_app->expanded.setOpen(kennungFuer(block), true);
                rebuildTree();
            }
            return;
        }
        if (g_app->doc.pasteAfter(at)) { rebuildTree(); }
    };
    // Nichts in Windows, oder noch unser eigener Text: die interne Ablage.
    if (text.find_first_not_of(" \t\r\n") == std::string::npos ||
        text == g_app->ablageText) {
        // Die Ablage des Programms, nicht die des Reiters - der kann neu sein
        // (die Quelle geschlossen, "+", Neu, Oeffnen) oder eine aeltere haben.
        // Nicht IN rein(): der Zweig unten legt den gelesenen Text selbst ab.
        g_app->doc.setClipboard(g_app->ablage);
        rein();
        return;
    }
    const bool vonBehaved = text.rfind("//(BHVD)", 0) == 0;
    auto uebernehmen = [text, vonBehaved, rein] {
        std::string rumpf = text;
        if (vonBehaved) {
            const std::size_t nl = rumpf.find('\n');
            rumpf = (nl == std::string::npos) ? std::string() : rumpf.substr(nl + 1);
        }
        Script sc;
        std::vector<Diag> d;
        (void)readScript(rumpf, sc, d);
        // Leerzeilen am Rand gehoeren nicht zum Kopierten.
        while (!sc.nodes.empty() && sc.nodes.front().kind == Node::Kind::Blank) {
            sc.nodes.erase(sc.nodes.begin());
        }
        while (!sc.nodes.empty() && sc.nodes.back().kind == Node::Kind::Blank) {
            sc.nodes.pop_back();
        }
        diag::detail("Einfuegen aus der Windows-Zwischenablage: " +
                     std::to_string(sc.nodes.size()) + " Zeilen" +
                     (vonBehaved ? " (von BehavEd)" : " (fremder Text)"));
        if (sc.nodes.empty()) {
            return;
        }
        g_app->doc.setClipboard(std::move(sc.nodes));
        rein();
    };
    if (vonBehaved) {
        uebernehmen();
    } else {
        // Original: "You seem to be pasting something that wasn't copied
        // from BehavEd" / "Proceed?"
        frage(tr(Str::AskPasteForeign), uebernehmen);
    }
}

// Die Frage selbst, im Stil des Programms.
void drawAskSave() {
    if (!g_app->askSaveOpen) {
        return;
    }
    ImGui::OpenPopup(tr(Str::AppTitle));
    // Mittig ueber dem Fenster - eine MessageBox steht auch dort.
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        ImVec2{vp->Pos.x + vp->Size.x * 0.5F, vp->Pos.y + vp->Size.y * 0.5F},
        ImGuiCond_Appearing, ImVec2{0.5F, 0.5F});
    if (!ImGui::BeginPopupModal(tr(Str::AppTitle), nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize |
                                    ImGuiWindowFlags_NoSavedSettings)) {
        return;
    }
    ImGui::TextUnformatted(tr(Str::MsgUnsavedChanges));
    ImGui::Spacing();
    ImGui::TextDisabled("%s", g_app->askSaveName.c_str());
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Die Antwort erst NACH dem Schliessen ausfuehren: das Weitermachen
    // kann selbst wieder fragen (beim Beenden mit mehreren Reitern), und
    // dann muss dieses Fenster schon zu sein.
    int antwort = 0;   // 1 = Ja, 2 = Nein, 3 = Abbrechen

    // --- Knoepfe: uebliche Hoehe, mittig, mit Luft darunter ---------------
    //
    // Gemeldet: "die buttons sind etwas abgeschnitten".
    //
    // Sie hatten eine FESTE Hoehe aus dy(kEdFieldH) - gedacht fuer
    // Eingabefelder, wo sie zur Zeilenhoehe passen muss. Bei 144 dpi faellt
    // sie hoeher aus als das, was das Fenster fuer eine Knopfzeile
    // einplant, und der untere Rand schneidet ab.
    //
    // Mit Hoehe 0 nimmt ImGui die Hoehe, die zum Text passt - und die
    // stimmt dann auch mit der gemessenen Fenstergroesse ueberein. Dazu
    // eine Leerzeile darunter, damit die Knoepfe nicht auf der Kante
    // sitzen.
    //
    // Und mittig, wie im Ereignisfenster: drei Knoepfe links in der Ecke
    // sehen aus, als haette man den Rest vergessen.
    const float knopfB = bhed::gui::dx(kEdOkW);
    const float reihe = knopfB * 3.0F + ImGui::GetStyle().ItemSpacing.x * 2.0F;
    ImGui::SetCursorPosX(
        std::max((ImGui::GetWindowWidth() - reihe) * 0.5F,
                 ImGui::GetStyle().WindowPadding.x));

    if (ImGui::Button(tr(Str::AnswerYes), ImVec2{knopfB, 0.0F})) {
        antwort = 1;
    }
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::AnswerNo), ImVec2{knopfB, 0.0F})) {
        antwort = 2;
    }
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::EditorCancel), ImVec2{knopfB, 0.0F}) ||
        ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        antwort = 3;
    }
    ImGui::Spacing();
    if (antwort != 0) {
        ImGui::CloseCurrentPopup();
        g_app->askSaveOpen = false;
        std::function<void()> then = std::move(g_app->askSaveThen);
        std::function<void()> beiAbbruch = std::move(g_app->askSaveCancel);
        g_app->askSaveThen = nullptr;
        g_app->askSaveCancel = nullptr;
        ImGui::EndPopup();
        if (antwort == 1) {
            // Ging das Speichern NICHT (Speichern unter abgebrochen,
            // schreibgeschuetzt, Fehler), ist das wie Abbrechen - sonst blieb
            // beim Beenden quitAskFrom hinter diesem Reiter stehen, und ein
            // spaeteres Beenden uebersprang ihn ungefragt (Code-Pruefung
            // 03.10.).
            if (doSave()) {
                if (then) { then(); }
            } else if (beiAbbruch) {
                beiAbbruch();
            }
        } else if (antwort == 2) {
            if (then) { then(); }
        } else if (beiAbbruch) {
            beiAbbruch();
        }
        return;
    }
    ImGui::EndPopup();
}

// Ein Skript aus dem Speicher oeffnen.
//
// Herausgezogen, weil es jetzt zwei Aufrufer gibt: das .pk3-Fenster und die
// Skriptliste der Karte. Erkannt wird am INHALT, ob es eine kompilierte .ibi
// ist - die Endung sagt es nicht zuverlaessig, in Movie Duels heissen sie
// mal .IBI und mal .ibi.
void openScriptFromMemory(const std::string& data, const std::string& shownName) {
    // Auch von hier in einen eigenen Reiter - dieselbe Regel wie beim
    // Oeffnen aus einer Datei. Gerade beim Laden einer Mission mit zehn
    // Skripten ist das der Sinn der Sache.
    //
    // Die KARTE bleibt dabei stehen: der neue Reiter uebernimmt den Pfad
    // des alten. Beim Laden einer Mission gehoeren alle Skripte zu
    // derselben .bsp, und die zehnmal zu laden waere Unsinn. Nur ein
    // ausdruecklich neuer Reiter (das Pluszeichen) faengt ohne Karte an.
    const bool leerUndSauber =
        g_app->path.empty() && g_app->shownName.empty() &&
        !g_app->doc.dirty() && g_app->doc.script().nodes.empty();
    if (!leerUndSauber) {
        addTab(true);
        takeTab(g_app->activeTab);
    }
    Script sc;
    std::vector<Diag> d;
    const bool isIbi = data.size() > 4 && data.compare(0, 3, "IBI") == 0;
    if (isIbi) {
        std::vector<IbiBlock> bl;
        if (readIbi(data, bl, d)) {
            (void)decompile(bl, g_app->db, sc, d);
        }
    } else {
        (void)readScript(data, sc, d);
    }
    g_app->doc = Document{sc};
    // Aus einem Archiv gibt es keinen Pfad zum Zurueckspeichern.
    g_app->path.clear();
    g_app->shownName = fileName(shownName);
    // "md_ga/intro_jedi.txt" -> "md_ga/intro_jedi": so heisst es fuer ICARUS.
    {
        std::string sp = shownName;
        for (char& c : sp) { if (c == '\\') { c = '/'; } }
        const auto punkt = sp.find_last_of('.');
        const auto strich = sp.find_last_of('/');
        if (punkt != std::string::npos && (strich == std::string::npos || punkt > strich)) {
            sp.resize(punkt);
        }
        g_app->skriptPfad = sp;
    }
    g_app->selectedPath.clear();
    g_app->selection.clear();
    aenderungsStandNeu();
    g_app->lesezeichen.erase(tabKennung(g_app->activeTab));
    diag::info("Skript geladen: " + shownName);
    rebuildTree();
    setStatus(tr(Str::MsgLoaded), static_cast<int>(g_app->rows.size()), 0);
}


// --- Ueber Dateigrenzen hinweg sichtbar --------------------------------
//
// Diese Funktionen ruft app_view3d.cpp auf. Sie stehen deshalb AUSSERHALB
// der anonymen Gruppe - alles Uebrige darin bleibt auf diese Datei
// beschraenkt, wie es sich gehoert.

void rebuildTree() {
    // Kennungen richtigstellen, BEVOR die Zeilen gebaut werden.
    //
    // Der einzige Ort, an dem das noetig ist: hier laufen alle
    // Bearbeitungen zusammen. Keine der fuenfzehn Stellen, an denen ein
    // Node entsteht, muss etwas von Kennungen wissen.
    g_app->doc.vergibKennungen();
    // Die Gizmos zeigen die Kameras des Skripts - aendert sich das Skript,
    // muss die Ansicht neu.
    g_app->mapDirty = true;
    // Und die Kamerabahn ebenso: sie wird aus denselben Befehlen gebaut.
    g_app->camTrackValid = false;
    // Wege, die es nicht mehr gibt, aus der Auswahl werfen. Sonst zeigt sie
    // nach einer Loeschung auf Knoten, die weg sind - und die naechste
    // Bearbeitung traefe irgendetwas.
    for (std::size_t i = g_app->selection.size(); i > 0; --i) {
        if (nodeAt(g_app->doc.script(), g_app->selection[i - 1]) == nullptr) {
            g_app->selection.erase(g_app->selection.begin() +
                                   static_cast<std::ptrdiff_t>(i - 1));
        }
    }
    g_app->treeOpt.expanded = &g_app->expanded;
    buildTree(g_app->doc.script(), g_app->db, g_app->treeOpt, g_app->rows);
    g_app->issues.clear();
    validate(g_app->doc.script(), g_app->db, g_app->issues, g_app->settings.mpZiel);

    // Die Auswahl haengt am WEG, nicht an der Zeilennummer. Sonst springt
    // sie bei jeder Einfuegung eine Zeile weiter, und der naechste Klick auf
    // "Delete" trifft den falschen Knoten.
    g_app->doc.vergibKennungen();
    g_app->zeilenMarke = zeilenMarken(g_app->activeTab, g_app->doc.script(), g_app->rows);
    g_app->selected = -1;
    if (!g_app->selectedPath.empty()) {
        for (std::size_t i = 0; i < g_app->rows.size(); ++i) {
            if (g_app->rows[i].path == g_app->selectedPath) {
                g_app->selected = static_cast<int>(i);
                break;
            }
        }
        if (g_app->selected < 0) {
            g_app->selectedPath.clear();   // Knoten ist weg (geloescht)
        }
    }
}

void openEditorForNode(const Path& p) {
    const Node* n = nodeAt(g_app->doc.script(), p);
    // Ein Makrokopf (standOnly, walkOnly ...) hat keinen Editor. "Edit"
    // klappt ihn im Original auf und wieder zu - am laufenden BehavEd
    // beobachtet (27.09.): dreimal Edit auf standOnly, zu/auf/zu, kein
    // Fenster, keine Meldung.
    if (n != nullptr && n->kind == Node::Kind::Macro) {
        g_app->doc.vergibKennungen();
        const Node* k = nodeAt(g_app->doc.script(), p);
        if (k != nullptr) {
            g_app->expanded.setOpen(k->kennung, !g_app->expanded.isOpen(k->kennung));
            rebuildTree();
        }
        return;
    }
    if (n == nullptr || n->kind != Node::Kind::Command) {
        return;
    }
    // Dieselbe Wahl wie der Pruefer (selectOverload): Feldzahl, Block, die
    // Markierung /*@SET_TYPES*/ und die Art jedes Arguments.
    //
    // Hier stand nur die Feldzahl. Der Abgleich mit dem Original (jeder
    // Befehl geoeffnet, 27.09.) fand zwei Folgen: `set ( "variablename",
    // "value" )` oeffnete mit der SET_TYPES-Klappliste statt zwei
    // Textfeldern, und `wait ( "kyle1 line 2" )` - auf einen Task warten -
    // als Millisekundenfeld <float>. Das Original zeigt beide Male <str>.
    const Command* best = selectOverload(*n, g_app->db);
    if (best == nullptr) {
        return;
    }
    g_app->editorOpen = true;
    g_app->editorInsert = false;
    g_app->editorCmd = best;
    g_app->editorPath = p;
    g_app->editorValues.clear();
    g_app->editorIsExpr.clear();
    g_app->editorWarExpr.clear();
    g_app->editorKinds.clear();
    g_app->editorDefs.clear();
    g_app->editorRefreshFrom = -1;
    // Auch die Helferzeilen zuruecksetzen.
    //
    // Sie wurden nie geleert, und `helferGross` waechst nur. Wer an einem
    // `if` den Ausdruckshelfer benutzt hatte, fand beim NAECHSTEN `if`
    // dieselben Werte vor - Typ, Name, Marke und Bereich.
    //
    // shank zu rc540, mit zwei Bildern: erst
    //     STRING / SET_LEADER / ORIGIN / 0.000 .. 2.000
    // eingestellt, dann ein anderes `if` geoeffnet - und dort stand alles
    // wieder so. "It should reset."
    //
    // Leeren genuegt: `helferGross` fuellt beim naechsten Zeichnen mit den
    // Vorgaben FLOAT / SET_PARM1 / ORIGIN / 0.0 .. 1.0 nach.
    g_app->helpGetType.clear();
    g_app->helpGetName.clear();
    g_app->helpTagName.clear();
    g_app->helpTagType.clear();
    g_app->helpRangeLow.clear();
    g_app->helpRangeHigh.clear();
    // Jedes Oeffnen beginnt in der Mitte des Programmfensters. Die Mitte
    // wird im ersten Bild aus der Arbeitsflaeche gesetzt (drawEditor);
    // negativ heisst "noch nicht gesetzt".
    //
    // Die gemessene Reihe, die frueher hier auf null ging (rc545: sie
    // sickerte vom vorigen Befehl in den naechsten), ist seit rc568 nur
    // noch ein Protokollwert - die Breite wird vor `Begin` ausgerechnet.
    g_app->editorMitte = ImVec2{-1.0F, -1.0F};
    g_app->editorRowW = 0.0F;
    for (const Arg& a : n->args) {
        g_app->editorValues.push_back(a.text);
        g_app->editorIsExpr.push_back(a.kind == Arg::Kind::Expr ? 1 : 0);
        g_app->editorWarExpr.push_back(
            a.kind == Arg::Kind::Expr ? 1 : 0);
    }
    // Die Helferzeilen aus dem WERT zurueckgewinnen.
    //
    // Geleert werden sie oben weiter - sonst schleppte der naechste Befehl
    // die Einstellungen des vorigen mit, und genau das hat shank in rc541
    // gemeldet. Aber danach wird gelesen, was im Feld steht.
    //
    // shank zu rc552: "die werte die ich bei den einzelnen parameters
    // eintrage werden nicht mehr gespeichert."
    //
    // Stimmt: der HELFER baut aus seinen vier Zeilen einen Ausdruck und
    // schreibt ihn ins Feld. Das Feld wird gespeichert, die vier Zeilen
    // nicht - sie standen beim naechsten Oeffnen wieder auf den Vorgaben,
    // obwohl im Feld `get( FLOAT, "SET_PARM1" )` stand.
    //
    // Die drei Formen sind fest und werden an genau einer Stelle gebaut,
    // also lassen sie sich auch an genau einer Stelle zurueckgewinnen:
    //
    //     get( <TYP>, "<NAME>" )
    //     tag( "<NAME>", <ART> )
    //     random( <VON>, <BIS> )
    //
    // Was nicht in diese Formen passt - ein von Hand getippter Ausdruck -,
    // laesst die Helferzeilen auf ihren Vorgaben. Raten waere hier
    // schlimmer als nichts zu tun.
    helferAusEinstellungen(best->name);
    helferAusWert();
    // Was der Editor GELADEN hat - Werte, Ausdrucksmodus, Helferzeilen.
    //
    // shank: "hast du ueberall debug hinzugefuegt?" Fuer den Editor bisher
    // nicht: das Protokoll zeigte die Fenstergroesse und die Spaltenkanten,
    // aber nie die WERTE. Und genau um die ging es.
    {
        std::string z = "Editor auf \"" + best->name + "\":";
        for (std::size_t i = 0; i < g_app->editorValues.size(); ++i) {
            z += " [" + std::to_string(i) + "] \"" + g_app->editorValues[i] +
                 "\"" +
                 ((i < g_app->editorIsExpr.size() && g_app->editorIsExpr[i])
                      ? " (Ausdruck)"
                      : "");
            if (i < g_app->helpGetType.size()) {
                z += " Helfer=" + g_app->helpGetType[i] + "/" +
                     g_app->helpGetName[i] + "/" + g_app->helpTagType[i];
            }
        }
        diag::detail(z);
    }
}

std::string fileName(const std::string& path) {
    const std::size_t cut = path.find_last_of("\\/");
    return cut == std::string::npos ? path : path.substr(cut + 1);
}

std::string directoryOf(const std::string& path) {
    const std::size_t cut = path.find_last_of("\\/");
    return cut == std::string::npos ? std::string{} : path.substr(0, cut);
}

std::string slurp(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// --- Ein Bericht, den man anhaengt --------------------------------------
//
// Jede Runde des GPU-Umbaus begann damit, dass jemand eine Zahl vorlesen
// musste: welche Zustaende laufen, wie viele Aufrufe, welche Grafikkarte,
// was die Debugschicht sagt. Abgelesen wurde sie aus Bildschirmfotos, und
// zweimal stand die gesuchte Zahl in einem Menue, das beim Hinsehen zu war.
//
// Hier steht alles in einer Datei, neben dem Protokoll.
void schreibeBericht() {
    // --- Kein Knopf ------------------------------------------------------
    //
    // Die erste Fassung stand als "Bericht schreiben..." im View-Menue.
    // shanks Antwort darauf: "das bei view etwas drin ist um es ins log zu
    // schreiben ist bescheuert".
    //
    // Er hat recht, und die Begruendung steht seit rc425 im Quelltext ueber
    // der Debugschicht: "Ein Werkzeug, das man erst einschalten muss, ist
    // genau dann aus, wenn man es braucht." Dieselbe Ueberlegung, und ich
    // habe sie einen Bau lang nicht auf meinen eigenen Knopf angewandt.
    //
    // Der Bericht steht deshalb IM PROTOKOLL, und er wird ohne Zutun
    // geschrieben - beim Beenden und beim Absturz. Eine Datei weniger zum
    // Mitschicken.
    std::ostringstream f;
    f << "\n--- Bericht ---\n";
    // Die Fassung GANZ OBEN. Zu rc448 kam ein Protokoll zurueck, aus dem
    // sich nicht entscheiden liess, welches der beiden Pakete dieses
    // Namens gelaufen war - siehe include/bhed/fassung.h.
    f << "behaved 1.0.0-" << kFassung << ", gebaut " << __DATE__ << " "
      << __TIME__ << "\n";

    f << "\nKarte\n";
    f << "  Dreiecke     " << (g_app->mesh.indexes.size() / 3U) << "\n";
    f << "  Stapel       " << g_app->mesh.batches.size() << "\n";
    f << "  Texturen     " << g_app->textures.found << " gefunden, "
      << g_app->textures.missing << " fehlen\n";
    f << "  Feinheit     " << g_app->meshDetail << "\n";

    f << "\nAnsicht\n";
    f << "  Weg          Grafikkarte (Direct3D 11)"
      << (gpu::verfuegbar() ? "" : " - NICHT verfuegbar") << "\n";
    f << "  Helligkeit   " << g_app->mapBrightness << "\n";
    f << "  Grundlicht   " << g_app->mapMinLight << "\n";
    f << "  Effekte      " << (g_app->showEffects ? "an" : "aus") << "\n";
    f << "  Gluehen      " << (g_app->showGlow ? "an" : "aus") << "\n";

    f << "\nGrafikkarte\n";
    {
        // Der Text kommt aus gpumap - dort steht das Wissen ueber Geraet,
        // Zustaende, Shader und Zeiten, und es soll nicht zweimal existieren.
        std::istringstream is(gpu::berichtText());
        std::string zeile;
        while (std::getline(is, zeile)) {
            f << "  " << zeile << "\n";
        }
    }
    f << "  Aufrufe      " << g_app->gpuAufrufe << ", uebersprungen "
      << g_app->gpuUebersprungen << "\n";
    f << "               Karte " << g_app->gpuKarte
      << ", Effekte " << g_app->gpuEffekte
      << ", Mover " << g_app->gpuMover
      << ", Figuren " << g_app->gpuFiguren2
      << ", Gluehen " << g_app->gpuGluehen << "\n";
    if (!g_app->gpuFehler.empty()) {
        f << "  FEHLER       " << g_app->gpuFehler << "\n";
    }

    // --- Die Debugschicht ------------------------------------------------
    //
    // Diese Meldungen gab es seit rc425, gelesen hat sie niemand: die
    // Schicht schreibt an den Windows-Debugger. Was seither aufgelaufen
    // ist, steht hier.
    // --- Auffaellige Kartenstapel ----------------------------------------
    //
    // Die schwarzen Vierecke auf dem Hologrammtisch waren Kartenflaechen
    // (holo1, holo11, holo111). Drei Runden lang habe ich bei den Figuren
    // gesucht, weil es dort eine Liste gab und hier keine.
    f << "\nAuffaellige Kartenstapel\n";
    {
        const std::vector<gpu::StapelNotiz> n = gpu::auffaelligeStapel();
        if (n.empty()) {
            f << "  keine\n";
        } else {
            for (const gpu::StapelNotiz& z : n) {
                f << "  " << z.anzahl << "x  " << z.shader << "  ["
                  << z.was << "]\n";
            }
        }
    }

    f << "\nFlaechen der Figuren\n";
    if (g_app->figurArten.empty()) {
        f << "  keine\n";
    } else {
        for (const auto& [z, n] : g_app->figurArten) {
            f << "  " << n << "x  " << z << "\n";
        }
    }

    f << "\nMeldungen der Debugschicht\n";
    if (g_app->d3dMeldungen.empty()) {
        f << "  keine\n";
    } else {
        for (const std::string& z : g_app->d3dMeldungen) {
            f << "  " << z << "\n";
        }
    }

    // Zeilenweise ins Protokoll, damit die Einrueckung des Ablaufs stimmt.
    std::istringstream is(f.str());
    std::string zeile;
    while (std::getline(is, zeile)) {
        diag::info(zeile);
    }
}

void addStatus(const std::string& text) {
    char buf[400];
    std::snprintf(buf, sizeof(buf), "(%d) : %s",
                  static_cast<int>(g_app->statusLines.size()), text.c_str());
    g_app->statusLines.emplace_back(buf);
    // Und ins Protokoll, wortgleich.
    //
    // Bisher stand die Statusleiste NUR auf dem Schirm. In shanks
    // rc537-Protokoll steht zweimal `Ziel gleich Herkunft - nichts getan`,
    // also hat er zweimal die neue Meldung aus rc536 gesehen - vermutlich.
    // Nachpruefen liess es sich nicht, und er selbst konnte hinterher
    // nicht nachlesen, was ihm gesagt wurde.
    //
    // Wer blind entwickelt und Protokolle liest, muss darin sehen, was der
    // Benutzer gelesen hat. Sonst deutet man die Haelfte.
    diag::detail("Statuszeile: " + text);
}

void setStatus(const char* fmt, int a, int b) {
    char buf[240];
    std::snprintf(buf, sizeof(buf), fmt, a, b);
    addStatus(buf);
}

void rescanGamePaths() {
    diag::Step step("Spielordner durchsuchen");
    // rc604: zuerst das Regelwerk - es bestimmt, welche Spielordner gelten
    // (Jedi Outcast hat eigene) und wie Ghoul2-Modelle gelesen werden
    // (JK2-Skelett); die Zwischenspeicher unten werden dafuer geleert.
    regelwerkAnwendenJetzt();
    g_app->gamePaths.clear();

    // Alles verwerfen, was AUS den Spielordnern stammt.
    //
    // Die NPC-Liste wurde einmal gelesen und nie wieder - kommt der Ordner
    // mit den Modellen erst danach dazu, bleibt sie fuer immer leer, und
    // keine Figur bekommt ein Modell. Genau so sieht es aus, als suche das
    // Programm nur in der einen gewaehlten .pk3.
    g_app->npcMapRead = false;
    g_app->npcMap.clear();
    // Dasselbe gilt fuer die Shadertabelle und die geladenen Modelle: beide
    // stammen aus den Archiven der Spielordner. Wer einen Ordner hinzufuegt,
    // bekaeme sonst weiter die alte Antwort - und zwar genau die, wegen der
    // er den Ordner hinzugefuegt hat.
    g_app->shaderMapRead = false;
    g_app->shaderMap.clear();
    g_app->modelCache.clear();
    // Die Bildtabelle GEHOERT dazu: ihre Bilder stammen aus denselben
    // Archiven. Wer einen Spielordner hinzufuegt, will die Textur von dort
    // sehen - und bekaeme sonst weiter die alte.
    g_app->texCache.clear();
    ++g_app->texCacheStand;
    // rc603 Zusammenfuehrung: die Stufenbilder der Figuren ebenso. Sie sind
    // nur schwach gehalten, leben aber weiter, solange noch jemand ihre
    // Texturen haelt (die Modellansicht, App::modelTextures, ueberlebt das
    // Neueinlesen). Ohne das bekaemen die neu geladenen Figuren das Bild aus
    // den ALTEN Spielordnern - nach einem Spielwechsel bei Load mission
    // (Movie Duels <-> JKA) kann derselbe Name ein anderes Bild sein.
    g_app->figurStufenBilder.clear();
    g_app->skeletons.clear();
    g_app->effects.clear();
    g_app->effectRunners.clear();
    // Die Liste LEEREN, nicht nur die Merker zuruecksetzen.
    //
    // loadActorModels laeuft nur, wenn die Zahl der Eintraege nicht zur Zahl
    // der Figuren passt. Ein blosses tried=false aendert die Zahl nicht - es
    // liefe also nie wieder, und die Modelle blieben aus, auch nachdem der
    // richtige Spielordner dazugekommen ist.
    g_app->actorAssets.clear();
    g_app->actorWhy.clear();
    // ROFF-Bahnen ebenso - auch die gemerkten Fehlschlaege: der neue Ordner
    // bringt die Datei vielleicht mit.
    g_app->roffs.clear();
    // rc603: die geltenden Spielordner - die dieser Sitzung, wenn eine
    // Mission eines anderen Spiels geladen ist, sonst die eingerichteten.
    // rc603 (Durchsicht): welche durchsucht wurden, auch die ohne .pk3
    // (setzeSitzungsSpielordner vergleicht damit).
    g_app->eingeleseneSpielordner = aktiveSpielordner();
    g_app->spielordnerEingelesen = true;
    for (const std::string& dir : aktiveSpielordner()) {
        GamePath gp;
        if (scanGamePath(dir, gp)) {
            diag::info(dir + ": " + std::to_string(gp.archives.size()) + " Archive, " +
                       std::to_string(gp.fileCount) + " Dateien");
            g_app->gamePaths.push_back(std::move(gp));
        } else {
            diag::warn("keine .pk3 in " + dir);
        }
    }
    // Ist schon eine Karte geladen, ihre Texturen neu suchen und die Szene
    // (Effekte, Modelle, Figuren) neu aufbauen - oben wurde alles geleert.
    // Vorher blieben nach dem Hinzufuegen eines Spielordners die fehlenden
    // Texturen bis zum Neuladen der Karte fehlend (Kartentest 27.09.).
    if (!g_app->geo.empty()) {
        loadMapTextures();
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
    }
}

void refreshPk3List() {
    g_app->pk3Files.clear();
    std::vector<std::string> ext;
    switch (g_app->pk3Kind) {
        case 1: ext = {".bsp"}; break;
        case 2: ext = {".glm"}; break;
        default: ext = {".txt", ".ibi"}; break;
    }
    for (const GamePath& gp : g_app->gamePaths) {
        for (const FoundFile& f : findByExtension(gp, ext)) {
            // Nach Archiv filtern, wenn eines gewaehlt ist.
            if (!g_app->pk3Archive.empty() &&
                f.archive.find(g_app->pk3Archive) == std::string::npos) {
                continue;
            }
            g_app->pk3Files.push_back(f);
        }
    }
}

bool readFromArchives(const std::string& name, std::string& out,
                      std::string* fromArchive) {
    // RUECKWAERTS durch die Spielordner - der SPAETERE gewinnt.
    //
    // Das war ein handfester Fehler, und er hat still das halbe Programm
    // betroffen. Hier lief die Schleife vorwaerts, also gewann "base" gegen
    // "MD" - und damit das Originalspiel gegen den Mod.
    //
    // Die Engine haelt es umgekehrt, code/qcommon/files.cpp,
    // FS_AddGameDirectory():
    //
    //     search->next = fs_searchpaths;
    //     fs_searchpaths = search;
    //
    // Jedes Archiv wird VORN angehaengt. Da sie aufsteigend sortiert
    // hinzukommen (qsort mit paksort) und fs_game NACH fs_basegame an die
    // Reihe kommt, steht am Ende ganz vorn: das alphabetisch letzte Archiv
    // des ZULETZT hinzugefuegten Ordners. Also gewinnt der Mod.
    //
    // Was das ausgemacht hat, an einem Beispiel: Movie Duels ersetzt
    // models/players/_humanoid/_humanoid.gla durch eine Fassung mit 30384
    // Bildern. Geladen wurde die des Originalspiels mit 21376 - jede
    // Animation jenseits davon fehlte, und zwar ohne jede Meldung.
    for (auto gp = g_app->gamePaths.rbegin(); gp != g_app->gamePaths.rend();
         ++gp) {
        for (auto a = gp->archives.rbegin(); a != gp->archives.rend(); ++a) {
            if (const Pk3Entry* e = a->find(name)) {
                std::string err;
                if (readPk3File(*a, *e, out, &err)) {
                    if (fromArchive != nullptr) {
                        *fromArchive = a->path;
                    }
                    return true;
                }
                diag::warn("Auspacken fehlgeschlagen: " + name + " - " + err);
            }
        }
    }
    return false;
}

// Wie lange dauert ein Klang? -1, wenn er nicht zu finden ist.
//
// Gebraucht, damit die Zeitleiste weiss, WANN ein Klang noch laeuft. Ohne
// diese Zahl blieb endMs gleich startMs, und die Bedingung "laeuft gerade"
// war nie erfuellt - deshalb stieg der Klang beim Wiedereinschalten nicht
// ein, sondern man wartete bis zum naechsten.
//
// Das Ergebnis wird gemerkt: dieselbe Datei mehrfach zu entziffern, nur um
// ihre Laenge zu erfahren, waere Verschwendung - und beim Abspielen faellt
// so etwas sofort auf.
double soundLengthMs(const std::string& name) {
    if (name.empty()) {
        return -1.0;
    }
    const auto merk = g_app->soundLen.find(name);
    if (merk != g_app->soundLen.end()) {
        return merk->second;
    }
    double ms = -1.0;
    std::string data;
    if (findSoundData(name, data)) {
        const sound::Sound s = sound::decode(
            reinterpret_cast<const unsigned char*>(data.data()), data.size());
        if (s.ok && s.sampleRate > 0 && s.channels > 0) {
            ms = static_cast<double>(s.samples.size()) /
                 static_cast<double>(s.channels) /
                 static_cast<double>(s.sampleRate) * 1000.0;
        }
    }
    g_app->soundLen[name] = ms;
    return ms;
}

// Einen Klang entschluesselt und gepuffert holen.
//
// Fuer die Mundanimation: sie braucht die Abtastwerte in JEDEM Bild, und
// eine .wav je Bild neu zu entschluesseln waere nicht tragbar.
//
// Steht neben playSound und damit AUSSERHALB des namenlosen Namensraums -
// app_view3d.cpp braucht sie. Genau der Fallstrick aus Abschnitt 4.2 des
// rc241-Handouts; lint_link.py meldet ihn vor dem Binder.
const sound::Sound* soundFor(const std::string& name) {
    if (name.empty()) {
        return nullptr;
    }
    const auto have = g_app->soundCache.find(name);
    if (have != g_app->soundCache.end()) {
        return have->second.ok ? &have->second : nullptr;
    }
    sound::Sound s;
    std::string data;
    if (findSoundData(name, data)) {
        s = sound::decode(reinterpret_cast<const unsigned char*>(data.data()),
                          data.size());
    }
    // AUCH den Fehlschlag merken. Sonst wird in jedem Bild erneut in
    // vierzig Archiven gesucht.
    const auto put = g_app->soundCache.emplace(name, std::move(s));
    return put.first->second.ok ? &put.first->second : nullptr;
}

const std::vector<std::int16_t>* klangFuerWiedergabe(const std::string& name) {
    const auto have = g_app->klangPuffer.find(name);
    if (have != g_app->klangPuffer.end()) {
        return &have->second;
    }
    const sound::Sound* s = soundFor(name);   // entpackt, ebenfalls gemerkt
    if (s == nullptr) {
        return nullptr;
    }
    constexpr int kRate = 44100;
    constexpr int kChannels = 2;
    const auto put = g_app->klangPuffer.emplace(
        name, sound::convert(s->samples, s->sampleRate, s->channels, kRate, kChannels));
    return &put.first->second;
}

// Die im Hintergrund geoeffneten Tongeraete uebernehmen (siehe
// App::tonVorab). `warten`: vor dem ersten Klang - dann blockiert es nur,
// wenn der Klang in den ersten Sekundenbruchteilen nach dem Start kommt.
// Ein Geraet, das schon offen ist, bleibt; das vorab geoeffnete schliesst
// dann sein Destruktor.
void tonUebernehmen(bool warten) {
    if (!g_app->tonVorab.valid()) {
        return;
    }
    if (!warten && g_app->tonVorab.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        return;
    }
    App::TonVorab t = g_app->tonVorab.get();
    if (!g_app->audio.ready() && t.ton.ready()) { g_app->audio = std::move(t.ton); }
    if (!g_app->musicAudio.ready() && t.musik.ready()) { g_app->musicAudio = std::move(t.musik); }
}

void playSound(const std::string& name, double abMs, bool background,
               float lautstaerke, float rechts, std::uint64_t schluessel) {
    if (name.empty()) {
        return;
    }
    if (rechts < 0.0F) {
        rechts = lautstaerke;   // ohne Raum: beide Ohren gleich
    }
    // rc601 ton: mitschreiben, BEVOR Stille oder `stumm` aussteigen - der
    // Selbsttest soll sehen, was gespielt worden waere.
    klangMitschreiben(background ? 'M' : 'E', std::string{}, name, lautstaerke, rechts, schluessel, abMs);
    if (lautstaerke <= 0.001F && rechts <= 0.001F) {
        // Auch ein unhoerbarer Satz schneidet den vorigen ab.
        if (schluessel != 0 && g_app->audio.ready()) {
            (void)g_app->audio.play({}, 0.0F, 0.0F, schluessel);
        }
        return;   // zu weit weg, um gehoert zu werden
    }
    diag::Step step("Klang abspielen: " + name);
    // Aus dem Zwischenspeicher: einmal gesucht, entpackt und umgerechnet.
    const std::vector<std::int16_t>* fertig = klangFuerWiedergabe(name);
    if (fertig == nullptr) {
        step.fail("nicht gefunden");
        // Die Meldung wurde gebaut und dann weggeworfen - man hoerte nur
        // nichts und wusste nicht, warum (Kartentest 27.09.). Jetzt in die
        // Statusliste, je Datei einmal, damit das Abspielen sie nicht flutet.
        static std::set<std::string> gemeldet;
        if (gemeldet.insert(name).second) {
            char msg[300];
            std::snprintf(msg, sizeof(msg), tr(Str::SoundNotFound), name.c_str());
            addStatus(msg);
        }
        return;
    }
    // Festes Geraeteformat, alles wird umgerechnet. Begruendung steht in
    // bhed/sound.h: waveOut lehnt ungewoehnliche Raten je nach Treiber ab,
    // und ein Geraet, das je Klang neu geoeffnet wird, wuergt den vorigen ab.
    constexpr int kRate = 44100;
    constexpr int kChannels = 2;
    // Musik auf ein EIGENES Geraet - siehe musicAudio in app_internal.h.
    // Auf einem gemeinsamen Geraet reiht waveOut die Puffer hintereinander,
    // und eine minutenlange Musik haelt jede Stimme dahinter auf.
    tonUebernehmen(true);
    Audio& geraet = background ? g_app->musicAudio : g_app->audio;
    if (!geraet.open(kRate, kChannels)) {
        addStatus(tr(Str::SoundNoDevice));
        return;
    }
    // Ein neues Stueck ERSETZT das laufende. Genau das tut die Engine auch:
    // SetConfigstring(CS_MUSIC) schaltet den Hintergrundstrom um, es
    // laufen nie zwei Stuecke uebereinander.
    if (background) {
        geraet.stopAll();
    }
    std::vector<std::int16_t> puffer = *fertig;
    // Die Daempfung gibt der MISCHER an, nicht wir hier.
    //
    // Vorher wurde der Puffer selbst leiser gerechnet. Das ist zwar
    // dasselbe Ergebnis, aber es verschenkt die Genauigkeit: der Mischer
    // summiert in int und begrenzt erst danach, waehrend ein vorher
    // halbierter int16-Puffer schon Stellen verloren hat.
    // MITTEN hinein einsteigen.
    //
    // Gebraucht, wenn der Ton waehrend des Abspielens wieder eingeschaltet
    // wird: der laufende Klang hat dann schon begonnen. Ihn von vorn zu
    // spielen waere gegenueber dem Bild verschoben, ihn wegzulassen hiess
    // bis zum naechsten Klang Stille - beim Anwender fuenf Sekunden.
    //
    // Der Puffer liegt ohnehin ganz im Speicher, also genuegt es, vorn
    // abzuschneiden. Auf eine RAHMENgrenze runden: bei zwei Kanaeln liegen
    // links und rechts abwechselnd im Puffer, und ein Versatz um eine
    // ungerade Zahl vertauscht sie.
    if (abMs > 0.0) {
        const std::size_t rahmen =
            static_cast<std::size_t>(abMs / 1000.0 * kRate);
        const std::size_t ab =
            std::min(rahmen * static_cast<std::size_t>(kChannels),
                     puffer.size());
        puffer.erase(puffer.begin(),
                     puffer.begin() + static_cast<std::ptrdiff_t>(ab));
        if (puffer.empty()) {
            return;   // laengst vorbei
        }
    }
    // Im Selbsttest stumm: alles bis hierher laeuft (Suchen, Entpacken,
    // Umrechnen, Versatz), nur hoeren soll man es nicht - shank arbeitet
    // nebenher ("mach bitte den play sound button aus wenn du testest").
    if (!g_app->stumm) {
        if (schluessel != 0 || rechts != lautstaerke) {
            geraet.play(puffer, lautstaerke, rechts, schluessel);
        } else {
            geraet.play(puffer, lautstaerke);
        }
    }
    char msg[300];
    std::snprintf(msg, sizeof(msg), tr(Str::SoundPlayed), name.c_str());
}

// Musik wie S_StartBackgroundTrack: "music/intro.mp3 music/loop.mp3" - das
// Intro einmal, dann die Schleife endlos; ein einzelner Name ist die
// Schleife (target_play_music, g_target.cpp:1242; worldspawn "music",
// g_spawn.cpp:1682). Vorher wurde der ganze Text als EIN Dateiname
// gesucht, und zwei Namen fanden nie etwas.
//
// abMs: mitten hinein (Wiedereinstieg). Gepuffert wird bis 4 Minuten nach
// dem Einstieg - laenger laeuft keine Zwischensequenz am Stueck.
void spieleMusik(const std::string& spec, double abMs) {
    std::vector<std::string> teile;
    {
        std::string wort;
        for (const char c : spec + " ") {
            if (c == ' ' || c == '\t' || c == ';') {
                if (!wort.empty()) {
                    teile.push_back(wort);
                    wort.clear();
                }
            } else {
                wort += c;
            }
        }
    }
    if (teile.empty()) {
        return;
    }
    const std::vector<std::int16_t>* intro = teile.size() >= 2 ? klangFuerWiedergabe(teile[0]) : nullptr;
    const std::vector<std::int16_t>* schleife = klangFuerWiedergabe(teile.back());
    if (schleife == nullptr || schleife->empty()) {
        playSound(teile.back(), abMs, true);   // meldet "nicht gefunden"
        return;
    }
    constexpr int kRate = 44100;
    constexpr int kChannels = 2;
    tonUebernehmen(true);
    Audio& geraet = g_app->musicAudio;
    if (!geraet.open(kRate, kChannels)) {
        addStatus(tr(Str::SoundNoDevice));
        return;
    }
    geraet.stopAll();
    const std::size_t ab = static_cast<std::size_t>(std::max(0.0, abMs) / 1000.0 * kRate) * kChannels;
    const std::size_t bis = ab + static_cast<std::size_t>(240.0 * kRate) * kChannels;
    std::vector<std::int16_t> puffer;
    puffer.reserve(std::min<std::size_t>(bis, 64U << 20));
    if (intro != nullptr) {
        puffer.insert(puffer.end(), intro->begin(), intro->end());
    }
    while (puffer.size() < bis) {
        const std::size_t n = std::min(schleife->size(), bis - puffer.size());
        puffer.insert(puffer.end(), schleife->begin(), schleife->begin() + static_cast<std::ptrdiff_t>(n));
    }
    if (ab >= puffer.size()) {
        return;
    }
    puffer.erase(puffer.begin(), puffer.begin() + static_cast<std::ptrdiff_t>(ab));
    // rc601 ton: mit s_musicvolume (Vorgabe 0.25, snd_dma.cpp:458 MD) - die
    // Engine gibt die Musik mit diesem Faktor in S_RawSamples (snd_dma.cpp:2175
    // ff. MD), die Stimmen mit s_volumeVoice 1.0. Vorher 1.0: die Musik stand
    // um 12 dB zu laut gegen die Saetze.
    const float pegel = tonMusikPegel();
    klangMitschreiben('M', std::string{}, spec, pegel, pegel, 0, abMs);
    if (!g_app->stumm) {
        geraet.play(puffer, pegel);
    }
}

void playSoundSchleife(const std::string& name, double dauerMs, float lautstaerke, float rechts,
                       std::uint64_t schluessel) {
    if (rechts < 0.0F) {
        rechts = lautstaerke;
    }
    // rc601 ton: mitschreiben wie in playSound.
    klangMitschreiben('S', std::string{}, name, lautstaerke, rechts, schluessel);
    if (name.empty() || dauerMs <= 1.0 || (lautstaerke <= 0.001F && rechts <= 0.001F)) {
        return;
    }
    const std::vector<std::int16_t>* fertig = klangFuerWiedergabe(name);
    if (fertig == nullptr || fertig->empty()) {
        return;
    }
    constexpr int kRate = 44100;
    constexpr int kChannels = 2;
    tonUebernehmen(true);
    Audio& geraet = g_app->audio;
    if (!geraet.open(kRate, kChannels)) {
        addStatus(tr(Str::SoundNoDevice));
        return;
    }
    // Auf ganze Rahmen, sonst tauschen links und rechts; hoechstens zwei
    // Minuten, damit ein Sprung weit nach vorn keinen riesigen Puffer baut.
    const double sek = std::min(dauerMs, 120000.0) / 1000.0;
    const std::size_t werte =
        static_cast<std::size_t>(sek * kRate) * static_cast<std::size_t>(kChannels);
    std::vector<std::int16_t> puffer;
    puffer.reserve(werte);
    while (puffer.size() < werte) {
        const std::size_t n = std::min(fertig->size(), werte - puffer.size());
        puffer.insert(puffer.end(), fertig->begin(),
                      fertig->begin() + static_cast<std::ptrdiff_t>(n));
    }
    if (!g_app->stumm) {
        if (schluessel != 0 || rechts != lautstaerke) {
            geraet.play(puffer, lautstaerke, rechts, schluessel);
        } else {
            geraet.play(puffer, lautstaerke);
        }
    }
}

// Die Textur zu EINEM Shadernamen laden.
//
// Herausgezogen aus loadMapTextures, damit die .md3-Modelle denselben Weg
// gehen: erst die Datei direkt versuchen, dann ueber das Shaderskript, und
// am Ende Bewegung und Mischart mitnehmen. Zwei getrennte Wege dafuer
// waeren zwei Gelegenheiten, unterschiedlich falsch zu liegen.
// "Nicht gefunden" ist keine Antwort - man will wissen, WO man suchen muss.
//
// Wenn eine Textur fehlt, hat das in der Praxis drei Gruende:
//
//   1. Das Archiv, in dem sie liegt, ist nicht installiert. Sehr haeufig:
//      eine Karte aus MD_Missions_Ep3 benutzt eine Textur aus
//      MD_Maps_Ep3, und wer nur die Missionen hat, hat die Textur nicht.
//   2. Sie heisst anders, als der Shader sagt - anderer Ordner, andere
//      Schreibweise.
//   3. Es gibt sie wirklich nirgends.
//
// Die drei lassen sich unterscheiden, indem man ueber ALLE Archive nach
// dem blossen DATEINAMEN sucht, ohne Ordner und ohne Endung. Findet sich
// "lava" irgendwo, ist es Fall 2 und man sieht sofort wo. Findet sich gar
// nichts, ist es Fall 1 oder 3 - und die Zahl der durchsuchten Archive
// beantwortet nebenbei die Frage, ob ueberhaupt ueberall gesucht wurde.
void reportMissingTexture(const std::string& shaderName) {
    if (!diag::detailOn()) {
        return;
    }
    std::string basis = shaderName;
    const std::size_t slash = basis.find_last_of('/');
    if (slash != std::string::npos) {
        basis = basis.substr(slash + 1);
    }
    for (char& c : basis) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    int archive = 0;
    int treffer = 0;
    for (const GamePath& gp : g_app->gamePaths) {
        for (const Pk3& a : gp.archives) {
            ++archive;
            for (const Pk3Entry& e : a.entries) {
                std::string low = e.name;
                for (char& c : low) {
                    c = static_cast<char>(
                        std::tolower(static_cast<unsigned char>(c)));
                }
                const std::size_t sl = low.find_last_of('/');
                const std::string nur =
                    (sl == std::string::npos) ? low : low.substr(sl + 1);
                const std::size_t dot = nur.find_last_of('.');
                const std::string ohne =
                    (dot == std::string::npos) ? nur : nur.substr(0, dot);
                if (ohne != basis) {
                    continue;
                }
                if (treffer < 6) {
                    diag::detail("  gleicher Dateiname in " + fileName(a.path) +
                                 ": " + e.name);
                }
                ++treffer;
            }
        }
    }
    if (treffer == 0) {
        diag::detail("  -> NICHT GEFUNDEN. \"" + basis +
                     "\" kommt in KEINEM der " + std::to_string(archive) +
                     " durchsuchten Archive vor - das Archiv mit dieser "
                     "Textur ist wahrscheinlich nicht installiert.");
    } else {
        diag::detail("  -> NICHT GEFUNDEN, aber der Dateiname kommt " +
                     std::to_string(treffer) +
                     " mal vor (siehe oben) - dann stimmt der ORDNER nicht.");
    }
}

bool loadTextureFor(const std::string& shaderName, TextureSet::Tex& out) {
    // Wie gross duerfen Texturen sein?
    //
    // 256 war zu klein. Die Lava von Mustafar ist 1024x1024; auf 256
    // geschrumpft und dann nah betrachtet sieht sie kloetzchenhaft aus -
    // genau das war zu sehen. 512 vervierfacht den Speicher je Textur,
    // aber bei rund hundert Texturen je Karte sind das etwa 100 MB statt
    // 25 MB, und das ist heute keine Groesse mehr.
    //
    // Warum nicht die volle Aufloesung: die Texturen werden bei JEDEM
    // Bildpunkt angesprochen, und was nicht in den Zwischenspeicher des
    // Rechners passt, kostet mehr als es bringt. 512 ist der Punkt, an dem
    // eine Wand aus normalem Abstand nicht mehr auffaellt.
    constexpr int kMaxSize = 512;
    ShaderMap& shaderMap = g_app->shaderMap;
        // --- Das Skript geht vor der Datei ----------------------------
        //
        // R_FindShader (tr_shader.cpp:3433) sucht ZUERST im Shadertext
        // (FindShaderInShaderText) und greift erst zur Bilddatei, wenn dort
        // nichts steht. Ein Shadername mit Skript faellt also NIE auf eine
        // gleichnamige Datei zurueck.
        //
        // behaved hat es umgekehrt gemacht - Datei zuerst. Solange beide
        // dasselbe Bild meinen, faellt das nicht auf. Wo sie es nicht tun,
        // entsteht ein Zwitter: das BILD kommt aus der Datei, aber
        // `blend`, `texGen`, `alphaConst` und `rgbWave` kommen aus der
        // ersten Stufe des SKRIPTS. Die beiden gehoeren nicht zusammen.
        //
        // So entstand die Verschlechterung an den Tempelsaeulen in rc348:
        // textures/jeditemple/pillarbase hat als erste Stufe
        // textures/imperial/env_shiny_floor mit `tcGen environment`. Das
        // Bild kam aus pillarbase.jpg, die Spiegelkoordinaten aus der
        // Skriptstufe - Koordinaten einer Spiegelung auf einer
        // Steintextur.
        //
        // Gemessen ueber die 43 Karten: 1575 BSP-Shader, 644 mit Skript,
        // 146 davon mit einer ersten Bildstufe, die anders heisst als der
        // Shader - und bei 61 von ihnen liegt zusaetzlich eine Datei
        // gleichen Namens herum. Genau diese 61 waren Zwitter.
        //
        // Die Datei bleibt als RUECKFALL, falls das Bild der ersten Stufe
        // fehlt.
        std::vector<std::string> tries;
        std::string lowered = shaderName;
        for (char& c : lowered) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        const auto it = shaderMap.find(lowered);
        if (it != shaderMap.end() && !it->second.image.empty()) {
            tries = textureCandidates(it->second.image);
        }
        for (const std::string& c : textureCandidates(shaderName)) {
            tries.push_back(c);
        }

        std::string data;
        bool got = false;
        for (const std::string& name : tries) {
            // --- Schon entziffert? ------------------------------------
            //
            // Dieselbe Tabelle wie bei den Figuren (rc187), hier fuer die
            // Karte. Gemessen: 1887 ms fuer 103 Texturen, und mehrere
            // Shader zeigen regelmaessig auf dasselbe Bild.
            //
            // ZWEI Fallen stecken darin:
            //
            // 1. Die Karte verkleinert auf 512, die Figuren auf 256. Ein
            //    Eintrag allein nach dem Namen wuerde der Karte das kleine
            //    Bild unterschieben - die Lava von Mustafar saehe
            //    kloetzchenhaft aus. Deshalb steht die Groesse IM
            //    Schluessel.
            //
            // 2. An die fertige Textur haengen Shaderwerte: Scrollen,
            //    Blendmodus, Wellen. Die gehoeren zum SHADER, nicht zum
            //    Bild - zwei Shader koennen dasselbe Bild verschieden
            //    bewegen. Gemerkt werden darum nur die Bildpunkte; die
            //    Werte kommen danach jedes Mal frisch dazu.
            const std::string key =
                image::mappingName(name) + "@" + std::to_string(kMaxSize);
            g_app->kartenBilder.insert(key);
            TextureSet::Tex t;
            const auto hit = g_app->texCache.find(key);
            if (hit != g_app->texCache.end()) {
                t = hit->second;
                if (it != shaderMap.end()) {
                    // Dieselbe Meldung wie an der anderen Stelle - sonst
                    // fehlen im Protokoll genau die KARTEN-Shader, und
                    // `table_blue` (der Holotisch) tauchte nirgends auf.
                    if (it->second.imageStages > 1 ||
                        it->second.blend != BlendMode::Opaque) {
                        const char* wie =
                            (it->second.blend == BlendMode::Add) ? "additiv"
                            : (it->second.blend == BlendMode::AddAlpha)
                                ? "additiv-alpha"
                            : (it->second.blend == BlendMode::Alpha) ? "alpha"
                            : (it->second.blend == BlendMode::Filter)
                                ? "filter"
                                : "deckend";
                        diag::detail(
                            "Shader \"" + name + "\": " +
                            std::to_string(it->second.imageStages) +
                            " Bildstufe(n) gelesen, gezeichnet wird EINE (" +
                            wie + ")");
                    }
                    for (int k = 0; k < it->second.numTexMods; ++k) {
                        t.texMods[k] = it->second.texMods[k];
                    }
                    t.numTexMods = it->second.numTexMods;
                    t.blend = it->second.blend;
                    // Der Alphatest gehoert ZUM SHADER, genau wie blend -
                    // und er ist der Grund, warum die Geonosianer im
                    // schwarzen Kasten standen. Ihn hier zu vergessen
                    // heisst: der Kartenzeichner bekommt ihn nie zu sehen.
                    t.alphaTest = it->second.alphaTest;
                t.alphaConst = it->second.alphaConst;
                t.autosprite = it->second.autosprite;
                    t.cull = it->second.cull;
                    t.rgbWave = it->second.rgbWave;
                    t.alphaWave = it->second.alphaWave;
                    t.portalWeite = it->second.portalWeite;
                    t.istPortal = it->second.istPortal;
                    t.deformWave = it->second.deformWave;
                    t.deformSpread = it->second.deformSpread;
                    t.deform2 = it->second.deform2;
                    t.polygonOffset = it->second.polygonOffset;
                    // glow der Stufe, aus der auch das Bild kommt.
                    t.glow = it->second.glow;
                    t.clamp = it->second.clamp;
                    t.depthWrite = it->second.depthWrite;
                    t.depthFunc = it->second.depthFunc;
                    t.alphaGen = it->second.alphaGen;
                    for (int k = 0; k < 3; ++k) {
                        t.rgbConst[k] = it->second.rgbConst[k];
                    }
                    t.ausSkript = true;
                    t.lightmapStufe = it->second.lightmapStufe;
                    t.rgbVertex = it->second.rgbVertex;
                    t.srcFactor = it->second.srcFactor;
                    t.dstFactor = it->second.dstFactor;
                    // --- tcGen environment gilt NUR fuer Zusatzstufen ---
                    //
                    // Gemessen an duel_jeditemple: 43 Shader dort haben die
                    // Umgebungsspiegelung als ERSTE Stufe:
                    //
                    //     textures/jeditemple/pillar
                    //     {
                    //         { map textures/imperial/env_shiny_floor
                    //           tcGen environment }
                    //         { map textures/jeditemple/pillar
                    //           blendFunc GL_SRC_ALPHA GL_SRC_ALPHA }
                    //         { map $lightmap
                    //           blendFunc GL_DST_COLOR GL_ZERO }
                    //     }
                    //
                    // behaved nimmt die erste Bildstufe als DIE Textur der
                    // Flaeche. Hier ist das die Spiegelung - und die
                    // eigentliche Saeulentextur aus der zweiten Stufe wird
                    // nie gezeichnet, weil `GL_SRC_ALPHA GL_SRC_ALPHA`
                    // weder Add noch AddAlpha ist.
                    //
                    // Das ist ein VORHANDENER Fehler, nicht einer von
                    // tcGen. Aber die Spiegelkoordinaten machen ihn
                    // sichtbar: mit den Koordinaten aus der .bsp lag die
                    // Spiegeltextur wie eine gewoehnliche Wand und sah
                    // zufaellig plausibel aus; als echte Spiegelung wird
                    // sie glatt, und die Saeule verliert ihre Zeichnung.
                    //
                    // Solange die Stufenwahl falsch ist, ruecken richtige
                    // Koordinaten das Bild also WEITER vom Spiel weg.
                    // Deshalb bleibt die Basisstufe hier unangetastet -
                    // gemessen, nicht vermutet, und als offener Punkt
                    // notiert statt stillschweigend gedreht.

                }
                // Die Bildpyramide bauen - siehe Tex::buildMips. Ohne sie
                // bliebe alles bei Stufe 0 und flimmerte in der Ferne
                // weiter.
                t.buildMips();
            out = std::move(t);
                got = true;
                break;
            }

            if (!readFromArchives(name, data)) {
                continue;
            }
            // Format am INHALT erkennen, nicht an der Endung: in JKA-Mods
            // liegen regelmaessig JPEG-Dateien mit der Endung .tga.
            const image::Image im = image::decode(
                reinterpret_cast<const unsigned char*>(data.data()), data.size());
            if (!im.ok || im.width <= 0 || im.height <= 0) {
                continue;
            }
            image::shrinkTo(im, kMaxSize, t.width, t.height, t.rgba);
            // In die Tabelle kommen die BILDPUNKTE - vor den Shaderwerten,
            // damit der naechste Shader mit demselben Bild seine eigenen
            // bekommt.
            g_app->texCache[key] = t;
            // Die Bewegung aus dem Shader mitnehmen - sonst steht die Lava
            // still, obwohl das Bild stimmt.
            if (it != shaderMap.end()) {
                // --- Was von einem mehrstufigen Shader ankommt ---------
                //
                // Gemeldet: "der Tisch und das Modell dahinter sehen so
                // komisch aus, ingame sieht das anders aus."
                //
                // Ein Hologramm ist typisch MEHRSTUFIG - der Todesstern in
                // epiii_boc.shader hat vier Stufen, alle mit
                // `blendFunc GL_ONE GL_ONE`, die uebereinander addiert
                // werden.
                //
                // behaved zeichnet nur EINE Stufe, und zwar die zuletzt
                // gefundene. Aus vier aufeinandergelegten Bildern wird also
                // eines - das sieht zwangslaeufig anders aus.
                //
                // Diese Zeile macht sichtbar, WELCHE Flaechen davon
                // betroffen sind und wie viel verlorengeht. Nur die
                // auffaelligen, sonst stehen hier neuntausend Zeilen.
                if (it->second.imageStages > 1 ||
                    it->second.blend != BlendMode::Opaque) {
                    const char* wie =
                        (it->second.blend == BlendMode::Add) ? "additiv"
                        : (it->second.blend == BlendMode::AddAlpha)
                            ? "additiv-alpha"
                        : (it->second.blend == BlendMode::Alpha) ? "alpha"
                        : (it->second.blend == BlendMode::Filter)
                            ? "filter"
                            : "deckend";
                    diag::detail(
                        "Shader \"" + name + "\": " +
                        std::to_string(it->second.imageStages) +
                        " Bildstufe(n) gelesen, gezeichnet wird EINE (" +
                        wie + ")");
                }
                for (int k = 0; k < it->second.numTexMods; ++k) {
                    t.texMods[k] = it->second.texMods[k];
                }
                t.numTexMods = it->second.numTexMods;
                t.blend = it->second.blend;
                t.alphaTest = it->second.alphaTest;
                t.alphaConst = it->second.alphaConst;
                t.autosprite = it->second.autosprite;
                t.cull = it->second.cull;
                t.rgbWave = it->second.rgbWave;
                t.alphaWave = it->second.alphaWave;
                t.portalWeite = it->second.portalWeite;
                t.istPortal = it->second.istPortal;
                t.deformWave = it->second.deformWave;
                t.deformSpread = it->second.deformSpread;
                t.deform2 = it->second.deform2;
                t.polygonOffset = it->second.polygonOffset;
                t.glow = it->second.glow;
                t.clamp = it->second.clamp;
                t.depthWrite = it->second.depthWrite;
                t.depthFunc = it->second.depthFunc;
                t.alphaGen = it->second.alphaGen;
                for (int k = 0; k < 3; ++k) {
                    t.rgbConst[k] = it->second.rgbConst[k];
                }
                t.ausSkript = true;
                t.lightmapStufe = it->second.lightmapStufe;
                t.rgbVertex = it->second.rgbVertex;
                t.srcFactor = it->second.srcFactor;
                t.dstFactor = it->second.dstFactor;
            }
            t.buildMips();
            out = std::move(t);
            got = true;
            break;
        }
        return got;
}

// --- Die Wolkenschichten eines Himmelsshaders (rc592) -------------------
//
// RB_StageIteratorSky zeichnet erst die Box, dann ALLE Bildstufen des
// Shaders als Wolken auf der Kuppel (R_BuildCloudData). Bis rc591 fielen sie
// weg - der Himmel von duel_jedi_battleground stand ohne seine zwei
// ziehenden Schichten da. Jede Stufe wird ein eigener Eintrag hinter der
// Box, mit ihrer Mischart, ihrer tcMod-Kette und der Wolkenhoehe.
void wolkenLaden(const ShaderInfo& info, const std::string& name, TextureSet::Tex& himmel) {
    int zahl = 0;
    for (const ShaderInfo::Stage& st : info.stages) {
        if (st.lightmap || st.image.empty() || st.alphaGen == AlphaGen::Unsupported) {
            continue;
        }
        TextureSet::Tex wolke;
        if (!loadTextureFor(st.image, wolke)) {
            diag::detail("Wolkenschicht \"" + st.image + "\" von " + name + " nicht ladbar");
            continue;
        }
        wolke.blend = st.blend;
        wolke.srcFactor = st.srcFactor;
        wolke.dstFactor = st.dstFactor;
        wolke.alphaTest = st.alphaTest;
        wolke.alphaConst = st.alphaConst;
        wolke.alphaWave = st.alphaWave;
        wolke.portalWeite = st.portalWeite;
        wolke.alphaGen = st.alphaGen;
        for (int k = 0; k < st.numTexMods; ++k) {
            wolke.texMods[k] = st.texMods[k];
        }
        wolke.numTexMods = st.numTexMods;
        wolke.rgbWave = st.rgbWave;
        for (int k = 0; k < 3; ++k) {
            wolke.rgbConst[k] = st.rgbConst[k];
        }
        wolke.rgbVertex = st.rgbVertex;
        wolke.glow = st.glow;
        wolke.clamp = st.clamp;
        wolke.depthWrite = st.depthWrite;
        wolke.depthFunc = st.depthFunc;
        wolke.ausSkript = true;
        wolke.lightmapStufe = false;
        wolke.wolkenHoehe = info.wolkenHoehe;
        himmel.extraStages.push_back(static_cast<int>(g_app->textures.byShader.size()));
        g_app->textures.byShader.push_back(std::move(wolke));
        ++zahl;
    }
    if (zahl > 0) {
        diag::detail("Shader \"" + name + "\": " + std::to_string(zahl) + " Wolkenschicht(en), Hoehe " +
                     std::to_string(static_cast<int>(info.wolkenHoehe)));
    }
}

// rc604 fix604b (B1): EINE Zusatzstufe eines Shaders laden und an `t` haengen
// (Rumpf aus loadMapTextures herausgezogen, damit die Effektshader dieselben
// Stufen bekommen - RB_IterateStagesGeneric zeichnet jede Stufe, tr_shade.cpp).
void zusatzstufeLaden(const ShaderInfo& info, const ShaderInfo::Stage& st, const std::string& shaderName,
                      TextureSet::Tex& t) {
    if (st.alphaGen == AlphaGen::Unsupported) {
        diag::detail("Stufe \"" + st.image + "\" von " +
                     shaderName +
                     ": alphaGen nicht nachgebaut - bleibt"
                     " weg statt falsch");
        return;
    }
    TextureSet::Tex zusatz;
    if (!loadTextureFor(st.image, zusatz)) {
        diag::detail("Stufe \"" + st.image +
                     "\" von " + shaderName +
                     " nicht ladbar - wird uebersprungen");
        return;
    }
    // Die Stufe bringt ihre EIGENE Mischart mit, nicht die
    // des Shaders - darum geht es ja gerade.
    zusatz.blend = st.blend;
    zusatz.alphaTest = st.alphaTest;
    // --- `depthFunc equal` auf einer alphagetesteten Grundstufe
    //
    // shank, 02.10.: die Geonosier im Publikum der Arena
    // (textures/md_ga/geonosian1) standen in einem schwarzen
    // Viereck. Der Shader: Bild mit `alphaFunc GE128`, dann
    // DASSELBE Bild noch einmal multiplizierend mit
    // `depthFunc equal`. Im Spiel scheitert diese Stufe genau
    // dort, wo die erste den Bildpunkt verworfen und keine
    // Tiefe geschrieben hat. Unser Zeichner rechnet "equal"
    // als LESS_EQUAL (siehe gpumap_win32.cpp, holeTiefe) - die
    // Stufe lag VOR der Wand dahinter, bestand den Test und
    // multiplizierte die Wand mit dem schwarzen Rand des
    // Bildes. Mit demselben Bild ist der Alphatest der
    // Grundstufe genau die Maske, die das Spiel ueber die
    // Tiefe bekommt.
    if (st.depthFunc == DepthFunc::Equal && zusatz.alphaTest == AlphaTest::None &&
        t.alphaTest != AlphaTest::None &&
        image::mappingName(st.image) == image::mappingName(info.image)) {
        zusatz.alphaTest = t.alphaTest;
    }
    zusatz.alphaConst = st.alphaConst;
    for (int k = 0; k < st.numTexMods; ++k) {
        zusatz.texMods[k] = st.texMods[k];
    }
    zusatz.numTexMods = st.numTexMods;
    zusatz.rgbWave = st.rgbWave;
    zusatz.alphaWave = st.alphaWave;
    zusatz.portalWeite = st.portalWeite;
    // Die Stufe bringt ihr EIGENES glow mit. Bei
    // `table_blue` glueht genau diese dritte Stufe,
    // die beiden darunter nicht.
    zusatz.glow = st.glow;
    zusatz.texGen = st.texGen;
    for (int k = 0; k < 6; ++k) { zusatz.tcGenVec[k] = st.tcGenVec[k]; }   // rc605
    zusatz.clamp = st.clamp;
    zusatz.depthWrite = st.depthWrite;
    zusatz.depthFunc = st.depthFunc;
    zusatz.alphaGen = st.alphaGen;
    for (int k = 0; k < 3; ++k) {
        zusatz.rgbConst[k] = st.rgbConst[k];
    }
    // Eine Zusatzstufe ist immer geskriptet und bringt nie
    // die Lightmap mit - die gehoert der Grundstufe.
    zusatz.ausSkript = true;
    zusatz.lightmapStufe = false;
    zusatz.rgbVertex = st.rgbVertex;
    // --- deformVertexes gilt fuer den GANZEN Shader -------
    //
    // Es steht vor den Stufen, nicht in ihnen
    // (RB_DeformTessGeometry laeuft EINMAL je Flaeche, bevor
    // die Stufen gezeichnet werden). Bis hierher bekam nur
    // der Basiseintrag die Verformung.
    //
    // Die Folge war schwerer zu finden als der Fehler:
    // die Basisstufe verschob die Ecken entlang ihrer
    // Normalen, die Zusatzstufe nicht - also lagen ihre
    // Bildpunkte auf einer ANDEREN Tiefe und fielen durch
    // den Tiefenvergleich.
    //
    // Gemessen an textures/plasma_mustafar/lava
    // (`deformVertexes wave 180 sin 11 13 0 .4`):
    //
    //     mit Tiefentest        193 Bildpunkte
    //     ohne Tiefentest   103 059 Bildpunkte
    //
    // Gemeldet als "beim Magmafall sind es im Spiel zwei
    // Texturanimationen und bei uns nur eine" - die zweite
    // Stufe wurde schlicht nie sichtbar.
    zusatz.deformWave = info.deformWave;
    zusatz.deformSpread = info.deformSpread;
    zusatz.deform2 = info.deform2;
    zusatz.srcFactor = st.srcFactor;
    zusatz.dstFactor = st.dstFactor;
    // --- animMap: die uebrigen Bilder dazuladen ---------
    //
    // Jedes Bild bekommt einen VOLLSTAENDIGEN Eintrag mit
    // denselben Eigenschaften; der Zeichner tauscht dann
    // nur den Zeiger. Das kostet ein paar Zahlen je Bild
    // und spart dem Zeichner jede Sonderbehandlung.
    //
    // Das erste Bild ist `zusatz` selbst - deshalb steht
    // sein eigener Index als erster in animFrames, und
    // deshalb wird er VOR den uebrigen abgelegt.
    const int eigenerIndex =
        static_cast<int>(g_app->textures.byShader.size());
    zusatz.animFreq = st.animFreq;
    zusatz.animOneShot = st.animOneShot;
    if (st.animImages.size() > 1) {
        zusatz.animFrames.push_back(eigenerIndex);
    }
    t.extraStages.push_back(eigenerIndex);
    g_app->textures.byShader.push_back(std::move(zusatz));
    if (st.animImages.size() > 1) {
        for (std::size_t bi = 1; bi < st.animImages.size();
             ++bi) {
            TextureSet::Tex bild =
                g_app->textures.byShader[
                    static_cast<std::size_t>(eigenerIndex)];
            bild.rgba.clear();
            bild.mips.clear();
            if (!loadTextureFor(st.animImages[bi], bild)) {
                continue;
            }
            // Die Eigenschaften der Stufe uebernehmen, das
            // Bild ist das einzig Neue.
            TextureSet::Tex& erst =
                g_app->textures.byShader[
                    static_cast<std::size_t>(eigenerIndex)];
            bild.blend = erst.blend;
            bild.alphaTest = erst.alphaTest;
            bild.alphaConst = erst.alphaConst;
            bild.numTexMods = erst.numTexMods;
            for (int k = 0; k < erst.numTexMods; ++k) {
                bild.texMods[k] = erst.texMods[k];
            }
            bild.rgbWave = erst.rgbWave;
            bild.alphaWave = erst.alphaWave;
            bild.portalWeite = erst.portalWeite;
            bild.glow = erst.glow;
            bild.texGen = erst.texGen;
            for (int k = 0; k < 6; ++k) { bild.tcGenVec[k] = erst.tcGenVec[k]; }   // rc605
            bild.clamp = erst.clamp;
            bild.alphaGen = erst.alphaGen;
            for (int k = 0; k < 3; ++k) {
                bild.rgbConst[k] = erst.rgbConst[k];
            }
            bild.ausSkript = erst.ausSkript;
            bild.lightmapStufe = erst.lightmapStufe;
            bild.rgbVertex = erst.rgbVertex;
            bild.srcFactor = erst.srcFactor;
            bild.dstFactor = erst.dstFactor;
            bild.animFrames.clear();
            bild.animFreq = 0.0F;
            erst.animFrames.push_back(static_cast<int>(
                g_app->textures.byShader.size()));
            g_app->textures.byShader.push_back(
                std::move(bild));
        }
    }
}

// rc604 fix604b (B1): animMap auf der BASISSTUFE - die uebrigen Bilder laden
// (aus loadMapTextures herausgezogen; die Effektshader brauchen es ebenso:
// gfx/exp/explosion1 ist eine oneshotanimmap, RB_CalcAnimMap/R_BindAnimatedImage
// waehlt das Bild nach tess.shaderTime, tr_shade.cpp). eigenerIndex: der Platz
// von `t` in byShader.
void basisBildfolgeLaden(const ShaderInfo& info, int eigenerIndex, const std::string& shaderName,
                         TextureSet::Tex& t) {
    if (info.animImages.size() <= 1) { return; }
    t.animFreq = info.animFreq;
    t.animOneShot = info.animOneShot;
    t.animFrames.push_back(eigenerIndex);
    for (std::size_t bi = 1;
         bi < info.animImages.size(); ++bi) {
        TextureSet::Tex bild;
        if (!loadTextureFor(info.animImages[bi],
                            bild)) {
            continue;
        }
        bild.blend = t.blend;
        bild.alphaTest = t.alphaTest;
        bild.alphaConst = t.alphaConst;
        bild.autosprite = t.autosprite;
        bild.cull = t.cull;
        bild.rgbWave = t.rgbWave;
        bild.alphaWave = t.alphaWave;
        bild.portalWeite = t.portalWeite;
        bild.deformWave = t.deformWave;
        bild.deformSpread = t.deformSpread;
        bild.deform2 = t.deform2;
        bild.polygonOffset = t.polygonOffset;
        bild.glow = t.glow;
        bild.clamp = t.clamp;
        bild.alphaGen = t.alphaGen;
        bild.numTexMods = t.numTexMods;
        for (int k = 0; k < t.numTexMods; ++k) {
            bild.texMods[k] = t.texMods[k];
        }
        for (int k = 0; k < 3; ++k) {
            bild.rgbConst[k] = t.rgbConst[k];
        }
        bild.ausSkript = t.ausSkript;
        bild.lightmapStufe = t.lightmapStufe;
        bild.rgbVertex = t.rgbVertex;
        bild.srcFactor = t.srcFactor;
        bild.dstFactor = t.dstFactor;
        // Die Zusatzstufen gehoeren an die BASIS, nicht an
        // jedes Bild der Folge - sonst wuerden sie mehrfach
        // gezeichnet.
        t.animFrames.push_back(static_cast<int>(
            g_app->textures.byShader.size()));
        g_app->textures.byShader.push_back(std::move(bild));
    }
    diag::detail("Shader \"" + shaderName + "\": Bildfolge mit " +
                 std::to_string(t.animFrames.size()) +
                 " Bildern, " +
                 std::to_string(
                     static_cast<int>(info.animFreq)) +
                 " je Sekunde");
}

void loadMapTextures() {
    diag::Step step("Texturen suchen");
    // Was die vorige Karte benutzt hat - aufgeraeumt wird am Ende, wenn
    // feststeht, was die neue davon wieder braucht.
    const std::unordered_set<std::string> alteKartenBilder = std::move(g_app->kartenBilder);
    g_app->kartenBilder.clear();
    g_app->textures = TextureSet{};
    // rc604 surfaceSprites: die Stufen haengen an den Textureintraegen.
    g_app->grasStufen.clear();
    g_app->grasNetz = BspMesh{};
    g_app->grasFlachSchichten = 0;
    g_app->grasPlatzNull = 0;
    ++g_app->grasStand;
    // Die hochgeladenen Bilder hingen an den alten Nummern.
    gpu::vergissKarte();
    g_app->fehlendeTexturen.clear();
    // Die Zuordnungen zeigen in textures.byShader - wird die Liste neu
    // gebaut, sind die Nummern wertlos. Sie muessen MIT weg, sonst zeigt
    // ein Partikel die Textur einer Wand.
    g_app->effectShaders.clear();
    if (g_app->geo.shaders.empty() || g_app->gamePaths.empty()) {
        return;
    }

    // Alle .shader-Skripte EINMAL einlesen - und zwar dieselbe Tabelle, die
    // auch die Figurenmodelle benutzen. Bei shanks Installation sind es 40
    // Archive und 9014 Eintraege; sie zweimal aufzubauen kostet Sekunden und
    // bringt nichts.
    ShaderMap& shaderMap = g_app->shaderMap;
    if (!g_app->shaderMapRead) {
        g_app->shaderMapRead = true;
        // Auch hier gilt: der SPAETERE Spielordner gewinnt.
        //
        // parseShaderScript ueberschreibt gleichnamige Eintraege, also muss
        // der Mod ZULETZT gelesen werden - und dafuer die Ordner vorwaerts,
        // nicht rueckwaerts. Das ist genau umgekehrt zu readFromArchives,
        // und aus gutem Grund: dort gewinnt der ERSTE Treffer, hier der
        // LETZTE Eintrag. Beide Male soll der Mod gewinnen.
        //
        // Innerhalb eines Ordners ebenso: die Archive kommen aufsteigend,
        // und das alphabetisch letzte soll oben liegen - also auch hier
        // vorwaerts lesen.
        for (const GamePath& gp : g_app->gamePaths) {
            for (const FoundFile& f : findByExtension(gp, {".shader"})) {
                std::string text;
                if (readFromArchives(f.name, text)) {
                    parseShaderScript(text, shaderMap);
                    parseFogParms(text, g_app->nebelMap);
                }
            }
        }
        diag::info(std::to_string(shaderMap.size()) + " Shadereintraege gelesen");
    }

    g_app->textures.byShader.resize(g_app->geo.shaders.size());

    // --- Die Bilder NEBENLAEUFIG vorwaermen -----------------------------
    //
    // Gemessen: 1931 ms fuer 103 Texturen, und der Zwischenspeicher aus
    // rc188 hat daran NICHTS geaendert. Die Erklaerung liegt nahe - jeder
    // Shader zeigt auf ein eigenes Bild, es gibt also nichts zu sparen.
    // Die Zeit steckt im Entpacken und Entziffern einzigartiger Bilder,
    // und das ist reine Rechenarbeit.
    //
    // Reine Rechenarbeit ohne gemeinsamen Zustand laesst sich verteilen.
    // Erlaubt ist es hier, weil readPk3File seinen EIGENEN Dateizeiger
    // oeffnet (src/pk3.cpp) und weder die Archivliste noch die Shadertabelle
    // dabei veraendert werden - beide sind zu diesem Zeitpunkt fertig
    // aufgebaut und werden nur gelesen.
    //
    // Geschrieben wird erst HINTERHER, von einem Faden: jeder Arbeiter legt
    // seine Ergebnisse in ein eigenes Fach, und das Zusammenfuehren laeuft
    // der Reihe nach. Damit bleibt das Ergebnis unabhaengig von der Zahl
    // der Faeden - dieselbe Regel, die den Zeichner bytegleich haelt.
    {
        std::vector<std::string> namen;
        namen.reserve(g_app->geo.shaders.size());
        for (const BspShader& sh : g_app->geo.shaders) {
            if (sh.isDrawn()) {
                namen.push_back(sh.name);
            }
        }
        const unsigned kerne = std::max(1U, std::thread::hardware_concurrency());
        const std::size_t faeden =
            std::min<std::size_t>(kerne, std::max<std::size_t>(1, namen.size()));
        std::vector<std::vector<std::pair<std::string, TextureSet::Tex>>> faecher(
            faeden);
        std::atomic<std::size_t> naechste{0};
        const auto arbeit = [&](std::size_t fach) {
            for (;;) {
                const std::size_t k = naechste.fetch_add(1);
                if (k >= namen.size()) {
                    return;
                }
                // Dieselben Kandidaten wie in loadTextureFor, damit der
                // Treffer dort auch wirklich passt.
                std::vector<std::string> tries = textureCandidates(namen[k]);
                std::string lowered = image::mappingName(namen[k]);
                const auto it2 = shaderMap.find(lowered);
                if (it2 != shaderMap.end()) {
                    for (const std::string& c :
                         textureCandidates(it2->second.image)) {
                        tries.push_back(c);
                    }
                }
                for (const std::string& name : tries) {
                    // Liegt das Bild SCHON in der Tabelle? Dann gar nicht
                    // erst lesen.
                    //
                    // Genau hier steckten die 440 ms, die das Protokoll des
                    // Anwenders zeigte: beim Reiterwechsel stand
                    // "Bilder vorgewaermt: 0 neu" - und trotzdem dauerte
                    // "Texturen suchen" jedes Mal so lange wie beim ersten
                    // Mal. Der Grund: geprueft wurde erst beim EINSORTIEREN.
                    // Bis dahin waren alle 101 Texturen entpackt und
                    // entziffert - und wurden dann weggeworfen.
                    //
                    // Nur LESEN aus der Tabelle, und geschrieben wird erst
                    // hinterher von einem Faden. Damit bleibt es
                    // ungefaehrlich.
                    if (g_app->texCache.count(
                            image::mappingName(name) + "@512") != 0U) {
                        break;
                    }
                    std::string data;
                    if (!readFromArchives(name, data)) {
                        continue;
                    }
                    const image::Image im = image::decode(
                        reinterpret_cast<const unsigned char*>(data.data()),
                        data.size());
                    if (!im.ok || im.width <= 0 || im.height <= 0) {
                        continue;
                    }
                    TextureSet::Tex t;
                    image::shrinkTo(im, 512, t.width, t.height, t.rgba);
                    faecher[fach].emplace_back(
                        image::mappingName(name) + "@512", std::move(t));
                    break;
                }
            }
        };
        std::vector<std::thread> mannschaft;
        mannschaft.reserve(faeden);
        for (std::size_t f = 1; f < faeden; ++f) {
            mannschaft.emplace_back(arbeit, f);
        }
        arbeit(0);
        for (std::thread& th : mannschaft) {
            th.join();
        }
        std::size_t neu = 0;
        for (const auto& fach : faecher) {
            for (const auto& eintrag : fach) {
                // Auch vorgewaermte Bilder gehoeren zur Karte - sonst blieben
                // die, die loadTextureFor nie nachfragt, fuer immer liegen.
                g_app->kartenBilder.insert(eintrag.first);
                if (g_app->texCache.emplace(eintrag.first, eintrag.second)
                        .second) {
                    ++neu;
                }
            }
        }
        diag::info("Bilder vorgewaermt: " + std::to_string(neu) + " neu, " +
                   std::to_string(faeden) + " Faeden");
    }

    for (std::size_t i = 0; i < g_app->geo.shaders.size(); ++i) {
        const BspShader& sh = g_app->geo.shaders[i];
        if (!sh.isDrawn()) {
            continue;
        }
        TextureSet::Tex t;
        // --- Der Himmel ------------------------------------------------
        //
        // Ein Himmelsshader hat oft GAR KEINE eigene Textur - im Protokoll
        // stand deshalb "keine Textur fuer textures/skies/pmus", und die
        // Flaeche fiel ganz aus. Uebrig blieb der Verlaufshintergrund.
        //
        // Die sechs Seiten heissen <name>_rt _lf _bk _ft _up _dn, in genau
        // dieser Reihenfolge (ParseSkyParms). Sie ist nicht alphabetisch:
        // `bk` steht VOR `ft`.
        //
        // Die Ersatzregel stammt ebenfalls von dort: fehlt eine Seite, wird
        // die VORIGE genommen; fehlt gleich die erste, bleibt sie leer.
        // Das ist keine Schoenheit, sondern das Verhalten der Engine.
        {
            std::string lowSky = sh.name;
            for (char& c : lowSky) {
                c = static_cast<char>(
                    std::tolower(static_cast<unsigned char>(c)));
            }
            const auto sk = g_app->shaderMap.find(lowSky);
            if (sk != g_app->shaderMap.end() && !sk->second.skyBox.empty()) {
                static const char* kSuf[6] = {"rt", "lf", "bk",
                                              "ft", "up", "dn"};
                int letzte = -1;
                int geladen = 0;
                for (const char* suf : kSuf) {
                    TextureSet::Tex seite;
                    if (loadTextureFor(sk->second.skyBox + "_" + suf,
                                       seite)) {
                        letzte = static_cast<int>(
                            g_app->textures.byShader.size());
                        g_app->textures.byShader.push_back(std::move(seite));
                        ++geladen;
                    }
                    t.skyFaces.push_back(letzte);
                }
                diag::detail("Shader \"" + sh.name + "\": Himmelsbox " +
                             sk->second.skyBox + ", " +
                             std::to_string(geladen) + " von 6 Seiten");
                if (letzte >= 0) {
                    // Der Zeichner braucht eine nicht leere Textur, sonst
                    // gilt die Flaeche als texturlos. Die erste vorhandene
                    // Seite tut es - gezeichnet wird sie nie, die Farbe
                    // kommt aus skyFaces.
                    const TextureSet::Tex& ersteSeite =
                        g_app->textures.byShader[
                            static_cast<std::size_t>(t.skyFaces[0] >= 0
                                                         ? t.skyFaces[0]
                                                         : letzte)];
                    t.width = ersteSeite.width;
                    t.height = ersteSeite.height;
                    t.rgba = ersteSeite.rgba;
                    t.mips = ersteSeite.mips;
                    wolkenLaden(sk->second, sh.name, t);
                    g_app->textures.byShader[i] = std::move(t);
                    ++g_app->textures.found;
                    continue;
                }
                t.skyFaces.clear();
            }
        }
        if (loadTextureFor(sh.name, t)) {
            // --- Die Stufen DARUEBER mitladen -------------------------
            //
            // Gemessen an md_am_sith (rc343): von 70 Shadern haben 31 die
            // Redewendung "Lightmap, dann Textur multiplizieren" - die
            // rechnet der Zeichner ohnehin selbst. Wirklich fehlen die
            // Stufen darueber: das additive Leuchten von `table_blue` und
            // die vier- bis fuenfstufigen Bildschirme.
            //
            // Deshalb wird ab der Stufe geladen, die auf die erste echte
            // Bildstufe folgt - alles davor ist der Standardfall und
            // bleibt unangetastet. So aendert sich an den 31 nichts.
            std::string low = sh.name;
            for (char& c : low) {
                c = static_cast<char>(
                    std::tolower(static_cast<unsigned char>(c)));
            }
            const auto si = g_app->shaderMap.find(low);
            if (si != g_app->shaderMap.end()) {
                bool ersteEchte = true;
                for (const ShaderInfo::Stage& st : si->second.stages) {
                    if (st.lightmap || st.image.empty()) {
                        continue;   // $lightmap belegt keine eigene Textur
                    }
                    if (ersteEchte) {
                        ersteEchte = false;
                        if (st.ss.aktiv()) {
                            // Die Grundstufe selbst traegt surfaceSprites:
                            // RB_StageIteratorGeneric ueberspringt sie
                            // (tr_shade.cpp:1888-1892 MD), und die Bilder
                            // zeichnet es erst ab Stufe 1 (:2148-2156) - an
                            // Platz 0 also gar nichts. rc605: unsichtbar
                            // statt als Schicht (additiv mit Farbe 0 aendert
                            // keinen Bildpunkt und schreibt keine Tiefe) -
                            // etwa textures/system/customrainsurf auf
                            // duel_tower *13.
                            t.blend = BlendMode::Add;
                            t.srcFactor = BlendFactor::One;
                            t.dstFactor = BlendFactor::One;
                            t.rgbVertex = false;
                            t.rgbWave = Wave{};
                            for (float& c : t.rgbConst) { c = 0.0F; }
                            t.depthWrite = false;
                            t.glow = false;
                            ++g_app->grasPlatzNull;
                        }
                        continue;   // die steckt schon in `t`
                    }
                    // --- rc604: surfaceSprites NIE als Schicht -------
                    //
                    // RB_StageIteratorGeneric ueberspringt die Stufe
                    // (tr_shade.cpp:1888 MD, "We check for surfacesprites
                    // AFTER drawing everything else") und zeichnet sie
                    // danach als Halme (RB_DrawSurfaceSprites). Bis rc604
                    // lag hier das Bild der Halme flach auf dem Boden.
                    if (st.ss.aktiv()) {
                        grasStufeLaden(si->second, st,
                                       static_cast<std::size_t>(&st - si->second.stages.data()), sh.name, i);
                        continue;
                    }
                    // --- NUR ADDITIVE Stufen -------------------------
                    //
                    // Gemeldet: "meine Lava ist jetzt schwarz."
                    //
                    // Und zwar zu Recht. Der Lavashader hat drei Stufen:
                    //
                    //     { map lava        rgbGen wave ... }
                    //     { map rock_3      blendFunc GL_DST_COLOR GL_ZERO }
                    //     { map rock_3_glow blendFunc GL_ONE GL_ONE  glow }
                    //
                    // Die mittlere MULTIPLIZIERT mit dunklem Fels. Ich habe
                    // sie zusaetzlich obendrauf gezeichnet - auf eine
                    // Flaeche, die behaveds Zeichner ohnehin schon mit
                    // Textur und Licht multipliziert hat. Zweimal
                    // multipliziert wird schwarz.
                    //
                    // Additive Stufen koennen das nicht: sie ADDIEREN nur
                    // Licht. Wird eine doppelt gezeichnet, ist es zu hell -
                    // aber nie schwarz, und es faellt sofort auf.
                    //
                    // Deshalb ab hier nur `GL_ONE GL_ONE`. Das ist genau
                    // das, was am Holotisch fehlte (das Leuchten von
                    // `table_blue`) und was die Bildschirme brauchen.
                    //
                    // Multiplikative Zusatzstufen bleiben damit offen. Sie
                    // richtig zu legen hiesse, den ganzen Stapel von unten
                    // aufzubauen statt auf das fertige Bild zu addieren -
                    // ein eigener Umbau, kein Zusatz.
                    //
                    // --- GL_SRC_ALPHA GL_ONE kommt dazu ---------------
                    //
                    // Additiv, aber mit dem Alphakanal gewichtet. Auch das
                    // kann nur HELLER machen, nie schwarz - die Begruendung
                    // oben gilt unveraendert.
                    //
                    // Aber nicht jede solche Stufe darf mit. Gemessen ueber
                    // die sechs pk3: von den 141 Weltstufen mit dieser
                    // Mischart brauchen
                    //
                    //     108   tcGen environment   (jetzt gebaut)
                    //      17   alphaGen lightingSpecular
                    //       5   alphaGen entity
                    //
                    // `lightingSpecular` ERSETZT den Alphakanal durch ein
                    // gerechnetes Glanzlicht aus Licht- und Blickrichtung.
                    // behaved kann das nicht. Die Stufe mit dem Alpha des
                    // BILDES zu zeichnen waere kein Naeherungswert, sondern
                    // etwas anderes: ein gleichmaessiger Schleier statt
                    // eines wandernden Glanzes. Dasselbe gilt fuer
                    // `entity`, dessen Wert von einer Spielfigur kommt, die
                    // es hier gar nicht gibt.
                    //
                    // Solche Stufen bleiben deshalb draussen - und das wird
                    // gemeldet, damit die Zahl im Protokoll steht statt in
                    // einer Vermutung.
                    //
                    // --- Jetzt kommen ALLE Bildstufen mit ---------------
                    //
                    // Bis rc349 gingen nur additive Stufen durch, mit der
                    // Begruendung, dass sie nur heller machen koennen und
                    // deshalb nie etwas ausloeschen. Das war eine
                    // Vorsichtsmassnahme gegen eine fehlende RECHNUNG, nicht
                    // gegen ein fehlendes Verstaendnis: es gab keinen
                    // allgemeinen Mischer.
                    //
                    // Seit rc349 gibt es ihn - gerechnet wird, was OpenGL
                    // rechnet, aus den beiden Faktoren von glBlendFunc. Und
                    // seit dieser Runde stimmt auch die REIHENFOLGE, weil
                    // nach dem Shader sortiert wird und nicht mehr je Stufe
                    // (siehe Prepared::sortBlend).
                    //
                    // Damit faellt der Grund weg. Gemessen ueber die
                    // Weltshader der neun pk3, was bisher unter den Tisch
                    // fiel:
                    //
                    //     597  alpha      alle weggelassen
                    //      39  filter     alle weggelassen
                    //      11  deckend    alle weggelassen
                    //
                    // 647 Bildstufen. Darunter die eigentliche
                    // Saeulentextur von textures/jeditemple/pillar, die im
                    // Spiel sichtbar ist und bei uns fehlte.
                    zusatzstufeLaden(si->second, st, sh.name, t);   // rc604 fix604b (B1)
                }
                if (!t.extraStages.empty()) {
                    diag::detail("Shader \"" + sh.name + "\": " +
                                 std::to_string(t.extraStages.size()) +
                                 " zusaetzliche Stufe(n) geladen");
                }
                // --- animMap auf der BASISSTUFE ---------------------------
                //
                // Beide Faelle kommen vor: `textures/asjc_coruscant/gras`
                // hat die Bildfolge als erste Stufe,
                // `textures/asjc_coruscant/rail_1` als dritte. Die
                // Zusatzstufen sind oben behandelt; hier die Basis.
                //
                // Das erste Bild steckt schon in `t` - deshalb kommt der
                // Index von `t` selbst zuerst in die Liste. Den kennt man
                // hier: es ist `i`, der Platz in byShader, an den `t`
                // gleich wandert.
                if (si != g_app->shaderMap.end()) {
                    basisBildfolgeLaden(si->second, static_cast<int>(i), sh.name, t);   // rc604 fix604b (B1)
                }
            }
            g_app->textures.byShader[i] = std::move(t);
            ++g_app->textures.found;
        } else {
            ++g_app->textures.missing;
            g_app->fehlendeTexturen.push_back(sh.name);
            diag::info("keine Textur fuer " + sh.name);
            reportMissingTexture(sh.name);
        }
    }
    diag::info(std::to_string(g_app->textures.found) + " gefunden, " +
               std::to_string(g_app->textures.missing) + " nicht gefunden");
    // Bilder der vorigen Karte, die diese nicht wieder benutzt, freigeben.
    // Alle Nutzer von loadTextureFor bekommen KOPIEN - es zeigt nichts
    // hinein.
    std::size_t weg = 0;
    for (const std::string& k : alteKartenBilder) {
        if (g_app->kartenBilder.count(k) == 0U) {
            weg += g_app->texCache.erase(k);
        }
    }
    if (weg != 0U) {
        diag::detail("Bilder der vorigen Karte freigegeben: " + std::to_string(weg));
    }
    g_app->mapDirty = true;
}

void loadMapFromBytes(const std::string& bytes, const std::string& shownPath) {
    std::string err;
    MapData m;
    if (!readBsp(bytes, m, &err)) {
        platform::showError(shownPath + "\n" + err, tr(Str::AppTitle));
        return;
    }
    m.path = shownPath;

    // Wer eine Karte ausdruecklich laedt, bekommt sie als EIGENE.
    //
    // Das ist der einzige Weg, auf dem ein Reiter zu einer kommt. Alle
    // anderen benutzen die des Arbeitsbereichs mit.
    if (g_app->activeTab >= 0 &&
        static_cast<std::size_t>(g_app->activeTab) < g_app->tabs.size()) {
        g_app->tabs[static_cast<std::size_t>(g_app->activeTab)].ownMap = true;
    }

    // --- Alles wegraeumen, was zur ALTEN Karte gehoerte -------------------
    //
    // Gemeldet: "dieses mesh objekt ist von der anderen map", und es blieb
    // bis zum Neustart stehen.
    //
    // `clearMap()` raeumt diese Listen - aber die wird beim WECHSEL gar
    // nicht gerufen: eine neue Karte laden ist kein Leeren. Also blieben
    // die aufgestellten Modelle der vorigen Karte im Bild, und das Schiff
    // von Mustafar stand auf Kamino.
    //
    // Betroffen ist alles, was aus den ENTITIES der Karte entsteht:
    // aufgestellte Modelle, laufende Effekte, die Zuordnung der
    // Effektshader. Die Dateicaches (md3Files, md3Meshes) bleiben - die
    // haengen am Dateinamen und nicht an der Karte, und sie noch einmal zu
    // lesen waere reine Verschwendung.
    // Die BRUSH-NETZE muessen mit weg, und zwar besonders dringend.
    //
    // Sie liegen nach UNTERMODELLNUMMER ab ("Netz 5"), nicht nach Namen.
    // Nach einem Kartenwechsel gibt es die Nummer 5 wieder - nur meint sie
    // ein voellig anderes Stueck Geometrie. Gemeldet als "es ist immer noch
    // was da von der anderen map, eventuell ein brush oder so": das waren
    // genau diese Netze, in der neuen Karte an fremder Stelle gezeichnet.
    //
    // Nach Namen abgelegte Caches (md3Files, md3Meshes, texCache) haben
    // dieses Problem nicht - derselbe Name meint dieselbe Datei. Nur
    // Nummern sind je Karte neu vergeben.
    const std::size_t vorherModelle = g_app->mapModels.size();
    const std::size_t vorherNetze = g_app->brushMeshes.size();
    // Alles, was aus DIESER Karte entsteht:
    g_app->mapModels.clear();       // aufgestellte .md3 (misc_model)
    g_app->nebenModelle.clear();    // _d1/_u1 und Bruchstuecke dazu
    g_app->effectShaders.clear();   // Zuordnung Effekt -> Shader
    g_app->effectRunners.clear();   // laufende fx_runner
    g_app->effects.clear();         // gelesene .efx
    g_app->brushMeshes.clear();     // Netze der Brush-Modelle
    // Und was auf NUMMERN in der Karte zeigt.
    //
    // pickedEntity ist ein Index in map.entities - nach dem Wechsel meint
    // dieselbe Zahl eine andere Entity, und die Anzeige zeigte Angaben, die
    // es in der neuen Karte gar nicht gibt.
    g_app->pickedEntity = -1;
    // Ebenso Auswahl, Animationsfenster und Wegkette (Code-Pruefung rc592).
    entityNummernVergessen();
    // Auch Kamera- und Schluesselwahl gehoeren zur alten Karte (selectedKey
    // wurde nirgends zurueckgesetzt und zeigte danach auf einen anderen Schluessel).
    g_app->cameraSelected = false;
    g_app->selectedKey = -1;
    g_app->hiddenEntities.clear();
    // Die Klickziele der vorigen Karte (wie clearMap, lint_mapstate).
    g_app->markers.clear();
    g_app->keyMarks.clear();
    g_app->actorMarks.clear();
    // Tueren und ihre Klaenge gehoeren zur alten Karte (wie clearMap).
    g_app->moverSim = MoverSim{};
    g_app->moverKlaenge.clear();
    // Und neu AUFBAUEN: Kartenmodelle und Effekte entstehen in
    // prepareScene(), das die Zeitleiste nur ruft, wenn camTrackValid fehlt.
    // Ohne diese Zeile blieben sie nach einem Kartenwechsel leer - nur die
    // erste Karte nach dem Start hatte sie (Kartenpruefung 27.09.: "Modelle
    // 0" bei jeder Karte). Die Kamerabahn loest ref_tags ueber die neuen
    // Entities auf, auch sie gilt nicht mehr.
    g_app->camTrackValid = false;
    if (vorherModelle != 0U || vorherNetze != 0U) {
        diag::detail("Karte gewechselt: " + std::to_string(vorherModelle) +
                     " aufgestellte Modelle und " + std::to_string(vorherNetze) +
                     " Brush-Netze der vorigen Karte entfernt");
    }

    // Waren die Entities dieser Karte ersetzt (.ent, Bearbeitung), gelten
    // die - nicht die der .bsp (entitiesJeKarte).
    if (const auto gemerkt = g_app->entitiesJeKarte.find(m.path); gemerkt != g_app->entitiesJeKarte.end()) {
        m.entities = gemerkt->second;
        collectNames(m);
        diag::detail(std::to_string(m.entities.size()) + " gemerkte Entities fuer " + m.path);
    }
    g_app->map = std::move(m);
    // Die Effektbahnen (samt Spurprotokoll) gehoeren zur alten Geometrie.
    vergissEffektBahnen();

    {
        diag::Step geoStep("Kartengeometrie lesen");
        BspGeometry geo;
        std::string gerr;
        // rc601 (bewegung, Wege): die Brushes fuer die Spuren der NPCs. Die
        // Lichtschwerter (Paket saber) nehmen dieselbe Kollisionskarte ueber
        // kollisionsSpeicher() (rc601 integ: kein zweites Laden mehr).
        kollisionLaden(bytes, shownPath);
        saberKarteGeladen();
        if (readBspGeometry(bytes, geo, &gerr)) {
            // Externe Lightmaps (q3map2 -external) VOR dem Netz: die Stapel
            // haengen an der Lightmapnummer. tr.worldDir ist "maps/<karte>".
            {
                std::string karte = shownPath;
                for (char& c : karte) { if (c == '\\') { c = '/'; } }
                const std::size_t schnitt = karte.find_last_of('/');
                if (schnitt != std::string::npos) { karte = karte.substr(schnitt + 1); }
                const std::size_t punkt = karte.find_last_of('.');
                if (punkt != std::string::npos) { karte = karte.substr(0, punkt); }
                const int extern2 = ladeExterneLightmaps(
                    geo, "maps/" + karte,
                    [](const std::string& n, std::string& out) { return readFromArchives(n, out); });
                if (extern2 > 0) {
                    diag::detail("Externe Lightmaps: " + std::to_string(extern2) + " aus maps/" + karte + "/");
                }
            }
            // Die Lichtstufen 1..3 (schaltbare und flackernde Lichter) in
            // Stufe 0 einrechnen - VOR dem Netz, das die Eckenfarben kopiert
            // (rc592, siehe stileEinrechnen).
            {
                // Schaltbare und flackernde Stile bleiben Schichten und werden
                // je Bild zur Zeit gemischt (rc593, lichtStileZuZeit).
                const std::array<bool, 256> dyn = lichtStilDynamisch(g_app->map.entities);
                const int stilFlaechen = stileEinrechnen(geo, lichtStilGewichte(g_app->map.entities), &dyn);
                (void)stileAnwenden(geo, lichtStilGewichteZuZeit(g_app->map.entities, {}, 0.0));
                if (!geo.stilSchichten.empty()) {
                    diag::detail("Lightmap-Stile: " + std::to_string(geo.stilSchichten.size()) +
                                 " Schichten schaltbarer oder flackernder Lichter");
                }
                if (stilFlaechen > 0) {
                    diag::detail("Lightmap-Stile: " + std::to_string(stilFlaechen) +
                                 " Flaechen mit zusaetzlichen Lichtstufen eingerechnet");
                }
            }
            g_app->geo = std::move(geo);
            // Lightmaps und Texturen der vorigen Karte auf der Grafikkarte
            // vergessen - sie liegen dort nach Nummer.
            gpu::vergissKarte();
            g_app->mapBereit = false;
            // Die Welt ist Untermodell 0 - OHNE die beweglichen Teile, genau
            // wie beim Neubau nach einer Detailaenderung (app_view3d.cpp).
            // Hier stand buildMesh: bis zur ersten Detailaenderung stand jedes
            // Brush-Modell doppelt da, einmal in der Welt und einmal als
            // Mover. Deckend deckungsgleich und unsichtbar, durchscheinend
            // doppelt so dicht - die Hologramme in md_am_sith (alphaGen const
            // 0.4) kamen mit 64 statt 40 Prozent.
            g_app->mesh = g_app->geo.models.empty()
                              ? buildMesh(g_app->geo, g_app->meshDetail, !g_app->showSky)
                              : buildModelMesh(g_app->geo, 0, g_app->meshDetail,
                                               !g_app->showSky);
            g_app->mapBereit = !g_app->mesh.batches.empty();
            bool placed = false;
            for (const MapEntity& e : g_app->map.entities) {
                if (e.classname != "info_player_start" &&
                    e.classname != "info_player_deathmatch") {
                    continue;
                }
                if (e.origin.empty()) { continue; }
                std::istringstream is(e.origin);
                is >> g_app->cam.pos[0] >> g_app->cam.pos[1] >> g_app->cam.pos[2];
                g_app->cam.pos[2] += 64.0F;
                placed = true;
                break;
            }
            if (!placed) {
                for (int k = 0; k < 3; ++k) {
                    g_app->cam.pos[k] = (g_app->geo.mins[k] + g_app->geo.maxs[k]) * 0.5F;
                }
            }
            loadMapTextures();
        } else {
            geoStep.fail(gerr);
            g_app->geo = BspGeometry{};
            g_app->mesh = BspMesh{};
        }
    }
    g_app->mapDirty = true;
    setStatus(tr(Str::MapLoaded), static_cast<int>(g_app->map.entities.size()),
              static_cast<int>(g_app->map.targetNames.size()));
    // rc601 integ: welches Regelwerk die Vorschau rechnet (OpenJK / Movie Duels).
    (void)regelwerkPruefen();
    addStatus(regelwerkStatusZeile());
    diag::info("Karte mit Regelwerk " + std::string(regelwerkMovieDuels() ? "Movie Duels" : "OpenJK") +
               ", g_spskill " + std::to_string(regelwerkSpielstufe()));
}

// Eine Karte ueber ihre KENNUNG aus den Archiven holen.
//
// Gebraucht beim Reiterwechsel: eine Mission kommt aus einem .pk3 und hat
// keinen Pfad auf der Platte, nur ihren Namen darin. Die Entities daneben
// (.ent) laedt der Missionsweg; hier geht es nur um die Geometrie, und
// mehr braucht der Wechsel auch nicht - die Entities haengen am Skript,
// nicht am Reiter.
void merkeEntities() {
    if (!g_app->map.path.empty()) {
        g_app->entitiesJeKarte[g_app->map.path] = g_app->map.entities;
    }
}

void vergissEntities(const std::string& kennung) {
    g_app->entitiesJeKarte.erase(kennung);
}

bool kartenBytes(const std::string& id, std::string& out) {
    if (id.empty()) {
        return false;
    }
    if (readFromArchives(id, out)) {
        return true;
    }
    const auto q = g_app->kartenQuelle.find(id);
    if (q == g_app->kartenQuelle.end() || q->second == id) {
        return false;
    }
    if (q->second.find(':') != std::string::npos || q->second.find('\\') != std::string::npos) {
        out = slurp(q->second);
        return !out.empty();
    }
    return kartenBytes(q->second, out);
}

bool loadMapFromArchive(const std::string& id) {
    std::string data;
    if (id.empty() || !kartenBytes(id, data)) {
        return false;
    }
    loadMapFromBytes(data, id);
    g_app->settings.mapPath.clear();
    return true;
}

void loadMap(const std::string& p) {
    diag::Step step("Karte laden: " + p);
    const std::string bytes = slurp(p);
    if (bytes.empty()) {
        step.fail("nicht lesbar");
        platform::showError(p, tr(Str::AppTitle));
        return;
    }
    loadMapFromBytes(bytes, p);
    g_app->settings.mapPath = p;
}

void doLoadMap() {
    const std::string start = g_app->settings.mapPath.empty()
                                  ? g_app->settings.lastDir
                                  : directoryOf(g_app->settings.mapPath);
    const std::string p = platform::openFileDialog(
        tr(Str::MapLoad), "JKA maps (*.bsp)|*.bsp|All files (*.*)|*.*", start);
    if (!p.empty()) {
        vergissEntities(p);
        loadMap(p);
    }
}

void openEntFile() {
    const std::string p = platform::openFileDialog(
        tr(Str::OpenEnt), "Entity files (*.ent)|*.ent|All files (*.*)|*.*",
        g_app->settings.lastDir);
    if (p.empty()) {
        return;
    }
    diag::Step step("Entities laden: " + p);
    const std::string text = slurp(p);
    if (text.empty()) {
        step.fail("nicht lesbar");
        platform::showError(p, tr(Str::AppTitle));
        return;
    }
    std::vector<MapEntity> ents;
    parseEntities(text, ents);
    if (ents.empty()) {
        step.fail("keine Entities gefunden");
        return;
    }
    g_app->map.entities = std::move(ents);
    merkeEntities();
    // Die Namenslisten neu aufbauen - der Editor bietet sie an.
    g_app->map.targetNames.clear();
    g_app->map.cameraGroups.clear();
    for (const MapEntity& e : g_app->map.entities) {
        for (auto* list : {&g_app->map.targetNames, &g_app->map.cameraGroups}) {
            const std::string& v = (list == &g_app->map.targetNames)
                                       ? e.targetname
                                       : e.cameraGroup;
            if (v.empty()) {
                continue;
            }
            if (std::find(list->begin(), list->end(), v) == list->end()) {
                list->push_back(v);
            }
        }
    }
    std::sort(g_app->map.targetNames.begin(), g_app->map.targetNames.end());
    std::sort(g_app->map.cameraGroups.begin(), g_app->map.cameraGroups.end());
    g_app->settings.lastDir = directoryOf(p);
    g_app->mapDirty = true;
    // Alles, was an den ALTEN Entities hing, gilt nicht mehr: die gewaehlte
    // und die ausgeblendeten Entities stehen als Index da, und die
    // Kamerabahn loest ref_tags ueber die Entities auf - neu aufbauen
    // (Kartentest 27.09.; vorher blieb alles auf dem alten Stand).
    g_app->pickedEntity = -1;
    g_app->hiddenEntities.clear();
    g_app->camTrackValid = false;
    char msg[200];
    std::snprintf(msg, sizeof(msg), tr(Str::EntLoaded),
                  static_cast<int>(g_app->map.entities.size()));
    addStatus(msg);
}

void selectRow(std::size_t rowIndex, bool ctrl, bool shift) {
    g_app->auswahlDurchEinfuegen.clear();   // jetzt waehlt der Benutzer
    // Wer zuletzt gewaehlt hat, bekommt die kraeftige Farbe - die andere
    // Liste behaelt ihre Markierung gedaempft. So macht es Windows mit
    // Listen ohne Fokus.
    g_app->selectionOwner = 2;
    // Eine Zeile, die man anklickt, steht schon da, wo man hinschaut - ein
    // liegengebliebenes "zur Auswahl rollen" darf sie nicht in die Mitte
    // ziehen. Die Tastatur setzt das Flag danach selbst wieder (moveSel).
    g_app->scrollToSelected = false;
    if (rowIndex >= g_app->rows.size()) {
        return;
    }
    const Path& p = g_app->rows[rowIndex].path;

    if (shift && g_app->selected >= 0 &&
        static_cast<std::size_t>(g_app->selected) < g_app->rows.size()) {
        // Von der letzten Auswahl bis hierher, ueber die ANGEZEIGTEN Zeilen -
        // nicht ueber die Wege. Der Nutzer sieht Zeilen, keine Baumpfade,
        // und erwartet genau das, was dazwischen steht.
        const auto a = static_cast<std::size_t>(g_app->selected);
        const std::size_t lo = std::min(a, rowIndex);
        const std::size_t hi = std::max(a, rowIndex);
        if (!ctrl) {
            g_app->selection.clear();
        }
        for (std::size_t i = lo; i <= hi; ++i) {
            const Path& q = g_app->rows[i].path;
            if (std::find(g_app->selection.begin(), g_app->selection.end(), q) ==
                g_app->selection.end()) {
                g_app->selection.push_back(q);
            }
        }
        g_app->selectedPath = p;
        return;
    }

    if (ctrl) {
        const auto it = std::find(g_app->selection.begin(), g_app->selection.end(), p);
        if (it != g_app->selection.end()) {
            g_app->selection.erase(it);
            // Der abgewaehlte darf nicht der Bezugspunkt bleiben.
            if (g_app->selectedPath == p && !g_app->selection.empty()) {
                g_app->selectedPath = g_app->selection.back();
            }
        } else {
            g_app->selection.push_back(p);
            g_app->selectedPath = p;
        }
        g_app->selected = static_cast<int>(rowIndex);
        return;
    }

    g_app->selection.assign(1, p);
    g_app->selectedPath = p;
    g_app->selected = static_cast<int>(rowIndex);
}

bool isSelected(const Path& p) {
    return std::find(g_app->selection.begin(), g_app->selection.end(), p) !=
           g_app->selection.end();
}

std::vector<Path> selectionOrCurrent() {
    if (!g_app->selection.empty()) {
        return g_app->selection;
    }
    if (g_app->selectedPath.empty()) {
        return {};
    }
    return {g_app->selectedPath};
}

// Einstellungen laden und anwenden. Der Ort kommt von der Plattformschicht
// (%APPDATA%\behaved), damit die Oberflaeche nichts ueber Windows weiss.
void loadSettings(App* a) {
    diag::Step step("Einstellungen laden");
    a->settingsFile = platform::settingsDirectory();
    if (a->settingsFile.empty()) {
        return;
    }
    a->settingsFile += "/behaved.cfg";
    if (!loadSettingsFile(a->settingsFile, a->settings)) {
        return;   // erste Benutzung
    }
    // In WELCHER Sprache laeuft die Oberflaeche - und woher kommt sie?
    //
    // Ohne diese Zeile liess es sich nicht nachsehen. Aufgefallen ist es
    // nur, weil in shanks rc539-Protokoll eine Statuszeile englisch war,
    // waehrend der Kopf `Gebiet de-DE` meldete. Die Protokolltexte selbst
    // sind fest deutsch und sagen darueber nichts.
    const char* woher = "aus dem Gebietsschema";
    if (const i18n::LanguageInfo* l = i18n::findLanguage(a->settings.language)) {
        i18n::setLanguage(l->language);
        woher = "aus den Einstellungen";
    }
    {
        std::string code = "?";
        for (const i18n::LanguageInfo& l : i18n::languages()) {
            if (l.language == i18n::currentLanguage()) { code = l.code; }
        }
        diag::detail("Sprache: " + code + " (" + woher + ", Einstellung \"" +
                     a->settings.language + "\")");
    }
    // Die Masse der Anordnung in den Programmzustand holen.
    //
    // Sie liegen in beiden - in den Einstellungen, weil sie ueberdauern
    // sollen, und im Zustand, weil das Zeichnen sie jedes Bild braucht.
    // Hier der eine Abgleich beim Start, unten der andere beim Sichern.
    a->timelineFrac = a->settings.timelineFrac;
    a->timelineFrames = a->settings.timelineFrames;
    a->mapSidebar = a->settings.mapSidebar;
    a->schattenArt = a->settings.schattenArt;
    a->showFog = a->settings.nebel;
    a->showDynLights = a->settings.dynLicht;
    a->splitFracX = a->settings.splitFracX;
    a->splitFracY = a->settings.splitFracY;
    // --- Protokoll: was aus der Datei kam --------------------------------
    //
    // Gemeldet: "beim starten ist immer noch der balken links". Statt zu
    // raten, was davon nicht ankommt, schreibt der Start jetzt auf, WELCHE
    // Werte gelesen wurden. Stehen dort die Vorgaben, wurde nichts
    // gesichert; stehen die richtigen und das Bild sieht anders aus, liegt
    // es an der Anordnung, nicht am Sichern.
    {
        char zeile[300];
        std::snprintf(zeile, sizeof(zeile),
                      "Anordnung geladen: Spalten Ereignisse %.3f, Karte "
                      "%.3f, Status %.3f",
                      static_cast<double>(a->settings.splitEvents),
                      static_cast<double>(a->settings.splitMap),
                      static_cast<double>(a->settings.splitStatus));
        diag::info(zeile);
        std::snprintf(zeile, sizeof(zeile),
                      "   Zeitleiste %.3f, Bilder %s, Seitenleiste %s, "
                      "Raster %.3f/%.3f",
                      static_cast<double>(a->settings.timelineFrac),
                      a->settings.timelineFrames ? "ja" : "nein",
                      a->settings.mapSidebar ? "offen" : "zu",
                      static_cast<double>(a->settings.splitFracX),
                      static_cast<double>(a->settings.splitFracY));
        diag::info(zeile);
    }
    if (const theme::Theme* t = theme::findTheme(a->settings.theme)) {
        a->activeTheme = t;
        a->themePending = true;
    }
    a->uiScale = a->settings.uiScale;
    a->treeOpt.showTypes = a->settings.showTypes;
    a->treeOpt.gFloats = a->settings.gFloats;
    a->treeOpt.foldMacros = a->settings.foldMacros;
}

// Lage und Zustand des Programmfensters. Siehe `Settings::windowPlacement`.
//
// Zwei schmale Funktionen, damit `main_win32.cpp` nicht `app_internal.h`
// einbinden muss - dort ist `App` absichtlich unbekannt.
std::string gemerkteFensterlage() {
    return (g_app != nullptr) ? g_app->settings.windowPlacement : std::string{};
}

void merkeFensterlage(const std::string& wert) {
    if (g_app != nullptr) {
        g_app->settings.windowPlacement = wert;
    }
}

void saveSettings(App* a) {
    if (a == nullptr || a->settingsFile.empty()) {
        return;
    }
    for (const i18n::LanguageInfo& l : i18n::languages()) {
        if (l.language == i18n::currentLanguage()) {
            a->settings.language = l.code;
        }
    }
    if (a->activeTheme != nullptr) {
        a->settings.theme = a->activeTheme->id;
    }
    a->settings.uiScale = a->uiScale;
    a->settings.showTypes = a->treeOpt.showTypes;
    a->settings.gFloats = a->treeOpt.gFloats;
    a->settings.foldMacros = a->treeOpt.foldMacros;
    // Und die Masse der Anordnung zurueck - das Gegenstueck zum Laden.
    a->settings.timelineFrac = a->timelineFrac;
    a->settings.timelineFrames = a->timelineFrames;
    a->settings.mapSidebar = a->mapSidebar;
    a->settings.schattenArt = a->schattenArt;
    a->settings.nebel = a->showFog;
    a->settings.dynLicht = a->showDynLights;
    a->settings.splitFracX = a->splitFracX;
    a->settings.splitFracY = a->splitFracY;
    // Das Gegenstueck zum Ladeprotokoll.
    //
    // Beim Start steht da, WAS gelesen wurde; ohne diese Zeile weiss man
    // nicht, ob es ueberhaupt geschrieben wurde. Genau diese Frage stand
    // schon einmal im Raum ("beim Starten ist immer noch der Balken
    // links") und liess sich nur zur Haelfte beantworten.
    const bool gesichert = saveSettingsFile(a->settingsFile, a->settings);
    {
        char z[300];
        std::snprintf(z, sizeof(z),
                      "Anordnung gesichert (%s): Ereignisse %.3f, Karte %.3f, "
                      "Status %.3f, Zeitleiste %.3f, Raster %.3f/%.3f",
                      gesichert ? "ok" : "FEHLGESCHLAGEN",
                      static_cast<double>(a->settings.splitEvents),
                      static_cast<double>(a->settings.splitMap),
                      static_cast<double>(a->settings.splitStatus),
                      static_cast<double>(a->settings.timelineFrac),
                      static_cast<double>(a->settings.splitFracX),
                      static_cast<double>(a->settings.splitFracY));
        diag::info(z);
    }
}

// Beim Start die zuletzt bearbeitete Datei wieder oeffnen - der Schalter
// "Re-open last file at startup" aus Dialog 131.
void saveSettings(App* a);

void reopenLastIfWanted(App* a, const std::string& kommandozeile) {
    // Absturz erkennen wie das Original (`legal_exit`): beim Start "nicht
    // sauber" SOFORT speichern, destroyApp setzt es beim Beenden zurueck.
    const bool zuletztSauber = a->settings.legalExit;
    a->settings.legalExit = false;
    saveSettings(a);
    if (!a->settings.gamePaths.empty() || !a->settings.gamePathsJk2.empty()) {
        App* keepGp = g_app;
        g_app = a;
        // rc604: ein Ordner von Jedi Outcast in der Liste von Jedi Academy
        // (aus einer Fassung vor rc604 oder von Hand) kommt in die eigene -
        // sonst ueberdeckten sich gleich benannte Dateien beider Spiele.
        for (std::size_t i = a->settings.gamePaths.size(); i > 0; --i) {
            const std::string o = a->settings.gamePaths[i - 1];
            if (jediOutcastOrdner(o)) {
                a->settings.gamePaths.erase(a->settings.gamePaths.begin() + static_cast<std::ptrdiff_t>(i - 1));
                a->settings.gamePathsJk2.insert(a->settings.gamePathsJk2.begin(), o);
                diag::info("Spielordner von Jedi Outcast in die eigene Liste: " + o);
            }
        }
        rescanGamePaths();
        g_app = keepGp;
    }
    // Eine Datei von der Kommandozeile geht vor. Das Original: "Error reading
    // file "%s" specified in command line, ignoring..." - loadPath meldet
    // einen Lesefehler selbst.
    if (!kommandozeile.empty()) {
        diag::info("Kommandozeile: " + kommandozeile);
        App* keepCmd = g_app;
        g_app = a;
        loadPath(kommandozeile);
        g_app = keepCmd;
        return;
    }
    // Die Karte NUR laden, wenn "Letzte Datei beim Start oeffnen" gesetzt
    // ist - genau wie das Skript darunter.
    //
    // Vorher geschah es ungefragt, und das ist aus zwei Gruenden falsch:
    // das Programm startete mit einer Karte, die man vor Tagen zuletzt
    // benutzt hatte, ohne dass man es verlangt haette. Und eine .bsp ist
    // gross - yavin2 hat 13,8 MB - der Start dauerte spuerbar laenger,
    // ohne dass man wusste, wofuer.
    if (!a->settings.reopenLastFile || a->settings.recent.empty()) {
        return;
    }
    // Erst fragen, dann laden - wie das Original: "Auto-load most recent
    // file: "%s"?", nach einem Absturz "It appears you didn't exit BehavEd
    // legally last time (crash?) Should I still try and open your last
    // file "%s"?". Die Frage erscheint im ersten Bild.
    const std::string datei = a->settings.recent.front();
    const std::string karte = a->settings.mapPath;
    char text[900];
    std::snprintf(text, sizeof(text),
                  tr(zuletztSauber ? Str::AskAutoLoad : Str::AskCrashReopen),
                  datei.c_str());
    App* keep = g_app;
    g_app = a;
    frage(text, [datei, karte] {
        if (!karte.empty()) {
            loadMap(karte);
        }
        loadPath(datei);
    });
    g_app = keep;
}

// Wird von IM_ASSERT gerufen (siehe gui/imgui_config.h). Schreibt zuerst ins
// Protokoll, bricht dann ab - so wie die Vorgabe von ImGui es auch tut.
void logImGuiAssert(const char* expression, const char* file, int line) {
    char buf[512];
    std::snprintf(buf, sizeof(buf), "IMGUI-ZUSICHERUNG: %s   (%s:%d)",
                  expression, file, line);
    diag::error(buf);
    const std::string step = diag::currentStep();
    if (!step.empty()) {
        diag::error("  waehrend: " + step);
    }
    std::abort();
}

void resetIconTexture() { resetIcons(); }

App* createApp() {
    App* a = new App();
    a->activeTheme = theme::findTheme("windows");
    // Ein Reiter von Anfang an. Ohne ihn haette das lebende Dokument keinen
    // Platz zum Parken, und der erste Wechsel wuerde es verlieren.
    a->tabs.push_back(App::Parked{});
    a->activeTab = 0;
    return a;
}

void destroyApp(App* a) {
    a->settings.legalExit = true;   // sauber beendet
    if (a == g_app) { karteneditor::beimBeenden(); }   // Karten-Editor: g_strLastMap (main.cpp:476)
    saveSettings(a);
    delete a;
}

void setApp(App* a) {
    g_app = a;
    if (a != nullptr) {
        a->themePending = true;   // nicht hier anwenden, siehe App::themePending
    }
}

const theme::Theme* currentTheme(App* a) {
    return a != nullptr ? a->activeTheme : nullptr;
}

void setThemeById(App* a, const char* id) {
    if (a == nullptr || id == nullptr) {
        return;
    }
    if (const theme::Theme* t = theme::findTheme(id)) {
        a->activeTheme = t;
        a->themePending = true;
    }
}

float uiScale(App* a) { return a != nullptr ? a->uiScale : 1.0F; }
void setUiScale(App* a, float s) {
    if (a != nullptr) {
        a->uiScale = s;
    }
}

// Der Bildschirmmassstab kommt von aussen; die Oberflaeche fragt nicht bei
// Windows nach.
void setDpiScale(App* a, float s) {
    if (a != nullptr && s > 0.0F) {
        a->dpiScale = s;
    }
}

// Befehlsmodell laden. Der Ort wird vom Aufrufer bestimmt, damit die
// Oberflaeche nichts ueber Pfade wissen muss.
bool loadModel(App* a, const std::string& dataDir, std::string* error) {
    diag::Step step("Befehlsmodell laden: " + dataDir);
    a->dataDir = dataDir;
    // Eine in den Einstellungen angegebene .bhc hat Vorrang - Movie Duels
    // bringt moeglicherweise eine eigene mit.
    if (!a->settings.commandFile.empty()) {
        std::vector<LoadDiag> own;
        const std::string dir = a->settings.sourcePath.empty() ? dataDir
                                                               : a->settings.sourcePath;
        if (loadCommandDb(a->settings.commandFile, dir, a->db, own)) {
            (void)loadCommandDb(dataDir + "/supplement.bhc", dir, a->db, own);
            return true;
        }
        diag::warn("eigene .bhc nicht lesbar, nehme die mitgelieferte: " +
                   a->settings.commandFile);
    }
    std::vector<LoadDiag> ld;
    // rc604: der Quelldateipfad gilt auch ohne eigene .bhc - vorher nur
    // zusammen mit ihr (der Knopf "Reload" nahm ihn schon immer).
    std::string kopf = a->settings.sourcePath.empty() ? dataDir : a->settings.sourcePath;
    bool geladen = loadCommandDb(dataDir + "/behaved.bhc", kopf, a->db, ld);
    if (!geladen && kopf != dataDir) {
        diag::warn("Quelldateipfad nicht nutzbar, nehme die mitgelieferten Kopfdateien: " + kopf);
        a->db = CommandDb{};
        ld.clear();
        kopf = dataDir;
        geladen = loadCommandDb(dataDir + "/behaved.bhc", kopf, a->db, ld);
    }
    if (!geladen) {
        if (error != nullptr) {
            // rc601 release: in der eingestellten Sprache und mit dem, was zu
            // tun ist. Vorher englisch und technisch ("behaved.bhc not found
            // in ..."), meist nach dem Auspacken nur der .exe aus dem Zip.
            // Der Ordner bleibt Teil der Meldung - er sagt, WO gesucht wurde.
            char meldung[2048];
            std::snprintf(meldung, sizeof(meldung), tr(Str::DataMissing), dataDir.c_str());
            *error = meldung;
            diag::warn("Befehlsmodell nicht gefunden: " + dataDir + "/behaved.bhc");
        }
        return false;
    }
    (void)loadCommandDb(dataDir + "/supplement.bhc", kopf, a->db, ld);
    char msg[200];
    std::snprintf(msg, sizeof(msg), tr(Str::MsgModelLoaded),
                  static_cast<int>(a->db.commands.size()),
                  static_cast<int>(a->db.typesets.size()),
                  static_cast<int>(a->db.macros.size()));
    a->statusLines.push_back(msg);
    return true;
}

// Ein Bild zeichnen. Der Aufrufer hat ImGui::NewFrame() schon gerufen.
// Darf das Fenster zugehen?
//
// Fuer das KREUZ oben rechts. Der Beenden-Knopf und der Menueeintrag fragen
// selbst, aber das Kreuz geht an der Oberflaeche vorbei - es kommt als
// WM_CLOSE bei der Fensterschicht an, und die weiss nichts von ungesicherten
// Aenderungen.
//
// Gemeldet wurde genau das: "If I go to exit the program or load a new
// script, it should ask me if I want to save before exiting. Currently, if I
// load a script and make some changes and exit the program, it instantly
// closes and my changes are not saved."
//
// Beim LADEN wurde schon gefragt (confirmDiscard in loadPath und im
// pk3-Fenster) - beim Beenden nicht.
// Die Verschiebung und Drehung des Gizmos ins Skript schreiben.
//
// Das ist der Schritt, an dem es darauf ankommt. Drei Dinge sind wichtig:
//
//   1. Ueber replaceAt(), nicht am Skript vorbei. Damit landet die Aenderung
//      im Rueckgaengig-Speicher, das Dokument gilt als ungesichert, und die
//      Auswahl bleibt gueltig - alles, was eine Bearbeitung ausmacht.
//   2. Nur der EINE Wert wird angefasst. Der Knoten wird kopiert, ein
//      Argument ersetzt, der Rest bleibt Zeichen fuer Zeichen stehen. Ein
//      neu gebauter Knoten wuerde Anmerkungen und Schreibweisen verlieren,
//      und dann waere roundtrip nicht mehr bytegleich.
//   3. Geschrieben wird mit "%.3f" - so schreibt BehavEd seine Zahlen, und
//      so stehen sie in Ravens Dateien.
//
// Der Weg fuehrt ueber den Pfad des Schluessels: er zeigt auf genau den
// camera(MOVE)-Befehl, aus dem die Marke entstanden ist.
// Die Drehung ins Skript schreiben.
//
// Sie gehoert NICHT zum MOVE, sondern zum PAN, der darauf folgt - die
// Blickrichtung ist ein eigener Befehl. Gesucht wird deshalb ab dem
// Schluessel VORWAERTS der naechste camera(PAN); steht keiner davor, gibt es
// nichts zu drehen, und das sagen wir auch.
//
// Warum vorwaerts und nicht rueckwaerts: die Engine arbeitet die Befehle der
// Reihe nach ab, ein PAN nach dem MOVE gehoert zu diesem MOVE. Ein PAN
// davor gehoerte zum vorigen.

bool gizmoWriteAngles() {
    // Der Schluessel kommt aus dem SKRIPT, nicht aus App::keyMarks - die
    // Liste des letzten Bildes kannte nur die sichtbaren Schluessel und war
    // beim Abspielen leer (siehe kameraSchluessel).
    const std::vector<KameraSchluessel> ks = kameraSchluessel(g_app->doc.script());
    if (g_app->selectedKey < 0 || static_cast<std::size_t>(g_app->selectedKey) >= ks.size()) {
        return false;
    }
    const KameraSchluessel& schluessel = ks[static_cast<std::size_t>(g_app->selectedKey)];
    const Path& p = schluessel.path;
    if (p.empty()) {
        return false;
    }
    // Geschrieben wird, WAS DIE VORSCHAU ZEIGT - absolut: die Winkel, die
    // am Schluessel angezeigt werden, plus die Drehung. (Vorher "gefundenes
    // PAN + Drehung"; die Kamera sprang nach dem Schreiben um genau einen
    // Schwenk zurueck.)
    float ziel[3];
    for (int k = 0; k < 3; ++k) {
        ziel[k] = (schluessel.hasAng ? schluessel.ang[k] : 0.0F) + g_app->gizmoAngles[k];
    }
    char buf[96];
    std::snprintf(buf, sizeof(buf), "%.3f %.3f %.3f", static_cast<double>(ziel[0]),
                  static_cast<double>(ziel[1]), static_cast<double>(ziel[2]));

    const Node* eltern = nullptr;
    const std::vector<Node>* geschwister = &g_app->doc.script().nodes;
    if (p.size() > 1) {
        Path oben = p;
        oben.pop_back();
        eltern = nodeAt(g_app->doc.script(), oben);
        if (eltern == nullptr) {
            return false;
        }
        geschwister = &eltern->children;
    }
    // Das PAN, das zu diesem Schluessel gehoert: das erste nach dem MOVE,
    // vor dem naechsten MOVE.
    for (std::size_t i = p.back() + 1U; i < geschwister->size(); ++i) {
        const Node& n = (*geschwister)[i];
        if (n.name != "camera" || n.args.size() < 2) {
            continue;
        }
        if (n.args[0].text != "PAN") {
            if (n.args[0].text == "MOVE") {
                break;
            }
            continue;
        }
        if (n.args[1].kind != Arg::Kind::Vector) {
            break;
        }
        Node neu = n;
        neu.args[1].text = buf;
        // Die Drehrichtung (drittes Feld) nur fuer die gedrehten Achsen
        // neu - wie im Original "die kurze Richtung".
        if (neu.args.size() >= 3 && neu.args[2].kind == Arg::Kind::Vector) {
            float dir[3] = {0.0F, 0.0F, 0.0F};
            if (std::sscanf(neu.args[2].text.c_str(), "%f %f %f", &dir[0],
                            &dir[1], &dir[2]) == 3) {
                for (int k = 0; k < 3; ++k) {
                    if (g_app->gizmoAngles[k] != 0.0F) {
                        dir[k] = panDirectionFor(g_app->gizmoAngles[k]);
                    }
                }
                char dbuf[64];
                std::snprintf(dbuf, sizeof(dbuf), "%.0f %.0f %.0f",
                              static_cast<double>(dir[0]),
                              static_cast<double>(dir[1]),
                              static_cast<double>(dir[2]));
                neu.args[2].text = dbuf;
            }
        }
        Path panPfad = p;
        panPfad.back() = i;
        if (!g_app->doc.replaceAt(panPfad, neu)) {
            return false;
        }
        for (int k = 0; k < 3; ++k) {
            g_app->gizmoAngles[k] = 0.0F;
        }
        rebuildTree();
        return true;
    }
    // --- Kein eigenes PAN: eins direkt hinter den MOVE setzen -----------
    //
    // Der Schluessel erbte seinen Blick von einem frueheren PAN. Dann
    // scheiterte "write to script" bisher still. Ein PAN mit Dauer 0 setzt
    // den Blick sofort - genau das, was die Vorschau zeigt.
    const Node* move = nodeAt(g_app->doc.script(), p);
    if (move == nullptr || move->args.empty()) {
        return false;
    }
    Node pan = *move;
    pan.children.clear();
    pan.args.clear();
    Arg art = move->args[0];
    art.text = "PAN";
    pan.args.push_back(art);
    Arg winkel;
    winkel.kind = Arg::Kind::Vector;
    winkel.text = buf;
    pan.args.push_back(winkel);
    Arg richtung;
    richtung.kind = Arg::Kind::Vector;
    char dbuf[64];
    std::snprintf(dbuf, sizeof(dbuf), "%.0f %.0f %.0f",
                  static_cast<double>(panDirectionFor(g_app->gizmoAngles[0])),
                  static_cast<double>(panDirectionFor(g_app->gizmoAngles[1])),
                  static_cast<double>(panDirectionFor(g_app->gizmoAngles[2])));
    richtung.text = dbuf;
    pan.args.push_back(richtung);
    Arg dauer;
    dauer.kind = Arg::Kind::Number;
    dauer.text = "0";
    pan.args.push_back(dauer);
    if (!g_app->doc.insertAfter(p, pan)) {
        return false;
    }
    for (int k = 0; k < 3; ++k) {
        g_app->gizmoAngles[k] = 0.0F;
    }
    rebuildTree();
    return true;
}

bool gizmoWriteBack() {
    // Aus dem Skript, nicht aus keyMarks - siehe gizmoWriteAngles.
    const std::vector<KameraSchluessel> ks = kameraSchluessel(g_app->doc.script());
    if (g_app->selectedKey < 0 || static_cast<std::size_t>(g_app->selectedKey) >= ks.size()) {
        return false;
    }
    const Path p = ks[static_cast<std::size_t>(g_app->selectedKey)].path;
    const Node* alt = nodeAt(g_app->doc.script(), p);
    if (alt == nullptr || alt->args.size() < 2) {
        return false;
    }
    // Argument 0 ist der Unterbefehl ("MOVE"), Argument 1 der Ort.
    if (alt->args[1].kind != Arg::Kind::Vector) {
        return false;
    }
    float xyz[3] = {0.0F, 0.0F, 0.0F};
    if (std::sscanf(alt->args[1].text.c_str(), "%f %f %f", &xyz[0], &xyz[1],
                    &xyz[2]) != 3) {
        return false;
    }
    Node neu = *alt;
    char buf[96];
    std::snprintf(buf, sizeof(buf), "%.3f %.3f %.3f",
                  static_cast<double>(xyz[0] + g_app->gizmoOffset[0]),
                  static_cast<double>(xyz[1] + g_app->gizmoOffset[1]),
                  static_cast<double>(xyz[2] + g_app->gizmoOffset[2]));
    neu.args[1].text = buf;
    if (!g_app->doc.replaceAt(p, neu)) {
        return false;
    }
    for (int k = 0; k < 3; ++k) {
        g_app->gizmoOffset[k] = 0.0F;
    }
    rebuildTree();
    // Dasselbe wie beim Drehen: dorthin springen, wo der neue Ort erreicht
    // ist. Ein camera(MOVE, ...) faehrt ueber eine Dauer; am Anfang steht
    // die Kamera noch am alten Platz.
    return true;
}

bool confirmQuit() {
    // Schon beantwortet? Dann nicht nochmal fragen.
    //
    // Der Beenden-Knopf fragt selbst und laesst danach WM_CLOSE schicken;
    // ohne diese Abfrage kaeme derselbe Dialog ein zweites Mal.
    if (g_app->quitConfirmed) {
        return true;
    }
    // Ist gerade eine Frage offen, nicht noch eine anstossen.
    if (g_app->askSaveOpen) {
        return false;
    }
    // Entities einer Mission, die noch nicht gespeichert sind - sie stehen in
    // keinem Reiter, die Skriptfragen unten saehen sie nicht.
    if (!missionBeendenFrage()) {
        return false;
    }
    // ALLE Reiter pruefen, nicht nur den aktiven.
    //
    // Mit mehreren offenen Skripten reicht der aktive nicht: die anderen
    // liegen geparkt daneben und haben ihren eigenen
    // Rueckgaengig-Speicher. Wer sie beim Beenden uebergeht, verliert genau
    // die Arbeit, um die es hier geht.
    //
    // Vorgegangen wird von vorn nach hinten: jeder ungesicherte Reiter wird
    // zuerst SICHTBAR gemacht und dann gefragt. Man soll sehen, worueber
    // man entscheidet.
    // Den ersten ungesicherten suchen.
    //
    // Keine Schleife mit Frage darin: das Fenster spannt sich ueber mehrere
    // Bilder, eine Schleife koennte gar nicht warten. Stattdessen wird
    // EINER gefragt, und was danach geschieht, ist "nochmal von vorn
    // suchen". So arbeitet sich die Kette ab, ein Reiter je Antwort.
    for (std::size_t i = g_app->quitAskFrom; i < g_app->tabs.size(); ++i) {
        const bool aktiv = (static_cast<int>(i) == g_app->activeTab);
        const bool schmutzig =
            aktiv ? g_app->doc.dirty() : g_app->tabs[i].doc.dirty();
        if (!schmutzig) {
            continue;
        }
        if (g_app->quitAskFrom == 0) {
            // Der Reiter, in dem man stand - dorthin nach "Abbrechen".
            g_app->quitZurueck = g_app->activeTab;
        }
        if (!aktiv) {
            switchTab(static_cast<int>(i));
            g_app->homeTab = static_cast<int>(i);   // das Band zeigt, wonach gefragt wird
        }
        // Diesen Reiter als beantwortet vormerken, BEVOR gefragt wird.
        //
        // "Nein" heisst wegwerfen - es macht das Dokument aber nicht
        // sauber. Ohne diese Marke faende die naechste Runde denselben
        // Reiter wieder und fragte erneut, endlos. Genau das war der
        // Fehler: "Clicking No when asked to save before closing never
        // closes the program."
        g_app->quitAskFrom = i + 1U;
        withUnsaved([] {
            // Beantwortet - jetzt der naechste. Ist keiner mehr da, ist
            // confirmQuit() beim naechsten Mal sofort fertig.
            if (confirmQuit()) {
                g_app->quitConfirmed = true;
                g_app->wantQuit = true;
            }
        },
        [] {
            // Abgebrochen: die Kette faengt beim naechsten Versuch von
            // vorn an. Sonst uebersaehe sie die schon gefragten Reiter.
            g_app->quitAskFrom = 0;
            updater::neustartVergessen();   // "Neu starten" ebenso abgebrochen
            // Und zurueck in den Reiter, in dem man gearbeitet hat - sonst
            // stand man im gefragten, waehrend das Band den alten zeigte.
            const int z = g_app->quitZurueck;
            g_app->quitZurueck = -1;
            if (z >= 0 && z < static_cast<int>(g_app->tabs.size())) {
                switchTab(z);
                g_app->homeTab = z;
            }
        });
        return false;
    }
    // "Exit?" - nur mit "Yes/No query on Open/New/Exit" (Original), und nur,
    // wenn eben keine Speichernfrage kam: wer die beantwortet hat, hat sich
    // schon entschieden.
    if (g_app->settings.queryOnDiscard && g_app->quitAskFrom == 0) {
        if (!g_app->frageOffen) {
            frage(tr(Str::AskExit), [] {
                g_app->quitConfirmed = true;
                g_app->wantQuit = true;
            }, [] { updater::neustartVergessen(); });
        }
        return false;
    }
    return true;
}

// Was soll oben im Fenster stehen?
//
// Bisher stand dort immer "BehavEd". Wer zwei Fenster offen hat, sieht dann
// nicht, welches welches ist - genau so gemeldet: "Sometimes I have multiple
// scripts open so I need to see which script I am working on."
//
// Aufbau wie in jedem Editor: Dateiname, ein Stern bei ungesicherten
// Aenderungen, dann der Programmname. Der Dateiname zuerst, weil die
// Taskleiste von links abschneidet.
std::string windowTitle() {
    // Stufe 7 Punkt 6f: im Map-Editor der Titel von NRC (Kartenname, " *"),
    // nicht der des Skripts; in den Skript-Modi wie bisher.
    if (karteneditor::aktiv()) { return karteneditor::fensterTitel(); }
    std::string t;
    if (!g_app->path.empty()) {
        t = fileName(g_app->path);
    } else if (!g_app->shownName.empty()) {
        t = g_app->shownName;
    } else {
        t = "unnamed.txt";
    }
    if (g_app->doc.dirty()) {
        t += " *";
    }
    t += std::string(" - ") + tr(Str::AppTitle);
    return t;
}

// Darf die Hauptschleife ruhen (rc596, siehe main_win32.cpp)? Nur, wenn
// kein Bild aus sich heraus anders wird: nichts spielt ab, die Karte muss
// nicht neu gezeichnet werden, kein Ton laeuft (der Mischer muss
// nachgefuellt werden), kein Textfeld blinkt, keine Maustaste ist gedrueckt
// - und nie im Selbsttest, der Bild fuer Bild vorgeht.
bool darfRuhen() {
    if (g_app == nullptr) {
        return false;
    }
    static const bool imTest = std::getenv("BHED_EDITORTEST") != nullptr;
    if ((imTest && !g_app->ruheImTest) || g_app->playing || g_app->mapDirty || g_app->tonVorab.valid()) {
        return false;
    }
    if (g_app->audio.activeBuffers() > 0 || g_app->musicAudio.activeBuffers() > 0) {
        return false;
    }
    // rc601 tour: Puls, "Zeig es mir" und das Weiterschalten nach einem
    // erledigten Schritt brauchen Bilder.
    if (tourAnimiert()) {
        return false;
    }
    // Karten-Editor: Kamerafahrt (IdleDraw2-Schleife), Freelook, Texturen laden.
    if (karteneditor::aktiv() && karteneditor::kameraBrauchtBilder()) {
        return false;
    }
    // Stufe 3: waehrend q3map2 baut, Bild fuer Bild seine Ausgabe lesen
    if (karteneditor::bauLaeuft()) {
        return false;
    }
    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput) {
        return false;
    }
    for (const bool unten : io.MouseDown) {
        if (unten) {
            return false;
        }
    }
    return true;
}

// Nach einem neu aufgebauten Grafikgeraet (rc596): was an Texturen des alten
// hing, ist weg - die Gizmo-Ueberlagerung neu anlegen, die Karte neu
// zeichnen.
void grafikNeuAufgebaut() {
    if (g_app == nullptr) {
        return;
    }
    g_app->gizmoTexture = nullptr;
    g_app->gizmoTexW = 0;
    g_app->gizmoTexH = 0;
    g_app->gizmoBild.rgba.clear();
    g_app->gizmoZeigen = false;
    ++g_app->grafikNeuAufbauten;
    g_app->mapDirty = true;
}

bool wantsQuit() {
    // Einmal abfragen und zuruecksetzen: sonst schliesst das Fenster in
    // jedem folgenden Bild erneut, und ein abgebrochener Speichern-Dialog
    // wuerde nichts nuetzen.
    const bool q = g_app->wantQuit;
    g_app->wantQuit = false;
    return q;
}

