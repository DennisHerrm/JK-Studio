// ziffern.h - TEST1411 F: Zahlen mit Ziffern fester Breite (wie tabellarische Ziffern) fuer Bild- und Zeitanzeigen.
//
// Die Schrift des Programms hat Ziffern verschiedener Breite ("1" schmaler als "0"), und eine Anzeige wie "9 / 14  0.60 s"
// wird mit "10 / 14  0.67 s" breiter - alles rechts daneben rutschte beim Abspielen (shank 14.11.: "das sollte still sitzen").
// Hier steht jede Ziffer in einer Zelle so breit wie die breiteste Ziffer; die Breite einer Anzeige misst man an ihrem
// groessten Wert (zifferBreite der laengsten Zahl), dann bleibt sie beim Zaehlen gleich. Andere Zeichen behalten ihre Breite.
#ifndef BHED_GUI_ZIFFERN_H
#define BHED_GUI_ZIFFERN_H

#include "imgui.h"

#include <algorithm>
#include <cmath>

namespace bhed::gui {

// Die Zelle einer Ziffer: die breiteste der zehn in der aktuellen Schrift
[[nodiscard]] inline float zifferZelle() {
    float b = 0.0F;
    for (char c = '0'; c <= '9'; ++c) {
        const char s[2] = {c, '\0'};
        b = std::max(b, ImGui::CalcTextSize(s).x);
    }
    return b;
}

// Laenge eines Zeichens in UTF-8 (1 fuer ASCII)
[[nodiscard]] inline int zifferZeichenLaenge(const char* p) {
    const auto c = static_cast<unsigned char>(*p);
    if (c < 0x80U) { return 1; }
    int n = c >= 0xF0U ? 4 : (c >= 0xE0U ? 3 : 2);
    for (int k = 1; k < n; ++k) {
        if (p[k] == '\0') { return k; }
    }
    return n;
}

// Breite des Texts mit Ziffern fester Breite
[[nodiscard]] inline float zifferBreite(const char* text) {
    const float zelle = zifferZelle();
    float b = 0.0F;
    for (const char* p = text; *p != '\0';) {
        const int n = zifferZeichenLaenge(p);
        b += (*p >= '0' && *p <= '9') ? zelle : ImGui::CalcTextSize(p, p + n).x;
        p += n;
    }
    return b;
}

// Den Text ab `ort` zeichnen, jede Ziffer mittig in ihrer Zelle (Bildpunkte ganzzahlig wie AddText)
inline void zifferText(ImDrawList* dl, ImVec2 ort, ImU32 farbe, const char* text) {
    const float zelle = zifferZelle();
    float x = ort.x;
    for (const char* p = text; *p != '\0';) {
        const int n = zifferZeichenLaenge(p);
        const float b = ImGui::CalcTextSize(p, p + n).x;
        if (*p >= '0' && *p <= '9') {
            dl->AddText(ImVec2{std::floor(x + (zelle - b) * 0.5F), ort.y}, farbe, p, p + n);
            x += zelle;
        } else {
            dl->AddText(ImVec2{std::floor(x), ort.y}, farbe, p, p + n);
            x += b;
        }
        p += n;
    }
}

}  // namespace bhed::gui
#endif
