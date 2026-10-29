# Stufe 9: die restlichen Karten-Werkzeuge (Teil 1)

Arbeitskopie `Y:\karte`, Zweig `karte1`, ab `81b9a345` (= Release rc605).
Auftrag (Koordinator): zwölf Werkzeuge nach dem Release, in der Reihenfolge der Priorität.
Erledigt sind die Punkte **1 bis 3**; die Punkte 4 bis 12 sind offen (unten). Regeln wie
bisher: NRC-treu, wo NRC etwas hat; eigene Funktionen mit "(nicht in NetRadiant-Custom)";
je Punkt ein gezielter Test (rot/grün), ctest, Commit; Handbuch in vier Sprachen.

## Commits

| Punkt | Commit | Inhalt |
|---|---|---|
| 1 | 9d4fb591 | Build > Test in game: Spiel mit +devmap, Testordner über fs_cdpath/fs_homepath |
| 2 | ee5c66cb, c29b70af | Geometrie zu Brushes: Kern (assimp, konvexe Zerlegung, UV, caulk, detail), Dialog, BSPs anderer id-Tech-3-Spiele |
| 3 | a2e3c116 | Modell per Ziehen: MD3 samt Shader und Texturen, misc_model unter dem Zeiger |
| – | f6b20b9d | Schlussrunde: Absturz im Modellbrowser (Index), Testklick, Menügrößen |
| – | (dieser) | Notizen |

## Punkt 1: Im Spiel testen

**Build > Test in game** (Untermenü): die Engine aus den Einstellungen und jedes bekannte
Spielprogramm im Engine-Ordner (jasp/jamp, OpenJK SP/MP x86/x64, Movie Duels SP/MP, bei
Jedi-Outcast-Karten jk2sp/jk2mp und OpenJO). Die Wahl wird gespeichert (`BhedTestGameExe`).

- Welche .bsp: das Schnell-Backen des aktuellen Stands (`schnellBspFuer`), sonst eine .bsp, die
  neuer ist als die gespeicherte, unveränderte Karte, sonst ein Bau mit dem ersten
  `[Test]`/`(test)`-Eintrag des Build-Menüs (danach weiter über `g_lauf.danach`).
- Wohin: `zzz_jkstudio_test.pk3` (paksort: zuletzt) mit `maps/<Karte>.bsp` und – seit Punkt 3 –
  dem Inhalt des Import-Ordners in `<Einstellungen>/spieltest/<Spielordner>/`.
- Suchpfad je Engine, **aus der .exe gelesen** (Cvar-Name als Text mit Null, nie gestartet):
  | Programm | fs_cdpath | fs_homepath |
  |---|---|---|
  | jasp.exe, jk2sp.exe | ja | nein |
  | jamp.exe, jk2mp.exe | ja | ja |
  | MovieDuels-SP/MP (OpenJK) | ja | ja |
  `fs_cdpath` (FS_Startup: vor fs_basepath, niedrigster Rang) ist die Vorgabe – die Einstellungen
  des Spielers bleiben. Hat das Spiel schon `maps/<Karte>.bsp` (VFS des Editors), nimmt es
  `fs_homepath` (höchster Rang). Geht beides nicht (jasp mit Namenskonflikt, oder der Ordner hat
  `+`, `"`, `;`): erst eine Frage (qtMeldung Ja/Nein), dann `<Engine>/jkstudio_test/` mit
  `+set fs_game jkstudio_test` bzw. der Ordner der aktiven Mod (zwei Mods stapeln sich nicht).
- Befehl: `"<Engine><exe>" +set fs_cdpath "<Ordner>" +set sv_pure 0 [+set fs_game <Mod>] +devmap <Karte>`
  (constructEngineArgs). Gestartet wird nur auf Knopfdruck, losgelöst im Engine-Ordner; im
  Selbsttest (`BHED_EDITORTEST`) nur gemerkt (`kartenBefehlStarten`).
- NRCs eigenes „Run Engine After Compile“ bleibt unverändert.

Tests: ctest `karte_spieltest` (Plan und Befehl je Engine-Art, Konflikt, Mod-Ordner nur nach
Bestätigung, ungültige Ordner/Namen; mit dem Steam-Ordner als Argument die echten .exe; rot mit
einem Stub: 13 Fehler), Selbsttest `stufe9` (Menüpunkt anklicken: kein Bau, Archiv im Testordner
byte-gleich, Befehl gemerkt, nicht gestartet).

## Punkt 2: Geometrie zu Brushes

**File > Import geometry as brushes...** liest .obj/.fbx/.glb/.gltf über **assimp 6.0.2**
(BSD-3, GPL-verträglich, lag für q3map2 schon im Baum; mit dem clang-q3map2 baut CMake ein
eigenes Ziel nur mit den Lesern OBJ/FBX/glTF und zlib aus `third_party/zlib`). Je Netz und
Knoten ein Teil (Lage eingerechnet, gespiegelte Lagen drehen den Umlauf), Y-oben → Z-oben, Maßstab.

- Geschlossene Teile (jede Kante genau zweimal, entgegengesetzt): Solid-Leaf-BSP über die
  Dreiecksebenen (gleiche Ebenen zusammengefasst = planare Flächen eine Seite; Teilerwahl nach
  Splits/Gleichgewicht, axiale bevorzugt wie SelectSplitPlaneNum). Jedes feste Blatt ist ein
  konvexer Brush (Windungen wie CreateBrushWindings, leere Seiten fallen weg).
- Seiten auf einer Netz-Fläche: Shader = Vorsatz + Materialname, UV als drei st-Punkte →
  `texdefAusSt` (BP exakt, Quake-Format bestmöglich). Seiten ohne Netz-Fläche: caulk.
- Offene Teile: konvexe planare Flächen (benachbarte Dreiecke zusammengelegt) als Prismen
  (Dicke 8), Rückseite und Ränder caulk. Klein (< 64) oder offen → detail.
- Modell + Clip: die Teile als .obj (usemtl = Shader) in `<Einstellungen>/import/models/import/`,
  ein misc_model, Clip-Brushes aus derselben Zerlegung. Der Import-Ordner ist, sobald es ihn gibt,
  erster Zusatz-Suchordner des Editors (`kartenVfsOrdner`) und `-fs_pakpath` für q3map2.
- Auto: Brushes, wenn geschlossen und höchstens 256 Brushes, sonst Modell + Clip.
- Dialog: Tabelle der Teile (Dreiecke, geschlossen, Art, Vorschau „N Brushes (detail)“),
  Einfügen = eine Undo-Stufe, alles gewählt, danach der Optimierer.
- `.bsp` anderer id-Tech-3-Spiele: „Open .bsp“ mit `-game` nach Kennung/Fassung
  (`q3map2TypFuerBsp`: IBSP 46 quake3, IBSP 47 wolf, FBSP qfusion, RBSP eigenes Spiel).

Tests: ctest `karte_geoimport` (Würfel → 1 Brush, 6 Seiten, Material, UV an allen drei Punkten;
L-Körper → 2 konvexe Brushes, Kerbe frei, verdeckte Seite caulk; Platte → Prisma, detail, Auto
= Modell + Clip; glTF eingebettet mit Y-oben und Maßstab; Einfügen und Undo; .obj-Rundlauf;
BSP-Typen; kaputte .fbx abgewiesen; rot mit abgeschalteter Umwandlung: 11 Fehler), Selbsttest
`stufe9` (Dialog, „Insert“ anklicken, Brushes, misc_model, Datei, Suchordner, Optimierer, Undo;
rot: der Knopf lag zuerst außerhalb des Fensters).

## Punkt 3: Modell per Ziehen

Eine .obj/.fbx/.glb/.gltf aus dem Explorer auf das Fenster (WM_DROPFILES, `DragAcceptFiles`):
im Karten-Editor wird sie ein MD3 samt Shader, sonst geht sie wie eine Datei von außen auf.

- `md3Paket`: ein Bild, je Shader eine Fläche, geteilt bei 1000 Punkten / 2000 Dreiecken
  (SHADER_MAX_VERTEXES/INDEXES), höchstens 32 Flächen; Umlauf entgegen der Normalen (wie JKA,
  GL_Cull FRONT); Normalen lat/lng wie R_LoadMD3; über ±511 Einheiten verkleinert + `modelscale`.
- Texturen: eingebettet (glb/fbx) oder neben der Datei (`netzLesenDatei` liest über assimps
  Datei-Leser auch die .mtl); Shadername = Bildpfad ohne Endung (so findet auch ein Programm ohne
  die .shader das Bild), `scripts/<name>.shader` mit `qer_editorimage`, `map`, `rgbGen lightingDiffuse`.
- Alles in den Import-Ordner; misc_model unter dem Zeiger in der 2D-Ansicht (XYWnd_MouseToPoint,
  dritte Achse = Mitte der Workzone) bzw. 128 vor der Kamera; eine Undo-Stufe; danach wird der
  Suchpfad neu gelesen.

Tests: ctest `karte_md3import` (Rücklesen mit `readMd3`: Ort, Normalen nach außen, Umlauf,
Shader, Textur, Skript; Normalen hin und zurück; modelscale; Teilung von 3200 Dreiecken; rot mit
falschem Umlauf), Selbsttest `stufe9` (Ablegen auf die 2D-Ansicht, Dateien, readMd3, Archiv von
„Test in game“ mit Modell/Bild/Shader, Undo) – zusammen 50 OK.

## Schlussrunde

Skript `SP/s9/ende_s9.sh` (aus `SP/s8d/ende_s8d.sh`; Selbsttests über `SP/s8d/idle_jk.sh`, ein
Prozess zur Zeit, Idle, 8-GB-Wächter), Summe `SP/s9/ende_summe.txt`.

| Schritt | Ergebnis |
|---|---|
| Bau | ok |
| ctest gesamt | **139/139** (230 s; 3 neue: karte_spieltest, karte_geoimport, karte_md3import) |
| karteneditor voll (mit großer Karte) | erst **Absturz** nach 133 s → Befund unten → 3149 OK, 2 FEHLER (Menügrößen) → **3151 OK, 0 FEHLER** (926 s, Spitze 1036 MB) |
| Modus 1 | **2645 OK, 0 FEHLER** (434 s) |
| übrige 22 Modi | alle **0 FEHLER**, dieselben Zahlen wie 8d (karte 227, kartenfunde 53, fix604b 32, laserfalle 8, geschuetzanim 3, zielstrahl 2, effektgrund 1, figurdeform 1, jk2 15, tuerlauf 13, kamera 95, missionsbau 95, szenen 26, ton 34, npcklasse 12, gluehen 8, gras 13, spulen 13, figurstufen 8, missionsliste 11, rest605 6, modellimfesten 1) |
| Leistung gegen rc605 (md_am_sith, je 2 Läufe) | Vorbereitung rc605 6585/6573 ms, Stufe 9 6813/6584 ms; Bild Mittel 13/7,7/10 und 15/8,4/11 gegen 15/8,7/13 und 15/8,4/11 ms, max. 163/202 gegen 200/201 ms – im Rauschen gleich |
| Kartenprüfung (278 MD-Karten, gegen 8d) | alle Summen gleich (0 Abstürze, 0 nicht geladen, 0 D3D-Meldungen, 1 → 1 nicht gezeichnete Entity) |
| sauberer Neubau | **0 Fehler, 0 Warnungen** (410 s, mit dem neuen assimp-Ziel) |

Befund (behoben in f6b20b9d): das Build-Menü reicht mit „Test in game“ bis an die Statuszeile;
der Klick des Selbsttests „in die Statuszeile (Menüs zu)“ traf bei 25 % der Breite nun das Menü
(riss es ab), die folgenden Klicks kamen nicht an, und im Modellbrowser wies ein Klick einem
leeren Ordner die „aktuelle“ Zelle des vorigen Ordners zu → Zugriff hinter `dateien`
(0xC0000005). Das ist ein alter Fehler im Modellbrowser (`modellZuweisen` ohne Grenzprüfung),
jetzt mit Prüfung; der Testklick geht an den linken Rand. Die Menügrößen-Tabelle (QMenu-Maße wie
NRC) kennt die neuen eigenen Einträge (File: Breite des längsten Eintrags 291, eine Zeile mehr;
Build: eine Zeile mehr). Die Änderung kam nach Neubau und ctest; sie betrifft nur die Oberfläche
und den Selbsttest, der danach 0 Fehler zeigt.

## Offene Punkte

- **Punkte 4 bis 12** (Sprungweite/Platz, NPC-Wege, verknüpfte Prefabs, Kurven-Werkzeug,
  Modelle verteilen, Gelände formen, Blender hin und zurück, Asset-Browser, automatisch
  speichern) sind nicht begonnen. Für Punkt 4 fehlt im Spieler-Pmove (`navSpielerPmove`) noch
  der Machtsprung (PM_CheckJump mit forceJumpHeight/-Strength je Stufe) – der müsste zuerst 1:1
  aus OpenJK portiert werden.
- „Im Spiel testen“ ist ohne echtes Spiel geprüft (nur die Befehlszeile, wie verlangt); ein
  Handtest je Engine (jasp mit fs_cdpath, Movie Duels mit fs_homepath) steht aus.
- MD3-Shader: `rgbGen lightingDiffuse` wie die Modelle von Raven; ob ein per misc_model
  eingebackenes Modell damit im Spiel richtig hell ist, zeigt erst der Handtest.
- Der Import-Ordner muss für eine Veröffentlichung von Hand in die Mod/pk3 (das Handbuch sagt es);
  „Pack map as .pk3“ nimmt ihn noch nicht mit.
- Vorschau der Geometrie: die Tabelle zeigt Zahlen; eine 3D-Vorschau vor dem Einfügen gibt es nicht
  (Einfügen + Undo ist der Weg).

## Changelog

## English

- Map editor: **Build → Test in game** starts Jedi Academy SP/MP, OpenJK, Movie Duels or Jedi Outcast with the current map (`+devmap`). It takes the current quick bake or an up-to-date .bsp, or builds first; the map goes into a test folder outside the game, which the game finds through a start parameter (`fs_cdpath`, or `fs_homepath` when the game already has a map of that name). Only if neither works does it ask before writing into a mod folder.
- Map editor: **File → Import geometry as brushes...** turns .obj, .fbx and .glb/.gltf models into brushes - planar faces merged, convex decomposition, UV as texture alignment, hidden faces caulk, decoration detail; per part brushes, model + clip or Auto, with a preview, one undo step and the optimizer afterwards. A .bsp of another id Tech 3 game (Quake III, RTCW, Enemy Territory, Warsow) opens with that game's reader.
- Map editor: drag an .obj, .fbx or .glb file onto the editor and it becomes an MD3 with its own shader and textures, inserted as misc_model under the mouse pointer.
- Fixed a crash in the model browser when clicking into an empty folder.

## Deutsch

- Karten-Editor: **Build → Im Spiel testen** startet Jedi Academy SP/MP, OpenJK, Movie Duels oder Jedi Outcast mit der aktuellen Karte (`+devmap`). Es nimmt das aktuelle Schnell-Backen oder eine aktuelle .bsp, sonst wird zuerst gebaut; die Karte kommt in einen Testordner außerhalb des Spiels, den das Spiel über einen Startparameter findet (`fs_cdpath`, oder `fs_homepath`, wenn das Spiel schon eine Karte mit diesem Namen hat). Nur wenn beides nicht geht, fragt es, bevor es in einen Mod-Ordner schreibt.
- Karten-Editor: **File → Geometrie als Brushes importieren...** macht aus .obj-, .fbx- und .glb/.gltf-Modellen Brushes - planare Flächen zusammengefasst, konvex zerlegt, UV als Texturausrichtung, verdeckte Seiten caulk, Deko als detail; je Teil Brushes, Modell + Clip oder Auto, mit Vorschau, einem Undo-Schritt und danach dem Optimierer. Eine .bsp eines anderen id-Tech-3-Spiels (Quake III, RTCW, Enemy Territory, Warsow) öffnet mit dem Leser dieses Spiels.
- Karten-Editor: eine .obj-, .fbx- oder .glb-Datei auf den Editor ziehen, und sie wird ein MD3 mit eigenem Shader und Texturen, eingesetzt als misc_model unter dem Mauszeiger.
- Behoben: Absturz im Modellbrowser beim Klick in einen leeren Ordner.

## 中文

- 地图编辑器:**Build → 在游戏中测试** 用当前地图(`+devmap`)启动 Jedi Academy SP/MP、OpenJK、Movie Duels 或 Jedi Outcast。它使用当前的快速烘焙或最新的 .bsp,否则先编译;地图放入游戏之外的测试文件夹,游戏通过启动参数找到它(`fs_cdpath`;若游戏已有同名地图则用 `fs_homepath`)。只有两者都不可用时,才会在写入模组文件夹前询问。
- 地图编辑器:**File → 将几何体导入为画刷...** 把 .obj、.fbx 和 .glb/.gltf 模型转换为画刷——合并共面面、凸分解、UV 作为纹理对齐、被遮挡的面使用 caulk、装饰设为 detail;每个部件可选画刷、模型 + 碰撞或自动,带预览、一步撤销,之后打开优化器。其他 id Tech 3 游戏(Quake III、RTCW、Enemy Territory、Warsow)的 .bsp 会用该游戏的读取器打开。
- 地图编辑器:把 .obj、.fbx 或 .glb 文件拖到编辑器上,它会变成带自己着色器和纹理的 MD3,并作为 misc_model 插入到鼠标指针下。
- 修复:在模型浏览器中单击空文件夹时崩溃。

## 日本語

- マップエディター:**Build → ゲーム内でテスト** で、現在のマップ(`+devmap`)を使って Jedi Academy SP/MP、OpenJK、Movie Duels、Jedi Outcast を起動します。現在のクイックベイクか最新の .bsp を使い、なければ先にビルドします。マップはゲーム外のテストフォルダーに置かれ、ゲームは起動パラメーターでそれを見つけます(`fs_cdpath`、同じ名前のマップがすでにある場合は `fs_homepath`)。どちらも使えない場合だけ、MOD フォルダーに書き込む前に確認します。
- マップエディター:**File → ジオメトリをブラシとして読み込む...** で .obj・.fbx・.glb/.gltf のモデルをブラシにします。平面をまとめ、凸分解し、UV をテクスチャの向きにし、隠れた面は caulk、装飾は detail。部品ごとにブラシ・モデル + クリップ・自動を選べ、プレビュー付きで、元に戻すは 1 回、その後で最適化が開きます。ほかの id Tech 3 ゲーム(Quake III、RTCW、Enemy Territory、Warsow)の .bsp はそのゲームの読み込みで開きます。
- マップエディター:.obj・.fbx・.glb ファイルをエディターにドラッグすると、独自のシェーダーとテクスチャ付きの MD3 になり、マウスポインターの下に misc_model として挿入されます。
- 修正:モデルブラウザーで空のフォルダーをクリックすると落ちる問題。

## Integration rc606

Zweig `karte1` (Worktree `Y:\karte`), Ausgang cf77740f. Skript der Schlussrunde `SP/rc606/ende.sh`
(aus `SP/s9/ende_s9.sh`; Selbsttests über `SP/s8d/idle_jk.sh`: ein Prozess zur Zeit, Idle,
8-GB-Wächter), Summe `SP/rc606/ende_summe.txt`.

| Commit | Inhalt |
|---|---|
| c11a589d | Merge `hilfe1` (072c2907) – **keine Konflikte** (karte1 hatte seit 6fe2c104 nur Notizen und Selbsttest). Bau 0 Warnungen, ctest 152/152 |
| 372319e8 | Merge `menue1` (7fd2a4a5) – Konflikte in `tools/gen_i18n.py` und `tools/handbuch/gen_handbuch.py` (beide Seiten hängen ein Textmodul an: HILFE1 und MENUE1 behalten), `tools/handbuch/inhalt.py` (Seitenliste mit den Helfer-Seiten und `inhalt_menue`); erzeugte Dateien nicht gemischt: `i18n.h`/`i18n.cpp` (gen_i18n.py) und das ganze Handbuch samt `suche.js` (588 Dateien in Konflikt, gen_handbuch.py) neu erzeugt; `src/hilfe_orte.inc` war veraltet (`gen_hilfe_orte.py --check`) → neu, ein Ort mehr (Modusknopf „Menu Editor“). Bau 0 Warnungen, ctest 158/158 |
| e57f1c3b | Undo-Fund aus 9c (unten) |
| 60abb763 | Merge `licht9c` (d8bb392c) – Konflikte in `gen_i18n.py`/`gen_handbuch.py` (dazu KARTE9C), `i18n.*`, `handbuch/*/suche.js` neu erzeugt, Ortskatalog aktuell. Bau 0 Warnungen, ctest 158/158. Notizen der Lichtstufe: `NOTIZEN_karte9c_licht.md` |
| 3acebea0 | Schlussrunde: Menügröße Help (Test) |

### Undo nach „Fix all safe“ (offener Punkt aus 9c)

Gemeldet: auf md_rev_TarisUpper2 fehlten nach „Fix all safe“ und Undo in der Editor-Kamera Modelle
(Bäume, Felsen), bis Redo sie zurückbrachte; der Kartentext war richtig. Vermutet war ein nicht neu
aufgebauter Modell-Cache – **das ist es nicht**.

Selbsttest `karteneditor` mit `BHED_KARTE_NUR=optsicher` erweitert: nach „Fix all safe“ Undo und Redo
im Editor; geprüft werden die Dreiecke des Kamera-Teils „Rest“ (Punkt-Entities und Modelle), die
sichtbaren misc_model mit geladenem Modell, Stichproben (48 × 36) aus 32 Blicken je auf ein
misc_model (vor dem Fix zweimal für unruhige Stichproben, nach dem Fix, nach Undo, nach Redo;
2 × 2-Blöcke wie im übrigen Test) und der Kartentext nach dem Undo.

- **Rot** im alten Ablauf (`BHED_OPTSICHER_ALT=1`: wie 9c 1 s nach dem Öffnen weiter): Modelle
  vor/Fix/Undo/Redo **0/0/0/212**, Rest-Dreiecke 12/12/12/48814, Text nach Undo gleich – genau der
  Fund. Das Protokoll zeigt die Ursache: File > Open .bsp gewinnt die misc_model im Hintergrund
  zurück (hier 22–23 s); Fix und Undo kamen davor. Weil Undo die Geometrie wieder wie in der .bsp
  machte (gleicher Fingerabdruck), wurde das Ergebnis danach als neuer Schritt „Recover misc_model
  (212)“ übernommen – dabei leerte sich der Redo-Stapel („Redo: no redo available“). Die Modelle
  kamen also nicht vom Redo, sondern von der Rückgewinnung, die während der langen Blick-Durchgänge
  fertig wurde.
- **Behoben (Test):** der Test wartet vor „Karte merken“ auf das Ende der Rückgewinnung (so drückt
  der Anwender „Fix all safe“ auf der fertigen Karte); vorher waren auch die Blicke „vorher“ ohne
  die misc_model.
- **Grün** im selben Test (alle Blicke): Rest-Dreiecke 48814/48814/48814/48814, Modelle
  212/212/212/212, Text nach Undo gleich, 32 Modell-Blicke (31 verschieden), nach Undo 0 und nach
  Redo 0 Stichproben anders; dazu wie bisher 50 Blicke auf neu gecaulkte Flächen, 0 Stichproben wie
  im Spiel anders, Eckpunkte 0,000 gewandert – 14 OK, 0 FEHLER.

### Schlussrunde (Endstand 60abb763, Test-Tabelle dann 3acebea0)

| Schritt | Ergebnis |
|---|---|
| sauberer Neubau (`--clean-first`, /W4 /WX) | **0 Fehler, 0 Warnungen** (437 s) |
| ctest gesamt | **158/158** (208 s) |
| karteneditor voll (mit großer Karte) | 3192 OK, **1 FEHLER** (996 s, Spitze 1029 MB): „Menue Help: 227 x 178 wie NRC“, hier 247 × 202 – der Eintrag „Helper: what do you want to do?“ aus hilfe1 (eine Zeile mehr, längster Eintrag); nur die Maßtabelle des Tests (3acebea0), Fotolauf `BHED_KARTE_NUR=fotos` danach **78 OK, 0 FEHLER** |
| menueeditor | **25 OK, 0 FEHLER** |
| karteneditor + `BHED_KARTE_NUR=hilfe1` | **59 OK, 0 FEHLER** |
| Modus 1 | **2645 OK, 0 FEHLER** (433 s) |
| kuerzel | **199 OK, 0 FEHLER** |
| übrige 22 Modi | alle **0 FEHLER**, dieselben Zahlen wie Stufe 9 (karte 227, kartenfunde 53, fix604b 32, laserfalle 8, geschuetzanim 3, zielstrahl 2, effektgrund 1, figurdeform 1, jk2 15, tuerlauf 13, kamera 95, missionsbau 95, szenen 26, ton 34, npcklasse 12, gluehen 8, gras 13, spulen 13, figurstufen 8, missionsliste 11, rest605 6, modellimfesten 1) |
| optsicher (md_rev_TarisUpper2, alle Blicke; auf e57f1c3b vor dem licht9c-Merge) | **14 OK, 0 FEHLER** |
| Kartenprüfung (278 MD-Karten, gegen 8d) | alle Summen gleich (0 Abstürze, 0 nicht geladen, 0 D3D-Meldungen, 1 → 1 nicht gezeichnete Entity) |

### Offen

- Programmseitig unverändert (bewusstes Verhalten, Meldung KeRkGeaendert): wer während der
  Rückgewinnung die Geometrie ändert (auch „Fix all safe“), verliert deren Ergebnis; macht er es vor
  ihrem Ende rückgängig, kommt das Ergebnis als neuer Schritt und Redo ist weg. Vorschlag: die
  Behebungen des Optimierers sperren (mit Hinweis), solange die Rückgewinnung läuft – braucht einen
  neuen Text in vier Sprachen und einen Satz im Handbuch.
- Kein Handbuchbild zeigt das Help-Menü des Karten-Editors; nichts neu aufzunehmen.
