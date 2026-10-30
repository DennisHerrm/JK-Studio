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

## dock2 (in Arbeit) – durchgehender DockSpace

Auftrag: SP\auftrag_dock2.md. Abgebrochen durch den Neustart (Windows Update
00:29), neu begonnen 04:55, harte Zeitgrenze 06:40 – daher nur ein Teil.

### Stand

- **Durchgehender DockSpace für Events, Mission, Model** (`gui/app_dockspace.inc`),
  **hinter einem Schalter, ab Werk aus**: `BHED_DOCKSPACE=1` schaltet ihn beim
  Start an (`dockraumAn()`, der Selbsttest `dock2mass` schaltet ihn um). Ohne
  Schalter ist alles wie in dock1 (alter Weg unverändert).
  - Prinzip: Die Standardanordnung wird nicht nachgerechnet. Dieselben
    ImGui-Tabellen wie vorher richten sich ein (nur gemessen, ohne Inhalt:
    `dockraum::zelleMessen`/`innenMessen` statt drawEventsList/drawFlowArea/
    drawButtonColumns), und die Dock-Knoten bekommen genau die gemessenen
    Spalten: je Panel ein Knoten (Modusleiste, Ebenen/Animationen, Ansicht bzw.
    Ereignisliste, Script Flow, Actions; Zeitleiste/Leiste als Band darunter),
    Knotenabstand 1 Punkt (= Tabellenrand, `ImGuiStyleVar_DockingSeparatorSize`),
    Polster der Panelfenster = Zellpolster (Band: ItemSpacing.y − 1), Kinder
    `frameH = -FLT_MIN` (bis an den Inhaltsrand = altes Spaltenende).
  - Aufbau per DockBuilder beim ersten Bild des Bereichs (die Panelfenster
    tragen NoSavedSettings, daher `SetNextWindowDockID` im ersten Begin – mit
    DockBuilderDockWindow allein standen sie schwebend bei 66,131, rot im
    ersten dock2mass-Lauf). Script Flow ist der Mittelknoten (nimmt beim
    freien Anordnen den Rest; ein leerer Mittelknoten bleibt stehen, darum
    nicht Actions – Lauf 7). Die Breite, die das Band in Mission/Model über die
    Spalten hinausreicht, geht über die Knotengrössen (`erzwingen` mit der
    Grösse des Knotens: erstes Kind gemessen, zweites der Rest) in die letzte
    Spalte; deren Inhaltsbereich wird auf die Spaltenbreite gekürzt.
  - Geometrie (gemessen, rot → grün in dock2mass): ImGui-Spalten liegen
    lückenlos [MinX, MaxX], ihr erster Punkt ist der Rand – Knoten daher
    [MinX+1, MaxX]; die Zellen der inneren Tabelle beginnen ein Zellpolster
    tiefer als die der äusseren; alle Kinder enden ein Zellpolster über dem
    Zeilenende (Panelpolster y = 2·CellPadding.y, äussere Panels rücken um
    CellPadding.y hoch); das Band beginnt am Zeilenende + 1 mit Polster
    ItemSpacing.y − 1 und reicht über die volle Breite (Inhalt des
    Hauptfensters ± Zellpolster). Zeilenende ohne Runden (Mission hat
    gebrochene Höhen: frameH −316,83). Modusleiste: ohne
    Reiterleiste, nicht abdockbar, nichts darüber andockbar. Übrige Panels:
    versteckte Reiterleiste mit ImGuis Dreieck (Klick zeigt die Reiter, Ziehen
    dockt ab) – andocken an jede Stelle, als Reiter, heraus auf andere
    Bildschirme.
  - Standardanordnung erkannt (jedes Panel allein in seinem Knoten, Baum
    unverändert) → vor `DockSpace()` bekommt jede Teilung die gemessenen
    Grössen (`SizeRef`), ImGuis Teiler sind aus (`NoResize`, gezeichnet als
    1-Punkt-Linie in TableBorderLight wie der Tabellenrand), die alten Griffe
    `##spalteLinks/##spalteRechts` (Grenzen, Anteile) und die Ziehkante der
    Zeitleiste (`drawTlKante`, jetzt im Panel) bleiben → Teiler-Verhalten wie
    bisher. Sonst (frei angeordnet): ImGuis Teiler, keine erzwungenen Grössen.
- Menü „Fenster“ im DockSpace (Schalter an, Bereiche 0–2): Ausblenden lässt
  den Knoten leer (die Nachbarn nehmen den Platz, Einblenden stellt die
  Standardanordnung wieder her), „eigenes Fenster“ dockt ab
  (DockContextQueueUndockWindow), Andocken setzt das Panel in seinen Knoten
  oder baut die Standardanordnung neu, „Zurücksetzen“ baut sie neu.
  `draussen` = nicht im DockSpace des Bereichs angedockt. Reiterleisten:
  AutoHideTabBar (zwei Panels in einem Knoten zeigen Reiter).
- Helfer (F1) kennt das Menü „Fenster“: `tools/gen_hilfe_orte.py` liest
  `menueInhalt` aus `gui/fenster_dock.cpp` (`fenstermenue()`), 8 neue Orte.

### Selbsttest dock2mass

Misst in Events, Mission und Model die Kindfenster der Panels (modebar, events,
##flowarea, buttons, toolbar, mapview, modelview, animliste, entlayers,
toolband) mit den Tabellen und im DockSpace im selben Lauf und vergleicht sie
(±0,5 Punkte).

- Lauf 1 (rot): Panels schwebend (66,131, winzig) – DockBuilderDockWindow
  wirkt nicht bei NoSavedSettings → SetNextWindowDockID.
- Lauf 2 (rot): 1–3 Punkte daneben (Randpunkt der Spalten, Zellpolster der
  inneren Tabelle, gerundetes Zeilenende, Band zu schmal).
- Lauf 3 (grün): alle 17 Kindfenster gleich (z. B. Events: modebar
  12,124 141x1295, events 166,124 435x1295, ##flowarea 614,121 1466x1304,
  buttons 2093,121 419x1298; Mission toolband 12,1244 2514x184).
- Lauf 4 (grün, 17 OK): Actions per DockBuilderDockWindow als Reiter zu
  Script Flow → ein Knoten mit zwei Fenstern im Hauptfenster, keine
  Standardanordnung mehr; „Anordnung zurücksetzen“ (Menü Fenster) baut neu
  auf → 4/4 Kindfenster von Events wieder mit den alten Massen; linker Griff
  80 nach rechts im DockSpace → Ereignisliste +80, Script Flow −80, Actions 0.
- spaltenUeberhang (Nebenbefund dock1): Lauf 5 rot – Events nach dem Start
  602/1478/432 (Überhang 40), nach einem Durchgang durch die Modi
  602/1479/432 (Überhang 39). Ursache: der Startwert 40 wurde erst ab 2
  Punkten Unterschied durch die Messung ersetzt. Behoben: die erste Messung
  gilt immer (`spaltenUeberhangGemessen`). Lauf 6 grün (19 OK): Start und
  nach den Modi 602/1479/432 (Überhang 39).
- Lauf 7 (26 OK, 2 FEHLER): Menü im DockSpace – Actions ausgeblendet bzw.
  herausgenommen, Script Flow blieb 1466 breit: Actions war Mittelknoten,
  ein leerer Mittelknoten bleibt stehen → Script Flow wieder Mittelknoten.
- Lauf 8 (27/1): ausgeblendet und herausgenommen → Script Flow 1898,
  eingeblendet und angedockt → 4/4 alte Masse. Reiter-Schritt rot:
  DockBuilderDockWindow in den Knoten von Script Flow, während Actions noch
  angedockt war – beim Abdocken verschmilzt ImGui den Baum, der Zielknoten
  verschwindet, Actions schwebte.
- Lauf 9: mit DockContextQueueDock bei noch angedocktem Actions Absturz nach
  dem Zurücksetzen (Zielknoten freigegeben). Test wie ImGuis Ziehen: erst
  abdocken, dann ablegen.
- Lauf 10 grün: **29 OK, 0 FEHLER** (17 Kindfenster gleich, Spalten nach dem
  Start, Reiter, Zurücksetzen, Griff, Aus-/Einblenden, eigenes
  Fenster/Andocken). dock1 mit Schalter aus danach: 86 OK, 0 FEHLER.

### Mit Schalter (BHED_DOCKSPACE=1)

- missionsbau 95 OK, 0 FEHLER; szenen 26 OK, 0 FEHLER.
- Modus 1, erster Lauf (nach ~6 Minuten abgebrochen): 13 FEHLER – die
  Elementsuche mit Fenster „###main“ fand nichts in angedockten Panels
  (##splitx, Klicks der Tour auf Close/Move down/Duplicate/Copy/Paste,
  „Interactive tour“): ein angedocktes Fenster ist seine eigene RootWindow.
  Behoben in `gui/elemente.cpp` (wurzelName): ein Panel im DockSpace des
  Hauptfensters heisst wie die Wurzel seines Dock-Baums („###main“); in
  schwebenden Dock-Knoten (Wirt mit DockNodeHost) bleibt der Panelname.
  Ohne Schalter unverändert (dock1 86 OK).
- Modus 1, zweiter Lauf: **2648 OK, 0 FEHLER** (wie dock1 ohne Schalter).
- kamera 95 OK, ton 34 OK, je 0 FEHLER.
- dock1 mit Schalter: 83 OK, 18 FEHLER – erwartet, dock1 prüft die alte
  Semantik: „zuhause“ über die Anordnungslisten, Andocken über den
  Schliessknopf des eigenen Fensters (angedockte Panels haben bei
  versteckter Reiterleiste keinen), Speichern/Laden der Anordnung (Panels
  tragen NoSavedSettings), Zurückziehen auf den alten Platz. Diese Prüfungen
  sind beim Umstellen auf den DockSpace neu zu fassen.
- Nicht mit Schalter gelaufen: karte, karteneditor, menueeditor (Bereiche 3/4
  unverändert), npcklasse, laserfalle, geschuetzanim, zielstrahl,
  effektgrund, figurdeform, dock1neustart.

### Regression (Schalter aus)

- dock1 (Schalter aus, mit dem Überhang-Fix): 86 OK, 0 FEHLER (wie vorher).
- ctest 158/158 (nach vollem, inkrementellem Bau; 0 Warnungen). Kein sauberer
  Neubau, kein Modus 1, keine übrigen Modi (Zeit) – der alte Weg ist ohne
  Schalter unverändert bis auf den Überhang-Fix (Events-Spalten nach dem Start
  jetzt wie nach einem Moduswechsel).
- lint_sources, lint_fassung, lint_tabellenid, lint_layout, lint_hoehe,
  lint_gapp, lint_imgui_context u. a.: ok.

### Offen (dock2)

- Schalter einschalten (ab Werk an), sobald dock1 (neu gefasst), dock1neustart
  und alle Modi mit `BHED_DOCKSPACE=1` grün sind (Modus 1, missionsbau,
  szenen, kamera, ton sind es schon); dann der alte Weg
  (spaltenFrei, Zurückziehen auf den alten Platz, Anordnungslisten
  draussen/versteckt, drawDockFenster für Events/Mission/Model) raus. dock1
  prüft heute das Herausnehmen über die Anordnungslisten – im DockSpace
  zeichnet drawDockFenster für Bereich 0–2 nicht, dock1 mit Schalter wird
  daher rot, bis das Menü umgestellt ist.
- Im DockSpace bekannt, ungeprüft: die Hover-Linie der alten Griffe zeichnet
  ins Hauptfenster und ist unter den Panels nur im 1-Punkt-Spalt zu sehen
  (Griff selbst greift über den Panels, FlattenChildren); ImGuis Dreieck der
  versteckten Reiterleiste liegt auf der linken oberen Ecke jedes Panels
  (0,55·Schrift Trefferfläche).
- Im DockSpace: Speichern/Laden der Dock-Bäume (Panelfenster tragen noch
  NoSavedSettings, der Baum wird je Sitzung neu gebaut), dock1-Anordnungen
  lesen oder mit Hinweis auf Standard; Andocken, wenn der eigene Knoten als
  Geschwister des aktuellen beim Abdocken verschmilzt (dann schwebt das
  Panel – über DockContextQueueDock nach dem Abdocken lösen, wie im Test);
  Script Flow ausgeblendet lässt den leeren Mittelknoten stehen.
- Map Editor (je NRC-Layout eine DockBuilder-Vorgabe), Menu Editor („Text
  unten“), Drag&Drop je OS-Fenster, OpenGL-Weg, alte Handbuchbilder, Handbuch
  „Fenster und Bildschirme anordnen“, Selbsttest: Panel im DockSpace auf ein
  eigenes Windows-Fenster und zurück, Neustart mit gespeichertem Dock-Baum,
  Bildzeit.
- Helfer: Orte des Menüs „Fenster“ sind im Katalog (ctest hilfe_orte), „Zeig
  es mir“ im laufenden Programm nicht geprüft.
- lint_endtable meldet 2 Stellen in app_karte_entitaeten.cpp:1876/1877 (nicht
  von dock2, Datei unberührt).

## dock3 – DockSpace fertig (Teil), Schalter

Auftrag: SP\auftrag_dock3.md (06:28–08:40). Schalter `BHED_DOCKSPACE` bleibt
**ab Werk aus** (Begründung unter „Schalter“).

### 1. Dock-Bäume speichern, laden, Neustart

- Die Panelfenster im DockSpace tragen kein `NoSavedSettings` mehr: ImGui
  schreibt ihre Lage, ihren Knoten und den Baum des DockSpace in den Ini-Text;
  `merkenLaufend` legt ihn wie bisher je Bereich in `fenster=b<n>` ab
  (`iniFiltern` nimmt den DockSpace-Baum mit, weil Panels des Bereichs darin
  stecken). Kein neues Format.
- `dockraum::uebernehmen`: steht der Baum des Bereichs (Wurzel
  `bhed.dockraum.<praefix>`) nach dem Start oder nach „Anordnung laden“ in
  ImGuis Knoten und hat jedes Panel eine Fenstereinstellung, gilt er; sonst wird
  die Standardanordnung gebaut. Die Heimknoten findet `heimAusBaum` über die
  Wege der Standardanordnung (`heimWege`, '0'/'1' je Teilung, wie `bauen`
  teilt) – Knotennummern dürfen sich ändern (`iniErsetzen` nummeriert beim
  Laden neu). `NoUndocking`/`NoDockingOverMe` der Modusleiste schreibt ImGui
  nicht mit, sie werden nach dem Übernehmen neu gesetzt.
- Nach dem Laden einer benannten Anordnung liest `dockraumNeuLesen` den Baum
  des Bereichs neu (vorBild).
- **Anordnungen aus dock1** (Liste „in eigenem Fenster“, Fensterlagen, kein
  DockSpace-Baum) werden übernommen, nicht verworfen: beim ersten Aufbau kommen
  die Panels der Liste nicht in ihre Knoten, sondern schweben an ihrer
  gespeicherten Stelle; danach ist die Liste leer (`dock::listeLeeren`) und es
  gilt der Baum. Ein Hinweis auf die Standardanordnung ist damit nicht nötig.
- Rückweg (Schalter aus) mit einer im DockSpace gespeicherten Anordnung: die
  Liste ist leer, alle Panels stehen zuhause; die Bäume bleiben unbenutzt im
  Ini-Text (ImGui löst ein Fenster, dessen DockSpace nicht eingereicht wird,
  beim ersten Begin).
- Im DockSpace haben Panels in eigenen Fenstern einen Schliessknopf (wie in
  dock1): Schliessen dockt sie an. Andocken, wenn der Heimknoten verschmolzen
  ist: Standardanordnung neu, **andere Panels in eigenen Fenstern bleiben
  draussen** (`draussenLassen`; vorher holte ein Andocken alle zurück).

### 2. Selbsttest dock1 im DockSpace

dock1 läuft mit und ohne Schalter; wo sich die Bedeutung ändert, prüft er mit
Schalter die DockSpace-Fassung: „in eigenem Fenster“ = nicht im DockSpace des
Hauptfensters angedockt (`dock1::aktiv`); Andocken durch Ziehen = die
Ebenenliste als Reiter auf die Missionsansicht (ImGuis Ziel in der Mitte des
Knotens, an jede Stelle des Hauptfensters), dann Zurücksetzen =
Standardanordnung (`dockraumStandard`); Actions ausgeblendet = Script Flow bis
an den rechten Rand. dock1neustart prüft zusätzlich den Baum im Hauptfenster
(Ebenenliste als Reiter neben der Ansicht nach dem Neustart) und die Übernahme
einer dock1-Anordnung (Model: Leiste schwebt bei 300,200 800 breit, übrige
Panels in der Standardanordnung, Liste leer; Zurücksetzen = Standard).

Fehler unterwegs (rot → grün im selben Test):

1. Panel eines gerade nicht gezeigten Bereichs galt als „draussen“
   (ImGui hält dann nur `DockId`, `DockNode` ist leer) – Herausnehmen im ersten
   Bild nach dem Bereichswechsel tat nichts (dock1 mit Schalter: 92 OK,
   2 FEHLER „ereignisse.liste/modell.ansicht gezeichnet“) → `draussen` folgt
   auch der `DockId` → grün.
2. dock1neustart setzte `a.teil` nicht: vor den Neustart-Schritten lief der
   ganze allgemeine Knopftest (Befehlstest …), die eigenen Schritte kamen
   nicht dran (Lauf nach 2 Minuten beendet, 0 Zeilen „dock1neustart“) → wie
   die anderen Modi `teil = 2` und Endschritte.
3. Ein Bau lief, während der Test geändert wurde: die Läufe a1–a3 (84/15)
   prüften noch den alten Test – MSBuild hielt selbsttest.cpp für aktuell. Nur
   bauen, wenn nichts mehr geändert wird.

### 4. Drag&Drop je OS-Fenster

`main_win32.cpp`: jedes neue eigene Fenster (Platform_CreateWindow) nimmt
Dateien an (`DragAcceptFiles`) und bekommt eine Fensterprozedur davor, die
WM_DROPFILES wie das Hauptfenster weitergibt (`dateienAusDrop`, Stelle in
Bildschirmpunkten); alles andere geht an ImGuis Prozedur. Selbsttest dock1:
eine .menu-Datei per WM_DROPFILES (`platform::dateiAblegen`, DROPFILES-Block
wie aus dem Explorer) auf das Windows-Fenster der herausgenommenen Fläche des
Menü-Editors → der Menü-Editor öffnet sie. Rot mit abgeschaltetem Haken
(92 OK, 1 FEHLER „abgelegte Datei im Menue-Editor geoeffnet“), grün mit Haken.
Handbuch: ein Satz in „Fenster und Bildschirme anordnen“ (4 Sprachen).

### Ergebnisse

(unter Last – fünf andere Agenten laufen parallel; Läufe im Ordner
SP\dock3_laeufe, je eigener Einstellungsordner)

| Lauf | Ergebnis |
|---|---|
| dock1, `BHED_DOCKSPACE=1` | vorher (dock2) 83 OK / 18 FEHLER; jetzt **92 OK, 0 FEHLER** |
| dock1neustart, `BHED_DOCKSPACE=1` (zweiter Prozess, selber Ordner) | **11 OK, 0 FEHLER** (Baum von Events und Mission übernommen, Zeitleiste + Script Flow draussen an 200,150 700x260, Ebenenliste als Reiter im Hauptfenster, dock1-Anordnung für Model übernommen) |
| dock1, Schalter aus | **86 OK, 0 FEHLER** (wie vorher; Datei auf eigenes Fenster: geöffnet) |
| dock1neustart, Schalter aus | **5 OK, 0 FEHLER** |
| dock2mass (schaltet selbst um) | **29 OK, 0 FEHLER** (wie dock2) |
| Drag&Drop rot (Haken aus) / grün | 92 OK 1 FEHLER / 92 OK 0 FEHLER |

Schlussrunde (Schalter aus = Normalweg; Modus 1 mit BHED_HANDBUCH):

| Prüfung | Ergebnis |
|---|---|
| Sauberer Neubau (`--clean-first`, alle Ziele) | 0 Warnungen, 0 Fehler |
| ctest | **158/158** (306 s, zum Teil parallel zum Modus-1-Lauf) |
| Modus 1 | **2648 OK, 0 FEHLER**, 0 Zusicherungen |
| menueeditor | **46 OK, 0 FEHLER** |
| karteneditor (`BHED_KARTE_NUR=fotos`) | **78 OK, 0 FEHLER** |
| Modus 1 mit `BHED_DOCKSPACE=1` | erster Lauf (unter Last) **2643 OK, 8 FEHLER**, alle in einem Tutorial: Klick „Next“ im Fenster Tutorials kam nicht an („unter der Maus: -“), danach 7 Folgeprüfungen der Tour (Schritte 5–7, „Do it for me“, Editor zu). Wiederholung allein: **2648 OK, 0 FEHLER** (wie ohne Schalter; der Tutorial-Klick war unter Last, vgl. dock1-Schlussrunde) |
| lint_sources, lint_fassung, lint_tabellenid, lint_layout, lint_hoehe, lint_gapp, lint_imgui_context, lint_feldinit | ok |
| lint_portability / lint_i18n / lint_endtable / lint_settings / lint_fremdprogramm | Befunde nur in nicht berührten Dateien (wie vorher) |

Gegen ein Vorbild geprüft: ImGuis eigenes Verhalten (Speichern/Laden der
Dock-Bäume über den Ini-Text, Andocken über ImGuis Ziel in der Knotenmitte,
Schliessen eines schwebenden Fensters) im laufenden Programm; Drag&Drop mit
einem DROPFILES-Block wie aus dem Explorer (nicht mit einem echten
Explorer-Zug – der echte Zeiger darf im Test nicht bewegt werden). Nicht
geprüft: OpenGL-Weg (Selbsttest läuft mit Direct3D 11), echtes Ziehen von
Dateien aus dem Explorer auf ein eigenes Fenster.

### Schalter

**Aus.** Punkt 5 verlangt 1–4 grün und alle Modi mit Schalter grün. Offen
sind Punkt 3 (Karten-Editor und Menü-Editor im DockSpace) und die alten
Handbuchbilder – damit gäbe es mit Schalter zwei Arten, Panels anzuordnen
(Events/Mission/Model im DockSpace, Map/Menu Editor wie dock1), und das
Handbuch beschriebe den alten Weg. Modus 1 mit Schalter ist grün (zweiter
Lauf allein), die übrigen Modi liefen mit Schalter in dock3 nicht. Der
Normalweg (Schalter aus) ist
unverändert bis auf: Drag&Drop auf eigene Fenster, Schliessknopf/Andocken nur
im DockSpace, dock1neustart-Lauf.

### Offen (dock3)

Erledigt aus „Offen (dock2)“: Speichern/Laden der Dock-Bäume samt Neustart,
dock1-Anordnungen, Drag&Drop je OS-Fenster, Andocken bei verschmolzenem
Heimknoten, Selbsttest „Panel im DockSpace auf eigenes Fenster und zurück,
Neustart mit gespeichertem Baum“ (dock1/dock1neustart mit Schalter).

- Punkt 3: Karten-Editor im DockSpace (je NRC-Layout eine DockBuilder-Vorgabe,
  Masse gleich) und Menü-Editor inkl. „Text unten“ – nicht begonnen.
- Ältere Handbuchbilder mit der Menüleiste ohne „Fenster“ – nicht neu
  aufgenommen (Handbuch-Modus je Sprache).
- Andocken, wenn der Heimknoten fehlt, baut die Standardanordnung der Panels
  im Hauptfenster neu (eine freie Anordnung im Hauptfenster geht dabei
  verloren; herausgenommene bleiben draussen).
- Ungeprüfter Randfall: Schalter an → aus → (Panel in dock1-Art herausgenommen)
  → an: dann gibt es Baum und alte Liste; der Baum gilt, die Liste bleibt
  stehen und würde erst beim nächsten Neuaufbau des Bereichs (Andocken ohne
  Heimknoten) ihre Panels draussen lassen. Zurücksetzen leert sie. Sauber
  wäre: beim Übernehmen eines Baums die Liste leeren – erst nachstellen.
- Danach: Schalter an, alter Weg (Anordnungslisten, Zurückziehen auf den
  alten Platz, drawDockFenster für Bereich 0–2) raus.

## dock4 – Docking fertig, Schalter an

Auftrag: SP\auftrag_dock4.md (30.10., ab 09:00). Ergebnis: Schalter
`BHED_DOCKSPACE` **ab Werk AN** (`BHED_DOCKSPACE=0` = Rückweg, im Handbuch unter
„Fehlerbehebung“).

### 1. Karten-Editor im DockSpace

`gui/dockvorgabe.{h,cpp}` (neu): eine Vorgabe = der Baum einer Anordnung aus
Blättern (Panels) und Teilungen (nebeneinander/untereinander). Je
NRC-Anordnung eine Vorgabe mit genau den Teilern von MainFrame::Create
(`kartenVorgabe`, app_karte.cpp): m_hSplit nebeneinander, darin m_vSplit und
m_vSplit2 untereinander, der eingebaute Gruppendialog als drittes Kind von
m_hSplit; Split mit der Belegung der Plätze. Schlüssel (= feste Wurzel
`bhed.dockraum.<schlüssel>`): `karte.regular`, `karte.regularleft`,
`karte.split.<belegung>`, je mit `.gruppe`. Floating hat keinen DockSpace.

- **Masse:** Die Teiler rechnen wie bisher (Qts Rechnung, Einklappen,
  MaximizeView); `feld()` misst im DockSpace nur, und nach `DockSpace()`
  bekommen die Blätter genau diese Rechtecke (`groessenSetzen`: Pos/Size des
  Blatts, SizeRef der Teilungen) – solange der Baum der Vorgabe entspricht
  (`passt`: jedes Blatt leer, mit seinem Panel oder ganz verschmolzen, kein
  fremdes Fenster, keine fremde Teilung). Dann zeichnet der Bereich NRCs
  Griffe (Punkte, Ziehen wie QSplitterHandle) über dem Abstand; ImGuis
  Teiler sind aus (`NoResize`, Randfarbe durchsichtig). Sonst gelten ImGuis
  Teiler und Masse, NRCs Griffe fehlen. Kein Blatt ist Mittelknoten – ein
  leeres Blatt gibt seinen Platz ab wie ein versteckter Teil eines QSplitter.
- Jedes Feld zeichnet in seinem Panel ein Kindfenster `##feld<Name>` wie
  vorher (Namen, Hintergrund, Polster 0) – die Fotos und Prüfungen der
  Karten-Selbsttests finden dieselben Fenster.
- Herausgenommene Ansichten zeichnen weiter (Echtzeit-Licht), Klick macht sie
  aktiv, Kürzel gelten dort (dock1 mit Schalter: Kamera im eigenen
  Windows-Fenster, Licht näher = Boden heller, Pfeil hoch fährt, Freelook 300
  Punkte = 15 Grad).
- Wechsel der Vorgabe (andere NRC-Anordnung, Belegung): der alte Baum weicht,
  schwebende Panels bleiben draussen. Neustart/geladene Anordnung: der Baum
  der aktuellen Vorgabe wird übernommen.

### 2. Menü-Editor im DockSpace

`gui/app_menue_dock.inc` (eingebunden in app_menue.cpp, im Menü-Editor nur ein
Block geändert): Vorgaben `menue.reiter` (Bausteine | Fläche | Reiter-Spalte)
und `menue.textunten` (Bausteine+Gliederung | Fläche | Eigenschaften, darunter
Text). zeichneInnen rechnet die Spalten wie bisher in einem Weg für beide
Layouts (`spalte`, `spaltenTeiler`); im DockSpace misst `spalte` nur. Neue
Panels `menue.eigenschaften`, `menue.text` (nur „Text unten“, nur im
DockSpace; Titel aus vorhandenen Texten). Ohne Menü stehen Panels in eigenen
Fenstern weiter da (mit dem Hinweis).

### 3. Rest aus dock2/dock3

- **Randfall an/aus/an:** (a) Liste aus dock1 neuer als der Baum: beim
  Übernehmen schweben ihre Panels, die Liste wird geleert
  (`alteListeSchweben`) – vorher blieb sie stehen und liess beim nächsten
  Neuaufbau ein schon wieder angedocktes Panel herausfallen. (b) Ohne
  DockSpace heraus und wieder zurück: ImGui löst das Fenster beim Zeichnen
  ohne DockSpace aus dem Baum; jetzt bekommt es beim Zurücklegen eine
  Knoten-Marke, mit DockSpace setzt der Bereich es in sein Blatt oder baut die
  Vorgabe neu (vorher schwebte es).
- **Hover-Linie der alten Griffe:** über den Panels meldete `IsItemHovered`
  nichts (das Fenster unter der Maus ist ein Panel bzw. der Wirt des
  DockSpace) – die Linie fehlte ganz (Foto: 0 von 107 Zeilen). Jetzt zählt der
  Hover-Zustand des Knopfs selbst (FlattenChildren), die Linie kommt in die
  Zeichenlisten der Panels (107/107 Zeilen, 2 Punkte breit). Dasselbe für
  NRCs Griffe und die Teiler des Menü-Editors (Zeiger zum Ziehen).
- **ImGuis Dreieck oben links:** ImGui zeichnet es in Begin unter den Inhalt
  (im Karten-Editor ganz verdeckt), und ein Klick darauf ging zugleich an die
  Ansicht darunter (die Ansichten fragen IsWindowHovered). Jetzt: sichtbar nur
  unter der Maus, über dem Inhalt; solange es die Maus hat, bekommt der Inhalt
  keine (`dreieckBeginnen`). Klick zeigt den Reiter, Ziehen nimmt heraus.
- **OpenGL:** `BHED_OPENGL=1` (DEBUGGEN.md 3.5) nimmt gleich den OpenGL-Weg;
  der Selbsttest liest das Bild eigener Fenster dort mit glReadPixels.
- **Dateien wie aus dem Explorer:** jedes Windows-Fenster hat
  WS_EX_ACCEPTFILES (die Bedingung, unter der der Explorer WM_DROPFILES
  schickt); ein Modell auf die XY-Ansicht im eigenen Fenster landet als
  misc_model unter dem Zeiger; mit `BHED_DOCK4_EXTERN` legt ein zweiter
  Prozess (`SP\d4_ablage_extern.py`, PostMessage über die Prozessgrenze wie
  der Explorer) die Datei ab.

### Fehler unterwegs (rot → grün im selben Test)

1. Menü-Editor im DockSpace: Absturz (ImGui-Zusicherung „SetCursorPos ... to
   extend window boundaries“) – der DockSpace setzte den Cursor zurück, danach
   kam kein Element mehr → nicht zurücksetzen (menueeditor 46 OK).
2. Reiter-Prüfung in dock4 zählte die Griffe der Werkzeugleisten mit – nur
   NRCs Teiler-Griffe (Testfehler).
3. Hover-Linie (dock4: 0 von 107 Zeilen) und Zeiger über NRCs Griffen und den
   Teilern des Menü-Editors (3 FEHLER) → Hover des Knopfs selbst.
4. Dreieck: über der Ansicht nicht zu sehen, Klick traf zugleich die Ansicht
   (2 FEHLER) → grün.
5. Randfall an/aus/an mit Liste (2 FEHLER) → grün; ohne DockSpace heraus und
   zurück (dock4: Karten-Editor zur Laufzeit umgeschaltet, 1 FEHLER, 40
   Kindfenster fehlten) → grün.
6. Herausnehmen eines Panels, dessen Bereich noch nicht gezeigt war (dock1:
   „menue.bausteine gezeichnet: nicht nach 120 Bildern“) → grün.
7. Neustart: schwebende Fläche des Menü-Editors ohne Menü nicht gezeichnet
   (dock1neustart 1 FEHLER) → gespeicherte Fenstereinstellung zählt → grün.
8. OpenGL: Bild der eigenen Fenster nicht gelesen (Puffergrösse aus
   FramebufferScale = 0) → Innenfläche des Windows-Fensters → grün.
9. Ablage aus dem zweiten Prozess landete daneben (misc_model -128 144 statt
   0 48): der Helfer war nicht DPI-bewusst und rechnete bei 150 % mit
   verkleinerten Fenstermassen – wie der Explorer DPI-bewusst → grün.
10. **Fotovergleich Schalter an/aus** (karteneditor-Fotolauf, 65 Fotos, je
   Lauf mit und ohne DockSpace): zuerst jedes Foto anders, entlang der
   Feldränder (≈6600 Punkte). Zwei Ursachen: (a) ein angedocktes Fenster
   schneidet seinen Inhalt um seine Randbreite ein – der Rand der Konsole
   fehlte; (b) das leere Panel liess ImGui Hintergrund und Rand der
   Kindfenster in die Zeichenliste von ##feldKonsole legen („render
   decorations in parent“ nur bei nicht leerem Elternfenster), der Feldrahmen
   kam darüber. Jetzt: angedockte Panels ohne Rand, ohne eigenen Hintergrund
   und mit einem Punkt ohne Fläche → **62 Fotos gleich**, 3 nur im Namen des
   Laufordners, den die Konsole zeigt.

### 4. Handbuch

Seite „Fenster und Bildschirme anordnen“ neu für den DockSpace (überall
andocken, Dreieck, Karten-Editor mit NRCs Massen, Menü-Editor beide Layouts,
Fehlerbehebung mit `BHED_DOCKSPACE=0`); Bilder h_dock_* neu (h_dock_zurueck
zeigt jetzt ImGuis Markierungen über dem Hauptfenster), h_dock_ecke neu – die
Fotos laufen mit leerem Missionsordner (vorher stand auf der Startseite ein Pfad
mit Benutzernamen).

Alte Bilder mit der Menüleiste ohne „Window“: Bildläufe `BHED_EDITORTEST=handbuch`
mit DockSpace je Sprache (en, de, zh-Hans, ja), Auswahl nach dem Ausschnitt
(`SP\d4_hb_auswahl.py`: Ausschnitt beginnt in der Schrift der Menüleiste und
reicht bis dorthin, wo „Window“ steht bzw. „Help“ hinrückte; je Sprache
vereint mit der englischen Liste, weil „Window“ in zh/ja weiter links steht),
daraus `tools/handbuch/bilder.py`. Karten-Editor: 29–33 Bilder je Sprache
(karteneditor, kartebauen, karte4/5/7/8/8b/9c, gpubacken mit dem Tutorial) –
sie zeigten auch noch nicht den Modus „Menu Editor“ in der Modusleiste.
Hauptmenüleiste (grosser Bildlauf, Übungsmission und Archiv auf X: wie
/y/zl4_hbvoll.sh): 28 Bilder je Sprache (Start, Skript, Optionen, Tutorials,
Karte). Die Karten-Editor-Bilder wurden nach dem Fotovergleich (Fehler 10)
noch einmal aufgenommen. es/cs: nicht neu aufgenommen (Auftrag: 4 Sprachen) –
ihre Bilder mit Menüleiste zeigen weiter kein „Window“, die neuen Texte der
Seite fallen dort auf Englisch zurück (13 Bilder, 76 Texte noch englisch).

### Ergebnisse (Schlussrunde dock4)

Unter Last (andere Agenten laufen parallel); Läufe in SP\dock4_laeufe, je
eigener Einstellungsordner (dock1neustart im Ordner von dock1). Programm aus
dem sauberen Neubau (`--clean-first`, alle Ziele): **0 Warnungen**; danach eine
Zeile in dockvorgabe.cpp, inkrementell gebaut, 0 Warnungen. **ctest 158/158.**

| Lauf (Schalter ab Werk an) | Ergebnis |
|---|---|
| dock4 (schaltet selbst um), mit Ablage aus einem zweiten Prozess | **101 OK, 0 FEHLER** |
| dock1 | **97 OK, 0 FEHLER** (Bildzeit 1 Viewport 4,20 ms, 3 Viewports 4,20 ms) |
| dock1neustart (zweiter Start) | **18 OK, 0 FEHLER** |
| dock2mass | **29 OK, 0 FEHLER** |
| menueeditor | **46 OK, 0 FEHLER** |
| karteneditor (`BHED_KARTE_NUR=fotos`, mit Fotos) | **78 OK, 0 FEHLER** |
| missionsbau / szenen / kamera / ton | **95 / 26 / 95 / 34 OK**, je 0 FEHLER |
| Modus 1 (mit BHED_HANDBUCH) | **2648 OK, 0 FEHLER** |
| ImGui-Zusicherungen | in keinem Lauf |
| OpenGL (`BHED_OPENGL=1`, dock1) | 96 OK, 1 FEHLER: Echtzeit-Licht – braucht Direct3D 11 (kartegpu::verfuegbar, vorher schon so); eigene Fenster zeichnen (Zeitleiste 832, Kamera 126, Fläche 285 Farben) |

Messwerte aus dock4: alle 46 Kindfenster der Felder und Spalten (Map Editor
Regular, Regular Left, Split, Split frei belegt, mit Gruppendialog, MaximizeView,
ohne XY/Kamera/Texturen, Griff gezogen; Menu Editor beide Layouts) im DockSpace
auf ±0,5 Punkte wie ohne – z. B. Regular XY 165,110 910x1194, Konsole 165,1312
910x64, Kamera 1083,110 1364x786, Texturen 1083,904 1364x472; Menü „Text unten“
Bausteine 165,111 312x803, Text 165,932 2361x358. Hover-Linie 107/107 Zeilen
2 Punkte breit; Dreieck 136/136 Punkte; Ablage aus dem zweiten Prozess:
misc_model bei 0 48 (soll 0 48).

Gegen ein Vorbild geprüft: NRCs Anordnungen gegen den bisherigen Weg ohne
DockSpace (der gegen NRC-Fotos geprüft ist, karteneditor „Anordnung Regular wie
die NRC-Fotos“), Punkt für Punkt im selben Lauf; ImGuis eigenes Verhalten
(Dreieck, Markierungen, Reiter) im laufenden Programm; Ablage wie aus dem
Explorer mit einem zweiten Prozess (WM_DROPFILES über die Prozessgrenze, Windows
reicht den Block weiter). Nicht geprüft: ein echtes Ziehen aus dem Explorer mit
der Maus (der echte Zeiger darf im Test nicht bewegt werden).

### Schalter

**Ab Werk AN.** Bedingung aus dem Auftrag erfüllt: 1–4 grün und mit Schalter
an grün: dock1, dock2mass, dock1neustart, menueeditor, karteneditor kurz +
Fotos, missionsbau, szenen, kamera, ton, Modus 1 (Tabelle oben), dazu dock4.
`BHED_DOCKSPACE=0` bleibt der Rückweg (Handbuch: Fenster und Bildschirme
anordnen > Fehlerbehebung) und ist geprüft: dock1 86 OK, dock1neustart 5 OK,
menueeditor 46 OK, karteneditor (Fotos) 78 OK, je 0 FEHLER.

### Offen (dock4)

- Events/Mission/Model (app_dockspace.inc, dock2) sind nur in Massen gegen den
  alten Weg geprüft (dock2mass ±0,5), nicht Punkt für Punkt im Foto wie der
  Map Editor; ihre Panels haben Polster (= Zellpolster), Fehler 10 betrifft sie
  so nicht, ein Fotovergleich steht aus.
- Echtzeit-Licht unter OpenGL: gibt es nicht (kartegpu nur Direct3D 11, vorher
  schon so) – der OpenGL-Weg zeigt die Kamera ohne Licht.
- Ein Bau ohne Direct3D (`-DBEHAVED_D3D11=OFF`) lässt sich nicht binden
  (karte_gpu_win32.cpp ruft D3DCompile, der Selbsttest gpu::figurDeform*,
  moverSprite* – vorher schon so); geprüft wurde der OpenGL-Weg daher mit
  `BHED_OPENGL=1`.
- Ein echtes Ziehen aus dem Explorer mit der Maus ist nicht geprüft (der echte
  Zeiger darf im Test nicht bewegt werden); geprüft: WS_EX_ACCEPTFILES je
  Fenster und WM_DROPFILES aus einem zweiten, DPI-bewussten Prozess.
- Andocken, wenn das Blatt eines Panels fehlt, baut die Vorgabe neu (eine
  freie Anordnung im Hauptfenster geht dabei verloren; Panels in eigenen
  Fenstern bleiben draussen) – wie dock3 für Events/Mission/Model.
- es/cs: Texte der Seite und Bilder mit Menüleiste noch englisch bzw. ohne
  „Window“.

## Changelog

## English
- Window layout like Blender: every panel (events list, Script Flow, Actions, mission view, entities, timeline, model view, map editor views, console, texture browser, menu editor canvas …) can be docked anywhere in the main window - beside, above, below or as a tab of another panel - or taken out into its own window on a second screen. Dock it back by closing its window, from the menu or by dropping it on a marker.
  - As long as nothing is moved, every area looks exactly as before, with the same dividers.
  - New menu **Window**: show/hide panels, put them into their own window, save layout as, load, delete, reset - per area; the current layout is restored at the next start.
  - The small triangle in the top left corner of a panel appears under the mouse: drag it to take the panel out, click it to show the panel's tab. A click on it no longer reaches the view below.
  - Windows on an unplugged or too small screen come back; text and size follow the scale of the screen a window is on.
  - Map editor: NetRadiant's layouts (Regular, Regular Left, Split, also with freely assigned places and the built-in entity window) stay the default with exactly NetRadiant's sizes and dividers; taken-out views keep drawing (real-time light included), free look and shortcuts work in the window you clicked.
  - Menu editor: both layouts - tabs on the right and text below - are made of panels.
  - Files can be dragged onto a panel's own window just like onto the main window.
  - If docking in the main window causes problems, start with `BHED_DOCKSPACE=0`: panels stay at fixed places and can only be taken out into their own window, as before (Guide: Arrange windows and screens > Troubleshooting).
- The helper (F1) also finds the entries of the **Window** menu.
- The columns of Events no longer move by one point after the first switch between the areas.

## Deutsch
- Fensteranordnung wie in Blender: jedes Panel (Ereignisliste, Script Flow, Actions, Missionsansicht, Entities, Zeitleiste, Modellansicht, Ansichten des Karten-Editors, Konsole, Texturbrowser, Fläche des Menü-Editors …) lässt sich überall im Hauptfenster andocken – neben, über, unter oder als Reiter zu einem anderen Panel – oder in ein eigenes Fenster auf einem zweiten Bildschirm nehmen. Zurück durch Schließen seines Fensters, über das Menü oder durch Ablegen auf einer Markierung.
  - Solange nichts verschoben ist, sieht jeder Bereich genau aus wie vorher, mit denselben Trennlinien.
  - Neues Menü **Fenster**: Panels zeigen/ausblenden, in ein eigenes Fenster nehmen, Anordnung speichern unter, laden, löschen, zurücksetzen – je Bereich; die laufende Anordnung ist beim nächsten Start wieder da.
  - Das kleine Dreieck in der linken oberen Ecke eines Panels erscheint unter der Maus: daran ziehen nimmt das Panel heraus, ein Klick zeigt seinen Reiter. Ein Klick darauf erreicht die Ansicht darunter nicht mehr.
  - Fenster auf einem abgezogenen oder zu kleinen Bildschirm kommen zurück; Schrift und Größe folgen dem Maßstab des Bildschirms.
  - Karten-Editor: NetRadiants Anordnungen (Regular, Regular Left, Split, auch mit frei belegten Plätzen und dem eingebauten Entity-Fenster) bleiben die Vorgabe mit genau NetRadiants Größen und Trennlinien; herausgenommene Ansichten zeichnen weiter (auch Echtzeit-Licht), Umschauen und Tastenkürzel gelten im angeklickten Fenster.
  - Menü-Editor: beide Layouts – Reiter rechts und Text unten – bestehen aus Panels.
  - Dateien lassen sich auf das eigene Fenster eines Panels ziehen wie auf das Hauptfenster.
  - Macht das Andocken im Hauptfenster Probleme, mit `BHED_DOCKSPACE=0` starten: die Panels stehen an festen Plätzen und lassen sich nur in ein eigenes Fenster nehmen, wie vorher (Handbuch: Fenster und Bildschirme anordnen > Fehlerbehebung).
- Der Helfer (F1) findet auch die Einträge des Menüs **Fenster**.
- Die Spalten von Events verschieben sich nach dem ersten Wechsel zwischen den Bereichen nicht mehr um einen Punkt.

## 中文
- 像 Blender 一样排列窗口:每个面板(事件列表、Script Flow、Actions、任务视图、实体、时间轴、模型视图、地图编辑器视图、控制台、纹理浏览器、菜单编辑器画布……)都可以停靠到主窗口的任意位置——另一个面板的旁边、上方、下方或作为其选项卡——也可以移到第二个屏幕上的独立窗口中。关闭其窗口、使用菜单或放到标记上即可重新停靠。
  - 只要不移动任何东西,每个区域看起来都与以前完全相同,分隔线也相同。
  - 新菜单 **窗口**:显示/隐藏面板、放入独立窗口、布局另存为、加载、删除、重置——按区域;当前布局在下次启动时恢复。
  - 面板左上角的小三角形在鼠标下方出现:拖动它即可移出面板,单击它显示面板的选项卡。单击它不会再同时作用于下方的视图。
  - 拔下的或过小的屏幕上的窗口会回来;文字和大小跟随窗口所在屏幕的缩放比例。
  - 地图编辑器:NetRadiant 的布局(Regular、Regular Left、Split,包括自由分配位置和内置实体窗口的情况)仍为默认,尺寸和分隔线与 NetRadiant 完全相同;移出的视图继续绘制(包括实时光照),自由视角和快捷键在所单击的窗口中生效。
  - 菜单编辑器:两种布局——选项卡在右侧和文本在下方——都由面板组成。
  - 可以像拖到主窗口一样把文件拖到面板的独立窗口上。
  - 如果在主窗口中停靠出现问题,用 `BHED_DOCKSPACE=0` 启动:面板固定在原位置,只能像以前一样移到独立窗口中(手册:排列窗口和屏幕 > 故障排除)。
- 帮助助手 (F1) 也能找到 **窗口** 菜单中的条目。
- 首次在各区域之间切换后,事件的列不再移动一个像素。

## 日本語
- Blender のようなウィンドウ配置:すべてのパネル(イベント一覧、Script Flow、Actions、ミッションビュー、エンティティ、タイムライン、モデルビュー、マップエディターのビュー、コンソール、テクスチャブラウザー、メニューエディターのキャンバスなど)を、メインウィンドウ内の好きな場所(別のパネルの横・上・下・タブ)にドッキングしたり、2 台目の画面の独立したウィンドウに取り出したりできます。ウィンドウを閉じる、メニューを使う、マーカーにドロップする、のいずれかで元に戻ります。
  - 何も動かさない限り、各領域は区切り線も含めて以前とまったく同じです。
  - 新しいメニュー **ウィンドウ**:パネルの表示・非表示、独立ウィンドウ化、レイアウトの保存・読み込み・削除・リセット(領域ごと)。現在のレイアウトは次回の起動時に復元されます。
  - パネル左上の角の小さな三角形はマウスを合わせると表示されます。ドラッグするとパネルを取り出し、クリックするとパネルのタブを表示します。クリックが下のビューに届くことはなくなりました。
  - 外した画面や小さすぎる画面にあったウィンドウは戻ってきます。文字と大きさはウィンドウのある画面の拡大率に従います。
  - マップエディター:NetRadiant のレイアウト(Regular、Regular Left、Split、場所を自由に割り当てた場合や内蔵のエンティティウィンドウ付きも含む)が NetRadiant とまったく同じ大きさと区切り線で引き続き既定です。取り出したビューも描画を続け(リアルタイムライトを含む)、自由視点とショートカットはクリックしたウィンドウで働きます。
  - メニューエディター:右側にタブがあるレイアウトもテキストが下にあるレイアウトも、パネルでできています。
  - メインウィンドウと同じように、パネルの独立ウィンドウにもファイルをドラッグできます。
  - メインウィンドウ内でのドッキングで問題が起きる場合は `BHED_DOCKSPACE=0` で起動します。パネルは固定の場所に置かれ、以前と同じく独立したウィンドウへの取り出しだけができます(ガイド:ウィンドウと画面の配置 > トラブルシューティング)。
- ヘルパー (F1) が **ウィンドウ** メニューの項目も見つけるようになりました。
- 領域を初めて切り替えた後に、イベントの列が 1 ポイントずれることがなくなりました。
