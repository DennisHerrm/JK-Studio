# Notizen Modell-Block Stufe 3: Knochen-Erkennung, Control Rig mit einem Klick, Posen, Ghosts, Pfade, Backen

Auftrag `auftrag_mb3.md`. Direkt in `Y:\karte` auf `karte1` ab b51504a63 (Stufe 2), Build `build-test`, Laeufe `laeufe/mb4_*`
(Rig-Oberflaeche `mb4_rig*`, Rundlauf `mb4_rl*_<spiel>`, Handbuch `mb4_hb_<sprache>`, Bildzeit `mb4_zeit_*`, Schluss `mb4e_*`).
Laufskript `lauf.py` wie mb2. kFassung bleibt rc610. Kein Spiel, kein 3ds Max, kein Blender gestartet; die Spieldaten nur gelesen.

## 1. Knochen-Erkennung (`bhed/modell/rig.h`, `src/modell/rig.cpp`)

- Je Knochen eine Rolle (Wurzel, Bewegung, Becken, Wirbel, Hals, Kopf, Gesicht, Schluesselbein, Ober-/Unterarm mit
  Drehknochen, Hand, Finger, Ober-/Unterschenkel mit Drehknochen, Fuss, Zehen, Anhang, Tag-Knochen, Sonstiges) und eine Seite.
  Drei Quellen:
  - **Name**: die Muster der drei Spiele (pelvis, lhumerus/rhumerusX, r_d1_j1, mc7, L_Upper_Arm, leg_left_bone01, femur_3,
    R_calve, Skeleton_Root ...). Allgemeine Glieder (r_armbone, leg_*_bone0n) bekommen die freien Rollen ihrer Kette.
  - **Hierarchie**: der erste Knochen der Mitte ueber einem Seitenknochen ist der Ansatz. Je Glied (oberster Seitenknochen unter
    dem Ansatz) stimmen die Namen ab, ob es ein Bein oder ein Arm ist; ein Glied aus nur einem Knochen (JK2: alles flach unter
    pelvis/thoracic) nimmt die Abstimmung des Ansatzes; ohne Mehrheit traegt der am tiefsten reichende Ansatz die Beine.
  - **Lage** (Grundstellung, Spalte 3 von basePose): die Seitenachse ist die Achse, auf der sich die Namenspaare am weitesten
    unterscheiden (alle Skelette: X, links positiv - JKA lhumerus +5,92, rhumerus -5,92). Seite aus der Lage ab 3 % der
    Hoehe (gemessen: der seitlichste Mittelknochen leye bei 1,8 %, der innerste Seitenknochen lfemurYZ bei 5,7 %). Knochen in der
    Mitte (JKA lclavical/rclavical bei x = 0) nehmen die Lage der Ecken, die sie tragen (mit Modell).
  - Reihenfolge im Glied nur, wo sie belegt ist: Vorfahr vor Nachfahr; Geschwister unter einem Eltern, der keine Wurzel ist, nach
    dem Abstand (ab 3 % der Hoehe). Becken < Wirbel < Hals < Kopf entlang der Hierarchie.
- Widersprechen sich Name und Lage, gilt die Lage, der Knochen ist **unklar** (Grund: Seite, Reihenfolge, Arm/Bein).
- **Uebersicht** (Fenster "Rig-Uebersicht"): eine Figur zum Anklicken (eigene Zeichnung): gruen erkannt, gelb unklar, grau leer,
  Zahl bei mehreren Knochen; Tooltip mit Knochen und Grund; Fleck waehlen, dann Knochen in "Unklar und ohne Rolle" oder
  "Knochen zuordnen..." - eine Zuordnung von Hand im Modelldokument (Strg+Z), das Rig wird sofort neu angelegt.

### Trefferquote (Hand-Tabellen `tests/rig_handtabelle.inc`, je Name von Hand eingetragen)

| Skelett | Knochen | richtig (Rolle und Seite) | unklar |
|---|---|---|---|
| JKA `_humanoid` (GameData original) | 53 | 53 | 0 |
| Movie Duels `_humanoid` (60861 Bilder, gleiche 53 Knochen) | 53 | 53 | 0 |
| JK2 `_humanoid` (72, flach) | 72 | 72 | 0 |
| MD Droide `droideka` (59) | 59 | 59 | 0 |
| MD Kreatur `bantha` (15) | 15 | 15 | 0 |

Je Rolle (alle Tabellen zusammen, JKA zweimal: GameData original und Movie Duels) 100 %: Wurzel 5/5, Bewegung 3/3, Becken 5/5,
Wirbel 16/16, Hals 6/6, Kopf 6/6, Gesicht 27/27, Schluesselbein 8/8, Oberarm 8/8, Oberarm-Drehknochen 6/6, Unterarm 8/8,
Unterarm-Drehknochen 6/6, Hand 8/8, Finger 56/56, Oberschenkel 9/9, Oberschenkel-Drehknochen 6/6, Unterschenkel 11/11, Fuss
11/11, Zehen 2/2, Anhang 7/7, Tag-Knochen 5/5, Sonstiges 33/33 (Bericht `mb4_rig_<spiel>.txt`).

Alle Skelette der Spiele ohne Hand-Tabelle: JKA 66 GLA, JK2 21, Movie Duels 120. Unklar bleiben (Schluss-Rundlauf): hazardtrooper
`Left_thumb_end1` (Name links, liegt rechts), howler `L/R_Mid_Toe`, `L/R_Heel` (einzelne Knochen unter der Wurzel), Movie
Duels `attevm turret5_r` (Name rechts, liegt links), `custom_seat ik_hand.l/r` (IK-Ziele an den Fuessen). Genau dafuer ist die
Uebersicht da.

**Rot**: absichtlich vertauschte Namen (ctest `modell_rig`): lhumerus/rhumerus, rfemurYZ/rtalus, cranium/pelvis,
rhumerus/rfemurYZ, lhand/rtalus, Schluesselbeine (nur ueber die Ecken). Nur nach Namen: 6 FEHL; mit Hierarchie/Lage: alle nach
der Lage richtig und als unklar gemeldet (0 FEHL).

## 2. Control Rig mit einem Klick

- `rigAnlegen`: Griffe fuer Becken, Wirbel (je Wirbel), Hals, Kopf, Schluesselbein, Ober-/Unterarm, Hand, Ober-/Unterschenkel,
  Fuss, Zehen; Pol-Ziele fuer Ellbogen (hinten) und Knie (vorn, Richtung aus den Gesichtsknochen), eine Oberarm- bzw.
  Oberschenkellaenge weit; Fussrolle Ferse/Zehenspitze (hinterste/vorderste Ecke des Fusses) und Ballen (am Zehenknochen, wo es
  einen gibt); Saebelgriff am Tag `*r_hand`/`*l_hand`. Pol-Ziele und Fussrolle wirken erst mit IK (Stufe 4), der Tooltip sagt es.
- **FK** wie G2API_SetBoneAngles mit BONE_ANGLES_POSTMULT (tr_ghoul2.cpp:1800-1812): X(b) * M. Knochen, die zum Glied gehoeren,
  aber nicht darunter haengen, drehen mit (JKA: Finger und rhang_tag_bone unter rradius; JK2: der ganze Arm flach unter
  thoracic, 21 Knochen). Der Drehknochen (rradiusX, rhumerusX, rfemurX) nimmt den Anteil der Drehung um die Achse des Segments,
  der seiner Lage auf der Strecke entspricht (Grundstellung: 0,50). Gemessen in `_humanoid.gla`: der Twist von rradiusX folgt
  der Hand im Mittel mit 0,73 (rechts) / 0,63 (links), aber mit 33-38 Grad Streuung - in den Daten gibt es keine feste Regel,
  darum die Lage.
- **Schicht**: je Sequenz (Name der animation.cfg-Zeile) und Knochen Schluessel {Bild, q, t} im Modelldokument (Strg+Z); zwischen
  zwei Schluesseln slerp/lerp, davor/danach der Randschluessel; ausserhalb der Sequenz wirkt sie nicht. Rig-Aenderungen aendern
  `Inhalt::netzStand` nicht - die Anzeige baut das Netz nicht neu (Pruefzeile "Netz nicht neu gebaut").
- **Bedienung**: Griffe im Szenen-Explorer (Gruppe "Control Rig", eigene Symbole Rig/Griff/Pol, Auge), in der Ansicht wie 3ds
  Max als Kasten, wie Blender als Ring (links blau, rechts gruen, Mitte gelb); Klick waehlt. Max: E dreht mit dem Ring-Gizmo, W
  verschiebt das Becken; Blender: R dreht (X/Y/Z Achse, sonst Blickachse), G verschiebt das Becken, Enter/Klick uebernimmt,
  Esc/rechts bricht ab.

## 3. Posen, Auswahl, Ghosts, Silhouette, Pfade

- Pose kopieren / einfuegen / gespiegelt einfuegen (D'(b) = S D(Partner) S, D = Welt * Grundstellung^-1, S an der Seitenachse);
  auf Bindepose (Auswahl, ganze Figur); Handposen aus den Animationen des Spiels: offen = Bindepose, Faust = erstes Bild von
  BOTH_MELEE1, Saebelgriff = erstes Bild von BOTH_STAND1 (Fingerkruemmung gemessen: 110,8 / 104,4 Grad gegen die Bindepose).
- Auswahl Gegenseite / Eltern / Kind.
- Ghosts: Posen davor (blau) / danach (orange), Anzahl, Abstand, Farbe einstellbar; gezeichnet als eigene Durchgaenge (erst Tiefe,
  dann Farbe auf gleicher Tiefe, je Ghost jeder Bildpunkt einmal ueber den Stencil; alpha-getestete Flaechen behalten ihre Form).
- Silhouette: Figur schwarz auf hellem Grund.
- Bewegungspfade fuer Knochen oder Tags: ein Punkt je Bild, das aktuelle weiss, Schluessel als Quadrate, Bereich davor/danach.
- Erreichbar: Menue **Animation** in der Menueleiste des Model-Tabs, Rechtsklick der Ansicht (Untermenue Control Rig), Rechtsklick
  im Explorer, Suche X/F3, Befehlsfeld (Rollouts Control Rig, Pose, Ghosts, Bewegungspfade, Backen), Tooltip an jeder Zeile.
  Tasten: wie 3ds Max Bild auf/Bild ab (Eltern/Kind, `max_hotkeys_de.txt`); wie Blender Strg+C/Strg+V/Strg+Umschalt+V
  (pose.copy/paste/flipped), Alt+R (pose.rot_clear, `blender_keys.txt`), [ ] (pose.select_hierarchy), Strg+Umschalt+M
  (pose.select_mirror) - im Bereich Animation.

## 4. Backen (`rigBacken`)

- Die gewaehlte Sequenz mit ihrer Schicht: A'(b) = A(b) * B(b) * M * B(b)^-1, dann compressBone wie Carcass/G2C (Vorgaben wie
  `xsiAufnehmen`); ohne Schluessel bleiben die 14 Bytes der Quelle. Ergebnis auf den naechsten freien Platz des Ziels aus Stufe 2
  (`modellGlaAufPlatz`, mit dem XSI-Weg geteilt), Name `<Sequenz>_rig`, loopFrames und fps der Sequenz.

### Rundlauf Rig -> Backen (Schluss `mb4e_rl_<spiel>`, jede GLA ganz, jede Figur)

| Spiel | Skelette | Figuren (Rig / ohne Griff-Rolle) | Knochen-Bilder ohne Aenderung Byte fuer Byte |
|---|---|---|---|
| JKA | 66 | 86 (70 / 16) | 6 851 646 von 6 851 646 |
| JK2 | 21 | 48 (41 / 7) | 1 353 691 von 1 353 691 |
| Movie Duels | 120 | 553 (517 / 36) | 79 404 829 von 79 404 829 |

Ohne Griff-Rolle sind Geschuetze, Wagen, Kameras (model_root und Bauteile).

**Alles neu durch compressBone** (Pruefung der Kompression): Verschiebung in jedem Knochen-Bild gleich (0 Stufen). Die
Quaternionen der Raven-Daten liegen nicht auf Einheitslaenge (bis 35 Stufen, | |q| - 1 | * 16383) - das Spiel zeigt sie so
(MC_UnCompressQuat normiert nicht: M = (1 - |q|^2) I + |q|^2 R). compressBone zielt auf die naechste Drehung zu M; Grenze je
Knochen-Bild (Frobenius): |1 - |q|^2| |I - R| + Quantisierung (1,5 Stufen je Anteil: 4 * 3/16383 + 2 * 3/16383 * 2 sqrt 2).
Groesster Anteil an der Grenze: JKA 0,966, JK2 0,977, MD 0,966. Gegen die Bytes allein waren es bis 47 Stufen (darum ist die
Grenze aus compressBone und den Daten begruendet, nicht "2 Stufen"). Groesster Ortsunterschied in der Welt (mit fScale) 0,43
(JKA/MD, `_humanoid_vjun3` rhang_tag_bone) bzw. 0,53 Einheiten (JK2 atst) - das ist die Scherung der Raven-Daten, die ein
geaenderter Knochen beim Backen verliert; unveraenderte Knochen behalten die Bytes.

## Rot und gruen

- **ctest `modell_rig`** (neu): Erkennung nur nach Namen 6 FEHL (die vertauschten Namen), mit Hierarchie/Lage 0; dazu Skelett ohne
  Beine (Arme bleiben Arme), Mitnehmen JKA/JK2, Drehknochen-Anteil, slerp, Backen Byte fuer Byte, FK gebacken gegen die
  Schicht, gespiegelt gegen die Hand-Erwartung (Ry(+30) -> Ry(-30), Rx(+20) bleibt), Bindepose, Handpose offen, Pfad.
- **Gezogener Griff drehte nichts** (`mb4_rig1`: 0,00 Grad statt 30, Schluessel ohne Wirkung): posieren() lief nur bei modelDirty,
  das am Ende desselben Bilds wieder ausging, in dem der Griff das Dokument aenderte. Jetzt auch bei geaendertem Dokument
  (`mb4_rig2`: 30,00 Grad; Blender R 20,00 Grad).
- **Marka Ragnos ohne Beine**: die Schulter, die am tiefsten reichte, galt als Beinansatz - 18 Knochen unklar (`mb4_rl_jka`).
  Jetzt stimmen die Namen ab: 0.
- **Kreaturen mit IK-Fuessen** (Assassin-Droide, Tauntaun, Rancor, Hazardtrooper): die Reihenfolge nach dem Abstand vom Ansatz
  verteilte die Rollen falsch (4/8/4/5 unklar, z. B. rtalus = Unterschenkel). Jetzt nur, wo Hierarchie oder Geschwisterlage sie
  belegen: 0/0/0/1 (`mb4_rl4`).
- **Howler**: Beine und Arme am selben Knochen der Mitte, die Mehrheit der Armnamen machte die Beine zu Armen (12 unklar). Jetzt
  stimmt jedes Glied fuer sich: 4 (`mb4_rl5`).
- Selbsttest `rig` (Gruppe in `modellblock`): `mb4_rig1` 16/13 (die Fehler oben und Testfehler: Explorer nicht gerollt, Menues
  heissen "Titel###Menu_00"), `mb4_rig4` 26/0, `mb4_rig5` 26/0.

## Bildzeit (allein, BHED_D3D_DEBUG=0, nacheinander)

- `modellleistung` mit der exe von Stufe 2 (`mb4_zeit_vor`, b51504a63) gegen die neue (`mb4_zeit_nach`), dieselben 23 Bloecke:
  Summe der Mittel 24,27 / 25,28 ms, Summe der p99 66,22 / 58,76 ms - innerhalb der Streuung zwischen zwei Laeufen (einzelne
  Bloecke mal schneller, mal langsamer; der Bildabstand bleibt bei 4,2 ms). Neu im Grundweg ist nur das Posieren nach einer
  Aenderung des Dokuments und der leere Blick in die Schicht.
- Grosse Figur (`mb4_zeit_rig`, `BHED_MODELL1_NUR=rig`, clonetrooper_p2, 28460 Ecken, Animation laeuft): ohne Rig-Hilfen
  Bild-CPU 0,73 / 1,87 ms (Mittel/p99), Grafikkarte 0,055 ms; mit Rig, 10 Ghosts und 3 Pfaden (je 41 Bilder) Bild-CPU 1,05 /
  2,23 ms, Modell-CPU 0,20 / 1,27 ms, Grafikkarte 0,65 ms (21 Durchgaenge mehr), Bildabstand 4,17 -> 4,20 ms - fluessig.

## Schluss

- Sauberer Neubau (`--clean-first`, alle Ziele, `mb4e_bau.txt`): 0 Warnungen, 0 Fehler. ctest -j16 (`mb4e_ctest.txt`): 223 / 224 -
  `hilfe_orte` meldete `src/hilfe_orte.inc` veraltet (neue Handbuchseite); mit `tools/gen_hilfe_orte.py` erzeugt, danach gruen
  (224 / 224, neu `modell_rig`).
- Lints (`mb4e_lint.txt`): alle ok bis auf portability 101 -> 99 nach zwei fehlenden Kopfdateien in `tests/modell_rig.cpp`
  (wie mb2: 99), pruefcodes 11, efxflags, fremdprogramm (unveraendert, keine Datei dieses Auftrags); link/tidy brauchen Linux.
  Handbuch `--pruefen --pflicht=es --pflicht=cs` ok.
- Selbsttests parallel (19 zugleich): modellblock funktionen 60/0, tasten 43/0, mb2 37/0, blender 44/0, rig 26/0, rundlauf
  jka/jk2/md je 2/0, rigrundlauf jka/jk2/md je 4/0, bild jka/jk2/md je 1/0; modell 14/0, mission1 98/0, kuerzel 202/0,
  **Modus 1 2635/0**. modellblock mb3 (Stufe 2) war 15/6: die Rig-Rollouts schoben die Plaetze aus dem sichtbaren Teil des
  Befehlsfelds, das Ziehen auf BOTH_CIN_3 traf nicht. Pose, Ghosts, Pfade und Backen sind jetzt ab Werk zu, der Test rollt
  zu den Plaetzen wie sein Handbuchschritt; danach `mb4e2_mb3` 21/0, `mb4e2_rig` 26/0.
- Handbuch: Seite `modell-rig` in 6 Sprachen, Bilder h_mb4_ghosts/uebersicht/menue (2538 x 1384, alle angesehen), die Bilder
  h_mb3_* der Stufe 2 neu aufgenommen (der Bereich Animation hat jetzt die Rig-Rollouts). Beim Ansehen behoben: die Ghosts
  faerbten die aktuelle Pose ganz ein (jetzt Boden, Ghosts, Tiefe loeschen, Figur obenauf), die Figur stand klein am unteren
  Rand (eingepasst, schraeg von vorn), die Uebersicht nannte Rollen und Gruende als englische Schluessel (jetzt uebersetzt),
  Gesichtsflecken lagen uebereinander, zh-Bilder waren englisch (Sprache `zh-Hans`).

## Offen

- IK (Stufe 4): Pol-Ziele und Fussrolle sind angelegt und gezeichnet, wirken aber noch nicht; der Ballen fehlt bei Skeletten
  ohne Zehenknochen (JKA 53).
- Bewegungspfade in 3D bearbeiten kommt mit 3b/4.
- Ein Rig im Mission-Tab (mehrere Figuren) kommt mit Stufe 7.
- Kein Spielversuch (Auftrag): eine gebackene Sequenz im Spiel ist nur ueber die Dateien belegt (neu gelesen, Welt gegen die
  Schicht innerhalb der Kompression).

## Vorschlag 3b und 4

- **3b Animationswerkzeuge**: Dopesheet auf den Schluesseln der Schicht (je Griff eine Zeile, Schluessel ziehen = Bild aendern),
  Kurveneditor fuer q/t (Bezier statt slerp, Tangenten), Breakdowner/Push-Pull auf zwei Nachbarschluesseln, Aufraeumen
  (gebackene GLA-Bilder -> wenige Schluessel mit Fehlergrenze aus compressBone), Animation spiegeln (poseEinfuegen je Bild),
  Bewegungspfade mit ziehbaren Schluesselpunkten (FK: Drehung des Eltern, der den Punkt bewegt).
- **4 IK/FK**: Zwei-Glied-IK (Ober-/Unterarm, Ober-/Unterschenkel) mit den Pol-Zielen, Fussrolle um Ferse/Ballen/Zehe, Max:
  Greifen der Hand = IK, Drehen = FK, der Wechsel schreibt Schluessel; Blender: Regler je Glied + Einrasten. Pruefung: hin und
  zurueck an jedem Bild unter der Kompressionsgrenze aus Abschnitt 4.

## Changelog

### English
- Model tab, area Animation: create a control rig with one click. Every bone is recognised (role and side from name, hierarchy and position) - all bones of Jedi Academy (53), Jedi Outcast (72) and the droids and creatures of Movie Duels; unclear bones are shown in the rig overview, a figure to click on, and can be assigned by hand.
- Controls in the scene explorer and in the views (like 3ds Max boxes with the gizmo, like Blender rings with R/G): rotate with FK the way the game turns a head; fingers and twist bones follow. Pole targets for elbows and knees, foot roll and saber grip are prepared for IK.
- Poses: copy, paste, paste mirrored, back to the bind pose, hand poses (open, fist, saber grip) from the animations of the game; select parent, child or the opposite bone.
- Ghosts (onion skin) before and after the current frame, silhouette view and motion paths of bones and tags with frame dots and keys.
- Bake the sequence with the rig into the GLA, onto the next free slot of the map GLA, _humanoid.gla or an own GLA; unchanged bones stay byte for byte.
- New menu Animation in the model tab with every rig function, also in the right-click and the search (X / F3).

### Deutsch
- Model-Tab, Bereich Animation: ein Control Rig mit einem Klick anlegen. Jeder Knochen wird erkannt (Rolle und Seite aus Name, Hierarchie und Lage) - alle Knochen von Jedi Academy (53), Jedi Outcast (72) und den Droiden und Kreaturen von Movie Duels; unklare Knochen zeigt die Rig-Übersicht, eine Figur zum Anklicken, dort lassen sie sich von Hand zuordnen.
- Griffe im Szenen-Explorer und in den Ansichten (wie 3ds Max Kästen mit Gizmo, wie Blender Ringe mit R/G): mit FK drehen, so wie das Spiel einen Kopf dreht; Finger und Drehknochen gehen mit. Pol-Ziele für Ellbogen und Knie, Fußrolle und Säbelgriff sind für IK vorbereitet.
- Posen: kopieren, einfügen, gespiegelt einfügen, zurück zur Bindepose, Handposen (offen, Faust, Säbelgriff) aus den Animationen des Spiels; Eltern, Kind oder Gegenseite wählen.
- Ghosts (Onion Skin) vor und nach dem aktuellen Frame, Silhouetten-Ansicht und Bewegungspfade von Knochen und Tags mit Punkten je Frame und Schlüsseln.
- Die Sequenz mit dem Rig in die GLA backen, auf den nächsten freien Platz der Karten-GLA, der _humanoid.gla oder einer eigenen GLA; unveränderte Knochen bleiben Byte für Byte.
- Neues Menü Animation im Model-Tab mit jeder Rig-Funktion, auch im Rechtsklick und in der Suche (X / F3).

### 中文
- 模型选项卡的动画区域: 一键创建控制绑定。识别每根骨骼 (由名称、层次和位置得出角色和侧别) - Jedi Academy (53)、Jedi Outcast (72) 以及 Movie Duels 的机器人和生物的全部骨骼; 不确定的骨骼显示在绑定概览 (可点击的人形) 中, 可以手动指定。
- 场景资源管理器和视图中的控制器 (像 3ds Max 的方框加小部件, 像 Blender 的圆环加 R/G): 用 FK 旋转, 与游戏转动头部的方式相同; 手指和扭转骨骼会跟随。肘和膝的极向目标、脚部滚动和光剑握把已为 IK 准备好。
- 姿势: 复制、粘贴、镜像粘贴、回到绑定姿势、来自游戏动画的手部姿势 (张开、握拳、光剑握把); 选择父级、子级或对侧骨骼。
- 当前帧前后的残影 (洋葱皮)、剪影视图以及骨骼和标签的运动路径 (每帧一点并显示关键帧)。
- 将带绑定的序列烘焙到 GLA, 放到地图 GLA、_humanoid.gla 或自有 GLA 的下一个空闲槽位; 未改动的骨骼逐字节保持不变。
- 模型选项卡新增动画菜单, 包含每个绑定功能, 也可在右键和搜索 (X / F3) 中找到。

### 日本語
- モデルタブのアニメーション領域: ワンクリックでコントロールリグを作成。すべてのボーンを認識します (名前、階層、位置から役割と左右) - Jedi Academy (53)、Jedi Outcast (72)、Movie Duels のドロイドとクリーチャーのすべてのボーン。不明なボーンはクリックできる人体図のリグ概要に表示され、手動で割り当てられます。
- シーンエクスプローラーとビューのコントロール (3ds Max 風はギズモ付きの箱、Blender 風は R/G で使う輪): ゲームが頭を回すのと同じ FK で回転し、指とツイストボーンが追従します。肘と膝のポールターゲット、フットロール、セーバーグリップは IK 用に準備済みです。
- ポーズ: コピー、貼り付け、反転して貼り付け、バインドポーズへ戻す、ゲームのアニメーションから作ったハンドポーズ (開く、拳、セーバーグリップ)。親、子、反対側のボーンを選択。
- 現在フレームの前後のゴースト (オニオンスキン)、シルエット表示、フレームごとの点とキー付きのボーンとタグのモーションパス。
- リグ込みのシーケンスを GLA にベイクし、マップ GLA、_humanoid.gla、独自 GLA の次の空きスロットへ。変更のないボーンはバイト単位でそのまま。
- モデルタブに新しいアニメーションメニュー。リグの全機能があり、右クリックと検索 (X / F3) にもあります。

### Español
- Pestaña Model, área Animación: crea un rig de control con un clic. Se reconoce cada hueso (función y lado por nombre, jerarquía y posición): todos los huesos de Jedi Academy (53), Jedi Outcast (72) y los droides y criaturas de Movie Duels; los dudosos aparecen en la vista general del rig, una figura para hacer clic, y se pueden asignar a mano.
- Controles en el explorador de escena y en las vistas (como cajas con gizmo en 3ds Max, como anillos con R/G en Blender): gira con FK como el juego gira una cabeza; los dedos y los huesos de giro acompañan. Los objetivos de polo de codos y rodillas, el balanceo del pie y el agarre del sable quedan preparados para IK.
- Poses: copiar, pegar, pegar reflejada, volver a la pose de unión, poses de mano (abierta, puño, agarre del sable) de las animaciones del juego; seleccionar padre, hijo o el hueso opuesto.
- Fantasmas (papel cebolla) antes y después del fotograma actual, vista de silueta y trayectorias de huesos y tags con un punto por fotograma y las claves.
- Hornea la secuencia con el rig en el GLA, en el siguiente hueco libre del GLA de mapa, _humanoid.gla o un GLA propio; los huesos sin cambios quedan byte a byte.
- Nuevo menú Animación en la pestaña Model con cada función del rig, también en el clic derecho y en la búsqueda (X / F3).

### Čeština
- Karta Model, oblast Animace: jedním kliknutím vytvořte ovládací rig. Každá kost se rozpozná (role a strana podle jména, hierarchie a polohy) - všechny kosti Jedi Academy (53), Jedi Outcast (72) a droidů a tvorů Movie Duels; nejasné kosti ukáže přehled rigu, postava ke klikání, kde je lze přiřadit ručně.
- Ovladače v průzkumníku scény a v pohledech (jako v 3ds Max kvádry s gizmem, jako v Blenderu kroužky s R/G): otáčení s FK tak, jak hra otáčí hlavou; prsty a kroutící kosti jdou s ním. Cíle pólů loktů a kolen, odval chodidla a úchop meče jsou připraveny pro IK.
- Pózy: kopírovat, vložit, vložit zrcadlově, zpět do vazné pózy, pózy ruky (otevřená, pěst, úchop meče) z animací hry; vybrat rodiče, potomka nebo protější kost.
- Duchové (cibulová slupka) před a po aktuálním snímku, zobrazení siluety a dráhy pohybu kostí a tagů s tečkou za každý snímek a klíči.
- Zapečte sekvenci s rigem do GLA, na další volný slot mapového GLA, _humanoid.gla nebo vlastního GLA; nezměněné kosti zůstanou bajt po bajtu.
- Nová nabídka Animace na kartě Model s každou funkcí rigu, také v pravém tlačítku a ve vyhledávání (X / F3).
