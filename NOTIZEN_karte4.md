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
