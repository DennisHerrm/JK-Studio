// spielbeschreibung.cpp - Karten-Editor: die Spielbeschreibungen (.game),
// die Mod-Auswahl und die Vorgaben der Build-Seite.
//
// Portiert aus NetRadiant-Custom 20260114 (Quellen in spielbeschreibung.h).
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
#include "bhed/karte/spielbeschreibung.h"

#include <algorithm>
#include <cctype>

namespace bhed::karte {

namespace {

// path_equal unter Windows (OS_CASE_INSENSITIVE): string_equal_nocase
bool pfadGleich(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) { return false; }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) { return false; }
    }
    return true;
}

}  // namespace

const std::vector<SpielBeschreibung>& spielBeschreibungen() {
    // gamepacks/games/ja.game und jk2.game (NRC 20260114)
    static const std::vector<SpielBeschreibung> s = {
        {"ja.game", "ja", "Jedi Knight Jedi Academy", "C:/Program Files/LucasArts/Star Wars Jedi Knight Jedi Academy/GameData/", "jaSP.exe",
         "jaMP.exe", "base", "Jedi Knight Jedi Academy", "Custom JA modification", "MBII", "Movie Battles II", 0.25F, true},
        {"jk2.game", "jk2", "Jedi Knight II: Jedi Outcast", "C:/Program Files/LucasArts/Star Wars JK II Jedi Outcast/GameData/", "jk2SP.exe",
         "jk2MP.exe", "base", "Jedi Knight II Outcast", "Custom JK2 modification", "", "", 0.25F, true},
    };
    return s;
}

const SpielBeschreibung& spielBeschreibung(const std::string& datei) {
    for (const SpielBeschreibung& b : spielBeschreibungen()) {
        if (pfadGleich(datei, b.datei)) { return b; }
    }
    return spielBeschreibungen().front();
}

std::vector<std::string> modNamen(const SpielBeschreibung& b) {
    std::vector<std::string> n{b.basisName};
    if (b.bekannterName[0] != '\0') { n.emplace_back(b.bekannterName); }   // if ( !string_empty( known ) )
    n.emplace_back(b.eigenerName);
    return n;
}

// gamecombo_for_dir (gtkdlgs.cpp:119-130)
SpielAuswahl spielAuswahlFuerOrdner(const SpielBeschreibung& b, const std::string& ordner) {
    if (pfadGleich(ordner, b.basisSpiel)) { return {0, ordner, false}; }
    if (pfadGleich(ordner, b.bekanntesSpiel)) { return {1, ordner, false}; }
    return {b.bekanntesSpiel[0] == '\0' ? 1 : 2, ordner, true};
}

// gamecombo_for_gamename (gtkdlgs.cpp:132-143)
SpielAuswahl spielAuswahlFuerName(const SpielBeschreibung& b, const std::string& name) {
    if (name.empty() || name == b.basisName) { return {0, b.basisSpiel, false}; }
    if (name == b.bekannterName) { return {1, b.bekanntesSpiel, false}; }
    return {b.bekanntesSpiel[0] == '\0' ? 1 : 2, "", true};
}

// DefaultableString::Import (watchbsp.cpp:143-148): string_equal_prefix( string, "default:" )
void VorgabeText::importieren(const std::string& s) { text = s.rfind("default:", 0) == 0 ? std::string() : s; }

std::string VorgabeText::mitVorgabe(const std::string& vorgabe) const { return text.empty() ? "default:" + vorgabe : text; }

std::string VorgabeText::wert(const std::string& vorgabe) const { return text.empty() ? vorgabe : text; }

// constructEngineArgs (watchbsp.cpp:169-196), Zweig fuer Quake3-Spiele
std::string engineArgumente(const std::string& fsGame, const std::string& basisSpiel) {
    std::string s = "+set sv_pure 0";
    if (fsGame != basisSpiel) { s += " +set fs_game " + fsGame; }   // !string_equal( fs_game, basegame_get() )
    s += " +devmap %mapname%";
    return s;
}

// DirectoryCleaned (path.h:263-285)
std::string ordnerBereinigt(const std::string& pfad) {
    if (pfad.empty()) { return pfad; }
    std::string s = pfad;
    std::replace(s.begin(), s.end(), '\\', '/');
    if (s.back() != '/') { s += '/'; }   // path_separator( *--i )
    return s;
}

// QE_InitVFS (qe3.cpp:68-102): paths_push sammelt nur verschiedene, nicht leere Pfade
std::vector<std::string> vfsOrdner(const std::string& enginePfad, const std::string& fsGame, const std::string& basisSpiel,
                                   const std::vector<std::string>& zusaetzlich) {
    std::vector<std::string> pfade;
    const auto dazu = [&](const std::string& p) {
        if (p.empty()) { return; }
        if (std::none_of(pfade.begin(), pfade.end(), [&](const std::string& q) { return pfadGleich(q, p); })) { pfade.push_back(p); }
    };
    for (const std::string& p : zusaetzlich) { dazu(p); }
    dazu(enginePfad + fsGame + "/");       // globalGamePath
    dazu(enginePfad + basisSpiel + "/");   // globalBasePath
    return pfade;
}

}  // namespace bhed::karte
