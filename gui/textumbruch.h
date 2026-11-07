// textumbruch.h - p_render: ImGui-Text mit Umbruch zwischen CJK-Zeichen (bhed/umbruch.h).
//
// Gleiche Aufrufe wie ImGui::Text, TextDisabled, TextColored, TextWrapped und TextUnformatted. Ist ein Umbruch
// gesetzt (PushTextWrapPos bzw. TextWrapped) und enthaelt der Text CJK-Zeichen, bricht bhed::umbruchZeilen um
// (zwischen den Zeichen, mit Zeilenanfang- und -endsperren) und ImGui zeichnet die fertigen Zeilen als einen Text
// ohne eigenen Umbruch; sonst ruft jede Funktion genau ihr ImGui-Gegenstueck (gleiches Bild).
#ifndef BHED_GUI_TEXTUMBRUCH_H
#define BHED_GUI_TEXTUMBRUCH_H

#include "imgui.h"

#include <string>
#include <vector>

namespace bhed::gui {

void cjkText(const char* fmt, ...) IM_FMTARGS(1);
void cjkTextDisabled(const char* fmt, ...) IM_FMTARGS(1);
void cjkTextColored(const ImVec4& farbe, const char* fmt, ...) IM_FMTARGS(2);
void cjkTextWrapped(const char* fmt, ...) IM_FMTARGS(1);
void cjkTextUnformatted(const char* text, const char* ende = nullptr);

// Selbsttest: der letzte umbrochene CJK-Text (Breite, Zeilen, Breite der ersten Zeile) und die Zahl der Aufrufe
struct CjkUmbruchStand {
    int aufrufe = 0;
    float breite = 0.0F;
    float ersteZeile = 0.0F;
    float breitesteZeile = 0.0F;
    int zeilen = 0;
    std::vector<float> zeilenBreiten;
    std::string text;
};
[[nodiscard]] CjkUmbruchStand& cjkUmbruchStand();

}  // namespace bhed::gui

#endif  // BHED_GUI_TEXTUMBRUCH_H
