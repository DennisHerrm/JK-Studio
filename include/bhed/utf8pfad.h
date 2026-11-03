// utf8pfad.h - Stufe 8d Punkt 3: ein UTF-8-Text als Pfad
//
// std::filesystem::u8path ist seit C++20 veraltet (MSVC C4996/STL4021, bis Stufe 8d 94 Warnungen).
// Der Ersatz ist der Konstruktor aus std::u8string - dieselbe Umwandlung (UTF-8 nach UTF-16 unter
// Windows), nur ohne Warnung.
#ifndef BHED_UTF8PFAD_H
#define BHED_UTF8PFAD_H

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

namespace bhed {

// rc608 Teil 3b: Namen aus .menu-Dateien sind Latin-1. Kein gueltiges UTF-8 warf hier eine Ausnahme: Es gilt dann jedes Byte als ein Zeichen.
[[nodiscard]] inline std::filesystem::path utf8Pfad(std::string_view s) {
    bool gueltig = true;
    for (std::size_t i = 0; i < s.size() && gueltig;) {
        const auto c = static_cast<unsigned char>(s[i]);
        const std::size_t n = c < 0x80 ? 1 : (c >= 0xC2 && c <= 0xDF) ? 2 : (c >= 0xE0 && c <= 0xEF) ? 3 : (c >= 0xF0 && c <= 0xF4) ? 4 : 0;
        gueltig = n != 0 && i + n <= s.size();
        for (std::size_t k = 1; k < n && gueltig; ++k) { gueltig = (static_cast<unsigned char>(s[i + k]) & 0xC0) == 0x80; }
        i += n == 0 ? 1 : n;
    }
    if (gueltig) { return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(s.data()), s.size())); }
    std::u16string u;
    for (const char c : s) { u.push_back(static_cast<char16_t>(static_cast<unsigned char>(c))); }
    return std::filesystem::path(u);
}

}  // namespace bhed

#endif
