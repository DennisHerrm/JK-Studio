// wegpunkte.h - das Wegnetz der NPCs: lesen wie die Engine, erzeugen, pruefen
//
// Gewuenscht (11.10.2026): "kannst du noch einbauen das man npc waypoints
// erstellen kann bzw. er die automatisch erstellt kannst auch die originalen
// maps anschauen von JKA und JKO das du verstehst wie das system aufgebaut ist".
//
// So baut Movie Duels das Netz (code/game/g_nav.cpp, g_navigator.cpp,
// NAV::LoadFromEntitiesAndSaveToFile):
//
//   Knoten     waypoint, waypoint_small (Wegpunkte), waypoint_navgoal* (Ziele),
//              point_combat (Kampfpunkte)
//   Kanten     NUR vom Designer: target, target2, target3, target4 eines
//              Knotens nennen den targetname eines anderen; targetJump ist
//              eine Sprungkante (nur wer springen kann, nimmt sie)
//   automatisch NUR Kampf- und Zielpunkte: zu den 4 naechsten Knoten
//              (MIN_WAY_NEIGHBORS) bis 500 Einheiten (MAX_EDGE_AUTO_LEN),
//              hoechstens 60 hoeher oder tiefer (MAX_EDGE_FLOOR_DIST), im PVS;
//              zu einem anderen Kampf-/Zielpunkt nur bis 250 und hoechstens
//              einer (MAX_NONWP_NEIGHBORS); nicht mit NO_AUTO_CONNECT
//   Wegpunkt   Kasten -20 -20 -24 bis 20 20 45; spawnflags 1 SOLID_OK,
//              2 DROP_TO_FLOOR; ein Wegpunkt im Festen wird mit "ERROR:
//              Waypoint ... in solid!" verworfen
//
// In den 33 Original-Karten mit Wegnetz stehen 150 bis 490 Wegpunkte je
// Level, fast alle benannt und meist mit einem target (Ketten); der naechste
// Nachbar liegt im Median 226 Einheiten weg, verbundene im Median 306.
//
// Weil normale Wegpunkte NICHT automatisch verbunden werden, schreibt der
// Erzeuger die Kanten als target..target4 - sonst laeufen die NPCs im Spiel
// nicht.
#ifndef BHED_WEGPUNKTE_H
#define BHED_WEGPUNKTE_H

#include "bhed/bsp.h"

#include <array>
#include <cstddef>
#include <functional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace bhed {

struct WegKnoten {
    enum class Art { Wegpunkt, Ziel, Kampf };
    Art art = Art::Wegpunkt;
    int entity = -1;            // Index in den Entities
    std::array<float, 3> ort{};
    std::string name;           // targetname
    bool keinAuto = false;      // NO_AUTO_CONNECT (Kampf-/Zielpunkte)
};

struct WegKante {
    int a = -1;                 // Knotenindizes
    int b = -1;
    bool designer = true;       // aus target..target4 / targetJump
    bool sprung = false;        // targetJump
};

struct WegNetz {
    std::vector<WegKnoten> knoten;
    std::vector<WegKante> kanten;
    // target-Namen, die keinen Knoten finden ("nav unable to locate target")
    std::vector<std::pair<int, std::string>> fehlendeZiele;   // (Knoten, Name)
};

// Ist eine Strecke frei? (Fuer das PVS der automatischen Kanten - die Engine
// fragt gi.inPVS; eine freie Sichtlinie ist die strengere, sichere Naeherung.)
// Leer: alles gilt als sichtbar.
using WegSicht = std::function<bool(const std::array<float, 3>& a, const std::array<float, 3>& b)>;

[[nodiscard]] bool istWegKlasse(const std::string& klasse);
[[nodiscard]] WegNetz baueWegNetz(const std::vector<MapEntity>& ents, const WegSicht& sicht = {});

// --- Erzeugen ----------------------------------------------------------------
//
// Ein Strahl in die Karte: true bei Treffer, dann Trefferpunkt und Normale (fuer pruefeWegNetz).
using WegStrahl = std::function<bool(const std::array<float, 3>& a, const std::array<float, 3>& b,
                                     std::array<float, 3>& treffer, std::array<float, 3>& normale)>;

// rc609 W3a: die Welt, wie die Figuren sie sehen.
struct WegSpurTreffer {
    float anteil = 1.0F;          // trace.fraction
    bool imFesten = false;        // startsolid oder allsolid
    std::array<float, 3> ende{};  // trace.endpos
    std::array<float, 3> normale{};
};
struct WegWelt {
    // gi.trace mit Kasten gegen die Kollision des Spiels, Maske der NPCs (MASK_NPCSOLID); Tueren offen, wie die
    // Kantenpruefung des Spiels sie oeffnet. Ohne: keine Erzeugung.
    std::function<WegSpurTreffer(const std::array<float, 3>& a, const std::array<float, 3>& mins,
                                 const std::array<float, 3>& maxs, const std::array<float, 3>& b)>
        spur;
    // Behaelt das Spiel einen waypoint mit diesem origin (SP_waypoint, NAV::SpawnedPoint)? Leer: ja.
    std::function<bool(const std::array<float, 3>& ort)> knoten;
    // Behaelt das Spiel die Kante (NAV::TestEdge, in Jedi Outcast HardConnect)? Leer: ja.
    std::function<bool(const std::array<float, 3>& a, const std::array<float, 3>& b)> kante;
    // rc609 W3b: springt eine Figur, die springen darf (SCF_NAV_CAN_JUMP), vom Wegpunkt mit origin a zu dem mit origin
    // b (NPC_TryJump/NPC_Jump, wie STEER::Path eine Sprungkante nimmt)? Leer: keine Sprungkanten.
    std::function<bool(const std::array<float, 3>& a, const std::array<float, 3>& b)> sprung;
    // rc609 H2: G_ClearLOS von a nach b (Strahl mit CONTENTS_OPAQUE durch hoechstens drei func_glass, NPC_senses.cpp:929
    // OpenJK). Leer: keine Kampfpunkt-Vorschlaege.
    std::function<bool(const std::array<float, 3>& a, const std::array<float, 3>& b)> sicht;
};

struct WegErzeugung {
    float abstand = 192.0F;     // Rasterweite (Originale: Median 226)
    int maxKnoten = 1023;       // NUM_NODES 1024 mit dem Platzhalter [0] (graph_vs.h); der Aufrufer zieht vorhandene ab
    // Je Startpunkt: != 0, wenn er selbst ein Wegpunkt ist - dann ist sein origin der Knoten (nicht der Boden + 25).
    std::vector<char> startIstWegpunkt;
    // rc609 W3b (Mike: "highlight/draw an area and then click generate", Startpunkt per Klick). Alle leer bzw. -1: wie
    // die volle Erzeugung (W3a).
    // Nur wo das true ist, waechst das Laufgitter (ein gemalter Bereich); Argument: der Boden.
    std::function<bool(const std::array<float, 3>& boden)> laufErlaubt;
    // Nur wo das true ist, entsteht ein neuer Punkt (Sperrbereiche); Argument: der Boden unter dem origin.
    std::function<bool(const std::array<float, 3>& boden)> punktErlaubt;
    // So viele erste Startpunkte saeen das Laufgitter; die uebrigen (vorhandene Wegpunkte) kommen nur dazu, wenn das
    // Gitter sie erreicht. -1: alle saeen.
    int saat = -1;
    // Keine Rasterpunkte, wo ein vorhandener Wegpunkt (ein Startpunkt, der Wegpunkt ist) zu Fuss hoechstens eine
    // Rasterweite entfernt liegt - die Stelle ist schon abgedeckt.
    bool nurUnbedeckt = false;
    // Fortschritt (Anteil 0..1 und Schritt 1 Laufgitter, 2 Punkte, 3 Kanten, 4 Ketten); false bricht ab. Aendert das
    // Ergebnis nicht.
    std::function<bool(float anteil, int schritt)> fortschritt;
};

struct NeuerWegpunkt {
    std::array<float, 3> boden{};      // Bodenpunkt
    std::array<float, 3> ort{};        // origin (Boden + 25: der Kasten beginnt 1 ueber dem Boden)
    std::vector<int> nachbarn;         // Indizes in der Ergebnisliste
    int start = -1;                    // rc609 W1: aus diesem Startpunkt (Index), sonst -1
};

// rc609 W3a: von den Startpunkten aus zu Fuss wie Pmove (Stufen bis STEPSIZE, Boden bis MIN_WALK_NORMAL) ueber
// ein feines Gitter, dann Punkte im Abstand `abstand`, oben und unten an Treppen und Rampen und wo das Raster
// eine Stelle nicht abdeckt; Kanten, die eine Figur gerade gehen kann und die das Spiel behaelt. Einzelheiten in
// wegpunkte.cpp. Braucht das Raster mehr Punkte, als das Spiel nimmt (maxKnoten), wird es weiter (bis zur dreifachen
// Rasterweite).
struct WegErzeugungInfo {
    float abstand = 0.0F;   // die Rasterweite, die es wurde
    // Ketten, die nicht durchkamen (ein Teil des Netzes bleibt ohne Verbindung): der letzte Punkt, die Stelle des
    // Laufgitters danach, und wie oft auf dem Stueck danach kein Punkt moeglich war, keine gerade Linie zu Fuss ging oder
    // die Kantenpruefung des Spiels die Kante verwarf.
    struct Luecke {
        std::array<float, 3> anker{};
        std::array<float, 3> weiter{};
        int ohnePunkt = 0;
        int ohneGehen = 0;
        int ohneKante = 0;
    };
    std::vector<Luecke> luecken;
    bool abgebrochen = false;   // rc609 W3b: fortschritt sagte false (das Ergebnis ist leer)
};
[[nodiscard]] std::vector<NeuerWegpunkt> erzeugeWegpunkte(const std::vector<std::array<float, 3>>& starts,
                                                          const WegWelt& welt, const WegErzeugung& p = {},
                                                          WegErzeugungInfo* info = nullptr);

// Die Kanten auf target..target4 verteilen (die Engine verbindet in beide
// Richtungen): erst ein aufspannender Wald, damit das Netz zusammenhaengt, dann
// die uebrigen, ein volles Ende gibt Kanten ueber eine Kette ab (rc609 W3a).
// Ergebnis je Knoten: die Indizes, die er als target..target4 nennt.
// rc609 W4 (Plan G "Abdeckung"): begehbarer Boden (das Laufgitter der Erzeugung von den Startpunkten aus), den kein
// vorhandener Wegpunkt (istWegpunkt je Startpunkt != 0) zu Fuss in `reichweite` erreicht. `weiter` (Anteil 0..1) false
// bricht ab.
struct WegAbdeckung {
    std::vector<std::array<float, 3>> offen;   // Boden der Gitterzellen ohne Wegpunkt in Reichweite (je 32 x 32)
    std::size_t zellen = 0;                    // Gitterzellen insgesamt
    bool abgebrochen = false;
};
[[nodiscard]] WegAbdeckung wegAbdeckung(const std::vector<std::array<float, 3>>& starts, const std::vector<char>& istWegpunkt,
                                        const WegWelt& welt, float reichweite, const std::function<bool(float anteil)>& weiter = {});
// rc609 W4: der origin fuer einen Wegpunkt am Boden `boden` wie bei der Erzeugung (Boden + 25), wenn das Spiel ihn dort
// behaelt; sonst die naechste solche Stelle bis `weite` auf demselben Boden mit freier Sicht. false: keine.
[[nodiscard]] bool wegStelleSuchen(const std::array<float, 3>& boden, const WegWelt& welt, float weite, std::array<float, 3>& ort);

// rc610 F46 B4-3: frei = freie target-Felder je Punkt (nullptr: `felder` fuer jeden), schon = Paare (min, max), die
// schon verbunden sind und kein Feld brauchen (nullptr: keine)
[[nodiscard]] std::vector<std::vector<int>> verteileZiele(const std::vector<NeuerWegpunkt>& wp, int felder = 4, const std::vector<int>* frei = nullptr,
                                                          const std::set<std::pair<int, int>>* schon = nullptr);

// --- rc609 W3b: Bereiche, Pfade, Sprungkanten ---------------------------------
//
// Mike (03.11.): "more control over where the waypoints are generated ... highlight/draw an area and then click
// generate. Like paint a path around the map." Ein Bereich ist ein Pinselstrich (Kreise um die gemalten Bodenpunkte)
// oder ein Umriss (Lasso); er gilt fuer Boden, der hoechstens eine Kastenhoehe (64) ueber oder unter den gemalten
// Punkten liegt - eine Etage darueber oder darunter gehoert nicht dazu.
struct WegBereich {
    enum class Art { Erzeugen, Sperre };
    Art art = Art::Erzeugen;
    bool lasso = false;                          // Umriss (sonst Pinselstrich)
    float raster = 192.0F;                       // Rasterweite beim Erzeugen in diesem Bereich
    float pinsel = 96.0F;                        // Halbmesser des Pinsels
    std::vector<std::array<float, 3>> punkte;    // Boden unter dem Pinsel bzw. die Ecken des Umrisses
};
[[nodiscard]] bool imWegBereich(const WegBereich& b, const std::array<float, 3>& boden);
// Gespeichert mit der Karte (nicht in der .bsp): eine Zeile je Bereich.
[[nodiscard]] std::string wegBereicheText(const std::vector<WegBereich>& b);
[[nodiscard]] std::vector<WegBereich> wegBereicheLesen(const std::string& text);

// Pfad: entlang der gemalten Linie (Bodenpunkte) je `abstand` ein Punkt auf dem Boden (wie die Erzeugung: Kasten der
// Figur, origin = Boden + 25, nur wo das Spiel den Wegpunkt behaelt), Nachbarn verbunden, wo die Kantenpruefung des
// Spiels die Kante in beide Richtungen behaelt. `brueche`: die Mitte jeder Strecke ohne Kante, `ohnePunkt`: Stellen
// der Linie, an denen das Spiel keinen Wegpunkt behielte.
struct WegPfad {
    std::vector<NeuerWegpunkt> punkte;
    std::vector<std::array<float, 3>> brueche;
    std::vector<std::array<float, 3>> ohnePunkt;
};
[[nodiscard]] WegPfad erzeugePfad(const std::vector<std::array<float, 3>>& linie, const WegWelt& welt, float abstand);

// Sprungkanten (targetJump) vorschlagen: das Spiel springt nur auf einer Sprungkante (STEER::Path,
// g_navigator.cpp:5027-5034 OpenJK), prueft sie beim Laden nicht (WE_JUMPING ohne TestEdge, :1878-1884) und laesst nur
// Figuren mit SCF_NAV_CAN_JUMP darauf (is_valid :556). Vorgeschlagen wird eine Kante nur zwischen Teilen des Netzes, die
// sonst nicht verbunden sind, und nur, wenn WegWelt::sprung in beide Richtungen ja sagt (das Spiel geht jede Kante in
// beide Richtungen). Je Wegpunkt ein targetJump-Feld: `hatSprung` != 0 heisst belegt. `weiter` false bricht ab.
struct WegSprung {
    int von = -1;    // traegt targetJump
    int nach = -1;
};
[[nodiscard]] std::vector<WegSprung> sprungKanten(const std::vector<std::array<float, 3>>& orte,
                                                  const std::vector<std::vector<int>>& nachbarn,
                                                  std::vector<char> hatSprung, const WegWelt& welt,
                                                  const std::function<bool()>& weiter = {});

// --- rc609 H2: Kampfpunkt-Vorschlaege hinter Deckung ---------------------------
//
// Mike (03.11.): point_combat "should be sprinkled around randomly (usually behind cover) ... used by gunners to move to
// cover or get a better angle for shooting". Ein Schuetze (Sturmtruppler) waehlt einen Kampfpunkt mit
// NPC_FindCombatPoint (OpenJK code/game/NPC_combat.cpp:2721-2899, Movie Duels 9 :3487 gleich); ST_Commander verlangt
// CP_CLEAR|CP_COVER, sobald er den Feind nicht mehr sieht oder der Feind zu nah ist (AI_Stormtrooper.cpp:2071-2111), und
// immer CP_AVOID_ENEMY|CP_HAS_ROUTE|CP_TRYFAR mit avoidDist 200 (:2129-2132). Fuer einen Punkt (origin `ort`, wie
// level.combatPoints[i].origin) und einen Feind, der mit seinem Ursprung `feind` steht, gilt die Deckungsregel, wenn
//   Kasten     gi.trace(origin, NPC->mins, NPC->maxs, origin, MASK_NPCSOLID) nicht startsolid/allsolid (:2860-2864;
//              playerMins/Maxs -16..16 x -24..40, NPC_stats.cpp:2028)
//   Abstand    CP_CLEAR: Abstand zum Feind hoechstens visrange (:2812-2816; stats.visrange 1024, NPC_stats.cpp:1988);
//              CP_AVOID_ENEMY: mindestens avoidDist (:2844-2848)
//   CP_CLEAR   NPC_ClearLOS(Muendung, Feind) (:2869-2878): von der Muendung (CalcEntitySpot SPOT_WEAPON, um den origin
//              versetzt) auf Ursprung oder Kopf des Feindes (G_ClearLOS Position zu Entity, NPC_senses.cpp:967-984)
//   CP_COVER   NOT NPC_ClearLOS(origin, Feind) (:2881-2884): vom origin selbst weder Ursprung noch Kopf
// Jedi Outcast (codeJK2/game/NPC_combat.cpp:2489-2700): CP_COVER prueft nur den Ursprung (NPC_ClearLOS(origin,
// enemyPosition)), CP_CLEAR vom origin aus (nicht von der Muendung) auf Ursprung oder Kopf, Abstand bis visrange; kein
// avoidDist ohne Schwert des Feindes (AI_Stormtrooper.cpp:2203-2207 JK2), gesammelt in CP_COLLECT_RADIUS 512 um den Feind.
struct WegDeckungRegeln {
    bool jk2 = false;
    float sichtweite = 1024.0F;   // NPCInfo->stats.visrange (Vorgabe NPC_stats.cpp:1988)
    float meiden = 200.0F;        // avoidDist von ST_Commander (Jedi Academy); Jedi Outcast 0
    float sammeln = 0.0F;         // Jedi Outcast: CP_COLLECT_RADIUS 512 um den Feind; 0 = ohne
};
// G_ClearLOS (WegWelt::sicht) und der Kasten der Figur (frei = nicht im Festen); `kastenFrei` leer: frei.
using WegSichtFn = std::function<bool(const std::array<float, 3>& a, const std::array<float, 3>& b)>;
using WegKastenFn = std::function<bool(const std::array<float, 3>& o)>;
[[nodiscard]] bool wegDeckungGilt(const WegSichtFn& sicht, const WegKastenFn& kastenFrei, const std::array<float, 3>& ort,
                                  const std::array<float, 3>& feind, const WegDeckungRegeln& r);

struct WegDeckungWunsch {
    WegDeckungRegeln regeln;
    float reichweite = 192.0F;   // Kandidaten: Zellen des Laufgitters, die ein Wegpunkt zu Fuss in dieser Weite erreicht
    float feindRaster = 192.0F;  // Feindstellen: eine Zelle des Laufgitters je feindRaster x feindRaster
    float abstand = 231.0F;      // Vorschlaege untereinander und zu vorhandenen Kampfpunkten mindestens so weit
    int hoechstens = 64;
    std::vector<std::array<float, 3>> vorhanden;                    // origin der vorhandenen point_combat
    std::function<bool(const std::array<float, 3>& boden)> erlaubt;  // nur dort (gemalte Bereiche); leer: ueberall
    std::function<bool(float anteil)> weiter;                       // false bricht ab
    int faeden = 1;                                                 // so viele Faeden pruefen die Kandidaten
    // Der Kasten der Figur am origin (frei = nicht im Festen) in der Welt zur Laufzeit; leer: WegWelt::spur (die Welt der
    // Erzeugung, Tueren offen).
    WegKastenFn kastenFrei;
};
struct WegDeckung {
    std::array<float, 3> boden{};
    std::array<float, 3> ort{};          // origin des point_combat (Boden + 25, ganzzahlig wie "Kampfpunkt setzen")
    int feinde = 0;                      // Feindstellen, gegen die die Regel gilt
    std::array<float, 3> feind{};        // eine davon (die naechste an ihrer Mitte) - zum Nachpruefen
    std::array<float, 3> feindMitte{};   // Mitte der Feindstellen
    float richtung = 0.0F;               // Gier vom Punkt zur Mitte (Grad, 0 = +x)
};
struct WegDeckungErgebnis {
    std::vector<WegDeckung> punkte;      // die besten zuerst
    std::size_t kandidaten = 0;
    std::size_t feindStellen = 0;
    std::size_t mitDeckung = 0;          // Kandidaten, fuer die die Regel gegen mindestens eine Feindstelle gilt
    bool abgebrochen = false;
};
// Kandidaten: das Laufgitter der Erzeugung (W3a) von den Startpunkten aus, Zellen in `reichweite` eines vorhandenen
// Wegpunkts (istWegpunkt je Startpunkt) - die Abdeckung aus W4. Feindstellen: Zellen des Laufgitters. Je Kandidat zaehlt,
// gegen wie viele Feindstellen die Regel gilt; die mit den meisten zuerst, mit `abstand` untereinander.
[[nodiscard]] WegDeckungErgebnis wegDeckungVorschlaege(const std::vector<std::array<float, 3>>& starts, const std::vector<char>& istWegpunkt,
                                                       const WegWelt& welt, const WegDeckungWunsch& w);

// --- Pruefen ----------------------------------------------------------------
struct WegBefund {
    enum class Art { ImFesten, Schwebend, Allein, ZielFehlt, ZuVieleZiele };
    Art art;
    int knoten = -1;
    std::string text;           // fuer die Liste (Englisch, der Aufrufer uebersetzt die Art)
};

// `strahl` wie oben; leer: nur die Pruefungen ohne Karte (Allein, ZielFehlt). rc609 W3b: `boden` (wenn gesetzt) sucht
// den Boden fuer "schwebend" wie die Engine (Drop To Floor mit allen Entities, auch Boeden aus func_static).
[[nodiscard]] std::vector<WegBefund> pruefeWegNetz(const WegNetz& netz, const std::vector<MapEntity>& ents,
                                                   const WegStrahl& strahl = {}, const WegStrahl& boden = {});

}  // namespace bhed
#endif
