# Karten-Editor, Stufe 2: alle Bauwerkzeuge

Stand: Y:\karte, Zweig `main` der Arbeitskopie (auf Stufe 1b, `87299d9`).
Vorbild ist NetRadiant-Custom 20260114 (NRC, Git-Tag `20260114`, ausgepackt
nach `SP\nrc_20260114`); reine Fehlerbehebungen aus master sind genannt.
Quellangaben in den Kommentaren: `Funktion (datei:zeile)`. Das Projekt steht
unter GPL v2 (Herkunftskoepfe in allen portierten Dateien, ctest
`karte_lizenz`).

`SP` = `C:\Users\shank\AppData\Local\Temp\claude\c--Users-shank-Downloads-behaved-1-0-0-rc567-behaved\af9c6614-e078-4efa-a574-1fa901786464\scratchpad`

Regeln dieser Stufe: jeder Punkt mit einem gezielten Rot/Gruen-Wirkungstest
(absichtliche `// MUTATION` mit eindeutiger Marke, danach zurueckgesetzt,
grep auf `// MUTATION` = 0), ctest nach jedem Punkt, ein Commit je Punkt;
radiant.exe wurde in dieser Stufe nie gestartet (alle Vergleiche mit den
vorhandenen Fotos in `SP\nrc_fotos`).

## Commits

| Commit | Punkt | Inhalt |
|---|---|---|
| `8cd5536` | 1a | Komponenten: Ecken, Kanten, Flaechen, Huelle im Ecken-Modus, Texture Vertex Lock, Build-Werkzeug |
| `e9a1822` | 1b | Flaechen herausziehen (Strg+Alt+M1, Massen-Extrusion) und UV-Werkzeug (G) |
| `123ec92` | 2 | Clipper |
| `1bd2fc0` | 3 | CSG |
| `b0d56fa` | 4 | Pinsel-Grundformen |
| `d381aa8` | 5 | Drehen, Spiegeln, Arbitrary rotation/scale, Repeat/Reset Transforms |
| `71e92eb` | 6 | das ganze Curve-Menue |
| `db029da` | 7a | Surface Inspector und Texturtasten |
| `08daebe` | 7b | Tags im Texturbrowser |
| `0aba897`, `2742c3c` | 7c | Einstellungsseite Textures |
| `9bc24c3` | 8 | Auswahl und Datei (Select Inside/Touching, Region, Import, Save Selected/Region, Map Info, Find Brush, Autosave) |
| `456e4f8` | 9 | Filter (Menue, Filterleiste mit Rechtsklick) |
| `f8a7af5` | 10a | Ebenen |
| `eb21f09` | 10b | misc_model-Modelle (md3, ase, glm) |
| `f8e1dcf` | 10c | Modell-Browser |
| `651cd90` | 11a | Seiten Settings, Undo, Brush, Grid, Orthographic, Build |
| `0c9c440` | 11b | Spiel, Mod, Mapping mode, Pfade, Dialog "Global Preferences" |
| `c77cf7d` | 11c | abreissbare Menues, Fullscreen, OpenGL Font, 2D-Hintergrundbild, About, Bug report, Shader Editor, Engine Path |
| `f0856b8` | 12 | Selbsttest loest jeden neuen Befehl aus (Wirkung), Zeiten auf duel_mos_espa |
| `f29a0ec` | 13 | Handbuchseite "Karten-Editor: Bauwerkzeuge" in vier Sprachen |
| (dieser) | 14 | diese Notizen, Schlussrunde |

## Rot/Gruen je Punkt

| Punkt | ctest (neu) | Mutationen -> Fehlschlaege | Selbsttest gruen |
|---|---|---|---|
| 1a | karte_komponenten | Huellenbau (`ebenen.size() < 400`) -> 18 | s2p1c 26/0 |
| 1b | karte_komponenten | `m_extrudiert = false` -> 10; commitTransform ohne Wirkung -> 4 | s2p1d 28/0 |
| 2 | karte_csg | addPlane-Reihenfolge -> 6 | s2p2 39/0 |
| 3 | karte_csg (erweitert) | Sortierung + Pull ohne flipWinding -> 6 | s2p3b 59/0 |
| 4 | karte_grundformen | Tangente + Truncate vertauscht -> 8 | s2p4 68/0 |
| 5 | karte_transform | Vorzeichen quaternion_for_axis90 + Repeat ohne Klonen -> 6 | s2p5 78/0 |
| 6 | karte_patchwerkzeuge | Insert-Mitte + Normalenrichtung -> 4; Absturz nachgestellt ("die Welt bleibt") | s2p6b 96/0 |
| 7a | karte_texturbefehle | convertScale + Patch_SetTexdef ohne tex2local -> 2 | s2p7 106/0 |
| 7b | karte_texturtags | ohne Textverschmelzung -> 4 | s2p7b 114/0 |
| 7c | karte_texturbefehle | ResampleGamma ohne +0.5, Mip-Basis max statt min, Modusliste ohne Tausch -> 3 (in Punkt 14 nachgeholt) | s2p7e 116/0 |
| 8 | karte_region | Inside-Regel, worldspawn, Autosave >=, SelectBrush -> 7 | s2p8b 141/0 |
| 9 | karte_filter | leerer Pinsel, EXCLUDE_ENT, Flaechen-Caulk, Invert -> 6 | s2p9b 159/0 |
| 10a | karte_ebenen | vier Mutationen -> 17 bzw. 6 | s2p10c 172/0 |
| 10b | karte_modell | t, Reihenfolge, Laengenvorrang, textures/ -> 4 | s2p10d 180/0 |
| 10c | karte_modellbrowser | Rand, z-Gewicht, Wurzel, Gross/klein -> 5 | s2p10e 186/0 |
| 11a | karte_spiel | quantise, "default:", VFS-Reihenfolge, knowngame -> 4; Selbsttest: 14 Mutationen -> 15 | s2p11gruen 47/0 |
| 11b | - | Selbsttest: 8 Mutationen -> 15 (verdeckte einzeln: 3) | s2p11c 29/0, s2p11d 66/0 |
| 11c | - | Selbsttest: 8 Mutationen -> 12 (verdeckte einzeln: 1) | s2p11g 33/0, s2p11h 87/0 |
| 12 | - | Selbsttest befehlsWirkungen: 7 Mutationen -> 17 | 107/0; Gesamtlauf Stufe 2 s2p12e 371/0 |

ctest wuchs von 87 (Stufe 1b) auf 100 Tests; nach jedem Punkt 100 % gruen.

## Punkt 1: Komponenten

Portiert: selection.cpp (ComponentSelectionTestable, Ecken/Kanten/Flaechen
mit `$SELPOINT`/`$DEEPPOINT`, Huelle im Ecken-Modus), brush.cpp
(Vertex/Edge/FaceInstance, Brush::vertex_clip, Texture Vertex Lock),
selection_mtable_extrude.h (DragExtrudeFaces, brush_extrudeDiag),
selection.cpp BuildManipulator (B), UV-Werkzeug (G, selection_mtor_uv).
Neue Dateien `src/karte/komponenten.cpp`, `include/bhed/karte/komponenten.h`.

Abweichungen: die "lingering" Komponentenwahl abgewaehlter Pinsel wird nur
fuer Flaechen gezeichnet; FaceInstance-Wahl folgt der Flaeche (NRC:
Instanzvektor, operator= kopiert nur m_face); `$DEEPPOINT` ueber den
Tiefentest statt einer Okklusionsabfrage je Punkt (teilverdeckte Punkte
gelten schon als verdeckt); UV-Patch-Overlay mit der Normale aus
m_lines2world (GL: Normale durch die Modelview).

## Punkt 2: Clipper

Portiert: selection_mtor_clip.cpp (ClipManipulator ganz), clippertool.cpp
(Clipper_*, Einstellungen, Doppelklick 200 ms), brushmanip.cpp
Scene_BrushSplitByPlane / BrushSplitByPlaneSelected, BrushClipPlane-
Zeichnung, Teile von csg.cpp fuer Wrap-Merge. Seite Clipper: 4 Checkboxen +
Radio (Reset Flipped State haengt an Reset Points on Split).

ctest karte_csg: Clipper 2D/3D, Caulk, Punkte zuruecksetzen, Split, Flip,
3 Punkte in 2D, 2 Punkte -> Huelle. Selbsttest: X mit Status wie NRC,
Clip, Undo "clipperClip", Strg+Return Flip, Umschalt+Return Split,
Doppelklick-Split, Strg+M1 schnell.

## Punkt 3: CSG

Portiert: csg.cpp ganz (Subtract mit Flaechenreihenfolge Z>Y>X und
Gegenflaeche, Merge, Wrap Merge mit QuickHull, Intersect, Make Room,
Hollow Diag/Wrap/Extrude/Pull, Ausschluss -faces/+faces/-proj/-cam,
Shrink/Expand, CSG Tool), brushmanip.cpp Select_MakeDetail/MakeStructural.
Dialog CSG Tool nach Foto (451 x 103, Raster 11/6, SpinBoxLabel "Grid->"
1:1 mit `m_dragAccum += pos - start`, Rad mit Umschalt/Strg).

ctest: Subtract (5 Teile, Reihenfolge, Rauminhalt 229376), Merge inkl. der
NRC-Eigenheit "L aus drei Wuerfeln -> Quadrat", Huelle 6/8 Flaechen,
Intersect, Room, vier Hollow-Arten mit Ausschluss, Versatz, Detail.

Abweichung: AutoCaulk Selected (F4) ist Stufe 3 - NRC ruft dafuer
`q3map2 -autocaulk` auf (BSP, Portale, Flut, autocaulk_write).

## Punkt 4: Grundformen

Portiert: brushmanip.cpp:76-460 (Prism, Cone, Sphere, Rock mit rand(),
Icosahedron mit drawtri + QuickHull, Brush_ConstructPrefab mit Undo-Namen),
:805-816 Scene_BrushConstructPrefab, BrushMakeSided (Strg+3..9),
gtkdlgs.cpp:272-327 DoSides. Masse am Foto: Feld 166 x 22 hinter der
Beschriftung, die versteckte Checkbox laesst ihren Zeilenabstand stehen.
Neue Dateien `src/karte/grundformen.cpp`, `gui/app_karte_formen.cpp`.

## Punkt 5: Transformationen

Portiert: select.cpp:532-660 (Select_FlipAxis, quaternion_for_axis90,
Select_RotateAxis), :1017-1105 (Flip/Rotate nach der Ansicht), :1133-1160
(quaternion_for_euler_xyz_degrees), :1580-1757 (RotateDialog/ScaleDialog mit
eventFilter Esc/Return/Tab/Leertaste, Qt-bound mit Umlauf), selection.cpp
TransformsObserved, repeatTransforms mit Scene_Clone_Selected,
resetTransforms (auch beim Wechsel in den Clipper), mainframe.cpp:1116-1131
(vier dynamische Menuepunkte). Neue Datei `gui/app_karte_transform.cpp`.

## Punkt 6: Patches

Portiert: patch.cpp:323-1590 (Redisperse, Smooth, InsertRemove, MakeCap,
Flip/Translate/Scale/RotateTexture, SetTextureRepeat, CapTexture,
NaturalTexture, ConstructSeam, constructPlane, ConstructPrefab mit allen 12
Vorlagen), :2870-3290 (Thicken), patchmanip.cpp:49-560 und :837-1004
(Dialoge Density, Deform, Cap, Thicken), Seite Patches. Neue Dateien
`src/karte/patchwerkzeuge.cpp`, `src/karte/patchbefehle.cpp`,
`gui/app_karte_patch.cpp`; Kappen-Symbole als Vektorzeichnungen.

Fehler gefunden und behoben (Rot/Gruen): nach Rueckgaengig von Thicken
blieben die geloeschten Patches gewaehlt; die naechste Vorlage loeschte mit
Select_Delete die Welt (Absturz). Nachgestellt in karte_patchwerkzeuge
("die Welt bleibt" rot), behoben durch Karte::auswahlBereinigen nach
Undo/Redo (wie NRC: zerstoerte Instanzen sind abgewaehlt).

## Punkt 7: Texturieren

7a: surfacedialog.cpp (Dialog nach Foto, Increments, Match Grid,
Invert H/V, Fit/Width/Height mit Rechtsklick "Only", Project
Axial/Ortho/Cam, Reset, Seite und Schluessel SI_*), select.cpp
Texdef_Shift/Scale/Rotate (Tasten), Select_FitTexture/ProjectTexture,
patchmanip.cpp PatchTexdefConstructor + Patch_SetTexdef, SelectTextured.
JA/JK2 haben kein `si_flags` - NRC zeigt dann keine Flaggen, wir ebenso.

7b: Tags (libs/xml/xmltextags.cpp als kleines XML-Dokument mit dem
Verhalten von libxml2, texwindow.cpp Tags-Reiter, Liste, Menue, Suche
"[TAGS] ...", Untagged, Copy/Paste). Datei
`<Einstellungen>\karte_<ja|jk2>_shadertags.xml`. Abweichung: NRC speichert
erst beim Beenden, wir nach jeder Aenderung (gleicher Inhalt).

7c: Seite Textures (ResampleGamma, MostDetailedMip, sechs Filtermodi als
D3D11-Abtaster, Anisotropie, Kompression "None"). Abweichungen:
Anisotropie wirkt in D3D11 nur mit Trilinear; Gamma-Spinner Schritt 0.1.

## Punkt 8: Auswahl und Datei

Portiert: select.cpp SelectByBounds (Touching/Inside), Strg+E, map.cpp
Region (alle Varianten, Map_SaveRegion mit Waenden und Startpunkt),
Map_SaveSelected, ImportMap, DoMapInfo, DoFind, autosave.cpp (mit
Snapshots), EverySecondTimer. Node::visible um "ausgeschlossen" erweitert,
alle 74 lesenden Stellen auf `sichtbar()` umgestellt. Neue Dateien
`src/karte/region.cpp`, `src/karte/kartendatei.cpp`, `gui/app_karte_datei.cpp`.

Abweichungen: Import einer .bsp (q3map2 -convert) ist Stufe 3; Map Info ist
nicht in der Groesse veraenderbar; Textbreiten ueber GDI Tahoma 8 pt (Qt
misst 1-2 Punkte breiter: Find Brush Feld ab 84 statt 86, Map Info 232 statt
241 breit).

## Punkt 9: Filter

Portiert: filters.cpp (Maske SI_Exclude, 22 Befehle mit Tasten, Invert,
Reset), brushmanip.cpp/brush.cpp alle Pinsel- und Flaechenregeln,
patchmanip.cpp, entity.cpp Entitaetsfilter, filterbar.cpp Rechtsklick
(CommonFunc_tex mit Umlauf, CommonFunc_command, CommonFunc_group).
Gefilterte Flaechen fehlen in der Kamera und in der Auswahl; das
2D-Drahtgitter bleibt (wie NRC).

## Punkt 10: Ebenen, misc_model, Modell-Browser

10a: libs/layers.h, layerswindow.cpp ganz (Kontextmenue, Umschalt+Q,
Strg+Q, Ziehen und Ablegen nach Qts dropEvent, Knoepfe). NRC hat fuer Ebenen
kein Rueckgaengig (layerswindow.cpp:61 "? undo") - so uebernommen.

10b: nullmodel.cpp, referencecache.cpp, md3model, pm_ase.c +
picointernal.c, miscmodel.cpp (Remap, angle/angles/modelscale/
modelscale_vec, Pivot, Name), Treffer auf Dreiecke. Ueber NRC hinaus (wie
im Spiel): .glm in Grundhaltung mit model_default.skin. Abweichungen:
Refresh models laedt alle Modelle neu (keine Aenderungszeiten im
Dateisystem); ASE ueber pm_ase.c (NRC 20260114: assimp, gleiche Namen).

10c: modelwindow.cpp ganz (Baum, Zellen, Licht, Drehen, Rollen,
entityAssignModel, insertModel), Seite Model Browser. Abweichung: der Baum
listet md3/ase/glm (was der Editor laden kann).

## Punkt 11: Einstellungen, Spiel, Menues

11a: Settings (NudgeAfterClone, LoadLastMap), Undo (UndoLevels), Brush
(TexdefDefaultScale, BrushType, Always caulk, Snap plane points), Grid
(Default, Max coordinate), Orthographic (NoStipple mit 0xAAAA/3 im
2D-Shader, ChaseMouse, 2DZoomInToPointer, XYMSAA), Build (Engine, Argumente,
DefaultableString "default:", Region Box Shader). Neue Datei
`src/karte/spielbeschreibung.cpp` (ja.game/jk2.game, gamecombo_*,
QE_InitVFS, DirectoryCleaned).

11b: preferences.cpp (LatchedValue "Selected Game", Neustart-Frage,
gamefile/gamePrompt, Dialog "Global Preferences"), mainframe.cpp
(setEnginePath, Extra Resource Paths 0..4, gamemode_set, Radiant_Restart),
gtkdlgs.cpp (Select mod, fs_game, Mapping mode), QSpinBox
AdaptiveDecimalStep fuer alle ganzzahligen Spinner.

11c: QTornOffMenu (Qt 5.15 qmenu.cpp), MainFrame_toggleFullscreen (F11),
OpenGLFont_select mit QFontDialog-Nachbau ("Select Font"), Schrift der 2D-,
3D- und Texturbrowser-Ansicht, Background image (xywindow.cpp:1104-1216),
OpenBugReportURL, DoAbout (391 x 488), PathsDialog "Engine Path
Configuration", DoShaderView mit "Shader Editor Command".

## Punkt 12: Selbsttest, Fotos, Zeiten

`ausfuehren()` merkt jeden ausgeloesten Befehl; `befehle::stufe2()`
(aus `tools/karte/gen_befehle.py` STUFE_2_FERTIG, 145 Befehle). Der Schluss
der Stufe-2-Schritte prueft, dass jeder davon ausgeloest wurde (Taste,
Menue, Knopf oder ausfuehren). Der erste Lauf fand 79, die bis dahin nur
ueber den Kern geprueft waren; dafuer `stufe2::befehlsWirkungen` (107
Pruefungen mit Wirkung, z. B. Brush N sided = N+2 Flaechen, Rotate X/Y/Z
tauscht die Ausdehnung, alle Patch-Vorlagen, Insert/Delete Row/Column,
Tex Shift/Scale/Rotate je auf der richtigen Achse, alle Filter an einem
passenden Objekt). Dazu `stufe1Reste` (25/0): versteckte Knoepfe Open,
Save, Change views, zuletzt geoeffnet 2 bis 9, Texture Browser in Split.
Gesamtlauf Stufe 2 (s2p12e): 371 OK, 0 FEHLER in 104 s, "145 Befehle,
nicht ausgeloest: -".

### Zeiten auf duel_mos_espa (824 Entities, 31671 Pinsel, 6820 Patches)

| Messung | Wert |
|---|---|
| Laden | 742 ms |
| CSG Subtract mit einem Pinsel aus der Mitte | 8.7 ms (31671 -> 31673 Pinsel), laengstes der 6 Bilder danach 2.9 ms |
| Rueckgaengig danach | 238 ms |
| Clip (Ebene durch die Mitte) | 6.0 ms, Bild danach 2.4 ms |
| Ecken-Modus, 1 Pinsel | < 0.01 ms, Bild 2.8 ms |
| Ecken-Modus, 1000 Pinsel | < 0.1 ms, Bild 3.5 ms |
| Editor je Bild | 2.30 ms Mittel, 95 % 4.93 ms |
| Bild nach Auswahlwechsel | 7.96 ms |

### Fotos gegen NRC

Unsere Fotos: Selbsttest-Lauf s2p12e (`SP\s2\fotos\karteneditor_*.bmp`,
ganzer Bildschirm, Massstab 2); NRC: `SP\nrc_fotos\dialoge` (Qt-Massstab 2,
mit Windows-Rahmen). Gemessen mit `SP\s2\fotovergleich.py`: bei uns die
Innenflaeche unter der blauen Titelleiste bis zum Rahmen, bei NRC die Flaeche
in der Dialogfarbe (Rahmen 11, Titel 45 Bildpunkte). Werte in Bildpunkten
(2 je Qt-Punkt).

| Dialog | unser Client | NRC Client | Abweichung | Grund |
|---|---|---|---|---|
| CSG Tool | 900 x 205 | 902 x 206 | -2 / -1 | Rundung |
| Prism (Sides) | 444 x 155 | 448 x 156 | -4 / -1 | Textbreite "Sides:" (GDI-Tahoma 1-2 Punkte schmaler als Qt) |
| Icosahedron (Subdivisions, Truncate) | 510 x 195 | 516 x 196 | -6 / -1 | Textbreite |
| Arbitrary rotation | 440 x 199 | 448 x 200 | -8 / -1 | Textbreiten der Beschriftungen |
| Arbitrary scale | 464 x 199 | 472 x 200 | -8 / -1 | Textbreiten |
| Simple Patch Mesh (Density) | 456 x 199 | 460 x 200 | -4 / -1 | Textbreite |
| Exact Cylinder | 456 x 251 | 460 x 252 | -4 / -1 | Textbreite |
| Deform | 508 x 195 | 512 x 196 | -4 / -1 | Textbreite |
| Thicken | 486 x 247 | 508 x 248 | -22 / -1 | die Radiozeile X/Y/Z/Normal ist bei Qt 174 Punkte breit und bestimmt die Feldspalte; mit unseren vier etwas schmaleren Texten bleibt sie unter 166, das Feld behaelt 166 |
| Surface Inspector | 582 x 727 | 570 x 728 | +12 / -1 | Spalte Fit/Axial/Natural 51 statt 43.5 Punkte (Knopfbreite aus unserer Textmessung) |
| Find brush | 520 x 199 | 526 x 200 | -6 / -1 | Textbreite (Feld ab 84 statt 86, Punkt 8) |
| Map Info | - | - | - | inhaltsabhaengig: NRCs Foto zeigt eine andere Karte (26 Entities, lange Klassennamen) |
| About | 780 x 975 | 782 x 976 | -2 / -1 | Rundung (Logo 128, Text 145, Knoepfe 80 x 22 alle 28 wie das Foto) |
| Preferences (alle Seiten) | 1192 x 983 | 1194 x 984 | -2 / -1 | Rundung |

Einstellungsseiten, Zeilenabstaende von Hand an den Fotos gemessen (Punkt
11a/11b): Brush 56 / 54 / 52, Build 52, 52, 54, 56, 56, 56, 54, 54, Paths je
56, Orthographic und Game wie das Foto - gleich wie NRC. Settings, Undo,
Grid, Autosave, Model Browser, Interface: Felder, Reihenfolge und Vorgaben
wie die Fotos `pref_<Seite>.png`; Global und Display sind wie bei NRC leer.
Ebenen- und Modell-Browser: Zeilen 22, Spalten und Knoepfe nach den Fotos
`dlg_View_Layers_Browser_0.png` / `dlg_View_Model_Browser_0.png` (Punkt 10a,
10c). Ohne NRC-Foto (aus den Qt-Massen gebaut): Cap, Select Font, Engine Path
Configuration, Global Preferences, die Neustart-Frage, die Tag-Rueckfrage;
unsere Fotos `karteneditor_patch_cap`, `_select_font`, `_engine_path`,
`_global_preferences`, `_neustart`, `_tags_frage`.

## Punkt 13: Handbuch

Neue Seite "Karten-Editor: Bauwerkzeuge" (`tools/handbuch/inhalt_karteneditor2.py`,
id `karteneditor_bauen`) in vier Sprachen: Ecken/Kanten/Flaechen, Clipper,
CSG, Formen und Transformationen, Patches, Texturieren, Auswahl/Regionen/
Dateien, Filter, Ebenen, Modelle, Einstellungen (Spiel, Mod, Mapping mode,
Pfade, Neustart), Menues und Fenster - mit den Tasten von NRC und 11 neuen
Bildern je Sprache aus `karteneditorHandbuchSchritte()` (Laeufe hb2en,
hb2de, hb2zhHans, hb2ja, je 1 OK / 0 FEHLER; `tools/handbuch/bilder.py`).
Die Seite "Der Karten-Editor" sagt jetzt, dass nur noch q3map2 und die
Plugins ausgegraut sind, und nennt Preferences > Game / Paths.
`gen_handbuch.py` liest dafuer auch `i18n_karte2.KARTE2`. Alle Aussagen
vorher im Code nachgesehen (Clipper: bis zu drei Punkte in 2D, in der
Kamera auf Flaechen - ClipManipulator::testen; Filter-Rechtsklick nach
FILTER in `tools/karte_menues.py`; Ebenen Umschalt+Q / Strg+Q; Modell-
Browser Klick/Doppelklick). `gen_handbuch.py`: 135 Seiten x 4 Sprachen,
`--pruefen`: handbuch_sprachen ok.

## Punkt 14: Abschluss der Stufe 2

### Tests am Ende

Ein Testprozess zur Zeit, Selbsttests mit Prioritaet Idle und dem
8-GB-Waechter (nie ausgeloest, hoechste Spitze 1.5 GB in spulen); Skripte
`SP\s2\ende_s2.sh`, `nach_s2.sh`, `nach2_s2.sh`.

| Test | Ergebnis |
|---|---|
| sauberer Neubau (Release, `--clean-first`) | 0 Fehler; die zwei Warnungen aus Stufe 2 (app_karte_xy.cpp verdecktes `o`, selbsttest.cpp verworfener `[[nodiscard]]`, dazu C4244 im Tupel der Drehpruefung) behoben, uebrig nur Warnungen aelterer Stande |
| ctest | 100 / 100 |
| Karten-Editor voll mit grosser Karte (e3ke) | 783 OK / 0 FEHLER (242 s, Spitze 1.0 GB); "145 Befehle, nicht ausgeloest: -"; Checkliste "290 fertige Befehle, 3 Menue-Rufe, 282 nachgebaut; es fehlt: -; zu viel: -" |
| Karten-Editor nur Stufe 2, Stand des Commits (e4s2) | 371 / 0 |
| Modus 1 (Gesamttest) | 2645 / 0 (435 s, Spitze 582 MB) - wie rc605 |
| karte | 227 / 0 (nach der Korrektur unten; vorher 226 / 1) - wie rc605 |
| kartenfunde, fix604b, laserfalle, geschuetzanim, zielstrahl, effektgrund, figurdeform, jk2, tuerlauf | 53, 32, 8, 3, 2, 1, 1, 15, 13 OK / je 0 FEHLER |
| kamera, missionsbau, szenen, ton, npcklasse | 95, 95, 26, 34, 12 / je 0 |
| gluehen, gras, spulen, figurstufen, missionsliste, rest605 | 8 (nach der Korrektur), 13, 13, 8, 11, 6 / je 0 |
| Leistung (md_am_sith, BHED_D3D_DEBUG=0, rc605-Bau aus Y:\integ und unser Bau abwechselnd, je zweimal) | Vorbereitung 6.79 / 6.24 s (rc605) gegen 6.38 / 6.46 s; intro2_sith Mittel 14 / 14 gegen 13 / 15 ms (max 177 / 227 gegen 165 / 169); Flug 8.2 / 7.9 gegen 7.6 / 7.7 ms; zweiter Durchgang 11 / 10 gegen 10 / 11 ms - kein Unterschied |
| Kartenpruefung (278 Karten, Movie Duels, gegen das rc605-Schlussaudit `f5audit`) | alle Summen gleich: abgestuerzt/haengend, nicht geladen, GPU-Fehler, D3D-Meldungen, Protokollfehler 0 -> 0; nicht gezeichnete Entities 1 -> 1; NPC ohne Modell 2 -> 2; Weltshader-Luecken 0 -> 0; fehlende efx/Klaenge/Texturen/Modelle 27/29/139/5 unveraendert (593 s) |
| grosse Karte duel_mos_espa (e3ke) | Laden 661 ms; Editor je Bild 2.05 ms (95 % 2.41); Bild nach Auswahlwechsel 6.97 ms; Neubauten Geometrie 0 / Rest 10; CSG Subtract 7.4 ms, Bild danach 2.4 ms, Rueckgaengig 145 ms; Clip 6.4 ms, Bild danach 3.3 ms; Ecken-Modus 1 Pinsel < 0.01 ms (Bild 2.5 ms), 1000 Pinsel < 0.1 ms (Bild 5.5 ms); Speicher privat +934 MB, Grafik +523 MB. Ueber drei Laeufe schwankten die Bildzeiten (Idle-Prioritaet): CSG 7.4-11.4 ms, Clip 6.4-9.7 ms, Bild mit 1000 Pinseln im Ecken-Modus 5.5-11.2 ms |
| Handbuch | `gen_handbuch.py --pruefen`: handbuch_sprachen ok |

In der Schlussrunde gefunden und behoben (Rot/Gruen):

| Punkt | Rot (vorher) | Gruen (nachher) |
|---|---|---|
| Pruefung "Checkliste: jeder Befehl der Stufe 1b ist nachgebaut, sonst keiner" (Stufe 1b) kannte die Menue-Rufe About, Bug report und Set 2D Background image nicht (MenueArt::Ruf, in NRC FreeCaller ohne Befehlsnamen; seit 11c nachgebaut). Lief in den Stufe-2-Laeufen (BHED_KARTE_NUR=stufe2) nicht mit. Jetzt: jeder fertige Befehl (Stufe 1b und 2) und jeder Menue-Ruf nachgebaut, sonst keiner | e2ke: 782 / 1, "zu viel: DoAbout OpenBugReportURL WXY_SetBackgroundImage" | e2ke2, e3ke: 783 / 0, "zu viel: -" |
| Pruefung "Gluehen Figur: rotes Auge von sbd1 liegt im Bild" (Modi karte und gluehen, rc604): Schwelle 30, rc605 hatte 31. Die Szene ist seit Stufe 1b 1359 statt 1384 Bildpunkte breit - die Modusleiste ist so breit wie ihre laengste Beschriftung, und das ist seit dem Eintrag "Map Editor" dieser; die Figur wird mit festem fov_x kleiner (29 Punkte). Nachgestellt mit dem rc605-Bau (zweimal 1384 / 31 / gruen), unserem Bau (zweimal 1359 / 29 / rot) und dem Bau der Stufe 1b aus k14a (1359 / 29 / rot). Schwelle auf 20 gesenkt, Begruendung im Code | karte 226 / 1, gluehen 7 / 1 | karte 227 / 0, gluehen 8 / 0 |
| Punkt 7c hatte keinen dokumentierten Mutationstest: nachgeholt (ResampleGamma ohne +0.5, Mip-Basis max statt min, Modusliste ohne Tausch) | karte_texturbefehle 3 Fehlschlaege | 0 |

Offene Punkte nach Stufe 2:

- Textbreiten der Dialoge ueber GDI-Tahoma 11 Bildpunkte: Qt misst bei
  Massstab 2 je Text 1-2 Punkte breiter. Folgen: Thicken 11 Punkte, die
  meisten kleinen Dialoge 2-4 Punkte schmaler, Surface Inspector 6 Punkte
  breiter (Knopfspalte). Abhilfe waere eine Textmessung wie Qt (Schrift in
  Geraetegroesse messen und zurueckrechnen) fuer alle Dialoge auf einmal.
- Exit bleibt im Selbsttest ungeprueft (beendet behaved).
- Windows-Dialoge (Datei- und Ordnerwahl) werden im Selbsttest ersetzt,
  nicht angeklickt.

### Checkliste (`tools/karte/nrc_inventar.md`)

Gesamt: 720 fertig, 152 ausgegraut, 2 offen von 874 Eintraegen
(Stufe 1b: 329 / 452 / 93).

| Abschnitt | fertig | ausgegraut | offen |
|---|---|---|---|
| Menues | 229 | 7 | 1 |
| Werkzeugleisten | 54 | 0 | 0 |
| Registrierte Befehle | 320 | 75 | 1 |
| Dialoge und Fenster | 21 | 3 | 0 |
| Einstellungsseiten | 17 | 0 | 0 |
| Einstellungsfelder | 79 | 0 | 0 |
| Plugins | 0 | 67 | 0 |

Offen: nur Exit (Menue und Befehl) - nachgebaut, beendet behaved mit
dessen Rueckfragen, im Selbsttest nicht ausfuehrbar. Ausgegraut (Stufe 3):
AutoCaulk Selected, Pointfile, Next/Previous leak spot, Build > Customize /
Run recent build, Check for NetRadiant update, Manual (F1), alle
Plugin-Befehle, die Dialoge "Build Menu" und "Q3Map debug window";
"Install Map Developer's Files" bleibt ausgegraut, weil ja.game/jk2.game
keinen install-Ordner haben (NRC zeigt dann ebenfalls nichts an).

### Portierte Dateien (NRC 20260114 -> unsere Datei)

| unsere Datei | portiert aus |
|---|---|
| `src/karte/komponenten.cpp` | radiant/selection.cpp (Komponententests), brush.cpp/.h (Vertex/Edge/FaceInstance, vertex_clip), selection_mtable_extrude.h |
| `src/karte/bearbeitung.cpp` (erweitert) | selection_mtor_clip.cpp, selection_mtor_uv.cpp, selection.cpp (BuildManipulator, TransformsObserved, repeat/resetTransforms), clippertool.cpp |
| `src/karte/csg.cpp` | radiant/csg.cpp, brushmanip.cpp (MakeDetail/Structural, Split by plane) |
| `src/karte/grundformen.cpp` | radiant/brushmanip.cpp:76-460, 805-816 |
| `src/karte/patchwerkzeuge.cpp`, `patchbefehle.cpp` | radiant/patch.cpp:323-1590, 2870-3290, patchmanip.cpp:49-560 |
| `src/karte/texturbefehle.cpp` | radiant/select.cpp (Texdef_*, Fit, Project), surfacedialog.cpp, patchmanip.cpp (PatchTexdefConstructor), patchdialog.cpp, textures.cpp |
| `src/karte/texturtags.cpp` | libs/xml/xmltextags.cpp (mit dem Verhalten von libxml2) |
| `src/karte/region.cpp`, `kartendatei.cpp` | radiant/map.cpp (Region, SaveRegion/Selected, Import, MapInfo, Find), brushmanip.cpp ConstructRegionBrushes, autosave.cpp |
| `src/karte/filter.cpp` | radiant/filters.cpp, brushmanip.cpp, brush.cpp, patchmanip.cpp, entity.cpp |
| `src/karte/ebenen.cpp` | libs/layers.h, radiant/layerswindow.cpp |
| `src/karte/modell.cpp` | plugins/model (nullmodel.cpp, md3model), referencecache.cpp, libs/picomodel (pm_ase.c, picointernal.c), plugins/entity/miscmodel.cpp |
| `src/karte/modellbrowser.cpp` | radiant/modelwindow.cpp |
| `src/karte/spielbeschreibung.cpp` | games/ja.game, jk2.game, radiant/gtkdlgs.cpp (gamecombo_*), qe3.cpp (QE_InitVFS), watchbsp.cpp (DefaultableString, Engine-Argumente), mainframe.cpp |
| `src/karte/befehle_stufe2.inc` | erzeugt von tools/karte/gen_befehle.py (STUFE_2_FERTIG) |
| `gui/app_karte_csg.cpp` | radiant/csg.cpp:1372-1588, libs/gtkutil/spinbox.h |
| `gui/app_karte_formen.cpp` | radiant/gtkdlgs.cpp:272-327 |
| `gui/app_karte_transform.cpp` | radiant/select.cpp:1580-1757 |
| `gui/app_karte_patch.cpp` | radiant/patchmanip.cpp:837-1004, libs/gtkutil/dialog.cpp |
| `gui/app_karte_si.cpp` | radiant/surfacedialog.cpp |
| `gui/app_karte_tags.cpp` | radiant/texwindow.cpp:1267-1575 |
| `gui/app_karte_datei.cpp` | radiant/map.cpp:1855-2131, gtkdlgs.cpp |
| `gui/app_karte_filter.cpp` | radiant/filters.cpp, filterbar.cpp |
| `gui/app_karte_ebenen.cpp` | radiant/layerswindow.cpp:192-564, Qt 5.15 qtreewidget.cpp, qabstractitemview.cpp, qinputdialog.cpp |
| `gui/app_karte_modelle.cpp` | radiant/referencecache.cpp, plugins/entity/model.h, miscmodel.cpp |
| `gui/app_karte_modellbrowser.cpp` | radiant/modelwindow.cpp:527-1386 |
| `gui/app_karte_dialoge.cpp` (erweitert) | radiant/preferences.cpp, brushmodule.cpp, grid.cpp, undo.cpp, xywindow.cpp, watchbsp.cpp, mainframe.cpp (Paths, About), Qt 5.15 qfontdialog.cpp, qspinbox.cpp |
| `gui/app_karte_qtmenue.cpp` (erweitert) | Qt 5.15 qmenu.cpp (QTornOffMenu) |
| `gui/app_karte_xy.cpp` (erweitert) | radiant/xywindow.cpp (Stipple, ChaseMouse, Background image) |

### Was Stufe 3 braucht

- q3map2 1:1 (BSP, VIS, LIGHT) mit dem Build-Menue (Customize, Run recent
  build, "Build Menu"-Dialog), dem Fenster "Q3Map debug window"
  (feedback.cpp), Pointfile und Leak-Spots (points.cpp).
- AutoCaulk Selected (q3map2 -autocaulk) und Import einer .bsp
  (q3map2 -convert, tryDecompile).
- Plugins: bobToolz, brushexport2, meshTex, prtview, shaderPlug (Create tag
  file), sunPlug, uFO:AI.
- Check for NetRadiant update, Manual (F1).
- Die Build-Einstellungen (Engine, Argumente, Region Box Shader) sind schon
  gespeichert und werden dann benutzt.

## Abweichungen und Gruende

- Das Spiel des Editors folgt ohne `gamefile` dem Regelwerk des Programms
  (NRC fragt beim ersten Start); ohne Engine Path gilt der Ordner ueber dem
  ersten Spielordner des Programms, gesucht wird in den Spielordnern des
  Programms (Movie Duels vor base). Radiant_Restart laedt den Editor mit dem
  neuen Spiel und der Karte neu, statt das Programm neu zu starten. Engine
  Path und Mapping mode fragen vor dem Neuladen nach ungesicherten
  Aenderungen (bei NRC ist dieses ConfirmModified `#if 0`).
- Detachable Menus gilt sofort (NRC: LatchedBool, nach Neustart).
- About zeigt das Direct3D-Geraet statt OpenGL; "About Qt" wird "About Dear
  ImGui"; "Changelog" oeffnet die Aenderungen des Programms; das Logo ist eine
  eigene Vektorzeichnung der NRC-Sonne.
- Select Font: Schriftsystem nur "Any"; Stile aus den Registry-Dateien. Der
  Modell-Browser behaelt die Oberflaechenschrift.
- Neue Karten sind wie NRC Brush Primitives; ChaseMouse laeuft je Bild statt
  alle 4 ms (gleicher Faktor ms/10); "Load last map" gilt beim ersten Bild des
  Editors.
- Tags werden nach jeder Aenderung gespeichert (NRC: beim Beenden).
- Anisotropie wirkt in D3D11 nur mit Trilinear.
- Refresh models laedt alle Modelle neu; ASE ueber pm_ase.c statt assimp;
  der Modell-Browser listet nur md3/ase/glm; .glm-Modelle werden (wie im
  Spiel) mit model_default.skin gezeigt.
- Map Info nicht in der Groesse veraenderbar; Textbreiten 1-2 Punkte
  schmaler als Qt (GDI-Tahoma).
- Komponenten: siehe Punkt 1 (lingering-Wahl, `$DEEPPOINT` ueber Tiefentest).
- Windows-Dialoge (Dateiwahl, Ordnerwahl) werden im Selbsttest nicht
  angeklickt; der Weg dahinter wird mit ersetzter Dateiwahl geprueft.

## Changelog

## English

- Map editor: vertices, edges and faces can be dragged (V, E, F) like in
  NetRadiant-Custom, faces can be extruded (Ctrl+Alt+drag), with the build
  tool (B), the UV tool (G) and Texture Lock (Shift+T).
- Map editor: the clipper (X) - up to three points in a 2D view or on
  surfaces in the camera, clip, split and flip (Enter, Shift+Enter,
  Ctrl+Enter), double click, "Clipper" preference page.
- Map editor: CSG Subtract, Merge, Wrap Merge, Intersect, Make Room, Make
  Detail / Structural and the CSG Tool (hollow, shrink, expand).
- Map editor: brush shapes (prism, cone, sphere, rock, icosahedron, 3 to 9
  sides with Ctrl+3 ... 9), rotate and mirror, Arbitrary rotation and scale,
  Repeat and Reset Transforms.
- Map editor: the whole Curve menu - every patch shape, insert and delete
  rows and columns, invert, transpose, redisperse, smooth, cap, thicken,
  deform and the patch texture commands.
- Map editor: Surface Inspector (S) and the texture keys (shift, scale,
  rotate, fit, project), Select Textured, tags in the texture browser and
  the "Textures" preference page (gamma, mipmaps, filtering, anisotropy).
- Map editor: Select Inside / Touching, regions, Save Selected, Save Region,
  Import, Map Info, Find Brush, autosave and snapshots.
- Map editor: filters in the View menu and the filter bar, with
  NetRadiant-Custom's right-click actions.
- Map editor: layers (Layers Browser, move to layer, hide and show,
  nesting), misc_model models (md3, ase, glm) in the 2D views and the
  camera, and the Model Browser.
- Map editor: every preference page works, including Game (Jedi Academy or
  Jedi Outcast, mod, mapping mode) and Paths (game folder and five extra
  folders) with the restart question, Settings, Undo, Brush, Grid,
  Orthographic and Build.
- Map editor: tear-off menus, Fullscreen (F11), a font for the 2D and camera
  views, a 2D background image for tracing, the About dialog, the bug report
  link and a custom shader editor.
- Map editor: the self-test runs every new command and checks what it does;
  on the largest Movie Duels map CSG Subtract takes 9 ms and a clip 6 ms.
- Guide: new page "Map editor: building tools" in all four languages.

## Deutsch

- Karten-Editor: Ecken, Kanten und Flaechen lassen sich wie in
  NetRadiant-Custom ziehen (V, E, F), Flaechen herausziehen (Strg+Alt+Ziehen),
  dazu das Build-Werkzeug (B), das UV-Werkzeug (G) und Texture Lock
  (Umschalt+T).
- Karten-Editor: der Clipper (X) - bis zu drei Punkte in einer 2D-Ansicht
  oder auf Flaechen in der Kamera, Schneiden, Teilen und Umdrehen (Enter,
  Umschalt+Enter, Strg+Enter), Doppelklick, Einstellungsseite "Clipper".
- Karten-Editor: CSG Subtract, Merge, Wrap Merge, Intersect, Make Room,
  Detail / Structural und das CSG Tool (aushoehlen, schrumpfen, erweitern).
- Karten-Editor: Brush-Formen (Prisma, Kegel, Kugel, Fels, Ikosaeder, 3 bis
  9 Seiten mit Strg+3 ... 9), Drehen und Spiegeln, Arbitrary rotation und
  scale, Transformationen wiederholen und zuruecksetzen.
- Karten-Editor: das ganze Curve-Menue - alle Patch-Formen, Zeilen und
  Spalten einfuegen und loeschen, umkehren, transponieren, neu verteilen,
  glaetten, Kappen, Dicke, Verformen und die Patch-Texturbefehle.
- Karten-Editor: Surface Inspector (S) und die Texturtasten (verschieben,
  skalieren, drehen, einpassen, projizieren), Select Textured, Tags im
  Texturbrowser und die Einstellungsseite "Textures" (Gamma, Mipmaps,
  Filter, Anisotropie).
- Karten-Editor: Select Inside / Touching, Regionen, Save Selected, Save
  Region, Import, Map Info, Find Brush, automatisches Sichern und Snapshots.
- Karten-Editor: Filter im Menue View und in der Filterleiste, mit den
  Rechtsklick-Aktionen von NetRadiant-Custom.
- Karten-Editor: Ebenen (Ebenen-Browser, in Ebene verschieben, aus- und
  einblenden, verschachteln), misc_model-Modelle (md3, ase, glm) in den
  2D-Ansichten und der Kamera und der Modell-Browser.
- Karten-Editor: alle Einstellungsseiten wirken, auch Game (Jedi Academy
  oder Jedi Outcast, Mod, Bearbeitungsmodus) und Paths (Spielordner und fuenf
  weitere Ordner) mit der Neustart-Frage, Settings, Undo, Brush, Grid,
  Orthographic und Build.
- Karten-Editor: abreissbare Menues, Vollbild (F11), eine Schrift fuer die
  2D- und Kamera-Ansichten, ein 2D-Hintergrundbild zum Nachzeichnen, der
  About-Dialog, der Link fuer Fehlerberichte und ein eigener Shader-Editor.
- Karten-Editor: der Selbsttest loest jeden neuen Befehl aus und prueft seine
  Wirkung; auf der groessten Movie-Duels-Karte dauert CSG Subtract 9 ms und
  ein Schnitt 6 ms.
- Handbuch: neue Seite "Karten-Editor: Bauwerkzeuge" in allen vier Sprachen.

## 中文

- 地图编辑器:像 NetRadiant-Custom 一样拖动顶点、边和面(V、E、F),挤出面(Ctrl+Alt+拖动),并提供建造工具(B)、UV 工具(G)和纹理锁定(Shift+T)。
- 地图编辑器:裁剪工具(X)——在二维视图中最多三个点或在摄像机中点击表面,切割、分割和翻转(Enter、Shift+Enter、Ctrl+Enter),双击操作,首选项页"Clipper"。
- 地图编辑器:CSG 减去、合并、包裹合并、相交、生成房间、设为细节/结构,以及 CSG 工具(挖空、收缩、扩展)。
- 地图编辑器:笔刷形状(棱柱、圆锥、球体、岩石、二十面体,Ctrl+3 ... 9 生成 3 到 9 边),旋转和镜像,任意旋转和缩放,重复和重置变换。
- 地图编辑器:完整的 Curve 菜单——所有曲面片形状,插入和删除行列,反转、转置、重新分布、平滑、封盖、加厚、变形以及曲面片纹理命令。
- 地图编辑器:表面检查器(S)和纹理按键(移动、缩放、旋转、适配、投影),选择同纹理,纹理浏览器标签,以及首选项页"Textures"(伽马、Mipmap、过滤、各向异性)。
- 地图编辑器:选择内部/接触、区域、保存所选、保存区域、导入、地图信息、查找笔刷、自动保存和快照。
- 地图编辑器:View 菜单和过滤器栏中的过滤器,带有 NetRadiant-Custom 的右键操作。
- 地图编辑器:图层(图层浏览器、移到图层、隐藏和显示、嵌套),二维视图和摄像机中显示 misc_model 模型(md3、ase、glm),以及模型浏览器。
- 地图编辑器:所有首选项页都可用,包括 Game(Jedi Academy 或 Jedi Outcast、模组、制图模式)和 Paths(游戏文件夹和五个附加文件夹)及重启询问,以及 Settings、Undo、Brush、Grid、Orthographic 和 Build。
- 地图编辑器:可撕下的菜单、全屏(F11)、二维和摄像机视图的字体、用于描摹的二维背景图像、关于对话框、错误报告链接和自定义着色器编辑器。
- 地图编辑器:自检会执行每个新命令并检查其效果;在最大的 Movie Duels 地图上,CSG 减去需要 9 毫秒,一次裁剪需要 6 毫秒。
- 手册:新增四种语言的"地图编辑器:建造工具"页面。

## 日本語

- マップエディター:NetRadiant-Custom と同じように頂点・エッジ・面をドラッグでき(V、E、F)、面の押し出し(Ctrl+Alt+ドラッグ)、ビルドツール(B)、UV ツール(G)、テクスチャロック(Shift+T)があります。
- マップエディター:クリッパー(X)- 2D ビューで最大 3 点、またはカメラで面をクリックし、切断・分割・反転(Enter、Shift+Enter、Ctrl+Enter)、ダブルクリック、設定ページ「Clipper」。
- マップエディター:CSG の減算・結合・ラップ結合・交差・部屋の作成・ディテール/ストラクチャル、CSG ツール(くり抜き、縮小、拡大)。
- マップエディター:ブラシの形(角柱、円錐、球、岩、二十面体、Ctrl+3〜9 で 3〜9 辺)、回転と反転、任意の回転と拡大縮小、変形の繰り返しとリセット。
- マップエディター:Curve メニューのすべて - すべてのパッチ形状、行と列の挿入・削除、反転、転置、再分配、平滑化、キャップ、厚み付け、変形、パッチのテクスチャコマンド。
- マップエディター:サーフェスインスペクター(S)とテクスチャキー(移動、拡大縮小、回転、フィット、投影)、同じテクスチャを選択、テクスチャブラウザーのタグ、設定ページ「Textures」(ガンマ、ミップマップ、フィルタリング、異方性)。
- マップエディター:内側/接触を選択、リージョン、選択範囲を保存、リージョンを保存、インポート、マップ情報、ブラシ検索、自動保存とスナップショット。
- マップエディター:View メニューとフィルターバーのフィルター。NetRadiant-Custom と同じ右クリック操作付き。
- マップエディター:レイヤー(レイヤーブラウザー、レイヤーへ移動、表示/非表示、入れ子)、2D ビューとカメラでの misc_model モデル(md3、ase、glm)、モデルブラウザー。
- マップエディター:すべての設定ページが機能します。Game(Jedi Academy または Jedi Outcast、Mod、マッピングモード)と Paths(ゲームフォルダーと 5 つの追加フォルダー)と再起動の確認、Settings、Undo、Brush、Grid、Orthographic、Build。
- マップエディター:切り離せるメニュー、全画面(F11)、2D ビューとカメラのフォント、なぞり描き用の 2D 背景画像、About ダイアログ、バグ報告のリンク、独自のシェーダーエディター。
- マップエディター:セルフテストが新しいコマンドをすべて実行して効果を確認します。最大の Movie Duels マップで CSG 減算は 9 ms、切断は 6 ms です。
- ガイド:4 言語で新しいページ「マップエディター:作成ツール」。
