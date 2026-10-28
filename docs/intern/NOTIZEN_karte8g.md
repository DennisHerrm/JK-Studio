# Karten-Editor Stufe 8g: GPU-Backen – die ganze Lichtrechnung auf der Grafikkarte

Arbeitskopie `Y:\karte`, Zweig `karte1`, ab `d982e310` (Ende Stufe 8c).
Auftrag (Koordinator, Entscheidung von shank): die Lichtrechnung von bhed_q3map2 auf
die GPU, **Byte-gleich** mit dem CPU-Weg und deutlich schneller; Standard wird es
erst, wenn shank entscheidet (eine Einstellung). Grundregeln wie immer: Fehler erst
nachstellen, dann beheben, im selben Test bestätigen; exakt heißt Byte-Vergleich.

## Commits

| Punkt | Commit | Inhalt |
|---|---|---|
| 1 | 744df369 | Shader vorab mit fxc übersetzt (DXBC in der exe), kein d3dcompiler_47.dll mehr |
| 2a | 5baf50fe | die ganze Lichtschleife von IlluminateRawLightmap auf der GPU |
| 2b | 185eba4d | Lichtgitter auf der GPU, schnelle Lichtlisten, große Lightmaps in Teilen |
| 2c | da093c0d | Gerätestart neben dem Laden der Karte; Tests NaN/Adapterwahl |
| 2d | 1fae4acb | ereignisgetriebener Stempellauf, mehrere Vorhersage-Runden |
| 3 | 20e157be | DXR-Vortest (`-bhedgpudxr`, nur Messung) |
| 5 | 247ba443 | Editor: Build > GPU bake, GPU in Statuszeile/Fortschritt, Bericht in Konsole und Tooltip |
| 2e | c254a71d, c5d644cc | Dispatches nie unter 4096 Fäden (-samples 4: 293 s → 88 s); Gitter auch nach Punkten geteilt (TDR auf Intel UHD) |
| 6 | ec36ae9c | Handbuchseite `karte_gpubacken` in vier Sprachen |
| 7 | 873912b1, dcef9f2b | erste Lightmap je Runde geprüft; Grenzentabelle, Maß des Build-Menüs |
| 8 | (Commit dieser Notizen) | Notizen, Changelog |


## Ergebnis in einem Satz

`-bhedgpubake` rechnet jetzt **die ganze Lichtrechnung** (Lightmaps mit Supersampling und
Filter, alle Bounce-Runden, das Lichtgitter) auf der GPU, **auf der RTX 4070 Byte-gleich**
mit dem CPU-Weg (100,0000 % der nachgerechneten Luxel und alle 18 Lumps auf fünf Karten,
drei Schalter-Sätzen) und auf großen Karten **1,6–2,7× schneller als 32 CPU-Threads**
(md_am_sith 62,0 → 31,3 s, kejim_post mit Bounce 48,8 → 20,0 s). Kleine Karten gewinnen
wenig (Start der GPU), **ein Fall ist langsamer**: duel_mygeeto mit `-samples 4`
(67 → 101 s). DXR ist als Vortest gemessen und **nicht** eingebaut (Gewinn ≤ 7–8 %, nicht
genau). Im Editor: **Build > GPU bake** (Vorgabe aus, eine Konstante `kGpuBackenVorgabe`).

## Punkt 1: Shader vorab übersetzt

- `bhed_gpubacken.hlsl` (Bake) und `bhed_gpuschatten.hlsl` (`-bhedgpu`, Stufe 8b) liegen
  als eigene Dateien; `src/q3map2/CMakeLists.txt` sucht fxc im neuesten Windows SDK
  (Registry `KitsRoot10`), übersetzt jede Variante (`/T cs_5_0 /O3 /Gis`, wie zuvor
  D3DCompile) in Header (`/Fh`), die exe enthält DXBC. Kein `d3dcompiler_47.dll`, kein
  `D3DCompile` mehr in der exe (ctest `q3map2_gpubake` prüft die Zeichenketten; rot mit der
  8c-exe). Varianten: sechs Einstiege × double/ohne double, Messschalter `MESS` 1/2 nur mit
  double.
- Gemessen (mp__duel1, je drei Läufe): Start 2,23 s → 0,71 s, gesamt 6,86 s → 4,96 s, Lumps
  gleich. Seit Punkt 2c läuft der Gerätestart zusätzlich in einem eigenen Faden neben dem
  Laden der Karte: der Start kostet danach praktisch nichts mehr (0,00–0,03 s Wartezeit).
- Für DXR (Punkt 3) übersetzt dxc aus demselben SDK `bhed_dxr.hlsl` vorab als DXIL
  (`cs_6_5`, signiert über das `dxil.dll` daneben); ohne dxc entfällt nur der Vortest.

## Punkt 2: die Lichtschleife auf der GPU

### Aufbau (je Block von Roh-Lightmaps, Speicherbudget vorab geschätzt)

1. **Erstabtastung** (Kern `erst`, ein Faden je Paar Licht × abgebildeter Super-Luxel):
   `LightContributionToSample` wie in 8c, dazu je Licht der Lightmap die Zahl der
   beleuchteten Luxel (Ganzzahl, atomar – reihenfolgeunabhängig).
2. **Stil-Prüfung auf der CPU** (`BhedStilPruefen`) in Lichtreihenfolge mit diesen Zahlen:
   Stilplatz, „nichts beleuchtet → überspringen“, Stil setzen, Warnung „Hit per-surface style
   limit“ – genau die Schleife von `IlluminateRawLightmap`; nur ein paar Byte je Licht gehen
   hin und zurück.
3. **Nachabtasten** (`-samples`): Vorhersage (`kandidaten`, ein Faden je aktives Licht ×
   Stempelzeile) mit den Werten der Erstabtastung → Kandidatenliste und Stempelmaske; bis zu
   drei weitere Runden mit den schon nachabgetasteten Werten; `unter` tastet die Kandidaten
   parallel nach (`SubsampleRawLuxel_r` als Schleife mit eigenem Stapel bis `-samples 7`,
   `SubmapRawLuxel` samt q3map2s by/bx-Eigenheit, `trace->cluster = cluster[0]`,
   `ClusterForPointExtFilter` mit BSP-Knoten, Ebenen, Blättern, Brushes);
   `stempellauf` (ein Faden je aktives Licht) macht den genauen Lauf in Zeilenfolge wie
   Stufe 7, aber **ereignisgetrieben**: geprüft werden nur vorhergesagte Stempel und solche,
   von denen ein Luxel gerade nachabgetastet wurde – alle anderen haben nur Werte der
   Erstabtastung und schlagen wie in der Vorhersage nicht an (Begründung im Code).
4. **Filtern und Aufsummieren** (`filtern`, ein Faden je Luxel): die aktiven Lichter in
   Listenreihenfolge, gewichtetes Filter von `-filter`, negative Lichter, Stil 0 ab dem
   Umgebungslicht. Zurück kommen nur die fertigen Luxel je benutztem Stilplatz; die CPU setzt
   sie ein (`BhedBakeEinsetzen`) und macht den Rest von `IlluminateRawLightmap` (Floodlight,
   Dirt, Filter der unbeleuchteten Luxel) wie immer.
5. **Lichtgitter** (`gitterKern`): ein Faden je Gitterpunkt, alle Lichter in ihrer
   Reihenfolge in vielen kurzen Dispatches (Teile der Punkte und der Lichter, der Stand je
   Punkt bleibt auf der GPU): `LightContributionToPoint` mit inhibitRadius = halbe Gitterweite,
   skipGrid, passSolid ≠ opaque (ein Dreieckstreffer setzt die Farbe 0, der Beitrag zählt –
   q3map2s Verhalten), dann `gp.dir`, `VectorNormalized` und die Aufteilung in gerichtetes und
   Umgebungslicht je Stil (inkl. `+ contributions.ambient`, das aus −0 +0 macht). Der
   Speicherteil (`BhedGitterSpeichern`) bleibt auf der CPU. Floodlight und `-sunonly`: CPU.
6. **Lichtlisten** (`CreateTraceLightsForBounds`): ein Baum über den Lichtkugeln liefert
   Kandidaten (mit Rand, nie zu wenige), darauf dieselben Tests in Listenreihenfolge – auf
   allen neun Runden von md_am_sith **0 von 14146 Listen verschieden**
   (`BHED_GPUBAKE_LISTENPRUEFEN`), 8,4 s → 0,3 s.
7. **Bounce**: jede Runde läuft durch denselben Weg (Flächenlichter wie in 8c).
   `RadCreateDiffuseLights` (Geometrie, 1 s auf md_am_sith), Vertexlicht
   (`IlluminateVertexes`, 2 s), Nähte und Speichern bleiben auf der CPU (andere Rechnung, je
   Fläche, kein Lichtschleifen-Engpass).

### Gleichheit – wie

- dieselben float-Operationen in derselben Reihenfolge wie q3map2 (`precise`, `/Gis`, keine
  mad), Divisionen genau gerundet in Ganzzahlen (`teilen`), keine Summen in
  Ausführungsreihenfolge (nur Ganzzahl-Zähler atomar);
- `VectorNormalize`/Wurzeln (q3map2: double) **ohne double-Befehle**: float-float-Rechnung
  (Dekker/Knuth) bestimmt den genau gerundeten float-Wert, solange er weiter als 2^-38 von einer
  float-Mitte liegt (Fehler der Näherung < 2^-43); sonst der genaue double-Weg (Tuckerman,
  ddiv). Grund: double ist auf der RTX 4070 Laptop so langsam, dass die Erstabtastung mit
  double-Normalisieren 0,72 s statt 0,44 s brauchte (mp__duel1);
- Ebenenabstände von `ClusterForPointExt` in float mit Toleranz, double nur knapp an der Grenze;
- was nicht genau geht, rechnet für die ganze Lightmap (bzw. den Gitterpunkt) die CPU:
  Alpha-Schatten/Lichtfilter (brauchen die Textur), unsichere Division, nicht endliche Werte.

### Absicherungen (je mit Test)

| Absicherung | wie | Test |
|---|---|---|
| TDR | adaptive Dispatch-Größe 4096 … 4 M Fäden (halbiert über 0,5 s je Wartegruppe), Gitter zusätzlich nach Punkten und Lichtern (0,02–0,15 s) | Intel UHD auf duel_mygeeto: vorher Geräteverlust im ersten Gitter-Dispatch (11 s, alles CPU), nachher 148 s auf der GPU ohne Verlust |
| Geräteverlust | Meldung, der Block und der Rest auf der CPU, Stile erst nach Erfolg übernommen | ctest `q3map2_gpubake` (vorgetäuscht nach 2 Dispatches): Lightmaps und Gitter gleich der CPU |
| VRAM | Budget = ¼ des eigenen Grafikspeichers (64 MB … 1,5 GB, wenig eigener: ½ des geteilten), 40 Byte je Paar; Blöcke; eine Lightmap über dem Budget in Teilen ihrer Lichtliste (Stil-Prüfung und Startwerte laufen von Teil zu Teil) | ctest: `BHED_GPUBAKE_BUDGET_MB=1` → 162 Blöcke, 20 Lightmaps in Teilen, Lumps gleich; md_am_sith 18 Lightmaps in Teilen |
| NaN/Inf | Lightmap bzw. Gitterpunkt neu auf der CPU | ctest: `BHED_GPUBAKE_NAN` (eine Lightmap, ein Gitterpunkt vorgetäuscht) → Lumps gleich |
| Adapterwahl | `-bhedgpuadapter N|Name`, sonst die schnellste (HIGH_PERFORMANCE) | ctest: Adapter 0 und ein unbekannter Name (Meldung, Rückfall) |

### Abweichungsbericht

Jede 64. Lightmap (bis 1 M Paare; dazu die erste kleine jeder Runde) rechnet die CPU ein
zweites Mal (Zustand gesichert und zurückgesetzt) und vergleicht **alle Luxel Byte für Byte**
samt Stilen; das GPU-Ergebnis bleibt. `-bhedgpubakecheck N` (1 = alle). Ausgabe:
`BehavEd GPU bake report: … luxels in … lightmaps checked on the CPU, … % identical; …` und
`<Karte>.bhedgpu.txt`.

## Punkt 3: DXR als Vortest – gemessen, nicht eingebaut

`-bhedgpudxr` (mit `-bhedgpu`): D3D12 zur Laufzeit, RayQuery (DXIL vorab), BLAS aus den
Trace-Dreiecken mit Schattenwurf (ohne Himmel), jeder 8b-Strahlenblock zusätzlich durch die
Hardware; gezählt werden Übereinstimmung und Zeit (beide mit Hoch- und Herunterladen).

| Karte | Strahlen verglichen | DXR verfehlt einen Treffer | DXR zusätzlich | festes Blatt | Compute | DXR |
|---|---|---|---|---|---|---|
| mp__duel1 | 1,94 M | 1285 (0,07 %) | 21 | 26867 | 0,285 s | 0,151 s |
| md_ch_ben | 30 445 | 25 | 0 | 203 | 0,048 s | 0,027 s |
| kejim_post | 8,37 M | 12592 (0,15 %) | 618 | 400579 | 1,200 s | 0,576 s |
| md_am_sith | 61,0 M | 29780 (0,05 %) | 330 | 234809 | 18,34 s | 7,81 s |
| duel_mygeeto | 17,6 M | 67167 (0,38 %) | 17006 | 593768 | 29,18 s | 7,25 s |

Bewertung: DXR ist 2–4× schneller für den Strahl, aber (1) ohne q3map2s Epsilons nicht gleich
(verfehlte Treffer → als Vortest bräuchte jedes „frei“ eine Nachprüfung oder vergrößerte
Dreiecke), (2) feste Blätter des Trace-Baums sieht es nicht (Traversierung bleibt), (3) die
Dreieckstests sind nur 7–8 % einer `-bhedgpubake`-Zeit (Messschalter `BHED_GPUBAKE_MESS=1`:
md_am_sith 37,6 → 30,3 s *ohne jeden* Dreieckstest, kejim_post 5,9 → 5,4 s). Höchstgewinn
also ein paar Prozent bei großem Aufwand (die ganze Pipeline in D3D12 oder Interop) –
**ausgeschaltet gelassen**. Intel UHD: „the GPU has no DXR tier 1.1 – not used“, Ergebnis
gleich (ctest `q3map2_gpu`).

## Punkt 4: Messung

RTX 4070 Laptop, i9-14900HX (32 Threads), Idle, ein Prozess zur Zeit, Median aus drei, der
ganze `-light`-Schritt; CPU = bhed_q3map2 ohne Schalter (32 Threads), GPU = `-bhedgpubake`
mit dem Vorgabe-Bericht. GPU+DXR: nicht eingebaut (siehe Punkt 3).

| Karte | Schalter | CPU 32 Threads (Median) | GPU (Median) | Faktor | Illuminate CPU/GPU | Gitter CPU/GPU | Luxel gleich | Lumps |
|---|---|---|---|---|---|---|---|---|
| mp__duel1 | `-fast -patchshadows -samples 2 -filter -bounce 8` | 1,8 s | 1,2 s | 1,5 | 1,2 / 0,6 s | 0,0 / 0,0 s | 100.0000 % | alle 18 gleich |
| mp__duel1 | `-fast -patchshadows -samples 2 -filter` | 0,4 s | 0,4 s | 1,0 | 0,0 / 0,0 s | 0,0 / 0,0 s | 100.0000 % | alle 18 gleich |
| mp__duel1 | `-fast -patchshadows -samples 4 -filter -bounce 2` | 1,1 s | 0,8 s | 1,3 | 0,6 / 0,4 s | 0,0 / 0,0 s | 100.0000 % | alle 18 gleich |
| md_ch_ben | `-fast -patchshadows -samples 2 -filter -bounce 8` | 2,5 s | 2,2 s | 1,1 | 0,5 / 0,2 s | 0,7 / 0,7 s | 100.0000 % | alle 18 gleich |
| md_ch_ben | `-fast -patchshadows -samples 2 -filter` | 1,7 s | 1,6 s | 1,1 | 0,2 / 0,0 s | 0,7 / 0,7 s | 100.0000 % | alle 18 gleich |
| md_ch_ben | `-fast -patchshadows -samples 4 -filter -bounce 2` | 2,3 s | 2,1 s | 1,1 | 0,4 / 0,2 s | 0,7 / 0,7 s | 100.0000 % | alle 18 gleich |
| kejim_post | `-fast -filter -super 2` | 8,5 s | 6,2 s | 1,4 | 2,2 / 0,9 s | 1,8 / 0,6 s | 100.0000 % | alle 18 gleich |
| kejim_post | `-fast -filter -super 2 -bounce 2` | 48,8 s | 20,0 s | 2,4 | 35,9 / 7,5 s | 1,8 / 0,7 s | 100.0000 % | alle 18 gleich |
| kejim_post | `-fast -filter -super 2 -samples 3` | 9,0 s | 7,3 s | 1,2 | 2,9 / 2,1 s | 1,7 / 0,7 s | 100.0000 % | alle 18 gleich |
| md_am_sith | `-fast -patchshadows -samples 2 -filter -bounce 8` | 62,0 s | 31,3 s | 2,0 | 28,7 / 10,8 s | 18,4 / 2,5 s | 100.0000 % | alle 18 gleich |
| md_am_sith | `-fast -patchshadows -samples 2 -filter` | 31,7 s | 12,1 s | 2,6 | 8,0 / 3,9 s | 18,4 / 2,3 s | 100.0000 % | alle 18 gleich |
| md_am_sith | `-fast -patchshadows -samples 4 -filter -bounce 2` | 52,9 s | 28,0 s | 1,9 | 25,5 / 15,6 s | 18,7 / 2,3 s | 100.0000 % | alle 18 gleich |
| duel_mygeeto | `-fast -patchshadows -samples 2 -filter -bounce 8` | 74,3 s | 47,1 s | 1,6 | 45,7 / 18,0 s | 9,8 / 7,4 s | 100.0000 % | alle 18 gleich |
| duel_mygeeto | `-fast -patchshadows -samples 2 -filter` | 31,6 s | 16,6 s | 1,9 | 20,5 / 6,7 s | 10,1 / 7,4 s | 100.0000 % | alle 18 gleich |
| duel_mygeeto | `-fast -patchshadows -samples 4 -filter -bounce 2` | 67,2 s | 101,3 s | 0,7 | 47,9 / 84,4 s | 9,5 / 7,4 s | 100.0000 % | alle 18 gleich |

Bemerkungen: mp__duel1 ohne Bounce und md_ch_ben sind gleich schnell (die Lichtrechnung ist
dort ein kleiner Teil; md_ch_ben: das Gitter mit 25 Sonnen-/Himmelslichtern und langen
`testAll`-Strahlen ist auf der GPU nicht schneller). duel_mygeeto mit `-samples 4` ist auf
der GPU **langsamer** (101 s gegen 67 s): das tiefe Nachabtasten (bis 84 Abtastungen je
Kandidat, 2,3 M Kandidaten) verzweigt je Luxel anders; vorher war es durch eine zu kleine
Dispatch-Größe noch 293 s (behoben in c254a71d). md_am_sith mit `-samples 4` ist trotzdem
1,9× schneller.

### Intel UHD (ohne double, float-Näherungen in den knappen Fällen)

| Karte | Zeit | gleiche Luxel (Stichprobe) | Lumps |
|---|---|---|---|
| mp__duel1 | 9,1 s | 44,10 % (max. 0,013) | 10, 14 anders |
| md_ch_ben | 7,4 s | (Stichprobe vor der Erst-Lightmap-Regel: 0 Luxel) | 14 anders |
| kejim_post | 13,4 s | 90,13 % (max. 0,27) | 10, 14, 15, 17 anders |
| md_am_sith | 94,0 s | 76,28 % (max. 0,45) | 10, 14, 15, 17 anders |
| duel_mygeeto | 147,7 s | 86,27 % (max. 0,25) | 10, 13, 14, 15, 17 anders |

Die Unterschiede sind winzig (Mittel ≤ 0,0008), aber nicht null – auf GPUs ohne double ist der
Schalter nicht Byte-gleich (Handbuch: für den endgültigen Bau aus).

## Punkt 5: Editor

- **Build > GPU bake** (`BhedGpuBake`, Vorgabe `karte::kGpuBackenVorgabe = false` in
  `include/bhed/karte/editordaten.h` – **die eine Stelle**, falls shank es zum Standard macht;
  ohne passende GPU fällt bhed_q3map2 von selbst auf die CPU zurück): jeder `-light`-Schritt
  bekommt `-bhedgpubake`; das Schnell-Backen des Echtzeit-Lichts `-bhedgpubake
  -bhedgpubakecheck 0`.
- Die GPU steht in der Statuszeile („… - GPU: NVIDIA …“) und im Fortschrittsfenster; nach dem
  Bau der übersetzte Abweichungsbericht in der Konsole (Warnfarbe, wenn eine Lightmap abweicht)
  und im Tooltip des Menüpunkts; „keine nutzbare GPU“ ebenso.
- Tests: ctest `karte_bau` (Schalter, Leser der echten Ausgabezeilen); Selbsttest-Teil
  `stufe8g` nach dem Tutorial (BHED_KARTE_NUR=stufe8g): Klick auf den Menüpunkt, [Final] Fast,
  Schalter im Befehl, GPU und 100-%-Bericht in der Konsole, **alle 18 Lumps gleich dem
  CPU-Bau**, Einstellung gespeichert und wieder aus – 40 OK, 0 FEHLER. Unterwegs gefunden
  (Testfehler): die .bsp-Datei trägt zwischen Kopf und Lumps die Uhrzeit des Baus – verglichen
  werden die Lumps.

## Punkt 6: Handbuch

Seite `karte_gpubacken` (vier Sprachen, nach „Terrain, Spielen, Logik …“): Bedienung, was auf
der GPU läuft, warum gleich, Rückfälle, Intel-Hinweis, Messtabelle (aus `messung8g.py`), die
Schalter, DXR-Vortest. Bilder `h_karte_gpubacken_menue/_fortschritt/_bericht` aus
`BHED_HB_NUR=gpubacken` in vier Sprachen (je 28 OK, 0 FEHLER). `gen_handbuch.py --pruefen`:
ok (145 Seiten × 4 Sprachen).

## Punkt 7: Schlussrunde

Skript `SP/s8g/ende_s8g.sh` (nach 8c, ein Prozess zur Zeit, Idle, 8-GB-Wächter), Summe `SP/s8g/ende_summe.txt`.

| Schritt | Ergebnis |
|---|---|
| Bau | ok |
| ctest gesamt | 133/134 – `karte_grenzen`: Zeilenverweis der Grenzentabelle auf light.cpp:1878 (MAX_MAP_LIGHTGRID, durch den Gitter-Code verschoben) → behoben (dcef9f2b, grenzen.json und Handbuch neu); danach `karte_grenzen`, `handbuch_sprachen` grün |
| karteneditor voll (mit großer Karte) | 3140 OK, 1 FEHLER: Maß des Build-Menüs (ein Eintrag mehr) → behoben (dcef9f2b); Wiederholung **3141 OK, 0 FEHLER** (885 s, Spitze 1022 MB) |
| Modus 1 | **2645 OK, 0 FEHLER** (437 s) |
| übrige 22 Modi | alle **0 FEHLER** (karte 227, kartenfunde 53, fix604b 32, laserfalle 8, geschuetzanim 3, zielstrahl 2, effektgrund 1, figurdeform 1, jk2 15, tuerlauf 13, kamera 95, missionsbau 95, szenen 26, ton 34, npcklasse 12, gluehen 8, gras 13, spulen 13, figurstufen 8, missionsliste 11, rest605 6, modellimfesten 1 OK) |
| Leistung gegen rc605 (md_am_sith, je 2 Läufe) | Vorbereitung rc605 5234/5183 ms, 8g 5123/5235 ms; Bild Mittel 11/4,9/8 ms gleich, max. 157/145 gegen 142/141 ms – gleich |
| Kartenprüfung (278 MD-Karten, gegen 8c) | alle Summen gleich (0 Abstürze, 0 nicht geladen, 0 D3D-Meldungen, 1 → 1 nicht gezeichnete Entity) |
| sauberer Neubau | 0 Fehler, 130 Warnungen (428 s; 8c: 130 nach Behebung) – keine aus geänderten Dateien |
| q3map2-ctests (je Punkt) | 11/11; `q3map2_gpubake` 13 Prüfungen, `q3map2_gpu` mit DXR-Vortest |

## Offene Punkte

- duel_mygeeto `-samples 4` ist auf der GPU langsamer: das Nachabtasten bräuchte eine
  Ebenen-weise Abarbeitung (alle Kandidaten einer Rekursionsstufe je Dispatch) statt einer
  Schleife je Faden.
- Intel UHD: ohne double nicht Byte-gleich; ein Weg wäre, die knappen Fälle je Paar auf der CPU
  nachzurechnen (statt der float-Näherung) – oder auf solchen GPUs den Schalter verbieten.
- Lichtgitter mit vielen Sonnen (md_ch_ben) ist auf der GPU nicht schneller (lange
  testAll-Strahlen, Dreieckstests).
- Kleine Karten: die GPU lohnt erst ab einigen Sekunden Lichtrechnung; ein „automatisch“-Modus
  könnte nach Lichter×Luxel entscheiden.
- DXR: nur mit vergrößerten Dreiecken + Nachprüfung der Grenzfälle und einer Lösung für feste
  Blätter denkbar; Gewinn ≤ 7–8 %.
- GPU-Vorschau mit Bounce in der Kamera (aus 8c): weiter offen.
- Vertexlicht, `RadCreateDiffuseLights`, Speichern bleiben CPU (auf md_am_sith zusammen ~8 s).

## Changelog

## English

- Map editor: **Build → GPU bake** computes the whole light of a map on the graphics card – lightmaps with supersampling and filter, every bounce round and the light grid – with the same calculations in the same order as the processor, so the compiled map is byte for byte the same. On big maps it is up to 2.7 times faster than 32 processor threads; small maps gain little. Off by default; after the build the console shows a deviation report (lightmaps checked on the processor). Without a suitable graphics card the processor computes as always.
- bhed_q3map2: `-bhedgpubake` no longer needs d3dcompiler_47.dll (the shaders are built into the program) and starts the graphics card while the map loads.

## Deutsch

- Karten-Editor: **Build → GPU-Backen** rechnet das ganze Licht einer Karte auf der Grafikkarte – Lightmaps mit Supersampling und Filter, jede Bounce-Runde und das Lichtgitter – mit denselben Rechnungen in derselben Reihenfolge wie der Prozessor; die gebaute Karte ist also Byte für Byte dieselbe. Auf großen Karten bis zu 2,7-mal schneller als 32 Prozessor-Threads, kleine Karten gewinnen wenig. Zu Beginn aus; nach dem Bau zeigt die Konsole einen Abweichungsbericht (auf dem Prozessor nachgerechnete Lightmaps). Ohne passende Grafikkarte rechnet der Prozessor wie immer.
- bhed_q3map2: `-bhedgpubake` braucht kein d3dcompiler_47.dll mehr (die Shader sind im Programm) und startet die Grafikkarte, während die Karte lädt.

## 中文

- 地图编辑器:**Build → GPU 烘焙** 在显卡上计算地图的全部光照——带超采样和滤波的光照贴图、每一轮反弹以及光照网格——计算和顺序都与处理器相同,因此编译出的地图逐字节相同。在大地图上最多比 32 个处理器线程快 2.7 倍,小地图收益不大。默认关闭;编译后控制台会显示偏差报告(由处理器复核的光照贴图)。没有合适的显卡时,处理器照常计算。
- bhed_q3map2:`-bhedgpubake` 不再需要 d3dcompiler_47.dll(着色器已内置于程序中),并在加载地图时就启动显卡。

## 日本語

- マップエディター:**Build → GPU ベイク** はマップの光をすべてグラフィックカードで計算します(スーパーサンプリングとフィルター付きのライトマップ、すべてのバウンス、ライトグリッド)。計算も順序もプロセッサーと同じなので、コンパイルされたマップはバイト単位で同じです。大きなマップでは 32 スレッドのプロセッサーより最大 2.7 倍速く、小さなマップでは効果はわずかです。最初はオフです。ビルドの後、コンソールに差分レポート(プロセッサーで再計算したライトマップ)が表示されます。適したグラフィックカードがなければ、プロセッサーが通常どおり計算します。
- bhed_q3map2:`-bhedgpubake` は d3dcompiler_47.dll を必要としなくなり(シェーダーはプログラムに組み込み)、マップの読み込み中にグラフィックカードを起動します。
