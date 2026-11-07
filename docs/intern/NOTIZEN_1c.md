# Notizen Modell-Block 1c: Texture Painting

Stand 07.11.2026 abends, Zweig `paint1` (Worktree `Y:\paint`), Basis e6daa5ca8. Paralleles Fenster 19-23 Uhr, ein Agent.
Laufnamen `pt_*`, Bau `Y:/paint/build-paint` (wie `Y:/karte/build-test`), Laufskript `ab/lauf_paint.py`. kFassung bleibt rc610.

## Quellen

- Neu geschrieben, nichts aus WhiteoutFlakes uebernommen (WF hat kein Texture Painting; `edit/skin_brush.*` nicht gelesen,
  die Begriffe Abstand/Druck/Abfall sind die jedes Malprogramms) - darum kein BSD-3-Kopf, Dateien ausserhalb von `wof/`.
- Ziel-Bild wie die Engine: Haut (.skin) bzw. Shader der Flaeche, sonst erste map-Stufe des gleichnamigen Skripts
  (R_FindShader/ParseStage, tr_shader.cpp; im Programm wie `texturesFor` in app_view3d.cpp), Suchreihenfolge der Endungen
  ueber `textureCandidates` (MD sucht .jpg vor .tga, 1b-Fund) - die Datei behaelt Namen und Endung des Originals.
- TGA: LoadTGA (tr_image_tga.cpp) weist RLE von oben nach unten ab ("RLE-RGB Images (type 10) must be in bottom-to-top
  format", derselbe Text in `src/image.cpp`) -> alle Schreibungen von unten nach oben wie die Bilder des Spiels.
- JPEG: ITU-T T.81 (Baseline, Anhang K.3 Huffman-Tabellen, Anhang K.1 Quantisierung), JFIF-Farbraum wie libjpeg jccolor.c;
  geschrieben mit den DQT-Tabellen des Originals (gleiche Qualitaetsstufe), 4:4:4.
- PNG: RFC 2083/1950/1951 mit ungepackten Deflate-Bloecken (das Projekt hat keinen Packer).
- Bedienung: 3ds Max Viewport Canvas (Ebenen, Pinsel in der Ansicht), Blender Texture Paint (Draw, Soften, Smear, Clone,
  Fill, Mask; Normal Falloff 80 Grad; Spacing 10 %; F/Umschalt+F Radius/Staerke, S Farbe, X Farben tauschen laut
  Blender-Handbuch, 1 Malmaske laut `belege/blender_keys.txt` "Image Paint").

## 1. Gebaut

- `include/bhed/modell/malen.h`, `src/modell/malen.cpp`, `src/modell/malen_datei.cpp` (Namensraum `modell::texmalen`):
  Leinwand je Bild (Original-Bytes, Groesse, Format, TGA-Tiefe/RLE), Ebenen (unterste = Original, Deckkraft, Mischart
  normal/multiplizieren/addieren/screen/overlay, sichtbar), Maske, Ergebnis-Stapel; Pinsel mit Radius, Staerke, Haerte
  (Abfall smoothstep), Abstand im Strich, Druck auf Groesse/Staerke; Werkzeuge Malen, Klonen (Versatz in UV),
  Verwischen (Richtung des Strichs), Weichzeichnen (3 x 3), Fuellen (zusammenhaengend, Toleranz), Maske, Radieren
  (Ebene: Alpha, Original: zurueck zum Original); Tupfer in 3D als Kugel in der Ruhelage (ueber Naehte ohne Bruch),
  Normal-Abfall 80 Grad, in 2D als Kreis in Texeln; Spiegeln (Achse x/y/z in der Ruhelage); Ausbluten ueber die
  UV-Grenzen (Ausblutkarte je Leinwand, Vorgabe 4 Texel wie das Umbacken in 1b); Rueckgaengig in Kacheln 64 x 64 (ein
  Strich = ein Schritt, Grenze 512 MB); Umbacken alte -> neue UV (aus dem Test 1b als echter Befehl, alle Ebenen und die
  Maske, ein Rueckgaengig-Schritt); Schreiben: unbemalt die Bytes des Originals, sonst TGA (Tiefe und RLE des
  Originals, Zeilen von unten), JPEG (eigener Baseline-Schreiber mit den DQT-Tabellen des Originals), PNG.
- `gui/app_modelltab_malen.cpp`: Bereich Painting im Model-Tab - Rollouts (Viewport Canvas / Texture Paint: Werkzeuge,
  Pinsel, Farbe, Spiegeln, Druck, Rueckseiten, Ausbluten; Bild: Textur und Stufe des Shaders; Ebenen; Fuer das Spiel),
  2D-Bild links neben der Ansicht (Leinwand / Image Editor, UV-Dreiecke, Maske rot, Rad zoomt, Mitte schiebt, nur das
  geaenderte Rechteck wird hochgeladen), Malen in der 3D-Ansicht (perspektivisch richtiger Treffer, Gruppe unter dem
  Pinsel, auch waehrend die Animation laeuft), Vorschau-Textur der Flaechen sofort neu (TexWache meldet den neuen Inhalt),
  Menue "Painting", Rechtsklick, Aktionssuche, Tasten wie Blender (F, Umschalt+F, S, X, 1), Strg+Z/Strg+Y in Kacheln,
  "Fuer das Spiel bauen" (nur bemalte Bilder, nie in einen Spielordner), Stiftdruck ueber WM_POINTER (main_win32.cpp).
- Texte `tools/i18n_paint.py` (6 Sprachen), Handbuchseite `tools/handbuch/inhalt_paint.py` (`modell-painting`).
- Tests: ctest `modell_malen` (`tests/modell_malen.cpp`), Selbsttest `gui/selbsttest_pt.inc` (Gruppen `pt`, `ptspiel`,
  `ptzeit`, Handbuch `BHED_HB_NUR=pt`).

## 2. Nachtest ZUS (08.11.)

- Neubau (modelltab.h) 0 Warnungen. Selbsttest pt zuerst 34/1 (`zs_pt_pt`): die Pruefung des Befehlsfelds suchte die Knoepfe im
  Wurzelfenster statt im Kindfenster `mtrollouts` (Testfehler, `e.innen`); danach fehlte "Fuer das Spiel bauen" (unten ausserhalb des
  Sichtbaren) -> eigene Pruefung nach Rollen ans Ende. Gruen `zs_pt_pt3` 36/0.
- Handbuchbild h_pt_malen 6 Sprachen (`zs_hbpt_*`, angesehen): abgeschnitten waren der Hinweis zum Stift, "Layer down" am Rand, die
  Deckkraft "100 %" und im Tschechischen die Mischart "Normální" -> Hinweis bricht um, Ebenen-Knoepfe fliessen in Zeilen, Deckkraft
  in eigene Zeile, wenn sie nicht passt, Mischart-Feld so breit wie der laengste Name; Bilder neu (`zs_hb3pt_*`, en/cs angesehen).
- Movie-Duels-Spielvergleich (`ptspiel` -> Mod mit 7262 roten Texeln in trooper-armor.jpg, `ptspiel.py`, eigener fs_homepath,
  Fotos 1920x1440 in `Y:\werkzeuge\zs\ptspiel_lauf`, Spielordner vorher/nachher gleich): das Spiel laedt das bemalte Bild (rote Kante
  am Hals und an der Huefte sichtbar). Das Vorschau-Foto der Gruppe ptspiel zeigt den Streifen nicht (Brust unter der Auswahl,
  andere Haltung) - Paarbild Spiel gegen Vorschau an derselben Stelle offen.
- Offen wie oben: Bildzeit (ptzeit, Schlussrunde ZUS), GPU-Malen.

## Changelog

### English
- Model tab: texture painting - a new area Painting paints directly on the images behind the shaders of a model, on the model in 3D (also while the animation plays) and flat in the image beside the view; the preview shows the whole shader as in the game.
- Tools Draw, Soften, Smear, Clone, Fill, Mask and Erase; brush with radius, strength, hardness, spacing and pen pressure; mirror painting, back faces on/off, bleeding over the UV borders so the seams stay closed.
- Layers above the untouched original with opacity, blend mode and visibility, a mask, undo per stroke; "Bake to new UVs" carries the painting onto changed UVs.
- "Build for the game" writes only the painted images under their game path, in the size and format of the original (TGA depth and compression, JPEG with the original tables, PNG); menu Painting, right-click, action search, tooltips and Blender keys.

### Deutsch
- Model-Tab: Texturen bemalen - ein neuer Bereich Painting malt direkt auf die Bilder hinter den Shadern eines Modells, auf dem Modell in 3D (auch während die Animation läuft) und flach im Bild neben der Ansicht; die Vorschau zeigt den ganzen Shader wie im Spiel.
- Werkzeuge Malen, Weichzeichnen, Verwischen, Klonen, Füllen, Maske und Radieren; Pinsel mit Radius, Stärke, Härte, Abstand und Stiftdruck; gespiegelt malen, Rückseiten an/aus, Ausbluten über die UV-Grenzen, damit die Nähte geschlossen bleiben.
- Ebenen über dem unberührten Original mit Deckkraft, Mischart und Sichtbarkeit, eine Maske, Rückgängig je Strich; „Auf neue UV umbacken“ bringt die Bemalung auf geänderte UV.
- „Für das Spiel bauen“ schreibt nur die bemalten Bilder unter ihrem Spielpfad, in Größe und Format des Originals (TGA-Tiefe und Kompression, JPEG mit den Tabellen des Originals, PNG); Menü Painting, Rechtsklick, Aktionssuche, Tooltips und Blender-Tasten.

### 中文
- 模型选项卡: 纹理绘制 - 新的“绘制”区域可直接在模型着色器背后的图像上绘制, 既可在 3D 模型上 (动画播放时也可以), 也可在视图旁的平面图像中; 预览像游戏中一样显示整个着色器。
- 工具: 绘制、柔化、涂抹、克隆、填充、遮罩和擦除; 画笔具有半径、强度、硬度、间距和笔压; 镜像绘制、背面开/关、跨 UV 边界出血, 使接缝保持闭合。
- 在未改动的原图之上的图层, 带不透明度、混合模式和可见性, 一个遮罩, 每笔一步撤销; “烘焙到新 UV”将绘制转到更改后的 UV。
- “为游戏构建”只按游戏路径写入已绘制的图像, 保持原图的大小和格式 (TGA 位深和压缩、使用原图量化表的 JPEG、PNG); 绘制菜单、右键菜单、操作搜索、提示和 Blender 按键。

### 日本語
- モデルタブ: テクスチャペイント - 新しい「ペイント」領域でモデルのシェーダーの背後にある画像に直接描けます。3D のモデル上 (アニメーション再生中も) とビュー横の平面画像の両方で描け、プレビューはゲームと同じくシェーダー全体を表示します。
- ツールはドロー、ぼかし、スミア、クローン、塗りつぶし、マスク、消しゴム。ブラシは半径、強さ、硬さ、間隔、筆圧に対応。ミラーペイント、裏面のオン/オフ、UV 境界を越えるにじみでシームが閉じたままになります。
- 変更されないオリジナルの上のレイヤー (不透明度、描画モード、表示)、マスク、ストローク単位の取り消し。「新しい UV に焼き付け」で変更後の UV にペイントを移せます。
- 「ゲーム用にビルド」は塗った画像だけをゲーム内のパスで、オリジナルのサイズと形式 (TGA の深度と圧縮、オリジナルのテーブルを使う JPEG、PNG) で書き出します。ペイントメニュー、右クリック、アクション検索、ツールチップ、Blender キーに対応。

### Español
- Pestaña Model: pintar texturas - una nueva área Pintura pinta directamente en las imágenes detrás de los shaders de un modelo, sobre el modelo en 3D (también mientras se reproduce la animación) y plano en la imagen junto a la vista; la vista previa muestra el shader completo como en el juego.
- Herramientas Dibujar, Suavizar, Difuminar, Clonar, Rellenar, Máscara y Borrar; pincel con radio, fuerza, dureza, espaciado y presión del lápiz; pintura reflejada, caras traseras sí/no y sangrado sobre los bordes UV para que las costuras sigan cerradas.
- Capas sobre el original intacto con opacidad, modo de fusión y visibilidad, una máscara y deshacer por trazo; «Hornear a las UV nuevas» lleva la pintura a las UV cambiadas.
- «Construir para el juego» escribe solo las imágenes pintadas con su ruta del juego, con el tamaño y formato del original (profundidad y compresión TGA, JPEG con las tablas del original, PNG); menú Pintura, clic derecho, búsqueda de acciones, descripciones y teclas de Blender.

### Čeština
- Karta Model: malování textur - nová oblast Malování maluje přímo na obrázky za shadery modelu, na modelu ve 3D (i když běží animace) i naplocho v obrázku vedle pohledu; náhled ukazuje celý shader jako ve hře.
- Nástroje Kreslit, Změkčit, Rozmazat, Klonovat, Vyplnit, Maska a Gumovat; štětec s poloměrem, silou, tvrdostí, rozestupem a přítlakem pera; zrcadlové malování, odvrácené plochy zap/vyp a přesah přes hranice UV, aby švy zůstaly zavřené.
- Vrstvy nad nedotčeným originálem s krytím, režimem prolnutí a viditelností, maska a krok zpět po každém tahu; „Přepéct na nové UV“ přenese malbu na změněné UV.
- „Sestavit pro hru“ zapíše jen namalované obrázky pod jejich herní cestou ve velikosti a formátu originálu (hloubka a komprese TGA, JPEG s tabulkami originálu, PNG); menu Malování, pravé tlačítko, hledání akcí, nápovědy a klávesy Blenderu.
