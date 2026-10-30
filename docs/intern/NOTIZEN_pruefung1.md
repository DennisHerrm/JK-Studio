# Notizen pruefung1 - Mission-Tab/Vorschau: "nichts zugeschnitten"

Zweig `pruefung1` ab karte1 fdb3b348 (Integration 3), Worktree `Y:\sprache_cs`,
Build `Y:/sprache_cs/build-sprache`.

Auftrag (shank): Alles im Mission-Tab und in der Vorschau muss fuer neue,
selbst gebaute Missionen genauso funktionieren wie fuer die Originale. Nichts
darf auf vorhandene Karten, Skripte, Figuren- oder Entity-Namen zugeschnitten
sein; erlaubt sind nur Engine-eigene Tabellen/Sonderfaelle mit Quelle.

## Vorgehen

Unabhaengige Suche (ohne fruehere Pruefberichte) in src/scene*, ablauf*,
navlauf*, mover*, camtrack*, roff*, ausruestung*, npcklasse*, saberkontakt*,
efx*, gui/app_view3d*, app_mission*, app_kamera, app_animation, app_bewegung,
app_wetter:

- alle Vergleiche mit Zeichenketten-Literalen (`==`, `!=`, `rfind`, `sameName`
  ...) gesammelt und die ungewoehnlichen einzeln gelesen,
- Literale mit Karten-, Skript-, Figuren-, Shader-, Sound- und Modellnamen,
- Kommentare mit Missionsnamen (ob der Code daneben allgemein ist),
- Marker wie "gemessen", "Naeherung", "angepasst an", "Sonderfall",
- feste Koordinaten-Tripel, feste Zeiten.

Engine-Vergleich gegen OpenJK (JACoders/OpenJK master, code/ und codeJK2/)
und MovieDuels-SJE-SP-Rend2 (master), dazu die Auszuege in `Y:\quellen`.

## Fundliste

### Zuschnitte / Abweichungen (behoben)

1. **Kino-Animationen `_humanoid_<karte>` fuer alle Regelwerke nach der Movie-
   Duels-Liste** (`kinoSkeletteAbgleichen`, gui/app_view3d_spiel.inc). Die
   Liste humanoidExact[] aus G_StandardHumanoid (g_client.cpp:1340-1407 MD)
   galt auch unter OpenJK und Jedi Outcast. Engine: OpenJK haengt die Kino-
   Datei nur an das Skelett `_humanoid` (Q_stricmp, code/game/NPC_stats.cpp:
   1116), Jedi Outcast laedt keine (codeJK2/game/NPC_stats.cpp,
   G_ParseAnimFileSet:467 ff.), Movie Duels jedes Skelett der Liste
   (NPC_stats.cpp:1249 MD). Jetzt `kinoSkelettAnhang(skelett, spiel)` in
   src/npcklasse.cpp; der Abgleich merkt sich Karte UND Regelwerk und nimmt
   einen alten Anhang beim Wechsel wieder ab.
   - rot: npcklasse_probe 2 Fehler (OpenJK haengt an _humanoid_md/protocol/
     eigenes Skelett, JK2 an _humanoid), gruen: 0 Fehler.
2. **kill/remove fanden eine Figur der Karte nur bei gleicher Schreibweise**
   (scene.cpp, `a.name != ziel`). Engine: CQuake3GameInterface::Kill/Remove
   suchen mit G_Find (FOFS(targetname), Q_stricmp; Q3_Interface.cpp:9661,
   9712 OpenJK). Eine neue Mission, deren Skript `kill ( "held" )` schreibt,
   waehrend die Karte `HELD` nennt, liess die Figur stehen. Allgemein auf
   sameName umgestellt, dazu die anderen Namenssuchen derselben Art:
   affect-Zuordnung (GetByName, Q_strupr), SET_LOOK_TARGET (findEntity, G_Find;
   "NULL" ohne Gross/klein), Blickziel auf eine bewegte Figur, fx_target_beam-
   Ziel (mover.cpp, G_Find).
   - rot: pruefmission 4 Fehler (OpenJK und MD, Satz B: Figur nicht getoetet,
     Spuren A/B verschieden), gruen: 108 Proben, 0 Fehler.
   - fx_target_beam-Ziel (MoverSim): rot mover_probe 1 Fehler (Probe mit der
     alten Zeile), gruen 0. Dieselbe Suche in der Oberflaeche (Strahl zeichnen,
     app_view3d_spiel.inc; Portalkamera misc_portal_surface -> locateCamera,
     app_view3d.cpp) nimmt jetzt `engineNamenGleich` (bhed/scene.h) - dort
     ohne eigenen Selbsttest (gleiche Funktion wie die gepruefte).

### Tote Zuschnitte (entfernt)

3. `gehDauerMs`/`gehDauerInFahrtMs` (scene.cpp): Anlauf 300 ms + Faktor 1,5,
   "angepasst an neun Messungen im Spiel (md_dd_jedi ...)". Nirgends mehr
   aufgerufen (Laufen rechnet planeLauf). Entfernt.
4. `typeFromClassname` (scene.cpp): "NPC_Foo" -> "foo" als Naeherung.
   Nirgends mehr aufgerufen (npcklasse.cpp hat die SP_NPC_*-Tabelle). Entfernt.

### Gesehen, Engine-Vorbild (bleibt, mit Quelle im Code)

- npcklasse.cpp: SP_NPC_*-Tabelle beider Regelwerke, MD-Zufallslisten
  (NPC_spawn.cpp:63 ff. MD), JK2-Tabelle.
- ausruestung.cpp: NPC_WeaponsForTeam (OpenJK/MD/JK2, "monmothma",
  "md_grogu", "kyle"), Schwertstile nach Klasse/Rang, Saberklaenge/-griffe
  nach CLASS_DESANN/LUKE/KYLE (WP_SaberInitBladeData).
- scene.cpp: Bossformel der Gesundheit nach NPC_type tavion_sith_sword,
  tavion_scepter, kyle_boss, alora_dual, alora_boss (NPC_spawn.cpp:1051 OJ,
  1781 MD); ClassNames[] von JK2; Torso-Tabellen der Waffen (MD);
  "kyle" als Spieler nur unter Jedi Outcast (g_client.cpp:1620 JK2);
  munroWaffenAbgleich nur JK2 (g_active.cpp:1119).
- mover.cpp / app_view3d_spiel.inc: tie_fighter/tie_bomber.md3
  (g_breakable.cpp:590, 1221, 1281 OJ), Modellpfade der misc_*-Spawnfunktionen,
  Bruchklaenge nach Material.
- saberkontakt.cpp: BOTH_CIN_16/17, BOTH_A7_HILT, MD BOTH_CIN_50/70,
  BOTH_MD_CIN_1 (wp_saber.cpp:4636 OJ, 8282 MD).
- app_view3d.cpp: Blobschatten nach CLASS_RANCOR/ATST/SAND_CREATURE
  (CG_PlayerShadow); Spielermodell g_char_model "jedi_hm" (g_main.cpp:801).
- app_wetter.cpp, app_saber.cpp, app_view3d.cpp: Renderer-Bilder der Engine
  (gfx/world/rain, gfx/effects/sabers/... - cgs.media).
- Missionsbau (app_mission.cpp): Namensvorschlag `md_<karte>_jedi`, Ordner
  ohne `_jedi/_sith`, Ziel-pk3 im MD-Ordner - nur Vorschlaege im Dialog
  (aenderbar), kein Verhalten der Vorschau.
- Regelwerk "automatisch": Movie Duels, wenn ein Spielordner `...\md` heisst
  oder "movie duels" enthaelt (movieDuelsOrdner) - Erkennung der
  Installation, in den Einstellungen umschaltbar.

### Naeherungen (kein Namenszuschnitt, nur vermerkt)

- scene.cpp: Todesanimation bei kill immer BOTH_DEATH1 (im Spiel nach
  Trefferstelle/zufaellig); Hock-Grundzustand; Schwerkraft einzelner Klassen.
- navlauf.cpp:5543: upmove in BS_NOCLIP nicht gefuehrt.
- efxdraw.cpp: Quadrat nicht gegen die Welt geschnitten (2162, 2198).

## Testsatz (Aufgabe 3)

`tests/pruefmission.cpp` (ctest `pruefmission`): vier selbst gebaute
Mini-Missionen, nur eigene Daten (Karten-Entities, Skripte, NPC-Werte,
Klang-/Animations-/ROFF-Laengen stehen im Test):

1. Laufen/Wegnetz, Animation mit Haltezeit, task/dowait, signal/waitsignal,
   run, Stimme (CHAN_VOICE) mit dowait, SET_WATCHTARGET auf eine Figur nur aus
   der Karte, SET_ORIGIN/SET_ANGLES.
2. Kamera: ENABLE, MOVE/PAN auf ref_tag-Marken ($tag$), ZOOM, FADE, SHAKE,
   DISABLE.
3. Mover/ROFF: move/rotate auf einem Brush-Modell mit script_targetname,
   dowait auf die Fahrt, PLAY_ROFF mit dowait.
4. Entity-Logik: use -> target_relay -> target_delay -> target_counter ->
   target_scriptrunner (usescript), NPC_spawner auf use mit spawnscript,
   fx_runner, func_door, kill/remove, target_level_change.

Jede laeuft unter OpenJK und Movie Duels zweimal - Satz A mit Namen wie in den
Originalen (obi1, cam1b, md_am/intro2_sith, kor2/roffs/boulder ...), Satz B
mit frischen Namen, in der Karte gross geschrieben. Verglichen wird Bild fuer
Bild (100 ms): Ort/Winkel/Animation/Sichtbarkeit jeder Figur und ihre
Schritte, Kamera, Mover, Effektstarts, Skriptstarts, Benutzungen, Hinweise,
Kartenwechsel. Dazu je Vorlage Proben, dass das Verhalten eintritt.

Beim Bau gefunden (kein Fehler, Engine-Verhalten): zwei task-Bloecke gleichen
Namens im selben affect - der zweite ersetzt den ersten schon beim Laden
(CTaskManager::AddTaskGroup leert die Gruppe), die Vorschau bildet das nach.

Grenze der Probe: ohne Kartengeometrie (navWeltAusEntities, kein Boden) fand
die Wegplanung fuer einen Weg mit Knick (Wegpunkte 160/0, 320/120, Ziel
480/120) keinen Weg; gerade Strecken laufen. Mit Geometrie prueft das die
Vorschau im Selbsttest (mission1 Mini-Szene); nicht weiter untersucht.

## Offen / nicht geprueft

- Untertitel: die Vorschau zeigt keine (SET_USE_SUBTITLES/SET_CAPTIONTEXTCOLOR
  sind Stub) - nichts zuzuschneiden, aber auch nichts zu pruefen.
- GUI-seitige Teile (Figurenmodelle, Kino-Skelett im laufenden Programm)
  prueft weiter der Selbsttest mission1 (Mini-Szene m1szene mit eigenem
  _humanoid_m1szene) - er laedt die Grundkarte md_am_sith aus den
  Spielordnern.
- Die GUI-Selbsttests mission1/kamera/missionsbau/umriss arbeiten sonst mit
  Original-Missionen (md_am_sith, md_ga_jedi ...) und brauchen die Spieldaten.

## Schlussrunde

- Sauberer Neubau (`--clean-first`): 0 Warnungen, 0 Fehler.
- ctest: 158/158 gruen (darin neu `pruefmission`, erweitert `npcklasse`,
  `mover`).
- lint_sources, lint_fassung: ok.
- Selbsttests (eigener Laufordner `Y:\pq_laeufe`, Idle-Prioritaet):
  mission1 98 OK / 0 FEHLER, kamera 95/0, missionsbau 95/0, umriss 46/0,
  Modus 1 (BHED_HANDBUCH) 2648/0.
- Kartenpruefung (tools/kartenpruefung.py, 278 Karten, Movie Duels, gegen
  den Bericht kpi3 nach Integration 3): alle Summen gleich - abgestuerzt/
  haengend, nicht geladen, GPU-Fehler, D3D-Meldungen, Protokollfehler 0 -> 0,
  nicht gezeichnete Entities 1 -> 1, NPCs ohne Modell 2 -> 2.
- Handbuch: keine sichtbare Bedienung geaendert (nur Verhalten wie im
  Spiel), daher keine neuen Bilder/Texte; keine neuen Programmtexte (kein
  tools/i18n_pruefung1.py noetig).

## Pruefung 2 - markierte Naeherungen 1:1 nach der Engine

Zweig `pruefung1` weitergefuehrt (Worktree `Y:\sprache_cs`). Test
`tests/pruefung2.cpp` (ctest `pruefung2`): selbst gebaute Mini-Missionen je
Punkt, OpenJK, Movie Duels und Jedi Outcast. Rot = derselbe Test gegen den
alten Stand der geaenderten Datei (git show HEAD:...), gruen = neu.
Engine-Quellen: OpenJK master (code/, codeJK2/; fehlende Dateien von
raw.githubusercontent.com nach `SP\p2`), MovieDuels-SJE-SP-Rend2 (`SP\mdgit`).

### 1. Todesanimation (bhed/todesanim.h, src/todesanim.cpp)

Vorher fest BOTH_DEATH1. Jetzt die Kette des Spiels: kill aus ICARUS ruft
GEntity_DieFunc(victim, NULL, NULL, o_health, MOD_UNKNOWN) (Q3_Interface.cpp:
9661 OJ / MD gleich), fuer Figuren player_die (NPC_spawn.cpp:1166 OJ). Je
Regelwerk portiert: player_die-Zweige (Mark1_die, Interrogator_die, Galak,
ATST, MD-Droideka, Schluessel am Arm "message", Fallen/MOD_FALLING, normal),
G_CheckSpecialDeathAnim (Rollen, Saltos, Drehangriffe, Knockdown/Getup mit
legsAnimTimer, liegend nach torsoAngles PITCH, hockend), G_CheckLedgeDive mit
G_CheckForLedge (Spuren der NavWelt) und G_Throw (PMF_TIME_KNOCKBACK, MD
charNoKnockback), Rueckstoss-Zweig, G_PickDeathAnim mit G_GetHitLocation
(pos1: vec3_origin -> HL_NONE; Start einer ROFF-Bahn; trigger_hurt:
currentOrigin) und Totflop (BOTH_DEADFLOP1/2) bei einem zweiten kill,
PM_PickAnim (BOTH_DEATH1..25). Unterschiede, die das ergibt: MD springt im
Normalfall direkt zu PM_PickAnim mit dem "spinny"-Nachwurf (g_combat.cpp:
5498-5522 MD), OJ geht ueber G_PickDeathAnim; PLAYER_KNOCKDOWN_HOLD_EXTRA_TIME
4000 (OJ) gegen 300 (MD); JK2 ohne Rollen/Totflop, mit BOTH_RESTRAINED1.
- Zufall: Q_irand mit der Formel von irand (q_math.c:248 OJ), Saat fest aus
  Entity-Nummer und Todeszeit (nie aus dem Namen - pruefmission vergleicht
  Namenssaetze). Im Spiel haengt der Zug an Rand_Init und allen Zuegen davor.
- trigger_hurt-Tode bekommen MOD_TRIGGER_HURT (SHOCK 64: BOTH_DEATH17) bzw.
  MOD_FALLING (FALLING 32) ueber SzenenHilfe::hurtAusloeser.
- holdingSaber: SaberDeactivate (Klinge aus).
- rot: pruefung2 32 Fehler (alter scene.cpp: immer BOTH_DEATH1), gruen 0.
  scenetest erwartete BOTH_DEATH1 (die alte Naeherung) - jetzt "eine
  BOTH_DEATH*, eingefroren" (begruendet im Test).
- Offen (bewusst, Begruendung): renderInfo.torsoAngles kommt im Programm noch
  nicht aus dem Bolzen chestBolt ("thoracic") - die Oberflaeche reicht keinen
  SzenenHilfe::rumpfWinkel herein, es gilt VectorClear wie ohne Bolzen; das
  betrifft nur liegend (LYING_UP/DN) und Drehangriffe (SPIN_90/180). Der Wurf
  ueber die Kante bewegt die Leiche nicht (keine PM_DEAD-Leichenphysik in der
  Vorschau). Nicht nachgebaut: Waffe fallen lassen (TossClientItems, MD mit
  playerUsableWeapons, Kartennamen, com_kotor), Schwert verlieren
  (WP_SaberLose), Todesklaenge (EV_DEATH1-3, *falling1), DeathFX der Droiden,
  NPC_RemoveBody, EF_HELD_BY_*, CONTENTS_NODROP, Ruestung bei trigger_hurt.

### 2. upmove bei BS_NOCLIP

OpenJK/JK2 denken das Verhalten alle 100 ms; das Denkbild dazwischen faehrt
ClientThink mit dem ganzen NPCInfo->last_ucmd (NPC.cpp:2279/2518 OJ, JK2
NPC.cpp:2114). Die Vorschau fuehrte upmove nicht (LaufZustand::letzterHoch neu).
- rot: nach 1,5 s vorn 129,4 / oben 47,1 (OJ, JK2); gruen 129,4 / 130,4 (der
  Rest ist floor(127 * 0,99999994) = 126 nach vorn, auch in der Engine).

### 3. Decals (efxdraw.cpp)

Ein Decal ist im Spiel eine Marke: FxScheduler (FxScheduler.cpp:1855 MD) ruft
CG_ImpactMark - R_MarkFragments schneidet das Quadrat auf die Flaechen
(Port aus saberspuren), CG_AddMarks laesst es MARK_TOTAL_TIME (10 s) stehen
und blendet in der letzten Sekunde die Alpha (alphaFade). Groesse, Farbe,
Alpha, Drehung vom Anlegen; MARKER_OFFSET 0 (gegen Flimmern das
polygonOffset des Shaders); keine CParticle-Keulung.
- rot: 5 Fehler (alt: flaches Quadrat 2 vor der Wand, z -5..15, Lebenszeit
  der .efx), gruen 0. efxtest erwartete das flache Quadrat - jetzt "ohne
  Flaechen keine Marke".
- Offen: der gemeinsame Markenspeicher (256 Polygone, CG_AllocMark) mit den
  Brandspuren der Schwerter; FX_GHOUL2_DECALS. Der Karteneditor (Echtzeit-
  Effekte, app_karte_echtfx.inc) reicht keine BSP herein - dort erscheinen
  Decals jetzt nicht mehr (vorher als Quadrat).

### 4. Weg mit Knick ohne Kartengeometrie

Kein Fehler: NAV::LoadFromEntitiesAndSaveToFile baut mit hoechstens drei
Knoten kein Netz (g_navigator.cpp:1514 OJ, :1471 MD), STEER::SafeToGoTo geht
direkt nur unter 400. Die Probe aus pruefung1 hatte drei Knoten - im Spiel
bleibt die Figur stehen (JK2 laeuft). Mit vier Knoten laeuft sie den Knick.
Test mit beiden Faellen, ohne Codeaenderung.

### 5. Weitere markierte Naeherungen

Gesucht nach "Naeherung", "vereinfacht", "ungefaehr", "approx", "TODO",
"noch nicht", "Stub", "nicht nachgebaut/nachgebildet" in den Missions- und
Vorschau-Dateien.

Portiert:
- scene.cpp "Im Hocken ein anderer Grundzustand ... UNSERE Naeherung":
  PM_Footsteps setzt mit PMF_DUCKED ohne Bewegung BOTH_CROUCH1 (bg_pmove.cpp:
  8174 OJ, :14267 MD, JK2 :5047), ab dem Denkbild nach SET_CROUCHED - nicht
  BOTH_CROUCH1IDLE sofort. rot 3, gruen 0.

Bleibt (Fundstelle -> Engine-Vorbild, Grund):
- app_view3d_spiel.inc:555 Geschuetze mit Feind (pas_fire, Zielen) - die
  Vorschau hat keine Feinde/KI-Kampf.
- app_view3d_spiel.inc:1347 Schaden von fx_target_beam (G_Damage) - kein
  Schadensmodell ausser trigger_hurt.
- app_view3d_spiel.inc:1868 / efxdraw.cpp spurFuer: useBBox als Strahl mit
  Vorlauf (CParticle::UpdateOrigin, theFxHelper.Trace mit Kasten) - braucht
  die Kastenspur (KollisionsKarte) im Effektpfad; Aufwand, kein Hindernis.
- app_view3d_spiel.inc:1970 Leuchten liegender Schwerter, weapon_saber mit
  NPC_type (CG_Item) - offen.
- app_view3d_spiel.inc:2183 misc_sentry_turret klappt nach 1 s auf (pas_think)
  - offen.
- app_view3d.cpp:3403 Blinzeln/Leerlaufmimik (Q_flrand-Zeitgeber,
  cg_players.cpp:5159) - mit fester Saat moeglich, offen.
- app_view3d.cpp:5107 trigger_hurt: Spieler, Ruestung (CheckArmor),
  PM_InForceFall (MD), Reihenfolge im Bild - offen.
- scene.cpp:1797 forced_rgb_saber_colours - offen (in den Mod-Dateien nicht
  benutzt).
- scene.cpp:2685 Fahrzeuge ohne Schwerkraft (Fahrzeugart unbekannt) - offen.
- ausruestung.cpp:18-46 Holster, tag_hack, Schwertklaenge beim Zuenden durch
  SET_SABER1/2, Muendung am Bolzen "*flash" - offen.
- ausruestung.cpp:986 Remove vor dem ersten Add nimmt Ghoul2-Modell 0 (die
  Figur selbst) - Engine-Eigenheit, offen.
- navlauf.cpp:3007 PM_FallToDeath - offen.
- ablauf.cpp:1692 zweites use waehrend der Bruchverzoegerung - offen.
- mover.cpp:1304 misc_model_breakable target4 (wirft sich) - offen.
- camtrack.cpp:706 CAMERA_ACCEL vor ZOOM - Kamera, offen.
- clock.cpp:132 wait( "name" ) bis zum Ende des eigenen Anstosses - gehoert
  zur Zeitleiste (Signale fuer Kamerabahn und Zeitleiste), gemeldet, nicht
  angefasst.
- Neu gesehen: Gehen geduckt (BOTH_CROUCH1WALK/-BACK, Tempo im Ducken) und
  PM_AdjustStandAnimForSlope (Fuesse am Hang, auch im Stehen) fehlen.

## Pruefung 2 - Schlussrunde

Nach dem Neustart des Rechners (Windows Update, 30.10. 00:29) alles neu
gelaufen - die Laeufe von vorher gelten nicht.
- Sauberer Neubau (`--clean-first`, 15:51 min unter Last): 0 Warnungen,
  0 Fehler.
- ctest: 159/159 gruen (darin neu `pruefung2`; geaendert `szene`/scenetest
  und `efxtest`, die beide die alten Naeherungen erwarteten).
- lint_sources, lint_fassung, lint_headers: ok. lint_efxflags meldet "keine
  Flagtabelle gefunden" - die Tabelle steht seit frueher in
  app_view3d_spiel.inc, nicht in app_view3d.cpp; nicht von Pruefung 2.
- Selbsttests (Laufordner `Y:\pq_laeufe\p2_*`, Idle-Prioritaet): mission1
  98 OK / 0 FEHLER, kamera 95/0, missionsbau 95/0, umriss 46/0, Modus 1
  (BHED_HANDBUCH) 2648/0.
- Kartenpruefung (278 Karten, Movie Duels, gegen kpi3): alle Summen gleich -
  abgestuerzt/haengend, nicht geladen, GPU-Fehler, D3D-Meldungen,
  Protokollfehler 0 -> 0, nicht gezeichnete Entities 1 -> 1, NPCs ohne
  Modell 2 -> 2 (832 s).
- Handbuch: keine Bedienung geaendert (nur Verhalten wie im Spiel), keine
  neuen Programmtexte.
- Nicht gegen das Spiel selbst geprueft (kein Spielstart): die Ketten sind
  Zeile fuer Zeile aus den Quellen portiert und mit Mini-Missionen geprueft;
  der Zufall (Q_irand) folgt der Formel, nicht der Folge des Spiels.
- Offen fuer eine naechste Runde: SzenenHilfe::rumpfWinkel in der
  Oberflaeche (chestBolt aus dem Skelett), Waffe/Schwert fallen lassen,
  Leichenphysik nach dem Wurf, Todesklaenge, gemeinsamer Markenspeicher mit
  den Brandspuren, die Liste unter 5.

## English
- Death animation as in the game: `kill` from a script and deaths in a `trigger_hurt` now pick the animation like the engine (stance and timer of the legs, crouching, lying, rolls and flips, a key on the arm, Mark1/ATST/Galak, falling deaths, a ledge in front, hit location, then a random one the model has) for OpenJK, Movie Duels and Jedi Outcast; a second `kill` makes the corpse twitch, a saber blade goes out. Before, every figure died with BOTH_DEATH1.
- BS_NOCLIP: a goal above or below is reached as fast as one ahead (the engine repeats the whole last move command, up/down included, in the think frame between two behaviour thinks).
- Effect decals lie on the map like in the game: clipped to the surfaces they hit, kept for ten seconds and faded out in the last one, with size, colour and rotation from the moment they were laid (before: a flat square in front of the wall for the particle's life).
- `SET_CROUCHED` without moving plays BOTH_CROUCH1 from the next think frame, as the engine does (before: BOTH_CROUCH1IDLE at once).
- Names of entities are found without regard to case, as in the game: `kill`, `remove`, `affect`, `SET_LOOK_TARGET`, a moving watch target, the target of an `fx_target_beam` and the camera of a `misc_portal_surface` now find "HELD" in the map when the script writes "held" (before, a new mission with different spelling left the figure standing or the beam off).
- Story cutscene animations of the level (`models/players/_humanoid_<map>`) follow the rule set: OpenJK attaches them only to the `_humanoid` skeleton, Movie Duels to every skeleton in its standard-humanoid list, Jedi Outcast to none. Switching the rule set takes an old attachment off again.
- New self-test `pruefmission`: four self-built mini missions (walking, animation, signals, voice; camera; movers and ROFF; entity logic, effects, door, level change), each under OpenJK and Movie Duels rules, once with names as in the original missions and once with fresh names - both must give the same result frame by frame.

## Deutsch
- Todesanimation wie im Spiel: `kill` aus einem Skript und Tode in einem `trigger_hurt` wählen die Animation wie die Engine (Haltung und Timer der Beine, Hocken, Liegen, Rollen und Saltos, Schlüssel am Arm, Mark1/ATST/Galak, Sturztod, eine Kante vor der Figur, Trefferort, sonst eine zufällige, die das Modell hat) für OpenJK, Movie Duels und Jedi Outcast; ein zweites `kill` lässt die Leiche zucken, eine Klinge geht aus. Vorher starb jede Figur mit BOTH_DEATH1.
- BS_NOCLIP: ein Ziel darüber oder darunter wird so schnell erreicht wie eines vorn (die Engine wiederholt im Denkbild zwischen zwei Verhaltensschritten den ganzen letzten Bewegungsbefehl, auf/ab eingeschlossen).
- Decals aus Effekten liegen wie im Spiel auf der Karte: auf die getroffenen Flächen zugeschnitten, zehn Sekunden lang, in der letzten ausgeblendet, mit Größe, Farbe und Drehung vom Moment des Anlegens (vorher ein flaches Quadrat vor der Wand für die Lebenszeit des Teilchens).
- `SET_CROUCHED` ohne Bewegung spielt wie die Engine ab dem nächsten Denkbild BOTH_CROUCH1 (vorher sofort BOTH_CROUCH1IDLE).
- Entity-Namen werden wie im Spiel ohne Rücksicht auf Groß-/Kleinschreibung gefunden: `kill`, `remove`, `affect`, `SET_LOOK_TARGET`, ein bewegtes Blickziel, das Ziel eines `fx_target_beam` und die Kamera einer `misc_portal_surface` finden jetzt „HELD“ in der Karte, wenn das Skript „held“ schreibt (vorher blieb in einer neuen Mission mit anderer Schreibweise die Figur stehen oder der Strahl aus).
- Die Kino-Animationen der Karte (`models/players/_humanoid_<karte>`) folgen dem Regelwerk: OpenJK hängt sie nur an das Skelett `_humanoid`, Movie Duels an jedes Skelett seiner Standard-Humanoiden-Liste, Jedi Outcast an keines. Ein Wechsel des Regelwerks nimmt einen alten Anhang wieder ab.
- Neuer Selbsttest `pruefmission`: vier selbst gebaute Mini-Missionen (Laufen, Animation, Signale, Stimme; Kamera; Mover und ROFF; Entity-Logik, Effekte, Tür, Kartenwechsel), je unter OpenJK und Movie Duels, einmal mit Namen wie in den Original-Missionen und einmal mit frischen Namen – beide müssen Bild für Bild dasselbe ergeben.

## 中文
- 死亡动画与游戏一致：脚本中的 `kill` 和 `trigger_hurt` 造成的死亡现在按引擎方式选择动画（腿部姿态与计时器、蹲伏、躺卧、翻滚与空翻、手臂上的钥匙、Mark1/ATST/Galak、坠落死亡、前方的边缘、命中部位，否则随机选择模型拥有的动画），适用于 OpenJK、Movie Duels 和 Jedi Outcast；第二次 `kill` 会让尸体抽搐，光剑剑刃熄灭。以前所有角色都以 BOTH_DEATH1 死亡。
- BS_NOCLIP：位于上方或下方的目标与前方的目标到达速度相同（引擎在两次行为思考之间的帧中重复完整的上一次移动命令，包括上下移动）。
- 特效贴花与游戏一样贴在地图上：裁剪到所击中的表面，保留十秒并在最后一秒淡出，大小、颜色和旋转取自放置时刻（以前是墙前的平面方块，持续粒子寿命）。
- 不移动时的 `SET_CROUCHED` 与引擎一样从下一思考帧起播放 BOTH_CROUCH1（以前立即播放 BOTH_CROUCH1IDLE）。
- 实体名称的查找与游戏一致，不区分大小写：`kill`、`remove`、`affect`、`SET_LOOK_TARGET`、移动中的注视目标、`fx_target_beam` 的目标以及 `misc_portal_surface` 的摄像机，现在在脚本写 "held" 时也能找到地图中的 "HELD"（以前，拼写不同的新任务会让角色原地不动或光束不亮）。
- 关卡的过场动画（`models/players/_humanoid_<地图>`）遵循规则集：OpenJK 只附加到 `_humanoid` 骨架，Movie Duels 附加到其标准人形列表中的每个骨架，Jedi Outcast 不附加。切换规则集会移除旧的附加。
- 新自测 `pruefmission`：四个自建迷你任务（行走、动画、信号、语音；摄像机；移动物体与 ROFF；实体逻辑、特效、门、换图），分别在 OpenJK 和 Movie Duels 规则下运行，一次使用原版任务中的名称，一次使用新名称——两者必须逐帧一致。

## 日本語
- 死亡アニメーションをゲームと同じに：スクリプトの `kill` と `trigger_hurt` による死亡は、エンジンと同じ方法でアニメーションを選びます（脚の姿勢とタイマー、しゃがみ、横たわり、ロールと宙返り、腕の鍵、Mark1/ATST/Galak、落下死、前方の段差、命中部位、それ以外はモデルが持つものからランダム）。OpenJK、Movie Duels、Jedi Outcast に対応し、2 回目の `kill` で死体がけいれんし、ライトセーバーの刃が消えます。以前はすべてのキャラクターが BOTH_DEATH1 で死んでいました。
- BS_NOCLIP：上や下の目標にも前方の目標と同じ速さで到達します（エンジンは 2 回の行動思考の間のフレームで、上下を含む直前の移動コマンド全体を繰り返します）。
- エフェクトのデカールはゲームと同じくマップ上に貼られます：当たった面に合わせて切り取られ、10 秒間残り最後の 1 秒でフェードアウトし、サイズ・色・回転は置かれた瞬間のものです（以前は粒子の寿命の間、壁の前の平らな四角形でした）。
- 移動しない `SET_CROUCHED` はエンジンと同じく次の思考フレームから BOTH_CROUCH1 を再生します（以前はすぐに BOTH_CROUCH1IDLE）。
- エンティティ名はゲームと同じく大文字小文字を区別せずに検索されます：`kill`、`remove`、`affect`、`SET_LOOK_TARGET`、移動する注視対象、`fx_target_beam` のターゲット、`misc_portal_surface` のカメラは、スクリプトが "held" と書いてもマップの "HELD" を見つけます（以前は綴りの異なる新しいミッションでキャラクターが立ったまま、またはビームが消えたままでした）。
- マップのカットシーン用アニメーション（`models/players/_humanoid_<マップ>`）はルールセットに従います：OpenJK は `_humanoid` スケルトンのみ、Movie Duels は標準ヒューマノイド一覧の各スケルトン、Jedi Outcast はなし。ルールセットを切り替えると古い付加は外れます。
- 新しいセルフテスト `pruefmission`：自作のミニミッション 4 つ（歩行・アニメーション・シグナル・音声、カメラ、ムーバーと ROFF、エンティティロジック・エフェクト・ドア・マップ切替）を OpenJK と Movie Duels の両ルールで、オリジナルミッションと同じ名前と新しい名前でそれぞれ実行し、両者がフレームごとに一致することを確認します。
