# Notizen ENTSCHEID2 (12.11.): verbindliche Entscheidungen von shank zu AP10 im Bereich Animation des Model-Tabs - rc612

Auftrag ENTSCHEID2 (Opus 5.5, ein Agent): „ENTSCHEIDUNGEN shank 12.11. AP10 … ENTSCHEID2“ aus `naechste_auftraege.md`. Worktree `Y:\p_ent2`,
Zweig `p_ent2` auf karte1 `2bb34d874`. Selbsttest neu `gui/selbsttest_ent2.inc` (`BHED_EDITORTEST=modellblock`, `BHED_MODELL_NUR=ent2 | ent2foto`),
angepasst `ap10`; Skripte, Läufe, Max-Auslese `Y:\werkzeuge\ent2\`.

## Was sich ändert

| Punkt | Was | Vorbild / Quelle | Datei |
|---|---|---|---|
| 1a FK → IK beim Erstellen | **Control Rig erstellen** legt eine Animation der Sequenz gleich auf das Rig (wie bisher nur „Auf Rig legen“): die Bilder werden Schlüssel an den Griffen (`rigReduzieren`), je Glied an **jedem Bild ein IK-Schlüssel mit Anteil 1** (Ziel auf Hand/Fuß der FK-Pose, Pol in der Ebene der Kette, `ikAufFkBereich`) und die **FK-Schlüssel der Kette an jedem Bild** (`ikKetteJedesBild`, neu). Die Meldung nennt die größte Abweichung. Eine Sequenz mit schon vorhandener Schicht bleibt unberührt (Neu erkennen überschreibt nichts); „Auf Rig legen“ bleibt für später geladene Animationen und tut dasselbe. Auch der Knopf „Control Rig erstellen“ der Rig-Übersicht und die Aktion (Suche, Menü, Rechtsklick) gehen diesen Weg. Animation-Rig (nur FK) legt nichts automatisch auf | CAT: alle vier Glieder der ausgelesenen Szene stehen auf IK (`layerIKFKRatio` = 0,0 = IK), die IK-Ziele/Plattformen tragen die Bewegung (siehe CAT-Belege) | `modellAnimRigErstellen` (`app_modelltab_anim.cpp`), `modellAnimAufRigLegen` (`app_modelltab_kurven.cpp`), `ikKetteJedesBild` (`src/modell/ik.cpp`) |
| 1b Griffe wie CAT | danach arbeitet man an Hand-/Fuß-IK-Ziel (Fußplatte), Pol (Ellbogen/Knie) und Fußrolle an jedem Bild - nur dieses Bild ändert sich (dafür die dichten FK-Schlüssel der Kette: vorher verschob ein Griff die Nachbarbilder um 2,34 Einheiten, weil das Auto-Einrasten einen FK-Schlüssel in eine reduzierte FK-Kurve setzte, auf der die IK aufsetzt). Im Bedienmodus wie Blender greift G das IK-Ziel jetzt sofort (vorher lag das Glied nach „Auf Rig legen“ auf FK, G meldete „nur bei IK“) | CAT-Hilfe „CAT's IK System“: „In IK the child end of the limb … always tries to follow the IK target“; „Limb IK Targets“: FootPlatform für Beine, Punkt für Arme | unverändert `ikZielSetzen`, `ikPolSetzen`, `ikRolleSetzen` |
| 1c IK/FK ohne Sprung | der Knopf IK/FK je Glied (wie 3ds Max) und die Aktionen „Auf IK“/„Auf FK“ schalten **ab dem Bild bis zum nächsten Wechsel** um - wie ein Schlüssel der IK/FK-Spur eines CAT-Glieds, der bis zum nächsten gilt - und gleichen an jedem dieser Bilder vorher die andere Seite an (nach FK: FK-Schlüssel auf die sichtbare IK-Pose; nach IK: Ziel und Pol auf die FK-Pose). Bisher wirkte der Wechsel nach dem Auflegen (ein Schlüssel je Bild) nur auf ein Bild. Tooltip neu. Regler und Einrast-Knöpfe wie Blender (Rigify) unverändert | CAT-Hilfe „Limb Animation Controls“: IK/FK-Regler 0 = IK, 1 = FK, „Match IK and FK … seamless blend“ mit Auto Key; Rigify-Snap | `ikFkUmschaltenAb` (neu, `ik.cpp`), `modellIkWechselnRoh` (`app_modelltab_ik.cpp`) |
| 2 Preset-Kategorien | die **acht des Entwurfs** in seiner Folge immer (Stehen, Gehen, Laufen, Rückwärts, Ducken, Springen, Kampf, Gesten), dahinter, was `presets.txt` sonst nennt (**Sonstiges**). Leere Kategorie: kurzer Hinweis statt Liste („noch keine Presets … nur mit freier Lizenz … eigene über Animation importieren“) | Entwurf A3 (`entwurf_modelltab.html`: `["Stehen","Gehen","Laufen","Rückwärts","Ducken","Springen","Kampf","Gesten"]`) | `presetKategorien`, `drawModellPresetBereich` (`app_modelltab_retarget.cpp`) |
| 3 Generator-Gänge in der Liste | in Gehen und Laufen steht oben **Gehen (erzeugt)** bzw. **Laufen (erzeugt)** (Quelle „Generator“). Gewählt stehen **Tempo** und **Armschwung** unter der Liste (nach dem Ziel-Platz, wie der Entwurf), Vorschau auf Figur und Auf Figur legen (Retarget) wirken wie bei Presets (Platz BOTH_WALK1/BOTH_RUN1 als Vorschlag). **Generator …** zeigt den Rest unter dem Hauptknopf (Haltung, Schrittlänge, Hüftschwung, Hub, Messen, Befund) und rollt ihn ins Bild; ohne gewählten Gang wählt der Knopf den der Kategorie. Rechtsklick auf einen Eintrag: Vorschau auf Figur, Auf Figur legen, Messen, Generator …; neue Aktion „Erzeugten Gang in der Preset-Liste wählen“ (Suche, Menü Animation, Rechtsklick der Ansicht). Die Wahl Gehen/Laufen im alten Generator-Feld entfällt (die Einträge ersetzen sie) | Entwurf A3: Eintrag „Gehen – Säbel gezogen · Generator“, Regler Tempo und Armschwung unter der Liste | `modellPresetGangWaehlen` (neu), `gangRegler`, `gangMehr`, Aktion in `app_modelltab_rig.cpp` |

Zuordnung der vorhandenen Presets (`presets.txt` bleibt die Quelle, unverändert): Stehen = idle (Idle, Idle (sword), Crouch idle), Gehen = walk (Walk,
Walk (formal), vier 100STYLE-Gänge, Crouch walk) + Gehen (erzeugt), Laufen = run (Jog, Sprint) + Laufen (erzeugt), Kampf = combat (Sword attack,
Punch jab, Punch cross), Sonstiges = other (Sitting idle, Death). Leer: Rückwärts, Ducken, Springen, Gesten. Crouch idle/Crouch walk stehen laut
Datei in Stehen/Gehen, nicht in Ducken (Frage an shank).

Texte in `tools/i18n_ent2.py` (Präfix `En`, 6 Sprachen); geändert in `i18n_ap10.py`: `AzRigErstellenHint`, `AzAufRigHint`, `AzIkTippMax`.
Entfernt, weil nach ENT2 unbenutzt (lint_i18n): `AzAufRigMeldung` (→ `EnAufRigMeldung` mit Abweichung), `AzGeneratorHint` (→ `EnGeneratorHint`),
`MbIkAnHint` (→ `EnIkKnopfHint`), `PrKategorie`, `PrKategorieHint`, `PrGehen`, `PrLaufen` (die Einträge der Liste ersetzen die Wahl). lint_i18n ok.

## CAT-Verhalten belegt

- **Autodesk-Hilfe** (3ds Max 2017, CAT): „Limb Animation Controls“ (https://help.autodesk.com/cloudhelp/2017/ENU/3DSMax/files/GUID-621BFF43-1BA3-4FBA-86B5-2558EAA95C5B.htm):
  IK/FK-Regler „0.0 being IK and 1.0 being FK“; „Match IK and FK: Creates a seamless blend between IK and FK modes. Click this button with Auto
  Key on before you blend to or from IK“; „Move IK Target to Palm“ für den Weg zurück nach IK. „CAT's IK System“
  (…/GUID-1D80FD75-5175-4748-909C-BEB2514DCAA9.htm): „takes the current FK rotations of a limb and then fits them to the IK target“, „In IK the
  child end of the limb … always tries to follow the IK target“. „Limb IK Targets“ (3ds Max 2012, GUID-C7120A31-…-1348): IK-Ziel der Beine ist die
  FootPlatform (Drehpunkt animierbar), der Arme ein Punkt. MAXScript „LimbFPInterface“ (2016, GUID-B69FF2D2-…): IKTarget, UpNode, Palm,
  MoveIKTargetToEndOfLimb.
- **Echtes Rig** (`Special move zetzel.max`, 3dsmaxbatch 2027 im Hintergrund, Skript `Y:\werkzeuge\ent2\cat_ikfk_lesen.ms`, Ergebnis
  `cat_ikfk.txt`; nur gelesen, nichts gespeichert; ADSL-Meldung wie bekannt): 4 Glieder (`CATLimbData2`: LLeg, RLeg, LArm, RArm), je Glied die
  Spuren `layerIKFKRatio` (LayerFloat mit `bezier_float` - schlüsselbar, ein Schlüssel, Wert **0,0 = IK**), `layerLimbIKPos` 3,0,
  `layerRetargeting`; IKTarget der Beine `JK_humanoidLLegPlatform`/`RLegPlatform`, der Arme `LArmIKTarget`/`RArm`, UpNode (Pol)
  `…UpVectorNode` an Hüfte bzw. Brust. Abstand Ende des Glieds (Palm) zum IK-Ziel an Bild 1, 45,5 und 90 gleich (Beine 7,12, Arme 4,00): das
  Glied folgt seinem Ziel über die ganze Animation - die Bewegung liegt in den Zielen.
- **Abweichung von CAT (bewusst):** CAT gleicht beim Umschalten nur am aktuellen Bild an; hier an jedem Bild des umgeschalteten Abschnitts
  (nach dem Auflegen hat jedes Bild einen Schlüssel - sonst sähe man an den folgenden Bildern einen Sprung). Das **IK/FK-Verhältnis** ist schon
  heute je Glied 0-1 und schlüsselbar (Regler wie Blender, Anteil im IK-Schlüssel), gilt aber **bis zum nächsten Schlüssel** statt zwischen
  zwei Schlüsseln zu überblenden wie CATs Bezier-Spur - siehe Fragen.

## FK → IK: gemessene Abweichung (kyle, BOTH_WALK1, 30 Bilder, Lauf `ent2g2`)

Je Glied der größte Ortsunterschied eines Knochens (Kette mit Hand/Fuß, Fingern, Zehen) über alle Bilder:

| Glied | gegen die Animation ohne Rig | davon die IK allein (gegen dieselben Schlüssel als FK) |
|---|---|---|
| Arm L | 0,0575 | 0,00002 |
| Arm R | 0,0475 | 0,00002 |
| Bein L | 0,0255 | 0,00001 |
| Bein R | 0,0339 | 0,00001 |

Begründung der Grenzen (Selbsttest `ENT2 1 Abweichung`): gesamt < 1/16 Einheit wie „Auf Rig legen“ in AP10 (rigReduzieren hält jedes
Knochen-Bild in einer Stufe der gepackten Verschiebung, 1/64 je Achse, entlang der Kette bis 8/64) - die Abweichung ist die Reduktion, nicht die
IK. IK allein < 0,0001: float hat bei Orten von 32-64 Einheiten einen Schritt von 2^-18 ≈ 0,0000038, die Kette rechnet drei Drehungen und die
Lösung nacheinander (~26 Schritte Spielraum). Umschalten IK↔FK: Sprung gemessen 0,00002 / 0,00001 (Grenze ebenso 0,0001; vor den dichten
FK-Schlüsseln 0,0085).

## Selbsttests (rot → grün)

| Test | rot (Bau `bau_rot` = karte1 + Kern + Texte + Tests, Oberfläche noch alt) | grün |
|---|---|---|
| `ent2` (neu, 22 Prüfungen: FK→IK beim Erstellen, Abweichung je Glied, Hand an zwei Bildern, Pol, Fuß/Fußplatte, Fußrolle, IK/FK ab Bild hin und zurück ohne Sprung, Neu erkennen überschreibt nicht, acht Kategorien, leer mit Hinweis, Sonstiges, Gehen/Laufen (erzeugt), Regler unter der Liste, Tempo ziehen, Vorschau, Hauptknopf, Generator …, Rechtsklick, Aktionssuche) | 2 OK, **26 FEHLER** (`Y:\werkzeuge\ent2\rot_ent2_protokoll.txt`; die Griff-Prüfungen sind rot, weil ohne Auflegen keine Schicht da ist) | 22 / 0 (`ent2n1`, `ent2n2`) |
| `ap10` (angepasst, s. u.) | 65 OK, **6 FEHLER** (`rot_ap10_protokoll.txt`) | 71 / 0 |

Funde der Läufe (je rot nachgewiesen, behoben, grün):
- `ent2g1`: Griffe an Bild +6/+14 änderten die Nachbarbilder um **2,34** (Hand) bzw. **1,17** Einheiten (Pol/Fuß) → FK-Schlüssel der Kette an jedem
  Bild (`ikKetteJedesBild`) → `ent2g2` 0,00000; Sprung beim Umschalten 0,0085 → 0,00002.
- `ent2g1`: Gehen (erzeugt) stand als neunter Eintrag unter sechs sichtbaren Zeilen (nicht zu sehen) → oben in der Liste.
- `ent2g1`/`ap10`: Generator … klappte den Rest über sich auf und schob Knopf und Hauptknopf aus dem Bild → Rest unter dem Hauptknopf, beim
  Aufklappen ins Bild gerollt.
- Nachbargruppe `rig` (`ent2n1_rig` 23 OK / **4 FEHLER**): Bindepose, gespiegelt einfügen und Handpose schrieben FK-Schlüssel, die IK der
  aufgelegten Glieder verdeckte sie → nach diesen Posen folgen Ziel und Pol der neuen FK-Pose (`ikFolgtFk`, `app_modelltab_rig.cpp`) →
  `ent2n2_rig` 27 / 0. Der vierte Fehler („Strg+Z: keine Schlüssel“) war die Annahme des Tests, s. u.
- Nachbargruppe `pr` (15 / **1**): die Gruppe Presets hat eine Aktion mehr (Erzeugten Gang wählen) → Test zählt 5 statt 4 → 16 / 0.

## Angepasste vorhandene Selbsttests (nichts abgeschwächt)

- `ap10` A6 (Max): nach Control Rig erstellen stehen die Knöpfe auf **IK** (vorher FK); der Klick auf Arm L wechselt ab dem Bild auf **FK** und
  prüft zusätzlich „kein Sprung“ (ikSprung < 0,01) - Grund: Entscheidung 1 (die Animation liegt von FK auf IK auf dem Rig).
- `ap10` A7 (Blender): der Regler steht auf IK; nach rechts ziehen stellt FK ein (vorher von FK nach IK).
- `ap10` A2 „Auf Rig legen“: erwartet Anteil 1 an **jedem** Bild (vorher Anteil 0 an einem Bild); Rückgängig stellt die Schlüssel **von vorher**
  her (die Schicht aus Control Rig erstellen bleibt, vorher 0 Schlüssel).
- `ap10` A4: nach zwei Schritten Rückgängig wieder die Schlüssel von vorher (vorher 0) - gleicher Grund.
- `ap10` A3 Laufen: die Kurzinfo zählt Laufen (erzeugt) mit; A3 Generator …: der erzeugte Gang ist gewählt, der Rest unter dem Hauptknopf ins
  Bild gerollt, Tempo/Armschwung unter der Liste über dem Hauptknopf (eigene Zeile; vorher „Regler klappen unter der Bibliothek auf“).
- `mb3` (Gruppe `rig`) „MB4 Drehen: Strg+Z“: erwartet nach Rückgängig die Schlüssel von vorher statt 0 - das Rig entsteht dort über die Suche
  (Control Rig erstellen) und legt jetzt die Sequenz auf.
- `rt` (Gruppe `pr`) „PR Menü Animation > Presets“: 5 Aktionen statt 4 (neue Aktion Erzeugten Gang wählen).
- Handbuchschritte unverändert; `h_pr_gang` zeigt jetzt Gehen (erzeugt) gewählt mit Tempo/Armschwung.

## Fotos

`Y:\werkzeuge\ent2\fotos\ent2_f_ent2_<variante>\` (Selbsttest `ent2foto`, je Lauf 0 FEHLER): en, de, ja, hell (3818 × 2104 bei Oberfläche 1,5),
klein (1328 × 844 bei 1,0); je Bedienmodus `ikfk`, `gang`, `mehr`, `leer`. Angesehen: en Max (ikfk, gang, mehr, leer), de Max gang, hell Max gang,
ja Blender ikfk, klein Max gang; neben dem Entwurf `vergleich\v_en_max_presets.png` (das Entwurfsbild zeigt die Gruppe Presets nicht - Vergleich
gegen den Entwurf-Quelltext: Kategorien in zwei Spalten, Liste mit Quelle, Ziel-Platz, Tempo/Armschwung, Vorschau | Generator, Hauptknopf, Tipp
in derselben Folge). Befund: nichts abgeschnitten (de/ja), helles Thema lesbar; Mindestgröße zeigt wie in AP10 bekannt nur den Kopf des Felds
(Zeitleiste ~40 % der Höhe, AP1/AP3/AP14).

## Handbuch

- `inhalt_ap10.py` (noch nicht eingehängt, AP14): Rig erstellen (FK → IK wie CAT, Griffe, Auf Rig legen), Presets (acht Kategorien, Hinweis,
  erzeugte Gänge, Regler, Generator …, Rechtsklick), IK/FK (Knopf ab dem Frame mit Match IK and FK) - 6 Sprachen; Platzhalter alle vorhanden.
- eingehängt: `inhalt_rt.py` „Presets: Geh- und Laufgenerator“ (über die Einträge der Liste; der veraltete Satz „Bibliotheken liegen noch nicht
  bei“ ersetzt), `inhalt_animimp.py` Preset-Bibliothek (Kategorien, Hinweis, erzeugte Gänge) - 6 Sprachen; Handbuch neu erzeugt.
- Bilder neu in 6 Sprachen: `h_ap10_max`, `h_ap10_blender` (Rig mit der Animation auf IK, Meldung mit Abweichung), `h_pr_gang` (Gehen (erzeugt)
  gewählt), `h_ai_bibliothek` (acht Kategorien); angesehen en `h_pr_gang`, ja `h_ap10_blender`.
- `gen_handbuch.py --pruefen` ok (Hinweis „2 Einträge in uebersetzung/“ wie auf karte1).

## Schlussrunde des Zweigs

| Schritt | Ergebnis |
|---|---|
| Bau alle Ziele (inkrementell, /W4 /WX) | 0 Fehler, 0 Warnungen (`bau_alles`) |
| ctest | 258 / 258 |
| `ent2` | 22 / 0 (rot 2 / 26) |
| `ap10` | 71 / 0 (rot 65 / 6) |
| Nachbargruppen (`n1`, Nachlauf `n2` nach den Funden) | ap1 142 · ap3 105 · ap4 105 · ap5 120 · mb3 22 · mb5 160 · mb6 38 · mb7 25 · rig 27 · rt 20 · rtjka 2 · pr 16 · ai 32 · ph 30 · pfix 18 · blender 68 · funktionen 62 · tasten 44 - je 0 FEHLER |
| Fotos `ent2foto` | 5 Varianten, je 0 FEHLER |
| Handbuchbilder | ap10, rt, ai in 6 Sprachen, je 0 FEHLER |
| Lints (`lints.sh`, 25) | ok bzw. wie karte1: pruefcodes 11, portability 99, efxflags und fremdprogramm dieselbe Meldung |

Bewusst nicht gelaufen (volle Runde nur auf dem zusammengeführten Stand): Modus 1, übrige modellblock-Gruppen, karteneditor, menueeditor,
sauberer Neubau, Sprachläufe. Keine Bildzeit-Messung: nach dem Erstellen rechnet die Vorschau je Bild die geschlossene Zwei-Glied-IK
für vier Glieder (vorher lagen sie nach „Auf Rig legen“ auf Anteil 0 und wurden übersprungen) - ungemessen, siehe Offen.

## Offen / Fragen an shank

1. IK/FK-Verhältnis zwischen zwei Schlüsseln **überblenden** wie CAT (Bezier-Spur je Glied) statt „gilt bis zum nächsten Schlüssel“? Heute steckt
   der Anteil im IK-Schlüssel (eine Spur für Ziel, Pol, Rolle, Raum und Anteil); Überblenden hieße eine eigene Spur je Glied - neues
   Dateiformat (Missions-Datei `figuranim`, Dopesheet) und ein anderer Wechsel im Mission-Tab. Nicht ohne Rückfrage gebaut.
2. Crouch idle / Crouch walk in die Kategorie Ducken umziehen (heute laut `presets.txt` in Stehen/Gehen, Ducken bleibt leer)?
3. Tempo zeigt Einheiten je Sekunde (90 Gehen, 300 Laufen wie die NPCs des Spiels), der Entwurf einen Faktor (1,0×) - so lassen?
- Die Meldung beim Umschalten nennt die Bilder der GLA (wie die übrigen IK-Meldungen), nicht die Zählung der Zeitleiste.
- Rig-Darstellung (CAT-Kästen, Fußplatte als Rechteck) macht AP13.
- Bildzeit mit aufgelegtem Rig (IK an vier Gliedern je Bild) nicht gemessen - für die Messrunde.

## Changelog

## English

- Model tab, Animation: like a CAT rig, Create control rig puts an animation of the sequence onto the rig from FK to IK - hands and feet follow their IK targets and poles, the motion looks the same (the message names the largest deviation). Then you work on the controls (hand or foot target, pole, foot roll) at any frame; only that frame changes.
- IK/FK per limb like CAT: the button switches the limb from the current frame up to the next switch and matches the other side first - nothing jumps.
- Presets show the eight categories of the design (Idle, Walk, Run, Backwards, Crouch, Jump, Combat, Gestures, then Other); a category without presets yet shows a short note.
- The own generator stands in the list as Walk (generated) and Run (generated): speed and arm swing under the list, Preview on figure and Put on figure as for presets, Generator... for the rest; right-click on an entry offers its commands.

## Deutsch

- Model-Tab, Animation: wie bei einem CAT-Rig legt Control Rig erstellen eine Animation der Sequenz von FK auf IK auf das Rig - Hände und Füße folgen ihren IK-Zielen und Polen, die Bewegung sieht gleich aus (die Meldung nennt die größte Abweichung). Danach arbeitet man an den Griffen (Hand- oder Fußziel, Pol, Fußrolle) an jedem Frame; nur dieser Frame ändert sich.
- IK/FK je Glied wie CAT: der Knopf schaltet das Glied ab dem aktuellen Frame bis zum nächsten Wechsel um und gleicht vorher die andere Seite an - nichts springt.
- Presets zeigen die acht Kategorien des Entwurfs (Stehen, Gehen, Laufen, Rückwärts, Ducken, Springen, Kampf, Gesten, dahinter Sonstiges); eine Kategorie ohne Presets zeigt einen kurzen Hinweis.
- Der eigene Generator steht als Gehen (erzeugt) und Laufen (erzeugt) in der Liste: Tempo und Armschwung unter der Liste, Vorschau auf Figur und Auf Figur legen wie bei Presets, Generator ... für den Rest; der Rechtsklick auf einen Eintrag bietet seine Befehle.

## 中文

- 模型选项卡的动画: 与 CAT 绑定相同, “创建控制绑定”会把序列的动画从 FK 转到 IK 放到绑定上 - 手脚跟随其 IK 目标和极向, 动作看起来不变 (消息会给出最大偏差)。之后可在任意帧上操作控制器 (手或脚的目标、极向、脚部滚动); 只有该帧会改变。
- 与 CAT 相同的每个肢体 IK/FK: 按钮从当前帧起直到下一次切换, 切换此肢体, 并先匹配另一侧 - 不会跳变。
- 预设显示设计中的八个类别 (站立、行走、跑步、后退、蹲伏、跳跃、战斗、手势, 其后是其他); 还没有预设的类别会显示简短提示。
- 自带生成器以“行走 (生成)”和“跑步 (生成)”出现在列表中: 速度和摆臂在列表下方, “在角色上预览”和“应用到角色”与预设相同, 其余设置在“生成器...”中; 右键单击条目会提供其命令。

## 日本語

- モデルタブのアニメーション: CAT リグと同じく、「コントロールリグを作成」がシーケンスのアニメーションを FK から IK に移してリグに載せます - 手足は IK ターゲットとポールに追従し、動きは同じに見えます (メッセージに最大のずれを表示)。その後、どのフレームでもコントロール (手や足のターゲット、ポール、フットロール) を操作でき、変わるのはそのフレームだけです。
- CAT と同じ手足ごとの IK/FK: ボタンは現在のフレームから次の切り替えまで手足を切り替え、先にもう一方を合わせます - ジャンプしません。
- プリセットはデザインの 8 つのカテゴリ (待機、歩き、走り、後退、しゃがみ、ジャンプ、戦闘、ジェスチャー、その後ろにその他) を表示します。まだプリセットのないカテゴリには短い案内が出ます。
- 独自ジェネレーターは「歩き (生成)」「走り (生成)」として一覧に並びます: 速度と腕の振りは一覧の下、「フィギュアでプレビュー」と「フィギュアに載せる」はプリセットと同じ、その他は「ジェネレーター...」。項目の右クリックでそのコマンドを選べます。

## Español

- Pestaña Model, Animación: como un rig de CAT, Crear rig de control pone la animación de la secuencia en el rig de FK a IK - manos y pies siguen sus objetivos IK y polos, el movimiento se ve igual (el mensaje da la mayor desviación). Después se trabaja con los controles (objetivo de mano o pie, polo, giro del pie) en cualquier fotograma; solo cambia ese fotograma.
- IK/FK por extremidad como CAT: el botón cambia la extremidad desde el fotograma actual hasta el siguiente cambio e iguala antes el otro lado - nada salta.
- Los presets muestran las ocho categorías del diseño (Reposo, Andar, Correr, Hacia atrás, Agachado, Salto, Combate, Gestos y detrás Otros); una categoría aún sin presets muestra una nota breve.
- El generador propio está en la lista como Andar (generado) y Correr (generado): velocidad y balanceo de brazos bajo la lista, Vista previa en la figura y Poner en la figura como en los presets, Generador... para el resto; el clic derecho en una entrada ofrece sus órdenes.

## Čeština

- Karta Model, Animace: jako rig CAT položí Vytvořit ovládací rig animaci sekvence na rig z FK na IK - ruce a nohy sledují své cíle IK a póly, pohyb vypadá stejně (zpráva uvede největší odchylku). Pak se na kterémkoli snímku pracuje s ovladači (cíl ruky nebo nohy, pól, odval chodidla); změní se jen tento snímek.
- IK/FK pro každou končetinu jako CAT: tlačítko přepne končetinu od aktuálního snímku do další změny a předtím srovná druhou stranu - nic neskočí.
- Předvolby ukazují osm kategorií návrhu (Stání, Chůze, Běh, Pozpátku, Přikrčení, Skok, Boj, Gesta, za nimi Ostatní); kategorie zatím bez předvoleb ukáže krátkou poznámku.
- Vlastní generátor je v seznamu jako Chůze (generovaná) a Běh (generovaný): rychlost a švih paží pod seznamem, Náhled na postavě a Položit na postavu jako u předvoleb, Generátor... pro zbytek; pravé tlačítko na položce nabídne její příkazy.
