# Notizen vis2 / vis2b / vis2c (10./11.11.): VIS – deterministisch schneller, Bauen anhalten und fortsetzen (q3map2 und Karten-Editor)

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
anhalten und fortsetzen (q3map2-Seite fertig und im ctest); **der Karten-Editor hält einen Bau an und setzt ihn fort** (vis2c, Abschnitt 8): [Bauen anhalten] im Baufortschritt, Build > Bauen fortsetzen... mit „Aktuelle Karte“ / „Karte wie angehalten“, und ein beendeter Licht-Schritt behält die fertigen Durchgänge seines Licht-Caches.

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
- Licht (nicht angefasst, LCB2b): der Licht-Cache dient beim Fortsetzen als Zwischenstand (`-bhedlightcache`).
  Geprüft am 10.11. (vis2c, `Y:\werkzeuge\vis2\skripte\lichtabbruch.py`, zweiraum.map, `-light -super 4 -samples 8 -threads 2
  -bounce 4 -bhedlightcache`, Prozess beendet, sobald Durchgang 2 von 5 geschrieben gemeldet ist): **mitten im Licht-Schritt
  beendet bleibt heute nichts erhalten.** Es liegt nur `zweiraum.bhedlight.neu` da (19,9 MB, 2 Abschnitte begonnen, 1
  vollständig – das Ende des zweiten hängt noch im Schreibpuffer), kein `zweiraum.bhedlight`; der nächste Lauf meldet „light
  cache not usable: no cache yet - everything is traced“ (0 von 90 Lightmaps aus dem Cache) und überschreibt die `.neu`.
  Innerhalb eines Durchgangs sichert der Cache nichts (ein Abschnitt je Durchgang, am Durchgangsende geschrieben). Ohne Eingriff
  in die Licht-Rechnung ließen sich die fertigen Durchgänge retten: `fflush` nach jedem Abschnitt (eine Zeile in
  `bhed_lichtcache.cpp`) und der Editor benennt beim Anhalten die `.neu` zum Cache des angehaltenen Baus um (der Leser nimmt
  Caches mit weniger Durchgängen und verwirft einen abgeschnittenen letzten Abschnitt selbst) – gebaut in vis2c, Abschnitt 8.

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

- Editor-Seite: erledigt in vis2c (Abschnitt 8). Dort offen: der Satz „duel_mos_espa: 5 instead of 1.5 minutes“ im Tooltip von
  „Same VIS in every build“ und im Handbuch (bewusst nicht angefasst – nach der Messrunde im Ruhefenster); Licht-Fortsetzen
  innerhalb eines Durchgangs und das Zusammenführen von altem und gerettetem Cache (Schnittstelle für LCB2, Abschnitt 8).
- Endgültige Messtabelle im Ruhefenster: `sh /y/werkzeuge/vis2/messung_final.sh` (etwa 25 min) → `messung_final.txt`.
- Vorgabe: bleibt NRCs Regel. Soll der deterministische VIS Vorgabe werden, entscheidet shank mit der Tabelle aus dem
  Ruhefenster (Preis: auf offenen Karten etwa das 1,1- bis 1,7-fache der VIS-Zeit; Gewinn: gleiche .bsp bei jedem Bau,
  Fortsetzen Byte für Byte).
- Zwischenstand sehr großer Karten: je fertigem Fluss ein voller Vektor (md_rev_TarisUpper2: bis 2,9 GB je Sicherung);
  ein dünnes Format (nur Wörter ≠ 0) fehlt.
- md_fh1_jedi im Kartensatz ungeprüft; kejim_post, taspir1, mb2_manaan mit dem neuen Pool nicht gelaufen.

## 8. Editor-Seite: Bauen anhalten und fortsetzen (vis2c, 11.11.)

Auftrag `auftrag_vis2c_1011.md`, Abschnitt VIS2c. An `vis.cpp`/`visflow.*`/`threads.cpp` ist nichts geändert (dort arbeitet VIS2s).

### Was der Editor jetzt tut

- **[Bauen anhalten]** im Fenster Baufortschritt (neben [Bauen abbrechen]), per Rechtsklick in diesem Fenster und als
  Build > Bauen anhalten; **Build > Bauen fortsetzen...** öffnet das Fenster „Bauen fortsetzen“ (Eintrag, Schritt, Datum,
  fertige Portale bzw. Licht-Durchgänge, Größe und Ort des Zwischenstands, „Karte geändert?“, ehrlicher Satz zur VIS-Regel) mit
  [Aktuelle Karte], [Karte wie angehalten], [Zwischenstand verwerfen]. Jeder Knopf und Menüpunkt hat einen Tooltip. Der
  Karten-Editor hat keine Aktionssuche (die gibt es im Model-Tab) – auffindbar über Menü, Fenster, Rechtsklick und das Handbuch.
- Jedes `-vis` von bhed_q3map2 läuft aus dem Build-Menü mit `-bhedvisstate` (`befehlMitVisStand`; ein fremdes q3map2 bekommt
  den Schalter nicht). Anhalten im VIS = Stoppdatei `<karte>.bhedvisstop`, q3map2 sichert und endet mit Rückgabe 3; war das
  VIS schon fertig (`-fast`, Treffer von `-bhedvisreuse`, letzter Fluss durch), wird nach dem Schritt angehalten. Jeder andere
  Schritt wird beendet und beginnt beim Fortsetzen neu.
- Zwischenstand = Ordner `<karte>.bhedpause` neben der gebauten Karte (nie im Spielordner – dort baut der Editor ohnehin
  nicht): Kopie der Karte (.map bzw. .reg), .bsp, .prt, .srf, der VIS-Zwischenstand (verschoben, nicht kopiert: bis 2,9 GB,
  und ein `-vis -fast` dazwischen löschte ihn sonst), der Licht-Cache (fester Link, sonst Kopie) und `stand.json` (Eintrag,
  alle Befehle, Schritt, Zeiten, Hash der Karte). Ein Zwischenstand je Karte; erneutes Anhalten ersetzt ihn, ein normaler
  Bau lässt ihn liegen und sagt es in der Konsole.
- **„Karte wie angehalten“**: die übrigen Befehle laufen auf den Kopien im Ordner (die Karte des Anwenders wird nicht
  überschrieben), `stand.json` geht nach jedem fertigen Schritt mit, danach kommen .bsp, .prt, .srf, Licht-Cache, Editor-Daten
  und der Ordner externer Lightmaps neben die Karte. **„Aktuelle Karte“**: der Eintrag wird für die Karte von jetzt neu
  gebaut; vorher kommt der VIS-Zwischenstand zurück neben die .bsp – q3map2 nimmt ihn nur bei gleicher .prt und gleichen
  Schaltern („vis state not used (other portals)“ sonst), der Licht-Cache prüft jede Lightmap. Ergebnis wie ein Vollbau.
- **Licht**: vor jedem Licht-Schritt sichert der Editor die .bsp (`<karte>.bhedprelight`; ein beendetes `-light` kann sie halb
  geschrieben hinterlassen) – nach Abbrechen/Anhalten ist die .bsp wieder die von vor dem Schritt. Aus `<karte>.bhedlight.neu`
  wird nach der Regel `bauLichtCacheRetten` der Cache: ohne brauchbaren alten Cache ab einem fertigen Durchgang; passt der
  alte genau, nur wenn mehr Durchgänge fertig sind, als er hat; wird der alte über den Inhalt zugeordnet (Geometrie geändert),
  gewinnt der mit dem kleineren Rest (alt: Summe der gemeldeten Anteile je Durchgang; gerettet: die unfertigen Durchgänge
  ganz). Das gilt für Anhalten, Abbrechen und das Programmende. Verloren geht der Durchgang, der gerade lief; ohne
  „Teil-Kompilierung: Licht wiederverwenden“ der ganze Licht-Schritt (die Konsole sagt es).
- **Anzeige** im Baufortschritt, VIS-Schritt: „aus dem Zwischenstand fortgesetzt: n von m Portalen (x % der Arbeit)“ bzw.
  „Zwischenstand nicht genutzt: Grund“, Größe des Zwischenstands auf der Platte (einmal je Sekunde gelesen), die Regel
  (deterministisch: Byte für Byte / NRC: gültiger Lauf), „Wird angehalten...“. Vor dem VIS: Höchstgröße des Zwischenstands
  aus der .prt (`visStandHoechstBytes`, beim Sichern kurz doppelt) gegen den freien Platz – Warnung in Konsole und Fenster.
  Restzeit nach dem Fortsetzen: der Pacifier beginnt bei x %, gezählt wird nur der Rest (erster Bau und mit Verlauf).
  Die Zeiten eines fortgesetzten Baus werden nicht als Verlauf und nicht für GPU-Auto gemerkt.
- **Zwei Funde am Beginn eines Schritts** (beide im Selbsttest rot gesehen, NRC prüft dort nur die Zeit): (1) nach einem langen
  Bild (16 s: große Karte gespeichert, erstes Bild) endete der Bau mit „timeout“, obwohl q3map2 längst am lauschenden Socket
  wartete – die wartende Verbindung wird jetzt vor der Zeitgrenze angenommen; (2) unter Last brauchte bhed_q3map2 (im Selbsttest
  mit Leerlauf-Priorität) über 5 s bis zum Verbinden – „timeout“, und q3map2 lief ohne Rückkanal verwaist weiter. Jetzt gilt die
  Zeitgrenze nur, wenn der Prozess schon zu Ende ist; ein q3map2 ohne Rückkanal wird bis zu seinem Ende abgewartet (Abbrechen
  geht), dann kommt dieselbe Meldung.
- Der Editor bleibt während des Baus bedienbar (war schon so: der Bau ist eine Zustandsmaschine je Bild) – jetzt im
  Selbsttest belegt (Duplicate und Undo während des Licht-Schritts, Bilder werden weiter gezeichnet).

### Zwei Anzeigefehler, rot nachgestellt und behoben (ctest karte_bau: 4 FEHL → 0, 119 Prüfungen)

1. Mit `-bhedvisstate` steht nach dem Kopf der Flussphase die Zeile „BehavEd: vis state not used ...“ bzw. „vis resumed ...“;
   der Beobachter erwartete danach keinen Pacifier mehr (Pacifier −1: der VIS-Balken wanderte nur).
2. Beim Anhalten schreibt q3map2 mitten im Pacifier „\nBehavEd: vis paused ...“; jedes Ende des Pacifiers galt als fertig
   (Sprung auf 100 %). Jetzt beendet nur „ (Sekunden)“ die Phase.

### Licht-Cache: rot und grün (ctest q3map2_baupause)

- Rot mit dem q3map2 von 5433c92eb: LIGHT beendet, sobald „pass 2 of 5 ... vertex lit surfaces“ gemeldet ist; aus der `.neu`
  den Cache gemacht – der nächste Licht-Schritt meldete „2 of 5 passes kept“, nahm aber in Durchgang 2 **0 von 2447**
  Lightmaps (der zweite Abschnitt hing noch im Schreibpuffer; die Lumps waren trotzdem gleich dem Bezug – der Leser verwirft
  den abgeschnittenen Abschnitt).
- Behoben mit **zwei Zeilen** in `src/q3map2/bhed_lichtcache.cpp` (BhedLichtCacheDurchgangEnde, gehört LCB2):
  `if ( g_neu != nullptr ) fflush( g_neu );` vor der Zeile „... vertex lit surfaces ...“ – eingefügt nach der schließenden
  Klammer des Schreib-Blocks, eine unveränderte Zeile neben den auf karte1 geänderten Zeilen (lässt sich ohne Konflikt
  zusammenführen). Die Licht-Rechnung ist nicht berührt.
- Grün: Durchgang 1 und 2 ganz aus dem Cache, die übrigen gerechnet, alle Lumps gleich dem Bau ohne Abbruch – für
  „Karte wie angehalten“ und für Abbrechen + noch einmal bauen.

### Schnittstellen für LCB2 (Licht-Cache, Baufortschritt)

- Der Editor liest das Format des Caches nicht; er zählt die Zeilen „light cache pass x of y: ... vertex lit surfaces ...“
  (= Abschnitt x steht auf der Platte), liest „found (geometry and switches unchanged, k of n passes kept)“ und die Anteile
  aus „pass x of y: a of b lightmaps from the cache (p % ...“ und benennt `<karte>.bhedlight.neu` → `<karte>.bhedlight` um.
  Bleiben diese drei Zeilen und der Name `.neu`, bleibt das Retten heil; der Text hinter „(p %“ ist frei.
- `BauLichtCache` hat drei neue Felder (`geschrieben`, `geoGleich`, `anteile`) am Ende; karte1 hat dort `nachInhalt` – beim
  Zusammenführen beide behalten (`geoGleich` = nicht `nachInhalt` bei gefundenem Cache).
- Im Fenster Baufortschritt sind nur der VIS-Block (vor dem Licht-Cache-Block) und die Knopfzeile am Ende neu.
- Vorschlag (nicht gebaut, wäre LCB2s Datei): beim Öffnen des alten Caches eine liegen gebliebene `.neu` mit gleichem Kopf
  (Schalter, worldspawn, Geometrie) mitlesen und ihre Durchgänge vorziehen – dann blieben nach Geometrie-Änderungen die
  fertigen Durchgänge UND die Inhalts-Treffer der späteren, und die Abwägung im Editor entfiele.

### Was der VIS-Zwischenstand über die Reihenfolge der Flüsse annimmt (für VIS3, `-bhedvisfixed`)

Gelesen in `visflow.cpp` (BhedStateWrite, BhedStateRead, BhedRunFlow) und `vis.cpp` (BhedVisStateSetup) – Stand vis2b, hier nicht geändert:

- Die Datei `<karte>.bhedvisstate` („BHVST1“) enthält den Schlüssel (VIS-Schalter fast/merge/mergeportals/nopassage/passageOnly/
  nosort/hint, farplane, die Regel `exact=`), die ganze .prt, die Wortzahl und je **fertigem** Fluss die Nummer der Portalseite in
  `portals[]` (Reihenfolge der .prt, zwei Seiten je Portal) mit ihrem `portalvis`. **Die Reihenfolge der Flüsse steht nicht darin**,
  auch nicht, welche gerade liefen. Beim Fortsetzen entsteht die Sortierung (`sorted_portals`, `g_bhedRank`) neu aus derselben .prt
  und denselben Schaltern; die übernommenen Flüsse werden über ihre Portalnummer als fertig markiert.
- Angenommen wird also: (1) ein Fluss mit Status „fertig“ hat sein **endgültiges** `portalvis` (es ändert sich später nicht mehr);
  (2) BasePortalVis und die Sortierung hängen nur von .prt und Schaltern ab; (3) für „Byte für Byte“: das Ergebnis jedes noch offenen
  Flusses hängt nur von den Endergebnissen einer durch die Regel festgelegten Menge anderer Flüsse ab, nicht davon, was zeitlich
  zuerst fertig war. Mit `-bhedvisexact` gilt (3) durch die Regel selbst (jeder Fluss liest `portalvis` genau der Flüsse vor ihm in
  der Sortierung und `portalflood` aller danach); mit NRCs Regel gilt (3) nicht – dort ist das Fortsetzen ein gültiger Lauf der Regel.
- Eine neue feste Regel (`-bhedvisfixed`) funktioniert damit unverändert, wenn (a) ihr Schalter in den Schlüssel kommt (sonst nimmt
  eine Regel den Stand der anderen), (b) (1) und (3) für sie gelten – sie darf die Flüsse in jeder Reihenfolge rechnen, solange sie
  übernommene Flüsse als fertig behandelt und auf die nicht übernommenen wartet, von denen ein Fluss abhängt –, und (c) das Anhalten
  bleibt, wie es ist: keine neuen Flüsse, laufende und wartende enden ohne Ergebnis, Stand schreiben, Rückgabe 3.
- Der Editor ist von der Regel unabhängig. Er braucht: den Schalter `-bhedvisstate`, die Stoppdatei `<karte>.bhedvisstop`, die
  Rückgabe 3 bzw. die Zeile „BehavEd: vis paused, n of m portals done ...“ und die Zeilen „vis resumed from ...: n of m portals done
  (x % of the work)“ / „vis state not used (Grund): ...“. Den Satz zum Ergebnis („Byte für Byte“ gegen „gültiger Lauf“) wählt er
  heute nach `-bhedvisexact` im Befehl (Fenster „Bauen fortsetzen“) bzw. nach der Zeile „BehavEd: deterministic vis (the result of
  -threads 1)“ (Baufortschritt). Bekommt der Editor einen Schalter für `-bhedvisfixed`, braucht diese Regel dort einen eigenen Satz
  (oder dieselbe Zeile, wenn sie Byte für Byte reproduzierbar ist) – zwei Stellen in `gui/app_karte_bau.cpp` (KeBpRegelExakt / KeBpRegelNrc).

### Tests

- ctest **karte_bau** (tests/kartebau.cpp, +15 Prüfungen): Zeilen des VIS-Zwischenstands, Pacifier nach „not used“/„resumed“
  und beim Anhalten, Restzeit nach dem Fortsetzen (erster Bau 23,75 s; mit Verlauf 67,5 → 47,5 s), Regel fürs Retten des
  Licht-Caches (drei Fälle), `befehlMitVisStand`, Höchstgröße aus der .prt, `stand.json` hin und zurück, alle Dateischritte
  in einem Temp-Ordner (Anhalten, alte Karte, aktuelle Karte, .bsp vor dem Licht, Cache retten, Verwerfen).
- ctest **q3map2_baupause** (neu, tools/karte/q3map2_baupause.py + tests/kartepause.cpp, 19 Prüfungen, Gitter 7 × 7 mit
  Lichtern): Bezug ohne Pause; VIS mitten im Fluss angehalten → „Karte wie angehalten“ / „aktuelle Karte“ nach einem
  `-vis -fast` dazwischen / „aktuelle Karte“ mit geänderter Karte („other portals“, gleich dem Vollbau der neuen Karte);
  LIGHT nach zwei fertigen Durchgängen beendet → „Karte wie angehalten“ und Abbrechen + neu bauen; NRCs Regel (10848 gegen
  10829 Paare, Grenze 2 %). Alle Lumps gleich dem Bezug.
- Selbsttest **vis2c** (gui/selbsttest_vis2c.inc, drei Programmstarts mit einem Laufordner, `sh /y/werkzeuge/vis2/skripte/vis2c.sh`):
  Lauf 1 7/0 (ein Bauprozess, der erst nach 7 s verbindet, und ein Bild von 6 s nach dem Baustart enden nicht mit „timeout“; Bezug; im VIS
  mit dem Knopf angehalten – Tooltip, beide Knöpfe ganz zu sehen, fertige und offene Portale, z. B. 1328 von 1876), Lauf 2 8/0
  (Zwischenstand nach dem Neustart da; Menü, Tooltips, Fenster mit drei Knöpfen; „Karte wie angehalten“: 18 von 18 Lumps gleich
  dem Bezug; im Licht per Rechtsklick angehalten: 2 von 5 Durchgängen gerettet), Lauf 3 6/0 („Aktuelle Karte“: 18 von 18 Lumps
  gleich; während des Baus Duplicate und Undo, der Editor zeichnet weiter; ohne Build-Überwachung im VIS angehalten: 1060 von 1876
  Portalen; Verwerfen). Zusammen 21/0. Testkarte: offene Halle 9 × 9 Felder mit niedrigen Blöcken (ein Gitter aus Räumen mit
  Türen war für einen Klick zu schnell: VIS unter 1 s).
- Schlussrunde: nach der Regel „Schlussrunde je Zweig ab 11.11.“ – Bau inkrementell ohne Warnungen (/W4 /WX); ctest 245 von 248: rot
  q3map2_importe (seit vis2b: `std::atomic::wait` importiert api-ms-win-core-synch-l1-2-0.dll; die Erlaubnisliste ist im Zweig p_vis2s
  schon ergänzt – hier nicht doppelt), q3map2_teilbau (zwei lcb2-Fälle der Basis a00f6cf90, wie in den anderen Zweigen) und
  karte_lizenz (die zwei neuen Dateien nannten ihre Herkunft nicht – behoben, kartelizenz 232/0); Selbsttest vis2c 21/0; berührte
  Gruppen: stufe3 223/0 (Build-Menü, Bauablauf), stufe9d 28/0 (Same VIS in every build), stufe8 41/5 – alle fünf in der
  Rückgewinnung nach Open .bsp („nicht nach 180 s“, in zwei Läufen gleich; nicht in geändertem Code, Ursache nicht untersucht), die
  Bau-Prüfungen der Gruppe (Fortschritt, Abbrechen, Teil-Kompilierung) grün. Drei vorhandene Erwartungen folgen dem neuen Schalter
  (`-vis ... -bhedvisstate "<Karte>"` in stufe3, stufe8, stufe9d). Lints: portability 99 (wie Basis; ein eigener Fund – `std::size_t`
  ohne `<cstddef>` – behoben), fremdprogramm 1 und pruefcodes 11 wie Basis, die übrigen ohne Beanstandung. Handbuchbilder in sechs
  Sprachen angesehen (2538 breit). Bewusst NICHT gelaufen (einmal auf dem zusammengeführten Stand): sauberer Neubau, Modus 1,
  karteneditor ganz, die übrigen Gruppen.

## Changelog

## English

- Map compiler, "Same VIS in every build" (`-bhedvisexact`): 1.4 to 2 times as fast on open maps. A portal now waits for an earlier one only until that one has found what is asked for, and waiting portals no longer keep worker threads from starting others. The result is unchanged - byte for byte what one thread computes.
- Map compiler: a full VIS can be paused and continued. `bhed_q3map2 -vis -bhedvisstate` saves the finished portals every two minutes; a file `<map>.bhedvisstop` pauses the run (return code 3, the .bsp stays untouched), and the next run with the same portal file and switches continues where it stopped. With "Same VIS in every build" the result equals an uninterrupted VIS byte for byte.
- Map editor: a build can be paused and continued later. **Pause build** in the Build progress window (also by right click there and in the Build menu) saves the finished steps, a copy of the map and the state of the running step beside the map; JK Studio can be closed in between. Build > **Resume build...** continues for the current map - reusing only what is still valid, with the result of a full build - or finishes exactly the paused build on the saved copy of the map.
- Map editor: a full VIS that is paused saves its finished portals and ends within seconds; a cancelled VIS continues from its last saved state next time if the portals are the same. The Build progress window shows what was taken over and how large the state on disk is, and warns before the VIS if the drive is too full.
- Map editor: a light step that is paused or cancelled keeps its finished passes in the light cache (with "Partial compile: reuse light"); until now everything was computed again. The .bsp is put back as it was before the light step.
- Map editor: the Build progress window keeps the progress bar of the VIS when the VIS runs with a saved state, and the remaining time after a resume counts only what is left.

## Deutsch

- Karten-Compiler, „Dasselbe VIS in jedem Bau“ (`-bhedvisexact`): auf offenen Karten 1,4- bis 2-mal so schnell. Ein Portal wartet auf ein früheres nur noch, bis dieses das Gefragte gefunden hat, und wartende Portale halten keine Threads mehr davon ab, andere zu beginnen. Das Ergebnis ist unverändert – Byte für Byte das eines Threads.
- Karten-Compiler: ein volles VIS lässt sich anhalten und fortsetzen. `bhed_q3map2 -vis -bhedvisstate` sichert alle zwei Minuten die fertigen Portale; eine Datei `<karte>.bhedvisstop` hält den Lauf an (Rückgabe 3, die .bsp bleibt unberührt), und der nächste Lauf mit derselben Portaldatei und denselben Schaltern macht dort weiter. Mit „Dasselbe VIS in jedem Bau“ ist das Ergebnis Byte für Byte das eines VIS ohne Pause.
- Karten-Editor: ein Bau lässt sich anhalten und später fortsetzen. **Bauen anhalten** im Fenster Baufortschritt (auch per Rechtsklick dort und im Build-Menü) sichert die fertigen Schritte, eine Kopie der Karte und den Stand des laufenden Schritts neben der Karte; JK Studio kann dazwischen geschlossen werden. Build > **Bauen fortsetzen...** macht für die aktuelle Karte weiter - verwendet nur wieder, was noch gilt, mit dem Ergebnis eines vollen Baus - oder führt genau den angehaltenen Bau auf der gesicherten Kopie der Karte zu Ende.
- Karten-Editor: ein volles VIS, das angehalten wird, sichert seine fertigen Portale und endet in Sekunden; ein abgebrochenes VIS macht beim nächsten Mal mit seinem letzten Zwischenstand weiter, wenn die Portale gleich sind. Das Fenster Baufortschritt zeigt, was übernommen wurde und wie groß der Zwischenstand auf der Platte ist, und warnt vor dem VIS, wenn das Laufwerk zu voll ist.
- Karten-Editor: ein Licht-Schritt, der angehalten oder abgebrochen wird, behält seine fertigen Durchgänge im Licht-Cache (mit „Teil-Kompilierung: Licht wiederverwenden“); bisher wurde alles neu gerechnet. Die .bsp ist danach wieder die von vor dem Licht-Schritt.
- Karten-Editor: das Fenster Baufortschritt behält den Balken des VIS, wenn das VIS mit Zwischenstand läuft, und die Restzeit nach dem Fortsetzen zählt nur, was noch fehlt.

## 中文

- 地图编译器,“每次编译得到相同的 VIS”(`-bhedvisexact`):在开阔的地图上快 1.4 到 2 倍。一个门户只需等到前面的门户找到所需的内容为止,等待中的门户也不再占着线程不让其他门户开始。结果不变——与单线程逐字节相同。
- 地图编译器:完整的 VIS 可以暂停并继续。`bhed_q3map2 -vis -bhedvisstate` 每两分钟保存已完成的门户;文件 `<地图>.bhedvisstop` 会暂停运行(返回码 3,.bsp 保持不变),下一次使用相同门户文件和相同开关的运行会从停下的地方继续。打开“每次编译得到相同的 VIS”时,结果与不间断的 VIS 逐字节相同。
- 地图编辑器:编译可以暂停,之后再继续。编译进度窗口中的 **暂停编译**(也可在该窗口中点右键或使用 Build 菜单)会把已完成的步骤、地图的副本以及正在运行的步骤的状态保存在地图旁边;其间可以关闭 JK Studio。Build > **继续编译...** 可以针对当前地图继续——只重用仍然有效的部分,结果与完整编译相同——或者在保存的地图副本上原样完成被暂停的编译。
- 地图编辑器:被暂停的完整 VIS 会保存已完成的门户并在几秒内结束;被取消的 VIS 下次会在门户相同时从最后保存的状态继续。编译进度窗口会显示接手了多少、磁盘上的状态有多大,并在驱动器空间不足时于 VIS 之前给出警告。
- 地图编辑器:被暂停或取消的光照步骤会把已完成的遍保留在光照缓存中(需打开“部分编译:重用光照”);此前一切都要重新计算。之后 .bsp 恢复为光照步骤之前的状态。
- 地图编辑器:VIS 使用保存的状态运行时,编译进度窗口仍会显示 VIS 的进度条;继续之后的剩余时间只计算还没完成的部分。

## 日本語

- マップコンパイラー、「毎回のビルドで同じ VIS」(`-bhedvisexact`): 開けたマップで 1.4〜2 倍速くなりました。ポータルは、前のポータルが必要な分を見つけるまでだけ待ち、待っているポータルがほかのポータルの開始を妨げることもなくなりました。結果は変わりません。1 スレッドの結果とバイト単位で同じです。
- マップコンパイラー: 完全な VIS を一時停止して再開できます。`bhed_q3map2 -vis -bhedvisstate` は完了したポータルを 2 分ごとに保存します。ファイル `<マップ>.bhedvisstop` で実行が一時停止し(戻り値 3、.bsp はそのまま)、同じポータルファイルと同じスイッチの次の実行が続きから計算します。「毎回のビルドで同じ VIS」では、結果は中断なしの VIS とバイト単位で同じです。
- マップエディター: ビルドを一時停止して、あとで再開できます。ビルドの進行状況ウィンドウの **ビルドを一時停止**(同じウィンドウの右クリックや Build メニューからも可)で、完了したステップ、マップのコピー、実行中のステップの状態をマップの隣に保存します。その間に JK Studio を閉じてもかまいません。Build > **ビルドを再開...** は、現在のマップに対して続ける(まだ有効な分だけを再利用し、結果はフルビルドと同じ)か、保存したマップのコピーで一時停止したビルドをそのまま最後まで行います。
- マップエディター: 完全な VIS を一時停止すると、完了したポータルを保存して数秒で終わります。中止した VIS は、ポータルが同じなら次回、最後に保存した状態から続けます。ビルドの進行状況ウィンドウには、引き継いだ内容とディスク上の状態の大きさが表示され、ドライブの空きが足りないときは VIS の前に警告します。
- マップエディター: 一時停止または中止したライトのステップは、終わったパスをライトキャッシュに残します(「部分コンパイル:ライトを再利用」がオンのとき)。これまではすべて計算し直していました。.bsp はライトのステップの前の状態に戻ります。
- マップエディター: VIS が保存した状態を使って動くときも、ビルドの進行状況ウィンドウに VIS の進行バーが表示され、再開後の残り時間は残っている分だけを数えます。

## Español

- Compilador de mapas, «VIS igual en cada compilación» (`-bhedvisexact`): entre 1,4 y 2 veces más rápido en mapas abiertos. Un portal solo espera a uno anterior hasta que este ha encontrado lo que se le pide, y los portales en espera ya no impiden que los hilos empiecen otros. El resultado no cambia: byte a byte el de un solo hilo.
- Compilador de mapas: un VIS completo se puede pausar y continuar. `bhed_q3map2 -vis -bhedvisstate` guarda cada dos minutos los portales terminados; un archivo `<mapa>.bhedvisstop` pausa la ejecución (código de retorno 3, el .bsp queda intacto) y la siguiente ejecución con el mismo archivo de portales y los mismos parámetros continúa donde se detuvo. Con «VIS igual en cada compilación», el resultado es byte a byte el de un VIS sin pausa.
- Editor de mapas: una compilación se puede pausar y continuar más tarde. **Pausar la compilación** en la ventana Progreso de la compilación (también con clic derecho allí y en el menú Build) guarda los pasos terminados, una copia del mapa y el estado del paso en curso junto al mapa; entretanto se puede cerrar JK Studio. Build > **Reanudar la compilación...** continúa para el mapa actual (reutilizando solo lo que sigue siendo válido, con el resultado de una compilación completa) o termina exactamente la compilación pausada sobre la copia guardada del mapa.
- Editor de mapas: un VIS completo que se pausa guarda sus portales terminados y acaba en segundos; un VIS cancelado continúa la próxima vez desde su último estado guardado si los portales son los mismos. La ventana Progreso de la compilación muestra lo que se ha recuperado y cuánto ocupa el estado en disco, y avisa antes del VIS si la unidad está demasiado llena.
- Editor de mapas: un paso de luz que se pausa o se cancela conserva sus pasadas terminadas en la caché de luz (con «Compilación parcial: reutilizar la luz»); hasta ahora se calculaba todo de nuevo. Después, el .bsp vuelve a ser el de antes del paso de luz.
- Editor de mapas: la ventana Progreso de la compilación conserva la barra de progreso del VIS cuando este se ejecuta con un estado guardado, y el tiempo restante tras reanudar solo cuenta lo que falta.

## Čeština

- Kompilátor map, „Stejné VIS při každém sestavení“ (`-bhedvisexact`): na otevřených mapách 1,4krát až 2krát rychlejší. Portál čeká na dřívější jen do chvíle, kdy ten našel, na co se ptá, a čekající portály už nebrání vláknům začít jiné. Výsledek se nemění – bajt po bajtu to, co spočítá jedno vlákno.
- Kompilátor map: plné VIS lze pozastavit a pokračovat v něm. `bhed_q3map2 -vis -bhedvisstate` ukládá každé dvě minuty hotové portály; soubor `<mapa>.bhedvisstop` běh pozastaví (návratový kód 3, .bsp zůstane nedotčený) a další běh se stejným souborem portálů a stejnými přepínači pokračuje tam, kde skončil. Se „Stejné VIS při každém sestavení“ je výsledek bajt po bajtu stejný jako VIS bez pauzy.
- Editor map: sestavení lze pozastavit a později v něm pokračovat. **Pozastavit sestavení** v okně Průběh sestavení (také pravým tlačítkem tamtéž a v nabídce Build) uloží hotové kroky, kopii mapy a stav běžícího kroku vedle mapy; JK Studio lze mezitím zavřít. Build > **Pokračovat v sestavení...** pokračuje pro aktuální mapu - znovu použije jen to, co stále platí, s výsledkem plného sestavení - nebo dokončí přesně pozastavené sestavení na uložené kopii mapy.
- Editor map: pozastavené plné VIS uloží hotové portály a skončí během několika sekund; zrušené VIS příště pokračuje ze svého posledního uloženého stavu, pokud jsou portály stejné. Okno Průběh sestavení ukazuje, co bylo převzato a jak velký je stav na disku, a před VIS varuje, je-li jednotka příliš plná.
- Editor map: krok světla, který je pozastaven nebo zrušen, si ponechá hotové průchody v mezipaměti světla (s volbou „Dílčí kompilace: znovu použít světlo“); dosud se vše počítalo znovu. Soubor .bsp je poté opět ten z doby před krokem světla.
- Editor map: okno Průběh sestavení si ponechá ukazatel průběhu VIS, když VIS běží s uloženým stavem, a zbývající čas po pokračování počítá jen to, co ještě chybí.
