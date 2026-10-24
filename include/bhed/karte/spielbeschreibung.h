// spielbeschreibung.h - Karten-Editor: die Spielbeschreibungen (.game) von
// Jedi Academy und Jedi Outcast, die Mod-Auswahl der Seite "Game" und die
// Vorgaben der Seite "Build".
//
// Portiert aus NetRadiant-Custom 20260114: gamepacks/games/ja.game und
// jk2.game (die Schluessel, die der Editor liest), radiant/preferences.cpp:
// 296-323 (CGameDialog::ScanForGames: die .game-Dateien in der Reihenfolge
// des Ordners), radiant/gtkdlgs.cpp:81-147 (GameComboConfiguration,
// gamecombo_for_dir, gamecombo_for_gamename), 150-158 (MappingMode),
// radiant/watchbsp.cpp:136-196 (DefaultableString, constructEngineArgs),
// radiant/qe3.cpp:68-102 (QE_InitVFS: die Reihenfolge der Suchordner).
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
#ifndef BHED_KARTE_SPIELBESCHREIBUNG_H
#define BHED_KARTE_SPIELBESCHREIBUNG_H

#include <string>
#include <vector>

namespace bhed::karte {

// CGameDescription: die Schluessel aus ja.game und jk2.game (Windows-Werte:
// enginepath_win32, engine_win32, mp_engine_win32).
struct SpielBeschreibung {
    const char* datei;            // mGameFile
    const char* typ;              // type
    const char* name;             // name ("Select the game")
    const char* enginePfad;       // enginepath_win32
    const char* engine;           // engine_win32
    const char* mpEngine;         // mp_engine_win32
    const char* basisSpiel;       // basegame
    const char* basisName;        // basegamename
    const char* eigenerName;      // unknowngamename
    const char* bekanntesSpiel;   // knowngame ("" ohne)
    const char* bekannterName;    // knowngamename ("" ohne)
    float standardSkala;          // default_scale
    bool spielModusZeigen;        // show_gamemode
};

// ScanForGames: die .game-Dateien des Gamepacks in der Reihenfolge des
// Ordners (ja.game vor jk2.game). Dieses Programm kennt nur diese beiden.
[[nodiscard]] const std::vector<SpielBeschreibung>& spielBeschreibungen();
// Die Beschreibung zu einer Datei ("ja.game"); unbekannt -> ja.game.
[[nodiscard]] const SpielBeschreibung& spielBeschreibung(const std::string& datei);

// gamecombo_t: Index im Klappfeld "Select mod", der fs_game-Text und ob er
// bearbeitet werden darf.
struct SpielAuswahl {
    int spiel;
    std::string fsGame;
    bool bearbeitbar;
};
// Die Eintraege von "Select mod": basegamename, knowngamename (wenn da),
// unknowngamename (Game_constructPreferences).
[[nodiscard]] std::vector<std::string> modNamen(const SpielBeschreibung& b);
[[nodiscard]] SpielAuswahl spielAuswahlFuerOrdner(const SpielBeschreibung& b, const std::string& ordner);   // gamecombo_for_dir
[[nodiscard]] SpielAuswahl spielAuswahlFuerName(const SpielBeschreibung& b, const std::string& name);       // gamecombo_for_gamename

// MappingMode: "Single Player mapping mode" (sp) / "Multiplayer mapping mode" (mp)
inline constexpr const char* kSpModus = "Single Player mapping mode";
inline constexpr const char* kMpModus = "Multiplayer mapping mode";

// DefaultableString (watchbsp.cpp:136-167): leer = die Vorgabe; im Feld steht
// dann "default:<Vorgabe>", ein Text mit diesem Anfang gilt wieder als leer.
struct VorgabeText {
    std::string text;
    void importieren(const std::string& s);                                      // Import
    [[nodiscard]] std::string mitVorgabe(const std::string& vorgabe) const;      // ExportWithDefault
    [[nodiscard]] std::string wert(const std::string& vorgabe) const;            // string()
};

// constructEngineArgs<isMP> fuer Quake3-Spiele (nicht q2/heretic2/wolf):
// "+set sv_pure 0", bei einer Mod " +set fs_game <mod>", " +devmap %mapname%".
[[nodiscard]] std::string engineArgumente(const std::string& fsGame, const std::string& basisSpiel);

// DirectoryCleaned (libs/os/path.h:255-285): '\' -> '/', dazu ein '/' am
// Ende, wenn er fehlt (leer bleibt leer). setEnginePath legt so ab.
[[nodiscard]] std::string ordnerBereinigt(const std::string& pfad);

// QE_InitVFS: die Suchordner - die zusaetzlichen Pfade, <Engine>/<fs_game>/,
// <Engine>/<basegame>/ (gleiche nur einmal; ohne userRoot wie unter Windows).
[[nodiscard]] std::vector<std::string> vfsOrdner(const std::string& enginePfad, const std::string& fsGame, const std::string& basisSpiel,
                                                 const std::vector<std::string>& zusaetzlich);

}  // namespace bhed::karte
#endif
