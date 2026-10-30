// pazaak.h - Menue-Editor M7 Punkt 14: die Pazaak-Ownerdraws von Movie Duels (2000 + Art * 100 +
// Platz, ui/pazaak/sje_pazaak.menu) - Spielfeld, Hand, Namen, Punkte, Dialog, Kartenwahl, Anleitung.
// Ohne laufende Partie: ein beschrifteter Beispielzustand (Spielmitte, Kartenwahl befuellt).
//
// Portiert aus OpenJK-Ableger Movie Duels: code/ui/ui_pazaak.cpp (UI_Pazaak_OwnerDraw, SJE_Pazaak_DrawCardSlot,
// SJE_Pazaak_DrawSelCardSlot, SJE_Pazaak_DrawSideDeckSlot, SJE_Pazaak_DrawHandSlot,
// SJE_Pazaak_DrawNames, SJE_Pazaak_DrawPoints, SJE_Pazaak_DrawDialog, SJE_Pazaak_DrawTimeout,
// SJE_Pazaak_DrawWaiting, SJE_Pazaak_HowToWords, SJE_Pazaak_DrawHowTo, Pazaak_GetCardShader,
// Pazaak_GetNeutralCardValue, Pazaak_GetCardValue, Pazaak_DrawCardText, Pazaak_CacheShaders) und
// code/ui/pazaak_core.h (Kartennummern); dort aus Jedi Knight Galaxies.
//
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2001 - 2013, Activision, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//
//   Movie Duels: GNU General Public License version 2.
#ifndef BHED_MENUE_PAZAAK_H
#define BHED_MENUE_PAZAAK_H

#include "bhed/menue/ownerdraw.h"

#include <array>
#include <functional>
#include <string>
#include <vector>

namespace bhed::menue {

// PZCARD_*: -2 Rueckseite, -1 keine, 0-9 Deck 1-10, 10-15 +1..+6, 16-21 -1..-6, 22-27 +/-1..6,
// 28 Gleichstand, 29 Doppel, 30 +/-1/2, 31 2&4, 32 3&6
struct PazaakKarte {
    int id = -1;
    int param = 0;
};

// PzkState / PDlgData, Beispiel einer laufenden Partie
struct PazaakZustand {
    std::array<PazaakKarte, 18> feld{{{4, 0}, {7, 0}, {12, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0},
                                      {6, 0}, {9, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}};
    std::array<PazaakKarte, 8> hand{{{11, 0}, {18, 0}, {25, 0}, {28, 0}, {-2, 0}, {-2, 0}, {-2, 0}, {-2, 0}}};
    std::array<int, 23> karten{2, 2, 1, 1, 0, 0, 2, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0};   // playercards[].amount
    std::array<int, 10> seitendeck{10, 11, 12, 13, 16, 17, 22, 23, 28, -1};   // sidedeck (inuse = wie oft darin)
    int punkteSpieler = 16, punkteGegner = 17;      // total_player / total_opponent
    std::string nameSpieler = "Padawan", nameGegner = "Gambler";
    bool stehtSpieler = false, stehtGegner = false;   // standing_*
    std::string dialog1, dialog2;                    // PDlgData.line1/2 (leer = kein Dialog)
    int restZeit = 0;                                // Sekunden bis timeout (0 = keiner)
    bool wartet = false;                             // waitingStartTime
};

struct PazaakKontext {
    std::function<int(const std::string& text, float scale)> breite;   // R_Font_StrLenPixels mit "ergoec"
    int zeitMs = 0;                                                      // PZK_Millis
    PazaakZustand zustand;
};

// UI_Pazaak_OwnerDraw(ownerDraw, x, y, w, h, shader); false = keine Pazaak-Nummer (2000..2999)
bool pazaakOwnerDraw(int id, float x, float y, float w, float h, const std::string& shader, const PazaakKontext& k, std::vector<OdTeil>& out);

}  // namespace bhed::menue

#endif  // BHED_MENUE_PAZAAK_H
