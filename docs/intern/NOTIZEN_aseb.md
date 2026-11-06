# Notizen ASEB: .ase, .md3 und weitere Modellformate zu Brushes

Auftrag `auftrag_ase_brush.md` (Tester GP, Discord: „This update now have .ase to brush or not?“; shank 06.11.: „kannst du das nach stufe 3 einfügen bitte das ase auch geht“). Direkt in `Y:\karte` auf `karte1` ab 7904943c9 (Modell-Block Stufe 3b), Build `build-test`, Läufe `laeufe/aseb_*` (rot `aseb_rot`, grün `aseb_g1` bis `aseb_g3`, Bilder `aseb_hb_<sprache>`, Schluss `asebe_*`), Laufskript `lauf.py`. kFassung bleibt rc610. Kein Spiel gestartet, Spielordner nur gelesen.

## Rot

- `aseb_rot` (Selbsttest `karteneditor`, `BHED_KARTE_NUR=aseb`, neuer Test auf altem Programm): 15 OK, 19 FEHLER - „Cannot read …truhe.ase: unsupported format .ase“, ebenso .md3 und .glm („unsupported format .glm“ statt eines Grundes), kein Eintrag im Rechtsklick-Menü und unter File, der Helfer fand „ase to brush“ nicht.

## Umsetzung

- **Laden wie q3map2** (`modellWieQ3map2`, `src/karte/modell_assimp.cpp`): q3map2 (NRC 20260114) lädt misc_model nicht mehr über picomodel, sondern über assimp - auch .ase und .md3 (`tools/quake3/q3map2/model.cpp` LoadModel :297-340, Einstellungen :165-181). Dieselben Einstellungen (MD3 ohne Shader-Dateien und ohne Multipart, Tangenten weg, SplitLargeMeshes 999, Frame 0), dieselben Schritte, dieselbe Drehung Y oben -> Z oben; Shadername wie `AssModelMesh` (:200-238): Materialname, wenn er mit `textures/`/`models/` beginnt, sonst das Diffuse-Bild (`*BITMAP`), ohne Endung; ein Name ohne `/` kommt in den Ordner des Modells, ein absoluter Pfad oder einer mit `..` wird ab `/models/` bzw. `/textures/` gekürzt. `assimpLesen` (NRCs Editor) und `modellWieQ3map2` teilen den Code; `assimpLesen` hat jetzt auch die MD3/MDL-Einstellungen von `pico_initialise` (assmodel/plugin.cpp:197-209).
  - Abweichung vom Auftragstext: dort steht picomodel `pm_ase.c`; in der Quelle (NRC 20260114) nimmt q3map2 assimp. Belegt durch die Gegenprobe `-bhedmodels` (siehe Tests).
- **Geometrie zu Brushes** (`gui/app_karte_geoimport.cpp`): .obj/.fbx/.glb/.gltf wie bisher (Vorsatz, Y oben); .ase, .md3 und jede weitere Endung, die q3map2 liest (`modellEndungenAssimp`, wie der Modell-Browser), über `modellWieQ3map2`. Der VFS-Name der Datei (Wurzel der Shadernamen): relativ zu einem Ordner des Suchpfads, sonst ab `models/`, sonst nur der Dateiname. Im Fenster statt Vorsatz/Y oben der Hinweis „Shader so, wie q3map2 dieses Modell einbackt“ mit Tooltip; Maßstab bleibt. `.glm`: Meldung „Ghoul2-Modelle (.glm) sind animierte Figuren, keine Kartengeometrie …“.
- **Körper statt Flächen** (`nachKoerpern`, `src/karte/geometrieimport.cpp`): q3map2 liefert je Shader eine Fläche; ein Kasten mit Multi/Sub-Object-Material wären zwei offene Teile (dünne Prismen). Die Flächen werden über Punkte gleicher Lage zu Körpern verbunden, je Körper ein Teil mit Shader je Dreieck (`dreieckShader`); die Brush-Seiten nehmen den Shader des Dreiecks, das sie überdeckt; die Modell+Clip-.obj wechselt `usemtl` je Dreieck.
- **Modell in Brushes umwandeln** (eigene Funktion, nicht in NetRadiant-Custom - dort gibt es nichts dergleichen, gesucht in radiant/, plugins/, contrib/): oben im Rechtsklick-Menü der Ansichten (nur mit gewähltem misc_model), unter File nach „Geometrie als Brushes importieren…“, mit Tooltip; Helfer-Wörter „ase to brush“, „md3 to brush“, „misc_model in Brushes umwandeln“ (6 Sprachen). Das Modell wie q3map2 (`modellWieQ3map2` aus dem VFS, auch .pk3), Skin und `_remap` wie InsertModel (`kartenModellShader`, dieselbe Funktion wie die Anzeige), Lage `miscModellMatrix` (origin, angles, modelscale/modelscale_vec); gespiegelt dreht `netzVonModell` den Umlauf wie InsertModel (model.cpp:1216, 1430-1450). Gleiches Fenster; Einfügen entfernt die misc_model in derselben Undo-Stufe `importGeometry`.
  - Die Karten-Editor-Oberfläche hat keine „Alle Aktionen suchen“ (die gibt es nur im Model-Tab); dort finden Helfer, Menü, Rechtsklick und Tooltip.
- Fenster: die Spalte Teil so breit wie der längste Name (höchstens 40 %), mit Tooltip - im Japanischen schnitt die alte Dehnung „1: holz (+1)“ ab (Handbuchbild angesehen, behoben, neu aufgenommen).
- Handbuch (6 Sprachen): Werkzeuge 9, Abschnitt Geometrie zu Brushes: Absatz Formate und Shadernamen, Bild `h_aseb_fenster`, Absatz Umwandeln, Bild `h_aseb_menue` (`tools/handbuch/inhalt_aseb.py`); Tutorial k-geometrie: Tipp „Spielmodelle“ neu (.ase/.md3, Rechtsklick). Texte `tools/i18n_aseb.py`, `KeGiMenueTip` in allen Sprachen um .ase/.md3 ergänzt.
  - Das Rechtsklick-Menü listet alle Entity-Klassen, ist höher als das Fenster und steht darum in einem eigenen ImGui-Viewport, den das Foto des Programmfensters nicht zeigt. Das Bild zeigt denselben Eintrag unter File; die Bildunterschrift sagt das.

## Tests

- Selbsttest `gui/selbsttest_aseb.inc` (`BHED_KARTE_NUR=aseb`, auch im vollen `karteneditor` nach MODOBJ; Bilder `BHED_EDITORTEST=handbuch BHED_HB_NUR=aseb`). Eigene Dateien im Laufordner (Extra Resource Path): `truhe.ase` wie aus 3ds Max (Multimaterial holz mit absolutem `*BITMAP` bis `base\textures`, `textures/aseb/metall` als Materialname, griff mit Bild ohne Ordner; zwei geschlossene Kästen), `stein.md3` (zwei Flächen, eine mit Shader ohne Ordner), `figur.glm`.
  - Gegenprobe `bhed_q3map2 -bhedmodels`: truhe `models/aseb/griff 12, textures/aseb/holz 8, textures/aseb/metall 4`, stein `models/aseb/kante 2, textures/aseb/stein 10`.
  - Grün (`aseb_g3`, 27/0): .ase 2 Teile (Truhe, Griff) geschlossen, Dreiecke je Shader und Seiten-Shader = q3map2, eingefügt 2 Brushes, Hüllkasten (-32 -28 0)-(32 24 32) = q3map2, Abweichung 0; .md3 1 Körper, = q3map2 (auch `models/aseb/kante`); .glm Meldung; Rechtsklick auf misc_model mit angles 0 90 0 und modelscale_vec -1 1 1: Eintrag da, Tooltip, Fenster mit 2 Teilen, Brushes (232 96 0)-(284 160 32) = Soll, Abweichung 0, misc_model weg, ein Strg+Z: keine Brushes, misc_model mit denselben Schlüsseln; File-Menü: Eintrag, Fenster, Abbrechen ändert nichts; Helfer „ase to brush“ -> tour:k-geometrie an erster Stelle.
  - Auf dem Weg: `aseb_g1` 18/15 und `aseb_g2` 20/11 waren Testfehler (Neue Karte fragte nach dem Speichern; der Optimierer nach dem Einfügen verdeckte die 2D-Ansicht).
- ctest `hilfe`: neue Fragen „ase to brush“ (en) und „misc_model in Brushes umwandeln“ (de).

## Schluss

- Sauberer Neubau (`--clean-first`, alle Ziele, `aseb_neubau.txt`, 11 min): 0 Warnungen, 0 Fehler.
- ctest -j 16 (`asebe_ctest.txt`): 225 / 225 (mit `hilfe` und den neuen Fragen, `karte_obj`, `karte_modell`, `handbuch_sprachen`).
- Selbsttests parallel (`asebe_*`): `karteneditor` aseb 27/0, tutgeo 23/0, modobj 25/0, stufe9 84/0, fotos 77/1 - „Menue File: 342 x 381 wie NRC“, hier 405: der neue Eintrag unter File ist eine Zeile mehr; die Erwartung (`selbsttest_karte3bis5.inc`, schon um die eigenen File-Einträge ergänzt) um 24 erhöht, nach dem Bau (0 Warnungen) `asebe_fotos2` 78/0. Modus 1 2635/0.
- Handbuchbilder `aseb_hb_<sprache>` je 9/0 (`BHED_HB_FENSTER=2560x1440@1.5`, parallel), 2538 x 1384, angesehen (en, ja, cs, de). `gen_handbuch.py --pruefen`: es/cs 0 Texte, 0 Bilder englisch, `handbuch_sprachen` ok; `src/hilfe_orte.inc` unverändert.
- Lints (`asebe_lint.txt`): wie vorher - ok bis auf portability 99, pruefcodes 11, efxflags, fremdprogramm (alle schon vor ASEB); link/tidy brauchen Linux.
- Gegen das Vorbild geprüft: q3map2 selbst (`-bhedmodels`, dieselbe LoadModel-Funktion): Shader und Dreiecke je Shader, Hüllkasten der Punkte. Nicht geprüft: ein Bau der Karte mit den Brushes und ein Spielvergleich (kein Spielstart), eine echte .ase aus 3ds Max (die Testdatei folgt dem Format des Exports).

## Eigene Annahmen

- Datei außerhalb des Suchpfads: Wurzel der Shadernamen ab `models/` im Pfad, sonst ohne Ordner (q3map2 kennt nur VFS-Pfade).
- Körper über Punkte gleicher Lage (1/1000 wie die übrige Zerlegung); Name „Nummer: häufigster Shader (+weitere)“.
- Rechtsklick nur für `misc_model` (das backt q3map2 ein); `misc_model_static`/`_breakable` lädt das Spiel zur Laufzeit.

## Offen

- Die Modell+Clip-.obj im Import-Ordner nennt die Shader per `usemtl` ohne `.mtl` (wie bisher bei .obj aus Blender); ob q3map2 diese Namen ohne Materialdatei übernimmt, ist nicht geprüft.
- Die Tutorial-Bilder `h_tut_geo_*` zeigen die alte Spaltenbreite der Teile-Liste (Inhalt gleich, nicht neu aufgenommen).
- Im Bild `h_aseb_menue` steht kein Tooltip (die Maus über dem Eintrag zeigte ihn im Foto nicht); der Selbsttest prüft den Tooltip im Rechtsklick-Menü.

## Changelog

### English
- Map editor: **File > Import geometry as brushes...** also reads .ase (3ds Max), .md3 and every other model format q3map2 reads. The brushes get the shader names q3map2 bakes for the model as misc_model (material name or *BITMAP path, cut from textures/ or models/), and the surfaces of one closed body stay one part, so a box with a multi-material becomes solid brushes. Ghoul2 models (.glm) get a clear message.
- Map editor: right-click on a selected misc_model (also from a .pk3) > **Convert model to brushes...** (also in File) turns it into brushes in the same place - angles, modelscale and mirrored models included, _remap and skin like q3map2 - and removes the misc_model, in one undo step.

### Deutsch
- Karten-Editor: **Datei > Geometrie als Brushes importieren...** liest auch .ase (3ds Max), .md3 und jedes weitere Modellformat, das q3map2 liest. Die Brushes bekommen die Shadernamen, die q3map2 dem Modell als misc_model einbackt (Materialname oder *BITMAP-Pfad, ab textures/ oder models/), und die Flächen eines geschlossenen Körpers bleiben ein Teil - ein Kasten mit Multimaterial wird so feste Brushes. Ghoul2-Modelle (.glm) bekommen eine klare Meldung.
- Karten-Editor: Rechtsklick auf ein gewähltes misc_model (auch aus einer .pk3) > **Modell in Brushes umwandeln...** (auch unter Datei) macht Brushes an derselben Stelle daraus - mit Winkeln, modelscale und gespiegelten Modellen, _remap und Skin wie bei q3map2 - und entfernt das misc_model, in einem Undo-Schritt.

### 中文
- 地图编辑器:**文件 > 将几何体导入为画刷...** 现在也能读取 .ase(3ds Max)、.md3 以及 q3map2 能读取的其他所有模型格式。画刷获得 q3map2 把该模型作为 misc_model 烘焙时的着色器名(材质名或 *BITMAP 路径,从 textures/ 或 models/ 截取),同一封闭物体的各个面仍是一个部件,因此带多重材质的箱子会变成实心画刷。Ghoul2 模型(.glm)会给出明确提示。
- 地图编辑器:在选中的 misc_model(也可以来自 .pk3)上单击右键 > **将模型转换为画刷...**(文件菜单中也有),会在同一位置把它变成画刷——包括角度、modelscale 和镜像的模型,_remap 和皮肤与 q3map2 相同——并删除该 misc_model,一步撤销。

### 日本語
- マップエディター: **ファイル > ジオメトリをブラシとして読み込む...** で .ase(3ds Max)、.md3、そして q3map2 が読めるほかのすべてのモデル形式も読めるようになりました。ブラシには q3map2 がそのモデルを misc_model として焼き込むときのシェーダー名(マテリアル名か *BITMAP のパス。textures/ か models/ から)が付き、閉じた 1 つの物体の面は 1 つの部品のままなので、マルチマテリアルの箱も固体のブラシになります。Ghoul2 モデル(.glm)ははっきりしたメッセージを表示します。
- マップエディター: 選択した misc_model(.pk3 内のものも可)を右クリック > **モデルをブラシに変換...**(ファイルメニューにもあります)で、同じ位置のブラシにします。角度、modelscale、ミラーしたモデルも含み、_remap とスキンは q3map2 と同じです。misc_model は削除され、元に戻すは 1 回です。

### Español
- Editor de mapas: **Archivo > Importar geometría como brushes...** lee también .ase (3ds Max), .md3 y cualquier otro formato de modelo que lea q3map2. Los brushes reciben los nombres de shader que q3map2 integra al modelo como misc_model (nombre del material o ruta de *BITMAP, desde textures/ o models/), y las superficies de un cuerpo cerrado siguen siendo una sola parte, así que una caja con multimaterial se convierte en brushes sólidos. Los modelos Ghoul2 (.glm) reciben un mensaje claro.
- Editor de mapas: clic derecho sobre un misc_model seleccionado (también de un .pk3) > **Convertir modelo en brushes...** (también en Archivo) lo convierte en brushes en el mismo sitio, con ángulos, modelscale y modelos reflejados, _remap y skin como en q3map2, y elimina el misc_model, en un solo paso de deshacer.

### Čeština
- Editor map: **Soubor > Importovat geometrii jako brushe...** čte také .ase (3ds Max), .md3 a každý další formát modelu, který čte q3map2. Brushe dostanou názvy shaderů, které q3map2 modelu jako misc_model zapéká (název materiálu nebo cesta *BITMAP, od textures/ nebo models/), a plochy jednoho uzavřeného tělesa zůstanou jednou částí - bedna s multimateriálem se tak stane pevnými brushi. Modely Ghoul2 (.glm) dostanou jasnou zprávu.
- Editor map: pravé tlačítko na vybraném misc_model (i z .pk3) > **Převést model na brushe...** (také v nabídce Soubor) z něj udělá brushe na stejném místě - včetně úhlů, modelscale a zrcadlených modelů, _remap a skin jako v q3map2 - a misc_model odstraní, v jednom kroku zpět.
