// kartendatei.h - Karten-Editor: Map Info, Find Brush, Import und Autosave.
//
// Portiert aus NetRadiant-Custom 20260114: radiant/map.cpp:843-978
// (EntityBreakdownWalker, CountStuffWalker, DoMapInfo), 1536-1585
// (Map_ImportFile: die .map-Seite), 1855-1862 (ImportMap: "mapImport"),
// 1943-2081 (BrushFindByIndexWalker, EntityFindByIndexWalker,
// Scene_FindEntityBrush, SelectBrush, BrushFindIndexWalker,
// EntityFindIndexWalker, GetSelectionIndex), radiant/autosave.cpp:42-158
// (DoesFileExist, Map_Snapshot, QE_CheckAutoSave, AutoSave_clear),
// libs/os/path.h (path_get_filename_base_end, PathFilenameless).
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
#ifndef BHED_KARTE_KARTENDATEI_H
#define BHED_KARTE_KARTENDATEI_H

#include "bhed/karte/mathe.h"

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <map>
#include <string>

namespace bhed::karte {

class Karte;

// --- DoMapInfo ---------------------------------------------------------------------------

struct KartenInfo {
    std::size_t pinsel = 0;       // g_brushCount
    std::size_t patches = 0;      // g_patchCount
    std::size_t entitaeten = 0;   // g_entityCount (mit worldspawn)
    int imSpiel = 0;              // Scene_CountStuff: ents_ingame
    int gruppen = 0;              // groupents
    int gruppenImSpiel = 0;       // groupents_ingame
    std::map<std::string, std::size_t> aufschluesselung;   // EntityBreakdown (classname -> Anzahl)
};
[[nodiscard]] KartenInfo karteInfo(const Karte& k);

// --- Find Brush --------------------------------------------------------------------------

// GetSelectionIndex: Nummer der Entitaet und des Primitivs des zuletzt
// Gewaehlten (0, 0 ohne Auswahl).
void auswahlNummern(const Karte& k, int& entitaet, int& pinsel);
// SelectBrush: das Primitiv "pinsel" der Entitaet "entitaet" (bzw. die
// Entitaet ohne Kinder) dazuwaehlen; mitte = worldAABB().origin fuer
// XYWnd::SetOrigin. false, wenn nichts gefunden wurde.
bool pinselFinden(Karte& k, int entitaet, int pinsel, Vektor3f* mitte = nullptr);

// --- Import ------------------------------------------------------------------------------

// ImportMap mit Map_ImportFile (.map): Rueckgaengig "mapImport", der Text
// wie beim Einfuegen zusammengefuehrt (Convert_Brushes, Map_mergeLayers,
// Map_gatherNamespaced, Map_mergeClonedNames, MergeMap) - die vorige
// Auswahl bleibt, das Eingelesene kommt dazu.
void karteImportieren(Karte& k, const std::string& text);

// --- Autosave ----------------------------------------------------------------------------

class AutoSpeichern {   // autosave.cpp:104-158
public:
    bool an = true;                 // g_AutoSave_Enabled ("Autosave")
    int minuten = 15;               // m_AutoSave_Frequency ("AutosaveMinutes", 1..60)
    bool schnappschuesse = false;   // g_SnapShots_Enabled ("Snapshots")

    enum class Aktion { Nichts, Uebersprungen, Speichern, Schnappschuss };
    void leeren() { m_aenderungen = 0; }   // AutoSave_clear
    // QE_CheckAutoSave (jede Sekunde ohne gedrueckte Maustaste):
    // aenderungen = MapFile::changes(); unbenannt = Map_Unnamed.
    [[nodiscard]] Aktion pruefen(std::time_t jetzt, std::size_t aenderungen, bool unbenannt);

private:
    std::time_t m_start = 0;          // s_start
    std::size_t m_aenderungen = 0;    // s_changes
};

// "<pfad>/<name>.autosave.<ext>" (QE_CheckAutoSave fuer eine benannte Karte)
[[nodiscard]] std::string autosavePfad(const std::string& karte);
// Map_Snapshot: "<pfad>/snapshots" und darin "<name>.<n>.<ext>" mit dem
// ersten freien n; summe = Groesse der vorhandenen (fuer die 50-MB-Meldung).
[[nodiscard]] std::string schnappschussOrdner(const std::string& karte);
[[nodiscard]] std::string schnappschussPfad(const std::string& karte, std::uintmax_t* summe = nullptr);

}  // namespace bhed::karte

#endif
