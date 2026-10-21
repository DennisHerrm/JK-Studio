// tastatur.h - Karten-Editor: die Tastaturbelegung von Windows, wie Qt sie fuer
// Tastenkuerzel liest (QWindowsKeyMapper::updatePossibleKeyCodes).
//
// Qt (LGPL), wie NetRadiant-Custom es benutzt: fuer jede Taste die Zeichen,
// die sie mit den acht Zusatztasten-Kombinationen erzeugt (KeyboardLayoutItem,
// qwindowskeymapper.cpp), daraus bhed::karte::befehle::moeglicheTasten.
#ifndef BHED_GUI_TASTATUR_H
#define BHED_GUI_TASTATUR_H

#include "bhed/karte/befehle.h"

namespace bhed::gui::tastatur {

// Fuer eine ImGui-Taste (ImGuiKey als int) die Belegung aus der aktuellen
// Windows-Tastaturbelegung. ziffernblock: Qt setzt dann KeypadModifier;
// erweitertesReturn: das Enter des Ziffernblocks (Key_Enter statt Key_Return).
// false, wenn die Taste keine Belegung hat (Zusatztasten, Maus ...).
bool belegung(int imguiKey, karte::befehle::TastenBelegung& out, bool& ziffernblock, bool& erweitertesReturn);

// Die Tastenwiederholung von Windows (SPI_GETKEYBOARDDELAY / -SPEED), wie
// Qt sie fuer QAction::autoRepeat erlebt: Verzoegerung und Abstand in
// Sekunden. false, wenn das System sie nicht nennt.
bool wiederholung(float& verzoegerung, float& abstand);

}  // namespace bhed::gui::tastatur
#endif
