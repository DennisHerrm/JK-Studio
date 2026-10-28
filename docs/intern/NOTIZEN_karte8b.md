# Karten-Editor Stufe 8b: Terrain, freie Split-Fenster, Im Editor spielen, Logik, Ansichtsmodi, Spiegel, Undo-Verlauf, misc_model, GPU-Schattentests

Arbeitskopie `Y:\karte`, Zweig `karte1`, ab `983752cb` (Ende Stufe 8a).
Auftrag (Koordinator, Wünsche shank und Tester): zwölf Punkte der Reihe nach,
je Punkt eine gezielte Wirkungsprüfung (rot -> grün), ctest und Commit.
Grundregeln wie immer: Aussehen und Verhalten wie NetRadiant-Custom (Tag
20260114); eigene Funktionen mit eigenem Menüeintrag oder Schalter und dem
Hinweis "nicht in NetRadiant-Custom", Vorgabe so, dass sich der Editor wie
NRC verhält; "wie im Spiel" über die vorhandenen 1:1-Ports; exakt heißt
Byte-Vergleich im Test.

## Commits

| Punkt | Commit | Inhalt |
|---|---|---|
| 1 | cd8106d8 | Terrain Generator aus NRC master (Plugins > TerrainGenerator), fester Startwert als eigene Ergänzung |
| 2 | fe668314 | Freie Split-Fenster (eigener Schalter, Vorgabe aus = NRC) |
| 3 | b167e010, 41d2eb38 | Im Editor spielen: Pmove-Spieler und Kollision direkt aus der Karte (Kern), View-Menü und F6 |
| 4 | 895e0f31 | Logik-Ansicht der Entity-Verknüpfungen |
| 5 | 1894d0b5 | Ansichtsmodi der Kamera: Lightmap-Dichte, Kollision, Sichtbarkeit (PVS), Dreieckslast |
| 6 | 44a83aa5 | Spiegel-Modus (Gegenstück live jenseits einer X/Y/Z-Ebene) |
| 7 | a7bf37fb | Undo-Verlauf als Liste |
| 8 | 1be28c24 | misc_model-Kandidaten aus allen Modellordnern (Modell-Index mit Zwischenspeicher) |
| 9 | 45926d6c | q3map2 -light -bhedgpu: bitgleiche Schattentests auf der GPU - kein Gewinn, darum aus |
| 10 | f1c0cb91 | Handbuch: neue Seite "Terrain, Spielen, Logik, Spiegel und mehr", Unterschiede-Seite, alle Kartenbilder in vier Sprachen (der Entwurf dieser Notizen ging versehentlich mit) |
| 11 | a0e1c8d4 | Behebungen der Schlussrunde: Dialog "Add to compiled map" zwischen den Tests schließen, Menügrößen mit den Einträgen der Stufe 8b, Abdeckung aus dem Schlusslauf |
| 12 | (Commit dieser Notizen) | Notizen, Changelog |

## Punkt 1: Terrain Generator

- Was: das Plugin TerrainGenerator aus dem NRC-Quelltext master (nach dem
  Release 20260114) 1:1 portiert (`src/karte/terraingen.cpp`, Dialog
  `gui/app_karte_terrain.cpp`): Rauschen (Perlin, Simplex, Zufall), Grundformen,
  Terrassen, Raster, unregelmäßiges Gitter, Use Selection / Manual Size, Textur
  aus dem Texturbrowser, ein Rückgängig-Schritt "terrainGenerator.generate".
  Eigene Ergänzung (eigene Zeile, Vorgabe aus): "Seed: Fixed" startet die
  Zufallsfolge mit einem festen Wert; aus läuft sie weiter wie in NRC
  (std::rand).
- Test: ctest `karte_terrain` übersetzt die unveränderten NRC-Dateien
  (`tests/karte/terrain_nrc/`, mit Shim) und vergleicht: 495 Wertekombinationen,
  1,3 Mio. Flächen Bit für Bit gleich. Selbsttest stufe8b/terrain 33 OK (Menü,
  Dialog, Generate, Undo byte-gleich, gleicher Startwert = gleiche Karte, Fehlerkästen,
  Pick, About).
- Abweichung: keine (der feste Startwert ist zusätzlich und aus).

## Punkt 2: Freie Split-Fenster

- Was: Preferences > Interface > Layout > "Free split windows" (Layout Split,
  Vorgabe aus): jedes der vier Fenster bekommt ein Eckmenü (Kamera, XY, XZ, YZ,
  auch doppelt), Strg+Tab wechselt den Typ des aktiven 2D-Fensters, Teiler und
  Belegung werden gespeichert (BhedSplitFree, BhedSplitSlots, BhedViewTypes,
  BhedSplitH/V/V2).
- Test: Selbsttest stufe8b/split 22 OK und drei Neustart-Läufe (Belegung und
  Größen nach dem Neustart, aus = Lage wie NRC).
- Fehler gefunden und behoben: das Menü bildete die Wahl auf NRCs VIEWTYPE-Enum
  ab (YZ=0, XZ=1, XY=2) - die Fenster zeigten die falsche Ansicht; rot im Test
  "Platzmenü > XY", grün nach der ausdrücklichen Zuordnung.

## Punkt 3: Im Editor spielen

- Was: View > "Play in editor" (F6, keine NRC-Belegung) und "Play: start at
  info_player_start": der Pmove-Port des Programms (`src/navlauf.cpp`
  navSpielerPmove) mit Kollision direkt aus Brushes und Patches
  (`src/karte/spielkollision.cpp`: surfaceparms des Spiels wie q3map2,
  SetBrushContents, AddBrushBevels, SnapPlaneImproved; Patches wie
  CM_GeneratePatchCollide). Regelwerk JKA/Movie Duels bzw. Jedi Outcast.
  W/A/S/D, Sprung, Ducken, Gehen, Noclip, Esc stellt die Kamera wieder her.
- Test: ctest `karte_spielen`: 20 000 Spurtests gegen die gebaute
  spielraum.bsp gleich (ausgenommen und gezählt: 67 startsolid-Fälle, deren
  Inhalt/Normale von der Reihenfolge abhängt), Läufe Bild für Bild gleich, Sprung
  31,6224 hoch. Selbsttest stufe8b/spielen 23 OK (echte Tasten, Treppen,
  playerclip, Noclip, Esc, Spawn-Start).
- Abweichung: nur der Spieler bewegt sich (keine Mover, Trigger, NPCs).

## Punkt 4: Logik-Ansicht

- Was: View > "Logic view...": Graph aller Verknüpfungen (target, target2..5,
  NPC_target, Türziele, jeder *target*-Schlüssel der Definitionen und der
  OpenJK-Felder), Skript-Schlüssel im Kasten, fehlende Ziele rot; Klick wählt
  und zentriert, Doppelklick öffnet das ICARUS-Skript im Skript-Editor.
- Test: ctest `karte_logik` (Kanten, fehlende Ziele, Skripte, Spalten, Kreis,
  gleiche Lage bei zwei Läufen); Testerkarte jeditemple_garden: 559 Knoten,
  347 Kanten, 2 fehlende Ziele, 1,4 ms. Selbsttest stufe8b/logik 17 OK.

## Punkt 5: Ansichtsmodi der Kamera

- Was: View > "Camera view modes": Lightmap-Dichte (Luxelgröße je Fläche wie
  q3map2: q3map_lightmapSampleSize, _samplesize, _lightmapscale, -samplesize,
  vergrößert bis 128 Luxel), Kollision (Inhalte wie q3map2, sichtbare Wände ohne
  Spieler-Kollision rot), Sichtbarkeit (PVS aus der .bsp), Dreieckslast
  (Wärmekarte aus der .bsp).
- Test: Selbsttest stufe8b/ansichten 16 OK: Luxel 16 / 32 (func_group
  _lightmapscale 2) / 33 (4096er Platte vergrößert), PVS 9 von 20 Clustern =
  die Bits im Vis-Lump, Last = Gegenrechnung aus der .bsp. Nach der Pause im
  Auftrag wiederholt: 16 OK, 0 FEHLER.
- Abweichung: q3map2 fasst Flächen zu Roh-Lightmaps zusammen und vergrößert
  dann für alle - die Dichte-Anzeige rechnet je Fläche.

## Punkt 6: Spiegel-Modus

- Was: View > "Mirror mode": Aus / Spiegelebene X / Y / Z, Ebene durch den
  Ursprung oder durch die Mitte der Auswahl (aufs Raster), magentafarbene Linie
  in 2D. Jede Änderung an der Auswahl wird auf das Gegenstück geschrieben -
  Pinsel und Patches einzeln, Punkt-Entities und Gruppen als Ganzes, origin,
  angle (auch -1/-2) und angles gespiegelt, Texturen wie NRC-"Flip"
  (Brush::transform mit Texture Lock, Patch mit Zeilenumkehr). Ein vorhandenes
  genaues Spiegelbild wird als Paar erkannt (Form-Vergleich), sonst legt die
  erste Änderung eine Kopie an; Löschen nimmt das Gegenstück mit; Objekte
  symmetrisch auf der Ebene bekommen keins. Im selben Rückgängig-Schritt: live
  beim Ziehen (Abgleich direkt nach jeder Manipulator-Bewegung und jedes Bild),
  sonst über den neuen Haken `Rueckgaengig::vorAbschluss` kurz vor finish();
  außerhalb eines Befehls (Undo/Redo) wird nur der neue Stand gemerkt.
- Test: ctest `karte_spiegel` 32 Prüfungen: Pinsel verschieben -> Gegenstück
  folgt genau (Text gleich dem Spiegelbild), offener Befehl live, ein Schritt,
  Undo x3 und Redo x3 byte-gleich, andere Seite bearbeiten, vorhandenes
  Spiegelbild als Paar, Löschen, Patch, angle/angles, func_door als Ganzes,
  Achsen Y/Z, aus. Rot ohne den Haken (kein Gegenstück). Selbsttest
  stufe8b/spiegel 22 OK (Maus-Ziehen in XY, Strg+Z/Strg+Y byte-gleich, Entity,
  alle Menüpunkte).
- Fehler gefunden und behoben: das Gegenstück hinkte beim Ziehen ein Bild nach
  (der Abgleich lief vor der Mausbewegung des Bildes) - rot im Selbsttest
  ("live 0"), behoben durch den Abgleich gleich nach `auswahlBewegen`, grün.
- Abweichung/Grenze: kopierte Entities behalten targetname/target.

## Punkt 7: Undo-Verlauf als Liste

- Was: View > "Undo history...": Anfang, alle Rückgängig-Schritte mit NRCs
  Befehlsnamen (ältester zuerst, aktueller Stand hervorgehoben), darunter grau
  die Wiederholen-Schritte; Klick springt mit so vielen Undo/Redo wie nötig
  (jedes über den normalen Befehl, mit Konsolenzeile). Kern:
  `Rueckgaengig::undoNamen`, `redoNamen`, `springen`.
- Test: ctest `karte_undoliste` 9 Prüfungen (Namen, Sprung zurück/vor/Anfang/
  Ende byte-gleich, außerhalb abgelehnt, neuer Befehl verwirft Redo).
  Selbsttest stufe8b/undoliste 17 OK (Menü, Zeilen, Klicks, hin und zurück
  byte-gleich).

## Punkt 8: misc_model aus allen Modellordnern

- Was: Modell-Index über alle Modelle des Spiels (je Netz Eckenzahl, Signatur
  der Dreiecksmenge wie Durchgang 1, Shader wie Durchgang 2) im
  Einstellungsordner (`karte_modellindex_<hash>.txt`, Schlüssel Archiv +
  Größe + Zeit); fehlende/veraltete Einträge lädt bhed_q3map2 -bhedmodels
  einmal. Kandidaten = Ordnerregel + jedes Modell mit passendem Netz oder
  genutztem Shader - genau die, die die Suche finden kann (außer forcemeta mit
  _remap).
- Messung (MD-Installation, 1557 Modelle; Dump aller Modelle 37,6 s einmalig,
  Index 459 KB, lesen 2 ms, Auswahl 7..27 ms):

  | Karte | vorher (Ordnerregel) | jetzt | Kandidaten | Suche |
  |---|---|---|---|---|
  | t2_trip | 0 | 13 | 0 -> 95 | 0,05 s |
  | yavin1 | 18 | 32 | 122 -> 169 | 0,27 s |
  | hoth2 | 5 | 32 | 92 -> 191 | 0,11 s |
  | kor1 | 52 | 52 | 12 -> 86 | 1,30 s |
  | jeditemple_garden | 265/265, 0 falsch | 265/265, 0 falsch | 11 -> 14 | 0,78 s |

  Dieselben Trefferzahlen wie eine Suche über alle 1557 Modelle. Im Editor:
  t2_trip erstes Öffnen 33,6 s (Index wird gebaut), zweites 3,3 s.
- Test: kartemodellrueck (Index schreiben/lesen, abgeschnitten abgelehnt,
  Signatur- und Shader-Regel; Messmodus `--alle`), Selbsttest
  stufe8b/modellindex 14 OK, stufe8/modelle weiter 17 OK.

## Punkt 9: GPU-Schattentests in bhed_q3map2 (bitgleich, aber ohne Gewinn - aus)

- Was: `-light -bhedgpu` (vor argsToInject entnommen, steht also nicht in der
  .bsp): die Entscheidung "verdeckt?" der ersten Abtastung von
  IlluminateRawLightmap (ein Strahl je Luxel und Licht, testAll = false) auf
  der GPU per D3D11-Compute-Shader; Licht bleibt CPU. Dieselbe Traversierung wie
  TraceLine_r/TraceTriangle, alles `precise` in derselben Reihenfolge (keine
  mad-Kontraktion), die beiden Divisionen genau gerundet in Ganzzahlen
  (D3D11-Division darf 2,5 ULP abweichen), eine Vorprüfung mit der
  Hardware-Division verwirft nur, was auch genau verworfen wird. Vorlauf nimmt
  die Strahlen auf (TraceLine merkt nur die Eingaben), Blöcke von bis zu 65 536
  Strahlen, Wiedergabe nach Byte-Vergleich der Eingaben; Alpha-Schatten/
  Lichtfilter, voller Stapel, belegte GPU -> CPU. d3d11.dll, dxgi.dll,
  d3dcompiler_47.dll per LoadLibrary (Windows-System-DLLs): keine neuen festen
  Importe (q3map2_importe unverändert: bcrypt, KERNEL32, WS2_32); der Adapter
  mit dem meisten Grafikspeicher.
- Pflichttest: alle 18 Lumps byte-gleich mit und ohne GPU auf mp__duel1,
  md_ch_ben, kejim_post, duel_pit, t1_inter (Schalter wie das Build-Menü;
  kejim_post und duel_pit auch auf der Intel UHD, alle auf der RTX 4070 Laptop),
  0 Eingabe-Abweichungen, 0 Strahlen von der GPU an die CPU zurückgegeben.
  ctest `q3map2_gpu` (eine Testkarte, -threads 1, rot mit absichtlich falschem
  Tiefentest).
- Messung LIGHT (Median aus 3, 32 Threads, RTX 4070 Laptop):

  | Karte | CPU | mit -bhedgpu | Strahlen auf der GPU | GPU-Zeit |
  |---|---|---|---|---|
  | mp__duel1 | 2,3 s | 3,2 s | 2,81 M | 0,46 s |
  | md_ch_ben | 3,1 s | 3,9 s | 0,04 M | 0,07 s |
  | kejim_post | 10,0 s | 11,3 s | 11,8 M | 1,62 s |
  | duel_pit | 0,7 s | 1,1 s | 0,95 M | 0,12 s |
  | t1_inter | 0,3 s | 0,8 s | 0 | - |

  Ein Thread, kejim_post: 31,0 s -> 32,8 s (24,3 M Strahlen, GPU 6,4 s). Die
  Strahlen der ersten Abtastung sind nur ein kleiner Teil von -light (Gitter,
  Supersampling, große Lightmaps über den Pool, Bounce, Nachbearbeitung), der
  Vorlauf kostet so viel, wie die GPU spart, und 32 CPU-Threads sind schnell.
  **Ehrliches Ergebnis: bitgleich, aber kein Gewinn - der Schalter bleibt aus
  und steht nicht im Build-Menü.**

## Punkt 10: Handbuch

- Neue Seite "Map editor: terrain, playing, logic, mirror and more"
  (`tools/handbuch/inhalt_karte8b.py`, vier Sprachen): View-Menü, Terrain
  Generator (Dialog und Ergebnis), freie Split-Fenster, Im Editor spielen,
  Logik-Ansicht, Ansichtsmodi (Dichte, Kollision), Spiegel-Modus, Undo-Verlauf,
  misc_model aus allen Modellordnern (Tabelle), GPU-Schattentests (aus, mit
  Zahlen). Unterschiede-Seite: "Eigene Editor-Werkzeuge (Stufe 8b)".
- Bilder: Selbsttest `BHED_HB_NUR=karte8b` (neu) und alle Kartenbild-Läufe
  (karte8, karte7, karteneditor, kartebauen, karte4, karte5) in vier Sprachen
  neu (View- und Plugins-Menü und die Einstellungen haben neue Einträge).
- `gen_handbuch.py` und `--pruefen` ohne Fehler.

## Abweichungen von NRC (neu in Stufe 8b)

- Alles außer dem Terrain Generator ist eigene Funktion mit eigenem
  Menüeintrag/Schalter und dem Hinweis "nicht in NetRadiant-Custom"; Vorgabe
  aus (Split frei, Spiegel, Ansichtsmodi), Spielen/Logik/Undo-Liste nur auf
  Aufruf.
- Terrain Generator: aus NRC master (nicht im Release 20260114, Hinweis im
  Dialog); fester Startwert zusätzlich.
- Ansichtsmodus Lightmap-Dichte: je Fläche statt je Roh-Lightmap.
- Spiegel-Modus: kopierte Entities behalten targetname/target; Objekte
  symmetrisch auf der Ebene bekommen kein Gegenstück.
- bhed_q3map2 `-bhedgpu`: neuer Schalter, aus, nicht im Build-Menü; nimmt in
  der Wiedergabe für Verdeckt-Ergebnisse die Lage des getroffenen Dreiecks
  (hit) des GPU-Wegs, `compileFlags` werden in diesem Weg nicht gesammelt -
  beides liest der einzige Nutzer (erste Abtastung von IlluminateRawLightmap,
  testAll = false) nicht; die Lumps sind byte-gleich.

## Schlussrunde

Vorlage `SP/s8/ende_s8.sh` -> `SP/s8b/ende_s8b.sh` (Idle, 8-GB-Wächter, ein
Prozess zur Zeit), Summe `SP/s8b/ende_summe.txt`. Erster Durchgang auf
f1c0cb91 (Handbuch-Commit):

| Schritt | Ergebnis |
|---|---|
| Bau | ok |
| ctest | 131/131 (171 s) |
| karteneditor voll (mit duel_mos_espa + MD-BSP) | 3068 OK, 165 FEHLER (898 s, Spitze 1129 MB): das Fenster "Add to compiled map" der Stufe 8 blieb offen und lag über der XY-Ansicht; der Zieh-Test des Spiegel-Modus klickte hinein, danach kam kein Klick mehr an (Folgefehler in 8b und in den Menüfotos) -> `karteDialogeZu` schließt jetzt auch diesen Dialog, die Logik-Ansicht und den Undo-Verlauf |
| Modus 1 | 2645 OK, 0 FEHLER (437 s) |
| karte, kartenfunde, fix604b, laserfalle, geschuetzanim, zielstrahl, effektgrund, figurdeform, jk2, tuerlauf, kamera, missionsbau, szenen, ton, npcklasse, gluehen, gras, spulen, figurstufen, missionsliste, rest605, modellimfesten | alle 0 FEHLER (227, 53, 32, 8, 3, 2, 1, 1, 15, 13, 95, 95, 26, 34, 12, 8, 13, 13, 8, 11, 6, 1 OK) |
| Leistung gegen rc605 (md_am_sith, je 2 Läufe) | Vorbereitung rc605 6279/6246 ms, Stufe 8b 6310/6440 ms; Bild Mittel 13/13 gegen 14/14 ms, Flug 8,1/8,7 gegen 8,6/8,6 ms, dritte Szene 10/10 gegen 10/9 ms, max. 207/186 gegen 199/195 ms - gleich im Rahmen der Streuung (die Szenen-Vorschau berührt Stufe 8b nicht) |
| Kartenprüfung (278 MD-Karten, gegen Stufe 8a) | alle Summen gleich (0 Abstürze, 0 nicht geladen, 0 D3D-Meldungen, 1 -> 1 nicht gezeichnete Entity) |
| sauberer Neubau | 0 Fehler, 130 Warnungen (467 s; Stufe 8a: 130) |

Nach der Behebung: karteneditor voll 3111 OK, 2 FEHLER (897 s) - die
Größenprüfung der Menüs View (292 x 611 mit den sechs neuen Einträgen der
Stufe 8b) und Plugins (162 x 202 mit TerrainGenerator) nannte noch die alten
Maße -> Erwartung mit Begründung angepasst, gezielt BHED_KARTE_NUR=fotos
78 OK, 0 FEHLER; danach der ganze Lauf: **3113 OK, 0 FEHLER** (924 s, Spitze
1033 MB). Abdeckung aus diesem Lauf: 980 Einträge, 976 mit Wirkungsprüfung,
4 begründete Ausnahmen, 0 ohne.

## Offene Punkte

- GPU-Schattentests: bitgleich, aber ohne Gewinn. Ein Gewinn bräuchte mehr
  als die erste Abtastung: die großen Lightmaps (Pool-Weg, Stufe 7), die
  Sonnen-/Himmelsstrahlen (testAll mit Himmelsflags und skyIndices), das
  Lichtgitter, das Supersampling und Bounce - und den Vorlauf ohne doppelte
  LightContributionToSample-Rechnung. Erst danach lohnt sich ein neuer Versuch.
- Spiegel-Modus: nur achsparallele Ebenen; keine Dreh-Symmetrie; Namen der
  kopierten Entities bleiben gleich.
- Im Editor spielen: nur der Spieler (keine Türen, Mover, Trigger, NPCs,
  Waffen).
- Logik-Ansicht: zeigt und wählt, bearbeitet die Verknüpfungen aber nicht.
- Freie Split-Fenster nur im Layout Split.
- misc_model-Index: forcemeta-Modelle mit _remap findet auch der Index nicht
  (ihr Shader steht nicht in der .bsp); das erste Öffnen baut den Index
  (bei der MD-Installation etwa 30 s) ohne eigene Fortschrittszeile.
- Ansichtsmodi: Sichtbarkeit und Dreieckslast brauchen die gebaute .bsp;
  Dichte je Fläche (siehe Abweichungen).

## Was Stufe 8c braucht

- Tutorial "Erste eigene Karte" (Schritt für Schritt im Programm, wie die
  Tutorials des Skript-Teils), dazu ein großer Gesamttest über alle
  Karten-Werkzeuge.
- Zusammenführung mit dem Hauptprojekt vorbereiten: Zweig `karte1` gegen
  `main`, Konflikte (selbsttest.cpp, CMakeLists.txt, i18n), der Bau mit
  `-DBEHAVED_Q3MAP2_LLVM` (clang-cl) im Release-Weg, Handbuch und
  Changelogs zusammen.
- Umbenennen in "JK Studio": Programmname, Updater (Kanal, alte Fassungen
  erkennen), Einstellungsordner (Übernahme der alten Einstellungen),
  exe-Name, Handbuch und Texte in vier Sprachen.
- GPU-Plan Schritt 2: das ganze Licht-Backen auf der GPU (nicht nur die
  Schattentests) mit einem Abweichungsbericht gegen die CPU (welche Luxel um
  wie viel), dazu die GPU-Vorschau mit Bounce im Editor.

## Changelog

## English

- Map editor: Plugins > TerrainGenerator > Generate Terrain - the terrain generator from the current NetRadiant-Custom source, the same terrain for the same values; an optional fixed seed gives the same terrain again.
- Map editor: free split windows (Preferences > Interface > Layout, split layout) - every window can show the camera, XY, XZ or YZ, also twice; Ctrl+Tab and the splitter positions are kept.
- Map editor: View > Play in editor (F6) - walk through the open map without compiling, with the game's movement and collision straight from the brushes and patches; start at the camera or at info_player_start, Esc ends.
- Map editor: View > Logic view - every link between the entities as a graph (targets, NPC targets, door targets, script keys); click selects and centers, double-click opens the ICARUS script.
- Map editor: View > Camera view modes - lightmap density as q3map2 computes it, collision by contents, visibility (PVS) and triangle load from the compiled .bsp.
- Map editor: View > Mirror mode - every change to the selection is mirrored live onto its counterpart across an X, Y or Z plane (brushes, patches, entities with origin, angle and angles), in the same undo step.
- Map editor: View > Undo history - all undo and redo steps with their command names; a click jumps to that state.
- Map editor: File > Open .bsp finds misc_model from all model folders of the game with a cached model index (t2_trip: 0 -> 13 models, jeditemple_garden still 265 of 265).
- bhed_q3map2: `-light -bhedgpu` runs the shadow tests on the graphics card with byte-identical results; it is off and not in the Build menu because it was not faster on the measured computer.
- Guide: new page "Map editor: terrain, playing, logic, mirror and more"; all map pictures updated in four languages.

## Deutsch

- Karten-Editor: Plugins > TerrainGenerator > Generate Terrain - der Terrain-Generator aus dem aktuellen Quelltext von NetRadiant-Custom, bei gleichen Werten dasselbe Terrain; ein wahlweise fester Startwert gibt dasselbe Terrain wieder.
- Karten-Editor: freie Split-Fenster (Preferences > Interface > Layout, Layout Split) - jedes Fenster kann Kamera, XY, XZ oder YZ zeigen, auch doppelt; Strg+Tab und die Lage der Teiler bleiben.
- Karten-Editor: View > Im Editor spielen (F6) - ohne Kompilieren durch die offene Karte laufen, mit der Bewegung des Spiels und der Kollision direkt aus Brushes und Patches; Start an der Kamera oder an info_player_start, Esc beendet.
- Karten-Editor: View > Logik-Ansicht - jede Verknüpfung zwischen den Entities als Graph (Ziele, NPC-Ziele, Türziele, Skript-Schlüssel); Klick wählt und zentriert, Doppelklick öffnet das ICARUS-Skript.
- Karten-Editor: View > Ansichtsmodi der Kamera - Lightmap-Dichte wie q3map2 sie rechnet, Kollision nach Inhalt, Sichtbarkeit (PVS) und Dreieckslast aus der gebauten .bsp.
- Karten-Editor: View > Spiegel-Modus - jede Änderung an der Auswahl wird live auf das Gegenstück jenseits einer X-, Y- oder Z-Ebene gespiegelt (Brushes, Patches, Entities mit origin, angle und angles), im selben Rückgängig-Schritt.
- Karten-Editor: View > Undo-Verlauf - alle Rückgängig- und Wiederholen-Schritte mit ihren Befehlsnamen; ein Klick springt zu diesem Stand.
- Karten-Editor: File > Open .bsp findet misc_model aus allen Modellordnern des Spiels mit einem zwischengespeicherten Modell-Index (t2_trip: 0 -> 13 Modelle, jeditemple_garden weiter 265 von 265).
- bhed_q3map2: `-light -bhedgpu` rechnet die Schattentests auf der Grafikkarte mit Byte-gleichem Ergebnis; der Schalter ist aus und steht nicht im Build-Menü, weil er auf dem gemessenen Rechner nicht schneller war.
- Handbuch: neue Seite „Karten-Editor: Terrain, Spielen, Logik, Spiegel und mehr“; alle Kartenbilder in vier Sprachen erneuert.

## 中文

- 地图编辑器:Plugins > TerrainGenerator > Generate Terrain——来自 NetRadiant-Custom 当前源代码的地形生成器,相同数值得到相同地形;可选的固定种子可再次得到同一地形。
- 地图编辑器:自由分屏窗口(Preferences > Interface > Layout,Split 布局)——每个窗口都可以显示摄像机、XY、XZ 或 YZ,也可重复;Ctrl+Tab 和分隔条位置会保留。
- 地图编辑器:View > 在编辑器中试玩(F6)——无需编译即可在打开的地图中行走,使用游戏的移动方式,碰撞直接来自画刷和面片;从摄像机或 info_player_start 开始,Esc 结束。
- 地图编辑器:View > 逻辑视图——以图的形式显示实体之间的每个连接(目标、NPC 目标、门的目标、脚本键);单击选中并居中,双击打开 ICARUS 脚本。
- 地图编辑器:View > 摄像机视图模式——按 q3map2 的计算显示光照贴图密度、按内容显示碰撞,以及根据已编译的 .bsp 显示可见性(PVS)和三角形负载。
- 地图编辑器:View > 镜像模式——对选择的每次修改都会实时镜像到 X、Y 或 Z 平面另一侧的对应物上(画刷、面片、实体及其 origin、angle 和 angles),在同一个撤销步骤中。
- 地图编辑器:View > 撤销历史——列出所有撤销和重做步骤及其命令名称;单击即可跳到该状态。
- 地图编辑器:File > Open .bsp 借助缓存的模型索引从游戏的所有模型文件夹中查找 misc_model(t2_trip:0 -> 13 个模型,jeditemple_garden 仍为 265/265)。
- bhed_q3map2:`-light -bhedgpu` 在显卡上进行阴影测试,结果逐字节相同;由于在测量所用的电脑上没有更快,该开关保持关闭,也不在 Build 菜单中。
- 手册:新增页面“地图编辑器:地形、试玩、逻辑、镜像及更多”;所有地图图片以四种语言更新。

## 日本語

- マップエディター:Plugins > TerrainGenerator > Generate Terrain - NetRadiant-Custom の現在のソースにある地形ジェネレーター。同じ値なら同じ地形になり、固定シードを選べば同じ地形をもう一度作れます。
- マップエディター:自由な分割ウィンドウ(Preferences > Interface > Layout、Split レイアウト)- 各ウィンドウにカメラ・XY・XZ・YZ を表示でき、重複も可能。Ctrl+Tab と仕切りの位置は保たれます。
- マップエディター:View > エディター内でプレイ(F6)- コンパイルせずに開いているマップの中を歩けます。動きはゲームと同じで、衝突はブラシとパッチから直接作られます。カメラか info_player_start から始まり、Esc で終了します。
- マップエディター:View > ロジックビュー - エンティティ同士のつながりをすべてグラフで表示(ターゲット、NPC ターゲット、ドアのターゲット、スクリプトキー)。クリックで選択して中央に、ダブルクリックで ICARUS スクリプトを開きます。
- マップエディター:View > カメラの表示モード - q3map2 の計算どおりのライトマップ密度、内容ごとの衝突、コンパイル済み .bsp からの可視性(PVS)と三角形の負荷。
- マップエディター:View > ミラーモード - 選択への変更を X・Y・Z 平面の反対側にある対のオブジェクトへリアルタイムにミラーします(ブラシ、パッチ、origin・angle・angles を含むエンティティ)。元に戻すは 1 ステップです。
- マップエディター:View > 元に戻す履歴 - 元に戻す/やり直しのステップをコマンド名つきですべて表示し、クリックでその状態に移ります。
- マップエディター:File > Open .bsp がキャッシュしたモデルインデックスでゲームのすべてのモデルフォルダーから misc_model を見つけます(t2_trip:0 -> 13 モデル、jeditemple_garden は引き続き 265/265)。
- bhed_q3map2:`-light -bhedgpu` は影のテストをグラフィックカードで行い、結果はバイト単位で同じです。測定したコンピューターでは速くならなかったため、スイッチはオフで Build メニューにもありません。
- ガイド:新しいページ「マップエディター:地形・プレイ・ロジック・ミラーなど」。すべてのマップ画像を 4 言語で更新しました。
