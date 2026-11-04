// bsplumps.h - rc609 J: Shader, Nebel und Flaechen fertiger Karten bearbeiten.
//
// Liest und schreibt in einer fertigen .bsp das Shader-Lump (dshader_t: Name,
// surfaceFlags, contentFlags), das Fog-Lump (dfog_t: Shader, brushNum,
// visibleSide) und die Zuordnung je Flaeche (dsurface_t.shaderNum, fogNum) -
// IBSP 46 (Quake 3) und RBSP 1 (Jedi Outcast, Jedi Academy, Movie Duels; die
// Karten von JK2 sind ebenfalls RBSP 1, gemessen an assets0.pk3).
// Alle anderen Lumps bleiben Byte fuer Byte, ein neuer Shader wird angehaengt.
// Vorbild: die Lump-Reiter von SomaZ/BSP-Entity-Edit (MIT); dazu die Pruefungen
// wie die Engine laedt (Quellstellen bei den Funktionen).
//
// Portiert aus OpenJK (code/qcommon/cm_load.cpp CMod_LoadShaders/CMod_LoadBrushes/
// CMod_LoadBrushSides/CMod_LoadPatches, code/rd-vanilla/tr_bsp.cpp ShaderForShaderNum/
// ParseFace/R_LoadFogs, code/qcommon/qfiles.h, code/game/surfaceflags.h und
// codeJK2/game/surfaceflags.h; Movie Duels 9 ebenso) und aus q3map2 (bspfile_ibsp.cpp,
// bspfile_rbsp.cpp: dshader_t, dfog_t, dbrush_t, dbrushside_t, dsurface_t).
//
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2001 - 2013, Activision, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//
//   This file is part of the OpenJK source code. OpenJK is free software; you
//   can redistribute it and/or modify it under the terms of the GNU General
//   Public License version 2 as published by the Free Software Foundation.
//   This program is distributed in the hope that it will be useful, but WITHOUT
//   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//   FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
//   details (LICENSE).
#ifndef BHED_KARTE_BSPLUMPS_H
#define BHED_KARTE_BSPLUMPS_H

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace bhed::karte {

// qfiles.h (OJK code/ und codemp/, MD9 code/ und codemp/: je 0x400; der JK2-Modus von
// OpenJK nimmt code/qcommon/qfiles.h). Die Engine prueft die Zahl beim Laden nicht, sie ist
// die Grenze der Werkzeuge (q3map2) und des Formats.
constexpr std::size_t kMaxMapShaders = 0x400;
// dshader_t/dfog_t: char shader[MAX_QPATH] (64); CMod_LoadShaders kopiert mit Q_strncpyz(...,
// MAX_QPATH) - es bleiben 63 Zeichen und die NUL.
constexpr std::size_t kShaderNameMax = 63;

struct LumpShader {
    std::string name;
    std::uint32_t surfaceFlags = 0;
    std::uint32_t contentFlags = 0;
};
struct LumpNebel {
    std::string shader;
    std::int32_t brushNum = -1;      // -1: globaler Nebel
    std::int32_t visibleSide = -1;   // -1: keine Oberflaeche
};
struct LumpFlaeche {
    std::int32_t shaderNum = 0;
    std::int32_t fogNum = -1;
    std::int32_t typ = 0;        // MST_BAD 0, PLANAR 1, PATCH 2, TRIANGLE_SOUP 3, FLARE 4
    std::int32_t lightmap = -1;  // lightmapNum[0]: >= 0 gebackene Lightmap, -3 LIGHTMAP_BY_VERTEX ...
};
struct LumpBrush {
    std::int32_t ersteSeite = 0;
    std::int32_t seiten = 0;
    std::int32_t shaderNum = 0;
};

struct BspLumps {
    bool rbsp = true;                       // RBSP 1, sonst IBSP 46
    std::vector<LumpShader> shader;
    std::vector<LumpNebel> nebel;
    std::vector<LumpFlaeche> flaechen;
    std::vector<LumpBrush> brushes;         // nur gelesen (fuer die Pruefungen)
    std::vector<std::int32_t> seitenShader; // dbrushside_t.shaderNum, nur gelesen
};

// Liest die Lumps 1, 8, 9, 12, 13 (Laengen wie die Engine: "funny lump size" bei Rest).
[[nodiscard]] bool bspLumpsLesen(const std::string& bsp, BspLumps& out, std::string* fehler = nullptr);

// Prueft neu gegen alt (beide aus derselben Datei). "" = in Ordnung, sonst die Meldung
// (englisch, mit Quellstelle). Regeln: siehe bsplumps.cpp.
[[nodiscard]] std::string bspLumpsPruefen(const BspLumps& alt, const BspLumps& neu);

// Die Datei original mit den Lumps aus neu: Shader- und Fog-Lump (unveraenderte Eintraege
// Byte fuer Byte, auch hinter der NUL), im Flaechen-Lump nur shaderNum und fogNum; alle
// anderen Lumps unveraendert in ihrer Reihenfolge (bspMitLumps). "" bei Fehler.
[[nodiscard]] std::string bspLumpsSchreiben(const std::string& original, const BspLumps& neu, std::string* fehler = nullptr);

// --- Text ("Lumps als Text") ------------------------------------------------------------
// [shaders]  je Zeile:  nr "name" surfaceFlags contentFlags
// [fogs]     je Zeile:  nr "shader" brushNum visibleSide
// Kommentare mit //, Leerzeilen erlaubt; Zahlen dezimal oder 0x...; nr = Zeilennummer im
// Lump (0, 1, ...); eine Shaderzeile mit der naechsten Nummer haengt einen Shader an.
[[nodiscard]] std::string lumpsAlsText(const BspLumps& l);
struct LumpTextFehler {
    int zeile = 0;   // ab 1
    std::string text;
};
// Liest den Text auf der Grundlage basis (Flaechen, Brushes bleiben). false mit Fehler.
[[nodiscard]] bool lumpsAusText(const std::string& text, const BspLumps& basis, BspLumps& out, LumpTextFehler* fehler);
// Zeile (ab 1) des Shaders bzw. Nebels nr im Text, -1 wenn nicht gefunden.
[[nodiscard]] int lumpTextZeile(const std::string& text, bool nebel, int nr);
// Umgekehrt: welcher Eintrag steht in Zeile zeile (ab 1)? nebel sagt, in welchem Teil.
[[nodiscard]] int lumpTextEintrag(const std::string& text, int zeile, bool& nebel);

// --- Hilfen fuer die Bedienung ----------------------------------------------------------
// Der Eintrag fuer name: ein vorhandener mit genau diesen Werten, sonst angehaengt.
int shaderEintrag(BspLumps& l, const std::string& name, std::uint32_t surfaceFlags, std::uint32_t contentFlags);
// Wer den Shader benutzt. Kollision (cm_load.cpp): brushes -> contents (CMod_LoadBrushes),
// seiten -> surfaceFlags der Spuren (CMod_LoadBrushSides), patches -> contents und surfaceFlags
// der Patch-Kollision (CMod_LoadPatches). flaechen: alle Zeichenflaechen.
struct ShaderNutzung {
    int flaechen = 0;
    int patches = 0;
    int brushes = 0;
    int seiten = 0;
};
[[nodiscard]] ShaderNutzung shaderNutzung(const BspLumps& l, int shader);

// Namen aus surfaceflags.h je Spiel (OJK code/game, MD9 code/game, OJK codeJK2/game).
enum class FlagSpiel { Jka, MovieDuels, Jk2 };
struct FlagName {
    std::uint32_t bit = 0;
    const char* name = "";
};
[[nodiscard]] const std::vector<FlagName>& surfaceFlagNamen(FlagSpiel s);
[[nodiscard]] const std::vector<FlagName>& contentFlagNamen(FlagSpiel s);
// MATERIAL_* (die unteren MATERIAL_BITS = 5 Bit der surfaceFlags, MATERIAL_MASK 0x1f).
[[nodiscard]] const std::vector<FlagName>& materialNamen(FlagSpiel s);
constexpr std::uint32_t kMaterialMaske = 0x1fU;

}  // namespace bhed::karte

#endif
