# Notizen uebersetzung1: Spanisch und Tschechisch nachziehen (Stand karte1) + Sprach-Reste

Zweig `uebersetzung1` ab karte1 1247acc6, Worktree `Y:\sprache_es`, Bau `build-sprache`.
Auftrag: SP\auftrag_uebersetzung1.md. Skripte: `SP/u1/` (`idle.sh` nach int3 mit Bau und Handbuch dieses
Worktrees, `bilder.sh` Bildsätze, `modi.sh` Selbsttests je Sprache), Laufordner `Y:/es_arbeit/u1/laeufe`.
Unterbrechung: Windows Update startete den Rechner am 30.10. um 00:29 neu; was danach lief, ist verworfen
(die 93 geänderten Bilder geprüft: alle vollständig, die Bildläufe waren vorher fertig).

| Commit | Inhalt |
|---|---|
| 4df6cf08 | Punkt 1+2: 121 Programmtexte und 45 Handbuchtexte je Sprache |
| bfb1b96a | Punkt 4: Pluralformen (CLDR), lint_i18n liest gui/*.inc, Funde daraus, Stufe 4 sprachunabhängig |
| 092b19b9 | Punkt 3: Bilder es/cs; Menü-Editor-Spalten schneiden nichts mehr ab (mit Selbsttest-Prüfung) |
| (dieser) | Notizen, Schlussrunde |

## Zahlen: noch englisch je Sprache

| | es vorher | es nachher | cs vorher | cs nachher |
|---|---|---|---|---|
| Programmtexte (`lint_i18n.py --sprache`) | 121 | **0** | 121 | **0** |
| Handbuchtexte (`gen_handbuch.py --pflicht`) | 45 | **0** | 45 | **0** |
| Handbuchseiten mit englischen Texten | 9 | **0** | 9 | **0** |
| Handbuchbilder englisch | 9 | **0** | 9 | **0** |

## Punkt 1: Programmtexte

Die 121 Texte (karte9d 19, mission1 1, menue2 101) in `tools/i18n_sprache_es.py` / `_cs.py` als eigener
Block „uebersetzung1“, Begriffe nach den Glossaren der Sprach-Notizen (es: lienzo, elemento, paquete,
conjunto de menús, diagnóstico en vivo; cs: plátno, prvek, balíček, sada menu, živá diagnostika; NRC-Namen
und Spieltermini bleiben). Platz: s. Punkt 3 (Fotodurchsicht) – die Überläufe lagen im Layout des
Menü-Editors, nicht an den neuen Texten.

## Punkt 2: Handbuch

45 Texte je Sprache auf 9 Seiten; 6 davon gehörten zu altem englischem Wortlaut (die alten Einträge in
`inhalt_es.py`/`inhalt_cs.py` entfernt, der Hinweis „12 Einträge nicht mehr gebraucht“ ist weg). Zitierte
Programmtexte wie im Programm („No hay camino desde aquí hasta ...“, „Odsud nevede cesta k ...“).

## Punkt 3: Bilder

`SP/u1/bilder.sh`: `BHED_EDITORTEST=handbuch BHED_HB_NUR=karte7|karte9d|hilfe2` und `menueeditor`, je
`BHED_SPRACHE` es/cs, `BHED_FOTOS`, dann `tools/handbuch/bilder.py` mit allen Bildern des Laufs; jeder Lauf
0 FEHLER. Durchsicht (es/cs, zum Vergleich en): abgeschnitten waren im Menü-Editor die Suchzeile der rechten
Spalte (schon englisch „In all op|“, es „En todo|“, cs „Ve všec|“), der Hinweis der Bibliothek (schon
englisch), die Seitenverhältnis-Auswahl (cs „Roztáhnout (originál“) und die Kachel es „Asignación de tec“.
Behoben: die Zeile bricht um, wenn das nächste Element nicht passt; der Hinweis bricht um; die Auswahl ist so
breit wie ihr längster Eintrag; es „Asignar tecla“. Neue Prüfung im Selbsttest `menueeditor`: keine Spalte
des Menü-Editors breiter als ihr Platz (`ScrollMax.x` von `##links`/`##rechts`/`##codeBereich`) – mit dem
alten Layout auf Englisch **46 OK / 2 FEHLER**, danach en, es, cs je **48 OK / 0**. Die Menü-Editor-Bilder
danach in allen sechs Sprachen neu (je 48/0).

## Punkt 4: Sprach-Reste

- **Pluralformen** nach CLDR: `tools/sprachen.py MEHRZAHL` (en/de/es one/other, cs one/few/other, zh/ja
  other). Ein Zähltext schreibt seine Formen in Klammern: `"%d {file|files}"`, cs
  `"{Zkopírován|Zkopírovány|Zkopírováno} %d {příkaz|příkazy|příkazů}"`; `{2:...}` richtet sich nach der
  zweiten Zahl. `gen_i18n.py` prüft die Formenzahl je Sprache, erzeugt alle Fassungen (Tabelle
  `kZaehltexte`) und `pluralForm`; `i18n::trN(id, n...)` / `trKN` (Karten-Editor) wählen nach der Regel der
  Sprache, deren Text es ist (fehlt der Text: Englisch mit englischer Regel), `tr()` liefert „other“.
  112 Zähltexte umgestellt (Dateien, Schritte, Pinsel, Entities, Optimierer-Angaben, VIS-/GPU-Bericht ...;
  die „(s)“-Formen sind weg), alle Aufrufe auf trN/trKN; die Optimierer-Angaben nehmen die Zahlen aus ihren
  Werten. Bewusst nicht: `MmsgM014` (erscheint erst ab 33), `KeVisLang`, `KeGpuGrund*` (Schwellen weit über
  4), Texte „X von Y“ ohne Nomen. Test `sprache`: rot (kompilierte nicht; „%d command(s) copied“, cs
  „2 příkazů“), grün: CLDR-Regeln aller sechs Sprachen, Beispiele je Sprache, zwei Zahlen, und jeder Zähltext
  in jeder Sprache für 0/1/2/3/5/21 ohne Klammerrest mit den Platzhaltern des Englischen. `lint_i18n` meldet
  Zähltexte, die über tr()/trK() geholt werden; das Handbuch (`{{Kennung}}`) nimmt die Form „other“.
- **„vier Sprachen“**: im Handbuch schon von Integration 3 auf sechs gebracht (alle sechs Sprachen
  durchsucht, nichts mehr); `tools/release_paket.py` meldete noch „alle vier Sprachen“ → Zahl aus SPRACHEN.
- **„Window“** in `gui/app_menue.cpp`: in karte1 noch fest → `MeAufloesungFenster` in
  `tools/i18n_uebersetzung1.py` (Kennung und Wortlaut wie in menue3/M4, es „Ventana“, cs „Okno“). **Beim
  Zusammenführen mit menue3 steht die Zeile doppelt** (gen_i18n.py bricht mit „doppelte Kennung“ ab) – dann
  die Zeile in `i18n_uebersetzung1.py` löschen; die Codezeile ist in beiden Zweigen gleich.
- **lint_i18n** liest gui/*.inc, gui/*.h und src/** (ohne das erzeugte i18n.cpp): **694 → 0**
  Beanstandungen. Falschmeldungen weg (reine Formate wie „= %s“, „...##id“, Spielnamen, Such-Schalter „.*“/
  „Aa“/„ab“, .menu-Schlüsselwörter). Echte Funde: die Optimierer-Angabe KeOptD35 (9d) erschien nie übersetzt
  (fehlte in der Liste der Oberfläche) → ergänzt; das Namensfeld bei „Auswahl als Vorlage speichern“ hatte
  keine Beschriftung (MeVorlageName lag ungenutzt) → Platzhaltertext; nie benutzt und entfernt (in keinem
  Zweig benutzt): KeBrushCount, KePatchCount, KeEntityCount, KeBspKarte, KeBspQuelle, KeTgMaster, MeSpielFps.
- **Stufe-4-Pixelvergleiche**: die Plugin-Teile (bobtoolz, brushexport, kleine Plugins, MeshTex) entstehen
  und laufen mit „Begriffe wie NetRadiant“ – die Schritte werden mit den Beschriftungen gebaut, die der Lauf
  dann zeigt (daran war CS gescheitert); die Platzprüfung je Sprache bleibt in pluginTexte. Tschechisch
  `BHED_KARTE_NUR=stufe4 BHED_STUFE4_NUR=plugins`: Basis-exe (karte1) **183 OK / 6 FEHLER** (Intersect ×2,
  Make Chain, Path Plotter, brushexport, Get Info), neu **189 OK / 0**.
- **Übrige Selbsttests auf es/cs**: s. Schlussrunde und Offen.

## Schlussrunde

SCHLUSSRUNDE_PLATZHALTER

## Gegen das Vorbild geprüft / nicht geprüft

- CLDR: Regeln one/few/other nach plurals.xml (cs nur 1 = one, 2–4 = few; en/de/es 1 = one) im Test.
- NRC: die Plugin-Dialoge in Pixeln gegen NRCs Fotos jetzt auch im tschechischen Lauf.
- Nicht geprüft: die übrigen Selbsttests auf es/cs vollständig (s. Offen).

## Offen

- Übrige Selbsttests einmal auf es und cs (`SP/u1/modi.sh`: karteneditor voll, mission1, optsicher, kuerzel
  und die 22 Modi; etwa 80 min je Sprache) – wegen des Neustarts nicht mehr gelaufen. Nächster Schritt:
  `SPRACHEN="es cs" sh SP/u1/modi.sh`, sprachabhängig Rotes beheben.
- Doppelte Kennung `MeAufloesungFenster` beim Merge mit menue3 (oben).
- Die Kacheln der Bauteile schneiden lange Namen weiter bewusst ab (Tooltip zeigt den Namen); `spaltenGanz`
  sieht das nicht (gezeichnet, nicht gesetzt).
- Neue Texte späterer Zweige (Zeitleiste, M3–M6, Docking, Licht, Prüfung) in der zweiten Runde; Zähltexte
  dort mit `{…|…}` und trN schreiben (lint_i18n meldet tr() auf Zähltexte).

## Changelog

## English
- Spanish and Czech are complete again: the new texts of the map editor (live shader editor, patch bridge, real-time effects, VIS warnings), of the cutscene preview and of the menu editor (videos, library, texts, checks, live diagnosis, test in game, fonts), with the guide pages and their pictures.
- Counts use the right plural form in every language: "1 file", "2 files"; Czech "1 soubor", "2 soubory", "5 souborů" (Unicode CLDR rules) - no more "file(s)".
- Menu editor: the search row, the library hint and the aspect selection no longer cut off text; "Window" in the resolution list and the template name field are labelled in every language.
- Map editor: the optimizer's suggestion for small structural brushes after a build now appears in the program language.

## Deutsch
- Spanisch und Tschechisch sind wieder vollständig: die neuen Texte des Karten-Editors (Shader-Editor live, Patch-Brücke, Echtzeit-Effekte, VIS-Warnungen), der Zwischensequenz-Vorschau und des Menü-Editors (Videos, Bibliothek, Texte, Prüfungen, Live-Diagnose, Test im Spiel, Schriften) samt Handbuchseiten und Bildern.
- Zahlen stehen in jeder Sprache mit der richtigen Mehrzahl: „1 Datei“, „2 Dateien“; Tschechisch „1 soubor“, „2 soubory“, „5 souborů“ (Unicode-CLDR-Regeln) – kein „Datei(en)“ mehr.
- Menü-Editor: Suchzeile, Hinweis der Bibliothek und die Auswahl des Seitenverhältnisses schneiden keinen Text mehr ab; „Fenster“ in der Auflösungsliste und das Namensfeld für Vorlagen sind in jeder Sprache beschriftet.
- Karten-Editor: der Vorschlag des Optimierers zu kleinen strukturellen Pinseln nach einem Bau erscheint jetzt in der Programmsprache.

## 中文
- 西班牙语和捷克语再次完整:地图编辑器的新文本(实时着色器编辑器、面片桥接、实时特效、VIS 警告)、过场动画预览以及菜单编辑器(视频、组件库、文本、检查、实时诊断、游戏内测试、字体),连同手册页面及其图片。
- 计数在每种语言中都使用正确的复数形式:“1 file”“2 files”;捷克语“1 soubor”“2 soubory”“5 souborů”(Unicode CLDR 规则),不再出现“file(s)”。
- 菜单编辑器:搜索行、组件库提示和宽高比选择不再截断文字;分辨率列表中的“窗口”和模板名称字段在每种语言中都有标注。
- 地图编辑器:编译后优化器对小型结构笔刷的建议现在以程序语言显示。

## 日本語
- スペイン語とチェコ語が再び完全になりました:マップエディターの新しいテキスト(ライブシェーダーエディター、パッチブリッジ、リアルタイムエフェクト、VIS の警告)、カットシーンプレビュー、メニューエディター(動画、ライブラリ、テキスト、チェック、ライブ診断、ゲーム内テスト、フォント)と、ガイドのページと画像。
- 数はどの言語でも正しい複数形で表示されます:「1 file」「2 files」、チェコ語「1 soubor」「2 soubory」「5 souborů」(Unicode CLDR の規則)。「file(s)」はなくなりました。
- メニューエディター:検索行、ライブラリのヒント、縦横比の選択で文字が切れなくなりました。解像度リストの「ウィンドウ」とテンプレート名の欄はどの言語でも表示されます。
- マップエディター:ビルド後の小さな構造ブラシに関するオプティマイザーの提案がプログラムの言語で表示されるようになりました。

## Español
- El español y el checo vuelven a estar completos: los textos nuevos del editor de mapas (editor de shaders en vivo, puente de patches, efectos en tiempo real, avisos de VIS), de la vista previa de cinemáticas y del editor de menús (vídeos, biblioteca, textos, comprobaciones, diagnóstico en vivo, prueba en el juego, fuentes), con las páginas de la guía y sus imágenes.
- Los números llevan el plural correcto en cada idioma: "1 archivo", "2 archivos"; en checo "1 soubor", "2 soubory", "5 souborů" (reglas CLDR de Unicode): se acabó "archivo(s)".
- Editor de menús: la fila de búsqueda, la indicación de la biblioteca y la selección de proporción ya no cortan texto; "Ventana" en la lista de resoluciones y el campo del nombre de plantilla aparecen en cada idioma.
- Editor de mapas: la sugerencia del optimizador para brushes estructurales pequeños tras una compilación aparece ahora en el idioma del programa.

## Čeština
- Španělština a čeština jsou opět úplné: nové texty editoru map (živý editor shaderů, most patchů, efekty v reálném čase, varování VIS), náhledu animačních sekvencí a editoru menu (videa, knihovna, texty, kontroly, živá diagnostika, test ve hře, písma) včetně stránek příručky a jejich obrázků.
- Počty mají v každém jazyce správný tvar množného čísla: „1 soubor“, „2 soubory“, „5 souborů“ (pravidla Unicode CLDR) – žádné „soubor(ů)“.
- Editor menu: řádek hledání, nápověda knihovny a výběr poměru stran už neořezávají text; „Okno“ v seznamu rozlišení a pole pro název šablony jsou popsané v každém jazyce.
- Editor map: návrh optimalizátoru k malým strukturálním brushům po sestavení se nyní zobrazuje v jazyce programu.
