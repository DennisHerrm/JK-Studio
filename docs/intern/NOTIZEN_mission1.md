# Notizen mission1: vier Fehler aus shanks Test (rc606)

Zweig `mission1` ab karte1 = rc606 (1274434d), Worktree `Y:\mission`, Bau
`Y:/mission/build-mission`. Selbsttest `BHED_EDITORTEST=mission1`
(`gui/selbsttest_mission1.inc`); einzelne Teile mit `BHED_M1_TEIL` (md, jka,
zwischen, gizmo, drehen, undo - durch Leerzeichen getrennt). Daten: das reine
JKA (`BHED_JKA_BASIS`, sonst Steam "GameData original/base") und die
eingerichteten Movie-Duels-Ordner. Neue Programmtexte gab es nicht (die
vorhandenen Bahn- und Undo-Texte passen), darum keine `tools/i18n_mission1.py`.

Gegenprobe: derselbe Selbsttest gegen den Stand ohne die Korrekturen
(`git apply -R` der Korrekturen, Lauf `rot5`) und mit ihnen (Lauf `gruen6`).

## Ergebnisse je Fehler

### 1. Gesichtsanimation / "rote Pose" = T-Pose aus dem Kino-Skelett

shanks genaue Angabe: academy1, intro.ibi, 0:16.72 (Kamera lukewalk2) - Luke
läuft die Treppe in T-Pose herunter. Nachgestellt mit den JKA-SP-Daten
(GameData original/base): Luke spielt dort `BOTH_CIN_1`; das steht nur in
`models/players/_humanoid_academy1/animation.cfg` (Bilder 18..41 der
`_humanoid_academy1.gla`).

Wie das Spiel es lädt (OpenJK NPC_stats.cpp G_ParseAnimFileSet): für
"_humanoid" erst `_humanoid/animation.cfg` (glaIndex 0), dann
`_humanoid_<level.mapname>/animation.cfg` in DIESELBE Tabelle animations[]
(glaIndex 1, G2API_PrecacheGhoul2Model der zweiten .gla direkt nach der
ersten); PM_SetAnimFinal schaltet mit G2API_SetAnimIndex auf die .gla der
gesetzten Animation. behaved hatte das schon nachgebaut (Kino-Skelett als
`GlaAnimation::anhang`, Bilder hinter denen des Grundskeletts), aber:

- Ursache: `figurKnochen` kappte jedes Bild auf `numFrames - 1` des
  Grundskeletts. Bild 21395 (= 21376 + 19) wurde zum letzten Bild der
  `_humanoid.gla` - der T-Pose. Rot: Abweichung vom Bild der Animation 24,37;
  Foto T-Pose. Jetzt `GlaAnimation::bildZahl()` (samt Anhang).
- Die zweite Datei überschrieb gleichnamige Einträge nicht, sondern kam hinten
  dazu - FACE_ALERT bis FACE_TALK4 stehen in academy1 in beiden; die Suche
  fand die alte Zeile. Jetzt ersetzt der Kino-Eintrag den gleichnamigen wie in
  G_ParseAnimationFile(1, ...). Damit kommt FACE_TALK in academy1 aus dem
  Kino-Skelett - das ist der Zusammenhang mit "Face Talk": beim Sprechen lief
  der Mund schon richtig, die T-Pose kam vom Körperbild.

Wächter über alle 32 SP-Karten mit eigenem `_humanoid_<karte>` (jeweils der
intro-Reiter, alle 500 ms, jede sichtbare Figur): was gezeichnet wird, gleich
dem Bild der Animation (ungekappt). Rot: 908 falsche Proben auf 10 Karten
(academy3/5, t1_danger, t1_inter, t1_surprise, t2_rogue, t2_trip, t3_bounty,
t3_stamp, taspir1). Grün: 29 Karten geprüft (academy4, academy6, vjun1 lassen sich nicht als Mission laden), 0 falsche Proben, 1332 Proben mit einem Bild aus dem Kino-Anhang.

Ausserdem gleich geblieben und als Wächter im Test: der Mundlauf liegt nur auf
"face" (CG_G2SetHeadAnim auf faceBone), über die ganze Szene kein Körper vom
Mund bewegt (MD und JKA).

### 2. Gizmo bleibt beim Abspielen (und alle Bearbeitungshilfen)

Ursache: beim Abspielen schaut die Ansicht durch die Kamera; dieser Zweig
rief `drawCameraGizmos` nicht auf, `gizmoShown` blieb vom letzten freien Bild
stehen, und `kameraBahnZeichnen` malte den Kamera-Gizmo an der alten
Bildstelle obenauf - die ganze Wiedergabe lang. Der Entity-Gizmo, Auswahl,
Umriss, Bahnen, Griffe, Fadenkreuz und Schilder wurden ohnehin weiter
gezeichnet.

Jetzt: solange die Wiedergabe läuft, keine Bearbeitungshilfen (Gizmo,
Fadenkreuz, Auswahlkasten, Umriss, Figuren- und Kamerabahnen, Griffe,
Wegnetz, Animationsrauten, Entity-Schilder und Namen, Kamera-Leiste und
-Vorschau, Gizmo- und Platzieren-Leiste) und nichts davon greifbar; nach Stop
wird einmal neu gezeichnet, alles ist wieder da.

Test (Fotos, Pixelprüfung im Kartenbild), Figur gewählt / Kamera gewählt:

| Messung (rot = alter Stand, grün = neu) | vorher | beim Abspielen | nach Stop |
|---|---|---|---|
| Figur gewählt: Gizmo-Achsenfarben, rot | 1071 | 1135 / 1141 | 1069 |
| Figur gewählt: Gizmo-Achsenfarben, grün | 1071 | 0 / 0 | 1069 |
| Figur gewählt: Fadenkreuz-Anteil, rot | 0,32 | 0,50 / 0,50 | 0,32 |
| Figur gewählt: Fadenkreuz-Anteil, grün | 0,32 | 0,00 / 0,00 | 0,32 |
| Figur gewählt: Hilfsfarben (Auswahl, Griffe, Marken), rot | 5080 | 2128 / 2149 | 4957 |
| Figur gewählt: Hilfsfarben, grün | 5080 | 0 / 0 | 4957 |
| Kamera gewählt: Gizmo-Achsenfarben, rot | 1338 | 1830 / 1832 | 1338 |
| Kamera gewählt: Gizmo-Achsenfarben, grün | 1338 | 216 / 216 | 1338 |
| Kamera gewählt: Fadenkreuz-Anteil, rot | 0,50 | 0,43 / 0,43 | 0,50 |
| Kamera gewählt: Fadenkreuz-Anteil, grün | 0,50 | 0,00 / 0,00 | 0,50 |

Die Achsenfarben beim Abspielen in der Kameraansicht (grün 216) sind Bildpunkte
der Karte in ähnlichen Farben, kein Gizmo (das Foto zeigt keinen).

### 3. Wegpunkt zwischen Anfang und Ende "löscht das Ende"

Nachgestellt: Rosh (npc_rosh2) läuft 85250-91550 ms zu exitNav1; bei 88400 ms
an der X-Achse gezogen. Das Skript bekam ein neues Laufziel vor dem alten,
Anfang (Spawner) und Ende (exitNav1) blieben - aber zwei Fehler:

- Die Vorschau: Rosh blieb am neuen Punkt stehen ("findet keinen Weg"), die
  Bahn zum alten Ziel verschwand - das sah aus wie ein gelöschtes Ende.
  Ursache, gegen OpenJK nachgelesen: der neue Punkt war eine Kopie des alten
  Ziels, ein waypoint_navgoal - und damit ein Knoten des Wegnetzes
  (NAV::SpawnedPoint, PT_GOALNODE), mit NO_AUTO_CONNECT eine Insel ohne Kante
  (Netz: exitNav4 insel 1, 0 Kanten; auch ohne NO_AUTO_CONNECT keine Kante,
  kein Wegpunkt in Reichweite). Angekommen ist er der nächste Knoten der Figur
  (GetNearestNode bestraft alle anderen über `previous`, g_navigator.cpp
  2500-2509), NAV::FindPath findet von der Insel keinen Weg - das Spiel ließe
  sie genauso stehen. Jetzt ist der Zwischenpunkt ein `ref_tag`: SET_NAVGOAL
  sucht zuerst einen TAG (Q3_SetNavGoal: TAG_GetOrigin2/TAG_GetRadius), ein
  ref_tag ist ein TAG ohne Knoten (SP_reference_tag, g_ref.cpp:400, Radius 16
  - der Port nahm 12, jetzt 16), die Wege laufen über das unveränderte Netz.
  Rot: Meldung "findet keinen Weg" 1, kein Lauf nach dem Punkt; grün: 0, Rosh
  läuft vom Punkt weiter zu exitNav1 (über den nächsten Wegpunkt, wie NAV).
- Findet eine Figur von einem Punkt wirklich keinen Weg, zeigt die Bahn der
  gewählten Figur jetzt eine rote gestrichelte Linie zum Ziel mit "Kein Weg von
  hier zu <Ziel> (ab <Zeit>) - die Figur bleibt stehen wie im Spiel"
  (Probe: der neue Punkt zur Probe als unverbundener Navgoal).
- Strg+Z über der Karte nahm nur das Laufziel zurück, das Skript zeigte danach
  auf ein fehlendes Ziel. Ursache: zwei getrennte Rückgängig-Stapel, Strg+Z
  über der Karte griff immer zuerst zu den Entities. Jetzt trägt jeder Schritt
  einen Stempel (`naechsterAenderungsStempel`, Document::undoStempel,
  App::entUndoStempel); Strg+Z nimmt die jüngste Änderung zurück, Laufziel und
  Skript eines Zwischenpunkts gehen zusammen, Strg+Y holt den zuletzt
  zurückgenommenen Schritt (App::zurueckGenommen). Rot: "Skript gleich 0";
  grün: Strg+Z und Strg+Y bytegleich.

### 4. Drehen zwischen zwei Punkten verdreht den Anfang

md_am_sith (Mustafar), Schiff sta2 (zwei move: 0-5000, 5050-11050 ms).

- Mitten in einer Fahrt (2500 ms) war es schon richtig: neuer Bahnpunkt mit
  Winkeln, Anfang und Ende unverändert (auch während des Zuges).
- Nach der letzten Fahrt (und in einer Pause zwischen zwei Fahrten) drehte
  bzw. verschob der Gizmo die Entity selbst - also den Anfang: rot,
  Entity-Winkel und Stand bei 0 s änderten sich.

Jetzt (`fahrtSchluesselMoeglich`/`fahrtSchluesselSchreiben`): steht das
Schiff nach einer Fahrt, entsteht ein neuer Schlüssel - ein `move` vom Ende
der vorigen Fahrt bis zu diesem Augenblick (Q3_Lerp2Pos dreht ab
currentAngles), eingefügt an der Zeit des Fahrtendes (befehlZurZeit teilt
ein wait, die Summe bleibt). Ein folgender `move` ohne Winkel bekommt die,
mit denen er vorher ankam; ebenso der zweite Teil eines geteilten `move`.

Test (Werte vorher/nachher): 12050 ms, moves 2 -> 3, Entity-Winkel
0 0 0 -> 0 0 0, Anfang 0,00, letzter Schlüssel 0,00, zur Zeit gedreht 90,4°.

### Kamera: Auto-Key und Werte als EIN Rückgängig-Schritt

In der Mini-Szene gefunden (eine Kamerafahrt von 4 s, angefasst mittendrin):
der Auto-Key legte erst den neuen Schlüssel an (ein Schritt) und schrieb beim
Loslassen die Werte (noch ein Schritt) - Strg+Z nahm nur die Werte zurück, der
Schlüssel blieb (rot: "Skript 0"). Jetzt `Document::fasseZusammen` ab der Tiefe
vor dem Auto-Key: ein Schritt.

### Rückgängig/Wiederholen nach jedem Gizmo-Zug (Wunsch von shank)

Figur (Luke, academy1), Schiff (sta2, md_am_sith), Kamera (beide Karten);
vor dem ersten, zwischen zwei, nach dem letzten Punkt; Verschieben und Drehen
- 30 Fälle plus zwei Ketten aus drei Zügen. Je Fall: Stand vorher, nach der
Änderung, nach Strg+Z, nach Strg+Y, nach einem zweiten Strg+Z; verglichen
werden Skripttext (writeScript, bytegleich), alle Entities, die Schlüssel
bzw. Wegpunkte (Anzahl, Zeiten, Orte, Winkel) und die Vorschau (Ort und
Winkel am Anfang und zur Zeit). Rot: Strg+Y stellte in 14 von 30 Fällen
nicht her (alter Wiederholen-Schritt des anderen Stapels). Grün: alle.

### Szenenzustand bei t=0 und am Ende (Wunsch von shank)

Je Gizmo-Zug (Figur, Schiff, Kamera; vor, zwischen, nach; verschieben und
drehen) der Zustand aller Figuren, Objekte und der Kamera bei t=0 und am Ende
der Szene (Ort, Winkel, Animation, Sichtbarkeit; beim Ziel dazu die Zeit seines
letzten Fahrtendes), bytegleich verglichen:

- zwischen zwei Punkten: niemand weicht bei t=0 oder am Ende ab;
- vor dem ersten Punkt: bei t=0 nur das Ziel, am Ende niemand;
- nach dem letzten Punkt: bei t=0 niemand, am Ende nur das Ziel.

Zwei Regeln aus dem Spiel: eine laufende Figur hält, sobald sie im Radius des
Laufziels ist (NPC_ReachedGoal) - kommt sie auf anderem Weg an, darf ihr Ort am
Ende um höchstens 16 Einheiten abweichen (berichtet). Und wartet ein Ablauf auf
ihre Ankunft (dowait), verschiebt ein längerer Weg alles danach - verglichen
wird darum mit dem Ende der Szene NACH der Änderung.

Rot (vor der Korrektur): eine Figur NACH ihrem letzten Lauf verschoben
verschob den Spawner, also den Anfang (bei t=0 wich npc_luke1 ab:
-1408 -1304 744 -> -1376 -1304 744). Jetzt (`laufZielNachLaufMoeglich`/
`...Schreiben`) ein neues Laufziel (ref_tag), gesetzt, wenn sie am bisherigen
Ende ankommt. Ergebnis grün: alle 29 Fälle in academy1/md_am_sith und alle 5 in der
Mini-Szene. Beispiele (academy1, npc_luke1): vor dem ersten verschoben - bei
t=0 nur Luke (-1408 -1304 744 -> -1376 -1304 744), am Ende niemand; zwischen -
niemand; nach dem letzten verschoben - bei t=0 niemand, am Ende nur Luke
(-1487 -1165 688 -> -1472 -1165 688). Mini-Szene: m1_held zwischen verschoben
kommt 3,1 Einheiten anders an (im Radius), sonst niemand.

### Selbst gebaute Mini-Szene

Neue Mission "m1szene" (New mission auf der Geometrie von md_am_sith), eigene
Entities (Figur m1_held, Laufziel m1_ziel, Schiff m1_schiff), eigenes Skript,
eigenes Kino-Skelett `models/players/_humanoid_m1szene/` in einer pk3 eines
eigenen Spielordners im Laufordner. Darin: BOTH_CIN_1 aus dem eigenen
Kino-Skelett, Zwischenpunkt mit exaktem Strg+Z/Strg+Y, Hilfen beim Abspielen,
Drehen nach der letzten Fahrt, fünf Rückgängig-Fälle mit Szenenzustand und
eine Kette. Ergebnis: 22 von 22 grün. Rot war dort nur der Kamera-Auto-Key (siehe unten);
die übrigen Fehler waren vorher schon an academy1/md_am_sith rot.

## Schlussrunde

- Sauberer Neubau (`--clean-first`): 0 Fehler, 0 Warnungen; ctest 158/158.
- Selbsttest mission1 voll (Lauf voll7, alle Teile): 91 OK, 5 FEHLER - alle in
  der Mini-Szene, danach behoben (Lauf mini4: 22/22).
- umriss 46/46, kamera 95/95, missionsbau 95/95, karteneditor (fotos) 78/78.
- Modus 1 (mit BHED_HANDBUCH, sonst finden die Touren aus dem Laufordner
  /y/mission_laeufe kein handbuch\<sprache>	ouren.json): 2644 OK, 1 FEHLER -
  "Copy schreibt in die Windows-Zwischenablage" (die Zwischenablage ist mit den
  parallelen Läufen geteilt; im ersten Lauf zweimal, hier einmal - nicht von
  mission1 berührt).
- Kartenprüfung (tools/kartenpruefung.py, 278 Karten, Movie Duels): 0
  abgestürzt/hängend, 0 nicht geladen, 0 GPU-/D3D-/Protokollfehler; die übrigen
  Summen (1 nicht gezeichnete Entity, 2 Karten mit NPC ohne Modell, 27 mit
  fehlender .efx, 29 mit fehlendem Klang) gleich wie kp606.
- lint_sources, lint_fassung: ok.

## Gegen das Spiel geprüft / nicht geprüft

Gegen den OpenJK-Quelltext (nachgelesen, Formeln/Abläufe im Programm gleich):

- Mundlauf nur auf dem Knochen "face" (cg_players.cpp CG_G2PlayerHeadAnims /
  CG_G2SetHeadAnim, G2-Erbe CBoneCache::EvalLow): Knochenvergleich über die
  ganze Szene.
- ICARUS: `set` blockiert nicht, nur wait/waitsignal (icarus/TaskManager.cpp
  CTaskManager::Go) - das neue Laufziel wird darum über ein wait zur
  Ankunftszeit gesetzt, wie bisher.
- `move` mit Winkeln dreht ab currentAngles über den kürzeren Weg
  (Q3_Lerp2Pos); der neue Schlüssel nach einer Fahrt ist ein solcher move.
- Kino-Skelett: G_ParseAnimFileSet / G_ParseAnimationFile (NPC_stats.cpp,
  zweite Datei in dieselbe Tabelle, glaIndex 1), PM_SetAnimFinal mit
  G2API_SetAnimIndex (bg_panimate.cpp): Bild der Animation gegen das
  Gezeichnete, über 29 SP-Karten.
- Zwischenpunkt: Q3_SetNavGoal (TAG zuerst, TAG_GetRadius), SP_reference_tag
  (Radius 16), SP_waypoint_navgoal/NAV::SpawnedPoint, NAV::GetNearestNode
  (previous-/goal-Strafen), NAV::FindPath - nachgelesen; die Wegsuche selbst
  ist der vorhandene NAV-Port (Ergebnis nicht im Spiel nachgeprüft).
- Ankunft im Radius (NPC_ReachedGoal) als einzige erlaubte Abweichung am Ende.

Nicht gegen das Spiel geprüft (kein Spielstart erlaubt): das Bild beim
Abspielen ist "ohne Bearbeitungshilfen" (das Spiel zeigt keine) - geprüft per
Foto und Pixelzählung, nicht per Vergleich mit einem Spielbild; Rückgängig/
Wiederholen ist reine Editorfunktion ohne Spiel-Vorbild.

## Offen

- Kino-Skelett: im Spiel schaltet G2API_SetAnimIndex die .gla des ganzen
  Modells um (die zuletzt gesetzte Animation entscheidet); behaved liest je
  Knochenlauf aus der richtigen Datei. Unterschied nur, wenn Beine und
  Oberkörper gleichzeitig Animationen aus verschiedenen Dateien spielen.
- Drehen einer Figur zu einer Zeit, die hinter einem waitsignal liegt, setzt
  nichts (Statuszeile "Zeit unbekannt") - unverändert.
- Wächter Kino-Skelett: academy4, academy6 und vjun1 lassen sich über "Load
  mission" nicht als Mission laden und sind darum nicht geprüft.
- Rot/grün je Fehler: T-Pose (Lauf kinorot gegen den alten Stand), Zwischenpunkt
  und Undo (rot5), Hilfen beim Abspielen (rotab), Figur nach dem letzten Lauf
  (inv1), Kamera-Auto-Key (mini3); die Mini-Szene selbst lief nur mit dem
  neuen Stand vollständig (mit dem alten nur bis zu diesen Fehlern).

## Changelog

## English
- Moving a figure after its last walk adds a new navgoal it walks to when it arrives, instead of moving its spawner (the start).
- Story cutscenes: animations that exist only in the level's own skeleton (models/players/_humanoid_<map>, e.g. BOTH_CIN_1 in academy1) are drawn with their frames - figures no longer stand in T-pose; a name in both files takes the level's frames, as in the game (also FACE_TALK).
- A point inserted in the middle of a walk is now a ref_tag: the nav network stays as it was, and the figure walks on to its old goal as in the game. If a figure finds no path from a point, a red dashed line with "No path from here to ..." leads to the goal.
- While the timeline plays, the map view looks like the game: no gizmo, crosshair, selection box or outline, paths, handles, markers or labels; after Stop everything is back.
- One Ctrl+Z takes back a whole gizmo change in the map view (a new path point together with its new navgoal), Ctrl+Y redoes exactly the step taken back last.
- Moving or rotating a ship after a flight (in a pause or after the last one) creates a new key instead of changing the start.

## Deutsch
- Eine Figur nach ihrem letzten Lauf zu verschieben legt ein neues Laufziel an, zu dem sie nach der Ankunft geht, statt ihren Spawner (den Anfang) zu verschieben.
- Story-Zwischensequenzen: Animationen, die nur im Kino-Skelett der Karte stehen (models/players/_humanoid_<karte>, z. B. BOTH_CIN_1 in academy1), werden mit ihren Bildern gezeichnet - keine T-Pose mehr; ein Name in beiden Dateien bekommt die Bilder der Karte, wie im Spiel (auch FACE_TALK).
- Ein mitten im Lauf eingefügter Punkt ist jetzt ein ref_tag: das Wegnetz bleibt, wie es war, und die Figur läuft wie im Spiel weiter zu ihrem alten Ziel. Findet eine Figur von einem Punkt keinen Weg, führt eine rote gestrichelte Linie mit "Kein Weg von hier zu ..." zum Ziel.
- Während die Zeitleiste abspielt, sieht die Kartenansicht aus wie im Spiel: kein Gizmo, Fadenkreuz, Auswahlrahmen oder Umriss, keine Bahnen, Griffe, Marken oder Namen; nach Stop ist alles wieder da.
- Ein Strg+Z nimmt eine ganze Gizmo-Änderung in der Kartenansicht zurück (einen neuen Bahnpunkt samt neuem Navgoal), Strg+Y stellt genau den zuletzt zurückgenommenen Schritt wieder her.
- Ein Schiff nach einem Flug (in einer Pause oder nach dem letzten) zu verschieben oder zu drehen legt einen neuen Schlüssel an, statt den Anfang zu ändern.

## 中文
- 在角色最后一段行走之后移动它,会新增一个到达后走向的导航目标,而不再移动其生成点(起点)。
- 剧情过场:只存在于关卡自带骨架(models/players/_humanoid_<地图>,例如 academy1 中的 BOTH_CIN_1)里的动画现在按其帧绘制——角色不再呈 T 字姿势;两个文件中同名的动画使用关卡的帧,与游戏一致(FACE_TALK 也是)。
- 在行走途中插入的点现在是 ref_tag:导航网络保持不变,角色会像游戏中一样继续走向原来的目标。如果角色从某点找不到路径,一条红色虚线和“从此处无法到达 ...”会指向目标。
- 时间轴播放时,地图视图与游戏中一致:不显示操纵器、准星、选择框或轮廓、路径、控制柄、标记或名称;停止后全部恢复。
- 一次 Ctrl+Z 撤销地图视图中整个操纵器修改(新的路径点连同其新导航点),Ctrl+Y 准确恢复最后撤销的那一步。
- 在飞行结束后(停顿中或最后一段之后)移动或旋转飞船会新建关键帧,而不再修改起点。

## 日本語
- 最後の歩行の後にキャラクターを移動すると、スポナー(始点)を動かす代わりに、到着後に向かう新しいナビゴールが追加されます。
- ストーリーのカットシーン:マップ専用の骨格(models/players/_humanoid_<マップ>、例: academy1 の BOTH_CIN_1)にだけあるアニメーションを、そのフレームで描画します。キャラクターが T ポーズになることはもうありません。両方のファイルにある名前はゲームと同じくマップ側のフレームを使います(FACE_TALK も)。
- 歩行の途中に挿入した点は ref_tag になりました。ナビネットワークは変わらず、キャラクターはゲームと同じく元の目的地へ歩き続けます。ある点から経路が見つからない場合は、赤い破線と「ここから ... への経路がありません」が目的地へ伸びます。
- タイムラインの再生中、マップビューはゲームと同じ見た目になります。ギズモ、照準、選択枠や輪郭、経路、ハンドル、マーカー、名前は表示されず、停止すると元に戻ります。
- Ctrl+Z を 1 回押すと、マップビューでのギズモ変更全体(新しい経路点とその新しいナビゴール)が元に戻り、Ctrl+Y は最後に元に戻した手順を正確にやり直します。
- 飛行の後(間の停止中または最後の飛行の後)に船を移動・回転すると、始点を変えずに新しいキーが作られます。
