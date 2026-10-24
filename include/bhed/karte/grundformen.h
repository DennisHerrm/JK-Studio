// grundformen.h - Karten-Editor: Pinsel-Grundformen (Prism, Cone, Sphere,
// Rock, Icosahedron) und das Umbauen des gewaehlten Pinsels.
//
// Portiert aus NetRadiant-Custom 20260114: radiant/brushmanip.cpp:76-460
// (max_extent, max_extent_2d, Brush_ConstructPrism, Brush_ConstructCone,
// Brush_ConstructSphere, Brush_ConstructRock, icosahedron::drawtri,
// icosahedron::Brush_ConstructIcosahedron, Brush_ConstructPrefab),
// :805-816 (Scene_BrushConstructPrefab), :1413-1432 (BrushMakeSided) und
// radiant/gtkdlgs.cpp:272-327 (DoSides: Bereiche und Vorgaben).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#ifndef BHED_KARTE_GRUNDFORMEN_H
#define BHED_KARTE_GRUNDFORMEN_H

#include "bhed/karte/pinsel.h"
#include "bhed/karte/protokoll.h"

#include <cstddef>
#include <string>

namespace bhed::karte {

class Karte;

// EBrushPrefab (brushmanip.h)
enum class Grundform { Quader, Prisma, Kegel, Kugel, Fels, Ikosaeder };

// Grenzen und Vorgabe des Dialogs "Arbitrary sides" (DoSides, gtkdlgs.cpp:286-305)
struct SeitenBereich {
    int vorgabe;
    int min;
    int max;
};
[[nodiscard]] SeitenBereich seitenBereich(Grundform art);

void prismaBauen(Pinsel& b, const Kasten& grenzen, std::size_t seiten, std::size_t achse, const std::string& shader,
                 const TexturProjektion& projektion, const Kontext& k, Protokoll& p);
void kegelBauen(Pinsel& b, const Kasten& grenzen, std::size_t seiten, std::size_t achse, const std::string& shader,
                const TexturProjektion& projektion, const Kontext& k, Protokoll& p);
void kugelBauen(Pinsel& b, const Kasten& grenzen, std::size_t seiten, const std::string& shader, const TexturProjektion& projektion,
                const Kontext& k, Protokoll& p);
// Die Zufallszahlen kommen wie bei NRC aus rand() der C-Laufzeit.
void felsBauen(Pinsel& b, const Kasten& grenzen, std::size_t seiten, const std::string& shader, const TexturProjektion& projektion,
               const Kontext& k, Protokoll& p);
void ikosaederBauen(Pinsel& b, const Kasten& grenzen, std::size_t unterteilungen, bool stutzen, const std::string& shader,
                    const TexturProjektion& projektion, const Kontext& k);

// Brush_ConstructPrefab (brushmanip.cpp:407-460) mit dem Rueckgaengig-Befehl
// ("brushPrism -sides 8 -axis 2", "brushSphere -sides 8", ...); achse =
// GlobalXYWnd_getCurrentViewType (0 YZ, 1 XZ, 2 XY).
void grundformBauen(Karte& k, Pinsel& b, Grundform art, const Kasten& grenzen, std::size_t seiten, bool option, std::size_t achse,
                    const std::string& shader, Protokoll& p);
// Scene_BrushConstructPrefab (brushmanip.cpp:805-816): der zuletzt gewaehlte
// Pinsel wird in seinen Grenzen neu gebaut (Projektion TexDef_Construct_Default).
void grundformAuswahl(Karte& k, Grundform art, std::size_t seiten, bool option, std::size_t achse, const std::string& shader, Protokoll& p);

}  // namespace bhed::karte

#endif
