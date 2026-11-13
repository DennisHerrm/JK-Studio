# Notizen ROT612 (13.11.): neun rote modellblock-Läufe auf karte1 - Ursachen und Anpassung - rc612

Auftrag ROT612 (Opus 5.5) nach AP11 (`naechste_auftraege.md`, 13.11.: „ROT auf karte1 (nicht von AP11): mb_rig, mb_mb5, mb_mb6,
mb_ptfix3, mb_ptsprung, mb_pfix, mb_rrl_md, mb_rrl_jka, mb_rrl_jk2“). Worktree `Y:\p_ap6`, Zweig `p_rot612` auf karte1 `9fe5a817c`.
Läufe über `tools/testrunde.py` (Pool 2, Leerlauf), Skripte und Protokolle `Y:\werkzeuge\rot612\`.

## Ergebnis

Alle neun sind **Testfehler**: kein Programmfehler. Die Tests erwarteten den Aufbau vor fünf gewollten Änderungen (Entscheidungen
und Prüffunde). Gemeinsam waren zwei Ursachen: die **vier Ansichten je Bereich wie 3ds Max** (R7, AP5, seit FIX612 P3-4 auch nach
dem Wechsel über „wie Blender“; drei Läufe) und **ENT3B** (Griff der Wurzel, IK/FK-Spur; vier Läufe). Eine gemeinsame Hilfsfunktion
der Selbsttests war betroffen: `pt1c::figurLaden` (alle Malgruppen und ap9).

Rot auf karte1 `9fe5a817c` (Lauf `r0`, ohne Änderung), grün nach der Anpassung (Läufe `r1`, `r2`, `r4`; Schlussrunde unten):

| Lauf | rote Zeile (r0) | Protokoll (r0) | Ursache | Art | Commit der Ursache (Entscheidung) | Behebung | grün |
|---|---|---|---|---|---|---|---|
| mb_rrl_md, mb_rrl_jka, mb_rrl_jk2 | „MB4 Rig-Rundlauf md/jka/jk2: fuer jede Figur mit einer Griff-Rolle ein Rig“ | jk2: „48 Figuren (Rig angelegt 48, ohne Rolle fuer einen Griff 7)“, jka 86 (86, 16), md 553 (553, 36) | Seit B17 trägt jedes Rig am Ende den Griff der Wurzel (Plattform wie CATParent, Rigify root) - auch Fahrzeuge und Geschütze ohne FK-Rolle (swoop, x-wing, eweb, turret …) bekommen ein nicht leeres Rig und zählten zweimal | Test | `40bc342dd` ENT3B B17 (Entscheidung B17: „Der Bodenkreis wird Griff der Wurzel … ganze Figur verschieben und drehen, wie CATs Plattform“) | `selbsttest_mb3.inc`: FK-Griffe (ohne Wurzel) und Wurzelgriffe getrennt zählen; Figur mit FK-Rolle: mindestens ein FK-Griff und genau ein Griff der Wurzel; Figur ohne FK-Rolle: **genau** der Griff der Wurzel (strenger als vorher „Rig leer“); der Bericht `mb4_rig_<spiel>.txt` nennt jede Figur ohne Rolle | 6 / 0 je Spiel; jk2 48 = 41 + 7, jka 86 = 70 + 16, md 553 = 517 + 36 |
| mb_rig | „MB4 Pfad: 21 Punkte je Bild, der Schluessel hervorgehoben, Punkt des Bilds = Ort der Hand in der Pose“ | „MB4 Pfad - Punkte 84, Schluessel 4“ | Der Test wechselt wie 3ds Max → wie Blender → wie 3ds Max; seit P3-4 kehrt der Bereich Animation mit seinen vier Ansichten zurück (R7), `pfadPunkte` zählt über alle gezeichneten Ansichten (4 × 21). Bis P3-4 ging der Wechsel mit einer Ansicht weiter - genau der Prüffund P3-4 | Test | `e77c27be3` FIX612 P3-4 (Prüffund P3-4 [N], R7 aus AP5: vier Ansichten je Bereich wie im Entwurf) | Zählung je sichtbare Ansicht: Ansichten = 4 bzw. 1 nach `vierAnsichten`, Punkte = 21 × Ansichten, Schlüssel = Ansichten; die Zeile nennt die Ansichten | 28 / 0 („Punkte 84, Schluessel 4, Ansichten 4 (vier)“) |
| mb_mb5 | „MB5 Pfad ziehen: das Becken steht bei +8 unter der Maus (1,5 Punkte), in der Bildebene, ein Schluessel“ (auch mit Pausen nach Bildern) | Statuszeile „Path point moved: pelvis at frame 9“; Ansicht 3 von vier, 656 × 393 | dieselbe: nach wie Blender → wie 3ds Max (Schritt „MB5: wieder wie 3ds Max“) vier Ansichten; dort liegen die Punkte +8 und +9 0,3 Punkte auseinander, ImGui rundet die Maus auf ganze Punkte ab, der Griff nahm +9 (Einzelheiten unten) | Test | `e77c27be3` FIX612 P3-4 | der Zug beginnt auf dem ganzen Punkt nahe +8, dem +8 am nächsten liegt (jedes andere Bild mindestens 0,05 weiter); die vier Ansichten bleiben; die Zeile vor dem Zug nennt Ansicht, Punkte und Abstände | 161 / 0, zweimal nebeneinander (Ansicht 3 von vier 656 × 393, Zug ab dem Punkt 1,40 neben +8, das nächste andere Bild 0,13 weiter; „Path point moved: pelvis at frame 8“) |
| mb_mb6 | „MB6 Glied auf FK (Rechtsklick der Ansicht): FK ab Frame +24“ und „MB6 Max E am Oberarm unter IK: das Glied wechselt an diesem Frame auf FK (Schluessel), der Oberarm dreht“ | „Arm R is FK from frame 24 to 24 - the FK keys took the visible pose“; „Max E - Hand bewegt 1.697, Anteil 0“ | Seit B14 hat jedes Glied seine IK/FK-Spur (wie `layerIKFKRatio` von CAT); der Anteil steht nicht mehr im IK-Schlüssel. Der Wechsel auf FK setzt einen Schlüssel der IK/FK-Spur (Anteil 0) und kein Ziel mehr - der Test suchte den IK-Schlüssel am Bild | Test | `a0c24eda8` ENT3B B14 (Entscheidung B14: „Überblenden wie CAT. Eigene Spur je Glied“) | neue Hilfe `fkSchluessel(art, bild)`: Schlüssel der IK/FK-Spur am Bild mit Anteil 0; beide Zeilen prüfen ihn statt `ikSchluessel` (der Wechsel auf IK prüft weiter Ziel-Schlüssel); die Zeile „Max E“ nennt Zug, FK- und IK-Schlüssel | 39 / 0 („Zug 4, FK-Schluessel der IK/FK-Spur ja, IK-Schluessel nein“) |
| mb_pfix | „PF H2: Vorschau mit neuem Teil - das Dokument behaelt die Aenderung (Name, Rueckgaengig)“ | „Teile 93 (vorher 93)“ | Der Test setzte `mz().art = 1` (In einen neuen Teil) **vor** dem Start von Detach. Seit KLEIN 4 merkt sich jedes Werkzeug seine Art und der Start setzt sie (Detach: Element im Teil) | Test | `9cfb323c8` KLEIN 4 (Prüffund 11.11.: Arten wie die getrennten Werte von Editable Poly in 3ds Max) | Art im offenen Kästchen wählen wie das Feld Art (art = 1, neu planen, Vorschau) - wie `selbsttest_klein.inc` und `selbsttest_md.inc` | 19 / 0 („Teile 94 (vorher 93)“) |
| mb_ptfix3 | „PT fix3 P24: Umbacken nach dem Detach meldet die geaenderten Dreiecke“ | „Image baked from the old UVs onto the new ones.“, „Teile 93“ | dieselbe (Detach blieb im Teil, die Dreiecke der Leinwand änderten sich nicht) | Test | `9cfb323c8` KLEIN 4 | wie pfix; dazu prüft P30 jetzt „Teile = vorher + 1“ - die Zeile „Leinwand traegt alle Dreiecke“ war ohne neuen Teil trivial grün | 14 / 0 („Teile 94 (vorher 93)“, Meldung „geänderte Dreiecke“) |
| mb_ptsprung | „PT Sprung: Flaeche - Strich ohne Sprung, Texel nach ptsprung_flaeche.bin“ | „groesster Sprung in der Ruhelage 13.88“ (r_arm → torso), Grenze 1,0; Ansicht 1 von vier, 371 × 540 | `pt1c::figurLaden` setzt Bereich Painting und `vierAnsichten = false` im selben Schritt. Der Abgleich je Bereich (R7) sieht den Wechsel erst im nächsten Bild, schreibt das `false` dem alten Bereich zu und gibt Painting seine Vorgabe (vier Ansichten). Alle Malgruppen laufen seit AP5 in vier kleinen Ansichten; der Strich ±25 Punkte über den Rumpf reichte in der kleinen Ansicht über den Arm | Test | `5d7037289` AP5 R7 (vier Ansichten je Bereich wie 3ds Max, Entwurf) | `figurLaden`: zuerst `modellPrAnsichtenAbgleich()` (derselbe Abgleich wie jedes Bild und wie beim Umschalten des Bedienmodus seit P3-4), dann eine Ansicht von vorn - wie der Schritt es nennt. Die Zeile „Leinwand body, beide Haende“ nennt die Ansicht | 5 / 0 („Ansicht 1 von einer, 745 × 1083“, größter Sprung 0,27) |

Kein Lauf hing an Zeitprüfungen unter Last oder an der Leerlauf-Priorität (alle Läufe mit Leerlauf-Priorität, Pool 2, rot und grün
reproduzierbar).

### Gegenprobe der gemeinsamen Hilfsfunktion figurLaden

`figurLaden` gilt für alle Malgruppen und ap9; mit einer Ansicht laufen sie wieder unter der Bedingung, für die sie geschrieben und
gemessen wurden (bis AP5). Grün (Lauf `r2`, Bau mit allen Anpassungen): mb_pt 43 / 0, mb_ptfix 7 / 0, mb_ptfix2 7 / 0 (ja),
mb_ptstufen 6 / 0, mb_ptfix3 14 / 0, mb_ptsprung 5 / 0, mb_ap9 98 / 0. Nicht im Pool und nicht gelaufen: ptlang
(Messung), ptspiel (Mod-Bau mit Fotos), ptzeit (Bildzeit) - sie nutzen figurLaden ebenso.

### mb5: vier Ansichten und der Punkt bei +9

Der Gang (BOTH_WALK1) läuft auf der Stelle: auch in der Seitenansicht liegen die Beckenpunkte der Nachbarbilder **unter einem Punkt**
auseinander - in einer Ansicht (1315 × 789) +7 1,0 und +9 0,6, in jeder der vier Ansichten (656 × 393) +7 0,5 und +9 0,3 (Läufe `r3`,
`r3v`). ImGui rundet die Mausposition in jedem Bild auf ganze Punkte ab (`ImFloor` in `UpdateMouseInputs`); der Griff
(`punktUnter`, `app_modelltab_kurven.cpp`) nimmt richtig den Punkt mit dem kleinsten Abstand zur gerundeten Maus. In vier Ansichten
lag danach +9 näher („Path point moved: pelvis at frame 9“), in einer Ansicht traf die Rundung zufällig +8. Ein Versuchslauf mit den
vier Ansichten und dem alten Startpunkt (Schalter nur im Versuchsbau, wieder entfernt) war rot wie auf karte1; mit einer Ansicht grün.

Kein Programmfehler: Punkte unter einem Punkt Abstand kann keine Maus unterscheiden, der nächste gewinnt (wie in 3ds Max). Die erste
Anpassung (eine Ansicht wie vor P3-4) war grün, hing aber weiter an der Rundung; die endgültige wählt den Startpunkt so, dass +8
eindeutig am nächsten liegt, und läuft in den vier Ansichten, die das Programm heute zeigt. Die Prüfung (Becken bei +8 unter der Maus
auf 1,5 Punkte, in der Bildebene, ein Schlüssel) ist unverändert.

Die Zeile „MB5 Rechtsklick der Ansicht: Untermenue Animationswerkzeuge“ war im ersten Lauf von AP11 einmal rot (von drei). In allen
sieben mb5-Läufen dieses Zweigs in vier Ansichten (r0 zweimal, r1, r3v, r4, r4b, s1) war sie grün - nicht nachgestellt. Falls sie
wiederkommt: der Rechtsklick liegt bei 85 % / 20 % der kleinen Ansicht (656 × 393) nahe am ViewCube.

### Was die Änderungen nicht tun

- Kein Test ist schwächer: rrl prüft für Figuren ohne FK-Rolle jetzt „genau der Griff der Wurzel“ (vorher „leer“), ptfix3 P30 prüft
  den neuen Teil mit, mb_rig verlangt 21 Punkte in **jeder** gezeichneten Ansicht. mb6 prüft den Schlüssel, den das Programm heute
  setzt, mit seinem Wert (Anteil 0).
- Die Malgruppen laufen in einer Ansicht - der Zustand, den `figurLaden` nennt und in dem sie geschrieben wurden (Alt+W, ein üblicher
  Zustand in 3ds Max); die vier Ansichten selbst prüfen ap5 (R7, P3-4) und fix612. mb_rig und mb5 laufen in den vier Ansichten.

## Hinweis für AP14b

`h_pt_malen` (pt, `bilder_neu.md`) entsteht mit `figurLaden`: auf karte1 bis jetzt in vier kleinen Ansichten, mit diesem Zweig wieder
in einer Ansicht von vorn (wie das Bild bis AP5). Ein vor dem ff aufgenommenes Bild danach neu aufnehmen.

## Schlussrunde des Zweigs

Auf dem Endstand (5 Commits, Bau `b5`, karte1 `9fe5a817c` unverändert - kein Rebase nötig):

| Prüfung | Ergebnis |
|---|---|
| Bau ALLES (`--parallel 2`, /W4 /WX) | 0 Fehler, 0 Warnungen |
| ctest | 273 / 273 (644 s, `-j 2`, Leerlauf) |
| die neun Läufe (`s1`) | mb_rig 28/0, mb_mb5 161/0, mb_mb6 39/0, mb_ptfix3 14/0, mb_ptsprung 5/0, mb_pfix 19/0, mb_rrl_md 6/0, mb_rrl_jka 6/0, mb_rrl_jk2 6/0 |
| berührte Läufe (`s1`, `s2`) | figurLaden: mb_pt 43/0, mb_ptfix 7/0, mb_ptfix2 7/0 (ja), mb_ptstufen 6/0, mb_ap9 98/0; Nachbarn: mb_mb3 23/0, mb_irl_jk2 3/0 |
| Lints | wie AP11: pruefcodes 11, portability 101, efxflags/fremdprogramm bekannt; alle übrigen ok |

Bewusst nicht gelaufen: Programmteile sind nicht geändert (nur `gui/selbsttest_*.inc`), daher keine Zeichen-, D3D11- oder Handbuchläufe;
ptlang/ptspiel/ptzeit (nicht im Pool, Messung bzw. Mod-Bau). Lastregel eingehalten: Bau `--parallel 2`, höchstens 2 jkstudio,
Leerlauf-Priorität.

## Changelog

## English

- Self tests of the Model tab follow the current behaviour: every rig carries the root control (also vehicles and turrets), switching a limb to FK keys its IK/FK track, like 3ds Max the Animation and Painting areas show four views, and each modelling tool keeps its own type; the paint tests pick one view again, as they were written.

## Deutsch

- Selbsttests des Model-Tabs folgen dem heutigen Verhalten: jedes Rig trägt den Griff der Wurzel (auch Fahrzeuge und Geschütze), der Wechsel eines Glieds auf FK setzt einen Schlüssel seiner IK/FK-Spur, die Bereiche Animation und Painting zeigen wie 3ds Max vier Ansichten, und jedes Modellierwerkzeug behält seine eigene Art; die Maltests wählen wieder eine Ansicht, wie sie geschrieben wurden.

## 中文

- Model 标签的自检遵循当前行为: 每个绑定都带有根控制器 (载具和炮塔也一样), 将肢体切换为 FK 时会在其 IK/FK 轨道上设置关键帧, 动画和绘制区域像 3ds Max 一样显示四个视图, 每个建模工具保留自己的类型; 绘制测试重新选择单个视图, 与编写时一致。

## 日本語

- Model タブのセルフテストを現在の動作に合わせました: すべてのリグにルート コントロールがあり (乗り物や砲台も)、肢を FK に切り替えると IK/FK トラックにキーが入り、アニメーションとペイントの領域は 3ds Max と同様に 4 つのビューを表示し、各モデリング ツールは独自のタイプを保持します。ペイントのテストは書かれた当時と同じく 1 つのビューを選びます。

## Español

- Las autopruebas de la pestaña Model siguen el comportamiento actual: cada rig lleva el control raíz (también vehículos y torretas), pasar una extremidad a FK pone una clave en su pista IK/FK, las áreas Animación y Painting muestran cuatro vistas como 3ds Max y cada herramienta de modelado conserva su propio tipo; las pruebas de pintura vuelven a usar una sola vista, como se escribieron.

## Čeština

- Autotesty karty Model odpovídají dnešnímu chování: každý rig nese kořenový ovladač (i vozidla a věže), přepnutí končetiny na FK vloží klíč do její stopy IK/FK, oblasti Animace a Painting ukazují jako 3ds Max čtyři pohledy a každý modelovací nástroj si ponechává vlastní typ; testy malování opět volí jeden pohled, jak byly napsány.
