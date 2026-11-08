// textumbruch.cpp - siehe textumbruch.h

#include "textumbruch.h"

#include "imgui_internal.h"

#include "bhed/umbruch.h"

#include <algorithm>
#include <cfloat>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iterator>
#include <string_view>
#include <unordered_map>

namespace bhed::gui {

namespace {

// Breite eines Stuecks ohne das Aufrunden von ImGui::CalcTextSize (IM_TRUNC(x + 0.99999f) je Aufruf): die Zeile ist die Summe
// der Vorschuebe ihrer Zeichen (ImFont::RenderText), aufgerundet je Stueck brach sie bis zu einem Punkt je Zeichen zu frueh um.
float breiteIn(ImFont* schrift, float groesse, std::string_view s) {
    return schrift->CalcTextSizeA(groesse, FLT_MAX, 0.0F, s.data(), s.data() + s.size()).x;
}

// MR6 P28: das Ergebnis je (Schrift, Groesse, Breite, Text). Vorher brach jeder lange CJK-Text (Changelog im Update-Fenster,
// Helfertexte) in jedem Bild neu um, je Stueck mit CalcTextSizeA. Was 120 Bilder lang nicht gefragt war, faellt heraus.
// MR7 P41: die Schrift als ihre Nummer im Atlas (io.Fonts->Fonts), nicht als Adresse - nach buildFonts koennen neue ImFont an
// alten Adressen liegen; buildFonts leert den Speicher (cjkSchriftenNeu).
struct Gemerkt {
    std::string fertig;
    int bild = 0;
};
std::unordered_map<std::string, Gemerkt>& gemerkt() {
    static std::unordered_map<std::string, Gemerkt> m;
    return m;
}

constexpr std::size_t kKopf = sizeof(int) + sizeof(float) * 2;   // vor dem Text im Schluessel

std::string schluesselVon(std::string_view t, float breite, const ImFont* schrift, float groesse) {
    const ImVector<ImFont*>& alle = ImGui::GetIO().Fonts->Fonts;
    const int nummer = alle.contains(const_cast<ImFont*>(schrift)) ? alle.index_from_ptr(alle.find(const_cast<ImFont*>(schrift))) : -1;
    std::string k(sizeof(int) + sizeof(float) * 2, '\0');
    std::memcpy(k.data(), &nummer, sizeof(int));
    std::memcpy(k.data() + sizeof(int), &groesse, sizeof(float));
    std::memcpy(k.data() + sizeof(int) + sizeof(float), &breite, sizeof(float));
    k.append(t);
    return k;
}

std::string zeilenText(std::string_view t, const std::vector<std::pair<std::size_t, std::size_t>>& zeilen) {
    std::string fertig;
    fertig.reserve(t.size() + zeilen.size());
    for (std::size_t i = 0; i < zeilen.size(); ++i) {
        if (i > 0) { fertig += '\n'; }
        fertig.append(t.substr(zeilen[i].first, zeilen[i].second - zeilen[i].first));
    }
    return fertig;
}

// Die Zeilen fuer `breite` als ein Text mit '\n' (bhed::umbruchZeilen); Stand und Protokoll des Selbsttests
std::string umbrechen(std::string_view t, float breite, ImFont* schrift, float groesse) {
    const auto messen = [schrift, groesse](std::string_view s) { return breiteIn(schrift, groesse, s); };
    CjkUmbruchStand& st = cjkUmbruchStand();
    ++st.umbruchAufrufe;
    if (!st.messen) {   // P18: ausserhalb des Selbsttests nur die Zeilen, keine Messdaten
        auto& m = gemerkt();
        const int bild = std::max(1, ImGui::GetFrameCount());
        if (m.size() > 64U && bild % 120 == 0) {
            for (auto it = m.begin(); it != m.end();) { it = bild - it->second.bild > 120 ? m.erase(it) : std::next(it); }
        }
        Gemerkt& g = m[schluesselVon(t, breite, schrift, groesse)];
        if (g.bild == 0) {
            ++st.umbruchGerechnet;
            g.fertig = zeilenText(t, umbruchZeilen(t, breite, messen));
        }
        g.bild = bild;
        return g.fertig;
    }
    ++st.umbruchGerechnet;
    const auto zeilen = umbruchZeilen(t, breite, messen);
    std::string fertig;
    fertig.reserve(t.size() + zeilen.size());
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
    if (st.protokollieren) {
        CjkUmbruchStand::Eintrag e;
        if (const ImGuiWindow* w = ImGui::GetCurrentWindowRead(); w != nullptr) { e.fenster = w->Name; }
        e.text.assign(t);
        e.breite = breite;
        e.zeilenBreiten = st.zeilenBreiten;
        for (std::size_t i = 0; i + 1 < zeilen.size(); ++i) {
            std::size_t n = zeilen[i].second;
            while (n < t.size() && t[n] == ' ') { ++n; }
            e.hart.push_back(n < t.size() && t[n] == '\n');
            // was nach dem Zeilenende bis zum Ende der naechsten Gruppe kommt (Leerzeichen und Gruppe, bhed::gruppeEnde)
            const std::size_t g = std::max(gruppeEnde(t, zeilen[i + 1].first), zeilen[i].second);
            e.naechstes.push_back(messen(t.substr(zeilen[i].second, g - zeilen[i].second)));
        }
        e.hart.push_back(true);
        e.naechstes.push_back(0.0F);
        st.protokoll.push_back(std::move(e));
    }
    return fertig;
}

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
    const std::string fertig = umbrechen(t, breite, ImGui::GetFont(), ImGui::GetFontSize());
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
    static CjkUmbruchStand s = [] {
        CjkUmbruchStand x;
        x.messen = std::getenv("BHED_EDITORTEST") != nullptr;   // P18: nur im Selbsttest
        return x;
    }();
    return s;
}

void cjkSchriftenNeu() { gemerkt().clear(); }

std::size_t cjkUmbruchGemerkt(std::string_view text) {
    std::size_t n = 0;
    for (const auto& [k, g] : gemerkt()) {
        n += k.size() == kKopf + text.size() && std::string_view(k).substr(kKopf) == text ? 1U : 0U;
    }
    return n;
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

ImVec2 cjkTextSize(const char* text, const char* ende, bool verstecken, float breite) {
    const char* sichtbar = verstecken ? ImGui::FindRenderedTextEnd(text, ende) : (ende != nullptr ? ende : text + std::strlen(text));
    const std::string_view t(text, static_cast<std::size_t>(sichtbar - text));
    if (breite <= 0.0F || !hatCjk(t)) { return ImGui::CalcTextSize(text, ende, verstecken, breite); }
    const std::string fertig = umbrechen(t, breite, ImGui::GetFont(), ImGui::GetFontSize());
    return ImGui::CalcTextSize(fertig.c_str(), fertig.c_str() + fertig.size());
}

void cjkAddText(ImDrawList* dl, ImFont* schrift, float groesse, const ImVec2& ort, ImU32 farbe, const char* text, const char* ende,
                float breite) {
    // ImDrawList::AddText (imgui_draw.cpp): ohne Schrift/Groesse die der Liste
    ImFont* f = schrift != nullptr ? schrift : dl->_Data->Font;
    const float g = groesse != 0.0F ? groesse : dl->_Data->FontSize;
    const std::string_view t(text, ende != nullptr ? static_cast<std::size_t>(ende - text) : std::strlen(text));
    if (breite <= 0.0F || !hatCjk(t)) {
        dl->AddText(f, g, ort, farbe, text, ende, breite);
        return;
    }
    const std::string fertig = umbrechen(t, breite, f, g);
    dl->AddText(f, g, ort, farbe, fertig.c_str(), fertig.c_str() + fertig.size());
}

}  // namespace bhed::gui
