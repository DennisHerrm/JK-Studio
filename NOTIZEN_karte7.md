# Karten-Editor Stufe 7: Tempo und Echtzeit-Licht

Arbeitskopie `Y:\karte`, ab `ebb2594` (Ende Stufe 6).
Auftrag (shank): "dass es schneller ist aber genauso gut", "alles was wir
optimieren können", Licht in Echtzeit. Grundregeln: gleiches Ergebnis mit Test
belegt, jede Verbesserung vorher/nachher gemessen (Median aus 3, Idle, dieselben
Karten), das Größte zuerst; Echtzeit-Licht ist nur Vorschau (beschriftet, nie
gespeichert), das Ergebnis kommt immer aus bhed_q3map2; Aussehen und Bedienung
NRC-treu, eigene Funktionen mit eigenen Menüeinträgen. Dazu die Zusatzpunkte
6b-6f des Koordinators (Wünsche shank, Fehlerberichte des Testers).

## Commits

| Punkt | Commit | Inhalt |
|---|---|---|
| 1 | 7aa5b07 | Messliste: `tools/karte/tempo.py` (q3map2, editor, vorschau, tabelle, handbuch), Selbsttest-Teil `tempo`, Abschnittszeiten in q3map2 (`BHED_Q3_ZEITEN`) |
| 2 | d0c7737, e57c972, a621d20 | q3map2 -light über alle Kerne (Lightmaps, Gitter-Vorlauf, Lichtgitter-Zwillinge), -bsp T-Junction-Suche parallel; Zeilenverweise der Grenzentabelle |
| 3 | (kein Commit) | -vis: zwei deterministische Varianten mit mehreren Threads gemessen und verworfen, NRCs Regel bleibt (unten) |
| 4 | 74d044b, f4988e7, a96822d | Editor: Auswahl umkehren ohne quadratische Suche, NPC-Dateien des Optimierers im Faden, Auswahl::gewaehlt in konstanter Zeit |
| 5 | 10897a5 | Echtzeit-Licht in der Kamera, Schnell-Backen im Hintergrund |
| 6 | (kein Commit) | q3map2-Speicherlecks GL#95: gemessen, begründet belassen (unten) |
| 6b | f66fda0 | Menüs im Stil des Programms, Programm-Einstellungen in View, Knopfgröße, Strg+Rad im Map-Editor aus |
| 6c | 4317437 | Freelook: gleiche Drehung wie NRC bei jeder Bildrate (Tester: "terribly low mouse sensitivity") |
| 6d | 3446fcf | eindeutige ImGui-IDs (Texturbaum "Asjc", Leistenmenü, Entity-Menü), ID-Konflikte als Selbsttest-FEHLER, Warnung im Release aus |
| 6e | a26f069 | Open .bsp: hint/areaportal/skip wieder structural; Kamera im zweiseitigen Brush wie NRC; was eine .bsp nicht mehr enthält |
| 6f | 588f215 | Spiel vor der Karte laden, offene Karte folgt einem Neuladen; Fenstertitel wie NRC |
| 7 | 65916a9, a6d1cf95 | Handbuch: Seite "Tempo und Echtzeit-Licht", "Unterschiede zu NRC" ergänzt, alle Kartenbilder neu (4 Sprachen); Tempo-Tabellen aus der Messliste |
| 8/9 | 99fb153, (Commit dieser Notizen) | tempo.py handbuch; Schlussrunde, Messliste, Notizen, Changelog |

## Messliste vorher/nachher

Vorher = Stufe 6 (`laeufe/s7_basis`: behaved mit dem Tempo-Teil, q3map2 von
ebb2594), nachher = Stand der Schlussrunde, NRC = q3map2.exe von NRC 20260114.
Laptop mit 32 Threads, Idle-Priorität, Median aus 3 Läufen, ohne andere eigene
Läufe (`SP/s7/messen.sh`, Rohdaten `laeufe/mess7/*.json`).

### q3map2 (Sekunden)

| Karte | Stufe | Threads | NRC | Stufe 6 | jetzt | jetzt gegen Stufe 6 |
|---|---|---|---|---|---|---|
| duel_pit | BSP | alle | 0.20 | 0.20 | 0.20 | -0 % |
| duel_pit | VIS | alle | 0.15 | 0.10 | 0.10 | +0 % |
| duel_pit | LIGHT | alle | 0.66 | 0.66 | 0.61 | -8 % |
| duel_pit | BSP | 1 | 0.20 | 0.20 | 0.20 | -0 % |
| duel_pit | VIS | 1 | 0.15 | 0.15 | 0.15 | +0 % |
| duel_pit | LIGHT | 1 | 3.02 | 3.33 | 3.08 | -8 % |
| mp__duel1 | BSP | alle | 0.36 | 0.31 | 0.31 | -1 % |
| mp__duel1 | VIS | alle | 0.15 | 0.11 | 0.10 | -2 % |
| mp__duel1 | LIGHT | alle | 2.53 | 2.99 | 1.92 | -36 % |
| mp__duel1 | BSP | 1 | 0.35 | 0.31 | 0.31 | +0 % |
| mp__duel1 | VIS | 1 | 0.20 | 0.15 | 0.16 | +0 % |
| mp__duel1 | LIGHT | 1 | 8.72 | 9.68 | 9.63 | -1 % |
| t1_inter | BSP | alle | 0.31 | 0.26 | 0.26 | -1 % |
| t1_inter | VIS | alle | 0.15 | 0.10 | 0.10 | -0 % |
| t1_inter | LIGHT | alle | 0.31 | 0.26 | 0.31 | +21 % |
| t1_inter | BSP | 1 | 0.30 | 0.25 | 0.26 | +0 % |
| t1_inter | VIS | 1 | 0.15 | 0.11 | 0.10 | -4 % |
| t1_inter | LIGHT | 1 | 0.31 | 0.31 | 0.31 | -0 % |
| md_ch_ben | BSP | alle | 0.81 | 0.71 | 0.76 | +7 % |
| md_ch_ben | VIS | alle | 2.77 | 2.27 | 2.23 | -2 % |
| md_ch_ben | LIGHT | alle | 3.73 | 4.24 | 2.58 | -39 % |
| md_ch_ben | BSP | 1 | 0.86 | 0.71 | 0.71 | +0 % |
| md_ch_ben | VIS | 1 | 30.53 | 24.90 | 24.60 | -1 % |
| md_ch_ben | LIGHT | 1 | 12.66 | 12.65 | 12.76 | +1 % |
| kejim_post | BSP | alle | 4.74 | 4.59 | 4.14 | -10 % |
| kejim_post | VIS | alle | 745.40 | 621.47 | 573.70 | -8 % |
| kejim_post | LIGHT | alle | 9.18 | 10.28 | 7.21 | -30 % |
| duel_mos_espa | BSP | alle | 53.35 | 44.86 | 19.57 | -56 % |

### Editor und Vorschau (Millisekunden, Selbsttest-Teil `tempo` bzw. Szenen-Modus)

| Messung | Stufe 6 | jetzt |
|---|---|---|
| Alles auswaehlen (duel_mos_espa) | 29744.8 | 5.2 |
| .bsp oeffnen, dekompilieren (duel_mos_espa) | 7708.1 | 7980.6 |
| .bsp oeffnen, dekompilieren (md_am_sith) | 3287.1 | 3422.6 |
| .bsp oeffnen, dekompilieren (kejim_post) | 2010.9 | 2639.4 |
| Texturen bis fertig (md_am_sith) | 1928.6 | 2159.0 |
| Texturen bis fertig (duel_mos_espa) | 1638.8 | 1803.1 |
| Optimierer Pruefung (duel_mos_espa) | 1465.8 | 1336.5 |
| Vorschau: Szene vorbereiten (md_am_sith) | 1438.3 | 1397.2 |
| Alles auswaehlen (md_am_sith) | 1356.2 | 0.8 |
| Alles auswaehlen (kejim_post) | 1341.4 | 0.7 |
| .map laden (duel_mos_espa) | 1312.8 | 1292.0 |
| .map speichern (duel_mos_espa) | 815.8 | 745.4 |
| .map laden (md_am_sith) | 805.6 | 827.7 |
| .map laden (kejim_post) | 662.5 | 717.4 |
| Programmstart bis erstes Bild | 390.6 | 393.8 |
| Rueckgaengig Verschieben 2, alles (duel_mos_espa) | 350.3 | 301.1 |
| Optimierer Pruefung (md_am_sith) | 303.7 | 257.4 |
| Rueckgaengig Verschieben, alles (duel_mos_espa) | 294.1 | 353.0 |
| Wiederholen Verschieben, alles (duel_mos_espa) | 286.0 | 309.6 |
| Auswahl verschieben, alles (duel_mos_espa) | 285.1 | 329.4 |
| Moduswechsel bis Spieldaten bereit | 273.3 | 258.1 |
| Vorschau: Bild Szene max | 210.0 | 227.0 |
| .map speichern (md_am_sith) | 175.9 | 189.6 |
| Rueckgaengig CSG Subtract (duel_mos_espa) | 171.1 | 207.4 |
| Optimierer Pruefung (kejim_post) | 162.4 | 201.8 |
| .map speichern (kejim_post) | 142.5 | 173.7 |
| Auswahl verschieben, alles (md_am_sith) | 62.9 | 68.0 |
| Auswahl verschieben, alles (kejim_post) | 53.2 | 72.0 |
| Optimierer Start (duel_mos_espa) | 48.6 | 39.2 |
| Wiederholen Verschieben, alles (md_am_sith) | 46.4 | 32.1 |
| CSG Subtract (duel_mos_espa) | 45.8 | 34.3 |
| Rueckgaengig Verschieben, alles (md_am_sith) | 33.3 | 32.6 |
| Rueckgaengig Verschieben 2, alles (md_am_sith) | 30.5 | 36.2 |
| Wiederholen Verschieben, alles (kejim_post) | 27.7 | 30.1 |
| Rueckgaengig Verschieben, alles (kejim_post) | 26.2 | 36.4 |
| Rueckgaengig Verschieben 2, alles (kejim_post) | 24.5 | 33.0 |
| Vorschau: Bild Flug max | 17.0 | 17.0 |
| Moduswechsel: erstes Bild | 16.3 | 26.9 |
| Vorschau: Bild Szene Mittel | 15.0 | 15.0 |
| Optimierer Start (md_am_sith) | 14.7 | 11.8 |
| CSG Subtract (md_am_sith) | 13.1 | 15.2 |
| Rueckgaengig CSG Subtract (md_am_sith) | 12.1 | 10.1 |
| Clipper Clip (duel_mos_espa) | 11.0 | 6.6 |
| Optimierer Start (kejim_post) | 10.2 | 13.1 |
| CSG Subtract (kejim_post) | 9.7 | 10.1 |
| Rueckgaengig CSG Subtract (kejim_post) | 8.6 | 7.9 |
| Optimierer groesstes Bild waehrend der Pruefung (duel_mos_espa) | 8.4 | 9.1 |
| Vorschau: Bild Flug Mittel | 8.2 | 8.1 |
| Optimierer groesstes Bild waehrend der Pruefung (md_am_sith) | 4.6 | 4.5 |
| Optimierer groesstes Bild waehrend der Pruefung (kejim_post) | 4.4 | 8.5 |
| Texturen bis fertig (kejim_post) | 4.2 | 821.6 |
| erstes Bild nach dem Laden (duel_mos_espa) | 3.1 | 3.4 |
| Clipper Clip (md_am_sith) | 2.2 | 3.8 |
| erstes Bild nach dem Laden (md_am_sith) | 2.1 | 1.7 |
| Clipper Clip (kejim_post) | 1.6 | 3.4 |
| erstes Bild nach dem Laden (kejim_post) | 1.0 | 1.6 |

Auffällig: die Lightmaps (-light, alle Threads) 30-40 % schneller (mp__duel1
2,99 -> 1,92 s, md_ch_ben 4,24 -> 2,58 s, kejim_post 10,28 -> 7,21 s), -bsp
auf duel_mos_espa 44,9 -> 19,6 s (NRC 53,4 s), Auswahl umkehren auf
duel_mos_espa 29,7 s -> 5,2 ms. -vis ist NRCs Code (Unterschiede zwischen
NRC/Stufe 6/jetzt sind Schwankung bzw. die schon in Stufe 6 genommenen
Speicherfixes). kejim_post "Texturen bis fertig" war in Stufe 6 nur 4 ms,
weil der Lauf Jedi Outcast ohne Ordner lud (0 Shader - der 6f-Fehler, im
Protokoll "Jedi Outcast (jk2): 0 shaders"); jetzt 1187 Shader. Nicht
geänderte Editor-Schritte (Verschieben, Rückgängig, .bsp öffnen) schwanken um
etwa 20 % zwischen den Läufen; hier ist nichts schneller geworden (offener
Punkt).

## Die Optimierungen

Jede mit Gleichheitsnachweis; die gespeicherten .map-Hashes der Tempo-Karten
blieben unverändert (duel_pit 0c312bafe551a758, mp__duel1 e1a57227b1d62daf,
t1_inter df6c8e4d9c973e88, kejim_post 7fe6d746f9578597, duel_mos_espa
6119b96058b2a6a8, md_am_sith bd74b12d1f1e7ed7).

- **-light, Lightmaps** (light_ydnar.cpp `BhedLichterParallel`): Pass 1 je
  Block, Stilprüfung seriell, das adaptive Supersampling (Pass 2) mit
  Vorhersage -> paralleles Vorrechnen -> exaktem seriellem Stempel-Scan je
  Licht (ein Subsample-Ergebnis hängt nur vom Pass-1-Wert ab), Pass 3 je Zeile.
  Persistenter Thread-Pool `bhed_par::Pool` (mutex/condvar, keine neuen
  Importe - ctest q3map2_importe). Gleich: Light-BSPs Lump für Lump gleich dem
  alten 1-Thread-Ergebnis bei 1/3/7/32 Threads auf mp__duel1, md_ch_ben,
  t1_inter, duel_pit, kejim_post (ctest q3map2_determinismus verlangt bei >= 4
  Kernen, dass der parallele Weg lief).
- **-light, Lichtgitter**: Vorlauf je Cluster parallel; Punkte, deren
  Schubskasten ganz fest ist, ziehen nur ihre Zufallszahlen
  (`BhedBoxMayBeOpen`); Zwillingssuche über Eimer nach styles/ambient/directed
  plus Gedächtnis (bspfile_rbsp.cpp) - dieselbe erste Übereinstimmung.
- **-bsp, T-Junctions** (tjunction.cpp): AddEdge-Suche in Bündeln à 4096
  parallel, Anwenden seriell in der alten Reihenfolge. Gleich: -bsp mit
  .prt/.srf auf 7 Karten byte-gleich. mos_espa -bsp 42,0 -> 16,0 s.
- **Editor, Auswahl umkehren** (`Auswahl::umschaltenAlle`): 26,8 s -> 2,5 ms
  auf duel_mos_espa (vorher eine Suche je Primitiv in der Auswahlliste).
- **Editor, Auswahl::gewaehlt**: Zwischenspeicher, ungültig über
  `objekteZerstoert()`; die Ansichten fragen es je Primitiv.
- **Editor, NPC-Dateien des Optimierers**: geteilt in `npcLeseAuftrag`
  (Hauptfaden) und `npcAuftragLesen` (beliebiger Faden), vorab mit
  std::future. Gleich: 412 Dateien, 1393 Typen wie der alte Weg (Selbsttest).

## -vis (Punkt 3): NRCs Regel bleibt

Zwei Varianten, mit mehreren Threads dasselbe wie mit einem zu rechnen:
"spekulativ blockierend" (md_ch_ben 3,3-4,3 s gegen 2,24 s mit NRCs Regel,
287-297 Portale neu geflossen, Fluss-Summe 57 s gegen 23 s mit 1 Thread,
Warten 27 s; kejim nach über 18 min abgebrochen, NRC 486 s) und
"aufgeschoben" (md_ch_ben 6,3 s, 1232 neu geflossen). Beide gleich dem
1-Thread-Ergebnis, beide langsamer. NRCs Regel bleibt: mit `-threads 1`
deterministisch, mit mehreren Threads kann das Ergebnis leicht abweichen.
Versuche: `laeufe/s7q/vis_versuche/visflow_spekulativ_*.cpp`. PR #316
aa96ece971 bleibt offen (Stufe 6).

## Speicherlecks GL#95 (Punkt 6)

Spitze des Arbeitsspeichers bei -light auf mp__duel1 mit Bounce 1/4/8/16:
Stufe 6 70,5/75,6/78,7/83,3 MB, jetzt 99,5/131,5/122,8/129,1 MB (die
statischen Blockpuffer der parallelen Lightmaps, höchstens 64 MB). Wachstum
je Bounce etwa 0,8 MB. LSan gibt es unter Windows (MSVC/clang-cl ASan) nicht.
Begründet belassen: der Prozess lebt einen Bau lang, das Wachstum ist klein.

## Echtzeit-Licht (Punkt 5)

- Formeln aus LightContributionToSample (light.cpp), pointScale 7500,
  linearScale 1/8000, Hülle wie mit -fast; Sonne aus q3map_sun/q3map_sunExt,
  Flächenlicht als ein Punktlicht je Fläche. Shader SM 4.1, bis 64 Lichter je
  Bild (cbuffer), 32 Schattenwürfel (TextureCubeArray R16F, LRU, höchstens 8
  neu je Bild), Sonnen-Schattenkarte 2048² R32F. Luxel auf 255 normiert, ×2.
- Beschriftet oben rechts "Real-time light: preview (no bounce, no filter)",
  nie gespeichert. ctest karte_echtlicht prüft die Formeln mit Sollwerten.
- Bildzeit: duel_mos_espa (43 Lichter) Mittel 4,39 ms, p99 8,05 ms,
  Lichtliste 0,03 ms (240-Hz-Grenze 4,17 ms).
- Schnell-Backen: Prozess mit IDLE_PRIORITY_CLASS, temp-Ordner
  `bhed_schnellbacken_<pid>`, -meta, -vis -fast (nur mit .prt), -light -fast;
  eine Änderung bricht ab. Testraum 1,68-1,77 s; Lumps 14/15/16 gleich der
  Kommandozeile mit denselben Schaltern (Selbsttest). Das Lichtgitter wird
  noch nicht angezeigt, nur die Lightmaps.

## Zusatzpunkte 6b-6f

- **6b** (shank): Menüs im Stil des Programms (Vorgabe; Einträge,
  Reihenfolge, Untermenüs, Haken, Kürzel, Mnemonik-Tasten, Abreißkante und
  Wirkung wie NRC; Preferences > Interface > "Menus like NetRadiant-Custom"
  stellt den Qt-Stil wieder her - die NRC-Maße und -Fotos des Selbsttests
  laufen damit). View endet mit Target game, Theme, Language, Interface Scale,
  Toolbar Button Size (Misc behält Sprache/Farbschema/Skalierung). Knopfgröße
  Klein/Normal (NRC: Symbol 24, Knopf 31)/Groß, sofort, gespeichert
  (BhedToolbarButtons). Strg+Rad: NRC gibt das Rad mit und ohne Strg den
  Ansichten, darum skaliert es im Map-Editor nicht (rot vorher: uiScale 1,1).
- **6c** (Tester): Ursache nicht die Einstellungen (alle Vorgaben NRC ab dem
  ersten Bild, mit und ohne cfg, 116 Einstellungen rundlaufgleich), sondern
  ImGui::TeleportMousePos: der Zeiger springt erst im nächsten NewFrame, und
  ImGui verwirft in dem Bild alle Mauslagen der Warteschlange - der Weg jedes
  zweiten Bildes ging verloren (rot: 15 statt 30 Grad bei 30/144 fps, 20 bei
  500 fps, 20 statt 15 bei 150 %). Jetzt wie FreezePointer: GetCursorPos /
  SetCursorPos sofort, Rest unter einem logischen Qt-Punkt wird mitgenommen
  (Kamera und 2D-Verschieben/-Zoomen). Test mit simuliertem Zeiger (die
  Hauptschleife spielt Windows, der echte Zeiger bleibt unberührt).
  Warum der Tester "nach dem Wechsel auf dunkel normal" sah, ließ sich nicht
  belegen; der Verlust hängt am Bildtakt.
- **6d** (Tester): der Texturbaum folgt NRC (TextureGroups_compare ordnet
  ohne Groß/Klein, TextureGroups_constructTreeModel vergleicht mit) - zwei
  Wurzeln "Asjc" gibt es in NRC genauso; der Zeilenschlüssel trägt jetzt die
  Lage. Die neue Prüfung (jede Kennung je Bild einmal, außer ImGuis eigene
  Kindfenster-Doppelung) fand außerdem das doppelt gezeichnete Leistenmenü und
  "waypoint" (Gruppe und Klasse) im Entity-Menü. ConfigDebugHighlightIdConflicts
  im Release aus (Programm und Map-Editor), im Selbsttest/Debug an.
- **6e** (Tester, jeditemple_garden): q3map2 -convert prüfte "structural" über
  das Inhaltsflag, das in den SOF2/JA/JK2-Tabellen 0 ist - alle hint (81
  Flächen), areaportal (15) und skip (359) kamen als detail. Jetzt über die
  Compile-Flags (q3map_structural), wie SetBrushContents; sonst änderte sich in
  der umgewandelten Karte nichts. Übrig, nicht Teil des Berichts: Trigger und
  Brushes von Brush-Entities kommen detail (im Bau ohnehin translucent ->
  detail), einige Detail-Brushes ganz im Festen kommen structural.
  Kamera im durchsichtigen Brush: wie NRC (Rückseiten weg außer bei cull
  none/twosided/disable); der Tester stand in garden_fog (cull twosided).
  Lichter/misc_model: q3map2 entfernt sie (SetLightStyles,
  UnparseEntities, AddTriangleModels; func_group -> worldspawn), im Handbuch
  erklärt.
- **6f** (Tester): das erste Bild lud das Spiel vor den Einstellungen bzw. vor
  den Spielordnern; die Karte blieb am ersten Kontext (0 Shader). Jetzt erst
  die Einstellungen, und ein neu geladenes Spiel geht an die offene Karte
  (wie EnginePath_Realise). Rot: 36/36 Flächen "notfound" nach Neustart, neue
  Pinsel 6/6; grün 0. Fenstertitel wie Sys_SetTitle. Nachgestellt über drei
  Läufe mit einem Einstellungsordner (`SP/s7/idle_spielpfad.sh`,
  `idle_weiter.sh`, `BHED_KARTE_NUR=letzte1/2/3`). Hinweis: system/caulk hat in
  Jedi Academy kein Bild (auch NRCs ja.game-Gamepack bringt keins mit); der
  Test nimmt darum eine Spieltextur.

## Abweichungen von NRC (neu in Stufe 7)

- Build-Menü 215 statt 193 Punkte breit (die Haken-Spalte der eigenen
  Einträge Real-time light / Quick bake), eigene Menüeinträge mit Tooltip.
- Menüs im Stil des Programms als Vorgabe, View-Abschnitt, Knopfgröße,
  Strg+Rad (siehe 6b); Fenstertitel ohne Programmname wie NRC.

## Schlussrunde

Vorlage `SP/s6/ende_s6.sh` -> `SP/s7/ende_s7.sh` (Idle, 8-GB-Wächter, ein
Prozess zur Zeit). Ein erster Durchgang wurde durch einen Neustart des
Rechners abgebrochen (Pause des Anwenders); vollständig neu gestartet auf
99fb153f:

| Schritt | Ergebnis |
|---|---|
| Bau | ok |
| ctest | 121/121 (181 s) |
| karteneditor voll (mit duel_mos_espa + MD-BSP) | 3014 OK, 0 FEHLER (766 s, Spitze 1094 MB) |
| Modus 1 | 2645 OK, 0 FEHLER (440 s) |
| karte, kartenfunde, fix604b, laserfalle, geschuetzanim, zielstrahl, effektgrund, figurdeform, jk2, tuerlauf, kamera, missionsbau, szenen, ton, npcklasse, gluehen, gras, figurstufen, missionsliste, rest605, modellimfesten | alle 0 FEHLER (227, 53, 32, 8, 3, 2, 1, 1, 15, 13, 95, 95, 26, 34, 12, 8, 13, 8, 11, 6, 1 OK) |
| spulen | im Durchgang Absturz 0xC0000005 in einer System-/Treiber-DLL (Adresse 0x7FFA2677A034, schreibend an 0xFFFFFFFFFFFFFF85, "GPU-Weg zuletzt: Karte: zeichnen", beim Spulen mit 3778 Bildern/s); zweimal wiederholt: je 13 OK, 0 FEHLER. Nicht nachstellbar, Abbild `laeufe/s7espulen/behaved-absturz.dmp` |
| Leistung gegen rc605 (md_am_sith, je 2 Läufe) | Vorbereitung rc605 6159/6449 ms, Stufe 7 6486/6217 ms; Bild Mittel 14/13 gegen 17/14 ms, Flug 7.3/7.5 gegen 8.5/8.1 ms, max. 169/178 gegen 261/172 ms - der erste Lauf langsamer, der zweite gleich (Schwankung; Stufe 6: 15/14 gegen 14/14) |
| Kartenprüfung (278 MD-Karten, gegen Stufe 6) | alle Summen gleich (1 -> 1 nicht gezeichnete Entity, 0 Abstürze, 0 D3D-Meldungen) |
| sauberer Neubau | 0 Fehler, 130 Warnungen (432 s; Stufe 6: 130) |
| ctest auf dem sauberen Bau | 121/121 |

## Was Stufe 8 braucht

- Handbuch-Tutorial "Erste eigene Karte" und ein großer Gesamttest.
- Zusammenführen mit `-DBEHAVED_Q3MAP2_LLVM` (der clang-Bau von q3map2).
- misc_models aus der Geometrie zurückgewinnen und Lichter schätzen (6e,
  geplant vom Koordinator).
- Schnell-Backen: das Lichtgitter anzeigen (bisher nur die Lightmaps).
- Editor: .bsp öffnen, Laden, Speichern, Verschieben und Rückgängig wurden
  nicht weiter beschleunigt; szeneStand geht je Bild alle Auswahlelemente mit
  Sperre durch; für die Kantensuche in -bsp wäre ein weiterer Index möglich.
- -vis mit mehreren Threads deterministisch ohne Tempoverlust bleibt offen.

## Changelog

## English

- Map editor: Real-time light (Build menu) lights the camera on the graphics card with q3map2's formulas - point lights, spotlights, the sun and surface lights, with shadows. It is labelled as a preview and never saved.
- Map editor: Quick bake (Build menu) builds the map in the background at low priority after a short pause and shows the real lightmaps from bhed_q3map2; any change cancels it.
- Map compiler: -light uses all cores (lightmaps and light grid) and -bsp fixes T-junctions on all threads - with byte-identical results to a 1-thread build.
- Map editor: Invert Selection on big maps takes milliseconds instead of seconds; the optimizer reads its NPC files in the background.
- Map editor: menus look like the rest of the program (the NetRadiant-Custom look is a setting), the View menu has target game, theme, language, interface scale and toolbar button size (small, normal, large); Ctrl + mouse wheel no longer scales the program inside the map editor.
- Map editor: mouse look in the camera and 2D panning turn and move as far as in NetRadiant-Custom at any frame rate (before, about half of the mouse movement was lost).
- Map editor: no more "conflicting ID" message; the texture tree, the toolbar menu and the entity menu have unique entries.
- Map editor: File > Open .bsp brings hint, areaportal and skip brushes back as structural.
- Map editor: after a restart the last map and new brushes show their textures; the window title shows the map name as in NetRadiant-Custom.
- Guide: new page "Map editor: speed and real-time light", what a .bsp no longer contains, own handling on the differences page, all map pictures updated in four languages.

## Deutsch

- Karten-Editor: Echtzeit-Licht (Build-Menü) beleuchtet die Kamera auf der Grafikkarte mit den Formeln von q3map2 - Punktlichter, Spotlichter, die Sonne und Flächenlicht, mit Schatten. Es ist als Vorschau beschriftet und wird nie gespeichert.
- Karten-Editor: Schnell-Backen (Build-Menü) baut die Karte nach einer kurzen Pause im Hintergrund mit niedriger Priorität und zeigt die echten Lightmaps von bhed_q3map2; jede Änderung bricht es ab.
- Karten-Compiler: -light nutzt alle Kerne (Lightmaps und Lichtgitter), -bsp behebt T-Junctions auf allen Threads - mit byte-gleichem Ergebnis wie ein Bau mit einem Thread.
- Karten-Editor: Auswahl umkehren dauert auf großen Karten Millisekunden statt Sekunden; der Optimierer liest seine NPC-Dateien im Hintergrund.
- Karten-Editor: die Menüs sehen aus wie im übrigen Programm (das Aussehen von NetRadiant-Custom ist eine Einstellung), das View-Menü hat Zielspiel, Farbschema, Sprache, Skalierung und Knopfgröße (klein, normal, groß); Strg + Mausrad skaliert im Karten-Editor nicht mehr das Programm.
- Karten-Editor: Umsehen mit der Maus in der Kamera und Verschieben in 2D drehen und bewegen so weit wie in NetRadiant-Custom, bei jeder Bildrate (vorher ging etwa die Hälfte des Mauswegs verloren).
- Karten-Editor: keine Meldung "conflicting ID" mehr; Texturbaum, Leistenmenü und Entity-Menü haben eindeutige Einträge.
- Karten-Editor: File > Open .bsp bringt hint-, areaportal- und skip-Brushes structural zurück.
- Karten-Editor: nach einem Neustart zeigen die letzte Karte und neue Brushes ihre Texturen; der Fenstertitel zeigt den Kartennamen wie in NetRadiant-Custom.
- Handbuch: neue Seite „Karten-Editor: Tempo und Echtzeit-Licht“, was eine .bsp nicht mehr enthält, eigene Bedienung auf der Seite der Unterschiede, alle Kartenbilder in vier Sprachen erneuert.

## 中文

- 地图编辑器:实时光照(Build 菜单)使用 q3map2 的公式在显卡上照亮摄像机视图——点光源、聚光灯、太阳和面光源,并带有阴影。它标注为预览,永远不会被保存。
- 地图编辑器:快速烘焙(Build 菜单)在短暂停顿后以低优先级在后台编译地图,并显示 bhed_q3map2 生成的真实光照贴图;任何修改都会将其取消。
- 地图编译器:-light 使用所有核心(光照贴图和光照网格),-bsp 在所有线程上修复 T 形接缝——结果与单线程编译逐字节一致。
- 地图编辑器:在大地图上反选只需几毫秒而不是几秒;优化器在后台读取 NPC 文件。
- 地图编辑器:菜单外观与程序其他部分一致(NetRadiant-Custom 的外观可在设置中选择),View 菜单包含目标游戏、配色、语言、界面缩放和工具栏按钮大小(小、正常、大);在地图编辑器中,Ctrl + 鼠标滚轮不再缩放程序。
- 地图编辑器:摄像机中的鼠标环视和 2D 平移在任何帧率下都与 NetRadiant-Custom 转动和移动得一样远(之前约有一半的鼠标移动丢失)。
- 地图编辑器:不再出现 "conflicting ID" 消息;纹理树、工具栏菜单和实体菜单的条目都是唯一的。
- 地图编辑器:File > Open .bsp 会把 hint、areaportal 和 skip 笔刷以 structural 还原。
- 地图编辑器:重启后,上次的地图和新笔刷都能显示纹理;窗口标题像 NetRadiant-Custom 一样显示地图名称。
- 手册:新增页面“地图编辑器:速度与实时光照”,说明 .bsp 中不再包含的内容,在区别页面中加入自有操作,所有地图图片以四种语言更新。

## 日本語

- マップエディター:リアルタイムライト(Build メニュー)は、q3map2 の式でグラフィックカード上でカメラを照らします。ポイントライト、スポットライト、太陽、面光源に対応し、影も付きます。プレビューと表示され、保存されることはありません。
- マップエディター:クイックベイク(Build メニュー)は、短い間を置いてからバックグラウンドで低優先度のままマップをビルドし、bhed_q3map2 の本物のライトマップを表示します。変更すると中止されます。
- マップコンパイラー:-light は全コアを使い(ライトマップとライトグリッド)、-bsp は全スレッドで T ジャンクションを修正します。結果は 1 スレッドのビルドとバイト単位で同じです。
- マップエディター:大きなマップでの選択の反転が数秒ではなく数ミリ秒で終わります。オプティマイザーは NPC ファイルをバックグラウンドで読み込みます。
- マップエディター:メニューはプログラムのほかの部分と同じ見た目になりました(NetRadiant-Custom の見た目は設定で選べます)。View メニューに対象ゲーム、配色、言語、UI の拡大率、ツールバーのボタンサイズ(小・標準・大)があります。マップエディターでは Ctrl + マウスホイールでプログラムが拡大縮小されなくなりました。
- マップエディター:カメラのマウスルックと 2D の移動は、フレームレートに関係なく NetRadiant-Custom と同じだけ回転・移動します(以前はマウスの移動の約半分が失われていました)。
- マップエディター:"conflicting ID" のメッセージが出なくなりました。テクスチャツリー、ツールバーメニュー、エンティティメニューの項目は一意です。
- マップエディター:File > Open .bsp で hint・areaportal・skip ブラシが structural として戻ります。
- マップエディター:再起動後、前回のマップと新しいブラシにテクスチャが表示されます。ウィンドウタイトルには NetRadiant-Custom と同じくマップ名が表示されます。
- ガイド:新しいページ「マップエディター:速度とリアルタイムライト」、.bsp に含まれなくなったもの、違いのページに独自の操作を追加、すべてのマップ画像を 4 言語で更新しました。
