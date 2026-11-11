# Tests schneller bei gleichem Ergebnis (TESTTEMPO, rc612)

shank 11.11.: "kannst du schauen das man die Tests vielleicht bisschen optimiert? Das es weniger tests gibt damit es
schneller geht mit demselben ergebnis? Das man vielleicht dinge zusammen legen kann?"

Grundsatz: jede Pruefung, die heute einen Fehler findet, findet ihn danach auch. Weniger wird nur, was doppelt ist oder
nur Zeit kostet. **Eine Ausnahme, von shank am 11.11. entschieden:** die volle Runde laeuft nur noch englisch (Abschnitt
"Entscheidung: volle Runde nur englisch") - das ist die einzige Stelle, an der weniger geprueft wird als bis rc611.

## Teil A - Messung (nur gelesen)

Quelle: die volle Runde vor rc611 (`Y:\karte\laeufe\rel611_*`, `Y:\werkzeuge\rel611\summe*.txt`, `ctest*.txt`), 193 Laufordner,
ausgewertet mit `Y:\werkzeuge\tt\laeufe_auswerten.py` und `enthalten.py`. Zeiten sind Richtwerte unter Last (Pool mit
8-12 Laeufen, Leerlauf-Prioritaet, daneben Bauten): Laufzeit = Protokoll angelegt bis zuletzt geschrieben, Kopieren =
Laufordner angelegt bis Protokoll angelegt.

### Summe einer vollen Runde

| | Laeufe | Laufzeit | Pruefzeilen | Kopieren + Start bis erstes Bild |
|---|---|---|---|---|
| alle Selbsttests | 193 | 31623 s (8,8 h Rechenzeit) | 40428 | 507 s + 243 s (2,4 %) |
| karteneditor voll (en, es, cs) und Teile | 20 | 6284 s | 10793 | 59 s |
| menueeditor (en, ja, es, cs), menueloeschen | 5 | 5915 s | 11253 | 49 s |
| modellblock (alle Gruppen, Rundlaeufe je Spiel) | 50 | 3945 s | 848 | 173 s |
| Handbuchbilder Menue-Editor (bm, 6 Sprachen) | 6 | 3437 s | 3488 | 72 s |
| Modus 1 (en, es, cs, dazu "mp" und "sp") | 5 | 2871 s | 10792 | 39 s |
| Docks, Tabfenster | 7 | 115 s | 312 | 52 s |
| uebrige Modi | 100 | 9056 s | 2942 | 306 s |
| ctest (245 Faelle, `-j 16`) | 1 | 170-422 s Wanduhr | | |

Wanduhr der Runde: nach unten begrenzt durch die Kette der drei vollen karteneditor-Laeufe (feste Ports 39000-39063,
deshalb nacheinander): 2246 + 1353 + 1129 = 4728 s (79 min). ctest dauert so lange wie sein laengster Fall (`defgpu`,
170-422 s je nach Last); danach `updater_uebergang` (76-173 s), `hlsl` (98-200 s), `netze` (65-101 s),
`karte_optimierer` (45-80 s).

### Die 30 teuersten Laeufe

| Lauf | Modus | Laufzeit | Schritte | davon Pausen | Pruefzeilen |
|---|---|---|---|---|---|
| karteneditor | karteneditor | 2246 s | 12958 | 2051 | 3407 |
| menueeditor_cs | menueeditor | 1548 s | 12591 | 3652 | 2787 |
| menueeditor_es | menueeditor | 1546 s | 12591 | 3652 | 2787 |
| menueeditor | menueeditor | 1543 s | 12591 | 3652 | 2787 |
| mission1 | mission1 | 1495 s | 12 | 0 | 98 |
| kartenpruefung | kartenpruefung | 1481 s | 3110 | 1 | 0 (Bericht je Karte) |
| mb_bild2_md | modellblock bild2 | 1422 s | | 0 | 2 |
| karteneditor_es | karteneditor | 1353 s | 12958 | 2051 | 3407 |
| allekarten | allekarten | 1278 s | 60 | 1 | 283 |
| menueeditor_ja | menueeditor | 1193 s | 12591 | 3652 | 2785 |
| karteneditor_cs | karteneditor | 1129 s | 12958 | 2051 | 3407 |
| mb_kr_md | modellblock kurvenrundlauf | 1061 s | 9 | 0 | 2 |
| 1_es | 1 | 854 s | 10668 | 681 | 2635 |
| 1_cs | 1 | 853 s | 10668 | 681 | 2635 |
| 1 | 1 | 861 s | 10668 | 681 | 2635 |
| mb_bild_md | modellblock bild | 764 s | | 0 | 1 |
| sp | (kein Modus: Modus 1) | 688 s | 10668 | 681 | 2635 |
| update | update | 603 s | 26 | | 5 |
| bm_ja, bm_de, bm_cs, bm_zh, bm_en, bm_es | menueeditor, Bilder | 556-583 s | 2464 | 714 | 581 |
| handbuch | handbuch | 505 s | 2329 | 676 | 6 |
| mp | (kein Modus: Modus 1) | 435 s | 10668 | 681 | 2635 |
| mb_kr_jk2 | modellblock kurvenrundlauf | 433 s | 10 | 0 | 2 |
| optsicher | karteneditor optsicher | 430 s | 91 | | 14 |
| mb_irl_md | modellblock ikrundlauf | 353 s | 9 | 0 | 2 |
| karte | karte | 351 s | 2793 | 237 | 233 |

(bm_* zaehlt als sechs Laeufe; zusammen sind es 30.)

### Wo die Zeit bleibt

1. **Feste Pausen.** 34039 Schritte der Runde sind `pause(sekunden)` (Wanduhr, 0,1-1,0 s, im Quelltext 4459 Stellen mit
   im Mittel 0,38 s). Je Lauf: menueeditor 3652, karteneditor 2051, bm 714, Modus 1 681, handbuch 676, kuerzel 322,
   mb5 279. Der Lauf `kuerzel` (100 s) besteht fast nur aus seinen 322 Pausen. Die Pausen dauern unter Last so lange wie
   ohne Last - sie sind der Teil, der sich durch mehr Kerne nicht verkuerzt. Gemessene Anteile stehen in Teil B.
2. **Dasselbe mehrfach.** Sprachlaeufe (es, cs, ja) wiederholen alle Pruefzeilen der englischen Laeufe: karteneditor es+cs
   2482 s, menueeditor es+cs+ja 4287 s, Modus 1 es+cs 1707 s, zusammen mit kuerzel/zeitleiste1/hilfe/tutgeo 9456 s
   (30 %; seit 11.11. nur noch auf Zuruf, siehe "Entscheidung"). Dazu echte Doppel (naechster Abschnitt) rund 2500 s (8 %).
3. **Lange Schleifen ueber Spieldaten.** kartenpruefung 1481 s, mission1 1495 s, allekarten 1278 s, die Rundlaeufe des
   Modell-Blocks ueber alle Modelle/Sequenzen (bild2_md 1422 s, kr_md 1061 s, bild_md 764 s, kr_jk2 433 s, irl_md 353 s,
   kr_jka 346 s, rrl_md 321 s). Hier laeuft je Lauf EIN Faden; die Zeit ist Rechnen und Laden, keine Wartezeit.
4. **Programmstart und Kopieren sind es nicht.** Start bis zum ersten Bild 243 s und Kopieren der exe/Daten 507 s ueber
   alle 193 Laeufe = 2,4 % der Runde. Gruppen in einem Programmstart zusammenzulegen spart hoechstens diesen Anteil
   (beim Modell-Block 173 s von 3945 s) und verlangt, dass jede Gruppe ihren Ausgangszustand selbst herstellt - das
   Risiko steht in keinem Verhaeltnis zum Gewinn. Nicht umgesetzt; die Gewinne liegen in 1-3.
5. **Wanduhr:** die Kette der drei vollen karteneditor-Laeufe (79 min) und bei ctest der eine Fall `defgpu`.
6. **Leerlauf-Prioritaet neben Bauten** (am 11.11. nachgemessen, nicht aus den alten Protokollen lesbar): die Laufskripte
   starten jeden Selbsttest mit Leerlauf-Prioritaet. Bauen daneben andere, bekommt er kaum Rechenzeit - derselbe Lauf
   `geteilt` 155 s statt 16 s (94 ms statt 7,8 ms je Bild), `mb_mb2` 208 s statt 31 s. Das ist an einem Tag mit vielen
   Agenten der groesste einzelne Posten, und er erklaert die Wackler unter Last. Die neue Zeile "Knopftest Zeiten:
   gesamt ... s, ... Bilder | Pause ... | Warte ... | Foto ... | sonst ..." in jedem Protokoll zeigt es je Lauf.

### Doppeltes

Belegt mit `enthalten.py` (woertlich gleiche Pruefzeilen) und im Quelltext (`gui/selbsttest.cpp`, Verteilung der Modi):

| Lauf (Zeit in der Runde) | steht ganz in | Grund |
|---|---|---|
| `mp` 435 s, `sp` 688 s | `1` | keine Modi: jeder unbekannte Wert von BHED_EDITORTEST faellt auf den Hauptlauf (2629 von 2629 Zeilen gleich); stammen aus dem Testkatalog, der zwei Zeichenketten des Karten-Tests fuer Modi hielt |
| `kartenfunde` 223 s | `gegenstaende` | derselbe Modus unter zwei Namen (`nurGegenstaende`) |
| `fix604b` 144 s, `figurstufen` 66 s, `geschuetzanim` 31 s, `zielstrahl` 24 s, `effektgrund` 24 s, `figurdeform` 20 s | `gegenstaende` | ihre Schrittlisten laufen im Modus gegenstaende mit |
| `kuerzel` 100 s, `ziehen` 90 s, `fenster` 48 s, `zeitleiste` 16 s, `ton` 12 s, `szenen` 10 s, `geteilt` 8 s, `lesezeichen` 4 s | `1` | Teilmodi "zum schnellen Wiederholen"; der Hauptlauf haengt dieselben Schrittlisten an |
| `kuerzel_es`, `kuerzel_cs` je 108 s | `1_es`, `1_cs` | wie oben |
| `kamera` 131 s, `gluehen` 57 s, `npcklasse` 36 s | `karte` | Schrittlisten laufen im Modus karte mit |
| `licht1` 58 s, `r2a` 36 s, `modobj` 27 s | `karteneditor` | BHED_KARTE_NUR waehlt nur einen Teil des vollen Laufs |
| `editorportal_tester` 35 s | `editorportal` | BHED_P_NUR waehlt nur einen Teil |
| modellblock ohne NUR (`ohnenur`) | `mb_funktionen` + `mb_tasten` | ohne BHED_MODELL_NUR laufen genau diese zwei |
| `bm_*` (6 x 570 s) | `menueeditor` | Bildlaeufe fuers Handbuch (2560x1500); 579 von 579 Zeilen stehen im vollen Lauf - nur noetig, wenn Bilder neu aufgenommen werden |

Die Teilmodi bleiben zum gezielten Wiederholen; in der vollen Runde bleiben sie weg (`tools/testrunde.py`, Spalte
`doppel`; `--pruefe-doppel` belegt es aus den Protokollen einer Runde, `--mit-doppelten` faehrt sie trotzdem).

Gleiche Daten je Lauf neu geladen: jeder Modell-Block-Lauf laedt den Sturmtruppler und `_humanoid` (1,2 s), die
Karten-Modi laden md_am_sith je Lauf (~25 s unter Last, 20 Laeufe). Das Zusammenlegen dieser Laeufe siehe Punkt 4 oben.

### Wackler (rot unter Last, gruen beim Wiederholen auf demselben Bau)

| Lauf | Pruefung | Ursache |
|---|---|---|
| karteneditor | "Cancel build" | Zeitrennen: der Bau ist unter Last noch nicht so weit, wie der Schritt annimmt |
| menueeditor | "M5b ... Sicherung weg" | Zeitrennen mit dem Sichern im Hintergrund |
| zeitleiste1 (en, es, cs) | je 1 FEHLER bei 200 s statt 99 s Laufzeit | Abspielzeit gegen Wanduhr unter Last |
| spulen | 1 FEHLER bei 391 s statt 114 s | Bildzeit-Schwelle unter Last |
| mp (= Modus 1) | 1 FEHLER, beim zweiten Mal gruen | unter Last, Stelle nicht festgehalten |
| zeitleiste (allein, 11.11.) | "Ton: ... der Klang kommt, sobald sie gezeichnet wird (aufgeschoben, nicht verschluckt)" | rot in zwei Laeufen gleichzeitig neben einem vollen Bau, allein wiederholt gruen (neue exe und rc611); in Modus 1 gruen. **Runde 2:** der Schritt setzt die Zeit in Stufen von 50 ms, das Abspielen zaehlt die Dauer des naechsten Bildes dazu - nach einem Bild ueber 300 ms liegt der Klang hinter dem Fenster der Pruefung; nachgestellt und behoben |
| 1 mit Pausen nach Bildern (11.11.) | `Klick "Close"` im Tutorial-Fenster: "Klick kam nicht an", "beim Loslassen nicht mehr unter der Maus" | das Fenster wandert noch, wenn geklickt wird; 1 von 3 Laeufen. **Runde 2:** haengt nicht an der Pausenart (auch nach der Wanduhr nachgestellt) - das Fenster waechst 0,3 s nach dem Schrittwechsel um die Zeile "The button is hidden right now", solange der Knopf Compile! sein Ergebnis zeigt; behoben |
| karteneditor (UIFORM, 11.11.) | "Maus 3D: Strg + Maus im Freelook ...", "Klick auf [MapFile]: ... Tooltip \"Copied to clipboard.\"" | **Runde 2:** das Programm misst beides an seiner Zeit (kurzer Rechtsklick unter 300 ms, Hinweis 666 ms), der Schritt braucht 13 bzw. 5 Bilder - unter Last zu lang. Nicht die Zwischenablage (der Selbsttest hat eine eigene je Prozess); nachgestellt und behoben |
| karteneditor (UIFORM, 11.11.) | "Port 39000 belegt: gebaut mit 127.0.0.1:39001", "Port 39001 nicht belegt" | ein anderes JK Studio baut gerade auf denselben Ports. **Runde 2:** eigener Portbereich je Lauf (`BHED_BAU_PORT`) |
| karteneditor, handbuch (LCB2, VIS2c, 11.11.) | "Rueckgewinnung fertig: nicht nach 180 s" | die Rueckgewinnung laedt die Modelle in einem Prozess mit niedriger Prioritaet - unter Last 172 s und mehr statt 21-24 s. **Runde 2:** der Schritt wartet auf ihr Ende |
| karteneditor | "Stufe 7: grosse Karte - keine Ruckler: 99. Perzentil des Bildabstands unter 50 ms", "Grosse Karte: ... kein Bild ueber 250 ms", "zweiter Bau: ... Schaetzung gegen die echte Dauer" | Schwellen gegen die Wanduhr: Bildzeit bzw. Dauer zweier Bauten nacheinander; unter Last rot, in beiden Pausenarten (Runde 2, offen - Frage 3) |
| karteneditor (Runde 2, 11.11. abends) | "Doppel-M2 in der Kamera: das Entitaetenmenue" mit zwei Folgezeilen | der zweite Rechtsklick zaehlt nur innerhalb von 250 ms Programmzeit als Doppelklick; unter der Last einer vollen Runde daneben rot im ganzen Lauf, gruen im Teil ke_grund. Offen: die Behebung (beide Klicks in Simulationszeit) liegt in `Y:\werkzeuge\tt\r2\patch_doppelm2.py`, ist aber noch nicht unter der Bremse rot und gruen gezeigt |
| dock2mass, dock5, teil11, fix604b, gegenstaende/kartenfunde, mb_bild_jka, mb_bild2_jka | je 1 FEHLER, gruen beim Wiederholen | unter Last (Fenster/Foto noch nicht fertig) |

Gemeinsame Ursache: Schritte warten eine feste Zeit oder Bildzahl statt auf das Ereignis. Unter Last reicht sie nicht.
Nachstellen laesst sich das ohne Last mit `BHED_TEST_BREMSE=<ms>` (jedes Bild unregelmaessig bis zu so lang).

### Vorschlaege mit geschaetztem Gewinn (Reihenfolge = Umsetzung)

| # | Vorschlag | Gewinn je voller Runde (Rechenzeit von 31623 s) | Stand |
|---|---|---|---|
| 0 | Selbsttests mit normaler Prioritaet (shank 03.11.: erlaubt) | neben Bauten 6-10x je Lauf (gemessen) | Teil B |
| 1 | Pausen nach Bildern mit Simulationszeit statt Wanduhr (`BHED_TEST_PAUSEN=bild`), je Lauf nachgewiesen | geschaetzt bis ~6000 s; gemessen: menueeditor -30 %, kuerzel -29 %, Modus 1 -14 %; umgestellt sind menueeditor und sechs kleinere Laeufe, karte und Modus 1 nicht (Gruende in den Notizen) | Teil B |
| 2 | Doppel aus der vollen Runde nehmen (Tabelle oben) | 2700 s ohne bm, mit bm-Laeufen nur bei Bildaufnahme 6100 s | Teil B |
| 3 | Eine Runde, ein Befehl: `tools/testrunde.py` (Pool, Umgebung, haengende Laeufe, eine Zeile je Lauf) | Wanduhr: laengste zuerst, keine Handarbeit; ersetzt die Kopien `Y:\werkzeuge\*\schluss*.sh`, `pool.py`, `lauf.py` | Teil B |
| 4 | `tools/test_betroffen.py`: aus dem Unterschied die Bereiche | Zweig-Schlussrunde: 1 von 8 Bereichen statt allem (z. B. menue 5900 s statt 31623 s) | Teil B |
| 5 | Zeit je Schrittart im Protokoll ("Knopftest Zeiten: ...") | macht 1 messbar, findet die langen Schritte | Teil B |
| 6 | ctest: `defgpu` in parallele Teile (WARP / Grafikkarte) | ctest-Wanduhr 170-422 s -> ~125-200 s | Teil B |
| 7 | Sprachlaeufe es/cs/ja aus der vollen Runde (Entscheidung shank, kein Doppel) | 9456 s; die Kette der drei karteneditor-Laeufe entfaellt, Wanduhr 79 min -> 37 min (karteneditor en) | Teil B |
| 8 | Rundlaeufe des Modell-Blocks und kartenpruefung auf mehrere Prozesse (`BHED_TEST_ANTEIL=i/n`: zusammenhaengende Abschnitte der Liste), mission1 in Teilen (BHED_M1_TEIL) | Wanduhr: kartenpruefung 25 min -> 4 Anteile nebeneinander; Rechenzeit gleich | Runde 2: kartenpruefung in vier Anteilen (Berichte gleich, 282 / 282); mission1 bleibt ganz (in Teilen nicht gleich), die Rundlaeufe laufen ungeteilt (Nachweis nicht gefahren), allekarten bleibt ganz (Frage 2) |
| 9 | Pausen nach Bildern fuer Modus 1, karteneditor, karte, reiter - nach dem Beheben der Stellen, die an der Wanduhr hingen (Tutorial-Fenster, Messfenster in karte) | nebeneinander gemessen: karteneditor -25 % (2399 -> 1797 s), karte -21 %, Modus 1 -15 % (1121 -> 958 s) | Runde 2 |
| 10 | karteneditor in sechs Teilen nebeneinander (`BHED_KARTE_GRUPPEN`), jeder Lauf mit eigenem Portbereich der Bau-Ueberwachung (`BHED_BAU_PORT`) | Wanduhr: die laengste Strecke der Runde (37 min) zerfaellt in Teile von 2-14 min; Rechenzeit + 5 x gemeinsamer Anfang | Runde 2 (Grenze: Frage 5) |
| 11 | Wackler beheben statt wiederholen: auf das Ereignis warten, Simulationszeit fuer Bedienung, die das Programm an seiner Zeit misst | jeder rote Lauf kostet heute eine Wiederholung (karteneditor 30-40 min, Modus 1 15 min) | Runde 2: sieben Stellen nachgestellt und behoben; Schwellen gegen die Wanduhr offen (Frage 3) |
| 12 | kartenpruefung in den Lauf allekarten legen (jede Karte einmal laden) | rund 1400 s Rechenzeit (9 %) | Vorschlag (Frage 4) |

## Teil B - Umsetzung und Nachweise

Siehe `docs/intern/NOTIZEN_testtempo_1111.md`.

## Entscheidung: volle Runde nur englisch (shank 11.11.)

Auf die Frage, ob die Sprachlaeufe weiter alles wiederholen sollen, shank: "eigentlich nicht wichtig ist erstmal
englisch. wenn da was waere. Muss man mir bescheid sagen von der community."

Das ist kein Doppel, sondern eine Entscheidung: **hier wird weniger geprueft als bis rc611.** Ein Fehler, der nur in
einer anderen Sprache auftritt (ein Text, der in es/cs/ja breiter ist und etwas verschiebt oder abschneidet, eine Stelle,
die nur mit CJK-Schrift bricht) und den keine der verbleibenden Pruefungen unten misst, faellt in der vollen Runde nicht
mehr auf - er kommt dann von der Community.

Aus der vollen Runde genommen (17 Laeufe, 9456 s der Runde rc611), erreichbar mit `tools/testrunde.py --mit-sprachen`
oder einzeln beim Namen:

| Lauf | Zeit rc611 |
|---|---|
| `karteneditor_es`, `karteneditor_cs` | 1353 s, 1129 s |
| `menueeditor_es`, `menueeditor_cs`, `menueeditor_ja` | 1546 s, 1548 s, 1193 s |
| `1_es`, `1_cs` | 854 s, 853 s |
| `hilfe1_es`, `hilfe1_cs`, `hilfe2_es`, `hilfe2_cs`, `tutgeo_es`, `tutgeo_cs` | 77, 75, 179, 203, 20, 20 s |
| `zeitleiste1_es`, `zeitleiste1_cs` | 95 s, 96 s |
| `kuerzel_es`, `kuerzel_cs` (zugleich Doppel von `1_es`/`1_cs`) | 107 s, 108 s |

Was an Sprachpruefungen bleibt (laeuft weiter in jeder vollen Runde):

- **In den englischen Laeufen selbst**, die die Sprache umschalten: `menueeditor` 770 Pruefzeilen mit Sprachnamen (u. a.
  "Breiten Leiste Properties/Events" und "Links" je en, de, zh-Hans, ja, es, cs: jedes Element liegt ganz im Feld, nichts
  abgeschnitten), `karteneditor` 205, Modus `1` 48.
- **Laeufe, die eine andere Sprache brauchen:** `cjkweit_ja`, `cjkweit_zh` (BHED_KARTE_NUR=cjkweit, je 19 Pruefzeilen,
  19 s), `mb_ptfix2` (Sprache ja: Name der Grundebene in der Sprache der Oberflaeche).
- **ctest:** `sprache`, `handbuch_sprachen`, `changelog_sprachen`, `umbruch`, `menue_text`, `lint_cjkumbruch`
  (+ `_selbsttest`).
- Die Bildlaeufe des Handbuchs (handbuch, bm) laufen je Sprache, wenn Bilder neu aufgenommen werden.

## Fragen an shank (nicht umgesetzt, weil es Pruefungen schwaechen koennte)

1. **Handbuchbilder des Menue-Editors (bm, 6 x 570 s):** nur noch laufen lassen, wenn sich Menue-Editor oder Texte
   geaendert haben? Ihre Pruefzeilen stehen alle im vollen Lauf. (In `tools/testrunde.py` laufen sie schon nur auf
   Zuruf: `bm_en` ... `bm_cs`.) **Entschieden (Hauptchat 11.11.): nur auf Zuruf.**
2. **`allekarten` (1278 s) teilen?** Der Lauf laedt alle 282 Karten nacheinander und prueft danach an der ersten Karte,
   ob Speicher liegen blieb; je Karte auch, ob Bilder der VORIGEN Karte im Grafikspeicher stehen. In vier Teilen
   nebeneinander saehe die Speicherpruefung je Teil nur ein Viertel der Karten (ein Leck je Karte wuerde viermal
   schwerer gefunden), und andere Kartenpaare folgten aufeinander. Deshalb ungeteilt - er ist jetzt die laengste Strecke
   der Runde. Teilen nur, wenn shank die schwaechere Speicherpruefung annimmt (oder ein fuenfter, ungeteilter Lauf nur
   fuer sie bleibt - dann ist nichts gewonnen).
3. **Schwellen gegen die Wanduhr** werden unter Last rot, ohne dass am Programm etwas falsch ist: `spulen` "zurueck so
   fluessig wie vor (Mittel und 95 %)", `zeitleiste1` "Filmdurchgang ohne laengere Bilder", karteneditor "Stufe 7: grosse
   Karte - keine Ruckler: 99. Perzentil unter 50 ms", "Grosse Karte: ... kein Bild ueber 250 ms" und "zweiter Bau: ...
   Schaetzung gegen die echte Dauer" (die Dauer zweier Bauten nacheinander). Sie messen Bildzeiten bzw. Bauzeiten - Simulationszeit hilft nicht, und ein anderes Mass
   (Rechenzeit statt Wanduhr) saehe das Warten auf die Grafikkarte nicht mehr. Wege: (a) so lassen und bei Rot den einen
   Lauf wiederholen (heute von Hand), (b) die Messung im Lauf einmal wiederholen, wenn sie rot ist (findet einen Fehler,
   der nur manchmal auftritt, seltener), (c) diese Pruefungen nur im Ruhefenster werten (wie `leistung`). Nicht geaendert.
4. **Vorschlag: `kartenpruefung` in den Lauf `allekarten` legen.** Beide laden jede der 282 Karten, zeichnen zwei Bilder
   und werten aus - der eine schreibt den Bericht je Karte, der andere die Pruefzeile je Karte. In einem Lauf wuerde
   jede Karte einmal geladen: rund 1400 s Rechenzeit weniger je Runde (9 %). Dazu muss der Berichtsschritt (220 Zeilen in
   `gui/selbsttest_vorschau_a.inc`) eine eigene Funktion werden, und der Nachweis braucht drei lange Laeufe; die
   Speicherpruefung von allekarten saehe die Zwischenspeicher des Berichts mit. Nicht umgesetzt - stattdessen laeuft
   kartenpruefung in vier Anteilen nebeneinander (gleiche Rechenzeit, kuerzere Strecke).
5. **karteneditor in Teilen: reichen die Teile, oder soll vor einem Release zusaetzlich der ganze Lauf laufen?** Die
   sechs Teile liefern dieselben Pruefzeilen wie der ganze Lauf (nachgewiesen), aber jeder Teil beginnt mit frischem
   Programm: was eine Gruppe der naechsten hinterlaesst, wirkt an den fuenf Teilgrenzen nicht mehr. Am Rot-Stand war das
   zu sehen - der eingebaute Fehler wurde in beiden gefunden, im ganzen Lauf kamen 13 Folgefehler in spaeteren Gruppen
   dazu, die der Teil nicht zeigt. Ein Fehler, der sich NUR so zeigt (eine Gruppe besteht, verdirbt aber den Zustand
   fuer eine Gruppe im naechsten Teil), fiele in den Teilen nicht auf. Die Runde faehrt jetzt die Teile (Wunsch des
   Hauptchats, Strecke 37 min -> 6-14 min); `python -I tools/testrunde.py --ungeteilt` faehrt den ganzen Lauf.
