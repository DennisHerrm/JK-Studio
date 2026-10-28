# Karten-Editor, Stufe 4: fertige Karten oeffnen, Originallicht, alle Plugins

Stand: Y:\karte, Zweig `karte1` (auf Stufe 3, `f7334a9`). Vorbild ist
NetRadiant-Custom 20260114 (NRC, Git-Tag `20260114` in
`SP\nrc_src\netradiant-custom`, ausgepackt nach `SP\s4\tag`; Binaries in
`SP\nrc_dl\nrc`). Quellangaben in den Kommentaren: `Funktion (datei:zeile)`.

`SP` = `C:\Users\shank\AppData\Local\Temp\claude\c--Users-shank-Downloads-behaved-1-0-0-rc567-behaved\af9c6614-e078-4efa-a574-1fa901786464\scratchpad`

Regeln wie in Stufe 3: je Punkt ein gezielter Rot/Gruen-Test, ctest, ein
Commit je Punkt; radiant.exe nie gestartet; ein Testprozess zur Zeit mit
Prioritaet Idle (`SP\s2\idle_karte.sh`, 8-GB-Waechter).

## Punkt 0a: Absturz 0xC000001D im Modus `karte` (ASan)

Methode: eigener ASan-Bau `Y:\karte\build-asan` (MSVC `/fsanitize=address /Zi
/bigobj`, `clang_rt.asan_dynamic-x86_64.dll` und `llvm-symbolizer.exe` neben
der exe, `ASAN_OPTIONS=log_path=asan.log:halt_on_error=1`), Selbsttest ueber
`SP\s4\asan_lauf.sh` (Idle, 8-GB-Waechter, eigener Einstellungsordner).

| Lauf | Modus | Ergebnis |
|---|---|---|
| s4asan1..3 | `karte` (je samt "Halte D", seitlich) | je 227 OK, 0 FEHLER, kein ASan-Bericht |
| s4asanke | `karteneditor` (ganz) | **heap-use-after-free**, Lauf bei der ersten Meldung beendet |
| s4asanke2 | `karteneditor` nach dem Fix | 1089 OK, 0 FEHLER, kein ASan-Bericht, Spitze 2036 MB |

Fund: `kameraSzeneBauen` (src/karte/kameraszene.cpp) merkte sich den Shader
der letzten Flaeche als `const std::string*`, um das Kleinschreiben und Suchen
zu sparen. Bei misc_model zeigte der Zeiger in die `ModellAnsicht` der
Entitaet (die Shader nach Skin und `_remap`), die nach jeder Entitaet
freigegeben wird; der naechste Vergleich las freigegebenen Speicher. Lag die
Ansicht des naechsten Modells an derselben Stelle, verglich der Zeiger den
neuen Shader mit sich selbst, und das Modell landete im Stapel (Shader) des
vorigen - ein sichtbarer Fehler (falsche Textur am zweiten Modell), dazu
undefiniertes Verhalten. Rot: ctest `kartekamera` im ASan-Bau mit dem neuen
Fall "sechs misc_model mit eigenen Shadern" -> ASan meldet dieselbe Stelle
(kameraszene.cpp:355, freigegeben in :560) wie der Selbsttest. Fix: der letzte
Shader als Kopie (`std::string` + gueltig). Gruen: `kartekamera` unter ASan
0 Fehlschlaege, der ganze Karten-Editor-Selbsttest unter ASan ohne Bericht.
Ob genau das der einmalige 0xC000001D war, laesst sich nicht beweisen: der
Absturz selbst kam in 3 ASan-Laeufen von `karte` nicht wieder, und das war die
einzige Speicherverletzung, die ASan in beiden Modi fand.

## Punkt 0c: keine abgeschnittene Beschriftung (Einstellungen, 4 Sprachen)

Neue Pruefung im Selbsttest (`BHED_KARTE_NUR=stufe4`, `stufe4::reste0`): jede
der 23 Einstellungsseiten in Englisch, Deutsch, Chinesisch und Japanisch; aus
der Zeichenliste des Fensters jede Glyphe (Vierecke mit der Schrift-Textur,
nicht das weisse Pixel und nicht die UV der kantengeglaetteten Linien, ohne
den Fenstertitel) muss waagerecht im Rahmen der Seitengruppe liegen (164..586
- links davon verdeckt der Seitenbaum, was hinausragt), jede Glyphe des Baums
in dessen Ausschnitt. ImGui gibt Glyphen ganz ausserhalb des Ausschnitts gar
nicht aus; die eine, die ueber den Rand reicht, ist der Befund.

| Lauf | Ergebnis |
|---|---|
| s4p0rot4 (vorher) | Englisch 0, Deutsch 23, Japanisch 23 FEHLER: Baum "Oberflaechen-Inspektor" 4 Bildpunkte zu lang; Seite Brush: "Ebenenpunkte auf ganze Zahlen runden (kann Pinsel zerstoeren)" rechts hinaus; Japanisch: Seite Interface Beschriftung links hinaus (x 966 < 1000), Seite Textures (988), Seite Build ("リージョン枠のシェーダー", der Befund aus Stufe 3), Seite Brush rechts hinaus, Baum "サーフェスインスペクター" und "テクスチャブラウザー" |
| s4p0gruen5 (nachher) | 105 OK / 0 FEHLER (92 Seiten-Pruefungen + Surface Inspector) |

Ursache links: `labelBreite` mass mit Tahoma (DirectWrite, Qt-Regeln) -
Zeichen, die Tahoma nicht hat, misst DirectWrite mit seiner Ersatzschrift,
gezeichnet wird mit der CJK-Schrift des Programms (breiter). Jetzt gilt bei
Text mit Zeichen ab U+0800 das Groessere von beidem (Latein unveraendert,
also bleiben die gegen die NRC-Fotos gemessenen Lagen). Baum und
Kontrollkaestchen: zu lange Uebersetzungen gekuerzt (deutsch
"Flaecheninspektor" ueberall statt "Oberflaechen-Inspektor", Baum
"Autosicherung", "Ebenenpunkte ganzzahlig (kann Pinsel zerstoeren)";
japanisch "表面インスペクタ", "テクスチャ一覧", "モデルブラウザ", kuerzerer Text
fuer "Snap plane points"). Die englische Spalte (= NRC) ist unveraendert.

## Punkt 0d: Surface Inspector so breit wie das NRC-Foto

| Lauf | Innenraum | Texturing-Spalten (x/Breite) |
|---|---|---|
| NRC-Foto `dlg_View_Surface_Inspector_0.png` | 285 x 364 | 23/43, 72/44, 122/67, 195/67 |
| s4p0rot4 (vorher) | 292 x 364 | 23/43, 72/52, 130/67, 203/67 |
| s4p0gruen5 (nachher) | 285 x 364 | 23/43, 72/44, 122/67, 195/67 |

Unser Fenster war 7 Punkte (14 Bildpunkte bei Massstab 2) **breiter**, nicht
schmaler (die Notiz in Stufe 3 "12 Punkte schmaler" war verdreht; die Tabelle
dort zeigte 582 gegen 570 Bildpunkte). Ursache: wir rechneten die Knopfspalten
der Gruppe "Texturing" mit Text + 16. NRC zieht das Fenster auf 99 x 99
(BuildDialog, surfacedialog.cpp:637), also auf die Mindestgroesse des
QVBoxLayout; die Knoepfe haben `setMinimumWidth( 10 )` (surfacedialog.cpp:
762-830) und zaehlen dafuer nicht. Das Breiteste ist das obere Raster (285).
In der Gruppe verteilt QGridLayout 221 Punkte mit qGeomCalc zwischen Mindest-
(Beschriftung "Project:" 43, Knoepfe 10, Spinbox 67) und Wunschbreite
(QPushButton: Text + 8, wenigstens 80 - QCommonStyle/QFusionStyle
CT_PushButton). Nachgebaut mit `karte::geomCalc` (aus Stufe 1b).

## Punkt 1: fertige Karten oeffnen

Kern `include/bhed/karte/bspoeffnen.h`, `src/karte/bspoeffnen.cpp` (map.cpp
tryDecompile, q3map2 bsp.cpp -onlyents), Oberflaeche `gui/app_karte_bsp.cpp`.

- **File > Open .bsp...** (eigener Eintrag direkt nach Import...; Abweichung:
  NRC kennt die .bsp nur beim Import - mapq3/plugin.cpp:239 "quake3 compiled
  maps" mit can_load = false). Der Dialog "Open compiled map" listet jede
  maps/**.bsp der Spielordner wie das Spiel sie sieht (je Name das erste Archiv
  in der Suchreihenfolge: fs_game vor base, spaetere .pk3 zuerst), mit Filter;
  "From file..." nimmt eine .bsp von der Platte. Ohne NRC-Foto: Qt-Regeln (Rand
  11, Abstand 6, Knoepfe 80 x 22), Innenraum 520 x 388.
- Die .bsp kommt nach `<Einstellungen>/karte_bsp/<Name>/` (`<Name>.bsp` und
  `<Name>_original.bsp`), nie in den Spielordner. Dort laeuft
  `bhed_q3map2 -v -game ja|jk2 -fs_basepath "<Engine>" -fs_homepath "" -fs_game
  <base|MD> -convert -format map|map_bp "<Name>.bsp"` (Map_ImportFile:
  map.cpp:1583-1594; Movie Duels mit -fs_game MD, Jedi Outcast mit -game jk2).
  Das Format folgt "New map Brush Type" (die neue Karte wird wie bei Open frei
  gemacht); q3map2 schreibt `<Name>_converted.map`, die als `<Name>.map`
  geladen wird (dort sucht -onlyents in Punkt 2 die Entitaeten). Warten wie
  Q_Exec( ..., waitfor ) hoechstens 10 Minuten.
- **File > Import...** nimmt wie bei NRC auch .bsp (Dateifilter
  `*.map;*.reg;*.bsp`): umgewandelt im Arbeitsordner (NRC: neben der .bsp) und
  eingefuegt, im Format der offenen Karte.

Rot/Gruen:

| Pruefung | rot | gruen |
|---|---|---|
| ctest `karte_bspoeffnen` (32: Befehle wie NRC, Fingerabdruck, raum.nrc.bsp -convert/-onlyents, je zwei Originalkarten aus JA/JK2/MD gegen die Massenlast) | Mutationen M2 (Fingerabdruck fest) und M3 (Einsetzen mit den neuen Lumps): 6 FEHL | 32/32 |
| Selbsttest `BHED_STUFE4_NUR=oeffnen` (s4p1rot: Mutation M1 - .pk3 in der Liste uebergangen) | 15 OK / 16 FEHLER | s4p1gruen: 26 / 0 |

Gezaehlt gegen die Massenlast (`SP\karte_dekomp\<spiel>\bp`, von NRCs q3map2
umgewandelt): mp/duel1 26 Entitaeten / 362 Pinsel / 66 Patches (Selbsttest,
aus assets0.pk3 von Movie Duels); t1_inter, mp__duel1 (JA), duel_pit,
kejim_post (JK2), duel_twinsuns, md_ch_ben (MD) im ctest - alle gleich. Import
von duel1 in duel1: 26 + 25 Entitaeten (worldspawn verschmilzt), Rueckgaengig
wieder 26.

## Punkt 2: Originallicht behalten

- Beim Oeffnen (Punkt 1) merkt sich der Editor Herkunft, Arbeitsordner und den
  **Fingerabdruck der Geometrie** (`karte::geometrieFingerabdruck`: jede
  Entitaet mit Pinseln/Patches nur mit ihren Primitiven - Ebenenpunkte,
  Shader, Texturprojektion, Flags, Kontrollpunkte, ohne Schluessel und ohne
  "// entity N" -, dazu jede Entitaet, die q3map2 einbackt: misc_model,
  _decal, _skybox mit Schluesseln, UnparseEntities in bspfile_abstract.cpp).
- **Build > Update entities only** (Abweichung, eigener Eintrag nach "Cancel
  build"; nur mit geoeffneter .bsp, Tooltip erklaert es): ist der
  Fingerabdruck gleich (Schluessel/Werte geaendert, Punkt-Entities verschoben
  oder neu), wird die Karte als `<Arbeitsordner>/<Name>.map` geschrieben, die
  Original-.bsp frisch nach `<Name>.bsp` kopiert und im Hintergrund (Konsole,
  Statuszeile, Schlusszeile wie beim Bauen) `bhed_q3map2 ... -bsp -onlyents
  [-keeplights] "<Name>.map"` ausgefuehrt. Danach wird der Entity-Lump aus
  q3map2s Ergebnis in die Original-Datei eingesetzt
  (`karte::bspEntitaetenEinsetzen`), die Konsole meldet "Entities updated: ...
  - every other lump byte for byte as in the original (17 of 17)".
- Ist die Geometrie anders: der Kasten "Update entities only" mit dem Hinweis,
  dass neu gebaut und belichtet werden muss (BSP, VIS, LIGHT), auch in der
  Konsole; q3map2 laeuft nicht.

Warum mehr als nur `-onlyents`:
- Ravens Jedi-Outcast-Karten haben ihre Lichter im Entity-Lump; SetLightStyles
  (writebsp.cpp:222-260) wirft jedes light ohne targetname hinaus, wenn nicht
  `-keeplights` gilt. Darum `-keeplights`, wenn die Original-.bsp ein light
  hat (Jedi Academy: nie).
- WriteRBSPFile packt das Lichtgitter neu (AddLightGridLumps: Naeherungs-
  Zwillinge mit LG_EPSILON 4). Bei Ravens JA-Karten ergibt das dieselben Bytes,
  bei JK2 nicht: duel_pit nach `-onlyents` lightgrid und lightarray anders.
  Mit dem Einsetzen in die Original-Datei bleiben alle Lumps ausser Lump 0
  Byte fuer Byte (Reihenfolge wie im Original, je auf 4 Bytes aufgefuellt).
- Rundlauf ohne Aenderung (q3map2 allein, SP\s4\werk\onlyents_serie.py):
  t1_inter, academy1, mp__ffa3, mp__siege_hoth: Entity-Lump gleich; hoth2, kor1,
  mp__duel1: nur "origin" von Pinsel-Entities mit Ursprungspinsel neu
  formatiert ("6120 -4064 1568" -> "6120.000000 -4064.000000 1568.000000",
  LoadMapFile); alle anderen Lumps gleich.

Rot/Gruen:

| Pruefung | rot | gruen |
|---|---|---|
| ctest `karte_bspoeffnen` (Einsetzen, -keeplights, raum: -onlyents Byte-gleich und die Aenderung im Entity-Lump; JK2 duel_pit/kejim_post) | M3 (Einsetzen mit q3map2s Lumps): JK2 2 FEHL; M2 (Fingerabdruck fest): 3 FEHL | 32/32 |
| Selbsttest `BHED_STUFE4_NUR=licht` (worldspawn "message", info_player_deathmatch verschoben, Build > Update entities only; Pinsel verschoben -> Hinweis) | s4p1rot: M2 - "Pinsel verschoben: Fingerabdruck anders", "Hinweis ..." FEHLER | s4p2gruen |

Gepruefte Wirkung im Selbsttest: duel1.bsp nach "Update entities only" - 17
von 17 Lumps ausser den Entities Byte-gleich zu `duel1_original.bsp`, der
Entity-Lump traegt `"message" "Stufe 4 bhed_test"` und `"origin" "100 200
64"`; nach dem Verschieben eines Pinsels der Kasten und kein q3map2; nach
Rueckgaengig wieder gleich; eine andere Karte geoeffnet: der Eintrag gesperrt.

## Punkt 0b: -light langsamer als NRCs q3map2

Messung (`SP\s4\werk\lichtzeit.py`, mp__duel1 nach NRCs -vis samt .srf/.map,
`-light -fast -patchshadows -samples 2 -filter -bounce 8`, Median aus 3, Idle):

| Bau | -threads 1 | Verhaeltnis zu NRC | q3map2_vergleich |
|---|---|---|---|
| NRC 20260114 (MinGW-GCC -O3) | 8.39-8.45 s | 1 | - |
| unser Bau wie bisher (/O2 /fp:precise) | 11.66 s | 1.39 | 0 Abweichungen |
| + /arch:AVX2 | 11.57 s | 1.40 | 0 |
| + /Ob3 | 12.22 s | 1.45 | 0 |
| + /Ob3 /GL /LTCG | 11.77 s | 1.40 | 0 |
| + /GS- (ohne Stapelpruefung) | 11.66 s | 1.39 | 0 |
| nur P-Kerne (Affinitaet 0xFFFF) | 11.91 gegen 8.45 s | 1.41 | - |
| nur E-Kerne (0xFFFF0000) | 23.65 gegen 18.57 s | 1.27 | - |
| -threads 4 / 16 / 32 | 3.73 / 3.52 / 3.61 gegen 2.78 / 2.48 / 2.48 s | 1.34 / 1.42 / 1.46 | - |

Profil ohne Administratorrechte: ein eigener Abtaster
(`SP\s4\werk\abtaster.py`: SuspendThread/GetThreadContext je Thread, RIP,
Module, dbghelp aus den Windows Kits gegen die PDB eines Baus mit /Zi in
`Y:\karte\build-q3perf`). Arbeit (ohne die wartenden Threads des Windows-Pools,
ntdll!NtWaitForWorkViaWorkerFactory): TraceLine 25 %, VectorNormalize<float>
22 %, IlluminateRawLightmap 13 %, LightContributionToSample 11 %,
PointToPolygonFormFactor 9 %, SetupTrace 8 %, acos 6 %. Kein Allocator im Profil
(keine HeapAlloc/malloc-Proben). VectorNormalize ist bei MSVC eine eigene
Funktion (in double: sqrtpd und drei divsd - dieselbe Rechnung wie bei GCC);
acos der UCRT (acos_fma) ist schneller als msvcrt!acos bei NRC (dort 10 % der
Arbeit). NRCs q3map2.exe hat keine Symbole, ein Vergleich je Funktion geht
nicht; einen GCC oder clang gibt es auf dem Rechner nicht.

Ergebnis: keiner der Schalter bringt etwas, die Ergebnisse bleiben mit jedem
gleich. Die 40 % stecken im Maschinencode, den MSVC fuer die heissen
Rechenfunktionen erzeugt, nicht in Threads, Allocator oder Inlining. Es bleibt
bei /O2 /fp:precise (unveraenderter Bau); offen fuer spaeter: ein Bau mit
clang-cl (VS-Komponente "C++-Clang-Tools", nicht installiert) zum Vergleich.

## Punkt 3a: bobToolz

Portiert (NRC 20260114, contrib/bobtoolz, GPL v2 nach LGPL 2.1 Abschnitt 3):
`include/bhed/karte/bobtoolz.h`, `src/karte/bobtoolz_d.h/.cpp` (DPlane, DBrush,
DPatch, DEntity, DMap, DShape, DWinding, DPoint, DEPair, mathlib in float wie
bobToolz), `src/karte/bobtoolz.cpp` (funchandlers-GTK.cpp Do*, shapes.cpp,
DTreePlanter, DTrainDrawer, DBobView, ScriptParser, lists, misc),
`src/karte/bobtoolz_vis.cpp` (bsploader.cpp, visfind.cpp), die Oberflaeche
`gui/app_karte_plugins.cpp` (QERPlug_Dispatch, Werkzeugleiste, die acht
Dialoge aus dialogs-gtk.cpp, "BSP leafs" aus DVisDrawer.cpp, Zeichnen von
DBobView / DTrainDrawer / DVisDrawer, DTreePlanter::mouseDown ueber
XYWindowMouseDown), Qt-Bausteine fuer alle Plugin-Dialoge in
`gui/karte_qtdialog.h/.cpp`; NRCs Datendateien `setup/data/tools/plugins/bt/*.txt`
unveraendert nach `data/karte/bt` (.gitattributes `-text`).

Alle 23 Befehle des Menues und die 9 Knoepfe der Plugin-Leiste (Tree Planter
rastet ein) sind nachgebaut; Checkliste: 46 Eintraege `bobToolz::*` auf `[x]`
(Befehle und Plugin-Menue).

Masse der Dialoge nach den Qt-Regeln (karte_qtdialog.h). Gegen die NRC-Fotos:
Intersect 192 x 157 (OK 15/124, Cancel 101/124), Path Plotter 367 x 180
(Enable / Disable / Cancel 104 / 190 / 276, y 147), Make Chain 362 x 100
statt 361 (Abweichung +1: die Beschriftung "Basename for chain's targetnames"
misst unsere DirectWrite-Rechnung 168, Qt 167 - am Foto beginnt die
Feldspalte bei 184). Gefunden dabei: QCheckBox ist wie QRadioButton
14 + 4 + 6 + Text + 1 breit (QCommonStyle CT_CheckBox, "4 pixels for label
margins"), nicht 14 + 6 + Text + 1. QFormLayout::addWidget legt Haken und
Knopfleisten in die Feldspalte (FieldRole), daher die Feldbreite 166 (OK /
Cancel) bzw. 252 (drei Knoepfe). Ohne Foto nach denselben Regeln: Polygon
Builder, Stair Builder, Door Builder, Aperture Door Builder (QTabWidget:
Reiter 22, Seite mit Rand 11), Texture Reset, BSP leafs.

Abweichungen (bewusst): die Uebersetzungen der zweiten Ausrichtung im Door
Builder sagen "Ost - West" (NRC beschriftet beide Knoepfe "North - South",
im Modus "Begriffe wie NetRadiant" bleibt das so); Vis Viewer liest
`<Karte>.bsp` neben der Karte wie NRC; im Selbsttest ist time( nullptr ) = 0
(srand), damit Teamnamen fest sind.

| Test | Rot | Gruen |
|---|---|---|
| ctest `karte_bobtoolz` (neu, 82 Pruefungen: jeder Befehl mit Wirkung, Undo-Namen, Meldungen) | 5 Mutationen (Treppenhoehe, Duplikate-Liste, Kettenabstand, Merge ohne Loeschen, Tree Planter ohne Raster) je 1-2 FEHL | 0 Fehlschlaege |
| Selbsttest `BHED_STUFE4_NUR=bobtoolz` (Menue, Dialoge, Wirkung, 2D-Farbe, Klick in XY, Vis Viewer mit bsp stage) | s4p3bob1 42 / 8 (u. a. Intersect 188 statt 192: Haken-Hinweis) | s4p3bob3 50 / 0 |
| ctest gesamt | - | 108 / 108 |

Bei NRC gefunden und uebernommen: Brush Cleanup waehlt alles ab
(DMap::LoadAll: GlobalSelectionSystem().setSelectedAll( false ), DMap.cpp:107);
Reset Textures loescht den Pinsel und baut ihn neu (er steht danach hinten);
der Undo-Name der Aperture Door ist "bobToolz.buildApertureDoor"
(shapes.cpp:648).

## Punkt 3b: brushexport2

Portiert: `include/bhed/karte/brushexport.h`, `src/karte/brushexport.cpp`
(contrib/brushexport/export.cpp: ExportData, ExportDataAsWavefront,
ExportSelection; FloatFormat "%1.6lf", "%g" fuer Kd, Edge_isDegenerate),
`gui/app_karte_brushexport.cpp` (interface.cpp create_w_plugplug2,
callbacks.cpp). "About" ("About me...") und "Export selected as Wavefront
Object" (das Fenster, einmal gebaut, show/hide). Checkliste: 4 Eintraege
`brushexport2::*` auf `[x]`.

Masse am Foto: Innenraum 278 x 478, Spalten 125 / 125, Zeilen wie im Kopf von
app_karte_brushexport.cpp. Gefunden dabei: QGridLayout verteilt die Breite
eines Elements ueber zwei Spalten (die Liste, 256) mit qGeomCalc - gleich
geteilt, nicht Hinweis + halber Rest (sonst 146 / 104).

Abweichungen: "Save As" schlaegt `<Einstellungen>/karte_export/` vor statt
`<Spiel>/base/models/` (nie in den Spielordner); das Fenster ist nicht
ziehbar; der gewaehlte Pfad wird mit '/' gespeichert wie QFileDialog ihn
liefert (sonst stuende in "mtllib" der ganze Pfad: find_last_of( "//" )).
Die Farbe in der .mtl (Kd) ist qtexture_t::color: aus der Kamera, sonst das
Bild gemittelt.

| Test | Rot | Gruen |
|---|---|---|
| ctest `karte_brushexport` (neu, 19 Pruefungen) | 3 Mutationen (Achsen, Verschweissen, Material-Reihenfolge) je 1-2 FEHL | 0 |
| Selbsttest `BHED_STUFE4_NUR=brushexport` | s4p3be1 17 / 2 (Spalten 146 / 104, mtllib mit Pfad) | s4p3be4 19 / 0 |
| ctest gesamt | - | 109 / 109 |

## Punkt 3c: ShaderPlug, SunPlug, UFO:AI

Portiert: `include/bhed/karte/kleineplugins.h`, `src/karte/kleineplugins.cpp`
(shaderplug.cpp, sunplug.cpp, ufoai_level.cpp, ufoai_filters.cpp),
`gui/app_karte_kleineplugins.cpp` (die Kaesten, der Dialog "ET-MapCoordinator"),
`karte::kartenGrenzen` in auswahlsystem (GetMapBounds: worldAABB der Wurzel).
Checkliste: 40 Eintraege (ShaderPlug 2, SunPlug 2, UFO:AI 16; je Befehl und
Menue) auf `[x]`.

- ShaderPlug: About; "Create tag file" sammelt die Shader (nur "textures/")
  und die Bilder unter textures/ (jpg, tga, png, alle Tiefen: forEachFile mit
  depth 0) ohne gleichnamigen Shader, baut die Tag-Datei (AddShaderNode fuegt
  vorn ein - rueckwaerts eingefuegt steht sie sortiert in der Datei), fragt
  "Overwrite tag file?" wenn es sie gibt. Abweichung: die Datei wird gleich
  neu geladen (NRC: "Please restart Radiant now."), der INFO-Kasten sagt das.
- SunPlug: NRCs SunPlug ist nicht "q3map_sun aus Licht", wie der Auftrag
  vermutete, sondern About und der ET-MapCoordinator (mapcoordsmins /
  mapcoordsmaxs fuer Enemy Territory, das Menue zeigt ihn bei NRC fuer jedes
  Spiel). Nachgebaut wie bei NRC: Vorschlag aus den Kartengrenzen (Quadrat,
  halbe Seite >= 175), Dialog mit "Get optimal mapcoords", Set als Undo
  "SunPlug.entitySetMapcoords".
- UFO:AI: About (Version des Programms statt RADIANT_VERSION), Worldspawn /
  Worldspawn reset (maxlevel, maxteams), Perform check, Level 1-8 (Filter nach
  den Inhalts-Flags der letzten Flaeche und spawnflags & level_active wie
  NRC - level_active ist 1 << (n - 1)), StepOn / ActorClip / WeaponClip /
  NoDraw (verstecken an/aus). Abweichung: die Schluessel werden in einem
  Undo-Schritt "UFO:AI" gesetzt (NRC ohne UndoableCommand). Die Werkzeugleiste
  von UFO:AI gibt es bei NRC nur fuer das Spiel UFO:AI - fuer JA keine.

| Test | Rot | Gruen |
|---|---|---|
| ctest `karte_kleineplugins` (neu, 21 Pruefungen) | 3 Mutationen (Shader ohne Praefix, Mindestmass 175, Level-Flag der Entitaeten) je 1-2 FEHL | 0 |
| Selbsttest `BHED_STUFE4_NUR=klein` | s4p3kl1 36 / 7 (Erwartungen: 16 UFO:AI-Befehle, level_active als Bitmaske) | s4p3kl2 43 / 0 |
| ctest gesamt | - | 110 / 110 |

## Punkt 3d: MeshTex

Portiert: `include/bhed/karte/meshtex.h`, `src/karte/meshtex.cpp`
(MeshEntity.cpp Zeile fuer Zeile: Min/Max Align, Stretch / Shrink / Auto,
SetScale mit der Laengenmessung auf der Patch-Flaeche - Gauss-Legendre mit
10 Punkten, rekursiv bis Fehler 0,5 -, GeneralFunction, GetInfo mit den
Texten aus MeshEntityMessages.h; auch NRCs Eigenheit, dass GeneralFunction
alignRow fuer beide Ausrichtungen nimmt, MeshEntity.cpp:550-551),
`gui/app_karte_meshtex.cpp` (MainMenu, MeshVisitor, GenericDialog: drei
nicht modale Dialoge mit OK / Cancel / Apply). Checkliste: 32 Eintraege
`meshTex::*` auf `[x]`.

Masse: Get Info 357 x 126 nach den Qt-Regeln; Set S/T Scale 418 x 344 und
General Function 912 x 298 mit den Lagen der Gruppen, Felder und Knoepfe am
Foto gemessen (verschachtelte QGroupBox, QGridLayout mit setColumnStretch).
Wie bei NRC steht auch in der T-Zeile von General Function " * old_S  + ".
Abweichung: mehrere Kaesten eines Befehls (je Netz) zeigt das Programm als
einen Kasten mit allen Texten. Die Kopfzeile "Copyright 2012 Joel Baxter"
steht als "Copyright (C) 2012 Joel Baxter" (ctest karte_lizenz sucht "(C)").

| Test | Rot | Gruen |
|---|---|---|
| ctest `karte_meshtex` (neu, 24 Pruefungen) | 4 Mutationen (Spalten-Vorzeichen, Gauss-Gewicht, Kontrollpunkte aus Flaechenwerten, Auto-Wahl) je 1-8 FEHL | 0 |
| Selbsttest `BHED_STUFE4_NUR=meshtex` (11 Voreinstellungen mit Undo-Namen, Get Info mit Uebertragen, Set S/T Scale, General Function, Fotos) | s4p3mt1 32 / 5 (Zahl der Befehle 16, verdeckte Fenster) | s4p3mt3 35 / 0 |
| ctest gesamt | - | 111 / 111 |

Damit sind alle Plugins von NRC fuer JA/JK2 nachgebaut: bobToolz, brushexport2,
MeshTex, prtview (Stufe 3), ShaderPlug, SunPlug, UFO:AI.

## Punkt 4: die uebrigen Eintraege der Checkliste

- "Check for NetRadiant update" und "Manual" (F1) leiten seit Stufe 3 Punkt 0b
  auf die Update-Pruefung bzw. das Handbuch des Programms (Abweichung, dort
  dokumentiert; im Selbsttest nur gemerkt, ohne Browser und Netz).
- File > Exit war offen (`[ ]`): beendete das Programm ohne die Rueckfrage
  der Karte. Jetzt wie mainframe.cpp:621-625: ConfirmModified( "Exit
  Radiant" ) (Save / Discard / Cancel), danach der Beenden-Weg des Programms
  (confirmQuit: ungesicherte Skripte, "Exit?"). Rot s4p4rot3 13 / 2 (kein
  "Exit Radiant", das Programm fragte gleich "Exit?") -> gruen s4p4gruen 15 / 0.
- "Install Map Developer's Files" bleibt `[~]`: ja.game / jk2.game haben
  keinen install-Ordner, NRC zeigt dort nichts (Stufe 2 Punkt 11).

Checkliste jetzt: 935 `[x]`, 1 `[~]`, 0 `[ ]`.

## Punkt 6: Handbuch

- Neue Seiten `tools/handbuch/inhalt_karte4.py` (Gruppe "karte", nach "Karte
  bauen"): **Fertige Karten oeffnen und aendern** (`karte_oeffnen`: Open .bsp,
  was zurueckkommt und was nicht, Import einer .bsp, Originallicht mit "Update
  entities only", Kasten bei geaenderter Geometrie, -keeplights, Unterschied
  zu NRCs "Entity-Only compile", Testen im Spiel) und **Plugins**
  (`karte_plugins`: Menue, bobToolz-Tabelle aller Eintraege, Path Plotter in
  Schritten, brushexport2, MeshTex, ShaderPlug/SunPlug/UFO:AI/prtview).
- Veraltete Saetze ersetzt: "Plugins ausser prtview kommen in einer spaeteren
  Stufe" (inhalt_karteneditor.py, inhalt_karteneditor2.py, 4 Sprachen) ->
  Verweis auf [[karte_plugins]]; der Absatz zu File nennt Open .bsp.
- gen_handbuch.py laedt jetzt auch KARTE4 / KARTE4B ({{Kennung}} der Stufe 4).
- 7 Bilder je Sprache aus dem Selbsttest (`BHED_EDITORTEST=handbuch`,
  `BHED_HB_NUR=karte4`, LIESMICH ergaenzt): h_karte4_bspdialog, _geladen
  (mp/duel1 aus der .pk3, Kamera im Saal), _nurentities, _plugins,
  _pathplotter, _meshtex, _brushexport. `gen_handbuch.py --pruefen`:
  handbuch_sprachen ok (138 Seiten x 4 Sprachen).

### Fund beim Ansehen der Bilder: japanische Beschriftungen zu lang

Im japanischen Bild von brushexport2 ragten "1 つのメッシュにまとめる" und
"マテリアルごとにまとめる" unter die Knopfspalte. Ursache wie in Punkt 0c:
`qtd::textBreite` (die Masse aller Plugin-Dialoge) mass mit Tahoma
(DirectWrite misst CJK mit seiner Ersatzschrift), gezeichnet wird mit der
CJK-Schrift des Programms. Jetzt dieselbe Regel wie `labelBreite`: bei Text
mit Zeichen ab U+0800 das Groessere von beidem (Latein unveraendert, die
gegen die NRC-Fotos gemessenen Lagen bleiben).

Neue Pruefung `BHED_STUFE4_NUR=texte` (`stufe4::pluginTexte`): jeder
Radioknopf, jedes Kaestchen, jede Beschriftung vor einem Zahlenfeld und jeder
`qtd::text` meldet Platz (Qt-Hinweis bzw. textBreite + Abstand 6) und
gezeichnete Breite; 13 Dialoge (bobToolz 1-8, brushexport2, SunPlug, MeshTex
3) in 4 Sprachen.

| Lauf | Ergebnis |
|---|---|
| s4p6rot (vorher) | 54 OK / 10 FEHLER, alle Japanisch: z. B. brushexport2 bemodus0 gezeichnet 138, Platz 126; Intersect, Stair/Door Builder, Reset Textures, MeshTex Get Info; SunPlug ohne gemeldete Texte |
| s4p6rot2 (Mutation: alte textBreite, mit qtd::text-Meldung) | 54 OK / 10 FEHLER (dazu Aperture Door, Make Chain, Set S/T Scale, General Function) |
| s4p6gruen2 | 64 OK / 0 FEHLER |

Die chinesischen und japanischen Bilder danach neu aufgenommen.

## Punkt 7: Schlussrunde

Ein Testprozess zur Zeit, Selbsttests mit Prioritaet Idle und dem 8-GB-Waechter
(nie ausgeloest, hoechste Spitze 1.5 GB in spulen); Skript `SP\s4\ende_s4.sh`
(wie `SP\s3\ende_s3.sh`), Summe `SP\s4\ende_summe.txt`.

| Test | Ergebnis |
|---|---|
| ctest | 111 / 111 (auch nach dem sauberen Neubau) |
| Karten-Editor voll mit grosser Karte (s4eke) | 1246 / 2 - zwei Erwartungen aus frueheren Stufen (unten); nach der Korrektur s4eke2: 1248 / 0 (Spitze 952 MB); Checkliste "304 fertige Befehle, 3 Menue-Rufe, 61 Plugin-Eintraege, 357 nachgebaut; es fehlt: -; zu viel: -" |
| Modus 1 (Gesamttest) | s4e1: 2645 / 0 - wie rc605 |
| karte | s4ekarte: 227 / 0 (kein Absturz; in Stufe 3 einmal 0xC000001D) |
| kartenfunde, fix604b, laserfalle, geschuetzanim, zielstrahl, effektgrund, figurdeform, jk2, tuerlauf | 53, 32, 8, 3, 2, 1, 1, 15, 13 OK / je 0 FEHLER |
| kamera, missionsbau, szenen, ton, npcklasse | 95, 95, 26, 34, 12 / je 0 |
| gluehen, gras, spulen, figurstufen, missionsliste, rest605 | 8, 13, 13, 8, 11, 6 / je 0 |
| Leistung (md_am_sith, BHED_D3D_DEBUG=0, rc605-Bau aus Y:\integ und unser Bau abwechselnd, je zweimal) | Vorbereitung 8.40 / 6.81 s (rc605) gegen 7.16 / 6.56 s; intro2_sith Mittel 15 / 14 gegen 14 / 13 ms (max 235 / 197 gegen 177 / 171); Flug 6.8 / 7.3 gegen 6.8 / 7.5 ms; zweiter Durchgang 10 / 11 gegen 10 / 11 ms - kein Unterschied |
| Kartenpruefung (278 Karten, Movie Duels, gegen das rc605-Schlussaudit `f5audit`) | alle Summen gleich (604 s): abgestuerzt/haengend, nicht geladen, GPU-Fehler, D3D-Meldungen, Protokollfehler 0 -> 0; nicht gezeichnete Entities 1 -> 1; NPC ohne Modell 2 -> 2; Weltshader-Luecken 0 -> 0; fehlende efx/Klaenge/Texturen/Modelle 27/29/139/5 unveraendert |
| grosse Karte duel_mos_espa (s4eke / s4eke2) | Laden 1176 / 1065 ms; Editor je Bild 2.96 / 3.72 ms; Bild nach Auswahlwechsel 10.1 / 13.4 ms; Neubauten Geometrie 0 / Rest 10; Speicher privat +918 / +911 MB, Grafik +523 / +514 MB |
| A/B grosse Karte (`SP\s4\ab_gross.sh`, BHED_KARTE_NUR=gross, Stufe-3-Bau aus laeufe\s3eke2 und unser Bau abwechselnd, je zweimal) | Laden 875 / 1040 ms (Stufe 3) gegen 946 / 977 ms; Editor je Bild 3.36 / 3.63 gegen 3.00 / 3.92 ms; Bild nach Auswahlwechsel 13.0 / 10.7 gegen 15.2 / 13.5 ms (davon Kamera-Szene 5.8 / 4.8 gegen 6.6 / 6.0) - die Bereiche ueberlappen, die Streuung innerhalb eines Baus ist so gross wie der Abstand. Im Pfad geaendert ist nur Punkt 0a (letzterShader als Kopie: eine Zuweisung je Shaderwechsel neben dem schon vorhandenen klein() und map::find); der Ladeweg ist unveraendert |
| sauberer Neubau (Release, `--clean-first`) | 0 Fehler; 127 Warnungen wie in Stufe 3, alle aus aelteren Stellen (C4996 u8path, C4456/C4457/C4458 verdeckte Namen, C4125 in entdefs.cpp) - keine aus Stufe 4 |
| Handbuch | `gen_handbuch.py --pruefen`: handbuch_sprachen ok |

In der Schlussrunde gefunden:

| Punkt | Rot (vorher) | Gruen (nachher) |
|---|---|---|
| Zwei Erwartungen aus Stufe 3 im vollen Karten-Editor-Lauf (die Pruefungen je Punkt liefen nur mit BHED_STUFE4_NUR): "Plugins > bobToolz ... ausgegraut (Stufe 3)" - seit Punkt 3a nachgebaut; die Checkliste liess nur die fertigen Befehle der Stufen 1b/2 und die Menue-Rufe zu, die 61 Plugin-Befehle der Stufe 4 standen als "zu viel" da. Beide auf Stufe 4 gestellt (bobToolz nachgebaut; jeder Eintrag der Plugin-Menues muss nachgebaut sein) | s4eke 1246 / 2 | s4eke2 1248 / 0 |

## Punkt 8: Abschluss der Stufe 4

### Commits (Zweig karte1, auf Stufe 3 `f7334a9`)

| Commit | Punkt | Inhalt |
|---|---|---|
| `2616824` | 0c, 0d | keine abgeschnittene Beschriftung auf den Einstellungsseiten (4 Sprachen), Surface Inspector so breit wie NRC |
| `5fbef5a` | 1 | fertige Karten oeffnen (.bsp aus Ordner oder .pk3 des Spiels), Import einer .bsp |
| `a885583` | 2 | Originallicht behalten: Build > Update entities only |
| `53edf68` | 0a, 0b | ASan: veralteter Shader-Zeiger in der Kameraszene behoben; Messung -light |
| `a2816b6` | 3a | bobToolz: alle 23 Befehle, Werkzeugleiste, acht Dialoge, BSP leafs |
| `fa392bd` | 3b | brushexport2: Export als Wavefront .obj/.mtl |
| `4101dee` | 3c | ShaderPlug, SunPlug (ET-MapCoordinator), UFO:AI |
| `f8de283` | 3d | MeshTex |
| `b1cca05` | 4 | File > Exit mit ConfirmModified; Checkliste 935 / 1 / 0 |
| `991f57b` | 6 | Handbuchseiten, CJK-Breiten in den Plugin-Dialogen, Selbsttest "texte" |
| `3f8d94b` | 7 | Schlussrunde; Selbsttest erwartet die Plugins der Stufe 4 |

### Portiert (NRC 20260114, mit Quellangaben je Funktion)

| Datei | aus NRC |
|---|---|
| `include/bhed/karte/bspoeffnen.h`, `src/karte/bspoeffnen.cpp`, `gui/app_karte_bsp.cpp` | map.cpp tryDecompile / Map_ImportFile (q3map2 -convert), q3map2 bsp.cpp -onlyents (Ablauf), bspfile_rbsp.cpp (Lump-Verzeichnis); eigener Dialog "Open compiled map", Fingerabdruck, Einsetzen des Entity-Lumps |
| `include/bhed/karte/bobtoolz.h`, `src/karte/bobtoolz.cpp`, `bobtoolz_d.h/.cpp`, `bobtoolz_vis.cpp`, `gui/app_karte_plugins.cpp` | contrib/bobtoolz: bobToolz-GTK.cpp, funchandlers-GTK.cpp, shapes.cpp, DShape/DBrush/DPlane/DPatch/DEntity/DMap/DWinding/DPoint/DEPair, misc.cpp, DTreePlanter, DTrainDrawer, DBobView, DVisDrawer, visfind.cpp, bsploader.cpp, ScriptParser.cpp, lists.cpp, dialogs/dialogs-gtk.cpp; libs/mathlib |
| `include/bhed/karte/brushexport.h`, `src/karte/brushexport.cpp`, `gui/app_karte_brushexport.cpp` | contrib/brushexport: export.cpp, interface.cpp, callbacks.cpp, plugin.cpp |
| `include/bhed/karte/kleineplugins.h`, `src/karte/kleineplugins.cpp`, `gui/app_karte_kleineplugins.cpp` | contrib/shaderplug/shaderplug.cpp, contrib/sunplug/sunplug.cpp, contrib/ufoaiplug (ufoai.cpp, ufoai_level.cpp, ufoai_filters.cpp) |
| `include/bhed/karte/meshtex.h`, `src/karte/meshtex.cpp`, `gui/app_karte_meshtex.cpp` | contrib/meshtex: MeshEntity.cpp, MeshEntityMessages.h, MainMenu.cpp, MeshVisitor, GenericDialog, SetScaleDialog, GetInfoDialog, GeneralFunctionDialog |
| `gui/karte_qtdialog.h/.cpp` | Qt 5.15-Regeln der Plugin-Dialoge (QCommonStyle CT_CheckBox/CT_RadioButton/CT_PushButton, QSpinBox/QLineEdit/QComboBox sizeHint, QDialogButtonBox, QFormLayout, QGridLayout/qGeomCalc) |
| `data/karte/bt/*.txt` | setup/data/tools/plugins/bt (unveraendert) |
| Tests | `tests/kartebspoeffnen.cpp` (32), `kartebobtoolz.cpp` (82), `kartebrushexport.cpp` (19), `kartekleineplugins.cpp` (21), `kartemeshtex.cpp` (24); kartekamera um modellShader erweitert |

Lizenzen: bobToolz und prtview LGPL 2.1, nach Abschnitt 3 unter GPL v2;
brushexport, MeshTex, ShaderPlug, SunPlug, UFO:AI GPL v2 (ctest karte_lizenz).

### Rot / Gruen je Punkt (Kurzfassung)

| Punkt | Rot | Gruen |
|---|---|---|
| 0a | ASan karteneditor: heap-use-after-free kameraszene.cpp:355; ctest kartekamera unter ASan rot | ctest gruen, ASan karteneditor 1089 / 0 ohne Befund |
| 0c | s4p0rot4: Deutsch 23, Japanisch 23 FEHLER | s4p0gruen5 105 / 0 |
| 0d | Surface Inspector 292 statt 285 | 285 x 364, Spalten wie das Foto |
| 1 | s4p1rot 15 / 16; ctest Mutationen 6 FEHL | s4p1gruen 26 / 0; ctest 32 / 32 |
| 2 | M2/M3 (Fingerabdruck, Einsetzen) rot | s4p2gruen; 17 von 17 Lumps Byte-gleich |
| 3a bobToolz | s4p3bob1 42 / 8; 5 Mutationen | s4p3bob3 50 / 0 |
| 3b brushexport2 | s4p3be1 17 / 2; 3 Mutationen | s4p3be4 19 / 0 |
| 3c Shader/Sun/UFO | s4p3kl1 36 / 7; 3 Mutationen | s4p3kl2 43 / 0 |
| 3d MeshTex | s4p3mt1 32 / 5; 4 Mutationen | s4p3mt3 35 / 0 |
| 4 Exit | s4p4rot3 13 / 2 | s4p4gruen 15 / 0 |
| 6 CJK-Breiten | s4p6rot 54 / 10, s4p6rot2 (Mutation) 54 / 10 | s4p6gruen2 64 / 0 |
| 7 Erwartungen Stufe 3 | s4eke 1246 / 2 | s4eke2 1248 / 0 |

### Abweichungen von NRC (gesammelt)

- **File > Open .bsp...** ist ein eigener Eintrag nach Import... (NRC kennt
  die .bsp nur beim Import, mapq3 "quake3 compiled maps"); Dialog "Open
  compiled map" ohne NRC-Vorbild nach Qt-Regeln; Arbeitsordner
  `<Einstellungen>/karte_bsp/<Name>` statt neben der .bsp (nie im
  Spielordner); Import einer .bsp ebenfalls im Arbeitsordner.
- **Build > Update entities only** ist ein eigener Eintrag (nach Cancel
  build): Fingerabdruck der Geometrie, `-onlyents` auf eine frische Kopie,
  Einsetzen des Entity-Lumps in die Original-Datei (alle anderen Lumps
  Byte-gleich), `-keeplights`, wenn das Original Lichter hat.
- SunPlug ist bei NRC der ET-MapCoordinator (kein "q3map_sun"); so portiert.
- bobToolz Make Chain: Dialog 362 statt 361 breit (Textmass 168 statt 167).
- bobToolz Door Builder: die zweite Ausrichtung heisst in den Uebersetzungen
  "Ost - West" (NRC beschriftet beide "North - South"; mit "Begriffe wie
  NetRadiant" wie NRC).
- brushexport2: Save As schlaegt `<Einstellungen>/karte_export/` vor; das
  Fenster ist nicht ziehbar; Pfad mit '/'.
- ShaderPlug: die Tag-Datei wird gleich neu geladen (NRC: Neustart).
- MeshTex: mehrere Kaesten eines Befehls als ein Kasten.
- UFO:AI: Schluessel in einem Undo-Schritt "UFO:AI"; About mit der Fassung
  des Programms.
- Exit: nach ConfirmModified der Beenden-Weg des Programms (ungesicherte
  Skripte, "Exit?").
- Plugin-Dialoge auf Chinesisch/Japanisch so breit wie der gezeichnete Text
  (Tahoma kennt die Zeichen nicht; Englisch/Deutsch wie Qt).
- Farben der neuen Dialoge aus dem Farbschema des Programms.

### Checkliste

`tools/karte/nrc_inventar.md`: 935 `[x]`, 1 `[~]` ("Install Map Developer's
Files" - ja.game/jk2.game haben keinen install-Ordner, NRC zeigt nichts),
0 `[ ]` von 936.

### Messungen

- Selbsttest Punkt 5 (alle Teile der Stufe 4 im vollen Karten-Editor-Lauf):
  Einstellungsseiten in 4 Sprachen, Open .bsp, Originallicht mit
  Byte-Pruefung (17 von 17 Lumps), bobToolz 50, brushexport2 19, kleine
  Plugins 43, MeshTex 35, Texte der Plugin-Dialoge 64, Exit 15 Pruefungen;
  der volle Lauf 1248 / 0.
- Dialoge gegen die NRC-Fotos: Intersect 192 x 157, Path Plotter 367 x 180,
  Make Chain 362 x 100 (+1), brushexport2 278 x 478 (Spalten 125 / 125),
  MeshTex Get Info 357 x 126, Set S/T Scale 418 x 344, General Function
  912 x 298, Surface Inspector 285 x 364.
- Originallicht: mp/duel1 (Movie Duels) nach "Update entities only" 17 von 17
  Lumps ausser den Entities Byte-gleich; Rundlauf ohne Aenderung bei
  t1_inter, academy1, mp__ffa3, mp__siege_hoth mit gleichem Entity-Lump.
- q3map2 -light: 39-46 % langsamer als NRCs MinGW-Bau, mit keinem
  MSVC-Schalter besser (Punkt 0b).
- Schlussrunde (Punkt 7): Leistung wie rc605 (md_am_sith), Kartenpruefung
  unveraendert, grosse Karte im A/B gegen den Stufe-3-Bau ohne messbaren
  Unterschied.

### Was Stufe 5 braucht (Optimierer gegen die Grenzen von JKA/JK2)

- Die Grenzen je Regelwerk aus den Quellen der Engines, nicht geschaetzt:
  OpenJK (SP/MP, ja), Movie Duels (eigene Engine, in `SP\mdgit` vorhanden) und
  Jedi Outcast (jk2, Quellen noch zu beschaffen). Aus `SP\mdgit\code` (Movie
  Duels) schon gesehen: qfiles.h MAX_MAP_MODELS 0x400, MAX_MAP_BRUSHES 0x8000,
  MAX_MAP_ENTITIES 0x800, MAX_MAP_ENTSTRING 0x40000, MAX_MAP_SHADERS 0x400,
  MAX_MAP_AREAS 0x100, MAX_MAP_FOGS 0x100, MAX_MAP_PLANES / NODES /
  BRUSHSIDES / LEAFS / LEAFFACES / PORTALS 0x20000, MAX_MAP_LEAFBRUSHES
  0x40000, MAX_MAP_LIGHTING 0x800000, MAX_MAP_LIGHTGRID 65535 (bei q3map2
  0x100000 Punkte - eine Karte kann sich bauen lassen und trotzdem darueber
  liegen), MAX_MAP_LIGHTGRID_ARRAY 0x100000, MAX_MAP_VISIBILITY 0x400000,
  MAX_MAP_DRAW_SURFS 0x20000, MAX_MAP_DRAW_VERTS / INDEXES 0x80000;
  q_shared.h GENTITYNUM_BITS 12 (Movie Duels; OpenJK hat weniger), MAX_MODELS
  512, MAX_SOUNDS 512, MAX_SUBMODELS 512, MAX_CONFIGSTRINGS 2048,
  MAX_WORLD_COORD 65536; cm_patch.h MAX_FACETS 4096, MAX_PATCH_PLANES 8192;
  tr_local.h MAX_PATCH_SIZE 32, SHADERNUM_BITS 13. Fuer OpenJK und Jedi
  Outcast fehlen die Werte noch.
- Zaehlen aus der Karte und aus der gebauten .bsp: das Lump-Verzeichnis liest
  `karte::bspoeffnen` schon (Punkt 2) - Zahl und Groesse je Lump gegen die
  Grenzen; dazu was erst zur Laufzeit zaehlt (Entities mit NPC-Spawnern,
  Modelle / Klaenge / Effekte als Configstrings, Brush-Modelle,
  Patch-Kollision je Patch).
- Vorschlaege mit Wirkung und Undo wie die Plugins: Detail statt
  Struktur, AutoCaulk (Stufe 3), Hint-Brushes, Lightmap-Massstab,
  `gridsize` im worldspawn, Zusammenlegen von Brush-Modellen, zu grosse
  Patches teilen (bobToolz Split patch ist da).
- Testkarten, die jede Grenze knapp ueber- und unterschreiten (Rot/Gruen),
  und ein Abgleich gegen echte Karten der drei Spiele (die Massenlast aus
  `SP\karte_dekomp`).
- Offen aus Stufe 4: -light 40 % langsamer als NRCs q3map2 (MSVC-Maschinencode;
  ein Bau mit clang-cl zum Vergleich); der Absturz 0xC000001D im Modus karte
  (Stufe 3) ist nicht nachgestellt - ASan fand dabei den veralteten
  Shader-Zeiger (behoben), ob das die Ursache war, ist nicht bewiesen.

## Changelog

## English

- Map editor: open finished maps. File > Open .bsp... lists every compiled map of the game folders, also inside the game's .pk3 files, and turns it back into an editable map with q3map2 in a work folder - never in the game folder. File > Import... also takes a .bsp.
- Map editor: keep the original light. Build > Update entities only writes changed entities back into the opened map; geometry, lightmaps and light grid stay byte for byte as they were. If brushes or patches changed, it says so instead of building.
- Map editor: all plugins of NetRadiant-Custom with their menus and dialogs - bobToolz (23 commands and its toolbar: stair, door, aperture door and polygon builders, Path Plotter, Tree Planter, splines, Vis Viewer, patch tools and more), brushexport2 (export the selection as a Wavefront .obj with .mtl), MeshTex (texture scaling and alignment on patches), ShaderPlug, SunPlug and UFO:AI.
- Map editor: File > Exit asks whether to save a changed map first, like NetRadiant-Custom.
- Map editor: labels on the preference pages and in the plugin dialogs are no longer cut off in German, Chinese and Japanese; the Surface Inspector has NetRadiant-Custom's width.
- Map editor: fixed a possible crash in the camera view of maps with several models.
- Guide: new pages "Map editor: opening and changing finished maps" and "Map editor: plugins" in four languages.

## Deutsch

- Karten-Editor: fertige Karten öffnen. „Datei > .bsp öffnen...“ listet jede gebaute Karte der Spielordner, auch in den .pk3 des Spiels, und macht sie mit q3map2 in einem Arbeitsordner wieder zu einer bearbeitbaren Karte - nie im Spielordner. „Datei > Importieren...“ nimmt auch eine .bsp.
- Karten-Editor: das Originallicht behalten. „Kompilieren > Nur Entities aktualisieren“ schreibt geänderte Entities in die geöffnete Karte zurück; Geometrie, Lightmaps und Lichtgitter bleiben Byte für Byte, wie sie waren. Wurden Brushes oder Patches geändert, sagt es das, statt zu bauen.
- Karten-Editor: alle Plugins von NetRadiant-Custom mit ihren Menüs und Dialogen - bobToolz (23 Befehle und seine Werkzeugleiste: Treppen, Türen, Blendentüren, Polygone, Path Plotter, Tree Planter, Splines, Vis Viewer, Patch-Werkzeuge und mehr), brushexport2 (die Auswahl als Wavefront-.obj mit .mtl exportieren), MeshTex (Texturmaßstab und -ausrichtung auf Patches), ShaderPlug, SunPlug und UFO:AI.
- Karten-Editor: „Datei > Beenden“ fragt bei einer geänderten Karte erst, ob gespeichert werden soll, wie NetRadiant-Custom.
- Karten-Editor: Beschriftungen auf den Einstellungsseiten und in den Plugin-Dialogen werden auf Deutsch, Chinesisch und Japanisch nicht mehr abgeschnitten; der Surface Inspector hat die Breite von NetRadiant-Custom.
- Karten-Editor: ein möglicher Absturz in der Kameraansicht von Karten mit mehreren Modellen behoben.
- Handbuch: neue Seiten „Karten-Editor: fertige Karten öffnen und ändern“ und „Karten-Editor: Plugins“ in vier Sprachen.

## 中文

- 地图编辑器:打开已完成的地图。“文件 > 打开 .bsp...”列出游戏文件夹中所有已编译的地图(包括游戏 .pk3 中的),并用 q3map2 在工作文件夹中把它还原成可编辑的地图——绝不写入游戏文件夹。“文件 > 导入...”也接受 .bsp。
- 地图编辑器:保留原始光照。“编译 > 仅更新实体”把修改过的实体写回打开的地图;几何体、光照贴图和光照网格逐字节保持原样。如果笔刷或曲面片有改动,它会给出提示而不进行编译。
- 地图编辑器:NetRadiant-Custom 的全部插件及其菜单和对话框——bobToolz(23 条命令及其工具栏:楼梯、门、光圈门和多边形生成器、Path Plotter、Tree Planter、样条、Vis Viewer、曲面片工具等)、brushexport2(把所选内容导出为带 .mtl 的 Wavefront .obj)、MeshTex(曲面片上的纹理缩放与对齐)、ShaderPlug、SunPlug 和 UFO:AI。
- 地图编辑器:与 NetRadiant-Custom 一样,“文件 > 退出”会先询问是否保存已修改的地图。
- 地图编辑器:首选项页面和插件对话框中的标签在德语、中文和日语下不再被截断;表面检查器的宽度与 NetRadiant-Custom 相同。
- 地图编辑器:修复了包含多个模型的地图在摄像机视图中可能崩溃的问题。
- 手册:新增四种语言的“地图编辑器:打开并修改已完成的地图”和“地图编辑器:插件”页。

## 日本語

- マップエディター:完成したマップを開く。「ファイル > .bsp を開く...」で、ゲームフォルダー(ゲームの .pk3 の中も含む)のコンパイル済みマップをすべて一覧にし、作業フォルダーで q3map2 を使って編集できるマップに戻します。ゲームフォルダーには書き込みません。「ファイル > インポート...」でも .bsp を読み込めます。
- マップエディター:元のライトを残す。「ビルド > エンティティのみ更新」で、変更したエンティティを開いたマップに書き戻します。ジオメトリ、ライトマップ、ライトグリッドはバイト単位で元のままです。ブラシやパッチが変更されている場合は、ビルドせずにそのことを知らせます。
- マップエディター:NetRadiant-Custom のすべてのプラグインを、メニューとダイアログ付きで搭載。bobToolz(23 のコマンドとツールバー:階段・ドア・絞り式ドア・多角形の作成、Path Plotter、Tree Planter、スプライン、Vis Viewer、パッチツールなど)、brushexport2(選択範囲を .mtl 付きの Wavefront .obj として書き出し)、MeshTex(パッチのテクスチャのスケールと位置合わせ)、ShaderPlug、SunPlug、UFO:AI。
- マップエディター:NetRadiant-Custom と同じく、「ファイル > 終了」で変更されたマップを保存するかどうかを先に確認します。
- マップエディター:設定ページとプラグインのダイアログで、ドイツ語・中国語・日本語のラベルが切れなくなりました。サーフェスインスペクターの幅が NetRadiant-Custom と同じになりました。
- マップエディター:複数のモデルがあるマップのカメラビューで起こりうるクラッシュを修正しました。
- ガイド:新しいページ「マップエディター:完成したマップを開いて変更する」と「マップエディター:プラグイン」(4 言語)。
