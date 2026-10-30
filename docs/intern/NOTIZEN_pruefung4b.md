# Notizen pruefung4b - Entities, Effekte, Kamera gegen das Spiel

Zweig `pruefung4b` ab pruefung3 1b7b4bcd, Worktree `Y:\sprache_es`, Build
`Y:/sprache_es/build-sprache`. Auftrag: Liste 4b aus `auftrag_pruefung4.md`
(useBBox, Geschuetze/Schaden/trigger_hurt, liegende Schwerter, Fahrzeuge,
Remove/Bruch/target4, CAMERA_ACCEL+ZOOM), je Punkt rot/gruen, Abgleich mit dem
laufenden Spiel.

Tests: `tests/pruefung4b.cpp` (ctest `pruefung4b`, eigene Abschrift der
Engine-Formeln gegen das Programm). Rotlaeufe: die alte Logik in die
aktuellen .cpp gesetzt (Skripte im Scratchpad `p4b_rot*.py`), nur
pruefung4b gebaut, danach zurueck.

## Stand je Punkt

- 6. CAMERA_ACCEL und ZOOM (WIP-Commit 7d7ebc4d): `KameraFov` wie
  client_camera. pruefung4b Faelle a-e rot mit camtrack.cpp/h von 1b7b4bcd,
  f (ZOOM allein) war schon richtig; gruen.
- 3b. forced_rgb_saber_colours: in Movie Duels unerreichbar (die Zweige
  "saberColor"/"saber2Color" vergleichen 10/11 Zeichen und fangen die
  RGB-Schreibweise ab - Laenge passt nicht, "WARNING: bad saberColor",
  nichts gesetzt, NPC_stats.cpp:4215-4305 MD), OpenJK kennt sie nicht.
  scene.cpp: RGB aendert nichts; Jedi Outcast nur "saberColor". Rot mit
  scene.cpp von 1b7b4bcd, gruen.
- 5b. Zweites use in der Bruchverzoegerung (5121c79b): die Sollwerte des
  Vorgaengers waren falsch, nicht die Umsetzung - ein wait endet im ersten
  Bild NACH stempel + dauer (TaskManager.cpp:1141 "<"), "wait 1000" ab 0
  also bei 1050; der Bruch damit bei 3100 bzw. 1 s nach dem LETZTEN use.
  Dazu gefunden und gebaut: funcBBrushDieGo und G_FreeEntity sind thinks
  und laufen in G_RunFrame am Platz der Entity (G_RunThink vor ihrem
  ICARUS, g_main.cpp:1062-1088) - Nachbau: Geplant::Art::Bruch/Frei in
  Schritt 3 (`denke`), nicht mehr vor allen Entities; ein use vor dem Platz
  des func_breakable im Bild von G_FreeEntity bricht neu, eines danach
  findet nichts. Ablauf::Benutzung::reihe und MoverUse::reihe tragen den
  Platz zu MoverSim (dieselbe Entscheidung). pruefung4b 5b (8 Pruefungen)
  und ablauftest #12: rot mit der alten Logik (6 + 2 Fehler), gruen.
- 5a. Remove vor dem ersten Add (36019ed0): Ghoul2-Modell 0 (die Figur) ist
  fort - Koerper, Waffe und Griffe (Kinder in G2_Sort_Models) werden weder
  gezeichnet noch vom G2-Netztest getroffen (`ActorDraw::ohneKoerper`);
  ein Add danach haengt sich nicht an ("Empty Bolt List") und steht mit den
  Achsen des refEntity am Ursprung (`AnbauDraw::anWurzel`, `anbauWelt`).
  pruefung4b 5a gegen eine Abschrift des ghoul2-Felds (Init/Attach/Remove/
  Sort): rot mit dem alten ausruestung.cpp, gruen.
  - Nicht nachbildbar: haelt die Figur eine Waffe und kommt nach dem Remove
    ein Add, liegt das neue Modell in Fach 0, und die Waffe haengt wieder an
    einem gueltigen Modell - G2_GetBoltMatrixLow liest dann dessen LEERE
    Bolzenliste ohne Pruefung (tr_ghoul2.cpp:2165): undefiniert, im Spiel
    wohl ein Absturz. Die Vorschau zeichnet die Waffe dann nicht.
- 1. useBBox im Effektpfad: der Spurtest ist jetzt der von
  CParticle::UpdateOrigin - CG_PointContents am neuen Ort (Welt + Brush-
  Entities an s.origin; `kollisionsInhalt` = CM_PointContents), Spur nur,
  wenn dort MASK_SHOT|CONTENTS_WATER (ausser expensivePhysics, Vorgabe
  fx_expensivePhysics 1), CG_Trace mit mMin/mMax (useBBox) gegen
  Kollisionskarte, Brush-Entities am Stand der Mover-Simulation und Figuren
  (CM_TempBoxModel, `kollisionsSpurKasten`), startsolid/allsolid = kein
  Treffer, die Beschleunigung fuer den Weg bis zur Wand (VectorMA mit
  trace.fraction), SURF_NOIMPACT ohne impactFx. Die Schrittweite ist jetzt
  die Bildzeit der Vorgabe com_maxfps 125 (8 ms statt 2 ms) - davon haengt
  ab, ob ein Kasten beim Aufsetzen schon im Boden steckt (dann startsolid,
  er faellt durch; ohne expensivePhysics passiert das mit useBBox oft).
  `EffektSpurWelt`/`setzeEffektSpurWelt` (efxdraw.h), gesetzt von
  `effektSpurWeltSetzen` (app_bewegung.cpp) nach jeder Mover-Simulation.
  pruefung4b 1: efx::sim gegen eine Abschrift (Punkt, Kasten ohne/mit
  expensivePhysics) - rot mit 2 ms und ohne die Beschleunigung, gruen.
  efxtest: Boden-Probe mit Welt (Punkt liegt, Kasten faellt durch, mit
  expensivePhysics liegt er 5 hoeher), Effektnetz-Probe mit Strahl-Welt.

- 3a. weapon_saber mit NPC_type (e9622c17): FinishSpawningItem nimmt das
  Modell aus WP_SaberParseParms(NPC_type) - "player" ueber g_saber
  ("single_1"), unbekannt/ohne saberModel die Vorgabe saber_1.glm; Jedi
  Outcast nicht (`gegenstandAufstellen(..., sabers, jediOutcast)`). Das
  Leuchten aus CG_Item: dasselbe Modell mit gfx/effects/solidWhite_cull und
  RF_RGB_TINT (neu `FigurFarbe::Entity`, `FigurLicht::entityFarbe`,
  `texturenMitShader`), 14 Sprites gfx/effects/yellow_glow (Groesse 10, Alpha
  wv*0.5, FX_USE_ALPHA) entlang -ent.axis[2], ein Licht (1,1,0) mit
  wv*350+180 (OpenJK) bzw. wv*100 (Movie Duels); wv = sin(cg.time*0.002) *
  0.08 + 0.2 (cg.time = Zeitleiste). ITMSF_NOGLOW (64) ohne. pruefung4b 3a
  (Modell je NPC_type): rot mit dem alten "nicht nachgebildet", gruen.
- 5c. misc_model_breakable mit gravity/throwtarget: `objektAbsetzen`
  (misc_model_breakable_gravity_init mit dropToFloor, dann das erste
  G_RunObject - auf ebenem Boden werden Nicken/Rollen der Karte 0,
  pitch_roll_for_slope) und `wirfObjekt` (misc_model_throw_at_target4 bei
  jedem use: trDelta += kvel, z mindestens knockback; G_RunObject je 50 ms,
  G_BounceObject mit EF_BOUNCE_HALF und 60/mass, auf begehbarem Boden
  G_StopObjectMoving; kick_impact_silent + objectHit/objectHitHeavy).
  MoverSim::wuerfe, App::MapModel::wurf (gezeichnet), `objektWuerfeSetzen`.
  pruefung4b 5c gegen eine Abschrift: rot (alter Stand: weder abgesetzt noch
  geworfen), gruen.
  - Offen (gehoert zum Schadensmodell, Punkt 2): ein Treffer auf eine Figur
    (kick_impact, DoImpact mit G_Damage auf beide) - die Wege-Spur kennt die
    Figuren nicht; DoImpact gegen das Objekt selbst (health). Hinweis: in
    DoImpact steht `self->client->NPC_class` im zweiten Teil der Bedingung -
    fuer ein Objekt mit health und schwachem Aufprall liest die Engine dort
    einen Nullzeiger (Absturz), also nicht nachbildbar.
  - Ein use im selben Bild wie ein think des Objekts: die Reihenfolge der
    Entities entscheidet (hier: think vor dem Wurf).
- 4. Fahrzeuge: `FahrzeugDaten` aus der .veh (type, gravity, hoverHeight,
  hoverStrength, maxSlope, friction; app_view3d_spiel.inc), Actor::fahrzeug.
  Im Wegplaner fallen Fahrzeuge weiter nicht (ohneSchwerkraft), aber in
  schwerkraft.cpp: Laeufer/Tiere/Jaeger wie NPCs mit ps.gravity (Vehicle_
  Initialize: SVF_CUSTOM_GRAVITY bei gravity != g_gravity, Jaeger ohne Pilot
  nicht), Gleiter mit `schwebeSchritt` (PM_GroundTrace, PM_HoverTrace,
  PM_WalkMove |v| nach oben bzw. PM_AirMove mit Fahrzeugreibung, Schnitt an
  der Schwebespur, Schwerkraft; bewegt: beide Spuren noch einmal).
  pruefung4b 4 gegen eine Abschrift: rot (alter Stand: stehen in der Luft),
  gruen. Wasser (bouyancy) und Hangnormalen (das Bodenraster kennt keine)
  sind nicht dabei.
- 2 (Teil). trigger_hurt gegen den Spieler: der Spieler nimmt jetzt Schaden
  wie die NPCs (health 100, armor 0, playerMins +-15, beruehrt vor allen
  NPCs); stirbt er, Nachbau: missionStatusDeadTime = t + 1000, danach haelt
  ClientThink die Skripte an (ausser die Kamera laeuft). pruefung4b 2: rot,
  gruen.
- 2 (Teil). Schaden von fx_target_beam: jeder Schuss (zielStrahlFeuer,
  laserfalleSpurBei ohne Ghoul2) mit "damage" > 0, der eine Figur trifft
  (LaserfalleSpur::figurNr), geht als App::StrahlTreffer in den naechsten
  Durchgang von hurtTodeBerechnen: G_Damage mit CheckArmor, FL_GODMODE/
  FL_UNDYING wie bei trigger_hurt - die Figur stirbt (kill, deathscript), der
  Spieler wie oben. Gebaut; ohne gezielten Test (der braucht den Selbsttest
  `pruefung4b`, offen).

## Tests (Stand d5ac0221)

- Voller Bau (inkrementell, unter Last): 0 Warnungen, 0 Fehler. ctest 179/179
  gruen (pruefung4b, ablauftest, efxtest_probe, mover_probe, camtracktest
  einzeln vorher).
- Selbsttests (Laufordner im Scratchpad `p4b_laeufe`, danach geloescht):
  pruefung3 9 OK / 0 FEHLER, kamera 95/0, szenen 26/0. Nicht gelaufen:
  missionsbau, umriss, mission1, karteneditor kurz (Schlussrunde offen).
- Nicht gegen das laufende Spiel geprueft: kein Spielstart in dieser Runde
  (Geschuetz/Sentry, liegendes Schwert, ACCEL+ZOOM offen); geprueft gegen die
  portierten Formeln (pruefung4b) und die Engine-Quellen (OpenJK, Movie Duels,
  Jedi Outcast).

## Offen (Reihenfolge)

2 (Rest): Geschuetze mit Feind (pas_think mit pas_find_enemies/Zielen/pas_fire,
misc_turret turret_base_think/turret_head_think/turret_fire, misc_turretG2 MD)
samt Geschossen und deren Schaden, DoImpact (5c-Treffer auf Figuren); jeweils
in hurtTodeBerechnen (app_view3d.cpp) als weitere Quelle wie die
Strahltreffer. Treffer auf func_breakable/misc_model_breakable (health) durch
Schaden: MoverSim braucht dafuer Bruch-Ereignisse aus dem Schadensmodell.
Fundstellen (code/game/g_turret.cpp OpenJK): turret_fire :165, turret_head_think
:236, turret_find_enemies :438, turret_base_think :553, pas_fire :1414,
pas_adjust_enemy :1565, pas_think :1624 (bisher nur Aufklappen und Schwenk
ohne Feind: pasAufklappenMs/pasSchwenkLauf in mover.cpp, geschuetzKnochen in
app_view3d_spiel.inc).
Spielabgleich (Geschuetz/Sentry, liegendes Schwert, ACCEL+ZOOM), Selbsttest
`pruefung4b`, Handbuch/i18n, Schlussrunde.

## English

- Camera: a ZOOM during a running fovaccel keeps accelerating from the zoom target and jumps at the end, exactly like the game's client camera (FOV, FOV2, FOV_time, FOV_duration).
- Saber colours: saberColorRGB/saber2ColorRGB lines in NPC files no longer change a blade (the game never reaches them); Jedi Outcast only knows saberColor.
- func_breakable: every use until the brush is freed runs again (use script, break rescheduled); a second use during the delay moves the break to one delay after the last use, and the break and the removal happen at the breakable's own slot in the server frame.
- Remove before the first Add (SET_REMOVE*HANDBOLT_MODEL) removes the figure itself like Ghoul2 model 0 in the game; models added afterwards stand unattached at the figure's origin.
- Effects: particles with physics trace like the game - point contents first (except expensivePhysics), box trace with useBBox against the collision map, moving brush entities and figures, at the game's default frame time.
- Lightsabers lying on the floor (weapon_saber) use the hilt named by NPC_type and glow like in the game (yellow shell, sprites and light).
- misc_model_breakable with gravity settles on the floor and is thrown at its throwtarget when used, bouncing off walls like in the game.
- Vehicles fall: walkers, animals and empty fighters with gravity, speeders hover down with the game's hover trace.
- trigger_hurt also hurts the player; if the player dies, all scripts stop one second later (unless the camera is running).
- fx_target_beam with "damage" hurts and kills the figures its beam hits.

## Deutsch

- Kamera: ein ZOOM waehrend eines laufenden fovaccel beschleunigt vom Zoomziel weiter und springt am Ende - genau wie die Kamera des Spiels (FOV, FOV2, FOV_time, FOV_duration).
- Schwertfarben: saberColorRGB/saber2ColorRGB in NPC-Dateien aendern keine Klinge mehr (das Spiel erreicht sie nie); Jedi Outcast kennt nur saberColor.
- func_breakable: jedes use, bis der Brush freigegeben ist, laeuft erneut (use-Skript, Bruch neu geplant); ein zweites use in der Verzoegerung schiebt den Bruch auf eine Verzoegerung nach dem letzten use, und Bruch und Entfernen geschehen am Platz des func_breakable im Serverbild.
- Remove vor dem ersten Add (SET_REMOVE*HANDBOLT_MODEL) entfernt wie im Spiel die Figur selbst (Ghoul2-Modell 0); danach angehaengte Modelle stehen frei am Ursprung der Figur.
- Effekte: Teilchen mit Physik spuren wie im Spiel - erst der Inhalt am neuen Ort (ausser expensivePhysics), mit useBBox ein Kasten gegen Kollisionskarte, bewegte Brush-Entities und Figuren, in der Bildzeit der Spielvorgabe.
- Am Boden liegende Lichtschwerter (weapon_saber) nehmen den Griff aus NPC_type und leuchten wie im Spiel (gelbe Huelle, Sprites und Licht).
- misc_model_breakable mit gravity legt sich auf den Boden und wirft sich bei use auf sein throwtarget, prallt wie im Spiel an Waenden ab.
- Fahrzeuge fallen: Laeufer, Tiere und leere Jaeger mit der Schwerkraft, Gleiter sinken mit der Schwebespur des Spiels.
- trigger_hurt trifft auch den Spieler; stirbt er, halten alle Skripte eine Sekunde spaeter an (ausser die Kamera laeuft).
- fx_target_beam mit "damage" verletzt und toetet die Figuren, die sein Strahl trifft.

## 中文

- 摄像机：在 fovaccel 进行中执行 ZOOM 时，会从缩放目标继续加速并在结束时跳变，与游戏的客户端摄像机完全一致（FOV、FOV2、FOV_time、FOV_duration）。
- 光剑颜色：NPC 文件中的 saberColorRGB/saber2ColorRGB 不再改变光刃（游戏从不执行到它们）；Jedi Outcast 只识别 saberColor。
- func_breakable：在画刷被释放之前，每次 use 都会重新执行（use 脚本、重新安排破碎）；延迟期间的第二次 use 会把破碎推迟到最后一次 use 之后的一个延迟，破碎和移除发生在服务器帧中该实体自己的位置。
- 在第一次 Add 之前执行 Remove（SET_REMOVE*HANDBOLT_MODEL）会像游戏中一样移除角色本身（Ghoul2 模型 0）；之后添加的模型不附着，立在角色原点。
- 特效：带物理的粒子像游戏中一样检测碰撞——先检查新位置的内容（expensivePhysics 除外），useBBox 时用盒体对碰撞地图、移动的画刷实体和角色进行追踪，并使用游戏默认的帧时间。
- 地上的光剑（weapon_saber）使用 NPC_type 指定的剑柄，并像游戏中一样发光（黄色外壳、精灵和灯光）。
- 带 gravity 的 misc_model_breakable 会落到地面，被 use 时扑向其 throwtarget，并像游戏中一样从墙壁弹回。
- 载具会下落：步行机、动物和空的战机受重力影响，悬浮车按游戏的悬浮追踪缓慢降落。
- trigger_hurt 也会伤害玩家；玩家死亡后，一秒后所有脚本停止（摄像机运行时除外）。
- 带 "damage" 的 fx_target_beam 会伤害并杀死被其光束击中的角色。

## 日本語

- カメラ：fovaccel の実行中に ZOOM すると、ズーム先から加速を続けて最後にジャンプします。ゲームのクライアントカメラ（FOV、FOV2、FOV_time、FOV_duration）と同じです。
- セイバーの色：NPC ファイルの saberColorRGB/saber2ColorRGB はブレードを変えなくなりました（ゲームはそこに到達しません）。Jedi Outcast は saberColor のみを認識します。
- func_breakable：ブラシが解放されるまで、use のたびに再実行されます（use スクリプト、破壊の再予約）。遅延中の 2 回目の use は破壊を最後の use から 1 遅延後に移し、破壊と削除はサーバーフレーム内のそのエンティティ自身の位置で行われます。
- 最初の Add より前の Remove（SET_REMOVE*HANDBOLT_MODEL）は、ゲームと同様にキャラクター自体（Ghoul2 モデル 0）を削除します。その後に追加したモデルは取り付けられず、キャラクターの原点に立ちます。
- エフェクト：物理付きのパーティクルはゲームと同じようにトレースします。まず新しい位置の内容を調べ（expensivePhysics を除く）、useBBox ならボックスで衝突マップ・移動するブラシエンティティ・キャラクターに対してトレースし、ゲームの既定フレーム時間で計算します。
- 床に置かれたライトセイバー（weapon_saber）は NPC_type の柄を使い、ゲームと同じように光ります（黄色のシェル、スプライト、ライト）。
- gravity 付きの misc_model_breakable は床に落ち着き、use されると throwtarget に向かって投げられ、ゲームと同じように壁で跳ね返ります。
- 乗り物が落下します：ウォーカー、動物、無人の戦闘機は重力で、スピーダーはゲームのホバートレースでゆっくり降ります。
- trigger_hurt はプレイヤーにも当たります。プレイヤーが死ぬと、1 秒後にすべてのスクリプトが停止します（カメラ実行中を除く）。
- "damage" 付きの fx_target_beam は、ビームが当たったキャラクターにダメージを与え、倒します。
