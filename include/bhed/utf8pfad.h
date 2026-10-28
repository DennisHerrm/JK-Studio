// utf8pfad.h - Stufe 8d Punkt 3: ein UTF-8-Text als Pfad
//
// std::filesystem::u8path ist seit C++20 veraltet (MSVC C4996/STL4021, bis Stufe 8d 94 Warnungen).
// Der Ersatz ist der Konstruktor aus std::u8string - dieselbe Umwandlung (UTF-8 nach UTF-16 unter
// Windows), nur ohne Warnung.
#ifndef BHED_UTF8PFAD_H
#define BHED_UTF8PFAD_H

#include <filesystem>
#include <string>
#include <string_view>

namespace bhed {

[[nodiscard]] inline std::filesystem::path utf8Pfad(std::string_view s) {
    return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(s.data()), s.size()));
}

}  // namespace bhed

#endif
