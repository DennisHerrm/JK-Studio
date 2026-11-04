# Notizen rc609 F: Restliche Funde der Codeprüfung

Auftrag `auftrag_rc609_f.md` (Funde aus `pruefung_b5b1_funde.md`). Worktree `Y:\funde`, Zweig `rc609f`, Build `Y:/funde/build-funde`. Rotproben und Patchskripte in `<SP neu>/f_neu`.

## Ergebnisse

### F9: lint_mapstate
- Rot: `python tools/lint_mapstate.py` meldete "brushMeshes, pickedEntity", obwohl beide zurückgesetzt werden (las nur `gui/app.cpp`, `loadMapFromBytes` steht in `app_ablage.inc`).
- Neu: liest alle Quellen in `gui/`, zählt die Rümpfe aufgerufener gui-Funktionen mit (Tiefe 4: `kartenSzeneVergessen`, `karteGeometrieLesen`, `loadMapTextures` ...), Rücksetzen = `.clear()`, jede Zuweisung, `++`. `--selbsttest` nimmt `moverKlaenge.clear()` und `pickedEntity = -1` einzeln aus dem Wechsel-Rumpf und muss beide gemeldet bekommen.
- ctest `lint_mapstate` und `lint_mapstate_selbsttest` (neu).
- Echte Funde des reparierten Lints: `textures`, `cameraSx`, `cameraSy`, `gizmoZeigen`. Nachgestellt im Fehlschlagweg des Wechsels (F10): dort blieben sie stehen. Im Erfolgsweg setzt `loadMapTextures` die Texturen über `ladeSatz()` zurück, Kamera-Klickziel und Gizmo frischt das Kartenbild jedes Bild auf - dort kein Fehler.

### F10: Kartenwechsel mit unlesbarer Geometrie
- Rot (Selbsttest `fundef`, neu): yavin1b (Gras 1 Stufe, Grasnetz 984 Indizes, 277 Texturen), dann Kopie mit Vertex-Lump hinter dem Dateiende über `loadMapFromBytes`: danach Gras 1, Grasnetz 984, Texturen 347, `mapBereit` 1.
- Behoben: `kartenGeometrieVergessen()` (Geometrie, Netz, Texturen, Gras, `gpu::vergissKarte`, `mapBereit`, Kamera-Klickziel, Gizmo) - von `clearMap` und vom Fehlschlagweg in `karteGeometrieLesen` benutzt, keine doppelte Liste mehr.

### F14: TGA wie LoadTGA
- Quelle: OpenJK und MD9 `code/rd-common/tr_image_tga.cpp` (gleicher Code).
- Rot (ctest `bilder`): Typ 1, Typ 2 mit Farbtabelle, Typ 11, 16 Bit, Typ 3 mit 16 Bit, Typ 10 von oben wurden angenommen; Typ 2 mit 8 Bit las drei Bytes je Bildpunkt (83/90/40 statt 40/40/40, Lesen hinter das Ende); ein überlaufendes letztes RLE-Paket wurde abgewiesen. 8 FEHL.
- Behoben: Kopfprüfungen in der Reihenfolge von LoadTGA, 8 Bit als Grau, Typ 10 nur von unten und 24/32 Bit, RLE-Überlauf abgeschnitten (`goto breakOut`).
- Bit 0x10 (Spiegelung): die Engine wählt zwar iXStart/iXStep, schreibt aber mit `*pOut++` - die Zeile bleibt ungespiegelt. Unser Leser tat das schon; der Rotvorschlag des Funds ("x=0 blau") wäre falsch gewesen. Probe bleibt als Festhaltung (grün vorher und nachher).

### F15: GLA-Elternkette
- Nicht nachstellbar: ctest `skelett` mit 4096 Knochen in umgekehrter Kette - kein Absturz im Release-Bau (4096/4096 Matrizen). Nicht geändert. Die Probe bleibt als Festhaltung. Laufzeit von `topologicalOrder` nicht gemessen (keine Tempomessungen in diesem Auftrag).

### F16: Skriptbaum ohne Tiefengrenze
- Rot (ctest `ibi`): Text mit 20000 verschachtelten `if` - SegFault in `readScript`.
- Die Engine hat keine Grenze (Sequencer `Route -> ParseIf -> Route` rekursiv) und stürzt selbst ab. Grenze `kSkriptTiefeMax = 256`: Messgrund die tiefste der 1510 Raven-Skripte (12 Ebenen, `t1_inter/intermission`), dieselbe Grenze wie die GLM-Flächenhierarchie.
- Textleser: ab Ebene 256 Meldung L007 (6 Sprachen, `tools/i18n_rc609_f.py`), der Rest der Datei bleibt wortgetreu (geht beim Speichern nicht verloren), Übersetzen verweigert (C005). `.IBI`: `decompile` weist tiefere Blöcke mit Meldung ab.

### F19: IBI-Kopf
- Rot (ctest `ibi`): "IBIX" + 1.57f gelesen. Engine: `strcmp(id_header, "IBI")` auf vier Bytes (BlockStream.cpp:562-580). Behoben; eine im Text-Modus übertragene Datei ("IBI ") behält ihre eigene Meldung.

### F20: inflate reserviert die behauptete Größe
- Rot (ctest `archiv`): 10 Byte Deflate mit behaupteten 250 MB reservierten 262144000 Byte.
- Behoben: vorab höchstens 1032 x Eingabe (zlib technical details: höchstes Deflate-Verhältnis 1032:1); die behauptete Größe bleibt Obergrenze der Ausgabe.

### Übrige Verdachtsfälle Bereich 1
- F13 (Kodierung), F17 (lose Dateien): in W2b behoben. F18 (führender Strich): in W2a behoben. Kein weiterer offen.

### lint_endtable (app_karte_entitaeten.cpp:1876)
- Falschmeldung: `return` in einem Lambda (`std::all_of(..., [](char c) { return ...; })`). Lint berichtigt (Lambda auf derselben Zeile ausgenommen), `--selbsttest` (echter Ausstieg gemeldet, Lambda nicht). ctest `lint_endtable`, `lint_endtable_selbsttest` (neu).

### IBSP (Quake 3, Fassung 46)
- Kein Spiel des Programms nutzt IBSP (JKA, JK2, MD: RBSP 1). Rot: ctest `kartengeometrie` (IBSP 46 mit drei Q3-Ecken: gelesen, 1 Ecke) und Selbsttest `fundef` (als IBSP 46 gekennzeichnete Karte wurde geladen).
- Behoben: `readBspEntityLump` und `readBspGeometry` prüfen die Fassung wie die Engine (BSP_VERSION 1; cm_load.cpp:764, tr_bsp.cpp:1409) und melden "IBSP has wrong version number (46 should be 1) - Quake 3, not Jedi Academy / Jedi Outcast"; die geladene Karte bleibt. Die Kennung prüft die Engine nicht - unser Leser lehnt unbekannte Kennungen wie bisher ab. Kollision (`kollision.cpp`) und Karteneditor-Import (`bspoeffnen`) lesen IBSP weiter mit eigenen Größen.

## Tests
- Rotlauf einmal (Proben zuerst, Quellen unveraendert): ctest `bilder` 8 FEHL, `archiv` 1, `ibi` SegFault, `kartengeometrie` 2, `lint_mapstate` 4 Beanstandungen, `skelett` gruen (F15); Selbsttest `fundef` 1 OK / 2 FEHLER.
- Gruen: dieselben ctests 9/9, `fundef` 3/0 (die F10-Pruefung misst seither gleich nach dem Wechsel: im naechsten Bild haengt prepareScene die Modellbilder der neuen Entities an `textures` an - gehoert zur neuen Karte).
- Schluss (parallel): Bau aller Ziele 0 Warnungen; ctest 206/206; Selbsttests `missionsbau` 95/0, `mission1` 98/0; Lints lint_sources, lint_fassung, lint_i18n, lint_i18n_erzeugt, lint_settings, lint_gapp, lint_headers, lint_imgui_context, lint_feldinit, lint_tabellenid, lint_doppelanzeige, lint_mapstate, lint_endtable ok, lint_portability 106 (Basis).
- Handbuch: keine Bedienung geaendert (nur Leser und eine Lesemeldung) - nichts nachzuziehen.

## Offen
- F15: Laufzeit von `topologicalOrder` bei 4096 Knochen (quadratisch) nur mit Zeitmessung prüfbar - für den Hauptchat allein.

## English
- Map change: when the geometry of the new map cannot be read, grass, textures and graphics data of the previous map no longer stay behind.
- TGA images are read exactly like the game: colour-mapped, 15/16-bit and RLE greyscale TGAs are rejected as in the game, 8-bit TGAs of type 2 show as grey, an RLE TGA whose last packet runs past the image is shown like in the game.
- Scripts nested deeper than 256 levels no longer crash the program: the rest of the file stays as text with a message; an .IBI with such nesting is rejected. An .IBI whose header is not exactly "IBI" is rejected like in the game.
- A Quake 3 map (IBSP 46) is rejected with a clear message instead of showing wrong geometry.
- Reading a broken .pk3 no longer reserves the size the archive claims.

## Deutsch
- Kartenwechsel: Lässt sich die Geometrie der neuen Karte nicht lesen, bleiben Gras, Texturen und Grafikdaten der vorigen Karte nicht mehr stehen.
- TGA-Bilder werden genau wie im Spiel gelesen: TGAs mit Farbtabelle, 15/16 Bit und RLE-Graustufen werden wie im Spiel abgewiesen, 8-Bit-TGAs vom Typ 2 erscheinen grau, ein RLE-TGA, dessen letztes Paket über das Bild hinausgeht, erscheint wie im Spiel.
- Skripte mit mehr als 256 verschachtelten Ebenen bringen das Programm nicht mehr zum Absturz: der Rest der Datei bleibt mit Meldung als Text stehen; eine .IBI mit solcher Schachtelung wird abgewiesen. Eine .IBI, deren Kopf nicht genau "IBI" lautet, wird wie im Spiel abgewiesen.
- Eine Quake-3-Karte (IBSP 46) wird mit klarer Meldung abgewiesen, statt falsche Geometrie zu zeigen.
- Beim Lesen einer kaputten .pk3 wird nicht mehr die Größe reserviert, die das Archiv behauptet.

## 中文
- 切换地图:如果新地图的几何体无法读取,上一张地图的草、纹理和图形数据不再残留。
- TGA 图像的读取方式与游戏完全一致:带调色板、15/16 位和 RLE 灰度的 TGA 会像游戏中一样被拒绝,类型 2 的 8 位 TGA 显示为灰色,最后一个数据包超出图像的 RLE TGA 会像游戏中一样显示。
- 嵌套超过 256 层的脚本不再导致程序崩溃:文件的其余部分保留为文本并给出提示;具有这种嵌套的 .IBI 会被拒绝。文件头不完全是 "IBI" 的 .IBI 会像游戏中一样被拒绝。
- Quake 3 地图 (IBSP 46) 会以明确的提示被拒绝,而不是显示错误的几何体。
- 读取损坏的 .pk3 时不再预留压缩包声称的大小。

## 日本語
- マップ切り替え: 新しいマップのジオメトリが読めない場合、前のマップの草、テクスチャ、グラフィックデータが残らなくなりました。
- TGA 画像はゲームとまったく同じように読み込まれます。カラーマップ付き、15/16 ビット、RLE グレースケールの TGA はゲームと同様に拒否され、タイプ 2 の 8 ビット TGA はグレーで表示され、最後のパケットが画像を超える RLE TGA はゲームと同様に表示されます。
- 256 段を超えて入れ子になったスクリプトでプログラムが落ちなくなりました。ファイルの残りはメッセージ付きでテキストのまま残ります。そのような入れ子の .IBI は拒否されます。ヘッダーが正確に "IBI" でない .IBI はゲームと同様に拒否されます。
- Quake 3 のマップ (IBSP 46) は誤ったジオメトリを表示せず、明確なメッセージで拒否されます。
- 壊れた .pk3 を読むとき、アーカイブが主張するサイズを予約しなくなりました。

## Español
- Cambio de mapa: si la geometría del mapa nuevo no se puede leer, la hierba, las texturas y los datos gráficos del mapa anterior ya no se quedan.
- Las imágenes TGA se leen exactamente como en el juego: las TGA con paleta, de 15/16 bits y en escala de grises RLE se rechazan como en el juego, las TGA de 8 bits de tipo 2 se ven en gris y una TGA RLE cuyo último paquete se sale de la imagen se muestra como en el juego.
- Los scripts anidados a más de 256 niveles ya no cierran el programa: el resto del archivo se conserva como texto con un aviso; un .IBI con ese anidamiento se rechaza. Un .IBI cuya cabecera no sea exactamente "IBI" se rechaza como en el juego.
- Un mapa de Quake 3 (IBSP 46) se rechaza con un mensaje claro en lugar de mostrar una geometría errónea.
- Al leer un .pk3 dañado ya no se reserva el tamaño que declara el archivo.

## Čeština
- Změna mapy: když nejde přečíst geometrie nové mapy, nezůstávají už tráva, textury a grafická data předchozí mapy.
- Obrázky TGA se čtou přesně jako ve hře: TGA s paletou, 15/16bitové a RLE ve stupních šedi se odmítnou jako ve hře, 8bitové TGA typu 2 se zobrazí šedě a RLE TGA, jehož poslední paket přesahuje obrázek, se zobrazí jako ve hře.
- Skripty vnořené hlouběji než 256 úrovní už program neshodí: zbytek souboru zůstane se zprávou jako text; .IBI s takovým vnořením se odmítne. .IBI, jehož hlavička není přesně "IBI", se odmítne jako ve hře.
- Mapa z Quake 3 (IBSP 46) se odmítne s jasnou zprávou místo zobrazení chybné geometrie.
- Při čtení poškozeného .pk3 se už nerezervuje velikost, kterou archiv udává.
