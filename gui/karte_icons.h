// karte_icons.h - die Symbole von NetRadiant-Custom fuer den Karten-Editor.
//
// Erzeugt wird gui/karte_icons.cpp von tools/gen_karte_icons.py aus NRCs
// setup/data/tools/bitmaps (GNU GPL v2): ein PNG-Streifen in einer Zeile,
// kKarteIconGroesse x kKarteIconGroesse je Symbol.
#ifndef BHED_GUI_KARTE_ICONS_H
#define BHED_GUI_KARTE_ICONS_H

#include <cstddef>

namespace bhed::gui::karteneditor {

struct KarteIcon {
    const char* name;   // Dateiname ohne Endung ("select_mouseresize")
    int spalte;         // Position im Streifen
};

extern const unsigned char kKarteIconPng[];
extern const std::size_t kKarteIconPngGroesse;
extern const int kKarteIconGroesse;
extern const KarteIcon kKarteIcons[];
extern const std::size_t kKarteIconAnzahl;

}  // namespace bhed::gui::karteneditor
#endif
