# Stufe 9, Teil 3: Optimierer-Fund, Asset-Browser, Texturen überblenden

Arbeitskopie `Y:\karte`, Zweig `karte1`, ab `6f09b182` (Teil 2, `NOTIZEN_karte9b.md`).
Erledigt: **Optimierer-Fund** (vom Anwender gemeldet, Vorrang), **Punkt 11** (Asset-Browser),
**Punkt 9: Texturen überblenden** (q3map2 `_indexmap`). Offen: Terrain-Brushes formen (unten).

## Commits

| Teil | Commit | Inhalt |
|---|---|---|
| 11 | 83ca36cd | Asset-Browser: Kern (sammeln, filtern, Vorschau-Daten), ctest karte_assets |
| Fund | (siehe git log) | Optimierer: „Fix all safe“ ändert nichts Sichtbares mehr |
| 11 | (siehe git log) | Asset-Browser: Fenster, Ziehen und Ablegen, Prefabs, Handbuch |
| 9 | (siehe git log) | Texturen überblenden: Kern, Fenster, ctest karte_ueberblenden, q3map2_ueberblenden |

## Optimierer-Fund: md_rev_TarisUpper2 (Movie Duels)

Gemeldet: nach File > Open .bsp und „Fix all safe“ sind sichtbare Flächen caulk (hinter einem
Fenster, schmale Streifen zwischen Pfeilern), an Kanten wirkt Geometrie/Textur verschoben.

**Nachgestellt (rot):** Selbsttest-Modus `BHED_KARTE_NUR=optsicher` mit
`BHED_OPTSICHER_KARTEN="a.bsp;b.map;..."` (gui/selbsttest_optsicher.inc). Ablauf je Karte wie der
Anwender: öffnen (.bsp über File > Open .bsp), Optimizer prüfen, „Fix all safe“. Dann werden die
Karte vorher und nachher je **frisch geladen** und an festen Punkten mit der Editor-Kamera
abgetastet (48×36 Stichproben): je neu gecaulkter Fläche ein Blick auf sie aus dem freien Raum
(Punkt per Spielkollision mit den surfaceparms der Shader geprüft, ein Kasten ±8 darf nicht im
Festen stecken; Kamera im Festen sähe Flächen, die im Spiel niemand sieht), dazu Punkt-Entities im
Freien in vier Richtungen. Gewertet wird **wie im Spiel**: mit dem Filter „Caulk“ (caulk ist
nodraw); unruhige Stichproben (vorher zweimal verschieden: Shader-Animation) zählen nicht; eine
einzelne abweichende Stichprobe ohne abweichenden Nachbarn zählt als Haarriss (Teilpixel-Spalt an
einer Pinselkante, nur in der Editor-Kamera; wird mitgezählt und gemeldet); neu gecaulkte Flächen
ganz ohne Blick gelten als nicht geprüft (Fehler). Vor jedem Abtasten wird gewartet, bis alle
Texturen geladen sind. Dazu die größte Eckpunkt-Verschiebung (Primitive werden
abgeglichen, gelöschte entartete Pinsel übersprungen). Untersuchungshilfen: `BHED_OPTSICHER_NUR=<id>`
(nur eine Behebung), `BHED_OPTSICHER_ORTE="x y z nick gier;..."`, `BHED_OPTSICHER_NURFEST=1`,
`BHED_OPTSICHER_MAXFL=n`, `BHED_OPTSICHER_BILDER=1` (die ersten 6 abweichenden Blicke als .ppm).

Mit dem alten Optimierer (Stand 83ca36cd) auf md_rev_TarisUpper2: 87 Flächen caulk, **2448
Stichproben in 14 von 87 Blicken wie im Spiel anders** (2×2-Blöcke; dazu 227 einzelne) – rot; mit
dem neuen: 50 Flächen, 0 – grün. Die Bilder zeigen
die Pfeiler-Streifen (C-K1-Tar/LTS_Pwall03 bei -5170 -3890) und Balken-Enden, die nachher fehlen.

Holzwege unterwegs (für spätere Prüfungen wichtig): (a) Kamera im deckenden Pinsel – sieht von
innen die verdeckte Fläche; (b) Bild sofort nach dem Kamerasprung – Platzhalter-Texturen;
(c) Undo/Redo statt frischem Laden – nach dem Undo des großen Schritts fehlten im Editor Modelle
(Bäume, Felsen) bis zum Redo (siehe Offene Punkte); (d) Primitive nach Index verglichen – das
Löschen entarteter Pinsel verschob alle folgenden.

**Ursachen:**
1. Verdeckt-Regel: eine einzelne undurchsichtige Fläche eines anderen Pinsels galt als Verdecker.
   Hat dieser Pinsel aber eine Seite aus Glas, eine Werkzeug-Seite oder eine **caulk-/nodraw-Seite**
   (im Spiel nicht gezeichnet), sieht man durch ihn hindurch auf die „verdeckte“ Fläche – genau
   die Streifen zwischen den Pfeilern und die Flächen hinter dem Fenster.
2. Glas-Shader nur mit `blendFunc` bzw. `alphaFunc` in der ersten Stufe (ohne
   `surfaceparm trans`/`qer_trans`) galten als undurchsichtig.
3. Die Fläche durfte 0,05 über den Verdecker hinausragen (Ebenenabstand ebenso 0,05) – an
   Kanten ein sichtbarer Streifen.
4. Einrasten („offgrid“, sicher nur bis 0,05) ohne Texturfixierung und ohne Formprüfung.
Kein Grund war der Caulk-Name: `textures/common/caulk` steht in JKA (system.shader) als Verweis auf
`textures/system/caulk`; „Fix all safe“ setzt jetzt trotzdem das eigentliche `textures/system/caulk`.

**Behoben:** Verdecker nur Pinsel, deren Seiten ALLE gezeichnet, undurchsichtig, bekannt und keine
Werkzeuge sind; `skriptBlock` erkennt mischende bzw. ausschneidende erste Stufen
(`ShaderSkript::durchsichtig`); die Fläche muss ganz im Verdecker liegen (0,001), Ebenenabstand
höchstens 0,01; Einrasten mit Texturfixierung, bei „sicher“ nur, wenn jeder alte Eckpunkt
höchstens 0,05 von einem neuen liegt und die Flächenzahl bleibt – sonst bleibt der Pinsel, wie er
war; „entartet“ löscht nur Pinsel, die auch der Editor für entartet hält. Die Erklärung des
Befunds im Optimizer (4 Sprachen) sagt, was als Verdecker zählt. Auf Taris: 50 statt 87 Flächen
gecaulkt, nichts sichtbar anders.

**Pflichttest** (Lauf optpflicht3, `BHED_OPTSICHER_MAXFL=60`, je Karte bis 60 Blicke auf neu
gecaulkte Flächen und bis 10 Punkt-Entities × 4 Richtungen): „Fix all safe“ ändert auf
**md_rev_TarisUpper2** (Open .bsp) und **13 dekompilierten Originalkarten** nichts wie im Spiel
Sichtbares (0 Stichproben in 2×2-Blöcken), keine Eckpunkt-Verschiebung; 26 OK, 0 FEHLER:

| Karte | Blicke | neu caulk | Haarrisse |
|---|---|---|---|
| md_rev_TarisUpper2.bsp | 50 | 50 | 0 |
| ja: academy1 | 100 | 272 | 0 |
| ja: academy2 | 100 | 274 | 0 |
| ja: hoth2 | 100 | 1410 | 1 |
| ja: kor1 | 44 | 12 | 0 |
| ja: mp/ctf1 | 41 | 1 | 0 |
| ja: mp/duel1 | 55 | 19 | 0 |
| ja: mp/duel5 | 42 | 2 | 0 |
| md: duel_hoth | 100 | 357 | 3 |
| md: duel_jabba | 100 | 7847 | 0 |
| md: md_jt | 100 | 1502 | 0 |
| md: duel_tantiveiv | 100 | 4132 | 0 |
| md: duel_mustafar | 100 | 339 | 3 |
| md: duel_vong | 100 | 8951 | 0 |

Die Haarrisse (Bilder angesehen: 1-Pixel-Linien an Pinselkanten, durch die die Editor-Kamera die
Fläche dahinter trifft) zählen nicht; ohne Filter („sonst anders“) unterscheiden sich höchstens 3
Stichproben je Karte, ebenfalls Einzelpunkte.

ctests: `karte_optimierer` (erwartet system/caulk), neu `karte_optsicher` (durchsichtige Stufen;
ein Pinsel mit Glas-, Werkzeug- oder caulk-Seite deckt nicht).

## Punkt 11: Asset-Browser

**View > Map tools (JK Studio) > Asset browser** (nicht in NetRadiant-Custom): Modelle (.md3,
.glm, .ase, .obj aus `models/`), Prefabs (`.map` im Import-Ordner `import/prefabs`) und Klänge
(.wav, .mp3 aus `sound/`), Vorschau (Modell: Textur des ersten Shaders, Klang: Wellenform aus dem
Ton-Leser, Prefab: Umriss von oben), Suche (alle Wörter, ohne Groß/Klein). Ziehen auf eine
2D-Ansicht bzw. die Kamera setzt an den Ort unter dem Zeiger (`ablageOrt`, wie das Modell-Ablegen
aus Punkt 2), Doppelklick vor die Kamera; Modell → misc_model, Klang → target_speaker, Prefab →
eingefügt (Boden-Mitte an den Ort). „Save selection as prefab“ schreibt die Auswahl in den
Import-Ordner; „Pack map as .pk3“ nimmt ihn mit.

Selbsttest (stufe9): Menüklick öffnet das Fenster; Modelle und Klänge da; Suche findet das
Modell, Unsinn nichts; ein Modell **mit der Maus** aus dem Browser in die XY-Ansicht gezogen wird
misc_model bei 0 0; Klang → target_speaker; Brush als Prefab gespeichert, gelistet, eingefügt;
Undo bis zum Anfang.

## Punkt 9 (Rest): Texturen überblenden

**View > Map tools (JK Studio) > Texture blending** (nicht in NetRadiant-Custom): für die
func_group der Auswahl 2–4 Schichten mit je einer Textur, „Set up“ setzt die q3map2-Schlüssel
`_indexmap` (`maps/<karte>_<nr>_blend.tga`), `_layers`, `_shader` (`jks_blend/<karte>_<nr>`),
schreibt in den Import-Ordner die Index-Karte (32-Bit-TGA, oben links; Rot = Schicht·256/n + Mitte,
wie `LoadEntityIndexMap` `(Rot·n)/256` liest) und `shaders/jks_blend_<karte>_<nr>.shader`
(Grund-Shader mit `q3map_terrain`, je Schicht `_<i>`, je Paar `_<a>to<b>` mit zweiter Stufe über
`alphaGen vertex` – die Namen bildet `GetIndexedShader`) und gibt allen Flächen der Gruppe
(außer caulk/nodraw) den Grund-Shader – ein Undo-Schritt; danach „Refresh shaders“. Gemalt wird
in einer Draufsicht im Fenster (oben = größtes Y wie `GetShaderIndexForPoint`, Grenzen wie q3map2:
Pinsel und Patch-Kontrollpunkte, „min“/„max“ gehen vor); beim Loslassen wird die TGA geschrieben.
Eine schon eingerichtete Gruppe wird beim Wählen wieder aufgenommen. Kern
`src/karte/ueberblenden.cpp` (Port der q3map2-Formeln).

Tests: ctest `karte_ueberblenden` (Bildpunkt, Malen, TGA hin und zurück, Grenzen, Shader-Namen);
`q3map2_ueberblenden` (bhed_q3map2 baut eine func_group aus vier Platten mit Index-Karte: die
Flächen liegen auf `_0`, `_1` und `_0to1`, Gegenprobe ohne `_indexmap`: keine); Selbsttest
stufe9: Menüklick, „Set up“ angeklickt, Schlüssel/Dateien/Flächen, Schicht 1 mit der Maus in die
Draufsicht gemalt (Karte und Datei gleich), Undo.

## Offene Punkte

- **Punkt 9, Rest:** Terrain-Brushes formen (Gelände formen wirkt nur auf Patches; Brushes formt
  man mit dem Eckpunkt-Werkzeug bzw. dem Terrain-Plugin) – vor dem Release nicht begonnen.
  Texturen überblenden: keine Vorschau der Überblendung in der Editor-Kamera (der Editor zeigt
  den Grund-Shader mit dem Bild der Schicht 0; die Draufsicht im Fenster zeigt die Schichten).
- Optimizer, „verdeckt“: die Regel ist bewusst vorsichtig (ein Pinsel mit einer caulk-Seite deckt
  nichts, auch wenn diese Seite selbst verdeckt ist); dadurch werden weniger Flächen gecaulkt als
  möglich.
- **Undo nach „Fix all safe“** auf md_rev_TarisUpper2: danach fehlten in der Editor-Kamera Modelle
  (misc_model: Bäume, Felsen, ferne Stadt), bis Redo sie zurückbrachte; die Karte selbst (Text)
  war richtig. Nicht untersucht (Zeit); nachstellen mit dem optsicher-Modus über Undo/Redo statt
  frischem Laden.
- Asset-Browser: keine 3D-Vorschau der Modelle (Textur des ersten Shaders), Klänge nicht
  abspielbar.

## Changelog

## English

- Map editor, optimizer: **Fix all safe** no longer changes what you see. It counted brushes with glass, caulk, nodraw or tool faces as covering (you can look through them in the game), let faces stick out of the covering face by up to 0.05 units, and snapped brushes without keeping textures in place (reported on md_rev_TarisUpper2).
- Map editor: **View > Map tools > Asset browser** - models, prefabs and sounds with previews and search; drag one onto a 2D view or the camera to place it. Save the selection as a prefab.
- Map editor: **View > Map tools > Texture blending** - blend 2 to 4 textures on a terrain func_group: paint where each lies, q3map2 builds the blended surfaces (index map, shaders and keys are created for you).

## Deutsch

- Karten-Editor, Optimierer: **Fix all safe** ändert nichts Sichtbares mehr. Er zählte Pinsel mit Glas-, caulk-, nodraw- oder Werkzeug-Seiten als Verdecker (im Spiel sieht man durch sie hindurch), ließ Flächen bis 0,05 Einheiten über die deckende Fläche ragen und rastete Pinsel ein, ohne die Texturen festzuhalten (gemeldet an md_rev_TarisUpper2).
- Karten-Editor: **View > Map tools > Asset browser** – Modelle, Prefabs und Klänge mit Vorschau und Suche; eines auf eine 2D-Ansicht oder die Kamera ziehen setzt es dort ein. Die Auswahl als Prefab speichern.
- Karten-Editor: **View > Map tools > Texture blending** – 2 bis 4 Texturen auf einer Gelände-func_group überblenden: malen, wo welche liegt; q3map2 baut die überblendeten Flächen (Index-Karte, Shader und Schlüssel legt der Editor an).

## 中文

- 地图编辑器,优化器:**Fix all safe** 不再改变可见内容。它以前把带有玻璃面、caulk 面、nodraw 面或工具面的画刷当作遮挡物(在游戏中可以透过它们看到),允许面超出遮挡面最多 0.05 单位,并在对齐网格时没有锁定纹理(在 md_rev_TarisUpper2 上报告)。
- 地图编辑器:**View > Map tools > Asset browser**——带预览和搜索的模型、预制件和声音;把一项拖到二维视图或摄像机上即可放置。可将所选内容保存为预制件。
- 地图编辑器:**View > Map tools > Texture blending**——在地形 func_group 上混合 2 到 4 种纹理:绘制每种纹理的位置,q3map2 生成混合表面(索引图、着色器和键由编辑器创建)。

## 日本語

- マップエディター、最適化:**Fix all safe** が見た目を変えなくなりました。ガラス・caulk・nodraw・ツールの面を持つブラシを覆いとして数え(ゲームではそれらを透かして見えます)、面が覆う面から最大 0.05 ユニットはみ出すことを許し、テクスチャを固定せずにブラシをグリッドに合わせていました(md_rev_TarisUpper2 で報告)。
- マップエディター:**View > Map tools > Asset browser** — プレビューと検索付きのモデル・プレハブ・サウンド。2D ビューかカメラにドラッグして配置します。選択範囲をプレハブとして保存できます。
- マップエディター:**View > Map tools > Texture blending** — 地形の func_group で 2〜4 種類のテクスチャをブレンド。どこに何を置くかを塗ると、q3map2 がブレンドされた面を作ります(インデックスマップ・シェーダー・キーはエディターが作ります)。
