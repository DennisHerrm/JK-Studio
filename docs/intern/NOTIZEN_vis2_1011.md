# Notizen vis2 / vis2b (10.11.): VIS – deterministisch schneller, Anhalten und Fortsetzen (q3map2-Seite)

Aufträge `auftrag_vis2_1011.md` und `auftrag_vis2b_1011.md`. Worktree `Y:\p_vis2`, Zweig `p_vis2` ab a00f6cf90, Bau
`Y:/p_vis2/build`. Werkzeuge, Karten, Läufe und Messfassungen: `Y:\werkzeuge\vis2` (Inhalt in `UEBERGABE.md` dort).
Rechner: i9-14900HX (30 nutzbare Threads), 64 GB. **Alle Zeiten dieser Notiz sind unter Last gemessen (Richtwerte)**:
die anderen Agenten bauten und testeten durchgehend, auch in den Messfenstern liefen deren Bauten weiter (Fremdlast
je Lauf 9 bis 25 Kerne, in `messung.txt` je Zeile). Belastbar sind die Verhältnisse im selben Fenster, die CPU-Zeiten
und die Bytevergleiche. Die endgültige Tabelle misst `Y:\werkzeuge\vis2\messung_final.sh` im Ruhefenster.

Bezug für „gleich“ ist überall das Ergebnis von `-threads 1` (alle Lumps ohne den Kopf mit NRCs Zeitstempel).

## Stand in einem Satz

Der deterministische VIS (`-bhedvisexact`) ist auf offenen Karten 1,4- bis 2-mal so schnell wie vorher, erreicht die
heutige Vorgabe (NRCs Regel) aber nicht – **die Vorgabe bleibt NRCs Regel**; `-vis` lässt sich in beiden Regeln
anhalten und fortsetzen (q3map2-Seite fertig und im ctest), die Editor-Seite ist nicht begonnen.

## 1. Was die Läufe der heutigen Vorgabe unterscheidet (vis2b Punkt 1)

`pvsdiff.py` zählt die sichtbaren Cluster-Paare (gesetzte Bits des Visibility-Lumps) gegen das Ergebnis von `-threads 1`:

| Karte | Cluster | Paare `-threads 1` | Läufe NRC-Regel (32 Threads) | mehr Paare | nur in `-threads 1` | nur im Lauf | Obermenge? |
|---|---|---|---|---|---|---|---|
| t3_byss | 2885 | 1 306 381 | 5 | +444 … +623 (+0,034 … +0,048 %) | 77 … 132 | 542 … 755 | nie |
| duel_mos_espa | 2224 | 1 144 422 | 2 | +44, +53 (+0,004, +0,005 %) | 3, 4 | 47, 57 | nie |
| md_ch_ben | 709 | 482 580 | 2 | 0 | 0 | 0 | gleich (geschlossene Karte) |
| ctf3 | 1069 | 311 618 | 2 | +17, +64 (+0,005, +0,021 %) | 8, 4 | 25, 68 | nie |
| Gitter 14 × 14 (ctest) | 1042 | 64 920 | 1 (angehalten, fortgesetzt) | +262 (+0,4 %) | | | |

- **Auf den offenen Karten ist kein Lauf der Vorgabe eine Obermenge** des `-threads 1`-Ergebnisses: jedem fehlen einige
  Paare, die der deterministische Lauf hat, und er hat einige Dutzend bis ein paar hundert mehr (md_ch_ben: beide Läufe gleich). Der Unterschied ist winzig (unter 0,05 % auf den großen
  Karten), im Spiel nicht zu sehen und nicht zu messen.
- Ursache (visflow.cpp): ein Fluss nimmt für ein passiertes Portal dessen fertiges `portalvis`, sonst `portalflood`.
  Mit mehreren Threads ist „fertig“ Zufall. Das wäre nur lockerer, gäbe es nicht den Trennebenen-Zwischenspeicher
  (SEPERATORCACHE): er gehört dem Rahmen und bekommt seinen Inhalt vom ersten Nachbarportal, das die Clip-Stelle erreicht;
  wird dieses dabei weggeschnitten, bleibt nur ein Anfangsstück der Ebenen, und alle späteren Nachbarn werden schwächer
  geschnitten. Wer zuerst kommt, hängt über „kann nichts Neues sehen“ vom bisher Gefundenen ab – also von der Reihenfolge.
  Darum geht es in beide Richtungen.
- Folgerung: „deterministisch“ ist nicht das bessere PVS, sondern das **reproduzierbare** (gleiche .map → gleiche .bsp,
  Anhalten/Fortsetzen Byte für Byte). Für die Entscheidung über die Vorgabe zählt allein die Zeit.

## 2. Wohin die Zeit geht (vis2 Punkt 1)

- Ganzer Bau duel_mos_espa (LCB1, 10.11., `NOTIZEN_lcb1_1011.md`): `-meta` 17,6 s + `-vis` 62,6 s + `-light` 230,8 s –
  VIS ist ein Fünftel, Licht drei Viertel. BasePortalVis und CreatePassages zusammen unter 1 s; die Zeit liegt im Fluss.
- Messfassung `diag\dep.patch` (je Fluss die Takte seines Threads und jede **echte** Abhängigkeit: ein früherer Fluss,
  dessen Endergebnis für die Abfrage nicht `portalflood` ist), Auswertung `dep_ana.py`:

| | t3_byss | duel_mos_espa |
|---|---|---|
| Flüsse | 14 848 | 13 704 |
| CPU-Summe (mit Messfassung) | 442 s | 1517 s |
| echte Abhängigkeiten | 1,89 Mio. (127 je Fluss) | 2,55 Mio. (186 je Fluss) |
| teuerste Flüsse | 5,0 / 3,9 / 3,2 s | **103,6 / 82,8 / 81,9 s** |
| kritischer Pfad (unendlich viele Kerne) | 14,4 s (98 Flüsse) | 178,4 s (141 Flüsse, davon der letzte 103,6 s) |
| Simulation 8 / 16 / 24 / 32 Kerne | 55,5 / 28,5 / 20,2 / 16,6 s | – / – / – / 180 s |
| Simulation 32 Kerne, höchstens 128 / 512 begonnene Flüsse | 20,4 / 16,8 s | 204 / 183 s |

- Der deterministische VIS **kann** also so schnell sein wie NRCs Regel (t3_byss 16 s, duel_mos_espa 169 s unter Last
  gemessen) – die echten Abhängigkeiten erlauben es. Auf duel_mos_espa begrenzt ein einzelner Fluss von über 100 s
  beide Regeln (NRC-Regel: 97 s, vis1).
- Warum der Stand vis1 trotzdem weit darüber lag (t3_byss 64–78 s, duel_mos_espa 821–893 s):
  1. **Weiches Warten.** Ein Fluss wartete auf jeden laufenden früheren Fluss, dessen bisher Gefundenes die Abfrage
     nicht deckte – auch wenn dessen Endergebnis für die Abfrage `portalflood` war (er also nur die Bits noch finden
     musste). t3_byss: 8426 solche Wartefälle (6586 Thread-s) gegen 5372 echte (1925 Thread-s).
  2. **Fenster.** Jeder der 4 × numthreads Threads nahm sich zuerst einen Fluss und wartete dann auf einen Zug; die
     meisten hielten Flüsse, die gar nicht anfangen konnten (Zeitleiste: 100–118 von 128 Threads wartend, bis zu 22
     Züge frei).
  3. Ein Lauf mit „Orakel“ (`diag\orakel_patch.py`: er kennt das Endergebnis eines früheren Laufs und wartet nur auf
     echte Abhängigkeiten) ist die Obergrenze für alles ohne Umbau der Flüsse: t3_byss 28,4 s im selben Fenster, in dem
     NRCs Regel 27,4–29,1 s brauchte.

## 3. Deterministischer VIS schneller (vis2b Punkt 2) – umgesetzt, Byte für Byte `-threads 1`

1. **Warten nur, bis die gefragten Bits gefunden sind** (`BhedResolve`, `BhedWaitFound`): der wartende Fluss schläft auf
   dem Zähler des Portals (`std::atomic::wait`, kein Mutex im rechnenden Fluss) und macht mit `portalflood` weiter,
   sobald der laufende Fluss alle Bits der Abfrage gefunden hat (dann ist Endergebnis ∩ Abfrage = flood ∩ Abfrage),
   sonst mit dem Endergebnis. Geweckt wird nicht bei jedem neuen Bit (das weckte einen Wartenden im Mittel 250-mal
   vergeblich: 4–9 Mio. Aufwachen bei 16–32 Tsd. Wartefällen), sondern wenn der Fluss so viele Bits gefunden hat, wie
   dem Wartenden mindestens fehlen (`marks`, `wakeAt`).
   Ein erster Versuch mit Mutex und Bedingungsvariable je 64 Portale war langsamer als vorher (t3_byss 133–149 s):
   der rechnende Fluss hing am Mutex der Aufgeweckten.
2. **Fluss-Pool mit Zügen** (`BhedFlowPool`): ein Thread ohne Zug hält keinen Fluss; ein freier Zug geht zuerst an
   einen Fluss, dessen Warten vorbei ist (kleinster Rang), sonst beginnt ein geparkter oder neuer Thread den nächsten
   Fluss. Höchstens `max(4 × numthreads, 64)` Threads; ein Thread, der tiefer als 48 Rahmen war, endet nach seinem
   Fluss (Stapel und Rahmenpuffer bleiben sonst so groß wie der tiefste Fluss).
   t3_byss im selben Fenster (Fremdlast 9–19 Kerne): alte Schleife mit 128 Threads 50,2 s → Pool 33,3 s (CPU 434 → 333 s);
   512 Threads 33,2 s, 2048 Threads 41,2 s (1,3 GB) – mehr Threads bringen nichts, weil dann nur mehr Flüsse warten.

| Karte | vorher `-bhedvisexact` (a00f6cf90) | jetzt `-bhedvisexact` | heutige Vorgabe (NRC-Regel, alt) | Verhältnis jetzt / Vorgabe | gleich `-threads 1` |
|---|---|---|---|---|---|
| t3_byss, Fenster 11:31 (Fremdlast 9–19 Kerne) | 50,2 s mit „nur bis gefunden“ (ohne: 64–78 s außerhalb) | 33,3 s (CPU 333 s) | 29,1 s (CPU 418 s) | 1,14 | ja |
| t3_byss, Fenster 12:12 (Fremdlast 12–14 Kerne) | – | 39,4 / 41,0 s (CPU 404 / 400 s) | 23,3 / 24,8 s (CPU 407 / 398 s) | 1,67 | ja |
| duel_mos_espa, Fenster 12:32 und 12:52 (Fremdlast 22 und 27 Kerne) | 821,5 s (Vorgänger, CPU 1315 s), 892,9 s (Messfassung) – beide unter leichterer Last, als NRCs Regel 168,9 s brauchte | 583,7 s (CPU 1296 s) | 496,6 s (CPU 1508 s) | 1,18 | ja |
| t3_byss, Fenster 12:52 (Fremdlast 26 Kerne) | 115,1 s (CPU 406 s) | – | – | | ja |
| md_ch_ben, ctf3 | 3,4 s / 1,5 s (vis1) | gleich `-threads 1` geprüft, Zeit im Ruhefenster | | | ja |

- **Ziel nicht erreicht**: unter Last braucht der deterministische VIS auf t3_byss das 1,1- bis 1,7-fache der Vorgabe,
  auf duel_mos_espa das 1,2-fache (bei gleicher oder weniger CPU-Zeit – er rechnet im Mittel nur mit 8–10 Kernen, weil
  Ketten echter Abhängigkeiten und das restliche weiche Warten bremsen). Je mehr Kerne frei sind, desto größer der
  Abstand: NRCs Regel nutzt sie alle (duel_mos_espa unter leichterer Last 168,9 s), der deterministische VIS nicht. Nach vis2b Punkt 3 bleibt damit die heutige Vorgabe; `-bhedvisexact` bleibt der Schalter
  „Same VIS in every build“.
- Speicher: Spitze t3_byss 233–249 MB gegen 154 MB mit NRCs Regel (128 Threads mit eigenem Stapel).

## 4. Geprüft und verworfen (mit Zahlen)

- **SIMD-Trennebenensuche** (Vorgänger, SSE2/AVX2, bytegleich): langsamer als NRCs Schleife und wieder **entfernt**.
  md_ch_ben `-threads 1`, CPU: Schleife 66,6 s, SSE2 69,3 s, AVX2 73,5 s (Fenster 12:12, Fremdlast 23–25 Kerne);
  Vorgänger 57,5 / 62,8 / 63,6 s; t3_byss NRC-Regel 32 Threads CPU 440 s (Schleife) gegen 479 s (AVX2). NRC findet
  meist schon mit dem ersten Punkt eine gültige Ebene; die SIMD-Fassung rechnete alle Punkte samt Division.
- **Mehr wartende Flüsse zulassen** (512, 2048 Threads): kein Gewinn, siehe oben.
- **Immer spekulieren** („weich“ annehmen, bei Widerlegung den Fluss von vorn; Simulation `dep_spek.py` auf den
  gemessenen Abhängigkeiten): t3_byss 29,0 s bei 911 CPU-s und 8759 Neustarts (Warten mit Orakel: 16,6 s),
  duel_mos_espa 214 s bei 4122 CPU-s. Nicht besser als Warten, doppelte bis dreifache CPU-Zeit. Nicht gebaut.
- **Was die Lücke schlösse** (nicht gebaut): Spekulation mit Rücksprung auf den Rahmen der Annahme statt Neustart
  (Rahmen in einem Feld je Tiefe, Marken in einem Protokoll, eigene Sicht bis zur Bestätigung) – Obergrenze ist der
  Orakel-Lauf, also etwa die Zeit der NRC-Regel. Einen einzelnen Fluss auf mehrere Threads zu teilen (die 100-s-Flüsse
  von duel_mos_espa) scheitert weiter am Trennebenen-Zwischenspeicher (vis1, Abschnitt 3).

## 5. Zwischenstand, Anhalten, Fortsetzen (vis2 Punkt 3, q3map2-Seite)

- `bhed_q3map2 -vis -bhedvisstate [-bhedvisstatetime s]` (vor argsToInject genommen, der Entity-Lump bleibt gleich):
  ein Schreibfaden sichert alle `s` Sekunden (Vorgabe 120) `portalvis` aller fertigen Flüsse nach `<karte>.bhedvisstate`
  (Kopf `BHVST1`, Schlüssel = VIS-Schalter + farplane + Regel, ganze .prt, je Fluss Portalnummer + Wörter; direkt in
  eine `.tmp` daneben, dann umbenannt; höchstens 5 % der Zeit wird geschrieben).
- Eine Datei `<karte>.bhedvisstop` hält an: keine neuen Flüsse, laufende und wartende enden ohne Ergebnis, der Stand
  wird geschrieben, `-vis` endet mit **Rückgabe 3**, die .bsp ist unverändert, die .prt bleibt, die Stoppdatei ist weg.
  Liegt sie schon vor dem Fluss da, wird kein Fluss gerechnet. Kommt sie nach dem letzten Fluss, endet `-vis` normal
  (Rückgabe 0) und räumt sie weg.
- Der nächste Lauf mit derselben .prt und denselben Schaltern meldet „BehavEd: vis resumed from …: n of m portals done
  (x % of the work)“ und rechnet den Rest; sonst „vis state not used (no state | state damaged | other vis switches |
  other portals)“. Nach fertigem VIS ist der Stand gelöscht.
- **Mit `-bhedvisexact` Byte für Byte der VIS ohne Pause**, mit jeder Thread-Zahl beim Fortsetzen.
  **Mit NRCs Regel** (heutige Vorgabe) gibt es kein „gleich“ – schon zwei Läufe ohne Pause unterscheiden sich; die
  übernommenen Flüsse sind fertige Flüsse dieser Regel, der Rest wird nach ihr gerechnet. Die Regel gehört zum
  Schlüssel (ein Stand der einen wird von der anderen nicht genommen).
- Fehler des Vorgängers, rot im neuen ctest, dann grün: `-bhedvisstatetime` nahm das **erste übrige** Argument als Zeit
  (`-vis -nopassage -bhedvisstate -bhedvisstatetime 1` rechnete ohne `-nopassage`, `-saveprt` ging verloren);
  jetzt `takeNext()`.
- Geändert gegenüber dem Vorgänger: Zwischenstand auch ohne `-bhedvisexact`; die Stoppdatei wird beim Start nicht mehr
  gelöscht (ein Anhalten, während q3map2 noch lädt, ging sonst verloren – der Aufrufer räumt alte Stoppdateien weg);
  der Stand wird nicht mehr als Abbild im Speicher aufgebaut.
- Licht (nicht angefasst, LCB2b): der Licht-Cache dient beim Fortsetzen als Zwischenstand (`-bhedlightcache`); ob er
  mitten im Licht-Schritt schon Teile sichert, ist nicht geprüft.

## 6. Tests

- ctest **q3map2_vispause** (neu, `tools/karte/q3map2_vispause.py`, Gitter 14 × 14, 34 Prüfungen): Anhalten mitten im
  Fluss und Fortsetzen mit anderer Thread-Zahl für `-vis`, `-vis -nopassage`, `-vis -passageOnly` und
  `-vis -saveprt -hint`; zweimal anhalten; Stoppdatei vor dem Start; beschädigter Stand; Stand anderer Schalter,
  anderer Portale, der anderen Regel; NRCs Regel anhalten/fortsetzen (Clusterzahl gleich, Paare höchstens 2 % neben dem
  Bezug). Das Anhalten „mitten im Fluss“ löst `BHED_VIS_STOPAFTER=<n>` aus (nach n fertigen Flüssen, wie eine Stoppdatei
  in diesem Augenblick) – die Uhr entscheidet nichts. Rot: 5 FEHLER mit dem Stand des Vorgängers (Argumentfehler), grün.
- ctest q3map2_visgleich (8, 32, 100 Threads = `-threads 1`, Visibility-Lump = NRCs q3map2.exe), q3map2_determinismus,
  karte_bau: grün mit dem neuen Pool. karte_grenzen: Prüfstelle `vis.cpp:745` (war 742) nachgezogen, `grenzen.json` neu.
- Bytevergleich aller Lumps gegen `alt -threads 1`: ctf3 (2328ad26…/dbcca994…), md_ch_ben (bfdfd32b…/b8db8bea…),
  t3_byss (2c91eaa0…/63f23aed…) mit 32 Threads und `-bhedvisexact`, jeder Zwischenstand der Arbeit (über 25 Läufe);
  duel_mos_espa (39a0dcd7…/a4e5ceb1…, `alt -threads 1`: 1433 CPU-s, 75 min Wand unter Last) mit 32 Threads und `-bhedvisexact`.
- Große Karte von Hand (`pause2.py`, t3_byss, 32 Threads): deterministisch nach 20 s angehalten (10 174 von 14 848 Flüssen
  fertig, 16,1 % der Arbeit), fortgesetzt: gleich `-threads 1`. NRCs Regel nach 8 s angehalten (12 281 Flüsse, 38,2 %),
  fortgesetzt: +543 Paare (+0,042 %), 104 nur im Bezug, 647 nur im Lauf – in der Streuung der Läufe ohne Pause.
  Vom Stoppen bis zum Ende 0,3 bis 8 s (unter Last; solange der Schreibfaden gerade sichert, sieht er die Stoppdatei nicht).
  Nach dem Anhalten zählt der Pacifier die abgebrochenen Flüsse nicht mehr mit (lief vorher bis 9...).

## 7. Offen

- **Editor-Seite ganz** (vis2 Punkte 3 und 4): [Anhalten] im Baufortschritt (im VIS-Schritt Stoppdatei und auf Rückgabe 3
  warten – `bauSchritt()` läuft heute nach jeder Rückgabe weiter –, sonst Prozess beenden), Kopie der .map und der
  Zwischenstände, Build > Fortsetzen mit „aktuelle Karte / alte Karte“, Anzeige „aus Zwischenstand fortgesetzt“ und
  Restzeit (die Zeile „vis resumed … x % of the work“ auswerten; der Pacifier beginnt dann bei x %), 6 Sprachen,
  Handbuch mit Bildern, Selbsttest, ctest mit Anhalten mitten im Licht. Entwurf und Fundstellen: `UEBERGABE.md`.
- Endgültige Messtabelle im Ruhefenster: `sh /y/werkzeuge/vis2/messung_final.sh` (etwa 25 min) → `messung_final.txt`.
- Vorgabe: bleibt NRCs Regel. Soll der deterministische VIS Vorgabe werden, entscheidet shank mit der Tabelle aus dem
  Ruhefenster (Preis: auf offenen Karten etwa das 1,1- bis 1,7-fache der VIS-Zeit; Gewinn: gleiche .bsp bei jedem Bau,
  Fortsetzen Byte für Byte).
- Zwischenstand sehr großer Karten: je fertigem Fluss ein voller Vektor (md_rev_TarisUpper2: bis 2,9 GB je Sicherung);
  ein dünnes Format (nur Wörter ≠ 0) fehlt.
- md_fh1_jedi im Kartensatz ungeprüft; kejim_post, taspir1, mb2_manaan mit dem neuen Pool nicht gelaufen.

## Changelog

## English

- Map compiler, "Same VIS in every build" (`-bhedvisexact`): 1.4 to 2 times as fast on open maps. A portal now waits for an earlier one only until that one has found what is asked for, and waiting portals no longer keep worker threads from starting others. The result is unchanged - byte for byte what one thread computes.
- Map compiler: a full VIS can be paused and continued. `bhed_q3map2 -vis -bhedvisstate` saves the finished portals every two minutes; a file `<map>.bhedvisstop` pauses the run (return code 3, the .bsp stays untouched), and the next run with the same portal file and switches continues where it stopped. With "Same VIS in every build" the result equals an uninterrupted VIS byte for byte.

## Deutsch

- Karten-Compiler, „Dasselbe VIS in jedem Bau“ (`-bhedvisexact`): auf offenen Karten 1,4- bis 2-mal so schnell. Ein Portal wartet auf ein früheres nur noch, bis dieses das Gefragte gefunden hat, und wartende Portale halten keine Threads mehr davon ab, andere zu beginnen. Das Ergebnis ist unverändert – Byte für Byte das eines Threads.
- Karten-Compiler: ein volles VIS lässt sich anhalten und fortsetzen. `bhed_q3map2 -vis -bhedvisstate` sichert alle zwei Minuten die fertigen Portale; eine Datei `<karte>.bhedvisstop` hält den Lauf an (Rückgabe 3, die .bsp bleibt unberührt), und der nächste Lauf mit derselben Portaldatei und denselben Schaltern macht dort weiter. Mit „Dasselbe VIS in jedem Bau“ ist das Ergebnis Byte für Byte das eines VIS ohne Pause.

## 中文

- 地图编译器,“每次编译得到相同的 VIS”(`-bhedvisexact`):在开阔的地图上快 1.4 到 2 倍。一个门户只需等到前面的门户找到所需的内容为止,等待中的门户也不再占着线程不让其他门户开始。结果不变——与单线程逐字节相同。
- 地图编译器:完整的 VIS 可以暂停并继续。`bhed_q3map2 -vis -bhedvisstate` 每两分钟保存已完成的门户;文件 `<地图>.bhedvisstop` 会暂停运行(返回码 3,.bsp 保持不变),下一次使用相同门户文件和相同开关的运行会从停下的地方继续。打开“每次编译得到相同的 VIS”时,结果与不间断的 VIS 逐字节相同。

## 日本語

- マップコンパイラー、「毎回のビルドで同じ VIS」(`-bhedvisexact`): 開けたマップで 1.4〜2 倍速くなりました。ポータルは、前のポータルが必要な分を見つけるまでだけ待ち、待っているポータルがほかのポータルの開始を妨げることもなくなりました。結果は変わりません。1 スレッドの結果とバイト単位で同じです。
- マップコンパイラー: 完全な VIS を一時停止して再開できます。`bhed_q3map2 -vis -bhedvisstate` は完了したポータルを 2 分ごとに保存します。ファイル `<マップ>.bhedvisstop` で実行が一時停止し(戻り値 3、.bsp はそのまま)、同じポータルファイルと同じスイッチの次の実行が続きから計算します。「毎回のビルドで同じ VIS」では、結果は中断なしの VIS とバイト単位で同じです。

## Español

- Compilador de mapas, «VIS igual en cada compilación» (`-bhedvisexact`): entre 1,4 y 2 veces más rápido en mapas abiertos. Un portal solo espera a uno anterior hasta que este ha encontrado lo que se le pide, y los portales en espera ya no impiden que los hilos empiecen otros. El resultado no cambia: byte a byte el de un solo hilo.
- Compilador de mapas: un VIS completo se puede pausar y continuar. `bhed_q3map2 -vis -bhedvisstate` guarda cada dos minutos los portales terminados; un archivo `<mapa>.bhedvisstop` pausa la ejecución (código de retorno 3, el .bsp queda intacto) y la siguiente ejecución con el mismo archivo de portales y los mismos parámetros continúa donde se detuvo. Con «VIS igual en cada compilación», el resultado es byte a byte el de un VIS sin pausa.

## Čeština

- Kompilátor map, „Stejné VIS při každém sestavení“ (`-bhedvisexact`): na otevřených mapách 1,4krát až 2krát rychlejší. Portál čeká na dřívější jen do chvíle, kdy ten našel, na co se ptá, a čekající portály už nebrání vláknům začít jiné. Výsledek se nemění – bajt po bajtu to, co spočítá jedno vlákno.
- Kompilátor map: plné VIS lze pozastavit a pokračovat v něm. `bhed_q3map2 -vis -bhedvisstate` ukládá každé dvě minuty hotové portály; soubor `<mapa>.bhedvisstop` běh pozastaví (návratový kód 3, .bsp zůstane nedotčený) a další běh se stejným souborem portálů a stejnými přepínači pokračuje tam, kde skončil. Se „Stejné VIS při každém sestavení“ je výsledek bajt po bajtu stejný jako VIS bez pauzy.
