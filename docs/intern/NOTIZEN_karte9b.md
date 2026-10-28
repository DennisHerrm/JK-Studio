# Stufe 9, Teil 2: die restlichen Karten-Werkzeuge (Punkte 4 bis 12)

Arbeitskopie `Y:\karte`, Zweig `karte1`, ab `0afc2050` (Teil 1 + Merge der Release-Korrekturen
aus main). Teil 1 (Punkte 1-3) steht in `NOTIZEN_karte9.md`. Erledigt: **4, 5, 6, 7, 8, 10, 12**,
**9 zum Teil** (Formen ja, Texturen überblenden nein), dazu „Pack map as .pk3 nimmt den
Import-Ordner mit“. Offen: **11** (Asset-Browser) und der Rest von 9 (unten).

## Commits

| Punkt | Commit | Inhalt |
|---|---|---|
| 4 | c1ec7fde, da7843e5 | Machtsprung im Spieler-Pmove (OpenJK 1:1), Sprungweite und Platz (Kern, View-Menü, Anzeige) |
| 5 | 430fe4a7, 40e3abd7, 66fe7fc5, bd1a3725 | NPC-Wege: Netz und Testlauf (Kern, CMake doppelt raus, Menü/Anzeige, Lizenzkopf) |
| – | 46cb322b | Pack map as .pk3 nimmt den Import-Ordner mit |
| 6 | 76f01084 | Verknüpfte Kopien (PrefabVerbund, ein Befehl über vorAbschluss) |
| 7 | 74da92c8 | Kurven-Werkzeug (Weg, Rohr, Geländer, Brücke, path_corner, Wegpunkte) |
| 8 | 8d13f0dd, eecafb42 | Modelle verteilen; Grenze MAX_MISC_ENTS im Optimierer |
| 9 | d74a6a3f | Gelände formen auf Patches (heben, senken, glätten, ebnen) |
| 10 | 92ecf53d | Für Blender exportieren (.glb neu, .obj aus brushexport) und zurück |
| 12 | 1ec12647 | Nach einem Absturz das Autosave anbieten |
| – | 761b49a0, (dieser) | Schlussrunde: ein Untermenü „Map tools“, Sitzungsdatei je Prozess, Machtstufe-Vorgabe; Notizen |

## Punkt 4: Machtsprung, Sprungweite und Platz

**Machtsprung 1:1 aus OpenJK** in `navSpielerPmove` (src/navlauf.cpp, Quellen aus
`JACoders/OpenJK` master gelesen, nur gelesen):
- `PM_CheckJump`, Teil `METROID_JUMP` (bg_pmove.cpp:1090-1416) am Anfang von `PM_AirMove`:
  `PM_ForceJumpingUp` (in der Luft, PMF_JUMPING, velocity[2] > 0, FP_LEVITATION > 0, ohne
  forceJumpCharge/Sonderanimation/Zwischensequenz) und PMF_JUMP_HELD; `curHeight` gegen
  `forceJumpHeight` {32, 96, 192, 384}, Tempo `(H - h)/H * forceJumpStrength {225, 420, 590, 840} / 10 + 225`,
  sonst Kappe auf 225 bzw. 0; `upmove = 0` (wirkt auf PM_CmdScale).
- Die Kappe vor `PM_StepSlideMove` in `PM_AirMove` (bg_pmove.cpp:2998-3008, codeJK2 1811-1829).
- `forceJumpZStart` beim Absprung, am Boden 0; FP_LEVITATION endet am Boden.
- **Jedi Outcast**: codeJK2 hat denselben Code und dieselben Tabellen. **Movie Duels** liest
  die Tabellen mit `FP_TableLevel` (höchstens 3) – das Feld ist 0..3.
- Die Kraftleiste (forcePower) gilt als voll; der Verbrauch je Sprung wird nicht gerechnet.

**View > Map tools > Check jumps and space** (`sprungpruefung`): im Raster fällt der geduckte Kasten;
begehbare Böden (Normale z ≥ 0,7) sind Stehplätze (voller Kasten frei) oder „nur geduckt“;
Kanten = ein Nachbar ohne Boden in Stufenhöhe; von höchstens 160 Kanten läuft der Spieler mit
250 in 8 Richtungen an und springt mit gehaltener Taste – ohne und mit Machtsprung der gewählten
Stufe; gemerkt wird, was höher oder über eine Lücke landet. Im Hintergrund (Faden), Anzeige in
2D und Kamera (grün/gelb/blau/violett). Die Stufe gilt auch für „Im Editor spielen“.

Tests: ctest `karte_sprung` (Scheitel 31,6 / ~96 / ~192 / ~384, losgelassen niedriger, JK2 gleich,
Lücke normal, Sims nur mit Machtsprung; rot ohne Machtsprung: 5 Fehler), Selbsttest.

## Punkt 5: NPC-Wege

`npcwege`: die Entities der offenen Karte wie im Entity-Lump, das Netz aus `baueNavNetz` mit
Spuren gegen die Kollision der Brushes, der Testlauf mit `planeLauf` (rennend 300 / gehend 90).
**Das Spiel baut erst ab mehr als drei Wegpunkten ein Netz** (`size_nodes() <= 3`) – das zeigte
der erste rote Lauf; die Konsole sagt es jetzt. View > NPC paths: Netz zeigen (türkis, Sprung
orange), Testlauf vom ersten zum letzten gewählten Entity (gelbe Bahn, ein Kasten in Echtzeit).

Tests: ctest `karte_npcwege` (vier Wegpunkte, Lücke in der Wand, ohne Wegpunkte bleibt die Figur
stehen), Selbsttest (rot zuerst: Wegpunkte außerhalb des Raums).

## Punkt 6: Verknüpfte Kopien

func_groups mit `_bhed_link` / `_bhed_link_origin`. `PrefabVerbund` nach dem Muster des
Spiegel-Modus: Inhalt je Exemplar relativ zum Ursprung (primitivText nach Verschieben mit Texture
Lock); eine Änderung schreibt die anderen **im selben Befehl** (`Rueckgaengig::vorAbschluss`),
also ein Undo; ein ganz verschobenes Exemplar bewegt nur seinen Ursprung; außerhalb eines Befehls
und während eines Ziehens wird nicht abgeglichen (nur gemerkt bzw. gar nicht). Nur Verschiebung,
keine Drehung der Kopien.

Tests: ctest `karte_verbund` (rot ohne Abgleich: 5), Selbsttest.

## Punkt 7: Kurven-Werkzeug

Catmull-Rom durch die gewählten Punkt-Entities (Reihenfolge der Wahl), gleich weit abgetastet
(ungerade Zahl wie Patch-Zeilen, Abstand = Raster × 8). Weg (3 Spalten), Rohr (Kreis aus 9
Kontrollpunkten wie eCylinder), Geländer (zwei Rohre in 40), Brücke (Deck 7 Spalten, Unterseite 3)
als Patches mit der Textur des Texturbrowsers; path_corner- bzw. waypoint-Kette mit
targetname/target. Die Vorderseite des Wegs wie NRCs ebener Patch (der erste Testlauf war rot:
nach unten).

## Punkt 8: Modelle verteilen

Vorlage = gewähltes misc_model_static/misc_model, Bereich = gewählte Brushes (sonst ±256);
senkrechte Punktspur auf die Kollision, begehbar, ≥ 48 auseinander, zufälliger Winkel und
Maßstab. **Neue Grenze** `laufzeit.miscents` = MAX_MISC_ENTS 2000 (OpenJK code/cgame/cg_main.cpp
:1949/:1967, Movie Duels :3147/:3166 – im lokalen MD-Quelltext geprüft; für den Grenzen-Test liegt
OpenJKs cg_main.cpp jetzt in `SP/ojk/oj/code/cgame/`), Jedi Outcast ohne. Der Optimierer zählt
jedes misc_model_static; das Verteilen hält an der Grenze an. `tools/handbuch/grenzen.json` neu.

## Punkt 9: Gelände formen (Teil)

View > Terrain sculpt: M1 in der Kamera formt die Patches unter dem Zeiger (Scene_Intersect):
heben, senken, glätten, ebnen; Radius 64/128/256, Abfall (1 − d/r)²; ein Strich = ein Befehl
(`Patch::punkteSetzen`). **Offen**: Terrain-Brushes und Texturen überblenden (q3map2 `_indexmap`
/ Alpha-Map malen).

## Punkt 10: Blender hin und zurück

File > Export selection for Blender: .glb (neuer Schreiber `glbSchreiben`: glTF 2.0 binär, je
Brush Knoten und Netz, POSITION/TEXCOORD_0/Indizes, Material = Shader, Y oben) oder .obj (NRCs
brushexport schreibt schon Y oben – der erste Selbsttest war rot, weil doppelt umgerechnet).
Zurück mit Import geometry as brushes (Y oben): am selben Ort (Selbsttest: Grenzen gleich).
Patches exportiert brushexport nicht.

## Punkt 12: Nach einem Absturz wiederherstellen

Sitzungsdatei `<Einstellungen>/karte_sitzung.txt` mit dem Namen der offenen Karte, beim sauberen
Beenden gelöscht. Beim Start: liegt sie noch da und ist das Autosave (NRC:
`<Karte>.autosave.map`, ohne Namen `maps/autosave.map`) neuer als die Karte, fragt der Editor;
Ja lädt es unter dem Namen der Karte, als geändert.

## Schlussrunde

Skript `SP/s9b/ende_s9b.sh` (aus `SP/s9/ende_s9.sh`), Summe `SP/s9b/ende_summe.txt`.

| Schritt | Ergebnis |
|---|---|
| Bau | ok |
| ctest gesamt | **146/146** (270 s; neu: karte_sprung, karte_npcwege, karte_verbund, karte_kurven, karte_streuen, karte_formen, karte_wiederherstellen) |
| karteneditor voll (mit großer Karte) | 3129 OK, **87 FEHLER** → Befund 1 → 3144 OK, 58 FEHLER → Befund 2 → 3172 OK, 1 FEHLER (Menübreite File) → **3173 OK, 0 FEHLER** (989 s, Spitze 1034 MB) |
| Modus 1 | **2645 OK, 0 FEHLER** (434 s) |
| übrige 22 Modi | alle **0 FEHLER**, dieselben Zahlen wie Teil 1 |
| Leistung gegen rc605 (md_am_sith, je 2 Läufe) | Vorbereitung rc605 6254/7185 ms, Stufe 9b 6788/6529 ms; Bild Mittel 15/9,5/10 und 15/7,9/11 gegen 15/8,4/12 und 13/8,6/13 ms – im Rauschen gleich |
| Kartenprüfung (278 MD-Karten, gegen Teil 1) | alle Summen gleich (0 Abstürze, 0 nicht geladen, 0 D3D-Meldungen, 1 → 1 nicht gezeichnete Entity) |
| sauberer Neubau | **0 Fehler, 0 Warnungen** (558 s) |

Befund 1: die sechs eigenen Untermenüs machten **View** höher als das Fenster des Selbsttests;
das Menü blieb offen, die folgenden Klicks kamen nicht an (87 Folgefehler). Jetzt ein Untermenü
**View > Map tools (JK Studio)** mit den sechs Werkzeugen; Menügrößen-Tabelle, Selbsttest und
Handbuch nachgezogen. Außerdem spielte „Im Editor spielen“ mit der Vorgabe-Machtstufe 2 höher als
erwartet – die Vorgabe ist jetzt „ohne“.

Befund 2: der Selbsttest startet den Editor im selben Prozess neu (LastMap); die Sitzungsdatei
stand noch da, und die neue Absturzprüfung stellte ihre Frage. Die Sitzungsdatei trägt jetzt die
Prozessnummer; derselbe Prozess zählt nie als Absturz (ctest `karte_wiederherstellen` prüft es).

Die Änderungen nach dem Neubau (Prozessnummer, Menübreite) betreffen die Oberfläche, eine kleine
Kerndatei und den Selbsttest; sie sind warnungsfrei gebaut, und der letzte Volllauf hat 0 Fehler.

## Offene Punkte

- **Punkt 11, Asset-Browser** (Modelle, Prefabs, Klänge mit Vorschaubildern, Suche, Ziehen):
  nicht begonnen.
- **Punkt 9**: Texturen überblenden (q3map2 `_indexmap`, Alpha-Map im Import-Ordner malen) und
  Terrain-Brushes formen.
- Machtsprung: die Kraftleiste wird nicht verbraucht; Salto-Animationen und weitere Machtsprünge
  (Wandlauf, Flip) sind nicht dabei – für die Sprungweite zählt nur die Höhe/Weite.
- Sprungprüfung: Startpunkte höchstens 160 je Lauf, 8 Richtungen.
- NPC-Testlauf: Standardwerte (runSpeed 300 / walkSpeed 90), keine NPC-Datei; Türen/Mover zur
  Zeit sind nicht eingebunden (Kollision nur aus den Brushes).
- Verknüpfte Kopien: keine Drehung; die Menüs „View“ sind dadurch höher (Menügrößen-Tabelle
  nachgezogen).
- Handtests im Spiel für Punkt 1 (siehe Teil 1) stehen weiter aus.

## Changelog

## English

- Map editor, **View > Map tools (JK Studio)**:
  - **Check jumps and space**: shows where the player can stand or only crouch, and which edges a running jump or a force jump (level 1-3) reaches - with the game's real movement. Play in editor (F6) now knows the force jump too: hold the space bar to jump higher.
  - **NPC paths**: shows the waypoint network the game builds, and lets an NPC walk from one selected entity to another with the game's path finding.
  - **Linked copies**: change one copy of a group and all others follow in the same step; one undo takes it back.
  - **Curve tool**: a smooth curve through selected point entities becomes a path, pipe, railing or bridge of patches, or a chain of path_corner or waypoints.
  - **Scatter models**: puts copies of a misc_model_static on the walkable surfaces of an area, with random angle and scale, within the game's limit of 2000 (now also counted by the optimizer).
  - **Terrain sculpt**: raise, lower, smooth and flatten terrain patches with the mouse in the camera.
- Map editor: **File > Export selection for Blender** writes .glb or .obj; Import geometry as brushes brings it back to the same place.
- Map editor: after a crash, the editor offers to load the newer autosave of the map at the next start.
- **Pack map as .pk3** now includes the models, shaders and textures from the import folder.

## Deutsch

- Karten-Editor, **View > Map tools (JK Studio)**:
  - **Sprünge und Platz prüfen**: zeigt, wo der Spieler stehen oder nur geduckt passt und welche Kanten ein Sprung mit Anlauf bzw. ein Machtsprung (Stufe 1-3) erreicht - mit der echten Bewegung des Spiels. Im Editor spielen (F6) kennt den Machtsprung jetzt auch: Leertaste halten springt höher.
  - **NPC-Wege**: zeigt das Wegenetz, das das Spiel baut, und lässt einen NPC mit der Wegfindung des Spiels von einem gewählten Entity zum anderen laufen.
  - **Verknüpfte Kopien**: eine Kopie einer Gruppe ändern, und alle anderen folgen im selben Schritt; ein Undo nimmt es zurück.
  - **Kurven-Werkzeug**: eine glatte Kurve durch gewählte Punkt-Entities wird ein Weg, ein Rohr, ein Geländer oder eine Brücke aus Patches, oder eine Kette path_corner bzw. Wegpunkte.
  - **Modelle verteilen**: setzt Kopien eines misc_model_static auf die begehbaren Flächen eines Bereichs, mit zufälligem Winkel und Maßstab, innerhalb der Grenze des Spiels von 2000 (die jetzt auch der Optimierer zählt).
  - **Gelände formen**: Terrain-Patches mit der Maus in der Kamera heben, senken, glätten und ebnen.
- Karten-Editor: **File > Auswahl für Blender exportieren** schreibt .glb oder .obj; Geometrie als Brushes importieren holt es an denselben Ort zurück.
- Karten-Editor: nach einem Absturz bietet der Editor beim nächsten Start das neuere Autosave der Karte an.
- **Karte als .pk3 packen** nimmt jetzt die Modelle, Shader und Texturen aus dem Import-Ordner mit.

## 中文

- 地图编辑器,**View > Map tools (JK Studio)**:
  - **检查跳跃与空间**:显示玩家可以站立或只能蹲下的位置,以及助跑跳跃或原力跳跃(等级 1-3)能到达的边缘——使用游戏的真实移动。“在编辑器中游玩”(F6)现在也支持原力跳跃:按住空格键可以跳得更高。
  - **NPC 路径**:显示游戏构建的路点网络,并让 NPC 使用游戏的寻路从一个选中的实体走到另一个。
  - **链接副本**:修改组的一个副本,其他副本会在同一步中跟随;撤销一次即可还原。
  - **曲线工具**:穿过所选点实体的平滑曲线可以生成由面片构成的道路、管道、栏杆或桥,或者 path_corner 链或路点链。
  - **散布模型**:把 misc_model_static 的副本放到区域内可行走的表面上,角度和缩放随机,并遵守游戏 2000 个的上限(优化器现在也会统计)。
  - **地形塑形**:在摄像机中用鼠标抬高、降低、平滑和整平地形面片。
- 地图编辑器:**File > 为 Blender 导出所选内容** 写出 .glb 或 .obj;用“将几何体导入为画刷”可放回原位置。
- 地图编辑器:崩溃后,编辑器会在下次启动时提供加载地图较新的自动保存。
- **将地图打包为 .pk3** 现在会包含导入文件夹中的模型、着色器和纹理。

## 日本語

- マップエディター、**View > Map tools (JK Studio)**:
  - **ジャンプと空間を確認**:プレイヤーが立てる場所としゃがんでだけ入れる場所、助走ジャンプやフォースジャンプ(レベル 1〜3)で届く縁を、ゲームの本当の動きで表示します。「エディターで遊ぶ」(F6)もフォースジャンプに対応しました。スペースキーを押し続けると高く跳びます。
  - **NPC の経路**:ゲームが作るウェイポイント網を表示し、ゲームの経路探索で NPC を選んだエンティティから別のエンティティへ歩かせます。
  - **リンクされたコピー**:グループのコピーを 1 つ変えると、ほかのすべてが同じ手順で追従し、元に戻す 1 回で戻ります。
  - **カーブツール**:選んだポイントエンティティを通る滑らかなカーブから、パッチの道・パイプ・手すり・橋、または path_corner やウェイポイントの列を作ります。
  - **モデルを散らす**:misc_model_static のコピーを範囲内の歩ける面に、ランダムな角度とスケールで置きます。ゲームの上限 2000 を守ります(最適化でも数えるようになりました)。
  - **地形の造形**:カメラ内でマウスを使い、地形パッチを上げる・下げる・なめらかにする・平らにすることができます。
- マップエディター:**File > 選択範囲を Blender 用に書き出す** で .glb か .obj を書き出し、「ジオメトリをブラシとして読み込む」で同じ場所に戻せます。
- マップエディター:クラッシュの後、次回の起動時にマップのより新しい自動保存を読み込むか尋ねます。
- **マップを .pk3 にまとめる** は、インポートフォルダーのモデル・シェーダー・テクスチャも含めるようになりました。
