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
#include "bhed/karte/eklasse.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/texturen.h"
#include "bhed/karte/zuletzt.h"

#include <functional>
#include <future>
#include <memory>
#include <string>
#include <vector>

namespace bhed::gui::karteneditor {

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
