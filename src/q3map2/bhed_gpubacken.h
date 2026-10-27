// bhed_gpubacken.h - BehavEd Stufe 8c Punkt 2: GPU-Backen fuer q3map2 -light,
// Schalter -bhedgpubake (Vorgabe aus, nicht im Build-Menue als Standard).
//
// Anders als -bhedgpu (Stufe 8b: nur die Entscheidung "verdeckt?" der ersten
// Abtastung, Byte-gleich) rechnet hier die GPU den ganzen Lichtbeitrag eines
// Luxels zu einem Licht - LightContributionToSample samt Strahl - fuer die erste
// Abtastung jedes Lichts in IlluminateRawLightmap, in jeder Runde (auch den
// Bounce-Runden mit ihren Flaechenlichtern). Supersampling-Raster (-super) sind
// schon die Luxel der Roh-Lightmap; das adaptive Nachabtasten (-samples), der
// Filter (-filter), Lichtgitter und Nachbearbeitung bleiben auf der CPU.
//
// Nicht Byte-gleich, aber so nah wie moeglich: dieselben Formeln in derselben
// Reihenfolge, jede Rechnung "precise" (keine Zusammenfassung zu mad), die
// float-Division genau gerundet in Ganzzahlen, VectorNormalize, sqrt und acos von
// q3map2 in double (wo die GPU double kann: genau gerundete Wurzel nach
// Tuckerman, acos nach fdlibm), dieselbe Traversierung samt Epsilons wie
// TraceLine_r/TraceTriangle. Unterschiede bleiben: fp32-Denormale (D3D11 setzt
// sie auf null), acos (fdlibm gegen UCRT, je hoechstens 1 ULP in double) und GPUs
// ohne double (dann float-Naeherungen). Alpha-Schatten und Lichtfilter rechnet
// die CPU (Status "CPU").
//
// Nach dem Backen ein Abweichungsbericht: eine feste Stichprobe der Paare rechnet
// die CPU nach; getrennt gezaehlt werden andere Treffer-Entscheidungen
// (verdeckt/frei) und Abweichungen in der Rechnung.
//
// Direct3D 11 wird zur Laufzeit geladen (d3d11.dll, dxgi.dll, Teil von Windows
// 10/11): bhed_q3map2.exe bindet keine neue DLL fest. Die Shader sind seit Stufe 8g
// im Bau vorab uebersetzt (bhed_gpubacken.hlsl, fxc) - kein d3dcompiler_47.dll. Kurze
// Dispatches (TDR), Geraeteverlust oder Speichermangel: der Rest rechnet die CPU.
//
//   Copyright (C) 1999-2007 id Software, Inc. and contributors (TraceLine,
//   TraceTriangle, LightContributionToSample, PointToPolygonFormFactor aus
//   q3map2, als HLSL nachgebaut); fdlibm acos: Copyright (C) 1993 by Sun
//   Microsystems, Inc. ("Permission to use, copy, modify, and distribute this
//   software is freely granted, provided that this notice is preserved").
//   Copyright (C) 2026 BehavEd-Remake contributors.
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version (LICENSE).
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct BhedBakeKnoten {     // traceNode_t (mit numItems auch fuer innere Knoten: testAll)
	int typ;
	float n[ 3 ];
	float dist;
	int kind[ 2 ];
	int anzahl;             // numItems
	int erstes;             // Blaetter: erstes Element in der Liste
	int pad;
};
struct BhedBakeDreieck {
	float v0[ 3 ], e1[ 3 ], e2[ 3 ];
	int info;
};
struct BhedBakeInfo {
	int cast;
	int skipGrid;
	int flaeche;
	int flags;              // 1 Himmel, 2 Alpha/Filter mit Bild (CPU)
	int skyIndex;
	int pad[ 3 ];
};
struct BhedBakeLicht {
	int typ, flags, cluster, windErste;
	float o[ 3 ];
	float n[ 3 ];
	float dist, photons;
	float c[ 3 ];
	float radiusByDist, fade, angleScale, extraDist, add;
	float envelope, falloffTol;
	int skyIndex, windAnzahl;
};
struct BhedBakeLuxel {
	float o[ 3 ];
	float n[ 3 ];
	int cluster;
	int lm;                 // Index in der Liste der Lightmap-Angaben
};
struct BhedBakeLm {
	int recv, sErste, sAnzahl, twoSided;
};
struct BhedBakePaar {
	int luxel, licht;
};
struct BhedBakeErg {
	float c[ 3 ];
	int code;               // LightContributionToSample: -1 verdeckt, 0 nichts, 1 Licht; 9 = die CPU rechnet
};
struct BhedBakeSchalter {
	int faster, lightAngleHL, bouncing, testOcclusion, oneSky, noSurfaces;
	float linearScale;
};

// Szene einmal hochladen (nach SetupTraceNodes); false: keine GPU.
bool BhedBakeStart( const std::vector<BhedBakeKnoten>& knoten, const std::vector<int>& items, const std::vector<BhedBakeDreieck>& dreiecke,
                    const std::vector<BhedBakeInfo>& infos, const int *flaechen, int anzahlFlaechen, int kopfKnoten,
                    const unsigned char *vis, std::size_t visBytes, const std::string& adapterWahl );
// Lichter einer Runde (alle Lichter der Liste, Windungen der Flaechenlichter).
bool BhedBakeLichter( const std::vector<BhedBakeLicht>& lichter, const std::vector<float>& windungen, const BhedBakeSchalter& s,
                      const std::vector<BhedBakeInfo>& infos );
// Ein Block: Luxel, Lightmap-Angaben und Paare hochladen, rechnen, Ergebnisse lesen.
// false: die GPU ist ausgefallen (alle Ergebnisse code 9, die CPU rechnet).
bool BhedBakeRechnen( const std::vector<BhedBakeLuxel>& luxel, const std::vector<BhedBakeLm>& lms, const std::vector<BhedBakePaar>& paare,
                      std::vector<BhedBakeErg>& erg );
bool BhedBakeBereit();
void BhedBakeEnde();
std::string BhedBakeAdapter();
bool BhedBakeDoppelt();     // die GPU rechnet double (genau gerundete Wurzel, acos)
// Zaehler fuer den Bericht
struct BhedBakeZahlen {
	long long paare = 0, gpuMikros = 0, dispatches = 0, cpuPaare = 0, nichtEndlich = 0, bloecke = 0;
	bool ausgefallen = false;
	std::string grund;
};
BhedBakeZahlen BhedBakeStatistik();

// q3map2-Seite
inline bool g_bhedBakeAn = false;                           // -bhedgpubake und die GPU ist bereit
bool BhedBakeVorbereiten( const std::string& adapter );    // light_trace.cpp: nach SetupTraceNodes
std::vector<BhedBakeInfo> BhedBakeInfos();                 // light_trace.cpp: die Infos (skyIndex entsteht erst in LightWorld)
void BhedBakeBericht( const char *quelle );                // light_ydnar.cpp: nach LightWorld
