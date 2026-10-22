# Karten-Editor, Stufe 1b: die Oberflaeche

Stand: Zweig `karte1` in Y:\karte. Vorbild ist das veroeffentlichte
NetRadiant-Custom 20260114 (NRC, `SP\nrc_dl\nrc`, Quelltext = Tag `20260114`
in `SP\nrc_src\netradiant-custom`, ausgepackt nach `SP\nrc_20260114`).
Quellangaben in den Kommentaren: `Funktion (datei:zeile)`.

`SP` = `C:\Users\shank\AppData\Local\Temp\claude\c--Users-shank-Downloads-behaved-1-0-0-rc567-behaved\af9c6614-e078-4efa-a574-1fa901786464\scratchpad`

## Commits

| Commit | Punkt | Inhalt |
|---|---|---|
| `41cdb15` | 1 | Checkliste `tools/karte/nrc_inventar.md` (+ Erzeuger/Pruefer) |
| `eda17a8` | 2 | Arbeitsbereich: Neu/Oeffnen/Speichern/Zuletzt, Spiel aus dem Regelwerk |
| `e728ec8` | 3 | Anordnungen, Menues, Werkzeugleisten |
| `8568308` | 3b | Farben aus dem Farbschema des Programms (Entscheidung shank) |
| `594c60b` | 4 | Tastenkuerzel wie Qt, "Mapped Commands" |
| `cb2d91b` | 3c | eigene Vektorsymbole an NRCs Stellen; Feldgroessen mit Qts Rechnung |
| `cace8c2` | 3d | Checkliste aus NRC 20260114, Textbreiten wie Qt |
| `78185be` | 3e | Plugin-Menues, Menuesymbole, Dialoge wie NRC 20260114 |
| `ff5dbd0` | 5 | 2D-Ansichten XY / XZ / YZ |
| `41b24c7` | 6 | 3D-Kamera |
| `a1cf62b` | 6b | Einstellungsseiten nach den NRC-Fotos (Interface, Layout, Camera) |
| `61cc11f` | 7 | Auswaehlen und Bearbeiten (Manipulatoren, Befehle, Seite Selection) |
| `b1d37cf` | 8 | Texturbrowser (Baum, Filter, Vorschaubilder, Kontextmenue, Find / Replace, Shader-Text, Seite Texture Browser) |
| `cd4068b` | 8b | Texturen mit M3 kopieren und auftragen (TexManipulator) |
| `4178c3e` | 9 | Entitaeten: Entity Inspector (Reiter unten), Erzeugen-Menue 2D/Kamera, Licht- und Farbdialog, Entity List, Verbinden, Entity-Menue, Seite Entities |
| `7aa3c39` | 10 | Konsole wie NRC: Zeilen aus Stuecken, Meldungen beim Laden/Speichern, Umbruch, Auswahl, Kontextmenue |
| `89f473e` | 11 | Menues wie QMenu (Masse, Abreisskante, Mnemonik), jeder Befehl im Selbsttest, Fotos aller Menues/Seiten/Dialoge |
| `4303239` | 11b | Preferences und Mapped Commands mit Qts Massen (Tahoma 8 pt, Baumzeilen 16), Checkboxen 14 |
| `ca72a2e` | 12 | grosse Karte: Szenen in Geometrie und Rest geteilt (Auswahlwechsel 136 -> 7 ms), Kopien nach dem Hochladen frei |
| `c5b9fb9` | 13 | Handbuchkapitel "Der Karten-Editor" (4 Sprachen, 6 Bilder aus dem Selbsttest, AltGr-Hinweis); Reiter des Gruppendialogs repariert |
| (dieser) | 14 | Notizen fertig, Checkliste nachgezogen, Wirkungspruefungen fuer 7 Befehle |

## Masse des Hauptfensters (Auftrag shank: "Proportionen wie das Original")

### Was das Foto wirklich zeigt

Die ersten Fotos (`radiant_layout_<n>_*.png`, 1721 x 1033) sind nicht das
ganze Fenster: das Fotoskript fragt `GetWindowRect` ohne DPI-Bewusstsein,
Windows rechnet die Lage auf 96 dpi herunter (Bildschirm 2560 x 1600 bei
150 %, Arbeitsbereich 2560 x 1528), `PrintWindow` liefert aber echte
Bildpunkte. Das Foto ist darum die linke obere Ecke (1721 x 1033) des
maximierten NRC-Fensters (2582 x 1550, Client 2560 x 1494). Daher der
Eindruck "XY-Spalte 0-1012, Texturen nur unten": rechts und unten fehlt ein
Drittel.

Mit einem DPI-bewussten Abzug derselben Art (`SP\nrc_fotos\foto_layout_dpi.ps1`,
`SP\nrc_fotos\dpi\radiant_dpi_layout_0_0.png`, Regular; vor der Bitte, NRC
nicht mehr selbst zu starten, einmal aufgenommen) ist das ganze Fenster da.
Split und Regular Left sind am beschnittenen Foto gemessen (alle Teilungen
liegen im Ausschnitt).

Qt 5.15 (AA_EnableHighDpiScaling, Rundung Round, NRC main.cpp:410-411) rundet
150 % auf den Massstab 2: alles in NRC ist "logischer Punkt x 2". Die Schrift
ist Segoe UI 9 pt = 12 Punkte Geviert (Grossbuchstabe F 17 Bildpunkte hoch).
Der Editor rechnet deshalb in logischen Qt-Punkten mal `massstab()` =
gerundeter Bildschirmmassstab x Schriftgroesse des Programms (bei shank 2,
wie NRC).

### Wie Qt verteilt (nachgebaut in `src/karte/teiler.cpp`)

- QSplitter (qsplitter.cpp doResize, setSizes, moveSplitter): Platz im
  Verhaeltnis der gesetzten Groessen (`sizer`), Mindestgroessen zuerst,
  Griff 4. Kein "die GL-Ansichten schlucken den Rest": die Verhaeltnisse
  384:576, 377:20, 250:150 bleiben bei jeder Fenstergroesse. Ziehen setzt
  die neuen Groessen als Verhaeltnis; unter der halben Mindestgroesse klappt
  ein Feld ein.
- Split und Floating setzen keine Groessen (RestoreGuiState nur ohne
  FloatingGroupDialog): beide Spalten und Zeilen gleich (Split 619/618 x 331).
- Werkzeugleisten (QToolBarLayout): Rand 4, Griff 9, Knopf 31, Trenner 8,
  was nicht passt kommt hinter den Erweiterungsknopf (12, Lage
  Ende - Rand - 12 + 2). Rechter Andockbereich (QToolBarAreaLayoutLine::
  fitLayout): Filterleiste und darunter die Plugin-Leiste; erst beide
  Mindestmasse (60), der Rest der Reihe nach, die letzte bis zum Ende.
- Statuszeile (QStatusBar::reformat + QBoxLayout): 3 + 16 + 2 hoch, Abstand
  6, Befehl/Position/Textur teilen den Rest, Zaehler 168 (Zahlenfeld 30 =
  "99999" in Tahoma 8 pt, der Grundschrift beim Anlegen des QLabel).
- Menuetitel (QMenuBar::calcActionRects): Textbreite (aufgerundet) + 8,
  Abstand 6, erster bei 6.

### Tabelle: NRC gegen unseren Editor (Client 1280 x 747 logische Punkte)

NRC px = Bildpunkte im Foto (Client-Ecke bei 11/45 abgezogen), NRC log =
NRC px / 2; unser log = Rechteck im Selbsttest (Massstab 1, Arbeitsbereich
1280 x 747), per Bildvergleich am eigenen Foto
(`laeufe/fotos_m5/karteneditor_mass_*.bmp`) nachgemessen. Bei Massstab 2
sind unsere Bildpunkte genau das Doppelte.

Regular (Foto `dpi/radiant_dpi_layout_0_0.png`):

| Feld | NRC px (x, y, b, h) | NRC log | unser log | Abw. |
|---|---|---|---|---|
| Menueleiste | 0, 0, 2560, 42 | 0, 0, 1280, 21 | 0, 0, 1280, 21 | 0 |
| Werkzeugleiste | 0, 42, 2560, 78 | 0, 21, 1280, 39 | 0, 21, 1280, 39 | 0 |
| 1. Knopf (Flip H) | 26, 50, 62, 62 | 13, 25, 31, 31 | 13, 25, 31, 31 | 0 |
| CSG-Gruppe ab | 290 | 145 | 145 | 0 |
| Resize (Q) ab | 756 | 378 | 378 | 0 |
| XY | 0, 120, 990, 1258 | 0, 60, 495, 629 | 0, 60, 495, 629 | 0 |
| Griff XY/Konsole | 0, 1378, 990, 8 | 0, 689, 495, 4 | 0, 689, 495, 4 | 0 |
| Konsole | 0, 1386, 990, 66 | 0, 693, 495, 33 | 0, 693, 495, 33 | 0 |
| Griff Spalten | 990, 120, 8, 1332 | 495, 60, 4, 666 | 495, 60, 4, 666 | 0 |
| Kamera | 998, 120, 1484, 828 | 499, 60, 742, 414 | 499, 60, 742, 414 | 0 |
| Griff Kamera/Texturen | 998, 948, 1484, 8 | 499, 474, 742, 4 | 499, 474, 742, 4 | 0 |
| Texturbrowser | 998, 956, 1484, 496 | 499, 478, 742, 248 | 499, 478, 742, 248 | 0 |
| Filterleiste | 2482, 120, 78, 1212 | 1241, 60, 39, 606 | 1241, 60, 39, 606 | 0 |
| 1. Filterknopf | 2490, 146, 62, 62 | 1245, 73, 31, 31 | 1245, 73, 31, 31 | 0 |
| Reset Filters | 2490, 1066 | 1245, 593 | 1245, 593 | 0 |
| Erweiterung Filter | 2490, 1182, 62, 24 | 1245, 651, 31, 12 | 1245, 651, 31, 12 | 0 |
| Plugin-Leiste | 2482, 1332, 78, 120 | 1241, 666, 39, 60 | 1241, 666, 39, 60 | 0 |
| Polygon Builder | 2490, 1358 | 1245, 679 | 1245, 679 | 0 |
| Erweiterung Plugin | 2490, 1422, 62, 24 | 1245, 711, 31, 12 | 1245, 711, 31, 12 | 0 |
| Statuszeile | 0, 1452, 2560, 42 | 0, 726, 1280, 21 | 0, 726, 1280, 21 | 0 |
| Zaehler (Bild 1) | 1310 | 655 | 655 | 0 |
| Raster-Text | 2313 (Tinte) | 1156,5 (Tinte) | 1155 (Feld) | <=1 (Kantenglaettung) |
| Menuetitel File..Help (Tinte) | 10, 42, 77, 118, 169, 210, 248, 287, 331, 374, 419, 472 | | 11, 43, 77, 118, 170, 210, 248, 287, 331, 374, 420, 473 | <=1 (Kantenglaettung) |

Regular Left (beschnittenes Foto `radiant_layout_3_0.png`):

| Feld | NRC px | NRC log | unser log | Abw. |
|---|---|---|---|---|
| Kamera | 0, 120, 990, 828 | 0, 60, 495, 414 | 0, 60, 495, 414 | 0 |
| Texturbrowser ab | y 956 | y 478 | y 478 | 0 |
| XY ab | x 998 | x 499 | x 499 (742 x 629) | 0 |

Split (beschnittenes Foto `radiant_layout_2_0.png`):

| Feld | NRC px | NRC log | unser log | Abw. |
|---|---|---|---|---|
| Kamera | 0, 120, 1238, 662 | 0, 60, 619, 331 | 0, 60, 619, 331 | 0 |
| YZ ab | y 790 | y 395 | y 395 | 0 |
| XY ab | x 1246 | x 623 | x 623 (618 x 331) | 0 |
| XZ ab | 1246, 790 | 623, 395 | 623, 395 | 0 |

Floating (Fotos `radiant_layout_1_*.png`, Fenster einzeln): Hauptfenster ohne
Mitte (Menue, Werkzeugleiste, Filterleiste, Statuszeile wie oben). Die vier
Ansichten mit Innenmass 400 x 300 (Kamerafoto: Client 800 x 600 Bildpunkte),
Rahmen bei 50/100, 500/100, 500/450, 50/450 (mainframe.cpp:1756-1811) -
unsere schwebenden Fenster haben dasselbe Innenmass und dieselbe Lage des
Rahmens, ihre Titelleiste ist die von ImGui (Innenecke 24 tiefer statt der
Windows-Titelleiste). Gruppendialog 444 x 777 (groupdialog.cpp:85, Foto
910 x 1610 = 444 x 777 Client); seine Lage setzt Windows, wir mitten ins
Programmfenster.

Nicht messbar ohne Eingaben in Radiant: Mindestgroessen der Konsole und des
Texturbrowsers (wir: 0 - Unterschied nur beim Ziehen bis an den Rand) und
der Hinweis des eingebauten Gruppendialogs (wir: 444).

Vorher (Commit `594c60b`): Werkzeugleiste aus der Schriftgroesse
(Symbol = 2 x Schrift), Filterleiste Schrift x 2 + 10, Statuszeile
Textzeile + 8, Teilung als Anteil des Platzes, Massstab 1,5 statt Qts 2,
keine Plugin-Leiste, kein Erweiterungsknopf - der Selbsttest
`Masse Rahmen` schlug im ersten Lauf an (m1: Menue- und Statusbreiten,
Floating), danach gruen (m5: 91 OK, 0 FEHLER).

## Punkt 6: 3D-Kamera

Portiert (NRC-Datei:Zeile -> unsere Datei):

| NRC | hier |
|---|---|
| camwindow.cpp:207-364 (camera_t, Projektion, Fernebene, Modelview, Achsen) | `src/karte/kamera.cpp` |
| camwindow.cpp:367-434, 715-752 (Camera_FreeMove, Camera_motionDelta) | `Kamera::freiBewegen`, `strafeAusMaus` |
| camwindow.cpp:436-685 (Bewegungs-Flags, Cam_KeyControl, diskrete Schritte) | `Kamera::tastenSteuern`, `*Diskret` |
| camwindow.cpp:1097-1177, 1768-1803 (Freelook, Rechtsklick-Uhr) | `gui/app_karte_cam.cpp` |
| camwindow.cpp:1112-1123, 1222-1238 (Orbit) | `Kamera::orbitBeginnen/orbitRollen` (Treffer: Punkt 7) |
| camwindow.cpp:1256-1318 (Mausrad, camera_zoom mit affiner Inverse) | `Kamera::rad/zoom` |
| camwindow.cpp:1708-1765 (FloorHeightWalker) | `Kamera::etageWechseln` |
| camwindow.cpp:2144-2261 (CenterView, Focus, Cubic Clip) | `Kamera::winkelZuruecksetzen/fokusPosition`, `kubikHinein/-Hinaus` |
| camwindow.cpp:1929-2116, renderstate.cpp:1217-1867, 1962-2057 (Cam_Draw, Zustaende) | `kameraZeichnen`, `gui/karte_gpu_win32.cpp` (3D-Teil) |
| camwindow.cpp:811-918 (CamDrawWorkzone), 756-808 (CamDrawSize) | `workzone3d`, Groessentexte |
| textures.cpp:150-260, 330-420; texmanip.cpp:39-214 | `Bildlager`, `neuAbtasten` |
| gl/skybox_vp.glsl, skybox_fp.glsl | HLSL `ps3` (Wuerfel) |
| light.cpp:179-660, 714-828 (Kugel, Oktaeder, LightRadii) | `src/karte/kameraszene.cpp` |
| entitylib.h:46-300 (arrow_draw, aabb_draw_flatshade) | `pfeilLinien`, `kastenFuellung` |
| group.cpp:169-215 (Name/Pfeil der Gruppe) | `kameraSzeneBauen`, 2D `szeneBauen` |
| accelerator.cpp:88-170 (PressedKeys) | `kameraTasteVerbraucht`, `tastenVerarbeiten` |
| camwindow.cpp:2398-2466 (Seite Camera) | `kameraSeite` (app_karte_dialoge.cpp) |

Rot/Gruen: ctest `karte_kamera` (neu, 0 Fehlschlaege). Selbsttest
`karteneditor`: mit dem alten Kamerafeld (nur Hintergrund) 128 OK, 8 FEHLER
(Bild, Freelook, Toenung, Draht, Flach, Mausrad, W im Freelook), danach 136 OK,
mit allen Tasten/Befehlen der Kamera 156 OK, 0 FEHLER.

Gemessen im Selbsttest (Testkarte, Fenster 1370 x 704 Geraetepunkte):
Freelook 100 Bildpunkte = 50 logische -> Yaw -5,0 Grad; W 0,4 s -> 153
Einheiten (500 u/s nach 200 ms); Flatshade-Flaeche zur Kamera = Durchschnitts-
farbe x 1,0 auf +-1; Toenung = 0,7 x ohne + 0,3 x color_selbrushes3d auf +-1.

Abweichungen Punkt 6:
- Orbit (Alt+M2): Drehpunkt ohne Szenentreffer (256 vor der Nahebene); der
  Treffer (Scene_Intersect / BestPointSelector) kommt mit der Auswahl (Punkt 7).
- Doppel-M2 (Entitaetenmenue) merkt den Aufruf, das Menue selbst: Punkt 9.
- Modelle (misc_model, Klassen mit Modell) als Kasten, nicht als Modell
  (Stufe 2) - wie in 2D.
- Texturen werden im Hintergrund dekodiert; bis dahin fehlt der Stapel (NRC
  laedt beim Realisieren und blockiert).
- Licht der Pfeile/Kugeln: NRC nimmt die zuletzt gesetzte Normale (GL-Zustand);
  hier (0,0,-1) nach einem Kasten bzw. die GL-Vorgabe (0,0,1).
- Linienbreite 0,5 ($CAM_WIRE) als 1 Bildpunkt; Strichel- und
  Polygon-Versatz im Geometrie-/Pixel-Shader nachgebaut (D3D11 kennt beides
  nicht fuer Linien).
- Ohne Direct3D 11 (OpenGL-Rueckfall der Oberflaeche) zeigt die Kamera nur
  den Hintergrund und die Texte.
- "Selected faces wire" ist einstellbar, wirkt mit der Flaechenauswahl (Punkt 7).
- Strg+\, Strg+[ / Strg+] sind auf deutscher Tastatur wie in NRC nicht
  erreichbar (AltGr), im Test ueber die Befehle.
- Statistik "msec": Zeit unseres Zeichnens (CPU), nicht GL.

## Punkt 7: Auswaehlen und Bearbeiten

Portiert (NRC-Datei:Zeile -> unsere Datei; Einzelheiten je Funktion in den
Kommentaren):

| NRC | hier |
|---|---|
| include/selectable.h:30-92, libs/math/frustum.h:54-362, line.h, plane.h | `src/karte/auswahltest.cpp` (Schnitttests, SelectionIntersection) |
| radiant/selection_.h, selection_volume.h, selection_selector.h, view.h | `auswahltest.cpp` (SelectionVolume, BestPoint, Selector) |
| radiant/selection.cpp:1386-1600 (SelectPoint, SelectArea, Paint), 1941-1960 (Scene_TestSelect), brush.h/patch.h testSelect, light.cpp light_testselect | `src/karte/auswahlsystem.cpp` |
| libs/transformlib.h, math/quaternion.h, math/matrix.h, libs/pivot.h, selection_render.h, libs/render.h (draw_*) | `src/karte/manipulator.cpp` |
| radiant/selection.cpp:1008-1940 (RadiantSelectionSystem: SelectManipulator 1261, startMove 1257, MoveSelected 1749, endMove, ConstructPivot, translate/rotate/scale/skew, renderSolid) | `src/karte/bearbeitung.cpp` (`AuswahlBearbeitung`) |
| selection_mtable_translate.h (368), _rotate.h, _scale.h, _skew.h, _brush.h | `bearbeitung.cpp` (VerschiebeAchse, VerschiebeAchse2, VerschiebeFrei, KastenFang, VerschiebeFreiXyZ, DreheFrei, DreheAchse, SkaliereAchse, SkaliereFrei, VerzerreAchse, NeuerPinsel) |
| selection_mtor_translate.cpp (219), _rotate.cpp (311), _scale.cpp (157), _skew.cpp (392), _drag.cpp (605) | `bearbeitung.cpp` (Verschiebe-, Dreh-, Skalier-, Verzerr-, ZiehManipulator, BauManipulator, UrsprungManipulator) |
| libs/dragplanes.h (370), brush.h (selectPlanes, bestPlane*, selectByPlane, gatherPolygonsByPlane, evaluateTransform) | `bearbeitung.cpp` (ZiehFlaechen, BesteEbene) |
| radiant/selection.cpp:2482-2838 (Selector_, Manipulator_, RadiantWindowObserver) | `gui/app_karte_auswahl.cpp` |
| radiant/tools.cpp:1-441 (Werkzeuge, Statustexte, Toggle*Modes) | `gui/app_karte_auswahl.cpp` (`werkzeug`, `statusText`) |
| radiant/select.cpp:600-1300, 1840-1936 (Select_registerCommands) | `src/karte/auswahlbefehle.cpp`, Befehle in `gui/app_karte_auswahl.cpp` |
| radiant/map.cpp (Map_ExportSelected/ImportSelected, MapMerge*, Map_mergeClonedNames), libs/uniquenames.h, plugins/entity/namekeys.h | `auswahlbefehle.cpp` (kopieren, einfuegen, Namensraum) |
| radiant/brush.cpp vertexModeSnap / vertexModeBuildHull, libs/quickhull | `Pinsel::eckenEinrasten`, `third_party/quickhull/` (unveraendert) |
| radiant/selection.cpp:2313-2325 (SelectionSystem_constructPreferences) | Einstellungsseite Selection (`auswahlSeite`) |
| renderstate.cpp $WIRE_OVERLAY, $FLATSHADE_OVERLAY, $PLANE_WIRE_OVERLAY, $POINT, $BIGPOINT, $SELPOINT, $CAM_FACEWIRE; xywindow.cpp XORRectangle | `gui/app_karte_xy.cpp`, `gui/app_karte_cam.cpp`, `gui/karte_gpu_win32.cpp` (Punkte, breite Linien, Rechteck) |

Rot/Gruen: ctest `karte_auswahl` und `karte_bearbeitung` (neu, je 0
Fehlschlaege; u. a. Achsen-Verschieben mit Rasterfang, Drehen 90 Grad mit
Quaternion-Text wie NRC, Skalieren, Verzerren, Ziehen mit binaerer Suche
-32..-24, neue Pinsel 2D/3D, Snap to Grid mit QuickHull, Befehle mit NRCs
Rueckgaengig-Namen). Selbsttest `karteneditor`: mit abgeschaltetem
Beobachter und abgeschalteten Befehlen 159 OK, 26 FEHLER (k7rot), danach
185 OK, 0 FEHLER (k7a); mit den uebrigen Befehlen der Checkliste (Knoepfe
Resize/Translate/Rotate/Scale/Transform, Undo/Redo, Hide Selected samt
Rechtsklick, Shift+Space, Backspace, Z, C, I, Esc, Shift+A, Shift+V,
Strg+Shift+V, Alt+Pfeile, Ziffernblock +/-, Edit > Invert Selection)
zuerst 222 OK, 3 FEHLER (k7b: Undo/Redo stehen wie in NRC in
g_toolbarHiddenButtons, Hide Selected liegt bei dieser Fensterhoehe hinter
dem Erweiterungsknopf - der Test zeigt sie jetzt erst), dann 225 OK,
0 FEHLER (k7c); mit der Seite Selection (Felder, Vorgaben, Move style
umschalten, "Create brushes in 3D" aus -> kein neuer Pinsel in der Kamera)
229 OK, 0 FEHLER (k7f; davor k7d/k7e rot, weil ImGui Klappfelder nicht
meldet - der Test findet es jetzt ueber seine Zeile).

Gemessen: Drehen im Selbsttest ueber den Bildkreis ergibt
"rotateTool -eulerXYZ 0 -0 0.713938" (91,1 Grad) - der eine Bildpunkt
Versatz zwischen XY_ToPoint und window_to_normalised_device steckt auch in
NRC. Fotos: `karteneditor_verschieben`, `_drehen`, `_transform`,
`_rechteck` (halb aufgezogen), `_kamera_verschieben`.

Abweichungen Punkt 7:
- Clipper, UV-Werkzeug und Build-Modus: Knoepfe da, Funktion Stufe 2;
  Strg+M1 in 2D (schneller Clipper) tut bis dahin nichts.
- Ziehen: Extrudieren (Strg+Alt), Alt-Ziehen von Ecken in 2D und Einfuegen
  von Komponenten-Ecken: Stufe 2 (Komponentenmodi).
- Texturen mit M3 (TexManipulator): Punkt 8.
- Gruppen-Entitaeten ganz gewaehlt: die Kinder werden transformiert, der
  Schluessel "origin" der Gruppe bleibt (NRC: wie hier fuer Q3-Gruppen ohne
  Ursprung).
- Zwischenablage programmintern (NRC: QClipboard "RadiantClippings").
- Loeschen gewaehlter Komponenten (CSG): Stufe 2; der Befehl legt wie NRC
  den leeren Eintrag "deleteSelectedComponents" an.
- Statt revert/evaluate/freeze der Transform-Knoten ein Schnappschuss je
  Bewegung, danach die volle Transformation neu angewandt (gleiches Ergebnis,
  nur Nicht-Identitaeten wie TransformModifier::freezeTransform).
- Seite "Settings" (Nudge after duplication, Load last map) folgt mit den
  uebrigen Einstellungsseiten.

## Punkt 8: Texturbrowser

Portiert (NRC-Datei:Zeile -> unsere Datei):

| NRC | hier |
|---|---|
| plugins/shaders/shaders.cpp:820-1155, 1488-1523 (CShader, g_ActiveShaders, Try_Shader_ForName, FreeShaders, SetInUse) | `src/karte/texturbrowser.cpp` (`AktiveShader`) |
| radiant/renderstate.cpp:666-675, brush.h:355-361 (incrementUsed/decrementUsed, FaceShader::instanceAttach) | `texturenAktualisieren` (gui/app_karte_texturen.cpp): nach jedem Befehl die Shader der Karte |
| radiant/texwindow.cpp:95-134 (TextureGroups_compare/addShader/addDirectory), 1104-1188 (Baum) | `texturGruppen`, `texturGruppenBaum` |
| texwindow.cpp:153-331 (TextureBrowser: getTextureWH, setOriginY, clampOriginY, getOriginY, updateScroll) | `TexturBrowser` |
| texwindow.cpp:405-525 (TextureLayout, Texture_filtered, Texture_IsShown, evaluateHeight) | `TexturAnordnung`, `gezeigt`, `gesamtHoehe` |
| texwindow.cpp:634-810 (trackRecentFolders, ShowDirectory, ShowDirectoryOfShader, SetHideUnused, Focus) | `verzeichnisZeigen`, `verzeichnisDesShaders`, `fokus` |
| texwindow.cpp:812-902 (Texture_At, SelectTexture, Tracking, ViewShader) | `texturBei`, `texturKlick`, `ansichtMaus` |
| texwindow.cpp:919-1067 (TextureBrowser::draw) | `ansichtZeichnen` (Rahmen und Strichel Punkt fuer Punkt wie GL, siehe unten) |
| texwindow.cpp:1234-1270, 1577-1642, 1651-1885 (View-Menue, Kontextmenue, Filterfeld, TexWndGLWidget, constructWindow) | `gui/app_karte_texturen.cpp` |
| texwindow.cpp:1900-2151 (Befehle, Einstellungen, Seite) | `texturBefehl`, `texBroSeite` (app_karte_dialoge.cpp), app_karte_menue.cpp |
| libs/gtkutil/mousepresses.h, libs/timer.h (MousePresses, DoubleClickTimer 250 ms) | `druecken`, `loslassen` |
| radiant/findtexturedialog.cpp (FindTextureDialog), select.cpp:679-707 (FindReplaceTextures) | Dialog in app_karte_texturen.cpp, `texturenErsetzen` (auswahlbefehle.cpp) |
| select.cpp:489-502, 826-829, 889-892; brushmanip.cpp:646-717, 854-881; patchmanip.cpp:311-371 | `shaderSetzen`, `waehleNachShader`, `waehleFlaechenNachShader` |
| radiant/gtkdlgs.cpp:445-453 (DoShaderInfoDlg), 1600-1640 (QLineEdit_search), 2263-2510 (TextEditor, DoShaderView) | Shader Info, Shader-Text in app_karte_texturen.cpp; `Dateisystem::finden` (findFile) |

Rot/Gruen: ctest `karte_texturbrowser` (neu, 0 Fehlschlaege: Masse, Lagen,
Hoehe, alle Schalter, Filter, Hide nonShaders, Texture_At, Fokus, Klemmen,
ShowDirectory mit "Showing N shaders" und "Skipping invalid texture name",
ShowDirectoryOfShader, zuletzt gezeigte Ordner, Show All, Titel, Status,
Baum, Shader-Befehle mit NRCs Rueckgaengig-Namen). Selbsttest
`karteneditor`: mit dem alten Texturfeld (nur Hintergrund, Befehle aus)
240 OK, 29 FEHLER (k8rot), danach 270 OK, 0 FEHLER (k8f; dazwischen k8a-k8e mit Testfehlern: Textur ausserhalb des Bildes, Find-Dialog zu breit, common/ statt system/, lose shaders/system.shader im Spielordner).

Gemessen am DPI-Foto von NRC (radiant_dpi_layout_0_0.png, Massstab 2):
linker Teil 100 breit (Teiler { 100, 800 }, Dehnfaktoren 0 / 1), Griff 4,
Leiste 25 hoch (dark.qss "QSplitter QToolBar": Rand 1 + Polster 1, Knoepfe
16 + 3 + Rand 2 = 21, Abstand 1 -> Knoepfe bei 2 / 24 / 46), Filterfeld 22,
Baum ab 47, Zeilen 16, Text 24 vom Rand, Rollbalken 7 (dark.qss). Unser
Selbsttest misst dasselbe ("Lagen wie NRC"). Der rote Rahmen der gewaehlten
Textur (2 breit, Ecken ohne den aeussersten Punkt) und die Strichel (Muster
0xF000, Zaehler ueber die ganze Schleife: links abwaerts, unten nach
rechts, rechts aufwaerts, oben nach links) sind Punkt fuer Punkt mit dem
Foto verglichen (door01locked, 48 x 48).

Abweichungen Punkt 8:
- Tags (Tags GUI, Add/Rename/Delete/Copy/Paste/Search tag, Search Untagged,
  ShaderPlug-Tagdatei): Stufe 2, ausgegraut.
- Bilder laden im Hintergrund (wie die Kamera, Punkt 6); bis dahin steht
  nur der Name und der Rahmen. NRC laedt beim Zeigen und blockiert.
- Mindestbreiten der beiden Teile des Texturbrowser-Teilers nicht aus Qt
  nachgerechnet (0); sie zaehlen erst, wenn der Teiler sehr schmal wird.
- Der Shader-Text hat keine Hervorhebung der Shader-Schluesselwoerter und
  keinen Verweis ins Shader-Handbuch (QPlainTextEdit_Shader); ohne Datei im
  Ordner (aus dem .pk3) eine eigene Leseansicht mit hervorgehobener
  Fundstelle. "Open Shader Externally" startet bei losen Dateien das
  Standardprogramm (ShellExecute wie NRC) - im Selbsttest nicht ausgeloest.
- Shader Info ohne Fettschrift (der Text wie NRC).
- Die Zwischenablage (TextureClipboard_textureSelected) setzt erst der
  Surface Inspector (Stufe 2) zurueck; das Texturieren mit M3 kam mit 8b.

### Punkt 8b: Texturen mit M3 (TexManipulator)

| NRC | hier |
|---|---|
| radiant/selection.cpp:2434-2479 (TexManipulator_), 2718-2723 (onMouseDown c_button_texture) | `texturDruck/-Bewegung/-Loslassen` (gui/app_karte_auswahl.cpp) |
| radiant/surfacedialog.cpp:1107-1175 (EPasteMode, FaceTexture, FaceTextureClipboard_setDefault) | `src/karte/texturkopie.cpp` |
| surfacedialog.cpp:1180-1349 (PatchEdgeIter, Patch_getClosestTriangle) | `RandSchritt`, `naechstesDreieck` |
| surfacedialog.cpp:1353-1535 (Face/Patch/Light_get/setTexture) | `flaecheHolen/-Setzen`, `patchHolen/-Setzen`, `lichtHolen/-Setzen` |
| surfacedialog.cpp:1538-1702 (Face_getClosest, OccludeSelector, Scene_getClosestTexturable, Scene_copy/applyClosestTexture) | `naechstesTexturierbar`, `naechsteTexturHolen/-Setzen/-Auftragen` |
| radiant/patch.cpp:1212-1228 (Patch::ProjectTexture) | `Patch::texturProjizieren`, `Patch::texturkoordinatenTransformieren` |
| radiant/select.cpp:504-509, 915-923 (Select_SetTexdef, Select_ProjectTexture) | `texdefSetzenAuswahl`, `texturProjizierenAuswahl` |

M3 ohne Zusatztaste kopiert (Shader in den Browser, Werte in die Ablage),
mit Zusatztaste wird aufgetragen: Umschalt Werte, Strg nahtlos, beide
projiziert, Alt ohne Shader; ein Rueckgaengig-Rahmen von Druck bis
Loslassen mit NRCs Namen (paintTexture usw.). In 2D erreichen nur die
Kombinationen den Beobachter, die XYWnd nicht selbst nimmt (Alt+M3 usw.).
Rot/Gruen: ctest `karte_texturbrowser` (Arten, Namen, Kopieren, Werte,
nur Werte, nahtlos, Licht, Auswahl, Patch kopieren und projizieren);
Selbsttest mit abgeschaltetem M3-Zweig 270 OK, 3 FEHLER (k8brot), danach
273 OK, 0 FEHLER (k8g).
- Schrift der Namen: Segoe UI statt Myriad Pro (wie in den 2D-Ansichten).

## Punkt 9: Entitaeten (Entity Inspector, Erzeugen, Entity List, Verbinden, Farbe)

Portiert (NRC-Datei:Zeile -> unsere Datei):

| NRC | hier |
|---|---|
| radiant/entity.cpp:60-170 (EntitySetKeyValueSelected, EntitySetClassnameSelected, Scene_EntitySetClassname_Selected) | `schluesselSetzenAuswahl`, `klasseSetzenAuswahl` (src/karte/entitaetbefehle.cpp) |
| entity.cpp:276-300 (Entity_moveSelectedPrimitives), 304-332 (Entity_connectSelected), 380-497 (Entity_createFromSelection), 500-565 (NormalizeColor, Entity_normalizeColor, Entity_setColour), 607-612 (Entity_reloadDefinitions), 663-686 (Entity_constructPreferences), 708-746 (Menue, Befehle) | `primitiveZurEntitaet`, `entitaetenVerbinden`, `entitaetErzeugen`, `farbeNormiert`, `farbeNormierenAuswahl`, `farbeSetzenAuswahl`; `entitaetBefehl`, `entitaetenKlassenNeuLaden`, Seite Entities |
| plugins/entity/entity.cpp:127-200 (ConnectEntities, ScenePath_getEntity, connectEntities: makeUnique "<classname>1") | `entitaetenVerbinden` |
| radiant/map.cpp:1654-1692 (ParentSelectedBrushesToEntityWalker, Scene_parent(Subgraph)SelectedBrushesToEntity, CountSelectedBrushes), libs/scenelib.h:381-400 (ParentBrushes), brushmanip.cpp:844-848 (Brush_ConstructPlacehoderCuboid), filterbar.cpp (GetCommonShader) | `gewaehltePrimitiveUmhaengen`, `gewaehltePrimitive`, `platzhalterQuader`, `gemeinsamerShader` |
| radiant/select.cpp:420-458 (ExpandSelectionToEntitiesWalker), 711-757 (EntityFindByPropertyValueWalker), 850-887 (Select_EntitiesByKeyValue), 1764-1836 (Select_ConnectedEntities) | `auswahlAufEntitaetenErweitern`, `waehleNachSchluesselWert`, `waehleVerbundene` |
| radiant/entityinspector.cpp:86-106, 772-805, 1000-1140 (SelectedEntity_getValueForKey, Entity_GetKeyValues_Selected, Spawnflags, apply/clear) | `gewaehlteSchluessel`, `spawnflagsSetzen`, `schluesselLoeschenAuswahl`, `alleSchluesselLoeschen` |
| entityinspector.cpp:146-716 (Boolean/String/Shader/Texture/Color/Model/Sound/Angle/Direction/Angles/Vector3/List-Attribute, NonModalEntry), 838-998 (SetComment, SpawnFlags_setEntityClass, EntityAttributeFactory), 1193-1385 (EntityInspector_constructWindow) | `gui/app_karte_entitaeten.cpp` (Seite "Entities") |
| radiant/groupdialog.cpp:76-160 (GroupDlg: QTabWidget South, 444 x 777, Seitentitel) | `gruppenDialog`, `gruppenInhalt` (gui/app_karte.cpp) |
| radiant/entitylist.cpp, radiant/treemodel.cpp:44-215 (node_get_name, GraphTreeNode::Compare, BackgroundRole) | Entity List in app_karte_entitaeten.cpp |
| radiant/xywindow.cpp:743-878 (entitycreate_activated, EntityMenu, EntityClassMenuInserter, OnContextMenu), 969-1083 (EntityCreate_MouseDown/Move/Up), 1917-1934 (XYWnd_MouseToPoint, OnEntityCreate); camwindow.cpp:1097-1102 (context_menu_show) | `entitaetMenue` (Kern), `entitaetenMenueOeffnen`, `entitaetMenueGewaehlt`; app_karte_xy.cpp, app_karte_cam.cpp |
| radiant/gtkdlgs.cpp:408-443 (DoLightIntensityDlg), radiant/gtkmisc.cpp:331-337 (color_dialog) | Dialoge "Set Light intensity" und "Choose Color" |
| Qt 5.15 qcolordialog.cpp (QWellArray, QColorWell, QColorPicker, QColorLuminancePicker, QColorShower, initWidgets), qplatformdialoghelper.cpp (QColorDialogStaticData), qcolor.cpp (toHsv, toRgb, qt_div_257) | Farbdialog in app_karte_entitaeten.cpp |
| plugins/entity/namedentity.h:54-80, plugins/entity/entity.cpp:414-424 (KeyIsName::m_nameKey) | Namen in 2D/3D: classname, mit "Entity Names = Targetnames" targetname |

Rot/Gruen: ctest `karte_entitaeten` (neu, 0 Fehlschlaege: Schluessel der
Auswahl mit Vorgaben, entitySetKeyValue/entityDeleteKey/entityClear/
entitySetSpawnflags, Klasse wechseln in allen fuenf Faellen samt
Platzhalter-Quader und Rueckgaengig, entityCreate mit Trigger-Shader, Fehler
ohne Pinsel, offenem Schritt fuer light/misc_model, ungroupSelectedPrimitives,
movePrimitivesToEntity, entityConnectSelected mit makeUnique,
entitySetColour/entityNormalizeColour, Select by key/value, verbundene
Entitaeten, Expand Selection, Menue-Gruppen). Selbsttest `karteneditor`:
mit abgeschalteter Oberflaeche (Seite, Dialoge, Menue und Befehle als
leere Rueckkehr) 282 OK, 61 FEHLER (k9rot2; k9rot stuerzte noch an einem
Test ohne Nullpruefung ab), danach 328 OK, 0 FEHLER (k9g7; dazwischen
k9g-k9g6: der Doppel-M2 der Kamera-Tests oeffnete jetzt das Menue - die
Tests warten nun 0,3 s zwischen den Klicks wie ein Mensch; Escape schloss
das Wurzelmenue nicht und loeste danach UnSelectSelection2 aus; die
Kommentarschrift doppelt skaliert; Tests fuer Strg+M, Ungroup und das Ziehen
an "Intensity:" dazu).

Gemessen am Foto `dlg_View_Entity_Inspector_0.png` (Massstab 2, Innenmass
444 x 777): Grundschrift der Widgets ist MS Shell Dlg 2 = Tahoma 8 pt (Qt 5
nimmt unter Windows DEFAULT_GUI_FONT; Menues und Statuszeile Segoe UI 9 pt
aus lfMenuFont/lfStatusFont), fm.height() 13. Daraus und aus dem Foto:
Reiter unten 25 hoch (13 + PM_TabBarTabVSpace 12), Breite Text + 24
(Entities 61, Models 58, Layers 55), linksbuendig; Seite reicht 2 in die
Reiter (PM_TabBarBaseOverlap), Inhalt 2 innen (SE_TabWidgetTabContents):
440 x 750; Teiler 55 / 174 / 255 / 254 bei 750 (Foto 55 / 175 / 256 / 254;
Qts Verteilung von { 55, 175, 255, 255 } auf 750 - 12); Behaelter: Liste 171
(Foto 170,5), Key/Value 22 mit Abstand 6, Beschriftungsspalte = "Value",
Werkzeugknoepfe 21 (Symbol 16 + 5), der Doppelpfeil 37, Augen 27, Clear All
und Delete Key je 148, Raender 4; Zeilen der Listen 16, Text 3 nach dem
Rand (PM_FocusFrameHMargin + 1); Attribute im Gitter mit Rand 9, Spalten
111 : 333. Entity List (`dlg_View_Entity_List_0.png`): 350 x 500, Raender
4 / 0 / 4 / 4, Einzug 20, Zeilen 16, unten "AutoFocus on Selection" und das
Filterfeld mit Lupe.

Abweichungen Punkt 9:
- Reihenfolge beim Umhaengen von Primitiven und gleichnamiger Zeilen der
  Entity List: die der Karte. NRC laeuft durch die nach Zeigern geordnete
  InstanceMap bzw. sortiert gleiche Namen nach Knotenzeigern - das ist
  nicht nachbildbar.
- "Entity Names = Targetnames" gilt sofort (NRC: Neustart, wie die
  Fensteranordnung).
- Die Klassenliste sortiert wie QString::localeAwareCompare mit ICU fuer die
  vorkommenden Namen (Satzzeichen vor Ziffern vor Buchstaben, ohne
  Gross/klein, dann klein vor gross); Klassen, die erst die Karte anlegt
  ("unknown"), fehlen darin wie bei NRC (Liste beim Aufbau gefuellt).
- Dateiwahl fuer model/noise und misc_model: der Dateidialog von Windows wie
  NRC (file_dialog); die Liste der Modell-Endungen ist eine feste Auswahl
  statt der von assimp gemeldeten. Im Selbsttest nicht geoeffnet.
- Choose Color: ohne Ziehen von Farben zwischen den Feldern (Qt Drag&Drop)
  und ohne Tastaturbedienung der Farbfelder; "Pick Screen Color" liest den
  Bildschirmpunkt erst auf Knopfdruck des Anwenders (nie im Selbsttest).
  Eigene Farben stehen in den Einstellungen des Programms statt in
  QtProject.
- Der Gruppendialog ist bei Massstab 2 hoeher als unser Programmfenster
  (777 x 2): die Reiter liegen dann unten ausserhalb - wie bei NRC auf
  kleinen Bildschirmen.
- Die Dialoge von Punkt 9 nehmen Tahoma 8 pt wie NRC; die frueheren
  (Preferences, Mapped Commands, Find / Replace, Shader Info) noch Segoe UI
  9 pt - offen fuer den Lagevergleich in Punkt 11.
- Spinner mit -/+-Knoepfen (wie bisher); "Intensity:" laesst sich wie NRCs
  SpinBoxLabel ziehen.

## Punkt 10: Konsole

| NRC | hier |
|---|---|
| radiant/console.cpp:96-200 (QPlainTextEdit_console, Console_constructWindow, Sys_Print mit SYS_STD/WRN/ERR) | `gui/app_karte_konsole.cpp` (`konsoleSchreiben`, `konsoleZeichnen`); Qt 5.15 qwidgettextcontrol.cpp createStandardContextMenu |
| plugins/vfspk3/vfs.cpp:93-99, 215-292, 418-420 (InitPakFile, InitDirectory, initialise) | `Dateisystem::protokoll` (src/karte/texturen.cpp) |
| plugins/shaders/shaders.cpp:1449-1465, 1603-1670 (LoadShaderFile, ShaderList_addFromArchive, Shaders_Load) | `Texturinventar::protokoll` |
| radiant/eclass.cpp:131-213 (EntityClassesLoadFile, EntityClassQuake3_constructDirectory), eclass_def.cpp:287-367, eclass_xml.cpp:528-532 | `EntitaetsKlassen::protokoll` (src/karte/eklasse_lesen.cpp) |
| radiant/map.cpp:982-1022, 1249-1255 (ScopeTimer, Map_LoadFile, Map_Save); referencecache.cpp:64-140 (MapResource_loadFile/saveFile/save, file_saveBackup) | `karteOeffnen`, `karteSchreibenMitMeldungen` (gui/app_karte.cpp) |
| radiant/camwindow.cpp:2474-2485 (CameraSpeed_increase/decrease) | `kameraBefehl` (Text Ausgabe, Zahl Warnung) |
| radiant/texwindow.cpp:679, 724 (Skipping invalid texture name vor Showing N shaders) | `texturVerzeichnisZeigen` usw. |
| plugins/mapq3/plugin.cpp:263 (detectedFormat als Warnung) | schon im Leser, jetzt nach "success" |

Neu: `include/bhed/karte/protokoll.h` (Stuecke mit Art, wie die drei
Ausgabestroeme). Die Konsole: Zeilen aus Stuecken, Farben Ausgabe =
Textfarbe, Warnung (255, 127, 0), Fehler (255, 0, 0); nach '\n' ans Ende
rollen; Umbruch an der Breite an Wortgrenzen (sonst irgendwo), Auswahl
ziehen/Doppelklick Wort/dreifach Zeile/Umschalt erweitert; kein
Tastaturfokus (Strg+C bleibt das Kopieren der Karte); Kontextmenue beim
Loslassen: Copy (Ctrl+C, nur mit Auswahl), Trenner, Select All (Ctrl+A),
Copy All, Clear. Schrift Tahoma 8 pt, Rand 1 + documentMargin 4,
Rollbalken 7.

Rot/Gruen: ctest `karte_texturbrowser` (vfs directory, pak file, vfs
directory not found als Fehler, No shaderlist.txt found, Parsing
shaderfile) und `karte_klassen` (searching *.def zuerst, Ignoring
'mp_entities.def', ScanFile, *.fgd, *.ent). Selbsttest `karteneditor`: mit
der alten Konsole (jede Meldung eine Zeile, kein Ladeprotokoll, kein Menue,
keine Auswahl) 329 OK, 14 FEHLER (k10rot), danach 339 OK, 0 FEHLER
(k10g2/k10g3; k10g: der Leser meldet "detectedFormat" wie NRC als Warnung
zwischen "success" und dem Timer, der Test nahm das nicht an; das Speichern
im Test brauchte eine Auswahl fuer den Schluessel).

Abweichungen Punkt 10:
- Die Meldungen beim Laden des Spiels erscheinen, wenn das Laden im
  Hintergrund fertig ist (NRC: beim Start, blockierend); dazu bleibt unsere
  Warnung, wenn fuer das Spiel kein Ordner eingerichtet ist.
- Kein radiant.log (NRC schreibt nur mit "Log Console" in den
  Einstellungen); unsere Konsole geht ins behaved-detail.log.
- Pfade in den Meldungen mit '/' wie aus Qts Dateiwahl.
- Kein Fokus und keine Tasten in der Konsole wie bei NRC; ImGui zeigt die
  Kuerzel im Menue rechts wie Qt.

## Punkt 11: Selbsttest vollstaendig, Fotos gegen NRC, Layout-Vergleich

Der Modus `karteneditor` deckt jetzt ab: Karte oeffnen/speichern/vergleichen,
alle Anordnungen (Mass-Schritte, schon seit Punkt 3), Kuerzel, Pinsel mit
der Maus, Verschieben, Undo/Redo, Textur zuweisen, Entity + Schluessel,
NRC-Begriffe umschalten - dazu neu:

- `karteneditorAlleBefehleSchritte`: die Checkliste gegen die nachgebauten
  Befehle (jeder Befehl der Stufe 1b ist nachgebaut, keiner sonst; reine
  KeyEvents wie CameraFreeMove* zaehlen ueber `tastenEreignisNachgebaut`;
  145 Eintraege der Stufe 1b = 134 Befehle + 11 KeyEvents), dann 131 der
  134 nachgebauten Befehle einmal mit gewaehltem Pinsel und
  Licht (Umschalter zweimal, danach alle Dialoge zu), New Map zuletzt;
  nicht Exit (beendet), OpenMap und SaveMapAs (Windows-Dateidialog - ihre
  Funktionen pruefen die Schritte am Anfang). Raster, Kamera, Anzeige,
  2D-Ansichten, Texturbrowser, Fenster, Werkzeug kommen zurueck.
- `karteneditorFotoSchritte` mit "Begriffe wie NetRadiant": alle 38 Menues
  (Namen wie NRCs Foto-Skript), jedes gegen die Groesse des NRC-Fotos
  geprueft; Alt+E / E / F (Mnemoniken); alle 23 Einstellungsseiten; die
  Dialoge Shortcuts, Entity Inspector, Entity List. Je Foto eine `.rect`
  mit dem Ausschnitt; `SP\vergleich11.py` legt NRC und uns nebeneinander
  (`SP\vergleich11*`).
- `BHED_KARTE_NUR=fotos`: nur die Fotos (77 Schritte, ein Lauf ~2 min) zum
  Nachmessen.

### Menues wie QMenu (neu: `gui/app_karte_qtmenue.cpp`)

| Qt 5.15 / NRC | hier |
|---|---|
| qmenu.cpp:358-470 (updateActionRects), :2775 (paintEvent: Abreisskante), keyPressEvent (Mnemonik, Escape) | `qtMenueAnfang/qtPopupAnfang/qtPunkt/qtTrenner/menueAbschliessen/tastenAuswerten/escapeAmAnfang` |
| qcommonstyle.cpp:4951-4977 (CT_MenuItem: fm.height + 8, Breiten), :1487 (CE_MenuTearoff) | Zeile 24, Breitenformel, `abrissZeichnen` |
| qfusionstyle.cpp:1548-1763 (CE_MenuItem: Spalte 4/21, Haken (8,5,14,14), Symbol (7,4), Text 28, Taste Breite-15-tab, Pfeil), :3198-3229 (CT_MenuItem), Konstanten qfusionstyle_p_p.h:141-144, PM_SubMenuOverlap -1 | `eintragZeichnen`, `pfeil` |
| qstylesheetstyle.cpp:5173-5180 (Trenner = size() der Regel), dark.qss "QMenu::separator" (1 + 3 + 3, Linie 5..Breite-7) | Trenner 7 |
| mainframe.cpp:863 (Detachable Menus an), grid.cpp:224 und theme.cpp:160 (QActionGroup = Punkt) | Abreisskante, `exklusiv()` |
| build.cpp:1224 (Build_constructMenu), :1276 (LoadBuildMenu), gamepacks/ja.game und jk2.game/default_build_menu.xml | `buildEintraege`, `data/karte/ja|jk2/default_build_menu.xml` |
| mainframe.cpp:1116-1131, selection.cpp:918-940 (Modify > Repeat: " Translate 0 0 0" usw.) | Liste "Transforms" |
| gtkmisc.cpp:233-275 (Leistenmenue bleibt beim Klicken offen) | `leistenMenue` |
| Qt 5 QWindowsFontEngine (GDI) | `platform::gdiTextBreite` (Segoe UI 12 px fuer Menues, Tahoma 11 px fuer Dialoge) |

Gemessen am NRC-Foto: Help > General ist 335 breit = "Complete list of
Q3map2 command line parameters" (GDI 274) + 61 aus den Stilformeln - genau.
Alle 38 Menues (Lauf k11h, logische Punkte; Hoehe = Abreisskante 10 +
Zeilen 24 + Trenner 7):

| Menue | NRC | hier | Breite | Hoehe |
|---|---|---|---|---|
| File | 212 x 285 | 212 x 285 | +0 | +0 |
| Edit | 356 x 549 | 357 x 549 | +1 | +0 |
| View | 262 x 333 | 261 x 333 | -1 | +0 |
| View > Camera | 255 x 216 | 254 x 216 | -1 | +0 |
| View > Orthographic | 307 x 240 | 307 x 240 | +0 | +0 |
| View > Show | 253 x 401 | 254 x 401 | +1 | +0 |
| View > Filter | 270 x 593 | 269 x 593 | -1 | +0 |
| View > Region | 264 x 106 | 262 x 106 | -2 | +0 |
| Modify | 258 x 230 | 258 x 230 | +0 | +0 |
| Modify > Components | 177 x 82 | 177 x 82 | +0 | +0 |
| Modify > Nudge | 218 x 161 | 217 x 161 | -1 | +0 |
| Modify > Rotate | 206 x 137 | 207 x 137 | +1 | +0 |
| Modify > Flip | 183 x 137 | 183 x 137 | +0 | +0 |
| Modify > Repeat | 225 x 154 | 226 x 154 | +1 | +0 |
| Build | 193 x 405 | 195 x 405 | +2 | +0 |
| Grid | 163 x 346 | 161 x 346 | -2 | +0 |
| Misc | 262 x 178 | 260 x 178 | -2 | +0 |
| Misc > Theming | 263 x 497 | 264 x 497 | +1 | +0 |
| Misc > Theming > GUI Theme | 122 x 106 | 121 x 106 | -1 | +0 |
| Entity | 263 x 137 | 263 x 137 | +0 | +0 |
| Brush | 273 x 350 | 273 x 350 | +0 | +0 |
| Brush > CSG | 288 x 154 | 286 x 154 | -2 | +0 |
| Brush > Clipper | 264 x 82 | 264 x 82 | +0 | +0 |
| Curve | 277 x 429 | 279 x 429 | +2 | +0 |
| Curve > Insert Delete | 316 x 223 | 317 x 223 | +1 | +0 |
| Curve > Matrix | 255 x 168 | 256 x 168 | +1 | +0 |
| Curve > Texture | 225 x 106 | 225 x 106 | +0 | +0 |
| Plugins | 144 x 178 | 142 x 178 | -2 | +0 |
| Plugins > MeshTex | 159 x 429 | 160 x 429 | +1 | +0 |
| Plugins > ShaderPlug | 135 x 58 | 134 x 58 | -1 | +0 |
| Plugins > SunPlug | 166 x 58 | 167 x 58 | +1 | +0 |
| Plugins > UFO AI | 156 x 415 | 155 x 415 | -1 | +0 |
| Plugins > bobToolz | 201 x 597 | 202 x 597 | +1 | +0 |
| Plugins > brushexport2 | 253 x 58 | 250 x 58 | -3 | +0 |
| Plugins > prtview | 187 x 175 | 186 x 175 | -1 | +0 |
| Help | 215 x 130 | 214 x 130 | -1 | +0 |
| Help > General | 335 x 298 | 335 x 298 | +0 | +0 |
| Help > Jedi Knight Jedi Academy | 286 x 250 | 286 x 250 | +0 | +0 |

Die Hoehen stimmen alle, die Breiten bis 3 daneben (Qt misst Text mit
QTextLayout, wir mit GetTextExtentPoint32; ImGuis eigene ungehintete Breite
lag 0..+3 daneben, je Zeichen gerundete 12-px-Vorschuebe -1..-6).

### Dialoge mit Qts Massen (11b)

Qt 5 nimmt unter Windows fuer Item-Views (QTreeView, QListView) die Schrift
lfIconTitleFont = Segoe UI 9 pt, fuer alles andere SystemFont = MS Shell
Dlg 2 = Tahoma 8 pt (QWindowsTheme::refreshFonts). Am Foto Mapped Commands
nachgemessen: Zeilen "ArbitraryRotation" 180 Bildpunkte = Segoe UI (GDI
91 x 2), Kopf "Command" 97 = Tahoma (47 x 2), Knopf "Edit" 36 = Tahoma.

- Preferences (`gui/app_karte_dialoge.cpp`): Rand 11, Abstand 6, Baum 11/11
  147 x 470 mit Zeilen 16 (Text 35 bzw. 55, Pfeil 8 x 4 bei 18), Seite als
  QGroupBox 164/11 422 x 442 (Titel 166, Rahmen ab 28, Inhalt 176..574 ab
  39), Spalten nach qGeomCalc 111 : 333 mit den Mindestbreiten (Camera: 98 +
  6 + 294; Layout: die Bilderzeile 4 x 72 + 3 x 6 = 306 setzt Spalte 1),
  Zeilen 22 (Feld) / 20 (Checkbox) / 60 (Bilder + Radioknoepfe), Knoepfe
  80 x 22 bei 164, 420, 506. Alles am Foto pref_Camera/pref_Layout gemessen.
- Mapped Commands: 399 x 533, Suchfelder 11 und 159 (142 x 22), Baum 11/39
  290 x 483 mit Kopf 20 (Tahoma) und Zeilen 16 (Segoe), Spalten nach
  Inhalt (GDI), Rollbalken 7, Knoepfe 308/39 + n x 28 (80 x 22). Der Dialog
  wird bei jedem Oeffnen neu gebaut (leere Suche, nichts gewaehlt) wie
  DoCommandListDlg; die Knoepfe sind immer frei (ohne Wahl tun sie nichts).
- QCheckBox ueberall mit Kaestchen 14 (PM_IndicatorWidth), Text 6 nach
  indicator.right() (`qtHaken`): Spawnflags, boolean-Attribute, Entity
  List, Lichtstaerke, Find / Replace, Preferences.

### Fehler, die der volle Lauf fand

- Die Checkliste nahm RepeatTransforms (Ctrl+R) und ResetTransforms (Alt+R)
  ohne Taste auf: der Ausdruck in `nrc_inventar.py` endete am ersten ';' im
  Lambda. Behoben, Checkliste neu erzeugt.
- Das Leistenmenue schloss sich beim Klicken (NRC haelt es offen).
- Lauf k11e stuerzte bei RefreshShaders mit der Ausnahme 0x87D der
  D3D11-Debugschicht ab (Abbild: Aufruf aus behaved.exe in
  D3D11_3SDKLayers.dll). RefreshShaders gab die Bilder sofort frei, die die
  Zeichenlisten desselben Bildes noch nannten (der Schritt laeuft nach dem
  Zeichnen, ein Menueklick ebenso) - jetzt erst im naechsten Bild
  (`kartegpu::aufraeumen`). Ob das die Ursache war, ist nicht bewiesen: der
  Rot-Lauf k11rot (sofortige Freigabe, dreimal RefreshShaders mit sichtbaren
  Bildern) stuerzte nicht ab.

Rot/Gruen (Selbsttest `karteneditor`): k11a 368 OK / 3 FEHLER (Checkliste
ohne KeyEvents, View > Region nicht gefunden - der Klick in die Statuszeile
traf das hohe Filter-Menue), k11b 406/4 (Menuemasse), k11c 399/24
(Leistenmenue bleibt offen, Eintraege ausserhalb des Ausschnitts), k11e
Absturz (s. o.), k11rot 383/27 (GDI-Breiten, Toleranz 0); gruen k11f 410/0
(Commit 11), k11g 411/0 und k11h 411/0 (11b). ctest 87/87.

Abweichungen Punkt 11:
- Breiten der Menues bis 3 logische Punkte (s. o.).
- Die Abreisskante ist da, das Abreissen selbst (QTornOffMenu) kommt mit
  "Detachable Menus" in Stufe 2 (Tooltip).
- Gesperrte Eintraege ohne den Schatten von SH_EtchDisabledText; Farben,
  Kaestchen, Hervorhebung aus dem Farbschema des Programms.
- Oeffnet ein Buchstabe ein Untermenue, ist dessen erster Eintrag nicht
  hervorgehoben (Qt: setFirstActionActive).
- Fenstertitel: ImGuis Titelleiste statt der Windows-Titelleiste (NRC 45
  Bildpunkte bei 2x, wir kuerzer); die Innenmasse stimmen.
- Suchfelder: Lupensymbol statt Emoji im Platzhalter (schon Punkt 4).
- Spinner: Feld mit -/+ statt Qts Pfeilen (schon Punkt 6b).
- Der Gruppendialog (Entity Inspector) ist bei 2x hoeher als unser Fenster,
  die Reiter unten sind im Foto abgeschnitten (schon Punkt 9).
- Die Fotos von NRC zeigen andere Karten (Entity List) und andere Zustaende
  (Raster 16, ein MRU-Eintrag leer); fuer File leert der Test die MRU-Liste.

## Punkt 12: Leistung auf duel_mos_espa

Karte: `SP\karte_dekomp\md\q3\duel_mos_espa.map` (q3map2-Dekompilat des
Movie-Duels-BSP): 824 Entities, 31671 Pinsel, 6820 Patches. Selbsttest
`karteneditorGrossSchritte` (`BHED_KARTE_GROSS=<pfad>`, im vollen Lauf am
Ende; `BHED_KARTE_NUR=gross` nur sie): Laden mit `karteOeffnen`, 240 Bilder
mit sich drehender Kamera und wanderndem 2D-Bild, fuenf Auswahlwechsel,
Speicher (privat, Heap ueber HeapSummary, Grafikspeicher ueber DXGI).

| Messung | vorher (g1-g11) | nachher (k12b; g12 ohne Debugschicht) |
|---|---|---|
| Laden (`karteOeffnen`, blockierend) | 0.65-1.12 s | 0.79 s |
| Editor je Bild (zeichne, CPU) | 2.4 ms Mittel, 4.7 ms 95 % (ohne D3D-Debugschicht 1.2 / 2.4) | 2.5 / 4.8 ms (ohne Debugschicht 1.4 / 3.0) |
| Bildabstand | 4.2 ms (240 Hz) | 4.2 ms |
| Bild nach einem Auswahlwechsel | 136 ms (Kamera-Szene 111, 2D 15) | 7.9 ms (Kamera-Szene 3.9, 2D 1.3) |
| Speicher privat | +1182 MB | +899 MB |
| davon Heap (new/malloc) | - | +171 MB (Karte 110, buildBRep 64) |
| Grafikspeicher | +598 MB | +524 MB |

Laden im Hintergrund ist nicht noetig: 0.8 s fuer die groesste Karte, NRC
laedt ebenfalls blockierend (Map_LoadFile mit ScopeTimer).

Was geaendert wurde:
- Die Kamera-Szene in zwei Teilen (`KameraTeil::Geometrie` / `Rest`,
  `src/karte/kameraszene.cpp`): die Shader-Stapel der Pinsel und Patches
  haengen nicht an der Auswahl und bauen nur neu, wenn sich die Karte
  aendert (`geometrieStand()`: Undo, Laden, Verstecken, Pinselfarbe); der
  Rest (Punktentitaeten, Gruppen, Namen, Lichtradien, alles Ausgewaehlte)
  bei jedem Auswahlwechsel. Gezeichnet werden beide in denselben drei
  Durchgaengen (deckend, additiv, durchscheinend); die Reihenfolge der
  durchscheinenden Shader-Stapel bleibt die der Geometrie.
- Die 2D-Szenen ebenso (`geo` mit allen Kanten in ihrer Farbe, das
  Ausgewaehlte in `wahl` breit darueber).
- Der Stapel der letzten Flaeche wird ohne Kleinschreiben und Suchen
  wiederverwendet, die Matrix je Stapel steht im Stapel (statt einer Map).
- Nach dem Hochladen gibt die Kamera ihre Kopien der Dreiecke und Linien
  frei (rund 190 MB); ein Puffer der Grafikkarte, der mehr als doppelt so
  gross wie noetig ist, wird neu angelegt (nach dem Drahtmodell blieb der
  Linienpuffer sonst bei 108 MB).
- Der Rest des privaten Speichers (+730 MB ueber dem Heap) gehoert dem
  Grafiktreiber (Puffer und Texturen; Grafikspeicher +524 MB).

Rot/Gruen: die Messlaeufe g10/g11 zeigten 136 ms nach jedem Auswahlwechsel
(Kamera-Szene 111 ms); danach k12b: 416 OK / 0 FEHLER mit den neuen
Pruefungen "Auswahlwechsel bauen nur den Rest neu" (kleine Testkarte und
grosse Karte: Geometrie 0 Neubauten, Rest 10) und "Kamera-Szene nach dem
Hochladen nicht mehr im Speicher". ctest 87/87.

Abweichungen Punkt 12:
- Im Textured+Wire-Modus zeichnet $CAM_WIRE jetzt auch die Kanten
  ausgewaehlter Flaechen grau; das Overlay der Auswahl liegt darueber (wie
  vorher sichtbar).
- Im 2D liegt unter dem Ausgewaehlten seine Kante in der normalen Farbe
  (von der breiten Auswahllinie verdeckt).

## Punkt 13: Handbuch

Neue Seite `tools/handbuch/inhalt_karteneditor.py` (id `karteneditor`,
Gruppe "Karte und Szene", nach "Die Karte"/"Entities"/Szene/Wegpunkte):
Oeffnen und Speichern, Menues/Fenster/Anordnungen mit "Begriffe wie
NetRadiant", 2D-Ansicht und Kamera, Waehlen und Bauen, Texturen, Entities,
Konsole, Tastenkuerzel mit "Mapped Commands" und der AltGr-Hinweis
(Strg+[ / Strg+] = Far Clip Plane In/Out auf deutscher Tastatur nicht
erreichbar wie in NRC; Ausweg Menue View > Camera oder andere Taste). Dazu
eine Zeile "Karten-Editor" in der Modusleisten-Tabelle von "Jeder Knopf"
(`inhalt_knoepfe.py`). Alle Beschriftungen als `{{Kennung}}` aus
`tools/i18n_karte.py` - dafuer laedt `gen_handbuch.py` jetzt auch die
Karten-Texte (`from i18n_karte import KARTE`). Jede beschriebene Bedienung
vorher im Code nachgesehen (app_karte_xy.cpp Maus, app_karte_auswahl.cpp
M1/M2 mit Umschalt/Strg, app_karte_cam.cpp Freelook, befehle_liste.inc
Tasten, app_karte_texturen.cpp Klicks, app_karte_dialoge.cpp OK der
Einstellungen).

Bilder: Selbsttest `karteneditorHandbuchSchritte()` (BHED_EDITORTEST=handbuch,
BHED_HB_NUR=karteneditor, BHED_HB_KARTE=`laeufe\hb\mp_duel1.map` aus
`SP\karte_dekomp\ja\q3\mp__duel1.map`): h_karteneditor (ganzer Editor),
h_karteneditor_menue (View), h_karteneditor_entities (Entity Inspector mit
info_null, dessen Klasse eine Beschreibung hat), h_karteneditor_texturen
(Feld Texturbrowser), h_karteneditor_einstellungen (Seite Layout),
h_karteneditor_tasten (Mapped Commands); Laeufe hben, hbde, hbzhhans, hbja,
`tools/handbuch/bilder.py` je Sprache. `gen_handbuch.py`: 134 Seiten x 4
Sprachen, `--pruefen`: handbuch_sprachen ok. ctest 87/87.

Fehler beim Bildermachen gefunden und behoben (Rot/Gruen): die Reiter des
Gruppendialogs (Entities, Models, Layers) waren verdeckt - ein heller Balken
mit nur "Layers" darin. Ursache: ImGui 1.92.8 hat bei `ImDrawList::AddRect`
`thickness` und `flags` vertauscht; `AddRect(a, c, farbe, 2 * g_s,
ImDrawFlags_RoundCornersBottom)` (app_karte.cpp, seit Punkt 9) gab die Flags
(192) als Dicke weiter. Die anderen AddRect-Aufrufe im Programm geprueft:
alle mit sechs Werten (rounding, 0, thickness) treffen die alte Ueberladung
richtig, keiner sonst uebergibt Flags als fuenften Wert. Neue Pruefung
`zeichnungImFenster("/##gruppeinnen_")` (alles Gezeichnete liegt im
Fenster): rot im Lauf hben vor dem Fix ("372 Punkte, 90 ausserhalb, 94 Pixel
draussen"), gruen danach ("240 Punkte, 0 ausserhalb"); dieselbe Pruefung
steht im Karten-Editor-Test nach "Lagen wie NRC: Reiter unten". Die
bisherigen Fotos des Gruppendialogs hatten die Reiter nicht gezeigt (bei
Skalierung 2 ragt der 777 hohe Dialog unter den Bildschirmrand), deshalb
fiel es dort nicht auf.

Beobachtung ohne Aenderung: die Klassen aus den Movie-Duels-Definitionen
(`entdefsErgaenzen`, z. B. info_player_duel2) haben oft nur eine Leerzeile
als Beschreibung - fuer das Bild nimmt der Selbsttest deshalb die erste
Punktentitaet mit mindestens 40 Zeichen Beschreibung.

## Punkt 14: Abschluss der Stufe 1b

### Tests am Ende

- ctest: 87 / 87 (nach jedem Punkt, zuletzt nach Punkt 14).
- Voller Selbsttest-Modus `karteneditor` mit grosser Karte (Lauf k14a,
  `SP\k14.sh`): 422 OK / 0 FEHLER. Neu gegenueber k12b (416): die
  Reiter-Pruefung aus Punkt 13 und fuenf Wirkungspruefungen (ShowBboxes,
  ToggleGridSnap, NextView, ToggleModelBrowser / ToggleLayersBrowser,
  ToggleTextures), die vorher nur als "ausgefuehrt" galten.
- Handbuchbilder in vier Sprachen (hben, hbde, hbzhhans, hbja): je 1 OK /
  0 FEHLER, `gen_handbuch.py --pruefen` ok.
- Grosse Karte (duel_mos_espa, k14a): Laden 798 ms, Editor je Bild 2.58 ms
  (95 % 5.86), Bild nach Auswahlwechsel 6.6 ms (Kamera-Szene 3.0, 2D 1.1),
  Neubauten nach Auswahlwechseln Geometrie 0 / Rest 10, Speicher privat
  +927 MB (Heap +171 MB, Grafikspeicher +524 MB).
- Nicht gelaufen (nach Absprache "Tests buendeln" fuer den Schlussschritt
  vor dem Zusammenfuehren): Gesamttest Modus 1, alle uebrigen Modi,
  Kartenpruefung, perf_lauf.sh.

### Checkliste (`tools/karte/nrc_inventar.md`)

Gesamt: 329 fertig, 452 ausgegraut, 93 offen von 874 Eintraegen.

| Abschnitt | fertig | ausgegraut | offen |
|---|---|---|---|
| Menues | 99 | 126 | 12 |
| Werkzeugleisten | 13 | 37 | 4 |
| Registrierte Befehle | 175 | 220 | 1 |
| Dialoge und Fenster | 7 | 0 | 17 |
| Einstellungsseiten | 5 | 0 | 12 |
| Einstellungsfelder | 30 | 1 | 48 |
| Plugins | 0 | 67 | 0 |

In Punkt 14 nachgezogen: Preferences (Menue und Befehl; getestet mit
Alt+E, F), Model Browser, Layers Browser, Show Entity Boxes, ToggleGridSnap
(neue Wirkungspruefungen), Dialog "NetRadiant Preferences" (Rahmen, Baum,
23 Seiten), MRU-Eintrag &1 (im Selbsttest angeklickt); Set 2D Background
image, Bug report und About auf "ausgegraut, Stufe 2" (so zeigt sie das
Menue). Was offen bleibt und warum:

- Exit (Menue und Befehl): nachgebaut (beendet behaved mit dessen
  Rueckfragen), aber im Selbsttest nicht ausfuehrbar.
- Werkzeugknoepfe Open, Save, Change views: in NRC von Haus aus versteckt
  (ToolbarHiddenButtons), ueber das Kontextmenue der Leiste zu zeigen; ihre
  Befehle sind getestet, ein Klick auf den eingeblendeten Knopf nicht (Open
  oeffnet den Windows-Dateidialog). Knopf und Menuepunkt Texture Browser:
  nur in Floating/Split bzw. mit eingebautem Gruppendialog, dort nicht
  einzeln angeklickt.
- MRU-Eintraege &2 bis &9 (derselbe Weg wie &1), der dynamische
  Menuepunkt mainframe.cpp:1125.
- 17 Dialoge, 12 Einstellungsseiten, 48 Felder: gehoeren zu Funktionen der
  Stufen 2 und 3 (Liste unten).

### Portierte Dateien (NRC 20260114 -> unsere Datei)

Die Zeilen je Funktion stehen in den Kommentaren (`Funktion (datei:zeile)`)
und bei den Punkten 6 bis 12 oben.

| unsere Datei | portiert aus |
|---|---|
| `src/karte/befehle.cpp`, `befehle_liste.inc`, `menues.inc` | radiant/commands.cpp, mainframe.cpp (create_main_menu 1225ff., Werkzeugleisten 1328ff., Befehle 2073ff.), filterbar.cpp, gtkmisc.cpp; Erzeuger tools/karte/gen_befehle.py |
| `src/karte/eklasse.cpp`, `eklasse_lesen.cpp` | radiant/eclass.cpp, eclass_def.cpp, eclass_xml.cpp, libs/eclasslib.h |
| `src/karte/karte.cpp`, `kartenformat.cpp` | radiant/map.cpp, referencecache.cpp, brushtokens.h, patch.h, libs/entitylib.h, traverselib.h |
| `src/karte/pinsel.cpp`, `quickhull.cpp`, `wicklung.cpp`, `texdef.cpp` | radiant/brush.cpp/.h, brushmanip.cpp, winding.cpp, brush_primit.cpp |
| `src/karte/patch.cpp` | radiant/patch.cpp/.h, libs/render.h |
| `src/karte/raster.cpp`, `rueckgaengig.cpp`, `zuletzt.cpp` | radiant/grid.cpp, undo.cpp, mru.cpp |
| `src/karte/texturen.cpp`, `texturbrowser.cpp`, `texturkopie.cpp` | plugins/shaders/shaders.cpp, plugins/vfspk3/vfs.cpp, radiant/image.cpp, qe3.cpp, texwindow.cpp, select.cpp, surfacedialog.cpp |
| `src/karte/xyansicht.cpp` | radiant/xywindow.cpp/.h, light.cpp |
| `src/karte/kamera.cpp`, `kameraszene.cpp` | radiant/camwindow.cpp, renderstate.cpp, light.cpp, group.cpp, generic.cpp, namedentity.h, libs/view.h |
| `src/karte/auswahlsystem.cpp`, `auswahltest.cpp`, `auswahlbefehle.cpp` | radiant/selection.cpp, select.cpp, selection_selector.h, selection_volume.h, selectable.h |
| `src/karte/bearbeitung.cpp`, `manipulator.cpp` | radiant/selection.cpp, selection_mtor_*.cpp, selection_mtable_*.h, dragplanes.h, brushmanip.cpp, libs/pivot.h |
| `src/karte/entitaetbefehle.cpp` | radiant/entity.cpp, entityinspector.cpp, select.cpp, xywindow.cpp, map.cpp |
| `src/karte/teiler.cpp` | Qt 5.15 qlayoutengine.cpp, qsplitter.cpp, qboxlayout.cpp, qtoolbarlayout.cpp; radiant/mainframe.cpp:1717-1870 |
| `gui/app_karte.cpp` | radiant/map.cpp, mru.cpp, qe3.cpp, mainframe.cpp (Anordnungen), groupdialog.cpp |
| `gui/app_karte_menue.cpp` | radiant/mainframe.cpp, filterbar.cpp, gtkmisc.cpp, colors.cpp, mru.cpp, qe3.cpp |
| `gui/app_karte_qtmenue.cpp` | Qt 5.15 qmenu.cpp, qcommonstyle.cpp, qfusionstyle.cpp, qstylesheetstyle.cpp; NRC theme.cpp / dark.qss |
| `gui/app_karte_befehle.cpp` | radiant/commands.cpp, accelerator.cpp, grid.cpp, mainframe.cpp |
| `gui/app_karte_dialoge.cpp` | radiant/preferences.cpp, mainframe.cpp, commands.cpp (Mapped Commands), Qt-Masse (Tahoma 8 pt, Fusion) |
| `gui/app_karte_xy.cpp` | radiant/xywindow.cpp, map.cpp |
| `gui/app_karte_cam.cpp` | radiant/camwindow.cpp, renderstate.cpp, texmanip.cpp, textures.cpp |
| `gui/app_karte_auswahl.cpp` | radiant/select.cpp, selection.cpp, tools.cpp |
| `gui/app_karte_texturen.cpp` | radiant/texwindow.cpp, gtkdlgs.cpp, findtexturedialog.cpp |
| `gui/app_karte_entitaeten.cpp` | radiant/entityinspector.cpp, entitylist.cpp, entity.cpp, xywindow.cpp, camwindow.cpp, gtkdlgs.cpp, gtkmisc.cpp, treemodel.cpp; Qt qcolordialog.cpp |
| `gui/app_karte_konsole.cpp` | radiant/console.cpp |
| `gui/karte_gpu_win32.cpp` | Zeichenzustaende aus xywindow.cpp / renderstate.cpp, mit D3D11 statt OpenGL |
| `gui/karte_vektoricons.cpp` | eigene Vektorsymbole an den Stellen von NRCs bitmaps (Entscheidung shank) |

Aus master uebernommene reine Fehlerbehebungen: siehe "Abweichungen und
Gruende" (Statuszeile "x: ", "CSG Intersect").

### Was Stufe 2 braucht

Die 146 Befehle der Stufe 2 sind sichtbar und ausgegraut (Tooltip
"Kommt in Stufe 2 des Karten-Editors"):

- Clipper (X, Clip / Split / Flip, Strg+M1 in 2D) mit Seite "Clipper".
- CSG: Subtract, Merge, Wrap Merge, Intersect, Make Room, CSG Tool;
  AutoCaulk, Make Detail / Structural.
- Pinsel-Grundformen (3- bis 9-seitig, Prisma, Kegel, Kugel, Fels,
  Ikosaeder; Dialog "Arbitrary sides").
- Drehen und Spiegeln per Knopf, "Arbitrary rotation" / "Arbitrary scale",
  Repeat / Reset Transforms, Matrix Transpose.
- Komponenten: Ecken, Kanten, Flaechen ziehen (V / E / F), UV- und
  Build-Werkzeug.
- Patches: das ganze Curve-Menue, die Dialoge Density, Deform, Cap,
  Thicken, Seite "Patches".
- Texturieren: Surface Inspector (S) mit Seite, Fit / Projizieren /
  Schieben / Skalieren / Drehen per Taste, Texture Lock, Find Brush,
  Map Info.
- Filter (Menue und Filterleiste), Regionen, Import, Save Selected /
  Region, Select Inside / Touching / Textured, Layers (Menue, Browser),
  Model Browser.
- Tags im Texturbrowser (mit der ShaderPlug-Tagdatei).
- Abreissbare Menues (die Kante ist schon da), Fullscreen.
- Einstellungen: 12 Seiten mit 48 Feldern (Autosave, Brush, Clipper, Grid,
  Paths, Model Browser, Patches, Surface Inspector, Textures, Undo, Build,
  Orthographic) und die zugehoerigen Wirkungen.

Stufe 3 (ausgegraut): Plugins (bobToolz, brushexport2, meshTex, prtview,
shaderPlug, sunPlug, uFO:AI), der Build-Lauf (q3map2) mit Pointfile und
Leak-Spots, Check for Update, Manual.

### Hinweis zu den NRC-Fotos

Selbst aufgenommen habe ich in Stufe 1b einen DPI-bewussten Abzug des
Hauptfensters (`SP\nrc_fotos\dpi\radiant_dpi_layout_0_0.png`, Regular) -
vor der Bitte, radiant.exe nicht mehr selbst zu starten. Danach habe ich
NRC nicht mehr gestartet; alle Vergleiche danach nutzen die vorhandenen
Fotos in `SP\nrc_fotos\dialoge` und `SP\nrc_fotos\menues`.

## Dialoge und ihre Vorlage-Fotos (SP\nrc_fotos\dialoge)

| unser Dialog | verglichen mit | Stand |
|---|---|---|
| Edit > Preferences, Rahmen und Baum | `dlg_Edit_Preferences_0.png`, `pref_Global.png` | Baum, Groesse 597 x 492, Clean/OK/Cancel; Punkt 11b: alle Lagen nachgemessen (Tahoma, Zeilen 16/22/20, Knoepfe 80); Fotos `karteneditor_pref_<Seite>` je Seite |
| Preferences > Interface | `pref_Interface.png` | Formular: Beschriftung rechtsbuendig, Feld volle Breite; dazu unsere Einstellung "Begriffe wie NetRadiant" |
| Preferences > Layout | `pref_Layout.png` | vier Bilder, Radioknoepfe ohne Text darunter, Detachable Menus (aus), Built-In Group Dialog |
| Preferences > Camera | `pref_Camera.png` | alle 15 Felder in NRCs Reihenfolge und mit NRCs Vorgaben; Spinner als Feld mit -/+ (unsere Knoepfe) |
| Preferences > Selection | `pref_Selection.png` | Selector size (pixels) 12 (2..64), Prefer point entities in 2D (an), Create brushes in 3D (an), Move style in 3D (XY plane + Z with Alt); unser Foto `karteneditor_einstellungen_auswahl` |
| Preferences > Texture Browser | `pref_Texture_Browser.png` | Texture scrollbar, Texture Thumbnail Scale (50%), Thumbnails Max/Min Size (160/48), Mousewheel Increment (64), Load Shaders at Startup (None/System), Hide nonShaders in system/ folder; unser Foto `karteneditor_einstellungen_texturen` |
| Find / Replace Texture(s) | kein Foto, aus findtexturedialog.cpp:130-170 gebaut | Find, Replace (Platzhalter "Empty = search mode"), "Within selected brushes only", senkrecht Apply / Close; unser Foto `karteneditor_texturen_finden` |
| Shader Info | kein Foto, aus gtkdlgs.cpp:445-453 | Text und OK; unser Foto `karteneditor_texturen_shaderinfo` |
| Shader-Text (TextEditor) | kein Foto, aus gtkdlgs.cpp:2263-2510 | 550 x 700, Text, Save, Suchfeld; unser Foto `karteneditor_texturen_shadertext` |
| Preferences, uebrige 17 Seiten | `pref_<Seite>.png` | noch Platzhalter "Stufe 2" (die Felder kommen mit ihren Funktionen); Rahmen, Baum, Titel und Knoepfe wie die Fotos |
| Edit > Shortcuts (Mapped Commands) | `dlg_Edit_Shortcuts_0.png` | 399 x 533; Punkt 11b: Suchfelder, Kopf 20, Zeilen 16, Spalten nach Inhalt, Knoepfe 80 x 22 nachgemessen; Foto `karteneditor_dlg_Edit_Shortcuts_0` |
| View > Entity Inspector (Gruppendialog "Entities") | `dlg_View_Entity_Inspector_0.png` | 444 x 777, Reiter unten, Teiler, Behaelter und Attribute gemessen (Punkt 9); unsere Fotos `karteneditor_entities`, `karteneditor_entities_licht` |
| View > Entity List | `dlg_View_Entity_List_0.png` | 350 x 500, Baum mit Kartenname, Haken (Punkt 11b: Kaestchen 14) und Filterfeld; unsere Fotos `karteneditor_entityliste`, `karteneditor_dlg_View_Entity_List_0` |
| Preferences > Entities | `pref_Entities.png` | Names Display Distance (3D) 512, Names Display Ratio (2D) 64, Entity Names "= Targetnames"; unser Foto `karteneditor_einstellungen_entities` |
| Set Light intensity | kein Foto, aus gtkdlgs.cpp:410-443 gebaut | Tip, Intensity (SpinBox, ziehbare Beschriftung), Checkbox, OK / Cancel; unser Foto `karteneditor_lichtstaerke` |
| Entity > Select Color (Choose Color) | kein Foto, aus Qt 5.15 qcolordialog.cpp (initWidgets) gebaut | Grundfarben 6 x 8, Pick Screen Color, eigene Farben 2 x 8, Add to Custom Colors, Farbfeld 220 x 200, Helligkeit 20, Hue/Sat/Val, Red/Green/Blue, HTML, OK / Cancel; unser Foto `karteneditor_farbe` |
| Entitaetenmenue (2D, Kamera) | kein Foto, aus xywindow.cpp:806-878 | Gruppen bis '_', Strg+Klick, Rechtsklick wandelt; unser Foto `karteneditor_entitaetenmenue` |
| Konsole und ihr Kontextmenue | kein Foto der Konsole allein; Feld im Layout-Foto `radiant_layout_0_0.png`; Menue aus console.cpp:96-104 und Qts createStandardContextMenu | Copy, Select All, Copy All, Clear; unsere Fotos `karteneditor_konsole`, `karteneditor_konsole_menue` |
| Help > About | `dlg_Help_About_0.png` | noch nicht nachgebaut (Menuepunkt ausgegraut) |

Unsere Fotos dazu aus dem Selbsttest: `karteneditor_einstellungen_layout`,
`karteneditor_einstellungen_interface`, `karteneditor_einstellungen_kamera`.
Abweichung: die Spinner-Pfeile von Qt sind bei uns -/+-Knoepfe (Knoepfe im
Stil des Programms), Haken und Klappfelder ebenso.

## Abweichungen und Gruende

- Schwebende Fenster (Floating) schweben im Programmfenster: ImGui 1.92 ohne
  Viewports kann keine eigenen Desktop-Fenster oeffnen.
- Farben aus dem Farbschema des Programms, Knoepfe und Symbole im Stil des
  Programms (Entscheidungen shank); Lage, Reihenfolge, Gruppen, Trenner,
  Schaltzustand und Tooltips wie NRC.
- Schrift: Segoe UI wie NRC (Menues, Statuszeile, Baeume und Listen),
  Tahoma 8 pt in den Dialogen (Beschriftungen, Felder, Knoepfe); der Rest
  des Programms behaelt seine Schrift.
- Erweiterungsknopf einer Leiste: Qt klappt die Leiste aus, wenn Platz ist;
  wir zeigen die verdeckten Knoepfe als Menue (Qts Verhalten ohne Platz).
- Abreissbare Menues (Detachable Menus): die Kante ist da (Punkt 11), das
  Abreissen kommt in Stufe 2.
- Strg+[ ist auf deutscher Tastatur wie in NRC nicht erreichbar (AltGr+8
  ergibt "[", Strg+AltGr+8 bleibt "[").
- Eine neue Fensteranordnung gilt sofort (NRC: nach Neustart).
- Uebernommene Fehlerbehebungen aus master (Entscheidung Koordinator):
  Statuszeile "x: " statt "x:: ", "CSG Intersect" statt "CSG Intesect";
  Suchfelder mit unserem Lupensymbol statt Emoji.

## Changelog

## English

- Map editor: panes, toolbars and the status bar have exactly the sizes and
  positions of NetRadiant-Custom 20260114, including the filter and plugin
  toolbars with their "show more" buttons.
- Map editor: toolbar icons drawn in the program's own modern style.
- Map editor: select, move, rotate, scale, transform and resize like
  NetRadiant-Custom, with its manipulators in the 2D views and the camera,
  selection rectangles, new brushes by dragging (also in 3D), and the
  commands Undo/Redo, Copy/Paste (to camera), Duplicate, Delete, Clear and
  Invert Selection, Select All Of Type, Hide Selected/Show Hidden, Snap To
  Grid and Nudge; preference page "Selection".
- Map editor: texture browser like NetRadiant-Custom - folder tree, filter,
  thumbnails with in-use and shader borders, click to apply, middle click
  to pick, right-drag scrolling, context menu (select textured objects or
  surfaces, recent folders, shader info, open shader), Find / Replace
  Texture(s) and the "Texture Browser" preference page.
- Map editor: middle click copies the texture under the pointer, with Shift,
  Ctrl or Alt it paints it (values, seamless, projected, without shader).
- Map editor: entities like NetRadiant-Custom - the Entity Inspector (class
  list, description, spawnflags, keys and values, typed fields such as
  angle with "<-cam", colour, model and sound), right-click in a 2D view or
  double right-click in the camera for the entity menu (Ctrl+click creates
  several), light intensity and colour dialogs, Entity List (Shift+L),
  Connect Entities (Ctrl+K), Move Primitives to Entity (Ctrl+M), Expand
  Selection To Entities, Select Connected Entities, Reload Entity
  Definitions and the "Entities" preference page.
- Map editor: the console reads like NetRadiant-Custom's - game loading,
  map loading and saving report the same lines, a message and its value
  can have different colours on one line, long lines wrap, text can be
  selected with the mouse and the context menu offers Copy, Select All,
  Copy All and Clear.
- Map editor: menus look and measure like NetRadiant-Custom's (row height,
  separators, tear-off bar, width, icon and check columns, shortcut column),
  mnemonics are underlined and work with Alt and letters, the Build menu
  lists the game's build steps; the Preferences and Mapped Commands dialogs
  use NetRadiant-Custom's fonts and measurements.
- Map editor: large maps stay smooth - changing the selection on the
  largest Movie Duels map (31671 brushes) takes 8 ms instead of 136 ms,
  and the map needs about 280 MB less memory.
- Guide: new chapter "The map editor" in all four languages - opening and
  saving, menus and layouts, 2D views and camera, selecting and building,
  textures, entities, console and shortcuts, with a note for keyboards
  where [ and ] need AltGr.

## Deutsch

- Karten-Editor: Felder, Werkzeugleisten und Statuszeile haben genau die
  Groessen und Lagen von NetRadiant-Custom 20260114, samt Filter- und
  Plugin-Leiste mit ihren Knoepfen fuer weitere Eintraege.
- Karten-Editor: Werkzeugsymbole im modernen Stil des Programms.
- Karten-Editor: Auswaehlen, Verschieben, Drehen, Skalieren, Verzerren und
  Groesse aendern wie in NetRadiant-Custom, mit seinen Manipulatoren in den
  2D-Ansichten und der Kamera, Auswahlrechtecken, neuen Pinseln per Ziehen
  (auch in 3D) und den Befehlen Rueckgaengig/Wiederholen, Kopieren/Einfuegen
  (auch bei der Kamera), Duplizieren, Loeschen, Auswahl aufheben/umkehren,
  Alle dieses Typs, Ausgewaehltes verstecken/Verstecktes zeigen, Am Raster
  ausrichten und Schubsen; Einstellungsseite "Selection".
- Karten-Editor: Texturbrowser wie in NetRadiant-Custom - Ordnerbaum, Filter,
  Vorschaubilder mit Rahmen fuer benutzte Texturen und Shader, Klick setzt
  die Textur, Mittelklick waehlt sie nur, Rollen mit rechts gezogener Maus,
  Kontextmenue (Objekte oder Flaechen mit der Textur auswaehlen, zuletzt
  gezeigte Ordner, Shader-Info, Shader ansehen), Textur(en) suchen /
  ersetzen und die Einstellungsseite "Texture Browser".
- Karten-Editor: Mittelklick kopiert die Textur unter dem Zeiger, mit
  Umschalt, Strg oder Alt traegt er sie auf (Werte, nahtlos, projiziert,
  ohne Shader).
- Karten-Editor: Entities wie in NetRadiant-Custom - der Entity Inspector
  (Klassenliste, Beschreibung, Spawnflags, Schluessel und Werte, Felder je
  Art wie Winkel mit "<-cam", Farbe, Modell und Sound), Rechtsklick in
  einer 2D-Ansicht oder doppelter Rechtsklick in der Kamera fuer das
  Entity-Menue (Strg+Klick erzeugt mehrere), Dialoge fuer Lichtstaerke und
  Farbe, Entity-Liste (Umschalt+L), Entities verbinden (Strg+K), Primitive
  in die Entity verschieben (Strg+M), Auswahl auf Entities erweitern,
  Verbundene Entities waehlen, Entity-Definitionen neu laden und die
  Einstellungsseite "Entities".
- Karten-Editor: die Konsole liest sich wie bei NetRadiant-Custom - Laden
  des Spiels, Laden und Speichern der Karte melden dieselben Zeilen, eine
  Meldung und ihr Wert koennen auf einer Zeile verschiedene Farben haben,
  lange Zeilen brechen um, Text laesst sich mit der Maus auswaehlen und das
  Kontextmenue bietet Kopieren, Alles auswaehlen, Alles kopieren und Leeren.
- Karten-Editor: die Menues sehen aus und messen wie bei NetRadiant-Custom
  (Zeilenhoehe, Trenner, Abreisskante, Breite, Symbol- und Hakenspalte,
  Tastenspalte), Mnemoniken sind unterstrichen und gehen mit Alt und
  Buchstaben, das Build-Menue zeigt die Build-Schritte des Spiels; die
  Dialoge Preferences und Mapped Commands haben NetRadiant-Customs Schriften
  und Masse.
- Karten-Editor: grosse Karten bleiben fluessig - ein Auswahlwechsel auf der
  groessten Movie-Duels-Karte (31671 Pinsel) dauert 8 ms statt 136 ms, und
  die Karte braucht rund 280 MB weniger Speicher.
- Handbuch: neues Kapitel "Der Karten-Editor" in allen vier Sprachen -
  Oeffnen und Speichern, Menues und Anordnungen, 2D-Ansichten und Kamera,
  Waehlen und Bauen, Texturen, Entities, Konsole und Tastenkuerzel, mit
  einem Hinweis fuer Tastaturen, auf denen [ und ] AltGr brauchen.

## 中文

- 地图编辑器:窗格、工具栏和状态栏的大小与位置与 NetRadiant-Custom 20260114 完全一致,包括过滤工具栏和插件工具栏及其"显示更多"按钮。
- 地图编辑器:工具栏图标采用本程序自己的现代风格。
- 地图编辑器:像 NetRadiant-Custom 一样选择、移动、旋转、缩放、变换和调整大小,2D 视图和摄像机中都有它的操纵器、选择框、拖动创建新画刷(也可在 3D 中),以及撤销/重做、复制/粘贴(到摄像机)、复制副本、删除、取消/反向选择、选择同类、隐藏选中/显示隐藏、对齐网格和微移命令;首选项页"Selection"。
- 地图编辑器:像 NetRadiant-Custom 一样的纹理浏览器 - 文件夹树、过滤、带有使用中和着色器边框的缩略图、单击应用纹理、中键仅选取、右键拖动滚动、右键菜单(选择使用该纹理的对象或表面、最近的文件夹、着色器信息、打开着色器)、查找 / 替换纹理以及"Texture Browser"首选项页。
- 地图编辑器:中键复制指针下的纹理,按住 Shift、Ctrl 或 Alt 则涂上纹理(数值、无缝、投影、不含着色器)。
- 地图编辑器:像 NetRadiant-Custom 一样处理实体 - 实体检查器(类列表、说明、生成标志、键和值、按类型的字段,如带"<-cam"的角度、颜色、模型和声音),在 2D 视图中右键或在摄像机中双击右键打开实体菜单(Ctrl+单击可连续创建),光照强度和颜色对话框,实体列表(Shift+L),连接实体(Ctrl+K),将图元移入实体(Ctrl+M),选择扩展到实体,选择相连实体,重新加载实体定义以及"Entities"首选项页。
- 地图编辑器:控制台与 NetRadiant-Custom 一致 - 加载游戏、加载和保存地图时输出相同的行,同一行中的消息和数值可以有不同颜色,长行自动换行,可用鼠标选择文本,右键菜单提供复制、全选、全部复制和清空。
- 地图编辑器:菜单的外观和尺寸与 NetRadiant-Custom 一致(行高、分隔线、可撕下边、宽度、图标和勾选列、快捷键列),助记符带下划线并可用 Alt 和字母操作,Build 菜单列出游戏的构建步骤;Preferences 和 Mapped Commands 对话框使用 NetRadiant-Custom 的字体和尺寸。
- 地图编辑器:大型地图保持流畅——在最大的 Movie Duels 地图(31671 个笔刷)上切换选择只需 8 毫秒而不是 136 毫秒,并且该地图少用约 280 MB 内存。
- 手册:新增四种语言的"地图编辑器"一章——打开和保存、菜单和布局、二维视图和摄像机、选择和建造、纹理、实体、控制台和快捷键,并附有针对 [ 和 ] 需要 AltGr 的键盘的说明。

## 日本語

- マップエディター:ペイン、ツールバー、ステータスバーのサイズと位置が NetRadiant-Custom 20260114 と完全に一致します(フィルターツールバーとプラグインツールバー、「さらに表示」ボタンを含む)。
- マップエディター:ツールバーのアイコンをプログラム独自のモダンなスタイルで描画しました。
- マップエディター:NetRadiant-Custom と同じように選択・移動・回転・拡大縮小・変形・サイズ変更ができます。2D ビューとカメラにマニピュレーター、選択矩形、ドラッグによる新規ブラシ(3D でも)、元に戻す/やり直し、コピー/貼り付け(カメラ位置へも)、複製、削除、選択解除/選択反転、同じ種類をすべて選択、選択を隠す/隠したものを表示、グリッドに合わせる、ナッジの各コマンドがあります。設定ページ「Selection」。
- マップエディター:NetRadiant-Custom と同じテクスチャブラウザー - フォルダーツリー、フィルター、使用中とシェーダーの枠付きサムネイル、クリックで適用、中クリックで選択のみ、右ドラッグでスクロール、コンテキストメニュー(このテクスチャのオブジェクトや面を選択、最近のフォルダー、シェーダー情報、シェーダーを開く)、テクスチャの検索 / 置換、設定ページ「Texture Browser」。
- マップエディター:中クリックでポインター下のテクスチャをコピー、Shift・Ctrl・Alt を押しながらで貼り付け(値、シームレス、投影、シェーダーなし)。
- マップエディター:NetRadiant-Custom と同じエンティティ操作 - エンティティインスペクター(クラス一覧、説明、スポーンフラグ、キーと値、「<-cam」付きの角度・色・モデル・サウンドなど種類ごとの入力欄)、2D ビューの右クリックまたはカメラのダブル右クリックでエンティティメニュー(Ctrl+クリックで連続作成)、ライトの強さと色のダイアログ、エンティティ一覧(Shift+L)、エンティティの接続(Ctrl+K)、プリミティブをエンティティへ移動(Ctrl+M)、選択をエンティティへ拡張、接続されたエンティティを選択、エンティティ定義の再読み込み、設定ページ「Entities」。
- マップエディター:コンソールが NetRadiant-Custom と同じ内容に - ゲームの読み込み、マップの読み込みと保存で同じ行を出力し、1 行の中でメッセージと値を別の色で表示、長い行は折り返し、マウスでテキストを選択でき、右クリックメニューにコピー・すべて選択・すべてコピー・クリアがあります。
- マップエディター:メニューの見た目と寸法が NetRadiant-Custom と同じに(行の高さ、区切り線、切り離し線、幅、アイコンとチェックの列、ショートカットの列)、ニーモニックに下線が付き Alt と文字キーで操作でき、Build メニューにゲームのビルド手順が並びます。Preferences と Mapped Commands のダイアログは NetRadiant-Custom のフォントと寸法を使います。
- マップエディター:大きなマップでも滑らかに。最大の Movie Duels マップ(31671 ブラシ)で選択の切り替えが 136 ms から 8 ms になり、メモリも約 280 MB 少なくなりました。
- ガイド:4 言語で新しい章「マップエディター」- 開く・保存、メニューとレイアウト、2D ビューとカメラ、選択と作成、テクスチャ、エンティティ、コンソール、ショートカット。[ と ] に AltGr が必要なキーボード向けの注意付き。
