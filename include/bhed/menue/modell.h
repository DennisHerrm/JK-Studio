// modell.h - Menue-Editor M2 Punkt 1: 3D-Modelle in Menues (ITEM_TYPE_MODEL)
// mit der vorhandenen Figurendarstellung (readGlm/applySkin/readGla/
// worldMatrices) zeichnen.
//
// Die Kamera ist aus OpenJK portiert (code/ui/ui_shared.cpp Item_Model_Paint
// ohne JK2_MODE: refdef mit RDF_NOWORLDMODEL, fov_x = rect.w/640*90 wenn
// nicht gesetzt, fov_y aus atan2, origin aus g2mins/g2maxs (sonst -16 -16
// -24 / 16 16 32), origin[0] = len / 0.268, Drehung angle + realTime /
// rotationSpeed; UI_UpdateCharacterSkin fuer das Item "character":
// models/players/<ui_char_model>/|head|torso|legs):
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
//
// Gezeichnet wird mit einem kleinen eigenen Rasterer in ein RGBA-Bild mit
// durchsichtigem Grund (das Spiel zeichnet das Modell ohne Weltmodell ueber
// das Menue). Licht: die Engine setzt ein Licht (500, weiss) an den
// Augpunkt; hier Lambert von vorn plus Umgebungsanteil - kein Lichtgitter
// des Renderers.
#ifndef BHED_MENUE_MODELL_H
#define BHED_MENUE_MODELL_H

#include "bhed/glm.h"
#include "bhed/menue/menue.h"
#include "bhed/menue/spieldaten.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace bhed::menue {

struct MenueModell {
    std::string schluessel;   // Modell|Haut|Animation - neu laden, wenn er sich aendert
    bool ok = false;
    GlmModel glm;
    GlaAnimation gla;
    std::vector<AnimEntry> anims;
    struct Bild {
        int w = 0, h = 0;
        std::vector<std::uint8_t> rgba;
    };
    std::vector<Bild> texturen;   // je Flaeche (leer = Ersatzfarbe)
    std::string fehler;
};

// Cvar-Lesen fuer das Item "character" (ui_char_model, ui_char_skin_*)
using CvarLeser = std::function<std::string(const std::string&)>;

// Was das Item zeigt: Modellpfad, Haut, Animation (leer = Grundstellung).
[[nodiscard]] std::string modellSchluessel(const Item& it, const CvarLeser& cvar);
// Modell, Haut, Skelett und Texturen aus den Spieldaten.
// bild: Shader-/Bildname -> Bildpunkte (die Oberflaeche loest Shader auf)
using ModellBildLader = std::function<bool(const std::string& name, MenueModell::Bild& out)>;
bool ladeMenueModell(const Spieldaten& d, const Item& it, const CvarLeser& cvar, const ModellBildLader& bild, MenueModell& out);
// Item_Model_Paint in ein Bild breite x hoehe (RGBA, Grund durchsichtig).
// zeitMs = DC->realTime. Gibt die Zahl der gezeichneten Dreiecke zurueck.
int zeichneMenueModell(const MenueModell& m, const Item& it, int zeitMs, int breite, int hoehe, std::vector<std::uint8_t>& rgba);

}  // namespace bhed::menue

#endif  // BHED_MENUE_MODELL_H
