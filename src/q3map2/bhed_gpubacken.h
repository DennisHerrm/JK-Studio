// bhed_gpubacken.h - BehavEd Stufe 8c/8g: GPU-Backen fuer q3map2 -light,
// Schalter -bhedgpubake (Vorgabe aus).
//
// Stufe 8g: die ganze Lichtschleife von IlluminateRawLightmap laeuft je Block von
// Roh-Lightmaps auf der GPU, die Daten bleiben zwischen den Schritten dort, zurueck
// kommen nur die fertigen Luxel:
//   1. Erstabtastung (LightContributionToSample) jedes Lichts der Lichtliste fuer jeden
//      abgebildeten Super-Luxel, dazu je Licht die Zahl der beleuchteten Luxel;
//   2. die CPU macht mit diesen Zahlen die Stil-Pruefung in Lichtreihenfolge (Stilplaetze,
//      "nichts beleuchtet -> ueberspringen", Warnungen) - ein paar Byte je Licht;
//   3. Nachabtasten (-samples): Vorhersage der Stempel mit den Werten der Erstabtastung,
//      die vorhergesagten Luxel parallel unterabgetastet (SubsampleRawLuxel_r samt
//      SubmapRawLuxel und ClusterForPointExtFilter), dann je Licht der genaue Stempellauf
//      in Zeilenfolge wie Stufe 7 (BhedLichterParallel);
//   4. je Luxel Filtern und Aufsummieren in die Stil-Lightmaps, die Lichter in ihrer
//      Reihenfolge, Stil 0 ab dem Umgebungslicht.
// Danach setzt die CPU die Luxel ein und macht den Rest von IlluminateRawLightmap wie
// immer (Floodlight, Dirt, Filter der unbeleuchteten Luxel).
//
// Byte-gleich: dieselben float-Operationen in derselben Reihenfolge wie q3map2, jede
// Rechnung "precise" (keine Zusammenfassung zu mad), die float-Division genau gerundet in
// Ganzzahlen, VectorNormalize, sqrt und acos von q3map2 in double (wo die GPU double kann:
// genau gerundete Wurzel nach Tuckerman, acos nach fdlibm), dieselbe Traversierung samt
// Epsilons wie TraceLine_r/TraceTriangle, keine Summen in Ausfuehrungsreihenfolge (nur
// Ganzzahl-Zaehler werden atomar addiert). GPUs ohne double: float-Naeherungen. Was die GPU
// nicht genau kann (Alpha-Schatten, Lichtfilter, unsichere Division, nicht endliche
// Werte), rechnet fuer die ganze Lightmap die CPU.
//
// Abweichungsbericht: jede N-te Lightmap (Vorgabe 16, -bhedgpubakecheck N) rechnet die CPU
// ein zweites Mal und vergleicht alle Luxel Byte fuer Byte; das Ergebnis der GPU bleibt.
//
// Direct3D 11 wird zur Laufzeit geladen (d3d11.dll, dxgi.dll, Teil von Windows
// 10/11): bhed_q3map2.exe bindet keine neue DLL fest. Die Shader sind seit Stufe 8g
// im Bau vorab uebersetzt (bhed_gpubacken.hlsl, fxc) - kein d3dcompiler_47.dll. Kurze
// Dispatches (TDR), Speicherbedarf vorab geschaetzt und in Bloecke geteilt;
// Geraeteverlust oder Speichermangel: der Rest rechnet die CPU.
//
//   Copyright (C) 1999-2007 id Software, Inc. and contributors (TraceLine,
//   TraceTriangle, LightContributionToSample, PointToPolygonFormFactor,
//   SubsampleRawLuxel_r, SubmapRawLuxel, ClusterForPointExt aus q3map2, als HLSL
//   nachgebaut); fdlibm acos: Copyright (C) 1993 by Sun Microsystems, Inc.
//   ("Permission to use, copy, modify, and distribute this software is freely
//   granted, provided that this notice is preserved").
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
	float mins[ 3 ], maxs[ 3 ];   // light->minmax (Lichtgitter)
	int style, pad;
};
// BSP fuer ClusterForPointExt (Unterabtastung)
struct BhedBakeBsp {
	struct Knoten { int ebene, k0, k1, pad; };
	struct Ebene { float n[ 3 ]; float dist; };
	struct Blatt { int cluster, ersterBrush, anzahlBrushes, pad; };
	struct Brush { int ersteSeite, anzahlSeiten, opak, pad; };
	std::vector<Knoten> knoten;
	std::vector<Ebene> ebenen;
	std::vector<Blatt> blaetter;
	std::vector<int> blattBrushes;
	std::vector<Brush> brushes;
	std::vector<int> seiten;
	int maxOpaqueBrush = -1;
};
// ein abgebildeter Super-Luxel (Cluster >= 0)
struct BhedBakeLuxel {
	float o[ 3 ];
	float n[ 3 ];
	int cluster;
	int lm;                 // Nummer der Lightmap im Block
	int x, y;
};
// eine Roh-Lightmap im Block (Lm im Shader)
struct BhedBakeLm {
	int recv, sErste, sAnzahl, twoSided;
	int sw, sh, m, luxelBase;
	int idxBase, lichtBase, nLichter, paarBase;
	int clusterBase, nClusters, aktivBase, nAktiv;
	float ambient[ 3 ];
	int zeilenBase;
	int aus[ 4 ];           // erster Eintrag in R je Stilplatz, -1: kein Licht in diesem Stil
	int start;              // 1: die Stilplaetze beginnen mit startWert (Teil einer grossen Lightmap) statt Umgebung/0
	int pad[ 3 ];
};
// ein Licht der Lightmap nach der Stil-Pruefung, das etwas beleuchtet
struct BhedBakeAktiv {
	int j, slot, radius, negativ;
};
struct BhedBakeSchalter {
	int faster, lightAngleHL, bouncing, testOcclusion, oneSky, noSurfaces;
	float linearScale;
	int lightSamples;
	float suchBox;          // 0.25f * lightSamplesSearchBoxSize
};
// ein Block von Lightmaps fuer die GPU
struct BhedBakeBlock {
	std::vector<BhedBakeLuxel> luxel;
	std::vector<BhedBakeLm> lms;
	std::vector<int> idx;           // je Lightmap sw * sh: Nummer des abgebildeten Luxels oder -1
	std::vector<int> lichtListe;    // je Lightmap die Lichtliste (Nummern in der Lichttafel)
	std::vector<int> lmCluster;     // je Lightmap lightClusters
	std::vector<BhedBakeAktiv> aktiv;
	std::vector<int> aktivNr;       // je Licht der Lightmap: Eintrag in aktiv oder -1
	std::vector<float> startWert;   // Lightmaps mit start: 4 float je Luxel und Stilplatz (wie die Ausgabe)
	std::uint32_t paare = 0, zeilen = 0, aus = 0;
};

// Szene einmal hochladen (nach SetupTraceNodes); false: keine GPU.
bool BhedBakeStart( const std::vector<BhedBakeKnoten>& knoten, const std::vector<int>& items, const std::vector<BhedBakeDreieck>& dreiecke,
                    const std::vector<BhedBakeInfo>& infos, const int *flaechen, int anzahlFlaechen, int kopfKnoten,
                    const unsigned char *vis, std::size_t visBytes, const BhedBakeBsp& bsp, const std::string& adapterWahl );
// Lichter einer Runde (alle Lichter der Liste, Windungen der Flaechenlichter).
bool BhedBakeLichter( const std::vector<BhedBakeLicht>& lichter, const std::vector<float>& windungen, const BhedBakeSchalter& s,
                      const std::vector<BhedBakeInfo>& infos );
// ein Punkt des Lichtgitters (GitterPunkt im Shader)
struct BhedBakeGitterPunkt {
	float o[ 3 ];
	int cluster;
	float dir[ 3 ];
	std::uint32_t stile;    // gp.styles, 4 Byte
	float ambient[ 12 ];
	float directed[ 12 ];
	std::uint32_t neu;      // zurueck: in diesem Lauf vergebene Stilplaetze (Bit 1..3)
	int flag;               // zurueck: 1 = die CPU rechnet diesen Punkt
	int pad[ 2 ];
};
struct BhedBakeGitterSchalter {
	float inhibit;          // trace.inhibitRadius: die halbe groesste Gitterweite
	float ambientDirectionality, directionality;
	int cheapgrid;
};
// Paare, die ein Block hoechstens haben darf (Grafikspeicher vorab geschaetzt)
std::size_t BhedBakeBudgetPaare();
// Schritt 1: Erstabtastung; zaehler je Licht der Lightmaps, flags je Lightmap (1: die CPU rechnet sie).
// false: die GPU ist ausgefallen (der Block rechnet die CPU).
bool BhedBakeErst( const BhedBakeBlock& b, std::vector<std::uint32_t>& zaehler, std::vector<std::uint32_t>& flags );
// Schritt 3 und 4 mit den aktiven Lichtern (b.lms mit aktivBase/nAktiv/zeilenBase/aus): die fertigen Luxel.
bool BhedBakeRest( const BhedBakeBlock& b, std::vector<float>& luxelAus, std::vector<std::uint32_t>& flags );
// TraceGrid fuer die Punkte (in place); false: die GPU ist ausgefallen (die CPU rechnet alle Punkte)
bool BhedBakeGitter( std::vector<BhedBakeGitterPunkt>& punkte, const BhedBakeGitterSchalter& s );
bool BhedBakeBereit();
void BhedBakeEnde();
std::string BhedBakeAdapter();
bool BhedBakeDoppelt();     // die GPU rechnet double (genau gerundete Wurzel, acos)
// Zaehler fuer den Bericht
struct BhedBakeZahlen {
	long long paare = 0, gpuMikros = 0, dispatches = 0, bloecke = 0, budget = 0;
	long long kernMikros[ 6 ] = {};   // BHED_GPUBAKE_ZEITEN: je Kern (erst, kandidaten, unter, stempellauf, filtern, gitter), mit Warten gemessen
	long long gitterPunkte = 0, gitterCpu = 0;
	bool ausgefallen = false;
	std::string grund;
};
BhedBakeZahlen BhedBakeStatistik();

// q3map2-Seite
inline bool g_bhedBakeAn = false;                           // -bhedgpubake und die GPU ist bereit
inline int g_bhedBakePruefen = 16;                          // -bhedgpubakecheck N: jede N-te Lightmap rechnet die CPU nach (0: keine)
bool BhedBakeVorbereiten( const std::string& adapter );    // light_trace.cpp: nach SetupTraceNodes
std::vector<BhedBakeInfo> BhedBakeInfos();                 // light_trace.cpp: die Infos (skyIndex entsteht erst in LightWorld)
void BhedBakeBericht( const char *quelle );                // light_ydnar.cpp: nach LightWorld
// light_ydnar.cpp: die Lichttafel dieser Runde hochladen (nr/zeiger: Nummer je Licht und umgekehrt, duerfen null sein)
bool BhedBakeLichtTafel( void *nr, void *zeiger );
void BhedBakeGitterZaehlen( long long gpu, long long cpu ); // light.cpp: Punkte des Lichtgitters auf GPU/CPU
