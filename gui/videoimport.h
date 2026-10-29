// videoimport.h - Menue-Editor M2 Punkt 2: Videos (mp4, webm, avi, wmv ...)
// ueber Windows Media Foundation in eine Bildfolge lesen.
//
// Eigener Code.
#ifndef BHED_GUI_VIDEOIMPORT_H
#define BHED_GUI_VIDEOIMPORT_H

#include "bhed/menue/roq.h"

#include <string>

namespace bhed::gui {

// Hoechstens maxBilder Bilder; false mit Grund in fehler.
bool leseVideoDatei(const std::string& pfad, menue::Bildfolge& out, int maxBilder, std::string* fehler);

}  // namespace bhed::gui

#endif  // BHED_GUI_VIDEOIMPORT_H
