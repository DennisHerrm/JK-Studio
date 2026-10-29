// schrift_ttf.h - Menue-Editor M2 Punkt 9: Zeichen einer TTF rastern.
#ifndef BHED_GUI_SCHRIFT_TTF_H
#define BHED_GUI_SCHRIFT_TTF_H

#include "bhed/menue/schriftbau.h"

#include <string>
#include <vector>

namespace bhed::gui {

// Die Zeichen 32..255 (Windows-1252) in `pixel` Hoehe (Auf- bis Abstieg), dazu die Masse.
bool rastereTtf(const std::string& ttf, int pixel, std::vector<menue::GlyphBild>& aus, menue::SchriftMasse& m);

}  // namespace bhed::gui

#endif  // BHED_GUI_SCHRIFT_TTF_H
