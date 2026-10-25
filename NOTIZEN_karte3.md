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

## Punkt 1: q3map2 im eigenen Bau

`bhed_q3map2.exe` = q3map2 aus NRC 20260114 (Commit `68ecbed`), mit MSVC 18
(VS 2026) als eigenes Programm gebaut: Ziel `bhed_q3map2`,
`src/q3map2/CMakeLists.txt`, im Wurzel-CMake mit `BEHAVED_Q3MAP2` (Vorgabe an
bei MSVC). Statische Laufzeit wie behaved.exe; feste Importe nur
`KERNEL32`, `WS2_32`, `bcrypt` (ctest `q3map2_importe`).

### Was uebernommen wurde (unveraendert, `.gitattributes` `-text`)

| Ordner | Quelle | Fassung | Lizenz |
|---|---|---|---|
| `third_party/q3map2/tools/quake3/q3map2`, `.../common` | NRC-Tag 20260114 (`SP\s3\tag`) | 2.5.17n-git-68ecbed | GPL v2+ |
| `third_party/q3map2/include`, `.../libs` (Auswahl, 63 Dateien) | dito, nur was q3map2 einbindet (`SP\s3\werk\libs_noetig.txt`) | | GPL v2+, crunch zlib, ddslib BSD-3, tiny_webp BSD-3, rapidjson MIT |
| `third_party/zlib` | zlib.net | 1.3.1 | zlib |
| `third_party/libpng` | libpng.org | 1.6.53 | PNG v2 |
| `third_party/libjpeg-turbo` (ohne `simd/`, Testbilder, Doku) | GitHub-Release | 3.1.3 | IJG, BSD-3, zlib |
| `third_party/libxml2` | gitlab.gnome.org | 2.15.1 | MIT |
| `third_party/assimp` (ohne test/samples/doc/tools/port, ohne eigenes zlib) | GitHub-Release | 6.0.2 | BSD-3, contrib MIT/BSD/Boost/ISC/zlib/Unlicense |

Dieselben Fassungen wie die DLLs neben NRCs `q3map2.exe`. **Nicht uebernommen:**
`libs/etclib.c/.h` (Apache-2.0, mit GPL v2 unvertraeglich) - eigene Fassung
`src/q3map2/kompat/etclib.c` nach der Khronos-Beschreibung, auf 4 Mio.
Zufallsbloecken gleich der NRC-Fassung. Herkunft und alle Lizenzen:
`third_party/q3map2/README.md`; README nennt die Bibliotheken mit Fassung.

### Anpassungen an MSVC (alle ausserhalb von third_party)

| Nr. | Was | Wo | Warum |
|---|---|---|---|
| 1 | `g_locale_to_utf8`, `g_free`, `GDir` (FindFirstFileW) | `kompat/glib.h` | inout.cpp/vfs.cpp brauchen drei GLib-Aufrufe; ganze GLib (meson, gnulib, pcre2) waere ein Brocken. Windows-GLib nimmt dafuer auch CP_ACP. |
| 2 | `R_OK`/`F_OK`/`W_OK`, `<io.h>` | `kompat/msvc_vorab.h` (/FI) | cmdlib.cpp:172 `access( path, R_OK )`; MinGW bringt R_OK mit, MSVC nicht |
| 3 | `NOMINMAX` | CMake | windows.h-Makros min/max gegen `std::min/max` in inout.cpp |
| 4 | `ASSIMP_INCLUDE(x)` | `kompat/msvc_vorab.h` | Makefile setzt `-DASSIMP_INCLUDE(x)=<x>`; ueber MSBuild nicht als Befehlszeilen-Makro moeglich |
| 5 | `Span<T>( behaelter )` statt `Span( behaelter )` an 2 Stellen | Kopien surface.cpp, model.cpp | MSVC leitet Alias-Template-Argumente (CTAD) aus dem eingeschraenkten Leitfaden von std::span nicht ab (C2641) |
| 6 | `LittleLong` in crnlib.cpp umbenannt | CMake (Quell-Define) | GCC: inline-Fassung schwaches Symbol; MSVC LNK2005 gegen cmdlib.cpp |
| 7 | `std::sort`/`std::ranges::sort` wie libstdc++ | `kompat/gcc_sort.h`, alle 9 Kopien | Punkt 2 |
| 8 | `entities.emplace_back()` verschiebt beim Wachsen | `kompat/gcc_vektor.h`, Kopie map.cpp | Punkt 2 (Fehler, nicht nur Reihenfolge) |

Sonst wie NRCs Makefile: Stapel 4 MB (`/STACK:4194304`), `/fp:precise`
(GCC -O3 ohne -ffast-math: SSE2, keine FMA), dieselben Kennungen
`Q3MAP_VERSION` usw. (stehen in jeder .bsp). libxml2 nur mit Threads/Output,
assimp ohne Export/Tests/Werkzeuge, beide gegen unser zlib. Die Kopien
schreibt `bhed_q3_kopie` beim Konfigurieren nach `build/src/q3map2/angepasst`
und bricht ab, wenn eine zu ersetzende Stelle fehlt.

### Release

- Zip: `BehavEd-Remake/bhed_q3map2.exe` neben behaved.exe (vor ihr, behaved.exe
  bleibt letzter Eintrag) und `BehavEd-Remake/licenses/` mit 21 Texten (GPL v2,
  README von third_party/q3map2, die Lizenzdateien, und aus Quelltextkoepfen
  die Bloecke von crunch, ddslib, tiny_webp, rapidjson, Open3DGC, stb).
  `tools/release_paket.py zip ... --q3map2 <exe>`, `pruefe ... --q3map2`;
  `.github/workflows/release.yml` ruft beides.
- **Abweichung:** die eigenstaendige .exe enthaelt q3map2 nicht. Eine .exe, die
  eine zweite .exe in sich traegt und beim Start auspackt, ist das Muster eines
  "Droppers" - bei einem unsignierten Programm ein sicherer Weg zu
  Virenscanner-Fehlalarmen. Das Build-Menue sucht `bhed_q3map2.exe` neben
  behaved.exe; die Release-Notizen sagen, dass der Kartenbauer nur im Zip ist.

### Rot/Gruen

| Pruefung | rot | gruen |
|---|---|---|
| `karte_lizenz` Abschnitt 4 (README Fassungen, Lizenzdateien, etclib nicht dabei, .gitattributes) | 5 FEHL (README nannte die Bibliotheken nicht) | 0 |
| `release_paket selbsttest` (neu: Zip mit q3map2, Block aus Quelltextkopf, 4 kaputte Pakete) | - | 47/47 |
| echtes Zip `pruefe --fassung rc604 --q3map2` | - | ok (2672 Eintraege, 21 Lizenztexte) |
| `q3map2_importe` | bcrypt.dll "unbekannt" | ok (bcrypt.dll erlaubt: libxml2 dict.c BCryptGenRandom, seit Vista Teil von Windows) |
| erster Vergleich ein_pinsel.map (-bsp) | - | alle 18 Lumps Byte-gleich mit NRC, Ausgabe gleich bis auf Pfade/Baudatum |

## Punkt 2: Gleiche Ergebnisse wie das Original

Verglichen mit `SP\nrc_dl\nrc\q3map2.exe` (NRC 20260114, MinGW-GCC). Schalter
wie das Build-Menue: JA/MD "[Final] Fast" (`-meta`; `-vis`; `-light -fast
-patchshadows -samples 2 -filter -bounce 8`), JK2 (`-meta`; `-vis -saveprt`;
`-light -fast -filter -super 2`); MD mit `-fs_game MD`. Alle Laeufe mit
Idle-Prioritaet in `Y:\karte\laeufe\q3vgl`, Werkzeuge in `SP\s3\werk`
(`alle.py`, `kanon.py`, `lichtvergleich.py`, `*serie.py`).

### Ergebnis in einem Satz

Byte-gleich ist es, wo NRC selbst bei jedem Lauf dasselbe schreibt; wo NRC
von Lauf zu Lauf andere Bytes schreibt, sind unsere Unterschiede genau
dieselben, und alles, was davon nicht abhaengt, ist gleich (jedes Dreieck mit
allen Eckdaten, jede Ebene, jeder Knoten, Sichtbarkeit, Lichtgitter,
Eckfarben). Dabei kamen zwei echte Unterschiede heraus, beide behoben (Nr. 7, 8).

### Was gefunden wurde

1. **Muell in `leafbrushes` (behoben, Nr. 8).** Jeder Eintrag zeigte ins Leere
   (z. B. 582359808 statt 136..317), und `-light` stuerzte auf mp__duel1 ab
   (0xC0000005). Ursache: `std::vector<entity_t> entities` waechst beim Laden;
   MSVCs `std::list` ist nicht noexcept verschiebbar
   (`is_nothrow_move_constructible_v<std::list<int>>` = 0, bei libstdc++ 1),
   also kopiert der Vektor die Entities samt Pinsellisten, und
   `brush_t::original = &b` (map.cpp:828) zeigt in freigegebenen Speicher;
   writebsp.cpp:133 schreibt `b.original->outputNum`. Abhilfe ohne die Quelle
   zu aendern: in der Kopie von map.cpp `bhed_gcc::emplace_back( entities )`,
   das vorher selbst mit `std::move` vergroessert (`kompat/gcc_vektor.h`).
   Danach `leafbrushes` Byte-gleich, `-light` laeuft durch.
2. **Reihenfolge gleichwertiger Elemente (behoben, Nr. 7).** `std::sort` ist
   nicht stabil; MSVC teilt anders als libstdc++ (Grenze 32 statt 16, Median
   aus 9). q3map2 sortiert u. a. Lightmaps, Kanten, Portale mit Gleichstaenden.
   `kompat/gcc_sort.h` ist das Introsort von libstdc++ (SGI-STL-Verfahren).
   Wirkung: raum_einfach.map nach `-light` - ohne Nr. 7 andere `lightmaps` und
   `drawverts`, mit Nr. 7 Byte-gleich.
3. **NRC ist selbst nicht deterministisch.** 12 Laeufe `-bsp` auf mp__duel1:
   12 verschiedene .bsp - bei NRC wie bei uns, auch mit `-threads 1`.
   Ursachen, je mit Beleg:
   - surface_meta.cpp:144 `CompareMetaTriangles`: `a.si < b.si` vergleicht
     Zeiger auf `shaderInfo_t`; die Shader liegen in einem `std::multiset`
     (`UnsortedSet`, q3map2.h:1930), ihre Adressen kommen vom Heap. Versuch:
     Vergleich ueber den Shadernamen statt des Zeigers -> drei Laeufe
     hintereinander gleich bis auf `drawverts` (danach zurueckgebaut). Betroffen:
     `surfaces`, `drawverts`, `drawindexes`, `leafsurfaces`,
     `brushsides.surfaceNum`, und welche Dreiecke zu einer Flaeche
     zusammengefasst werden.
   - die Lightmap-Koordinaten der Ecken sind nach `-bsp` nicht belegt
     (Speicherreste, z. B. 7.19e-43 oder 1.37e+32) - auch bei NRC.
   - lightmaps_ydnar.cpp:837 `CompareSurfaceInfo`: `aInfo.si < bInfo.si` beim
     Packen der Lightmaps; die Reihenfolge entscheidet auch, welche
     "identischen" Lightmaps gemittelt werden (`MergeBSPLuxels`, :1395).
     12 Laeufe `-light` auf derselben Eingabe: "identical surface lightmaps,
     using N luxels" zwischen 10818 und 463809.
   - `-vis` und `-light` mit mehreren Threads: oa_fan `-vis` bei NRC in 6
     Laeufen 2 verschiedene Ergebnisse, bei uns 3; mit `-threads 1` beide
     immer dasselbe und Byte-gleich. Lichtgitter mit Threads schwankend, mit
     einem Thread gleich.
   Ein Ergebnis "wie NRC" gibt es damit nur bis auf diese Reihenfolge. Jedes
   Programm hat dabei sein eigenes Heap-Muster: auf raum_leck.map waren NRC
   und wir in 6 Laeufen je stabil, aber verschieden; auf mp__duel1 fasste
   unser `-light` in 4 von 12 Laeufen sieben kleine Flaechen zu einer
   einfarbigen Lightmap zusammen, NRC in 0 von 12 - bei uns von Lauf zu Lauf
   verschieden, also Reihenfolge, nicht Rechnung.
4. **Fliesskomma ist gleich.** Mit `/fp:precise` sind alle Dreiecke aller
   Karten gleich (unten). Gegenprobe `-DBHED_Q3MAP2_FP=/fp:fast`: 43 von 45,
   413 von 446, 1115 von 1123 Dreiecken gleich, Eckfarben anders - der
   Vergleich sieht Rechenunterschiede also. `/fp:strict`: wie precise. Es
   bleibt bei `/fp:precise`; `/arch` bleibt bei SSE2 (NRC: GCC ohne -march).

### Vergleich je Karte und Stufe

Jede Stufe mit derselben Eingabe: `-bsp` aus der .map, `-vis` auf NRCs .bsp
nach `-bsp`, `-light` auf NRCs .bsp nach `-vis`; daneben NRC gegen NRC
(zweiter Lauf) als Grundlinie. "Dreiecke" = je Shader die Menge der Dreiecke
mit allen Eckdaten ausser den unbelegten Lightmap-Koordinaten. "unabh." = alle
Lumps, die nicht von der Flaechenreihenfolge abhaengen (entities, shaders,
planes, nodes, leafs und models ohne Flaechenverweis, leafbrushes, brushes,
brushsides ohne surfaceNum, fogs). Licht: Lightmap-Farbe am Schwerpunkt jedes
Dreiecks (unabhaengig von der Packung), Anzahl anders (groesste Abweichung).
Kuerzel: lsurf leafsurfaces, bside brushsides, dverts drawverts, dindex
drawindexes, surf surfaces, lmaps lightmaps, grid lightgrid, larray lightarray.

| Karte | Spiel | KB | Dreiecke gleich | unabh. | -bsp Bytes anders | NRC/NRC anders | -vis 1 Thread/mehrere | -light Bytes anders | Licht anders (max) | NRC/NRC (max) | Eckfarben anders | -bsp s uns/NRC | -light s uns/NRC |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| ein_pinsel | ja | 1 | 0/0 | gleich | - | - | gl/gl | - | 0 (0) | 0 (0) | 0 | 0.08/0.10 | 0.06/0.13 |
| ein_licht | ja | 1 | 2/2 | gleich | - | - | gl/gl | - | 0 (0) | 0 (0) | 0 | 0.06/0.10 | 0.07/0.13 |
| nrc_tiny_structural_brush | ja | 4 | 12/12 | gleich | - | - | gl/gl | - | 0 (0) | 0 (0) | 0 | 0.06/0.10 | 0.08/0.11 |
| vergleich_q3 | ja | 4 | 45/45 | gleich | lsurf, bside, dverts, surf | lsurf, bside, dverts, surf | gl/gl | - | 0 (0) | 0 (0) | 0 | 0.08/0.12 | 0.10/0.14 |
| vergleich_bp | ja | 5 | 45/45 | gleich | lsurf, bside, dverts, surf | lsurf, bside, dverts, surf | gl/gl | - | 0 (0) | 0 (0) | 0 | 0.07/0.12 | 0.10/0.14 |
| raum (Testkarte) | ja | 5 | 55/55 | gleich | lsurf, bside, dverts, surf | lsurf, bside, dverts, surf | gl/gl | dverts, lmaps, grid, larray | 0 (0) | 0 (0) | 0 | 0.06/0.11 | 0.37/0.39 |
| raum_leck (Testkarte) | ja | 7 | 135/135 | gleich | lsurf, bside, dverts, dindex, surf | dito | gl/gl | dverts, surf, lmaps, grid, larray | 0 (0) | 0 (0) | 0 | 0.07/0.10 | 0.45/0.44 |
| oa_fan | ja | 134 | 1118/1118 | gleich | lsurf, bside, dverts, dindex, surf | dito | gl/an | dverts, surf, lmaps, larray | 0 (0) | 0 (0) | 0 | 0.18/0.22 | 4.28/3.12 |
| t1_inter | ja | 81 | 446/446 | gleich | lsurf, bside, dverts, surf | dito | gl/an | dverts, surf, lmaps, larray | 0 (0) | 0 (0) | 0 | 0.21/0.24 | 0.26/0.31 |
| mp__duel1 | ja | 408 | 1446/1446 | gleich | lsurf, bside, dverts, dindex, surf | dito | gl/gl | dverts, surf, lmaps, larray | 49 (51) | 2 (1) | 0 | 0.28/0.33 | 3.57/2.43 |
| duel_pit | jk2 | 177 | 1123/1123 | gleich | lsurf, bside, dverts, dindex, surf | dito | gl/gl | dverts, lmaps, grid, larray | 9 (34) | 6 (34) | 0 | 0.61/0.19 | 0.74/0.63 |
| duel_jedi | jk2 | 291 | 1409/1409 | gleich | lsurf, bside, dverts, dindex, surf | dito | gl/an | dverts, surf, lmaps, grid, larray | 2 (2) | 1 (2) | 0 | 0.26/0.29 | 0.40/0.42 |
| duel_twinsuns | md | 34 | 29/29 | gleich | lsurf, bside, dverts, surf | dverts | gl/gl | larray | 0 (0) | 0 (0) | 0 | 0.66/0.75 | 1.62/1.70 |
| md_ch_ben | md | 85 | 258/258 | gleich | lsurf, bside, dverts, dindex, surf | lsurf, bside, dverts, surf | gl/gl | dverts, lmaps, grid, larray | 0 (0) | 0 (0) | 0 | 0.71/0.79 | 4.62/3.52 |
| duel_sithcatacombs | md | 181 | 1421/1421 | gleich | lsurf, bside, dverts, dindex, surf | dito | gl/gl | surf, larray | 0 (0) | 0 (0) | 0 | 0.80/0.90 | 0.81/0.94 |

Die entkompilierten Karten (`SP\karte_dekomp\<spiel>\q3`) mit den Spieldaten
des Rechners (JA "GameData original", MD "GameData Movie Duels", JK2). Lumps
je Karte und Stufe mit Groesse, Anzahl und groesster Abweichung in Einheiten:
`Y:\karte\laeufe\q3vgl\alle\alle.json`. Die 49 Schwerpunkte auf mp__duel1 sind
die sieben zusammengefassten Flaechen aus 3. (in den Laeufen ohne diese
Zusammenfassung: 3 bis 4, wie NRC gegen NRC). Mit `-threads 1` auf duel_pit:
Lightmaps NRC/NRC 11 Flaechen (max 140), uns/NRC 10 bzw. 14 (max 173/150),
Lichtgitter gleich.

### Zeit

`-bsp` und `-vis` gleich schnell (duel_pit `-bsp` 0,15 gegen 0,19 s, Median aus 3; die 0,61 s in der Tabelle waren der erste Lauf mit kaltem Dateicache). `-light` bei uns auf grossen Karten langsamer:
mp__duel1 3,45 s gegen 2,39 s (je Median aus 3, 12 Threads), mit `-threads 1`
11,5 s gegen 8,2 s (+41 %) - die Zeit steckt im Ausleuchten der
Radiosity-Lichter (IlluminateRawLightmap je Bounce). Rechnung und Ergebnis sind
gleich; es ist der Code, den MSVC fuer die kleinen Vektor-Rechnungen erzeugt.
`/GL /Ob3 /LTCG` brachten nichts (3,46 s) und sind wieder raus. Offen fuer
Stufe 4: mit einem Profiler (xperf braucht Administratorrechte) die heisse
Stelle suchen.

### Test mit festen Erwartungen: ctest `q3map2_vergleich`

`tools/karte/q3map2_vergleich.py pruefen bhed_q3map2.exe tests/karte/q3map2`:
drei Testkarten aus `tools/karte/gen_q3map2_testkarten.py` (raum: zwei Raeume,
Durchgang, zwei Lichter, Detail-Saeule, Tuer, Patch, 7 Shader; raum_leck: Loch
in der Ostwand; raum_einfach: raum mit einem Shader), ohne Spieldaten (leerer
`-fs_basepath`), `-threads 1`. Erwartungen aus NRCs q3map2.exe
(`... erwartung <NRC> <uns> tests/karte/q3map2 --laeufe 6`, je Stufe 6 Laeufe
von beiden): erwartet wird jedes Merkmal, das bei beiden stabil und gleich war.
Pflicht (der Test scheitert) sind die Merkmale, die von Zeigern und
Speicherresten nicht abhaengen koennen; auf raum_einfach (ein Shader) jeder
Lump ausser `drawverts` nach `-bsp`. Die uebrigen werden gemeldet ("dazu
Byte-gleich n/m").

| Lauf | Mutation | Ergebnis |
|---|---|---|
| p2_rot | Nr. 8 aus (map.cpp unveraendert uebersetzt) | 2 Abweichungen: `leafbrushes` auf raum und raum_leck |
| p2_rot7 | Nr. 7 aus (MSVCs std::sort) | 2 Abweichungen: raum_einfach `-light` `drawverts`, `lightmaps` |
| p2_gruen7 | - | 0 Abweichungen; raum bsp 17/17 Pflicht + 10/10 Byte-gleich, vis 18/18 + 8/8, light 19/19 + 5/5; raum_leck 17/17 + 2/2; raum_einfach 26/26 je Stufe |

## Punkt 3: Build-Menue wie NRC

Kern ohne Oberflaeche: `include/bhed/karte/bau.h`, `src/karte/bau.cpp`
(build.cpp: VariableString/Conditional/Tool, die XML-Leser mit dem
ExtraResourcePaths-Kniff, build_construct_commands, ProjectXMLExporter wie
XMLStreamWriter, version_compatible) und `include/bhed/karte/xmlstrom.h`,
`src/karte/xmlstrom.cpp` (XML Stueck fuer Stueck wie der SAX-Leser von libxml2:
Elemente, Attribute, Entities, CR LF -> LF; auch fuer den Rueckkanal von q3map2
in Punkt 4). Oberflaeche: `gui/app_karte_bau.cpp` (Build_constructMenu mit den
Befehlen als Tooltip, Build_runRecentExecutedBuild auf F5, der Dialog
"Build Menu" = BuildMenuDialog_construct/DoBuildMenu, LoadBuildMenu/
SaveBuildMenu, build_init_variables, RunBSP bis zu den Befehlen).

- Gespeichert wie NRC erst beim Beenden (BuildMenu_Destroy) und nur, wenn
  geaendert; Datei `<Einstellungen>\karte_<ja|jk2>_build_menu.xml` (NRC:
  `settings/<spiel>.game/build_menu.xml`), Name in der Einstellung
  `BuildMenu`; ohne eigene Datei die `default_build_menu.xml` des Spiels aus
  `data/karte/<spiel>`. Reset liest die Datei neu ("Reset to editor start
  state"), Cancel stellt Eintraege, Werkzeuge und den zuletzt gebauten Eintrag
  wieder her. Fenstergroesse in `BuildMenu/geometry` (Vorgabe 700 x 500).
- Dialog: Eintragsliste und Befehlsliste wie QTreeWidget_drag (Doppelklick/F2
  bearbeitet an Ort und Stelle, leer = entfernen, "+" haengt an, Entf, Strg+C/
  Strg+V vor dem aktuellen, Ziehen verschiebt, mit Strg kopiert, nicht unter
  "+"), Befehlsliste 16 x max(Zeilen, 4) + 7 + 2 hoch; "Build Variables"
  (flache, ankreuzbare Gruppe): Konstanten mit Kopiersymbol (Klick legt
  "[Name]" in die Zwischenablage, Tooltip 666 ms), Werkzeuge mit "+" (Name:
  CustomDelegate0, Wert als XML-Text: CustomDelegate1); OK / Cancel / Reset
  (Tooltip). Modal wie QDialog::exec - das Build-Menue oeffnet sich solange
  nicht. Waechst der Inhalt (Variablen aufgeklappt), waechst der Dialog.
- Variablen wie build_init_variables: RadiantPath (Ordner von behaved.exe),
  ExecutableType exe, EnginePath, UserEnginePath (leer: die .game haben kein
  prefix), ExtraResourcePaths (` -fs_pakpath "..."` je zusaetzlichem Pfad),
  MonitorAddress (127.0.0.1:39000 bei Ueberwachung), GameName, BspFile,
  MapFile (bei aktiver Region die .reg), MapName.

Abweichungen:
- `data/karte/ja|jk2/default_build_menu.xml`: `[RadiantPath]bhed_q3map2` statt
  `[RadiantPath]q3map2` (unser Programm; Kommentar in der Datei), der
  ExtraResourcePaths-Kniff erkennt beide Namen.
- `[GameName]`: beim Regelwerk Movie Duels ohne gesetztes fs_game der Ordner der
  Mod (`MD`), sonst wie NRC gamename_get (fs_game oder basegame).
- Eine Variable `[ExtraQ3map2Args]` gibt es in NRC 20260114 nicht (build.cpp,
  qe3.cpp, Spielpakete durchsucht); die Entsprechung ist `[ExtraResourcePaths]`
  - nachgebaut sind genau NRCs Variablen.
- Farben nach dem Farbschema des Programms; OK ohne den Rahmen des
  Standardknopfs (wie die anderen Dialoge des Editors); lange Werte in den
  Variablentabellen abgeschnitten statt umgebrochen (QTableView::wordWrap).
- Bis Punkt 4 baute ein Eintrag nur die Befehle und schrieb sie in die Konsole.

Fotovergleich (`SP\s3\werk\foto_bau_vgl.py`, Lauf s3p3gruen, Massstab 2):
Innenraum 700 x 500 wie NRC; im Selbsttest auf den Punkt geprueft: OK 611/9
80 x 22, Cancel 37, Reset 65, Eintragsliste 21/37 572 x 276, Befehlsliste
21/359 572 x 73, Build Variables 9/450 Hoehe 41 - alles wie im Foto
`dlg_Build_Customize_0.png` gemessen.

### Rot/Gruen

| Pruefung | rot | gruen |
|---|---|---|
| ctest `karte_bau` (42 Pruefungen) | 10 FEHL (Menue noch mit `q3map2`, Rundlauf, XML-Leser bei CR am Stueckende) | 42/42 |
| Selbsttest `BHED_STUFE3_NUR=bau`, Lauf s3p3rot: Cancel stellt nicht zurueck, Strg+V nach statt vor, ohne MonitorAddress (`// MUTATION s3p3`) | 34 OK / 13 FEHLER | s3p3gruen2: 47 / 0 |

Checkliste: BuildMenuCustomize, Build_runRecentExecutedBuild (je Menue und
Befehl), Dialog "Build Menu" [x]; dazu OpenManual und CheckForUpdate aus Punkt
0b nachgetragen (gen_befehle.py STUFE_3_FERTIG). Stand: 790 fertig, 144
ausgegraut, 2 offen von 936.

## Punkt 4: Bauen aus dem Editor

Kern ohne Oberflaeche: `include/bhed/karte/baumonitor.h`,
`src/karte/baumonitor.cpp` (watchbsp.cpp: der l_net-Rahmen - 4 Byte Laenge LE,
Inhalt bis NUL -, der SAX-Zustand wie message_info_t mit recurse, ignore_depth,
stop_depth, msg_level, geometry_depth; message, polyline, select, pointmsg,
windingmsg; Stop Compilation on Leak; Pointfile_Parse fuer die .lin).
Windows: `gui/karte_bau_plattform.h`, `gui/karte_bau_win32.cpp` (q3map2 starten
und lesen, Port 39000, die Engine losgeloest starten). Oberflaeche:
`gui/app_karte_bau.cpp` (DoMonitoringLoop, DoEBeginStep, RoutineProcessing,
runEngine, RunBSP, CDbgDlg "Q3Map debug window").

Wie NRC:
- RunBSP speichert vorher (SaveMap; eine unbenannte Karte fragt mit Speichern
  unter, abgebrochen -> "build cancelled"; mit Region wird `<Karte>.reg`
  geschrieben und gebaut - NRCs Temp-Datei), loescht die alte .lin
  (Pointfile_Delete) und wertet die Befehle aus.
- Je Schritt: "Setting up", lauschen auf Port 39000, "Listening...",
  "=== running build command ===" mit dem Befehl, q3map2 starten, bis 5 s auf
  die Verbindung warten (sonst der Kasten "The connection timed out, assuming
  the build process failed ..."), "Connected.", den XML-Strom lesen (Stufe
  1/2/3 als Ausgabe/Warnung/Fehler in der Konsole, live), "Connection closed.",
  der naechste Schritt. Eine Meldung der Stufe 3 haelt die Ueberwachung am Ende
  ihres Elements an (bei einem Leck nur mit "Stop Compilation on Leak"). Port
  belegt: NRCs Meldung in der Konsole und der Kasten "Build monitoring".
- Ohne "Enable Build Process Monitoring" (oder ein Befehl ohne
  [MonitorAddress]): RunBatch. Run Engine After Compile: runEngineCmd mit
  %mapname% = Name der Karte (im Selbsttest nur gemerkt, nie gestartet).
- Q3Map debug window: oeffnet sich mit der ersten Rueckmeldung (select, point,
  winding, Leck); eine Zeile waehlen hebt den Pinsel hervor (CSelectMsg). Das
  Zeichnen von Punkten, Wicklungen und der Leck-Linie kommt mit Punkt 5.

Abweichungen (Auftrag, Windows):
- q3map2 laeuft ohne Konsolenfenster (CREATE_NO_WINDOW) in einem Job: es endet
  mit dem Editor, auch wenn dieser abstuerzt. Der Socket lauscht nur auf
  127.0.0.1 und exklusiv (kein Dialog der Windows-Firewall, kein fremder
  Prozess auf dem Port). Das Kind erbt nur NUL und das Schreibende der Pipe.
- Ohne Ueberwachung laufen die Befehle nacheinander im Hintergrund statt als
  qe3bsp.bat in einem Konsolenfenster; die Ausgabe kommt ueber eine Pipe live in
  die Konsole (WARNING gelb, ERROR und leaked rot - q3map2s eigene
  Konsolenfarben). "Dump non Monitored Builds Log" schreibt sie nach
  `<Einstellungen>\junk.txt` (NRC: "> junk.txt").
- Dazu: Build > Cancel build (beendet q3map2), der Fortschritt in der
  Statuszeile ("Building <Eintrag>: step i of n, m:ss"), am Ende eine Zeile mit
  Eintrag, Schritten, Zeit und - wenn nicht 0 - den Rueckgaben von q3map2
  ("(1: exit 0xC0000005)"), bei einem Leck ein Hinweis, wie man es findet. Ein
  neuer Bauauftrag beendet einen laufenden (NRC: nur eine Warnung, der alte
  Prozess lief weiter). Das Build-Menue ist dadurch 405 + 7 + 24 Punkte hoch.
- Ziel-Ordner (Seite Build, Einstellung `BuildTargetFolder`, mit Hinweis
  darunter): leer = neben der .map wie NRC; sonst wird die Karte (bzw. die .reg)
  dorthin kopiert und dort gebaut. Liegt der Ziel-Ordner - oder ohne Ziel-Ordner
  die Karte - im Spielordner (Engine-Pfad oder ein Suchordner des Spiels), wird
  im Arbeitsordner `<Einstellungen>\karte_bau` gebaut, mit Hinweis in der
  Konsole. Nie im Spielordner.
- Engine-Argumente: constructEngineArgs nimmt gamename_get; hier dessen
  Entsprechung `[GameName]` (beim Regelwerk Movie Duels `+set fs_game MD`),
  auch als Vorgabe auf der Seite Build.

Gefunden und behoben (je erst im Selbsttest rot):
- s3p4a, s3p4b: in 2 von 3 Laeufen von [Final] Fast "Failed to get a listening
  socket on port 39000" vor Schritt 2. Ursache: CreateProcess mit
  bInheritHandles vererbte alle erblichen Handles, auch den lauschenden Socket;
  q3map2 hielt Port 39000, bis es ganz zu Ende war. Ein Versuch mit SO_LINGER 0
  aenderte nichts (wieder entfernt). Behoben mit
  PROC_THREAD_ATTRIBUTE_HANDLE_LIST und WSA_FLAG_NO_HANDLE_INHERIT; ab s3p4c
  verbindet sich jeder Schritt.
- s3p4a: Run Engine unter Movie Duels ohne `+set fs_game MD`.
- s3p4f: eine Karte ohne Entitaet (New Map, gleich gespeichert) laesst q3map2
  abstuerzen - NRCs q3map2.exe genauso (gleiche Ausgabe, gleiche Rueckgabe). Die
  Zeile am Ende sagte trotzdem nur "fertig", weil der Prozess beim Schliessen
  des Sockets noch nicht zu Ende war. Jetzt wartet der Schritt bis 1 s auf das
  Ende und nennt den Code. Der Absturz selbst bleibt (Fremdquelle unveraendert).
- Beobachtet, nicht geaendert: `-light` ohne .bsp ruft Error() mit leerem Text
  auf (nur der Rahmen "**** ERROR ****") - NRCs q3map2 genauso.

Selbsttest `BHED_STUFE3_NUR=bau` (mit `BHED_Q3MAP2` und `BHED_Q3MAP2_DATEN`, siehe
TESTING.md), Spielordner Movie Duels als fs_basepath (nur gelesen):
- [Final] Fast aus dem Menue: waehrenddessen die Statuszeile; danach je Schritt
  die Zeilen wie watchbsp, q3map2s Ausgabe live, Warnungen gelb, keine roten
  Zeilen, die Schlusszeile; .bsp und .srf neben der Karte (keine .prt: -vis ohne
  -saveprt loescht sie), keine .lin.
- Die .bsp gegen `raum.nrc.bsp`: shaders, planes, nodes, leafbrushes, brushes
  gleich; leafs, models, brushsides ohne Flaechenbezug gleich; visibility
  (72 Byte), lightmaps, lightgrid gefuellt, lightarray 459 Gitterpunkte.
- -light ohne .bsp: ERROR rot, die Ueberwachung haelt an. Port 39000 belegt:
  Meldung und Kasten. Ein ueberwachter Befehl, der sich nicht verbindet: nach
  5 s der Kasten. Ohne Ueberwachung mit Log: Pipe, junk.txt; dort ein Fehler mit
  "(1: exit 1)". Run Engine: `"<Engine>jaSP.exe" +set sv_pure 0 +set fs_game MD
  +devmap raum` gemerkt. Ziel-Ordner ueber die Seite Build eingetragen: dort
  gebaut; Ziel-Ordner bzw. Karte im Spielordner: der Arbeitsordner (nur die
  Vorschau, ohne zu bauen). Region: die .reg. Unbenannt: abgebrochen bzw. unter
  neuem Namen gespeichert und gebaut; leere Karte: der Absturzcode.
- Bedienbar: waehrend eines langen -light zoomt die 2D-Ansicht (1 -> 1.25);
  Build > Cancel build beendet q3map2; der naechste Eintrag baut wieder.

### Rot/Gruen

| Pruefung | rot | gruen |
|---|---|---|
| ctest `karte_bau` (58, Abschnitt 9: der Rueckkanal mit einem aufgenommenen Strom `raum_leck.strom.bin`/`.lin`) | Mutation "Stufe 3 setzt stop_depth nicht": 2 FEHL | 58/58 |
| Selbsttest s3p4rot: Ziel-Ordner wirkungslos, Abbrechen tut nichts, Meldungsstufen ohne Farbe (`// MUTATION M1-M3`) | 62 OK / 4 FEHLER | s3p4g: 71 / 0 |
| Selbsttest s3p4a/b: Handle-Vererbung (s. oben) | 60 / 7 | s3p4c: Port gruen; s3p4d: 66 / 0 |
| Selbsttest s3p4f: Rueckgabe beim Absturz | 69 / 2 (der zweite war der Test: ein Pinsel ohne Startpunkt leckt - jetzt ein dichter Raum mit info_player_start) | s3p4g: 71 / 0 |

ctest 104/104.

## Punkt 5: Leck-Linie, Rueckmeldungen in 2D, Portal Viewer

Leck-Linie (radiant/points.cpp) in `gui/app_karte_leck.cpp`:
- Aus dem Rueckkanal (polyline, Punkt 4) oder mit File > Pointfile
  (TogglePointfile) aus `<gebaute Karte>.lin`, sonst `.pts`; Meldungen wie NRC
  ("Pointfile X not found" rot, je fuer .lin und .pts; "Reading pointfile X";
  "Corrupt point file, line N" als Warnung). Wie NRC bleibt eine alte Linie im
  Speicher, wenn die Datei fehlt, und erscheint beim naechsten Einschalten.
- Gezeichnet wie $POINTFILE: rot, 4 breit; in der Kamera mit Tiefentest und
  -schreiben (eSortFullbright, nach den deckenden Flaechen), in den
  2D-Ansichten nach den Pinseln.
- Next / Previous leak spot (Strg+Umschalt+K / L): Pointfile_UpdateViews -
  Kamera und alle 2D-Ansichten an den Punkt, Gier/Nick zum naechsten Punkt;
  "End of pointfile" / "Start of pointfile". Das erste Next nach dem
  Einschalten geht wie bei NRC gleich zu Punkt 2.
- Pointfile_Clear bei New Map, Open Map und beim Speichern (Map_Save) - also
  auch vor jedem Bauen, das die Karte speichert.
- Rueckmeldungen pointmsg / windingmsg (feedback.cpp Draw2D): die gewaehlte
  Zeile des Q3Map debug window zeichnet in den 2D-Ansichten den Punkt (4) und
  ein Quadrat +-8 rot bzw. die Ecken rot und die Flaeche (0.133, 0.4, 1, 0.5)
  gemischt. Die Zeile einer select-Meldung waehlt die Entitaet/den Pinsel
  (SelectBrush; beim Leck die Entitaet, aus der es leckt).

Portal Viewer (contrib/prtview) - Kern `include/bhed/karte/portale.h`,
`src/karte/portale.cpp`, Oberflaeche `gui/app_karte_portale.cpp`:
- Plugins > prtview: Load .prt file (der modale Dialog "Load .prt": Datei mit
  Suchen-Knopf, Show 3D, Show 2D, OK/Cancel; Vorgabe `<gebaute Karte>.prt`),
  Unload, Toggle portals (3D)/(2D), Configure Portal Viewer, About. Jeder
  Befehl schreibt wie QERPlug_Dispatch `Portal Viewer plugin: Command "..."`.
- CPortals::Load wortgleich (PRT1, PRT2, PRT1-AM, Hint-Flags an der vierten
  Kopfzeile erkannt, alle Fehlermeldungen); CBspPortal::Build mit Mitte,
  inneren Punkten (1 % zur Mitte), Grenzen und Zufallsfarbe aus rand() - hier
  eine eigene Folge wie die C-Laufzeit von Windows (41, 18467, 6334, ...),
  damit kein anderer rand()-Aufruf die Farben aendert.
- 2D: Linienzuege der Portale in color_2d, width_2d. Kamera: erst die Umrisse
  (innere Punkte, color_3d, width_3d), dann die Polygone (Zufallsfarbe,
  Opacity, nach Abstand sortiert ausser bei "Z-Buffer Test and Write") - die
  Reihenfolge, die OpenGLState_less aus den Zustandsbits ergibt (beide
  eSortOverlayFirst). Tiefe je Z-Buffer-Stufe wie Portals_constructShaders
  (Umriss mit RENDER_DEPTHWRITE schon in der Grundstufe), Wuerfel-Ausschnitt wie
  calculateCubicClipVolume (auch NRCs vertauschte min/max), ohne RENDER_FILL
  (Kamera "Wireframe") die Polygone als Linien.
- Nebel: RENDER_FOG mit GL_EXP, Dichte 0.001 - neu im Shader der Kamera
  (f = exp(-Dichte * w), w = Abstand im Auge), nur fuer die Portale.
- Einstellungen wie prtview.ini (LoadConfig beim ersten Gebrauch, SaveConfig beim
  Beenden): `<Einstellungen>\karte_prtview.ini` (Name wie die anderen Dateien des
  Editors), Inhalt wie IniFile + "wt": sortiert, `Wert        ; Kommentar`, CRLF.
- Dialog "Portal Viewer Configuration": nicht modal, Lagen aus dem Foto
  gemessen (Massstab 2, Innenraum 386 x 510): ankreuzbare Gruppen (aus = Inhalt
  gesperrt), Schieber wie QSlider (Lage = QStyle::sliderPositionFromValue,
  Ziehen = sliderValueFromPosition, Klick in die Rinne = Seitenschritt 10,
  mittlere Taste = absolut, Rad = 3), Color-Knoepfe mit dem Farbdialog
  (RGB_PACK wie OnColor), Klappliste Z-Buffer, Draw Hint/Regular Portals.
  Im Selbsttest jede Lage auf den Punkt gegen das Foto geprueft.

Abweichungen:
- Farben der Dialoge nach dem Farbschema des Programms; About als Kasten mit
  Text (NRC: Rich Text mit mailto-Links), "Built against NetRadiant
  1.6.0n-git-68ecbed (BehavEd-Remake, <Datum>)".
- "Load .prt" ohne NRC-Foto: Groesse nach Qt-Regeln (Rand 11, Abstand 6, Knoepfe
  80 x 22) - 188 x 118.
- Map_Name fuer .lin/.prt ist die gebaute Datei (mit Ziel-Ordner die Kopie dort).
- Der Leck-Hinweis (Punkt 4) nennt File > Pointfile (vorher faelschlich View).
- prtview steht unter LGPL 2.1 (oder spaeter); die portierten Dateien tragen
  nach deren Abschnitt 3 den Vermerk der GPL v2 (README, ctest karte_lizenz).

### Rot/Gruen

| Pruefung | rot | gruen |
|---|---|---|
| ctest `karte_portale` (25: IniFile, LoadConfig/SaveConfig, rand(), Load an raum.nrc.prt, PRT2/PRT1-AM, alle Fehler) | Mutation Hint-Erkennung: 4 FEHL | 25/25 |
| Selbsttest `BHED_STUFE3_NUR=leck`, Lauf s3p5rot: Linie in 2D nicht gezeichnet, Gier/Nick vertauscht, kein Nebel im Shader, Seitenschritt falsch herum (`// MUTATION M1-M4`) | 48 OK / 7 FEHLER | s3p5b: 55 / 0 |
| Selbsttest s3p5a (erste Fassung) | 52 / 4 (Erwartungen: die Bildmitte nur teilweise rot, die Zeile heisst "Entity 3, Brush 0: Entity leaked" - es leckt aus dem Licht in Raum B -, eine 1 Punkt breite Kante unter MSAA) | s3p5b: 55 / 0 |

ctest 105/105. Checkliste: TogglePointfile, NextLeakSpot, PrevLeakSpot und die
sechs prtview-Befehle (Menue, Befehl, Plugin) [x] - Stand 808 fertig, 126
ausgegraut, 2 offen von 936.

## Punkt 6: AutoCaulk Selected (F4) und der Rest der Checkliste

AutoCaulk wie map_autocaulk_selected (radiant/map.cpp:2121-2368) - Kern in
`src/karte/auswahlbefehle.cpp` (triggerAbwaehlen = DeselectTriggers,
autocaulkPinsel = Map_Traverse_Selected mit WriteBrushesWalker, caulkLesen,
caulkAnwenden = CaulkBrushesWalker) und `src/karte/kartenformat.cpp`
(autocaulkKarteSchreiben), Oberflaeche `autoCaulkAuswahl` in
`gui/app_karte_bau.cpp`:
- unbenannt: erst Speichern unter; gewaehlte trigger_* samt Kindern ab; ohne
  Auswahl "map_autocaulk_selected(): nothing is selected"; das Auge muss in den
  Auswahlgrenzen +-1024 liegen ("camera must be near selection!").
- `<Karte>_ac.map` wie NRC: "{ classname worldspawn" mit den gewaehlten Pinseln
  (ohne Kommentare), dem Kasten (ConstructRegionBrushes mit dem Region Box
  Shader), "}", info_player_start am Auge ("%g"), die gewaehlten
  Punktentitaeten ueber writeGraph (Ebenen, "// entity N"), am Ende das '\n'
  des ersten TokenWriter; Textmodus (CRLF).
- `bhed_q3map2 -game quake3 -fs_basepath ... -fs_homepath "" -fs_game <[GameName]>
  -autocaulk -fulldetail "<Datei>"` ohne Fenster, der Editor wartet (Q_Exec
  waitfor); dann `<Karte>_ac.caulk`: je beitragende Flaeche ein Zeichen - c
  caulk, w watercaulk, l lavacaulk, s slimecaulk, N nodraw, n nodrawnonsolid
  (GetCommonShader: textures/system/...); Rueckgaengig "AutoCaulk N faces".
- Wie NRC zaehlt ein Pinsel ohne beitragende Flaeche beim Anwenden mit, obwohl
  er nicht in die Datei kommt (dann verrutschen die Nummern) - so gelassen.

Abweichungen:
- Die Dateien `_ac.map`, `_ac.caulk` (und was q3map2 dazu schreibt: `_ac.bsp`,
  `_ac.srf`) entstehen wie beim Bauen nie im Spielordner (Arbeitsordner, mit
  Ziel-Ordner dort). Das Programm ist bhed_q3map2 (in Anfuehrungszeichen - NRC
  setzt den Pfad ohne, mit Leerzeichen scheitert es). Warten hoechstens 10
  Minuten (NRC ohne Grenze).

Checkliste: AutoCaulkSelected (Befehl und Menue Brush) [x] Punkt 6; "Q3Map
debug window" [x] (Punkte 4/5). Die uebrigen Plugins (bobToolz, brushexport2,
MeshTex, ShaderPlug, SunPlug, UFO:AI) tragen jetzt "Stufe 4" (Auftrag; Tooltip
und Checkliste). Stand 811 fertig, 123 ausgegraut, 2 offen (Exit: Menue und
Befehl - aus Stufe 1, nicht Teil von Stufe 3) von 936.

### Rot/Gruen

| Pruefung | rot | gruen |
|---|---|---|
| ctest `karte_autocaulk` (16: DeselectTriggers, Reihenfolge, die _ac.map, .caulk lesen, anwenden, Rueckgaengig) | erste Fassung 1 FEHL (Test: Shadernamen stehen ohne "textures/" in der .map) | 16/16 |
| Selbsttest `BHED_STUFE3_NUR=caulk`, Lauf s3p6rot: ohne -autocaulk, ohne DeselectTriggers (`// MUTATION`) | 22 OK / 3 FEHLER | s3p6b: 25 / 0 (Boden unten und Nordwand aussen caulk, innen nicht; 41 Flaechen) |
| s3p6a (erste Fassung) | 23 / 2: das Auge stand in der Detail-Saeule - q3map2 "Entity in solid", leaked, keine .caulk (NRC genauso) | Auge bei (-384 128 64) |

ctest 106/106.

## Punkt 7: Selbsttest im Modus karteneditor

Stufe 3 im Selbsttest (`BHED_KARTE_NUR=stufe3`; Teile mit `BHED_STUFE3_NUR`
= `maus`, `bau` (Build-Menue, Bauen, Leck, Portale, AutoCaulk), `leck`,
`caulk`):
- Bauen aus dem Editor (Punkt 4): [Final] Fast mit BSP, VIS, LIGHT; die Konsole
  Zeile fuer Zeile wie watchbsp; die .bsp gegen `raum.nrc.bsp` (shaders, planes,
  nodes, leafbrushes, brushes gleich, leafs/models/brushsides ohne
  Flaechenbezug, visibility/lightmaps/lightgrid gefuellt, lightarray 459); die
  Fehlerfaelle; bedienbar beim Bauen, Abbrechen.
- Leck-Karte (Punkt 5): die Linie aus dem Rueckkanal gleich der .lin, rot in
  2D und 3D (Bildpunkte), Next/Previous leak spot, File > Pointfile.
- Portale (Punkt 5): Load .prt, Linien in 2D, Polygone in 3D, Nebel, der
  Konfigurationsdialog, die Ini.
- Fotos gegen NRC: "Build Menu" (Punkt 3) und "Portal Viewer Configuration"
  (Punkt 5) - die Lagen prueft der Test auf den logischen Punkt; nebeneinander
  (SP\s3\werk\foto_bau_vgl.py, Lauf s3p7a) stimmen Rahmen, Listen und Knoepfe
  ueberein, die Farben folgen dem Farbschema des Programms.
- AutoCaulk (Punkt 6).

Neu in Punkt 7: der Test findet bhed_q3map2.exe und die Erwartungen selbst
(die Arbeitskopie ueber behaved.exe: `build-test` bzw. `build/bin/Release`,
`tests/karte/q3map2`); BHED_Q3MAP2 / BHED_Q3MAP2_DATEN gehen weiter vor.

Laeufe: s3p7a (mit den Variablen und Fotos) 222 OK / 0 FEHLER, s3p7b (ohne
Variablen) 222 / 0.

## Punkt 8: Handbuchseite "Karte bauen"

`tools/handbuch/inhalt_kartebauen.py` (id `karte_bauen`, Gruppe "Karte und
Szene", nach den Bauwerkzeugen), vier Sprachen: das Menue Build und der Dialog
Build Menu, die drei Stufen (Tabelle) und die Eintraege, was beim Bauen
passiert (Speichern, Hintergrund, Konsole, Statuszeile, Abbrechen,
Schlusszeile, ohne Ueberwachung), Lecks finden (rote Linie, Q3Map debug window,
Strg+Umschalt+K / L, File > Pointfile), Portale (prtview: Laden, Anzeige,
Einstellungen), wohin die Dateien kommen (Ziel-Ordner, nie in den Spielordner),
im Spiel testen (die .bsp selbst nach `maps` kopieren, das Spiel selbst
starten, `devmap <Name>` in der Konsole; Run Engine After Compile als Hinweis)
und AutoCaulk. Beschriftungen ueber `{{Kennung}}` (in jeder Sprache wie im
Programm, z. B. deutsch "Kompilieren > Bauen abbrechen").

Neun Bilder je Sprache aus dem Selbsttest (`BHED_EDITORTEST=handbuch`,
`BHED_HB_NUR=kartebauen`, Laeufe hb4en, hb4de, hb4zhHans, hb4ja; auch Teil von
`BHED_HB_NUR=karteneditor`): Menue, Customize, Statuszeile beim Bauen, Konsole
danach, Seite Build, Leck (2D, Kamera auf der Linie, Q3Map debug window),
Load .prt, Portale, Portal Viewer Configuration. Fuer die Bilder bekommt die
Testkarte Texturen des Spiels (imperial) statt test/* ("shader not found").

Mitgezogen: die Seiten "Der Karten-Editor" und "Bauwerkzeuge" sagten,
Kompilieren und Plugins seien ausgegraut - jetzt nur noch die Plugins ausser
prtview, mit Verweis auf [[karte_bauen]]. LIESMICH: die Bildlaeufe des
Karten-Editors (und zh-Hans -> Ordner zh).

`gen_handbuch.py`: 136 Seiten x 4 Sprachen, `--pruefen`: ok.

Beobachtet (aelter, nicht Teil von Stufe 3): auf der Seite Build der
Einstellungen ist auf Japanisch die Beschriftung "リージョン枠のシェーダー" links
abgeschnitten - die Spaltenbreite misst mit Tahoma (labelBreite), gezeichnet
wird mit der Schrift der Sprache.

## Punkt 9: Schlussrunde

Ein Testprozess zur Zeit, Selbsttests mit Prioritaet Idle und dem 8-GB-Waechter
(nie ausgeloest, hoechste Spitze 1.5 GB in spulen); Skript `SP\s3\ende_s3.sh`,
Summe `SP\s3\ende_summe.txt`.

| Test | Ergebnis |
|---|---|
| ctest | 106 / 106 |
| Karten-Editor voll mit grosser Karte (s3eke) | 986 / 1 - die Breite des Menues Help (unten); nach der Korrektur s3eke2: 987 / 0 (Spitze 973 MB); "145 Befehle, nicht ausgeloest: -"; Checkliste "304 fertige Befehle, 3 Menue-Rufe, 296 nachgebaut; es fehlt: -; zu viel: -" |
| Modus 1 (Gesamttest) | s3e1: 2644 / 1 ("... und legt das ganze Skript in die Zwischenablage" - die Windows-Zwischenablage, die Statuszeile meldete "18 command(s) copied"); Wiederholung s3e1b: 2645 / 0 - wie rc605 |
| karte | s3ekarte: Absturz nach 57 s bei "Halte D" (0xC000001D, s. u.); Wiederholungen s3ekarte2, s3ekarte3: je 227 / 0 |
| kartenfunde, fix604b, laserfalle, geschuetzanim, zielstrahl, effektgrund, figurdeform, jk2, tuerlauf | 53, 32, 8, 3, 2, 1, 1, 15, 13 OK / je 0 FEHLER |
| kamera, missionsbau, szenen, ton, npcklasse | 95, 95, 26, 34, 12 / je 0 |
| gluehen, gras, spulen, figurstufen, missionsliste, rest605 | 8, 13, 13, 8, 11, 6 / je 0 |
| Leistung (md_am_sith, BHED_D3D_DEBUG=0, rc605-Bau aus Y:\integ und unser Bau abwechselnd, je zweimal) | Vorbereitung 6.48 / 6.46 s (rc605) gegen 6.81 / 6.51 s; intro2_sith Mittel 15 / 14 gegen 14 / 15 ms (max 203 / 196 gegen 219 / 181); Flug 8.0 / 8.2 gegen 8.6 / 8.1 ms; zweiter Durchgang 10 / 12 gegen 11 / 10 ms - kein Unterschied |
| Kartenpruefung (278 Karten, Movie Duels, gegen das rc605-Schlussaudit `f5audit`) | alle Summen gleich (563 s): abgestuerzt/haengend, nicht geladen, GPU-Fehler, D3D-Meldungen, Protokollfehler 0 -> 0; nicht gezeichnete Entities 1 -> 1; NPC ohne Modell 2 -> 2; Weltshader-Luecken 0 -> 0; fehlende efx/Klaenge/Texturen/Modelle 27/29/139/5 unveraendert |
| grosse Karte duel_mos_espa (s3eke2) | Laden 823 ms; Editor je Bild 2.75 ms (Median 2.37, 95 % 4.66); Bild nach Auswahlwechsel 7.24 ms; Neubauten Geometrie 0 / Rest 10; Speicher privat +946 MB, Grafik +524 MB (Stufe 2: 661 ms, 2.05 ms, 6.97 ms, +934 / +523 - Idle-Prioritaet, die Bildzeiten schwankten schon dort um bis zu das Doppelte) |
| sauberer Neubau (Release, `--clean-first`) | 0 Fehler; 127 Warnungen, alle aus aelteren Stellen (C4996 u8path, C4456/C4458 verdeckte Namen, C4125 in entdefs.cpp) - keine aus Stufe 3 |
| Handbuch | `gen_handbuch.py --pruefen`: handbuch_sprachen ok |

In der Schlussrunde gefunden:

| Punkt | Rot (vorher) | Gruen (nachher) |
|---|---|---|
| Menue Help 227 statt 215 breit: seit Punkt 3 ist OpenManual nachgebaut (gen_befehle STUFE_3_FERTIG, Stufe 3 -> 1), unser Eintrag Manual zeigt darum seine Taste F1, und QMenu legt dafuer die Tastenspalte an (max_column_width += tabWidth = Vorschub "F1" = 12). Erwartung 215 + 12, Begruendung im Code | s3eke 986 / 1 | s3eke2 987 / 0 |
| Absturz im Modus karte bei "Halte D" (Kamera seitwaerts in der Kartenansicht, nicht im Editor): 0xC000001D, RIP 0x...4C7E = `std::string::_Assign+0x3E` (dbghelp gegen behaved.pdb, `SP\s3\werk\dump_sym.py`), mitten in einem Befehl; der Stapel ueber RSP haelt nur noch wWinMainCRTStartup (`dump_stapel.py`) - ein Sprung an eine falsche Adresse, kein Fehler in std::string. Zweimal wiederholt: 227 / 0. Nicht nachgestellt, also nicht behoben; Stufe 3 aendert an der Kartenansicht nur den Aufruf von karteneditor::bauSchritt (im Leerlauf ohne Wirkung) und darfRuhen | s3ekarte: Absturz | s3ekarte2, s3ekarte3: 227 / 0 (offen) |
