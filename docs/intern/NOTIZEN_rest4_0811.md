# Notizen REST4: Lade-, Skin-, JK2-Reste und Statustabelle (09.11.2026, `Y:\karte`, Zweig `karte1`, Basis 3137d44ca)

Build `Y:/karte/build-test` (`--parallel 8 -- /nodeReuse:false`), Laeufe `r4_*` unter `Y:\karte\laeufe`, Skripte und Protokolle
`Y:\werkzeuge\r4\`. Engine-Abgleich: OpenJK `Y:\werkzeuge\sp_654alt\ojk` (code/, codeJK2/, Raven-JK2 im Verlauf a09d4827),
Movie Duels `Y:\werkzeuge\sp_654alt\md9`.

## Punkt 1: Statustabelle P1-P62

- Angehaengt an `JKStudio_Uebergabe/plaene/codepruefung_seit_rc610.md` ("Statustabelle P1-P62"): alle 62 erledigt (Commit je
  Fund), keiner offen. Nur Messungen der Schlussrunde: P6/P15 (`ptzeit`), P9, P11, P28, P38, P47.
- Die Reste P45 (Spielbeleg), P53 (CommaParse, JK2) und P54 (Original-JKA) sind REST4 Punkte 5, 3 und 6.

## Punkt 2: Bildregister vollstaendig (6eec507b8)

- `bhed/vorwelt.h`/`src/vorwelt.cpp`: die festen Registrierungen eines frischen Spielstarts je Regelwerk aus den Quellen -
  CL_InitRenderer (R_Init mit CreateExternalShaders projectionShadow/sun, gfx/2d/charsgrid_med, Schrift ocr_a nur OpenJK/JK2_MODE,
  white, console), UI_Init(inGameLoad qfalse) mit ui/menus.txt, white, AssetCache (Movie Duels dazu cursor_* und die Rahmen der
  Figurenwahl, NEW_FEEDER); SV_SpawnServer: RE_SVModelInit (R_InitShaders - Shadertabelle neu, Bilder bleiben: `shaderVergessen`);
  nach dem Server CL_StartHunkUsers (SCR_UpdateScreen) mit Renderer und UI_Init(inGameLoad: ui/ingame.txt, JK2_MODE weiter
  menus.txt); dann NPC_Begin (Serverbild 1500, vor dem Spielstand: Com_Frame laeuft SV_Frame vor CL_Frame, der Spielstand kommt
  erst im naechsten Com_EventLoop) mit den Waffen der Figuren; CG_Init mit Schriften, Ladebalken, Kraftsymbolen (MD 26, JK2 11),
  HUD-Menues (ui/jahud.txt, JK2 ui/jk2hud.txt), gfx/hud/mp_levelload, Modell und Effekte aus CG_RegisterSounds; CG_RegisterEffects.
- Menues/HUD (`menueListeRegistrieren`): Liste und Menues mit dem Menue-Parser, Schluessel in der Folge der Datei wie UI_ParseMenu -
  assetGlobalDef small/medium/bigFont (RE_RegisterFont: "fonts/<Name>" ohne Mipmaps auch ohne .fontdat, UI_RegisterFont mit
  Rueckfall ergoec), gradientbar, cursor; menuDef background, font (nur ohne fontRegistered); itemDef background, asset_shader,
  asset_model, model_g2skin.
- NPC-Klassen: NPC_PrecacheByClassName (OpenJK/MD9 je Name, nur ohne targetname und delay > 0) bzw. die Klassen-Spawnfunktionen von
  Jedi Outcast (immer); nur G_EffectIndex wirkt vor der Welt (RegisterItem schreibt nur CS_ITEMS, G_ModelIndex erst
  CG_RegisterGraphics).
- Fahrzeuge (`fahrzeugLaden`): VEH_LoadVehicle im SP-Spielmodul (QAGAME) - COM_ParseExt/SkipRestOfLine/SkipBracedSection
  (`ComLeser`), Effekte der .veh in Dateifolge, dann die der Waffen (.vwp), Haeute sofort (alle aus "a|b", sonst model_default),
  dann ship_explosion_mark (explosionDamage != 0), swoop_dust (hoverHeight > 0), black_smoke, fire; VF_SHADER registriert das
  Spielmodul nicht. Dateiliste wie FS_GetFileList mit 2048-Zeichen-Puffer. NPC_Vehicle spawnt ohne targetname sofort
  (G_VehicleSpawn -> NPC_Spawn_Do), nicht bei 1350; mit targetname laedt BG_VehicleGetIndex sofort.
- Jedi Outcast: RE_RegisterSkin gibt ohne cls.cgameStarted 1 zurueck und laedt nichts (Raven-JK2 tr_image.cpp:2134) - Haeute des
  Servers und des Hauptmenues erst nach der Welt.
- Himmel nach einer gescheiterten Stufe: der Shader ist defaultShader - keine Himmelsbox, die Flaechen zeigen tr.defaultShader.
- "Vorige Karte": in der Vorschau gibt es keine vorige Karte - jede Karte laedt wie nach einem frischen Spielstart (im Spiel
  ueberlebt AllocatedImages den Kartenwechsel, R_DeleteTextures nur bei vid_restart, RE_RegisterImages_LevelLoadEnd raeumt nur
  Bilder, die die neue Karte nicht benutzt).
- Rot `r4_p2rot` (neue Pruefungen, altes Laden; geschuetze t1_sour, OpenJK, DXT): 18 OK / 3 FEHLER - "AssetCache - Stufen 7, Klammern 0,
  zuerst geladen von textures/desert/stucco_portalsky1", Himmel "Seiten 6, Kasten 0", Menue (Probe fand noch keinen Hintergrund; altes
  Laden: 40 Bilder vor der Welt, keins aus den Menues). Gruen `r4_p2gr3`: 21/0 - "Stufen 1, Klammern 1, zuerst geladen von
  gfx/menus/scrollbar_thumb", Menue "gfx/menus/menu_side_text zuerst geladen von gfx/menus/menu_side_text" (Skript: dessen Mipmaps),
  Himmel "Seiten 0, Kasten 1"; 260 Bilder vor der Welt. ctest bilder: 10 neue Pruefungen (ComLeser, NPC-Klassen je Regelwerk,
  Fahrzeug, Phasen) gruen.
- Nicht nachgebaut (Laufzeit, nicht beim Laden festgelegt): was Menues erst beim Oeffnen zeichnen (Ownerdraw, Levelshots,
  Script_SetBackground), asiatische Schriftbilder (Sprache Englisch), die Haut eines Schwerts (SaberArt kennt keine).

## Punkt 3: Haeute (6c5ea5a86)

- `hautEintraege` (glm.h) liest wie RE_RegisterIndividualSkin mit CommaParse: eine tag_-Zeile liest nur Namen und Komma (ihr Shader
  wird der naechste Flaechenname, alle Woerter danach verschieben sich), eine Zeile ohne Komma nimmt das erste Wort der naechsten,
  ein Wort beginnt auch mit einem Komma (do-while), Leerraum trennt, // nach dem Shader gehoert nicht dazu, Namen auf 63 Zeichen.
  applySkin und BildRegister::haut nehmen es.
- Jedi Outcast wie Raven-JK2: strstr "tag_", kein "_off", 48 Eintraege (skin_t::surfaces[MD3_MAX_SURFACES/2] = 32 + 32/2), Zeichen ab
  0x80 Leerraum, Wort mit 1024 Zeichen verworfen; keine dreiteiligen Haeute (G_SetSkin baut model_<customSkin>.skin);
  G2API_SetSkin setzt nur mCustomSkin: "*off" ist ein Shader ohne Bild (zeichnet nichts), eine ungenannte Flaeche zeichnet
  tr.defaultShader (tr_ghoul2.cpp:1526-1539 a09d4827; JKA nimmt den Shader der .glm).
- Ein Hautshader ohne Skript und ohne Bild zeichnet nichts (R_FindShader: defaultShader ohne Stufe, SS_FOG) statt der Ersatzfarbe.
- In den Spieldaten: 11 Haeute (Movie Duels jedi_spanki*b torso_a2.skin: "r_handmodels/players/jedi_spanki1b/arms.tga" ohne Komma)
  verschieben ihre Woerter im Spiel so (`Y:\werkzeuge\r4\hautsuche.py`, 5004 Haeute durchsucht).
- Rot (`p3_rot_glmtest.txt`/`p3_rot_imagetest.txt`): modell 7 FEHL, bilder 2 FEHL; gruen 0/0. Zwei alte Pruefungen auf die Regel
  der Engine gestellt (tag_-Zeile ohne Shaderwort; JK2 "arm_off,*off" gibt den Shader *off).

## Punkt 4: Jedi Outcast

- NPC_BuildRandom (codeJK2 NPC_stats.cpp:965-1054, Q_irand(87, 102) / 100) laeuft nur fuer NPC_type "random" und baut eine
  md3-Figur aus Elite-Force-Teilen (garren, crewthin ...). md3Model bleibt qtrue, CG_RegisterClientModels -> CG_RegisterClientRenderInfo
  findet die md3 nicht (in assets0-5.pk3 gibt es keine) und dann auch DEFAULT_HEADMODEL "" nicht (g_shared.h:109) -> CG_Error
  "DEFAULT_MODELS failed to register" (cgame/cg_main.cpp, ERR_DROP). Im Spiel erscheint die Figur also nie, ihr Massstab ist nicht
  zu sehen - nicht anwendbar. Gegen die Vorschau nicht eigens geprueft (NPC_type random hat dort keinen Eintrag in den NPC-Daten).
- JK2-MP-Bildreihenfolge: JK2MV (GPL-2.0, aus der Raven-JK2-MP-Quelle, `mvdevs/jk2mv` src/renderer/tr_image.cpp R_LoadImage):
  .jpg, .png, .tga nach COM_StripExtension (genannte Endung nie zuerst), dann dieselbe Folge mit JKA-Daten - png und der zweite
  Durchgang sind Zusaetze von JK2MV ("try png first"); die Raven-JK2-SP-Quelle (a09d4827) nimmt .jpg, .tga. Es gibt kein Regelwerk
  "Jedi Outcast MP" - nicht umgesetzt (die Vorschau ist Einzelspieler).

## Punkt 5: ERR_DROP im Spiel belegt (633e7865b)

- `Y:\werkzeuge\r4\p5\spiel_abbruch.py` (aus FIX5 spiel.py: Launcher an der Eltern-PID, vorher keine fremden Spielprozesse): Testkarte
  f5welle mit textures/f5/welle "deformVertexes wave 96 random 0 10 0.1 0.35" in Movie Duels SP. Protokoll: "ERROR: TableForFunc called
  with invalid function '7' in shader 'textures/f5/welle'" - das Spiel bricht ab, die Bilder danach zeigen das Hauptmenue. Spielordner
  vorher = nachher (True), kein eigener Prozess uebrig.
- ctest karte_optimierer: derselbe Shaderrumpf als Fall - die Pruefung meldet spielabbruch.

## Punkt 6: Original-JKA im Handbuch

- Hinweis auf der Seite "Die Karte" (tools/handbuch/inhalt_rest4.py, 6 Sprachen): jasp.exe/jamp.exe suchen immer .jpg, .png, .tga,
  uebergehen die genannte Endung und schneiden am ersten Punkt ab (FIX6 P54, Raven-JKA 14cea156); die Vorschau folgt OpenJK. Dazu
  die Hinweise zu Punkt 2 (vor der Welt, Himmel) und Punkt 3 (Haeute).

## Punkt 7: duel_exegol-Rest - offen

- Nicht bearbeitet (Kontext dieses Auftrags). Stand REST3d: im Nebel ist die Vorschau gleichmaessig G +0,6 / B +1,3 heller (mit
  Gluehen +1,8 / +3,2), Messaufbau `R3D_ECHT` in `Y:\werkzeuge\r3d\spiel.py`.
- Eingegrenzt (r_DynamicGlow 0 bzw. BHED_GV_GLUEHEN=0, Kamera -600 -5952 208 / 10 0, `Y:\werkzeuge\r3d\mittel.py`): mit Nebel Spiel
  G 4,6 / B 8,8, Vorschau 5,2 / 10,0 (`r4exA`/`r4_exA`); ohne Nebel (Spiel `seta r_drawfog 0`, Vorschau `nebel=0`) Spiel 0,5 / 2,2,
  Vorschau 0,9 / 3,5 (`r4exB`/`r4_exB`). Der Unterschied in Blau bleibt ohne Nebel gleich (+1,3) - er kommt nicht aus der Nebelmischung,
  sondern aus den Flaechen selbst. Im aufgehellten Paar ohne Nebel (`Y:\werkzeuge\r4\exB_paar.png`, links Spiel) zeigt die Vorschau
  Dinge, die im Spielbild fehlen: einen dunklen schraegen Gegenstand in der Mitte, oben Mitte eine rot-blaue Figur, und die rechte
  Wand ist heller. Die Entities sind dieselben (390 aus maps/duel_exegol.ent). Naechster Schritt: diese Gegenstaende bestimmen (Figur:
  Spieler/NPC zur Bildzeit, Gegenstand: Modell oder Mover) und danach das Flaechenlicht vergleichen.

## Punkt 8: Endrunde-4-Wettermessung nachgemessen

- Neuer Aufbau (`R3D_ECHT="r_weather 0"`, md_acc_jedi, Kamera 755 286 80, Nick 10, Gier -143): die Wetterdatei laeuft zweimal, pj2.cfg
  erst danach; das Spielbild zeigt weder Schnee noch Nebelschwaden (Mittel 1,9/14,8/22,7 gegen 3,6/19,1/27,8 im alten Aufbau mit
  Schnee; `Y:\werkzeuge\r4\acc_alt_neu.png`, oben alt, unten neu). Vorschau (`r4_acc`): "Weather/md_acc_jedi.cfg, 0 Teilchenwolken" -
  gleich, keine Aenderung noetig. Endrunde 4 stimmt (dort Fensterfotos nach dem Raeumen).

## Punkt 9: AT-ST-Spieler

- Nur misc_atst_use (codeJK2 g_misc.cpp:2625-2652) und der Befehl "drive" (g_cmds.cpp:1103) rufen G_DriveATST. misc_atst_use verlangt
  den Spieler als Ausloeser (activator->s.number 0) und auf dem AT-ST stehend (ps.groundEntityNum). Ein Skript koennte den Spieler per
  use ausloesen, das Stehen auf dem AT-ST ergibt aber erst die Spielerbewegung (Pmove) - die Vorschau steuert und bewegt den Spieler
  nicht. Nicht anwendbar.

## Schlussrunde (`Y:\werkzeuge\r4\schluss.sh`, parallel nach dem sauberen Neubau, Summe `schluss_summe.txt`)

- Sauberer Neubau (`--clean-first`) 0 Warnungen (785 s); ctest 244/244 (-j 12 ohne q3map2_pr316), q3map2_pr316 allein 1/1.
- Selbsttests: Modus 1 2635/0, mission1 98/0, figurlicht 8/0, geschuetze 21/0, laserfalle 9/0, karteneditor rd 38/0 und r3 22/0,
  editorportal 19/0, cjkweit ja 19/0, menueeditor en 2787/0, modellblock md 86/0, pt 41/0, ptfix3 12/0, haut 1/0, funktionen 60/1.
- funktionen "MB1 Pruefen: fehlendes Bild an torso gemeldet" war rot durch Punkt 3: der Hautshader ohne Bild bekam die leere Stufe
  (figurOhneStufe) und zaehlte als Bild. Jetzt traegt die Stufe `Tex::ohneStufe`, die Modellpruefung zaehlt sie als fehlend;
  Neubau 0 Warnungen, funktionen `r4_s_funktionen2` 61/0.
- Lints wie die Basis (portability 99 nach `<cstddef>` in src/vorwelt.cpp, pruefcodes 11, efxflags-/fremdprogramm-Meldung wie vorher,
  link/tidy nur unter Linux, sonst ok); Handbuch `--pruefen` ok. Keine Bildzeit-Messung (Auftrag).

## Offen

- Punkt 7 (duel_exegol): eingegrenzt, nicht behoben - siehe oben.
- Messungen der Schlussrunde vor dem Release: P6/P15 (`ptzeit`), P9, P11, P28, P38, P47; dazu die Ladezeit mit den Menues vor der Welt
  (jede Karte liest ui/menus.txt, ui/ingame.txt und ui/jahud.txt samt Menues) nicht gemessen.

## Changelog

## English

- Before the map the preview loads what a freshly started game loads first - menus, HUD, fonts, the effects of NPC classes and vehicles and the weapons of the characters - so a picture shared with a menu looks like in the game; a sky whose stage picture is missing shows the default box.
- Skins are read word for word like the engine (tag_ lines and lines without a comma shift the following words); Jedi Outcast skins work like jk2sp.exe ("*off" draws nothing, unnamed surfaces show the default box, no three-part skins). A skin shader without picture draws nothing.
- The map check's game-stop warning is confirmed in Movie Duels; the manual explains how the original Jedi Academy looks for picture files.

## Deutsch

- Vor der Karte lädt die Vorschau, was ein frisch gestartetes Spiel zuerst lädt - Menüs, HUD, Schriften, die Effekte der NPC-Klassen und Fahrzeuge und die Waffen der Figuren -, ein mit einem Menü geteiltes Bild sieht darum aus wie im Spiel; ein Himmel ohne Stufenbild zeigt den Standardkasten.
- Häute werden Wort für Wort wie in der Engine gelesen (tag_-Zeilen und Zeilen ohne Komma verschieben die folgenden Wörter); Häute unter Jedi Outcast wirken wie in jk2sp.exe ("*off" zeichnet nichts, ungenannte Flächen zeigen den Standardkasten, keine dreiteiligen Häute). Ein Hautshader ohne Bild zeichnet nichts.
- Die Warnung der Kartenprüfung vor einem Spielabbruch ist in Movie Duels bestätigt; das Handbuch erklärt, wie das Original-Jedi-Academy Bilddateien sucht.

## 中文

- 在地图之前,预览会先加载刚启动的游戏最先加载的内容——菜单、HUD、字体、NPC 类别和载具的特效以及角色的武器——因此与菜单共用的图像看起来与游戏中一样;缺少阶段图像的天空显示默认方框。
- 皮肤与引擎一样逐词读取(tag_ 行和没有逗号的行会使后面的词错位);Jedi Outcast 皮肤与 jk2sp.exe 相同("*off" 不绘制任何内容,未提及的表面显示默认方框,没有三部分皮肤)。没有图像的皮肤着色器不绘制任何内容。
- 地图检查关于游戏中止的警告已在 Movie Duels 中得到证实;手册说明了原版 Jedi Academy 如何查找图像文件。

## 日本語

- マップの前に、プレビューは起動したばかりのゲームが最初に読み込むもの(メニュー、HUD、フォント、NPC クラスと乗り物のエフェクト、キャラクターの武器)を読み込みます。そのためメニューと共有する画像はゲームと同じに見えます。ステージ画像のない空は既定のボックスを表示します。
- スキンはエンジンと同じく 1 語ずつ読み込まれます(tag_ 行やカンマのない行は後続の語をずらします)。Jedi Outcast のスキンは jk2sp.exe と同じです("*off" は何も描かず、指定のないサーフェスは既定のボックスを表示し、3 部構成のスキンはありません)。画像のないスキンシェーダーは何も描きません。
- ゲーム中断に関するマップチェックの警告を Movie Duels で確認しました。マニュアルでオリジナルの Jedi Academy が画像ファイルを探す方法を説明しています。

## Español

- Antes del mapa la vista previa carga lo que un juego recién iniciado carga primero (menús, HUD, fuentes, los efectos de las clases de NPC y vehículos y las armas de los personajes), así que una imagen compartida con un menú se ve como en el juego; un cielo sin imagen de etapa muestra la caja por defecto.
- Las pieles se leen palabra por palabra como el motor (las líneas tag_ y las líneas sin coma desplazan las palabras siguientes); las pieles de Jedi Outcast funcionan como en jk2sp.exe ("*off" no dibuja nada, las superficies no nombradas muestran la caja por defecto, sin pieles de tres partes). Un shader de piel sin imagen no dibuja nada.
- El aviso de la comprobación del mapa sobre la interrupción del juego está confirmado en Movie Duels; el manual explica cómo busca el Jedi Academy original los archivos de imagen.

## Čeština

- Před mapou náhled načte to, co čerstvě spuštěná hra načítá jako první (nabídky, HUD, písma, efekty tříd NPC a vozidel a zbraně postav), takže obrázek sdílený s nabídkou vypadá jako ve hře; obloha bez obrázku fáze ukazuje výchozí rámeček.
- Skiny se čtou slovo po slově jako v enginu (řádky tag_ a řádky bez čárky posouvají následující slova); skiny Jedi Outcast fungují jako v jk2sp.exe ("*off" nic nekreslí, neuvedené plochy ukazují výchozí rámeček, žádné trojdílné skiny). Shader skinu bez obrázku nic nekreslí.
- Varování kontroly mapy před přerušením hry je potvrzeno v Movie Duels; příručka vysvětluje, jak původní Jedi Academy hledá soubory obrázků.
