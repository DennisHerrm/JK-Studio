# Notizen ZUS: Zweige des parallelen Fensters (07.11.) nachgetestet und in karte1 zusammengefuehrt

Stand 08.11.2026 frueh. Ein Agent (Auftrag `auftraege/auftrag_zusammen_0711.md`), Laeufe `zs_*`, Skripte und Protokolle unter
`Y:\werkzeuge\zs\`. Basis e6daa5ca8 (Modell-Block 1d). kFassung bleibt rc610, nichts gepusht.

## Neue Orte

- Laufskripte `Y:\werkzeuge\sp_af9c\ab\*.py|*.sh` (22 Dateien), Spielskripte `Y:\werkzeuge\sp_654neu\**` (86), Skripte in `Y:\*.sh` (15):
  alte %TEMP%-Scratchpads auf `Y:\werkzeuge\sp_af9c|sp_654alt|sp_654neu` umgestellt (`Y:\werkzeuge\zs\pfade_um.py`).
- CMake-Caches `build-test` und die sechs Worktree-Builds: BEHAVED_MD_QUELLEN, BEHAVED_OPENJK_QUELLEN, BEHAVED_SCRIPTS nach
  `Y:/werkzeuge/sp_af9c/...`, BEHAVED_Q3MAP2_PR316 = `Y:/werkzeuge/pr316/regression_tests/auto-pigeon`.
- Repo: `git grep` findet keine festen Temp-Pfade in Code oder Testwerkzeugen (nur alte Notizen karte1-4 und eine Binaerprobe).

## Zweige

| Zweig | Commit | Nachgetestet / behoben |
|---|---|---|
| kreste1 | 9a1ca4a5d | `--clean-first` 0 Warnungen; karteneditor modobj 25/0, stufe9 84/0, fotos 78/0 |
| render1 | cf838285c | rd 20/0, r3 22/0, reste1 13/0, editorportal 19/0, gluehen 9/0, Modus 1 2635/0, ctest 232/232; Kartenpruefung aller 282 Karten gegen e6daa5ca8: nur `gpu_aufrufe` auf 4 Karten anders (q3map_<surfaceparm> wirkt dort nicht) |
| jka1 | 78067d3be | GUI baut 0 Warnungen; md 83/2 -> Testfehler (QuickSlice suchte die Ansicht von vorn bei einer Ansicht) -> 84/0; funktionen 60/0; ctest 231/231; Modus 1 2634/1 (Ton unter Last, `ton` allein 34/0); Handbuchbild h_md_modell 6 Sprachen |
| animimp1 | eeb18bbc1 | pr 15/0; h_ai_bibliothek 6 Sprachen; ja: "Slot" hinter dem Rand -> Zeile prueft Feld + Beschriftung; gen_handbuch |
| paint1 | ec9ff6245 | pt 34/1 -> zwei Testfehler (Kindfenster, Knopf ausserhalb des Sichtbaren) -> 36/0; h_pt_malen 6 Sprachen: vier abgeschnittene Stellen (Stift-Hinweis, Ebenen-Knoepfe, 100 %, cs "Normální") behoben; MD-Spielvergleich siehe NOTIZEN_1c |
| fix1 | 7534a0a1c | Gruenlauf der WIP-Behebungen; H3 rot nachgestellt; M6 (Test berichtigt), M7, M8 rot -> gruen; N13 Generator im Repo; Einzelheiten NOTIZEN_pfix |

## Zusammenfuehren

Reihenfolge kreste1 (vorspulen) -> render1 -> jka1 -> animimp1 -> paint1 -> fix1. Konflikte:
- Erzeugte Dateien (suche.js, Handbuch-HTML, i18n.h/.cpp, hilfe_orte.inc) nie von Hand: eine Seite genommen und nach jedem Merge
  `gen_i18n.py`, `gen_handbuch.py`, `gen_hilfe_orte.py`, `gen_handbuch.py --pruefen` neu (`Y:\werkzeuge\zs\merge_rest.sh`).
- Angehaengte Listen (gen_i18n.py, gen_handbuch.py, i18n_sprache_es/cs.py): beide Seiten behalten.
- `tools/handbuch/inhalt.py` (paint1): dieselbe Zeile auf beiden Seiten geaendert - "beide behalten" ergab eine doppelte Zeile,
  gen_handbuch brach ab (ctest handbuch_sprachen rot) -> eine Zeile mit Animations-Import UND Painting, Handbuch neu (d4c6c4226).
- fix1 von Hand: CMakeLists (beide Tests), selbsttest.cpp (beide Gruppen), app.cpp (Kommentare von fix1 + Lizenzzeile der
  Presets), modelltab.h (Erklaerung modellRtEinstellen mit `zug`), app_modelltab.cpp (erst `modellOffenesAbbrechen`, dann das
  Rueckgaengig der Malerei), app_modelltab_mesh.cpp (Slice/Hinge-Vorgaben + `z.schritt = aendern`), app_modelltab_retarget.cpp:
  der Animations-Import liest jetzt ohne Rueckgaengig-Schritt (`importLesen`), damit pfix M5 (Quelle nach Rueckgaengig neu laden)
  auch FBX/glTF/BVH/COLLADA nachladen kann; `modellRtImportLaden` legt den Schritt an.
- Nach den Merges: lint_portability 104 -> 99 (Nicht-ASCII in zwei Kommentaren, fehlende Standardkoepfe in zwei Tests), c2eb720be.

## Schluss (exe c045d7da8, Laeufe `zs_s_*`, Summe `Y:\werkzeuge\zs\schluss_summe.txt`)

- Sauberer Neubau `build-test` 0 Warnungen (13 min 40 s); ctest 235/235.
- Modell-Block alle Gruppen mit Testvariablen 0 FEHLER: funktionen 60, tasten 43, mb2 37, blender 44, mb3 21, rig 26, mb5 159, mb6 37,
  uv 50, uvmd3 3, md 84, mdkarte 7, mb7 24, mb7spiel 4, mb7textur, ph 29, rt 19, rtjka, pr 15, pt 36, ai 29, pfix 17, pfixkarte 5,
  Rundlaeufe md/jka/jk2 (rundlauf, rigrundlauf, kurvenrundlauf, ikrundlauf, bild).
- editorportal, gluehen, figurlicht, mission1 98/0, modell, kuerzel 202/0, Modus 1 2635/0.
- teil6 13/3: dieselben 3 FEHLER wie e6daa5ca8 (`zs_teil6_alt` 12/3: G_MissileImpacted, NPC_ChoosePainAnimation, G_RadiusDamage).
- karteneditor voll: 3275 OK, 2 FEHLER (KR truhe/punkt Modell+Clip), dann haengt "KR stuhl: Schnell-Backen fertig" (beendet,
  siehe Offen); die Gruppen danach allein und nacheinander: kr 39/0, r2a 22/0, r3 22/0, rd 20/0, reste1 13/0.
- Lints wie e6daa5ca8 (portability 99 nach c2eb720be, msvclink 110 Dateien ok); Handbuch `--pruefen --pflicht=es --pflicht=cs` ok.

## Bildzeit (allein, BHED_D3D_DEBUG=0, CPU Mittel / p99 ms, alt = e6daa5ca8)

| Messung | alt | neu |
|---|---|---|
| leistung Mission md_toaj_jedi Flug | 3.45 / 16.39 | 3.14 / 13.68 |
| leistung Mission Szene spielt | 3.59 / 5.81 | 3.56 / 4.86 |
| leistung Map Editor Vorlage Kamera dreht | 2.32 / 2.92 | 2.35 / 3.76 |
| leistung Map Editor md_am_sith mit Effekten | 2.64 / 3.87 | 2.57 / 5.53 |
| leistung Map Editor duel_yavin mit Halmen | 2.03 / 2.55 | 2.11 / 3.78 |
| leistung Model stormtrooper Animation | 0.49 / 0.80 | 0.50 / 1.09 |
| modellleistung Animation | 1.14 / 2.77 | 1.18 / 4.29 |
| R3-Glow (Echtzeit-Licht, 240 Bilder) | 4.29 / 8.59 | 4.44 / 11.95 |
| mdzeit Modellieren, nichts gewaehlt | 0.65 / 1.65 | 0.49 / 0.84 |
| rtzeit Retargeting-Vorschau | 0.48 / 0.80 | 0.61 / 1.49 |
| ptzeit Ansehen | - | 0.40 / 0.82 |
| ptzeit Painting, 2D-Bild offen | - | 31.20 / 36.62 |
| ptzeit Painting, je Bild ein Tupfer | - | 43.36 / 49.94 |

Mittelwerte im Rauschen (+-0,1 ms) ausser Painting: mit offenem 2D-Bild 31 ms je Bild (Malen auf der CPU, Bild je Bild neu
hochgeladen?) - Fehler nach der Regel "Schnell", offen fuer paint. p99 einzelner Messungen hoeher (eine Messung je Seite, nicht
wiederholt); rtzeit +0,13 ms Mittel.

## Offen

- karteneditor voll unter paralleler Last: "Schnell-Backen fertig" wartet ewig (KR stuhl; in der Basis e6daa5ca8 ebenso bei
  "R2a Gras", davor dort 4 FEHLER R2a Gras mit 0 Halmen). Allein laufen r2a 22/0 und kr gruen. Verdacht: fester Port 39000 der
  Bauueberwachung (wie NetRadiant-Custom) - eine zweite gleichzeitig bauende Instanz nimmt die Meldung von q3map2 an. Nicht bewiesen.
- fix1: M9, N1, N5, N6, N7, N10, N14 nicht bearbeitet (NOTIZEN_pfix).
- paint: Paarbild Spiel gegen Vorschau an derselben Stelle (NOTIZEN_1c).
- uvrundlauf zeigt 0 Pruefungen (auch in der Basis `md_e_uvrl_*`).
- Painting mit offenem 2D-Bild 31 ms je Bild (siehe Bildzeit).
