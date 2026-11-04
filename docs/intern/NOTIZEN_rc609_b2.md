# Notizen rc609 B2: Mission ↔ Map ohne Verlust

Zweig `rc609b2` (Start 5ac74e00), Worktree `Y:\missionkarte`, Build `build-missionkarte`.

shank (03.11.): „wenn ich merke in der Mission es fehlt einen Bereich in der map. das ich rüber Switchen kann zu map den kurz adden und danach weiter machen kann an der Mission.“

### 1: Nachgestellt (rot)

Selbsttest `kartemission`, Lauf `b2rot3` (vor B2, die Karte im Map-Tab von Hand geöffnet bzw. die .bsp von Hand zurückgewandelt): 13 OK, 6 FEHLER.
- Verloren geht beim Wechseln nichts: Skripttext, eigene Figur, geänderter Spielerstart, Kamera, Zeitleiste, Auswahl und das offene Entity-Text-Fenster im Mission-Tab sowie Kamera, Auswahl und Logik-Fenster im Map-Tab bleiben in beide Richtungen (Punkt 4 war schon gegeben).
- Der Mission-Tab lädt die neue .bsp nicht: Strahl bei x 511,9 statt an der neuen Säule (x 200), die Entity-Liste zeigt noch die alte Tür `tuer` statt `tor`, kein Bezug wird gemeldet (A), auch nach dem Versetzen der Säule nicht (A2).
- Fertige Karte aus einer .pk3 (B): der Bau landet im Arbeitsordner, die .pk3 der Mission bleibt unverändert, der Mission-Tab zeigt die alte Geometrie.
- Testgerüst-Funde in den Läufen davor (kein Programmfehler): Bau-Menü erst nach `bauMenueSicherstellen`, das Logik-Fenster lag über der Modusleiste, q3map2 nimmt `light` aus dem Entity-Lump (geprüft wird deshalb der Spielerstart), die misc_model-Suche nach dem Zurückwandeln braucht mit Movie Duels Minuten (im Test abgebrochen).

### 2: "Karte der Mission bearbeiten" (Map-Tab, File)
- Neuer Eintrag unter File nach "Open .bsp..." (eigene Funktion, nicht in NRC): öffnet die Quelle `<karte>.map` neben einer losen .bsp; sonst die lose .bsp über `bspOeffnenDatei` (mit `.bhed` genau, sonst zurückgewandelt); aus einer .pk3 die Bytes des Eintrags über `bspOeffnenAus` (mit `maps/<karte>.bhed` aus derselben .pk3). Fragt vorher wie File > Open bei ungespeicherter Karte.
- Der Map-Tab merkt sich die Verbindung (Kennung, Herkunft aus W2a, die .map). Beim Bau (`bauStarten`) der Stand der .bsp der Mission, nach einem Bau ohne Rückgabe ungleich 0 wird geschrieben:
  - lose .bsp: wurde in place gebaut (Quelle daneben) nur `<karte>.bsp.bak` mit dem alten Stand, sonst `.bak` und sicher ersetzen (`bspSicherungUndSchreiben`);
  - .pk3: genau der Eintrag (`aufWegSchreiben` Weg 1, `pk3EintragErsetzen`, `.pk3.bak`), ohne Frage - der Auftrag ist ausdrücklich "in die .pk3 der Mission";
  - neue Mission ohne eigene .bsp: in ihren Missionsordner `maps/<name>.bsp` (`kartenQuelle`), ohne Missionsordner bleibt sie im Arbeitsordner und der Mission-Tab nimmt sie von dort (Meldung).
- Eigene Befehle (AutoCaulk, Optimierer) lösen nichts aus, nur Einträge des Build-Menüs.

### 3: Nachladen im Mission-Tab (`gui/app_kartemission.cpp`, Abgleich `src/kartemission.cpp`)
- Abgleich alt (Entity-Lump vor dem Bau) / Mission (angezeigt, mit allen ungespeicherten Änderungen) / neu (Entity-Lump des Baus). Regel für "dieselbe Entity", der Reihe nach, je Partner einmal:
  1. worldspawn = worldspawn;
  2. gleicher classname und Name (targetname, sonst script_targetname, sonst NPC_targetname; ohne Groß/Klein), bei mehreren der nächste Ort;
  3. Brush-Entity ohne Namen: gleicher classname und gleiche Grenzen des Brush-Modells `*N` (Lump 7, je Achse 1 Einheit) - die Nummer darf sich verschieben;
  4. sonst gleicher classname und Ort (origin, je Achse 1 Einheit).
- Mission-Entities, die Schlüssel für Schlüssel einer alten gleichen, gelten als unverändert: es gilt die Fassung der neuen Karte. Geänderte: nur die geänderten Schlüssel kommen auf die Entity der neuen Karte, `model *N` bleibt die neue Nummer (Hinweis "*2 -> *3 angepasst" in der Liste). Eigene: stehen sie nach der Regel schon in der neuen Karte (unter den Entities, die keiner alten entsprechen), werden ihre Schlüssel dort eingetragen - nicht doppelt; sonst angehängt. Von der Mission gelöschte bleiben gelöscht.
- Bezüge: was vorher auf eine Entity der Mission zeigte und nachher auf keine - Schlüssel `target`, `target2..4`, `targetJump`, `closetarget`, `opentarget`, `paintarget`, `NPC_target`, `NPC_target2`, `NPC_target4` (OJK `code/game/g_spawn.cpp:245-249, :304-306, :333-335`) und Skripte aller Reiter (`affect`, `use`, `kill`, `remove` wie der Missionsgraph); `*N`, das es nicht mehr gibt oder das andere Geometrie meint; eine von der Mission geänderte Entity, die der Map-Tab entfernt hat (bleibt erhalten, angehängt). Liste im Fenster "Mission und Karte: Bezüge" (in der 3D-Ansicht, Spalten Problem/Bezug/Wo) und eine Einblendung mit den Namen.
- Der Mission-Tab behält Kamera, Orbit, Zeitpunkt, Ausschnitt der Zeitleiste, Auswahl (über die neuen Nummern), versteckte Entities, Animationsfenster, Herkunft der .bsp und .ent. Skripte werden nicht angefasst. Der Entity-Verlauf wird geleert wie bei "Karte neu laden" (W2a F3) - er zählte die Nummern der alten Fassung. Ungespeicherte Lump-Änderungen (J) gehörten zu den Flächen des alten Baus: verworfen mit Meldung.
- Gefunden und behoben: die Wege der Vorschau (`navWeltBasis`) merkten sich ihre Welt nach Pfad, Entities und Flächenzahl - nach einem Bau mit versetzter Säule (gleiche Flächenzahl, A2: 27 -> 27) liefen die Figuren durch die alte Geometrie. Der Schlüssel zählt jetzt jedes Laden der Kollision mit.
- Gefunden und behoben: die Liste öffnete beim ersten Mal über der Modusleiste (Klicks auf "Map Editor" kamen nicht an); sie öffnet jetzt in der Mitte des Hauptfensters, höchstens halb so groß. Die Zeile mit der Regel war abgeschnitten (gesehen im Handbuchbild) - jetzt umbrochen.

### 4: Wechseln
- War schon gegeben (Rotlauf): Mission-Tab und Map-Tab behalten Kamera, Auswahl und offene Fenster in beide Richtungen; geprüft im selben Test.

### Tests
- ctest `kartemission` (neu, `tests/kartemissiontest.cpp`): Regeln 1-4, unveränderte/geänderte/eigene/gelöschte Entities, `*N` neu nummeriert (Regel 3), eigene schon in der Karte nicht doppelt, Gegenprobe mit umbenannter Tür, gelöschter Tür ohne Namen, `*9` und `*1` mit anderer Geometrie, Lump 7. Grün.
- Selbsttest `kartemission` (neu), alles im Laufordner:
  - A b2raum (Quelle neben der .bsp): Rotlauf `b2rot3` 13 OK / 6 FEHLER (Strahl x 511,9, Wege 511,9, Tür noch `tuer`, kein Bezug gemeldet; A2 und B ebenso). Grün (`b2gruen3` 26/0; `b2gruen4` dazu nur die Probe auf GPU-Aufrufe rot, siehe Offen): Strahl und Wege x 199,9 an der Säule, Wegpunkt-Kante x 150 -> 400 jetzt zu, Flächen 22 -> 27, Tür `tor`, Befunde `tuer` @ `target_relay "relais": target` und `tuer` @ `intro.txt: use`, `.bsp.bak` = alte Karte; Skripttext, eigene Figur (1x), Spielerstart angle 135, Kamera 96 -48 128, Zeit 400, Auswahl (Figur + Spielerstart) und Entity-Text-Fenster unverändert.
  - Punkt 4: Map-Tab Kamera -64 32 96, Auswahl 1, Logik-Fenster offen; Mission-Tab Kamera und Zeit nach dem zweiten Wechsel gleich.
  - A2 Säule versetzt (Flächen 27 -> 27): ohne den Schlüssel-Fix Strahl 295,9, Wege 199,9 (rot, Lauf `b2navrot`); mit Fix beide 295,9.
  - B Gegenprobe b2zwei nur in `zz_b2.pk3` (zurückgewandelt): Strahl x 299,9, nur `maps/b2zwei.bsp` in der .pk3 anders, `.pk3.bak`, alle Mission-Änderungen da.
- Vorhandene Tests: `mission1` 98/0, `missionsbau` 95/0, `dateiort` 19/0, `karteneditor` mit `BHED_KARTE_NUR=fotos`: Menü File eine Zeile höher (342 x 381 statt 357) durch den neuen Eintrag - Erwartung angepasst, danach 78/0; Handbuchbilder je Sprache 14/0; ctest 203: zuerst alle bis auf `handbuch_sprachen` (die Bilder fehlten noch), am Ende 203/203; Bau aller Ziele 0 Warnungen (inkrementell).

### Offen
- Teil-Compile mit BSP-Merge für fertige Karten ohne Quelle (Plan `bsp_merge_entitaeten.md`) ist nicht Teil von B2: hier wird die zurückgewandelte Karte ganz neu gebaut (Licht, Vis neu).
- Steht die Karte der Mission im Spielordner, schreibt der Bau wie W2a an diesen Ort (der Nutzer hat sie von dort geöffnet); der Test arbeitet nur im Laufordner.
- Ein `*N`-Bezug in Skripten gibt es in ICARUS nicht; geprüft werden Schlüssel und Brush-Modelle der Entities.
- Keine Bildzeit-Messung (nicht erlaubt bei parallelen Agenten).
- Die Mission-Vorschau zeichnet die Testräume (nur `test/*`-Shader, die es nicht gibt) im Testlayout gar nicht (0 GPU-Aufrufe, schon vor dem Nachladen, unabhängig von B2); das Bild `h_kartemission_liste` zeigt deshalb eine leere Ansicht mit der Liste. Für ein schöneres Bild bräuchte die Testkarte Texturen aus dem Spiel.

## English
- Mission and map: new "File > Edit map of the mission" in the map editor. It opens the map of the mission open in the Mission tab - its source .map next to the .bsp, otherwise the .bsp (exact with editor data, otherwise converted back) - and the build writes into exactly the mission's file: the loose .bsp (old one as .bsp.bak) or its entry in the .pk3 (.pk3.bak).
- Mission and map: after the build the Mission tab reloads the map by itself - geometry, light, collision, waypoints and navigation, entity list and preview - and keeps scripts, placed and changed entities, camera, timeline and selection. Entities already in the new map are not added twice; brush models that were renumbered are adjusted.
- Mission and map: references the new build breaks (a script or target naming an entity that is gone, a missing or changed brush model *N) are listed in the window "Mission and map: references" - nothing is dropped silently.
- NPC preview: after a rebuild with the same number of surfaces the figures walked through the old geometry; the paths now follow the new collision.

## Deutsch
- Mission und Karte: neu "File > Karte der Mission bearbeiten" im Karten-Editor. Öffnet die Karte der Mission im Mission-Tab - ihre Quelle .map neben der .bsp, sonst die .bsp (mit Editor-Daten genau, sonst zurückgewandelt) -, und der Bau schreibt in genau die Datei der Mission: die lose .bsp (die alte als .bsp.bak) oder ihren Eintrag in der .pk3 (.pk3.bak).
- Mission und Karte: nach dem Bau lädt der Mission-Tab die Karte selbst nach - Geometrie, Licht, Kollision, Wegpunkte und Navigation, Entity-Liste und Vorschau - und behält Skripte, gesetzte und geänderte Entities, Kamera, Zeitleiste und Auswahl. Entities, die die neue Karte schon hat, kommen nicht doppelt; neu nummerierte Brush-Modelle werden angepasst.
- Mission und Karte: Bezüge, die der neue Bau bricht (ein Skript oder target nennt eine Entity, die fehlt; ein Brush-Modell *N fehlt oder ist anders), stehen im Fenster "Mission und Karte: Bezüge" - nichts fällt still weg.
- NPC-Vorschau: nach einem neuen Bau mit gleich vielen Flächen liefen die Figuren durch die alte Geometrie; die Wege folgen jetzt der neuen Kollision.

## 中文
- 任务与地图:地图编辑器新增"File > 编辑任务的地图"。它打开任务选项卡中任务的地图——.bsp 旁边的源 .map,否则打开 .bsp(有编辑器数据时完全还原,否则反编译)——编译会写入任务原本的文件:单独的 .bsp(旧文件保留为 .bsp.bak)或其在 .pk3 中的条目(.pk3.bak)。
- 任务与地图:编译后任务选项卡会自动重新载入地图——几何体、光照、碰撞、路径点与导航、实体列表和预览——并保留脚本、放置和修改过的实体、摄像机、时间轴和选择。新地图中已有的实体不会重复添加;重新编号的画刷模型会自动调整。
- 任务与地图:新编译导致失效的引用(脚本或 target 指向已不存在的实体,画刷模型 *N 缺失或已改变)会列在"任务与地图:引用"窗口中——不会悄悄丢失任何内容。
- NPC 预览:重新编译后若表面数量相同,角色会穿过旧的几何体;现在路径会跟随新的碰撞。

## 日本語
- ミッションとマップ: マップエディターに「File > ミッションのマップを編集」を追加。ミッションタブで開いているミッションのマップを開きます(.bsp の隣のソース .map、なければ .bsp。エディターデータがあれば正確に、なければ逆変換)。ビルドはミッションのファイルにそのまま書き込みます: 単独の .bsp(古いものは .bsp.bak)または .pk3 内の項目(.pk3.bak)。
- ミッションとマップ: ビルド後、ミッションタブはマップを自動で読み直し(ジオメトリ、ライト、衝突、ウェイポイントとナビゲーション、エンティティ一覧、プレビュー)、スクリプト、置いた・変えたエンティティ、カメラ、タイムライン、選択を保持します。新しいマップに既にあるエンティティは二重に作りません。番号が変わったブラシモデルは調整されます。
- ミッションとマップ: 新しいビルドで壊れた参照(スクリプトや target がないエンティティを指す、ブラシモデル *N がない・変わった)は「ミッションとマップ: 参照」ウィンドウに一覧表示されます。黙って失われるものはありません。
- NPC プレビュー: サーフェス数が同じ再ビルドの後、キャラクターが古いジオメトリを通り抜けていました。経路は新しい衝突に従うようになりました。

## Español
- Misión y mapa: nuevo "File > Editar el mapa de la misión" en el editor de mapas. Abre el mapa de la misión abierta en la pestaña Misión (su .map fuente junto al .bsp; si no, el .bsp, exacto con datos del editor o convertido) y la compilación escribe exactamente en el archivo de la misión: el .bsp suelto (el anterior como .bsp.bak) o su entrada en el .pk3 (.pk3.bak).
- Misión y mapa: tras compilar, la pestaña Misión recarga el mapa por sí sola (geometría, luz, colisión, puntos de ruta y navegación, lista de entidades y vista previa) y conserva scripts, entidades colocadas y cambiadas, cámara, línea de tiempo y selección. Las entidades que el mapa nuevo ya tiene no se duplican; los modelos de brush renumerados se ajustan.
- Misión y mapa: las referencias que rompe la nueva compilación (un script o target que nombra una entidad que ya no existe, un modelo de brush *N que falta o cambió) aparecen en la ventana "Misión y mapa: referencias"; nada se pierde en silencio.
- Vista previa de PNJ: tras recompilar con el mismo número de superficies, las figuras atravesaban la geometría antigua; ahora las rutas siguen la nueva colisión.

## Čeština
- Mise a mapa: v editoru map nové "File > Upravit mapu mise". Otevře mapu mise otevřené na kartě Mise (její zdrojovou .map vedle .bsp, jinak .bsp - s daty editoru přesně, jinak zpětně převedenou) a sestavení zapíše přesně do souboru mise: do samostatné .bsp (stará jako .bsp.bak) nebo do její položky v .pk3 (.pk3.bak).
- Mise a mapa: po sestavení karta Mise mapu sama znovu načte (geometrii, světlo, kolize, waypointy a navigaci, seznam entit a náhled) a ponechá skripty, umístěné a změněné entity, kameru, časovou osu a výběr. Entity, které nová mapa už má, se nezdvojí; přečíslované brush modely se upraví.
- Mise a mapa: odkazy, které nové sestavení poruší (skript nebo target uvádí entitu, která chybí; brush model *N chybí nebo je jiný), jsou v okně "Mise a mapa: odkazy" - nic nezmizí potichu.
- Náhled NPC: po novém sestavení se stejným počtem ploch procházely postavy starou geometrií; cesty teď sledují nové kolize.
