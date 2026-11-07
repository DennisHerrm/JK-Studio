// umbruchtest.cpp - p_render: Zeilenumbruch mit CJK-Zeichen (bhed/umbruch.h).
// Breite je Zeichen fest: ASCII 1, sonst 2 (wie ein Vollbreitenzeichen).

#include "bhed/umbruch.h"

#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace {

int g_fehler = 0;

void pruefe(bool ok, const char* was) {
    if (!ok) {
        std::printf("FEHLER: %s\n", was);
        ++g_fehler;
    }
}

float messen(std::string_view s) {
    float w = 0;
    for (std::size_t i = 0; i < s.size();) {
        const auto b = static_cast<unsigned char>(s[i]);
        const std::size_t n = b >= 0xF0 ? 4 : b >= 0xE0 ? 3 : b >= 0xC0 ? 2 : 1;
        w += n == 1 ? 1.0F : 2.0F;
        i += n;
    }
    return w;
}

std::vector<std::string> zeilen(const std::string& t, float breite) {
    std::vector<std::string> z;
    for (const auto& [a, e] : bhed::umbruchZeilen(t, breite, messen)) { z.push_back(t.substr(a, e - a)); }
    return z;
}

bool beginntMit(const std::string& s, const char* p) { return s.rfind(p, 0) == 0; }
bool endetMit(const std::string& s, const char* p) {
    const std::string q(p);
    return s.size() >= q.size() && s.compare(s.size() - q.size(), q.size(), q) == 0;
}

}  // namespace

int main() {
    // der Hinweis des IK-Rollouts (Modell-Block 4) auf Japanisch: ImGui brach nach "\u624B\u3001" um
    const std::string ja = "\u624B\u3001\u8DB3\u307E\u305F\u306F\u80A2\u306E\u30B3\u30F3\u30C8\u30ED\u30FC\u30EB\u3092\u9078\u629E\u3057\u3066\u304F\u3060\u3055\u3044 (\u307E\u305F\u306F IK/FK \u3067\u80A2\u3092\u9078\u629E)\u3002";
    pruefe(bhed::hatCjk(ja), "hatCjk ja");
    pruefe(!bhed::hatCjk("Select a hand, foot or limb control."), "hatCjk en");
    {
        const auto z = zeilen(ja, 30.0F);
        pruefe(z.size() >= 2, "ja: mehrere Zeilen");
        for (std::size_t i = 0; i < z.size(); ++i) {
            pruefe(messen(z[i]) <= 30.0F, "ja: Zeile zu breit");
            if (i + 1 < z.size()) { pruefe(messen(z[i]) >= 26.0F, "ja: Zeile ausser der letzten mindestens 26 von 30"); }
            // Zeilenanfang: kein U+3001, U+3002, ")" und keine Leerzeichen; Zeilenende: keine oeffnende Klammer
            pruefe(!beginntMit(z[i], "\u3001") && !beginntMit(z[i], "\u3002") && !beginntMit(z[i], ")") && !beginntMit(z[i], " "), "ja: Sperre Zeilenanfang");
            pruefe(!endetMit(z[i], "("), "ja: Sperre Zeilenende");
        }
        std::string alles;
        for (const std::string& s : z) { alles += s; }
        std::string ohne;
        for (const char c : ja) {
            if (c != ' ') { ohne += c; }
        }
        std::string alles2;
        for (const char c : alles) {
            if (c != ' ') { alles2 += c; }
        }
        pruefe(alles2 == ohne, "ja: kein Zeichen verloren");
    }
    // Chinesisch mit Vollbreitenkomma
    {
        const auto z = zeilen("\u9009\u62E9\u624B\u3001\u811A\u6216\u80A2\u4F53\u63A7\u5236\u5668\uFF0C\u6216\u5728 IK/FK \u4E2D\u9009\u62E9\u80A2\u4F53\u3002\u8FD8\u6CA1\u6709\u6A21\u578B\u3002", 16.0F);
        for (std::size_t i = 0; i + 1 < z.size(); ++i) { pruefe(messen(z[i]) >= 12.0F, "zh: Zeile ausser der letzten mindestens 12 von 16"); }
        for (const std::string& s : z) { pruefe(!beginntMit(s, "\uFF0C") && !beginntMit(s, "\u3002") && !beginntMit(s, "\u3001"), "zh: Sperre Zeilenanfang"); }
    }
    // Lateinisch: nur an Leerzeichen, Leerzeichen am Zeilenanfang entfallen
    {
        const auto z = zeilen("aaa bbb ccc ddd", 7.0F);
        pruefe(z.size() == 2 && z[0] == "aaa bbb" && z[1] == "ccc ddd", "en: an Leerzeichen");
        const auto l = zeilen("abcdefghij", 4.0F);
        pruefe(l.size() == 3 && l[0] == "abcd" && l[2] == "ij", "en: zu langes Wort zeichenweise");
    }
    // Zeilenwechsel bleibt
    {
        const auto z = zeilen("\u65E5\u672C\n\u4E2D\u6587", 100.0F);
        pruefe(z.size() == 2 && z[0] == "\u65E5\u672C" && z[1] == "\u4E2D\u6587", "Zeilenwechsel");
    }
    std::printf("umbruch: %s (%d Fehler)\n", g_fehler == 0 ? "ok" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
