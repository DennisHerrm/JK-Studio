# Notizen vis4 (11.11.): das VIS verdeckt, was man sieht – drei Fehler von q3map2 bei doppelt geschriebenen Portal-Ecken

Auftrag `auftrag_vis4_1111.md`. Worktree `Y:\p_vis3`, Zweig `p_vis3` (auf vis3 38488e148), Bau `Y:/p_vis3/build`
(`bhed_q3map2_clang`, `kartegrenzen`). Werkzeuge, Läufe, Messfassungen: `Y:\werkzeuge\vis4` (Inhalt in `UEBERGABE.md`
dort). Vorher: `NOTIZEN_vis3_1111.md` (der Befund: 131 Portale auf Sichtlinien fehlen auf kejim_post, und die Prüfung
`-bhedvischeck`).

Rechner: i9-14900HX (30 nutzbare Threads), 64 GB. Neben allen Läufen bauten und testeten andere Agenten (Fremdlast je
Lauf 10 bis 30 Kerne). Belastbar sind die Bytevergleiche und die Zählungen (Sichtlinien, Paare, Rahmen); die Zeiten
sind Richtwerte, Abschnitt 7.

## Stand in einem Satz

Der Fehler gehört q3map2 selbst – seit id's q3map, NetRadiant-Custom hat ihn unverändert, unsere Änderungen haben
nichts damit zu tun –, er hat drei Ursachen, alle an Portal-Ecken, die der BSP-Schritt doppelt schreibt, und er ist
größer als der Befund von vis3: in einem geraden Gang aus drei Portalen macht NRCs q3map2.exe den Raum am Ende
unsichtbar, und auf kejim_post fehlen mit NRCs Rechnung 2403 Portale auf Sichtlinien, 35 davon führen in einen
Cluster, den das Blatt gar nicht sieht. **Mit der Korrektur (jetzt Vorgabe) fehlt auf allen acht Karten und der
Testkarte keines mehr**; `-bhedvisnrcplanes` rechnet wie NRC. Mit `-threads 1` ändern sich die Bytes nur dort, wo NRCs
Portal-Ebenen falsch sind (t3_byss, taspir1; kejim_post ist nur mit der festen Regel verglichen); die Korrektur kostet
einen Punkt-Test je Trennebene, unter Last rund 2 bis 4 % CPU-Zeit (Abschnitt 7).

## 1. Wessen Fehler

| Probe | Ergebnis |
|---|---|
| NRCs unveränderte q3map2.exe (`Y:\werkzeuge\sp_af9c\nrc_dl\nrc\q3map2.exe`, nur diese exe gestartet) auf der Testkarte `karten\eigen3` (drei Ketten aus je vier Blättern, eigene Maße) | L0 der Kette T sieht L3 nicht, L0 der Ketten E und N weder L2 noch L3 – mit `-threads 1` und `-threads 8`, `-vis`, `-vis -nopassage`, `-vis -passageOnly` derselbe Lump 14fd21d4cbfd904a. Geraden von a durch b nach c gibt es (im ctest ohne das Programm gerechnet: 308 / 308 / 400) |
| dieselbe exe auf der Kette um Portal 48507 von kejim_post (`karten\kette4`, 4 Blätter) | Cluster 0 sieht Cluster 3 nicht |
| `-threads 1` | hat die Lücken: Ausschnitte von kejim_post um die drei Wurzeln von vis3 (`karten\kp3`, `kq`, `kr`; 61 bis 110 Blätter, ein Lauf 0,3 s) zeigen sie mit einem Thread |
| unser Stand vor allen VIS-Änderungen | die VIS-Quellen des Imports ee4920981 (vis.cpp, visflow.cpp, vis.h, visflow.h) mit dem Rest des heutigen Baus gebunden (`v\import_exe`): auf eigen3, kette4, kp3, kq, kr Byte für Byte NRCs Lump. `AddSeperators`, `PassageChopWinding`, `PlaneFromWinding` sind seit dem Import Zeichen für Zeichen gleich |
| unser Stand vor vis4 (38488e148) und vis2\alt (a00f6cf90) | derselbe Lump wie NRC |
| ctf3 mit einem Thread | NRCs q3map2.exe 2328ad2681764096 = unsere exe mit `-bhedvisnrcplanes` = unsere Vorgabe |

Also: ein Fehler von q3map2 (id → ydnar → NRC), in jeder Fassung, mit jeder Thread-Zahl. Der Geometrie-Merker von
vis2s, die feste Regel von vis3 und die Reihenfolge der Flüsse haben nichts damit zu tun.

## 2. Die drei Ursachen

Auslöser ist immer eine Ecke, die in der Portaldatei zweimal steht, wenige Tausendstel auseinander (kejim_post: 250
von 24 686 Portalen haben eine Kante kürzer als 0,1; t3_byss 31, taspir1 38, duel_mos_espa 6, ctf3 3, mb2_manaan 2,
md_ch_ben und md_fh1_jedi 0) – der BSP-Schritt schreibt sie an schrägen Flächen, und dieselbe Ecke steht in zwei
Portalen mit anderen Ziffern.

1. **Trennebene mit der Quelle auf beiden Seiten** (visflow.cpp `AddSeperators` für die Passagen, `ClipToSeperators`
   für die Flüsse). Die Ebene geht durch eine Kante der Quelle und einen Punkt des Durchgangs; die Seite der Quelle
   nimmt der Code vom ERSTEN ihrer Punkte, der weiter als ON_EPSILON (0,1) von der Ebene liegt. Teilen Quelle und
   Durchgang eine Kante, ist der Punkt des Durchgangs das Ende der Kante, 0,008 Einheiten daneben: die drei Punkte
   liegen fast auf einer Geraden (Sinus 1e-5), die Ebene dreht sich um die Kante, wie die Rundung es will, und das
   Kreuzprodukt in einfacher Genauigkeit verfehlt die eigenen Punkte um 0,11 bis 0,18. Eine fast doppelte Ecke der
   Quelle neben der Kante liegt dann um diesen Fehler „vorn“ und entscheidet: die Ebene wird genommen, obwohl Quelle
   und Durchgang auf derselben Seite liegen, und alles hinter dem Durchgang wird weggeschnitten.
   Das sind alle drei Wurzeln von vis3 (48507 → 48503, 49178 → 49188, 47503 → 47498), jeweils in der **Passage**:
   `diag\trenner32.py <k.prt> 48507 48503 48490` rechnet `AddSeperators` in float32 nach (Kante 2, Punkt 1:
   |Kreuzprodukt| 2,3, Sinus 1,3e-5, Quelle bei −0,18 / +415 / 0 / −0,18 – „Quelle auf beiden Seiten“, das Ziel 48490
   ganz dahinter), `diag\spur_patch.py` zeigt dieselbe Ebene im Programm. Die drei Wurzel-Portale selbst haben
   richtige Ebenen; mit Ursache 2 hängen sie nur über die doppelten Ecken zusammen.
   Das Nachrechnen von vis3 (`trenner_zeigen.py`) fand die Ebene nicht, weil es in doppelter Genauigkeit rechnete:
   dort hält die Ebene ihre eigenen Punkte, die doppelte Ecke liegt in ihr, und der erste Punkt außerhalb ist der
   richtige. Der Fehler braucht das Kreuzprodukt in einfacher Genauigkeit.
2. **Portal-Ebene aus den ersten drei Punkten** (vis.cpp `PlaneFromWinding`). Sind zwei der drei fast derselbe
   Punkt, dreht sich die Ebene um die Gerade durch die anderen beiden: sie verfehlt die eigene Wicklung um mehr als
   0,1 bei kejim_post 59 Portalen (bis 376 Einheiten, 65°), t3_byss 5, ctf3 1 (`diag\ebenen_zaehlen.py`).
   BasePortalVis sortiert mit ihr „vor“ und „hinter“ und die Flüsse schneiden mit ihr. In eigenen Ketten mit der
   doppelten Ecke unter den ersten drei Punkten fehlt bei NRC in 232 von 400 Fällen ein Cluster
   (`skripte\kette_suchen2.py`). **Das ist auf kejim_post die Hauptursache** (Abschnitt 5), und die Prüfung von vis3
   konnte sie nicht sehen, weil sie selbst mit diesen Ebenen urteilte.
3. **Keine Portal-Ebene** (vis.cpp `PlaneFromWinding`, qmath.h `PlaneFromPoints`). Liegen die ersten drei Punkte
   genau auf einer Geraden – die doppelte Ecke an einer Kante längs einer Achse –, ist das Kreuzprodukt 0,
   `PlaneFromPoints` gibt auf und hinterlässt die Ebene 0 0 0 0. Nichts liegt vor oder hinter ihr: das Portal
   sieht nichts und wird von nichts gesehen. kejim_post 3 Portale, taspir1 3, sonst 0. Die Kette N der Testkarte
   ist ein gerader Gang aus drei gleichen Quadraten mit so einer Ecke im mittleren.

## 3. Die Korrektur

- **Portal-Ebene aus allen Punkten** (`BhedPlaneOfAllPoints`, vis.cpp), wo NRCs Ebene keine ist oder einen Punkt der
  eigenen Wicklung um mehr als ON_EPSILON verfehlt: Normale = Summe der Kreuzprodukte des Fächers von Punkt 0
  (doppelte Fläche, im Drehsinn von `PlaneFromPoints`), Abstand = Mittel über alle Punkte, in doppelter Genauigkeit.
  Sie verfehlt die Wicklung dann um höchstens 0,002. Wo NRCs Ebene die Wicklung innerhalb von ON_EPSILON hält –
  der Toleranz, mit der das ganze VIS rechnet –, bleibt sie Ziffer für Ziffer.
- **Die Seite der Quelle aus allen ihren Punkten** (`BhedSeperates`, visflow.cpp; in `AddSeperators` und
  `BhedEdgeSeperator`): keiner der Punkte, die NRCs Schleife ansieht, darf weiter als ON_EPSILON vor der Ebene
  liegen. Eine Ebene durch eine Kante einer konvexen Wicklung besteht das immer; eine, die es nicht besteht, ist
  keine Trennebene und wird ausgelassen wie NRCs andere Fehlversuche (der nächste Punkt des Durchgangs kommt dran).
  Gerechnet werden nur die Punkte nach dem, der NRC die Seite gab – für eine Quelle mit drei Punkten nichts, für
  eine mit vier höchstens ein Punktprodukt je angenommener Ebene.
- Die Konsole nennt, was berichtigt wurde: `BehavEd: n portal planes from all points of the winding …`,
  `BehavEd: n separating planes left out in the passages / flows …`.
- **`-bhedvisnrcplanes`**: beides wie NRC (dessen .bsp, mit den Lücken). **`-bhedvisstrictplanes`**: die strengere
  Regel aus Abschnitt 6 (nicht Vorgabe). Beide stehen im Schlüssel von `-bhedvisstate` und `-bhedvisreuse`: ein
  Zwischenstand oder ein gemerktes VIS der einen Rechnung gilt nicht für die andere (und ein gemerktes VIS von vor
  vis4 wird einmal neu gerechnet).
- Nicht angefasst: der BSP-Schritt, der die doppelten Ecken schreibt (Abschnitt 9).

## 4. Die Prüfung `-bhedvischeck` (Punkt 4 des Auftrags)

Die Prüfung von vis3 urteilte mit q3map2s Portal-Ebenen – an den Portalen mit falscher Ebene sah sie Sichtlinien
nicht oder falsch, durch Portale ohne Ebene ging für sie nie eine Linie. Jetzt (`src/q3map2/bhed_vischeck.cpp`):

- **Eigene Form je Portal**: Ebene aus allen Punkten in doppelter Genauigkeit; Ecken näher als ON_EPSILON an der
  vorigen fallen weg (die doppelt geschriebene Ecke – ihre „Kante“ hat eine Richtung, die nur Rundung ist). Was
  bleibt, ist konvex und liegt in der Wicklung, ein Punkt darin liegt also im Portal. Das Innere liegt links jeder
  Kante (die Normale zeigt ins Blatt, in das das Portal führt); bleiben weniger als drei Ecken, ist das Portal kein
  Weg für eine Linie. Die erste Fassung ließ nur kurze Kanten weg und nahm beide Seiten – an Splitter-Dreiecken
  kann das Punkte außerhalb für innen halten; vor dem ersten großen Lauf ersetzt.
- **Ziele in der Nähe**: jede vierte Linie geht zu einem Portal des Blatts hinter dem Portal oder der Blätter
  dahinter, was immer die Flutung sagt. Vorher kamen die Ziele nur aus `portalflood` – ein Portal, das die Flutung
  zu Unrecht verloren hat, war nie Ziel: im Gang der Kette N fand die Prüfung nichts zu beanstanden.
- **Zählt die Ebenen der Flüsse**, die ihre eigene Wicklung um mehr als ON_EPSILON verfehlen oder keine sind.

Auf kejim_post mit NRCs Rechnung: vis3 131 Portale auf Sichtlinien, 0 in einen nicht sichtbaren Cluster; jetzt
1 502 845 Sichtlinien, 2795 Portale darauf nicht sichtbar, 47 davon in einen Cluster, den das Blatt nicht sieht, und
124 Ebenen (62 Portale), die ihre Wicklung verfehlen oder keine sind. Die meisten Wurzeln sind Portale mit falscher
Ebene – ihr `portalflood` hat das fehlende Portal gar nicht –, die Trennebenen von Ursache 1 stellen den kleineren Teil.

## 5. Ergebnisse je Karte

Feste Regel (`-bhedvisfixed`, in jedem Lauf dieselben Bytes), 30 Threads, exe = Endstand; „Lücken“ = Portale auf
Sichtlinien, die nicht sichtbar sind (in Klammern: davon in einen Cluster, den das Blatt nicht sieht – nur das
fehlt im Spiel). „vorher“ = `-bhedvisnrcplanes`; dessen Hashes sind die von vis3.

| Karte (Portale × 2) | Sichtlinien | Lücken vorher | Lücken Vorgabe | falsche / fehlende Ebenen vorher | berichtigt: Ebenen / Trennebenen Passagen / Flüsse | sichtbare Cluster-Paare vorher → Vorgabe |
|---|---|---|---|---|---|---|
| ctf3 (5524) | 461 765 | 0 | **0** | 1 | 1 / 11 / 15 | 311 765 → 311 759 |
| md_ch_ben (3500) | 354 639 | 0 | **0** | 0 | 0 / 0 / 0 | 482 580 → 482 580 (gleich) |
| md_fh1_jedi (6954) | 1 294 988 | 0 | **0** | 0 | 0 / 0 / 0 | 1 628 170 → 1 628 170 (gleich) |
| mb2_manaan (10118) | 1 007 422 | 0 | **0** | 0 | 0 / 0 / 33 | 1 696 737 → 1 696 737 (gleich) |
| t3_byss (14848) | 781 242 | 0 | **0** | 5 | 5 / 5 / 7780 | 1 307 324 → 1 307 334 |
| duel_mos_espa (13704) | 961 985 | 0 | **0** | 0 | 0 / 9 / 7867 | 1 144 518 → 1 144 521 |
| taspir1 (25010) | – (Far Plane) | – | – | 3 (keine Ebene) | 3 / 11 / 647 | 1 094 341 → 1 094 400 |
| taspir1, Portale ohne die Far Plane | 1 013 339 | 623 (0) | **0** | 3 (keine Ebene) | 3 / 11 / 2322 | 1 437 567 → 1 437 606 |
| kejim_post (49372) | 1 502 155 | 2795 (47) | **0** | 62 (59 falsch, 3 keine) | 62 / 183 / 41 421 | 4 085 213 → 4 085 441 |
| Testkarte eigen3 (18, ein Thread) | 2078 | 647 (647) | **0** | 2 | 2 / 2 / 1 | 37 → 48 |
| Ausschnitte kp3 / kq / kr (ein Thread) | 36 131 / 29 363 / 48 950 | 278 / 9 / 20 (0) | **0 / 0 / 0** | 8 / 2 / 7 | | |

- **Keine Sichtlinie mehr verdeckt**: 0 auf allen Karten, auf denen die Prüfung läuft, zusammen 6,4 Mio. Sichtlinien
  je Lauf; nichts liegt außerhalb von `portalflood`. taspir1 hat eine Far Plane (die Prüfung lässt solche Karten
  aus); seine Portale sind deshalb ohne sie geprüft (dieselbe .prt mit einer .bsp ohne den Schlüssel).
- **Dichte**: die Sichtbarkeit wird nicht merklich lockerer – höchstens +0,006 % sichtbare Cluster-Paare
  (kejim_post, taspir1), auf ctf3 sind es 6 Paare weniger. Mit der festen Regel verschiebt jede Änderung der Arbeit
  eines Flusses die Uhr der Regel, ein Teil dieser Zahlen ist das und nicht die Korrektur; die reine Wirkung zeigt
  die nächste Tabelle.

Dieselbe Rechnung wie `-threads 1` (`-bhedvisexact`, 30 Threads) – das ist der Vergleich „Bytes gegen NRC“:

| Karte | NRCs Ebenen (= NRCs q3map2.exe `-threads 1`) | Vorgabe | Paare | Cluster-Paare nur vorher / nur nachher | Cluster, die mehr sehen |
|---|---|---|---|---|---|
| ctf3 | 2328ad26… | **gleich** (obwohl 1 Ebene und 26 Trennebenen berichtigt sind) | 311 618 | 0 / 0 | 0 |
| md_ch_ben | bfdfd32b… | **gleich** | 482 580 | 0 / 0 | 0 |
| md_fh1_jedi | df524e55… | **gleich** | 1 628 170 | 0 / 0 | 0 |
| mb2_manaan | 2e7b3b41… | **gleich** | 1 696 389 | 0 / 0 | 0 |
| duel_mos_espa | 39a0dcd7… | **gleich** (9 + 4882 Trennebenen ausgelassen) | 1 144 422 | 0 / 0 | 0 |
| t3_byss | 2c91eaa0… | 55a25fc4… | 1 306 381 → 1 306 392 (+0,0008 %) | 18 / 29 | 17 |
| taspir1 | b5ade02e… | b3788ed1… | 1 094 195 → 1 094 207 (+0,0011 %) | 3 / 15 | 10 |
| kejim_post | nicht gelaufen (`-threads 1` wären Stunden) | anders (62 Ebenen berichtigt) | feste Regel: 4 085 213 → 4 085 441 | | |

**Die .bsp ändert sich gegenüber NRC also auf drei der acht Karten – genau denen, deren Portal-Ebenen bei NRC falsch
sind oder fehlen (t3_byss 5, taspir1 3, kejim_post 62).** Auf ctf3 ist eine Ebene berichtigt (sie verfehlte um 0,11)
und das Ergebnis trotzdem dasselbe. Mit `-bhedvisnrcplanes` kommt überall NRCs Ergebnis heraus (ctest: Lump von NRCs
q3map2.exe; ctf3 mit einem Thread gegen NRCs exe; die Hashes von vis3 mit der festen Regel auf allen acht Karten).
Mit NRCs Regel für mehrere Threads (die Vorgabe des Editors) gibt ohnehin jeder Lauf andere Bytes.

Was das Spiel heute falsch zeigt (Karten, die mit q3map2 / NetRadiant-Custom / JK Studio vor dieser Korrektur gebaut
sind): auf sieben der acht Karten nichts Nachweisbares – wo ein Portal etwas nicht sieht, sieht es ein anderes
Portal desselben Blatts. Auf kejim_post führen 47 der 2795 fehlenden Portale in einen Cluster, den das Blatt nicht sieht (Beispiel der Prüfung: Portal 43782
von Portal 22713 aus) – dort fehlen im Spiel Flächen; 304 Cluster sehen nach der Korrektur mehr. Auf taspir1 sind drei Portale für das VIS zu (10 Cluster
sehen danach mehr). Wie schlimm es werden kann, zeigt die Testkarte: ein Raum hinter zwei Durchgängen ist weg.

## 6. Zur Entscheidung für shank: die Regel für Trennebenen

Für die Portal-Ebenen gibt es nichts zu entscheiden (eine Ebene, die ihr Portal verfehlt, ist falsch). Bei den
Trennebenen gibt es zwei Stufen:

| | **R2: die Seite aus allen Punkten** (Vorgabe) | **R1: dazu die drei Punkte der Ebene** (`-bhedvisstrictplanes`) |
|---|---|---|
| Regel | keiner der Punkte der Quelle, die NRCs Schleife ansieht, liegt weiter als 0,1 vor der Ebene | dazu: die zwei Punkte der Kante und der Punkt des Durchgangs, aus denen die Ebene gemacht ist, liegen innerhalb 0,1 auf ihrer Seite – eine Ebene, die ihre eigenen Punkte verfehlt, ist ein Werk der Rundung |
| schließt | alle bekannten Lücken: die drei Wurzeln von kejim_post, die Kette T, 0 Lücken auf allen Karten | dasselbe: 0 Lücken auf allen Karten (kejim_post: 1 502 155 Sichtlinien, 0). Keine bekannte Lücke braucht die Zugabe; eigene Ketten in vierfacher und sechzehnfacher Größe, mit und ohne doppelte Ecke: mit R2 keine Lücke (`skripte\kette_suchen3.py`, 9 × 400 Ketten) |
| ausgelassene Trennebenen (Passagen / Flüsse, feste Regel) | ctf3 11 / 15, t3_byss 5 / 7780, taspir1 11 / 647, duel_mos_espa 9 / 7867, mb2_manaan 0 / 33, kejim_post 183 / 41 421 | ctf3 40 / 466, md_fh1_jedi 0 / 217, mb2_manaan 0 / 3494, t3_byss 25 / 28 898, taspir1 168 / 22 002, duel_mos_espa 264 / 246 106, kejim_post 691 / 310 752 |
| Bytes gegen NRC (`-threads 1`) | anders auf t3_byss, taspir1, kejim_post (dort sind auch Portal-Ebenen berichtigt); gleich auf ctf3, md_ch_ben, md_fh1_jedi, mb2_manaan, duel_mos_espa | anders auf denselben drei Karten (mit anderen Bytes als R2); gleich auf ctf3, md_ch_ben, md_fh1_jedi, mb2_manaan und duel_mos_espa (dort 264 + 163 357 Ebenen ausgelassen, dasselbe Ergebnis) |
| Dichte gegen NRC (`-threads 1`; kejim_post: feste Regel) | t3_byss +11 Paare (+0,0008 %), taspir1 +12 (+0,0011 %), kejim_post +228 (+0,0056 %) | t3_byss +10 Paare, taspir1 +9 (gegen R2 ein Paar und drei Paare weniger), kejim_post +343 (+0,0084 %; gegen R2 115 mehr) |
| Bytes mit der festen Regel | anders auf ctf3, t3_byss, taspir1, duel_mos_espa, kejim_post (die Uhr der Regel verschiebt sich mit jeder ausgelassenen Ebene) | dieselben fünf Karten (ctf3 mit denselben Bytes wie R2, die anderen vier mit anderen) |
| Tempo | Abschnitt 7: unter Last rund 2 bis 4 % mehr CPU-Zeit als vor vis4 (ein Punkt-Test je angenommener Trennebene), die Arbeit bleibt | 2 bis 5 Prozentpunkte darüber (drei Punkt-Tests mehr je Ebene) |

**Empfehlung: R2 als Vorgabe lassen, R1 hinter dem Schalter.** R2 ist genau NRCs eigener Maßstab, auf alle Punkte
angewendet statt auf den ersten – es berichtigt den Fehler und sonst nichts. R1 verwirft ein Vielfaches an
Ebenen (in den Flüssen das Drei- bis Hundertfache), ohne dass eine Lücke es verlangt; es ist die Versicherung gegen einen Fall, der sich nicht zeigen ließ
(eine Ebene, die bei sehr großen Portalen ihre eigenen Punkte um viele Einheiten verfehlt und trotzdem alle anderen
Punkte auf der richtigen Seite hat). Für beide gilt: auf den Karten, die sich mit `-threads 1` vergleichen lassen, ändert sich das, was das Spiel
sieht, um höchstens 29 Cluster-Paare von 1,3 Millionen. Wer R1 will: `-bhedvisstrictplanes` zur Vorgabe machen heißt eine Zeile in `VisMain`.

## 7. Tempo (Punkt 5)

Die Portal-Ebenen kosten nichts (einmal je Portal beim Laden). Die Trennebenen kosten je angenommener Ebene den Test
der Punkte der Quelle hinter dem, der NRC die Seite gab: bei drei Punkten nichts, bei vier ein Punktprodukt, sonst
eines je übrigem Punkt (`BhedSeperates`; Schalter und Zählen liegen in `BhedSeperatesRare` außerhalb des heißen
Wegs). Geprüft wird nur eine Ebene, die NRCs Code schon angenommen hat – nicht jeder Versuch.

**Unter Last gemessen (Richtwerte)**: Messfenster 20:59 (Sperre; daneben liefen 99 fremde Bau-/Testprozesse,
Fremdlast 12 bis 26 Kerne), feste Regel, 30 Threads, alt (38488e148) / neu (Endstand) / neu mit `-bhedvisnrcplanes` /
neu mit `-bhedvisstrictplanes` in wechselnder Reihenfolge; Median der CPU-Sekunden, in Klammern gegen alt:

| Karte | Läufe je Fassung | alt | neu (Vorgabe) | nrc | streng | Rahmen alt → neu (Mio.) | Wand alt → neu (s) |
|---|---|---|---|---|---|---|---|
| ctf3 | 4 | 17,1 | 16,2 (0,950) | 16,6 (0,974) | 17,6 (1,035) | 10,72 → 10,74 | 1,41 → 1,48 |
| md_ch_ben | 4 | 45,8 | 46,5 (1,016) | 46,5 (1,014) | 47,8 (1,043) | 56,37 → 56,37 | 2,64 → 2,71 |
| mb2_manaan | 2 | 187,9 | 196,8 (1,048) | 195,7 (1,042) | 205,4 (1,094) | 177,29 → 177,29 | 11,64 → 12,37 |
| t3_byss | 2 | 295,1 | 306,8 (1,040) | 307,6 (1,042) | 312,8 (1,060) | 200,21 → 201,44 | 17,93 → 18,23 |
| duel_mos_espa | 1 | 1072,7 | 1067,9 (0,996) | – | – | 713,25 → 711,00 | 138,1 → 117,9 (neu lief zuerst) |

- **Arbeit**: die Flüsse brauchen −0,3 bis +0,6 % Rahmen, wo sich das Ergebnis ändert, sonst genau dieselbe Zahl.
- **CPU-Zeit**: die Vorgabe lag zwischen 5 % unter und 5 % über dem Stand vor vis4; auf den drei Karten mit gleicher
  Arbeit und mehreren Runden +1,6 bis +4,8 %. Der Punkt-Test ist nicht umsonst: **rund 2 bis 4 % CPU-Zeit** ist der
  ehrliche Richtwert, einzelne Läufe streuen um ±5 %. `nrc` kostet dasselbe wie die Vorgabe (der Test läuft vor dem
  Schalter), die strenge Regel 2 bis 5 Prozentpunkte mehr. Zum Vergleich: vis2s hatte dem Fluss 22 bis 39 % CPU-Zeit
  abgenommen.
- **Wandzeit**: innerhalb der Streuung.
- Zwei frühere Fenster (20:04 und 20:22; die Fassung vor dem schlanken heißen Weg, alt in jeder Runde zuerst)
  zeigten die Vorgabe bei +4 bis +10 % – und alt auch vor `nrc`, das dort dieselben Befehle ausführte. Der erste Lauf
  nach einer Pause ist auf diesem Rechner schneller: duel_mos_espa brauchte als erster Lauf 85 und 66 s (alt), als
  zweiter 118 und 98 s (neu); im dritten Fenster lief neu zuerst (118 s) und alt danach (138 s), bei gleicher
  CPU-Zeit (1068 und 1073 s). Deshalb wechselt die Reihenfolge in `liste_final.txt` jetzt von Runde zu Runde.
  Alle Läufe: `Y:\werkzeuge\vis4\messung_unter_last.txt`.
- Billiger geht der Test nicht, ohne zu raten: ab wann der erste Punkt verlässlich ist, wüsste man nur aus der
  Kondition der Ebene, und die zu rechnen kostet mehr als der Test.
- Entscheidend bleibt die Messung im Ruhefenster: `sh /y/werkzeuge/vis4/messung_final.sh` (etwa 8 min, ein
  Messfenster) → `messung_final.txt`.

## 8. Tests

- ctest **q3map2_visecken** (neu, `tools/karte/q3map2_visecken.py`, 33 Prüfungen, 4 s): die Testkarte aus drei
  Ketten – Gegenprobe der Geraden ohne das Programm; mit `-bhedvisnrcplanes` der Lump von NRCs q3map2.exe mit den
  Lücken (`-vis`, `-nopassage`, `-passageOnly`), die Prüfung meldet 647 Portale und 4 Ebenen; die Vorgabe zeigt in
  jeder Kette jedes Blatt (1 und 8 Threads, `-nopassage`, `-passageOnly`, `-bhedvisfixed`, `-bhedvisexact`,
  `-bhedvisstrictplanes`), 2078 Sichtlinien ohne Lücke; Zwischenstand und gemerktes VIS mit NRCs Ebenen gelten nicht
  für die Vorgabe. Rot mit der exe vor vis4: 22 FEHLER (`log\visecken_rot.txt`), grün mit dem Endstand.
- ctest, Auswahl `^q3map2_|^karte_grenzen$` (16 Tests, exe = Endstand): **grün** q3map2_visecken, q3map2_visgleich
  (die Vorgabe gibt auf der Gitterkarte NRCs Lumps: wo NRC richtig rechnet, ändert sich nichts), q3map2_visfest,
  q3map2_vispause, q3map2_determinismus, karte_grenzen (Prüfstelle `vis.cpp:825`, war 759; `grenzen.json` neu),
  q3map2_vergleich, q3map2_pr316, q3map2_gpu, q3map2_gpubake, q3map2_funde, q3map2_pfade, q3map2_uebersetzer,
  q3map2_zusammen, q3map2_importe. **q3map2_teilbau rot wie auf der Basis** (dieselben 2 Fehlschläge im Licht-Cache:
  „-bounce 2, Brush (Detail in Raum B): 9/24, 7/24, 7/24“ und „Shader (Wand in Raum A): 18/18, 18/18, 18/18“,
  104 Zeilen OK – gehört LCB2; im Lauf der ganzen Gruppe lief er neben rund 100 fremden Bau- und Testprozessen in
  seine Zeitgrenze von 1800 s, allein danach 394 s mit genau diesen zwei). Unter dieser Last brauchten q3map2_visfest
  und q3map2_vispause 773 und 788 s von 900, q3map2_determinismus 1514 s – ohne Last 106, 120 und 283 s.
- Bau: `bhed_q3map2_clang`, `kartegrenzen` ohne Warnung. Die Reihen aus Abschnitt 5 und 6 liefen mit der Fassung vor
  dem schlanken heißen Weg (dieselben Entscheidungen); der Endstand gibt dieselben Bytes (ctf3 und t3_byss mit der
  festen Regel in allen drei Rechnungen, die Testkarte; `neu\bhed_q3map2.exe` = Endstand).
- Lints: lint_sources ok, lint_headers ok, lint_fassung ok; lint_portability 99, lint_pruefcodes 11,
  lint_fremdprogramm 1 – dieselben Zahlen wie vor vis4, keine Meldung in einer geänderten Datei.
  `gen_handbuch.py --pruefen` (handbuch_sprachen): ok, es und cs ohne englische Reste.
- Handbuch: Absatz „Portal corners written twice“ auf der Seite `karte_teilbau`, sechs Sprachen.
- Bewusst nicht gelaufen (Regel „Schlussrunde je Zweig“): die übrigen ctests, alle Selbsttest-Gruppen, karteneditor,
  Modus 1, voller Neubau – geändert sind `visflow.cpp`, `vis.cpp`, `vis.h`, `visflow.h`, `help.cpp`,
  `bhed_vischeck.cpp/.h`, eine Zeile in `src/karte/grenzen.cpp` samt `grenzen.json`, das neue Prüfskript,
  CMakeLists.txt (ein Test) und ein Absatz im Handbuch. Die Oberfläche kennt die Schalter nicht (und braucht sie
  nicht: die Korrektur ist die Vorgabe).

## 9. Offen

- **Entscheidung shank**: R2 (Vorgabe) oder R1 (Abschnitt 6).
- **Der BSP-Schritt schreibt die doppelten Ecken** (und dieselbe Ecke in zwei Portalen mit anderen Ziffern). Das
  VIS kommt jetzt damit zurecht; an der Quelle beheben hieße die .prt und damit jede .bsp gegenüber NRC ändern –
  eigener Auftrag, wenn gewünscht.
- kejim_post mit `-threads 1` gegen NRC ist nicht gelaufen (Stunden); der Vergleich dort ist die feste Regel.
- Endgültige Zeiten im Ruhefenster: `sh /y/werkzeuge/vis4/messung_final.sh` (etwa 8 min).
- `-nopassage` hat keinen Geometrie-Merker (vis2s) – unverändert; die Korrektur gilt dort ebenso (ctest).
- Die Prüfung findet nur, was auf ihren Linien liegt (256 je Portal, 1 Einheit Abstand von Kanten und Ebenen).

## Changelog

## English

- Map compiler: the VIS no longer hides what you can see where a portal has a corner written twice, some thousandths of a unit apart (the BSP step writes such corners, mostly at slanted faces). q3map2 has three faults there since the first q3map, and NetRadiant-Custom has them too: the plane of a portal is made from its first three points - turned if two of them are nearly the same point, missing if the three are on one line, and then the portal sees nothing -, and a separating plane between two portals takes the side of the portal from its first point alone. In a straight corridor of three portals NetRadiant-Custom's q3map2 makes the room at the end invisible. bhed_q3map2 takes all points; where q3map2 is right, the result is the same. `-vis -bhedvisnrcplanes` computes as NetRadiant-Custom does, `-vis -bhedvisstrictplanes` also leaves out separating planes that miss their own three points.
- Map compiler: `-vis -bhedvischeck` judges by planes of its own, also draws lines to the portals nearby whatever the flood says, and counts the portal planes that miss their portal.

## Deutsch

- Karten-Compiler: das VIS verdeckt nicht mehr, was man sieht, wo ein Portal eine Ecke zweimal hat, wenige Tausendstel auseinander (der BSP-Schritt schreibt solche Ecken, meist an schrägen Flächen). q3map2 hat dort seit dem ersten q3map drei Fehler, NetRadiant-Custom ebenso: die Ebene eines Portals entsteht aus seinen ersten drei Punkten – verdreht, wenn zwei davon fast derselbe Punkt sind, gar keine, wenn die drei auf einer Geraden liegen, dann sieht das Portal nichts –, und eine Trennebene zwischen zwei Portalen nimmt die Seite des Portals von dessen erstem Punkt allein. In einem geraden Gang aus drei Portalen macht das q3map2 von NetRadiant-Custom den Raum am Ende unsichtbar. bhed_q3map2 nimmt alle Punkte; wo q3map2 richtig rechnet, ist das Ergebnis dasselbe. `-vis -bhedvisnrcplanes` rechnet wie NetRadiant-Custom, `-vis -bhedvisstrictplanes` lässt zusätzlich Trennebenen aus, die ihre eigenen drei Punkte verfehlen.
- Karten-Compiler: `-vis -bhedvischeck` urteilt mit eigenen Ebenen, zieht Linien auch zu den Portalen in der Nähe, was immer die Flutung sagt, und zählt die Portal-Ebenen, die ihr Portal verfehlen.

## 中文

- 地图编译器:当门户的一个顶点被写了两次、相距千分之几个单位时(BSP 步骤会写出这样的顶点,多在倾斜的面上),VIS 不再遮掉本来看得见的东西。从最早的 q3map 起,q3map2 在这里有三个错误,NetRadiant-Custom 也一样:门户的平面由它的前三个点求出——其中两个点几乎重合时平面是歪的,三个点共线时根本没有平面,这个门户就什么也看不见——;两个门户之间的分隔平面只按门户的第一个点判断门户在哪一侧。在一条由三个门户组成的笔直走廊里,NetRadiant-Custom 的 q3map2 会让尽头的房间不可见。bhed_q3map2 使用全部的点;在 q3map2 算得正确的地方,结果相同。`-vis -bhedvisnrcplanes` 按 NetRadiant-Custom 的方式计算,`-vis -bhedvisstrictplanes` 还会舍弃偏离自身三个点的分隔平面。
- 地图编译器:`-vis -bhedvischeck` 用自己求出的平面来判断,无论洪泛结果如何也会向附近的门户画线,并统计偏离门户的门户平面数量。

## 日本語

- マップコンパイラー: ポータルの角が千分の数ユニットだけ離れて二重に書かれているところ(BSP ステップが、主に斜めの面で書き出します)でも、VIS が見えるはずのものを隠さなくなりました。q3map2 には最初の q3map 以来ここに 3 つの誤りがあり、NetRadiant-Custom も同じです。ポータルの平面を最初の 3 点から作るため、そのうち 2 点がほぼ同じ点だと平面が傾き、3 点が一直線上にあると平面ができず、そのポータルは何も見えなくなります。また、2 つのポータルの間の分離平面は、ポータルがどちら側にあるかを最初の 1 点だけで決めます。3 つのポータルが並ぶまっすぐな通路では、NetRadiant-Custom の q3map2 は突き当たりの部屋を見えなくしてしまいます。bhed_q3map2 はすべての点を使います。q3map2 が正しく計算しているところでは結果は同じです。`-vis -bhedvisnrcplanes` は NetRadiant-Custom と同じように計算し、`-vis -bhedvisstrictplanes` は自身の 3 点から外れている分離平面も除外します。
- マップコンパイラー: `-vis -bhedvischeck` は独自の平面で判定し、フラッドの結果にかかわらず近くのポータルにも線を引き、ポータルから外れているポータル平面の数を数えます。

## Español

- Compilador de mapas: el VIS ya no oculta lo que se ve allí donde un portal tiene una esquina escrita dos veces, separadas por unas milésimas de unidad (el paso BSP escribe esas esquinas, sobre todo en caras inclinadas). q3map2 tiene ahí tres fallos desde el primer q3map, y NetRadiant-Custom también: el plano de un portal se calcula con sus tres primeros puntos –girado si dos de ellos son casi el mismo punto, inexistente si los tres están en una recta, y entonces el portal no ve nada–, y un plano separador entre dos portales toma el lado del portal solo de su primer punto. En un pasillo recto de tres portales, el q3map2 de NetRadiant-Custom hace invisible la sala del fondo. bhed_q3map2 usa todos los puntos; donde q3map2 calcula bien, el resultado es el mismo. `-vis -bhedvisnrcplanes` calcula como NetRadiant-Custom, y `-vis -bhedvisstrictplanes` descarta además los planos separadores que no cumplen con sus propios tres puntos.
- Compilador de mapas: `-vis -bhedvischeck` juzga con planos propios, traza también líneas hacia los portales cercanos diga lo que diga la inundación y cuenta los planos de portal que no cumplen con su portal.

## Čeština

- Kompilátor map: VIS už neskrývá to, co je vidět, tam, kde má portál roh zapsaný dvakrát, pár tisícin jednotky od sebe (takové rohy zapisuje krok BSP, většinou u šikmých ploch). q3map2 tam má už od prvního q3map tři chyby a NetRadiant-Custom také: rovina portálu se počítá z jeho prvních tří bodů - je pootočená, když jsou dva z nich skoro tentýž bod, a nevznikne vůbec, když všechny tři leží na jedné přímce, a portál pak nevidí nic -, a dělicí rovina mezi dvěma portály bere stranu portálu jen podle jeho prvního bodu. V rovné chodbě ze tří portálů udělá q3map2 z NetRadiant-Custom místnost na konci neviditelnou. bhed_q3map2 bere všechny body; kde q3map2 počítá správně, je výsledek stejný. `-vis -bhedvisnrcplanes` počítá jako NetRadiant-Custom, `-vis -bhedvisstrictplanes` navíc vynechá dělicí roviny, které nesplňují své vlastní tři body.
- Kompilátor map: `-vis -bhedvischeck` posuzuje podle vlastních rovin, vede přímky i k portálům poblíž bez ohledu na zaplavení a počítá roviny portálů, které svůj portál nesplňují.
