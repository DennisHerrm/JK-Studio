# Notizen LCB1 (10.11.): Licht-Cache auch mit -bounce, bytegleich - rc612 Punkt 1

Auftrag `auftrag_lcb1_1011.md` (Rueckmeldung eines Mappers 09.11.: „VIS nach 1,5 min fertig, aber LIGHT schaetzt 5 Stunden
aus dem letzten Bau und rechnet das Licht wirklich komplett neu.“). Ursache wie im Handbuch dokumentiert
(`karte_teilbau.html:42`): mit `-bounce` schaltete sich `-bhedlightcache` ganz ab („light cache off (-bounce ...)“), das
Fenster Baufortschritt sagte nichts davon, und die Restzeit kam aus dem letzten vollen Bau. Arbeitsort `Y:\karte`, Zweig
`karte1` ab 41bde01b8 (rc611), Skripte und Messungen `Y:\werkzeuge\lcb1`.

## Punkt 1: nachgestellt (rot)

- Neue Testkarte `tests/karte/q3map2/zweiraum.map` (Erzeuger `Y:\werkzeuge\lcb1\zweiraum_gen.py`): zwei Raeume, verbunden
  durch einen U-foermigen Gang (A sieht B nicht), je ein Licht, eines im Gang.
- ctest `q3map2_teilbau` Teil e (neu, `BHED_TEILBAU_NUR=bounce` allein): je Fall erster Bau mit Cache, Aenderung, Teilbau mit
  Cache gegen vollen Bau ohne Cache - .bsp und jede externe Lightmap per Bytevergleich, dazu die Treffer je Durchgang.
- Mit dem rc611-`bhed_q3map2.exe`: 15 von 15 Faellen rot („light cache off“, keine Zeile je Durchgang, kein Cache geschrieben):
  bounce 1/2/8, [Final] Fast, [Final] Extreme Quality, -super 2 -bouncegrid, -external, nichts geaendert, nur
  Nachbearbeitung, Entity, Brush, Shader, GPU-Backen, Wechsel CPU -> GPU. Ergebnis jedes Laufs: alles neu gerechnet.
- Dabei gefunden: zwei volle Baeue derselben Karte unterscheiden sich in genau einem Byte-Bereich - q3map2 (wie NRCs)
  schreibt „I LOVE MY Q3MAP2 ... on <Datum Uhrzeit>“ in den Kopf der .bsp. Der Vergleich nimmt nur diese Uhrzeit aus, alles
  andere (Kopf, alle Lumps, Abstaende, externe Lightmaps) muss gleich sein.
- Zeiten je Phase auf der grossen Karte: siehe Punkt 5 (Spalte „alt“).

## Punkt 2: Licht-Cache je Durchgang (gruen)

`src/q3map2/bhed_lichtcache.cpp/.h`, `light.cpp` (LightWorld, Bounce-Schleife), `light_ydnar.cpp` (BhedIlluminateRawLightmaps,
GPU-Pfad, IlluminateVertexes).

- **Je Durchgang ein Abschnitt** in `<karte>.bhedlight` (Format `BHLICHT3`): der erste Durchgang und jeder Bounce. Der Kopf
  traegt zwei Schluessel: die Licht-Schalter (ohne Nachbearbeitung) und die Geometrie (Lumps ohne die von -light geschriebenen
  Felder, .srf, alle Shader-Skripte, worldspawn ohne Nachbearbeitungs-Schluessel, Brush-Entities). Ist einer anders, greift
  nichts, und der Lauf sagt welcher. Die neue Datei wird waehrend des Laufs als `.bhedlight.neu` geschrieben und erst am Ende
  umbenannt - ein abgebrochener Bau laesst den alten Cache stehen.
- **Fingerabdruck je Lightmap und Durchgang** (128 Bit): Durchgangsnummer, der Zufallsgenerator ihres Arbeitsschritts
  (`BhedNextPassSeed`), ihre beschnittene Lichtliste (jedes Licht mit allen Feldern, die LightContributionToSample liest, in
  Listenfolge - im Bounce sind das die Flaechenlichter, die RadCreateDiffuseLights aus dem Durchgang davor gemacht hat und die
  sie laut PVS/Huelle erreichen) und ihr eigener Stand am Anfang des Durchgangs, soweit IlluminateRawLightmap ihn liest: Stile,
  Umgebungslicht (im Bounce 0), von Stil-Lightmap 0 die nicht abgebildeten Luxel (abgebildete bekommen zuerst Umgebung und
  Zaehler 1, Stile 1-3 werden geleert - nur ihr Dasein zaehlt; mit den Debug-Fuellungen alles), Flags, Cluster (der
  Filterdurchgang flutet), Deluxel. Gleich -> der Stand nach dem Durchgang (vor dem Naehen) wird Bit fuer Bit uebernommen.
- **Warum bitgleich**: IlluminateRawLightmap ist eine Funktion genau dieser Eingaben und der festen Geometrie (Schluessel).
  Naehen (StitchSurfaceLightmaps), StoreSurfaceLightmaps und RadCreateDiffuseLights laufen in jedem Durchgang ueber alle
  Lightmaps wie immer; sie lesen nur, was die Durchgaenge hinterlassen. Induktiv ist der Stand jeder Lightmap nach jedem
  Durchgang derselbe wie im vollen Bau: uebernommen nur bei gleichen Eingaben, sonst aus demselben Stand neu gerechnet.
  Reihenfolge und Threads: die Lichtliste ist in Listenfolge (Stufe 6 stellt die 1-Thread-Folge der Bounce-Lichter her), jede
  Lightmap rechnet mit dem Generator ihrer eigenen Nummer - wer sie rechnet und wann, aendert nichts. GPU: dieselben Rechnungen
  in derselben Reihenfolge (Stufe 8g, double); der Fall „Cache vom Prozessor, Teilbau auf der GPU“ ist bytegleich zum vollen
  GPU-Bau.
- **Vor dem Durchgang** (`BhedLichtCacheVorab`, auf allen Threads): Lichtliste wie IlluminateRawLightmap_ sie macht, Abdruck,
  Treffer wiederhergestellt. Der Prozessor-Durchgang ueberspringt sie (`g_bhedVorab`), der GPU-Pfad nimmt sie in keine Liste -
  vorher rechnete die GPU auch Lightmaps aus dem Cache und warf das Ergebnis weg. Die Nummerierung der Durchgaenge bleibt.
- **Vertexbeleuchtete Flaechen**: Abdruck aus Durchgang, Generator, Lichtliste, Vertex-Stilen und den Vertex-Luxeln (im Bounce
  `vertLuxel += radVertLuxel`).
- **Lichtgitter**: je Durchgang mit Gitter (erster, mit `-bouncegrid` jeder Bounce) aus allen Lichtern, dem rohen Gitter davor
  und den Gitter-Urspruengen (`BhedPrepareGridOrigins`, rand() des Hauptthreads); gleich -> roh uebernommen, nur der
  Speicherteil laeuft (auf derselben Zahl Arbeitsschritte).
- **Hash** jetzt wortweise (2 x 64 Bit ueber 8-Byte-Woerter) statt byteweise FNV - der Geometrieschluessel brauchte auf grossen
  Karten Sekunden.
- Ausgabe je Durchgang: `BehavEd: light cache pass 3 of 9: 812 of 840 lightmaps from the cache (2.5 % of the work to trace)`
  (Arbeit = Luxel x Lichter), danach `... vertex lit surfaces ..., grid ...`; am Ende die bekannte Summenzeile ueber alle
  Durchgaenge.
- Nicht wiederverwendet (neu gerechnet, absichtlich): jede Aenderung an Geometrie, Shadern oder Brush-Entities - Lightmap-
  und Flaechennummern, Cluster und Schattengeometrie haengen am ganzen BSP; eine sichere Zuordnung ueber Bauten gibt es nicht.
  Ebenso andere Licht-Schalter (auch eine andere Bounce-Zahl).

ctest `q3map2_teilbau` Teil e gruen (alle Faelle, 0 Fehlschlaege, mit den alten Teilen a-d):

| Fall | aus dem Cache je Durchgang (Lightmaps) | Dateien gleich |
|---|---|---|
| -bounce 1, Licht verschoben | 9/18, 6/18 | ja |
| -bounce 2, Licht verschoben | 9/18, 6/18, 6/18 | ja |
| -bounce 8, Licht verschoben | 9, 6, 6, 6, 5, 2, 2, 0, 0 von 18 | ja |
| [Final] Fast, Licht verschoben | 9, 6, 6, 6, 2, 2, 2, 0, 0 von 18 | ja |
| [Final] Extreme Quality, Licht verschoben | 9, 6, 0, ... von 18 | ja |
| -super 2 -bouncegrid -bounce 2 | 9/18, 6/18, 6/18 | ja |
| -external -bounce 2 (3 Dateien) | 9/18, 6/18, 6/18 | ja |
| -bounce 2, nichts geaendert | 18/18 je Durchgang | ja |
| -bounce 2, nur Nachbearbeitung (-gamma, -exposure, -saturation, -compensate) | 18/18 je Durchgang | ja |
| -bounce 2, Entity (info_player_start) | 18/18 je Durchgang | ja |
| -bounce 2, Brush (Detail in Raum B) | 0/24 („geometry ... changed“) | ja |
| -bounce 2, Shader (Wand in Raum A) | 0/18 („geometry ... changed“) | ja |
| [Final] Fast / Extreme mit GPU-Backen | wie ohne GPU | ja |
| [Final] Fast, Cache vom Prozessor, Teilbau auf der GPU | wie ohne GPU | ja |

Eine Licht-Aenderung breitet sich mit jedem Bounce aus (das zurueckgeworfene Licht erreicht den anderen Raum ueber den Gang),
darum nehmen spaetere Durchgaenge weniger aus dem Cache - jedes Bit, das sich aendern kann, wird neu gerechnet.

## Punkt 3: GPU-Backen und Bounce

- Belegt: Auto (`-bhedgpubakeauto`) entscheidet einmal je -light-Schritt; waehlt es die GPU, gilt `g_bhedBakeAn` fuer jeden
  Durchgang - BhedIlluminateRawLightmaps ist dieselbe Funktion im ersten Durchgang und in jedem Bounce. zweiraum.map,
  [Final] Fast, Auto mit gemerkten Zeiten CPU 60 s / GPU 1 s: „GPU (learnedgpu)“, Bericht „9 rounds, 127 lightmaps on the GPU
  ... 35 on the CPU“ = 18 x 9 (erster Durchgang + 8 Bounces), 35 davon nahm der Prozessor, waehrend die GPU rechnete.
- Mit dem Licht-Cache geht eine Lightmap, die in einem Durchgang aus dem Cache kommt, nicht mehr an die GPU (s. Punkt 2).
- Das Lichtgitter mit `-bouncegrid` rechnet BhedTraceGridAlle ebenfalls je Bounce auf der GPU (unveraendert).
- Nichts nachzuziehen.

## Punkt 4: Anzeige und Schaetzung

- `include/bhed/karte/baufortschritt.h` `BauLichtCache`, `bauLichtCacheZeile`: liest „found“, „not usable: <Grund>“
  (kein Cache, aelteres Format, beschaedigt, Licht-Schalter, Geometrie/Shader/Brush-Entities), die Zeile je Durchgang und die
  Summe. Die Zeile des Durchgangs steht zwischen dem Kopf von IlluminateRawLightmap und dessen Pacifier - der Beobachter
  entscheidet erst am Zeilenende, ob er den Pacifier weiter erwartet (rot im ctest: Pacifier -1).
- Fenster **Baufortschritt** im Licht-Schritt (`bauLichtCacheText`): „Licht-Cache: 22 von 22 Lightmaps uebernommen (Durchgang 5
  von 17)“, „Licht-Cache gefunden ...“ vor dem ersten Durchgang, orange „Licht-Cache greift nicht: <Grund> - jede Lightmap wird
  neu gerechnet.“, ohne den Schalter grau „Licht-Cache aus: jede Lightmap wird gerechnet (Kompilieren > Teil-Kompilierung: Licht
  wiederverwenden)“. Nie still voll rechnen. Tooltip von „Licht wiederverwenden“ ohne „aus bei -bounce“. 6 Sprachen
  (`tools/i18n_lcb1.py`).
- **Restzeit** (`bauSchaetzen`): im Licht-Schritt mit `-bhedlightcache` zaehlen die IlluminateRawLightmap-Phasen des letzten
  Baus mit dem Arbeitsanteil, den der Cache jetzt neu rechnen laesst (vergangene Phasen dieses Laufs mit ihrem eigenen Anteil,
  kommende mit dem des letzten Durchgangs; greift der Cache nicht: voll). War der letzte Bau selbst ein Teilbau, rechnet sein
  gespeicherter Anteil (`karte_bauzeiten.json`, dritter Wert je Phase) auf den vollen Bau hoch. Das Tempo gegen den letzten Lauf
  vergleicht skalierte Zeiten. ctest karte_bau: 10 % neu -> 20 s statt 80 s; Cache greift nicht -> 80 s; letzter Bau ein
  Teilbau, jetzt alles neu -> 80 s (aus dem Teilbau allein 20 s); Anteil gespeichert und gelesen (rot vorher 3 FEHL, jetzt 100/0).
- Selbsttest `karteneditor` `BHED_KARTE_NUR=stufe8 BHED_STUFE8_NUR=teilbau`: Bounce-Eintrag - erst „greift nicht: Licht-Schalter“,
  nach einer Licht-Aenderung uebernimmt der erste Durchgang 4 von 16 und das Fenster nennt den letzten Durchgang (3 von 3), ohne
  den Schalter „Licht-Cache aus“. 21 OK / 0 FEHLER (allein; unter der Last von sieben Laeufen brach einmal der vorhandene erste
  Bau mit „timeout“ ab - Ueberwachung von NRC, nicht von LCB1).

## Punkt 5: Messung (allein)

`Y:\werkzeuge\lcb1\mess.py` (Protokolle `Y:\werkzeuge\lcb1\mess\`), allein auf der Maschine (i9-14900HX, RTX 4070 Laptop):
duel_mos_espa (Movie Duels, dekompiliert, 64 682 Raw-Lightmaps, 2 382 vertexbeleuchtete Flaechen), Spieldaten Movie Duels,
`-light -fast -patchshadows -samples 2 -filter -bounce 8` ([Final] Fast) mit `-bhedgpubake`. Aenderung: ein Licht (300) 64
Einheiten ueber dem Spielerstart mit der kleinsten x+y (Ecke, -13068 -5184 104). Der Teilbau, der volle Bau ohne Cache und der
Lauf mit dem rc611-exe bekamen dieselbe .bsp/.srf nach `-meta` und `-vis -bhedvisreuse` der geaenderten Karte.

| Phase (Sekunden, Summe ueber die Durchgaenge) | rc611 (heute) | voll, ohne Cache | Teilbau mit Cache |
|---|---|---|---|
| Bounce: IlluminateRawLightmap | 134,9 | 128,3 | 80,4 |
| Bounce: StoreSurfaceLightmaps | 20,6 | 19,2 | 19,2 |
| Bounce: RadCreateDiffuseLights | 19,9 | 20,3 | 10,7 |
| Store: collapsing | 17,4 | 15,1 | 15,8 |
| Laden, Setup* | 16,3 | 17,2 | 16,9 |
| Bounce: IlluminateVertexes | 16,1 | 15,0 | 1,7 |
| SetupTraceNodes | 6,2 | 6,3 | 6,2 |
| erster Durchgang IlluminateRawLightmap | 0,9 | 0,7 | 0,1 |
| **-light gesamt** | **241,3** | **230,8** | **160,0** |
| Teilbau auf dem Prozessor (Cache aus dem GPU-Bau) | | | 150,9 |

- Bau gesamt: voll `-meta` 17,6 + `-vis` 62,6 + `-light` 230,8 = 311 s; Teilbau `-meta` 23,0 + `-vis` (wiederverwendet) 0,2 +
  `-light` 160,0 = 183 s.
- Treffer je Durchgang (Lightmaps aus dem Cache / Anteil der Arbeit, der neu gerechnet wird): 64 498 (0,6 %), 63 806 (16,0 %),
  63 349 (16,2 %), 62 855 (16,4 %), 63 388 (14,4 %), 64 260 (15,5 %), 64 468 (21,2 %), 64 621 (0,1 %), 64 630 (0,4 %); zusammen
  575 875 von 582 138 Lightmaps (98,9 %) und 21 409 von 21 438 Flaechen. rc611: „light cache off (-bounce ...)“, alles neu.
- **Bytegleich**: die .bsp des Teilbaus (GPU und Prozessor) und des rc611-Laufs gleich dem vollen Bau ohne Cache (nur die
  Uhrzeit im Kopf ausgenommen); externe Lightmaps schreibt dieser Eintrag keine (geprueft im ctest mit `-external`).
- Warum nicht schneller: die Lightmaps, die das neue Licht erreicht, sind die grossen am Boden um den Startpunkt - in den Bounces
  2-7 je 16 % der Arbeit (Luxel x Lichter), aber 60-65 % der Zeit des Durchgangs (auf GPU und Prozessor gleich: 29 gegen 45 s im
  ersten Bounce), die uebrigen 98 % Lightmaps kosten fast nichts. Was bleibt, laeuft in jedem Bau ueber alle Lightmaps:
  StoreSurfaceLightmaps, das Zusammenlegen, RadCreateDiffuseLights, Laden und SetupTraceNodes - zusammen rund 70 s.
- Die Schaetzung des Fensters nimmt den Arbeitsanteil: hier 16 % statt der tatsaechlichen gut 60 % der Bounce-Zeit, also anfangs
  rund 120 statt 160 s fuer das Licht (vorher: 231 s aus dem letzten vollen Bau); das Tempo gegen den letzten Lauf (bis x2)
  zieht sie waehrend des Laufs nach.
- Der Cache dieser Karte mit 9 Durchgaengen ist 750 MB gross (je Durchgang ein Abschnitt mit allen Super-Luxeln).

## Handbuch

- „Bau-Fortschritt und Teil-Kompilierungen“: der Hinweis „Mit -bounce wird der Licht-Cache nicht benutzt“ ersetzt (je Durchgang,
  Ausbreitung ueber die Bounces, was alles neu rechnet, die Zeile im Fenster, die Restzeit); Bild `h_karte8_fortschritt` neu in
  6 Sprachen (zweiter Bau, nur `-gamma` geaendert: „22 von 22 Lightmaps uebernommen (Durchgang 5 von 17)“, angesehen), Unterschrift
  ohne feste Bounce-Zahl. „GPU-Backen“: Lightmaps aus dem Cache gehen nicht an die Grafikkarte, auch im Bounce. 6 Sprachen.

## Schlussrunde

(SCHLUSS)

## Offen

- Wiederverwenden ueber Geometrie-, Shader- und Brush-Entity-Aenderungen hinweg (je Lightmap nach Schattengeometrie) - nicht
  sicher bytegleich zuzuordnen; heute rechnet so ein Bau alles neu und sagt es.
- Andere Bounce-Zahl als im Cache: heute alles neu (die Schalter sind Teil des Schluessels).
- Restzeit: der Arbeitsanteil (Luxel x Lichter) unterschaetzt teure Lightmaps (Messung: 16 % Arbeit, gut 60 % Zeit). Besser
  waere die gemessene Zeit je Lightmap aus dem letzten Lauf im Cache - auf der GPU nicht je Lightmap messbar.
- Grosse Caches mit `-bounce` (duel_mos_espa: 750 MB neben der Karte).
- Fester Rest eines Teilbaus (StoreSurfaceLightmaps, Zusammenlegen, RadCreateDiffuseLights, Laden: ~70 s auf duel_mos_espa)
  laeuft wie im vollen Bau; Kandidaten fuer rc612 Punkt 2 (Tempo).

## Changelog

## English

- Map editor, partial compile "reuse light": the light cache now also works with `-bounce` (the [Final] build entries) - every pass is kept, and a lightmap is reused in a bounce pass when the bounced light reaching it is unchanged. The result stays byte-identical to a full build.
- The build progress window says what the light cache does ("Light cache: 812 of 840 lightmaps reused (pass 3 of 9)") or why it is not used (geometry changed, light switches changed, no cache yet, switch off) - it never silently computes everything.
- The remaining time of the light step counts only what the cache leaves to compute, no longer the hours of the last full build.
- GPU bake: lightmaps that come from the light cache are no longer sent to the graphics card.

## Deutsch

- Karten-Editor, Teil-Kompilierung „Licht wiederverwenden“: der Licht-Cache arbeitet jetzt auch mit `-bounce` (die [Final]-Einträge) - jeder Durchgang wird behalten, und eine Lightmap wird im Bounce übernommen, wenn das zurückgeworfene Licht, das sie erreicht, gleich ist. Das Ergebnis bleibt Byte für Byte gleich einem vollen Bau.
- Das Fenster Baufortschritt sagt, was der Licht-Cache tut („Licht-Cache: 812 von 840 Lightmaps übernommen (Durchgang 3 von 9)“) oder warum er nicht greift (Geometrie geändert, Licht-Schalter geändert, noch kein Cache, Schalter aus) - nie mehr still alles neu.
- Die Restzeit des Licht-Schritts zählt nur, was der Cache noch zu rechnen lässt, nicht mehr die Stunden des letzten vollen Baus.
- GPU-Backen: Lightmaps aus dem Licht-Cache gehen nicht mehr an die Grafikkarte.

## 中文

- 地图编辑器,部分编译“重用光照”:光照缓存现在也支持 `-bounce`([Final] 编译条目)——每一遍都会保存,反弹遍中到达某张光照贴图的反弹光未改变时就重用它。结果仍与完整编译逐字节相同。
- 构建进度窗口会显示光照缓存在做什么(“光照缓存:已重用 812 / 840 个光照贴图(第 3 / 9 遍)”),或缓存为何未被使用(几何体已更改、光照开关已更改、还没有缓存、开关关闭)——不再悄悄全部重算。
- 光照步骤的剩余时间只计算缓存留下的工作,不再是上一次完整编译的几个小时。
- GPU 烘焙:来自光照缓存的光照贴图不再送到显卡。

## 日本語

- マップエディター、部分コンパイル「ライトを再利用」: ライトキャッシュが `-bounce`([Final] のビルド項目)でも働くようになりました。各パスを保持し、届く反射光が変わっていないライトマップはバウンスのパスでも再利用します。結果はフルビルドとバイト単位で同じままです。
- ビルド進行ウィンドウに、ライトキャッシュの動作(「ライトキャッシュ: 812 / 840 個のライトマップを再利用(パス 3 / 9)」)または使われない理由(ジオメトリの変更、ライトのスイッチの変更、キャッシュなし、スイッチがオフ)が表示されます。黙ってすべてを計算し直すことはありません。
- ライトの段階の残り時間は、キャッシュが残した計算だけを数えます。前回のフルビルドの何時間もではありません。
- GPU ベイク: ライトキャッシュから取ったライトマップはグラフィックカードに送られなくなりました。

## Español

- Editor de mapas, compilación parcial «reutilizar la luz»: la caché de luz ahora también funciona con `-bounce` (las entradas [Final]): se guarda cada pasada, y un lightmap se reutiliza en una pasada de rebote si la luz rebotada que le llega no ha cambiado. El resultado sigue siendo idéntico byte a byte a una compilación completa.
- La ventana de progreso de compilación dice qué hace la caché de luz («Caché de luz: 812 de 840 lightmaps reutilizados (pasada 3 de 9)») o por qué no se usa (geometría cambiada, parámetros de luz cambiados, aún no hay caché, opción desactivada); nunca vuelve a calcularlo todo en silencio.
- El tiempo restante del paso de luz solo cuenta lo que la caché deja por calcular, ya no las horas de la última compilación completa.
- Horneado en GPU: los lightmaps que salen de la caché de luz ya no se envían a la tarjeta gráfica.

## Čeština

- Editor map, dílčí kompilace „znovu použít světlo“: mezipaměť světla nyní funguje i s `-bounce` (položky [Final]) – každý průchod se uchová a lightmapa se v průchodu odrazu znovu použije, když se odražené světlo, které na ni dopadá, nezměnilo. Výsledek zůstává bajt po bajtu stejný jako při úplném sestavení.
- Okno průběhu sestavení říká, co mezipaměť světla dělá („Mezipaměť světla: znovu použito 812 z 840 lightmap (průchod 3 z 9)“), nebo proč se nepoužije (změněná geometrie, změněné přepínače světla, zatím žádná mezipaměť, vypnutý přepínač) – už nikdy nepočítá vše potichu znovu.
- Zbývající čas kroku světla počítá jen to, co mezipaměť nechá spočítat, ne hodiny posledního úplného sestavení.
- Pečení na GPU: lightmapy z mezipaměti světla se už neposílají na grafickou kartu.
