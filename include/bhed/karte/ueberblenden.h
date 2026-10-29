// ueberblenden.h - Karten-Editor, Stufe 9 Punkt 9: Texturen ueberblenden (q3map2 _indexmap)
//
// Portiert aus q3map2 (NetRadiant-Custom 20260114): map.cpp LoadEntityIndexMap (Schluessel
// _indexmap/_layers/_shader, TGA: Index = Rot * Schichten / 256), map.cpp AdjustBrushesForOrigin
// davor (eMinmax: Grenzen aller Pinsel und Patch-Kontrollpunkte, Schluessel "min"/"max"),
// surface.cpp GetShaderIndexForPoint (s = (x - min) / Breite, t = (max.y - y) / Hoehe) und
// GetIndexedShader (textures/<_shader>_<n> bzw. _<a>to<b>). Shader q3map_terrain (shaders.cpp).
// Das Malen der Index-Karte im Editor ist neu (nicht in NetRadiant-Custom).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
#pragma once

#include <string>
#include <vector>

namespace bhed::karte {

class Entitaet;
struct Kontext;

// Die Index-Karte einer func_group: je Bildpunkt die Schicht 0 .. schichten-1. Zeile 0 ist oben
// (groesstes Y), Spalte 0 links (kleinstes X) - wie GetShaderIndexForPoint.
struct IndexKarte {
    int breite = 0;
    int hoehe = 0;
    int schichten = 2;
    std::vector<unsigned char> index;   // breite * hoehe
};

// eMinmax der Entitaet wie q3map2 (Pinsel-Grenzen, Patch-Kontrollpunkte; "min"/"max" gehen vor)
bool blendGrenzen(const Entitaet& e, const Kontext& k, float mins[3], float maxs[3]);
// Bildpunkt fuer einen Weltpunkt (GetShaderIndexForPoint)
void blendBildpunkt(const float mins[3], const float maxs[3], int breite, int hoehe, float x, float y, int& px, int& py);
// Kreis mit Radius (Welteinheiten) um (x, y) auf die Schicht setzen; Zahl der geaenderten Punkte
int blendMalen(IndexKarte& im, const float mins[3], const float maxs[3], float x, float y, float radius, int schicht);
// 32-Bit-TGA (oben links, BGRA): Rot = Schicht * 256 / schichten + Mitte - LoadEntityIndexMap liest
// (Rot * schichten) / 256 zurueck
std::vector<unsigned char> blendTga(const IndexKarte& im);
bool blendTgaLesen(const std::vector<unsigned char>& daten, int schichten, IndexKarte& aus);
// Shader-Datei: Grund-Shader textures/<basis> (q3map_terrain, Bild der Schicht 0) und je Schicht
// textures/<basis>_<n>, je Paar textures/<basis>_<a>to<b> (zweite Schicht per Vertex-Alpha)
std::string blendShaderText(const std::string& basis, const std::vector<std::string>& texturen);

}  // namespace bhed::karte
