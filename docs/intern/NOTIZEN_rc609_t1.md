# Notizen rc609 T1: Tabs als eigene Fenster

Wunsch shank (03.11.): "dann mach auch das man tabs als eigene Fenster öffnen kann das man in behaved und in mission gleichzeitig arbeiten kann wenn man die auf 2 Bildschirme zieht". Ein ganzer Tab (Events, Mission, Model, Map Editor, Menu Editor) wird ein eigenes Windows-Fenster; beide Tabs sind gleichzeitig bedienbar.

## Stufe 1: Bestandsaufnahme `leftMode`

Stand 6e4892d2: 542 Treffer in `gui/` (`grep -c leftMode`), davon 117 ausserhalb der Selbsttests. `leftMode` ist der Tab des Hauptfensters (0 Events, 1 Mission, 2 Model, 3 Map Editor, 4 Menu Editor, `gui/app_internal.h:426`).

Arten:
- **Z** Zeichnen: Anordnung und Inhalt des Tabs, der gerade zeichnet.
- **M** Menueleiste: Inhalt des Menues haengt am Tab.
- **K** Tastenkuerzel und Eingabe: wohin eine Taste, ein Mausrad oder eine abgelegte Datei geht.
- **U** Undo: welcher Stapel (laeuft ueber K und M).
- **F** Fokus: "welcher Tab ist gemeint", ohne zu zeichnen (Helfer F1, KI-Anbindung, Hervorhebung).
- **W** Wechsel: setzt `leftMode` (zu einem Tab gehen).
- **G** global: Bildzustand, der einmal je Bild gilt (Moduswechsel, Abspielen, Leerlauf).
- **E** Einstellungen: Anteil je Tab (`splitMap/Model/Events`) und Anordnung je Bereich.
- **S** Selbsttest, Tour, Messwerte.

### Ausserhalb der Selbsttests (117)

| Datei | Zeilen | Art | Was |
|---|---|---|---|
| `app.cpp` | 5470-5471 | K | Strg+Rad vergroessert, nicht im Map/Menu Editor |
| `app.cpp` | 5502, 5551 | Z, M | NRC-Menueleiste und Reiterleiste der Skripte je Tab |
| `app.cpp` | 5735-5753 | G | Moduswechsel einmal je Bild (Einschwingbild, Abspielen anhalten beim Verlassen von Mission) |
| `app.cpp` | 5790 | S | Groessen ins Detailprotokoll |
| `app.cpp` | 5995-6838 (19) | Z, E | Spalten, Tabellen-ID je Tab, Anteile je Tab, Band unter den Spalten, Debug-Fenster |
| `app.cpp` | 3797, 5034 | W | Modell laden -> Model; Menue-Projekt -> Menu Editor |
| `app_anordnung.inc` | 1369-1541 (12) | Z | `computeLayout` je Tab (Knopfspalte, Kopf, Band, Reiterzeile) |
| `app_anordnung.inc` | 1707, 1718 | Z, W | Modusleiste: hervorgehoben, Klick wechselt |
| `app_anordnung.inc` | 1798-2171 (14) | Z | linke Spalte, Ansicht, Ebenen je Tab |
| `app_anordnung.inc` | 3026, 3308 | Z | Felder der Aufteilung nur bei Events |
| `app_anordnung.inc` | 4781, 4787 | Z | Statuszeile je Tab |
| `app_anordnung.inc` | 4998, 5455 | M | Menue "Aufteilen" nur bei Events; Menue "Fenster" des Bereichs |
| `app_dock.inc` | 11-64 (7) | Z | Panels des Tabs (`p.bereich != bereich`), Spalten ohne herausgenommene Panels |
| `app_dockspace.inc` | 73-75, 427 | Z | DockSpace des Tabs (Wurzel global je Bereich: `wurzelId`, `ImHashStr`) |
| `app_ansichten.cpp` | 946, 1048 | W | Graph: Entity -> Mission |
| `app_ansichten.cpp` | 1167 | G | Spuren-Ansicht treibt die Uhr, wenn Mission nicht zeichnet |
| `app_ansichten.cpp` | 1423 | Z | Felder der Aufteilung |
| `app_enttext.cpp` | 693 | K, F | Entity-Text hat den Fokus |
| `app_enttext.cpp` | 716 | Z | Entity-Text nur im Mission-Tab |
| `app_karte.cpp` | 365 | Z, K | `karteneditor::aktiv()`: zeichnet und nimmt NRC-Tasten (`tastenVerarbeiten`, im Zeichnen) |
| `app_karte_logik.cpp` | 79 | W | Skript aus der Karte -> Events |
| `app_karte_texturliste.cpp` | 343 | Z | Texturen der Karte nur im Map Editor |
| `app_kartemission.cpp` | 301 | G | Dialog "Bezuege nach dem Nachladen" (wird nach den Tabs gezeichnet) |
| `app_lumpedit.cpp` | 318-1094 (7) | Z, K | Shader-Panel: Dokument des Tabs (Mission/Map Editor), Fokus |
| `app_menue.cpp` | 7045 | Z, K | `menueeditor::aktiv()`: zeichnet und nimmt Tasten (`editorTasten`, im Zeichnen) |
| `app_mission.cpp` | 1044 | K | Entf und Strg+D auf Entities |
| `app_mission.cpp` | 4094, 4696 | W | neue/geoeffnete Mission -> Mission |
| `app_view3d.cpp` | 1897 | W | Modell laden -> Model |
| `app_view3d_spiel.inc` | 4933 | W | Mission gewaehlt -> Mission |
| `app_zeitleiste.cpp` | 1772 | Z | Erklaerung in der Zeitleiste |
| `app_ablage.inc` | 58, 60 | K | abgelegte Datei: Modell in den Map Editor, Menue in den Menu Editor |
| `fenster_dock.cpp` | 192 | M, E | Anordnung speichern unter (Bereich) |
| `fenster_dock.h` | 32 | - | Kommentar |
| `hilfe.cpp` | 141-311 (4) | F | Helfer gewichtet Treffer des Tabs |
| `ki.cpp` | 251, 360 | F, W | KI liest und setzt den Tab |
| `tour.cpp` | 307, 472, 580, 1160 | S, W | Tour: Zustand, 3D-Ziel, Aktion "Modus", Menue-Tutorials |
| `tour_messwerte.cpp` | 212-291 (13) | S | Messwerte der Touren |
| `app_internal.h` | 426 | - | Feld |

Dazu gleichbedeutend: `karteneditor::aktiv()` / `menueeditor::aktiv()` in `handleShortcuts` und `handleTreeKeys` (`app.cpp:2370, 2736`, K), Fenstertitel (`app_ablage.inc:4180`, Hauptfenster) und Leerlauf (`app_ablage.inc:4228`, G).

### Selbsttests (425)

Groesste: `selbsttest_handbuch.inc` 97, `selbsttest_menue.inc` 49, `selbsttest_vorschau_b.inc` 45, `selbsttest_dock1.inc` 26, `selbsttest_programm.inc` 17, `selbsttest_dock4.inc` 16, `selbsttest_menue_rc608.inc` 15, `selbsttest_menue_pruefung.inc` 14, `selbsttest_hilfe2.inc` 13, `selbsttest_vorschau_a.inc` 12, `selbsttest_menue_leiste.inc` 11, `selbsttest_bspmerge.inc` 10, Rest 1-7 je Datei. Fast alle setzen den Tab (`g_app->leftMode = n`, W) oder pruefen ihn (S) - immer den des Hauptfensters, zwischen zwei Bildern.

### Folgerung fuer Stufe 2
- `leftMode` bleibt das Feld, das alle lesen. Zwischen den Bildern (Selbsttests, Touren, W) ist es der Tab des Hauptfensters. Beim Zeichnen eines eigenen Fensters steht darin dessen Tab (Z, E, M ohne Aenderung an den 117 Stellen).
- Pro Fenster gehalten (sonst laeuft es zwischen zwei Fenstern im selben Bild hin und her): `letzterModus`, `moduswechselBild`, `letzteFensterB`, `fensterStabilSeit`, `modeBarW`, die Wahl der Reiterleiste (`imguiWahl`, bisher `static`).
- K: Kuerzel (`handleShortcuts`, `handleTreeKeys`) laufen mit dem Tab des Fensters mit Fokus. NRC-Tasten und Menu-Editor-Tasten werden im Zeichnen verarbeitet - sie gelten nur, wenn ihr Fenster den Fokus hat.
- W: Ein Wechsel zu einem Tab, der in einem eigenen Fenster steht, holt dieses Fenster nach vorn, statt das Hauptfenster umzuschalten.
- G: "Mission zeichnet" heisst "Mission steht in irgendeinem Fenster" (`tabSichtbar(1)`), nicht `leftMode == 1`.

## Stufe 2: Zustand je Fenster
- `gui/tabfenster.h/.cpp`: Liste der eigenen Fenster; je Fenster getauscht werden `leftMode`, `letzterModus`, `moduswechselBild`, `letzteFensterB`, `fensterStabilSeit`, `modeBarW` und die Wahl der Reiterleiste (bisher `static int imguiWahl`, jetzt `App::reiterWahl`).
- `app.cpp`: Menue- und Reiterleiste als `fensterKopf()`, der Inhalt eines Tabs als `bereichZeichnen(l)` - dieselben Zeilen fuer Haupt- und eigene Fenster (`tabFensterZeichnen()` nach der Statuszeile).
- Wechsel (Art W) zu einem Tab mit eigenem Fenster: `tabfenster::bildAnfang` setzt das Hauptfenster zurueck und holt das Fenster nach vorn; ein Wechsel aus einem eigenen Fenster heraus holt den anderen Tab nach vorn, das Fenster bleibt bei seinem.
- Kuerzel: `handleShortcuts`, `handleTreeKeys` und der Helfer (F1) laufen in `tabfenster::imFokus` mit dem Tab des Fensters mit Fokus. NRC-Tasten (`app_karte_befehle.cpp`) und Menue-Editor-Tasten (`editorTasten`) nur bei `fensterHatFokus()`. Fokus: `NavWindow` -> Hauptfenster, eigenes Fenster (auch angedockte Panels, Klappmenues darin) oder Panel eines Bereichs (`###dock.`); Dialoge aendern ihn nicht.
- Art G: Abspielen haelt beim Loesen von Mission nicht an; Spuren-Uhr, Dialog "Bezuege nach dem Nachladen" und Leerlauf des Map Editors fragen `tabfenster::sichtbar(n)`. Strg+Rad und abgelegte Dateien gehen an den Tab unter der Maus.
- Ohne eigenes Fenster sind `fokusModus()`, `modusUnterMaus()` = `leftMode` und `fensterHatFokus()` = wahr: dasselbe Verhalten wie vorher.

## Stufe 3: Herausloesen
- Gewaehlt: ImGui-Viewports (wie die Panels aus dock1), kein zweites Win32-Fenster mit eigenem Kontext. Grund: dieselben Zeichenfunktionen und Texturen ohne Kopie, ein ImGui-Kontext (Ziehen, Klappmenues, Tooltips zwischen Fenstern), Schrift je Bildschirm ueber `ConfigDpiScaleFonts`. Ein eigener Kontext haette zweite Schriftatlanten und zweite Eingabewege gebraucht.
- Fensterklasse: `NoAutoMerge` (bleibt eigenes Windows-Fenster auch ueber dem Hauptfenster), `ParentViewportId = 0` (gleichrangig, eigener Taskleisteneintrag, nicht immer vor dem Hauptfenster); im Selbsttest `NoFocusOnAppearing` und verborgen (DWM-Cloak wie die Panels).
- Bedienung: Rechtsklick auf den Tab in der Modusleiste (`In eigenem Fenster oeffnen` / `Zurueck ins Hauptfenster`), Knopf aus dem Fenster ziehen und draussen loslassen, Menue Fenster `Diesen Tab in eigenem Fenster` und `Alle Tabs zurueck ins Hauptfenster`. Zurueck: Schliessknopf, WM_CLOSE (Alt+F4), Titelleiste auf die Modusleiste des Hauptfensters (sie leuchtet wie der alte Platz eines Panels).
- Startlage rechts neben der Modusleiste des Hauptfensters, damit sie frei bleibt. Ein Tab mit eigenem Fenster steht in der Modusleiste blass mit Rahmen; Klick holt sein Fenster nach vorn.
- Ein modaler Dialog des Hauptfensters holt das Hauptfenster nach vorn (sonst lag "Exit?" hinter dem eigenen Fenster - im ersten Gruenlauf 9 Fehler "Klick kam nicht an").
- Einstellungen: `Settings::fenster["tabfenster"]` (`tab1|<tab>,<x>,<y>,<b>,<h>,<Arbeitsbereich des Bildschirms>|...`, `bhed/fensteranordnung.h`); `Anordnung speichern unter` legt `t<bereich>/<name>` dazu, Laden holt sie zurueck, Loeschen raeumt sie weg. Fehlt der Bildschirm, kommt das Fenster mittig auf den des Hauptfensters, zu gross wird verkleinert (`aufBildschirm`).
- DPI je Bildschirm: ImGui skaliert die Schrift je Viewport (wie bei den Panels). Hier gibt es nur einen Bildschirm (2560 x 1528 Arbeitsbereich): zwei Bildschirme mit verschiedener Skalierung sind NICHT geprueft.

## Stufe 4: Gemeinsame Daten und Undo
- Es gibt keine Kopie: beide Fenster zeichnen aus denselben Daten (`g_app->doc`, Reiter, Karte, Map-Editor-Dokument). Eine Aenderung steht im naechsten Bild im anderen Fenster.
- Regel Undo: ein Stapel je Dokument (Skript: `Document`, Karte des Map Editors: `karte.undo`, Menue: Textpuffer), unabhaengig vom Fenster, in dem geaendert wurde. Strg+Z/Strg+Y gehen an das Dokument des Fensters mit Fokus. Begruendung: Events und Mission zeigen dasselbe Skript - zwei Stapel fuer ein Dokument koennten Schritte in anderer Reihenfolge zuruecknehmen, als sie gemacht wurden, und der Stand haenge davon ab, wo man drueckt. So nimmt Strg+Z im Mission-Fenster genau den Schritt zurueck, den Events gerade gemacht hat (Selbsttest).

## Stufe 5: Mehrere 3D-Ansichten, Leerlauf je Fenster
- Leerlauf je Fenster (`backend_win32.cpp`, `plattformFenster`): ein eigenes Fenster zeichnet und zeigt nur, wenn seine Zeichenliste eine andere Pruefsumme hat (Ecken, Indizes, Befehle, Lage, Groesse, Massstab), eine Textur darin in diesem Bild bemalt wurde (`gui/bemalt.h`: Renderziel setzen, leeren, kopieren, aufloesen, hochladen, neue Sicht - in `gpumap_win32.cpp`, `karte_gpu_win32.cpp`, `backend_win32.cpp`) oder ImGui Texturen geaendert hat. Sonst bleibt das gezeigte Bild - es waere dasselbe. Gilt fuer alle eigenen Fenster (auch Panels aus dock1); angeforderte Bilder (Selbsttest) und Fenster nach dem Verkleinern zeichnen immer.
- Das Hauptfenster zeichnet weiter jedes Bild: sein Present (vsync) taktet die Schleife; ohne Eingabe ruht ohnehin alles (rc596).
- Die Bildrate (fps) in der Statuszeile von Mission steht in einem eigenen Fenster nur mit Fokus - sie wechselt fast jedes Bild, und ein ruhendes Fenster muesste sonst immer neu zeichnen.
- Messung (allein, `BHED_EDITORTEST=tabfensterleistung`, Karte mp/duel1, Map Editor bm_duel1.map, Fenster 1400 x 900):

| Fall | CPU Mittel / p99 ms | GPU Mittel / p99 ms |
|---|---|---|
| Mission allein, Flug | 1.12 / 2.64 | 1.205 / 1.594 |
| Mission Flug + Map Editor in eigenem Fenster (ruhend) | 3.20 / 7.10 | 1.957 / 4.660 |
| Mission Flug + Map Editor in eigenem Fenster (Kamera dreht) | 2.29 / 4.19 | 1.186 / 1.256 |
| Map Editor allein, Kamera dreht | 1.00 / 2.73 | 0.496 / 0.739 |
| Map Editor dreht + Mission in eigenem Fenster (ruhend) | 1.61 / 9.74 | 0.657 / 9.106 |
| Map Editor dreht + Mission in eigenem Fenster (Flug) | 2.75 / 19.29 | 0.783 / 2.597 |

  - Leerlauf (Prozessor, ganze Schleife ruht): Mission allein 1.3 % eines Kerns, mit Map Editor in eigenem Fenster 2.3 %.
  - Die Vorschau zeichnet mit zweitem Fenster in derselben Groesse (1350 x 931) - nichts wird kleiner oder einfacher.
  - Ein ruhendes Mission-Fenster zeigt 0 Bilder in 90, waehrend das Hauptfenster zeichnet (Selbsttest).

## Tests
- ctest `tabfenster` (neu, `tests/tabfenster.cpp`): Kodieren/Lesen, 11 kaputte Zeilen abgelehnt, Lage beim Laden (Bildschirm da, Arbeitsbereich anders, Bildschirm fehlt, zu gross, ragt hinaus), Weg durch die Einstellungsdatei. Rot 1 Fehler (Zahl mit 8 Ziffern angenommen), gruen 0.
- Selbsttest `tabfenster` (neu): rot vor dem Bau 19 FEHLER von 24 (kein Kontextmenue, kein eigenes Fenster; einer davon Testaufbau: die Graph-Ansicht der Einstellungen). Gruen 21 OK, 0 FEHLER (`laeufe/t1_gruen10`). Geprueft: Kontextmenue -> eigenes Windows-Fenster (eigener Viewport, Plattformfenster), Modusleiste bleibt frei; beide Script Flows zeigen das Skript, Entf im Hauptfenster loescht die Zeile in beiden; Strg+Z im Mission-Fenster nimmt sie zurueck; Map Editor aus der Modusleiste gezogen; Strg+Z im Map-Editor-Fenster nur die Karte, im Hauptfenster nur das Skript; ruhendes Mission-Fenster 0 Bilder in 90; Hover darin zeichnet; Menue Fenster; Schliessknopf, WM_CLOSE, Zurueckziehen auf die Modusleiste holen den Tab zurueck.
- Selbsttest `tabfensterneustart` (mit den Einstellungen des Laufs davor): beide Fenster an derselben Stelle und Groesse (761,765 862x647 und 885,557 1675x971), dann alle zurueck, Einstellung leer. Rot vor dem Bau: nichts wiederhergestellt (die Liste gab es nicht).
- `tabfensterleistung`: Zahlen oben (allein gelaufen).
- Bilder: `BHED_EDITORTEST=tabfenster BHED_TF_NUR=fotos BHED_HB_FENSTER=2560x1440@1.5`, je Sprache ein Lauf (parallel), `tools/handbuch/bilder.py`; 4 Bilder je Sprache, 2538 breit. Mit eigenen Fenstern zusammengesetzt aus den eigenen Grafikpuffern (`render::fotoMitFensternAnfordern`), kein Bildschirmfoto.

## Schluss
- Sauberer Neubau (`--clean-first`): rc 0, 0 Warnungen (/W4 /WX).
- ctest: 215 von 215 gruen.
- Selbsttests mit eigenem Build, in Wellen parallel (`laeufe/skripte/schluss.sh`): tabfenster 21/0, tabfensterneustart 3/0, dock2mass 29/0, dock4 115/0, dock5 26/0, dock1neustart 18/0, menueeditor 2775/0, mission1 98/0, missionsbau 95/0, kartemission 26/0, karteneditor (fotos) 78/0, Modus 1 2651/0 sowie die Modi aus integ.sh (menueloeschen, dateiort, enttext, lumpedit, wegpunkte, wegpunkte2, wegerzeugung, wegmalen, kemd, kampfpunkte, deckung, patchlicht, fundef, editorportal, texturliste, bspmerge, modellsetzen) alle 0 FEHLER.
- dock1: 99 OK, 2 FEHLER ("mission.enttext/mission.lumps wieder im Hauptfenster") - gleich mit der exe von 6e4892d2 ohne T1 (`laeufe/t1_ref_dock1`: 99 OK, dieselben 2 FEHLER). Kommt aus W2b/J (Panels ohne Platz im Hauptfenster), nicht aus T1.
- Lints: alle wie vorher; lint_portability, lint_pruefcodes, lint_fremdprogramm, lint_efxflags melden nur Stellen ausserhalb von T1.

## Offen
- Map Editor: seine 3D-Ansichten (Kamera, XY) zeichnen in jedem Bild neu, wie im Hauptfenster. Ein ruhendes Map-Editor-Fenster zeigt deshalb weiter Bilder, solange die Schleife laeuft (gemessen 95 in 90 Bildern). Abhilfe waere NRCs `queueDraw` (nur bei Aenderung zeichnen) - ein eigener Punkt, kein Raten an einem Fingerabdruck des Zustands.
- Das Hauptfenster zeichnet jedes Bild, solange die Schleife laeuft (es taktet sie).
- Zwei Bildschirme mit verschiedener Skalierung: nicht geprueft (nur ein Bildschirm vorhanden).
- Nicht-modale Dialoge (Ereigniseditor, Suchen, Einstellungen) erscheinen im Hauptfenster, auch wenn sie aus einem eigenen Fenster geoeffnet werden.
- Tour und Selbsttests schalten weiter den Tab des Hauptfensters (zwischen den Bildern).
- dock1: 2 FEHLER von vor T1 (siehe Schluss).

## English
- Whole tabs (Events, Mission, Model, Map Editor, Menu Editor) can be opened in their own window - right-click in the mode bar, drag the tab button out, or Window > This tab in its own window - and moved to a second screen. Both tabs work at the same time on the same data; shortcuts and undo go to the window you work in. Closing the window, Alt+F4 or dragging its title bar onto the mode bar brings the tab back. Position, size and screen are kept and saved with named layouts.
- Own windows that do not change show no new frame (also floating panels).

## Deutsch
- Ganze Tabs (Ereignisse, Mission, Modell, Karten-Editor, Menü-Editor) lassen sich in einem eigenen Fenster öffnen - Rechtsklick in der Modusleiste, den Tab-Knopf herausziehen oder Fenster > Diesen Tab in eigenem Fenster - und auf einen zweiten Bildschirm ziehen. Beide Tabs arbeiten gleichzeitig auf denselben Daten; Tastenkürzel und Rückgängig gelten dem Fenster, in dem du arbeitest. Fenster schließen, Alt+F4 oder die Titelleiste auf die Modusleiste ziehen holt den Tab zurück. Lage, Größe und Bildschirm bleiben erhalten und werden mit benannten Anordnungen gespeichert.
- Eigene Fenster, in denen sich nichts ändert, zeigen kein neues Bild (auch schwebende Panels).

## 中文
- 整个标签页(事件、任务、模型、地图编辑器、菜单编辑器)可以在独立窗口中打开——在模式栏中右键单击、把标签页按钮拖出,或使用"窗口 > 此标签页使用单独窗口"——并移到第二个屏幕。两个标签页在同一数据上同时工作;快捷键和撤销作用于你正在操作的窗口。关闭窗口、Alt+F4 或把标题栏拖到模式栏上即可取回标签页。位置、大小和屏幕会保留,并随命名布局一起保存。
- 没有变化的独立窗口不会显示新画面(浮动面板也一样)。

## 日本語
- タブ全体(イベント、ミッション、モデル、マップエディター、メニューエディター)を別ウィンドウで開けます。モードバーで右クリック、タブのボタンを外へドラッグ、または「ウィンドウ > このタブを別ウィンドウに」を使い、2 台目の画面へ移せます。両方のタブが同じデータで同時に動作し、ショートカットと元に戻すは作業中のウィンドウに働きます。ウィンドウを閉じる、Alt+F4、またはタイトルバーをモードバーへドラッグするとタブが戻ります。位置・大きさ・画面は保持され、名前付きレイアウトと一緒に保存されます。
- 変化のない別ウィンドウは新しい画像を表示しません(フローティングパネルも同様)。

## Español
- Las pestañas enteras (Eventos, Misión, Modelo, Editor de mapas, Editor de menús) pueden abrirse en su propia ventana - clic derecho en la barra de modos, arrastrar el botón de la pestaña hacia fuera o Ventana > Esta pestaña en ventana propia - y llevarse a una segunda pantalla. Ambas pestañas trabajan a la vez sobre los mismos datos; los atajos y deshacer van a la ventana en la que trabajas. Cerrar la ventana, Alt+F4 o arrastrar su barra de título a la barra de modos devuelve la pestaña. La posición, el tamaño y la pantalla se conservan y se guardan con las disposiciones con nombre.
- Las ventanas propias en las que nada cambia no muestran una imagen nueva (también los paneles flotantes).

## Čeština
- Celé karty (Události, Mise, Model, Editor map, Editor menu) lze otevřít ve vlastním okně - pravým kliknutím v liště režimů, vytažením tlačítka karty ven nebo Okno > Tuto kartu ve vlastním okně - a přesunout na druhou obrazovku. Obě karty pracují současně se stejnými daty; klávesové zkratky a Zpět platí pro okno, ve kterém pracujete. Zavření okna, Alt+F4 nebo přetažení titulku na lištu režimů kartu vrátí. Poloha, velikost a obrazovka se zachovají a ukládají se s pojmenovanými rozvrženími.
- Vlastní okna, ve kterých se nic nemění, neukazují nový obraz (i plovoucí panely).
