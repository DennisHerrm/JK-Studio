# Karten-Editor Stufe 5: der Optimierer

Arbeitskopie `Y:\karte`, Zweig `karte1`, ab `add109f` (Ende Stufe 4).
Auftrag: Grenzen von JKA, JK2 und Movie Duels aus den Quellen, die Karte
dagegen zaehlen, Bauprobleme finden und auf Knopfdruck beheben; q3map2
schneller ohne anderes Ergebnis.

## Commits

| Punkt | Commit | Inhalt |
|---|---|---|
| 1 | a11dc1f | Grenzen von JKA (SP und MP), Movie Duels und Jedi Outcast als Daten |
| 2 | 8505fe3 | zaehlen gegen jede Grenze (Karte und BSP) |
| 3 | 13795af | Pruefungen auf Bauprobleme |
| 4 | dadb397 | Beheben auf Knopfdruck |
| 5 | 70d109e | Fenster Build > Optimizer... |
| 6 | 561e232 | q3map2 mit clang-cl |
| 7 | 1d07a71 | Selbsttest: jede Pruefung und Behebung mit Wirkung, gebaute Karten, Originalkarte, grosse Karte (4 Funde behoben) |
| 8 | 976e772 | Handbuch "Optimierer und Grenzen" in vier Sprachen |
| 8b | fd439ed | Nachtrag: Pruefungen aus bekannten Fehlern (OpenJK#1358, NRC#270, NRC#48/#98, GL#189, GT#271); q3map2-clang-Bau ohne ExternalProject |
| 9 | a4dc3a8 | Schlussrunde; der Selbsttest erwartet den Eintrag "Optimizer..." im Build-Menü |
| 10 | (dieser) | diese Notizen, TESTING.md |

## Grenzen je Spiel in Kurzform (erlaubter Wert)

| Grenze | JKA SP | JKA MP | Movie Duels | Jedi Outcast |
|---|---|---|---|---|
| laufzeit.entitaeten (MAX_GENTITIES) | 1021 | 990 | 4093 | 1021 |
| kollision.submodelle (MAX_SUBMODELS) | 511 | 510 | 511 | 511 |
| kollision.patchebenen (MAX_PATCH_PLANES) | 2048 | 2048 | 8192 | 2048 (offen) |
| kollision.patchfacetten (MAX_FACETS) | 1024 | 1024 | 4096 | 1024 |
| laufzeit.modelle (MAX_MODELS) | 255 | 511 | 511 | 255 |
| laufzeit.klaenge (MAX_SOUNDS) | 379 | 255 | 511 | 255 |
| laufzeit.effekte (MAX_FX) | 127 | 63 | 255 | 127 |
| laufzeit.welteffekte (MAX_WORLD_FX) | 65 | - | 127 | 3 |
| laufzeit.npcmodelle (MAX_MODELS_PER_LEVEL) | 60 | - | 120 | - |
| laufzeit.gamestate (MAX_GAMESTATE_CHARS) | 16000 | 16000 | 32000 | 16000 |
| laufzeit.subbsp (MAX_SUB_BSP) | 31 | 32 | 31 | - |
| spawn.variablen (MAX_SPAWN_VARS) | 64 | 64 | 64 | 64 |
| spawn.zeichen (MAX_SPAWN_VARS_CHARS) | 2048 | 4096 | 2048 | 2048 |
| renderer.lightmaps (MAX_LIGHTMAPS) | 256 | 256 | 256 | 256 |
| renderer.nebel (QSORT_FOGNUM_SHIFT) | 30 | 30 | 30 | 30 |
| renderer.flaechenpunkte (SHADER_MAX_VERTEXES) | 999 | 999 | 999 | 999 |
| renderer.flaechenindizes (SHADER_MAX_INDEXES) | 5999 | 5999 | 5999 | 5999 |
| bsp.lichtgitter (MAX_MAP_LIGHTGRID) | 65535 | 65535 | 65535 | 65535 |

Hart je Spiel: JKA SP 28, JKA MP 25, Movie Duels 28, Jedi Outcast 26 (davon 1 offen); dazu je 19 weiche Formatgrenzen (MAX_MAP_* aus qfiles.h, nie "über"). Volle Tabellen mit Quellen am Ende.

## Punkt 1: Grenzen als Daten (`include/bhed/karte/grenzen.h`, `src/karte/grenzen.cpp`)

Je Regelwerk (JKA SP, JKA MP, Movie Duels, Jedi Outcast) eine Tabelle: Wert,
Define mit Datei:Zeile in jedem Quellbaum, Art (BSP-Lump, Laufzeit,
Renderer, Kollision, Spawn, Compiler), erlaubter Wert (strengerer
Vanilla-Wert, abzueglich der Plaetze, die die Engine selbst belegt), hart
oder weich, die Stelle, an der die Engine abbricht. Quellen: OpenJK
`code/` (JKA SP), `codemp/` (MP), `code/` mit `JK2_MODE` + `codeJK2/`
(Jedi Outcast), `SP\mdgit` (Movie Duels, constexpr), q3map2 aus NRC
20260114 (Compiler).

- Die `MAX_MAP_*` aus `qfiles.h` sind Formatgrenzen, die weder die Engine
  noch q3map2 prueft: Ravens eigene Karten liegen darueber (t2_dpred 2304
  Entities und 290934 Bytes Entity-Text, yavin1b 265730 Ebenen; Movie Duels
  duel_jedi_battleground 76975 Brushes, 1.6 Mio. Brush-Seiten). Sie heissen
  "weich" und werden nie "ueber", nur "Hinweis".
- Hart sind u. a.: MAX_SUBMODELS-1 (BOX_MODEL_HANDLE; MP -2 mit
  CAPSULE), Lichtgitter <= 65535 Proben (unsigned short), MAX_LIGHTMAPS
  256, Nebel 30, SHADER_MAX_VERTEXES/INDEXES (>= bricht ab: 999/5999),
  MAX_PATCH_PLANES (Vanilla 2048, MD 8192), MAX_FACETS,
  G_FindConfigstringIndex ab 1 (MAX-1), G_Spawn MAX_GENTITIES-2-MAX_CLIENTS
  (SP 1021, MD 4093, MP 990), MAX_SPAWN_VARS 64 / CHARS 2048 (MP 4096),
  MAX_ROFFS 128, MAX_MODELS_PER_LEVEL 60 (MD 120, JK2 keine),
  MAX_SUB_BSP 31 (JK2 keine), MAX_GAMESTATE_CHARS 16000 (MD 32000).
- Jedi Outcast: wo `codeJK2/` nichts eigenes hat, gilt der Wert aus
  `code/` mit `JK2_MODE`; ohne JK2-Quelle "offen" markiert.

Test `karte_grenzen` (tests/kartegrenzen.cpp): liest jedes #define und
constexpr aus den Quellen (Kommentare, JK2_MODE-Zweige, Ausdruecke ueber
Dateien hinweg), prueft Wert und Zeile jeder Stelle und dass jede
Pruefstelle "Datei:Zeile" auf die abbrechende Zeile zeigt. 2829 Pruefungen.
Rot (Mutationen): Vanilla 4096 statt 2048 -> 2 FEHL; Zeile von
MAX_MAP_BRUSHES 213->214 -> 2 FEHL; MD MAX_FX 128 statt 256 -> 1 FEHL.
Gruen 2829/0.

## Punkt 2: zaehlen (`optimierer.h/.cpp`, `patchkollision.h/.cpp`)

Aus der .map (Momentaufnahme `KartenAbbild`, im Hintergrund auswertbar):
Entities nach dem Spawn (die SP_*-Funktionen, die sich selbst freigeben, je
Spiel), Schluessel und Text je Entity, Brush-Modelle, Modelle/Klaenge/
Effekte/Welteffekte als Configstrings (Portierungen von SP_CreateRain/Snow/
Wind/WindZone/Puff und misc_weather_zone), ROFFs, NPC-Modelle, schaltbare
Lichtstile, Weltgroesse, Patchgroesse und Kollisionsebenen/-facetten je
Patch (CM_GeneratePatchCollide Zeile fuer Zeile portiert, float wie die
Engine). Aus der .bsp: jeder Lump, Lichtgitter mit Ueberlauf und "light grid
array mismatch", Lightmaps, Nebel, Ecken/Indizes je Flaeche,
Patch-Kollision, Entity-Lump, Flaechen je Ansicht aus dem PVS.

Ampel: ok / knapp (ab 90 %, einstellbar) / ueber / Hinweis (weich).
Abgleich mit allen Originalkarten (JA 57, JK2 45, MD 278 .bsp): keine
harte Grenze "ueber". Echter Fehler gefunden: Movie Duels duel_mos_espa -
das Lichtgitter ist voll, 21752 Zellen zeigen auf Index 65535 (hinter das
Gitter; q3map2 bspfile_rbsp.cpp schreibt dann 65535) - die Figuren dort
bekommen falsches Licht.

Rot (Mutationen): Modelle erlaubt = MAX_MODELS statt -1 -> 1 FEHL;
notsingle ignoriert -> 6 FEHL; Patch ohne Gegen-Ebene -> 1 FEHL. Gruen.

## Punkt 3: Pruefungen (`optimierer_pruefen.cpp`)

23 Pruefungen, jede mit Quelle (Engine, q3map2 oder Tutorial) und je einem
Testfall mit und ohne das Problem: Leck (.lin) und Detail an der
Leck-Linie, verdeckte Flaechen (Caulk), entartete, Mikro- und
Off-Grid-Pinsel, Hint/Areaportal als detail, Areaportal in Entity, Deko als
structural (Schaetzung), Ueberlappung, Blockgitter (1024), T-Verbindungen,
Patches nahe der Kollisionsgrenzen oder ueber das 129er-Gitter, Clip-Modelle,
fehlende Texturen, Brush-Entity ohne Pinsel, aus der BSP Lichtgitter,
Lightmaps (_lightmapscale), VIS-Aufwand, Dreiecke je Ansicht (Schaetzung aus
dem PVS) und Spawnpunkt im Festen (CM_PointLeafnum). T-Verbindungen erst
13 s auf duel_mos_espa, mit einem Eckengitter 271 ms; alle Pruefungen auf
duel_mos_espa (38491 Primitive) 0.7 s.

Rot (Mutationen): verdeckt mit gleicher Richtung -> 4 FEHL; Blockgitter 8
-> 1 FEHL; BSP-Kinder vertauscht -> 2 FEHL. Gruen 137/0.

## Punkt 4: Beheben (`optimierer_beheben.cpp`)

Caulk, Make Detail, Make Structural, Snap to Grid, Pinsel loeschen (Mikro,
entartet), Patch teilen (bobToolz Split patch), Brush-Modelle
zusammenlegen, worldspawn gridsize/_lightmapscale, Brush-Entity ohne Pinsel
loeschen. Je Behebung Vorschau, eigener Rueckgaengig-Schritt
("Optimizer: ..."), danach neu gezaehlt; "Alles Sichere beheben" nur
Caulk, entartet, Raster (Rundungsrauschen <= 0.05) und Mischinhalt, in einem
Schritt. Hint-Vorschlaege nur angezeigt.

Rot (Mutationen): Alles Sichere nimmt auch unsichere -> 2 FEHL;
Zusammenlegen loescht nur (zuerst unentdeckt - Pruefung "kein Pinsel geht
verloren" ergaenzt, dann 1 FEHL); eigener Undo-Schritt fehlt -> 6 FEHL.
Gruen 204/0.

## Punkt 5: das Fenster (`gui/app_karte_optimierer.cpp`)

Build > Optimizer... (eigener Eintrag nach "Update entities only"), eigenes
Fenster im Stil der NRC-Dialoge: Regelwerk (automatisch oder fest),
Filter nach Art, "Nur Probleme", Reiter Grenzen (Ampel, Wert, erlaubt,
Anteil, aus Karte/BSP) und Pruefungen (Anzahl, Behebung, sicher), Details
mit Quellen und groessten Verbrauchern; Klick waehlt und zentriert; Beheben
mit Vorschau, Alles Sichere, Bericht als Text. Pruefung in eigenem Faden auf
der Momentaufnahme, nach jeder Behebung neu; auf Wunsch nach jedem Bau.
Einstellungsseite "Optimizer" (Knapp ab, Nach jedem Bau, Regelwerk,
Schwellen fuer VIS und Dreiecke).

Selbsttest `BHED_KARTE_NUR=stufe5 BHED_STUFE5_NUR=fenster`: rot mit
abgeschaltetem `zeichneOptimierer()` 14 OK / 24 FEHLER, gruen 26 / 0.

## Punkt 6: q3map2 schneller, gleiches Ergebnis

Verglichen: MSVC 14.50 (`/O2 /fp:precise`, bisher), clang-cl 23.1.4
(`/O2 /fp:precise`, lld-link), clang-cl mit `-O3` und ThinLTO, und NRCs
`q3map2.exe` (MinGW GCC -O3) als Bezug.

**Gleiches Ergebnis:**
- ctest `q3map2_vergleich` gruen (alle Pflicht-Merkmale wie NRC).
- Byte-Vergleich wie Stufe 3 (`alle.py`, sieben Karten: ja mp__duel1,
  t1_inter; jk2 duel_pit, duel_jedi; md md_ch_ben, duel_twinsuns,
  duel_sithcatacombs; clang gegen NRC und NRC gegen NRC): bei jeder Karte
  dieselben Unterschiede wie der MSVC-Bau in NOTIZEN_karte3.md (Tabelle
  Zeile 441 ff.) - alle Dreiecke gleich, die von der Reihenfolge unabhaengigen
  Lumps gleich, -vis mit einem Thread gleich, -light im Rahmen von NRC gegen
  NRC (Punkte anders 27/51 bei mp__duel1 wie bisher, sonst 0-3).
- `q3map2_importe`: nur bcrypt, KERNEL32, WS2_32 (statische Laufzeit).

**Schneller** (Median aus 3, -light mit einem Thread wie im Byte-Vergleich,
Prioritaet Idle, je Programm nacheinander; `q3zeiten.py`):

| Karte | Stufe | NRC | MSVC (vorher) | clang-cl (nachher) | Gewinn |
|---|---|---|---|---|---|
| mp__duel1 (JA) | BSP | 0.34 s | 0.27 s | 0.30 s | - |
| | VIS | 0.14 s | 0.09 s | 0.09 s | - |
| | LIGHT | 9.95 s | 13.77 s | 10.86 s | -21 % |
| duel_pit (JK2) | BSP | 0.21 s | 0.17 s | 0.17 s | - |
| | VIS | 0.12 s | 0.08 s | 0.08 s | - |
| | LIGHT | 3.17 s | 3.89 s | 3.27 s | -16 % |
| md_ch_ben (MD) | BSP | 0.88 s | 0.70 s | 0.71 s | - |
| | VIS | 3.29 s | 3.29 s | 2.48 s | -25 % |
| | LIGHT | 12.98 s | 15.35 s | 13.23 s | -14 % |

ThinLTO + `-O3`: LIGHT 11.29 / 3.32 / 13.41 s - kein Gewinn, also nicht genommen.
BSP und VIS der kleinen Karten liegen unter der Messgenauigkeit.

**Einbau:** `-DBEHAVED_Q3MAP2_LLVM=<LLVM>/bin` baut nur `bhed_q3map2` als
eigenen Bauschritt mit Ninja und clang-cl (seit 5/8b `tools/karte/q3map2_clang.cmake`, vorher ExternalProject; behaved bleibt MSVC); fehlt ein
Werkzeug oder ninja.exe, Warnung und MSVC wie bisher. clang-cl und lld-link
finden VS und SDK selbst (kein vcvars noetig, auch aus MSBuild heraus).
Zwei Anpassungen in `src/q3map2/CMakeLists.txt`, nur fuer clang-cl:
libxml2 haengt fuer "Clang" `-pedantic` an (clang-cl kennt es nicht:
`-Wno-unknown-argument` fuer den Fremdcode), und llvm-rc lehnt das (c) in
`assimp.rc` ab (die Versionsressource landet ohnehin nicht in der .exe:
`HEADER_FILE_ONLY` mit llvm-rc). Das Werkzeug liegt unter
`Y:\werkzeuge\llvm-23.1.4` (nur clang-cl, lld-link, llvm-lib, llvm-rc,
llvm-mt und lib/clang, 242 MB, aus dem offiziellen Paket
clang+llvm-23.1.4-x86_64-pc-windows-msvc); build-test ist damit konfiguriert.

**Release-Pipeline:** `.github/workflows/release.yml` laedt den offiziellen
Installer LLVM-23.1.4-win64.exe, installiert ihn still in RUNNER_TEMP und
uebergibt `-DBEHAVED_Q3MAP2_LLVM`; klappt das nicht, Warnung und MSVC. Der
Lauf auf GitHub ist hier nicht ausprobiert (kein Push) - beim naechsten
Probelauf (workflow_dispatch) auf die Zeile "clang version 23.1.4" und den
gruenen `q3map2_uebersetzer` achten.

**Test rot/gruen:** ctest `q3map2_uebersetzer` (Rich-Kopf von link.exe vs.
lld-link): mit der MSVC-q3map2 und Erwartung clang rot ("gebaut mit msvc,
erwartet clang"), nach dem Einbau gruen; der Rueckfall (falscher Pfad)
erwartet msvc. ctest 114/114.

## Punkt 7: Selbsttest

`BHED_KARTE_NUR=stufe5` hat jetzt fünf Teile (`BHED_STUFE5_NUR=` fenster,
pruefungen, gebaut, original, gross); der ganze Lauf 62 OK / 0 FEHLER,
dabei Fotos mit `BHED_FOTOS` (karteneditor_s5_befunde, _leck, _halle,
_duel1, _gross, karteneditor_optimierer, karteneditor_pref_optimierer) -
angesehen: Fenster, Ampeln, Spalten und Details stimmen.

| Teil | Was | rot | grün |
|---|---|---|---|
| pruefungen | opt_befunde.map: je Prüfung ein Fall, alle 14 Kartenprüfungen schlagen an (dazu Patch-Ebenen als Grenze "über"); jede Behebung im laufenden Programm (verdeckt, entartet, mikro, offgrid, mischinhalt, deko, patchgross, leereentity, modellezusammen): danach kein Befund, ein Schritt "Optimizer: ...", Rückgängig bringt dieselbe Anzahl zurück; Alles Sichere; Klick wählt die Objekte | erster Lauf 31/3: Erwartung "nichts über" falsch (der 31x31-Patch IST über), offgrid blieb nach Snap to Grid (Fund 2) | 34/0 (mit dem Nachtrag 5/8b: 39/0) |
| gebaut | opt_leck (Loch + Detail-Pinsel darin) mit bhed_q3map2, "Nach jedem Bau": Leck und Detail an der Leck-Linie, Make Structural, neu gebaut dicht. opt_halle (8192 x 8192 x 1024, 64 Kacheln je auf eigener Höhe, 144 farbige Lichter, eigener Build-Eintrag ohne -bounce): Lichtgitter voll (36339-36712 Zellen hinter dem Gitter), 128 Lightmaps (knapp ab 40 %), VIS/Dreiecke über Schwelle 1; gridsize + _lightmapscale, neu gebaut: beides weg. Spawnpunkt im Leeren | ohne Fund 1: 16/9 (der Optimierer lief nach dem Leck nicht) | 21/0 (mit dem Nachtrag 5/8b: 23/0) |
| original | mp/duel1 aus den .pk3, Regeln JKA SP und MP: keine Grenze über, nichts knapp; die BSP des Spiels mitgeprüft; Start 1-53 ms, Prüfung 15-29 ms; keine Meldung der vorigen Karte | (Fund 4 im ganzen Lauf gesehen) | 15/0 |
| gross | duel_mos_espa.map (31671 Pinsel, 6820 Patches) mit der BSP aus Movie Duels, Regeln MD: keine Grenze über, Lichtgitter-Fehler 21752 gefunden. Momentaufnahme 53-103 ms, Prüfung 1.7-2.6 s, 388-451 Bilder währenddessen, größtes 9-11 ms, Mittel 4.3-5.7 ms: bedienbar | - | 14/0 |

Funde dieser Läufe (behoben, je mit Test):
1. "Nach jedem Bau" lief nicht, wenn ein Leck den Bau anhält - gerade dann
   braucht man die .lin und die Detail-Pinsel an der Leck-Linie
   (`gui/app_karte_bau.cpp`, zusammenfassung art 1 mit Grund "leak").
2. Ein Pinsel dünner als 1 Einheit zählte auch als "offgrid", Snap to Grid
   kann ihn aber nur zerdrücken; jetzt nur noch Mikropinsel (Behebung
   löschen). Unit-Test "Einrasten mit Mikropinsel daneben": rot 2 FEHL,
   grün 206/0.
3. Der Lightmap-Befund nannte die Bytes des Licht-Lumps (6291456) statt der
   Zahl der Lightmaps (128): die Renderer-Grenze geht jetzt vor
   MAX_MAP_LIGHTING (Selbsttest prüft 64 < Anzahl <= 256).
4. Die Meldung der letzten Behebung blieb nach dem Öffnen einer anderen
   Karte unter der Liste stehen (Foto karteneditor_s5_gross aus dem ersten
   ganzen Lauf: "Optimizer: worldspawn _lightmapscale 2" bei mos_espa);
   jetzt geleert, wenn das Ergebnis zu einer anderen Karte gehört
   (Selbsttest-Prüfung "keine Meldung einer Behebung aus der vorigen Karte").

ctest 114/114 (handbuch_sprachen mit dem Stand des Commits geprüft; die
Punkt-8-Dateien lagen da schon im Arbeitsordner).

## Punkt 8: Handbuch

Neue Seite "Karten-Editor: Optimierer und Grenzen" (`karte_optimierer`,
`tools/handbuch/inhalt_karte5.py`) in vier Sprachen: Öffnen, Regelwerk, BSP,
nach jedem Bau; die Ampeln und weich/hart; woher die Grenzen stammen;
Prüfungen und Behebungen (Tabelle aller Prüfungen - 23, mit dem Nachtrag 28 - mit Bedeutung,
Behebung, sicher - die Texte kommen aus den Oberflächentexten KeOptPr*,
also genau wie im Programm); Einstellungen; je Spiel eine Tabelle aller
Grenzen mit Define, erlaubtem Wert, Art, Folge und Quelle (Datei:Zeile).
Die Tabellen kommen aus `tools/handbuch/grenzen.json`, die
`kartegrenzen --json` aus der Tabelle im Code schreibt; der ctest
`karte_grenzen` vergleicht beide (rot ohne/mit veralteter Datei: "Handbuch:
grenzen.json passt zur Tabelle" FEHL, grün 2818/0). Dazu ein Hinweis auf der
Seite "Karte bauen" (Abschnitt Lecks) mit Link. Bilder aus dem Selbsttest
`BHED_EDITORTEST=handbuch BHED_HB_NUR=karte5` (mp/duel1, Regelwerk JKA MP)
in en/de/zh/ja: h_karte5_menue (mit dem Tooltip), _grenzen, _pruefungen,
_beheben, _einstellungen - angesehen, keine abgeschnittenen Beschriftungen.
`gen_handbuch.py` und `--pruefen`: 139 Seiten x 4 Sprachen, ok.

## Nachtrag zu Punkt 8: Prüfungen aus bekannten Fehlern

Auftrag des Koordinators (zweiter Zusatz): die "OPT"-Zeilen aus
`scratchpad\nrc_bugs\bekannte_fehler.md` in den Optimierer, wenn sie passen.

| Quelle | Prüfung (Kennung) | Schwere | Ein-Klick | Herleitung |
|---|---|---|---|---|
| OpenJK#1358 | misc_model_static wird nie gezeichnet (`modellunsichtbar`, braucht die gebaute BSP) | Fehler | zoffset: der kleinste Versatz (1 Einheit genau, bis 1024 nach oben/unten), der den Prüfpunkt in einen Cluster bringt; das Modell bleibt stehen | g_misc.cpp:417 liest zoffset; cg_main.cpp:3226-3229 (Movie-Duels-Quelle, gleich OpenJK SP) prüft origin + (0 0 zoffset+1) mit inPVS; sv_game.cpp:236-239 liest bei Cluster -1 mask[-1] - nie gezeichnet |
| NRC#270 | common/origin statt system/origin (`commonorigin`) | Fehler | Textur auf textures/system/origin (nicht "sicher": die Entity steht danach woanders, nämlich richtig) | map.cpp:792 erkennt Ursprungs-Pinsel nur an C_ORIGIN = surfaceparm origin (games.cpp:803); JA/JK2 haben das nur in system/origin. Ein Mod mit common/origin samt surfaceparm origin wird erkannt (Shader-Skript) |
| NRC#109 | viele kleine/schräge Structural-Brushes | - | Make Detail | gibt es schon (Prüfung `deko`), Quelle um NRC#109 ergänzt |
| NRC#48, NRC#98 | Bau-Schalter, die das Spiel nicht kann (`baubefehl`) | Fehler | - (Build-Menü) | `-external` oder `-lightmapsize` ungleich 128 (light.cpp:2522-2547 speichert dann extern - auch kleiner als 128, nicht nur grösser) bei Jedi Outcast immer, sonst wenn die Karte Lichtstile hat (style/_style, oder targetname: light.cpp:337/433; lightmaps_ydnar.cpp:3109 "fixme: make this work with external lightmaps"). **Das Build-Menü startet so einen Eintrag nicht** (rote Zeile in der Konsole) - Abweichung von NRC |
| JKHub MAX_MAP_LIGHTGRID | gridsize mit einem Klick | - | worldspawn gridsize | gibt es schon (`lichtgitter`) |
| GL#189, NRC#144 | Flächenlicht unter festen Pinseln (`flaechenlichtleck`) | Warnung | - | light_bounce.cpp:544-566: die Backsplash-Fläche liegt q3map_backsplash-Abstand (Vorgabe 23, q3map2.h:100-101) vor der Lichtfläche; liegt dazwischen ein fester Pinsel, leuchtet sie auf seiner anderen Seite. q3map_surfacelight und q3map_backsplash liest der Optimierer jetzt aus den Shader-Skripten (`shaderSkriptLesen`) |
| GT#271 | riesige Pinsel (`riesenpinsel`) | Hinweis | - | Kante ab 16384 Einheiten |

Tests: Unit-Test `testeBekannteFehler` (Shader-Skript-Leser mit
Kommentaren und Stufen, Lichtstile, Schalter je Spiel, common/origin samt
Behebung/Rückgängig/Wiederholen, riesig, Backsplash eng/weit/frei/aus,
misc_model_static an raum.nrc.bsp samt kleinstem zoffset) - rot mit
Platzhaltern 17 FEHL, grün 250/0. Selbsttest: pruefungen (common/origin,
riesig, kor1_lava 8 unter einer Platte mit dem echten korriban.shader,
-external mit Lichtstil: Befund und das Build-Menü baut nicht), gebaut
(misc_model_static in der Bodenkachel der Halle, zoffset, ohne Neubau weg).

Nicht im Optimierer (passt nicht hinein, für die Prüfstufe): die Vorschau
"wie im Spiel" soll ein misc_model_static mit Prüfpunkt im Festen ebenfalls
nicht zeigen (OpenJK#1358, zweiter Teil); Hinweis vor dem Region-Bau
(NRC#132); Speicherschätzung vor -light (JKHub safe_malloc).

Nebenbei: das Bauen von q3map2 mit clang-cl lief über ExternalProject; dessen
Stempel-Regeln erzeugten mit dem Visual-Studio-Generator die Warnung MSB8064,
sobald der Schritt neu lief. Jetzt ein eigener Bauschritt
(`tools/karte/q3map2_clang.cmake`): einmal konfigurieren, dann Ninja -
warnungsfrei.

## Punkt 9: Schlussrunde

Je ein Prozess, Selbsttests mit Priorität Idle und 8-GB-Wächter
(`SP\s5\ende_s5.sh`, in drei Teilen gestartet: A bau+ctest+karteneditor,
B Modus 1 und alle übrigen Modi, C Leistung+Kartenprüfung+Neubau).

| Schritt | Ergebnis |
|---|---|
| Bau | ok |
| ctest (Idle) | 114/114 (94 s) |
| karteneditor voll (Fotos, grosse Karte mit BSP) | 1308 OK, 1 FEHLER (481 s, Spitze 1100 MB): das Build-Menü misst mit dem neuen Eintrag "Optimizer..." 193 x 484 statt der erwarteten 460 - Erwartung um 24 ergänzt (a4dc3a8), `BHED_KARTE_NUR=fotos` danach 78/0 |
| Modus 1 | 2645 OK, 0 FEHLER (431 s, 577 MB) |
| karte | 227/0 |
| kartenfunde | 53/0 |
| fix604b | 32/0 |
| laserfalle, geschuetzanim, zielstrahl, effektgrund, figurdeform | 8/0, 3/0, 2/0, 1/0, 1/0 |
| jk2, tuerlauf, kamera, missionsbau | 15/0, 13/0, 95/0, 95/0 |
| szenen, ton, npcklasse, gluehen, gras | 26/0, 34/0, 12/0, 8/0, 13/0 |
| spulen, figurstufen, missionsliste, rest605 | 13/0, 8/0, 11/0, 6/0 |
| Leistung gegen rc605 (Szenen-Modus md_am_sith, je zweimal abwechselnd) | rc605: Vorbereitung 7516 / 7336 ms, Bild Mittel 16 / 7.6 / 11 ms bzw. 16 / 7.7 / 11 ms; Stufe 5: 7122 / 7044 ms, 16 / 7.9 / 13 bzw. 15 / 7.6 / 12 ms - gleich (das grösste Einzelbild schwankt beim ersten Lauf: 178-239 ms) |
| Kartenprüfung (278 Karten, gegen das rc605-Schlussaudit) | exit 0 (636 s); jede Summe vorher = nachher (0 Abstürze, 0 GPU-/D3D-/Protokollfehler, 1 nicht gezeichnete Entity wie bisher) |
| sauberer Neubau | 0 Fehler, 127 Warnungen (435 s) - so viele wie am Ende von Stufe 4, keine in den Dateien der Stufe 5 (alle C4456/C4996 in älterem Code); der clang-cl-Schritt für q3map2 ohne MSB8064 |

## Abweichungen von NetRadiant-Custom (alle gewollt, im Handbuch genannt)

- NRC hat keinen Optimierer: eigener Eintrag "Build > Optimizer..." (nach
  "Update entities only", mit Tooltip "not in NetRadiant-Custom"), eigenes
  Fenster im Stil der NRC-Dialoge (Farben und Knöpfe aus unserem Programm),
  eigene Einstellungsseite "Optimizer" als letzte Seite unter Settings.
- Die Quell- und Detailtexte der Grenzen und Befunde (Pruefstelle,
  Rechnung, Befund-Detail wie "116 visible faces ...") sind englisch wie die
  Meldungen der Engine und von q3map2, auch in den anderen Sprachen; Titel,
  Erklärungen, Spalten und Knöpfe sind übersetzt (DE/EN/ZH/JA).
- "Nach jedem Bau" läuft auch, wenn ein Leck den Bau anhält.
- Der Optimierer fasst die Karte nie selbst an, ausser über "Beheben" bzw.
  "Alles Sichere beheben" (je ein Rückgängig-Schritt). Hint-Vorschläge nur
  angezeigt.
- Movie Duels wird gegen die eigene Engine von Movie Duels geprüft (z. B.
  MAX_PATCH_PLANES 8192, MAX_GENTITIES 4096), JKA gegen den strengeren
  Vanilla-Wert (MAX_PATCH_PLANES 2048 statt OpenJKs 4096).
- q3map2 wird mit clang-cl statt MSVC gebaut, wenn `BEHAVED_Q3MAP2_LLVM`
  gesetzt ist (build-test ist es; Release-Pipeline lädt LLVM 23.1.4).

## Was Stufe 6 braucht

- Handbuch-Tutorial "Erste eigene Karte": von der leeren Karte über Bauen
  (Build-Menü), Leck finden, Optimierer (Grenzen, Beheben) bis zum Spielen -
  mit Bildern aus dem Selbsttest; die Testkarten opt_leck/opt_halle aus
  `stufe5::gebaut()` taugen als Vorlage.
- Grosser Gesamttest über alle Stufen des Karten-Editors (karteneditor voll
  mit grosser Karte und Fotos) und die übrigen Modi, wie in Punkt 9.
- Zusammenführen mit dem Hauptprojekt (Zweig karte1 -> main): dabei
  - `BEHAVED_Q3MAP2_LLVM` im Bauordner des Hauptprojekts setzen (LLVM 23.1.4
    liegt unter `Y:\werkzeuge\llvm-23.1.4\bin`), sonst baut q3map2 mit MSVC
    (gleiches Ergebnis, langsamer);
  - den Release-Workflow einmal per workflow_dispatch laufen lassen und
    prüfen, dass "clang version 23.1.4" erscheint und `q3map2_uebersetzer`
    clang erwartet;
  - die Fehler-Kandidaten unten an die Prüfstufe geben.
- Offene Kleinigkeiten: die Befund-Details (englisch) könnten übersetzt
  werden; die Prüfung "patchgitter" ist nur im Unit-Test mit einer Spitze von
  1e6 belegt, nicht im Selbsttest (so eine Karte kann der Editor nicht
  sinnvoll zeigen).

### Fehler-Kandidaten für die Prüfstufe

Nebenbei gefunden, nicht behoben (blockierten die Arbeit nicht). Pfade unter
`third_party/q3map2/tools/quake3/q3map2/` (NRC 20260114), OpenJK unter `code/`.

| # | Wo (Datei:Zeile) | Symptom | Fix ändert die BSP-Ausgabe? |
|---|---|---|---|
| 1 | q3map2 `bspfile_rbsp.cpp:101`, `:158`, `:161` (AddLightGridLumps) | Mehr als 65535 verschiedene Lichtgitter-Proben: die Liste ist voll, jede weitere Zelle bekommt den Index 65535 = `gridPoints.size()` - er zeigt hinter den Lump `lightgrid`. Die Engine liest dort fremden Speicher (Figuren falsch oder gar nicht beleuchtet). Belegt: Movie Duels duel_mos_espa 21752 Zellen, die Testhalle des Selbsttests 36339 Zellen; keine Warnung von q3map2. | ja, nur bei Karten mit vollem Gitter (z. B. naechste vorhandene Probe nehmen oder abbrechen) |
| 2 | q3map2 `bspfile_rbsp.cpp:114-155` | Die Suche nach einer "fast gleichen" Probe vergleicht jede Zelle mit allen bisherigen Proben (Zellen x Proben, bis 149769 x 65535 Vergleiche) - langsam bei grossen Gittern. Nur Laufzeit. | nein, wenn die Reihenfolge der Suche gleich bleibt |
| 3 | q3map2 `surface_meta.cpp:144` (CompareMetaTriangles) | Sortiert Meta-Dreiecke zuerst nach dem Zeiger auf `shaderInfo_t` (Heap-Adresse): Reihenfolge und Zusammenfassung der Flächen sind je Lauf anders (NRC gegen NRC byte-anders: surfaces, drawverts, drawindexes, leafsurfaces, brushsides). Schon in Stufe 3 gefunden. | ja (Reihenfolge der Flächen; die Dreiecke selbst bleiben gleich) |
| 4 | q3map2 `lightmaps_ydnar.cpp:837` | Dasselbe beim Packen der Lightmaps: Sortierung nach `si`-Zeiger, dadurch je Lauf andere Lightmap-Belegung und andere "identische" Lightmaps, die gemittelt werden. | ja |
| 5 | q3map2 `-vis` und `-light` mit mehreren Threads | Ergebnis hängt von der Reihenfolge der Threads ab (duel_jedi: visibility mit mehreren Threads anders, mit einem gleich; light: Punkte anders je Lauf). | ja, wenn deterministisch gemacht |
| 6 | q3map2 `writebsp.cpp:448-453` | Mehr als 30 Nebel (RBSP) nur eine Warnung ("Visual inconsistencies are expected"), die BSP wird trotzdem geschrieben; die Engine kann nur 30 (Sortierbits). Grenzwert passt zum Spiel, die Reaktion nicht. | nein für gültige Karten (Abbruch statt Datei) |
| 7 | q3map2 `lightmaps_ydnar.cpp:634-640` ("hack (god this sucks)") | Liegen viele Flächen in einer Ebene, legt q3map2 sie in eine Roh-Lightmap und erhöht still die Probengrösse, bis sie in 128x128 passt (Meldung nur mit -v). Eine 8192er Halle aus 64 Kacheln bekam so 3 statt 128 Lightmaps - die Auflösung ist nicht die verlangte. Vermutlich Absicht, aber eine Zahl, die nicht zur Einstellung passt. | ja |
| 8 | NRC Snap to Grid im Ecken-Modus (`brush.cpp:517-522` vertexModeSnap, danach die Hülle) - in unserem Port `Pinsel::eckenEinrasten` | Ein Pinsel, der dünner als das Raster ist (0.5), bleibt neben dem Raster, ohne Meldung. Ob NRC sich genauso verhält, ist zu prüfen (hier nur am Port gesehen); der Optimierer zählt solche Pinsel jetzt als Mikropinsel. | nein (Editor) |
| 9 | OpenJK `code/qcommon/cm_patch.cpp` CM_SubdivideGridColumns, `cm_patch.h:92/99` (`points[129][129]`) | Engine, nicht NRC/q3map2: ein Patch mit sehr starker Krümmung wird über 129 Spalten unterteilt, ohne Grenzprüfung - Speicherfehler. Der Optimierer meldet es (Prüfung "patchgitter"). | - (Engine) |
| 10 | OpenJK `code/rd-vanilla/tr_bsp.cpp:246` R_LoadLightmaps (auch codemp) | Engine, nicht NRC/q3map2: schreibt `tr.lightmaps[]` (MAX_LIGHTMAPS 256) ohne Grenzprüfung - mehr als 256 Lightmaps überschreiben Speicher. q3map2 begrenzt die Zahl nicht. Der Optimierer meldet es als harte Grenze. | - (Engine); ein q3map2-Abbruch ab 257 würde nur kaputte Karten verhindern |

## Die Grenzen je Spiel mit Quellen

Aus `tools/handbuch/grenzen.json` (= Tabelle im Code, ctest karte_grenzen).

### Jedi Academy (OpenJK SP)

| id | Art | Define | Wert | erlaubt | hart | Quelle(n) | Pruefstelle |
|---|---|---|---|---|---|---|---|
| bsp.entitaeten | BSP lump | MAX_MAP_ENTITIES | 2048 | 2048 | weich | OpenJK code/qcommon/qfiles.h:214 MAX_MAP_ENTITIES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.enttext | BSP lump | MAX_MAP_ENTSTRING | 262144 | 262144 | weich | OpenJK code/qcommon/qfiles.h:215 MAX_MAP_ENTSTRING | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.shader | BSP lump | MAX_MAP_SHADERS | 1024 | 1024 | weich | OpenJK code/qcommon/qfiles.h:216 MAX_MAP_SHADERS | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.modelle | BSP lump | MAX_MAP_MODELS | 1024 | 1024 | weich | OpenJK code/qcommon/qfiles.h:212 MAX_MAP_MODELS | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.pinsel | BSP lump | MAX_MAP_BRUSHES | 32768 | 32768 | weich | OpenJK code/qcommon/qfiles.h:213 MAX_MAP_BRUSHES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.pinselseiten | BSP lump | MAX_MAP_BRUSHSIDES | 131072 | 131072 | weich | OpenJK code/qcommon/qfiles.h:222 MAX_MAP_BRUSHSIDES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.ebenen | BSP lump | MAX_MAP_PLANES | 131072 | 131072 | weich | OpenJK code/qcommon/qfiles.h:220 MAX_MAP_PLANES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.knoten | BSP lump | MAX_MAP_NODES | 131072 | 131072 | weich | OpenJK code/qcommon/qfiles.h:221 MAX_MAP_NODES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.blaetter | BSP lump | MAX_MAP_LEAFS | 131072 | 131072 | weich | OpenJK code/qcommon/qfiles.h:223 MAX_MAP_LEAFS | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.blattflaechen | BSP lump | MAX_MAP_LEAFFACES | 131072 | 131072 | weich | OpenJK code/qcommon/qfiles.h:224 MAX_MAP_LEAFFACES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.blattpinsel | BSP lump | MAX_MAP_LEAFBRUSHES | 262144 | 262144 | weich | OpenJK code/qcommon/qfiles.h:225 MAX_MAP_LEAFBRUSHES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.flaechen | BSP lump | MAX_MAP_DRAW_SURFS | 131072 | 131072 | weich | OpenJK code/qcommon/qfiles.h:233 MAX_MAP_DRAW_SURFS | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.punkte | BSP lump | MAX_MAP_DRAW_VERTS | 524288 | 524288 | weich | OpenJK code/qcommon/qfiles.h:234 MAX_MAP_DRAW_VERTS | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.indizes | BSP lump | MAX_MAP_DRAW_INDEXES | 524288 | 524288 | weich | OpenJK code/qcommon/qfiles.h:235 MAX_MAP_DRAW_INDEXES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.nebel | BSP lump | MAX_MAP_FOGS | 256 | 256 | weich | OpenJK code/qcommon/qfiles.h:219 MAX_MAP_FOGS | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.licht | BSP lump | MAX_MAP_LIGHTING | 8388608 | 8388608 | weich | OpenJK code/qcommon/qfiles.h:227 MAX_MAP_LIGHTING | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.sicht | BSP lump | MAX_MAP_VISIBILITY | 4194304 | 4194304 | weich | OpenJK code/qcommon/qfiles.h:231 MAX_MAP_VISIBILITY | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.portale | BSP lump | MAX_MAP_PORTALS | 131072 | 131072 | weich | OpenJK code/qcommon/qfiles.h:226 MAX_MAP_PORTALS | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.lichtgitter | BSP lump | MAX_MAP_LIGHTGRID | 65535 | 65535 | ja | OpenJK code/qcommon/qfiles.h:228 MAX_MAP_LIGHTGRID<br>q3map2 tools/quake3/q3map2/bspfile_rbsp.cpp:80 MAX_MAP_GRID | code/rd-vanilla/tr_bsp.cpp R_LoadLightGridArray: indexes are unsigned short; q3map2 bspfile_rbsp.cpp:158 writes index 65535 (behind the array) when the grid is full |
| bsp.lichtgitterfeld | BSP lump | MAX_MAP_LIGHTGRID_ARRAY | 1048576 | 1048576 | weich | OpenJK code/qcommon/qfiles.h:229 MAX_MAP_LIGHTGRID_ARRAY<br>q3map2 tools/quake3/q3map2/q3map2.h:208 MAX_MAP_LIGHTGRID | the engine reads any size (code/rd-vanilla/tr_bsp.cpp R_LoadLightGridArray); q3map2 light.cpp:1749 raises gridsize until the array fits |
| bsp.areas | BSP lump | MAX_MAP_AREAS | 256 | 256 | ja | OpenJK code/qcommon/qfiles.h:218 MAX_MAP_AREAS<br>OpenJK code/qcommon/q_shared.h:228 MAX_MAP_AREA_BYTES<br>q3map2 tools/quake3/q3map2/q3map2.h:201 MAX_MAP_AREAS | code/qcommon/cm_test.cpp CM_WriteAreaBits: bit field of MAX_MAP_AREA_BYTES (32 bytes = 256 areas) |
| renderer.flaechenpunkte | renderer | SHADER_MAX_VERTEXES | 1000 | 999 | ja | OpenJK code/qcommon/qfiles.h:33 SHADER_MAX_VERTEXES<br>q3map2 tools/quake3/q3map2/games.cpp:756 | code/rd-vanilla/tr_bsp.cpp:555 ParseTriSurf, tr_surface.cpp:61 RB_CheckOverflow (Com_Error at numVerts >= SHADER_MAX_VERTEXES) |
| renderer.flaechenindizes | renderer | SHADER_MAX_INDEXES | 6000 | 5999 | ja | OpenJK code/qcommon/qfiles.h:34 SHADER_MAX_INDEXES | code/rd-vanilla/tr_surface.cpp RB_CheckOverflow (Com_Error at indexes >= SHADER_MAX_INDEXES) |
| renderer.lightmaps | renderer | MAX_LIGHTMAPS | 256 | 256 | ja | OpenJK code/rd-vanilla/tr_local.h:843 MAX_LIGHTMAPS | code/rd-vanilla/tr_bsp.cpp:250 R_LoadLightmaps writes tr.lightmaps[] without a check (array of MAX_LIGHTMAPS) |
| renderer.nebel | renderer | QSORT_FOGNUM_SHIFT | 30 | 30 | ja | OpenJK code/rd-vanilla/tr_local.h:874 QSORT_FOGNUM_SHIFT<br>OpenJK code/rd-vanilla/tr_local.h:875 QSORT_REFENTITYNUM_SHIFT<br>q3map2 tools/quake3/q3map2/q3map2.h:204 MAX_RBSP_FOGS | code/rd-vanilla/tr_main.cpp R_AddDrawSurf: the fog sits in bits 2..6 of the sort key; q3map2 writebsp.cpp:450 warns |
| renderer.patchgroesse | renderer | MAX_PATCH_SIZE | 32 | 31 | ja | OpenJK code/rd-vanilla/tr_local.h:592 MAX_PATCH_SIZE<br>q3map2 tools/quake3/q3map2/q3map2.h:124 MAX_PATCH_SIZE | q3map2 fog.cpp:128 Error "MAX_PATCH_SIZE after split"; NRC patchmodule.cpp:58 (31) |
| renderer.drawsurfs | renderer | MAX_DRAWSURFS | 65536 | 65536 | ja | OpenJK code/rd-vanilla/tr_local.h:847 MAX_DRAWSURFS | code/rd-vanilla/tr_main.cpp R_AddDrawSurf: index = numDrawSurfs & DRAWSURF_MASK (overwrites) |
| renderer.shader | renderer | MAX_SHADERS | 8192 | 8192 | ja | OpenJK code/rd-vanilla/tr_local.h:43 SHADERNUM_BITS<br>OpenJK code/rd-vanilla/tr_local.h:44 MAX_SHADERS | code/rd-vanilla/tr_shader.cpp (every loaded shader: world, models, effects, menus) |
| renderer.skins | renderer | MAX_SKINS | 512 | 512 | ja | OpenJK code/rd-vanilla/tr_local.h:844 MAX_SKINS | code/rd-vanilla/tr_image.cpp RE_RegisterSkin |
| kollision.submodelle | collision | MAX_SUBMODELS | 512 | 511 | ja | OpenJK code/qcommon/q_shared.h:642 MAX_SUBMODELS<br>OpenJK code/qcommon/cm_local.h:31 BOX_MODEL_HANDLE | code/qcommon/cm_load.cpp:143 Com_Error above MAX_SUBMODELS; the last slot is BOX_MODEL_HANDLE |
| kollision.patchebenen | collision | MAX_PATCH_PLANES | 4096 | 2048 | ja | OpenJK code/qcommon/cm_patch.h:65 MAX_PATCH_PLANES | code/qcommon/cm_patch.cpp:514 / :568 Com_Error when one more plane comes (numPlanes == MAX_PATCH_PLANES) |
| kollision.patchfacetten | collision | MAX_FACETS | 1024 | 1024 | ja | OpenJK code/qcommon/cm_patch.h:64 MAX_FACETS | code/qcommon/cm_patch.cpp:1094 Com_Error "MAX_FACETS" |
| kollision.patchpunkte | collision | MAX_PATCH_VERTS | 1024 | 1024 | ja | OpenJK code/qcommon/cm_load.cpp:527 MAX_PATCH_VERTS | code/qcommon/cm_load.cpp:564 Com_Error "ParseMesh: MAX_PATCH_VERTS" |
| kollision.weltgrenze | collision | MAX_MAP_BOUNDS | 65535 | 65534 | ja | OpenJK code/qcommon/cm_polylib.h:44 MAX_MAP_BOUNDS<br>OpenJK code/qcommon/q_shared.h:802 MAX_WORLD_COORD<br>q3map2 tools/quake3/common/maxworld.h:4 MAX_WORLD_COORD | code/qcommon/cm_patch.cpp CM_ValidateFacet: a facet with a bound >= MAX_MAP_BOUNDS is dropped |
| laufzeit.entitaeten | runtime | MAX_GENTITIES | 1024 | 1021 | ja | OpenJK code/qcommon/q_shared.h:621 GENTITYNUM_BITS<br>OpenJK code/qcommon/q_shared.h:618 MAX_CLIENTS | code/game/g_utils.cpp:847 G_Error "G_Spawn: no free entities" |
| laufzeit.modelle | runtime | MAX_MODELS | 256 | 255 | ja | OpenJK code/qcommon/q_shared.h:632 MAX_MODELS | code/game/g_utils.cpp:76 G_Error "G_FindConfigstringIndex: overflow" |
| laufzeit.klaenge | runtime | MAX_SOUNDS | 380 | 379 | ja | OpenJK code/qcommon/q_shared.h:637 MAX_SOUNDS | code/game/g_utils.cpp:76 G_Error "G_FindConfigstringIndex: overflow" |
| laufzeit.effekte | runtime | MAX_FX | 128 | 127 | ja | OpenJK code/qcommon/q_shared.h:644 MAX_FX | code/game/g_utils.cpp:76 G_Error "G_FindConfigstringIndex: overflow" |
| laufzeit.welteffekte | runtime | MAX_WORLD_FX | 66 | 65 | ja | OpenJK code/qcommon/q_shared.h:649 MAX_WORLD_FX | code/game/g_utils.cpp:76 G_Error "G_FindConfigstringIndex: overflow"; code/game/g_fx.cpp (fx_rain, fx_snow, fx_wind, ...) |
| laufzeit.gamestate | runtime | MAX_GAMESTATE_CHARS | 16000 | 16000 | ja | OpenJK code/qcommon/q_shared.h:730 MAX_GAMESTATE_CHARS | client/cl_parse.cpp Com_Error "MAX_GAMESTATE_CHARS exceeded" (not in the source checkout SP\ojk\oj; the same check as MovieDuels code/client/cl_parse.cpp:457) |
| laufzeit.roffs | runtime | MAX_ROFFS | 128 | 128 | ja | OpenJK code/game/g_roff.h:34 MAX_ROFFS | code/game/g_roff.cpp:648 "MAX_ROFFS count exceeded. Skipping load" (the ROFF is missing) |
| laufzeit.npcmodelle | runtime | MAX_MODELS_PER_LEVEL | 60 | 60 | ja | OpenJK code/game/NPC_stats.cpp:43 MAX_MODELS_PER_LEVEL | code/game/NPC_stats.cpp:1179 Com_Error "About to overflow modelsAlreadyDone" |
| laufzeit.subbsp | runtime | MAX_SUB_BSP | 32 | 31 | ja | OpenJK code/qcommon/q_shared.h:640 MAX_SUB_BSP | code/qcommon/cm_load.cpp:1117 Com_Error "CM_LoadSubBSP: too many unique sub BSPs"; code/game/g_utils.cpp:76 G_Error "G_FindConfigstringIndex: overflow" (G_BSPIndex) |
| spawn.variablen | spawn | MAX_SPAWN_VARS | 64 | 64 | ja | OpenJK code/game/g_local.h:221 MAX_SPAWN_VARS | code/game/g_spawn.cpp:1257 "G_ParseSpawnVars: MAX_SPAWN_VARS" |
| spawn.zeichen | spawn | MAX_SPAWN_VARS_CHARS | 2048 | 2048 | ja | OpenJK code/game/g_local.h:222 MAX_SPAWN_VARS_CHARS | code/game/g_spawn.cpp:1189 "G_AddSpawnVarToken: MAX_SPAWN_VARS" |
| compiler.lichtschalter | compiler | MAX_SWITCHED_LIGHTS | 32 | 32 | ja | q3map2 tools/quake3/q3map2/q3map2.h:191 MAX_SWITCHED_LIGHTS | q3map2 writebsp.cpp:272 Error "MAX_SWITCHED_LIGHTS (32) exceeded" (SetLightStyles) |
| compiler.sicht | compiler | MAX_MAP_VISIBILITY | 33554440 | 33554440 | ja | q3map2 tools/quake3/q3map2/q3map2.h:145 VIS_HEADER_SIZE<br>q3map2 tools/quake3/q3map2/q3map2.h:209 MAX_MAP_VISCLUSTERS<br>q3map2 tools/quake3/q3map2/q3map2.h:210 MAX_MAP_VISIBILITY | q3map2 vis.cpp:733 Error "MAX_MAP_VISIBILITY exceeded" |

### Jedi Academy MP (OpenJK)

| id | Art | Define | Wert | erlaubt | hart | Quelle(n) | Pruefstelle |
|---|---|---|---|---|---|---|---|
| bsp.entitaeten | BSP lump | MAX_MAP_ENTITIES | 2048 | 2048 | weich | OpenJK codemp/qcommon/qfiles.h:316 MAX_MAP_ENTITIES | not checked by the engine (codemp/qcommon/cm_load.cpp / codemp/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.enttext | BSP lump | MAX_MAP_ENTSTRING | 262144 | 262144 | weich | OpenJK codemp/qcommon/qfiles.h:317 MAX_MAP_ENTSTRING | not checked by the engine (codemp/qcommon/cm_load.cpp / codemp/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.shader | BSP lump | MAX_MAP_SHADERS | 1024 | 1024 | weich | OpenJK codemp/qcommon/qfiles.h:318 MAX_MAP_SHADERS | not checked by the engine (codemp/qcommon/cm_load.cpp / codemp/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.modelle | BSP lump | MAX_MAP_MODELS | 1024 | 1024 | weich | OpenJK codemp/qcommon/qfiles.h:314 MAX_MAP_MODELS | not checked by the engine (codemp/qcommon/cm_load.cpp / codemp/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.pinsel | BSP lump | MAX_MAP_BRUSHES | 32768 | 32768 | weich | OpenJK codemp/qcommon/qfiles.h:315 MAX_MAP_BRUSHES | not checked by the engine (codemp/qcommon/cm_load.cpp / codemp/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.pinselseiten | BSP lump | MAX_MAP_BRUSHSIDES | 131072 | 131072 | weich | OpenJK codemp/qcommon/qfiles.h:324 MAX_MAP_BRUSHSIDES | not checked by the engine (codemp/qcommon/cm_load.cpp / codemp/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.ebenen | BSP lump | MAX_MAP_PLANES | 131072 | 131072 | weich | OpenJK codemp/qcommon/qfiles.h:322 MAX_MAP_PLANES | not checked by the engine (codemp/qcommon/cm_load.cpp / codemp/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.knoten | BSP lump | MAX_MAP_NODES | 131072 | 131072 | weich | OpenJK codemp/qcommon/qfiles.h:323 MAX_MAP_NODES | not checked by the engine (codemp/qcommon/cm_load.cpp / codemp/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.blaetter | BSP lump | MAX_MAP_LEAFS | 131072 | 131072 | weich | OpenJK codemp/qcommon/qfiles.h:325 MAX_MAP_LEAFS | not checked by the engine (codemp/qcommon/cm_load.cpp / codemp/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.blattflaechen | BSP lump | MAX_MAP_LEAFFACES | 131072 | 131072 | weich | OpenJK codemp/qcommon/qfiles.h:326 MAX_MAP_LEAFFACES | not checked by the engine (codemp/qcommon/cm_load.cpp / codemp/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.blattpinsel | BSP lump | MAX_MAP_LEAFBRUSHES | 262144 | 262144 | weich | OpenJK codemp/qcommon/qfiles.h:327 MAX_MAP_LEAFBRUSHES | not checked by the engine (codemp/qcommon/cm_load.cpp / codemp/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.flaechen | BSP lump | MAX_MAP_DRAW_SURFS | 131072 | 131072 | weich | OpenJK codemp/qcommon/qfiles.h:334 MAX_MAP_DRAW_SURFS | not checked by the engine (codemp/qcommon/cm_load.cpp / codemp/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.punkte | BSP lump | MAX_MAP_DRAW_VERTS | 524288 | 524288 | weich | OpenJK codemp/qcommon/qfiles.h:335 MAX_MAP_DRAW_VERTS | not checked by the engine (codemp/qcommon/cm_load.cpp / codemp/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.indizes | BSP lump | MAX_MAP_DRAW_INDEXES | 524288 | 524288 | weich | OpenJK codemp/qcommon/qfiles.h:336 MAX_MAP_DRAW_INDEXES | not checked by the engine (codemp/qcommon/cm_load.cpp / codemp/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.nebel | BSP lump | MAX_MAP_FOGS | 256 | 256 | weich | OpenJK codemp/qcommon/qfiles.h:321 MAX_MAP_FOGS | not checked by the engine (codemp/qcommon/cm_load.cpp / codemp/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.licht | BSP lump | MAX_MAP_LIGHTING | 8388608 | 8388608 | weich | OpenJK codemp/qcommon/qfiles.h:329 MAX_MAP_LIGHTING | not checked by the engine (codemp/qcommon/cm_load.cpp / codemp/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.sicht | BSP lump | MAX_MAP_VISIBILITY | 6291456 | 6291456 | weich | OpenJK codemp/qcommon/qfiles.h:332 MAX_MAP_VISIBILITY | not checked by the engine (codemp/qcommon/cm_load.cpp / codemp/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.portale | BSP lump | MAX_MAP_PORTALS | 131072 | 131072 | weich | OpenJK codemp/qcommon/qfiles.h:328 MAX_MAP_PORTALS | not checked by the engine (codemp/qcommon/cm_load.cpp / codemp/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.lichtgitter | BSP lump | MAX_MAP_LIGHTGRID | 65535 | 65535 | ja | OpenJK codemp/qcommon/qfiles.h:330 MAX_MAP_LIGHTGRID<br>q3map2 tools/quake3/q3map2/bspfile_rbsp.cpp:80 MAX_MAP_GRID | codemp/rd-vanilla/tr_bsp.cpp R_LoadLightGridArray: indexes are unsigned short; q3map2 bspfile_rbsp.cpp:158 writes index 65535 (behind the array) when the grid is full |
| bsp.lichtgitterfeld | BSP lump | MAX_MAP_LIGHTGRID_ARRAY | 1048576 | 1048576 | weich | OpenJK codemp/qcommon/qfiles.h:331 MAX_MAP_LIGHTGRID_ARRAY<br>q3map2 tools/quake3/q3map2/q3map2.h:208 MAX_MAP_LIGHTGRID | the engine reads any size (codemp/rd-vanilla/tr_bsp.cpp R_LoadLightGridArray); q3map2 light.cpp:1749 raises gridsize until the array fits |
| bsp.areas | BSP lump | MAX_MAP_AREAS | 256 | 256 | ja | OpenJK codemp/qcommon/qfiles.h:320 MAX_MAP_AREAS<br>OpenJK codemp/qcommon/q_shared.h:222 MAX_MAP_AREA_BYTES<br>q3map2 tools/quake3/q3map2/q3map2.h:201 MAX_MAP_AREAS | codemp/qcommon/cm_test.cpp CM_WriteAreaBits: bit field of MAX_MAP_AREA_BYTES (32 bytes = 256 areas) |
| renderer.flaechenpunkte | renderer | SHADER_MAX_VERTEXES | 1000 | 999 | ja | OpenJK codemp/qcommon/qfiles.h:34 SHADER_MAX_VERTEXES<br>q3map2 tools/quake3/q3map2/games.cpp:756 | codemp/rd-vanilla/tr_bsp.cpp:538 ParseTriSurf, tr_surface.cpp:59 RB_CheckOverflow (Com_Error at numVerts >= SHADER_MAX_VERTEXES) |
| renderer.flaechenindizes | renderer | SHADER_MAX_INDEXES | 6000 | 5999 | ja | OpenJK codemp/qcommon/qfiles.h:35 SHADER_MAX_INDEXES | codemp/rd-vanilla/tr_surface.cpp RB_CheckOverflow (Com_Error at indexes >= SHADER_MAX_INDEXES) |
| renderer.lightmaps | renderer | MAX_LIGHTMAPS | 256 | 256 | ja | OpenJK codemp/rd-vanilla/tr_local.h:878 MAX_LIGHTMAPS | codemp/rd-vanilla/tr_bsp.cpp:246 R_LoadLightmaps writes tr.lightmaps[] without a check (array of MAX_LIGHTMAPS) |
| renderer.nebel | renderer | QSORT_FOGNUM_SHIFT | 30 | 30 | ja | OpenJK codemp/rd-vanilla/tr_local.h:897 QSORT_FOGNUM_SHIFT<br>OpenJK codemp/rd-vanilla/tr_local.h:898 QSORT_REFENTITYNUM_SHIFT<br>q3map2 tools/quake3/q3map2/q3map2.h:204 MAX_RBSP_FOGS | codemp/rd-vanilla/tr_main.cpp R_AddDrawSurf: the fog sits in bits 2..6 of the sort key; q3map2 writebsp.cpp:450 warns |
| renderer.patchgroesse | renderer | MAX_PATCH_SIZE | 32 | 31 | ja | OpenJK codemp/rd-vanilla/tr_local.h:648 MAX_PATCH_SIZE<br>q3map2 tools/quake3/q3map2/q3map2.h:124 MAX_PATCH_SIZE | q3map2 fog.cpp:128 Error "MAX_PATCH_SIZE after split"; NRC patchmodule.cpp:58 (31) |
| renderer.drawsurfs | renderer | MAX_DRAWSURFS | 65536 | 65536 | ja | OpenJK codemp/rd-vanilla/tr_local.h:882 MAX_DRAWSURFS | codemp/rd-vanilla/tr_main.cpp R_AddDrawSurf: index = numDrawSurfs & DRAWSURF_MASK (overwrites) |
| renderer.shader | renderer | MAX_SHADERS | 16384 | 16384 | ja | OpenJK codemp/rd-vanilla/tr_local.h:40 SHADERNUM_BITS<br>OpenJK codemp/rd-vanilla/tr_local.h:41 MAX_SHADERS | codemp/rd-vanilla/tr_shader.cpp (every loaded shader: world, models, effects, menus) |
| renderer.skins | renderer | MAX_SKINS | 1024 | 1024 | ja | OpenJK codemp/rd-vanilla/tr_local.h:879 MAX_SKINS | codemp/rd-vanilla/tr_image.cpp RE_RegisterSkin |
| kollision.submodelle | collision | MAX_SUBMODELS | 512 | 510 | ja | OpenJK codemp/qcommon/cm_local.h:30 MAX_SUBMODELS<br>OpenJK codemp/qcommon/cm_local.h:31 BOX_MODEL_HANDLE | codemp/qcommon/cm_load.cpp:146 Com_Error above MAX_SUBMODELS; the last slot is BOX_MODEL_HANDLE |
| kollision.patchebenen | collision | MAX_PATCH_PLANES | 4096 | 2048 | ja | OpenJK codemp/qcommon/cm_patch.h:67 MAX_PATCH_PLANES | codemp/qcommon/cm_patch.cpp:479 / :533 Com_Error when one more plane comes (numPlanes == MAX_PATCH_PLANES) |
| kollision.patchfacetten | collision | MAX_FACETS | 1024 | 1024 | ja | OpenJK codemp/qcommon/cm_patch.h:66 MAX_FACETS | codemp/qcommon/cm_patch.cpp:1060 Com_Error "MAX_FACETS" |
| kollision.patchpunkte | collision | MAX_PATCH_VERTS | 1024 | 1024 | ja | OpenJK codemp/qcommon/cm_load.cpp:519 MAX_PATCH_VERTS | codemp/qcommon/cm_load.cpp:556 Com_Error "ParseMesh: MAX_PATCH_VERTS" |
| kollision.weltgrenze | collision | MAX_MAP_BOUNDS | 65535 | 65534 | ja | OpenJK codemp/qcommon/cm_polylib.h:44 MAX_MAP_BOUNDS<br>OpenJK codemp/qcommon/q_shared.h:54 MAX_WORLD_COORD<br>q3map2 tools/quake3/common/maxworld.h:4 MAX_WORLD_COORD | codemp/qcommon/cm_patch.cpp CM_ValidateFacet: a facet with a bound >= MAX_MAP_BOUNDS is dropped |
| laufzeit.entitaeten | runtime | MAX_GENTITIES | 1024 | 990 | ja | OpenJK codemp/qcommon/q_shared.h:895 GENTITYNUM_BITS<br>OpenJK codemp/qcommon/q_shared.h:890 MAX_CLIENTS | codemp/game/g_utils.c:845 G_Error "G_Spawn: no free entities" |
| laufzeit.modelle | runtime | MAX_MODELS | 512 | 511 | ja | OpenJK codemp/qcommon/q_shared.h:920 MAX_MODELS | codemp/game/g_utils.c:110 Error "G_FindConfigstringIndex: overflow" |
| laufzeit.klaenge | runtime | MAX_SOUNDS | 256 | 255 | ja | OpenJK codemp/qcommon/q_shared.h:921 MAX_SOUNDS | codemp/game/g_utils.c:110 Error "G_FindConfigstringIndex: overflow" |
| laufzeit.effekte | runtime | MAX_FX | 64 | 63 | ja | OpenJK codemp/qcommon/q_shared.h:923 MAX_FX | codemp/game/g_utils.c:110 Error "G_FindConfigstringIndex: overflow" |
| laufzeit.gamestate | runtime | MAX_GAMESTATE_CHARS | 16000 | 16000 | ja | OpenJK codemp/qcommon/q_shared.h:946 MAX_GAMESTATE_CHARS | client/cl_parse.cpp Com_Error "MAX_GAMESTATE_CHARS exceeded" (not in the source checkout SP\ojk\oj; the same check as MovieDuels code/client/cl_parse.cpp:457) |
| laufzeit.subbsp | runtime | MAX_SUB_BSP | 32 | 32 | ja | OpenJK codemp/qcommon/q_shared.h:925 MAX_SUB_BSP | codemp/qcommon/cm_load.cpp:1040 Com_Error "CM_LoadSubBSP: too many unique sub BSPs" |
| spawn.variablen | spawn | MAX_SPAWN_VARS | 64 | 64 | ja | OpenJK codemp/game/bg_public.h:36 MAX_SPAWN_VARS | codemp/game/g_spawn.c:1182 "G_ParseSpawnVars: MAX_SPAWN_VARS" |
| spawn.zeichen | spawn | MAX_SPAWN_VARS_CHARS | 4096 | 4096 | ja | OpenJK codemp/game/bg_public.h:37 MAX_SPAWN_VARS_CHARS | codemp/game/g_spawn.c:976 "G_AddSpawnVarToken: MAX_SPAWN_VARS" |
| compiler.lichtschalter | compiler | MAX_SWITCHED_LIGHTS | 32 | 32 | ja | q3map2 tools/quake3/q3map2/q3map2.h:191 MAX_SWITCHED_LIGHTS | q3map2 writebsp.cpp:272 Error "MAX_SWITCHED_LIGHTS (32) exceeded" (SetLightStyles) |
| compiler.sicht | compiler | MAX_MAP_VISIBILITY | 33554440 | 33554440 | ja | q3map2 tools/quake3/q3map2/q3map2.h:145 VIS_HEADER_SIZE<br>q3map2 tools/quake3/q3map2/q3map2.h:209 MAX_MAP_VISCLUSTERS<br>q3map2 tools/quake3/q3map2/q3map2.h:210 MAX_MAP_VISIBILITY | q3map2 vis.cpp:733 Error "MAX_MAP_VISIBILITY exceeded" |

### Movie Duels

| id | Art | Define | Wert | erlaubt | hart | Quelle(n) | Pruefstelle |
|---|---|---|---|---|---|---|---|
| bsp.entitaeten | BSP lump | MAX_MAP_ENTITIES | 2048 | 2048 | weich | MovieDuels code/qcommon/qfiles.h:321 MAX_MAP_ENTITIES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.enttext | BSP lump | MAX_MAP_ENTSTRING | 262144 | 262144 | weich | MovieDuels code/qcommon/qfiles.h:322 MAX_MAP_ENTSTRING | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.shader | BSP lump | MAX_MAP_SHADERS | 1024 | 1024 | weich | MovieDuels code/qcommon/qfiles.h:323 MAX_MAP_SHADERS | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.modelle | BSP lump | MAX_MAP_MODELS | 1024 | 1024 | weich | MovieDuels code/qcommon/qfiles.h:319 MAX_MAP_MODELS | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.pinsel | BSP lump | MAX_MAP_BRUSHES | 32768 | 32768 | weich | MovieDuels code/qcommon/qfiles.h:320 MAX_MAP_BRUSHES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.pinselseiten | BSP lump | MAX_MAP_BRUSHSIDES | 131072 | 131072 | weich | MovieDuels code/qcommon/qfiles.h:329 MAX_MAP_BRUSHSIDES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.ebenen | BSP lump | MAX_MAP_PLANES | 131072 | 131072 | weich | MovieDuels code/qcommon/qfiles.h:327 MAX_MAP_PLANES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.knoten | BSP lump | MAX_MAP_NODES | 131072 | 131072 | weich | MovieDuels code/qcommon/qfiles.h:328 MAX_MAP_NODES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.blaetter | BSP lump | MAX_MAP_LEAFS | 131072 | 131072 | weich | MovieDuels code/qcommon/qfiles.h:330 MAX_MAP_LEAFS | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.blattflaechen | BSP lump | MAX_MAP_LEAFFACES | 131072 | 131072 | weich | MovieDuels code/qcommon/qfiles.h:331 MAX_MAP_LEAFFACES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.blattpinsel | BSP lump | MAX_MAP_LEAFBRUSHES | 262144 | 262144 | weich | MovieDuels code/qcommon/qfiles.h:332 MAX_MAP_LEAFBRUSHES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.flaechen | BSP lump | MAX_MAP_DRAW_SURFS | 131072 | 131072 | weich | MovieDuels code/qcommon/qfiles.h:340 MAX_MAP_DRAW_SURFS | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.punkte | BSP lump | MAX_MAP_DRAW_VERTS | 524288 | 524288 | weich | MovieDuels code/qcommon/qfiles.h:341 MAX_MAP_DRAW_VERTS | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.indizes | BSP lump | MAX_MAP_DRAW_INDEXES | 524288 | 524288 | weich | MovieDuels code/qcommon/qfiles.h:342 MAX_MAP_DRAW_INDEXES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.nebel | BSP lump | MAX_MAP_FOGS | 256 | 256 | weich | MovieDuels code/qcommon/qfiles.h:326 MAX_MAP_FOGS | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.licht | BSP lump | MAX_MAP_LIGHTING | 8388608 | 8388608 | weich | MovieDuels code/qcommon/qfiles.h:334 MAX_MAP_LIGHTING | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.sicht | BSP lump | MAX_MAP_VISIBILITY | 4194304 | 4194304 | weich | MovieDuels code/qcommon/qfiles.h:338 MAX_MAP_VISIBILITY | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.portale | BSP lump | MAX_MAP_PORTALS | 131072 | 131072 | weich | MovieDuels code/qcommon/qfiles.h:333 MAX_MAP_PORTALS | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.lichtgitter | BSP lump | MAX_MAP_LIGHTGRID | 65535 | 65535 | ja | MovieDuels code/qcommon/qfiles.h:335 MAX_MAP_LIGHTGRID<br>q3map2 tools/quake3/q3map2/bspfile_rbsp.cpp:80 MAX_MAP_GRID | code/rd-vanilla/tr_bsp.cpp R_LoadLightGridArray: indexes are unsigned short; q3map2 bspfile_rbsp.cpp:158 writes index 65535 (behind the array) when the grid is full |
| bsp.lichtgitterfeld | BSP lump | MAX_MAP_LIGHTGRID_ARRAY | 1048576 | 1048576 | weich | MovieDuels code/qcommon/qfiles.h:336 MAX_MAP_LIGHTGRID_ARRAY<br>q3map2 tools/quake3/q3map2/q3map2.h:208 MAX_MAP_LIGHTGRID | the engine reads any size (code/rd-vanilla/tr_bsp.cpp R_LoadLightGridArray); q3map2 light.cpp:1749 raises gridsize until the array fits |
| bsp.areas | BSP lump | MAX_MAP_AREAS | 256 | 256 | ja | MovieDuels code/qcommon/qfiles.h:325 MAX_MAP_AREAS<br>MovieDuels code/qcommon/q_shared.h:222 MAX_MAP_AREA_BYTES<br>q3map2 tools/quake3/q3map2/q3map2.h:201 MAX_MAP_AREAS | code/qcommon/cm_test.cpp CM_WriteAreaBits: bit field of MAX_MAP_AREA_BYTES (32 bytes = 256 areas) |
| renderer.flaechenpunkte | renderer | SHADER_MAX_VERTEXES | 1000 | 999 | ja | MovieDuels code/qcommon/qfiles.h:33 SHADER_MAX_VERTEXES<br>q3map2 tools/quake3/q3map2/games.cpp:756 | code/rd-vanilla/tr_bsp.cpp:525 ParseTriSurf, tr_surface.cpp:60 RB_CheckOverflow (Com_Error at numVerts >= SHADER_MAX_VERTEXES) |
| renderer.flaechenindizes | renderer | SHADER_MAX_INDEXES | 6000 | 5999 | ja | MovieDuels code/qcommon/qfiles.h:34 SHADER_MAX_INDEXES | code/rd-vanilla/tr_surface.cpp RB_CheckOverflow (Com_Error at indexes >= SHADER_MAX_INDEXES) |
| renderer.lightmaps | renderer | MAX_LIGHTMAPS | 256 | 256 | ja | MovieDuels code/rd-vanilla/tr_local.h:827 MAX_LIGHTMAPS | code/rd-vanilla/tr_bsp.cpp:236 R_LoadLightmaps writes tr.lightmaps[] without a check (array of MAX_LIGHTMAPS) |
| renderer.nebel | renderer | QSORT_FOGNUM_SHIFT | 30 | 30 | ja | MovieDuels code/rd-vanilla/tr_local.h:857 QSORT_FOGNUM_SHIFT<br>MovieDuels code/rd-vanilla/tr_local.h:858 QSORT_REFENTITYNUM_SHIFT<br>q3map2 tools/quake3/q3map2/q3map2.h:204 MAX_RBSP_FOGS | code/rd-vanilla/tr_main.cpp R_AddDrawSurf: the fog sits in bits 2..6 of the sort key; q3map2 writebsp.cpp:450 warns |
| renderer.patchgroesse | renderer | MAX_PATCH_SIZE | 32 | 31 | ja | MovieDuels code/rd-vanilla/tr_local.h:576 MAX_PATCH_SIZE<br>q3map2 tools/quake3/q3map2/q3map2.h:124 MAX_PATCH_SIZE | q3map2 fog.cpp:128 Error "MAX_PATCH_SIZE after split"; NRC patchmodule.cpp:58 (31) |
| renderer.drawsurfs | renderer | MAX_DRAWSURFS | 65536 | 65536 | ja | MovieDuels code/rd-vanilla/tr_local.h:830 MAX_DRAWSURFS | code/rd-vanilla/tr_main.cpp R_AddDrawSurf: index = numDrawSurfs & DRAWSURF_MASK (overwrites) |
| renderer.shader | renderer | MAX_SHADERS | 8192 | 8192 | ja | MovieDuels code/rd-vanilla/tr_local.h:42 SHADERNUM_BITS<br>MovieDuels code/rd-vanilla/tr_local.h:43 MAX_SHADERS | code/rd-vanilla/tr_shader.cpp (every loaded shader: world, models, effects, menus) |
| renderer.skins | renderer | MAX_SKINS | 1024 | 1024 | ja | MovieDuels code/rd-vanilla/tr_local.h:828 MAX_SKINS | code/rd-vanilla/tr_image.cpp RE_RegisterSkin |
| kollision.submodelle | collision | MAX_SUBMODELS | 512 | 511 | ja | MovieDuels code/qcommon/q_shared.h:684 MAX_SUBMODELS<br>MovieDuels code/qcommon/cm_local.h:31 BOX_MODEL_HANDLE | code/qcommon/cm_load.cpp:188 Com_Error above MAX_SUBMODELS; the last slot is BOX_MODEL_HANDLE |
| kollision.patchebenen | collision | MAX_PATCH_PLANES | 8192 | 8192 | ja | MovieDuels code/qcommon/cm_patch.h:62 MAX_PATCH_PLANES | code/qcommon/cm_patch.cpp:423 / :475 Com_Error when one more plane comes (numPlanes == MAX_PATCH_PLANES) |
| kollision.patchfacetten | collision | MAX_FACETS | 4096 | 4096 | ja | MovieDuels code/qcommon/cm_patch.h:61 MAX_FACETS | code/qcommon/cm_patch.cpp:1005 Com_Error "MAX_FACETS" |
| kollision.patchpunkte | collision | MAX_PATCH_VERTS | 1024 | 1024 | ja | MovieDuels code/qcommon/cm_load.cpp:545 MAX_PATCH_VERTS | code/qcommon/cm_load.cpp:581 Com_Error "ParseMesh: MAX_PATCH_VERTS" |
| kollision.weltgrenze | collision | MAX_MAP_BOUNDS | 65535 | 65534 | ja | MovieDuels code/qcommon/cm_polylib.h:44 MAX_MAP_BOUNDS<br>MovieDuels code/qcommon/q_shared.h:924 MAX_WORLD_COORD<br>q3map2 tools/quake3/common/maxworld.h:4 MAX_WORLD_COORD | code/qcommon/cm_patch.cpp CM_ValidateFacet: a facet with a bound >= MAX_MAP_BOUNDS is dropped |
| laufzeit.entitaeten | runtime | MAX_GENTITIES | 4096 | 4093 | ja | MovieDuels code/qcommon/q_shared.h:668 GENTITYNUM_BITS<br>MovieDuels code/qcommon/q_shared.h:665 MAX_CLIENTS | code/game/g_utils.cpp:1130 G_Error "G_Spawn: no free entities" |
| laufzeit.modelle | runtime | MAX_MODELS | 512 | 511 | ja | MovieDuels code/qcommon/q_shared.h:678 MAX_MODELS | code/game/g_utils.cpp:107 (message only, then writes into the next range) "G_FindConfigstringIndex: overflow" |
| laufzeit.klaenge | runtime | MAX_SOUNDS | 512 | 511 | ja | MovieDuels code/qcommon/q_shared.h:680 MAX_SOUNDS | code/game/g_utils.cpp:107 (message only, then writes into the next range) "G_FindConfigstringIndex: overflow" |
| laufzeit.effekte | runtime | MAX_FX | 256 | 255 | ja | MovieDuels code/qcommon/q_shared.h:686 MAX_FX | code/game/g_utils.cpp:107 (message only, then writes into the next range) "G_FindConfigstringIndex: overflow" |
| laufzeit.welteffekte | runtime | MAX_WORLD_FX | 128 | 127 | ja | MovieDuels code/qcommon/q_shared.h:693 MAX_WORLD_FX | code/game/g_utils.cpp:107 (message only, then writes into the next range) "G_FindConfigstringIndex: overflow"; code/game/g_fx.cpp (fx_rain, fx_snow, fx_wind, ...) |
| laufzeit.gamestate | runtime | MAX_GAMESTATE_CHARS | 32000 | 32000 | ja | MovieDuels code/qcommon/q_shared.h:769 MAX_GAMESTATE_CHARS | code/client/cl_parse.cpp:457 and cl_cgame.cpp:354 Com_Error "MAX_GAMESTATE_CHARS exceeded" |
| laufzeit.roffs | runtime | MAX_ROFFS | 128 | 128 | ja | MovieDuels code/game/g_roff.h:32 MAX_ROFFS | code/game/g_roff.cpp:459 "MAX_ROFFS count exceeded. Skipping load" (the ROFF is missing) |
| laufzeit.npcmodelle | runtime | MAX_MODELS_PER_LEVEL | 120 | 120 | ja | MovieDuels code/game/NPC_stats.cpp:66 MAX_MODELS_PER_LEVEL | code/game/NPC_stats.cpp:1334 Com_Error "About to overflow modelsAlreadyDone" |
| laufzeit.subbsp | runtime | MAX_SUB_BSP | 32 | 31 | ja | MovieDuels code/qcommon/q_shared.h:682 MAX_SUB_BSP | code/qcommon/cm_load.cpp:1131 Com_Error "CM_LoadSubBSP: too many unique sub BSPs"; code/game/g_utils.cpp:107 (message only, then writes into the next range) "G_FindConfigstringIndex: overflow" (G_BSPIndex) |
| spawn.variablen | spawn | MAX_SPAWN_VARS | 64 | 64 | ja | MovieDuels code/game/g_local.h:245 MAX_SPAWN_VARS | code/game/g_spawn.cpp:1430 "G_ParseSpawnVars: MAX_SPAWN_VARS" |
| spawn.zeichen | spawn | MAX_SPAWN_VARS_CHARS | 2048 | 2048 | ja | MovieDuels code/game/g_local.h:246 MAX_SPAWN_VARS_CHARS | code/game/g_spawn.cpp:1355 "G_AddSpawnVarToken: MAX_SPAWN_VARS" |
| compiler.lichtschalter | compiler | MAX_SWITCHED_LIGHTS | 32 | 32 | ja | q3map2 tools/quake3/q3map2/q3map2.h:191 MAX_SWITCHED_LIGHTS | q3map2 writebsp.cpp:272 Error "MAX_SWITCHED_LIGHTS (32) exceeded" (SetLightStyles) |
| compiler.sicht | compiler | MAX_MAP_VISIBILITY | 33554440 | 33554440 | ja | q3map2 tools/quake3/q3map2/q3map2.h:145 VIS_HEADER_SIZE<br>q3map2 tools/quake3/q3map2/q3map2.h:209 MAX_MAP_VISCLUSTERS<br>q3map2 tools/quake3/q3map2/q3map2.h:210 MAX_MAP_VISIBILITY | q3map2 vis.cpp:733 Error "MAX_MAP_VISIBILITY exceeded" |

### Jedi Outcast (OpenJK JK2_MODE)

| id | Art | Define | Wert | erlaubt | hart | Quelle(n) | Pruefstelle |
|---|---|---|---|---|---|---|---|
| bsp.entitaeten | BSP lump | MAX_MAP_ENTITIES | 2048 | 2048 | weich | OpenJK code/qcommon/qfiles.h:214 MAX_MAP_ENTITIES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.enttext | BSP lump | MAX_MAP_ENTSTRING | 262144 | 262144 | weich | OpenJK code/qcommon/qfiles.h:215 MAX_MAP_ENTSTRING | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.shader | BSP lump | MAX_MAP_SHADERS | 1024 | 1024 | weich | OpenJK code/qcommon/qfiles.h:216 MAX_MAP_SHADERS | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.modelle | BSP lump | MAX_MAP_MODELS | 1024 | 1024 | weich | OpenJK code/qcommon/qfiles.h:212 MAX_MAP_MODELS | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.pinsel | BSP lump | MAX_MAP_BRUSHES | 32768 | 32768 | weich | OpenJK code/qcommon/qfiles.h:213 MAX_MAP_BRUSHES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.pinselseiten | BSP lump | MAX_MAP_BRUSHSIDES | 131072 | 131072 | weich | OpenJK code/qcommon/qfiles.h:222 MAX_MAP_BRUSHSIDES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.ebenen | BSP lump | MAX_MAP_PLANES | 131072 | 131072 | weich | OpenJK code/qcommon/qfiles.h:220 MAX_MAP_PLANES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.knoten | BSP lump | MAX_MAP_NODES | 131072 | 131072 | weich | OpenJK code/qcommon/qfiles.h:221 MAX_MAP_NODES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.blaetter | BSP lump | MAX_MAP_LEAFS | 131072 | 131072 | weich | OpenJK code/qcommon/qfiles.h:223 MAX_MAP_LEAFS | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.blattflaechen | BSP lump | MAX_MAP_LEAFFACES | 131072 | 131072 | weich | OpenJK code/qcommon/qfiles.h:224 MAX_MAP_LEAFFACES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.blattpinsel | BSP lump | MAX_MAP_LEAFBRUSHES | 262144 | 262144 | weich | OpenJK code/qcommon/qfiles.h:225 MAX_MAP_LEAFBRUSHES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.flaechen | BSP lump | MAX_MAP_DRAW_SURFS | 131072 | 131072 | weich | OpenJK code/qcommon/qfiles.h:233 MAX_MAP_DRAW_SURFS | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.punkte | BSP lump | MAX_MAP_DRAW_VERTS | 524288 | 524288 | weich | OpenJK code/qcommon/qfiles.h:234 MAX_MAP_DRAW_VERTS | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.indizes | BSP lump | MAX_MAP_DRAW_INDEXES | 524288 | 524288 | weich | OpenJK code/qcommon/qfiles.h:235 MAX_MAP_DRAW_INDEXES | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.nebel | BSP lump | MAX_MAP_FOGS | 256 | 256 | weich | OpenJK code/qcommon/qfiles.h:219 MAX_MAP_FOGS | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.licht | BSP lump | MAX_MAP_LIGHTING | 8388608 | 8388608 | weich | OpenJK code/qcommon/qfiles.h:227 MAX_MAP_LIGHTING | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.sicht | BSP lump | MAX_MAP_VISIBILITY | 4194304 | 4194304 | weich | OpenJK code/qcommon/qfiles.h:231 MAX_MAP_VISIBILITY | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.portale | BSP lump | MAX_MAP_PORTALS | 131072 | 131072 | weich | OpenJK code/qcommon/qfiles.h:226 MAX_MAP_PORTALS | not checked by the engine (code/qcommon/cm_load.cpp / code/rd-vanilla/tr_bsp.cpp allocate each lump in its size) |
| bsp.lichtgitter | BSP lump | MAX_MAP_LIGHTGRID | 65535 | 65535 | ja | OpenJK code/qcommon/qfiles.h:228 MAX_MAP_LIGHTGRID<br>q3map2 tools/quake3/q3map2/bspfile_rbsp.cpp:80 MAX_MAP_GRID | code/rd-vanilla/tr_bsp.cpp R_LoadLightGridArray: indexes are unsigned short; q3map2 bspfile_rbsp.cpp:158 writes index 65535 (behind the array) when the grid is full |
| bsp.lichtgitterfeld | BSP lump | MAX_MAP_LIGHTGRID_ARRAY | 1048576 | 1048576 | weich | OpenJK code/qcommon/qfiles.h:229 MAX_MAP_LIGHTGRID_ARRAY<br>q3map2 tools/quake3/q3map2/q3map2.h:208 MAX_MAP_LIGHTGRID | the engine reads any size (code/rd-vanilla/tr_bsp.cpp R_LoadLightGridArray); q3map2 light.cpp:1749 raises gridsize until the array fits |
| bsp.areas | BSP lump | MAX_MAP_AREAS | 256 | 256 | ja | OpenJK code/qcommon/qfiles.h:218 MAX_MAP_AREAS<br>OpenJK code/qcommon/q_shared.h:228 MAX_MAP_AREA_BYTES<br>q3map2 tools/quake3/q3map2/q3map2.h:201 MAX_MAP_AREAS | code/qcommon/cm_test.cpp CM_WriteAreaBits: bit field of MAX_MAP_AREA_BYTES (32 bytes = 256 areas) |
| renderer.flaechenpunkte | renderer | SHADER_MAX_VERTEXES | 1000 | 999 | ja | OpenJK code/qcommon/qfiles.h:33 SHADER_MAX_VERTEXES<br>q3map2 tools/quake3/q3map2/games.cpp:756 | code/rd-vanilla/tr_bsp.cpp:555 ParseTriSurf, tr_surface.cpp:61 RB_CheckOverflow (Com_Error at numVerts >= SHADER_MAX_VERTEXES) |
| renderer.flaechenindizes | renderer | SHADER_MAX_INDEXES | 6000 | 5999 | ja | OpenJK code/qcommon/qfiles.h:34 SHADER_MAX_INDEXES | code/rd-vanilla/tr_surface.cpp RB_CheckOverflow (Com_Error at indexes >= SHADER_MAX_INDEXES) |
| renderer.lightmaps | renderer | MAX_LIGHTMAPS | 256 | 256 | ja | OpenJK code/rd-vanilla/tr_local.h:843 MAX_LIGHTMAPS | code/rd-vanilla/tr_bsp.cpp:250 R_LoadLightmaps writes tr.lightmaps[] without a check (array of MAX_LIGHTMAPS) |
| renderer.nebel | renderer | QSORT_FOGNUM_SHIFT | 30 | 30 | ja | OpenJK code/rd-vanilla/tr_local.h:874 QSORT_FOGNUM_SHIFT<br>OpenJK code/rd-vanilla/tr_local.h:875 QSORT_REFENTITYNUM_SHIFT<br>q3map2 tools/quake3/q3map2/q3map2.h:204 MAX_RBSP_FOGS | code/rd-vanilla/tr_main.cpp R_AddDrawSurf: the fog sits in bits 2..6 of the sort key; q3map2 writebsp.cpp:450 warns |
| renderer.patchgroesse | renderer | MAX_PATCH_SIZE | 32 | 31 | ja | OpenJK code/rd-vanilla/tr_local.h:592 MAX_PATCH_SIZE<br>q3map2 tools/quake3/q3map2/q3map2.h:124 MAX_PATCH_SIZE | q3map2 fog.cpp:128 Error "MAX_PATCH_SIZE after split"; NRC patchmodule.cpp:58 (31) |
| renderer.drawsurfs | renderer | MAX_DRAWSURFS | 65536 | 65536 | ja | OpenJK code/rd-vanilla/tr_local.h:847 MAX_DRAWSURFS | code/rd-vanilla/tr_main.cpp R_AddDrawSurf: index = numDrawSurfs & DRAWSURF_MASK (overwrites) |
| renderer.shader | renderer | MAX_SHADERS | 8192 | 8192 | ja | OpenJK code/rd-vanilla/tr_local.h:43 SHADERNUM_BITS<br>OpenJK code/rd-vanilla/tr_local.h:44 MAX_SHADERS | code/rd-vanilla/tr_shader.cpp (every loaded shader: world, models, effects, menus) |
| renderer.skins | renderer | MAX_SKINS | 512 | 512 | ja | OpenJK code/rd-vanilla/tr_local.h:844 MAX_SKINS | code/rd-vanilla/tr_image.cpp RE_RegisterSkin |
| kollision.submodelle | collision | MAX_SUBMODELS | 512 | 511 | ja | OpenJK code/qcommon/q_shared.h:642 MAX_SUBMODELS<br>OpenJK code/qcommon/cm_local.h:31 BOX_MODEL_HANDLE | code/qcommon/cm_load.cpp:143 Com_Error above MAX_SUBMODELS; the last slot is BOX_MODEL_HANDLE |
| kollision.patchebenen | collision | MAX_PATCH_PLANES | 4096 | 2048 | ja | OpenJK code/qcommon/cm_patch.h:65 MAX_PATCH_PLANES **offen** | code/qcommon/cm_patch.cpp:514 / :568 Com_Error when one more plane comes (numPlanes == MAX_PATCH_PLANES) |
| kollision.patchfacetten | collision | MAX_FACETS | 1024 | 1024 | ja | OpenJK code/qcommon/cm_patch.h:64 MAX_FACETS | code/qcommon/cm_patch.cpp:1094 Com_Error "MAX_FACETS" |
| kollision.patchpunkte | collision | MAX_PATCH_VERTS | 1024 | 1024 | ja | OpenJK code/qcommon/cm_load.cpp:527 MAX_PATCH_VERTS | code/qcommon/cm_load.cpp:564 Com_Error "ParseMesh: MAX_PATCH_VERTS" |
| kollision.weltgrenze | collision | MAX_MAP_BOUNDS | 65535 | 65534 | ja | OpenJK code/qcommon/cm_polylib.h:44 MAX_MAP_BOUNDS<br>OpenJK code/qcommon/q_shared.h:802 MAX_WORLD_COORD<br>q3map2 tools/quake3/common/maxworld.h:4 MAX_WORLD_COORD | code/qcommon/cm_patch.cpp CM_ValidateFacet: a facet with a bound >= MAX_MAP_BOUNDS is dropped |
| laufzeit.entitaeten | runtime | MAX_GENTITIES | 1024 | 1021 | ja | OpenJK code/qcommon/q_shared.h:621 GENTITYNUM_BITS<br>OpenJK code/qcommon/q_shared.h:618 MAX_CLIENTS | codeJK2/game/g_utils.cpp:802 G_Error "G_Spawn: no free entities" |
| laufzeit.modelle | runtime | MAX_MODELS | 256 | 255 | ja | OpenJK code/qcommon/q_shared.h:632 MAX_MODELS | codeJK2/game/g_utils.cpp:78 G_Error "G_FindConfigstringIndex: overflow" |
| laufzeit.klaenge | runtime | MAX_SOUNDS | 256 | 255 | ja | OpenJK code/qcommon/q_shared.h:635 MAX_SOUNDS | codeJK2/game/g_utils.cpp:78 G_Error "G_FindConfigstringIndex: overflow" |
| laufzeit.effekte | runtime | MAX_FX | 128 | 127 | ja | OpenJK code/qcommon/q_shared.h:644 MAX_FX | codeJK2/game/g_utils.cpp:78 G_Error "G_FindConfigstringIndex: overflow" |
| laufzeit.welteffekte | runtime | MAX_WORLD_FX | 4 | 3 | ja | OpenJK code/qcommon/q_shared.h:647 MAX_WORLD_FX | codeJK2/game/g_utils.cpp:78 G_Error "G_FindConfigstringIndex: overflow" |
| laufzeit.gamestate | runtime | MAX_GAMESTATE_CHARS | 16000 | 16000 | ja | OpenJK code/qcommon/q_shared.h:730 MAX_GAMESTATE_CHARS | client/cl_parse.cpp Com_Error "MAX_GAMESTATE_CHARS exceeded" (not in the source checkout SP\ojk\oj; the same check as MovieDuels code/client/cl_parse.cpp:457) |
| laufzeit.roffs | runtime | MAX_ROFFS | 128 | 128 | ja | OpenJK codeJK2/game/g_roff.h:34 MAX_ROFFS | codeJK2/game/g_roff.cpp:425 "MAX_ROFFS count exceeded. Skipping load" (the ROFF is missing) |
| spawn.variablen | spawn | MAX_SPAWN_VARS | 64 | 64 | ja | OpenJK codeJK2/game/g_local.h:176 MAX_SPAWN_VARS | codeJK2/game/g_spawn.cpp:1037 "G_ParseSpawnVars: MAX_SPAWN_VARS" |
| spawn.zeichen | spawn | MAX_SPAWN_VARS_CHARS | 2048 | 2048 | ja | OpenJK codeJK2/game/g_local.h:177 MAX_SPAWN_VARS_CHARS | codeJK2/game/g_spawn.cpp:970 "G_AddSpawnVarToken: MAX_SPAWN_VARS" |
| compiler.lichtschalter | compiler | MAX_SWITCHED_LIGHTS | 32 | 32 | ja | q3map2 tools/quake3/q3map2/q3map2.h:191 MAX_SWITCHED_LIGHTS | q3map2 writebsp.cpp:272 Error "MAX_SWITCHED_LIGHTS (32) exceeded" (SetLightStyles) |
| compiler.sicht | compiler | MAX_MAP_VISIBILITY | 33554440 | 33554440 | ja | q3map2 tools/quake3/q3map2/q3map2.h:145 VIS_HEADER_SIZE<br>q3map2 tools/quake3/q3map2/q3map2.h:209 MAX_MAP_VISCLUSTERS<br>q3map2 tools/quake3/q3map2/q3map2.h:210 MAX_MAP_VISIBILITY | q3map2 vis.cpp:733 Error "MAX_MAP_VISIBILITY exceeded" |


## Changelog

## English

- Map editor: new optimizer (Build > Optimizer...). It counts what the map uses against every limit of Jedi Academy (single and multiplayer), Movie Duels and Jedi Outcast - each limit taken from the engine's source code, with file and line - and shows a traffic light per limit; format limits the game does not check are only noted.
- Map editor: the optimizer finds build problems - leaks and detail brushes on the leak line, hidden faces not caulked, degenerate, micro and off-grid brushes, overlapping brushes, T-junctions, patches with too many collision planes, missing textures, brush entities without brushes, a full light grid, too many lightmaps, expensive VIS, spawn points in solid space and more - and fixes many of them at the push of a button, each as one undo step; "Fix all safe" applies only the harmless ones.
- Map editor: the optimizer also knows bugs of the tools and the game: a misc_model_static the game never draws (one click sets zoffset), common/origin brushes that put the entity at 0 0 0 (one click: system/origin), surface lights that shine through a floor (backsplash) and huge brushes; the build menu refuses external lightmaps that Jedi Outcast or light styles cannot use.
- Map editor: the optimizer runs in the background, even on the largest maps, and can run by itself after every build (new preference page "Optimizer").
- Map editor: the map compiler bhed_q3map2.exe is built with clang-cl: the same maps, light 14-21 % and vis up to 25 % faster.
- Guide: new page "Map editor: optimizer and limits" in four languages, with the limits of every game and their sources.

## Deutsch

- Karten-Editor: neuer Optimierer („Kompilieren > Optimierer...“). Er zählt, was die Karte verbraucht, gegen jede Grenze von Jedi Academy (Einzel- und Mehrspieler), Movie Duels und Jedi Outcast - jede Grenze aus dem Quelltext der Engine, mit Datei und Zeile - und zeigt je Grenze eine Ampel; Formatgrenzen, die das Spiel nicht prüft, werden nur als Hinweis genannt.
- Karten-Editor: der Optimierer findet Bauprobleme - Lecks und Detail-Pinsel an der Leck-Linie, nicht gecaulkte verdeckte Flächen, entartete, Mikro- und Pinsel neben dem Raster, überlappende Pinsel, T-Verbindungen, Patches mit zu vielen Kollisionsebenen, fehlende Texturen, Brush-Entities ohne Pinsel, ein volles Lichtgitter, zu viele Lightmaps, teures VIS, Spawnpunkte im Festen und mehr - und behebt viele davon auf Knopfdruck, jeweils als ein Rückgängig-Schritt; „Alles Sichere beheben“ nimmt nur die harmlosen.
- Karten-Editor: der Optimierer kennt auch Fehler der Werkzeuge und des Spiels: ein misc_model_static, das das Spiel nie zeichnet (ein Klick setzt zoffset), common/origin-Pinsel, die die Entity bei 0 0 0 landen lassen (ein Klick: system/origin), Flächenlicht, das durch einen Boden scheint (Backsplash), und riesige Pinsel; das Build-Menü verweigert externe Lightmaps, die Jedi Outcast oder Lichtstile nicht können.
- Karten-Editor: der Optimierer läuft im Hintergrund, auch bei den größten Karten, und auf Wunsch nach jedem Bau von selbst (neue Einstellungsseite „Optimierer“).
- Karten-Editor: der Karten-Compiler bhed_q3map2.exe wird mit clang-cl gebaut: dieselben Karten, Licht 14-21 % und VIS bis 25 % schneller.
- Handbuch: neue Seite „Karten-Editor: Optimierer und Grenzen“ in vier Sprachen, mit den Grenzen jedes Spiels und ihren Quellen.

## 中文

- 地图编辑器:新增优化器(“编译 > 优化器...”)。它把地图的用量与 Jedi Academy(单人和多人)、Movie Duels 和 Jedi Outcast 的每一项限制进行比较——每项限制都取自引擎源码,并注明文件和行号——并为每项限制显示信号灯;游戏不检查的格式限制只作提示。
- 地图编辑器:优化器能发现编译问题——泄漏及泄漏线上的 detail 笔刷、未用 caulk 处理的隐藏面、退化笔刷、微小笔刷和不在网格上的笔刷、重叠笔刷、T 形接合、碰撞平面过多的曲面片、缺失的纹理、没有笔刷的笔刷实体、光照网格已满、光照贴图过多、VIS 开销大、出生点位于实体内部等——并能一键修复其中许多问题,每次修复都是一个撤销步骤;“修复所有安全项”只应用无害的修复。
- 地图编辑器:优化器还能识别工具和游戏本身的已知问题:游戏永远不会绘制的 misc_model_static(一键设置 zoffset)、让实体落到 0 0 0 的 common/origin 笔刷(一键改为 system/origin)、透过地板照出的面光源(backsplash)以及巨大笔刷;编译菜单会拒绝 Jedi Outcast 或光照样式无法使用的外部光照贴图。
- 地图编辑器:优化器在后台运行,即使是最大的地图也可以;还可以在每次编译后自动运行(新的首选项页“优化器”)。
- 地图编辑器:地图编译器 bhed_q3map2.exe 改用 clang-cl 编译:生成的地图相同,光照快 14-21%,VIS 最多快 25%。
- 手册:新增四种语言的“地图编辑器:优化器与限制”页,列出每个游戏的限制及其来源。

## 日本語

- マップエディター:新しい最適化ツール(「ビルド > 最適化...」)。マップが使っている量を Jedi Academy(シングル/マルチプレイ)、Movie Duels、Jedi Outcast のすべての上限と比べます。上限はいずれもエンジンのソースコードから取り、ファイルと行を示します。上限ごとに信号で表示し、ゲームがチェックしない形式上の上限は注意として示すだけです。
- マップエディター:最適化ツールはビルド上の問題を見つけます。リークとリークライン上の detail ブラシ、caulk されていない隠れた面、縮退ブラシ・極小ブラシ・グリッド外のブラシ、重なったブラシ、T 字接合、衝突平面が多すぎるパッチ、見つからないテクスチャ、ブラシのないブラシエンティティ、いっぱいになったライトグリッド、多すぎるライトマップ、重い VIS、固体内のスポーン地点などです。その多くをボタン一つで修正でき、修正はそれぞれ 1 回の元に戻すステップになります。「安全なものをすべて修正」は無害な修正だけを適用します。
- マップエディター:最適化ツールはツールやゲーム自体の既知の不具合も見つけます。ゲームが描画しない misc_model_static(ワンクリックで zoffset を設定)、エンティティを 0 0 0 に置いてしまう common/origin ブラシ(ワンクリックで system/origin に)、床を通して漏れる面光源(backsplash)、巨大なブラシです。ビルドメニューは、Jedi Outcast やライトスタイルで使えない外部ライトマップを実行しません。
- マップエディター:最適化ツールはバックグラウンドで動き、最大級のマップでも使えます。ビルドのたびに自動で実行することもできます(新しい設定ページ「最適化」)。
- マップエディター:マップコンパイラー bhed_q3map2.exe を clang-cl でビルドするようにしました。出来上がるマップは同じで、ライトは 14~21%、VIS は最大 25% 速くなります。
- ガイド:新しいページ「マップエディター:最適化ツールと上限」(4 言語)。各ゲームの上限とその出典を載せています。
