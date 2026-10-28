// sprungpruefung.h - Karten-Editor, Stufe 9 Punkt 4: Sprungweite und Platz pruefen
// (eigene Funktion, nicht in NetRadiant-Custom).
//
// Portiert aus OpenJK: die Bewegung ist der Pmove-Port (navSpielerPmove: PM_WalkMove,
// PM_AirMove, PM_CheckJump mit METROID_JUMP, PM_StepSlideMove, PM_CheckDuck; bg_pmove.cpp),
// die Masse sind die des Spielers (DEFAULT_MINS/MAXS -16..16 x -24..40, CROUCH_MAXS_2 16,
// MIN_WALK_NORMAL 0.7, g_speed 250, g_gravity 800, forceJumpHeight/Strength).
//
//   Copyright (C) 1999-2005 Id Software, Inc.; 2000-2013 Raven Software; 2013-2026 OpenJK contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
//
// Platz: in einem Raster ueber dem Bereich faellt der Spielerkasten (geduckt) von oben; jeder
// begehbare Boden (Normale z >= 0.7) ist ein Standpunkt - "stehen", wenn dort auch der volle
// Kasten frei ist, sonst "nur geduckt". Bis zu vier Boeden je Spalte.
// Sprung: von Standpunkten an Kanten (ein Nachbar ohne Boden in Stufenhoehe) laeuft der Spieler
// mit voller Geschwindigkeit in jede Richtung los und springt (Sprungtaste gehalten), einmal
// ohne und einmal mit Machtsprung der gewaehlten Stufe - die echte Bewegung, Bild fuer Bild.
// Gemerkt wird jeder Sprung, der auf einem anderen Boden landet (weiter als ein Raster, nicht
// tiefer als die Fallgrenze), mit Bahn.
#ifndef BHED_KARTE_SPRUNGPRUEFUNG_H
#define BHED_KARTE_SPRUNGPRUEFUNG_H

#include "bhed/navlauf.h"

#include <atomic>
#include <vector>

namespace bhed::karte {

struct SprungOptionen {
    NavVec bereichMin{};
    NavVec bereichMax{};
    float raster = 32.0F;
    int machtStufe = 0;           // 0: nur der normale Sprung
    bool jk2 = false;
    int richtungen = 8;
    int maxKanten = 160;          // Startpunkte hoechstens (gleichmaessig ausgewaehlt)
    float fallGrenze = 128.0F;    // tiefer gelandet zaehlt nicht als Sprungziel
    int bildMs = 16;
};

struct StandPunkt {
    NavVec ort{};                 // Ursprung des Spielers (Kastenmitte unten + 24)
    bool stehen = true;           // false: nur geduckt
    bool kante = false;
};

struct Sprung {
    NavVec start{};
    NavVec landung{};
    bool macht = false;           // nur mit Machtsprung erreicht
    float weite = 0.0F;           // waagerecht
    float hoehe = 0.0F;           // Landung - Start
    std::vector<NavVec> bahn;
};

struct SprungBericht {
    std::vector<StandPunkt> punkte;
    std::vector<Sprung> spruenge;
    std::size_t kanten = 0;
    std::size_t simuliert = 0;    // Laeufe
};

// abbrechen: von aussen setzbar (Faden); fortschritt 0..1000.
[[nodiscard]] SprungBericht sprungPruefen(const NavSpurFn& spur, const SprungOptionen& o, const std::atomic<bool>* abbrechen = nullptr,
                                          std::atomic<int>* fortschritt = nullptr);

// Ein einzelner Sprung (fuer Test und Anzeige): Anlauf in Richtung gier mit voller
// Geschwindigkeit, Sprung im ersten Bild, Taste gehalten; Ende bei der Landung oder nach 4 s.
// Liefert true bei einer Landung; landung/bahn/scheitel gefuellt.
bool sprungSimulieren(const NavSpurFn& spur, const NavVec& start, float gier, int machtStufe, bool jk2, int bildMs, NavVec& landung,
                      std::vector<NavVec>* bahn = nullptr, float* scheitel = nullptr);

}  // namespace bhed::karte

#endif
