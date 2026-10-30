# Notizen pruefung3 - der Rest gegen das Spiel

Zweig `pruefung3` ab karte1 b6309500 (Integration 4), Worktree `Y:\licht`,
Build `Y:/licht/build-licht`. Auftrag: die offenen Stellen aus Pruefung 1/2
(NOTIZEN_pruefung1.md) 1:1 nach der Engine (OpenJK, Movie Duels, Jedi
Outcast), je Punkt rot/gruen.

Tests: `tests/pruefung3.cpp` (ctest `pruefung3`, Formeln der Engine mit
eigenen Daten, alle drei Regelwerke) und der Selbsttest-Modus `pruefung3`
(`gui/selbsttest_pruefung3.inc`: je Punkt eine eigene Mission auf der
Geometrie von md_am_sith - New mission, eigene Entities, eigenes Skript,
nichts aus der Original-Mission). Rot = derselbe Test gegen den alten Stand
der geaenderten Dateien (bzw. ohne den neuen Haken, wo das den alten Stand
nachbildet).

Engine-Quellen: OpenJK master (code/, codeJK2/; die cgame-Dateien von
raw.githubusercontent.com nach `SP\p3oj`), MovieDuels-SJE-SP-Rend2 (`SP\mdgit`).
Gegen das laufende Spiel wurde nicht verglichen (kein Spielstart): die
Pruefungen laufen gegen die portierten Formeln und gegen die echten Skelette
und Karten im Programm.

## 1. Rumpfwinkel aus dem Skelett

renderInfo.torsoAngles kommt aus dem chestBolt (cg_players.cpp:7516-7527 OJ):
G2API_GetBoltMatrix(chestBolt) -> NEGATIVE_Z -> vectoangles.
- Knochen des chestBolt je Regelwerk (G_SetG2PlayerModelInfo): Humanoid
  (G_StandardHumanoid: OJ "models/players/_humanoid*" + sechs volle Pfade, MD
  die Vorsilbenliste, JK2 nach dem Modellnamen) -> "thoracic", protocol
  "upper_lumbar"; sonst die Ausnahmen (Droiden, Rancor, Sand Creature, ATST,
  CLASS_VEHICLE/HOWLER - JK2 nach Modellnamen, galak_mech), wampa (MD auch
  kotor_monster) "upper_spine", sonst "thoracic". G2API_AddBolt scheitert,
  wenn das Skelett den Knochen nicht hat -> VectorClear.
  `npcklasse.cpp: standardHumanoid, rumpfBolzenKnochen`;
  `todesanim.cpp: engineVectoangles (q_math.c:616), rumpfWinkelAusBolzen`.
- Oberflaeche: `figurPose` (dieselbe Knochenrechnung wie die Zeichnung, aus
  figurNetzTreffer herausgezogen), `figurRumpfWinkel`, Haken
  `SzenenHilfe::rumpfWinkel` im zweiten Szenenaufbau.
- pruefung3: vectoangles, NEGATIVE_Z, Knochen je Regelwerk (46 Proben).
- Selbsttest (md_ani_ep3_robed auf _humanoid): stehend torsoAngles
  -350,3 / 91,8 bei Gier 90 (Brust nach vorn, 10 Grad geneigt);
  BOTH_DEADBACKWARD1 -87,4 -> BOTH_DEATH_LYING_UP (rot: LYING_DN);
  BOTH_SLEEP1 +6,9 (die Brust zeigt in den Bildern der .gla nach vorn) ->
  LYING_DN, wie die Formel es sagt.
- Zeitpunkt: die Engine liest den Wert aus dem letzten gezeichneten Bild vor
  dem kill; die Vorschau rechnet die Stellung im Augenblick des Todes.

## 2. Tod: Waffe, Schwert, Klaenge, DeathFX

player_die (g_combat.cpp OJ/MD/JK2) ausser der Animation, in der Reihenfolge
des Spiels (die Zuege von Q_irand/Q_flrand liegen damit wie im Spiel vor bzw.
nach denen der Animation):
- Schwert in der Hand: Q_irand(0, 1) (hitLoc HL_NONE, MOD_FALLING wirft
  immer) -> TossClientItems -> WP_SaberLose: WP_SaberLaunch (Hand frei, Strahl
  vom Ursprung frei - Haken handPunkt aus dem Bolzen "*r_hand", muzzleDir
  aus der Klinge), WP_SaberDrop (soundOff am Schwert, JK2 nach Team), das
  Schwert faellt (TR_GRAVITY, EF_BOUNCE_HALF, G_BounceMissile, je Aufprall
  WP_SaberFallSound - fallSound1..3 der .sab, Sith-Schwert sword/fallN, sonst
  saber/bounceN) und ist fort, sobald es liegt (WP_SaberUpdate gibt es frei,
  der Besitzer ist tot; dreimal je Denkbild eines NPC). Sonst bleibt es in
  der Hand, die Klinge geht aus.
- Shadowtrooper: der Kraftkristall (Drop_Item(AMMO_FORCE)).
- TossClientItems je Regelwerk (Waffenfolge OJ/JK2 bis MAX_PLAYER_WEAPONS,
  MD playerUsableWeapons[], MD ohne Abwurf auf md_ga_jedi/sith, md_gb_*,
  md_arena1-3 und mit charKOTORWeapons; Klassen ohne Abwurf) -> Drop_Item:
  forward * 150, z + 200 + Q_flrand * 50, LaunchItem dreht eine Waffe auf
  (0, Q_flrand * 180, 90); G_RunItem/G_BounceItem mit physicsBounce 0 (ein
  abgeworfener Gegenstand: liegt beim ersten Bodenkontakt). OJ/MD:
  "qboolean droppedSaber = qtrue" gilt fuer JEDEN Gegenstand - beim Aufprall
  saber/bounceN, beim Liegen PITCH 90, ROLL 0 und pitch_roll_for_slope (so im
  Spiel; JK2 nicht). Sonst G_RemoveWeaponModels (bleiben: WP_BRYAR_PISTOL, MD
  auch SBD/JAWA/Doppelpistolen/WRIST/DROIDEKA, JK2 WP_BLASTER_PISTOL).
- Klaenge: EV_DEATH1-3 -> "*deathN.wav" auf CHAN_VOICE (nicht ueber die
  Kante), Sturz: "*falling1.wav" (NPCAI_DIE_ON_IMPACT, ps.friction 1) bzw.
  fallsplat auf CHAN_BODY, Kante: "*falling1.wav" (friction 0), Mark1
  death1/2, Galak/Schluessel EV_DEATH; DeathFX je Klasse (OJ/JK2/MD mit den
  MD-Klassen und NPC_type cultist_destroyer) mit Effekten und Klaengen. Die
  eigenen Klaenge loest die Vorschau wie CG_RegisterCustomSounds auf
  (sound/chars/<snd>/misc/, g_sex "f": erst _f, fehlend Variante 1).
- Orte: Actor::hoehe kommt erst nach buildScene (schwerkraftEinrechnen) -
  `todesFolgenHoehe` setzt DeathFX und Abwurf danach auf den Ort, an dem die
  Figur stirbt, und rechnet die Bahnen neu.
- `bhed/abwurf.h` (neu), `todesanim` (waffeVorher/-Nachher, deathFx,
  tossClientItems, EngineZufall::flrand), `scene` (TodesFolge, TodesAbwurf,
  todesFolgenAnlegen), Oberflaeche: Klaenge (app_ton), DeathFX als Effekte,
  Abwurf gezeichnet (Griff/Waffenmodell, G2_GenerateWorldMatrix).
- pruefung3: Zugfolge (Drop_Item, PM_PickAnim, EV_DEATH), Bahn gegen
  EvaluateTrajectory, Winkel nach dem Liegen, Schwert mit Abprallern, beide
  Wuerfe des Schwerts, md_ga_jedi, DeathFX R2D2 - je Regelwerk.
- Selbsttest p3tod: die Waffe geht aus der Hand und liegt (gezeichnet),
  DeathFX des R2D2, Todesklaenge. Rot nicht eigens gebaut (neue
  Schnittstelle); der alte Stand hatte nichts davon (Pruefung 2: "Nicht
  nachgebaut: Waffe fallen lassen, Schwert verlieren, Todesklaenge, DeathFX").
- Offen: Leichenphysik nach dem Wurf ueber die Kante (PM_DEAD: PM_DeadMove,
  ps.friction 0/1, PMF_TIME_KNOCKBACK, die Kiste aus DeadThink) - braucht den
  Totenlauf im Wegplaner und die Hoehe aus der Schwerkraft; in dieser Runde
  nicht gebaut. NPC_RemoveBody (Leiche nach g_corpseRemovalTime 10 s fort,
  Feinde nur ausser Sicht des Spielers, nicht mit Schluessel oder laufendem
  Skript) - braucht Blick und Sichtlinie des Spielers und den Stand der
  Skripte je Entity; nicht gebaut. Ein Schwert mit ausgeschalteter Klinge
  wuerde WP_SaberLaunch ohne WP_SaberDrop fliegen lassen (zurueck zur
  Leiche) - nicht nachgebaut, selten.

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
- pruefung3: Reihenfolge im Bild, 250 Brandspuren + Decal (die Zeitgruppe
  faellt), Ausblenden, delay -> verzoegert; pruefung2 (Decal als Marke) auf
  den neuen Weg umgestellt.
- Selbsttest p3marke: ein eigener Effekt (pk3 im Laufordner) auf einer
  ebenen Flaeche - 21 Marken im Speicher und vom Markenzeichner gezeichnet.
  Rot nicht eigens gebaut: Test und Code brauchen die neue Schnittstelle; im
  alten Stand zeichnete das Effektnetz das Decal, der Markenspeicher kannte
  es nicht (Pruefung 2: "Offen: der gemeinsame Markenspeicher").

## 4. Gehen in der Hocke, Beine am Hang

SET_CROUCHED setzt SCF_CROUCHED; NPC_ApplyScriptFlags gibt in jedem Denkbild
ucmd.upmove = -127 (auch mit last_ucmd in Denkbildern ohne Verhalten, OJ),
PM_CheckDuck duckt auf crouchheight (CROUCH_MAXS_2 16, .npc "crouchheight"),
steht auf, wenn der Kasten frei ist; PM_WalkMove begrenzt wishspeed auf
ps.speed * pm_duckScale (0,5); PM_CmdScale zaehlt upmove; PM_Footsteps waehlt
BOTH_CROUCH1WALK bzw. -BACK (PMF_BACKWARDS_RUN), animSpeed mit
moveSpeedOfAnim 75.
- rot 12 Fehler (alter Stand: 200 statt 100, BOTH_RUN1), gruen 0.
- Offen: aus dem Laufen heraus versucht PM_Footsteps eine Rolle (PM_TryRoll)
  - nicht nachgebaut.

PM_AdjustStandAnimForSlope: aus den Fussbolzen ("*l_leg_foot"/"*r_leg_foot",
ATST "*l_foot"/"*r_foot") PM_FootSlopeTrace (Kasten +-3 bzw. +-16, interval 4
bzw. 10), dann die Stufe LEFTUP/RIGHTUP 1..5, die Familie nach der jetzigen
Beinanimation (S1, allgemein, S3..S7), eine Stufe je SLOPE_RECALC_INT (100 ms)
in der Reihenfolge der Animationsnummern; nur die Beine (SETANIM_LEGS,
SETANIM_FLAG_NORMAL), wo PM_Footsteps die Standanimation setzt.
- `scene: hangBeinSchritt, hangStandErlaubt, hangBeineRechnen,
  Actor::hangBeine`; Haken `SzenenHilfe::fussPunkte` (figurFussPunkte).
  Steht die Figur still auf einer Stelle mit ebenem Boden unter dem ganzen
  Grundriss, rechnet die Vorschau die Fuesse dort nicht (diff ist 0).
- pruefung3: Schritte 4-7 gegen die Tabelle (17 Faelle), Szene auf einer Stufe
  (LUP1, nach 150 ms LUP2, Oberkoerper BOTH_STAND1), ebener Boden ohne. Rot
  (ohne Fussbolzen = alter Stand): keine Hanganimation.
- Selbsttest: Fussbolzen des echten Skeletts an den Fuessen, auf ebenem Boden
  BOTH_STAND1.

## 5. Die uebrigen Stellen aus NOTIZEN_pruefung1.md

- Neu gefunden und behoben: SET_ANIM einer Animation, die das Modell nicht hat
  (BOTH_DEADFORWARD1 fehlt in _humanoid/animation.cfg) - Q3_SetAnimLower/Upper
  fragen PM_HasAnimation und setzen nichts. pruefung3 rot (alter Stand
  BOTH_DEADFORWARD1) / gruen.
- Nicht in dieser Runde (je Grund): useBBox im Effektpfad (Kastenspur der
  KollisionsKarte in efx::sim); Geschuetze mit Feind und Schaden von
  fx_target_beam (die Vorschau hat kein Schadensmodell ausser kill/
  trigger_hurt); liegende Schwerter mit NPC_type (CG_Item); misc_sentry_turret
  (pas_think); Blinzeln/Leerlaufmimik; trigger_hurt gegen Spieler/Ruestung;
  forced_rgb_saber_colours; Fahrzeuge ohne Schwerkraft; Holster/tag_hack/
  Zuendklang bei SET_SABER1/2/Muendung "*flash"; Remove vor dem ersten Add;
  PM_FallToDeath (braucht wie die Leichenphysik den Tod aus dem Wegplaner);
  zweites use waehrend der Bruchverzoegerung; misc_model_breakable target4;
  CAMERA_ACCEL und ZOOM gleichzeitig (gelesen: ZOOM ueberschreibt FOV_time,
  FOV2, FOV_duration, die Beschleunigung laeuft damit vom Zoomziel weiter und
  springt am Ende auf FOV2 - nicht gebaut); wait("name") in clock.cpp gehoert
  zur Zeitleiste (anderer Zweig).

## Schlussrunde

- Sauberer Neubau (`--clean-first`, unter Last): 0 Warnungen, 0 Fehler.
- ctest: 178/178 gruen (darin neu `pruefung3`, geaendert `pruefung2` -
  Decal-Teil auf den Markenspeicher umgestellt).
- lint_sources, lint_fassung, lint_headers: ok.
- Selbsttests (Laufordner `Y:\p3_laeufe`, Idle-Prioritaet): pruefung3 9 OK /
  0 FEHLER, szenen 26/0, kamera 95/0, missionsbau 95/0, umriss 45/0, mission1
  98/0 (mit BHED_FOTOS - ohne Fotoordner schlagen die acht Bildproben der
  Gizmos fehl, weil sie die Fotos auswerten), karteneditor kurz
  (BHED_KARTE_NUR=fotos) 78/0. Nach dem Neubau nur noch todesanim.cpp ohne
  leere Zweige geaendert: inkrementell gebaut (0 Warnungen), ctest 178/178,
  pruefung3 9/0 und mission1 98/0 erneut.
- Handbuch: keine Bedienung geaendert (nur Verhalten wie im Spiel), keine neuen
  Programmtexte (kein tools/i18n_pruefung3.py noetig).
- Nicht gegen das laufende Spiel geprueft: kein Spielstart in dieser Runde;
  geprueft wurde gegen die portierten Formeln (pruefung3) und mit den echten
  Skeletten, Effekten und Karten im Programm (Selbsttest).

## Pruefung 4a (angehalten 30.10. nachmittags, wird fortgesetzt)

Auftrag SPuftrag_pruefung4.md, Liste 4a. Angehalten auf Bitte des
Koordinators (hoechstens 4 Agenten). Kein Spiel gestartet, keine Sperrdatei.

Stand:
- Punkt 8 (mission1 ohne BHED_FOTOS 8 FEHLER): Ursache im Test - die acht
  Gizmo-Proben (je Durchgang vier) werten Fotos aus, die `foto()` nur mit
  BHED_FOTOS schreibt; ohne lasen sie kein Bild (Werte -1). Rot nachgestellt
  (Lauf ohne BHED_FOTOS: die vier Gizmo-/Abspiel-Proben FEHLER). Behoben im
  Test: `mission1::probeFoto` schreibt immer, ohne BHED_FOTOS nach
  `<Programmordner>\m1_fotos`, `FotoBild::lesen` liest dort. Gruen: mission1
  ohne BHED_FOTOS 98 OK, 0 FEHLER (30.10., Lauf m1ohne).
- Punkt 3 (Schwert mit ausgeschalteter Klinge) fertig (c1112bc2):
  TossClientItems -> WP_SaberLose wirft jetzt auch mit ausgeschalteter Klinge
  (WP_SaberLaunch immer, WP_SaberDrop nur mit brennender). AbwurfBahn ist eine
  trajectory_t je Serverbild (cgame wertet s.pos/s.apos zur Zeit cg.time aus).
  WP_SaberThrow fuer den toten Besitzer: nach WP_SaberDrop wartet das Schwert,
  solange es hoechstens 16 von handRPoint ist (WP_SaberCatch faengt nicht) -
  bleibt die Hand, haengt es (cgame zeichnet TR_GRAVITY weiter, so im Spiel);
  Klinge aus: im naechsten Denkbild zielt WP_RunSaber es einmal (saberEntityDist
  aus FP_SABERTHROW der .npc, JK2 FORCE_LEVEL_1; Heim = Hand unter 100, sonst
  eyePoint aus dem headBolt, npcklasse kopfBolzen; Stufe 3 WP_SaberFindEnemy
  mit G_ClearLOS/InFront/inPVS/WP_SaberRateEnemy, nur nach kill - trigger_hurt
  setzt self->enemy), dann WP_SaberDrop mit soundOff. JK2: liegendes Schwert
  erst 5 s nach WP_SaberDrop fort. apos nach WP_SaberDrop ab currentAngles
  (0, G_Spawn). Die Bahn entsteht am Ende von buildScene und nach der
  Schwerkraft neu (todesFolgenHoehe mit Hand/Auge aus figurHakenSetzen).
  - Rot (alte Regel schwertFaellt = klingeAn): pruefung3 6 Fehler ("Klinge aus,
    FP_SABERTHROW 0/2/1"); gruen 153 OK, 0 Fehler; ctest 178/178.
  - Abweichungen: Stufen 4/5 lesen in OpenJK saberThrowDist[]/
    saberThrowDistSquared[] ueber das Tabellenende - hier wie Movie Duels
    (FP_TableLevel) Stufe 3. fwdangles[PITCH/ROLL] in WP_SaberFindEnemy sind im
    Spiel unbelegt (Stapelinhalt) - hier 0. SET_PLAYER_TEAM kennt die Szene
    nicht (Team aus der .npc). maxs[2] des Feindes geduckt nach der
    Animation BOTH_CROUCH*.
- Punkt 1/2/4/5/6/7: nur gelesen (Engine-Stellen, siehe unten), nichts gebaut.
  - Leiche: CorpsePhysics je NPC_Think (erstes Pmove nach dem Tod 1 ms wegen
    serverTime = level.time - 1000, dann 50), PM_DEAD: kein PM_CheckDuck,
    PM_DeadMove (-20 je Bild am Boden), PM_Friction mit ps.friction (6, 0 nach
    Kantenwurf/Wurf, 1 nach PM_FallToDeath; PMF_TIME_KNOCKBACK ohne Reibung),
    clipmask ohne MONSTERCLIP/BOTCLIP, DeadThink-Kasten maxs[2] = eyePoint -
    origin + 4 (>= -8; JK2 waechst x/y bis 32), pitch_roll_for_slope am Boden
    (Leiche wird geneigt gezeichnet, mins[2]), PM_CrashLand in Todesanimation:
    fallsplat/bodyfall_humanN, FALLDEATH1 -> FALLDEATH1LAND, NPCAI_DIE_ON_IMPACT
    -> G_Damage 1000 -> player_die-Zucken. Wurf-Zweige der Todeskette setzen
    ps.friction = 0 (fehlt in todesanim). Plan: Totenlauf als planeLauf-Modus
    nach schwerkraftEinrechnen (Z aus hoehe), WalkTo-Stueck + hoehe ab Tod.
  - PM_FallToDeath in PM_GroundTraceMissed (Vorhersage 400 voraus, >= 128,
    > 256 freier Fall bzw. Schaden >= health).
  - NPC_RemoveBody: BodyRemovalPadTime (g_corpseRemovalTime 10, Droiden 0, MD
    md_ga/gb SBD/BATTLEDROID/DROIDEKA 3000, JK2 fest 10000), DeadThink ->
    EF_NODRAW fuer Droiden (MD-Liste groesser), G_OkayToRemoveCorpse (message,
    ICARUS laeuft), TEAM_ENEMY/PROTOCOL: Spieler < 128 oder
    InFOVFromPlayerView(110, 90, cg.refdef = Kamera) + NPC_ClearLOS, frei nur
    mit self->enemy (trigger_hurt setzt enemy = Ausloeser, kill nicht).
  - PM_TryRoll (bg_pmove.cpp:3782 OJ) aus PM_Footsteps bei PMF_DUCKED und
    PM_RunningAnim; G_StartRoll, waehrend der Rolle PM_CmdForRoll mit speed 400
    und G_CheckRollSafety.
  - Punkt 5/6: Stellen in ausruestung.cpp:18-46 und app_view3d.cpp (Blinzeln).

Naechster Schritt: Patch wieder einspielen (`git apply`), Punkt 3 fertig
(Szene, Haken, Tests rot/gruen), dann Punkt 1, 2, 4, 5, 6, 7, mission1 gruen,
Spielfotos, Schlussrunde. Laufskript SP\p4a\lauf.sh (Laufordner /y/p4a_laeufe).

## English
- Lying figures die like in the game: the torso angle for the death animation now comes from the chest bone of the drawn skeleton (per rule set: OpenJK, Movie Duels, Jedi Outcast) - a figure lying on its back dies with BOTH_DEATH_LYING_UP instead of always BOTH_DEATH_LYING_DN.
- When a figure dies, the preview now does what the game does besides the animation: the weapon leaves the hand and falls to the ground as an item (or just disappears, depending on weapon and rule set), a saber may drop out of the hand and vanish when it lands, the death cries, falling screams and splats play with the figure's own sounds, and droids explode with their death effects and sounds.
- Effect decals and saber burn marks now share one mark store with 256 places like in the game: when it is full the oldest marks of one moment go together, and both fade out in their last second.
- A crouched figure (SET_CROUCHED) now walks crouched at half speed with BOTH_CROUCH1WALK (backwards BOTH_CROUCH1WALKBACK) and stands up again when there is room.
- On a slope, a standing figure puts its feet like in the game (the slope leg animations, one step every 100 ms).
- SET_ANIM with an animation the model does not have no longer changes anything, like in the game.

## Deutsch
- Liegende Figuren sterben wie im Spiel: der Rumpfwinkel fuer die Todesanimation kommt jetzt aus dem Brustknochen des gezeichneten Skeletts (je Regelwerk: OpenJK, Movie Duels, Jedi Outcast) - eine Figur auf dem Ruecken stirbt mit BOTH_DEATH_LYING_UP statt immer mit BOTH_DEATH_LYING_DN.
- Stirbt eine Figur, tut die Vorschau jetzt, was das Spiel ausser der Animation tut: die Waffe geht aus der Hand und faellt als Gegenstand zu Boden (oder verschwindet nur, je nach Waffe und Regelwerk), ein Schwert kann aus der Hand fallen und ist fort, sobald es liegt, Todesschreie, Sturzschreie und Aufprall klingen mit den eigenen Klaengen der Figur, und Droiden explodieren mit ihren Todeseffekten und Klaengen.
- Decals aus Effekten und Brandspuren der Schwerter teilen sich jetzt wie im Spiel einen Markenspeicher mit 256 Plaetzen: ist er voll, gehen die aeltesten Marken eines Augenblicks zusammen, und beide blenden in ihrer letzten Sekunde aus.
- Eine geduckte Figur (SET_CROUCHED) geht jetzt geduckt mit halbem Tempo und BOTH_CROUCH1WALK (rueckwaerts BOTH_CROUCH1WALKBACK) und steht wieder auf, wenn Platz ist.
- Am Hang stellt eine stehende Figur die Fuesse wie im Spiel (die Hang-Beinanimationen, eine Stufe je 100 ms).
- SET_ANIM mit einer Animation, die das Modell nicht hat, aendert wie im Spiel nichts mehr.

## 中文
- 躺着的角色死亡时与游戏一致：死亡动画所用的躯干角度现在取自所绘骨架的胸骨（按规则集：OpenJK、Movie Duels、Jedi Outcast）——仰躺的角色以 BOTH_DEATH_LYING_UP 死亡，而不再总是 BOTH_DEATH_LYING_DN。
- 角色死亡时，预览现在会执行游戏在动画之外所做的事：武器离手并作为物品掉落到地上（或根据武器和规则集直接消失），光剑可能脱手并在落地后消失，死亡惨叫、坠落尖叫和落地声使用角色自己的声音播放，机器人会以其死亡特效和声音爆炸。
- 特效贴花和光剑灼痕现在与游戏一样共用一个 256 个位置的标记存储：存满时同一时刻最旧的标记一起消失，两者都在最后一秒淡出。
- 蹲下的角色（SET_CROUCHED）现在以一半速度蹲行，使用 BOTH_CROUCH1WALK（后退为 BOTH_CROUCH1WALKBACK），有空间时会重新站起。
- 在斜坡上，站立的角色会像游戏中一样摆放双脚（斜坡腿部动画，每 100 毫秒一级）。
- 对模型没有的动画使用 SET_ANIM 时，与游戏一样不再产生任何变化。

## 日本語
- 横たわったキャラクターがゲームと同じように死ぬようになりました：死亡アニメーションの胴体角度は、描画されたスケルトンの胸の骨から取るようになりました（ルールセットごと：OpenJK、Movie Duels、Jedi Outcast）。仰向けのキャラクターは、常に BOTH_DEATH_LYING_DN ではなく BOTH_DEATH_LYING_UP で死にます。
- キャラクターが死ぬと、プレビューはアニメーション以外にゲームが行うことも行うようになりました：武器が手から離れてアイテムとして地面に落ち（武器とルールセットによっては消えるだけ）、ライトセーバーは手から落ちて着地すると消えることがあり、断末魔、落下の叫び、着地音がキャラクター自身の音で鳴り、ドロイドは死亡エフェクトと音とともに爆発します。
- エフェクトのデカールとライトセーバーの焦げ跡は、ゲームと同じく 256 か所のマーク領域を共有するようになりました：いっぱいになると同じ瞬間の最も古いマークがまとめて消え、どちらも最後の 1 秒でフェードアウトします。
- しゃがんだキャラクター（SET_CROUCHED）は、半分の速度で BOTH_CROUCH1WALK（後退は BOTH_CROUCH1WALKBACK）を使ってしゃがみ歩きし、空間があれば立ち上がるようになりました。
- 斜面では、立っているキャラクターがゲームと同じように足を置きます（斜面用の脚アニメーション、100 ミリ秒ごとに 1 段階）。
- モデルにないアニメーションを SET_ANIM で指定しても、ゲームと同じく何も変わらなくなりました。
