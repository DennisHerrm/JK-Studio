// Erzeugt von tools/gen_changelog.py aus tools/changelog.txt - nicht von Hand aendern.
#pragma once

#include <cstddef>

namespace bhed::changelog {

struct Eintrag {
    const char* fassung;   // "rc583"
    const char* datum;     // "2026-10-11"
    // Je Sprache (Reihenfolge wie i18n::Language) die Punkte, durch '\n'
    // getrennt, UTF-8; nullptr = diese Fassung hat die Sprache nicht.
    const char* text[6];
};

// Neueste Fassung zuerst.
[[nodiscard]] const Eintrag* eintraege();
[[nodiscard]] std::size_t anzahl();
// Die Punkte einer Fassung in einer Sprache (Index wie i18n::Language) -
// englisch, wenn die Fassung sie nicht hat.
[[nodiscard]] const char* text(const Eintrag& e, int sprache);

}  // namespace bhed::changelog
