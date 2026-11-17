# NOTIZEN D3D12-5 (16.11.2026): Speicher unter Direct3D 12, Halme auf duel_yavin

Zweig `p_dx12e` (Worktree `Y:\p_dx12`, auf karte1 e4179bddf), Werkzeuge `Y:\werkzeuge\dx12e` (Kopien der Messskripte von dx12d
mit eigenen Laufordnern, dazu `probe\speicherprobe.cpp`, `speichertab.py`, `protab.py`, `portal.py`, `messen.sh`, `tests5.sh`,
`ctest_waechter.py`). Auftrag `auftraege/auftrag_d3d12_5.md`. Gemessen am Tag: shank arbeitete am Rechner (Firefox, Discord, VS
Code; Fremdlast 2-4,5 Kerne), zeitweise liefen fremde MovieDuels-Spiele (andere Sitzung) auf der NVIDIA-Karte. `messen.sh` wartet
seit 14:57 auch auf fremde Programme der Karte (nvidia-smi). Zeiten der Grafikkarte aus dem Tag sind Richtwerte; Speicherwerte
schwanken zwischen Laeufen um 10-50 MB.

## Ergebnis kurz

- **Speicher (Hauptursache gefunden und behoben):** Direct3D 12 behielt im Karten-Editor rund 200-240 MB Befehlsspeicher. Beim
  Oeffnen der .map zeichnen einzelne Bilder 355 000-431 000 Mal; alle Teile eines Bildes schrieben in EINEN Befehlsspeicher
  (ID3D12CommandAllocator) des Bildplatzes, und der Treiber behaelt den Speicher fuer das Geraet. Jetzt: Befehlsspeicher aus einem
  Vorrat je Liste, Liste nach 8192 Zeichenbefehlen abgeschickt. Dazu gehen leere Seiten der dynamischen Puffer und Vorraete an
  Windows zurueck (ein eigener Faden gibt frei). Endmessung (Median dreier Runden, Abschnitt 4), privat Direct3D 12 gegen
  Direct3D 11: nach der .map +294 -> +57 MB, mit Echtzeit-Licht +259 -> +30, Ende (Model-Tab, Menue) +239 -> +19, nach
  duel_mustafar +154 -> +33. Direct3D 12 selbst: 7727 -> 7482, 7617 -> 7388, 7731 -> 7507 MB.
- **Bildabstand und GPU-Zeit nicht schlechter** (Abschnitt 4): Gegenprobe in einer Sitzung, vorher und nachher abwechselnd je
  dreimal: Bildabstand Karten-Editor/Model/Menue 4,201 -> 4,188 ms, Missionen 4,496 -> 4,459. GPU-Zeit Direct3D 12 gegen 11 in
  Karten-Editor, Model, Menue vorher -14..+5 %, nachher -5..+6 %. Dazwischen gefunden und behoben: das Zurueckgeben der Vorraete,
  alle Faelligen in einem Bild, kostete beim Umschalten im Karten-Editor bis 40 ms (Befund 8), und die neue Messzeile
  "Speicherlage" je Ansicht ein Bild von 20-30 ms zwischen den Messfenstern (jetzt nur auf Wunsch).
- **Grafikspeicher:** gleich bzw. darunter, wo Direct3D 11 gemessen wird (Mission -405, Ende -2057 MB); nach der .map +325 -> +271,
  mit Echtzeit-Licht +168 -> +122 MB. Rest belegt: Hintergrundpuffer unter Optimus doppelt (+45), ruhende Kopien (+53, GPU-Zeit), Auslagern
  erst nach 4 s (siehe Abschnitt 1, Punkt "Was bleibt").
- **Halme:** keine Abhilfe gefunden, bleibt offen (Known limits rc613; Ereignisaufzeichnung mit Adminrechten nicht freigegeben). Ohne Missionen davor (nur Abschnitt e) ist Direct3D 12 auf duel_yavin SCHNELLER als
  Direct3D 11 (mit Halmen 2,79 gegen 3,13 ms, Direct3D 12 / 11). Im ganzen Lauf streut das Himmelsportal unter BEIDEN Wegen um bis zu das Doppelte -
  dieselben Befehle in derselben Liste brauchen je Bild 1,06 bis 2,42 ms (Bildprotokoll). In den Endmessungen dieser Nacht liegt
  der Nachteil OHNE Halme (+9..+11 %, Direct3D 11 / 12: 2,723-2,772 / 3,004-3,032 ms), mit Halmen ist Direct3D 12 schneller
  (3,290-3,308 / 3,246-3,273) - in D3D12-4 war es umgekehrt. Belege und Versuche in Abschnitt 2.

## 1. Speicher

### Werkzeuge

| Werkzeug | was |
|---|---|
| Speicherprobe (`probe\speicherprobe.cpp`, ohne Fenster) | je Zuteilungsart privat / Arbeitssatz / lokal / nicht lokal unter D3D11 und D3D12 |
| `Leistung: Speicherlage <Punkt oder Ansicht>` (nur mit `BHED_LEISTUNG_SPEICHERLAGE=1`) | Windows: lokal, nicht lokal, Apertur, nicht im bevorzugten Segment, herabgestuft, privat sichtbar |
| `D3D12-Speicher Zuteilung` | kleine Texturen (dicht mit 4 KB?), nie gebrauchte Texturen (davon im Grafikspeicher), Puffer: Zuteilung gegen Inhalt, ruhende Kopien |
| `D3D12-Speicher Rahmen` | Bilder der Oberflaeche, Hintergrundpuffer, Eckpuffer der Oberflaeche, Befehlsspeicher (angelegt / frei / unterwegs) |
| `D3D12-Start Speicher` | privat / lokal / nicht lokal nach jedem Schritt des Starts |
| Speicherspur (`BHED_D3D12_SPEICHERSPUR=1`) | je Bild, wenn "nicht lokal" sich um 2 MB aendert: Bloecke, Hochladepuffer, Zeichenbefehle, Abgaben |

### Befunde mit Belegen

1. **Befehlsspeicher (Hauptursache).** Speicherspur v2s12: was ausserhalb von Bloecken und Hochladepuffern "nicht lokal" liegt,
   waechst beim Oeffnen der .map in den Bildern 6554-6556 (431 000 / 355 000 / 31 000 Zeichenbefehle) um 106 MB und bleibt bis zum
   Ende. Speicherprobe: 400 000 Kopien in einer Liste +50 MB privat, +36 MB nicht lokal - nach der Freigabe des Allokators kommt
   nichts zurueck, ein zweiter Allokator nimmt denselben Speicher wieder (`probe_gross.txt`); dieselben 400 000 Kopien in 100
   Stuecken zu 4000 auf vier Allokatoren im Kreis +4,8 / +3,5 MB (`probe_stuecke.txt`). Der D3D11-Treiber schickt in Stuecken.
   **Abhilfe:** Befehlsspeicher aus einem Vorrat je Liste (zurueck mit dem Zaunwert der Abgabe, dann Reset), hoechstens 16
   unterwegs; eine Liste geht nach 8192 Zeichenbefehlen (und UpdateSubresource) am naechsten Zeichenbefehl ab (`BHED_D3D12_LISTE`,
   0 = ohne Grenze). Die gemessenen Ansichten (hoechstens 4768 je Bild) werden dadurch nicht geteilt (v4: Abgaben je Bild mit
   Grenze 8192 2,18 / ohne 2,37 - die Zahl folgt der Prozessorlast). Grenze 8192 / 16384 / 32768 gleich gut (v4: privat nach der
   .map 6770 / 6745 / 6789, ohne 6932).
2. **Seiten der dynamischen Puffer.** Die Seiten (1 MB) der Groessenklassen gingen nie zurueck; im Karten-Editor lagen 26-37 MB
   freie Bloecke. Jetzt geht eine Seite zurueck, wenn alle ihre Bloecke 600 Bilder lang frei waren, nach dem Laden eines Levels
   (render::grafikTrimmenAnfordern, unter Direct3D 11 IDXGIDevice3::Trim) sofort; abgelaufene grosse Bloecke und Puffer im Vorrat
   gehen je Bild so viele, wie in 0,5 ms passen, statt hoechstens einer (Befund 8). Frei im Karten-Editor danach 8-11 MB (v6m12).
3. **Kleine Texturen.** Der D3D11-Treiber legt Texturen bis 64 KB dicht (Probe: 500 x 32x32 mit Mips 4,1 MB, als eigene
   D3D12-Zuteilungen 31,2 MB; 128x128 mit Mips unter beiden 128 KB). In den Laeufen gibt es nur 337 solche Texturen (14 MB).
   Versuch gemeinsame Heaps (v1m12 gegen v1m12k0): privat gleich, lokal +2..6 MB (halb leere 4-MB-Heaps) - wieder entfernt.
4. **Start.** `D3D12-Start Speicher`: Warteschlange und Swapchain +109 MB privat / +94 lokal / +59 nicht lokal. Die drei
   Hintergrundpuffer (je 14,9 MB) liegen unter Optimus zweimal im Grafikspeicher und einmal im Arbeitsspeicher der Karte; Direct3D 11
   belegt nach dem Start 52 / 45 MB insgesamt. Nur mit weniger Hintergrundpuffern (3 wie Direct3D 11) zu aendern - nicht gemacht.
5. **Ruhende Kopien** (53 MB im Karten-Editor): Kopien ruhender dynamischer Eck- und Indexpuffer im Grafikspeicher (D3D12-2, GPU-Zeit).
   Bleiben.
6. **Ladepunkte** (gleich nach dem ersten fertigen Bild einer Karte): Hochladepuffer der Ladung (44-66 MB, nach 60 Bildern weg) und
   kleine freigegebene Ressourcen, deren Commit der Treiber fuer neue behaelt (Probe: 2000 x 64x64 freigegeben -> nur -8,6 von
   +136 MB, die naechsten 300 Texturen nehmen ihn wieder).
7. **Nicht die Ursache:** Lage der Zuteilungen (nicht im bevorzugten Segment 0, herabgestuft 0 an allen Punkten), Eckpuffer der
   Oberflaeche (imgui_impl_dx12, hoechstens 4,4 MB), Lesepuffer (10 MB, unter Direct3D 11 ebenso STAGING).
8. **Lange Bilder durch das Zurueckgeben (in diesem Auftrag entstanden und behoben).** Die erste Endmessung des Endstands
   (`messung_final_kand1.txt`, fc7d9cc2d-Baum) zeigte md_am_sith ohne Effekte mit Bildabstand 4,30-4,36 statt 4,18-4,22 ms.
   Laeufe mit `BHED_D3D12_LANGBILD=8` (nur e und ganzer Lauf, Endstand gegen e4179bddf abwechselnd, `diag_abstand*.sh`,
   `dab*.py`): der Endstand hatte beim Umschalten auf "Kamera dreht", "mit Halmen", "mit Effekten" einzelne Bilder von 20-43 ms,
   e4179bddf nicht. Die erweiterte Zeile "langes Bild" (Vorraete gekuerzt, Seiten angelegt, Befehlsspeicher) nannte die Ursache:
   153 Freigaben in einem Bild 13,6 ms, 382 Freigaben 34,9 ms (je Release ~0,09 ms im Systemkern) - "alle Faelligen je Bild"
   (Befund 2). Eine Zeitgrenze je Bild allein reichte nicht: ein einzelnes Release kostete gelegentlich 8,7 / 9,5 ms.
   **Abhilfe:** ein eigener Freigabefaden ruft Release, das Uebergeben an ihn ist
   auf 0,5 ms je Bild begrenzt (`BHED_D3D12_KUERZEN_MS`, 0 = ohne), eine Seite geht zurueck, ohne die Freiliste zu durchsuchen
   (Generation je Seite). `BHED_D3D12_FREIGABEFADEN=0` schaltet den Faden ab. Selbsttest-Schritt `Leistung: Vorraete ohne lange
   Bilder` (FEHLER, wenn ein Bild mehr als 5 ms dafuer braucht).

### Speicher je Messpunkt (MB, D3D11 / D3D12, je ein Lauf; vorher v1m11/v1m12 = Stand e4179bddf mit Messzeilen, nachher v6m11/v6m12)

In der Endmessung (Abschnitt 4, Median dreier Runden je Weg; dort auch Grafikspeicher und "nicht lokal").

## 2. Halme (duel_yavin, GPU +6,5 % in D3D12-4)

| Pruefung | Befund |
|---|---|
| Nur Abschnitt e (v1h11/v1h12, ohne Missionen davor) | Direct3D 12 schneller (Direct3D 12 / 11): ohne Halme 2,518 / 2,779 ms, mit Halmen 2,793 / 3,132; Himmelsportal 1,08 / 1,24 ms |
| Ganzer Lauf, mehrere Laeufe (v7, v9, v11) | Himmelsportal ("Nebensicht") D3D12 1,49-1,85 ms, D3D11 1,34-1,65; derselbe Bau mit denselben Einstellungen 3,21 und 3,63 ms je Bild (v11t1, v9t1b) |
| Teile des Himmelsportals (`BHED_GPU_ABSCHNITTE_SICHT=himmel`, Versuch, nicht committet) | Karte, Gluehen, Mover gleich verteilt; das Portal zeichnet keine Halme |
| Bildprotokoll mit Schritten, eine Liste je Bild (`BHED_D3D12_TEILABGABE=0`, v10e/v10g, `portal.py`) | dieselben Befehle des Portals in derselben Liste: 1,06-1,12 ms in einem Bild, 1,56-2,42 ms im naechsten - nur e wie ganzer Lauf. Die Mehrzeit sitzt in den speicherlastigen Teilen (1035 Zeichenbefehle der Karte 0,42 -> 0,63-1,27 ms, Aufloesen 0,12 -> 0,45-0,65 ms); Loeschen und Zustandswechsel bleiben gleich |
| Lage im Speicher (`Speicherlage` je Ansicht) | nicht im bevorzugten Segment 0, herabgestuft 0 - auch waehrend yavin |
| Takt (nvidia-smi, 200 ms, mit berichtigtem `tab.py`) | 2460 / 8001 MHz unter beiden; die Aussage von D3D12-4 bleibt (dort nahm `WERT=Uhr` fuer jede Spalte das Taktprotokoll des letzten Laufs) |
| Teilabgabe aus (v9t0a/b) | Portal 1,72-1,81 (mit Teilabgabe 1,72-1,77), Bildabstand 5,3-6,3 statt 4,2-4,6 ms - Teilabgabe bleibt |
| Erste Teilabgabe spaeter (2 / 3 ms, v11) | Portal 1,71 / 1,85 (1 ms: 1,49), Bildabstand mit Halmen 5,3 / 5,7 ms - keine Abhilfe |
| Befehlsspeicher behoben (v7: neu gegen alt) | yavin unveraendert im Rahmen der Streuung |
| Endmessung 16./17.11. nachts (Abschnitt 4, je 3 Runden) | Direct3D 11 / 12 ohne Halme 2,772 / 3,014 (vorher), 2,756 / 3,004 (nachher 2) und 2,723 / 3,032 ms (Endbau) = +9..+11 %; mit Halmen 3,308 / 3,246, 3,295 / 3,252 und 3,290 / 3,273 = -1..-2 %. In D3D12-4 (14.11.) ohne +0 %, mit +7 %: der Nachteil wandert zwischen den Ansichten |

Deutung: Innerhalb EINER Befehlsliste aendert sich am Befehlsstrom nichts, trotzdem ist die Grafikkarte in manchen Bildern bei
denselben speicherlastigen Befehlen halb so schnell. Das passt zu etwas, das gleichzeitig auf der Karte laeuft - am
wahrscheinlichsten die Kopie des vorigen Bildes zur Intel-Grafik (Optimus: Copy-Knoten 1,15 ms je Bild unter beiden Wegen), die
mit dem Anfang des naechsten Bildes zusammenfaellt, und das Himmelsportal ist dessen erste grosse Arbeit. Wie stark sich beides
ueberlappt, haengt davon ab, wann der Prozessor das Bild abgibt - unter Direct3D 11 machten die Halme (mehr Arbeit des Prozessors
vor dem Portal) das Portal in D3D12-4 schneller, heute langsamer. Nachweisen laesst sich die Ueberlappung ohne Ereignisaufzeichnung
der Karte (GPUView/ETW braucht Adminrechte) nicht; ein Bild auf einem Schirm direkt an der NVIDIA-Karte gibt es auf diesem Rechner
nicht. Ein spaeterer Abgabebeginn (Versuch 11) aenderte nichts Messbares. Was es kostet: im Mittel der Laeufe mit Halmen 0,1-0,3 ms
GPU-Zeit je Bild, der Bildabstand bleibt bei 240 Hz (4,2-4,6 ms) wie unter Direct3D 11 (4,2-4,4).

## 3. Rot / Gruen

| Punkt | Rot | Gruen |
|---|---|---|
| Befehlsspeicher (privat MB nach der .map / mit Echtzeit-Licht / Ende) | ohne Grenze je Liste (`BHED_D3D12_LISTE=0`, v3m12l0): 7760 / 7640 / 7758; Stand e4179bddf (v1m12): 7733 / 7609 / 7727 | Grenze 8192 (v3m12): 7520 / 7402 / 7525; D3D11 (v3m11): 7430 / 7369 / 7504 |
| Selbsttest `Leistung: Befehlslisten begrenzt` (neu) | `BHED_D3D12_LISTE=0` (lrot): 1 FEHLER "eine Befehlsliste ueber der Grenze (hoechstens 302814 Zeichenbefehle je Liste, Grenze 0)" | Vorgabe (tB_le12): OK, hoechstens 8193 (gezaehlt einschliesslich des Zeichenbefehls, der die Liste abschickt), Grenze 8192; ganzer Lauf 6 OK / 0 FEHLER |
| Speicherspur: "nicht lokal" ausserhalb von Bloecken/Hochladepuffern | Stand e4179bddf (v2s12): +106 MB in den Bildern 6554-6556 (431k/355k Zeichenbefehle), bleibt | v3m12: keine Stufe mehr; Befehlsspeicher 11-27 angelegt, hoechstens 16 unterwegs |
| Seiten und Vorraete (freie Bloecke im Karten-Editor; privat mit Echtzeit-Licht / Ende) | v5m12: frei 26 MB; 7449 / 7568 | v6m12: frei 11 / 8 MB; 7379 / 7493 |
| Messablauf: Messzeile "Speicherlage" je Ansicht (Gegenprobe, D3D12, lange Bilder ab 8 ms) | gp_k1..3 (783dbb16a): an fast jedem Uebergang zwischen zwei Ansichten ein Bild von 20-30 ms ohne Posten im Abgeber, gp_v nicht | gp2_k1..3 (cadde94b5, Zeile nur mit `BHED_LEISTUNG_SPEICHERLAGE=1`): keine solchen Bilder, wie gp2_v |
| Vorraete zurueckgeben ohne lange Bilder (Selbsttest `Leistung: Vorraete ohne lange Bilder`, nur e, Endbau 783dbb16a) | `BHED_D3D12_KUERZEN_MS=0 BHED_D3D12_FREIGABEFADEN=0` (dak4_rot): hoechstens 40,08 ms in einem Bild, FEHLER; lange Bilder "gekuerzt 155 / 15,3 ms" und "382 / 40,1 ms" | Vorgabe (dak4_g1): 0,08 ms, OK; vorher dreimal mit Faden (dak3_g1..3) 0,25 / 0,91 / 0,12 ms, kein langes Bild durch das Zurueckgeben |

## 4. Endmessung

`messung_final.sh` wie in D3D12-4 (Vorlage dx12d): Anlauf D3D12, dann drei Runden, je Runde Direct3D 11 und 12 abwechselnd
(Reihenfolge wechselt je Runde), normale Prioritaet, allein; danach je ein Profil-Lauf (Hauptfaden je Bild). Gestartet von
`endmessung_waechter.py` (Vorgabe Hauptchat): erst wenn die Karte 5 Minuten frei war (kein fremdes Programm auf der NVIDIA-Karte
laut nvidia-smi, kein cl/link/cmake/ctest, kein rechnendes MSBuild, keine fremde Messsperre), Messsperre alle 4 Minuten erneuert,
Abbruch und Verwerfen bei fremdem Spiel oder Bau (kam nicht vor). Am Tag lief die Karte mit fremden MovieDuels-Spielen und einem
Steam-Spiel; frei war sie ab 22:59. Fremdlast in allen Laeufen 0,7-1,8 Kerne.

| Teil | Bau | Zeit | Ergebnis |
|---|---|---|---|
| vorher | e4179bddf (`vor_exe`) | 16.11. 23:04-23:27 | 7 Laeufe exit 0; `messung_vorher.txt` |
| nachher 1 | fc7d9cc2d-Baum (`kand1_exe`) | 23:34-23:56 | `messung_final_kand1.txt` - md_am_sith ohne Effekte Bildabstand 4,30-4,36 ms: Befund 8 |
| Profile vorher | e4179bddf | 17.11. 00:01-00:09 | `messung_vorher_profil.txt` (die Profile des ersten Teils fehlten, siehe unten) |
| nachher 2 | 783dbb16a (`kand2_exe`, Abhilfe Befund 8) | 01:03-01:25 | `messung_final_kand2.txt` |
| nachher (Endbau) | cadde94b5 (`kand_exe`, dazu Speicherlage nur auf Wunsch, siehe Gegenprobe) | 02:07-02:29 | 7 Laeufe exit 0; `messung_final.txt` - die Tabellen unten |

Der erste Waechter startete messung_final.sh ohne gueltige Standardeingabe: in profil12.py scheiterte am Ende `taskkill`
(WinError 6), die Profile fehlten und zwei eigene jkstudio.exe liefen weiter (um 23:27 beendet, eigene PIDs; die Messrunden
lagen davor und sind nicht betroffen). Behoben im Waechter (stdin DEVNULL, nach jedem Teil eigene jkstudio.exe unter
`dx12e\laeufe` beenden), Profile des Vorher-Stands nachgeholt.

**Ergebnis:**

- **Speicher:** privat Direct3D 12 gegen Direct3D 11 nach der .map +294 -> +57 MB (nachher 2: +32), mit Echtzeit-Licht +259 -> +30,
  Ende +239 -> +19, nach duel_mustafar +154 -> +33, nach der Szene +166 -> +96. Direct3D 12 selbst: nach der .map 7727 -> 7482 MB,
  mit Echtzeit-Licht 7617 -> 7388, Ende 7731 -> 7507. Wohin die gut 200 MB gingen: in den Befehlsspeicher, den der Treiber nach
  den Bildern mit 355 000-431 000 Zeichenbefehlen behielt (Befund 1: +106 MB "nicht lokal" in drei Bildern, Probe 400 000 Kopien
  +50 MB privat ohne Rueckgabe), dazu bis 70 MB in leeren Seiten und Vorraeten (Befund 2). Grafikspeicher nach der .map
  +325 -> +271, mit Echtzeit-Licht +168 -> +122 MB; wo Direct3D 11 mehr belegt (Mission, Ende), bleibt es so.
- **GPU-Zeit:** Karten-Editor, Model und Menue Direct3D 12 gegen Direct3D 11 vorher -14..+5 %, nachher -5..+6 % (ohne yavin
  ohne Halme: +9 / +11 %, Abschnitt 2); Echtzeit-Licht vorher +2 / +5 %, nachher -1 / +1 %. Die Messfenster der Missionen schwanken zwischen den Sitzungen unter
  BEIDEN Wegen stark (md_toaj Flug Direct3D 11 1,229 / 1,690 / 1,745 / 1,294 ms, Direct3D 12 1,284 / 1,326 / 1,338 / 1,746 in
  D3D12-4, vorher, nachher 2, nachher - zweigipflig), deshalb dort kein Schluss aus einer Sitzung.
- **Bildabstand:** Karten-Editor, Model, Menue nachher je Fenster Direct3D 12 gegen Direct3D 11 -2..+1 % (vorher -2..+2 %);
  md_am_sith ohne Effekte 4,18 (nachher 1: 4,30-4,36, Befund 8). Die Gegenprobe (unten, eine Sitzung, abwechselnd) vergleicht
  vorher und nachher direkt.
- **Prozessor (Hauptfaden je Bild):** die Profile (Stichproben, `messung_vorher_profil.txt` / `messung_final_kand2.txt`) zeigen
  Direct3D 12 vorher/nachher gleich (Karten-Editor 2,06-3,22 / 2,06-3,00 ms, Menue 0,50-0,52 / 0,54-0,55). Die Bildzeit-Mittel
  streuen zwischen Sitzungen auch unter Direct3D 11 stark (Echtzeit-Licht ruhend 1,87 vorher, 3,33 nachher).
- **Halme:** Abschnitt 2 - ohne Halme +9..+11 %, mit Halmen -1..-2 % (vorher wie nachher).

**Gegenprobe Bildabstand** (`gegenprobe.sh`, `gegenprobe2.sh`: nur Direct3D 12, ganzer Lauf, vorher und nachher abwechselnd je
dreimal in einer Sitzung, lange Bilder ab 8 ms im Protokoll):

| Satz | Bau nachher | Ansichten | Bildabstand vorher (3 Laeufe) | nachher (3 Laeufe) |
|---|---|---|---|---|
| gp (01:41-01:56) | 783dbb16a | Karten-Editor, Model, Menue (ohne Echtzeit-Licht) | 4,226 / 4,186 / 4,200 -> 4,204 ms | 4,198 / 4,209 / 4,213 -> 4,207 ms |
| gp | 783dbb16a | Missionen | 4,427 / 4,487 / 4,513 -> 4,476 | 4,492 / 4,447 / 4,452 -> 4,463 |
| gp2 (02:29-02:44) | cadde94b5 | Karten-Editor, Model, Menue | 4,195 / 4,236 / 4,172 -> 4,201 | 4,196 / 4,190 / 4,179 -> 4,188 |
| gp2 | cadde94b5 | Missionen | 4,460 / 4,537 / 4,492 -> 4,496 | 4,438 / 4,432 / 4,507 -> 4,459 |

Mittel je Lauf ueber die Fenster der Gruppe (Werte je Fenster: `python -I tab.py gp2_v1 ... gp2_k3 WERT=Abstand`). Einzelne Fenster
springen in BEIDEN Staenden zwischen 4,17 und 4,3-4,4 ms (ein einzelnes langes Bild in 240), im Mittel nachher nicht schlechter.
In gp hatte der Endstand an fast jedem Uebergang zwischen zwei Ansichten ein Bild von 20-30 ms ohne Posten im Abgeber: die neue
Messzeile `Leistung: Speicherlage` je Ansicht (VirtualQuery ueber den Adressraum, eine DXGI-Fabrik, D3DKMTQueryStatistics je
Segment) - eine Messhilfe, die den Messablauf veraenderte. Seit cadde94b5 nur mit `BHED_LEISTUNG_SPEICHERLAGE=1`, an den
Messpunkten nur QueryVideoMemoryInfo; in gp2 sind diese Bilder weg (wie vorher).

**GPU (ms, D3D11 / D3D12)**

| Ansicht | D3D12-4 Endmessung (14.11. nachts) | vorher e4179bddf (16./17.11.) | nachher cadde94b5 (17.11.) |
|---|---|---|---|
| Mission mp/duel1 ruhend | 0.162 / 0.154 (-5 %) | 0.160 / 0.155 (-3 %) | 0.161 / 0.156 (-3 %) |
| Mission md_toaj_jedi ruhend | 0.183 / 0.184 (+1 %) | 0.188 / 0.202 (+7 %) | 0.190 / 0.181 (-5 %) |
| Mission md_toaj_jedi Flug | 1.690 / 1.284 (-24 %) | 1.229 / 1.326 (+8 %) | 1.294 / 1.746 (+35 %) |
| Mission Szene angehalten | 0.444 / 0.443 (-0 %) | 0.452 / 0.361 (-20 %) | 0.413 / 0.439 (+6 %) |
| Mission Szene spielt | 2.094 / 1.539 (-27 %) | 2.637 / 1.824 (-31 %) | 2.479 / 1.581 (-36 %) |
| Mission duel_mustafar ruhend | 0.389 / 0.382 (-2 %) | 0.385 / 0.414 (+8 %) | 0.380 / 0.411 (+8 %) |
| Map Editor Vorlage wie im Spiel ruhend | 2.507 / 2.486 (-1 %) | 2.515 / 2.484 (-1 %) | 2.512 / 2.481 (-1 %) |
| Map Editor Vorlage wie im Spiel Kamera dreht | 2.084 / 2.033 (-2 %) | 2.075 / 2.037 (-2 %) | 2.076 / 2.028 (-2 %) |
| Map Editor duel_yavin ohne Halme | 3.011 / 3.011 (+0 %) | 2.772 / 3.014 (+9 %) | 2.723 / 3.032 (+11 %) |
| Map Editor duel_yavin mit Halmen | 3.072 / 3.273 (+7 %) | 3.308 / 3.246 (-2 %) | 3.290 / 3.273 (-1 %) |
| Map Editor md_am_sith ohne Effekte | 2.621 / 2.639 (+1 %) | 2.652 / 2.638 (-1 %) | 2.654 / 2.660 (+0 %) |
| Map Editor md_am_sith mit Effekten | 2.834 / 2.706 (-5 %) | 2.812 / 2.725 (-3 %) | 2.847 / 2.705 (-5 %) |
| Map Editor Echtzeit-Licht ruhend | 22.430 / 21.954 (-2 %) | 21.561 / 21.987 (+2 %) | 22.096 / 21.934 (-1 %) |
| Map Editor Echtzeit-Licht Kamera dreht | 11.792 / 11.943 (+1 %) | 11.322 / 11.894 (+5 %) | 11.727 / 11.856 (+1 %) |
| Model stormtrooper ruhend | 0.403 / 0.391 (-3 %) | 0.398 / 0.398 (+0 %) | 0.404 / 0.388 (-4 %) |
| Model stormtrooper Animation und Drehteller | 0.449 / 0.466 (+4 %) | 0.475 / 0.415 (-13 %) | 0.440 / 0.436 (-1 %) |
| Menu Editor ui/main.menu ruhend | 0.398 / 0.394 (-1 %) | 0.405 / 0.423 (+4 %) | 0.398 / 0.388 (-3 %) |
| Menu Editor ui/main.menu Maus bewegt | 0.390 / 0.405 (+4 %) | 0.410 / 0.352 (-14 %) | 0.394 / 0.417 (+6 %) |

**CPU (ms, D3D11 / D3D12)**

| Ansicht | D3D12-4 Endmessung (14.11. nachts) | vorher e4179bddf (16./17.11.) | nachher cadde94b5 (17.11.) |
|---|---|---|---|
| Mission mp/duel1 ruhend | 0.44 / 0.46 (+5 %) | 0.35 / 0.34 (-3 %) | 0.43 / 0.38 (-12 %) |
| Mission md_toaj_jedi ruhend | 0.63 / 0.66 (+5 %) | 0.52 / 0.51 (-2 %) | 0.66 / 0.60 (-9 %) |
| Mission md_toaj_jedi Flug | 3.09 / 3.09 (+0 %) | 2.99 / 3.00 (+0 %) | 3.04 / 3.19 (+5 %) |
| Mission Szene angehalten | 0.95 / 1.00 (+5 %) | 0.96 / 0.95 (-1 %) | 0.98 / 1.02 (+4 %) |
| Mission Szene spielt | 3.47 / 3.46 (-0 %) | 4.22 / 4.39 (+4 %) | 3.89 / 4.34 (+12 %) |
| Mission duel_mustafar ruhend | 1.01 / 1.09 (+8 %) | 1.20 / 1.19 (-1 %) | 1.06 / 1.06 (+0 %) |
| Map Editor Vorlage wie im Spiel ruhend | 2.19 / 2.24 (+2 %) | 2.09 / 2.31 (+11 %) | 2.17 / 2.28 (+5 %) |
| Map Editor Vorlage wie im Spiel Kamera dreht | 2.14 / 2.16 (+1 %) | 2.09 / 2.28 (+9 %) | 2.18 / 2.25 (+3 %) |
| Map Editor duel_yavin ohne Halme | 1.98 / 2.06 (+4 %) | 1.95 / 2.09 (+7 %) | 1.93 / 2.03 (+5 %) |
| Map Editor duel_yavin mit Halmen | 2.08 / 2.12 (+2 %) | 2.10 / 2.18 (+4 %) | 2.06 / 2.18 (+6 %) |
| Map Editor md_am_sith ohne Effekte | 1.52 / 1.74 (+14 %) | 1.84 / 2.01 (+9 %) | 1.84 / 1.98 (+8 %) |
| Map Editor md_am_sith mit Effekten | 2.43 / 2.41 (-1 %) | 2.76 / 3.02 (+9 %) | 2.79 / 3.00 (+8 %) |
| Map Editor Echtzeit-Licht ruhend | 3.36 / 3.72 (+11 %) | 1.87 / 2.06 (+10 %) | 3.33 / 2.60 (-22 %) |
| Map Editor Echtzeit-Licht Kamera dreht | 3.02 / 3.53 (+17 %) | 2.81 / 3.02 (+7 %) | 3.23 / 3.45 (+7 %) |
| Model stormtrooper ruhend | 0.62 / 0.73 (+18 %) | 0.66 / 0.68 (+3 %) | 0.69 / 0.70 (+1 %) |
| Model stormtrooper Animation und Drehteller | 0.72 / 0.85 (+18 %) | 0.79 / 0.77 (-3 %) | 0.74 / 0.81 (+9 %) |
| Menu Editor ui/main.menu ruhend | 0.50 / 0.62 (+24 %) | 0.45 / 0.45 (+0 %) | 0.46 / 0.50 (+9 %) |
| Menu Editor ui/main.menu Maus bewegt | 0.42 / 0.53 (+26 %) | 0.45 / 0.45 (+0 %) | 0.47 / 0.52 (+11 %) |

**Abstand (ms, D3D11 / D3D12)**

| Ansicht | D3D12-4 Endmessung (14.11. nachts) | vorher e4179bddf (16./17.11.) | nachher cadde94b5 (17.11.) |
|---|---|---|---|
| Mission mp/duel1 ruhend | 4.17 / 4.18 (+0 %) | 4.17 / 4.17 (+0 %) | 4.22 / 4.17 (-1 %) |
| Mission md_toaj_jedi ruhend | 4.30 / 4.34 (+1 %) | 4.19 / 4.17 (-0 %) | 4.18 / 4.18 (+0 %) |
| Mission md_toaj_jedi Flug | 5.45 / 5.23 (-4 %) | 5.25 / 5.11 (-3 %) | 5.30 / 5.34 (+1 %) |
| Mission Szene angehalten | 4.50 / 4.60 (+2 %) | 4.43 / 4.18 (-6 %) | 4.60 / 4.41 (-4 %) |
| Mission Szene spielt | 4.18 / 4.23 (+1 %) | 4.46 / 4.95 (+11 %) | 4.34 / 4.53 (+4 %) |
| Mission duel_mustafar ruhend | 4.17 / 4.18 (+0 %) | 4.17 / 4.44 (+6 %) | 4.17 / 4.41 (+6 %) |
| Map Editor Vorlage wie im Spiel ruhend | 4.17 / 4.20 (+1 %) | 4.20 / 4.17 (-1 %) | 4.17 / 4.20 (+1 %) |
| Map Editor Vorlage wie im Spiel Kamera dreht | 4.17 / 4.18 (+0 %) | 4.17 / 4.17 (+0 %) | 4.18 / 4.20 (+0 %) |
| Map Editor duel_yavin ohne Halme | 4.17 / 4.22 (+1 %) | 4.17 / 4.25 (+2 %) | 4.17 / 4.19 (+0 %) |
| Map Editor duel_yavin mit Halmen | 4.18 / 4.20 (+0 %) | 4.20 / 4.18 (-0 %) | 4.17 / 4.18 (+0 %) |
| Map Editor md_am_sith ohne Effekte | 4.18 / 4.22 (+1 %) | 4.17 / 4.18 (+0 %) | 4.17 / 4.18 (+0 %) |
| Map Editor md_am_sith mit Effekten | 4.18 / 4.18 (+0 %) | 4.18 / 4.17 (-0 %) | 4.17 / 4.18 (+0 %) |
| Map Editor Echtzeit-Licht ruhend | 22.50 / 21.97 (-2 %) | 21.59 / 22.02 (+2 %) | 22.23 / 22.04 (-1 %) |
| Map Editor Echtzeit-Licht Kamera dreht | 12.03 / 12.03 (+0 %) | 11.66 / 11.95 (+2 %) | 12.02 / 12.14 (+1 %) |
| Model stormtrooper ruhend | 4.20 / 4.25 (+1 %) | 4.17 / 4.18 (+0 %) | 4.17 / 4.18 (+0 %) |
| Model stormtrooper Animation und Drehteller | 4.25 / 4.25 (+0 %) | 4.20 / 4.17 (-1 %) | 4.17 / 4.22 (+1 %) |
| Menu Editor ui/main.menu ruhend | 4.29 / 4.27 (-0 %) | 4.25 / 4.23 (-0 %) | 4.17 / 4.20 (+1 %) |
| Menu Editor ui/main.menu Maus bewegt | 4.17 / 4.27 (+2 %) | 4.24 / 4.17 (-2 %) | 4.31 / 4.22 (-2 %) |

**Speicher privat (MB, D3D11 / D3D12, Unterschied)**

| Messpunkt | D3D12-4 Endmessung (14.11. nachts) | vorher e4179bddf (16./17.11.) | nachher cadde94b5 (17.11.) |
|---|---|---|---|
| Start | 292 / 321 (+29) | 293 / 323 (+30) | 294 / 319 (+25) |
| nach maps/mp/duel1.bsp | 897 / 1001 (+104) | 902 / 1001 (+99) | 899 / 988 (+89) |
| nach maps/md_toaj_jedi.bsp | 1908 / 2035 (+127) | 1915 / 2039 (+124) | 1903 / 2017 (+114) |
| nach md_toaj_jedi | 1883 / 1957 (+74) | 1883 / 1961 (+78) | 1886 / 1941 (+55) |
| nach maps/md_am_sith.bsp | 2423 / 2637 (+214) | 2427 / 2642 (+215) | 2428 / 2612 (+184) |
| nach der Szene | 3512 / 3639 (+127) | 3485 / 3651 (+166) | 3489 / 3585 (+96) |
| nach maps/duel_mustafar.bsp | 2542 / 2695 (+153) | 2546 / 2700 (+154) | 2536 / 2569 (+33) |
| nach der .map | 7429 / 7719 (+290) | 7433 / 7727 (+294) | 7425 / 7482 (+57) |
| mit Echtzeit-Licht | 7351 / 7610 (+259) | 7358 / 7617 (+259) | 7358 / 7388 (+30) |
| Ende | 7474 / 7723 (+249) | 7492 / 7731 (+239) | 7488 / 7507 (+19) |

**Speicher Grafik (MB, D3D11 / D3D12, Unterschied)**

| Messpunkt | D3D12-4 Endmessung (14.11. nachts) | vorher e4179bddf (16./17.11.) | nachher cadde94b5 (17.11.) |
|---|---|---|---|
| Start | 52 / 105 (+53) | 52 / 105 (+53) | 52 / 105 (+53) |
| nach maps/mp/duel1.bsp | 331 / 384 (+53) | 331 / 384 (+53) | 331 / 384 (+53) |
| nach maps/md_toaj_jedi.bsp | 563 / 617 (+54) | 563 / 618 (+55) | 563 / 617 (+54) |
| nach md_toaj_jedi | 563 / 159 (-404) | 563 / 159 (-404) | 564 / 159 (-405) |
| nach maps/md_am_sith.bsp | 783 / 707 (-76) | 783 / 707 (-76) | 783 / 677 (-106) |
| nach der Szene | 791 / 716 (-75) | 791 / 716 (-75) | 791 / 686 (-105) |
| nach maps/duel_mustafar.bsp | 839 / 749 (-90) | 840 / 749 (-91) | 840 / 728 (-112) |
| nach der .map | 2441 / 2765 (+324) | 2440 / 2765 (+325) | 2438 / 2710 (+272) |
| mit Echtzeit-Licht | 2441 / 2607 (+166) | 2440 / 2608 (+168) | 2437 / 2560 (+123) |
| Ende | 2433 / 423 (-2010) | 2433 / 424 (-2009) | 2430 / 375 (-2055) |

**Speicher nicht lokal (MB, D3D11 / D3D12, Unterschied)**

| Messpunkt | D3D12-4 Endmessung (14.11. nachts) | vorher e4179bddf (16./17.11.) | nachher cadde94b5 (17.11.) |
|---|---|---|---|
| Start | - | - | 45 / 80 (+35) |
| nach maps/mp/duel1.bsp | - | - | 57 / 141 (+84) |
| nach maps/md_toaj_jedi.bsp | - | - | 77 / 163 (+86) |
| nach md_toaj_jedi | - | - | 67 / 113 (+46) |
| nach maps/md_am_sith.bsp | - | - | 63 / 145 (+82) |
| nach der Szene | - | - | 72 / 149 (+77) |
| nach maps/duel_mustafar.bsp | - | - | 65 / 130 (+65) |
| nach der .map | - | - | 110 / 279 (+169) |
| mit Echtzeit-Licht | - | - | 92 / 191 (+99) |
| Ende | - | - | 99 / 186 (+87) |

## 5. Tests (783dbb16a: `tests5.sh tF`; Endbau cadde94b5: leistung und ctest; Debugschicht an, Leerlauf-Prioritaet, je eigener Laufordner, hoechstens zwei zugleich)

| Test | Direct3D 12 | Direct3D 11 |
|---|---|---|
| grafik ganzer Lauf | 23 OK / 0 FEHLER (Debugschicht 0 Fehler; 4 Warnungen ClearRenderTargetView "clear values do not match", gleich viele wie mit fc7d9cc2d) | 22 / 0 |
| R1 (`BHED_GRAFIK_NUR=verlust`) | 6 / 0 | 5 / 0 |
| verlustreihe (`BHED_GRAFIK_NUR=verlustreihe`, mit Freigabefaden) | 7 / 0 (Objekte frueherer Geraete 0 / 0 / 0) | 6 / 0 |
| DXR (`karteneditor`, `BHED_KARTE_NUR=dxr`) | 36 / 0 | - |
| leistung ganzer Lauf mit Fotos | 6 / 0 (neue Schritte: "Befehlslisten begrenzt" hoechstens 8193 / Grenze 8192, "Vorraete ohne lange Bilder" hoechstens 0,74 ms) | 5 / 0 |
| teil13c mit Fotos | 8 / 0 | 7 / 0 |
| Rot-Probe leistung e mit `BHED_D3D12_LISTE=0` (lrot) | 3 / 1 (der Schritt "Befehlslisten begrenzt", wie erwartet) | - |
| Rot-Probe leistung e mit `BHED_D3D12_KUERZEN_MS=0 BHED_D3D12_FREIGABEFADEN=0` (dak4_rot) | 3 / 1 (der Schritt "Vorraete ohne lange Bilder", 40,08 ms) | - |
| leistung ganzer Lauf mit Fotos auf dem Endbau cadde94b5 (Selbsttest geaendert, `leistung_end.sh`) | 6 / 0 ("Vorraete ohne lange Bilder" hoechstens 0,45 ms; keine Zeile "Speicherlage" mehr ohne Schalter) | 5 / 0 |
| ctest -j32 (Endbau cadde94b5, alle Ziele gebaut, Leerlauf, Waechter `ctest_waechter.py`, `BEHAVED_MD_QUELLEN` = md9) | 285 / 285 bestanden in 232 s, kein Haenger | |

Bau: alle Ziele `--parallel 2`, /W4 /WX: 0 Fehler, 0 Warnungen.

Handbuch: nicht betroffen - nichts Sichtbares geaendert; `BHED_D3D12_LISTE`, `BHED_D3D12_KUERZEN_MS`, `BHED_D3D12_FREIGABEFADEN`,
`BHED_D3D12_SPEICHERSPUR` und `BHED_LEISTUNG_SPEICHERLAGE` sind Schalter fuer Messungen wie die bisherigen `BHED_D3D12_*`.

## 6. Bildvergleich (`vgl.py`, Fotos der Testlaeufe)

| Vergleich | Ergebnis |
|---|---|
| teil13c Direct3D 11 gegen Direct3D 12 (Endbau; k13c_yavin mit/ohne Halme, amsith mit/ohne, mustafar, deathstar, ih) | 7 von 7 bitgleich |
| teil13c Direct3D 12 vorher (e4179bddf) gegen Endbau | 7 von 7 bitgleich |
| leistung Direct3D 12 vorher gegen Endbau | 10 von 11 bitgleich; `leistung_mission` 82 Bildpunkte rechts unten = die fps-Zahl |
| leistung Direct3D 12 fc7d9cc2d-Baum gegen Endbau | 11 von 11 bitgleich |
| leistung Direct3D 11 vorher gegen Endbau | 10 von 11; `editor_echtlicht` 2 Bildpunkte um 1/255. Derselbe Bau streut so: vorher gegen vorher (zweiter Lauf `vor2_le11`) `editor` 19, `editor_echtlicht` 5 Bildpunkte um 1/255, Endbau gegen Endbau 8 / 8. Direct3D 11 ist von den Aenderungen nicht beruehrt (nur Messzeilen) |
| leistung Endbau cadde94b5 (tH) gegen 783dbb16a (tF) | Direct3D 11: 10 von 11 (`editor_echtlicht` 2 Bildpunkte um 1/255, Streuung wie oben); Direct3D 12: 10 von 11 - `menue` 847 339 Bildpunkte: der Menue-Hintergrund ist ein laufendes Video (main.menu), das Foto zeigt im ganzen Lauf ein anderes Bild davon (Auge offen/zu). Gegenprobe nur Abschnitt n (`menue_probe.sh`, gleiche Bedingungen): cadde94b5, 783dbb16a und e4179bddf bitgleich, cadde94b5 zweimal bitgleich |
| leistung Direct3D 11 gegen Direct3D 12 (Endbau) | wie bisher: `editor_vorlage` 1416, `editor` 1076, `editor_echtlicht` 1080 Bildpunkte an Kanten der Mehrfachabtastung (Ursache NOTIZEN_dx12c Punkt 5), `mission` 7, `modell` 3, `modell_rig` 2 Bildpunkte um 1/255; Rest gleich |

## Changelog

## English

- Direct3D 12 uses about 200 MB less memory after a map is opened in the map editor, about as much as Direct3D 11: frames with very many draw calls (up to 430 000 while a .map opens) no longer go to the graphics card as one huge command list whose memory the driver keeps for good. Lists are split after 8192 draw calls and their memory is reused.
- Direct3D 12: memory pages of small dynamic buffers that are no longer used go back to Windows, and the pools of freed buffers shrink right after a level has loaded and otherwise within a few seconds. A background thread releases them a few at a time, so switching views in the map editor does not stutter.
- Performance self-test: memory at each checkpoint as Windows sees it (graphics memory, system memory read by the card, allocations outside their preferred segment) and what the Direct3D 12 renderer keeps in its pools and command memory.

## Deutsch

- Direct3D 12 braucht nach dem Öffnen einer Karte im Karten-Editor rund 200 MB weniger Speicher, etwa so viel wie Direct3D 11: Bilder mit sehr vielen Zeichenbefehlen (bis 430 000 beim Öffnen einer .map) gehen nicht mehr als eine riesige Befehlsliste an die Grafikkarte, deren Speicher der Treiber für immer behält. Listen werden nach 8192 Zeichenbefehlen geteilt, ihr Speicher wird wiederverwendet.
- Direct3D 12: Speicherseiten kleiner dynamischer Puffer, die nicht mehr gebraucht werden, gehen an Windows zurück; die Vorräte freigegebener Puffer schrumpfen gleich nach dem Laden eines Levels, sonst nach wenigen Sekunden. Ein eigener Faden gibt sie nach und nach frei, damit das Umschalten im Karten-Editor nicht ruckelt.
- Leistungs-Selbsttest: Speicher an jedem Messpunkt, wie Windows ihn sieht (Grafikspeicher, von der Karte gelesener Arbeitsspeicher, Zuteilungen außerhalb ihres bevorzugten Segments), und was der Direct3D-12-Abgeber in Vorräten und Befehlsspeicher hält.

## 中文

- 在地图编辑器中打开地图后,Direct3D 12 少用约 200 MB 内存,与 Direct3D 11 大致相当:绘制调用极多的帧(打开 .map 时多达 430 000 次)不再作为一个巨大的命令列表提交给显卡(其内存会被驱动永久保留)。命令列表在 8192 次绘制调用后拆分,其内存会被重复使用。
- Direct3D 12:不再使用的小型动态缓冲区的内存页会归还给 Windows;已释放缓冲区的储备在关卡加载完成后立即缩小,否则在几秒内缩小。由后台线程分批释放,因此在地图编辑器中切换视图时不会卡顿。
- 性能自检:每个测量点按 Windows 统计的内存(显存、显卡读取的系统内存、不在首选段中的分配),以及 Direct3D 12 渲染器在储备和命令内存中保留的内容。

## 日本語

- マップエディターでマップを開いた後、Direct3D 12 のメモリ使用量が約 200 MB 減り、Direct3D 11 とほぼ同じになりました。描画呼び出しが非常に多いフレーム(.map を開くときに最大 430 000 回)を、ドライバーがメモリを永久に保持する巨大なコマンドリスト 1 つとしてグラフィックカードに送らなくなりました。リストは 8192 回の描画呼び出しごとに分割され、そのメモリは再利用されます。
- Direct3D 12:使われなくなった小さな動的バッファーのメモリページは Windows に返され、解放済みバッファーの予備はレベルの読み込み直後に、それ以外は数秒以内に縮小します。解放はバックグラウンドスレッドが少しずつ行うため、マップエディターで表示を切り替えてもカクつきません。
- パフォーマンス自己テスト:各計測点で Windows から見たメモリ(グラフィックメモリ、カードが読むシステムメモリ、優先セグメント外の割り当て)と、Direct3D 12 レンダラーが予備とコマンドメモリに保持している量を記録します。

## Español

- Direct3D 12 usa unos 200 MB menos de memoria tras abrir un mapa en el editor de mapas, más o menos lo mismo que Direct3D 11: los fotogramas con muchísimas llamadas de dibujo (hasta 430 000 al abrir un .map) ya no van a la tarjeta gráfica como una única lista de comandos enorme cuya memoria el controlador conserva para siempre. Las listas se dividen tras 8192 llamadas de dibujo y su memoria se reutiliza.
- Direct3D 12: las páginas de memoria de búferes dinámicos pequeños que ya no se usan vuelven a Windows, y las reservas de búferes liberados se reducen justo después de cargar un nivel y, si no, en pocos segundos. Un hilo en segundo plano los libera poco a poco, así que cambiar de vista en el editor de mapas no produce tirones.
- Autoprueba de rendimiento: la memoria en cada punto de medida tal como la ve Windows (memoria gráfica, memoria del sistema que lee la tarjeta, asignaciones fuera de su segmento preferido) y lo que el renderizador Direct3D 12 guarda en sus reservas y en la memoria de comandos.

## Čeština

- Direct3D 12 po otevření mapy v editoru map spotřebuje asi o 200 MB méně paměti, zhruba tolik co Direct3D 11: snímky s velmi mnoha vykreslovacími voláními (až 430 000 při otevírání .map) už nejdou do grafické karty jako jeden obrovský seznam příkazů, jehož paměť si ovladač ponechá navždy. Seznamy se dělí po 8192 vykreslovacích voláních a jejich paměť se znovu používá.
- Direct3D 12: stránky paměti malých dynamických vyrovnávacích pamětí, které se už nepoužívají, se vracejí systému Windows a zásoby uvolněných vyrovnávacích pamětí se zmenší hned po načtení úrovně, jinak během několika sekund. Uvolňuje je postupně vlákno na pozadí, takže přepínání pohledů v editoru map nezadrhává.
- Výkonnostní samotest: paměť v každém měřicím bodě tak, jak ji vidí Windows (grafická paměť, systémová paměť čtená kartou, alokace mimo preferovaný segment), a co renderer Direct3D 12 drží v zásobách a v paměti příkazů.
