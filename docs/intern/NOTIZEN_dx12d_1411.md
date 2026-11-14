# NOTIZEN D3D12-4 (14.11.2026): Direct3D 12 gegen Direct3D 11 - Messung geprueft, Ursachen, Endmessung

Zweig `p_dx12d` (Worktree `Y:\p_dx12`, auf karte1 f81f9f807), Werkzeuge `Y:\werkzeuge\dx12d` (Kopien der Messskripte von
`Y:\werkzeuge\dx12` mit eigenen Laufordnern, dazu `lauf.sh` (ein leistung-Lauf mit Taktprotokoll von nvidia-smi), `tab.py`
(Laeufe nebeneinander, Takt je Messfenster), `vgltab.py`, `tests.sh`, Patchskripte p01-p07, `invert.py`). Allein am Rechner
(Nacht), alle Messungen mit normaler Prioritaet, Fremdlast 0,5-0,7 Kerne. Das Ruhefenster vom 13.11. 21:16 hatte 1,9-3,5 Kerne
Fremdlast; seine CPU-Werte liegen deshalb unter beiden Wegen hoeher als heute Nacht.

## Ergebnis kurz

- **Die GPU-Rueckstaende des Ruhefensters waren zum groessten Teil ein Messfehler des D3D12-Wegs** (Leerlauf der Grafikkarte
  zwischen den Teilabgaben mitgezaehlt, verstaerkt durch eine Liste, die nur die Zeitmarke der Messung trug). Mit der
  berichtigten Messung und den Abhilfen unten liegt Direct3D 12 bei der GPU-Zeit in 16 von 18 Ansichten gleich oder darunter
  (Grenze +-3 %, in leichten Ansichten unter 0,5 ms auch innerhalb der Spanne der D3D11-Runden). Knapp drueber: Menue Maus bewegt
  +3,8 % (0,390 / 0,405 ms, leichte Ansicht bei 930 MHz, D3D11-Spanne 0,387..0,401). Klar drueber: Karten-Editor duel_yavin mit
  Halmen +6,5 % (3,072 / 3,273 ms).
- **Bildabstand**: in allen 18 Ansichten innerhalb +-3 % (hoechstens +2,4 %, Menue Maus).
- **Hauptfaden-CPU**: die Rechenzeit des Hauptfadens je Bild (Profil der Endmessung, Zyklen) ist unter Direct3D 12 in 13 von 18
  Ansichten niedriger und hoechstens +5,7 % hoeher (Vorlage dreht 2,29 / 2,42 ms; unter Direct3D 11 steckt darin noch Present). Die Spalte "CPU" von leistung (Wanduhr von newFrame bis vor Present)
  liegt in leichten Ansichten und im Echtzeit-Licht 5-26 % hoeher: dort wartet der Hauptfaden im Messfenster auf das vorige
  Present (bis 0,16 ms), das unter Direct3D 11 ausserhalb des Messfensters im Hauptfaden selbst laeuft. Nicht behoben (Abschnitt 3).
- **Bild**: bitgleich gegen den D3D12-Stand vor dieser Runde und gegen Direct3D 11 wie bisher belegt (Abschnitt 6).
- **Tests**: grafik, R1, verlustreihe unter beiden Wegen, DXR, leistung unter beiden Wegen gruen (Abschnitt 5).

=> **Grenze erreicht** (GPU yavin mit Halmen, CPU-Spalte in leichten Ansichten); die Commits sind fertig und einzeln ff-faehig,
Rest in `Y:\werkzeuge\dx12d\UEBERGABE.md`.

## 1. Die Messung: was die GPU-Spalte unter Direct3D 12 mass

| Pruefpunkt | Befund |
|---|---|
| Kalibrierung | beide Wege 1 000 000 000 Hz (neue Zeile `Bildmessung: Zeitmarken mit ... Hz (Direct3D 11/12)`) |
| Bereich, Present | beide: Anfangsmarke in newFrame, Endmarke nach der Oberflaeche des Hauptfensters, vor Present |
| Abfragen | unter Direct3D 12 dieselben D3D11-Abfragen am Abgeber: EndQuery in der offenen Liste, ResolveQueryData beim Abschicken, GetData nach dem Zaunwert |
| **Teilabgaben** | Der D3D11-Treiber legt die Marken eines Bildes in einen Befehlsstrom. Direct3D 12 schickt ein Bild in Teilen ab (Teilabgabe nach 1 ms an einem Zielwechsel, D3D12-2). Zwischen den Teilen wartet die Grafikkarte auf den Prozessor - die Strecke zwischen zwei Marken zaehlte diesen Leerlauf mit |
| **Marke oeffnet die Liste** | Die Anfangsmarke des Bildes oeffnete die Befehlsliste schon in newFrame; die Uhr der Teilabgabe lief ab dort, und der erste Zielwechsel nach 1 ms Oberflaechen-Code schickte eine Liste ab, die nichts als die Marke trug. Ab da lief die GPU-Uhr ueber die ganze Aufzeichnung des Bildes: die GPU-Spalte war fast die CPU-Zeit (Ruhefenster Flug: CPU 3,70 / GPU 3,63 ms, Map Editor Vorlage CPU 3,30 / GPU 3,00) - und die Messung aenderte die Abgaben, die sie mass |
| Gegenprobe Windows | Rechenzeit der Grafikkarte fuer den Prozess, wie Windows sie zaehlt (D3DKMTQueryStatistics je Knoten, neue Zeile `Leistung: GPU-System`). Direct3D 11: Zeitmarken = Windows - 0,01 bis 0,04 ms (duel1 0,158 / 0,171, Vorlage 2,519 / 2,520, Echtzeit-Licht 22,057 / 22,074 ms); Ausnahme Szene spielt (Zeitmarken 2,06, Windows 1,54 ms: dort zaehlen die D3D11-Zeitmarken selbst Leerlauf zwischen den Teilen des Treibers mit). Direct3D 12: Windows meldet den 3D-Knoten mit Hardware-Planung nur in Bloecken (in leichten Ansichten 0, einmal 44 ms je Bild) - brauchbar nur, wo die Karte ausgelastet ist: Echtzeit-Licht Zeitmarken 21,98 / Windows 22,09 ms, 11,80 / 11,80 ms |
| Unter Last (D3D12-3) | D3D12-3 mass mit `BHED_D3D12_TEILABGABE=0` (ein Teil je Bild, kein Leerlauf dazwischen): reine Rechenzeit, Direct3D 12 gleich oder schneller. Das Ruhefenster mass mit Teilabgaben und der Liste mit nur der Marke: Leerlauf mitgezaehlt. Beides war richtig gemessen, aber nicht dasselbe |
| Nachgestellt | Bau f81f9f807 heute Nacht (`b11`/`b12`, alte Messung): Flug D3D12 3,456 / D3D11 1,242 ms, Szene spielt 2,820 / 2,104 - wie im Ruhefenster. Derselbe Stand nur mit der neuen Messung (`a11`/`a12`): 1,298 / 1,255 und 1,548 / 2,059; roh (`BHED_D3D12_MARKEN=roh`, ohne Liste mit nur der Marke): 1,467 und 1,902 |
| Takt | In leichten Ansichten (nur Oberflaeche, 5-10 % ausgelastet) laeuft die Karte mit 900-2200 MHz statt 2460 (nvidia-smi im Messfenster, `tab.py WERT=Uhr`). Die GPU-Zeit dort folgt dem Taktzustand: unter Direct3D 11 springt Menue ruhend zwischen Laeufen desselben Baus zwischen 0,190 und 0,403 ms (Ruhefenster: D3D12 0,210..0,397). Kleine Unterschiede dort sind erst ueber Runden belastbar |

**Neue Messung (Commit 1):**
- Grenzmarken je Abgabe (Anfang jeder Liste, Ende der letzten, aufgeloest in der letzten Liste): `rahmen::leerlaufBis(serie)`
  summiert den Leerlauf zwischen den Abgaben, die Zeitmarken des Abgebers ziehen ihn ab ("Arbeitsuhr"). Die Strecke zwischen zwei
  Marken ist damit, was die Grafikkarte dazwischen gerechnet hat - wie bei den D3D11-Zeitmarken. `BHED_D3D12_MARKEN=roh`: wie vorher.
- Eine Liste, die nur Zeitmarken traegt, startet die Uhr der Teilabgabe nicht mehr (die Messung aenderte sonst die Abgaben).
- `Leistung: GPU-System <Ansicht>`: Rechenzeit nach Windows (3D, Copy, alle Knoten) je Bild, Abgaben je Bild, Ortszeit des
  Messfensters (Abgleich mit dem Taktprotokoll), Warten des Hauptfadens (Direct3D 12: Zaunwerte und Abschickfaden).
- `Leistung: GPU-Oberflaeche <Ansicht>`: Grafikkarte fuer die Oberflaeche des Hauptfensters (Marke vor dem Loeschen bis zur
  Endmarke), unter beiden Wegen an derselben Stelle (neuer Kopf `gui/bildmessung.h`, damit backend.h nicht waechst).
- Der Copy-Knoten arbeitet unter beiden Wegen gleich 1,15 ms je Bild (das Bild geht ueber die Intel-Grafik an den Schirm, Optimus).

Danach misst die Runde nur noch mit der Arbeitsuhr, je Ansicht 240 Bilder, Endmessung 3 Runden abwechselnd.

## 2. Ursachen und Abhilfe (je Ursache ein Commit)

Belege aus Laeufen dieser Nacht (Laufordner `Y:\werkzeuge\dx12d\laeufe\m_<name>`, Tabellen mit `tab.py`), je Ansicht 240 Bilder.

| Ursache | Beleg | Abhilfe (Commit) |
|---|---|---|
| **Hauptfenster loeschen.** Den Hintergrundpuffer der Swapchain loescht der Treiber unter Direct3D 12 mit einem vollen Schreibdurchgang | GPU-Oberflaeche Mission/Karten-Editor: D3D12 0,186-0,203 ms gegen D3D11 0,142-0,158 (`c11`/`c12`); ohne Loeschen (Versuch `c12l`) 0,129-0,142. Loeschen mit Schwarz 0,202, nach DiscardResource 0,202 ms (`d12s`, `d12v`) - kein schnelles Loeschen moeglich. Mission duel1 ruhend gesamt 0,202 -> 0,153 ms (D3D11 0,160, `f12`) | nur den Rand loeschen, den der gerundete Hintergrund des Hauptfensters frei laesst (Rundung + 2 Bildpunkte); die Bedingung wird an der Zeichenliste jedes Bildes geprueft, sonst ganz wie vorher (`hauptfensterRand`). `BHED_D3D12_LOESCHEN=immer`: ganz |
| **UpdateSubresource in Puffer**: je Aufruf zwei Barrieren (Lesezustand -> COPY_DEST, Kopie, zurueck); die Zeichenschichten schreiben je Bild Dutzende bis Hunderte kleine Stuecke in dieselben Index- und Eckpuffer (Patch-Stufen je Patch, CPU-Abschnitte der Patches, CPU-Ecken je Sicht) | Bildprotokoll (`pe_vor`/`pe_nach`, p01 gegen Endstand): Karten-Editor Vorlage, Kamera dreht 47,2 / 80,5 / 56,6 Zustandswechsel je Bild vorher, 32,7 / 33,8 / 33,0 nachher (ruhend 32,0 beide); Mission Flug 28,1 / 28,3 gegen 28,0 / 28,2 (dort kaum Uploads in Puffer). In der GPU-Zeit vom Loeschen und vom Rauschen nicht zu trennen | der Puffer bleibt fuer weitere Kopien in COPY_DEST und geht vor dem naechsten Zeichenbefehl zusammen mit allen so beschriebenen zurueck (`kopierZiele`) |
| **Barrieren vor einem Zeichenbefehl in bis zu drei Aufrufen** (Ziele, gelesene Texturen, kopierte Puffer) - jeder Aufruf ein Wartepunkt der Grafikkarte | Bildprotokoll Karten-Editor: in den Nachbearbeitungsketten (Gluehen weichzeichnen 320x240 hin und her, Auflegen, Spielgamma) je Durchgang ein Ziel- und ein Texturwechsel in getrennten Aufrufen. Wirkung in der GPU-Zeit nicht vom Rauschen zu trennen (die Zahl der Wechsel bleibt, nur die Aufrufe werden weniger) | ein ResourceBarrier-Aufruf je Zeichenbefehl, danach OMSetRenderTargets |
| **Hauptfaden wartet vor der Oberflaeche auf alle Abgaben** (abschickenAbwarten: auch die Teilabgaben dieses Bildes, je ein ExecuteCommandLists) | neue Messzeile: Warten des Hauptfadens je Bild vorher bis 0,11 ms (Echtzeit-Licht ruhend), 0,04-0,06 (Model, Menue), `i12` | nur noch auf das vorige Present warten (`zeigenAbwarten`) |

Gemessen und **nicht** die Ursache (Versuche, wieder entfernt): Bloecke der dynamischen Puffer im Hochlade-Heap oder im
GPU_UPLOAD-Heap (`c12u`, `c12g`, `c12gg`: GPU gleich), Kopie der vom Shader gelesenen Puffer aus (`e12k`: Echtzeit-Licht +0,55 ms
ohne sie, sonst gleich), Teilabgabe aus (`e12t0`: gleich), Abschickfaden mit niedrigerer Prioritaet (`j12m`: Model wartet 0,25 ms)
oder ohne Faden (`j12d`: CPU +0,1 bis +0,6 ms).

## 3. Offen (Grenze)

**duel_yavin mit Halmen, GPU +6,5 %.** Das Bildprotokoll (mit Notizen je Zeichenschritt, Versuch `diag_schritte.patch`) zeigt den
Unterschied genau: die Halme sind EIN Zeichenbefehl von 0,258 ms (Teil "Halme gemischt"); ohne Halme ist Direct3D 12 gleich
(3,011 / 3,011 ms). Unter Direct3D 11 misst derselbe Teil 0,25 ms - aber die Summe des Bildes waechst mit den Halmen nur um
0,03 ms (3,00 -> 3,03; auch nach Windows 3,028 -> 3,053), waehrend der Teil "Nebensicht" (Himmelsportal, unter Direct3D 12 in
beiden Faellen 1,52-1,57 ms) unter Direct3D 11 von 1,556 auf 1,28-1,32 ms faellt. Dieselben Zeichenbefehle, dasselbe Bild
(teil13c: k13c_yavin bitgleich D3D11/D3D12) - unter Direct3D 11 wird das Himmelsportal schneller, sobald die Halme an sind.
Takt im Messfenster unter beiden 2460 MHz (nvidia-smi, 200 ms). Nicht gefunden: Barriere, Teilabgabe, Puffer-Heap, SRV-Kopie
sind es nicht (Versuche oben). Vermutung zum Pruefen: Ueberlappung mit der Kopie des vorigen Bildes zur Intel-Grafik (Copy-Knoten
1,15 ms je Bild) oder Taktwechsel im ms-Bereich, die der D3D11-Treiber anders anstoesst.

**CPU-Spalte (Wanduhr newFrame bis vor Present) in leichten Ansichten +5 bis +26 %, md_am_sith ohne Effekte +14 %, Echtzeit-Licht
+11 bis +17 %.** Die Rechenzeit des Hauptfadens ist dort unter Direct3D 12 niedriger (Profil: Menue 0,46 / 0,66, Model 0,67 / 0,83,
Echtzeit-Licht 2,13 / 2,34 ms je Bild), die Wanduhr hoeher. Teil davon gemessen: der Hauptfaden wartet im Messfenster bis
0,16 ms auf das vorige Present (Model, Menue, toaj ruhend; Abschickfaden). Unter Direct3D 11 laeuft Present im Hauptfaden selbst,
ausserhalb des Messfensters (Profil D3D11: Present 9-19 % des Hauptfadens in diesen Ansichten). Das Warten nur vor das Messfenster
zu legen, haette die Zahl geschoent und dem Programm nichts gebracht - nicht gemacht. Abhilfe-Vorschlag: den Hintergrundpuffer
des naechsten Bildes vorhersagen (Flip-Modell, drei Puffer im Ring; nach ResizeBuffers neu fragen) und nur vor dem Abschicken
pruefen - Risiko: liegt die Vorhersage einmal falsch (Present abgelehnt, verdeckt), zeichnet ein Bild in den falschen Puffer. Der
Rest (md_am_sith ohne Effekte, Echtzeit-Licht: Wanduhr +0,2-0,5 ms bei gleicher oder kleinerer Rechenzeit, Warten gemessen
0,00-0,12 ms) ist nicht eingegrenzt; der Hauptfaden steht dort, ohne zu warten oder zu rechnen (Planer des Systems).

Speicher wie nach REST612 (nicht Ziel dieser Runde): privat nach der .map 7429 / 7719 MB.

## 4. Endmessung

`messung_final.sh` (Kopie in `Y:\werkzeuge\dx12d`, Bau `kand_exe` = Endstand afa50e8c4, gleicher Baum wie a9843f92c), allein,
normale Prioritaet, Anlauf + 3 Runden abwechselnd D3D11/D3D12, danach je ein Profil. Ausgabe `Y:\werkzeuge\dx12d\messung_final.txt`
(Kopie `messung_final_nachher.txt`). "vorher, neue Messung": derselbe Ablauf mit dem Bau, der nur die neue Messung hat (p01 auf
f81f9f807, `messung_vorher.txt`). Grenze fuer "nicht schlechter": +3 %; in leichten Ansichten (GPU unter 0,5 ms) zusaetzlich
innerhalb der Spanne der D3D11-Runden. Median der 3 Runden, D3D11 / D3D12 (Unterschied):

**GPU (ms, D3D11 / D3D12)**

| Ansicht | vorher: Ruhefenster 13.11. (alte Messung) | vorher: Stand f81f9f807, neue Messung | nachher |
|---|---|---|---|
| Mission mp/duel1 ruhend | 0.163 / 0.287 (+76 %) | 0.160 / 0.203 (+27 %) | 0.162 / 0.154 (-5 %) |
| Mission md_toaj_jedi ruhend | 0.185 / 0.202 (+9 %) | 0.184 / 0.209 (+14 %) | 0.183 / 0.184 (+1 %) |
| Mission md_toaj_jedi Flug | 1.743 / 3.633 (+108 %) | 1.740 / 1.315 (-24 %) | 1.690 / 1.284 (-24 %) |
| Mission Szene angehalten | 0.363 / 0.488 (+34 %) | 0.434 / 0.540 (+24 %) | 0.444 / 0.443 (-0 %) |
| Mission Szene spielt | 2.468 / 3.785 (+53 %) | 2.139 / 1.561 (-27 %) | 2.094 / 1.539 (-27 %) |
| Mission duel_mustafar ruhend | 0.330 / 0.354 (+7 %) | 0.385 / 0.389 (+1 %) | 0.389 / 0.382 (-2 %) |
| Map Editor Vorlage wie im Spiel ruhend | 2.531 / 2.996 (+18 %) | 2.511 / 2.535 (+1 %) | 2.507 / 2.486 (-1 %) |
| Map Editor Vorlage wie im Spiel Kamera dreht | 2.107 / 2.729 (+30 %) | 2.083 / 2.087 (+0 %) | 2.084 / 2.033 (-2 %) |
| Map Editor duel_yavin ohne Halme | 3.057 / 3.125 (+2 %) | 3.005 / 3.079 (+2 %) | 3.011 / 3.011 (+0 %) |
| Map Editor duel_yavin mit Halmen | 3.079 / 3.420 (+11 %) | 3.008 / 3.320 (+10 %) | 3.072 / 3.273 (+7 %) |
| Map Editor md_am_sith ohne Effekte | 2.612 / 2.762 (+6 %) | 2.622 / 2.683 (+2 %) | 2.621 / 2.639 (+1 %) |
| Map Editor md_am_sith mit Effekten | 2.902 / 3.421 (+18 %) | 2.814 / 2.767 (-2 %) | 2.834 / 2.706 (-5 %) |
| Map Editor Echtzeit-Licht ruhend | 22.075 / 22.213 (+1 %) | 22.066 / 21.950 (-1 %) | 22.430 / 21.954 (-2 %) |
| Map Editor Echtzeit-Licht Kamera dreht | 11.432 / 12.154 (+6 %) | 11.733 / 11.846 (+1 %) | 11.792 / 11.943 (+1 %) |
| Model stormtrooper ruhend | 0.213 / 0.392 (+84 %) | 0.404 / 0.392 (-3 %) | 0.403 / 0.391 (-3 %) |
| Model stormtrooper Animation und Drehteller | 0.226 / 0.441 (+95 %) | 0.442 / 0.483 (+9 %) | 0.449 / 0.466 (+4 %) |
| Menu Editor ui/main.menu ruhend | 0.190 / 0.397 (+109 %) | 0.415 / 0.380 (-8 %) | 0.398 / 0.394 (-1 %) |
| Menu Editor ui/main.menu Maus bewegt | 0.202 / 0.395 (+96 %) | 0.418 / 0.390 (-7 %) | 0.390 / 0.405 (+4 %) |

**CPU (ms, D3D11 / D3D12)**

| Ansicht | vorher: Ruhefenster 13.11. (alte Messung) | vorher: Stand f81f9f807, neue Messung | nachher |
|---|---|---|---|
| Mission mp/duel1 ruhend | 0.43 / 0.46 (+7 %) | 0.43 / 0.47 (+9 %) | 0.44 / 0.46 (+5 %) |
| Mission md_toaj_jedi ruhend | 0.72 / 0.76 (+6 %) | 0.57 / 0.71 (+25 %) | 0.63 / 0.66 (+5 %) |
| Mission md_toaj_jedi Flug | 3.80 / 3.70 (-3 %) | 3.20 / 3.13 (-2 %) | 3.09 / 3.09 (+0 %) |
| Mission Szene angehalten | 1.25 / 1.34 (+7 %) | 0.98 / 1.00 (+2 %) | 0.95 / 1.00 (+5 %) |
| Mission Szene spielt | 4.36 / 5.09 (+17 %) | 3.43 / 3.47 (+1 %) | 3.47 / 3.46 (-0 %) |
| Mission duel_mustafar ruhend | 1.50 / 1.51 (+1 %) | 1.08 / 1.09 (+1 %) | 1.01 / 1.09 (+8 %) |
| Map Editor Vorlage wie im Spiel ruhend | 2.84 / 3.30 (+16 %) | 2.09 / 2.32 (+11 %) | 2.19 / 2.24 (+2 %) |
| Map Editor Vorlage wie im Spiel Kamera dreht | 2.92 / 3.03 (+4 %) | 2.11 / 2.22 (+5 %) | 2.14 / 2.16 (+1 %) |
| Map Editor duel_yavin ohne Halme | 2.46 / 2.61 (+6 %) | 1.93 / 2.04 (+6 %) | 1.98 / 2.06 (+4 %) |
| Map Editor duel_yavin mit Halmen | 2.71 / 2.85 (+5 %) | 2.00 / 2.17 (+8 %) | 2.08 / 2.12 (+2 %) |
| Map Editor md_am_sith ohne Effekte | 2.20 / 1.97 (-10 %) | 1.56 / 1.66 (+6 %) | 1.52 / 1.74 (+14 %) |
| Map Editor md_am_sith mit Effekten | 3.75 / 3.42 (-9 %) | 2.47 / 2.49 (+1 %) | 2.43 / 2.41 (-1 %) |
| Map Editor Echtzeit-Licht ruhend | 2.98 / 3.09 (+4 %) | 3.24 / 3.72 (+15 %) | 3.36 / 3.72 (+11 %) |
| Map Editor Echtzeit-Licht Kamera dreht | 4.07 / 4.08 (+0 %) | 3.37 / 3.43 (+2 %) | 3.02 / 3.53 (+17 %) |
| Model stormtrooper ruhend | 0.90 / 0.83 (-8 %) | 0.61 / 0.69 (+13 %) | 0.62 / 0.73 (+18 %) |
| Model stormtrooper Animation und Drehteller | 0.99 / 0.99 (+0 %) | 0.70 / 0.73 (+4 %) | 0.72 / 0.85 (+18 %) |
| Menu Editor ui/main.menu ruhend | 0.57 / 0.60 (+5 %) | 0.45 / 0.50 (+11 %) | 0.50 / 0.62 (+24 %) |
| Menu Editor ui/main.menu Maus bewegt | 0.58 / 0.62 (+7 %) | 0.46 / 0.55 (+20 %) | 0.42 / 0.53 (+26 %) |

**Abstand (ms, D3D11 / D3D12)**

| Ansicht | vorher: Ruhefenster 13.11. (alte Messung) | vorher: Stand f81f9f807, neue Messung | nachher |
|---|---|---|---|
| Mission mp/duel1 ruhend | 4.22 / 4.17 (-1 %) | 4.18 / 4.18 (+0 %) | 4.17 / 4.18 (+0 %) |
| Mission md_toaj_jedi ruhend | 4.17 / 4.20 (+1 %) | 4.29 / 4.46 (+4 %) | 4.30 / 4.34 (+1 %) |
| Mission md_toaj_jedi Flug | 5.71 / 5.56 (-3 %) | 5.59 / 5.27 (-6 %) | 5.45 / 5.23 (-4 %) |
| Mission Szene angehalten | 4.55 / 4.56 (+0 %) | 4.38 / 4.53 (+3 %) | 4.50 / 4.60 (+2 %) |
| Mission Szene spielt | 4.65 / 5.59 (+20 %) | 4.17 / 4.22 (+1 %) | 4.18 / 4.23 (+1 %) |
| Mission duel_mustafar ruhend | 4.17 / 4.18 (+0 %) | 4.20 / 4.18 (-0 %) | 4.17 / 4.18 (+0 %) |
| Map Editor Vorlage wie im Spiel ruhend | 4.17 / 4.27 (+2 %) | 4.17 / 4.22 (+1 %) | 4.17 / 4.20 (+1 %) |
| Map Editor Vorlage wie im Spiel Kamera dreht | 4.17 / 4.18 (+0 %) | 4.18 / 4.19 (+0 %) | 4.17 / 4.18 (+0 %) |
| Map Editor duel_yavin ohne Halme | 4.18 / 4.18 (+0 %) | 4.17 / 4.24 (+2 %) | 4.17 / 4.22 (+1 %) |
| Map Editor duel_yavin mit Halmen | 4.17 / 4.17 (+0 %) | 4.18 / 4.25 (+2 %) | 4.18 / 4.20 (+0 %) |
| Map Editor md_am_sith ohne Effekte | 4.18 / 4.18 (+0 %) | 4.20 / 4.24 (+1 %) | 4.18 / 4.22 (+1 %) |
| Map Editor md_am_sith mit Effekten | 4.45 / 4.34 (-2 %) | 4.20 / 4.27 (+2 %) | 4.18 / 4.18 (+0 %) |
| Map Editor Echtzeit-Licht ruhend | 22.10 / 22.24 (+1 %) | 22.08 / 22.00 (-0 %) | 22.50 / 21.97 (-2 %) |
| Map Editor Echtzeit-Licht Kamera dreht | 11.90 / 12.14 (+2 %) | 12.06 / 12.00 (-0 %) | 12.03 / 12.03 (+0 %) |
| Model stormtrooper ruhend | 4.17 / 4.18 (+0 %) | 4.22 / 4.23 (+0 %) | 4.20 / 4.25 (+1 %) |
| Model stormtrooper Animation und Drehteller | 4.17 / 4.17 (+0 %) | 4.19 / 4.24 (+1 %) | 4.25 / 4.25 (+0 %) |
| Menu Editor ui/main.menu ruhend | 4.17 / 4.17 (+0 %) | 4.17 / 4.18 (+0 %) | 4.29 / 4.27 (-0 %) |
| Menu Editor ui/main.menu Maus bewegt | 4.17 / 4.19 (+0 %) | 4.29 / 4.17 (-3 %) | 4.17 / 4.27 (+2 %) |

**Hauptfaden-Rechenzeit je Bild (Profil der Endmessung, ms, D3D11 / D3D12):** duel1 0,59 / 0,42, toaj ruhend 0,71 / 0,55, Flug
2,45 / 2,23, Szene angehalten 1,15 / 1,00, Szene spielt 3,62 / 3,65, mustafar 1,45 / 1,13, Vorlage ruhend 2,28 / 2,36, Vorlage dreht
2,29 / 2,42, yavin ohne 2,12 / 2,18, yavin mit 2,31 / 2,25, md_am_sith ohne 1,72 / 1,75, mit 2,65 / 2,58, Echtzeit-Licht ruhend
2,34 / 2,13, dreht 3,08 / 2,90, Model 0,83 / 0,67 und 0,94 / 0,73, Menue 0,66 / 0,46 und 0,67 / 0,47. Unter Direct3D 11 enthaelt
die Zahl Present (1-19 %), unter Direct3D 12 laeuft Present auf dem Abschickfaden. Treiberfaden D3D11 0,27-1,88 ms je Bild.

**Speicher** (nicht Ziel, wie REST612): privat nach der .map 7429 / 7719 MB, Grafikspeicher 2441 / 2765 MB.

## 5. Tests (Endstand afa50e8c4 = Baum von a9843f92c, Debugschicht an, Leerlauf-Prioritaet, je eigener Laufordner, parallel)

| Test | Direct3D 12 | Direct3D 11 |
|---|---|---|
| grafik ganzer Lauf | 23 OK / 0 FEHLER | 22 / 0 |
| R1 (`BHED_GRAFIK_NUR=verlust`) | 6 / 0 | 5 / 0 |
| verlustreihe (`BHED_GRAFIK_NUR=verlustreihe`) | 7 / 0 (Objekte frueherer Geraete 0 / 0 / 0) | 6 / 0 |
| DXR (`karteneditor`, `BHED_KARTE_NUR=dxr`) | 36 / 0 | - |
| leistung ganzer Lauf mit Fotos | 6 / 0 | 5 / 0 |

Nicht gelaufen (Auftrag nennt sie nicht, keine Aenderung ausserhalb des D3D12-Wegs ausser Messzeilen): ctest, Lints, volle Runde.
Bau `--parallel 4`, /W4 /WX: 0 Fehler, 0 Warnungen.

## 6. Bildvergleich (`vgl.py`, Fotos von leistung ganz, je Weg ein Lauf)

| Vergleich | Ergebnis |
|---|---|
| D3D12 vorher (f81f9f807) gegen nachher | 10 von 11 bitgleich; `leistung_mission` 82 Bildpunkte rechts unten = die fps-Zahl (ebenso D3D11 vorher/nachher) |
| D3D12 Messbau gegen Rand-Loeschen (leistung ksemn) | 10 von 10 bitgleich (Mission, Model und Menue sind ganze Programmfenster mit den gerundeten Ecken) |
| D3D11 vorher gegen nachher | 8 gleich; `leistung_editor` 8, `leistung_editor_echtlicht` 22 Bildpunkte um 1/255 (dieselben Bilder schwanken schon zwischen zwei Laeufen desselben Baus, D3D12-3/REST612), Mission fps-Zahl |
| D3D11 gegen D3D12 (nachher) | wie bisher: `editor_vorlage` 1416, `editor` 1076, `editor_echtlicht` 1065 Bildpunkte an Kanten der Mehrfachabtastung (Ursache in NOTIZEN_dx12c Punkt 5), `mission` 7, `modell` 2, `modell_rig` 3 Bildpunkte um 1/255; Rest gleich |
| teil13c D3D11 gegen D3D12 (Zwischenstand vor Commit 3-5, zur Frage der Halme) | 7 von 7 bitgleich (k13c_yavin mit Halmen, amsith, mustafar, deathstar, ih); teil13c 8/0 und 7/0 |

## Rot / Gruen

| Punkt | Rot | Gruen |
|---|---|---|
| Messung | Bau f81f9f807: Flug D3D12 3,456 / D3D11 1,242 ms, Szene spielt 2,820 / 2,104 (`b12`/`b11`), Abgaben je Bild bis 2,15 mit einer Liste nur fuer die Marke | Messbau p01: 1,298 / 1,255, 1,548 / 2,059 (`a12`/`a11`); D3D11-Zeitmarken gegen Windows 0,01-0,04 ms |
| Hauptfenster loeschen | GPU-Oberflaeche 0,186-0,203 gegen D3D11 0,142-0,158 ms (`c12`/`c11`) | 0,129-0,142 ms (`g12`), Endmessung siehe Tabelle; Fotos bitgleich |
| Warten auf alle Abgaben | Warten des Hauptfadens bis 0,11 ms je Bild (`i12`) | wartet nur noch auf das vorige Present (Rest bis 0,16 ms in leichten Ansichten, Abschnitt 3) |
| Pufferkopien gebuendelt | Vorlage, Kamera dreht: 47-81 Zustandswechsel je Bild (`pe_vor`) | 33-34 (`pe_nach`) |

## Fragen an shank

1. CPU-Spalte in leichten Ansichten: soll der Hintergrundpuffer vorhergesagt werden (Abschnitt 3, kleines Risiko eines falschen
   Bilds, wenn Windows ein Present ablehnt), oder genuegt die niedrigere Rechenzeit des Hauptfadens (Profil)?
2. duel_yavin mit Halmen (+0,2 ms GPU): weiter suchen (Ueberlappung mit der Kopie zur Intel-Grafik) oder hinnehmen?

## Changelog

## English

- Direct3D 12: the GPU time per frame in the performance self test now counts only the time the graphics card works. Before, the idle time between the parts of a frame was counted too - and measuring itself sent an extra part - so Direct3D 12 looked up to twice as slow on the graphics card as it is. Windows' own GPU time and the GPU time of the user interface are shown next to it.
- Direct3D 12: the main window is no longer cleared completely every frame, only the border its rounded background leaves visible (the driver clears the window buffer slowly under Direct3D 12). The picture is unchanged; 0.05 ms less GPU time per frame.
- Direct3D 12: small uploads into the same buffer no longer make the graphics card wait after each piece, and all state changes before a draw go out together.
- Direct3D 12: drawing the main window waits only for the previous frame to be shown, not for all parts of the current frame.

## Deutsch

- Direct3D 12: Die GPU-Zeit je Bild im Leistungs-Selbsttest zählt jetzt nur noch, was die Grafikkarte rechnet. Vorher zählte die Wartezeit zwischen den Teilen eines Bildes mit - und die Messung selbst schickte einen Teil mehr ab -, sodass Direct3D 12 auf der Grafikkarte bis zu doppelt so langsam aussah, wie es ist. Daneben stehen die GPU-Zeit nach Windows und die der Oberfläche.
- Direct3D 12: Das Hauptfenster wird nicht mehr in jedem Bild ganz gelöscht, nur der Rand, den sein gerundeter Hintergrund frei lässt (der Treiber löscht den Fensterpuffer unter Direct3D 12 langsam). Das Bild bleibt gleich; 0,05 ms weniger GPU-Zeit je Bild.
- Direct3D 12: Kleine Uploads in denselben Puffer lassen die Grafikkarte nicht mehr nach jedem Stück warten, und alle Zustandswechsel vor einem Zeichenbefehl gehen zusammen ab.
- Direct3D 12: Das Zeichnen des Hauptfensters wartet nur noch, bis das vorige Bild gezeigt ist, nicht auf alle Teile des laufenden Bildes.

## 中文

- Direct3D 12:性能自检中每帧的 GPU 时间现在只计算显卡实际工作的时间。此前帧各部分之间的空闲时间也被计入(而且测量本身多提交了一部分),使 Direct3D 12 在显卡上看起来最多慢一倍。旁边还显示 Windows 统计的 GPU 时间和界面的 GPU 时间。
- Direct3D 12:主窗口不再每帧整体清除,只清除其圆角背景留出的边缘(Direct3D 12 下驱动清除窗口缓冲很慢)。画面不变;每帧 GPU 时间少 0.05 毫秒。
- Direct3D 12:向同一缓冲区的小块上传不再让显卡在每块之后等待,绘制前的所有状态切换一起提交。
- Direct3D 12:绘制主窗口时只等待上一帧显示完成,而不再等待当前帧的所有部分。

## 日本語

- Direct3D 12:パフォーマンス自己テストのフレームごとの GPU 時間は、グラフィックカードが実際に処理した時間だけを数えるようになりました。以前はフレームの各部分の間の待ち時間も含まれ(測定自体が余分な部分を送っていたため)、Direct3D 12 は GPU 上で実際の最大 2 倍遅く見えていました。Windows による GPU 時間と UI の GPU 時間も並べて表示します。
- Direct3D 12:メインウィンドウを毎フレーム全体クリアせず、角の丸い背景が残す縁だけをクリアします(Direct3D 12 ではドライバーがウィンドウバッファーのクリアに時間がかかるため)。画像は変わらず、フレームあたり GPU 時間が 0.05 ms 減ります。
- Direct3D 12:同じバッファーへの小さなアップロードのたびにグラフィックカードが待たなくなり、描画前の状態遷移はまとめて送られます。
- Direct3D 12:メインウィンドウの描画は、前のフレームの表示だけを待ち、現在のフレームのすべての部分は待たなくなりました。

## Español

- Direct3D 12: el tiempo de GPU por fotograma de la autoprueba de rendimiento cuenta ahora solo el tiempo en que trabaja la tarjeta gráfica. Antes contaba también la espera entre las partes de un fotograma (y la propia medición enviaba una parte de más), así que Direct3D 12 parecía hasta el doble de lento en la GPU. Al lado se muestran el tiempo de GPU según Windows y el de la interfaz.
- Direct3D 12: la ventana principal ya no se borra entera en cada fotograma, solo el borde que deja libre su fondo redondeado (el controlador borra lentamente el búfer de la ventana con Direct3D 12). La imagen no cambia; 0,05 ms menos de GPU por fotograma.
- Direct3D 12: las cargas pequeñas en el mismo búfer ya no hacen esperar a la tarjeta gráfica tras cada pieza, y todos los cambios de estado antes de un dibujo se envían juntos.
- Direct3D 12: dibujar la ventana principal solo espera a que se muestre el fotograma anterior, no a todas las partes del fotograma actual.

## Čeština

- Direct3D 12: čas GPU na snímek ve výkonnostním samotestu teď počítá jen dobu, kdy grafická karta pracuje. Dříve se započítávalo i čekání mezi částmi snímku (a samotné měření odesílalo část navíc), takže Direct3D 12 vypadalo na GPU až dvakrát pomalejší. Vedle je čas GPU podle Windows a čas rozhraní.
- Direct3D 12: hlavní okno se už nemaže celé v každém snímku, jen okraj, který nechává volný jeho zaoblené pozadí (ovladač maže vyrovnávací paměť okna v Direct3D 12 pomalu). Obraz zůstává stejný; o 0,05 ms méně času GPU na snímek.
- Direct3D 12: malá nahrávání do stejné vyrovnávací paměti už nenutí grafickou kartu čekat po každém kousku a všechny změny stavu před vykreslením se odešlou společně.
- Direct3D 12: kreslení hlavního okna čeká jen na zobrazení předchozího snímku, ne na všechny části aktuálního snímku.
