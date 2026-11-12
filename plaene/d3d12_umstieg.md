# Plan: Umstieg auf Direct3D 12 (rc612)

> **Stand 12.11.2026 (D3D12-2): Direct3D 12 ist Vorgabe, Direct3D 11 der automatische Rueckfall** (Start, Startmarke nach
> einem abgestuerzten Start, Neuaufbau nach Geraeteverlust). Teilabgabe nach 1 ms macht den Bildabstand gleich dem von Direct3D 11;
> Messung, Ursachen und Offenes: `docs/intern/NOTIZEN_dx12_1211.md`. Was unten "Vorgabe bleibt Direct3D 11" heisst, ist ueberholt.


Stand 11.11.2026, Agent D3D12 (Fable-Phase 2), Worktree `Y:\p_dx12`, Zweig `p_dx12`, Basis a00f6cf90.
Entscheidung shank 10.11. ~14:30: **nativer D3D12-Weg wird Vorgabe, sobald das Bild pixelgleich ist und er nicht langsamer ist**
(Bildzeit, Ladezeit, Speicher im Ruhefenster); Direct3D 11 bleibt Rueckfall; D3D11On12 hoechstens als Uebergang; EINE Folge von
Zeichenbefehlen mit zwei Abgebern statt jede Zeichenschicht doppelt; Shader bleiben DXBC.
Grundlage: Stufe-0-Messung (Abschnitt 1 und 2, Rohdaten `Y:\werkzeuge\dx12\`), Code `gui/backend_win32.cpp`,
`gui/backend_d3d12_win32.cpp`, `gui/abgeber12*.{h,cpp}`, `gui/gpumap_win32.cpp` (9490 Zeilen, 888 D3D11-Stellen),
`gui/karte_gpu_win32.cpp` (3012 Zeilen, 444 Stellen), `src/q3map2/bhed_dxr.cpp` (D3D12/DXR im Compiler-Prozess).

## 0. Kurzfassung

1. **Gebaut (11.11.): der native D3D12-Abgeber.** Die Zeichenschichten gibt es einmal; ihre Befehlsfolge geht an die
   Schnittstelle `ID3D11Device`/`ID3D11DeviceContext`. Unter Direct3D 11 fuehrt sie die Laufzeit aus, unter Direct3D 12 ein eigener
   Abgeber (`gui/abgeber12*`, 3400 Zeilen in vier neuen Dateien), der dieselbe Schnittstelle bedient und jeden Befehl sofort als D3D12-Befehl in
   die eine Befehlsliste des Bildes schreibt. Kein D3D11On12 mehr, kein zweites Geraet; gpumap/kartegpu bleiben, wie sie sind.
   Damit laufen ALLE 3D-Ansichten nativ (Kartenbild, Figuren, Effekte, Gluehen, Karten-Editor, Echtzeit-Licht, Model-Tab,
   Menue-Figuren) - die Stufen 2 bis 5 sind keine eigenen Ports mehr, sondern Pruefstufen (Bildvergleich je Bereich).
2. **Bild:** siehe Abschnitt 6 - Oberflaeche, Vorschau, Spielbild des Karten-Editors, Model-Tab Bildpunkt fuer Bildpunkt gleich;
   drei untersuchte Reste (Mip-Stufen von Texturen ungerader Groesse, Kanten im Echtzeit-Licht, eine Lavakante in einem
   Kamerabild). Die 69 abweichenden Fotos des Menue-Editors vom 10.11. waren Inhalt (Zeit), nicht Zeichnen.
3. **Tempo:** siehe Abschnitt 6 (Richtwerte unter Last; die Entscheidung braucht `messung_final.sh` im Ruhefenster).
4. **Vorgabe bleibt Direct3D 11**, bis die Messung im Ruhefenster vorliegt und shank umschaltet.
5. **Darauf aufbauend:** DXR-Schatten und Rechen-Shader bekommen ueber `abgeber::ressource()` die D3D12-Ressource hinter jedem
   Puffer und jeder Textur der Zeichenschichten und schreiben in dieselbe Befehlsliste (`rahmen::liste()`).

## 1. Messung Stufe 0: Bildzeiten (Bau a00f6cf90, Direct3D 11)

Selbsttest `leistung`, Abschnitte k s e m n, `BHED_GPU_ABSCHNITTE=1`, `BHED_D3D_DEBUG=0`, warmer Shader-Speicher, allein unter der
Messsperre (10.11. 11:07-11:21, keine fremden Bauten und Selbsttests; Fremdlast laut GetSystemTimes 3,7-5,5 Kerne von 32, davon
sichtbar VS Code/cpptools ~1,2 Kerne - trifft alle Runden gleich). Median der Runden 1-3 (min..max). CPU = newFrame bis vor
Present auf dem Hauptfaden, GPU = Zeitmarken ueber dieselbe Strecke, je 240 Bilder nach 12 Bildern Anlauf.
**Der Bildschirm laeuft mit 240 Hz: ein Bild darf 4,17 ms kosten.**

| Ansicht | CPU Mittel ms | CPU p99 | GPU Mittel ms | Bildabstand ms | gebunden an |
| Mission mp/duel1 ruhend | 0.40 (0.40..0.59) | 0.88 | 0.161 (0.159..0.296) | 4.18 (4.17..4.24) | Bildschirmtakt (239 Hz), frei: 90 % |
| Mission md_toaj_jedi ruhend | 0.55 (0.54..0.73) | 0.93 | 0.177 (0.174..0.190) | 4.17 (4.17..4.18) | Bildschirmtakt (240 Hz), frei: 87 % |
| Mission md_toaj_jedi Flug | 3.65 (3.40..4.11) | 14.55 | 1.889 (1.365..1.895) | 5.53 (5.49..5.69) | Bildschirmtakt (181 Hz), frei: 34 % |
| Mission Szene angehalten | 1.00 (0.99..1.52) | 1.72 | 0.433 (0.355..0.436) | 4.35 (4.29..4.72) | Bildschirmtakt (230 Hz), frei: 77 % |
| Mission Szene spielt | 4.68 (4.61..7.12) | 11.04 | 2.874 (2.861..4.039) | 5.05 (4.93..7.48) | Prozessor |
| Mission duel_mustafar ruhend | 1.27 (1.18..1.69) | 2.23 | 0.364 (0.363..0.368) | 4.17 (4.17..4.17) | Bildschirmtakt (240 Hz), frei: 70 % |
| Map Editor Vorlage wie im Spiel ruhend | 2.89 (2.81..4.25) | 4.58 | 2.850 (2.837..2.921) | 4.23 (4.18..4.68) | Bildschirmtakt (236 Hz), frei: 32 % |
| Map Editor Vorlage wie im Spiel Kamera dreht | 3.01 (2.76..5.85) | 6.44 | 2.459 (2.395..3.230) | 4.29 (4.16..6.30) | Bildschirmtakt (233 Hz), frei: 30 % |
| Map Editor duel_yavin ohne Halme | 2.59 (2.36..2.76) | 6.00 | 3.678 (3.654..3.678) | 4.37 (4.25..4.43) | Grafikkarte |
| Map Editor duel_yavin mit Halmen | 2.90 (2.57..3.12) | 6.48 | 3.736 (3.702..3.763) | 4.26 (4.23..4.29) | Grafikkarte |
| Map Editor md_am_sith ohne Effekte | 2.35 (2.05..2.39) | 3.86 | 3.312 (3.292..3.312) | 4.22 (4.18..4.41) | Bildschirmtakt (237 Hz), frei: 22 % |
| Map Editor md_am_sith mit Effekten | 3.92 (3.43..4.20) | 5.91 | 3.479 (3.445..3.593) | 4.73 (4.39..4.95) | Prozessor |
| Map Editor Echtzeit-Licht ruhend | 6.39 (4.90..7.02) | 14.87 | 21.842 (21.758..21.891) | 21.91 (21.86..22.12) | Grafikkarte |
| Map Editor Echtzeit-Licht Kamera dreht | 4.70 (4.51..6.86) | 16.48 | 11.500 (11.409..11.576) | 12.23 (11.98..12.61) | Grafikkarte |
| Model stormtrooper ruhend | 0.59 (0.48..0.78) | 1.52 | 0.402 (0.402..0.408) | 4.18 (4.17..4.18) | Bildschirmtakt (239 Hz), frei: 86 % |
| Model stormtrooper Animation und Drehteller | 0.64 (0.58..1.07) | 1.41 | 0.474 (0.453..0.474) | 4.23 (4.17..4.25) | Bildschirmtakt (236 Hz), frei: 85 % |
| Menu Editor ui/main.menu ruhend | 0.55 (0.52..0.97) | 1.50 | 0.409 (0.406..0.434) | 4.23 (4.17..4.46) | Bildschirmtakt (236 Hz), frei: 87 % |
| Menu Editor ui/main.menu Maus bewegt | 0.54 (0.46..0.72) | 1.20 | 0.406 (0.404..0.411) | 4.22 (4.17..4.27) | Bildschirmtakt (237 Hz), frei: 87 % |

Lesart "gebunden an": Bildabstand am Takt = Prozessor und Grafikkarte haben Luft (frei = Abstand zum Takt); sonst nennt die
Spalte, was den Abstand bestimmt.

- Oberflaeche ohne grosse 3D-Last (Menue-Editor, Model-Tab, ruhende Mission): 0,4-0,6 ms CPU, 0,2-0,5 ms GPU - 85-90 % Luft.
- Kartenbilder (Editor-Spielbild, Vorlage): CPU 2,4-3,9 ms und GPU 2,5-3,7 ms - beide dicht am Takt von 4,17 ms.
- Mission "Szene spielt": CPU 4,7 ms (p99 11 ms) - ueber dem Takt, der Prozessor bestimmt.
- Flug md_toaj_jedi: CPU-Mittel 3,65 ms, aber p99 14,5 ms (Nachladen im Flug; eigene Arbeitsfaeden rechnen dabei ~14 ms je Bild).
- Echtzeit-Licht (Taris): GPU 21,8 ms ruhend (46 Bilder/s), 11,5 ms bei drehender Kamera - die Grafikkarte bestimmt.

## 2. Messung Stufe 0: Wohin geht die Prozessorzeit je Bild (Profil)

Werkzeug `Y:\werkzeuge\dx12\profil12.py` (neu geschrieben, die Fassung des Vorgaengers lief nie und haette falsch gemessen:
~20 Stichproben/s, Stapel ohne runSession verworfen = alles, was im Treiber abreisst). Jetzt: nur der Hauptfaden, ~250
Stichproben/s, der Faden steht je Stichprobe ~0,1 ms (Stapelkopie, abgewickelt wird danach); eine Stichprobe zaehlt als
Rechenzeit, wenn der Faden in den 0,2 ms danach gerechnet hat (Zyklenzaehler) - Warten auf den Bildschirmtakt und in Present
zaehlt nicht. Gegenprobe je Zeile: Anteil rechnender Stichproben gegen Zyklen des Fadens. Klasse = erstes Modul von oben, das
weder System noch C-Laufzeit ist. Fuer D3D/Treiber steht die Quellzeile des Aufrufs im eigenen Code dabei (inline-genau),
daraus die Art (Zustandswechsel, Map/Update, Present, Draw). Rechenzeit aller anderen Faeden aus ihren Zyklen.
Nur das Messfenster des Selbsttests (die 240 gemessenen Bilder, ohne Anlauf).

| Ansicht | Hauptfaden ms/Bild | eigener Code | D3D11-Laufzeit | Treiber | D3D+Treiber ms/Bild | davon Zustand / Map,Update / Present / Draw (% des Hauptfadens) | Treiberfaden ms/Bild | eigene Arbeitsfaeden ms/Bild | Stichproben |
|---|---|---|---|---|---|---|---|---|---|
| Mission mp/duel1 ruhend | 0.87 | 73 % | 2.3 % | 25.0 % | 0.24 | 0.0 / 0.0 / 22.7 / 0.0 | 0.33 | 0.00 | 73 |
| Mission md_toaj_jedi ruhend | 1.26 | 83 % | 1.7 % | 10.3 % | 0.15 | 1.7 / 1.7 / 6.9 / 0.0 | 0.47 | 5.95 | 90 |
| Mission md_toaj_jedi Flug | 3.17 | 91 % | 5.1 % | 3.8 % | 0.28 | 1.3 / 1.3 / 4.5 / 0.0 | 1.22 | 13.79 | 186 |
| Mission Szene angehalten | 3.72 | 94 % | 0.9 % | 4.9 % | 0.22 | 0.0 / 0.4 / 4.5 / 0.0 | 0.53 | 0.00 | 253 |
| Mission Szene spielt | 5.89 | 81 % | 8.5 % | 10.5 % | 1.12 | 8.1 / 6.1 / 3.1 / 0.0 | 2.84 | 0.00 | 301 |
| Mission duel_mustafar ruhend | 1.61 | 87 % | 7.1 % | 6.2 % | 0.21 | 1.8 / 0.9 / 9.7 / 0.0 | 0.42 | 0.00 | 154 |
| Map Editor Vorlage wie im Spiel ruhend | 4.89 | 77 % | 12.8 % | 10.6 % | 1.14 | 11.5 / 3.0 / 6.4 / 0.9 | 3.26 | 0.00 | 244 |
| Map Editor Vorlage wie im Spiel Kamera dreht | 5.58 | 81 % | 11.7 % | 7.2 % | 1.05 | 8.3 / 4.5 / 3.0 / 0.4 | 3.39 | 1.46 | 267 |
| Map Editor duel_yavin ohne Halme | 3.21 | 77 % | 15.0 % | 7.9 % | 0.74 | 12.1 / 5.1 / 2.8 / 0.5 | 2.37 | 0.00 | 220 |
| Map Editor duel_yavin mit Halmen | 3.16 | 79 % | 12.5 % | 8.9 % | 0.68 | 9.4 / 8.3 / 2.1 / 0.0 | 2.02 | 0.00 | 200 |
| Map Editor md_am_sith ohne Effekte | 2.88 | 83 % | 8.3 % | 8.9 % | 0.50 | 6.8 / 4.7 / 4.2 / 0.0 | 2.06 | 0.00 | 207 |
| Map Editor md_am_sith mit Effekten | 3.73 | 84 % | 6.0 % | 10.4 % | 0.61 | 6.5 / 5.5 / 3.0 / 0.0 | 2.19 | 0.00 | 205 |
| Map Editor Echtzeit-Licht ruhend | 6.30 | 97 % | 1.4 % | 1.9 % | 0.21 | 1.1 / 0.0 / 1.4 / 0.0 | 1.40 | 0.00 | 384 |
| Map Editor Echtzeit-Licht Kamera dreht | 5.56 | 92 % | 2.8 % | 5.6 % | 0.47 | 2.5 / 1.1 / 4.2 / 0.0 | 1.49 | 0.00 | 304 |
| Model stormtrooper ruhend | 0.91 | 92 % | 0.0 % | 6.1 % | 0.06 | 0.0 / 0.0 / 6.1 / 0.0 | 0.47 | 0.00 | 96 |
| Model stormtrooper Animation und Drehteller | 0.95 | 92 % | 4.8 % | 3.6 % | 0.08 | 0.0 / 1.2 / 4.8 / 0.0 | 0.58 | 0.00 | 113 |
| Menu Editor ui/main.menu ruhend | 0.87 | 81 % | 3.8 % | 15.2 % | 0.17 | 0.0 / 3.8 / 12.7 / 0.0 | 0.49 | 0.00 | 103 |
| Menu Editor ui/main.menu Maus bewegt | 0.72 | 80 % | 8.5 % | 11.9 % | 0.15 | 1.7 / 0.0 / 13.6 / 0.0 | 0.40 | 0.00 | 85 |

Hinweis zur Tabelle: dieser erste Profil-Lauf wertet noch den ganzen Schritt aus (mit den 12 Bildern Anlauf und einmaligen
Arbeiten darin); "Hauptfaden ms/Bild" ist deshalb in einigen Zeilen hoeher als die CPU-Zeit aus Abschnitt 1, und der Anteil
"eigener Code" eher zu hoch. profil12.py wertet inzwischen nur das Messfenster aus - der saubere Lauf ist OFFEN (ein
Messfenster: `sh fenster.sh "Profil" sh profil2.sh "11 12" ksemn s1`). Die Groessenordnung aendert das nicht.

Beispiel "Mission Szene spielt" (Hauptfaden 5,9 ms): eigener Code 81 % (zeichneKarte 25 %, Effektnetz bauen 16 %, Mover 13 %,
Heap 7 %), D3D11-Laufzeit + Treiber 19 % = 1,12 ms: Zustandswechsel 8,1 % (PSSetShaderResources 4,1, Konstantenpuffer binden
3,0), Map/Update 6,1 %, Present 3,1 %, Draw 0,0 %. Beispiel "Echtzeit-Licht ruhend" (6,3 ms): eigener Code 97 %, davon
echtLichtBildVorbereiten 52 % (Lichtauswahl sortieren).

## 3. Aufbau: eine Befehlsfolge, zwei Abgeber

**Wo die Folge geschnitten ist.** Die Zeichenschichten setzen heute schon eine flache Folge von Zeichenbefehlen ab: Zustand
setzen, Puffer fuellen, Bereich zeichnen - an `ID3D11DeviceContext`. Diese Folge IST die Stapelliste. Der Schnitt liegt deshalb
an dieser Schnittstelle und nicht eine Ebene darueber:

| Schnitt | Was er kostet | Urteil |
|---|---|---|
| je Zeichenschicht eine zweite Datei (`gpumap_d3d12.cpp`) | 12 500 Zeilen dauerhaft doppelt, jede Aenderung "wie im Spiel" zweimal | verworfen (Entscheidung 10.11.) |
| eigene Stapelliste oberhalb von Direct3D (Zustandsschluessel, Bildsatz, Konstantenblock, Bereich), Zeichenschichten umschreiben | 1300 D3D11-Stellen in gpumap/kartegpu anfassen, waehrend drei Zweige taeglich darin arbeiten (Model-Tab, Karten-Editor); der D3D11-Weg aendert sich mit - Rueckfall nicht mehr "unveraendert" | nicht noetig |
| **die Schnittstelle `ID3D11Device`/`ID3D11DeviceContext` selbst** | ein eigener Abgeber, der sie bedient (3400 Zeilen in vier neuen Dateien); gpumap/kartegpu unveraendert | **gebaut** |

Was dafuer spricht: der D3D11-Weg bleibt Byte fuer Byte derselbe Code (Rueckfall ohne Risiko); alles, was andere Zweige in
den Zeichenschichten aendern, laeuft ohne Zutun auch unter Direct3D 12; jeder Selbsttest prueft beide Abgeber mit demselben
Ablauf. Was dagegen spricht: der Abgeber muss die Regeln von Direct3D 11 nachbilden, die die Zeichenschichten benutzen
(unten) - das ist begrenzt (rund 45 Methoden des Kontexts, 27 des Geraets; alles andere meldet sich im Protokoll als "nicht
umgesetzt" und laesst den Selbsttest scheitern).

Das ist kein D3D11On12: kein zweites Geraet, keine allgemeine Uebersetzungsschicht, keine doppelte Textur, kein Abschicken
mitten im Bild. Der Abgeber kennt die Nutzung und schreibt je Zeichenbefehl nur das Geaenderte.

**Wie der Abgeber arbeitet** (`gui/abgeber12.h`, `abgeber12_intern.h`, `abgeber12_geraet_win32.cpp`, `abgeber12_kontext_win32.cpp`):

- **Eine Befehlsliste je Bild** (`backend_d3d12_win32.cpp`): zuerst die Befehle der 3D-Ansichten, am Ende die Oberflaeche
  (imgui_impl_dx12). Davor laeuft die Vor-Liste mit dem Hochladen neuer Texturen und Puffer.
- **Zustandsobjekte -> Pipeline-Zustand (PSO).** Shader (DXBC unveraendert), Eingabe, Misch-, Tiefen- und Rasterzustand,
  Zielformate, Probenzahl und Topologieart bilden einen Schluessel (48 Byte); das PSO entsteht beim ersten Gebrauch und bleibt.
  Gleiche Beschreibung -> dasselbe Zustandsobjekt (wie Direct3D 11), das haelt die Zahl der PSOs klein.
- **Eine Wurzelsignatur fuer alles:** Konstantenpuffer als Root-CBV mit Adresse (b0-b7 Vertex, b0-b7 Pixel, b0-b3 Geometrie;
  `VSSetConstantBuffers1` mit Versatz = Adresse + erste Konstante * 16), Texturen und Abtaster je Platz eine Tabelle mit EINEM
  festen Deskriptor (t0-t15, s0-s3) - binden heisst eine Adresse setzen, nichts wird je Zeichenbefehl kopiert. Ohne Bindung:
  eine Seite Nullen, die leere Sicht, der Vorgabe-Abtaster von Direct3D 11.
- **`D3D11_USAGE_DYNAMIC`:** ein Block im Hochladespeicher; `Map(WRITE_DISCARD)` nimmt einen frischen Block (Groessenklassen
  256 B bis 64 KB aus Seiten zu 1 MB, groessere einzeln), der alte wird wieder vergeben, wenn die Grafikkarte die Listen
  abgearbeitet hat, die ihn lesen. `WRITE_NO_OVERWRITE` schreibt in denselben Block (der Konstanten-Ring von rc596 bleibt, was
  er ist).
- **Texturen und Puffer im Grafikspeicher:** Anfangsdaten ueber den Hochladespeicher in der Vor-Liste; `UpdateSubresource`
  an seiner Stelle in der Befehlsliste des Bildes. Zustaende (Ziel, lesbar, Kopierquelle ...) fuehrt der Abgeber je Textur
  und wechselt sie vor dem Befehl, der sie braucht; Puffer werden je Ausfuehrung einmal lesbar gestellt.
- **Lesen (`D3D11_USAGE_STAGING`):** ein Lesepuffer; `Map(READ)` schickt die Liste ab und wartet - wie Direct3D 11. Tiefe mit
  Stencil kommt in der Aufteilung von Direct3D 11 zurueck (zwei Ebenen zusammengesetzt).
- **Zeitmarken:** ein Abfragestapel im Ring, aufgeloest beim Abschicken; `GetData` liefert, sobald die Liste gelaufen ist.
- **Regeln von Direct3D 11, die nachgebildet sind:** eine Textur, die Ziel wird, faellt aus den Textur-Plaetzen (und umgekehrt);
  gebundene Objekte leben, bis sie geloest sind; ohne Tiefenziel keine Tiefenpruefung; ohne `ScissorEnable` das ganze Ziel;
  Vorgaben fuer fehlende Zustandsobjekte; `GenerateMips`; `ResolveSubresource`.
- **Bilder der 3D-Ansichten in der Oberflaeche:** jede Sicht hat ihren Deskriptor im gemeinsamen Stapel - die Oberflaeche zeigt
  ihn direkt (keine Bruecke, keine Kopie).

**Fuer DXR und Rechen-Shader vorbereitet:** `abgeber::ressource(ID3D11Resource*)` gibt die D3D12-Ressource (und bei dynamischen
Puffern den Versatz) hinter jedem Puffer und jeder Textur der Zeichenschichten; `rahmen::liste()` ist die offene Befehlsliste,
`abgeber::fremdGezeichnet()` laesst danach alle Zustaende neu setzen. Ein Schatten-Durchgang mit RayQuery (DXIL, eigenes PSO)
baut seine Beschleunigungsstruktur aus den vorhandenen Eckpuffern und schreibt in dieselbe Liste.

**Mehrkern spaeter:** der Abgeber uebersetzt heute sofort auf dem zeichnenden Faden. Die Stelle fuer eine zweite Stufe ist
dieselbe Schnittstelle: Befehle aufzeichnen und auf einem eigenen Faden uebersetzen (so arbeitet der D3D11-Treiber heute) -
nur wenn die Messung im Ruhefenster zeigt, dass der Hauptfaden mehr traegt als unter Direct3D 11.

## 4. Was vom frueheren Plan gilt

| Frueher | Jetzt |
|---|---|
| Stufe 1 ueber D3D11On12, Bruecke ohne Kopie | ersetzt: nativer Abgeber, D3D11On12 ist ausgebaut (kein `d3d11on12.h`, kein `D3D11On12CreateDevice`) |
| Probe 2a (deckender Kartendurchgang als D3D12-Abgeber, Tor 0,5 ms) | entfaellt: alle Durchgaenge laufen nativ; das Tor ist die Messung im Ruhefenster |
| Stufen 2b-5 als einzelne Ports | Pruefstufen: je Bereich Bildvergleich Direct3D 11 gegen Direct3D 12 und Richtwert |
| Shader bleiben DXBC, dxc/SM6 nur fuer RayQuery | gilt |
| Deskriptoren: ein Stapel, Freigabe erst nach der Grafikkarte | gilt (65536 Plaetze, Freigabe am Zaunwert) |
| Konstanten: Upload-Ring je Bildplatz | ersetzt durch Bloecke mit Groessenklassen (ein Puffer darf laenger als ein Bild leben) |
| `ID3D12PipelineLibrary` neben dem Shader-Speicher | offen (Stufe 7): der Treiber haelt die PSOs schon auf Platte; erst messen, was der erste Start kostet |

## 5. Stufenfolge

| Stufe | Inhalt | Stand |
|---|---|---|
| 0 | Messen, Plan | erledigt (10.11.) |
| 1 | Geraet, Swapchain, Bildtakt, eine Befehlsliste je Bild, Oberflaeche (imgui_impl_dx12), Abgeber, Schalter, Rueckfall | erledigt (11.11.) |
| 2 | Kartenbild (Vorschau): deckend, gemischt, Lightmaps, Himmel, Nebel, Mover | nativ ueber den Abgeber; Fotos `toaj`, `mustafar` gleich |
| 3 | Figuren, Effekte, Gluehen, Nebensichten, Menue-Figuren | nativ; Foto `szene40` (Figuren, Effekte, Gluehen) gleich; Kamerabild der Zeitleiste 7 Bildpunkte (Treiber) |
| 4 | Karten-Editor (kartegpu), Spielbild, Echtzeit-Licht | nativ; Spielbild gleich; Vorlage 1416 Bildpunkte um 1/255 (Mip-Stufen ungerader Groessen), Echtzeit-Licht 1080 Bildpunkte auf Kanten - offen fuer shank |
| 5 | Model-Tab (Modellziele, Malen, UV) | nativ; Foto `modell` gleich |
| 6 | Messung im Ruhefenster (`messung_final.sh`), Entscheidung Vorgabe | offen - Hauptchat |
| 7 | Nach der Messung: Grafikspeicher (Evict/MakeResident im Leerlauf, Schattenkarten-Felder, kleine Texturen in gemeinsamen Heaps), Hauptfaden in zeichenlastigen Ansichten (zweiter Faden), Start (PSO) | offen |
| 8 | DXR-Schatten im Echtzeit-Licht, X-Ray (eigene Auftraege) | offen |

## 6. Ergebnisse

**Bildvergleich Direct3D 11 gegen Direct3D 12 (Abgeber), pixelgenau** - Einzelheiten und Ursachen in
`docs/intern/NOTIZEN_d3d12_1011.md`:

| Bereich | Fotos | Ergebnis |
|---|---|---|
| Oberflaeche | Modus `grafik`: Start, Menue Ansicht, About, festes Bild aller Mischarten (121 Faktorpaare wiederholt und geklemmt, additiv, ohne Mischen) | gleich; ein Foto unterscheidet sich um die Ziffer im Tooltip "laeuft jetzt: Direct3D 11/12" |
| Vorschau: Kartenbild | `leistung_toaj`, `leistung_mustafar` (3D-Ziel 1350 x 931) | gleich |
| Vorschau: Figuren, Effekte, Gluehen | `leistung_szene40` | gleich |
| Vorschau: Kamerabilder der Zeitleiste | `leistung_mission` (ganzes Fenster) | 7 Bildpunkte, hoechstens 2/255, an einer Lavakante in einem Kamerabild (224 x 140); D3D11On12 zeigt genau dasselbe wie der Abgeber |
| Karten-Editor: Spielbild | `leistung_editor_vorlage_spiel` (1920 x 1440) | gleich |
| Karten-Editor: Vorlage | `leistung_editor_vorlage` | 1416 von 1,6 Millionen Bildpunkten um 1/255: Mip-Stufen (`GenerateMips`) von Texturen mit ungerader Groesse |
| Karten-Editor: Echtzeit-Licht | `leistung_editor_echtlicht` | 1080 von 1,07 Millionen Bildpunkten, hoechstens 11/255: duenne Linien auf Dreieckskanten |
| Karten-Editor: Texturbrowser | `leistung_editor` (ganzes Fenster) | gleich (nach dem Angleichen an imgui_impl_dx11: nur die oberste Stufe) |
| Model-Tab | `leistung_modell` | gleich |
| Menue-Editor | 165 Fotos, je 2-3 Laeufe je Schnittstelle (noch ueber D3D11On12) | 89 gleich, 72 schwanken schon unter Direct3D 11 (Videos, Uhrzeit, fps), 4 zeitabhaengig |

**Abgeber in Zahlen** (ein ganzer `leistung`-Lauf, Abschnitte k s e m n): 10,4 Millionen Zeichenbefehle, 113 Pipeline-Zustaende
(1,8 Millionen Wechsel), 7,6 Millionen frische Bloecke (`Map(WRITE_DISCARD)`), 0,85 Millionen Zustandswechsel, 154 MB
Hochladespeicher in Bloecken, 0 nicht umgesetzte Aufrufe.

**Tempo:** Richtwert vom 11.11. (ein Lauf je Schnittstelle, 15-21 von 32 Kernen Fremdlast; Tabelle in den Notizen): keine Ansicht zeigt die
Verlangsamung von D3D11On12; der Hauptfaden ist in 8 von 18 Ansichten schneller, in den zeichenlastigen (Editor-Spielbild, Szene
spielt) unter dieser Last +11 bis +76 % langsamer; Start und Laden in 9 von 10 Werten kuerzer; Grafikspeicher im Leerlauf und im
Karten-Editor mit Echtzeit-Licht deutlich hoeher (616 statt 67 MB, 4784 statt 2507 MB). Entscheiden kann nur `messung_final.sh`
im Ruhefenster.

## 7. Wo Direct3D 12 voraussichtlich nichts bringt

- **Bildrate an der Grafikkarte** (Echtzeit-Licht 21,8 ms): dieselben Shader, dieselbe Zeit - schneller wird es durch weniger
  Arbeit je Punkt oder DXR als andere Schattenart.
- **Eigener Code** (77-97 % des Hauptfadens): unabhaengig von der Schnittstelle, siehe Abschnitt 8.
- **Ruhender Editor:** zeichnet nicht.

## 8. Nebenbefund: wo die Zeit wirklich liegt (eigener Code, Hauptfaden)

Aus dem Profil der Stufe 0, je Bild, ohne dass sich am Bild etwas aendern muesste (Regel "Tempo ohne Qualitaetsverlust": gleiches
Bild, gleiche Dateien - jeder Punkt braucht seinen eigenen Rot/Gruen-Beleg):

| Ansicht | Funktion (inklusive) | Anteil Hauptfaden | etwa ms je Bild |
|---|---|---|---|
| Echtzeit-Licht ruhend | `echtLichtBildVorbereiten` (Lichtauswahl, stable_sort je Bild, obwohl nichts sich bewegt) | 52 % | 3,2 |
| Editor-Spielbild und Echtzeit-Licht | `spielPruefen` -> `importOrdner` -> `platform::settingsDirectory` je Bild | 6-11 % | 0,3-0,5 |
| Editor-Spielbild | `holeBild` + `holeLightmap` (Suche je Stapel) | 15 % | 0,7 |
| Mission Szene spielt | `buildEffectMesh` je Bild | 16 % | 1,0 |
| Mission Szene spielt | Heap (`malloc`/`free`) im Bild, `std::map<int, SRV*>` | 11 % | 0,6 |
| Mission Szene angehalten | Zeitleiste + Ebenen + `MapEntity::find`/`entSchluesselIndex` (Textvergleiche je Bild) | 30 % | 0,3 |
| Menue-Editor (neu, 11.11.) | `lesbarkeitVon` -> `umgebung` -> `FeederDaten::baue` -> `dateiListe` je Bild (alle Archive, tolower je Name) | Stapel eines unter Last stehenden Laufs, nicht gemessen | - |

## 9. Risiken

- **Bitgleichheit** ist belegt fuer das, was Abschnitt 6 fotografiert hat, auf dieser Karte und diesem Treiber (RTX 4070,
  NVIDIA). Andere Hersteller koennen DXBC unter D3D12 anders uebersetzen als unter D3D11 - der Bildvergleich (Modus `grafik`
  und `leistung` mit `BHED_FOTOS`, je Schnittstelle) gehoert deshalb in die Release-Runde.
- **Nicht umgesetzte Methoden:** der Abgeber deckt, was die Zeichenschichten heute rufen. Ruft ein anderer Zweig eine neue
  Methode (Rechen-Shader, UAV, Textur im Vertex-Shader, mehr als 16 Texturen, mehr als 4 Abtaster oder Ziele), steht es als
  Fehler im Protokoll, und jeder Selbsttest unter Direct3D 12 endet mit FEHLER (Befund am Ende des Laufs).
- **Erster Start:** PSOs entstehen beim ersten Gebrauch (Treiber-Speicher auf Platte ab dem zweiten Start). Ladezeit und
  erstes Bild gehoeren in die Messung.
- **Grafikspeicher** (Richtwert 11.11.): Direct3D 12 haelt belegt, was angelegt ist. Der D3D11-Treiber gibt im Leerlauf den
  Grafikspeicher ungenutzter Texturen frei (67 statt 616 MB) und belegt grosse Ziele offenbar erst beim Beschreiben
  (Karten-Editor mit Echtzeit-Licht 2507 statt 4784 MB - dieselbe Zahl wie unter D3D11On12). Vor dem Umschalten der Vorgabe
  zu loesen (Stufe 7): im Leerlauf `Evict`, vor dem Gebrauch `MakeResident`; Schattenkarten-Felder nach Bedarf oder als
  reservierte Ressource. Dazu: jede Textur ist heute eine eigene Zuteilung (mindestens 64 KB).
- **Geraeteverlust:** der Neuaufbau-Weg aus rc596 gilt (render::shutdown/init); unter Direct3D 12 im Selbsttest noch nicht
  gespielt.
- **Eigene Fenster (Multi-Viewport):** imgui_impl_dx12 zeichnet sie in eigene Listen nach der Befehlsliste des Bildes; die
  Gruppen `tabfenster`/`dock` sind unter Direct3D 12 noch nicht gelaufen.

## 10. Naechste Auftraege

1. **Messung im Ruhefenster** (Hauptchat): `sh /y/werkzeuge/dx12/messung_final.sh` (~27 min, nimmt den Bau aus
   `Y:\p_dx12\build\bin\Release`) - Bildzeit, Ladezeit, erstes Bild, Speicher Direct3D 11 gegen Direct3D 12. Danach entscheidet
   shank ueber die Vorgabe.
2. **Entscheidungen von shank zu den drei Bildresten** (Abschnitt 6): Mip-Stufen des Karten-Editors selbst rechnen (dann unter
   beiden Schnittstellen gleich; aendert das Bild unter Direct3D 11 um hoechstens 2/255 in verkleinerten Texturen ungerader
   Groesse); Kanten im Echtzeit-Licht und die Lavakante: Ursache weiter eingrenzen (welcher Durchgang) oder als Unterschied
   des Treibers hinnehmen.
3. **Volle Runde auf dem zusammengefuehrten Stand je Schnittstelle** (`BHED_D3D12=1`): Modus 1, `menueeditor`, `karteneditor`,
   `tabfenster`/`dock` (eigene Fenster), Geraeteverlust; jeder Lauf endet mit dem Befund des Abgebers.
4. **Stufe 7, nach der Messung:** Grafikspeicher angleichen (im Leerlauf `Evict`/`MakeResident`; die Schattenkarten-Felder
   des Echtzeit-Lichts nach Bedarf oder als reservierte Ressource; kleine Texturen in gemeinsame Heaps) - der Richtwert zeigt
   616 statt 67 MB im Leerlauf und 4784 statt 2507 MB im Karten-Editor mit Echtzeit-Licht; Abgabe auf einem zweiten Faden,
   falls der Hauptfaden in zeichenlastigen Ansichten auch im Ruhefenster langsamer ist; PSOs beim Start nur, wenn noetig.
5. **DXR-Schatten im Echtzeit-Licht** (eigener Auftrag): RayQuery-Durchgang ueber `abgeber::ressource()` und `rahmen::liste()`.
6. **Tempo im eigenen Code** (Abschnitt 8), je Punkt ein kleiner Auftrag mit Rot/Gruen-Beleg - der sichere Gewinn, unabhaengig
   von der Schnittstelle.

## 11. Zweite Runde (11.11. nachmittags): Tempo und Speicher des nativen Wegs

Anlass: das Ruhefenster von 14:47 zeigte den nativen Weg langsamer (Karten-Editor: Bildabstand 6,3-7,5 statt 4,17 ms) und mit
mehr Grafikspeicher. shank 11.11.: Direct3D 12 soll Vorgabe werden. Ursachen, Belege und Richtwerte im Einzelnen:
`docs/intern/NOTIZEN_d3d12_1011.md`, Abschnitt "Zweite Runde". Hier, was sich am Aufbau geaendert hat.

**Was der Abgeber jetzt anders macht als in Abschnitt 3 beschrieben:**

- **Dynamische Puffer: Bloecke in gewoehnlichem Speicher.** Die Bloecke kommen aus einem eigenen Heap (CUSTOM, L0, WRITE_BACK)
  statt aus einem Hochlade-Heap: fuer den Prozessor Speicher mit Zwischenspeicher, fuer die Grafikkarte lesbar. Die
  Zeichenschichten lesen stellenweise aus einem abgebildeten Puffer zurueck; aus schreibkombiniertem Speicher kostete das 1,1 ms
  je Bild. Der Block traegt seine Groessenklasse; freie grosse Bloecke gehen nach 600 Bildern ohne Bedarf zurueck.
- **Ruhende dynamische Eck- und Indexpuffer liegen im Grafikspeicher.** Ab 64 KB und nach zwei Bildern ohne `Map` wird der
  Inhalt einmal in eine Kopie im Grafikspeicher kopiert und von dort gezeichnet; das naechste `Map` gilt wieder dem Block.
- **Puffer im Grafikspeicher ohne ausdrueckliche Zustandswechsel:** Direct3D 12 stellt sie aus COMMON beim ersten lesenden
  Gebrauch von selbst lesbar; ausdruecklich nur nach dem Beschreiben in derselben Ausfuehrung. Freigegebene Puffer warten auf
  die Grafikkarte und werden fuer das naechste `CreateBuffer` derselben Groesse wiederverwendet.
- **Zustandswechsel von Texturen gebuendelt** (Quelle und Ziel eines Kopierens/Aufloesens, Ziele und Tiefe eines Durchgangs,
  alle gebundenen Texturen vor einem Zeichenbefehl: je ein Aufruf).
- **Grosse, leer angelegte Texturfelder sind reservierte Ressourcen** (ab 4 Schichten, 32 MB und 1 MB je Schicht, eine Stufe,
  ohne Mehrfachabtastung, Ziel oder nicht; braucht `D3D12_TILED_RESOURCES_TIER_2`): eine Schicht bekommt ihren Speicher (ein
  Heap je Schicht, `UpdateTileMappings`) beim ersten Schreiben - Ziel setzen, Loeschen, Kopieren, Hochladen, Aufloesen. Leere
  Schichten lesen 0. Kopieren aus einer leeren in eine leere Schicht entfaellt. `BHED_D3D12_DUENN=0`: aus.
- **Auslagern** (`auslagernPruefen`, `gebraucht`, `einlagern` in `abgeber12_geraet_win32.cpp`): der D3D11-Treiber haelt nur im
  Grafikspeicher, was gebraucht wird; unter Direct3D 12 ist das Sache des Programms. Jede Stelle, die eine Textur in eine
  Befehlsliste bringt, stempelt sie mit dem laufenden Bild (`stelle`, `stelleZwei`, `wechsle`, Binden als Eingang, die gebundenen
  Eingaenge am Anfang jeder Liste, `sichtZeigen` fuer die Oberflaeche, `ressource()`). Alle 64 Bilder (oder nach einer Sekunde)
  wird geprueft: wer seit 4 Sekunden und 8 Bildern nicht gebraucht wurde und mindestens 1 MB gross ist, gibt ab (`Evict`;
  duenne Felder: ihre Heaps), das am laengsten Ungebrauchte zuerst; der naechste Gebrauch holt den Speicher vorher zurueck
  (`MakeResident`). Der Inhalt bleibt erhalten. **Wer eine neue Stelle baut, die eine Textur
  des Abgebers in eine Liste bringt, muss `gebraucht()` rufen** - sonst zeichnet die Liste mit einer ausgelagerten Ressource
  (die Debugschicht meldet es). `BHED_D3D12_AUSLAGERN=0`: nie; `=budget`: nur, was ueber dem Budget liegt, das das System dem
  Prozess nennt (`QueryVideoMemoryInfo`), bis ein Sechzehntel darunter; `BHED_D3D12_BUDGET_MB=<n>`: ein Budget annehmen (so
  prueft der Selbsttest diesen Weg).
- **Der Abschickfaden** (`backend_d3d12_win32.cpp`): `ExecuteCommandLists`, `Present` und `Signal` laufen als Auftraege in
  ihrer Reihenfolge auf einem eigenen Faden. Der Hauptfaden schliesst die Liste, gibt sie ab und nimmt fuer das naechste Stueck
  eine Liste aus dem Vorrat; Zaunwerte vergibt er selbst. Wer Warteschlange oder Swapchain selbst braucht (Hintergrundpuffer
  waehlen, Groesse aendern, eigene Fenster von imgui_impl_dx12, Abbau), ruft vorher `abschickenAbwarten`. Ein verlorenes Geraet
  faellt ein Bild spaeter auf (das Ergebnis von `Present` kommt vom Faden). Wartet der Hauptfaden laenger als zwei Sekunden,
  arbeitet er gesendete Fensternachrichten ab (DXGI kann aus `Present` an den Faden des Fensters senden).
  `BHED_D3D12_ABSCHICKEN=direkt`: ohne den Faden.
- **Freigegebenes geht am Anfang des naechsten Bildes zurueck** (vorher am Ende).

**Messhilfen, die bleiben:**

- **Bildprotokoll** (`gui/abgeber12_protokoll_win32.cpp`, `BHED_D3D12_PROTOKOLL=<n>`): jedes n-te Bild mit jedem Ziel, Loeschen,
  Aufloesen, Kopieren, Hochladen, Zustandswechsel und Abschicken, je Eintrag eine Zeitmarke der Grafikkarte; dazu Zaehler je Bild
  (Zeichenbefehle, PSO-Wechsel, Zustandswechsel, Abschicken, Warten, angelegte Ressourcen) und ein Speicherbericht (Grafikspeicher
  und Arbeitsspeicher nach Art, die groessten Texturen).
- **Profil mit Quellzeilen und Aufrufstellen** (`Y:\werkzeuge\dx12\profil12.py`): eigener Code je Quellzeile; alles unter dem
  Abgeber (eigener Code, D3D12, Treiber) je Methode und Aufrufstelle der Zeichenschicht - das Gegenstueck zu "D3D + Treiber"
  unter Direct3D 11.
- Selbsttest `leistung`: "Speicher mit Echtzeit-Licht" (nach den Messschritten des Echtzeit-Lichts).

**Stand gegen die Bedingungen fuer die Vorgabe** (Richtwerte; entscheidend ist das Ruhefenster):

- **Pixelgleich:** wie nach der ersten Runde. Gleich: Oberflaeche, `toaj`, `mustafar`, `szene40`, `modell`, Spielbild des
  Karten-Editors. Reste: Kamera des Karten-Editors (1416 und rund 1050 Bildpunkte um 1/255 an Dreieckskanten, nur mit
  Mehrfachabtastung - Ursache eingegrenzt, nicht gefunden), `mission` 7 Bildpunkte.
- **Nicht langsamer - Hauptfaden:** in den Profilen von 16:54 in allen fuenf Ansichten unter Direct3D 11 (vorher +10 bis +64 %).
  Im Laufpaar von 22:08 im Karten-Editor meist deutlich darunter, im Mission-Tab +4 bis +40 % - unter Last nicht zu entscheiden.
- **Nicht langsamer - GPU:** 2D-Ansicht gleich (vorher bis 4,7 ms mehr); Spielbild auf Karten mit Himmelsportal noch +0,4 bis
  +3,5 % (eine Kopie und sieben Aufloesungen mit 8 Proben, im Treiber langsamer).
- **Speicher:** Grafikspeicher im Karten-Editor 2,6-2,8 GB gegen 2,1-2,4 GB unter Direct3D 11 (vorher 4,8 GB), am Ende 0,4 GB;
  unmittelbar nach dem Laden einer .map noch bis 3,8 GB (Abhilfe geschrieben). Arbeitssatz +12 bis +42 % - offen.
- **Entscheidung:** braucht `messung_final.sh` im Ruhefenster; die Richtwerte dieser Runde stammen aus Fenstern neben 5 bis 28
  Kernen Fremdlast.

**Auch an der Zeichenschicht (beide Schnittstellen):** `gpu::zeichneGluehen` legt das Gluehen mit Mehrfachabtastung gleich ins
Ziel auf (es liest dort die aufgeloeste Szene) - der zweite Vollpuffer und die Kopie zurueck entfallen.

**Offen:** GPU-Zeit beim Kopieren/Aufloesen mit Mehrfachabtastung (im Treiber, rund +0,2 ms je Bild im Spielbild des
Karten-Editors auf Karten mit Himmelsportal: eine Kopie und sieben Aufloesungen); Kanten der Kamera des Karten-Editors (1/255,
Ursache eingegrenzt); Grafikspeicher der Swapchain (+52 MB); vorbereitet und nicht angewandt: Konstantenpuffer ueber eine
Schattenkopie (`Y:\werkzeuge\dx12\abg2\p10_schatten.py`), Loeschwert folgt der Loeschfarbe (`p16_loeschwert.py`).
