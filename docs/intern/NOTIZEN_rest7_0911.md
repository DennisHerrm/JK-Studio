# Notizen REST7: Shader-Vorrang bei doppelten Namen, duel_exegol-Wand (09.11.2026, `Y:\karte`, Zweig `karte1`, Basis d28f9e02e)

Build `Y:/karte/build-test` (`--parallel 8 -- /nodeReuse:false`), Laeufe `r7_*` unter `Y:\karte\laeufe`, Skripte und Protokolle
`Y:\werkzeuge\r7\`. Spielbilder ueber `screenshot_tga` (`Y:\werkzeuge\r3d\spiel.py`, `R3D_VOR="seta r_DynamicGlow 0;seta r_drawfog 0"`,
`R3D_ECHT="r_weather 0"`), Movie Duels per Popen, eigener fs_homepath, Spielordner jedes Mal unveraendert. Quellen: Movie Duels
`Y:\werkzeuge\sp_654alt\md9`, OpenJK `Y:\werkzeuge\sp_af9c\m3\ojq\src`, Jedi Outcast 1.04 im OpenJK-Verlauf a09d4827
(`Y:\werkzeuge\sp_654alt\ojk`, codeJK2/renderer, codeJK2/qcommon).

## Punkt 1: Shader-Vorrang wie im Spiel (b905b7a1c)

- Spiel: ScanAndLoadShaderFiles (tr_shader.cpp:3715-3802 MD9, :3760-3820 OpenJK) liest FS_ListFiles("shaders", ".shader")
  (FS_ListFilteredFiles files.cpp:2134-2250 MD9: Suchliste hoechster Rang zuerst, je .pk3 in Zip-Folge, lose Dateien je Ordner
  ohne Unterordner, in .pk3 bis zwei Ebenen, eindeutig ohne Gross/klein, hoechstens 4095), haengt die Dateien UMGEKEHRT aneinander
  (strcat: bis zum ersten Nullbyte, danach "\n"), COM_Compress, SetupShaderEntryPtrs (:3667-3701) behaelt den ERSTEN Namen
  (ShaderEntryPtrs_Insert tr_stl.cpp:51-63). Also gewinnt die spaeter gelistete Datei (die .pk3 mit dem niedrigeren Rang, zuletzt
  die losen Dateien von base) und in einer Datei der erste Eintrag. MAX_SHADER_FILES MD9 8192 (:3710), OpenJK 4096 (:3759).
- Jedi Outcast 1.04 (codeJK2/renderer/tr_shader.cpp:3373-3466, qcommon/files.cpp:1448-1535 a09d4827): MAX_SHADER_FILES 1024,
  "\n" VOR jeder Datei, KEIN COM_Compress (ein mehrzeiliger Kommentar hinter einem Namen wird ab der zweiten Zeile zu Namen), sonst
  gleich; kein fs_dirbeforepak.
- Vorschau bis REST7: je Spielordner alle .shader der .pk3 (auch ausserhalb von shaders/, ohne lose Dateien), nach Namen sortiert,
  der letzte Eintrag gewann (auch in einer Datei); Menue-Editor: Dateifolge schon wie das Spiel, in einer Datei aber der letzte.
- Neu: `fsListFiles` (bhed/pk3.h), `shaderTextWieSpiel`/`parseShaderTextWieSpiel`/`maxShaderDateien` (bhed/shaderscript.h, Ports von
  COM_Compress, COM_ParseExt, SkipBracedSection, SkipRestOfLine, SetupShaderEntryPtrs; ParseShader ohne "{" -> kein Eintrag, verdeckt
  aber). `shaderTabelleLesen` (app_ablage.inc) ersetzt die zwei gleichen Leseschleifen (Welt und Figuren); Menue-Editor und
  Kartenpruefung (`shaderTexte`) nehmen dieselbe Regel. Jeder geltende Eintrag wird fuer sich zerlegt, Klammern als eigenes Wort
  auf eigener Zeile - dabei gefunden: eine Zeile `{ map textures/tatooine_outpost/sandblendtexture` liess den Zeilenleser im
  zusammengehaengten Text alle folgenden Shader verlieren (erster Gruenlauf: 5655 statt 9559 Eintraege); jetzt 9583 Eintraege
  (9898 Namen, 263 verdeckte Doppel).
- Gezaehlt (`doppelt.py`, eigener Port derselben Regeln in Python): Movie Duels 624 Dateien (alte Vorschau las 630), 253 Namen
  mehrfach, 233 davon in verschiedenen Dateien, 76 mit verschiedenem Inhalt; 45 Shader haben jetzt einen anderen Gewinner mit
  anderem Inhalt (Liste `Y:\werkzeuge\r7\md.txt`, Zeilen W; auf Weltflaechen u. a. duel_coruscantcity tower2, md_arena1-3
  t_wall_back3/t_floor_cir, duel_carbon/duel_cloudcity/md_sn_intro_jedi impdetention/metal2, duel_jeditemple bridgefloor2,
  duel_yavin light_yellow). Original-JKA: 68 Dateien, 26 doppelte Namen (alle in verschiedenen Dateien), kein Gewinner wechselt.
  Kein Gewinner ohne "{" danach, keine Datei mit offener Klammer.
- Rot/Gruen: Selbsttest rest605 (BHED_REST=moversprite) "Shader-Vorrang: bei doppelten Namen gilt der Eintrag des Spiels"
  (bridgefloor2 erste Stufe env_shiny_floor, light_yellow mit glow, t_floor_cir 3 Stufen): rot `r7_ms_rot` (Stufe '', glow 0,
  4 Stufen), gruen `r7_ms_gruen2` 7 OK 0 FEHLER. ctest shadervorrang (neu, 22 Pruefungen mit kuenstlichen .pk3/losen Dateien:
  Folge fs_dirbeforepak 0/1, Rang, Ebenen, Nullbyte, offene Klammer, Name ohne "{", "{ map" auf einer Zeile, Jedi-Outcast-Kommentar,
  MAX_SHADER_FILES, alte Regel als Gegenprobe) gruen.
- Nicht umgestellt: das Echtzeit-Licht des Karten-Editors (`echtShaderInfo`) zerlegt die Datei, die das Shader-Inventar wie
  NetRadiant-Custom waehlt (erste Definition in NRCs Ladefolge, texturen.cpp:689) - der Editor folgt dort NRC und q3map2.

## Punkt 2: duel_exegol, Wand textures/exegol/front 3,4-fach zu hell (e49ccf8ca)

- Nicht Punkt 1: textures/exegol/front steht nur einmal (MD_Maps_ST.pk3 exegol.shader).
- Stufen an den Bildpunkten der Wand aus den Daten gerechnet (`pixel.py`: Strahl je 16. Bildpunkt der Spielkamera exB -600 -5952 208
  / 10 0, st und Lightmap-Koordinaten der Ecken, sith_wall und lm_0000.tga bilinear; `sim.py`: jede Stufe auf 8 Bit gerundet wie der
  Bildspeicher, Gamma-Tabelle wie spielGammaWert): Lightmap 4096 x 4096 extern, die Stillagen 32/33 liegen bei +0,333/+0,666 auf
  derselben Seite (genau die tcMod-transform-Stufen). Ergebnis (G/B, 3104 Bildpunkte): Spiel 0,318/2,421; Vorschau 1,149/5,651;
  Rechnung alle Stufen mit Lightmap-Alpha 255: 1,202/5,706 (= Vorschau); nur Grundstufe 0,32/1,93; alle Stufen mit dem Alpha aus
  lm_0000.tga: 0,32/2,415 (= Spiel).
- Ursache: R_FindLightmap (tr_shader.cpp:3325-3360 MD9) laedt externe Lightmaps mit R_FindImageFile - mit Alphakanal, ohne
  R_ColorShiftLightingBytes. lm_0000.tga ist 32 Bit, Alpha im Mittel 23,72 (an der Wand 8). Die Stufe `map $lightmap` /
  `blendFunc GL_SRC_ALPHA GL_ONE` / `alphaGen const 0.125` mischt mit Bildalpha x 0,125. Die Vorschau lud externe Lightmaps nur als
  RGB und setzte im Pixelshader Alpha 1,0. Jetzt: BspGeometry::lightmapAlpha (ladeExterneLightmaps, nur bei Alphakanal),
  holeLightmap laedt es hoch, kLightmapBild nimmt das Alpha der Lightmap (aus der .bsp weiter 255 wie R_LoadLightmaps).
  Betroffen: nur Karten mit 32-Bit-lm_*.tga - in den Daten duel_exegol und md_ch_palp_ep9 (`lmscan.py`; 9 weitere externe sind
  24 Bit); 27 exegol-Shader haben diese Stufe.
- Rot/Gruen: ctest gpushader "kLightmapBild: das Alpha kommt aus der Lightmap" rot (1 FEHL), gruen; Selbsttest moversprite
  "Externe Lightmap: das Alpha aus lm_0000.tga bleibt erhalten" (4096 x 4096, Mittel 23,7226) gruen in `r7_ms_gruen2`.
- Flaechenlicht neu (Spiel r4exB/r4exA, Vorschau `r7_exB2`/`r7_exA2`): ohne Nebel Spiel 0,48/2,16 (G/B), Vorschau 0,43/2,02
  (vorher 0,85/3,48); mit Nebel 4,60/8,76 gegen 4,86/8,95 (vorher 5,23/10,02). Blitz: die Spielbilder sind r3bb (nicht im Blitz,
  REST6 Punkt 2), die Vorschau bei 4000 ms (Stil 32/33 aus).
- Gegenprobe auf einer Karte, die nicht zum Finden diente: md_ch_palp_ep9 (Kamera 0 -2304 100 / 5 270, `.ent` ohne die
  Einleitungs-Kamera `md_ch/ep9palpatine_intro` ueber den eigenen fs_homepath, 12 Bilder; ab r3cr04 ruhig, r3cr06 genommen):
  Spiel 0,06/1,84/4,49; Vorschau alt (Exe vor REST7) 0,07/2,18/5,90, neu bei 500 ms 0,06/1,81/4,45 (alle 12 Felder gleich,
  `t1.png`). Bei 4000 ms ist die Grube in der Vorschau hell: der Blitz der Karte (func_timer random 1.5 -> target_random ->
  flickernow-Lichter Stil 32/33), dessen Zufall in Spiel und Vorschau verschieden faellt - kein Fehler der Zeichnung.

## Punkt 3 und Schluss (`Y:\werkzeuge\r7\schluss.sh`, Summe `schluss_summe.txt`)

- Sauberer Neubau (`--clean-first`) 0 Warnungen (1126 s); ctest 245/245 (-j 12 ohne q3map2_pr316), q3map2_pr316 1/1.
- Parallel: figurlicht 8/0, mission1 98/0, karteneditor voll 3391/0, editorportal 19/0, laserfalle 9/0, gluehen 9/0,
  rest605 moversprite 7/0, modellblock funktionen 61/0 und md 86/0, menueeditor 2787/0, Modus 1 2635/0.
- Lints wie die Basis (portability 99, pruefcodes 11, efxflags-/fremdprogramm-Meldung bekannt, link/tidy nur unter Linux, sonst ok).
  Handbuch nicht betroffen. Keine Bildzeit-Messung (Auftrag).

## Offen

- Kartenpruefung (kp) nicht gelaufen (Eingabe durch Punkt 1 geaendert) - in der Release-Runde.

- Karten-Editor, Echtzeit-Licht: Shader mit doppeltem Namen nach NRCs Inventar statt nach der Spielregel (siehe Punkt 1).
- alphaGen const / rgbGen const: das Spiel speichert den Wert als Byte (constantColor, 255 * atof, abgeschnitten: 0,125 -> 31/255),
  die Vorschau rechnet mit dem Gleitwert (2,7 % bei 0,125); an der exegol-Wand nicht messbar.

## Changelog

## English

- Preview: when several shader files define the same shader name, the preview now uses the definition the game uses (the earlier file in the game's search order loses, and within one file the first definition wins); 45 shaders of Movie Duels looked different before.
- Preview: external lightmaps keep their alpha channel like in the game, so the walls of duel_exegol are no longer three times too bright.

## Deutsch

- Vorschau: Steht derselbe Shadername in mehreren Shader-Dateien, nimmt die Vorschau jetzt die Definition, die auch das Spiel nimmt (in einer Datei gilt die erste); 45 Shader von Movie Duels sahen vorher anders aus.
- Vorschau: Externe Lightmaps behalten wie im Spiel ihren Alphakanal, die Wände von duel_exegol sind nicht mehr dreimal zu hell.

## 中文

- 预览：当多个着色器文件定义同一个着色器名称时，预览现在使用游戏所用的定义（同一文件中以第一个定义为准）；此前 Movie Duels 中有 45 个着色器显示不同。
- 预览：外部光照贴图像游戏中一样保留其 Alpha 通道，duel_exegol 的墙壁不再亮三倍。

## 日本語

- プレビュー：同じシェーダー名が複数のシェーダーファイルにある場合、プレビューはゲームと同じ定義を使うようになりました（1つのファイル内では最初の定義が有効）。以前は Movie Duels の 45 個のシェーダーの見た目が違っていました。
- プレビュー：外部ライトマップがゲームと同じくアルファチャンネルを保つため、duel_exegol の壁が 3 倍明るすぎることはなくなりました。

## Español

- Vista previa: si varios archivos de shaders definen el mismo nombre, la vista previa usa ahora la definición que usa el juego (dentro de un archivo vale la primera); antes 45 shaders de Movie Duels se veían distintos.
- Vista previa: los lightmaps externos conservan su canal alfa como en el juego, así que las paredes de duel_exegol ya no salen tres veces demasiado claras.

## Čeština

- Náhled: pokud stejný název shaderu stojí ve více souborech shaderů, náhled nyní použije definici, kterou používá hra (v jednom souboru platí první); 45 shaderů Movie Duels předtím vypadalo jinak.
- Náhled: externí lightmapy si jako ve hře ponechávají alfa kanál, stěny duel_exegol už nejsou třikrát příliš světlé.
