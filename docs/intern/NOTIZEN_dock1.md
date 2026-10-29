# Notizen dock1 – Fenster frei anordnen, mehrere Bildschirme (Zweig dock1)

Auftrag: SP\auftrag_docking.md mit SP\auftrag_gemeinsam.md. Ziel (shank): jedes
Panel als eigenes Windows-Fenster herausnehmen (zweiter Bildschirm), wieder
andocken, Anordnung frei wählen wie in Blender; Layouts speichern, benennen,
wechseln, zurücksetzen. Kein Qt – Dear ImGui 1.92.9b-docking.

## Aufbau

| Teil | Datei |
|---|---|
| Docking + Multi-Viewport einschalten, DPI je Bildschirm, Selbsttest-Fenster ohne Fokus | `gui/main_win32.cpp` (runSession, imguiWin32Starten) |
| Eigene Fenster zeichnen (D3D11: Swapchain je Fenster aus imgui_impl_dx11; OpenGL: Kontext je Fenster wie example_win32_opengl3), Bild eines Fensters lesen (Selbsttest), Zeit messen | `gui/backend_win32.cpp` (plattformFenster, glFenster*, viewportBild*) |
| Panels mit stabiler Kennung, Zustand je Bereich, Menü „Fenster“, eigene Fenster, Zurückziehen, Fenster einsammeln | `gui/fenster_dock.{h,cpp}` |
| Anordnung kodieren/lesen, Auszug aus ImGuis Ini-Text, Bereich austauschen mit Neunummerierung der Dock-Knoten | `include/bhed/fensteranordnung.h`, `src/fensteranordnung.cpp`, Probe `tests/fenster.cpp` (ctest `fenster`) |
| Events/Mission/Model: Spalten ohne herausgenommene Panels, Panels in eigenen Fenstern | `gui/app_dock.inc`, `gui/app.cpp` (draw), `gui/app_anordnung.inc` (drawEventsList geschnitten in drawEbenenInhalt / drawAnsichtInhalt / drawEreignisListe, computeLayout) |
| Map Editor: Felder herausnehmen (QSplitter-`versteckt`), Felder in eigenen Fenstern, Menü „Fenster“ hinter NRCs Menüs | `gui/app_karte.cpp`, `gui/app_karte_menue.cpp` |
| Menu Editor: Bausteine, Fläche, Reiter-Spalte (die M2-Panels) | `gui/app_menue.cpp` (Layout in zeichneInnen, dockFenster) |
| Einstellungen `fenster=<b<n>[/<name>]>=<kodiert>` | `include/bhed/settings.h`, `src/settings.cpp` |
| Selbsttest `dock1`, `dock1neustart`, Fotos `BHED_DOCK_NUR=fotos` | `gui/selbsttest_dock1.inc` |
| Texte | `tools/i18n_dock1.py` (es/cs fallen auf Englisch zurück) |
| Handbuch | `tools/handbuch/inhalt_dock.py` (Seite `fenster_anordnen`) |

### Panels

| Bereich | Panels (Kennung) | Platz im Hauptfenster ohne sie |
|---|---|---|
| Events | `ereignisse.liste`, `.fluss`, `.aktionen` | Actions weg: die zwei Spalten wie bei der Mission; Liste oder Script Flow weg: einfache Tabelle, linke Spalte so breit wie Modusleiste |
| Mission | `mission.ansicht`, `.ebenen`, `.fluss`, `.zeitleiste` | Ebenen/Ansicht: ihre Spalte der linken Tabelle fehlt; Zeitleiste: Band und Ziehkante fehlen (toolH 0) |
| Model | `modell.ansicht`, `.animationen`, `.fluss`, `.aktionen`, `.leiste` | wie Mission; Leiste: Band fehlt |
| Map Editor | `karte.kamera`, `.xy`, `.xz`, `.yz`, `.konsole`, `.texturen`, `.gruppe` | Feld im QSplitter versteckt (NRC-Teiler gibt den Platz den Nachbarn), Spalte ohne Feld ebenso; nur Felder der aktuellen NRC-Anordnung; Floating: nichts |
| Menu Editor | `menue.bausteine`, `.flaeche`, `.reiter` | Spalte und Teiler fehlen; nur im Layout „Reiter rechts“ |
| überall | Helfer, Tutorials, Meldungen (Menü „Weitere Fenster“) | sind ohnehin eigene Fenster |

Standardanordnung = alles zuhause = derselbe Code wie vorher (einzige sichtbare
Änderung: der Menüeintrag „Fenster“ vor „Help“ bzw. hinter NRCs Menüs).

## Entscheidungen und Abweichungen

- **Kein durchgehender DockSpace im Hauptfenster.** Die Spalten (Tabellen mit
  eigenen Griffen, Anteilen, Ueberhang-Messung), das Band und NRCs QSplitter
  pixelgleich in ImGui-Dock-Knoten nachzubauen, hätte jede Masszahl der
  bisherigen Selbsttests verschoben. Stattdessen: Panels sind zuhause im alten
  Layout; herausgenommen in ImGui-Fenstern, die frei andocken (Reiter/Teilung
  untereinander), auf jeden Bildschirm. Zurück per Schliessknopf, Menü oder
  Ziehen auf den alten Platz (eigenes Ziel, ImGui kann nicht in Tabellenzellen
  andocken). Panels anderer Bereiche und Dialoge docken nicht zusammen
  (ImGuiWindowClass, DockingAllowUnclassed = false).
- **ImGui rechnet mit eigenen Fenstern in Bildschirmpunkten.** Angepasst:
  Freelook-Zeiger (Bildschirmpunkte statt Hauptfenster-Punkte),
  abgelegte Dateien (ClientToScreen), Abdunklung der Tour (ab
  GetMainViewport()->Pos). Im Selbsttest liegt die Innenfläche des
  Hauptfensters auf (0,0) (gleich gross wie vorher), damit die vielen
  Fotoausschnitte in Fensterpunkten weiter stimmen; dock1 verschiebt das
  Fenster und prüft mit Ursprung ≠ 0.
- **Simulierte Maus meldet ihr Fenster** (AddMouseViewportEvent mit der Suche
  aus ImGui), sonst nimmt ImGui das Fenster unter dem echten Zeiger.
- **Fensterlagen** stehen als Auszug aus ImGuis Ini-Text in der Anordnung
  (behaved hat keine imgui.ini). ImGui schreibt den Namen ab „###“
  (`[Window][dock.mission.fluss]`) – erst so gefiltert (rot im Selbsttest:
  gespeicherte Anordnung ohne Lagen). Dock-Knoten nummeriert ImGui von 1 an;
  beim Laden eines Bereichs werden kollidierende Knoten neu nummeriert, die
  Bäume der anderen Bereiche bleiben.
- **Laden zwischen zwei Bildern** (`dock::vorBild` vor ImGui::NewFrame): mitten
  im Bild geladen zerfiel der Dock-Baum (rot im Selbsttest, nach dem Laden
  standen die Panels einzeln) – jetzt wie ImGui es vorsieht.
- **SetNextWindowPos nur für neue Panelfenster** (sonst kann BeginDocked ein
  angedocktes Fenster lösen).
- **Fenster einsammeln**: ImGui hält ein Fenster nur mit einem Rand im Bild und
  verkleinert nie. Liegt weniger als ein Viertel oder nicht die Titelleiste auf
  einem Bildschirm, kommt es ganz auf den Bildschirm des Hauptfensters; ist es
  grösser als sein Bildschirm, wird es passend verkleinert (auch Dock-Wirte).
- **DPI**: io.ConfigDpiScaleFonts (Schrift je Bildschirm; für das Hauptfenster
  derselbe Wert wie dpiScale) und ConfigDpiScaleViewports (Fenstergrösse beim
  Wechsel). Polster/Abstände bleiben im Massstab des Hauptfensters (applyScale).
- **OpenGL**: eigene Fenster mit Kontextumschaltung je Fenster; nicht im
  Selbsttest geprüft (der läuft mit Direct3D 11).

## Ergebnisse

(Messwerte unter Last – fünf andere Agenten laufen parallel.)

- Probe `fenster`: Kodieren, Filter (echtes ImGui-Format), Austausch mit
  Neunummerierung, Einstellungsdatei – grün.
- Selbsttest `dock1`: 86 OK, 0 Fehler. Geprüft: Standardanordnung von Events,
  Mission, Model in Zahlen (Spalten, Tabelle, Band, Fenster) vor allem
  Verschieben = nach dem Zurücksetzen = am Ende; Hauptfenster verkleinert und
  verschoben (Ursprung ≠ 0); Zeitleiste über das Menü heraus, eigenes
  Windows-Fenster (sichtbar, Swapchain-Bild gelesen: Grösse = Fenster, Farben),
  Band weg; Script Flow auf die Zeitleiste gezogen = ein Dock-Knoten;
  Speichern unter / Zurücksetzen / Laden (Lage ±1, wieder zusammengesetzt) /
  Löschen; Andocken per Schliessknopf (Reiter), Menü und Ziehen auf den alten
  Platz; zweiter Bildschirm (vorgetäuscht) abgezogen → zurück auf den
  Bildschirm des Hauptfensters in voller Grösse; kleiner Bildschirm 640x480 →
  verkleinert; Massstab 150 % → Fenster ×1,5, Schrift ×1,5, zurück; jedes
  Panel aller Bereiche einmal heraus (zeichnet) und zurück; Map Editor Regular
  und Split (Feld fehlt im Teiler, steht im eigenen Fenster), Kamera mit
  Testkarte und Echtzeit-Licht im eigenen Windows-Fenster (Licht näher → Boden
  heller), Klick macht sie aktiv, Pfeil hoch fährt, Freelook 300 Punkte =
  15 Grad wie NRC (Qt-Massstab 2); Menu Editor Fläche heraus (Reiter-Spalte
  breiter), zurück per Schliessknopf; Events Actions ausgeblendet (zwei
  Spalten), Liste heraus.
- Bildzeit (Mittel über 120 Bilder, unter Last): mit 1 Viewport 4,20 ms, mit
  3 Viewports 4,20 ms; Zeichnen und Zeigen der zwei eigenen Fenster 1,0–2,9 ms
  je Bild (D3D11, Present ohne VSync je eigenem Fenster) – fällt in die
  Wartezeit auf das VSync des Hauptfensters.
- Selbsttest `dock1neustart` im selben Einstellungsordner (zweiter Prozess):
  Zeitleiste und Script Flow draussen, als Reiter zusammengesetzt, an der
  gespeicherten Stelle; gespeicherte Anordnung noch da; Zurücksetzen – 5 OK.
- Sauberer Neubau 0 Warnungen, ctest 158/158 (neu: `fenster`).

### Schlussrunde (unter Last, je Modus eigener Einstellungsordner)

| Modus | Ergebnis |
|---|---|
| dock1 | 86 OK, 0 FEHLER |
| menueeditor | 46 OK, 0 FEHLER |
| karteneditor (BHED_KARTE_NUR=fotos) | 78 OK, 0 FEHLER |
| karte / kamera / missionsbau / szenen / ton | 227 / 95 / 95 / 26 / 34 OK, je 0 FEHLER |
| npcklasse / laserfalle / geschuetzanim / zielstrahl / effektgrund / figurdeform | 12 / 8 / 3 / 2 / 1 / 1 OK, je 0 FEHLER |
| 1 | erster Lauf 2601/93 – ohne Handbuchordner (Lauf ausserhalb des Worktrees, touren.json nicht gefunden; Fehler nur in den Touren); mit BHED_HANDBUCH 2645/4 (Klick „Close“ im Tutorial-Fenster kam nicht an, zwei Folgeprüfungen, Ton-Zeitpunkt) – wiederholt **2648 OK, 0 FEHLER** (wie Integration 3) |
| ImGui-Zusicherungen | in keinem Lauf |

### Fehler unterwegs (rot → grün im selben Test)

1. Gespeicherte Anordnung ohne Fensterlagen: Filter suchte „###dock.“, ImGui
   schreibt den Namen ab „###“ → Probe mit echtem Format, Filter auf den Namen.
2. Geladene Anordnung zerfiel (Panels einzeln statt als Reiter) →
   LoadIniSettingsFromMemory vor NewFrame, Panels im selben Bild.
3. Menü-Editor: Zurück-Knopf unter dem eigenen Fenster (Startlage über dem
   alten Platz) → der ganze Editor ist kein Panel mehr, sondern seine drei.
4. Klick auf „Reset layout“ traf die herausgenommene Ereignisliste: ihre
   Startlage ragte aus dem Hauptfenster, sie wurde ein eigenes Windows-Fenster
   über den Menüs → Startlage ganz im Hauptfenster.
5. Zurückziehen: die Beschriftung „Loslassen zum Andocken“ lag über der
   Titelleiste des gezogenen Fensters (Handbuchbild) → oben im Platz, mit
   Hintergrund.

### Beobachtung (vorher schon so)

- `spaltenUeberhang` startet bei 40 und wird erst ab 2 Punkten Unterschied
  übernommen; nach dem Start sind Script Flow und Events daher 1 Punkt anders
  als nach einem Wechsel in den Model-Tab (gemessen 39). dock1 schwingt deshalb
  erst einmal durch alle Modi, bevor es die Standardanordnung misst.

## Offen

- Panels des Menü-Editors nur im Layout „Reiter rechts“; „Text unten“ bleibt
  ein Stück.
- Dateien auf ein eigenes Fenster ziehen (WM_DROPFILES) – nur auf das
  Hauptfenster.
- OpenGL-Weg der eigenen Fenster ungeprüft (Selbsttest läuft mit D3D11).
- Echter Mauszeiger beim Freelook in einem eigenen Fenster nur mit dem
  simulierten Windows-Zeiger geprüft (der echte darf im Test nicht bewegt
  werden).
- es/cs: neue Texte und Handbuchseite noch englisch.
- Der Helfer (F1) kennt die Einträge des Menüs „Fenster“ nur über die
  Handbuchseite: tools/gen_hilfe_orte.py liest die Menüs aus dem Menüblock
  in app_anordnung.inc, das Menü steht in fenster_dock.cpp.
- Ältere Handbuchbilder mit der Menüleiste zeigen noch keinen Eintrag
  „Fenster“ (nicht neu aufgenommen).

## Changelog

## English
- Window layout like Blender: every panel (events list, Script Flow, Actions, mission view, entities, timeline, model view, map editor views, console, texture browser, menu editor canvas …) can be taken out into its own window - drag it onto a second screen, combine panels as tabs or side by side, dock it back by closing it or dragging it onto its old place.
  - New menu **Window**: show/hide panels, put them into their own window, save layout as, load, delete, reset - per area; the current layout is restored at the next start.
  - Windows on an unplugged or too small screen come back; text and size follow the scale of the screen a window is on.
  - Map editor: NetRadiant's layouts stay the default; taken-out views keep drawing (real-time light included), free look and shortcuts work in the window you clicked.

## Deutsch
- Fensteranordnung wie in Blender: jedes Panel (Ereignisliste, Script Flow, Actions, Missionsansicht, Entities, Zeitleiste, Modellansicht, Ansichten des Karten-Editors, Konsole, Texturbrowser, Fläche des Menü-Editors …) lässt sich in ein eigenes Fenster nehmen – auf einen zweiten Bildschirm ziehen, Panels als Reiter oder nebeneinander zusammensetzen, durch Schließen oder Ziehen auf den alten Platz wieder andocken.
  - Neues Menü **Fenster**: Panels zeigen/ausblenden, in ein eigenes Fenster nehmen, Anordnung speichern unter, laden, löschen, zurücksetzen – je Bereich; die laufende Anordnung ist beim nächsten Start wieder da.
  - Fenster auf einem abgezogenen oder zu kleinen Bildschirm kommen zurück; Schrift und Größe folgen dem Maßstab des Bildschirms.
  - Karten-Editor: NetRadiants Anordnungen bleiben die Vorgabe; herausgenommene Ansichten zeichnen weiter (auch Echtzeit-Licht), Umschauen und Tastenkürzel gelten im angeklickten Fenster.

## 中文
- 像 Blender 一样排列窗口:每个面板(事件列表、Script Flow、Actions、任务视图、实体、时间轴、模型视图、地图编辑器视图、控制台、纹理浏览器、菜单编辑器画布……)都可以移到独立窗口中——拖到第二个屏幕上,以选项卡或并排方式组合面板,关闭或拖回原位置即可重新停靠。
  - 新菜单 **窗口**:显示/隐藏面板、放入独立窗口、布局另存为、加载、删除、重置——按区域;当前布局在下次启动时恢复。
  - 拔下的或过小的屏幕上的窗口会回来;文字和大小跟随窗口所在屏幕的缩放比例。
  - 地图编辑器:NetRadiant 的布局仍为默认;移出的视图继续绘制(包括实时光照),自由视角和快捷键在所单击的窗口中生效。

## 日本語
- Blender のようなウィンドウ配置:すべてのパネル(イベント一覧、Script Flow、Actions、ミッションビュー、エンティティ、タイムライン、モデルビュー、マップエディターのビュー、コンソール、テクスチャブラウザー、メニューエディターのキャンバスなど)を独立したウィンドウに取り出せます。2 台目の画面へドラッグし、タブや横並びで組み合わせ、閉じるか元の場所へドラッグすると元に戻ります。
  - 新しいメニュー **ウィンドウ**:パネルの表示・非表示、独立ウィンドウ化、レイアウトの保存・読み込み・削除・リセット(領域ごと)。現在のレイアウトは次回の起動時に復元されます。
  - 外した画面や小さすぎる画面にあったウィンドウは戻ってきます。文字と大きさはウィンドウのある画面の拡大率に従います。
  - マップエディター:NetRadiant のレイアウトが引き続き既定です。取り出したビューも描画を続け(リアルタイムライトを含む)、自由視点とショートカットはクリックしたウィンドウで働きます。
