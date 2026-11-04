# Notizen rc609 K

## K: Texturen der Karte - Übersicht und Ersetzen (Karten-Editor)

Auftrag `auftrag_rc609_k.md` (Plan `plaene/auftraege_naechster_release.md`, Abschnitt K; Wunsch eines Testers vom 04.11.: "the program shows which textures we use on the map and maybe we can fully replace the textures on brushes, patches to another from another folder"). Worktree `Y:\texturen`, Zweig `rc609k`, Build `build-texturen`. Skripte in `<SP neu>/k`.

### 1: Logik (`src/karte/texturliste.cpp`, `include/bhed/karte/texturliste.h`)
- `texturListe`: alle benutzten Shader mit Brush-Seiten, Brushes und Patches, ohne Groß/klein zusammengefasst (erste Schreibweise), sortiert wie `shader_less_t`.
- Umfang `Alle` (auch Verstecktes), `Auswahl` (gewählte Brushes, Patches, Entities als Ganzes und gewählte Flächen), `Sichtbar` (Entity und Primitiv sichtbar, Fläche nicht vom Flächenfilter verborgen). Wählen und Ersetzen benutzen denselben Umfang.
- `texturListeWaehlen`: Auswahl leeren (Strg: dazu), alle Brush-Seiten (FaceSelectByShader) und Patches (PatchSelectByShader) im Umfang wählen.
- `texturListeErsetzen`: beliebig viele Paare in einem Rückgängig-Schritt `textureListReplace ...`; ohne Treffer kein Schritt; ungültige Namen (texdef_name_valid) übergangen. Setzen über `Flaeche::setzeShader` (Face::SetShader) und `Patch::setzeShader`.
- Gleiche Weltgröße: q3map2 nimmt bei Quake- und Valve-220-Texdefs `vecs / scale` und `shift` (`map.cpp` QuakeTextureVecs, ParseRawBrush) und teilt durch die Bildgröße (`surface.cpp:725`). Darum `scale *= alt/neu`, `shift *= neu/alt`. Brush Primitives stehen in der .map normiert und bleiben unverändert (ohne den Schalter rechnet setzeShader sie wie NRC mit removeScale/addScale um). Patches haben normierte st; ihre Weltgröße hängt nicht an der Bildgröße und bleibt in beiden Fällen.
- `texturOrdnerZuordnung`: zu jedem Shader unter dem alten Ordner der gleichnamige unter dem neuen (Unterordner bleiben); fehlt er (weder Shaderdefinition noch Bild), ist er eine Lücke und bleibt beim Ersetzen.

### 2: Fenster (`gui/app_karte_texturliste.cpp`)
- Panel `karte.texliste` ohne Platz im Hauptfenster (wie J: eigenes andockbares Fenster, Einstellung `texListeOffen`). Geöffnet über **Misc > Texturen der Karte...** und im Texturbrowser **View** (Listeneintrag `ProgrammTexturen`, nicht in NRC).
- Tabelle sortier- und filterbar: Bild (das Bild der Kamera, `texturBild`), Shader, Brush-Seiten, Brushes, Patches, Größe; "(Bild fehlt)" wie das Ersatzbild der Kamera. Liste neu nur bei geändertem `szeneStand()` oder Umfang.
- Klick in eine Zeile wählt (Strg: dazu). "Ersetzen durch" mit Feld und "Aus dem Texturbrowser" (TextureBrowser_GetSelectedShader, jeder Ordner), Warnung, wenn es den Namen im Spiel nicht gibt. "Ordner ersetzen" mit "Ordner des Texturbrowsers", "Zuordnung zeigen" (Liste mit Lücken) und "n Texturen ersetzen".
- "Find / Replace Texture(s)" bleibt 1:1 NRC. Abweichung mit Absicht: NRC ersetzt auf allen Brushes, aber nur sichtbaren Patches (Scene_BrushFindReplaceShader / Scene_PatchFindReplaceShader) - das neue Fenster behandelt beide gleich (Memory: keine NRC-Fehler übernehmen).

### Tests
- ctest `texturliste` (neu, `tests/texturlistetest.cpp`): rot (Logik leer) 18 FEHLER, grün 29 OK / 0 FEHLER.
- Selbsttest `texturliste` (neu, `gui/selbsttest_texturliste.inc`): rot 2 OK / 7 FEHLER (`k_rot`), grün 8 OK / 0 FEHLER (`k_gruen3`, Schlussrunde `k_s3_tl`).
- Handbuch: Abschnitt "Texturen der Karte" im Karten-Editor, Bild `h_texturliste` in 6 Sprachen (Foto 3818 x 2104 bei Fenster 3840 x 2160 @ 2, Handbuchbild 2560 x 1411), `gen_handbuch.py --pruefen` ohne fehlende Bilder. Beim Ansehen nachgebessert: Marke 1 deckte die Spaltenköpfe (jetzt genau die Zeile: Bild 2 x Schrift + Zellabstand), Marke 2 war breiter als der Knopf und ragte über den Fensterrand (jetzt Beschriftung + Rahmenabstand, am Fenster geklemmt), Marke 3 deckte den Kopf der Zuordnung.
- Schlussrunde: sauberer Neubau 0 Warnungen. ctest 203 von 203 grün. `karteneditor` mit `BHED_KARTE_NUR=fotos` 78 OK / 0 FEHLER. lint_sources, lint_i18n, lint_i18n_erzeugt, lint_settings, lint_headers, lint_feldinit ok.
  - `karteneditor`: "Menue Misc: 262 x 257 wie NRC" war rot (hier 288): genau Trenner 7 + Zeile 24 des neuen Eintrags, also der Stand von 54ade1d8 plus K. Sollhöhe im Test nachgezogen (Abweichung wie die übrigen Programm-Einträge).
  - `karte_lizenz` war rot: texturliste.cpp/.h ohne Herkunftskopf. Jetzt mit den portierten Stellen (NRC brushmanip.cpp, patchmanip.cpp, brush.h, shaderlib.h; q3map2 map.cpp, surface.cpp) und der GtkRadiant-Copyright-Zeile.
  - `karte_befehle` war rot (99 x "Englisch = NRC: "" -> Map textures"): veraltete Objektdatei im eigenen Build (Str::Count des alten i18n.h); nach dem sauberen Neubau grün, kein Fehler im Code.

### Offen
- Übersetzungsteil von lint_headers übersprungen: kein g++/clang++/cl im Suchpfad (nur clang-cl in llvm-23.1.4).
- Keine Bildzeit-Messung (nur allein, Hauptchat).
- "Gleiche Weltgröße" nur über die q3map2-Formel im ctest geprüft, nicht an den st-Werten einer gebauten Karte und nicht im Spiel.

## English
- Map editor: new window "Map textures" (Misc > Map textures..., also in the texture browser's View menu): every shader and texture of the map with image, brush sides, brushes, patches and image size, sortable and filterable. A click selects all faces and patches with it; "Replace with" takes a name or the texture selected in the texture browser (any folder); "Replace folder" swaps every texture of one folder for the one with the same name in another and lists the gaps first. Brushes and patches change together in one undo step, for the whole map, the selection or only what is visible; "Keep world size" rescales brush sides to a larger or smaller image.

## Deutsch
- Karten-Editor: neues Fenster "Texturen der Karte" (Misc > Texturen der Karte..., auch im Menü View des Texturbrowsers): jeder Shader und jede Textur der Karte mit Bild, Brush-Seiten, Brushes, Patches und Bildgröße, sortier- und filterbar. Ein Klick wählt alle Flächen und Patches damit; "Ersetzen durch" nimmt einen Namen oder die im Texturbrowser gewählte Textur (jeder Ordner); "Ordner ersetzen" tauscht jede Textur eines Ordners gegen die gleichnamige eines anderen und listet vorher die Lücken. Brushes und Patches ändern sich zusammen in einem Rückgängig-Schritt, für die ganze Karte, die Auswahl oder nur das Sichtbare; "Gleiche Weltgröße" skaliert Brush-Seiten auf ein größeres oder kleineres Bild.

## 中文
- 地图编辑器:新增窗口“地图纹理”(Misc > 地图纹理...,也位于纹理浏览器的 View 菜单):列出地图的每个着色器和纹理及其图像、画刷面、画刷、面片数量和图像尺寸,可排序和筛选。单击会选中使用它的所有面和面片;“替换为”可输入名称或采用纹理浏览器中选中的纹理(任意文件夹);“替换文件夹”把一个文件夹的每个纹理换成另一个文件夹中的同名纹理,并先列出缺口。画刷和面片在一个撤销步骤内一起更改,范围可以是整个地图、选中内容或仅可见内容;“保持世界尺寸”会按更大或更小的图像重新缩放画刷面。

## 日本語
- マップエディター: 新しいウィンドウ「マップのテクスチャ」(Misc > マップのテクスチャ...、テクスチャブラウザーの View メニューにもあります)。マップのすべてのシェーダーとテクスチャを、画像、ブラシ面・ブラシ・パッチの数、画像サイズとともに表示し、並べ替えと絞り込みができます。クリックでそれを使うすべての面とパッチを選択。「置き換え先」には名前か、テクスチャブラウザーで選んだテクスチャ (どのフォルダーからでも) を使います。「フォルダーを置き換え」は、あるフォルダーの各テクスチャを別のフォルダーの同名のテクスチャに替え、先に欠けを一覧表示します。ブラシとパッチは 1 回の元に戻す操作で一緒に変わり、範囲はマップ全体、選択、表示中のみから選べます。「ワールドサイズを保つ」は大きい (小さい) 画像に合わせてブラシ面を拡大縮小します。

## Español
- Editor de mapas: nueva ventana "Texturas del mapa" (Misc > Texturas del mapa..., también en el menú View del explorador de texturas): cada shader y textura del mapa con imagen, caras de brush, brushes, patches y tamaño de imagen, ordenable y filtrable. Un clic selecciona todas las caras y patches que la usan; "Sustituir por" toma un nombre o la textura elegida en el explorador de texturas (cualquier carpeta); "Sustituir carpeta" cambia cada textura de una carpeta por la del mismo nombre en otra y antes muestra los huecos. Brushes y patches cambian juntos en un solo paso de deshacer, para el mapa entero, la selección o solo lo visible; "Mantener el tamaño en el mundo" reescala las caras de brush a una imagen mayor o menor.

## Čeština
- Editor map: nové okno "Textury mapy" (Misc > Textury mapy..., také v nabídce View prohlížeče textur): každý shader a textura mapy s obrázkem, počtem stran brushů, brushů, patchů a velikostí obrázku, s řazením a filtrem. Klik vybere všechny plochy a patche s ní; "Nahradit" vezme název nebo texturu vybranou v prohlížeči textur (libovolná složka); "Nahradit složku" vymění každou texturu jedné složky za stejnojmennou z jiné a předem vypíše mezery. Brushe a patche se mění společně v jednom kroku zpět, pro celou mapu, výběr nebo jen viditelné; "Zachovat velikost ve světě" přeškáluje strany brushů na větší nebo menší obrázek.
