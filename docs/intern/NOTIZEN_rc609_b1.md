# Notizen rc609 B1

## B1: Brush-Entities in fertige Karten einfügen - Teil-Compile und BSP-Merge, Stufe 1 (unsichtbar)

Auftrag `auftrag_rc609_b1.md` (Wunsch `plaene/bsp_merge_entitaeten.md`). Worktree `Y:\bspmerge`, Zweig `rc609b1`.

### Ablauf
- Karten-Editor, Build > "In fertige Karte einfügen" (`bspInFertigeKarte`, `gui/app_karte_bsp.cpp`), aktiv bei geöffneter .bsp.
- Neu sind die Brush-Entities, deren Brushes es beim Öffnen nicht gab (`karte::neuePinselEntitaeten`, Abdrücke in `BspHerkunft::pinselAbdruecke`). Neue Punkt-Entities (`neuePunktEntitaeten`, Abdruck mit Schlüsseln) kommen mit, damit ein neuer Trigger sein neues Ziel bekommt. Geänderte vorhandene Punkt-Entities: Warnung, dafür "Update entities only".
- Stufe-1-Schranke: der Rest (Welt-Brushes, vorhandene Brush-Entities, eingebackene Modelle) muss dem Abdruck beim Öffnen gleichen (`geometrieFingerabdruck(k, ohne)`), sonst Meldung (Clips in der Welt = Stufe 2).
- Teilkarte `<stamm>_teil.map`: worldspawn (Schlüssel) mit Platzhalter, die neuen Entities; `q3map2 -bsp` (`teilBauBefehl`).
- `karte::bspModelleAnhaengen` (`src/karte/bspmerge.cpp`): Untermodelle des Teils als neue `*N` an `<stamm>_original.bsp`, dann Speichern wie im Fenster "Shader und Flächen" (`lumpBspSpeichern`: lose mit `.bak`, aus .pk3 auf den Wegen aus W2a). Danach gelten die eingefügten Entities als vorhanden (kein doppeltes Einfügen).

### Quellen
- Ladeweg: OJK/MD9 `code/qcommon/cm_load.cpp` und `codemp/qcommon/cm_load.cpp`: `CMod_LoadSubmodels` baut für jedes Modell > 0 ein eigenes Blatt aus firstBrush..numBrushes (die Blätter des Lumps braucht das Spiel für Untermodelle nicht), `CMod_LoadBrushes`/`-BrushSides` (liest nur planeNum und shaderNum, nicht drawSurfNum), `CM_InlineModel` (`index >= TotalSubModels` -> ERR_DROP).
- Grenzen: `CMod_LoadSubmodels` `count > MAX_SUBMODELS` (512) -> ERR_DROP; `CM_ClipHandleToModel` prüft `handle < numSubModels` vor `BOX_MODEL_HANDLE` (MAX_SUBMODELS-1) -> ab 512 Modellen träfe CM_TempBoxModel das Modell 511: SP höchstens 511; MP dazu `CAPSULE_MODEL_HANDLE` (MAX_SUBMODELS-2, `codemp/qcommon/cm_local.h:32`) -> höchstens 510. cgame: `inlineDrawModel[MAX_SUBMODELS]` (SP, JK2) bzw. `[MAX_MODELS]` = 512 (MP). JKA, JK2 (OpenJK code/ mit JK2_MODE) und MD9 gleich. `qfiles.h` MAX_MAP_* (Shader 0x400, Ebenen 0x20000, Blätter 0x20000, Blatt-Brushes 0x40000, Brushes 0x8000, Seiten 0x20000, Entstring 0x40000): die Engine prüft sie nicht -> Warnung.
- q3map2 (NRC 20260114): `MergeBSPMain` (`convert_bsp.cpp`, -mergebsp ohne -world) als Vorbild für das Umrechnen (Brushes, Seiten, Ebenen, Shader, `*N` = k + Modelle - 1, "minus world"); `EmitShader` (`writebsp.cpp`: gleiche Flags + Name ohne Groß/klein -> wiederverwenden); `EndModel` -> `EmitDrawNode_r` -> `EmitLeaf` (je Untermodell ein Blatt: cluster/area/Kasten wie gebaut, leafbrushes, keine Flächen, firstLeafSurface = Zahl der Blattflächen); `ProcessModels` (`bsp.cpp`: eine Entity ohne Brushes wird übersprungen, auch die worldspawn - dann verrutschen die `*N`, darum der Platzhalter); `ParseMapEntity` (`map.cpp`: das Pinselformat legt der erste Brush fest); `EmitBrushes` (Seite ohne Fläche: surfaceNum 0 aus emplace_back).
- Spielverhalten Spawn: `SelectSpawnPoint` origin[2] += 9 (MD9 `code/game/g_client.cpp:543`); `Cmd_SetViewpos_f` zieht 25 ab (MD9 `code/game/g_cmds.cpp`); `Use_Target_Print` gibt mit developer 1 "target_print '...' used by ..." aus (MD9 `code/game/g_target.cpp:155`).

### Abweichungen von MergeBSPMain (je mit Grund)
- Datei nicht neu geschrieben (WriteBSPFile packt das Lichtgitter neu), sondern `bspMitLumps`: nur Entities, Shader, Ebenen, Blätter, Blatt-Brushes, Modelle, Brushes, Seiten neu, an alle wird nur angehängt.
- Shader: vorhandene wiederverwendet (Regel von EmitShader); MergeBSPMain hängt alle an.
- Ebenen: nur benutzte, als Paar (n, n^1); ein Byte-gleiches Paar an gerader Stelle wird wiederverwendet.
- Je Modell ein Blatt wie beim vollen Bau (MergeBSPMain lässt sie weg).
- surfaceNum ohne Fläche bleibt (MergeBSPMain zählt auch dort hoch - NRC-Fehler, nicht übernommen).

### Eigene Annahmen (keine Engine-Quelle)
- Entity-Lump = Original-Text + die Entities des Teils (wie MergeBSPMain); geänderte vorhandene Punkt-Entities bleiben (Warnung, "Update entities only").
- Platzhalter der Teil-Welt: Kopie des ersten neuen Brushes mit `system/caulk` (strukturell und solide; ein Trigger allein gibt "Backwards tree volume").
- Überschneidung: nur solide neue Brushes (CONTENTS_SOLID) gegen solide Welt-Brushes, genaue Prüfung konvexer Körper (Ecken des um 0,1 verkleinerten Schnitts); Trigger dürfen in Wände reichen.
- Ziele: `target`/`target2` gegen alle targetname (ohne Groß/klein wie G_Find/Q_stricmp); nur Warnung.
- JK2 MP: keine Quelle in OpenJK, Grenze wie JKA MP angenommen.

### Tests
- ctest `bspmerge` (neu, `tools/karte/bspmerge_test.py` + `tests/bspmergetest.cpp`): Original raum.map + target_print, gebaut mit -meta/-vis/-light -fast (Lightmaps 49152 Bytes, VIS 72 Bytes); Teil mit trigger_multiple, trigger_once, func_usable (nodraw solid + origin-Brush).
  - Rot (erster Lauf): Shader-Skripte in `scripts/` statt `shaders/` (games.cpp ja), brushlose worldspawn -> q3map2 überspringt sie und die Modelle verrutschen (model 0 = Trigger), drawSurfNum 0 als "Fläche" gewertet. Behoben.
  - Grün: Welt-Lumps 10/10 Byte-gleich; 7/7 übrige Lumps nur angehängt; *2..*4; lückenlose Indizes; 2 neue Shader statt 3 (Trigger teilen einen); je Modell ein Blatt; CM-Port lädt 5 Modelle, Punkt-im-Brush 0x80000400/0x80000400/0x8001 (func_usable nur mit Ursprung); Welt im CM-Port an 6171 Punkten gleich; Warnungen "ziel" und "ueberschneidung"; Grenzen 510 ok, 511 nur MP-Fehler, 512 SP+MP-Fehler; sichtbarer func_door -> "sichtbar"; IBSP 46 gegen RBSP 1 -> "format".
- Selbsttest `bspmerge` (neu, `gui/selbsttest_bspmerge.inc`): mp/duel1 als lose Datei im Laufordner, Trigger vor einem Spielerstart (Weg frei im CM-Port), target_print, Build > In fertige Karte einfügen.
  - Rot: Spielerstart nicht gefunden (Start ohne +9 steckt im Boden), Pfad mit '\' gegen Herkunft mit '/', Platzhalter im Quake-Format in einer brushDef-Karte ("MatchToken"), Platzhalter als Trigger ("Backwards tree volume"). Behoben.
  - Grün 7/7: *2, Modelle 2 -> 3, Welt-Lumps 10/10, .bak = vorher, Mission-Tab: trigger_multiple mit *2 in der Liste, Modell der Vorschau am Kasten (Abweichung 0), CM-Port 0x80000400; Karten-Editor wieder geöffnet: trigger_multiple 0 -> 1, nichts neu.
  - Erster Lauf auf mp/ffa3 rot: die Rückgewinnung der misc_model (Stufe 8a) brauchte unter Volllast über 300 s, "In fertige Karte einfügen" ist währenddessen gesperrt (bspRueckSperre) - Wartezeit des Tests auf 1500 s.
  - Gegenprobe mp/ffa3 (nicht zum Entwickeln benutzt) grün 7/7: 9 Modelle -> 10 (*9), Welt-Lumps 10/10, Mission-Tab *9 in der Liste, Kasten Abweichung 0, Inhalt 0x80000400; trigger_multiple 2 -> 3.
- Im Spiel (Movie Duels SP per Popen, eigener fs_homepath im Laufordner, Karte in `<home>/MD/maps/mp`, Fenster außer Sicht, `+set developer 1`, `logfile 2`):
  - mp/bm_duel1: lädt ohne Fehler (`CM_LoadMap( maps/mp/bm_duel1.bsp, 1 )`, kein ERR_DROP); setviewpos an den Start, +forward: `target_print 'bm_drucker' used by player 0: "bspmerge trigger"`. Stehen bleiben (Gegenprobe): keine Zeile.
  - mp/bm_ffa3 (Gegenprobe): lädt, `target_print 'bm_drucker' used by player 0` beim Hineinlaufen; stehen bleiben: keine Zeile. Beide Läufe parallel mit eigenem fs_homepath.

### Erkenntnisse für Stufe 2 (Clips in der Welt)
- Das Spiel sucht Welt-Brushes nur über den Baum (CM_PointLeafnum, leafbrushes der Welt-Blätter); ein Clip in der Welt muss in die leafbrushes jedes Blatts, das er schneidet. Genau das macht MergeBSPMain `-world` (Kastentest gegen leaf.minmax, Einfügen mitten in die Liste, alle späteren firstLeafBrush verschoben) - die Welt-Lumps Blätter und Blatt-Brushes ändern sich dann in der Mitte, "Byte-gleich" gilt nur noch für Baum, VIS, Flächen, Lightmaps.
- Genauer als der Kastentest: Blätter über den Baum suchen (CM_BoxLeafnums mit den Ebenen der Knoten) - der Port `blaetterIm` in `src/kollision.cpp` ist dafür da.
- Brushes von Untermodellen können sofort Clips sein (`func_static`/`func_wall` mit playerclip): ein Weg ohne Eingriff in die Welt-Lumps, nur wenn die Klasse in SP und MP spawnt.
- Die Prüfungen "Überschneidung" und die Grenzen aus Stufe 1 lassen sich übernehmen.

### Erkenntnisse für Stufe 3 (sichtbare Mover mit Lightmaps)
- Der Teil hat dann Flächen, Ecken, Indizes und (mit -light) Lightmaps: Umrechnen wie MergeBSPMain (firstVert, firstIndex, shaderNum, fogNum, lightmapNum + größte Lightmap-Nummer + 1), surfaceNum der Seiten über FixBrushSides; Blattflächen für das Modellblatt (EmitLeaf: drawSurfReferences).
- Licht passend zur Karte: der Teil braucht die Welt als Schattenwerfer - Teil-Compile mit der zurückgewandelten Welt als Platzhalter (statt der Kopie eines Brushes), -light mit -keeplights, dann nur die Untermodelle übernehmen; Modell 0 bleibt weg wie jetzt.
- Externe Lightmaps (q3map2 -external) und RBSP-Lightmap-Stile (4 je Fläche) sind beim Umrechnen eigens zu behandeln.
- Das Spiel registriert jedes Untermodell als Renderer-Modell "*N" (R_LoadSubmodels); die Grenze 511/510 gilt weiter.

### Offen
- Original-JKA/JK2 (Steam) nicht gestartet; nur Movie Duels SP. MP nicht im Spiel geprüft (nur die Grenzen nach Quelle).
- Keine Bildzeit-/Tempomessungen (paralleler Betrieb).

## English
- Map editor: new command Build > "Insert into compiled map": triggers, a func_usable with nodraw brushes and other invisible brush entities built on a compiled map without its source are compiled on their own with q3map2 and appended to the .bsp as new models *N. New point entities such as the target_print a trigger targets come along. BSP tree, VIS, lightmaps and surfaces of the world stay byte for byte; the .bsp is saved with a .bak or on the ways of a .pk3.
- Insert into compiled map: checks as the game loads the map - at most 511 models in single player and 510 in multiplayer; visible brush entities, changed world brushes and a different BSP format are refused; warnings for solid brushes overlapping the world and targets without a targetname.

## Deutsch
- Karten-Editor: neuer Befehl Build > "In fertige Karte einfügen": Trigger, ein func_usable mit nodraw-Brushes und andere unsichtbare Brush-Entities, gebaut auf einer fertigen Karte ohne Quelle, werden allein mit q3map2 gebaut und als neue Modelle *N an die .bsp gehängt. Neue Punkt-Entities wie der target_print, auf den ein Trigger zeigt, kommen mit. BSP-Baum, VIS, Lightmaps und Flächen der Welt bleiben Byte für Byte; gespeichert wird mit .bak oder auf den Wegen einer .pk3.
- In fertige Karte einfügen: geprüft wie das Spiel die Karte lädt - höchstens 511 Modelle im Einzelspieler und 510 im Mehrspieler; sichtbare Brush-Entities, geänderte Welt-Brushes und ein anderes BSP-Format werden abgelehnt; Warnungen bei soliden Brushes, die die Welt überschneiden, und bei Zielen ohne targetname.

## 中文
- 地图编辑器:新命令 Build > "插入到已编译地图":在没有源文件的已编译地图上建立的触发器、带 nodraw 画刷的 func_usable 以及其他不可见画刷实体,会单独用 q3map2 编译,并作为新模型 *N 追加到 .bsp。新的点实体(例如触发器指向的 target_print)会一起插入。世界的 BSP 树、VIS、光照贴图和表面逐字节保持不变;保存时先备份 .bak,或按 .pk3 的方式保存。
- 插入到已编译地图:按游戏加载地图的方式检查——单人模式最多 511 个模型,多人模式 510 个;可见画刷实体、修改过的世界画刷和不同的 BSP 格式会被拒绝;实心画刷与世界重叠或目标没有 targetname 时给出警告。

## 日本語
- マップエディター:新コマンド Build > 「コンパイル済みマップに挿入」。ソースのないコンパイル済みマップ上で作ったトリガー、nodraw ブラシの func_usable などの見えないブラシエンティティを単独で q3map2 でビルドし、新しいモデル *N として .bsp に追加します。トリガーが指す target_print などの新しいポイントエンティティも一緒に入ります。ワールドの BSP ツリー、VIS、ライトマップ、サーフェスはバイト単位でそのままで、保存は .bak を作るか .pk3 の方法で行います。
- コンパイル済みマップに挿入:ゲームの読み込みどおりにチェックします。シングルプレイは最大 511 モデル、マルチプレイは 510 モデル。見えるブラシエンティティ、変更したワールドブラシ、異なる BSP 形式は拒否し、ワールドと重なる実体のあるブラシや targetname のないターゲットには警告を出します。

## Español
- Editor de mapas: nuevo comando Build > "Insertar en el mapa compilado": los triggers, un func_usable con brushes nodraw y otras entidades de brush invisibles creados sobre un mapa compilado sin su fuente se compilan solos con q3map2 y se añaden al .bsp como modelos nuevos *N. Las entidades de punto nuevas, como el target_print al que apunta un trigger, van con ellas. El árbol BSP, VIS, lightmaps y superficies del mundo quedan byte a byte; se guarda con .bak o por los modos de un .pk3.
- Insertar en el mapa compilado: se comprueba como el juego carga el mapa - como máximo 511 modelos en un jugador y 510 en multijugador; se rechazan entidades de brush visibles, brushes del mundo cambiados y otro formato BSP; avisos para brushes sólidos que se solapan con el mundo y targets sin targetname.

## Čeština
- Editor map: nový příkaz Build > "Vložit do hotové mapy": triggery, func_usable z nodraw brushů a další neviditelné brush entity vytvořené na hotové mapě bez zdroje se sestaví samostatně přes q3map2 a připojí k .bsp jako nové modely *N. Nové bodové entity, například target_print, na který trigger ukazuje, jdou s nimi. Strom BSP, VIS, lightmapy a plochy světa zůstanou bajt po bajtu; ukládá se se zálohou .bak nebo způsoby .pk3.
- Vložit do hotové mapy: kontroluje se tak, jak hra mapu načítá - nejvýše 511 modelů pro jednoho hráče a 510 pro více hráčů; viditelné brush entity, změněné brushe světa a jiný formát BSP se odmítnou; varování u pevných brushů, které se překrývají se světem, a u cílů bez targetname.
