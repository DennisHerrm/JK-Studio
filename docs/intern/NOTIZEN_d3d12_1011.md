# Notizen D3D12 (10.-11.11.2026): Messen, Plan, nativer Abgeber - rc612

Auftrag `auftrag_d3d12_1011.md` und Entscheidung shank 10.11. ~14:30 ("Der Plan waere tatsaechlich gerne auf D3D12 umzusteigen
damit man aktuell ist"): nativer D3D12-Weg wird Vorgabe, sobald das Bild pixelgleich und er nicht langsamer ist; Direct3D 11
bleibt Rueckfall. Worktree `Y:\p_dx12`, Zweig `p_dx12` ab a00f6cf90, Werkzeuge und Rohdaten `Y:\werkzeuge\dx12`
(UEBERGABE.md dort nennt jedes Skript). Plan: `plaene/d3d12_umstieg.md`.

## Ergebnis in Kuerze

- **Der native Abgeber ist gebaut (11.11.).** Die Zeichenschichten (gpumap, kartegpu, Echtzeit-Licht, Model-Tab) gibt es
  einmal; ihre Zeichenbefehle gehen an `ID3D11Device`/`ID3D11DeviceContext`. Unter Direct3D 11 fuehrt sie die Laufzeit aus,
  unter Direct3D 12 der Abgeber (`gui/abgeber12_*.cpp`): er bedient dieselbe Schnittstelle und schreibt jeden Befehl als
  D3D12-Befehl in die eine Befehlsliste des Bildes. D3D11On12 ist ausgebaut. gpumap und kartegpu sind dafuer nicht geaendert.
- **Alle 3D-Ansichten laufen damit nativ** - ein ganzer `leistung`-Lauf (Karten, Szene, Karten-Editor mit Echtzeit-Licht,
  Model-Tab, Menue-Editor: 10,4 Millionen Zeichenbefehle, 113 Pipeline-Zustaende) ohne einen nicht umgesetzten Aufruf.
- **Bild:** Bildpunkt fuer Bildpunkt gleich mit Direct3D 11 sind die Oberflaeche (Modus `grafik`: Start, Menue, About und das
  feste Bild aller Mischarten), die 3D-Bilder der Vorschau (md_toaj_jedi, duel_mustafar, Szene 40 mit Figuren, Effekten und
  Gluehen), das Spielbild des Karten-Editors (1920 x 1440), der Model-Tab und der Texturbrowser. Drei Reste, jeder untersucht
  (Abschnitt "Fotounterschiede"): Mip-Stufen von Editor-Texturen mit ungerader Groesse (1416 von 1,6 Millionen Bildpunkten um
  1/255), duenne Linien auf Dreieckskanten im Echtzeit-Licht (1080 von 1,07 Millionen, hoechstens 11/255) und eine Lavakante in
  einem Kamerabild der Zeitleiste (7 Bildpunkte, hoechstens 2/255; D3D11On12 von Microsoft zeigt dort dasselbe wie der Abgeber).
- **Fotounterschiede im Menue-Editor (69 von 101, Stand 10.11.) sind Inhalt, nicht Zeichnen:** schon zwei Laeufe unter
  Direct3D 11 unterscheiden sich in denselben Fotos (laufende Videos, drehende Figuren, Shader mit Zeit, Uhrzeit, fps-Zahl).
- **Tempo:** Richtwert unter 15-21 Kernen Fremdlast (ein Lauf je Schnittstelle): keine Ansicht zeigt die Verlangsamung von D3D11On12
  (x2,3-5,6); Oberflaeche, Menue-Editor und Echtzeit-Licht sind auf dem Hauptfaden schneller, zeichenlastige Ansichten
  (Editor-Spielbild, Szene spielt) unter dieser Last +11 bis +76 % langsamer; Start und Laden kuerzer. Offen ist der
  Grafikspeicher (im Leerlauf 616 statt 67 MB, Karten-Editor mit Echtzeit-Licht 4784 statt 2507 MB).
- **Vorgabe bleibt Direct3D 11**, bis `messung_final.sh` im Ruhefenster gelaufen ist und shank umschaltet.

## Stufe 0: Messung

Selbsttest `leistung` (Abschnitte k s e m n, `BHED_GPU_ABSCHNITTE=1`, `BHED_D3D_DEBUG=0`, warmer Shader-Speicher), allein unter der
Messsperre 10.11. 11:07-11:21, keine fremden Bauten oder Selbsttests (Fremdlast laut GetSystemTimes 3,7-5,5 von 32 Kernen,
sichtbar VS Code/cpptools ~1,2). Median der Runden 1-3 (min..max); CPU = newFrame bis vor Present, je 240 Bilder.

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

Profil des Hauptfadens (`Y:\werkzeuge\dx12\profil12.py`, nur das Messfenster der 240 Bilder; Gegenprobe Stichproben gegen
Zyklenzaehler je Zeile in der Rohdatei):

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

## Der Abgeber: was gebaut ist

Begruendung des Schnitts und Aufbau im Einzelnen: `plaene/d3d12_umstieg.md` Abschnitt 3. Dateien:

- `gui/abgeber12.h` - die Naht: `rahmen::` (was der Abgeber vom Rahmen braucht: Geraet, Befehlsliste des Bildes, Vor-Liste,
  Hochladespeicher, Deskriptoren, spaetes Freigeben, Abschicken, Zaun) und `abgeber::` (anlegen/abbauen, Geraet und Kontext,
  Bruecke zur Oberflaeche, `ressource()` fuer DXR, Zaehler).
- `gui/abgeber12_intern.h` - die Objekte hinter `ID3D11*`: Puffer, Textur, Sichten (Lesen, Ziel, Tiefe), Shader, Eingabe,
  Misch-/Tiefen-/Raster-/Abtastzustand, Abfrage; Geraet (`ID3D11Device`) und Kontext (`ID3D11DeviceContext1`).
- `gui/abgeber12_geraet_win32.cpp` - anlegen: Texturen (auch Mehrfachabtastung, Wuerfel, Felder, Lesepuffer), Puffer
  (dynamisch = Bloecke im Hochladespeicher mit Groessenklassen; sonst Grafikspeicher), Sichten, Zustandsobjekte (gleiche
  Beschreibung = dasselbe Objekt), Wurzelsignatur, Deskriptorstapel, Zeitmarken.
- `gui/abgeber12_kontext_win32.cpp` - zeichnen: Bindungen merken, vor jedem Zeichenbefehl das Geaenderte schreiben
  (Pipeline-Zustand nach Schluessel, Root-CBV, Tabellen), Map/Unmap, UpdateSubresource, Kopieren, Aufloesen, Loeschen,
  GenerateMips, Abfragen. Nicht umgesetzte Methoden melden sich einmal im Protokoll (Fehler) und werden gezaehlt.
- `gui/backend_d3d12_win32.cpp` - der Rahmen: eine Befehlsliste je Bild (3D-Ansichten, dann Oberflaeche), Vor-Liste,
  Abschicken, spaetes Freigeben am Zaunwert; D3D11On12 und die Bruecke sind entfernt. `BHED_D3D12_FEHLER=on12` heisst jetzt
  `abgeber`.
- gpumap und kartegpu: keine Aenderung gegenueber dem Stand der Stufe 1 (dort zwei Stellen: `zeichneUeberlagerung` ueber
  `render::zeichneInD3d11Ziel`, `ohneMischen` ueber `render::zeichneOhneMischen`).
- Selbsttest: Modus `grafik` mit zwei festen Bildern - `grafik_mischarten` (alle 121 Faktorpaare von `zeichneStufe`,
  wiederholt und geklemmt, dazu `zeichneAdditiv` und `zeichneOhneMischen` ueber einem Verlauf) und `grafik_mipstufen` (alle
  Stufen von vier Editor-Texturen aus `GenerateMips`). Jeder Selbsttest endet unter Direct3D 12 mit einem Befund des Abgebers
  (`render::grafikBefund`): eine Pruefung mehr, FEHLER bei einem nicht umgesetzten Aufruf oder einem Fehler der Debugschicht.
- Handbuch und Tooltip (sechs Sprachen): Direct3D 12 zeichnet Oberflaeche und 3D-Ansichten.

Regeln von Direct3D 11, die beim Bauen aufgefallen sind (jede im Abgeber nachgebildet):

- imgui_impl_dx11 tastet mit MaxLOD 0 ab: die Oberflaeche zeigt ein Bild der 3D-Ansichten immer aus der obersten Stufe der
  Sicht. imgui_impl_dx12 tastet ueber alle Stufen ab - die Vorschaubilder im Texturbrowser waren deshalb anders (174 261
  Bildpunkte im Foto `leistung_editor`). Die Oberflaeche bekommt jetzt eine Sicht mit genau einer Stufe.
- Ein Lesepuffer mit `D32_FLOAT_S8X24_UINT` kommt unter Direct3D 11 als float + 32 Bit je Bildpunkt zurueck; Direct3D 12
  kopiert zwei Ebenen. `Map` setzt sie zusammen (Tiefe lesen fuer die Gizmo-Linien ohne Mehrfachabtastung).
- Puffer im Grafikspeicher fallen nach jedem `ExecuteCommandLists` auf COMMON zurueck; der Abgeber stellt sie je Ausfuehrung
  einmal ausdruecklich lesbar.

## Tests und Bildvergleich (11.11., Bau 47d493390 bis 881a492ed)

Gruppen, die gelaufen sind (die den geaenderten Code beruehren): Modus `grafik` je Schnittstelle und mit den fuenf gespielten
Fehlerstellen; Modus `leistung` (Abschnitte k s e m n, mit Fotos) je Schnittstelle; `menueeditor` je Schnittstelle am Stand der
Stufe 1 (fuer die Ursache der Fotounterschiede). **Bewusst nicht gelaufen** (Regel "Schlussrunde je Zweig", laufen auf dem
zusammengefuehrten Stand): sauberer Neubau, Modus 1, `menueeditor` und `karteneditor` am neuen Stand, `tabfenster`/`dock`
(eigene Fenster unter Direct3D 12), alle uebrigen Gruppen.

| Gruppe | Direct3D 11 | Direct3D 12 (Abgeber) | Fotos D3D11 gegen D3D12 |
|---|---|---|---|
| `grafik` (Bau 7f5c94c51) | 9 OK / 0 FEHLER | 10 OK / 0 FEHLER (eine Pruefung mehr: Befund des Abgebers) | 4 gleich (Start, Ansicht, About, Mischarten); `grafik_menue` 85 Bildpunkte = die Ziffer im Tooltip "laeuft jetzt: Direct3D 11/12"; `grafik_mipstufen` 97 Werte (ungerade Groessen, siehe unten) |
| `grafik` mit `BHED_D3D12_FEHLER=dll, geraet, swap, abgeber, imgui` (Bau 47d493390) | - | je Rueckfall auf Direct3D 11, 9 OK / 0 FEHLER | - |
| `leistung` k s (Vorschau: Karten, Szene; Bau 47d493390 bis 7f5c94c51) | laeuft durch | laeuft durch, Befund OK; mit Debugschicht 0 Fehler | `toaj`, `mustafar`, `szene40` (3D-Ziele 1350 x 931) gleich; `mission` (ganzes Fenster) 7 Bildpunkte, hoechstens 2/255 |
| `leistung` e (Karten-Editor, Echtzeit-Licht; Bau 1ded33fa3, mit Debugschicht 881a492ed) | laeuft durch | laeuft durch, Befund OK; mit Debugschicht 0 Fehler | `editor_vorlage_spiel` (1920 x 1440) gleich; `editor_vorlage` 1416 Bildpunkte, hoechstens 1/255; `editor_echtlicht` 1080 Bildpunkte, hoechstens 11/255; `editor` (ganzes Fenster): Texturbrowser gleich, sonst die Bildpunkte des Echtzeit-Lichts und zwei Statuszeilen (Inhalt) |
| `leistung` m n (Model-Tab, Menue-Editor) | laeuft durch | laeuft durch, Befund OK; mit Debugschicht 0 Fehler (Lauf e m n: 7 188 830 Zeichenbefehle, 89 Pipeline-Zustaende) | `modell` gleich; `menue` schwankt schon zwischen zwei Laeufen unter Direct3D 11 (Hintergrundvideo des Hauptmenues, 519 497 Bildpunkte) |
| ganzer `leistung`-Lauf ohne Fotos (k s e m n, Bau 7f5c94c51) | - | 1 OK / 0 FEHLER: 10 429 246 Zeichenbefehle, 113 Pipeline-Zustaende, 7 628 215 frische Bloecke, 0 nicht umgesetzte Aufrufe | - |
| `menueeditor` (Bau 018f3a988, D3D11On12) | 3 Laeufe, 2413-2643 OK / 0 FEHLER bis zur Zeitgrenze | 2 Laeufe, 2479 OK / 0 FEHLER bis zur Zeitgrenze | 89 gleich, 72 schwanken schon unter Direct3D 11, 4 zeitabhaengig (unten) |

Bau: inkrementell, 0 Warnungen (/W4 /WX). Lints: sources, fassung, d3dbesitz, settings, i18n ok; portability 99 (Basis 99).
Letzter Stand der Quellen (881a492ed): `grafik` Direct3D 11 9/0, Direct3D 12 10/0 mit Debugschicht, Rueckfall `abgeber` 9/0;
ganzer `leistung`-Lauf unter Direct3D 12 Befund OK (11 494 231 Zeichenbefehle, 113 Pipeline-Zustaende).

## Fotounterschiede: Ursachen

**Menue-Editor (Stand 10.11.: 32 gleich, 69 anders, 36 fehlen).** Kontrolle: drei Laeufe unter Direct3D 11 (me11, me11b, me11c)
und zwei unter Direct3D 12 (me12, me12b), alle am Bau 018f3a988, je bis zur Zeitgrenze von 60 min (2413-2643 Pruefungen, 0
FEHLER; die fehlenden Fotos waren die Zeitgrenze). Auswertung als Mengen (`vgl_menge.py`), 165 gemeinsame Fotos:

| Befund | Fotos |
|---|---|
| ein D3D12-Lauf ist Bildpunkt fuer Bildpunkt gleich einem D3D11-Lauf | 89 |
| die D3D11-Laeufe unterscheiden sich schon untereinander, D3D12 liegt nicht weiter weg | 72 |
| D3D12 weiter weg als die D3D11-Laeufe untereinander | 4: zwei laufende Videos (`menue_rahmen_video_laeuft_sp/_mp`), eine drehende Figur (`menue_modell_griffe`), `menue_ablegen` |

Was schwankt, ist Inhalt: die fps-Zahl und die Uhrzeit in der Statuszeile, das laufende Hintergrundvideo des Hauptmenues (ein
anderes Einzelbild je Lauf), Shader mit Zeit, drehende Figuren. Beispiel `menue_leiste_text`: me11 = me11c = me12 (0
Bildpunkte), me11b und me12b zeigen ein anderes Videobild. Die Mischarten der Oberflaeche, die nur der Menue-Editor benutzt,
prueft seitdem das feste Bild `grafik_mischarten`: Direct3D 11 = Direct3D 12.

**Kamerabilder der Zeitleiste (`leistung_mission`: 7 Bildpunkte, hoechstens 2/255).** Drei Laeufe unter Direct3D 11 sind
untereinander gleich, D3D11On12 (10.11.) und der native Abgeber (11.11.) sind untereinander gleich (0 Bildpunkte) - und
weichen beide in denselben 7 Bildpunkten von Direct3D 11 ab. Rohbilder der Kamerabilder (224 x 140, vor dem Skalieren in der
Oberflaeche): 59 von 65 gleich. In 5 der 6 anderen (bis 2421 Bildpunkte, Rauch und Lava) ist der EINGANG anders: das
Effektnetz haengt am Auge der Hauptansicht, und das stand in den beiden Laeufen in diesem Abschnitt des Tests woanders
(Pruefsumme des Effektnetzes je Kamerabild: 58 gleich, 7 anders, darunter diese fuenf). Uebrig bleibt ein Kamerabild (6 s) mit 14
Bildpunkten (hoechstens 3/255) an der Oberkante des Lavafalls bei gleichem Eingang - das sind die 7 des Fotos.

**Kanten im Echtzeit-Licht (`leistung_editor_echtlicht`: 1080 von 1,07 Millionen Bildpunkten, hoechstens 11/255).** Die Abweichungen
liegen als ein Bildpunkt breite Linien auf Dreieckskanten; 1048 der 1080 sind unter Direct3D 12 heller, im Median um 1/255
(90 % hoechstens 1/255) - eine Rundung an Kantenbildpunkten, kein anderer Inhalt. Probe: alle Shader mit `D3DCOMPILE_IEEE_STRICTNESS` uebersetzt - unter Direct3D 11
bleibt das Bild gleich, unter Direct3D 12 aendert es sich in einzelnen Bildpunkten (7 und 10 Bildpunkte um 1/255 in den
Szenenbildern, die ohne den Schalter gleich sind). Belegt ist damit: die beiden Treiber uebersetzen denselben DXBC nicht in
dieselbe Rechenfolge. Dass die Kanten daher kommen (ein Bildpunkt haengt dort an der letzten Stelle der Tiefe oder der Lage),
ist die naheliegende Erklaerung, aber nicht belegt. Der Schalter ist kein Mittel (er macht es nicht gleich) und ist nicht eingebaut.
eingegrenzt, nicht behoben. Mit `karte=CameraMSAA=0` (`CFG=abg2\cfg_msaa0.cfg`, Laeufe k11m/k12m) sind alle vier Karten-Editor-Fotos zwischen den Schnittstellen Bildpunkt fuer Bildpunkt gleich - auch `editor_vorlage`. Dessen 1416 Bildpunkte kommen also nicht von den Mip-Stufen des Treibers (so stand es nach der ersten Runde), sondern wie die des Echtzeit-Lichts von der Mehrfachabtastung der Kamera. Die abweichenden Bildpunkte liegen auf Dreieckskanten (`abg2/maske.py` malt sie); unter Direct3D 12 sind sie immer um genau 1/255 heller, in genau einem Kanal (Vorlage: alle 1416; Echtzeit-Licht: 1015 von 1048, der Rest bis 5 ist Laufstreuung - Direct3D 11 gegen Direct3D 11 schwankt dort um bis zu 11). Vier Kantenversuche im Modus `grafik` (voruebergehende Patches `abg2/tmp_kante*.py`, Auswertung `abg2/kante.py`: RGBA8 mit 8 Proben, 16 Dreiecke mit schraeger Kante, die acht Proben je Bildpunkt mit einem eigenen Shader ausgelesen) schliessen aus: die Abdeckung (0 von 1672 Kantenbildpunkten verschieden), die Rundung des Treibers beim Aufloesen (beide Schnittstellen runden halbe Werte ab), die Loeschfarbe (auch mit 0.118 und mit der Kamerafarbe 0.169 / 0.169 / 0.18 gleich; keine Probe ausserhalb n/255), zwei gezeichnete Farben an einer Kante und die Auswertung interpolierter Werte an Bildpunkten, deren Mitte das Dreieck nicht bedeckt - in allen vier Versuchen sind Proben und aufgeloestes Bild unter beiden Schnittstellen gleich. Die Ursache liegt damit in etwas, das nur die Kamera des Karten-Editors tut (das Spielbild mit derselben Mehrfachabtastung ist gleich): Tiefe und Stencil mit Proben (D24S8), `discard`, die Schnittebene (SV_ClipDistance), anisotrope Abtastung an Kantenbildpunkten oder das Mischen mehrerer Stufen. Der Abgeber gibt an diesen Stellen die Zustaende von Direct3D 11 unveraendert weiter (Linienregel, Probenmaske, Alpha-to-Coverage nachgesehen). Naechster Versuch: den Kantenversuch um je eine dieser Zutaten erweitern, bis die Bilder auseinanderlaufen.

**Mip-Stufen der Editor-Texturen (`leistung_editor_vorlage`: 1416 von 1,6 Millionen Bildpunkten, hoechstens 1/255).**
`ID3D11DeviceContext::GenerateMips` rechnet unter Direct3D 11 der Treiber (die Regel legt Direct3D nicht fest), unter
Direct3D 12 der Abgeber (je Stufe ein Viereck mit linearem Filter). Festes Bild `grafik_mipstufen`: Texturen mit geraden
Groessen (64 x 64, 256 x 16, 100 x 60 bis 25 x 15) sind in allen Stufen gleich; aus ungeraden Groessen (37 x 21, 25 x 15 ->
12 x 7) weichen 12 % der Werte um 1-2/255 ab - der D3D11-Treiber rechnet dort mit feineren Gewichten als der Texturfilter der
Grafikkarte (8 Bit).

## Richtwerte (unter Last)

Ein Messfenster am 11.11. 13:49-13:58 (`fenster.sh` + `richtwert.sh n2 ksemn`), derselbe Bau (1ded33fa3), je Schnittstelle EIN
Lauf des Selbsttests `leistung` (k s e m n, `BHED_D3D_DEBUG=0`, warmer Shader-Speicher). **Fremdlast im Fenster: 20,9 Kerne
(Direct3D 11) bzw. 15,5 Kerne (Direct3D 12) von 32 - sechs Agenten, 10-16 fremde jkstudio.** Die Bildzeiten liegen damit drei-
bis sechsmal ueber denen der Stufe 0 (ruhiger Rechner) und streuen in beide Richtungen; sie taugen nur fuer die Frage, ob der
Abgeber in der Groessenordnung von Direct3D 11 liegt oder wie D3D11On12 (dort Editor-Spielbild x2,3-5,6). Rohdaten
`laeufe\m_n2_11_1`, `m_n2_12_1`, Tabelle `python -I ausw12.py n2_11 n2_12 RUNDEN=1 TEILE=1`.

| Ansicht (CPU Mittel ms, Hauptfaden newFrame bis vor Present) | Direct3D 11 | Direct3D 12 (Abgeber) |
|---|---|---|
| Mission mp/duel1 ruhend | 2,08 | 1,29 |
| Mission md_toaj_jedi ruhend / Flug | 2,52 / 10,96 | 2,41 / 7,03 |
| Mission Szene angehalten / spielt | 4,05 / 16,97 | 5,07 / 19,88 |
| Mission duel_mustafar ruhend | 3,58 | 5,09 |
| Map Editor Vorlage wie im Spiel ruhend / Kamera dreht | 11,20 / 7,92 | 11,78 / 13,95 |
| Map Editor duel_yavin ohne / mit Halmen | 10,31 / 9,78 | 11,46 / 12,31 |
| Map Editor md_am_sith ohne / mit Effekten | 7,29 / 12,16 | 9,74 / 10,97 |
| Map Editor Echtzeit-Licht ruhend / Kamera dreht | 34,43 / 26,59 | 24,60 / 20,27 |
| Model stormtrooper ruhend / Animation | 1,61 / 1,80 | 1,69 / 1,86 |
| Menu Editor ruhend / Maus bewegt | 1,64 / 1,53 | 1,35 / 1,24 |

| Laden und Speicher | Direct3D 11 | Direct3D 12 (Abgeber) |
|---|---|---|
| Programmstart bis erstes Bild | 2035 ms | 1226 ms |
| Karte laden, erstes fertiges Bild: duel1 / md_toaj_jedi / md_am_sith / duel_mustafar | 3695 / 9279 / 9192 / 5443 ms | 2248 / 7175 / 8509 / 6073 ms |
| Szene aufbauen md_am/intro2_sith | 7505 ms | 5950 ms |
| Karten-Editor oeffnen (Taris, .map) | 19 971 ms | 16 184 ms |
| Speicher Start: privat / Grafikkarte | 296 / 52 MB | 316 / 104 MB |
| Speicher nach md_toaj_jedi: privat / Grafikkarte | 1924 / 497 MB | 2201 / 616 MB |
| im Leerlauf danach: Grafikkarte | 67 MB | 616 MB |
| Speicher nach der .map (Echtzeit-Licht): privat / Grafikkarte | 7478 / 2507 MB | 7900 / 4784 MB |
| Spitze Arbeitssatz des Laufs | 5278 MB | 5812 MB; nach den Aenderungen von WIP 13 (Lauf m_n3_12_1): 3874 MB |

Lesart:

- **Keine Ansicht zeigt die Verlangsamung von D3D11On12** (dort x2,3-5,6 im Editor-Spielbild). Der Hauptfaden ist in 8 von 18
  Ansichten schneller (Oberflaeche, Menue-Editor, Echtzeit-Licht, Flug) und in 10 langsamer: drei um hoechstens 5 %, die
  zeichenlastigen deutlicher (Editor-Spielbild +11 bis +76 %, Szene spielt +17 %, duel_mustafar +42 %) - obwohl der D3D12-Lauf
  weniger Fremdlast hatte. Das passt zur Erwartung aus Stufe 0: unter Direct3D 11 arbeitet der Treiber die Zeichenbefehle auf
  einem eigenen Faden ab, der Abgeber uebersetzt auf dem zeichnenden Faden. Wie gross der Abstand ohne Last ist (Stufe 0:
  Editor-Spielbild 2,9 ms bei 4,17 ms je Bild), sagt erst das Ruhefenster; die Stelle fuer einen zweiten Faden ist vorbereitet
  (Plan Abschnitt 3, "Mehrkern spaeter").
- **Laden und Start** sind nicht langsamer (9 von 10 Werten kuerzer); die Pipeline-Zustaende (113 im ganzen Lauf) kosten beim
  ersten Gebrauch nicht sichtbar.
- **Grafikspeicher ist der offene Punkt:** (a) im Leerlauf gibt der D3D11-Treiber den Grafikspeicher ungenutzter Texturen frei
  (67 MB), unter Direct3D 12 bleibt alles belegt (616 MB) - dafuer muesste der Abgeber im Leerlauf `Evict` rufen und vor dem
  naechsten Gebrauch `MakeResident`; (b) im Karten-Editor mit Echtzeit-Licht 4784 statt 2507 MB - dieselbe Zahl wie unter
  D3D11On12 (4850 MB): Direct3D 12 belegt jede Textur beim Anlegen ganz, der D3D11-Treiber offenbar erst beschriebene Teile
  (Verdacht: die beiden Schattenkarten-Felder aus `kartenAnlegen`, R32_FLOAT, Groesse x Groesse x Schichten; nicht gemessen);
  (c) beim Start 52 MB mehr.
- Die GPU-Zeit je Bild ist zwischen den Schnittstellen nicht vergleichbar (Direct3D 12 schickt ein Bild am Stueck ab, der
  D3D11-Treiber in Teilen; siehe backend_win32.cpp bei markenGeraet).

## Abweichungen und Offenes

- **Fuer shank zu entscheiden (Regel "Bild bitgleich, Unterschiede der Hardware belegen und klaeren"):**
  1. Mip-Stufen aus ungeraden Groessen (oben): gleich wird es nur, wenn der Karten-Editor seine Stufen selbst rechnet statt
     `GenerateMips` zu rufen (dann aendert sich auch das Bild unter Direct3D 11 um hoechstens 2/255 in verkleinerten
     Texturen mit ungerader Groesse) - Vorschlag: so machen, die Regel des Treibers ist ohnehin je Hersteller anders.
  2. Lavakante im Kamerabild (7 Bildpunkte): D3D11On12 von Microsoft zeigt Bildpunkt fuer Bildpunkt dasselbe wie der
     Abgeber - ein Unterschied des Treibers, nicht des Programms.
  3. Linien auf Dreieckskanten im Echtzeit-Licht (1080 Bildpunkte, hoechstens 11/255): Ursache nicht belegt (naheliegend
     dieselbe; ein Vergleich mit D3D11On12 liegt fuer dieses Bild nicht vor). Entweder weiter eingrenzen (welcher Durchgang:
     Schattenkarten, Licht, Mehrfachabtastung) oder hinnehmen.
- **Messung im Ruhefenster fehlt** (`sh /y/werkzeuge/dx12/messung_final.sh`, ~27 min; nimmt den Bau aus
  `Y:\p_dx12\build\bin\Release`). Die Richtwerte oben entstanden bei 15-21 von 32 Kernen Fremdlast.
- **Grafikspeicher** (Richtwerte): im Leerlauf und im Karten-Editor mit Echtzeit-Licht belegt Direct3D 12 deutlich mehr - vor
  dem Umschalten der Vorgabe zu loesen (im Leerlauf Evict/MakeResident; Schattenkarten-Felder nach Bedarf oder als reservierte
  Ressource). Ebenso der Hauptfaden in zeichenlastigen Ansichten, falls das Ruhefenster den Abstand bestaetigt (zweiter Faden).
- **Nicht gelaufen unter Direct3D 12:** Modus 1, `menueeditor`, `karteneditor`, `tabfenster`/`dock` am neuen Stand;
  Geraeteverlust. (Die Debugschicht von Direct3D 12 meldet ueber den `leistung`-Lauf - Vorschau, Karten-Editor mit
  Echtzeit-Licht, Model-Tab, Menue-Editor - keinen Fehler; einzige Warnung: ein Ziel wird mit einer anderen Farbe geloescht
  als beim Anlegen genannt, das kostet nur den schnellen Loeschweg.)
- **ctest:** alle Ziele gebaut (0 Warnungen), `ctest -C Release -j 12`: 245 von 246 bestanden. Rot ist `q3map2_teilbau` (2 Fehlschlaege) - der
  Test gehoert zum Licht-Cache (LCB2) und steht an der Basis dieses Zweigs als angefangene Arbeit (a00f6cf90 "WIP lcb2 1",
  acaa65dde "red: 22 cases"); dieser Zweig aendert weder `src/q3map2` noch `tools/karte`. An der Basis selbst nicht nachgestellt.
- Das Menue Ansicht ist eine Zeile laenger: die Handbuchbilder mit offenem Ansicht-Menue (s_aend_gleich, o_theme, o_sprache,
  o_groesse, mp_an, mp_aus; sechs Sprachen) sind in der Release-Runde neu aufzunehmen.
- Nebenbefund (nicht dieser Zweig): der Menue-Editor baut je Bild die Dateiliste aller Archive neu (`lesbarkeitVon` ->
  `umgebung` -> `FeederDaten::baue` -> `dateiListe`, tolower je Name) - unter Last stand ein Lauf minutenlang in diesem Stapel.
- Nach `python tools/gen_i18n.py` baut das ganze Programm neu (i18n.h bekommt ein neues Datum, auch bei gleichem Inhalt).

## Zweite Runde (11.11. nachmittags): warum der native Weg langsamer war, und was daran getan ist

Auftrag nach dem Ruhefenster von 14:47 (`messung_final.txt`: Karten-Editor Bildabstand 6,3-7,5 statt 4,17 ms, GPU +16 bis +126 %,
Hauptfaden +16 bis +61 %, Grafikspeicher mit Echtzeit-Licht +96 %). shank 11.11.: Direct3D 12 soll Vorgabe werden - "schaust du
deshalb wieso es langsamer ist?". Ursachen zuerst, jede mit Zaehler, Zeitmarke der Grafikkarte oder Profil belegt. Alle Zahlen
dieser Runde sind **Richtwerte** aus Messfenstern von hoechstens 10 Minuten neben anderen Agenten (Fremdlast je Fenster genannt);
die Entscheidung braucht `messung_final.sh` im Ruhefenster. Werkzeuge und Rohdaten: `Y:\werkzeuge\dx12\abg2\`.

### War die Messung unfair gegen Direct3D 12?

| Frage | Befund |
|---|---|
| Zeitmarken der Grafikkarte verschieden gesetzt? | Ja, in einem Punkt: die Endmarke der Bildmessung stand unter Direct3D 12 in einer eigenen Befehlsliste - ein zweites `ExecuteCommandLists` je Bild, nur beim Messen, nur unter Direct3D 12 (ein Kernaufruf auf dem Hauptfaden). **Behoben:** die Endmarke steht in der Befehlsliste des Bildes, hinter dem Hauptfenster wie unter Direct3D 11. Anfangsmarke bei beiden am Bildanfang. |
| Debugschicht im gemessenen Lauf an? | Nein, bei beiden aus (`BHED_D3D_DEBUG=0`, im Protokoll keine Zeile "Debugschicht AN"). |
| Swapchain, Present, Bildgrenze verschieden? | Nein: beide Flip-Modell (FLIP_DISCARD), drei Puffer, wartbar mit hoechstens zwei Bildern Vorlauf, `Present(1, 0)`, dasselbe Warten vor dem Bild. |
| Shader-Speicher nur auf einer Seite warm? | Nein: der Shader-Speicher des Programms ist fuer beide vorbelegt, der Anlauf-Lauf waermt den Treiber fuer Direct3D 12, alle Laeufe wechseln ab. |
| Speicherzahl "nach der .map" | Zeigt einen Zeitpunkt, nicht den Bedarf: der D3D11-Treiber belegt den Atlas der Schattenwuerfel (1788 MB) erst, soweit er beschrieben ist - in zehn Laeufen an derselben Stelle 2208 bis 4508 MB; Direct3D 12 belegte ihn beim Anlegen ganz (4563-4784 MB). Der Selbsttest meldet jetzt zusaetzlich "Speicher mit Echtzeit-Licht" (nach den Messschritten). |
| eigener PC-Gebrauch von shank | Nein: die Runden wechselten ab, die Spanne war eng (GPU 2,815..2,829 gegen 5,858..5,982 ms). |

Die Messung war also bis auf die Endmarke fair - die Verlangsamung war echt und hatte benennbare Ursachen.

### Ursachen, Belege, Behebung

| # | Ursache | Beleg | Behebung |
|---|---|---|---|
| 1 | **Grafikkarte: die 2D-Ansicht des Karten-Editors zeichnete ihren Linienpuffer jedes Bild aus dem Arbeitsspeicher.** Der Puffer (dynamisch, 42 MB auf der Taris-Karte) wird einmal gefuellt und danach jedes Bild gezeichnet; unter Direct3D 12 lag er im Hochladespeicher, die Grafikkarte holte ihn je Bild ueber den Bus. Der D3D11-Treiber haelt so etwas im Grafikspeicher. | Bildprotokoll: 4 Zeichenbefehle in das Ziel der 2D-Ansicht = 4,7 von 7,9 ms des Bildes. Dieselben Zeitmarken unter beiden Schnittstellen um `kartegpu::beginnen/beenden` (Fenster 15:37, Fremdlast 8-15 Kerne): Ziel 100 unter Direct3D 11 0,18 ms, unter Direct3D 12 3,1-4,8 ms (Vorlage), 0,11 gegen 0,65-0,78 ms (duel_yavin), 0,16-0,21 gegen 0,82-1,16 ms (md_am_sith). | Ruhende dynamische Eck-/Indexpuffer ab 64 KB (zwei Bilder ohne `Map`) bekommen eine Kopie im Grafikspeicher. Danach (Fenster 15:55): 0,19 / 0,13 / 0,17 ms - wie Direct3D 11. |
| 2 | **Hauptfaden: Zuruecklesen aus einem abgebildeten Konstantenpuffer.** `gpumap_win32.cpp:8926` kopiert nach dem Fuellen von `g_cbBild` den Inhalt zurueck (`memcpy(g_bildDaten, f, ...)`). Der Hochlade-Heap ist schreibkombiniert - Lesen daraus geht am Zwischenspeicher des Prozessors vorbei. Unter Direct3D 11 gibt der Treiber gewoehnlichen Speicher heraus. | Profil mit Quellzeilen (15:23): diese eine Zeile 19,5 % des Hauptfadens im Karten-Editor (duel_yavin), unter Direct3D 11 nicht messbar. Profil im Ruhefenster: `zeichneKarte` selbst 28,5 % = 1,14 ms je Bild gegen 0,06 ms. | Bloecke der dynamischen Puffer liegen in einem eigenen Heap (CUSTOM, L0, WRITE_BACK): gewoehnlicher Speicher mit Zwischenspeicher, von der Grafikkarte lesbar. Die Zeile fehlt danach im Profil. |
| 3 | **Hauptfaden: Abschicken und Zeigen.** `ExecuteCommandLists` und `Present` sind je ein Kernaufruf; unter Direct3D 11 macht sie der Faden des Treibers. | Profil im Ruhefenster: Present 0,11 ms, ExecuteCommandLists 0,11 ms je Bild (duel_yavin; unter Direct3D 11 Present 0,01 ms auf dem Hauptfaden). | Abschickfaden: ExecuteCommandLists, Present und Signal laufen in Auftragsreihenfolge auf einem eigenen Faden; geschlossene Listen kommen aus einem Vorrat. `BHED_D3D12_ABSCHICKEN=direkt` schaltet ihn ab. |
| 4 | **Hauptfaden: Zustandswechsel von Puffern.** Jeder Puffer im Grafikspeicher wurde je Ausfuehrung ausdruecklich lesbar gestellt. | Zaehler je Bild: 339 Zustandswechsel (Vorlage), 232 (duel_yavin); `ResourceBarrier` 2,3-3,5 % des Hauptfadens. | Direct3D 12 stellt Puffer aus COMMON beim ersten lesenden Gebrauch von selbst lesbar - ausdruecklich nur noch nach dem Beschreiben und bei Puffern mit mehreren Lesearten. Danach 36 und 60 je Bild. Wechsel von Texturen gehen gebuendelt in einen Aufruf. |
| 5 | **Hauptfaden: Kleinkram je Bild.** Kernaufrufe fuer die Frequenz der Zeitmarken (je `GetData`) und fuer "Geraet verloren" (je Frage nach einem Zaunwert); je `Map(WRITE_DISCARD)` (rund 3300 je Bild) 44 Vergleiche ueber alle Bindungsplaetze und zweimal die Suche nach der Groessenklasse; Schluessel der Pipeline-Zustaende Byte fuer Byte gestreut (2,5 % des Hauptfadens bei 1143 Wechseln je Bild); Anfang des Deskriptorstapels je gebundener Textur neu gefragt; die Zeichenschicht legt 0,4 Puffer je Bild neu an (je ein `CreateCommittedResource`). | Profil (Methoden und Aufrufstellen), Zaehler des Bildprotokolls. | Frequenz und Verluststand gemerkt; der Puffer merkt sich seine Bindungsplaetze; der Block traegt seine Klasse; Schluessel in 64-Bit-Schritten; Stapelanfang gemerkt; freigegebene Puffer im Grafikspeicher werden wiederverwendet. |
| 6 | **Grafikkarte: Kopieren und Aufloesen von Zielen mit Mehrfachabtastung.** `CopyResource` zwischen zwei 8-fach abgetasteten Zielen (1920 x 1440) kostet unter Direct3D 12 0,29-0,32 ms, unter Direct3D 11 0,19 ms; `ResolveSubresource` 0,08-0,10 gegen 0,063 ms. Das Spielbild des Karten-Editors kopierte bis zu dreimal und loest siebenmal je Bild auf: +0,40 ms (duel_yavin 1,45 gegen 1,05 ms, md_am_sith 1,44 gegen 1,03 ms). | Zeitmarken je Vorgang unter beiden Schnittstellen (Fenster 16:30, Fremdlast 3 Kerne). Das war der ganze Rest des Abstands in der GPU-Zeit (duel_yavin 4,10 gegen 3,65 ms). | **Zwei von drei Kopien entfallen, unter beiden Schnittstellen:** `gpu::zeichneGluehen` legte das Gluehen ueber einen zweiten Vollpuffer auf und kopierte zurueck. Mit Mehrfachabtastung liest der Durchgang ohnehin die aufgeloeste Szene - er zeichnet jetzt gleich ins Ziel (ein Vollbilddreieck ohne Mischen und ohne Tiefe schreibt jede Probe; die Fotos `mustafar`, `szene40` und das 3D-Bild von `mission` sind unter beiden Schnittstellen unveraendert). Bleibt je Bild eine Kopie (Portalbild in die Hauptszene) und sieben Aufloesungen, die der D3D12-Treiber langsamer ausfuehrt: rund +0,2 ms auf Karten mit Himmelsportal. **Sieben Versuche am Abgeber aenderten daran nichts:** Loeschwert wie die Loeschfarbe; Ziel des Aufloesens als Renderziel angelegt; `ResolveSubresourceRegion`; `DiscardResource` vor dem Kopieren; Kopieren als `ResolveSubresourceRegion(DECOMPRESS)`; Zustandswechsel gebuendelt; Farbziele im neuen Zustandsmodell (Enhanced Barriers mit `DIRECT_QUEUE_COPY_SOURCE`/`_DEST`, Fenster 19:07: Kopie 0,34-0,40 statt 0,31-0,32 ms - nicht besser). |

### Grafikspeicher und Arbeitssatz

| Stelle | Ursache | Beleg | Stand |
|---|---|---|---|
| Karten-Editor mit Echtzeit-Licht: 4784 statt 2445 MB | **Zwei Ursachen.** (a) Der Atlas der Schattenwuerfel (3072 x 2048 x 142 Schichten, R16_FLOAT = 1788 MB) war unter Direct3D 12 beim Anlegen ganz belegt. (b) Der groessere Teil: unter Direct3D 12 bleibt alles, was lebt, im Grafikspeicher - der D3D11-Treiber haelt nur, was gebraucht wird. Im Karten-Editor leben auch die Zielsaetze des Spielbilds (je 88 MB Farbe + 176 MB Tiefe mit 8 Proben) und die Texturen der anderen Ansicht, ohne gezeichnet zu werden. | (a) Speicherbericht des Abgebers (groesste Textur 1788 MB). (b) Nach (a) allein: nach dem Oeffnen der .map 2904 gegen 2269 MB - aber sobald alle Schattenwuerfel gerechnet sind, wieder 4595 gegen 2075 MB (Laeufe e12v/e11v, 18:29), obwohl unter Direct3D 11 derselbe Atlas ganz beschrieben ist und dieselben Objekte leben (Speicherbericht ohne Atlas: 2688 MB). Die Zahl von Direct3D 11 waechst zwischen "nach der .map" und dem Ende nicht (Ruhefenster: 2445 -> 2439 MB). | (a) Grosse, leer angelegte Texturfelder (ab 4 Schichten und 32 MB) sind reservierte Ressourcen; eine Schicht bekommt ihren Speicher beim ersten Schreiben (`BHED_D3D12_DUENN=0` schaltet es ab). (b) **Auslagern:** Texturen ab 1 MB, die seit 4 Sekunden und 8 Bildern in keiner Befehlsliste vorkamen, geben ihren Grafikspeicher ab (`ID3D12Device::Evict`), das am laengsten Ungebrauchte zuerst; vor dem naechsten Gebrauch holen sie ihn zurueck (`MakeResident`). Der Inhalt bleibt erhalten. Direct3D 11 belegte im Karten-Editor in sechs von sieben Laeufen dieses Tages 2,1-2,4 GB (einmal 4731 MB), auch im Ruhefenster von 14:47 - das Budget, das das System dem Prozess nennt, stand dabei unveraendert bei 7180 MB. Ein Abgeber, der nur dem Budget folgt, behielte alles (Messfenster 21:50: 4881 gegen 2300 MB). Deshalb nach Zeit. `BHED_D3D12_AUSLAGERN=0`: nie; `=budget`: nur, was ueber dem Budget des Systems liegt (`QueryVideoMemoryInfo`; unter Volllast mit vier Laeufen sank es auf 1486-1884 MB, und der Abgeber folgte); `BHED_D3D12_BUDGET_MB=<n>`: ein Budget annehmen (Selbsttest). |
| Start: 104 statt 52 MB | Die Swapchain: der belegte Grafikspeicher steigt beim Anlegen der Swapchain von 11,9 auf 101,5 MB (drei Puffer, Flip-Modell; das Fenster des Selbsttests ist 1920 x 1440). Der Abgeber selbst belegt beim Start 0,1 MB. | Stationen in `d3d12::init` (voruebergehende Messhilfe `abg2/tmp_vram.py`). | Nicht unser Speicher (DXGI/Treiber). Offen: weniger Puffer waere eine andere Swapchain als unter Direct3D 11. |
| "Speicher Ende" | Freigegebene Texturen gingen erst am Ende des naechsten Bildes zurueck. | - | Sie gehen am Anfang des naechsten Bildes zurueck. |
| Arbeitssatz +15 bis +41 % | Der Arbeitssatz enthaelt, was die Speicherverwaltung der Grafikkarte gerade im Arbeitsspeicher haelt: laufen zwei Prozesse mit je 4,5 GB Grafikspeicher zugleich, verdraengt sie beim D3D11-Prozess 2 GB aus dem Grafikspeicher (Grafikkarte 2279 MB, Arbeitssatz 3783 MB) - beim D3D12-Prozess lag der ganze Atlas (1788 MB) im Arbeitssatz. Dazu eigene Anteile: Bloecke (95-143 MB, davon 53-82 MB frei), ein freier 42-MB-Block des Linienpuffers, Hochladepuffer der Bildplaetze (12-87 MB); beim Start +59 MB (Laufzeit und Treiber eines D3D12-Geraets, Swapchain). | Speicherkarte: Bereiche nach Art, Groesse und Anteil im Arbeitssatz. | Freie grosse Bloecke gehen nach 600 Bildern ohne Bedarf zurueck; mit dem duennen Atlas entfaellt dessen Anteil. Rest offen (Ruhefenster). |

### Richtwerte vorher und nachher

Alle Zahlen sind Richtwerte aus Messfenstern neben anderen Agenten. Brauchbar sind: die Profile von 16:54 (keine fremden Bau-
und Testprozesse), die GPU-Zeiten je Ansicht und je Vorgang (16:30, 19:07) und das Laufpaar von 22:08 (Endstand; Direct3D 11
bei 6, Direct3D 12 bei 5 Kernen Fremdlast). Die Fenster von 20:40 und 21:50 liefen bei 9-28 Kernen Fremdlast gegen 5-13 fuer
den Lauf unter Direct3D 11 - ihre Bildzeiten taugen nicht, ihre Speicherzahlen schon. Das Fenster von 16:11 war gestoert.

**Hauptfaden, ms je Bild (Profil, Fenster 16:54; in Klammern das Ruhefenster von 14:47 vor dieser Runde), Direct3D 11 -> 12:**

| Ansicht | vor der Runde | 16:54 |
|---|---|---|
| Karten-Editor duel_yavin | 2,44 -> 3,99 | 2,73 -> 2,63 |
| Karten-Editor md_am_sith | 2,33 -> 2,99 | 2,36 -> 2,16 |
| Karten-Editor Vorlage | 2,85 -> 3,55 | 3,45 -> 3,21 |
| Mission Szene spielt | 4,14 -> 4,76 | 5,21 -> 5,14 |
| Mission Szene angehalten | 1,18 -> 1,30 | 1,42 -> 1,16 |

Darunter: Direct3D 11 mit Laufzeit und Treiber auf dem Hauptfaden 0,52 / 0,60 / 1,03 / 1,04 ms, der Abgeber mit allem darunter
0,47 / 0,57 / 1,04 / 0,70 ms; der Treiberfaden von Direct3D 11 1,8-2,75 ms gegen den Abschickfaden 0,27-0,41 ms.

**GPU, ms je Bild, Direct3D 11 -> 12:**

| Ansicht | vor der Runde (14:47) | 16:30 | Endstand 22:08 |
|---|---|---|---|
| Karten-Editor Vorlage wie im Spiel, ruhend | 2,82 -> 5,95 | 2,87 -> 2,99 | 2,86 -> 2,66 |
| Karten-Editor duel_yavin ohne Halme | 3,64 -> 4,22 | 3,65 -> 4,10 | 3,51 -> 3,52 |
| Karten-Editor md_am_sith ohne Effekte | 3,30 -> 4,85 | 3,28 -> 3,77 | 3,03 -> 3,14 |
| Karten-Editor Echtzeit-Licht ruhend / Kamera dreht | - | - | 22,2 -> 22,7 / 11,6 -> 12,0 |

**Endstand, Laufpaar von 22:08 (je EIN Lauf; der erste Direct3D-12-Lauf desselben Fensters war bei 12 Kernen Fremdlast auf dem
Hauptfaden bis doppelt so langsam - so gross ist die Streuung), Direct3D 11 -> 12:**

| Ansicht | Hauptfaden ms | GPU ms | Bildabstand ms |
|---|---|---|---|
| Mission mp/duel1 ruhend | 0,38 -> 0,53 | 0,16 -> 0,20 | 4,17 -> 4,16 |
| Mission md_toaj_jedi ruhend / Flug | 0,62 -> 0,74 / 3,96 -> 4,11 | 0,18 -> 0,20 / 1,80 -> 1,64 | 4,17 -> 4,18 / 5,91 -> 5,84 |
| Mission Szene angehalten / spielt | 1,24 -> 1,38 / 4,77 -> 6,09 | 0,44 -> 0,36 / 2,74 -> 1,76 | 4,74 -> 4,17 / 5,09 -> 6,58 |
| Mission duel_mustafar ruhend | 1,39 -> 1,72 | 0,37 -> 0,45 | 4,17 -> 4,34 |
| Karten-Editor Vorlage ruhend / dreht | 4,32 -> 4,55 / 4,22 -> 4,86 | 2,86 -> 2,66 / 2,53 -> 2,22 | 4,83 -> 5,63 / 4,82 -> 5,74 |
| Karten-Editor duel_yavin ohne / mit Halmen | 5,01 -> 4,24 / 4,99 -> 3,44 | 3,51 -> 3,52 / 3,71 -> 3,80 | 5,57 -> 6,27 / 5,54 -> 6,09 |
| Karten-Editor md_am_sith ohne / mit Effekten | 4,68 -> 2,37 / 7,97 -> 4,34 | 3,03 -> 3,14 / 3,66 -> 3,24 | 5,51 -> 4,43 / 8,41 -> 6,07 |
| Karten-Editor Echtzeit-Licht ruhend / dreht | 7,98 -> 5,65 / 7,96 -> 4,34 | 22,2 -> 22,7 / 11,6 -> 12,0 | 22,3 -> 22,7 / 13,4 -> 12,4 |
| Model ruhend / Animation | 1,03 -> 0,60 / 1,02 -> 1,24 | 0,41 -> 0,40 / 0,45 -> 0,48 | 4,17 -> 4,16 / 4,17 -> 4,31 |
| Menue-Editor ruhend / Maus | 0,70 -> 0,80 / 0,70 -> 0,77 | 0,40 -> 0,37 / 0,41 -> 0,37 | 4,20 -> 4,17 / 4,17 -> 4,16 |

Was danach noch langsamer aussieht: der Hauptfaden im Mission-Tab (+4 bis +40 %, Szene spielt +28 %) - unter Last nicht zu
entscheiden, erste Frage an das Ruhefenster; GPU auf Karten mit Himmelsportal +0,4 bis +3,5 % (Ursache 6); der Bildabstand im
Karten-Editor lag in diesem Fenster bei beiden ueber der Grenze.

**Speicher, Grafikspeicher / Arbeitssatz in MB:**

| Stelle | Direct3D 11 | Direct3D 12 vor der Runde (14:47) | Direct3D 12 Endstand (22:08) |
|---|---|---|---|
| Start | 52 / 231 | 104 / 290 | 104 / 289 |
| nach maps/md_am_sith.bsp | 782 / 1571 | 667 / 2005 | 708 / 1992 |
| nach der Szene | 790 / 2622 | 674 / 3036 | 716 / 2949 |
| nach md_toaj_jedi (Szene steht) | 563 / 1264 | 616 / 1461 | 159 / 1465 |
| Karten-Editor nach der .map | 2447 / 2539 | 4784 / 3601 | 3777 / 3613 (20:40: 2772 / 3525) |
| Karten-Editor mit Echtzeit-Licht | 2446 / 2474 | (4784) | 2603 / 3250 |
| Ende | 2442 / 2600 | 4780 / 3342 | 419 / 3298 |

Die Zahl von Direct3D 11 im Karten-Editor ist nicht fest: 2,1-2,4 GB in sechs von sieben Laeufen des Tages (auch im
Ruhefenster), einmal 4731 MB (20:40). Ohne Auslagern belegt Direct3D 12 dort 4,6-4,9 GB (21:50: 4881 gegen 2300 MB). "Nach der
.map" haengt am Augenblick: waehrend des Ladens laufen keine Bilder, und was kurz davor gebraucht wurde, geht erst vier Sekunden
spaeter (Abhilfe geschrieben, nicht angewandt: `abg2/p19_zaun.py`, Auslagern nach dem Zaunwert). Offen: der Arbeitssatz liegt in
allen Ansichten ueber Direct3D 11 (+12 bis +42 %), der Start bei +52 MB Grafikspeicher (Swapchain).

### Bild

Unveraendert gegenueber dem Stand vom Mittag (jede Aenderung dieser Runde gegen dieselben Fotos verglichen): Oberflaeche, `toaj`,
`mustafar`, `szene40`, `modell`, `editor_vorlage_spiel` Bildpunkt fuer Bildpunkt gleich mit Direct3D 11; `editor_vorlage` 1416
Bildpunkte um 1/255, `editor_echtlicht` 1048-1089 Bildpunkte, `mission` 7 Bildpunkte.

- **Mip-Stufen wie das Spiel, hinter einem Schalter** (Vorgabe unveraendert): `kartegpu::mipsWieSpiel` - Editor-Einstellung
  `BhedMipsLikeGame` (in `behaved.cfg`: `karte=BhedMipsLikeGame=1`) oder `BHED_KARTE_MIPS=spiel`. Dann rechnet der Editor alle Stufen
  selbst (`TextureSet::Tex::buildMips`, R_MipMap aus tr_image.cpp:466) statt `GenerateMips`. Selbsttest `grafik`: jede Stufe von vier
  Testtexturen (auch 37 x 21 und 100 x 60) ist Byte fuer Byte die von R_MipMap - unter beiden Schnittstellen 0 abweichende Bytes; das
  Foto `grafik_mipstufen_spiel` ist unter Direct3D 11 und 12 gleich. Die Pruefung war einmal rot (mit `BHED_KARTE_MIPS=treiber`
  erzwungen: Mit `BHED_KARTE_MIPS=treiber` erzwungen: 1 FEHLER unter Direct3D 12 (10 OK) und 1 FEHLER unter Direct3D 11 (9 OK), sonst 11/0 und 10/0).
- **Kanten im Echtzeit-Licht:** eingegrenzt, nicht behoben. Mit `karte=CameraMSAA=0` (`CFG=abg2\cfg_msaa0.cfg`, Laeufe k11m/k12m) sind alle vier Karten-Editor-Fotos zwischen den Schnittstellen Bildpunkt fuer Bildpunkt gleich - auch `editor_vorlage`. Dessen 1416 Bildpunkte kommen also nicht von den Mip-Stufen des Treibers (so stand es nach der ersten Runde), sondern wie die des Echtzeit-Lichts von der Mehrfachabtastung der Kamera. Die abweichenden Bildpunkte liegen auf Dreieckskanten (`abg2/maske.py` malt sie); unter Direct3D 12 sind sie immer um genau 1/255 heller, in genau einem Kanal (Vorlage: alle 1416; Echtzeit-Licht: 1015 von 1048, der Rest bis 5 ist Laufstreuung - Direct3D 11 gegen Direct3D 11 schwankt dort um bis zu 11). Vier Kantenversuche im Modus `grafik` (voruebergehende Patches `abg2/tmp_kante*.py`, Auswertung `abg2/kante.py`: RGBA8 mit 8 Proben, 16 Dreiecke mit schraeger Kante, die acht Proben je Bildpunkt mit einem eigenen Shader ausgelesen) schliessen aus: die Abdeckung (0 von 1672 Kantenbildpunkten verschieden), die Rundung des Treibers beim Aufloesen (beide Schnittstellen runden halbe Werte ab), die Loeschfarbe (auch mit 0.118 und mit der Kamerafarbe 0.169 / 0.169 / 0.18 gleich; keine Probe ausserhalb n/255), zwei gezeichnete Farben an einer Kante und die Auswertung interpolierter Werte an Bildpunkten, deren Mitte das Dreieck nicht bedeckt - in allen vier Versuchen sind Proben und aufgeloestes Bild unter beiden Schnittstellen gleich. Die Ursache liegt damit in etwas, das nur die Kamera des Karten-Editors tut (das Spielbild mit derselben Mehrfachabtastung ist gleich): Tiefe und Stencil mit Proben (D24S8), `discard`, die Schnittebene (SV_ClipDistance), anisotrope Abtastung an Kantenbildpunkten oder das Mischen mehrerer Stufen. Der Abgeber gibt an diesen Stellen die Zustaende von Direct3D 11 unveraendert weiter (Linienregel, Probenmaske, Alpha-to-Coverage nachgesehen). Naechster Versuch: den Kantenversuch um je eine dieser Zutaten erweitern, bis die Bilder auseinanderlaufen.

### Was gelaufen ist (Schlussrunde dieses Zweigs)

Stand 02bf83b7f (WIP 28), Bau `behaved` ohne Warnungen (`bau_r17.txt`). Alle Laeufe neben anderen Agenten (15-28 Kerne
Fremdlast), mit `PRIO=unter`.

| Was | Direct3D 12 | Direct3D 11 |
|---|---|---|
| `grafik` | 11 OK / 0 FEHLER, Debugschicht an, 0 Fehler (g12y) | 10 / 0 (g11z) |
| `karteneditor` mit `BHED_KARTE_NUR=licht1` | 16 / 0, Debugschicht 0 Fehler (kl12y) | 15 / 0 (Stand WIP 25; der Weg ist seitdem unveraendert) |
| `leistung` Abschnitt e (Karten-Editor, Echtzeit-Licht) | 1 / 0, 7,3 Mio. Zeichenbefehle, 0 nicht umgesetzte Aufrufe, Debugschicht 0 Fehler, 545-mal ausgelagert, 87-mal zurueckgeholt (e12y) | Lauf ohne Befund (e11x; die eine Pruefung des Abschnitts gilt dem Abgeber) |
| `leistung` Abschnitte k, s, m, n (Karten, Szene, Model, Menue) | 1 / 0, Debugschicht 0 Fehler (s12y) | Abschnitt s ohne Befund (s11w) |
| Fotos | `toaj`, `mustafar`, `szene40`, `modell`, `editor_vorlage_spiel`, `grafik_mipstufen_spiel` und die Oberflaeche gleich mit Direct3D 11; `editor_vorlage` 1416, `editor_echtlicht` 1040-1089, `mission` 7 Bildpunkte, `grafik_mipstufen` 97 Werte, `grafik_menue` 85 (die Ziffer im Tooltip) - wie vor dieser Runde | Gluehen direkt ins Ziel: `mustafar`, `szene40`, 3D-Bild von `mission`, `editor_vorlage_spiel` unveraendert gegen den Stand davor |
| Lints | sources, fassung, d3dbesitz, settings, i18n, headers, feldinit, cbuffer ok; portability 99 (= Basis) | |
| ctest (alle Ziele gebaut, nichts neu zu bauen) | 245 von 246 bestanden; rot nur `q3map2_teilbau` - schon an der Basis rot (LCB2-Arbeit, nicht dieser Zweig). `defgpu` lief einzeln nach (der Sammellauf wurde vor dem Messfenster angehalten): bestanden | |

Bewusst nicht gelaufen (laut Regel einmal auf dem zusammengefuehrten Stand, je Schnittstelle): Modus 1, `menueeditor`, voller
`karteneditor`, `tabfenster`/`dock` (eigene Fenster - dort wartet imgui_impl_dx12 auf den Abschickfaden), Geraeteverlust.
Neu zu pruefen sind dabei zwei Dinge dieser Runde: das Auslagern beim Wechsel zwischen Ansichten (ein vergessenes
`gebraucht()` meldet die Debugschicht als Gebrauch einer ausgelagerten Ressource) und der Abschickfaden mit eigenen Fenstern.

### Offen nach dieser Runde

- **Messung im Ruhefenster** (`sh /y/werkzeuge/dx12/messung_final.sh`, unveraendert aufzurufen; die Tabelle hat eine Zeile mehr:
  "Speicher mit Echtzeit-Licht"; die Profil-Tabelle eine Spalte "Abgeber mit allem darunter").
- **GPU-Zeit im Spielbild des Karten-Editors** auf Karten mit Himmelsportal (duel_yavin, md_am_sith): eine Kopie und sieben
  Aufloesungen mit Mehrfachabtastung je Bild, Ursache 6 - im Treiber. Weitere Abhilfe nur in der Zeichenschicht: das Portalbild
  nicht kopieren (Zielsaetze tauschen oder die Portalszene gleich in das Ziel der Hauptszene zeichnen) - nicht gebaut, die
  Zielsaetze haben mehrere Abnehmer.
- **Kanten in der Kamera des Karten-Editors** (1/255 an Dreieckskanten mit Mehrfachabtastung): Ursache eingegrenzt, nicht gefunden.
- **Der Loeschwert folgt der Loeschfarbe** (`abg2/p16_loeschwert.py`, fertig geschrieben, NICHT angewandt): Direct3D 12 loescht nur
  mit dem Wert schnell, mit dem ein Ziel angelegt ist (die Debugschicht warnt bei jedem anderen); bringt je Bild etwa 0,05 ms und
  aendert das Bild nicht (Kantenversuch).
- **Auslagern, wenn das Programm nicht zeichnet:** geprueft wird am Anfang eines Bildes; steht das Programm still, bleibt der
  Speicher belegt (der D3D11-Treiber raeumt auch dann).
- **Auslagern kostet Rucke und etwas Arbeitssatz:** was ausgelagert ist, liegt im Arbeitsspeicher (Karten-Editor in den
  Messlaeufen +130 bis +260 MB Arbeitssatz fuer -2,1 GB Grafikspeicher; mit Debugschicht neben anderen Laeufen +1,5 GB); das
  Zurueckholen haelt den Hauptfaden an (im Mittel 3 ms, einzeln bis 43 ms mit Debugschicht unter Fremdlast) - ein Ruck beim
  Wechsel auf eine Ansicht, die laenger als 4 s nicht gezeichnet wurde. Abhilfe waere `EnqueueMakeResident` auf dem
  Abschickfaden (nicht gebaut). Wer es anders abwaegen will: `=budget` oder `kAuslagernMs`.
- **Arbeitssatz in allen Ansichten ueber Direct3D 11** (Szenen +9 bis +27 %, Karten-Editor +19 bis +35 %): unsere eigenen Anteile
  sind klein (Bloecke 88 MB, Hochladepuffer 12 MB); der Rest liegt bei Laufzeit und Treiber. Nicht eingegrenzt.
- **Kleine dynamische Konstantenpuffer ueber eine Schattenkopie** (`abg2/p10_schatten.py`, fertig geschrieben, NICHT angewandt und
  nicht gebaut): `Map` gibt immer denselben Speicher heraus, gebunden wird eine Kopie im Hochladespeicher des Bildes - spart die
  Blockverwaltung je `Map(WRITE_DISCARD)`. Nur noetig, wenn das Ruhefenster den Hauptfaden noch ueber Direct3D 11 zeigt.
- Volle Runde je Schnittstelle auf dem zusammengefuehrten Stand (Modus 1, `menueeditor`, `karteneditor`, `tabfenster`/`dock`,
  Geraeteverlust) - wie bisher. Neu zu beachten: der Abschickfaden (eigene Fenster von imgui_impl_dx12 warten auf ihn).

## Changelog

Der Changelog fuer rc612 steht in `NOTIZEN_dx12_1211.md` (D3D12-2: Direct3D 12 ist dort Vorgabe, Direct3D 11 der Rueckfall). Der
fruehere Eintrag hier ("Direct3D 12 als Versuch, Direct3D 11 bleibt Vorgabe") gilt nicht mehr und ist deshalb entfernt.

## English

- (see NOTIZEN_dx12_1211.md)

## Deutsch

- (siehe NOTIZEN_dx12_1211.md)

## 中文

- (见 NOTIZEN_dx12_1211.md)

## 日本語

- (NOTIZEN_dx12_1211.md を参照)

## Español

- (ver NOTIZEN_dx12_1211.md)

## Čeština

- (viz NOTIZEN_dx12_1211.md)
