// untertitel.h - Untertitel wie das Spiel (Zeitleiste 2, Punkte 2 und 7).
//
// Portiert aus OpenJK und Movie Duels:
//   code/cgame/cg_servercmds.cpp, codeJK2/cgame/cg_servercmds.cpp und
//   Movie Duels cgame/cg_servercmds.cpp (CG_ParseServerinfo: stripLevelName),
//   code/cgame/cg_text.cpp (cg_SP_GetStringTextStringWithRetry, CG_CaptionText,
//   CG_DrawCaptionText), codeJK2/cgame/cg_text.cpp, Movie Duels
//   cgame/cg_text.cpp (cg_textprintscale, 2026-10-13; Vorgabe aus der DLL vom 27.10.), code/game/Q3_Interface.cpp
//   (PlayIcarusSound: Schluessel und wann "ct" gesendet wird, SetTextColor),
//   code/game/g_main.cpp (g_subtitles), code/qcommon/strip.cpp
//   (JK2SP_GetStringID, FixIllegalChars), code/rd-common/tr_font.cpp
//   (AnyLanguage_ReadCharFromString, RE_Font_StrLenPixels mit JK2_MODE),
//   shared/qcommon/q_color.c (colorTable).
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
// Outcast), die Schrift bhed::menue::ladeFontdat - hier steht, WELCHE Pakete
// eine Karte liest, welcher Schluessel zu einem Klang gehoert und wie das
// Spiel den Text als Untertitel zeigt.
#ifndef BHED_UNTERTITEL_H
#define BHED_UNTERTITEL_H

#include "bhed/menue/vorschau.h"

#include <array>
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

// --- Der Untertitel im Bild (Punkt 7) ----------------------------------------

// g_subtitles (g_main.cpp): 0 keine, 1 alle Saetze, 2 nur in Zwischensequenzen.
// Vorgabe: Jedi Academy und Movie Duels "0", Jedi Outcast "2". Das Menue
// (ui/setup.menu) bietet 0 und 2, Movie Duels auch 1.
[[nodiscard]] int gSubtitlesVorgabe(Spiel spiel) noexcept;

// PlayIcarusSound: schickt das Spiel fuer diesen Klang "ct" (den Untertitel)?
struct Klangfall {
    int gSubtitles = 0;
    bool npcMitUntertiteln = false;   // ent->NPC und SCF_USE_SUBTITLES (SET_USE_SUBTITLES "true")
    bool imKino = false;              // in_camera
    bool rundfunk = false;            // CHAN_ANNOUNCER, CHAN_VOICE_GLOBAL oder ein target_scriptrunner
    bool gedaempft = false;           // CHAN_VOICE_ATTEN: 350 statt 1200 Einheiten
    double abstandQuadrat = 0.0;      // vom Sprecher zum Spieler (g_entities[0])
};
[[nodiscard]] bool wirdGesendet(const Klangfall& k) noexcept;

// SetTextColor (SET_CAPTIONTEXTCOLOR): RGBA aus colorTable, unbekannt = WHITE.
// Ohne SET_CAPTIONTEXTCOLOR ist textcolor_caption 0 0 0 0 - CG_DrawCaptionText
// nimmt dann WHITE.
[[nodiscard]] std::array<float, 4> textFarbe(const std::string& name);

// Movie Duels (cg_text.cpp, CG_CaptionSizeCvar): cg_textprintscale auf
// 0.25 .. 2 begrenzt. Vorgabe wie in der installierten SP-DLL
// MovieDuels-gamex86_64.dll vom 27.10.2026: "0.66f", CVAR_ARCHIVE (Eintrag der
// Cvar-Tabelle nachgelesen; mdgit vom 13.10. hatte noch "0.5f", ebenso die
// aeltere 32-Bit-DLL und die MP-cgame).
inline constexpr float kMdTextprintscale = 0.66F;
[[nodiscard]] float mdGroesse(float cgTextprintscale) noexcept;

// cg.caption* nach CG_CaptionText.
struct Untertitel {
    double startMs = 0.0;             // cg.captionTextTime
    std::vector<std::string> zeilen;  // cg.captionText[0 .. scrollTextLines-1]
    int buchstabenMs = 0;             // cg.captionLetterTime
    int textY = 0;                    // cg.captionTextY (eine Mitte, kein Anfang)
    float schriftGroesse = 1.0F;      // fFontScale (Movie Duels: * cg_captionSize)
    double ersteSeiteBis = 0.0;       // cg.captionNextTextTime der ersten zwei Zeilen
};

// RE_Font_StrLenPixels: Jedi Academy/Movie Duels ueberspringen ^0-^9, Jedi
// Outcast (JK2_MODE) zaehlt sie mit.
[[nodiscard]] int breite(const menue::Schrift& f, const std::string& text, float scale, Spiel spiel);

// CG_CaptionText: den Text in Zeilen brechen (an Leerzeichen und ! ? , . ; :,
// sobald eine Zeile 640 Punkte erreicht), Buchstabenzeit aus der Klanglaenge.
// klangMs: cgi_S_GetSampleLength; balkenZiel: client_camera.bar_height_dest
// (48 nach camera ENABLE, sonst 0); mdGroesseJetzt: nur Movie Duels.
[[nodiscard]] Untertitel setzen(const std::string& text, int klangMs, double startMs, bool imKino, float balkenZiel,
                                const menue::Schrift& schrift, Spiel spiel, float mdGroesseJetzt = kMdTextprintscale);

struct Zeile {
    std::string text;
    int x = 0;   // RE_Font_DrawString(x, y, ...) im virtuellen Bildschirm 640x480
    int y = 0;
};
// Die Clientbilder, in denen CG_DrawCaptionText laeuft: com_maxfps "125" ->
// min_msec 1000 / 125 = 8 ms (Com_Frame, common.cpp; wie kSaberClientTaktMs).
inline constexpr double kClientBildMs = 8.0;

// CG_DrawCaptionText zur Zeit ms: die gezeichneten Zeilen (leer: nichts mehr).
// Je zwei Zeilen, bis ihre Buchstabenzeit um ist. Das Spiel blaettert im
// ersten Clientbild NACH captionNextTextTime und rechnet von dort weiter (die
// Bilder ab der Ankunft des Untertitels alle kClientBildMs). imKino/balkenZiel:
// Stand jetzt (Movie Duels ab Groesse 1).
[[nodiscard]] std::vector<Zeile> zeigen(const Untertitel& u, double ms, const menue::Schrift& schrift, Spiel spiel, bool imKino,
                                        float balkenZiel);

}  // namespace bhed::untertitel

#endif  // BHED_UNTERTITEL_H
