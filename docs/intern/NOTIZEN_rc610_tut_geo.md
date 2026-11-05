# Notizen rc610 / TUT-GEO: Tutorial „Ein Modell zu Brushes machen“

Auftrag `auftrag_rc610_tut_geo.md` (shank 05.11. abends: „wie benutzt man das model zu brush? gibt es dafür ein Tutorial bereits?“ - bis dahin nur der Abschnitt „Geometrie zu Brushes“ der Seite „Karten-Editor: Werkzeuge 9“; „ja mach ein Tutorial im Handbuch“). Direkt in `Y:\karte` auf `karte1` ab a81fe01f (LICHT1), Build `build-test`, Laeufe `laeufe/tg_*` (Tour `tg_g1` bis `tg_g8`, Bilder `tg_hb_<sprache>`, Schluss `tge_*`), Laufskript `lauf.py` wie LICHT1. kFassung bleibt rc609. Kein Spiel gestartet.

## Umsetzung

- Handbuch: neue Seite `k-geometrie` („Turn a model into brushes“ / „Ein Modell zu Brushes machen“), Gruppe Tutorials, bei den Karten-Tutorials (Übersicht „Deine erste Karte“, Karten und Index) neben `tut-karte` und `tut-karte-tuer`. Sechs Schritte wie im Auftrag: Modell in Blender vorbereiten (Größe - der Spieler ist 64 Einheiten hoch -, ein Objekt je Teil, geschlossene Teile, Materialnamen wie Texturen, Export .obj/.fbx/.glb/.gltf mit Y oben), **Datei > Geometrie als Brushes importieren…**, Teile-Liste (Auto / Brushes / Modell + Clip, Spalte Vorschau mit Zahl und detail/strukturell, Shader-Vorsatz), Einfügen als ein Undo-Schritt mit Optimierer, Ergebnis (Material und UV außen, caulk innen, detail, misc_model mit Clip-Brushes, Import-Ordner), Bauen und im Spiel ansehen (nur beschrieben). „Gut zu wissen“: .bsp öffnen (erkannte Modelle werden misc_model, „Nicht erkannte Modellflächen als Pinsel behalten“), Spielmodelle .md3/.glm gehen im Import nicht (Asset-Browser), nur das Modell ziehen / zurück nach Blender. Querverweise: Werkzeuge 9, Abschnitt Geometrie zu Brushes → Tutorial (neuer Hinweis), Tutorial → Werkzeuge 9.
  - Texte in 6 Sprachen in `tools/handbuch/inhalt_tutgeo.py` (wie `inhalt_tabfenster.py`: en/de/zh/ja in T, es/cs als ES/CS, in `inhalt_es.py`/`inhalt_cs.py` nur je zwei Zeilen Import). Keine neuen Programmtexte - `tools/i18n_rc610_tut.py` war nicht nötig.
  - Beispielmodell: schreibt der Selbsttest selbst (keine fremden Dateien) als .obj wie ein Blender-Export mit Y oben: Kiste 48 Einheiten (`crate01`), Säule mit Sockel (ein geschlossenes, nicht konvexes Netz, `pillar_base01`), offene Pflanzenwand (`temple_vines2`); mit dem Vorsatz `textures/yavin/` Texturen von Jedi Academy.
- Tour im Programm (`k-geometrie` in `inhalt_touren.py`, wie `tut-karte-tuer`): 1 Knopf Karten-Editor (Mach es), 2 Menü File > Import geometry as brushes… (fertig: Fenster offen; nie Mach es - Dateidialog), 3 das Fenster (Weiter), 4 Knopf Einfügen (fertig: mehr Brushes), 5 Kamera (Weiter), 6 Menü Build (fertig: gebaut oder Spielen).
- Helfer: Ziel `tour:k-geometrie` in `src/hilfe_daten.inc` (de/en/zh/ja) und `hilfe_sprache_es/cs.inc`; hilfetest mit shanks Frage „wie benutzt man das model zu brush?“ und „how do I turn a Blender model into brushes“ → die Tour (Top 3, 101/101).
- Selbsttest `gui/selbsttest_tutgeo.inc`: `BHED_KARTE_NUR=tutgeo`, auch im Selbsttest `hilfe2` (`BHED_H2_NUR=geo`); Bilder `BHED_EDITORTEST=handbuch BHED_HB_NUR=tutgeo` (je Schritt das ganze Fenster mit der Tour, `h_tut_geo_1` bis `_6`). Echte Aktionen (Klicks, Tippen des Vorsatzes, Klappliste der Ranke auf Brushes und zurück); statt des Dateidialogs ruft der Test `geoImportLaden` auf. Geprüft wird, was das Tutorial verspricht: Teile und geschlossen/offen, Vorschau (Kiste 1 Brush detail mit crate01, Säule 2 strukturelle Brushes, Ranke Auto = Modell + Clip, als Brushes ein dünner Detail-Brush mit temple_vines2 vorn), nach Einfügen so viele Brushes wie die Vorschau, misc_model auf `models/import/beispiel_geo.obj` im Import-Ordner, Optimierer offen, Meldung in der Sprache des Programms, im Ergebnis Kiste detail ganz crate01, Säule strukturell mit caulk innen, Clip-Brushes detail; ein Undo nimmt alles zurück; jeder Tourschritt schaltet nur mit seiner Wirkung weiter.

## Fehler, die das Tutorial fand (rot und grün)

1. Import in eine neue Karte stürzte ab (`tg_g1`, Zugriffsverletzung; Minidump: `importEinfuegen` → `Entitaet::kindEinfuegen` mit leerem `welt()`): eine neue Karte hat noch keinen worldspawn. Jetzt wie NRC `Map_FindOrInsertWorldspawn` (`Karte::weltHolenOderAnlegen`, beim ersten Brush). Grün `tg_g2` (22/0) - genau der Weg, den das Tutorial beschreibt (neue Karte, importieren).
2. Fenster Geometrie zu Brushes bei großer Oberfläche: feste Tabellenhöhe 220 Punkte und feste Breiten - bei 150 % lag die dritte Zeile unter dem Tabellenrand (ihre Klappliste nicht erreichbar), die Vorschau war abgeschnitten (`tg_hb_*` erster Lauf, Prüfung „Klappliste der dritten Zeile liegt ganz in der Tabelle“ rot, Bild). Jetzt: Größe mit der Schrift des Karten-Editors (13 Punkte = 1), höchstens 60 x 80 % des Fensters (daneben bleibt Platz für das Tutorial), die Tabelle füllt das Fenster bis über die Knöpfe, Spalte Als so breit wie ihr längster Eintrag, Dreiecke/Geschlossen nach Inhalt, „Danach den Optimierer öffnen“ in eigener Zeile. Grün in allen 6 Sprachen (`tg_hb_*` 11/0, Bilder angesehen: nichts abgeschnitten).
   - Die Klappliste meldet ImGui nicht unter ihrer Kennung; ihre Lage merkt sich das Fenster wie die Dialoge (`klappOrte`).

## Schluss

- Sauberer Neubau (`--clean-first`) 0 Warnungen, 0 Fehler (13 min); danach nur die Reihenfolge in `hilfe2` (Bau 0 Warnungen).
- ctest -j 16: 220 von 220 (`hilfe` 101/101 Top 3, `tour`, `handbuch_sprachen`).
- Selbsttests parallel (`tge_*`, 7 gleichzeitig): `karteneditor` tutgeo es 23/0, cs 23/0 (en `tg_g8` 23/0), hilfe1 59/0, Stufe 9 (Geometrie-Import der Stufe) 84/0, fotos 78/0; hilfe2 54/1 - k-update erwartet nach der Tür den Karten-Editor, die neue Tour endet in Events: sie läuft jetzt vor der Tür, `tge_hilfe2b` 55/0. Modus 1 unter Volllast 2633/18 (nur Kürzelfenster „wartet auf eine Taste“, wie in F6b ein Ausreißer unter Last, außerhalb des Geänderten), zu zweit wiederholt `tge_modus1b` 2651/0.
- Handbuchbilder `h_tut_geo_1` bis `_6` in 6 Sprachen aus `BHED_HB_NUR=tutgeo` (`BHED_HB_FENSTER=2560x1440@1.5`, parallel), je 2538 x 1384, angesehen; `gen_handbuch.py --pruefen` ok (es/cs 0 Texte, 0 Bilder englisch).
- Lints: sources, fassung, i18n, i18n_erzeugt, headers, feldinit, gapp, settings, d3dbesitz, mapstate, imgui_context, rueckkopplung, cbuffer, tabellenid ok; portability 99 (keine aus diesem Auftrag; die 99. ist `src/schattenatlas.cpp` aus LICHT1).
- Gegen das Vorbild geprüft: NRC `Map_FindOrInsertWorldspawn` (worldspawn beim ersten Brush). Das Tutorial selbst ist eigene Funktion (nicht in NRC); geprüft ist, dass jede Aussage des Textes im Programm so eintritt. Nicht geprüft: Bauen und Spielen (Schritt 6, kein Spielstart) und ein echter Blender-Export (das Beispiel ist eine .obj in derselben Form).

## Offen

- Kurven-Werkzeug (`src/karte/kurven.cpp:182`, `k.welt()->kindEinfuegen`): dasselbe Muster wie Fehler 1 - mit nur Punkt-Entities in einer neuen Karte (ohne worldspawn) wohl derselbe Absturz. Nicht nachgestellt und nicht geändert (außerhalb des Auftrags).
- Im Bild von Schritt 5 liegen über der Pflanzenwand ihre Clip-Brushes; `textures/system/clip` hat in den Spieldaten (Movie Duels base) kein Editorbild, darum das Schachbrett „SHADER IMAGE MISSING“ statt des Modells. Wie NRC.
- Schritt 6 (bauen, im Spiel ansehen) nur beschrieben, wie im Auftrag; im Tour-Selbsttest bleibt er stehen und wird übersprungen (gebaut wird in `hilfe1`).
- Beobachtung: Der Karten-Editor kam in einem einzelnen Lauf mit `BHED_HB_FENSTER=2560x1440@1.5` kleiner heraus als in parallelen Läufen (Maßstab des Karten-Editors = gerundete DPI mal Oberflächengröße, die DPI hängt vom Bildschirm des verdeckten Fensters ab). Die Handbuchbilder stammen alle aus parallelen Läufen mit gleichem Maßstab.

## Changelog

### English
- Guide: new tutorial "Turn a model into brushes" - from Blender to the map editor, step by step with an example model, also as an interactive tutorial in the program. The section "Geometry to brushes" links to it, and the helper finds it ("how do I turn a model into brushes").
- Map editor: importing geometry as brushes into a new, empty map no longer crashes.
- Map editor: the window "Geometry to brushes" grows with the interface size; at 150 % the third part and the preview were cut off.

### Deutsch
- Handbuch: neues Tutorial „Ein Modell zu Brushes machen“ - von Blender in den Karten-Editor, Schritt für Schritt mit einem Beispielmodell, auch als interaktives Tutorial im Programm. Der Abschnitt „Geometrie zu Brushes“ verweist darauf, und der Helfer findet es („wie mache ich aus einem Modell Brushes“).
- Karten-Editor: Geometrie als Brushes in eine neue, leere Karte zu importieren stürzt nicht mehr ab.
- Karten-Editor: Das Fenster „Geometrie zu Brushes“ wächst mit der Oberflächengröße; bei 150 % waren der dritte Teil und die Vorschau abgeschnitten.

### 中文
- 手册:新教程“把模型变成画刷”——从 Blender 到地图编辑器,用示例模型逐步讲解,也可在程序中作为交互式教程运行。“几何体转画刷”一节会链接到它,助手也能找到它(“如何把模型变成画刷”)。
- 地图编辑器:把几何体作为画刷导入到新的空地图时不再崩溃。
- 地图编辑器:“几何体转画刷”窗口会随界面大小放大;以前在 150% 时第三个部件和预览会被截断。

### 日本語
- ガイド: 新しいチュートリアル「モデルをブラシにする」- Blender からマップエディターまで、サンプルモデルで順を追って説明します。プログラム内のインタラクティブ チュートリアルとしても動きます。「ジオメトリをブラシに」の節からリンクし、ヘルパーでも見つかります(「モデルをブラシにするには」)。
- マップエディター: 新しい空のマップにジオメトリをブラシとして読み込んでも落ちなくなりました。
- マップエディター: ウィンドウ「ジオメトリをブラシに」がインターフェースの大きさに合わせて大きくなります。150% では 3 つ目の部品とプレビューが切れていました。

### Español
- Guía: nuevo tutorial «Convertir un modelo en brushes»: de Blender al editor de mapas, paso a paso con un modelo de ejemplo, también como tutorial interactivo en el programa. La sección «Geometría a brushes» enlaza con él y el asistente lo encuentra («cómo convertir un modelo en brushes»).
- Editor de mapas: importar geometría como brushes en un mapa nuevo y vacío ya no se cierra con un error.
- Editor de mapas: la ventana «Geometría a brushes» crece con el tamaño de la interfaz; al 150 % la tercera parte y la vista previa quedaban cortadas.

### Čeština
- Příručka: nový tutoriál „Udělat z modelu brushe“ - z Blenderu do editoru map, krok za krokem s ukázkovým modelem, také jako interaktivní tutoriál v programu. Oddíl „Geometrie na brushe“ na něj odkazuje a pomocník ho najde („jak udělat z modelu brushe“).
- Editor map: import geometrie jako brushů do nové prázdné mapy už nespadne.
- Editor map: okno „Geometrie na brushe“ roste s velikostí rozhraní; při 150 % byly třetí část a náhled oříznuté.
