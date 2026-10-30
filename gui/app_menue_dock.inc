// app_menue_dock.inc - dock4: der Menu Editor im DockSpace
//
// Eingebunden von gui/app_menue.cpp vor zeichneInnen (im Namensraum
// menueeditor), weil die Panels und ihr Zustand dort stehen.
//
// Beide Layouts haben eine Vorgabe (dockvorgabe.h): "Reiter rechts" mit
// Bausteinen, Flaeche und Reiter-Spalte nebeneinander; "Text unten" mit
// Bausteinen und Gliederung, Flaeche, Eigenschaften nebeneinander und dem Text
// darunter. zeichneInnen rechnet die Spalten wie bisher (Teiler, Breiten aus
// den Einstellungen); im DockSpace misst spalte() nur, und die Blaetter des
// Baums bekommen genau diese Rechtecke.

struct MenueDock {
    bool aktiv = false;   // der DockSpace gilt fuer den Menu Editor
    bool qt = false;      // der Baum entspricht der Vorgabe: Teiler und Masse von zeichneInnen
    std::vector<std::pair<std::string, ImRect>> rechteck;
};
MenueDock g_menueDock;
dockvorgabe::Vorgabe g_menueVorgabe;

dockvorgabe::Vorgabe menueVorgabe() {
    dockvorgabe::Vorgabe v;
    const bool unten = g_app != nullptr && g_app->settings.menueTextUnten;
    const int links = v.blatt("menue.bausteine");
    const int mitte = v.blatt("menue.flaeche");
    const int paar = v.teilung(false, links, mitte);
    const int rechts = v.blatt(unten ? "menue.eigenschaften" : "menue.reiter");
    const int reihe = v.teilung(false, paar, rechts);
    if (unten) {
        const int text = v.blatt("menue.text");
        (void)v.teilung(true, reihe, text);
    }
    v.schluessel = unten ? "menue.textunten" : "menue.reiter";
    return v;
}

bool menueDockBeginnen() {
    g_menueDock = MenueDock{};
    if (!dockraumAn()) { return false; }
    g_menueVorgabe = menueVorgabe();
    g_menueDock.aktiv = true;
    g_menueDock.qt = dockvorgabe::beginnen(4, g_menueVorgabe);
    return true;
}

// Die Spalten der Layouts: Kindfenster (Name, Rollen; alle mit Rand) und Inhalt.
struct SpalteArt {
    const char* panel;
    const char* kind;   // nullptr: ohne eigenes Kindfenster (der Text)
    ImGuiWindowFlags fenster;
};

const SpalteArt& spalteArt(const char* panel) {
    static const SpalteArt arten[] = {
        {"menue.bausteine", "##links", ImGuiWindowFlags_None},
        {"menue.flaeche", "##mitte", ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse},
        {"menue.reiter", "##rechts", ImGuiWindowFlags_None},
        {"menue.eigenschaften", "##rechts", ImGuiWindowFlags_None},
        {"menue.text", nullptr, ImGuiWindowFlags_None},
    };
    for (const SpalteArt& a : arten) {
        if (std::strcmp(a.panel, panel) == 0) { return a; }
    }
    return arten[0];
}

void spalteInhalt(const char* panel, Dokument& d) {
    const std::string p = panel;
    if (p == "menue.bausteine") {
        panelBausteine(d);
        if (g_app->settings.menueTextUnten) { panelHierarchie(d); }   // "Text unten": links auch die Gliederung
    } else if (p == "menue.flaeche") {
        panelFlaeche(d);
    } else if (p == "menue.reiter") {
        panelReiter(d);
    } else if (p == "menue.eigenschaften") {
        panelEigenschaften(d);
    } else if (p == "menue.text") {
        panelText(d);
    }
}

// Eine Spalte an ihrer Stelle im Fluss, Groesse wie bei BeginChild (0 = der
// Rest). Im DockSpace nur gemessen (das Panel zeichnet in menueDockZeichnen).
void spalte(const char* panel, ImVec2 groesse, Dokument& d) {
    const SpalteArt& a = spalteArt(panel);
    if (g_menueDock.aktiv) {
        const ImVec2 rest = ImGui::GetContentRegionAvail();
        const ImVec2 g = ImTrunc(ImGui::CalcItemSize(groesse, rest.x, rest.y));
        const ImVec2 p = ImGui::GetCursorScreenPos();
        g_menueDock.rechteck.emplace_back(panel, ImRect{p, ImVec2{p.x + g.x, p.y + g.y}});
        ImGui::Dummy(g);   // wie EndChild: ItemSize und ItemAdd mit der Groesse
        dock::heimPlatz(panel, ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        return;
    }
    if (a.kind == nullptr) {
        spalteInhalt(panel, d);
        return;
    }
    ImGui::BeginChild(a.kind, groesse, ImGuiChildFlags_Borders, a.fenster);
    spalteInhalt(panel, d);
    ImGui::EndChild();
    dock::heimPlatz(panel, ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
}

// Ein Teiler zwischen zwei Spalten. Im DockSpace liegt dessen Wirt (ein
// Kindfenster) ueber dem Abstand - der Teiler gilt auch dort; entspricht der
// Baum nicht mehr der Vorgabe, teilen ImGuis Teiler, hier nur der Platz.
void spaltenTeiler(const char* id, float& wert, bool senkrecht, float min, float max, float laenge, bool umgekehrt = false) {
    if (g_menueDock.aktiv && !g_menueDock.qt) {
        ImGui::Dummy(senkrecht ? ImVec2{6, std::max(1.0F, laenge)} : ImVec2{std::max(1.0F, laenge), 6});
        return;
    }
    splitter(id, wert, senkrecht, min, max, laenge, umgekehrt,
             static_cast<ImGuiButtonFlags>(g_menueDock.aktiv ? static_cast<int>(ImGuiButtonFlags_FlattenChildren) : 0));
}

// Ohne Menue (zeichneInnen zeigt nur den Hinweis): kein DockSpace, aber Panels in
// eigenen Fenstern stehen weiter da (wie dock1) - mit demselben Hinweis.
void menueDockOhneMenue() {
    if (!dockraumAn()) { return; }
    const dockvorgabe::Vorgabe v = menueVorgabe();
    for (std::size_t i = 0; i < v.panels.size(); ++i) {
        if (!dock::draussen(v.panels[i].c_str()) || dock::versteckt(v.panels[i].c_str())) { continue; }
        const int k = static_cast<int>(i);
        if (dockvorgabe::panelBeginnen(4, v, k)) { ImGui::TextDisabled("%s", tr(Str::MeKeinMenue)); }
        dockvorgabe::panelEnde(4, v, k);
    }
}

// Nach den Spalten: der DockSpace auf `flaeche` und die Panels in ihren
// Blaettern (auch in eigenen Fenstern) - jedes mit seinem Kindfenster wie vorher.
void menueDockZeichnen(Dokument& d, const ImRect& flaeche) {
    const dockvorgabe::Vorgabe& v = g_menueVorgabe;
    std::vector<ImRect> r(v.panels.size());
    for (std::size_t i = 0; i < v.panels.size(); ++i) {
        for (const auto& [p, x] : g_menueDock.rechteck) {
            if (p == v.panels[i]) { r[i] = x; }
        }
    }
    dockvorgabe::dockspace(4, v, r, flaeche, 6.0F);
    for (std::size_t i = 0; i < v.panels.size(); ++i) {
        const int k = static_cast<int>(i);
        if (!dockvorgabe::zeigen(4, v, k, r)) { continue; }
        if (dockvorgabe::panelBeginnen(4, v, k)) {
            const SpalteArt& a = spalteArt(v.panels[i].c_str());
            if (a.kind != nullptr) {
                ImGui::BeginChild(a.kind, ImVec2{0.0F, 0.0F}, ImGuiChildFlags_Borders, a.fenster);
                spalteInhalt(a.panel, d);
                ImGui::EndChild();
            } else {
                spalteInhalt(a.panel, d);
            }
        }
        dockvorgabe::panelEnde(4, v, k);
    }
    dockvorgabe::fertig(4);
}
