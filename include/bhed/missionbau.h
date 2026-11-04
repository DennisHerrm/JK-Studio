// missionbau.h - eine Mission anlegen und als .pk3 zusammenstellen
//
// Gegenstueck zu mission.h, das Missionen FINDET. Anlass (11.10.2026): beim
// Bau einer eigenen Mission ging alles ausser den Skripten nur mit eigenen
// Hilfsskripten - Entities setzen, die .ent schreiben (sie ERSETZT alle
// Entities der Karte), Skripte uebersetzen, die .str mit ENDMARKER (ohne
// bricht Movie Duels schon beim Start ab), das Archiv packen.
//
// Hier steht der Teil ohne Oberflaeche, damit er sich pruefen laesst
// (tests/missionbautest.cpp). Die Oberflaeche sammelt ein, was offen ist,
// und schreibt das Ergebnis mit writePk3 (pk3.h) und in den Missionsordner.
#ifndef BHED_MISSIONBAU_H
#define BHED_MISSIONBAU_H

#include "bhed/bsp.h"
#include "bhed/commands.h"
#include "bhed/pk3.h"
#include "bhed/script.h"

#include <string>
#include <vector>

namespace bhed {

// Erlaubt sind Buchstaben, Ziffern und _ - so heissen alle Karten und
// Skriptordner von Raven und Movie Duels, und die Engine sucht Dateien ohne
// Ruecksicht auf Gross/Klein. Hoechstens 48 Zeichen.
[[nodiscard]] bool gueltigerMissionsName(const std::string& name);

// Die Entities einer NEUEN Mission auf einer vorhandenen Karte:
//
//   - alle Entities der Karte (die .ent ersetzt sie alle, also muessen
//     worldspawn, Tueren und Wegpunkte mit hinein),
//   - der Spielerstart loest das Intro aus (target -> target_scriptrunner),
//     genau wie in den Movie-Duels-Missionen,
//   - ein target_scriptrunner mit usescript "<ordner>/intro".
//
// Hat der Spielerstart schon ein target, bekommt der neue Scriptrunner
// denselben Namen - beide werden ausgeloest, nichts geht verloren. Fehlt ein
// Spielerstart, wird einer bei (0 0 64) angelegt.
struct NeueMission {
    std::vector<MapEntity> entities;
    std::string introRef;        // "md_x/intro"
    std::string introText;       // Vorlage des Intro-Skripts
    bool startAngelegt = false;  // kein info_player_start auf der Karte
};
[[nodiscard]] NeueMission neueMission(const std::vector<MapEntity>& karte, const std::string& ordner,
                                      const std::string& name);

// Was in das Archiv gehoert.
struct MissionInhalt {
    std::string name;                 // "md_tpa_jedi" -> maps/md_tpa_jedi.bsp/.ent
    std::string bsp;                  // die Kartengeometrie (leer: nicht mitpacken)
    std::vector<MapEntity> entities;  // ALLE Entities
    // rc609 W2b: der Text der .ent (Entity-Texteditor, Kommentare und Reihenfolge);
    // leer: writeEntities(entities).
    std::string entText;
    struct Skript {
        std::string ref;              // "md_tpa/intro_jedi"
        std::string text;             // Quelltext
    };
    std::vector<Skript> skripte;
    std::vector<Pk3Datei> weitere;    // Klaenge, strings/*.str, levelshots ... unveraendert
};

// Was beim Zusammenstellen auffiel. Code + Argument wie die Pruefcodes, die
// Oberflaeche zeigt sie in der eingestellten Sprache.
//
//   S001  Skript fehlt              arg: "md_x/end"
//   S002  Skript laesst sich nicht uebersetzen   arg: "md_x/end", dazu diag*
//   S003  kein Spielerstart
//   S004  .str ohne ENDMARKER - ergaenzt          arg: Dateiname
//   S005  target ohne Ziel          arg: "<Entity>: <Name>"
//   S006  Name ungueltig            arg: Name
//   S007  Pfad verlaesst das Spiel  arg: Pfad (rc609 W2a F6: "..", "::", Laufwerk, fuehrender Strich)
//   S008  zu viele Schluessel       arg: "<Entity>: <Anzahl>" (rc609 W2a F8: MAX_SPAWN_VARS 64)
//   S009  Entity zu lang            arg: "<Entity>: <Zeichen>" (rc609 W2a F8: MAX_SPAWN_VARS_CHARS 2048)
struct MissionBefund {
    bool fehler = false;   // true: das Archiv wird so nicht geschrieben
    std::string code;
    std::string arg;
    // Bei S002 die erste Meldung des Uebersetzers (Diag): Code (C001...),
    // Argument und der Text - die Oberflaeche zeigt sie wie beim Compile.
    std::string diagCode;
    std::string diagArg;
    std::string diagText;
};

// Die Dateien des Archivs in fester Reihenfolge: Karte, .ent, je Skript .txt
// und .IBI, dann die weiteren. Fehler (fehler == true) stehen in befunde; die
// Liste ist dann trotzdem vollstaendig, nur ohne die nicht uebersetzbaren
// .IBI - der Aufrufer entscheidet.
[[nodiscard]] std::vector<Pk3Datei> missionDateien(const MissionInhalt& m, const CommandDb& db,
                                                   std::vector<MissionBefund>& befunde);

// rc609 W2a F6: ein Pfad, den die Engine liest und den das Programm in den
// Missionsordner schreiben darf - kein "..", kein "::" (FS_FOpenFileRead weist
// beides ab, OJK code/qcommon/files.cpp:1283), kein Laufwerk (":"), kein
// fuehrender Strich.
[[nodiscard]] bool sichererSpielPfad(const std::string& pfad);

// rc609 W2a F8: die Grenzen je Entity, die das Spiel beim Laden prueft - fuer
// .ent, "In die BSP uebernehmen" und Missionen (S008, S009 als Fehler).
// G_ParseSpawnVars: hoechstens MAX_SPAWN_VARS (64) Paare, G_AddSpawnVarToken:
// hoechstens MAX_SPAWN_VARS_CHARS (2048) Zeichen (Summe von Laenge + 1 je
// Schluessel und Wert). OJK code/game/g_local.h:221-222, g_spawn.cpp:1188, 1255;
// Movie Duels 9 gleich (code/game/g_local.h:246-247, g_spawn.cpp:1353, 1427);
// JK2 gleich (codeJK2/game/g_local.h:176-177).
// kMaxSpawnVars und kMaxSpawnVarsChars stehen seit rc609 W2b in bsp.h (der Leser prueft sie auch).
void entityGrenzenPruefen(const std::vector<MapEntity>& ents, std::vector<MissionBefund>& befunde);

// rc609 W2a F1: die Skripte, die ein Skript selbst startet - run ( "x" ) und
// set ( "SET_*SCRIPT", "x" ) (SET_SPAWNSCRIPT, SET_DEATHSCRIPT ...), auch in
// Bloecken; ohne die Verhaltensnamen wie bei skriptVerweise.
[[nodiscard]] std::vector<std::string> skriptFolgeVerweise(const Script& sc);

// Die ICARUS-Pfade, auf die die Entities zeigen (usescript, deathscript ...),
// ohne die Verhaltensnamen ("idle", "default" ...), die die Engine statt
// eines Skripts versteht (NPC_spawn.cpp, BSTATE_STRINGS).
[[nodiscard]] std::vector<std::string> skriptVerweise(const std::vector<MapEntity>& ents);

}  // namespace bhed
#endif
