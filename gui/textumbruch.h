// textumbruch.h - p_render: ImGui-Text mit Umbruch zwischen CJK-Zeichen (bhed/umbruch.h).
//
// Gleiche Aufrufe wie ImGui::Text, TextDisabled, TextColored, TextWrapped und TextUnformatted. Ist ein Umbruch
// gesetzt (PushTextWrapPos bzw. TextWrapped) und enthaelt der Text CJK-Zeichen, bricht bhed::umbruchZeilen um
// (zwischen den Zeichen, mit Zeilenanfang- und -endsperren) und ImGui zeichnet die fertigen Zeilen als einen Text
// ohne eigenen Umbruch; sonst ruft jede Funktion genau ihr ImGui-Gegenstueck (gleiches Bild).
// MR4 Punkt 5: im ganzen Programm (tools/lint_cjkumbruch.py prueft gui/), dazu cjkTextSize und cjkAddText fuer die Stellen,
// die mit ImGui::CalcTextSize(..., breite) Platz reservieren und mit ImDrawList::AddText(..., breite) zeichnen.
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
// wie ImGui::CalcTextSize(text, ende, verstecken, breite): mit CJK-Zeichen und breite > 0 die Zeilen von cjkTextWrapped
[[nodiscard]] ImVec2 cjkTextSize(const char* text, const char* ende = nullptr, bool verstecken = false, float breite = -1.0F);
// wie ImDrawList::AddText(schrift, groesse, ort, farbe, text, ende, breite) (schrift nullptr / groesse 0: die der Liste)
void cjkAddText(ImDrawList* dl, ImFont* schrift, float groesse, const ImVec2& ort, ImU32 farbe, const char* text, const char* ende,
                float breite);

// Selbsttest: der letzte umbrochene CJK-Text (Breite, Zeilen, Breite der ersten Zeile) und die Zahl der Aufrufe
struct CjkUmbruchStand {
    // P18 (Codepruefung seit rc610): gemessen wird nur im Selbsttest (BHED_EDITORTEST gesetzt), sonst bleibt der Bildpfad frei
    bool messen = false;
    int aufrufe = 0;
    float breite = 0.0F;
    float ersteZeile = 0.0F;
    float breitesteZeile = 0.0F;
    int zeilen = 0;
    std::vector<float> zeilenBreiten;
    std::string text;
    // MR4 Punkt 5: mit protokollieren jeder umbrochene CJK-Text (Fenster, Breite, Zeilen; naechstes = Breite des ersten
    // Zeichens bzw. Worts der Folgezeile, hart = die Zeile endet an einem Zeilenwechsel des Texts)
    struct Eintrag {
        std::string fenster;
        std::string text;
        float breite = 0.0F;
        std::vector<float> zeilenBreiten;
        std::vector<float> naechstes;
        std::vector<bool> hart;
    };
    bool protokollieren = false;
    std::vector<Eintrag> protokoll;
};
[[nodiscard]] CjkUmbruchStand& cjkUmbruchStand();

}  // namespace bhed::gui

#endif  // BHED_GUI_TEXTUMBRUCH_H
