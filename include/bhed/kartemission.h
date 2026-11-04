// kartemission.h - rc609 B2: Mission <-> Map ohne Verlust
//
// shank (03.11.): "wenn ich merke in der Mission es fehlt einen Bereich in der
// map. das ich rueber Switchen kann zu map den kurz adden und danach weiter
// machen kann an der Mission."
//
// Nach dem Bau im Map-Tab laedt der Mission-Tab die neue .bsp nach. Seine
// Entities sind dann ein Abgleich aus drei Listen:
//   alt      die Entities der .bsp VOR dem Bau (Entity-Lump)
//   mission  die Entities des Mission-Tabs (mit allen ungespeicherten Aenderungen)
//   neu      die Entities der neu gebauten .bsp
// Ergebnis: neu, darauf die Aenderungen der Mission gegenueber alt.
//
// Regel fuer "dieselbe Entity" (Abgleich, in dieser Reihenfolge, je Partner
// hoechstens einmal):
//   1. worldspawn ist worldspawn.
//   2. gleicher classname und gleicher Name (targetname, sonst
//      script_targetname, sonst NPC_targetname; ohne Gross/Klein) - bei
//      mehreren Kandidaten der mit dem naechsten Ort.
//   3. Brush-Entity ohne Namen: gleicher classname und gleiche Grenzen des
//      Brush-Modells *N (je Achse hoechstens kOrtToleranz) - die Nummer selbst
//      darf sich beim Bau verschieben.
//   4. sonst gleicher classname und gleicher Ort (origin, je Achse hoechstens
//      kOrtToleranz).
// Aenderungen der Mission:
//   - eine Entity Schluessel fuer Schluessel gleich einer alten: unveraendert,
//     es gilt die Fassung der neuen Karte (auch wenn der Map-Tab sie geaendert hat);
//   - gegenueber ihrer alten Entity geaendert: die geaenderten Schluessel kommen
//     auf die Entity der neuen Karte, "model" *N bleibt der neuen Karte;
//   - eigene (ohne alte Entity): steht sie nach der Regel schon in der neuen Karte,
//     werden ihre Schluessel dort eingetragen (nicht doppelt), sonst angehaengt;
//   - von der Mission geloescht: auch in der neuen Karte entfernt.
// Was nicht aufgeht, steht in den Befunden (nichts geht still verloren).
#ifndef BHED_KARTEMISSION_H
#define BHED_KARTEMISSION_H

#include "bhed/bsp.h"
#include "bhed/script.h"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace bhed {

inline constexpr float kOrtToleranz = 1.0F;   // Einheiten je Achse (Regel 3 und 4)

// Grenzen der Brush-Modelle einer .bsp (Lump 7 LUMP_MODELS, dmodel_t: mins[3],
// maxs[3], ... - 40 Bytes, IBSP 46 und RBSP 1 gleich; OJK code/qcommon/qfiles.h).
using ModellGrenzen = std::array<float, 6>;
[[nodiscard]] std::vector<ModellGrenzen> bspModellGrenzen(const std::string& bsp);

// Ein Name, den ein Skript anspricht (affect, use, kill, remove - wie der
// Missionsgraph), mit Herkunft fuer die Meldung.
struct SkriptVerweis {
    std::string name;
    std::string skript;    // Anzeigename des Skripts
    std::string befehl;    // affect, use, kill, remove
};
void skriptVerweiseSammeln(const Script& sc, const std::string& skript, std::vector<SkriptVerweis>& aus);

struct KarteMissionBefund {
    enum class Art {
        NameFehlt,          // Verweis (Skript oder Schluessel) zeigte vorher auf eine Entity, jetzt auf keine
        ModellFehlt,        // *N gibt es in der neuen Karte nicht
        ModellVerschoben,   // *N meint in der neuen Karte andere Geometrie
        ModellAngepasst,    // *N wurde zu *M (Hinweis, kein Bruch)
        EntityNichtInKarte, // von der Mission geaendert, in der neuen Karte entfernt - angehaengt
    };
    Art art = Art::NameFehlt;
    std::string was;       // Name bzw. "*N"
    std::string wo;        // "Skript x: affect", "func_door tuer: target", ...
    std::string neu;       // ModellAngepasst: die neue Nummer
};

struct KarteMissionErgebnis {
    std::vector<MapEntity> entities;
    std::vector<int> neuVonMission;   // Index in mission -> Index in entities (-1: entfernt)
    std::vector<KarteMissionBefund> befunde;
    int unveraendert = 0;
    int geaendert = 0;       // Aenderungen auf eine Entity der neuen Karte gelegt
    int eigeneAngehaengt = 0;
    int eigeneSchonDa = 0;   // eigene, die in der neuen Karte schon standen
    int entfernt = 0;        // von der Mission geloescht, auch in der neuen Karte
    [[nodiscard]] bool gebrochen() const;   // ein Befund ausser ModellAngepasst
};

[[nodiscard]] KarteMissionErgebnis karteMissionAbgleich(const std::vector<MapEntity>& alt, const std::vector<ModellGrenzen>& altModelle,
                                                        const std::vector<MapEntity>& mission, const std::vector<MapEntity>& neu,
                                                        const std::vector<ModellGrenzen>& neuModelle,
                                                        const std::vector<SkriptVerweis>& skriptVerweise);

}  // namespace bhed
#endif
