// textumbruch.cpp - siehe textumbruch.h

#include "textumbruch.h"

#include "imgui_internal.h"

#include "bhed/umbruch.h"

#include <algorithm>
#include <cstdarg>
#include <string_view>

namespace bhed::gui {

namespace {

// wrapPos wie ImGui (TextWrapPos des Fensters; < 0 kein Umbruch). Wie TextEx (imgui_widgets.cpp): Breite bis wrapPos ab dem Cursor
void zeichnen(const char* text, const char* ende, float wrapPos) {
    const std::string_view t(text, ende != nullptr ? static_cast<std::size_t>(ende - text) : std::char_traits<char>::length(text));
    if (wrapPos < 0.0F || !hatCjk(t)) {
        const bool anders = wrapPos != ImGui::GetCurrentWindow()->DC.TextWrapPos;   // TextWrapped ohne gesetzten Umbruch
        if (anders) { ImGui::PushTextWrapPos(wrapPos); }
        ImGui::TextUnformatted(t.data(), t.data() + t.size());
        if (anders) { ImGui::PopTextWrapPos(); }
        return;
    }
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    const float breite = std::max(1.0F, ImGui::CalcWrapWidthForPos(w->DC.CursorPos, wrapPos));
    const auto messen = [](std::string_view s) { return ImGui::CalcTextSize(s.data(), s.data() + s.size()).x; };
    const auto zeilen = umbruchZeilen(t, breite, messen);
    std::string fertig;
    fertig.reserve(t.size() + zeilen.size());
    CjkUmbruchStand& st = cjkUmbruchStand();
    st.breitesteZeile = 0.0F;
    st.zeilenBreiten.clear();
    for (std::size_t i = 0; i < zeilen.size(); ++i) {
        if (i > 0) { fertig += '\n'; }
        const std::string_view z = t.substr(zeilen[i].first, zeilen[i].second - zeilen[i].first);
        fertig.append(z);
        const float zb = messen(z);
        if (i == 0) { st.ersteZeile = zb; }
        st.zeilenBreiten.push_back(zb);
        st.breitesteZeile = std::max(st.breitesteZeile, zb);
    }
    ++st.aufrufe;
    st.breite = breite;
    st.zeilen = static_cast<int>(zeilen.size());
    st.text.assign(t);
    ImGui::PushTextWrapPos(-1.0F);
    ImGui::TextUnformatted(fertig.c_str(), fertig.c_str() + fertig.size());
    ImGui::PopTextWrapPos();
}

void formatiert(float wrapPos, const char* fmt, va_list args) {
    // wie ImGui::TextV: "%s" ohne Kopie
    if (fmt[0] == '%' && fmt[1] == 's' && fmt[2] == 0) {
        const char* s = va_arg(args, const char*);
        zeichnen(s != nullptr ? s : "(null)", nullptr, wrapPos);
        return;
    }
    const char* a = nullptr;
    const char* e = nullptr;
    ImFormatStringToTempBufferV(&a, &e, fmt, args);
    zeichnen(a, e, wrapPos);
}

float wrapJetzt() { return ImGui::GetCurrentWindow()->DC.TextWrapPos; }

}  // namespace

CjkUmbruchStand& cjkUmbruchStand() {
    static CjkUmbruchStand s;
    return s;
}

void cjkText(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    formatiert(wrapJetzt(), fmt, args);
    va_end(args);
}

void cjkTextDisabled(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    formatiert(wrapJetzt(), fmt, args);
    ImGui::PopStyleColor();
    va_end(args);
}

void cjkTextColored(const ImVec4& farbe, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ImGui::PushStyleColor(ImGuiCol_Text, farbe);
    formatiert(wrapJetzt(), fmt, args);
    ImGui::PopStyleColor();
    va_end(args);
}

void cjkTextWrapped(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    // TextWrappedV: ohne gesetzten Umbruch am Rand des Inhalts (PushTextWrapPos(0))
    const float w = wrapJetzt();
    formatiert(w < 0.0F ? 0.0F : w, fmt, args);
    va_end(args);
}

void cjkTextUnformatted(const char* text, const char* ende) { zeichnen(text, ende, wrapJetzt()); }

}  // namespace bhed::gui
