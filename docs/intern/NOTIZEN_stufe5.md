# Notizen Modell-Block Stufe 5: Retargeting und neue Rigs, 5b Presets (Generator)

Stand 07.11.2026, Zweig `karte1`, auf 57862ebcf (Stufe 6). Ein Agent. Laufnamen `rt_*` (5b `pr_*`, Schluss `rte_*`), Laufskript
`lauf.py` wie Stufe 6 mit `BHED_RT_GLA` (die GLAs der Spiele als lose Dateien, nur gelesen: `scratchpad/rt/gla_holen.py`).
kFassung bleibt rc610.

## Quellen

- Vorbilder nur nach Doku: Unreal IK Retargeter (Retarget-Pose, Ketten, Op-Stapel Becken -> FK -> IK -> Wurzel), MotionBuilder/
  HumanIK (Charakterisieren in der T-Pose, Reach, Plotten). Kein fremder Code.
- Namen der Vorlagen: HumanIK und Biped nach MotionBuilder SDK `tasks/3ds Max Biped Template.py` (bipedMap), Mixamo = HumanIK mit
  `mixamorig:`; CAT nach `3ds Max 2016/plugcfg_ln/CAT/CATRigs/Base Human.rig` (nur gelesen, Max nicht gestartet).
- Die 34 eigenen Skelette: `GameData original/base` (assets*.pk3), Ketten aus `rtVorschlag`, von Hand durchgesehen
  (`scratchpad/rt/vorlagen_gen.py`, KORREKTUR je Skelett), erzeugt `src/modell/rig_vorlagen.inc`.
- fScale: `mdx_format.h:385` ("scale it was built with"); die Grundstellung der GLA traegt ihn (`_humanoid`: BasePoseMat = 0,64 R).
- Generator: NPC walkSpeed 90 / runSpeed 300 (NPC_stats.cpp:1996-1997 OpenJK), Standphase 60 % beim Gehen (Perry/Burnfield, Gait
  Analysis 2010), ~35 % beim Laufen (Novacheck 1998: 39 % beim Joggen), Becken +-4 Grad drehen/kippen (Perry/Burnfield).
- Kontaktregel aus Stufe 6 (tiefster Fusspunkt hoechstens 0,5 ueber seinem tiefsten Stand), Zwei-Glied-IK aus Stufe 4.

## 1. Gebaut

- Backend `include/bhed/modell/retarget.h`, `src/modell/retarget.cpp`, Einstellungen im Dokument (`RigDaten::rt`, rigdaten.h):
  - Charakter (Ketten Wirbel/Hals/Kopf/Schluesselbein/Arm/Bein/Finger/Anhang, Becken, Wurzel, Motion, Drehknochen mit Anteil,
    Folger: Finger/Tags der Hand, Kettenglieder im flachen JK2-Skelett, Doppelknochen am selben Ort).
  - 38 Vorlagen: JKA _humanoid (53), JK2 _humanoid (72), HumanIK/Mixamo, Biped, CAT, die 33 uebrigen eigenen Skelette des
    Basisspiels; Erkennung ueber Datei + Knochenzahl, sonst Anteil gleicher Namen (>= 80 %); sonst Ketten vorgeschlagen.
  - Zuordnung ueber den Kettenschluessel (arm_l_0 ...), von Hand bleibt, nur leere fuellen; Knochen in der Kette Glied fuer Glied,
    sonst gleicher Name (Finger), sonst ueber die Lage auf der Kette; uebrige ueber den Namen.
  - Retarget-Pose (Bindepose / T / A, je Glied die kleinste Drehung; von Hand je Knochen; benannte Posenpaare).
  - Uebertragen: Becken (k = Beinlaenge Ziel/Quelle, Huefte ueber dem Knoechel), FK (Weltdrehung gegenueber der Pose), IK
    (Haende/Fuesse, Reach), Fusskontakte (gleichmaessig mit dem Boden der Quelle, nach dem Abheben ueber den Schwung ausgeblendet),
    Wurzel; Reihenfolge umstellbar. Backen mit compressBone (Skelett des Ziels, fps der animation.cfg), auch als Schicht ueber der
    Bindepose (Figur im Mission-Tab). Pruefen: Fussrutschen, Spruenge (mit Knochen).
  - 5b: Geh-/Laufzyklus aus Reglern (Tempo, Schrittlaenge, Arm-/Hueftschwung, Fusshub, Haltung normal/schwer/schleichend/
    verletzt/Saebel gezogen), nahtlos, Standfuss exakt am Boden; Messen von Tempo/Schritt an BOTH_WALK1/BOTH_RUN1 der Figur.
- Model-Tab (`gui/app_modelltab_retarget.cpp`): Rollouts "Retargeting" und "Presets" unter Auto-Physik, Untermenues im Menue
  Animation und im Rechtsklick (Rig), Suche (X / F3), Tooltips; Begriffe je Modus (Max/MotionBuilder: Charakter-Definition,
  Charakterisierungs-Pose, Plotten; Blender: Knochen-Zuordnung, Ruhepose, Backen). Vorschau auf der Figur = gepackte GLA, die
  Quelle als Skelett daneben (Achse nach der Ansicht), Abspielen, Fussrutsch-Messer je Bild, Spruenge zum Anklicken, Backen auf
  einen Platz (Karten-GLA / _humanoid.gla / eigene GLA wie Stufe 2).
- Tests: ctest `modell_retarget`, Selbsttest-Gruppen `rt`, `rtjka`, `rtspiel`, `rtzeit`, `pr` in modellblock, Handbuchseite
  `modell-retarget` (mit Abschnitt Presets).

## 2. Rot -> gruen

ctest `modell_retarget` (Hand-Tabelle der Knochen-Erkennung: JKA, JK2, Bantha, Droideka; Testgang mit IK-gefuehrtem Schwungfuss):
- Bindepose: Quelle mit T-Bindepose, Ziel mit der A-Bindepose des Spiels: ohne Ausrichten 48,08 Grad verdrehte Arme (Posen 48,08)
  -> ausgerichtet T 0,00 / A 0,03 Grad; Pose von Hand 20 Grad am Oberarm -> 19,997.
- Laenge: Beine 1,3-fach, ohne Laengenausgleich rutscht der Standfuss 24,36 Einheiten (Messer 24,72) -> mit Becken (k 1,2809 =
  Laenge der gestreckten Beine) 0,016; nur Unterschenkel 1,3-fach: FK 0,68 -> IK + Kontakte 0,023 (Messer 0,003); Reach 0 = FK,
  Reach 0,5 0,35 dazwischen; Reihenfolge FK, IK, Becken 21,7 (umgestellt wirkt).
- Gleiche Figur: Quelle = Ziel ohne IK Ort 0,018 / Drehung 0,003 Grad; mit Fuss-IK 0,31 / 1,0 (das fast gestreckte Knie folgt dem
  gepackten Fussort, Grenze 0,4 / 1,5), Fuesse 0,019; gebacken 0,30; als Schicht ueber der Bindepose gebacken = rtBacken.
- fScale der Grundstellung (die GLAs des Spiels: BasePoseMat = 0,64 R): Quelle = Ziel gebacken 21,07 Einheiten daneben (rot, die
  Vorschau im Model-Tab lag 25 Einheiten neben der Rechnung, `rt_rt2`) -> 0,31 (`starrMitMass`: innen starr, am Ende X = starr *
  Mass); Generator 1,000 -> 0,640.
- Saebel-Tag rhang_tag_bone an rhand (JK2 -> JKA): 0,0000; Spruenge: ein umgedrehtes Bild gefunden (10/11), gleiche Folge keine.
- Vorlagen JKA/JK2/Mixamo/Biped/CAT erkannt (Mixamo/Biped zuerst nicht: Anteil an allen Eintraegen statt an den gemeinsamen),
  JK2 zuerst als Protokolldroide (Rockschoesse in der JK2-Vorlage, Gleichstand nach Treffern) -> JK2.
- Generator (5b): je Haltung Gehen/Laufen Naht 0,0000, Standfuss am Boden 0,0000, Gleiten 0,0000; Pruefer-Rotprobe (Schritt 80) Boden
  22,0; Saebel gezogen rechte Hand 13,7 vor dem Becken (normal 7,5).

Selbsttest `rt` (Movie Duels, Sturmtruppler; Quellen aus BHED_RT_GLA):
- Lose Quelle las die animation.cfg aus den Archiven von Movie Duels (readFromArchives nimmt auch lose Dateien): 1 Bild statt 26
  (`rt_rt1`) -> Datei auf der Platte zuerst (`rt_rt2`: 26 Bilder, 20 fps wie JK2 BOTH_WALK1).
- Fussrutschen bei Gaengen auf der Stelle: der Messer mass den Gang selbst, das Kontakthalten hielt den Fuss fest, wo er im Spiel
  gleiten muss (Wampa "alles an" 10,1) -> Kontakt = gleichmaessig mit dem Boden der Quelle (Wampa 0,24, ohne Becken 11,2).
- Spruenge beim Abheben (JK2 -> MD 10) -> Ausblenden ueber den Schwung (5); k 0,906 (Beinlaenge bis zu den JK2-Zehen) -> Knoechel,
  k 1,000; uebrig 4 Spruenge nur mit Kontakten (ohne Kontakte 0, nur FK 0): die Korrektur bis zur Abweichung der Quelle (1,45) am
  fast gestreckten Knie - die Pruefung markiert sie (Knochen im Tooltip).
- Vorschau im Leerlauf: `rt_zeit1` 0 von 120 Bildern neu gezeichnet (die Uhr lief im Rollout, das bei der Messung nicht gezeichnet
  wird, und der Model-Tab meldete keinen Bildbedarf) -> `rt_zeit3` 120 von 120 (`modellRtUhr` neben der Uhr der Zeitleiste,
  `modellBrauchtBilder`).
- Untermenue Auto-Physik zeigte die Ueberschrift "IK/FK" (kGruppen endet bei 5) -> eigene Ueberschrift (auch Retargeting, Presets).

## 3. Gegenproben (Selbsttest, Einheiten des Spiels)

- Vorlagen: alle 34 eigenen Skelette von JKA (assets*.pk3) erkennen ihre Vorlage mit fScale; JK2 -> `_humanoid (JK2)`, MD -> JKA;
  rancor_ice_spider (nur MD) bekommt 11 vorgeschlagene Ketten, 8 Glieder, Becken hip_bone.
- JK2 BOTH_WALK1 -> MD-Sturmtruppler: k 1,000, 15 von 17 Ketten (JK2 ohne Rockschoesse), Fussrutschen alles an 0,064, nur FK 1,44,
  ohne Becken 1,36, Quelle selbst 1,45; Saebel-Tag 0,0000; Vorschau gepackt gegen gerechnet 0,023.
- JKA BOTH_RUN1 -> MD-Figur (Menschenform): Fussrutschen 0,000 (Quelle 0,77), keine Spruenge, 17 Ketten.
- JKA BOTH_WALK1 -> Wampa (Kreatur, Vorlage): k 1,675, 13 Ketten; ohne Becken 11,20, nur FK 3,05, alles an 0,24 (Quelle 2,26), 1 Sprung.

## 4. Im Spiel

Szene mb7raum (wie Stufe 6/7): mb7_a spielt den JK2-Gang, uebertragen auf ihr Skelett (rtAlsSchicht), zweimal (52 Bilder, 20 fps),
gebaut als Karten-GLA (SET_ANIM_BOTH BOTH_CIN_1), Kamera von der Seite; Vorschau je 250 ms, Spiel je ~0,16 s (`rt/spiel.py`,
`rt/orig_spiel.py`), Auswertung `rt/vergleich.py` (Silhouette ohne Kinobalken, Versatz einmal gesucht).
- Movie Duels (`rtmd1`): im Gang Deckung 0,75-0,94, Schwerpunkt 0,4-5 px (Fotoabstand bis 0,08 s), danach im Stand 0,987 / 1,5 px.
- Original-JKA (`rtorig1`; steam.exe lief, ActiveUser != 0, vor dem Start geprueft): im Gang 0,58-0,92, Stand 0,943 / 0,6 px.
- Paarbilder 1920 x 2880 (oben Spiel, unten Vorschau): `rt/paare_md`, `rt/paare_orig` im Scratchpad, angesehen.
- Spielordner: Movie Duels gleich; "GameData" und "GameData original" gleich.

## 5. Bildzeit (allein, BHED_D3D_DEBUG=0)

- `rt_zeit3`: Sturmtruppler, eigene Animation abspielend 0,79 / 1,40 ms (Bild-CPU Mittel / p99); Retargeting-Vorschau abspielend mit
  der Quelle daneben und offenem Rollout 0,82 / 1,39 ms; Bildabstand 4,2 ms (240 Hz), je 120 von 120 Bildern gezeichnet. Eine
  Rechnung (Uebertragen + Packen, 26 Bilder) 1,6 ms. Presets: ein Zyklus 5-9 ms (24-30 Bilder, nur nach einer Aenderung).

## 6. Schluss

Einmal, nach sauberem Neubau (`--clean-first`, /W4 /WX, 0 Warnungen; danach nur Folgebauten, je 0 Warnungen):

- ctest 229 von 229 (112 s), darunter neu `modell_retarget` (alles ok, mit Drehknochen 45,0 Grad bei 90 Grad Handdrehung).
- Laeufe `rte_*` (zwei Wellen, Summe `laeufe/rte_summe.txt`): modellblock funktionen 60/0, tasten 43/0, mb2 37/0, blender 44/0,
  mb3 21/0, rig 26/0, mb5 159/0, mb6 37/0, mb7 24/0, mb7spiel 4/0, mb7textur 1/0, ph 27/0, rt 19/0, rtjka 1/0, pr 15/0; rundlauf,
  rigrundlauf, bild, kurvenrundlauf, ikrundlauf je jka/jk2/md ohne FEHLER; modell 14/0, mission1 98/0, kuerzel 202/0, Modus 1 2635/0.
- Handbuch: Seite `modell-retarget` (Gruppe Missionen, nach modell-physik) in 6 Sprachen mit Abschnitt Presets, Bilder
  `h_rt_uebertragen`, `h_rt_ketten`, `h_pr_gang` je Sprache (2538 x 1384; de, ja, cs, en angesehen); `gen_handbuch` alle Seiten x 6
  Sprachen, `--pruefen` ok, `gen_hilfe_orte` 206 Orte.
- Lints wie in der Basis (portability 99, pruefcodes 11, efxflags/fremdprogramm/link/tidy unveraendert), alle anderen ok, auch
  lint_sources, lint_i18n, lint_i18n_erzeugt, lint_fassung (rc610). Gefunden und behoben: ein fester Text "k = %.3f" (lint_i18n),
  `<cstddef>` im ctest (lint_portability 100 -> 99).
- Spielordner: Movie Duels je Lauf vorher/nachher gleich; Original-JKA "GameData" und "GameData original" gleich.
- Nach den Schlusslaeufen (beim Ansehen der Handbuchbilder): in es waren die Reglerbeschriftungen der Presets abgeschnitten
  ("Balanceo de brazos") und das Preset-Bild schnitt die Figur ab - Regler und Auswahlfelder nehmen jetzt die Breite bis zu ihrer
  Beschriftung, das Bild passt die Figur allein ein; Folgebau 0 Warnungen, Handbuchbilder 6 Sprachen neu (`rte_hb3_*`, es angesehen).

## 7. Abweichungen und Offen

- Eigene GLA mit neuem Skelett im SP laden: nicht im Spiel geprueft (es braucht eine .glm, die auf das neue Skelett zeigt, ihre
  Haut und einen NPC-Typ - das Bauen der eigenen GLA selbst ist Stufe 2). Im Spiel geprueft ist die Karten-GLA (SET_ANIM_BOTH).
- Mit Fusskontakten markiert die Pruefung bei JK2 -> MD 4 Bilder als Sprung (Huefte/Knie, die Korrektur der Quelle am fast
  gestreckten Knie); ohne Kontakte keine. Kein weiches IK (Soft-IK) - dafuer fehlte eine Quelle fuer die Weichheit.
- Kontaktregel ohne Geschwindigkeit (Regel und Messgrund aus Stufe 6): Bilder kurz vor dem Aufsetzen, die schon tief genug sind,
  zaehlen zum Kontakt; der Boden des Kontakts ist der Mittelwert ueber ihn.
- Mixamo/Biped/CAT: nur die Namen (Vorlagen); Dateien aus diesen Programmen (FBX/BIP) liest JK Studio nicht - der Weg dorthin ist
  heute XSI/GLA. Keine Mixamo-Dateien geladen oder mitgeliefert.
- Drehknochen ohne Quelle nehmen ihren Anteil am Twist des Endknochens; Fingerketten mit verschiedener Gliederzahl nehmen den
  gleichen Namen, sonst die Lage auf der Kette.
- 5b Bibliotheken: Quaternius Universal Animation Library (itch.io, "Name your own price", Standard ohne Mindestpreis, CC0 laut
  Seite) und 100STYLE (ianxmason.com/100style: "Creative Commons Attribution 4.0 International License", Nennung "The 100STYLE
  Dataset - Ian Mason") sind belegt, aber nicht heruntergeladen und nicht eingebaut: es fehlt ein Import fuer glTF/FBX bzw. BVH.
  Ohne sie gibt es keine fremden Presets, daher auch kein Eintrag in CREDITS/Ueber-Dialog. Mitgeliefert ist nur der eigene Generator.
- Generator: Haltungen, Armschwung, Hub, Spur sind Regler-Vorgaben ohne Spielwert; Laufen mit dem an BOTH_RUN1 gemessenen Schritt
  (62 Einheiten bei Beinlaenge 29) senkt das Becken tief, damit der Standfuss am Boden bleibt.
- MD-MP: Daten von shank geloescht, nicht geprueft.

## 8. Vorschlag 1b (UV)

- UV-Bereich im Model-Tab nach WhiteoutFlakes (nur UV, wie vereinbart): Abwicklung je Flaeche der GLM mit Naehten aus den geteilten
  Ecken (die .glm teilt Ecken an UV-Naehten - Stufe 1a), Ansicht der UV-Inseln ueber der Textur, Auswahl/Verschieben/Drehen/Skalieren
  wie Max (Edit UVWs) und Blender (UV-Editor), Rueckgaengig ueber das Dokument.
- Pruefen gegen das Spiel: GLM schreiben (Stufe 2) und im Spiel fotografieren (Textur sitzt), Rundlauf UV -> GLM -> UV bytegleich,
  Grenzen der GLM (Ecken je Flaeche) als Befund. Danach 1c Painting auf denselben Inseln.

## Changelog

### English
- Model tab: retargeting - give your figure the animation of another skeleton (JK2 to JKA, JKA to a Movie Duels figure, a human walk onto a creature). Both go into the same T or A pose (each limb with the smallest rotation, adjustable by hand), the chains are mapped automatically and by hand, the pelvis and root motion are scaled by the leg lengths, hands and feet reach their places (Reach like HumanIK), standing feet move with the ground of the source; the steps pelvis, FK, IK, root can be reordered.
- Rig templates for JKA _humanoid, JK2 _humanoid, HumanIK/Mixamo, 3ds Max Biped and CAT and all 34 own skeletons of the base game (rancor, wampa, AT-ST, howler ...); any other skeleton gets suggested chains.
- The figure shows the result packed like the game, the source plays as a skeleton beside it; a foot sliding meter and marked jumps check it; "Plot to slot" bakes it with the fps of the source's animation.cfg into the map GLA, _humanoid.gla or your own GLA. In the game (Movie Duels and original JKA) the transferred walk matches the preview.
- Presets: walk and run cycles from our own generator - speed, step length (measured from the figure's own walk/run), arm and hip swing, foot lift, posture normal, heavy, sneaking, injured or saber drawn; seamless, the standing foot exactly on the ground.
- Rollouts "Retargeting" and "Presets" with submenus in the menu Animation and the right-click, all actions in the search (X / F3), terms like MotionBuilder in 3ds Max mode and like Blender in Blender mode.

### Deutsch
- Model-Tab: Retargeting - gib deiner Figur die Animation eines anderen Skeletts (JK2 auf JKA, JKA auf eine Movie-Duels-Figur, ein Menschengang auf eine Kreatur). Beide gehen in dieselbe T- oder A-Pose (jedes Glied mit der kleinsten Drehung, von Hand anpassbar), die Ketten werden automatisch und von Hand zugeordnet, Becken und Wurzelbewegung nach den Beinlängen skaliert, Hände und Füße erreichen ihre Orte (Reach wie HumanIK), stehende Füße bewegen sich mit dem Boden der Quelle; die Schritte Becken, FK, IK, Wurzel lassen sich umstellen.
- Rig-Vorlagen für JKA _humanoid, JK2 _humanoid, HumanIK/Mixamo, 3ds Max Biped und CAT und alle 34 eigenen Skelette des Basisspiels (Rancor, Wampa, AT-ST, Howler ...); jedes andere Skelett bekommt vorgeschlagene Ketten.
- Die Figur zeigt das Ergebnis gepackt wie im Spiel, die Quelle spielt als Skelett daneben; ein Fußrutsch-Messer und markierte Sprünge prüfen es; „Auf Platz plotten“ backt es mit den fps der animation.cfg der Quelle in die Karten-GLA, die _humanoid.gla oder eine eigene GLA. Im Spiel (Movie Duels und Original-JKA) entspricht der übertragene Gang der Vorschau.
- Presets: Geh- und Laufzyklen aus unserem eigenen Generator - Tempo, Schrittlänge (gemessen am eigenen Gehen/Laufen der Figur), Arm- und Hüftschwung, Fußhub, Haltung normal, schwer, schleichend, verletzt oder Säbel gezogen; nahtlos, der Standfuß genau am Boden.
- Rollouts „Retargeting“ und „Presets“ mit Untermenüs im Menü Animation und im Rechtsklick, alle Aktionen in der Suche (X / F3), Begriffe wie MotionBuilder im 3ds-Max-Modus und wie Blender im Blender-Modus.

### 中文
- 模型选项卡: 重定向 - 让角色获得另一个骨架的动画 (JK2 到 JKA、JKA 到 Movie Duels 角色、人的步行到生物)。两者进入同一 T 或 A 姿势 (每个肢体以最小旋转, 可手动调整), 链条自动或手动对应, 骨盆和根运动按腿长缩放, 手脚到达目标 (Reach 同 HumanIK), 站立的脚随来源地面移动; 骨盆、FK、IK、根的步骤可调整顺序。
- 绑定模板: JKA _humanoid、JK2 _humanoid、HumanIK/Mixamo、3ds Max Biped 和 CAT, 以及基础游戏全部 34 个自有骨架 (rancor、wampa、AT-ST、howler ...); 其他骨架获得建议的链条。
- 角色按游戏方式显示压缩后的结果, 来源以骨架在旁边播放; 脚打滑计和标记的跳变用于检查; “绘制到槽位”以来源 animation.cfg 的 fps 烘焙到地图 GLA、_humanoid.gla 或自己的 GLA。在游戏中 (Movie Duels 和原版 JKA) 传递的步行与预览一致。
- 预设: 我们自己的生成器生成步行和跑步循环 - 速度、步长 (从角色自己的步行/跑步测得)、手臂和髋部摆动、抬脚、姿态普通、沉重、潜行、受伤或拔出光剑; 无缝, 支撑脚精确贴地。
- “重定向”和“预设”卷展栏, Animation 菜单和右键中有子菜单, 所有操作都可在搜索 (X / F3) 中找到, 3ds Max 模式用 MotionBuilder 术语, Blender 模式用 Blender 术语。

### 日本語
- モデルタブ: リターゲット - 別のスケルトンのアニメーションをフィギュアに (JK2 から JKA、JKA から Movie Duels のフィギュア、人の歩きをクリーチャーへ)。両方を同じ T/A ポーズにし (各部位を最小の回転で、手で調整可)、チェーンを自動または手で対応付け、骨盤とルートモーションを脚の長さで拡縮し、手足は目標に届き (HumanIK の Reach と同じ)、立ち足はソースの地面と一緒に動きます。骨盤・FK・IK・ルートの順序は変更可能。
- リグのテンプレート: JKA _humanoid、JK2 _humanoid、HumanIK/Mixamo、3ds Max Biped、CAT、ベースゲームの独自スケルトン 34 種すべて (ランコア、ワンパ、AT-ST、ハウラー ...)。その他のスケルトンにはチェーンを提案します。
- フィギュアはゲームと同じく圧縮した結果を表示し、ソースはスケルトンとして隣で再生。足の滑りメーターとジャンプの印で確認し、「スロットへプロット」でソースの animation.cfg の fps でマップ GLA、_humanoid.gla、独自 GLA にベイクします。ゲーム (Movie Duels とオリジナル JKA) で転送した歩きはプレビューと一致します。
- プリセット: 独自ジェネレーターによる歩きと走りのサイクル - 速さ、歩幅 (フィギュア自身の歩き/走りから測定)、腕と腰の振り、足の上げ、姿勢 普通・重い・忍び足・負傷・セーバーを抜く。継ぎ目なし、立ち足は正確に接地。
- ロールアウト「リターゲット」と「プリセット」、Animation メニューと右クリックにサブメニュー、全操作は検索 (X / F3) にも。3ds Max 風では MotionBuilder、Blender 風では Blender の用語。

### Español
- Pestaña Model: retargeting: da a tu figura la animación de otro esqueleto (JK2 a JKA, JKA a una figura de Movie Duels, un paso humano a una criatura). Ambos pasan a la misma pose T o A (cada miembro con el giro mínimo, ajustable a mano), las cadenas se emparejan solas y a mano, la pelvis y el movimiento raíz se escalan por la longitud de las piernas, manos y pies alcanzan su sitio (Reach como en HumanIK), los pies apoyados se mueven con el suelo del origen; los pasos pelvis, FK, IK, raíz se pueden reordenar.
- Plantillas de rig para JKA _humanoid, JK2 _humanoid, HumanIK/Mixamo, Biped y CAT de 3ds Max y los 34 esqueletos propios del juego base (rancor, wampa, AT-ST, howler ...); cualquier otro esqueleto recibe cadenas sugeridas.
- La figura muestra el resultado empaquetado como el juego y el origen se reproduce como esqueleto al lado; un medidor de deslizamiento y saltos marcados lo comprueban; «Trazar en el hueco» lo hornea con los fps del animation.cfg del origen en el GLA de mapa, el _humanoid.gla o un GLA propio. En el juego (Movie Duels y JKA original) el paso transferido coincide con la vista previa.
- Presets: ciclos de andar y correr de nuestro propio generador: velocidad, longitud de paso (medida en el andar/correr propio de la figura), balanceo de brazos y cadera, altura del pie, postura normal, pesada, sigilosa, herida o sable desenvainado; sin costura, el pie de apoyo justo en el suelo.
- Paneles «Retargeting» y «Presets» con submenús en el menú Animation y en el clic derecho, todas las acciones en la búsqueda (X / F3), términos como MotionBuilder en modo 3ds Max y como Blender en modo Blender.

### Čeština
- Karta Model: retargeting - dejte postavě animaci jiné kostry (JK2 na JKA, JKA na postavu Movie Duels, lidskou chůzi na tvora). Obě přejdou do stejné T- nebo A-pózy (každá končetina nejmenší rotací, ručně upravitelná), řetězy se přiřadí automaticky i ručně, pánev a pohyb kořene se škálují podle délky nohou, ruce a chodidla dosáhnou svých míst (Reach jako HumanIK), stojící chodidla se pohybují se zemí zdroje; kroky pánev, FK, IK, kořen lze přeskládat.
- Šablony rigu pro JKA _humanoid, JK2 _humanoid, HumanIK/Mixamo, 3ds Max Biped a CAT a všech 34 vlastních koster základní hry (rancor, wampa, AT-ST, howler ...); každá jiná kostra dostane navržené řetězy.
- Postava ukazuje výsledek zabalený jako ve hře, zdroj hraje jako kostra vedle ní; měřič klouzání chodidel a označené skoky ho kontrolují; „Vykreslit do pozice“ ho zapeče s fps z animation.cfg zdroje do mapového GLA, _humanoid.gla nebo vlastního GLA. Ve hře (Movie Duels a původní JKA) odpovídá přenesená chůze náhledu.
- Předvolby: cykly chůze a běhu z našeho vlastního generátoru - rychlost, délka kroku (změřená na vlastní chůzi/běhu postavy), švih paží a boků, zvednutí chodidla, držení normální, těžké, plíživé, zraněné nebo s taseným mečem; bez švu, stojné chodidlo přesně na zemi.
- Rollouty „Retargeting“ a „Předvolby“ s podnabídkami v nabídce Animation a v pravém kliknutí, všechny akce ve vyhledávání (X / F3), pojmy jako MotionBuilder v režimu 3ds Max a jako Blender v režimu Blender.
