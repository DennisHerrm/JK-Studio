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
