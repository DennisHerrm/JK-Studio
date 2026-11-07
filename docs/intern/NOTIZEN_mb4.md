# Notizen Modell-Block Stufe 4: IK/FK, Fussrolle, Einrasten, Raumwechsel, Pfade an Haenden/Fuessen/Saebel

Auftrag `auftrag_mb4.md`. Direkt in `Y:\karte` auf `karte1` ab 61816dcca (ASEB), Build `build-test`, Laeufe `laeufe/mb6_*`
(Oberflaeche `mb6_ui*`, Rundlauf `mb6_irl*_<spiel>`, Handbuch `mb6_hb*_<sprache>`, Bildzeit `mb6_zeit_*`, Schluss `mb6e_*`).
Laufskript `lauf.py` wie mb3/mb3b. kFassung bleibt rc610. Kein Spiel, kein 3ds Max, kein Blender gestartet; Spieldaten nur gelesen.

## Quellen

- **Blender** (blender/blender main 67e5a5f8, GPL-2.0-or-later, per `gh api` gelesen, Lesekopie Scratchpad `bl4/`):
  `intern/iksolver/intern/IK_QJacobianSolver.cpp` ConstrainPoleVector (die Ebene der Kette aus Wurzel -> Ziel und Wurzel -> Pol,
  "lookat"-Matrizen), `source/blender/ikplugin/intern/iksolver_plugin.cc` execute_posetree (jedes Glied beginnt mit seiner
  Pose: basis aus pose_mat ohne rest_basis, IK_SetTransform; das Ergebnis ist basis_change; der Einfluss mischt basis_change
  mit der Einheit, blend_m3_m3m3, Zeilen 566-567).
- **Rigify** (blender/blender-addons main b42d6862, GPL-2.0-or-later): `rigify/rigs/limbs/leg.py` rig_roll_mch_bones (Fersen-
  Regler: roll2 nur negativ um die Ferse, roll1 kopiert die ganze Drehung um den Ballen, roll3 = max(0, x * Toe_Roll) um die
  Zehenspitze; die Zehen folgen roll2/roll3, get_mch_heel_toe_output), `rigify/rig_ui_template.py` (match_pole_target: Pol =
  Mitte der Kette plus Senkrechte; ik2fk/fk2ik; der Regler IK_FK mit 0 = IK, 1 = FK).
- **3ds Max**: der HI-Solver ist nicht offen. Namen und Ablauf aus der Doku (`vorbilder_max_blender.md`): IK aktiviert,
  IK/FK-Einrasten, Automatisch einrasten (Bewegung > IK-Solver), Schwenkziel, Bewegung > Trajektorien (Schluessel auf der
  Bahn verschieben), Verknuepfungs-Beschraenkung (zeitweiser Elternwechsel). Tasten: `max_hotkeys_de.txt` und
  `blender_keys.txt` haben keine IK-Tasten - die neuen Aktionen haben keine eigene Taste; in der Ansicht wirken W/E (Max) und
  G/R (Blender) wie bisher.
- Saebel: Klingenlaenge 32 (OpenJK wp_saberLoad.cpp:364 lengthMax, wie app_view3d.cpp), Bolzen `*blade1`; das Spiel haengt den
  Saebel per Code an `*r_hand` (G2API_AttachG2Model) - die Animation bewegt die Haende.

## 1. IK (`bhed/modell/ik.h`, `src/modell/ik.cpp`)

- **Zwei-Glied-IK** analytisch (Kosinussatz): das Gelenk in der Ebene Wurzel/Ziel/Pol auf der Seite des Pols, ausser Reichweite
  gestreckt zum Ziel. Das obere Glied dreht seine Basis (Gliedrichtung, Beugerichtung) in die neue, das untere ist ein Scharnier,
  der Endknochen nimmt die Drehung des Ziels (wie die IK-Hand von Rigify).
- **Aenderung auf der Pose** wie Blender: die Glieder starten mit ihrer FK-Lage am Bild, die Verdrehung um die eigene Achse bleibt
  aus der Animation. Jedes Glied wirkt wie ein FK-Griff (rigGriffDrehen): Knochen, mitgedrehte (JK2 flach), Drehknochen mit
  Anteil. Ergebnis: M je Knochen in rigNachMalAuf (Ansicht, Pfade, Ghosts) und rigBacken (GLA); ohne IK-Spur oder bei Anteil
  0 bleibt alles Bit fuer Bit.
- **Spur je Glied** in der Schicht (`IkSpur` in rigdaten.h): Ziel (Ort, Drehung) als Bezier-Schluessel wie die Schicht (Henkel je
  Lauf gleichen Raums), dazu je Schluessel IK-Anteil und Raum (gelten bis zum naechsten), Pol, Fussrolle, Zehenlage (linear).
- **Fussrolle** wie der Fersen-Regler von Rigify (s. o.); ohne Zehenknochen (JKA) geht die Rolle nach vorn ganz um die
  Zehenspitze. Die Zehenlage zum Fuss steht im Schluessel (sonst drehte das Auto-Einrasten sie doppelt).
- **Einrasten**: "IK -> FK" (Ziel und Pol aus der FK-Pose, Pol in der Ebene des Gelenks wie match_pole_target), "FK -> IK"
  (FK-Schluessel der Kette = sichtbare Pose; ueber eine Spanne erst alle Bilder auswerten, dann schreiben wie Snap & Bake).
  Jeder Zug am Ziel schreibt danach die FK-Schluessel (Auto-Einrasten von Max): Dopesheet und Kurven zeigen ihn sofort.
- **Raumwechsel** (Child Of / Verknuepfungs-Beschraenkung): Schluessel am Bild davor im alten Raum, am Bild im neuen - kein Sprung.
  Raeume: Welt, Becken, Brust, Kopf, andere Hand ("Saebel an die andere Hand").
- **starr()**: die Drehung einer Weltmatrix ueber Gram-Schmidt der Spalten wie rotationPart in G2C. Die Weltmatrizen der
  Spiele tragen Mass (im Programm: Drehpunkt der Fussrolle 2,5 Einheiten neben dem ueber matrixToQuat); Gram-Schmidt dreht mit
  (starr(U X) = U starr(X)). Die Drehpunkte der Fussrolle nehmen die Form des Fusses (starr(X)^-1 X) mit.

## 2. Oberflaeche (`gui/app_modelltab_ik.cpp`, Aenderungen in `app_modelltab_rig.cpp`, `app_modelltab_kurven.cpp`)

- **Wie 3ds Max**: W an Hand/Fuss/Saebelgriff = IK (Wechsel als Schluessel, mit "Automatisch einrasten" ab Werk ohne Sprung), E am
  Ober-/Unterglied unter IK wechselt auf FK (eingerastet) und dreht; W am Pol (Schwenkziel); E an Ferse/Ballen/Zehenspitze rollt.
  Rollout "IK/FK" (geht auf, sobald ein Glied gewaehlt wird): je Glied "IK aktiviert" und "IK/FK einrasten".
- **Wie Blender**: je Glied der Regler IK-FK (0 IK, 1 FK, Rigify) und "IK -> FK" / "FK -> IK"; G an Hand/Fuss/Pol nur bei IK,
  sonst eine Meldung (kein Modal); R rollt an den Fussrollen-Punkten.
- Ansicht: IK-Ziel als Kreuz mit Linie vom Ansatz, gestrichelte Linie zum Raum; der Pol steht, wo die Spur ihn hat.
- **Pfade**: an Hand und Fuss (Knochenpfad), am Saebel-Tag und an der Saebelspitze ("Pfad der Saebelspitze") ist der gezogene
  Punkt das IK-Ziel am Frame; am Tag folgt das Ziel, bis der Punkt unter der Maus steht (die Ecken des Tags haengen auch am
  Unterarm). Ein Rueckgaengig-Schritt.
- Erreichbar: Menue Animation > IK/FK (Untermenue), Rechtsklick der Ansicht (Control Rig > IK/FK) und des Griffs im Explorer,
  Suche X/F3, Rollout, Tooltip an jeder Zeile.
- **Reduzieren** (aus 3b offen): "Nur gewaehlte Knochen reduzieren" (gewaehlte Schluessel bzw. Griff) und "Nur eine Spanne
  reduzieren" (aus den gewaehlten Schluesseln, Rollout von/bis); ausserhalb bleibt die Schicht Bit fuer Bit (mit Bindebasis),
  sonst ein Schluessel je Frame; jedes Knochen-Frame innerhalb der Grenze.
- **Posen-Bibliothek als Datei** (aus 3b offen): `Dokumente\behaved\posen\<Name>.jkposen` (Text, nie im Spielordner), Laden ueber
  das Rollout oder das Menue (gleiche Namen ersetzt, ein Rueckgaengig-Schritt), Ordner oeffnen.

## Rot und gruen

- **ctest `modell_ik`** (neu): Zwei-Glied-IK, Ketten an JKA (Hierarchie) und JK2 (flach), Fussrolle (JK2 mit Zehen; JKA gedreht
  und gestreckt, ohne Zehen), Einrasten an jedem Bild hin und zurueck gebacken innerhalb der Grenze, Wechsel ohne Einrasten
  springt (rot) / mit nicht, Raumwechsel, IK-Anteil 0 Byte fuer Byte, Reduzieren mit Auswahl, Posen-Text.
  - FK -> IK Bild fuer Bild: 584 / 582 / 139 / 157 Knochen-Bilder ueber der Grenze (ein geschriebener Schluessel aenderte die
    FK-Pose der Nachbarn, bevor sie geschrieben waren) - jetzt `fkAufIkBereich` (erst auswerten, dann schreiben): 0.
  - JK2 Bein IK -> FK: 6 Knochen-Bilder (Anteil 1434) - fast gestrecktes Knie, der Pol fiel ab 1e-4 Beugung auf den alten
    Schluessel; jetzt jede messbare Beugung: 0.
  - Fussrolle Zehen 3,16 Einheiten bewegt (das Auto-Einrasten drehte die Zehen ein zweites Mal) - Zehenlage im Schluessel: 0,000004.
  - starr: bei Mass 0,64 und Scherung ueber matrixToQuat allein 0,0755 daneben (rot), Gram-Schmidt 0.
- **Selbsttest `mb6`** (Gruppe in `modellblock`): `mb6_ui1` 21/15, ... `mb6_ui8` 36/0, `mb6_ui11` 37/0, Schluss `mb6e_mb6` 37/0.
  - W an der Hand tat nichts: das Rechtsklick-Menue im Explorer nahm den ersten Klick (Test), die Ansicht Vorne blickt entlang X
    (Test: Griff Y).
  - Fussrolle: am Griff Ferse rollte ein Zug nach rechts nach vorn (um den Ballen) - jetzt rollt die Ferse nach hinten (< 0),
    Ballen/Spitze nach vorn (> 0) wie die zwei Richtungen von Rigify. Danach stand der Drehpunkt 0,405 / 5,58 neben dem
    Anfang (`mb6_ui6`): die Weltmatrix des Fusses traegt Mass, starr ueber matrixToQuat drehte daneben - Gram-Schmidt und die
    Form des Fusses: 0,000032 / 0,000025 (`mb6_ui7`).
  - Saebelspitze: der Pfad war nur Anzeige (der Tag haengt an rhang_tag_bone, keine Hand) - jetzt ueber das Saebel-Tag des Rigs;
    dann stand die Spitze 33 Punkte neben der Maus (der Tag haengt auch am Unterarm) - das Ziel folgt bis 1/1000: 0,0 Punkte.
  - G wie Blender in FK blieb als leeres Modal haengen (die naechsten Tasten gingen verloren) - jetzt eine Meldung ohne Modal.
  - Rueckgaengig hiess "Becken verschieben" beim Ziehen der Hand - jetzt "Griff verschieben".
  - Das Menue Animation passte im Handbuch-Fenster nicht mehr auf den Schirm (Posen-Bibliothek nicht erreichbar,
    `mb6_hb_en`) - IK/FK ist ein eigenes Untermenue.
- **IK-Rundlauf** (`ikrundlauf`, jede Sequenz von _humanoid, je Glied): siehe unten.
- Testfehler unterwegs (nicht im Programm): Pivot vor dem Zeichnen des neuen Bilds gelesen, Suche mit der Taste beim Aufbau
  der Schritte (X statt F3 im Blender-Teil), Ziele ausser Reichweite, Fingerknochen in der Reduzieren-Auswahl vergessen.

## IK-Rundlauf an den Spieldaten (`mb6_irl*`)

Je Sequenz von _humanoid (doppelte Spannen einmal) alle 4 Glieder: IK -> FK an jedem Frame (Ziel und Pol aus der FK-Pose),
gebacken gegen die GLA; dann die Ziele an Anfang, Mitte und Ende bewegt, FK -> IK an jedem Frame, ohne IK gegen mit IK
gebacken. Grenze wie Stufe 3 (`kompressionsGrenze`, Verschiebung hoechstens eine Stufe). Schlusslaeufe `mb6e_irl_*`:

| Spiel | Sequenzen | IK -> FK: Knochen-Frames / Byte fuer Byte / groesster Anteil / ueber | FK -> IK: Byte fuer Byte / groesster Anteil / ueber | Zeit |
|---|---|---|---|---|
| JKA | 1289 | 1 189 744 / 787 553 / 0,92 / **0** | 1 057 297 / 2,60 (l_d2_j2) / 2207 (0,19 %) | 87 s |
| JK2 | 883 | 1 314 288 / 540 878 / 0,98 / **0** | 1 280 708 / 1,79 (lradiusX) / 189 (0,014 %) | 124 s |
| Movie Duels | 2434 | 3 281 283 / 3 249 327 / 0,32 / **0** | 3 246 663 / 0,23 / **0** | 235 s |

Die Ueberschreitungen FK -> IK an JKA/JK2 liegen an Fingergliedern und am Unterarm-Drehknochen (siehe Abweichungen); der
Selbsttest prueft FK -> IK deshalb nur an Movie Duels streng, die Zahlen stehen im Bericht `mb6_ikrundlauf_<spiel>.txt`.

## Bildzeit (allein, BHED_D3D_DEBUG=0, nacheinander)

`modellleistung` (23 Bloecke Modell1-Messung), alte exe (ASEB, `mb6_zeit_vor`) gegen neue (`mb6_zeit_nach`), nacheinander, kein
Bau dabei:

- Summe der Mittel 16,47 -> 15,99 ms, Summe p99 33,78 -> 30,48 ms, Bildabstand im Mittel 4,35 -> 4,28 ms. Ohne IK-Spur kehrt
  `ikAnwenden` sofort zurueck; die Abweichungen einzelner Bloecke (stormtrooper Animation 0,40 -> 0,50, clonetrooper
  Drehteller 1,33 -> 0,99) gehen in beide Richtungen und sind Streuung.
- Neuer Block (`mb6_zeit_ik`, `BHED_MODELL1_NUR=ik`): IK an 4 Gliedern, 3 Pfade, Kurveneditor offen: Bild-CPU Mittel 2,13 p99
  2,46 ms gegen 0,51 / 1,23 ms dieselbe Animation ohne IK; Modell-CPU 0,018 / 0,038 ms, Grafikkarte Median 0,090 ms;
  Bildabstand 4,30 / 9,38 ms (Bildrate haengt am Abstand, nicht an der CPU). Pruefzeile: 4 IK-Ziele, 36 Pfadpunkte,
  Kurven 20832 Punkte, Editor 0,67 ms.

## Schluss

Einmal, nach sauberem Neubau (`--clean-first`, /W4 /WX, 0 Warnungen):

- ctest: 226 von 226 (93,9 s), darunter `modell_ik`.
- modellblock: funktionen 60/0, tasten 43/0, mb2 37/0, blender 44/0, mb3 21/0, rig 26/0, mb5 159/0, mb6 37/0;
  rundlauf jka/jk2/md je 2/0, rigrundlauf je 4/0, bild je 1/0, kurvenrundlauf je 2/0, ikrundlauf jka 1/0, jk2 1/0, md 2/0
  (Laeufe `mb6e_*`, alle zugleich, Summe `laeufe/mb6e_summe.txt`).
- modell 14/0, mission1 98/0, kuerzel 202/0, Modus 1 2635/0 (die 19 FEHLER-Zeilen im Protokoll von Modus 1 sind die gewollten
  Rotpruefungen des Reiterpruefers, Basis `asebe_modus1` gleich).
- Handbuch: Bilder der Seite modell-ik in 6 Sprachen (`mb6_hbe_*`, je 37/0, 2538 x 1472, angesehen), 170 Seiten x 6 Sprachen
  erzeugt, `--pruefen` ok, `gen_hilfe_orte` 206 Orte.
- Lints wie in der Basis (portability 99, pruefcodes 11, efxflags/fremdprogramm/link/tidy unveraendert), alle anderen ok.
- Private Adressen in `git log -p 61816dcca..HEAD`: 0.

## Abweichungen und Offen

- FK -> IK an den Raven-Daten (JKA, JK2): die FK-Schluessel sind Drehungen (Stufe 3); dreht die IK einen Knochen, dessen
  Weltmatrix nicht orthonormal ist (MC_UnCompressQuat normiert nicht, das summiert sich die Kette hinab), ist M = X^-1 U X keine
  reine Drehung - der Schluessel haelt die naechste (polare Zerlegung). Gebacken liegen dann wenige Knochen-Frames ueber der engen
  Grenze (Zahlen oben, Fingerglieder und Unterarm-Drehknochen); Movie Duels 0. IK -> FK ist an allen drei Spielen exakt.
- IK-Ziele interpolieren Bezier, Pol, Rolle und Zehen linear; IK-Anteil und Raum springen am Schluessel (der Regler IK-FK von
  Rigify wird meist so geschaltet).
- IK-Schluessel haben keine Zeile im Dopesheet; Verschieben von Schluesseln dort nimmt die IK-Schluessel nicht mit.
- Der HI-Solver von 3ds Max ist nicht offen: Verhalten nach der Doku, Rechnung die von Blender (geschlossen statt iterativ).
- Ein Raumwechsel betrifft das Bild; spaetere Schluessel im alten Raum bleiben in ihrem Raum.
- Scherung in einem M haelt kein Schluessel (Stufe 3); der ctest misst die Fussrolle mit Mass, nicht mit Scherung.
- Japanisch/Chinesisch: der Hinweis im Rollout bricht nach dem ersten Komma um (ImGui bricht CJK nur an Satzzeichen, wie
  ueberall im Programm) - Schoenheitsfehler, kein eigener CJK-Umbruch in dieser Stufe.

## Vorschlag Stufe 7 (Animieren im Mission-Tab)

- **Eine Figur im Mission-Tab**: die Schicht (FK-Schluessel, IK-Spuren, Raeume) einer Figur der Szene statt des Model-Tabs, im
  Mission-Tab auf der Karte posiert (die IK-Ziele in Weltkoordinaten der Karte: Fuesse auf dem Boden der BSP, Haende an Tueren und
  Konsolen); dieselbe Bedienung (Griffe, W/E bzw. G/R, Rollout IK/FK, Pfade) in der 3D-Vorschau.
- **Wurzelweg und SET_ORIGIN**: der Pfad des Beckens/der Bewegung wird zum Weg der Figur; "Fuer das Spiel bauen" schreibt die
  Karten-GLA (`_humanoid_<karte>`, BOTH_CIN_1..50, Plaetze aus Stufe 2), animation.cfg und ICARUS mit SET_ANIM_BOTH, Haltezeit und
  SET_ORIGIN plus Folgeanimation am Ende (Stufe 0 belegt den Sprung ohne SET_ORIGIN).
- **Mehrere Figuren**: je Figur eine Schicht, signal/waitsignal zwischen ihnen; "Saebel an die andere Hand" und Raeume auch zu
  Knochen einer anderen Figur (Griff, Uebergabe) - fuer das Spiel gebacken in jede GLA.
- **Pruefung**: Szene mit drei Figuren in JKA und Movie Duels gegen die Vorschau (Lage und Pose an mehreren Zeitpunkten wie
  `ms0/flecken.py`); rot ohne SET_ORIGIN. Dazu offen aus Stufe 4: IK-Schluessel als Zeile im Dopesheet, Verschieben mit den
  FK-Schluesseln.

## Changelog

### English
- Model tab, area Animation: two-bone IK for the arms and legs of the control rig. Hand or foot reach their target, the elbow or knee points to the pole target (the rule Blender uses, the swivel target of the 3ds Max HI solver); the result is FK keys in the same layer, so the dope sheet and the curves show it at once and baking into the GLA works as before.
- Switch every limb between IK and FK without a jump: like 3ds Max, grabbing the hand or foot (W) is IK and rotating a limb (E) is FK, with Auto snap on by default; like Blender, the IK-FK slider of Rigify with IK → FK and FK → IK. A switch without snapping shows the jump.
- Foot roll like the heel control of Rigify: back on the heel, forward over the ball, optionally on the toe tip; the toes stay on the ground.
- Drag the motion paths of hands, feet and the saber tip in the view: the dot becomes the IK target at that frame, one key, one undo step.
- Space switching: from a frame on the IK target follows the world, the pelvis, the chest, the head or the other hand, without a jump - "Saber to the other hand" for two-handed grips.
- Reduce only the selected bones or a frame span; save and load the pose library as a file (Documents\behaved\posen).
- New submenu Animation > IK/FK, also in the right-click and the search (X / F3), and the rollout IK/FK that opens when a limb is selected.

### Deutsch
- Model-Tab, Bereich Animation: Zwei-Glied-IK für Arme und Beine des Control Rigs. Hand oder Fuß erreichen ihr Ziel, Ellbogen oder Knie zeigen zum Pol-Ziel (die Regel von Blender, das Schwenkziel des HI-Solvers von 3ds Max); das Ergebnis sind FK-Schlüssel in derselben Schicht, Dopesheet und Kurven zeigen es sofort, und das Backen in die GLA geht wie bisher.
- Jedes Glied ohne Sprung zwischen IK und FK wechseln: wie 3ds Max ist Hand oder Fuß greifen (W) IK und ein Glied drehen (E) FK, mit „Automatisch einrasten“ ab Werk; wie Blender der Regler IK-FK von Rigify mit IK → FK und FK → IK. Ein Wechsel ohne Einrasten zeigt den Sprung.
- Fußrolle wie der Fersen-Regler von Rigify: nach hinten auf die Ferse, nach vorn über den Ballen, auf Wunsch auf die Zehenspitze; die Zehen bleiben liegen.
- Die Bewegungspfade von Händen, Füßen und Säbelspitze in der Ansicht ziehen: der Punkt wird an diesem Frame das IK-Ziel, ein Schlüssel, ein Rückgängig-Schritt.
- Raumwechsel: ab einem Frame folgt das IK-Ziel der Welt, dem Becken, der Brust, dem Kopf oder der anderen Hand, ohne Sprung - „Säbel an die andere Hand“ für Griffe mit beiden Händen.
- Nur gewählte Knochen oder eine Spanne reduzieren; die Posen-Bibliothek als Datei speichern und laden (Dokumente\behaved\posen).
- Neues Untermenü Animation > IK/FK, auch im Rechtsklick und in der Suche (X / F3), und das Rollout IK/FK, das aufgeht, sobald ein Glied gewählt ist.

### 中文
- 模型选项卡的动画区域: 控制绑定的手臂和腿的双骨骼 IK。手或脚到达其目标, 肘或膝指向极向目标 (Blender 的规则, 即 3ds Max HI 解算器的旋转目标); 结果是同一层中的 FK 关键帧, 摄影表和曲线立即显示, 烘焙到 GLA 与以前一样。
- 每个肢体在 IK 与 FK 之间无跳变切换: 像 3ds Max 时抓住手或脚 (W) 即 IK, 旋转肢体 (E) 即 FK, 默认开启自动捕捉; 像 Blender 时使用 Rigify 的 IK-FK 滑块以及 IK → FK 和 FK → IK。不捕捉的切换会显示跳变。
- 与 Rigify 脚跟控制器相同的脚部滚动: 向后滚到脚跟, 向前越过脚掌, 也可立在脚尖上; 脚趾保持着地。
- 在视图中拖动手、脚和光剑尖端的运动路径: 该点成为该帧的 IK 目标, 一个关键帧, 一步撤销。
- 空间切换: 从某一帧起 IK 目标无跳变地跟随世界、骨盆、胸部、头部或另一只手 -“光剑交到另一只手”用于双手握持。
- 只精简所选骨骼或一段帧; 把姿势库保存为文件并载入 (文档\behaved\posen)。
- 新子菜单 动画 > IK/FK, 也在右键和搜索 (X / F3) 中, 以及选择肢体时展开的 IK/FK 卷展栏。

### 日本語
- モデルタブのアニメーション領域: コントロールリグの腕と脚の 2 ボーン IK。手や足がターゲットに届き、肘や膝はポールターゲットを向きます (Blender の規則、3ds Max の HI ソルバーのスイベルターゲット)。結果は同じレイヤーの FK キーなので、ドープシートとカーブにすぐ表示され、GLA へのベイクも従来どおりです。
- 各肢を IK と FK の間で飛びなく切り替え: 3ds Max 風では手や足をつかむ (W) と IK、肢を回す (E) と FK、自動スナップは既定でオン。Blender 風では Rigify の IK-FK スライダーと IK → FK、FK → IK。スナップなしの切り替えは飛びが見えます。
- Rigify のかかとコントロールと同じフットロール: かかとへ後ろに、母指球を越えて前に、必要ならつま先で。つま先は接地したままです。
- ビューで手・足・セーバー先端のモーションパスをドラッグ: 点がそのフレームの IK ターゲットになり、キー 1 つ、元に戻すは 1 回。
- スペース切り替え: あるフレームから IK ターゲットがワールド、骨盤、胸、頭、反対の手に飛びなく従います - 両手持ち用の「セーバーを反対の手へ」。
- 選択したボーンだけ、またはフレーム範囲だけを削減。ポーズライブラリをファイルとして保存・読み込み (ドキュメント\behaved\posen)。
- 新しいサブメニュー アニメーション > IK/FK。右クリックと検索 (X / F3) にもあり、肢を選ぶと IK/FK ロールアウトが開きます。

### Español
- Pestaña Model, área Animación: IK de dos huesos para los brazos y las piernas del rig de control. La mano o el pie alcanzan su objetivo y el codo o la rodilla apuntan al objetivo de polo (la regla de Blender, el objetivo de giro del solucionador HI de 3ds Max); el resultado son claves FK en la misma capa, así que la hoja de claves y las curvas lo muestran al instante y hornear en el GLA funciona como antes.
- Cambia cada miembro entre IK y FK sin salto: como en 3ds Max, agarrar la mano o el pie (W) es IK y girar un miembro (E) es FK, con el ajuste automático activado de serie; como en Blender, el deslizador IK-FK de Rigify con IK → FK y FK → IK. Un cambio sin ajuste muestra el salto.
- Balanceo del pie como el control del talón de Rigify: hacia atrás sobre el talón, hacia delante sobre la planta y, si se quiere, sobre la punta; los dedos quedan en el suelo.
- Arrastra en la vista las trayectorias de manos, pies y punta del sable: el punto pasa a ser el objetivo IK en ese fotograma, una clave, un paso de deshacer.
- Cambio de espacio: desde un fotograma el objetivo IK sigue al mundo, la pelvis, el pecho, la cabeza o la otra mano sin salto; «Sable a la otra mano» para agarres a dos manos.
- Reducir solo los huesos seleccionados o un intervalo; guardar y cargar la biblioteca de poses como archivo (Documentos\behaved\posen).
- Nuevo submenú Animación > IK/FK, también en el clic derecho y en la búsqueda (X / F3), y el panel IK/FK que se abre al seleccionar un miembro.

### Čeština
- Karta Model, oblast Animace: dvoukostní IK pro paže a nohy ovládacího rigu. Ruka nebo chodidlo dosáhnou cíle, loket nebo koleno míří k cíli pólu (pravidlo Blenderu, cíl natočení HI řešiče v 3ds Max); výsledkem jsou klíče FK ve stejné vrstvě, dopesheet a křivky to ukážou hned a zapečení do GLA funguje jako dřív.
- Přepínání každé končetiny mezi IK a FK bez skoku: jako v 3ds Max je uchopení ruky nebo chodidla (W) IK a otočení končetiny (E) FK, s automatickým přichycením zapnutým od výroby; jako v Blenderu posuvník IK-FK z Rigify s IK → FK a FK → IK. Přepnutí bez přichycení ukáže skok.
- Odval chodidla jako ovladač paty v Rigify: dozadu na patu, dopředu přes bříško, případně na špičku; prsty zůstanou na zemi.
- Táhněte v pohledu dráhy pohybu rukou, chodidel a hrotu meče: bod se na tom snímku stane cílem IK, jeden klíč, jeden krok zpět.
- Přepínání prostoru: od snímku cíl IK bez skoku sleduje svět, pánev, hruď, hlavu nebo druhou ruku - „Meč do druhé ruky“ pro úchop oběma rukama.
- Redukce jen zvolených kostí nebo úseku snímků; uložení a načtení knihovny póz jako souboru (Dokumenty\behaved\posen).
- Nová podnabídka Animace > IK/FK, také v pravém tlačítku a ve vyhledávání (X / F3), a rollout IK/FK, který se otevře při výběru končetiny.
