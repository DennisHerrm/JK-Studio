// app_dock.inc - dock1: die Panels von Events, Mission, Model und Menu Editor
// in eigenen Fenstern, und die Spalten des Hauptfensters ohne sie.
//
// Eingebunden von gui/app.cpp direkt nach app_anordnung.inc (im selben
// namenlosen Namensraum), weil die Zeichenfunktionen der Panels dort stehen.
// Was ein Panel zeigt, bleibt die Sache seiner Zeichenfunktion; hier steht
// nur, in welches Fenster sie zeichnet (fenster_dock.h).

// Die Panels der Spalten des aktuellen Bereichs.
const char* flussPanel() {
    return (g_app->leftMode == 1) ? "mission.fluss" : (g_app->leftMode == 2) ? "modell.fluss" : "ereignisse.fluss";
}
const char* aktionenPanel() { return (g_app->leftMode == 2) ? "modell.aktionen" : "ereignisse.aktionen"; }

// Steht ein Panel der Spalten nicht an seinem Platz, gilt die feste
// Aufteilung (Anteile, eigene Griffe, app.cpp) nicht mehr - dann teilt eine
// einfache Tabelle den Platz unter den uebrigen auf: die linke Spalte so
// breit wie Modusleiste und Ebenenliste, wenn ihr Inhalt fehlt, sonst
// gedehnt; der Script Flow gedehnt; Actions in ihrer eingestellten Breite.
bool spaltenFrei() { return !dock::zuhause(flussPanel()) || !dock::zuhause(linksInhaltPanel()); }

void drawSpaltenFrei(const Layout& l) {
    const bool fluss = dock::zuhause(flussPanel());
    const bool knoepfe = g_app->leftMode != 1 && dock::zuhause(aktionenPanel());
    const bool linksVoll = dock::zuhause(linksInhaltPanel());
    const bool linksDehnen = linksVoll || !fluss;
    const bool ebenen = (g_app->leftMode == 1 || g_app->leftMode == 2) &&
                        dock::zuhause(g_app->leftMode == 1 ? "mission.ebenen" : "modell.animationen");
    const float fs = ImGui::GetFontSize();
    const float zelle = ImGui::GetStyle().CellPadding.x * 2.0F;
    const float linksB = std::ceil(std::max(g_app->modeBarW, fs * 4.0F) + (ebenen ? fs * kEntitySpalte + zelle : 0.0F) + zelle);
    char id[64];
    std::snprintf(id, sizeof(id), "##spaltenFrei%d.%d%d%d", g_app->leftMode, linksVoll ? 1 : 0, fluss ? 1 : 0, knoepfe ? 1 : 0);
    if (!ImGui::BeginTable(id, 1 + (fluss ? 1 : 0) + (knoepfe ? 1 : 0),
                           ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoPadOuterX |
                               ImGuiTableFlags_NoSavedSettings,
                           ImVec2{0.0F, 0.0F})) {
        return;
    }
    ImGui::TableSetupColumn("links", linksDehnen ? ImGuiTableColumnFlags_WidthStretch
                                                 : (ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize),
                            linksDehnen ? 1.0F : linksB);
    if (fluss) { ImGui::TableSetupColumn("Flow", ImGuiTableColumnFlags_WidthStretch, 1.0F); }
    if (knoepfe) { ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, l.buttonsW); }
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    drawEventsList(l);
    if (fluss) {
        ImGui::TableNextColumn();
        drawFlowArea(l);
        dock::heimPlatz(flussPanel(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    }
    if (knoepfe) {
        ImGui::TableNextColumn();
        drawButtonColumns(l);
        dock::heimPlatz(aktionenPanel(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    }
    ImGui::EndTable();
}

// Die herausgenommenen Panels des aktuellen Bereichs, jedes in seinem Fenster.
// Der Map Editor zeichnet seine Ansichten selbst (app_karte.cpp, dockFelder).
void drawDockFenster(const Layout& l) {
    const int bereich = g_app->leftMode;
    if (bereich == 3) { return; }
    if (bereich == 4) {
        menueeditor::dockFenster();
        return;
    }
    for (const dock::Panel& p : dock::panels()) {
        // rc609 W2b: der Entity-Text zeichnet sein Fenster selbst (drawEntTextFenster, auch im DockSpace)
        if (p.bereich != bereich || !dock::draussen(p.id) || dock::zusatzOffen(p.id) != nullptr) { continue; }
        if (dock::fensterBeginnen(p.id)) {
            // Bis an den Fensterrand: negativ heisst "der Rest minus diesem
            // Betrag" - wie im Hauptfenster, dort ohne Statuszeile und Band.
            Layout f = l;
            f.frameH = -1.0F;
            const std::string id = p.id;
            const float breite = std::max(ImGui::GetContentRegionAvail().x, 80.0F);
            if (id == "ereignisse.liste") {
                drawEreignisListe(f, breite);
            } else if (id == "ereignisse.fluss" || id == "mission.fluss" || id == "modell.fluss") {
                drawFlowArea(f);
            } else if (id == "ereignisse.aktionen" || id == "modell.aktionen") {
                drawButtonColumns(f);
            } else if (id == "mission.ansicht" || id == "modell.ansicht") {
                ImGui::BeginGroup();
                drawAnsichtInhalt(f, breite);
                ImGui::EndGroup();
            } else if (id == "mission.ebenen" || id == "modell.animationen") {
                drawEbenenInhalt(f);
            } else if (id == "mission.zeitleiste") {
                drawTimeline();
            } else if (id == "modell.leiste") {
                drawModelPanel();
            }
        }
        dock::fensterEnde();
    }
}
