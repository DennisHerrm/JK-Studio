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
