# Notizen GDPFAD: JKA-Ordner "GameData" statt "GameData original", kein Absturz ohne JKA-Daten (11.11.2026, `Y:\p_gd`, Zweig `p_gd`, Basis karte1 852d12be7)

shank 11.11.: "ich habe ihn umbenannt zu gamedata so wie es im originalen war". Das Original-JKA des Rechners liegt wieder in
`C:\Program Files (x86)\Steam\steamapps\common\Jedi Academy\GameData\base` (assets0-3.pk3), daneben `GameData Movie Duels`.
Befund MBMAL am selben Tag: mit dem alten Ordnernamen als Vorgabe stuerzte die Gruppe rtjka ab (0xC0000005, Lesezugriff
0xFFFFFFFFFFFFFFF0, "waehrend: Programmzustand aufbauen"). Auftrag: Vorgaben umstellen, den Absturz erst rot nachstellen und
beheben, pruefen, was das Programm selbst ohne einen eingetragenen Spielordner tut, und die JKA-Gruppen mit dem neuen Ordner
gruen zeigen. Spielordner wurden nur gelesen (Dateiliste vorher/nachher unten).

## Punkt 1: Vorgaben "GameData original" -> "GameData"

- `gui/selbsttest.cpp`: `jkaBasis(variable)` ist die EINE Stelle mit dem Steam-Ordner. Daraus nehmen ihren Ordner:
  `mb1::spielSetzen` (BHED_MB_JKA; rtjka und jede Gruppe des Modell-Blocks mit BHED_MB_SPIEL=jka), `kemd::jaBasis`
  (BHED_KEMD_JA), mission1, missionsliste, zeitleiste0, zeitleiste1 (BHED_JKA_BASIS). Vorher stand der Pfad sechsmal da.
- `tools/karte/q3map2_funde.py` (BHED_JA_PFAD), `tools/karte/tempo.py`: neue Vorgabe.
- Kommentare und Aufrufbeispiele, die sagen, woher Daten kommen: `gui/app_deckung.cpp`, `gui/selbsttest_mission1.inc`,
  `gui/selbsttest_programm.inc`, `tests/rig_handtabelle.inc`, `src/modell/rig_vorlagen.inc` und
  `tools/modell/rig_vorlagen_gen.py`, `tools/karte_dekomp.py`, `tools/gen_karte_vergleich.py`.
- `tools/testrunde.py` kennt weiter beide Namen und nimmt den Ordner, den es gibt (TR_STEAM fuer andere Rechner).
- Bewusst nicht geaendert:
  - alle aelteren `docs/intern/NOTIZEN_*` (Verlauf: so hiess der Ordner damals);
  - `tests/karte/q3map2/fortschritt_licht.bin`, `_lichtA.bin`, `_lichtB.bin`, `_meta.bin`, `_vis.bin`: aufgezeichnete
    Stroeme echter q3map2-Laeufe. Der alte Pfad steht dort in der Zeile "VFS Init"; `tests/kartebau.cpp`
    (fortschrittPruefen) liest aus den Stroemen nur Phasen, Pacifier und Zeiten, den Pfad prueft nichts. Jede Nachricht
    traegt ihre Laenge (4 Byte, `BauNetzRahmen`), die `.zeit`-Dateien nennen Byte-Stellen - ein Ersetzen waere ein Eingriff
    in eine Aufzeichnung ohne Nutzen.
- `git grep "GameData original"` ausserhalb der Notizen: nur noch die zwei Kommentare, die die Umbenennung erklaeren
  (`gui/selbsttest.cpp`, `tools/testrunde.py`).

## Punkt 2: der Absturz - erst rot

Rotlaeufe mit der exe der Basis 852d12be7 (`Y:\werkzeuge\gd\rot.py`: `tools/testrunde.py`, aber BHED_MB_JKA, BHED_JKA_BASIS,
BHED_KEMD_JA und BHED_JA_PFAD zeigen auf `Y:\p_gd\build\laeufe\kein_jka`, einen Ordner, den es nie gab; kein Spielordner
wird angefasst). Ergebnis je Lauf:

| Lauf (JKA-Ordner fehlt) | Basis 852d12be7 | mit GDPFAD |
|---|---|---|
| `mb_rtjka` | **Absturz**, exit 3221225477 (0xC0000005), keine Zusammenfassung | gruen: 1 OK, 0 FEHLER, 1 uebersprungen (13 s; "9 Schritte ausgelassen") |
| `zeitleiste1` | **Absturz**, exit 3221225477, lesend an 0xFFFFFFFFFFFFFFE0 | gruen: 86 OK, 0 FEHLER, 1 uebersprungen (256 s) |
| `mb_bild_jka` | 1 OK, 1 FEHLER | gruen: 1 OK, 0 FEHLER, 1 uebersprungen (9 s) |
| `kemd` | 13 OK, 2 FEHLER (475 s: wartet 20000 Bilder auf ein Spielbild) | gruen: 7 OK, 0 FEHLER, 1 uebersprungen (78 s; "K2 md" und die Assets laufen) |
| `missionsliste` | 7 OK, 10 FEHLER | gruen: 1 OK, 0 FEHLER, 1 uebersprungen (5 s) |
| `zeitleiste0` | 4 OK, 5 FEHLER | gruen: 3 OK, 0 FEHLER, 1 uebersprungen (126 s; md_am_sith laeuft) |
| `mission1` | 93 OK, 10 FEHLER (2623 s; die Teile nach "jka" liefen auf academy1 aus dem Movie-Duels-Ordner weiter) | 55 OK, 1 FEHLER, 1 uebersprungen (970 s): md, drehen, undo am Schiff und mini laufen; der FEHLER ist die Gizmo-Pruefung der Basis im Teil mini (Schluss) - auf dem Bau davor 56 OK, 0 FEHLER |
| `mb_rtjka`, Ordner da, Figur fehlt (BHED_MB_JKA = Jedi Outcast) | **Absturz**, derselbe Lesezugriff 0xFFFFFFFFFFFFFFF0 | kein Absturz: 1 OK, 1 FEHLER ("wampa/model.glm: nicht in den Archiven"), exit 0, "6 Schritte ausgelassen" (8 s) |

### Ursache

Nicht das Programm, die Testschritte. Beide Abstuerze entstehen in Schritten, die nach einem gescheiterten Laden
weiterlaufen und aus einer leeren Liste lesen:

- rtjka: `mb1::laden` meldete "wampa/model.glm: nicht in den Archiven" und liess die Gruppe weiterlaufen. Der naechste
  Schritt ("RT: Bereich Animation, BOTH_STAND1", `rt5::figurLaden`) setzt `g_app->animFrame = f0()`; `kv5::seq()` ist
  `animList[animIndex]` ohne Pruefung - mit leerer Liste und animIndex -1 der Nullzeiger minus sizeof(AnimEntry) 48, das
  Feld firstFrame bei +32: Adresse -16 = 0xFFFFFFFFFFFFFFF0. Genau die gemeldete Adresse.
- zeitleiste1: ohne geladene Karte liest "die Vorschau laeuft den Weg wie das Spiel" `g_app->map.entities.back()` einer
  leeren Liste (0xFFFFFFFFFFFFFFE0).
- "waehrend: Programmzustand aufbauen" nennt nur den aeussersten noch offenen `diag::Step` (`gui/main_win32.cpp`), nicht den Ort.
- Das Programm selbst prueft den Index der Sequenz ueberall (`bildSchritt`, `bildRand`, `sequenz()` in den Bereichen des
  Model-Tabs); rund zwei Dutzend Teststellen lesen `animList[animIndex]` ungeprueft. Deshalb sitzt die Behebung am
  gemeinsamen Lader und nicht an der einen Stelle.

### Behebung (Testseite)

- `restAuslassen()` (`gui/selbsttest.cpp`): laesst die Schritte hinter dem laufenden weg, bis zu den Schlussschritten des
  Laufs (`endeSchritte` markiert sie mit `Schritt::schluss`); das Protokoll nennt die Zahl ("N Schritte ausgelassen").
- `mb1::laden` (der Figuren-Lader aller Gruppen des Modell-Blocks, 41 Aufrufe): laedt die Figur nicht, bleibt es bei dem
  FEHLER und der Rest des Laufs bleibt weg.
- `spielBasisFehlt(ordner)` / `ohneSpieldaten(teil, grund)`: ein Teil, dem die Daten seines Spiels fehlen (kein
  assets0.pk3 im Ordner), meldet EINE Zeile "Knopftest uebersprungen (Daten fehlen): ..." - wie bei den entfernten
  MD-MP-Archiven (TD1, `meldeUebersprungen`): kein FEHLER, kein OK.
  - `mb1::spielSetzen`: fuer jka und jk2, danach `restAuslassen()` - deckt rtjka, bild, bild2, rundlauf, rigrundlauf,
    kurvenrundlauf, ikrundlauf, uvrundlauf, mdspiel und uvspiel ohne Namensliste.
  - kemd: der Teil "K2 ja"; mission1: jka, zwischen, undo (academy1), gizmo, laden, kino (md, drehen, undo am Schiff und
    mini laufen weiter); missionsliste: die ganze Gruppe; zeitleiste0 und zeitleiste1: der Teil mit academy1.
- `tools/testrunde.py`: die Zeile eines Laufs nennt die Zahl der uebersprungenen Pruefungen (", N uebersprungen (Daten
  fehlen)") - ein gruener Lauf, der nichts geprueft hat, faellt so auf.

## Punkt 2b: das Programm, wenn ein eingetragener Spielordner fehlt

Neuer Selbsttest `spielordnerfehlt` (`gui/selbsttest_gdpfad.inc`): die Einstellungen des Laufs nennen einen Ordner, den es
nicht gibt (im Laufordner, nie angelegt), danach dasselbe mit der Liste von Jedi Outcast.

**Befund vor der Aenderung** (Bau ohne die neuen Zeilen in rescanGamePaths; `Y:\werkzeuge\gd\b2_gd.out`, 23 OK, 5 FEHLER):

- **Kein Absturz.** Model-Tab (Figur aus den Archiven: "nicht in den Archiven", alle sechs Bereiche, Retargeting-Rollout,
  NPC-Browser, Files in .pk3), Mission (Load mission mit einem Archiv, das es nicht gibt: Meldung, keine Auswahl), Map Editor,
  Menu Editor, Events und das Fenster Game folders zeichnen weiter - je 9 bis 121 Bilder gezaehlt, ebenso unter den Regeln
  von Jedi Outcast. Die 23 OK sind diese Pruefungen.
- **Kein brauchbarer Hinweis** (die 5 FEHLER): die Statuszeile schwieg; im Protokoll stand "keine .pk3 in <ordner>". Das
  Fenster Game folders zeigte hinter dem Ordner "no archives found - is the path right?", Files in .pk3 sagte "No game folder
  added yet" - obwohl einer eingetragen war (Fotos `Y:\werkzeuge\gd\b2_gdpfad_spielordner.png`, `b2_gdpfad_pk3.png`).

**Aenderung:**

- `rescanGamePaths` (`gui/app_ablage.inc`, `spielordnerFehlendMerken`): merkt sich die geltenden Ordner, die keine
  Verzeichnisse sind (`App::spielordnerFehlend`), und nennt jeden einmal in der Statuszeile: "Game folder not found: <ordner> -
  check Edit > Game folders" (`Str::GdOrdnerFehlt`). Einmal, weil jede Stelle ohne Spielordner neu einliest (der Selbsttest
  liest viermal ein: eine Zeile). Wieder gemeldet wird ein Ordner erst, wenn es ihn zwischendurch gab oder er zwischendurch
  nicht eingetragen war; der Wechsel des Regelwerks (Liste Jedi Academy / Jedi Outcast) meldet nichts doppelt.
  Der Model-Tab zeigt unten die letzte Meldung des Modells statt der Statuszeile (`gui/app_anordnung.inc`); dieselbe Zeile
  steht darum auch dort (`mtab.meldung`).
- Fenster Game folders (`gui/app.cpp` drawGamePaths): "folder not found - was it renamed or moved?" (`Str::GdOrdnerWeg`)
  statt "no archives found - is the path right?", wenn es den Ordner nicht gibt. Ein vorhandener Ordner ohne Archive behaelt
  den alten Text.
- Files in .pk3 (`gui/app.cpp`): nennt den Ordner mit derselben Zeile wie die Statuszeile (umbrochen), statt "No game folder
  added yet"; ohne eingetragenen Ordner bleibt der alte Text.
- Texte in 6 Sprachen: `tools/i18n_gdpfad.py`. Handbuch (Einstellungen und Updates > Spielordner): ein Absatz in 6 Sprachen,
  kein neues Bild - `h_spielordner` zeigt den Normalfall.

**Danach** (Endstand, Lauf `gd_spielordnerfehlt`, `Y:\werkzeuge\gd\gd.out`): **30 OK, 0 FEHLER** (57 s). Dieselben Pruefungen,
dazu zwei neue (Meldung im Model-Tab, Model-Tab ohne Figur). Die Zeile im Protokoll: "Statuszeile: Game folder not found:
...\gdpfad_kein_spiel\base - check Edit > Game folders". Fotos angesehen (`Y:\werkzeuge\gd\gruen_*.png`, 2538 x 1472): Game
folders zeigt hinter dem Ordner gelb "folder not found - was it renamed or moved?", Files in .pk3 die gelbe Zeile statt "No
game folder added yet", der Model-Tab unten rechts dieselbe Zeile.

## Punkt 3: die JKA-Gruppen mit dem neuen Ordner

Laeufe auf dem Endstand NUR mit den Vorgaben des Programms (`Y:\werkzeuge\gd\vorgabe.py`: `tools/testrunde.py` ohne
BHED_JKA_BASIS, BHED_MB_JKA, BHED_KEMD_JA, BHED_JA_PFAD - die Runde setzt sie sonst selbst, und eine falsche Vorgabe fiele
nicht auf). Der Ordner im Protokoll: `C:\Program Files (x86)\Steam\steamapps\common\Jedi Academy\GameData\base`.

| Lauf | Ergebnis | Vergleich |
|---|---|---|
| `mb_rtjka` | 2 OK, 0 FEHLER (34 s) | Wampa: 13 Ketten, k 1,6748, Fussrutschen 11,1966 / 3,0456 / 0,2353 - Zahl fuer Zahl wie im Lauf vor rc611 (`rel611_mb_rtjka`) |
| `mb_bild_jka` (mb1a1-Bildvergleich) | 2 OK, 0 FEHLER (134 s) | 77 Modelle, gleich 77, abweichend 0, nicht ladbar 0 - wie NOTIZEN_mb1a1 |
| `kemd` | 13 OK, 0 FEHLER (161 s) | K2 md: 39 Shader, 8 / 8 ohne Bild; K2 ja: 39 Shader, nur im Editor ohne Bild 0, `textures/mp/s_squarelight` nicht in shaderlist.txt - wie NOTIZEN_rc609_kemd |
| `missionsliste` (selbsttest_programm.inc) | 12 OK, 0 FEHLER (126 s) | Vergleich mit findMissions 57 gleich, 0 verschieden |
| `zeitleiste0` | 5 OK, 0 FEHLER (323 s) | |
| `zeitleiste1` | 100 OK, **1 FEHLER** (dreimal: 333 s, 584 s, 452 s) | der FEHLER ist die Bildzeit-Pruefung "academy1_intro - Filmdurchgang ohne laengere Bilder" (Schluss); die 14 uebrigen Pruefungen an academy1 sind gruen (Gegenprobe mit dem Export, Untertitel, Aufnahme) |
| `mission1` | 98 OK, **1 FEHLER** (3030 s) | der FEHLER ist "Gizmo: Kamera ... nach Stop wie vorher" und steht so auch auf der Basis (unten); alle Teile mit dem reinen JKA (jka, zwischen, undo, gizmo, laden, kino mit 32 Karten) laufen |
| ctest `q3map2_funde` | Passed | nrc72 laeuft wieder (11 Flaechen, davon caulk 0), vorher still uebersprungen |

Dazu, weil der geaenderte Code sie beruehrt: `mb_funktionen` 62 OK, 0 FEHLER (mb1::laden, Schlussschritte), `jk2` 16 OK, 0
FEHLER (Wechsel des Regelwerks, Einlesen der Liste von Jedi Outcast), Modus 1 2636 OK, 0 FEHLER (wie die Basis; erster
Lauf mit 8 FEHLER aus einem verfehlten Klick, siehe Schluss).

Testdaten: `Y:\karte\laeufe\testdaten` (Vorgabe von `tools/testrunde.py`) wurde am 11.11. beim Aufraeumen entfernt; kemd lief
mit `TR_TESTDATEN=Y:\werkzeuge\testdaten` (von der Runde neu erzeugt: `bm_duel1.map`). Ein Zwischenlauf von kemd auf dem Bau
davor scheiterte im Teil "K2 md", weil der Ersatzordner waehrend des Laufs verschwand - nicht gewertet.

## Hilfsskripte in Y:\werkzeuge (nicht Teil des Repos)

Skripte abgeschlossener Auftraege gehoeren ihren Auftraegen und sind nicht geaendert. Den alten Ordner nennen noch
(durchsucht: `*.sh`, `*.py`, `*.ps1`, `*.bat` bis zwei Ebenen tief, ohne sp_af9c, sp_654alt, sp_654neu, pr316, g2c, llvm):

- beim Konfigurieren eines Baus, ersetzen den Namen selbst (harmlos): `ap2/konfig.py`, `ap3/konfig.py`, `mbn/konfig.py`,
  `zus/konfig.py`;
- Bau- und Messhelfer: `lcb2/neubau.sh`, `mess/bau_alt.sh`, `mess/bau_ziel.sh`;
- Such-, Mess- und Spielskripte alter Auftraege: `d1/scan2.py`, `d1/scan_deform.py`, `m4/m8/notc_suche.py`,
  `m4/ms6/aufbau6.py`, `m4/ms6/orig.py`, `m4/ms6/orig_akt.py`, `m4/ms6/orig_m6.py`, `mg2/p27e/spiel.py`,
  `mr/glm_karten.py`, `mr/glm_liste.py`, `mr/p4k/geteilt.py`, `mr/spiel_glm.py`, `p3/doppelt.py`, `p3/p27/spiel.py`,
  `r3/suche_shader.py`, `r6/lm_none_scan.py`, `r7/lmscan.py`, `rel611/b/hautskript.py`, `rel611/b/shadersuch.py`,
  `rel611/b/skinsuch.py`, `rel611/det.py`, `rel611/xw_suche.py`;
- Texte: `mbm/schluss_fuellen.py`, `mbm/uebergabe_11.py`, `gamedata_pfad.py`, `pause_mbm2.py`.

Die vier Messskripte fuer das Ruhefenster loesen den JKA-Ordner nicht auf, keins ist betroffen:

- `vis2s/messung_final.sh`, `vis3/messung_final.sh`: Karten aus `Y:\werkzeuge\vis2\karten`, `-fs_basepath` ist der leere
  Ordner `sp_af9c\vis2\leer` (`skripte/vl.py`).
- `lcb2/messung_final.sh`: nur Movie Duels (`mess4.py`: `-fs_basepath ...\GameData Movie Duels -fs_game MD`).
- `dx12/messung_final.sh`: Einstellungen `behaved.cfg.sicher` (Spielordner Movie Duels base und MD), Karte aus
  `sp_af9c\s9c\taris`.

## Nebenbei gefunden

- `python tools/modell/rig_vorlagen_gen.py` wuerde aus `src/modell/rig_vorlagen.inc` die zwei von Hand eingetragenen
  Vorlagen "Unreal Mannequin" und "100STYLE (BVH)" entfernen (95 Zeilen; der Kommentar dort sagt "Von Hand eingetragen").
  Die erzeugte Datei ist darum nur in der einen Kommentarzeile geaendert, der Generator in derselben Zeile. Nicht behoben.
- Der q3map2-Fund nrc72 (`tools/karte/q3map2_funde.py`) wurde seit der Umbenennung still uebersprungen ("ok nrc72:
  uebersprungen (kein Jedi-Academy-Ordner ...)"); mit der neuen Vorgabe laeuft er wieder: 11 Flaechen, davon caulk 0.

## Selbsttest

- Neu: Modus `spielordnerfehlt` (`gui/selbsttest_gdpfad.inc`, Lauf `spielordnerfehlt` in `tools/testrunde.py`, Bereich
  programm in `tools/test_betroffen.py`). Die Einstellungen des Laufs nennen einen Ordner im Laufordner, den es nie gibt.
- Geaendert: `gui/selbsttest.cpp` (jkaBasis, spielBasisFehlt, ohneSpieldaten, restAuslassen, Schritt::schluss),
  `selbsttest_vorschau_b.inc` (endeSchritte), `selbsttest_mb1a1.inc` (laden, spielSetzen), `selbsttest_kemd.inc`,
  `selbsttest_mission1.inc`, `selbsttest_programm.inc` (missionsliste), `selbsttest_zeitleiste0.inc`, `selbsttest_zeitleiste1.inc`.
- Laufskripte in `Y:\werkzeuge\gd\`: `rot.py` (JKA-Variablen auf einen Ordner, den es nicht gibt, oder mit GD_JKA_ORDNER auf
  einen anderen), `vorgabe.py` (ohne die vier JKA-Variablen: nur die Vorgaben des Programms), `schluss.sh`.

## Schluss

Schlussrunde des Zweigs (Regel vom 11.11.: Bau, ctest, eigener Selbsttest, die Gruppen des geaenderten Codes, Lints; die
volle Runde laeuft einmal auf dem zusammengefuehrten Stand). Skript `Y:\werkzeuge\gd\schluss.sh`, Ausgaben in
`Y:\werkzeuge\gd\` (`bau4.log`, `ctest.txt`, `vg.out`, `vg2.out`, `gd.out`, `ohne.out`, `ohnefigur.out`, `lint_neu\`).
Alle Laeufe unter Last (eigene Laeufe nebeneinander, dazu die volle Runde und Bauten anderer Zweige).

| Lauf | Ergebnis |
|---|---|
| Bau des Endstands (inkrementell, /W4 /WX; `bau4`) | exit 0, 0 Warnungen, 681 s (davor `bau1` Basis 2472 s, `bau2` 847 s: je 0 Warnungen) |
| ctest alle (`-j 16`) | **254 von 254**, 1484 s |
| `spielordnerfehlt` (eigener Selbsttest) | 30 OK, 0 FEHLER |
| JKA-Gruppen mit der Vorgabe "GameData" | Tabelle Punkt 3 |
| JKA-Gruppen ohne JKA-Ordner, rtjka ohne Figur | Tabelle Punkt 2 |
| `mb_funktionen`, `jk2` | 62 OK, 0 FEHLER; 16 OK, 0 FEHLER |
| Modus 1 | 2636 OK, 0 FEHLER (1113 s; `vg2_1`) - der erste Lauf neben ctest (`-j 16`) und fuenf weiteren eigenen Laeufen: 2631 OK, 8 FEHLER |
| Lints, alle 32 (`lint_neu`) gegen die Basis (`Y:\werkzeuge\runde\lint`, 852d12be7) | Rueckgabewerte gleich; Ausgaben gleich bis auf verschobene Zeilennummern in lint_fremdprogramm (`tools/test_betroffen.py`, `tools/testrunde.py`) und die Zahl der Felder in lint_gapp (610 / 1358 statt 609 / 1354: die zwei neuen Felder) |
| `gen_i18n.py`, `gen_hilfe_orte.py --check`, `gen_handbuch.py` | 6014 Texte x 6 Sprachen, es 0 / cs 0 noch englisch; Ortskatalog aktuell; 178 Seiten x 6 Sprachen (geaendert: `einstellungen.html` und `suche.js` je Sprache) |
| Spielordner vorher / nachher (`spielordner_liste.py`: Pfad, Groesse, Zeit) | 3835 Dateien in `Jedi Academy\GameData`, `GameData Movie Duels`, `Jedi Outcast\GameData`: Liste 17:32 und 20:16 gleich |

Rot, das nicht von GDPFAD kommt (belegt an der exe der Basis):

- **mission1, "Gizmo: Kamera - beim Abspielen kein stehengebliebener Gizmo, nach Stop wie vorher"** (Foto-Pruefung in
  `mission1GizmoSchritte`: Achsenfarben im Bild vor dem Abspielen und nach Stop; einmal im Teil gizmo an academy1, einmal im
  Teil mini). An academy1 auf dem Endstand 1303 -> 12 (voller Lauf und Teillauf `BHED_M1_TEIL="jka gizmo mini"`), auf der
  Basis genauso: im eigenen Rotlauf mit der exe von 852d12be7 1304 -> 12, im Lauf `tr_mission1` der vollen Runde auf dem
  zusammengefuehrten Stand (`Y:\p_zus\build\laeufe`: 97 OK, 2 FEHLER) 1303 -> 12. Im Teil mini schwankt der Wert nach Stop
  bei 1291 davor: 1364 und 1451 (gruen), 1691 (Basis), 1841 (Basis in der Runde, Endstand zweimal) - rot. GDPFAD beruehrt
  weder Gizmo noch Abspielen; die Pruefung gehoert dem zusammengefuehrten Stand.
- **zeitleiste1, "academy1_intro - Filmdurchgang ohne laengere Bilder"** (Bildzeit im Filmdurchgang hoechstens das 1,5-fache
  der Ruhe + 4 ms): dreimal rot, auch im ruhigsten Lauf (31,43 ms gegen 10,32 ms; unter Last 103 gegen 22 und 133 gegen 49).
  md_am_sith besteht dieselbe Pruefung. Gegenprobe mit der exe der Basis (`Y:\p_zus\build\bin\Release`, 852d12be7, Lauf
  `basis_zeitleiste1`): dieselbe Zeile rot, 33,32 ms gegen 15,59 ms, der Lauf 100 OK, 1 FEHLER wie der Endstand. GDPFAD
  aendert nichts, was je Bild laeuft; auch diese Pruefung gehoert dem zusammengefuehrten Stand (`docs/intern/tests_tempo.md`
  fuehrt zeitleiste1 mit je 1 FEHLER unter Last als Wackler - hier bleibt die Zeile auch im ruhigen Lauf rot).
- Modus 1, erster Lauf: "Klick Next: beim Loslassen nicht mehr unter der Maus" im Tutorial-Fenster und sieben Folgefehler
  der Tour - das Fenster wanderte unter Last noch (dieselbe Art Fehlklick nennt `tools/testrunde.py` bei Modus 1); die
  Wiederholung ist gruen.

Bewusst nicht gelaufen (volle Runde auf dem zusammengefuehrten Stand): karteneditor, menueeditor, Docks, die uebrigen Gruppen
des Modell-Blocks (ihr Lader `mb1::laden` aendert sich nur im Fehlerfall; `mb_funktionen` deckt den Normalfall), die
Rundlaeufe des Modell-Blocks mit BHED_MB_SPIEL=jka ausser dem Bildvergleich (derselbe `spielSetzen`), handbuch.

## Offen

- Der Hinweis erscheint, sobald die Spielordner gelesen werden (das Programm liest sie bei Bedarf, nicht beim Start): beim
  Oeffnen einer Mission, im Model-Tab, in Files in .pk3. Kein eigenes Fenster, kein Dialog.
- Geprueft auf Englisch; die Texte der anderen fuenf Sprachen ueber `lint_i18n` (Platzhalter, Vollstaendigkeit). In Files in .pk3
  wird die Zeile umbrochen, im Fenster Game folders rollt die Zeile waagrecht wie bisher.
- Die Testhelfer selbst sind nicht gehaertet: `kv5::seq()` und rund zwei Dutzend weitere Stellen lesen `animList[animIndex]`
  ungeprueft, zeitleiste1 `map.entities.back()`. Sie werden nur nicht mehr erreicht, wenn die Figur oder die Daten des Spiels
  fehlen. Fehlen einem Lauf die Daten von Movie Duels (die eingerichteten Ordner), bleibt es bei FEHLER-Zeilen.
- `tools/modell/rig_vorlagen_gen.py` kennt zwei Vorlagen der erzeugten Datei nicht (oben).
- Ausserhalb des Repos nennen den alten Ordner noch: die Hilfsskripte oben und `JKStudio_Uebergabe\00_LIES_MICH_ZUERST.md`
  (Abschnitt 2, Spielordner).
- Beim Einordnen in `karte1`: `include/bhed/i18n.h`, `src/i18n.cpp`, `handbuch/` und die Listen der i18n-Module
  (`tools/gen_i18n.py`, `tools/i18n_sprache_es.py`, `tools/i18n_sprache_cs.py`, `tools/handbuch/gen_handbuch.py`) neu erzeugen
  bzw. zusammenfuehren; `tools/testrunde.py` ist auch in p_test in Arbeit.

## Changelog

## English

- JK Studio no longer runs into nothing when a game folder from the settings is gone (renamed, moved, drive not connected): the status line names the folder once and points to Edit > Game folders, the Game folders window marks it with "folder not found - was it renamed or moved?", and "Files in .pk3" names it instead of saying that no game folder was added yet.

## Deutsch

- JK Studio läuft nicht mehr ins Leere, wenn es einen Spielordner aus den Einstellungen nicht mehr gibt (umbenannt, verschoben, Laufwerk nicht angeschlossen): Die Statuszeile nennt den Ordner einmal und verweist auf Bearbeiten > Spielordner, das Fenster Spielordner markiert ihn mit „Ordner nicht gefunden - umbenannt oder verschoben?“, und „Dateien in .pk3“ nennt ihn, statt zu sagen, es sei noch kein Spielordner hinzugefügt.

## 中文

- 当设置中的游戏目录已不存在（被重命名、被移动、驱动器未连接）时，JK Studio 不再无提示地找不到数据：状态栏会提示一次该目录并指向 编辑 > 游戏目录，“游戏目录”窗口会将其标记为“未找到文件夹——是否已重命名或移动？”，“.pk3 中的文件”也会列出该目录，而不再显示尚未添加游戏目录。

## 日本語

- 設定にあるゲームフォルダーが存在しなくなった場合（名前の変更、移動、ドライブ未接続）でも、JK Studio は黙って空振りしなくなりました。ステータス行がそのフォルダーを 1 回表示して 編集 > ゲームフォルダー を案内し、「ゲームフォルダー」ウィンドウには「フォルダーが見つかりません — 名前の変更または移動をしましたか？」と表示され、「.pk3 内のファイル」も「ゲームフォルダーが未追加」ではなくそのフォルダーを表示します。

## Español

- JK Studio ya no se queda sin datos sin avisar cuando una carpeta del juego de los ajustes ha desaparecido (renombrada, movida, unidad no conectada): la línea de estado nombra la carpeta una vez y remite a Edición > Carpetas del juego, la ventana Carpetas del juego la marca con "no se encontró la carpeta - ¿se renombró o se movió?", y "Archivos en .pk3" la nombra en lugar de decir que aún no se ha añadido ninguna carpeta del juego.

## Čeština

- JK Studio už nenaráží naprázdno, když složka hry z nastavení zmizela (přejmenovaná, přesunutá, disk není připojen): stavový řádek složku jednou uvede a odkáže na Úpravy > Složky hry, okno Složky hry ji označí textem „složka nenalezena - byla přejmenována nebo přesunuta?“ a „Soubory v .pk3“ ji uvede, místo aby tvrdilo, že zatím nebyla přidána žádná složka hry.
