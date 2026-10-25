// bobtoolz.h - Karten-Editor, Stufe 4: das Plugin bobToolz von
// NetRadiant-Custom ohne Oberflaeche.
//
// Portiert aus NetRadiant-Custom 20260114, contrib/bobtoolz (die Dateien, die
// NRCs Makefile fuer bobToolz baut, Makefile:1135-1157):
//   bobToolz-GTK.cpp (QERPlug_Dispatch, Menue, Werkzeugleiste), funchandlers-GTK.cpp
//   (Do*), shapes.cpp, DShape.cpp, DBrush.cpp, DPlane.cpp, DPatch.cpp,
//   DEntity.cpp, DMap.cpp, DWinding.cpp, DPoint.cpp, DEPair.cpp, misc.cpp,
//   DTreePlanter.cpp, DTrainDrawer.cpp, DBobView.cpp, DVisDrawer.cpp,
//   visfind.cpp, bsploader.cpp, ScriptParser.cpp, lists.cpp,
//   dialogs/dialogs-gtk.cpp (die Werte der Dialoge: *RS)
// und libs/mathlib (float-Vektoren wie bobToolz).
//
//   BobToolz plugin for GtkRadiant
//   Copyright (C) 2001 Gordon Biggans
//
//   Urspruenglich unter der GNU Lesser General Public License 2.1 (oder
//   spaeter); nach deren Abschnitt 3 hier unter der GNU General Public
//   License, Version 2: This program is free software; you can redistribute
//   it and/or modify it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; version 2 of the License. This
//   program is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//   FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
//   details (LICENSE).
#ifndef BHED_KARTE_BOBTOOLZ_H
#define BHED_KARTE_BOBTOOLZ_H

#include "bhed/karte/mathe.h"

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace bhed::karte {
class Karte;
class Entitaet;
class Patch;
}

namespace bhed::karte::bobtoolz {

// Meldungen wie globalOutputStream / globalWarningStream / globalErrorStream
struct Meldung {
    int art = 0;   // 0 Ausgabe, 1 Warnung, 2 Fehler
    std::string text;
};
using Meldungen = std::vector<Meldung>;

// Was bobToolz von Radiant und aus seinen Dateien holt.
struct Umgebung {
    std::string aktuelleTextur = "textures/common/caulk";   // GetCurrentTexture (TextureBrowser_getSelectedShader)
    std::vector<std::string> ausschluss;          // bt/bt-el1.txt (LoadLists: exclusionList)
    std::vector<std::string> ausschlussFlaeche;   // bt/bt-el2.txt (exclusionList_Face)
    Vektor3f kamera;                              // GlobalRadiant().Camera_getOrigin()
    float raster = 8.0F;                          // GetGridSize
    // rand() / srand( time( nullptr ) ) der C-Laufzeit; im Selbsttest fest
    std::function<int()> zufall;
    std::function<void(unsigned)> zufallStart;
    std::function<long long()> zeit;
    // Bildgroesse einer Textur (IShader::getTexture()->width/height), 0 = unbekannt
    std::function<std::pair<int, int>(const std::string&)> texturGroesse;
};

// bt/*.txt lesen (lists.cpp LoadExclusionList / LoadListStore): eine Zeile je Eintrag
[[nodiscard]] std::vector<std::string> listeLesen(const std::string& text);

// --- die Werte der Dialoge (dialogs/dialogs-gtk.h) -------------------------
enum Richtung { MOVE_NORTH = 0, MOVE_SOUTH = 1, MOVE_EAST = 2, MOVE_WEST = 3 };
enum TreppenStil { STYLE_ORIGINAL = 0, STYLE_BOB = 1, STYLE_CORNER = 2 };
enum TuerRichtung { DIRECTION_NS = 0, DIRECTION_EW = 1 };

struct IntersectRS {                 // DoIntersectBox
    bool ganzeKarte = true;          // nBrushOptions == BRUSH_OPT_WHOLE_MAP
    bool detail = false;             // bUseDetail
    bool nurDuplikate = false;       // bDuplicateOnly
};
struct PolygonRS {                   // DoPolygonBox
    int seiten = 6;                  // nSides (3..128)
    int randBreite = 16;             // nBorderWidth (8..256)
    bool rand = false;               // bUseBorder
    bool invers = false;             // bInverse
    bool obenAusrichten = false;     // bAlignTop
};
struct BuildStairsRS {               // DoBuildStairsBox
    int stufenHoehe = 8;             // stairHeight (1..1024)
    int richtung = MOVE_NORTH;       // direction
    int stil = STYLE_ORIGINAL;       // style
    bool detail = false;             // bUseDetail
    std::string hauptTextur;         // mainTexture (Vorgabe: GetCurrentTexture)
    std::string stirnTextur;         // riserTexture
};
struct DoorRS {                      // DoDoorsBox
    std::string hauptTextur;         // mainTexture (ohne "textures/")
    std::string randTextur;          // trimTexture
    bool hauptH = true, hauptV = true, randH = true, randV = false;   // bScaleMainH/V, bScaleTrimH/V
    int ausrichtung = DIRECTION_NS;  // nOrientation
};
struct ApertureDoorRS {              // DoApertureDoorsBox, Vorgaben wie dialogs-gtk.h:95-136
    bool ausWicklung = false;        // fromWinding
    int segmente = 16;               // segments (3..256)
    bool startVersetzt = false;      // offsetStartAngle
    std::string hauptTextur = "textures/base_floor/diamond2c";      // textureMain
    std::string randTextur = "textures/base_trim/yellow_rustbx128";  // textureTrim
    enum class Innen { keine = 0, segmentiert = 1, schraeg = 2 } innen = Innen::keine;   // innerType
    std::string innenHauptTextur = "textures/base_floor/clang_floor3blava";   // innerTextureMain
    std::string innenRandTextur = "textures/base_floor/achtung_clang";        // innerTextureTrim
    double innenTiefe1 = 4, innenTiefe2 = 8;       // innerDepth1/2
    bool schraegeSegmente = false;   // slopedSegments
    bool schraegRund = false;        // slopedSegmentsRoundize
    double schraegTiefe1 = 16, schraegTiefe2 = 8;  // slopedDepth1/2
    enum class Stille { keine = 0, pinsel = 1 } stille = Stille::keine;      // silenceType
    Vektor3d stilleVersatz{4096, 0, 0};            // silenceBrushesOffset
    bool tempoNutzen = false;        // speedUse (sonst time)
    double zeit = 2;                 // time
    double tempo = 40;               // speed
    bool distanzSetzen = false;      // distanceSet (sonst aus dem Radius)
    double distanz = 256;            // distance
    double oeffnungsWinkel = 0;      // openAngle
    bool gesundheit = true;          // health
    double warten = .25;             // wait
};
struct PathPlotterRS {               // DoPathPlotterBox
    int punkte = 50;                 // nPoints (1..200)
    float faktor = 3;                // fMultiplier (1..10)
    float schwerkraft = -800;        // fGravity (-10000..-1)
    bool keineAktualisierung = false;   // bNoUpdate
    bool hilfslinien = false;        // bShowExtra
};
struct ResetTextureRS {              // DoResetTextureBox (static: bleibt zwischen den Aufrufen)
    bool nameErsetzen = false;       // bResetTextureName
    std::string alterName, neuerName;   // textureName, newTextureName
    bool skalaSetzen[2] = {false, false};   // bResetScale
    float skala[2] = {0.5F, 0.5F};          // fScale
    bool verschiebungSetzen[2] = {false, false};   // bResetShift
    float verschiebung[2] = {0, 0};                // fShift
    bool drehungSetzen = false;      // bResetRotation
    int drehung = 0;                 // rotation
};
struct MakeChainRS {                 // DoMakeChainBox
    int glieder = 1;                 // linkNum
    std::string name;                // linkName
};

// --- die Befehle (QERPlug_Dispatch -> Do*) ----------------------------------
// Jeder Befehl arbeitet an der Auswahl bzw. der ganzen Karte, mit einem
// Undo-Schritt "bobToolz.*" wie bei NRC, und liefert die Meldungen.
Meldungen intersect(Karte& k, const IntersectRS& rs, const Umgebung& u);          // DoIntersect
Meldungen findDuplicates(Karte& k);                                                // DoFindDuplicates
Meldungen polygonBuilder(Karte& k, const PolygonRS& rs, const Umgebung& u);       // DoPolygons
[[nodiscard]] Meldungen polygonPruefen(const Karte& k);                            // DoPolygons vor dem Dialog
Meldungen brushCleanup(Karte& k);                                                  // DoFixBrushes
Meldungen resetTextures(Karte& k, const ResetTextureRS& rs, bool alle);           // DoResetTextures (eIDOK / eIDYES)
Meldungen stairBuilder(Karte& k, const BuildStairsRS& rs);                         // DoBuildStairs
[[nodiscard]] Meldungen stairPruefen(const Karte& k);
Meldungen doorBuilder(Karte& k, const DoorRS& rs, const Umgebung& u);             // DoBuildDoors
[[nodiscard]] Meldungen doorPruefen(const Karte& k);
Meldungen apertureDoor(Karte& k, const ApertureDoorRS& rs, const Umgebung& u);    // DoBuildApertureDoors
[[nodiscard]] Meldungen aperturePruefen(const Karte& k);
Meldungen pitOMatic(Karte& k, const Umgebung& u);                                  // DoPitBuilder
Meldungen caulkSelection(Karte& k);                                                // DoCaulkSelection
Meldungen explodeEntity(Karte& k);                                                 // DoExplodeEntity
Meldungen makeChain(Karte& k, const MakeChainRS& rs);                              // DoMakeChain
Meldungen mergePatches(Karte& k);                                                  // DoMergePatches
Meldungen splitPatch(Karte& k, int art);   // 0 Split patch, 1 Split patch columns, 2 Split patch rows
Meldungen flipTerrain(Karte& k);                                                   // DoFlipTerrain

// Path Plotter (DBobView): die Bahn eines trigger_push zu seinem Ziel.
struct Bahn {
    bool aktiv = false;
    std::vector<Vektor3f> punkte;    // path[ nPathCount ]
    bool hilfslinien = false;        // m_bShowExtra: dieselbe Linie um die vier Kastenversaetze
    bool aktualisieren = true;       // !bNoUpdate: folgt dem Ziel
    std::string zielName;            // targetName
    PathPlotterRS rs;
};
// DBobView_setEntity mit DBobView::Begin; bahn wird gesetzt oder geleert.
Meldungen pathPlotter(const Karte& k, const PathPlotterRS& rs, Bahn& bahn);
// DBobView::UpdatePath (nach jeder Aenderung des Ziels)
bool bahnAktualisieren(const Karte& k, Bahn& bahn);
// Die vier Versaetze der Hilfslinien (g_transform_box1..4)
[[nodiscard]] std::array<Vektor3f, 4> bahnKastenVersaetze();

// Plot Splines (DTrainDrawer): die Splines aus info_train_spline_main/_control.
struct Splines {
    bool anzeigen = false;           // m_bDisplay
    std::vector<std::vector<Vektor3f>> linien;   // je Spline m_vertexList
};
Meldungen splinesBauen(const Karte& k, Splines& s);

// Tree Planter (DTreePlanter): an, Klick in die XY-Ansicht, Drop Entity.
struct TreePlanter {
    bool aktiv = false;
    std::vector<std::shared_ptr<void>> intern;   // geladene Pinsel (DEntity m_world)
    std::vector<std::string> modelle;            // m_trees
    int versatz = 0;                             // m_offset
    int pitchMin = 0, pitchMax = 0, yawMin = 0, yawMax = 0;
    std::string klasse;                          // m_entType
    std::string verknuepfung;                    // m_linkName
    int verknuepfungen = 0;                      // m_linkNum
    float skalaMin = 0, skalaMax = 0;
    bool skalaNutzen = false, winkelSetzen = false, autoVerknuepfen = false;
};
// DTreePlanter::DTreePlanter: die gewaehlten Pinsel und bt/tp_ent.txt
Meldungen treePlanterStarten(const Karte& k, const std::string& konfiguration, TreePlanter& tp);
// DTreePlanter::mouseDown (nur M1 in der XY-Ansicht): true, wenn gepflanzt.
bool treePlanterKlick(Karte& k, TreePlanter& tp, const Vektor3f& weltPunkt, const Umgebung& u, Meldungen& m);
// DoDropEnts / DTreePlanter::DropEntsToGround
Meldungen dropEntities(Karte& k, const TreePlanter* tp);

// Vis Viewer (DVisDrawer, visfind.cpp, bsploader.cpp)
struct VisBlatt {
    int blatt = 0, blaetter = 0, flaechen = 0, shader = 0;   // ui_leaf_add
};
struct VisFlaeche {                  // DMetaSurf
    std::vector<Vektor3f> ecken;
    std::vector<unsigned> indizes;
    Vektor3f farbe;
};
struct VisAnsicht {
    bool offen = false;
    std::string bsp;
    std::vector<VisBlatt> liste;     // die Tabelle "BSP leafs"
    std::vector<VisFlaeche> flaechen;   // BuildTrace
    std::vector<std::vector<Vektor3f>> wicklungen;   // BuildLeafWindings
    bool farbeJeFlaeche = false;     // m_colorPerSurf
    int gewaehlt = -1;               // m_table->currentRow(): das Blatt
    std::shared_ptr<void> daten;     // die geladene .bsp
};
// SetupVisView( <Karte>.bsp, Ursprung ): die Blaetter, die von dort sichtbar sind
Meldungen visAnalyse(const std::string& bspPfad, const Vektor3f& ursprung, VisAnsicht& v);
// currentCellChanged: die Flaechen und Wicklungen eines Blatts
void visBlattWaehlen(VisAnsicht& v, int blatt);

// Die Werkzeugleiste (bobToolz-GTK.cpp:184-279) und das Menue (PLUGIN_COMMANDS)
extern const char* const kPluginBefehle;   // "About...,-,Stair Builder...,..."
extern const char* const kAbout;           // PLUGIN_ABOUT (ohne HTML)

}  // namespace bhed::karte::bobtoolz

#endif
