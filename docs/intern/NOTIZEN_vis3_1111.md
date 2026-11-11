# Notizen vis3 (11.11.): eine feste VIS-Regel für mehrere Threads – dieselben Bytes in jedem Lauf und mit jeder Thread-Zahl

Auftrag `auftrag_vis3_1111.md`. Worktree `Y:\p_vis3`, Zweig `p_vis3` (auf p_vis2s eabbe40af, also mit den inneren
Schleifen aus vis2s Runde 2), Bau `Y:/p_vis3/build` (`bhed_q3map2_clang`, `kartegrenzen`, `kartezusammen`). Werkzeuge, Läufe,
Messfassungen: `Y:\werkzeuge\vis3` (Inhalt in `UEBERGABE.md` dort). Vorher: `NOTIZEN_vis2_1011.md` (Abhängigkeiten der
Flüsse, die Kette, Anhalten/Fortsetzen), `NOTIZEN_vis2s_1011.md` (Abschnitt 5: der Vorschlag).

Rechner: i9-14900HX (30 nutzbare Threads), 64 GB. **Alle Zeiten dieser Notiz sind unter Last gemessen (Richtwerte)**:
neben den Läufen bauten und testeten andere Agenten (Fremdlast je Lauf 2 bis 29 Kerne, sie steht bei jeder Zeile).
Belastbar sind die Bytevergleiche, die Zählungen (Rahmen, Paare, Sichtlinien) und das Verhältnis von Läufen, die
unmittelbar nacheinander liefen. Über die Wandzeit entscheidet die Messung im Ruhefenster:
`sh /y/werkzeuge/vis3/messung_final.sh` (etwa 9 min, ein Messfenster) → `messung_final.txt`.

## Stand in einem Satz

`bhed_q3map2 -vis -bhedvisfixed` rechnet das VIS auf einer festen Maschine – 64 Ketten, die im Gleichtakt Rahmen
zählen –, liefert damit in jedem Lauf und mit jeder Thread-Zahl dieselben Bytes, besteht die Gültigkeitsprüfung
(`-bhedvischeck`) auf sechs echten Karten ohne einen Verstoß, hat auf der siebten (kejim_post) genau die 131 Lücken,
die NRCs Regel dort auch hat (sie stecken in der Rechnung der Flüsse unter der Regel, Abschnitt 2), und brauchte unter
Last meist 5 bis 35 % mehr Wandzeit als NRCs Regel, einmal 12 % weniger (duel_mos_espa); **Vorgabe wird sie erst nach
der Messung im Ruhefenster – die Bedingung „nicht langsamer“ ist unter Last nicht belegt.**

## 1. Die Regel

NRCs Regel: ein Fluss, der ein Portal passiert, nimmt dessen `portalvis`, wenn der Fluss dieses Portals gerade fertig
ist, sonst `portalflood`. Was „gerade fertig“ ist, entscheidet die Uhr des Rechners – jeder Lauf fällt anders aus. Die
feste Regel ersetzt die Uhr des Rechners durch eine Uhr, die Arbeit zählt (`visflow.cpp`, `g_bhedFixed`):

- **Maschine**: 64 Ketten (ihre Threads). Zeit ist die Zahl der Rahmen der Rekursion eines Flusses – sie hängt nur
  davon ab, was der Fluss liest.
- **Start**: die Flüsse starten in der sortierten Reihenfolge an den Grenzen eines Rasters; an einer Grenze nimmt jede
  Kette, die bis dahin frei ist, das nächste Portal (NRCs Thread-Schleife mit Threads, die nur an den Grenzen nach
  Arbeit sehen). Ein Rasterschritt ist halb so lang wie der Median der Flüsse, die zwei Schritte vorher endeten; wo
  keiner endete, wird er um ein Achtel weiter.
- **Ende**: ein Fluss, der bei s startet und n Rahmen hat, endet bei e = s + n + 1.
- **Sicht**: ein Fluss, der bei s gestartet ist und w Rahmen hinter sich hat, liest `portalvis` genau der Flüsse mit
  e ≤ s + ¾·w, `portalflood` aller anderen. Lange Flüsse sehen so, was endet, während sie laufen – wie bei NRC.

Damit ist jeder gelesene Vektor fertig (oder `portalflood`) und in jedem Lauf derselbe, also auch Ergebnis und
Rahmenzahl jedes Flusses, also alle Starts und Enden. Die Regel ist **ein möglicher Lauf von NRCs Regel** mit 64
Threads: ein Thread nimmt das Portal bei s, macht seinen w-ten Rahmen zur Zeit s + ¾·w und meldet das Ende bei
s + n + 1; nach Arbeit sieht er an den Rastergrenzen. Sie ist **nicht** der Lauf von `-threads 1` (mit einer Kette
ist sie es: `BHED_VIS_FIXED_CHAINS=1`, Prüfung im ctest).

Die echten Threads arbeiten diese Maschine ab, jeder ist eine Kette (`BhedFixedChain`). Sie halten sich an eines:
nichts wird entschieden, bevor es sicher ist.

- Eine Grenze ist passiert (die nächste „offen“), wenn jede Kette mit Fluss sie passiert hat – dann steht fest, welche
  Ketten bis dahin frei waren (`BhedFixedAdvance`). Wer bis zur offenen Grenze frei wird, nimmt seinen nächsten Fluss
  sofort: er startet dort, was die anderen auch tun.
- `portalflood` für ein Portal ohne Ende gibt es erst, wenn das Ende nicht vor der Sicht kommen kann
  (`BhedFixedUncertain`): sein Fluss läuft auf einer Kette, deren Uhr die Sicht erreicht hat, oder es hat noch keine
  Kette und kann nicht vor der Sicht starten (`BhedHorizon`: die offene Grenze; für alle bis auf so viele nächste
  Portale, wie Ketten noch hinter der offenen Grenze sind, die Grenze danach). Sonst wartet der Fluss und gibt
  seinen Zug ab.
- `-threads` sagt nur, wie viele Ketten zugleich rechnen (Züge, die Kette mit der niedrigsten Uhr zuerst). Die Kette
  mit der niedrigsten Uhr wartet nie.

Warum 64 Ketten, ¾ und der halbe Median: Abschnitt 4.

## 2. Nachweise

### Reproduzierbar (Ziel 1)

| Karte | Läufe mit `-bhedvisfixed` | Ergebnis |
|---|---|---|
| Gitter 9 × 9 (ctest `q3map2_visfest`) | `-vis` mit 1, 2, 8, 8, 32, 100 Threads; `-nopassage`, `-passageOnly` mit 1 und 8 | alle Lumps gleich |
| ctf3 | 30 Threads fünfmal, 5 Threads (Endstand); 30 / 8 / 3 / 1 Threads (Zwischenstände, je ihr eigener Hash) | a4260f5e… jedes Mal |
| t3_byss | 30 Threads viermal, 8 Threads | e5adb8c7… jedes Mal |
| duel_mos_espa | 30 Threads dreimal | 9099cfd7… jedes Mal (zwei weitere Läufe: dieselbe Zahl sichtbarer Portalpaare) |
| md_ch_ben | 30 Threads zweimal | bfdfd32b… = `-threads 1` (geschlossene Karte: jede Regel gibt dasselbe) |
| kejim_post (JK2, 24 686 Portale) | 30 Threads zweimal | 270704a5… beide Male |

Das Ergebnis hängt von der Karte ab und von den drei Konstanten der Regel (64 Ketten, ¾, halber Median) – nicht vom
Lauf, nicht von `-threads`, nicht von der Größe des Geometrie-Merkers (derselbe Hash vor und nach vis2s Runde 2 und
mit vollen wie kleinen Merkern). Mit einer Kette (`BHED_VIS_FIXED_CHAINS=1`): Visibility-Lump von NRCs q3map2.exe
`-threads 1` für `-vis`, `-vis -nopassage`, `-vis -passageOnly` (ctest), ctf3 gleich `bezug.txt`.

### Gültig (Ziel 2): `-vis -bhedvischeck`

Eine Teilmengen-Beziehung zu `-threads 1` garantiert der Algorithmus **nicht** – das gilt schon für NRCs Regel
(`NOTIZEN_vis2_1011.md` Abschnitt 1: der Trennebenen-Zwischenspeicher eines Rahmens füllt sich vom ersten Portal, das
ankommt). Was er garantiert und was geprüft wird (`src/q3map2/bhed_vischeck.cpp`, für jede Regel):

1. **obere Schranke**: `portalvis` ⊆ `portalflood` je Portal (ein Fluss nimmt nur Bits weg); auf der .bsp: Cluster-PVS ⊆ PVS von `-vis -fast`.
2. **untere Schranke**: eine Gerade, die ein Portal in sein Blatt verlässt und von Blatt zu Blatt durch das Innere von
   Portalen geht, ist eine Sichtlinie (die Blätter sind konvex) – jedes Portal auf ihr muss in `portalvis` stehen.
   256 Versuche je Portal zwischen Zufallspunkten des Portals und eines Portals aus seinem `portalflood`. Die Linien
   halten 1 Einheit Abstand von den Kanten der Portale und von den Ebenen (das Zehnfache von ON_EPSILON), und ihr Weg
   durch jedes Blatt ist länger als das. Gezählt wird auch, wie viele der fehlenden Portale in einen Cluster führen,
   den die Blatt-Sichtbarkeit nicht hat (nur das fehlt im Spiel).
3. **gegen `-threads 1`** (das Bild der Portal-Sichtbarkeit von `-bhedvisexact`): die Paare, die nur einer hat, werden
   gezählt; für jedes Paar, das nur `-threads 1` hat, werden 256 Sichtlinien genau zwischen diesen zwei Portalen
   versucht – keine darf es geben; und kein Portal auf einer Sichtlinie darf dort sichtbar sein und hier nicht.

| Karte (Portale × 2) | Regel | Sichtlinien | Portale darauf nicht sichtbar | Paare außerhalb `portalflood` | Portalpaare nur hier / nur `-threads 1` | Sichtlinie für „nur `-threads 1`“ | Cluster-Paare | nicht in `-fast` |
|---|---|---|---|---|---|---|---|---|
| ctf3 (5524) | `-threads 1` | 434 743 | 0 | 0 | – | – | 311 618 | 0 |
| | NRC | 434 743 | 0 | 0 | 245 / 10 | 0 | 311 698 | 0 |
| | **fest** | 434 743 | **0** | **0** | 491 / 8 | **0** | 311 765 | 0 |
| md_ch_ben (3500) | `-threads 1` | 364 178 | 0 | 0 | – | – | 482 580 | 0 |
| | NRC | 364 178 | 0 | 0 | 6 / 2 | 0 | 482 580 | 0 |
| | **fest** | 364 178 | **0** | **0** | 15 / 0 | **0** | 482 580 | 0 |
| md_fh1_jedi (6954) | `-threads 1`, NRC, **fest** | 1 417 351 | **0** | **0** | 0 / 0 (alle drei gleich) | – | 1 628 170 | 0 |
| mb2_manaan (10118) | `-threads 1` | 970 598 | 0 | 0 | – | – | 1 696 389 | 0 |
| | NRC | 970 598 | 0 | 0 | 908 / 3 | 0 | 1 696 716 | 0 |
| | **fest** | 970 598 | **0** | **0** | 976 / 7 | **0** | 1 696 737 | 0 |
| t3_byss (14848) | `-threads 1` | 759 966 | 0 | 0 | – | – | 1 306 381 | 0 |
| | NRC | 759 966 | 0 | 0 | 3347 / 312 | 0 | 1 306 876 | 0 |
| | **fest** | 759 966 | **0** | **0** | 5417 / 411 | **0** | 1 307 324 | 0 |
| duel_mos_espa (13704) | `-threads 1` | 958 026 | 0 | 0 | – | – | 1 144 422 | 0 |
| | NRC | 958 026 | 0 | 0 | 1024 / 77 | 0 | 1 144 483 | 0 |
| | **fest** | 958 026 | **0** | **0** | 1726 / 259 | **0** | 1 144 518 | 0 |
| kejim_post (49372) | NRC | 1 297 679 | 131 (keines in einen nicht sichtbaren Cluster) | 0 | – | – | 4 091 653 | – |
| | **fest** | 1 297 679 | **131 – dieselben wie bei NRC** (keines in einen nicht sichtbaren Cluster) | **0** | gegen NRCs Lauf: 10 160 / 1463 | **0** (für „nur NRC“) | 4 093 006 | – |
| taspir1 (25010) | NRC, **fest** | – (Far Plane 4096) | – | – | – | – | 1 094 314 / 1 094 341 (`-threads 1`: 1 094 195) | 0 |

- Sechs Karten, zusammen 4,9 Mio. Sichtlinien je Regel: kein Portal auf einer Sichtlinie fehlt, nichts liegt außerhalb
  von `portalflood`, und für kein Paar, das nur `-threads 1` zeigt, gibt es eine Sichtlinie. Die Zeilen `-threads 1`
  und NRC sind die Gegenprobe: dieselbe Prüfung, dieselben Sichtlinien.
- **kejim_post** (JK2; die größte Karte der Reihe, 6,2 Mrd. Rahmen – `-threads 1` wären rund drei Stunden und ist
  nicht gelaufen, der Bezug ist dort ein Lauf mit NRCs Regel): auf 1 297 679 Sichtlinien sind 131 von 7,7 Mio.
  Portalen nicht sichtbar, **bei NRCs Regel und bei der festen dieselben 131** – die Listen sind Zeile für Zeile
  gleich, kein Portal auf einer Sichtlinie ist bei NRC sichtbar und bei der festen Regel nicht, und für keines der
  1463 Paare, die nur NRCs Lauf hat, gibt es eine Sichtlinie. Keines der 131 führt in einen Cluster, den die
  Blatt-Sichtbarkeit nicht hat. Die feste Regel verdeckt dort also nichts, was NRCs Regel zeigt; die Lücken liegen
  unter der Regel, in der Rechnung der Flüsse, die beide Regeln teilen:
  - Jede Lücke hat eine Wurzel – das letzte Portal der Linie, das das fehlende nicht sieht, während alle danach es
    sehen. Es sind nur drei: 48507 (125 Lücken, 68 verschiedene Portale; alle Linien gehen zuerst durch 48503),
    49178 (3) und 47503 (3). Bei 48507 stimmen die Ebenen aller Portale der Ketten – das sind echte Sichtlinien, und
    der Fluss von 48507 verliert sie, der von 48503 nicht.
  - An allen drei Wurzeln ist das Portal selbst oder das erste dahinter eines mit einer fast doppelten Ecke (Kante
    0,002 bis 0,03 Einheiten; 250 der 24 686 Portale haben so eine, in den anderen Karten 0 bis 38).
  - q3map2 rechnet die Ebene eines Portals aus seinen ersten drei Punkten (`PlaneFromWinding`, vis.cpp). Sind zwei
    davon fast gleich, ist die Ebene falsch: auf kejim_post bei 69 Portalen um mehr als 0,05°, bei 12 um mehr als 10°,
    bei 6 um mehr als 45° (t3_byss 6 Portale, ctf3 1, taspir1 3, die anderen 0). 8 der 131 Lücken liegen auf Ketten
    mit so einem Portal (49178: 0,06°, 47498: 2,0°); dort urteilt auch die Prüfung mit der falschen Ebene – diese 8
    können ebenso gut Lücken wie Fehlurteile sein. Die übrigen 123 nicht.
  - Was den Fluss von 48507 die Sicht kostet, ist **nicht geklärt**. Die Trennebenen zwischen 48507, 48503 und den
    Portalen dahinter, nach dem Verfahren von `ClipToSeperators` nachgerechnet (`diag\trenner_zeigen.py`), schneiden
    die fehlenden Portale nicht weg. Der Geometrie-Merker von vis2s ist es nicht: ein Lauf mit NRCs Regel und
    `BHED_VIS_GEOMEMO=0` (650,8 s, 12 503 CPU-s gegen 9743 mit Merker) hat dieselben 131 Lücken mit denselben
    Wurzeln und Ketten (`log\wurzel_kejim_ohne.txt`). Übrig bleiben NRCs Rechnung selbst und die Bit-Vektoren von
    vis1; ob NRCs unveränderte q3map2.exe die Lücken auch hat, ist nicht geprüft.
  - Werkzeuge: `diag\wurzel_patch.py` (Messfassung der Prüfung: Wurzel und Kette je Lücke; mit `-fast`,
    `BHED_VIS_CHECK_REF` und `BHED_VIS_CHECK_AUS_REF=1` prüft sie ein Abbild in 6 s), `diag\ebenen_pruefen.py`,
    `diag\kurze_kanten.py`, `diag\portal_zeigen.py`; Listen `log\fehlt_kejim_nrc.txt`, `log\fehlt_kejim_fest.txt`
    (gleich), `log\wurzel_kejim.txt`.
- Die Prüfung ohne den Abstand „Weg durch das Blatt länger als 1 Einheit“ meldete auf duel_mos_espa 59 von 6,6 Mio.
  Portalen auf Sichtlinien als nicht sichtbar – bei `-threads 1`, NRCs Regel und der festen Regel dieselben 59 (keines
  davon im Bezug sichtbar), keines in einen nicht sichtbaren Cluster. Alle betrafen zwei Portale (5083, 5132) auf
  Linien, die ein Blatt auf weniger als 1 Einheit durchqueren – bei 5083 liegen zwei Portale 0,1 Einheiten
  hintereinander, und in so einem Blatt halten die Flüsse zwei Portale für eine Ebene (ON_EPSILON). Das ist q3map2s
  Rechnung, nicht die Regel; mit dem Abstand meldet die Prüfung dort nichts mehr (958 026 Linien, 0).
- Dass die Prüfung anschlägt: `BHED_VIS_CHECK_HIDE=50` lässt sie jedes 50. sichtbare Paar als nicht sichtbar ansehen –
  auf dem Gitter 14 × 14 meldet sie dann 1890 Portale auf Sichtlinien als nicht sichtbar (ctest).
- Nicht mit Sichtlinien geprüft werden Karten mit zusammengelegten Blättern (`-merge`, `-hint`: nicht konvex) und mit
  Far Plane (verdeckt mit Absicht; taspir1) – die Prüfung sagt das und lässt sie aus; dort bleibt die obere Schranke.
- Laufprotokoll: `Y:\werkzeuge\vis3\gueltig.txt` (`skripte\gueltig.py`).

### Anhalten und Fortsetzen (`-bhedvisstate`)

Ein fertiger Fluss steht mit Vektor **und Ende** im Zwischenstand (Format `BHVST2`; ein Stand der alten Fassung:
„state of another version“, es wird neu gerechnet). Beim Fortsetzen läuft die Maschine von vorn, ein Fluss aus dem
Stand nimmt nur seinen Platz im Ablauf wieder ein (kein Rechnen); jede Teilmenge fertiger Flüsse ist ein gültiger
Stand. ctest: mitten im Fluss angehalten (1 und 8 Threads), mit 8 / 3 / 32 Threads fortgesetzt, zweimal angehalten,
`-nopassage` und `-passageOnly`, beschädigter Stand, Stand von NRCs Regel („other vis switches“) – alle Lumps gleich
dem Lauf ohne Pause. Die Regel steht im Schlüssel des Stands (`fixed=64`). Für die Editor-Seite (VIS2c) ändert sich
nichts am Ablauf: Stoppdatei, Rückgabe 3, die Zeilen „vis paused“ / „vis resumed“.

## 3. Messungen unter Last (Richtwerte)

Läufe unmittelbar nacheinander, 30 Threads, dieselbe exe (NRCs Regel = ohne Schalter). Rahmen = Arbeit der Flüsse
(Aufrufe der Rekursion; bei der festen Regel in jedem Lauf gleich), Paare = sichtbare Cluster-Paare.

| Karte | Regel | Wand s | CPU-s | Spitze MB | Rahmen Mio. | größter Fluss Mio. | Paare (gegen `-threads 1`) | Fremdlast Kerne |
|---|---|---|---|---|---|---|---|---|
| duel_mos_espa | NRC | 59,8 / 61,3 / 50,6² | 882 / 879 / 954 | 380 | 713,1 / 710,3 | 50,9 | 1 144 482 … 1 144 489 (+0,005 … +0,006 %) | 12 |
| | fest | 72,8¹ / 52,9 / 63,0² | 934 / 920 / 1016 | 455 | 713,3 | 50,9 | 1 144 518 (+0,008 %) | 12 / 5 |
| t3_byss | NRC | 12,9 / 11,6 / 12,5² | 264 / 260 / 296 | 336 | 198,5 / 197,1 | 1,90 – 1,97 | 1 306 822 … 1 306 935 (+0,034 … +0,042 %) | 7 – 8 |
| | fest | 14,3 / 14,0 / 14,2² | 286 / 287 / 322 | 410 | 200,2 | 1,92 | 1 307 324 (+0,072 %) | 9 |
| | fest, 8 Threads | 34,0 | 258 | 316 | 200,2 | 1,92 | dieselben Bytes | 16 |
| mb2_manaan² | NRC | 19,0 / 8,0 | 176 / 205 | 301 | | | 1 696 699 / 1 696 716 (+0,018 … +0,019 %) | |
| | fest | 16,7 / 9,2 | 188 / 209 | 375 | | | 1 696 737 (+0,021 %) | |
| md_fh1_jedi² | NRC | 9,1 | 206 | 266 | | | 1 628 170 (gleich) | |
| | fest | 9,6 | 208 | 332 | | | 1 628 170 (gleich) | |
| taspir1² | NRC | 8,4 | 201 | 509 | | | 1 094 314 (+0,011 %) | |
| | fest | 9,2 | 214 | 576 | | | 1 094 341 (+0,013 %) | |
| kejim_post² | NRC | 498,6 | 9743 | 1233 | 6263,8 | 238,3 | 4 091 653 | 10 |
| | fest | 674,5 / 523,6 | 10 492 / 10 299 | 1300 | 6228,5 | 230,5 | 4 093 006 (+0,033 % gegen NRCs Lauf) | 11 / 8 |
| md_ch_ben | NRC | 1,72 / 1,77 | 38,7 | 205 | 56,36 | 0,33 | 482 580 (gleich) | 3 |
| | fest³ | 2,09 | 41,0 | 410 | 56,37 | 0,33 | 482 580 (gleich) | 6 |
| ctf3 | NRC | 0,82 / 0,76 | 12,1 / 12,9 | 224 | 11,10 / 11,07 | 0,19 | 311 655 / 311 692 (+0,012 … +0,024 %) | 4 |
| | fest³ | 0,99 | 13,6 | 410 | 10,72 | 0,08 | 311 765 (+0,047 %) | 5 |

¹ gleichzeitig lief der eigene ctest. ² aus den Gültigkeitsläufen (Zeit mit der Prüfung; der jeweils erste Wert von
mb2_manaan und der dritte von duel_mos_espa neben einem eigenen Bau). ³ vor dem kleineren Merker-Vorrat gemessen
(Speicher danach: ctf3 283 MB, md_ch_ben 278 MB).

- **Wandzeit (Ziel 3)**: unter Last war die feste Regel meist langsamer – t3_byss 10 bis 20 % bei 9 % mehr CPU-Zeit,
  mb2_manaan, taspir1 und md_fh1_jedi 5 bis 15 %, die kleinen Karten 0,1 bis 0,3 s, kejim_post (die größte) 5 % bei
  8 Kernen Fremdlast und 35 % bei 11 (523,6 und 674,5 gegen 498,6 s bei 10 Kernen; 6 bis 8 % mehr CPU-Zeit, 0,6 %
  weniger Rahmen – dort rechneten im Mittel 15,6 Kerne für die feste Regel und 19,5 für NRCs). Auf duel_mos_espa (das VIS hängt
  dort an einem Fluss: 51 Mio. Rahmen von 713) streut es am meisten: einmal 12 % schneller (52,9 gegen 59,8 / 61,3 s),
  zweimal langsamer (63,0 gegen 50,6 s neben einem eigenen Bau, 72,8 s neben dem eigenen ctest). In einem ruhigeren
  Augenblick (Fremdlast 2 bis 4 Kerne, Zwischenstand mit vollen Merkern): t3_byss 11,1 s gegen 10,4 / 11,4 s. Die
  Maschine läuft im Gleichtakt: eine Kette, die der Rechner gerade nicht rechnen lässt, hält die anderen auf, sobald
  sie ihr um einen Rasterschritt und den Nachlauf der Sicht voraus sind – NRCs Threads warten auf nichts. Deshalb
  leidet die feste Regel unter Fremdlast mehr, und deshalb entscheidet erst das Ruhefenster.
- **Arbeit**: die Flüsse der festen Regel brauchen 0 bis 1,3 % mehr Rahmen als NRCs (ctf3: 3 % weniger); der größte
  Fluss ist so lang wie bei NRC.
- **Dichte (Ziel 4)**: gegen `-threads 1` +0,008 bis +0,072 % sichtbare Cluster-Paare; NRCs Läufe liegen bei +0,006 bis
  +0,042 %. Die feste Regel ist damit um bis zu 0,03 Prozentpunkte lockerer als der lockerste NRC-Lauf (t3_byss: 389
  Paare von 1,3 Mio.), auf md_ch_ben gleich. Im Spiel ist das nicht zu messen; „innerhalb der Spanne von NRC“ ist es
  auf t3_byss und ctf3 streng genommen nicht. kejim_post: +0,033 % gegen den einen Lauf mit NRCs Regel.
- **Speicher**: 64 Threads (Stapel, Rahmenpuffer) und der Vorrat der Geometrie-Merker – so viele volle Merker
  (5,5 MB) wie Ketten zugleich rechnen, für die Flüsse, die darüber hinaus unterwegs sind (sie warten), kleine
  (1,4 MB): t3_byss 410 gegen 336 MB, duel_mos_espa 455 gegen 380 MB, kejim_post 1300 gegen 1233 MB. Mit einem vollen Merker je unterwegs befindlichem
  Fluss waren es 515 bis 544 MB. (`-bhedvisexact`: 745 bis 887 MB, das ist der Pool von vis2 mit bis zu 128 Threads.)

## 4. Der Weg dahin (mit Zahlen) – was verworfen wurde und warum

Der Vorschlag aus vis2s („Wellen fester Größe“ oder „virtuelle Uhr“) war durchdacht, nicht gebaut. Gebaut und gemessen:

1. **Bild beim Start** (ein Fluss liest, was bei seinem Start geendet hat, sonst nichts; nächster Fluss an die zuerst
   freie Kette): reproduzierbar, aber die Flüsse sehen zu wenig. ctf3, Rahmen je Kettenzahl: 1 → 10,96 Mio.
   (= `-threads 1`), 8 → 11,38, 32 → 12,47, 64 → 13,27 (+21 %), größter Fluss 655 Tsd. gegen 113 bis 208 Tsd. bei NRC.
   NRCs lange Flüsse leben davon, dass sie sehen, was endet, während sie laufen. → Sicht, die mitläuft.
2. **Mitlaufende Sicht, Start an die zuerst freie Kette, exakt**: Arbeit wie NRC (ctf3 +2,5 %), aber jede Kette muss an
   jedem Flussende warten, bis keine andere Uhr mehr hinter ihrer liegt. t3_byss 27,9 / 36,8 s gegen 20,0 / 19,4 s.
3. **Jede Kette bekommt ihre Flüsse vorab** (kein Warten am Flussende): die Kettenlasten sind ungleich, weil niemand
   vorher weiß, welche Flüsse lang werden (t3_byss, 64 Ketten: 2,0 bis 5,7 Mio. Rahmen, Mittel 3,2) – im Gleichtakt
   kostet das 29 % Wandzeit (8,78 gegen 6,79 Mio. Rahmenzeiten). Mit einem Vorlauf der Sicht von 16 384 Rahmen dazu
   +14 % Arbeit auf ctf3. Eine Sicht nur auf Flüsse, die in ihrer Kette nicht weiter hinten stehen: +17 % Arbeit.
4. **Raster der Starts** (die Fassung jetzt): Start an die zuerst freie Kette, aber nur an Rastergrenzen. Ein Raster
   von einem 64tel der Zeit wuchs bei vielen kurzen Flüssen geometrisch (eine Kette, 5524 Flüsse: Überlauf) → Raster aus
   der Länge der Flüsse. Zuerst warteten die freien Ketten, bis alle die Grenze passiert hatten: t3_byss im Mittel 30
   von 64 Ketten frei und wartend, 5 von 30 Zügen unbenutzt, 17,8 s. Mit sofortigem Start an der offenen Grenze und
   dem Horizont für Portale ohne Kette: 21 frei, 2,3 Züge unbenutzt, 11,9 s (NRC im selben Fenster 11,4 s).

Die drei Konstanten (`visflow.cpp`, `vis.cpp`):

| | Rahmen ctf3 / t3_byss (Mio.) | größter Fluss t3_byss | Wartefälle t3_byss | dazu |
|---|---|---|---|---|
| Sicht ½ | 11,07 / 202,7 | 2,82 | 17 Tsd. | |
| **Sicht ¾** | 10,72 / 200,2 | 1,92 | 35 – 50 Tsd. | so lang wie bei NRC (1,90 – 1,97) |
| Sicht ⅞ | 10,58 / – | | | ctf3: 31 Tsd. Wartefälle gegen 26 Tsd. |
| Raster ¼ Median (Sicht ½) | – / 202,7 | 2,82 | 21 Tsd. | kein Gewinn |
| 48 Ketten | 10,79 (Sicht ¾) / 202,0 (Sicht ½) | 2,82 | 22 Tsd. | t3_byss: Ende der letzten Kette 6,57 Mio. – Summe / 30 = 6,7 Mio. |
| 32 Ketten (Sicht ½, Zwischenstand) | 11,17 / – | | | ctf3: Ende der letzten Kette 431 Tsd. gegen Summe / 30 = 372 Tsd. |

- **64 Ketten**: die längste Kette muss in die Zeit passen, die 30 Threads für alles brauchen (Summe / 30), sonst
  wartet am Ende der ganze Rechner auf sie – mit 32 und 48 Ketten ist das knapp oder verfehlt; und mit doppelt so
  vielen Ketten wie Zügen finden wartende Ketten Ersatz. Mehr Ketten kosten Arbeit (128 Ketten mit festen Ketten:
  +5 %) und Speicher.
- **Sicht ¾**: schneller sieht ein Fluss mehr (weniger Rahmen), wartet aber öfter.
- **Raster halber Median**: der Vorlauf, den eine Kette haben darf, ohne zu warten.

## 5. Tests

- ctest **q3map2_visfest** (neu, `tools/karte/q3map2_visfest.py`, 38 Prüfungen, Gitter 9 × 9 und 14 × 14): Punkte
  aus Abschnitt 2. Rot mit der exe vor vis3 (`Y:\werkzeuge\vis2s\v\basis`): 23 FEHLER (der Schalter fehlt – NRCs Regel
  ist nicht reproduzierbar, die Prüfung gibt es nicht), grün mit dem Endstand.
- ctest, Auswahl `^q3map2_|^karte_grenzen$` (15 Tests, Bau `Y:/p_vis3/build`, exe = Endstand): **grün** q3map2_visfest,
  q3map2_visgleich, q3map2_vispause (der Zwischenstand der anderen Regeln im neuen Format), q3map2_determinismus,
  karte_grenzen, q3map2_vergleich, q3map2_pr316, q3map2_gpu, q3map2_gpubake, q3map2_funde, q3map2_pfade,
  q3map2_uebersetzer, q3map2_zusammen, q3map2_importe. **q3map2_teilbau rot wie auf der Basis** (dieselben 2
  Fehlschläge im Licht-Cache: „-bounce 2, Brush (Detail in Raum B)“ und „Shader (Wand in Raum A)“ – gehört LCB2).
- Bau: `bhed_q3map2_clang`, `kartegrenzen`, `kartezusammen` ohne Warnung.
- Lints mit den geänderten Dateien: lint_sources ok; lint_portability 99, lint_pruefcodes 11, lint_fremdprogramm 1 –
  dieselben Zahlen wie vor vis3, keine Meldung in einer geänderten Datei. `gen_handbuch.py --pruefen`
  (handbuch_sprachen): ok, es und cs ohne englische Reste.
- Bewusst nicht gelaufen (Regel „Schlussrunde je Zweig“): die übrigen ctests, alle Selbsttest-Gruppen, karteneditor,
  Modus 1, voller Neubau – geändert sind `visflow.cpp`, `vis.cpp`, `vis.h`, `visflow.h`, `help.cpp`, die neue
  `bhed_vischeck.cpp/.h`, eine Zeile in `src/karte/grenzen.cpp` (Prüfstelle `vis.cpp:759`, war 745) samt
  `grenzen.json`, das Prüfskript und ein Absatz im Handbuch. Die Oberfläche kennt den Schalter noch nicht.

## 6. Offen

- **Vorgabe**: Entscheidung nach `messung_final.sh` im Ruhefenster (Hauptchat). Unter Last ist „nicht langsamer als
  NRCs Regel“ nicht belegt (Abschnitt 3: meist 5 bis 35 % langsamer, je einmal schneller auf duel_mos_espa und
  mb2_manaan).
- **Lücken in den Flüssen selbst (nicht die Regel, kejim_post, Abschnitt 2)**: 123 Portale auf echten Sichtlinien
  fehlen dem Fluss eines Portals (48507) bei NRCs wie bei der festen Regel; die Ursache ist nicht geklärt. Und q3map2
  rechnet Portal-Ebenen aus den ersten drei Punkten – mit fast doppelten Ecken werden sie falsch (kejim_post 69
  Portale, bis 90°). Beides gehört in einen eigenen Auftrag: eine Korrektur ändert die Bytes gegenüber NRC. Nächste
  Schritte: den Fluss von 48507 entlang einer Kette aus `log\wurzel_kejim.txt` verfolgen (wo wird das Fenster leer:
  `mightsee`, Passage, Trennebene?); dasselbe mit NRCs unveränderter q3map2.exe gegenprüfen.
- **Die Prüfung nimmt q3map2s Portal-Ebenen**: an Portalen mit falscher Ebene (oben) urteilt sie falsch – sie sollte
  die Ebene aus allen Punkten selbst rechnen oder Linien durch solche Portale auslassen und zählen. Auf den Karten
  mit „0“ ändert das nichts am Befund (eine Kette aus gesunden Portalen ist eine echte Sichtlinie), es fehlen nur
  Linien.
- **Dichte**: bis 0,03 Prozentpunkte über NRCs lockerstem Lauf (Abschnitt 3). Enger ginge mit schnellerer Sicht (mehr
  Wartefälle) oder weniger Ketten (längste Kette passt dann nicht mehr).
- **Rechner mit weit mehr oder weniger als 30 Threads**: es rechnen höchstens 64 Ketten zugleich, und für die Wandzeit
  sind 64 Ketten auf rund 30 Züge abgestimmt. Mit 8 Zügen ist ein einzelner langer Fluss langsamer als bei NRC (er
  rechnet im Gleichtakt mit den anderen 63 Ketten), mit mehr als 32 Zügen warten Ketten öfter. Nicht gemessen
  (t3_byss mit 8 Threads: 34 s, NRC mit 8 Threads nicht gelaufen). Die Kettenzahl ist Teil der Regel – eine andere
  Zahl gibt andere Bytes.
- **Editor**: Schalter im Bau-Dialog (VIS2c / Hauptchat); der Text „duel_mos_espa: 5 instead of 1.5 minutes“ bei
  {{KeVisExakt}} bleibt wie von vis2c vermerkt bis nach der Messrunde.
- `-bhedvisexact` und `-bhedvisfixed` zusammen: Fehler mit Meldung (zwei verschiedene Regeln).
- Der Zwischenstand der festen Regel hält je Fluss 8 Byte mehr (das Ende); das dünne Format fehlt weiter.

## Changelog

## English

- Map compiler: `bhed_q3map2 -vis -bhedvisfixed` computes a full VIS that is the same in every build and with any number of threads, without the waiting of "Same VIS in every build". A fixed machine of 64 chains that count their work decides which finished portals a portal uses, instead of whatever happens to be finished on the computer. The result is a valid run of NetRadiant-Custom's rule (not the one of `-threads 1`); pause and resume work with it.
- Map compiler: `-vis -bhedvischeck` checks the visibility after it was computed - sight lines through the portals must be visible, nothing may be visible that the flood rules out - and prints the counts.

## Deutsch

- Karten-Compiler: `bhed_q3map2 -vis -bhedvisfixed` rechnet ein volles VIS, das in jedem Bau und mit jeder Thread-Zahl dasselbe ist, ohne das Warten von „Dasselbe VIS in jedem Bau“. Eine feste Maschine aus 64 Ketten, die ihre Arbeit zählen, entscheidet, welche fertigen Portale ein Portal nimmt – statt dessen, was auf dem Rechner gerade fertig ist. Das Ergebnis ist ein gültiger Lauf der Regel von NetRadiant-Custom (nicht der von `-threads 1`); Anhalten und Fortsetzen gehen damit.
- Karten-Compiler: `-vis -bhedvischeck` prüft die Sichtbarkeit nach dem Rechnen – Sichtlinien durch die Portale müssen sichtbar sein, nichts darf sichtbar sein, was die Flutung ausschließt – und druckt die Zahlen.

## 中文

- 地图编译器:`bhed_q3map2 -vis -bhedvisfixed` 计算出的完整 VIS 在每次编译、任意线程数下都相同,而且没有“每次编译得到相同的 VIS”的等待。由一台固定的机器(64 条统计自身工作量的链)决定一个门户使用哪些已完成的门户,而不是电脑上碰巧已完成的那些。结果是 NetRadiant-Custom 规则的一次有效运行(不是 `-threads 1` 的结果);暂停和继续同样可用。
- 地图编译器:`-vis -bhedvischeck` 在计算之后检查可见性——穿过门户的视线必须可见,洪泛已排除的内容不得可见——并打印统计数字。

## 日本語

- マップコンパイラー: `bhed_q3map2 -vis -bhedvisfixed` は、どのビルドでもスレッド数がいくつでも同じになる完全な VIS を計算します。「毎回のビルドで同じ VIS」のような待ち時間はありません。コンピューター上でたまたま完了しているポータルではなく、作業量を数える 64 本のチェーンからなる固定のマシンが、ポータルがどの完了済みポータルを使うかを決めます。結果は NetRadiant-Custom のルールの有効な実行のひとつです(`-threads 1` の結果ではありません)。一時停止と再開も使えます。
- マップコンパイラー: `-vis -bhedvischeck` は計算後の可視性を検査します。ポータルを通る視線は可視でなければならず、フラッドが除外したものは可視であってはなりません。数を表示します。

## Español

- Compilador de mapas: `bhed_q3map2 -vis -bhedvisfixed` calcula un VIS completo que es el mismo en cada compilación y con cualquier número de hilos, sin las esperas de «VIS igual en cada compilación». Una máquina fija de 64 cadenas que cuentan su trabajo decide qué portales terminados usa un portal, en lugar de los que casualmente estén terminados en el ordenador. El resultado es una ejecución válida de la regla de NetRadiant-Custom (no la de `-threads 1`); pausar y continuar funcionan con él.
- Compilador de mapas: `-vis -bhedvischeck` comprueba la visibilidad después de calcularla –las líneas de visión a través de los portales tienen que ser visibles y no puede ser visible nada que la inundación descarte– e imprime las cifras.

## Čeština

- Kompilátor map: `bhed_q3map2 -vis -bhedvisfixed` spočítá plné VIS, které je stejné při každém sestavení a s libovolným počtem vláken, bez čekání jako u „Stejné VIS při každém sestavení“. O tom, které hotové portály portál použije, rozhoduje pevný stroj z 64 řetězů, které počítají svou práci – ne to, co je na počítači zrovna hotové. Výsledek je platný běh pravidla NetRadiant-Custom (ne ten z `-threads 1`); pozastavení a pokračování s ním fungují.
- Kompilátor map: `-vis -bhedvischeck` zkontroluje viditelnost po výpočtu – přímky pohledu skrz portály musí být viditelné a nesmí být viditelné nic, co zaplavení vylučuje – a vypíše počty.
