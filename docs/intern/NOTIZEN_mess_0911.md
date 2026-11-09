# Notizen MESS / MESS2: Bildzeit-, Lade- und Speicher-Messrunde vor rc611 (09.11.2026, `Y:\karte`, Zweig `karte1`, Basis af7782264)

Werkzeuge und Rohdaten: `Y:\werkzeuge\mess\` (mlauf.py, auswertung.py, speicherbild.py, mess_*.sh, tab_*.txt, Laufordner
`Y:\karte\laeufe\m_*`). Jede Messung allein (kein Bau, kein Test, kein Spiel), normale Prioritaet, `BHED_D3D_DEBUG=0`, je Bau
3 Laeufe abwechselnd (Runde 2 umgekehrt), Median und Spanne (min..max). "!" = mehr als 3 % schlechter und ausserhalb der Spanne.

- alt = rc610 (4c3c86e7, Bau `Y:\werkzeuge\mess\alt`), vor = 23e9b4ac6 (vor FIX4/FIX5, Bau `f4`), neu = af7782264 bzw. der Stand
  danach (neu2 = mit 72e08e52c, neu3 = mit Zeitmarken 1386d43eb, neu4 = mit e1f756a37).
- Fremdlast: `python -I fid3_full.py` (fremder Prozess unter `nice`, ab 18:22 etwa ein Kern) lief bei allen MESS2-Messungen; er
  trifft alt und neu gleich (abwechselnd gemessen). Nicht beendet (nicht eigener Prozess).
- Streuung: Bildzeiten der ruhenden Ansichten +-1-3 %, "Szene spielt" bis +-20 % (Teilchenzahl haengt von der Wanduhr ab),
  Ladezeiten +-5-10 %.

## Ergebnistabelle

| Messung | alt | neu | Diff | nach Fix | Beleg |
|---|---|---|---|---|---|
| ptzeit Painting (2D-Bild offen), CPU Mittel (P6/P15) | 0,88 ms (vor) | 0,41 ms | -53 % | - | tab b1 |
| ptzeit je Bild ein Tupfer, CPU Mittel | 8,03 (8,01..8,44) | 8,34 (8,12..8,46) | +3,9 % in der Spanne | - | tab b1 |
| mdzeit Modellieren, CPU Mittel | 0,53 / 0,66 | 0,53 / 0,66 | 0 % | - | tab b2 |
| rtzeit Retargeting, CPU Mittel | 0,51 | 0,49 | -4 % | - | tab b3 (gemessen: M1-Zeilen da; "0 OK" heisst nur: keine Pruefzeilen) |
| rtzeit eigene Animation, GPU Median | 0,05 ms | 0,07 ms | +0,02 ms | - | tab b3, GPU-Verformung (bleibt) |
| r3zeit R3-Glow, Mittel | 4,25 ms | 4,17 ms | -1,9 % | - | tab b4 |
| FIX5-Messblock (ks): Flug md_toaj_jedi GPU Mittel | 1,61 (vor) | 1,37 | -15 % | - | tab b5 |
| FIX5-Messblock: Szene spielt GPU Mittel | 2,53 (vor) | 2,72 | +7,4 % in der Spanne | - | tab b5 |
| P9 presetbau lange BVH (10000 Bilder) | 2,91 s (vor) | 0,68 s | -77 % | - | mess_p.log, Ausgabe sha256 gleich |
| P47 Uebersetzung deformVertexes (Summe 7 Shader) | 7733 ms (vor) | 4040 ms | -48 % | - | mess_p.log |
| P11 Echtzeit-Licht ruhend, CPU Mittel | 6,77 | 6,50 | -4 % | - | tab m2_g |
| P28 CJK (ja, Menue-Editor ruhend), CPU Mittel | 0,48 | 0,53 | +10 % in der Spanne | - | tab a3 |
| P38 tcMod/turb: Editor-Spielbild Karte deckend, GPU | 0,69 | 0,68 | -0,4 % | - | tab m2_g (GPU-Teil) |
| Editor-Spielbild md_am_sith, GPU Mittel | 2,72 | 3,29 | +21 % ! | - | tab m2_g, Punkt 3 |
| Editor-Spielbild duel_yavin, GPU Mittel | 3,24 | 3,65 | +12,9 % ! | - | tab m2_g, Punkt 3 |
| Editor-Vorlage duel_jeditemple, GPU Mittel | 2,84 | 3,05 | +7,4 % ! | - | tab m2_g, Punkt 3 |
| Echtzeit-Licht Taris ruhend, GPU Mittel | 20,82 | 21,96 | +5,5 % ! | - | tab m2_g, Punkt 3 |
| Mission Szene spielt, GPU Mittel | 3,04 (2,78..3,28) | 3,87 (3,79..4,62) | +27 % (Streuung gross) | - | tab m2_g, Punkt 3 |
| Speicher nach der .map: privat | 6876 MB | 7450 MB | +8,3 % | - | Punkt 4 |
| Speicher nach der .map: Arbeitssatz-Spitze | 3236 MB | 3224 MB | -0,4 % | - | tab m2_g |
| Speicher nach der .map: Grafikkarte | 4082 MB | 2445 MB | -40 % | - | tab m2_g |
| Laden md_am_sith, geladen (warmer Shader-Speicher) | 1436 | 3098 | +116 % | 1732 (72e08e52c) | tab aw / k2 |
| Laden mp/duel1, geladen | 297 | 583 | +96 % | 529 (e1f756a37) | tab m2_l |
| Laden md_toaj_jedi, geladen | 1703 | 2080 | +22 % | 1962 (e1f756a37) | tab m2_l |
| Register vor der Welt je Karte | - | 233-325 ms | - | 170-218 ms | Protokollzeile "Bild je Name: vor der Welt" |
| Programmstart bis erstes Bild | 487 ms | 501 ms | +2,8 % in der Spanne | - | tab m2_g |

## Punkt 1: Messung B (23e9b4ac6 gegen af7782264)

- b1 ptzeit: Painting mit offenem 2D-Bild -53 % (P6/P15 wirken), Tupfer je Bild +3,9 % in der Spanne. Wandzeit und Speicher des
  Laufs hoeher, weil neu mehr Schritte misst (Weichzeichnen, volle Maske) - kein Vergleich.
- b2 mdzeit, b3 rtzeit, b4 r3zeit: gleich. b3 hat gemessen (M1-Zeilen je Bau 3/3).
- b5 ks: Laden in b5 noch ohne 72e08e52c (neu = af7782264), daher die alten +85-100 %; Bildzeiten wie Tabelle.

## Punkt 2: P9 und P47

- P9 (FIX2, linear statt quadratisch): 2,82-2,94 s -> 0,67-0,78 s, die Ausgabe-BVH ist bytegleich (sha256 2bffb0a20d62def7).
- P47 (DEF1): Summe 7659/8097/7733 -> 4028/4080/4040 ms.

## Punkt 3: GPU-Mehrkosten zerlegt (Zeitmarken je Teil, 1386d43eb, `BHED_GPU_ABSCHNITTE=1`)

Derselbe Zeitmarken-Patch auch im alt-Bau (nur fuer die Messung, Worktree danach entfernt).
- Editor-Spielbild md_am_sith +0,57 ms: Nebensicht (Himmelsportal) 0,60 -> 1,18 ms; Karte, Effekte, Mover, Gluehen der Hauptsicht
  und Gamma gleich (+-1 %). duel_yavin +0,41 ms: Nebensicht 1,27 -> 1,68 ms. Ursache cf838285c (p_render): das Gluehen laeuft jetzt
  in jeder Sicht wie RB_DrawSurfs je RE_RenderScene (CG_DrawSkyBoxPortal, R_MirrorViewBySurface); zeichneGluehen laeuft dort nur,
  wenn die Sicht leuchtende Flaechen hat (n == 0 -> kein Weichzeichnen). Dient der Spieltreue, bleibt.
- Editor-Vorlage duel_jeditemple +0,21 ms: Mover deckend (Kartenmodelle) 0,19 -> 0,37 ms, sonst gleich. Wahrscheinlich die
  Modellstufen wie im Spiel (deformVertexes/autosprite je Instanz, FIX5/REST3); nicht weiter zerlegt (offen).
- Mission "Szene spielt": Effekte gemischt 1,21 -> 0,06 ms, Gluehen 0,31 -> 1,45 ms (die Teilchen mit glow und der Rauch gehen
  seit REST3c/p_render ins Gluehen), Summe der Teile 1,70 -> 1,58 ms; der Rest ausserhalb der Teile schwankt mit der Szenenzeit
  (alt 2,78..3,28, neu 3,79..4,62; FIX5-Block gegen 23e9b4ac6 nur +7,4 % in der Spanne). Nicht als Mehrarbeit nachweisbar.
- Echtzeit-Licht Taris +1,1 ms (+5,5 %): Taris hat kein Portal und keinen Spiegel; das Echtzeit-Licht hat keine Zeitmarken je Teil
  (offen). Kandidaten aus dem Verlauf: bewegte Grundstufen und Gluehen der Effekte im Echtzeit-Licht (cf838285c), vs_5_0 mit
  precise (P35), tcMod-Kette und Wellen bitgleich wie rd-vanilla (FIX4/FIX5) - alles Spieltreue.
- Reine Mehrarbeit ohne Bildwirkung: keine gefunden; deshalb kein Code an den Durchgaengen geaendert (Bild unveraendert).

## Punkt 4: Speicher privat +500 MB

Speicherbild bei "Speicher nach der .map" (speicherbild.py, VirtualQueryEx, MEM_COMMIT + MEM_PRIVATE je Zuteilung):
- alt 6873 MB in 766 Zuteilungen, neu 7345 MB in 786; der Zuwachs liegt fast ganz in der Klasse 16-64 MB (+437 MB: 32,000 MB x7
  statt x2, 31,938 MB x5 statt x3, 21,375 MB x6 statt x3) und 4-16 MB (+38 MB, 5,375 MB x18 statt x11).
- Die Heap-Grossbloecke der Laufzeit (1,004 / 2,004 / 4,004 / 16,004 MB = Groesse + 4-KB-Kopf) sind alt und neu gleich (262/52/121/28).
  Die neuen Bloecke haben genaue 64-KB-Groessen ohne Kopf, also VirtualAlloc ausserhalb des eigenen Codes (der Code ruft
  VirtualAlloc nirgends): 5,375 MB = 1024x1024 RGBA8 samt Pyramide, 21,375 MB = 2048x2048 samt Pyramide - Speicher von
  Direct3D/Treiber fuer Texturen und Hochladepuffer.
- Arbeitssatz gleich (2509 -> 2528 MB, Spitze -0,4 %), Grafikspeicher -40 % (MR8 ein Bild fuer Vorschau, Spielbild und
  Echtzeit-Licht). Die +500 MB sind zugesagter, nicht belegter Speicher des Treibers. Nicht ohne Eingriff in die Texturen
  (Format, Pyramide, Teilen) zu senken; das waere ein Qualitaets- bzw. Spieltreue-Tausch. Begruendet, nicht geaendert.

## Punkt 5: Laden

- Rot: Register vor der Welt 233-325 ms je Karte (neu3). bildVorhanden fragte fuer jedes Bild je fehlender Endung und Spielordner
  das Dateisystem, bevor es die naechste Endung in den Archiven suchte; nach jedem R_InitShaders (Oberflaeche vor/nach dem Server,
  Menues, Schriften) fragte das Register dieselben Bilder erneut.
- Fix e1f756a37: erst alle Archive, dann die Ordner (dieselbe Antwort: nur OB eine Datei da ist), Antworten je Ladevorgang
  gemerkt (neuer Ladevorgang = neue Liste; innerhalb eines Ladevorgangs aendern sich die Dateien nicht).
- Gruen: 170-218 ms; geladen duel1 583 -> 529 ms, md_toaj_jedi 2080 -> 1962 ms. Gleiches Register (1184 / 1218 Bilder, keine
  Warnung anders).
- Rest gegen rc610: +0,23-0,26 s je Karte = das Register selbst (rund 125 .menu-Dateien aus ui/menus.txt, ui/ingame.txt und
  ui/jahud.txt je Ladevorgang lesen und zerlegen; innerhalb eines Ladevorgangs nur 4 Dateien doppelt). Ein Zwischenspeicher ueber
  Ladevorgaenge braeuchte Dateizeit/Groesse aller Quellen (Archive und lose .menu); nicht gebaut (offen).

## Schluss

- WIP 35b89693a/43d189e5f zusammengefasst: 72e08e52c (Register liest jede Modelldatei einmal) und 5eeabb864 (leistung: Update-Leiste,
  Ruhe-Gruende), Baum gleich 43d189e5f.
- Commits: 72e08e52c, 5eeabb864, 1386d43eb (GPU-Teile), e1f756a37 (Laden), dieser Notiz-Commit.
- Schlussrunde (`Y:\werkzeuge\mess\schluss_m2.sh`, `schluss_m2b.sh`, Laufordner `Y:\karte\laeufe\m2s_*`): sauberer Neubau 0 Warnungen
  (18 min), ctest 246/246, geschuetze 21/0 (mit den Schaltern wie REST4; ein erster Aufruf ohne BHED_GV_* und sour.cfg zaehlt nicht),
  modellblock funktionen 61/0, md 86/0, pt 41/0, ptfix3 12/0, haut 1/0, karteneditor r3 22/0, Modus 1 2635/0, menueeditor 2787/0,
  leistung allein mit BHED_GPU_ABSCHNITTE=1 und Taris 2/0 (GPU-Teile ohne veraltete Werte: md_am_sith zeigt keine Halme und Figuren
  mehr). lint_sources, lint_fassung, lint_d3dbesitz, lint_headers, lint_settings ok, lint_portability 99 ohne die geaenderten Dateien.
- Worktrees `Y:\werkzeuge\mess\alt` und `f4` entfernt, `neu*` und `sc_*` geloescht.

## Offen

- Echtzeit-Licht +5,5 % GPU nicht je Durchgang zerlegt (keine Zeitmarken im Echtzeit-Licht).
- Editor-Vorlage Mover deckend +0,18 ms nicht weiter zerlegt.
- Laden: Rest +0,23-0,26 s je Karte (Register: Menues je Ladevorgang).
- Model-Tab stormtrooper ruhend CPU 0,36 -> 0,54 ms (ja, tab a3; neuer Model-Tab, absolut klein).

## Changelog

## English

- Maps load faster: the check which pictures the game finds looks into the archives first and asks each picture only once per load (md_toaj_jedi about 0.1 s faster).

## Deutsch

- Karten laden schneller: die Pruefung, welche Bilder das Spiel findet, sieht zuerst in den Archiven nach und fragt jedes Bild nur einmal je Ladevorgang (md_toaj_jedi etwa 0,1 s schneller).

## 中文

- 地图加载更快：检查游戏能找到哪些图片时先查看压缩包，并且每次加载每张图片只检查一次（md_toaj_jedi 约快 0.1 秒）。

## 日本語

- マップの読み込みが速くなりました：ゲームが見つける画像の確認で、まずアーカイブを調べ、各画像を読み込みごとに一度だけ確認します（md_toaj_jedi で約 0.1 秒短縮）。

## Español

- Los mapas cargan más rápido: la comprobación de qué imágenes encuentra el juego mira primero en los archivos y consulta cada imagen solo una vez por carga (md_toaj_jedi unos 0,1 s más rápido).

## Čeština

- Mapy se načítají rychleji: kontrola, které obrázky hra najde, se nejprve podívá do archivů a na každý obrázek se ptá jen jednou za načtení (md_toaj_jedi asi o 0,1 s rychleji).
