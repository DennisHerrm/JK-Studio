# Notizen rc609 KEMD: Karten-Editor mit Movie-Duels-Daten

Auftrag `auftrag_rc609_kemd.md`. Worktree `Y:\kemd`, Zweig `rc609kemd` (ab 5dc2bdcf), Build `Y:/kemd/build-kemd` (Schalter wie `Y:/karte/build-test`). Laufordner `Y:\kemd\laeufe` (nicht im Repo); `lauf.py` als Kopie dort (die Vorlage schreibt nach `Y:\karte`). Selbsttest neu: `BHED_EDITORTEST=kemd` (`gui/selbsttest_kemd.inc`, `BHED_KEMD_NUR=shader|assets`, `BHED_KEMD_KARTE=<.map>`, `BHED_KEMD_JA=<JKA-base>`).

## 1: q3map2 `-light` stuerzt ab (0xC0000005)

### Ursache
- Stapel (RelWithDebInfo-Bau von bhed_q3map2, eigener Debugger ueber dbghelp, ohne Debug-Heap): `MapSingleLuxel` light_ydnar.cpp:899 `vecs[0] = lm->vecs[0]` liest Adresse 0 (`lm->vecs == nullptr`), aus `MapTriangle` :1106, `MapRawLightmap` :1311.
- Die Flaeche: `models/map_objects/kejim/crate_04_panel` (zweite Flaeche von crate_04.md3, eben, 4 Ecken) des misc_model mit `spawnflags 4`. Model.cpp setzt dafuer `ESurfaceType::ForcedMeta`; ClassifySurfaces gibt ForcedMeta die Lightmap-Achse ( 0 0 0 ), weil sie zu Meta-Dreiecken werden soll. MakeEntityMetaTriangles wandelt aber nur, wenn `-meta` gesetzt ist oder der Shader `forceMeta` hat (`if ( !meta && !ds.shaderInfo->forceMeta ) continue;`) - nicht bei spawnflags 4. Ohne `-meta` bleibt die Flaeche ForcedMeta mit Achse 0 (in der .srf: `ESurfaceType::ForcedMeta V: 4 I: 6 planar`, kein lightmapAxis). Die nicht ebene Schwester (crate_04, 144 Ecken) wird Dreieckssuppe und ist per Ecke beleuchtet; die ebene bekommt eine Lightmap.
- -light: AddSurfaceToRawLightmap meldet "Chose a 0 valued axis" und bricht ab, bevor `lm.vecs` angelegt wird (`lm.w = lm.h = 0`, Rueckgabe false). SetupSurfaceLightmaps ignorierte die Rueckgabe und behielt die leere Roh-Lightmap; MapRawLightmap liest dann `lm->vecs`.
- **NRC-Fehler**: NetRadiant-Customs q3map2.exe 20260114 stuerzt auf demselben Nachbau ebenso ab (m4: `-light` 0xC0000005, m0: 0). Ebenso der alte ydnar-Code (gleiche Stellen).

### Behebung (wie Tabelle karte6: NRC-Fehler nicht uebernommen)
| Fund | Behebung | Test | Ausgabe anders als NRC |
|---|---|---|---|
| eigener Fund (KEMD): ebene forceMeta-Modellflaeche ohne `-meta` | `SetupSurfaceLightmaps` (lightmaps_ydnar.cpp): wird schon die erste Flaeche einer Roh-Lightmap abgelehnt (nur bei Achse 0 moeglich), faellt die Roh-Lightmap weg und die Flaeche wird per Ecke beleuchtet wie jede ohne `info.lm` | ctest `q3map2_funde` Fall `kemd1`: rot (unsere alte exe und NRC: -light 0xc0000005), gruen (0, Modellflaeche lightmapNum -3, Welt mit Lightmaps) | nur dort, wo NRC abstuerzt |

### Messwerte
- m4 (ffa3-Nachbau, -threads 1): rot 0xC0000005 (unsere alte exe und NRC), gruen 0; danach 570682 Luxel wie m0, 29 Lightmaps.
- m0: Ausgabe alte = neue exe Byte fuer Byte gleich.
- Voller Bau der zurueckgewandelten ffa3 mit `[Final] Fast` (-meta, -vis, -light -fast -patchshadows -samples 2 -filter -bounce 8): alt und neu ohne Absturz (mit `-meta` wird die Flaeche Meta), 17 von 18 Lumps gleich; nur Visibility anders (`-vis` mit Threads ist wie NRC nicht deterministisch, bekannt aus Stufe 5).
- Selbsttest `bspmerge` mit `BHED_BSPMERGE_KARTE=maps/mp/ffa3.bsp`: 11/0, Licht-Teil gebaut, Lightmap-Seiten 26 -> 28 (B13: Absturz, 26 -> 26).

## 2: "SHADER IMAGE MISSING" und violetter Boden (bm_duel1, MD)

### Ergebnis: kein Fehler der Shader- oder Bildsuche
- Selbsttest `kemd` (Teil K2): Karte = die zurueckgewandelte bm_duel1 aus B13 (`einst/karte_bsp/bm_duel1/bm_duel1.map`). Je Shader der Brush-Seiten und Patches: Ersatzbild in der Kamera? Soll = die Vorschau (Mission-Tab, `loadTextureFor` nach R_FindShader) fuer denselben Shader.
  - Movie Duels: 39 Shader, Editor ohne Bild 10, Vorschau ohne Bild 10, **nur im Editor ohne Bild: keiner**. Die 10 fehlen ueberall: `system/caulk, clip, nodraw, origin, physics_clip, trigger`, `common/caulk` (Editorbilder der Systemshader liefert weder JKA noch Movie Duels noch NRCs ja-Gamepack - NRC zeigt sie genauso), `mp/s_bespin` (Himmel, `qer_editorimage textures/mp/li_bespin.tga` gibt es nicht), `bespin/u_carbfloor` und `bespin/wall_middle` (Bild fehlt auch im Spiel).
  - JKA (Original-base): 39 Shader, nur im Editor ohne Bild 0; `textures/mp/s_squarelight` fehlt nur, weil in `GameData original/base/shaders/` eine `shaderlist.txt` (Astrocreeps "Clean Shaders" 2003, ohne `mp2`) liegt: NRC (Shaders_Load) und q3map2 (LoadShaderInfo) lesen dann nur die genannten Dateien, das Spiel alle. Wie NRC belassen (der Bau kennt den Shader dann auch nicht); der Test meldet solche Faelle eigens.
- Die grossen "SHADER IMAGE MISSING"-Flaechen im Befundbild sind der Trigger-Brush aus B13 (`textures/system/trigger`, qer_trans 0.5).
- **Violetter Boden**: die Oberseite des Kastens von `info_player_duel`/`info_player_deathmatch` (Klassenfarbe `(1 0 1)` aus der .def, Kasten bis 32 ueber dem Ursprung); die Kamera von B13 steht 48 ueber dem Spielerstart, also 16 ueber dem Kasten. Gemessen (Kamera wie im Befund, 45 Grad nach unten): mit Entities 225 0 225, mit **Filter > Entities** 49 52 48 (der Boden n_floor02); mit Echtzeit-Licht ebenso (225 0 225 / 10 7 7). NRC zeichnet Punkt-Entities ebenso gefuellt.
- Bild Editor mit/ohne Entities (2538 breit): `Y:\kemd\laeufe\kemd_bilder\k2_bm_duel1_md_editor_mit_ohne_entities.png` (nicht im Repo).

## 3: Asset-Browser haelt beim ersten Oeffnen an (MD-Daten)

### Ursache
- `Dateisystem::dateien` (und `verzeichnisse`) entfernten doppelte Namen durch Vergleich mit allen bisherigen (`std::none_of` mit `gleichOhneFall`, je Vergleich zwei Kleinschreib-Kopien) - quadratisch; mit Movie Duels 2470 Modelle und 24260 Klaenge (26730 Eintraege) und das im Hauptfaden.

### Behebung
- Doppelte ueber eine Menge der kleingeschriebenen Namen (erster Treffer bleibt - dieselbe Liste in derselben Reihenfolge).
- `assetsSammeln` laeuft im eigenen Faden (`std::async`, der Faden haelt das Spiel fest); das Fenster zeigt einen Fortschrittsbalken (7 Teile: je Endung, dann Prefabs) mit `KeAbSammelt`, die alte Liste bleibt bis die neue fertig ist. `assetListe` (nur Selbsttest) wartet auf das Ergebnis.

### Messwerte (Zeitmessung allein)
- Rot (`kemd_k3rot`): laengstes Bild 32046 ms, volle Liste nach 32046 ms.
- Gruen (`kemd_k3gruen`): laengstes Bild 28 ms, volle Liste nach 925 ms; `kemd_assets.txt` (26730 Zeilen, Art und Pfad) Byte fuer Byte gleich dem Rotlauf.

## 4: .obj im Editor

### Ursache
- Der Editor kannte nur die Lader fuer md3, ase und glm; NRCs Editor laedt alles andere ueber assimp (plugins/assmodel), q3map2 ebenso (model.cpp LoadModel/AssModelMesh).

### Behebung
- `src/karte/modell_assimp.cpp`: Port von NRC assmodel (`loadPicoModel`: dieselben aiProcess-Schritte und die Drehung Y oben -> Z oben, `CopyPicoSurface`: Shadername aus Materialname bzw. Diffuse-Textur mit den Regeln fuer `textures/`, `models/`, Wurzelordner, absolute Pfade; `AssIOSystem`: Dateien aus dem Spiel-VFS, auch .mtl). Fuer .obj, .fbx, .gltf, .glb (die Leser des Programms).
- **assimp-Fehler (auch in q3map2/NRC)**: `GenFaceNormalsProcess` verdoppelt Ecken, die mehrere Flaechen teilen, aber nur die Lage - UV, Farben, Tangenten der neuen Ecken wurden hinter dem Ende ihrer Felder gelesen (st z. B. 2.5e-29, von Lauf zu Lauf anders; Eckenzahl 17 oder 18 nach JoinIdenticalVertices, in Editor und q3map2 -bhedmodels). Jetzt werden diese Felder mit verdoppelt (`third_party/assimp/code/PostProcessing/GenFaceNormalsProcess.cpp`). Betrifft nur Modelle ohne Normalen (.obj ohne `vn`).

### Test
- ctest `karte_obj` (neu, `tests/karteobj.cpp`): .obj mit .mtl (zwei Materialien) im TEMP-Spielordner; Editor (`assimpLesen` ueber das VFS) gegen `bhed_q3map2 -bhedmodels` (wie InsertModel): Shader, Ecken (Lage, st), Dreiecke je Flaeche gleich, st nur 0/1. Vor der assimp-Behebung: Abweichung bis 7.6e+28, Eckenzahl wechselnd; danach 0.

## 5: wpauto_398 "Nav(1) in solid" (jtg)

### Ursache
- NAV::SpawnedPoint (g_navigator.cpp:2310-2336 OpenJK, MD gleich): Kasten (-20 -20 0)..(20 20 60), Spur 5 nach oben, MASK_NPCSOLID|CONTENTS_BOTCLIP. Am Ort (-320 1920 328) liegen die Brushes der Tueren `doorsBottom2` (*43, *44: func_door, spawnflags 13 = START_OPEN, lip 98, Breite 58 in Lump 7).
- SP_func_door rechnet den Weg mit `ent->maxs - ent->mins`, und das sind die Grenzen aus CM_ModelBounds - CMod_LoadSubmodels spreizt sie um einen Punkt (cm_load.cpp:201-203 MD): Breite 60, Weg 60 - 98 = -38. Unser Port nahm die Grenzen aus Lump 7 (Weg -40): die offene Tuer *44 stand bei x -399..-341 statt -397..-339 - 1 Einheit neben statt 1 Einheit im Kasten des Wegpunkts. Gleiches gilt fuer SP_func_plat (Hoehe ohne "height").
- Behebung: `cmGroesse` in src/mover.cpp (+2 je Achse) fuer func_door (Vorschau und `tuerStellungen` fuer Wegpunkte/Kanten) und func_plat.
- Test (Selbsttest `wegerzeugung`, neu): "jeder Wegpunkt wird beim Laden ein Knoten des Spiels" (Netz wie beim Laden) und mit `BHED_WE_SOLID="-320 1920 328"` "ein Wegpunkt dort wird wie im Spiel kein Knoten". Rot (Lump-7-Grenzen, `kemd_we_rot`): "Knoten (Spiel: in solid)", FEHLER; gruen (`kemd_we_gruen`): kein Knoten, 9/0. Die Erzeugung setzt den Punkt nicht mehr (787 statt 788 Wegpunkte, alle werden Knoten).

## Tests (Schluss, einmal, parallel; die Zeitmessung K3 allein)
- Bau aller Ziele /W4 /WX ohne Warnung. ctest 211/214 im ersten Lauf; die drei angepasst und gruen:
  - `mover`: die Testgeometrie war "Lump 7 = Groesse im Spiel"; jetzt Lump 7 je Seite 1 kleiner, damit das Spiel (gespreizt) die 8 x 64 x 128 sieht, mit denen die Erwartungen rechnen.
  - `karte_modell`: `.obj` ist jetzt bekannt (Erwartung umgedreht, `.3ds`/`.md2` bleiben unbekannt).
  - `hilfe` (H2-Liste 83.3 %): "add a door that slides open" - `k-tuer` und `k-tod` lagen gleichauf (21.710); jeder neue Absatz kippte die Reihenfolge. Die Tour "Tuer mit Trigger" (`tut-karte-tuer`, die beiden ersten Treffer) zaehlt jetzt als richtige Antwort: 87.5 %.
- Neu: ctest `karte_obj`, Fall `kemd1` in `q3map2_funde`; Selbsttest `kemd` (K2 10/0, K3 1/0), Pruefungen in `wegerzeugung` (9/0 mit `BHED_WE_SOLID`).
- Selbsttests: `karteneditor` (`BHED_KARTE_NUR=fotos`) 78/0, `bspmerge` mit ffa3 11/0, `modellsetzen` 18/0, `wegerzeugung` jtg 9/0, `kemd` 10/0 + 1/0, `mission1` siehe Bericht. lint_sources, lint_fassung ok.
- Handbuch (6 Sprachen): Seite Karten-Editor/Werkzeuge 9, Asset-Browser: ein Absatz (Sammeln im Hintergrund, Modelle ueber assimp); kein neues Bild (der Balken steht unter einer Sekunde).

## Eigene Annahmen
- q3map2: eine Flaeche, deren erste Lightmap-Projektion abgelehnt wird, wird per Ecke beleuchtet (wie ihre nicht ebene Schwester und jede Flaeche ohne Roh-Lightmap). Grund: NRC hat dort keine gueltige Ausgabe (Absturz).
- Asset-Browser: 7 Fortschrittsteile (je Endung einer, die Prefabs einer) - kein Vorbild in NRC (der Browser ist eigen).

## Offen
- Editorbilder fuer die Systemshader (caulk, clip, trigger, ...): die Spieldaten haben keine, NRC zeigt ebenso "SHADER IMAGE MISSING". Eigene Bilder waeren eine neue Funktion (shank entscheiden).
- `shaderlist.txt` in einem Spielordner blendet Shader fuer Editor und Bau aus (wie NRC/q3map2, das Spiel liest alle). Ein Hinweis in der Konsole waere moeglich (nicht gebaut).
- Echtzeit-Licht zeigt den Boden von bm_duel1 sehr dunkel (10 7 7; worldspawn `_minlight 30`, `ambient 60`): nicht untersucht.
- Ausloesefeld der Tueren (Think_SpawnNewDoorTrigger) nimmt im Spiel absmin/absmax (gespreizte Grenzen, dazu SV_LinkEntity +-1); unser Port die Lump-7-Grenzen - nicht geprueft.
- Kein neuer Spielvergleich fuer Punkt 5 (Beleg ist der H2-Spiellauf); Gegenprobe im Spiel auf einer zweiten Karte nicht gemacht.

## Changelog

### English
- Map editor: q3map2 no longer crashes in `-light` on a flat surface of a `misc_model` with `spawnflags 4` (force meta) compiled without `-meta` (NetRadiant-Custom's q3map2 crashes there too); the surface is lit per vertex.
- Map editor: the asset browser collects models and sounds in the background with a progress bar - with Movie Duels data it held the program for about 30 seconds when first opened; the list is the same.
- Map editor: models in .obj, .fbx, .gltf and .glb show in the editor with the surfaces and shaders q3map2 bakes in. Models without normals get correct texture coordinates in q3map2 and the editor (an assimp error read past the end).
- Doors and platforms move exactly as far as in the game (the game widens brush model bounds by one unit), so waypoints next to an open door are checked like the game does ("Nav in solid").

### Deutsch
- Karten-Editor: q3map2 stürzt bei `-light` nicht mehr ab, wenn eine ebene Fläche eines `misc_model` mit `spawnflags 4` (force meta) ohne `-meta` gebaut wird (das q3map2 von NetRadiant-Custom stürzt dort ebenso ab); die Fläche wird je Eckpunkt beleuchtet.
- Karten-Editor: der Asset-Browser sammelt Modelle und Klänge im Hintergrund mit Fortschrittsbalken - mit den Movie-Duels-Daten hielt er das Programm beim ersten Öffnen etwa 30 Sekunden an; die Liste ist dieselbe.
- Karten-Editor: Modelle in .obj, .fbx, .gltf und .glb erscheinen im Editor mit den Flächen und Shadern, die q3map2 einbackt. Modelle ohne Normalen bekommen in q3map2 und im Editor richtige Texturkoordinaten (ein assimp-Fehler las hinter dem Ende).
- Türen und Plattformen fahren genau so weit wie im Spiel (das Spiel weitet die Grenzen der Brush-Modelle um eine Einheit), daher werden Wegpunkte neben einer offenen Tür geprüft wie im Spiel („Nav in solid“).

### 中文
- 地图编辑器:当带 `spawnflags 4`(force meta)的 `misc_model` 的平面在不加 `-meta` 时编译,q3map2 的 `-light` 不再崩溃(NetRadiant-Custom 的 q3map2 在此同样崩溃);该面按顶点照明。
- 地图编辑器:资源浏览器在后台收集模型和声音并显示进度条——使用 Movie Duels 数据时,首次打开曾让程序停顿约 30 秒;列表不变。
- 地图编辑器:.obj、.fbx、.gltf 和 .glb 模型在编辑器中以 q3map2 烘焙的面和着色器显示。没有法线的模型在 q3map2 和编辑器中获得正确的纹理坐标(assimp 的一个错误会读到数组末尾之外)。
- 门和平台的移动距离与游戏完全一致(游戏将画刷模型的边界放大一个单位),因此打开的门旁边的路径点会像游戏一样检查(“Nav in solid”)。

### 日本語
- マップエディター: `spawnflags 4`(force meta)の `misc_model` の平らな面を `-meta` なしでビルドしても、q3map2 の `-light` がクラッシュしなくなりました(NetRadiant-Custom の q3map2 も同じ所でクラッシュします)。その面は頂点ごとにライティングされます。
- マップエディター: アセットブラウザーはモデルとサウンドを進捗バー付きでバックグラウンドで集めます。Movie Duels のデータでは初回に約 30 秒プログラムが止まっていました。リストは同じです。
- マップエディター: .obj、.fbx、.gltf、.glb のモデルが、q3map2 が焼き込むのと同じ面とシェーダーでエディターに表示されます。法線のないモデルは q3map2 とエディターで正しいテクスチャ座標になります(assimp の不具合で配列の外を読んでいました)。
- ドアとプラットフォームはゲームと同じ距離だけ動きます(ゲームはブラシモデルの境界を 1 単位広げます)。そのため開いたドアの隣のウェイポイントもゲームと同じように確認されます(「Nav in solid」)。

### Español
- Editor de mapas: q3map2 ya no se cuelga en `-light` con una superficie plana de un `misc_model` con `spawnflags 4` (force meta) compilado sin `-meta` (el q3map2 de NetRadiant-Custom también se cuelga ahí); la superficie se ilumina por vértice.
- Editor de mapas: el navegador de recursos reúne modelos y sonidos en segundo plano con una barra de progreso; con los datos de Movie Duels detenía el programa unos 30 segundos al abrirlo por primera vez; la lista es la misma.
- Editor de mapas: los modelos .obj, .fbx, .gltf y .glb aparecen en el editor con las superficies y shaders que q3map2 hornea. Los modelos sin normales obtienen coordenadas de textura correctas en q3map2 y en el editor (un error de assimp leía más allá del final).
- Puertas y plataformas se desplazan exactamente lo mismo que en el juego (el juego amplía los límites de los modelos de brush en una unidad), así que los puntos de ruta junto a una puerta abierta se comprueban como en el juego («Nav in solid»).

### Čeština
- Editor map: q3map2 už při `-light` nespadne, když se rovná plocha `misc_model` se `spawnflags 4` (force meta) staví bez `-meta` (q3map2 z NetRadiant-Custom tam padá také); plocha se osvětlí po vrcholech.
- Editor map: prohlížeč assetů sbírá modely a zvuky na pozadí s ukazatelem průběhu - s daty Movie Duels při prvním otevření zastavil program asi na 30 sekund; seznam je stejný.
- Editor map: modely .obj, .fbx, .gltf a .glb se v editoru zobrazí s plochami a shadery, které q3map2 zapeče. Modely bez normál dostanou v q3map2 i v editoru správné texturové souřadnice (chyba assimp četla za koncem pole).
- Dveře a plošiny se posouvají přesně tak daleko jako ve hře (hra rozšiřuje hranice brush modelů o jednotku), takže body trasy vedle otevřených dveří se kontrolují jako ve hře („Nav in solid“).
