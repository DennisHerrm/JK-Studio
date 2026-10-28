// modellimport.h - Karten-Editor, Stufe 9 Punkt 3: Modell-Import per Ziehen
// (eigene Funktion, nicht in NetRadiant-Custom).
//
// Portiert aus OpenJK (und Quake III Arena; code/qcommon/qfiles.h: md3Header_t, md3Frame_t,
// md3Surface_t, md3Shader_t, md3Triangle_t, md3St_t, md3XyzNormal_t, MD3_XYZ_SCALE,
// SHADER_MAX_VERTEXES/INDEXES; code/rd-vanilla/tr_model.cpp: die Normalen-Kodierung
// lat/lng, die R_LoadMD3 wieder auspackt) - das Format, der Schreiber ist neu.
//
//   Copyright (C) 1999-2005 Id Software, Inc.; 2000-2013 Raven Software; 2013-2026 OpenJK contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
//
// Aus einem gelesenen Netz (geometrieimport.h, .obj/.fbx/.glb) entsteht ein Paket fuer den
// Import-Ordner: <ordner>/<stamm>.md3 (ein Bild, je Shader eine Flaeche, zu grosse Flaechen
// geteilt: hoechstens 1000 Punkte und 2000 Dreiecke, hoechstens 32 Flaechen), die Texturen
// des Modells (eingebettet oder neben der Datei) und scripts/<stamm>.shader mit je Shader
// qer_editorimage und map. Der Shadername ist der Pfad der Textur ohne Endung - so findet
// auch ein Programm ohne die .shader das Bild. Ueber +-511 Einheiten wird verkleinert und
// "modelscale" gesetzt (MD3 speichert xyz als short * 64).
#ifndef BHED_KARTE_MODELLIMPORT_H
#define BHED_KARTE_MODELLIMPORT_H

#include "bhed/karte/geometrieimport.h"

#include <string>
#include <vector>

namespace bhed::karte {

struct Md3Paket {
    struct Datei {
        std::string pfad;     // relativ zum Import-Ordner, mit '/'
        std::string inhalt;
    };
    std::vector<Datei> dateien;   // [0] = die .md3
    std::string modellPfad;       // fuer misc_model "model"
    double modelscale = 1.0;      // != 1: "modelscale" setzen
    std::size_t flaechen = 0;
    std::size_t dreiecke = 0;
    std::string fehler;           // leer = gut
};

// stamm: Dateiname ohne Endung (klein, nur [a-z0-9_-]); Ordner models/import/<stamm>.
[[nodiscard]] Md3Paket md3Paket(const ImportNetz& netz, const std::string& stamm);

// Die Normalen-Kodierung von MD3 (lat = Azimut im hohen Byte, lng = Polarwinkel im niedrigen)
// und ihre Umkehrung wie R_LoadMD3 / tr.sinTable.
[[nodiscard]] short md3Normale(double x, double y, double z);
void md3NormaleAuspacken(short n, double& x, double& y, double& z);

}  // namespace bhed::karte

#endif
