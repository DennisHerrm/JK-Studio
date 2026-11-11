// ui_formular.cpp - der Formular-Baustein und die Mindestgroessen der Fenster (UIFORM, rc612; Beschreibung in ui_formular.h)
#include "ui_formular.h"

#include "app_internal.h"
#include "menueeditor.h"
#include "textumbruch.h"

#include "bhed/diag.h"
#include "bhed/i18n.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>

namespace bhed::gui::ui {

namespace {

// So viele Zeichenhoehen behaelt ein Eingabefeld, das mit dem Fenster schrumpft (Bedienreihen).
constexpr float kFeldGequetscht = 6.0F;
// Nach so vielen Anhebungen in Folge, nach denen wieder etwas hinausragt, waechst der Bedarf mit dem Fenster mit.
constexpr int kMitgewachsen = 3;

struct Eintrag {
    std::string name;
    ImGuiWindowFlags flags = 0;  // die Fahnen der Stelle
    bool schliessen = false;     // mit Schliessknopf (p_open)
    bool frei = false;           // das Programm bestimmt die Groesse: keine Mindestgroesse
    int begonnen = -1;           // Bild des letzten Begin
    int vorher = -1;             // Bild des Begin davor
    ImVec2 inhalt{0.0F, 0.0F};   // vom Inhalt gemeldet (Fenstergroesse)
    int inhaltBild = -1;
    ImVec2 untergrenze{0.0F, 0.0F};   // die Untergrenze des letzten Begin (mit der Grenze der Stelle; 0: keine)
    ImVec2 eigene{0.0F, 0.0F};   // die Untergrenze, die die Stelle selbst gesetzt hat (0: keine)
    ImVec2 davor{0.0F, 0.0F};    // Selbsttest: Groesse vor dem letzten Begin (Zeile "UIF hebt")
    float reihen = 0.0F;         // Fensterbreite, die die Bedienreihen brauchen (angehoben)
    ImVec2 groesste{0.0F, 0.0F};   // groesste Groesse des Fensters in dieser Sitzung - darueber hebt die Messung nie an
    // Messung der Bedienreihen (Haken) in einem Bild
    int messBild = -1;
    bool imInhalt = false;       // Begin ist durch: was jetzt kommt, ist Inhalt (nicht Titelzeile, Rollbalken, Ziehgriffe)
    float bedarf = 0.0F;         // rechte Kante der Bedienreihen ab der linken Fensterkante
    float messBreite = -1.0F;    // Fensterbreite und Inhaltsbreite, fuer die zuletzt gemessen wurde
    float messInhalt = -1.0F;
    bool zeileLeer = true;       // die laufende Zeile: senkrechter Bereich und rechte Kante ihres ersten Elements
    float zeileOben = 0.0F;
    float zeileUnten = 0.0F;
    float zeileErstes = 0.0F;
    int nach = 0;                // Anhebungen in Folge, nach denen wieder etwas hinausragte
    bool waechstMit = false;     // Bedarf waechst mit dem Fenster: nicht mehr anheben
};

struct Zustand {
    std::unordered_map<ImGuiID, Eintrag> fenster;
    float schrift = 0.0F;
    int sprache = -1;
    bool an = true;
    bool misst = false;                  // in diesem Bild misst mindestens ein Fenster seine Bedienreihen
    const ImGuiWindow* hakenFenster = nullptr;   // das Fenster des letzten Hakens und sein Eintrag (nullptr: misst nicht)
    Eintrag* hakenEintrag = nullptr;
    int knotenGehoben = 0;               // Selbsttest: Anhebungen von Fenstern, deren Groesse ein Dock-Knoten bestimmt
    std::string knotenListe;
};

Zustand& z() {
    static Zustand s;
    return s;
}

ImVec2 groesser(const ImVec2& a, const ImVec2& b) { return ImVec2{std::max(a.x, b.x), std::max(a.y, b.y)}; }

// Die Titelzeile: Text (ein langer Titel endet wie bei Windows mit "...", daher hoechstens 12 Zeichenhoehen davon), Einklapp- und
// Schliessknopf und Polster wie ImGui sie legt (imgui.cpp RenderWindowTitleBarContents: pad_l/pad_r = FramePadding.x, je Knopf
// FontSize + ItemInnerSpacing.x), dazu eine Zeile Inhalt.
ImVec2 titelMindest(const Eintrag& e) {
    const ImGuiStyle& st = ImGui::GetStyle();
    const float fs = ImGui::GetFontSize();
    ImVec2 m{fs * 10.0F, ImGui::GetFrameHeight() + st.WindowPadding.y * 2.0F};
    if ((e.flags & ImGuiWindowFlags_NoTitleBar) == 0) {
        const char* text = e.name.c_str();
        const char* ende = ImGui::FindRenderedTextEnd(text);
        float w = std::min(ImGui::CalcTextSize(text, ende).x, fs * 12.0F) + st.FramePadding.x * 2.0F + st.WindowBorderSize * 2.0F;
        if ((e.flags & ImGuiWindowFlags_NoCollapse) == 0) { w += fs + st.ItemInnerSpacing.x; }
        if (e.schliessen) { w += fs + st.ItemInnerSpacing.x; }
        m.x = std::max(m.x, w);
        m.y += fs + st.FramePadding.y * 2.0F;
    }
    if ((e.flags & ImGuiWindowFlags_MenuBar) != 0) { m.y += fs + st.FramePadding.y * 2.0F; }
    return m;
}

ImVec2 deckel() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    return ImVec2{std::floor(vp->WorkSize.x * 0.9F), std::floor(vp->WorkSize.y * 0.9F)};
}

// Was Inhalt und Bedienreihen verlangen (ohne die Titelzeile); letztesBild: das Bild, in dem der Inhalt sich gemeldet haben muss.
ImVec2 bedarfAus(const Eintrag& e, int letztesBild) {
    ImVec2 m{e.reihen, 0.0F};
    if (e.inhaltBild >= 0 && e.inhaltBild == letztesBild) { m = groesser(m, e.inhalt); }
    return m;
}

ImVec2 mindestAus(const Eintrag& e, int letztesBild) {
    const ImVec2 m = groesser(titelMindest(e), bedarfAus(e, letztesBild));
    const ImVec2 d = deckel();
    return ImVec2{std::floor(std::min(m.x, d.x)), std::floor(std::min(m.y, d.y))};
}

bool imSelbsttest() {
    static const bool an = std::getenv("BHED_EDITORTEST") != nullptr;
    return an;
}

// Legt ein Dock-Knoten das Fenster an (dock: sein Knoten bzw. der, in den es mit diesem Begin kommt)? DockIsActive allein reicht
// vor dem Begin nicht: die Bereiche setzen ihre Panels ueber den DockBuilder in die Knoten (ImGui DockNodeAddWindow:
// DockIsActive = Windows.Size > 1), erst BeginDocked setzt es wieder - deshalb auch der Knoten mit Wirt. Im DockSpace bestimmt die
// Anordnung die Groesse, auch in dem Bild, in dem der Knoten noch keinen Wirt hat: die Bereiche bauen ihre Anordnung beim Ein- und
// Ausklappen eines Felds mit dem DockBuilder neu und setzen die Panels per SetNextWindowDockID hinein - ImGui BeginDocked laesst
// das Fenster dann ein Bild lang ungedockt (Fast path "node->HostWindow == NULL"), erst das naechste Bild nimmt die Knotengroesse.
// Ein schwebender Knoten mit einem einzigen Fenster hat keinen Wirt: dort bestimmt das Fenster selbst.
bool knotenLegt(const ImGuiWindow* w, ImGuiID dock) {
    if (w != nullptr && w->DockIsActive) { return true; }
    if (dock == 0) { return false; }
    ImGuiDockNode* n = ImGui::DockBuilderGetNode(dock);
    return n != nullptr && (n->HostWindow != nullptr || ImGui::DockNodeGetRootNode(n)->IsDockSpace());
}

ImGuiWindowFlags vorBegin(const char* name, ImGuiWindowFlags flags, bool schliessen) {
    Zustand& s = z();
    if (!s.an) { return flags; }
    ImGuiContext& g = *GImGui;
    const ImGuiID id = ImHashStr(name);
    Eintrag& e = s.fenster[id];
    if (e.name != name) { e.name = name; }
    const ImGuiWindowFlags stelle = flags;
    e.flags = stelle;
    e.schliessen = schliessen;
    if (e.begonnen != g.FrameCount) {
        e.vorher = e.begonnen;
        e.begonnen = g.FrameCount;
    }
    // Das Programm bestimmt die Groesse (passt sich dem Inhalt an, laesst sich nicht ziehen, setzt sie in jedem Bild): die
    // Stelle bleibt, wie sie ist.
    const bool gesetzt = (g.NextWindowData.HasFlags & ImGuiNextWindowDataFlags_HasSize) != 0 && g.NextWindowData.SizeCond == ImGuiCond_Always;
    e.frei = (stelle & (ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) != 0 || gesetzt;
    if (e.frei) {
        e.untergrenze = ImVec2{0.0F, 0.0F};
        return flags;
    }
    const ImGuiWindow* w = ImGui::FindWindowByID(id);
    // Legt ein Dock-Knoten das Fenster an, bestimmt die Anordnung seine Groesse (Nutzer oder Bereich): keine Untergrenze, auch
    // nicht in dem Bild, in dem es in einen neu gebauten Knoten kommt (sonst wuerde ein schmal oder eingeklappt gestelltes Panel
    // dort ein Bild lang aufgeblaeht). Ist der Knoten schmaler als Inhalt oder Bedienreihen, rollt ein Fenster mit Fliesslayout
    // waagrecht, statt zu zerquetschen.
    const bool dockNeu = (g.NextWindowData.HasFlags & ImGuiNextWindowDataFlags_HasDock) != 0 &&
                         (w == nullptr || (w->SetWindowDockAllowFlags & g.NextWindowData.DockCond) != 0);
    if (knotenLegt(w, dockNeu ? g.NextWindowData.DockId : (w != nullptr ? w->DockId : 0))) {
        e.untergrenze = ImVec2{0.0F, 0.0F};
        e.eigene = ImVec2{0.0F, 0.0F};
        if ((stelle & ImGuiWindowFlags_NoScrollbar) == 0 && w != nullptr && w->Size.x + 0.5F < std::min(bedarfAus(e, e.vorher).x, deckel().x)) {
            flags |= ImGuiWindowFlags_HorizontalScrollbar;
        }
        return flags;
    }
    const ImVec2 m = mindestAus(e, e.vorher);
    ImVec2 lo = m;
    ImVec2 hi{FLT_MAX, FLT_MAX};
    ImGuiSizeCallback cb = nullptr;
    void* daten = nullptr;
    e.eigene = ImVec2{0.0F, 0.0F};
    if ((g.NextWindowData.HasFlags & ImGuiNextWindowDataFlags_HasSizeConstraint) != 0) {
        // Grenzen der Stelle selbst bleiben; nur die Untergrenze waechst (eine negative Achse heisst "Groesse behalten").
        const ImRect cr = g.NextWindowData.SizeConstraintRect;
        e.eigene = ImVec2{std::max(cr.Min.x, 0.0F), std::max(cr.Min.y, 0.0F)};
        lo.x = cr.Min.x < 0.0F ? cr.Min.x : std::max(cr.Min.x, m.x);
        lo.y = cr.Min.y < 0.0F ? cr.Min.y : std::max(cr.Min.y, m.y);
        hi.x = cr.Max.x < 0.0F ? cr.Max.x : std::max(cr.Max.x, lo.x);
        hi.y = cr.Max.y < 0.0F ? cr.Max.y : std::max(cr.Max.y, lo.y);
        cb = g.NextWindowData.SizeCallback;
        daten = g.NextWindowData.SizeCallbackUserData;
    }
    e.untergrenze = ImVec2{std::max(lo.x, 0.0F), std::max(lo.y, 0.0F)};
    if (imSelbsttest()) {
        // fuer die Zeile "UIF hebt" (nachBegin): die Groesse vor dem Begin - bei einem neuen Fenster die der Stelle
        const bool neu = w == nullptr && (g.NextWindowData.HasFlags & ImGuiNextWindowDataFlags_HasSize) != 0;
        e.davor = w != nullptr ? w->SizeFull : (neu ? g.NextWindowData.SizeVal : ImVec2{0.0F, 0.0F});
    }
    ImGui::SetNextWindowSizeConstraints(lo, hi, cb, daten);
    return flags;
}

// Nach dem Begin. Misst das Fenster in diesem Bild seine Bedienreihen, zaehlt erst, was jetzt kommt - Titelzeile, Rollbalken und
// Ziehgriffe hat Begin schon gemeldet. Im Selbsttest steht jede Anhebung im Protokoll (das Begin hat das Fenster auf die
// Untergrenze vergroessert): die vollen Laeufe zeigen damit, welches Fenster die Mindestgroesse groesser macht - erwartet nur
// dort, wo ein Test ein Fenster zusammenschiebt.
void nachBegin(const char* name) {
    Zustand& s = z();
    if (!s.misst && !imSelbsttest()) { return; }
    const ImGuiID id = ImHashStr(name);
    const auto it = s.fenster.find(id);
    if (it == s.fenster.end()) { return; }
    Eintrag& e = it->second;
    if (e.messBild == GImGui->FrameCount) { e.imInhalt = true; }
    if (!imSelbsttest() || e.frei) { return; }
    const ImGuiWindow* w = ImGui::FindWindowByID(id);
    if (w == nullptr || !w->Active) { return; }
    // angehoben: vorher kleiner als die Untergrenze, jetzt genau auf ihr (ein Knoten, der sein Panel aus anderem Grund
    // vergroessert, landet woanders) - und die Mindestgroesse war es, nicht die Grenze, die die Stelle selbst setzt (die hob
    // schon vor UIFORM an)
    const ImVec2 n = w->SizeFull;
    const bool breiter = e.davor.x > 0.0F && e.untergrenze.x > e.davor.x + 0.5F && e.untergrenze.x > e.eigene.x + 0.5F &&
                         std::fabs(n.x - e.untergrenze.x) <= 1.0F;
    const bool hoeher = e.davor.y > 0.0F && e.untergrenze.y > e.davor.y + 0.5F && e.untergrenze.y > e.eigene.y + 0.5F &&
                        std::fabs(n.y - e.untergrenze.y) <= 1.0F;
    if (!breiter && !hoeher) { return; }
    const ImVec2 t = titelMindest(e);
    const ImVec2 b = bedarfAus(e, e.vorher);
    const bool knoten = knotenLegt(w, w->DockId);
    char zeile[384];
    std::snprintf(zeile, sizeof zeile, "Knopftest: UIF hebt %.80s von %.0fx%.0f auf %.0fx%.0f (Titel %.0fx%.0f, Inhalt %.0fx%.0f, Reihen %.0f%s)", name,
                  static_cast<double>(e.davor.x), static_cast<double>(e.davor.y), static_cast<double>(n.x), static_cast<double>(n.y),
                  static_cast<double>(t.x), static_cast<double>(t.y), static_cast<double>(b.x), static_cast<double>(b.y), static_cast<double>(e.reihen),
                  knoten ? ", im Dock-Knoten" : "");
    diag::detail(zeile);
    if (knoten) {
        ++s.knotenGehoben;
        if (s.knotenListe.size() < 600) { s.knotenListe += " [" + std::string(name).substr(0, 80) + "]"; }
    }
}

}  // namespace

bool fensterBegin(const char* name, bool* offen, ImGuiWindowFlags flags) {
    const bool sichtbar = ImGui::Begin(name, offen, vorBegin(name, flags, offen != nullptr));
    nachBegin(name);
    return sichtbar;
}

bool modalBegin(const char* name, bool* offen, ImGuiWindowFlags flags) {
    // BeginPopupModal setzt NoCollapse selbst (imgui.cpp): die Titelzeile hat keinen Einklappknopf.
    const ImGuiWindowFlags mit = vorBegin(name, flags | ImGuiWindowFlags_NoCollapse, offen != nullptr);
    const bool sichtbar = ImGui::BeginPopupModal(name, offen, mit);
    nachBegin(name);
    return sichtbar;
}

void mindestGroesse(const ImVec2& fenster) {
    ImGuiContext& g = *GImGui;
    if (g.CurrentWindow == nullptr) { return; }
    const ImGuiWindow* w = g.CurrentWindow->RootWindow;
    Eintrag& e = z().fenster[w->ID];
    if (e.inhaltBild != g.FrameCount) {
        e.inhalt = fenster;
        e.inhaltBild = g.FrameCount;
    } else {
        e.inhalt = groesser(e.inhalt, fenster);
    }
}

void mindestInhalt(const ImVec2& inhalt) {
    ImGuiContext& g = *GImGui;
    if (g.CurrentWindow == nullptr) { return; }
    const ImGuiWindow* w = g.CurrentWindow->RootWindow;
    // Wie ImGui die Groesse aus dem Inhalt rechnet (imgui.cpp CalcWindowAutoFitSize: Inhalt + Polster + Titel- und Menuezeile; der
    // Rand liegt im Polster; Fenstergroessen sind ganze Bildpunkte, abgeschnitten). Platz fuer den senkrechten Rollbalken immer
    // mitrechnen: sonst springt die Breite, wenn er erscheint.
    mindestGroesse(ImVec2{std::floor(inhalt.x + w->WindowPadding.x * 2.0F + g.Style.ScrollbarSize),
                          std::floor(inhalt.y + w->WindowPadding.y * 2.0F + w->TitleBarHeight + w->MenuBarHeight)});
}

void bildAnfang() {
    ImGuiContext& g = *GImGui;
    Zustand& s = z();
    s.misst = false;
    s.hakenFenster = nullptr;
    s.hakenEintrag = nullptr;
    if (!s.an) { return; }
    const float fs = ImGui::GetFontSize();
    const int sprache = static_cast<int>(i18n::currentLanguage());
    if (std::fabs(fs - s.schrift) > 0.01F || sprache != s.sprache) {
        // Neue Schrift oder Sprache: alles neu messen (die groesste Groesse bleibt - Fenster wachsen nicht mit der Schrift).
        for (auto& paar : s.fenster) {
            Eintrag& e = paar.second;
            e.reihen = 0.0F;
            e.inhaltBild = -1;
            e.messBild = -1;
            e.messBreite = -1.0F;
            e.messInhalt = -1.0F;
            e.nach = 0;
            e.waechstMit = false;
        }
        s.schrift = fs;
        s.sprache = sprache;
        return;
    }
    const int letztes = g.FrameCount - 1;
    const float hoechstens = deckel().x;
    for (auto& [id, e] : s.fenster) {
        if (e.begonnen != letztes || e.frei) { continue; }
        const ImGuiWindow* w = ImGui::FindWindowByID(id);
        if (w == nullptr || w->LastFrameActive != letztes || w->Collapsed || w->SkipItems || w->Hidden) { continue; }
        e.groesste = groesser(e.groesste, w->SizeFull);
        // Bedienreihen nur im Fliesslayout: Fenster ohne Rollbalken teilen ihren Platz selbst auf (Panels, Zeichenflaechen mit
        // frei gesetzten Elementen), mit waagrechtem Rollbalken ist alles erreichbar.
        if ((e.flags & (ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_HorizontalScrollbar)) != 0 || e.waechstMit) { continue; }
        // Die Werte des letzten Bildes (DC wird erst im naechsten Begin des Fensters zurueckgesetzt).
        const float inhaltX = w->DC.CursorMaxPos.x - w->DC.CursorStartPos.x;
        if (e.messBild == letztes) {
            e.messBreite = w->Size.x;
            e.messInhalt = inhaltX;
            // bis zur rechten Kante des Arbeitsbereichs (imgui.cpp Begin: WorkRect = Groesse - Polster - Rollbalken)
            const float noetig = std::ceil(e.bedarf + w->WindowPadding.x + (w->ScrollbarY ? g.Style.ScrollbarSize : 0.0F));
            if (e.bedarf <= 0.0F || noetig <= w->Size.x + 1.0F) {
                e.nach = 0;   // alle Bedienreihen passen
            } else if (noetig <= e.groesste.x + 0.5F) {
                // Eine Bedienreihe ragt hinaus und hat in dieses Fenster schon gepasst. Stand es schon auf dem angehobenen Mass,
                // waechst der Bedarf vielleicht mit dem Fenster mit: nach einigen Malen in Folge nicht mehr anheben.
                const bool angehoben = !knotenLegt(w, w->DockId) && e.reihen > 0.0F && w->Size.x + 1.0F >= e.reihen;
                if (angehoben && ++e.nach >= kMitgewachsen) {
                    e.waechstMit = true;
                    e.reihen = 0.0F;
                } else {
                    e.reihen = std::max(e.reihen, std::min(noetig, hoechstens));
                }
            }
        } else if (inhaltX > w->WorkRect.GetWidth() + 1.0F &&
                   (std::fabs(w->Size.x - e.messBreite) > 0.5F || std::fabs(inhaltX - e.messInhalt) > 0.5F)) {
            // Etwas ragt rechts hinaus und ist in diesem Zustand noch nicht gemessen: in diesem Bild die Bedienreihen messen.
            e.messBild = g.FrameCount;
            e.imInhalt = false;
            e.bedarf = 0.0F;
            e.zeileLeer = true;
            s.misst = true;
        }
    }
    // Der Haken des Element-Registers laeuft fuer dieses Bild (elemente::bildEnde stellt ihn danach wieder nach seiner Stufe).
    if (s.misst) { g.TestEngineHookItems = true; }
}

// ImGui meldet nur Elemente mit Kennung (imgui.cpp ItemAdd: IMGUI_TEST_ENGINE_ITEM_ADD nur bei id != 0) und gibt als Rechteck
// den Rahmen (NavRect - ohne die Beschriftung rechts vom Feld). Text, Bilder und Gruppen kommen hier nicht an.
void hakenElement(ImGuiContext* ctx, ImGuiID /*id*/, const ImRect& bb) {
    Zustand& s = z();
    if (!s.misst) { return; }
    const ImGuiWindow* w = ctx->CurrentWindow;
    if (w == nullptr) { return; }
    if (w != s.hakenFenster) {
        s.hakenFenster = w;
        s.hakenEintrag = nullptr;
        if ((w->Flags & ImGuiWindowFlags_ChildWindow) == 0) {
            if (const auto it = s.fenster.find(w->ID); it != s.fenster.end() && it->second.messBild == ctx->FrameCount) { s.hakenEintrag = &it->second; }
        }
    }
    Eintrag* e = s.hakenEintrag;
    if (e == nullptr || !e->imInhalt) { return; }
    // Tabellen und Spalten teilen ihre Breite selbst auf (Zellen schneiden ab), die Menueleiste zaehlt nicht zum Inhalt; Elemente
    // ohne Flaeche zaehlen nicht.
    if (ctx->CurrentTable != nullptr || w->DC.CurrentColumns != nullptr || w->DC.MenuBarAppending || bb.GetWidth() <= 0.0F || bb.GetHeight() <= 0.0F) {
        return;
    }
    // Ein Element gehoert zu einer Bedienreihe und muss ganz hineinpassen, wenn es
    //   - rechts neben dem ersten Element seiner Zeile steht (senkrecht ueberlappend): Knopfreihe, Werkzeugleiste, oder
    //   - ein gerahmtes Bedienelement ist (so hoch wie ein Rahmen: Knopf, Feld, Ausklapper, Haken), das nicht am Zeilenanfang
    //     beginnt: hinter einer Beschriftung, eingerueckt gesetzt oder rechtsbuendig. Listeneintraege und Baumknoten sind
    //     niedriger (Zeichenhoehe + Zeilenabstand) und beginnen am Zeilenanfang - sie duerfen am Rand enden.
    // Sonst beginnt es eine neue Zeile und verlangt nichts.
    const ImGuiStyle& st = ctx->Style;
    const bool daneben = !e->zeileLeer && bb.Min.y < e->zeileUnten - 1.0F && bb.Max.y > e->zeileOben + 1.0F && bb.Min.x >= e->zeileErstes - 1.0F;
    if (daneben) {
        e->zeileOben = std::min(e->zeileOben, bb.Min.y);
        e->zeileUnten = std::max(e->zeileUnten, bb.Max.y);
    } else {
        e->zeileLeer = false;
        e->zeileOben = bb.Min.y;
        e->zeileUnten = bb.Max.y;
        e->zeileErstes = bb.Max.x;
    }
    // Zeilenanfang wie ImGui ihn setzt (imgui.cpp ItemSize: Pos.x + DC.Indent.x + DC.ColumnsOffset.x; Indent enthaelt das Rollen).
    const float anfang = w->Pos.x + w->DC.Indent.x + w->DC.ColumnsOffset.x;
    const float rahmen = ctx->FontSize + st.FramePadding.y * 2.0F;
    const bool gerahmt = st.ItemSpacing.y < st.FramePadding.y * 2.0F - 0.5F && bb.GetHeight() >= rahmen - 0.5F;
    if (daneben || (gerahmt && bb.Min.x > anfang + 1.0F)) { e->bedarf = std::max(e->bedarf, bb.Max.x - w->Pos.x + w->Scroll.x); }
    // Ein Eingabefeld, das bis zum rechten Rand reicht und mit dem Fenster schrumpft, behaelt eine Mindestbreite. Die Art steht
    // schon beim ItemAdd fest (imgui.cpp ItemAdd: LastItemData.ItemFlags vor dem Haken, InputTextEx gibt ImGuiItemFlags_Inputable
    // mit) - auch fuer ein Feld, das ganz hinter dem Rand liegt: ImGui schneidet es danach ab, und InputTextEx kehrt vor seinem
    // ItemInfo-Haken zurueck.
    const float mindest = ctx->FontSize * kFeldGequetscht;
    if ((ctx->LastItemData.ItemFlags & ImGuiItemFlags_Inputable) != 0 && bb.GetWidth() < mindest && bb.Max.x >= w->WorkRect.Max.x - 1.0F) {
        e->bedarf = std::max(e->bedarf, bb.Min.x + mindest - w->Pos.x + w->Scroll.x);
    }
}

ImVec2 mindestVon(const char* name) {
    const Zustand& s = z();
    const auto it = s.fenster.find(ImHashStr(name));
    if (!s.an || it == s.fenster.end() || it->second.frei) { return ImVec2{0.0F, 0.0F}; }
    return it->second.untergrenze;   // wie beim letzten Begin gesetzt (dort galt der Stil der Stelle, z. B. ihr Fensterpolster)
}

void mindestSchalter(bool an) { z().an = an; }

int knotenAnhebungen(std::string* liste) {
    const Zustand& s = z();
    if (liste != nullptr) { *liste = s.knotenListe; }
    return s.knotenGehoben;
}

// --- Formular-Baustein ----------------------------------------------------------------------------------

namespace {

ImVec4 mischen(const ImVec4& a, const ImVec4& b, float t) {
    return ImVec4{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
}

// Hauptknopf in der Betonungsfarbe (wie die Titelzeile des aktiven Fensters); gesperrt grau.
bool hauptKnopf(const char* text, float breite, bool aktiv) {
    if (!aktiv) {
        ImGui::BeginDisabled();
        (void)ImGui::Button(text, ImVec2{breite, 0.0F});
        ImGui::EndDisabled();
        return false;
    }
    // Die Betonungsfarbe des Themas (applyTheme: CheckMark = TitleBgActive = accent).
    ImVec4 a = ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
    a.w = 1.0F;
    const ImVec4 weiss{1.0F, 1.0F, 1.0F, 1.0F};
    const ImVec4 schwarz{0.0F, 0.0F, 0.0F, 1.0F};
    const float hell = 0.2126F * a.x + 0.7152F * a.y + 0.0722F * a.z;
    ImGui::PushStyleColor(ImGuiCol_Button, a);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, mischen(a, weiss, 0.15F));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, mischen(a, schwarz, 0.15F));
    ImGui::PushStyleColor(ImGuiCol_Text, hell > 0.6F ? schwarz : weiss);
    const bool k = ImGui::Button(text, ImVec2{breite, 0.0F});
    ImGui::PopStyleColor(4);
    return k;
}

float knopfBreite(const char* text) {
    if (text == nullptr) { return 0.0F; }
    const ImGuiStyle& st = ImGui::GetStyle();
    return std::max(ImGui::CalcTextSize(text, nullptr, true).x + st.FramePadding.x * 2.0F, ImGui::GetFontSize() * 6.0F);
}

}  // namespace

Formular::Formular(std::initializer_list<const char*> beschriftungen, float mindestFeld) {
    const float fs = ImGui::GetFontSize();
    float breit = 0.0F;
    for (const char* b : beschriftungen) {
        if (b != nullptr) { breit = std::max(breit, cjkTextSize(b, ImGui::FindRenderedTextEnd(b)).x); }
    }
    spalte_ = std::ceil(breit + fs * 0.75F);
    mindestFeld_ = std::floor(fs * mindestFeld);
    start_ = ImGui::GetCursorPosX();
    feld_ = mindestFeld_;
}

void Formular::zeile(const char* beschriftung) {
    ImGui::AlignTextToFramePadding();
    cjkTextUnformatted(beschriftung, ImGui::FindRenderedTextEnd(beschriftung));
    ImGui::SameLine(start_ + spalte_);
    feld_ = std::max(std::floor(ImGui::GetContentRegionAvail().x), mindestFeld_);
    feldOrt_ = ImGui::GetCursorScreenPos();
    ImGui::SetNextItemWidth(feld_);
}

void Formular::zeileOhne() {
    ImGui::SetCursorPosX(start_ + spalte_);
    feld_ = std::max(std::floor(ImGui::GetContentRegionAvail().x), mindestFeld_);
    feldOrt_ = ImGui::GetCursorScreenPos();
    ImGui::SetNextItemWidth(feld_);
}

bool Formular::feldKnopf(const char* id, UiIcon icon, const char* titel, const char* hinweis, bool imAusklapper) {
    // Das Feld aus der Zeile (nach einem offenen Ausklapper ist das letzte Element eines seiner Liste).
    const ImRect f{feldOrt_, ImVec2{feldOrt_.x + feld_, feldOrt_.y + ImGui::GetFrameHeight()}};
    const float h = f.GetHeight();
    const float rechts = f.Max.x - (imAusklapper ? ImGui::GetFrameHeight() : 0.0F);
    const ImVec2 a{rechts - h, f.Min.y};
    const ImVec2 nach = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(a);
    const bool geklickt = ImGui::InvisibleButton(id, ImVec2{h, h});
    const bool ueber = ImGui::IsItemHovered();
    const bool gedrueckt = ImGui::IsItemActive();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 b{a.x + h, a.y + h};
    // Deckend auf dem Feld (der Text des Felds laeuft darunter), wie die Pfeilflaeche des Ausklappers.
    dl->AddRectFilled(a, b, ImGui::GetColorU32(ImGuiCol_WindowBg, 1.0F));
    dl->AddRectFilled(a, b, ImGui::GetColorU32(ImGuiCol_FrameBg));
    if (ueber) { dl->AddRectFilled(a, b, ImGui::GetColorU32(gedrueckt ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered)); }
    dl->AddLine(ImVec2{a.x, a.y + 2.0F}, ImVec2{a.x, b.y - 2.0F}, ImGui::GetColorU32(ImGuiCol_Border));
    const float gr = std::floor(h * 0.62F);
    uiIcon(dl, icon, ImVec2{std::floor(a.x + (h - gr) * 0.5F), std::floor(a.y + (h - gr) * 0.5F)}, gr, ImGui::GetColorU32(uiIconFarbe(icon)));
    if (ueber) {
        ImGui::BeginTooltip();
        cjkTextUnformatted(titel);
        if (hinweis != nullptr && hinweis[0] != '\0') {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0F);
            cjkTextDisabled("%s", hinweis);
            ImGui::PopTextWrapPos();
        }
        ImGui::EndTooltip();
    }
    // Zurueck unter das Feld: der Knopf liegt in der Zeile des Felds.
    ImGui::SetCursorScreenPos(nach);
    return geklickt;
}

bool Formular::eingabeMitListe(const char* id, char* puffer, std::size_t groesse, const char* platzhalter,
                               const std::vector<std::string>& liste) {
    const ImGuiStyle& st = ImGui::GetStyle();
    const float h = ImGui::GetFrameHeight();
    ImGui::PushID(id);
    // Eingabe und Pfeil ohne Abstand: zusammen ein Feld wie der Ausklapper (imgui_widgets.cpp BeginCombo: Rahmen, rechts die
    // Pfeilflaeche in der Knopffarbe). Der Text der Eingabe endet vor dem Pfeil.
    ImGui::SetNextItemWidth(std::max(feld_ - h, 1.0F));
    bool geaendert = ImGui::InputTextWithHint("##eingabe", platzhalter, puffer, groesse);
    const ImVec2 f0 = ImGui::GetItemRectMin();
    ImGui::SameLine(0.0F, 0.0F);
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const ImVec2 b{a.x + h, a.y + h};
    // Wie der Ausklapper: ein Klick auf den Pfeil bei offener Liste schliesst sie nur (die offene Liste nimmt dem Fenster die Maus).
    const bool gedrueckt = ImGui::InvisibleButton("##pfeil", ImVec2{h, h});
    const bool ueber = ImGui::IsItemHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    // Die rechte Kante der Eingabe (ihr Rand, ihre runden Ecken) verschwindet unter der Feldfarbe; der Rand kommt um Eingabe und
    // Pfeil zusammen neu - ein Rahmen wie beim Ausklapper.
    dl->AddRectFilled(ImVec2{a.x - st.FrameRounding - st.FrameBorderSize, a.y}, ImVec2{a.x, b.y}, ImGui::GetColorU32(ImGuiCol_FrameBg));
    dl->AddRectFilled(a, b, ImGui::GetColorU32(ueber || ImGui::IsPopupOpen("##liste") ? ImGuiCol_ButtonHovered : ImGuiCol_Button), st.FrameRounding,
                      ImDrawFlags_RoundCornersRight);
    ImGui::RenderArrow(dl, ImVec2{a.x + st.FramePadding.y, a.y + st.FramePadding.y}, ImGui::GetColorU32(ImGuiCol_Text), ImGuiDir_Down, 1.0F);
    if (st.FrameBorderSize > 0.0F) { dl->AddRect(f0, b, ImGui::GetColorU32(ImGuiCol_Border), st.FrameRounding, ImDrawFlags_None, st.FrameBorderSize); }
    if (gedrueckt) { ImGui::OpenPopup("##liste"); }
    const float breite = b.x - f0.x;
    ImGui::SetNextWindowPos(ImVec2{f0.x, b.y});
    ImGui::SetNextWindowSizeConstraints(ImVec2{breite, 0.0F},
                                        ImVec2{std::max(breite, ImGui::GetFontSize() * 24.0F), ImGui::GetTextLineHeightWithSpacing() * 12.0F});
    if (ImGui::BeginPopup("##liste")) {
        for (const std::string& n : liste) {
            if (ImGui::Selectable(n.c_str(), std::strcmp(n.c_str(), puffer) == 0)) {
                std::snprintf(puffer, groesse, "%s", n.c_str());
                geaendert = true;
            }
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();
    return geaendert;
}

void Formular::hinweis(const char* text) {
    if (text == nullptr || text[0] == '\0') { return; }
    ImGui::SetCursorPosX(start_ + spalte_);
    ImGui::PushTextWrapPos(start_ + spalte_ + feld_);
    cjkTextDisabled("%s", text);
    ImGui::PopTextWrapPos();
}

void Formular::fehler(const char* text) {
    if (text == nullptr || text[0] == '\0') { return; }
    ImGui::SetCursorPosX(start_ + spalte_);
    ImGui::PushTextWrapPos(start_ + spalte_ + feld_);
    cjkTextColored(ImVec4{0.90F, 0.30F, 0.25F, 1.0F}, "%s", text);
    ImGui::PopTextWrapPos();
}

void Formular::codeVorschau(const char* id, const char* ueberschrift, const std::string& text) {
    const ImGuiStyle& st = ImGui::GetStyle();
    ImGui::SetCursorPosX(start_);
    if (ueberschrift != nullptr && ueberschrift[0] != '\0') { cjkTextDisabled("%s", ueberschrift); }
    ImFont* mono = menueeditor::codeSchrift();
    const std::string zeigen = text.empty() ? std::string("-") : text;
    if (mono != nullptr) { ImGui::PushFont(mono, 0.0F); }
    const ImVec2 gr = ImGui::CalcTextSize(zeigen.c_str(), zeigen.c_str() + zeigen.size());
    if (mono != nullptr) { ImGui::PopFont(); }
    ImGui::SetCursorPosX(start_);
    const float breite = std::max(std::floor(ImGui::GetContentRegionAvail().x), spalte_ + mindestFeld_);
    // So hoch wie die Zeilen. Zu lange Zeilen rollen waagrecht (nie Umbruch Zeichen fuer Zeichen): dann Platz fuer den Rollbalken.
    const bool rollt = gr.x > breite - st.FramePadding.x * 2.0F;
    const float hoehe = std::ceil(gr.y + st.FramePadding.y * 2.0F + (rollt ? st.ScrollbarSize : 0.0F));
    if (ImGui::BeginChild(id, ImVec2{breite, hoehe}, ImGuiChildFlags_FrameStyle, ImGuiWindowFlags_HorizontalScrollbar)) {
        if (mono != nullptr) { ImGui::PushFont(mono, 0.0F); }
        ImGui::TextUnformatted(zeigen.c_str(), zeigen.c_str() + zeigen.size());
        if (mono != nullptr) { ImGui::PopFont(); }
    }
    ImGui::EndChild();
}

Knopf knopfReihe(const char* haupt, const char* abbrechen, bool hauptAktiv, const char* hauptHinweis, float mindestBreite) {
    const ImGuiStyle& st = ImGui::GetStyle();
    ImGuiContext& g = *GImGui;
    const float w1 = knopfBreite(haupt);
    const float w2 = knopfBreite(abbrechen);
    const float breite = std::ceil((haupt != nullptr ? w1 + st.ItemSpacing.x : 0.0F) + w2);
    ImGui::Spacing();
    const float links = ImGui::GetCursorPosX();
    const float platz = std::max({std::floor(ImGui::GetContentRegionAvail().x), mindestBreite, breite});
    ImGui::SetCursorPosX(links + platz - breite);
    Knopf r = Knopf::Keiner;
    if (haupt != nullptr) {
        if (hauptKnopf(haupt, w1, hauptAktiv)) { r = Knopf::Haupt; }
        if (hauptHinweis != nullptr && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("%s", hauptHinweis);
        }
        ImGui::SameLine();
    }
    if (ImGui::Button(abbrechen, ImVec2{w2, 0.0F})) { r = Knopf::Abbrechen; }
    if (r == Knopf::Keiner && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
        if (haupt != nullptr && hauptAktiv && (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))) {
            r = Knopf::Haupt;
        } else if (g.ActiveIdPreviousFrame == 0 && g.ActiveId == 0 && ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            r = Knopf::Abbrechen;
        }
    }
    // Die Reihe ganz: links bis zur Inhaltskante, dazu die Knoepfe.
    const ImGuiWindow* w = g.CurrentWindow;
    mindestInhalt(ImVec2{(w->Pos.x - w->Scroll.x + links) - w->DC.CursorStartPos.x + breite, 0.0F});
    return r;
}

Formular::Knopf Formular::knoepfe(const char* haupt, const char* abbrechen, bool hauptAktiv, const char* hauptHinweis) {
    ImGui::SetCursorPosX(start_);
    const Knopf r = knopfReihe(haupt, abbrechen, hauptAktiv, hauptHinweis, spalte_ + mindestFeld_);
    const ImGuiStyle& st = ImGui::GetStyle();
    knopfBreite_ = std::ceil((haupt != nullptr ? knopfBreite(haupt) + st.ItemSpacing.x : 0.0F) + knopfBreite(abbrechen));
    return r;
}

Formular::~Formular() {
    ImGuiContext& g = *GImGui;
    const ImGuiWindow* w = g.CurrentWindow;
    if (w == nullptr) { return; }
    const float hoehe = (w->DC.CursorPos.y - g.Style.ItemSpacing.y) - w->DC.CursorStartPos.y;
    const float links = (w->Pos.x - w->Scroll.x + start_) - w->DC.CursorStartPos.x;
    const float breite = links + std::max(spalte_ + mindestFeld_, knopfBreite_);
    mindestInhalt(ImVec2{breite, hoehe});
}

}  // namespace bhed::gui::ui
