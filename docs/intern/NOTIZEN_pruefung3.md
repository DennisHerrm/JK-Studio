# Notizen pruefung3 - der Rest gegen das Spiel

Zweig `pruefung3` ab karte1 b6309500 (Integration 4), Worktree `Y:\licht`,
Build `Y:/licht/build-licht`. Auftrag: die offenen Stellen aus Pruefung 1/2
(NOTIZEN_pruefung1.md) 1:1 nach der Engine (OpenJK, Movie Duels, Jedi
Outcast), je Punkt rot/gruen.

Tests: `tests/pruefung3.cpp` (ctest `pruefung3`, Formeln der Engine mit
eigenen Daten) und der Selbsttest-Modus `pruefung3`
(`gui/selbsttest_pruefung3.inc`: je Punkt eine eigene Mission auf der
Geometrie von md_am_sith, nichts aus der Original-Mission). Rot = derselbe
Test gegen den alten Stand der geaenderten Dateien.

Engine-Quellen: OpenJK master (code/, codeJK2/; cgame-Dateien von
raw.githubusercontent.com nach `SP\p3oj`), MovieDuels-SJE-SP-Rend2 (`SP\mdgit`).

## 1. Rumpfwinkel aus dem Skelett

renderInfo.torsoAngles kommt aus dem chestBolt (cg_players.cpp:7516-7527 OJ):
G2API_GetBoltMatrix(chestBolt) -> NEGATIVE_Z -> vectoangles.
- Knochen des chestBolt je Regelwerk (G_SetG2PlayerModelInfo): Humanoid
  (G_StandardHumanoid: OJ "models/players/_humanoid*" + sechs volle Pfade, MD
  die Vorsilbenliste, JK2 nach dem Modellnamen) -> "thoracic", protocol
  "upper_lumbar"; sonst die Ausnahmen (Droiden, Rancor, Sand Creature, ATST,
  CLASS_VEHICLE/HOWLER - JK2 nach Modellnamen, galak_mech), wampa (und MD
  kotor_monster) "upper_spine", sonst "thoracic". G2API_AddBolt scheitert,
  wenn das Skelett den Knochen nicht hat -> VectorClear.
  `npcklasse.cpp: standardHumanoid, rumpfBolzenKnochen`.
- `todesanim.cpp: engineVectoangles (q_math.c:616), rumpfWinkelAusBolzen`.
- Oberflaeche: `figurPose` (dieselbe Knochenrechnung wie die Zeichnung, aus
  figurNetzTreffer herausgezogen), `figurRumpfWinkel`, Hook
  `SzenenHilfe::rumpfWinkel` im zweiten Szenenaufbau.
- Gegen die Engine-Formel: pruefung3 (vectoangles, NEGATIVE_Z, Knochen je
  Regelwerk: 46 Proben). Im Programm (md_am_sith, md_ani_ep3_robed auf
  _humanoid): stehend torsoAngles -350,3 / 91,8 bei Gier 90 (Brust nach vorn,
  10 Grad geneigt); BOTH_DEADBACKWARD1 -87,4 -> BOTH_DEATH_LYING_UP (vorher
  LYING_DN: rot); BOTH_SLEEP1 +6,9 (die Brust zeigt in den Bildern der .gla
  nach vorn) -> LYING_DN wie die Engine.
- Zeitpunkt: die Engine liest den Wert aus dem letzten gezeichneten Bild vor
  dem kill; die Vorschau rechnet die Stellung im Augenblick des Todes.
- Neu gefunden (Punkt 5): SET_ANIM einer Animation, die das Modell nicht hat
  (BOTH_DEADFORWARD1 fehlt in _humanoid/animation.cfg) - PM_SetAnimFinal
  bricht ab (animations[anim].numFrames == 0, bg_panimate.cpp OJ), die
  Vorschau setzt sie trotzdem.

## 3. Gemeinsamer Markenspeicher

Decals der Effekte (FxScheduler -> CG_ImpactMark) und Brandspuren der
Schwerter (CG_CreateSaberMarks) liegen im Spiel in EINEM Speicher
(cg_markPolys, 256, CG_AllocMark wirft die aelteste Zeitgruppe ab), CG_AddMarks
zeichnet und blendet beide.
- `efxdraw: effektMarkenIn` (die Decals eines Zeitfensters als SpurPoly,
  sofort/verzoegert), `markenBild` (ein Clientbild wie CG_DrawActiveFrame:
  sofortige Decals (CG_CheckEvents), Brandspuren (CG_AddPacketEntities),
  CG_AddMarks, verzoegerte Decals (AddScheduledEffects)). buildEffectMesh
  baut keine Decals mehr.
- `saberspuren`: SpurPoly mit eckenAlpha (CG_ImpactMark schreibt die Alpha in
  die Ecken) und Shader-Nummern ab kSpurShaderEffekt.
- `app_saber.cpp`: der Speicher spielt Brandspuren und Decals zusammen nach,
  auch ohne Schwerter; gezeichnet je Shader.
- pruefung2 (Decal als Marke) auf den neuen Weg umgestellt.

## 4. Gehen in der Hocke

SET_CROUCHED setzt SCF_CROUCHED; NPC_ApplyScriptFlags gibt in jedem Denkbild
ucmd.upmove = -127 (auch mit last_ucmd in Denkbildern ohne Verhalten, OJ),
PM_CheckDuck duckt auf crouchheight (CROUCH_MAXS_2 16, .npc "crouchheight"),
steht auf, wenn der Kasten frei ist; PM_WalkMove begrenzt wishspeed auf
ps.speed * pm_duckScale (0,5); PM_CmdScale zaehlt upmove; PM_Footsteps waehlt
BOTH_CROUCH1WALK bzw. -BACK (PMF_BACKWARDS_RUN), animSpeed mit
moveSpeedOfAnim 75.
- `navlauf`: LaufWerte::geduckt/hockHoehe, LaufZustand::geduckt (PMF_DUCKED),
  LaufTakt::geduckt, npcDucken; `ablauf` set_crouched plant neu; `scene`
  SET_CROUCHED plant neu, Laufanimation geduckt.
- rot 12 Fehler (alter Stand: 200 statt 100, BOTH_RUN1), gruen 0.
- Offen: aus dem Laufen heraus versucht PM_Footsteps eine Rolle (PM_TryRoll)
  - nicht nachgebaut.
