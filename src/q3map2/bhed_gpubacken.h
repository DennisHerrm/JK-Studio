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
//      in Zeilenfolge wie Stufe 7 (BhedLichterParallel); vis2: mit -randomsamples die zufaelligen
//      Nachproben im Kern zufall (eine Gruppe je Lightmap: der serielle Stempellauf, die Proben eines
//      Luxels auf allen Faeden, die Zufallszahlen aus dem Generator der Lightmap wie BhedZufallsProben);
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
// Abweichungsbericht: jede N-te Lightmap (Vorgabe 64, bis 1 M Paare; -bhedgpubakecheck N) rechnet die CPU
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

#include <atomic>
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
	struct Brush { int ersteSeite, anzahlSeiten, opak, pad; float lo[ 3 ], pad1, hi[ 3 ], pad2; };   // lo/hi: Box (vis4)
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
	unsigned int seed;      // vis2: Zufallsgenerator der Lightmap (-randomsamples, Kern zufall)
	int pad[ 2 ];
};
// ein Licht der Lightmap nach der Stil-Pruefung, das etwas beleuchtet
struct BhedBakeAktiv {
	int j, slot, radius, negativ;
	int maske;              // erstes Bit der Stempelmaske dieses Lichts (Vorhersage der Unterabtastung)
	int pad[ 3 ];
};
struct BhedBakeSchalter {
	int faster, lightAngleHL, bouncing, testOcclusion, oneSky, noSurfaces;
	float linearScale;
	int lightSamples;
	float suchBox;          // 0.25f * lightSamplesSearchBoxSize
	int zufall;             // vis1: -randomsamples (BhedBakeRest; vis2: zufaellige Nachproben im Kern zufall)
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
	std::uint32_t paare = 0, zeilen = 0, aus = 0, maskeBits = 0;
};

// Geraet und Shader schon beim Einlesen der Schalter in einem eigenen Faden anlegen (laeuft neben dem
// Laden der Karte; BhedBakeStart wartet darauf)
void BhedBakeGeraetVorab( const std::string& adapterWahl );
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
// vis2 (-randomsamples): GaussLikeRandom als Tafeln (je 15-Bit-Zufallswert x und y vor dem Radius, dann der
// Radius; 3 * 32768) und die Spruenge des Generators (je Probe b Multiplikator und Summand fuer 2 b Zuege, dazu
// am Ende fuer 2 lightSamples Zuege) - aus dem Code der CPU, light_ydnar.cpp
bool BhedBakeZufallTafel( const std::vector<float>& tafel, const std::vector<std::uint32_t>& sprung );
// Paare, die ein Block hoechstens haben darf (Grafikspeicher vorab geschaetzt)
std::size_t BhedBakeBudgetPaare();
// Schritt 1: Erstabtastung; zaehler je Licht der Lightmaps, flags je Lightmap (1: die CPU rechnet sie).
// false: die GPU ist ausgefallen (der Block rechnet die CPU).
bool BhedBakeErst( const BhedBakeBlock& b, std::vector<std::uint32_t>& zaehler, std::vector<std::uint32_t>& flags );
// Schritt 3 und 4 mit den aktiven Lichtern (b.lms mit aktivBase/nAktiv/zeilenBase/aus): die fertigen Luxel.
// -randomsamples, die Lightmaps mit zufaelligen Nachproben (ausser Teilen grosser Lightmaps, die tragen immer 2):
//   Cpu     - sie tragen in flags 2 (die CPU rechnet sie);
//   Gpu     - der Kern zufall rechnet sie (vis2);
//   Sammeln - sie tragen in flags 4: ihre Farben bleiben in P, bis BhedBakeSammeln sie in den Sammelpuffer
//             kopiert (vis4) - der Kern zufall rechnet sie spaeter mit denen anderer Bloecke zusammen
enum class BhedZufallWeg { Cpu, Gpu, Sammeln };
bool BhedBakeRest( const BhedBakeBlock& b, std::vector<float>& luxelAus, std::vector<std::uint32_t>& flags, BhedZufallWeg weg );
// vis4: der Sammelpuffer - die Farben der Erstabtastung (P) der aktiven Lichter von Lightmaps mit zufaelligen
// Nachproben aus mehreren Bloecken, damit der Kern zufall (eine Gruppe je Lightmap, latenzgebunden) genug Gruppen
// gleichzeitig hat. Plaetze in Paaren (0: kein Speicher dafuer).
std::size_t BhedBakeSammelPaare();
// P[ von, von + anzahl ) des letzten Blocks (nach BhedBakeRest) an die Stelle ziel des Sammelpuffers
bool BhedBakeSammeln( std::uint32_t von, std::uint32_t ziel, std::uint32_t anzahl );
// ein Block aus gesammelten Lightmaps (paarBase = ihre Stelle im Sammelpuffer, nur ihre aktiven Lichter): die
// Vorhersage, der Kern zufall und das Filtern wie BhedBakeRest mit Gpu - dieselben Rechnungen wie in ihrem Block
bool BhedBakeSammelRechnen( const BhedBakeBlock& b, std::vector<float>& luxelAus, std::vector<std::uint32_t>& flags );
// TraceGrid fuer die Punkte (in place); false: die GPU ist ausgefallen (die CPU rechnet alle Punkte).
// vis1: halt (darf nullptr sein) wird vor jedem Dispatch gelesen - gesetzt, hoert die GPU auf, *angehalten
// wird true und die Punkte sind nicht gerechnet (die CPU hat sie inzwischen alle genommen).
bool BhedBakeGitter( std::vector<BhedBakeGitterPunkt>& punkte, const BhedBakeGitterSchalter& s, const std::atomic<bool> *halt = nullptr,
                     bool *angehalten = nullptr );
bool BhedBakeBereit();
void BhedBakeEnde();
std::string BhedBakeAdapter();
bool BhedBakeDoppelt();     // die GPU rechnet double (genau gerundete Wurzel, acos)
// Zaehler fuer den Bericht
struct BhedBakeZahlen {
	long long paare = 0, gpuMikros = 0, dispatches = 0, bloecke = 0, budget = 0;
	long long kernMikros[ 7 ] = {};   // BHED_GPUBAKE_ZEITEN: je Kern (erst, kandidaten, unter, stempellauf, filtern, gitter, zufall), mit Warten gemessen
	long long zufallLm = 0;           // vis2: Lightmaps mit zufaelligen Nachproben auf der GPU
	long long zufallLuxel = 0;        // vis2: davon nachabgetastete Luxel
	long long sammelLaeufe = 0;       // vis4: Laeufe des Kerns zufall ueber gesammelte Lightmaps
	long long gitterPunkte = 0, gitterCpu = 0;
	long long kandidaten[ 4 ] = {};   // BHED_GPUBAKE_ZEITEN: neue Kandidaten je Vorhersage-Runde
	bool ausgefallen = false;
	std::string grund;
};
BhedBakeZahlen BhedBakeStatistik();

// q3map2-Seite
inline bool g_bhedBakeAn = false;                           // -bhedgpubake und die GPU ist bereit
inline int g_bhedBakePruefen = 64;                          // -bhedgpubakecheck N: jede N-te Lightmap rechnet die CPU nach (0: keine)
bool BhedBakeVorbereiten( const std::string& adapter );    // light_trace.cpp: nach SetupTraceNodes
// Stufe 8d (-bhedgpubakeauto): wartet auf den Geraetestart; 0 keine GPU, 1 GPU ohne double, 2 mit double
int BhedBakeGeraetArt( const std::string& adapter );
std::vector<BhedBakeInfo> BhedBakeInfos();                 // light_trace.cpp: die Infos (skyIndex entsteht erst in LightWorld)
void BhedBakeBericht( const char *quelle );                // light_ydnar.cpp: nach LightWorld
// light_ydnar.cpp: die Lichttafel dieser Runde hochladen (nr/zeiger: Nummer je Licht und umgekehrt, duerfen null sein)
bool BhedBakeLichtTafel( void *nr, void *zeiger );
void BhedBakeGitterZaehlen( long long gpu, long long cpu ); // light.cpp: Punkte des Lichtgitters auf GPU/CPU
