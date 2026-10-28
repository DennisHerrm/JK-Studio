// wiederherstellen.h - Karten-Editor, Stufe 9 Punkt 12: nach einem Absturz wiederherstellen
// (eigene Funktion, nicht in NetRadiant-Custom).
//
// Portiert aus NetRadiant-Custom 20260114 ist nur der Name der Autosave-Datei (autosave.cpp:
// <karte>.autosave.map bzw. maps/autosave.map ohne Namen); die Sitzungsdatei ist neu.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
//
// Solange der Karten-Editor eine Karte offen hat, steht ihr Name in einer Sitzungsdatei im
// Einstellungsordner; beim sauberen Beenden wird sie geloescht. Liegt sie beim naechsten Start
// noch da, endete die Sitzung nicht sauber. Ist dann das Autosave der Karte neuer als die Karte
// (oder die Karte fehlt), bietet der Editor an, es zu laden.
#ifndef BHED_KARTE_WIEDERHERSTELLEN_H
#define BHED_KARTE_WIEDERHERSTELLEN_H

#include <string>

namespace bhed::karte {

inline constexpr const char* kSitzungsDatei = "karte_sitzung.txt";

struct Wiederherstellung {
    bool anbieten = false;
    std::string karte;      // der Name der Karte der abgebrochenen Sitzung
    std::string autosave;   // die Datei, die geladen wuerde
};

// unbenanntAutosave: die Autosave-Datei einer Karte ohne Namen (maps/autosave.map im Benutzerordner).
// prozess: die Nummer des laufenden Prozesses - eine Sitzungsdatei desselben Prozesses ist kein
// Absturz (der Editor prueft beim Start; ein zweiter Aufruf im selben Lauf zaehlt nicht).
[[nodiscard]] Wiederherstellung wiederherstellungPruefen(const std::string& sitzungsPfad, const std::string& unbenanntAutosave, unsigned long prozess);
void sitzungSchreiben(const std::string& sitzungsPfad, const std::string& karte, unsigned long prozess);
void sitzungLoeschen(const std::string& sitzungsPfad);

}  // namespace bhed::karte

#endif
