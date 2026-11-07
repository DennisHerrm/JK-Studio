# Notizen p_render: offene Punkte Vorschau/Renderer (paralleles Fenster 07.11.)

Auftrag `auftrag_p_render.md`, Worktree `Y:\render`, Zweig `render1` ab e6daa5ca8 (Modell-Block 1d), Build `Y:/render/build-render`
(Konfiguration wie `Y:/karte/build-test`). Laeufe `laeufe/rot_*` (rote exe `laeufe/rd_exe_rot` = e6daa5ca8 plus Selbsttest rd, sonst
unveraendert) und `laeufe/gruen_*`. kFassung bleibt rc610. Movie Duels per Popen mit eigenem fs_homepath (Skript `rd_md.py` im
Scratchpad nach `skripte/g7/md7.py`), Spielordner vorher/nachher gleich. Original-JKA nicht gestartet, radiant.exe nicht gestartet.

Selbsttest: `BHED_EDITORTEST=karteneditor` mit `BHED_KARTE_NUR=rd` (Stufen, Spiegel, model2, Effekt; auch im vollen
karteneditor-Lauf), `rdcjk` (Lauf mit `BHED_SPRACHE=ja` bzw. `zh-Hans`), `rdnpc` (Movie-Duels-Figuren). ctest `umbruch`.
`BHED_RD_VORHER=1`: Gluehen nur im Hauptbild und ohne Effekte wie vor p_render (Vergleich im selben Lauf).

## 1. Glow in der Spiegel-/Portalsicht und an fx_runner

- Vorbild: RB_DrawSurfs (OpenJK `code/rd-vanilla/tr_backend.cpp:1306-1389`, MD9 gleich) laeuft je drawSurfs-Kommando. Eine
  Spiegel- oder Portalsicht ist ein eigenes R_RenderView (R_MirrorViewBySurface) mit eigenem Kommando, die Szene des Himmelsportals
  ein eigenes RE_RenderScene (CG_DrawSkyBoxPortal) - jede bekommt ihr Gluehen, bevor die Hauptszene darueber kommt. Effektteilchen sind
  Flaechen wie alle anderen: im Gluehdurchgang zeichnet RB_IterateStagesGeneric ihre Stufen mit glow.
- Echtzeit-Licht (`gui/app_karte_cam.cpp`): das Gluehen ist ein Lambda (`gluehenZeichnen`), das in jedem Durchgang in das gerade
  aktive Ziel zeichnet - Spiegel-/Portalsicht (Ziel 202), Himmelsportal (201), Hauptbild. Die Teilchen der fx_runner, deren Shader
  in der ersten Stufe glow hat, sammelt `echtEffekteZeichnen` (`app_karte_echtfx.inc`) mit Ecken und Zustand fuer das Gluehen.
- "Gebaute Karte wie im Spiel" (`gui/app_karte_spielbild.cpp`): das Gluehen auch in der Spiegelsicht und der Szene des Himmelsportals
  (vorher nur im Hauptbild).
- Mission-Tab (Vorschau, `app_view3d_bedienung.inc`): ebenso in der Spiegel-/Portalsicht und der Szene des Himmelsportals
  (Welt, Mover, Kartenmodelle, Effekte, Klingen); die Leuchtstufen der Figuren dort weiter nicht (offen). Der Selbsttest editorportal
  vergleicht Spielbild und Vorschau im Spiegel (duel_executor) - beide gluehen jetzt gleich.

## 2. Echtzeit-Licht: Stufen mit tcMod, tcGen, Wellen, animMap

- Vorbild: ComputeTexCoords/ComputeColors (`code/rd-vanilla/tr_shade.cpp`), RB_CalcScrollTexCoords usw. (`tr_shade_calc.cpp`),
  R_BindAnimatedImage. Die tcMod-Kette wird wie in der Vorschau zu einer affinen Abbildung zusammengerechnet
  (`gpu::texModAbbildung`, aus `packeKonstanten` herausgezogen, gleiche Rechnung); turb nach dem Ort im Shader wie die Vorschau;
  tcGen environment wie RB_CalcEnvironmentTexCoords (Q_rsqrt), tcGen vector; rgbGen/alphaGen wave geklemmt wie `mapview.cpp`
  (batchStateAus); animMap ueber `animBildIndex`. Zeit: die Shaderzeit von "Gebaute Karte wie im Spiel" (`spielbildZeit`).
- `stufeDarstellbar` laesst jetzt auch diese Stufen zu (weiter nicht: Video, rgbGen vertex/lightingDiffuse/entity, alphaGen
  entity/vertex/specular). Neu auch die Bildstufe der Grundstufe (ohne Lightmap die erste, sonst die des Paars mit der Lightmap): ihre
  Koordinaten, ihre Farbwelle und das Bild der Stufe (nicht qer_editorimage - das Spiel zeichnet die Stufe); das Licht je Bildpunkt
  steht weiter fuer die Lightmap.
- Konstanten des Kamera-Shaders (`kShader3`, `kShader4`): `tcMat`, `tcOff`, `tcGenS`, `tcGenT` hinten angehaengt; ohne Bewegung
  die Einheitsabbildung (gleiches Bild wie vorher).
- Editor-Licht (ohne Echtzeit): wie NetRadiant-Custom nur `qer_editorimage` - unveraendert.
- Tooltip `KeLichtGrenzenTip4` (statt Tip3), Handbuch karte_tempo (Absatz "Weitere Stufen und Glow", 6 Sprachen).

## 3. Movie-Duels-Modelle

- **`*off`-Flaechen:** Movie Duels imperial (models/players/imperial, MD_Models_OT-ST.pk3) belegt in model_default.skin `torso,*off`
  und `hips,*off` und zeigt statt dessen `off_torso`/`off_hips`. Die Hierarchie hat `torso` und `hips` zweimal (die erste als
  `..._off` mit Flagge OFF). G2API_SetSurfaceOnOffFromSkin findet ueber G2_IsSurfaceLegal nur die erste - die zweite bleibt an.
  RenderSurfaces (`tr_ghoul2.cpp:2276-2288`) nimmt fuer sie den Shader der Haut: "*off" - R_FindShader findet kein Bild und gibt
  einen Shader ohne Stufe zurueck (`tr_shader.cpp:3502-3509`, MD9 `:3457-3466`), RB_StageIteratorGeneric zeichnet nichts. Das Spiel
  zeigt sie nicht; der Model-Tab zeichnete beide in der Ersatzfarbe. Jetzt: eine Flaeche, die die Haut mit "*off" belegt, zeichnet
  die Ansicht nicht (`app_modelltab.cpp`), die Regel (Liste "gezeichnet") bleibt die von Ghoul2. Der Mission-Tab hielt sie schon aus.
  Spielfoto Movie Duels (playermodel imperial, `fotos/rd_imp2_1.png`): Huefte und Rumpf ohne Ersatzfarbe und ohne Loch.
- **StOfficerAlt doppelt:** Movie Duels hat den Namen dreimal: MD_Assets.pk3 `ext_data/npcs/z_JO_NPCs.npc` (customSkin officer,
  surfOn/surfOff auskommentiert), base/assets1.pk3 `stofficer.npc` (ebenso) und `stofficeralt.npc` (surfOn torso_pauldron, surfOff
  ...). NPC_LoadParms haengt die .npc-Dateien in der Reihenfolge von FS_GetFileList hintereinander (MD9 `code/game/NPC_stats.cpp:
  4740-4800`; FS_ListFilteredFiles ohne Sortierung: Suchpfade, im Archiv die Reihenfolge der Eintraege), NPC_ParseParms sucht den
  Namen von vorn (`:2407-2425`) - der erste Block gilt. JK Studio las die Dateien schon in dieser Reihenfolge, liess aber in Jedi
  Academy/Movie Duels den letzten Block gewinnen (`parseNpcFile`, nur Jedi Outcast den ersten). Jetzt gilt ueberall der erste.
  In Jedi Academy (ohne Movie Duels) liegt `stofficer.npc` in assets1.pk3 vor `stofficeralt.npc` (Eintrag 97 vor 98) - auch dort
  der Block ohne surfOn/surfOff mit customSkin officer.
  Spielfoto Movie Duels (`npc spawn stofficeralt`, `fotos/rd_sto2_1.png`): Sturmtruppler mit orangefarbenem Schulterstueck - die
  Haut officer (`torso_pauldron,.../shoulder-orange.tga`); der Block aus stofficeralt.npc haette die Haut default, in der
  torso_pauldron "*off" ist (surfOn schaltet sie an, gezeichnet wuerde sie nicht - siehe oben).

## 4. CJK-Umbruch

- ImGui bricht nur an Leerzeichen und nach `.,;!?"` sowie `、。` um (ImFontCalcWordWrapPositionEx, Zeichenklassen fest in
  imgui_draw.cpp); ein japanischer oder chinesischer Satz hat keine Leerzeichen - die erste Zeile endete am ersten Komma.
- Neu `bhed/umbruch.h` (`umbruchZeilen`): Umbruch zwischen zwei Zeichen, wenn eines ein CJK-Zeichen ist, mit Zeilenanfang- und
  -endsperren (JIS X 4051 / GB/T 15834: keine schliessenden Klammern, Satzzeichen, kleinen Kana, Laengezeichen am Zeilenanfang,
  keine oeffnenden Klammern am Ende); Leerzeichen und lateinische Woerter wie ImGui. `gui/textumbruch.h`: `cjkText`,
  `cjkTextDisabled`, `cjkTextColored`, `cjkTextWrapped`, `cjkTextUnformatted` - ohne CJK-Zeichen oder ohne Umbruch genau die
  ImGui-Funktion, sonst die fertigen Zeilen als ein Text. Im ganzen Model-Tab (alle Rollouts, 109 Aufrufe in 9 Dateien) eingesetzt.
- Uebrige Bereiche (Mission, Karten-Editor, Menue-Editor, Helfer) brechen weiter nach der ImGui-Regel um (offen).

## 5. model2 an Brush-Entities

- Vorbild: InitMover/InitBBrush setzen `s.modelindex2` aus `model2` (MD9 `code/game/g_mover.cpp:1104-1124`,
  `g_breakable.cpp:302-307`, OpenJK gleich), CG_Mover zeichnet das Modell an lerpOrigin/lerpAngles, dazu modelAngles
  (`cgame/cg_ents.cpp:1702-1718`). "Gebaute Karte wie im Spiel" und der Mission-Tab zeichneten es schon (rc592/rc608 13b); die
  Kamera des Editors nicht (wie NetRadiant-Custom).
- Jetzt (`src/karte/kameraszene.cpp`): func_bobbing, func_breakable, func_button, func_door, func_pendulum, func_plat,
  func_rotating, func_static, func_train, func_usable, func_wall mit `model2` *.md3: das Modell mit den Shadern der Datei am Ursprung
  wie q3map2 ihn setzt (Mitte des Brushes mit surfaceparm origin, sonst Schluessel origin, sonst 0 0 0), Winkel im Ruhestand nur bei
  func_static/func_wall (G_SetAngles) und func_pendulum (apos.trBase), dazu modelAngles. Gilt fuer Editor-Licht und Echtzeit-Licht.
  Die Flaechen eines Modells zeichnet dieselbe Funktion wie bei misc_model (`modellFlaechen`).
- Dazu der Ursprungs-Brush wie q3map2: Jedi Academy `textures/system/origin` (assets1.pk3 shaders/system.shader) hat kein
  `surfaceparm origin`, sondern `q3map_origin` - q3map2 wendet ein sonst unbekanntes `q3map_<name>` wie `surfaceparm <name>` an
  (ParseShaderFile, `shaders.cpp:1706-1711`). Das Shaderverzeichnis des Editors (`src/karte/texturen.cpp`) las nur `surfaceparm`;
  jetzt kommt `q3map_<name>` in die surfaceparms, wenn der Name in der Tafel von game_ja steht (`istSurfaceparm`). Die Editor-Flaggen
  (QER_*) bleiben wie NetRadiant-Custom (nur surfaceparm). Wirkt auf alles, was die Inhalte wie q3map2 rechnet (Inhaltsansichten,
  Kollision "Im Editor spielen", Kartenpruefung).
- Nicht: `.glm` als model2 (Ghoul2 am Mover), `model2scale` (nur MD9-Code), Bewegung (Ruhestand).

## Rot und gruen

Rot mit der exe von e6daa5ca8 plus Selbsttest (`laeufe/rd_exe_rot`, Laeufe `rot_*`), gruen `g2_*`/`g3_*`/`g4_*`. Fuer Spiegel und Effekt
zusaetzlich im selben Lauf mit `BHED_RD_VORHER=1` (`g2_vorher`), weil die Messung dort nach dem roten Lauf berichtigt wurde (der
Spiegel mit alphaGen portal war fast schwarz, der Ring um das Teilchen lag im Teilchen).

| Punkt | Pruefung | rot | gruen |
|---|---|---|---|
| 2 Stufen | Abweichung Echtzeit gegen Spielbild je Spalte (0..1), Zeit 1,75 s | `rot_rd`: scroll 0,323, welle 0,090, anim 0,350, umwelt 0,646, grund 0,499; weitere Stufen 0 | `g2_rd`: scroll 0,003, welle 0,006, anim 0,008, umwelt 0,004; weitere Stufen 4; grund siehe unten |
| 1 Spiegel | (Schein / Gelb) im Spiegel durch direkt | `g2_vorher`: 0,148 (0,079 / 0,531) | `g2_rd`: 0,816 (0,433 / 0,531) |
| 1 Effekt | Schein 6-40 Bildpunkte neben dem Teilchen, mit / ohne Effekte | `g2_vorher`: 2,0 / 2,0 | `g2_rd`: 64,1 / 2,0 |
| 3 NPC | StOfficerAlt aus 184 .npc-Dateien | `rot_npc`: customSkin default, surfOn torso_pauldron, surfOff ... | `gruen_npc`: customSkin officer, ohne surfOn/surfOff |
| 3 *off | imperial: Flaechen mit "*off" in der Haut, die gezeichnet werden | `rot_npc`: 2 (hips, torso) | `gruen_npc`: 0; Gegenprobe jedi_zm 0 |
| 4 CJK | Fuellung der Zeilen ausser der letzten (Befehlsfeld 362 Bildpunkte) | ImGui-Regel ja: 3 Zeilen, leerste 0,10 (erste Zeile "手、"); zh 0,81 | `g3_cjkja`: 2 Zeilen, 0,96; `g3_cjkzh`: 2 Zeilen, 0,93 |
| 5 model2 | Anteil Rot der Wuerfel-Vorderseite, Editor-Licht / Spielbild | `rot_rd`: 0,056 / 1,000 | siehe unten |

- Die CJK-Pruefung lief im roten Lauf ohne Modell (das Befehlsfeld gibt es erst mit Modell) und in `gruen_cjk*` in Englisch (`BHED_SPRACHE`
  gilt fuer den Selbsttest nicht, `SPRACHE=` des Laufskripts schon); die Rot-Zahl ist die ImGui-Regel (ImFont::CalcWordWrapPosition) an
  derselben Breite - so hat das Programm vorher gezeichnet.
- Spielfotos Movie Duels (1920 x 1440, angesehen): `rd_imp2_1.png` (playermodel imperial von vorn: Huefte und Rumpf ohne Ersatzfarbe),
  `rd_sto2_1.png` (npc spawn stofficeralt: orangefarbenes Schulterstueck = Haut officer). Spielordner vorher/nachher gleich.
- ctest 232/232 (`g2_ctest.txt`, -j 8), darunter neu `umbruch`.

## Offen

- Mission-Tab (Vorschau): die Leuchtstufen der Figuren gluehen in Spiegel- und Portalsicht nicht (nur im Hauptbild).
- Echtzeit-Licht: Spiegel und Himmelsportal zeichnen keine Effekte (fx_runner) - daher auch kein Gluehen von Effekten dort.
- CJK-Umbruch ausserhalb des Model-Tabs.
- model2 als .glm und model2scale in der Kamera des Editors.
- Bildzeit (Messblock `BHED_R3_ZEIT=1`, Glow-Platte im Echtzeit-Licht) nach dem Fenster allein messen - im Fenster nicht gemessen.

## Changelog

### English
- Map editor, real-time light: shader stages with tcMod, tcGen environment/vector, rgbGen/alphaGen wave and animMap move like in the game (at the shader time of "Built map as in the game"), also the first stage of a shader without a lightmap.
- Map editor, real-time light and "Built map as in the game": glowing stages also glow in mirrors, portals and the sky portal, and particles of effects (fx_runner) with a glow stage glow like in the game.
- Map editor: a "model2" on a brush entity (func_static, func_door and the other movers) is drawn in the camera at the entity's origin, like the game draws it.
- Model tab: Japanese and Chinese hints wrap between the characters instead of after the first comma.
- Model tab: surfaces that the skin sets to "*off" stay invisible even when the model switches them on (Movie Duels imperial: no stray hips and torso).
- NPCs: when a name is defined in several .npc files, the first one counts like in the game (Movie Duels StOfficerAlt with the officer skin).

### Deutsch
- Karten-Editor, Echtzeit-Licht: Shaderstufen mit tcMod, tcGen environment/vector, rgbGen/alphaGen wave und animMap bewegen sich wie im Spiel (zur Shaderzeit von „Gebaute Karte wie im Spiel“), auch die erste Stufe eines Shaders ohne Lightmap.
- Karten-Editor, Echtzeit-Licht und „Gebaute Karte wie im Spiel“: leuchtende Stufen leuchten auch im Spiegel, im Portal und im Himmelsportal, und Teilchen von Effekten (fx_runner) mit einer glow-Stufe leuchten wie im Spiel.
- Karten-Editor: ein „model2“ an einer Brush-Entity (func_static, func_door und die anderen Mover) erscheint in der Kamera am Ursprung der Entity, wie das Spiel es zeichnet.
- Model-Tab: japanische und chinesische Hinweise brechen zwischen den Zeichen um statt nach dem ersten Komma.
- Model-Tab: Flächen, die die Haut auf „*off“ setzt, bleiben unsichtbar, auch wenn das Modell sie einschaltet (Movie Duels imperial: keine überzählige Hüfte und kein Rumpf).
- NPCs: steht ein Name in mehreren .npc-Dateien, gilt wie im Spiel der erste (Movie Duels StOfficerAlt mit der Haut officer).

### 中文
- 地图编辑器,实时光照:带 tcMod、tcGen environment/vector、rgbGen/alphaGen wave 和 animMap 的着色器阶段会像游戏中一样运动(使用“已构建的地图如游戏中显示”的着色器时间),没有光照贴图的着色器的第一阶段也是如此。
- 地图编辑器,实时光照和“已构建的地图如游戏中显示”:发光阶段在镜子、传送门和天空传送门中也会发光,带 glow 阶段的特效(fx_runner)粒子也像游戏中一样发光。
- 地图编辑器:画刷实体(func_static、func_door 及其他移动体)上的“model2”会像游戏中一样显示在摄像机中实体的原点处。
- 模型标签页:日文和中文提示在字符之间换行,而不是在第一个逗号后换行。
- 模型标签页:皮肤设为“*off”的面即使被模型打开也保持不可见(Movie Duels imperial:不再多出臀部和躯干)。
- NPC:同一名称在多个 .npc 文件中定义时,与游戏中一样以第一个为准(Movie Duels StOfficerAlt 使用 officer 皮肤)。

### 日本語
- マップエディター、リアルタイムライト: tcMod、tcGen environment/vector、rgbGen/alphaGen wave、animMap のシェーダーステージがゲームと同じように動きます (「ビルドしたマップをゲームと同じ表示に」のシェーダー時間)。ライトマップのないシェーダーの最初のステージも同様です。
- マップエディター、リアルタイムライトと「ビルドしたマップをゲームと同じ表示に」: 光るステージは鏡、ポータル、スカイポータルの中でも光り、glow ステージを持つエフェクト (fx_runner) のパーティクルもゲームと同じように光ります。
- マップエディター: ブラシエンティティ (func_static、func_door などのムーバー) の "model2" が、ゲームと同じようにカメラでエンティティの原点に表示されます。
- モデルタブ: 日本語と中国語のヒントが最初の読点の後ではなく文字の間で折り返されます。
- モデルタブ: スキンで "*off" にした面は、モデルがオンにしても表示されません (Movie Duels imperial: 余分な腰と胴がなくなります)。
- NPC: 同じ名前が複数の .npc ファイルにある場合、ゲームと同じく最初のものが使われます (Movie Duels StOfficerAlt は officer スキン)。

### Español
- Editor de mapas, luz en tiempo real: las etapas de shader con tcMod, tcGen environment/vector, rgbGen/alphaGen wave y animMap se mueven como en el juego (con el tiempo de shader de «Mapa compilado como en el juego»), también la primera etapa de un shader sin lightmap.
- Editor de mapas, luz en tiempo real y «Mapa compilado como en el juego»: las etapas que brillan también brillan en espejos, portales y el portal del cielo, y las partículas de efectos (fx_runner) con una etapa glow brillan como en el juego.
- Editor de mapas: un «model2» en una entidad de brush (func_static, func_door y los demás movers) aparece en la cámara en el origen de la entidad, como lo dibuja el juego.
- Pestaña de modelo: las indicaciones en japonés y chino se cortan entre los caracteres en lugar de tras la primera coma.
- Pestaña de modelo: las superficies que la skin pone en «*off» siguen invisibles aunque el modelo las active (Movie Duels imperial: sin cadera ni torso de más).
- NPC: si un nombre está en varios archivos .npc, cuenta el primero como en el juego (Movie Duels StOfficerAlt con la skin officer).

### Čeština
- Editor map, světlo v reálném čase: fáze shaderu s tcMod, tcGen environment/vector, rgbGen/alphaGen wave a animMap se pohybují jako ve hře (v čase shaderu z „Sestavená mapa jako ve hře“), i první fáze shaderu bez lightmapy.
- Editor map, světlo v reálném čase a „Sestavená mapa jako ve hře“: zářící fáze září i v zrcadle, v portálu a v nebeském portálu a částice efektů (fx_runner) s fází glow září jako ve hře.
- Editor map: „model2“ na brush entitě (func_static, func_door a další movery) se v kameře zobrazí v počátku entity, jak ho kreslí hra.
- Karta modelu: japonské a čínské nápovědy se zalamují mezi znaky, ne až po první čárce.
- Karta modelu: plochy, které skin nastaví na „*off“, zůstanou neviditelné, i když je model zapne (Movie Duels imperial: žádné přebytečné boky a trup).
- NPC: je-li jméno ve více souborech .npc, platí jako ve hře první (Movie Duels StOfficerAlt se skinem officer).
