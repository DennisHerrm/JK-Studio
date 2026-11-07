# Notizen Animations-Import (p_animimport): glTF/FBX/BVH/COLLADA, freie Presets, eigenes Skelett im Spiel

Stand 07.11.2026 (paralleles Fenster 19-23 Uhr), Zweig `animimp1` in `Y:\animimp`, Basis e6daa5ca8 (1d). Ein Agent, keine
Unter-Agenten. Laufnamen `ai_*`, Laufskript `ab/lauf_animimp.py` (Ziel `Y:\animimp\laeufe`). kFassung bleibt rc610.

## Quellen

- assimp 6.0.2 (schon mit allen Lesern gebaut): FBXConverter.cpp (Metadaten UpAxis/FrontAxis/CoordAxis/UnitScaleFactor,
  correctRootTransform nur mit Netzen, mTicksPerSecond = Bildrate, "adjust relative timing"), glTF2Importer.cpp (1000 Ticks
  je s), ColladaLoader.cpp (mUnitSize, Z_UP -> Y_UP, AI_CONFIG_IMPORT_COLLADA_USE_COLLADA_NAMES), BVHLoader.cpp (Kanalfolge,
  1 / Frame Time, End Site = "EndSite_<eltern>", 3 Kanaele -> OFFSET).
- glTF 2.0 Spezifikation 3.4: Meter, +Y oben, Vorderseite +Z, -X rechts. FBX SDK FbxAxisSystem/FbxSystemUnit (cm je Einheit).
- JKA-Rahmen aus retarget.cpp: links +X, vorn -Y (= links x oben), oben +Z.
- Unreal Mannequin: Namen der Retarget-Ketten (IK Rig/IK Retargeter-Doku); 100STYLE: Hierarchie der BVH-Dateien selbst.
- SP g_client.cpp G_SetG2PlayerModel: rootBone = "model_root" (ohne ihn spielt SET_ANIM nichts).
- Lizenzen: Quaternius UAL itch.io-Seite und License.txt im Download ("CC0 1.0 Universal"), 100STYLE ianxmason.com/100style
  ("Creative Commons Attribution 4.0 International License", Nennung "The 100STYLE Dataset - Ian Mason") und Zenodo 8127870
  (license cc-by-4.0).

## 1. Gebaut

- `include/bhed/modell/animimport.h`, `src/modell/animimport.cpp`: `animImportieren` (assimp) -> GlaAnimation mit Skelett
  (Ruhepose der Datei als Grundstellung, starr, cm, JKA-Achsen) und je Bild den lokalen Stellungen unkomprimiert
  (`GlaAnimation::roh`, localMatrix liest dort; schreibGla lehnt ab) - Wurzelwege ueber 512 und Meter-Dateien passen nicht in
  die 14 Bytes des Spiels. Je Animation der Datei eine Sequenz (AnimEntry). Skelett = Knoten mit Spur oder als Netz-Knochen und
  die Knoten zwischen zweien; BVH-End-Sites weg. Start beim ersten Schluessel (Blender schreibt glTF/COLLADA ab frame_start).
  `schreibBvh` (Bibliothek): 6 Kanaele nur fuer bewegte Lagen, ZXY ueber atan2.
- Model-Tab, Rollout Retargeting: Knopf und Aktion "Animation importieren (FBX, glTF, BVH, DAE)..." (Menue Animation >
  Retargeting, Rechtsklick Rig > Retargeting, Suche X/F3, Tooltip); Mixamo auf einen Klick (Vorlage erkannt, zugeordnet,
  uebertragen, Vorschau spielt); Zeile mit Format/Achsen/Einheit/fps unter der Quelle; der Dateidialog "Quelle waehlen" nimmt
  die Endungen auch.
- Vorlagen dazu: "Unreal Mannequin" (Quaternius UAL), "100STYLE (BVH)".
- Preset-Bibliothek `data/presets` (eingebaut wie data/karte, ausgepackt nach data/presets): 17 Eintraege, 1 382 956 Bytes
  BVH + presets.txt + CREDITS.txt; Rollout Presets, Gruppe "Bibliothek": Kategorie (Stehen, Gehen, Laufen, Kampf, Sonstiges) ->
  Animation (Doppelklick legt auf) -> Auf Figur legen (Vorschau spielt) -> Platz -> Auf Platz plotten; Quelle und Lizenz je
  Eintrag (Tooltip und Zeile), Naht des Zyklus angezeigt. Werkzeug `tools/presetbau` (Zyklus suchen, auf der Stelle,
  Naht verteilen, offene Schleifen fortgesetzt), Aufruf in `scratchpad/ai/bibliothek_bauen.py`.
- Ueber-Fenster, THIRD-PARTY.md, data/presets/CREDITS.txt: Quaternius (CC0) und "The 100STYLE Dataset - Ian Mason" (CC BY 4.0).
- Handbuchseite `modell-animimport` (6 Sprachen, Text; kein eigenes Bild).
- Tests: ctest `modell_animimport`, Selbsttest-Gruppe `ai` in modellblock.

## 2. Rot -> gruen

ctest `modell_animimport` (Testdateien aus einer eigenen Blender-4.5-Szene, `tests/animimport/blender_testgang.py`, Blender per
Popen mit eigenen Benutzerordnern; Referenz = Blenders Weltorte der Knochenkoepfe je Bild):
- FBX: 23 Knochen statt 22 (das Objekt Armature als Wurzel - gewollt, die Pruefung zaehlt es jetzt mit).
- glTF und COLLADA: 8,94 cm daneben (rot) - Blender schreibt die Zeiten ab frame_start (Bild 1 = 1/30 s), gelesen wurde ab 0
  -> Start beim ersten Schluessel -> 0,0001 cm. glTF dann 30 statt 31 Bilder (float-Zeit 999,99997 ms) -> Rundung 1e-3 -> 31.
- COLLADA: "mixamorig:Hips fehlt" - assimp nimmt die id (Armature_mixamorig_Hips) -> USE_COLLADA_NAMES -> Namen da.
- BVH: 27 statt 22 Knochen (End Sites, von assimp auch als Netz-Knochen eingetragen) -> 22.
- Rundlauf glb -> BVH -> lesen: 0,0365 Grad - die Messung (acos der Spur nahe 0 Grad) war es, nicht der Schreiber -> Winkel ueber
  atan2 -> 0,0001 Grad; ZXY-Zerlegung im Schreiber ebenso ueber atan2.
- Ergebnis: alle vier Formate Lage aller 22 Knochen in allen 31 Bildern gegen Blender 0,0001 cm, 30 fps, vorn -Y/links +X/oben +Z,
  Vorlage HumanIK/Mixamo; BVH-Wurzel Bild 10 gegen assimp-Schluessel 0,0000; Wurzelweg 1200 cm unkomprimiert erhalten;
  Retargeting Mixamo-Namen -> JKA-Skelett (Hand-Tabelle): 9 Ketten, 20 Knochen mit Quelle, k 0,3242, Fussrutschen 0,184
  (Quelle 0,038, Grenze 0,5 wie die Kontaktregel).
- Bibliothek (17 Eintraege): Schleifen der Quelle griffen in die naechste Animation der Datei (Naht 315-660 cm) -> Zyklus hinter
  alle Bilder kopiert, offene Schleifen mit dem letzten Schritt fortgesetzt -> Naht hoechstens 0,7 x der groesste Schritt.
  100STYLE ohne Vorlage nur 7 Ketten -> Vorlage "100STYLE (BVH)" -> 9 Ketten.
- Boden (neu, Knoechel ltalus/rtalus gegen die Bindepose): 100STYLE-Gaenge schwebten 31,8-32,4 Einheiten (rot; auch im
  Handbuchbild zu sehen: nur die Fuesse oben im Bild) - BVH-Grundstellung ohne Lage (ROOT OFFSET 0, Becken in den Bildern 90 cm
  hoch) -> Grundstellung auf Wurzel des ersten Bilds und tiefsten Stand der Bilder -> -0,24 bis -0,92; alle anderen -1,45.
- ctest `eingebaute_daten` rot (57 eingebaute Dateien, Liste kannte data/presets nicht) -> Test kennt data/presets -> gruen.

Selbsttest `ai` (modellblock, Sturmtruppler):
- `ai_ai1` 27/3: COLLADA mit Armature (wie FBX), Platz BOTH_STAND1 leer (Ziel war die Karten-GLA - dort gibt es nur BOTH_CIN_*;
  der Test stellt jetzt _humanoid.gla ein), ImGui-Kennung "Slot" doppelt (Platz/Backen wie im Rollout Retargeting -> PushID).
- `ai_ai2` 29/0, `ai_ai3` (nach BVH-Boden und Handbuch-Schritten) siehe Schluss.

Zusatz 5 (`rt` ohne `BHED_RT_GLA`):
- `ai_rtrot`: exit 3221225477 (0xC0000005) nach "Kette arm_l von Hand auf arm_r" (Index -1 ohne Quelle), davor 8 FEHLER-Zeilen.
- `ai_rtgruen`: exit 0, eine Meldung "BHED_RT_GLA fehlt oder ist kein Ordner - Gruppe nicht gelaufen" (gilt fuer rt, rtjka,
  rtspiel, rtzeit und die Handbuchbilder BHED_HB_NUR=rt). Gegenprobe `ai_rt` mit BHED_RT_GLA: 19/0 wie Stufe 5.

## 3. Im Spiel (Movie Duels SP, eigener fs_homepath, Spielordner vorher/nachher gleich)

- Eigene Figur: das Skelett aus testgang.fbx (Wurzel model_root, 23 Knochen), auf _humanoid-Groesse (k 0,3242 = Beinlaenge-Regel des
  Retargetings), GLA mit dem Gang (rtBacken, gepackt gegen gerechnet 0,017), GLM mit eigener Haut (je Knochen ein Kasten, 504 Ecken,
  eigene Textur und Shader), animation.cfg (1671 Zeilen, alle auf den Gang), NPC `aitest` - erzeugt vom ctest
  (`modellanimimporttest <tests> <presets> <ziel>`), gestartet mit `scratchpad/ai/figur_spiel.py` auf der Testkarte raum_licht.
- `ai_spiel1`/`ai_spiel3`: das Spiel laedt models/players/aitest/model.glm, aitest.gla und animation.cfg (qconsole), `npc spawn
  aitest` steht im Raum, die Figur bewegt sich (Bildunterschiede im Bereich der Arme in 10 von 11 Fotos je 0,25 s), Fotos 1920x1440
  in `scratchpad/ai/laeufe/ai_spiel3`.
- Offen: Paarbild gegen die Vorschau von JK Studio (die eigene .glm liegt nicht in den Spieldaten; JK Studio oeffnet Modelle nur von
  dort) und Seitenansicht im Spiel (die Kamera folgt dem Spieler, npc spawn setzt die Figur vor ihn).
- Preset auf _humanoid ueber Karten-GLA im Spiel: nicht neu gemessen - es ist derselbe Weg wie Stufe 5/7 (rtBacken ->
  Karten-GLA), dort im Spiel bestaetigt.

## 4. Schluss (abgebrochen auf Anweisung, Nutzungslimit)

- Zuletzt gruen: Bau 0 Warnungen (bau9), ctest 232/232 nach Behebung eingebaute_daten (ctest1 231/232, danach eingebaute_daten
  und modell_animimport gruen), Modus 1 `ai_m1` 2635/0, modellblock ai `ai_ai3` 29/0, rt `ai_rt` 19/0, rtjka 1/0, funktionen 60/0,
  Lints wie Basis (portability 99, pruefcodes 11, efxflags/fremdprogramm unveraendert).
- pr `ai_pr` 14/1: die Pruefung zaehlte 3 Aktionen der Gruppe Presets, jetzt sind es 4 (Preset-Bibliothek oeffnen) - auf 4
  gestellt, nicht mehr gebaut/gelaufen.
- Offen: letzter Bau (bau10: Rollen zur Gruppe Bibliothek fuers Handbuchbild, PushID) abgebrochen; Handbuchbilder
  h_ai_bibliothek (ai_hb2) zeigen die Figur, aber nicht die Gruppe Bibliothek und den Kopf abgeschnitten - neu aufnehmen;
  gen_handbuch mit dem neuen Bild; Paarbild Spiel gegen Vorschau; Bildzeit-Messung (nach dem Fenster allein).


## 5. Nachtest ZUS (08.11.)

- Bau 0 Warnungen; pr `zs_ai_pr` 15/0 (Erwartung 4 Aktionen der Gruppe Presets).
- Handbuchbild h_ai_bibliothek in 6 Sprachen neu (`zs_hbai_*`, 2538 x 1472, angesehen): Gruppe Bibliothek und die ganze Figur im Bild.
  Im japanischen Bild stand die Beschriftung "Slot" (スロット) des Platzfelds hinter dem rechten Rand: die Zeile pruefte nur die Breite
  der Beschriftung, nicht Feld + Beschriftung -> jetzt beides (`app_modelltab_retarget.cpp`), Bilder neu (`zs_hb2ai_*`, ja angesehen:
  Feld und Beschriftung in eigener Zeile); pr danach `zs_ai_pr2` 15/0.
- Offen wie oben: Paarbild Spiel gegen Vorschau, Bildzeit (Schlussrunde ZUS).

## Changelog

### English
- Model tab: import animations from other programs - FBX (e.g. a Mixamo file you downloaded yourself), glTF/GLB, BVH motion capture and COLLADA. Units, axes and frame rate come from the file; known skeletons (Mixamo/HumanIK, Biped, CAT, Unreal Mannequin, 100STYLE) are mapped at once, so a Mixamo walk is on your figure with one click and can be baked into any slot.
- Free preset library in the rollout Presets: idle, walk, run, combat and other animations from Quaternius (Universal Animation Library, CC0) and from "The 100STYLE Dataset - Ian Mason" (CC BY 4.0, motion capture walks) - pick a category and an animation, put it on your figure with a live preview, choose the slot and bake; loops are seamless cycles in place, source and licence are shown for every entry (also in the About window and data/presets/CREDITS.txt).

### Deutsch
- Model-Tab: Animationen aus anderen Programmen importieren - FBX (z. B. eine selbst geladene Mixamo-Datei), glTF/GLB, BVH-Motion-Capture und COLLADA. Einheiten, Achsen und Bildrate kommen aus der Datei; bekannte Skelette (Mixamo/HumanIK, Biped, CAT, Unreal Mannequin, 100STYLE) werden sofort zugeordnet, ein Mixamo-Gang ist so mit einem Klick auf deiner Figur und lässt sich auf jeden Platz backen.
- Freie Preset-Bibliothek im Rollout Presets: Stehen, Gehen, Laufen, Kampf und mehr von Quaternius (Universal Animation Library, CC0) und aus „The 100STYLE Dataset - Ian Mason“ (CC BY 4.0, Motion-Capture-Gänge) - Kategorie und Animation wählen, mit Live-Vorschau auf die Figur legen, Platz wählen und backen; Schleifen sind nahtlose Zyklen auf der Stelle, Quelle und Lizenz stehen bei jedem Eintrag (auch im Über-Fenster und in data/presets/CREDITS.txt).

### 中文
- 模型选项卡: 从其他程序导入动画 - FBX (例如你自己下载的 Mixamo 文件)、glTF/GLB、BVH 动作捕捉和 COLLADA。单位、轴向和帧率取自文件; 已知骨架 (Mixamo/HumanIK、Biped、CAT、Unreal Mannequin、100STYLE) 立即映射, 因此 Mixamo 步行一键即可放到角色上, 并可烘焙到任意槽位。
- 预设卷展栏中的免费预设库: 来自 Quaternius (Universal Animation Library, CC0) 和 "The 100STYLE Dataset - Ian Mason" (CC BY 4.0, 动作捕捉步行) 的站立、步行、跑步、战斗等动画 - 选择类别和动画, 实时预览放到角色上, 选择槽位并烘焙; 循环为原地无缝周期, 每个条目都显示来源和许可 (也在关于窗口和 data/presets/CREDITS.txt 中)。

### 日本語
- モデルタブ: 他のプログラムのアニメーションを読み込めます - FBX (自分でダウンロードした Mixamo のファイルなど)、glTF/GLB、BVH モーションキャプチャ、COLLADA。単位・軸・フレームレートはファイルから取り、既知の骨格 (Mixamo/HumanIK、Biped、CAT、Unreal Mannequin、100STYLE) はすぐ対応付けるので、Mixamo の歩きがワンクリックでフィギュアに載り、任意のスロットへベイクできます。
- ロールアウト「プリセット」に無料のプリセットライブラリ: Quaternius (Universal Animation Library、CC0) と「The 100STYLE Dataset - Ian Mason」(CC BY 4.0、モーションキャプチャの歩き) の待機・歩き・走り・戦闘など - カテゴリとアニメーションを選び、ライブプレビューでフィギュアに載せ、スロットを選んでベイク。ループはその場の継ぎ目のないサイクルで、各項目に出典とライセンスを表示します (バージョン情報と data/presets/CREDITS.txt にも)。

### Español
- Pestaña Model: importar animaciones de otros programas: FBX (p. ej. un archivo de Mixamo que hayas descargado tú), glTF/GLB, captura de movimiento BVH y COLLADA. Unidades, ejes y fotogramas salen del archivo; los esqueletos conocidos (Mixamo/HumanIK, Biped, CAT, Unreal Mannequin, 100STYLE) se asignan al instante, así un paso de Mixamo está en tu figura con un clic y se puede hornear en cualquier hueco.
- Biblioteca de presets libre en el panel Presets: reposo, andar, correr, combate y más de Quaternius (Universal Animation Library, CC0) y de «The 100STYLE Dataset - Ian Mason» (CC BY 4.0, pasos de captura de movimiento): elige categoría y animación, ponla en tu figura con vista previa en vivo, elige el hueco y hornea; los bucles son ciclos sin costura en el sitio y cada entrada muestra origen y licencia (también en la ventana Acerca de y en data/presets/CREDITS.txt).

### Čeština
- Karta Model: import animací z jiných programů - FBX (např. sami stažený soubor Mixamo), glTF/GLB, BVH motion capture a COLLADA. Jednotky, osy a snímková frekvence jsou ze souboru; známé kostry (Mixamo/HumanIK, Biped, CAT, Unreal Mannequin, 100STYLE) se přiřadí hned, takže chůze z Mixama je na postavě jedním klikem a lze ji zapéct do libovolné pozice.
- Volná knihovna předvoleb v rolloutu Předvolby: stání, chůze, běh, boj a další od Quaterniuse (Universal Animation Library, CC0) a z „The 100STYLE Dataset - Ian Mason“ (CC BY 4.0, chůze z motion capture) - zvolte kategorii a animaci, položte ji na postavu s živým náhledem, zvolte pozici a zapečte; smyčky jsou plynulé cykly na místě, u každé položky je zdroj a licence (také v okně O programu a v data/presets/CREDITS.txt).
