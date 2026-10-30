// app_dockspace.inc - dock2: ein durchgehender DockSpace je Bereich
//
// shank: "wie bei Blender": jedes Panel ueberall hin - auch an jede Stelle im
// Hauptfenster (neben, unter, ueber, als Reiter zu jedem anderen Panel).
//
// Events, Mission und Model; Map Editor und Menu Editor haben ihre eigene
// Vorgabe (dockvorgabe.h). Schalter ab Werk AN (dock4; dockraum::an()):
// BHED_DOCKSPACE=0 schaltet ihn beim Start aus - der Rueckweg zu dock1 (Panels
// an festen Plaetzen, nur in eigene Fenster); der Selbsttest dock2mass schaltet
// ihn zum Vergleichen um. Der Baum steht mit den Panellagen in den
// Einstellungen (Neustart, benannte Anordnungen); Anordnungen aus dock1 werden
// beim ersten Aufbau uebernommen.
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
        return e == nullptr || e[0] != '0';
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

// dock4: die Hover-Linie eines alten Griffs (app.cpp, ##spalteLinks/##spalteRechts).
// Im Hauptfenster gezeichnet laege sie unter den Panels (zu sehen war nur der
// 1-Punkt-Spalt) - sie kommt in die Zeichenlisten der Panels, an deren Rand sie liegt.
struct Linie {
    bool an = false;
    float x = 0.0F;
    float oben = 0.0F;
    float unten = 0.0F;
    ImU32 farbe = 0;
};
Linie g_linie;

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
    // Beim Neuaufbau fuer ein Andocken: Panels in eigenen Fenstern bleiben draussen.
    std::vector<std::string> draussenLassen;
};
std::array<Raum, 3> g_raum;

int knotenZaehlen(const ImGuiDockNode* n) {
    if (n == nullptr) { return 0; }
    return 1 + knotenZaehlen(n->ChildNodes[0]) + knotenZaehlen(n->ChildNodes[1]);
}

bool istAktionen(const Blatt& p) { return std::string(p.id).find(".aktionen") != std::string::npos; }

ImGuiID wurzelId(int b) { return ImHashStr((std::string("bhed.dockraum.") + dock::praefix(b)).c_str()); }

// Wo jedes Panel in der Standardanordnung steht: der Weg von der Wurzel ('0'
// erstes, '1' zweites Kind), genau so, wie bauen() teilt. Ueber die Wege
// findet ein gespeicherter Baum (andere Knotennummern) seine Heimknoten.
std::vector<std::string> heimWege(int b) {
    const std::vector<Blatt> p = blaetter(b);
    std::vector<std::string> w(p.size());
    std::string kette;
    std::size_t spalten = p.size();
    if (mitBand(b)) {   // Teilung nach unten: oben die Spalten (erstes Kind), unten das Band
        w.back() = "1";
        kette = "0";
        spalten = p.size() - 1;
    }
    for (std::size_t i = 0; i + 1 < spalten; ++i) {
        if (i + 2 == spalten && istAktionen(p[i + 1])) {   // Actions rechts abgeteilt, Script Flow links davon
            w[i] = kette + "0";
            w[i + 1] = kette + "1";
        } else {
            w[i] = kette + "0";
            kette += "1";
            if (i + 2 == spalten) { w[i + 1] = kette; }
        }
    }
    return w;
}

// Die Heimknoten in einem (gespeicherten oder neu gebauten) Baum; 0, wo der
// Baum an der Stelle anders aussieht.
std::vector<ImGuiID> heimAusBaum(int b, const ImGuiDockNode* wurzel) {
    std::vector<ImGuiID> heim;
    for (const std::string& weg : heimWege(b)) {
        const ImGuiDockNode* n = wurzel;
        for (const char c : weg) {
            if (n == nullptr || n->IsLeafNode()) {
                n = nullptr;
                break;
            }
            n = n->ChildNodes[c == '1' ? 1 : 0];
        }
        heim.push_back(n != nullptr && n->IsLeafNode() ? n->ID : 0);
    }
    return heim;
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

// Die Modusleiste bleibt, wo sie ist: ohne Reiterleiste, nicht abdockbar,
// nichts darueber andockbar. NoUndocking und NoDockingOverMe schreibt ImGui
// nicht in die Einstellungen - nach dem Laden eines Baums daher neu setzen.
void modusFesthalten(ImGuiID knoten) {
    if (ImGuiDockNode* n = ImGui::DockBuilderGetNode(knoten); n != nullptr) {
        n->SetLocalFlags(n->LocalFlags | ImGuiDockNodeFlags_HiddenTabBar | static_cast<int>(ImGuiDockNodeFlags_NoTabBar) |
                         static_cast<int>(ImGuiDockNodeFlags_NoUndocking) | static_cast<int>(ImGuiDockNodeFlags_NoDockingOverMe));
    }
}

// Der Dock-Knoten, in dem ein Panel steht oder beim Anlegen stehen wird
// (gespeicherte Fenstereinstellung); 0 = keiner.
ImGuiID panelKnoten(const Blatt& p) {
    const std::string name = fensterName(p);
    if (const ImGuiWindow* w = ImGui::FindWindowByName(name.c_str()); w != nullptr) { return w->DockId; }
    const ImGuiWindowSettings* s = ImGui::FindWindowSettingsByID(ImHashStr(name.c_str()));
    return s != nullptr ? s->DockId : 0;
}

// Einen gespeicherten Baum des Bereichs uebernehmen (Neustart, geladene
// Anordnung): ImGui hat ihn aus den Einstellungen gebaut, die Panels kommen
// beim Anlegen aus ihren Fenstereinstellungen in ihre Knoten (oder schweben).
// Nur, wenn jedes Panel eine Einstellung hat - sonst stuende ein Panel ohne
// Platz irgendwo; dann die Standardanordnung.
bool uebernehmen(int b) {
    Raum& raum = g_raum[static_cast<std::size_t>(b)];
    const ImGuiID id = wurzelId(b);
    const ImGuiDockNode* n = ImGui::DockBuilderGetNode(id);
    if (n == nullptr || !n->IsDockSpace()) { return false; }
    const std::vector<Blatt> p = blaetter(b);
    for (const Blatt& x : p) {
        const std::string name = fensterName(x);
        if (ImGui::FindWindowByName(name.c_str()) == nullptr && ImGui::FindWindowSettingsByID(ImHashStr(name.c_str())) == nullptr) {
            diag::detail("Fenster: gespeicherter DockSpace von Bereich " + std::to_string(b) + " ohne Panel " + x.id + " - Standardanordnung");
            return false;
        }
    }
    raum.wurzel = id;
    raum.heim = heimAusBaum(b, n);
    raum.knoten = static_cast<int>(2 * p.size() - 1);
    raum.frisch = false;
    // dock4, Randfall Schalter an/aus/an: ohne DockSpace herausgenommene Panels
    // (Liste aus dock1, neuer als der Baum) schweben; die Liste ist erledigt.
    std::vector<std::string> ids;
    for (const Blatt& x : p) { ids.emplace_back(x.id); }
    dockvorgabe::alteListeSchweben(b, id, ids);
    modusFesthalten(panelKnoten(p[0]));
    diag::detail("Fenster: gespeicherter DockSpace von Bereich " + std::to_string(b) + " uebernommen (" + std::to_string(knotenZaehlen(n)) +
                 " Knoten)");
    return true;
}

void bauen(int b, const std::vector<ImRect>& r, const ImVec2& groesse) {
    Raum& raum = g_raum[static_cast<std::size_t>(b)];
    const std::vector<Blatt> p = blaetter(b);
    raum.wurzel = wurzelId(b);
    ImGui::DockBuilderRemoveNode(raum.wurzel);
    ImGui::DockBuilderAddNode(raum.wurzel, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(raum.wurzel, groesse);
    std::vector<ImGuiID> heim(p.size(), 0);
    ImGuiID kette = raum.wurzel;
    const std::size_t spalten = mitBand(b) ? p.size() - 1 : p.size();
    if (mitBand(b)) {
        const float anteil = std::clamp(r.back().GetHeight() / std::max(1.0F, groesse.y), 0.05F, 0.95F);
        ImGui::DockBuilderSplitNode(raum.wurzel, ImGuiDir_Down, anteil, &heim.back(), &kette);
    }
    float rest = groesse.x;
    for (std::size_t i = 0; i + 1 < spalten; ++i) {
        // Script Flow bleibt der Mittelknoten (nimmt beim freien Anordnen den
        // Rest; ein Mittelknoten bleibt auch leer stehen - Actions dagegen gibt
        // ausgeblendet oder herausgenommen seinen Platz ab).
        ImGuiID weiter = 0;
        if (i + 2 == spalten && istAktionen(p[i + 1])) {
            ImGui::DockBuilderSplitNode(kette, ImGuiDir_Right, std::clamp(r[i + 1].GetWidth() / std::max(1.0F, rest), 0.05F, 0.95F),
                                        &heim[i + 1], &heim[i]);
        } else {
            ImGui::DockBuilderSplitNode(kette, ImGuiDir_Left, std::clamp(r[i].GetWidth() / std::max(1.0F, rest), 0.05F, 0.95F),
                                        &heim[i], &weiter);
            kette = weiter;
            if (i + 2 == spalten) { heim[i + 1] = weiter; }
        }
        rest -= r[i].GetWidth();
    }
    for (std::size_t i = 0; i < p.size(); ++i) {
        if (ImGuiDockNode* n = ImGui::DockBuilderGetNode(heim[i]); n != nullptr) {
            n->SetLocalFlags(n->LocalFlags | ImGuiDockNodeFlags_HiddenTabBar);
        }
    }
    modusFesthalten(heim[0]);
    ImGui::DockBuilderFinish(raum.wurzel);
    raum.heim = heimAusBaum(b, ImGui::DockBuilderGetNode(raum.wurzel));
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

// `groesse`: die Groesse, die der Knoten bekommt. Das erste Kind bekommt genau
// seine gemessene Breite (Hoehe), das zweite den Rest - so landet die Breite,
// die das Band ueber die Spalten hinausreicht, in der letzten Spalte.
void erzwingen(ImGuiDockNode* n, const std::vector<ImGuiID>& heim, const std::vector<ImRect>& r, ImVec2 groesse) {
    if (n == nullptr || n->IsLeafNode()) { return; }
    const int achse = n->SplitAxis == ImGuiAxis_Y ? 1 : 0;
    const ImRect erstes = spanne(n->ChildNodes[0], heim, r);
    const float vorn = achse == 0 ? erstes.GetWidth() : erstes.GetHeight();
    ImVec2 g0 = groesse;
    ImVec2 g1 = groesse;
    g0[achse] = vorn;
    g1[achse] = std::max(1.0F, groesse[achse] - kAbstand - vorn);
    n->ChildNodes[0]->SizeRef = g0;
    n->ChildNodes[1]->SizeRef = g1;
    erzwingen(n->ChildNodes[0], heim, r, g0);
    erzwingen(n->ChildNodes[1], heim, r, g1);
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

bool draussen(const std::string& panel);

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
    if (raum.wurzel == 0 && !raum.neuBauen) { (void)uebernehmen(b); }
    if (raum.wurzel == 0 || raum.neuBauen || ImGui::DockBuilderGetNode(raum.wurzel) == nullptr) {
        raum.neuBauen = false;
        bauen(b, r, groesse);
    }
    // Eine Anordnung aus dock1 (Panels in eigenen Fenstern als Liste, ohne
    // DockSpace-Baum): diese Panels kommen beim Aufbau nicht in ihre Knoten,
    // sondern schweben an ihrer gespeicherten Stelle; danach gilt der Baum.
    const bool alteListe = raum.frisch && dock::listeDraussen(b);
    const bool std0 = standard(b);
    if (std0) { erzwingen(ImGui::DockBuilderGetNode(raum.wurzel), raum.heim, r, groesse); }
    ImGui::SetCursorScreenPos(alles.Min);
    const ImGuiWindowClass k = klasse();
    ImGui::PushStyleColor(ImGuiCol_Separator, ImGui::GetStyleColorVec4(ImGuiCol_TableBorderLight));
    ImGui::PushStyleVar(ImGuiStyleVar_DockingSeparatorSize, kAbstand);
    // AutoHideTabBar: zwei Panels in einem Knoten zeigen ihre Reiter (ImGui nimmt
    // HiddenTabBar dann weg), allein wieder nur das Dreieck.
    ImGui::DockSpace(raum.wurzel, groesse,
                     ImGuiDockNodeFlags_AutoHideTabBar | (std0 ? ImGuiDockNodeFlags_NoResize : ImGuiDockNodeFlags_None), &k);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    const std::vector<Blatt> p = blaetter(b);
    const std::size_t letzteSpalte = mitBand(b) ? p.size() - 2 : p.size() - 1;
    for (std::size_t i = 0; i < p.size(); ++i) {
        if (i > 0 && dock::versteckt(p[i].id)) { continue; }   // ausgeblendet: der Knoten bleibt leer, die Nachbarn nehmen den Platz
        const bool band = mitBand(b) && i + 1 == p.size();
        // Zellen der inneren Tabelle (Modusleiste, Ebenen/Animationen, Ansicht bzw.
        // Ereignisliste) beginnen ein Zellpolster tiefer als die der aeusseren
        // (Script Flow, Actions); alle enden ein Zellpolster ueber dem
        // Zeilenende der aeusseren Zeile (je Zeile oben und unten CellPadding.y).
        const bool innen = i < static_cast<std::size_t>(g_mess.innenN);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                            ImVec2{st.CellPadding.x, band ? st.ItemSpacing.y - 1.0F : st.CellPadding.y * 2.0F});
        int farben = 0;
        dockvorgabe::dreieckStil(farben);   // dock4: ImGuis Dreieck erst unter der Maus, ueber dem Inhalt
        ImGui::SetNextWindowClass(&k);
        const bool lassen = (alteListe && dock::inListeDraussen(p[i].id)) ||
                            std::find(raum.draussenLassen.begin(), raum.draussenLassen.end(), p[i].id) != raum.draussenLassen.end();
        // dock4, Schalter an/aus/an: ohne DockSpace heraus und wieder an seinen Platz -
        // sein Knoten ist weg (fenster_dock: heimVormerken). In sein Blatt, gibt es das
        // nicht mehr, die Standardanordnung neu (in eigenen Fenstern bleibt, was draussen ist).
        const ImGuiID knoten = panelKnoten(p[i]);
        if (!raum.frisch && !lassen && knoten != 0 && ImGui::DockBuilderGetNode(knoten) == nullptr) {
            const ImGuiDockNode* h = i < raum.heim.size() && raum.heim[i] != 0 ? ImGui::DockBuilderGetNode(raum.heim[i]) : nullptr;
            if (h != nullptr && h->IsLeafNode() && h->Windows.Size == 0) {
                ImGui::SetNextWindowDockID(raum.heim[i], ImGuiCond_Always);
            } else {
                raum.neuBauen = true;
                raum.draussenLassen.clear();
                for (const Blatt& x : p) {
                    if (x.id != p[i].id && draussen(x.id)) { raum.draussenLassen.emplace_back(x.id); }
                }
                diag::detail(std::string("Fenster: Panel ") + p[i].id + " ohne Knoten - Standardanordnung neu");
                ImGui::PopStyleColor(farben);
                ImGui::PopStyleVar();
                continue;
            }
        }
        if (raum.frisch && !lassen) { ImGui::SetNextWindowDockID(raum.heim[i], ImGuiCond_Always); }
        // In einem eigenen Fenster mit Schliessknopf: Schliessen dockt es wieder an
        // (wie dock1); im Hauptfenster ohne (die Reiterleiste ist versteckt).
        bool offen = true;
        const bool schwebt = i > 0 && draussen(p[i].id);
        // Ohne NoSavedSettings: ImGui schreibt Lage und Knoten der Panels und den
        // Baum in die Einstellungen (fenster_dock: merkenLaufend) - Neustart und
        // gespeicherte Anordnungen bringen ihn zurueck (uebernehmen).
        const bool sichtbar = ImGui::Begin(fensterName(p[i]).c_str(), schwebt ? &offen : nullptr,
                                           ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::PopStyleColor(farben);
        ImGui::PopStyleVar();
        const dockvorgabe::Dreieck dreieck = dockvorgabe::dreieckBeginnen();
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
        if (sichtbar && g_linie.an) {
            ImGuiWindow* w = ImGui::GetCurrentWindow();
            if (g_linie.x + 2.0F >= w->Pos.x && g_linie.x - 2.0F <= w->Pos.x + w->Size.x) {
                w->DrawList->PushClipRect(w->Pos, ImVec2{w->Pos.x + w->Size.x, w->Pos.y + w->Size.y}, false);
                w->DrawList->AddLine(ImVec2{g_linie.x, g_linie.oben}, ImVec2{g_linie.x, g_linie.unten}, g_linie.farbe, 2.0F);
                w->DrawList->PopClipRect();
            }
        }
        dockvorgabe::dreieckEnde(dreieck);
        ImGui::End();
        if (!offen) { dock::andocken(p[i].id); }
    }
    if (raum.frisch) { raum.draussenLassen.clear(); }
    if (alteListe) {
        diag::detail("Fenster: Anordnung aus dock1 fuer Bereich " + std::to_string(b) + " in den DockSpace uebernommen");
        dock::listeLeeren(b);
    }
    raum.frisch = false;
}

ImGuiWindow* panelFenster(const std::string& panel) {
    for (int b = 0; b < 3; ++b) {
        for (const Blatt& p : blaetter(b)) {
            if (panel == p.id) { return ImGui::FindWindowByName(fensterName(p).c_str()); }
        }
    }
    return nullptr;
}

// Ausserhalb des Hauptfensters: nicht im DockSpace des Bereichs angedockt.
bool draussen(const std::string& panel) {
    const ImGuiWindow* w = panelFenster(panel);
    if (w == nullptr) { return false; }
    // Ein Panel eines gerade nicht gezeigten Bereichs haelt nur seine DockId
    // (ImGui bindet es beim naechsten Begin wieder an den Knoten) - es steht
    // trotzdem im DockSpace (Selbsttest dock1: im ersten Bild nach dem Wechsel
    // des Bereichs galt es als draussen, Herausnehmen tat nichts).
    const ImGuiDockNode* n = w->DockNode != nullptr ? w->DockNode : (w->DockId != 0 ? ImGui::DockBuilderGetNode(w->DockId) : nullptr);
    while (n != nullptr && n->ParentNode != nullptr) { n = n->ParentNode; }
    return n == nullptr || !n->IsDockSpace();
}

void herausnehmen(const std::string& panel) {
    ImGuiWindow* w = panelFenster(panel);
    if (w == nullptr || draussen(panel) || panel.find(".modus") != std::string::npos) { return; }
    ImGui::DockContextQueueUndockWindow(GImGui, w);
    diag::detail("Fenster: Panel " + panel + " aus dem DockSpace in ein eigenes Fenster");
}

ImGuiID heimKnoten(const std::string& panel);

// Zurueck an seinen Platz der Standardanordnung: in seinen Knoten, wenn es ihn
// noch gibt (ausgeblendet oder als Reiter woanders), sonst die
// Standardanordnung des Bereichs neu aufbauen - Panels in eigenen Fenstern
// bleiben dabei draussen (ImGui verschmilzt einen Knoten, den sein letztes
// Panel verlaesst, mit dem Nachbarn; der Platz ist dann nicht mehr da).
void andocken(const std::string& panel) {
    ImGuiWindow* w = panelFenster(panel);
    const ImGuiID heim = heimKnoten(panel);
    const ImGuiDockNode* n = heim != 0 ? ImGui::DockBuilderGetNode(heim) : nullptr;
    if (w != nullptr && n != nullptr && n->IsLeafNode()) {
        ImGui::DockBuilderDockWindow(w->Name, heim);
        diag::detail("Fenster: Panel " + panel + " zurueck in seinen Knoten");
        return;
    }
    for (int b = 0; b < 3; ++b) {
        if (panel.rfind(dock::praefix(b), 0) != 0) { continue; }
        Raum& raum = g_raum[static_cast<std::size_t>(b)];
        // Abschluss rc607: neben die Panels auf der anderen Seite seiner Teilung der
        // Standardanordnung (der naechste zuerst) - eine freie Anordnung bleibt.
        const std::vector<Blatt> p = blaetter(b);
        const std::vector<std::string> wege = heimWege(b);
        for (std::size_t i = 0; w != nullptr && i < p.size(); ++i) {
            if (panel != p[i].id || wege[i].empty()) { continue; }
            const char seite = wege[i].back();
            const std::string gegenueber = wege[i].substr(0, wege[i].size() - 1) + (seite == '0' ? '1' : '0');
            std::vector<std::pair<std::string, ImGuiWindow*>> kandidaten;   // Weg von der Wurzel: links bzw. oben zuerst
            for (std::size_t j = 0; j < p.size(); ++j) {
                if (wege[j].rfind(gegenueber, 0) == 0 && !std::string(p[j].id).ends_with(".modus")) {
                    kandidaten.emplace_back(wege[j], ImGui::FindWindowByName(fensterName(p[j]).c_str()));
                }
            }
            std::sort(kandidaten.begin(), kandidaten.end(), [](const auto& x, const auto& y) { return x.first < y.first; });
            if (seite == '1') { std::reverse(kandidaten.begin(), kandidaten.end()); }
            std::vector<ImGuiWindow*> nachbarn;
            for (const auto& k : kandidaten) { nachbarn.push_back(k.second); }
            const bool band = mitBand(b) && wege[i].size() == 1;   // Teilung nach unten (bauen)
            const ImGuiDir richtung = band ? (seite == '0' ? ImGuiDir_Up : ImGuiDir_Down) : (seite == '0' ? ImGuiDir_Left : ImGuiDir_Right);
            if (dockvorgabe::nebenAndocken(w, nachbarn, richtung, raum.wurzel)) {
                diag::detail("Fenster: Panel " + panel + " - Knoten fehlt, neben seinen Nachbarn der Standardanordnung angedockt");
                return;
            }
        }
        raum.neuBauen = true;
        raum.draussenLassen.clear();
        for (const Blatt& x : blaetter(b)) {
            if (x.id != panel && draussen(x.id)) { raum.draussenLassen.emplace_back(x.id); }
        }
    }
    diag::detail("Fenster: Panel " + panel + " - Knoten fehlt, Standardanordnung neu");
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
