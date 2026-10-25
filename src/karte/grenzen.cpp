// grenzen.cpp - Karten-Editor, Stufe 5 Punkt 1: die Grenzen-Tabelle je
// Regelwerk (siehe grenzen.h).
//
// Portiert aus OpenJK (code/, codemp/, codeJK2/: die Werte der #defines und
// die Stellen, an denen die Engine sie prueft), aus Movie Duels (code/) und aus
// q3map2 (NetRadiant-Custom 20260114). Jede Zahl steht mit Datei:Zeile da;
// ctest karte_grenzen liest die Quelltexte und vergleicht Wert und Zeile.
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
#include "bhed/karte/grenzen.h"

#include "bhed/regelwerk.h"

#include <array>

namespace bhed::karte {

namespace {

using i18n::Str;

// Die Quellbaeume und ihre Ordner (relativ zur Wurzel des Baums).
constexpr const char* kOj = "OpenJK";        // SP\ojk\oj
constexpr const char* kMd = "MovieDuels";    // SP\mdgit
constexpr const char* kQ3 = "q3map2";        // third_party/q3map2

// Ein Spiel: wo seine Engine-Dateien liegen (fuer die Stellen).
struct Baum {
    const char* name;      // kOj / kMd
    const char* qcommon;   // "code/qcommon/"
    const char* renderer;  // "code/rd-vanilla/"
    const char* spiel;     // "code/game/" (Spielcode)
    bool jk2;
};

GrenzStelle st(const Baum& b, const char* ordner, const char* datei, int zeile, const char* name, long long wert) {
    GrenzStelle s;
    s.baum = b.name;
    s.datei = std::string(ordner) + datei;
    s.zeile = zeile;
    s.name = name;
    s.wert = wert;
    s.jk2 = b.jk2;
    return s;
}
GrenzStelle q3(const char* datei, int zeile, const char* name, long long wert) {
    GrenzStelle s;
    s.baum = kQ3;
    s.datei = datei;
    s.zeile = zeile;
    s.name = name;
    s.wert = wert;
    return s;
}

// Zeilen, die je Baum verschieden sind.
struct Zeilen {
    // qfiles.h
    int models, brushes, entities, entstring, shaders, areas, fogs, planes, nodes, brushsides, leafs, leaffaces,
        leafbrushes, portals, lighting, lightgrid, lightgridArray, visibility, drawSurfs, drawVerts, drawIndexes;
    int shaderMaxVerts, shaderMaxIndexes;
    // q_shared.h
    int maxClients, gentityBits, maxModels, maxSounds, maxSubBsp, maxSubmodels, maxFx, maxWorldFx, maxConfigstrings,
        maxGamestate, areaBytes, worldCoord;
    // cm_*.h / cm_load.cpp
    int facets, patchPlanes, patchVerts, mapBounds, boxHandle;
    // tr_local.h
    int shadernumBits, maxShaders, patchSize, maxLightmaps, maxSkins, maxDrawsurfs, fognumShift, refentShift;
    // Spielcode
    int spawnVars, spawnVarsChars, roffs, npcModelle;
};

// OpenJK code/ (JKA SP; mit JK2_MODE das Jedi Outcast der Engine)
constexpr Zeilen kZeilenSp{212, 213, 214, 215, 216, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 231, 233, 234, 235,
                           33, 34,
                           618, 621, 632, 637, 640, 642, 644, 649, 663, 730, 228, 802,
                           64, 65, 527, 44, 31,
                           43, 44, 592, 843, 844, 847, 874, 875,
                           221, 222, 34, 43};
// Jedi Outcast: dieselben Dateien mit JK2_MODE - nur diese Zeilen sind anders
Zeilen zeilenJk2() {
    Zeilen z = kZeilenSp;
    z.maxSounds = 635;
    z.maxWorldFx = 647;
    z.maxConfigstrings = 661;
    z.spawnVars = 176;        // codeJK2/game/g_local.h
    z.spawnVarsChars = 177;
    z.roffs = 34;             // codeJK2/game/g_roff.h
    z.npcModelle = 0;         // codeJK2/game/NPC_stats.cpp kennt keine Grenze
    return z;
}
// Movie Duels code/
constexpr Zeilen kZeilenMd{319, 320, 321, 322, 323, 325, 326, 327, 328, 329, 330, 331, 332, 333, 334, 335, 336, 338, 340, 341, 342,
                           33, 34,
                           665, 668, 678, 680, 682, 684, 686, 693, 704, 769, 222, 924,
                           61, 62, 545, 44, 31,
                           42, 43, 576, 827, 828, 830, 857, 858,
                           245, 246, 32, 66};
// OpenJK codemp/ (JKA MP)
constexpr Zeilen kZeilenMp{314, 315, 316, 317, 318, 320, 321, 322, 323, 324, 325, 326, 327, 328, 329, 330, 331, 332, 334, 335, 336,
                           34, 35,
                           890, 895, 920, 921, 925, 30, 923, 0, 937, 946, 222, 54,
                           66, 67, 519, 44, 31,
                           40, 41, 648, 878, 879, 882, 897, 898,
                           36, 37, 0, 0};

Grenze neu(const char* id, GrenzArt art, Str titel, const char* name, long long wert) {
    Grenze g;
    g.id = id;
    g.art = art;
    g.titel = titel;
    g.name = name;
    g.wert = wert;
    g.vanilla = wert;
    g.erlaubt = wert;
    return g;
}

std::vector<Grenze> baue(GrenzSpiel spiel) {
    const bool sp = spiel != GrenzSpiel::JkaMp;
    const bool md = spiel == GrenzSpiel::MovieDuels;
    const bool jk2 = spiel == GrenzSpiel::Jk2;
    const bool mp = spiel == GrenzSpiel::JkaMp;
    const Zeilen z = md ? kZeilenMd : (mp ? kZeilenMp : (jk2 ? zeilenJk2() : kZeilenSp));
    const Baum b = md ? Baum{kMd, "code/qcommon/", "code/rd-vanilla/", "code/game/", false}
                      : (mp ? Baum{kOj, "codemp/qcommon/", "codemp/rd-vanilla/", "codemp/game/", false}
                            : Baum{kOj, "code/qcommon/", "code/rd-vanilla/", jk2 ? "codeJK2/game/" : "code/game/", jk2});
    const std::string qc = b.qcommon;
    const std::string rd = b.renderer;
    const std::string gm = b.spiel;
    std::vector<Grenze> t;

    // --- BSP-Lumps (qfiles.h) ----------------------------------------------
    // Die Engine legt jeden Lump in seiner Groesse an (cm_load.cpp CMod_Load*,
    // tr_bsp.cpp R_Load*) und prueft die MAX_MAP_*-Werte nicht: Format- und
    // Compilergrenzen, "weich".
    const std::string nichtGeprueft = "nicht von der Engine geprueft (" + qc + "cm_load.cpp / " + rd +
                                      "tr_bsp.cpp legen den Lump in seiner Groesse an)";
    struct Lump {
        const char* id;
        Str titel;
        const char* name;
        long long wert;
        int zeile;
    };
    const Lump lumps[] = {
        {"bsp.entitaeten", Str::KeGrBspEntitaeten, "MAX_MAP_ENTITIES", 0x800, z.entities},
        {"bsp.enttext", Str::KeGrBspEnttext, "MAX_MAP_ENTSTRING", 0x40000, z.entstring},
        {"bsp.shader", Str::KeGrBspShader, "MAX_MAP_SHADERS", 0x400, z.shaders},
        {"bsp.modelle", Str::KeGrBspModelle, "MAX_MAP_MODELS", 0x400, z.models},
        {"bsp.pinsel", Str::KeGrBspPinsel, "MAX_MAP_BRUSHES", 0x8000, z.brushes},
        {"bsp.pinselseiten", Str::KeGrBspPinselseiten, "MAX_MAP_BRUSHSIDES", 0x20000, z.brushsides},
        {"bsp.ebenen", Str::KeGrBspEbenen, "MAX_MAP_PLANES", 0x20000, z.planes},
        {"bsp.knoten", Str::KeGrBspKnoten, "MAX_MAP_NODES", 0x20000, z.nodes},
        {"bsp.blaetter", Str::KeGrBspBlaetter, "MAX_MAP_LEAFS", 0x20000, z.leafs},
        {"bsp.blattflaechen", Str::KeGrBspBlattflaechen, "MAX_MAP_LEAFFACES", 0x20000, z.leaffaces},
        {"bsp.blattpinsel", Str::KeGrBspBlattpinsel, "MAX_MAP_LEAFBRUSHES", 0x40000, z.leafbrushes},
        {"bsp.flaechen", Str::KeGrBspFlaechen, "MAX_MAP_DRAW_SURFS", 0x20000, z.drawSurfs},
        {"bsp.punkte", Str::KeGrBspPunkte, "MAX_MAP_DRAW_VERTS", 0x80000, z.drawVerts},
        {"bsp.indizes", Str::KeGrBspIndizes, "MAX_MAP_DRAW_INDEXES", 0x80000, z.drawIndexes},
        {"bsp.nebel", Str::KeGrBspNebel, "MAX_MAP_FOGS", 0x100, z.fogs},
        {"bsp.licht", Str::KeGrBspLicht, "MAX_MAP_LIGHTING", 0x800000, z.lighting},
        {"bsp.sicht", Str::KeGrBspSicht, "MAX_MAP_VISIBILITY", mp ? 0x600000 : 0x400000, z.visibility},
        {"bsp.portale", Str::KeGrBspPortale, "MAX_MAP_PORTALS", 0x20000, z.portals},
    };
    for (const Lump& l : lumps) {
        Grenze g = neu(l.id, GrenzArt::BspLump, l.titel, l.name, l.wert);
        g.hart = false;
        g.stellen.push_back(st(b, b.qcommon, "qfiles.h", l.zeile, l.name, l.wert));
        g.pruefstelle = nichtGeprueft;
        g.rechnung = "Formatgrenze aus qfiles.h";
        t.push_back(std::move(g));
    }
    {   // Lichtgitter: das Feld lightGridArray haelt unsigned short (tr_bsp.cpp)
        Grenze g = neu("bsp.lichtgitter", GrenzArt::BspLump, Str::KeGrBspLichtgitter, "MAX_MAP_LIGHTGRID", 65535);
        g.stellen.push_back(st(b, b.qcommon, "qfiles.h", z.lightgrid, "MAX_MAP_LIGHTGRID", 65535));
        g.stellen.push_back(q3("tools/quake3/q3map2/bspfile_rbsp.cpp", 80, "MAX_MAP_GRID", 0xffff));
        g.pruefstelle = rd + "tr_bsp.cpp R_LoadLightGridArray: Indizes als unsigned short; q3map2 "
                             "bspfile_rbsp.cpp:158 schreibt bei vollem Gitter den Index 65535 (hinter dem Feld)";
        g.rechnung = "hoechstens 65535 Punkte (Index 0..65534)";
        t.push_back(std::move(g));
    }
    {
        Grenze g = neu("bsp.lichtgitterfeld", GrenzArt::BspLump, Str::KeGrBspLichtgitterfeld, "MAX_MAP_LIGHTGRID_ARRAY", 0x100000);
        g.hart = false;
        g.stellen.push_back(st(b, b.qcommon, "qfiles.h", z.lightgridArray, "MAX_MAP_LIGHTGRID_ARRAY", 0x100000));
        g.stellen.push_back(q3("tools/quake3/q3map2/q3map2.h", 208, "MAX_MAP_LIGHTGRID", 0x100000));
        g.pruefstelle = "Engine liest jede Groesse (" + rd + "tr_bsp.cpp R_LoadLightGridArray); q3map2 light.cpp:1749 "
                        "vergroessert gridsize, bis das Feld passt";
        g.rechnung = "Zellen des Gitters (gridBounds[0] * [1] * [2])";
        t.push_back(std::move(g));
    }
    {   // Areas: Bitfeld MAX_MAP_AREA_BYTES (cm_test.cpp CM_WriteAreaBits)
        Grenze g = neu("bsp.areas", GrenzArt::BspLump, Str::KeGrBspAreas, "MAX_MAP_AREAS", 0x100);
        g.stellen.push_back(st(b, b.qcommon, "qfiles.h", z.areas, "MAX_MAP_AREAS", 0x100));
        g.stellen.push_back(st(b, b.qcommon, "q_shared.h", z.areaBytes, "MAX_MAP_AREA_BYTES", 32));
        g.stellen.push_back(q3("tools/quake3/q3map2/q3map2.h", 201, "MAX_MAP_AREAS", 0x100));
        g.pruefstelle = qc + "cm_test.cpp CM_WriteAreaBits: Bitfeld aus MAX_MAP_AREA_BYTES (32 Bytes = 256 Areas)";
        g.rechnung = "MAX_MAP_AREA_BYTES * 8";
        t.push_back(std::move(g));
    }

    // --- Renderer ------------------------------------------------------------
    {
        Grenze g = neu("renderer.flaechenpunkte", GrenzArt::Renderer, Str::KeGrRenFlaechenpunkte, "SHADER_MAX_VERTEXES", 1000);
        g.jeObjekt = true;
        g.erlaubt = 999;
        g.stellen.push_back(st(b, b.qcommon, "qfiles.h", z.shaderMaxVerts, "SHADER_MAX_VERTEXES", 1000));
        g.stellen.push_back(q3("tools/quake3/q3map2/games.cpp", 756, "", 999));
        g.pruefstelle = rd + (md ? "tr_bsp.cpp:525 ParseTriSurf, tr_surface.cpp:60 RB_CheckOverflow"
                                 : (mp ? "tr_bsp.cpp:538 ParseTriSurf, tr_surface.cpp:59 RB_CheckOverflow"
                                       : "tr_bsp.cpp:555 ParseTriSurf, tr_surface.cpp:61 RB_CheckOverflow")) +
                        " (Com_Error bei numVerts >= SHADER_MAX_VERTEXES)";
        g.rechnung = "Fehler ab 1000 Punkten je Flaeche (>=), also hoechstens 999 - wie q3map2 (games.cpp:756)";
        t.push_back(std::move(g));
    }
    {
        Grenze g = neu("renderer.flaechenindizes", GrenzArt::Renderer, Str::KeGrRenFlaechenindizes, "SHADER_MAX_INDEXES", 6000);
        g.jeObjekt = true;
        g.erlaubt = 5999;
        g.stellen.push_back(st(b, b.qcommon, "qfiles.h", z.shaderMaxIndexes, "SHADER_MAX_INDEXES", 6000));
        g.pruefstelle = rd + "tr_surface.cpp RB_CheckOverflow (Com_Error bei indexes >= SHADER_MAX_INDEXES)";
        g.rechnung = "Fehler ab 6000 Indizes je Flaeche (>=)";
        t.push_back(std::move(g));
    }
    {
        Grenze g = neu("renderer.lightmaps", GrenzArt::Renderer, Str::KeGrRenLightmaps, "MAX_LIGHTMAPS", 256);
        g.stellen.push_back(st(b, b.renderer, "tr_local.h", z.maxLightmaps, "MAX_LIGHTMAPS", 256));
        g.pruefstelle = rd + (md ? "tr_bsp.cpp:236" : (mp ? "tr_bsp.cpp:246" : "tr_bsp.cpp:250")) +
                        " R_LoadLightmaps schreibt tr.lightmaps[] ohne Pruefung (Feld mit MAX_LIGHTMAPS Plaetzen)";
        g.rechnung = "Lightmaps zu 128 x 128 (LIGHTMAP_SIZE)";
        t.push_back(std::move(g));
    }
    {
        Grenze g = neu("renderer.nebel", GrenzArt::Renderer, Str::KeGrRenNebel, "QSORT_FOGNUM_SHIFT", 30);
        g.stellen.push_back(st(b, b.renderer, "tr_local.h", z.fognumShift, "QSORT_FOGNUM_SHIFT", 2));
        g.stellen.push_back(st(b, b.renderer, "tr_local.h", z.refentShift, "QSORT_REFENTITYNUM_SHIFT", 7));
        g.stellen.push_back(q3("tools/quake3/q3map2/q3map2.h", 204, "MAX_RBSP_FOGS", 30));
        g.pruefstelle = rd + "tr_main.cpp R_AddDrawSurf: der Nebel steht in den Bits 2..6 des Sortierschluessels; "
                             "q3map2 writebsp.cpp:450 warnt";
        g.rechnung = "5 Bits (7 - 2) = 31 Nebel-Indizes; Index = Nebel + 1, dazu der Platz der Nachtsicht (tr_bsp.cpp R_LoadFogs): 30";
        t.push_back(std::move(g));
    }
    {
        Grenze g = neu("renderer.patchgroesse", GrenzArt::Renderer, Str::KeGrRenPatchgroesse, "MAX_PATCH_SIZE", 32);
        g.jeObjekt = true;
        g.vanilla = 31;
        g.erlaubt = 31;
        g.stellen.push_back(st(b, b.renderer, "tr_local.h", z.patchSize, "MAX_PATCH_SIZE", 32));
        g.stellen.push_back(q3("tools/quake3/q3map2/q3map2.h", 124, "MAX_PATCH_SIZE", 31));
        g.pruefstelle = "q3map2 fog.cpp:128 Error \"MAX_PATCH_SIZE after split\"; NRC patchmodule.cpp:58 (31)";
        g.rechnung = "Kontrollpunkte je Richtung; q3map2 erlaubt 31";
        g.vanillaQuelle = "q3map2.h:124 (strenger als der Renderer)";
        t.push_back(std::move(g));
    }
    {
        Grenze g = neu("renderer.drawsurfs", GrenzArt::Renderer, Str::KeGrRenDrawsurfs, "MAX_DRAWSURFS", 0x10000);
        g.stellen.push_back(st(b, b.renderer, "tr_local.h", z.maxDrawsurfs, "MAX_DRAWSURFS", 0x10000));
        g.pruefstelle = rd + "tr_main.cpp R_AddDrawSurf: index = numDrawSurfs & DRAWSURF_MASK (ueberschreibt)";
        g.rechnung = "je Ansicht; geschaetzt aus dem PVS je Cluster (ohne Sichtkegel, ohne Modelle)";
        t.push_back(std::move(g));
    }
    {
        Grenze g = neu("renderer.shader", GrenzArt::Renderer, Str::KeGrRenShader, "MAX_SHADERS", mp ? (1 << 14) : (1 << 13));
        g.zaehlbar = false;
        g.stellen.push_back(st(b, b.renderer, "tr_local.h", z.shadernumBits, "SHADERNUM_BITS", mp ? 14 : 13));
        g.stellen.push_back(st(b, b.renderer, "tr_local.h", z.maxShaders, "MAX_SHADERS", mp ? (1 << 14) : (1 << 13)));
        g.pruefstelle = rd + "tr_shader.cpp (alle geladenen Shader: Welt, Modelle, Effekte, Menue)";
        g.rechnung = "1 << SHADERNUM_BITS; aus der Karte nicht zu zaehlen";
        t.push_back(std::move(g));
    }
    {
        Grenze g = neu("renderer.skins", GrenzArt::Renderer, Str::KeGrRenSkins, "MAX_SKINS", (md || mp) ? 1024 : 512);
        g.zaehlbar = false;
        g.stellen.push_back(st(b, b.renderer, "tr_local.h", z.maxSkins, "MAX_SKINS", (md || mp) ? 1024 : 512));
        g.pruefstelle = rd + "tr_image.cpp RE_RegisterSkin";
        g.rechnung = "alle geladenen Skins; aus der Karte nicht zu zaehlen";
        t.push_back(std::move(g));
    }

    // --- Kollision -----------------------------------------------------------
    {
        Grenze g = neu("kollision.submodelle", GrenzArt::Kollision, Str::KeGrKolSubmodelle, "MAX_SUBMODELS", 512);
        g.stellen.push_back(st(b, b.qcommon, mp ? "cm_local.h" : "q_shared.h", z.maxSubmodels, "MAX_SUBMODELS", 512));
        g.stellen.push_back(st(b, b.qcommon, "cm_local.h", z.boxHandle, "BOX_MODEL_HANDLE", 511));
        g.erlaubt = mp ? 510 : 511;
        g.pruefstelle = qc + (md ? "cm_load.cpp:188" : (mp ? "cm_load.cpp:146" : "cm_load.cpp:143")) +
                        " Com_Error bei mehr als MAX_SUBMODELS; der letzte Platz ist BOX_MODEL_HANDLE";
        g.rechnung = mp ? "MAX_SUBMODELS - 2 (BOX_MODEL_HANDLE, CAPSULE_MODEL_HANDLE, cm_local.h:31-32), samt Welt (Modell 0)"
                        : "MAX_SUBMODELS - 1 (BOX_MODEL_HANDLE, cm_local.h:31), samt Welt (Modell 0)";
        t.push_back(std::move(g));
    }
    {
        const long long w = md ? 8192 : 4096;
        Grenze g = neu("kollision.patchebenen", GrenzArt::Kollision, Str::KeGrKolPatchebenen, "MAX_PATCH_PLANES", w);
        g.jeObjekt = true;
        g.stellen.push_back(st(b, b.qcommon, "cm_patch.h", z.patchPlanes, "MAX_PATCH_PLANES", w));
        if (!md) {
            g.vanilla = 2048;
            g.erlaubt = 2048;
            g.vanillaQuelle = jk2 ? "offen: keine eigene JK2-Quelle; geprueft gegen Jedi Academy vanilla 2048 (" + qc +
                                        "cm_patch.h:65 \"Was 2048 on vanilla jka\")"
                                  : qc + (mp ? "cm_patch.h:67" : "cm_patch.h:65") + " \"Was 2048 on vanilla jka\"";
            g.offen = jk2;
        } else {
            g.erlaubt = w;
        }
        g.pruefstelle = qc + (md ? "cm_patch.cpp:423 / :475" : (mp ? "cm_patch.cpp:479 / :533" : "cm_patch.cpp:514 / :568")) +
                        " Com_Error, wenn eine weitere Ebene kommt (numPlanes == MAX_PATCH_PLANES)";
        g.rechnung = "je Patch (CM_GeneratePatchCollide); hoechstens MAX_PATCH_PLANES";
        t.push_back(std::move(g));
    }
    {
        const long long w = md ? 4096 : 1024;
        Grenze g = neu("kollision.patchfacetten", GrenzArt::Kollision, Str::KeGrKolPatchfacetten, "MAX_FACETS", w);
        g.jeObjekt = true;
        g.stellen.push_back(st(b, b.qcommon, "cm_patch.h", z.facets, "MAX_FACETS", w));
        g.pruefstelle = qc + (md ? "cm_patch.cpp:1005" : (mp ? "cm_patch.cpp:1060" : "cm_patch.cpp:1094")) + " Com_Error \"MAX_FACETS\"";
        g.rechnung = "je Patch; hoechstens MAX_FACETS";
        t.push_back(std::move(g));
    }
    {
        Grenze g = neu("kollision.patchpunkte", GrenzArt::Kollision, Str::KeGrKolPatchpunkte, "MAX_PATCH_VERTS", 1024);
        g.jeObjekt = true;
        g.stellen.push_back(st(b, b.qcommon, "cm_load.cpp", z.patchVerts, "MAX_PATCH_VERTS", 1024));
        g.pruefstelle = qc + (md ? "cm_load.cpp:581" : (mp ? "cm_load.cpp:556" : "cm_load.cpp:564")) + " Com_Error \"ParseMesh: MAX_PATCH_VERTS\"";
        g.rechnung = "Breite * Hoehe der Kontrollpunkte je Patch";
        t.push_back(std::move(g));
    }
    {
        Grenze g = neu("kollision.weltgrenze", GrenzArt::Kollision, Str::KeGrKolWeltgrenze, "MAX_MAP_BOUNDS", 65535);
        g.stellen.push_back(st(b, b.qcommon, "cm_polylib.h", z.mapBounds, "MAX_MAP_BOUNDS", 65535));
        g.stellen.push_back(st(b, b.qcommon, "q_shared.h", z.worldCoord, "MAX_WORLD_COORD", 65536));
        g.stellen.push_back(q3("tools/quake3/common/maxworld.h", 4, "MAX_WORLD_COORD", 65536));
        g.erlaubt = 65534;
        g.pruefstelle = qc + "cm_patch.cpp CM_ValidateFacet: eine Facette mit einer Grenze >= MAX_MAP_BOUNDS wird verworfen";
        g.rechnung = "groesster Betrag einer Koordinate (ganze Einheiten) unter MAX_MAP_BOUNDS";
        t.push_back(std::move(g));
    }

    // --- Laufzeit --------------------------------------------------------------
    {
        const long long bits = md ? 12 : 10;
        const long long clients = mp ? 32 : 1;
        Grenze g = neu("laufzeit.entitaeten", GrenzArt::Laufzeit, Str::KeGrLzEntitaeten, "MAX_GENTITIES", 1LL << bits);
        g.stellen.push_back(st(b, b.qcommon, "q_shared.h", z.gentityBits, "GENTITYNUM_BITS", bits));
        g.stellen.push_back(st(b, b.qcommon, "q_shared.h", z.maxClients, "MAX_CLIENTS", clients));
        g.erlaubt = (1LL << bits) - 2 - clients;
        g.pruefstelle = gm + (md ? "g_utils.cpp:1130" : (mp ? "g_utils.c:845" : (jk2 ? "g_utils.cpp:802" : "g_utils.cpp:847"))) +
                        " G_Error \"G_Spawn: no free entities\"";
        g.rechnung = "MAX_GENTITIES - ENTITYNUM_WORLD/NONE (2) - MAX_CLIENTS; gezaehlt wird, was nach dem Spawn bleibt";
        t.push_back(std::move(g));
    }
    const std::string cfgStelle = gm + (md ? "g_utils.cpp:107 (nur Meldung, schreibt dann in den naechsten Bereich)"
                                           : (mp ? "g_utils.c:110 Error" : (jk2 ? "g_utils.cpp:78 G_Error" : "g_utils.cpp:76 G_Error"))) +
                                  " \"G_FindConfigstringIndex: overflow\"";
    {
        const long long w = (md || mp) ? 512 : 256;
        Grenze g = neu("laufzeit.modelle", GrenzArt::Laufzeit, Str::KeGrLzModelle, "MAX_MODELS", w);
        g.stellen.push_back(st(b, b.qcommon, "q_shared.h", z.maxModels, "MAX_MODELS", w));
        g.erlaubt = w - 1;
        g.pruefstelle = cfgStelle;
        g.rechnung = "Index 1..MAX_MODELS-1 (G_FindConfigstringIndex beginnt bei 1)";
        t.push_back(std::move(g));
    }
    {
        const long long w = md ? 512 : ((jk2 || mp) ? 256 : 380);
        Grenze g = neu("laufzeit.klaenge", GrenzArt::Laufzeit, Str::KeGrLzKlaenge, "MAX_SOUNDS", w);
        g.stellen.push_back(st(b, b.qcommon, "q_shared.h", z.maxSounds, "MAX_SOUNDS", w));
        g.erlaubt = w - 1;
        g.pruefstelle = cfgStelle;
        g.rechnung = "Index 1..MAX_SOUNDS-1";
        t.push_back(std::move(g));
    }
    {
        const long long w = md ? 256 : (mp ? 64 : 128);
        Grenze g = neu("laufzeit.effekte", GrenzArt::Laufzeit, Str::KeGrLzEffekte, "MAX_FX", w);
        g.stellen.push_back(st(b, b.qcommon, "q_shared.h", z.maxFx, "MAX_FX", w));
        g.erlaubt = w - 1;
        g.pruefstelle = cfgStelle;
        g.rechnung = "Index 1..MAX_FX-1";
        t.push_back(std::move(g));
    }
    if (sp) {
        const long long w = md ? 128 : (jk2 ? 4 : 66);
        Grenze g = neu("laufzeit.welteffekte", GrenzArt::Laufzeit, Str::KeGrLzWelteffekte, "MAX_WORLD_FX", w);
        g.stellen.push_back(st(b, b.qcommon, "q_shared.h", z.maxWorldFx, "MAX_WORLD_FX", w));
        g.erlaubt = w - 1;
        g.pruefstelle = cfgStelle + (jk2 ? "" : "; " + gm + "g_fx.cpp (fx_rain, fx_snow, fx_wind, ...)");
        g.rechnung = "Index 1..MAX_WORLD_FX-1";
        t.push_back(std::move(g));
    }
    {
        const long long w = md ? 32000 : 16000;
        Grenze g = neu("laufzeit.gamestate", GrenzArt::Laufzeit, Str::KeGrLzGamestate, "MAX_GAMESTATE_CHARS", w);
        g.stellen.push_back(st(b, b.qcommon, "q_shared.h", z.maxGamestate, "MAX_GAMESTATE_CHARS", w));
        g.pruefstelle = md ? "code/client/cl_parse.cpp:457 und cl_cgame.cpp:354 Com_Error \"MAX_GAMESTATE_CHARS exceeded\""
                           : "client/cl_parse.cpp Com_Error \"MAX_GAMESTATE_CHARS exceeded\" (nicht im Quellstand SP\\ojk\\oj; "
                             "dieselbe Pruefung wie MovieDuels code/client/cl_parse.cpp:457)";
        g.rechnung = "alle Configstrings samt Endnull; aus der Karte nur eine Untergrenze (Modelle, Klaenge, Effekte, Text)";
        t.push_back(std::move(g));
    }
    if (sp) {
        Grenze g = neu("laufzeit.roffs", GrenzArt::Laufzeit, Str::KeGrLzRoffs, "MAX_ROFFS", 128);
        g.stellen.push_back(st(b, b.spiel, "g_roff.h", z.roffs, "MAX_ROFFS", 128));
        g.pruefstelle = gm + (md ? "g_roff.cpp:459" : (jk2 ? "g_roff.cpp:425" : "g_roff.cpp:648")) +
                        " \"MAX_ROFFS count exceeded. Skipping load\" (die ROFF fehlt)";
        g.rechnung = "verschiedene .rof je Level; num_roffs >= MAX_ROFFS laedt nicht mehr";
        t.push_back(std::move(g));
    }
    if (sp && !jk2) {
        const long long w = md ? 120 : 60;
        Grenze g = neu("laufzeit.npcmodelle", GrenzArt::Laufzeit, Str::KeGrLzNpcModelle, "MAX_MODELS_PER_LEVEL", w);
        g.stellen.push_back(st(b, b.spiel, "NPC_stats.cpp", z.npcModelle, "MAX_MODELS_PER_LEVEL", w));
        g.pruefstelle = gm + (md ? "NPC_stats.cpp:1334" : "NPC_stats.cpp:1179") + " Com_Error \"About to overflow modelsAlreadyDone\"";
        g.rechnung = "verschiedene Figurenmodelle (playerModel) je Level, samt Spieler";
        t.push_back(std::move(g));
    }
    if (!jk2) {   // codeJK2/game kennt kein misc_bsp (kein G_BSPIndex)
        Grenze g = neu("laufzeit.subbsp", GrenzArt::Laufzeit, Str::KeGrLzSubBsp, "MAX_SUB_BSP", 32);
        g.stellen.push_back(st(b, b.qcommon, "q_shared.h", z.maxSubBsp, "MAX_SUB_BSP", 32));
        g.erlaubt = mp ? 32 : 31;
        g.pruefstelle = qc + (md ? "cm_load.cpp:1131" : (mp ? "cm_load.cpp:1040" : "cm_load.cpp:1117")) +
                        " Com_Error \"CM_LoadSubBSP: too many unique sub BSPs\"" +
                        (mp ? std::string() : "; " + cfgStelle + " (G_BSPIndex)");
        g.rechnung = mp ? "verschiedene misc_bsp (bspmodel) je Level"
                        : "verschiedene misc_bsp je Level: G_BSPIndex legt sie als Configstring ab Index 1 an (MAX_SUB_BSP - 1)";
        t.push_back(std::move(g));
    }

    // --- Spawn -----------------------------------------------------------------
    {
        Grenze g = neu("spawn.variablen", GrenzArt::Spawn, Str::KeGrSpVariablen, "MAX_SPAWN_VARS", 64);
        g.jeObjekt = true;
        g.stellen.push_back(st(b, b.spiel, mp ? "bg_public.h" : "g_local.h", z.spawnVars, "MAX_SPAWN_VARS", 64));
        g.pruefstelle = gm + (md ? "g_spawn.cpp:1430" : (mp ? "g_spawn.c:1182" : (jk2 ? "g_spawn.cpp:1037" : "g_spawn.cpp:1257"))) +
                        " \"G_ParseSpawnVars: MAX_SPAWN_VARS\"";
        g.rechnung = "Schluessel je Entitaet (wie im Entity-Lump, samt \"model\" der Brush-Entities)";
        t.push_back(std::move(g));
    }
    {
        const long long w = mp ? 4096 : 2048;
        Grenze g = neu("spawn.zeichen", GrenzArt::Spawn, Str::KeGrSpZeichen, "MAX_SPAWN_VARS_CHARS", w);
        g.jeObjekt = true;
        g.stellen.push_back(st(b, b.spiel, mp ? "bg_public.h" : "g_local.h", z.spawnVarsChars, "MAX_SPAWN_VARS_CHARS", w));
        g.pruefstelle = gm + (md ? "g_spawn.cpp:1355" : (mp ? "g_spawn.c:976" : (jk2 ? "g_spawn.cpp:970" : "g_spawn.cpp:1189"))) +
                        " \"G_AddSpawnVarToken: MAX_SPAWN_VARS\"";
        g.rechnung = "je Entitaet: Summe (Laenge + 1) aller Schluessel und Werte";
        t.push_back(std::move(g));
    }

    // --- Compiler (q3map2) -------------------------------------------------------
    {
        Grenze g = neu("compiler.lichtschalter", GrenzArt::Compiler, Str::KeGrCoLichtschalter, "MAX_SWITCHED_LIGHTS", 32);
        g.stellen.push_back(q3("tools/quake3/q3map2/q3map2.h", 191, "MAX_SWITCHED_LIGHTS", 32));
        g.pruefstelle = "q3map2 writebsp.cpp:272 Error \"MAX_SWITCHED_LIGHTS (32) exceeded\" (SetLightStyles)";
        g.rechnung = "verschiedene Paare (targetname, style) der light-Entities mit targetname";
        t.push_back(std::move(g));
    }
    {
        Grenze g = neu("compiler.sicht", GrenzArt::Compiler, Str::KeGrCoSicht, "MAX_MAP_VISIBILITY", 8 + 0x4000LL * 0x800LL);
        g.stellen.push_back(q3("tools/quake3/q3map2/q3map2.h", 145, "VIS_HEADER_SIZE", 8));
        g.stellen.push_back(q3("tools/quake3/q3map2/q3map2.h", 209, "MAX_MAP_VISCLUSTERS", 0x4000));
        g.stellen.push_back(q3("tools/quake3/q3map2/q3map2.h", 210, "MAX_MAP_VISIBILITY", 8 + 0x4000LL * 0x800LL));
        g.pruefstelle = "q3map2 vis.cpp:733 Error \"MAX_MAP_VISIBILITY exceeded\"";
        g.rechnung = "Bytes der Sichtbarkeitsdaten";
        t.push_back(std::move(g));
    }
    return t;
}

std::array<std::vector<Grenze>, kGrenzSpiele>& tabellen() {
    static std::array<std::vector<Grenze>, kGrenzSpiele> t{baue(GrenzSpiel::JkaSp), baue(GrenzSpiel::JkaMp),
                                                          baue(GrenzSpiel::MovieDuels), baue(GrenzSpiel::Jk2)};
    return t;
}

}  // namespace

const char* grenzSpielName(GrenzSpiel s) {
    switch (s) {
    case GrenzSpiel::JkaSp: return "Jedi Academy (OpenJK SP)";
    case GrenzSpiel::JkaMp: return "Jedi Academy MP (OpenJK)";
    case GrenzSpiel::MovieDuels: return "Movie Duels";
    case GrenzSpiel::Jk2: return "Jedi Outcast (OpenJK JK2_MODE)";
    }
    return "";
}

const char* grenzSpielSchluessel(GrenzSpiel s) {
    switch (s) {
    case GrenzSpiel::JkaSp: return "jkasp";
    case GrenzSpiel::JkaMp: return "jkamp";
    case GrenzSpiel::MovieDuels: return "md";
    case GrenzSpiel::Jk2: return "jk2";
    }
    return "";
}

bool grenzSpielAusSchluessel(std::string_view s, GrenzSpiel& out) {
    for (int i = 0; i < kGrenzSpiele; ++i) {
        const auto g = static_cast<GrenzSpiel>(i);
        if (s == grenzSpielSchluessel(g)) {
            out = g;
            return true;
        }
    }
    return false;
}

const char* grenzArtName(GrenzArt a) {
    switch (a) {
    case GrenzArt::BspLump: return "BSP lump";
    case GrenzArt::Laufzeit: return "runtime";
    case GrenzArt::Renderer: return "renderer";
    case GrenzArt::Kollision: return "collision";
    case GrenzArt::Spawn: return "spawn";
    case GrenzArt::Compiler: return "compiler";
    }
    return "";
}

i18n::Str grenzArtTitel(GrenzArt a) {
    switch (a) {
    case GrenzArt::BspLump: return Str::KeGrArtBsp;
    case GrenzArt::Laufzeit: return Str::KeGrArtLaufzeit;
    case GrenzArt::Renderer: return Str::KeGrArtRenderer;
    case GrenzArt::Kollision: return Str::KeGrArtKollision;
    case GrenzArt::Spawn: return Str::KeGrArtSpawn;
    case GrenzArt::Compiler: return Str::KeGrArtCompiler;
    }
    return Str::Count;
}

const std::vector<Grenze>& grenzen(GrenzSpiel s) { return tabellen()[static_cast<std::size_t>(s)]; }

const Grenze* grenzeSuchen(GrenzSpiel s, std::string_view id) {
    for (const Grenze& g : grenzen(s)) {
        if (g.id == id) { return &g; }
    }
    return nullptr;
}

GrenzSpiel grenzSpielFuerRegelwerk(int regelwerkSpiel) {
    if (regelwerkSpiel == kRegelwerkMovieDuels) { return GrenzSpiel::MovieDuels; }
    if (regelwerkSpiel == kRegelwerkJediOutcast) { return GrenzSpiel::Jk2; }
    return GrenzSpiel::JkaSp;
}

}  // namespace bhed::karte
