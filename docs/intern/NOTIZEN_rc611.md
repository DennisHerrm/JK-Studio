# Notizen rc611: Release-Runde (Changelog, Handbuch, voller Lauf, Version rc611)

Auftraege `auftrag_rc611_release.md` (Teil 1) und `auftrag_rc611b_release.md` (RC611b). Direkt in `Y:\karte` auf `karte1` ab 03ee396d7 (180 Commits seit rc610 = 4c3c86e7), Build
`build-test` (`--parallel 8 -- /nodeReuse:false`), Laeufe `laeufe/rel611_*` (Programm `laeufe/rel611_exe`, nach den Fixes
`laeufe/rel611_exe2`, Gegenproben mit alten Programmstaenden `rel611x_*`), Skripte `Y:\werkzeuge\rel611\` (pool.py wie e10,
welleB.sh, menuebilder.sh, leiste.sh, umzug.py, update.py). Kein Spiel gestartet.

## Version und Changelog
- `kFassung` rc611. Ein rc611-Eintrag in 6 Sprachen aus den Changelog-Abschnitten der 46 seit rc610 neuen Notizen (1b, 1c, 1d,
  animimport, aseb, def1, fix2, fix6, fix7, kreste, mb1a1 bis mb7, merge1 bis merge4, mess, mr4, mreste, pfix, pjka, ptfix3, r3,
  render_p, rest bis rest10, stufe5, stufe6, td1): Uebersicht oben, dann Updates, Shader wie im Spiel, Grafik wie im Spiel,
  schneller laden, Model-Tab, Model-Tab Animation, Mission-Tab, Karten-Editor, Menue-Editor, Oberflaeche und Sprachen.
  Interne Pruefungs-, Lauf- und Agentennamen nicht genannt. `tools/changelog.txt`, `AENDERUNGEN.md`, `gen_changelog.py`
  (40 Fassungen x 6 Sprachen), lint_fassung ok (rc611).

## Handbuch
- Die Menue-Editor-Bilder zeigten in der Statuszeile und rechts in der Werkzeugleiste den Laufordner des Selbsttests
  ("Saved Y:\karte\laeufe\bm_en\einst\3b_drei.menu", "Exported ...\menue_probe.pk3", "Project saved: ...\projekt_mp\..."),
  in 192 von 300 Bildern (6 Sprachen). Behoben im Bildlauf: `foto()` kuerzt vor dem Bild jeden Pfad unter dem Laufordner auf
  den Dateinamen (Statuszeilen und Meldungszeile des Menue-Editors, `menueeditor::meldungZeile()`); die Meldung selbst bleibt.
- Neu aufgenommen: alle 49 Menue-Editor-Bilder des Handbuchs in en, de, zh, ja, es, cs (Bildlauf `BHED_MENUE_BILDER=1`,
  2538 x 1444, angesehen: menue_projekt_sicherung en, menue_reiter_properties de, Statuszeilen aller en-Bilder). Bildlauf
  en/zh/ja 581/0, cs 581/1, es 580/2, de 578/3 - die roten Zeilen betreffen menue_leiste_transition/menue_leiste_probe (nicht im
  Handbuch) und in de/es menue_reiter_properties (Pruefung "Reiter Text ... keine Spalte abgeschnitten", Altbestand MR5); das
  Bild selbst zeigt den Reiter Eigenschaften richtig und ist uebernommen. menue_start_oeffnen (anderer Lauf, ohne Pfad) bleibt.
- Fassungsnummern im Handbuch: nur geschichtliche Angaben ("seit rc601", "bis rc605 BehavEd-Remake", "vor rc600 hiess der Tab
  Map"), keine "Was ist neu"-Stelle mit Fassung. `gen_handbuch.py` ohne Aenderung, `--pruefen` ok (es/cs 0 englisch).

## Voller Lauf
Teil 1 (Programm `rel611_exe`, 183 Selbsttest-Laeufe parallel, 12 zugleich, dazu ctest und Lints): 158 Laeufe beim ersten Mal
gruen, 25 rot. Teil 2 (RC611b, Programm `rel611_exe4` = fe9be61a9, dann die Endstaende `rel611b_exe8/9`): die offenen Punkte
und die betroffenen Modi.

| Bereich | Ergebnis |
|---|---|
| Neubau | 0 Warnungen, 0 Fehler (Teil 1 `--clean-first`; Teil 2 jeder Zwischenbau) |
| ctest | 245/245 + pr316 (Teil 1), 246/246 (fe9be61a9), Endstand siehe RC611b |
| Modus 1 en/es/cs, sp | je 2635/0 |
| mp | 2634/1 unter Last (Bildzeit), allein 2635/0 |
| karteneditor en/es/cs | 3406/1 (Stufe 7 p99 unter Last, wie rc610) / 3407/0 / 3407/0 |
| menueeditor MD/JA/es/cs | 2787/0, 2785/0, 2787/0, 2787/0 |
| mission1 | 98/0 |
| zeitleiste1 en/es/cs, spulen, teil11 | unter Last je 1 rot (Bildzeit), allein gruen (Welle B) |
| dock2mass, dock5, fix604b, gegenstaende, kartenfunde, pruefung3, teil6 | nach Testnachzug gruen (Welle B) |
| update, umzug, updateleiste2 | eigene Skripte (update.py, umzug.py) gruen; im Pool rot, weil sie eigene Ordner/Server brauchen |
| einzel | im Pool 0/6 wie rc609/rc610 (braucht `BHED_EINZEL`), mit `BHED_EINZEL` siehe RC611b |
| mb_bild md/jka, mb_bild2 md/jka, mb_mb7 | *off-Fix (unten): gruen (Welle E) |
| geschuetze | G5 Turbolaser: Test nach dem Spiel aufgebaut, gruen (unten) |
| umriss | B3-3: Programm wie R_FindShader, gruen (unten) |
| allekarten | "Speicher: nichts liegen geblieben" rot, siehe Punkt 4 |
| Lints | sources, fassung, i18n, settings, imgui_context, mapstate, rueckkopplung, layout ok; portability 99, pruefcodes 11 wie bisher |

## RC611b (zweiter Teil der Release-Runde)
Auftrag `auftrag_rc611b_release.md`, Laeufe `laeufe/rel611b_*`, Skripte `Y:\werkzeuge\rel611\` (welleE..J.sh, b/).

### 1. *off-Flaechen bestaetigt
- Welle E mit `rel611_exe4` (passt zu fe9be61a9, Neubau ohne Aenderung, gleiche Pruefsumme): mb_bild2 md 2/0, mb_bild2 jka 2/0,
  mb_bild md 1/0, mb_bild jka 1/0, mb_md 86/0, mb_funktionen 61/0, mission1 98/0, ctest 246/246.
- Foto `mb2_nachher_imperial` (bild2 md mit Fotos, `rel611b_g_b2md`) angesehen: der imperiale Offizier ist ganz grau, kein heller
  Oberkoerper mehr.

### 2. geschuetze G5 Turbolaser
- Spielverhalten (Movie Duels 9, `code/game/g_turret.cpp`): `SP_misc_turretG2` gibt jedem misc_turretG2 `SPF_TURRETG2_MP_RULES`
  (:1162); `turret_find_enemies` bricht nur fuer den Turbo OHNE MP_RULES ab (:609) - der SP-Turbolaser aus misc_turretG2 sucht
  seine Feinde also selbst. Ohne alliedTeam/teamnodmg setzt er `noDamageTeam = TEAM_ENEMY` (:1245) und uebergeht Figuren dieses
  Teams (:701); wait 1000 und damage 500 ohne Kartenwert (:1236, :1240). OpenJK hat kein misc_turretG2 (nur misc_turret, dort
  sucht der Turbo nie selbst).
- Der Test war falsch aufgebaut: sein Ziel war TEAM_ENEMY (der Turbolaser schiesst nie darauf), in rc610 traf er eine Figur der
  Grundkarte und g5_ziel starb an der Splitterwirkung; mit geraeumter Umgebung 0 Schuesse (Welle C rot). Zwischendurch gruen nur,
  weil wieder eine Figur der Grundkarte stand (fe9be61a9, Radius 48).
- Neu: Ziel TEAM_PLAYER, 1200 Lebenspunkte, 480 vor dem Ort (rund 580 vom Geschuetz - 130 entfernt lag die Figur hinter den
  Muendungen des doppelt grossen Modells, turret_aim zielt von dort, das Geschuetz pendelte und traf 14-mal den Boden), Pruefung
  "alle 1000 ms" (wait) und Tod nach dem zweiten Schuss. Rot `rel611b_h_geschuetze` (14 Schuesse, alle in den Boden, Tod -1) ->
  gruen `rel611b_i_geschuetze` 12/0: 8 Schuesse, abwechselnd Muendung 0/1 und 2/3, zwei Volltreffer auf g5_ziel, Tod 7350 ms.
  Das Programm war richtig (Simulation 1:1 wie oben).

### 3. umriss B3-3
- Die Haut des Sturmtrupplers nennt fuer alle Kappen `models/players/stormtrooper/caps.tga`; die Datei gibt es nicht, wohl aber das
  Skript `models/players/stormtrooper/caps` in `shaders/common.shader` (textures/common/caps, caps_glow). Das Spiel sucht das
  Skript zuerst (R_FindShader: FindShaderInShaderText vor R_FindImageFile, rd-vanilla tr_shader.cpp) und fragt nie nach
  caps.tga/.jpg/.png - die Suche war also KEIN Spielverhalten, sondern ein Programmfehler (jede Ladung 3 vergebliche Suchen).
- Behebung `texturesFor` (gui/app_view3d.cpp): Nennt keine Stufe des Skripts gleichen Namens die Datei der Haut, gilt wie im Spiel
  das Skript zuerst (auch im Vorab-Lesen); nennt eine Stufe die Datei, bleibt der direkte Weg (gleiches Bild). Fehlt das Bild des
  Skripts, wie bisher die Datei. Gezaehlt in den Spieldaten (`b/hautskript.py`): Movie Duels 179 solche Hautnamen, 12 davon mit
  eigener Datei (u. a. darthvader/armor -> body2, droideka/shield -> energyshield, gunray_ep3/head) - dort zeigte die Vorschau bisher
  die Datei, das Spiel das Skript; Original-JKA 29, keiner mit eigener Datei (Bild dort unveraendert). Nicht im Spiel verglichen.
- Test wieder streng (jede Archivsuche zaehlt): rot `rel611b_f_umriss` (3 Suchen) -> gruen `rel611b_g_umriss` 49/0 (0 Suchen,
  Bildpunkte gleich).

### 4. allekarten "Speicher: nach allen Karten nichts liegen geblieben"
- Allein gemessen (gleiche Daten, gleicher Rechner, nacheinander): rc610-Programm (4c3c86e7) +578 MB, rc611 (`rel611b_exe8`)
  +672 MB - beide ueber der Grenze 512. Bei der Freigabe von rc610 waren es +311 (R1); seither hat sich die Movie-Duels-Installation
  geaendert (05.11. 23:18, jetzt 282 Karten). Je Karte schwankt der private Speicher zwischen zwei Laeufen um bis zu 300 MB
  (`b/akvergleich.py`), die Spitze ist gleich (4875 MB); der Unterschied 672 gegen 578 liegt darin.
- Leckprobe: alle Karten zweimal (`BHED_RUNDEN=2`, `rel611b_ak_runden2`): Runde 2 liegt im Mittel +358 MB ueber Runde 1, am Ende
  +923. Darum zerlegt: `gpu::SpeicherStand` und die Speicherzeile nennen jetzt auch Arbeitssatz und die Heaps des Prozesses
  (HeapSummary: zugeteilt = lebende Bloecke, belegt). Teilliste (51 Karten, 3 Runden, `rel611b_ak_r3`): zugeteilt 399 -> 556 ->
  577 MB, belegt 410 -> 715 -> 762 MB, privat ohne Heaps 440 -> 427 -> 366 MB. Also: einmaliger Warmlauf in Runde 1 (Zwischenspeicher
  nach Namen; im Groessenbild u. a. Bloecke von 18,9/17,9/12,9/11,6 MB), danach Verschnitt des Heaps (belegt minus zugeteilt +200 MB)
  und ein stetiger Rest: zugeteilt je Runde +18,7 MB (min 17,7, max 20,4) = 0,37 MB je Ladung.
- Der stetige Rest gehoert dem Grafiktreiber: Groessenbild der Heap-Bloecke je Runde (voruebergehende Messung, nicht im Programm
  geblieben): es wachsen Bloecke von 648, 120, 104 und 96 Byte (je rund 1160 je Runde bei 14 Karten) und 184 Byte; deren Inhalt zeigt
  ab Byte 8 in nvwgf2umx.dll (NVIDIA D3D11), keiner in jkstudio.exe. Mit `BHED_D3D_DEBUG=0` gleich (+3,7 MB je Runde), also nicht die
  Debugschicht. Die Zaehler des Programms (Bilder, Lightmaps, Mover-Netze, Figurenpuffer, texCache) sind nach jeder Runde wie
  vorher, der Grafikspeicher +2 bis +5 MB.
- Bisektion: Worktree `Y:\werkzeuge\rel611\bis` (wieder entfernt), rc610 (4c3c86e7) mit derselben Heap-Messung gebaut: Teilliste
  academy+yavin, 3 Runden: rc610 je Runde +2,5 MB (0,18 je Ladung), rc611 +4,0 MB (0,29), ohne Debugschicht +3,7. Der Rest ist also
  schon in rc610 da und kommt aus dem Treiber; eine weitere Bisektion der Differenz (0,1 MB je Ladung, nahe der Streuung 1,6..2,9
  gegen 2,7..4,3) entfiel.
- Ergebnis: kein liegengebliebener Speicher des Programms; Grenze begruendet auf 1024 MB (Kommentar im Test mit den Zahlen oben,
  hoechster Wert unter Last +801). Die Zaehler und der Grafikspeicher bleiben die eigentliche Pruefung, ein Leck wie rc608
  (+3,4 GB) faellt weiter auf. Rot: `rel611b_ak_head` +672 (alle Teil-1-/C-Laeufe +655..801) -> gruen: siehe Welle J.

### 5. Bild menue_eng_zu
- Die Probe setzte das Fenster samt Rahmen auf 1920 x 1080, das Bild war 1898 breit. Jetzt die Innenflaeche 1920 x 1080 (Rahmen aus
  Aussen- minus Innenmass), die Maus rechts in die leere Menueleiste (sonst stand im Lauf nur dieses Teils ein Tooltip im Bild).
  `BHED_MENUE_NUR=eng`, `BHED_MENUE_BILDER=1`, 6 Sprachen je 7/0, Bilder 1920 x 1080 (`tools/handbuch/bilder.py`), en/de/ja angesehen.

### Schluss RC611b (Endstand `rel611b_exe10`, Welle J, parallel)
- Neubau nach jeder Aenderung 0 Warnungen, 0 Fehler. ctest 246/246.
- Modus 1 2635/0, menueeditor (MD) 2787/0, mission1 98/0, allekarten 283/0 (unter Last +697 MB, Grafik +4,4 MB; Heaps zugeteilt
  399 -> 893, belegt 415 -> 1132), karteneditor kurz (`BHED_KARTE_NUR=fotos`) 78/0, einzel (mit `BHED_EINZEL`) 6/0.
- Betroffene Bereiche von "Skript zuerst": umriss 49/0, geschuetze 12/0, figurlicht 8/0, gluehen 9/0, teil9 29/0, karte 233/0,
  mission 8/0, npcklasse 13/0, figurstufen 8/0, gegenstaende 54/0, mb_md 86/0, mb_bild md/jka 1/0, mb_bild2 md/jka 2/0,
  mb_funktionen 61/0, mb_haut 1/0.
- Lints: sources, fassung, feldinit, gapp, d3dbesitz, i18n, imgui_context ok; portability 99 (wie Teil 1).
- Nicht wiederholt (gruen in Teil 1, Bereich nicht beruehrt): karteneditor voll, es/cs-Laeufe, uebrige Modi.

## Behoben
- Model-Tab ohne Dockspace (`BHED_DOCKSPACE=0`): die Spalte verdeckte die Zeitleiste (Teil 1).
- Hauteintrag "*off" wurde gezeichnet, wenn Ghoul2 die Flaeche anliess (RenderSurfaces): imperiale Offiziere in Movie Duels mit
  hellem Oberkoerper (Teil 1, bestaetigt in RC611b Punkt 1).
- Hauteintrag mit Shaderskript, das die Datei der Haut nicht nennt: jetzt das Skript zuerst wie R_FindShader; keine vergeblichen
  Archivsuchen mehr (stormtrooper caps.tga), 12 Movie-Duels-Haeute zeigen das Bild des Skripts (RC611b Punkt 3).
- Selbsttests nachgezogen: pruefung3, teil6, Lavawetter, bild Fahrzeuge, mb7 (Teil 1); geschuetze G5 nach dem Spiel aufgebaut,
  B3-3 wieder streng, allekarten mit Heap-Zerlegung und begruendeter Grenze (RC611b).
- Handbuchbilder: 49 Menue-Editor-Bilder ohne Laufordner-Pfade (Teil 1), menue_eng_zu 1920 breit (RC611b).

## Dem Nutzer nennen
- Licht-Cache greift noch nicht bei Licht mit -bounce: das Licht wird dann jedes Mal ganz neu gerechnet, und die Zeitschaetzung
  kann zu hoch sein (sie nimmt den letzten vollen Bau). Kommt in rc612 (Rueckmeldung eines Mappers).
- Hauteintraege mit eigenem Shaderskript (u. a. Rüstung von Darth Vader, Schild des Droideka in Movie Duels) zeigen jetzt das
  Bild des Skripts wie das Spiel; aus dem Quelltext der Engine, nicht im Spiel per Foto verglichen.
- Der Turbolaser (misc_turretG2 mit spawnflags 8) schiesst unter Movie Duels wie im Spiel nur auf Figuren, die nicht zum Team des
  Imperiums gehoeren (ohne alliedTeam/teamnodmg); unter OpenJK gibt es misc_turretG2 nicht.
- Speicher: Nach dem Laden vieler Karten bleibt der Prozessspeicher hoeher (Heap-Verschnitt, Treiber); das Programm selbst haelt
  nichts von alten Karten fest.
- Wie in rc610: Bildzeit-Pruefungen nur allein gueltig; Movie-Duels-MP-Daten fehlen auf diesem Rechner (Tests uebersprungen).

## Changelog

### English
- Version rc611 with the changelog of all rc611 parts.
- A skin entry whose shader script names another image draws that script like the game, which looks for the script first.

### Deutsch
- Version rc611 mit dem Changelog aller rc611-Teile.
- Ein Hauteintrag, dessen Shaderskript ein anderes Bild nennt, zeichnet wie im Spiel dieses Skript (das Spiel sucht das Skript zuerst).

### 中文
- 版本 rc611,包含所有 rc611 部分的更新日志。
- 若皮肤条目的着色器脚本指定了另一张图像,则与游戏相同地按该脚本绘制(游戏先查找脚本)。

### 日本語
- バージョン rc611。rc611 の全パートの変更履歴を収録。
- スキン項目のシェーダースクリプトが別の画像を指す場合、ゲームと同じくそのスクリプトで描きます (ゲームはスクリプトを先に探します)。

### Español
- Versión rc611 con el registro de cambios de todas las partes de rc611.
- Una entrada de skin cuyo script de shader nombra otra imagen dibuja ese script como el juego, que busca primero el script.

### Čeština
- Verze rc611 se seznamem změn všech částí rc611.
- Položka skinu, jejíž shaderový skript uvádí jiný obrázek, se kreslí podle tohoto skriptu jako ve hře, která skript hledá nejdřív.
