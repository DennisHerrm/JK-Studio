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
| 10 | (Handbuch-Commit) | Handbuch: neue Seite "Terrain, Spielen, Logik, Spiegel und mehr", Unterschiede-Seite, alle Kartenbilder in vier Sprachen |
| 11/12 | (Commit dieser Notizen) | Schlussrunde, Notizen, Changelog |

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
