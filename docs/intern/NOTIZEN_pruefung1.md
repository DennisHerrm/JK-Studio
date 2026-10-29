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

## English
- Names of entities are found without regard to case, as in the game: `kill`, `remove`, `affect`, `SET_LOOK_TARGET`, a moving watch target, the target of an `fx_target_beam` and the camera of a `misc_portal_surface` now find "HELD" in the map when the script writes "held" (before, a new mission with different spelling left the figure standing or the beam off).
- Story cutscene animations of the level (`models/players/_humanoid_<map>`) follow the rule set: OpenJK attaches them only to the `_humanoid` skeleton, Movie Duels to every skeleton in its standard-humanoid list, Jedi Outcast to none. Switching the rule set takes an old attachment off again.
- New self-test `pruefmission`: four self-built mini missions (walking, animation, signals, voice; camera; movers and ROFF; entity logic, effects, door, level change), each under OpenJK and Movie Duels rules, once with names as in the original missions and once with fresh names - both must give the same result frame by frame.

## Deutsch
- Entity-Namen werden wie im Spiel ohne Rücksicht auf Groß-/Kleinschreibung gefunden: `kill`, `remove`, `affect`, `SET_LOOK_TARGET`, ein bewegtes Blickziel, das Ziel eines `fx_target_beam` und die Kamera einer `misc_portal_surface` finden jetzt „HELD“ in der Karte, wenn das Skript „held“ schreibt (vorher blieb in einer neuen Mission mit anderer Schreibweise die Figur stehen oder der Strahl aus).
- Die Kino-Animationen der Karte (`models/players/_humanoid_<karte>`) folgen dem Regelwerk: OpenJK hängt sie nur an das Skelett `_humanoid`, Movie Duels an jedes Skelett seiner Standard-Humanoiden-Liste, Jedi Outcast an keines. Ein Wechsel des Regelwerks nimmt einen alten Anhang wieder ab.
- Neuer Selbsttest `pruefmission`: vier selbst gebaute Mini-Missionen (Laufen, Animation, Signale, Stimme; Kamera; Mover und ROFF; Entity-Logik, Effekte, Tür, Kartenwechsel), je unter OpenJK und Movie Duels, einmal mit Namen wie in den Original-Missionen und einmal mit frischen Namen – beide müssen Bild für Bild dasselbe ergeben.

## 中文
- 实体名称的查找与游戏一致，不区分大小写：`kill`、`remove`、`affect`、`SET_LOOK_TARGET`、移动中的注视目标、`fx_target_beam` 的目标以及 `misc_portal_surface` 的摄像机，现在在脚本写 "held" 时也能找到地图中的 "HELD"（以前，拼写不同的新任务会让角色原地不动或光束不亮）。
- 关卡的过场动画（`models/players/_humanoid_<地图>`）遵循规则集：OpenJK 只附加到 `_humanoid` 骨架，Movie Duels 附加到其标准人形列表中的每个骨架，Jedi Outcast 不附加。切换规则集会移除旧的附加。
- 新自测 `pruefmission`：四个自建迷你任务（行走、动画、信号、语音；摄像机；移动物体与 ROFF；实体逻辑、特效、门、换图），分别在 OpenJK 和 Movie Duels 规则下运行，一次使用原版任务中的名称，一次使用新名称——两者必须逐帧一致。

## 日本語
- エンティティ名はゲームと同じく大文字小文字を区別せずに検索されます：`kill`、`remove`、`affect`、`SET_LOOK_TARGET`、移動する注視対象、`fx_target_beam` のターゲット、`misc_portal_surface` のカメラは、スクリプトが "held" と書いてもマップの "HELD" を見つけます（以前は綴りの異なる新しいミッションでキャラクターが立ったまま、またはビームが消えたままでした）。
- マップのカットシーン用アニメーション（`models/players/_humanoid_<マップ>`）はルールセットに従います：OpenJK は `_humanoid` スケルトンのみ、Movie Duels は標準ヒューマノイド一覧の各スケルトン、Jedi Outcast はなし。ルールセットを切り替えると古い付加は外れます。
- 新しいセルフテスト `pruefmission`：自作のミニミッション 4 つ（歩行・アニメーション・シグナル・音声、カメラ、ムーバーと ROFF、エンティティロジック・エフェクト・ドア・マップ切替）を OpenJK と Movie Duels の両ルールで、オリジナルミッションと同じ名前と新しい名前でそれぞれ実行し、両者がフレームごとに一致することを確認します。
