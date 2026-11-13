// ui_menue.cpp - Menues im ganzen Programm (KLEIN2): bleiben im Fenster, aus dem sie aufgehen; Esc schliesst sie (ui_menue.h)
#include "ui_menue.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <cstdio>

namespace bhed::gui::ui {

namespace {

// Traegt der Viewport ein Fenster, aus dem Menues aufgehen (kein Popup, kein Menue, kein Tooltip)? Das Hauptfenster (sein Viewport
// gehoert keinem ImGui-Fenster), ein eigenes Tab-Fenster, ein schwebendes Panel.
bool wirt(const ImGuiViewportP* vp) {
    return vp != nullptr && (vp->Window == nullptr || (vp->Window->Flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_ChildMenu | ImGuiWindowFlags_Tooltip)) == 0);
}

// Das Menue oder Popup, das gleich aufgeht (ImGui-Fenster `name`), in den Viewport des Fensters legen, aus dem es aufgeht. Nicht, wenn
// der Aufrufer den Viewport schon bestimmt hat (dock::eigenesFensterWennVerdeckt: ein eigenes Fenster liegt ueber dem Hauptfenster, das
// Menue der Leiste muss darueber stehen) oder - bei einem Popup (`lageZaehlt`) - seine Lage (SetNextWindowPos: ImGui rueckt eine
// gesetzte Lage nicht ins Fenster, im festen Viewport waere der Rest abgeschnitten). Die Groesse aus dem letzten Bild sagt, ob es
// ueberhaupt ins Fenster passt (ein neues Popup zeichnet ImGui im ersten Bild verborgen und misst es); Rand wie
// GetPopupAllowedExtentRect (DisplaySafeAreaPadding auf jeder Seite).
void imFenster(const char* name, bool lageZaehlt) {
    ImGuiContext& g = *GImGui;
    const ImGuiWindow* w = g.CurrentWindow;
    const ImGuiNextWindowDataFlags gewaehlt =
        g.NextWindowData.HasFlags & (ImGuiNextWindowDataFlags_HasViewport | (lageZaehlt ? ImGuiNextWindowDataFlags_HasPos : 0));
    if (w == nullptr || w->SkipItems || gewaehlt != 0 || (g.ConfigFlagsCurrFrame & ImGuiConfigFlags_ViewportsEnable) == 0 || !wirt(w->Viewport)) {
        return;
    }
    const ImGuiWindow* m = ImGui::FindWindowByName(name);
    const ImVec2 rand{g.Style.DisplaySafeAreaPadding.x * 2.0F, g.Style.DisplaySafeAreaPadding.y * 2.0F};
    if (m == nullptr || (m->Size.x <= w->Viewport->Size.x - rand.x && m->Size.y <= w->Viewport->Size.y - rand.y)) {
        // Viewport fest: ViewportAllowPlatformMonitorExtend bleibt -1, die Lage rechnet im Rechteck des Fensters, und es wird kein
        // eigenes Fenster (WindowSelectViewport, FindBestWindowPosForPopup, "Late create viewport" in Begin)
        ImGui::SetNextWindowViewport(w->Viewport->ID);
    }
}

// Zu oder uebersprungen: BeginMenuEx und BeginPopupContextItem kehren bei SkipItems zurueck, ohne SetNextWindow* zu verbrauchen -
// sonst bekaeme das naechste Fenster des Bildes diesen Viewport
bool ergebnis(bool offen) {
    if (!offen) { GImGui->NextWindowData.ClearFlags(); }
    return offen;
}

// Name des Fensters eines Popups (BeginPopupEx: "##Popup_%08x" mit seiner Kennung)
void popupFenster(ImGuiID id) {
    char name[24];
    std::snprintf(name, sizeof name, "##Popup_%08x", id);
    imFenster(name, true);
}

}  // namespace

bool menue(const char* text, bool an) {
    // ImGui zeichnet das Menue dieser Tiefe immer in dasselbe Fenster ("<Titel>###Menu_<Tiefe>", BeginPopupMenuEx); die Lage eines
    // Menues rechnet ImGui immer selbst (SetNextWindowPos in BeginMenuEx ist nur der Bezugspunkt)
    char name[24];
    std::snprintf(name, sizeof name, "###Menu_%02d", GImGui->BeginMenuDepth);
    imFenster(name, false);
    return ergebnis(ImGui::BeginMenu(text, an));   // lint-menue: die eine Stelle fuer ImGui::BeginMenu
}

bool popup(const char* id, ImGuiWindowFlags flags) {
    if (ImGuiWindow* w = GImGui->CurrentWindow) { popupFenster(w->GetID(id)); }
    return ergebnis(ImGui::BeginPopup(id, flags));   // lint-menue: die eine Stelle fuer ImGui::BeginPopup
}

bool popupKontext(const char* id, ImGuiPopupFlags flags) {
    ImGuiContext& g = *GImGui;
    if (ImGuiWindow* w = g.CurrentWindow; w != nullptr && !w->SkipItems) {
        const ImGuiID kennung = id != nullptr ? w->GetID(id) : g.LastItemData.ID;   // wie BeginPopupContextItem
        if (kennung != 0) { popupFenster(kennung); }
    }
    return ergebnis(ImGui::BeginPopupContextItem(id, flags));   // lint-menue: die eine Stelle fuer ImGui::BeginPopupContextItem
}

bool popupKontextFenster(const char* id, ImGuiPopupFlags flags) {
    if (ImGuiWindow* w = GImGui->CurrentWindow) { popupFenster(w->GetID(id != nullptr ? id : "window_context")); }   // wie BeginPopupContextWindow
    return ergebnis(ImGui::BeginPopupContextWindow(id, flags));   // lint-menue: die eine Stelle fuer ImGui::BeginPopupContextWindow
}

int menueEsc() {
    ImGuiContext& g = *GImGui;
    if (g.OpenPopupStack.Size == 0 || g.IO.WantTextInput || !ImGui::IsKeyPressed(ImGuiKey_Escape, false)) { return 0; }
    // von oben alle Menues und Popups bis zum obersten modalen Dialog (der bleibt; Esc ist dort "Abbrechen" des Dialogs). Ein Popup,
    // das erst geoeffnet und noch nicht gezeichnet ist (kein Fenster), zaehlt wie ein modaler - wie ImGui::ClosePopupsExceptModals
    int bleiben = g.OpenPopupStack.Size;
    while (bleiben > 0) {
        const ImGuiWindow* w = g.OpenPopupStack[bleiben - 1].Window;
        if (w == nullptr || (w->Flags & ImGuiWindowFlags_Modal) != 0) { break; }
        --bleiben;
    }
    const int zu = g.OpenPopupStack.Size - bleiben;
    if (zu == 0) { return 0; }
    ImGui::ClosePopupToLevel(bleiben, true);
    // die Taste ist verbraucht, bis sie losgelassen wird: IsKeyPressed/Shortcut aller anderen sehen sie nicht (auch nicht die
    // Wiederholung beim Halten)
    ImGui::SetKeyOwner(ImGuiKey_Escape, ImHashStr("##ui_menue_esc"), ImGuiInputFlags_LockUntilRelease);
    return zu;
}

}  // namespace bhed::gui::ui
