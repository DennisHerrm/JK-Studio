// app_dockspace.inc - dock2 (in Arbeit): ein durchgehender DockSpace je Bereich
//
// shank: "wie bei Blender": jedes Panel ueberall hin - auch an jede Stelle im
// Hauptfenster (neben, unter, ueber, als Reiter zu jedem anderen Panel).
//
// Stand: Events, Mission und Model. Schalter ab Werk AUS (dockraum::an();
// BHED_DOCKSPACE=1 schaltet ihn beim Start an, der Selbsttest dock2mass schaltet
// ihn zum Vergleichen um). Map Editor und Menu Editor bleiben wie in dock1.
//
// Die Standardanordnung entsteht nicht aus eigenen Rechnungen: dieselben
// ImGui-Tabellen wie vorher richten sich ein (nur gemessen, ohne Inhalt), und
// die Dock-Knoten bekommen genau die Spalten der Tabellen - je Panel ein Knoten,
// Abstand 1 Punkt (der Tabellenrand), Polster wie die Zellen. Damit stehen alle
// Inhalte da, wo sie vorher standen. Die Griffe zwischen den Spalten bleiben die
// alten (Grenzen, Anteile); ImGuis eigene Teiler gelten erst, wenn die
// Anordnung nicht mehr die Standardanordnung ist (Panel woanders angedockt).
// Eingebunden von gui/app.cpp nach app_dock.inc.

namespace dockraum {

bool& an() {
    static bool a = [] {
        const char* e = std::getenv("BHED_DOCKSPACE");
        return e != nullptr && e[0] == '1';
    }();
    return a;
}

// Gemessene Spalten dieses Bildes (Bildschirmpunkte): die aeussere Tabelle
// (Links, Script Flow, Actions) und die innere der linken Spalte (Modusleiste,
// Ebenen/Animationen, Inhalt).
// Abstand der Knoten: der Tabellenrand (TABLE_BORDER_SIZE).
constexpr float kAbstand = 1.0F;

struct Messung {
    float aussen[3][2] = {};
    int aussenN = 0;
    float innen[3][2] = {};
    int innenN = 0;
    float oben = 0.0F;
    float bandLinks = 0.0F;    // das Band ueber die volle Breite (Inhalt des Hauptfensters +- Zellpolster)
    float bandRechts = 0.0F;
    float unten = 0.0F;   // Zeilenende der Tabelle (Inhalt der Zellen + Zellpolster)
};
Messung g_mess;

void zelleMessen(int k) {
    const ImGuiTable* t = ImGui::GetCurrentTable();
    if (t == nullptr || k < 0 || k >= 3) { return; }
    g_mess.aussen[k][0] = t->Columns[k].MinX;
    g_mess.aussen[k][1] = t->Columns[k].MaxX;
    g_mess.aussenN = std::max(g_mess.aussenN, k + 1);
}

// Die innere Tabelle der linken Spalte wie drawEventsList, ohne Inhalt.
void innenMessen() {
    const bool mitEbenen = g_app->leftMode == 1 || g_app->leftMode == 2;
    const char* const innenId = (g_app->leftMode == 1)   ? "##linksMap"
                                : (g_app->leftMode == 2) ? "##linksModel"
                                                         : "##linksEvents";
    if (!ImGui::BeginTable(innenId, mitEbenen ? 3 : 2,
                           ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoPadOuterX,
                           ImVec2{0.0F, 0.0F})) {
        return;
    }
    ImGui::TableSetupColumn("mode", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize,
                            std::max(g_app->modeBarW, ImGui::GetFontSize() * 4.0F));
    if (mitEbenen) {
        ImGui::TableSetupColumn("ebenen", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize,
                                ImGui::GetFontSize() * kEntitySpalte);
    }
    ImGui::TableSetupColumn("inhalt", ImGuiTableColumnFlags_WidthStretch, 1.0F);
    ImGui::TableNextRow();
    const ImGuiTable* t = ImGui::GetCurrentTable();
    g_mess.innenN = mitEbenen ? 3 : 2;
    for (int k = 0; k < g_mess.innenN; ++k) {
        g_mess.innen[k][0] = t->Columns[k].MinX;
        g_mess.innen[k][1] = t->Columns[k].MaxX;
    }
    ImGui::EndTable();
}

// --- Die Panels eines Bereichs -------------------------------------------------
struct Blatt {
    const char* id;    // Kennung wie in fenster_dock (Modusleiste: "<praefix>modus")
    Str titel;
};

std::vector<Blatt> blaetter(int b) {
    if (b == 1) {
        return {{"mission.modus", Str::DockPanels}, {"mission.ebenen", Str::EntityLayers}, {"mission.ansicht", Str::DockPanelMapView},
                {"mission.fluss", Str::GroupScriptFlow}, {"mission.zeitleiste", Str::DockPanelTimeline}};
    }
    if (b == 2) {
        return {{"modell.modus", Str::DockPanels},        {"modell.animationen", Str::ModelAnimations},
                {"modell.ansicht", Str::DockPanelModelView}, {"modell.fluss", Str::GroupScriptFlow},
                {"modell.aktionen", Str::GroupActions},   {"modell.leiste", Str::DockPanelModelBar}};
    }
    return {{"ereignisse.modus", Str::DockPanels},
            {"ereignisse.liste", Str::GroupEvents},
            {"ereignisse.fluss", Str::GroupScriptFlow},
            {"ereignisse.aktionen", Str::GroupActions}};
}

bool mitBand(int b) { return b == 1 || b == 2; }

std::string fensterName(const Blatt& p) { return std::string(tr(p.titel)) + "###dock." + p.id; }   // wie fenster_dock

struct Raum {
    ImGuiID wurzel = 0;
    std::vector<ImGuiID> heim;   // Blattknoten der Standardanordnung, wie blaetter()
    int knoten = 0;              // Knoten der Standardanordnung insgesamt
    // Gerade (neu) aufgebaut: die Panels im naechsten Begin in ihre Knoten
    // (SetNextWindowDockID - die Panelfenster tragen NoSavedSettings, ImGui liest
    // fuer sie keine Fenstereinstellungen, DockBuilderDockWindow allein reicht nicht).
    bool frisch = false;
    bool neuBauen = false;   // Anordnung zuruecksetzen: im naechsten Bild neu aufbauen
};
std::array<Raum, 3> g_raum;

int knotenZaehlen(const ImGuiDockNode* n) {
    if (n == nullptr) { return 0; }
    return 1 + knotenZaehlen(n->ChildNodes[0]) + knotenZaehlen(n->ChildNodes[1]);
}

// Die Rechtecke der Panels in der Standardanordnung, aus der Messung.
std::vector<ImRect> heimRechtecke(int b, float bandOben, float bandUnten) {
    const Messung& m = g_mess;
    std::vector<ImRect> r;
    // ImGui-Spalten liegen lueckenlos [MinX, MaxX]; ihr erster Punkt ist der Rand
    // (CellSpacingX1), der Inhalt beginnt bei MinX + 1 + CellPadding.x.
    const auto spalte = [&](float a, float e) { r.emplace_back(ImVec2{a + kAbstand, m.oben}, ImVec2{e, m.unten}); };
    for (int k = 0; k < m.innenN; ++k) { spalte(m.innen[k][0], m.innen[k][1]); }
    for (int k = 1; k < m.aussenN; ++k) { spalte(m.aussen[k][0], m.aussen[k][1]); }
    if (mitBand(b)) { r.emplace_back(ImVec2{m.bandLinks, bandOben}, ImVec2{m.bandRechts, bandUnten}); }
    return r;
}

ImGuiWindowClass klasse() {
    ImGuiWindowClass k;
    k.ClassId = ImHashStr("bhed.dock.panel");
    k.DockingAllowUnclassed = false;
    return k;
}

void bauen(int b, const std::vector<ImRect>& r, const ImVec2& groesse) {
    Raum& raum = g_raum[static_cast<std::size_t>(b)];
    const std::vector<Blatt> p = blaetter(b);
    raum.wurzel = ImHashStr((std::string("bhed.dockraum.") + dock::praefix(b)).c_str());
    ImGui::DockBuilderRemoveNode(raum.wurzel);
    ImGui::DockBuilderAddNode(raum.wurzel, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(raum.wurzel, groesse);
    raum.heim.assign(p.size(), 0);
    ImGuiID kette = raum.wurzel;
    const std::size_t spalten = mitBand(b) ? p.size() - 1 : p.size();
    if (mitBand(b)) {
        const float anteil = std::clamp(r.back().GetHeight() / std::max(1.0F, groesse.y), 0.05F, 0.95F);
        ImGui::DockBuilderSplitNode(raum.wurzel, ImGuiDir_Down, anteil, &raum.heim.back(), &kette);
    }
    float rest = groesse.x;
    for (std::size_t i = 0; i + 1 < spalten; ++i) {
        // Die letzte Spalte bleibt der Mittelknoten: sie nimmt den Rest (beim
        // freien Anordnen, und in Mission/Model die Breite, die das Band ueber
        // die Spalten hinaus hat - siehe zeichnen).
        ImGuiID weiter = 0;
        ImGui::DockBuilderSplitNode(kette, ImGuiDir_Left, std::clamp(r[i].GetWidth() / std::max(1.0F, rest), 0.05F, 0.95F),
                                    &raum.heim[i], &weiter);
        kette = weiter;
        if (i + 2 == spalten) { raum.heim[i + 1] = weiter; }
        rest -= r[i].GetWidth();
    }
    for (std::size_t i = 0; i < p.size(); ++i) {
        if (ImGuiDockNode* n = ImGui::DockBuilderGetNode(raum.heim[i]); n != nullptr) {
            ImGuiDockNodeFlags f = n->LocalFlags | ImGuiDockNodeFlags_HiddenTabBar;
            if (i == 0) {   // die Modusleiste bleibt, wo sie ist
                f |= static_cast<int>(ImGuiDockNodeFlags_NoTabBar) | static_cast<int>(ImGuiDockNodeFlags_NoUndocking) |
                     static_cast<int>(ImGuiDockNodeFlags_NoDockingOverMe);
            }
            n->SetLocalFlags(f);
        }
    }
    ImGui::DockBuilderFinish(raum.wurzel);
    raum.knoten = knotenZaehlen(ImGui::DockBuilderGetNode(raum.wurzel));
    raum.frisch = true;
    diag::detail("Fenster: DockSpace von Bereich " + std::to_string(b) + " aufgebaut (" + std::to_string(raum.knoten) + " Knoten)");
}

// Steht jedes Panel in seinem Knoten der Standardanordnung, allein, und hat der
// Baum keine weiteren Knoten?
bool standard(int b) {
    const Raum& raum = g_raum[static_cast<std::size_t>(b)];
    if (raum.wurzel == 0 || raum.knoten != knotenZaehlen(ImGui::DockBuilderGetNode(raum.wurzel))) { return false; }
    const std::vector<Blatt> p = blaetter(b);
    for (std::size_t i = 0; i < p.size(); ++i) {
        const ImGuiWindow* w = ImGui::FindWindowByName(fensterName(p[i]).c_str());
        if (w == nullptr || w->DockNode == nullptr || w->DockNode->ID != raum.heim[i] || w->DockNode->Windows.Size != 1) { return false; }
    }
    return true;
}

// Die Groessen der Standardanordnung in die Knoten, vor DockSpace(): jede
// Teilung bekommt fuer ihr erstes Kind genau die gemessene Breite (Hoehe).
ImRect spanne(const ImGuiDockNode* n, const std::vector<ImGuiID>& heim, const std::vector<ImRect>& r) {
    if (n->IsLeafNode()) {
        for (std::size_t i = 0; i < heim.size(); ++i) {
            if (heim[i] == n->ID) { return r[i]; }
        }
        return ImRect{};
    }
    ImRect a = spanne(n->ChildNodes[0], heim, r);
    a.Add(spanne(n->ChildNodes[1], heim, r));
    return a;
}

void erzwingen(ImGuiDockNode* n, const std::vector<ImGuiID>& heim, const std::vector<ImRect>& r) {
    if (n == nullptr || n->IsLeafNode()) { return; }
    const int achse = n->SplitAxis == ImGuiAxis_Y ? 1 : 0;
    const ImRect ganz = spanne(n, heim, r);
    const ImRect erstes = spanne(n->ChildNodes[0], heim, r);
    const float gesamt = achse == 0 ? ganz.GetWidth() : ganz.GetHeight();
    const float vorn = achse == 0 ? erstes.GetWidth() : erstes.GetHeight();
    const float abstand = kAbstand;
    n->ChildNodes[0]->SizeRef[achse] = vorn;
    n->ChildNodes[1]->SizeRef[achse] = std::max(1.0F, gesamt - abstand - vorn);
    n->ChildNodes[0]->SizeRef[achse ^ 1] = n->ChildNodes[1]->SizeRef[achse ^ 1] = achse == 0 ? ganz.GetHeight() : ganz.GetWidth();
    erzwingen(n->ChildNodes[0], heim, r);
    erzwingen(n->ChildNodes[1], heim, r);
}

void panelInhalt(const std::string& id, const Layout& l) {
    Layout f = l;
    f.frameH = -FLT_MIN;   // bis an den Inhaltsrand des Knotens = das alte Ende der Spalten
    const float breite = std::max(ImGui::GetContentRegionAvail().x, 80.0F);
    if (id.find(".modus") != std::string::npos) {
        g_app->modeBarW = drawModeBar(f.frameH, f.headerH);
    } else if (id == "ereignisse.liste") {
        drawEreignisListe(f, breite);
    } else if (id.find(".fluss") != std::string::npos) {
        drawFlowArea(f);
    } else if (id.find(".aktionen") != std::string::npos) {
        drawButtonColumns(f);
    } else if (id.find(".ansicht") != std::string::npos) {
        g_app->dbgRestW = breite;
        g_app->dbgZelleW = ImGui::GetContentRegionAvail().x;
        ImGui::BeginGroup();
        drawAnsichtInhalt(f, breite);
        ImGui::EndGroup();
    } else if (id == "mission.ebenen" || id == "modell.animationen") {
        drawEbenenInhalt(f);
    } else if (id == "mission.zeitleiste" || id == "modell.leiste") {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0, 0, 0, 0});
        if (id == "mission.zeitleiste") { drawTlKante(); }
        ImGui::BeginChild("toolband", ImVec2{-FLT_MIN, l.toolH}, ImGuiChildFlags_None,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        if (id == "modell.leiste") {
            drawModelPanel();
        } else {
            drawTimeline();
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
}

// Nach der Messung (Tabelle ohne Inhalt): der DockSpace an der Stelle der
// Tabelle und des Bandes, dann die Panels in ihren Knoten.
void zeichnen(const Layout& l) {
    const int b = g_app->leftMode;
    if (b < 0 || b > 2 || g_mess.aussenN < 2 || g_mess.innenN < 2) { return; }
    const ImGuiStyle& st = ImGui::GetStyle();
    const float kanteH = (b == 1) ? std::max(4.0F, st.ItemSpacing.y) + st.ItemSpacing.y : 0.0F;
    // g_mess.unten ist das Zeilenende der alten Tabelle; dort beginnt (mit dem
    // Knotenabstand) das Band, wie vorher nach EndTable + ItemSpacing.y.
    const float bandOben = g_mess.unten + kAbstand;
    const float bandUnten = g_mess.unten + st.ItemSpacing.y + kanteH + l.toolH;
    const std::vector<ImRect> r = heimRechtecke(b, bandOben, mitBand(b) ? bandUnten : g_mess.unten);
    ImRect alles = r.front();
    for (const ImRect& x : r) { alles.Add(x); }
    const ImVec2 groesse{alles.GetWidth(), alles.GetHeight()};
    Raum& raum = g_raum[static_cast<std::size_t>(b)];
    if (raum.wurzel == 0 || raum.neuBauen || ImGui::DockBuilderGetNode(raum.wurzel) == nullptr) {
        raum.neuBauen = false;
        bauen(b, r, groesse);
    }
    const bool std0 = standard(b);
    if (std0) { erzwingen(ImGui::DockBuilderGetNode(raum.wurzel), raum.heim, r); }
    ImGui::SetCursorScreenPos(alles.Min);
    const ImGuiWindowClass k = klasse();
    ImGui::PushStyleColor(ImGuiCol_Separator, ImGui::GetStyleColorVec4(ImGuiCol_TableBorderLight));
    ImGui::PushStyleVar(ImGuiStyleVar_DockingSeparatorSize, kAbstand);
    ImGui::DockSpace(raum.wurzel, groesse, std0 ? ImGuiDockNodeFlags_NoResize : ImGuiDockNodeFlags_None, &k);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    const std::vector<Blatt> p = blaetter(b);
    const std::size_t letzteSpalte = mitBand(b) ? p.size() - 2 : p.size() - 1;
    for (std::size_t i = 0; i < p.size(); ++i) {
        const bool band = mitBand(b) && i + 1 == p.size();
        // Zellen der inneren Tabelle (Modusleiste, Ebenen/Animationen, Ansicht bzw.
        // Ereignisliste) beginnen ein Zellpolster tiefer als die der aeusseren
        // (Script Flow, Actions); alle enden ein Zellpolster ueber dem
        // Zeilenende der aeusseren Zeile (je Zeile oben und unten CellPadding.y).
        const bool innen = i < static_cast<std::size_t>(g_mess.innenN);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                            ImVec2{st.CellPadding.x, band ? st.ItemSpacing.y - 1.0F : st.CellPadding.y * 2.0F});
        ImGui::SetNextWindowClass(&k);
        if (raum.frisch) { ImGui::SetNextWindowDockID(raum.heim[i], ImGuiCond_Always); }
        const bool sichtbar = ImGui::Begin(fensterName(p[i]).c_str(), nullptr,
                                           ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                               ImGuiWindowFlags_NoSavedSettings);
        ImGui::PopStyleVar();
        if (sichtbar && !band && !innen) { ImGui::SetCursorPosY(ImGui::GetCursorPosY() - st.CellPadding.y); }
        if (sichtbar && std0 && i == letzteSpalte && i < r.size()) {
            // Der Knoten ist so breit wie das Band; der Inhalt bleibt so breit wie die Spalte.
            ImGuiWindow* w = ImGui::GetCurrentWindow();
            const float zuviel = w->Size.x - r[i].GetWidth();
            if (zuviel > 0.0F) {
                w->WorkRect.Max.x -= zuviel;
                w->ContentRegionRect.Max.x -= zuviel;
            }
        }
        if (sichtbar) { panelInhalt(p[i].id, l); }
        ImGui::End();
    }
    raum.frisch = false;
}

ImGuiID heimKnoten(const std::string& panel) {
    for (int b = 0; b < 3; ++b) {
        const std::vector<Blatt> p = blaetter(b);
        const Raum& raum = g_raum[static_cast<std::size_t>(b)];
        for (std::size_t i = 0; i < p.size() && i < raum.heim.size(); ++i) {
            if (panel == p[i].id) { return raum.heim[i]; }
        }
    }
    return 0;
}

}  // namespace dockraum
