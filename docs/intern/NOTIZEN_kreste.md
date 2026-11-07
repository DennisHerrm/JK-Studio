# Notizen p_karte (kreste): offene Punkte Karten-Editor

Auftrag `auftrag_p_karte.md` (paralleles Fenster 07.11. 19-23 Uhr). Worktree `Y:\kreste`, Zweig `kreste1` ab e6daa5ca8 (Modell-Block 1d), Build `Y:/kreste/build-kreste` (wie `build-test`, `--parallel 4`), Läufe `laeufe/kr_*`, Laufskript `lauf_kreste.py`. kFassung bleibt rc610. Keine Bildzeit-Messung. Spiel: nur Movie Duels (eigener fs_homepath).

Selbsttest neu: `BHED_EDITORTEST=karteneditor BHED_KARTE_NUR=kr` (`gui/selbsttest_kreste.inc`, auch im vollen karteneditor-Lauf nach ASEB); Teil 7 in `teil6` um ein zweites fx_lava erweitert.

## 7. ctest q3map2_pr316: wer den Datenordner geleert hat

- Befund: Nicht der Test und kein Agent. Der Datenordner liegt unter `%TEMP%` (`<altesSP>\s6\pr316`). Die Windows-Speicheroptimierung (Storage Sense: `HKCU\...\StorageSense\Parameters\StoragePolicy` 01 = 1 an, 04 = 1 temporäre Dateien, 2048 = 0 „bei wenig Speicherplatz“; C: 357 GB frei) hat am 07.11. um 14:09:56-57 in allen Ordnern des alten Scratchpads Dateien gelöscht - nicht nur hier: alle `scripts`-Ordner von pr316 (gameroot und die Fälle q3map2/*), `bt-el2.txt` (leer) in jeder Kopie von `data/karte/bt` unter `ab/` und `d4hb/`. Dieselben Spuren am 02.11. 16:51:46 (README.txt, .patch, generate.py, .ase der Fälle, .tga) und 06.11. 13:30. Zu allen drei Zeiten zeigt das Anwendungsprotokoll je 15 Einträge `Microsoft-Windows-AppModel-State` (14:08:05 am 07.11.) - das Muster des Aufräumlaufs. Kein Papierkorb-Eintrag, kein Werkzeugaufruf eines Agenten mit dem Pfad (Protokolle 11:00-13:30 UTC durchsucht), letzter grüner Lauf `rte_ctest.out` 13:03, `q3map2_pr316.py` liest die Quelle nur (Lauf vorher/nachher: alle 129 Dateien blobgleich).
- Wiederhergestellt aus auto-pigeon/q3map2 (PR #316, Kopf 143cbd61) auf dem Stand von `cases.json` (Blob 8acac0cb = Commit 6f297e47): 44 Dateien (gameroot `apregress.shader`, `shaderlist.txt`, die `scripts`, Modelle und Texturen der q3map2-Fälle, READMEs); alle 129 Dateien blobgleich. Zusätzlich eine Kopie außerhalb von `%TEMP%`: `Y:\werkzeuge\pr316\regression_tests`.
- Ursache behoben im Test: `tools/karte/q3map2_pr316_dateien.json` (Pfad -> Git-Blob-SHA, 129 Dateien) und `--dateien` im ctest: fehlt oder weicht eine Datei ab, endet der Test mit „FEHLER Datenordner unvollständig“ und nennt die Dateien, statt mit Proben-Fehlschlägen. CMake-Kommentar: Ordner nicht unter `%TEMP%` ablegen.
- Rot: alte Prüfung mit leerem `scripts` (Kopie): `ANDERS R10.brushprim/classic/valve220`, 3 Abweichungen (wie `uve_ctest`/`md_e_ctest`). Neue Prüfung: „DATEN fehlt … apregress.shader, shaderlist.txt“, Rückgabe 1. Grün: beide Datenorte pass 65, fail 15, known_failure 1, 0 Abweichungen.
- Wichtig: Dieselbe Speicheroptimierung kann alle Dateien unter `%TEMP%` treffen - das alte Scratchpad mit `ab/` (Laufskripte), `m3/ojq`, `mdgit`, `ojk/oj`, `nrc_20260114`, `jascripts` und die Scratchpads der Chats. Vorschlag an shank: Speicheroptimierung für temporäre Dateien aus, oder diese Ordner nach `Downloads\behaved-1_0_0-rc567\wp` ziehen.

## 1. ASEB im echten Bau (Selbsttest `kr`)

Je Modell drei Karten im selben Raum, gebaut mit bhed_q3map2 `-meta` (Build-Eintrag des Tests): `_modell` (misc_model, die Vorgabe), `_brush` (Rechtsklick „Modell in Brushes umwandeln“, Auto), `_clip` (alle Teile Modell+Clip); verglichen in der .bsp: Shader, Dreiecke je Shader und Hüllkasten der Modellflächen. Modelle: `truhe.ase` (ASEB, wie ein Max-Export: Multimaterial, ein Bild ohne Ordner -> `models/aseb/griff`), `punkt.ase` (Bild `stein.v2.tga`) und die echte Max-Datei `models/map_objects/tatooine_outpost/to_chair.ase` aus `MD_Maps_Ep4.pk3`.

Rot (`kr_rot`, Stand e6daa5ca8 plus Test), zwei echte Fehler:
- **Brush-Seiten mit Shader außerhalb von `textures/`:** `truhe_brush` zeichnet `textures/eb/griff` statt `models/aseb/griff`. Der Editor schreibt Brush-Shader ohne die ersten 9 Zeichen (`shader_get_textureName`, wie NRC), q3map2 setzt `textures/` davor (map.cpp ParseRawBrush :1009). Brush-Seiten können im .map nur Shader unter `textures/` tragen.
- **Modell+Clip-.obj lag um 90 Grad gekippt:** `truhe_clip` ohne `models/aseb/griff` (die 12 Dreiecke lagen im Boden und fielen weg), Truhe in der .bsp bei y -32..0, z -24..24 statt y -24..24, z 0..32. q3map2 dreht jede assimp-Szene um +90 Grad um X (model.cpp LoadModel :323-329, Y oben -> Z oben); die .obj-Leser von assimp drehen nicht, `modellObj` schrieb aber Q3-Koordinaten. Betraf jede Modell+Clip-Karte seit Stufe 9 (auch die Anzeige im Editor, die dieselbe Drehung hat).

Behoben:
- `teilUmwandeln` (src/karte/geometrieimport.cpp): Auto macht einen Teil nur zu Brushes, wenn alle seine Shader unter `textures/` liegen (`shaderUnterTextures`), sonst Modell+Clip - dort behält die Modellfläche den Shader wie das misc_model. Handbuch-Absatz dazu (`tools/handbuch/inhalt_kreste.py`, 6 Sprachen). Der ASEB-Selbsttest erwartet jetzt Truhe = Brushes, Griff und `stein.md3` (Shader `models/aseb/kante`) = Modell+Clip.
- `modellObj`: Punkte als (x, z, -y) - die Drehung von q3map2 bringt sie wieder nach (x, y, z).

Grün (`kr_g1` 38/0): `truhe_brush` = `_modell` (Shader), `truhe_clip`, `punkt_clip`, `stuhl_clip` = `_modell` (Dreiecke je Shader und Hüllkasten, z. B. griff (-8 -28 10)-(8 -24 22) beide); `stuhl` (to_chair.ase: `textures/tatooine_outpost/metal_2` 68, `seatmaterial` 40) unter Auto Modell+Clip (offenes Netz), gleiche Shader. Ladetest: Schnell-Backen und „Gebaute Karte wie im Spiel“ mit dem umgewandelten Stuhl, Spielbild gezeigt (Foto `kr_g1/fotos/kr_stuhl_spielbild`). ASEB `kr_aseb` 27/0, TUT-GEO `kr_tutgeo` 23/0.
- Movie Duels (eigener fs_homepath, `+devmap`, Fenster 1920x1440 außer Sicht, stumm, kein Fokus): `kr_stuhl_modell` und `kr_stuhl_clip` (gebaut `-meta` + `-light -fast`) laden beide, der Stuhl steht aufrecht auf dem Boden; die Bilder unterscheiden sich im Mittel um 0,0009 (höchstens 6 von 255). Paarbild `laeufe/kr_md/kr_stuhl_paar.png` (1920 x 2880, ungeskaliert). Dunkel, weil ein Licht 600 mit `-fast`. Spielordner vorher/nachher gleich (0 neu, 0 weg, 0 geändert).

## 2. Modell+Clip-.obj ohne .mtl

- Ja, q3map2 übernimmt den `usemtl`-Namen: assimp legt für ein unbekanntes Material ein Material mit diesem Namen an (`ObjFileParser.cpp:596-605` getMaterialDesc, Meldung „OBJ: failed to locate material …, creating new material“ - nur eine Meldung), q3map2 nimmt den Materialnamen ohne Diffuse-Bild (model.cpp AssModelMesh :202-220). Gemessen mit `-bhedmodels` (`kr/obj_probe.py`) und im echten Bau (`_clip` oben).
- Zwei Regeln von q3map2 gelten dabei auch für .obj aus Blender: ein Name ohne `/` bekommt den Ordner des Modells davor (`griff` -> `models/kr/griff`, :221-223), und der letzte Punkt schneidet ab (`Material.001` -> `models/kr/Material`, PathExtensionless :219). Die .obj des Editors schreibt immer den vollen Shader mit Ordner (Vorsatz `textures/…`, ASEB: der Name von q3map2 selbst) - beides tritt dort nicht auf. Ein Shader mit Punkt im Namen ließ sich mit einer .ase nicht nachstellen (`punkt.ase`: q3map2 macht schon aus `stein.v2.tga` `textures/kr/stein`), darum nicht geändert.
- Die eigentliche Abweichung war die Drehung (Punkt 1).

## 3. Echte .ase aus 3ds Max

- Gesucht nur in Downloads, Dokumente, Steam „Jedi Academy“ und „Jedi Outcast“ (lose und in .pk3/.zip, `kr/ase_suche.py`): 98 Treffer, alle `*3DSMAX_ASCIIEXPORT`, 49 verschiedene in den Movie-Duels-Karten (MD_Maps_Canonical, Legends, Ep1, Ep2, Ep4, Ep5, Ep6; die zweite Hälfte in „GameData Movie Duels - Kopie“). Keine in Downloads.
- Geprüft mit `to_chair.ase` (MD_Maps_Ep4): Rechtsklick-Umwandeln (Brushes/Modell+Clip, Bau, Spiel, oben) und Datei-Import aus einer losen Kopie (Dreiecke je Shader = .bsp des misc_model). Shader weiterer Dateien wie q3map2: blue_barrel (blackmetal 44, bluebarrel 32), KamCover1 (`textures/amace_kamino/KamCover1` 84), sail_barge_bridge, uppertube.

## 4. Tutorialbilder `h_tut_geo_*` (und `h_aseb_*`)

- Neu aufgenommen: `h_tut_geo_1` bis `_6` und `h_aseb_fenster`/`h_aseb_menue` in 6 Sprachen (`BHED_HB_NUR=tutgeo` bzw. `aseb`, `BHED_HB_FENSTER=2560x1440@1.5`, je 4 parallel, `kr_hb_<teil>_<sprache>` je 11/0 bzw. 9/0), alle mindestens 1920 breit (2538), angesehen.
- Dabei gefunden: im japanischen `h_aseb_fenster` lief die Vorschau des Griffs („ブラシ 1 個(detail) + モデル + クリップ“, jetzt Modell+Clip) über den Fensterrand. Die Spalte Vorschau bricht jetzt um (`TextWrapped`), Bilder danach neu (`kr_hb2_*`).
- Im Bild von Schritt 5 verdecken die Clip-Brushes (Schachbrett „SHADER IMAGE MISSING“ wie NRC) die Ranke vorher wie nachher; die Drehung aus Punkt 1 ist dort nicht zu sehen.

## 5. Spiegel-Werkzeug: Ebene aufs Raster wie NRC

- „Flip“ (Select_FlipAxis -> scaleSelected mit Raster) war schon 1:1 (selection.cpp:1703-1709 = `skaliereAuswahl`). Abweichend war der Spiegel-Modus „Ebene an der Auswahl“: `std::round` statt NRC `vector3_snap`/`float_snapped` (llrint: genau halbes Raster zur geraden Zahl).
- Rot `kr_rot`: Brush x 0..16 / -16..0 / 16..32 bei Raster 16: Ebenen 16 / -16 / 32. Grün `kr_g1`: 0 / 0 / 32 (`karte::zahlEingerastet`, gui/app_karte_spiegel.cpp).

## 6. Mehrere fx_lava

- Nachgestellt an der Quelle statt mit einem Spielstart: jede Spawn-Funktion meldet ihre Befehle über `G_FindConfigstringIndex` an (game/g_utils.cpp:78-99 MD9, gleich in OpenJK): ein schon vorhandener Text (ohne Groß/Klein) gibt denselben Index, cgame führt jeden `CS_WORLD_FX`-Eintrag einmal aus (cg_main.cpp:1542). Zwei fx_lava = eine „lava“-Wolke und ein „fog“; das gilt für alle Wetter-Befehle (rain, snow, wind, fog, light_fog, heavyrainfog, constantwind mit seinem Text, JK2 „rain init <count>“).
- Rot `kr_rot6` (Teil 7, duel_mus_both2 plus ein zweites fx_lava): snowflake2 2 -> 3, alpha_smoke2b 2 -> 3. Schon die Karte allein gab zwei Nebel-Wolken alpha_smoke2b, wo das Spiel den Befehl „fog“ einmal ausführt (zwei Anmeldungen desselben Texts). Grün `kr_g6`: 2 -> 2, 1 -> 1; duel_ahs_wbw unverändert 1.
- `gui/app_wetter.cpp`: `einmal(befehl)` je Befehlstext.
- `teil6` hat außerdem 3 FEHLER, die schon auf e6daa5ca8 rot sind (`kr_rot6`, ohne Code-Änderung): G_MissileImpacted, NPC_ChoosePainAnimation, G_RadiusDamage - nicht aus diesem Block, nicht angefasst.

## Schluss

- Bau: erster Bau im neuen `build-kreste` von Grund auf, danach Folgebauten, alle 0 Warnungen; der saubere Neubau (`--clean-first`) wurde auf Anweisung des Hauptchats abgebrochen (offen).
- ctest -j 8 (`laeufe/kr_ctest.txt`): 231/231, darin `q3map2_pr316` (mit `Y:/werkzeuge/pr316/...`) und `kartegeoimport`; nach den Handbuchseiten handbuch/hilfe/touren/i18n 4/4.
- Selbsttests nach dem letzten Code-Stand (je höchstens 4 gleichzeitig): `kr_f_kr` 39/0, `kr_f_aseb` 27/0, `kr_f_tutgeo` 23/0; davor auf demselben Stand ohne den Umbruch `kr_e_kr` 39/0, `kr_e_modobj` 25/0, `kr_e_stufe9` 84/0, `kr_e_fotos` 78/0, `kr_g6` (teil6) 13/3 (die 3 alten); Handbuchbilder `kr_hb2_*` 12 x ohne FEHLER; Modus 1 `kr_m1` 2635/0.
- Lints (`laeufe/kr_lint.txt`): wie vorher - ok bis auf portability 99, pruefcodes 11, efxflags-Meldung, fremdprogramm (alle schon vorher). `gen_handbuch.py --pruefen`: es/cs 0 Texte, 0 Bilder englisch, handbuch_sprachen ok.
- Gegen das Vorbild geprüft: q3map2 selbst (Bau und `-bhedmodels`: Shader, Dreiecke, Lage), Movie Duels (Ladetest, Bild), NRC-Quelle (vector3_snap), MD9/OpenJK-Quelle (G_FindConfigstringIndex). Nicht geprüft: Original-JKA; Wetter mit zwei fx_lava im laufenden Spiel (Quelle statt Spielstart).

## Eigene Annahmen und Abweichungen

- Auto-Regel „nur Shader unter textures/ werden Brush-Seiten“: eigene Regel der ASEB-Funktion (nicht in NRC), begründet mit q3map2 map.cpp :1009. Wer „Als Brushes“ von Hand wählt, bekommt weiter Brushes (und die Shader so, wie NRC sie schreibt).
- Der Ladetest im Spiel lief mit Movie Duels (SP, `+devmap`), nicht mit Original-JKA (nur Agent p_jka).
- Keine Bildzeit-Messung (Fenster mit Last); die Änderungen betreffen keinen Zeichenpfad außer dem Umbruch einer Tabellenzelle.

## Offen

- Speicheroptimierung (Storage Sense) und `%TEMP%`: siehe Punkt 7, Entscheidung bei shank. Der ctest `q3map2_pr316` in `Y:/karte/build-test` zeigt weiter auf den (wiederhergestellten) Ordner im alten Scratchpad; besser `-DBEHAVED_Q3MAP2_PR316=Y:/werkzeuge/pr316/regression_tests/auto-pigeon` (so in `build-kreste`).
- `teil6`: 3 FEHLER schon auf e6daa5ca8 (siehe 6).

## Changelog

### English
- Map editor: converting a model to brushes now matches a build - parts whose shaders lie outside `textures/` (for example a texture next to an .ase from 3ds Max) become model + clip automatically, because brushes can only carry `textures/` shaders; before, the handle of a chest came out as `textures/eb/griff`.
- Map editor: "Model + clip" parts were baked lying on their side, 90 degrees tipped into the floor - the .obj next to the settings is now written Y up, so q3map2 places it exactly where the model was. Tested with a real 3ds Max .ase from Movie Duels in a built map and in the game.
- Map editor: mirror mode snaps the plane to the grid exactly like NetRadiant-Custom (half a grid rounds to even).
- Preview: two fx_lava (or two fx_rain, fx_snow ...) give one cloud per weather command, like the game - the same command is only sent once.
- Geometry to brushes: the preview column wraps instead of running out of the window.

### Deutsch
- Karten-Editor: Ein Modell in Brushes umzuwandeln passt jetzt zum Bau - Teile, deren Shader außerhalb von `textures/` liegen (etwa ein Bild neben einer .ase aus 3ds Max), werden von selbst Modell + Clip, weil Brushes nur Shader unter `textures/` tragen können; vorher kam der Griff einer Truhe als `textures/eb/griff` an.
- Karten-Editor: „Modell + Clip“-Teile wurden um 90 Grad gekippt in den Boden gebacken - die .obj neben den Einstellungen wird jetzt mit Y oben geschrieben, q3map2 setzt sie genau an die Stelle des Modells. Geprüft mit einer echten .ase aus 3ds Max aus Movie Duels in einer gebauten Karte und im Spiel.
- Karten-Editor: Der Spiegel-Modus rastet die Ebene genau wie NetRadiant-Custom aufs Raster (ein halbes Raster rundet zur geraden Zahl).
- Vorschau: Zwei fx_lava (oder zwei fx_rain, fx_snow …) geben wie im Spiel eine Wolke je Wetter-Befehl - derselbe Befehl wird nur einmal geschickt.
- Geometrie zu Brushes: Die Spalte Vorschau bricht um, statt aus dem Fenster zu laufen.

### 中文
- 地图编辑器:将模型转换为画刷现在与编译结果一致——着色器不在 `textures/` 下的部件(例如 3ds Max 导出的 .ase 旁边的贴图)会自动成为模型 + Clip,因为画刷只能使用 `textures/` 下的着色器;以前箱子的把手会变成 `textures/eb/griff`。
- 地图编辑器:“模型 + Clip”部件以前被烘焙成倒向一侧、旋转 90 度插进地面——设置旁的 .obj 现在以 Y 向上写出,q3map2 会把它准确放在模型原来的位置。已用 Movie Duels 中真实的 3ds Max .ase 在编译后的地图和游戏中测试。
- 地图编辑器:镜像模式将平面对齐到网格的方式与 NetRadiant-Custom 完全相同(正好半格时取偶数)。
- 预览:两个 fx_lava(或两个 fx_rain、fx_snow……)与游戏一样,每个天气命令只生成一团粒子——同一命令只发送一次。
- 几何体转画刷:预览列会换行,不再超出窗口。

### 日本語
- マップエディター: モデルをブラシに変換した結果がビルドと一致するようになりました - シェーダーが `textures/` の外にある部品(3ds Max の .ase の隣にあるテクスチャなど)は自動的にモデル + Clip になります。ブラシが持てるのは `textures/` のシェーダーだけだからです。以前は箱の取っ手が `textures/eb/griff` になっていました。
- マップエディター: 「モデル + Clip」の部品が 90 度倒れて床に埋まった状態で焼き込まれていました - 設定の隣の .obj を Y 上向きで書き出すようにしたので、q3map2 はモデルがあった位置にそのまま置きます。Movie Duels の本物の 3ds Max .ase で、ビルドしたマップとゲーム内で確認しました。
- マップエディター: ミラーモードは平面を NetRadiant-Custom とまったく同じようにグリッドへ合わせます(ちょうど半分のときは偶数へ)。
- プレビュー: fx_lava が 2 つ(または fx_rain、fx_snow が 2 つ…)あっても、ゲームと同じく天候コマンドごとに 1 つの雲になります - 同じコマンドは 1 回だけ送られます。
- ジオメトリをブラシに: プレビュー列は折り返し、ウィンドウからはみ出しません。

### Español
- Editor de mapas: convertir un modelo en brushes coincide ahora con la compilación: las partes cuyos shaders están fuera de `textures/` (por ejemplo una textura junto a un .ase de 3ds Max) pasan solas a modelo + clip, porque los brushes solo pueden llevar shaders de `textures/`; antes el asa de un cofre salía como `textures/eb/griff`.
- Editor de mapas: las partes «Modelo + clip» se integraban tumbadas, giradas 90 grados dentro del suelo; el .obj junto a la configuración se escribe ahora con Y hacia arriba y q3map2 lo coloca exactamente donde estaba el modelo. Probado con un .ase real de 3ds Max de Movie Duels en un mapa compilado y en el juego.
- Editor de mapas: el modo espejo ajusta el plano a la cuadrícula exactamente como NetRadiant-Custom (media cuadrícula redondea al par).
- Vista previa: dos fx_lava (o dos fx_rain, fx_snow…) dan una nube por comando de clima, como en el juego: el mismo comando solo se envía una vez.
- Geometría a brushes: la columna de vista previa hace salto de línea en vez de salirse de la ventana.

### Čeština
- Editor map: převod modelu na brushe nyní odpovídá sestavení - části, jejichž shadery leží mimo `textures/` (například textura vedle .ase z 3ds Max), se samy stanou model + clip, protože brushe mohou nést jen shadery z `textures/`; dříve vyšlo držadlo truhly jako `textures/eb/griff`.
- Editor map: části „Model + clip“ se zapékaly převrácené o 90 stupňů do podlahy - .obj vedle nastavení se nyní zapisuje s osou Y nahoru a q3map2 ho umístí přesně tam, kde model byl. Ověřeno se skutečným .ase z 3ds Max z Movie Duels v sestavené mapě i ve hře.
- Editor map: zrcadlový režim přichytává rovinu k mřížce přesně jako NetRadiant-Custom (přesně polovina mřížky se zaokrouhlí k sudému číslu).
- Náhled: dva fx_lava (nebo dva fx_rain, fx_snow …) dají jako ve hře jeden mrak na každý příkaz počasí - stejný příkaz se posílá jen jednou.
- Geometrie na brushe: sloupec náhledu se zalomí, místo aby přesahoval okno.
