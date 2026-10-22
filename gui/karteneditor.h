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
#include "bhed/karte/bearbeitung.h"
#include "bhed/karte/befehle.h"
#include "bhed/karte/eklasse.h"
#include "bhed/karte/entitaetbefehle.h"
#include "bhed/karte/kamera.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/raster.h"
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

namespace bhed::gui::karteneditor {

namespace befehle = karte::befehle;

// Das Spiel des Editors: Dateisystem, Textur-Inventar und Entitaetsklassen.
// NRC laedt sie beim Start aus der .game-Datei (QE_InitVFS, Shaders_Load,
// EntityClassQuake3_Construct); hier aus dem Regelwerk des Programms:
//   OpenJK        -> ja (base),     data/karte/ja/*.def (sp)
//   Movie Duels   -> ja + fs_game MD, ja-Klassen + bhed/entdefs.h
//   Jedi Outcast  -> jk2,           data/karte/jk2/*.def (sp)
struct Spielkontext {
    int spiel = 0;                  // kRegelwerkOpenJK / MovieDuels / JediOutcast
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
inline constexpr int kSymbol = 24;         // PM_ToolBarIconSize
inline constexpr int kKnopf = 31;          // 24 + 3 ("broken QToolButton") + padding 2 x 1 + border 2 x 1
inline constexpr int kTrenner = 8;         // dark.qss QToolBar::separator
inline constexpr int kLeisteH = kLeisteRand * 2 + kLeistePolster * 2 + kKnopf;   // 39
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
    float letzteX = 0;           // FreezePointer: letzte Mauslage (Bildpunkte)
    float letzteY = 0;
    float ankerX = 0;            // FreezePointer: Lage beim Festhalten
    float ankerY = 0;
    unsigned tasten = 0;         // m_buttonstate (RAD_*)
    bool gedrueckt = false;      // diese Ansicht hat den Druck bekommen
    karte::Vektor3f maus{0, 0, 0};   // m_mousePosition (eingerastet)
    float bildX = 0;             // linke obere Ecke im Programmfenster (Bildpunkte)
    float bildY = 0;
    // EntityCreate_MouseDown/Move/Up (xywindow.cpp:1065-1083): M2 ohne
    // Bewegung oeffnet beim Loslassen das Entitaetenmenue.
    bool entitaetErzeugen = false;   // m_entityCreate
    int erzeugX = 0;                 // m_entityCreate_x (Fensterpunkte)
    int erzeugY = 0;
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
    float letzteX = 0;              // Freelook: Lage des Zeigers nach dem Zuruecksetzen
    float letzteY = 0;
    std::vector<int> tasten;        // PressedKeys::keys (ImGuiKey)
    bool kontextmenue = false;      // context_menu_show (Doppel-M2) - fuer den Selbsttest
    double zeichenMs = 0.0;         // ShowStats: msec des letzten Bilds
    double letzteStatistik = -1.0;  // m_render_time (f2f)
};
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
    karte::ZuletztListe zuletzt;
    bool zuletztGeladen = false;

    // --- das Spiel ---
    std::shared_ptr<Spielkontext> spiel;   // fertig geladen
    std::future<std::shared_ptr<Spielkontext>> spielLaden;
    int spielStand = -1;                   // fuer welches Regelwerk geladen

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
    std::string tastenDialogBearbeitet;   // Befehl, dessen Taste gerade aufgenommen wird
    int tasteVerbrauchtBild = -1;         // in diesem Bild hat die Aufnahme die Taste geschluckt
    bool ueberOffen = false;
    std::string letzterBefehl;   // fuer den Selbsttest: zuletzt ausgeloest
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
    std::string zwischenablage;            // Copy/Paste (NRC: QClipboard "RadiantClippings")
    bool nachKlonenSchubsen = false;       // g_bNudgeAfterClone ("Nudge selected after duplication")
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
void texturBilderAbholen();
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
bool kameraTexturFarbe(const std::string& shader, float rgb[3]);

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
// Einstellungen des Editors lesen/schreiben (Settings::karte).
void einstellungenLaden();
void einstellungenSpeichern();
// true, solange der Karten-Editor Tastatur und Maus fuer sich hat (der
// Arbeitsbereich ist sichtbar). Dann gelten die BehavEd-Kuerzel nicht.
[[nodiscard]] bool aktiv();

// --- Datei (radiant/map.cpp) ---
// NewMap (map.cpp:1813): mit Rueckfrage, wenn geaendert.
void neueKarte();
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
