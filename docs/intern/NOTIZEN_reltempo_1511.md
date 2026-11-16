# REL-TEMPO (15.11.2026): schnellere Releases, Delta-Updates, ein Starter als Download

Auftrag: `JKStudio_Uebergabe/auftraege/auftrag_reltempo.md`. Worktree `Y:\p_ap6`, Zweig `p_reltempo` auf karte1 67ffd68a7.
Skripte, Logs, Testserver-Daten: `Y:\werkzeuge\reltempo\` (Uebergabe `UEBERGABE.md`).

Herkunft (shank 14.11.): Idee "nur der neue Code" - ein Release soll nicht jedes Mal alles neu uebersetzen (rc612: 55-60 min Bau,
dann ueber 34 min Unit-Tests im Runner, mit den Zeitgrenzen-Anlaeufen 90 -> 150 -> 300 min). Und: "lassen wir die aktuelle exe einfach
weg in Zukunft und bieten eine zentrale an und der Rest wird automatisch geupdated".

Vorab, Auftrag des Hauptchats (eigener Commit vor den Themen): zwei ctests waren auf 67ffd68a7 rot und haetten den Runner gestoppt -
`karte_lizenz` (CW-IMPORT-Dateien ohne Herkunftskopf) und `testrunde_ports` (Gruppe cwimport in keinem ke_*-Teil); dazu `karte_grenzen`
nur im lokalen Bau (alte MD-Quellen eingestellt). Siehe Rot -> Gruen.

## 1. Uebersetzungs-Cache

### Was der Runner bis rc612 tat (gemessen)

- `cmake --build build --config Release --parallel`: MSBuild baut bis zu 4 Projekte gleichzeitig, aber im Visual-Studio-Generator
  uebersetzt cl die Dateien EINES Projekts nacheinander - kein /MP (`build/bhed_core.dir/Release/bhed_core.tlog/CL.command.1.tlog`:
  `/Zi ... /Fd"...BHED_CORE.PDB"`, keine MultiProcessorCompilation). bhed_core (der groesste Teil) lief also auf einem Kern.
- Jeder Lauf baut alles neu (frischer Ordner), auch wenn sich seit dem letzten nur eine Datei geaendert hat.

### Wahl: ccache 4.14.3 (statt sccache)

| | ccache 4.14.3 | sccache 0.19.0 |
|---|---|---|
| MSVC und clang-cl | ja | ja |
| ohne Praeprozessor bei Treffern (direct mode) | ja, auch fuer MSVC | nein, MSVC immer ueber den Praeprozessor |
| `__TIME__` im Quelltext | nicht aus dem Cache (Zeit gehoert zum Ergebnis), `__DATE__` je Tag | wird nicht erkannt (alte Uhrzeit im Programm) |
| MSBuild (VS-Generator) | ccache.exe als `cl.exe` (CLToolExe/CLToolPath) + MultiToolTask | gleich |
| Lizenz | GPL v3 - nur Werkzeug im Runner, nicht ausgeliefert | Apache 2.0 |

`gui/main_win32.cpp` und `gui/app_ablage.inc` schreiben `__DATE__ __TIME__` in den Absturzbericht, drei Dateien `__DATE__` in
Ueber-Texte - mit sccache stuende dort die Zeit des ersten Baus. Deshalb ccache. Fest Fassung 4.14.3, SHA-256
`c845c3d65be81954c75659145a522a793a27586bf940c5dcfe18af33fa1a67dd` (im Workflow geprueft).

### Was sich aendert (CMakeLists.txt, `-DBEHAVED_CCACHE=<ccache.exe>`; leer = alles wie bisher)

- **/Z7 statt /Zi** fuer behaved, bhed_core, imgui: mit /Zi schreiben alle cl eines Projekts in EINE Compiler-PDB (mspdbsrv) - das kann
  kein Cache liefern. Mit /Z7 stehen die Debug-Infos in der .obj, die PDB des Programms (`jkstudio.pdb`) schreibt wie bisher der Linker
  (`/DEBUG /OPT:REF /OPT:ICF`). Vergleich unten.
- **ccache als `cl.exe`** (`<bau>/ccache_cl/cl.exe`, CMAKE_VS_GLOBALS `CLToolExe`/`CLToolPath`): ccache findet das echte cl im PATH,
  den MSBuild fuer die Werkzeuge setzt (Protokoll: `Compiler: ...\MSVC\14.50.35717\bin\HostX64\x64\cl.exe`).
- **UseMultiToolTask + EnforceProcessCountAcrossBuilds**: jede Datei ein eigener cl-Aufruf (ccache kann keine Buendel), parallel wie
  /MP, zusammen hoechstens so viele wie Kerne - auch im Runner ohne Cache-Treffer schneller als bisher.
- **TrackFileAccess=false**: mit dem FileTracker verbucht MSBuild die Dateien, die ccache in seinen Cache schreibt, als Ausgaben des
  Projekts; ein Neubau ("Clean") loeschte sie wieder - gemessen: jeder Neubau ein Fehlgriff (`ccache_probe.log`). Ohne Tracker
  uebersetzt MSBuild bei jedem Bau jede Datei - ueber den Cache, also schnell und nie veraltet (geprueft: drei Bauten hintereinander,
  8/8 Treffer je Bau).
- **clang-cl (bhed_q3map2)**: der eigene Ninja-Bau (tools/karte/q3map2_clang.cmake) bekommt `-DBEHAVED_CCACHE` weitergereicht und setzt
  dort CMAKE_C/CXX_COMPILER_LAUNCHER. Die Abhaengigkeiten kommen dort ueber `/showIncludes` (CMAKE_DEPFILE_FLAGS, Format msvc) statt
  `-clang:-MD -clang:-MF` - die kennt ccache nicht (gemessen: alle 429 clang-cl-Aufrufe "unsupported compiler option").
- **assimp** (beide Kopien, MSVC und clang-cl): mit Cache ASSIMP_INSTALL_PDB OFF und /Z7 - sonst setzt assimp selbst /Zi (gemessen: 205
  MSVC-Aufrufe ungecacht).
- **ccache `msvc_utf8=false`** (im Workflow und hier): ccache haengt beim Vorverarbeiten sonst `/utf-8` an, assimp hat
  `/source-charset:utf-8` -> cl bricht ab (D8016), jeder assimp-Aufruf "preprocessor_error". Ohne den Schalter liest cl die Quellen wie
  beim Uebersetzen; der Hash bleibt eindeutig (Quelle, Schalter, Kopfdateien).
- **fxc-Cache** (tools/karte/fxc_cache.cmake): die vorab uebersetzten GPU-Shader von bhed_q3map2 brauchten im Bau 3099 s Rechenzeit (eine
  Variante allein 324 s) - fxc ist kein Fall fuer ccache. Schluessel SHA-256 ueber fxc.exe, Schalter und .hlsl (ohne #include); Treffer
  kopiert den Header, den fxc bei genau diesen Eingaben geschrieben hat. Liegt unter `$CCACHE_DIR/bhed_fxc` und wandert mit dem
  ccache-Ordner in actions/cache; ci.yml raeumt nach dem Bau weg, was nicht gebraucht wurde. Innerer q3map2-Bau warm: 333 s -> 21 s.

### Runner (.github/actions/bauwerkzeuge, release.yml, ci.yml)

- Gemeinsame Aktion fuer CI und Release: LLVM 23.1.4 (wie bisher, jetzt auch in CI - CI baut damit dieselben Objektdateien wie das
  Release), ccache (fest, SHA-256), `actions/cache/restore@v6`.
- Schluessel: `ccache-<ImageOS>-vs<Visual Studio>-msvc<VCToolsVersion>-llvm23.1.4-cc4.14.3-<commit>`, wiederhergestellt der neueste mit
  demselben Anfang. ccache prueft jeden Treffer selbst (Compiler, Schalter, Quelle, Kopfdateien) - der Schluessel haelt nur fremde
  Werkzeugketten fern, damit der Cache nicht mit nutzlosen Eintraegen voll wird.
- Gespeichert wird nur von CI auf main (`actions/cache/save@v6`), vorher auf das gekuerzt, was dieser Bau benutzt hat
  (`ccache --evict-older-than`): Marken (Release-Laeufe) koennen Caches von main lesen, aber nur fuer sich selbst speichern. Ablauf:
  Push auf main -> CI baut und speichert -> Marke -> Release nimmt den Cache, uebersetzt nur, was seit dem letzten CI-Lauf neu ist.
- Groesse: CCACHE_MAXSIZE 4G; ein voller Bau braucht 0,62 GB (ccache gepackt, mit den fxc-Headern; unter der Grenze von 10 GB je Repository).
- Ohne ccache (Laden scheitert): Warnung, Bau wie bisher (/Zi).
- LLVM wird nicht gecacht (das installierte LLVM ist gross und wuerde den Platz des ccache-Caches nehmen); Laden und Installieren wie bisher.

### Messung (lokal, je `--parallel 2`, MultiToolTask zusammen hoechstens 2 cl - wie die Regel fuer Bauten hier; Leerlauf)

Richtwerte unter Last (eine Agentensitzung, daneben lief u. a. MovieDuels-SP und die Windows-Suche; fremde Bauprozesse 0, geprueft alle
30 s mit `Y:\werkzeuge\reltempo\fremdlast.ps1`). Endzahlen im Ruhefenster: `sh /y/werkzeuge/reltempo/messung_final.sh` (~2 h 15 min:
ohne Cache, kalt, warm, klein, A/B assimp x2 -> `Y:\werkzeuge\reltempo\messung_final.txt`).

| Bau (frischer Ordner, alle Ziele) | t0 ohne Cache (/Zi, wie der Runner bis rc612) | Lauf 1 (15.11.) | Lauf 2 (16.11. frueh) | **Lauf 3 (16.11., alles im Cache)** |
|---|---|---|---|---|
| kalt (leerer Cache) | 2171 s | 1888 s | 2189 s | 2998 s |
| warm (voller Cache, z. B. Release nach CI) | - | 487 s | 597 s | **179 s** |
| klein (eine .cpp geaendert) | - | 493 s | 600 s | **172 s** |
| ueber den Cache | - | 703 von 1337 | 703 von 1337 | **1337 von 1337** |
| Cache | - | 0,47 GB | 0,47 GB | 0,62 GB |

- Lauf 1 und 2 (gleiche Einstellung - die assimp-Behebung griff erst mit msvc_utf8=false): 634 Aufrufe ungecacht (assimp /Zi bzw.
  preprocessor_error, alle clang-cl), dazu die GPU-Shader. Lauf 3 mit allen Behebungen: warm 179 s statt 2171 s ohne Cache (-92 %),
  eine geaenderte .cpp 172 s (3 Fehlgriffe: update.cpp in bhed_core und im Starter, dazu die Datei mit `__TIME__`).
- Kalt schwankt stark mit der Last (gleiche Einstellung 1888 s und 2189 s; Lauf 3 2998 s). Was ein Fehlgriff wirklich kostet, zeigt
  der A/B im selben Fenster (`Y:\werkzeuge\reltempo\ab_assimp.ps1`, Ziel assimp, 220 Dateien, 2 cl, je leerer Cache):

  | assimp | Zeit |
  |---|---|
  | Cache aus (CCACHE_DISABLE, sonst gleich) | 185 s |
  | Vorgabe, kalt (220 Fehlgriffe) | 226 s (+22 %) |
  | Vorgabe, warm (220 Treffer) | 18 s (-90 %) |
  | depend_mode, kalt | 232 s |
  | depend_mode, warm | 20 s |

  Ein Fehlgriff kostet also gut ein Fuenftel mehr als ohne Cache (Hashen, Vorverarbeiten, gepackt ablegen); depend_mode (ohne
  Praeprozessor) bringt hier nichts und verliert die Rueckfallebene des Praeprozessor-Modus - darum bleibt die Vorgabe.
- Fuer den Runner heisst das: ein kalter Bau (neues Abbild mit anderem MSVC, Cache verdraengt) kostet je Datei ~20 % mehr, gewinnt aber
  durch MultiToolTask (bis rc612 uebersetzte cl die Dateien eines Projekts nacheinander, bhed_core auf einem Kern). Jeder weitere Lauf
  uebersetzt nur, was sich geaendert hat. Die echten Runner-Zeiten zeigt erst ein Lauf (siehe unten).

### Gleiches Ergebnis (/Zi gegen /Z7 mit ccache, beide /Brepro)

Kalter Bau aus Lauf 3 (ccache, /Z7, MultiToolTask) gegen t0 (ohne Cache, /Zi), beide /Brepro, gleiche Quellen
(`Y:\werkzeuge\reltempo\pe_vergleich.py`, `pdb_pruef.py`, `pdb_namen.py`):

| Datei | Ergebnis |
|---|---|
| jkstudio.exe (62 801 408 Bytes) | gleiche Groesse und Abschnitte; 74 Bytes in 9 Bereichen anders, alle erklaert: Debug-Eintraege (CodeView-GUID 18, Repro-Hash 32), Zeitstempel von Debug-Verzeichnis (16), Export (4) und PE-Kopf (4) - mit /Brepro Hashes ueber den Inhalt einschliesslich der PDB-GUID |
| bhed.exe, updatetest.exe, JK-Studio-Starter.exe | 0 Bytes Unterschied |
| bhed_q3map2.exe (clang-cl, mit fxc-Cache) | mit gleichem SOURCE_DATE_EPOCH 0 Bytes Unterschied (ohne: `__DATE__`/`__TIME__` im Programm); die fxc-Header gleich (`diff -rq` leer) |
| jkstudio.pdb | passt zur exe (GUID, Alter), 1163 Stroeme wie t0, die Funktionen des Updaters drin; 113 605 Namen gemeinsam, 306 Typnamen nur in der /Zi-PDB (Typen aus assimp-Dateien, deren Code der Linker weglaesst - /Zi sammelt sie im Typ-Server, /Z7 bringt nur die der gelinkten Objekte) und je 2 Namen anonymer Namensraeume (`?A0x...`, Hash je Bau) |

MSVC /Brepro macht `__DATE__`/`__TIME__` fest ("Jan 01 2000", "00:00:00") - darum sind die Programme ohne SOURCE_DATE_EPOCH
vergleichbar; im echten Release (ohne /Brepro) stehen wie bisher Datum und Uhrzeit des Baus darin (ccache cacht keine Datei mit
`__TIME__`, `__DATE__` nur am selben Tag).

## 2. ctest parallel

### Messung (lokal, 67ffd68a7, Bau `Y:\p_ap6\build`)

| Lauf | Zeit | Ergebnis |
|---|---|---|
| seriell (wie der Runner bis rc612) | 555 s | 276/279, rot: karte_lizenz, karte_grenzen, testrunde_ports |
| -j 32 | 179 s | dieselben 3 rot |
| -j 32 --schedule-random | 126 s | dieselben 3 rot |
| -j 64 --schedule-random | 123 s | dieselben 3 rot |
| **nachher** (p_reltempo, -j 32, 16.11.) | 291 s | **285/285 gruen** (6 Tests mehr) |

Der Lauf nachher ist laenger, weil updater_uebergang jetzt auch den Updater von rc612 prueft (allein 75 s statt 44 s) und neben 31
anderen Tests auf der Platte wartet (187 s, der letzte Test des Laufs; er packt und entpackt je Fall ein Paket mit 6060 Dateien).
Sein COST steht darum jetzt bei 123 (72 x 75 / 44) statt 72.

Keine Stoerung durch Parallelitaet gefunden: dieselben drei Roten in jedem Lauf, seriell wie parallel - sie sind inhaltlich (siehe
Rot -> Gruen). Geprueft dazu im Quelltext: jeder ctest mit festem Namen unter %TEMP% hat einen eigenen Namen (kein Name zweimal; die
drei Programme, die mehrere ctests bedienen - defgpu_probe, presetbau, bhed_dxr_quelle - schreiben je Aufruf andere Dateien), die
Python-Tests mit Arbeitsordner bekommen ihn je Test aus CMake, Testserver und Ports nehmen Port 0 (frei vom System), Umgebungsvariablen
gelten je Prozess. Darum keine RESOURCE_LOCK und kein RUN_SERIAL - es gibt keine geteilte Ressource.

### Runner

- `ctest -j $env:NUMBER_OF_PROCESSORS` (4) in release.yml und ci.yml, `--timeout 900` (wie CI bisher; Tests mit eigener Grenze
  behalten sie).
- **COST** (CMakeLists.txt, Ende): die Sekunden des Runners bei rc612 fuer die langen Tests - ctest startet sie zuerst. Ohne COST kaemen
  die vier langen q3map2-Tests in der Reihenfolge der Liste zuletzt. Lokal mischt ctest die COST-Werte mit seinen eigenen Messungen
  (Testing/Temporary/CTestCostData.txt) - im Runner (frischer Ordner) gelten nur die COST-Werte.
- **q3map2_teilbau_geometrie** (2237 s, die Haelfte aller Unit-Tests im Runner) als vier Tests `_geometrie_0` bis `_3`, je eine Reihe von
  f) (`BHED_TEILBAU_REIHE`). Die Reihen hingen schon vorher nicht voneinander ab (jede baut ihren eigenen ersten Bau); zusammen dieselben
  52 Pruefzeilen wie der ganze Teil (`Y:\werkzeuge\reltempo\geo\geo.py`: ganz 93,8 s / 52 OK; Reihen 23,6 + 38,8 + 21,3 + 20,6 s / 18 +
  18 + 8 + 8 OK, Zeilen gleich). Kein Test faellt weg, keine Pruefung wird schwaecher.
- Verhaeltnis Runner zu lokal (rc612-Log gegen `ctest_seriell1`): die vier langen q3map2-Tests 19-44x (lokal rechnet die GPU, im
  Runner der Prozessor), alle anderen ~2x. Schaetzung fuer 4 Kerne (laengste zuerst, Tests einfaedig angenommen): seriell 4530 s ->
  -j4 2237 s (begrenzt durch geometrie) -> mit den vier Reihen ~1150 s (untere Schranke Summe/4 = 1133 s). Rechnen Teile im Runner auf
  allen Kernen, liegt die Zeit dazwischen - das zeigt nur ein echter Lauf.

## 3. Delta-Updates

### Format (src/update_delta.cpp, tools/release_paket.py)

- `JK-Studio-<rc>-manifest.txt`: `# JK Studio manifest 1`, `fassung rcNNN`, je Datei `<sha256> <groesse> <pfad>` (ohne `JK-Studio/`,
  sortiert). Strenger Leser: unbekannter Kopf, kaputte Zeile, `..`, absoluter Pfad, Rueckstrich, doppelter Pfad -> abgelehnt.
- `JK-Studio-<rc>-from-<alt>.delta`: ein Zip (der Leser des Updaters, src/pk3.cpp) mit `delta.txt` (`von`, `nach`, `weg <pfad>`) und
  `inhalt/<sha256>` fuer jeden Inhalt des neuen Pakets, den `<alt>` nicht hat - einmal, auch wenn ihn mehrere Dateien haben
  (jkstudio.exe = behaved.exe). Die Endung ist **nicht .zip**: die Updater bis rc612 nehmen das ERSTE .zip des Releases (Rot-Probe
  `updater_uebergang --delta-als-zip`: dann nehmen rc604 und rc612 das Delta und "installieren" 0 Dateien).
- Das Paket traegt `JK-Studio/version.txt` ("rcNNN"): daran erkennt der Starter die installierte Fassung.
- Beide Dateien stehen in SHA256SUMS.txt - also wie das .zip geprueft und mit derselben Signatur (kSignaturSchluessel) gedeckt.

### Einspielen (bhed::update::einspielenDelta, aktualisieren)

1. Signatur ueber SHA256SUMS.txt (wenn ein Schluessel im Programm steht), Summe von Manifest und Delta.
2. Je Datei des Manifests (mit denselben Regeln wie das volle Paket: einspielenAls, ohne data\base nur das Programm): liegt sie schon so
   da (SHA-256), bleibt sie; sonst kommt ihr Inhalt aus dem Delta oder aus einer gleichen Datei im Ordner (verschoben, jkstudio.exe =
   behaved.exe). Jeder neue Inhalt wird gegen Summe und Groesse aus dem Manifest geprueft.
3. Fehlt ein Inhalt (eine Datei wurde von Hand geaendert) oder passt etwas nicht: nichts geaendert, und `aktualisieren` laedt das volle
   Paket wie bisher. Abbrechen: nichts geaendert, nichts weiter geladen.
4. Getauscht wird wie beim vollen Paket ueber den Zwischenordner (intern::tauschen, rc610 F78: Programm zuletzt, Rueckstellen); Dateien,
   die die neue Fassung nicht mehr hat (`weg`), gehen in den Zwischenordner und mit ihm weg. Neu: die Marke `fertig` im Zwischenordner -
   ohne neues Programm (oder beim ersten Start des Starters) gibt es kein `<exe>.alt`, an dem resteAufraeumen den fertigen Tausch erkennt.
- Das Programm (gui/update_win32.cpp installAuftrag) und der Starter nutzen denselben Ablauf `bhed::update::aktualisieren` mit eigener
  `Quelle` (Programm: holen mit Token-Weg und Selbsttest-Netz; Starter: WinHTTP direkt in die Datei; ctest: Speicher). Die Reihenfolge
  bleibt: erst die grosse Datei, dann SHA256SUMS.txt und Signatur (REST10: bricht man das Laden ab, kommt nichts weiter).
- Die Meldungen des Einspielens kommen ueber `meldungText` (Programm: i18n wie bisher, Starter: eigene Tabelle); die Summe des .zip wird
  jetzt in Stuecken gerechnet (das 470-MB-Paket lag dafuer bisher ganz im Speicher).

### Release (release.yml, Schritt "Deltas from the last releases")

- Je eine der letzten 3 veroeffentlichten Fassungen (`gh release list`, ohne Entwuerfe und Vorab-Releases), die ein Manifest hat:
  Delta bauen, mit `delta-pruefen` gegen beide Manifeste pruefen (jeder neue Inhalt im Delta oder schon in der alten Fassung, weg = alt
  ohne neu, keine unnoetigen Inhalte), nur behalten, wenn es deutlich (< 90 %) kleiner ist als das volle Paket.
- Fassungen ohne Manifest (bis rc612) bekommen kein Delta (16.11. geaendert; vorher wurde ihr Zip geladen und daraus das Manifest
  gebaut): ihr Updater nimmt das Zip, und der Starter kennt ihre Fassung nicht (keine version.txt) - ein Delta von ihnen luede niemand.
  Das spart im ersten Release 3 x 470 MB Download und drei Deltas. Das erste Release mit Starter hat also noch kein Delta, das
  naechste eines von ihm. `release_paket.py manifest --fassung` (Manifest aus einem alten Zip) bleibt fuer Messungen wie unten.
- Der Schritt endet ausdruecklich mit `exit 0`: `gh release download` ohne Treffer gibt 1 zurueck, und der Runner nimmt den letzten
  `$LASTEXITCODE` als Ergebnis des Schritts - beim ersten Release (keine Fassung mit Manifest) waere der Release-Lauf sonst hier
  gescheitert (Probe `Y:\werkzeuge\reltempo\exitcode_probe.ps1` mit der Schrittvorlage des Runners: ohne 1, mit 0).

### Messung (echte Pakete rc611 -> rc612)

| | Dateien | Bytes |
|---|---|---|
| rc612 ganz (ausgepackt) | 5980 | 586,5 MB |
| rc612 Zip (das volle Paket) | | **470,5 MB** |
| gleich wie in rc611 | 4183 | |
| neu / geaendert / weg | 390 / 1407 / 12 | |
| neue Inhalte (jkstudio.exe = behaved.exe einmal) | 1774 | 208,8 MB |
| `JK-Studio-rc612-from-rc611.delta` | | **137,4 MB (29 % des Zips)** |

Gebaut mit `release_paket.py manifest` (rc611-Zip, 4 s) und `release_paket.py delta` (28 s), `delta-pruefen` gegen beide Manifeste ok
(`Y:\werkzeuge\reltempo\delta_echt`). Groesster Teil: handbuch/bilder 104,6 MB und die Handbuchseiten je Sprache ~5,5 MB, dazu
jkstudio.exe 60,7 MB (ungepackt) und bhed_q3map2.exe 10,8 MB. Das Handbuch (Seite installation) nennt die Groessen so, wie der
Starter, der Updater im Programm und Windows sie zeigen (1 MB = 1024 x 1024 Bytes): Delta 131 MB statt 449 MB.

### Binaeres Delta fuer jkstudio.exe - nicht gebaut (Entscheidung)

Gemessen rc611 -> rc612: jkstudio.exe gepackt 21,9 MB, mit zstd --patch-from (Stufe 19, lange Fenster) 10,6 MB
(`Y:\werkzeuge\reltempo\delta_schaetzung.py`). Das waeren etwa 11 MB weniger in einem Delta von 137 MB - der groesste Teil
eines Deltas sind die Handbuchbilder (rc611 -> rc612: 139 MB von 209 MB geaenderten Bytes). Dafuer kaeme ein zweiter Decoder (zstd,
BSD/GPL v2 - vertraeglich) in den Weg, den jedes Update nimmt, und der Release-Lauf braeuchte die alten Programme. Lohnt sich, sobald die
Handbuchbilder zwischen den Fassungen gleich bleiben (dann ist das Programm der groesste Teil) - Vorschlag fuer spaeter.

## 4. Der Starter (starter/, JK-Studio-Starter.exe)

- **Ordner (wie heute):** neben dem Starter liegt jkstudio.exe -> dieser Ordner; sonst `JK-Studio` neben dem Starter (wie das entpackte
  Zip). Keine Administratorrechte (Manifest asInvoker); ist der Ordner nicht beschreibbar (Probedatei), sagt er es: Starter in einen
  eigenen Ordner legen (Rueckgabe 3 im Test).
- **Fassung:** `version.txt` im Ordner (kommt mit jedem Paket, auch ueber das Update des Programms). Ohne sie (Installation bis rc612)
  oder ohne jkstudio.exe: das volle Paket.
- **Ablauf:** GitHub /releases/latest (wie das Programm, bei 403/429 mit GH_TOKEN/GITHUB_TOKEN, falls gesetzt) -> neuer als installiert:
  `bhed::update::aktualisieren` mit Umfang::Alles (alles einspielen, auch in einen leeren Ordner) -> jkstudio.exe starten (Arbeitsordner
  = Programmordner) -> Ende. Gleich aktuell: sofort starten.
- **Fortschritt:** eigenes kleines Fenster (Win32, kein ImGui, kein comctl32 fest gebunden - der Balken ist selbst gezeichnet), Skalierung
  je Bildschirm (PerMonitorV2), Texte in 6 Sprachen nach der Sprache der Windows-Oberflaeche (`tools/gen_starter_texte.py` ->
  `starter/starter_texte.inc`; die Meldungen des Updaters wortgleich aus src/i18n.cpp, ctest starter_texte).
- **Abbruch:** "Abbrechen" (erster Start) bzw. "Ohne Update starten" (installiert): nichts geaendert; mit Installation startet sie.
- **Offline:** startet die installierte Fassung; ohne Installation Meldung mit "Noch einmal" / "Schliessen".
- **Klein:** 637 KB (651 776 Bytes) (Grenze 2 MB in release_paket.py pruefe --starter): nur der Updater (update*.cpp, Zip-Leser, inflate,
  json, diag) statt bhed_core (dessen Uebersetzungstabelle allein ueber 11 MB Objektdatei hat).
- **Geprueft:** jeder Download wie im Programm (SHA256SUMS.txt + Signatur). Der Starter selbst steht in SHA256SUMS.txt (signiert);
  Codesignatur gibt es keine (shank hat kein Zertifikat) - SmartScreen-Hinweis wie bisher.
- **Testserver:** nur per `BHED_UPDATE_API=http://127.0.0.1:<port>/repos/` (andere Adressen werden nicht angenommen), ab Werk
  unveraendert `https://api.github.com/repos/` - im Programm ebenso (apiBasis in gui/update_win32.cpp).

## Entscheidungen (shank: "mach die fragen so wie du sie denkst das sie richtig waeren")

1. ccache statt sccache - `__TIME__`/`__DATE__` (siehe oben), direct mode fuer MSVC.
2. /Z7 nur mit Cache (Option), lokal bleibt /Zi: die Bauordner hier sind inkrementell, und /Zi bleibt der bewaehrte Weg.
3. CI baut jetzt mit LLVM wie das Release: nur so liegen die clang-cl-Objekte von bhed_q3map2 im Cache, und CI prueft, was ausgeliefert wird.
4. Cache nur von main speichern, vorher kuerzen; Release liest nur.
5. Deltas von den letzten 3 Fassungen (Releases kommen oft, rc608-rc612 in 11 Tagen; jede aeltere laedt das volle Paket); nur, wenn < 90 %
   des Pakets, und nur von Fassungen mit Manifest (von rc612 und aelter luede kein Updater ein Delta).
6. Delta-Endung `.delta` - die alten Updater nehmen das erste .zip.
7. version.txt im Paket statt einer Datei, die der Starter selbst schreibt: so bleibt sie auch nach einem Update aus dem Programm richtig.
8. Starter-Ordner neben dem Starter (wie das entpackte Zip), keine Installation nach %LOCALAPPDATA%, kein Eintrag in Windows, keine Verknuepfung.
9. Die eigenstaendige .exe ist keine Datei des Releases mehr (shank: "lassen wir die aktuelle exe einfach weg"); ihr Test im Workflow
   bleibt (jkstudio.exe allein, --pruefe-daten) - alte eigenstaendige Installationen bekommen genau sie aus dem Zip.
10. Kein binaeres Delta (siehe oben).
11. q3map2_teilbau_geometrie in vier Reihen geteilt (dieselben 52 Pruefzeilen), damit -j im Runner wirkt.
12. ccache im Vorgabe-Modus (direct + Praeprozessor), kein depend_mode: im A/B nicht schneller (kalt 232 s gegen 226 s), aber ohne die
    Rueckfallebene des Praeprozessors.
13. Die GPU-Shader ueber einen eigenen kleinen Cache (tools/karte/fxc_cache.cmake, im ccache-Ordner) statt sie ins Repo zu legen:
    die Header bleiben Bauergebnis, und jede Aenderung an .hlsl, Schaltern oder fxc ergibt einen neuen Schluessel.

## Rot -> Gruen

- karte_lizenz: rot auf 67ffd68a7 (8 Fehlschlaege: cwimport.cpp, cwwerkzeuge.cpp, cwimport.h, cwwerkzeuge.h ohne Herkunft) -> gruen
  (Koepfe nach den Notizen CW-IMPORT: portiert sind nur die Regeln der Spielfigur aus OpenJK/Movie Duels bzw. die Clip-Spawnflags und
  SetBrushContents aus q3map2, der Rest ist eigener Code).
- testrunde_ports: rot ("Gruppe cwimport ... in keinem ke_*-Teil") -> gruen (cwimport in ke_karte nach cwpruef, wie im vollen Lauf).
- karte_grenzen: rot im lokalen Bau (BEHAVED_MD_QUELLEN = alte mdgit) -> gruen mit md9 (nur Einstellung).
- release.yml "Deltas from the last releases": rot (die Schrittvorlage des Runners mit drei Fassungen ohne Manifest endet mit 1 - der
  erste Release-Lauf mit Starter waere gescheitert) -> gruen mit `exit 0` (`Y:\werkzeuge\reltempo\exitcode_probe.ps1`, pwsh 7.6).
- Delta-Endung: `updater_uebergang --delta-als-zip` rot (15 Fehler: rc604 und rc612 nehmen das Delta), mit `.delta` gruen.
- ccache und FileTracker: rot (jeder Neubau 0/8 Treffer) -> gruen mit TrackFileAccess=false (8/8 Treffer).
- Schutz gegen das echte GitHub in den Testaufrufen (nach dem versehentlichen `update`-Lauf, siehe Funde): Programm und Starter nehmen
  ausser `http://127.0.0.1:` nur api.github.com - darum pruefen die Testtreiber selbst, bevor sie etwas starten
  (tools/karte/updater_gui_delta.py, tools/karte/starter_test.py, tools/handbuch/starter_bilder.py; der Selbsttest-Aufruf
  `Y:\werkzeuge\reltempo\lauf.py` ebenso fuer den Modus update). Probe `Y:\werkzeuge\reltempo\schutz_probe.py`: alte starter_bilder.py
  rot (laeuft weiter bis zum Kopieren des Starters), neue gruen (Abbruch vorher); geprobt ohne Starter, ohne Netz. lauf.py: Modus
  update ohne 127.0.0.1 -> Abbruch, nichts gestartet.
- Neue Funktionen bekamen ctests (Rot/Gruen gilt fuer Fehler): update (deltaproben), starter (A-I), starter_texte, starter_importe,
  release_paket (Manifest, Delta, Starter), updater_uebergang (rc612-Updater C/D).

## Tests

- **update** (tests/updatetest.cpp + updatetest_echtheit.cpp + neu updatetest_delta.cpp, `deltaproben`): Namen (rcVon, zipName,
  manifestName, deltaName), zipAsset nimmt bei Starter/Delta/Manifest das Zip, parseManifest/parseDeltaKopf (gut und 7 Abweisungen),
  sha256Datei, einspielenDelta echt mit Testschluessel (genaues Bild der neuen Fassung, verschobene Datei aus dem Ordner, weggefallene
  Datei weg), noch einmal = nichts zu tun, Ablehnungen ohne Aenderung (Summe, nicht gelistet, ohne SUMS, ohne Signatur, falsche Fassung,
  falscher Inhalt im Delta, von Hand geaenderte Datei), Tausch gesperrt -> alles zurueck, mitten im Tausch beendet -> resteAufraeumen,
  Marke fertig, eigenstaendige .exe, Umfang::Alles in einen leeren Ordner, aktualisieren (nur Delta/Manifest/SUMS; Delta kaputt -> Zip;
  kein Delta -> Zip vor SUMS; Fassung unbekannt -> Zip; Abbrechen beim Delta -> nichts weiter). Dazu unveraendert alle Proben der Stufe
  8d (das Einspielen ist umgebaut, die Proben blieben gruen).
- **starter** (tools/karte/starter_test.py, neu): A-I gegen den lokalen Testserver (siehe Abschnitt 4), Groesse.
- **starter_texte** (neu): starter/starter_texte.inc passt zu tools/gen_starter_texte.py und src/i18n.cpp.
- **starter_importe** (neu): JK-Studio-Starter.exe bindet nur Windows-DLLs (KERNEL32, USER32, GDI32, WINHTTP, bcrypt).
- **release_paket** (Selbsttest 63/63): version.txt im Zip und ihre Pruefung, Manifest (auch altes Paket mit --fassung), Delta
  (genau die neuen Inhalte, weg, keine .zip-Endung), delta-pruefen (gut und Abweisungen), pruefe --starter (gut, VC-Laufzeit, zu gross,
  falscher Name), Notizen mit dem Starter zuerst und ohne eigenstaendige exe.
- **updater_uebergang**: das neue Format (Starter, Delta, Manifest vor dem Zip, echte SHA256SUMS.txt); A/B rc604-Updater wie bisher,
  neu C/D mit dem Updater von rc612. Rot-Probe `--delta-als-zip`.
- **q3map2_teilbau_geometrie_0..3** statt q3map2_teilbau_geometrie (dieselben 52 Pruefzeilen).
- Selbsttest `updateleiste` (GUI-Weg des Updaters mit simuliertem Netz, volles Paket): alle Update-Pruefungen gruen; der eine FEHLER
  ist der Wettlauf aus "Funde" (auch auf 67ffd68a7).
- **Updater im Programm gegen den lokalen Server** (`tools/karte/updater_gui_delta.py`, Selbsttest `update` mit WinHTTP, je Fall eine
  eigene Kopie in `Y:\werkzeuge\reltempo\gui_delta`): Fall delta - geladen nur latest, .delta, Manifest, SHA256SUMS.txt (nie das Zip),
  "aus dem Delta, 4 Dateien ersetzt", jkstudio.exe/behaved.exe neu, version.txt da; Fall voll (ein Byte im Delta falsch) - "das Delta
  ging nicht (Pruefsumme ...) - das volle Paket", Zip geladen und eingespielt. 0 Fehler, je Knopftest 6 OK / 0 FEHLER.
- **Handbuchbilder** (`tools/handbuch/starter_bilder.py`): h_starter_laden und h_starter_delta in 6 Sprachen, 1944 Pixel breit, alle
  angesehen (Text vollstaendig, Zahlen im Format der Sprache, kein Pfad im Bild); `gen_handbuch.py --pruefen` ok.
- **Voller ctest -j32** auf dem Endstand: 285/285 gruen (siehe Abschnitt 2).
- Lints: lint_sources, lint_fassung, lint_i18n, lint_settings, lint_endtable, lint_mapstate, lint_cjkumbruch, lint_d3dbesitz ok;
  lint_portability 97 wie auf 67ffd68a7 (zwei neue Stellen behoben), lint_pruefcodes 11 wie vorher, lint_fremdprogramm 1 wie vorher.
- Workflows: `Y:\werkzeuge\reltempo\yaml_pruefen.py` (YAML, run/uses, Verweise auf frühere Stufen, Ausdruecke, jedes pwsh-Stueck mit dem
  PowerShell-Parser) 0 Fehler; Rot-Probe mit kaputtem Skript und falschem Stufennamen meldet beides.

## Was nur ein echter Runner-Lauf zeigen kann

- Ob ccache als `cl.exe` mit dem MSBuild des Runner-Abbilds so laeuft wie lokal (Visual Studio 18, MSVC 14.50 hier) und wie viele Treffer
  ein Release-Lauf nach dem CI-Lauf auf main bekommt; die echte Bauzeit (kalt mit MultiToolTask, warm, klein).
- Groesse des Caches im Runner und ob `actions/cache` (v6) Speichern/Wiederherstellen in der erwarteten Zeit schafft; ob Marken den Cache
  von main finden.
- Die echte Testzeit mit -j4 (geschaetzt 1150-2240 s statt 4530 s) - ob Teile im Runner alle Kerne belegen.
- `gh release list --exclude-drafts --exclude-pre-releases --json tagName` und `gh release download --pattern` im Runner (geladen
  werden nur die Manifeste der letzten Fassungen, je ~0,6 MB; das erste Release mit Starter findet keins und baut kein Delta).
- Dass GitHub die Dateien in der Liste mit dem Starter zuerst zeigt (die Reihenfolge des Uploads) - fuer die Updater egal (ein .zip).
- `releases/latest/download/JK-Studio-Starter.exe` als fester Link (Name ohne Fassung).
- Der Starter gegen das echte api.github.com und github.com-Downloads (Umleitung auf den Speicherort), mit Proxy; SmartScreen beim
  Herunterladen des Starters.
- Die Signatur, sobald RELEASE_SIGNING_KEY und kSignaturSchluessel gesetzt sind (lokal mit Testschluessel geprueft).
- Ob ccache im Runner dieselben Schalter so annimmt wie lokal (msvc_utf8=false, /showIncludes bei clang-cl): `ccache --show-stats` steht
  nach jedem Bau im Protokoll - "unsupported"/"preprocessor error" muessten dort 0 sein. Ob der fxc-Cache (`bhed_fxc`) mit dem
  ccache-Ordner gesichert und wiederhergestellt wird (Zeile "fxc cache: N shaders" im CI-Protokoll).
- Endzahlen der Bauzeiten lokal im Ruhefenster: `sh /y/werkzeuge/reltempo/messung_final.sh` (siehe Abschnitt 1).

## Funde (fuer den Hauptchat)

- CW-MODELL-Lizenz (nur gemeldet, nicht ausgeweitet): `src/modell/*` und `include/bhed/modell/*` prueft kein Herkunfts-Test
  (kartelizenz nimmt nur src/karte, include/bhed/karte, src/menue, include/bhed/menue); 20 von 29 .cpp unter src/modell haben in den
  ersten 12 Zeilen weder Herkunft noch Copyright. meshoptimizer steht in THIRD-PARTY.md und in den Lizenztexten des Pakets
  (release_paket LIZENZEN), aber nicht in der third_party-Liste von kartelizenz.
- Selbsttest `updateleiste`: "REST9 Map Editor wieder mit Leiste: 2 unruhige Bilder" ist ein Wettlauf im Test - der Karten-Editor laedt
  die Shader und Entity-Klassen von Movie Duels im Hintergrund, und wenn das genau im Ruhefenster fertig wird, zaehlen 2 Bilder als
  unruhig. Auf 67ffd68a7 (Bau Y:\p_ap9) 1 von 2 Laeufen rot, auf p_reltempo 3 von 3 (im Protokoll jedes Mal "254 entity classes" im
  Fenster); alle Update-Pruefungen der Leiste gruen. Nicht durch REL-TEMPO.
- Selbsttest `update` fragt das echte GitHub (BHED_UPDATE_LOKAL) - fuer den Updater-Test nach dem Veroeffentlichen gedacht; hier nicht
  gelaufen (einmal versehentlich gestartet: eine Abfrage /releases/latest, nichts geladen, sofort beendet).

## Changelog

### English

- Download and updates: one download, `JK-Studio-Starter.exe` (about 650 KB, a fixed name under `releases/latest/download/`). It puts
  JK Studio into a `JK-Studio` folder next to itself, checks the download, keeps it up to date and starts it - no administrator rights,
  nothing installed anywhere else; without a connection it starts the installed version. The standalone `JK-Studio-…-windows-x64.exe`
  is no longer part of a release; the zip stays as the complete program.
- Updates load only what has changed: each release carries, for the last three versions, a delta with only the files that version does
  not have yet, checked against `SHA256SUMS.txt` (and its signature) and file by file against the release's manifest. If it does not fit
  (a file was changed by hand) or there is none, the complete package is loaded as before. Older versions update as before.
- Guide: new page "Install and update" in six languages, "Getting started" points to the starter.
- Faster releases: the build goes through a compiler cache and compiles the files of a project in parallel; the unit tests run in
  parallel, the longest first, none left out.

### Deutsch

- Download und Updates: ein Download, `JK-Studio-Starter.exe` (etwa 650 KB, fester Name unter `releases/latest/download/`). Er legt
  JK Studio in einen Ordner `JK-Studio` neben sich, prüft den Download, hält es aktuell und startet es - ohne Administratorrechte, nichts
  wird anderswo installiert; ohne Verbindung startet er die installierte Version. Die eigenständige `JK-Studio-…-windows-x64.exe` gehört
  nicht mehr zum Release; das Zip bleibt als ganzes Programm.
- Updates laden nur, was sich geändert hat: jedes Release trägt für die letzten drei Versionen ein Delta mit nur den Dateien, die diese
  Version noch nicht hat, geprüft gegen `SHA256SUMS.txt` (und ihre Signatur) und Datei für Datei gegen das Manifest des Releases. Passt es
  nicht (eine Datei wurde von Hand geändert) oder gibt es keins, kommt wie bisher das ganze Paket. Ältere Versionen aktualisieren wie bisher.
- Handbuch: neue Seite "Installieren und aktualisieren" in sechs Sprachen, "Erste Schritte" zeigt auf den Starter.
- Schnellere Releases: der Bau geht über einen Übersetzungs-Cache und übersetzt die Dateien eines Projekts parallel; die Unit-Tests laufen
  parallel, die längsten zuerst, keiner fällt weg.

### 中文

- 下载与更新:只需一个下载 `JK-Studio-Starter.exe`(约 650 KB,`releases/latest/download/` 下的固定名称)。它把 JK Studio 放到自身旁边的
  `JK-Studio` 文件夹,校验下载、保持最新并启动程序——无需管理员权限,不会安装到其他位置;没有网络时启动已安装的版本。独立的
  `JK-Studio-…-windows-x64.exe` 不再随发布提供;zip 仍然是完整程序。
- 更新只下载有变化的部分:每个发布为最近三个版本各附带一个增量包,只含该版本还没有的文件,并根据 `SHA256SUMS.txt`(及其签名)以及发布的
  清单逐个文件校验。如果不匹配(某个文件被手动修改)或没有增量包,则像以前一样下载完整包。更旧的版本照旧更新。
- 手册:新页面“安装与更新”(六种语言),“快速上手”指向启动器。
- 更快的发布:构建经过编译缓存,并行编译项目中的文件;单元测试并行运行,最长的最先开始,一个都不省略。

### 日本語

- ダウンロードと更新: ダウンロードは `JK-Studio-Starter.exe` ひとつ(約 650 KB、`releases/latest/download/` の固定名)。JK Studio を自分の
  隣の `JK-Studio` フォルダーに入れ、ダウンロードを確認し、最新に保って起動します。管理者権限は不要で、ほかの場所には何もインストール
  しません。接続がなければインストール済みのバージョンを起動します。単体の `JK-Studio-…-windows-x64.exe` はリリースに含まれなくなり、
  zip は完全なプログラムとして残ります。
- 更新は変わった部分だけを読み込みます: 各リリースには直近 3 バージョン向けに、そのバージョンにまだないファイルだけの差分が付き、
  `SHA256SUMS.txt`(とその署名)およびリリースのマニフェストでファイルごとに確認されます。合わない場合(ファイルが手で変更された)や差分が
  ない場合は、これまでどおり完全なパッケージを読み込みます。古いバージョンはこれまでどおり更新されます。
- ガイド: 新しいページ「インストールと更新」(6 言語)。「最初の一歩」はスターターを案内します。
- リリースの高速化: ビルドはコンパイラキャッシュを通し、プロジェクト内のファイルを並列にコンパイルします。単体テストは並列に、長いものから
  実行し、省略するものはありません。

### Español

- Descarga y actualizaciones: una sola descarga, `JK-Studio-Starter.exe` (unos 650 KB, nombre fijo en `releases/latest/download/`).
  Pone JK Studio en una carpeta `JK-Studio` a su lado, comprueba la descarga, lo mantiene actualizado y lo inicia, sin derechos de
  administrador y sin instalar nada en otro sitio; sin conexión inicia la versión instalada. El `JK-Studio-…-windows-x64.exe`
  independiente ya no forma parte de una versión; el zip sigue siendo el programa completo.
- Las actualizaciones solo descargan lo que ha cambiado: cada versión lleva, para las tres últimas versiones, un delta con solo los
  archivos que esa versión aún no tiene, comprobado con `SHA256SUMS.txt` (y su firma) y archivo por archivo con el manifiesto de la
  versión. Si no encaja (un archivo se cambió a mano) o no hay ninguno, se descarga como antes el paquete completo. Las versiones más
  antiguas se actualizan como antes.
- Guía: nueva página "Instalar y actualizar" en seis idiomas; "Primeros pasos" remite al iniciador.
- Versiones más rápidas: la compilación usa una caché del compilador y compila en paralelo los archivos de cada proyecto; las pruebas
  unitarias se ejecutan en paralelo, las más largas primero, sin omitir ninguna.

### Čeština

- Stahování a aktualizace: jediné stažení, `JK-Studio-Starter.exe` (asi 650 KB, pevný název v `releases/latest/download/`). Uloží
  JK Studio do složky `JK-Studio` vedle sebe, zkontroluje stažení, udržuje ho aktuální a spustí ho - bez práv správce, nic se neinstaluje
  jinam; bez připojení spustí nainstalovanou verzi. Samostatný `JK-Studio-…-windows-x64.exe` už není součástí vydání; zip zůstává jako
  celý program.
- Aktualizace stahují jen to, co se změnilo: každé vydání nese pro poslední tři verze delta jen se soubory, které tato verze ještě nemá,
  ověřené proti `SHA256SUMS.txt` (a jeho podpisu) a soubor po souboru proti manifestu vydání. Když nesedí (soubor byl ručně změněn) nebo
  žádné není, stáhne se jako dosud celý balík. Starší verze se aktualizují jako dosud.
- Příručka: nová stránka "Instalace a aktualizace" v šesti jazycích, "Začínáme" odkazuje na spouštěč.
- Rychlejší vydání: sestavení jde přes mezipaměť překladače a soubory projektu překládá paralelně; jednotkové testy běží paralelně,
  nejdelší první, žádný nevypadne.
