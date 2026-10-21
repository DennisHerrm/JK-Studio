// zuletzt.h - Karten-Editor: die Liste der zuletzt geoeffneten Karten
// (File-Menue, "&1- <Pfad>" ... "&9- <Pfad>").
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): radiant/mru.cpp
// (MRU_MAX, MRU_AddFile, MRU_Activate, MRU_updateWidget, EscapedMnemonic).
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
//
// NRC haelt die Liste in neun festen Menueeintraegen (MRU_items) und den
// Namen in MRU_filenames; hier nur die Liste und die Beschriftung. Die
// Oberflaeche zeigt Eintrag 1 immer (ohne Datei ausgegraut: "&1" bzw. nach
// dem Leeren "Recent Files"), die uebrigen nur, wenn belegt.
#ifndef BHED_KARTE_ZULETZT_H
#define BHED_KARTE_ZULETZT_H

#include <cstddef>
#include <string>
#include <vector>

namespace bhed::karte {

class ZuletztListe {
public:
    static constexpr std::size_t kMax = 9;   // MRU_MAX

    // MRU_AddFile: vorhandener Name wandert nach vorn, sonst vorn einfuegen
    // (der letzte faellt bei neun heraus).
    void hinzufuegen(const std::string& pfad);
    // MRU_Activate: ist die Datei lesbar, kommt sie nach vorn und ihr Pfad
    // zurueck (zum Laden); sonst faellt sie still aus der Liste und "" kommt
    // zurueck.
    std::string aktivieren(std::size_t index, bool lesbar);
    // Gespeicherte Liste uebernehmen (MRU_Init: hoechstens neun).
    void setzen(const std::vector<std::string>& pfade);
    [[nodiscard]] const std::vector<std::string>& pfade() const { return m_pfade; }
    [[nodiscard]] std::size_t anzahl() const { return m_pfade.size(); }   // MRU_used

    // MRU_updateWidget: "&<n>- <Pfad>" mit verdoppeltem '&' im Pfad
    // (EscapedMnemonic); ohne Eintrag der Text des ersten Menuepunkts:
    // "&1" vor dem ersten Oeffnen, "Recent Files" nachdem die Liste leer wurde.
    [[nodiscard]] std::string beschriftung(std::size_t index) const;
    // Der erste Menuepunkt ist ausgegraut, solange die Liste leer ist.
    [[nodiscard]] bool ersterAktiv() const { return !m_pfade.empty(); }

private:
    std::vector<std::string> m_pfade;
    bool m_geleert = false;   // MRU_items[0]->setText( "Recent Files" )
};

// Qt-Mnemonik fuer die Anzeige entfernen: "&&" -> "&", "&x" -> "x".
std::string ohneMnemonik(const std::string& text);

}  // namespace bhed::karte
#endif
