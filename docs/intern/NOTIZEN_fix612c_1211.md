# Notizen FIX612C (12.11.): Funde P3-5 bis P3-10 und Vermutungen PV4/PV5 der Codepruefung seit rc611 - rc612

Auftrag FIX612C (Fable, nach dem Limit Opus 5.5): `JKStudio_Uebergabe/plaene/codepruefung_seit_rc611_p3.md`, Abschnitt "Neue Funde nach
der Pause" und die Vermutungen PV4/PV5. Worktree `Y:\p_ap3`, Zweig `p_fix612c`. Selbsttest neu `gui/selbsttest_fix612c.inc`
(`BHED_EDITORTEST=modellblock`, `BHED_MODELL_NUR=fix612c`), Skripte, Patches und Laeufe in `Y:\werkzeuge\fix612c\`.
Vorgehen je Fund: erst rot im laufenden Programm (Bau mit Test und Proben, ohne Behebung), dann beheben, dann derselbe Test gruen.

## Ergebnis je Fund

| Fund | Rot (Lauf, Messwert) | Behebung | Gruen (Lauf, Messwert) |
|---|---|---|---|
| P3-5 [M] nur FK nach Control Rig | `rot_fix612c`: nach nur FK 4 IK-Spuren mit Schluesseln, 0 IK-Griffe; Oberarm R um 30 Grad: rhand bewegt 0,000; gewaehltes Pol-Ziel -> Griff 22 = Saebelgriff (Art 17); "Auf IK" legt Schritt an; Auf Rig legen: 4 Spuren; Strg+Z: nur FK bleibt, 0 IK-Griffe (11 FEHLER) | `RigDaten::nurFk`, Backen beim Wechsel, Wahl wiederfinden, Sperren, `Dokument::zusammenlegen` | `g1`/`g3`: 0 Spuren, Pose ueber 30 Bilder max. 0,000033 (Grenze 0,0001), rhand 9,93 bewegt, Wahl -1, kein Schritt bei "Auf IK", Strg+Z: 4 Spuren + 8 IK-Griffe, Strg+Y wieder nur FK, Control Rig erstellen danach ein Schritt (11 OK) |
| P3-6 [M] Knochen waehlen wie 3ds Max | `rot_fix612c`: Anzeige fehlt (Probe -1), kein Punkt im Rechtsklick, nach Q Werkzeug 2 und 0 gewaehlt, nach Wechsel von wie Blender Werkzeug 2 (5 FEHLER) | Umschalter, Meldung, Haken, Anzeige oben in der Ansicht, Q/W/E/R, Moduswechsel | `g4`: an mit Meldung und Anzeige, Haken im Rechtsklick, Q beendet, Klick waehlt wieder den Rumpf, Wechsel von Blender: Werkzeug 0, Rechtsklick an/aus (5 OK) |
| P3-7 [N] Explorer hinter dem Listenende | `rot_fix612c`: "Loesen" am ersten von zwei Anbauten - 1 Zeile mit Index hinter dem Ende (Probe-Zaehler) | Schleife verlassen bei geaenderter Zahl, Grenzpruefungen | `g1`/`g3`: 0 Zeilen (Anbau), 0 Zeilen (Kopfzeile Control Rig > Animation-Rig nur FK, 34 Griffe -> nur FK) (2 OK) |
| P3-8 [N] Q im Gizmo-Zug | `rot_fix612c`: nach Q bei gehaltener Maus und Loslassen Zug offen; Strg+Y bringt die Drehung nicht (Abstand 0,2595) (2 FEHLER) | Zugende unabhaengig vom Gizmo | `g1`/`g3`: Zug zu, Strg+Z zurueck, Strg+Y Abstand 0,000000 (3 OK) |
| P3-9 [N] `[` `]` | `g1` (Bau ohne diese Behebung): im Painting nennt die Suche "Zoom viewport in ([)", obwohl `[` dort den Pinsel stellt (Radius 24 -> 21,6, Abstand bleibt) | nur Texte: Suche und Menue Navigation im Painting | `g3`: Suche ohne `[`-Zeile, mit Strg+=; ausserhalb Painting zoomt `[` (52,32 -> 47,09) und die Suche nennt beide (4 OK) |
| P3-10 [N] Gang-Regler nach Strg+Z | `rot_fix612c`: kein neuer Schritt fuer den Rest des Zugs (oben "FIX612C leer"), das naechste Strg+Z nahm "Put on rig" mit (2 FEHLER) | `rt.presetZug = false` in `modellOffenesAbbrechen` | `g1`/`g3`: Rest ist Schritt "Preset", ein Strg+Z nur dieser (Tempo 90) (2 OK) |
| PV4 Reiter faellt zurueck | `rot_fix612c`: Reiter Hierarchie (2) -> 3 nach Fenster 3840x900 (kleiner geht das Fenster nicht) | `feldEnde` uebernimmt die Belegung nur bei gezeichnetem Bereich | `g1`/`g3`: Reiter bleibt 2 (1 OK) |
| PV5 Vorlagenwechsel | kyle: nicht nachstellbar (Vorlage und Eigenes Skelett je 17 Ketten mit denselben Schluesseln, Zuordnung bleibt). `g1` rancor: Vorlage 18 Ketten, Eigenes Skelett 24 - zurueck: Kette 0 -> 0 statt der von Hand gesetzten 14, ohne Rueckgaengig | Zuordnung je Vorlage gemerkt | `g3`: rancor zurueck Kette 0 -> 14 wie von Hand (2 OK) |

## Was sich aendert

- **P3-5** Der Modus "Animation-Rig (nur FK)" steht jetzt im Dokument (`modell::RigDaten::nurFk`, bisher Zustand der Sitzung
  `ModellAnim::nurFk`, entfaellt; Zugriff `modellRigNurFk()` / `modellRigNurFkSetzen()`). Massstab CAT (IK/FK ohne Sprung, keine toten
  Spuren):
  - Wechsel auf nur FK = ein Rueckgaengig-Schritt: in JEDER Schicht (jede Sequenz der Figur) wird jede IK-Spur mit Schluesseln an jedem
    Bild ihrer Sequenz in FK-Schluessel gebacken (`fkAufIkBereich`; erst alle Spuren backen, dann leeren - so erhaelt es die Pose wie
    "IK backen" je Glied), danach `ik.clear()`. Meldung `AzNurFkGebacken` (Glieder, Sequenzen). Gemessen: Pose jedes Bilds gleich
    (siehe Tabelle, Grenze 0,0001 wie ENT2).
  - Zurueck mit "Control Rig erstellen": ein Schritt fuer den Modus; die Schicht bleibt (FK) wie in ENT2 festgelegt ("eine vorhandene
    Schicht bleibt unberuehrt"); ist sie leer, legt der Knopf wie bisher auf - Auflegen und Modus sind dann ueber das neue
    `Dokument::zusammenlegen()` EIN Schritt.
  - Rueckgaengig/Wiederholen: `rigSchluessel()` enthaelt den Modus, `modellRigAktualisieren` baut die Griffe danach selbst neu (IK-Ziele,
    Pole und die Gruppe IK/FK kommen zurueck bzw. gehen).
  - Die Wahl des Griffs wird nach dem Neuanlegen ueber Art, Seite, Knochen, Nummer und Tag wiedergefunden (bisher Index: ein Pol-Ziel
    wurde nach dem Wechsel zum Saebelgriff); `modellRigAktualisieren` setzt den alten Index nicht mehr zurueck.
  - Sperren, damit im Rig nur mit FK keine IK-Spur entsteht: `ikAendern` (Menue Animation > IK/FK, Gruppe; Meldung `AzIkNurFk`, kein
    Schritt), `modellIkSpur` (Pfad ziehen am Saebel-Tag), "Auf Rig legen" (`neu.ik.clear()`).
  - Tooltip `AzRigFkHint` (i18n_ap10.py) nennt das Backen und Strg+Z. Die Selbsttests ap10/ent2 benutzen `modellRigNurFk()`.
  - **Nachbar:** Zweig `p_klein` hatte `modellAnim().nurFk = false;` in `gui/selbsttest_klein.inc` - das Feld gibt es nicht mehr; dort
    `modellRigNurFkSetzen(false);` einsetzen (wer als Zweiter rebased).
- **P3-6** "Knochen waehlen" wie 3ds Max ist ein Umschalter wie "Abstand messen": Suche (X) schaltet an/aus mit Meldung
  (`BzKnochenAn`/`BzKnochenAus`), Rechtsklick der Ansicht hat den Punkt mit Haken (nur wie 3ds Max; wie Blender steht das Werkzeug in der
  Leiste), oben in der Mitte der aktiven Ansicht steht "Knochen waehlen - Q beendet" (`BzKnochenAnzeige`), Q/W/E/R beenden es, der
  Wechsel von wie Blender zu wie 3ds Max nimmt es nicht unsichtbar mit. Wie Blender unveraendert.
- **P3-7** Szenen-Explorer: aendert ein Befehl im Rechtsklick einer Zeile die Zahl der Anbauten oder Griffe, verlaesst die Schleife die
  Zeilen dieses Bilds (das naechste Bild baut den Baum neu); eine Zeile mit einem Index hinter dem Ende zeichnet nichts;
  `modellRigExplorerSymbol`/`-Hinweis` pruefen die Grenze wie `griffName`.
- **P3-8** Gizmo-Zug am Griff: das Ende haengt nicht mehr am Gizmo - nimmt eine Taste es bei gehaltener Maus weg (wie 3ds Max Q),
  endet der Zug sofort; Strg+Z/Strg+Y wirken danach normal. `modellOffenesAbbrechen` nicht angefasst (Rig-Block wie bisher).
- **P3-9** `[`/`]` wie 3ds Max: im Painting stellen sie den Pinsel (Entscheidung shank), sonst zoomen sie - im selben Bereich wirkt nur
  eine Bedeutung (geprueft je Bereich). Berichtigt nur die Texte: im Painting nennt die Suche das Zoomen nicht mit `[`/`]` (die Zeilen mit
  Strg+= / Strg+- bleiben), das Menue Navigation zeigt dort "Ansicht vergroessern (Strg+=)" / "(Strg+-)". Tooltip der Pinselgroesse nennt
  keine Taste (die steht in der Tastenspalte als `[`/`]`) - stimmt.
- **P3-10** `modellOffenesAbbrechen` setzt auch `rt.presetZug` zurueck (eigener kleiner Block oben, neben den Bloecken von FIX612/FIX612B):
  Strg+Z mitten im Ziehen des Gang-Reglers nimmt dessen Schritt, der Rest des Zugs legt einen eigenen an.
- **PV4** bestaetigt und behoben: `ui::feldEnde` uebernimmt die Belegung der Reiter nur, wenn der rollende Bereich in diesem Bild
  gezeichnet hat (nicht `SkipItems`) - sonst fiel der gewaehlte Reiter (Hierarchie) nach einem sehr kleinen Fenster auf den vorgegebenen.
- **PV5** mit rancor bestaetigt (andere Kettenzahl je Vorlage), mit kyle nicht (gleiche Ketten). Behoben: die Zuordnung der Ketten des Ziels
  gilt je Rig-Vorlage - wechselt die Vorlage auf andere Ketten, bleibt die alte Zuordnung (auch von Hand) gemerkt (je Modell, Quelle,
  Vorlage) und kommt beim Zurueckwechseln wieder. Ohne Rueckgaengig-Schritt wie bisher, weil die Wahl der Vorlage nicht im Dokument steht
  (Frage 3). Bei gleicher Kettenzahl mit anderen Ketten bliebe die Zuordnung stehen - mit den Vorlagen des Spiels nicht gefunden
  (kyle: dieselben Schluessel spine_0, neck_0, head_0 ...).

## Selbsttest `fix612c`

| Lauf | Bau | Ergebnis |
|---|---|---|
| `rot_fix612c` | Test und Proben, ohne Behebung (Basis 73c18113f) | 8 OK, 26 FEHLER - jeder Fund rot ausser P3-9 (Test kam spaeter) und PV5 (kyle nicht nachstellbar) |
| `g1_fix612c` | alle Behebungen ausser P3-9 und PV5 (rebased auf 82848b14e) | 33 OK, 3 FEHLER: P3-9b rot (wie erwartet), PV5 rancor rot (bestaetigt), P3-6c (Testfehler: das Menue der Ansicht nahm den Klick) |
| `g3_fix612c` | mit P3-9 und PV5 | 35 OK, 1 FEHLER (P3-6c, Testfehler wie oben: der erste Klick daneben nahm dem Model-Tab den Fokus) |
| `g4_fix612c` | Test P3-6c mit zwei Klicks daneben | **36 OK, 0 FEHLER** |
| `f_fix612c` | Endstand nach Rebase auf karte1 `499527165` (Bau aller Ziele ohne Warnungen) | **36 OK, 0 FEHLER**; ctest lint_/i18n/modell 32 / 32 |

Der Test prueft beobachtbares Verhalten (Schicht, Griffe, Pose, Auswahl, Schritte des Dokuments, Proben), damit derselbe Test rot und gruen
uebersetzt. Die Proben (`fix612cProbe()`: Zeilen hinter dem Listenende, Bild der Anzeige) bleiben im Programm (zwei Zaehler).

Ausserdem: ctest 259 / 260 (rot war nur `lint_endtable`: ein Lambda mit `return` in der Explorer-Schleife zwischen BeginTable und EndTable -
eine Falschmeldung des Lints, trotzdem ohne Lambda geschrieben; danach lint_endtable ok). Lints: alle wie auf karte1 (pruefcodes 11,
portability 101 - gleich der Basis), i18n ok, i18n_erzeugt ok. Bau ohne Warnungen (/W4 /WX).

## Gruppen, die der geaenderte Code beruehrt

Unter der Lastregel des Hauptchats (Bau `--parallel 2`, hoechstens zwei jkstudio, Leerlauf-Prioritaet), Direct3D 12 (Vorgabe):

| Gruppe | Warum | Ergebnis |
|---|---|---|
| ap10 | Rig erstellen, nur FK (A2), Uebertragen (PV5) | 72 OK, 0 FEHLER (Lauf n1 und n9) |
| ent2 | Control Rig wie CAT, IK/FK, Presets (P3-5, P3-10) | 23 / 0 (n2) |
| ap2 | Szenen-Explorer (P3-7) | 68 / 0 (n2) |
| rig | Control Rig, Griffe, Wahl (P3-5, P3-8) | 28 / 0 (g3) |
| mb6 | IK/FK, Pfade an Gliedern (P3-5 Sperren) | 39 / 0 (n3) |
| rt | Retargeting (PV5) | 21 / 0 (n3) |
| pr | Presets (P3-10) | 17 / 0 (n4) |
| ap3 | rechtes Feld, Reiter (PV4) | 106 / 0 (n4) |
| ap4, ap5 | Aufbau, Ansehen/Pruefen (P3-6) | 106 / 0, 121 / 0 (n5) |
| nav | Navigation, Zoom-Tasten (P3-9) | 134 / 0 (n6) |
| ap9, pt | Painting, Pinsel-Tasten (P3-9) | 98 / 0 (n6), 43 / 0 (n9) |
| blender, tasten, funktionen | Tasten je Bedienmodus, Suche (P3-6, P3-9) | 69 / 0, 45 / 0 (n7), 63 / 0 (n8) |
| ap13k | Rig-Darstellung, Auswahl der sichtbaren Form (neben P3-5/P3-8) | 66 / 0 (n8) |

Bewusst nicht: die volle Runde (laeuft nach dem Zusammenfuehren), Fotovergleich Direct3D 11 - die einzige neue Zeichnung (Hinweis
"Knochen waehlen" in der Ansicht, P3-6) ist eine ImGui-Zeichenliste wie die Messung daneben und geht in beiden Grafikwegen durch denselben
ImGui-Abgeber.

## Handbuch

`tools/handbuch/inhalt_fix612c.py`: zwei Absaetze in sechs Sprachen - `bloecke_animationsfeld()` (nur FK backt die IK, Strg+Z, Control
Rig erstellen danach) hinter den Absatz "rig" der Seite modell-animationsfeld (inhalt_ap10.py), `bloecke_aufbau()` (Knochen waehlen wie
3ds Max als Umschalter) hinter den Absatz ueber Messen der Seite des Aufbaus (inhalt_ap4.py). Beide Seiten haengen noch nicht im Handbuch
(AP14 haengt sie ein) - die Bloecke gehoeren mit dorthin. Keine neuen Bilder (die Aenderungen sind ein Menuepunkt und eine Zeile Text in
der Ansicht).

## Offen / Fragen an shank (bis zur Antwort gilt die Vorgabe)

1. P3-5: Nach "Animation-Rig (nur FK)" wieder "Control Rig erstellen" - sollen die vier Glieder dann automatisch wieder auf IK gehen
   (angeglichen, ohne Sprung)? Vorgabe: nein, die vorhandene Schicht bleibt wie in ENT2 festgelegt; auf IK je Glied mit dem Knopf oder
   mit "Auf Rig legen".
2. P3-6: "Knochen waehlen" wie 3ds Max als Umschalter (Suche, Rechtsklick mit Haken, Anzeige, Q beendet) - oder dort ganz weglassen?
   Vorgabe: Umschalter wie "Abstand messen".
3. PV5: Reicht es, die Zuordnung der Ketten je Vorlage zu merken (so umgesetzt), oder soll die Rig-Vorlage ins Dokument (dann nimmt
   Rueckgaengig den Wechsel mit)?

Beobachtet im Test (nicht geaendert): das Menue der Ansicht (Rechtsklick) schliesst ein Esc des Selbsttests nicht; der erste Klick
daneben schliesst es und nimmt dem Model-Tab den Tastaturfokus. Der Test klickt darum zweimal (wie `rig3::ueberSuche`).

## Changelog

## English

- Model tab, Animation: Animation rig (FK only) after Create control rig bakes the IK of every limb into FK keys first - the motion stays exactly the same, every rotation control turns its bone, and Ctrl+Z brings the IK targets back in one step.
- Model tab like 3ds Max: Select bone is a switch like Measure distance - in Search all actions and the context menu (with a check mark), a hint at the top of the view while it is on; Q, W, E and R end it.
- Model tab: fixed a gizmo drag on a rig control that stayed open when Q removed the gizmo; fixed Ctrl+Z while dragging the speed of a generated walk; fixed the Scene Explorer drawing rows after detaching an attachment; the chosen tab of the right panel stays after a very small window.
- Model tab like 3ds Max, Painting: the search and the Navigation menu name Ctrl+= and Ctrl+- for zooming there, because [ and ] set the brush size.

## Deutsch

- Model-Tab, Animation: Animation-Rig (nur FK) nach Control Rig erstellen backt die IK jedes Glieds zuerst in FK-Schlüssel - die Bewegung bleibt genau gleich, jeder Dreh-Griff dreht seinen Knochen, und Strg+Z holt die IK-Ziele in einem Schritt zurück.
- Model-Tab wie 3ds Max: Knochen wählen ist ein Schalter wie Abstand messen - in Alle Aktionen suchen und im Rechtsklick (mit Haken), oben in der Ansicht ein Hinweis, solange es an ist; Q, W, E und R beenden es.
- Model-Tab: behoben, dass ein Gizmo-Zug an einem Griff offen blieb, wenn Q das Gizmo wegnahm; Strg+Z beim Ziehen des Tempos eines erzeugten Gangs; der Szenen-Explorer zeichnete nach dem Lösen eines Anbaus Zeilen hinter dem Listenende; der gewählte Reiter des rechten Felds bleibt nach einem sehr kleinen Fenster.
- Model-Tab wie 3ds Max, Painting: Suche und Menü Navigation nennen dort Strg+= und Strg+- zum Zoomen, weil [ und ] die Pinselgröße stellen.

## 中文

- 模型选项卡, 动画: 在“创建控制绑定”之后使用“动画绑定 (仅 FK)”时, 会先把每个肢体的 IK 烘焙为 FK 关键帧 - 动作完全不变, 每个旋转控制器都能旋转其骨骼, Ctrl+Z 一步恢复 IK 目标。
- 模型选项卡 (像 3ds Max): “选择骨骼”与“测量距离”一样是开关 - 位于“搜索所有操作”和右键菜单 (带勾选标记), 开启期间视图顶部显示提示; Q、W、E、R 结束它。
- 模型选项卡: 修复了 Q 移除操纵器时绑定控制器上的拖动不结束; 修复了拖动生成步态速度时按 Ctrl+Z; 修复了分离附加物后场景资源管理器绘制超出列表末尾的行; 窗口很小之后右侧面板所选的选项卡保持不变。
- 模型选项卡 (像 3ds Max), 绘制: 搜索和“导航”菜单在此处以 Ctrl+= 和 Ctrl+- 表示缩放, 因为 [ 和 ] 用于设置画笔大小。

## 日本語

- モデルタブ、アニメーション: 「コントロールリグを作成」の後の「アニメーションリグ (FK のみ)」は、各手足の IK を先に FK キーへベイクします。動きはまったく変わらず、各回転コントロールがボーンを回し、Ctrl+Z で IK ターゲットが 1 ステップで戻ります。
- モデルタブ (3ds Max 風): 「ボーンを選択」は「距離を測定」と同じスイッチです。「すべてのアクションを検索」と右クリックメニュー (チェックマーク付き) にあり、オンの間はビューの上部にヒントが出ます。Q、W、E、R で終了します。
- モデルタブ: Q でギズモが消えたときにリグコントロールのドラッグが終わらない問題、生成した歩行の速度をドラッグ中に Ctrl+Z を押したときの問題、取り付けを外した後にシーンエクスプローラーがリストの末尾を越えて行を描く問題を修正しました。ウィンドウをとても小さくした後も右パネルで選んだタブが保たれます。
- モデルタブ (3ds Max 風)、ペイント: [ と ] がブラシサイズを設定するため、ここでは検索とナビゲーションメニューがズームに Ctrl+= と Ctrl+- を示します。

## Español

- Pestaña Model, Animación: Rig de animación (solo FK) después de Crear rig de control hornea primero el IK de cada extremidad en claves FK; el movimiento queda exactamente igual, cada control de giro gira su hueso y Ctrl+Z recupera los objetivos IK en un paso.
- Pestaña Model como 3ds Max: Seleccionar hueso es un interruptor como Medir distancia - en Buscar todas las acciones y en el menú contextual (con marca), con un aviso arriba en la vista mientras está activo; Q, W, E y R lo terminan.
- Pestaña Model: corregido que un arrastre del gizmo en un control del rig quedara abierto cuando Q quitaba el gizmo; Ctrl+Z al arrastrar la velocidad de un paso generado; el Explorador de escena dibujaba filas tras el final de la lista al soltar un acople; la pestaña elegida del panel derecho se mantiene tras una ventana muy pequeña.
- Pestaña Model como 3ds Max, Pintura: la búsqueda y el menú Navegación indican allí Ctrl+= y Ctrl+- para el zoom, porque [ y ] ajustan el tamaño del pincel.

## Čeština

- Karta Model, Animace: Animační rig (jen FK) po Vytvořit ovládací rig nejdřív zapeče IK každé končetiny do klíčů FK - pohyb zůstane přesně stejný, každý otočný ovladač otáčí svou kostí a Ctrl+Z vrátí cíle IK jedním krokem.
- Karta Model jako 3ds Max: Vybrat kost je přepínač jako Změřit vzdálenost - ve Vyhledat všechny akce a v místní nabídce (se zaškrtnutím), nahoře v pohledu je nápověda, dokud je zapnutý; Q, W, E a R ho ukončí.
- Karta Model: opraveno, že tažení gizma na ovladači rigu zůstalo otevřené, když Q gizmo odebralo; Ctrl+Z při tažení rychlosti generované chůze; Průzkumník scény po odpojení připojeného modelu kreslil řádky za koncem seznamu; zvolená karta pravého panelu zůstane i po velmi malém okně.
- Karta Model jako 3ds Max, Malování: vyhledávání a nabídka Navigace tam pro přiblížení uvádějí Ctrl+= a Ctrl+-, protože [ a ] nastavují velikost štětce.
