# NOTIZEN TEMPO (12.11.2026): eigener Code im Hauptfaden schneller, gleiches Bild

Agent TEMPO (Fable 5.2, ab 17:45 Nachfolger Opus 5.5), Worktree `Y:\p_ap10`, Zweig `p_tempo`, begonnen auf karte1 73c18113f,
13.11. rebased auf karte1 1465cb6a3 (mit DXR, AP13S, AP13K, LCFIX, FIX612B/C, FIX612), dann auf 3d0947208 (AP14, KLEIN).
Auftrag: `auftraege/welle_1211.md` Abschnitt 7 - weniger Hauptfaden-Zeit je Bild bei bitgleichem Bild und gleichen Dateien,
unter Direct3D 12 (Vorgabe) und Direct3D 11 (Rueckfall). Werkzeuge, Profile, Rohdaten: `Y:\werkzeuge\tempo\` (Uebergabe
`UEBERGABE.md`, Messskripte `mzyk.py`, `reihe.sh`, `tab.py`, Bildvergleich `vgl.py`, Endmessung `messung_final.sh`).

## Commits

| Commit | Posten | Dateien |
|---|---|---|
| tempo: self test leistung - BHED_LEISTUNG_BILDER | Werkzeug | `gui/selbsttest_leistung.inc` |
| tempo: real-time light remembers the light selection ... | A | `gui/app_karte_echtlicht.cpp`, `gui/karte_echtlicht.h`, `gui/selbsttest_leistung.inc`, `include/bhed/stabilsort.h`, `tests/stabilsort.cpp`, CMake-Block am Ende |
| tempo: map drawer keeps uploaded images and lightmaps in a flat store by slot | B | `gui/gpumap_win32.cpp`, `include/bhed/nachplatz.h`, `tests/nachplatz.cpp`, zwei CMake-Zeilen im selben Block |
| tempo: spielPruefen looks for the import folder once per call ... | C | `gui/app_karte.cpp`, `gui/platform_win32.cpp` |

Keine Funktionsaenderung, kein neuer Text, kein Handbuch. Keine Aenderung am Abgeber oder an der Ressourcenverwaltung von
Direct3D 12 (Nachbar D3D12-3).

## Profil vorher (Basis, Direct3D 12, 720 Bilder je Schritt, unter Last - die Anteile gelten)

`Y:\werkzeuge\tempo\profil_prof_basis.txt`, Anteil am Hauptfaden (inklusive):

| Ansicht | groesste Posten |
|---|---|
| Echtzeit-Licht ruhend | `echtLichtBildVorbereiten` 48 % (Kachelmasken 9, kugelImBild 7, Sortieren 6, gpuLicht 5, kugelKacheln 4), `spielPruefen` 10 % (davon `settingsDirectory` 7,0 %, 6,6 Punkte in `CreateDirectoryW`), `shaderFinden` 6 % |
| Echtzeit-Licht dreht | `echtLichtBildVorbereiten` 38 %, `spielPruefen` 12,5 % |
| Vorlage wie im Spiel ruhend | `zeichneKarte` 59 %, `zeichneMover` 18 %, `holeBild` 6,0 % + `holeLightmap` 4,7 % (Baum `std::map`), `buildDrawCalls` 7,5 %, `spielPruefen` 7,5 % |
| Mission, Szene spielt | `zeichneKarte` 24 %, `zeichneMover` 12 %, `buildEffectMesh` 9 %, Figuren 8,5 %, Gluehen 8,5 % |

## Was sich aendert

- **A, Echtzeit-Licht** (`echtLichtBildVorbereiten`): Auswahl der Lichter (Schritt 1/2: sichtbar, Rang, Kacheln) und Lichtliste
  fuer die Grafikkarte (Schritt 5/6) werden je Blick gemerkt (vier Plaetze, der am laengsten ungenutzte weicht).
  - Auswahl gilt, solange Kameramatrix, Auge, Bildgroesse und `g_welt.nummer` (jede neue Lichterliste zaehlt sie hoch) bitgleich sind.
  - Lichtliste gilt, solange zusaetzlich je gewaehltem Licht der Wuerfel (fertig/wartet, Atlasplatz, Weite), die Gueltigkeit der
    Sonnenkarten und `listenArt` gleich sind. Schritt 3/4 (Sonnenkarten, Schattenwuerfel) laufen wie bisher je Bild.
  - Rang und Tiefe ueber `bhed::stabileReihenfolge` (LSD-Radix auf dem float-Schluessel, genau die Reihenfolge von
    `std::stable_sort`; mit NaN der alte Weg), `gpuLicht` je Licht einmal je Lichterliste, Zwischenspeicher ueber die Bilder.
  - **DXR-Haken nach dem Rebase (13.11.):** Der Kopf von `echtLichtBildVorbereiten` (Schattenart `strahlenWahl`, Neubau-Bedingung,
    `strahlen`, `dxr::bildSetzen`, `wuerfelAus`/`wuerfelVerteilen`) und Schritt 3 (`sonnenBudget`/`himmelBudget` 0 mit Strahlen)
    stehen woertlich wie in karte1. Die zwei DXR-Zeilen aus Schritt 5 (Sonnen ohne Karte bleiben mit Strahlen in der Liste;
    ohne Wuerfel zaehlt mit Strahlen nicht als "ohne Schatten") stehen jetzt in `listeRechnen`, gesteuert ueber
    `listenArt = strahlen ? 1 : 0` am Haken nach der Lichtauswahl. `listenArt` ist Teil des Schluessels der gemerkten Liste:
    nach dem Umschalten der Schattenart bei stehender Kamera wird die Liste neu gerechnet. Mit Strahlen gibt `wuerfelAus` jedem
    Licht `nullptr` als Wuerfel - auch das steht im Schluessel (Wuerfelstand je Licht).
- **B, Bildspeicher des Kartenzeichners** (`gpumap_win32.cpp`): die fuenf Speicher nach Platz (Bilder, Lightmaps, Quellen,
  Lightmap-Stand; ebenso im geparkten Szenenspeicher) sind `bhed::NachPlatz<T>` (Feld nach Platz, Plaetze dicht ab 0 und
  durch `byShader.size()`/`lightmaps.size()` begrenzt) statt `std::map<int, T>`. Gleiche Hochladungen, gleiche Reihenfolge.
- **C, `spielPruefen` / `settingsDirectory`**: `importOrdner()` einmal je Aufruf statt zweimal (nicht ueber Bilder gemerkt:
  ein von aussen angelegter Ordner erscheint wie bisher im naechsten Bild). Mit `BHED_EINSTELLUNGEN` (nur Selbsttests)
  `CreateDirectoryW` einmal je Ordner statt bei jedem Aufruf (Mutex, wird auch aus Hintergrundfaeden gefragt).

## Rot / Gruen

| Pruefung | Rot | Gruen |
|---|---|---|
| Gegenprobe A (`BHED_ECHTLICHT_MERKPROBE=1`: jeder Treffer rechnet neu und vergleicht Wert fuer Wert), Selbsttest `leistung` Abschnitt e, Taris | Stand `st_rot` (Kameramatrix nicht im Schluessel), Lauf `m_rot12_e`: "ruhend: neu 0, gemerkt 257; dreht: Auswahl neu 0, gemerkt 257; Abweichungen 506", 1 FEHLER | nach dem Rebase auf karte1 1465cb6a3 (Stand `st_neu`, Code wie Endstand): `m_prn12_e` (Direct3D 12, Vorgabe Schattenkarten) 3/0, `m_prnx12_e` (Direct3D 12, `BHED_DXR=1`, "Raytracing-Schatten: 1991 Strukturen" im Protokoll) 3/0, `m_prn11_e` (Direct3D 11) 2/0 - je "ruhend: Lichtliste neu 0, gemerkt 257; dreht: Auswahl neu 253, gemerkt 4; Abweichungen 0"; Endstand: siehe Tests |
| ctest `stabilsort` (gegen `std::stable_sort`, +-0, +-unendlich, Denormale, Gleichstaende, bis 70001 Schluessel, auf- und absteigend) | -0 nicht wie +0 behandelt: 143 Fehler in 870 Faellen | 870 Faelle, 0 Fehler |
| ctest `nachplatz` (jede Operation gegen `std::map`, 16000 Zufallsschritte) | `groessterPlatz` liefert den kleinsten: 12661 Fehler | 0 Fehler |
| DXR-Haken (Umschalten der Schattenart bei stehender Kamera) | kein eigener Rot-Lauf: das Umschalten aendert ohnehin den Wuerfelstand jedes Lichts (`wuerfelAus` setzt `nullptr`) und baut die Schattengeometrie neu (Sonnenkarten ungueltig) - beides steht im Schluessel; `listenArt` sichert zusaetzlich | Selbsttest `karteneditor` `BHED_KARTE_NUR=dxr` (schaltet die Schattenart mehrfach um, vergleicht Bilder) mit Gegenprobe an: 35 OK / 0 FEHLER (`m_dxrn12`, wie bei DXR selbst 35/0) |
| Posten C | kein eigener Test: Verhalten gleich (ein Ordner, derselbe Pfad); Gruppen, die den Import-Ordner anlegen, siehe unten | |

Die Gegenprobe vergleicht, was an `kartegpu::lichtSetzen` geht. `gpuLicht` je Licht ist ueber `g_welt.nummer` gemerkt und
haengt nur am Licht (`gpuLicht` liest nichts ausser dem `EchtLicht`); die Lichterliste wird nur in `weltAktualisieren` ersetzt,
dort mit `++nummer`.

## Bildvergleich Basis gegen neu (`vgl.py`, pixelgenau, Selbsttest `leistung` mit `BHED_FOTOS`)

**Nach dem Rebase (13.11.):** Basis = `st_basis` (karte1 1465cb6a3 + BHED_LEISTUNG_BILDER, Bau 5b8a7a8f1), neu = `st_neu`
(dieselbe Basis + die TEMPO-Commits, Code wie der Endstand auf 3d0947208). Laeufe `m_rb*` / `m_rn*`, Ergebnis `vgl_r.txt`.

| Abschnitte | Direct3D 12 (Vorgabe: Schattenkarten) | Direct3D 12 mit `BHED_DXR=1` (Schattenstrahlen) | Direct3D 11 |
|---|---|---|---|
| e (Karten-Editor: Fenster, Echtzeit-Licht, Vorlage, Vorlage wie im Spiel) | 4 von 4 gleich | 4 von 4 gleich | 3 von 4 gleich; `leistung_editor.bmp` (Fensterfoto) 7 Bildpunkte, hoechstens 1/255 - unter Direct3D 11 schwankt dieses Foto schon Basis gegen Basis (vor dem Rebase `fb11_e`/`fb11_e2`: 26-35 Bildpunkte, hoechstens 4/255, gleiche Gegend); zweite Basis `rb11_e2` belegt es auch hier: Basis gegen Basis 8 Bildpunkte (hoechstens 1/255) in diesem Foto und 6 im Echtzeit-Licht-Foto - neu liegt im selben Rahmen |
| k, s (Mission: Szene, Vorschau) | 3 von 4 gleich; `leistung_mission.bmp` 103 Bildpunkte in einem Rechteck 9 x 14 unten rechts: die fps-Anzeige der Statuszeile ("4 fps" gegen "5 fps", angesehen) | - | 4 von 4 gleich |

**Vor dem Rebase (12.11., Basis karte1 73c18113f):** k/s und e unter beiden Grafikwegen alle gleich; m/r/n 2 von 3 gleich, nur
`leistung_menue.bmp` anders - schwankt schon Basis gegen Basis (`fb12_mrn2`: 827575 Bildpunkte im selben Rechteck; im Menue
laeuft ein Video, das Foto trifft ein anderes Einzelbild). Der beruehrte Code (Karten-Editor, Kartenzeichner) zeichnet im
Menue-Editor nicht; m/r/n nach dem Rebase deshalb nicht wiederholt.

## Messung (unter Last gemessen, Richtwert)

Selbsttest `leistung` Abschnitt e, Direct3D 12, Messfenster 18:11-18:17 (`mess_start.sh`), alt und neu nacheinander.
Zyklen = Rechenzeit des Hauptfadens je Bild aus seinen Prozessorzyklen (ganzes Bild), CPU-Wand = "CPU Mittel" des Selbsttests.
**Fremdlast: alt 29,2 Kerne, neu 18,3 Kerne** (67 fremde Bau-/Testprozesse beim Start; die Last fiel zwischen den Laeufen).
Nur eine Runde: die zweite Runde in umgekehrter Folge und die Runden fuer Mission (ks) und den Weg des Anwenders entfielen,
weil shank um 18:20 weniger Last brauchte (Hauptchat: "LAST SENKEN"). Die Wandzeiten sind deshalb nicht vergleichbar; die
Zyklen sind weniger lastempfindlich, aber nicht frei davon (geteilte Caches, Takt).

| Ansicht | Zyklen alt ms | Zyklen neu ms | neu gegen alt | Posten |
|---|---|---|---|---|
| Echtzeit-Licht ruhend | 15,59 | 3,59 | -77 % | A (+C) |
| Echtzeit-Licht Kamera dreht | 16,10 | 6,61 | -59 % | A (+C) |
| Vorlage wie im Spiel ruhend | 9,05 | 10,25 | +13 % | B (+C) |
| Vorlage wie im Spiel Kamera dreht | 10,47 | 9,92 | -5 % | B (+C) |
| duel_yavin ohne Halme | 9,12 | 5,93 | -35 % | B (+C) |
| duel_yavin mit Halmen | 10,40 | 5,47 | -47 % | B (+C) |
| md_am_sith ohne Effekte | 7,73 | 4,22 | -45 % | B (+C) |
| md_am_sith mit Effekten | 11,12 | 6,59 | -41 % | B (+C) |

Lesart: Beim Echtzeit-Licht passt die Groessenordnung zum Profil (48 % `echtLichtBildVorbereiten` + 7 % `settingsDirectory`
vorher). Die Spielbild-Zeilen sind in dieser einen Runde vom Lastabfall ueberlagert (Vorlage ruhend sogar +13 %): dort sind
nur hoechstens 10-15 % des Hauptfadens beruehrt (`holeBild` + `holeLightmap` + `spielPruefen`). **Endgueltige Zahlen** kommen
aus `Y:\werkzeuge\tempo\messung_final.sh` im Ruhefenster (alt `st_basis` gegen neu `st_neu`, Direct3D 12 drei Runden
abwechselnd, Direct3D 11 eine Runde, Weg des Anwenders eine Runde; je Lauf drei Teile `ks`, `e`, `mrn`, weil Abschnitt e allein
unter Direct3D 12 schon 7,9 GB privat erreichte; etwa 40 Minuten; Ergebnis `messung_final.txt`).

Speicher: Abschnitt e unter Direct3D 12 Spitze privat alt 7,0-7,6 GB, neu 7,6-7,9 GB (gleiche Lage, Schwankung von Lauf zu Lauf;
die Merkplaetze von A sind wenige MB). Die Groesse kommt aus dem Direct3D-12-Weg (an D3D12-3 gemeldet).

## Tests auf dem Endstand

Endstand = Kopf von `p_tempo` auf karte1 3d0947208. Selbsttest-Gruppen unter Leerlauf-Prioritaet, hoechstens 2 jkstudio
(Lastregel), auf `st_neu`/`st_basis` (Basis 1465cb6a3, Code wie Endstand), Laufordner `Y:\werkzeuge\tempo\sg\`, `sg2\`.

- Bau aller Ziele auf 3d0947208: 0 Warnungen, 0 Fehler (`bau_rebase3.txt`; ebenso auf 1465cb6a3 `bau_rebase1/2.txt`).
- Erzeugte Dateien (gen_i18n, gen_hilfe_orte, handbuch/gen_handbuch) nach jedem Rebase neu erzeugt: keine Aenderung.
- ctest auf dem Endstand: 269 von 269 gruen (`ctest_end.txt`, darin die neuen `stabilsort` und `nachplatz`; vor dem Rebase 260/260). Rot-Belege der zwei neuen ctests siehe Rot / Gruen.
- Gegenprobe A auf dem Endstand: `m_pre12_e` (Direct3D 12) 3 OK / 0 FEHLER und `m_pre11_e` (Direct3D 11) 2 OK / 0 FEHLER, je "ruhend: Lichtliste neu 0, gemerkt 257; dreht: Auswahl neu 253, gemerkt 4; Abweichungen 0"
- Gruppen, die der Code beruehrt:

| Gruppe | Direct3D 12 | Direct3D 11 |
|---|---|---|
| ke_licht (stufe7, stufe8b, gross) | neu 153 OK / 1 FEHLER, **Basis ebenso 153 OK / 1 FEHLER** - dieselbe Zeitzeile "Stufe 7: grosse Karte - keine Ruckler: p99 des Bildabstands unter 50 ms" (Leerlauf-Prioritaet unter Fremdlast); also nicht von TEMPO | 153 OK / 0 FEHLER (die Wiederholung mit Pausen nach Bildern: dieselbe Zeitzeile rot) |
| mission1 | 100 OK / 0 FEHLER | 99 OK / 0 FEHLER |
| ke_karte (licht1, stufe9c/9d, modobj = Import-Ordner) | 286 / 0 (12.11., vor dem Rebase) | - |
| teil11, teil13b, teil13c | 5/0, 7/0, 8/0 (12.11., vor dem Rebase) | - |
| karteneditor `BHED_KARTE_NUR=dxr` | 35 / 0 mit Gegenprobe | (DXR nur unter Direct3D 12) |
| leistung e/k/s (Fotolaeufe) | 0 FEHLER | 0 FEHLER |

  Bewusst nicht gelaufen: die uebrigen karteneditor-Teile (ke_grund, ke_befehle, ke_bau, ke_stufe8) und die volle Runde -
  der beruehrte Code ist das Echtzeit-Licht (ke_licht, ke_karte, dxr), der Kartenzeichner (Vorlage/Spielbild in leistung e,
  mission1, teil11/13b/13c) und spielPruefen (ke_karte mit modobj legt den Import-Ordner an). Die volle Runde laeuft nach dem
  Zusammenfuehren.
- Lints fuer die geaenderten Dateien: headers, mapstate, d3dbesitz, feldinit, resize, gapp, sources, rueckkopplung, msvclink ok; portability 101 gesamt, keiner in den geaenderten Dateien (die zwei Funde in den neuen Tests sind mit `<cstddef>` behoben). Private Adressen in Log und Diff: 0.

## Offen / Fragen an shank

- Endzahlen im Ruhefenster: `sh /y/werkzeuge/tempo/messung_final.sh` (startet nur eigene Laeufe aus `st_basis`/`st_neu`, beide auf
  karte1 1465cb6a3 und nur um TEMPO verschieden; etwa 40 Minuten; Ergebnis `messung_final.txt`).
- ke_licht "Stufe 7: keine Ruckler (p99 unter 50 ms)" ist unter Leerlauf-Prioritaet und Fremdlast rot - auf der Basis genauso;
  in der vollen Runde (Ruhe, normale Prioritaet) erneut ansehen.
- Weitere Posten, nicht angefasst (je eigener Beleg noetig): `flareSchilde` sucht je Bild alle Flaechen nach Flares (3,4 % in
  "Szene spielt"); `modelleImBild` setzt je Modell und Bild Namen zusammen (15 % in duel_yavin); Statuszeile zaehlt je Bild alle
  Pinsel (1,8-3 %); Texturbrowser `shaderFinden` (6 %). Nicht ohne Bildaenderung: `buildDrawCalls` (haengt an der Shaderzeit).
- Fragen an shank: keine.

## Changelog

## English

- Map editor: the real-time light needs far less processor time per frame while the camera rests - it keeps the light
  selection and the light list of each view instead of computing them again every frame. The picture stays exactly the same.
- Map editor and mission preview: textures and lightmaps of the map are found faster while drawing. Same picture.

## Deutsch

- Karten-Editor: Das Echtzeit-Licht braucht bei stehender Kamera deutlich weniger Rechenzeit je Bild - es merkt sich die
  Auswahl und die Liste der Lichter je Ansicht, statt sie in jedem Bild neu zu berechnen. Das Bild bleibt genau gleich.
- Karten-Editor und Missionsvorschau: Texturen und Lightmaps der Karte werden beim Zeichnen schneller gefunden. Gleiches Bild.

## 中文

- 地图编辑器：相机静止时，实时光照每帧所需的处理器时间大幅减少——它会按视图保留光源的选择和列表，而不是每帧重新计算。画面完全相同。
- 地图编辑器和任务预览：绘制时更快地找到地图的纹理和光照贴图。画面相同。

## 日本語

- マップエディター：カメラが止まっているとき、リアルタイムライトの1フレームあたりの処理時間が大きく減りました。ライトの選択とリストをビューごとに保持し、毎フレーム計算し直さなくなりました。画面はまったく同じです。
- マップエディターとミッションプレビュー：描画時にマップのテクスチャとライトマップをより速く見つけます。画面は同じです。

## Español

- Editor de mapas: la luz en tiempo real necesita mucho menos tiempo de procesador por fotograma con la cámara quieta: guarda
  la selección y la lista de luces de cada vista en lugar de calcularlas de nuevo en cada fotograma. La imagen es exactamente la misma.
- Editor de mapas y vista previa de misiones: las texturas y los lightmaps del mapa se encuentran más rápido al dibujar. La misma imagen.

## Čeština

- Editor map: světlo v reálném čase potřebuje při stojící kameře mnohem méně času procesoru na snímek – pamatuje si výběr
  a seznam světel pro každý pohled, místo aby je v každém snímku počítalo znovu. Obraz zůstává přesně stejný.
- Editor map a náhled misí: textury a lightmapy mapy se při kreslení najdou rychleji. Stejný obraz.
