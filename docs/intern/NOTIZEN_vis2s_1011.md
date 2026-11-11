# Notizen vis2s (11.11.): VIS schneller bei gleichem Ergebnis – Profil, Geometrie-Merker, Winding-Schnitt (Runde 1 und 2)

Auftrag `auftrag_vis2c_1011.md`, Abschnitt VIS2s. Worktree `Y:\p_vis2s`, Zweig `p_vis2s` ab fee4525c1, Bau
`Y:/p_vis2s/build` (nur `bhed_q3map2_clang` und `kartegrenzen`). Werkzeuge, Läufe, Messfassungen: `Y:\werkzeuge\vis2s`
(Inhalt in `UEBERGABE.md` dort). Vorher: `NOTIZEN_vis2_1011.md` (Abhängigkeiten der Flüsse, Anhalten/Fortsetzen).

Rechner: i9-14900HX (30 nutzbare Threads), 64 GB. **Alle Zeiten dieser Notiz sind unter Last gemessen (Richtwerte)**:
während der ganzen Arbeit bauten und testeten andere Agenten (Fremdlast je Lauf 21 bis 29 Kerne, im „Messfenster“
liefen 51 fremde Bau-/Testprozesse weiter). Einzelne Läufe streuen deshalb um ±10 % (ctf3 mit einem Thread: 19,2 bis
25,8 CPU-s für dasselbe Programm). Belastbar sind die Bytevergleiche, die Zählungen und die Verhältnisse von Läufen,
die **gleichzeitig** liefen. Die endgültige Tabelle misst `sh /y/werkzeuge/vis2s/messung_final.sh` im Ruhefenster.

Bezug für „gleich“ ist überall das Ergebnis von `-threads 1` (alle Lumps ohne den Kopf mit NRCs Zeitstempel,
`Y:\werkzeuge\vis2s\bezug.txt`), im ctest dazu der Visibility-Lump von NRCs q3map2.exe.

## Stand in einem Satz

Der Fluss rechnet auf den gemessenen Karten mit 22 bis 39 % weniger CPU-Zeit dasselbe Ergebnis Byte für Byte (beide
Regeln, jede Thread-Zahl; nach der ersten Runde waren es 7 bis 15 %). Die zweite Runde (Abschnitt 7) hat gemessen,
warum vom Geometrie-Merker so wenig ankam – nicht der Cache, sondern seine Größe und sein eigener Aufwand –, und ihn
danach umgebaut. Die Lücke zwischen dem deterministischen VIS und NRCs Regel schließt das nicht – sie liegt an der
Kette echter Abhängigkeiten, nicht an der Rechenzeit je Fluss (Abschnitt 5; die feste Regel baut Auftrag VIS3 in
`Y:\p_vis3`). **Die Vorgabe bleibt NRCs Regel.**

## 1. Profil: wohin die Zeit geht

Stichproben-Profil eines eigenen Laufs (`skripte\profil.py`: Fäden anhalten, RIP lesen, über dbghelp mit Inline-Ketten
auflösen; `v\prof` = fee4525c1 mit PDB):

| Anteil der Flusszeit | md_ch_ben, 1 Thread (16 423 Proben) | t3_byss, NRCs Regel, 32 Threads (1035 Proben) |
|---|---|---|
| Trennebenen suchen (`ClipToSeperators` mit `PlaneFromPoints`) | 50,8 % | 41,1 % |
| Windings schneiden (`VisChopWinding`) | 21,2 % | 30,5 % |
| Bitvektoren und Schleife (`RecursivePassagePortalFlow`, `BhedCommonWords`) | 25,0 % | 24,9 % |
| Speicher, Sperren, Atomics (`BhedFrameBuffers`, `BhedMark`, Status lesen, `__chkstk`) | 3 % | 3 % |

- In den Trennebenen kostet `PlaneFromPoints` allein 24,5 % der Flusszeit, davon rund 19 % das Normieren (Wurzel und
  drei Divisionen in double, zurück nach float); die Division von `( i + 1 ) % numpoints` 3 %.
- Synchronisation und Speicher spielen im Fluss keine Rolle. Der Abstand des deterministischen VIS kommt vom Warten
  (Abschnitt 5), nicht aus der Rechnung.

Zählfassung (`diag\stat_patch.py`, ein Thread; ctf3 in Klammern):

| | md_ch_ben (1750 Portale) | ctf3 (2762 Portale) |
|---|---|---|
| Rahmen (Aufrufe von `RecursivePassagePortalFlow`) | 56,4 Mio. | 11,0 Mio. |
| Portale in mightsee / ausgelassen („nichts Neues“) / in die Geometrie | 103,5 / 21,0 / 82,5 Mio. | 24,6 / 6,9 / 17,7 Mio. |
| weggeschnitten in Stufe 0 / Stufe 1 / weiter in die Tiefe | 24,9 / 1,3 / 56,3 Mio. | 6,0 / 0,8 / 10,9 Mio. |
| Abstiege durch ein schon markiertes Portal | 53,5 Mio. (95 %) | 9,1 Mio. (83 %) |
| `ClipToSeperators` / Versuche / gespeicherte Ebenen | 101,8 / 775,5 / 379,0 Mio. | 18,2 / 147,6 / 67,2 Mio. |
| `VisChopWinding` (ganz davor / ganz dahinter / geschnitten), Punkte im Mittel | 486,6 Mio. (69 / 4 / 27 %), 4,0 | 101,0 Mio. (60 / 6 / 35 %), 4,1 |

- Ein Fluss läuft dieselben Portale immer wieder ab: 56 Mio. Rahmen für 1750 Portale, 95 % der Abstiege durch ein
  schon markiertes Portal, um Bits zu jagen, die in mightsee stehen und nie sichtbar werden. Fast jeder Rahmen
  sucht seine Trennebenen neu (0,99 Füllungen von Stufe 0 je Rahmen), 2,05 Versuche je gefundener Ebene.
- Die schnellen Kugelproben am Anfang (`d < -radius`) weisen in PassagePortalFlow nie ab (0 Fälle): die Passagen
  haben diese Portale schon entfernt.
- Die Hälfte der Rahmen hat **genau die Windings eines früheren Rahmens**: 55 % (Tabelle mit 2^14 Plätzen) bis 66 %
  (2^18) der `ClipToSeperators`-Aufrufe auf md_ch_ben, 42 bis 48 % auf ctf3. Über Zeiger ist das nicht zu sehen
  (unbeschnittenes pass: nur 5 bis 7 %), nur über den Inhalt.

## 2. Umgesetzt – Byte für Byte dasselbe Ergebnis

1. **Geometrie-Merker des Flusses** (`visflow.cpp`, `BhedGeoMemo`). Was die Geometrie mit einem Portal p des Blatts
   macht, hängt nur ab vom Fluss (Ebene, Mitte und Radius des Basisportals), von den Windings source und pass, mit
   denen der Rahmen betreten wurde, von p und davon, wie viele Ebenen die zwei Trennebenen-Zwischenspeicher gerade
   halten – nicht von den Bitvektoren und nicht vom bisher Gefundenen. Der Merker hält diese Funktion je Thread:
   Zustand (Fluss, Punkte von source, Punkte von pass) → Nummer, Punkt für Punkt verglichen; Ergebnis (Nummer, p,
   Ebenen vorher) → Ebenen nachher und „weggeschnitten“ oder Nummer und Windings hinter p. Ein Rahmen, der sein
   Portal im Merker findet, sucht keine Ebene und schneidet nichts.
   - Der Zwischenspeicher zählt dann mehr Ebenen, als wirklich da sind (`pstack_t::sephave`). Ein späteres Portal
     desselben Rahmens, das nicht im Merker steht, baut die fehlenden nach (`BhedSeperatorsHave`): dieselben Ebenen,
     sie hängen nur an den Windings des Rahmens. Dafür ist NRCs innere Schleife von `ClipToSeperators` unverändert
     in eine eigene Funktion gezogen (`BhedEdgeSeperator`).
   - Gleich für jede Tabellengröße: ein überschriebener Platz bekommt eine neue Nummer, keine Nummer wird zweimal
     vergeben. NRCs Eigenheit (der Zwischenspeicher behält nur ein Anfangsstück, wenn das erste Portal dabei
     weggeschnitten wird) bleibt erhalten – sie steckt in den „Ebenen vorher/nachher“ des Schlüssels.
   - Größe in Runde 1: 2^12 Zustände und 2^14 Ergebnisse je Thread (1,4 MB); seit Runde 2 2^14 Zustände und 2^17
     Ergebnisse (5,5 MB, Abschnitt 7). `BHED_VIS_GEOMEMO=<n>` stellt 2^n Zustände ein, 0 schaltet den Merker ab
     (für Messungen und den ctest). `-v` zeigt am Ende des Flusses, was er getan hat.
   - Trefferquote: md_ch_ben 37,4 von 82,5 Mio. Portalen aus dem Merker (45 %; 2,5 Mio. überschriebene Zustände,
     4,7 Mio. nachgebaute Ebenen), ctf3 5,7 von 17,7 Mio. (32 %), t3_byss rund 31 %.
2. **Winding-Schnitt mit vier Punkten auf einmal** (`VisChopWinding`, SSE2): dieselben Produkte und Summen in
   einfacher Genauigkeit und in derselben Reihenfolge, der Ebenenabstand in double abgezogen, auf float gerundet wie
   `dists[i] = …`. Ein float liegt genau dann über dem double `ON_EPSILON`, wenn er mindestens `float( ON_EPSILON )`
   ist (0,1f ist der kleinste float über 0,1), und genau dann unter `-ON_EPSILON`, wenn er höchstens -0,1f ist. Die
   letzten vier Punkte dürfen die vier davor überlappen (dieselben Zahlen noch einmal); die Seiten werden nur
   ausgeschrieben, wenn wirklich geschnitten wird. Windings mit weniger als vier Punkten laufen durch NRCs Schleife.
   Gilt auch für `-vis -nopassage`.
3. **Kantenschleifen ohne Division**: `( i + 1 ) % numpoints` als Vergleich (`BhedEdgeSeperator`, `VisChopWinding`).

Nachweis „gleich `-threads 1`“ (Visibility-Lump / alle Lumps gegen `bezug.txt`):

| Karte | Läufe | Ergebnis |
|---|---|---|
| ctf3 (2328ad26…/dbcca994…) | 1 Thread: Merker 2^12, 2^1, aus; jeder Zwischenstand (über 20 Läufe) | gleich |
| md_ch_ben (bfdfd32b…/b8db8bea…) | 1 Thread, jeder Zwischenstand (12 Läufe) | gleich |
| t3_byss (2c91eaa0…/63f23aed…) | 32 Threads `-bhedvisexact`: nur Merker, Merker + Schnitt (7 Läufe); Endstand (48,6 s, 413 CPU-s) | gleich |
| duel_mos_espa (39a0dcd7…/a4e5ceb1…) | 32 Threads `-bhedvisexact`: nur Merker (668 s); Endstand (690 s, 1623 CPU-s) | gleich |
| Gitter 9 × 9 (ctest q3map2_visgleich) | `-vis`, `-vis -nopassage`, `-vis -passageOnly` mit 1, 8, 32, 100 Threads; Merker Vorgabe, 2^4, aus | Visibility-Lump = NRCs q3map2.exe |

Die Läufe „Endstand“ liefen mit `v\neu1`; der eingecheckte Stand unterscheidet sich davon nur in einem Kommentar –
alle Abschnitte der neu gebauten exe (.text, .rdata, .data, .pdata …) sind bytegleich (`neu\bhed_q3map2.exe`).

## 3. Richtwerte unter Last (Runde 1, Stand c3f5950d4)

Gleichzeitig gestartete Läufe (dieselbe Fremdlast für alle), CPU-Sekunden; „nur Schnitt“ = Fassung ohne Merker
(Zwischenstand `v\mB`), „nur Merker“ = `v\m1`:

| Karte, Lauf | fee4525c1 | neu | Verhältnis | dazu |
|---|---|---|---|---|
| md_ch_ben, 1 Thread, 2 Runden zu dritt (Fremdlast 29 Kerne) | 71,2 / 73,8 | 61,1 / 61,7 | 0,85 | Merker + Schnitt in der ersten Fassung 62,3 / 62,9 |
| md_ch_ben, 1 Thread, 2 Runden zu dritt (Fremdlast 29 Kerne) | 65,0 / 65,9 | – | | nur Schnitt 67,3 / 68,7 (1,03), Merker + Schnitt 58,0 / 59,4 (0,90) |
| md_ch_ben, 1 Thread, zu zweit | 77,5 | – | | nur Merker 66,3 (0,86) |
| ctf3, 1 Thread, zu zweit | 23,6 | – | | nur Merker 22,8 (0,97) |
| t3_byss, `-bhedvisexact`, 32 Threads, abwechselnd, je 2 Läufe (Fremdlast 21 Kerne) | 508 / 429 (Wand 50,6 / 48,5 s) | – | | nur Schnitt 424 / 385 (0,86; Wand 41,9 / 41,7 s), Merker + Schnitt 396 / 417 (0,87; Wand 36,7 / 72,4 s) |
| t3_byss, `-bhedvisexact`, 32 Threads, zu zweit | 485,2 | – | | nur Merker 472,5 (0,97) |
| duel_mos_espa, `-bhedvisexact`, 32 Threads, zu zweit (Fremdlast 27 Kerne) | 1772 (Wand 703 s) | – | | nur Merker 1744 (0,98; Wand 668 s) |
| duel_mos_espa, `-bhedvisexact`, 32 Threads, zu zweit (Fremdlast 27 Kerne und ein eigener Bau) | 1745 (Wand 726 s) | 1623 (Wand 690 s) | 0,93 | Spitze 284 → 469 MB |

- Der Merker bringt auf der geschlossenen Karte am meisten (md_ch_ben 10 bis 14 %), auf den offenen wenig (t3_byss,
  duel_mos_espa 2 bis 3 %; Treffer t3_byss 26 %, duel_mos_espa 28 % im Endstand): dort sind mehr Zustände neu, und unter voller Last liegen die Tabellen (1,4 MB je Thread)
  nicht mehr im Cache – ein Treffer spart rund 2000 Takte Geometrie, kostet aber zwei verfehlte Cache-Zeilen. Rechnerisch
  wären bei 31 bis 45 % Treffern 22 bis 32 % zu holen.
- Der Schnitt mit vier Punkten bringt auf t3_byss rund 12 % (dort sind 30 % der Zeit Schnitte), auf md_ch_ben war die
  erste Fassung mit Seiten und Zählern je Punkt nicht schneller (1,03); der Endstand schreibt die Seiten nur noch
  beim wirklichen Schnitt.
- Speicher: 1,4 MB je Thread. t3_byss deterministisch (bis 128 Threads im Pool): Spitze 238 → 410 MB.
- ctf3 mit einem Thread: alle Fassungen innerhalb der Streuung (19,2 bis 25,8 CPU-s), kein Unterschied belegbar.

## 4. Geprüft und verworfen (mit Zahlen)

- **Zwei Ebenen nebeneinander suchen** (die Ebene des nächsten Eckpunkts schon rechnen, während die erste geprüft
  wird; bytegleich): kein Gewinn messbar – ctf3 23,7 / 24,5 CPU-s gegen 23,1 / 25,6 ohne, md_ch_ben im selben Lauf
  86,3 gegen 78,4 (fee4525c1). Wieder entfernt. Die SIMD-Ebenensuche des Vorvorgängers war ebenfalls langsamer.
- **Merker je Ebene** (`PlaneFromPoints` nach seinen drei Punkten): 56 bis 70 % Treffer, aber Nachschlagen (36 Bytes
  Schlüssel) kostet so viel wie Rechnen (rund 80 Takte). Nicht gebaut.
- **Merker über Zeiger** (source = Basis-Winding, pass = unbeschnittenes Portal): nur 5 bis 7 % der Fälle. Nicht gebaut.
- **Ganzen Teilbaum merken** („dieser Zustand wurde schon ausgeschöpft“): nicht exakt. Beim zweiten Besuch sind mehr
  Portale ausgelassen, ein anderes füllt den Trennebenen-Zwischenspeicher, und wird es dabei weggeschnitten, bleibt
  ein kürzeres Anfangsstück – spätere Portale werden schwächer geschnitten als beim ersten Besuch und können Neues sehen.
- **Einen Fluss auf mehrere Threads teilen**: scheitert an derselben Stelle. Ein Helfer kennt die Bits der früheren
  Teilbäume nicht, lässt weniger aus, und sobald so ein „zu viel“ besuchtes Portal einen Zwischenspeicher füllt, weicht
  das Ergebnis ab. Unabhängig wären nur Teilbäume mit fremden mightsee-Mengen; in offenen Räumen überlappen sie fast ganz.
- **Ungültige Eckpunkte ohne Normieren abweisen** (Vorprobe mit der unnormierten Ebene): braucht eine strenge
  Fehlerschranke gegen NRCs float-Rechnung, Gewinn höchstens etwa 7 %. Nicht gebaut.

## 5. Die Lücke zum deterministischen VIS

- Alles oben ist ein gleicher Faktor für beide Regeln; am Verhältnis deterministisch / NRCs Regel ändert es nichts.
  Auf duel_mos_espa bleibt die Kette aus `NOTIZEN_vis2_1011.md`: 140 Flüsse, die jeweils das **Ende** des Vorgängers
  brauchen (74,8 s), dann der teuerste Fluss (103,6 s) – mindestens 178 s mit beliebig vielen Kernen, NRCs Regel
  97 bis 169 s.
- Wartefälle auf t3_byss gezählt (`diag\warte_patch.py`, 32 Threads, `-bhedvisexact`): 68 210 Abfragen an ein noch
  laufendes früheres Portal, 52 586 ohne Warten entschieden. Gewartet wurde 15 624-mal, zusammen 6727 Thread-Sekunden:

| Wartefall | Fälle | Thread-s | ohne Warten entscheidbar? |
|---|---|---|---|
| wird sicher nicht ausgelassen, übersteht die Geometrie | 11 872 | 4755 | nein – der Abstieg braucht den Vektor |
| wird sicher nicht ausgelassen, wird dann weggeschnitten | 2967 | 1341 | ja: erst die Geometrie, dann warten |
| wird sicher ausgelassen (selbst mit portalflood nichts Neues) | 313 | 137 | ja |
| offen | 472 | 494 | nein |

  22 % der Wartezeit ließen sich vermeiden (Geometrie vor dem Warten, sicheres Auslassen). Nicht gebaut: es entfernt
  keine der Abhängigkeiten, die das Ende des Vorgängers brauchen, und stellt die innerste Schleife um.
- **Mit Schranken weiterrechnen** (statt zu warten mit „gefunden ≤ Vektor ≤ portalflood“ absteigen und nur warten,
  wenn eine Entscheidung an einem offenen Bit hängt): durchdacht, nicht gebaut. Schon die erste Frage im nächsten
  Blatt („ist q im Vektor von p?“) hängt an Bits, die p erst viel später oder nie findet – „nie“ weiß man erst an
  seinem Ende. Die Kette bleibt.
- **Aussicht – eine deterministische Regel, die NRCs Regel erzeugen kann**: NRCs Code legt nicht fest, wann ein
  Thread läuft. Jede Zuordnung „Fluss r sieht genau die Menge D(r) als fertig“ ist ein möglicher Lauf, wenn es eine
  Reihenfolge gibt, in der jeder Fluss alle seine Abfragen auf einmal stellt und sein Ende danach meldet (z. B. Wellen
  fester Größe: ein Fluss sieht genau die früheren Wellen). So eine Regel wäre reproduzierbar, ohne Kette, und dürfte
  nach dem Auftrag die Vorgabe werden – ihr Ergebnis ist aber **nicht** das von `-threads 1`, und Flüsse von 80 bis
  100 s blockieren auch sie, solange man auf ihr Ende wartet (dafür bräuchte es eine virtuelle Uhr je Fluss). Das ist
  eine Entscheidung für shank und ein eigener Auftrag; hier nur als Vorschlag.

## 6. Tests

- ctest **q3map2_visgleich** erweitert (`tools/karte/q3map2_visgleich.py`, `merker()`): `-vis -v` mit einem Thread
  in der Vorgabegröße, mit 16 Zuständen (`BHED_VIS_GEOMEMO=4`) und ohne Merker – alle Lumps gleich, Visibility-Lump
  gleich NRCs; die Zeile „geometry memo“ muss Treffer, überschriebene Zustände und nachgebaute Ebenen zeigen
  (Gitter: 771 Portale aus dem Merker, 44 Ebenen nachgebaut; mit 16 Zuständen 125 Treffer, 91 überschrieben, 15
  nachgebaut). Rot mit der exe von fee4525c1 (2 FEHLER: kein Merker), grün mit dem Endstand.
- ctest, Auswahl `^q3map2_|^karte_grenzen$` (14 Tests, Bau `Y:/p_vis2s/build`, exe = Endstand): **grün**
  q3map2_visgleich, q3map2_vispause, q3map2_determinismus, karte_grenzen, q3map2_vergleich, q3map2_pr316,
  q3map2_gpu, q3map2_gpubake, q3map2_funde, q3map2_pfade, q3map2_uebersetzer, q3map2_zusammen.
- **q3map2_importe war rot – schon bei fee4525c1** (dieselbe Beanstandung an der exe des Vorgängers): seit dem
  Einbau von `std::atomic::wait` (c5d848b7c, `BhedWaitFound`) bindet bhed_q3map2.exe
  `api-ms-win-core-synch-l1-2-0.dll` (WaitOnAddress, Windows ab 8) fest, die Prüfung kannte den API-Satz nicht.
  In `tools/check_exe_imports.py` erlaubt, grün.
- **q3map2_teilbau ist rot – genauso mit der exe von fee4525c1** (dieselben 2 Fehlschläge, 104 Zeilen OK):
  „-bounce 2, Brush (Detail in Raum B): aus dem Cache je Durchgang 9/24, 7/24, 7/24“ und „Shader (Wand in Raum A):
  18/18, 18/18, 18/18“. Das ist der Licht-Cache; der Zweig beginnt auf einem WIP-Stand von LCB2 (a00f6cf90). Am
  Licht-Code ist hier nichts geändert, das VIS des Tests (ein Thread, kleine Karten) ist bytegleich. Gehört LCB2.
- Bewusst nicht gelaufen: die übrigen ctests (Kern, Oberfläche), alle Selbsttest-Gruppen, karteneditor, Modus 1 –
  geändert sind nur `visflow.cpp`, `vis.h` und zwei Prüfskripte; die volle Runde läuft auf dem zusammengeführten Stand.
  Lints mit den geänderten Dateien: lint_sources ok; lint_portability 99, lint_pruefcodes 11, lint_fremdprogramm 1
  (keine Meldung in einer geänderten Datei).
- Bau: `bhed_q3map2_clang`, `kartegrenzen`, `kartezusammen` ohne Warnung.

## 7. Runde 2 (11.11., Fable): warum vom Merker nur 2 bis 14 % ankamen – und der Merker danach

Stand vorher c3f5950d4 („r1“). Auch diese Zeiten sind **Richtwerte**: der Rechner war durchgehend mit rund 30 Kernen
fremd belegt; in den Messfenstern (Sperre) liefen 5 bis 21 fremde Kerne weiter. Ein Ruhefenster gab es nicht –
`messung_final.sh` ist nicht als Ganzes gelaufen (nur geprüft, siehe 7.5).

### 7.1 Gemessen: wo der Gewinn blieb

Messfassung `diag\zeit_patch.py` gegen c3f5950d4 (Takte des Zeitstempelzählers um Nachschlagen, Geometrie und
Eintragen, Zähler je Thread; Abschnitte über 2 Mio. Takte gelten als Unterbrechung und fallen weg; der Zeitnehmer
kostet 50 bis 60 Takte je Abschnitt – ohne Merker gemessen – und ist abgezogen):

| | md_ch_ben, 1 Thread | t3_byss, NRCs Regel, 32 Threads |
|---|---|---|
| Portale in die Geometrie | 82,5 Mio. | 361 Mio. |
| Geometrie ohne Merker | 137,8 Mrd. Takte (1669 je Portal) | 784,8 Mrd. (2173 je Portal) |
| Treffer mit 2^12 Zuständen | 37,4 Mio. (45 %) | 94,8 Mio. (26 %) |
| damit gespart | 54,3 Mrd. = 39 % der Geometrie (ein Treffer-Portal kostet 1453, nicht 1669) | 157 Mrd. = 20 % (1659 statt 2173) |
| Aufwand des Merkers | 15,1 Mrd. = 28 % des Gesparten | 72,9 Mrd. = 46 % des Gesparten |
| – Nachschlagen ohne Treffer | 86 Takte × 45,2 Mio. | 66 × 266 Mio. |
| – Zustand suchen und eintragen (Hash über alle Punkte, 216 Bytes) | 156 × 45,2 Mio. | 139 × 266 Mio. |
| – Treffer mit Kopie der Windings | 130 × 23,7 Mio. | 263 × 47,2 Mio. |
| – Treffer „weggeschnitten“ | 78 × 13,6 Mio. | 123 × 47,6 Mio. |
| Treffer, deren Zustand überschrieben war (Geometrie noch einmal) | 2,5 Mio. | 9,5 Mio. |
| über einen Treffer betretene Rahmen / davon mit einem Portal, das die Windings wirklich braucht | 23,7 / 2,6 Mio. (11 %) | 47,2 / 9,2 Mio. (20 %) |

- Die Rechnung der ersten Runde („Trefferquote × Anteil der Geometrie“ = 22 bis 32 %) lag zu hoch: Treffer-Portale
  sind billiger als der Durchschnitt, und ein Viertel bis die Hälfte des Gesparten fraß der Merker selbst – am
  meisten bei den Portalen **ohne** Treffer (Hash über alle Punkte, Zustand schreiben), also gerade auf den offenen Karten.
- **Nicht der Cache begrenzte, sondern die Größe.** Dieselbe Fassung mit 2^16 statt 2^12 Zuständen (22 statt 1,4 MB
  je Thread, weit außerhalb jedes Caches) auf t3_byss: 51 statt 26 % Treffer, 391 statt 479 CPU-s. Die Vermutung der
  ersten Runde („unter voller Last fallen die Tabellen aus dem Cache“) war damit widerlegt – ein Treffer spart rund
  2000 Takte, ein verfehlter Cache-Zugriff kostet 100 bis 300.
- 80 bis 89 % der über einen Treffer betretenen Rahmen brauchen ihre Windings nie; die Fassung der ersten Runde
  kopierte sie jedes Mal und rechnete die Geometrie neu, wenn der Zustand überschrieben war.

### 7.2 Umgesetzt – Byte für Byte dasselbe Ergebnis

1. **Windings erst bei Bedarf** (`pstack_t::lazy`, `BhedGeoWindings`). Ein Treffer gibt dem Rahmen darunter nur noch
   die Nummer der Windings mit; er läuft über die Ergebnisse dieser Nummer weiter. Erst wenn dort ein Portal nicht im
   Merker steht, werden die Punkte aus dem Zustand geholt – oder, falls dessen Platz inzwischen überschrieben ist,
   aus den Windings des Rahmens darüber neu gerechnet (dieselbe Geometrie mit den Füllständen der
   Trennebenen-Zwischenspeicher von damals; die Füllstände danach und der Platz des Ergebnisses werden geprüft,
   Abweichung = `Error`). Ein überschriebener Zustand kostet so keinen Treffer mehr.
2. **Ergebnisse in Buckets zu vier** (16 Bytes je Ergebnis, ein Bucket = eine Cache-Zeile; ein neues Ergebnis nimmt
   einen leeren Platz, sonst den des ältesten Zustands) und **acht Ergebnisse je Zustand** statt vier.
3. **Bucket vorab holen**: die Adresse steht fest, sobald das Portal in mightsee steht; `_mm_prefetch`, während der
   Rahmen noch seine Bitvektoren baut. Ein eben erst angelegter Zustand kann noch kein Ergebnis haben – sein Rahmen
   fragt gar nicht erst.
4. **Hash der Punkte**: ein Produkt 64 × 64 → 128 Bit je Punkt, keines wartet auf ein anderes (vorher zwei
   verkettete Multiplikationen je Punkt). Zustand 208 statt 216 Bytes (Nummern 32 Bit).
5. **32-Bit-Nummern**: ist die Hälfte verbraucht, beginnt der Merker zwischen zwei Flüssen leer; ein einzelner
   Fluss, dem die andere Hälfte ausgeht, rechnet ohne neue Zustände weiter. `BHED_VIS_GEOMEMO_IDS=<n>` setzt die
   Grenze für den ctest herab.
6. **Größe 2^14 Zustände und 2^17 Ergebnisse je Thread (5,5 MB)** statt 2^12 / 2^14 (1,4 MB).

Portale, für die die Geometrie noch gerechnet wird (ein Thread, also reproduzierbar; in Klammern Anteil an allen):

| Fassung | md_ch_ben (82,53 Mio.) | ctf3 (17,70 Mio.) | t3_byss (357,44 Mio.) |
|---|---|---|---|
| r1, 2^12 Zustände, 4 Ergebnisse je Zustand | 45,17 Mio. (55 %) | 11,99 Mio. (68 %) | – (32 Threads: 74 %) |
| Windings bei Bedarf, sonst wie r1 | 38,94 Mio. | 10,83 Mio. | 239,4 Mio. (67 %) |
| dazu Buckets zu vier, 8 Ergebnisse je Zustand | 34,07 Mio. | 10,09 Mio. | 208,2 Mio. (58 %; mit Zuständen zu zweit) |
| 2^13 Zustände | 31,04 Mio. | 9,97 Mio. | |
| **2^14 Zustände (Vorgabe)** | **30,38 Mio. (37 %)** | **9,93 Mio. (56 %)** | 158,5 Mio. (44 %; mit Zuständen zu zweit) |
| 2^16 Zustände | 30,17 Mio. | 9,91 Mio. | 150,3 Mio. (42 %; 4 Ergebnisse je Zustand) |

- md_ch_ben und ctf3 sind mit 2^14 am Boden (mehr Tabelle findet nichts mehr: was übrig ist, sind wirklich neue
  Windings). t3_byss gewinnt bis 2^16 weiter, aber wenig: im Messfenster (NRCs Regel, 32 Threads, je 2 Läufe)
  2^12 381 / 401 CPU-s, 2^13 330 / 336, **2^14 322 / 323**, 2^15 309 / 311, 2^16 303 / 304 – bei 200, 242, **326**,
  494 und 830 MB Spitze (fee4525c1: 159 MB). 2^14 ist das Knie; 2^15 brächte 4 % für den doppelten Speicher.
- Speicher: 5,5 MB je Thread. NRCs Regel mit 32 Threads: t3_byss Spitze 159 → 202 (r1) → 326 MB. Der
  deterministische VIS hält bis zu 4 × numthreads Threads (128): t3_byss 238 → 410 (r1) → 910 MB, duel_mos_espa
  284 → 469 → 959 MB. Kleiner stellen: `BHED_VIS_GEOMEMO=13` (2,7 MB je Thread, t3_byss rund 3 % mehr CPU-Zeit).

### 7.3 Geprüft und verworfen (mit Zahlen)

- **Zustände zu zweit je Platz** (der ältere weicht): md_ch_ben 34,07 → 34,00 Mio. (2^12), 31,04 → 30,83 Mio. (2^13),
  30,55 → 30,39 Mio. (2^14) gerechnete Portale. Unter 1 %, nicht eingebaut.
- **Andere Regel im Bucket** (Reihenfolge des Eintragens; zuletzt benutzt nach vorn): md_ch_ben 2^12 34,07 / 34,07 /
  33,99 Mio., ctf3 10,09 / 10,08 / 10,07 Mio. Kein Unterschied – genommen ist die Regel ohne Schreiben beim Treffer.
- **Ergebnisse direkt adressiert statt in Buckets**: 2^12 mit 4 / 8 / 16 Ergebnissen je Zustand md_ch_ben 38,94 /
  36,94 / 35,44 Mio.; Buckets zu vier schaffen mit 4 Ergebnissen 35,86 Mio., mit 8 34,07 Mio.
- **PlaneFromPoints vermeiden oder wiederverwenden** (Punkt 2 des Auftrags) – nicht gebaut, weil exakt nichts zu
  holen ist:
  - Wurzel und Division sind IEEE-genau gerundet; denselben float liefert nur dieselbe Operation. Kehrwert mal drei
    statt drei Divisionen trifft die Rundung nur mit einer Prüfung je Komponente (liegt das Produkt mehr als zwei
    Einheiten der letzten double-Stelle von einer float-Rundungsgrenze?) – das spart zwei Divisionen und kostet neun
    Operationen dazu.
  - Wiederverwenden hieße ein Merker je Ebene (36 Bytes Schlüssel): schon in Runde 1 gemessen, Nachschlagen kostet
    so viel wie Rechnen.
  - Kandidaten ohne Normieren aussondern (Vorprobe mit der unnormierten Ebene): exakt nur mit einer Fehlerschranke
    gegen NRCs float-Rechnung (rund 3,5 · 2^-24 · Summe der Koordinatenbeträge: bei 8192 Einheiten 0,01, bei 65 536
    schon 0,08 von 0,1 `ON_EPSILON`); die Vorprobe kostet je Kandidat rund 40 Operationen, spart bei den 51 %
    abgewiesenen rund 30 und kommt bei den 49 % angenommenen obendrauf.
  - Zwei Kandidaten nebeneinander und die SIMD-Suche waren schon gemessen (Abschnitt 4, `NOTIZEN_vis2_1011.md`).
  - Seit dem Merker rechnet die Geometrie nur noch für 37 bis 56 % der Portale; das Normieren (in Runde 1 rund 19 %
    der Flusszeit) macht heute etwa ein Zehntel aus – auch ein Drittel davon wären 3 %.
- **Geometrie vor dem Warten im deterministischen VIS** (Punkt 3): vom Hauptchat gestrichen – Reihenfolge und
  Warten der Flüsse gehören dem Auftrag VIS3 (`Y:\p_vis3`, ab c3f5950d4). An `BhedResolve`, `BhedWaitFound` und dem
  Pool ist hier nichts geändert.

### 7.4 Richtwerte: in Messfenstern (Sperre; 5 bis 21 fremde Kerne liefen weiter) und gleichzeitig ohne Sperre

basis = fee4525c1, r1 = c3f5950d4, neu = Endstand dieses Zweigs; CPU-Sekunden (Wandzeit in Klammern), abwechselnd im
selben Fenster, wo nichts anderes dabeisteht:

| Karte, Lauf | basis | r1 | neu | neu / basis (CPU) | gleich `-threads 1` |
|---|---|---|---|---|---|
| t3_byss, NRCs Regel, 32 Threads, 2 Runden | 474 / 479 (27,5 / 20,4 s) | 412 / 426 (18,2 / 18,5 s) | 322 / 323 (14,2 / 13,7 s) | 0,68 | – |
| t3_byss, `-bhedvisexact`, 32 Threads, 2 Runden (Fremdlast 19 bis 21 Kerne) | 444 / 463 (47,9 / 46,6 s) | 430 / 416 (42,6 / 43,5 s) | 337 / 312 (31,5 / 31,9 s) | 0,72 | ja |
| md_ch_ben, NRCs Regel, 32 Threads, 2 Runden | 71,5 / 77,4 (4,7 / 3,5 s) | 56,9 / 59,4 (3,3 / 2,7 s) | 44,7 / 49,8 (3,1 / 2,2 s) | 0,63 | – |
| md_ch_ben, `-bhedvisexact`, 32 Threads, 2 Runden | 82,9 / 87,7 (6,9 / 6,1 s) | – | 55,0 / 60,6 (4,5 / 4,1 s) | 0,68 | ja |
| md_ch_ben, 1 Thread, 2 Runden, alle Fassungen gleichzeitig (ohne Sperre, Fremdlast 29 Kerne) | 77,5 / 79,6 | 65,2 / 68,0 | 51,0 / 53,1 | 0,66 | ja |
| ctf3, 1 Thread, ebenso | 24,9 / 24,7 | 21,9 / 21,9 | 19,3 / 19,3 | 0,78 | ja |
| duel_mos_espa, NRCs Regel, 32 Threads, alle drei gleichzeitig (ohne Sperre, Fremdlast 24 Kerne) | 1665 (298 s) | 1540 (266 s) | 1048 (179 s) | 0,63 | – |
| duel_mos_espa, `-bhedvisexact`, 32 Threads, beide gleichzeitig (ohne Sperre) | 1799 (618 s) | – | 1099 (281 s) | 0,61 | ja |

Die jeweils erste Runde mit einem Thread lief mit dem Zwischenstand `v\r2b` und `BHED_VIS_GEOMEMO=14` (derselbe Code,
die Vorgabe stand noch auf 2^12). Ein Lauf von r1 mit NRCs Regel auf md_ch_ben wich vom `-threads 1`-Ergebnis ab – NRCs
Regel eben; alle Läufe mit einem Thread oder `-bhedvisexact` sind gleich.

- CPU-Zeit gegen fee4525c1: ctf3 0,78, md_ch_ben 0,63 bis 0,68, t3_byss 0,68 (NRCs Regel) und 0,72 (deterministisch), duel_mos_espa 0,63 (NRCs Regel) und 0,61 (deterministisch) – 22 bis 39 % weniger; gegen Runde 1 (c3f5950d4) 12 bis 32 % weniger. Ohne Merker (`BHED_VIS_GEOMEMO=0`) liegt der Endstand bei fee4525c1 (md_ch_ben 80,8 neben 79,6, ctf3 23,7 neben 24,7 CPU-s im selben Lauf): der Gewinn kommt fast ganz aus dem Merker.
- Die Wandzeit fällt mit, wo die Kerne frei sind (t3_byss, NRCs Regel: 20,4 bis 27,5 → 13,7 bis 14,2 s).

### 7.5 Tests (Runde 2)

- ctest **q3map2_visgleich** (`merker()`): `-vis -v` mit einem Thread in der Vorgabegröße, mit 16 Zuständen
  (`BHED_VIS_GEOMEMO=4`), mit 40 Nummern (`BHED_VIS_GEOMEMO=6`, `BHED_VIS_GEOMEMO_IDS=40`) und ohne Merker – alle
  Lumps gleich, Visibility-Lump gleich NRCs. Die Zeile „geometry memo“ muss zeigen: Vorgabe 775 Portale aus dem
  Merker, 41 Ebenen nachgebaut; klein 405 aus dem Merker, **39 Windings** und 36 Ebenen nachgebaut; wenige Nummern
  **974-mal geleert, 7493 Zustände ohne Nummer**. Rot mit der exe von c3f5950d4 (3 FEHLER), grün mit dem Endstand.
- Gleich `-threads 1` (alle Lumps gegen `bezug.txt`), Endstand (exe 088e5d0d…): ctf3 und md_ch_ben mit einem Thread,
  t3_byss (42,6 s, 383 CPU-s) und duel_mos_espa (322 s, 1294 CPU-s) mit 32 Threads und `-bhedvisexact`. Dazu jeder
  Zwischenstand: ctf3 und md_ch_ben mit 2^4, 2^12, 2^13, 2^14, 2^16 Zuständen und mit 64 Nummern, t3_byss mit einem
  Thread in fünf Fassungen des Merkers (über 60 Läufe, alle GLEICH in `messung.txt`).
- ctest, Auswahl `^q3map2_|^karte_grenzen$` (14 Tests, `-j 14`) auf dem Endstand: 13 grün (q3map2_visgleich, vispause, determinismus, vergleich, pr316, gpu, gpubake, funde, pfade, uebersetzer, zusammen, importe, karte_grenzen). **q3map2_teilbau rot wie am Zweigbeginn** – dieselben 2 Fehlschläge im Licht-Cache („-bounce 2, Brush (Detail in Raum B): 9/24, 7/24, 7/24“ und „Shader (Wand in Raum A): 18/18, 18/18, 18/18“), 104 Zeilen OK, 1022 s; gehört LCB2. Bau des Ziels `bhed_q3map2_clang` ohne Warnung.
- Lints mit den geänderten Dateien: lint_sources ok; lint_portability 99, lint_pruefcodes 11, lint_fremdprogramm 1
  wie am Zweigbeginn, keine Meldung in einer geänderten Datei.
- `messung_final.sh` (Ruhefenster, etwa 45 min, jetzt mit r1 als dritter Fassung; die Tabelle nimmt nur den letzten
  Aufruf): mit drei Läufen geprüft, Messreihe und Tabelle laufen durch. Als Ganzes nicht gelaufen – kein Ruhefenster.
- Bewusst nicht gelaufen: die übrigen ctests, Selbsttest-Gruppen, karteneditor, Modus 1 (geändert sind nur
  `visflow.cpp`, `vis.h` und `tools/karte/q3map2_visgleich.py`; die volle Runde läuft auf dem zusammengeführten Stand).

## 8. Offen

- Endgültige Messtabelle im Ruhefenster: `sh /y/werkzeuge/vis2s/messung_final.sh` (etwa 45 min) →
  `messung_final.txt` mit basis / r1 / neu / ohne Merker je Karte. Danach die Zahl im Changelog bestätigen.
- Speicher des Merkers im deterministischen VIS: je Thread ein Merker, bis zu 4 × numthreads Threads. Wenn VIS3 die
  Reihenfolge der Flüsse neu ordnet, den Merker dort an die rechnenden Flüsse binden (ein wartender Fluss braucht
  keinen; seine Rahmen müssten dann ihre Nummern vergessen) oder die Größe an die Thread-Zahl koppeln.
- Der Merker gilt nur für PassagePortalFlow (die Vorgabe). `-vis -nopassage` hat den schnelleren Schnitt, keinen Merker.
- Was die Geometrie heute noch rechnet, sind neue Windings (md_ch_ben 37 %, ctf3 56 %, t3_byss rund 44 % der
  Portale); dort bleibt nur die Ebenensuche selbst (7.3) und die Vorprobe mit Fehlerschranke als Aussicht von
  höchstens wenigen Prozent.
- Deterministische Regel als Vorgabe: Entscheidung shank, Umsetzung VIS3 (Abschnitt 5, letzter Punkt).

## Changelog

## English

- Map compiler: VIS takes a fifth to a third less computing time for the same result (and about 5 MB more memory for each thread). The portal flow remembers what it clipped before - it walks through the same chains of portals again and again - and cuts windings four points at a time. The visibility data stays the same byte for byte, with any number of threads and with "Same VIS in every build".

## Deutsch

- Karten-Compiler: VIS braucht für dasselbe Ergebnis ein Fünftel bis ein Drittel weniger Rechenzeit (und je Thread etwa 5 MB mehr Speicher). Der Portalfluss merkt sich, was er schon geschnitten hat – er läuft dieselben Portalketten immer wieder ab –, und schneidet Windings mit vier Punkten auf einmal. Die Sichtbarkeitsdaten bleiben Byte für Byte gleich, mit jeder Thread-Zahl und mit „Dasselbe VIS in jedem Bau“.

## 中文

- 地图编译器:VIS 得到相同结果所需的计算时间减少五分之一到三分之一(每个线程多用约 5 MB 内存)。门户流会记住已经裁剪过的内容(它会反复走过相同的门户链),并且一次裁剪四个顶点。可见性数据逐字节保持不变,无论线程数多少,也无论是否打开“每次编译得到相同的 VIS”。

## 日本語

- マップコンパイラー: VIS は同じ結果を得るための計算時間が 2 割から 3 割強短くなりました(スレッドごとに約 5 MB 多くメモリを使います)。ポータルフローは一度クリップした内容を覚えておき(同じポータルの連なりを何度もたどるため)、ワインディングを 4 点ずつ切ります。可視データはバイト単位で変わりません。スレッド数にも「毎回のビルドで同じ VIS」にも関係なく同じです。

## Español

- Compilador de mapas: VIS necesita entre una quinta parte y un tercio menos de tiempo de cálculo para el mismo resultado (y unos 5 MB más de memoria por hilo). El flujo de portales recuerda lo que ya ha recortado –recorre una y otra vez las mismas cadenas de portales– y corta los polígonos de cuatro en cuatro puntos. Los datos de visibilidad siguen siendo idénticos byte a byte, con cualquier número de hilos y con «VIS igual en cada compilación».

## Čeština

- Kompilátor map: VIS potřebuje pro stejný výsledek o pětinu až třetinu méně výpočetního času (a asi o 5 MB paměti více na vlákno). Tok portálů si pamatuje, co už ořezal – prochází stále znovu stejné řetězce portálů – a ořezává polygony po čtyřech bodech najednou. Data viditelnosti zůstávají bajt po bajtu stejná, s libovolným počtem vláken i se „Stejné VIS při každém sestavení“.
