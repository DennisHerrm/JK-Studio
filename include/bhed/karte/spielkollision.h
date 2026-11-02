// spielkollision.h - Karten-Editor, Stufe 8b Punkt 3 ("Im Editor spielen"):
// die Kollision direkt aus den Brushes und Patches der Karte, ohne Kompilieren.
//
// Portiert aus q3map2 (NetRadiant-Custom 20260114, tools/quake3/q3map2):
//   games.cpp (game_sof2 / game_jk2 / game_ja: die surfaceparm-Tabelle und
//   "default"), shaders.cpp (ApplySurfaceParm, AllocShaderInfo), map.cpp
//   (SetBrushContents, AddBrushBevels, SnapNormal/SnapPlaneImproved,
//   PlaneEqual), writebsp.cpp (EmitBrushes: der Inhalt des Brushes ist der
//   seines contentShader), surface.cpp (Patches werden auch nodraw
//   ausgegeben);
// und der Aufbau der Kollisionskarte wie CM_LoadMap (OpenJK
// code/qcommon/cm_load.cpp: CMod_LoadBrushes/CM_BoundBrush mit den Seiten
// 0..5 als Kasten, CMod_LoadPatches mit CM_GeneratePatchCollide) fuer den
// vorhandenen CM-Port (src/kollision.cpp).
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
#ifndef BHED_KARTE_SPIELKOLLISION_H
#define BHED_KARTE_SPIELKOLLISION_H

#include "bhed/kollision.h"
#include "bhed/saberspuren.h"
#include "bhed/karte/pinsel.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace bhed::karte {

class Karte;

// q3map2: contentFlags, surfaceFlags, compileFlags eines Shaders
struct Q3ShaderFlags {
    std::uint32_t inhalt = 0;
    std::uint32_t flaeche = 0;
    std::uint32_t compile = 0;
};
// AllocShaderInfo ("default") und ApplySurfaceParm je surfaceparm, wie
// game_sof2 (JK2) bzw. game_ja (dazu "inside", "forcesight").
[[nodiscard]] Q3ShaderFlags q3map2ShaderFlags(const std::vector<std::string>& surfaceparms, bool ja);

// SetBrushContents (map.cpp:455-503): welche Seite den Inhalt des Brushes
// bestimmt (contentShader), die vereinigten compileFlags, und ob es ein
// Ursprungs-Brush ist (C_ORIGIN: q3map2 entfernt ihn).
struct PinselInhalt {
    int seite = 0;                 // Index der Seite des contentShader
    Q3ShaderFlags flags;           // dessen Flags = der Inhalt des Brushes in der .bsp
    std::uint32_t compile = 0;     // compileFlags aller Seiten
    bool ursprung = false;
};
[[nodiscard]] PinselInhalt pinselInhalt(const std::vector<Q3ShaderFlags>& seiten);

// Liefert die surfaceparms eines Shaders (leer: nicht in den Skripten = "default").
using SurfaceparmFn = std::function<std::vector<std::string>(const std::string& shader)>;

struct SpielKollisionsBericht {
    int pinsel = 0;         // Brushes in der Kollisionskarte
    int seiten = 0;         // Seiten samt Bevels
    int bevels = 0;         // AddBrushBevels (Kasten- und Kantenbevels)
    int patches = 0;
    int ausgelassen = 0;    // trigger_*, Ursprungs-Brushes (C_ORIGIN), ungueltige
};

// Die Kollisionskarte aus allen Brushes und Patches von worldspawn,
// func_group und den Brush-Entities (ausser trigger_*: InitTrigger setzt
// CONTENTS_TRIGGER). Ein Baum mit einem Blatt: das Ergebnis jeder Spur ist das
// des Spiels (der BSP-Baum beschleunigt nur), nur bei genau gleich weiten
// Treffern kann die gemeldete Ebene eine andere sein.
SpielKollisionsBericht kollisionAusKarte(const Karte& k, const SurfaceparmFn& parms, bool ja, KollisionsKarte& aus);

// Was q3map2 aus einem Shaderskript fuer die Zeichenflaechen der Welt braucht (shaderInfo_t,
// Auszug): die surfaceparms und die Schalter, die CullSides und ClipSidesIntoTree fragen.
struct Q3ShaderInfo {
    std::vector<std::string> surfaceparms;
    bool autosprite = false;      // deformVertexes autosprite[2] (dazu detail und noClip)
    bool polygonOffset = false;   // polygonOffset
    bool noClip = false;          // q3map_noClip, q3map_terrain
    bool indexed = false;         // q3map_indexed, q3map_terrain
    // Abschluss rc607 (-meta, metaflaechen.inc): shaderInfo_t
    std::string name;             // shader, klein, ohne Endung (sortiert die Meta-Dreiecke)
    int breite = 64;              // shaderWidth (Bild oder q3map_textureSize; ohne Bild *default)
    int hoehe = 64;               // shaderHeight
    bool nonplanar = false;       // q3map_nonplanar, q3map_terrain
    bool forceMeta = false;       // q3map_forceMeta, q3map_terrain
    bool notjunc = false;         // q3map_notjunc, autosprite, q3map_terrain
    bool globalTexture = false;   // q3map_globaltexture
    bool invert = false;          // q3map_invert
    bool tcGen = false;           // q3map_tcGen vector / ivector
    std::array<std::array<float, 3>, 2> tcVecs{};
    float shadeAngle = 0;         // q3map_shadeAngle
    int lmSample = 0;             // q3map_lightmapSampleSize
    int lmBreite = 128;           // q3map_lightmapSize (LIGHTMAP_WIDTH)
    std::array<float, 3> lmAchse{};   // q3map_lightmapAxis
    float tessSize = 0;           // tessSize, q3map_tessSize
    bool bewegt = false;          // deformVertexes move: shaderInfo minmax gueltig (ShaderEintrag)
    std::array<float, 3> bewegtMin{};
    std::array<float, 3> bewegtMax{};
    // rc608 Teil 13 (Decal-Sonderfaelle): Nebel (FogDrawSurfaces) und FinishSurface
    bool nebelParms = false;      // fogparms
    bool keinNebel = false;       // q3map_noFog
    std::string klon;             // q3map_cloneShader
    std::string rueck;            // q3map_backShader
    bool skript = false;          // shaderText: der Shader steht in einem Skript (nicht nur ein Bild)
    int klonNr = -1;              // ShaderInfoForShader( klon ): Index in MarkenEingabe::shader (eingabeAusKarte)
    int rueckNr = -1;
};
using Q3ShaderInfoFn = std::function<Q3ShaderInfo(const std::string& shader)>;
struct ShaderEintrag;
// Was q3map2 fuer die Zeichenflaechen der Welt fragt, aus dem Shader des Inventars (nullptr: kein Shader).
[[nodiscard]] Q3ShaderInfo q3ShaderInfoAus(const ShaderEintrag* se);

// Liste reste2 Punkt 4: was beim Bau der Welt fuer die Marken herauskam (Selbsttest, ctest)
struct MarkenWeltBericht {
    int pinsel = 0;        // Pinsel der Welt (worldspawn, func_group)
    int seiten = 0;        // Zeichenflaechen aus Pinselseiten
    int verdeckt = 0;      // Seiten mit sichtbarem Teil, die CullSides verwirft
    int patches = 0;
    int modelle = 0;       // Flaechen der misc_model
    bool leck = false;     // FloodEntities: von draussen erreichbar
    bool leer = false;     // keine Entitaet im Freien (q3map2 schreibt dann keine Pinselseite)
};

// Die Welt fuer R_MarkFragments (Marken: Decals der Effekte, Brandspuren), wie bhed_q3map2 sie
// als Zeichenflaechen in die Welt der .bsp schreibt (src/karte/sichtbareseiten.inc, ein Port von
// ProcessWorldModel bis ClipSidesIntoTree): nur worldspawn und func_group (MoveBrushesToWorld -
// andere Brush-Entities sind eigene Modelle, R_MarkFragments geht nur tr.world ab); je Seite eines
// Brushes ihr sichtbarer Teil (visibleHull: die konvexe Huelle der Bruchstuecke in offenen
// Blaettern des BSP aus den Strukturpinseln, nach FloodEntities/FillOutside), ohne von CullSides
// verdeckte Seiten und ohne C_NODRAW; Patches ausser SURF_NODRAW (ParseMesh: SF_SKIP) und
// misc_model ohne target (AddTriangleModels, ueber k.kontext.modelle), beide nur, wenn sie ein
// offenes Blatt beruehren (FilterPatchIntoTree, FilterTrianglesIntoTree). SURF_NOIMPACT,
// SURF_NOMARKS und CONTENTS_FOG laesst R_BoxSurfaces_r liegen.
// meta: wie die Karte gebaut wird - mit -meta (die Vorgabe von NRC) setzt q3map2 die Flaechen neu zusammen.
[[nodiscard]] SpurWelt markenWeltAusKarte(const Karte& k, const Q3ShaderInfoFn& info, bool ja, MarkenWeltBericht* bericht = nullptr, bool meta = true);

// Dasselbe in zwei Teilen: markenEingabe schreibt im Hauptfaden ab, was der Bau aus der Karte liest
// (was der Editor in die .map schriebe: Ebenenpunkte, Shader, Schluessel, Patches, die Dreiecke der
// misc_model) - markenWeltBauen rechnet darauf in jedem Faden. *abbrechen true: so bald wie moeglich
// aufhoeren (leere Welt).
struct MarkenEingabe {
    // Die Texturangaben einer Seite, wie sie in der .map stehen (fuer die st der Meta-Dreiecke)
    struct Textur {
        std::array<float, 5> quake{};                     // shift, shift, rotate, scale, scale
        std::array<std::array<float, 3>, 2> bp{};         // texMat (brushDef)
        std::array<std::array<float, 4>, 2> valve{};      // side.vecs (Valve 220, schon durch scale geteilt)
    };
    // Was ParseEntityCompileParams aus den Schluesseln einer Entitaet liest
    struct Werte {
        int entityNum = 0;
        int cast = 1;
        int recv = 1;
        int sampleSize = 0;
        float lmScale = 0;
        float shadeAngle = 0;
        std::array<float, 3> ambient{1, 1, 1};
    };
    struct Seite {
        std::array<std::array<double, 3>, 3> punkte{};   // die Ebenenpunkte des Editors
        Textur textur;
        int shader = 0;                                  // Index in shader
        std::uint32_t inhalt = 0;                        // Inhaltsflags der Seite (Detail-Bit)
    };
    struct Patch {
        int breite = 0;
        int hoehe = 0;
        std::vector<std::array<float, 3>> punkte;
        std::vector<std::array<float, 2>> st;   // je Kontrollpunkt (rc608 Teil 13: -patchmeta, Nebel)
        int shader = 0;
    };
    struct Ent {
        std::string klasse;         // klein
        std::string ursprung;       // "origin"
        std::string blockGroesse;   // "_blocksize", "blocksize" oder "chopsize"
        std::string zielname;       // "targetname" (FindTargetEntity)
        std::string ziel;           // "target" (ProcessDecals)
        Werte werte;
        bool welt = false;
        std::vector<std::vector<Seite>> pinsel;
        std::vector<Patch> patches;
    };
    struct Modell {
        int shader = 0;
        std::vector<std::array<float, 3>> dreiecke;   // je drei Ecken, in der Welt
        std::vector<std::array<float, 3>> normalen;   // je Ecke
        std::vector<std::array<float, 2>> st;         // je Ecke
        bool erzwungen = false;                       // q3map_forceMeta oder spawnflags 4: geht in die Meta-Stufe
        Werte werte;
    };
    std::vector<Ent> entitaeten;
    std::vector<Modell> modelle;
    std::vector<Q3ShaderInfo> shader;
    bool ja = true;
    bool ganzzahlig = false;   // ebenenPunkteGanzzahlig()
    bool meta = true;          // gebaut mit -meta (die Vorgabe von NRC): metaflaechen.inc
    // rc608 Teil 13: die uebrigen Schalter von -bsp, die die Zeichenflaechen aendern (bsp.cpp BSPMain)
    float np = 0;              // -np <Grad>: npDegrees
    bool maxArea = false;      // -maxarea: MaxAreaFaceSurface statt StripFaceSurface
    bool patchMeta = false;    // -patchmeta: TriangulatePatchSurface
    bool nebel = true;         // ohne -nofog: FogDrawSurfaces
    // die Schluessel des worldspawn, die TriangulatePatchSurface liest (die Entitaet der Welt, auch fuer
    // die Patches der func_group)
    bool weltPatchMeta = false;     // "_patchMeta", "patchMeta"
    int weltPatchQualitaet = 0;     // "_patchQuality", "patchQuality"
    int weltPatchTeilung = 0;       // "_patchSubdivide", "patchSubdivide"
    bool weltPatchTeilungDa = false;
    PinselFormat format = PinselFormat::Quake3;
};
// Die Schalter aus dem -bsp-Befehl eines Build-Eintrags (-meta, -np, -maxarea, -patchmeta, -nofog)
void markenSchalterAusBefehl(const std::string& bspBefehl, MarkenEingabe& e);
[[nodiscard]] MarkenEingabe markenEingabe(const Karte& k, const Q3ShaderInfoFn& info, bool ja, bool meta = true);
[[nodiscard]] SpurWelt markenWeltBauen(const MarkenEingabe& e, MarkenWeltBericht* bericht = nullptr, const std::atomic<bool>* abbrechen = nullptr);

}  // namespace bhed::karte

#endif
