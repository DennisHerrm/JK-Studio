# Notizen REST6: duel_exegol - eingebackene Statue, helleres Spielbild, Flaechenlicht (09.11.2026, `Y:\karte`, Zweig `karte1`, Basis 8679be79b)

Build `Y:/karte/build-test` (`--parallel 8 -- /nodeReuse:false`), Laeufe `r6_*` unter `Y:\karte\laeufe`, Skripte und Protokolle
`Y:\werkzeuge\r6\`. Spielbilder ueber `screenshot_tga` (`Y:\werkzeuge\r3d\spiel.py`, `R3D_ECHT="r_weather 0"`), Movie Duels per Popen,
eigener fs_homepath, Spielordner jedes Mal unveraendert. Quellen: Movie Duels `Y:\werkzeuge\sp_654alt\md9`, OpenJK `Y:\werkzeuge\sp_af9c\m3\ojq\src`.

## Punkt 1: rgbGen lightingDiffuse auf der Welt ist schwarz (1cd0c1b23)

- Der Shader der Statue: `models/map_objects/rift/statue` steht zweimal, in MD_Maps_ST.pk3 `shaders/exegol.shader` (rgbGen vertex, glow-Stufe)
  und in MD_Assets.pk3 `shaders/rift_statuefix.shader` (q3map_nolightmap, `blendFunc GL_ONE GL_ZERO`, `rgbGen lightingDiffuse`). Im Spiel
  gilt der erste Eintrag im umgekehrt verketteten Text (ScanAndLoadShaderFiles tr_shader.cpp:3781 MD9, ShaderEntryPtrs_Insert tr_stl.cpp:51,
  FS_ListFiles in der Reihenfolge der Suchpfade files.cpp:2134) - rift_statuefix. Die Vorschau nimmt denselben (sortiert, der letzte gilt).
- Die Welt zeichnet das Spiel mit backEnd.currentEntity = &tr.worldEntity (tr_backend.cpp:705/:834 MD9, :671/:799 OpenJK). Dessen Licht ist null
  (memset(&tr) in R_Init, tr_init.cpp:1907 MD9, :1694 OpenJK, nirgends gesetzt): RB_CalcDiffuseColor gibt incoming = 0 -> ambientLightInt = 0,
  alle vier Bytes (tr_shade_calc.cpp:1053-1097 MD9, :1106 OpenJK); lightingDiffuseEntity ebenso mit Alpha 255 (tr_shade.cpp:1529-1537 MD9).
  Die Vorschau nahm fuer lightingDiffuse auf der Welt die Eckenfarbe (rgbVertex).
- Neu: ShaderInfo/Tex/BatchState tragen das rgbGen der Bildstufe (`rgbArt`), BspMesh::weltEntity markiert das Netz der Welt (buildMesh,
  buildModelMesh 0); buildDrawCalls zeichnet solche Stufen mit fester Farbe 0 (weltDiffus/weltDiffusSetzen, gpustate.h), zeichneAufrufe
  setzt die Konstanten ebenso. Brush-Entities bleiben: R_AddBrushModelSurfaces ruft R_SetupEntityLighting nur fuer bspInstance
  (tr_world.cpp:345-348 MD9), sonst steht im trRefEntity_t, was dort zuletzt stand - nicht nachbildbar.
- Alpha: gesetzt (lightingDiffuse 0, AGEN_SKIP tr_shader.cpp:1736-1741 MD9 / :1770-1776 OpenJK) nur, wo es wirkt - Quellalpha im Mischfaktor
  oder alphaFunc. Den Alphakanal des Bildspeichers schreibt die Vorschau nicht (holeBlend, nur RGB).
- Rot/Gruen: neue Probe im Selbsttest `rest605` (BHED_REST=moversprite) "Weltflaeche mit lightingDiffuse: die Statue von duel_exegol ist schwarz
  wie im Spiel" (Median der Bildpunkte unter den Mitten der 2 Statuenflaechen, ohne Nebel und Gluehen): rot `r6_ms_rot` Median 0 1 3
  (529 Bildpunkte), gruen `r6_ms_gruen` 0 0 0, 5 OK 0 FEHLER. ctest gpudraw (6 neue Pruefungen: Brush-Modell, deckend, GL_SRC_ALPHA,
  lightingDiffuseEntity, alphaGen vertex, rgbGen vertex) und figurstufen (rgbArt aus rift_statuefix.shader) gruen.
- Gegen das Spiel (Kamera 1500 -5000 600 / -5 18, r_drawfog 0, r_DynamicGlow 0; `r5stA` gegen `r6_stA`, `statue_mittel.py`): Statue Spiel
  0,00/0,01/0,09, Vorschau vorher 0,76/3,50/7,53, jetzt 0,01/0,02/0,09; ganzes Bild Spiel 0,02/0,18/0,93, Vorschau vorher 0,08/0,63/2,22, jetzt
  0,03/0,45/1,83.
- Gegenprobe auf einer Karte, die nicht zum Finden diente: duel_yavin, `models/map_objects/yavin/light` (erste Stufe GL_ONE GL_ONE glow
  lightingDiffuse) an der Wand, Kamera -2722 2400 295 / 0 90, r_DynamicGlow 0 (`r6yav` gegen `r6_yav_rot` mit der alten exe, `r6_yav`):
  Lampe links Spiel 144,5/110,3/61,2, vorher 163,8/148,2/87,2, jetzt 143,9/110,0/60,9; rechts 135,7/101,0/54,2, vorher 160,3/143,4/80,8, jetzt
  134,9/100,7/54,0 (`yav_drei.png`). Betroffen sind viele Karten: Weltflaechen mit lightingDiffuse in 91 von 282 Karten der Movie-Duels-Installation und 30 von 57
  Original-JKA-Karten (`lm_none_scan.py`, z. B. t3_byss brace_light, vjun3 glass_b, duel_jabba bcpanel).
- Nebenbefund (offen): mit `kAlphaKonst` an der deckenden Statuenstufe (alphaGen const, egal ob 0 oder 1) wurde das ganze Vorschaubild
  hell und dunstig (stA ganz 22/24/26 statt 0,03/0,45/1,83, auch abgewandt kein Unterschied in den Effekten); ohne das Bit richtig.
  Ursache nicht gefunden (Mischzustand, Konstanten und Shader-Variante fx_rc_ak_re geprueft). Eine deckende Stufe mit alphaGen const auf der
  Welt koennte denselben Fehler zeigen.

## Punkt 2: helleres Spielbild mit r_showcluster / r_novis - der Blitz der Karte

- Nicht die Render-Variablen: in `r5stB/D/E` ist nur das erste Bild (r3ba) heller, das zweite 1 s spaeter (r3bb) ist bytegleich mit den Laeufen
  ohne die Variablen (`r5stA/C`, groesste Abweichung 0).
- Neuer Lauf ohne zusaetzliche Variable, 40 Bilder im Abstand 0,5 s (`r6stF`): r3ba 0,02/0,71/1,94, Bilder 25-28 0,89/2,26, 0,74/1,99, 0,69/1,91,
  0,69/1,91, alle anderen 0,02/0,18/0,93 - der Blitz der Karte. Die Lichter `flickernow0/2` (Stile 32/33, switch_style 6/1, spawnflags 4 =
  aus) schaltet func_timer (wait 6, random 1,5) ueber target_random, target_relay (delay 1) und target_delay (0,55 +- 0,35) an und aus
  (SP_light, misc_dlight_use, misc_lightstyle_set g_misc.cpp:114-201 MD9); die Stilmuster flackern (darum die verschiedenen Werte).
- Die Vorschau bei 4000 ms (Lichtstil 32/33 0,00) entspricht dem dunklen Zustand des Spiels. Die Messungen von REST4 (`r4exA/B`) liegen nicht im
  Blitz (r3ba = r3bb), die Werte gelten.

## Punkt 3: Flaechenlicht neu verglichen - Rest auf einer Wandart, offen

- Ohne Nebel (`r4exB` gegen `r6_exB`): Spiel 0,00/0,47/2,15, Vorschau 0,01/0,85/3,48 (vorher 0,9/3,5; die Statue ist hier klein). Mit Nebel
  (`r4exA`/`r6_exA`): 4,60/8,77 gegen 5,23/10,02.
- Eingegrenzt: Boden (`textures/exegol_sh/floor_1`) Spiel 0,77/3,62, Vorschau 0,61/3,02; Felsen gleich; die rechte Wand und der Pfeiler
  (`textures/exegol/front`, Strahl `strahl.py`) Spiel 0,10/1,42, Vorschau 0,82/4,87 - 3,4-fach. Nicht von den Effekten (BHED_GV_EFFEKTE=0
  gleich), nicht vom Nebel, nicht vom Blitz.
- Die detail-Stufen ($lightmap mit tcMod transform 0,333/0,666, die Stillagen 32/33 im Atlas) tragen im Spiel wenig bei: mit
  `r_detailtextures 0` (`r6exD`, r3bb) Wand 0,10/1,27 (Faktor 1,12), Boden 0,70/2,50 (1,45) - wie aus den Lightmap-Texeln gerechnet
  (Wand 0,07, Boden 0,19 an den verschobenen Stellen). Aus Bild mal Lightmap ueber die Flaechen gemittelt: Wand/Boden in Blau 0,6; das Spiel
  zeigt 0,39, die Vorschau 1,6 - die Vorschau weicht an `textures/exegol/front` ab, nicht das Spiel.
- Ursache nicht gefunden. Naechster Schritt: in der Vorschau die Stufen von `textures/exegol/front` einzeln an einem Bildpunkt der Wand
  verfolgen (Grundstufe Lightmap x sith_wall, die drei Lightmap-Stufen) gegen dieselbe Rechnung aus den Daten.

## Schluss (`Y:\werkzeuge\r6\schluss.sh`, Summe `schluss_summe.txt`)

- (siehe Bericht)

## Offen

- Punkt 3: Vorschau an `textures/exegol/front` 3,4-fach zu hell, Ursache offen (siehe oben).
- Punkt 1 Nebenbefund: kAlphaKonst an einer deckenden Weltstufe macht das Vorschaubild dunstig.
- Shader-Reihenfolge allgemein: die Vorschau liest die .shader-Dateien nach Namen sortiert und laesst den letzten Eintrag gelten; das Spiel
  nimmt den ersten im umgekehrt verketteten Text (Suchpfade: das Archiv mit niedrigerem Vorrang gewinnt bei gleichem Shadernamen in
  verschiedenen Dateien). Fuer die Statue gleich, allgemein nicht geprueft.

## Changelog

## English

- Preview: surfaces of the map itself whose shader uses rgbGen lightingDiffuse are black like in the game (the statue in duel_exegol, the lamp bulbs of duel_yavin now only glow).

## Deutsch

- Vorschau: Flächen der Karte selbst, deren Shader rgbGen lightingDiffuse nutzt, sind wie im Spiel schwarz (die Statue in duel_exegol, die Lampen von duel_yavin leuchten nur noch mit ihrem Glühen).

## 中文

- 预览：着色器使用 rgbGen lightingDiffuse 的地图本身表面与游戏中一样为黑色（duel_exegol 中的雕像；duel_yavin 的灯泡现在只靠辉光发亮）。

## 日本語

- プレビュー：シェーダーが rgbGen lightingDiffuse を使うマップ自体のサーフェスは、ゲームと同じく黒になります（duel_exegol の像、duel_yavin の電球はグローだけで光ります）。

## Español

- Vista previa: las superficies del propio mapa cuyo shader usa rgbGen lightingDiffuse son negras como en el juego (la estatua de duel_exegol; las bombillas de duel_yavin ahora solo brillan con su resplandor).

## Čeština

- Náhled: plochy samotné mapy, jejichž shader používá rgbGen lightingDiffuse, jsou černé jako ve hře (socha v duel_exegol, žárovky v duel_yavin nyní svítí jen svou září).
