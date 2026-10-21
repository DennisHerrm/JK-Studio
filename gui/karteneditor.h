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
#include "bhed/karte/befehle.h"
#include "bhed/karte/eklasse.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/raster.h"
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
    double sekunden = 0.0;
};

// Eine Zeile der Konsole (radiant/console.cpp): Ausgabe, Warnung, Fehler.
struct KonsolenZeile {
    enum class Art { Ausgabe, Warnung, Fehler };
    Art art = Art::Ausgabe;
    std::string text;
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

// Ein Fenster der Ansichten (XYWnd / CamWnd) - wo es gerade steht.
enum class AnsichtTyp { YZ = 0, XZ = 1, XY = 2, Kamera = 3 };   // VIEWTYPE (xywindow.h)

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

    // --- Fenster (MainFrame) ---
    befehle::Anordnung anordnung = befehle::Anordnung::Regular;   // g_Layout_viewStyle
    bool gruppenDialogEingebaut = false;   // g_Layout_builtInGroupDialog
    float hSplit = 384.0F / 960.0F;        // RestoreGuiState: m_hSplit {384, 576}
    float vSplit = 377.0F / 397.0F;        // m_vSplit {377, 20}
    float vSplit2 = 250.0F / 400.0F;       // m_vSplit2 {250, 150}
    bool maximiert = false;                // MaximizeView (F12)
    int maximiertFeld = 0;                 // welches Feld maximiert ist
    // Floating: die Fenster Camera, XY, XZ, YZ (ToggleCamera, ToggleView ...).
    std::array<bool, 4> schwebendSichtbar{true, true, true, true};
    // Der Gruppendialog "Entities" (groupdialog.cpp) mit seinen Seiten.
    bool gruppenDialogOffen = false;
    int gruppenSeite = 0;   // 0 Entities, 1 Console, 2 Textures, 3 Models, 4 Layers
    bool gruppenSeiteWechseln = false;
    // Werkzeugleisten (toolbar_construct_control_menu)
    bool hauptleiste = true;
    bool filterleiste = true;
    bool pluginleiste = true;
    std::set<std::string> versteckteKnoepfe;   // g_toolbarHiddenButtons
    Farben farben;
    // Statuszeile (create_main_statusbar): c_status_command, _position, _texture
    std::string statusBefehl;
    std::string statusPosition;
    std::string statusTextur;

    // --- Raster und Kamera-Werte, die die Statuszeile zeigt ---
    karte::Raster raster;     // grid.cpp
    int kubikSkala = 14;      // g_camwindow_globals.m_nCubicScale (camwindow.h)

    // --- Befehle ---
    befehle::Belegung belegung;
    bool belegungGeladen = false;
    // Dialoge
    bool einstellungenOffen = false;
    bool tastenDialogOffen = false;
    bool ueberOffen = false;
    std::string letzterBefehl;   // fuer den Selbsttest: zuletzt ausgeloest
    std::vector<std::string> knoepfeImBild;   // fuer den Selbsttest: gezeichnete Knoepfe

    // Fuer den Selbsttest: die Kennung des Arbeitsbereichs im letzten Bild.
    bool gezeichnet = false;
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

// --- Menues, Werkzeugleisten, Statuszeile (app_karte_menue.cpp) ---
void zeichneMenueleiste();
void zeichneHauptleiste();
float zeichneFilterleiste(float hoehe);   // gibt die Breite zurueck
void zeichneStatuszeile();
float statuszeilenHoehe();
// Das Symbol <name> (bitmaps/<name>.svg) als Bild; false, wenn es fehlt.
bool symbol(const char* name, float groesse, bool hell = true);
// --- Dialoge (app_karte_dialoge.cpp) ---
void zeichneEinstellungen();   // "NetRadiant Preferences" (P)
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

// Konsole: globalOutputStream / globalWarningStream / globalErrorStream.
void ausgabe(const std::string& text);
void warnung(const std::string& text);
void fehler(const std::string& text);

// Das Spiel: gewaehltes Regelwerk; startet das Laden im Hintergrund, wenn
// es sich geaendert hat. bereit() wartet nicht.
void spielPruefen();
[[nodiscard]] bool spielBereit();
// Wartet, bis das Spiel geladen ist (fuer Oeffnen und Neu).
const std::shared_ptr<Spielkontext>& spielWarten();

}  // namespace bhed::gui::karteneditor
#endif
