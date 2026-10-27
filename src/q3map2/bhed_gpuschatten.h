// bhed_gpuschatten.h - BehavEd Stufe 8b Punkt 9: die Schattentests von
// q3map2 -light (TraceLine: trifft der Strahl Luxel -> Licht etwas?) auf der
// GPU, Schalter -bhedgpu (Vorgabe aus). Nur die Entscheidung "verdeckt ja/nein"
// laeuft auf der GPU - dieselbe Traversierung wie TraceLine_r/TraceTriangle,
// jede Rechnung "precise" in derselben Reihenfolge (keine Zusammenfassung zu
// mad), die zwei Divisionen genau gerundet in Ganzzahl-Arithmetik. Die
// Lichtrechnung bleibt auf der CPU. Wo die GPU nicht sicher ist (Alpha-Schatten,
// Lichtfilter, Stapel voll, Ueberlauf), rechnet die CPU wie bisher.
//
// Ablauf in IlluminateRawLightmap (light_ydnar.cpp): die Strahlen der ersten
// Abtastung (ein Strahl je Luxel und Licht) werden in einem Vorlauf aufgenommen
// (TraceLine merkt sich nur die Eingaben), die GPU rechnet sie in einem Rutsch,
// danach laeuft die Schleife wie immer und TraceLine nimmt das Ergebnis - nach
// einem Byte-Vergleich der Eingaben; jede Abweichung schaltet fuer diesen
// Block auf die CPU zurueck.
//
// Direct3D 11 wird zur Laufzeit geladen (LoadLibrary d3d11.dll, Teil von Windows
// 10/11): bhed_q3map2.exe bindet keine neue DLL fest (ctest q3map2_importe). Der
// Shader ist seit Stufe 8g im Bau vorab uebersetzt (bhed_gpuschatten.hlsl, fxc). Ohne GPU: die CPU wie immer.
//
//   Copyright (C) 1999-2007 id Software, Inc. and contributors (TraceLine,
//   TraceTriangle aus q3map2/light_trace.cpp, als HLSL nachgebaut).
//   Copyright (C) 2026 BehavEd-Remake contributors.
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version (LICENSE).
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

struct BhedGpuKnoten {      // traceNode_t
	int typ;                // TRACE_LEAF -1, TRACE_LEAF_SOLID -2, ePlaneX/Y/Z 0..2, sonst schraeg
	float n[ 3 ];
	float dist;
	int kind[ 2 ];
	int anzahl;             // numItems
	int erstes;             // erstes Element in der Liste der Dreiecke
};
struct BhedGpuDreieck {     // traceTriangle_t: v[0].xyz, edge1, edge2, infoNum
	float v0[ 3 ], e1[ 3 ], e2[ 3 ];
	int info;
};
struct BhedGpuInfo {        // traceInfo_t
	int cast;               // castShadows
	int skipGrid;
	int flaeche;            // surfaceNum
	int flags;              // 1 Himmel (C_SKY), 2 Alpha/Filter mit Bild (nur die CPU)
};
struct BhedGpuStrahl {      // die Eingaben von TraceLine, die die Entscheidung bestimmen
	float o[ 3 ], e[ 3 ], d[ 3 ];
	float dist, inhibit;
	int recv, sErste, sAnzahl, flags;   // flags: 1 inGrid, 2 noSurfaces
	int rest;               // 0; 1 = nicht geeignet (testAll, kein Schatten): die GPU antwortet "CPU"
};
struct BhedGpuErgebnis {
	int status;             // 0 frei, 1 verdeckt (Dreieck, tiefe), 2 fest (passSolid, h), 3 CPU
	float tiefe;
	float h[ 3 ];
	int p[ 3 ];
};

// Ein Block aufgenommener Strahlen je Faden (light_ydnar.cpp / light_trace.cpp).
struct BhedGpuSitzung {
	int modus = 0;          // 0 aus, 1 aufnehmen, 2 wiedergeben
	std::vector<BhedGpuStrahl> strahlen;
	std::vector<unsigned char> geeignet;
	std::vector<BhedGpuErgebnis> erg;
	std::size_t pos = 0;
	bool kaputt = false;
};
inline thread_local BhedGpuSitzung *g_bhedGpuSitzung = nullptr;
inline bool g_bhedGpu = false;   // -bhedgpu und die GPU ist bereit

// Szene hochladen; false: keine GPU (die CPU rechnet wie immer).
bool BhedGpuStart( const std::vector<BhedGpuKnoten>& knoten, const std::vector<int>& items, const std::vector<BhedGpuDreieck>& dreiecke,
                   const std::vector<BhedGpuInfo>& infos, const int *flaechen, int anzahlFlaechen, int kopfKnoten, const float konstanten[ 5 ] );
// konstanten: BARY_EPSILON, 1.0f + BARY_EPSILON, COPLANAR_EPSILON, SELF_SHADOW_EPSILON, TRACE_ON_EPSILON
// Die geeigneten Strahlen einer Sitzung rechnen (mehrere Faeden: nacheinander);
// die anderen bekommen Status 3.
void BhedGpuRechnen( BhedGpuSitzung& s );
void BhedGpuEnde();     // Zahlen ausgeben, Geraet frei
void BhedGpuZaehlen( int status, bool abweichung );   // aus der Wiedergabe
void BhedGpuVorbereiten();   // light_trace.cpp: nach SetupTraceNodes
// Darf ein Faden einen neuen Block aufnehmen? (hoechstens einer rechnet, einer wartet -
// sonst rechnet der Faden diesen Block selbst; die GPU ist ein zusaetzlicher Arbeiter)
bool BhedGpuFrei();
void BhedGpuBelegt();   // ein Block blieb bei der CPU (Zaehler)
