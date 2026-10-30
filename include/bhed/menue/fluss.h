// fluss.h - Menue-Editor M6 Punkt 1: Menuefluss. Portiert aus OpenJK (Wirkung von open/close
// wie Item_RunScript, Einstiege aus ui_main). Welche Menues oeffnet welches (open),
// wo geht es zurueck (close <selbst>, close all, onESC), welche oeffnet das Spiel selbst,
// welche erreicht man nie, wo gibt es keinen Weg hinaus oder keinen zurueck.
//
// Die Befehle und ihre Wirkung wie Item_RunScript (codemp/ui/ui_shared.c,
// code/ui/ui_shared.cpp): open/close mit Menus_OpenByName/Menus_CloseByName
// (Namen ohne Gross/Klein), "close all" = Menus_CloseAll, openGoToMenu = das Menue,
// das ein uiScript vorher gemerkt hat. Die Menues, die die Engine selbst oeffnet
// (Menus_ActivateByName/Menus_OpenByName mit festem Namen in code/ui/ui_main.cpp,
// codemp/ui/ui_main.c), sind die Einstiege.
//
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2001 - 2013, Activision, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//
//   OpenJK is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License version 2 as published by the
//   Free Software Foundation. This program is distributed in the hope that it
//   will be useful, but WITHOUT ANY WARRANTY; without even the implied
//   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//   GNU General Public License for more details (LICENSE).
#ifndef BHED_MENUE_FLUSS_H
#define BHED_MENUE_FLUSS_H

#include "bhed/menue/menue.h"

#include <string>
#include <vector>

namespace bhed::menue {

struct FlussMenue {
    std::string name;            // wie im menuDef (leer = fehlt: nur als Ziel von open bekannt)
    int datei = -1;              // Index in der uebergebenen Liste (-1 = fehlt)
    int menu = -1;               // Index in Datei::menus
    bool einstieg = false;       // die Engine oeffnet es selbst
    bool erreichbar = false;     // vom Einstieg ueber open erreichbar
    bool eingabe = false;        // nimmt Eingaben an (Knopf, Feld, Liste ... oder onESC)
    bool sackgasse = false;      // nimmt Eingaben, aber kein Weg hinaus (kein open, kein close)
    bool ohneZurueck = false;    // wird geoeffnet, aber nichts fuehrt zurueck (close/onESC/open Eltern)
    int ebene = -1;              // Schritte vom Einstieg (-1 = unerreichbar)
};

struct FlussKante {
    int von = 0, nach = 0;       // Indizes in Fluss::menues
    std::string wo;              // "onOpen", "onESC", "knopf1: action" ...
    int item = -1;               // Item im Menue "von" (-1 = Menue-Skript)
};

struct Fluss {
    std::vector<FlussMenue> menues;
    std::vector<FlussKante> kanten;   // open
};

// Die Menues, die die Engine des Dialekts mit festem Namen oeffnet.
[[nodiscard]] std::vector<std::string> einstiegsMenues(Dialekt d);

// Fluss ueber alle Menues der Dateien (ein Menuesatz).
[[nodiscard]] Fluss menueFluss(const std::vector<const Datei*>& dateien);

}  // namespace bhed::menue

#endif  // BHED_MENUE_FLUSS_H
