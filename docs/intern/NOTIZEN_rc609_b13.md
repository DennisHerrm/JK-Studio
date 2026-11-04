# Notizen rc609 B13

## B13: In fertige Karte einfügen, Stufe 2 (Clips in die Welt) und Stufe 3 (sichtbare Mover mit Lightmaps)

Auftrag `auftrag_rc609_b1s23.md`. Worktree `Y:\bspmerge2`, Zweig `rc609b13` (Start a1ce3506 = Stufe 1).

### Ablauf
- Build > "In fertige Karte einfügen" (`bspInFertigeKarte`, `gui/app_karte_bsp.cpp`) nimmt jetzt auch neue Welt-Brushes (`karte::neueWeltPrimitive`: Abdruck je Primitiv über `primitivText`, gemerkt beim Öffnen, nach der Rückgewinnung und nach dem Einfügen in `bspGeometrieMerken`).
- Schranke: jeder Welt-Brush von damals noch da (`fehlend == 0`) und der Fingerabdruck ohne Welt und ohne die neuen Entities gleich (`fingerabdruckOhneWelt`).
- Teilkarte (`karte::teilKarte`): neue Entities mit der Marke `_bhed_neu`, neue Welt-Brushes in einem `func_static` mit `_bhed_welt` (q3map2 baut es als Untermodell). Nur markierte Entities kommen ins Entity-Lump.
- Durchgang 1: `<stamm>_teil.map` mit Platzhalter-Welt, `q3map2 -bsp`. Meldet `bspModelleAnhaengen` nur "sichtbar", folgt Durchgang 2 (Stufe 3): `<stamm>_teillicht.map` mit der zurückgewandelten Welt (ohne die neuen Brushes), den eingebackenen Entities, den Lichtern und den Zielen der Spots; `teilLichtBefehle`: `-bsp` (+ `-meta`, wenn die -bsp-Stufe der Karte es hatte) und `-light` mit den Schaltern der letzten -light-Stufe aus `_q3map2_cmdline` (ohne Lightmap-Format, Pfade, Threads). Dann `bspModelleAnhaengen(..., beleuchtet = true)`.
- `bspModelleAnhaengen` (`src/karte/bspmerge.cpp`):
  - Stufe 2: die Brushes des `_bhed_welt`-Modells ans Ende der Welt-Brushes (Modell 0); leafbrushes, Nebel-`brushNum` und `firstBrush` der Untermodelle ab dort +n; jeder Clip vorn in die leafbrushes jedes Blatts, in das ihn `FilterBrushIntoTree_r` legt (Port mit `SplitBrush`, `CreateBrushWindings`, `BaseWindingForPlaneAccu`, `ChopWindingInPlaceAccu`, `ClipWindingEpsilonStrict`, `FixWindingAccu`, `BrushVolume`), Einfügen wie MergeBSPMain -world (alle späteren `firstLeafBrush` +1). Inhalt jedes Clips: ein Clip-Bit (PLAYERCLIP/MONSTERCLIP/BOTCLIP/SHOTCLIP), kein SOLID/LAVA/WATER/FOG/TERRAIN/OPAQUE/SLIME, keine Flächen - sonst Fehler "welt".
  - Stufe 3: je neuem Modell Flächen, Ecken, Indizes angehängt (shaderNum über EmitShader-Regel, fogNum über den gleichen Nebel der Karte, firstVert/firstIndex neu, lightmapNum[0..3] auf angehängte Seiten), Brush-Seiten (RBSP drawSurfNum) auf die neuen Flächen (FixBrushSides), Blattflächen des Modellblatts (EmitLeaf). Nur die benutzten Lightmap-Seiten werden angehängt.
  - Grenzen (Fehler): `MAX_LIGHTMAPS` 256, `MAX_MAP_DRAW_SURFS`/`_VERTS`/`_INDEXES`; externe Lightmaps der Karte oder des Teils: Fehler "lightmap".

### Quellen
- Ladeweg der Welt-Brushes: OJK/MD9 `code/qcommon/cm_load.cpp` `CMod_LoadLeafs` (firstLeafBrush, numLeafBrushes je Blatt), `CMod_LoadLeafBrushes` (int-Feld), `CMod_LoadBrushes` (Brush-Nummern frei, `BOX_BRUSHES` dahinter); `cm_trace.cpp` `CM_TraceThroughLeaf` und `cm_test.cpp` `CM_PointContents` lesen `cm.leafbrushes[leaf->firstLeafBrush + k]` - ein Welt-Brush kollidiert nur über die Blätter, die ihn führen. Untermodelle bekommen ihr eigenes Blatt aus firstBrush..numBrushes (`CMod_LoadSubmodels`), darum geht der Clip nicht als `*N`, sondern über die Welt-Blätter.
- q3map2 (NRC 20260114 mit PR 316): `MergeBSPMain` (`convert_bsp.cpp`) -world und das Umrechnen (shaderNum, fogNum, firstVert, firstIndex, lightmapNum, brushNum der Nebel, surfaceNum der Seiten); `FilterBrushIntoTree_r`, `SplitBrush`, `CreateBrushWindings`, `FixWindingAccu`, `SnapWeldVectorAccu`, `BrushVolume`, `BoundBrush` (`brush.cpp`); `BaseWindingForPlaneAccu`, `ChopWindingInPlaceAccu`, `ClipWindingEpsilonStrict` (`common/polylib.cpp`); `Q3MAP2_EXPERIMENTAL_HIGH_PRECISION_MATH_FIXES 1` (`q3map2.h:91`); `InjectCommandLine` (`bspfile_abstract.cpp`, Form von `_q3map2_cmdline`); `LightMain` lädt die Lichter aus der .map (`light.cpp:3394`); `UnparseEntities` (Lichter ohne targetname raus, `writebsp.cpp`); `ENTITY_CAST_SHADOWS 0` (`q3map2.h`).
- Renderer (MD9 `code/rd-vanilla`): `R_LoadLightmaps` (`tr_bsp.cpp`: LIGHTMAP_SIZE 128, `count = len / (128*128*3)`, keine Prüfung gegen `tr.lightmaps[MAX_LIGHTMAPS]` 256 aus `tr_local.h`), `R_FindLightmap` (`tr_shader.cpp`: externe `lm_%04d.tga` nach Nummer), `qfiles.h` MAX_MAP_DRAW_SURFS 0x20000, _VERTS/_INDEXES 0x80000.
- Inhalt (OJK `code/game/surfaceflags.h`): CONTENTS_PLAYERCLIP 0x10, MONSTERCLIP 0x20, BOTCLIP 0x40, SHOTCLIP 0x80, OPAQUE 0x8000 usw.; `textures/system/clip` (assets1.pk3 system.shader): nodraw nonsolid nonopaque playerclip monsterclip trans.
- Spiel: `Cmd_SetViewpos_f` zieht 25 ab (MD9 `code/game/g_cmds.cpp:1284`); `CG_Viewpos_f` gibt `refdef.vieworg` aus (`cg_consolecmds.cpp`); erste Person mit Schwert nur mit `cg_fpls` (MD9 `code/cgame/cg_view.cpp`); Augenhöhe `standheight + STANDARD_VIEWHEIGHT_OFFSET` (-4, `bg_pmove.cpp`, `bg_public.h`).

### Abweichungen von MergeBSPMain (je mit Grund)
- Blätter des Clips über den Baum (FilterBrushIntoTree_r) statt Kastentest gegen alle Blätter (-world prüft auch Untermodell-Blätter und Blätter, die der Kasten nur berührt). ctest: 4 Blätter statt 16.
- Lightmap-Nummern: neue Seite = Seiten der Karte + n, nur benutzte Seiten (MergeBSPMain: + größte Nummer + 1 und alle Seiten; das verrutscht, wenn die Karte Seiten hinter der größten benutzten hat, und der Licht-Teil hätte die ganze Welt noch einmal).
- Nebel: über Shader und Kasten des Nebel-Brushes der Karte zugeordnet statt angehängt (der Nebel des Teils hängt an einem Brush der Teil-Welt, die nicht übernommen wird).
- Nur die geänderten Lumps neu (`bspMitLumps`), wie in Stufe 1.

### Eigene Annahmen (keine Engine- oder Werkzeug-Quelle)
- Welt-Grenzen für `BoundBrush`: ±MAX_WORLD_COORD statt `c_worldMinmax` des Baus (gibt es nach dem Bau nicht mehr). Der Kniff "coplanarBrushes" aus `FilterBrushIntoTree_r` (PR 316, M3) braucht den Flächenbaum des Baus und fehlt.
- Ein Clip ist, was ein Clip-Bit hat und kein SOLID/LAVA/WATER/FOG/TERRAIN/OPAQUE/SLIME; alles andere würde nach einem vollen Bau Baum oder VIS ändern.
- Licht-Teil ohne `_q3map2_cmdline`: q3map2-Vorgaben (`-bsp`, `-light` ohne Schalter).
- Licht-Teil: Lichter = Licht-Entities der Karte im Editor (fertige Karten haben meist keine; dann Himmel, leuchtende Shader, worldspawn); vorhandene Brush-Entities werfen keinen Schatten (ENTITY_CAST_SHADOWS 0 wie beim Bau).
- Externe Lightmaps (Karte oder Teil) werden abgelehnt statt `lm_####.tga` mitzuschreiben (offen).

### Tests
- ctest `bspmerge` (erweitert):
  - Rot (erster Lauf): Syntaxfehler im Python-Test; dann 2 Fehlschläge: Welt im CM-Port 64/81 Punkte "anders" (Erwartung ohne CONTENTS_TRANSLUCENT des Clip-Shaders und mit Punkten genau auf der Clip-Seite zur Wand - dort führt das Blatt der Wand den Clip nicht, wie nach einem vollen Bau), Tür-Luxel 0 (q3map2 schreibt lightmapX/Y/Width/Height 0, die Koordinaten stehen in den Ecken). Beides Test-Erwartung; die Tür neben das Licht gestellt (ambient 8 allein gäbe 8).
  - Grün: Stufe 1 unverändert; Stufe 2: 1 Clip, Baum/VIS/Flächen/Ecken/Indizes/Nebel/Lightmaps/Lichtgitter 10/10 Byte-gleich, Modell 0 10 -> 11 Brushes, Clip in 4 Blättern (Kastentest 16), CM-Port Mitte 0x80000030 (vorher 0), Spieler-Strahl endet bei x -19.12 (erwartet -19, vorher frei), nur SOLID geht durch; Welt an 45045 Punkten gleich; Gegenprobe solider nodraw-Brush -> Fehler "welt". Stufe 3: Befehle aus `_q3map2_cmdline` (`-bsp -meta`, `-light -fast`); unbeleuchtet -> "sichtbar"; beleuchtet *2 mit 6 Flächen, 1 Seite; Baum/Nebel/Lichtgitter/VIS 5/5, Welt-Lightmaps/-Flächen/Ecken/Indizes/Blattflächen vorn Byte-gleich 5/5; Lightmap 1 im Bereich 1..1 (Welt 0..0); Tür-Luxel 54.1 (ambient 8); 6/6 Brush-Seiten auf die neuen Flächen; Blatt mit 6 Blattflächen; Grenze 256 Seiten -> MAX_LIGHTMAPS; externe Lightmaps -> "lightmap".
- Selbsttest `bspmerge` (erweitert um G, H, I), mp/duel1: grün 11/11 im ersten Lauf.
  - G: `*3 *3 clips 1`, Modelle 3 -> 4, Welt-Brushes 360 -> 361, Tür 6 Flächen, Lightmap-Seiten 46 -> 48, Baum/VIS/Lichtgitter 4/4, vorn 4/4, CM-Port im Clip 0x80000030.
  - H: Vorschau (Mission-Tab, Datei neu geladen): Tür *3 mit 6 Flächen und Lightmap, Helligkeit an 24 Ecken 39.6, func_door mit *3 in der Liste. Vergleichsbild Editor/Vorschau.
  - I: wieder geöffnet: func_door 0 -> 1, Clip-Brushes in der Welt 6 -> 7, nichts neu.
  - Gegenprobe mp/ffa3 (nicht zum Entwickeln): ROT - der Licht-Teil stürzt in `q3map2 -light` ab (0xC0000005 nach MapRawLightmap), der Editor fügte trotzdem ein (Tür ohne Lightmaps, Seiten 26 -> 26). Behoben im Ablauf: `bauRueckgabe()` (erste Rückgabe != 0 der Schritte) - ein abgestürzter Schritt bricht das Einfügen ab ("q3map2 exit 0xC0000005"); im selben Selbsttest danach: Meldung, Datei unverändert (Modelle 10 -> 10). Der Absturz selbst liegt in q3map2 und nicht in B13: eingegrenzt (Halbierung über 53 misc_model) auf einen einzelnen `misc_model` `models/map_objects/imperial/crate_04.md3` mit `spawnflags 4` (forceMeta) - die zurückgewandelte Welt allein und dasselbe Modell mit `spawnflags 0` gehen; Nachbau `laeufe/q3rep/min/m4.map` (-threads 1, jedes Mal). Ein voller Bau der zurückgewandelten ffa3 im Karten-Editor trifft ihn ebenso (offen).
  - Gegenprobe mp/duel4 und mp/duel2 (beide nicht zum Entwickeln): grün 11/11. duel4: `*2 *2 clips 1`, Welt-Brushes 588 -> 589, Tür 6 Flächen, Seiten 2 -> 4, Vorschau-Ecken 56.7; duel2: `*3 *3 clips 1`, 2398 -> 2399, Seiten 6 -> 7, 22.7.
- Karten-Editor kurz (`BHED_EDITORTEST=karteneditor BHED_KARTE_NUR=fotos`): 78 OK, 0 FEHLER.
- Im Spiel (Movie Duels SP per Popen, eigener fs_homepath, Fenster außer Sicht, `cg_fpls 1` für die erste Person):
  - mp/bm_duel1 Clip: setviewpos 768 40 58 90, 4 s vorwärts: Auge bei y 227 (Clip-Seite y 240, Spieler halb 15; ohne Clip, die .bak: y 291 bis zur Wand).
  - mp/bm_duel1 Tür: von (766 43) Blick 135 auf die Tür (x 681..697): Foto 1 Tür zu mit Licht, Foto 2 Tür fährt hoch, danach Auge bei (624 184) - hinter der Tür, also offen (geschlossen hielte sie bei x ~712 auf).
  - Foto Spiel (1920 x 1080, ungeskaliert) gegen Vorschau und Editor: `laeufe/vergleich_duel1_spiel_vorschau_editor.png` - Tür, Schatten und Helligkeit wie im Spiel.
  - Gegenprobe mp/bm_duel4: Clip: Auge bei y 3028 (Clip-Seite 3016, Spieler halb 15), ohne Clip (.bak) y 2978; Tür: (-2302 3212) -> (-2154 3064), hinter der Tür (y < 3129), Foto Spiel und Vorschau gleich (dunkler Shader `textures/mp/s_metal_lg_dk`).

### Offen
- q3map2 `-light` stürzt bei `misc_model` crate_04.md3 mit `spawnflags 4` (forceMeta) ab (mp/ffa3 zurückgewandelt); B13 bricht dann sauber ab, der Fehler gehört zu q3map2/Stufe 8a.
- Externe Lightmaps (`lm_####.tga`) für Karte und Teil.
- Original-JKA/JK2 (Steam) nicht gestartet; nur Movie Duels SP. MP nicht im Spiel.
- Keine Bildzeit-/Tempomessungen (paralleler Betrieb).

## English
- Map editor, Build > "Insert into compiled map": new clip brushes in the worldspawn (textures/system/clip, playerclip, monsterclip, botclip, shotclip) go into the world of a compiled map. Like a full q3map2 build they are entered into every leaf of the BSP tree they reach into, so the game collides with them; BSP tree, VIS, surfaces, lightmaps and light grid stay byte for byte.
- Insert into compiled map: visible movers (func_door, func_train, func_rotating, func_static, func_plat) get surfaces and lightmaps that fit the map: the part is compiled a second time with the converted world as shadow caster and the lights of the map (q3map2 -bsp, -light with the switches stored in the map), and only the new models with their lightmap pages are appended. The lightmaps of the world stay byte for byte; over 256 lightmap pages or the surface, vertex and index limits the command refuses.

## Deutsch
- Karten-Editor, Build > "In fertige Karte einfügen": neue Clip-Brushes in der worldspawn (textures/system/clip, playerclip, monsterclip, botclip, shotclip) kommen in die Welt einer fertigen Karte. Wie beim vollen q3map2-Bau stehen sie in jedem Blatt des BSP-Baums, in das sie hineinreichen, und das Spiel kollidiert mit ihnen; BSP-Baum, VIS, Flächen, Lightmaps und Lichtgitter bleiben Byte für Byte.
- In fertige Karte einfügen: sichtbare Mover (func_door, func_train, func_rotating, func_static, func_plat) bekommen Flächen und Lightmaps passend zur Karte: der Teil wird ein zweites Mal mit der zurückgewandelten Welt als Schattenwerfer und den Lichtern der Karte gebaut (q3map2 -bsp, -light mit den Schaltern aus der Karte), angehängt werden nur die neuen Modelle mit ihren Lightmap-Seiten. Die Lightmaps der Welt bleiben Byte für Byte; über 256 Lightmap-Seiten oder den Grenzen für Flächen, Ecken und Indizes lehnt der Befehl ab.

## 中文
- 地图编辑器,Build > "插入到已编译地图":worldspawn 中新的裁剪画刷(textures/system/clip、playerclip、monsterclip、botclip、shotclip)会进入已编译地图的世界。与 q3map2 完整编译一样,它们被登记到所伸入的每个 BSP 树叶节点中,游戏会与之碰撞;BSP 树、VIS、表面、光照贴图和光照网格逐字节保持不变。
- 插入到已编译地图:可见移动体(func_door、func_train、func_rotating、func_static、func_plat)获得与地图相符的表面和光照贴图:该部分以转换回来的世界作为投影体、使用地图的灯光再编译一次(q3map2 -bsp、-light,使用地图中保存的开关),只追加新模型及其光照贴图页。世界的光照贴图逐字节保持不变;超过 256 页光照贴图或表面、顶点、索引的限制时命令会拒绝。

## 日本語
- マップエディター、Build > 「コンパイル済みマップに挿入」:worldspawn 内の新しいクリップブラシ(textures/system/clip、playerclip、monsterclip、botclip、shotclip)をコンパイル済みマップのワールドに入れます。q3map2 のフルビルドと同じく、入り込む BSP ツリーのすべてのリーフに登録され、ゲームはそれと衝突します。BSP ツリー、VIS、サーフェス、ライトマップ、ライトグリッドはバイト単位でそのままです。
- コンパイル済みマップに挿入:見える可動物(func_door、func_train、func_rotating、func_static、func_plat)にマップに合ったサーフェスとライトマップが付きます。部分を、変換したワールドを影の投影元に、マップのライトでもう一度ビルドし(q3map2 -bsp、-light、マップに保存されたスイッチ)、新しいモデルとそのライトマップページだけを追加します。ワールドのライトマップはバイト単位でそのままで、256 ページのライトマップやサーフェス・頂点・インデックスの上限を超えるとコマンドは拒否します。

## Español
- Editor de mapas, Build > "Insertar en el mapa compilado": los brushes clip nuevos en el worldspawn (textures/system/clip, playerclip, monsterclip, botclip, shotclip) van al mundo de un mapa compilado. Como en una compilación completa de q3map2 se registran en cada hoja del árbol BSP en la que entran, y el juego colisiona con ellos; el árbol BSP, VIS, superficies, lightmaps y rejilla de luz quedan byte a byte.
- Insertar en el mapa compilado: los móviles visibles (func_door, func_train, func_rotating, func_static, func_plat) reciben superficies y lightmaps acordes al mapa: la parte se compila una segunda vez con el mundo convertido como proyector de sombras y las luces del mapa (q3map2 -bsp, -light con las opciones guardadas en el mapa) y solo se añaden los modelos nuevos con sus páginas de lightmap. Los lightmaps del mundo quedan byte a byte; por encima de 256 páginas de lightmap o de los límites de superficies, vértices e índices el comando lo rechaza.

## Čeština
- Editor map, Build > "Vložit do hotové mapy": nové clip brushe ve worldspawn (textures/system/clip, playerclip, monsterclip, botclip, shotclip) jdou do světa hotové mapy. Jako při úplném sestavení q3map2 se zapíší do každého listu stromu BSP, do kterého zasahují, a hra s nimi koliduje; strom BSP, VIS, plochy, lightmapy a mřížka světla zůstanou bajt po bajtu.
- Vložit do hotové mapy: viditelné pohyblivé objekty (func_door, func_train, func_rotating, func_static, func_plat) dostanou plochy a lightmapy odpovídající mapě: část se sestaví podruhé se zpětně převedeným světem jako vrhačem stínů a se světly mapy (q3map2 -bsp, -light s přepínači uloženými v mapě) a připojí se jen nové modely s jejich stránkami lightmap. Lightmapy světa zůstanou bajt po bajtu; nad 256 stránek lightmap nebo nad limity ploch, vrcholů a indexů příkaz odmítne.
