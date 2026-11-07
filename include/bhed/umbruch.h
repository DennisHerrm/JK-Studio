// umbruch.h - p_render: Zeilenumbruch fuer Text mit chinesischen, japanischen und koreanischen Zeichen.
//
// ImGui bricht nur an Leerzeichen und nach . , ; ! ? " sowie U+3001 U+3002 um (ImFontCalcWordWrapPositionEx,
// imgui_draw.cpp; die Zeichenklassen sind dort fest). Ein japanischer oder chinesischer Satz hat keine
// Leerzeichen: die erste Zeile endet am ersten Komma, der Rest wird mitten im Zeichen abgeschnitten.
// Hier darf zwischen zwei Zeichen umbrochen werden, wenn eines davon ein CJK-Zeichen ist, mit den
// ueblichen Sperren (JIS X 4051 Zeilenanfang/-ende, GB/T 15834): keine schliessenden Klammern,
// Satzzeichen, kleinen Kana oder Laengezeichen am Zeilenanfang, keine oeffnenden Klammern am Zeilenende.
// Leerzeichen und Woerter in lateinischer Schrift wie bei ImGui (Leerzeichen am Zeilenanfang entfallen,
// ein zu langes Wort wird zeichenweise geteilt).
#ifndef BHED_UMBRUCH_H
#define BHED_UMBRUCH_H

#include <cstddef>
#include <functional>
#include <string_view>
#include <utility>
#include <vector>

namespace bhed {

// Enthaelt der Text ein CJK-Zeichen (Ideogramme, Kana, Hangul, CJK-Satzzeichen, Vollbreite)?
[[nodiscard]] bool hatCjk(std::string_view text);
// Die Zeilen (Anfang, Ende als Byteversatz) fuer die Breite; messen gibt die Breite eines Stuecks.
// '\n' beendet eine Zeile; ohne CJK-Zeichen bricht die Regel nur an Leerzeichen (wie ImGui).
[[nodiscard]] std::vector<std::pair<std::size_t, std::size_t>> umbruchZeilen(std::string_view text, float breite,
                                                                            const std::function<float(std::string_view)>& messen);

}  // namespace bhed

#endif  // BHED_UMBRUCH_H
