// brushexport.h - Karten-Editor, Stufe 4 Punkt 3: das Plugin brushexport2
// ("Export selected as Wavefront Object") ohne Oberflaeche.
//
// Portiert aus NetRadiant-Custom 20260114, contrib/brushexport/export.cpp
// (ExportData, ExportDataAsWavefront::WriteToFile, ExportSelection) und
// callbacks.cpp (OnExportClicked: die Optionen des Dialogs).
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
#ifndef BHED_KARTE_BRUSHEXPORT_H
#define BHED_KARTE_BRUSHEXPORT_H

#include <array>
#include <functional>
#include <string>
#include <vector>

namespace bhed::karte {
class Karte;
}

namespace bhed::karte::brushexport {

enum class Zusammen { keine, alle, nachMaterial };   // COLLAPSE_NONE, COLLAPSE_ALL, COLLAPSE_BY_MATERIAL

struct Optionen {
    Zusammen modus = Zusammen::keine;     // "Don't collapse" (Vorgabe)
    bool materialien = true;              // "Create material information (.mtl file)"
    bool kurzeNamen = false;              // "Use short material names (max. 20 chars)"
    bool objekte = false;                 // "Create (o)bjects instead of (g)roups"
    bool verschweissen = true;            // "Weld vertices"
    std::vector<std::string> ignoriert;   // "Ignored materials" (Kurzname, ohne Gross/klein)
};

struct Ergebnis {
    bool ok = false;
    std::string objPfad;   // mit ".obj"
    std::string mtlPfad;
    std::string obj;       // Inhalt der .obj
    std::string mtl;       // Inhalt der .mtl (leer ohne materialien)
};

// Die Durchschnittsfarbe der Textur eines Shaders (qtexture_t::color)
using TexturFarbe = std::function<std::array<float, 3>(const std::string& shader)>;

// ExportSelection + WriteToFile: die gewaehlten Pinsel in Auswahlreihenfolge,
// je Flaeche, die beitraegt (Face::contributes). Schreibt nichts; der
// Aufrufer legt obj/mtl unter objPfad/mtlPfad ab.
[[nodiscard]] Ergebnis exportieren(const Karte& k, const Optionen& o, const std::string& pfad, const TexturFarbe& farbe);

// OnAddMaterial: path_get_filename_start
[[nodiscard]] std::string materialName(const std::string& eingabe);

extern const char* const kAbout;   // der Text von "About" (ohne HTML)

}  // namespace bhed::karte::brushexport

#endif
