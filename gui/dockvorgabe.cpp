// dockvorgabe.cpp - Map Editor und Menu Editor im DockSpace (Stufe dock4)
//
// Beschreibung in dockvorgabe.h.
#include "dockvorgabe.h"

#include "bhed/diag.h"
#include "fenster_dock.h"

#include "imgui_internal.h"

#include <algorithm>
#include <array>
#include <string>
#include <vector>

namespace bhed::gui::dockvorgabe {

int Vorgabe::blatt(const char* panel) {
    panels.emplace_back(panel);
    Knoten k;
    k.panel = static_cast<int>(panels.size()) - 1;
    knoten.push_back(k);
    wurzel = static_cast<int>(knoten.size()) - 1;
    return wurzel;
}

int Vorgabe::teilung(bool untereinander, int a, int b) {
    Knoten k;
    k.kind[0] = a;
    k.kind[1] = b;
    k.untereinander = untereinander;
    knoten.push_back(k);
    wurzel = static_cast<int>(knoten.size()) - 1;
    return wurzel;
}

namespace {

struct Raum {
    std::string schluessel;            // die Vorgabe, deren Baum gilt
    std::vector<std::string> panels;   // ihre Panels
    ImGuiID wurzel = 0;
    bool geprueft = false;   // gespeicherter Baum gelesen (Start, geladene Anordnung)
    bool neuBauen = false;   // im naechsten dockspace() nach der Vorgabe bauen
    bool frisch = false;     // in diesem Bild gebaut: die Panels in ihre Blaetter
    bool alteListe = false;  // beim Bau: Panels der dock1-Liste bleiben draussen
    bool qt = false;         // der Baum entspricht der Vorgabe
    std::vector<ImGuiID> heim;   // Blatt je Panel der Vorgabe (0: keins)
    std::vector<std::string> draussenLassen;
    bool geschlossen = false;    // Schliessknopf des gerade begonnenen Panels
    Dreieck dreieck;             // ImGuis Dreieck des gerade begonnenen Panels
};
std::array<Raum, dock::kBereiche> g_raum;

Raum& raum(int bereich) { return g_raum[static_cast<std::size_t>(std::clamp(bereich, 0, dock::kBereiche - 1))]; }

ImGuiID wurzelId(const std::string& schluessel) { return ImHashStr(("bhed.dockraum." + schluessel).c_str()); }

ImGuiWindowClass klasse() {
    ImGuiWindowClass k;
    k.ClassId = ImHashStr("bhed.dock.panel");
    k.DockingAllowUnclassed = false;
    return k;
}

// Der Knoten, in dem das Fenster steht oder beim naechsten Begin andocken wird.
ImGuiID dockIdVon(const std::string& name) {
    if (const ImGuiWindow* w = ImGui::FindWindowByName(name.c_str()); w != nullptr) { return w->DockNode != nullptr ? w->DockNode->ID : w->DockId; }
    const ImGuiWindowSettings* s = ImGui::FindWindowSettingsByID(ImHashStr(name.c_str()));
    return s != nullptr ? s->DockId : 0;
}

bool imBaum(ImGuiID knoten, ImGuiID wurzel) {
    const ImGuiDockNode* n = knoten != 0 ? ImGui::DockBuilderGetNode(knoten) : nullptr;
    while (n != nullptr && n->ParentNode != nullptr) { n = n->ParentNode; }
    return n != nullptr && n->ID == wurzel;
}

// Entspricht der Teilbaum `n` dem Knoten `k` der Vorgabe? Ein Blatt darf leer
// sein (Panel ausgeblendet, eingeklappt) oder mit seiner ganzen Seite fehlen
// (ImGui verschmilzt das Blatt eines herausgenommenen Panels mit dem Nachbarn);
// in einem Blatt steht hoechstens sein eigenes Panel. Jeder Knoten des Baums
// muss so einem Knoten der Vorgabe entsprechen.
bool passt(const ImGuiDockNode* n, const Vorgabe& v, int k, const std::vector<const ImGuiWindow*>& fenster, std::vector<ImGuiID>& heim) {
    if (n == nullptr || k < 0) { return false; }
    const Knoten& kn = v.knoten[static_cast<std::size_t>(k)];
    if (kn.panel >= 0) {
        if (!n->IsLeafNode()) { return false; }
        for (const ImGuiWindow* w : n->Windows) {
            if (w == nullptr || w != fenster[static_cast<std::size_t>(kn.panel)]) { return false; }
        }
        heim[static_cast<std::size_t>(kn.panel)] = n->ID;
        return true;
    }
    if (n->IsSplitNode() && n->SplitAxis == (kn.untereinander ? ImGuiAxis_Y : ImGuiAxis_X)) {
        std::vector<ImGuiID> h = heim;
        if (passt(n->ChildNodes[0], v, kn.kind[0], fenster, h) && passt(n->ChildNodes[1], v, kn.kind[1], fenster, h)) {
            heim = h;
            return true;
        }
    }
    for (const int bleibt : {kn.kind[1], kn.kind[0]}) {   // die andere Seite ist verschmolzen
        std::vector<ImGuiID> h = heim;
        if (passt(n, v, bleibt, fenster, h)) {
            heim = h;
            return true;
        }
    }
    return false;
}

bool passtJetzt(const Raum& r, const Vorgabe& v, std::vector<ImGuiID>& heim) {
    std::vector<const ImGuiWindow*> fenster;
    for (const std::string& p : v.panels) { fenster.push_back(ImGui::FindWindowByName(dock::fensterName(p.c_str()).c_str())); }
    heim.assign(v.panels.size(), 0);
    const ImGuiDockNode* w = ImGui::DockBuilderGetNode(r.wurzel);
    return w != nullptr && w->IsDockSpace() && passt(w, v, v.wurzel, fenster, heim);
}

bool gueltig(const ImRect& r) { return r.GetWidth() > 0.0F && r.GetHeight() > 0.0F; }

// Die Ausdehnung der Panels eines Teilbaums der Vorgabe entlang der Achse.
float spanne(const Vorgabe& v, int k, const std::vector<ImRect>& rechteck, int achse) {
    std::vector<int> offen{k};
    float lo = 0.0F;
    float hi = 0.0F;
    bool da = false;
    while (!offen.empty()) {
        const Knoten& kn = v.knoten[static_cast<std::size_t>(offen.back())];
        offen.pop_back();
        if (kn.panel < 0) {
            offen.push_back(kn.kind[0]);
            offen.push_back(kn.kind[1]);
            continue;
        }
        const ImRect& r = rechteck[static_cast<std::size_t>(kn.panel)];
        if (!gueltig(r)) { continue; }
        lo = da ? std::min(lo, r.Min[achse]) : r.Min[achse];
        hi = da ? std::max(hi, r.Max[achse]) : r.Max[achse];
        da = true;
    }
    return da ? hi - lo : 0.0F;
}

void teilen(ImGuiID id, const Vorgabe& v, int k, const std::vector<ImRect>& rechteck, std::vector<ImGuiID>& heim) {
    const Knoten& kn = v.knoten[static_cast<std::size_t>(k)];
    if (kn.panel >= 0) {
        heim[static_cast<std::size_t>(kn.panel)] = id;
        if (ImGuiDockNode* n = ImGui::DockBuilderGetNode(id); n != nullptr) { n->SetLocalFlags(n->LocalFlags | ImGuiDockNodeFlags_HiddenTabBar); }
        return;
    }
    const int achse = kn.untereinander ? 1 : 0;
    const float a = spanne(v, kn.kind[0], rechteck, achse);
    const float b = spanne(v, kn.kind[1], rechteck, achse);
    const float anteil = (a > 0.0F && b > 0.0F) ? std::clamp(a / (a + b), 0.05F, 0.95F) : 0.5F;
    ImGuiID erstes = 0;
    ImGuiID zweites = 0;
    ImGui::DockBuilderSplitNode(id, kn.untereinander ? ImGuiDir_Up : ImGuiDir_Left, anteil, &erstes, &zweites);
    teilen(erstes, v, kn.kind[0], rechteck, heim);
    teilen(zweites, v, kn.kind[1], rechteck, heim);
}

// Kein Mittelknoten: ein leeres Blatt gibt seinen Platz ab, wie ein
// versteckter Teil eines QSplitter (DockBuilderSplitNode reicht die Eigenschaft
// der Wurzel an ein Kind weiter).
void ohneMitte(ImGuiDockNode* n) {
    if (n == nullptr) { return; }
    n->SetLocalFlags(n->LocalFlags & ~static_cast<ImGuiDockNodeFlags>(ImGuiDockNodeFlags_CentralNode));
    ohneMitte(n->ChildNodes[0]);
    ohneMitte(n->ChildNodes[1]);
}

void bauen(int bereich, Raum& r, const Vorgabe& v, const std::vector<ImRect>& rechteck, const ImRect& flaeche) {
    ImGui::DockBuilderRemoveNode(r.wurzel);
    ImGui::DockBuilderAddNode(r.wurzel, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodePos(r.wurzel, flaeche.Min);
    ImGui::DockBuilderSetNodeSize(r.wurzel, ImVec2{std::max(1.0F, flaeche.GetWidth()), std::max(1.0F, flaeche.GetHeight())});
    r.heim.assign(v.panels.size(), 0);
    teilen(r.wurzel, v, v.wurzel, rechteck, r.heim);
    ohneMitte(ImGui::DockBuilderGetNode(r.wurzel));
    ImGui::DockBuilderFinish(r.wurzel);
    r.frisch = true;
    r.alteListe = dock::listeDraussen(bereich);
    diag::detail("Fenster: DockSpace " + r.schluessel + " nach der Vorgabe aufgebaut (" + std::to_string(v.panels.size()) + " Panels" +
                 (r.alteListe ? ", Anordnung aus dock1" : "") + ")");
}

// Einen gespeicherten Baum uebernehmen (Neustart, geladene Anordnung) - nur,
// wenn jedes eingeblendete Panel eine Fenstereinstellung hat (sonst stuende es
// ohne Platz irgendwo).
bool uebernehmen(int bereich, Raum& r, const Vorgabe& v) {
    const ImGuiDockNode* n = ImGui::DockBuilderGetNode(r.wurzel);
    if (n == nullptr || !n->IsDockSpace()) { return false; }
    for (const std::string& p : v.panels) {
        const std::string name = dock::fensterName(p.c_str());
        if (!dock::versteckt(p.c_str()) && ImGui::FindWindowByName(name.c_str()) == nullptr &&
            ImGui::FindWindowSettingsByID(ImHashStr(name.c_str())) == nullptr) {
            diag::detail("Fenster: gespeicherter DockSpace " + r.schluessel + " ohne Panel " + p + " - Vorgabe");
            return false;
        }
    }
    alteListeSchweben(bereich, r.wurzel, v.panels);
    diag::detail("Fenster: gespeicherter DockSpace " + r.schluessel + " uebernommen");
    return true;
}

void groessenSetzen(ImGuiDockNode* n, const std::vector<ImGuiID>& heim, const std::vector<ImRect>& rechteck, ImRect& spann, bool& da) {
    da = false;
    if (n == nullptr) { return; }
    if (n->IsLeafNode()) {
        for (std::size_t i = 0; i < heim.size(); ++i) {
            if (heim[i] != n->ID || !gueltig(rechteck[i])) { continue; }
            n->Pos = rechteck[i].Min;
            n->Size = rechteck[i].GetSize();
            n->SizeRef = n->Size;
            spann = rechteck[i];
            da = true;
        }
        return;
    }
    ImRect a;
    ImRect b;
    bool aDa = false;
    bool bDa = false;
    groessenSetzen(n->ChildNodes[0], heim, rechteck, a, aDa);
    groessenSetzen(n->ChildNodes[1], heim, rechteck, b, bDa);
    if (aDa && bDa) {
        n->ChildNodes[0]->SizeRef = a.GetSize();
        n->ChildNodes[1]->SizeRef = b.GetSize();
    }
    spann = aDa ? a : b;
    if (aDa && bDa) { spann.Add(b); }
    da = aDa || bDa;
}

}  // namespace

void alteListeSchweben(int bereich, ImGuiID wurzel, const std::vector<std::string>& panels) {
    if (!dock::listeDraussen(bereich)) { return; }
    for (const std::string& p : panels) {
        if (!dock::inListeDraussen(p.c_str())) { continue; }
        const std::string name = dock::fensterName(p.c_str());
        if (ImGuiWindow* w = ImGui::FindWindowByName(name.c_str()); w != nullptr) {
            if (imBaum(w->DockNode != nullptr ? w->DockNode->ID : w->DockId, wurzel)) {
                if (w->DockNode != nullptr) {
                    ImGui::DockContextQueueUndockWindow(GImGui, w);
                } else {
                    w->DockId = 0;
                }
            }
        } else if (ImGuiWindowSettings* s = ImGui::FindWindowSettingsByID(ImHashStr(name.c_str())); s != nullptr && imBaum(s->DockId, wurzel)) {
            s->DockId = 0;
        }
    }
    dock::listeLeeren(bereich);
    diag::detail("Fenster: Liste aus dock1 fuer Bereich " + std::to_string(bereich) + " - Panels schweben, Liste geleert");
}

bool beginnen(int bereich, const Vorgabe& v) {
    Raum& r = raum(bereich);
    if (r.schluessel != v.schluessel) {
        // Andere Vorgabe (NRC-Anordnung, Belegung der Split-Plaetze, Layout):
        // Panels, die gerade schweben, bleiben draussen; der Baum der alten
        // Vorgabe weicht (seine Panels kommen in die Blaetter der neuen).
        const bool wechsel = !r.schluessel.empty();
        // Beim ersten Bild des Bereichs bleiben Wuensche von vorher (herausnehmen, bevor
        // das Panel je gezeichnet wurde) stehen.
        std::vector<std::string> lassen = wechsel ? std::vector<std::string>{} : r.draussenLassen;
        if (wechsel) {
            for (const std::string& p : r.panels) {
                const ImGuiWindow* w = ImGui::FindWindowByName(dock::fensterName(p.c_str()).c_str());
                if (w != nullptr && w->WasActive && draussen(bereich, p.c_str())) { lassen.push_back(p); }
            }
            ImGui::DockBuilderRemoveNode(r.wurzel);
            diag::detail("Fenster: Vorgabe " + r.schluessel + " -> " + v.schluessel);
        }
        r = Raum{};
        r.schluessel = v.schluessel;
        r.wurzel = wurzelId(v.schluessel);
        r.draussenLassen = lassen;
        r.geprueft = wechsel;
        r.neuBauen = wechsel;
    }
    r.panels = v.panels;
    if (!r.geprueft) {
        r.geprueft = true;
        if (!uebernehmen(bereich, r, v)) { r.neuBauen = true; }
    }
    if (r.neuBauen || ImGui::DockBuilderGetNode(r.wurzel) == nullptr) { return true; }   // wird in diesem Bild nach der Vorgabe gebaut
    r.qt = passtJetzt(r, v, r.heim);
    return r.qt;
}

void dockspace(int bereich, const Vorgabe& v, const std::vector<ImRect>& rechteck, const ImRect& flaeche, float abstand) {
    Raum& r = raum(bereich);
    if (r.neuBauen || ImGui::DockBuilderGetNode(r.wurzel) == nullptr) {
        r.neuBauen = false;
        bauen(bereich, r, v, rechteck, flaeche);
    }
    r.qt = passtJetzt(r, v, r.heim);
    // Der DockSpace ist ein Element an der Stelle der Flaeche; danach steht der
    // Cursor darunter (nicht zuruecksetzen: ImGui erwartet nach SetCursorPos ein Element).
    ImGui::SetCursorScreenPos(flaeche.Min);
    const ImGuiWindowClass k = klasse();
    ImGui::PushStyleVar(ImGuiStyleVar_DockingSeparatorSize, abstand);
    // Entspricht der Baum der Vorgabe, zeichnet der Bereich seine Griffe selbst
    // (ImGui fuellt den Abstand sonst mit der Randfarbe).
    if (r.qt) { ImGui::PushStyleColor(ImGuiCol_Border, ImVec4{0.0F, 0.0F, 0.0F, 0.0F}); }
    ImGui::DockSpace(r.wurzel, ImVec2{std::max(1.0F, flaeche.GetWidth()), std::max(1.0F, flaeche.GetHeight())},
                     ImGuiDockNodeFlags_AutoHideTabBar | (r.qt ? ImGuiDockNodeFlags_NoResize : ImGuiDockNodeFlags_None), &k);
    if (r.qt) { ImGui::PopStyleColor(); }
    ImGui::PopStyleVar();
    if (r.qt) {
        // Die Blaetter genau auf die Rechtecke des Bereichs (ImGui hat sie eben
        // aus den Anteilen gerechnet); die Panels setzen sich beim Begin darauf.
        ImRect spann;
        bool da = false;
        groessenSetzen(ImGui::DockBuilderGetNode(r.wurzel), r.heim, rechteck, spann, da);
    }
}

bool zeigen(int bereich, const Vorgabe& v, int i, const std::vector<ImRect>& rechteck) {
    const char* id = v.panels[static_cast<std::size_t>(i)].c_str();
    if (dock::versteckt(id)) { return false; }
    if (!raum(bereich).qt || draussen(bereich, id)) { return true; }
    return gueltig(rechteck[static_cast<std::size_t>(i)]);
}

bool panelBeginnen(int bereich, const Vorgabe& v, int i) {
    Raum& r = raum(bereich);
    const std::string& id = v.panels[static_cast<std::size_t>(i)];
    const std::string name = dock::fensterName(id.c_str());
    const ImGuiWindow* w = ImGui::FindWindowByName(name.c_str());
    const bool neu = w == nullptr && ImGui::FindWindowSettingsByID(ImHashStr(name.c_str())) == nullptr;
    const ImGuiID dockId = dockIdVon(name);
    // Neu, oder sein Knoten ist weg (neu gebaut, waehrend es ausgeblendet war;
    // andere Vorgabe): in sein Blatt - ImGui legte sonst einen schwebenden
    // Knoten an der alten Stelle an.
    const bool verwaist = neu || (dockId != 0 && ImGui::DockBuilderGetNode(dockId) == nullptr);
    const bool lassen = std::find(r.draussenLassen.begin(), r.draussenLassen.end(), id) != r.draussenLassen.end() ||
                        (r.frisch && r.alteListe && dock::inListeDraussen(id.c_str()));
    bool imRaum = !draussen(bereich, id.c_str());
    if ((r.frisch || verwaist) && !lassen && static_cast<std::size_t>(i) < r.heim.size() && r.heim[static_cast<std::size_t>(i)] != 0) {
        ImGui::SetNextWindowDockID(r.heim[static_cast<std::size_t>(i)], ImGuiCond_Always);
        imRaum = true;
    } else if (neu) {   // neu und draussen gelassen: ein eigenes Fenster an seiner Startlage
        dock::startLage(id.c_str());
        imRaum = false;
    }
    const ImGuiWindowClass k = klasse();
    ImGui::SetNextWindowClass(&k);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    int farben = 0;
    if (imRaum) {   // im DockSpace wie vorher ein Kindfenster: ohne eigenen Hintergrund
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetStyleColorVec4(ImGuiCol_ChildBg));
        ++farben;
    }
    dreieckStil(farben);
    bool offen = true;
    const bool sichtbar = ImGui::Begin(name.c_str(), imRaum ? nullptr : &offen,
                                       ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleColor(farben);
    ImGui::PopStyleVar();
    r.geschlossen = !offen;
    r.dreieck = dreieckBeginnen();
    return sichtbar;
}

void panelEnde(int bereich, const Vorgabe& v, int i) {
    Raum& r = raum(bereich);
    dreieckEnde(r.dreieck);
    r.dreieck = Dreieck{};
    ImGui::End();
    if (r.geschlossen) {   // Schliessknopf eines eigenen Fensters: zurueck an seinen Platz
        r.geschlossen = false;
        andocken(bereich, v.panels[static_cast<std::size_t>(i)].c_str());
    }
}

void fertig(int bereich) {
    Raum& r = raum(bereich);
    if (r.frisch) {
        r.draussenLassen.clear();
        if (r.alteListe) {
            dock::listeLeeren(bereich);
            diag::detail("Fenster: Anordnung aus dock1 fuer Bereich " + std::to_string(bereich) + " in den DockSpace uebernommen");
        }
    }
    r.frisch = false;
    r.alteListe = false;
}

void dreieckStil(int& farben) {
    for (const ImGuiCol c : {ImGuiCol_Button, ImGuiCol_ButtonHovered, ImGuiCol_ButtonActive}) {
        ImGui::PushStyleColor(c, ImVec4{0.0F, 0.0F, 0.0F, 0.0F});
        ++farben;
    }
}

Dreieck dreieckBeginnen() {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* w = g.CurrentWindow;
    Dreieck d;
    if (w == nullptr || !w->DockIsActive || w->DockNode == nullptr || !w->DockNode->IsHiddenTabBar() || w->DockNode->IsNoTabBar()) { return d; }
    const ImGuiID id = ImHashStr("#UNHIDE", 0, w->ID);   // window->GetID("#UNHIDE") in RenderWindowDecorations
    if (g.HoveredId != id && g.ActiveId != id) { return d; }
    // Wie ImGui (RenderWindowDecorations), aber ueber dem Inhalt.
    const float gross = ImTrunc(g.FontSize * 0.70F);
    const ImVec2 p = w->DockNode->Pos;
    ImGui::GetForegroundDrawList(w->Viewport)
        ->AddTriangleFilled(p, ImVec2{p.x + gross, p.y}, ImVec2{p.x, p.y + gross},
                            ImGui::GetColorU32(g.ActiveId == id ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered));
    d.aktiv = true;
    d.vorher = g.HoveredWindow;
    g.HoveredWindow = nullptr;
    return d;
}

void dreieckEnde(const Dreieck& d) {
    if (d.aktiv && GImGui->HoveredWindow == nullptr) { GImGui->HoveredWindow = d.vorher; }
}

bool draussen(int bereich, const char* panel) {
    const std::string name = dock::fensterName(panel);
    const ImGuiWindow* w = ImGui::FindWindowByName(name.c_str());
    ImGuiID knoten = 0;
    if (w != nullptr) {
        knoten = w->DockNode != nullptr ? w->DockNode->ID : w->DockId;
    } else if (const ImGuiWindowSettings* st = ImGui::FindWindowSettingsByID(ImHashStr(name.c_str())); st != nullptr) {
        knoten = st->DockId;   // in dieser Sitzung noch nicht gezeichnet: wie gespeichert (Neustart)
    } else {   // noch nie gezeichnet: an seinem Platz - oder es soll in ein eigenes Fenster
        const Raum& r = raum(bereich);
        return std::find(r.draussenLassen.begin(), r.draussenLassen.end(), panel) != r.draussenLassen.end();
    }
    const ImGuiDockNode* n = knoten != 0 ? ImGui::DockBuilderGetNode(knoten) : nullptr;
    if (n == nullptr && knoten != 0) { return false; }   // Knoten weg: kommt beim Zeigen in sein Blatt
    while (n != nullptr && n->ParentNode != nullptr) { n = n->ParentNode; }
    return n == nullptr || !n->IsDockSpace();
}

void herausnehmen(int bereich, const char* panel) {
    ImGuiWindow* w = ImGui::FindWindowByName(dock::fensterName(panel).c_str());
    if (w == nullptr) {   // noch nie gezeichnet (Bereich noch nicht gezeigt, Menue-Editor ohne Menue)
        Raum& r = raum(bereich);
        if (std::find(r.draussenLassen.begin(), r.draussenLassen.end(), panel) == r.draussenLassen.end()) { r.draussenLassen.emplace_back(panel); }
        diag::detail(std::string("Fenster: Panel ") + panel + " kommt beim ersten Zeichnen in ein eigenes Fenster");
        return;
    }
    if (draussen(bereich, panel)) { return; }
    ImGui::DockContextQueueUndockWindow(GImGui, w);
    diag::detail(std::string("Fenster: Panel ") + panel + " aus dem DockSpace in ein eigenes Fenster");
}

void andocken(int bereich, const char* panel) {
    Raum& r = raum(bereich);
    const ImGuiWindow* w = ImGui::FindWindowByName(dock::fensterName(panel).c_str());
    ImGuiID blatt = 0;
    for (std::size_t i = 0; i < r.panels.size() && i < r.heim.size(); ++i) {
        if (r.panels[i] == panel && r.qt) { blatt = r.heim[i]; }
    }
    const ImGuiDockNode* n = blatt != 0 ? ImGui::DockBuilderGetNode(blatt) : nullptr;
    if (w != nullptr && n != nullptr && n->IsLeafNode() && n->Windows.Size == 0) {
        ImGui::DockBuilderDockWindow(w->Name, blatt);
        diag::detail(std::string("Fenster: Panel ") + panel + " zurueck in sein Blatt");
        return;
    }
    // Sein Blatt ist verschmolzen: die Vorgabe neu aufbauen, Panels in eigenen
    // Fenstern bleiben draussen.
    r.neuBauen = true;
    r.draussenLassen.clear();
    for (const std::string& p : r.panels) {
        if (p != panel && draussen(bereich, p.c_str())) { r.draussenLassen.push_back(p); }
    }
    diag::detail(std::string("Fenster: Panel ") + panel + " - Blatt fehlt, Vorgabe " + r.schluessel + " neu");
}

void zuruecksetzen(int bereich) {
    Raum& r = raum(bereich);
    r.neuBauen = true;
    r.draussenLassen.clear();
}

void neuLesen(int bereich) {
    Raum& r = raum(bereich);
    r.geprueft = false;
    r.neuBauen = false;
}

bool standard(int bereich) {
    const Raum& r = raum(bereich);
    return r.wurzel != 0 && r.qt && !r.neuBauen;
}

std::string schluesselVon(int bereich) { return raum(bereich).schluessel; }

}  // namespace bhed::gui::dockvorgabe
