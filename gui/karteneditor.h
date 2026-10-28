// karteneditor.h - der Arbeitsbereich "Karten-Editor" (Stufe 1b): Zustand und
// Schnittstelle zwischen den Teilen der Oberflaeche.
//
// Vorlage ist NetRadiant-Custom (NRC, Stand master 2026, GPL v2): Layout,
// Menues, Werkzeugleisten, Tastenkuerzel und Verhalten werden 1:1
// nachgebaut; die Quellstellen stehen in den Kommentaren als datei:zeile.
// Der Kartenkern (bhed::karte, src/karte) stammt aus Stufe 1a.
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
#ifndef BHED_GUI_KARTENEDITOR_H
#define BHED_GUI_KARTENEDITOR_H

#include "bhed/i18n.h"
#include "bhed/karte/prefabverbund.h"
#include "bhed/karte/wiederherstellen.h"
#include "bhed/karte/gelaende.h"
#include "bhed/karte/kurven.h"
#include "bhed/karte/npcwege.h"
#include "bhed/karte/sprungpruefung.h"
#include "bhed/karte/geometrieimport.h"
#include "bhed/karte/bau.h"
#include "bhed/karte/baufortschritt.h"
#include "bhed/karte/bobtoolz.h"
#include "bhed/karte/brushexport.h"
#include "bhed/karte/kleineplugins.h"
#include "bhed/karte/meshtex.h"
#include "bhed/karte/terraingen.h"
#include "bhed/karte/spielkollision.h"
#include "bhed/karte/logik.h"
#include "bhed/navlauf.h"
#include "bhed/karte/bspoeffnen.h"
#include "bhed/karte/editordaten.h"
#include "bhed/karte/baumonitor.h"
#include "bhed/karte/portale.h"
#include "bhed/karte/bearbeitung.h"
#include "bhed/karte/befehle.h"
#include "bhed/karte/eklasse.h"
#include "bhed/karte/entitaetbefehle.h"
#include "bhed/karte/grundformen.h"
#include "bhed/karte/texturbefehle.h"
#include "bhed/karte/texturtags.h"
#include "bhed/karte/kartendatei.h"
#include "bhed/karte/modell.h"
#include "bhed/karte/region.h"
#include "bhed/karte/spielbeschreibung.h"
#include "bhed/karte/kamera.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/raster.h"
#include "bhed/karte/spiegel.h"
#include "bhed/karte/teiler.h"
#include "bhed/karte/texturbrowser.h"
#include "bhed/karte/texturkopie.h"
#include "bhed/karte/xyansicht.h"
#include "bhed/karte/texturen.h"
#include "bhed/karte/zuletzt.h"

#include <array>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

struct ImDrawList;
struct ImFont;
struct ImVec2;
struct ImVec4;

namespace bhed::gui::kartegpu {
struct Zustand3;
}

namespace bhed::gui {
void schriftenNeuBauen();   // main_win32.cpp: buildFonts im naechsten Bild (OpenGL Font...)
}  // namespace bhed::gui

namespace bhed::gui::karteneditor {

namespace befehle = karte::befehle;

// Das Spiel des Editors: Dateisystem, Textur-Inventar und Entitaetsklassen.
// NRC laedt sie beim Start aus der .game-Datei (QE_InitVFS, Shaders_Load,
// EntityClassQuake3_Construct); hier aus dem Regelwerk des Programms:
//   OpenJK        -> ja (base),     data/karte/ja/*.def (sp)
//   Movie Duels   -> ja + fs_game MD, ja-Klassen + bhed/entdefs.h
//   Jedi Outcast  -> jk2,           data/karte/jk2/*.def (sp)
// Die Tags des Texturbrowsers (texwindow.cpp: TagBuilder, m_tags, m_all_tags,
// m_copied_tags, m_found_shaders, die Liste)
struct TagZustand {
    karte::TexturTags datei;          // TagBuilder
    bool geladen = false;
    bool gui = false;                 // m_tags ("TagsShowGui")
    int reiter = 0;                   // m_tabs->currentIndex()
    std::set<std::string> alle;       // m_all_tags
    std::vector<std::string> kopiert; // m_copied_tags
    std::set<std::string> gefunden;   // m_found_shaders
    std::set<std::string> markiert;   // selectedItems
    std::set<std::string> haken;      // CheckState der Zeilen (Tags des gewaehlten Shaders)
    std::string bearbeitet;           // editItem
    std::string puffer;
    bool bearbeitenFokus = false;
};

// EMessageBoxType
enum class MeldungArt { Info, Warnung, Frage, Fehler };

// Stufe 4 Punkt 2: woher die offene Karte kommt (File > Open .bsp...). Die
// Original-.bsp liegt als <ordner>/<stamm>_original.bsp im Arbeitsordner.
struct BspHerkunft {
    bool aktiv = false;
    std::string quelle;              // Anzeige: Pfad oder maps/... aus dem Spiel
    std::string ordner;              // <Einstellungen>/karte_bsp/<stamm>
    std::string stamm;
    std::uint64_t fingerabdruck = 0; // karte::geometrieFingerabdruck nach dem Laden
};

// Stufe 4 Punkt 3: bobToolz (contrib/bobtoolz). Die Dialoge (dialogs-gtk.cpp)
// sind modal wie bei NRC; dialog: 0 keiner, 1 Intersect, 2 Polygon Builder,
// 3 Stair Builder, 4 Door Builder, 5 Aperture Door Builder, 6 Path Plotter,
// 7 Texture Reset, 8 Make Chain. Die Werte je Aufruf neu, ausser bei
// Aperture Door und Texture Reset (dort "static" wie bei NRC).
struct BobToolzZustand {
    int dialog = 0;
    bool oeffnen = false;
    karte::bobtoolz::IntersectRS intersect;
    karte::bobtoolz::PolygonRS polygon;
    karte::bobtoolz::BuildStairsRS treppe;
    karte::bobtoolz::DoorRS tuer;
    karte::bobtoolz::ApertureDoorRS apertur;      // static ApertureDoorRS rs
    karte::bobtoolz::PathPlotterRS bahnWerte;
    karte::bobtoolz::ResetTextureRS textur;       // static ResetTextureRS rs
    karte::bobtoolz::MakeChainRS kette;
    // Reiter des Aperture-Dialogs (QTabWidget::currentIndex)
    int reiterInnen = 0;
    // die Laufzeit-Objekte (g_PathView, g_TrainView, g_TreePlanter, g_VisView)
    karte::bobtoolz::Bahn bahn;
    karte::bobtoolz::Splines splines;
    karte::bobtoolz::TreePlanter planter;
    karte::bobtoolz::VisAnsicht vis;
    bool visOeffnen = false;
    // LoadLists (bt/bt-el1.txt, bt/bt-el2.txt)
    bool listenGeladen = false;
    std::vector<std::string> ausschluss;
    std::vector<std::string> ausschlussFlaeche;
    std::vector<std::string> tuerTexturen;        // bt/door-tex.txt
    std::vector<std::string> rahmenTexturen;      // bt/door-tex-trim.txt
    std::string letzterBefehl;                    // Selbsttest
    int gepflanzt = 0;                            // Selbsttest: Tree Planter
};

// Stufe 4 Punkt 3: brushexport2 (contrib/brushexport/interface.cpp): ein
// Fenster, das einmal gebaut wird und seinen Zustand behaelt (show/hide).
struct BrushExportZustand {
    bool offen = false;
    bool oeffnen = false;
    int modus = 2;                         // 0 Collapse mesh, 1 by material, 2 Don't collapse
    std::vector<std::string> ignoriert;    // t_materialist
    int gewaehlt = -1;                     // die markierte Zeile
    int bearbeitet = -1;                   // Zeile im Bearbeiten (DoubleClicked / EditKeyPressed)
    std::string eingabe;                   // ed_materialname
    bool kurzeNamen = false;               // t_limitmatnames
    bool objekte = false;                  // t_objects
    bool verschweissen = true;             // t_weld
    bool materialien = true;               // t_exportmaterials
    std::string pfad;                      // s_export_path (leer: "Save" gesperrt)
    std::string zuletzt;                   // Selbsttest: zuletzt geschriebene .obj
};

// Stufe 4 Punkt 3: SunPlug "ET-MapCoordinator" (modal)
struct SunplugZustand {
    bool offen = false;
    bool oeffnen = false;
    std::array<int, 4> werte{0, 0, 0, 0};    // mapcoordsmins x y, mapcoordsmaxs x y
    karte::plugins::Kartenkoordinaten optimal;   // GetOptimalCoordinates
};

// Stufe 4 Punkt 3: MeshTex (contrib/meshtex) - drei nicht modale Dialoge, die
// ihre Werte behalten (GenericDialog: show/hide), Vorgaben wie bei NRC.
struct MeshTexSchnitt {               // "Zero" col/row: Radioknopf mit SpinBox( 0, 30 ) oder "Max"
    bool max = false;
    int nummer = 0;
};
struct MeshTexReferenz {              // "Use reference row/col" (ankreuzbare QGroupBox)
    bool an = true;
    MeshTexSchnitt schnitt;
    bool nurGesamt = true;            // "Total length only"
};
struct MeshTexZustand {
    bool setScaleOffen = false, getInfoOffen = false, genFuncOffen = false;
    bool setScaleOeffnen = false, getInfoOeffnen = false, genFuncOeffnen = false;
    // Set S/T Scale (SetScaleDialog.cpp)
    bool sSetzen = true, tSetzen = true;
    int sArt = 0, tArt = 0;           // 0 "Natural *", 1 "# Tiles"
    double sSkala = 1, sKacheln = 1, tSkala = 1, tKacheln = 1;
    MeshTexSchnitt nullSpalte, nullZeile;   // col_num_align, row_num_align
    MeshTexReferenz refZeile, refSpalte;
    // Get Info (GetInfoDialog.cpp)
    bool infoZeileAn = true, infoSpalteAn = true, uebertragen = false;
    int infoZeile = 0, infoSpalte = 0;
    // General Function (GeneralFunctionDialog.cpp)
    bool flaechenWerte = true;
    bool gfS = true, gfT = true;
    std::array<double, 6> sFaktoren{1, 0, 0, 0, 0, 0}, tFaktoren{1, 0, 0, 0, 0, 0};
    MeshTexSchnitt gfNullSpalte, gfNullZeile;
    MeshTexReferenz gfRefZeile, gfRefSpalte;
    std::string letzterBefehl;        // Selbsttest
};

// Stufe 8b Punkt 1: der Terrain Generator (NRC master, contrib/terrain_generator) -
// ein nicht modaler Dialog; die Werte bleiben die Sitzung lang (static in NRC).
// Stufe 9 Punkt 2 (eigene Funktion, nicht in NRC): Geometrie zu Brushes
struct GeoImportZustand {
    bool offen = false;
    bool neu = false;                         // Vorschau neu rechnen
    std::string datei;                        // die gelesene Datei
    karte::ImportNetz netz;
    std::vector<int> wahl;                    // je Teil karte::TeilArt
    std::vector<karte::TeilErgebnis> vorschau;
    float skala = 1.0F;
    bool yOben = true;
    bool optimierer = true;
    char vorsatz[128] = "textures/import/";
    karte::EinfuegeErgebnis letztes;          // Selbsttest
};

struct TerrainZustand {
    bool offen = false, oeffnen = false;
    karte::terraingen::Werte gespeichert;   // die static-Variablen (beim Schliessen uebernommen)
    karte::terraingen::Werte w;             // die Felder des offenen Dialogs
    std::string letzteTexturAbfrage;        // last_polled_shader
    int erzeugt = 0;                        // Selbsttest: erfolgreiche Generate
    karte::terraingen::Ergebnis letztes;    // Selbsttest
};

// Stufe 8b Punkt 3: Im Editor spielen (eigene Funktion, nicht in NRC)
struct SpielenZustand {
    bool aktiv = false;
    bool startAnSpawn = false;            // BhedPlayAtSpawn: an info_player_start statt an der Kamera
    bool amSpawn = false;                 // dieser Lauf begann an info_player_start
    std::shared_ptr<KollisionsKarte> kollision;
    karte::SpielKollisionsBericht bericht;
    NavSpieler s;
    NavSpielerEingabe eingabe;
    karte::Vektor3f kameraUrsprung{0, 0, 0};
    karte::Vektor3f kameraWinkel{0, 0, 0};
    double restMs = 0.0;
    int bilder = 0;                       // PmoveSingle-Aufrufe
    bool testEingabe = false;             // Selbsttest: Eingaben eingespielt, keine Tastatur/Maus, keine Bildzeit
};

// Stufe 8b Punkt 4: das Fenster "Logic view" (eigene Funktion, nicht in NRC)
struct LogikZustand {
    bool offen = false;
    karte::LogikGraph graf;
    std::size_t stand = static_cast<std::size_t>(-1);   // undo.abschluesse() beim Bau
    std::size_t ladeStand = static_cast<std::size_t>(-1);
    float verschiebung[2] = {20.0F, 20.0F};
    float zoom = 1.0F;
    float mass = 1.0F;           // Bildpunkte je Graph-Punkt (zoom x Schriftmassstab)
    bool einpassen = false;      // beim naechsten Bild den Graphen ins Fenster
    int gewaehlt = -1;
    std::string letztesSkript;   // Selbsttest: der zuletzt gefundene Pfad ("" = fehlte)
    bool nurPruefen = false;     // Selbsttest: nicht in den Skript-Editor wechseln
    bool lagenMerken = false;    // Selbsttest: die Kaesten in Bildpunkten
    std::vector<std::array<float, 4>> lagen;
};

// Stufe 8b Punkt 5: Ansichtsmodi der Kamera (eigene Funktion, nicht in NRC)
struct AnsichtsModiZustand {
    bool dichte = false;          // Lightmap density
    bool kollision = false;       // Collision
    bool sichtbarkeit = false;    // Visibility (PVS) aus der .bsp
    bool last = false;            // Triangle load aus der .bsp
    // Messwerte (Anzeige, Selbsttest)
    int dichteFlaechen = 0;       // Flaechen mit Lightmap
    int kollisionFarbig = 0;      // farbige Brushes (clip, Wasser, Trigger ...)
    int ohneKollision = 0;        // sichtbare Brushes/Patches ohne Kollision fuer den Spieler
    bool bspDa = false;
    int kameraCluster = -1;
    int clusterZahl = 0;
    int sichtbareCluster = 0;
    std::uint64_t sichtbareDreiecke = 0;
    std::uint64_t kameraLast = 0;
    std::uint64_t lastMax = 0;
    std::map<std::string, int> luxelJeShader;   // Selbsttest: groesste Luxelgroesse je Shader (0: ohne Lightmap)
};

struct Spielkontext {
    int spiel = 0;                  // kRegelwerkOpenJK / MovieDuels / JediOutcast
    std::string modus = "sp";       // gamemode_get beim Laden (Mapping mode)
    std::string name;               // "OpenJK (ja)" usw. fuer Meldungen
    std::vector<std::string> ordner;   // Suchpfad, Mod zuerst (QE_InitVFS)
    karte::Dateisystem fs;
    karte::Texturinventar inventar;
    karte::EntitaetsKlassen klassen;
    std::vector<std::string> meldungen;
    karte::Protokoll protokoll;     // die Meldungen beim Laden in NRCs Wortlaut und Reihenfolge
    double sekunden = 0.0;
};

// Eine Zeile der Konsole (radiant/console.cpp): Ausgabe, Warnung, Fehler.
// Eine Zeile kann Stuecke verschiedener Art haben (Sys_Print schreibt Teile,
// erst '\n' beendet die Zeile).
struct KonsolenZeile {
    enum class Art { Ausgabe, Warnung, Fehler };
    struct Teil {
        Art art = Art::Ausgabe;
        std::size_t von = 0;
        std::size_t laenge = 0;
    };
    Art art = Art::Ausgabe;      // Art des ersten Stuecks
    std::string text;            // die ganze Zeile ohne '\n'
    std::vector<Teil> teile;
    bool offen = true;           // noch ohne '\n'
};

// Die Farbrollen der Ansichten (xywindow.h:249-260 g_xywindow_globals,
// camwindow.h:67-68 g_camwindow_globals, render.h:291 g_colour_x/y/z,
// Texturbrowser- und Modellbrowser-Hintergrund). Die Werte kommen aus dem
// Farbschema des Programms (farbenAusThema) - Entscheidung shank: die Farben
// muessen nicht wie NRC sein; NRCs Grundwerte stehen nur zur Erinnerung da.
struct Farben {
    using F3 = std::array<float, 3>;
    F3 texBro{0.25F, 0.25F, 0.25F};          // ColorTexBroBackground
    F3 modBro{0.25F, 0.25F, 0.25F};          // ColorModBroBackground
    F3 kameraHinten{0.25F, 0.25F, 0.25F};    // ColorCameraBackground
    F3 rasterHinten{.225803F, .225803F, .225803F};   // ColorGridBackground
    F3 rasterGross{.294118F, .294118F, .294118F};    // ColorGridMajor
    F3 rasterKlein{.254902F, .254902F, .254902F};    // ColorGridMinor
    F3 rasterText{.972549F, .972549F, .972549F};     // ColorGridText
    F3 rasterBlock{1, 1, 1};                          // ColorGridBlocks
    F3 pinsel{0, 0, 0};                               // ColorGridWorldspawn
    F3 auswahl{1.0F, 0.627451F, 0.0F};                // ColorGridSelection
    F3 auswahl3d{1.0F, 0.627451F, 0.0F};              // ColorCameraSelection
    F3 clipper{0, 0, 1};                              // ColorClipperSplit
    F3 ansichtName{0.516136F, 0.516136F, 0.516136F};  // ColorGridActive
    F3 kamera{0, 0, 1};                               // ColorCameraIcon
    F3 achseX{1, 0, 0};                               // ColorAxisX
    F3 achseY{0, 1, 0};                               // ColorAxisY
    F3 achseZ{0, 0, 1};                               // ColorAxisZ
};
// g_ColoursMenu (colors.cpp:153-171): Beschriftung, Preference-Name, Feld.
struct FarbEintrag {
    const char* text;
    const char* name;
    Farben::F3 Farben::*feld;
};
[[nodiscard]] const std::vector<FarbEintrag>& farbEintraege();
// Die Rollen bekommen ihre Werte aus dem Farbschema des Programms (jedes Bild).
void farbenAusThema();

// Masse des Hauptfensters in logischen Qt-Bildpunkten - NRC 20260114 (Qt 5.15,
// Stil Fusion mit dark.qss), am Foto gemessen und mit Qts Formeln belegt.
// Mal massstab() ergibt Bildpunkte.
namespace mass {
inline constexpr int kSchrift = 16;        // Segoe UI 9 pt = 12 px Geviert = ImGui-Groesse 16 (Hoehe 16)
inline constexpr int kMenueH = 21;         // QMenuBar: fm.height() 16 + CT_MenuBarItem 5
inline constexpr int kMenuePolster = 8;    // CT_MenuBarItem (Fusion): Breite + 8
inline constexpr int kMenueAbstand = 6;    // PM_MenuBarItemSpacing (Fusion)
inline constexpr int kLeisteRand = 1;      // dark.qss QToolBar: border 1px
inline constexpr int kLeistePolster = 3;   // dark.qss QToolBar: padding 3px
inline constexpr int kLeisteGriff = 9;     // PM_ToolBarHandleExtent (Fusion)
inline constexpr int kTrenner = 8;         // dark.qss QToolBar::separator
// Knopfgroesse der Haupt-, Filter- und Plugin-Leiste (Stufe 7 Punkt 6b, eigene
// Einstellung, nicht in NRC): 0 klein, 1 normal (wie NRC), 2 gross. Normal ist
// NRC: PM_ToolBarIconSize 24, Knopf 31 = 24 + 3 ("broken QToolButton") +
// padding 2 x 1 + border 2 x 1, Leiste 39. Klein und gross aendern nur das
// Symbol (16 / 32), der Rest des Knopfes bleibt (+ 7).
inline int knopfStufe = 1;
[[nodiscard]] inline int symbol() { return knopfStufe == 0 ? 16 : (knopfStufe == 2 ? 32 : 24); }
[[nodiscard]] inline int knopf() { return symbol() + 7; }
[[nodiscard]] inline int leisteH() { return kLeisteRand * 2 + kLeistePolster * 2 + knopf(); }
inline constexpr int kStatusH = 21;        // QStatusBar::reformat: 3 + fm.height() 16 + 2
inline constexpr int kStatusBild = 16;     // create_main_statusbar: Bilder 16 hoch
inline constexpr int kStatusAbstand = 6;   // QHBoxLayout der Zaehler (PM_LayoutHorizontalSpacing)
inline constexpr int kTeilerGriff = 4;     // PM_SplitterWidth (Fusion)
}   // namespace mass

// Der Massstab des Editors: wie Qt 5.15 mit AA_EnableHighDpiScaling den
// Bildschirmmassstab rundet (Qt::HighDpiScaleFactorRoundingPolicy::Round:
// 150 % -> 2), mal der Schriftgroesse des Programms (uiScale).
[[nodiscard]] float massstab();
// Fuer den Selbsttest: fester Massstab (0 = wie oben) und feste Groesse des
// Arbeitsbereichs in logischen Punkten (0 = der ganze Platz).
extern float massstabErsatz;
extern int bereichErsatzB;
extern int bereichErsatzH;
// NRCs Schrift (Segoe UI), aus buildFonts; nullptr = die des Programms.
void setzeSchrift(ImFont* schrift);
[[nodiscard]] ImFont* schrift();

// Ein Fenster der Ansichten (XYWnd / CamWnd) - wo es gerade steht.
enum class AnsichtTyp { YZ = 0, XZ = 1, XY = 2, Kamera = 3 };   // VIEWTYPE (xywindow.h)

// Eine 2D-Ansicht (XYWnd) mit Maus- und Tastenzustand.
struct XyFenster {
    karte::XyAnsicht ansicht;
    bool verschieben = false;    // m_move_started (Pan mit M2)
    bool zoomen = false;         // m_zoom_started (Alt+M2)
    int zoomX = 0;               // g_zoom2x
    int zoomY = 0;
    int dragZoom = 0;            // g_dragZoom
    float restX = 0;             // FreezePointer: Rest unter einem logischen Punkt (Bildpunkte, Stufe 7 Punkt 6c)
    float restY = 0;
    float ankerX = 0;            // FreezePointer: Lage beim Festhalten
    float ankerY = 0;
    unsigned tasten = 0;         // m_buttonstate (RAD_*)
    bool gedrueckt = false;      // diese Ansicht hat den Druck bekommen
    bool pluginDruck = false;    // ein Plugin (XYWindowMouseDown) hat den Druck genommen
    karte::Vektor3f maus{0, 0, 0};   // m_mousePosition (eingerastet)
    float bildX = 0;             // linke obere Ecke im Programmfenster (Bildpunkte)
    float bildY = 0;
    // EntityCreate_MouseDown/Move/Up (xywindow.cpp:1065-1083): M2 ohne
    // Bewegung oeffnet beim Loslassen das Entitaetenmenue.
    bool entitaetErzeugen = false;   // m_entityCreate
    int erzeugX = 0;                 // m_entityCreate_x (Fensterpunkte)
    int erzeugY = 0;
    // XYWnd::chaseMouseMotion / ChaseMouse (xywindow.cpp:293-345): M1 am Rand
    // (16 Geraetepunkte) rollt die Ansicht; g_chasemouse_caller ruft alle
    // 4 ms - hier je Bild, der Faktor ist die Zeit seit dem letzten Mal / 10 ms.
    bool jagen = false;              // g_chasemouse_caller.isActive()
    int jagdDx = 0;                  // m_chasemouse_delta_x
    int jagdDy = 0;
    int jagdX = 0;                   // m_chasemouse_current_x
    int jagdY = 0;
    double jagdUhr = 0.0;            // g_chasemouse_timer (Sekunden, ImGui-Zeit)
    // XYWnd::m_backgroundImage (xywindow.h:55-72): Bild im Auswahlkasten einer Ansicht
    struct Hintergrund {
        void* bild = nullptr;        // _tex (Grafikkarte)
        karte::Sicht sicht = karte::Sicht::XY;   // _viewtype
        float xmin = 0, ymin = 0, xmax = 0, ymax = 0;
        std::string datei;           // fuer den Selbsttest
    } hintergrund;
};
// Das Kamerafenster (CamWnd, camwindow.cpp:1625-1803): Freelook, Rechtsklick-
// Uhr, gehaltene Tasten (PressedKeys, accelerator.cpp:88-170).
struct KameraFenster {
    bool frei = false;              // m_bFreeMove
    bool freiBetreten = false;      // m_bFreeMove_entering
    double rechtsUhr = -1000.0;     // m_rightClickTimer: Start (Sekunden, ImGui-Zeit)
    double rechtsWeg = 0.0;         // m_rightClickMove
    double tastenUhr = -1.0;        // m_keycontrol_timer: Start, -1 = steht
    float bildX = 0;                // linke obere Ecke im Programmfenster (Bildpunkte)
    float bildY = 0;
    int breite = 0;                 // Geraetepunkte
    int hoehe = 0;
    float restX = 0;                // Freelook: Rest unter einem logischen Punkt (Bildpunkte, Stufe 7 Punkt 6c)
    float restY = 0;
    std::vector<int> tasten;        // PressedKeys::keys (ImGuiKey)
    bool kontextmenue = false;      // context_menu_show (Doppel-M2) - fuer den Selbsttest
    double zeichenMs = 0.0;         // ShowStats: msec des letzten Bilds
    double letzteStatistik = -1.0;  // m_render_time (f2f)
    std::size_t bilder = 0;         // fuer den Selbsttest: gezeichnete Bilder der Kamera
};
// Stufe 7 Punkt 6c (Tester: "terribly low mouse sensitivity" im 3D-Fenster):
// der Mauszeiger, den der Freelook liest und zuruecksetzt. Im Programm ist es
// der von Windows (sofort: GetCursorPos / SetCursorPos); im Selbsttest
// bewegt ihn der Test (an = true) wie eine echte Maus, und die Hauptschleife
// spielt dazu Windows: WM_MOUSEMOVE als AddMousePosEvent, ein io.WantSetMousePos
// setzt den simulierten Zeiger erst vor dem naechsten Bild (wie
// ImGui_ImplWin32_NewFrame) - der echte Zeiger bleibt dabei unberuehrt.
struct ZeigerSim {
    bool an = false;
    float x = 0;            // Lage im Programmfenster (Bildpunkte, ganzzahlig wie Windows)
    float y = 0;
    bool setzenOffen = false;   // io.WantSetMousePos vor dem Backend abgefangen
    int gesetzt = 0;            // so oft gesetzt (SetCursorPos)
};
ZeigerSim& zeigerSim();
// Fuer den Selbsttest: fester Bildschirmmassstab des Freelooks (0 = der von Windows)
inline float freelookDpiErsatz = 0.0F;
// FreezePointer (Kamera-Freelook, 2D verschieben / zoomen): Qts devicePixelRatio
// (Rundung Round: 150 % -> 2), den Zeiger lesen und setzen (Fensterpunkte, sofort),
// und das Delta zum Anker in logischen Punkten - der Zeiger geht zum Anker
// zurueck, der Rest unter einem Punkt bleibt in restX/restY.
[[nodiscard]] float qtFaktor();
bool zeigerHolen(ImVec2& p);
void zeigerSetzen(ImVec2 p);
bool zeigerDelta(ImVec2 anker, float qt, float& restX, float& restY, int& dx, int& dy);
// Die Hauptschleife im Selbsttest: vor ImGui_ImplWin32_NewFrame bzw. davor ImGui::NewFrame
void zeigerSimVorBackend();
void zeigerSimVorBild();

// RadiantWindowObserver (selection.cpp:2640-2800) einer Ansicht mit Selector_
// und Manipulator_ (selection.cpp:2482-2638): 0..2 die 2D-Ansichten, 3 die Kamera.
struct FensterBeobachter {
    int breite = 1;                       // m_width (onSizeChanged)
    int hoehe = 1;                        // m_height
    bool mausUnten = false;               // m_mouse_down
    float bewegt = 0;                     // m_move (seit dem letzten Loslassen)
    float bewegtGedrueckt = 0;            // m_movePressed (seit dem Druck)
    karte::GeraeteVektor bewegStart;      // m_moveStart
    karte::GeraeteVektor bewegEnde;       // m_moveEnd
    enum class Ziel { Keins, Manipulator, Auswahl, Textur };   // g_mouseMovedCallback / g_mouseUpCallback
    Ziel ziel = Ziel::Keins;
    // Selector_
    karte::GeraeteVektor start;           // m_start
    karte::GeraeteVektor aktuell;         // m_current
    bool maus2 = false;                   // m_mouse2
    bool mausBewegt = false;              // m_mouseMoved
    bool mausBewegtGedrueckt = false;     // m_mouseMovedWhilePressed
    karte::AuswahlArt malArt = karte::AuswahlArt::Auswaehlen;   // m_paintMode
    karte::AuswahlRechteck rechteck;      // m_XORRect (draw_area)
    // Manipulator_
    bool ursprungBewegt = false;          // m_moving_transformOrigin
    bool manipBewegtGedrueckt = false;    // m_mouseMovedWhilePressed
    // TexManipulator_
    bool texRueckgaengig = false;         // m_undo_begun
};

// View > Show und die Seite "Orthographic" (xywindow.cpp:77-98,
// entity.cpp:114-122).
struct Anzeige {
    bool groesse2d = true;       // ShowSize2d (J)
    bool fadenkreuz = false;     // ToggleCrosshairs (Shift+X)
    bool raster = true;          // ToggleGrid (0)
    bool winkel = true;          // ShowAngles
    bool namen = true;           // ShowNames
    bool kaesten = false;        // ShowBboxes
    bool verbindungen = true;    // ShowConnections
    bool lichtRadien = true;     // ShowLightRadiuses
    bool bloecke = false;        // ShowBlocks
    bool koordinaten = true;     // ShowCoordinates
    bool umriss = true;          // ShowWindowOutline
    bool achsen = true;          // ShowAxes
    bool workzone2d = false;     // ShowWorkzone2d
    int namenVerhaeltnis = 64;   // g_showNamesRatio
    int msaa = 8;                // m_MSAA (XYMSAA)
    bool zumZeiger = true;       // m_bZoomToPointer
    bool jagen = true;           // m_bChaseMouse
    bool keineStrichelung = true;   // m_bNoStipple
};

// QColorDialog (Qt 5.15 qcolordialog.cpp) als Farbdialog von NRCs
// color_dialog (gtkmisc.cpp:331-337, Titel "Choose Color").
struct FarbDialog {
    bool offen = false;
    bool oeffnen = false;                  // im naechsten Bild als Popup oeffnen
    std::function<void(const karte::Vektor3f&)> fertig;   // nur bei OK
    int h = 0, s = 0, v = 255;             // QColorShower hue/sat/val
    int r = 255, g = 255, b = 255;         // curCol
    bool rgbOriginal = true;
    std::string html = "#ffffff";          // htEd
    int standardAktuell[2] = {0, 0};       // QWellArray curRow/curCol
    int standardGewaehlt[2] = {-1, -1};    // selRow/selCol
    int eigeneAktuell[2] = {0, 0};
    int eigeneGewaehlt[2] = {-1, -1};
    int naechsteEigene = 0;                // nextCust
    bool picken = false;                   // _q_pickScreenColor
    unsigned vorPicken = 0;                // beforeScreenColorPicking
    std::string pickText = "\n";          // lblScreenColorInfo
    int lumZiehen = 0;                     // 1: Farbfeld, 2: Helligkeit gezogen
};

// Die Seite "Entities" des Gruppendialogs, die Entity List, das
// Entitaetenmenue und die Dialoge dazu (Punkt 9).
struct EntitaetenOberflaeche {
    // Entity Inspector (entityinspector.cpp)
    karte::Teiler teiler;                  // QSplitter "EntityInspector/splitter"
    std::vector<int> groessen;             // gemerkt (sonst { 55, 175, 255, 255 })
    karte::GewaehlteSchluessel werte;      // g_selectedKeyValues / g_selectedDefaultKeyValues
    std::vector<const void*> auswahlStand; // fuer die Aktualisierung (Auswahl)
    bool geladen = false;
    std::string klasse;                    // current item der Klassenliste (Name)
    std::string kommentarKlasse;           // g_current_comment
    bool klasseRollen = false;             // scrollTo(current)
    bool klassenFokus = false;             // die Klassenliste hat den Fokus
    std::string schluessel;                // g_entityKeyEntry
    std::string wert;                      // g_entityValueEntry
    std::string zeile;                     // g_entprops_store->currentItem() (Schluessel), leer: keins
    bool zeilenFokus = false;
    bool autoFokus = false;                // g_focusToggleButton
    std::map<std::string, std::string> felder;   // Texte der NonModalEntry (Attribute) beim Bearbeiten
    std::string hinweis;                   // QToolTip der Validatoren
    double hinweisBis = 0;
    // Entity List (entitylist.cpp)
    bool listeOffen = false;
    bool listeAutoFokus = false;           // m_check
    bool listeVonAnfang = false;           // m_search_from_start (EntListSearchFromStart)
    std::string listeFilter;
    std::set<std::string> listeOffeneKnoten;   // aufgeklappte Zeilen
    const void* listeAnker = nullptr;      // fuer Umschalt+Klick (ExtendedSelection)
    // Entitaetenmenue (xywindow.cpp:743-878)
    bool menueOeffnen = false;
    bool kameraMenue = false;              // g_bCamEntityMenu
    int erzeugtZaehler = 0;                // g_entityCreationOffset
    // Set Light intensity (gtkdlgs.cpp:408-443)
    bool lichtOffen = false;
    bool lichtOeffnen = false;
    int lichtLetzte = 300;                 // g_iLastLightIntensity (LastLightIntensity)
    int lichtWert = 300;
    bool lichtNichtZeigen = false;         // g_dontDoLightIntensityDlg
    karte::ErzeugtErgebnis offen;          // der offene Schritt "entityCreate -class X"
    // Farbe
    karte::Vektor3f farbe{1, 1, 1};        // g_entity_globals.color_entity (EntitySelectedColor)
    FarbDialog farbDialog;
    std::array<unsigned, 16> eigeneFarben{};   // QColorDialogOptions customColors (QtProject Qt/customColors)
    bool eigeneFarbenGeladen = false;
    // Einstellungsseite Entities (entity.cpp:663-686)
    int namenAbstand = 512;                // g_showNamesDist (SI_ShowNamesDist)
    bool zielnamen = false;                // g_showTargetNames (SI_ShowTargetNames)
    int klassenStand = 0;                  // steigt mit Reload Entity Definitions
};

struct Editor {
    // --- die Karte (g_map) ---
    karte::Karte karte;
    std::string name = "unnamed.map";   // Map_Name; "unnamed.map" = Map_Unnamed
    std::shared_ptr<Spielkontext> karteSpiel;   // die Klassen der offenen Karte (Zeiger darauf)
    // Stufe 4: die offene Karte kam aus einer .bsp (File > Open .bsp...)
    BspHerkunft bspHerkunft;
    std::string bspNurEntitaetenFertig;   // Selbsttest: zuletzt mit -onlyents geschriebene .bsp
    // Stufe 4 Punkt 3: die Plugins
    BobToolzZustand bob;
    BrushExportZustand brushExport;
    SunplugZustand sunplug;
    MeshTexZustand meshtex;
    TerrainZustand terrain;   // Stufe 8b Punkt 1
    GeoImportZustand geoImport;   // Stufe 9 Punkt 2
    SpielenZustand spielen;   // Stufe 8b Punkt 3
    LogikZustand logik;       // Stufe 8b Punkt 4
    AnsichtsModiZustand ansichtsmodi;   // Stufe 8b Punkt 5
    karte::SpiegelModus spiegel;        // Stufe 8b Punkt 6
    karte::PrefabVerbund verbund;       // Stufe 9 Punkt 6: verknuepfte Prefabs
    bool undoListeOffen = false;        // Stufe 8b Punkt 7: Undo-Verlauf
    bool undoListeRollen = true;
    std::size_t undoListeStand = 0;
    karte::plugins::UfoaiZustand ufoai;
    karte::ZuletztListe zuletzt;
    bool zuletztGeladen = false;

    // --- das Spiel ---
    std::shared_ptr<Spielkontext> spiel;   // fertig geladen
    std::future<std::shared_ptr<Spielkontext>> spielLaden;
    int spielStand = -1;                   // fuer welches Regelwerk geladen
    std::string spielKennung;              // Spiel, Suchordner und Modus des geladenen (EnginePath_Realise)
    // Seiten Global > Game, Game, Paths (Stufe 2, Punkt 11b)
    std::string spielDatei;                // CGameDialog::m_sGameFile ("gamefile"); leer = nach dem Regelwerk
    std::string spielDateiNeu;             // m_sGameFile.m_latched: gilt nach dem Neustart
    std::vector<std::string> neustartFragen;   // g_restart_required
    bool spielFrage = false;               // m_bGamePrompt ("gamePrompt", Startup / Show Global Preferences)
    bool spielFrageGezeigt = false;        // CGameDialog::Init: nur beim Start
    bool globaleEinstellungenOffen = false;   // CGameDialog::DoGameDialog ("Global Preferences")
    std::string spielName;                 // g_gamename ("GameName" = fs_game); leer = basegame
    std::string spielModus = "sp";         // g_gamemode ("GameMode")
    std::string enginePfad;                // g_strEnginePath ("EnginePath"); leer = aus den Spielordnern des Programms
    std::array<std::string, 5> zusatzPfade;   // g_strExtraResourcePaths ("ExtraResourcePath0".."4")

    // --- Rueckfrage "Karte geaendert" (ConfirmModified, qe3.cpp:128) ---
    bool frageOffen = false;
    std::string frageTitel;                // "New Map", "Open Map"
    std::function<void()> nachFrage;       // was danach geschieht

    // --- Konsole ---
    std::vector<KonsolenZeile> konsole;
    std::size_t konsoleStand = 0;    // steigt mit jedem Schreiben und Leeren
    bool konsoleRollen = false;      // nach einem '\n' ans Ende rollen (ensureCursorVisible)

    // --- Fenster (MainFrame) ---
    befehle::Anordnung anordnung = befehle::Anordnung::Regular;   // g_Layout_viewStyle
    bool gruppenDialogEingebaut = false;   // g_Layout_builtInGroupDialog
    // QSplitter m_hSplit, m_vSplit, m_vSplit2 (MainFrame::Create) mit Qts
    // Rechnung; teilerFuer: fuer welche Anordnung sie aufgebaut sind.
    karte::Teiler hSplit;
    karte::Teiler vSplit;
    karte::Teiler vSplit2;
    int teilerFuer = -1;
    // Gemerkte Groessen (QSettings "MainFrame/m_hSplit" ...; RestoreGuiState
    // nur ohne FloatingGroupDialog, also Regular und Regular Left).
    std::vector<int> hGroessen;
    std::vector<int> vGroessen;
    std::vector<int> v2Groessen;
    // Stufe 8b Punkt 2: Split-Fenster frei belegen (Preferences > Layout, nicht
    // in NetRadiant-Custom; Vorgabe aus = NRC). Je Platz (oben links, unten
    // links, oben rechts, unten rechts) der Inhalt: 0..2 die 2D-Fenster xy[i],
    // 3 die Kamera - jedes erscheint genau einmal (NRC: Kamera, YZ, XY, XZ).
    // Mit dem Schalter bleiben Belegung, die 2D-Typen und die Split-Groessen
    // nach dem Neustart.
    bool splitFrei = false;
    std::array<int, 4> splitBelegung{3, 2, 0, 1};
    std::vector<int> splitHGroessen, splitVGroessen, splitV2Groessen;
    // MaximizeView (mainframe.cpp:1550-1595): Groessen vor dem Maximieren.
    bool maximiert = false;
    std::vector<int> maxH;
    std::vector<int> maxV;
    std::vector<int> maxV2;
    // Lage des Mauszeigers ueber m_hSplit in logischen Punkten (fuer
    // MaximizeView: mapFromGlobal(QCursor::pos())).
    int mausX = 0;
    int mausY = 0;
    // Floating: die Fenster Camera, XY, XZ, YZ (ToggleCamera, ToggleView ...).
    std::array<bool, 4> schwebendSichtbar{true, true, true, true};
    bool schwebendZuruecksetzen = false;   // Lage und Groesse wieder wie addWindow (ein Bild lang)
    // Der Gruppendialog "Entities" (groupdialog.cpp) mit seinen Seiten.
    bool gruppenDialogOffen = false;
    int gruppenSeite = 0;   // 0 Entities, 1 Console, 2 Textures, 3 Models, 4 Layers
    bool gruppenSeiteWechseln = false;
    // Werkzeugleisten (toolbar_construct_control_menu)
    bool hauptleiste = true;
    bool filterleiste = true;
    bool pluginleiste = true;
    std::set<std::string> versteckteKnoepfe;   // g_toolbarHiddenButtons
    // Alt + Mnemonik: dieses Menue der Leiste im naechsten Bild oeffnen
    // (Lage in befehle::menues(), -1 = keins).
    int menueOeffnen = -1;
    Farben farben;
    // Statuszeile (create_main_statusbar): c_status_command, _position, _texture
    std::string statusBefehl;
    std::string statusPosition;
    std::string statusTextur;

    // --- Raster und Kamera-Werte, die die Statuszeile zeigt ---
    karte::Raster raster;     // grid.cpp

    // --- Befehle ---
    befehle::Belegung belegung;
    bool belegungGeladen = false;
    // Dialoge
    bool einstellungenOffen = false;
    bool tastenDialogOffen = false;   // "Mapped Commands" (DoCommandListDlg)
    // CSG Tool (CSGToolDialog, csg.cpp:1372-1380): nicht modal, die Werte
    // bleiben bis zum Programmende (NRC speichert nur die Fensterlage).
    struct CsgWerkzeug {
        bool offen = false;
        double dicke = 16;      // DoubleSpinBox( 0, 9999, 16, 3, 16 )
        double schritt = 16;    // singleStep
        int ausschluss = 0;     // radFaces, radPlusFaces, radProj, radCam
        bool caulk = true;      // f-caulk.png, setChecked( true )
        bool innenWeg = true;   // csgtool_removeinner.png
    } csgWerkzeug;
    // Der Modell-Browser (modelwindow.cpp): Einstellungen und QSplitter
    struct ModellBrowserEinstellung {
        int zelle = 80;                           // m_cellSize ("ModelBrowserCellSize")
        std::string ordner = "*models/99*";       // m_prefFoldersToLoad ("ModelBrowserFolders")
        std::array<float, 3> hintergrund{.25F, .25F, .25F};   // m_background_color ("ColorModBroBackground")
    } modBroE;
    karte::Teiler modTeiler;                      // "ModelBrowser/splitter"
    // Der Ebenen-Browser (layerswindow.cpp: LayersBrowser g_lbro)
    struct EbenenBrowser {
        std::vector<char> sichtbar;          // item->data( Column::visible ) je Ebene
        std::size_t stand = static_cast<std::size_t>(-1);   // ladeZaehler beim Aufbau
        int gewaehlt = -1;                   // die gewaehlte Zeile
        int druck = -1;                      // Zeile beim Druecken (Klick oder Ziehen)
        float druckX = 0.0F;                 // Ort beim Druecken (Bildpunkte)
        float druckY = 0.0F;
        bool ziehen = false;                 // QAbstractItemView: DraggingState
        int menueEbene = -1;                 // context_menu: die Zeile unter der Maus
        bool menueOeffnen = false;           // context_moveto_menu (Umschalt+Q)
        bool menueOffen = false;
    } ebenenBro;
    // QInputDialog::getText (Layer Name)
    struct Eingabe {
        bool offen = false;
        bool oeffnen = false;
        std::string titel;
        std::string beschriftung;
        char puffer[256] = "";
        std::function<void(const std::string&)> fertig;
    } eingabe;
    // Filter (filters.cpp): g_filters_globals.exclude ("SI_Exclude")
    unsigned int filterMaske = 0;
    std::size_t filterSchluessel = 0;   // Stand, mit dem zuletzt gefiltert wurde
    // Region (map.cpp:1292-1297): g_region_active, g_region_mins/maxs
    karte::Region region;
    std::string regionKastenShader;   // g_regionBoxShader ("RegionBoxShader", Seite Build)
    // Autosave (autosave.cpp:104-158) mit dem EverySecondTimer
    karte::AutoSpeichern autosave;
    std::time_t autosaveVersatz = 0;  // Selbsttest: die Uhr vorstellen
    double autosaveTakt = 0.0;
    // "Find Brush" (DoFind, map.cpp:2085): modal, jedes Mal neu
    struct PinselSuche {
        bool offen = false;
        bool oeffnen = false;
        int entitaet = 0;
        int pinsel = 0;
    } pinselSuche;
    // "Map Info" (DoMapInfo, map.cpp:906): modal
    struct KartenInfoDialog {
        bool offen = false;
        bool oeffnen = false;
        karte::KartenInfo info;
        int sortSpalte = 0;      // sortByColumn( 0, AscendingOrder )
        bool absteigend = false;
        std::string gewaehlt;
    } kartenInfo;
    // "Arbitrary sides" (DoSides, gtkdlgs.cpp:272): modal, jedes Mal neu
    struct SeitenDialog {
        bool offen = false;
        bool oeffnen = false;
        karte::Grundform art = karte::Grundform::Prisma;
        int wert = 8;
        bool stutzen = false;   // "Truncate" (nur Icosahedron)
    } seitenDialog;
    // "Arbitrary rotation" / "Arbitrary scale" (RotateDialog, ScaleDialog,
    // select.cpp:1580-1757): nicht modal; die Werte bleiben nach OK stehen,
    // Cancel setzt sie zurueck (0 bzw. 1).
    struct ArbitraerDialog {
        bool offen = false;
        bool zeigen = false;
        std::array<double, 3> werte{};
    };
    ArbitraerDialog drehDialog{false, false, {0.0, 0.0, 0.0}};
    ArbitraerDialog skalierDialog{false, false, {1.0, 1.0, 1.0}};
    // Curve-Dialoge (patchmanip.cpp:837-1004): modal, jedes Mal neu;
    // art 1 "Patch density", 2 "Patch deform", 3 "Cap", 4 "Patch thicken"
    struct PatchDialog {
        int art = 0;
        bool oeffnen = false;
        karte::PatchVorlage vorlage = karte::PatchVorlage::Ebene;
        int spalten = 0;          // Index in der Breiten-Liste
        int zeilen = 0;
        bool quadrat = false;     // "Square" (Redisperse)
        int wert = 0;             // Max deform / Thickness
        int achse = 2;            // X Y Z (Normal)
        bool waende = true;       // "Side walls"
        int kappe = 0;            // Bevel, Endcap, Inverted Bevel, Inverted Endcap, Cylinder
    } patchDialog;
    // Surface Inspector (surfacedialog.cpp): nicht modal, S schaltet
    bool siOffen = false;
    karte::SiWerte siWerte;   // g_si_globals
    TagZustand tags;          // Tags des Texturbrowsers (app_karte_tags.cpp)
    // Textures_constructPreferences (textures.cpp:761-829)
    struct TexturQualitaet {
        int mipStufe = 0;          // g_Textures_mipLevel ("Texture Quality")
        float gamma = 1.0F;        // g_texture_globals.fGamma
        int modus = 5;             // g_texture_mode (ETexturesMode)
        int kompression = 0;       // m_nTextureCompressionFormat (D3D11: keine)
        bool anisotropie = true;   // g_TextureAnisotropy
    } texQual;
    bool abtasterGesetzt = false;
    // qt_MessageBox als Klappfenster
    struct Meldung {
        bool offen = false;
        bool neu = false;
        std::string titel;
        std::string text;
        MeldungArt art = MeldungArt::Info;
        bool jaNein = false;
        std::function<void(bool)> danach;
    } meldung;
    std::string tastenDialogBearbeitet;   // Befehl, dessen Taste gerade aufgenommen wird
    int tasteVerbrauchtBild = -1;         // in diesem Bild hat die Aufnahme die Taste geschluckt
    bool ueberOffen = false;              // DoAbout ("About NetRadiant")
    // Stufe 2, Punkt 11c
    bool menueAbreissbar = true;          // g_Layout_enableDetachableMenus ("DetachableMenus")
    // Stufe 7 Punkt 6b (shank: "Gerade das file edit usw. Koennte normal aussehen"):
    // Menueleiste und Menues im Stil des Programms (false, Vorgabe) oder wie
    // NRCs QMenu (true, "Menus like NetRadiant-Custom" unter Preferences > Interface).
    bool menueNrcStil = false;
    struct Abriss {                       // QTornOffMenu
        std::string titel;                // removeMnemonics( title ).trimmed()
        std::size_t start = 0;            // erster Eintrag des Menues in der Tabelle
        std::string kennung;              // die des Menues (fuer das Umschalten)
        float x = 0;                      // setGeometry( menu->geometry() )
        float y = 0;
        bool neu = true;
    };
    std::vector<Abriss> abrisse;
    // Stufe 7 Punkt 6f: fruehere Spiele der offenen Karte (die Klassen ihrer Entities), bis zur Freigabe
    std::vector<std::shared_ptr<Spielkontext>> karteSpielAlt;
    std::string shaderEditorBefehl;       // g_TextEditor_editorCommand ("CustomShaderEditorCommand")
    std::string glSchriftName = "Myriad Pro";   // g_OpenGLFont ("OpenGLFont")
    int glSchriftGroesse = 8;             // g_OpenGLFontSize ("OpenGLFontSize")
    float glSchriftVerhaeltnis = 1.33F;   // (hhea ascender - descender) / unitsPerEm der geladenen Schrift
    std::string glSchriftGeladen;         // die Datei der geladenen OpenGL-Schrift (leer: die Kartenschrift)
    bool schriftDialogOffen = false;      // QFontDialog::getFont ("Select Font")
    bool enginePfadDialogOffen = false;   // PathsDialog ("Engine Path Configuration")
    bool enginePfadGeprueft = false;      // EnginePath_verify beim Start
    // Fuer den Selbsttest ersetzbar: OpenURL und Q_Exec (sonst Browser bzw. Programm)
    std::function<void(const std::string&)> urlOeffnen;
    std::function<bool(const std::string&)> befehlStarten;
    std::function<std::string(const std::string&)> dateiDialog;   // file_dialog (Titel -> Pfad)
    std::string letzterBefehl;   // fuer den Selbsttest: zuletzt ausgeloest
    std::set<std::string> befehleAusgefuehrt;   // fuer den Selbsttest: jeder ausgeloeste Befehl
    std::size_t neuGeladen = 0;   // fuer den Selbsttest: RefreshShaders, RefreshReferences, Reload Entity Definitions
    std::size_t bauGestartet = 0; // fuer den Selbsttest: jeder Bau (BuildMonitor_Run)
    std::vector<std::string> knoepfeImBild;   // fuer den Selbsttest: gezeichnete Knoepfe
    std::vector<std::string> knoepfeVerdeckt; // fuer den Selbsttest: im Erweiterungsmenue gezeigte
    // Fuer den Selbsttest: die Rechtecke dieses Bildes in logischen Punkten,
    // bezogen auf die linke obere Ecke des Arbeitsbereichs.
    struct Rechteck {
        std::string name;
        int x = 0;
        int y = 0;
        int b = 0;
        int h = 0;
    };
    std::vector<Rechteck> felderImBild;
    // Fuer den Selbsttest: die Klapplisten (QComboBox) der Dialoge in diesem Bild
    // (Bildpunkte; ImGui meldet sie nicht unter ihrer Kennung).
    struct KlappOrt {
        std::string id;
        float x0 = 0;
        float y0 = 0;
        float x1 = 0;
        float y1 = 0;
    };
    std::vector<KlappOrt> klappOrte;
    int listenBild = -1;   // ImGui-Bild, in dem die Listen oben zuletzt geleert wurden (bildBeginnen)
    // Stufe 3 Punkt 0b: Unterkante der Menueleiste im Programmfenster und
    // Oberkante des Editors (Bildpunkte, fuer den Selbsttest: direkt darunter).
    float leisteUnten = 0.0F;
    float editorOben = 0.0F;
    // Stufe 3 Punkt 0b, "Rahmen + Abstand" (Entscheidung shank): jedes Feld hat
    // einen feinen Rahmen, das aktive einen in der Akzentfarbe. Fuer den
    // Selbsttest: was gezeichnet wurde (Bildpunkte).
    struct RahmenImBild {
        std::string feld;
        float x0 = 0;
        float y0 = 0;
        float x1 = 0;
        float y1 = 0;
        unsigned farbe = 0;
        bool aktiv = false;
    };
    std::vector<RahmenImBild> rahmenImBild;
    int aktivesFeld = 0;   // Feld (app_karte.cpp): zuletzt angeklickt; zu Beginn XY wie aktivXy
    int aktivXyVorher = -1;
    std::string letzteHilfe;   // fuer den Selbsttest: OpenManual / CheckForUpdate (Seite bzw. "update")
    std::string letzteUrl;     // fuer den Selbsttest: was ohne Ersatz geoeffnet bzw. gestartet worden waere

    // Fuer den Selbsttest: die Kennung des Arbeitsbereichs im letzten Bild.
    bool gezeichnet = false;
    // Fuer den Selbsttest (Punkt 12): Zeit des letzten Bildes im Editor
    // (zeichne, Millisekunden) und davon die Ansichten (2D und Kamera).
    double zeichenMs = 0.0;
    double ansichtenMs = 0.0;
    double kameraNeubauMs = 0.0;   // letzter Neubau der Kamera-Szene
    double xyNeubauMs = 0.0;       // letzter Neubau einer 2D-Szene
    std::size_t geoNeubauten = 0;  // Neubauten der Geometrie (Kamera und 2D)
    std::size_t restNeubauten = 0; // Neubauten des Rests (Auswahl, Punktentitaeten, Namen)

    // --- 2D-Ansichten und Kamera ---
    std::array<XyFenster, 3> xy;   // 0 m_pXYWnd (XY), 1 m_pXZWnd (XZ), 2 m_pYZWnd (YZ)
    int aktivXy = 0;               // ActiveXY
    karte::Kamera kamera;              // m_Camera
    karte::KameraEinstellungen kameraE;   // camwindow_globals(_private), camera_t statics
    KameraFenster kamFenster;
    // Select_getWorkZone (select.cpp:461-476): Grenzen der letzten Auswahl.
    karte::Kasten workzone{karte::Vektor3f(0, 0, 0), karte::Vektor3f(64, 64, 64)};
    Anzeige anzeige;
    std::size_t ladeZaehler = 0;   // jede neu geladene/neue Karte

    // --- Auswahl und Bearbeiten (Punkt 7) ---
    karte::AuswahlBearbeitung bearbeitung{karte};   // RadiantSelectionSystem
    karte::ManipModus werkzeug = karte::ManipModus::Ziehen;   // g_currentToolMode (g_defaultToolMode = DragMode)
    bool knotenVersteckt = false;          // g_nodes_be_hidden (HideSelected)
    std::size_t sichtStand = 0;            // Verstecken/Zeigen (nicht im Undo) fuer szeneStand
    // Stufe 9 Punkt 4: Sprungweite und Platz (View > Check jumps and space)
    int sprungMacht = 0;                   // Machtstufe fuer die Pruefung und "Im Editor spielen" ("BhedJumpCheckForce", Vorgabe ohne)
    bool sprungLaeuft = false;
    std::shared_ptr<karte::SprungBericht> sprungBericht;
    // Stufe 9 Punkt 5: NPC-Wege (View > NPC paths)
    bool npcNetzZeigen = false;
    bool npcRennen = true;
    std::shared_ptr<NavWelt> npcWelt;
    std::size_t npcWeltStand = static_cast<std::size_t>(-1);
    std::shared_ptr<LaufPlan> npcLauf;
    // Stufe 9 Punkt 9: Gelaende formen (View > Terrain sculpt; M1 in der Kamera)
    struct FormenZustand {
        bool an = false;
        bool strich = false;      // ein Befehl laeuft (M1 gedrueckt)
        int art = 0;              // karte::FormArt
        float radius = 128.0F;
        float ebeneZ = 0.0F;      // Ebnen: die Hoehe beim Druck
    } formen;
    double npcLaufStart = 0.0;
    std::string zwischenablage;            // Copy/Paste (NRC: QClipboard "RadiantClippings")
    bool nachKlonenSchubsen = false;       // g_bNudgeAfterClone ("Nudge selected after duplication")
    // --- Einstellungsseiten der Stufe 2, Punkt 11 ---
    bool letzteKarteLaden = true;          // g_bLoadLastMap ("LoadLastMap", map.cpp:2426)
    std::string letzteKarte;               // g_strLastMap ("LastMap")
    bool letzteKarteGeprueft = false;      // main.cpp:462: nur beim Start
    std::string sitzungKarte;              // Stufe 9 Punkt 12: der Name in der Sitzungsdatei
    int rasterStandard = karte::Raster::kStandard;   // g_grid_default ("GridDefault")
    int rasterMaxPotenz = 4;               // g_maxGridCoordPower ("GridMaxCoordPower")
    int pinselTyp = 1;                     // g_brushType ("BrushType"): quake3 -> Brush_Construct( eBrushTypeQuake3BP )
    // BuildMonitor_Construct (watchbsp.cpp:257-268); der Bau selbst kommt in Stufe 3
    // --- Build-Menue (Stufe 3, build.cpp) ---
    karte::BauProjekt bauProjekt;          // g_build_project + g_build_tools
    int bauProjektFuer = -2;               // Spiel (kRegelwerk*), fuer das bauProjekt gelesen ist
    std::string bauMenueName;              // g_buildMenu ("BuildMenu")
    bool bauGeaendert = false;             // g_build_changed
    long long bauZuletzt = -1;             // g_lastExecutedBuild (Index, -1 = keiner)
    bool bauDialogOffen = false;           // DoBuildMenu ("Build Menu")
    std::string bauKarte;                  // die Datei, die gebaut wird, wenn nicht die Karte selbst (Punkt 4)
    std::string bauKarteFuer;              // ... fuer diese Karte (sonst gilt die Karte selbst)
    // Q3Map debug window (feedback.cpp g_DbgDlg) und die Leck-Linie (points.cpp CPointfile)
    std::vector<karte::BauRueckmeldung> bauRueckmeldungen;   // m_feedbackElements
    int bauRueckmeldungWahl = -1;          // m_pHighlight (Zeile)
    bool bauRueckmeldungOffen = false;     // ShowDlg
    std::vector<karte::Vektor3f> punktdatei;   // s_pointvecs
    bool punktdateiSichtbar = false;       // shown()
    std::size_t punktdateiStelle = 0;      // s_check_point
    // Portal Viewer (contrib/prtview: CPortals portals, prtview.ini)
    karte::PortalEinstellungen portalE;            // die Einstellungen von CPortals
    std::vector<karte::BspPortal> portale;         // portals.portal
    bool portalHintFlags = false;                  // portals.hint_flags
    std::string portalDatei;                       // portals.fn
    bool portalEinstellungenGelesen = false;       // LoadConfig (PrtView_construct)
    bool portalLadenOffen = false;                 // DoLoadPortalFileDialog ("Load .prt")
    bool portalKonfigOffen = false;                // DoConfigDialog ("Portal Viewer Configuration")
    struct BauEinstellungen {
        bool ueberwachen = true;           // g_WatchBSP_Enabled ("BuildMonitor")
        bool leckStopp = true;             // g_WatchBSP_LeakStop ("BuildLeakStop")
        bool spielStarten = false;         // g_WatchBSP_RunQuake ("BuildRunGame")
        bool logAusgeben = false;          // g_WatchBSP0_DumpLog ("BuildDumpLog")
        karte::VorgabeText engine;         // g_engineExecutable ("BuildEngineExecutable")
        karte::VorgabeText engineMp;       // g_engineExecutableMP
        karte::VorgabeText argumente;      // g_engineArgs
        karte::VorgabeText argumenteMp;    // g_engineArgsMP
        std::string zielOrdner;            // Abweichung: Ziel-Ordner ("BuildTargetFolder"), leer = neben der .map
        bool melden = true;                // Stufe 8a: Taskleiste blinkt, wenn ein Bau fertig ist ("BhedBuildNotify")
        bool editorDaten = false;          // Stufe 8a: -keeplights und <karte>.bhed ("BhedKeepEditorData"), Vorgabe aus wie NRC
        bool visWieder = false;            // Stufe 8a Punkt 5a: -bhedvisreuse fuer -vis ("BhedVisReuse")
        bool lichtWieder = false;          // Stufe 8a Punkt 5b/5d: -bhedlightcache fuer -light ("BhedLightReuse")
        karte::GpuBacken gpuBacken = karte::kGpuBackenVorgabe;   // Stufe 8g/8d: Aus/Ein/Auto fuer -light und das Schnell-Backen ("BhedGpuBakeMode")
        std::string gpuWahlText;     // Stufe 8d: was Auto zuletzt gewaehlt hat und warum (Konsole, Statuszeile, Tooltip)
        std::string gpuBericht;            // Stufe 8g: der Abweichungsbericht des letzten GPU-Backens (eine Zeile, uebersetzt)
        std::string gpuAdapter;            // Stufe 8g: die GPU des laufenden bzw. letzten GPU-Backens
        std::string testExe;               // Stufe 9 Punkt 1: das Programm fuer "Im Spiel testen" ("BhedTestGameExe", leer = Engine)
        std::string testBefehl;            // Stufe 9 Punkt 1: die zuletzt erzeugte Befehlszeile (Selbsttest)
        std::string testPk3;               // Stufe 9 Punkt 1: das zuletzt geschriebene Archiv (Selbsttest)
    } bau;
    // Stufe 8a Punkt 3/4: was "Open .bsp" zurueckgewinnt (eigene Funktion, nicht in NRC)
    struct BspRueckEinstellungen {
        bool modelle = true;     // misc_model aus der Geometrie ("BhedBspRecoverModels")
        bool flaechen = true;    // nicht erkannte Flaechen als Pinsel in einer eigenen Ebene ("BhedBspUnknownFaces")
        bool lichter = false;    // Lichter schaetzen, experimentell ("BhedBspEstimateLights")
    } bspRueck;
    struct BspRueckLetzt {       // das letzte Ergebnis (Anzeige, Selbsttest)
        std::size_t treffer = 0;
        std::size_t forceMeta = 0;
        std::size_t pinsel = 0;
        std::size_t dreiecke = 0;
        double maxFehler = 0;
        double sekunden = 0;
        std::size_t lichter = 0;   // Punkt 4: geschaetzte Lichter
        // Stufe 8b Punkt 8: Modell-Index
        std::size_t indexModelle = 0;
        std::size_t indexNeu = 0;
        std::size_t kandidatenOrdner = 0;
        std::size_t kandidaten = 0;
    } bspRueckLetzt;
    // Stufe 8a Punkt 1: Fortschrittsfenster (zu bis zum naechsten Bau), Restzeit-Text, Meldungen im Selbsttest
    bool bauFortschrittZu = false;
    std::string bauFortschrittText;
    int bauMeldungenTest = 0;
    // Stufe 5: der Optimierer (eigene Funktion, nicht in NRC; Einstellungsseite "Optimizer")
    struct OptimiererEinstellungen {
        int knappProzent = 90;             // ab diesem Anteil "knapp" ("OptimizerNear")
        bool nachBau = false;              // nach jedem Bau pruefen ("OptimizerAfterBuild")
        int regelwerk = -1;                // -1 automatisch (Spiel der Karte), sonst karte::GrenzSpiel ("OptimizerRules")
        int portalSchwelle = 8000;         // VIS-Hinweis ab so vielen Portalen ("OptimizerPortals")
        int dreieckSchwelle = 20000;       // Dreieck-Hinweis ab ("OptimizerTriangles")
    } optimierer;
    // Stufe 7: Echtzeit-Licht (eigene Funktion, nicht in NRC; Build-Menue)
    struct EchtLichtZustand {
        bool an = false;                   // Vorschau in der Kamera ("RealtimeLight")
        bool backen = false;               // Schnell-Backen im Hintergrund ("QuickBake")
        bool patchSchatten = false;        // Stufe 9c: Patches werfen Schatten (q3map2 -patchshadows; Vorschau und Schnell-Backen)
        double pauseSekunden = 2.0;        // so lange ohne Aenderung, dann backen
        // fuer den Selbsttest und die Anzeige
        std::size_t lichter = 0;           // Lichter der Karte
        std::size_t gezeichnet = 0;        // davon im letzten Bild
        std::size_t schattenNeu = 0;       // Schattenwuerfel neu gerendert (gesamt)
        double lichtMs = 0;                // Lichtliste + Schatten im letzten Bild (Prozessor)
        // Schnell-Backen
        int bauStand = 0;                  // 0 aus, 1 wartet, 2 laeuft, 3 fertig, 4 abgebrochen, 5 Fehler
        double bauSekunden = 0;            // Dauer des letzten Baus
        std::size_t baueAbgebrochen = 0;   // abgebrochene Baue (neue Aenderung)
        std::string bauOrdner;             // der temporaere Ordner des letzten Baus
        bool gebackenZeigen = false;       // die echten Lightmaps statt der Vorschau
    } echtLicht;
    std::array<FensterBeobachter, 4> beobachter;
    std::array<karte::Blick, 4> blicke{karte::Blick(false), karte::Blick(false), karte::Blick(false), karte::Blick(true)};   // m_view
    int letzterBeobachter = -1;            // RadiantWindowObserver::m_latestObserver
    float letzteX = 0;                     // m_latestPosition (Fensterpunkte)
    float letzteY = 0;

    // --- Texturbrowser (Punkt 8, texwindow.cpp) ---
    karte::AktiveShader aktiveShader;      // g_ActiveShaders (shaders.cpp)
    karte::FlaechenTextur texturAblage;    // g_faceTextureClipboard (surfacedialog.cpp)
    karte::TexturBrowser texBro;           // g_TexBro
    karte::Teiler texTeiler;               // QSplitter "TextureBrowser/splitter"
    std::vector<int> texGroessen;          // gemerkte Groessen (sonst { 100, 800 })
    std::string texFilterText;             // m_filter_entry
    // Find / Replace Texture(s) (findtexturedialog.cpp)
    bool findenOffen = false;
    std::string findenText;                // m_strFind (ohne "textures/")
    std::string ersetzenText;              // m_strReplace
    bool findenNurAuswahl = false;         // m_bSelectedOnly
    bool findenAktiv = true;               // g_bFindActive (welches Feld zuletzt den Fokus hatte)
    // Shader Info (DoShaderInfoDlg) und der Shader-Text (DoShaderView, TextEditor)
    bool shaderInfoOffen = false;
    std::string shaderInfoName;
    std::string shaderInfoDatei;
    struct ShaderText {
        bool offen = false;
        std::string datei;                 // pathFull (Archiv "::" Datei oder Ordner + Datei)
        std::string pfad;                  // zum Speichern (nur lose Dateien)
        std::string text;
        bool editierbar = false;           // pathIsDir
        bool geaendert = false;            // document()->isModified()
        std::string suchen;                // QLineEdit_search
        std::size_t cursor = 0;            // Lage des Cursors / der Auswahl
        std::size_t auswahlEnde = 0;
        int springen = -1;                 // Zeile, zu der gerollt wird (-1: keine)
        int springenBilder = 0;            // so viele Bilder lang (die Groesse des Inhalts kommt erst nach dem ersten)
        bool frageSchliessen = false;      // ensure_saved
    } shaderText;

    // --- Entitaeten (Punkt 9) ---
    EntitaetenOberflaeche ent;
};

Editor& editor();

// Beschriftung von Menues, Knoepfen und Dialogen: in der Programmsprache,
// mit "Begriffe wie NetRadiant" (Einstellung karte nrcBegriffe) immer NRCs
// englischer Wortlaut. Tooltips nehmen tr() (immer Programmsprache).
[[nodiscard]] const char* trK(i18n::Str id);
[[nodiscard]] bool nrcBegriffe();
void setzeNrcBegriffe(bool an);

// Den Arbeitsbereich zeichnen (aus app.cpp, rechts neben der Modusleiste).
void zeichne(float hoehe);
// Stufe 3 Punkt 0b: die NRC-Menueleiste ist im Modus Karten-Editor die des
// Programmfensters. hauptleisteStilAn/Aus umklammern dessen Begin (Hoehe 21),
// zeichneHauptmenue zeichnet sie darin; bildBeginnen leert die Listen des Bildes.
void hauptleisteStilAn();
void hauptleisteStilAus();
void zeichneHauptmenue();
void bildBeginnen();

// --- Befehle (app_karte_befehle.cpp) ---
// Einen NRC-Befehl ausfuehren (Menue, Knopf, Taste). false: unbekannt oder in
// dieser Stufe noch nicht nachgebaut.
bool ausfuehren(const std::string& name);
// Ist der Befehl in dieser Stufe nachgebaut (sonst sichtbar, aber ausgegraut)?
[[nodiscard]] bool nachgebaut(const std::string& name);
// Zustand eines Umschalters (Toggle, Haken im Menue, gedrueckter Knopf).
[[nodiscard]] bool angehakt(const std::string& name);
// Alle nachgebauten Befehle (fuer den Selbsttest).
[[nodiscard]] std::vector<std::string> nachgebauteBefehle();
// Die Tastenkuerzel dieses Bildes auswerten (QShortcutMap::find ueber
// QWindowsKeyMapper::possibleKeys) - nur, solange der Karten-Editor den Fokus
// hat, nichts getippt wird und keine Maustaste gedrueckt ist
// (RadiantQMainWindow::event: "block shortcuts while mouse buttons are pressed").
void tastenVerarbeiten();
// Fuer den Selbsttest: eine feste Tastaturbelegung statt der von Windows
// (ImGuiKey als int); false = die echte Belegung nehmen.
extern std::function<bool(int, befehle::TastenBelegung&, bool&, bool&)> tastaturErsatz;
// Die Belegung einer ImGui-Taste (Ersatz oder Windows).
bool tastenBelegung(int imguiKey, befehle::TastenBelegung& out, bool& ziffernblock, bool& erweitertesReturn);
// Die Qt-Zusatztasten des Augenblicks (kShift|kCtrl|kAlt|kMeta).
[[nodiscard]] std::uint32_t qtZusatztasten();

// --- Menues, Werkzeugleisten, Statuszeile (app_karte_menue.cpp) ---
// Alle Lagen in logischen Punkten ab der Ecke des Arbeitsbereichs.
void zeichneMenueleiste();
void zeichneHauptleiste(int y, int breite);
void zeichneRechteLeisten(int x, int y, int hoehe);   // Filter- und Plugin-Leiste
void zeichneStatuszeile(int y, int breite);
// --- 2D-Ansichten (app_karte_xy.cpp) ---
// Die 2D-Ansicht index (0 XY, 1 XZ, 2 YZ) im aktuellen Kindfenster zeichnen
// und bedienen.
void xyZeichnen(int index);
// NextView, ViewTop/Front/Side, ZoomIn/Out, Zoom100, CenterXYView,
// XYFocusOnSelected (xywindow.cpp:2266-2278).
void xyBefehl(const std::string& name);
// Stufe 8b Punkt 2: Split-Fenster frei belegen (wahl 0 Kamera, 1 XY, 2 XZ, 3 YZ)
void splitPlatzWaehlen(int platz, int wahl);
// Stufe 8b Punkt 3: Im Editor spielen (app_karte_spielen.cpp)
[[nodiscard]] bool spielenAktiv();
void spielenStarten();
void spielenBeenden();
void spielenSchritt(int msec);                              // Pmove mit spielen.eingabe
void spielenBild(const ImVec2& o, const ImVec2& groesse);   // aus kameraZeichnen: Eingabe, Zeit, Anzeige
// Stufe 8b Punkt 4: Logik-Ansicht (app_karte_logik.cpp)
// Stufe 8b Punkt 5: Ansichtsmodi der Kamera (app_karte_ansichtsmodi.cpp)
void ansichtsmodiZeichnen(const float* matrix, const float auge[3]);   // in die Kamera, nach der Szene
void ansichtsmodiAnzeige(void* drawList, float x, float y);            // die Zeilen oben links
void logikZeigen();
void logikZeichnen();
void logikKnotenWaehlen(int nr);                  // Klick: waehlen und die Ansichten zentrieren
bool logikSkriptOeffnen(const std::string& wert); // Doppelklick: das ICARUS-Skript oeffnen (false: fehlt, Meldung)
[[nodiscard]] karte::Vektor3f xyMittelpunkt();   // XYWnd_Centralize: Mitte der Auswahl bzw. der Karte
// Map_StartPosition / FocusViews (map.cpp:456-485): Kamera auf den Start,
// alle 2D-Ansichten dorthin.
void startPosition();
// Die Ansichten, die es in der aktuellen Anordnung gibt (forEachXYWnd).
[[nodiscard]] std::vector<int> xyFenster();
// Ein Stand der Szene (aendert sich mit jedem Befehl, Undo, neuer Karte,
// neuer Auswahl und den Show-Schaltern).
[[nodiscard]] std::size_t szeneStand();
// Der Stand nur der Geometrie (ohne Auswahl und Anzeige): Pinsel und Patches
// bauen ihre Kanten und Dreiecke nur neu, wenn er sich aendert (Punkt 12).
[[nodiscard]] std::size_t geometrieStand();
// getBoundsSelected: false, wenn nichts ausgewaehlt ist.
bool auswahlGrenzen(karte::Kasten& b);
// Die Rad-Ereignisse dieses Bilds (Qt: ein wheelEvent je Ereignis): je
// Ereignis das Vorzeichen (|y| > |x| ? y : x).
[[nodiscard]] std::vector<int> radEreignisse();
// --- Auswaehlen und Bearbeiten (app_karte_auswahl.cpp) ---
// RadiantWindowObserver: knopf 0 links, 1 rechts, 2 Mitte; x/y in Fensterpunkten.
void beobachterGroesse(int fenster, int breite, int hoehe);   // onSizeChanged
void beobachterDruck(int fenster, float x, float y, int knopf, const karte::Zusatz& z);   // onMouseDown
void beobachterBewegung(int fenster, float x, float y);                                  // onMouseMotion
void beobachterLoslassen(int fenster, float x, float y, int knopf, const karte::Zusatz& z);   // onMouseUp
// onModifierDown / onModifierUp: g_modifiers aus der Tastatur dieses Bilds.
void zusatztastenPruefen();
// Die Befehle aus select.cpp und tools.cpp (Undo, Copy, Clone, Delete, ...).
void auswahlBefehl(const std::string& name);
[[nodiscard]] bool auswahlBefehlAn(const std::string& name);   // Umschalter (Werkzeuge, HideSelected)
// Was der Manipulator in einer Ansicht zeichnet (renderSolid).
[[nodiscard]] karte::ManipZeichnung manipulatorZeichnung(int fenster);
// Die Auswahl hat sich geaendert (setSelectedAll ... m_manipulator->setSelected(false),
// ComponentMode_SelectionChanged).
void auswahlGeaendert();
// Was die Komponenten in einer Ansicht zeichnen: BrushInstance::renderComponents
// / renderComponentsSelected (brush.h:3542-3596), Brush::renderComponents
// (:1823-1846), PatchInstance::renderComponents / renderComponentsSelected
// (patch.h:1434-1501), Patch::render_component (:629-637).
struct KomponentenZeichnung {
    std::vector<karte::Linie> punkte;       // $POINT (6 Bildpunkte): Ecken, Kantenmitten, Schwerpunkte, Patch-Punkte
    std::vector<karte::Linie> tiefPunkte;   // $DEEPPOINT (6): Ecken in der Kamera, verdeckte in colour_occluded
    std::vector<karte::Linie> gitter;       // $LATTICE: das Kontrollgitter der Patches (1, 0.5, 0)
    std::vector<karte::Linie> gewaehlt;     // $SELPOINT (4): die gewaehlten Komponenten (colour_selected)
};
[[nodiscard]] KomponentenZeichnung komponentenZeichnung(const karte::Blick& blick);
// BrushInstance::renderClipPlane (brush.h:3598-3602) mit BrushClipPlane
// (brush.h:3050-3093): im Clipper-Werkzeug je gewaehltem Pinsel die Wicklung
// der Schnittebene; die Normale fuer den Richtungsstrich in 2D.
struct ClipperFlaeche {
    std::vector<karte::Vektor3f> punkte;
    karte::Vektor3f normale;
    karte::Vektor3f mitte;
};
[[nodiscard]] std::vector<ClipperFlaeche> clipperFlaechen();
// Was die CSG-Befehle aus der Oberflaeche brauchen (Browser-Shader, Caulk, Raster).
[[nodiscard]] karte::CsgUmgebung csgUmgebung();
// Clipper_tryDoubleclick / Clipper_tryDoubleclickedCut (clippertool.cpp:155-172)
void clipperDoppelklickDruck();
void clipperDoppelklickLoslassen();

// --- Texturbrowser (app_karte_texturen.cpp) ---
// Der Texturbrowser im aktuellen Kindfenster (breite x hoehe logische Punkte).
void texturBrowserZeichnen(int breite, int hoehe);
// Je Bild vor dem Zeichnen: Spiel gewechselt, Shader der Karte anfordern,
// welche in Gebrauch sind (FaceShader::instanceAttach -> SetInUse).
void texturenAktualisieren();
// Find / Replace, Shader Info und der Shader-Text.
void texturDialogeZeichnen();
// Die Befehle aus texwindow.cpp (ShowInUse, ShowAllTextures, Toggle...,
// RefreshShaders) und findtexturedialog.cpp (FindReplaceTextures).
void texturBefehl(const std::string& name);
[[nodiscard]] bool texturBefehlAn(const std::string& name);
// TextureBrowser_SetSelectedShader: Auswahl, Status, Fokus, Find-Dialog.
void texturShaderWaehlen(const std::string& shader);
// SelectTexture (Klick in den Browser): texturieren = false wie M1 (setzt
// den Shader auf die Auswahl), true wie M3. mx/my in Geraetepunkten.
void texturKlick(int mx, int my, bool nurWaehlen);
// TextureBrowser_ShowDirectory mit Konsole ("Showing N shaders.").
void texturVerzeichnisZeigen(const std::string& dir);
// TextureBrowser_exportTitle: "Textures: <dir>" bzw. "Textures: all" (Programmsprache).
[[nodiscard]] std::string texturTitel();
// Fuer den Selbsttest: wo der Texturbrowser steht (Bildpunkte) und wo eine
// Textur darin liegt (Mitte, Bildpunkte; false wenn nicht sichtbar).
struct TexturBrowserLage {
    float ansichtX = 0;   // linke obere Ecke der GL-Flaeche
    float ansichtY = 0;
    int breite = 0;       // m_width
    int hoehe = 0;        // m_height
    bool gezeichnet = false;
};
[[nodiscard]] const TexturBrowserLage& texturBrowserLage();
// Fuer den Selbsttest: die GL-Flaeche gilt als neu (initializeGL beim
// naechsten Zeichnen, TextureBrowser_ShowStartupShaders).
void texturGlNeu();
bool texturMitte(const std::string& shader, ImVec2& mitte);

// --- Entitaeten (app_karte_entitaeten.cpp) ---
// Die Seite "Entities" des Gruppendialogs (EntityInspector_constructWindow)
// im aktuellen Kindfenster (breite x hoehe logische Punkte).
void entitaetenSeiteZeichnen(int breite, int hoehe);
// Entity List, Entitaetenmenue, Lichtstaerke- und Farbdialog (jedes Bild).
void entitaetenDialogeZeichnen();
// XYWnd::OnContextMenu / context_menu_show: das Entitaetenmenue oeffnen.
void entitaetenMenueOeffnen(bool kamera);
// Die Befehle aus entity.cpp, select.cpp und mainframe.cpp (Entity List).
void entitaetBefehl(const std::string& name);
// Fuer den Selbsttest: das Menue (Gruppen wie EntityClassMenuInserter).
[[nodiscard]] std::vector<karte::EntitaetMenuePunkt> entitaetenMenuePunkte();
// Ein Eintrag des Entitaetenmenues wurde gewaehlt (entitycreate_activated)
// bzw. mit Rechtsklick (Scene_EntitySetClassname_Selected).
void entitaetMenueGewaehlt(const std::string& klasse);
// Der Farbdialog (color_dialog): fertig wird nur bei OK gerufen.
void farbDialogOeffnen(const karte::Vektor3f& farbe, std::function<void(const karte::Vektor3f&)> fertig);
// Die Schriften der Qt-Widgets: Tahoma 8 pt (MS Shell Dlg 2, Qt 5 systemDefaultFont),
// fett fuer die Kommentare; aus buildFonts.
void setzeDialogSchrift(ImFont* normal, ImFont* fett);
// Die Dialogschrift (Tahoma 8 pt, fm.height() 13 logische Punkte) setzen/zuruecknehmen.
void dialogSchriftAn(bool fett = false);
void dialogSchriftAus();

// --- 3D-Kamera (app_karte_cam.cpp) ---
// Fuer den Texturbrowser: das Bild eines Shaders (ID3D11ShaderResourceView,
// nullptr solange es laedt), fertige Bilder hochladen, alles neu laden
// (RefreshShaders).
[[nodiscard]] void* texturBild(const std::string& shader);
// Fuer den Selbsttest (Stufe 7 Punkt 6f): zeigt die Kamera fuer den Shader das
// Ersatzbild (notex / shadernotex, "SHADER NOT FOUND")? -1 noch nicht geladen.
[[nodiscard]] int kameraBildErsatz(const std::string& shader);
void texturBilderAbholen();
// Der Zustand eines Shaders wie in der Kamera (Textured): Bild, Alphatest,
// Durchsicht; false, solange das Bild noch geladen wird (app_karte_cam.cpp).
bool shaderZustand(const std::string& shader, kartegpu::Zustand3& z, bool& durchsichtig);
void texturBilderNeuLaden();
// Die Kamera im aktuellen Kindfenster zeichnen und bedienen.
void kameraZeichnen();
// Die Befehle der Kamera (camwindow.cpp:2487-2570, 1332-1433).
void kameraBefehl(const std::string& name);
// Gehaltene Tasten (KeyEvents) vor den Kuerzeln: true, wenn die Taste
// key (ImGuiKey) in diesem Bild von einem KeyEvent verbraucht wird.
bool kameraTasteVerbraucht(int key);
// Haengt ein KeyEvent-Befehl gerade an seiner Taste (diskret/Freelook)?
[[nodiscard]] bool befehlVerbunden(const std::string& name);
// Ist name ein nachgebautes KeyEvent (GlobalKeyEvents_insert, camwindow.cpp:1332-1433)?
[[nodiscard]] bool tastenEreignisNachgebaut(const std::string& name);
// GlobalPressedKeys_releaseAll (Fokuswechsel)
void kameraTastenLoslassen();
// Braucht die Kamera Bilder (Bewegung, Freelook, Texturen laden)?
[[nodiscard]] bool kameraBrauchtBilder();
// Fuer den Selbsttest: sind alle angeforderten Bilder geladen?
[[nodiscard]] bool kameraBilderFertig();
// Fuer den Selbsttest (Punkt 12): Groesse der Kamera- und der 2D-Szenen als Text.
[[nodiscard]] std::string kameraSzeneMass();
[[nodiscard]] std::string xySzeneMass();
// DisableFreeMove (auch bei Fokusverlust)
void freelookBeenden();
// Fuer den Selbsttest: ein Bildpunkt der Kamera (Geraetepunkte im Fenster)
// und die Durchschnittsfarbe eines geladenen Shaders (qtexture_t::color).
bool kameraPixel(int x, int y, unsigned char rgba[4]);
bool kameraBild(std::vector<unsigned char>& rgba, int& breite, int& hoehe);   // Stufe 9c: Lichtmessung
bool xyPixel(int index, int x, int y, unsigned char rgba[4]);   // Selbsttest: ein Bildpunkt der 2D-Ansicht
bool kartenBildRgba(const std::string& name, std::vector<unsigned char>& rgba, int& breite, int& hoehe);   // QERApp_LoadImage
[[nodiscard]] int xyAbtastungen(int index);                      // Selbsttest: MSAA des Renderziels
bool kameraTexturFarbe(const std::string& shader, float rgb[3]);
void* kameraTexturBild(const std::string& shader);   // Stufe 7: SRV des Shaderbilds (nullptr: noch nicht da)

// Ein QSplitter im aktuellen Kindfenster (Lagen ab dessen Ecke): ordnet an,
// zeichnet die Griffe und ruft je Kind kinder[i](x, y, b, h) in logischen Punkten.
using TeilerInhalt = std::function<void(int x, int y, int b, int h)>;
void teilerImFenster(const char* id, karte::Teiler& t, bool senkrecht, int b, int h, const std::vector<TeilerInhalt>& kinder);
// Menueeintraege einer Liste ab erster (bis zum passenden Ende) zeichnen und
// ausfuehren (QMenu); ein Symbol im Stil der Werkzeugleisten.
void menueEintraege(const std::vector<befehle::MenueEintrag>& liste, std::size_t erster);
void leistenSymbol(ImDrawList* dl, const char* name, ImVec2 a, float g, bool aktiv);
// Alt + Buchstabe (QMenuBar-Mnemonik): das Menue oeffnen; false ohne Treffer.
bool leistenMnemonik(unsigned zeichen);

// --- QMenu wie NRC (app_karte_qtmenue.cpp): Qt 5.15, Fusion, dark.qss ---
// Art einer QAction im Menue: normal, pruefbar (Kaestchen) oder exklusiv
// (QActionGroup: Punkt).
enum class QtArt { Normal, Pruefbar, Exklusiv };
struct QtPunkt {
    const char* id = "";          // ImGui-Beschriftung (sichtbarer Text, dahinter "##..."/"###...")
    const char* nrc = nullptr;    // NRCs Text mit '&' (Mnemonik), nullptr = keine
    const char* taste = nullptr;  // Tastenkuerzel (Spalte rechts)
    bool haken = false;
    bool aktiv = true;
    QtArt art = QtArt::Normal;
    const char* bild = nullptr;   // Symbol (karteIcon)
    const float* farbe = nullptr; // Farbfeld als Symbol (RGB 0..1)
    std::string tooltip;
};
// Ein Menue der Menueleiste oder ein Untermenue (QMenu, abreissbar mit
// "Detachable Menus"); qtMenueEnde nur, wenn es offen ist.
bool qtMenueAnfang(const char* id, bool aktiv = true, const char* nrc = nullptr, bool abriss = true);
[[nodiscard]] bool qtMenueAbgerissen();   // die Abreisskante des obersten Menues geklickt (einmal)
bool qtAbrissFensterAnfang(const char* titel, const char* kennung, bool* offen, float x, float y, bool neu);
void qtAbrissFensterEnde();
void qtMenueEnde();
// Ein Kontextmenue (QMenu::popup an der Maus), offen mit ImGui::OpenPopup(id).
bool qtPopupAnfang(const char* id, bool abriss = false);
void qtPopupEnde();
bool qtPunkt(const QtPunkt& p);
void qtTrenner();
// QCheckBox wie Fusion an der Schreibstelle: Kaestchen 14 senkrecht mittig in
// einer Zeile der Hoehe zeileH (logisch), Text 5 dahinter.
bool qtHaken(const char* id, bool* wert, float zeileH);
// Die Mnemonik unterstreichen (Lage im Text, -1 = keine) bzw. ihre Lage,
// wenn text NRCs Wortlaut nrc (mit '&') ist.
void qtUnterstreichen(ImDrawList* dl, ImVec2 textPos, const std::string& text, int lage, unsigned int farbe);
[[nodiscard]] int qtMnemonik(const char* nrc, const std::string& text);
// MaximizeView (Taste, Menue) und der Aufbau der Teiler fuer die Anordnung.
void maximierenUmschalten();
void teilerAufbauen();
// Ein Rechteck fuer den Selbsttest merken und in Bildpunkte umrechnen.
void merkeFeld(const std::string& name, int x, int y, int b, int h);
[[nodiscard]] ImVec2 bildpunkt(int x, int y);
// Das Symbol <name> (NRCs bitmaps/<name>.svg, hier als eigene
// Vektorzeichnung) an der Schreibstelle; false, wenn es fehlt.
bool symbol(const char* name, float groesse, bool hell = true);
// --- Vektorsymbole (karte_vektoricons.cpp) ---
bool karteIcon(ImDrawList* dl, const char* name, ImVec2 pos, float groesse, unsigned int farbe);
ImVec4 karteIconFarbe(const char* name);
// --- Dialoge (app_karte_dialoge.cpp) ---
void zeichneEinstellungen();   // "NetRadiant Preferences" (P)
void zeichneTastenDialog();    // "Mapped Commands" (Ctrl+Shift+P)
void zeichneCsgWerkzeug();     // "CSG Tool" (app_karte_csg.cpp)
void seitenDialogOeffnen(karte::Grundform art);   // DoSides (app_karte_formen.cpp)
void zeichneSeitenDialog();
// Stufe 2, Punkt 8 (app_karte_datei.cpp): Import, Save selected/region,
// Select inside/touching, Expand To Primitives, Region, Find brush, Map Info
void dateiBefehl(const std::string& name);
bool dateiBefehlAn(const std::string& name);
void zeichnePinselSuche();
void zeichneKartenInfo();
bool karteImportierenAus(const std::string& pfad);    // ImportMap ohne Dateiwahl
bool auswahlSpeichernUnter(const std::string& pfad);  // Map_SaveSelected
bool regionSpeichernUnter(const std::string& pfad);   // Map_SaveRegion
// EverySecondTimer (mainframe.cpp:727-746): jedes Bild aufrufen
void autosaveJedeSekunde();
// Stufe 2, Punkt 10b (app_karte_modelle.cpp): die Modelle der misc_model
[[nodiscard]] const karte::ModellQuelle& kartenModelle();
void modelleLeeren();   // FlushReferences (Map_Free) / RefreshReferences
[[nodiscard]] karte::ModellAnsicht kartenModellPfad(const std::string& pfad);
// Stufe 2, Punkt 10c (app_karte_modellbrowser.cpp): der Modell-Browser
void modellBrowserZeichnen(int breite, int hoehe);
void modellBrowserBaumBauen();      // ModelBrowser_constructTree
void modellBrowserLagenNeu();       // CellSizeImport
[[nodiscard]] std::string modellBrowserOrdner();
[[nodiscard]] std::vector<std::string> modellBrowserDateien();
[[nodiscard]] int modellBrowserAktuell();
// Stufe 2, Punkt 10 (app_karte_ebenen.cpp): der Ebenen-Browser, Strg+Q, Umschalt+Q
void ebenenSeiteZeichnen(int breite, int hoehe);
void ebenenBefehl(const std::string& name);
void zeichneEbenenMenue();
// QInputDialog::getText: fertig( text ) nur bei OK
void qtEingabe(const std::string& titel, const std::string& beschriftung, const std::string& vorgabe, std::function<void(const std::string&)> fertig);
void zeichneQtEingabe();
// Stufe 2, Punkt 9 (app_karte_filter.cpp): die Filter
void filterBefehl(const std::string& name);
bool filterBefehlAn(const std::string& name);
void filterPruefen();                       // jedes Bild vor dem Zeichnen: Filterables neu
[[nodiscard]] std::size_t filterSchluesselJetzt();
void filterleisteBetreten();                // QEvent::Enter der Filterleiste
void filterleisteRechtsklick(const befehle::Knopf& k);
// Entity_createFromSelection mit den Nachfragen (app_karte_entitaeten.cpp)
void entitaetErzeugenAusAuswahl(const std::string& name, const karte::Vektor3f& ursprung);
// MapResource_saveFile / MapResource_loadFile (referencecache.cpp:64-111):
// "Open file X for write/read..." mit success / failure; Text wie
// TextFileOutputStream (CR LF).
bool karteDateiSchreiben(const std::string& pfad, const std::string& text);
bool karteDateiLesen(const std::string& pfad, std::string& text);
// map_open / map_save: der Startordner und die Dateitypen
[[nodiscard]] std::string kartenStartOrdner();
[[nodiscard]] const char* kartenDateiFilter();
// g_qeglobals.m_userGamePath + "maps/" (Autosave einer unbenannten Karte)
[[nodiscard]] std::string kartenBenutzerOrdner();
void arbitraerDialogZeigen(bool drehen);   // DoRotateDlg / DoScaleDlg (app_karte_transform.cpp)
void zeichneArbitraerDialoge();
void patchDialogOeffnen(int art, karte::PatchVorlage vorlage = karte::PatchVorlage::Ebene);   // app_karte_patch.cpp
void zeichnePatchDialog();
void siUmschalten();                 // SurfaceInspector_toggleShown (app_karte_si.cpp)
void siEinpassenBefehl(int art);     // FitTexture, FitTextureWidth, -Height, -WidthOnly, -HeightOnly
void siProjizierenBefehl(int art);   // TextureProjectAxial / Ortho / Cam
void zeichneSurfaceInspector();
// qt_MessageBox (app_karte_tags.cpp): OK bzw. Yes/No; danach( true ) bei OK/Yes
void qtMeldung(const std::string& titel, const std::string& text, MeldungArt art, bool jaNein, std::function<void(bool)> danach);
void zeichneQtMeldung();
void tagsLaden();                 // TextureBrowser_checkTagFile
void tagsShaderGewaehlt();        // TextureBrowser_tagsSetCheckboxesForShader
bool tagBefehlAn(const std::string& name);
void tagBefehl(const std::string& name);
void tagsReiterZeichnen(ImVec2 a, ImVec2 c, float sc);
void tagsListeZeichnen(ImVec2 a, ImVec2 c, float sc);
// Einstellungen des Editors lesen/schreiben (Settings::karte).
void einstellungenLaden();
// Stufe 7 Punkt 6f: der Fenstertitel im Map-Editor wie NRC (Sys_SetTitle, qe3.cpp:262-270):
// der Name der Karte, " *" bei Aenderung.
[[nodiscard]] std::string fensterTitel();
// Stufe 7 Punkt 6b: Knopfgroesse (0 klein, 1 normal, 2 gross) und Menuestil
// (true: wie NRC) setzen und gleich speichern.
void knopfGroesseSetzen(int stufe);
void menueStilSetzen(bool nrc);
void einstellungenSpeichern();
// true, solange der Karten-Editor Tastatur und Maus fuer sich hat (der
// Arbeitsbereich ist sichtbar). Dann gelten die BehavEd-Kuerzel nicht.
[[nodiscard]] bool aktiv();

// --- Datei (radiant/map.cpp) ---
// NewMap (map.cpp:1813): mit Rueckfrage, wenn geaendert.
void neueKarte();
void letzteKarteBeimStart();   // main.cpp:462 (g_bLoadLastMap)
// Stufe 9 Punkt 12: Sitzungsdatei pruefen - true, wenn die Frage nach dem Wiederherstellen offen ist
bool absturzPruefen();
[[nodiscard]] std::string sitzungsPfad();
void zuletztEintragen(const std::string& pfad);   // MRU_AddFile
[[nodiscard]] std::string einstellungsFeld(const std::string& id);   // Selbsttest: Text eines Feldes der Einstellungen
[[nodiscard]] int einstellungsSeitenAnzahl();     // Selbsttest: Seiten der Einstellungen
void einstellungsSeiteZeigen(int seite);          // Selbsttest: eine Seite zeigen
[[nodiscard]] const karte::SpielBeschreibung& kartenSpiel();   // g_pGameDescription
[[nodiscard]] std::string kartenFsGame();                       // gamename_get
[[nodiscard]] int kartenSpielNummer();                          // kRegelwerk* des Editors (gamefile, sonst Regelwerk)
[[nodiscard]] std::string enginePfadAnzeige();                  // EnginePath_get (leer: aus den Spielordnern)
[[nodiscard]] std::vector<std::string> kartenVfsOrdner();       // QE_InitVFS
// Engine Path, Extra Resource Paths, fs_game oder Mapping mode geaendert:
// EnginePath_Unrealise / Realise - das Spiel neu laden, die Karte aus der Datei
// (MapEntityClasses). Fragt vorher wie ConfirmModified, wenn die Karte geaendert ist.
void spielNeuRealisieren(const char* titel, std::function<void()> aendern);
void radiantNeustart();                                         // Radiant_Restart (Select the game)
void zeichneGlobaleEinstellungen();                             // CGameDialog "Global Preferences"
// Stufe 2, Punkt 11c
void zeichneAbrisse();                                          // QTornOffMenu
void abreissen(std::size_t start, const std::string& kennung, const std::string& titel, float x, float y);
void zeichneUeber();                                            // DoAbout
void zeichneEnginePfadDialog();                                 // PathsDialog
void enginePfadPruefen();                                       // EnginePath_verify
void zeichneSchriftDialog();                                    // QFontDialog
void hintergrundBildSetzen(int index, const std::string& relativ);   // BackgroundImage::set ohne Dateidialog
void hintergrundBildDialog();                                   // WXY_SetBackgroundImage
// Selbsttest: der angezeigte Wert im Surface Inspector (0/1 Shift, 2/3 Scale, 4 Rotate)
[[nodiscard]] float siAnzeigeWert(int i);
void hintergrundBildFrei(int index);                            // BackgroundImage::free_tex (Selbsttest: ohne Bild beginnen)
[[nodiscard]] ImFont* glSchrift();                              // GlobalOpenGL().m_font
[[nodiscard]] float glSchriftPx();                              // ImGui-Groesse der OpenGL-Schrift (96 dpi)
void glSchriftSetzen(const std::string& familie, int punkte);   // OpenGLFont_select nach OK
[[nodiscard]] std::string bugReportUrl();                       // OpenBugReportURL
void kartenUrlOeffnen(const std::string& url);                  // OpenURL
[[nodiscard]] bool kartenBefehlStarten(const std::string& zeile);   // Q_Exec
// file_dialog (gtkmisc): Dateiwahl von Windows, im Selbsttest ersetzbar
[[nodiscard]] std::string kartenDateiDialog(bool oeffnen, const char* titel, const char* filter, const std::string& start, const std::string& name = "");
// button_clicked_entry_browse_directory: Ordnerwahl von Windows, im Selbsttest ersetzbar (derselbe Ersatz)
[[nodiscard]] std::string kartenOrdnerDialog(const char* titel, const std::string& start);
// NRC#164: ein Ordner (Engine-Pfad, Suchpfad) absolut zum Programmordner, mit '/' am Ende
[[nodiscard]] std::string ordnerAbsolut(const std::string& pfad);
void shaderAnsehen(const std::string& datei, const std::string& name, bool extern_);   // DoShaderView
void beimBeenden();            // main.cpp:476 (g_strLastMap)
// OpenMap (map.cpp:1841) mit Dateiwahl; karteOeffnen ohne Dialog (fuer
// Kommandozeile, zuletzt geoeffnet und den Selbsttest).
void karteOeffnenDialog();
bool karteOeffnen(const std::string& pfad);
// SaveMap (map.cpp:1879) und SaveMapAs (map.cpp:1875).
bool karteSpeichern();
bool karteSpeichernUnterDialog();
bool karteSpeichernUnter(const std::string& pfad);
// MRU_Activate ueber LoadMRU (mru.cpp:144, mit ConfirmModified "Open Map").
void zuletztOeffnen(std::size_t index);
// Map_Modified
[[nodiscard]] bool geaendert();
// ConfirmModified( titel ) und danach (Save / Discard / Cancel).
void rueckfrageDann(const char* titel, std::function<void()> danach);
// Entity_reloadDefinitions: die Klassen neu lesen und die Karte neu laden
// (MapEntityClasses::unrealise/realise).
void entitaetenKlassenNeuLaden();

// --- Karten bauen (Stufe 3, app_karte_bau.cpp; build.cpp, qe3.cpp, watchbsp.cpp) ---
inline constexpr const char* kBauMonitorAdresse = "127.0.0.1:39000";   // RADIANT_MONITOR_ADDRESS
void bauMenueSicherstellen();          // BuildMenu_Construct/LoadBuildMenu fuer das Spiel des Editors
void bauMenueSpeichern();              // SaveBuildMenu (BuildMenu_Destroy)
void bauMenueNeuLaden();               // Reset: build_commands_clear + LoadBuildMenu
[[nodiscard]] std::string bauMenueDateiPfad();   // g_buildMenuFullPah
[[nodiscard]] karte::BauVariablen bauVariablen();   // build_init_variables
[[nodiscard]] std::string bauSpielName();          // [GameName]
[[nodiscard]] std::string bauKartenPfad();         // Map_Name der gebauten Datei
void bauMenueEintraegeZeichnen();      // Build_constructMenu
void bauStarten(std::size_t index);    // RunBSP
void bauZuletztStarten();              // Build_runRecentExecutedBuild
// Stufe 9 Punkt 1 (eigene Funktion, nicht in NRC): Build > Test in game - bauen bzw. das
// Schnell-Backen nehmen, die .bsp in den eigenen Testordner, das Spiel mit +devmap starten
void imSpielTesten(const std::string& exe);
[[nodiscard]] std::string spielTestOrdner();
[[nodiscard]] std::string standardTestExe();   // die Engine aus den Einstellungen (SP bzw. MP)
void bauAusfuehren(const std::vector<std::string>& befehle);   // BuildMonitor_Run
// Stufe 4: eigene Befehle im Hintergrund wie ein Build-Eintrag (Konsole,
// Statuszeile, Schlusszeile); danach(art) mit 0 fertig, 1 gestoppt, 2 abgebrochen.
void bauEigeneBefehle(const std::string& eintrag, const std::vector<std::string>& befehle, std::function<void(int)> danach);
// --- Stufe 5: der Optimierer (app_karte_optimierer.cpp) ---
void optimiererOeffnen();                // Build > Optimizer... (Fenster auf, Pruefung starten)
void optimiererStarten(bool nachBau = false);   // die Pruefung im Hintergrund starten
void zeichneOptimierer();                // das Fenster (jedes Bild) und das Abholen des Ergebnisses
[[nodiscard]] bool optimiererLaeuft();
[[nodiscard]] bool optimiererOffen();
void optimiererSchliessen();
void optimiererBeenden();                // beim Beenden: den Faden abwarten
void npcModelleVorab();                  // Stufe 7: NPC-Dateien fuer den Optimierer im Faden lesen (einmal)
[[nodiscard]] std::string optimiererBericht();                  // der Textbericht (Programmsprache)
bool optimiererBerichtSchreiben(const std::string& pfad);
// Fuer den Selbsttest
struct OptimiererBlick {
    bool hatErgebnis = false;
    bool veraltet = false;
    int grenzenUeber = 0;
    int grenzenKnapp = 0;
    int befunde = 0;
    double ms = 0;
    std::string bsp;                     // die gepruefte BSP ("" = keine)
    std::vector<std::string> befundIds;  // in der Reihenfolge der Liste
    std::vector<long long> befundAnzahl;
    std::vector<std::string> grenzenIds; // Grenzen, die nicht "ok" sind
    int laeufe = 0;                      // abgeschlossene Pruefungen
    std::string meldung;                 // die Zeile ueber den Knoepfen (letzte Behebung)
    double startAbbildMs = 0;            // letzter Start auf dem Hauptfaden: Momentaufnahme (Stufe 6)
    double startShaderMs = 0;            //   ... und Shader im Texturinventar nachsehen
    double startGesamtMs = 0;            //   ... der ganze Start (optimiererStarten)
};
[[nodiscard]] OptimiererBlick optimiererBlick();
void optimiererReiter(int reiter);       // 0 Grenzen, 1 Pruefungen
bool optimiererZeileWaehlen(int reiter, const std::string& id);   // waehlt die Objekte aus, zentriert die Ansichten
bool optimiererBefundBeheben(const std::string& id);              // wie "Beheben..." + "Anwenden"
int optimiererAllesSichere();            // wie "Alles Sichere beheben": Zahl der behobenen Befunde
// --- Stufe 4 Punkte 1/2: fertige Karten (app_karte_bsp.cpp) ---
void bspOeffnenDialog();                 // File > Open .bsp...
void zeichneBspDialog();                 // der Dialog "Open compiled map" (jedes Bild)
bool bspOeffnenSpiel(const std::string& pfad);   // maps/....bsp aus dem Dateisystem des Spiels
bool bspOeffnenDatei(const std::string& pfad);   // eine .bsp von der Platte
bool bspImportieren(const std::string& pfad);    // File > Import... mit .bsp (tryDecompile)
[[nodiscard]] bool bspDialogOffen();
[[nodiscard]] std::vector<karte::BspEintrag> bspDialogListe();
[[nodiscard]] bool bspNurEntitaetenMoeglich();   // eine .bsp ist offen
[[nodiscard]] bool bspGeometrieGeaendert();      // Fingerabdruck anders als beim Oeffnen
void bspNurEntitaeten();
// Stufe 8a Punkt 3: misc_model aus der Geometrie zurueckgewinnen (im Hintergrund nach Open .bsp)
struct BspRueckStand {
    int stand = 0;          // 0 frei, 1 Modelle laden, 2 suchen, 3 fertig (wird uebernommen), 4 Fehler, 5 abgebrochen
    double anteil = 0;
    std::size_t kandidaten = 0;
};
[[nodiscard]] BspRueckStand bspRueckStand();
[[nodiscard]] std::string modellIndexPfad();   // Stufe 8b Punkt 8: der Modell-Index im Einstellungsordner
void bspRueckStarten(const std::string& bspDaten, const std::string& ordner, const std::string& stamm, const std::string& karteDatei);
void bspRueckAbbrechen();
void bspRueckBeenden();
void bspRueckSchritt();   // jedes Bild: Ergebnis uebernehmen, Fortschritt zeigen
// Stufe 8a Punkt 5c: eine fertige .bsp als Vorlage (gezeichnet in Kamera und 2D, nicht bearbeitbar)
bool vorlageLaden(const std::string& bytes, const std::string& pfad, std::string* fehlerText = nullptr);
void vorlageEntfernen();
[[nodiscard]] bool vorlageAktiv();
[[nodiscard]] const std::string& vorlagePfad();
[[nodiscard]] const std::vector<karte::Linie>& vorlageLinien();
[[nodiscard]] std::size_t vorlageStand();
bool vorlageZeichnen(const float* matrix);
// Stufe 8a Punkt 5c: der Dialog "Zur fertigen Karte hinzufuegen"
void zusammenDialogOeffnen();
void zusammenDialogSchliessen();   // Stufe 8b: fuer karteDialogeZu im Selbsttest
void zusammenDialogZeichnen();
struct ZusammenStand {
    int stand = 0;           // 0 nichts, 1 baut, 2 fertig, 3 Fehler
    std::string ergebnis;    // die zusammengefuehrte .bsp
    std::vector<std::string> befunde;   // Pruefungen (Text)
    int fehler = 0;
    int kollisionGesamt = 0;
    int kollisionStoppen = 0;
    bool kollisionGeladen = false;
    int grenzenUeber = 0;
};
[[nodiscard]] const ZusammenStand& zusammenStand();
void zusammenVorlageWaehlenDatei(const std::string& pfad);   // fuer den Selbsttest ohne Dateidialog
void zusammenStarten(int ebene);                 // Build > Update entities only
void bauSchritt();                     // CWatchBSP::RoutineProcessing (jedes Bild, auch ausserhalb des Editors)
[[nodiscard]] bool bauLaeuft();        // ein Bauen laeuft (Bilder weiter zeichnen)
// Stufe 8d Punkt 1b (GPU bake Auto): die gemerkten -light-Zeiten (Datei karte_gpuzeiten.txt im
// Einstellungsordner) und der Text "was Auto gewaehlt hat und warum" (app_karte_bau.cpp)
[[nodiscard]] karte::GpuZeiten gpuZeitenHolen(const std::string& karte, const std::string& schalter);
void gpuZeitMerken(const std::string& karte, const std::string& schalter, bool gpu, double sekunden);
[[nodiscard]] std::string gpuWahlText(const karte::GpuBackBericht& b, long long cpuMs, long long gpuMs);
void bauAbbrechen();                   // Abweichung: Bauen abbrechen
void bauBeimBeenden();                 // ~CWatchBSP: kein q3map2 ohne Editor
void bauRueckmeldungenZeichnen();
// Stufe 8a Punkt 1: Fortschritt des laufenden Baus (eigene Funktion, nicht in NRC)
struct BauFortschrittStand {
    bool laeuft = false;
    std::size_t schritt = 0;
    std::size_t schritte = 0;
    std::string stufe;      // BSP / VIS / LIGHT / ...
    std::string phase;      // Phase von q3map2 ("IlluminateRawLightmap")
    double pacifier = -1;   // 0..1, -1 ohne
    int bounce = 0;
    int bounceVon = 0;
    karte::BauSchaetzung schaetzung;
    double vergangen = 0;
};
[[nodiscard]] BauFortschrittStand bauFortschrittStand();
void bauFortschrittZeichnen();      // "Q3Map debug window" (CDbgDlg)
void bauRueckmeldungWaehlen(int zeile);   // feedback_selection_changed
// Abweichung (Auftrag): der Ordner, in dem gebaut wird ("" = neben der Karte;
// nie im Spielordner - dann der Arbeitsordner), hinweis = warum.
[[nodiscard]] std::string bauOrdnerVorschau(std::string* hinweis = nullptr);
// Stufe 8c (eigene Funktion): Build > Pack map as .pk3 - das Archiv neben der gebauten .bsp
// (Pfad), "" bei einem Fehler (in der Konsole). Nie im Spielordner.
std::string karteAlsPk3();
void bauDialogZeichnen();              // BuildMenuDialog_construct (jedes Bild)
void autoCaulkAuswahl();               // map_autocaulk_selected ("AutoCaulkSelected", F4)
// --- Stufe 3 Punkt 5: Leck-Linie (points.cpp), Rueckmeldungen in 2D
// (feedback.cpp Draw2D) und der Portal Viewer (contrib/prtview) ---
void punktdateiUmschalten();           // Pointfile_Toggle ("TogglePointfile")
void punktdateiLeeren();               // Pointfile_Clear (Map_Free, Map_Save)
void leckWeiter();                     // Pointfile_Next ("NextLeakSpot")
void leckZurueck();                    // Pointfile_Prev ("PrevLeakSpot")
// CPointfile in den 2D-Ansichten: die Linie in Rot ($POINTFILE, 4 breit)
[[nodiscard]] std::vector<karte::Linie> punktdateiLinien();
// CPointMsg / CWindingMsg::Draw2D der gewaehlten Zeile des Q3Map debug window
struct RueckmeldungZeichnung {
    std::vector<karte::Linie> punkte;                 // glPointSize( 4 ), rot
    std::vector<karte::Linie> linien;                 // GL_LINE_LOOP, rot
    std::vector<std::vector<karte::Vektor3f>> flaechen;   // GL_POLYGON (0.133, 0.4, 1, 0.5) gemischt
};
[[nodiscard]] RueckmeldungZeichnung rueckmeldungZeichnung2d(karte::Sicht sicht);
// CPointfile::renderSolid in der Kamera ($POINTFILE: Tiefe testen und schreiben)
void punktdateiKameraZeichnen(const float* matrix);
// prtview: Befehle des Plugins (QERPlug_Dispatch), Dialoge, Einstellungen
void portalBefehl(const std::string& befehl);
void portalDialogeZeichnen();          // "Load .prt" und "Portal Viewer Configuration"
[[nodiscard]] ImVec2 portalKonfigPunkt(float x, float y);   // Selbsttest: logischer Punkt im Konfigurationsdialog -> Bildpunkt
void portalEinstellungenLesen();       // LoadConfig (beim ersten Gebrauch)
void portalEinstellungenSchreiben();   // SaveConfig (PrtView_destroy, beim Beenden)
[[nodiscard]] std::string portalIniPfad();
// CPortalsRender::renderWireframe: die Portale in den 2D-Ansichten
[[nodiscard]] std::vector<karte::Linie> portalLinien2d(float& breite);
// CPortalsRender::renderSolid in der Kamera; fuellen = RENDER_FILL der Darstellungsart
void portaleKameraZeichnen(const float* matrix, const karte::Matrix4& viewproj, bool fuellen);
// Stufe 4 Punkt 3: die Plugins (app_karte_plugins.cpp). Je Befehl des
// Plugins-Menues eine Umsetzung (QERPlug_Dispatch), "an" nur bei Umschaltern.
struct PluginUmsetzung {
    std::string name;
    std::function<void()> tun;
    std::function<bool()> an;
};
[[nodiscard]] std::vector<PluginUmsetzung> pluginUmsetzungen();
void pluginDialogeZeichnen();          // die modalen Dialoge und "BSP leafs"
void brushExportZeigen();              // brushexport2: CreateWindow (app_karte_brushexport.cpp)
// ShaderPlug, SunPlug, UFO:AI (app_karte_kleineplugins.cpp)
[[nodiscard]] std::vector<PluginUmsetzung> kleinePluginUmsetzungen();
void kleinePluginDialogeZeichnen();
// MeshTex (app_karte_meshtex.cpp)
[[nodiscard]] std::vector<PluginUmsetzung> meshTexUmsetzungen();
void meshTexDialogeZeichnen();
// Stufe 8b Punkt 1: Terrain Generator (app_karte_terrain.cpp)
[[nodiscard]] std::vector<PluginUmsetzung> terrainUmsetzungen();
void terrainDialogZeichnen();
// Stufe 9 Punkt 2: File > Import geometry as brushes (obj/fbx/glb/gltf; .bsp: Open .bsp mit dem Leser des Spiels)
void geoImportOeffnen();
bool geoImportLaden(const std::string& pfad);
void geoImportDialogZeichnen();
void geoImportEinfuegen();
// Stufe 9 Punkt 3: ein Modell aufs Fenster gezogen - als MD3 samt Shader und Texturen in den
// Import-Ordner, als misc_model unter dem Mauszeiger (2D-Ansicht) bzw. vor der Kamera
[[nodiscard]] bool istModellDatei(const std::string& pfad);
// Stufe 9 Punkt 10: die gewaehlten Brushes fuer Blender (.obj oder .glb, Y oben); pfad leer: Dateidialog
bool blenderExport(const std::string& pfad);
bool modellAbgelegt(const std::string& pfad, ImVec2 punkt);
[[nodiscard]] std::string importOrdner();   // <Einstellungen>/import: eigener Suchordner (wenn vorhanden)
void terrainErzeugenKnopf();   // "Generate" (auch fuer den Selbsttest)
void terrainSchliessen();      // "Close" / Schliessknopf: Werte uebernehmen
// Tags des Texturbrowsers: die Datei (LocalRcPath + SHADERTAG_FILE) und neu laden
[[nodiscard]] std::string tagDatei();
void tagsNeuLaden();
void brushExportZeichnen();
void pluginsFreigeben();               // BobToolz_destroy beim Wechsel der Karte
// XYWindowMouseDown_connect (DTreePlanter::mouseDown): true = genommen
bool pluginXyDruck(int fenster, int knopf, const karte::Vektor3f& welt);
// Was die Plugins in den 2D-Ansichten zeichnen (renderWireframe): je Gruppe eine Breite
struct PluginLinien {
    std::vector<karte::Linie> linien;
    float breite = 1.0F;
};
[[nodiscard]] std::vector<PluginLinien> pluginLinien2d();
// Stufe 9 Punkt 4 (app_karte_sprung.cpp)
void sprungPruefungStarten();
void sprungPruefungLeeren();
void sprungPruefungSchritt();     // jedes Bild: ein fertiges Ergebnis uebernehmen
void sprungPruefungBeenden();
[[nodiscard]] int sprungPruefungFortschritt();
void sprungLinien(std::vector<PluginLinien>& v);
// Stufe 9 Punkt 5 (app_karte_sprung.cpp)
void npcNetzUmschalten();
bool npcTestlaufStarten();
void npcTestlaufLeeren();
void npcLinien(std::vector<PluginLinien>& v);
// Stufe 9 Punkt 6 (app_karte_sprung.cpp)
bool verbundKopieBefehl();
bool verbundLoesenBefehl();
// Stufe 9 Punkt 7 (app_karte_sprung.cpp): Kurve durch die gewaehlten Punkt-Entities
bool kurveBefehl(karte::KurvenArt art);
// Stufe 9 Punkt 8 (app_karte_sprung.cpp): Modelle verteilen
bool streuBefehl(int anzahl);
// Stufe 9 Punkt 9 (app_karte_sprung.cpp): Gelaende formen
void gelaendeFormenMaus(float x, float y, bool unten, float dt);   // aus der Kamera, jedes Bild
std::size_t gelaendeFormenAn(const karte::Vektor3f& punkt, float dt);   // eine Anwendung im laufenden Strich
void gelaendeFormenEnde();
// Stufe 8b Punkt 6: Spiegel-Modus (app_karte_spiegel.cpp)
void spiegelTick();                    // jeden Frame vor den Ansichten
void spiegelSetzen(int achse);         // -1 aus, 0 X, 1 Y, 2 Z
void spiegelEbeneAnAuswahl();          // Ebene durch die Mitte der Auswahl (aufs Raster)
void spiegelEbeneNull();               // Ebene durch den Ursprung
void spiegelMenue();                   // View > Mirror mode
void spiegelLinien(std::vector<PluginLinien>& v);   // die Ebene in den 2D-Ansichten
// Stufe 8b Punkt 7: Undo-Verlauf als Liste (app_karte_undoliste.cpp)
void undoListeZeigen();
void undoListeZeichnen();
void undoListeSpringen(std::size_t ziel);              // zum Stand nach "ziel" Schritten (0 = Anfang)
[[nodiscard]] std::string undoListeBeschriftung(std::size_t i);   // die Zeile i (Text###uv<i>)
// ... und in der Kamera: eSortFullbright (DBobView) bzw. eSortOverlayFirst
// (DTrainDrawer, DVisDrawer); fuellen = Darstellung mit Flaechen (eFullMaterials)
void pluginKameraVoll(const float* matrix, bool fuellen);
void pluginKameraUeber(const float* matrix, bool fuellen);
struct BauDialogStand {                // fuer den Selbsttest
    bool offen = false;
    int aktuell = -1;
    int befehlAktuell = -1;
    bool variablenOffen = false;
    int bearbeitet = 0;
    bool kopiertSichtbar = false;
};
[[nodiscard]] BauDialogStand bauDialogStand();
// Bildpunkt (Mitte) einer Zeile im letzten Bild: liste 1 Eintraege, 2 Befehle,
// 3 Konstanten, 4 Werkzeuge (Tabellen: Spalte 0 oder 1).
[[nodiscard]] ImVec2 bauDialogZeile(int liste, int zeile, int spalte = 0);
[[nodiscard]] ImVec2 bauDialogPunkt(int x, int y);   // logischer Punkt im Dialog -> Bildpunkt

// Konsole (app_karte_konsole.cpp): Sys_Print - Text an das Ende, '\n'
// beendet die Zeile; ausgabe/warnung/fehler schreiben eine ganze Zeile
// (globalOutputStream / globalWarningStream / globalErrorStream << text << '\n').
void konsoleSchreiben(KonsolenZeile::Art art, const std::string& text);
void ausgabe(const std::string& text);
void warnung(const std::string& text);
void fehler(const std::string& text);
void konsoleProtokoll(const karte::Protokoll& p);
void konsoleLeeren();
[[nodiscard]] std::string konsoleAuswahl();   // fuer den Selbsttest
// Die Konsole im aktuellen Kindfenster (Console_constructWindow).
void konsoleZeichnen();

// Das Spiel: gewaehltes Regelwerk; startet das Laden im Hintergrund, wenn
// es sich geaendert hat. bereit() wartet nicht.
void spielPruefen();
[[nodiscard]] bool spielBereit();
// Wartet, bis das Spiel geladen ist (fuer Oeffnen und Neu).
const std::shared_ptr<Spielkontext>& spielWarten();

}  // namespace bhed::gui::karteneditor
#endif
