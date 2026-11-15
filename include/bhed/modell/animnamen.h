// modell/animnamen.h - die Namen, die das Spiel in einer animation.cfg kennt (CW-MODELL, 15.11.2026)
//
// G_ParseAnimationFile (OpenJK code/game/NPC_stats.cpp:927-940, Movie Duels Update 9 :1096) und BG_ParseAnimationFile (codemp/game/
// bg_panimate.c:2429, MD9 :6008) suchen jeden Namen der animation.cfg mit GetIDForString(animTable, ...) - auch fuer Fahrzeuge und
// eigene Skelette. Ein unbekannter Name wird uebergangen (die Warnung schreiben nur Entwicklerfassungen). Die Tabelle je Zielspiel ist
// aus cgame/animtable.h erzeugt (tools/modell/gen_animnamen.py -> src/modell/animnamen.inc): JKA SP 1544, MP 1543, JK2 1210,
// Movie Duels SP/MP 3213 Namen.
#ifndef BHED_MODELL_ANIMNAMEN_H
#define BHED_MODELL_ANIMNAMEN_H

#include <cstdint>
#include <string>
#include <vector>

namespace bhed::modell {

enum class Zielspiel : std::uint8_t;

// Die Namen der animTable des Zielspiels (in der Reihenfolge von anims.h) - fuer cfgPruefen(..., bekannt).
[[nodiscard]] const std::vector<std::string>& animNamenSpiel(Zielspiel s);

}  // namespace bhed::modell
#endif
