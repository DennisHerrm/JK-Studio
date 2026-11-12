# Notizen FIX612 (12.11.): Funde der Code-Prüfung seit rc611 - rot nachgestellt, behoben, grün

Worktree `Y:\p_ap7`, Zweig `p_fix612`, Basis karte1 73c18113f, abgegeben rebased auf karte1 1c6172246 (AP13S, AP13K, LCFIX, FIX612C und FIX612B drin). Quellen der Funde: `plaene/codepruefung_seit_rc611.md` (R2, R3, R4,
R6, R7, V1-V3) und `plaene/codepruefung_seit_rc611_p3.md` (P3-1 bis P3-4; P3-5 und folgende gehören FIX612C).
Begonnen von Fable 5.2 (Tests, Proben, Texte; Commit f4a10c7fd), fortgesetzt von Opus 5.5 (Rot-Lauf, Behebung, Grün-Lauf).

## Selbsttests

- `gui/selbsttest_fix612.inc`, eingehängt am Ende der Liste in `gui/selbsttest.cpp`:
  - `BHED_EDITORTEST=modellblock`, `BHED_MODELL_NUR=fix612` (alle Teile) bzw. `fix612r2`, `fix612p31`, `fix612p33`, `fix612n`
    (P3-2, P3-4, R6), `fix612r4` (R4), `fix612v` (V2, V3).
  - `BHED_EDITORTEST=karteneditor`, `BHED_KARTE_NUR=fix612`: R3 (a fremder Zwischenstand, b Verwerfen nach dem Verschieben,
    c "Karte wie angehalten" nach dem Verschieben).
- R7 steht in `gui/selbsttest_rest9.inc` (`BHED_EDITORTEST=updateleiste`) hinter "von Hand gesucht zeigt auch die übersprungene
  Version".
- Proben im Programm (zählen nur, ändern nichts): `ModellZeit::veralteteSequenz` (R2, zusätzlich am Ende der Leiste und in der
  Marker-Zeile), `fix612Probe().presetWahlVeraltet` (P3-1), `fix612Probe().platzHinterEnde` (P3-3).
- ctest `karte_bau` (tests/kartebau.cpp): zwei neue Prüfungen zu R3 (Bindung an den Ort, `bauPausePruefen`).

## Funde, rot und grün

| Fund | Rot (Bau f4a10c7fd + Tests, ohne Fix) | Grün (Bau mit Fix, rebased auf karte1 6f79e39f9) | Commit |
|---|---|---|---|
| R2 [H] Zeitleiste über alten `AnimEntry*` | A (wie Blender, Start = 3 getippt): 1 Bild mit der alten Liste; B (wie 3ds Max, Set Key bei offener Zeitkonfiguration): 1 Bild | A und B: 0 Bilder mit der alten Liste, Marker gesetzt (30293 -> 30295, 30 -> 28), Liste neu gelesen, Schlüssel gesetzt | R2 |
| P3-1 [H] Presets `b[-1]` | Absturz beim Klick auf "Gehen (erzeugt)" nach "Walk" (Ende mit 0xE06D7363, C++-Ausnahme aus der Kopie des `std::string` vor dem Feld) | Klick und Rechtsklick: gangWahl 0, bibliothekWahl -1, 0 Bilder mit der alten Wahl, kein Absturz | P3-1 |
| P3-3 [M] Platzliste "Leeren" hinter dem Ende | Absturz bei "Clear slot" auf dem ersten von drei Plätzen (0xC0000005) | 2 Plätze nach Leeren, 0 Zeilen hinter dem Ende, kein Absturz | P3-3 |
| R3 [M] Bau fortsetzen | a: das fremde Programm aus stand.json lief (fremde .bsp gebaut, 1 Bau, keine Meldung); b: nach dem Umbenennen bleibt der Ordner, "verworfen" gemeldet; c: "wie angehalten" baute an den alten Ort (.bsp am neuen Ort 0 Lumps, Zwischenstand bleibt) | a: keine fremde .bsp, 0 Bauten, Grund in der Konsole; b: Ordner am neuen Ort weg, kein Zwischenstand; c: .bsp am neuen Ort mit 18 Lumps, Zwischenstand weg, alter Ort leer, fertig gemeldet; ctest karte_bau und q3map2_baupause grün | R3 |
| R6 [N] Texte außerhalb der Übersetzung | de: "File not found", "Unable to find keyword 'loadmodel'", "No 'modelfile' keyword found!", "Brace mismatch error"; en: "nicht in den Archiven" | de: „die Datei lässt sich nicht öffnen“, „der Block „loadmodel“ fehlt“, „im Block „loadmodel“ fehlt „modelfile““, „die Klammern passen nicht zusammen (Zeile 3)“; en: „not found in the game files“ | R6 |
| R7 [N] Überspringen nach Suche von Hand | Leiste bleibt sichtbar, die Einstellung ist gesetzt | Leiste weg, Einstellung gesetzt; die nächste Suche von Hand zeigt sie wieder (updateleiste 75/0) | R7 |
| P3-2 [N] doppelte Bodenzeile | "Lowest point" 2 mal, 2 doppelte Kennungen im Popover | „Lowest point“ 1 mal, 0 doppelte Kennungen | P3-2 |
| R4 [N] Alt+R dreifach belegt | Aktionssuche nennt Alt+R neben "Texturen neu laden" auch wie 3ds Max im Modellieren und wie Blender in der Animation (2 FEHLER) | Suche nennt Alt+R nur in Ansehen/Prüfen (6/0) | R4 |
| P3-4 [N] Max -> Blender -> Max | `vierAnsichten` 0 nach der Rückkehr | vierAnsichten 1 | P3-4 |

### Was geändert ist

- **R2:** an der Wurzel - die Leiste hält keinen Zeiger in `g_app->animList` mehr über Aufrufe, die das Dokument ändern.
  `drawModellZeitleiste()` zeichnet mit einer Kopie der Sequenz des Bildanfangs (`sequenzDesBilds`, Dateivariable, kein
  Anfordern je Bild), `modellMarkerZeile()` holt die Sequenz erst nach ihrem `cfgAbgleich()` und hält sie als Kopie. Andere
  Halter eines `AnimEntry*` (kurven, rig, ik, physik, anim, mb3 Gruppe Animation) holen `sequenz()` je Funktion und rufen
  dazwischen keinen Abgleich - dort kein Fund.
- **P3-1:** die Wahl wird nach der Liste neu gelesen; der gewählte Eintrag der Bibliothek nur über einen geprüften Zeiger.
- **P3-3:** "Hierher" (Rechtsklick und Ziehen) und "Leeren" merkt die Schleife nur, ausgeführt wird nach der Schleife.
- **R3:** aus `stand.json` wird nichts gestartet und kein Ort übernommen:
  - `bauPauseLesen` bindet den Stand an den Ort der Karte von jetzt (`bauPauseAnOrt`); der Stamm aus der Datei steht nur noch
    in `stammInDatei` (Hinweis "die Karte wurde verschoben"). Endung der gebauten Datei nur .map/.reg; Texte aus der Datei
    für die Anzeige gekürzt und ohne Steuerzeichen.
  - "Karte wie angehalten" erzeugt die Befehle selbst aus dem Eintrag des eigenen Build-Menüs für die Kopien im Zwischenstand
    (dieselbe Kette wie `bauStarten`, herausgezogen nach `bauBefehleFuer`). Abgelehnt mit Grund (Knopf gesperrt, Grund im
    Fenster und im Tooltip): Eintrag fehlt; andere Schritte/Stufen als beim Anhalten; ein offener Schritt startet nicht den
    eigenen `bhed_q3map2.exe`; der Stand nennt ein Programm, das kein bhed_q3map2 ist. Andere Schalter: nur ein Hinweis in der
    Konsole. Der Tooltip des Knopfs zeigt die Befehle, die laufen werden.
  - Verwerfen meldet, wenn der Ordner sich nicht löschen ließ. `bauPauseAltFertig` schiebt `<name>.map`/`.reg` nie neben die Karte.
  - ctest `q3map2_baupause`: das Werkzeug `kartepause befehlealt` spielt die Kopien selbst nach (`bauPauseBefehleAlt` gibt es
    nicht mehr).
- **R6:** `szeneLesen` meldet zusätzlich die Art des Fehlers (`SzeneFehler`), der Text von ModView bleibt fürs Protokoll; die
  Meldung im Model-Tab kommt aus übersetzten Texten. `oeffneModellAusArchiv`: "nicht in den Spieldateien gefunden" übersetzt.
- **R7:** wie G2C (`drawUpdateBanner`: UpdSkip setzt `skippedUpdate` und `updateBannerHidden_`) blendet "Diese Version
  überspringen" die Leiste auch aus; die nächste Suche von Hand zeigt sie wieder.
- **P3-2:** wie Blender steht die Zeile Bodenhöhe/Tiefster Punkt nur noch unter den Hilfslinien.
- **P3-4:** `modellBedienungSetzen` gleicht zuerst die vier Ansichten je Bereich ab (dieselbe Funktion wie jedes Bild,
  `modellPrAnsichtenAbgleich`), dann gilt wie Blender eine Ansicht.

## R4 (Alt+R) - Verhalten wie im Vorbild, Anzeige berichtigt (Entscheidung B2)

Belegt am Code (`gui/app_modelltab.cpp`, Haupttabellen wie 3ds Max und wie Blender: Alt+R = Texturen neu laden;
`gui/wof/app_modelltab_mesh.cpp`: wie 3ds Max im Bereich Modellieren Alt+R = Ring; `gui/app_modelltab_rig.cpp`: wie Blender im
Bereich Animation Alt+R = `pose.rot_clear`). Die Bereichstabellen kommen in `modellTasten` vor der Haupttabelle. Das Verhalten
folgt so in jedem Bereich seinem Vorbild (3ds Max Editable Poly: Select Ring Alt+R; Blender Pose: Clear Rotation Alt+R; ModView:
Refresh textures Alt+R). **Entscheidung B2 (shank 12.11., `plaene/entscheidungen_1211.md`):** das Verhalten bleibt, die angezeigte
Taste wird berichtigt.

- Wo die Taste stand: in der Aktionssuche "Reload textures (Alt+R)" in jedem Bereich (der Text trägt die Taste). Der Knopf
  "Texturen neu laden" mit Alt+R steht nur in Ansehen/Prüfen - dort stimmt er.
- Rot (`fix612r4`, Bau mit Test ohne Fix): wie 3ds Max im Modellieren und wie Blender in der Animation zeigt die Suche
  "Reload textures (Alt+R)" (2 FEHLER), in Ansehen/Prüfen richtig (2 OK).
- Fix: `modellMeshHatTaste` und `modellRigHatTaste` beantworten, ob der Bereich die Taste selbst nimmt (dieselben Bedingungen wie
  ihre Tastenfunktionen, ohne Druck); `tastenZeile` lässt dann die Klammer mit der Taste am Ende des Texts weg (Suche und
  Meldung). Über die Suche bleibt "Texturen neu laden" in jedem Bereich ausführbar.
- Grün (`fix612r4`): wie 3ds Max im Modellieren und wie Blender in der Animation "Reload textures", in Ansehen/Prüfen
  "Reload textures (Alt+R)" (6/0); Modellieren `ap6` 95/0, `ap10` 72/0, `ap4` 106/0 mit diesem Bau.

## Vermutungen der ersten Prüfung (V1-V3) - nicht nachstellbar, nichts geändert

- **V2** (Seitenleiste wie Blender, gleiche Kennung "ort0" für Griff und Fläche): Im Programm nachgestellt (`fix612v`): Griff
  Becken gewählt, Feld Position X angeklickt, Strg+A, "7.5" getippt, kein Enter - `rigZieht` bleibt 0 (`WantTextInput` 1). Ein
  getipptes Feld öffnet keinen Zug am Griff; danach ein Klick ins Leere der Ansicht bzw. auf die Fläche torso im Outliner: `rigZieht`
  0. Ein Zug per Maus endet mit dem Loslassen, bevor ein anderer Klick möglich ist. Der vermutete Weg ist nicht erreichbar.
- **V3** (Zwischenspeicher des Explorers mit Schlüssel Adresse + Anzahl): Nachgestellt mit zwei neuen Listen gleicher Länge, die
  zweite in umgekehrter Folge und nachweislich an der Adresse der ersten ("Adresse gleich", 2635 Einträge): die Gruppe FACE_ zeigt
  danach weiter 9 von 9 FACE_-Sequenzen. Nicht nachstellbar.
- **V1** (Direct3D 12, `gesendeteNachrichten()` nach 2 s Warten): erreichbar nur, wenn die Grafikkarte mindestens 2 s hängt UND
  in genau diesem Fenster ein anderer Faden/Prozess das Fenster in der Größe ändert (nur dann kommt WM_SIZE als gesendete
  Nachricht an; eigene Größenänderungen laufen im Faden selbst). Beides lässt sich im Selbsttest nicht ohne Eingriff in den
  Abgeber herbeiführen; der Abgeber gehört in dieser Welle D3D12-3. Nicht nachgestellt, nichts geändert; Hinweis an D3D12-3.

## Handbuch

- Seite karte_teilbau, Abschnitt "Bauen anhalten und fortsetzen" (`tools/handbuch/inhalt_vis2c.py`, Absatz zu "Karte wie
  angehalten", 6 Sprachen): die Befehle kommen aus dem Build-Menü, aus dem gesicherten Ordner wird nichts gestartet; wann der
  Knopf gesperrt ist; ein verschobener Kartenordner. Kein neues Bild (das Fenster sieht ohne Grund aus wie bisher).
- R7: der Text der Update-Leiste (`inhalt_rest9.py`) sagt schon "zeigt sie für diese Version nie wieder" - jetzt stimmt er auch
  nach einer Suche von Hand. Unverändert.

## Schlussrunde des Zweigs

Last-Regel ab 12.11. abends: Bau `--parallel 2`, Leerlauf-Priorität, ein Pool mit höchstens 2 jkstudio.

- Bau: ohne Warnungen (/W4 /WX), auf 6f79e39f9 (bau_gruen2, alle Ziele, 0 Warnungen) und nach dem Rebase auf 82848b14e (bau_end) und auf 499527165 (bau_end2) 133c47078 (bau_end3) und 1c6172246 (bau_end4), alle Ziele, je 0 Warnungen.
- ctest: alle 259 grün auf 6f79e39f9 (254 + karte_bau, q3map2_baupause, modell_szene, handbuch_sprachen, handbuch_suche);
  nach dem Rebase karte_bau, q3map2_baupause, modell_szene, modell_rigformen, handbuch_* 6/6.
- Eigener Selbsttest auf 6f79e39f9: modellblock `fix612` 21/2 (die 2 waren die alte Vorbedingung von V2, danach als
  "nicht erreichbar" festgehalten: `fix612v` 8/0), `fix612r4` 6/0, karteneditor `fix612` 7/0, updateleiste 75/0.
  Nach dem Rebase (82848b14e und 499527165): modellblock `fix612` (alle Teile mit r4 und v) je 27/0, karteneditor `fix612` je 7/0.
- Berührte Gruppen (auf 6f79e39f9): mb_ap1 143/0 (Zeitleiste, R2), mb_ap10 72/0 (Presets, Plätze, P3-1/P3-3; mit R4 noch
  einmal 72/0), mb_ent2 23/0 (Presets), mb_ap5 121/0 (Ansehen/Prüfen, R6, P3-4), mb_kopf 117/0 und mb_raster 42/0 (Overlays,
  P3-2), mb_ap4 106/0 (wie Blender; mit R4 noch einmal 106/0), mb_ap6 95/0 (Modellieren, R4), ke_bau (stufe3-5) 613/0
  (bauStarten über `bauBefehleFuer`), vis2c drei Starts in einem Laufordner 9+10+8/0 (Anhalten/Fortsetzen, R3).
  Nach dem Rebase auf 82848b14e: mb_ap13k 66/0 (Knochen, Durchsicht Alt+X/Alt+Z, Kopfzeile - neben R4 und P3-2). Auf
  499527165 (LCFIX berührt Bauen, Licht-Cache, VIS-Zwischenstand): vis2c 9+10+8/0; ctest q3map2_baupause, q3map2_teilbau
  (4 Tests), q3map2_lichthash, q3map2_lichtabdruck, q3map2_visschluessel, karte_bau, modell_szene, handbuch_* 12/12.
  Auf 133c47078 (FIX612C im selben Model-Tab-Code: Aktionssuche, modellBedienungSetzen, modelltab.h): modellblock `fix612` 27/0, `fix612c` 36/0 (ohne
  Schritte von FIX612), karteneditor `fix612` 7/0; ctest modell_rig, modell_rigformen, modell_szene, karte_bau, handbuch_* 6/6.
  Zusammengeführt mit FIX612C: in der Suche wie 3ds Max gilt beides - die Zeile ohne Taste (R4) und im Painting keine Zeile
  für [ und ] (P3-9); `modellBedienungSetzen` gleicht erst die vier Ansichten ab (P3-4) und nimmt dann das Werkzeug Knochen
  wählen zurück (P3-6). `BHED_MODELL_NUR=fix612c` startet die Schritte von FIX612 nicht mehr mit (nur fix612 und seine Teile).
  Auf 1c6172246 (FIX612B: modelltab.h, Listen in selbsttest.cpp, gen_i18n, Sprachdateien; mb3, mesh und app_modelltab.cpp
  ohne Konflikt zusammengeführt): modellblock `fix612` 27/0, `fix612c` 36/0, `fix612b` 39/0 (beide ohne
  Schritte von FIX612), karteneditor `fix612` 7/0.
- Bewusst nicht gelaufen: die volle Runde (macht der Hauptchat), Sprachläufe, Fotos (kein Bild ändert sich außer dem Grund im
  Fenster "Bauen fortsetzen", der nur bei einem abgelehnten Stand erscheint; umgebrochener Text in einem Fenster mit fester
  Breite und freier Höhe), unter Direct3D 11 (nichts am Zeichnen geändert).
- Lints: alle ohne neue Beanstandung (pruefcodes 11, portability 101 nach dem Rebase; aus den Dateien von FIX612
  nur `tests/kartebau.cpp` std::size_t ohne <cstddef>, das stand schon vorher da).
- Adressen: `git log -p karte1..p_fix612` ohne private Adressen.

## Offen / Fragen an shank

1. V1 (Direct3D 12, Wiedereintritt über WM_SIZE nach 2 s Warten) bleibt eine Vermutung für D3D12-3.

## Changelog

## English

- Map editor, resume a paused build: "Map as paused" takes its commands from the entry of the Build menu and starts only the compiler of JK Studio - nothing written in the saved folder is run. If the entry has other steps now, the button is greyed out and says why; the tooltip lists the commands. After moving or renaming the map folder, the saved state counts at the new place (Discard, resume and the results).
- Model tab, timeline: changing Start/End (like Blender) or setting a key while the time configuration is open no longer reads freed memory (possible crash).
- Model tab, Presets: clicking "Walk (generated)" or "Run (generated)" while a preset of the library is chosen no longer crashes.
- Model tab, target GLA "own GLA": "Clear slot" on a slot that is not the last no longer crashes.
- Model tab, like Blender: the Overlays popover shows the floor height row once (the second "Lowest point" did nothing).
- Model tab: switching like 3ds Max -> like Blender -> like 3ds Max keeps the four views of the area.
- Model tab: errors when loading a ModView script, and "not found in the game files", appear in the language of the interface.
- Update bar: "Skip this version" also hides the bar after Help > Check for updates.
- Model tab: the action search shows Alt+R next to "Reload textures" only where the key reloads textures - like 3ds Max in Modelling Alt+R selects a ring, like Blender in Animation it clears the rotation.

## Deutsch

- Karten-Editor, angehaltenen Bau fortsetzen: „Karte wie angehalten“ nimmt die Befehle aus dem Eintrag des Build-Menüs und startet nur den Compiler von JK Studio - nichts, was im gesicherten Ordner steht, wird ausgeführt. Hat der Eintrag inzwischen andere Schritte, ist der Knopf ausgegraut und nennt den Grund; der Tooltip zeigt die Befehle. Nach dem Verschieben oder Umbenennen des Kartenordners gilt der Zwischenstand am neuen Ort (Verwerfen, Fortsetzen und die Ergebnisse).
- Model-Tab, Zeitleiste: Start/Ende ändern (wie Blender) oder einen Schlüssel setzen, während die Zeitkonfiguration offen ist, liest keinen freigegebenen Speicher mehr (möglicher Absturz).
- Model-Tab, Presets: ein Klick auf „Gehen (erzeugt)“ oder „Laufen (erzeugt)“, während ein Preset der Bibliothek gewählt ist, stürzt nicht mehr ab.
- Model-Tab, Ziel „eigene GLA“: „Platz leeren“ auf einem Platz, der nicht der letzte ist, stürzt nicht mehr ab.
- Model-Tab, wie Blender: das Popover Overlays zeigt die Zeile Bodenhöhe nur einmal (der zweite „Tiefster Punkt“ tat nichts).
- Model-Tab: wie 3ds Max -> wie Blender -> wie 3ds Max behält die vier Ansichten des Bereichs.
- Model-Tab: Fehler beim Laden eines ModView-Skripts und „nicht in den Spieldateien gefunden“ erscheinen in der Sprache der Oberfläche.
- Update-Leiste: „Diese Version überspringen“ blendet die Leiste auch nach Hilfe > Nach Updates suchen aus.
- Model-Tab: die Aktionssuche nennt Alt+R neben „Texturen neu laden“ nur dort, wo die Taste Texturen neu lädt - wie 3ds Max im Modellieren wählt Alt+R einen Ring, wie Blender in der Animation setzt sie die Drehung zurück.

## 中文

- 地图编辑器,继续被暂停的编译:“暂停时的地图”从 Build 菜单的条目取得命令,只启动 JK Studio 的编译器——不会执行保存文件夹中写入的任何内容。如果该条目现在的步骤不同,按钮会变灰并说明原因;工具提示列出这些命令。移动或重命名地图文件夹后,保存的状态在新位置有效(丢弃、继续和结果)。
- 模型标签页,时间轴:修改开始/结束(Blender 方式)或在时间配置打开时设置关键帧,不再读取已释放的内存(可能崩溃)。
- 模型标签页,预设:选中库中的预设时点击“行走(生成)”或“奔跑(生成)”不再崩溃。
- 模型标签页,目标 GLA“自己的 GLA”:对不是最后一个的槽位执行“清空槽位”不再崩溃。
- 模型标签页,Blender 方式:Overlays 弹出框中地面高度一行只显示一次(第二个“最低点”没有作用)。
- 模型标签页:3ds Max 方式 -> Blender 方式 -> 3ds Max 方式切换后,该区域保留四个视图。
- 模型标签页:加载 ModView 脚本时的错误以及“在游戏文件中未找到”以界面语言显示。
- 更新栏:在 帮助 > 检查更新 之后,“跳过此版本”也会隐藏更新栏。
- 模型标签页:动作搜索只在 Alt+R 确实重新加载纹理的地方于“重新加载纹理”旁显示 Alt+R——3ds Max 方式下在建模中 Alt+R 选择环,Blender 方式下在动画中它清除旋转。

## 日本語

- マップエディター、一時停止したビルドの再開: 「一時停止時のマップ」は Build メニューのエントリーからコマンドを作り、JK Studio のコンパイラだけを起動します。保存フォルダーに書かれた内容は何も実行しません。エントリーのステップが変わっていると、ボタンは灰色になり理由を示します。ツールチップにはコマンドが表示されます。マップのフォルダーを移動または名前変更したあとは、保存した状態は新しい場所で使われます(破棄、再開、結果)。
- モデルタブ、タイムライン: 開始/終了の変更(Blender 方式)や、時間設定を開いたままのキー設定で、解放済みのメモリを読まなくなりました(クラッシュの可能性)。
- モデルタブ、プリセット: ライブラリーのプリセットを選んだまま「歩き(生成)」または「走り(生成)」をクリックしてもクラッシュしなくなりました。
- モデルタブ、ターゲット GLA「自分の GLA」: 最後ではないスロットで「スロットを空にする」を使ってもクラッシュしなくなりました。
- モデルタブ、Blender 方式: Overlays ポップオーバーの床の高さの行が 1 回だけ表示されます(2 つ目の「最も低い点」は何もしませんでした)。
- モデルタブ: 3ds Max 方式 -> Blender 方式 -> 3ds Max 方式に切り替えても、その領域の 4 つのビューが保たれます。
- モデルタブ: ModView スクリプトを読み込むときのエラーと「ゲームファイルに見つかりません」が、インターフェースの言語で表示されます。
- アップデートバー: ヘルプ > アップデートを確認 のあとでも、「このバージョンをスキップ」でバーが隠れます。
- モデルタブ: アクション検索では、Alt+R が実際にテクスチャを再読み込みする場所でだけ「テクスチャを再読み込み」の横に Alt+R を表示します。3ds Max 方式のモデリングでは Alt+R はリングを選択し、Blender 方式のアニメーションでは回転をクリアします。

## Español

- Editor de mapas, reanudar una compilación pausada: «Mapa como se pausó» toma los comandos de la entrada del menú Build y solo inicia el compilador de JK Studio; no se ejecuta nada de lo escrito en la carpeta guardada. Si la entrada tiene ahora otros pasos, el botón aparece en gris y dice por qué; la descripción emergente muestra los comandos. Tras mover o renombrar la carpeta del mapa, el estado guardado vale en el lugar nuevo (descartar, reanudar y los resultados).
- Pestaña Model, línea de tiempo: cambiar Inicio/Fin (como Blender) o poner una clave con la configuración de tiempo abierta ya no lee memoria liberada (posible cierre).
- Pestaña Model, presets: hacer clic en «Caminar (generado)» o «Correr (generado)» con un preset de la biblioteca elegido ya no cierra el programa.
- Pestaña Model, GLA de destino «GLA propia»: «Vaciar ranura» en una ranura que no es la última ya no cierra el programa.
- Pestaña Model, como Blender: el panel emergente Overlays muestra la fila de altura del suelo una sola vez (el segundo «Punto más bajo» no hacía nada).
- Pestaña Model: cambiar como 3ds Max -> como Blender -> como 3ds Max conserva las cuatro vistas del área.
- Pestaña Model: los errores al cargar un script de ModView y «no se encuentra en los archivos del juego» aparecen en el idioma de la interfaz.
- Barra de actualización: «Omitir esta versión» también oculta la barra después de Ayuda > Buscar actualizaciones.
- Pestaña Model: la búsqueda de acciones muestra Alt+R junto a «Recargar texturas» solo donde la tecla recarga las texturas; como 3ds Max en Modelado Alt+R selecciona un anillo, como Blender en Animación borra la rotación.

## Čeština

- Editor map, pokračování pozastaveného sestavení: „Mapa jako při pozastavení“ bere příkazy z položky nabídky Build a spouští jen kompilátor JK Studio - nic, co je zapsáno v uložené složce, se nespustí. Má-li položka teď jiné kroky, tlačítko je šedé a uvádí důvod; tooltip ukazuje příkazy. Po přesunutí nebo přejmenování složky mapy platí uložený stav na novém místě (zahodit, pokračovat a výsledky).
- Karta Model, časová osa: změna začátku/konce (jako Blender) nebo nastavení klíče při otevřené konfiguraci času už nečte uvolněnou paměť (možný pád).
- Karta Model, předvolby: kliknutí na „Chůze (generovaná)“ nebo „Běh (generovaný)“, když je vybraná předvolba z knihovny, už nezpůsobí pád.
- Karta Model, cílová GLA „vlastní GLA“: „Vyprázdnit slot“ na slotu, který není poslední, už nezpůsobí pád.
- Karta Model, jako Blender: vyskakovací panel Overlays ukazuje řádek výšky podlahy jen jednou (druhý „Nejnižší bod“ nic nedělal).
- Karta Model: přepnutí jako 3ds Max -> jako Blender -> jako 3ds Max zachová čtyři pohledy oblasti.
- Karta Model: chyby při načítání skriptu ModView a „nenalezeno v souborech hry“ se zobrazují v jazyce rozhraní.
- Pruh aktualizace: „Přeskočit tuto verzi“ skryje pruh i po Nápověda > Zkontrolovat aktualizace.
- Karta Model: hledání akcí ukazuje Alt+R vedle „Znovu načíst textury“ jen tam, kde klávesa textury opravdu znovu načte - jako 3ds Max v modelování Alt+R vybírá prstenec, jako Blender v animaci ruší rotaci.
