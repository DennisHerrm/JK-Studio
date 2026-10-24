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
