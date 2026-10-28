# Karten-Editor Stufe 6: die Prüfstufe

Arbeitskopie `Y:\karte`, Zweig `main`, ab `1c3092a` (Ende Stufe 5).
Auftrag: jeder Knopf mit Wirkungsprüfung, keine Fehler von NetRadiant-Custom
(NRC 20260114) übernehmen, bekannte Fehler aus NRC, Xonotic-NetRadiant (GL#),
GtkRadiant (GT#), q3map2 (NRC-PR #316) und OpenJK aussortieren.
Fehlerliste: `bekannte_fehler.md` (Recherche 25.10.2026), Abschnitte A
(Editor), B (q3map2), C (JKA/Engine).

## Commits

| Punkt | Commit | Inhalt |
|---|---|---|
| 1 | 832f2e1, b53682b, b2e453c, ed9d979 | Wirkungsprüfung für jeden Eintrag der Checkliste, Abdeckungstabelle, ctest karte_abdeckung |
| 2 | e3db551 … b632a6a | Abschnitt A: NRC#302, #65, #130, #164, #213, #270/#43/#72, AutoCaulk -game, #145, #147/#152/#12, #261, GL#93, GT#679, #279 |
| 3 | 73e910a, 5d9e0b4, 06264c0, 89d05f7, 13bdbab | q3map2: deterministisch (-bsp/-light), Speicher-/UB-Fehler, NRC#101, NRC#102, -vis zurück auf NRC, Geometrie aus PR #316, Funde der Sanitizer-Läufe |
| 4 | 14b7223, f29e23e, ec349ab | Vorschau: misc_model_static im Festen (OpenJK#1358); Region-Bau-Hinweis (NRC#132), externe Lightmaps durch Shader (NRC#48/#98), weitere q3map2-Prüfungen; Update entities only bei geänderten Schaltlichtern |
| 5 | 7b7ea4b, 1f34061, 8eac2f6 | Fuzzer für kaputte .map/.bsp/.prt; Decimal-Fehler; schnellerer Optimierer-Start; größte Karte in vier Sprachen |
| 6 | a3549e1, f2fd9be | Handbuch "Unterschiede zu NetRadiant-Custom" |
| 7 | 20db7f8, d258a06, 36db681 | Schlussrunde: AutoCaulk mit alter .caulk, vier reihenfolgeabhängige Tests, Abdeckung aus dem vollen Lauf, Kartenprüfung kennt OpenJK#1358 |
| 8 | (Commit dieser Notizen) | Notizen, Changelog |

## Abdeckung in Kurzform

944 Einträge der Checkliste (Menüs, Werkzeugleisten, Kontextmenüs, Tasten,
Maus, Einstellungsseiten mit jedem Feld, Dialogknöpfe, Plugin-Menüs und
-Dialoge), 940 mit Wirkungsprüfung im Selbsttest (Teil `stufe6`: befehle,
robust, tasten, ereignisse, menues, knoepfe, felder, dialoge, funde, pfade).
Jeder Befehl, der die Karte ändert, wird über Rückgängig/Wiederholen Byte für
Byte verglichen; jeder Befehl läuft auch mit leerer Auswahl, leerer Karte und
gemischter Auswahl. 4 begründete Ausnahmen: Install Map Developer's Files
(startet ein fremdes Installationsprogramm), Exit als Befehl und als
Menüeintrag (beendet den Test), Hardware Texture Compression (wirkt nur auf
GL-Treiber, hier D3D11). Werkzeug `nrc_inventar.py abdeckung`, Tabelle
`tools/karte/abdeckung.md`, ctest `karte_abdeckung` (rot, sobald einem Eintrag
die Prüfung fehlt oder der Lauf einen FEHLER enthält).

## Fehlertabelle

Spalten: Ergebnis; rot/grün (Test vor und nach der Behebung, "grün" = der
Fehler war bei uns nicht da, der Test belegt es); Ausgabe = ändert sich eine
Datei, die der Editor oder q3map2 schreibt.

### A. Editor

| Quelle | Ergebnis | rot/grün | Ausgabe |
|---|---|---|---|
| NRC#302, GL#206 | behoben: Kappen-Richtung nach Größe und Form | rot 3 / grün 0 (kartepatchwerkzeuge) | ja (Kappen) |
| NRC#65 | behoben: Ecken/Kanten beim Verlassen des Modus abgewählt | rot 2 / grün (Selbsttest funde, Stufe 2 372/0) | nein |
| NRC#130 | behoben: Umlaute, Leerzeichen, CJK in Pfaden (Editor und q3map2-Manifest UTF-8) | rot 6 / grün 0 (q3map2_pfade, Selbsttest pfade 19/0) | nein |
| NRC#164 | behoben: Engine-/Ressourcenpfad beim Setzen absolut | rot / grün | nein |
| NRC#213 | behoben: AutoCaulk nennt den Grund (q3map2-Ausgabe) | rot / grün | nein |
| NRC#270 | bei uns nicht da (q3map2 von NRC 20260114 nimmt system/origin, der Editor ruft -game ja/jk2) | grün (q3map2_funde) | - |
| NRC#43, NRC#72 | nicht da | grün (q3map2_funde) | - |
| NRC#145 | behoben für JA/JK2: Namen aus system.shader (nodraw_solid, caulk_water, …), fehlender Shader bleibt weg | rot / grün (funde 24/0) | ja (AutoCaulk-Shader) |
| eigener Fund | AutoCaulk rief q3map2 mit -game quake3: Wasser bekam Caulk | rot / grün | ja |
| NRC#147, #152, #12 | nicht da | grün (funde) | - |
| NRC#261 | behoben: Anzeige außerhalb des Spinners um ganze Kacheln normiert, .map bleibt | rot 2 / grün | nein |
| GL#93 | nicht da (AP oben/unten nach Drehen+Skalieren: Scherung, Grenze des Formats; Seiten genau) | grün (kartetexdef) | - |
| GT#679 | nicht da (jede Kante und Ecke des Minimal-Pinsels gezogen) | grün (kartekomponenten) | - |
| NRC#279 | ABSICHT; Hinweis in der Programmsprache unter der englischen Zeile | rot / grün | nein |
| NRC#281 | ABSICHT, bleibt | - | - |
| NRC#132 | ABSICHT; Hinweis vor dem Region-Bau (Kamera im Pinsel, keine Entity) | rot / grün (funde 38/0) | nein |
| NRC master | Textfehler übernommen (Door Builder "East - West", "Distance Multiplier") | rot 2 / grün | - |
| eigene Funde | Texturbrowser setzte den Shader neuer Pinsel nie; Tag-Fenster doppelt gespeichert; Knopf Clean ohne Wirkung; Windows-Dateidialoge im Selbsttest erreichbar | je rot / grün | nein |
| eigener Fund (Schlussrunde) | AutoCaulk wendete eine alte <Karte>_ac.caulk an, wenn q3map2 keine neue schrieb (Leck, q3map2 fehlt) - NRC ebenso; jetzt wird die alte vorher gelöscht | rot 1 / grün (funde 39/0) | ja (keine falschen Caulk-Flächen) |
| eigener Fund (kartefuzz) | Decimal: Zahlen ab 1e10 abgeschnitten, 1e20 -> "1" (NRC liest dort hinter dem Puffer) | rot / grün | nur absurde Werte |
| JKHub (Lichtstile) | Update entities only verweigert, wenn sich die schaltbaren Lichter geändert haben | rot 1 / grün (Stufe 4 licht 27/0) | nein |

### B. q3map2

| Quelle | Ergebnis | rot/grün | Ausgabe |
|---|---|---|---|
| GL#173 | -bsp und -light deterministisch für jede Thread-Zahl (Sortierung nach Shadername, Lightmap-ST 0, RadLight-Reihenfolge, Gitter-Vorlauf, Zufall je Arbeitsschritt); -vis wie NRC | rot / grün (q3map2_determinismus, mp__duel1, t1_inter) | ja (Reihenfolge, siehe Abweichungen) |
| Stufe-5-Kandidat 5 (-vis mit Threads) | offen: fünf deterministische Regeln alle mehr als doppelt so langsam wie NRC (kejim_post 486 s) | - | - |
| PR #316 cf682e5b8 | behoben (mapplanes[-1]) | ASan rot / grün (R08.zeroarea) | nein |
| PR #316 ca2e281df | behoben (Deluxel ohne Deluxemap) | UBSan rot / grün (S12.passage) | nein |
| eigener Fund (UBSan) | writebsp: leere Grenzen float->int (UB), jetzt INT_MIN ausdrücklich | rot / grün | nein (Lumps gleich) |
| eigener Fund (ASan/UBSan, kartefuzz) | ParseMapEntity: entities.back() vor der ersten Entity; LoadMapFile: Karte ganz ohne Entity (entities[0] auf leerem Vektor, jetzt Fehlermeldung); GetToken: '"' als letztes Zeichen | 5 von 300 Fuzz-Karten rot / alle grün | nein |
| eigener Fund (UBSan, kartefuzz) | float->int außerhalb des Bereichs: Ebenen-Hash (FindFloatPlane, AddPlaneToHash), Texturbereich in AddMetaTriangleToSurface; jetzt INT_MIN ausdrücklich wie x86 | 3 Fuzz-Karten rot / grün | nein |
| eigener Fund (UBSan, Originalkarten) | mesh.cpp: vier Unterteilungen schoben die Zeilen mit expand[0] + width * i hinter Zeile 0 des 2D-Felds zusammen (-light brach unter UBSan auf mp__duel1, mp__duel5, t2_trip, kor1, hoth2 ab); jetzt Zeile für Zeile ins Netz kopiert | 5 von 9 Lichtläufen rot / grün | nein (Byte-Vergleich alt/neu) |
| NRC#101 | behoben: Dirt-Strahlen in den Himmel verdunkeln nicht (zweite kurze Spur bei passSolid) | rot 0.926 / grün 1.001 | ja (-dirty) |
| NRC#102, GL#154 | behoben: Kartenname mit Punkt ohne Endung | rot / grün | nein (Editor: volle Pfade) |
| PR #316 Geometrie | P1, P2, M4, M3 (2) übernommen; aa96ece971 und 6f297e4752 nicht | PR-Fälle 52 -> 65 | ja |
| GT#390 | hängt am Portal-Fix aa96ece971 (nicht übernommen, 42 Karten lecken) | - | - |
| GT#611 | Code nicht mehr da (std-Container) | - | - |
| GT#669 | nur mit -extravisnudge (aus) | - | - |
| GT#113, GL#143, GL#108, GL#128 | nicht da | grün (q3map2_funde) | - |
| GL#210 | nicht anwendbar (Punkt-Backsplash in NRC #if 0) | - | - |
| GL#58 | nicht nachstellbar (-bounce 8, Lichtzahl stabil) | grün | - |
| NRC#48 | Grenze des Formats (gemessen: Stil-Lightmaps -3); Build-Menü (Stufe 5) und Optimierer `lightmapextern` (Shader) | Unit-Test | nein |
| NRC#98 | Build-Menü (Stufe 5), Optimierer `lightmapextern` (JK2) | Unit-Test | nein |
| GL#59/94/95, GT#676 | ASan/UBSan über alle Karten (siehe unten); Speicherlecks (GL#95) nicht behoben: kurzlebiger Prozess | - | - |
| GL#189/NRC#144, GT#271, NRC#109 | Optimierer (Stufe 5) | - | - |
| NRC#226 | ABSICHT | - | - |

### C. JKA/Engine

| Quelle | Ergebnis | rot/grün | Ausgabe |
|---|---|---|---|
| OpenJK#1358 | Vorschau "wie im Spiel" zeigt misc_model_static mit Prüfpunkt im Festen nicht mehr; Optimierer (Stufe 5) bietet zoffset | rot / grün (modellimfesten) | nein |
| OpenJK#1358 (Schlussrunde) | die Kartenprüfung zählte die nun versteckten 124 Modelle auf 21 Karten als "Spiel zeigt, behaved nicht"; jetzt dieselbe Prüfung wie die Vorschau | rot 1 -> 125 / grün 1 -> 1 (278 Karten) | nein |
| JKHub Lichtstile | -game ja: Stile sind an (-style wirkungslos, -nostyle schaltet alle ab); der Shader-Hack gilt nur für IBSP. Neubau: Stile 32+j in Entity-Reihenfolge, passt zu den neu gebackenen Lightmaps. Update entities only: siehe A | - | - |
| JKHub MAX_MAP_LIGHTGRID, safe_malloc | Optimierer (Stufe 5) | - | - |

## Robustheit, Sanitizer, offene Punkte aus Stufe 5

- Fuzzer `kartefuzz` (ctest `karte_fuzz`): kaputte .map, .bsp, .prt und
  Punktdateien beim Öffnen, Importieren und Bauen; schreibt mit
  `--schreiben` 300 verformte Karten für die q3map2-Läufe.
- bhed_q3map2 unter ASan (MSVC /fsanitize=address) und UBSan (clang-cl,
  Trap-Modus mit eigenem Handler): 300 Fuzz-Karten (-bsp), 81 Fälle aus
  PR #316 (-bsp/-vis/-light), 10 Originalkarten (-bsp/-vis/-light, -bounce 8),
  alle 672 dekompilierten Karten (-bsp). Nach den Behebungen (13bdbab): 672/672
  ohne Befund unter ASan und unter UBSan, alle mit Rückgabe 0; die Fuzz-Karten
  und die 10 Originalkarten ohne Befund (ASan 3 h, UBSan 40 min).
- ASan-Lauf des Modus karteneditor: 2939 OK, 0 FEHLER, kein Befund (Schlussrunde).
- Abdeckung des -vis unter den Sanitizern: die dekompilierten Karten sind
  klein (mp__duel1 101 Cluster, -vis unter 1 s); die längeren -vis-Läufe
  kommen aus den PR-#316-Fällen (bis 68 s).
- NRC#132: Hinweis vor dem Region-Bau (Stufe 4b).
- Befund-Details des Optimierers übersetzt (32 Vorlagen, KeOptD01-32).
- Start des Optimierers auf duel_mos_espa.map (Movie-Duels-BSP daneben):
  130 -> 95 ms; Shader nachsehen 43 -> 12 ms (jede .shader-Datei einmal
  zerlegt statt je Name), Momentaufnahme 34 ms; 54 ms davon sind das einmalige
  Lesen der NPC-Dateien je Sitzung, jeder weitere Start ~46 ms. Ergebnis
  gleich (Unit-Test gegen die alte Suche je Name). Die 227 ms aus der
  Aufgabe ließen sich hier nicht nachstellen (Stufe 5 maß 53-103 ms).
- Größte Karte in vier Sprachen: Selbsttest Stufe 5 `gross`, Fotos
  `karteneditor_s6_gross_{grenzen,pruefungen}_{en,de,zh,ja}`.

## Gewollte Abweichungen im Byte-Vergleich gegen NRC

Gegen NRCs q3map2 (Tag 20260114), mit denselben Schaltern. Werkzeug
`tools/karte/q3map2_vergleich.py` (Stufe 5), Determinismus
`tools/karte/q3map2_determinismus.py` (ctest `q3map2_determinismus`).

| # | Stufe | Was anders ist | Warum | Wann sichtbar |
|---|---|---|---|---|
| 1 | -bsp | Reihenfolge der Flächen nach Shadername statt nach Zeiger (CompareMetaTriangles) | NRC sortiert nach Heap-Adressen, also je Lauf anders (GL#173) | jede Karte mit mehr als einem Shader: surfaces, drawverts, drawindexes, leafsurfaces, brushsides.surfaceNum anders sortiert; die Dreiecke selbst gleich |
| 2 | -bsp | Lightmap-Koordinaten der drawverts nach -bsp = 0 | NRC schreibt nicht belegten Speicher | drawverts nach -bsp (nach -light überschrieben, wo eine Lightmap liegt) |
| 3 | -light | Lightmaps packen nach Shadername statt Zeiger (CompareSurfaceInfo); damit auch, welche gleichen Lightmaps zusammengelegt werden, und die MD4-Namen der Stil-Shader | NRC je Lauf anders (GL#173) | lightmaps, surfaces (Lightmap-Nummern), drawverts (Lightmap-ST), bei IBSP shaders |
| 4 | -light | Radiosity-Lichter (-bounce) in der Reihenfolge eines Laufs mit einem Thread; Gitterpunkte im Festen mit dem Zufallsablauf eines 1-Thread-Laufs | Thread-Rennen (emplace_front unter ThreadLock), rand() je Thread | lightmaps/lightgrid bei mehreren Threads; mit -threads 1 Byte-gleich NRC |
| 5 | -light | -dirtmode 1, -randomsamples, Minimap: Zufall je Arbeitsschritt statt rand() | Thread-Rennen | nur mit diesen Schaltern, auch mit -threads 1 anders als NRC |
| 6 | -light | NRC#101: Dirt-Strahlen in den Himmel verdunkeln nicht mehr | Fehler (Himmel galt als Verdecker) | nur mit -dirty neben Himmel |
| 7 | alle | NRC#102/GL#154: Kartenname mit Punkt ohne Endung | Fehler | nur auf der Kommandozeile ohne .map (der Editor übergibt volle Pfade) |
| 8 | -bsp | PR #316 Geometrie: P1 (-microvolume, Teilstücke jedes positiven Volumens bleiben), P2 (kreuzende Flächen bei ON_EPSILON), M4 (Flächen fast auf einer Ebene), M3 zweimal (kleine Querschnitte) | Fehler aus dem PR, je einzeln gemessen: keine Karte schlechter | Karten mit sehr dünnen oder fast koplanaren Pinseln; auf 106 Originalkarten 2 von 2,1 Mio. Stichpunkten anders (mp__ffa1), sonst gleich |
| 9 | -bsp/-light | Speicher- und UB-Fehler (mapplanes[-1], Deluxel ohne Deluxemap, writebsp/Ebenen-Hash/Texturbereich float->int, mesh.cpp Zeilen hinter Zeile 0, ParseMapEntity ohne Entity, GetToken am Dateiende) | Speicherfehler, UB | keine Ausgabeänderung (Lumps alt/neu auf 8 Originalkarten Byte für Byte gleich) |
| 10 | -bsp | Karte ganz ohne Entity: Fehlermeldung "No entity in map file" | NRC stürzt ab (0xC0000005) | nur bei solchen Karten |
| - | -vis | bleibt wie NRC: mit mehreren Threads je Lauf leicht anders, mit -threads 1 deterministisch | jede deterministische Regel war mehr als doppelt so langsam (kejim_post 486 s) | - |

Byte-gleich NRC: -vis und -light mit einem Thread (ohne die Schalter aus 5
und 6) bei gleicher Eingabe-BSP; der Entity-Lump.

## Schlussrunde

Vorlage `SP/s5/ende_s5.sh` -> `SP/s6/ende_s6.sh` (Idle, 8-GB-Wächter, ein
Prozess zur Zeit). Erster Durchgang: karteneditor 2938 OK / 11 FEHLER - daraus
20db7f8 (AutoCaulk mit alter .caulk, ein echter Fehler; vier Tests, die von
der Reihenfolge der Teile abhingen). Zweiter Durchgang auf 20db7f8 (eine
Pause des Anwenders dazwischen, danach ab missionsbau fortgesetzt; seitdem
kein Code geändert bis 36db681):

| Schritt | Ergebnis |
|---|---|
| Bau | ok |
| ctest | 120/120 (111 s) |
| karteneditor voll (mit duel_mos_espa + MD-BSP) | 2950 OK, 0 FEHLER (699 s, Spitze 1053 MB) |
| Modus 1 | 2645 OK, 0 FEHLER (436 s) |
| karte, kartenfunde, fix604b, laserfalle, geschuetzanim, zielstrahl, effektgrund, figurdeform, jk2, tuerlauf, kamera, missionsbau, szenen, ton, npcklasse, gluehen, gras, spulen, figurstufen, missionsliste, rest605, modellimfesten | alle 0 FEHLER (227, 53, 32, 8, 3, 2, 1, 1, 15, 13, 95, 95, 26, 34, 12, 8, 13, 13, 8, 11, 6, 1 OK) |
| Leistung gegen rc605 (md_am_sith, je 2 Läufe) | Vorbereitung rc605 6567/6515 ms, Stufe 6 6484/6482 ms; Bild Mittel 14/14 gegen 15/14 ms, Flug 8.2/7.9 gegen 8.1/8.3 ms, max. 175/190 gegen 186/209 ms (Schwankung wie in Stufe 5) |
| Kartenprüfung (278 MD-Karten, gegen Stufe 5) | erst 1 -> 125 nicht gezeichnete Entities (OpenJK#1358, siehe C), nach 36db681 1 -> 1; sonst alle Summen gleich, keine Abstürze, keine D3D-Meldungen |
| ASan karteneditor (build-asan, MSVC /fsanitize=address, ohne die große Karte) | 2939 OK, 0 FEHLER, kein ASan-Bericht (2432 s, Spitze 2572 MB) |
| sauberer Neubau auf 36db681 | 0 Fehler, 130 Warnungen (371 s; Stufe 5: 127) |
| ctest auf dem sauberen Bau | 120/120 |

## Was Stufe 7 braucht

- -vis mit mehreren Threads deterministisch machen, ohne langsamer zu werden
  (Stufe-5-Kandidat 5): fünf Varianten gemessen, alle mehr als doppelt so
  langsam wie NRC (kejim_post 486 s).
- PR #316 aa96ece971 (Portale nur bei Entartung verwerfen) erst mit einem
  Ersatz für die winzigen Portale, die NRC heute fallen lässt (42 von 106
  Originalkarten lecken sonst); dann auch 6f297e4752 und GT#390.
- Speicherlecks von q3map2 (GL#95) sind nicht behoben: der Prozess lebt nur
  einen Bau lang.
- Die Kartenprüfung und der Optimierer lesen die NPC-Dateien einmal je
  Sitzung auf dem Hauptfaden (54 ms beim ersten Öffnen des Optimierers auf
  duel_mos_espa); in einen Faden nur, wenn readFromArchives dafür sicher ist.
- Das leere Verzeichnis `Y:\pr316daten` (Rest eines Versuchs; das Löschen
  wurde hier abgelehnt) bitte von Hand entfernen.

## Changelog

## English

- Map editor: every menu entry, toolbar button, key, mouse action, preference field and dialog button was checked for its effect in the self-test (940 of 944 checklist entries, 4 with a reason); every command that changes the map undoes and redoes byte for byte.
- Map editor: bugs known from NetRadiant-Custom fixed - bevel and end caps work in any patch orientation, vertices and edges are deselected when leaving their mode, paths with umlauts or Chinese and Japanese characters work, relative engine paths are made absolute, AutoCaulk says why it failed and uses the Jedi Academy and Jedi Outcast system shaders (water is no longer caulked, transparent solid brushes stay solid), the Surface Inspector shows shifts from other editors correctly.
- Map editor: own findings fixed - the texture browser now really sets the texture of new brushes, the Clean button of the preferences works, very large coordinates keep their value when a map is saved, AutoCaulk no longer applies the result of an earlier run when q3map2 fails.
- Map editor: where NetRadiant-Custom keeps its behaviour on purpose, BehavEd explains it - which patch size a cap needs (in the program language) and a warning before a region build that will leak.
- Map editor: "Update entities only" refuses when the switchable lights changed - their light is baked into the lightmaps per light style.
- Map compiler: the BSP and light stages give the same result every time, with any number of threads; dirtmapping no longer darkens surfaces next to the sky; map names with a dot work; memory errors and undefined behaviour with broken maps and when patches are subdivided fixed (checked with AddressSanitizer and UBSan on all maps), a map without any entity gets a clear error; five geometry fixes from NetRadiant-Custom pull request #316 (thin brush parts, crossing faces, faces almost on another plane, small cross-sections).
- Optimizer: the details of every finding are translated; new check for external lightmaps from shaders (q3map_lightmapSize) next to light styles or in Jedi Outcast.
- Preview as in the game: a misc_model_static the game never draws (its cull point lies in solid) is no longer shown.
- Guide: new page "Map editor: differences to NetRadiant-Custom" in four languages.

## Deutsch

- Karten-Editor: jeder Menüeintrag, Werkzeugknopf, jede Taste, Mausaktion, jedes Einstellungsfeld und jeder Dialogknopf wurde im Selbsttest auf seine Wirkung geprüft (940 von 944 Einträgen der Checkliste, 4 begründet); jeder Befehl, der die Karte ändert, lässt sich Byte für Byte rückgängig machen und wiederholen.
- Karten-Editor: aus NetRadiant-Custom bekannte Fehler behoben - Fasen- und Endkappen gehen in jeder Ausrichtung des Patches, Ecken und Kanten werden beim Verlassen ihres Modus abgewählt, Pfade mit Umlauten oder chinesischen und japanischen Zeichen gehen, relative Engine-Pfade werden absolut, AutoCaulk nennt den Grund eines Fehlschlags und nimmt die Systemshader von Jedi Academy und Jedi Outcast (Wasser wird nicht mehr gecaulkt, durchsichtige feste Pinsel bleiben fest), der Surface Inspector zeigt Verschiebungen aus anderen Editoren richtig.
- Karten-Editor: eigene Funde behoben - der Texturbrowser setzt die Textur neuer Pinsel jetzt wirklich, der Knopf Clean der Einstellungen wirkt, sehr große Koordinaten behalten beim Speichern ihren Wert, AutoCaulk wendet nicht mehr das Ergebnis eines früheren Laufs an, wenn q3map2 scheitert.
- Karten-Editor: wo NetRadiant-Custom sein Verhalten absichtlich behält, erklärt BehavEd es - welche Patch-Größe eine Kappe braucht (in der Programmsprache) und ein Hinweis vor einem Region-Bau, der lecken wird.
- Karten-Editor: „Nur Entities erneuern“ verweigert, wenn sich die schaltbaren Lichter geändert haben - ihr Licht steckt je Lichtstil in den Lightmaps.
- Karten-Compiler: BSP- und Lichtstufe ergeben jedes Mal dasselbe, mit beliebig vielen Threads; Dirtmapping dunkelt Flächen neben dem Himmel nicht mehr ab; Kartennamen mit Punkt gehen; Speicherfehler und undefiniertes Verhalten bei kaputten Karten und beim Unterteilen von Patches behoben (geprüft mit AddressSanitizer und UBSan über alle Karten), eine Karte ganz ohne Entity bekommt eine klare Fehlermeldung; fünf Geometrie-Fixes aus dem Pull-Request #316 von NetRadiant-Custom (dünne Pinselteile, kreuzende Flächen, Flächen fast in einer anderen Ebene, kleine Querschnitte).
- Optimierer: die Angaben jedes Befunds sind übersetzt; neue Prüfung auf externe Lightmaps durch Shader (q3map_lightmapSize) neben Lichtstilen oder in Jedi Outcast.
- Vorschau wie im Spiel: ein misc_model_static, das das Spiel nie zeichnet (sein Prüfpunkt liegt im Festen), wird nicht mehr gezeigt.
- Handbuch: neue Seite „Karten-Editor: Unterschiede zu NetRadiant-Custom“ in vier Sprachen.

## 中文

- 地图编辑器:每个菜单项、工具栏按钮、按键、鼠标操作、首选项字段和对话框按钮都在自检中检查过效果(检查清单 944 项中的 940 项,另 4 项注明原因);每个修改地图的命令都能逐字节地撤销和重做。
- 地图编辑器:修复了 NetRadiant-Custom 中已知的错误——斜面和端盖在曲面片的任何方向下都能使用;离开顶点或边模式时会取消选择;含变音符号或中文、日文字符的路径可以使用;相对引擎路径会转换为绝对路径;AutoCaulk 会说明失败原因,并使用 Jedi Academy 和 Jedi Outcast 的系统着色器(水不再被 caulk,透明的实体笔刷保持实体);表面检查器能正确显示其他编辑器写入的偏移。
- 地图编辑器:修复了自行发现的问题——纹理浏览器现在确实会设置新笔刷的纹理,首选项中的 Clean 按钮有效,保存地图时极大的坐标会保持原值;q3map2 失败时,AutoCaulk 不再套用之前一次运行的结果。
- 地图编辑器:对于 NetRadiant-Custom 有意保留的行为,BehavEd 会加以说明——封盖需要什么尺寸的曲面片(用程序语言),以及在会泄漏的区域编译之前给出警告。
- 地图编辑器:可开关的灯光发生变化时,“仅更新实体”会拒绝执行——它们的光照按光照样式烘焙在光照贴图中。
- 地图编译器:BSP 阶段和光照阶段每次都给出相同结果,任意线程数皆然;Dirtmapping 不再压暗靠近天空的面;含点号的地图名可以使用;修复了处理损坏地图和细分曲面片时的内存错误与未定义行为(已用 AddressSanitizer 和 UBSan 检查所有地图),完全没有实体的地图会得到明确的错误信息;采用了 NetRadiant-Custom 拉取请求 #316 中的五项几何修复(薄笔刷部分、交叉面、几乎位于另一平面上的面、小截面)。
- 优化器:每个结果的详细信息都已翻译;新增检查:光照样式附近或 Jedi Outcast 中由着色器(q3map_lightmapSize)产生的外部光照贴图。
- 如游戏中的预览:游戏永远不会绘制的 misc_model_static(其剔除点位于实体内部)不再显示。
- 手册:新增四种语言的“地图编辑器:与 NetRadiant-Custom 的区别”页。

## 日本語

- マップエディター:すべてのメニュー項目、ツールバーのボタン、キー、マウス操作、設定項目、ダイアログのボタンについて、自己テストで効果を確認しました(チェックリスト 944 項目中 940 項目、4 項目は理由付き)。マップを変更するコマンドはすべて、バイト単位で元に戻す・やり直しができます。
- マップエディター:NetRadiant-Custom で知られている不具合を修正しました。ベベルとエンドキャップはパッチの向きに関係なく使えます。頂点・エッジモードを抜けると選択が解除されます。ウムラウトや中国語・日本語の文字を含むパスが使えます。相対エンジンパスは絶対パスに変換されます。AutoCaulk は失敗の理由を示し、Jedi Academy と Jedi Outcast のシステムシェーダーを使います(水が caulk されず、透明なソリッドブラシはソリッドのままです)。サーフェスインスペクターは他のエディターが書いたシフトを正しく表示します。
- マップエディター:独自に見つけた問題を修正しました。テクスチャブラウザーが新しいブラシのテクスチャを実際に設定するようになり、設定の Clean ボタンが機能し、保存時に非常に大きな座標の値が保たれます。q3map2 が失敗したとき、AutoCaulk が以前の実行結果を適用しなくなりました。
- マップエディター:NetRadiant-Custom が意図的に動作を変えていない箇所では、BehavEd が説明します。キャップに必要なパッチのサイズ(プログラムの言語で)と、リークするリージョンビルドの前の警告です。
- マップエディター:切り替え可能なライトが変わった場合、「エンティティのみ更新」は実行しません。その光はライトスタイルごとにライトマップへ焼き込まれているためです。
- マップコンパイラー:BSP 段階とライト段階は、スレッド数にかかわらず毎回同じ結果になります。ダートマッピングが空に接する面を暗くしなくなりました。ドットを含むマップ名が使えます。壊れたマップやパッチ分割時のメモリエラーと未定義動作を修正しました(すべてのマップを AddressSanitizer と UBSan で確認)。エンティティがまったくないマップには明確なエラーが表示されます。NetRadiant-Custom のプルリクエスト #316 から 5 つのジオメトリ修正を取り込みました(薄いブラシ部分、交差する面、別の平面にほぼ重なる面、小さい断面)。
- 最適化ツール:各結果の詳細が翻訳されました。ライトスタイルのそばや Jedi Outcast で、シェーダー(q3map_lightmapSize)による外部ライトマップを見つける新しいチェックを追加しました。
- ゲームと同じプレビュー:ゲームが描画しない misc_model_static(カリング点が固体内部にあるもの)を表示しなくなりました。
- ガイド:新しいページ「マップエディター:NetRadiant-Custom との違い」(4 言語)。
