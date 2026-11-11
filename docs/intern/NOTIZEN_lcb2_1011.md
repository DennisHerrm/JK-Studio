# Notizen LCB2 (10./11.11.): Licht-Cache nach Geometrie-Änderungen, weniger Festkosten, kleiner - rc612

Aufträge `auftrag_lcb2_1011.md` und `auftrag_lcb2b_1011.md`, Fortsetzung von LCB1 (`NOTIZEN_lcb1_1011.md`). Arbeitsort `Y:\karte`,
Zweig `karte1` ab ebf35f6a1, Skripte und Messungen `Y:\werkzeuge\lcb2`. Regel wie LCB1: das Ergebnis ist **bytegleich** zum
vollen Bau ohne Cache (.bsp bis auf die Uhrzeit im Kopf, externe Lightmaps) - lieber weniger Treffer als ein falsches Byte.

## Punkt 1: Licht-Cache nach Geometrie-, Shader- und Brush-Entity-Änderungen

- Rot (LCB1): jede solche Änderung rechnete alles neu („geometry ... changed“).
- **Zuordnung über Inhalt**: jede Raw-Lightmap bekommt einen Abdruck ihres festen Inhalts (Flächen mit Geometrie, Shader,
  Lightmap-Achsen und -Größe, Luxel-Lage, Cluster in kanonischer Nummerierung mit ihren PVS-Bits) statt ihrer Nummer; der
  Abdruck des Durchgangs nimmt dazu die beschnittene Lichtliste (Cluster der Lichter relativ zur Lightmap), ihren Stand und
  die **Signatur des Strahlbaums** in dem Raum, den ihre Strahlen durchqueren können (`BhedSpurRaumSig`, `light_trace.cpp`):
  ändert sich dort ein Brush, Patch oder Modell, ist der Abdruck anders und die Lightmap wird neu gerechnet.
- Format `BHLICHT4` (nach Inhalt), Kopf mit eigenem Schlüssel für die worldspawn (`worldspawn keys changed` rechnet alles neu).
  Vertexlicht und Lichtgitter werden nach einer Geometrie-Änderung bewusst neu gerechnet (sie hängen an rohen Flächen- und
  Clusternummern).
- `floodlightRGB` (und die drei anderen Floodlight-Felder der Raw-Lightmap) wechselte je Lauf, wird aber nicht gelesen -
  aus dem Abdruck genommen (sonst 0 Treffer auf der GPU).
- Fenster Baufortschritt: „Geometrie geändert, Lightmaps über ihren Inhalt zugeordnet“ (`KeLcGefundenInhalt`, 6 Sprachen);
  `Grund::Welt` für „worldspawn keys changed“.
- ctest `q3map2_teilbau` Teil f (`BHED_TEILBAU_NUR=geometrie`), zweiraum.map mit Säule, Patch, Modell und Brush-Entity in Raum
  B; je Fall Teilbau mit Cache gegen vollen Bau ohne Cache, .bsp und externe Lightmaps Byte für Byte: Struktur-Brush
  verschoben / dazu / gelöscht, Detail-Brush dazu, Patch verschoben, misc_model gedreht, Shader-Tausch, Brush-Entity verschoben,
  je bounce 0 und 1, eine Auswahl mit [Final] Fast (bounce 8) auf Prozessor und GPU: alle grün, Raum A kommt aus dem Cache.
  Teil e: „Brush (Detail in Raum B)“ und „Shader (Wand in Raum A)“ erwarten jetzt Treffer.
- **Fehler im Abdruck gefunden und behoben (erst auf der großen offenen Karte sichtbar)**: `BhedLmVorab` hielt den Raum zu den
  Punkt- und Flächenlichtern als Verweis auf `raum.komp[0]`; jede Sonne in der Lichtliste hängt ein Element an, der Vektor
  zieht um, der Verweis zeigt in freigegebenen Speicher. Folgen: (a) der Raum ging verloren oder endete an den Lichtern - eine
  Lightmap konnte aus dem Cache kommen, obwohl sich zwischen ihr und einem Punktlicht etwas geändert hatte (auf zweiraum
  blieb das unsichtbar, weil dort ein Blatt des Strahlbaums den ganzen Raum deckt); (b) Schreiben in freigegebenen Speicher.
  Rot: Teilbau duel_jakku (71 Sonnen, Kiste verschoben + Licht) endete in 3 von 5 Läufen mit 0xC0000374 bzw. 0xC0000005
  (bounce 8) und in 2 von 8 Läufen mit 0xC0000374 (bounce 0, `absturz.py`, `abs_rot.txt`); Heap-Prüfung an jeder Marke
  (`dbg_heap.py`): Heap in Ordnung bis vor die Vorab-Schleife des ersten Durchgangs, Absturz darin. Behoben: der Raum liegt in
  einer eigenen Variable und wird vor dem Abdruck an seinen Platz gesetzt. Grün (11.11.): derselbe Lauf mit dem Stand
  9d215b83f 0 von 20 (bounce 0, `abs_gruen.txt`; bei 2 von 8 wären 20 Läufe ohne Absturz ein Zufall von 0,3 %), jeder Lauf
  bytegleich zum vollen Bau, 2118 Treffer wie zuvor; mit dem Endstand (sauberer Neubau, `exe_final`) 0 von 10 (bounce 0, `abs_ende.txt`, in jedem Lauf 3637 Treffer und T = V), dazu sieben Teilbaue mit bounce 8 auf duel_jakku ohne Absturz (rot waren 3 von 5). Die Testkarte von Teil f hat
  jetzt eine Sonne (light mit `_sun`) und den Fall „Detail-Platte vor der Wand dazu“ (Schatten eines Punktlichts auf Wand und
  Decke, die Platte liegt nicht über diesen Lightmaps).

## Punkt 1c (11.11.): offene Karten - warum duel_jakku nur 23 % traf, und was sich geändert hat

Auftrag der Fortsetzung: die Trefferquote auf duel_jakku (Struktur-Kiste um 32 verschoben + ein Licht, [Final] Fast, 9 Durchgänge:
74 334 von 328 779 Lightmaps = 22,6 %, erster Durchgang 2118 von 36 531 = 5,8 %) klären und verbessern, ohne ein Byte zu ändern.

**Gemessen** (Diagnose-exe, das je Lightmap und Durchgang die Teile des Abdrucks schreibt - fester Teil, Lichtliste, Strahlbaum,
Stand; `diag_patch.py`, `diag_vergleich.py`, `diag_baum.py`, `diag_baum_vergleich.py` in `Y:\werkzeuge\lcb2`; der Diagnose-Code ist
nicht im Repo). Erster Durchgang, alter Strahlraum:

| Grund | Lightmaps | Luxel |
|---|---|---|
| kein Abdruck: mehr als 63 Sonnen in der Lichtliste (die Maske hatte 64 Bit) - nie im Cache, auch ohne jede Änderung | 2616 (7,2 %) | 56,0 % |
| Signatur des Strahlbaums anders | 27 703 (75,8 %) | 30,8 % |
| andere Lightmap (fester Teil: Flächen, Luxel, BSP in ihrer Box) | 3003 (8,2 %) | 8,2 % |
| Lichtliste und Strahlbaum anders (das neue Licht) | 1078 (3,0 %) | 1,8 % |
| Treffer | 2118 (5,8 %) | 3,1 % |

- **Die 63-Sonnen-Grenze**: duel_jakku hat 71 Sonnen (Himmelslicht `q3map_skylight` mit 5 Schritten = 65 Sonnen, dazu die
  Sonnen des Himmels). Jede Lightmap, die nach oben zeigt, hat alle in der Liste - die großen Boden-Lightmaps bekamen keinen
  Abdruck und wurden in jedem Bau neu gerechnet, auch wenn sich nur ein Licht geändert hatte.
- **Der Strahlraum einer Sonne** war die Box der Lightmap, über die ganze Strecke zur Sonne gezogen (524 288 Einheiten), und
  jede Ebene des Strahlbaums wurde gegen diese ganze Strecke geprüft - für eine achsparallele Ebene ist das die Hüllbox der
  Strecke, ein ganzer Quadrant der Karte. Mit 65 Sonnen über die Halbkugel deckte das jede Karte ganz: jede Änderung am
  Strahlbaum, wo auch immer, änderte jede Signatur.
- **Der Strahlbaum selbst ändert sich weit über die Kiste hinaus** (Vergleich der beiden Bäume, je 82 000 Blätter): die Seite
  der Struktur-Kiste ist eine Ebene des BSP (Tiefe 9, x = -576 gegen -544) mit 9267 Blättern darunter - der ganze Block wird
  anders geteilt; zwei Teilungsebenen von `SubdivideTraceNode_r` (der abgerundete Mittelwert aller Vertices eines Knotens)
  rücken um 1 (x = -603/-602 und 1990/1991, 4285 und 4447 Blätter darunter); dazu 650 einzelne Blätter. 87 % der Blätter
  sind gleich, aber diese Ebenen stehen wie Wände in der Karte: jeder Strahl, der eine kreuzt, wird an anderer Stelle
  geschnitten. Das Ergebnis eines Strahls hängt an diesen Schnitten (welche Blätter er besucht, entscheidet sich mit
  TRACE_ON_EPSILON an jeder Ebene; ein Strahl 0,05 hinter einer Ebene läuft nur durch das vordere Kind) - ohne gleiche Ebenen
  gibt es keinen Beweis für gleiche Bytes, also keinen Treffer. Mit 20 Richtungen des Himmelslichts je Höhenstufe kreuzt fast
  jede Lightmap eine dieser Wände mit einem ihrer Strahlen.

**Geändert** (`light_trace.cpp`, `BhedSpurRaumSig`; Abdruck-Kennung `lm5`):
- Jeder Strahlraum trägt beim Abstieg den Teil t0..t1 seiner Strecke mit, der den Knoten erreichen kann: was `TraceLine_r` an
  das vordere Kind gibt, liegt bei front >= -TRACE_ON_EPSILON, was an das hintere, bei back < TRACE_ON_EPSILON (der ganze Strahl
  oder das Stück bis zum Schnittpunkt auf der Ebene), und weil der Abstand zur Ebene in t linear ist, begrenzt das t für jedes
  Kind. Geprüft wird nur noch dieses Stück der Strecke - der Strahlraum einer Sonne ist ein Balken so breit wie die Lightmap
  statt eines Quadranten. Welche Ebenen dabei in den Abdruck kommen, hängt vom Weg ab, der Schluss nicht: eine Ebene im Abdruck
  schneidet wie `TraceLine_r`, eine Ebene, die nicht darin steht, hat jeden Strahl, der sie erreicht, auf einer Seite, und der
  Strom eines Knotens (Blatt-Abdruck, oder Ebene und die Ströme der Kinder) legt fest, was `TraceLine_r` dort mit einem Strahl
  tut - gleich in welchem Baum. Rundung wie bisher über `kBhedSpurRand` (2 Einheiten).
- Keine Grenze der Sonnenzahl mehr (Liste statt 64-Bit-Maske).
- Der Vorlauf wurde dabei billiger: `Licht-Cache: Vorab` im ersten Durchgang auf duel_jakku 11,3 s -> 2,0 s (unter Last), obwohl
  jetzt jede Lightmap einen Abdruck bekommt.

**Ergebnis** (alles bytegleich zum vollen Bau, T = V; Trefferzahlen sind exakt, keine Zeitmessung):

| duel_jakku, [Final] Fast, GPU | vorher | jetzt |
|---|---|---|
| Struktur-Kiste verschoben + Licht, alle Durchgänge | 74 334 von 328 779 (22,6 %) | 75 881 (23,1 %) |
| ... erster Durchgang | 2118 von 36 531 (5,8 %; 3,1 % der Luxel) | 3637 (10,0 %; 4,9 % der Luxel) |
| nur ein Licht dazu, alle Durchgänge | 266 992 von 328 725 (81,2 %) | 270 592 (82,3 %) |
| ... erster Durchgang | 31 387 von 36 525 (85,9 %), angezeigt 12,7 % der Zeit | 33 632 (92,1 %), angezeigt 6,2 % der Zeit |
| **Detail**-Kiste verschoben + Licht, alle Durchgänge | 251 285 von 328 779 (76,4 %) | 268 038 von 328 779 (81,5 %) |
| ... erster Durchgang | 16 163 von 36 531 (44,2 %), angezeigt 65,0 % der Zeit | 31 526 von 36 531 (86,3 %) |

- Nach der Änderung im ersten Durchgang (Struktur-Kiste): Strahlbaum anders 28 195 (77,2 %, 72,4 % der Luxel), andere Lightmap
  3455 (9,5 %, 19,2 %), Licht + Strahlbaum 1231, Treffer 3637. **Der Rest ist die Karte, nicht der Strahlraum**: eine
  Struktur-Kiste ändert das BSP, und ihre Ebenen (und die verrückten Teilungsebenen) kreuzt unter 71 Sonnen fast jede
  Lightmap. Dieselbe Kiste als **Detail-Brush** lässt das BSP gleich: 81,5 % aus dem Cache statt 23,1 %.
- In den Bounce-Durchgängen (Struktur-Kiste) ist meist die Lichtliste anders (die Bounce-Lichter aus den geänderten Lightmaps
  reichen auf der offenen Karte weit): zweiter Durchgang Licht + Strahlbaum 38,5 %, nur Strahlbaum 41,1 %; im neunten 46 % Treffer.
- Für Mapper steht der Rat im Handbuch (6 Sprachen): unter freiem Himmel Detail- statt Struktur-Brushes verschieben.
- ctest `q3map2_teilbau` **Teil g** (`BHED_TEILBAU_NUR=offen`): ein Hof 4096 x 4096 unter dem Himmel (eine Sonne 35 Grad hoch,
  Himmelslicht mit 65 Sonnen, eigener Himmels-Shader im Test-Spielordner), 14 Fälle (bounce 0/1/8, Prozessor und GPU): Licht
  verschoben, Detail-Kiste in der Ecke verschoben, Struktur-Kiste verschoben, Kiste + Licht, hohe Säule dazu (ihr Schatten
  fällt quer über den Hof), Säule gelöscht - jede .bsp Byte für Byte gleich dem vollen Bau. Erster Durchgang, von 65 Lightmaps
  (in Klammern das alte exe): Licht 47 (37), Detail-Kiste 40 (19), Struktur-Kiste 21 (18), Kiste + Licht 21 (18), hohe Säule 8
  von 70 (8), Säule gelöscht 0 von 60 (0; die Flächen werden anders zu Lightmaps zusammengelegt). Verlangt: Licht und
  Detail-Kiste mehr als die Hälfte. **Rot** mit dem exe vor der Änderung: 3 Fehlschläge (Detail-Kiste 19 von 65), **grün**: 0
  (`g_rot2.txt`, `g_gruen2.txt`).
- Auf dem Hof, Struktur-Kiste: 7 Lightmaps sind andere (73 % der Luxel - der Boden wird neu zerlegt), 37 scheitern am
  Strahlbaum (3 Stellen mit anderer Ebene, alle im Block der Kiste; die Balken des Himmelslichts kreuzen ihn).

## Punkt 1d (11.11.): der Cache eines angehaltenen Licht-Schritts (Schnittstelle zur Editor-Seite, VIS2c)

VIS2c (Y:\p_vis2, Anhalten/Fortsetzen im Karteneditor) macht beim Anhalten im Licht aus `<karte>.bhedlight.neu` den Cache des
angehaltenen Baus. Was der Licht-Cache dazu zusagt (`src/q3map2/bhed_lichtcache.cpp`):
- Dateien neben der Karte: `<karte>.bhedlight` (fertiger Cache, nur gelesen) und `<karte>.bhedlight.neu` (wird während `-light
  -bhedlightcache` geschrieben und am Ende von LightWorld umbenannt: remove + rename in `BhedLichtCacheSchreiben`). Ein neuer
  Lauf überschreibt eine liegengebliebene `.neu` (fopen "wb") - wer sie retten will, benennt sie vorher um.
- Format `BHLICHT5`: 8 Bytes Kennung, 3 x 16 Bytes (Schalter, worldspawn, Geometrie), dann je Durchgang i32 Nummer, u64 Länge,
  der gepackte Abschnitt. Geschrieben wird ein Abschnitt am Ende seines Durchgangs, und **er ist auf der Platte (fflush), bevor
  die Zeile `light cache pass n of m: ... vertex lit surfaces ...` ausgegeben wird** (das fflush von VIS2c ist hier schon
  enthalten). Mitten in einem Durchgang wird nichts gesichert.
- Der Leser nimmt einen Cache mit weniger Durchgängen (`k of n passes kept`) und zählt einen Abschnitt, den die Datei nicht bis
  zu seinem Ende enthält, nicht mit (neu: Länge gegen die Dateigröße geprüft - vorher wurde er mitgezählt und erst beim Laden
  verworfen, eine zerrissene Länge hätte so viel Speicher angefordert). Ein Abschnitt, der sich nicht entpacken lässt, wird neu
  gerechnet.
- Test in `q3map2_teilbau` Teil e: Cache mitten im dritten Durchgang abgeschnitten -> „2 of 3 passes kept“, Durchgänge 1 und 2
  ganz aus dem Cache, der dritte gerechnet, bytegleich zum vollen Bau, der neue Cache wieder ganz. Rot vorher („3 of 3“,
  `e_rot.txt`), grün (`e_gruen.txt`).
- **Beim Zusammenführen** mit p_vis2: in `bhed_lichtcache.cpp` die Fassung von karte1 nehmen (das fflush steht darin an
  derselben Stelle); in `gui/app_karte_bau.cpp` ändert LCB2 in `bauFortschrittZeichnen` nur die eine Zeile mit der Restzeit
  (bricht jetzt um, s. Punkt 5) - beide Seiten behalten.

## Punkt 2: Festkosten - Profil, dann gleiches Ergebnis schneller

Profil mit `BHED_Q3_ZEITEN` (neue, feinere Abschnitte: `light: BSP laden`, `ParseEntities`, `LoadMapFile`, `SetEntityOrigins`,
`Licht-Cache Schluessel`, `SetupBrushes`, `SetupSurfaceLightmaps` mit `Setup: Flaechen, Cluster` und `Setup: Raw-Lightmaps`,
`Store: sorting / allocating / storing`, `Rad: RadLight`, `Bounce: Lichter freigeben`, `Gitter-Vorlauf: Cluster`,
`Licht-Cache: Vorab / Daten merken / packen, schreiben / lesen, entpacken / Inhalt, Spurbaum`; `IlluminateRawLightmap` ist jetzt
nur noch das Rechnen). Jede Änderung ist eine reine Beschleunigung: dieselben Rechnungen in derselben Reihenfolge, das Ergebnis
bytegleich zum Bau mit dem LCB1-exe (s. Punkt 6). Sie gelten für jeden Bau, nicht nur für den Teilbau.

| Stelle | vorher (duel_mos_espa, unter Last) | Ursache | jetzt |
|---|---|---|---|
| `Store: collapsing` | 17,8 s (9 Store) | jede Lightmap gegen jede spätere (n²/2 Aufrufe von CompareBSPLuxels) | Kandidaten nach den Schlüsseln gruppiert, die CompareBSPLuxels gleich verlangt (solid, customWidth/Height, brightness, w/h bei nicht-solid, Stil 0 bei `_minlight`), in der Gruppe in der alten Reihenfolge: 0,7 s |
| eine Boden-Lightmap je Bounce | 58 von 81 s im Vollbau, 31 von 32 s im Teilbau (Durchgang 2), ein Thread | die GPU kann sie nicht (alpha shadow/unsafe), sie hat weniger Luxel als der Anteil eines Threads an ALLEN Lightmaps -> kein Pool | eine eigene Lightmap des Prozessors geht an den Pool-Thread, wenn sie dort früher fertig ist (Arbeit = Paare Licht x Luxel; vor ihm liegende Arbeit < (Faktor - 1) x eigene, Faktor = halbe Threadzahl); auf dem Prozessor-Weg zählt „groß“ gegen die Lightmaps, die der Durchgang rechnet |
| `SetupSurfaceLightmaps` | 17-26 s | (a) je Fläche alle Blätter abgelaufen (Flächen x Blätter), (b) der Merge bot jeder Raw-Lightmap jede spätere Fläche gleicher Eckdaten an | (a) die Blätter je Fläche einmal vorab; (b) Gitter über die Karte: nur Flächen ohne Raw-Lightmap, deren Grenzen die der Raw-Lightmap in dem Moment berühren (AddSurfaceToRawLightmap weist die anderen ab, bevor es etwas ändert) - dieselben Aufrufe in derselben Folge: 1,0-1,3 s |
| `Rad: RadLight` | 14-29 s (8 Bounces) | RadSubdivideDiffuseLight holte je Schnitt zwei `radWinding_t` (je 64 KB, genullt) vom Heap, alle Threads zugleich | ein Paar je Thread und Rekursionstiefe, „alle auf einer Seite“ kopiert nur die vorhandenen Vertices: 3,2 s |
| `Bounce: StoreSurfaceLightmaps` (projecting) | 14-15 s (8 Bounces) | EmitShader fragte ShaderInfoForShader für jeden BSP-Shader neu (je Fläche bis zu allen Shadern) | einmal je Name (dieselbe Antwort): 1,9 s |
| `Store: allocating` | 12-13 s (9 Store); duel_jakku 40-82 s | FindOutLightmaps probierte auf fast vollen Seiten jeden der 16 384 Ursprünge - und für jeden Stempel wieder von der ersten Seite an | (a) je Seite und Zeile der längste freie Lauf; Seiten und Zeilen, in denen der längste belegte Lauf des Stempels keinen Platz hat, werden übersprungen; (b) 11.11.: die Suche für einen Stempel **gleicher Form** geht dort weiter, wo die letzte endete (Seite, Zeile, Spalte) - die Seiten füllen sich während eines StoreSurfaceLightmaps nur, was eine Form einmal abgewiesen hat, weist sie wieder ab; dieselben Ursprünge in derselben Folge ohne die bekannten Fehlschläge, also dieselbe erste passende Stelle. duel_jakku: 22 300 Stempel in 745 Formen, 520 Millionen Tests je Store vorher. Im Messfenster (11.11. 14:20-14:22, 5 fremde Bau-/Testprozesse am Anfang, 0 am Ende; `platz_mess.py`: drei exe gleichzeitig, `-bounce 0 -threads 8`, also ein Store): duel_jakku 1,89 s vorher / 1,28 s mit (a) / **0,19 s** mit (a) + (b); duel_mos_espa 0,56 / 0,24 / 0,39 s - dort gewinnt (b) nichts und kostet 0,15 s je Store für den Schlüssel der Form (s. Offen); die .bsp aller drei exe gleich. Ein erstes Fenster um 14:08 war gestört (ein fremder Test lief hinein, die Sperre wurde fremd gelöscht): 3,94 / 2,59 / 0,26 s und 2,31 / 1,07 / 1,10 s. Über die 9 Store eines [Final]-Baus auf duel_jakku unter Last: 40-82 s -> 4-9 s. |
| `Gitter-Vorlauf` | 3-4,4 s (duel_jakku 13 s) | für jeden Gitterpunkt, dessen ganze Schubs-Box fest ist, 600 Aufrufe von rand() nur zum Weiterzählen | der Zustand von rand() des Hauptthreads wird aus fünf gezogenen Zahlen bestimmt (auf eigenem Thread geprüft, dass rand()/srand() der bekannte Generator sind), die Schleife zieht aus einer Kopie und überspringt mit BhedRandomSkip, am Ende srand(): 1,1 s |

Nicht angefasst (Profil unter Last, Teilbau duel_mos_espa): `SetupTraceNodes` 7-16 s (der Strahlbaum wird seriell gebaut),
`Licht-Cache: Vorab` 6-10 s über neun Durchgänge (Lichtlisten, Abdrücke mit Baum-Signatur, Treffer wiederherstellen - das sind
die **Kosten der Baum-Signatur**; `Licht-Cache: Inhalt, Spurbaum` einmalig 0,2-0,6 s), `light: BSP laden` 2-10 s,
`IlluminateVertexes`, und dass mit `bounceStore` nach jedem Bounce ganz gespeichert und die .bsp geschrieben wird (wie NRC;
`-nobouncestore` gibt es).

## Punkt 3: Cache kleiner

- Format `BHLICHT5`: jeder Durchgang liegt gepackt in der Datei, verlustfrei. Die Bytes jedes 4-Byte-Worts (float, int)
  werden in vier Ebenen sortiert (Vorzeichen-/Exponent-Bytes wiederholen sich, die unteren Mantissen-Bytes nicht), dann in
  Blöcken von 4 MB mit zlib Stufe 1 gepackt, auf allen Threads, jeder Block für sich (die Bytes hängen nicht an der
  Threadzahl). Blob: u64 Länge, u32 Blöcke, u32 gepackte Länge je Block, die Blöcke. Eine beschädigte Länge legt nichts an
  (deflate packt höchstens 1032 : 1), ein Durchgang, der sich nicht entpacken lässt, wird neu gerechnet.
- duel_mos_espa, [Final] Fast (9 Durchgänge, 64 682 Raw-Lightmaps): **786,3 MB (LCB1, `BHLICHT3`) -> 151,6 MB** (-81 %);
  roh je Durchgang 72,4 MB (der erste mit dem Lichtgitter 181,6 MB -> 7,5 MB), gepackt 3 bis 33 MB.
- Versucht und verworfen (Vorgänger-WIP): jeden Durchgang als XOR gegen den Durchgang davor zu speichern. Gemessen mit
  `packprobe.py` auf demselben Cache: mit XOR 198,7 MB, ohne 151,6 MB, je Block das Kleinere 144,8 MB. Ein Bounce erreicht
  nicht dieselben Luxel wie der davor, und wo einer von beiden 0 ist, ist das XOR der andere. Die 4,5 % der dritten Fassung
  sind die doppelte Packzeit, den zweiten Durchgang im Speicher und die Abhängigkeit vom Durchgang davor nicht wert.
- Zeiten (duel_mos_espa, unter Last, Richtwert): Packen und Schreiben aller neun Durchgänge 1,8-3,1 s, Lesen und Entpacken
  0,8-1,2 s, „Daten merken“ 0,6-1,2 s (eigene Abschnitte `Licht-Cache: ...` in `BHED_Q3_ZEITEN`).
- Aufräumen: wie bisher wird die neue Datei als `.bhedlight.neu` geschrieben und erst am Ende umbenannt; ein älteres Format
  („older cache format“) wird beim nächsten Bau ersetzt. Mehr liegt nicht neben der Karte.

## Punkt 4: Schätzung aus gemessener Zeit

- `IlluminateRawLightmap_` ist ein Zeit-Wrapper; je Raw-Lightmap und Durchgang stehen im Cache zwei Zeiten zu ihrem Inhalts-
  Abdruck (`LmFest::sig`): die **Thread-Sekunden** (mit dem Pool: Wandzeit x Pool-Faktor) und die **eigene Wandzeit**. Eine
  Lightmap aus dem Cache behält die Zeiten des Laufs, der sie gerechnet hat; neue Inhalte zählen mit Luxel x Lichter zur
  Rate der bekannten.
- Anteil eines Durchgangs = max( Summe neu / Threads, längste neu ) / max( Summe alle / Threads, längste alle ): ein
  Durchgang dauert so lange wie seine Thread-Sekunden verteilt auf die Threads - oder wie seine längste Lightmap. Ausgabe
  unverändert „(x % of the time to trace)“, die Oberfläche liest nur die Zahl (`karte_bau` angepasst).
- **Geklärt: „auf der GPU hat keine Lightmap eine gemessene Zeit“ stimmt nicht.** `zeiten.py` auf den Cache: alle 64 682
  Lightmaps jedes Durchgangs haben eine Zeit - jede läuft durch `IlluminateRawLightmap_`, die GPU-Lightmaps mit dem
  Fertigstellen auf dem Prozessor (die Rechenzeit der Grafikkarte je Block ist keiner Lightmap zugeordnet und fehlt in der
  Summe). Die erste Fassung (Summe neu / Summe alle) lag trotzdem weit daneben: angezeigt 8,9 / 7,8 / 6,8 / 4,5 %, gemessen
  49 / 53 / 52 / 49 % (Durchgänge 2-5) - weil der Durchgang an EINER Lightmap auf einem Thread hing (s. Punkt 2), nicht an
  der Summe. Mit dem Pool für diese Lightmap und der längsten Lightmap in der Formel: angezeigt 25 / 30 / 26 / 15 % gegen gemessen
  18 / 18 / 23 / 31 % (Durchgänge 2-5, duel_mos_espa ein Licht dazu, unter hoher Last - Richtwert).
- Bleibt ungenau, wenn die Grafikkarte der Engpass ist (ihre Zeit zählt nicht mit) und in den letzten, sehr kurzen
  Durchgängen (unter 0,5 s); die Restzeit des Fensters zieht das Tempo gegen den letzten Lauf wie bisher nach.

## Punkt 5: Handbuch

- „Bau-Fortschritt und Teil-Kompilierungen“ (6 Sprachen): nach Geometrie-, Shader- und Brush-Entity-Änderungen werden die
  Lightmaps über ihren Inhalt gefunden (ein in einem Raum verschobener Brush lässt die anderen Räume im Cache; Vertex-Licht
  und Lichtgitter neu), die Restzeit zählt die gemessene Zeit je Lightmap, der Cache wird gepackt gespeichert.
- Bild `h_karte8_fortschritt` neu in 6 Sprachen: das ganze Programmfenster (2538 x 1472) statt des 840 breiten
  Ausschnitts, das Fenster Baufortschritt darin markiert (Rahmen und Pfeil), „22 von 22 Lightmaps übernommen (Durchgang n
  von 17)“. Der Selbsttest parkt die Maus vorher auf der Statuszeile - sonst stand der Tooltip eines Reiters im Bild.
  Angesehen (11.11.): alle sechs (en in voller Größe, de/zh/es/ja/cs in halber): das ganze Fenster, kein Tooltip, das Fenster Baufortschritt mit Rahmen und Pfeil, die Cache-Zeile in der Sprache des Bildes, nichts abgeschnitten.
- Beim Ansehen gefunden: auf Spanisch war die Zeile „vergangen - Restzeit“ am Fensterrand abgeschnitten („... de este ma“);
  sie bricht jetzt um (`gui/app_karte_bau.cpp`, eine Zeile).
- Neuer Hinweis im selben Abschnitt (6 Sprachen): unter freiem Himmel kreuzen die Strahlen zur Sonne und zum Himmelslicht
  die ganze Karte, und die Seiten eines Struktur-Brushes sind Ebenen des BSP - wo es geht, Detail-Brushes verschieben.
- Grenzen-Tabelle: Prüfstelle `MAX_MAP_LIGHTGRID` jetzt `light.cpp:2149` (`grenzen.cpp`, `tools/handbuch/grenzen.json`).

## Punkt 6: Messung

Alle Zeiten **unter Last** gemessen (Richtwerte: je Lauf liefen 17-57 fremde MSBuild, bis 10 fremde q3map2, 3-9 jkstudio,
dazu eigene Läufe nebeneinander) - Wandzeit, in Klammern die CPU-Zeit des Prozesses; [Final] Fast (`-fast -patchshadows -samples 2
-filter -bounce 8`) mit GPU-Backen. Trefferzahlen und Bytevergleiche hängen nicht an der Last. Die endgültigen Zeiten kommen aus
`messung_final.sh` im Ruhefenster (s. Offen).

| Karte, Änderung | N0 voll + Cache | V voll | T Teilbau | aus dem Cache (alle Durchgänge) | Bytes |
|---|---|---|---|---|---|
| duel_mos_espa, ein Licht dazu (10.11., Stand ohne Platzieren) | 282 s (2927) | 210 s (2862) | 127 s (666) | - | T = V, R0 = N0 |
| duel_mos_espa, Struktur-Kiste verschoben + Licht (10.11.) | - | 265 s | 195 s | 571 822 von 582 192 (98,2 %) | T = V, RV = V |
| duel_mos_espa, Struktur-Kiste verschoben + Licht (11.11., Endstand von q3map2) | 256 s (2831) | 321 s (2580) | 407 s (1410) | 566 809 von 582 192 (97,4 %) | T = V, RV = V |
| duel_jakku, Struktur-Kiste verschoben + Licht (11.11.) | 213 s (993) | 167 s (752) | 127 s (794) | 75 881 von 328 779 (23,1 %) | T = V, RV = V |
| duel_jakku, Detail-Kiste verschoben + Licht (11.11.) | 198 s (880) | 114 s (956) | 107 s (505) | 268 038 von 328 779 (81,5 %) | T = V |
| duel_jakku, ein Licht dazu (11.11., noch mit dem alten Platzieren) | 289 s (1018) | 235 s (841) | 197 s (440) | 270 592 von 328 725 (82,3 %) | T = V |

- RV = voller Bau der geänderten Karte mit der LCB1-exe (`Y:\werkzeuge\lcb1\neu\bhed_q3map2.exe`: altes Zusammenlegen, altes
  Platzieren, alter Cache) - dieselbe .bsp. Auf duel_jakku brauchte sie im selben Fenster 387 s (1058) gegen 167 s (752).
- Cache duel_jakku (9 Durchgänge, 36 531 Raw-Lightmaps): 89,8 MB.
- duel_mos_espa am 11.11.: der Teilbau brauchte unter dieser Last mehr Wandzeit als der volle Bau bei halber CPU-Zeit (neben dem
  eigenen Neubau und 51 fremden MSBuild; die Durchgänge 2-4 hängen an der einen großen Boden-Lightmap, die nach der Kisten-Änderung
  neu gerechnet wird: 56 / 55 / 118 s gegen 59 / 67 / 45 s im vollen Bau) - keine Aussage ohne das Ruhefenster. Die LCB1-exe
  brauchte für denselben vollen Bau 898 s (3114).
- **Trefferzahlen zweier Sitzungen auf duel_mos_espa sind nicht genau vergleichbar**: das Mehrkern-VIS liefert je Lauf leicht
  andere PVS-Bits, und die stehen im Abdruck (10.11.: 571 822, 11.11.: 566 809 und 568 144). Auf denselben .bsp (`gleiche_bsp.sh`):
  exe vor der Änderung 568 144, Endstand 568 472 von 582 192 (erster Durchgang beide 64 488 von 64 688) - die Einengung des
  Strahlraums kostet auf der geschlossenen Karte keinen Treffer. Auf duel_jakku rechnet VIS 0,4 s und ist in jedem Lauf gleich
  (2118 Treffer in jedem Lauf mit dem alten Strahlraum).
- Je Phase (duel_jakku, Struktur-Kiste, 11.11., unter Last, Summe über 9 Durchgänge bzw. 9 Store, V / T): `Bounce:
  IlluminateRawLightmap` 60,5 / 42,2 s, `IlluminateRawLightmap` 28,7 / 11,4 s, `Store: collapsing` 22,4 / 14,0 s, `Store: allocating`
  9,3 / 7,3 s, `Licht-Cache: Vorab` - / 10,5 s, `Licht-Cache: packen, schreiben` - / 3,6 s, `Gitter-Vorlauf` 12,8 / 5,9 s.

## Schlussrunde

Schlussrunde je Zweig nach `parallel_1011.md` („Schlussrunde je Zweig ab 11.11.“): Bau ohne Warnungen, ctest, die eigenen Tests und die
Gruppen, die den geänderten Code berühren, Lints. Die volle Runde läuft einmal auf dem zusammengeführten Stand.

| Schritt | Ergebnis |
|---|---|
| Bau | 0 Warnungen, 0 Fehler (`bau24.txt`): ein sauberer Neubau von `build-test` samt q3map2 (clang, 461 Schritte) - er war gestartet, bevor die Regel „inkrementell genügt“ kam. Stand der exe: Baum 218037d21 - danach kamen nur noch die Aufteilung des ctest (CMakeLists.txt, kein Quelltext), die Bilder es/ja/cs und die Notizen. |
| ctest | 246 Tests mit `-j 16`: 245 grün; `q3map2_teilbau` habe ich nach 1200 s beendet, als es noch im Teil e stand (Zeitgrenze 1800 s) -> in vier Tests geteilt (`BHED_TEILBAU_NUR=karten/bounce/geometrie/offen`), die vier grün: 44 / 634 / 1213 / 239 s unter Last, zusammen 2130 s - als ein Test wäre es über die Zeitgrenze gelaufen (`ct8_alle.txt`, `ct9_teilbau.txt`). Zusammen 249 von 249. |
| q3map2-Gruppen | in ctest: q3map2_teilbau (4), _determinismus, _gpubake, _gpu, _vergleich (gegen NRCs .bsp), _pr316, _visgleich, _zusammen, _funde, _pfade, _importe, _uebersetzer, dazu karte_grenzen, karte_bau - alle grün. |
| eigene Tests | Absturz-Lauf mit `exe_final` 0 von 10; Bytevergleiche: duel_jakku Struktur-Kiste (T = V, RV = V), Detail-Kiste (T = V), Licht (T = V), duel_mos_espa Struktur-Kiste (T = V, RV = V); Platzieren: .bsp der drei exe gleich (beide Karten). |
| Selbsttest | karteneditor `BHED_KARTE_NUR=stufe8` (Fenster Baufortschritt mit den Licht-Cache-Zeilen, Teil-Kompilierungen, Rückgewinnung): **44 OK, 0 FEHLER** (`ke8b.txt`). Der erste Lauf neben den eigenen Läufen hatte 38 OK, 9 FEHLER - alles Zeitgrenzen unter Last: die Bau-Überwachung bricht wie NRCs watchbsp ab, wenn q3map2 sich nicht binnen 5 s verbindet („build stopped ... (timeout)“), und die Rückgewinnung der misc_model (Index über 1572 Modelle) braucht dann mehr als 180 s. |
| Handbuch | `handbuch_sprachen: ok` (war zwischendurch rot: der englische Schlüssel der es/cs-Texte muss dem geänderten englischen Text folgen); Bilder s. Punkt 5. Im Selbsttest `handbuch` (nur `BHED_HB_NUR=karte8` für die Bilder gelaufen) schlug „Rückgewinnung fertig: nicht nach 180 s“ unter Last in jedem Lauf fehl - derselbe Schritt ist in der Gruppe stufe8 grün. |
| Lints | alle 33 Lints und die Handbuch-Prüfung zeilengleich mit dem Ergebnis der Basis ebf35f6a1 (`lint_neu2.txt` gegen `Y:\werkzeuge\lcb1\lint_neu.txt`). |
| bewusst nicht gelaufen | Modus 1, die übrigen karteneditor-Gruppen (auch stufe8c/8g mit dem GPU-Backen und alle, die Karten bauen), menueeditor, Docks, der Selbsttest `handbuch` als Ganzes - volle Runde auf dem zusammengeführten Stand. |
| Adressen | 0 in Log und Diff ab ebf35f6a1. |

## Offen

- Die endgültige Messtabelle (Ruhefenster): `Y:\werkzeuge\lcb2\messung_final.sh` -> `messung_final.txt` (rund 35 min, braucht
  `exe_final\bhed_q3map2.exe` - liegt da, Stand in `exe_final\STAND.txt`); bis dahin stehen unter Punkt 6 nur Richtwerte unter Last.
- Offene Karten, Struktur-Brush geändert: die Trefferquote bleibt niedrig, solange der Strahlbaum sich ändert (Punkt 1c). Mehr
  Treffer gäbe nur ein Strahlbaum, der eine kleine Änderung klein lässt (`SubdivideTraceNode_r` teilt am Mittelwert aller
  Vertices) - das wäre ein anderer Baum als der von q3map2 und damit andere Bytes als NRC, also nicht ohne Entscheidung.
- Bounce-Durchgänge: der Strahlraum zu den Punkt- und Flächenlichtern ist eine einzige Box um Lightmap und alle Lichter ihrer
  Liste; je Licht ein Kegel (dieselbe Einengung wie bei den Sonnen) wäre enger, bei Tausenden Bounce-Lichtern aber nur gruppiert
  bezahlbar - nicht gemacht, Nutzen ungemessen.
- Platzieren (b) auf duel_mos_espa: der Schlüssel der Form (ein Byte je Luxel des Stempels) kostet dort mehr, als das Fortsetzen
  spart (0,24 -> 0,39 s je Store, im [Final]-Bau rund 1,4 s von über 200 s); die Luxel ließen sich als Bits in der Schleife
  sammeln, die den Stempel ohnehin abläuft - nicht gemacht (es hieße die q3map2-Gruppen noch einmal laufen lassen).
- Ein deterministischer Rot-Test für den falschen Treffer aus dem Abdruck-Fehler (Punkt 1) fehlt weiter; der Beleg ist der
  Absturz (2 von 8 / 3 von 5 rot, 0 von 20 grün) und der Code.
- Vertexlicht und Lichtgitter werden nach einer Geometrie-Änderung ganz neu gerechnet (rohe Flächen-/Clusternummern im
  Abdruck); andere Bounce-Zahl oder andere Licht-Schalter: alles neu (wie LCB1).
- `SetupTraceNodes` (seriell, 6-16 s auf duel_mos_espa), `Licht-Cache: Vorab` (0,7-1,2 s je Durchgang, auch wenn fast alles
  aus dem Cache kommt), `light: BSP laden`, das Speichern und Schreiben der .bsp nach jedem Bounce (`bounceStore`),
  `Store: collapsing` auf duel_jakku (0,7-2,5 s je Store unter Last - jetzt mehr als das Platzieren).
- Schätzung: die Rechenzeit der Grafikkarte ist keiner Lightmap zugeordnet; Durchgänge, die an der GPU hängen, werden
  unterschätzt.
- Der Pool-Faktor (halbe Threadzahl) ist eine Annahme, keine Messung; wer rechnet, ändert kein Byte, nur die Dauer.
- Eine Lightmap, die die Arbeiter von der GPU-Liste übernehmen, geht wie bisher nur an den Pool, wenn sie mehr Luxel hat
  als der Anteil eines Threads an allen Lightmaps.

## Changelog

## English

- Map editor, partial compile "reuse light": the light cache now also works after changes to geometry, shaders and brush entities. Lightmaps are found by their content, and a lightmap is reused when nothing its rays can meet has changed - a brush moved in one room leaves the lightmaps of the other rooms in the cache. The result stays byte-identical to a full build.
- The light step is faster in every build, with the same bytes: merging identical lightmaps, setting up the lightmaps, the bounce lights between the passes and storing the lightmaps take a fraction of their time, and the one lightmap a bounce pass used to wait for is now lit by all threads.
- The light cache file is packed and about a fifth of its size.
- The remaining time of a partial compile comes from the time measured for every lightmap in the last build.
- The build progress window says when the geometry has changed and the lightmaps were matched by their content.
- The light cache now also keeps lightmaps lit by more than 63 suns (a sky light with many samples) - on such maps the big outdoor lightmaps were traced again in every build - and the rays to the sun and the sky no longer make a lightmap depend on the whole map: after a light or a detail brush changed, most of an open map comes from the cache (duel_jakku, a detail box moved and a light added: 82 % of the lightmaps). The handbook says why a structural brush under an open sky changes far more than a detail brush.
- Placing the lightmaps on the lightmap pages is up to ten times faster on big maps (the same pages), and the line with the elapsed and the remaining time in the build progress window wraps instead of being cut off (Spanish).

## Deutsch

- Karten-Editor, Teil-Kompilierung „Licht wiederverwenden“: der Licht-Cache arbeitet jetzt auch nach Änderungen an Geometrie, Shadern und Brush-Entities. Lightmaps werden über ihren Inhalt gefunden, und eine Lightmap wird übernommen, wenn sich nichts geändert hat, was ihre Strahlen treffen können - ein in einem Raum verschobener Brush lässt die Lightmaps der anderen Räume im Cache. Das Ergebnis bleibt Byte für Byte gleich einem vollen Bau.
- Der Licht-Schritt ist in jedem Bau schneller, bei gleichen Bytes: das Zusammenlegen gleicher Lightmaps, das Einrichten der Lightmaps, die Bounce-Lichter zwischen den Durchgängen und das Speichern der Lightmaps brauchen einen Bruchteil ihrer Zeit, und die eine Lightmap, auf die ein Bounce-Durchgang bisher wartete, rechnen jetzt alle Threads.
- Die Datei des Licht-Caches ist gepackt und etwa ein Fünftel so groß.
- Die Restzeit einer Teil-Kompilierung kommt aus der Zeit, die im letzten Bau für jede Lightmap gemessen wurde.
- Das Fenster Baufortschritt sagt, wenn die Geometrie geändert ist und die Lightmaps über ihren Inhalt zugeordnet wurden.
- Der Licht-Cache behält jetzt auch Lightmaps unter mehr als 63 Sonnen (ein Himmelslicht mit vielen Proben) - auf solchen Karten wurden die großen Lightmaps im Freien in jedem Bau neu gerechnet -, und die Strahlen zur Sonne und zum Himmel machen eine Lightmap nicht mehr von der ganzen Karte abhängig: nach einem geänderten Licht oder Detail-Brush kommt der größte Teil einer offenen Karte aus dem Cache (duel_jakku, eine Detail-Kiste verschoben und ein Licht dazu: 82 % der Lightmaps). Das Handbuch erklärt, warum ein Struktur-Brush unter freiem Himmel viel mehr ändert als ein Detail-Brush.
- Das Platzieren der Lightmaps auf den Lightmap-Seiten ist auf großen Karten bis zu zehnmal schneller (dieselben Seiten), und die Zeile mit der vergangenen und der restlichen Zeit im Fenster Baufortschritt bricht um, statt abgeschnitten zu werden (Spanisch).

## 中文

- 地图编辑器,部分编译“重用光照”:几何体、着色器和画刷实体改动后,光照缓存现在也能工作。光照贴图按内容查找,只要其光线可能碰到的东西都没有改变就重用——在一个房间里移动画刷,其他房间的光照贴图仍留在缓存中。结果仍与完整编译逐字节相同。
- 光照步骤在每次编译中都更快,字节不变:合并相同的光照贴图、设置光照贴图、各遍之间的反弹光源以及保存光照贴图只需原来的一小部分时间;以前让一遍反弹等待的那一张光照贴图,现在由所有线程一起计算。
- 光照缓存文件经过压缩,大小约为原来的五分之一。
- 部分编译的剩余时间来自上一次编译中每张光照贴图实测的时间。
- 构建进度窗口会说明几何体已更改、光照贴图已按内容匹配。
- 光照缓存现在也保留被超过 63 个太阳照亮的光照贴图(采样很多的天空光)——在这类地图上,室外的大张光照贴图以前每次编译都要重新计算——而且射向太阳和天空的光线不再让一张光照贴图依赖整张地图:改动一盏灯或一个细节画刷后,露天地图的大部分都来自缓存(duel_jakku,移动一个细节箱子并加一盏灯:82% 的光照贴图)。手册说明了为什么在露天下结构画刷的改动比细节画刷大得多。
- 在大地图上,把光照贴图排到光照贴图页上的速度最多快十倍(页面相同);构建进度窗口中显示已用时间和剩余时间的那一行会换行,不再被截断(西班牙语)。

## 日本語

- マップエディター、部分コンパイル「ライトを再利用」: ジオメトリ、シェーダー、ブラシエンティティを変更した後でもライトキャッシュが働くようになりました。ライトマップは内容で探し、光線が当たりうるものが何も変わっていなければ再利用します。ある部屋でブラシを動かしても、他の部屋のライトマップはキャッシュに残ります。結果はフルビルドとバイト単位で同じままです。
- ライトの段階はどのビルドでも速くなり、バイトは同じです: 同じライトマップの統合、ライトマップの準備、パスの間のバウンスライト、ライトマップの保存にかかる時間はわずかになり、バウンスのパスが待っていた 1 枚のライトマップは全スレッドで計算します。
- ライトキャッシュのファイルは圧縮され、大きさは約 5 分の 1 です。
- 部分コンパイルの残り時間は、前回のビルドで各ライトマップについて測った時間から求めます。
- ビルド進行ウィンドウに、ジオメトリが変更されライトマップを内容で対応づけたことが表示されます。
- ライトキャッシュは 63 個を超える太陽に照らされるライトマップも保持するようになりました(サンプルの多いスカイライト)。こうしたマップでは屋外の大きなライトマップがビルドのたびに計算し直されていました。また、太陽と空への光線のためにライトマップがマップ全体に依存することもなくなりました。ライトやディテールブラシを変えた後、開けたマップの大部分はキャッシュから取られます(duel_jakku でディテールの箱を動かしライトを 1 つ追加: ライトマップの 82 %)。開けた空の下で構造ブラシがディテールブラシよりずっと多くを変える理由はマニュアルに書いてあります。
- 大きなマップでは、ライトマップをライトマップページに配置する処理が最大で 10 倍速くなりました(ページは同じ)。ビルド進行ウィンドウの経過時間と残り時間の行は、切れずに折り返すようになりました(スペイン語)。

## Español

- Editor de mapas, compilación parcial «reutilizar la luz»: la caché de luz ahora también funciona tras cambios en la geometría, los shaders y las entidades de brush. Los lightmaps se buscan por su contenido, y un lightmap se reutiliza si no ha cambiado nada que sus rayos puedan encontrar: un brush movido en una sala deja en la caché los lightmaps de las demás salas. El resultado sigue siendo idéntico byte a byte a una compilación completa.
- El paso de luz es más rápido en cada compilación, con los mismos bytes: fusionar lightmaps idénticos, preparar los lightmaps, las luces de rebote entre pasadas y guardar los lightmaps tardan una fracción de su tiempo, y el lightmap por el que antes esperaba una pasada de rebote lo calculan ahora todos los hilos.
- El archivo de la caché de luz está comprimido y ocupa aproximadamente una quinta parte.
- El tiempo restante de una compilación parcial se obtiene del tiempo medido para cada lightmap en la última compilación.
- La ventana de progreso de compilación indica cuándo ha cambiado la geometría y los lightmaps se han asignado por su contenido.
- La caché de luz conserva ahora también los lightmaps iluminados por más de 63 soles (una luz de cielo con muchas muestras) - en esos mapas los grandes lightmaps de exterior se volvían a calcular en cada compilación - y los rayos hacia el sol y el cielo ya no hacen que un lightmap dependa de todo el mapa: tras cambiar una luz o un brush de detalle, la mayor parte de un mapa abierto sale de la caché (duel_jakku, una caja de detalle movida y una luz añadida: el 82 % de los lightmaps). El manual explica por qué un brush estructural a cielo abierto cambia mucho más que uno de detalle.
- Colocar los lightmaps en las páginas de lightmaps es hasta diez veces más rápido en mapas grandes (las mismas páginas), y la línea con el tiempo transcurrido y el restante en la ventana de progreso de compilación salta de línea en lugar de quedar cortada.

## Čeština

- Editor map, dílčí kompilace „znovu použít světlo“: mezipaměť světla nyní funguje i po změnách geometrie, shaderů a brushových entit. Lightmapy se hledají podle obsahu a lightmapa se znovu použije, když se nezměnilo nic, co mohou její paprsky zasáhnout – brush posunutý v jedné místnosti nechá lightmapy ostatních místností v mezipaměti. Výsledek zůstává bajt po bajtu stejný jako při úplném sestavení.
- Krok světla je rychlejší při každém sestavení, se stejnými bajty: slučování stejných lightmap, příprava lightmap, odražená světla mezi průchody a ukládání lightmap trvají zlomek původního času a tu jednu lightmapu, na kterou průchod odrazu dříve čekal, teď počítají všechna vlákna.
- Soubor mezipaměti světla je zabalený a má zhruba pětinovou velikost.
- Zbývající čas dílčí kompilace vychází z času naměřeného pro každou lightmapu při posledním sestavení.
- Okno průběhu sestavení říká, když se změnila geometrie a lightmapy byly přiřazeny podle obsahu.
- Mezipaměť světla nyní uchovává i lightmapy osvětlené více než 63 slunci (světlo oblohy s mnoha vzorky) - na takových mapách se velké venkovní lightmapy počítaly znovu při každém sestavení - a paprsky ke slunci a k obloze už nezpůsobují, že lightmapa závisí na celé mapě: po změně světla nebo detailního brushe pochází většina otevřené mapy z mezipaměti (duel_jakku, posunutá detailní bedna a přidané světlo: 82 % lightmap). Příručka vysvětluje, proč strukturální brush pod otevřeným nebem změní mnohem víc než detailní.
- Umisťování lightmap na stránky lightmap je na velkých mapách až desetkrát rychlejší (stejné stránky) a řádek s uplynulým a zbývajícím časem v okně průběhu sestavení se zalamuje, místo aby byl oříznut (španělština).
