// geometrieimport.h - Karten-Editor, Stufe 9 Punkt 2: Geometrie zu Brushes
// (eigene Funktion, nicht in NetRadiant-Custom).
//
// Portiert aus q3map2 (NRC 20260114, tools/quake3/q3map2): die Regeln von brush.cpp
// (CreateBrushWindings: jede Ebene ein Kasten-Polygon, an allen anderen beschnitten;
// leere Seiten fallen weg) und facebsp.cpp (SelectSplitPlaneNum: Teilerwahl nach
// Splits und Gleichgewicht) - die Rechnung selbst ist neu geschrieben.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
//
// Ablauf: netzLesen (obj, fbx, glb/gltf ueber assimp 6.0.2, BSD-3, liegt fuer q3map2 schon
// im Baum) liefert Teile (je Netz und Knoten, Lage eingerechnet, Y-oben auf Z-oben, Skala).
// teilUmwandeln macht aus einem Teil Brushes:
//   - geschlossenes Netz: ein BSP aus den Dreiecksebenen (gleiche Ebenen zusammengefasst =
//     planare Flaechen in einer Brush-Seite); jedes feste Blatt ist ein konvexer Brush
//     (die konvexe Zerlegung). Seiten auf einer Netz-Flaeche bekommen deren Material und
//     die UV als Texturmatrix (drei Punkte und ihre st), alle anderen (verdeckt, innen)
//     caulk.
//   - offenes Netz (Deko, Flaechen): jede planare Flaeche als dünnes Prisma nach hinten,
//     Rueckseite und Raender caulk.
//   - kleine oder offene Teile werden detail.
//   - Modell+Clip: das Netz bleibt ein Modell (misc_model), dazu Clip-Brushes aus derselben
//     Zerlegung (geschlossen) bzw. der konvexen Huelle (offen).
//   - Auto: Brushes, wenn geschlossen und hoechstens maxPinsel Stueck, sonst Modell+Clip.
#ifndef BHED_KARTE_GEOMETRIEIMPORT_H
#define BHED_KARTE_GEOMETRIEIMPORT_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace bhed::karte {

struct ImportPunkt {
    double x = 0, y = 0, z = 0;
};

struct ImportTeil {
    std::string name;
    std::string material;
    std::vector<ImportPunkt> punkte;
    std::vector<std::array<double, 2>> uv;           // je Punkt (st: s nach rechts, t nach unten)
    std::vector<std::array<std::uint32_t, 3>> dreiecke;
    bool hatUv = false;
};

struct ImportNetz {
    std::vector<ImportTeil> teile;
};

struct NetzOptionen {
    double skala = 1.0;
    bool yOben = true;   // Blender/glTF: Y oben -> Q3: Z oben
};

// endung: "obj", "fbx", "glb", "gltf" (klein). false mit Grund, wenn nichts lesbar ist.
[[nodiscard]] bool netzLesen(const std::string& bytes, const std::string& endung, const NetzOptionen& o, ImportNetz& out,
                             std::string* fehler = nullptr);

struct ImportSeite {
    std::array<ImportPunkt, 3> p;    // Ebenenpunkte wie in der .map (Normale = (p2-p0) x (p1-p0))
    std::array<ImportPunkt, 3> st;   // Texturkoordinaten der drei Punkte (nur mit uvGesetzt)
    bool uvGesetzt = false;
    std::string shader;
};

struct ImportPinsel {
    std::vector<ImportSeite> seiten;
    bool detail = false;
};

enum class TeilArt { Auto, Pinsel, ModellClip };

struct ZerlegOptionen {
    std::string caulk = "textures/system/caulk";
    std::string clip = "textures/system/clip";
    std::string shaderVorsatz = "textures/";   // + Material (ohne Endung, Kleinbuchstaben)
    std::string ohneMaterial = "textures/system/caulk";
    double dicke = 8.0;            // Prisma fuer offene Flaechen
    double detailGroesse = 64.0;   // Teile, deren groesste Kante kleiner ist, werden detail
    std::size_t maxPinsel = 256;   // Auto: mehr Brushes -> Modell+Clip
};

struct TeilErgebnis {
    TeilArt art = TeilArt::Pinsel;   // was tatsaechlich gewaehlt wurde (nie Auto)
    bool geschlossen = false;
    bool detail = false;
    std::size_t dreiecke = 0;
    std::size_t ebenen = 0;          // verschiedene Ebenen (planare Flaechen)
    std::vector<ImportPinsel> pinsel;   // Pinsel: die Geometrie; Modell+Clip: die Clip-Brushes
};

[[nodiscard]] bool teilGeschlossen(const ImportTeil& t);
[[nodiscard]] TeilErgebnis teilUmwandeln(const ImportTeil& t, TeilArt wahl, const ZerlegOptionen& o);

// Der Shadername eines Materials: Vorsatz + Name ohne Pfad und Endung, klein, nur [a-z0-9_/-].
[[nodiscard]] std::string materialShader(const std::string& material, const ZerlegOptionen& o);

}  // namespace bhed::karte

#endif
