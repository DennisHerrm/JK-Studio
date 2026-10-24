# Karten-Editor, Stufe 3: Karten bauen

Stand: Y:\karte, Zweig `karte1` (auf Stufe 2, `d1e5ac4`). Vorbild ist
NetRadiant-Custom 20260114 (NRC, Git-Tag `20260114` in
`SP\nrc_src\netradiant-custom`, Binaries in `SP\nrc_dl\nrc`).
Quellangaben in den Kommentaren: `Funktion (datei:zeile)`.

`SP` = `C:\Users\shank\AppData\Local\Temp\claude\c--Users-shank-Downloads-behaved-1-0-0-rc567-behaved\af9c6614-e078-4efa-a574-1fa901786464\scratchpad`

Regeln wie in Stufe 2: je Punkt ein gezielter Rot/Gruen-Test (absichtliche
`// MUTATION` mit Marke, danach entfernt, grep = 0), ctest, ein Commit je
Punkt; radiant.exe nie gestartet.

## Punkt 0: Textbreiten wie Qt

### Was Qt bei NRC wirklich tut

NRC laeuft mit `AA_EnableHighDpiScaling`; bei 150 % rundet Qt 5.15 auf den
Massstab 2. Dann ist `QHighDpiScaling::isActive()`, und unter Windows nimmt Qt
fuer jede Schrift den DirectWrite-Font-Engine (qwindowsfontdatabase.cpp
`useDirectWrite`). Dessen Vorschuebe sind ungehintete Entwurfsmasse
(qwindowsfontenginedirectwrite.cpp `recalcAdvances`: `GetDesignGlyphMetrics`,
Entwurf * pixelSize / unitsPerEm als QFixed, also je Zeichen auf 1/64
gerundet). HarfBuzz setzt mit Unterschneidung aus GPOS 'kern' (die alte
'kern'-Tabelle nur ohne GPOS - Tahoma hat "Tr" nur dort).
`QTextLine::naturalTextWidth` zaehlt eine negative rechte Peilung des letzten
Zeichens dazu, `fm.boundingRect`/`fm.size` runden auf
(`QRectF::toAlignedRect`), `fm.horizontalAdvance` rundet.

Die Schriftgroesse ist die logische: Dialogschrift = `DEFAULT_GUI_FONT`
(Tahoma, lfHeight -11 bei 96 dpi = 8,25 pt) = **11 Punkte** (nicht 10,67),
Menueschrift Segoe UI 9 pt = 12 Punkte. Beleg am Foto
`dlg_Misc_Find_brush_0.png`: die Felder beginnen bei 86 = 11 + 69 + 6, und nur
diese Rechnung gibt "Entity number" = 68,36 -> 69 (GDI 11 px: 67; GDI in
Geraetegroesse 21/22 px halbiert: 66,5/67,5; Entwurf bei 10,67: 66,3).

Dazu `QRadioButton`/`QCheckBox::sizeHint`: `itemTextRect(..., enabled =
false, ...)` macht den Text mit `SH_EtchDisabledText` einen Punkt breiter.
Am Foto `dlg_Curve_Thicken_0.png`: die Kreise X, Y, Z, Normal liegen je 38
auseinander = 24 + ("X" 7 + 1) + 6; die Zeile ist 174 breit.

Umgesetzt in `gui/qttext_win32.cpp` (`platform::qtTextMass`, `qtTextBreite`,
`qtTextVorschub`): IDWriteTextLayout in Entwurfseinheiten (Schriftgroesse =
unitsPerEm, Modus NATURAL), ein eigener IDWriteTextRenderer sammelt Glyphen
und Vorschuebe; je Glyphe QFixed(Entwurf) + QFixed(Unterschneidung), dazu die
Peilung. `dwrite.dll` wird zur Laufzeit geladen (Importtabelle bleibt,
ctest `exe_importe`). Alle Dialoge (Formen, Transformationen, Patches,
Surface Inspector, Find Brush, Map Info, Ebenen, Preferences, Shortcuts) und
die Menues messen jetzt damit; `gdiTextBreite` bleibt nur als Werkzeug.

### Rot/Gruen

| Test | Rot | Gruen |
|---|---|---|
| ctest `karte_qttext` (neu, Erwartungen unabhaengig aus den Schriftdateien mit fontTools, `SP\s3\werk\qt.py`) | Mutation "wie vorher GDI": 13 Fehlschlaege | 0 |
| Selbsttest stufe2 "Find Brush: Felder bei 86, Innenraum 263 breit wie das NRC-Foto" | s3p0rot 371 / 1 (Feld ab 84, 261 breit) | s3p0gruen 372 / 0 |
| Selbsttest fotos: Menuebreiten jetzt mit Toleranz 1 statt 3 | (Stufe 1b mit GDI: bis 3 daneben) | s3p0menue 78 / 0, 30 von 38 Menues genau, 8 um +1 |

### Fotovergleich der Dialoge (Lauf s3p0gruen2, `SP\s3\werk\fotovergleich.py`)

Bildpunkte bei Massstab 2 (2 je Qt-Punkt), Innenflaeche wie in Stufe 2
gemessen.

| Dialog | Stufe 2 | jetzt | NRC | Abweichung jetzt |
|---|---|---|---|---|
| CSG Tool | 900 x 205 | 900 x 205 | 902 x 206 | -2 / -1 (kein Text, Rundung des Rahmens) |
| Prism (Sides) | 444 x 155 | 446 x 155 | 448 x 156 | -2 / -1 |
| Icosahedron | 510 x 195 | 514 x 195 | 516 x 196 | -2 / -1 |
| Arbitrary rotation | 440 x 199 | 446 x 199 | 448 x 200 | -2 / -1 |
| Arbitrary scale | 464 x 199 | 470 x 199 | 472 x 200 | -2 / -1 |
| Simple Patch Mesh | 456 x 199 | 458 x 199 | 460 x 200 | -2 / -1 |
| Exact Cylinder | 456 x 251 | 458 x 251 | 460 x 252 | -2 / -1 |
| Deform | 508 x 195 | 510 x 195 | 512 x 196 | -2 / -1 |
| Thicken | 486 x 247 | 506 x 247 | 508 x 248 | -2 / -1 |
| Surface Inspector | 582 x 727 | 582 x 727 | 570 x 728 | +12 / -1 (offen, s. u.) |
| Find brush | 520 x 199 | 524 x 199 | 526 x 200 | -2 / -1 |
| Map Info | - | 478 x 311 | 478 x 288 | inhaltsabhaengig (andere Karte im NRC-Foto) |
| About | 780 x 975 | 780 x 975 | 782 x 976 | -2 / -1 |

Alle Dialoge haben jetzt dieselbe Abweichung wie die textfreien (CSG Tool,
About): -2 / -1 Bildpunkte, also die Rundung des Rahmens, nicht mehr die
Texte. Offen bleibt der Surface Inspector: NRC zieht ihn auf die kleinste
Groesse, und die Knoepfe der Gruppe "Texturing" haben `setMinimumWidth( 10 )`
(surfacedialog.cpp:762-830) - die Spalten 43 / 44 / 67 / 67 sind also das
Ergebnis von qGeomCalc zwischen Mindest- und Wunschgroesse, nicht der
Knopftext + 16, mit dem wir rechnen. Das ist eine Layoutfrage, keine der
Textbreite.

Menues (Lauf s3p0menue, logische Punkte, NRC-Foto gegen uns): genau bei 30
von 38; +1 bei File, Modify > Components, Modify > Rotate, Misc > Theming,
Curve, Curve > Texture, Help > General, Help > Jedi Knight Jedi Academy. Mit
GDI (Stufe 1b) waren es bis 3 in beide Richtungen.

## Punkt 0b: eine Leiste, die umschaltet; Rahmen + Abstand

Auftrag shank (Bildschirmfoto: zwei Menueleisten uebereinander, dazwischen die
Skript-Reiter und eine leere Zeile): **"Eine Leiste, die umschaltet"** und
**"Rahmen + Abstand"**, zurueckhaltend ("dem Original treu, nur bisschen
besser erkennbar und sauberer").

Umgesetzt:

- Im Modus Karten-Editor ist die Menueleiste des Programmfensters selbst die
  von NRC (File Edit View Modify Build Grid Misc Entity Brush Curve Plugins
  Help, Reihenfolge und Inhalt wie `create_main_menu`). Schrift und Polster
  stehen schon beim `Begin` des Hauptfensters, damit sie wie NRCs QMenuBar 21
  Qt-Punkte hoch ist (`karteneditor::hauptleisteStilAn/Aus`,
  `zeichneHauptmenue`). Das Kindfenster des Editors hat keine eigene Leiste
  mehr; seine Lagen bleiben ab der Oberkante der Leiste gerechnet (der Editor
  beginnt bei y = 21), alle Masse-Pruefungen gelten unveraendert.
- Skript-Reiter und Kopfzeile fallen im Karten-Editor weg
  (`computeLayout`: `tabH`/`headerH` = 0), der Editor (Werkzeugleiste, dann
  die Ansichten) beginnt direkt unter der Leiste; die Modusleiste links
  bleibt. Wer den Modus verlaesst, hat wieder die Menues, Reiter und Zeilen
  des Programms. Tastenkuerzel unveraendert (nur mit Fokus im Editor).
- Programmweite Eintraege in NRCs Menues (Abweichungen von NRC, je so wenig
  wie moeglich):
  - **Misc**: am Ende ein Trenner und **Language**, **Color Scheme**,
    **Scaling** (dieselben Wirkungen wie View > Sprache/Farbgebung/Skalierung
    des Programms).
  - **Help**: oben **Manual** (F1, `OpenManual`; bei NRC registriert, der
    Menuepunkt auskommentiert, mainframe.cpp:1214) - oeffnet das Handbuch des
    Programms auf der Seite "Der Karten-Editor"; am Ende **About BehavEd**.
  - **Help > Check for NetRadiant update** (`CheckForUpdate`) startet die
    Update-Pruefung des Programms (NRC: OpenUpdateURL, eine Webseite).
  - **File > Exit** beendet das Programm (wie bisher).
  - Die Menuefotos Misc und Help sind damit hoeher als NRCs (Misc + 7 + 3 x 24,
    Help + 2 x 24); der Selbsttest erwartet genau das.
- Rahmen: jedes Feld (2D-Ansichten, Kamera, Konsole, Texturbrowser,
  eingebauter Gruppendialog, die schwebenden Fenster bei Floating) hat einen
  Rahmen von einem Bildpunkt auf seiner Innenkante, im Ton
  Flaeche + 18 % Richtung Schrift des Farbschemas (dunkel etwas heller, hell
  etwas dunkler), ohne Rundung, Schatten oder Polster - Lagen und Groessen
  bleiben die von NRC. Dazwischen NRCs Griff (PM_SplitterWidth 4) mit seinen
  Punkten wie bisher.
- Aktives Feld: das zuletzt angeklickte (jede Maustaste; wechselt die aktive
  2D-Ansicht per Befehl, folgt es ihr) hat den Rahmen in der Akzentfarbe des
  Farbschemas - dieselbe Dicke und Lage wie NRCs Umriss der aktiven 2D-Ansicht
  (XYWnd overlayDraw, 1 Bildpunkt), der ihn ersetzt; bei den 2D-Ansichten gilt
  weiter View > Show > Window Outline. Abweichung: NRC hebt nur 2D-Ansichten
  hervor (in der Farbe je Ansicht), wir jedes Feld in der Akzentfarbe.
- Handbuch: Seite "Der Karten-Editor" mit zwei neuen Absaetzen (eine Leiste,
  Programm-Eintraege; Rahmen, aktives Feld, Teiler) in vier Sprachen, alle 17
  Bilder der beiden Karten-Editor-Seiten neu (Laeufe hb3en/hb3de/hb3zh/hb3ja,
  das Hauptbild jetzt mit Menueleiste und Modusleiste).

Rot/Gruen (Selbsttest `BHED_KARTE_NUR=stufe3`, neue Schritte `stufe3::leiste`):

| Lauf | Ergebnis |
|---|---|
| s3p0brot (Mutation 1: im Karten-Editor die alte Programm-Leiste und die Reiter; Mutation 2: keine Rahmen) | 17 OK / 37 FEHLER (zwoelf Titel, keine Reiter, Misc/Help-Eintraege, alle Rahmen, Masse Rahmen) |
| s3p0b2, s3p0b3 | 43 OK / 0 FEHLER |

Gepruefte Wirkungen: genau eine Menueleiste mit NRCs zwoelf Titeln in
Reihenfolge, kein Programm-Menue (Debug); keine Reiter; Oberkante des Editors
= Unterkante der Leiste, Leiste 21 x Massstab hoch; Misc > Scaling > 125 %
skaliert das Programm; Help > Manual und F1 fragen das Handbuch an
(karteneditor.html), Check for NetRadiant update die Update-Pruefung, About
BehavEd oeffnet das About des Programms; Rahmen je Feld auf dessen Innenkante
(Groesse = Feld), genau eines in der Akzentfarbe (auch in der Zeichenliste),
Klick in Kamera / Konsole / XY verschiebt den Akzent; Window Outline aus:
kein Akzent in den 2D-Ansichten; Griff der Konsole bis unten gezogen: Konsole
eingeklappt, wieder hochgezogen: da; Regular Left, Split, Floating je mit
allen Rahmen; helles Schema: feiner Rahmen dunkler als die Flaeche, dunkles:
heller; Modus verlassen: Programm-Leiste mit Debug und Reiter zurueck,
zurueck im Editor nur die NRC-Leiste; dazu die Masse-Schritte (unten).

### Lagen gegen NRC (in den Rahmen gemessen)

Bei 1280 x 747 logischen Punkten (= NRCs Client im Foto, 2560 x 1494 bei 2x;
das beschnittene Foto 1721 x 1033 ist dessen linke obere Ecke) liefern die
Masse-Schritte dieselben Werte wie in Stufe 1b: Menueleiste 0/0/1280/21,
Werkzeugleiste 0/21/1280/39, XY 0/60/495/629, Griff 4, Konsole 33, Kamera
499/60/742/414, Texturen 248, Filterleiste 1241/60/39/606, Statuszeile
0/726/1280/21; Regular Left, Split (619/618 x 331) und Floating (400 x 300)
ebenso - alle gruen. Der Rahmen liegt auf der Innenkante, die Felder sind
also unveraendert.

Beim echten Massstab (2) im Fenster des Selbsttests (2538 x 1472, wie das
Fenster des Anwenders mit etwa 2559 x 1521): Bereich 1185 x 709 (mit der
Leiste), Werkzeugleiste 0/21/1185/39, XY 0/60/457/593, Griff 0/653/457/4,
Konsole 31, Griff h 457/60/4/628, Kamera 461/60/685/390, Griff v2 461/450,
Texturen 461/454/685/234, Filterleiste 1146/60/39/568, Statuszeile
0/688/1185/21 - QSplitter-Verhaeltnisse 384 : 576, 377 : 20, 250 : 150 wie
NRC (1142 x 384/960 = 456,8 -> 457). Vor Punkt 0b war der Bereich um die
Reiterzeile und die leere Zeile niedriger.

Fotos `laeufe\fotos_s3p0b\karteneditor_s3_*.bmp` neben
`SP\nrc_fotos\radiant_layout_*.png` und `radiant_duel1.png`: Lagen, Griffe
mit Punkten und der Umriss von einem Bildpunkt um die aktive Ansicht wie NRC;
der einzige Unterschied ist der feine Rahmen um jedes Feld und die
Akzentfarbe (NRC: Blau 0,0,255 nur um die aktive 2D-Ansicht).

## Punkt 0c: Stimmt alles von der Steuerung?

Auftrag shank: jede Mausbelegung von NRC pruefen, die Teiler (einklappen,
Griff am Rand, zurueckziehen) und Maximize view.

### Mausbelegung (Checkliste, Abschnitt "Mausbelegung")

Quelle `docs/mouse shortcuts.txt` (Tag 20260114, 51 Zeilen) gegen den Code
gelesen (xywindow.cpp XY_MouseDown/Up/Moved und EntityMenu, camwindow.cpp
enable/disable_freelook_*, wheelmove_scroll, Strafe-Modi, selection.cpp
RadiantWindowObserver/Selector_/Manipulator_/TexManipulator_,
selection_mtor_drag.cpp, selection_mtable_brush.h/_translate.h,
surfacedialog.cpp Scene_applyClosestTexture, texwindow.cpp, entity.cpp
EntitySetClassnameSelected). Ergaenzt um, was die Liste nicht nennt: die
Zusatztasten allein (onModifierDown/Up), die Varianten von M3 mit Alt
(Ausrichtung ohne Namen), worldspawn im Entitaetenmenue, Return/Umschalt+
Return/Strg+Return im Clipper, Esc in Menue und Filterfeld, Return/Esc/Tab/
Leertaste in Arbitrary rotation, die Teiler und F12.
`tools/karte/mausbelegung.py` haelt die Liste (Belegung | Ansicht | Wirkung
bei NRC | Ort bei NRC | Ort bei uns | Pruefung); `nrc_inventar.py erzeugen`
schreibt sie als Abschnitt "Mausbelegung" in die Checkliste.

Ergebnis: **62 Belegungen, alle 62 verhalten sich wie NRC** - jede mit echten
Maus-/Tastenereignissen und einer Wirkungspruefung im Selbsttest (37 neue
Pruefungen `Maus 2D/3D/Texturbrowser/Entitaetenmenue: ...` in
`stufe3::maus`, die uebrigen aus Stufe 1b/2 mit Namen in der Liste). Im Code
war keine Belegung falsch. Was die neuen Pruefungen zuerst rot zeigten, waren
Annahmen, die NRC anders macht (jeweils im Code von NRC nachgesehen und so
gelassen):

- M1 ohne Bewegung laeuft durch alle Treffer der Tiefe - auch das Licht der
  Karte, das zwischen den Pinseln C und A liegt (selection.cpp:1455 eCycle).
- Nach dem Umwandeln im Entitaetenmenue ist die neue Entity nicht gewaehlt
  (entity.cpp:132-163: neuer Knoten, der alte geloescht).
- Im Verschieben-Werkzeug greift ein Klick genau auf den Drehpunkt den
  Ursprung, nicht das Objekt (transformOrigin_isTranslatable,
  selection.cpp:1281); "M1 + Alt = senkrecht" gilt beim freien Ziehen im
  QE-Werkzeug (selection_mtable_translate.h:347).
- Die Oberseite von A war von C verdeckt; Umschalt+M2 kuerzer als 16 Punkte
  oeffnet das Kontextmenue (texwindow.cpp:1740).
- Strg+Umschalt+M1 oeffnet nur lose Shader-Dateien extern; Movie Duels hat
  `shaders/system.shader` lose im Ordner.

Gefunden und behoben: ohne Ersatzfunktion haette der Selbsttest bei
Strg+Umschalt+M1 die lose `system.shader` mit der Anwendung des Anwenders
geoeffnet (`platform::openWithDefaultApp`). Das ist in den ersten beiden
Laeufen (s3p0c1, s3p0c2) geschehen - die Datei wurde dabei nur geoeffnet, nicht
veraendert. Seitdem starten `kartenUrlOeffnen`/`kartenBefehlStarten` im
Selbsttest (BHED_EDITORTEST) nichts mehr, sie merken die Anfrage nur.

### Teiler und Maximize view

`stufe3::teiler`: jeder Teiler jeder Anordnung bis ans Ende gezogen (das Feld
dahinter klappt ein, der Griff steht am Rand: Griff = Ende des vorderen
Feldes), vom Rand zurueckgezogen (das Feld ist wieder da), dasselbe an den
Anfang - Regular (h, v, v2 und der Teiler im Texturbrowser), Regular Left (h,
v, v2), Split (h, v, v2), Regular mit eingebautem Gruppendialog (h:2);
Floating hat keine Teiler. F12 mit der Maus ueber der Kamera: die Kamera
fuellt den Bereich (1142 x 624), XY/Konsole/Texturen eingeklappt; F12 noch
einmal: alle Groessen wie vorher (XY 457 x 593). 47 Pruefungen.

### Rot/Gruen

| Lauf | Mutation | Ergebnis |
|---|---|---|
| s3p0crot2 | Griff versteckt, wenn ein Nachbar 0 gross ist; Maximize stellt nicht zurueck | 64 OK / 35 FEHLER (alle Teiler, F12) |
| s3p0crot3 | Tunnel immer "ersetzen"; Umschalt/Strg beim Auftragen vertauscht; Umschalt+M2 ohne Faktor 4 | 98 / 6 (Tunnel, vier M3-Varianten, Rollen) |
| s3p0crot6 | Orbit ohne camera_orbit_init | 103 / 1 (Orbit: Abstand 190,84 statt 194,81) |
| s3p0c13 (nur Maus und Teiler), s3p0c14 (Stufe 3 ganz) | - | 104 / 0, 128 / 0 |

Der erste Lauf mit Mutation fuer die Teiler (Griff versteckt, wenn das Kind
"eingeklappt" heisst) blieb gruen: beim Ziehen bis 0 setzt QSplitter nur die
Groesse 0, nicht das Flag - die Mutation traf nichts; erst die Pruefung auf
Groesse 0 zeigte die Wirkung. Ebenso war die erste Orbit-Pruefung zu weich
(der Punkt lag auf der Blickachse, Kreisen um 0 0 0 fiel nicht auf) und ist
jetzt neben der Achse.

Checkliste: 782 fertig, 152 ausgegraut, 2 offen von 936 Eintraegen (neu:
Mausbelegung 62 / 0 / 0).
