# Karten-Editor Stufe 8c: Tutorial, GPU-Backen, Umbenennung in JK Studio, Vorbereitung der Zusammenführung, Gesamttest

Arbeitskopie `Y:\karte`, Zweig `karte1`, ab `1ed7a1ee` (Ende Stufe 8b).
Auftrag (Koordinator, Entscheidungen von shank): Tutorial „Erste eigene Karte“,
volles GPU-Backen mit Abweichungsbericht, Umbenennung in **JK Studio** samt
Updater-Übergang, Vorbereitung des Zusammenführens, großer Gesamttest, Notizen.
Grundregeln wie immer: jeder Fehler erst nachgestellt (rot), dann behoben, dann
im selben Test bestätigt; exakt heißt Byte-Vergleich; Texte über
`tools/gen_i18n.py` in vier Sprachen; kFassung bzw. Marke **nicht** geändert
(das entscheidet der Anwender beim Release).

## Commits

| Punkt | Commit | Inhalt |
|---|---|---|
| 1 | ddc8a9b4 | Tutorial „Your first map“: echter Oberflächenlauf mit Prüfungen, Build > Pack map as .pk3, Handbuchseite in vier Sprachen |
| 2 | d325b55f, f9f8f7b3 | q3map2 `-light -bhedgpubake`: Lichtbeiträge auf der GPU, Abweichungsbericht, ctest `q3map2_gpubake`; Zeilenverweis der Grenzentabelle |
| 2b | cc25ab8a | Messschalter `BHED_GPUBAKE_MESS` (ohne Dreieckstests / ohne Strahl) – die obere Grenze für jeden schnelleren Strahltest, also auch DXR |
| 3 | 767f62ae | Umbenennung in JK Studio: Name, Symbol, Logo, jkstudio.exe, Umzug der Einstellungen, Updater-Übergang von rc604 |
| 4 | bed5bd76 | ctest `q3map2_uebersetzer` prüft zusätzlich die LLVM-Fassung 23.1.4 |
| 5 | (siehe Gesamttest) | Behebungen aus dem Gesamttest |
| 6 | (Commit dieser Notizen) | Notizen, Changelog |

## Punkt 1: Tutorial „Erste eigene Karte“

- **Was:** Handbuchseite `tut-karte` (Gruppe Tutorials, Karte in der
  Tutorial-Übersicht und im Index), 15 Schritte mit 16 Bildern in vier
  Sprachen: Spiel und Ordner (Preferences > Game/Paths), File > New Map,
  Brush ziehen, Höhe in der Vorderansicht (Num 1, Oberkante ziehen), Textur aus
  dem Texturbrowser, Aushöhlen mit dem CSG Tool (Hollow::pull faces, Caulk aus),
  Bodentextur (Umschalt+M1 in der Vorderansicht), Licht und info_player_start
  aus dem Entity-Menü, Speichern, AutoCaulk (F4, Rechteck mit Umschalt+M2),
  Optimizer, Build > [Final] Fast mit dem Fortschrittsfenster, View > Play:
  start at info_player_start + F6, Build > Pack map as .pk3. Querverweise auf
  die Fachseiten (karteneditor_bauen, karte_bauen, karte_teilbau,
  karte_optimierer, karte_tempo, karte_werkzeuge8b, tut-projekt).
- **Neue Funktion** (eigene, nicht in NRC): **Build > Pack map as .pk3** –
  `<Karte>.pk3` neben der gebauten .bsp mit `maps/<Karte>.bsp` (und `.bhed`).
  Das Programm schreibt nie in Spielordner (die .bsp liegt nach
  `bauOrdnerVorschau` nie dort, die Prüfung `imSpielordner` sichert zweitens ab);
  ins Spiel kopiert der Anwender selbst (GameData/base bzw. Mod-Ordner,
  `devmap <Karte>`). NRCs „Create .pk3“ (`q3map2 -pk3`) schreibt dagegen in den
  Engine-Pfad – darum nicht im Tutorial.
- **Test:** Selbsttest-Teil `stufe8c` (im vollen Modus karteneditor,
  einzeln `BHED_KARTE_NUR=stufe8c`) und Bildlauf `BHED_HB_NUR=tutorial`: alle
  Schritte mit Maus und Tasten in der Oberfläche, danach geprüft: ein Brush
  512 × 512 × 256, Wandtextur auf allen 6 Seiten, sechs Wände 16 dick, Boden mit
  eigener Textur, Licht und Spielerstart im Raum, AutoCaulk 30 Außenseiten
  (innen bleiben 5 Wand- und 1 Bodenseite), Optimizer ohne überschrittene
  Grenze, **Bau ohne Leck** (kein LEAKED, keine Fehlerzeile), **der Spieler steht
  am info_player_start auf dem Boden und läuft über 200 Einheiten**, die .pk3
  enthält die .bsp Byte für Byte, die Meldung nennt `devmap meinekarte`.
  35 OK, 0 FEHLER; Bildlauf in vier Sprachen je 23 OK, 0 FEHLER.
- **Rot/Grün:** ohne die Funktion hinter dem Menüpunkt (Aufruf entfernt) 34 OK,
  1 FEHLER (die .pk3-Prüfung), danach 35 OK.
- Unterwegs im Test gefunden (Testfehler, kein Programmfehler): Umschalt+M1
  ziehen **malt** die Auswahl wie in NRC, das Rechteck ist Umschalt+M2 – das
  Handbuch sagt es so.

## Punkt 2: GPU-Backen (`-bhedgpubake`)

### Was gebaut ist

- `src/q3map2/bhed_gpubacken.cpp/.h` (D3D11-Compute, zur Laufzeit geladen):
  `LightContributionToSample` der **ersten Abtastung jedes Lichts jeder
  Roh-Lightmap** auf der GPU – Punkt-, Spot-, Sonnen-/Himmels- und
  Flächenlichter, also auch **jede Bounce-Runde** (Radiosity erzeugt
  Flächenlichter, `PointToPolygonFormFactor` in double). Supersampling (-super)
  sind die Luxel der Roh-Lightmap selbst. Adaptives Nachabtasten (-samples),
  Filter, Lichtgitter, Stitching und Nachbearbeitung bleiben auf der CPU.
- Genauigkeit: dieselben Formeln und Reihenfolgen wie q3map2, jede Rechnung
  `precise` (IEEE-Strenge, keine mad-Kontraktion), float-Division genau gerundet
  in Ganzzahlen, `VectorNormalize` wie q3map2 in double (Wurzel per Newton und
  Tuckerman-Test mit fma, Division `ddiv` 0,5 ULP), float-Wurzeln als genau
  gerundete double-Wurzel (53 ≥ 2·24+2), acos in double nach fdlibm,
  Traversierung von q3map2s eigenem Trace-Baum mit TRACE_ON-, BARY-, COPLANAR-,
  SELF_SHADOW-Epsilon, testAll (leere Äste übersprungen), feste Blätter, die
  ersten 256 Testblätter, „erstes Himmelsdreieck setzt den skyIndex“. Alpha-
  Schatten/Lichtfilter → CPU (Code 9). Keine Wave-Intrinsics, keine Summen in
  Ausführungsreihenfolge (Ergebnis unabhängig von Wave-Größe und Hersteller).
- Abweichungsbericht nach jedem GPU-Backen (stdout und `<Karte>.bhedgpu.txt`):
  eine feste Stichprobe (Paar (k+i) % 64 == 0) rechnet die CPU nach; gezählt
  getrennt: **Treffer-Entscheid** (verdeckt/frei verschieden, nach Lichtart),
  **Grenzfall** (Kultur/Hüllkurve), **Rechnung** (gleicher Code, andere Farbe),
  mittlere/maximale Abweichung, je Lightmap Proben/Abweichungen/Maximum;
  Lichtgitter: CPU (exakt).
- Robustheit: Dispatches zu 32768 Paaren, Flush mit Warten alle 16 (TDR),
  Gerätverlust (`GetDeviceRemovedReason`, im Test vorgetäuscht mit
  `BHED_GPUBAKE_VERLUST=<n>`) → Meldung, der Rest auf der CPU, kein Absturz,
  kein falsches Ergebnis; nicht endliche Werte → CPU; Speichermangel beim
  Anlegen → CPU mit Meldung; Blöcke bis 4 M Paare, eine Lightmap darüber rechnet
  die CPU. Adapter: `-bhedgpuadapter <n|Name>`, sonst
  `EnumAdapterByGpuPreference(HIGH_PERFORMANCE)` (ohne IDXGIFactory6 der mit dem
  meisten Grafikspeicher), alle Adapter und der gewählte stehen in der
  Ausgabe. bhed_q3map2.exe exportiert `NvOptimusEnablement` und
  `AmdPowerXpressRequestHighPerformance`. Keine neuen festen Importe (ctest
  `q3map2_importe` unverändert grün).
- Behoben vor dem Commit (rot → grün): der skyIndex eines Shaders entsteht
  erst in LightWorld – die GPU sah −1 und machte jeden sonnenbeschienenen Luxel
  dunkel. md_ch_ben (nur Himmelslichter): **21,03 % gleich, 32807 andere
  Treffer-Entscheidungen** (alle Sonne, GPU „verdeckt“, Grund „skyIndex-Bit“ aus
  dem Diagnoseschalter `BHED_GPUBAKE_DIAG`); md_am_sith 99,93 % / 26641. Jetzt
  werden die Infos je Runde hochgeladen: 100 %. Im ctest nachgestellt (Hof unter
  einem Himmels-Shader mit q3map_sunExt): rot 43,25 % / 1728 Treffer und Lumps
  10, 13, 14 verschieden, grün 100 % und alle Lumps gleich.

### Messwerte (RTX 4070 Laptop, i9-14900HX 32 Threads, Idle, ein Lauf je Zeile)

Schalter wie im Build-Menü „[Final] Fast“ (JA/MD: `-light -fast -patchshadows
-samples 2 -filter -bounce 8`; JK2: `-light -fast -filter -super 2`).

| Karte | CPU gesamt | CPU Illuminate | GPU gesamt | GPU Illuminate | Start (Shader übersetzen) | Bericht NVIDIA | Lumps NVIDIA | Intel UHD (float) |
|---|---|---|---|---|---|---|---|---|
| mp__duel1 | 2,1 s | 1,40 s | 8,5 s | 4,16 s | 3,6 s | 1,92 M Proben, **100 %** | **alle 18 gleich** | 10,5 s; 96,11 % (0 Treffer, 4 Grenze, 74592 Rechnung, max 0,18); Lumps 10, 14 anders |
| md_ch_ben | 2,8 s | 0,51 s | 7,6 s | 0,94 s | 3,8 s | 0,25 M, **100 %** | **alle gleich** | 7,1 s; 98,04 % (1 Grenze, 4881 Rechnung); Lump 14 anders |
| kejim_post | 10,1 s | 2,88 s | 16,9 s | 6,92 s | 2,6 s | 2,59 M, **100 %** | **alle gleich** | 24,2 s; 96,46 % (16 Treffer, 31 Grenze, 91705 Rechnung, max 30,2); Lumps 10, 14 |
| md_am_sith (Movie Duels) | 77,2 s | 38,1 s | 173,9 s | 124,6 s | 2,6 s | 38,9 M, **100 %** | **alle gleich** | nicht gemessen (aus Zeitgründen abgebrochen) |

Ein zweiter Lauf (erste Messreihe) lag innerhalb von 10–20 %.

### Wo die Zeit hingeht (Punkt 2b, Messschalter `BHED_GPUBAKE_MESS`)

| Karte | Paare | Modus | GPU-Zeit | IlluminateRawLightmap |
|---|---|---|---|---|
| kejim_post | 169,1 M | 0 voll | 2,90 s | 4,44 s |
| | | 1 ohne Dreieckstests (nur Traversierung, feste Blätter) | 1,70 s | 3,28 s |
| | | 2 ohne jeden Strahl (obere Grenze für jeden schnelleren Strahltest) | 1,66 s | 3,50 s |
| | | CPU, 32 Threads | – | 2,1–2,9 s |
| mp__duel1 (9 Runden) | 123,7 M | 0 voll | 1,66 s | 3,27 s |
| | | 2 ohne Strahl | 1,49 s | 3,15 s |
| | | CPU, 32 Threads | – | 1,30 s |

Datenmengen: je Paar 8 Byte hoch und 16 Byte zurück (kejim_post 1,35 GB hoch,
2,7 GB zurück), dazu die CPU-Schleife danach (Filter, Nachabtasten,
Aufsummieren je Licht). Der Strahlanteil liegt bei 0,2–1,2 s.

### Ehrliches Ergebnis

Das GPU-Backen ist auf der NVIDIA-GPU **Byte-gleich mit der CPU** (alle 18
Lumps auf vier Karten, 100 % der Proben) – es ist also kein Weg „schnell, aber
nicht gleich“ geworden, sondern „gleich, aber langsamer“: 2–4× langsamer als
32 CPU-Threads, und selbst ohne jeden Strahl (Modus 2) bliebe es langsamer.
**Der Schalter bleibt aus und steht nicht im Build-Menü**; das normale Backen
bleibt exakt und Standard. Auf der Intel UHD (keine double-Befehle) greifen die
float-Näherungen: 96–98 % gleich; die Abweichung hängt also vom Hersteller bzw.
von double ab, nicht von der Reihenfolge.

**DXR (D3D12 RayQuery) ist nicht gebaut** (Entscheidung des Koordinators nach
dieser Messung): im jetzigen Aufbau kann ein schnellerer Strahltest höchstens
die 0,2–1,2 s Strahlanteil sparen. Dazu kommt: q3map2 entscheidet „verdeckt“
auch über feste Blätter des Trace-Baums (Wände mit Caulk-Seiten haben keine
Dreiecke) und über Epsilons (BARY 0,01, COPLANAR 0,25, Selbstschatten); reine
Hardware-Dreieckstests gegen die Trace-Dreiecke wären hier nicht gleich.

### Künftige Arbeit: GPU-Backen, das sich lohnt (Schätzung: mehr als ein Arbeitstag)

1. **Die ganze Lichtschleife je Lightmap auf die GPU**, nur die fertigen Luxel
   zurück: Erstabtastung je Licht in einen GPU-Puffer (bleibt dort), dann je
   Luxel die Lichter **in Listenreihenfolge** filtern (gewichtetes 3×3 wie
   q3map2, nur abgebildete Nachbarn) und in die Stil-Lightmaps aufsummieren –
   die float-Additionen in derselben Reihenfolge wie die CPU, also bitgleich
   möglich; Stilplätze, negative Lichter, „nichts beleuchtet → überspringen“
   wie IlluminateRawLightmap. Damit entfallen 16 Byte Rücklesen je Paar und die
   CPU-Schleife.
2. **Nachabtasten (-samples) auf der GPU**: die 2×2-Stempel hängen in
   Zeilenfolge voneinander ab (Stufe 7, Wellenfront) – je Lightmap ein
   sequentieller GPU-Faden für die Stempel, die Unterabtastungen
   (SubsampleRawLuxel_r) parallel.
3. Paare nicht mehr als Liste hochladen (je Lightmap Luxelbereich und
   Lichtliste, der Kern sucht sein Paar), Rückgabe gepackt.
4. Danach **DXR als Vortest mit exakter Nachprüfung**: Hardware-RT gegen die
   Trace-Dreiecke (vergrößert um BARY_EPSILON) entscheidet die sicheren Fälle
   „frei“ bzw. „getroffen“, Fälle nahe an Kanten/Epsilons und alles, was feste
   Blätter betrifft, prüft der Trace-Baum exakt nach; Rückfall Compute/CPU,
   d3d12.dll zur Laufzeit, DXIL vorab mit dxc aus dem Windows SDK.
5. Lichtgitter (TraceGrid) und Bounce-Gitter genauso.
6. Die Shader-Übersetzung beim Start (2,6–3,8 s, `D3DCompile` von zwei
   Varianten) vorab im Bau erledigen (fxc aus dem Windows SDK, Bytecode als
   Header) – **offen**, weil der Schalter ohnehin aus ist.

Vergleich (Recherche des Koordinators, nicht selbst geprüft): online gibt es
keinen GPU-Fork von q3map2; der Pfleger von ericw-tools nennt GPU-Backen den
Weg zur besten Leistung; bei Unity weichen CPU- und GPU-Lightmapper vor allem
beim indirekten Licht (Bounce) ab – hier ist Bounce auf NVIDIA bitgleich;
Godots LightmapperRD zeigt, dass schon reines Compute schneller als die CPU sein
kann, wenn die ganze Pipeline auf der GPU bleibt (genau Punkt 1–3 oben); Unity
„Unified Ray Tracing“ = Hardware-RT mit Compute-Rückfall; VRAM-Mangel macht
Bakes erheblich langsamer (hier: Blöcke, Rückfall auf die CPU); DXC
`-denorm preserve` (SM 6.2) erhielte fp32-Denormale – im D3D11-Weg werden sie
auf null gesetzt (in den Proben ohne Wirkung); D3D11-fp32-Division ist nicht
genau gerundet, `ddiv` schon; TDR-Grenze 2 s, NVIDIA-Optimus-/AMD-PowerXpress-
Exporte, Agility SDK nicht nötig (kein D3D12 gebaut).

### Tests

ctest `q3map2_gpubake` (eine Testkarte mit drei Lichtern, Spot, Patch, Bounce 2;
dazu ein Hof unter einem Sonnen-Himmel): feste Obergrenze ≥ 99,9 % gleich und 0
Treffer-Entscheide, mit double alle 18 Lumps Byte-gleich, zweites Backen gleich,
vorgetäuschter Gerätverlust nach 2 Dispatches (Lightmaps und Gitter gleich der
CPU), float-Weg ≥ 75 % (gemessen 81,6 %). Rot gegen eine bhed_q3map2.exe ohne
den Schalter.

### GPU-Vorschau mit Bounce in der Kamera

**Nicht gebaut** (Zeit) – offen für Stufe 9.

## Punkt 3: Umbenennung in JK Studio

### a) Sichtbarer Name

- Fenstertitel `<Skript> - JK Studio`, About („About JK Studio“, Logo,
  „Mod tools for Jedi Knight: Jedi Academy and Jedi Outcast“, „Modules:
  BehavEd (scripts), Preview, Map Editor“, darunter das Original-Logo von
  BehavEd mit „BehavEd: reconstruction of BehavEd 2.0 …“, unten „Not affiliated
  with Lucasfilm, Disney or Raven Software.“), Hilfe-Menü „About JK Studio…“,
  Karten-Editor (Help > About JK Studio, NRC-About „JK Studio rcNNN“, Portal
  Viewer About), Meldungen (Absturz-Rückfrage, fehlende Daten mit
  `jkstudio.exe`/`JK-Studio`, Taskleisten-Hinweis, Backup-/Handbuch-Hinweise),
  Kennung des Updaters `JK-Studio/rcNNN`, Versionsangaben der .exe.
- Handbuch in vier Sprachen: alle Produktnennungen (BehavEd-Remake → JK
  Studio, behaved.exe → jkstudio.exe), Fußzeile mit Untertitel und Hinweis zur
  Marke, Startseite mit neuem Einleitungstext, About-Bild `h_ueber` und Hinweis;
  Bild `o_hilfe` (Hilfe-Menü) neu (Bildlauf `BHED_HB_NUR=hilfe`, vier Sprachen),
  Logo des Handbuchs = Programmsymbol.
- README (englisch) neu: Name, Untertitel, Hinweis zur Marke, drei Bereiche,
  Download `JK-Studio-rcNNN-windows-x64.*`, Umstieg von BehavEd-Remake.
- Release-Notizen-Werkzeug (`release_paket.py notizen`): JK-Studio-Dateien,
  Untertitel, Hinweis zur Marke, Umstieg.
- **„BehavEd“ bleibt**, wo es den Skript-Bereich oder das Original meint:
  Bereichsname, „Original-BehavEd“, „BehavEd 2.0 von Raven“, BehavEd.exe-
  Ressourcen, Thema „BehavEd Classic“, Einfüge-Rückfrage („nicht aus BehavEd
  kopiert“ – Original-Text).
- **Grep-Prüfung** (`git grep "BehavEd-Remake"` bzw. `"BehavEd"` ohne
  Handbuch-Ausgabe, third_party, Notizen): übrig und begründet:
  - historische Changelog-Einträge (`tools/changelog.txt`, `src/changelog.cpp`,
    AENDERUNGEN.md, HANDOUT/BEFUNDE/NOTIZEN früherer Stufen);
  - Lizenzköpfe `Copyright (C) 2026 BehavEd-Remake contributors` und Code-
    Kommentare „BehavEd stage N“ in den q3map2-Anpassungen (Herkunftsangaben);
  - Dateiformate: Kopfzeile `//Generated by BehavEd` der ICARUS-Skripte,
    Marken `(BehavEd) roff`/`(BehavEd) animation`, Missionsdatei
    `# BehavEd-Remake mission` (Leser erkennen sie daran), `behaved.bhc`;
  - interne Dateinamen: Einstellungsdatei `behaved.cfg` (im neuen Ordner,
    Selbsttest-Skripte nutzen sie), Protokolle `behaved.log`,
    `behaved-detail.log`, Absturzabbild `behaved-absturz.dmp`, Fensterklasse
    `behaved`, das CMake-Ziel `behaved` (Datei heißt jkstudio.exe);
  - Texte über den Übergang („bis rc605 BehavEd-Remake“, alter Repo-Name im
    Updater-Kommentar und im Übergangstest);
  - Entwickler-Dokumentation (BAUEN.md, ARCHITEKTUR.md …, deutsch) – nicht
    nachgezogen.

### b) Symbol und Logo

- `res/jkstudio.ico` (shanks .ico, 16–256 px) ist Symbol 1 der .exe;
  `res/behaved.ico` entfernt. Klein lesbar geprüft: bis 24 px klar, bei 16 px
  noch erkennbar „JK“.
- Logo: `res/jkstudio_logo.svg` mit Edge headless transparent gerendert,
  480 × 320 als PNG-Ressource `JKLOGO` (156 KB), im About mit 10 Schriftzeilen
  Höhe; die Vorlagen `res/jkstudio_logo.svg`, `res/jkstudio_icon.svg` im Repo.
  Die Schriftzüge stammen aus Cinzel und Michroma (SIL OFL 1.1):
  `licenses/OFL-Cinzel.txt`, `licenses/OFL-Michroma.txt`, im Release-Zip unter
  `licenses/`.
- Selbsttest (Bildlauf hilfe): About offen, Logo geladen (480 × 320).

### c) Programmdatei jkstudio.exe

- CMake `OUTPUT_NAME jkstudio` (das Ziel heißt weiter `behaved`, alle ctests
  über `$<TARGET_FILE:behaved>`); PDB `jkstudio.pdb`.
- Paket: Ordner `JK-Studio/`, darin `jkstudio.exe` und **`behaved.exe` als
  dieselbe Datei** (vorletzter/letzter Eintrag), Assets
  `JK-Studio-rcNNN-windows-x64.zip` / `.exe`, `SHA256SUMS.txt`;
  `release_paket.py` (Bau, Prüfung, Selbsttest 49/49) und `release.yml`
  nachgezogen.
- **bhed_q3map2.exe behält ihren Namen**: das Build-Menü (NRC-Format) nennt
  sie über `[RadiantPath]bhed_q3map2`, eigene Build-Menüs der Anwender liegen in
  den Einstellungen und würden mit einem neuen Namen brechen; sie ist ein
  Werkzeug, kein Produktname.
- Selbsttest-Skripte: `SP/s8c/idle_jk.sh` nimmt jkstudio.exe, sonst behaved.exe
  (namensunabhängig); die Originale anderer Stufen unverändert.

### d) Einstellungen

- Neuer Ordner **`%APPDATA%\jkstudio`** (ohne Leerzeichen: er steht in
  q3map2-Befehlszeilen, der Bau-Arbeitsordner `karte_bau` liegt darin; gleicher
  Name wie die .exe). Beim ersten Start, wenn er fehlt und `%APPDATA%\behaved`
  existiert: **Kopie** über `<neu>.umzug` und Umbenennen (ein Abbruch hinterlässt
  keinen halben Ordner), das Original bleibt. Nicht kopiert (entsteht neu):
  `shader\`, `karte_modellindex_*.txt`, `data\`, `karte_bau`, `karte_bsp`,
  `karte_zusammen`, `*.log`, `*.dmp` (`src/umzug.cpp`). `BHED_EINSTELLUNGEN`
  wie bisher; `BHED_APPDATA` ersetzt im Test den Ordner %APPDATA%.
- Tests: ctest `umzug` (Auslassliste, Kopie, Original unverändert, zweiter Start
  tut nichts, ohne alten Ordner nichts); Selbsttest-Modus `umzug` mit
  umgelenktem APPDATA (`SP/s8c/umzug_lauf.sh`): Ordner, behaved.cfg, Sprache
  Japanisch, Thema midnight, LastMap und Kartenliste des Karten-Editors,
  Original samt Shader-Zwischenspeicher bleibt, Zwischenspeicher nicht kopiert –
  6 OK, 0 FEHLER; rot ohne den Umzug 2 OK, 4 FEHLER.

### e) Updater und Übergang

- `kRepo = "DennisHerrm/JK-Studio"` an **einer** Stelle (`include/bhed/update.h`),
  dort auch `kProgramm`/`kAltesProgramm`.
- **Was der alte Updater (rc601–rc605, in git 8cc7bd1e = rc604 unverändert) tut:**
  GET `api.github.com/repos/DennisHerrm/BehavEd-Remake/releases/latest` mit
  WinHTTP und automatischem Folgen von Weiterleitungen; neuer, wenn die
  rc-Nummer der Marke größer als seine kFassung (rc604) ist; lädt das **erste
  .zip** (browser_download_url, folgt dem 302 zum Speicherort); streift den
  gemeinsamen obersten Ordner ab; ein Eintrag **`behaved.exe` ganz oben** (oder
  einer mit dem Namen der laufenden .exe) kommt **an die Stelle der laufenden
  .exe** (die wird zu `.alt` umbenannt), alles andere daneben – außer neben der
  .exe liegt kein `data\base\behaved.bhc` (eigenständige .exe), dann nur das
  Programm; danach startet er **den Pfad der laufenden .exe** neu
  (`--nach-update=<pid>`).
- Daraus das Paket: `behaved.exe` = jkstudio.exe im Zip. Ordner-Installation:
  behaved.exe wird JK Studio, jkstudio.exe und alles andere liegt daneben, der
  Neustart startet behaved.exe, die **reicht an jkstudio.exe weiter** (gleiche
  Argumente, `--nach-update` inklusive; `uebergangsStart` in main_win32.cpp) –
  Verknüpfungen auf behaved.exe bleiben gültig. Eigenständige .exe
  (`BehavEd-Remake-rc604-windows-x64.exe`): wird JK Studio unter ihrem alten
  Namen, sonst nichts daneben. Der neue Start räumt `behaved.exe.alt` weg und
  zieht die Einstellungen um.
- Neue Regeln (`einspielenAls`): genau ein Programm-Eintrag – der Name der
  laufenden .exe, sonst jkstudio.exe; die zweite Programmdatei ist eine Datei
  daneben, bei einer eigenständigen .exe entfällt sie (ctest `update`).
- **Simulation (ctest `updater_uebergang`)**: `tests/uebergangtest.cpp` ist der
  Updater-Code von rc604 (`tests/uebergang/*.inc`, erzeugt aus 8cc7bd1e, nur
  Server-Adresse, laufende .exe und Ablage umgelenkt), dazu ein lokaler
  Testserver (`tools/karte/updater_uebergang.py`), der GitHub nach der
  Umbenennung spielt: 301 vom alten Repo-Namen auf `/repositories/<id>/…`,
  Release v1.0.0-rc9999 mit dem echten Paket aus jkstudio.exe und
  bhed_q3map2.exe, 302 beim Download. Geprüft: Weiterleitungen gefolgt,
  Ordner-Installation (behaved.exe = jkstudio.exe, jkstudio.exe, data,
  handbuch, bhed_q3map2.exe, licenses neu, behaved.exe.alt), Neustart der
  eingespielten behaved.exe → „Programm: jkstudio.exe“, Rückgabe 0;
  eigenständige .exe wird JK Studio, nichts daneben, Neustart läuft selbst.
  **Rot** mit einem Paket ohne behaved.exe (schlichte Umbenennung): behaved.exe
  bleibt alt, die eigenständige .exe bekommt 0 Dateien – 6 FEHLER; grün mit dem
  echten Paket: 0 FEHLER. Kein Netz zu GitHub.
- Nicht live geprüft: die echte 301-Antwort von api.github.com (kein Netz zu
  GitHub im Test); GitHub dokumentiert die Weiterleitung umbenannter
  Repositorys für Web, git und API.

### f) Hinweis zur Marke

„Not affiliated with Lucasfilm, Disney or Raven Software“ im About (4 Sprachen),
in der Handbuch-Fußzeile und auf der Startseite (4 Sprachen), README,
Release-Notizen, Versionsangaben der .exe.

## Release-Anleitung für den Koordinator (Umbenennung, Übergang)

1. Zusammenführen: `karte1` ist Nachfolger von `fix605` (rc605-Stand, 0 hinter,
   172 Commits vor) und von rc604 (8cc7bd1e); Zusammenführen = vorspulen.
2. **kFassung** setzt der Anwender (z. B. rc606) – sie muss **größer als 604**
   sein (bzw. als die höchste veröffentlichte Marke), die Marke heißt
   `v1.0.0-<kFassung>` mit „rc“ und Nummer: die alten Updater vergleichen **nur
   die rc-Nummer**. Changelog-Abschnitt `## <kFassung>` in `tools/changelog.txt`
   (die Zeilen unten) und AENDERUNGEN.md.
3. **GitHub-Repo umbenennen**: Settings → Repository name →
   `DennisHerrm/BehavEd-Remake` → **`DennisHerrm/JK-Studio`** (Name an einer
   Stelle: `kRepo` in include/bhed/update.h). Nicht neu anlegen, sondern
   umbenennen – nur so leitet GitHub den alten Namen (API, Web, git) mit 301
   weiter, und alte Installationen finden das Release. Danach den alten Namen
   **nie wieder vergeben** (sonst endet die Weiterleitung).
4. Lokale Klone: `git remote set-url origin https://github.com/DennisHerrm/JK-Studio.git`
   (geht auch ohne, über die Weiterleitung).
5. Marke pushen → release.yml baut, prüft (ctest inkl. `updater_uebergang`,
   `release_paket pruefe`: jkstudio.exe und behaved.exe gleich und zuletzt,
   Lizenztexte inkl. OFL) und legt den **Entwurf** an mit genau einem Zip
   `JK-Studio-<rc>-windows-x64.zip`, `JK-Studio-<rc>-windows-x64.exe`,
   `SHA256SUMS.txt`. **Nie als Vorab-Release** (sonst sieht /releases/latest es
   nicht), **genau ein .zip** (der alte Updater nimmt das erste).
6. Prüfschritte vor dem Veröffentlichen: (a) eine echte alte Installation
   (rc604-Ordner) und eine eigenständige rc604-.exe in Testordnern mit
   `BHED_EINSTELLUNGEN` auf das Release aktualisieren lassen (Hilfe → Nach
   Updates suchen; vor dem Veröffentlichen nicht möglich – dann direkt nach dem
   Veröffentlichen); danach muss das Fenster „JK Studio“ heißen, About das neue
   Logo zeigen, die Einstellungen da sein (ohne `BHED_EINSTELLUNGEN`: Ordner
   `%APPDATA%\jkstudio` angelegt, `%APPDATA%\behaved` unverändert);
   (b) `curl -sI https://api.github.com/repos/DennisHerrm/BehavEd-Remake/releases/latest`
   muss 301 mit Location `…/repositories/<id>/releases/latest` geben;
   (c) die Zip-Struktur stichprobenartig: `JK-Studio/jkstudio.exe`,
   `JK-Studio/behaved.exe` (gleiche SHA256), `JK-Studio/licenses/OFL-*.txt`.
7. **behaved.exe im Paket lassen**, solange noch Installationen vor JK Studio
   unterwegs sein können (mindestens einige Releases): wer rc604 erst später
   aktualisiert, braucht genau diesen Eintrag.
8. README und Release-Seite nennen JK Studio; das alte Download-Repository
   (`BehavEd-Remake-Releases`, archiviert) braucht nichts.

## Punkt 4: Vorbereitung der Zusammenführung

- `karte1` taugt als nächste Hauptfassung: baut warnungsgleich, ctest
  134/134 im Arbeitsbau, frisches Build-Verzeichnis siehe unten, alle Modi im
  Gesamttest; vorgespult von fix605/rc604 aus.
- release.yml: LLVM 23.1.4 wird geladen und mit `clang version 23.1.4`
  geprüft, `-DBEHAVED_Q3MAP2_LLVM` gesetzt; der ctest `q3map2_uebersetzer`
  prüfte bisher nur den Linker (lld-link statt link.exe) – jetzt zusätzlich, dass
  `clang-cl --version` „clang version 23.1.4“ meldet (rot mit 99.0.0 geprüft).
- Frisches Build-Verzeichnis `build-frisch` ohne lokale Sonderpfade (kein
  BEHAVED_Q3MAP2_LLVM, kein PR316, nur IMGUI_DIR): Ergebnis siehe Gesamttest.

### Liste für den Koordinator

- Commits: rc604 → fix605 (35, rc605-Stand), fix605 → karte1 (172, Karten-Editor
  Stufen 1a–8c); die 8c-Commits oben.
- Was sich für Anwender ändert: Name JK Studio, Programmdatei jkstudio.exe
  (behaved.exe startet sie), Einstellungsordner %APPDATA%\jkstudio (wird
  übernommen), neues Symbol/Logo, der ganze Karten-Editor (Stufen 1–8c), das
  Tutorial „Your first map“, Build > Pack map as .pk3; Updates aus
  DennisHerrm/JK-Studio.
- Offene Risiken: (1) die echte GitHub-Weiterleitung ist nur dokumentiert, nicht
  live getestet; (2) Virenscanner/SmartScreen sehen eine neue Programmdatei
  (unsigniert) – Rückfragen beim ersten Start möglich; (3) Anwender mit eigenen
  Verknüpfungen auf eine eigenständige `BehavEd-Remake-rcNNN-…exe` behalten den
  alten Dateinamen (das Programm darin ist JK Studio); (4) nach dem ersten Start
  existieren beide Einstellungsordner – Änderungen in einer älteren Fassung
  kommen nicht mehr an; (5) ein Release mit zwei .zip oder als Vorab-Release
  bräche den Weg der alten Updater.

## Punkt 5: Großer Gesamttest

(siehe Abschnitt Schlussrunde unten)

## Offene Punkte

- GPU-Backen: Umbau der ganzen Lichtschleife auf die GPU, -samples auf der GPU,
  DXR als Vortest, Lichtgitter, vorab übersetzte Shader (siehe oben).
- GPU-Vorschau mit Bounce in der Kamera: nicht gebaut.
- Startseite des Programms ohne Logo (das Logo steht im About und im Handbuch).
- Entwickler-Dokumentation (deutsch) nennt noch BehavEd-Remake/behaved.exe.
- Die echte 301-Weiterleitung von GitHub beim Release prüfen (Anleitung 6b).

## Was Stufe 9 braucht

- **Sprungweite und Platz**: im Editor zeigen, wohin der Spieler springen kann
  (Pmove-Port, Sprunghöhe 31,6, Laufsprung) und wo er nicht durchpasst
  (Kollisionskasten 30 × 30 × 56 bzw. geduckt).
- **NPC-Wege**: Wegpunkte/navgoals setzen und prüfen (Navigator des Spiels, .nav
  aus der Karte), Erreichbarkeit anzeigen.
- **Verknüpfte Prefabs**: Gruppen, die als Verweis mehrfach stehen und
  gemeinsam geändert werden.
- **Kurven-Werkzeug**: Bögen, Rohre, Treppen aus Kurven (Patches und Brushes).
- **Modelle verteilen**: misc_model über Flächen streuen (Dichte, Drehung,
  Größe, Abstand), mit festem Startwert.
- **Gelände formen**: Pinsel zum Heben, Senken, Glätten des Terrains aus Stufe 8b.
- **Modell-Import per Ziehen**: .md3/.glm/.obj aus dem Explorer in die Ansicht.
- **Blender hin und zurück**: Auswahl als .obj/.ase exportieren, bearbeitet als
  Brushes/Patches/misc_model zurück.
- **Asset-Browser**: Texturen, Modelle, Klänge, Effekte mit Vorschau und Suche.
- **Autosave/Wiederherstellen**: wie NRC (Autosave-Seite) plus Wiederherstellen
  nach einem Absturz.
- **Geometrie zu Brushes**: Dreiecksnetze (misc_model, Patches) in Brushes
  wandeln (q3map2 -convert und eigene Zerlegung).
- Dazu aus 8c: GPU-Backen-Umbau (oben), GPU-Vorschau mit Bounce.

## Changelog

## English

- The program is now called **JK Studio** – mod tools for Jedi Knight: Jedi Academy and Jedi Outcast, with the parts BehavEd (scripts), Preview and Map Editor. New icon and logo; About shows the parts and the note "Not affiliated with Lucasfilm, Disney or Raven Software".
- The program file is `jkstudio.exe`; `behaved.exe` in the same folder starts it, so old shortcuts keep working. Downloads are called `JK-Studio-…-windows-x64.zip` / `.exe` and come from DennisHerrm/JK-Studio.
- Updating from BehavEd-Remake works by itself (Help → Check for updates); folder installations and the standalone exe both become JK Studio.
- Your settings move along: on the first start JK Studio copies `%APPDATA%\behaved` to `%APPDATA%\jkstudio`; the old folder stays unchanged.
- Map editor: new tutorial "Your first map" in the guide – a room from brushes, textures, caulk, a light, the player start, the optimizer, building, walking through it in the editor and the .pk3 for the game; every picture comes from a self-test run that checks the result.
- Map editor: Build → Pack map as .pk3 – puts the compiled map into a .pk3 next to the .bsp; you copy it into GameData/base or your mod folder yourself (the program never writes into game folders).
- bhed_q3map2: experimental `-light -bhedgpubake` computes the light contributions on the graphics card with a deviation report; on the measured NVIDIA card the result is byte-identical with the CPU, but slower than 32 CPU threads, so it is off and not in the Build menu.

## Deutsch

- Das Programm heißt jetzt **JK Studio** – Mod-Werkzeuge für Jedi Knight: Jedi Academy und Jedi Outcast, mit den Bereichen BehavEd (Skripte), Vorschau und Karten-Editor. Neues Symbol und Logo; das About-Fenster zeigt die Bereiche und den Hinweis „Nicht verbunden mit Lucasfilm, Disney oder Raven Software“.
- Die Programmdatei heißt `jkstudio.exe`; `behaved.exe` im selben Ordner startet sie, alte Verknüpfungen funktionieren weiter. Die Downloads heißen `JK-Studio-…-windows-x64.zip` / `.exe` und kommen aus DennisHerrm/JK-Studio.
- Das Update von BehavEd-Remake geht von selbst (Hilfe → Nach Updates suchen); Ordner-Installationen und die eigenständige .exe werden beide zu JK Studio.
- Die Einstellungen ziehen mit: beim ersten Start kopiert JK Studio `%APPDATA%\behaved` nach `%APPDATA%\jkstudio`; der alte Ordner bleibt unverändert.
- Karten-Editor: neues Tutorial „Deine erste Karte“ im Handbuch – ein Raum aus Brushes, Texturen, Caulk, ein Licht, der Spielerstart, der Optimierer, Bauen, im Editor hindurchlaufen und die .pk3 fürs Spiel; jedes Bild stammt aus einem Selbsttest-Lauf, der das Ergebnis prüft.
- Karten-Editor: Build → Karte als .pk3 packen – legt die gebaute Karte als .pk3 neben die .bsp; nach GameData/base oder in den Mod-Ordner kopierst du sie selbst (das Programm schreibt nie in Spielordner).
- bhed_q3map2: das experimentelle `-light -bhedgpubake` rechnet die Lichtbeiträge auf der Grafikkarte mit Abweichungsbericht; auf der gemessenen NVIDIA-Karte ist das Ergebnis Byte-gleich mit der CPU, aber langsamer als 32 CPU-Threads – darum aus und nicht im Build-Menü.

## 中文

- 程序现在名为 **JK Studio**——Jedi Knight: Jedi Academy 与 Jedi Outcast 的模组工具,包含 BehavEd(脚本)、预览和地图编辑器三个部分。全新图标和标志;“关于”窗口显示各部分以及“与 Lucasfilm、Disney 或 Raven Software 无关联”的说明。
- 程序文件为 `jkstudio.exe`;同一文件夹中的 `behaved.exe` 会启动它,旧的快捷方式继续可用。下载文件名为 `JK-Studio-…-windows-x64.zip` / `.exe`,来自 DennisHerrm/JK-Studio。
- 从 BehavEd-Remake 更新会自动完成(帮助 → 检查更新);文件夹安装和独立 exe 都会变成 JK Studio。
- 设置会一起迁移:首次启动时 JK Studio 把 `%APPDATA%\behaved` 复制到 `%APPDATA%\jkstudio`;旧文件夹保持不变。
- 地图编辑器:手册中新增教程“你的第一张地图”——用画刷搭房间、纹理、caulk、灯光、玩家起点、优化器、编译、在编辑器中走一遍以及用于游戏的 .pk3;每张图片都来自会检查结果的自检运行。
- 地图编辑器:Build → 将地图打包为 .pk3——把编译好的地图打包成 .bsp 旁的 .pk3;请自行复制到 GameData/base 或模组文件夹(程序从不写入游戏文件夹)。
- bhed_q3map2:实验性的 `-light -bhedgpubake` 在显卡上计算光照贡献并输出偏差报告;在测试用的 NVIDIA 显卡上结果与 CPU 逐字节相同,但比 32 个 CPU 线程慢,因此保持关闭,也不在 Build 菜单中。

## 日本語

- プログラムの名前は **JK Studio** になりました。Jedi Knight: Jedi Academy と Jedi Outcast 向けの MOD ツールで、BehavEd(スクリプト)・プレビュー・マップエディターの 3 つの部分があります。新しいアイコンとロゴ。「バージョン情報」に各部分と「Lucasfilm、Disney、Raven Software とは関係ありません」の注記を表示します。
- プログラムファイルは `jkstudio.exe` です。同じフォルダーの `behaved.exe` がそれを起動するので、古いショートカットも使えます。ダウンロードは `JK-Studio-…-windows-x64.zip` / `.exe` で、DennisHerrm/JK-Studio から配布されます。
- BehavEd-Remake からの更新は自動で行われます(ヘルプ → 更新を確認)。フォルダー版も単体の exe も JK Studio になります。
- 設定も引き継がれます。初回起動時に JK Studio が `%APPDATA%\behaved` を `%APPDATA%\jkstudio` にコピーします。古いフォルダーはそのまま残ります。
- マップエディター:ガイドに新しいチュートリアル「はじめてのマップ」――ブラシで部屋を作り、テクスチャ、caulk、ライト、プレイヤーの開始位置、最適化、ビルド、エディター内での試し歩き、ゲーム用の .pk3 まで。画像はすべて結果を確認するセルフテストの実行から撮影しています。
- マップエディター:Build → マップを .pk3 にまとめる――コンパイルしたマップを .bsp の隣の .pk3 にまとめます。GameData/base や MOD のフォルダーへはご自分でコピーしてください(プログラムがゲームのフォルダーに書き込むことはありません)。
- bhed_q3map2:実験的な `-light -bhedgpubake` はライトの寄与をグラフィックカードで計算し、差分レポートを出します。測定した NVIDIA カードでは結果が CPU とバイト単位で同じですが、CPU 32 スレッドより遅いため、オフのままで Build メニューにもありません。
