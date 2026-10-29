// untertitel.h - Untertitel wie das Spiel (Zeitleiste 2, Punkte 2 und 7).
//
// Portiert aus OpenJK und Movie Duels:
//   code/cgame/cg_servercmds.cpp, codeJK2/cgame/cg_servercmds.cpp und
//   Movie Duels cgame/cg_servercmds.cpp (CG_ParseServerinfo: stripLevelName),
//   code/cgame/cg_text.cpp (cg_SP_GetStringTextStringWithRetry, CG_CaptionText),
//   code/game/Q3_Interface.cpp (PlayIcarusSound: der Schluessel aus dem
//   Klangnamen), code/qcommon/strip.cpp (JK2SP_GetStringID, FixIllegalChars).
//
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2001 - 2013, Activision, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//
//   OpenJK is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License version 2 as published by the
//   Free Software Foundation. This program is distributed in the hope that it
//   will be useful, but WITHOUT ANY WARRANTY; without even the implied
//   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//   GNU General Public License for more details (LICENSE).
//
// Die Texte selbst liest bhed::menue::Texte (ladeStr fuer die StringEd-Pakete
// von Jedi Academy und Movie Duels, ladeSp fuer die StripEd-Pakete von Jedi
// Outcast) - hier steht nur, WELCHE Pakete eine Karte liest und welcher
// Schluessel zu einem Klang gehoert.
#ifndef BHED_UNTERTITEL_H
#define BHED_UNTERTITEL_H

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace bhed::untertitel {

enum class Spiel { JediAcademy, MovieDuels, JediOutcast };

// CG_ParseServerinfo: die Paketnamen (cgs.stripLevelName, gross), in denen
// CG_CaptionText nacheinander sucht. karte: "maps/kejim_post.bsp" oder
// "kejim_post". paketDa(name): laedt cgi_SP_Register strip/<name>.sp (nur
// Jedi Outcast fragt das - dort haengen Umbenennung und Zusatzpakete daran).
//   Jedi Academy: YAVIN1B liest dazu YAVIN1.
//   Movie Duels:  dazu die Karten von Jedi Outcast, ohne nachzufragen.
//   Jedi Outcast: YAVIN_FINAL/YAVIN_SWAMP -> YAVIN_CANYON, YAVIN_TRIAL ->
//                 YAVIN_TEMPLE, VALLEY -> ARTUS_TOPSIDE, wenn das eigene Paket
//                 fehlt; sonst Zusatzpakete (KEJIM_* -> ARTUS_MINE ...).
[[nodiscard]] std::vector<std::string> pakete(const std::string& karte, Spiel spiel,
                                              const std::function<bool(const std::string&)>& paketDa = {});

// Der Schluessel-Rest zu einem Klang aus dem Skript, wie PlayIcarusSound und
// CG_CaptionText ihn bilden: klein, ohne Endung, alles nach dem letzten '/'.
// "" ohne '/' (CG_CaptionText: "given audio filename with no '/'").
[[nodiscard]] std::string referenz(const std::string& klang);

// cg_SP_GetStringTextStringWithRetry: "<PAKET>_<REFERENZ>" in jedem Paket
// (gross wie JK2SP_GetStringID / SE_GetString); eine Referenz mit '#' am
// Anfang traegt ihr Paket schon selbst. werte: Schluessel gross -> Text.
// schluessel (wenn nicht nullptr): der gefundene, sonst der erste versuchte.
[[nodiscard]] std::string text(const std::string& klang, const std::vector<std::string>& pakete,
                               const std::map<std::string, std::string>& werte, std::string* schluessel = nullptr);

// FixIllegalChars (strip.cpp): was Jedi Outcast beim Laden eines Textes der
// europaeischen Sprachen ersetzt (typografische Anfuehrungszeichen, Striche,
// "...", "?." und Tabulatoren).
[[nodiscard]] std::string jk2Reinigen(std::string text);

}  // namespace bhed::untertitel

#endif  // BHED_UNTERTITEL_H
