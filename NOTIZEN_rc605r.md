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
