# Notizen TESTTEMPO (11.11., Zweig p_test ab a00f6cf90)

Auftrag: Tests schneller bei gleichem Ergebnis. Messung und Vorschlaege: `docs/intern/tests_tempo.md`. Hier: was
umgesetzt ist, mit Nachweis. Alle Zeiten sind Richtwerte unter Last (rund zehn Agenten bauten und testeten daneben;
ein Ruhefenster gab es nicht) - verglichen wird deshalb immer nebeneinander im selben Fenster.

## Ergebnis in Zahlen (volle Runde, gerechnet mit den Laufzeiten der Runde rc611)

| | Laeufe | Rechenzeit | laengste Kette (Wanduhr mindestens) |
|---|---|---|---|
| Runde rc611 (`Y:\werkzeuge\rel611\pool.py` + Bildlaeufe) | 193 | 31623 s | 79 min (drei karteneditor nacheinander) |
| ohne Doppel (26 Laeufe der Runde rc611, belegt enthalten) | | - 2538 s | |
| ohne Bildlaeufe bm (6, nur bei Bildaufnahme) | | - 3437 s | |
| ohne Sprachlaeufe (15, Entscheidung shank) | | - 9241 s | 37 min (karteneditor en) |
| `python -I tools/testrunde.py` | 141 | rund 16400 s (-48 %) | 37 min |

Dazu, nicht in der Tabelle: normale Prioritaet statt Leerlauf (neben Bauten 6-10x schneller je Lauf, gemessen),
Pausen nach Bildern fuer menueeditor und sechs weitere Laeufe (menueeditor nebeneinander gemessen -30 %), ctest
`defgpu` in zwei Teilen (laengster Fall halbiert). Eine volle Runde mit dem neuen Befehl ist NICHT gefahren (rund
zehn Agenten belasteten den Rechner; gelaufen sind die Laeufe dieser Notizen) - die Endzahl im Ruhefenster ist offen.

## 1. Eine Runde, ein Befehl: `tools/testrunde.py`

- Ersetzt die Kopien `Y:\werkzeuge\*\lauf.py`, `pool.py`, `schluss*.sh`: Tabelle aller Laeufe mit ihren
  Umgebungsvariablen (BHED_MB_BELEGE, BHED_RT_GLA, BHED_MB7_*, Sprache ja fuer ptfix2, JKA-Ordner `GameData` bzw.
  `GameData original` - der, den es gibt - ueber BHED_JKA_BASIS, BHED_MB_JKA, BHED_KEMD_JA, BHED_JA_PFAD), Pool
  (Vorgabe 6), eigene Laufordner (`<build>/laeufe`), 8-GB- und Zeitgrenze, Messsperre (`Y:\werkzeuge\MESSUNG_LAEUFT.txt`
  juenger als 10 min: kein neuer Lauf startet).
- **Haengende Laeufe:** 180 s ohne Protokollzeile UND ohne 1 s Rechenzeit -> eigene PID samt Kindern beendet, Zeile
  `HAENGT`. (Ein Lauf, der rechnet, aber nichts schreibt - Rundlaeufe -, zaehlt nicht als haengend.)
- **Eine Zeile je Lauf**, FEHLER-Zeilen darunter, Summe in `<laeufe>/<praefix>summe.txt`, letzte Zeile `RUNDE: ...`.
- `--liste` zeigt, was laeuft und was warum wegbleibt (doppelt / Sprachlauf / allein); `--mit-doppelten`,
  `--mit-sprachen`; `--betroffen <basis>`; Laeufe beim Namen (auch die, die sonst wegbleiben).
- Nachweis-Helfer: `--namen DATEI` (alle Pruefzeilen der Runde), `--vergleich A B [--ziffern]`, `--pruefe-doppel`.
- Nicht im Pool (Spalte `allein`): `update`, `umzug` (eigene Vorbereitung), `leistung`, `tabfensterleistung`
  (Messungen), `bm_*` (Bildlaeufe). Lints laufen ueber ctest bzw. von Hand wie bisher.
- Gegenprobe des Werkzeugs: drei Laeufe mit der exe von rc611 (`lesezeichen` 18/0, `geteilt` 43/0, `mb_haut` 1/0),
  danach alle Laeufe dieser Notizen.

## 2. Normale Prioritaet statt Leerlauf

`lauf.py` startete jeden Selbsttest mit Leerlauf-Prioritaet. Solange andere bauen, bekommt ein solcher Prozess kaum
Rechenzeit. Gemessen 11.11. ~12:45, dieselbe exe, dieselbe Last:

| Lauf | Leerlauf | normal | Bilder | je Bild |
|---|---|---|---|---|
| `geteilt` | 155 s | 16 s | 1213 | 94 ms -> 7,8 ms |
| `mb_mb2` | 208 s | 31 s | 533 / 1090 | 252 ms -> 23 ms |

Gleiche Pruefzeilen, gleiche Ergebnisse. shank hat normale Prioritaet am 03.11. erlaubt; `tools/testrunde.py` nimmt
sie als Vorgabe (`--leerlauf` stellt das alte Verhalten her). Die Wackler unter Last (tests_tempo.md) entstehen genau
in diesem Aushungern: Schritte mit festen Zeiten oder Bildzahlen bekommen zu wenige Bilder.

## 3. Doppel aus der vollen Runde

27 Laeufe, Tabelle und Gruende in tests_tempo.md, Spalte `doppel` in `tools/testrunde.py`. Nachweis je Lauf: jede
seiner Pruefzeilen steht woertlich im genannten groesseren Lauf (`--pruefe-doppel`):

- an den Protokollen der Runde rc611 (`--pruefe-doppel --laeufe Y:\karte\laeufe --praefix rel611_`): licht1 15/15,
  modobj 25/25, r2a 22/22 in karteneditor; kuerzel_es 202/202 in 1_es, kuerzel_cs 202/202 in 1_cs; editorportal_tester
  4/4 in editorportal; kartenfunde 54/54, fix604b 33/33, geschuetzanim 3/3, zielstrahl 2/2, effektgrund 1/1,
  figurdeform 1/1, figurstufen 8/8 in gegenstaende; kamera 96/96, gluehen 9/9, npcklasse 13/13 in karte; mp und sp je
  2629/2629 in 1_es (`enthalten.py`; der Ordner rel611_1 ist ein spaeterer Teillauf mit 252 Zeilen).
- am eigenen Bau (11.11., `--praefix g2w_`): kuerzel 202/202, ziehen 194/194, fenster 78/78, zeitleiste 47/47, ton 34/34,
  szenen 26/26, geteilt 43/43, lesezeichen 18/18 in 1.
- mb_ohnenur 97/97 in mb_funktionen + mb_tasten (exe rc611).

Gegenprobe mit eingebautem Fehler: Rot-Stand `Y:\werkzeuge\tt\rot.py` (F2 "naechstes Lesezeichen" springt
rueckwaerts). Der weggelassene Lauf `lesezeichen` wird rot (14 OK, 4 FEHLER: "F2 springt zum ersten Lesezeichen",
"F2 springt zum naechsten", "F2 am Ende beginnt wieder oben", "Umschalt+F2 am Anfang springt ans Ende"), der Lauf `1`,
der bleibt, wird mit denselben vier Zeilen rot (2631 OK, 4 FEHLER). Gruen sind beide am Stand ohne den Fehler.

## 4. Sprachlaeufe (Entscheidung shank, kein Doppel)

Siehe tests_tempo.md, Abschnitt "Entscheidung: volle Runde nur englisch": 17 Laeufe nur noch mit `--mit-sprachen`,
dort auch die Liste der Sprachpruefungen, die bleiben. Hier wird bewusst weniger geprueft als bis rc611.

## 5. Pausen nach Bildern (`BHED_TEST_PAUSEN=bild`)

- Befund: 34039 Pausen je Runde (`pause(sekunden)`, 4459 Stellen), Wanduhr. Ohne Last sind sie der groesste Posten der
  Bedien-Laeufe, unter Last dauern sie gleich lang, waehrend die Bilder teurer werden.
- Umsetzung (`gui/selbsttest.cpp`, `pause` und `selbsttestVorBild`, zusammen ein Block): mit der Variable rueckt die
  ImGui-Zeit in jedem Bild einer Pause um mindestens 1/20 s vor; die Pause endet, wenn die ImGui-Zeit um `sekunden`
  weiter ist UND ein Viertel davon wirklich vergangen ist - spaetestens aber wie bisher nach `sekunden` Wanduhr. Alles,
  was an der ImGui-Zeit haengt (Doppelklick-Frist, Tooltips, Tastenwiederholung, Animationen ueber DeltaTime), sieht
  dieselbe Zeit wie bisher; die Pause dauert nie laenger als bisher. **Ohne die Variable ist nichts anders.**
- Grenze: Programmteile, die die Wanduhr selbst lesen (std::chrono, rund 150 Stellen) oder im Hintergrund rechnen,
  bekommen in einer solchen Pause weniger echte Zeit (mindestens ein Viertel). Deshalb gilt die Variable nur fuer
  Laeufe, die damit nachweislich dieselben Pruefzeilen mit demselben Stand liefern (Spalte `pausen` der Tabelle);
  alle anderen laufen wie bisher. Wer eine Gruppe aufnimmt: beide Arten nebeneinander fahren
  (`--var BHED_TEST_PAUSEN=bild`), `--namen` und `--vergleich --ziffern`, dazu ein Rot-Stand.
- Dazu im Protokoll jedes Laufs (immer): `Knopftest Zeiten: gesamt ... s, ... Bilder | Pause ...x ... s | Warte ... |
  Foto ... | sonst ...`; mit `BHED_TEST_ZEITEN=1` die zehn laengsten Schritte. `tools/testrunde.py` nennt die
  Pausenzeit je Lauf.

Nachweis:

Jeweils derselbe Lauf zweimal nebeneinander (`Y:\werkzeuge\tt\pruef.sh`: `w_` Wanduhr, `b_` nach Bildern), danach
`--namen` beider Seiten und `--vergleich --ziffern`.

| Welle | Laeufe | Pruefzeilen links / rechts | nur links / nur rechts / anderer Stand | Summe Wanduhr -> nach Bildern |
|---|---|---|---|---|
| 1: Leerlauf-Prioritaet, 1/60 s | kuerzel, lesezeichen, mb5, funktionen, tasten, fenster, geteilt, mb2 | 599 / 599 | 0 / 0 / 0 | 2207 -> 2167 s |
| 2: normale Prioritaet, 1/60 s | menueeditor, 1, kuerzel, mb5, funktionen, fenster, lesezeichen, geteilt, ziehen, zeitleiste, ton, szenen | 5038 / 5038 | 0 / 0 / 0 | 4880 -> 3858 s |
| 3: normal, 1/20 s (Endstand) | kuerzel, mb5, funktionen, tasten, mb2, menueloeschen, missionsbau, karte | 893 / 893 | 0 / 0 / **2** | 1352 -> 1184 s |
| 4: normal, 1/20 s, nur `b_` gegen Welle 2 `w_` | 1 (2413 Zeilen), menueeditor (1834 Zeilen) | gleich | 0 / 0 / 0 | - |

Einzelne Laeufe (Laufzeit, in Klammern die Pausen):

| Lauf | Wanduhr | nach Bildern | |
|---|---|---|---|
| menueeditor (Welle 2, 1/60 s) | 2673 s (871 s) | 1865 s (495 s) | -30 % |
| menueeditor (Welle 4, 1/20 s, anderes Fenster) | | 875 s (206 s) | |
| 1 (Welle 2, 1/60 s) | 1098 s (209 s) | 944 s (110 s) | -14 % |
| kuerzel (Welle 3) | 143 s (86 s) | 102 s (38 s) | -29 % |
| mb_mb5 (Welle 3) | 202 s (81 s) | 168 s (43 s) | -17 % |
| menueloeschen (Welle 3) | 118 s (42 s) | 95 s (16 s) | -19 % |
| missionsbau (Welle 3) | 268 s (50 s) | 250 s (22 s) | -7 % |
| mb_funktionen (Welle 3) | 65 s (16 s) | 56 s (10 s) | -14 % |

- Unter Leerlauf-Prioritaet neben Bauten (Welle 1) bringt die Umstellung fast nichts: ein Bild kostet dann 100-250 ms,
  und die Pause endet wie bisher nach der Wanduhr. Der Gewinn kommt mit normaler Prioritaet.
- **`karte` bleibt bei der Wanduhr** (Welle 3: zwei Pruefungen werden rot): "Ruhe: ohne Eingabe hoechstens wenige Bilder
  je Sekunde" und "Spulen: ... zurueck so fluessig wie vor" messen die Wanduhr ueber eine `pause(2.0)`. Genau dafuer ist
  der Nachweis je Lauf da; die Karten-/Vorschau-Modi und alles nicht Nachgewiesene laufen unveraendert.
- **Modus `1` bleibt bei der Wanduhr:** gleich in Welle 2 und 4 (2635 OK), aber im Rot-Stand-Lauf nach Bildern kamen
  zu den vier erwarteten Zeilen zwei weitere: `Klick "Close": Klick kam nicht an (... Fenster Tutorials###tour)` und
  `... beim Loslassen nicht mehr unter der Maus` - das Tutorial-Fenster wanderte noch, als geklickt wurde (1 von 3
  Laeufen nach Bildern). Ein Zeitrennen, das die kuerzere Pause sichtbar macht; die Stelle braucht eine Bedingung
  ("Fenster steht") statt der Pause, bevor Modus 1 umgestellt wird. Offen.
- **Umgestellt (Spalte `pausen` in `tools/testrunde.py`):** menueeditor, menueloeschen, missionsbau, mb_funktionen,
  mb_tasten, mb_mb2, mb_mb5.
- **Rueckfall:** wird ein umgestellter Lauf rot, wiederholt `tools/testrunde.py` ihn sofort nach der Wanduhr; es zaehlt
  die Wiederholung, und die Zeile nennt, was nur nach Bildern rot war (Protokoll des ersten Laufs:
  `<praefix><lauf>_nach_bildern-detail.log`). So kann ein Wackler der neuen Pausen die Runde nicht faerben und bleibt
  doch sichtbar. Probiert mit dem Rot-Stand: `mb_mb5` nach Bildern rot (156 OK, 3 FEHLER), sofort nach der Wanduhr wiederholt, wieder rot mit denselben Zeilen, die Zeile nennt beides; `mb_mb2` daneben gruen.
- Gegenprobe mit eingebautem Fehler (Rot-Stand `rot.py`, beide Pausenarten nebeneinander): `mb_mb5` (Dopesheet: ein
  gezogener Schluessel landet ein Bild daneben) wird in beiden Arten mit denselben drei Zeilen rot (156 OK, 3 FEHLER:
  "MB5 Kurveneditor: der Wert stieg um 40 Bildpunkte ...", "... der gezogene Vektor-Henkel ist Frei ...", "... ein
  gezogener Auto-Henkel wird Ausgerichtet ..."). `menueeditor` (Strg+Z tut nichts) wird in beiden Arten rot mit denselben Zeilen (2492 OK, 301 FEHLER; Vergleich der Rot-Runde: 4410 / 4412 Pruefzeilen, anderer Stand 0, nur rechts 2 - die zwei Zeilen aus Modus 1 oben); nebeneinander 1437 s (Pausen 653 s) nach der Wanduhr, 1051 s (221 s) nach Bildern.
- Wackler in Welle 2 (kein Unterschied der Pausenarten): `zeitleiste` allein war in beiden Arten gleichzeitig rot
  ("Ton: ... der Klang kommt, sobald sie gezeichnet wird (aufgeschoben, nicht verschluckt)", 46 OK, 1 FEHLER), waehrend
  daneben der volle Bau lief; allein wiederholt gruen (47 OK) mit der neuen exe und mit der von rc611, in Modus 1 beide
  Male gruen.

## 6. ctest: `defgpu` in zwei Teilen

`defgpu` war der laengste ctest-Fall und bestimmte die Wanduhr von ctest (170-422 s). `defgpu_probe warp` rechnet auf
WARP (und die Gegenproben ohne Geraet), `defgpu_probe grafikkarte` auf der Grafikkarte; ctest faehrt beide
nebeneinander (`defgpu`, `defgpu_grafikkarte`). Die Eingaben sind dieselben (kein Zufallswert entsteht in einer
Geraeteschleife). Nachweis im selben Fenster: `defgpu_probe` ohne Angabe 204 Zeilen in 755 s; `warp` + `grafikkarte`
zusammen dieselben 204 Zeilen (sortiert verglichen, 0 Unterschiede, 0 FEHL) in 377 s und 397 s nebeneinander.

## 7. `tools/test_betroffen.py`

Aus `git diff <basis>..HEAD` (und dem Arbeitsbaum) die Bereiche der Tabelle (programm, vorschau, karteneditor, menue,
modell, dock, update, handbuch); eine Datei ohne Regel heisst ALLES. Sammeldateien (CMakeLists.txt, i18n.h/.cpp,
hilfe_orte.inc, changelog.cpp) und `gui/selbsttest.cpp` zaehlen nicht, wenn nur Zeilen dazukommen (neue Texte, neue
Tests, Einbinden und Schalter einer neuen Gruppe). Nur lesend an den Zweigen vom 11.11. probiert: p_mbmal -> handbuch,
modell; p_vis2 -> handbuch, karteneditor; p_vis2s -> karteneditor; p_uiform, p_dx12, p_mbnav, p_mbvers, p_numpad -> ALLES
(Kern-Dateien bzw. geaenderte bestehende Zeilen). `tools/testrunde.py --betroffen <basis>` faehrt die Bereiche.

## Nicht umgesetzt (mit Grund)

- **Gruppen in einem Programmstart zusammenlegen:** Start und Kopieren sind 2,4 % der Runde (gemessen); der Gewinn
  waere hoechstens das, das Risiko (jede Gruppe muesste ihren Ausgangszustand selbst herstellen) traefe 50 Laeufe.
- **karteneditor in Teilen nebeneinander / eigener Portbereich:** mit der Entscheidung "nur englisch" entfaellt die
  Kette; der eine volle Lauf (37 min) bleibt die laengste Strecke der Runde.
- **Rundlaeufe des Modell-Blocks und kartenpruefung auf mehrere Prozesse** (bild2_md 1422 s, kr_md 1061 s,
  kartenpruefung 1481 s, mission1 1495 s, allekarten 1278 s): groesster verbleibender Posten, braucht je Gruppe eine
  Aufteilung der Modelle/Karten (z. B. BHED_MB_TEIL=i/n) - offen.
- **Pausen nach Bildern fuer karteneditor und die uebrigen Modi:** nicht nachgewiesen (der volle karteneditor braucht
  die festen Ports und 37 min je Lauf); sie laufen wie bisher.

(Stand von Runde 1. Die letzten drei Punkte hat Runde 2 aufgenommen - siehe unten R2 und R3.)

## Schlussrunde

- Bau `Y:/p_test/build` (Release, /W4 /WX): 0 Warnungen (`Y:\werkzeuge\tt\bau2.log` Programm, `bau3.log` alles,
  `bau5.log` Endstand nach dem Rot-Stand).
- ctest `-j 16` (`ctest1.log`): 246 von 247 bestanden. Rot: `q3map2_teilbau` (2 Fehlschlaege: "zweiraum, -bounce 2,
  Brush (Detail in Raum B)" und "Shader (Wand in Raum A)") - der Stand der Basis a00f6cf90 ("WIP lcb2 1: ... (red)"),
  gleich in `Y:\werkzeuge\mbm\ctest.log` und `mbv\ctest.log`; dieser Zweig aendert q3map2 nicht. `defgpu` und
  `defgpu_grafikkarte` bestanden. Nach dem ctest-Lauf aenderte sich nur noch die Konstante der Pause (1/60 -> 1/20 s).
- Selbsttests am Endstand (exe `Y:\werkzeuge\tt\final_bin`): die Laeufe der Wellen 3 und 4 oben (Wanduhr-Seite =
  Vorgabe, alle gruen), `zeitleiste` allein gruen. Nicht gelaufen: karteneditor, die Karten-Modi ausser karte und
  missionsbau, die uebrigen Modell-Block-Gruppen, Docks - der Code dieses Zweigs aendert fuer sie nichts (ohne
  BHED_TEST_PAUSEN laeuft `pause` wie vorher; dazu kommt nur die Zeile "Knopftest Zeiten").
- Lints: nur neue Python-Werkzeuge und Doku; `git ls-files --eol` LF.

## Changelog

Kein Eintrag (nur Testwerkzeuge, das Programm verhaelt sich unveraendert).

# Runde 2 (11.11. nachmittags und abends)

Auftrag des Hauptchats nach Runde 1: Modus 1 und karteneditor auf Pausen nach Bildern (zuerst die Stellen, die an der
Wanduhr haengen), der karteneditor in Teilen nebeneinander mit eigenem Portbereich je Instanz, lange Schleifen ueber
Spieldaten auf mehrere Prozesse, die Wackler, die andere Agenten gemeldet haben, die Haengt-Erkennung proben, der Ort der
grossen Karte einstellbar. Entschieden vorab: normale Prioritaet bleibt Vorgabe, die Handbuchbilder des Menue-Editors
(bm) laufen nur auf Zuruf.

Alle Zeiten sind Richtwerte unter Last (rund zehn Agenten, dazu die eigenen vier Laeufe nebeneinander); verglichen wird
nebeneinander im selben Fenster. Zwei Staende: **vor dem Umsetzen** auf karte1 (Runde 1 + Runde 2 auf a00f6cf90, exe
`bin_a`/`bin_b`, nachmittags) und **der Endstand** (karte1 852d12be7 + Runde 2, abends, exe `bin_b`, Rot-Stand `bin_rot`, beide neu gebaut). Protokolle:
`Y:\werkzeuge\tt\laeufe\<praefix>*` (nur noch Protokolle und Textdateien), Ausgaben `Y:\werkzeuge\tt\r2\<praefix>.out`.

## Ergebnis in Zahlen

| | Rechenzeit der Runde | laengste Strecke (Wanduhr mindestens) |
|---|---|---|
| Runde rc611 | 31623 s | 79 min (drei karteneditor nacheinander) |
| nach Runde 1 (gerechnet mit den Zeiten von rc611) | rund 16400 s | 37 min (karteneditor) |
| nach Runde 2 (gerechnet: karteneditor -25 %, Modus 1 -15 %, karte -21 %, dazu fuenfmal der Anfang des karteneditor und drei Starts der kartenpruefung; ohne die neun Laeufe, die karte1 inzwischen dazubekam) | rund 15750 s | 25 min (mission1, ungeteilt), danach bild2 Movie Duels 24 min und allekarten 21 min; der karteneditor zerfaellt in sechs Teile (der laengste 6-14 min je nach Last) |

Der Gewinn von Runde 2 liegt nicht in der Rechenzeit (-4 %), sondern in der Strecke (der karteneditor zerfaellt in sechs
Teile, kartenpruefung in vier Anteile) und darin, dass ein Lauf unter Last nicht mehr wegen eines Zeitrennens wiederholt
werden muss (sieben Stellen behoben). Gemessen nebeneinander:

| Lauf | Wanduhr | nach Bildern | |
|---|---|---|---|
| karteneditor ganz (vor dem Umsetzen) | 2399 s (Pausen 619,9 s) | 1797 s (173,8 s) | -25 % |
| Modus 1 (vor dem Umsetzen, Endstand des Codes) | 1608 s (287,9 s) | 1330 s (157,8 s) | -17 % |
| karte (vor dem Umsetzen) | 323 s (83,2 s) | 256 s (30,2 s) | -21 % |
| Modus 1 (Endstand) | 1121 s (222,7 s) | 958 s (83,9 s) | -15 % |

## R1. Wackler: nachgestellt, Ursache, behoben

Werkzeug zum Nachstellen ohne Last: `BHED_TEST_BREMSE=<ms>` (gab es schon: jedes Bild unregelmaessig bis zu so lang).
Damit wird ein Zeitrennen, das unter Last "1 von 3 Laeufen" trifft, beim ersten Lauf rot. Neu: `BHED_TEST_SIMZEIT=0`
schaltet die Simulationszeit ab - dieselbe exe zeigt den alten Fehler.

| Stelle | nachgestellt (rot) | Ursache | Behebung | danach |
|---|---|---|---|---|
| Modus 1 / `reiter`: `Klick "Close"` bzw. `"Next"` im Tutorial-Fenster ("Klick kam nicht an", "beim Loslassen nicht mehr unter der Maus") | alte exe, `reiter`, BREMSE=60, Pausen nach der WANDUHR: 89 OK, 4 FEHLER (`r2\x1w.out`) - haengt also nicht an der Pausenart | Tutorial "Compile and fix errors": der Knopf Compile! zeigt 1,5 s "Done" bzw. 2 s "Failed" und hat so lange keine Beschriftung; das Ziel `knopf:Compile` der Schritte 2 und 4 ist dann nicht zu finden, und 0,3 s nach dem Schrittwechsel erscheint die Zeile "The button is hidden right now" - das Fenster waechst um eine Zeile (30 Bildpunkte), spaeter schrumpft es wieder. Drei gleiche Bilder davor (`tourRuhig`) sagen darueber nichts | `TourSicht::hinweisKommt` (die Zeile steht noch aus); `tourRuhig()` wartet darauf; `compileRuhig()` wartet vor den zwei Klicks in k-uebersetzen, bis der Knopf wieder seine Beschriftung zeigt | `reiter`, BREMSE=60: 91 OK, 0 FEHLER nach der Wanduhr (`a1_`) und nach Bildern (`b1_`) |
| karteneditor Stufe 3: "Maus 3D: Strg + Maus im Freelook ..." (zwei Zeilen), in Laeufen mit Leerlauf-Prioritaet rot | `ke_bau` nur stufe3, BREMSE=400, `BHED_TEST_SIMZEIT=0`: 213 OK, 3 FEHLER (`b2_`) | der Schritt haelt die rechte Maustaste 13 Bilder; das Programm wertet einen Rechtsklick unter 300 ms seiner Zeit als kurz (Freelook bleibt an), sonst endet der Freelook beim Loslassen (wie NRC) | der Klick laeuft in Simulationszeit (`simZeit`: je Bild hoechstens 1/60 s) | derselbe Lauf mit Simulationszeit: 216 OK, 0 FEHLER (`b3_`) |
| karteneditor Stufe 3: "Klick auf [MapFile]: ... Tooltip \"Copied to clipboard.\"" (UIFORM: "Zwischenablage") | derselbe Lauf `b2_` | der Hinweis steht 666 ms Programmzeit; Klick, Pause und Pruefung brauchen fuenf bis sechs Bilder. Die Zwischenablage ist es nicht: der Selbsttest ersetzt sie je Prozess (`testablage`), nur die Probe im Modus original geht an die echte, und die ist ueber einen Mutex gereiht | Klick, Pause und Pruefung in Simulationszeit | derselbe Lauf `b3_`: gruen |
| `zeitleiste`/`ton`: "Ton: ... der Klang kommt, sobald sie gezeichnet wird" | `ton`, BREMSE=1500, SIMZEIT=0: diese Zeile und drei weitere rot (`a4_`: dazu "steigt mitten im Satz wieder ein", "Zuenden nur bei eingefahrener Klinge", "ein abgeschalteter Effektlaeufer klingt nicht weiter") | `tonAbspielen` setzt die Zeit in Stufen von 50 ms, das Abspielen zaehlt im naechsten Bild dessen Dauer dazu; nach einem Bild ueber 300 ms liegt der Klang hinter dem Fenster der Pruefung (950-1300 ms). Beim Wiedereinstieg laeuft das Abspielen ueber rund 15 Bilder weiter - bei wenigen Bildern je Sekunde ist der Satz vorbei | `tonAbspielen` und der Wiedereinstieg laufen in Simulationszeit | Endstand, `ton`, BREMSE=1500: ohne Simulationszeit 18 OK, 4 FEHLER (`t0_`, die vier Zeilen), mit 22 OK, 0 FEHLER (`t1_`). Beide Laeufe enden danach am Warteschritt "Ton: die Schleifen laufen" (bei 0,75 s je Bild kommt er nicht zustande - eine Folge der Bremse, nicht der Pruefung) |
| karteneditor Stufe 8, handbuch: "Rueckgewinnung fertig: nicht nach 180 s" (LCB2, VIS2c) | Protokolle: Runde rc611 172,6 s (en) gegen 21,5 / 23,9 s (es, cs); LCB2 `hb2_*.txt` ueber 180 s | die Rueckgewinnung laedt die Modelle in einem Prozess mit niedriger Prioritaet - neben anderen Laeufen bekommt er kaum Rechenzeit | `stufe8::rueckFertig()`: wartet auf das Ende (sie lief und laeuft nicht mehr); endet sie ohne Ergebnis, sofort FEHLER (vorher erst nach 180 s); Grenze 1500 s nur gegen das Haengen (wie dieselben Schritte in bspmerge) | Endstand unter Last: `ke_stufe8` nach Bildern 45 OK, 0 FEHLER in 566 s (`pb2_`: die Rueckgewinnung brauchte 281,8 s) und nach der Wanduhr 45 / 0 in 819 s (`sw2_`: 418,5 s) - beide waeren an der alten Grenze von 180 s gescheitert |
| karteneditor Stufe 8: "zweiter Bau: Schritt 3 im zweiten Durchgang: nicht nach 60 s" | eigener Lauf `pb_ke_stufe8` unter Last: der Bau brauchte 82 s statt 6-10 s | Grenze von 60 s auf einen Zustand waehrend des Baus | `stufe8::warteImBau()` fuer die drei Schritte dieser Art: der Zustand ist da - oder der Bau lief und ist zu Ende, ohne ihn gezeigt zu haben (sofort FEHLER) | wie oben |
| `karte`: "Ruhe: ohne Eingabe hoechstens wenige Bilder je Sekunde" (rot mit Pausen nach Bildern, Runde 1) | `g3b_karte`: 38 Bilder in 1,2 s | die Ruhe beginnt eine Sekunde WANDUHR ohne Eingabe, gezaehlt werden Bilder je Sekunde Wanduhr; die Pause nach Bildern war vorbei, ehe die Ruhe begann | beide Pausen sind `messfenster(2.0)` (immer nach der Uhr) | `karte` nebeneinander: 233 OK, 0 FEHLER in beiden Arten, "Ruhe - 13 Bilder in 3,36 s" (Wanduhr) und "13 Bilder in 3,40 s" (nach Bildern) |

Dazu, beim Lesen gefunden: **eine Pause nach Bildern liess die Simulationszeit vorauslaufen.** Jedes ihrer Bilder
rueckte die ImGui-Zeit um 1/20 s vor, bis ein Viertel der Pause auf der Wanduhr vergangen war - bei schnellen Bildern
(3 ms) sind das fuer `pause(0.2)` siebzehn Bilder = 0,85 s Programmzeit statt 0,2 s. Hinweise (666 ms) und Fristen des
Programms waeren auf einem ruhigen Rechner abgelaufen; die Nachweise von Runde 1 entstanden unter Last (20-50 ms je
Bild, dort faellt es nicht auf). Jetzt rueckt die Zeit nur vor, bis die Laenge der Pause erreicht ist; die Pause dauert
gleich lang wie vorher. Die sieben Laeufe aus Runde 1 sind deshalb neu nachgewiesen (R6).

Kleiner Fehler im PROGRAMM, nicht behoben (ausserhalb des Auftrags, an den Hauptchat gemeldet): im Tutorial "Compile and
fix errors" steht waehrend der 1,5/2 s, in denen der Knopf sein Ergebnis zeigt, "The button is hidden right now", und
der gelbe Rahmen um den Knopf fehlt - die Tour sucht ihn ueber die Beschriftung, die er dann nicht hat (die Kennung
`###compile` bleibt).

Nicht geaendert, weil es die Pruefung aendern wuerde (Frage 3 in tests_tempo.md): Schwellen gegen die Wanduhr -
`spulen` "zurueck so fluessig wie vor", `zeitleiste1` "Filmdurchgang ohne laengere Bilder", karteneditor "Stufe 7: grosse
Karte - keine Ruckler: 99. Perzentil unter 50 ms" (am 11.11. rot in beiden Pausenarten: 69,9 ms bei 28,9 ms im Mittel;
Runde rc611 195,7 / 45,0 / 27,9 ms), "Grosse Karte: ... kein Bild ueber 250 ms" (252 ms) und "zweiter Bau: ... Schaetzung
gegen die echte Dauer" (LIGHT 12,1 s statt 7,9 s im Bau davor).

## R2. karteneditor: Pausen nach Bildern, in Teilen nebeneinander, eigener Portbereich

**Pausen nach Bildern, der ganze Lauf nebeneinander** (vor dem Umsetzen; `kw_` Wanduhr, `kb_` nach Bildern, je eigener
Portbereich):

| | Pruefzeilen | Laufzeit | davon Pausen (2051) |
|---|---|---|---|
| Wanduhr | 3409 OK, 1 FEHLER | 2399 s | 619,9 s |
| nach Bildern | 3408 OK, 2 FEHLER | 1797 s (-25 %) | 173,8 s |

`--vergleich --ziffern`: 3115 / 3115 Pruefzeilen, nur links 0, nur rechts 0, anderer Stand 1. Rot in beiden: "Stufe 7:
grosse Karte - keine Ruckler" (Bildzeit unter Last). Der andere Stand: "zweiter Bau: ... Schaetzung gegen die echte
Dauer". Beides sind Schwellen gegen die Wanduhr (R1, Frage 3), keine Folge der Pausenart.

**Gruppen.** Der volle Lauf ist eine Folge von 35 Gruppen (mit numpad aus karte1); vor jeder steht jetzt
`gruppe("name")` (nur Einfuegungen in `gui/selbsttest_handbuch.inc`; eine neue Gruppe eines anderen Zweigs gehoert ohne
eigenes `gruppe()` zur vorigen). `BHED_KARTE_GRUPPEN="stufe3,stufe4"` faehrt nach dem gemeinsamen Anfang (12 Pruefzeilen)
nur die genannten, in der Reihenfolge des vollen Laufs; ein unbekannter Name ist ein FEHLER. Jede Gruppe schreibt
"Knopftest: Gruppe <name> ab <s> s" - daraus die Zeiten je Gruppe (`Y:\werkzeuge\tt\r2\gruppenzeiten.py`).

**Eigener Portbereich.** Die Bau-Ueberwachung lauscht auf 39000 und weicht bis 39063 aus; Stufe 3 belegt im Test erst
39000, dann alle 64 Ports ("Failed to get a listening socket") - jedes andere JK Studio, das gerade baut, scheitert dann
oder verschiebt die Erwartung ("monitoring on 127.0.0.1:39002"). Neu: `BHED_BAU_PORT=<erster Port>` gibt einer Instanz
einen eigenen Bereich von 64 Ports (ohne die Variable unveraendert 39000). `tools/testrunde.py` gibt jedem Lauf einen
freien Bereich im Raster 40000 + 64 k; nur `ke_bau` (mit Stufe 3) baut mit NRCs Ports, weil Stufe 3 die Adresse
127.0.0.1:39000 selbst prueft (ohne die Variable muss sie es sein, `s3Adresse`), und traegt dafuer die Sperre "ports".

**Teile.** Sechs Laeufe `ke_*` in `tools/testrunde.py` (KE_TEILE), jede Gruppe in genau einem; zusammen bleibt, was
aufeinander baut: die Gruppen der Stufe 1 mit stufe2 (der Anfang des vollen Laufs), stufe8c + stufe8g + stufe9 (die
gebaute Karte) samt den Echtzeit-Gruppen danach; 9c, 9d, licht1, modobj und aseb beginnen, wenn sie nicht auf ihre
Vorgaengerin folgen, wie ihr Einzellauf (BHED_KARTE_NUR) mit `stufe9c::einrichten()`. Vor dem Umsetzen, je sechs Teile
nacheinander in einem Platz:

| Teil | Gruppen | Pruefzeilen | Wanduhr (`pw_`) | nach Bildern (`pb_`) |
|---|---|---|---|---|
| ke_stufe8 | stufe8 | 44 | 101 s | 738 s (rot: Grenze 60 s unter Last, siehe R1) |
| ke_befehle | befehle, stufe6 | 1665 | 346 s | 319 s |
| ke_licht | stufe7, stufe8b, gross | 152 | 234 s (rot: Stufe 7 Bildzeit) | 174 s |
| ke_bau | stufe3, stufe4, stufe5 (NRCs Ports) | 619 | 768 s (rot: "kein Bild ueber 250 ms": 252 ms) | 200 s |
| ke_karte | stufe8c, stufe8g, stufe9, stufe9c, stufe9d, licht1, modobj, aseb, kreste, r2a, r3, rd, rest8, reste1, fotos | 284 | 300 s | 706 s |
| ke_grund | rahmen, tasten, (numpad,) mass, xy, kamera, auswahl, textur, entitaeten, konsole, stufe2 | 706 | 238 s | 122 s |
| zusammen | | 3470 = 3410 + 5 x 12 (der gemeinsame Anfang) | 1986 s | 2259 s |
| der ganze Lauf | | 3410 | 2399 s | 1797 s |

Die Zeiten je Teil schwanken mit der Last des Augenblicks um das Siebenfache (ke_stufe8 101 s gegen 738 s, ke_bau 768 s
gegen 200 s): die Teile liefen nacheinander in je einem Platz ueber 45 Minuten, daneben bauten und massen andere. Der
laengste Teil brauchte 768 s, der ganze Lauf 2399 s bzw. 1797 s - die Strecke des karteneditor in der Runde sinkt von
30-40 Minuten auf rund 13 Minuten unter dieser Last (in den ruhigeren Abschnitten 2-6 Minuten je Teil).

**Nachweis "die Teile zusammen = der ganze Lauf"** (`--namen` fuehrt die Teile unter dem Namen des ganzen Laufs,
`--vergleich --ziffern`):

| | Pruefzeilen ganz / Teile | nur ganz | nur Teile | anderer Stand |
|---|---|---|---|---|
| vor dem Umsetzen, Wanduhr: `kw_` gegen `pw_` | 3115 / 3115 | 0 | 0 | 1: "Grosse Karte: ... kein Bild ueber 250 ms" (252 ms im Teil) |
| vor dem Umsetzen, nach Bildern: `kb_` gegen `pb_` | 3115 / 3116 | 0 | 1: die Zeile des Warteschritts "zweiter Bau: Schritt 3 im zweiten Durchgang: nicht nach 60 s" (es gibt sie nur, wenn er scheitert; seitdem behoben) | 2: "Stufe 7 ... keine Ruckler", "zweiter Bau ... Schaetzung" - rot im ganzen Lauf, gruen in den Teilen |
| Endstand, nach Bildern: `kb2_` gegen `pb2_` | 3152 / 3150 | 2: die Zeilen zweier Klickschritte (`Klick "info": nicht gefunden`), die es nur gibt, wenn der Doppel-Rechtsklick davor scheitert | 0 | 3: der Doppel-Rechtsklick mit seiner Folgezeile (250 ms Programmzeit, unter Last zu spaet - seitdem in Simulationszeit, R1) und "kein Bild ueber 250 ms": rot im ganzen Lauf, gruen in den Teilen. "Stufe 7 ... keine Ruckler" rot in beiden |

Am Endstand (karte1 mit numpad, je Lauf eine Pruefzeile mehr am Ende; daneben lief die volle Runde des Agenten RUNDE):
der ganze Lauf nach Bildern 3449 OK, 6 FEHLER in 2675 s (Pausen 257,7 s); die Teile ke_stufe8 45 / 0 in 566 s,
ke_befehle 1666 / 0 in 864 s, ke_licht 152 / 1 in 354 s, ke_bau 620 / 0 in 238 s, ke_karte 285 / 0 in 473 s, ke_grund
749 / 0 in 219 s - zusammen 2713 s, der laengste 864 s.

Jede Pruefzeile des ganzen Laufs steht in den Teilen wieder und umgekehrt; was abweicht, sind die Schwellen gegen die
Wanduhr und die Zeitrennen aus R1.

**Die Grenze der Teile:** jeder Teil beginnt mit frischem Programm. Was eine Gruppe der naechsten hinterlaesst, wirkt
im ganzen Lauf an allen 34 Uebergaengen, in den Teilen an 29 - an den fuenf Teilgrenzen nicht mehr (der Rot-Stand in R5
zeigt es an 13 Folgezeilen). Die Pruefzeilen sind dieselben, die Vorgeschichte von fuenf Gruppen ist es nicht. Der ganze
Lauf bleibt erreichbar: `python -I tools/testrunde.py --ungeteilt` oder beim Namen (Frage 5 in tests_tempo.md).

## R3. Lange Schleifen ueber Spieldaten: Anteile nebeneinander

`BHED_TEST_ANTEIL=i/n` (1 <= i <= n): die Schleife nimmt nur den i-ten von n ZUSAMMENHAENGENDEN Abschnitten ihrer
sortierten Liste (`testtempo::imAnteil/anteil` in `gui/selbsttest.cpp`). Zusammenhaengend statt "jeder n-te", damit
dieselben Eintraege in derselben Reihenfolge aufeinander folgen wie im ganzen Lauf (nur an n-1 Nahtstellen nicht).
Eingebaut in kartenpruefung (die Nummer der Karte im Bericht bleibt die der ganzen Liste) und in die Rundlaeufe bild,
bild2, kurvenrundlauf, ikrundlauf, rigrundlauf des Modell-Blocks.

**kartenpruefung in vier Anteilen** (`kartenpruefung_1v4` ... `_4v4`, Bericht der Runde aus allen vieren):
Endstand: der ganze Lauf (`kq_`, 1101 s) gegen die vier Anteile (`kp2_`: 407, 724, 621, 1159 s, nacheinander in einem
Platz unter wechselnder Last) - `Y:\werkzeuge\tt\r2\kp_vergleich.py` vergleicht die Berichte Karte fuer Karte und
uebergeht nur `lade_ms`: **282 / 282 Karten, nur links 0, nur rechts 0, andere Werte 0.** (Vor dem Umsetzen liefen die
Anteile schon einmal gruen: 316, 286, 804, 841 s.)

**mission1 in Teilen** (BHED_M1_TEIL gab es schon; "undo" bringt jka und drehen mit): **nicht uebernommen.** Am Endstand lief der Teil "jka zwischen gizmo" (`m1t_mission1_jka`, 14 OK, 1 FEHLER) anders als
der ganze Lauf: "Gizmo: Kamera - beim Abspielen kein stehengebliebener Gizmo, nach Stop wie vorher" rot (Achsenfarben im
Bild nach Stop 12 statt 1303), im ganzen Lauf daneben gruen (`m1g_`: 1303). Im ganzen Lauf steht zwischen "zwischen" und
"gizmo" der Abschnitt undo (academy1) - die Abschnitte haengen an ihrer Reihenfolge. mission1 bleibt EIN Lauf (1495 s in
der Runde rc611) und ist damit die laengste Strecke der Runde.

**Die Rundlaeufe des Modell-Blocks** kennen die Variable, laufen in der Runde aber ungeteilt: der Nachweis ganz gegen
Anteile ist nicht gefahren (bild2 Movie Duels: 24 Minuten je Seite). Mit `geteilt("mb_bild2_md", 3)` in
`tools/testrunde.py` ist es eine Zeile - nach dem Nachweis (Summen der Zeile "MB2 Bild md ..." vergleichen).

**allekarten bleibt ganz** (Frage 2 in tests_tempo.md): nach allen 282 Karten laedt der Lauf die erste noch einmal und
vergleicht den Speicher, und je Karte prueft er auf Bilder der VORIGEN im Grafikspeicher. In Anteilen saehe die
Speicherpruefung je Prozess nur einen Teil der Karten.

## R4. `tools/testrunde.py`

- **Eigener Portbereich je Lauf** (`BHED_BAU_PORT`, siehe R2); Laeufe mit `ports="vorgabe"` (ke_bau, der ganze
  karteneditor) bauen mit 39000-39063 und tragen die Sperre "ports".
- **Teile und Anteile:** `ke_*` (BHED_KARTE_GRUPPEN), `<name>_1vN` (BHED_TEST_ANTEIL=i/N, `geteilt(name, n)`). Der ganze
  Lauf steht als Doppel seiner Teile in der Tabelle und bleibt beim Namen erreichbar; `--ungeteilt` faehrt die ganzen
  Laeufe statt der Teile. `--namen` fuehrt die Pruefzeilen der Teile unter dem Namen des ganzen Laufs - `--vergleich`
  vergleicht eine Runde in Teilen unmittelbar mit einer, die den ganzen Lauf fuhr; steht eine Zeile mehrfach da, zaehlt
  jedes FEHLER. Die Ausgabe ist UTF-8, auch umgeleitet (eine Pruefzeile mit "Č" brach `--vergleich` sonst ab).
- **Pausen nach Bildern** tragen jetzt Modus 1, karteneditor und seine Teile, karte und reiter (zu menueeditor,
  menueloeschen, missionsbau, mb_funktionen, mb_tasten, mb_mb2, mb_mb5 aus Runde 1).
- **Bericht der kartenpruefung:** aus den Anteilen entsteht `<laeufe>/<praefix>kartenpruefung.jsonl`, nach der Nummer
  der Karte sortiert, wie ihn der ungeteilte Lauf schreibt (Auswertung wie bisher: `tools/kartenpruefung.py
  --nur-auswerten`).
- **Testdaten:** `braucht` je Lauf; fehlt eine Datei, startet der Lauf nicht und ist ROT ("Testdaten fehlen: ...") -
  er liefe sonst mit weniger Pruefungen gruen durch. Die grosse Karte des karteneditor (Gruppen stufe7 und gross:
  `q3\duel_mos_espa.map`, `bsp\duel_mos_espa.bsp`, mit q3map2 -convert aus Movie Duels dekompiliert) liegt im
  Sitzungsordner `Y:\werkzeuge\sp_af9c\karte_dekomp\md`; an einem anderen Ort: `TR_GROSS=<ordner>`. (Nicht verschoben.)
- **Beenden ohne taskkill:** `_nachkommen()` nimmt als Kinder nur Prozesse, die NACH ihrem Elternprozess entstanden sind
  (Windows vergibt PIDs neu; Vorfall 11.11.), `_beenden()` beendet den eigenen Prozess und jede dieser PIDs einzeln
  (TerminateProcess) - kein fremdes Programm mehr, `lint_fremdprogramm` ist wieder bei 1 (der alte Fund in
  `tools/kartenpruefung.py`); `tools/test_betroffen.py` meldet ein fehlendes git mit einem Satz statt eines Rueckverfolgs.
- **Haengt-Erkennung geprobt:** `python -I tools/testrunde.py probe_haengt --haengt 20` startet einen Befehl, der nichts
  schreibt und nicht rechnet, mit einem Kindprozess: nach 20 s "probe_haengt: ROT, exit HAENGT (20 s ohne Protokoll und
  ohne Rechenzeit)", Eltern- und Kindprozess beendet (nachgesehen ueber die Befehlszeile), Ausgang 1 (`r2\hp.out`).
  Grenze der Erkennung, gesehen an `ton` mit BHED_TEST_BREMSE=1500: ein gebremster Lauf schlaeft zwischen den Bildern
  und schreibt in einem langen Warteschritt nichts. Mit 180 s gilt er als haengend (falsch: er wartet nur langsam), mit
  400 s nie (er sammelt in der Zeit ueber 1 s Rechenzeit) - gebremste Laeufe von Hand beobachten.
- **Nicht angefasst:** der Ort der Testdaten (`TR_TESTDATEN`, `Y:\karte\laeufe\testdaten` - seit dem Aufraeumen leer;
  der Agent RUNDE stellt ihn um) und die drei Starts von vis2c in einem Laufordner (fehlen in der Tabelle: sie braeuchte
  eine Spalte "Laufordner des Vorgaengers weiterbenutzen").

## R5. Gegenprobe mit eingebauten Fehlern (Rot-Stand `Y:\werkzeuge\tt\r2\rot2.py`, exe `r2\bin_rot`)

Drei Fehler im PROGRAMM: F2 (naechstes Lesezeichen) springt rueckwaerts; im Karten-Editor gilt kein Rechtsklick mehr als
kurz (`freelookEndetBeimLoslassen`); die Kartenpruefung haelt `ref_tag` fuer "nicht im Spiel".
| Lauf | Variante | Ergebnis |
|---|---|---|
| Modus 1 | nach der Wanduhr (`xw_`, vor dem Umsetzen) | 2631 OK, 4 FEHLER: "F2 springt zum ersten Lesezeichen", "F2 springt zum naechsten", "F2 am Ende beginnt wieder oben", "Umschalt+F2 am Anfang springt ans Ende" |
| Modus 1 | nach Bildern (`xb_`, Endstand) | 2632 OK, 4 FEHLER: dieselben vier Zeilen |
| karteneditor | ganz, nach Bildern (`xk_`, Endstand) | 3368 OK, 99 FEHLER (83 verschiedene Pruefzeilen) |
| karteneditor | Teile ke_grund und ke_bau, nach Bildern (`xp_`, Endstand) | 700 OK, 63 FEHLER und 618 OK, 2 FEHLER: jede der 65 roten Zeilen ist auch im ganzen Lauf rot, keine nur im Teil |
| kartenpruefung, 15 Karten (`BHED_KARTEN=academy,t1_`) | ganz gegen vier Anteile am Rot-Stand (`xr_`) | 15 / 15 Karten, andere Werte 0 |
| kartenpruefung, dieselben Karten | gruener Stand (`xg_`) gegen Rot-Stand | 13 Karten anders (`nicht_im_spiel` mit `ref_tag`) - der Fehler steht im Bericht des ganzen Laufs und der Anteile gleich |

**Was die Teile des karteneditor nicht mehr pruefen, zeigt der Rot-Stand:** im ganzen Lauf sind in den Gruppen von
ke_bau 13 Zeilen mehr rot als im Teil (Teiler der Anordnungen, Maus im Texturbrowser, "Leere Karte", "Unbenannte Karte") -
Folgefehler: die Kamera-Gruppe hinterlaesst den Freelook im falschen Zustand, und Stufe 3 erbt ihn. Im Teil beginnt Stufe 3
mit frischem Programm. Der Fehler selbst wird in beiden gefunden (die 65 Zeilen); verloren geht an den fuenf Teilgrenzen
nur, was eine Gruppe der naechsten hinterlaesst. Wer das mitpruefen will: `python -I tools/testrunde.py --ungeteilt`
(Frage 5 in tests_tempo.md).

## R6. Die Laeufe aus Runde 1 nach der geaenderten Pause

Weil die Pause nach Bildern geaendert ist (R1), sind die sieben Laeufe aus Runde 1 am Endstand noch einmal
nebeneinander gelaufen (`ew_`/`eb_` menueeditor, `fw2_`/`fb2_` die sechs kleinen):

| Lauf | Wanduhr | nach Bildern | Vergleich |
|---|---|---|---|
| menueeditor | 2789 OK, 0 FEHLER, 1389 s (Pausen 672,7 s) | 2789 / 0, 956 s (204,7 s) = -31 % | 1836 / 1836, nur links 0, nur rechts 0, anderer Stand 0 |
| menueloeschen | 108 / 0, 90 s (40,2 s) | 108 / 0, 68 s (13,1 s) | zusammen 481 / 481, 0 / 0 / 0 |
| missionsbau | 96 / 0, 183 s (46,3 s) | 96 / 0, 223 s (15,0 s) | |
| mb_funktionen | 62 / 0, 36 s (15,2 s) | 62 / 0, 22 s (4,7 s) | |
| mb_tasten | 44 / 0, 26 s (6,0 s) | 44 / 0, 24 s (2,2 s) | |
| mb_mb2 | 38 / 0, 24 s (7,1 s) | 38 / 0, 18 s (2,0 s) | |
| mb_mb5 | 160 / 0, 126 s (73,5 s) | 160 / 0, 76 s (23,5 s) | |

(missionsbau besteht fast nur aus Laden und Bauen; seine Laufzeit folgt der Last, nicht den Pausen.)

## Schlussrunde (Endstand)

- Umsetzen auf karte1 852d12be7: ein Konflikt (`gui/selbsttest_handbuch.inc`: die neue Gruppe numpad bekam ihr
  `gruppe("numpad")` und steht im Teil ke_grund); `tools/testrunde.py` fuegte sich ohne Konflikt hinter die neun neuen
  Laeufe von karte1.
- Bau `Y:/p_test/build` (Release, /W4 /WX), alles: 0 Warnungen (`Y:\werkzeuge\tt\r2\bau_k1.log`); Rot-Stand und
  zurueck je 0 Warnungen (`bau_rot2.log`, `bau_zurueck.log`).
- ctest `-j 16` (`r2\ctest_k1.log`): 254 von 254 bestanden (1565 s neben der vollen Runde des Agenten RUNDE).
- Lints, karte1 gegen den Zweig (`r2\lints_k1.txt`): sources, settings, gapp, endtable, imgui_context, i18n, feldinit,
  mapstate, d3dbesitz, headers gleich (ok), portability 99 = 99, pruefcodes 11 = 11, fremdprogramm 5 -> 1 (bleibt der
  alte Fund in `tools/kartenpruefung.py`). `git ls-files --eol`: LF.
- Selbsttests am Endstand (exe `r2\bin_b`), jeweils die Laeufe, die Runde 2 aendert, in beiden Pausenarten: Modus 1,
  menueeditor, menueloeschen, missionsbau, mb_funktionen, mb_tasten, mb_mb2, mb_mb5, karte (`cw2_` 234 OK, 0 FEHLER,
  `cb2_` 234 / 0, Vergleich 220 / 220, 0 / 0 / 0), ke_stufe8; nach Bildern der ganze karteneditor und seine sechs Teile;
  kartenpruefung ganz und in vier Anteilen; `ton` unter der Bremse; Rot-Stand wie in R5. Alles gruen bis auf die
  Schwellen gegen die Wanduhr (R1) und die Stelle unten.
- Zuletzt mit dem Werkzeug, wie die Runde es faehrt (`pv_`, `tz_`): `ke_bau` mit NRCs Ports 39000-39063 (ohne
  `BHED_BAU_PORT`: Stufe 3 prueft die Adresse 127.0.0.1:39000) 620 OK, 0 FEHLER; `reiter` 92 / 0 und `ke_karte`
  285 / 0 mit eigenem Portbereich und Pausen nach Bildern aus der Tabelle.
- **Offen, im letzten ganzen Lauf gefunden:** "Doppel-M2 in der Kamera: das Entitaetenmenue" (Gruppe entitaeten) wurde im
  ganzen Lauf unter der Last der vollen Runde rot (mit zwei Folgezeilen), im Teil ke_grund gruen: der zweite Rechtsklick
  zaehlt nur innerhalb von 250 ms Programmzeit als Doppelklick (context_menu_try) - dieselbe Art Zeitrennen wie der
  kurze Rechtsklick. Die Behebung (die zwei Klicks in Simulationszeit) liegt fertig in
  `Y:\werkzeuge\tt\r2\patch_doppelm2.py`, ist aber nicht eingebaut: sie ist nicht unter der Bremse rot und gruen gezeigt.
- Nicht gelaufen: eine volle Runde (faehrt der Agent RUNDE auf dem zusammengefuehrten Stand), die Laeufe, die Testdaten
  aus `Y:\karte\laeufe\testdaten` brauchen (kemd, wegerzeugung - der Ordner fehlt seit dem Aufraeumen), der Nachweis fuer
  die Rundlaeufe des Modell-Blocks in Anteilen; der ganze Lauf mission1 wurde nach dem Befund am Teil (R3, bei 47 von
  rund 99 Pruefzeilen, bis dahin gruen) abgebrochen.

## Changelog (Runde 2)

Kein Eintrag: Testwerkzeuge und Selbsttest. Am Programm neu ist nur die Umgebungsvariable `BHED_BAU_PORT` (eigener
Portbereich der Bau-Ueberwachung fuer Laeufe nebeneinander) - ohne sie verhaelt sich das Programm unveraendert.
