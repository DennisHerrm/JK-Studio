# Karten-Editor Stufe 8a: Bau-Fortschritt, BSP-Import ohne Verlust, Teil-Kompilierungen

Arbeitskopie `Y:\karte`, Zweig `karte1`, ab `ef64d56f` (Ende Stufe 7).
Auftrag (Koordinator, Wünsche shank und Tester): ein Kompilier-Fortschritt mit
Restzeit; eigene Karten verlustfrei über die .bsp zurück; misc_model und
Lichter fremder Karten aus der .bsp zurückgewinnen bzw. schätzen;
Teil-Kompilierungen mit gleichem Ergebnis wie der volle Bau; einer fertigen
Karte etwas hinzufügen, ohne sie neu zu bauen. Grundregeln wie immer: gleiches
Ergebnis ist Pflicht, wo etwas als exakt gilt (Byte für Byte, mit Test
belegt), Näherungen beschriftet; jeder Fehler erst nachgestellt (rot), dann
behoben (grün); eigene Funktionen mit eigenen Menüeinträgen und dem Hinweis
"nicht in NetRadiant-Custom".

## Commits

| Punkt | Commit | Inhalt |
|---|---|---|
| 1 | e513232e | Bau-Fortschritt mit Restzeit (Fenster "Build progress", Zeiten je Schritt, Melden) |
| 2 | 0d7ca39e | Editor-Daten behalten (`-keeplights` + `<karte>.bhed`), Optimierer-Prüfung "enttext" |
| 3 | 93c1c3d5, 9dc56b94, 9425bac1 | misc_model aus der BSP-Geometrie (`-bhedmodels`, Abgleich), beim Öffnen im Hintergrund; Behebung: Ergebnis verworfen, wenn sich die Geometrie inzwischen geändert hat (Punkt 7 gefunden) |
| 4 | 3b44b895 | Lichter schätzen (experimentell) |
| 5a | 12b00b10 | VIS wiederverwenden (`-bhedvisreuse`) |
| 5b/5d | 1ab6c994, 5aaf35db | Licht nur für betroffene Lightmaps, Nachbearbeitung ohne Strahlen (`-bhedlightcache`); Behebung mit `-super` (Punkt 7 gefunden) |
| 5c | 77c1cef0 | Zur fertigen Karte hinzufügen (`-mergebsp -world -fixnames`, Prüfungen, Kollision) |
| - | 181fcec5, 4c4402d7 | Abdeckungsliste mit den Einträgen der Stufe 8a (aus dem Schlusslauf); Herkunftsköpfe der neuen Dateien, Zeilenverweis der Grenzentabelle |
| 6 | a5438c33, 423ceaae | Handbuch: neue Seite, Abschnitte auf "Fertige Karten öffnen", Bilder in 4 Sprachen |
| 7/8 | (Commit dieser Notizen) | Schlussrunde, Messungen, Notizen, Changelog |

## Punkt 1: Kompilier-Fortschritt mit Restzeit

- Was: q3map2s eigene Ausgabe (Phasenköpfe `--- X ---`, der Pacifier
  `0...1...2...` mit 40 Ticks, `--- Radiosity (bounce x of y) ---`,
  `############### model N`) wird Zeichen für Zeichen aus dem XML-Strom und der
  Batch-Leitung gelesen (`src/karte/baufortschritt.cpp`). Fenster
  "Build progress": Eintrag, Schritt/Stufe, Phasenbalken, Gesamtbalken,
  vergangene Zeit, Restzeit, Abbrechen. Am Ende die Zeit je Schritt gegen den
  letzten Bau in der Konsole; Zeiten je Karte und Eintrag in
  `karte_bauzeiten.json` (höchstens 64). Build > "Melden, wenn der Bau fertig
  ist": `FlashWindowEx` (kein Ton, kein Dialog; im Selbsttest nur gezählt).
- Restzeit: mit Verlauf aus den Schrittzeiten des letzten Baus (Phase in der
  Stufe, Tempo gegen den letzten Bau begrenzt auf 0,5..2); ohne Verlauf aus der
  laufenden Phase als Untergrenze ("mindestens").
- Genauigkeit: Selbsttest stufe8/fortschritt, zweiter Bau nach 3,3 s:
  geschätzt 7,4 s gesamt, echt 7,6 s (-2 %); Prüfgrenze 50 %.
- Tests: ctest `karte_bau` mit aufgezeichneten q3map2-Strömen
  (`tests/karte/q3map2/fortschritt_*.bin/.zeit`, 85 Prüfungen), Selbsttest
  stufe8/fortschritt (Fenster, Restzeit, Abbrechen im Fenster, Melden aus).

## Punkt 2: Editor-Daten behalten

- Was: eigene Build-Option (vorgegeben aus). Jeder BSP-Befehl bekommt
  q3map2s `-keeplights`; nach dem Bau `<karte>.bhed` neben der .bsp: Kopf,
  Fingerabdruck der Geometrie-Lumps 2..9, Länge, die genaue Quellkarte.
  File > Open .bsp findet die Datei (auch über das Dateisystem des Spiels, also
  in .pk3) und öffnet bei passendem Fingerabdruck die Quelle statt des
  Dekompilats; sonst Meldung und normales Dekompilieren.
- Engine geprüft (OpenJK SP/MP, JK2): `SP_light` gibt Lichter ohne targetname
  frei, `G_Spawn` nimmt freie Plätze beim Kartenstart wieder (sv.time 1000),
  cgame übergeht misc_model; misc_model bleibt **nicht** im Lump (q3map2 streicht
  es; Dekompilierer würden das eingebackene Modell verdoppeln), es steht nur in
  der .bhed. Grenzen: MAX_TOKEN_CHARS 1024 (neue Optimierer-Prüfung "enttext":
  Schlüssel/Werte über 1023 Zeichen, Entity-Text gegen MAX_MAP_ENTSTRING
  0x40000 mit dem Anteil der behaltenen Lichter; die Engine prüft die Grenze
  selbst nicht).
- Gleichheit: Selbsttest stufe8/editordaten baut dieselbe Karte ohne und mit
  Option - Lumps 1..17 Byte für Byte gleich, nur der Entity-Lump wächst (298 ->
  464 Byte, Lichter 1 -> 3); Open .bsp mit .bhed gibt Lichter, misc_model (mit
  Winkel/Skalierung), func_group mit 2 Pinseln zurück und speichert Byte für
  Byte wie die Quelle; ohne .bhed kommen die Lichter aus dem Lump.
- Messung jeditemple_garden (Testerkarte): ohne Option 700 s, mit 675 s (gleich
  im Rauschen); Unterschied nur Lump 0, 39853 -> 83045 Byte, Lichter 9 -> 388;
  Lightmaps (9781248 Byte) und Gitter (163140 Byte) gleich.

## Punkt 3: misc_model aus der BSP-Geometrie

- Was: `bhed_q3map2 -bhedmodels <liste> <aus>` lädt die Kandidaten genau wie
  InsertModel (assimp, gleiche Flags, Eckenreihenfolge, st). Kandidaten = die
  Modelldateien des Spiels in den Ordnern der BSP-Shader. Abgleich
  (`src/karte/modellrueck.cpp`): Durchgang 1 Dreiecksuppen Ecke für Ecke
  (affin nach kleinsten Quadraten, zerlegt in angles und modelscale_vec, auch
  gespiegelt); Durchgang 2 forcemeta-Modelle in den verschmolzenen
  Meta-Dreiecken (affine Abbildung aus zwei benachbarten nicht ebenen
  Dreiecken, auf allen Ecken samt st geprüft, die einfachste gleichwertige
  Lage). Treffer werden misc_model mit origin, angle(s), modelscale(_vec),
  _remap, spawnflags 4 (forcemeta). Nur genaue Treffer (Eckenfehler klein).
- Nicht erkannte Flächen (kein Treffer, auf keiner Pinselseite, kanonische
  Shadernamen, Ebenenabstand <= 0,5): zu konvexen Vielecken verschmolzen,
  Option "Nicht erkannte Modellflächen als Pinsel behalten" (vorgegeben an) ->
  dünne Detail-Pinsel (Seiten `system/nodraw`) in der Ebene "Nicht erkannte
  Modellflächen".
- Läuft beim Öffnen im Hintergrund (Fortschrittsfenster, Abbrechen, ein
  Rückgängig-Schritt); "Update entities only" bleibt möglich.
- Genauigkeit jeditemple_garden (Quelle bekannt): 265 von 265 misc_model, 0
  falsch; größte Abweichung Ort 0,002, Winkel 0,0002°, Skalierung 2e-6
  (relativ); 473 Dreiecke ohne Treffer -> 401 Vielecke.
- Robustheit (Originalkarten ohne Quelle): kor1 12 Kandidaten -> 52 Treffer,
  yavin1 100 -> 18, hoth2 91 -> 5, t2_trip 0 Kandidaten (Shader in anderen
  Ordnern als die Modelle); mp/duel1 (Handbuch-Lauf): 14 Treffer (alle
  forcemeta), 85 von 86 Kandidaten geladen, 938 Dreiecke -> 779 dünne Pinsel.
  Laufzeit: Dump etwa 1 s, Suche 0,2..1,3 s (kor1 vorher 73 s, mit
  Schnellprüfung und Wiederverwendung der Kandidaten 1,3 s), duel1 gesamt 3,3 s.
- Tests: ctest `karte_modellrueck` (Lagemathematik, Kandidaten, Dump-Leser;
  mit Argumenten Messwerkzeug), Selbsttest stufe8/modelle (pillar.md3 mit angle
  45, modelscale 1.5 zurück, Fehler 1e-5, Rückgängig, ohne Option keins;
  Pinsel verschoben, während die Rückgewinnung läuft -> Ergebnis verworfen).
- **Fehler gefunden und behoben (9425bac1):** im Schlusslauf importierte der
  Test der Stufe 4 direkt nach dem Öffnen von duel1 eine .bsp; die
  Rückgewinnung wurde danach fertig, fügte ihr Ergebnis ein und setzte den
  Geometrie-Fingerabdruck neu - damit hätte "Update entities only" eine
  geänderte Geometrie als Original geschrieben. Rot nachgestellt im Selbsttest
  (Pinsel während der Suche verschoben: Fingerabdruck "unverändert", misc_model
  eingefügt), behoben (Ergebnis verwerfen, Meldung in der Konsole, Handbuch),
  grün. Die Tests der Stufen 4/5/7, die das Dekompilat selbst prüfen, öffnen
  die .bsp jetzt ohne Rückgewinnung (sonst zeitabhängig).

## Punkt 4: Lichter schätzen (experimentell)

- Was: Option "Lichter schätzen (experimentell)" (vorgegeben aus). Strahlen
  von jeder Lichtgitter-Zelle entlang ihrer Lichtrichtung stimmen in einem
  32er-Raster ab; Spitzen werden zum Punkt kleinster Quadrate der Strahlen, die
  sie erklären; Stärke aus gerichtet * d² / 7500 (LightContributionToPoint),
  Farbe aus dem gerichteten Licht; neben info_null-Zielen, auf die niemand
  zeigt, bekommen Lichter ein target; in der .bsp behaltene Lichter werden
  nicht verdoppelt. Ebene "Geschätzte Lichter", Schlüssel `_bhed_geschaetzt 1`
  (q3map2 und G_ParseField übergehen ihn).
- Genauigkeit (ehrlich): raum.map 2/2 Lichter innerhalb 8 Einheiten, Stärke
  x0,98..1,00; Testkarte des Selbsttests: das hellste Licht < 48 Einheiten, drei
  Lichter auf einer Linie -> das zweite liegt 84 Einheiten daneben, dazu
  Fehlspitzen (8 geschätzt für 2 entfernte). jeditemple_garden: 279 geschätzt für
  388 der Quelle, nur 2 innerhalb 32 Einheiten (meist schwache Spotlichter
  unter dem 64x64x128-Gitter); neu gebacken weichen die Lightmaps im Mittel um
  38,5 von 255 Stufen ab (mit den echten Lichtern neu gebacken: 8,8), größte
  255; Verteilung je 16 Stufen 59 % / 11 % / 6 % / 4 % / ... / 13 % über 128.
  Darum experimentell, als Ausgangspunkt beschriftet.
- Tests: ctest `karte_lichtschaetzung` (raum_licht.bsp), Selbsttest
  stufe8/lichter (Wirkung: Ebene, Schlüssel, eines nahe am hellsten Licht, das
  behaltene nicht doppelt).

## Punkt 5: Teil-Kompilierungen

### a) VIS wiederverwenden (exakt)

`-vis -bhedvisreuse`: Cache `<karte>.bhedvis` (BHVIS1: Schlüssel aus den
Schaltern und `_farplanedist`, die .prt-Bytes, die Sichtbarkeits-Bytes). Gleich
-> Sichtbarkeit kopiert ("visibility reused"), der Schalter wird nicht in die
worldspawn-Befehlszeile geschrieben. Beweis: ctest `q3map2_teilbau`
(raum, raum_einfach: Entity- und Detail-Änderung Byte für Byte gleich dem vollen
Bau, Struktur-Änderung neu gerechnet), dazu mp__duel1, t1_inter, md_ch_ben und
die Messung unten. Exakt nur gegenüber `-vis -threads 1` (mit mehreren Threads
weicht -vis selbst von Lauf zu Lauf ab, Stufe 7).

### b/d) Licht nur für betroffene Lightmaps, Nachbearbeitung ohne Strahlen (exakt)

`-light -bhedlightcache`: Cache `<karte>.bhedlight` (BHLICHT2) mit den rohen
Super-Luxeln, Deluxeln, Flags und (neu) Super-Clustern je Raw-Lightmap, dem
Vertex-Licht der vertex-beleuchteten Flächen und den rohen Gitterpunkten.
Geometrie-Schlüssel ohne die Felder, die -light schreibt
(lightmapOrigin/-Vecs, Drawvert-Lightmap/Farbe, Flächen-Lightmapfelder), mit
.srf, allen Shader-Skripten, Brush-Entities, worldspawn ohne die
Nachbearbeitungs-Schlüssel und den Licht-Schaltern ohne die der
Nachbearbeitung; kein Cache mit -bounce. Eine Lightmap kommt aus dem Cache, wenn
ihre gefilterte Lichtliste (jedes Feld jedes Lichts, gleiche Reihenfolge)
gleich ist; das Gitter, wenn sich kein Licht geändert hat. Nachbearbeitung
(gamma, exposure, contrast, compensate, saturation, gridscale,
gridambientscale, _minlight, _minvertexlight, _mingridlight, _maxlight) läuft
auf den rohen Werten.

Beweis: ctest `q3map2_teilbau` (verschobenes/neues Licht, nur Nachbearbeitung,
nur -light noch einmal; **neu zwei Fälle mit -super 2**), alles Byte für Byte
gegen volles -light.

**Fehler gefunden und behoben (5aaf35db):** bei der Zeitmessung (Punkt 7) auf
duel_pit (JK2, `-fast -filter -super 2`) waren Lightmaps und Eckenfarben nach
einem Lauf aus dem Cache anders als beim vollen -light. Ursache: der
Filterdurchgang von IlluminateRawLightmap setzt gefüllte unbelegte Luxel auf
CLUSTER_FLOODED, StoreSurfaceLightmaps liest das beim Mitteln der
Super-Samples; der Cache behielt die Cluster des Geometrie-Durchgangs. Rot
nachgestellt im ctest (4 Fehlschläge mit -super 2), behoben (Cluster im Cache,
Format 2, alter Cache wird nicht benutzt), grün; zusätzlich auf duel_pit und
mp__duel1 geprüft mit -super 2/4, -dirty, -deluxe, -floodlight,
-lightmapsize 256, -samples 3 -filter -patchshadows, -nocollapse -approx 4 -
überall gleich.

### c) Zur fertigen Karte hinzufügen (Näherung beim Licht, sonst geprüft)

Build > "Zur fertigen Karte hinzufügen...": Vorlage (.bsp aus dem Spiel oder
Datei) beleuchtet in der Kamera und als Kanten in 2D, nicht bearbeitbar; eine
Ebene wird exportiert, allein gebaut (`-meta`, `-light -fast -samples 2`) und
mit `q3map2 -mergebsp -world -fixnames -game ja/jk2` in eine Kopie eingefügt
(`<vorlage>_merged.bsp` im Einstellungsordner `karte_zusammen`). Licht des
Teils angenähert: Gittermittel der Vorlage im Kasten als `_ambient`/`_color`,
behaltene Lichter ohne targetname in der Nähe. Prüfungen: inside/outside
gemischt (Fehler, sperrt), Wetter-Pinsel außerhalb jeder misc_weather_zone
(Warnung), gleiche target/targetname (Hinweis, -fixnames benennt um),
Grenzen der zusammengeführten .bsp (Optimierer-Zählung), Kollision jedes
eingefügten festen Welt-Pinsels im CM-Port (Spielerkasten von außen auf die
Mitte, MASK_PLAYERSOLID). Der Dialog wählt die aktuelle Ebene vor und zeigt
Licht und Kollision (nicht nur die Konsole).

Tests: ctest `q3map2_zusammen` (raum_licht + Säule, Wettermischung, Zone,
Namen; mit Spiel JA mp/duel1 + Säule: lädt, die Säule stoppt den Spieler),
Selbsttest stufe8/teilbau (VIS/Licht wiederverwenden im Editor, Zusammenführen,
Kollision 1/1, keine Grenze über).

### Zeiten: Teil-Kompilierung gegen vollen Bau (Punkt 7)

`SP/s8/teilzeit.py`, Rohdaten `laeufe/teil8/teilzeit_*.json`; Median aus 3,
Idle, `-vis -threads 1`, Licht JA `-fast -patchshadows -samples 2 -filter`,
JK2 `-fast -filter -super 2`; Änderung = ein neues Licht (300) 64 über dem
ersten Startpunkt; jedes Ergebnis Lump für Lump gegen den vollen Bau
verglichen - **alle gleich** (3 Karten x 3 Läufe x 3 Vergleiche).

| Karte | voller Bau (bsp/vis/light) | nach Licht-Änderung mit Cache | voller Bau derselben Änderung | Lightmaps aus dem Cache | nur -gamma: Cache / voll |
|---|---|---|---|---|---|
| mp__duel1 | 0,77 s (0,31/0,16/0,31) | 0,76 s (VIS 0,10 wiederverwendet) | 0,82 s | 149 von 439 | 0,31 / 0,31 s |
| md_ch_ben | 25,52 s (0,72/23,14/1,67) | **2,68 s** (VIS 0,20 wiederverwendet) | 25,41 s | 18 von 48 | 1,16 / 1,62 s |
| duel_pit | 0,92 s (0,21/0,16/0,56) | 0,92 s (VIS 0,10) | 0,92 s | 166 von 369 | 0,21 / 0,56 s |

Dazu md_ch_ben mit `-super 2 -samples 4 -filter -patchshadows` (ein Lauf):
voll 27,25 s, nach der Licht-Änderung 2,98 s, nur -gamma 1,21 gegen 2,02 s;
alles gleich. Der erste Bau mit Cache kostet so viel wie ohne (Schreiben des
Caches im Rauschen). Ehrlich: bei diesen kleinen Karten ist -light vom Laden
(Shader, Bilder, Gitter-Vorlauf) bestimmt, die Ersparnis der Lightmaps sieht
man kaum; groß ist sie bei VIS (Faktor 10 auf md_ch_ben) und bei der reinen
Nachbearbeitung.

## Punkt 6: Handbuch

Neue Seite "Karten-Editor: Bau-Fortschritt und Teil-Kompilierungen"
(`tools/handbuch/inhalt_karte8.py`): Fortschritt, VIS/Licht wiederverwenden,
Zur fertigen Karte hinzufügen mit den Prüfungen, der Licht-Näherung und den
Wetter-Regeln aus dem JKHub-Thread. "Fertige Karten öffnen und ändern":
Editor-Daten behalten, misc_model aus der Geometrie, Lichter schätzen
(experimentell, mit den Zahlen), "Was eine .bsp nicht mehr enthält" um
Ebenen/Gruppenschlüssel/Texturausrichtung und Flächen ohne Brush erweitert.
"Unterschiede zu NRC": die eigenen Bau-Funktionen. Bilder aus dem Selbsttest
`BHED_HB_NUR=karte8` (neu), alle Kartenbilder in 4 Sprachen neu (Build-Menü
und Open-.bsp-Dialog haben sich geändert; die Läufe karte4/karte5 warten auf
die Rückgewinnung). `gen_handbuch.py --pruefen`: ok.

## Abweichungen von NRC (neu in Stufe 8a)

- Build-Menü: fünf eigene Einträge (Editor-Daten behalten, VIS/Licht
  wiederverwenden, Zur fertigen Karte hinzufügen, Melden), Fenster "Build
  progress"; Open-.bsp-Dialog: drei eigene Haken. Alle mit Tooltip "nicht in
  NetRadiant-Custom".
- bhed_q3map2: Schalter `-bhedmodels`, `-bhedvisreuse`, `-bhedlightcache`
  (ohne sie unverändert).

## Schlussrunde

Vorlage `SP/s7/ende_s7.sh` -> `SP/s8/ende_s8.sh` (Idle, 8-GB-Wächter, ein
Prozess zur Zeit), Summe `SP/s8/ende_summe.txt`. Erster Durchgang auf 423ceaae
(+ Licht-Cache-Behebung 5aaf35db):

| Schritt | Ergebnis |
|---|---|
| Bau | ok |
| ctest | 123/125 - `karte_lizenz` (die 10 neuen Dateien ohne Herkunftskopf) und `karte_grenzen` (Zeilenverweis light.cpp:1855 durch die Haken der Stufe 8a verschoben) -> behoben (4c4402d7) |
| karteneditor voll (mit duel_mos_espa + MD-BSP) | 3028 OK, 10 FEHLER (794 s, Spitze 1059 MB): 8 Folgen der Rückgewinnung im Hintergrund in den Tests der Stufen 4/6 (Import gleich nach dem Öffnen von duel1 - der echte Fehler oben), 1 in Stufe 8/2 (die Rückgewinnung lief im Gesamtlauf rechtzeitig fertig), 1 Größe des Build-Menüs (215 x 532 -> 241 x 652 mit den fünf neuen Einträgen) -> behoben (9425bac1) |
| Modus 1 | 2645 OK, 0 FEHLER (437 s) |
| karte, kartenfunde, fix604b, laserfalle, geschuetzanim, zielstrahl, effektgrund, figurdeform, jk2, tuerlauf, kamera, missionsbau, szenen, ton, npcklasse, gluehen, gras, spulen, figurstufen, missionsliste, rest605, modellimfesten | alle 0 FEHLER (227, 53, 32, 8, 3, 2, 1, 1, 15, 13, 95, 95, 26, 34, 12, 8, 13, 13, 8, 11, 6, 1 OK); spulen diesmal ohne Absturz |
| Leistung gegen rc605 (md_am_sith, je 2 Läufe) | Vorbereitung rc605 5289/5331 ms, Stufe 8a 5346/5402 ms; Bild Mittel 11/11 gegen 11/11 ms, Flug 5.1/5.1 gegen 6.1/5.1 ms, max. 147/146 gegen 144/147 ms - gleich |
| Kartenprüfung (278 MD-Karten, gegen Stufe 7) | alle Summen gleich (0 Abstürze, 0 nicht geladen, 0 D3D-Meldungen, 1 -> 1 nicht gezeichnete Entity) |
| sauberer Neubau | 0 Fehler, 130 Warnungen (404 s; Stufe 7: 130) |

Nach den Behebungen (4c4402d7):

| Schritt | Ergebnis |
|---|---|
| ctest | 125/125 |
| karteneditor voll | 3039 OK, 0 FEHLER (780 s, Spitze 1039 MB); Abdeckung 951 Einträge, 947 mit Wirkungsprüfung, 4 begründete Ausnahmen, 0 ohne |

Die übrigen Modi liefen auf dem Bau vor den Behebungen; die Behebungen
betreffen nur die Rückgewinnung im Karten-Editor und Selbsttest-Schritte des
Karten-Editors.

## Offene Punkte

- Lichter schätzen bleibt ungenau (siehe Punkt 4): schwache Spotlichter unter
  der Gitterauflösung sind nicht zu trennen; Lichter auf einer Linie
  verschmelzen.
- Kandidaten für misc_model nur aus den Ordnern der BSP-Shader: Karten wie
  t2_trip (Modelle aus anderen Ordnern) finden nichts; mp/duel1 lässt 938
  Dreiecke als dünne Pinsel zurück. Eine Suche über alle Modelle des Spiels
  wäre möglich, kostet aber Zeit (Dump aller Modelle).
- forcemeta-Treffer nur geometrisch (mit st); _remap aus dem Shader-Vergleich.
- Zur fertigen Karte hinzufügen: Licht angenähert; das Lichtgitter der Vorlage
  wird nicht erneuert; ein Bau des Ganzen bleibt für die fertige Karte nötig.
- Licht-Cache: Änderungen am Umgebungslicht (`_ambient`, `_color` der
  worldspawn) und an Filter/Dirt-Parametern lassen alles neu rechnen (nicht
  als Nachbearbeitung abbildbar); -bounce ohne Cache.
- VIS-Wiederverwendung ist exakt nur gegenüber `-vis -threads 1`.
- Die .bhed muss man selbst neben die .bsp in die .pk3 legen.
- Bei kleinen Karten bestimmt das Laden die Zeit von -light (siehe Messung).

## Was Stufe 8b braucht

- Terrain Generator.
- Frei belegbare Split-Fenster samt Merken nach Neustart.
- Im Editor spielen.
- Logik-Ansicht (Verbindungen target/targetname, Skripte).
- Ansichtsmodi.
- Spiegel-Modus.
- Undo-Verlauf als Liste.
- GPU-Vorschau mit Bounce.
- GPU-Klick-Auswahl.
- Optionales GPU-Backen mit Abweichungsbericht gegen bhed_q3map2.
- Tutorial "Erste eigene Karte" im Handbuch.
- Großer Gesamttest.
- Zusammenführen mit `-DBEHAVED_Q3MAP2_LLVM` (clang-Bau von q3map2).
- Umbenennung in JK Studio.

## Changelog

## English

- Map editor: the window "Build progress" shows the step, q3map2's own progress (phase, dots, bounce), a bar over all steps and the remaining time - from the second build of a map taken from its last build; the console lists the time of each step against the last build; the taskbar button flashes when a build is done (option, no sound).
- Map editor: Build > Keep editor data - all lights stay in the .bsp and `<map>.bhed` next to it holds the exact source map; File > Open .bsp then brings back misc_model, func_group, texture alignment, detail and layers. Lightmaps, light grid and geometry stay byte-identical.
- Map editor: File > Open .bsp recovers misc_model from the compiled geometry (only exact matches, also forcemeta models) and can keep unrecognized model faces as thin brushes in their own layer; runs in the background.
- Map editor: File > Open .bsp can estimate lights from the light grid (experimental, own layer, marked with `_bhed_geschaetzt`).
- Map editor: partial compiles - reuse VIS when the portals are unchanged and light only the lightmaps a change reaches; changing only gamma, exposure and similar retraces nothing. The .bsp is byte-identical to a full build.
- Map editor: Build > Add to compiled map - build one layer and insert it into a finished .bsp with q3map2 -mergebsp, with checks for weather brushes, zones, names, limits and collision; the light of the addition is approximated.
- Map optimizer: new check "Entity lump and its limits" (keys or values over 1023 characters, entity text near the engine's limit).
- Guide: new page "Map editor: build progress and partial compiles", the open-maps page explains editor data, misc_model recovery and estimated lights; all map pictures updated in four languages.

## Deutsch

- Karten-Editor: das Fenster „Baufortschritt“ zeigt den Schritt, den eigenen Fortschritt von q3map2 (Phase, Punkte, Bounce), einen Balken über alle Schritte und die Restzeit - ab dem zweiten Bau einer Karte aus deren letztem Bau; die Konsole nennt die Zeit jedes Schritts gegen den letzten Bau; der Knopf in der Taskleiste blinkt, wenn ein Bau fertig ist (Option, ohne Ton).
- Karten-Editor: Build > Editor-Daten behalten - alle Lichter bleiben in der .bsp, und `<Karte>.bhed` daneben enthält die genaue Quellkarte; File > Open .bsp bringt dann misc_model, func_group, Texturausrichtung, Detail und Ebenen zurück. Lightmaps, Lichtgitter und Geometrie bleiben Byte für Byte gleich.
- Karten-Editor: File > Open .bsp gewinnt misc_model aus der gebauten Geometrie zurück (nur genaue Treffer, auch forcemeta-Modelle) und kann nicht erkannte Modellflächen als dünne Brushes in einer eigenen Ebene behalten; läuft im Hintergrund.
- Karten-Editor: File > Open .bsp kann Lichter aus dem Lichtgitter schätzen (experimentell, eigene Ebene, markiert mit `_bhed_geschaetzt`).
- Karten-Editor: Teil-Kompilierungen - VIS wiederverwenden, wenn die Portale gleich sind, und nur die Lightmaps neu beleuchten, die eine Änderung erreicht; nur gamma, exposure und Ähnliches geändert: kein einziger Strahl. Die .bsp ist Byte für Byte die eines vollen Baus.
- Karten-Editor: Build > Zur fertigen Karte hinzufügen - eine Ebene bauen und mit q3map2 -mergebsp in eine fertige .bsp einfügen, mit Prüfungen für Wetter-Brushes, Zonen, Namen, Grenzen und Kollision; das Licht des Teils ist angenähert.
- Karten-Optimierer: neue Prüfung „Entity-Lump und seine Grenzen“ (Schlüssel oder Werte über 1023 Zeichen, Entity-Text nahe der Grenze der Engine).
- Handbuch: neue Seite „Karten-Editor: Bau-Fortschritt und Teil-Kompilierungen“, die Seite zu fertigen Karten erklärt Editor-Daten, die Rückgewinnung von misc_model und geschätzte Lichter; alle Kartenbilder in vier Sprachen erneuert.

## 中文

- 地图编辑器:“编译进度”窗口显示当前步骤、q3map2 自身的进度(阶段、进度点、反弹)、覆盖全部步骤的进度条以及剩余时间——从地图的第二次编译起根据上一次编译计算;控制台列出每个步骤的时间并与上一次编译比较;编译完成时任务栏按钮会闪烁(可选,无声音)。
- 地图编辑器:Build > 保留编辑器数据——所有灯光保留在 .bsp 中,旁边的 `<地图>.bhed` 保存完整的源地图;之后 File > Open .bsp 会恢复 misc_model、func_group、纹理对齐、detail 和图层。光照贴图、光照网格和几何体逐字节不变。
- 地图编辑器:File > Open .bsp 会从已编译的几何体中恢复 misc_model(只接受精确匹配,包括 forcemeta 模型),并可将未识别的模型面作为薄笔刷保留在单独的图层中;在后台运行。
- 地图编辑器:File > Open .bsp 可以根据光照网格估算灯光(实验性,单独图层,带 `_bhed_geschaetzt` 标记)。
- 地图编辑器:部分编译——门户未变时复用 VIS,只为改动影响到的光照贴图重新计算光照;只改 gamma、exposure 等时不追踪任何光线。生成的 .bsp 与完整编译逐字节相同。
- 地图编辑器:Build > 添加到已编译的地图——编译一个图层并用 q3map2 -mergebsp 插入已完成的 .bsp,并检查天气笔刷、区域、名称、限制和碰撞;新增部分的光照为近似值。
- 地图优化器:新增检查“实体块及其限制”(超过 1023 个字符的键或值、接近引擎上限的实体文本)。
- 手册:新增页面“地图编辑器:编译进度与部分编译”,已完成地图页面说明了编辑器数据、misc_model 恢复和估算的灯光;所有地图图片以四种语言更新。

## 日本語

- マップエディター:「ビルドの進行状況」ウィンドウに、ステップ、q3map2 自身の進み具合(フェーズ、進行の点、バウンス)、全ステップのバー、残り時間が表示されます。残り時間はマップの 2 回目のビルドから前回のビルドをもとに求めます。コンソールには各ステップの時間が前回との比較つきで表示され、ビルドが終わるとタスクバーのボタンが点滅します(オプション、音なし)。
- マップエディター:Build > エディターデータを保持 - すべてのライトが .bsp に残り、隣の `<マップ>.bhed` に正確なソースマップが入ります。File > Open .bsp で misc_model、func_group、テクスチャの位置合わせ、detail、レイヤーが戻ります。ライトマップ、ライトグリッド、ジオメトリはバイト単位で同じです。
- マップエディター:File > Open .bsp がコンパイル済みのジオメトリから misc_model を取り戻します(正確に一致したものだけ、forcemeta モデルも含む)。認識できなかったモデルの面は専用レイヤーの薄いブラシとして残せます。バックグラウンドで実行されます。
- マップエディター:File > Open .bsp でライトグリッドからライトを推定できます(実験的、専用レイヤー、`_bhed_geschaetzt` の印つき)。
- マップエディター:部分コンパイル - ポータルが同じなら VIS を再利用し、変更が届くライトマップだけを再計算します。gamma や exposure などだけを変えた場合はレイを 1 本も飛ばしません。.bsp はフルビルドとバイト単位で同じです。
- マップエディター:Build > コンパイル済みマップに追加 - 1 つのレイヤーをビルドし、q3map2 -mergebsp で完成した .bsp に挿入します。天候ブラシ、ゾーン、名前、上限、衝突を確認します。追加部分のライトは近似です。
- マップオプティマイザー:新しい確認項目「エンティティランプとその上限」(1023 文字を超えるキーや値、エンジンの上限に近いエンティティテキスト)。
- ガイド:新しいページ「マップエディター:ビルドの進行状況と部分コンパイル」。完成したマップのページでエディターデータ、misc_model の復元、推定ライトを説明。すべてのマップ画像を 4 言語で更新しました。
