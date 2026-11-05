# Notizen rc610 / MODOBJ: Modell-Browser zeigt .obj, .fbx usw. wie NetRadiant-Custom, Setzen wandelt in .md3

Auftrag `auftrag_rc610_modobj.md` (Meldung 05.11. abends: „NetRadiant can show the obj models here but JKS cant.“, shank: „kannst du das so adden für obj fbx usw.“, „convertiert er die obj und fbx nicht direkt in JKA useable format?“). Direkt in `Y:\karte` auf `karte1` ab 3950e98a (TUT-GEO), Build `build-test`, Läufe `laeufe/mo_*` (rot `mo_rot` mit der Kopie `laeufe/mo_exe_rot` = 3950e98a plus der neue Selbsttest und `q3map2Aufruf` als Testhilfe aus dem anonymen Namensraum; grün `mo_g1`, `mo_g2`; Bilder `mo_hb_<sprache>`; Schluss `mo_e_*`), Laufskript `lauf.py` wie TUT-GEO. kFassung bleibt rc609. Kein Spiel gestartet.

## Vorbild (nachgelesen, nicht geraten)

- NRC 20260114 `radiant/modelwindow.cpp:1216-1252` ModelBrowser_constructTree: die Endungen sind `GetTypeList( ModelLoader::Name )` = alle geladenen Modell-Module (StringSetNoCase, also auch groß geschrieben), aus jedem Archiv (`.pk3` und Ordner).
- Welche Module: `ja.game` hat `modeltypes="*"`; die ausgelieferte NRC (`nrc_dl/nrc.zip`) hat unter `modules/` nur `assmodel.dll` (kein md3model, kein picomodel). `plugins/assmodel/plugin.cpp:304-319` Radiant_RegisterModules: je Endung von `Assimp::Importer::GetExtensionList` ein Modul, ohne `bsp`, `pk3` (Leser geht nicht), `md5anim`, `md5camera` (kein Modell), `ogex` (stürzt ab).
- Welche assimp-Leser: in NRCs `libassimp-6.dll` stehen die Beschreibungen aller Leser, die assimp 6.0.2 von sich aus baut; es fehlen genau M3D, USD, VRML und C4D (in assimp ausgeschaltet). Der Editor baut sein assimp jetzt genauso (wie schon der q3map2), die Liste entsteht zur Laufzeit wie bei NRC. Dazu `.glm` (Abweichung wie bisher: Ghoul2 hat in NRC keinen Lader).
- Spiel: `RE_RegisterModel` lädt nur MD3 und Ghoul2 (OpenJK `code/rd-vanilla/tr_model.cpp:686-698`), `misc_model_static` nur `.md3` (`code/cgame/cg_main.cpp:1978-1983`, sonst ERR_DROP „is not an md3“ - die Karte lädt nicht). Bilder ohne Endung: jpg, png, tga (`rd-common/tr_image_load.cpp:95-97`).

## Umsetzung

- Liste: `karte::modellEndungenAssimp()` (GetExtensionList ohne die fünf), `modellBrowserEndungen()` (dazu glm), `modellArtAssimp` = eine davon außer md3/ase (eigene Lader). Vorschau, 2D und Kamera laufen über denselben Cache wie bisher (`kartenModellPfad`, Shader nach NRCs Regel aus Material bzw. Diffuse-Textur).
- Umwandlung `modellUmwandeln` (app_karte_geoimport.cpp): das Modell, wie der Editor es zeigt (Dreiecke, Normalen, st, Umlauf), über `md3PaketAusModell` als Paket in den Import-Ordner `models/import/<stamm>/` (`importStamm`: Pfad ohne `models/` mit Endung, z. B. `modobj_kiste_obj`). Je Shader: ein Skript des Spiels bleibt (kein Eintrag im Skript des Pakets), sonst kommt das Bild mit (Shader `models/import/<stamm>/<bild>` mit qer_editorimage und map). Nur geänderte Dateien werden geschrieben (sonst lädt die Live-Wache neu); `modelscale` über ±511 wie beim Ziehen. Namen in der .md3 bleiben unter MAX_QPATH (Stamm höchstens 20 Zeichen, zu lange Namen mit Prüfsumme gekürzt; sonst Fehler statt still abgeschnitten).
- Modell-Browser: Schalter **MD3** in der Leiste neben „Reload“ (Vorgabe an, `ModelBrowserConvertMd3`). Doppelklick (insertModel) und Klick (entityAssignModel) setzen die .md3; aus = der Original-Pfad wie NRC. Entities, die das Spiel zur Laufzeit lädt (nicht misc_model/_decal/_skybox), bekommen die .md3 immer (misc_model_static auch statt .glm).
- Asset-Browser (rc609 M0): misc_model_static wandelt um statt „nur .md3“ zu melden; misc_model nach dem Schalter; der Boden-Kasten mit modelscale.
- Kartenprüfung (Optimierer): Befund `modellformat` (Fehler) für Laufzeit-Entities mit `model`/`model2` in einem Format, das das Spiel nicht lädt (misc_model_static: nicht .md3).
- Ausliefern: „Pack map as .pk3“ und „Im Spiel testen“ nehmen den ganzen Import-Ordner schon mit (Stufe 9) - geprüft, nichts zu ergänzen. q3map2 backt misc_model mit .obj usw. direkt (assimp, `-fs_pakpath` für Extra Resource Paths und Import-Ordner im Build-Menü).
- Teil-Compile von „In fertige Karte einfügen“: `teilBauBefehl`/`teilLichtBefehle` hatten kein `-fs_pakpath` - die misc_model der Karte (Schattenwerfer im Licht-Teil) aus dem Import-Ordner oder einem Extra Resource Path fand q3map2 dort nicht. Jetzt wie `[ExtraResourcePaths]` (`Q3map2Aufruf::pakPfade`).
- Handbuch: Seite „Karten-Editor: Bauen“, Abschnitt Modelle neu (alle Formate), Bild `h_modobj_browser` (6 Sprachen, `BHED_HB_NUR=modobj`), Absatz zur Umwandlung und zum Schalter, Verweis auf das Tutorial `k-geometrie` (`tools/handbuch/inhalt_modobj.py`). Texte `tools/i18n_rc610_modobj.py`.

## Rot und grün

Selbsttest `karteneditor` mit `BHED_KARTE_NUR=modobj` (`gui/selbsttest_modobj.inc`, auch im vollen Lauf nach LICHT1): Testmodelle schreibt der Test selbst in einen Extra Resource Path (kiste.obj+mtl+tga, SAEULE.OBJ mit einem Shader-Skript des Spiels als Material, platte.fbx als ASCII-FBX, ring.glb, marke.ase, wuerfel.md3; in `modobj.pk3` KEGEL.OBJ); echte Klicks im Browser.

1. `.obj`/`.fbx`/`.glb` fehlten im Baum (`mo_rot` 13/15: modobj zeigte nur `marke.ase wuerfel.md3`, `modobj_pk3/` fehlte ganz; Setzen ohne Umwandlung, kein Schalter, keine Warnung, .pk3 ohne .md3, Licht-Teil ohne `-fs_pakpath`). Grün `mo_g2` 25/0: Baum mit `modobj/` und `modobj_pk3/` (KEGEL.OBJ aus der .pk3, groß geschrieben), alle sechs Dateien, nicht .mtl/.tga; Vorschau jedes Modells mit seinen Dreiecken (12, 12, 1, 2, 8, 8); Doppelklick = misc_model mit `models/import/modobj_kiste_obj/modobj_kiste_obj.md3`, Bild und Skript daneben; die .md3 im Editor mit 12 Dreiecken wie die .obj, Kasten-Abweichung 0; Klick auf misc_model_static = .md3 mit dem Shader des Spiels unverändert, kein Skript; MD3 aus = `models/modobj/kiste.obj`, gespeichert; platte.fbx = .md3 mit 2 Dreiecken; `modellformat` 2 (misc_model_static mit .glb, misc_model_breakable mit .fbx; nicht das misc_model mit .obj); q3map2 `-meta`: die Kiste als .md3 und als .obj je 12 Dreiecke in der .bsp, die Platte 2; .pk3 mit .md3, Bild, Skript, ohne die Quellen; Licht-Teil mit beiden `-fs_pakpath`.
2. Gefunden in `mo_g1` (24/2): die erste Umwandlung legt den Import-Ordner an; die .md3 wurde gezeichnet, bevor der Ordner im Suchpfad war („Model load failed“), und blieb ein NullModel (die Live-Wache merkt sich beim ersten Blick nur den Stand). Ursache allgemein: beim neuen Suchpfad behielt der Modell-Cache alte Ergebnisse. Jetzt wie NRC (ReferenceCache hängt am Dateisystem, Realise lädt jedes Modell neu): `spielUebernehmen` leert den Cache, wenn die Karte ein neues Spiel bekommt. Grün `mo_g2`.

ctest: `karte_modell` (Endungen wie NRC, md3/ase mit eigenen Ladern), `karte_modellbrowser` (Liste mit 3ds/dae/blend/..., ohne bsp/pk3/md5anim/md5camera/ogex; F.OBJ und g.Fbx im Baum, h.mtl nicht), `karte_md3import` (md3PaketAusModell: Orte, st, Normalen und Umlauf genau, Bild ins Paket, Spiel-Shader bleibt und kommt nicht ins Skript, nur Spiel-Shader = keine .shader-Datei; Stamm aus dem Pfad, lange Stämme 20 Zeichen und verschieden), `karte_optimierer` (Vorlage 36).

## Schluss

- Sauberer Neubau (`--clean-first`) 0 Warnungen, 0 Fehler. jkstudio.exe 39,0 -> 42,4 MB (alle assimp-Leser, wie NRC und der q3map2).
- ctest -j 16: 220 von 220 (`karte_modell`, `karte_modellbrowser`, `karte_md3import`, `karte_obj`, `karte_optimierer`, `karte_bspoeffnen`, `handbuch_sprachen`, `hilfe_orte`).
- Selbsttests parallel (`mo_e_*`, 6 gleichzeitig mit ctest): `karteneditor` modobj 25/0 (Bild angesehen: Schalter MD3 in der Leiste, alle sechs Modelle in der Vorschau), fotos 78/0, Stufe 9 (Ziehen, md3Paket) 84/0; `modellsetzen` 18/0 (Asset-Browser); `bspmerge` 23/0 (Teil-Compile mit `-fs_pakpath`); Modus 1 2651/0.
- Handbuchbild `h_modobj_browser` in 6 Sprachen (`BHED_HB_NUR=modobj`, `BHED_HB_FENSTER=2560x1440@1.5`, parallel, je 4/0), 2538 x 1384, angesehen; `gen_handbuch.py`: es/cs 0 Texte, 0 Bilder englisch.
- Lints: sources, fassung, i18n (erst 1: fester Text der Checkbox -> `KeModBroMd3`), i18n_erzeugt, headers, feldinit, gapp, settings, d3dbesitz, mapstate, imgui_context, rueckkopplung, cbuffer, tabellenid ok; portability 99 (wie TUT-GEO).
- Gegen das Vorbild geprüft: NRCs Endungsliste (Quelle und die ausgelieferte DLL), NRCs Shader-Regel (unverändert), q3map2 (die .md3 gibt dieselben Dreiecke in der .bsp wie die .obj), die Spielregeln (RE_RegisterModel, CG_CreateMiscEntFromGent, R_ImageLoader_Init) aus der OpenJK-Quelle. Nicht geprüft: das Bild im Spiel (kein Spielstart) und ob jede der rund 50 Endungen mit echten Dateien lädt (getestet: obj, fbx, glb, ase, md3).

## Offen

- Die Dateinamen der Zellen überlappen bei langen Namen (`SAEULE.OBJ` / `kiste.obj` im Bild) - wie NRC (drawString ohne Beschnitt), nicht geändert.
- Spiel nicht gestartet: dass die umgewandelte .md3 im Spiel so aussieht wie die eingebackene .obj, ist über q3map2 belegt (gleiche Dreiecke in der .bsp) und über das Format (readMd3 wie R_LoadMD3), nicht im Spielbild.
- Der Asset-Browser listet weiter nur md3, glm, ase, obj (eigene Funktion; alle ~50 Endungen würden das Sammeln je Endung verlängern).
- Zwei Texturen mit gleichem Dateinamen aus verschiedenen Ordnern in einem Modell bekommen im Paket denselben Shader (wie beim Ziehen seit Stufe 9).

## Changelog

### English
- Map editor: the model browser lists every model type NetRadiant-Custom lists - .obj, .fbx, .glb/.gltf, .3ds, .dae, .md2, .mdl, .md5mesh, .blend and the rest of its model loader, besides .md3, .ase and .glm - also in capital letters and from .pk3 files, with preview, in the 2D views and in the camera.
- Map editor: placing such a model converts it to an .md3 with its textures and a shader in the import folder (switch "MD3" in the model browser, on by default); the map takes the files along into its .pk3, so the model also works in misc_model_static and misc_model_breakable. Off: the original path for misc_model, like NetRadiant-Custom.
- Map editor: the asset browser converts models for misc_model_static instead of refusing them; the map check reports entities the game loads at run time with a model format it cannot load.
- Map editor: a model in a newly created import folder no longer stays empty; "Insert into finished map" finds models in the extra resource paths and the import folder.

### Deutsch
- Karten-Editor: Der Modell-Browser listet jede Modellart, die NetRadiant-Custom listet - .obj, .fbx, .glb/.gltf, .3ds, .dae, .md2, .mdl, .md5mesh, .blend und den Rest seines Modell-Laders, neben .md3, .ase und .glm - auch groß geschrieben und aus .pk3-Dateien, mit Vorschau, in den 2D-Ansichten und in der Kamera.
- Karten-Editor: Setzen wandelt ein solches Modell in eine .md3 mit seinen Texturen und einem Shader im Import-Ordner um (Schalter „MD3“ im Modell-Browser, Vorgabe an); die Karte nimmt die Dateien in ihre .pk3 mit, so geht das Modell auch in misc_model_static und misc_model_breakable. Aus: der Original-Pfad für misc_model wie in NetRadiant-Custom.
- Karten-Editor: Der Asset-Browser wandelt Modelle für misc_model_static um, statt sie abzulehnen; die Kartenprüfung meldet Entities, die das Spiel zur Laufzeit lädt, mit einem Modellformat, das es nicht laden kann.
- Karten-Editor: Ein Modell in einem neu angelegten Import-Ordner bleibt nicht mehr leer; „In fertige Karte einfügen“ findet Modelle in den Extra Resource Paths und im Import-Ordner.

### 中文
- 地图编辑器:模型浏览器会列出 NetRadiant-Custom 列出的所有模型类型——除了 .md3、.ase 和 .glm,还有 .obj、.fbx、.glb/.gltf、.3ds、.dae、.md2、.mdl、.md5mesh、.blend 以及其模型加载器支持的其他格式——大写扩展名和 .pk3 文件中的模型也会列出,并带预览,在 2D 视图和摄像机中显示。
- 地图编辑器:放置这类模型时会把它转换为导入文件夹中带纹理和着色器的 .md3(模型浏览器中的开关“MD3”,默认打开);地图会把这些文件一起打包进 .pk3,因此模型也能用于 misc_model_static 和 misc_model_breakable。关闭时:misc_model 使用原始路径,与 NetRadiant-Custom 相同。
- 地图编辑器:资源浏览器会为 misc_model_static 转换模型,而不是拒绝;地图检查会报告游戏在运行时加载、但模型格式无法加载的实体。
- 地图编辑器:新建的导入文件夹中的模型不再显示为空;“插入到已编译地图”能找到附加资源路径和导入文件夹中的模型。

### 日本語
- マップエディター: モデルブラウザーが NetRadiant-Custom と同じすべてのモデル形式を一覧にします。.md3・.ase・.glm に加えて .obj、.fbx、.glb/.gltf、.3ds、.dae、.md2、.mdl、.md5mesh、.blend などモデルローダーが読めるもの全部です。大文字の拡張子や .pk3 内のものも出ます。プレビュー、2D ビュー、カメラにも表示されます。
- マップエディター: そうしたモデルを配置すると、インポートフォルダーにテクスチャとシェーダー付きの .md3 として変換します(モデルブラウザーのスイッチ「MD3」、既定でオン)。マップはそのファイルを .pk3 に一緒に入れるので、misc_model_static や misc_model_breakable でも使えます。オフ: misc_model には NetRadiant-Custom と同じく元のパス。
- マップエディター: アセットブラウザーは misc_model_static 用のモデルを拒否せずに変換します。マップ検査は、ゲームが実行時に読み込むのに読めないモデル形式を持つエンティティを報告します。
- マップエディター: 新しく作ったインポートフォルダーのモデルが空のままになりません。「完成したマップに挿入」は追加リソースパスとインポートフォルダーのモデルを見つけます。

### Español
- Editor de mapas: el navegador de modelos muestra todos los tipos de modelo que muestra NetRadiant-Custom: .obj, .fbx, .glb/.gltf, .3ds, .dae, .md2, .mdl, .md5mesh, .blend y el resto de su cargador de modelos, además de .md3, .ase y .glm; también en mayúsculas y desde archivos .pk3, con vista previa, en las vistas 2D y en la cámara.
- Editor de mapas: colocar un modelo así lo convierte en un .md3 con sus texturas y un shader en la carpeta de importación (interruptor «MD3» en el navegador de modelos, activado por defecto); el mapa lleva los archivos en su .pk3, así el modelo funciona también en misc_model_static y misc_model_breakable. Desactivado: la ruta original para misc_model, como en NetRadiant-Custom.
- Editor de mapas: el navegador de recursos convierte los modelos para misc_model_static en lugar de rechazarlos; la comprobación del mapa avisa de las entidades que el juego carga en tiempo de ejecución con un formato de modelo que no puede cargar.
- Editor de mapas: un modelo en una carpeta de importación recién creada ya no queda vacío; «Insertar en el mapa terminado» encuentra modelos en las rutas de recursos adicionales y en la carpeta de importación.

### Čeština
- Editor map: prohlížeč modelů uvádí každý typ modelu, který uvádí NetRadiant-Custom - vedle .md3, .ase a .glm také .obj, .fbx, .glb/.gltf, .3ds, .dae, .md2, .mdl, .md5mesh, .blend a zbytek jeho zavaděče modelů - i velkými písmeny a ze souborů .pk3, s náhledem, ve 2D pohledech i v kameře.
- Editor map: umístění takového modelu ho převede na .md3 s texturami a shaderem ve složce importu (přepínač „MD3“ v prohlížeči modelů, ve výchozím stavu zapnuto); mapa vezme soubory do svého .pk3, takže model funguje i v misc_model_static a misc_model_breakable. Vypnuto: původní cesta pro misc_model jako v NetRadiant-Custom.
- Editor map: prohlížeč assetů převádí modely pro misc_model_static, místo aby je odmítl; kontrola mapy hlásí entity, které hra načítá za běhu, s formátem modelu, který neumí načíst.
- Editor map: model v nově vytvořené složce importu už nezůstane prázdný; „Vložit do hotové mapy“ najde modely v dalších cestách zdrojů a ve složce importu.
