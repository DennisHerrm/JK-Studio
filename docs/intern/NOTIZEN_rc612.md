# Notizen rc612: Release-Runde (Changelog, Handbuch, voller Lauf, Messtabellen, Version rc612)

Auftrag `auftrag_rc612_release.md`. Direkt in `Y:\karte` auf `karte1` ab 1f3e4a88d (274 Commits seit rc611 = 41bde01b8, keine
WIP-Commits), Build `build-test` (`--parallel 4 -- /nodeReuse:false`, Leerlauf-Prioritaet), Laeufe `laeufe/rel612_*`, Skripte
`Y:\werkzeuge\rel612\`. Kein Spiel gestartet.

## Version und Changelog
- `kFassung` rc612. Ein rc612-Eintrag in 6 Sprachen aus den Changelog-Abschnitten der 50 seit rc611 neuen oder geaenderten
  Notizen (ap1 bis ap14b, d3d12, dx12, dx12c, dxr, ent1 bis ent3b, fix612 bis fix612c, gdpfad, gitter, klein, klein2, lcb1, lcb2,
  lcfix, mbkopf, mbmal, mbnav, mbraster, mbvers, numpad, rest612, tempo, uiform, vis2, vis2s, vis3, vis4; ohne Eintrag: mbentwurf,
  rot612 und testtempo, nur Selbsttests): Uebersicht oben, dann Direct3D 12, Raytracing-Schatten, Model-Tab, Model-Tab Animation,
  Karten-Editor und Compiler, Behoben. Interne Pruefungs-, Lauf- und Agentennamen nicht genannt.
- Abweichend von der Notiz vis2s ("etwa 5 MB mehr je Thread") sagt der Eintrag nur "braucht mehr Speicher": im Ruhefenster
  braucht VIS mit 32 Threads auf duel_mos_espa 202 -> 371 MB (Regel) bzw. 284 -> 956 MB (`-bhedvisexact`), siehe Messtabelle.
- Nach D3D12-4 (14.11.) eine Zeile mehr im Abschnitt Direct3D 12 (weniger Arbeit fuer die Grafikkarte); die Zeile ueber die
  Messung des Leistungs-Selbsttests nicht (betrifft keine Bedienung).
- `tools/changelog.txt`, `AENDERUNGEN.md`, `gen_changelog.py` (41 Fassungen x 6 Sprachen), lint_fassung ok (rc612).

## Handbuch
- "Was ist neu"-Stellen mit Fassungsnummer gibt es im Handbuch nicht (die Knoepfe "Was ist neu?" der Update-Leiste oeffnen die
  Notizen des Releases). Fassungsnummern stehen nur als Geschichte: "vor rc600 hiess der Tab Map", "seit rc601", "bis rc605
  BehavEd-Remake", "bis rc611 in der Zeitleiste", "seit rc612 sind Dopesheet und Kurveneditor Spuren der Zeitleiste" - alle
  richtig, in allen 6 Sprachen gleich.
- `gen_handbuch.py` neu laufen lassen: kein Unterschied zum eingecheckten Stand (184 Seiten x 6 Sprachen, Suchindex `suche.js`
  aktuell). `--pruefen`: handbuch_sprachen ok, es/cs 0 Texte und 0 Bilder noch englisch (Hinweis: 2 Eintraege in
  `uebersetzung/` werden nicht mehr gebraucht).

## Testrunde nachgezogen (`tools/testrunde.py`)
- Die Gruppe `dxr` des vollen karteneditor-Laufs stand in keinem Teil `ke_*`: die volle Runde liess die Raytracing-Schatten still
  weg. Neu prueft die Runde beim Start, dass jede Gruppe (`gruppe("...")` in `gui/selbsttest_handbuch.inc`) in genau einem Teil
  steht - rot mit dem Stand 1f3e4a88d ("Gruppe dxr ... steht in keinem ke_*-Teil"), gruen nach dem Eintrag `dxr` in `ke_karte`.
- In keinem Lauf standen die Gruppen der Zweige nach TESTTEMPO: `mb_ent3a`, `mb_ent3b`, `mb_fix612`, `mb_fix612b`,
  `mb_fix612babsturz`, `mb_fix612c`, `ke_fix612` (`BHED_KARTE_NUR=fix612`) und der Modus `grafik` (Vorgabe, Direct3D 11, die sechs
  gespielten Fehlerstellen des Starts, der abgestuerzte Start). Jetzt 191 Laeufe in der Runde (vorher 175).

## Voller Lauf
Englisch (Sprachlaeufe es/cs/ja entfallen, Entscheidung 11.11.), Direct3D 12 (Vorgabe), Pool 4, Leerlauf-Prioritaet, eigene Laufordner.

| Teil | Stand | Ergebnis |
|---|---|---|
| Neubau `--clean-first` (n1) | 1f3e4a88d + Changelog | exit 0, 1062 s, 0 Fehler, 0 Warnungen; danach inkrementell n2-n5 je 0/0 |
| ctest c1 / c2 | vor / nach den Behebungen der Runde 1 | 273/273 (348 s) / 273/273 (239 s) |
| ctest c3 | Endstand mit D3D12-4 | 273/273 (181 s); ein frueherer c3 neben lints.py 272/273 (q3map2_baupause, siehe unten) |
| ctest c4 q3map2_baupause allein | Endstand | 1/1 gruen (23 s) |
| Bau n6 + ctest c5 | Endstand mit den Testfixes der Nachlaeufe | 0 Fehler, 0 Warnungen (195 s); 273/273 (181 s) |
| Lints | Endstand | wie rc611: portability 101 -> 97 (Include-Fix), pruefcodes 11, fremdprogramm 1, efxflags wie immer, Rest ok; link/tidy nur Linux |
| Runde 1 (`rel612_`, 191 Laeufe) | 1f3e4a88d + Changelog + Testrunde | 187 gruen, 4 rot (allekarten, 1, menueeditor, eigenstaendig), 4045 s Wanduhr, 16068 s Summe |
| Runde 2 (`rel612b_`, die roten und die mit Exit-Radiant-Zeile) | nach den Behebungen | 9/9 gruen, 597 s |
| Runde 4 (`rel612d12_`, Zeichengruppen + grafik, D3D12) | Endstand mit D3D12-4 | 48/48 gruen, 938 s |
| Runde 3 (`rel612d11_`, Zeichengruppen, D3D11 `BHED_D3D12=0`) | Endstand mit D3D12-4 | 42/43, ke_karte rot (Testfehler, siehe unten) -> nach dem Testfix ke_karte unter D3D11 292/0 (ohne `BHED_DXR_ERWARTET`), unter D3D12 308/0 |
| ke_grund `BHED_TEST_BREMSE=400` | Endstand | vorher 753/7 (Zeitstellen des Tests), nach den Testfixes 760/0 (2482 s); ohne Bremse 760/0 |

Jede FEHLER-Zeile geklaert (Rot -> Ursache -> Gruen):
- **1 / eigenstaendig** "ohne auspackbare Daten endet der Start mit 2": Code 0xC0000409 statt 2 - **Programmfehler** aus D3D12-3: der
  Vorwaermfaden des Direct3D-12-Starts war ein globaler `std::thread`; kehrt `main` vor dem Start der Grafik mit 2 zurueck, beendet
  sein Destruktor den Prozess (`std::terminate`). rc611 gab 2. Jetzt joint eine Huelle den Faden beim Prozessende
  (`backend_d3d12_win32.cpp`). Gruen in Runde 2 (eigenstaendig 9/0, 1 2638/0).
- **menueeditor** Klick "BOTH_STAND1" "nicht ganz sichtbar (2080..2613, sichtbar 2075..2538)" - **Programmfehler** aus KLEIN2:
  `ui::popup` legt ein Popup beim Aufgehen in das Fenster; waechst die Liste danach, legte ImGui es nicht neu. Jetzt rueckt
  `nachruecken()` (`ui_menue.cpp`) es zurueck ins Fenster. Gruen in Runde 2 (2790/0) und unter D3D11.
- **allekarten** "Speicher: nichts liegen geblieben": Grafik +65,6 MB (Grenze 64; rc611 unter D3D11 +4,4) - **Testfehler**: der
  Vorrat der Hochladepuffer des Direct3D-12-Abgebers (hoechstens 64 MB, nach 600 Bildern zurueckgegeben) zaehlte als liegen
  geblieben. Die Pruefung zieht den Vorrat ab und schreibt die Speicherzeilen ins Protokoll: "Zuwachs Grafik 2,5 MB (ohne den
  Vorrat der Puffer: vorher 0,0, nachher 63,1 MB)". Gruen in Runde 2 (285/0). Die Grenze bleibt 64 MB.
- **hilfe2, modellsetzen, dock4, tabfenster**: gruen, aber nach der Zusammenfassung "Beenden: Programm laeuft nach No weiter (Exit
  Radiant)" (auch in rc611) - **Testfehler**: die Antwort "No" des Selbsttests kannte die Frage des Map Editors nicht; jetzt Discard
  (`selbsttest_vorschau_b.inc`). Zeile weg in Runde 2.
- **ke_karte nur mit Pausen nach Bildern** "DXR: nach der Ruhezeit ist ein schnell gebauter Eimer einmal auf schnelle Strahlen
  nachgezogen" - **Testfehler**: die Ruhezeit (0,5 s Wanduhr im Programm) wurde in Bildern gewartet; jetzt nach der Wanduhr. Gruen in
  Runde 2 (308/0).
- **ke_karte unter Direct3D 11** (20 FEHLER, alle DXR, erste: "das Geraet ist, was der Lauf erwartet (BHED_DXR_ERWARTET)": Grund 1
  "need Direct3D 12, and Direct3D 11 is running", erwartet 0) - **Testfehler**: der DXR-Teil prueft, was der Lauf erwartet
  (`BHED_DXR_ERWARTET`, Vorgabe: DXR geht); der Lauf unter Direct3D 11 setzte nur `BHED_D3D12=0`. Das Programm meldet richtig, dass
  Raytracing Direct3D 12 braucht. Jetzt erwartet ein Lauf mit `BHED_D3D12=0` ohne `BHED_DXR_ERWARTET` den Grund "Direct3D 11 laeuft"
  (dieselbe Regel wie `main_win32.cpp`); ohne die Variable unter Direct3D 12 bleibt "DXR geht" die Erwartung, die Gegenprobe (Geraet
  ohne DXR gespielt, keine Erwartung) bleibt rot. Nach dem Fix (Bau n6) ke_karte unter Direct3D 11 292/0 (DXR: "Grund 1 (erwartet 1)", Bild Bildpunkt fuer Bildpunkt wie die
  Vorgabe, Menuepunkt ausgegraut, Klick schaltet nichts), unter Direct3D 12 308/0 (22 DXR-Zeilen OK).
- **ke_grund mit `BHED_TEST_BREMSE=400`** (jedes Bild bis 400 ms, nicht Teil der Runde) - vorher 753/7, alle an Stellen, an denen das Programm die Bedienung an seiner Zeit misst und der Test die
  Ereignisse ein langes Bild auseinander legte: "W im Freelook ... danach steht die Kamera" (313 vorwaerts statt hoechstens 260 -
  das Halten von 0,4 s ueberzog um ein Bild), "Freelook aus" (Druck und Loslassen ueber 300 ms = kein kurzer Klick, NRCs
  `freelookEndetBeimLoslassen`) mit drei Folgezeilen (diskrete Pfeil-/A-Tasten im noch offenen Freelook), "Doppelklick: Verzeichnis
  des Shaders" (DoubleClickTimer 250 ms wie NRC), "Doppelklick: Split" (Clipper 200 ms). Das Programm verhaelt sich dabei wie NRC -
  **Testfehler**: wie der TESTTEMPO-Patch fuer den Doppel-M2 laufen jetzt der kurze Rechtsklick der Kamera, das gehaltene W, die
  Doppelklicks von `klickAn` und `doppelklickXy` in Simulationszeit (je Bild hoechstens 1/60 s Programmzeit; bei schnellen Bildern
  aendert sich nichts). Die Pruefungen und ihre Grenzen bleiben. Nach dem Fix 760/0 mit Bremse, 760/0 ohne. "Zweimal: der zweite um
  max( 8, Raster )" war in beiden Bremse-Laeufen gruen. Die anderen Laeufe mit Doppelklicks (`pruefung`, `mb_ap3`, `ke_bau`,
  `modobj`) auf Bau n6: alle gruen, gleich viele Pruefzeilen wie in Runde 1 bzw. 4 (pruefung 25/0, mb_ap3 106/0, ke_bau 622/0, modobj 27/0).
- **q3map2_baupause** (ctest c3 neben lints.py): "lichtabbruch: der naechste Licht-Schritt nimmt Durchgang 1 und 2 aus dem Cache" -
  es waren 3 von 5 Durchgaengen fertig, als der Abbruch kam. **Zeitrennen des Tests unter Last**: der Test liest die Ausgabe von q3map2 (Leerlauf-Prioritaet) und beendet es, sobald
  "light cache pass 2 of 5" gemeldet ist; neben lints.py las er die Zeile erst, als Durchgang 3 schon fertig war. Allein auf dem
  Endstand gruen (ctest c4 `-j 1 -R q3map2_baupause`), ebenso c1, c2, c3 und c5. Kein Programmfehler, Test unveraendert.
- Bekannte Kandidaten: ke_licht "99. Perzentil des Bildabstands unter 50 ms" gruen in Runde 1, 3 und 4; ke_grund "Zweimal: der
  zweite um max( 8, Raster )" ohne Bremse gruen; mb_rig "MB4 Pfad" gruen in Runde 1, 3 und 4 (nach ROT612 bestaetigt).
- Uebersprungen statt FEHLER: die Mehrspieler-Menues von Movie Duels (Daten bewusst geloescht, 05.11.).

## Messtabellen des Ruhefensters
Ruhefenster 13.11. 21:16-23:17 (Kette `Y:\werkzeuge\ruhefenster_1311.sh`, keine fremden Bau- oder Testprozesse beim Start jeder
Messung; Fremdlast = Rechenzeit aller anderen Prozesse in Kernen, steht je Lauf in den Dateien). Dazu vom 12.11. 14:03-14:24 vis3
und vis4. Zahlen wie gemessen; '!' = mehr als 3 % schlechter und ausserhalb der Spanne der Runden, '+' = besser.

### Direct3D 12 gegen Direct3D 11 - gueltig ist die Endmessung von D3D12-4 (`Y:\werkzeuge\dx12d\messung_final.txt`, 14.11. nachts, allein, Fremdlast 0,5-0,7 Kerne, Anlauf + 3 Runden abwechselnd)
**Die Tabelle des Ruhefensters vom 13.11. (`Y:\werkzeuge\dx12\messung_final.txt`) hatte einen Messfehler im D3D12-Weg:** Die
Zeitmarken zaehlten die Pausen der Grafikkarte zwischen den Teilabgaben eines Bildes mit, und die Messung schickte selbst eine
Liste mehr ab. Ihre GPU-Spalte (dort +6 bis +109 %) und der Bildabstand "Mission Szene spielt" (+20 %) gelten nicht mehr. D3D12-4
hat die Messung berichtigt (gegen die GPU-Zeit nach Windows geprueft) und vier echte Ursachen behoben (`NOTIZEN_dx12d_1411.md`);
gemessen auf dem Endstand dieser Runde.

**Kernaussage:** Direct3D 12 haelt in allen 18 Ansichten den Bildabstand von Direct3D 11 (hoechstens +2,4 %) und braucht in 16 von
18 Ansichten gleich viel oder weniger GPU-Zeit (Grenze +-3 %, in leichten Ansichten unter 0,5 ms auch innerhalb der Spanne der
D3D11-Runden, so Model Animation +4 %); darueber liegen Menue "Maus bewegt" (+3,8 %, 0,390 / 0,405 ms,
innerhalb der Spanne der D3D11-Runden) und der Karten-Editor duel_yavin mit Halmen (+6,5 %, 3,072 / 3,273 ms, Ursache offen). Die
CPU-Spalte (Wanduhr bis vor Present) liegt in leichten Ansichten und im Echtzeit-Licht 5-26 % hoeher, weil der Hauptfaden unter
Direct3D 12 im Messfenster auf das vorige Present wartet; die Rechenzeit des Hauptfadens (Profil) ist unter Direct3D 12 in 13 von 18
Ansichten niedriger. Arbeitsspeicher nach der .map privat 7429 / 7719 MB (+0,29 GB), Grafikspeicher 2441 / 2765 MB.

| Ansicht | Bildabstand ms D3D11 / D3D12 | GPU ms D3D11 / D3D12 | CPU-Spalte ms (Wanduhr) D3D11 / D3D12 |
|---|---|---|---|
| Mission mp/duel1 ruhend | 4,17 / 4,18 (+0 %) | 0,162 / 0,154 (-5 %) | 0,44 / 0,46 (+5 %) |
| Mission md_toaj_jedi ruhend | 4,30 / 4,34 (+1 %) | 0,183 / 0,184 (+1 %) | 0,63 / 0,66 (+5 %) |
| Mission md_toaj_jedi Flug | 5,45 / 5,23 (-4 %) | 1,690 / 1,284 (-24 %) | 3,09 / 3,09 (+0 %) |
| Mission Szene angehalten | 4,50 / 4,60 (+2 %) | 0,444 / 0,443 (-0 %) | 0,95 / 1,00 (+5 %) |
| Mission Szene spielt | 4,18 / 4,23 (+1 %) | 2,094 / 1,539 (-27 %) | 3,47 / 3,46 (-0 %) |
| Mission duel_mustafar ruhend | 4,17 / 4,18 (+0 %) | 0,389 / 0,382 (-2 %) | 1,01 / 1,09 (+8 %) |
| Map Editor Vorlage wie im Spiel ruhend | 4,17 / 4,20 (+1 %) | 2,507 / 2,486 (-1 %) | 2,19 / 2,24 (+2 %) |
| Map Editor Vorlage wie im Spiel Kamera dreht | 4,17 / 4,18 (+0 %) | 2,084 / 2,033 (-2 %) | 2,14 / 2,16 (+1 %) |
| Map Editor duel_yavin ohne Halme | 4,17 / 4,22 (+1 %) | 3,011 / 3,011 (+0 %) | 1,98 / 2,06 (+4 %) |
| Map Editor duel_yavin mit Halmen | 4,18 / 4,20 (+0 %) | 3,072 / 3,273 (+7 %) | 2,08 / 2,12 (+2 %) |
| Map Editor md_am_sith ohne Effekte | 4,18 / 4,22 (+1 %) | 2,621 / 2,639 (+1 %) | 1,52 / 1,74 (+14 %) |
| Map Editor md_am_sith mit Effekten | 4,18 / 4,18 (+0 %) | 2,834 / 2,706 (-5 %) | 2,43 / 2,41 (-1 %) |
| Map Editor Echtzeit-Licht ruhend | 22,50 / 21,97 (-2 %) | 22,430 / 21,954 (-2 %) | 3,36 / 3,72 (+11 %) |
| Map Editor Echtzeit-Licht Kamera dreht | 12,03 / 12,03 (+0 %) | 11,792 / 11,943 (+1 %) | 3,02 / 3,53 (+17 %) |
| Model stormtrooper ruhend | 4,20 / 4,25 (+1 %) | 0,403 / 0,391 (-3 %) | 0,62 / 0,73 (+18 %) |
| Model stormtrooper Animation und Drehteller | 4,25 / 4,25 (+0 %) | 0,449 / 0,466 (+4 %) | 0,72 / 0,85 (+18 %) |
| Menu Editor ui/main.menu ruhend | 4,29 / 4,27 (-0 %) | 0,398 / 0,394 (-1 %) | 0,50 / 0,62 (+24 %) |
| Menu Editor ui/main.menu Maus bewegt | 4,17 / 4,27 (+2 %) | 0,390 / 0,405 (+4 %) | 0,42 / 0,53 (+26 %) |

**Hauptfaden-Rechenzeit je Bild (Profil der Endmessung, ms, D3D11 / D3D12):** duel1 0,59 / 0,42, toaj ruhend 0,71 / 0,55, Flug
2,45 / 2,23, Szene angehalten 1,15 / 1,00, Szene spielt 3,62 / 3,65, mustafar 1,45 / 1,13, Vorlage ruhend 2,28 / 2,36, Vorlage dreht
2,29 / 2,42, yavin ohne 2,12 / 2,18, yavin mit 2,31 / 2,25, md_am_sith ohne 1,72 / 1,75, mit 2,65 / 2,58, Echtzeit-Licht ruhend
2,34 / 2,13, dreht 3,08 / 2,90, Model 0,83 / 0,67 und 0,94 / 0,73, Menue 0,66 / 0,46 und 0,67 / 0,47. Unter Direct3D 11 enthaelt
die Zahl Present (1-19 %), unter Direct3D 12 laeuft Present auf dem Abschickfaden. Treiberfaden D3D11 0,27-1,88 ms je Bild.

### Raytracing-Schatten gegen Schattenkarten (`Y:\werkzeuge\dxr\messung_final.txt`, 21:38-21:47, je 3 Runden, Fremdlast 2,5-3,5 Kerne)
**Kernaussage:** Strahlen statt Schattenkarten kosten im Echtzeit-Licht 3,5 bis 4,7 % Bildabstand (ruhend 22,64 -> 23,44 ms),
ohne Echtzeit-Licht keinen; sie sparen rund 2,3 GB privaten und 1,8 GB Grafikspeicher (die Schattenkarten entfallen, die
Beschleunigungsstrukturen der Karte brauchen 13 MB), die Karte oeffnet 15 % schneller. Die Vorgabe (Schattenkarten) gegen eine
Basis kurz vor 73c18113f: 3 von 4 Fotos bitgleich, anders das Fensterfoto `leistung_editor` (4425 Bildpunkte, bis 187/255) - als
Maske angesehen (`Y:\werkzeuge\rel612\le_mask.png`): nur der Text der Statuszeile unten links und rechts, die Ansichten und der
Texturbrowser sind gleich.

| Ansicht | Bildabstand ms Karten / Strahlen | CPU ms Karten / Strahlen | GPU ms Karten / Strahlen |
|---|---|---|---|
| Map Editor Vorlage wie im Spiel ruhend | 4,48 / 4,44 (-0,9 %) | 4,06 / 3,84 (-5,4 %) | 3,802 / 3,663 (-3,7 %) |
| Map Editor Vorlage wie im Spiel Kamera dreht | 4,45 / 4,29 (-3,6 %) | 4,08 / 3,97 (-2,7 %) | 3,612 / 3,567 (-1,2 %) |
| Map Editor duel_yavin ohne Halme | 4,19 / 4,22 (+0,7 %) | 3,25 / 3,04 (-6,5 %) | 3,743 / 3,571 (-4,6 %) |
| Map Editor duel_yavin mit Halmen | 4,27 / 4,28 (+0,2 %) | 3,41 / 3,30 (-3,2 %) | 4,014 / 3,891 (-3,1 %) |
| Map Editor md_am_sith ohne Effekte | 4,18 / 4,29 (+2,6 %) | 2,68 / 2,54 (-5,2 %) | 3,610 / 3,612 (+0,1 %) |
| Map Editor md_am_sith mit Effekten | 4,73 / 4,74 (+0,2 %) | 4,08 / 4,07 (-0,2 %) | 4,372 / 4,430 (+1,3 %) |
| Map Editor Echtzeit-Licht ruhend | 22,64 / 23,44 (+3,5 % !) | 6,63 / 6,35 (-4,2 %) | 22,611 / 23,429 (+3,6 % !) |
| Map Editor Echtzeit-Licht Kamera dreht | 12,27 / 12,85 (+4,7 % !) | 5,46 / 5,02 (-8,1 %) | 12,213 / 12,713 (+4,1 %) |

| Start und Speicher | Schattenkarten | Strahlen | Diff |
|---|---|---|---|
| Programmstart bis erstes Bild ms | 1141,82 | 1074,41 | -5,9 % |
| Editor oeffnen ms | 8787 | 7437 | -15,4 % + |
| Speicher nach der .map Spitze MB | 3406 | 3070 | -9,9 % + |
| Speicher nach der .map Arbeitssatz MB | 3357 | 3025 | -9,9 % + |
| Speicher nach der .map privat MB | 7480 | 5224 | -30,2 % + |
| Speicher nach der .map Grafik MB | 2815 | 982 | -65,1 % + |
| Speicher Ende Spitze MB | 3406 | 3070 | -9,9 % + |
| Speicher Ende Arbeitssatz MB | 2974 | 2655 | -10,7 % + |
| Speicher Ende privat MB | 7113 | 4847 | -31,9 % + |
| Speicher Ende Grafik MB | 2655 | 822 | -69,0 % + |

### Tempo: alt gegen neu (`Y:\werkzeuge\tempo\messung_final.txt`, 21:48-22:21, Fremdlast 1,8-7,0 Kerne)
**Kernaussage:** Unter Direct3D 12 (3 Runden abwechselnd) braucht der neue Stand in der Mission 14-25 % und im Karten-Editor 7-45 %
weniger Prozessorzeit je Bild (Echtzeit-Licht ruhend -45 %), mit drehender Kamera im Echtzeit-Licht +2 % Zyklen und +11 %
Wanduhr, im Model-Tab -6 bis +6 %, im Menue-Editor +10 % (0,59 -> 0,65 ms). Die Laeufe unter Direct3D 11 und auf dem Weg des
Anwenders (je nur 1 Runde) zeigen dagegen Mission +26 bis +108 % bzw. Karten-Editor +16 bis +60 % - unter hoher Fremdlast
(bis 7,0 Kerne im neu-Lauf) und nicht wiederholt, also nicht belastbar, aber so gemessen.

*Direct3D 12: alt (A) gegen neu (B), Median der Runden 1,2,3 (min..max)*

| Ansicht | Zyklen ms alt / neu | CPU-Wand ms alt / neu | Bildabstand ms alt / neu |
|---|---|---|---|
| Mission mp/duel1 ruhend | 0,601 / 0,474 (-21,1 %) | 0,54 / 0,46 (-14,8 %) | 4,20 / 4,18 |
| Mission md_toaj_jedi ruhend | 0,757 / 0,634 (-16,2 %) | 0,83 / 0,65 (-21,7 %) | 4,27 / 4,17 |
| Mission md_toaj_jedi Flug | 3,339 / 2,873 (-14,0 %) | 3,99 / 3,50 (-12,3 %) | 5,55 / 5,35 |
| Mission Szene angehalten | 3,525 / 2,751 (-22,0 %) | 1,45 / 1,21 (-16,6 %) | 4,18 / 4,18 |
| Mission Szene spielt | 6,958 / 5,483 (-21,2 %) | 5,71 / 5,00 (-12,4 %) | 6,12 / 5,45 |
| Mission duel_mustafar ruhend | 2,078 / 1,558 (-25,0 %) | 1,97 / 1,52 (-22,8 %) | 4,24 / 4,19 |
| Map Editor Vorlage wie im Spiel ruhend | 3,919 / 2,895 (-26,1 %) | 3,87 / 2,87 (-25,8 %) | 4,38 / 4,27 |
| Map Editor Vorlage wie im Spiel Kamera dreht | 3,909 / 3,015 (-22,9 %) | 3,93 / 3,03 (-22,9 %) | 4,33 / 4,36 |
| Map Editor duel_yavin ohne Halme | 3,912 / 3,505 (-10,4 %) | 3,64 / 3,54 (-2,7 %) | 4,41 / 4,98 |
| Map Editor duel_yavin mit Halmen | 3,793 / 3,362 (-11,4 %) | 3,54 / 3,30 (-6,8 %) | 4,27 / 4,25 |
| Map Editor md_am_sith ohne Effekte | 2,804 / 2,611 (-6,9 %) | 2,83 / 2,72 (-3,9 %) | 4,24 / 4,42 |
| Map Editor md_am_sith mit Effekten | 5,296 / 4,308 (-18,7 %) | 5,33 / 4,13 (-22,5 %) | 5,93 / 4,84 |
| Map Editor Echtzeit-Licht ruhend | 5,944 / 3,291 (-44,6 %) | 5,97 / 3,62 (-39,4 %) | 22,38 / 22,55 |
| Map Editor Echtzeit-Licht Kamera dreht | 5,010 / 5,111 (+2,0 %) | 4,90 / 5,46 (+11,4 %) | 12,05 / 12,34 |
| Model stormtrooper ruhend | 0,970 / 0,909 (-6,3 %) | 1,01 / 0,90 (-10,9 %) | 4,22 / 4,17 |
| Model stormtrooper Animation und Drehteller | 1,035 / 1,061 (+2,5 %) | 1,05 / 1,13 (+7,6 %) | 4,17 / 4,16 |
| Model stormtrooper Control Rig ruhend | 0,881 / 0,930 (+5,6 %) | 0,98 / 1,02 (+4,1 %) | 4,17 / 4,18 |
| Model stormtrooper Control Rig Animation | 1,101 / 1,107 (+0,5 %) | 1,08 / 1,21 (+12,0 %) | 4,16 / 4,29 |
| Menu Editor ui/main.menu ruhend | 0,591 / 0,654 (+10,7 %) | 0,58 / 0,68 (+17,2 %) | 4,17 / 4,18 |
| Menu Editor ui/main.menu Maus bewegt | 0,566 / 0,623 (+10,1 %) | 0,54 / 0,69 (+27,8 %) | 4,17 / 4,22 |

*Direct3D 11: alt (A) gegen neu (B)*

| Ansicht | Zyklen ms alt / neu | CPU-Wand ms alt / neu | Bildabstand ms alt / neu |
|---|---|---|---|
| Mission mp/duel1 ruhend | 0,697 / 0,962 (+38,0 %) | 0,44 / 0,60 (+36,4 %) | 4,17 / 4,17 |
| Mission md_toaj_jedi ruhend | 0,737 / 1,202 (+63,1 %) | 0,52 / 0,87 (+67,3 %) | 4,17 / 4,17 |
| Mission md_toaj_jedi Flug | 3,219 / 4,057 (+26,0 %) | 3,10 / 4,31 (+39,0 %) | 5,25 / 5,92 |
| Mission Szene angehalten | 2,556 / 5,322 (+108,2 %) | 1,03 / 1,74 (+68,9 %) | 4,29 / 4,56 |
| Mission Szene spielt | 5,172 / 7,263 (+40,4 %) | 3,97 / 5,88 (+48,1 %) | 4,30 / 6,17 |
| Mission duel_mustafar ruhend | 1,922 / 2,465 (+28,3 %) | 1,60 / 1,79 (+11,9 %) | 4,18 / 4,20 |
| Map Editor Vorlage wie im Spiel ruhend | 4,429 / 4,138 (-6,6 %) | 4,24 / 3,88 (-8,5 %) | 4,84 / 4,46 |
| Map Editor Vorlage wie im Spiel Kamera dreht | 3,847 / 3,650 (-5,1 %) | 3,57 / 3,42 (-4,2 %) | 4,36 / 4,26 |
| Map Editor duel_yavin ohne Halme | 3,823 / 3,543 (-7,3 %) | 3,71 / 3,34 (-10,0 %) | 4,69 / 4,28 |
| Map Editor duel_yavin mit Halmen | 3,922 / 3,971 (+1,2 %) | 3,80 / 3,75 (-1,3 %) | 5,44 / 4,45 |
| Map Editor md_am_sith ohne Effekte | 2,965 / 2,420 (-18,4 %) | 2,79 / 2,23 (-20,1 %) | 4,23 / 4,19 |
| Map Editor md_am_sith mit Effekten | 4,097 / 3,985 (-2,7 %) | 3,92 / 3,77 (-3,8 %) | 4,47 / 4,51 |
| Map Editor Echtzeit-Licht ruhend | 5,470 / 2,880 (-47,3 %) | 5,83 / 2,75 (-52,8 %) | 21,80 / 22,13 |
| Map Editor Echtzeit-Licht Kamera dreht | 5,040 / 4,206 (-16,5 %) | 4,89 / 4,25 (-13,1 %) | 12,17 / 12,07 |
| Model stormtrooper ruhend | 0,854 / 1,096 (+28,3 %) | 0,70 / 0,87 (+24,3 %) | 4,16 / 4,18 |
| Model stormtrooper Animation und Drehteller | 0,998 / 1,229 (+23,1 %) | 0,89 / 0,95 (+6,7 %) | 4,17 / 4,17 |
| Model stormtrooper Control Rig ruhend | 0,870 / 0,976 (+12,2 %) | 0,72 / 0,79 (+9,7 %) | 4,17 / 4,17 |
| Model stormtrooper Control Rig Animation | 0,971 / 1,213 (+24,9 %) | 0,83 / 0,98 (+18,1 %) | 4,17 / 4,17 |
| Menu Editor ui/main.menu ruhend | 0,718 / 0,874 (+21,7 %) | 0,50 / 0,61 (+22,0 %) | 4,17 / 4,17 |
| Menu Editor ui/main.menu Maus bewegt | 0,646 / 0,834 (+29,1 %) | 0,51 / 0,60 (+17,6 %) | 4,25 / 4,18 |

*Direct3D 12 auf dem Weg des Anwenders (BHED_APPDATA): alt (A) gegen neu (B)*

| Ansicht | Zyklen ms alt / neu | CPU-Wand ms alt / neu | Bildabstand ms alt / neu |
|---|---|---|---|
| Mission mp/duel1 ruhend | 0,668 / 0,713 (+6,7 %) | 0,64 / 0,65 (+1,6 %) | 4,17 / 4,16 |
| Mission md_toaj_jedi ruhend | 0,669 / 0,943 (+41,0 %) | 0,66 / 1,04 (+57,6 %) | 4,16 / 4,17 |
| Mission md_toaj_jedi Flug | 3,127 / 4,216 (+34,8 %) | 3,43 / 5,28 (+53,9 %) | 5,32 / 6,25 |
| Mission Szene angehalten | 2,836 / 3,411 (+20,3 %) | 1,14 / 1,41 (+23,7 %) | 4,17 / 4,18 |
| Mission Szene spielt | 5,823 / 7,522 (+29,2 %) | 5,27 / 6,72 (+27,5 %) | 5,45 / 7,06 |
| Mission duel_mustafar ruhend | 1,212 / 3,098 (+155,6 %) | 1,36 / 2,15 (+58,1 %) | 4,34 / 4,20 |
| Map Editor Vorlage wie im Spiel ruhend | 3,500 / 4,257 (+21,6 %) | 3,45 / 4,35 (+26,1 %) | 4,36 / 6,94 |
| Map Editor Vorlage wie im Spiel Kamera dreht | 3,962 / 4,710 (+18,9 %) | 4,16 / 4,86 (+16,8 %) | 4,76 / 6,43 |
| Map Editor duel_yavin ohne Halme | 3,258 / 3,788 (+16,3 %) | 3,30 / 3,95 (+19,7 %) | 5,33 / 8,25 |
| Map Editor duel_yavin mit Halmen | 3,192 / 4,141 (+29,7 %) | 3,45 / 4,20 (+21,7 %) | 5,72 / 9,11 |
| Map Editor md_am_sith ohne Effekte | 2,381 / 3,713 (+55,9 %) | 2,42 / 3,77 (+55,8 %) | 5,36 / 8,09 |
| Map Editor md_am_sith mit Effekten | 3,745 / 5,981 (+59,7 %) | 3,77 / 6,08 (+61,3 %) | 5,38 / 8,52 |
| Map Editor Echtzeit-Licht ruhend | 6,498 / 4,667 (-28,2 %) | 6,60 / 4,86 (-26,4 %) | 33,05 / 44,60 |
| Map Editor Echtzeit-Licht Kamera dreht | 4,762 / 5,844 (+22,7 %) | 4,71 / 6,10 (+29,5 %) | 14,72 / 18,54 |
| Model stormtrooper ruhend | 0,784 / 0,986 (+25,8 %) | 0,82 / 1,01 (+23,2 %) | 4,17 / 4,17 |
| Model stormtrooper Animation und Drehteller | 1,261 / 1,131 (-10,3 %) | 1,38 / 1,13 (-18,1 %) | 4,17 / 4,17 |
| Model stormtrooper Control Rig ruhend | 0,836 / 0,634 (-24,2 %) | 0,84 / 0,74 (-11,9 %) | 4,17 / 4,27 |
| Model stormtrooper Control Rig Animation | 1,053 / 1,157 (+9,9 %) | 1,07 / 1,25 (+16,8 %) | 4,18 / 4,20 |
| Menu Editor ui/main.menu ruhend | 0,639 / 0,604 (-5,5 %) | 0,66 / 0,69 (+4,5 %) | 4,17 / 4,25 |
| Menu Editor ui/main.menu Maus bewegt | 0,639 / 0,652 (+2,0 %) | 0,66 / 0,67 (+1,5 %) | 4,17 / 4,17 |

*Was der Selbsttest-Weg (BHED_EINSTELLUNGEN) am alten Stand kostet: Anwender-Weg (A) gegen Selbsttest-Weg (B), Runde 1*

| Ansicht | Zyklen ms alt / neu | CPU-Wand ms alt / neu | Bildabstand ms alt / neu |
|---|---|---|---|

### Licht-Cache-Abdruck (`Y:\werkzeuge\lcfix\messung_final.txt`, 22:20-22:21, Fremdlast 0)
**Kernaussage:** Die Korrekturen am Abdruck kosten auf der kleinen Testkarte keine messbare Zeit (voller Bau mit Cache 2,6 s gegen
3,2 s vorher, Teilbau 3,1 s gegen 2,7 s - beides innerhalb der Streuung eines 3-s-Baus), der Teilbau nimmt wie vorher 28 von 144
Lightmaps aus dem Cache, und der volle Bau ist bytegleich mit dem alten.

| Schritt (-fast -patchshadows -samples 2 -filter -bounce 2) | neu Wand s | neu CPU s | vorher (73c18113f) Wand s | vorher CPU s |
|---|---|---|---|---|
| Licht voll, legt den Cache an | 2,6 | 27,5 | 3,2 | 29,6 |
| Licht Teilbau (ein Licht dazu) | 3,1 | 29,7 | 2,7 | 28,1 |
| aus dem Cache | 28 von 144 Lightmaps, 1 von 30 Vertexlicht-Flaechen | | 28 von 144, 1 von 30 | |

### VIS schneller (`Y:\werkzeuge\vis2s\messung_final.txt`, 22:21-22:42, Fremdlast 1,4-6,4 Kerne)
**Kernaussage:** Der neue Fluss braucht 25-44 % weniger Rechenzeit (ein Thread 0,56-0,75 der Basis, 32 Threads 0,62-0,74) und
mit 32 Threads 33-52 % weniger Wanduhr, bei denselben Sichtbarkeitsdaten, wo das Ergebnis reproduzierbar ist; dafuer steigt der
Spitzenspeicher deutlich (duel_mos_espa 202 -> 371 MB, deterministisch 284 -> 956 MB).

| Karte | Art | Laeufe basis/neu | Wand s basis -> neu | CPU s basis -> neu | Wand / CPU gegen basis | Spitze MB basis -> neu | Ergebnis |
|---|---|---|---|---|---|---|---|
| ctf3 | 1 Thread | 2/2 | 10,6 -> 8,1 | 10,2 -> 7,6 | 0,76 / 0,75 | 54 -> 59 | gleich -threads 1 |
| md_ch_ben | 1 Thread | 2/2 | 34,3 -> 19,2 | 33,2 -> 18,6 | 0,56 / 0,56 | 20 -> 25 | gleich -threads 1 |
| md_ch_ben | Regel, 32 | 2/2 | 2,3 -> 1,6 | 53,9 -> 33,2 | 0,67 / 0,62 | 45 -> 213 | - |
| md_ch_ben | exakt, 32 | 1/1 | 3,6 -> 2,5 | 54,2 -> 40,1 | 0,68 / 0,74 | 111 -> 786 | gleich -threads 1 |
| t3_byss | Regel, 32 | 3/3 | 15,8 -> 10,1 | 400,9 -> 257,3 | 0,64 / 0,64 | 156 -> 323 | - |
| t3_byss | exakt, 32 | 2/2 | 27,1 -> 17,9 | 300,1 -> 222,9 | 0,66 / 0,74 | 236 -> 908 | gleich -threads 1 |
| duel_mos_espa | Regel, 32 | 2/2 | 100,0 -> 48,1 | 1413,8 -> 977,4 | 0,48 / 0,69 | 202 -> 371 | - |
| duel_mos_espa | exakt, 32 | 1/1 | 213,9 -> 104,6 | 918,2 -> 595,2 | 0,49 / 0,65 | 284 -> 956 | gleich -threads 1 |

(basis = fee4525c1 vor VIS2S, neu = Endstand VIS2S; "Regel" = NRCs Regel, je Lauf andere Bytes; Zwischenstufe r1 in der Datei)

### Licht-Cache nach Geometrieaenderungen (`Y:\werkzeuge\lcb2\messung_final.txt`, 22:42-23:17, Fremdlast 0 beim Start und Ende)
**Kernaussage:** Der Teilbau spart auf einer grossen Karte mit kleiner Aenderung mehr als die Haelfte (duel_mos_espa ein Licht
dazu: 57 statt 117 s, Kiste verschoben und Licht: 84 statt 115 s), bringt bei einer verschobenen Struktur-Kiste unter offenem
Himmel aber nichts und ist dort sogar langsamer als ein voller Bau (duel_jakku 37,7 statt 32,4 s, nur 10-46 % der Lightmaps aus dem
Cache); ein voller Bau, der den Cache anlegt, kostet 12-34 % mehr als einer ohne. Jeder Teilbau ist bytegleich mit dem vollen Bau.
Gemessen mit dem Stand LCB2 (53655c8fe, 11.11.); LCFIX aenderte danach den Abdruck (Tabelle oben).

| Karte, Aenderung ([Final] Fast, -bounce 8, GPU) | voll + Cache s | voll ohne Cache s | Teilbau s | alter Stand (LCB1) voll / Teil s | aus dem Cache (Durchgang 1 / 9) |
|---|---|---|---|---|---|
| duel_mos_espa, ein Licht dazu | 130,7 | 117,1 | 57,1 | 284,2 / 187,4 | 64498 / 64466 von 64682 |
| duel_mos_espa, ein Licht dazu, Teilbau auf dem Prozessor | - | - | 55,6 | - | wie oben |
| duel_mos_espa, Kiste verschoben + Licht | 128,2 | 115,0 | 84,3 | 268,8 / - | 64488 / 64580 von 64688 |
| duel_jakku, Kiste verschoben + Licht | 38,1 | 32,4 | 37,7 | 104,1 / - | 3637 / 16786 von 36531 |
| duel_jakku, Detail-Kiste verschoben + Licht | 40,6 | 30,3 | 25,2 | - | 31526 / 29742 von 36531 |

(Summe der gemessenen Abschnitte des Licht-Schritts; die Datei enthaelt die Zerlegung je Abschnitt)

### Feste VIS-Regel (`Y:\werkzeuge\vis3\ruhe2_1211.log`, 12.11. 14:04-14:09, 30 Threads, Fremdlast 0-3 Kerne)
**Kernaussage:** Die feste Regel (`-bhedvisfixed`) braucht 8-11 % mehr Wanduhr als NRCs Regel, liefert dafuer in jedem Lauf und mit
30 wie mit 8 Threads dieselben Bytes - deshalb bleibt sie ein Schalter.

| Karte | NRCs Regel Wand s (Median) | feste Regel Wand s (Median) | fest / NRC | Speicher MB NRC / fest | Ergebnis NRC / fest |
|---|---|---|---|---|---|
| ctf3 | 0,70 | 0,77 | 1,10 | 222 / 283 | 3 verschiedene / jeder Lauf gleich |
| md_ch_ben | 1,52 | 1,65 | 1,09 | 205 / 275 | gleich / gleich |
| t3_byss | 10,07 | 11,14 | 1,11 | 333 / 407 | 3 verschiedene / gleich (auch mit 8 Threads) |
| duel_mos_espa | 49,89 | 53,96 | 1,08 | 377 / 451 | 2 verschiedene / gleich |

### Portal-Ebenen aus allen Punkten (`Y:\werkzeuge\vis4\ruhe2_1211.log`, 12.11. 14:19-14:24, Fremdlast 0-4 Kerne)
**Kernaussage:** Die Portal-Ebenen aus allen Punkten kosten 0-5 % Rechenzeit (auf duel_mos_espa 10 % weniger, 1 Lauf); mit
`-bhedvisnrcplanes` sind die Sichtbarkeitsdaten bytegleich mit dem alten Stand.

| Karte | Laeufe | CPU s alt | CPU s neu | neu / alt | nrc / alt | streng / alt | Sichtbarkeit nrc |
|---|---|---|---|---|---|---|---|
| ctf3 | 4 | 10,4 | 10,7 | 1,024 | 1,014 | 1,067 | = alt |
| md_ch_ben | 4 | 34,9 | 36,5 | 1,047 | 1,030 | 1,066 | = alt |
| mb2_manaan | 2 | 166,2 | 166,2 | 1,000 | 0,994 | 1,028 | = alt |
| t3_byss | 2 | 250,9 | 262,8 | 1,047 | 1,044 | 1,072 | = alt |
| duel_mos_espa | 1 | 998,7 | 895,1 | 0,896 | - | - | - |

## Dem Nutzer nennen
- Direct3D 12 (jetzt Vorgabe) haelt ueberall denselben Bildabstand wie Direct3D 11. In einer schweren Ansicht des Karten-Editors
  (Gras/Halme, duel_yavin) braucht es rund 0,2 ms mehr GPU-Zeit je Bild (+6,5 %, Ursache offen), und es braucht rund 0,25-0,3 GB
  mehr Arbeitsspeicher als Direct3D 11 (die letzten rund 250 MB liegen in Zuteilungen des Treibers, eigene Heaps nach rc612).
  Wer es merkt: Ansicht > Grafikschnittstelle > Direct3D 11.
- Raytracing-Schatten gibt es nur mit Direct3D 12 und einer Grafikkarte mit DirectX Raytracing 1.1; im Echtzeit-Licht kosten
  sie 3,5-4,7 % Bildabstand gegenueber den Schattenkarten (dafuer rund 2 GB weniger Speicher). Ab Werk aus.
- Feste VIS-Regel (`-bhedvisfixed`) 8-11 % langsamer als NRCs Regel, deshalb nur ein Schalter (fuer eigene Build-Eintraege, nicht
  im Build-Menue).
- VIS braucht mehr Speicher als bisher (auf grossen Karten mit 32 Threads bis rund 0,7 GB mehr).
- Licht-Cache: Nach einer verschobenen Struktur unter offenem Himmel dauert ein Teilbau so lange wie ein voller Bau oder laenger;
  ein voller Bau, der den Cache anlegt, dauert 12-34 % laenger als einer ohne. Das Ergebnis ist immer bytegleich mit dem vollen Bau.
- Model-Tab: Import von .xsi, .fbx, .obj und .glb als Modell kommt spaeter (Animationen aus FBX, glTF/GLB, BVH und COLLADA gehen
  schon); die Werkzeugleiste zweispaltig (80-120 Punkte breit wie Blender) kommt spaeter.
- Wie bisher: Bildzeit-Pruefungen gelten nur allein; die Mehrspieler-Menues von Movie Duels sind auf diesem Rechner nicht
  vorhanden (Tests uebersprungen).

## Changelog

### English
- Version rc612 with the changelog of all rc612 parts.

### Deutsch
- Version rc612 mit dem Changelog aller rc612-Teile.

### 中文
- 版本 rc612,包含所有 rc612 部分的更新日志。

### 日本語
- バージョン rc612。rc612 の全パートの変更履歴を収録。

### Español
- Versión rc612 con el registro de cambios de todas las partes de rc612.

### Čeština
- Verze rc612 se seznamem změn všech částí rc612.
