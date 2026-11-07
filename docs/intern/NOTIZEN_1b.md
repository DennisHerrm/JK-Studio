# Notizen Modell-Block 1b: UV erstellen

Stand 07.11.2026, Zweig `karte1`, auf 07daa31e3. Ein Agent. Laufnamen `uv_*` (Schluss `uve_*`), Bau `Y:/karte/build-test`,
Laufskript `lauf.py` wie Stufe 5-7. kFassung bleibt rc610.

## Quellen

- WhiteoutLib 35d068d (nur gelesen): `src/whiteout/models/wem/geometry/uv/` islands.cpp, seams.cpp, flatten.cpp, layout.cpp,
  checks.cpp, common.h; `select.cpp` (SharpEdges). WhiteoutFlakes (nur gelesen): `tools/basic_viewer/edit/uv_edits.cpp`,
  `edit/uv_pick.cpp`, `export/uv_charts.cpp`, `ui/uv_window.cpp`, `uv_panel.cpp`, `uv_canvas.cpp`, `uv_gizmo*.cpp`,
  `guide/markdown/uv.md`. Beides BSD-3 (Fernando Sahmkow) mit LICENSE-AI.md; jede abgeleitete Datei liegt in einem eigenen
  Ordner `wof/` mit dem BSD-3-Kopf und dem LICENSE-AI-Hinweis, die Herkunft je Funktion steht im Kopf der Datei.
- Verfahren: LSCM (Levy et al. 2002, ueber CGLS wie WL), minimale Dehnung (SLIM, Rabinovich et al. 2017, Tutte-Start wie WL),
  Entspannen (Sander L2-Dehnung wie WL). xatlas jpcy/xatlas f700c779 (MIT) fuer "Karten nach Form" (Vorlage Requisit).
- JKA-Grenzen: SHADER_MAX_VERTEXES 1000 (`tr_local.h` OpenJK, `Grenzen::eckenJeFlaeche`), GLM nach `mdx_format.h`, MD3 nach
  `qfiles.h` (md3Surface_t, md3St_t), Haut/Shader der Flaeche wie die Vorschau (`GlmSurface::texture`, sonst Shader).
- Tasten: 3ds-Max-Tabelle (Unwrap UVW: 1/2/4/5 Ebene, Q W E R, S Fang, G Raster, Z Einpassen) und WF (C, Shift+C, U, Ctrl+U,
  Shift+U, P, T, Shift+S, D, Ctrl+L, Shift+P, nur ueber dem UV-Editor im Max-Modus); Blender-Tabelle (1/2/3/4, G/R/S modal,
  Shift+Tab Fang, U = VIEW3D_MT_uv_map auch in der 3D-Ansicht, W Kontextmenue).
- Lizenzen: `licenses/WhiteoutFlakes-BSD-3.txt` (mit LICENSE-AI-Text), `third_party/xatlas/LICENSE`, Zeilen in `THIRD-PARTY.md`,
  `tools/release_paket.py` (LIZENZEN), Zeile im About-Fenster (`AboutUvLizenz`), Handbuchseite Lizenzen.

## 1. Gebaut

- `include/bhed/modell/wof/uv.h`, `src/modell/wof/` (uv_inseln, uv_abflachen, uv_anordnen, uv_naehte, uv_pruefen,
  uv_formkarten, uv_befehle): Arbeitsnetz je Texturgruppe und LOD (Halbkanten, Keile = UV-Punkte, Schnitte = Marken oder
  UV-Uneinigkeit), Inseln, Naehte (markieren, loeschen, automatisch nach Winkel, aus dem Skelett mit Rollen-Faltung,
  Spiegelmarken), Abwickeln (LSCM, minimale Dehnung), Projektion (planar/Box/Zylinder/Kugel), Entspannen, Pins, Begradigen,
  Rechteck, Stapeln gespiegelt, Packen (Rasterabdruck, Spaltenprofile, Drehen), Dichte angleichen, Ausrichten, Sperren,
  Pruefen, Messen (Winkel, Konformitaet, Flaeche, Texel-Dichte). "Quick unwrap" mit Vorlagen Figur / Requisit / Harte Flaeche.
- `zurueckschreiben`: je Datei-Ecke eine Ecke je verschiedener UV (Kopien behalten Gewichte und Rohfelder, `Ecke::dateiIndex`),
  verweigert bei >= 1000 Ecken je Flaeche mit Meldung; Tags (`tag_*`) bleiben aussen vor; Gewichte unveraendert.
- MD3: `md3datei.h/.cpp` - lesen (auch aus dem Archiv), schreiben: unveraenderte Flaechen nur st gepatcht (bytegleich),
  geaenderte je Frame aus `dateiIndex` neu. GLM ueber den Schreiber der Stufe 2.
- Bedienung `gui/wof/app_modelltab_uv.cpp`: UV-Editor links der Ansichten (Textur ueber Shader/Haut, Schachbrett, Dehnung,
  Ueberlappung), Auswahl Punkt/Kante/Flaeche/Insel im Editor und auf dem Modell zugleich, Gizmo/Ziehen/Modal G R S mit
  Pixelfang, Raster, Rahmen, Naehte als Linien in 3D, Rollout im Befehlsfeld, Menue UV (Hauptmenue), Rechtsklick, Aktionssuche,
  Tooltips, Tasten nach Max- und Blender-Tabelle. Rueckgaengig: ein Schritt je Befehl ueber das Modelldokument.
- xatlas als eigene statische Bibliothek (`third_party/xatlas`, Originalbytes, `-text`).
- Handbuch: Seite `modell-uv` in 6 Sprachen, Bilder `h_uv_editor`, `h_uv_schnell` (2538 x 1384, angesehen).
- Selbsttest-Gruppen `uv`, `uvmd3`, `uvrundlauf`, `uvspiel`, `uvzeit` (`gui/selbsttest_uv.inc`), ctest `modelluvtest`.
- Lints erfassen die `wof/`-Ordner.

## 2. Rot -> gruen

- Packen liess eine Splitter-Insel ueberlappen (`uv_a2`: 3 Ueberlappungen) -> Abdruck nimmt die Texel entlang der
  Dreieckskanten mit -> `uv_a4` 0, ctest "Splitter".
- Alle Flaechen eine Gruppe "[nomaterial]" (`uv_spiel2`) -> Gruppe nach `GlmSurface::texture` -> `uv_spiel3` 4 Texturen.
- Spiel lud die umgebackenen Texturen nicht (MD sucht .jpg vor .tga, `uv_spiel3/alt1..neu3`) -> Testskript legt .jpg ab ->
  `neu4`; Glanzkarten (`_spec`) mit umgebacken -> `neu6`.
- Harte Flaeche: 2 gekippte Dreiecke aus LSCM (`uv_a7`, `uv_a9`) -> Standard-Methode minimale Dehnung wie WF (unwrapMethod 1)
  -> `uv_a10` 50 OK / 0 FEHLER, 0 gekippt.
- Menueleiste im Selbsttest auf- und zugeklappt, doppeltes Menue "Anzeige" -> nur oeffnen wenn zu, Gruppe "UV-Anzeige".
- Handbuchbilder schnitten Werkzeugleiste und Status ab (`uv_hb1`) -> umbrechende Leiste und Status, Kopf ohne Hinweis wenn
  schmal -> `uv_hb2` (es/cs angesehen).
- ctest: Erwartungen falsch (Kopf ohne Naehte, ungespiegeltes Gitter, offene Roehre) -> Tests richtiggestellt; Vorlage Figur
  schneidet uebrige Ringe (`ringNaht`).
- lint_i18n 111 / portability +13, weil `wof/` nicht erfasst war -> Lints erweitert, Includes ergaenzt.

## 3. Gegenproben und Zahlen

ctest `modell_uv` auf der JKA-Figur der Fixtures (Winkelverzerrung Mittel, Konformitaet, Flaechenverzerrung, Texel-Dichte):

| Teil / Verfahren | Inseln | Winkel | konform | Flaeche | Dichte | gekippt |
|---|---|---|---|---|---|---|
| Kopf Datei | 3 | 8,999 Grad (max 58,7) | 1,4375 | 0,1478 | 33,6/34,0/39,3 | 53 |
| Kopf LSCM neu | 3 | 3,439 (max 22,3) | 1,1194 | 0,4965 | 33,6/34,0/39,3 | 0 |
| Kopf minimale Dehnung | 3 | 4,615 (max 32,6) | 1,1753 | 0,1068 | 33,6/34,2/39,3 | 0 |
| Kopf Quick unwrap Requisit (xatlas) | 13 | 1,339 (max 14,0) | 1,0448 | 0,0204 | 29,9 einheitlich | 0 |
| Kopf Quick unwrap Harte Flaeche | 3 | 4,401 (max 32,2) | 1,1675 | 0,1015 | 28,4 einheitlich | 0 |
| Torso Datei | 2 | 17,377 (max 42,8) | 1,9625 | 0,0393 | 13,9 | 0 |
| Torso LSCM | 2 | 0,769 (max 5,1) | 1,0265 | 0,0394 | 13,9 | 0 |
| Torso minimale Dehnung = Harte Flaeche | 2 | 0,904 (max 5,3) | 1,0315 | 0,0192 | 13,9 | 0 |
| Torso Quick unwrap Requisit | 8 | 0,640 (max 5,9) | 1,0213 | 0,0099 | 13,9 | 0 |

- Packen ohne Ueberlappung (Kasten 6 Inseln, zwei Texturen, Splitter); gespiegeltes Gitter gestapelt; Dichte angeglichen.
- Naht-Teilung: die 54 (Kopf) und 32 (Torso) geteilten Ecken behalten Ort, Normale, Knochen und Gewichte (0 Abweichungen);
  andere Flaechen bytegleich; Grenze: 209 Ecken >= 178 abgewiesen, Flaeche unveraendert, eine Ecke mehr Grenze geschrieben.
- Rundlauf aller LODs ueber das Arbeitsnetz bytegleich: Fixture 4/4; Selbsttest `uvrundlauf` MD 924/924, JKA 127/127, JK2 71/71.
- Selbsttest `uv_a10` (MD-Sturmtruppler, Gruppe trooper-armor, 200 Inseln aus der Datei): Abwickeln halber Torso 14 Stuecke
  0 gekippt; Figur 294 Inseln 67 Stapel 0,99 Grad Flaeche 0,014 Dichte 4,6; Requisit 387 / 0,43 / 0,004; Harte Flaeche
  301 / 3,15 / 0,045; Packen 0 Ueberlappung. MD3 `uv_md3b`: tank.md3, 2 Frames 2 Tags, Requisit 56 Inseln, geschrieben und
  wieder gelesen; exegol_rift bei 1100 Ecken verweigert (Grenze greift).

## 4. Im Spiel (Movie Duels)

MovieDuels-SP per Popen, eigener fs_homepath, fs_game MD, stumm, Fenster ausserhalb des Sichtbaren, kein Fokus, Fotos per
PrintWindow; duel_deathstar, Spieler als Sturmtruppler von vorn (dritte Person, r_lodbias -5 = LOD 0). Die Mod
(`uvspiel`): Quick unwrap Figur auf LOD 0 aller Texturgruppen, die Texturen von der alten auf die neue UV umgebacken
(Farbe und `_spec`), model.glm geschrieben.

- `alt6` (Original) gegen `neu6` (neue UV + umgebackene Texturen): mittlere Abweichung 0,29-0,39 im ganzen Bild, 0,77-1,03 im
  Kasten um die Figur (0..255). Vor dem Umbacken der Glanzkarten 0,42-0,49 / 1,11-1,31; mit falscher Textur 1,04-1,09 /
  2,77-2,92. Der Rest ist die Farbe des Lichtschwerts (orange gegen gelb, g_saber_color wirkt nicht) und Filterung.
- `nurglm1` (nur die neue .glm, alte Texturen): dasselbe Wuerfelmuster wie die Vorschau "nachher" im Model-Tab - das Spiel
  liest die neue UV so wie JK Studio sie zeigt.
- Umbacken geprueft: mittlere Abweichung 1,6-6,0 an den Dreiecksmitten.
- Spielordner vor und nach jedem Lauf gleich.
- Original-JKA: nicht gestartet - das Verfahren der Stufe 7 legt die Original-Dateien fuer den Lauf in `GameData`, die Regel
  dieses Auftrags verbietet jedes Schreiben in Spielordner; der Vergleich lief in Movie Duels.

## 5. Bildzeit (allein, BHED_D3D_DEBUG=0, `uv_zeit1`)

CPU je Bild Mittel / p99: Ansehen 0,31 / 0,69 ms; UV mit offenem Editor 0,46 / 0,80 ms; nach Quick unwrap 0,45 / 0,78 ms;
der Editor allein 0,16-0,19 ms.

## 6. Schluss

- Sauberer Neubau (`--clean-first`, `laeufe/uve_bau.txt`) 0 Fehler, 0 Warnungen; Folgebau (`uve_bau2.txt`) 0 / 0.
- ctest 229/230 (`laeufe/uve_ctest.txt`), `modell_uv` gruen. Rot: `q3map2_pr316` (R10.brushprim/classic/valve220: Probe
  Wasser-/Clip-Kasten) - der Datenordner des Tests ausserhalb des Repos (`scratchpad/s6/pr316/.../gameroot/baseq3/scripts`)
  ist seit 07.11. 14:09 leer, die Shader fehlen; nicht aus diesem Block (kein q3map2-Code beruehrt), Ordner nicht angefasst.
- Selbsttest `uve_*` (Summe `laeufe/uve_summe.txt`): modellblock funktionen 60/0, tasten 43/0, mb2 37/0, blender 44/0, mb3
  21/0, rig 26/0, mb5 159/0, mb6 37/0, mb7 24/0, mb7spiel 4/0, mb7textur 1/0, ph 27/0, rt 19/0, rtjka 1/0, pr 15/0, uv 50/0,
  uvmd3 3/0, uvrundlauf md/jka/jk2 (924/127/71 gleich), rundlauf 2/0 x3, rigrundlauf 4/0 x3, kurvenrundlauf 2/0 x3,
  ikrundlauf 2/1/1, bild 1/0 x3; modell 14/0, kuerzel 202/0, mission1 98/0, Modus 1 2635/0.
  (mb5, mb7*, ph, rt, rtjka liefen zuerst ohne BHED_MB_BELEGE/BHED_RT_GLA/BHED_MB7_*; mit den Variablen wie Stufe 5 gruen.)
- Lints wie der Stand vorher (portability 99, pruefcodes 11, efxflags-Meldung, fremdprogramm), sonst ok; Handbuch es/cs
  0 englisch, handbuch_sprachen ok.

## 7. Abweichungen und Offen

- ABF gibt es in WhiteoutLib nicht; "Abwickeln" bietet LSCM und minimale Dehnung (Standard minimale Dehnung wie WF).
- Ein UV-Satz (JKA hat einen), keine Arbeitssaetze; kein Live-Abwickeln, kein Schnittpinsel/Schleifen-/Pfadwerkzeug.
- Ein gemeinsames Netz je Texturgruppe statt Hindernissen je Netz beim Packen.
- Spiegel: Ortsspiegel -x des Model-Tabs (Toleranz 0,1) statt WL-Topologiespiegel; Rollen aus der eigenen Knochen-Erkennung.
- Vorlage Figur schneidet uebrige Ringe automatisch (WF laesst sie stehen und meldet sie).
- Packer-Abdruck nimmt die Kanten-Texel mit (gegen Splitter-Ueberlappung).
- Eigener MD3-Schreiber statt md3Paket (md3Paket schreibt einen Frame und teilt neu).
- WF-Tasten im Max-Modus nur ueber dem UV-Editor; vorn = +Y wie die Ansicht "Vorne".
- Umbacken der Textur nur im Test (Funktion fuer 1c); im Spiel nur LOD 0 verglichen; .jpg-Umwandlung im Testskript.
- Offen: Original-JKA-Vergleich (siehe 4).
- Offen (nicht aus 1b): `rt` ohne BHED_RT_GLA endet mit 0xC0000005 statt mit FEHLER-Zeilen (erster Schlusslauf); der Test
  sollte ohne Quelle abbrechen. ctest `q3map2_pr316`: Shader im externen Datenordner fehlen (siehe 6).

## 8. Vorschlag 1c

Texture Painting auf denselben Inseln: Pinsel in 3D und im UV-Editor, Ebenen je Texturgruppe, Umbacken alt -> neu als Befehl
(heute nur im Test), Naht-Bluten ueber die Inselraender, Schreiben als .tga/.png und auf Wunsch .jpg, Glanzkarten mit.

## Changelog

### English
- Model tab: create UVs - a UV editor beside the views shows each texture of the model (through shader and skin) with selection of points, edges, faces and islands on the editor and the model at once, gizmo, grid and pixel snap.
- Seams by hand, by angle or from the skeleton, unwrap (LSCM, minimum stretch), planar/box/cylinder/sphere projection, relax, pins, straighten, rectangle, stack mirrored, pack, match density, orient, lock and a check; "Quick unwrap" with the presets Character, Prop and Hard surface.
- Works on .glm (each surface and LOD, tags left out, weights kept) and .md3; the 1000 vertex limit per surface is respected; untouched files save byte for byte.
- Menu UV, right-click, action search and tooltips, keys like 3ds Max or Blender; every step can be undone.

### Deutsch
- Model-Tab: UV erstellen - ein UV-Editor neben den Ansichten zeigt jede Textur des Modells (über Shader und Haut), Auswahl von Punkten, Kanten, Flächen und Inseln im Editor und auf dem Modell zugleich, Gizmo, Raster und Pixelfang.
- Nähte von Hand, nach Winkel oder aus dem Skelett, Abwickeln (LSCM, minimale Dehnung), Projektion planar/Box/Zylinder/Kugel, Entspannen, Pins, Begradigen, Rechteck, gespiegelt stapeln, Packen, Dichte angleichen, Ausrichten, Sperren und eine Prüfung; „Quick unwrap“ mit den Vorlagen Figur, Requisit und Harte Fläche.
- Arbeitet auf .glm (jede Fläche und LOD, Tags bleiben außen vor, Gewichte bleiben) und .md3; die Grenze von 1000 Ecken je Fläche wird eingehalten; unberührte Dateien werden Byte für Byte gespeichert.
- Menü UV, Rechtsklick, Aktionssuche und Tooltips, Tasten wie in 3ds Max oder Blender; jeder Schritt lässt sich rückgängig machen.

### 中文
- 模型标签页：创建 UV —— 视图旁的 UV 编辑器显示模型的每张纹理（通过着色器和皮肤），可在编辑器和模型上同时选择点、边、面和岛，带操纵器、网格和像素吸附。
- 手动、按角度或按骨架生成接缝，展开（LSCM、最小拉伸），平面/盒/圆柱/球形投影，松弛、固定、拉直、矩形、镜像堆叠、打包、统一密度、对齐、锁定和检查；“快速展开”提供角色、道具和硬表面预设。
- 适用于 .glm（每个表面和 LOD，跳过标签，保留权重）和 .md3；遵守每个表面 1000 个顶点的上限；未改动的文件逐字节保存。
- UV 菜单、右键菜单、操作搜索和提示，按键与 3ds Max 或 Blender 相同；每一步都可撤销。

### 日本語
- モデルタブ：UV 作成 —— ビューの横の UV エディタにモデルの各テクスチャ（シェーダーとスキン経由）を表示し、エディタとモデルの両方で点・辺・面・アイランドを同時に選択、ギズモ、グリッド、ピクセルスナップ付き。
- シームを手動・角度・スケルトンから作成、展開（LSCM、最小ストレッチ）、平面/ボックス/円柱/球投影、リラックス、ピン、直線化、矩形、ミラー重ね、パック、密度合わせ、向き合わせ、ロック、チェック；「クイック展開」にキャラクター・小道具・ハードサーフェスのプリセット。
- .glm（各サーフェスと LOD、タグは除外、ウェイトは保持）と .md3 に対応；サーフェスあたり 1000 頂点の上限を守り、変更していないファイルはバイト単位で同じに保存。
- UV メニュー、右クリック、アクション検索とツールチップ、3ds Max または Blender と同じキー；すべての操作を元に戻せます。

### Español
- Pestaña Modelo: crear UV - un editor UV junto a las vistas muestra cada textura del modelo (a través del shader y la skin), con selección de puntos, aristas, caras e islas en el editor y en el modelo a la vez, gizmo, cuadrícula y ajuste a píxel.
- Costuras a mano, por ángulo o desde el esqueleto, desplegar (LSCM, estiramiento mínimo), proyección plana/caja/cilindro/esfera, relajar, fijar, enderezar, rectángulo, apilar reflejadas, empaquetar, igualar densidad, orientar, bloquear y una comprobación; «Quick unwrap» con los ajustes Personaje, Accesorio y Superficie dura.
- Funciona con .glm (cada superficie y LOD, sin las etiquetas, los pesos se conservan) y .md3; se respeta el límite de 1000 vértices por superficie; los archivos sin tocar se guardan byte a byte.
- Menú UV, clic derecho, búsqueda de acciones y descripciones emergentes, teclas como en 3ds Max o Blender; cada paso se puede deshacer.

### Čeština
- Karta Model: tvorba UV - UV editor vedle pohledů ukazuje každou texturu modelu (přes shader a skin), výběr bodů, hran, ploch a ostrovů v editoru i na modelu zároveň, gizmo, mřížka a přichycení na pixely.
- Švy ručně, podle úhlu nebo z kostry, rozbalení (LSCM, minimální natažení), projekce rovinná/kvádr/válec/koule, uvolnění, připíchnutí, narovnání, obdélník, zrcadlové skládání, balení, sjednocení hustoty, natočení, zamčení a kontrola; „Quick unwrap“ s předvolbami Postava, Rekvizita a Tvrdý povrch.
- Pracuje s .glm (každá plocha a LOD, bez tagů, váhy zůstávají) a .md3; limit 1000 vrcholů na plochu je dodržen; nezměněné soubory se ukládají bajt po bajtu.
- Menu UV, pravé tlačítko, hledání akcí a nápovědy, klávesy jako v 3ds Max nebo Blenderu; každý krok lze vrátit zpět.
