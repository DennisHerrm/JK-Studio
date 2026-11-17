# Notizen rc613: Release-Runde (Changelog, Handbuch, voller Lauf, Workflow-Gegenpruefung, Version rc613)

Auftrag `auftrag_rc613_release.md`. Direkt in `Y:\karte` auf `karte1` ab 5da1ca603 (50 Commits seit rc612 = 348f0e345, keine
WIP-Commits, alle DennisHerrm), Build `build-test` (`--parallel 2 -- /nodeReuse:false`, Leerlauf-Prioritaet), Laeufe
`laeufe/rel613*_`, Skripte `Y:\werkzeuge\rel613\`. Nachtfenster 16./17.11. bis 07:00 (shank: "du hast jetzt 8 stunden Zeit
dafuer"): Pool und ctest mit Volllast, Zeitpruefungen danach allein. Kein Spiel gestartet.

## Version und Changelog
- `kFassung` rc613. Ein rc613-Eintrag in 6 Sprachen aus den Changelog-Abschnitten der 6 seit rc612 neuen Notizen (test1411,
  cwpruef_1311, cwimport_1411, cwmodell_1511, reltempo_1511, dx12e_1611): Uebersicht oben, dann Download und Updates, Model-Tab,
  Mission laden, Direct3D 12, Clone-Wars-Werkzeuge (ab Werk aus). Interne Pruefungs-, Lauf- und Agentennamen nicht genannt.
- Zusammengefasst statt Zeile fuer Zeile: die 23 Zeilen von TEST1411 (a-e) zu acht Zeilen Model-Tab und einer Zeile Mission laden;
  CW-PRUEF, CW-IMPORT und CW-MODELL zu einem Abschnitt, der zuerst sagt, dass alles ab Werk aus ist bzw. nur wirkt, wenn man es
  aufruft (Entscheidung 14.11.: "drin, aber nicht Standard"). Die Grenze SHADER_MAX_VERTEXES von Movie Duels (4000 statt 1000) gilt
  auch ohne das Profil und steht deshalb als eigene Zeile.
- Direct3D 12: die Zahlen der Endmessung D3D12-5 (privat gegen Direct3D 11 nach der .map +294 -> +57 MB, mit Echtzeit-Licht
  +259 -> +30, Ende +239 -> +19; also rund 0,23 GB weniger) statt "about 200 MB" der Notiz; Bildabstand nicht schlechter. Die Zeile
  der Notiz ueber die Messzeilen des Leistungs-Selbsttests nicht (betrifft keine Bedienung).
- Schnellere Releases: ein Satz im Abschnitt Download und Updates.
- Neu gegenueber den Notizen: "Versionen bis rc612 aktualisieren mit dem ganzen Paket" (rc612 hat kein Manifest, siehe Dem Nutzer
  nennen).
- `tools/changelog.txt`, `AENDERUNGEN.md`, `gen_changelog.py` (42 Fassungen x 6 Sprachen), lint_fassung ok (rc613).
- Die Fassung steht sonst nirgends fest: der Starter liest `kFassung` (nur im User-Agent), `version.txt` im Zip schreibt
  `release_paket.py zip` aus `kFassung` (Paketpruefung: "version.txt: rc613"), die Versionsangaben der .rc-Dateien sind 1.0.0.0.

## Starter, Release-Text und Handbuch
- **Release-Text** (`release_paket.py notizen`, KOPF): die Download-Zeile nennt jetzt auch den festen Link
  `https://github.com/DennisHerrm/JK-Studio/releases/latest/download/JK-Studio-Starter.exe` (Auftrag: der Starter ist der
  Haupt-Download mit festem Link, das Zip der Rettungsweg). Rot: neue Selbstpruefung "die Download-Zeile nennt den festen Link"
  (1 FEHL), gruen 64 von 64 (ctest release_paket).
- **Handbuch, Seite "Installieren und aktualisieren":** das Bild h_starter_delta zeigte "Update auf rc613 - nur, was sich
  geaendert hat". Das stimmt fuer rc613 nicht: rc612 und aelter haben kein Manifest (und keine version.txt), ein Delta
  rc612 -> rc613 gibt es nicht (die Seite sagt selbst "JK Studio bis rc612 nimmt das ganze Paket"). `tools/handbuch/starter_bilder.py`
  bildet jetzt fuer das Delta-Bild ein Release rc614 mit Delta von rc613 nach (installiert rc613); h_starter_laden bleibt der erste
  Start mit rc613. 12 Bilder neu (1944 breit, angesehen: en/de/zh/ja/es/cs Delta "rc614", zh Laden "rc613", kein Pfad).
- Sonst keine "Was ist neu"-Stelle mit Fassung; die Fassungsnummern im Handbuch sind Geschichte und stimmen (vor rc600 Map, seit
  rc601, bis rc605 BehavEd-Remake, bis rc611 in der Zeitleiste, seit rc612 Dopesheet/Kurveneditor, Beispiel rc611 -> rc612, "bis
  rc612 das ganze Paket"), in allen 6 Sprachen gleich.
- `gen_handbuch.py`: ausser den Bildern kein Unterschied (187 Seiten x 6 Sprachen, `suche.js` aktuell); `--pruefen`:
  handbuch_sprachen ok, es/cs 0 Texte und 0 Bilder noch englisch.

## Testrunde nachgezogen (`tools/testrunde.py`)
- Die Selbsttest-Gruppen von TEST1411 (`BHED_MODELL_NUR=test1411`) und CW-MODELL (`BHED_MODELL_NUR=cwmodell`) standen in keinem Lauf
  der Runde - die volle Runde haette die Testrueckmeldungen und die Detailstufen, Bolts und Skelette still weggelassen. Neu
  `mb_cwmodell` und TEST1411 in drei Teilen mit frischem Programm (`BHED_T1411`: `mb_test1411` d..e, `mb_test1411b` a,z,
  `mb_test1411c` x,s,w,b,fotos) - wie die Abschnitte gebaut und geprueft wurden; in EINEM Programm (alle Abschnitte) waren nach a/z
  der Mauspunkt der Statuszeile leer (x) und die Pruefungen des leeren Model-Tabs rot (B), in den Teilen gruen bis auf den
  Programmfehler B unten. Auch unter Direct3D 11. Jetzt 195 Laeufe.
- `handbuch` laeuft mit `BHED_HB_TESTRUNDE=1` (siehe "handbuch" unten).
- Abgleich der Gruppennamen im Code (`n != "..."` in `gui/selbsttest_*.inc`) mit `BHED_MODELL_NUR` der Runde: sonst nur Bild-,
  Handbuch-, Zeit- und Spiellaeufe (nicht im Pool, wie in der Tabelle vermerkt) und Teilmengen (ap14h, kopfab, navmax/navblender,
  mbvmesh/mbvumform, mindest).

## Voller Lauf
Englisch (Sprachlaeufe es/cs/ja entfallen, Entscheidung 11.11.), Direct3D 12 (Vorgabe), Pool 8, normale Prioritaet (Nachtfenster),
eigene Laufordner.

| Teil | Stand | Ergebnis |
|---|---|---|
| Neubau `--clean-first` (n1) | 5da1ca603 | exit 0, 1145 s, 0 Fehler, 0 Warnungen (mit JK-Studio-Starter.exe; bhed_q3map2 eigener Ninja-Bau, aktuell) |
| Bau n2 / n3 / n4 / n5 inkrementell | Fassung / Testfixes / Programmfixes / Testfix updateleiste | je exit 0, 0 Fehler, 0 Warnungen (323 / 216 / 18 / 215 s) |
| ctest c1 `-j32` (Waechter: Stapel bei Haenger) | n2 | 285/285, 177 s, kein Haenger |
| ctest c2 `-j32` | Endstand (n5) | 285/285, 181 s, kein Haenger (auch modell_uv nicht) |
| Lints | Endstand | wie rc612: portability 97, pruefcodes 11, fremdprogramm 1, efxflags rc 1 (wie immer), Rest ok; link/tidy nur Linux |
| Runde 1 (`rel613_`, 193 Laeufe, D3D12) | n2 | 180 gruen, 13 rot, 2904 s Wanduhr, 22558 s Summe; darin 1 (Modus 1) 2639/0, allekarten 288/0, eigenstaendig 9/0, kartenpruefung 4x 2/0, grafik 22/0, mb_cwmodell 32/0 |
| Runde 2 (`rel613b_`, die 12 roten nach den Testfixes) | n3 | 10 gruen; rot mb_ap4, mb_test1411c (Programmfehler, unten) |
| Runde 3 (`rel613d11_`, Zeichengruppen D3D11, 47 Laeufe) | n3 | 45 gruen; rot mb_ap4, mb_test1411c (dieselben) |
| Runde 4 (`rel613c_` D3D12 / `rel613cd11_` D3D11) | n4 | mb_ap4 106/0 bzw. 105/0, mb_test1411c 19/0 bzw. 18/0, mb_ap13k 66/0, mb_test1411 73/0 |
| allein (Zeitpruefungen) | n4 | ke_bau 622/0, teil11 5/0, zeitleiste1 102/0 |
| updateleiste allein | n4 / n5 | 3x rot (74/1) / 3x gruen (75/0), updateleiste2 3x 5/0 |

Jede FEHLER-Zeile geklaert:
- **ke_karte** "Menue View: 292 x 635 wie NRC" (hier 659) und "Menue Modify: 258 x 230" (hier 261) - **Testfehler**: CW-IMPORT haengt
  das Untermenue "Test figure" an View (+24) und Trenner + "Clone Wars tools" an Modify (+7 +24) - bewusst, die Werkzeuge wirken nur
  beim Aufruf; die Tabelle der NRC-Masse nannte die Abweichung nicht (wie bei Build, Grid, Misc jetzt mit Kommentar). Gruen 343/0.
- **mb_funktionen** "Klick ##mzmodus nicht gefunden" und **mb_ap3** R4 "Bedienmodus erreichbar: Statuszeile" - **Testfehler**: seit
  TEST1411 G steht die Zeile mit dem Bedienmodus im Modus 3ds Max wie in 3ds Max nur auf Wunsch unter der Statuszeile (Einstellung
  modellTastenzeile; dazu das [+]-Menue der Ansicht). Die Tests schalten sie fuer den Klick an wie ap1 R8. Gruen 63/0, 106/0.
- **mb_ap3** UIFORM "Felder Ziel und Karte fehlen" - **Testfehler**: seit TEST1411 N stehen GLA, animation.cfg, Ziel-GLA und Plaetze
  im Reiter Dienstprogramme; der Test waehlt ihn (wie selbsttest_mb2.inc).
- **modell** "Klick Force loop nicht gefunden" (dazu Abspielen/Stop) - **Testfehler**: seit TEST1411 G steht Force loop im Modus
  3ds Max in der Zeitkonfiguration; der Test klickt es dort. Gruen 16/0.
- **mb_kopf** "MBKOPF B: alle vier Buchstaben ... auch der vor dem ViewCube" (N 0 Punkte) - **Testfehler**: TEST1411 L verdeckt den
  Buchstaben hinter dem Wuerfel bewusst (wie 3ds Max). Erwartet jetzt O/S/W lesbar, N verdeckt. Gruen 117/0.
- **mb_mb3** "keine Befunde fuer Movie Duels SP" (7), "nach Strg+Z kein Befund", "Ziel 1 neu geladen" und danach "Klick Yes nicht
  gefunden" beim Beenden - **Testfehler** seit CW-MODELL (Entscheidung 4: die Namenspruefung gilt ohne Profil): die
  `_humanoid/animation.cfg` von Movie Duels (MD_Assets2.pk3) hat 7 Zeilen, deren Namen die animTable von Update 9 nicht kennt
  (BOTH_INV_USE, BOTH_PARRY_WALK_DUAL/_STAFF, BOTH_RUN3_MP, BOTH_SPRINT_MP, BOTH_SPRINT_SABER/_MP - in der MD9-Quelle nur in
  Kommentaren; das Spiel uebergeht sie). Das Bauen des ganzen Satzes fragt deshalb jetzt im Programm. Der Test erwartet genau diese
  Namen (unabhaengig gegen `animNamenSpiel` gezaehlt, sonst keinen Befund), prueft den Befund "hinter dem Ende" gezielt und
  beantwortet die Frage mit Trotzdem, nachdem er ihre Zeilen geprueft hat. Gruen 24/0.
- **menueeditor** "M7 Punkt 14: ... 0 Platzhalter" (ownerdraw 351) - **Umgebung**: die Spieldaten `MD\Update9.pk3` (auf dieser
  Platte seit 15.11., Menues vom 09.11.) nutzen in ui/MD_MP/ingame_charforce/playerforce.menu eine zwoelfte Kraft (351); die
  MD9-Quelle hier (Stand 03.11.) kennt in `codemp/ui/menudef.h` nur 340-350, die schon als "ohne Quelle" zaehlen. 351 zaehlt jetzt
  mit. Gruen 2788/0. Hinweis fuer spaeter: die MD9-Quelle hat inzwischen MP-UI-Code fuer 340-350 (ui_main.c) - Nachbau moeglich.
- **handbuch** "h_mission_neu / h_mission_speichern zeigt keinen Pfad des Laufs" - **Umgebung der Runde**: die Pruefung (TEST1411e)
  gilt dem Bildlauf, der Missionen und Zielarchiv auf ein Laufwerk X: legt (`skripte/ab/bilder.sh`); die Runde hat kein X:, ihre
  Bilder gehen nicht ins Handbuch. Sie setzt `BHED_HB_TESTRUNDE=1`, die Zeile steht als uebersprungen da (Pfad im Protokoll); der
  Bildlauf prueft wie bisher. 8/0, 2 uebersprungen.
- **mb_test1411c** B "im Befehlsfeld dieselben belegten Reiter ... wie mit Modell" (Animation: Reiter 61/60, Gruppen 7/6) -
  **Programmfehler** aus CW-MODELL gegen TEST1411 B: die Gruppe "eigenes Skelett" steht mit einer .glm im Bereich Animation, die leere
  Szene (`leereGruppenListe`) kannte sie nicht - ohne Modell ein belegter Reiter weniger. Jetzt in der Liste (`app_modelltab.cpp`).
  Rot in Runde 2 und 3, gruen 19/0 (D3D12) und 18/0 (D3D11).
- **mb_ap4** P8 "ein Klick auf ein Gelenk waehlt einen Knochen, dessen Gelenk genau dort liegt" (geklickt lhumerus, gewaehlt
  lclavical) - **Programmfehler** aus TEST1411 H: die Form eines Knochens endet am Folgegelenk, bei lclavical ueber das Geschwister
  lhumerus; Select Bone (wie Blender) nahm zuerst die Form, also den vorigen Knochen der Kette. Jetzt waehlt ein Klick in der
  Treffergrenze der Formen (max(5, 0,3 Zeichenhoehen)) auf ein Gelenk dessen Knochen, sonst wie bisher Form und naechstes Gelenk
  (`knochenUnter`, `app_modelltab_blender.cpp`). Rot in Runde 1-3 (deterministisch), gruen 106/0 und 105/0; mb_ap13k (Formen) 66/0.
- **ke_bau** "Grosse Karte ... kein Bild ueber 250 ms" (279 ms, davon 150 ms Texturen hochladen), **teil11** "Zeichner des Editors -
  Boden 0,5-1,1 x Spielbild" (52 grau, Bild 1 s Wanduhr nach dem Umschalten gelesen), **zeitleiste1** "Filmdurchgang ohne laengere
  Bilder" (Mittel 26,9 gegen 12,5 ms; das Protokoll sagt selbst "unter Last") - **Zeitpruefungen unter Volllast** (Pool 8 und daneben
  das Zip der Workflow-Gegenpruefung): allein gruen (622/0, 5/0, 102/0). Tests unveraendert.
- **updateleiste** (bekannter Kandidat) "REST9 Map Editor wieder mit Leiste: 2 unruhige Bilder" - in Runde 1 unter Last gruen (das
  Spiel des Karten-Editors kam erst nach den Ruhepruefungen), allein 3 von 3 rot: der Karten-Editor laedt beim ersten Zeigen Shader
  und Entity-Klassen von Movie Duels im Hintergrund (0,5 s), "254 entity classes" kam genau im Ruhefenster - **Testfehler
  (Wettlauf)**. Der Test wartet vor den Ruhepruefungen im Map Editor auf das Spiel (`spielBereit`); allein 3 von 3 gruen (75/0).
- Uebersprungen statt FEHLER wie bisher: die Mehrspieler-Menues von Movie Duels (Daten bewusst geloescht, 05.11.).

## Workflow-Gegenpruefung (release.yml lokal)
`Y:\werkzeuge\rel613\workflow_lokal.py` stellt die Schritte von release.yml gegen den lokalen Bau nach (Python statt PowerShell,
Programme per subprocess ohne Fenster), dist in `Y:\werkzeuge\rel613\dist`: **0 FEHLER**.
- Version: `release_paket.py fassung` rc613, `--marke v1.0.0-rc613` ok, Gegenprobe `--marke v1.0.0-rc612` abgewiesen; lint_fassung,
  lint_sources ok.
- Paket: `zip` (6060 Eintraege), `pruefe --fassung rc613 --q3map2 --starter` ok (version.txt rc613, Handbuch 6 Sprachen,
  jkstudio.exe/bhed_q3map2.exe/Starter nur Windows-DLLs, Starter 636 KB, 28 Lizenztexte), `manifest` (6060 Dateien),
  check_exe_resources ok (7 Symbolgroessen, Manifest, asInvoker), `bhed.exe info --data data/base` ok.
- Eigenstaendige exe `--pruefe-daten` in leerem Ordner mit eigenem Einstellungsordner: exit 0, kein data neben der exe, eingebaute
  Daten ausgepackt (Befehlsmodell 30 Befehle, 0 Meldungen).
- Deltas: im Runner gibt es fuer rc613 keins (rc612, rc611, rc610 ohne Manifest -> "no delta", Schritt endet mit exit 0). Die
  Maschine selbst wie fuer rc614 geprobt: Manifest des rc612-Pakets mit `--fassung rc612`, `delta` (1834 Inhalte, 140,7 MB = 29 % des
  Pakets, behalten), `delta-pruefen` ok.
- Pruefsummen: `pruefsummen` ueber alle Dateien von dist (Starter 651 776, Manifest 646 971, Zip 482 457 782 Bytes), genau ein Zip,
  Starter da; jede Summe nachgerechnet und `sha256sum -c` ok.
- Notizen: `notizen rc613` - Starter zuerst mit festem Link, Zip als Weg ohne Starter, "What's new in rc613" aus dem [en]-Block.
- Nicht nachstellbar: gh release list/download, Signatur (lokal kein Geheimnis; `kSignaturSchluessel` ist leer, der Schritt signiert
  nur mit gesetztem RELEASE_SIGNING_KEY und warnt dann), Hochladen, Entwurf, ccache im Runner.

## Messungen der Pakete (Kurzfassung)

### REL-TEMPO: Bau und ctest (`NOTIZEN_reltempo_1511.md`)
Richtwerte unter Last (je `--parallel 2`, Leerlauf; eine Agentensitzung, daneben u. a. MovieDuels-SP und die Windows-Suche). Die
geplante Endmessung im Ruhefenster (`Y:\werkzeuge\reltempo\messung_final.sh`, ~2 h 15 min) ist NICHT gelaufen - es gibt keine
`messung_final.txt`; die Zahlen unten sind die unter Last.

| Bau (frischer Ordner, alle Ziele) | ohne Cache (/Zi, wie der Runner bis rc612) | mit Cache, alle Behebungen (Lauf 3, 16.11.) |
|---|---|---|
| kalt (leerer Cache) | 2171 s | 2998 s (Lauf 1/2: 1888 / 2189 s - kalt schwankt stark mit der Last) |
| warm (voller Cache, z. B. Release nach CI) | - | 179 s (-92 %) |
| eine .cpp geaendert | - | 172 s |
| ueber den Cache | - | 1337 von 1337 Aufrufen, Cache 0,62 GB |

- Ein Fehlgriff kostet gut ein Fuenftel mehr als ohne Cache (A/B assimp im selben Fenster: aus 185 s, kalt 226 s, warm 18 s).
- Gleiches Ergebnis: jkstudio.exe /Zi gegen /Z7+ccache gleiche Groesse und Abschnitte, 74 Bytes anders (alle Debug-Eintraege und
  Zeitstempel); bhed.exe, updatetest.exe, JK-Studio-Starter.exe 0 Bytes; bhed_q3map2.exe mit gleichem SOURCE_DATE_EPOCH 0 Bytes.
- Diese Runde (ohne Cache, `--parallel 2`, Leerlauf, Nachtfenster): Neubau `--clean-first` 1145 s.

| ctest (lokal) | Zeit | Ergebnis |
|---|---|---|
| seriell (wie der Runner bis rc612), 67ffd68a7 | 555 s | 276/279 (3 inhaltlich rot, in REL-TEMPO behoben) |
| -j 32, 67ffd68a7 | 179 s | dieselben 3 rot |
| -j 32 --schedule-random, 67ffd68a7 | 126 s | dieselben 3 rot |
| nachher (REL-TEMPO, -j 32) | 291 s | 285/285 |
| Hauptchat nach D3D12-5 (5da1ca603, -j 32) | 160 s | 285/285, kein Haenger |
| diese Runde c1 / c2 (-j 32) | 177 / 181 s | 285/285, kein Haenger |

- Runner: echte Zeiten (ccache mit dem MSBuild des Runner-Abbilds, Cache sichern/wiederherstellen, ctest -j4) zeigt erst der
  erste Lauf auf main und der Release-Lauf von rc613.
- Der seltene Haenger von modell_uv (16.11. einmal 1494 s) kam in diesen zwei Laeufen nicht (Waechter: Stapel nach 90 s).

### D3D12-5: Endmessung (`NOTIZEN_dx12e_1611.md` Abschnitt 4, 16./17.11. nachts, allein, Fremdlast 0,7-1,8 Kerne, je 3 Runden abwechselnd)
- **Speicher privat, Direct3D 12 gegen Direct3D 11:** nach der .map +294 -> +57 MB, mit Echtzeit-Licht +259 -> +30, Ende (Model-Tab,
  Menue) +239 -> +19, nach duel_mustafar +154 -> +33, nach der Szene +166 -> +96. Direct3D 12 selbst nach der .map 7727 -> 7482 MB.
  Grafikspeicher nach der .map +325 -> +271, mit Echtzeit-Licht +168 -> +122 MB.
- **Bildabstand:** Gegenprobe in einer Sitzung, vorher/nachher abwechselnd je dreimal (nur Direct3D 12): Karten-Editor/Model/Menue
  4,201 -> 4,188 ms, Missionen 4,496 -> 4,459 ms - nicht schlechter. Direct3D 12 gegen 11 nachher je Fenster -2..+1 % (Karten-Editor,
  Model, Menue); in der Mission duel_mustafar ruhend 4,17 / 4,41 ms (+6 %) - so schon vorher (4,17 / 4,44), keine Folge von D3D12-5.
- **GPU-Zeit:** Karten-Editor, Model, Menue Direct3D 12 gegen 11 vorher -14..+5 %, nachher -5..+6 %; Echtzeit-Licht nachher -1 / +1 %.
  Die Messfenster der Missionen streuen zwischen Sitzungen unter beiden Wegen stark (md_toaj Flug D3D11 1,23-1,75 ms) - dort kein
  Schluss aus einer Sitzung.
- **Halme (offen):** duel_yavin ohne Halme D3D12 +9 / +11 % GPU (2,772 / 3,014 vorher, 2,723 / 3,032 nachher), mit Halmen -2 / -1 %
  (3,308 / 3,246 und 3,290 / 3,273). In D3D12-4 war es umgekehrt (ohne +0 %, mit +7 %). Allein (nur dieser Abschnitt) ist Direct3D 12
  dort schneller (2,79 gegen 3,13 ms mit Halmen); im ganzen Lauf streut das Himmelsportal unter beiden Wegen 1,06-2,42 ms bei
  gleichen Befehlen. Vermutung Optimus-Kopie; Beleg nur per Ereignisaufzeichnung mit Adminrechten (nicht freigegeben).

## Dem Nutzer nennen
- Erstes Release mit Starter: rc612 und aelter haben kein Manifest - ihr Update auf rc613 laedt einmal das ganze Paket (rund 480 MB).
  Deltas gibt es erst ab dem naechsten Release.
- Windows SmartScreen kann beim ersten Start von JK-Studio-Starter.exe warnen (nicht codesigniert): Weitere Informationen ->
  Trotzdem ausfuehren.
- Direct3D 12: duel_yavin auf der Grafikkarte teils etwas langsamer als Direct3D 11, je nach Ansicht; Bildrate gleich. Direct3D 12
  braucht nur noch rund 20-60 MB mehr Arbeitsspeicher als Direct3D 11 (rc612: 0,25-0,3 GB). Wer es merkt: Ansicht >
  Grafikschnittstelle > Direct3D 11.
- Movie Duels: die `_humanoid/animation.cfg` hat 7 Zeilen, deren Namen das Spiel nicht kennt (es uebergeht sie); der Model-Tab nennt
  sie und fragt, bevor er den ganzen Satz baut.
- Clone-Wars-Werkzeuge ab Werk aus (Pruefprofil und Testfigur von Hand an); mit den Vorgaben alles wie vorher.
- Model-Tab: Import von .xsi, .fbx, .obj und .glb als Modell kommt spaeter (Animationen aus FBX, glTF/GLB, BVH und COLLADA gehen
  schon); die Werkzeugleiste zweispaltig (wie Blender) kommt spaeter.
- Wie bisher: Bildzeit-Pruefungen gelten nur allein; die Mehrspieler-Menues von Movie Duels sind auf diesem Rechner nicht
  vorhanden (Tests uebersprungen).

## Changelog

### English
- Version rc613 with the changelog of all rc613 parts.

### Deutsch
- Version rc613 mit dem Changelog aller rc613-Teile.

### 中文
- 版本 rc613,包含所有 rc613 部分的更新日志。

### 日本語
- バージョン rc613。rc613 の全パートの変更履歴を収録。

### Español
- Versión rc613 con el registro de cambios de todas las partes de rc613.

### Čeština
- Verze rc613 se seznamem změn všech částí rc613.
