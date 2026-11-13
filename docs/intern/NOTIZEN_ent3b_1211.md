# Notizen ENTSCHEID3B (12./13.11.): B14-B17, C1, C3 - IK/FK-Spur wie CAT, Ducken, Tempo als Faktor, Griff der Wurzel, nur FK zurueck auf IK, Rig-Vorlage im Dokument - rc612

Auftrag ENTSCHEID3B (Opus 5.5; am Wochenlimit 12.11. ~23:15 gestoppt, 13.11. fortgesetzt): `plaene/entscheidungen_1211.md` B14-B17, C1, C3
(verbindlich). Worktree `Y:\p_ent2`, Zweig `p_ent3b`, begonnen auf karte1 `fe0cbece7`, rebased auf karte1 `3d0947208` (mit DXR, AP14, KLEIN). Selbsttest neu `gui/selbsttest_ent3b.inc` (`BHED_EDITORTEST=modellblock`,
`BHED_MODELL_NUR=ent3b`; Handbuch `BHED_EDITORTEST=handbuch`, `BHED_HB_NUR=ent3b`), Proben `gui/app_modelltab_ent3b.h/.cpp` (Text des Reglers Tempo,
Zeilen des Dopesheets), ctest neu `modell_ent3b` mit `tests/figuranim/ent3b_figur_alt.txt` (geschrieben vom Programm vor ENTSCHEID3B). Texte
`tools/i18n_ent3b.py` (Praefix `E3`, 6 Sprachen), Handbuch `tools/handbuch/inhalt_ent3b.py`. Skripte, Patches, Laeufe `Y:\werkzeuge\ent3b\`.

## Was sich aendert

| Punkt | Was | Vorbild / Quelle | Dateien |
|---|---|---|---|
| B14 IK/FK ueberblenden | jedes Glied hat eine eigene **IK/FK-Spur** (`IkSpur::anteil`, Schluessel wie die Schicht, Wert = Anteil IK im Kanal t[0]); zwischen zwei Schluesseln blendet das Glied ueber (Interpolation des linken Schluessels: Bezier/Auto Clamped wie Max und Blender, Linear, Konstant). Im Dopesheet je Glied eine Zeile **IK/FK** (Model-Tab und Mission-Tab): anklicken, ziehen, loeschen, Interpolation - wie jeder Schluessel (Code `ikAnteilCode`, Liste ueber `schluesselListe`). Wie Blender setzt der Regler des Glieds einen Schluessel am Frame (CAT-Regler mit Auto Key); wie 3ds Max neu der Regler **IK/FK-Verhaeltnis** unter dem gewaehlten Glied (CAT Limb Animation: 0 = IK, 1 = FK); der Zustand zeigt waehrend des Ueberblendens „IK n % · FK m %“. Der Knopf IK/FK schaltet weiter ab dem Bild bis zum naechsten Wechsel (ENT2): Schluessel an bild-1 (alter Anteil) und am Bild, Angleichen wie bisher | CAT `layerIKFKRatio` (LayerFloat mit bezier_float, ENT2-Auslese `cat_ikfk.txt`), Autodesk-Hilfe „Limb Animation Controls“ (Match IK and FK vor dem Ueberblenden), Rigify IK_FK als schluesselbare Eigenschaft | `rigdaten.h`, `ik.h/.cpp` (`ikAnteil*`, `ikAnteilAm`), `animkurven.cpp` (`liste`, `wahlIndizes`), `figuranim.cpp` (Zeile `ikfk`), `app_modelltab_kurven.cpp`, `app_figuranim_ui.inc`, `app_modelltab_ik.cpp` |
| B14 aeltere Dateien | Missions-Dateien ohne Zeile `ikfk`: der Anteil der IK-Schluessel (bis rc612: gilt bis zum naechsten) wird beim Laden zu Schluesseln mit Interpolation **Konstant** - dieselbe Treppe, jedes Bild und Kommabild gleich (`ikAnteilUebernehmen`). Programme davor lesen die Zeile `ikfk` nicht und rechnen weiter mit dem Anteil im IK-Schluessel | - | `figuranim.cpp`, `ik.cpp` |
| B15 Ducken | Crouch idle und Crouch walk in der Kategorie `crouch` (Ducken); Crouch walk auf **BOTH_CROUCH1WALK** (anims.h: „Walking while crouched“, vorher BOTH_WALK1) | Entwurf A3 (Ducken), `data/base/anims.h` | `data/presets/presets.txt` |
| B16 Tempo als Faktor | der Regler zeigt den Faktor auf das Tempo der NPCs (1,00× = 90 Einheiten/s beim Gehen, 300 beim Laufen), der Tooltip nennt Basis und das Tempo in Einheiten je Sekunde; gespeichert bleiben Einheiten je Sekunde - derselbe Faktor gibt dieselbe Bewegung | Entwurf A3 „1,0×“, NPC_stats.cpp:1996-1997 (walkSpeed/runSpeed) | `app_modelltab_retarget.cpp` (`tempoRegler`) |
| B17 Griff der Wurzel | neuer Griff `GriffArt::Wurzel` (am Ende der Griffe, kein FK-Knochen, Ort = Mitte des Bodens wie die Rig-Stile, `rigBodenMitte`). Bodenkreis (Control Rig, Blender) und Bodenform (CAT) sind seine Form (waehlbar, duenn wie bisher); die Kopfmarke bleibt Schmuck. Verschieben (W/G) und Drehen (E/R, um die Mitte des Kreises) setzen einen Schluessel der **Plattform** (`RigSchicht::plattform`) am Frame, Rueckgaengig wie jeder Zug; die Plattform wirkt auf die Wurzelknochen (`rigNachMalFk`: M' = M X^-1 P X) und auf die IK-Ziele im Raum Welt (Raum -1 = Plattform), Backen schreibt sie in die Wurzel, Reduzieren behaelt sie als Spur. Eigene Zeile im Dopesheet, „Schluessel loeschen“ loescht ihren Schluessel am Frame, Missions-Zeile `plattform`. Im Stil Standard kein Bodenkreis (der Griff steht im Explorer) | CATParent (die Glied-Ziele haengen an ihm), Rigify `root` | `rigdaten.h`, `rig.h/.cpp`, `ik.cpp`, `rigformen.h/.cpp`, `app_modelltab_rig.cpp`, `app_modelltab_knochen.cpp`, `animkurven.cpp` |
| C1 nur FK zurueck | „Control Rig erstellen“ nach „nur FK“ stellt die Glieder jeder Schicht mit Schluesseln wieder auf IK: FK -> IK wie beim ersten Erstellen (IK-Schluessel je Bild auf der FK-Pose, Anteil 1, FK-Kette je Bild); mit dem Wechsel des Modus EIN Rueckgaengig-Schritt (`Dokument::zusammenlegen`); Meldung mit Gliedern, Sequenzen, Abweichung | ENT2 1a | `app_modelltab_anim.cpp` |
| C3 Rig-Vorlage im Dokument | `RigDaten::vorlage/vorlageFuer` statt Sitzungszustand; ein Wechsel ist ein Schritt („Rig-Vorlage“), Rueckgaengig/Wiederholen bauen das Rig neu (`rigSchluessel` enthaelt die Wahl); die Zuordnung je Vorlage (FIX612C PV5) bleibt unveraendert gemerkt | - | `rigdaten.h`, `app_modelltab_anim.cpp`, `app_modelltab_rig.cpp`, `modelltab.h` |

## Rot -> gruen

| Lauf | Bau | Ergebnis |
|---|---|---|
| `ent3b_rot` | karte1 + Selbsttest, Proben, Felder im Kopf (ohne Behebung) | **4 OK, 35 FEHLER** - jeder Punkt rot (`Y:\werkzeuge\ent3b\rot_ent3b.out`) |
| `ent3b_g1` | Umsetzung | 33 / 7 (Testfehler: Klick auf Vorschau ausserhalb des Felds, Zeile IK/FK weggerollt; Laufen misst sein Tempo) |
| `ent3b_g2` | Test berichtigt | 35 / 3 (Zeile IK/FK des Dopesheets weggerollt) |
| `ent3b_g3` | Dopesheet gerollt | **38 OK, 0 FEHLER** |
| `s_ent3b` | nach dem Rebase auf karte1 `3d0947208` | **38 OK, 0 FEHLER** |

Messwerte (kyle, BOTH_WALK1, Lauf `ent3b_g2`):
- B14 Anteil zwischen +5 (IK) und +15 (FK): +10 = 0,500; angeglichen (nach Control Rig erstellen) an jedem Frame +4..+16 Pose wie vorher **0,000020** (Grenze 0,0001 wie ENT2); nicht angeglichen (Ziele 6 hoch): +5 = IK-Pose 0,000000, +15 = FK-Pose 0,000000, groesster Schritt der Hand je Zehntel-Frame **0,115** bei 6,0 Abstand IK/FK (rot mit der Treppe bis rc612: 6,029).
- B16 Gehen 1,00× = 90, gezogen 3,03× = 272,7 Einheiten/s; zurueck: jedes der 9 Bilder des erzeugten Gangs gleich (0,000000). Laufen misst an BOTH_RUN1: 0,51× = 153,2.
- B17 Verschieben (12, -5, 0): Abweichung 0,000013 an allen Bildern; Drehen 90 Grad um die Mitte: 0,000022; Rueckgaengig 0,000000.
- C1 nach nur FK zurueck: Abweichung je Glied 0,000016 (gesamt und nur IK), ein Rueckgaengig: dieselbe Schicht (0,000000), keine Spur.
- ctest `modell_ent3b`: alte Datei 20 952 Zahlen gegen das alte Programm, groesster Unterschied **0**; Ueberblenden Hand 0,110 je Zehntel bei 6,0; Plattform: ohne Glieder 4,8e-6, Glieder unter IK 0,0013 (Grenze 1/64: die Zwei-Glied-IK loest das Knie ueber acos, nahe der Streckung wird ein Rundungsunterschied von 1e-7 zu ~5e-4 im Winkel - gemessen auch bei reiner Verschiebung); gebacken 0,0123 (< 8/64).

## Angepasste vorhandene Tests (nichts abgeschwaecht)

- `selbsttest_fix612c.inc` P3-5i: nach nur FK legt Control Rig erstellen die Glieder wieder auf IK (C1) - erwartet vier IK-Spuren statt keiner, Rueckgaengig
  zurueck auf nur FK mit derselben Schicht und ohne Spur (vorher FIX612C-Vorgabe „Schicht bleibt“).
- `selbsttest_ent2.inc` „1 Abweichung“: die Kopie „nur FK“ leert auch die IK/FK-Spur (sonst haette `w.ik = 0` nichts mehr bewirkt und die Pruefung
  waere still bestanden).
- `selbsttest_ap13k.inc` S2/S3/S4 und ctest `modell_rigformen`: die Form am Boden ist der Griff der Wurzel (bis dahin Schmuck) - gezaehlt ueber den
  Griff statt ueber `schmuck`; ihre Farbe bleibt weiss (nicht nach Seite).
- Handbuch `inhalt_fix612c.py` (Absatz nur FK): der Satz „eine vorhandene Animation bleibt auf FK“ ersetzt durch C1 (6 Sprachen).

## Handbuch

Eigene Seite `tools/handbuch/inhalt_ent3b.py` (6 Sprachen): modell-ent3b „Model-Tab: IK/FK überblenden, Plattform und Rig-Vorlage“, im Register
`tools/handbuch/inhalt_modell.py` eine Zeile `Seite("inhalt_ent3b")` hinter `inhalt_ap13k`. Bilder `h_ent3b_ikfk` (wie Blender, Dopesheet mit den
Zeilen IK/FK, Arm R zwischen +5 und +15 auf 0,50) und `h_ent3b_wurzel` (wie 3ds Max, Bodenkreis gewählt, Figur verschoben) in 6 Sprachen
(2538 × 1472, je 2 / 0), angesehen en, de, ja. Absatz „nur FK“ von FIX612C (`inhalt_fix612c.py`) an C1 angepasst. Handbuch und Hilfe-Orte
(`gen_handbuch.py`, `gen_hilfe_orte.py`: der Regler IK/FK-Verhältnis ist ein Ort) nach dem Rebase neu erzeugt; `gen_handbuch.py --pruefen` ok.

## Schlussrunde des Zweigs (auf karte1 `3d0947208`)

Unter der Lastregel (Bau `--parallel 2`, höchstens zwei jkstudio, Leerlauf), Direct3D 12 (Vorgabe):

| Schritt | Ergebnis |
|---|---|
| Bau alle Ziele (inkrementell, /W4 /WX) | 0 Fehler, 0 Warnungen (`rebase3`) |
| ctest | **268 / 268** (darin modell_ent3b, modell_rigformen, hilfe_orte, die Lints und i18n) |
| `ent3b` | 38 / 0 (rot 4 / 35) |
| ctest `modell_ent3b` | ok (alte Datei 0 Unterschied, Überblenden, ab dem Bild, Plattform, Backen, Reduzieren, Missions-Zeilen) |
| Gruppen | ap10 72 · ent2 23 · pr 17 · rt 21 · ap13k 66 · fix612c 36 · mb3 23 - je 0 FEHLER; rig 27 / 1 (MB4 Pfad, auf karte1 ebenso rot, s. u.) |
| Handbuchbilder | 6 Sprachen je 2 / 0 |

Bewusst nicht gelaufen: volle Runde, Sprachläufe, Fotovergleich Direct3D 11 - neu gezeichnet wird nichts: der Bodenkreis ist die Form aus AP13K
(dieselbe Linie, gewählt weiß wie jede Griff-Form), die Zeilen des Dopesheets und der Regler sind ImGui. Keine Bildzeit-Messung: ohne Plattform
rechnet nichts mehr als vorher (leere Spur, `plattform.empty()` übersprungen); mit Plattform eine Weltrechnung mehr je Bild.

## Offen / Fragen an shank

- Die Gruppe `rig` hat auf karte1 schon einen roten Schritt (MB4 Pfad: 84 statt 21 Punkte - gleich im Bau ohne ENT3B, Lauf `n0_rig`); nicht Teil dieses
  Auftrags.
- Der Knopf IK/FK (wie 3ds Max) schaltet weiter hart (ein Frame Uebergang); laengeres Ueberblenden ueber den Regler IK/FK-Verhaeltnis oder durch
  Ziehen des Schluessels im Dopesheet. Soll der Knopf selbst ueberblenden (z. B. ueber eine einstellbare Dauer)?

## Changelog

## English

- Model tab, Animation: like a CAT rig every limb has its own IK/FK track - between two keys the limb blends from IK to FK, and the track has a row IK/FK in the dope sheet (click, drag, delete, interpolation). Like 3ds Max an IK/FK ratio slider sits under the selected limb; older mission files load unchanged.
- Model tab, Animation: the ground circle of the rig styles is the root control - like CAT's platform it moves and turns the whole figure, the IK targets of hands and feet go along; with a key, its own dope sheet row and Ctrl+Z.
- Model tab, Animation: Create control rig after Animation rig (FK only) puts the limbs back on IK like the first time, in one undo step; the choice of the rig template is undone and redone with Ctrl+Z and Ctrl+Y.
- Presets: Crouch idle and Crouch walk are in the category Crouch; the speed of the generated walk and run is a factor (1.00× = the speed of the NPCs), the tooltip names the units per second.

## Deutsch

- Model-Tab, Animation: wie bei einem CAT-Rig hat jedes Glied eine eigene IK/FK-Spur - zwischen zwei Schlüsseln blendet das Glied von IK nach FK über, und die Spur hat im Dopesheet eine Zeile IK/FK (anklicken, ziehen, löschen, Interpolation). Wie 3ds Max steht unter dem gewählten Glied ein Regler IK/FK-Verhältnis; ältere Missions-Dateien laden unverändert.
- Model-Tab, Animation: der Bodenkreis der Rig-Stile ist der Griff der Wurzel - wie CATs Plattform verschiebt und dreht er die ganze Figur, die IK-Ziele von Händen und Füßen gehen mit; mit Schlüssel, eigener Zeile im Dopesheet und Strg+Z.
- Model-Tab, Animation: Control Rig erstellen nach Animation-Rig (nur FK) stellt die Glieder wie beim ersten Mal wieder auf IK, in einem Rückgängig-Schritt; die Wahl der Rig-Vorlage nehmen Strg+Z und Strg+Y zurück und wieder vor.
- Presets: Crouch idle und Crouch walk stehen in der Kategorie Ducken; das Tempo des erzeugten Gehens und Laufens ist ein Faktor (1,00× = Tempo der NPCs), der Tooltip nennt die Einheiten je Sekunde.

## 中文

- 模型选项卡, 动画: 与 CAT 绑定相同, 每个肢体都有自己的 IK/FK 轨道 - 在两个关键帧之间肢体从 IK 过渡到 FK, 该轨道在摄影表中有一行 IK/FK (单击、拖动、删除、插值)。同 3ds Max 时所选肢体下方有 IK/FK 比例滑块; 旧的任务文件不变地加载。
- 模型选项卡, 动画: 绑定样式的地面圆环是根控制器 - 像 CAT 的平台一样移动和旋转整个角色, 手脚的 IK 目标一起移动; 带关键帧、摄影表中的专用行和 Ctrl+Z。
- 模型选项卡, 动画: 在“动画绑定 (仅 FK)”之后使用“创建控制绑定”会像首次一样把肢体重新设为 IK, 一步撤销; 绑定模板的选择可用 Ctrl+Z 和 Ctrl+Y 撤销和重做。
- 预设: Crouch idle 和 Crouch walk 位于“蹲伏”类别; 生成的行走和跑步速度为倍数 (1.00× = NPC 的速度), 工具提示给出每秒单位数。

## 日本語

- モデルタブ、アニメーション: CAT リグと同じく手足ごとに IK/FK トラックがあり、2 つのキーの間で IK から FK へブレンドします。トラックにはドープシートに IK/FK の行があります (クリック、ドラッグ、削除、補間)。3ds Max 風では選択した手足の下に IK/FK 比率スライダーがあります。古いミッションファイルはそのまま読み込めます。
- モデルタブ、アニメーション: リグのスタイルの地面の円はルートのコントロールです - CAT のプラットフォームと同じくフィギュア全体を移動・回転し、手足の IK ターゲットも一緒に動きます。キー、ドープシートの専用行、Ctrl+Z 付き。
- モデルタブ、アニメーション: 「アニメーションリグ (FK のみ)」の後の「コントロールリグを作成」は最初と同じく手足を再び IK にし、1 ステップで戻せます。リグテンプレートの選択は Ctrl+Z と Ctrl+Y で戻し・やり直しができます。
- プリセット: Crouch idle と Crouch walk はカテゴリ「しゃがみ」に入りました。生成した歩きと走りの速度は倍率 (1.00× = NPC の速度) で、ツールチップに毎秒のユニット数が出ます。

## Español

- Pestaña Model, Animación: como en un rig de CAT, cada extremidad tiene su propia pista IK/FK; entre dos claves la extremidad se mezcla de IK a FK, y la pista tiene una fila IK/FK en la hoja de claves (pulsar, arrastrar, borrar, interpolación). Como 3ds Max hay un control Proporción IK/FK bajo la extremidad elegida; los archivos de misión antiguos se cargan sin cambios.
- Pestaña Model, Animación: el círculo del suelo de los estilos de rig es el control raíz; como la plataforma de CAT mueve y gira toda la figura, y los objetivos IK de manos y pies van con ella; con clave, fila propia en la hoja de claves y Ctrl+Z.
- Pestaña Model, Animación: Crear rig de control después de Rig de animación (solo FK) vuelve a poner las extremidades en IK como la primera vez, en un paso de deshacer; la elección de la plantilla de rig se deshace y rehace con Ctrl+Z y Ctrl+Y.
- Presets: Crouch idle y Crouch walk están en la categoría Agachado; la velocidad de la marcha y la carrera generadas es un factor (1,00× = velocidad de los NPC) y la información emergente da las unidades por segundo.

## Čeština

- Karta Model, Animace: jako u rigu CAT má každá končetina vlastní stopu IK/FK - mezi dvěma klíči končetina přechází z IK do FK a stopa má v dope sheetu řádek IK/FK (kliknout, táhnout, smazat, interpolace). Jako 3ds Max je pod vybranou končetinou posuvník Poměr IK/FK; starší soubory misí se načtou beze změny.
- Karta Model, Animace: kruh na zemi stylů rigu je ovladač kořene - jako plošina CAT posouvá a otáčí celou postavu, cíle IK rukou a nohou jdou s ní; s klíčem, vlastním řádkem v dope sheetu a Ctrl+Z.
- Karta Model, Animace: Vytvořit ovládací rig po Animačním rigu (jen FK) vrátí končetiny jako poprvé na IK, jedním krokem zpět; volbu šablony rigu vrátí a zopakují Ctrl+Z a Ctrl+Y.
- Předvolby: Crouch idle a Crouch walk jsou v kategorii Přikrčení; rychlost generované chůze a běhu je násobek (1,00× = rychlost NPC), popisek uvádí jednotky za sekundu.
