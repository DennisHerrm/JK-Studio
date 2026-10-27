// terraingen.h - Karten-Editor, Stufe 8b Punkt 1: das Plugin "Terrain
// Generator" von NetRadiant-Custom ohne Oberflaeche.
//
// Portiert aus NetRadiant-Custom master (origin/master 8216133, Commits
// 506ce1a, 8ebe370, 81ff3f5, b11bacc, 1c14e4c, 801018f - NICHT im Release
// 20260114, das sonst die Referenz ist), contrib/terrain_generator:
//   terrain_math.h (Vec3d, BrushData), noise.cpp (Perlin, Simplex),
//   terrain_engine.cpp (Hoehen- und Tunnelkarten), brush_builder.cpp (die
//   Pinsel), terrain_generator.cpp (dispatch: "About", "Generate Terrain" -
//   die Werte des Dialogs und do_generate).
// Dazu radiant/brush.h (Brush::addPlane mit contentFlags) und
// radiant/brushmodule.cpp (Brush_addFace reicht faceData.contents weiter) in
// der Fassung von master.
//
//   Terrain Generator for NetRadiant-Custom
//   Copyright (C) 2026 vallz and vld ("Developed by vallz and vld", About-Text;
//   die Quelldateien tragen keinen eigenen Kopf, es gilt die Lizenz von
//   NetRadiant-Custom).
//
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; version 2 of the License. This program is distributed
//   in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
//   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//   PURPOSE. See the GNU General Public License for more details (LICENSE).
#ifndef BHED_KARTE_TERRAINGEN_H
#define BHED_KARTE_TERRAINGEN_H

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace bhed::karte {
class Karte;
}

namespace bhed::karte::terraingen {

// --- terrain_math.h ---------------------------------------------------------------
struct BrushData {
    double min_x = 0, max_x = 0;
    double min_y = 0, max_y = 0;
    double min_z = 0, max_z = 0;
    double width_x = 0, length_y = 0, height_z = 0;
};

// --- terrain_engine.h -------------------------------------------------------------
using HeightMap = std::map<std::pair<double, double>, double>;
using WallMap = std::map<std::pair<double, double>, double>;

struct GridPoint {
    double x, y, z;
};
using TerrainMap = std::map<std::pair<double, double>, GridPoint>;

struct TunnelMaps {
    HeightMap floor_map;
    HeightMap ceiling_map;
    WallMap left_wall_map;
    WallMap right_wall_map;
    double step_z = 0;
};

enum class ShapeType { Flat = 0, Hill = 1, Crater = 2, Ridge = 3, Slope = 4, Volcano = 5, Valley = 6, Tunnel = 7, SlopeTunnel = 8 };
enum class NoiseType { Perlin = 0, Simplex = 1, Random = 2 };
enum class Axis { X = 0, Y = 1 };

// std::rand() der C-Laufzeit, die NRC benutzt (MinGW-msvcrt und MSVC-UCRT:
// dieselbe lineare Folge, RAND_MAX 0x7fff, Zustand je Faden, Anfang 1 ohne
// srand). NRC ruft nie srand: die Folge laeuft ueber die ganze Sitzung weiter.
using Zufall = std::function<int()>;
constexpr int kRandMax = 0x7fff;

double perlinNoise(double x, double y);    // Perlin::noise
double simplexNoise(double x, double y);   // Simplex::noise

BrushData make_manual_brush_data(double width, double length, double height);
void adjust_bounds_to_fit_grid(BrushData& target, double step_x, double step_y);
TerrainMap generate_height_map(const BrushData& target, double step_x, double step_y, ShapeType shape_type, double shape_height, double variance,
                               double frequency, NoiseType noise_type, double terrace_step, Axis axis, bool jitter_grid, double grid_step,
                               const Zufall& zufall);
TunnelMaps generate_tunnel_height_maps(const BrushData& target, double step_x, double step_y, double cave_height, double slope_height,
                                       double variance, double frequency, NoiseType noise_type, double terrace_step, Axis axis, double grid_step,
                                       const Zufall& zufall);

// --- brush_builder.h: was der Erbauer anlegt -------------------------------------
// _QERFaceData, wie fill_face sie fuellt (texdef scale 0.03125, contents
// BRUSH_DETAIL_MASK).
struct FaceData {
    double p0[3] = {0, 0, 0};
    double p1[3] = {0, 0, 0};
    double p2[3] = {0, 0, 0};
    float shift[2] = {0, 0};
    float rotate = 0;
    float scale[2] = {1, 1};
    std::string shader;
    int contents = 0;
    int flags = 0;
    int value = 0;
};
// Die func_group-Gruppen in der Reihenfolge von create_func_group, je Gruppe
// die Pinsel in Einfuege-Reihenfolge, je Pinsel die Flaechen (Brush_addFace).
struct Bau {
    std::vector<std::vector<std::vector<FaceData>>> gruppen;
    std::vector<int> gewaehlt;           // select_generated (Gruppen-Index)
    std::vector<std::string> fehler;     // globalErrorStream
};
void build_terrain_brushes(const BrushData& target, double step_x, double step_y, const TerrainMap& height_map, const char* top_texture,
                           bool split_diagonally, Bau& bau);
void build_tunnel_brushes(const BrushData& target, double step_x, double step_y, const TunnelMaps& maps, const char* top_texture,
                          double cave_height, double slope_height, Axis axis, Bau& bau);

// --- terrain_generator.cpp: die Werte des Dialogs ---------------------------------
// Die static-Variablen des Dialogs (Commit 801018f "add the storage mod for
// all variables"): sie bleiben, solange das Programm laeuft. Vorgaben wie NRC.
struct Werte {
    int ziel = 0;                      // s_target_index: 0 Use Selection, 1 Manual Size
    double manuellB = 1024, manuellL = 1024, manuellH = 64;   // s_manual_w/l/h (64..131072, Schritt 64)
    int quadratIndex = 3;              // s_sq_index: 8 16 32 64 128 256 512 1024
    bool erweitert = false;            // s_advanced
    int schrittX = 64, schrittY = 64;  // s_step_x/y (8..512, Schritt 8)
    bool unregelmaessig = false;       // s_jitter ("Irregular Grid")
    int form = 0;                      // s_shape_index (ShapeType)
    int achse = 0;                     // s_axis_index
    double formHoehe = 256;            // s_shape_height (0..4096, 2 Stellen, Schritt 8)
    double tunnelHoehe = 256;          // s_tunnel_height
    double terrasse = 0;               // s_terrace (0..512)
    int rasterIndex = 0;               // s_grid_step_index: 1 2 4 ... 1024
    int rauschen = 0;                  // s_noise_index (NoiseType)
    double varianz = 32;               // s_variance (0..1024, Schritt 1)
    double frequenz = 0.005;           // s_frequency (0.0001..1, 4 Stellen, Schritt 0.001)
    std::string textur;                // s_texture (leer: "textures/" bzw. die Wahl des Texturbrowsers)
    // nicht in NetRadiant-Custom: die Zufallsfolge mit einem festen Startwert
    // beginnen (srand vor dem Erzeugen); aus = wie NRC (die Folge laeuft weiter)
    bool startwertFest = false;
    int startwert = 1;
};
extern const int kQuadrate[8];        // { 8, 16, 32, 64, 128, 256, 512, 1024 }
extern const int kRasterSchritte[11]; // { 1, 2, 4, ..., 1024 }
extern const char* const kAbout;      // About-Text
extern const char* const kStandardTextur;   // DEFAULT_TEXTURE "textures/"
extern const char* const kGroessenSchluessel;   // TERRAINGEN_KEY "_terraingen_size"

// Die Auswahlgrenzen (query_sel): getBoundsSelected
struct AuswahlGrenzen {
    double x0 = 0, x1 = 0, y0 = 0, y1 = 0, z0 = 0, z1 = 0;
    bool valid = false;
};
[[nodiscard]] AuswahlGrenzen auswahlGrenzen(const Karte& k, double minGroesse = 64.0);
// read_stored_size: der gespeicherte Fussabdruck an der Entitaet eines gewaehlten Knotens
[[nodiscard]] bool gespeicherteGroesse(const Karte& k, double& w, double& l, double& h);

// Ein Fehlerkasten (m_pfnMessageBox mit EMessageBoxType::Error)
struct Meldekasten {
    std::string titel, text;
};
struct Ergebnis {
    bool ok = false;
    Meldekasten fehler;                     // nur wenn !ok
    std::vector<std::string> ausgabe;  // globalOutputStream
    std::vector<std::string> fehlerStrom;   // globalErrorStream
    int gruppen = 0, pinsel = 0;
};
// do_generate (terrain_generator.cpp:407-590): pruefen, Grenzen bestimmen, den
// Auswahl-Pinsel loeschen, erzeugen, waehlen, Fussabdruck schreiben - ein
// Undo-Schritt "terrainGenerator.generate".
Ergebnis erzeugen(Karte& k, const Werte& w, const Zufall& zufall);

// slope_derived (Use Selection + Slope / Slope Tunnel) und die Hoehe, die
// update_target_mode dann eintraegt (gespeicherte Hoehe, sonst die Auswahl);
// false: nichts eintragen.
[[nodiscard]] bool schraegeAusAuswahl(const Karte& k, const Werte& w, double& hoehe);

}  // namespace bhed::karte::terraingen

#endif
