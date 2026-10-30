// seitenvorlagen.h - Menue-Editor M6 Punkt 5: Vorlagen ganzer Menues je Spiel - Hauptmenue,
// Einstellungsseite mit Reitern, Credits, Ladebildschirm. Ergebnis ist ein normaler menuDef mit
// Schluesseln, die alle Fassungen (JA/JK2/MD, SP/MP) lesen; die Namen, die die Engine selbst
// oeffnet (fluss.h), bekommen Hauptmenue (SP mainMenu, MP main) und Ladebildschirm
// (SP loadscreen aus code/cgame/cg_info.cpp, MP Connect aus UI_DrawConnectScreen).
//
// Eigener Code.
#ifndef BHED_MENUE_SEITENVORLAGEN_H
#define BHED_MENUE_SEITENVORLAGEN_H

#include "bhed/menue/menue.h"

#include <string>

namespace bhed::menue {

enum class SeitenVorlage { Hauptmenue, Einstellungen, Credits, Ladebildschirm };

// Der Name, unter dem die Engine das Menue findet (Hauptmenue, Ladebildschirm) bzw. der
// Name, den das Hauptmenue der Vorlage oeffnet (Einstellungen, Credits).
[[nodiscard]] std::string seitenVorlageName(SeitenVorlage art, Dialekt d);

// Der menuDef-Block (Einrueckung einr, Zeilenende nl), name = Name des menuDef.
[[nodiscard]] std::string seitenVorlage(SeitenVorlage art, Dialekt d, const std::string& name, const std::string& einr, const std::string& nl);

}  // namespace bhed::menue

#endif  // BHED_MENUE_SEITENVORLAGEN_H
