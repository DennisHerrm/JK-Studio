# NOTIZEN rc605r – Restpunkte ohne Laufen (Zweig fix605r, Basis rc604 8cc7bd1)

Ein Testprozess zugleich, normale Priorität, Bau `--parallel 6`. Je Punkt ein gezielter Rot/Grün-Test
(Selbsttestmodus `rest605`, `BHED_REST=<teil>` wählt einen Teil) und ctest.

## 1. Weltshader: tcGen vector und tcMod entityTranslate

- Spiel: ComputeTexCoords TCGEN_VECTOR (code/rd-vanilla/tr_shade.cpp:1675-1680 MD): st = (xyz · v0, xyz · v1)
  mit tess.xyz nach den Deforms (an Brush-Modellen im Modellraum); ParseStage liest die zwei Vektoren mit
  ParseVector (tr_shader.cpp:1611-1617 MD). TMOD_ENTITY_TRANSLATE (tr_shade.cpp:1709-1712 MD) ist
  RB_CalcScrollTexCoords mit `backEnd.currentEntity->e.shaderTexCoord`: die Welt zeichnet mit tr.worldEntity
  (R_Init leert tr), Brush-Modelle und Figuren mit den refEntities aus cg_ents.cpp (memset; shaderTexCoord setzt
  nur CG_TestLine, cg_effects.cpp:634) – Verschiebung 0, die Anweisung ändert nichts.
- behaved: TexGen::Vector mit `tcGenVec[6]` durch Parser (shaderscript.cpp), TextureSet::Tex, BatchState,
  Stapelpuffer gTcGenS/gTcGenT (gpuconst.h 36/40, kKonstFloats 44), Vertexshader-Bit kTexGenVec
  (`aus.uv = (welt·S, welt·T)`, gpushader.cpp). entityTranslate bleibt ausgelassen (= wie im Spiel); die
  Kartenprüfung zählt beide nicht mehr als Lücke.
- Nicht geändert: der Dlight-Weg (ProjectDlightTexture2 nimmt eine tcGen-vector-Stufe als dStage, mit den
  Texturkoordinaten der zuletzt gezeichneten Stufe) zeichnet weiter mit der Datei-Texturkoordinate.

| Test | Rot (vorher) | Grün (nachher) |
|---|---|---|
| Selbsttest rest605/tcgen (duel_jedi_battleground, Gelände ter_ruusan vom Spielerstart, mit/ohne) | Bildstufen mit tcGen vector 0, Bildpunkte anders 0 | 3 Stufen, 1 103 593 Bildpunkte anders (Fels statt verschmierter Datei-UV) |
| gpuconst-Probe (Lage 36/40, Shader rechnet dot(welt, …)) | – (neu) | ok |
| kartenpruefung-Probe (Welt: tcGen vector keine Lücke) | – | ok |
| ctest | – | 67 / 67 |

Changelog:

- EN: World shaders with `tcGen vector` (the terrain of duel_jedi_battleground, the star wall on duel_tower) are textured like the game; `tcMod entityTranslate` (conveyor belts) is checked to change nothing in the game either.
- DE: Weltshader mit `tcGen vector` (das Gelände von duel_jedi_battleground, die Sternwand in duel_tower) sind texturiert wie im Spiel; `tcMod entityTranslate` (Förderbänder) ändert auch im Spiel nichts – geprüft.
- ZH: 带 `tcGen vector` 的世界着色器(duel_jedi_battleground 的地形、duel_tower 的星形墙)现在与游戏中一样贴图;已核实 `tcMod entityTranslate`(传送带)在游戏中同样不产生变化。
- JA: `tcGen vector` を使うワールドシェーダー(duel_jedi_battleground の地形、duel_tower の星の壁)がゲームと同じようにテクスチャ表示されます。`tcMod entityTranslate`(コンベヤーベルト)はゲームでも何も変えないことを確認しました。

## 2. surfaceSprites auf Brush-Modellen

- Spiel: RB_DrawSurfaceSprites läuft für jede Fläche in tess, auch für Brush-Modelle (tess.xyz im Raum des
  Modells); für eine Entity nimmt es backEnd.ori.viewOrigin und R_WorldNormalToEntity der Blickachsen
  (tr_surfacesprites.cpp:1471-1489 OpenJK), ssfwdvector/ssrightvectors und Wind bleiben die der Welt
  (:163-183). Gezeichnet wird mit der Stellung der Entity; EF_NODRAW (func_wall START_OFF, SP_func_wall
  g_mover.cpp:2793-2801 MD) zeichnet nichts. Eine surfaceSprites-Stufe an Platz 0 zeichnet die Engine weder
  als Schicht (tr_shade.cpp:1888-1892 MD) noch als Bilder (Schleife ab Stufe 1, :2148-2156).
- behaved (gui/app_gras.cpp): `flaechenAus` für Welt und Brush-Netze, `BrushGras` je Brush-Modell; das
  Gras wird jetzt NACH den Movern gebaut (app_view3d.cpp) und bekommt deren Stellungen dieses Bildes
  (nur gezeichnete Brush-Modelle, keine Zweitmodelle). Auge/Achsen mit der Umkehrung der Drehung ins
  Modell, die Vierecke mit baueMoverWelt in die Welt. Shader mit surfaceSprites an Platz 0: Grundstufe
  unsichtbar (additiv, rgbGen const 0, ohne Tiefe) statt als Schicht (app.cpp, `grasPlatzNull`).
- duel_tower *13 (textures/system/customrainsurf, alle drei Stufen ssFXWeather): die Karte hat kein Wetter
  (kein fx_rain, r_surfaceWeather 0) – im Spiel also nie Spritzer; mit dem Eingriff des Tests (fx_rain)
  erscheinen sie erst nach dem use. Die Welt von duel_tower hat eigene Regenflächen (88 Vierecke bei
  Regen), die rc604 schon zeichnete.
- Windböen: R_SurfaceSpriteFrameUpdate würfelt sie mit Q_flrand (nicht nachstellbar) – es bleibt der
  eingeschwungene Wind (Mitte), wie in rc604 dokumentiert.

| Test | Rot (vorher) | Grün (nachher) |
|---|---|---|
| Selbsttest rest605/brushgras (duel_tower + fx_rain, use octelevator bei 2000 ms, dann origin 0 0 100) | im verschobenen Kasten von *13 0 Vierecke; Platz-0-Shader 0 (Grundstufe als Schicht) | auf Brush-Modellen vor dem use 0, danach 2 (Mitte z -446,9), verschoben 2 (Mitte z -346,9 = +100); Platz-0-Shader 1 |
| Selbsttest gras (Weltgras unverändert) | – | 13 OK / 0 FEHLER |
| ctest | – | 67 / 67 |

Changelog:

- EN: Grass and rain splashes (`surfaceSprites`) also grow on brush entities, moving with them and only while they are shown - for example on a wall that appears when it is used; a shader whose first stage carries surfaceSprites no longer shows that stage as a flat layer, like the game.
- DE: Gras und Regenspritzer (`surfaceSprites`) wachsen auch auf Brush-Entities, bewegen sich mit ihnen und nur, solange sie gezeigt werden - etwa auf einer Wand, die beim Benutzen erscheint; ein Shader, dessen erste Stufe surfaceSprites trägt, zeigt diese Stufe wie im Spiel nicht mehr als flache Schicht.
- ZH: 草和雨滴溅花(`surfaceSprites`)现在也会出现在笔刷实体上,随其移动,且只在实体显示时出现——例如被使用后才出现的墙;第一阶段带有 surfaceSprites 的着色器不再把该阶段显示为平面图层,与游戏一致。
- JA: 草や雨の跳ね(`surfaceSprites`)がブラシエンティティ上にも表示され、エンティティと一緒に動き、表示されている間だけ現れます(たとえば使用されて初めて現れる壁)。最初のステージに surfaceSprites を持つシェーダーは、ゲームと同じくそのステージを平面レイヤーとして表示しなくなりました。

## 3. deformVertexes normal / autosprite / autosprite2 an Modellen und Effekten

- Datenprüfung (Skript über alle .shader, .md3, .efx der Movie-Duels-Archive): 166 Shader tragen eines der
  drei Deforms. Kein .efx nennt einen davon als Teilchenshader, kein Figurenmodell (rc604). Zwei Modelle
  benutzen autosprite: maps/fogsprite_1..4.md3 (textures/exegol_sh/fog_gfx_*), Modelle des Emitters in
  effects/exegol/fogsprites.efx (fx_runner in duel_exegol und md_ch_palp_ep9), und
  models/players/droideka/shield_sprite.md3 (von keiner Karte/keinem Effekt benutzt). deformVertexes normal:
  kein Modell/Effekt – die Welt- und Mover-Stapel rechnen es seit rc592 (kDeformNormal); an Figuren bleibt
  es offen (kein Fall in den Daten).
- Spiel: AutospriteDeform/Autosprite2Deform (code/rd-vanilla/tr_shade_calc.cpp:358-405/427-447 MD) bauen
  die Vierecke in tess.xyz im Raum des Modells, mit GlobalVectorToLocal (:343-347) = DotProduct mit
  backEnd.ori.axis – den Achsen der Entity, bei nonNormalizedAxes (CEmitter::Draw, code/cgame/
  FxPrimitives.cpp:1382-1398 MD: Achsen * radius) ohne Ausgleich.
- Gefunden beim Test: KEIN Emittermodell wurde gezeichnet – loadMapModels leerte md3Files nach
  loadMapEffects (emitterModels kannte die Modelle, md3Files nicht mehr). Jetzt bleiben die Dateien der
  Emittermodelle (gui/app_view3d.cpp).
- behaved: zeichneMover (gui/gpumap_win32.cpp) gibt einem Netz mit autosprite/autosprite2-Stapeln die
  Blickachsen im Raum des Modells (Spalten der Mover-Matrix, wie GlobalVectorToLocal); der Vertexshader
  liest gVorn/gRechts/gHoch nur dafür.

| Test | Rot (vorher) | Grün (nachher) |
|---|---|---|
| Selbsttest rest605/moversprite (duel_exegol bei 12 s, 377 Nebelmodelle im Flug, mit/ohne Modellachsen) | Mover-Aufrufe mit autosprite 0, Bildpunkte anders 0 (Modelle gar nicht gezeichnet) | 754 Aufrufe (2 Durchgänge), 6 526 Bildpunkte anders |
| ctest | – | 67 / 67 |

Offen: Leistung mit vielen Emittermodellen (377 Mover-Aufrufe je Durchgang auf duel_exegol) – im
Leistungslauf der Schlussrunde ansehen; das Glühen der Mover nimmt die Weltachsen (kein Fall in den Daten).

Changelog:

- EN: Models carried by effects (`useModel`, such as the fog wisps on duel_exegol) are drawn again, and surfaces with `deformVertexes autosprite`/`autosprite2` on models turn towards the camera like the game.
- DE: Modelle, die Effekte mitführen (`useModel`, etwa die Nebelschwaden in duel_exegol), werden wieder gezeichnet, und Flächen mit `deformVertexes autosprite`/`autosprite2` an Modellen drehen sich wie im Spiel zur Kamera.
- ZH: 特效携带的模型(`useModel`,例如 duel_exegol 的雾气)重新被绘制;模型上带 `deformVertexes autosprite`/`autosprite2` 的表面会像游戏中一样朝向镜头。
- JA: エフェクトが持つモデル(`useModel`、たとえば duel_exegol の霧)が再び描画されるようになり、モデル上の `deformVertexes autosprite`/`autosprite2` の面はゲームと同じくカメラの方を向きます。

## 4. Reste der Sprungkette

- **Uebergang in den freien Fall** (PM_GroundTraceMissed, bg_pmove.cpp:7149-7397 MD; codeJK2 2865-3135 und
  OpenJK gleich): fuer eine Figur, die gerade noch stand - Kante, zu flacher Sprung (vz <= 100, kein
  kickoff), Treppe - ausser im Flip die Spur 64 nach unten; trifft sie nichts: vz <= 0 -> BOTH_INAIR1 /
  INAIRBACK1 / INAIRRIGHT1 / INAIRLEFT1 nach Bewegung gegen Gier, unter FP_LEVITATION die FORCE-Folge
  (PM_ForceJumpAnimForJumpAnim 6442 ff.), sonst ohne FP_LEVITATION BOTH_JUMP1/JUMPBACK1 (PMF_BACKWARDS_JUMP).
  Immer jumpZStart = origin[2]; am Boden 0 (34146). navlauf.cpp `inDenFall`.
- **PM_CrashLand bei jeder Landung** (5835-6125 MD, OpenJK 3967 ff., codeJK2 2514-2640), nicht mehr nur
  nach dem kickoff: delta aus jumpZStart fuer FP_LEVITATION >= 1 ((Fall - max(128, forceJumpHeight[
  FP_TableLevel])) / 2, force_landing ab 96, mindestens 1), sonst |vz| / 10; "JET"-Klassen delta 1
  (OpenJK: Boba, Raketentruppler; MD dazu Mandalorianer, Jango, JangoDual; JK2 keine); Raketentruppler
  landen immer (unter RANK_LT ganzer Koerper); sonst bei delta > 10, PMF_BACKWARDS_JUMP, aktivem
  FP_LEVITATION oder force_landing. PM_GetLandingAnim fuer alle Luft-/Sprungfolgen (LAND/LANDBACK/
  LANDLEFT/LANDRIGHT und FORCE...), sonst BOTH_LAND1 bzw. LANDBACK1; velocity x/y halb (MD/OpenJK, nicht
  JK2). SETANIM_BOTH nur bei PM_ForceJumpingAnim(Beine) und Oberkoerper == Beine (JK2: immer LEGS).
  Korrektur: kraftSprungAnim fuehrte BOTH_JUMP1/BOTH_INAIR1 als PM_ForceJumpingAnim - die stehen in
  PM_JumpingAnim (10188 ff. MD), nicht in PM_ForceJumpingAnim (10161-10186).
- **Spawn in der Luft / Fall ausserhalb eines Wegs** (schwerkraft.cpp): NPC_Begin setzt groundEntityNum =
  ENTITYNUM_NONE (NPC_spawn.cpp:2053 MD), also kein Uebergang, jumpZStart 0 -> delta |vz| / 10; die Szene
  zeigt BOTH_LAND1 (HOLD) ab der Landung (Actor::fallAnims), eine spaetere Skriptanimation gilt.
- **Skriptanimation oben vor dem Sprung**: NPC_JumpAnimation, BOTH_INAIR1/BOTH_LAND1 (NPC_BSJump) und
  PM_CrashLand mit SETANIM_BOTH setzen den Oberkoerper; die Szene loescht eine vorher begonnene
  Skriptanimation darauf ab diesem Zeitpunkt (LaufTakt::torsoGesetztAb), statt sie nach dem Sprung
  wiederkehren zu lassen.
- **Nicht portiert:** die Waffenzweige von PM_TorsoAnimation (bg_panimate.cpp:9905 ff. MD, ~1500 Zeilen:
  Gewehrtraeger halten auch im Sprung TORSO_WEAPONREADY*) - die Vorschau hat PM_TorsoAnimation auch fuer
  Stehen und Gehen nicht; das gehoert als eigenes Stueck dorthin. PM_FallToDeath (Sturz in den Tod) nicht.

| Test | Rot (vorher) | Grün (nachher) |
|---|---|---|
| bewegungtest KF1-KF4 (Kante 32/100, FP_LEVITATION 2, Raketentruppler) | – (neu; der alte Code setzte ohne kickoff keine Landung) | ok: KF1 keine Luftanimation, BOTH_LAND1 Beine, vx -297 -> -148; KF2 BOTH_INAIR1 im Fall, BOTH_LAND1; KF3 32 ohne, 100 mit Landung; KF4 ganzer Koerper |
| bewegungtest SP1e + torsoGesetztAb | – | ok (2100) |
| Selbsttest tuerlauf md_acc_sith (Uebersicht: Landeanimationen ohne Sprung) | 0 | 68 (dazu kor1 9); 39 Figuren mit Landung nach dem Spawn, alle in der Szene als BOTH_LAND1 gezeigt |
| tuerlauf kor2, taspir2, md_acc_sith, t3_byss, t2_rancor alt/neu | – | 5 Uebersichtszeilen anders: taspir2 jumper1 letztes Stueck +150 ms (BS_JUMP-Landung jetzt nur Beine, PM_ForceJumpingAnim), md_acc_sith rey_run1/rey_run2/kylo3 +-100 ms bzw. 1-3 Einheiten (Spawn-Landung halbiert die Geschwindigkeit) |
| ctest | – | 67 / 67 |

Changelog:

- EN: NPCs falling from a ledge, stepping off with a jump too low to leave the ground, or spawned in the air now play the game's in-air and landing animations (a hard landing slows them down, like the game); an upper-body script animation that began before a jump no longer comes back after it.
- DE: NPCs, die von einer Kante fallen, mit einem zu flachen Sprung über die Kante gehen oder in der Luft gespawnt werden, spielen jetzt die Luft- und Landeanimationen des Spiels (eine harte Landung bremst sie wie im Spiel); eine Skriptanimation auf dem Oberkörper, die vor einem Sprung begann, kommt danach nicht mehr wieder.
- ZH: 从边缘掉落、以过低的跳跃越过边缘或在空中生成的 NPC,现在会播放游戏的空中和落地动画(与游戏中一样,重落地会让它们减速);跳跃前开始的上半身脚本动画在跳跃后不再恢复。
- JA: 段差から落ちる NPC、低すぎるジャンプで段差を越える NPC、空中にスポーンした NPC が、ゲームの空中アニメーションと着地アニメーションを再生するようになりました(ゲームと同じく激しい着地では減速します)。ジャンプ前に始まった上半身のスクリプトアニメーションは、ジャンプ後に戻らなくなりました。

## 5. Jedi Outcast: Laufzeitzustand des Navigators, PushBlocker, Kyles Waffe

### 5c. Kyle uebernimmt die Waffe des Spielers (umgesetzt)

- Spiel: G_NPCMunroMatchPlayerWeapon (codeJK2/game/g_active.cpp:1119-1129, aus ClientThink 2170) ruft fuer
  NPCs mit NPCAI_MATCHPLAYERWEAPON (NPC_type "kyle", NPC_spawn.cpp:1274-1279) ausserhalb der Skriptkamera
  (`!in_camera`) in jedem Bild G_MatchPlayerWeapon (1078-1117): Waffe des Spielers, ueber WP_DET_PACK ->
  WP_BRYAR_PISTOL, WP_NONE aendert nichts. Q3_SetWeapon auf Kyle loescht das Merkmal (Q3_Interface.cpp:
  3311-3398).
- behaved: munroWaffenAbgleich (src/ausruestung.cpp) fuegt dem Kyle-NPC Waffenschritte ein - zu jeder
  Waffe des Spielers ausserhalb der Kamera und zu jedem Kamera-Ende (kameraVerborgen des Spielers), bis zu
  Kyles erstem eigenen SET_WEAPON; aufgerufen in prepareScene (gui/app_view3d.cpp), nur unter Jedi Outcast.

| Test | Rot (vorher) | Grün (nachher) |
|---|---|---|
| npcklasse-Probe (Spieler WP_BLASTER 1000, Kamera 2000-4000 mit WP_REPEATER 3000, WP_STUN_BATON 5000, Kyle SET_WEAPON 6000) | " 6000:WP_SABER" | " 1000:WP_BLASTER 4000:WP_REPEATER 5000:WP_BRYAR_PISTOL 6000:WP_SABER" |
| Selbsttest jk2 (gezielt) | – | 15 OK / 0 FEHLER |
| ctest | – | 67 / 67 |

Changelog:

- EN: Jedi Outcast: a Kyle NPC takes over the player's weapon whenever a script changes it outside the camera, until a script gives Kyle a weapon of his own - like the game.
- DE: Jedi Outcast: ein Kyle-NPC übernimmt die Waffe des Spielers, wenn ein Skript sie außerhalb der Kamera wechselt, bis ein Skript Kyle eine eigene Waffe gibt - wie im Spiel.
- ZH: Jedi Outcast:脚本在镜头之外更换玩家武器时,Kyle NPC 会随之换上同样的武器,直到脚本给 Kyle 指定自己的武器为止——与游戏中一样。
- JA: Jedi Outcast:スクリプトがカメラの外でプレイヤーの武器を変えると、Kyle の NPC も同じ武器に持ち替えます。スクリプトが Kyle に自分の武器を与えるまで続きます(ゲームと同じ)。

## 6. Fallenstrahlen: andere Fallen, misc_model_breakable, Waffe in der Hand

- Spiel: laserTrapThink spurt MASK_SHOT mit Kasten -4..4 und G2_RETURNONHIT (wp_trip_mine.cpp:150-153
  OpenJK). Eine andere Falle ist eine Punkt-Entity mit Kasten -4..4 und CONTENTS_SHOTCLIP (touchLaserTrap,
  :46-50) bis G_FreeEntity nach WP_Explode; misc_model_breakable hat den Kasten aus mins/maxs (Vorgabe
  +-16, tie_bomber +-80, modelscale samt Ursprungsausgleich, SP_misc_model_breakable g_breakable.cpp:
  1215-1300 MD) und contents nur mit spawnflags 1 (CONTENTS_SOLID|...) oder health (CONTENTS_SHOTCLIP),
  nach dem Bruch 0 ausser spawnflags 4 (590-600). G2_TraceModels geht ueber das ganze ghoul2-Feld der
  Figur - auch die Waffe/den Griff am Handbolzen. func_breakable ist ein Brush-Modell (war schon drin).
- behaved (gui/app_view3d.cpp): FallenWelt::minen/modelle, Kastenspuren in laserfalleSpurBei (gilt auch
  fuer fx_target_beam), die Lebenszeiten der Fallen werden wiederholt gerechnet, bis sie stehen (eine
  Falle haelt die anderen auf, bis sie fort ist); figurNetzTreffer prueft die Modelle am Bolzen
  (baueGriffWelt wie die Zeichnung).
- fx_target_beam-Schaden: keine fx_target_beam in den 278 Karten hat "damage" > 0 (SP_fx_target_beam liest
  nur "damage"; vjun2 schreibt "dmg") - nichts zu brechen, nicht portiert.
- Nicht portiert: Kettenexplosionen ueber G_RadiusDamage (eine Falle hat health 15 und takedamage); in den
  Daten liegt laut Suchskript keine Falle oder kein misc_model_breakable nahe an einem Strahl (die
  Hindernisse aendern auf den Karten also nichts sichtbar).

| Test | Rot (vorher) | Grün (nachher) |
|---|---|---|
| Selbsttest rest605/fallen (t3_byss #168, 148 lang; Falle B an der Wand am Ende, misc_model_breakable +-8 in der Mitte, dasselbe ohne contents) | B 0,0 vor dem Ende, Modell 73,9 von seiner Mitte (geht durch) | B 4,1 davor, Modell 12,1 (Kasten 8 + 4), ohne contents 0,0 |
| Selbsttest rest605/fallenwaffe (gonk_wander_1 als stormtrooper mit WP_BLASTER, Raster aus 868 Spuren) | nur Koerper (alter Weg): 161 Treffer | mit Waffe 163, 2 nur an der Waffe |
| Selbsttest laserfalle (t3_byss, gezielt) | – | 5 OK / 0 FEHLER, Explosion #168 bei 600 ms unveraendert |
| ctest | – | 67 / 67 |

Changelog:

- EN: Laser trip mine beams (and fx_target_beam shots) now stop at other trip mines and at solid breakable models, and the weapon in a figure's hand sets a mine off too - like the game.
- DE: Strahlen von Laserfallen (und Schüsse von fx_target_beam) enden jetzt an anderen Laserfallen und an festen zerbrechlichen Modellen, und auch die Waffe in der Hand einer Figur löst eine Falle aus - wie im Spiel.
- ZH: 激光绊雷的光束(以及 fx_target_beam 的射击)现在会停在其他绊雷和实体可破坏模型处,角色手中的武器也会触发绊雷——与游戏中一样。
- JA: レーザートリップマインのビーム(と fx_target_beam の射撃)が、ほかのトリップマインや固体の壊せるモデルで止まるようになり、キャラクターが手に持つ武器でもマインが作動します(ゲームと同じ)。

## 7. Effekte ohne Shaderskript, AT-ST-Lukendeckel

### 7a. Effektbilder ohne Skript (umgesetzt)

- Spiel: das FX-System registriert mit RE_RegisterShader (SFxHelper::RegisterShader -> cgi_R_RegisterShader,
  code/cgame/FxUtil.cpp:136-140 MD) -> R_FindShader(name, lightmaps2d) (tr_shader.cpp:3535-3536 MD). Ohne
  Skript der implizite Shader fuer LIGHTMAP_2D (:3482-3491): rgbGen vertex, alphaGen vertex,
  GLS_DEPTHTEST_DISABLE | SRC_ALPHA ONE_MINUS_SRC_ALPHA, Bild ohne Mip mit GL_CLAMP (:3450). Mit Skript gilt
  das Skript - ohne blendFunc deckend (GLS_DEFAULT).
- behaved (gui/app_view3d.cpp): die "Faustregel" (Alpha nach dem kleinsten Bildalpha, sonst additiv) ist
  weg; ohne Skript der implizite 2D-Shader, mit Skript ohne blendFunc deckend wie die Engine. In den
  Movie-Duels-Daten betrifft es gfx/misc/steamer (duel_kamino, effects/kamino/babyboom.efx), in den
  JK2-Daten 18 Effektbilder.
- identityLighting = tr.identityLight = 1 / (1 << overbrightBits); r_overBrightBits Vorgabe 0 (tr_init.cpp:
  1659 MD) -> 1 - die bisherige Annahme stimmt (geprueft, keine Aenderung).

| Test | Rot (vorher) | Grün (nachher) |
|---|---|---|
| Selbsttest rest605/effektohne (duel_kamino, gfx/misc/steamer) | Mischart One/One (additiv), Tiefentest an, clamp 0 | SrcAlpha/OneMinusSrcAlpha, Tiefentest aus, clamp 1, alphaGen vertex |
| Selbsttest effektgrund (gezielt) | – | 1 OK / 0 FEHLER |
| ctest | – | 67 / 67 |

### 7b. AT-ST-Lukendeckel (geprueft, keine Aenderung)

G2_SetSurfaceOnOff (code/rd-vanilla/G2_surfaces.cpp:210-252 OpenJK) uebernimmt aus dem Argument nur die Bits
G2SURFACEFLAG_OFF und G2SURFACEFLAG_NODESCENDANTS; SP_misc_atst_drivable und misc_atst_use rufen es mit 0
(g_misc.cpp:3168/3204 MD, 3070/3104 OpenJK) - das loescht OFF, die Flaeche head_hatchcover ist AN. Die
woertliche Portierung (Deckel sichtbar) stimmt; der Kommentar "open the hatch" im Spielcode meint die
Animation BOTH_STAND2.

Changelog:

- EN: Effect images without a shader script are drawn like the game's built-in shader for them: blended by their alpha with the particle's colour and fading, and visible through walls (before: a guessed blend mode).
- DE: Effektbilder ohne Shaderskript zeichnet die Vorschau wie den eingebauten Shader des Spiels: nach ihrem Alpha gemischt, mit Farbe und Ausblenden des Teilchens und durch Wände sichtbar (vorher: eine geschätzte Mischart).
- ZH: 没有着色器脚本的特效图像现在按游戏为其内置的着色器绘制:按其 alpha 混合,带有粒子的颜色和淡出,并且可以透过墙壁看到(之前:猜测的混合方式)。
- JA: シェーダースクリプトのないエフェクト画像を、ゲームの組み込みシェーダーと同じく描画するようになりました。アルファで合成され、パーティクルの色とフェードを使い、壁越しにも見えます(以前は推測した合成方法)。
