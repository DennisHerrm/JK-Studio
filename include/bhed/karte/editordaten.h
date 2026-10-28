// editordaten.h - Karten-Editor Stufe 8a Punkt 2: "Editor-Daten behalten"
// (eigene Bauoption, nicht in NetRadiant-Custom; Vorgabe aus wie NRC).
//
// Was eine .bsp verliert (q3map2, NRC 20260114): Lichter ohne targetname
// (writebsp.cpp SetLightStyles), misc_model/_decal/_skybox
// (bspfile_abstract.cpp UnparseEntities; model.cpp AddTriangleModels backt
// das Modell ein), func_group (MoveBrushesToWorld: verschmilzt mit
// worldspawn), die Texturprojektion der Pinsel, detail/structural, Ebenen.
//
// Die Option tut zweierlei:
//   1. -keeplights fuer jede BSP-Stufe (q3map2s eigener Schalter, bsp.cpp:
//      setzt "_keepLights" "1" im worldspawn): die Lichter bleiben im
//      Entity-Lump. Schadlos: SP_light gibt ein Licht ohne targetname beim
//      Spawnen sofort frei (OpenJK code/game/g_misc.cpp:164,
//      codemp/game/g_misc.c:162, codeJK2/game/g_misc.cpp:147); G_Spawn nimmt
//      freigegebene Plaetze waehrend des Kartenstarts sofort wieder
//      (code/game/g_utils.cpp:800 freetime > 2000 - sv.time ist 1000,
//      sv_init.cpp:288; codemp/game/g_utils.c:825 level.startTime + 2000).
//      Lightmaps, Lichtgitter und Geometrie bleiben Byte fuer Byte gleich,
//      nur der Entity-Lump waechst (Test).
//   2. Eine Begleitdatei <karte>.bhed neben der .bsp: die Quelle (.map bzw.
//      .reg) Byte fuer Byte mit einem Fingerabdruck der Geometrie der .bsp.
//      Sie ist selbst eine ladbare .map (der Kopf sind //-Kommentare).
//      "Open .bsp" nimmt sie, wenn der Fingerabdruck passt - dann kommt alles
//      zurueck: misc_model, func_group, Texturprojektion, detail, Ebenen.
//
// Warum misc_model nicht im Entity-Lump: die Engine wuerde sie zwar schadlos
// verwerfen (SP_misc_model ist G_FreeEntity in allen drei Spielen, die
// cgame-Liste in codemp/cgame/cg_spawn.c kennt misc_model nicht), aber
// q3map2 entfernt sie selbst (UnparseEntities, auch bei -light/-onlyents),
// und jeder Dekompilierer (q3map2 -convert) bekaeme jedes Modell doppelt -
// als Entity und als eingebackene Dreiecke; ein Neubau verdoppelte sie.
//
// Portiert aus q3map2 (NetRadiant-Custom 20260114: bspfile_abstract.cpp
// UnparseEntities - welche Entities in den Lump kommen; writebsp.cpp SetLightStyles
// und -keeplights) und aus OpenJK (code/game/g_misc.cpp SP_light, g_utils.cpp
// G_Spawn - was das Spiel mit behaltenen Lichtern macht).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2001 - 2013, Activision, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//   Copyright (C) 2026 BehavEd-Remake contributors.
//
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. This program is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#ifndef BHED_KARTE_EDITORDATEN_H
#define BHED_KARTE_EDITORDATEN_H

#include <cstdint>
#include <string>
#include <vector>

namespace bhed::karte {

struct BspDatei;

// Fingerabdruck der Geometrie einer .bsp: die Lumps planes, nodes, leafs,
// leafsurfaces, leafbrushes, models, brushes, brushsides (2..9 in IBSP und
// RBSP) - sie entstehen in -bsp und bleiben bei -vis, -light und -onlyents.
[[nodiscard]] std::uint64_t bspGeometrieFingerabdruck(const BspDatei& b);

// "<Pfad ohne Endung>.bhed"
[[nodiscard]] std::string editorDatenPfad(const std::string& bsp);

// Die Begleitdatei: Kopf (drei //-Zeilen) und der Text der Quelle.
[[nodiscard]] std::string editorDatenSchreiben(const std::string& mapText, const std::string& bspDaten);

enum class EditorDatenStand { Passt, AndereBsp, Kaputt };
// Prueft die Begleitdatei gegen die .bsp; bei Passt steht die Quelle in mapText.
[[nodiscard]] EditorDatenStand editorDatenLesen(const std::string& begleit, const std::string& bspDaten, std::string& mapText);

// Ein Befehl der BSP-Stufe (bauStufeAusBefehl == Bsp, auch -onlyents) bekommt
// -keeplights vor dem letzten Argument (der Karte); andere Stufen und Befehle,
// die es schon haben, bleiben.
[[nodiscard]] std::string befehlMitLichtern(const std::string& befehl);
// Stufe 8a Punkt 5a: ein -vis-Befehl bekommt -bhedvisreuse (bhed_q3map2: die Sichtbarkeit
// aus <karte>.bhedvis, wenn .prt und Schalter gleich sind)
[[nodiscard]] std::string befehlMitVisWiederverwendung(const std::string& befehl);
// Stufe 8a Punkt 5b/5d: ein -light-Befehl bekommt -bhedlightcache (bhed_q3map2: Lightmaps,
// Vertexlicht und Lichtgitter aus <karte>.bhedlight, wo sie sich nicht aendern koennen)
[[nodiscard]] std::string befehlMitLichtWiederverwendung(const std::string& befehl);

// Stufe 8g: GPU-Backen. Stufe 8d (shank: "Das Backen soll automatisch die schnellere Variante
// nehmen, CPU oder GPU"): Build > "GPU bake" hat drei Werte, Vorgabe Auto - bhed_q3map2 waehlt mit
// -bhedgpubakeauto die schnellere Variante (bhed/karte/gpuwahl.h), die GPU nur, wenn sie double kann
// (bitgleich). Die Entscheidung, was ohne gespeicherte Einstellung gilt, ist nur diese Konstante.
enum class GpuBacken { Aus, Ein, Auto };
inline constexpr GpuBacken kGpuBackenVorgabe = GpuBacken::Auto;
// Einstellung "BhedGpuBakeMode": "off" / "on" / "auto" (bis Stufe 8g "BhedGpuBake" 0/1)
[[nodiscard]] const char* gpuBackenName(GpuBacken g);
[[nodiscard]] GpuBacken gpuBackenAusName(const std::string& s, GpuBacken sonst);
// ein -light-Befehl bekommt -bhedgpubakeauto -bhedgpubakezeiten <cpuMs> <gpuMs> (die gemerkten
// Zeiten des letzten CPU- und GPU-Laufs dieser Karte mit diesen Schaltern, 0 = unbekannt)
[[nodiscard]] std::string befehlMitGpuBackenAuto(const std::string& befehl, long long cpuMs, long long gpuMs);
// Schluessel der gemerkten Zeiten: die Schalter eines -light-Befehls ab "-light" ohne die Karte
// und ohne die Schalter der GPU-Wahl (-bhedgpubake*, -bhedgpuadapter); leer ohne -light
[[nodiscard]] std::string lichtSchalter(const std::string& befehl);
// Die gemerkten Zeiten (Datei gpuzeiten.txt im Einstellungsordner): je Karte und Licht-Schaltern
// die Dauer des letzten -light-Laufs auf der CPU und auf der GPU in ms
struct GpuZeiten {
    long long cpuMs = 0, gpuMs = 0;
};
class GpuZeitenSpeicher {
public:
    bool lesen(const std::string& text);
    [[nodiscard]] std::string schreiben() const;
    [[nodiscard]] GpuZeiten holen(const std::string& karte, const std::string& schalter) const;
    void merken(const std::string& karte, const std::string& schalter, bool gpu, long long ms);
    [[nodiscard]] std::size_t groesse() const { return eintraege_.size(); }

private:
    struct Eintrag {
        std::string karte, schalter;
        GpuZeiten z;
    };
    std::vector<Eintrag> eintraege_;   // zuletzt benutzt am Ende; hoechstens kMax
    static constexpr std::size_t kMax = 500;
};
// ein -light-Befehl bekommt -bhedgpubake (bhed_q3map2: die ganze Lichtrechnung auf der GPU,
// Byte-gleich mit der CPU)
[[nodiscard]] std::string befehlMitGpuBacken(const std::string& befehl);
// Was bhed_q3map2 -bhedgpubake ueber sich ausgibt (die Zeilen "BehavEd GPU bake: ..."):
// die GPU und der Abweichungsbericht
struct GpuBackBericht {
    std::string adapter;             // "BehavEd GPU bake: <adapter> (feature level ..."
    bool keineGpu = false;           // "no usable GPU - the CPU computes everything"
    bool kopf = false;               // Zeile "... rounds, N lightmaps on the GPU ..." gesehen
    long long gpuLm = 0, cpuLm = 0;
    bool bericht = false;            // Zeile "BehavEd GPU bake report: ..." gesehen
    long long luxel = 0, lms = 0, lmsAnders = 0, gitterGpu = 0, gitterCpu = 0;
    std::string gleich;              // Anteil gleicher Luxel in %, wie ausgegeben ("100.0000")
    // Stufe 8d: "BehavEd GPU bake auto: GPU (large) - 1286512 super luxels, ..."
    bool autoGesehen = false, autoGpu = false;
    std::string autoGrund;           // Kurzname (gpuGrundName): nogpu, nodouble, learnedgpu, ...
    long long autoLuxel = 0;
};
// eine Ausgabezeile von bhed_q3map2 auswerten; true: sie gehoerte zum GPU-Backen
bool gpuBackZeile(const std::string& zeile, GpuBackBericht& b);

}  // namespace bhed::karte

#endif
