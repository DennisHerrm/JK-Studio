// ui_menue.cpp - Menues im ganzen Programm (KLEIN2): Esc schliesst sie (ui_menue.h)
#include "ui_menue.h"

#include "imgui.h"
#include "imgui_internal.h"

namespace bhed::gui::ui {

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
