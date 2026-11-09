# Notizen REST3d: Rest von REST3c (09.11.2026, `Y:\karte`, Zweig `karte1`, Basis 36a1c97b0)

Build `Y:/karte/build-test` (`--parallel 8 -- /nodeReuse:false`), Laeufe `r3d_*` unter `Y:\karte\laeufe`, Skripte und Spielbilder
`Y:\werkzeuge\r3d\`. Spielbilder ueber `screenshot_tga` (`Y:\werkzeuge\r3d\spiel.py`, Vorlage r3c), Movie Duels per Popen, eigener
fs_homepath, Spielordner jedes Mal unveraendert. Installiert ist Movie Duels "Update 8 Release Candidate" vom 07.11.2026
(`MovieDuels-SP-Update-8 ... Nov 7 2026`).

## Punkt 1: Schwaden duel_exegol - ein Fehler des Messaufbaus, die Vorschau stimmt

- Die "weichen Schwaden" der REST3b/REST3c-Spielbilder sind die Teilchenwolken `fog` und `light_fog` von fx_wind (spawnflags 98)
  aus `maps/duel_exegol.ent`. Das Spiel liest die Entities aus dieser .ent, nicht aus der .bsp (CMod_LoadEntityString,
  qcommon/cm_load.cpp:485-509 MD9, Protokoll "Loaded entities from maps/duel_exegol.ent") - darum hatten alle Versuche von REST3c
  mit umgeschriebener .bsp (ohne Nebel-Runner, ohne fx_wind, ohne func_timer) keine Wirkung, und die Blitze blieben.
- `Weather/duel_exegol.cfg` (`r_weather 0`) raeumt diese Wolken im Spiel: IT_LoadWeatherParms schickt nach CG_Init
  "weather clear\nexecq Weather/%s\n" (game/g_itemLoad.cpp, Text auch in der installierten MovieDuels-gamex86_64.dll). Das sind
  Befehle am ENDE des Befehlspuffers - im Messaufbau standen dort noch alle Zeilen der pj.cfg (wait, Kamera, screenshot_tga), die
  Wolken wurden also erst nach den Bildern geraeumt. Beim Spielen ist der Puffer leer: geraeumt im naechsten Bild.
- Beleg im Spiel (r_DynamicGlow 0, Kamera -600 -5952 208, Nick 10): alter Aufbau G 11,9 / B 16,9 (Schwaden); mit "weather clear"
  bzw. "r_weather 0" vor dem Bild 4,6 / 8,7 (keine Schwaden). Neuer Aufbau `R3D_ECHT` (spiel.py): die Wetterdatei im eigenen
  fs_homepath fuehrt pj2.cfg erst bei ihrem zweiten Lauf aus (nach CG_Init) - also in der Reihenfolge des Spiels: 4,6 / 8,8, keine
  Schwaden, das Protokoll zeigt die Datei zweimal. Die Vorschau raeumt die Wolken ebenso (Wetter: 2 Teilchenwolken aus der Karte,
  Weather/duel_exegol.cfg 0) - G 5,2 / B 10,0, keine Code-Aenderung.
- Gegenprobe md_acc_jedi (fx_snow, fx_wind 64, `r_weather 0`) und duel_ahs_wbw (`r_weather 6`) im alten Aufbau: auch dort wirkt die
  Datei erst nach den Bildern - Endrunde 4 (rc608) hatte md_acc_jedi mit Fensterfotos gemessen (geraeumt).
- Die Nebelmodelle (fogsprites*.efx) tragen im echten Spielbild nichts bei: mit schwarzem fogsprite.tga im fs_homepath bleibt es
  bei 4,6 / 8,7 - ebenso in der Vorschau (mit/ohne Effekte gleich).
- Rest (echtes Spielbild gegen Vorschau): ohne Gluehen ist die Vorschau im Mittel G +0,6 / B +1,3 heller (je Bildpunkt 0,73 von
  255), mit Gluehen +1,8 / +3,2 (1,87). Das Gluehen ist anteilig richtig (Spiel mit/ohne 2,65/2,51-fach, Vorschau 2,69/2,53); der
  Unterschied liegt gleichmaessig auf den Flaechen im Nebel (rechte Wand Spiel 2,9 / 6,2, Vorschau 3,9 / 9,0). Nicht weiter
  eingegrenzt.

## Punkt 2: Detailstufen Rest

- **.md3 mit Detailstufen** (fa6c79c11): Kartenmodelle (misc_model_static, _breakable, model2 an Movern) laden Name_2.md3 und
  Name_1.md3 wie RE_RegisterModel (tr_model.cpp:656-736 MD9; fehlt eine Stufe unter einer geladenen, bricht das Spiel ab - hier
  eine Meldung, ohne Stufen) mit eigenen Flaechen und Shadern je Stufe und waehlen je Sicht wie R_ComputeLOD (tr_mesh.cpp:176-219
  MD9): Radius aus den Grenzen des gezeichneten Bildes (RadiusFromBounds), ohne s.radius, modelScale, 0,75 und ohne die Grenze 20
  fuer r_lodscale. Vorschau und Spielbild des Karten-Editors.
  - ctest modell_dokument: md3LodWieImSpiel gleich der abgeschriebenen Engine-Funktion in 9696 von 9696 Faellen; r_lodscale 30
    ungekappt (G2_ComputeLOD: Stufe 1, R_ComputeLOD: 0).
  - Probe (Selbsttest geschuetze, duel_yavin, Schwertregal saber_w.md3 + saber_w_1.md3, Radius 9,34): Kamera 200 vor dem Regal
    md3-Stufen 0:12, 450 davor 0:1 1:11 (Grenze laut Formel 293). Vorher lud die Vorschau keine _1/_2.md3 (immer Stufe 0).
  - Kein Spielvergleich der Stufe (die Regale stehen in einer Nische, ein Bild mit Stufenwechsel ist nicht aufgenommen).
- **Fahrzeuge**: im Einzelspieler eigene NPC-Entities mit der Regel jeder Figur - s.radius aus "height" der .npc
  (NPC_stats.cpp:3659-3686 MD9) bzw. mins/maxs (G_SetG2PlayerModelInfo); figurLodRadius rechnet das schon. "g2radius" der .veh
  liest der Einzelspieler nur fuer die Radaranzeige (cg_draw.cpp:10065 MD9), als Zeichenradius (s.g2radius) gilt er nur im
  Mehrspieler. Kommentar ergaenzt, keine Rechenaenderung.
- **Spieler im AT-ST** (G_DriveATST, s.radius 320): offen. Die Vorschau kennt das Einsteigen nicht - misc_atst_use verlangt den
  Spieler selbst als Ausloeser und auf dem AT-ST stehend (groundEntityNum, g_misc.cpp:2972-2988 OpenJK); die Vorschau steuert den
  Spieler nicht. Der fahrbare AT-ST selbst hat 320 (seit REST3b).

## Punkt 3: Glanzband am Unterarm - die Normale dreht nur der erste Knochen (8bf67eb83)

- Eingegrenzt gegen das Spielbild (Testkarte p27f, Protokolldroide, Kamera 160 60 150 / 0 90): das Band ist die Umgebungsstufe
  (gfx/effects/chr_inv, tcGen environment), nicht der Glanz (`r_detailtextures 0` nimmt die Glanzstufe weg, der Unterschied
  bleibt, goldfarben), nicht die anisotrope Filterung (`r_ext_texture_filter_anisotropic 0`: Unterarm 29,50 -> 29,58). Das Band war
  verschoben, nicht heller - also eine andere Normale.
- Ursache: G2_SkinSurface (code/rd-vanilla/tr_ghoul2.cpp:3799-3803 MD9; OpenJK RB_SurfaceGhoul :2992-3002) dreht tess.normal
  NUR mit dem ersten Knochen der Ecke, ungewichtet - der gewichtete Zweig (:2941-2985 OpenJK) steht unter `#if 0`. Die Vorschau
  mischte die Normalen nach den Gewichten (seit rc608 Teil 7 so beschrieben). Jetzt der erste Knochen, auch mit Gewicht 0
  (packeEcken behaelt seine Nummer).
- Rot/Gruen gegen das Spielbild (screenshot_tga): Figur scale 150 4,21 -> 1,24 von 255 (Bildpunkte ueber 40: 504 -> 34), Unterarm
  16,86 -> 2,08; ctest gpushader prueft den Shader (Normale nur vom ersten Knochen, keine gewichtete Summe).
- Gegenprobe auf der Testkarte p27e (nicht zum Finden benutzt: zwei Protokolldroiden scale 100/150 in 500 Einheiten, neues
  Spielbild): an den 8297 Bildpunkten, die sich durch die Aenderung aendern, alte exe 5,42 -> neue 1,62 von 255.
  figurlicht 8/0 mit der neuen Normale.

## Punkt 4: Ungleicher Massstab je Achse - gemessen, gleich gut wie gleicher Massstab

- Testfigur p27_gross mit scaleX 300 / scaleY 200 / scaleZ 150 (NPC-Datei im eigenen fs_homepath bzw. eigener pk3
  `Y:\werkzeuge\r3d\p27u_pk3`, Karte p27f): Figur 48643 Bildpunkte, mittlere Abweichung 3,99 von 255. Gegenprobe gleicher
  Massstab scale 150, sonst gleich: 34152 Bildpunkte, 4,21. Kein zusaetzlicher Fehler durch den ungleichen Massstab (die Normale
  diag(1/s) R^T aus REST3c gilt). Nach Punkt 3: ungleich 1,24, gleich 1,24.

## Schlussrunde

- Sauberer Neubau (--clean-first, nach dem letzten Code-Commit) 0 Warnungen, 0 Fehler. Parallel (`Y:/werkzeuge/r3d/schluss.sh`,
  Summe `schluss_summe.txt`): ctest 245/245, Modus 1 2635/0, mission1 98/0, figurlicht 8/0, karteneditor rd 38/0, r3 22/0,
  editorportal 19/0, laserfalle 9/0, gluehen 9/0, modellblock md 86/0, funktionen 61/0, rest605 moversprite 4/0. Lints wie die
  Basis (portability 99, pruefcodes 11, fremdprogramm 1, link/tidy nur unter Linux), Handbuch `--pruefen` ok (inhaltlich nicht
  betroffen). Keine Bildzeit-Messung (Auftrag). Ein erster Schlusslauf (mission1/Modus 1) wurde fuer Punkt 3 abgebrochen
  (eigene PIDs, taskkill /T, nachgeprueft) und nach dem Neubau vollstaendig wiederholt.

## Changelog

## English

- Preview and map editor: map models in .md3 with level-of-detail files (name_1.md3, name_2.md3) switch levels with the distance like the game.
- Preview: shine and chrome reflections on figures sit where the game draws them - a vertex between two bones takes its normal from the first bone, like the game (e.g. the band on the protocol droid's forearm).

## Deutsch

- Vorschau und Karten-Editor: Kartenmodelle als .md3 mit Detailstufen-Dateien (Name_1.md3, Name_2.md3) wechseln die Stufe mit dem Abstand wie im Spiel.
- Vorschau: Glanz und Chromspiegelung auf Figuren liegen dort, wo das Spiel sie zeichnet - eine Ecke zwischen zwei Knochen nimmt ihre Normale wie im Spiel vom ersten Knochen (z. B. das Band am Unterarm des Protokolldroiden).

## 中文

- 预览和地图编辑器:带细节级别文件(name_1.md3、name_2.md3)的 .md3 地图模型会像游戏一样随距离切换级别。
- 预览:角色上的光泽和镀铬反射位于游戏绘制的位置——位于两根骨骼之间的顶点像游戏一样从第一根骨骼取法线(例如礼仪机器人前臂上的光带)。

## 日本語

- プレビューとマップエディター: 詳細レベルのファイル (name_1.md3、name_2.md3) を持つ .md3 のマップモデルは、ゲームと同じく距離でレベルを切り替えます。
- プレビュー: キャラクターの光沢とクロームの映り込みがゲームと同じ位置に出ます。2 本のボーンの間の頂点は、ゲームと同じく最初のボーンから法線を取ります (例: プロトコルドロイドの前腕の帯)。

## Español

- Vista previa y editor de mapas: los modelos de mapa en .md3 con archivos de nivel de detalle (name_1.md3, name_2.md3) cambian de nivel con la distancia como en el juego.
- Vista previa: el brillo y el reflejo cromado de las figuras quedan donde los dibuja el juego; un vértice entre dos huesos toma su normal del primer hueso, como en el juego (p. ej., la franja del antebrazo del droide de protocolo).

## Čeština

- Náhled a editor map: modely mapy ve formátu .md3 se soubory úrovní detailů (name_1.md3, name_2.md3) mění úroveň podle vzdálenosti jako ve hře.
- Náhled: lesk a chromový odraz na postavách leží tam, kde je kreslí hra - vrchol mezi dvěma kostmi bere normálu jako ve hře z první kosti (např. pruh na předloktí protokolárního droida).
