// fenster_dock.cpp - Panels herausnehmen, andocken, Anordnungen (Stufe dock1)
//
// Beschreibung in fenster_dock.h. Die Fensterlagen haelt ImGui (Docking- und
// Viewport-Zweig 1.92.9b); hier steht nur, welches Panel wo ist, wie das in
// die Einstellungen kommt, und das Zurueckholen von Fenstern, deren
// Bildschirm fehlt.
#include "fenster_dock.h"

#include "app_internal.h"
#include "bhed/diag.h"
#include "bhed/fensteranordnung.h"
#include "dockvorgabe.h"
#include "hilfe.h"
#include "tour.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cstring>
#include <array>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

namespace bhed::gui::dock {

namespace {

// Das Rechteck eines Panels im Hauptfenster, auf die Ecke des Hauptfensters
// bezogen (das Hauptfenster kann inzwischen woanders stehen).
struct Heim {
    ImVec2 a;
    ImVec2 b;
};

struct Zustand {
    std::array<fenster::Anordnung, kBereiche> jetzt;
    std::array<VerfuegbarFn, kBereiche> verfuegbar{};
    bool geladen = false;          // Einstellungen beim Start uebernommen
    // Eine geladene Anordnung: Ini-Text und Panels, beide erst vor dem
    // naechsten ImGui::NewFrame uebernommen (vorBild).
    std::string iniLaden;
    int ladenBereich = -1;
    fenster::Anordnung ladenAnordnung;
    bool merken = false;           // Einstellungen beim naechsten Bildanfang schreiben
    std::map<std::string, Heim> heim;
    // Ziehen auf den alten Platz: das Panel, dessen Fenster im letzten Bild
    // ueber seinem Platz gezogen wurde.
    std::string ueberPlatz;
    std::string offenesFenster;    // zwischen fensterBeginnen und fensterEnde
    bool fensterGeschlossen = false;
    // "Anordnung speichern unter"
    bool speichernOffen = false;
    char name[64] = "";
};

Zustand& z() {
    static Zustand s;
    return s;
}

const char* const kPraefix[kBereiche] = {"ereignisse.", "mission.", "modell.", "karte.", "menue."};

std::string panelFensterName(const Panel& p) { return std::string(tr(p.titel)) + fenster::kFensterKennung + p.id; }

// dock4, Schalter an/aus/an: ImGui loest ein Fenster aus seinem Dock-Baum, sobald es
// gezeichnet wird, waehrend sein DockSpace nicht gezeichnet wird - ohne DockSpace
// herausgenommen haengt ein Panel danach an keinem Knoten mehr. Kommt es ohne
// DockSpace wieder an seinen Platz, bekommt sein Fenster einen Knoten, den es nicht
// gibt: mit DockSpace setzt ihn der Bereich dann in sein Blatt (wie nach einem
// Neuaufbau), statt dass es schwebt. Ohne DockSpace wird die Marke beim naechsten
// Herausnehmen wieder entfernt (fensterBeginnen).
void heimVormerken(const Panel& p) {
    if (ImGuiWindow* w = ImGui::FindWindowByName(panelFensterName(p).c_str()); w != nullptr && w->DockNode == nullptr) {
        w->DockId = ImHashStr((std::string("bhed.dock.heim.") + p.id).c_str());
    }
}

bool istPanelFenster(const ImGuiWindow* w) {
    return w != nullptr && std::string(w->Name).find(fenster::kFensterKennung) != std::string::npos;
}

void merkenLaufend() {
    if (g_app == nullptr || ImGui::GetCurrentContext() == nullptr) { return; }
    std::size_t n = 0;
    const char* text = ImGui::SaveIniSettingsToMemory(&n);
    const std::string ini(text, n);
    for (int b = 0; b < kBereiche; ++b) {
        z().jetzt[static_cast<std::size_t>(b)].ini = fenster::iniFiltern(ini, kPraefix[b]);
        g_app->settings.fenster[fenster::schluesselLaufend(b)] = fenster::kodieren(z().jetzt[static_cast<std::size_t>(b)]);
    }
    ImGui::GetIO().WantSaveIniSettings = false;
}

// Beim Start: die laufenden Anordnungen aller Bereiche und ihre Fensterlagen.
void ausEinstellungenLaden() {
    std::string ini;
    for (int b = 0; b < kBereiche; ++b) {
        fenster::Anordnung& a = z().jetzt[static_cast<std::size_t>(b)];
        const auto it = g_app->settings.fenster.find(fenster::schluesselLaufend(b));
        if (it == g_app->settings.fenster.end() || !fenster::dekodieren(it->second, a)) {
            a = fenster::Anordnung{};
            continue;
        }
        ini = fenster::iniErsetzen(ini, a.ini, kPraefix[b]);
    }
    if (!ini.empty()) { ImGui::LoadIniSettingsFromMemory(ini.c_str(), ini.size()); }
    diag::detail("Fenster: Anordnung beim Start uebernommen (" + std::to_string(ini.size()) + " Zeichen Fensterlagen)");
}

// --- Fenster zurueckholen ----------------------------------------------------
//
// ImGui haelt ein eigenes Fenster nur so weit im Bild, dass ein Rand von
// DisplayWindowPadding sichtbar bleibt (imgui.cpp, ClampWindowPos), und
// verkleinert es nie. Fehlt sein Bildschirm (abgezogen, anders angeordnet),
// steht es halb im Nichts; ist der Bildschirm kleiner als das Fenster, reicht
// es ueber den Rand. Beides hier: liegt weniger als ein Viertel des Fensters
// oder nicht seine Titelleiste auf einem Bildschirm, kommt es ganz auf den
// Bildschirm des Hauptfensters; ist es groesser als sein Bildschirm, wird es
// passend verkleinert.
void einsammeln() {
    ImGuiContext& g = *GImGui;
    if ((g.IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) == 0 || g.PlatformIO.Monitors.empty()) { return; }
    const float rand = ImGui::GetFontSize();
    for (ImGuiWindow* w : g.Windows) {
        if (w->ParentWindow != nullptr || !w->WasActive || w->Hidden || w->DockIsActive || !w->ViewportOwned) { continue; }
        if ((w->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) != 0) { continue; }
        if (g.MovingWindow != nullptr && g.MovingWindow->RootWindowDockTree == w->RootWindowDockTree) { continue; }
        const ImRect r = w->Rect();
        const float flaeche = std::max(1.0F, r.GetWidth() * r.GetHeight());
        int beste = -1;
        float besteFlaeche = 0.0F;
        bool titelDa = false;
        const ImVec2 titel{r.Min.x + std::min(r.GetWidth() * 0.5F, rand * 3.0F), r.Min.y + std::max(1.0F, w->TitleBarHeight * 0.5F)};
        for (int i = 0; i < g.PlatformIO.Monitors.Size; ++i) {
            const ImGuiPlatformMonitor& m = g.PlatformIO.Monitors[i];
            const ImRect mr{m.WorkPos, ImVec2{m.WorkPos.x + m.WorkSize.x, m.WorkPos.y + m.WorkSize.y}};
            ImRect x = r;
            x.ClipWithFull(mr);
            const float f = std::max(0.0F, x.GetWidth()) * std::max(0.0F, x.GetHeight());
            if (f > besteFlaeche) {
                besteFlaeche = f;
                beste = i;
            }
            if (mr.Contains(titel)) { titelDa = true; }
        }
        const bool verloren = beste < 0 || besteFlaeche < flaeche * 0.25F || !titelDa;
        const ImGuiPlatformMonitor& ziel = verloren ? g.PlatformIO.Monitors[std::clamp(static_cast<int>(static_cast<ImGuiViewportP*>(ImGui::GetMainViewport())->PlatformMonitor), 0, g.PlatformIO.Monitors.Size - 1)]
                                                    : g.PlatformIO.Monitors[beste];
        const ImVec2 platz{std::max(rand * 8.0F, ziel.WorkSize.x - rand * 2.0F), std::max(rand * 6.0F, ziel.WorkSize.y - rand * 2.0F)};
        const bool zuGross = r.GetWidth() > platz.x + 0.5F || r.GetHeight() > platz.y + 0.5F;
        if (!verloren && !zuGross) { continue; }
        const ImVec2 gr{std::min(r.GetWidth(), platz.x), std::min(r.GetHeight(), platz.y)};
        const ImVec2 lo{ziel.WorkPos.x + rand, ziel.WorkPos.y + rand};
        const ImVec2 hi{ziel.WorkPos.x + ziel.WorkSize.x - rand - gr.x, ziel.WorkPos.y + ziel.WorkSize.y - rand - gr.y};
        const ImVec2 pos = verloren ? ImVec2{std::floor(ziel.WorkPos.x + (ziel.WorkSize.x - gr.x) * 0.5F),
                                             std::floor(ziel.WorkPos.y + (ziel.WorkSize.y - gr.y) * 0.5F)}
                                    : ImVec2{std::clamp(r.Min.x, lo.x, std::max(lo.x, hi.x)), std::clamp(r.Min.y, lo.y, std::max(lo.y, hi.y))};
        ImGui::SetWindowPos(w, pos, ImGuiCond_Always);
        ImGui::SetWindowSize(w, gr, ImGuiCond_Always);
        if (w->DockNodeAsHost != nullptr) {   // schwebender Dock-Knoten: DockNodeUpdate setzt den Wirt aus Pos/Size des Knotens
            w->DockNodeAsHost->Pos = pos;
            w->DockNodeAsHost->Size = gr;
        }
        char t[256];
        std::snprintf(t, sizeof(t), "Fenster: \"%s\" %s (%.0f,%.0f %.0fx%.0f -> %.0f,%.0f %.0fx%.0f)", w->Name,
                      verloren ? "zurueckgeholt - Bildschirm fehlt" : "verkleinert - Bildschirm zu klein", static_cast<double>(r.Min.x),
                      static_cast<double>(r.Min.y), static_cast<double>(r.GetWidth()), static_cast<double>(r.GetHeight()),
                      static_cast<double>(pos.x), static_cast<double>(pos.y), static_cast<double>(gr.x), static_cast<double>(gr.y));
        diag::detail(t);
        z().merken = true;
    }
}

void speichernFenster() {
    Zustand& s = z();
    if (!s.speichernOffen || g_app == nullptr) { return; }
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Appearing, ImVec2{0.5F, 0.5F});
    const std::string titel = std::string(tr(Str::DockSaveTitle)) + "###docksichern";
    if (ImGui::Begin(titel.c_str(), &s.speichernOffen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking |
                                                           ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::TextUnformatted(tr(Str::DockSaveHint));
        if (ImGui::IsWindowAppearing()) { ImGui::SetKeyboardFocusHere(); }
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 16.0F);
        const bool enter = ImGui::InputText("##dockname", s.name, sizeof(s.name), ImGuiInputTextFlags_EnterReturnsTrue);
        const std::string name = fenster::nameBereinigen(s.name);
        ImGui::BeginDisabled(name.empty());
        if (ImGui::Button(tr(Str::DockSave)) || (enter && !name.empty())) {
            if (speichernUnter(g_app->leftMode, name)) { s.speichernOffen = false; }
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::EditorCancel))) { s.speichernOffen = false; }
    }
    ImGui::End();
}

}  // namespace

const std::vector<Panel>& panels() {
    static const std::vector<Panel> p = {
        {"ereignisse.liste", 0, Str::GroupEvents},
        {"ereignisse.fluss", 0, Str::GroupScriptFlow},
        {"ereignisse.aktionen", 0, Str::GroupActions},
        {"mission.ansicht", 1, Str::DockPanelMapView},
        {"mission.ebenen", 1, Str::EntityLayers},
        {"mission.fluss", 1, Str::GroupScriptFlow},
        {"mission.zeitleiste", 1, Str::DockPanelTimeline},
        // rc609 W2b: ohne Platz im Hauptfenster - offen heisst: in seinem eigenen
        // (andockbaren) Fenster; ab Werk zu (Einstellung entTextOffen).
        {"mission.enttext", 1, Str::EtPanel},
        {"modell.ansicht", 2, Str::DockPanelModelView},
        {"modell.animationen", 2, Str::ModelAnimations},
        {"modell.fluss", 2, Str::GroupScriptFlow},
        {"modell.aktionen", 2, Str::GroupActions},
        {"modell.leiste", 2, Str::DockPanelModelBar},
        {"karte.kamera", 3, Str::KeViewCamera},
        {"karte.xy", 3, Str::KeViewXY},
        {"karte.xz", 3, Str::KeViewXZ},
        {"karte.yz", 3, Str::KeViewYZ},
        {"karte.konsole", 3, Str::KePageConsole},
        {"karte.texturen", 3, Str::KePageTextures},
        {"karte.gruppe", 3, Str::KePageEntities},
        {"menue.bausteine", 4, Str::MePalette},
        {"menue.flaeche", 4, Str::DockPanelCanvas},
        {"menue.reiter", 4, Str::DockPanelMenuSide},
        // dock4: das Layout "Text unten" (nur im DockSpace)
        {"menue.eigenschaften", 4, Str::MeReiterEigenschaften},
        {"menue.text", 4, Str::MeReiterText},
    };
    return p;
}

const Panel* panel(const std::string& id) {
    for (const Panel& p : panels()) {
        if (id == p.id) { return &p; }
    }
    return nullptr;
}

const char* praefix(int bereich) { return (bereich >= 0 && bereich < kBereiche) ? kPraefix[bereich] : ""; }

std::string fensterName(const char* id) {
    const Panel* p = panel(id);
    return p != nullptr ? panelFensterName(*p) : std::string(fenster::kFensterKennung) + id;
}

namespace {
fenster::Anordnung* anordnungVon(const char* id) {
    const Panel* p = panel(id);
    return p == nullptr ? nullptr : &z().jetzt[static_cast<std::size_t>(p->bereich)];
}
bool verfuegbarIst(const Panel& p) {
    const VerfuegbarFn fn = z().verfuegbar[static_cast<std::size_t>(p.bereich)];
    return fn == nullptr || fn(p.id);
}
}  // namespace

namespace {
// dock2: im DockSpace (Schalter an) gelten ImGuis Dock-Baeume statt der
// Anordnungslisten - fuer Events, Mission und Model (app_dockspace.inc), dock4
// auch fuer Map Editor und Menu Editor (dockvorgabe.h).
bool imDockraum(const char* id) {
    const Panel* p = panel(id);
    return p != nullptr && dockraumAn();
}
int bereichVon(const char* id) {
    const Panel* p = panel(id);
    return p != nullptr ? p->bereich : -1;
}
}  // namespace

// rc609 W2b: Panels ohne Platz im Hauptfenster (der Entity-Text). Offen = eigenes Fenster.
bool* zusatzOffen(const char* id) {
    return std::strcmp(id, "mission.enttext") == 0 ? &g_app->settings.entTextOffen : nullptr;
}

bool draussen(const char* id) {
    if (const bool* o = zusatzOffen(id)) { return *o; }
    if (imDockraum(id)) { return bereichVon(id) <= 2 ? dockraumDraussen(id) : dockvorgabe::draussen(bereichVon(id), id); }
    const fenster::Anordnung* a = anordnungVon(id);
    return a != nullptr && fenster::enthaelt(a->draussen, id);
}

bool versteckt(const char* id) {
    if (const bool* o = zusatzOffen(id)) { return !*o; }
    const fenster::Anordnung* a = anordnungVon(id);
    return a != nullptr && fenster::enthaelt(a->versteckt, id);
}

bool zuhause(const char* id) { return !draussen(id) && !versteckt(id); }

bool listeDraussen(int bereich) {
    return bereich >= 0 && bereich < kBereiche && !z().jetzt[static_cast<std::size_t>(bereich)].draussen.empty();
}

bool inListeDraussen(const char* id) {
    const fenster::Anordnung* a = anordnungVon(id);
    return a != nullptr && fenster::enthaelt(a->draussen, id);
}

void listeLeeren(int bereich) {
    if (bereich < 0 || bereich >= kBereiche) { return; }
    z().jetzt[static_cast<std::size_t>(bereich)].draussen.clear();
    z().merken = true;
}

void herausnehmen(const char* id) {
    if (bool* o = zusatzOffen(id)) {
        *o = true;
        return;
    }
    if (imDockraum(id)) {
        if (bereichVon(id) <= 2) {
            dockraumHerausnehmen(id);
        } else {
            dockvorgabe::herausnehmen(bereichVon(id), id);
        }
        return;
    }
    fenster::Anordnung* a = anordnungVon(id);
    if (a == nullptr || fenster::enthaelt(a->draussen, id)) { return; }
    fenster::einfuegen(a->draussen, id);
    fenster::entfernen(a->versteckt, id);
    diag::detail(std::string("Fenster: Panel ") + id + " in eigenem Fenster");
    z().merken = true;
}

void andocken(const char* id) {
    if (bool* o = zusatzOffen(id)) {   // kein Platz im Hauptfenster: zu
        *o = false;
        return;
    }
    if (imDockraum(id)) {
        fenster::Anordnung* v = anordnungVon(id);
        if (v != nullptr) { fenster::entfernen(v->versteckt, id); }
        if (bereichVon(id) <= 2) {
            dockraumAndocken(id);
        } else {
            dockvorgabe::andocken(bereichVon(id), id);
        }
        return;
    }
    fenster::Anordnung* a = anordnungVon(id);
    if (a == nullptr || (!fenster::enthaelt(a->draussen, id) && !fenster::enthaelt(a->versteckt, id))) { return; }
    if (fenster::enthaelt(a->draussen, id)) { heimVormerken(*panel(id)); }
    fenster::entfernen(a->draussen, id);
    fenster::entfernen(a->versteckt, id);
    diag::detail(std::string("Fenster: Panel ") + id + " wieder im Hauptfenster");
    z().merken = true;
}

void ausblenden(const char* id, bool aus) {
    if (bool* o = zusatzOffen(id)) {
        *o = !aus;
        return;
    }
    fenster::Anordnung* a = anordnungVon(id);
    if (a == nullptr || fenster::enthaelt(a->versteckt, id) == aus) { return; }
    if (aus) {
        fenster::einfuegen(a->versteckt, id);
        if (!dockraumAn() && fenster::enthaelt(a->draussen, id)) { heimVormerken(*panel(id)); }
        fenster::entfernen(a->draussen, id);
    } else {
        fenster::entfernen(a->versteckt, id);
    }
    diag::detail(std::string("Fenster: Panel ") + id + (aus ? " ausgeblendet" : " eingeblendet"));
    z().merken = true;
}

void zuruecksetzen(int bereich) {
    if (bereich < 0 || bereich >= kBereiche) { return; }
    fenster::Anordnung& a = z().jetzt[static_cast<std::size_t>(bereich)];
    if (!dockraumAn()) {
        for (const std::string& id : a.draussen) {
            if (const Panel* p = panel(id); p != nullptr) { heimVormerken(*p); }
        }
    }
    a.draussen.clear();
    a.versteckt.clear();
    if (dockraumAn()) {   // dock2/dock4: DockSpace neu aufbauen
        if (bereich <= 2) {
            dockraumZuruecksetzen(bereich);
        } else {
            dockvorgabe::zuruecksetzen(bereich);
        }
    }
    diag::detail("Fenster: Anordnung von Bereich " + std::to_string(bereich) + " zurueckgesetzt");
    z().merken = true;
}

bool speichernUnter(int bereich, const std::string& roh) {
    const std::string name = fenster::nameBereinigen(roh);
    if (g_app == nullptr || name.empty() || bereich < 0 || bereich >= kBereiche) { return false; }
    merkenLaufend();
    g_app->settings.fenster[fenster::schluesselBenannt(bereich, name)] = fenster::kodieren(z().jetzt[static_cast<std::size_t>(bereich)]);
    diag::detail("Fenster: Anordnung \"" + name + "\" gespeichert (Bereich " + std::to_string(bereich) + ")");
    char t[160];
    std::snprintf(t, sizeof(t), tr(Str::DockSaved), name.c_str());
    addStatus(t);
    return true;
}

bool laden(int bereich, const std::string& name) {
    if (g_app == nullptr || bereich < 0 || bereich >= kBereiche) { return false; }
    const auto it = g_app->settings.fenster.find(fenster::schluesselBenannt(bereich, name));
    fenster::Anordnung neu;
    if (it == g_app->settings.fenster.end() || !fenster::dekodieren(it->second, neu)) { return false; }
    std::size_t n = 0;
    const char* text = ImGui::SaveIniSettingsToMemory(&n);
    z().iniLaden = fenster::iniErsetzen(fenster::iniFiltern(std::string(text, n)), neu.ini, kPraefix[bereich]);
    z().ladenBereich = bereich;
    z().ladenAnordnung = neu;
    diag::detail("Fenster: Anordnung \"" + name + "\" geladen (Bereich " + std::to_string(bereich) + ")");
    char t[160];
    std::snprintf(t, sizeof(t), tr(Str::DockLoaded), name.c_str());
    addStatus(t);
    return true;
}

bool loeschen(int bereich, const std::string& name) {
    if (g_app == nullptr) { return false; }
    const bool weg = g_app->settings.fenster.erase(fenster::schluesselBenannt(bereich, name)) != 0U;
    if (weg) { diag::detail("Fenster: Anordnung \"" + name + "\" geloescht"); }
    return weg;
}

std::vector<std::string> gespeicherte(int bereich) {
    return g_app == nullptr ? std::vector<std::string>{} : fenster::benannte(g_app->settings.fenster, bereich);
}

void bildAnfang() {
    if (g_app == nullptr) { return; }
    Zustand& s = z();
    einsammeln();
    if (s.merken || ImGui::GetIO().WantSaveIniSettings) {
        s.merken = false;
        merkenLaufend();
    }
    speichernFenster();
}

void jetztMerken() { merkenLaufend(); }

void vorBild() {
    Zustand& s = z();
    if (g_app == nullptr || ImGui::GetCurrentContext() == nullptr) { return; }
    if (!s.geladen) {   // beim Start, vor dem ersten Bild
        s.geladen = true;
        ausEinstellungenLaden();
    }
    if (s.iniLaden.empty()) { return; }
    // Zwischen zwei Bildern, wie ImGui es fuer LoadIniSettingsFromMemory
    // vorsieht: mitten im Bild geladen zerfiel der Dock-Baum (Selbsttest dock1 -
    // nach dem Laden standen die Panels wieder einzeln). Panels und Fensterlagen
    // gelten ab demselben Bild.
    ImGui::LoadIniSettingsFromMemory(s.iniLaden.c_str(), s.iniLaden.size());
    s.iniLaden.clear();
    if (s.ladenBereich >= 0 && s.ladenBereich < kBereiche) {
        fenster::Anordnung& a = s.jetzt[static_cast<std::size_t>(s.ladenBereich)];
        a.draussen = s.ladenAnordnung.draussen;
        a.versteckt = s.ladenAnordnung.versteckt;
        // dock3: den geladenen Baum des Bereichs neu lesen (Knotennummern
        // koennen neu vergeben sein) - oder aus einer dock1-Anordnung aufbauen.
        if (dockraumAn()) {
            if (s.ladenBereich <= 2) {
                dockraumNeuLesen(s.ladenBereich);
            } else {
                dockvorgabe::neuLesen(s.ladenBereich);
            }
        }
    }
    s.ladenBereich = -1;
    s.merken = true;
}

void setzeVerfuegbar(int bereich, VerfuegbarFn fn) {
    if (bereich >= 0 && bereich < kBereiche) { z().verfuegbar[static_cast<std::size_t>(bereich)] = fn; }
}

void heimPlatz(const char* id, const ImVec2& a, const ImVec2& b) {
    const ImVec2 o = ImGui::GetMainViewport()->Pos;
    z().heim[id] = Heim{ImVec2{a.x - o.x, a.y - o.y}, ImVec2{b.x - o.x, b.y - o.y}};
}

namespace {

// Wohin das Panel beim Zurueckziehen muss: sein letzter Platz im Hauptfenster,
// sonst die Mitte des Hauptfensters.
ImRect zielRechteck(const char* id) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const auto it = z().heim.find(id);
    if (it != z().heim.end() && it->second.b.x - it->second.a.x > 8.0F && it->second.b.y - it->second.a.y > 8.0F) {
        return ImRect{ImVec2{vp->Pos.x + it->second.a.x, vp->Pos.y + it->second.a.y},
                      ImVec2{vp->Pos.x + it->second.b.x, vp->Pos.y + it->second.b.y}};
    }
    const ImVec2 m = vp->GetWorkCenter();
    return ImRect{ImVec2{m.x - vp->WorkSize.x * 0.2F, m.y - vp->WorkSize.y * 0.2F}, ImVec2{m.x + vp->WorkSize.x * 0.2F, m.y + vp->WorkSize.y * 0.2F}};
}

}  // namespace

void zielPlatz(const char* id, ImVec2& a, ImVec2& b) {
    const ImRect r = zielRechteck(id);
    a = r.Min;
    b = r.Max;
}

void startLage(const char* id) {
    // Ganz im Hauptfenster: ein Fenster, das darueber hinausragt, wird ein eigenes
    // Windows-Fenster und laege ueber den Menues des Hauptfensters (Selbsttest
    // dock1: der Klick auf "Reset layout" traf die Ereignisliste).
    const ImRect r = zielRechteck(id);
    const float fs = ImGui::GetFontSize();
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 lo{vp->WorkPos.x + fs, vp->WorkPos.y + fs};
    const ImVec2 hi{vp->WorkPos.x + vp->WorkSize.x - fs, vp->WorkPos.y + vp->WorkSize.y - fs};
    const ImVec2 gr{std::min(std::max(r.GetWidth(), fs * 16.0F), std::max(fs * 8.0F, hi.x - lo.x)),
                    std::min(std::max(r.GetHeight(), fs * 10.0F), std::max(fs * 6.0F, hi.y - lo.y))};
    const ImVec2 pos{std::clamp(r.Min.x + fs * 2.0F, lo.x, std::max(lo.x, hi.x - gr.x)),
                     std::clamp(r.Min.y + fs * 2.0F, lo.y, std::max(lo.y, hi.y - gr.y))};
    ImGui::SetNextWindowPos(pos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(gr, ImGuiCond_FirstUseEver);
}

bool fensterBeginnen(const char* id) {
    Zustand& s = z();
    const Panel* p = panel(id);
    if (p == nullptr) { return false; }
    s.offenesFenster = id;
    s.fensterGeschlossen = false;
    // Nur mit anderen Panels zusammensetzen, nicht mit Dialogen (Suchen,
    // Einstellungen): die haben ihre eigene Lage und eigene Schliessregeln.
    ImGuiWindowClass klasse;
    klasse.ClassId = ImHashStr("bhed.dock.panel");
    klasse.DockingAllowUnclassed = false;
    ImGui::SetNextWindowClass(&klasse);
    // Startlage: wo das Panel im Hauptfenster stand, etwas versetzt - man
    // sieht, dass es jetzt ein eigenes Fenster ist.
    // Nur fuer ein Fenster, das es noch nicht gibt: SetNextWindowPos loest ein
    // angedocktes Fenster aus seinem Dock-Knoten (BeginDocked, PosUndock) -
    // auch mit ImGuiCond_FirstUseEver, solange ImGui die Bedingung fuer das
    // Fenster noch zulaesst (Selbsttest dock1: geladene Anordnung zerfiel).
    const std::string name = panelFensterName(*p);
    // Die Marke aus heimVormerken (oder ein Knoten, den es nicht mehr gibt): ohne
    // DockSpace schwebt das Panel - ImGui legte sonst einen Knoten mit dieser Nummer an.
    if (ImGuiWindow* w = ImGui::FindWindowByName(name.c_str()); w != nullptr) {
        if (w->DockNode == nullptr && w->DockId != 0 && ImGui::DockBuilderGetNode(w->DockId) == nullptr) { w->DockId = 0; }
    } else {
        if (ImGuiWindowSettings* st = ImGui::FindWindowSettingsByID(ImHashStr(name.c_str()));
            st != nullptr && st->DockId != 0 && ImGui::DockBuilderGetNode(st->DockId) == nullptr) {
            st->DockId = 0;
        }
        if (ImGui::FindWindowSettingsByID(ImHashStr(name.c_str())) == nullptr) { startLage(id); }
    }
    bool offen = true;
    const bool sichtbar = ImGui::Begin(name.c_str(), &offen, ImGuiWindowFlags_NoCollapse);
    s.fensterGeschlossen = !offen;
    return sichtbar;
}

void fensterEnde() {
    Zustand& s = z();
    ImGuiContext& g = *GImGui;
    ImGuiWindow* w = g.CurrentWindow;
    const std::string id = s.offenesFenster;
    ImGui::End();
    s.offenesFenster.clear();
    if (s.fensterGeschlossen) {   // Schliessknopf: zurueck an seinen Platz
        andocken(id.c_str());
        return;
    }
    if (zusatzOffen(id.c_str()) != nullptr) { return; }   // kein alter Platz zum Zurueckziehen
    // --- Zurueckziehen auf den alten Platz ------------------------------------
    // Waehrend das Fenster gezogen wird, zeigt das Hauptfenster seinen alten
    // Platz; wird es dort losgelassen, kommt es zurueck. Nicht, wenn darunter
    // ein anderes Panelfenster liegt - dort setzt ImGui die beiden zusammen.
    const bool gezogen = g.MovingWindow != nullptr && w != nullptr && g.MovingWindow->RootWindowDockTree == w->RootWindowDockTree;
    if (gezogen) {
        const ImRect ziel = zielRechteck(id.c_str());
        const ImGuiWindow* unter = g.HoveredWindowUnderMovingWindow;
        const bool ueberPanel = unter != nullptr && (istPanelFenster(unter->RootWindow) || unter->RootWindow->DockNodeAsHost != nullptr);
        const bool drin = !ueberPanel && ziel.Contains(g.IO.MousePos);
        ImDrawList* dl = ImGui::GetForegroundDrawList(ImGui::GetMainViewport());
        const ImVec4 farbe = ImGui::GetStyleColorVec4(ImGuiCol_DragDropTarget);
        dl->AddRectFilled(ziel.Min, ziel.Max, ImGui::GetColorU32(ImVec4{farbe.x, farbe.y, farbe.z, drin ? 0.35F : 0.12F}));
        dl->AddRect(ziel.Min, ziel.Max, ImGui::GetColorU32(farbe), 0.0F, 0, drin ? 3.0F : 1.0F);
        const char* text = tr(Str::DockDropHere);
        const ImVec2 tg = ImGui::CalcTextSize(text);
        // Oben im Platz, nicht in der Mitte: dort haengt das gezogene Fenster mit
        // seiner Titelleiste unter der Maus.
        const ImVec2 tp{ziel.GetCenter().x - tg.x * 0.5F, ziel.Min.y + tg.y};
        const float rand = tg.y * 0.3F;
        dl->AddRectFilled(ImVec2{tp.x - rand, tp.y - rand}, ImVec2{tp.x + tg.x + rand, tp.y + tg.y + rand}, ImGui::GetColorU32(ImGuiCol_PopupBg),
                          rand);
        dl->AddText(tp, ImGui::GetColorU32(ImGuiCol_Text), text);
        s.ueberPlatz = drin ? id : std::string();
    } else if (s.ueberPlatz == id) {
        s.ueberPlatz.clear();
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) { andocken(id.c_str()); }
    }
}

void menueInhalt(int bereich) {
    if (bereich < 0 || bereich >= kBereiche) { return; }
    ImGui::TextDisabled("%s", tr(Str::DockPanels));
    for (const Panel& p : panels()) {
        if (p.bereich != bereich) { continue; }
        const bool da = verfuegbarIst(p);
        const std::string l = std::string(tr(p.titel)) + "##dockzeigen." + p.id;
        if (ImGui::MenuItem(l.c_str(), nullptr, !versteckt(p.id), da)) { ausblenden(p.id, !versteckt(p.id)); }
    }
    ImGui::Separator();
    ImGui::TextDisabled("%s", tr(Str::DockOwnWindow));
    for (const Panel& p : panels()) {
        if (p.bereich != bereich || zusatzOffen(p.id) != nullptr) { continue; }   // W2b: steht immer im eigenen Fenster
        const bool da = verfuegbarIst(p);
        const std::string l = std::string(tr(p.titel)) + "##dockfenster." + p.id;
        if (ImGui::MenuItem(l.c_str(), nullptr, draussen(p.id), da)) {
            if (draussen(p.id)) {
                andocken(p.id);
            } else {
                herausnehmen(p.id);
            }
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("%s", tr(da ? Str::DockOwnWindowHint : Str::DockNotInLayout));
        }
    }
    ImGui::Separator();
    if (ImGui::MenuItem(tr(Str::DockSaveAs))) {
        z().speichernOffen = true;
        z().name[0] = '\0';
    }
    const std::vector<std::string> namen = gespeicherte(bereich);
    if (ImGui::BeginMenu(tr(Str::DockLoad), !namen.empty())) {
        for (const std::string& n : namen) {
            if (ImGui::MenuItem((n + "##dockladen").c_str())) { (void)laden(bereich, n); }
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(tr(Str::DockDelete), !namen.empty())) {
        for (const std::string& n : namen) {
            if (ImGui::MenuItem((n + "##dockloeschen").c_str())) { (void)loeschen(bereich, n); }
        }
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem(tr(Str::DockReset))) {
        zuruecksetzen(bereich);
        addStatus(tr(Str::DockResetDone));
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::DockResetHint)); }
    ImGui::Separator();
    ImGui::TextDisabled("%s", tr(Str::DockOther));
    if (ImGui::MenuItem(tr(Str::HlfMenu), nullptr, hilfeOffen())) {
        if (hilfeOffen()) {
            hilfeSchliessen();
        } else {
            hilfeOeffnen();
        }
    }
    if (ImGui::MenuItem(tr(Str::TourMenu))) { tourFensterOeffnen(); }
    if (ImGui::MenuItem(tr(Str::MessagesTitle), nullptr, g_app->messagesOpen)) { g_app->messagesOpen = !g_app->messagesOpen; }
}

void eigenesFensterWennVerdeckt() {
    ImGuiContext& g = *GImGui;
    if ((g.IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) == 0 || ImGui::GetWindowViewport() != ImGui::GetMainViewport()) { return; }
    const ImGuiViewport* haupt = ImGui::GetMainViewport();
    const ImRect flaeche{haupt->Pos, ImVec2{haupt->Pos.x + haupt->Size.x, haupt->Pos.y + haupt->Size.y}};
    for (const ImGuiViewportP* v : g.Viewports) {
        if (v == haupt || !v->PlatformWindowCreated || v->Window == nullptr ||
            (v->Window->Flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_ChildMenu)) != 0) {
            continue;
        }
        if (ImRect{v->Pos, ImVec2{v->Pos.x + v->Size.x, v->Pos.y + v->Size.y}}.Overlaps(flaeche)) {
            // Eine Kennung ohne Fenster: ImGui findet keins, prueft die Ueberdeckung (UpdateTryMergeWindowIntoHostViewport)
            // und legt das Menue nur dann in ein eigenes Fenster, wenn ein eigenes Fenster ueber ihm liegt.
            ImGui::SetNextWindowViewport(ImHashStr("bhed.menue.eigenesfenster"));
            return;
        }
    }
}

void menue(int bereich) {
    eigenesFensterWennVerdeckt();
    if (ImGui::BeginMenu(tr(Str::DockMenu))) {
        menueInhalt(bereich);
        ImGui::EndMenu();
    }
}

}  // namespace bhed::gui::dock
