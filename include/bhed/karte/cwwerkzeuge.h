// cwwerkzeuge.h - Karten-Editor, CW-IMPORT Punkte 3 bis 6 (Clone-Wars-Kette Teil 2, plaene/clonewars_kette.md): eigene Werkzeuge,
// nicht in NetRadiant-Custom. Sie wirken nur, wenn man sie aufruft (shank 14.11.: "zwar drin, aber nicht standard"); jedes ist eine
// Undo-Stufe. Was q3map2 daraus macht, steht je Werkzeug (Quelle: third_party/q3map2, NetRadiant-Custom 20260114).
//
// Portiert aus q3map2 (NetRadiant-Custom 20260114) sind nur diese Regeln (map.cpp SetBrushContents, model.cpp clipflags_doClip);
// die Werkzeuge selbst sind eigener Code.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version (LICENSE).
//
//   Punkt 3 raumSkalieren: die Auswahl (ein Raum) um einen Punkt mit einem Faktor skalieren; der Faktor steht in der Karte als
//           Schluessel cw_scale an einem info_notnull (Marker) und als Kommentar in der .map (raumKommentar) - nur Daten, das Spiel
//           liest beides nicht (G_ParseField uebergeht unbekannte Schluessel; Kommentare ueberliest jeder Leser).
//   Punkt 4 treppeZuRampe: aus einer gewaehlten Treppe (Brushes oder misc_model) eine glatte Clip-Rampe durch die Vorderkanten der
//           Stufen (Unterkante = Unterkante der Treppe, Oberkante = oberster Tritt); die sichtbaren Stufen verlieren ihre Kollision -
//           Brushes bekommen eine verdeckte Seite mit einem nicht festen Shader (SetBrushContents, q3map2 map.cpp:455-503: eine
//           nicht feste Seite macht den ganzen Pinsel durchlaessig; Jedi Academy: textures/system/nodraw = nodraw + nonsolid),
//           ein misc_model verliert seine Clip-Spawnflags (clipflags_doClip, model.cpp:970-999), reine Kollisionspinsel in der Auswahl
//           (alle Seiten nicht gezeichnet) fallen weg.
//   Punkt 5 glatteClipWand: vor einer gewaehlten unregelmaessigen Wand eine glatte Clip-Flaeche - eine Ebene (Quader in Richtung der
//           Wand, vorn am weitesten vorstehenden Punkt) oder die konvexe Huelle.
//   Punkt 6 die Ebene "Collision": alle Clip-Pinsel dieser Werkzeuge (und von "Modell + Clip" mit dem Pruefprofil Clone-Wars-Kette)
//           kommen in eine eigene, benannte Ebene; sie entsteht erst, wenn eines davon benutzt wird.
#ifndef BHED_KARTE_CWWERKZEUGE_H
#define BHED_KARTE_CWWERKZEUGE_H

#include "bhed/karte/mathe.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace bhed::karte {

class Karte;
class Entitaet;
class Pinsel;
struct Kontext;

// --- Punkt 6: die Ebene Collision ---------------------------------------------------------------------------------------------------
inline constexpr const char* kKollisionsEbene = "Collision";
[[nodiscard]] int kollisionsEbeneFinden(const Karte& k);   // -1: keine
// Findet oder legt sie an (oberste Ebene; die aktuelle Ebene bleibt die aktuelle)
int kollisionsEbeneHolen(Karte& k);

// Ein konvexer Pinsel aus Ebenen (Normale nach aussen, Abstand), alle Seiten mit einem Shader; nullptr, wenn er leer ist
[[nodiscard]] std::shared_ptr<Pinsel> pinselAusEbenen(const std::vector<Ebene3d>& ebenen, const std::string& shader, const Kontext& k);

// Die Shader von Jedi Academy (assets1.pk3 shaders/system.shader): clip = playerclip + monsterclip (die Spielfigur und NPCs, keine
// Schuesse), physics_clip = fest (alles), nodraw = nodraw + nonsolid
inline constexpr const char* kClipSpieler = "textures/system/clip";
inline constexpr const char* kClipAlles = "textures/system/physics_clip";
inline constexpr const char* kNichtFestUnsichtbar = "textures/system/nodraw";

// --- Punkt 3: Massstab je Raum ------------------------------------------------------------------------------------------------------
struct RaumOptionen {
    double faktor = 1.2;
    Vektor3d pivot{0, 0, 0};     // um diesen Punkt (Vorgabe im Dialog: Mitte der Auswahl, unten)
    std::string name;            // cw_room (frei)
    bool marker = true;          // info_notnull mit cw_scale setzen
};
struct RaumErgebnis {
    std::size_t primitive = 0;   // skalierte Pinsel und Patches
    std::size_t entitaeten = 0;  // verschobene Punkt-Entitaeten (die Groesse von Modellen bleibt)
    std::shared_ptr<Entitaet> marker;
    std::string fehler;          // englisch
};
// Eine Undo-Stufe "cwScaleRoom"; danach ist die Auswahl samt Marker gewaehlt (File > Save selected gibt sie als eigenes Teil aus)
RaumErgebnis raumSkalieren(Karte& k, const RaumOptionen& o);
// Die Kommentarzeile der .map fuer eine Entitaet mit cw_scale (ohne "//" und Zeilenende), sonst ""
[[nodiscard]] std::string raumKommentar(const Entitaet& e);

// --- Punkt 4: Treppe -> Clip-Rampe ----------------------------------------------------------------------------------------------------
struct TreppeOptionen {
    std::string clipShader = kClipSpieler;
    std::string nichtFest = kNichtFestUnsichtbar;
    double gehen = 0.7;          // MIN_WALK_NORMAL (OpenJK code/game/bg_public.h:69): steiler ist die Rampe eine Wand
};
struct TreppeErgebnis {
    bool ok = false;
    std::string fehler;          // englisch
    std::size_t tritte = 0;      // gefundene Stufenflaechen (verschiedene Hoehen)
    double unten = 0;            // Unterkante (z)
    double oben = 0;             // oberster Tritt (z)
    double steigung = 0;         // Hoehe je Einheit waagerecht
    bool begehbar = true;        // Normale der Rampe z >= gehen
    Vektor3d richtung{1, 0, 0};  // waagerecht, die Treppe hinauf
    std::size_t pinselNichtFest = 0;   // Stufen-Pinsel mit nicht fester Seite
    std::size_t sichtbareSeiten = 0;   // davon war die geaenderte Seite sichtbar (nicht caulk/nodraw)
    std::size_t modelleOhneClip = 0;   // misc_model ohne Clip-Spawnflags
    std::size_t modelleClipModell = 0; // misc_model, deren Shader q3map_clipModel tragen (q3map2 clippt sie trotzdem)
    std::size_t kollisionEntfernt = 0; // reine Kollisionspinsel entfernt
    std::shared_ptr<Pinsel> rampe;
};
// Die Tritte: begehbare, nach oben zeigende Flaechen (Normale z >= gehen) in der Welt, je Polygon die Ecken
struct TreppenFlaeche {
    std::vector<Vektor3d> ecken;
};
// Nur die Rechnung (fuer den ctest): Richtung, Tritte, Rampe als Ebenen
struct TreppenRampe {
    bool ok = false;
    std::string fehler;
    std::vector<Ebene3d> ebenen;
    std::size_t tritte = 0;
    double unten = 0, oben = 0, steigung = 0;
    Vektor3d richtung{1, 0, 0};
};
[[nodiscard]] TreppenRampe rampeRechnen(const std::vector<TreppenFlaeche>& tritte, double unten, double gehen);
// Das Werkzeug auf der Auswahl (Brushes, Patches als Tritte nicht, misc_model ueber die Modell-Ablage k.kontext.modelle); eine
// Undo-Stufe "cwStairsToRamp"; die Rampe kommt in die Ebene Collision und ist danach gewaehlt
TreppeErgebnis treppeZuRampe(Karte& k, const TreppeOptionen& o);

// --- Punkt 5: glatte Clip-Wand ---------------------------------------------------------------------------------------------------------
enum class WandArt { Ebene, Huelle };
struct WandErgebnis {
    bool ok = false;
    std::string fehler;
    std::size_t punkte = 0;      // Ecken der Auswahl
    Vektor3d normale{0, 0, 0};   // Ebene: zur Spielfigur hin
    std::shared_ptr<Pinsel> pinsel;
};
// blickVon: von wo die Spielfigur kommt (die Kamera) - die Clip-Flaeche liegt auf dieser Seite der Wand. Eine Undo-Stufe "cwClipWall"
WandErgebnis glatteClipWand(Karte& k, WandArt art, const Vektor3d& blickVon, const std::string& clipShader);
// Nur die Rechnung: die Ebenen der Clip-Wand aus Punkten. flaechenNormale: die Summe der Normalen mal Flaeche der Seiten zur Figur hin
// (die Richtung der Wand; ohne sie die Hauptachse der Punkte)
[[nodiscard]] std::vector<Ebene3d> wandEbenen(const std::vector<Vektor3d>& punkte, WandArt art, const Vektor3d& blickVon, Vektor3d* normale = nullptr,
                                              const Vektor3d* flaechenNormale = nullptr);

}  // namespace bhed::karte

#endif
