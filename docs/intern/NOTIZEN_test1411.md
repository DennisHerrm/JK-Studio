# NOTIZEN TEST1411 (14.11.2026): Testrückmeldungen von shank zu rc612

Auftrag `JKStudio_Uebergabe/auftraege/auftrag_test1411.md`, Fotos von shank `Y:\werkzeuge\test1411\fotos_shank\`. Zweig `p_test1411`
(Worktree `Y:\p_ap6`), Basis karte1 `348f0e345` (rc612). Erster Teil (D, F, I, H, G, J, K, L, M, C) von Opus 5.5 bis `fc44ff56d`, zweiter
Teil (N, O, P, A, E) als TEST1411b. Selbsttest `gui/selbsttest_test1411.inc`:
`BHED_EDITORTEST=modellblock BHED_MODELL_NUR=test1411 BHED_T1411=<punkte>` (Fenster 3840x2160 bei 1,5, alora).

## Ergebnis kurz

| Punkt | Was | Rot (vorher, gemessen) | Grün (nachher) |
|---|---|---|---|
| D | Modell doppelt beim Zoomen, Drahtgitter ab Werk | Ansichtsbild eine Raste hinter der Kamera: bis 116 px gegen frisch gezeichnet; Objektebene 4458 Kanten; Geister in allen Bereichen | 0 px; 0 Kanten auf der Objektebene; Geister nur im Bereich Animation |
| F | Zeitleiste: Knöpfe rutschen | Auto Key bei 611/625/640/654 px (Bild 0/10/100/659) | fest 661 px |
| I | Animation: Knochen erst mit Control Rig | Klick wählte die Fläche torso, kein Knochen, Drehen/Key ohne Wirkung | Knochen gewählt, 30,0° gedreht, Key gesetzt, Abspielen 30,0° |
| H | Knochen ohne saubere Hierarchie | 196 sich schneidende Kastenpaare (_humanoid MD) | 0 (auch rancor), Ketten-Enden stimmen, Drehhilfen als Punkt |
| G | Zeitleiste und Text darunter wie 3ds Max | 12 Prüfungen rot (Fließtext unter der Leiste, kein Zeitschieber, Fußzeile im Explorer) | grün; ap1 134/0, ap2 68/0 |
| J | Rechtsklick: Move/Rotate | kein Move/Rotate/Scale im Menü | Quad-Menü wie 3ds Max, Rotate setzt das Werkzeug; Blender: Transform |
| K | Fußplattform wie CAT | keine Plattform | Plattform am Boden, Fuß folgt, Knie 164° -> 106°, Abrollen um die Zehe |
| L | ViewCube: Buchstabe scheint durch | Klick am verdeckten "W" traf den Ring | trifft den Würfel, Übergang ohne Überschwingen |
| M | Silhouette: Beschriftung unlesbar | Kontrast 1,1:1 | >= 5,3:1 auf jedem Grund |
| C | Load .gla nur lose Dateien | nur Windows-Dialog | Liste der .gla aus den pk3 + "Datei öffnen..." |
| N | Motion-Feld zu voll | 17 Gruppen untereinander (Foto 26; ap10 A14 zählte sie als Soll) | Setup-Modus 6, Animationsmodus 9 Gruppen; Ghosts und Knochen-Darstellung im Reiter Anzeige; K im Setup-Modus 0 Schlüssel |
| O | Painting: Ziel-Textur von Hand | ein Strich Rumpf -> Gesicht malte nur ins erste Bild (der alte Weg = Schalter "Nur Ziel-Textur bemalen": 1 Bild) | Strich malt Rumpf, Kopf und Gesicht (3 Bilder), Strg+Z nimmt alle drei, Ziel folgt (Explorer, Alt+Klick, Vorschaubild) |
| P | Weights: Ecken einzeln wie Skin | Auswahl nicht im Rollout, keine Liste, keine Weight Table, kein Mirror Paste (Foto 28) | Ecke klicken -> Liste lradius 0,500 / lradiusX 0,500 -> 0,6 getippt, Summe 1 -> Mirror Paste -> Gegenseite gleich; Rahmen ohne Rückseiten 2488 -> 1619 Ecken |
| A | Load mission öffnet den pk3-Dialog | Dateidialog (im Selbsttest unterdrückt: Auswahl ging nicht auf); aus einer Liste ohne Archiv kein Eintrag "zuletzt geöffnet" | Liste des Spiels sofort, nach Archiv gruppiert; Eintrag mit vollem Pfad |
| E | Sequenzen im Baum ohne Suche | Ast mit 2635 Zeilen im Baum, Namen abgeschnitten | eigener Reiter mit Suche (Strg+F), Gruppe, Spalten Name/Bilder/Schleife sortierbar |
| B | Model-Tab leer sieht anders aus | | offen (siehe Offen) |

## Entscheidungen

- D: Objektebene ohne Kanten (Max und Blender); Unterobjekt-Ebenen wie Blenders Edit Mode schwarz, Alpha 170, 1 px; Max hell wie bisher. Ghosts nur im Bereich Animation, Schalter auch im [+]-Menü, ab Werk aus.
- I: ohne Rig dreht der Knochen selbst (FK, Rig mit einem Griff), kein automatisches Control Rig; mit Rig wählt der Klick den Griff. "hips is frozen" frisch geladen nicht nachstellbar.
- H: Kasten entlang der eigenen X-Achse der GLA-Grundstellung bis zum Folgegelenk; Drehhilfen als Hilfspunkt (Schalter "Hilfsknochen zeigen"); Verbindungslinie nur am Anfang einer Kette.
- G: eine einzeilige Statuszeile; Tempo/Interpolation/Schleife in der Zeitkonfiguration; Mini Curve Editor links vor dem Zeitschieber; Tastenzeile ab Werk aus; Umschalt-Ziehen kopiert Keys nur wie 3ds Max.
- J: Quad-Menü aus vier Kindfenstern um die Klickstelle; Isolate = nicht Gewähltes ausblenden. Blender: Untermenü "Transform" (G/R/S).
- K: Plattform im Stil Standard zwischen den Drehpunkten Ferse und Zehe (Breite 45 % der Länge), sie ist der IK-Fuß-Griff.
- L: Klick auf einen Buchstaben schaut von dort (wie bisher), nur wo der Würfel ihn nicht verdeckt; Übergang smoothstep.
- M: Weiß oder Schwarz nach WCAG-Leuchtdichte; Hover gelb nur, solange es 4,5:1 hat.
- C: "Open .skin..." geprüft: die Skins der pk3 stehen schon im Rollout "Level of detail and skin" - nicht umgebaut.
- N (CAT, Autodesk-Hilfe 2015 GUID-275009EE "Setup mode and Animation mode", GUID-D6D4CD17 Hierarchy Panel for CATRigs): Setup-Modus = Rig erstellen, Anbauten und Vorschau, Für das Spiel bauen, GLA, animation.cfg, Ziel-GLA und Plätze; Animationsmodus = Animation, Presets, Vorhandene Animation bearbeiten, Übertragen, IK/FK, Auto-Physik, Pose, Bewegungspfade, Kurven und Animationswerkzeuge, Posen-Bibliothek. IK/FK steht im Animationsmodus, weil es Schlüssel setzt (CAT: Limb Animation im Animationsmodus). Ghosts und "Knochen- und Rig-Darstellung" nur im Reiter Anzeige (Max: Display-Panel bzw. Views > Show Ghosting). Die Gruppen zum Aufbauen bleiben im Animationsmodus in ihrem eigenen Reiter (Hierarchie, Dienstprogramme); die Gruppen zum Animieren gibt es im Setup-Modus gar nicht (sie setzen Schlüssel). Setup-Modus: die Figur steht in der Grundstellung der GLA (CAT: "Setup mode is completely static"), kein Gizmo, K/Auto Key/G/R melden den Grund statt zu wirken; nur im Bereich Animation. Taste: Max kennt für CAT keine (max_hotkeys_de.txt) - Strg+Umschalt+M (im Max-Modus frei); Blender: Tab (Edit Mode / Pose Mode der Armatur), Modus-Feld im Kopf der Ansicht. Vorgabe nach dem Laden: Animationsmodus, wenn die GLA Sequenzen hat oder ein Rig da ist, sonst Setup.
- O: wie Viewport Canvas mit Multi/Sub-Object und Substance Painter malt ein Strich in jedes Bild unter dem Pinsel (Stufe 0 der Fläche), das Ziel folgt der zuletzt bemalten Fläche; ein Strich über mehrere Bilder trägt eine Kennung, Rückgängig und Wiederholen nehmen alle seine Schritte zusammen. Alt+Klick (frei: die Navigation nutzt Alt nur mit der Mitteltaste) und die Pipette "Textur aufnehmen" wählen die Textur; ein Klick auf eine Fläche im Explorer ebenso. Vorschaubilder aus der Leinwand bzw. dem Bild der Vorschau, deckend (ohne Alpha), höchstens alle 15 Bilder neu beim Malen. Die Ausklappliste der Shader bleibt darunter (ap9 prüft sie).
- P: Auswahl im Rollout wie Skin "Select: Vertices, Select Element, Backface Cull Vertices"; Backface Cull nach dem Umlauf am Schirm (im Uhrzeigersinn = Vorderseite, FrontCounterClockwise FALSE wie im Spiel). Liste des Weight Tools mit Mittelwert, eintippbar (Enter setzt an allen gewählten Ecken, die Summe bleibt 1). Copy/Paste/Paste-Pos; Paste-Pos-Bone wie Paste-Pos, weil alle Flächen ein Skelett teilen. Mirror Paste: Quelle ist die Seite mit den meisten gewählten Punkten. Die Knochenpaare stehen sichtbar (blau/grün) unter "Knochenpaare"; ändern lassen sie sich wie in Max nur über die Namen bzw. Toleranz (Max paart nach Ort und Schwelle, ohne Liste von Hand). Gewählte Ecken größer (0,2 Zeichenhöhen) mit dunklem Rand. Blender: Panel "Vertex-Gewichte".
- A: "Load mission..." (Menü, Werkzeugknopf, Startfläche, zuletzt geöffnet) ohne Dateidialog: die Missionen der geltenden Spielordner. Gruppen je Archiv der spielbaren Missionen (ab Werk offen, alphabetisch), am Ende "Karten ohne Skripte (Duellkarten)" (ab Werk zu). Die Gruppe ist das Archiv der .ent, ohne .ent das der .bsp - dort liegt die Karte, die die Engine lädt (`Mission::gruppe`). Bei Movie Duels liegen die Karten der Episoden in MD_Maps_Ep1 ... und manche Missionen in MD_Missions_Ep1 ... (Lauf t1411_a2: 23 Gruppen); die Skripte nennt die Gruppe nicht. "Ordner öffnen..." nutzt `spielordnerFuerOrdner` (dieselben Regeln wie für ein Archiv, Unit-Test missiontest); "Archiv öffnen..." ist der bisherige Weg. Ohne Spielordner: Hinweis und drei Knöpfe ("Spielordner einrichten..." öffnet Bearbeiten > Spielordner). `Mission::archivPfad` merkt den vollen Pfad des Archivs der .bsp; er geht in "zuletzt geöffnet" und in `missionArchive` (Save mission), alte Einträge `pk3|...` bleiben lesbar. Lose Dateien (maps/, scripts/ unverpackt) kennt `GamePath` nicht - die Liste nennt nur Karten aus pk3 (nicht nebenbei gebaut). Zwischenspeicher auf der Platte (empfohlen) nicht gebaut.
- E: dritter Reiter "Sequenzen" im Explorer (Max und Blender), im Baum ein Eintrag "Sequenzen (n)", der dorthin springt. Klick spielt ab wie bisher (Doppelklick ebenso), Rechtsklick das bisherige Menü. Strg+F setzt den Fokus, wenn die Maus über dem Explorer steht (wie Blenders Outliner; Max belegt Strg+F sonst mit dem Auswahlbereich). Zeigen (Laden, NPC-Browser) rollt in der Liste, ohne den Reiter zu wechseln; ein ausdrückliches Zeigen (Aktionssuche, Selbsttest) wechselt hin.

## Offen

- B (leerer Model-Tab wie mit Modell): nicht umgesetzt.
- Handbuch in 6 Sprachen für alle Punkte: nicht umgesetzt (siehe UEBERGABE).
- F: eigener Selbsttest der Vorschau-Zeitleiste; G: Mauspunkt auf dem Raster als X/Y/Z.
- A: Zwischenspeicher `missionsVerweise` auf der Platte und Gruppen schon während des Lesens (empfohlen).
- P: Knochenpaare von Hand ändern (Max hat das auch nicht als Liste).
- Läufe: ap1 134/0, ap10 71/1 (Tooltip "Auf Rig legen" - im Lauf davor grün, Zeit unter Last), ap2 67/1 (X17: Klick auf eine TORSO_-Zeile der
  Sequenzliste kommt nicht an, im E-Test wirkt der Klick auf eine Zeile). Noch nicht gelaufen: ap13k, ent2, ent3b, mb2, fix612, ap9, pt*,
  programm (Load mission), missionsbau, vorschau_b, Handbuch-Gruppen.

## Changelog

## English

- Model tab: the model no longer shows twice when zooming, and the object level shows no wireframe (edges only on sub-object levels); ghosts only in the Animation area.
- Model tab: the timeline buttons stand still while the frame number grows.
- Animation: bones are visible, selectable and rotatable from the start like in 3ds Max, without a control rig.
- Bones: one clean box per bone along the chain, twist helpers as small points.
- Timeline and status bar like 3ds Max in every area: time slider above the track bar, one status line, no text under the timeline.
- Right-click in the view: the quad menu of 3ds Max with Move, Rotate and Scale; in Blender mode Transform in the context menu.
- Control rig: a foot platform on the ground like CAT, with roll over heel and toe.
- ViewCube: compass letters behind the cube are hidden and cannot be clicked; the camera no longer swings.
- Labels over the view stay readable on any background (silhouette, white).
- Load .gla...: the list of all .gla files in the game archives, plus Open file... for a loose one.
- Motion panel: Setup Mode and Animation Mode like CAT (Blender: Edit Mode / Pose Mode). Setup shows the setup pose and sets no keys; ghosts and bone display moved to the Display panel.
- Painting: a stroke paints into the texture of every surface under the brush, across images; one undo takes the whole stroke back. The target follows the painted surface; thumbnails, Alt+click and a picker choose it; "Paint target texture only" keeps the old way.
- Weights like the Skin modifier of 3ds Max: select vertices in the panel (Vertices, Select Element, Backface Cull Vertices), the Weight Tool list with typed values, Copy/Paste/Paste-Pos, a dockable Weight Table and Mirror Paste.
- Load mission no longer opens a file dialog: the missions of the game folders appear at once, grouped by archive, with Open folder... and Open archive...; recently opened works without a chosen archive.
- Scene Explorer: the sequences have their own tab with search (Ctrl+F), group and sortable columns name, frames and loop.

## Deutsch

- Model-Tab: Das Modell erscheint beim Zoomen nicht mehr doppelt, auf der Objektebene ohne Drahtgitter (Kanten nur auf den Unterobjekt-Ebenen); Geister nur im Bereich Animation.
- Model-Tab: Die Knöpfe der Zeitleiste stehen still, wenn die Bildzahl wächst.
- Animation: Knochen sind wie in 3ds Max von Anfang an sichtbar, wählbar und drehbar, ohne Control Rig.
- Knochen: ein sauberer Kasten je Knochen entlang der Kette, Drehhilfen als kleine Punkte.
- Zeitleiste und Statuszeile wie 3ds Max in jedem Bereich: Zeitschieber über der Spurleiste, eine Statuszeile, kein Text unter der Zeitleiste.
- Rechtsklick in die Ansicht: das Quad-Menü von 3ds Max mit Move, Rotate und Scale; im Blender-Modus Transform im Kontextmenü.
- Control Rig: eine Fußplattform am Boden wie CAT, mit Abrollen über Ferse und Zehe.
- ViewCube: Kompassbuchstaben hinter dem Würfel sind verdeckt und nicht klickbar; die Kamera schwingt nicht mehr.
- Beschriftungen über der Ansicht bleiben auf jedem Grund lesbar (Silhouette, Weiß).
- Load .gla...: die Liste aller .gla der Spielarchive, dazu Datei öffnen... für eine lose.
- Motion-Feld: Setup-Modus und Animationsmodus wie CAT (Blender: Bearbeitungs-/Posenmodus). Setup zeigt die Setup-Pose und setzt keine Schlüssel; Geister und Knochen-Darstellung stehen im Reiter Anzeige.
- Painting: Ein Strich malt in die Textur jeder Fläche unter dem Pinsel, über Bilder hinweg; ein Rückgängig nimmt den ganzen Strich. Das Ziel folgt der bemalten Fläche; Vorschaubilder, Alt+Klick und eine Pipette wählen es; "Nur Ziel-Textur bemalen" hält den alten Weg.
- Weights wie der Skin-Modifier von 3ds Max: Ecken im Feld wählen (Ecken, Element wählen, Rückseiten-Ecken ausblenden), die Liste des Weight Tools mit eintippbaren Werten, Kopieren/Einfügen/Einfügen nach Ort, eine andockbare Gewichtstabelle und Spiegeln einfügen.
- Mission laden öffnet keinen Dateidialog mehr: die Missionen der Spielordner stehen sofort da, nach Archiv gruppiert, mit Ordner öffnen... und Archiv öffnen...; zuletzt geöffnet geht auch ohne gewähltes Archiv.
- Szenen-Explorer: Die Sequenzen haben einen eigenen Reiter mit Suche (Strg+F), Gruppe und sortierbaren Spalten Name, Bilder und Schleife.

## 中文

- 模型标签: 缩放时模型不再显示两次, 对象层级不显示线框 (只在子对象层级显示边); 残影只在动画区域显示。
- 模型标签: 帧数增加时时间轴按钮保持不动。
- 动画: 与 3ds Max 一样, 骨骼从一开始就可见、可选、可旋转, 无需控制绑定。
- 骨骼: 每根骨骼沿骨骼链一个整洁的方框, 扭转辅助骨骼显示为小点。
- 时间轴和状态栏在所有区域都与 3ds Max 相同: 轨迹栏上方的时间滑块, 一行状态栏, 时间轴下方没有文字。
- 在视图中右键: 3ds Max 的四元菜单, 含移动、旋转、缩放; Blender 模式在上下文菜单中提供变换。
- 控制绑定: 与 CAT 相同的地面脚部平台, 可绕脚跟和脚尖滚动。
- ViewCube: 立方体后面的指南针字母被遮挡且不可单击; 相机不再摆动。
- 视图上方的标签在任何背景上都保持可读 (剪影、白色)。
- 加载 .gla...: 游戏存档中所有 .gla 的列表, 另有 打开文件... 用于单独文件。
- 运动面板: 与 CAT 相同的设置模式和动画模式 (Blender: 编辑模式 / 姿态模式)。设置模式显示设置姿态且不设置关键帧; 残影和骨骼显示移到显示面板。
- 绘制: 一笔绘制到画笔下每个表面的纹理中, 跨越图像; 一次撤销撤回整笔。目标跟随绘制的表面; 缩略图、Alt+单击和吸管可选择目标; "仅绘制目标纹理" 保留旧方式。
- 权重与 3ds Max 的蒙皮修改器相同: 在面板中选择顶点 (顶点、选择元素、背面消隐顶点), 权重工具列表可输入数值, 复制/粘贴/按位置粘贴, 可停靠的权重表和镜像粘贴。
- 加载任务不再打开文件对话框: 游戏文件夹中的任务立即按存档分组显示, 并提供 打开文件夹... 和 打开存档...; 最近打开在未选择存档时也可用。
- 场景资源管理器: 序列有独立的标签页, 带搜索 (Ctrl+F)、分组和可排序的列 名称、帧数和循环。

## 日本語

- モデルタブ: ズーム時にモデルが二重に表示されなくなり、オブジェクトレベルではワイヤーフレームを表示しません (エッジはサブオブジェクトレベルのみ)。ゴーストはアニメーション領域のみ。
- モデルタブ: フレーム番号が増えてもタイムラインのボタンは動きません。
- アニメーション: 3ds Max と同じく、コントロールリグなしで最初からボーンが表示され、選択・回転できます。
- ボーン: チェーンに沿ってボーンごとに整ったボックスが 1 つ、ねじれ補助ボーンは小さな点。
- タイムラインとステータスバーはどの領域でも 3ds Max と同じ: トラックバーの上のタイムスライダー、1 行のステータスバー、タイムラインの下に文章なし。
- ビューで右クリック: 移動・回転・スケール付きの 3ds Max クアッドメニュー。Blender モードではコンテキストメニューにトランスフォーム。
- コントロールリグ: CAT と同じ地面のフットプラットフォーム、かかととつま先で転がせます。
- ViewCube: キューブの後ろの方位文字は隠れてクリックできません。カメラが揺れなくなりました。
- ビュー上のラベルはどの背景でも読めます (シルエット、白)。
- .gla を読み込む...: ゲームアーカイブ内のすべての .gla の一覧と、単独ファイル用の ファイルを開く...。
- モーションパネル: CAT と同じセットアップモードとアニメーションモード (Blender: 編集モード / ポーズモード)。セットアップはセットアップポーズを表示しキーを設定しません。ゴーストとボーン表示は表示パネルへ移動。
- ペイント: ストロークはブラシの下のすべてのサーフェスのテクスチャに、画像をまたいでペイントします。1 回の元に戻すでストローク全体を戻します。ターゲットはペイントしたサーフェスに追従し、サムネイル、Alt+クリック、スポイトで選択できます。「ターゲットテクスチャのみペイント」で従来の方法。
- ウェイトは 3ds Max のスキンモディファイヤと同じ: パネルで頂点を選択 (頂点、要素を選択、背面頂点をカリング)、値を入力できるウェイトツールの一覧、コピー/貼り付け/位置で貼り付け、ドッキング可能なウェイトテーブルとミラー貼り付け。
- ミッションの読み込みでファイルダイアログが開かなくなりました: ゲームフォルダーのミッションがすぐにアーカイブ別に表示され、フォルダーを開く... と アーカイブを開く... があります。最近開いた項目はアーカイブを選ばなくても使えます。
- シーンエクスプローラー: シーケンスに検索 (Ctrl+F)、グループ、並べ替え可能な列 名前・フレーム・ループ付きの独自タブ。

## Español

- Pestaña Modelo: el modelo ya no aparece doble al hacer zoom y el nivel de objeto no muestra malla de alambre (aristas solo en los subniveles); fantasmas solo en el área Animación.
- Pestaña Modelo: los botones de la línea de tiempo no se mueven cuando crece el número de fotograma.
- Animación: los huesos se ven, se seleccionan y se giran desde el principio como en 3ds Max, sin control rig.
- Huesos: una caja limpia por hueso a lo largo de la cadena, ayudantes de torsión como puntos pequeños.
- Línea de tiempo y barra de estado como en 3ds Max en todas las áreas: control de tiempo sobre la barra de pistas, una sola línea de estado, sin texto bajo la línea de tiempo.
- Clic derecho en la vista: el menú cuádruple de 3ds Max con Mover, Rotar y Escalar; en el modo Blender, Transformar en el menú contextual.
- Control rig: una plataforma de pie en el suelo como CAT, con giro sobre talón y punta.
- ViewCube: las letras de la brújula detrás del cubo quedan ocultas y no se pueden pulsar; la cámara ya no oscila.
- Las etiquetas sobre la vista se leen sobre cualquier fondo (silueta, blanco).
- Cargar .gla...: la lista de todos los .gla de los archivos del juego, y Abrir archivo... para uno suelto.
- Panel Movimiento: Modo de configuración y Modo de animación como CAT (Blender: Modo Edición / Modo Pose). La configuración muestra la pose de configuración y no pone claves; los fantasmas y la vista de huesos pasan al panel Visualización.
- Pintura: un trazo pinta en la textura de cada superficie bajo el pincel, a través de imágenes; un deshacer quita el trazo entero. El destino sigue a la superficie pintada; miniaturas, Alt+clic y un cuentagotas lo eligen; "Pintar solo la textura de destino" mantiene el modo anterior.
- Pesos como el modificador Piel de 3ds Max: seleccionar vértices en el panel (Vértices, Seleccionar elemento, Descartar vértices traseros), la lista de la Herramienta de peso con valores escritos, Copiar/Pegar/Pegar por posición, una Tabla de pesos acoplable y Pegar simetría.
- Cargar misión ya no abre un diálogo de archivo: las misiones de las carpetas del juego aparecen al momento, agrupadas por archivo, con Abrir carpeta... y Abrir archivo...; lo abierto recientemente funciona sin archivo elegido.
- Explorador de escena: las secuencias tienen su propia pestaña con búsqueda (Ctrl+F), grupo y columnas ordenables nombre, fotogramas y bucle.

## Čeština

- Karta Model: model se při zoomu už nezobrazuje dvakrát a na úrovni objektu bez drátového modelu (hrany jen na podúrovních); duchové jen v oblasti Animace.
- Karta Model: tlačítka časové osy stojí, když roste číslo snímku.
- Animace: kosti jsou jako v 3ds Max od začátku viditelné, vybíratelné a otočné, bez control rigu.
- Kosti: jeden čistý kvádr na kost podél řetězce, pomocné kosti natočení jako malé body.
- Časová osa a stavový řádek jako v 3ds Max v každé oblasti: časový posuvník nad stopovou lištou, jeden stavový řádek, žádný text pod časovou osou.
- Pravý klik do pohledu: čtyřdílné menu 3ds Max s Move, Rotate a Scale; v režimu Blender Transformace v kontextovém menu.
- Control rig: plošina chodidla na zemi jako CAT, s odvalením přes patu a špičku.
- ViewCube: písmena kompasu za krychlí jsou skrytá a nelze na ně kliknout; kamera už nekmitá.
- Popisky nad pohledem zůstávají čitelné na každém pozadí (silueta, bílá).
- Načíst .gla...: seznam všech .gla v archivech hry a Otevřít soubor... pro volný soubor.
- Panel Pohyb: režim nastavení a režim animace jako CAT (Blender: režim úprav / režim pózy). Nastavení ukazuje pozici nastavení a nenastavuje klíče; duchové a zobrazení kostí jsou v panelu Zobrazení.
- Malování: tah maluje do textury každé plochy pod štětcem, přes obrázky; jedno zpět vrátí celý tah. Cíl sleduje malovanou plochu; vybírají ho náhledy, Alt+klik a kapátko; "Malovat jen cílovou texturu" zachová starý způsob.
- Váhy jako modifikátor Skin v 3ds Max: výběr vrcholů v panelu (Vrcholy, Vybrat prvek, Vynechat zadní vrcholy), seznam Weight Tool se zadávanými hodnotami, Kopírovat/Vložit/Vložit podle místa, ukotvitelná tabulka vah a Zrcadlově vložit.
- Načíst misi už neotevírá dialog souboru: mise ze složek hry se zobrazí hned, seskupené podle archivu, s Otevřít složku... a Otevřít archiv...; naposledy otevřené funguje i bez zvoleného archivu.
- Průzkumník scény: sekvence mají vlastní kartu s hledáním (Ctrl+F), skupinou a řaditelnými sloupci název, snímky a smyčka.
