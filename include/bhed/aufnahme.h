// aufnahme.h - Zeitleiste 2 Punkt 1: eine gesteuerte Figur -> Laufziele.
//
// Die Vorschau laesst eine Figur mit dem Pmove-Port laufen (navSpielerPmove,
// Tempo aus der .npc). Daraus werden Laufziele, wie das Spiel NPCs per ICARUS
// laufen laesst: je Punkt ein ref_tag und ein set ( SET_NAVGOAL, ... ) - der
// Befehl wartet im Spiel, bis die Figur angekommen ist (Q3_Set: TID_MOVE_NAV).
// Den Weg dazwischen sucht im Spiel die NPC-Navigation; eine Zeit je Strecke
// oder eine Geschwindigkeitskurve kennt die Engine nicht - die Aufnahme
// liefert nur die Punkte und die Gangart.
#ifndef BHED_AUFNAHME_H
#define BHED_AUFNAHME_H

#include "bhed/navlauf.h"

#include <cstddef>
#include <vector>

namespace bhed {

struct AufnahmeProbe {
    NavVec ort{};        // ps.origin
    bool gehen = false;  // gehen (SET_WALKING) statt rennen (SET_RUNNING)
};

// Punkte nur wo noetig: Douglas-Peucker mit `toleranz` (Einheiten, waagerecht
// und senkrecht), dazu muss jede gerade Strecke zwischen zwei behaltenen
// Punkten fuer den Kasten einer Figur frei sein (`spur`, Stufen bis 18
// Einheiten wie NAV::ClearPathToPoint, mins z + STEPSIZE) - sonst bleibt ein
// Punkt dazwischen. Ein Wechsel der Gangart behaelt den Punkt davor. Ergebnis:
// die Indizes der behaltenen Proben ohne die erste (der Start), mit der
// letzten; leer, wenn sich die Figur nicht bewegt hat (keine Probe weiter als
// 16 Einheiten vom Start).
[[nodiscard]] std::vector<std::size_t> aufnahmeVereinfachen(const std::vector<AufnahmeProbe>& p, float toleranz, const NavSpurFn& spur);

}  // namespace bhed

#endif  // BHED_AUFNAHME_H
