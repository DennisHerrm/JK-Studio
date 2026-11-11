# Notizen AP9 (11./12.11.): rechtes Feld des Bereichs Painting nach dem freigegebenen Entwurf - MBENTWURF Teil B, rc612

Paket AP9 aus `auftrag_mbentwurf_b_1011.md`: die Zeilen T0a-T11 der Abgleichtabelle (Abschnitt 11, Bereich Painting). Gebaut auf dem
Rollout-Baustein von AP3 (`gui/ui_rollout.h`, Schnittstelle unverändert). Begonnen von AP9 (Fable 5.2, drei WIP-Stände), fertig gemacht
von Opus 5.5 nach `Y:\werkzeuge\ap9\UEBERGABE.md` (Testfehler, Feinschliff, Läufe, Fotos, Handbuchbilder, Notizen).

## Was sich geändert hat (`gui/app_modelltab_malen.cpp`)

- Das rechte Feld zeichnet fünf Gruppen über den Baustein, in der Reihenfolge des Entwurfs: **Ziel**, **Werkzeuge**, **Pinsel**,
  **Ebenen**, **Für das Spiel**; dazwischen der Tipp-Kasten unter den Ebenen.
- **Ziel:** Shader als Auswahl in einer Formularzeile (vorne um ganze Ordner gekürzt, der ganze Pfad im Tooltip), Stufe des Shaders,
  Bild und Format als Tabelle, darunter Bild zeigen / Auf neue UVs backen im Raster. Kurzinfo = Dateiname des Bilds.
- **Werkzeuge:** sieben Knöpfe mit Symbol im Raster aus zwei Spalten (zuerst die fünf des Entwurfs: Malen, Füllen, Klonen, Verwischen,
  Maske, dann Weichzeichnen, Radieren); das gewählte ist der Hauptknopf. Kurzinfo = Vorbild (Viewport Canvas / Texture Paint).
- **Pinsel:** Farbe (zwei Farbfelder, Tauschen, Aufnehmen als Symbolknöpfe), Radius, Stärke, Abstand als Regler-Zeilen; je Werkzeug
  Toleranz (Füllen) bzw. Freigeben (Maske); Härte, Spiegeln mit Achse, Stiftdruck, Rückseiten, Ausbluten unter „Erweitert“ (ab Werk zu).
  Kurzinfo: wie 3ds Max „[ ] Größe“, wie Blender „F Radius · Umschalt+F Stärke“.
- **Tasten [ und ]** machen den Pinsel einen Schritt kleiner/größer (Faktor 0,9 bzw. 1/0,9, mit Wiederholung; Blender
  `brush.scale_size`, km_image_paint) - in beiden Bedienmodi, auch in Alle Aktionen suchen und in der Statuszeile (`tools/i18n_ap1.py`:
  MzStMaxPaint / MzStBlPaint).
- **Ebenen:** Zeilen über die Breite mit Auge (Umschalter) bzw. Schloss am Original, gewählte Zeile in der Betonungsfarbe; Mischart und
  Deckkraft der gewählten Ebene als Formularzeilen; acht Knöpfe im Raster (Neue Ebene, Nach unten vereinen zuerst).
- **Für das Spiel:** Ausgabeordner als Formularzeile mit Feld und „…“ (Platzhalter zeigt das Ende des Pfads), Hauptknopf Für das Spiel
  bauen, Zahl der bemalten Bilder als Tabellenzeile und als Kurzinfo. Wie 3ds Max zusätzlich im Reiter Dienstprogramme, wie Blender unten
  im Reiter Werkzeug; Ziel wie Blender zusätzlich im Reiter Material.
- Sehr schmales Feld (Mindestgröße 1280 × 720): passt auch „.../datei“ nicht, wird hinten mit „...“ gekürzt statt abgeschnitten; die
  Stufe zeigt dann „1: .../datei“ (die Beschriftung der Zeile nennt die Stufe).
- Zwölf neue Symbole (`UiIcon::Mal*`, Block am Ende), 31 Texte in `tools/i18n_ap9.py`; sechs unbenutzte alte Texte aus
  `tools/i18n_paint.py` entfernt (`PtRadiusFmt`, `PtStaerkeFmt`, `PtHaerteFmt`, `PtAbstandFmt`, `PtToleranzFmt`, `PtBemaltZahl`;
  lint_i18n vorher 6, jetzt ok).

## Stand je Zeile der Abgleichtabelle (für den Hauptchat; die Tabelle hat AP9 nicht angefasst)

| Zeile | Stand |
|---|---|
| T0a Aufbau wie 3ds Max | gleich: Reiter Ändern, Reihenfolge, Stil (flache Balken, eckig, +/−); offen und nicht AP9: vier Ansichten (R7) |
| T0b Aufbau wie Blender | gleich: Reiter Werkzeug, Panels abgerundet; offen und nicht AP9: Werkzeugleiste links, Ansichtskopf „Texture Paint“ (AP4) |
| T1 Ziel | gleich (Tabelle, an erster Stelle); mehr: Stufe des Shaders, Auf neue UVs backen |
| T2 Werkzeuge | gleich (Raster mit Symbol, gewähltes als Hauptknopf); mehr: Weichzeichnen, Radieren |
| T3 Pinsel | gleich (eigene Gruppe, Kurzinfo je Modus, Regler, Tasten [ ]); mehr: Härte, Spiegeln, Stiftdruck, Rückseiten, Ausbluten unter Erweitert |
| T4 Ebenen | gleich (Auge, Schloss, Kurzinfo Zahl); mehr: Ebene hoch/runter/löschen, Maske zeigen/leeren/umkehren |
| T5 Für das Spiel | mehr (wie Vorschlag: Max Dienstprogramme, Blender Werkzeug unten) |
| T6 Tipp | bewusst anders: ein Tipp für Nutzer an derselben Stelle statt des Entwicklerhinweises |
| T7 Ansicht / 2D-Bild | gleich (war schon gleich) |
| T8 Tempo | gleich: Ergebnis bitgleich, Bildzeit nicht schlechter (unten) |
| T9 Blender-Tasten | gleich (war schon gleich; Umschalt+F im Test) |
| T10 kurze Zeitleiste | gleich (AP1) |
| T11 Texte | gleich: Ziel · Werkzeuge · Pinsel · Ebenen; der Name des Vorbilds steht als Kurzinfo der Gruppe Werkzeuge |

## Bewusst so (Entscheidung zur Frage an shank)

- **[ und ] im Bereich Painting gelten in BEIDEN Bedienmodi dem Pinsel.** Im Max-Modus überdecken sie dort das Zoomen der Ansicht von
  MBNAV ([ ] = Zoom wie 3ds Max) - so steht es im freigegebenen Entwurf („[ ] Größe“). Außerhalb von Painting zoomen [ ] weiter.
  Das echte Viewport Canvas nutzt Strg+Umschalt+Ziehen (Radius), Alt+Umschalt+Ziehen (Deckkraft), Strg+Alt+Ziehen (Härte) (Autodesk-Hilfe
  2017) - nicht gebaut. **Frage an shank:** so lassen (Entwurf), oder im Max-Modus Zoom behalten und die Ziehen-Kürzel bauen?

## Selbsttest `ap9` (rot -> grün)

- Rot (`ap9_rot1`, exe des Stands vor dem Umbau): 25 OK, 88 FEHLER (76 Zeilenprüfungen rot, 12 Folge-Klicks). Schon vorher grün (22):
  T7, T9, T10, Farben tauschen/aufnehmen, Zeile der Ebene wählen, Bauen.
- Grün (Endstand auf karte1 + AP3): **97 OK, 0 FEHLER** (`ap9_gruen5`).
- Testfehler im ersten grünen Lauf (90 / 9), behoben ohne eine Prüfung abzuschwächen:
  - „Breite“ (4): die Prüfung zählte das Kindfenster `mtrollouts` selbst und seinen Rollbalken (Kennung `GetWindowScrollbarID`, ohne
    Beschriftung) als Inhalt des Felds; beide ausgenommen - geprüft werden weiter alle Elemente darin (55-57).
  - Erweitert im Blender-Durchgang (5): der Max-Durchgang hatte Erweitert aufgeklappt (der Zustand gehört dem rollenden Bereich, nicht dem
    Bedienmodus); vor dem Blender-Durchgang wird zugeklappt, der Max-Durchgang prüft weiter den Werkszustand.

## Angepasste vorhandene Selbsttests (nichts gelöscht)

1. `selbsttest_pt.inc` „PT Befehlsfeld“: statt des Gruppentitels „Viewport Canvas“ jetzt die Gruppe „Tools“ mit dieser Kurzinfo.
2. `selbsttest_pt.inc` ptfix P3: der Deckkraft-Regler heißt `Opacity###ptdeck` und wird vor dem Klick ins Bild gerollt.
3. `selbsttest_pt.inc` ptlang fest: „das bemalte Bild ist gebaut“ erwartet `gebaut >= 1` statt `== 1` - schon auf der Basis rot (die
   Ansicht ist seit MBKOPF/AP1-AP3 anders groß, die festen Striche treffen auch den Kopf: „gebaut 2“; auf karte1 mit der exe von AP3
   ebenso rot, `ap9_fest_ap3`).
4. `selbsttest_pt.inc` ptlang, Zähler „ein Tupfer prüft nur die Dreiecke in seiner Nähe“: rot im Lauf mit 300 Strichen (17,4 %), obwohl
   je Tupfer wie vorher 78 Dreiecke geprüft wurden - der Anteil wurde gegen die Leinwand gerechnet, die am Ende des Blocks gewählt war
   (der letzte Strich traf head.jpg). Jetzt gegen die Leinwand jedes Tupfers (neuer Zähler `dreieckeLeinwand` in
   `include/bhed/modell/malen_mess.h`, eine Zeile in `src/modell/malen.cpp`, nur Zählung); Schwelle ein Zehntel unverändert. Danach 2,5 %.

## Tempo und Ergebnis (T8)

Gemessen allein (kein anderer Prozess, `BHED_D3D_DEBUG=0`), vorher = exe von AP3 auf karte1 (altes Malfeld), nachher = AP9:

| Lauf | vorher | nachher |
|---|---|---|
| ptlang fest (100 Striche): Bild / Datei | `aedd305898da9620` / `b04911c6508fcc7d` (3398275 Bytes) | gleich (bitgleich, 1163378 Texel, 2439 Tupfer, 5232 Kacheln) |
| ptlang 100: Bildzeit beim Malen | 1,37 ms | 1,31 ms |
| ptlang 100: Speicher privat | 1908 MB | 1904 MB |
| ptlang 300: Bildzeit (erste / letzte 100) | 2,91 ms (5,77 / 1,72; Lauf neben Selbsttests, Richtwert) | 1,60 ms (1,58 / 1,61) |
| ptlang 300: Zähler | Dreiecke 2,1 %, Fläche je Treffer 1,60, Maske 2× | 2,5 % (neuer Nenner), 1,59, Maske 2× |

ptlang 100 vorher/nachher je 8 / 0, ptlang 300 nachher 8 / 0. Skript `Y:\werkzeuge\ap9\mess1.sh`.

## Fotos (angesehen)

`Y:\werkzeuge\ap9\fotos\`: `v_ap9_fn2_en_max.png`, `v_ap9_fn2_en_blender.png` (Entwurf neben Programm, beide Modi);
`v_ap9_fn2_hell_ja_cs_zh.png` (helles Thema, Japanisch, Tschechisch, Chinesisch); `ap9_fn2_de`, `_es`; Mindestgröße
`v_ap9_fn2_klein.png`, `v_klein_seiten.png`, `v_klein_stufe2.png` (1328 × 844). Gesehen und behoben: Platzhalter des Ausgabeordners
zeigte nur den Anfang des Pfads; die Stufe war in der Mindestgröße abgeschnitten („Stage 1: moc“). Nichts abgeschnitten; lange Texte
(ja „新しい UV に焼き付け“) brechen um.

## Handbuch

Quelle `tools/handbuch/inhalt_ap9.py` (6 Sprachen, `einhaengen()` für die Seite modell-painting - eingehängt wird in AP14), Bilder
`h_ap9_max`, `h_ap9_blender` in 6 Sprachen (2538 × 1472, angesehen en/ja) unter `handbuch/bilder/<sprache>/`
(`BHED_EDITORTEST=handbuch BHED_HB_NUR=ap9`).

## Schlussrunde des Zweigs

| Schritt | Ergebnis |
|---|---|
| Bau alle Ziele (inkrementell, /W4 /WX) | 0 Fehler, 0 Warnungen |
| ctest | 256 / 256 |
| `ap9` | 97 / 0 |
| Malgruppen und Nachbarn (`ap9r_summe.txt`) | pt 42 · ptfix 6 · ptfix2 6 (ja) · ptfix3 13 · ptstufen 5 · ptsprung 4 · ap3 105 · ap1 137 · funktionen 62 · tasten 44 · blender 68 - je 0 FEHLER |
| ptlang fest / 100 / 300 | 7 / 0 · 8 / 0 · 8 / 0 |
| Fotos `ap9foto` | 8 Varianten (en, de, zh-Hans, ja, es, cs, hell, klein), je 0 FEHLER |
| Handbuchbilder | 6 Sprachen, je 0 FEHLER; `gen_handbuch.py --pruefen` ok |
| Lints (alle `tools/lint_*.py`) | ok; portability 99, pruefcodes 11, fremdprogramm 1, efxflags wie die Basis (karte1 + AP3) |

Bewusst nicht gelaufen (einmal auf dem zusammengeführten Stand): Modus 1, alle übrigen modellblock-Gruppen, karteneditor, menueeditor,
Rundläufe, sauberer Neubau.

## Offen

- Frage an shank zu [ ] im Max-Modus (oben).
- Vier Ansichten im Max-Modus (R7) und der Blender-Aufbau (Werkzeugleiste links, Ansichtskopf): nicht AP9 (R7, AP4).
- Wünsche an den Baustein (`Y:\werkzeuge\ap9\WUNSCH_AP3.md`, fünf Punkte: Listenzeile mit Zustand, Hauptknopf im Raster ohne Mehrhöhe,
  Symbolknopf, Beschriftung der Formularzeile im Element-Register, Ausklapper mit gekürztem Pfad) - örtlich in AP9 gelöst.
- Handbuch einhängen: AP14.

## Changelog

## English

- Model tab, Painting: the right panel follows the approved design. The groups Target, Tools, Brush, Layers and For the game stand in the order of the work; tools are a grid of buttons with symbols (the chosen one is highlighted), colour, radius, strength and spacing are rows with sliders, and hardness, mirroring, pen pressure, back faces and bleed are under Advanced.
- Layers are rows with an eye to show or hide them and a lock on the original; blend mode and opacity of the selected layer stand below the list, a tip box below the group.
- The keys [ and ] make the brush smaller and larger in both control modes (also in the search for all actions and in the status line).
- Like 3ds Max the groups stand in the tab Modify (For the game also under Utilities), like Blender in the tab Tool (Target also under Material).
- Long paths are shortened at the front (".../kyle/body.tga"), the output folder shows the end of its path; the full path is in the tooltip.

## Deutsch

- Model-Tab, Painting: das rechte Feld folgt dem freigegebenen Entwurf. Die Gruppen Ziel, Werkzeuge, Pinsel, Ebenen und Für das Spiel stehen in der Reihenfolge der Arbeit; die Werkzeuge sind ein Raster aus Knöpfen mit Symbol (das gewählte ist hervorgehoben), Farbe, Radius, Stärke und Abstand sind Zeilen mit Reglern, Härte, Spiegeln, Stiftdruck, Rückseiten und Ausbluten stehen unter Erweitert.
- Ebenen sind Zeilen mit einem Auge zum Ein- und Ausblenden und einem Schloss am Original; Mischart und Deckkraft der gewählten Ebene stehen unter der Liste, ein Tipp-Kasten unter der Gruppe.
- Die Tasten [ und ] machen den Pinsel in beiden Bedienmodi kleiner und größer (auch in der Suche nach allen Aktionen und in der Statuszeile).
- Wie 3ds Max stehen die Gruppen im Reiter Ändern (Für das Spiel auch unter Dienstprogramme), wie Blender im Reiter Werkzeug (Ziel auch unter Material).
- Lange Pfade werden vorne gekürzt („.../kyle/body.tga“), der Ausgabeordner zeigt das Ende seines Pfads; der ganze Pfad steht im Tooltip.

## 中文

- 模型选项卡,绘制:右侧面板遵循已批准的设计。“目标、工具、画笔、图层、用于游戏”这些分组按工作顺序排列;工具是带图标的按钮网格(所选工具高亮),颜色、半径、强度和间距是带滑块的行,硬度、镜像、笔压、背面和出血位于“高级”下。
- 图层是一行行条目,带有用于显示或隐藏的眼睛图标,原图带有锁;所选图层的混合模式和不透明度位于列表下方,分组下方有提示框。
- [ 和 ] 键在两种操作方式下都能缩小和放大画笔(在“搜索所有操作”和状态栏中也有)。
- 类似 3ds Max 时,这些分组位于“修改”选项卡(“用于游戏”也在“实用程序”下);类似 Blender 时位于“工具”选项卡(“目标”也在“材质”下)。
- 长路径从前面缩短(“.../kyle/body.tga”),输出文件夹显示其路径的末尾;完整路径在工具提示中。

## 日本語

- モデルタブ、ペイント: 右のパネルが承認済みのデザインどおりになりました。「ターゲット・ツール・ブラシ・レイヤー・ゲーム用」の各グループが作業の順に並び、ツールはアイコン付きボタンのグリッド (選択中のものを強調)、色・半径・強さ・間隔はスライダー付きの行、硬さ・ミラー・筆圧・裏面・にじみは「詳細設定」の下にあります。
- レイヤーは行として並び、目のアイコンで表示/非表示を切り替え、オリジナルには錠が付きます。選択中のレイヤーの描画モードと不透明度はリストの下に、ヒントの枠はグループの下にあります。
- [ と ] キーで、どちらの操作モードでもブラシを小さく/大きくできます (すべてのアクションの検索とステータス行にもあります)。
- 3ds Max 風ではグループは「修正」タブに (「ゲーム用」は「ユーティリティ」にも)、Blender 風では「ツール」タブに (「ターゲット」は「マテリアル」にも) あります。
- 長いパスは先頭側を省略し (「.../kyle/body.tga」)、出力フォルダーはパスの末尾を表示します。完全なパスはツールチップにあります。

## Español

- Pestaña Model, Pintura: el panel derecho sigue el diseño aprobado. Los grupos Destino, Herramientas, Pincel, Capas y Para el juego siguen el orden del trabajo; las herramientas son una cuadrícula de botones con símbolo (la elegida queda resaltada), color, radio, fuerza y espaciado son filas con deslizadores, y dureza, simetría, presión del lápiz, caras traseras y sangrado están en Avanzado.
- Las capas son filas con un ojo para mostrarlas u ocultarlas y un candado en el original; el modo de fusión y la opacidad de la capa elegida están bajo la lista, y un recuadro de consejo bajo el grupo.
- Las teclas [ y ] hacen el pincel más pequeño y más grande en ambos modos de control (también en la búsqueda de todas las acciones y en la línea de estado).
- Como 3ds Max, los grupos están en la pestaña Modificar (Para el juego también en Utilidades); como Blender, en la pestaña Herramienta (Destino también en Material).
- Las rutas largas se acortan por delante (".../kyle/body.tga") y la carpeta de salida muestra el final de su ruta; la ruta completa está en la información emergente.

## Čeština

- Karta Model, Malování: pravý panel odpovídá schválenému návrhu. Skupiny Cíl, Nástroje, Štětec, Vrstvy a Pro hru stojí v pořadí práce; nástroje tvoří mřížku tlačítek se symbolem (vybraný je zvýrazněn), barva, poloměr, síla a rozestup jsou řádky s posuvníky a tvrdost, zrcadlení, přítlak pera, odvrácené plochy a přesah jsou pod Rozšířené.
- Vrstvy jsou řádky s okem pro zobrazení či skrytí a se zámkem u originálu; režim prolnutí a krytí vybrané vrstvy stojí pod seznamem, rámeček s tipem pod skupinou.
- Klávesy [ a ] zmenšují a zvětšují štětec v obou režimech ovládání (také v hledání všech akcí a ve stavovém řádku).
- Jako 3ds Max stojí skupiny na kartě Upravit (Pro hru také pod Nástroje), jako Blender na kartě Nástroj (Cíl také pod Materiál).
- Dlouhé cesty se zkracují zepředu („.../kyle/body.tga“) a výstupní složka ukazuje konec své cesty; celá cesta je v popisku.
