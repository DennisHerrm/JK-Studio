# Notizen Modell-Block Stufe 7: Figuren im Mission-Tab animieren, fuer das Spiel bauen, mehrere Figuren

Stand 07.11.2026, Zweig `karte1`, auf 1bde2dc14 (Stufe 4). Zwei Agenten nacheinander: der erste baute den Hauptteil (drei WIP-Commits
bis 19449b0c6), der zweite klaerte vier Funde gegen das Spiel und machte die Schlussrunde.

## Quellen

- OpenJK (`code/game`, `code/rd-vanilla`) und Movie Duels Update 9 (Jacesolaris/MovieDuels-SJE-SP-MP-master, `code/game`, `code/cgame`),
  Stellen je Funktion im Kommentar.
- Stufe 0: `plaene/modellblock_stufe0/stufenplan.md` (Stufe 7, Rot ohne SET_ORIGIN), `nachweise_im_spiel.md`.
- `docs/intern/NOTIZEN_mb4.md`, Abschnitt "Vorschlag Stufe 7".

## 1. Gebaut (erster Agent)

- Backend `include/bhed/modell/figuranim.h`, `src/modell/figuranim.cpp`: Skriptzeilen `//(BehavEd) figur`/`figur+` je Figur (Grundanimation,
  Bilder, Start, Platz BOTH_CIN_n, Folgeanimation, waitsignal/signal, SET_ORIGIN, Kinomodus), FK-Schluessel, IK-Spuren, Raeume, Wurzelweg;
  ICARUS-Block mit `SET_BEHAVIOR_STATE BS_CINEMATIC`, `SET_ANIM_BOTH`, `SET_ANIM_HOLDTIME_BOTH`, `SET_ORIGIN` + Folgeanimation; Plaetze
  (JKA 50, MD 125), Backen in die Karten-GLA, fremde Raeume (Hand einer anderen Figur), Zusammenfuehren vorhandener Karten-GLAs.
- Fenster "Figur animieren" (`gui/app_figuranim.cpp`, `gui/app_figuranim_ui.inc`): Griffe in der Karte (W/E bzw. G/R), Rollouts Animation,
  Posieren, IK/FK, Pfade, Dopesheet mit IK-Zeilen, Fuer das Spiel bauen (Ziel-GLA erst beim Speichern: Karten-GLA, `_humanoid.gla` im
  Mod-Ordner, eigene GLA). Erreichbar ueber Inspektor, Rechtsklick der Entity, Menue, Aktionssuche (X), Rechtsklick am Griff, Tooltips mit
  Taste. Die Vorschau spielt die gebackene Karten-GLA (`kinoSkeletteAbgleichen`, `figurenKinoFuer`).
- Model-Tab: IK-Schluessel als eigene Zeile im Dopesheet, Verschieben mit den FK-Schluesseln der Kette.
- Tests: ctest `modell_figuranim`, Selbsttest-Gruppen `mb7` (Bedienung, 24/0), `mb7spiel`, `mb7zeit`, Handbuchseite `mission-animieren`.

## 2. Funde gegen das Spiel (zweiter Agent), je rot -> gruen

Spiel: Movie Duels SP (`MovieDuels-SP.x86_64.exe`, fs_game MD, eigener fs_homepath im Laufordner) und Original-JKA (Steam, `jasp.exe`;
steam.exe lief, ActiveUser != 0 - vor jedem Start geprueft; GameData-Kopie und Aufraeumen nach auftrag_gemeinsam.md, GameData und
"GameData original" danach je gleich). Szene `mb7szene` auf der Testkarte `mb7raum`, Skriptkamera senkrecht von oben (ZOOM 110), Fenster
1920 x 1440, Fotos je 0,25 s (Laufskripte `mb7/spiel.py`, `mb7/orig_spiel.py`, Auswertung `mb7/vergleich2.py`).

1. **Blockierter NPC-Spawn.** NPC_Begin prueft ohne SFB_NOTSOLID `SpotWouldTelefrag( ent, TEAM_FREE )` (NPC_spawn.cpp:1727-1752 MD /
   999-1020 OpenJK, g_client.cpp:284-316 MD) und versucht es `ent->wait` spaeter wieder (Spawner-wait, Vorgabe 500 ms, NPC_spawn.cpp:
   3062-3069 MD); wait < 0 benutzt target3 und gibt frei. Die Vorschau liess die Figur sofort erscheinen.
   - Nachbau (`src/ablauf.cpp`): `npcBegin` mit Wiederholung als think der Figur (`Geplant::Art::Beginn`, in Entity-Reihenfolge),
     `spawnBeimLadenBlockiert` fuer die Spawner vom Laden (NPC_Begin im ersten Spielbild, Wiederholungen auf der Zeitachse des Skripts),
     Skriptbeginn im Spiel aus dem Kartenstart-Nachbau (`AblaufWelt::startImSpielMs`, `startImSpiel` in `gui/app_view3d.cpp`; das offene
     Skript selbst, weil eine neue Mission in keinem Archiv liegt). Ein `affect` auf die noch fehlende Figur entfaellt wie im Spiel.
   - Rot: ctest `ablauf` (spawnBlockiert) 4 FEHLER, Selbsttest `mb7b_sp1` ("eng": c sofort da). Gruen: ctest `ablauf` alle,
     `mb7b_sp3` 4/0 (c erscheint bei 6500 ms).
   - Spiel (MD, "eng", c 28 Einheiten neben b): c erscheint zwischen 6,41 und 6,59 s Skriptzeit (Fotos 16,08 / 16,26 s, Versatz an der
     ersten Bewegung von a), Vorschau 6,50 s; Lage von c danach 0,2 px, Deckung der Silhouette 0,96.
2. **Karten ohne Texturen.** Fehlt Shaderskript und Bild, wird die Flaeche tr.defaultShader (R_FindShader -> defaultShader,
   tr_shader.cpp:3500-3509; ShaderForShaderNum; CreateInternalShaders 3836-3848 OpenJK): eine Stufe mit `*default` (16 x 16, Grau 32,
   weisser Rand, R_CreateDefaultImage tr_image.cpp:1281-1310), rgbGen CGEN_BAD -> identityLighting. Die Vorschau liess solche Flaechen weg
   (`gui/app_ablage.inc`, `standardShaderTextur`; gilt in jeder Ansicht, die die Texturen der Karte nimmt). Rot `mb7b_tex1` (Karte
   `raum_licht.bsp` schwarz), gruen `mb7b_tex2`/`mb7b_tex4` (219010 von 310002 Punkten grau, 82889 hell).
3. **Weg im Spiel 3-5 % kuerzer.** Die Messung: `auswertung.py` rechnete Bildpunkte mit der Tiefe der Knochenmitte in Einheiten um; der
   dunkle Fleck ist aber Kopf/Schultern plus Schatten am Boden. Neu `vergleich2.py`: Vorschaufotos durch dieselbe Skriptkamera im
   Spielfenster 4:3 (Rahmen aus dem Foto bei 0 ms, danach liegen die Kinobalken darueber), Flecken in Bildpunkten gegen das Spielfoto,
   Deckung der Silhouetten.
   - Movie Duels (mit SET_ORIGIN): Endlage a 0,8 px, b 0,1 px, c 0,9 px, Deckung 0,97 / 0,99 / 0,97; Weg von a in beiden 386 px.
     Waehrend der Bewegung bis 37 px - die Vorschau ist je 500 ms fotografiert, das Spiel je ~0,17-0,5 s, der Versatz ist auf ein
     Spielbild genau (bei rund 40 Einheiten/s).
   - Ohne SET_ORIGIN: a springt in beiden bei 4,5 s an den Start zurueck (Vorschau 1210/823, Spiel 1210/824, 384 px).
   - Original-JKA (mit): Endlage a 0,6 px, b 0,7 px, Deckung 0,96; ohne: Sprung wie in MD (1210/823 gegen 1210/824).
   - Dabei gefunden: **Schatten der Figur** waehrend einer Animation der Karten-GLA. CG_PlayerShadow (cg_players.cpp:4041-4055 MD) holt
     den Ursprung mit `G2API_GetBoltMatrix( ..., ent->rootBone, ...)`; rootBone ist die Knochennummer von model_root (0), gelesen als
     Bolzennummer: Bolzen 0 = `*head_eyes` (g_client.cpp:1681 MD). Der Schatten geht also mit der Figur; die Vorschau liess ihn am
     Spawner stehen (Foto `mb7_vorschau_mit_4000` vor der Behebung). `schattenUrsprung` in `gui/app_view3d.cpp`; rot `mb7b_sp1`
     (model_root bleibt am Spawner), gruen `mb7b_sp2`/`mb7b_sp3` (Schatten 4,9 Einheiten neben cervical, 92 Einheiten vom Spawner).
4. **Original-JKA**: steam.exe lief mit ActiveUser != 0, Vergleich mit und ohne SET_ORIGIN gemacht (Zahlen oben). Das Original liest keine
   `maps/<karte>.ent`: fuer den Lauf kamen die Entities der Mission in den Entity-Lump einer Kopie der .bsp (nur im fs_basepath).

Paarbilder Spiel/Vorschau (oben Spiel, unten Vorschau, je 1920 x 1440): `mb7/paare/paar_<mit|ohne|eng>_<ms>.png`,
`mb7/paare_orig/...` im Scratchpad, angesehen.

## 3. Bildzeit (allein, BHED_D3D_DEBUG=0)

- `mb7e_zeit` (Gruppe `mb7zeit`): drei Figuren ohne Figurenanimation, Abspielen: Bild-CPU Mittel 0,49 / p99 1,39 ms; drei Figuren
  animiert: 0,54 / 1,63 ms; Posieren mit Griffen (20), Wurzelweg, Pfaden (300 Punkte), Dopesheet und Kurven (420 Punkte): 1,11 / 1,52 ms,
  Ueberlagerung 0,63 ms. Bildabstand 4,2 ms (240 Hz) in allen drei.
- `leistung` alt (`laeufe/mb6e_mission1/jkstudio.exe`, Stufe 4) gegen neu, CPU-Mittel je Bild: Mission Szene spielt 3,55 -> 3,31 ms,
  angehalten 0,97 -> 0,92, duel_mustafar 1,13 -> 1,04, mp/duel1 0,30 -> 0,30, md_toaj_jedi ruhend 0,50 -> 0,62 (p99 1,02 -> 1,75; einmal
  gemessen, die uebrigen 15 Stellen gleich oder schneller), Map Editor 1,49-2,51 (vorher 1,55-2,74), Model 0,43-0,50 (0,49-0,54),
  Menu Editor gleich. GPU gleich (+-0,05) ausser md_toaj_jedi Flug 1,85 -> 1,35.

## 4. Schluss

Einmal, nach sauberem Neubau (`--clean-first`, /W4 /WX, 0 Warnungen; danach nur kleine Folgebauten, je 0 Warnungen):

- ctest 227 von 227 (113,9 s), darunter `ablauf` (neu: blockierter Spawn, Zwischensequenz) und `modell_figuranim`.
- modellblock (Laeufe `mb7e_*`, alle zugleich, Summe `laeufe/mb7e_summe.txt`): funktionen 60/0, tasten 43/0, mb2 37/0, blender 44/0,
  mb3 21/0, rig 26/0, mb5 159/0, mb6 37/0, mb7 24/0, mb7spiel 4/0, mb7textur 1/0; rundlauf jka/jk2/md je 2/0, rigrundlauf je 4/0,
  bild je 1/0, kurvenrundlauf je 2/0, ikrundlauf jka 1/0, jk2 1/0, md 2/0.
- modell 14/0, kuerzel 202/0.
- Modus 1: im ersten Lauf 2634/1 - "Schwerkraft: die per use gespawnte Figur faellt vom Spawner und landet": der neue Spawn-Check sah
  die beim Laden gespawnte Figur noch am Spawner 120 ueber dem Boden (der Nachbau kennt ihren Fall nicht) und hielt die zweite fest.
  Behoben (Koerper ohne Plan stehen fuer die Pruefung auf dem Boden unter ihrem Ort), `mb7e_modus1b` 2635/0.
- mission1: im ersten Lauf Absturz (C++-Ausnahme 0xE06D7363, zweimal gleich) nach "Mini: m1_held spielt BOTH_CIN_1 ..." und
  "Zwischenpunkt: eine laufende Figur ..." FEHLER. Ursache: der Selbsttest setzt den Spawner von m1_held genau auf den Spawner einer
  Figur der Grundkarte - im Spiel spawnte m1_held dort erst, wenn diese weggeht, und sein affect entfiele; die Vorschau macht das jetzt
  auch. Der Test setzt m1_held 40 Einheiten auf der Strecke weiter: `mb7e_mission1c` 98/0.
- Handbuch: Bilder der Seite mission-animieren in 6 Sprachen neu (`mb7b_hb_*`, je 1/0, 2538 x 1472, angesehen), Absatz zum blockierten
  Spawn neu, `gen_handbuch` alle Seiten x 6 Sprachen, `--pruefen` ok, `gen_hilfe_orte` 206 Orte.
- Lints wie in der Basis (portability 99, pruefcodes 11, efxflags/fremdprogramm/link/tidy unveraendert), alle anderen ok, auch
  lint_sources, lint_i18n, lint_i18n_erzeugt, lint_fassung (rc610).
- Spielordner: Movie Duels je Lauf vorher/nachher gleich; Original-JKA "GameData" und "GameData original" je gleich.
- Private Adressen in `git log -p 1bde2dc14..HEAD`: 0.

## 5. Abweichungen und Offen

- Grundanimation nur aus der animation.cfg des Skeletts; SET_ORIGIN behaelt die Starthoehe (Q3_SetOrigin +1, dann Pmove); Kurven im
  Fenster "Figur animieren" vereinfacht (voller Kurveneditor im Model-Tab); Jedi Outcast ohne Karten-GLA (erster Agent).
- Spawn-Pruefung: Koerper ohne Plan gelten als auf dem Boden unter ihrem Ort - in der ersten Sekunde nach ihrem Spawn (waehrend sie
  noch fallen) ist das zu frueh. Die Wiederholungen vor dem Skriptanfang nehmen die Lage beim Laden (vor dem Skript ruht alles).
  DROP_TO_FLOOR-Spawner verschieben den Ursprung beim Spawnen, die Lade-Pruefung nimmt den Spawnerort.
- Der Weg im Spiel war nicht kuerzer: die alte Auswertung rechnete Bildpunkte mit der falschen Tiefe um. Waehrend der Bewegung liegen
  Vorschau und Spiel bis 37 px auseinander, weil die Vorschau nur je 500 ms fotografiert ist (Zeitversatz bis 0,25 s).
- mission1 stuerzt ab, wenn "Zwischenpunkt" keine laufende Figur findet (C++-Ausnahme im Selbsttest-Pfad nach einem FEHLER) - nicht
  nachgegangen, im gruenen Lauf kommt der Pfad nicht vor.
- Handbuchbild h_mb7_bauen zeigt den Rollout "Fuer das Spiel bauen" zugeklappt (wie schon beim ersten Agenten).
- md_toaj_jedi ruhend 0,12 ms mehr CPU-Mittel, nur einmal gemessen.
- MD-MP: Daten von shank geloescht, nicht geprueft.

## 6. Vorschlag Stufe 6 (Auto-Physik)

- Grundlage wie hier: dieselbe Schicht (FK-Schluessel, IK-Spuren), Physik als Filter darueber, der Schluessel erzeugt statt sie zu
  ersetzen (wie Cascadeur "AutoPhysics": Schwerpunkt, Impuls, Bodenkontakte der Fuesse aus Stufe 4).
- Pruefung gegen das Spiel wie in Stufe 7: Spruenge/Fallen einer Figur mit SET_ORIGIN am Ende, Vorschaufotos durch die Skriptkamera
  (`vergleich2.py` in Bildpunkten, Deckung der Silhouetten), MD und Original-JKA.
- Kontakte an der Karte (BSP-Boden, Kanten) aus `traceRay` wie "Fuss auf den Boden"; Gewichte je Knochen aus der GLA-Hierarchie,
  bei Bedarf einstellbar; Rueckgaengig je Durchgang.
- Rot zuerst: Absprung ohne Physik (Schwerpunkt ausserhalb der Stuetzflaeche) messbar machen.

## Changelog

### English
- Mission tab: animate a figure of the scene right on the map - "Animate figure" in the inspector, the right-click menu or its own window gives an NPC its own animation from the time cursor on: a layer over a base animation of its skeleton with the same controls as the model tab (like 3ds Max: Q/W/E, F5-F8, K; like Blender: G/R with X/Y/Z, I), IK for hands and feet, FK for the rest, rollout IK/FK, dope sheet with IK rows and curves.
- IK targets in map coordinates: "Foot on the floor" puts a foot onto the floor of the map, "Target on surface" puts a hand onto a door, console or any surface you click.
- The path of the pelvis is the way of the figure: drag its dots in the map. The script block ends with SET_ORIGIN (start plus the way of the pelvis) together with the following animation, so the figure stays where the animation ended - without SET_ORIGIN it jumps back, as measured in Movie Duels and the original JKA.
- The preview plays what the game gets: every change bakes the map GLA _humanoid_<map> again (an existing map GLA keeps its slots) and hangs it on the skeleton like the game.
- "Build for the game" writes the map GLA (or _humanoid.gla in your mod folder, or an own GLA - chosen only now), its animation.cfg and the script as .txt and .IBI with SET_ANIM_BOTH, hold time, SET_ORIGIN and the following animation; the limits of the game are checked first.
- Several figures: each with its own layer and slot, waitsignal/signal between them, and a hand or foot can follow a bone of another figure (grip, hand-over), baked into its GLA.
- Model tab, dope sheet: IK keys have their own rows and move together with the FK keys of their chain.
- Mission preview like the game: an NPC that would spawn inside another body appears only when the place is free (NPC_Begin tries again every "wait", 500 ms by default); an affect on it before that is skipped.
- Maps without textures: surfaces whose shader and image are missing show the default box texture of the game instead of nothing.
- The shadow of a figure playing an animation of the map GLA follows the figure, like the game.

### Deutsch
- Mission-Tab: eine Figur der Szene direkt auf der Karte animieren - „Figur animieren“ im Inspektor, im Rechtsklick-Menü oder im eigenen Fenster gibt einem NPC ab dem Zeitzeiger eine eigene Animation: eine Schicht über einer Grundanimation seines Skeletts mit derselben Bedienung wie im Model-Tab (wie 3ds Max: Q/W/E, F5-F8, K; wie Blender: G/R mit X/Y/Z, I), IK für Hände und Füße, FK für den Rest, Rollout IK/FK, Dopesheet mit IK-Zeilen und Kurven.
- IK-Ziele in Koordinaten der Karte: „Fuß auf den Boden“ setzt einen Fuß auf den Boden der Karte, „Ziel auf Fläche“ legt eine Hand an eine Tür, Konsole oder jede angeklickte Fläche.
- Der Weg des Beckens ist der Weg der Figur: seine Punkte in der Karte ziehen. Der Skriptblock endet mit SET_ORIGIN (Start plus Weg des Beckens) zusammen mit der Folgeanimation, die Figur bleibt also stehen, wo die Animation endete - ohne SET_ORIGIN springt sie zurück, wie in Movie Duels und im Original-JKA gemessen.
- Die Vorschau spielt, was das Spiel bekommt: jede Änderung backt die Karten-GLA _humanoid_<karte> neu (eine vorhandene behält ihre Plätze) und hängt sie wie das Spiel an das Skelett.
- „Für das Spiel bauen“ schreibt die Karten-GLA (oder _humanoid.gla im eigenen Mod-Ordner oder eine eigene GLA - erst jetzt gewählt), ihre animation.cfg und das Skript als .txt und .IBI mit SET_ANIM_BOTH, Haltezeit, SET_ORIGIN und Folgeanimation; vorher werden die Grenzen des Spiels geprüft.
- Mehrere Figuren: jede mit eigener Schicht und eigenem Platz, waitsignal/signal zwischen ihnen, und eine Hand oder ein Fuß kann einem Knochen einer anderen Figur folgen (Griff, Übergabe), gebacken in ihre GLA.
- Model-Tab, Dopesheet: IK-Schlüssel haben eigene Zeilen und wandern mit den FK-Schlüsseln ihrer Kette.
- Missions-Vorschau wie im Spiel: Ein NPC, der in einem anderen Körper spawnen würde, erscheint erst, wenn der Platz frei ist (NPC_Begin versucht es je "wait" wieder, Vorgabe 500 ms); ein affect auf ihn davor entfällt.
- Karten ohne Texturen: Flächen ohne Shader und Bild zeigen das Kastenbild des Spiels statt gar nichts.
- Der Schatten einer Figur, die eine Animation der Karten-GLA spielt, geht mit der Figur mit - wie im Spiel.

### 中文
- 任务选项卡: 直接在地图上为场景中的角色制作动画 - 在检查器、右键菜单或独立窗口中点击“为角色制作动画”, NPC 从时间指针起获得自己的动画: 在其骨架的基础动画上的一层, 操作与模型选项卡相同 (像 3ds Max: Q/W/E、F5-F8、K; 像 Blender: G/R 配合 X/Y/Z、I), 手和脚用 IK, 其余用 FK, IK/FK 卷展栏, 带 IK 行和曲线的摄影表。
- 地图坐标中的 IK 目标: “脚落地”把脚放到地图地面上, “目标置于表面”把手放到门、控制台或任意点击的表面上。
- 骨盆路径就是角色的行进路线: 在地图中拖动其点。脚本块以 SET_ORIGIN (起点加骨盆位移) 和后续动画结尾, 因此角色停在动画结束处 - 没有 SET_ORIGIN 时会跳回, 这在 Movie Duels 和原版 JKA 中测得。
- 预览播放游戏得到的内容: 每次改动都会重新烘焙地图 GLA _humanoid_<地图> (已有的保留其槽位) 并像游戏一样挂到骨架上。
- “为游戏生成”写出地图 GLA (或你 mod 文件夹中的 _humanoid.gla, 或自有 GLA - 此时才选择)、其 animation.cfg 以及 .txt 与 .IBI 脚本, 含 SET_ANIM_BOTH、保持时间、SET_ORIGIN 和后续动画; 先检查游戏限制。
- 多个角色: 各有自己的层和槽位, 之间使用 waitsignal/signal, 手或脚可以跟随另一个角色的骨骼 (握持、交接), 烘焙到其 GLA 中。
- 模型选项卡的摄影表: IK 关键帧有自己的行, 并随其链的 FK 关键帧一起移动。
- 任务预览与游戏一致: 会在另一个身体内生成的 NPC 要等位置空出才出现 (NPC_Begin 按 "wait" 重试, 默认 500 毫秒); 在此之前对它的 affect 会被跳过。
- 没有纹理的地图: 缺少着色器和图像的表面显示游戏的默认方格纹理, 而不是什么都不显示。
- 播放地图 GLA 动画的角色, 其阴影会跟随角色移动, 与游戏一致。

### 日本語
- ミッションタブ: シーンのフィギュアをマップ上で直接アニメート - インスペクター、右クリックメニュー、専用ウィンドウの「フィギュアをアニメート」で NPC にタイムカーソルから独自のアニメーションを付けます: スケルトンのベースアニメーション上のレイヤーで、操作はモデルタブと同じ (3ds Max 風: Q/W/E、F5-F8、K。Blender 風: G/R と X/Y/Z、I)。手と足は IK、それ以外は FK、IK/FK ロールアウト、IK 行とカーブ付きのドープシート。
- マップ座標の IK ターゲット: 「足を床に」で足をマップの床に、「ターゲットを面に」で手をドア・コンソール・クリックした任意の面に置きます。
- 骨盤の経路がフィギュアの道筋です: マップ上でその点をドラッグ。スクリプトブロックは SET_ORIGIN (開始 + 骨盤の移動) と次のアニメーションで終わり、フィギュアはアニメーションの終わった場所に留まります - SET_ORIGIN がないと戻ります (Movie Duels とオリジナル JKA で測定)。
- プレビューはゲームが受け取るものを再生: 変更のたびにマップ GLA _humanoid_<マップ> をベイクし直し (既存のものはスロットを保持)、ゲームと同じくスケルトンに付けます。
- 「ゲーム用にビルド」はマップ GLA (または Mod フォルダーの _humanoid.gla、または独自 GLA - ここで選択)、その animation.cfg、.txt と .IBI のスクリプトを SET_ANIM_BOTH、保持時間、SET_ORIGIN、次のアニメーション付きで書き出します。先にゲームの制限を確認します。
- 複数のフィギュア: それぞれ独自のレイヤーとスロット、間に waitsignal/signal。手や足は別のフィギュアのボーンに従えます (つかみ、受け渡し)。その GLA にベイクされます。
- モデルタブのドープシート: IK キーに専用の行があり、チェーンの FK キーと一緒に動きます。
- ミッションプレビューをゲーム通りに: 別の体の中にスポーンする NPC は場所が空いてから現れます (NPC_Begin は "wait" ごとに再試行、既定 500 ms)。それ以前の affect は飛ばされます。
- テクスチャのないマップ: シェーダーも画像もない面は、何も表示しない代わりにゲームの既定の枠テクスチャを表示します。
- マップ GLA のアニメーションを再生するフィギュアの影は、ゲームと同様にフィギュアについて動きます。

### Español
- Pestaña Mission: anima una figura de la escena directamente en el mapa: «Animar figura» en el inspector, el menú del clic derecho o su propia ventana da a un NPC su propia animación desde el cursor de tiempo: una capa sobre una animación base de su esqueleto con los mismos controles que la pestaña Model (como 3ds Max: Q/W/E, F5-F8, K; como Blender: G/R con X/Y/Z, I), IK para manos y pies, FK para lo demás, panel IK/FK, hoja de claves con filas IK y curvas.
- Objetivos IK en coordenadas del mapa: «Pie al suelo» pone un pie en el suelo del mapa, «Objetivo en superficie» pone una mano en una puerta, una consola o cualquier superficie que pulses.
- El trayecto de la pelvis es el camino de la figura: arrastra sus puntos en el mapa. El bloque de script termina con SET_ORIGIN (inicio más el trayecto de la pelvis) junto con la animación siguiente, así la figura se queda donde terminó la animación; sin SET_ORIGIN vuelve atrás, como se midió en Movie Duels y en el JKA original.
- La vista previa reproduce lo que recibe el juego: cada cambio vuelve a hornear el GLA de mapa _humanoid_<mapa> (uno existente conserva sus huecos) y lo cuelga del esqueleto como el juego.
- «Crear para el juego» escribe el GLA de mapa (o _humanoid.gla en tu carpeta de mod, o un GLA propio, elegido solo ahora), su animation.cfg y el script como .txt e .IBI con SET_ANIM_BOTH, tiempo de espera, SET_ORIGIN y la animación siguiente; antes revisa los límites del juego.
- Varias figuras: cada una con su capa y su hueco, waitsignal/signal entre ellas, y una mano o un pie puede seguir a un hueso de otra figura (agarre, entrega), horneado en su GLA.
- Pestaña Model, hoja de claves: las claves IK tienen sus propias filas y se mueven con las claves FK de su cadena.
- Vista previa de misiones como en el juego: un NPC que aparecería dentro de otro cuerpo solo aparece cuando el sitio queda libre (NPC_Begin lo reintenta cada "wait", 500 ms por defecto); un affect sobre él antes de eso se omite.
- Mapas sin texturas: las superficies sin shader ni imagen muestran la textura de caja por defecto del juego en lugar de nada.
- La sombra de una figura que reproduce una animación del GLA de mapa sigue a la figura, como en el juego.

### Čeština
- Karta Mission: animace postavy scény přímo na mapě - „Animovat postavu“ v inspektoru, v nabídce pravého tlačítka nebo ve vlastním okně dá NPC od časového ukazatele vlastní animaci: vrstvu nad základní animací jeho kostry se stejným ovládáním jako v kartě Model (jako 3ds Max: Q/W/E, F5-F8, K; jako Blender: G/R s X/Y/Z, I), IK pro ruce a chodidla, FK pro zbytek, rollout IK/FK, dopesheet s řádky IK a křivkami.
- Cíle IK v souřadnicích mapy: „Chodidlo na zem“ postaví chodidlo na podlahu mapy, „Cíl na plochu“ položí ruku na dveře, konzoli nebo libovolnou klepnutou plochu.
- Dráha pánve je cestou postavy: táhněte její body v mapě. Blok skriptu končí příkazem SET_ORIGIN (začátek plus dráha pánve) spolu s následnou animací, takže postava zůstane, kde animace skončila - bez SET_ORIGIN skočí zpět, jak bylo změřeno v Movie Duels a v původním JKA.
- Náhled přehrává to, co dostane hra: každá změna znovu zapeče mapové GLA _humanoid_<mapa> (existující si ponechá sloty) a pověsí je na kostru jako hra.
- „Sestavit pro hru“ zapíše mapové GLA (nebo _humanoid.gla ve vlastní složce modu, nebo vlastní GLA - zvoleno až teď), jeho animation.cfg a skript jako .txt i .IBI se SET_ANIM_BOTH, dobou držení, SET_ORIGIN a následnou animací; předem zkontroluje limity hry.
- Více postav: každá s vlastní vrstvou a slotem, mezi nimi waitsignal/signal, a ruka nebo chodidlo může sledovat kost jiné postavy (úchop, předání), zapečeno do jejího GLA.
- Karta Model, dopesheet: klíče IK mají vlastní řádky a pohybují se s klíči FK svého řetězce.
- Náhled mise jako ve hře: NPC, který by se objevil uvnitř jiného těla, se objeví až po uvolnění místa (NPC_Begin to zkouší znovu po "wait", výchozí 500 ms); affect na něj předtím se vynechá.
- Mapy bez textur: plochy bez shaderu a obrázku ukazují výchozí rámečkovou texturu hry místo ničeho.
- Stín postavy, která přehrává animaci mapového GLA, jde s postavou - jako ve hře.
