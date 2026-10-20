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
