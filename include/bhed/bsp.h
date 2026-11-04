// bsp.h - Entities aus einer JKA-Karte lesen
//
// Nur das Entity-Lump, nicht die Geometrie. Das ist Lump 0 in jedem
// id-Tech-3-BSP und reiner Text - bei duel_kamino_lp.bsp 15 KB fuer 111
// Entities. Ein Leser fuer Flaechen, Patches und Lightmaps waere ein
// Vielfaches an Arbeit und braeuchte einen Zeichner dazu; fuer einen
// Skripteditor genuegt, was hier steht:
//
//   targetname   - was affect, use, kill, remove ansprechen
//   cameraGroup  - was SET_CAMERA_GROUP erwartet
//   classname    - zum Sortieren und Anzeigen
//   origin       - fuer spaetere Kamerapfade
//
// Unterstuetzt werden RBSP (Jedi Academy, Jedi Outcast) und IBSP (Quake 3).
// Die Kennung wird geprueft, aber nicht die Fassungsnummer: das Entity-Lump
// liegt bei allen an derselben Stelle.
#ifndef BHED_BSP_H
#define BHED_BSP_H

#include <cstddef>
#include <string>
#include <vector>

namespace bhed {

struct MapEntity {
    // Kurzfelder: dieselben Werte wie find("classname") usw. (rc609 W2b F11).
    std::string classname;
    std::string targetname;
    std::string cameraGroup;
    std::string origin;
    std::vector<std::pair<std::string, std::string>> keys;

    // Der Wert, den das Spiel fuer diesen Schluessel nimmt (entSchluesselIndex).
    [[nodiscard]] const std::string* find(const std::string& key) const;
};

// --- rc609 W2b F11: doppelte Schluessel und Gross/Klein wie das Spiel ------------
//
// Das Spiel legt beim Lesen ALLE Paare der Reihe nach ab (G_ParseSpawnVars) und
// liest dann auf zwei Wegen:
//   - Felder der Tabelle fields[] (G_ParseField, fuer jedes Paar der Reihe nach,
//     Q_stricmp): bei doppeltem Schluessel gewinnt der LETZTE. OJK
//     code/game/g_spawn.cpp:223 (Tabelle), :957 (G_ParseField), :1093 (Schleife);
//     MD9 code/game/g_spawn.cpp:321, :1104; JK2 codeJK2/game/g_spawn.cpp:188, :787.
//   - alles andere ueber G_SpawnString (OJK :80, MD9 :78): der ERSTE Treffer,
//     Q_stricmp - in JK2 strcmp, also genaue Schreibweise (codeJK2 g_spawn.cpp:54).
// Die Tabellen unterscheiden sich je Spiel (Movie Duels hat dazu fullName, icon,
// player_victoryscript; JK2 eine eigene) - es gilt das aktive Regelwerk.
[[nodiscard]] bool entFeldSchluessel(const std::string& key);
// Index des Paars, das das Spiel liest; -1 = keins.
[[nodiscard]] int entSchluesselIndex(const MapEntity& e, const std::string& key);

// --- rc609 W2b F12: der Leser des Spiels -----------------------------------------
//
// G_ParseSpawnVars (OJK code/game/g_spawn.cpp:1210, MD9 :1376, JK2 codeJK2
// g_spawn.cpp:991) mit COM_Parse (OJK/MD9 code/qcommon/q_shared.cpp:281/:299):
// Leerraum ist jedes Byte <= ' ', "//" und "/* */" sind Kommentare, ein Token in
// Anfuehrungszeichen reicht bis zum naechsten '"' (auch ueber Zeilen), ein Wort
// ohne Anfuehrungszeichen bis zum naechsten Byte <= 32 (als signed char - ein
// Latin-1-Zeichen beendet das Wort, q_shared.cpp:398), hoechstens 1023 Zeichen
// (MAX_TOKEN_CHARS - 1). Ein NUL beendet den Text.
inline constexpr int kMaxSpawnVars = 64;          // MAX_SPAWN_VARS (OJK g_local.h:221, MD9 :246, JK2 :176)
inline constexpr int kMaxSpawnVarsChars = 2048;   // MAX_SPAWN_VARS_CHARS (G_AddSpawnVarToken: Laenge + 1 je Token)
inline constexpr std::size_t kMaxTokenZeichen = 1023;   // MAX_TOKEN_CHARS - 1 (q_shared.h:197)

// Wo ein Paar und ein Block im Text stehen (Byte-Versatz; Token samt Anfuehrungszeichen).
struct EntPaarSpanne {
    std::size_t kAnf = 0, kEnde = 0;
    std::size_t wAnf = 0, wEnde = 0;
};
struct EntBlockSpanne {
    std::size_t anf = 0;    // '{'
    std::size_t ende = 0;   // hinter '}'
    std::vector<EntPaarSpanne> paare;   // gleich lang wie MapEntity::keys
};
// Woran das Spiel abbricht (G_Error) - und ein offenes Anfuehrungszeichen, das
// COM_Parse still bis zum Ende liest.
enum class EntFehler {
    Keiner,
    KlammerErwartet,        // "found %s when expecting {"
    OhneSchliessendeKlammer,   // "EOF without closing brace" (Zeile der '{')
    KlammerOhneWert,        // "closing brace without data"
    ZuVielePaare,           // "MAX_SPAWN_VARS"
    ZuVieleZeichen,         // G_AddSpawnVarToken: "MAX_SPAWN_VARS"
    OffenesAnfuehrungszeichen,
    ErsteNichtWorldspawn,   // G_SpawnEntitiesFromString (nur entTextPruefen)
};
struct EntTextFehler {
    EntFehler art = EntFehler::Keiner;
    int zeile = 0;          // ab 1
    std::string token;
    // Ein Token in Anfuehrungszeichen, das ueber ein Zeilenende reicht, in der
    // Entity des Fehlers (ab 1; 0 = keins): fast immer ein vergessenes '"' -
    // COM_Parse liest dann bis zum naechsten und bricht erst spaeter ab.
    int zeileMehrzeilig = 0;
};
// Liest wie das Spiel. false bei einem Fehler (out bis davor gefuellt); Bloecke
// ohne Paar entstehen nicht (das Spiel gibt sie gleich wieder frei: "no classname").
bool parseEntitiesGenau(const std::string& text, std::vector<MapEntity>& out, std::vector<EntBlockSpanne>* spannen = nullptr,
                        EntTextFehler* fehler = nullptr);

struct MapData {
    std::string path;
    std::string magic;              // "RBSP" oder "IBSP"
    int version = 0;
    std::vector<MapEntity> entities;

    // Sortiert und ohne Dubletten - genau das, was eine Klappliste braucht.
    std::vector<std::string> targetNames;
    std::vector<std::string> cameraGroups;

    // Die Skripte, auf die diese Karte verweist.
    //
    // So haengt eine Mission zusammen: die Karte selbst weiss nichts von
    // Skripten, aber ihre Entities tun es -
    //
    //     target_scriptrunner  usescript    "md_tfoaj/intro_jedi"
    //     NPC_spawner          deathscript  "md_tfoaj/templeguard_defeated"
    //
    // Daraus wird scripts/md_tfoaj/intro_jedi.txt bzw. .ibi. Bei
    // md_tfoaj_jedi.ent sind das sieben Skripte, und alle sieben liegen im
    // selben Archiv.
    struct ScriptRef {
        std::string path;       // "md_tfoaj/intro_jedi"
        std::string key;        // "usescript", "deathscript", ...
        std::string owner;      // targetname oder classname der Entity
    };
    std::vector<ScriptRef> scripts;

    [[nodiscard]] bool empty() const noexcept { return entities.empty(); }
};

// Die Namenslisten und Skriptverweise aus den Entities aufbauen.
//
// Getrennt von readBsp, weil die Entities auch aus einer .ent-Datei kommen
// koennen - Movie Duels arbeitet so.
void collectNames(MapData& out);

// Liest den rohen Inhalt einer .bsp. false, wenn es keine ist.
[[nodiscard]] bool readBsp(const std::string& bytes, MapData& out,
                           std::string* error = nullptr);

// Nur das Entity-Lump (Lump 0) einer .bsp als Text, mit denselben Pruefungen
// wie readBsp (Kennung RBSP/IBSP/FBSP, Lump innerhalb der Datei). Ohne
// Protokoll - darf in Arbeitsfaeden laufen (rc603, Missionssuche).
[[nodiscard]] bool readBspEntityLump(const std::string& bytes, std::string& text,
                                     std::string* error = nullptr);

// Nur das Entity-Lump als Text zerlegen (parseEntitiesGenau ohne Fehler: was vor
// einem Fehler steht, bleibt). Getrennt gehalten, damit es ohne Datei pruefbar ist.
void parseEntities(const std::string& text, std::vector<MapEntity>& out);

// --- Entities bearbeiten und als .ent schreiben (Entity-Inspektor) --------
//
// Einen Schluessel setzen: das Paar, das das Spiel liest (entSchluesselIndex) -
// bei doppeltem Schluessel also dasselbe, das Vorschau und Spiel nehmen. Ein
// leerer Wert entfernt den Schluessel ganz (alle Paare dieses Namens ohne
// Gross/Klein). Anfuehrungszeichen und Zeilenumbrueche kann das Format nicht
// tragen (der Leser der Engine kennt kein Escape); sie werden weggelassen.
// classname, targetname, cameraGroup und origin bleiben in Takt.
void setKey(MapEntity& e, const std::string& key, const std::string& value);

// Als .ent-Text. parseEntities liest daraus genau dieselben Entities zurueck.
// Eine .ent ERSETZT alle Entities der Karte (cm_load.cpp) - also gehoert die
// ganze Liste hinein, auch worldspawn.
[[nodiscard]] std::string writeEntities(const std::vector<MapEntity>& ents);

}  // namespace bhed
#endif
