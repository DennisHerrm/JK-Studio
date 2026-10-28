# Stufe 8d: Qualität und Sicherheit

Arbeitskopie `Y:\karte`, Zweig `karte1`, ab `c8626f0c` (Ende Stufe 8g).
Auftrag (Koordinator): das Release rundum professionell – Updater-Echtheit, statische Analyse,
0 Warnungen mit `/W4 /WX`, Fuzzer für alle Datei-Leser, Riesendateien aufteilen, restliche
Umbenennung, Projekt-Hygiene; dazu Punkt 1b (shank): **GPU-Backen Auto**. Grundregeln: Fehler
erst nachstellen, dann beheben, im selben Test bestätigen; was nichts Neues bringen soll, ist mit
den vorhandenen Tests als verhaltensgleich belegt. Keine Code-Signatur (Entscheidung shank).

## Commits

| Punkt | Commit | Inhalt |
|---|---|---|
| 1 | fac98119 | Updater prüft das .zip gegen SHA256SUMS.txt (BCrypt), optional ECDSA-P-256-Signatur; Entpacken in einen Zwischenordner, dann Tausch mit Rücknahme |
| 1b | 0e3ca755, ac6d1646 | Build > GPU bake: Auto / Ein / Aus (Vorgabe Auto), Wahl in bhed_q3map2, gelernte Zeiten je Karte |
| 7 | d4a63643 | interne Notizen nach `docs/intern/`, SECURITY.md, CONTRIBUTING.md, THIRD-PARTY.md, README (SmartScreen, Prüfsummen), CI-Workflow |
| 2 | aa8e36eb, 67eda767 | statische Analyse: clang-tidy + MSVC `/analyze`, Skript `tools/analyse/statische_analyse.py` |
| 3 | 2cc45725 | 130 → 0 Warnungen, danach `/WX` für den eigenen Code |
| 4 | 6bc7459d | Leser-Fuzzer (ASan/UBSan) und die Behebungen seiner Funde |
| 6 | 4ac10346 | `jkstudio.log`/`jkstudio-detail.log`, Logo auf der Startfläche, Programmname in Texten |
| 5 | 31bc1720 | selbsttest.cpp, app_view3d.cpp, app.cpp in Teile geschnitten (vorverarbeitet byte-gleich) |
| – | 60a7a603 | Doku: Fuzzing, Prüfsumme/Signatur in TESTING.md und SECURITY.md |
| 8 | 6b5fcb6c | Schlussrunde: Absturz von bhed_q3map2 im Fehlerweg mit GPU-Backen behoben |
| 9 | (Commit dieser Notizen) | Schlussrunde, Notizen, Changelog |

## Punkt 1: Updater-Echtheit

Bis rc605 lud der Updater per HTTPS und schützte nur gegen Zip-Slip (`zielImOrdner`); jede Datei
wurde sofort neben ihr Ziel geschrieben und getauscht. Jetzt (`src/update_einspielen.cpp`,
`bhed::update::einspielen`, von `gui/update_win32.cpp` nach dem Download aufgerufen):

1. **Prüfen** – `SHA256SUMS.txt` des Releases ist Pflicht; das .zip (die Bytes auf der Platte,
   die der pk3-Leser gleich öffnet) muss seiner Zeile gleichen (SHA-256 mit BCrypt). Steht in
   `include/bhed/update.h` ein `kSignaturSchluessel` (ECDSA P-256, `"04"+X+Y`), ist zusätzlich
   `SHA256SUMS.txt.sig` Pflicht (DER wie `openssl dgst -sha256 -sign`, BCryptVerifySignature).
   Im Repo ist der Schlüssel **leer** – bis der Koordinator ihn erzeugt (unten), gilt die
   Prüfsumme als Mindestanforderung.
2. **Entpacken** in den Zwischenordner `jkstudio-update.tmp` neben der .exe (derselbe
   Datenträger – der Tausch ist ein Umbenennen). Ein unlesbarer Eintrag: nichts geändert.
3. **Tauschen**: alte Datei in den Zwischenordner, neue an ihren Platz; die laufende .exe wird
   wie bisher `<exe>.alt`. Scheitert ein Tausch (Datei gesperrt), kommen alle schon getauschten
   Dateien zurück. Danach Zwischenordner weg; `beimStart` räumt einen Rest ab.

Neue Meldungen in vier Sprachen (`UpdErrNoSums`, `UpdErrNotListed`, `UpdErrChecksum`,
`UpdErrSignature`) – alle mit „nichts wurde geändert“.

Tests (ctest `update`, `tests/updatetest_echtheit.cpp`, 24 neue Prüfungen): SHA-256-Vektor,
sha256sum-Format (zwei Leerzeichen, `*`, CRLF), ein openssl-Signaturvektor (Wegwerf-Schlüssel,
privater Teil gelöscht) und ein BCrypt-Testschlüssel je Lauf; **Rot** mit ausgeschalteter
Prüfung: 6 FEHL (manipuliertes .zip, falsche/fehlende Summe, nicht gelistet, falsche/fehlende
Signatur wurden eingespielt) → **Grün** 0 FEHL. Jeder Ablehnungsfall vergleicht das Bild des
Programmordners vorher/nachher (gleich) und dass kein Zwischenordner bleibt; dazu „Summe passt,
4. Eintrag unlesbar“ und „Tausch scheitert an einer gesperrten Datei“ (alles zurück).
`updater_uebergang` (rc604-Updater holt das neue Paket) grün. `release_paket.py schluessel` macht
aus dem openssl-Schlüssel die Konstante (Selbsttest 50/50). `release.yml`: Schritt „Sign
checksums“ vorbereitet (Geheimnis `RELEASE_SIGNING_KEY`), `.sig` als viertes Asset.

Rest-Risiko (bewusst): ohne Schlüssel schützt die Summe gegen kaputte/abgefangene Downloads,
nicht gegen jemanden, der beide Release-Dateien ersetzen kann – dafür ist die Signatur da.

### Schlüssel und Signatur einrichten (Koordinator, einmalig)

Auf einem vertrauenswürdigen Rechner, nicht im Repo-Ordner (openssl aus Git for Windows):

```
openssl ecparam -name prime256v1 -genkey -noout -out jkstudio_release.pem
openssl ec -in jkstudio_release.pem -pubout -outform DER -out jkstudio_release_pub.der
python tools/release_paket.py schluessel jkstudio_release_pub.der      # 130 Hex-Zeichen
gh secret set RELEASE_SIGNING_KEY --repo DennisHerrm/JK-Studio < jkstudio_release.pem
```

1. `jkstudio_release.pem` offline sichern (Passwort-Manager), nie committen, danach vom Rechner
   löschen. Wer ihn hat, kann Updates signieren.
2. **Erst** das Geheimnis setzen und ein Release bauen: es trägt `SHA256SUMS.txt.sig` (der Lauf
   meldet „signing, but kSignaturSchluessel is empty“ – noch nicht geprüft). Prüfen:
   `openssl dgst -sha256 -verify pub.pem -signature SHA256SUMS.txt.sig SHA256SUMS.txt`.
3. **Dann** die 130 Hex-Zeichen in `kSignaturSchluessel` (`include/bhed/update.h`) eintragen,
   committen, nächstes Release. Ab dieser Fassung verlangt der Updater die Signatur; der
   Release-Lauf bricht ab, wenn der Schlüssel im Code steht, das Geheimnis aber fehlt, und
   prüft mit `release_paket.py schluessel --pruefe`, dass Geheimnis und Code zusammenpassen.
4. Schlüsselwechsel: das letzte mit dem alten Schlüssel signierte Release muss den neuen
   Schlüssel im Code tragen. Schlüssel verloren: ein Release ohne Signaturpflicht lässt sich
   dann nur noch von Hand installieren (die eingebauten Updater lehnen es ab).

## Punkt 1b: GPU-Backen Auto (shank)

- **Build > GPU bake** ist ein Untermenü **Auto / Ein / Aus**, Vorgabe Auto
  (`karte::kGpuBackenVorgabe`), Einstellung `BhedGpuBakeMode` (ein altes `BhedGpuBake=1` wird
  „Ein“, sonst Auto – bis 8g stand die Vorgabe 0 in jeder gespeicherten Einstellung).
- Die Wahl trifft bhed_q3map2 (`-bhedgpubakeauto`), denn erst nach `SetupSurfaceLightmaps`
  kennt es die Größe; Entscheidungstabelle `include/bhed/gpuwahl.h` (nur Kopfdatei, auch im
  Unit-Test): keine GPU → CPU; GPU ohne double → CPU (nicht bitgleich); beide Zeiten gemerkt →
  die schnellere; Super-Luxel × (1 + Bounce) < 400 000 → CPU; `-samples` ≥ 4 → CPU, außer die
  gemerkte CPU-Zeit ist ≥ 30 s und die GPU unbekannt (dann einmal messen); sonst GPU.
- Kalibriert an den 8g-Messungen; Super-Luxel gemessen: md_ch_ben 34 490, mp__duel1 101 149,
  md_am_sith 1 286 512, duel_mygeeto 1 393 026, kejim_post (-super 2) 6 086 248.
- Lernen: der Editor merkt je Karte und Licht-Schaltern die Dauer des letzten CPU- und
  GPU-Laufs (`karte_gpuzeiten.txt` im Einstellungsordner, höchstens 500 Einträge) und gibt sie
  mit `-bhedgpubakezeiten <cpu> <gpu>` mit; das Schnell-Backen ebenso.
- Konsole, Statuszeile und Tooltip nennen Wahl und Grund („GPU-Backen (Auto): Prozessor – wenig
  Lichtarbeit (34490 Super-Luxel)“).
- Tests: ctest `karte_gpuwahl` (Tabelle mit den Messkarten, Befehl, Schlüssel, Speicher,
  Ausgabezeile; Rot gefunden: doppeltes Anhängen des Schalters), `q3map2_gpubake` 7d (Auto klein →
  CPU, gemerkt GPU → GPU **bitgleich alle 18 Lumps**, gemerkt CPU → CPU, ohne double → CPU),
  Selbsttest `stufe8g` 41 OK (Auto lernt: erst CPU (small), nach Ein-Lauf GPU (learnedgpu),
  Lumps gleich dem CPU-Bau). Handbuchseite GPU-Backen in vier Sprachen mit neuen Bildern.

## Punkt 2: Statische Analyse

`tools/analyse/statische_analyse.py`: eigener Bauordner `build-analyse` (Ninja, cl, `/analyze`
für bhed_core und das Programm, `/analyze:external-`; Option `BEHAVED_ANALYSE`) und clang-tidy
(LLVM 20 aus Visual Studio) mit `tools/analyse/clang-tidy-8d.yaml` (bugprone, cert, performance,
ausgewählte modernize; Abschaltungen begründet) über jede eigene .cpp der Kompilierdatenbank.
Hingenommenes steht begründet in `tools/analyse/unterdrueckt.txt`; jeder andere Fund → Rückgabe
1. Optional als ctest `statische_analyse` (`-DBEHAVED_ANALYSE_CTEST=ON`).

Erster Lauf (ohne gui, weil `build-analyse` kein imgui kannte): 570 Funde. Zweiter Lauf mit
gui: **344 eigene .cpp, 1 265 Funde (27 von MSVC `/analyze`), 0 neu**. Durchgesehen; echte
Fehler im Programmcode hat die Analyse keine gezeigt – behoben: vier ungenutzte Variablen (eine schon im ersten Lauf) und
der Titel-Index einer Meldung (auf drei Einträge begrenzt). Hingenommen, je mit Grund: implizites
noexcept (bad_alloc), Index-Rechnung aus Dateiformaten (Sache der Fuzzer), Gleitkomma-Zähler und
Ganzzahl-Division wie in NRC/OpenJK, absichtliche Bitvergleiche, `rand()` mit festen Startwerten,
Leistungshinweise ohne Fehler, Win32-Casts (GetProcAddress, Griffe), gemappte
Konstantenpuffer. Die meisten Treffer der Speicherfehler-Art fanden erst die Fuzzer (Punkt 4).

## Punkt 3: Warnungen auf 0, dann /W4 /WX

130 → **0** (sauberer Neubau): 94 × C4996/STL4021 (`std::filesystem::u8path`) durch
`bhed::utf8Pfad` (`include/bhed/utf8pfad.h`, dieselbe Umwandlung über `std::u8string`); 36
Verdeckungen (C4456/C4457/C4458) im jeweiligen Gültigkeitsbereich umbenannt (Skript, das nur
Code außerhalb von Zeichenketten/Kommentaren ersetzt – ein Memberzugriff, den es mit erwischte,
fiel beim Bau auf und ist korrigiert); 3 × C4125 in `src/entdefs.cpp` (der Generator trennt die
Zeichenkette hinter einem Oktal-Escape vor einer Ziffer, Bytes gleich). Danach `/WX` für alles,
was bhed_core bindet (`BEHAVED_WARNUNGEN_FEHLER`, an); third_party, imgui und bhed_q3map2
ausgenommen. Der rc604-Code des Übergangstests behält seine Ausnahme für u8path.

## Punkt 4: Fuzzer für alle Datei-Leser

`tests/leserfuzz.cpp` (ctest `leser_fuzz`): je Leser 300 Mutanten (Bytes/Bits, Abschneiden,
Einfügen, Doppeln, 32-Bit-Grenzwerte, Syntaxzeichen, lange Wörter) für pk3/zip, md3, glm, gla,
skin, shader/fogParms, ICARUS .txt/.ibi, roff, npc, saber, weapons.dat, items.dat,
animation.cfg, efx, Bilder, Ton, inflate und die BSP-Lumps der Vorschau (Kartendaten, Geometrie,
Entities, Kollision). Saat aus dem Repo, mit `--spiel` je Art bis zu 12 echte Dateien aus den
assets*.pk3. Ein `.menu`-Leser existiert noch nicht (Menü-Editor folgt).

Läufe mit Jedi-Academy-Saat: **MSVC ASan** und **clang-cl UBSan** (Falle, ohne `object-size`:
Fehlalarme mit der MSVC-STL, wie schon in Stufe 6) je 6 900 Mutanten, die BSP-Leser und pk3 je
3 000, `kartefuzz` (2 000 Runden) unter beiden. Funde (alle nachgestellt, behoben, grün):

| Fund | Werkzeug | Behebung |
|---|---|---|
| Kollisionsleser las außerhalb: `ersteSeite + seiten` lief in int über, Karte ohne Ebenen | ASan | Bereiche in 64 Bit, ohne Ebenen keine Kollisionskarte |
| Patch-Gitter: Spaltenteilung schrieb über das 129er-Gitter (riesige Koordinaten) | ASan | Teilen endet am Gitterrand |
| Patch `width * height` (int-Überlauf) | UBSan | Prüfung in 64 Bit |
| Brush-Modell-Bereich ungeprüft (Spur bis 2^31 Runden) | Durchsicht | auf die Brushes begrenzt |
| Geometrieleser: Flächen, die alle Ecken nennen → 2,4 s / Gigabytes | Fuzzer (Hänger unter ASan 13–37 s) | Summe der numVerts ≤ 4 × Eckenzahl (alle 291 Spielkarten: genau 1 ×) → 0,01 s |
| Blatt mit numSurfaces bis 2^31 (Schleife) | Durchsicht | auf leafSurfaces begrenzt |
| Einfügen in eine Karte ohne Entitätsklassen: Entities zeigten in freigegebene Klassen | kartefuzz ASan (use-after-free) | Hilfskarte nutzt den Vorrat der Zielkarte |
| Patch-Kurven bildeten Zeiger zwei Zeilen hinter das Feld; Größe ≠ Punktzahl | UBSan-Spur | p2/p3 erst nach der Prüfung; solche Patches werden nicht tesseliert |

Werkzeug: `tests/absturz_melden.h` (Adresse bei einem Absturz/einer UBSan-Falle; mit `/MAP` und
einem Assembler-Listing mit `.cv_loc` der Zeile zugeordnet). ctest gesamt danach 136/136.

## Punkt 5: Riesendateien aufgeteilt

| Datei | vorher | nachher | Teile |
|---|---|---|---|
| gui/selbsttest.cpp | 44 267 | 6 319 | programm, vorschau_a, vorschau_b, werkzeuge, karte1bis2, karte3bis5, handbuch (2 700–6 600 Zeilen) |
| gui/app_view3d.cpp | 18 656 | 7 190 | app_view3d_spiel (Spiel-Nachbau), app_view3d_bedienung (Entity-Gruppen, Platzieren, Zeitleiste) |
| gui/app.cpp | 16 789 | 7 281 | app_anordnung (Undo-Liste, Symbole, Anordnung, Compile-Knopf), app_ablage (Zwischenablage, Berichte) |

Schnitte nur zwischen Abschnitten der obersten Ebene, als `.inc` an genau derselben Stelle
eingebunden (wie die vorhandenen `selbsttest_karte*.inc`). Begründung: die Teile teilen die
Helfer und den Zustand im anonymen Namensraum ihrer Datei; eigene Übersetzungseinheiten hätten
sie exportieren müssen – eine Logikänderung. Nachweis: die vorverarbeitete Ausgabe (`cl /EP`)
aller drei Dateien ist vorher/nachher **byte-gleich** (app.cpp nur `__TIME__`).

## Punkt 6: Umbenennung

`jkstudio.log` / `jkstudio-detail.log` (Werkzeuge lesen beide Namen); das Logo auf der
Startfläche der Vorschau (Bildlauf `hilfe` prüft es und nimmt `h_start` in vier Sprachen auf);
UI-Hinweise sagen „JK Studio“ statt „behaved“; Entwickler-Doku (BAUEN.md, TESTING.md) nennt
jkstudio.exe/jkstudio.log. Absturzabbild heißt weiter `behaved-absturz.dmp` (release.yml nennt
den Namen; nicht verlangt).

## Punkt 7: Projekt-Hygiene

Interne Notizen (NOTIZEN_*, HANDOUT-*, BEFUNDE-rc532, UEBERGABE, UMBAU-GPU, SCREENSHOTS-NEEDED)
nach `docs/intern/`, Verweise in Doku und Werkzeugen angepasst (Kommentare im Code nennen den
Dateinamen weiter). Neu: `SECURITY.md` (privater Meldeweg über GitHub-Advisories),
`CONTRIBUTING.md`, `THIRD-PARTY.md` (vollständig, mit Versionen und Programm); im Paket zusätzlich
die Lizenztexte von Dear ImGui (MIT) und minimp3 (CC0). README: SmartScreen erklärt, Prüfung mit
`Get-FileHash` gegen SHA256SUMS.txt. `.github/workflows/ci.yml` (Bauen + ctest ohne Spieldaten
bei Push/PR) angelegt, nicht gepusht.

## Punkt 8: Schlussrunde

Skript `SP/s8d/ende_s8d.sh` (nach 8g; Selbsttests über `SP/s8d/idle_jk.sh`, das den neuen
Protokollnamen liest; ein Prozess zur Zeit, Idle, 8-GB-Wächter), Summe `SP/s8d/ende_summe.txt`.

| Schritt | Ergebnis |
|---|---|
| Bau | ok |
| ctest gesamt | **136/136** (147 s); nach dem Fix unten noch einmal 136/136 |
| karteneditor voll (mit großer Karte) | 3139 OK, **4 FEHLER** → Befund unten → Wiederholung **3142 OK, 0 FEHLER** (922 s, Spitze 1034 MB) |
| Modus 1 | **2645 OK, 0 FEHLER** (433 s) |
| übrige 22 Modi | alle **0 FEHLER**, dieselben Zahlen wie 8g (karte 227, kartenfunde 53, fix604b 32, laserfalle 8, geschuetzanim 3, zielstrahl 2, effektgrund 1, figurdeform 1, jk2 15, tuerlauf 13, kamera 95, missionsbau 95, szenen 26, ton 34, npcklasse 12, gluehen 8, gras 13, spulen 13, figurstufen 8, missionsliste 11, rest605 6, modellimfesten 1) |
| Leistung gegen rc605 (md_am_sith, je 2 Läufe) | Vorbereitung rc605 6758/6470 ms, 8d 6797/6417 ms; Bild Mittel 16/8,7/11 gegen 16/8,5/12 ms, max. 221/184 gegen 209/192 ms – gleich |
| Kartenprüfung (278 MD-Karten, gegen 8g) | alle Summen gleich (0 Abstürze, 0 nicht geladen, 0 D3D-Meldungen, 1 → 1 nicht gezeichnete Entity) |
| sauberer Neubau | **0 Fehler, 0 Warnungen** (437 s; 8g: 130 Warnungen) |

Befund der Schlussrunde (karteneditor, behoben in 6b5fcb6c): mit GPU bake Auto als Vorgabe
bekommt jeder `-light`-Schritt `-bhedgpubakeauto`. „Single: light stage“ ohne .bsp ließ
bhed_q3map2 abstürzen (0xC0000409, keine Ausgabe): `Error()` endet mit `exit()`, und der noch
laufende Faden des Gerätestarts wurde als `std::thread` zerstört → `std::terminate`. Der Fehler
gab es seit 8g auch mit „Ein“ (`-bhedgpubake`), nur lief kein Test mit dem Schalter auf diesen
Fehlerweg. Jetzt wartet ein atexit-Haken auf den Faden; ctest `q3map2_gpubake` prüft beide
Schalter (rot → grün). Zwei Befehlsvergleiche des Selbsttests erwarten den Auto-Schalter.

## Offene Punkte

- **Signaturschlüssel**: erzeugt der Koordinator (Anleitung oben); bis dahin schützt nur die
  Prüfsumme. Danach muss jedes Release signiert sein.
- **Lokaler Test des neuen Updaters gegen einen Server**: `einspielen` ist vollständig ohne Netz
  geprüft; der Download von SHA256SUMS.txt/.sig in `installAuftrag` selbst läuft erst mit dem
  nächsten echten Release (Selbsttest-Modus `update`). Ein Testserver wie in
  `updater_uebergang.py` bräuchte eine einstellbare API-Adresse im Programm.
- **CI**: `.github/workflows/ci.yml` ist angelegt, aber noch nie auf GitHub gelaufen.
- **Statische Analyse**: 1 265 Funde begründet hingenommen – darunter Leistungshinweise in
  kalten Pfaden, die man nach und nach abbauen könnte (unterdrueckt.txt nennt sie gruppenweise).
- **`.menu`-Leser** gibt es noch nicht; der Fuzzer bekommt ihn mit dem Menü-Editor.
- **Aufteilen**: die neuen `.inc`-Teile sind keine eigenen Übersetzungseinheiten (Begründung
  oben); ein echter Schnitt in .cpp-Dateien müsste die Helfer im anonymen Namensraum in eigene
  Header ziehen – eine eigene Stufe.
- Das Absturzabbild heißt weiter `behaved-absturz.dmp`.
- GPU bake Auto: die Schwellen sind auf einer RTX 4070 + 32 Threads gemessen; auf schwächeren
  GPUs lernt Auto erst nach je einem CPU- und GPU-Lauf einer Karte.

## Release-Anleitung für den Koordinator

1. `kFassung` erhöhen, `AENDERUNGEN.md` und `tools/changelog.txt` (vier Sprachen, Text unten)
   ergänzen, `python tools/gen_changelog.py`, committen, nach `main`.
2. Signatur: falls noch nicht geschehen, erst das Geheimnis `RELEASE_SIGNING_KEY` setzen (oben,
   Schritte 1–2), den Schlüssel im Code erst im **folgenden** Release (Schritt 3).
3. Marke `v1.0.0-rcNNN` pushen; der Lauf baut, testet, packt, prüft, schreibt `SHA256SUMS.txt`
   (und mit Geheimnis `SHA256SUMS.txt.sig`) und legt den Entwurf an.
4. Artefakt laden, `SHA256SUMS.txt` gegen Zip und .exe prüfen (`Get-FileHash`), mit Signatur
   `openssl dgst -sha256 -verify pub.pem -signature SHA256SUMS.txt.sig SHA256SUMS.txt`;
   Selbsttest Modus 1 mit genau dieser .exe.
5. Veröffentlichen (`gh release edit v1.0.0-rcNNN --draft=false --latest`), **SHA256SUMS.txt nie
   löschen oder ändern** (sonst lehnen die neuen Updater das Release ab), kein zweites .zip.
6. Selbsttest-Modus `update` aus einer Kopie (BHED_UPDATE_LOKAL=rc500): jetzt mit Prüfung
   („Update: Pruefung von … echt (SHA-256)“ im Detailprotokoll `jkstudio-detail.log`).
7. Der Übergang alter Installationen (rc604/rc605) bleibt wie in 8c: `behaved.exe` im Paket
   lassen; die alten Updater prüfen nichts, das ist so gewollt.

## Changelog

## English

- The built-in updater checks every download before it changes anything: the zip must match the release's SHA256SUMS.txt (and, once enabled, a signature over it); it unpacks into a temporary folder first and only then swaps the files – if anything goes wrong, all old files stay or come back.
- Map editor: **Build → GPU bake** now has **Auto** (the new default), On and Off. Auto uses the graphics card only when its result is bit-identical and it is expected to be faster, remembers how long the processor and the graphics card took on each map and then takes the faster one; the console says what was chosen and why.
- Many robustness fixes for broken or hostile files found by fuzzing every file reader: map collision, curved patches, map geometry (a broken map could take seconds and gigabytes), and pasting into a map without entity definitions.
- The log files are now called `jkstudio.log` and `jkstudio-detail.log`; the start page of the preview shows the JK Studio logo.
- New: SECURITY.md (how to report a security problem), CONTRIBUTING.md and a complete list of third-party components; the README explains the SmartScreen warning and how to check a download with SHA256SUMS.txt.
- Under the hood: zero compiler warnings (now treated as errors), static analysis with clang-tidy and MSVC /analyze, and a build check on GitHub for every change.

## Deutsch

- Der eingebaute Updater prüft jeden Download, bevor er etwas ändert: das Zip muss zur SHA256SUMS.txt des Releases passen (und, sobald eingeschaltet, zu einer Signatur darüber); er entpackt erst in einen Zwischenordner und tauscht dann – geht etwas schief, bleiben alle alten Dateien oder kommen zurück.
- Karten-Editor: **Build → GPU-Backen** hat jetzt **Auto** (neue Vorgabe), Ein und Aus. Auto nimmt die Grafikkarte nur, wenn ihr Ergebnis bitgleich ist und sie voraussichtlich schneller ist, merkt sich je Karte, wie lange Prozessor und Grafikkarte gebraucht haben, und nimmt danach das Schnellere; die Konsole sagt, was gewählt wurde und warum.
- Viele Absicherungen gegen kaputte oder bösartige Dateien, gefunden durch Fuzzing aller Datei-Leser: Kollision der Karte, gebogene Patches, Kartengeometrie (eine kaputte Karte konnte Sekunden und Gigabytes kosten) und Einfügen in eine Karte ohne Entity-Definitionen.
- Die Protokolle heißen jetzt `jkstudio.log` und `jkstudio-detail.log`; die Startfläche der Vorschau zeigt das Logo von JK Studio.
- Neu: SECURITY.md (Meldeweg für Sicherheitslücken), CONTRIBUTING.md und eine vollständige Liste der Fremdbestandteile; das README erklärt die SmartScreen-Warnung und die Prüfung eines Downloads mit SHA256SUMS.txt.
- Unter der Haube: null Compiler-Warnungen (jetzt als Fehler behandelt), statische Analyse mit clang-tidy und MSVC /analyze und eine Bauprüfung auf GitHub für jede Änderung.

## 中文

- 内置更新程序在做任何更改之前都会校验下载:zip 必须与发布中的 SHA256SUMS.txt 相符(启用后还须通过其签名验证);它先解压到临时文件夹,再替换文件——一旦出错,所有旧文件都会保留或恢复。
- 地图编辑器:**Build → GPU 烘焙**现在有**自动**(新的默认值)、开和关。自动模式只在显卡结果逐位相同且预计更快时才使用显卡,并记住每张地图在处理器和显卡上各用了多久,之后选择更快的一方;控制台会显示选择了什么以及原因。
- 通过对所有文件读取器进行模糊测试,修复了许多针对损坏或恶意文件的问题:地图碰撞、曲面补丁、地图几何(损坏的地图可能耗费数秒和数 GB 内存),以及粘贴到没有实体定义的地图中。
- 日志文件现在名为 `jkstudio.log` 和 `jkstudio-detail.log`;预览的起始页显示 JK Studio 标志。
- 新增:SECURITY.md(安全问题报告方式)、CONTRIBUTING.md 以及完整的第三方组件列表;README 说明了 SmartScreen 警告以及如何用 SHA256SUMS.txt 校验下载。
- 内部:编译器警告为零(现在视为错误),使用 clang-tidy 和 MSVC /analyze 进行静态分析,并在 GitHub 上对每次更改进行构建检查。

## 日本語

- 内蔵アップデーターは何かを変更する前にダウンロードを検証します。zip はリリースの SHA256SUMS.txt と一致する必要があり(有効にした後はその署名も)、まず一時フォルダーに展開してからファイルを入れ替えます。問題が起きた場合、古いファイルはすべて残るか元に戻ります。
- マップエディター:**Build → GPU ベイク**に**自動**(新しい既定)、オン、オフが加わりました。自動はグラフィックカードの結果がビット単位で同一で、速いと見込まれる場合だけグラフィックカードを使い、マップごとにプロセッサーとグラフィックカードの所要時間を覚えて、以後は速い方を選びます。何を選んだか、その理由はコンソールに表示されます。
- すべてのファイル読み込みをファジングして見つけた、壊れたファイルや悪意あるファイルへの多くの対策:マップの衝突判定、曲面パッチ、マップのジオメトリ(壊れたマップで数秒と数 GB を要することがありました)、エンティティ定義のないマップへの貼り付け。
- ログファイルは `jkstudio.log` と `jkstudio-detail.log` になりました。プレビューのスタートページに JK Studio のロゴを表示します。
- 新規:SECURITY.md(セキュリティ問題の報告方法)、CONTRIBUTING.md、サードパーティ部品の完全な一覧。README で SmartScreen の警告と SHA256SUMS.txt によるダウンロードの確認方法を説明しています。
- 内部:コンパイラー警告ゼロ(現在はエラーとして扱います)、clang-tidy と MSVC /analyze による静的解析、変更ごとの GitHub 上でのビルド確認。
