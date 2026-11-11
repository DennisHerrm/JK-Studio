# Abgleich Navigation in den Ansichten des Model-Tabs: 3ds Max bzw. Blender gegen JK Studio (MBNAV, 11.11.2026)

Auftrag `auftraege/auftrag_mbnav_1011.md`. Jede Zeile: die Bedienung des Vorbilds | was das Vorbild genau tut | der Model-Tab bis rc611
(am laufenden Programm belegt: Selbsttest `modellblock`, `BHED_MODELL_NUR=nav`, Rot-Lauf auf dem alten Stand) | der Model-Tab jetzt | Prüfung.
Zahlen und Rot/Grün des Laufs stehen in `NOTIZEN_mbnav_1011.md`.

**Quellen** (nur gelesen, kein Programm gestartet):
- 3ds Max: Autodesk-Hilfe, Hotkey-Tabelle 2026 (`help.autodesk.com/cloudhelp/2026/ENU/3DSMax-Basics/files/GUID-A73E1B09-7BFE-4A22-8153-1D3D2237B8E9.htm`),
  „Zooming, Panning, and Orbiting Views“ (GUID-D65DE5FD-F859-4F66-9E14-F9A5C1016411), „Orbit, Orbit Selected, Orbit SubObject, Orbit Point of
  Interest“ (GUID-6DA741AB-9941-4957-903A-4B998C1C65E7), „Pan View“ (GUID-802A4CCA-1695-4D91-BC30-9A4C0DAEEE5F), Customize > Mouse
  (GUID-75E03BAB-0D94-43E0-B281-3AE3299BD2B9); die mit 3ds Max 2027 installierte Tastenkarte (`belege/max_hotkeys_de.txt`); shanks
  `AutoCam.ini` (ViewCube) und `3dsMax.ini`. Die Tastendatei `UI_ln\CUI\3dsmaxShortcuts.hsx` von 3ds Max 2027 ist leer (Vorgaben einkompiliert).
- ViewCube: `C:\3dsmaxproject\WhiteoutDex\src\native_sources\WhiteoutDexRenderer\max_viewcube.h` (verbindliche Vorlage).
- Blender 5.0: Quelltext (Zweig `blender-v5.0-release`): `source/blender/editors/space_view3d/view3d_navigate*.cc`,
  `view3d_gizmo_navigate*.cc`, `scripts/presets/keyconfig/keymap_data/blender_default.py` (km_view3d), `release/datafiles/userdef/userdef_default.c`,
  `scripts/startup/bl_ui/space_view3d.py` (VIEW3D_MT_view_pie).

Spalte „bis rc611“: **gleich**, **abweichend** oder **fehlt**. Spalte „jetzt“: **gleich**, **gleich (ohne Zahl)** - das Vorbild nennt keine Zahl, Richtung
und Drehpunkt stimmen -, **bewusst anders** oder **offen**.

Jeder Befehl ist an vier Stellen zu finden: mit seiner Taste, im Menü der Kopfzeile ([Name der Ansicht] > Navigation, im Blender-Modus View >
Navigation), im Rechtsklick-Menü der Ansicht (Navigation) und in der Suche (X bzw. F3); die Zeile über den Ansichten nennt die Maus-Navigation zuerst.

## Wie 3ds Max

| Nr | Bedienung | 3ds Max | bis rc611 | jetzt | Prüfung (nav) |
|---|---|---|---|---|---|
| M1 | Mitteltaste ziehen | Pan View: die Ansicht parallel zur Bildebene schieben, mit jedem Werkzeug | Ortho gleich; Perspektive abweichend: Abstand × 0,0015 je Bildpunkt, die Szene folgt der Maus nicht genau | gleich: der Punkt in der Tiefe des Drehpunkts folgt der Maus 1:1 | M1 |
| M2 | Umschalt+Mitteltaste | Schieben nur entlang der zuerst bewegten Achse | fehlt | gleich | M2 |
| M3 | Strg+Mitteltaste | schnelleres Schieben („To accelerate panning, hold down Ctrl“); der Faktor ist nicht dokumentiert | fehlt | offen (schiebt normal) - Frage | - |
| M4 | Alt+Mitteltaste | Orbit im Modus des Orbit-Flyouts, die Szene folgt der Maus (Trackball); eine Achsenansicht wird „Orthographic“ | abweichend: senkrecht gespiegelt (runter ziehen senkte die Kamera); in Achsenansichten geschah nichts | gleich (ohne Zahl: 0,4 Grad je Bildpunkt wie Blender; vor dem Pol bleibt die Ansicht wie bisher stehen) | M4 |
| M5 | Umschalt+Alt+Mitteltaste | „Holding Shift constrains rotation to the first axis you use“ | fehlt | gleich | M5 |
| M6 | Strg+Alt+Mitteltaste | Zoom ziehen (hoch = näher) | gleich (ohne Zahl) | gleich (ohne Zahl), jetzt um den Punkt unter der Maus beim Drücken | M6 |
| M7 | Mausrad | Zoom in Schritten; Zoom About Mouse Point für Ortho und Perspektive ab Werk an; Wheel Zoom Increment 1,0 (keine absolute Zahl) | abweichend: immer um die Bildmitte | gleich (Schritt wie bisher 0,9 / 0,85 je Raste), zwei Schalter wie in Max | M7 |
| M8 | `[` `]`, Strg+= Strg+- | Zoom Viewport In / Out | fehlt | gleich (ein Schritt wie eine Raste) | M8 |
| M9 | Z | Zoom Extents (All) Selected: alle Ansichten auf die Auswahl, ohne Auswahl auf alles | abweichend: nur die aktive Ansicht | gleich | M9 |
| M9 | Strg+Alt+Z | Zoom Extents: die aktive Ansicht auf alles | fehlt | gleich | M9 |
| M9 | Strg+Umschalt+Z | Zoom Extents All: alle Ansichten auf alles | fehlt | bewusst anders: die Taste ist im Model-Tab Wiederholen (Kollision); der Befehl steht im Menü und in der Suche | M9 |
| M25 | Strg+Alt+Umschalt+Z / Alt+Umschalt+Z | Zoom In 2X / Zoom Out 2X | fehlt | gleich | M25 |
| M10 | Alt+Z | Zoom Mode: mit links ziehen, Rechtsklick beendet | fehlt | gleich (auch Esc) | M10 |
| M11 | Strg+W | Zoom Region Mode: das aufgezogene Rechteck füllt die Ansicht | fehlt | gleich | M11 |
| M12 | Strg+P | Pan View (Modus) | fehlt | gleich | M12 |
| M13 | Strg+R | Orbit View Mode: gelber Kreis mit vier Griffen - innen frei, auf einem Griff eine Achse, außen rollen | fehlt | gleich (ohne Zahl: Kreis 0,38 der kleineren Seite) | M13 |
| M14 | I | Pan Viewport: der Punkt unter dem Mauszeiger kommt in die Bildmitte | fehlt | gleich | M14 |
| M15 | Orbit-Drehpunkt | Flyout: Orbit (Ansichtsmitte), Orbit Selected, Orbit SubObject (Auswahl; ohne Auswahl Ansichtsmitte), Orbit Point of Interest (Punkt unter der Maus) | abweichend: immer die Ansichtsmitte | gleich: vier Arten unter [Ansicht] > Navigation > Kreisen um; Vorgabe Orbit SubObject (Max-Vorgabe nicht belegbar - Frage) | M15 |
| M16 | Umschalt+Z / Umschalt+Y | Undo / Redo Viewport Operation, je Ansicht | fehlt | gleich (64 Schritte je Ansicht) | M16 |
| M17 | Alt+W | Maximize Viewport Toggle | gleich | gleich | (kopf, funktionen) |
| M18 | T B F L P U | Top, Bottom, Front, Left, Perspective, Orthographic | gleich | gleich | M18 |
| M18 | C | Camera View | fehlt | offen: der Model-Tab hat keine Szenenkamera | - |
| M19 | V | Viewports-Quad am Mauszeiger | fehlt | gleich als Menü am Mauszeiger (alle acht Ansichten) | M19 |
| M20 | Sichtfeld | Field-of-View-Knopf der Perspektive | Regler unter [+] | bewusst anders (Regler statt Ziehen) | (kopf) |
| M21 | Alt+Pos1, Alt+Strg+H | Home View / ViewCube Home | fehlt (nur Knopf und Menü) | gleich | M21 |
| M21 | Alt+Strg+V | ViewCube ein/aus | fehlt (nur Menü) | gleich | M21 |
| M22 | Umschalt+W, Pfeil hoch | SteeringWheels; WalkThrough View Mode (W A S D E C, Q Z, `[` `]`, Leertaste) | fehlt | offen - Frage | - |
| M23 | Strg+Umschalt+D, Strg+Alt+X, `` ` ``, Ziffernblock (virtuelles Ansichtsfenster) | Disable Viewport, Expert Mode, Redraw, Virtual Viewport | fehlt | nicht nachgebaut: im Model-Tab ohne Sinn | - |
| M24 | Pfeil- und Bildtasten | 3ds Max hat keine Navigation darauf (Pfeil hoch startet den Durchgangsmodus) | - | - | - |

### ViewCube (gegen `max_viewcube.h`)

| Bedienung | Vorlage | bis rc611 | jetzt | Prüfung |
|---|---|---|---|---|
| 26 Klickziele (6 Flächen, 12 Kanten, 8 Ecken) | Klick dreht in 0,28 s dorthin, die Projektion bleibt | gleich (einzelne Ziele geprüft) | gleich, alle 26 geprüft | nav VC, kopf |
| Ziehen am Würfel | `camera.Rotate`: kreist | abweichend: senkrecht gespiegelt wie Alt+Mitteltaste | gleich: wie Alt+Mitteltaste, in einer Achsenansicht „Orthographisch“ | nav VC, kopf |
| Kompassring ziehen, N/O/S/W klicken | nur Gier; Buchstabe: von dort schauen | gleich | gleich | kopf |
| Home (Haus, Rechtsklick-Menü) | fliegt in 0,5 s | gleich | gleich, dazu Alt+Pos1 / Alt+Strg+H | kopf, nav M21 |
| Pfeile zu den Nachbarflächen, Rollpfeile | bei gerader Sicht auf eine Fläche | gleich | gleich | kopf |
| Rechtsklick-Menü | Home, Set Current View as Home, Orthographic | gleich | gleich | kopf |
| Durchsichtigkeit | 0,55 ohne Maus, 1,0 mit | gleich | gleich | kopf |
| in Achsenansichten | Fläche = Achsenansicht, Kante/Ecke = Orthographisch | gleich | gleich | kopf |
| Abweichungen von der Vorlage | siehe `NOTIZEN_mbkopf_1011.md` (je Ansicht ein Würfel, genau senkrecht, Tooltips ...) | - | unverändert | - |

## Wie Blender

| Nr | Bedienung | Blender 5.0 | bis rc611 | jetzt | Prüfung (nav) |
|---|---|---|---|---|---|
| B1 | Mitteltaste ziehen | view3d.rotate, Turntable: 0,4 Grad je Bildpunkt (/ UI-Maßstab) um die Welt-Hochachse und die Ansichts-X-Achse, über den Pol, Auto Perspective nur aus Achsenansichten | abweichend: die Mitteltaste schob (Max-Belegung in beiden Modi) | gleich | B1 |
| B2 | Umschalt+Mitteltaste | view3d.move: 1:1 in der Tiefe des Drehpunkts | Ortho gleich; Perspektive abweichend (nicht 1:1) | gleich | B2 |
| B3 | Strg+Mitteltaste | view3d.zoom, Dolly: Abstand = Anfang × (2 × (len_neu / len_alt − 1) + 1), len = Abstand der Maus zur Oberkante | abweichend: schob | gleich | B3 |
| B4 | Umschalt+Strg+Mitteltaste | view3d.dolly | fehlt | offen - Frage | - |
| B5 | Alt+Mitteltaste ziehen | view_axis relativ: Vierteldrehung auf die Nachbar-Achsenansicht (O, W, N, S) | abweichend: kreiste (Max) bzw. nichts | gleich | B5 |
| B6 | Alt+Mitteltaste klicken | view_center_pick: der Punkt unter der Maus in die Bildmitte | fehlt | gleich | B6 |
| B7 | Alt während des Kreisens | Achsen-Einrasten: 15 Grad um die 26 Richtungen | fehlt | gleich | B7 |
| B8 | Mausrad | Abstand ÷ 1,2 bzw. × 1,2, nicht zur Maus | abweichend: 0,9 / 0,85 | gleich | B8 |
| B9 | Ziffernblock + −, Strg+= Strg+− | wie eine Raste | fehlt | gleich | B9 |
| B10 | Ziffernblock 1 3 7 (Strg: Gegenseite) | Achsenansicht orthographisch, weich in 200 ms, Mitte und Größe bleiben | abweichend: sprang ohne Übergang | gleich | B10 |
| B11 | Ziffernblock 5 | view_persportho: Perspektive/orthographisch, die Richtung bleibt | abweichend: sprang auf die nächste Achsenansicht | gleich | B11 |
| B12 | Ziffernblock 4 6 8 2 | view_orbit 15 Grad: 4 = Kamera nach links, 8 = Kamera hoch; weich; über den Pol; Auto Perspective nur aus Achsenansichten | abweichend: 4/6 gespiegelt, Kappen bei 89 Grad, auch die freie Orthoansicht wurde Perspektive | gleich | B12 |
| B13 | Strg+Ziffernblock 4 6 8 2 | view_pan: 32 Bildpunkte waagerecht, 25 senkrecht | fehlt | gleich | B13 |
| B14 | Umschalt+Ziffernblock 4 6 | view_roll 15 Grad: links = die Szene gegen den Uhrzeigersinn | abweichend: gespiegelt | gleich | B14 |
| B15 | Ziffernblock 9 | Gegenseite | gleich | gleich | (kopf) |
| B16 | Ziffernblock , | view_selected: ohne Auswahl nichts; Kastenmitte in die Bildmitte, größte Seite × 0,7 = halbe kleinere Bildseite | abweichend: ohne Auswahl alles, eigener Rand | gleich | B16 |
| B17 | Pos1 | view_all: dieselbe Rechnung für alles | abweichend: eigener Rand | gleich | B17 |
| B18 | Umschalt+C | view_all mit dem Ursprung im Kasten | fehlt | gleich | B17 |
| B19 | `` ` `` | Kuchenmenü View: Left, Right, Bottom, Top, Front, Back, View Camera, View Selected | fehlt | gleich; View Camera ausgegraut (keine Szenenkamera) | B19 |
| B20 | Strg+Alt+Q | Quad View: oben links Top, oben rechts die Perspektive, unten links Front, unten rechts Right | abweichend: Anordnung von 3ds Max | gleich | B20 |
| B21 | Umschalt+`` ` `` | view3d.navigate: Gehen/Fliegen | fehlt | offen - Frage | - |
| B22 | Vorgaben | Turntable; Orbit Around Selection aus; Zoom to Mouse Position aus; Auto Perspective an; Smooth View 200 ms; Rotation Angle 15 | teils | gleich; Orbit Around Selection und Zoom to Mouse Position als Schalter unter View > Navigation; Trackball offen | B22 |
| B23 | Gizmo | Kugel = Achsenansicht (genau entlang einer Achse gibt die Mitte die Gegenseite), Ziehen = rotate; Knöpfe Zoom (Dolly aus der Bildmitte), Move (1:1), Persp/Ortho (Richtung bleibt), Kamera | abweichend: Zoom/Move mit eigenen Raten, Persp/Ortho sprang, Mitte nur in Achsenansichten | gleich; Kamera-Knopf fehlt (keine Szenenkamera) | B23, kopf |
| B24 | Unten (Strg+Ziffernblock 7) | von unten: +X rechts, −Y oben | abweichend: wie 3ds Max (−X rechts, +Y oben) | gleich | B5 |
| B25 | Umschalt+B | view3d.zoom_border: ein Rechteck, dann zu Ende | fehlt | gleich | B25 |
| B26 | Ziffernblock 0, Strg+Ziffernblock ,, Ziffernblock / | Kamera, alle Bereiche, Local View | fehlt | nicht nachgebaut | - |
