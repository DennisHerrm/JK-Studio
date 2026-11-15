# -*- coding: utf-8 -*-
"""Oberflaechentexte von TEST1411 (14.11.: Testrueckmeldungen von shank zu rc612).

I: im Bereich Animation ist ein Knochen von Anfang an waehlbar (wie 3ds Max), auch ohne Control Rig.
Begriffe wie die vorhandenen Texte (AP4 "Knochen waehlen"/"Select bone", AP10 Control Rig, ENT2 IK/FK).
Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    # --- I: Knochen im Bereich Animation ---
    ("T1KnochenGewaehlt",
     "Bone %s selected - E rotates it, K sets a key (no control rig needed).",
     "Knochen %s gewählt - E dreht ihn, K setzt einen Schlüssel (ohne Control Rig).",
     "已选择骨骼 %s - E 旋转, K 设置关键帧 (无需控制绑定)。",
     "ボーン %s を選択しました - E で回転、K でキーを設定 (コントロールリグは不要)。",
     "Hueso %s seleccionado: E lo gira, K pone una clave (sin control rig).",
     "Kost %s vybrána - E ji otáčí, K nastaví klíč (bez control rigu)."),
    # --- H: Knochen als Kette je Glied ---
    ("T1Hilfsknochen", "Show helper bones", "Hilfsknochen zeigen", "显示辅助骨骼", "補助ボーンを表示", "Mostrar huesos auxiliares", "Zobrazit pomocné kosti"),
    ("T1HilfsknochenHint",
     "Twist helpers (e.g. lradiusX: same axis as the parent, in front of a sibling, no chain of their own) as a box instead of a small point. "
     "Every other bone is one box from its joint along its own axis to the next joint of the chain, like 3ds Max bones.",
     "Drehhilfen (z. B. lradiusX: gleiche Achse wie der Elternknochen, vor einem Geschwister, ohne eigene Kette) als Kasten statt als kleiner Punkt. "
     "Jeder andere Knochen ist ein Kasten von seinem Gelenk entlang seiner Achse bis zum nächsten Gelenk der Kette, wie Bones in 3ds Max.",
     "将扭转辅助骨骼 (例如 lradiusX: 与父骨骼同轴、位于同级骨骼之前、没有自己的骨骼链) 显示为方框而不是小点。"
     "其他每根骨骼都是一个方框, 从其关节沿自身轴线延伸到骨骼链的下一个关节, 与 3ds Max 的骨骼相同。",
     "ねじれ補助ボーン (例: lradiusX - 親と同じ軸、兄弟ボーンの手前、独自のチェーンなし) を小さな点ではなくボックスで表示します。"
     "他のボーンはすべて、関節から自身の軸に沿ってチェーンの次の関節まで伸びる 1 つのボックスです (3ds Max のボーンと同じ)。",
     "Ayudantes de torsión (p. ej. lradiusX: mismo eje que el padre, delante de un hermano, sin cadena propia) como caja en lugar de un punto pequeño. "
     "Cada uno de los demás huesos es una caja desde su articulación a lo largo de su eje hasta la siguiente articulación de la cadena, como los huesos de 3ds Max.",
     "Pomocné kosti pro natočení (např. lradiusX: stejná osa jako rodič, před sourozencem, bez vlastního řetězce) jako kvádr místo malého bodu. "
     "Každá jiná kost je jeden kvádr od svého kloubu podél vlastní osy k dalšímu kloubu řetězce, jako kosti v 3ds Max."),
    # --- G: Zeitleiste und Statuszeile wie 3ds Max ---
    ("T1SchieberHint", "Time slider: drag the handle to scrub, click the bar to jump there, < and > step one frame.",
     "Zeitschieber: den Griff ziehen spult, ein Klick in die Leiste springt dorthin, < und > gehen ein Bild weiter.",
     "时间滑块: 拖动滑块可拖拽播放, 单击滑轨跳到该处, < 和 > 前后移动一帧。",
     "タイムスライダー: つまみをドラッグしてスクラブ、バーをクリックでその位置へ、< と > で 1 フレーム移動します。",
     "Control de tiempo: arrastre el tirador para recorrer, un clic en la barra salta allí, < y > avanzan un fotograma.",
     "Časový posuvník: tažením úchytu se převíjí, klik do lišty skočí tam, < a > posunou o jeden snímek."),
    ("T1MiniKurve", "Mini Curve Editor", "Mini-Kurveneditor", "迷你曲线编辑器", "ミニカーブエディタ", "Mini editor de curvas", "Mini editor křivek"),
    ("T1MiniKurveHint", "Opens the dope sheet and the curve editor above the track bar, as in 3ds Max - the track bar stays the same.",
     "Öffnet Dope Sheet und Kurveneditor über der Spurleiste wie in 3ds Max - die Spurleiste bleibt dieselbe.",
     "像 3ds Max 一样在轨迹栏上方打开摄影表和曲线编辑器 - 轨迹栏保持不变。",
     "3ds Max と同じくトラックバーの上にドープシートとカーブエディタを開きます - トラックバーはそのままです。",
     "Abre la hoja de claves y el editor de curvas sobre la barra de pistas, como en 3ds Max; la barra de pistas sigue igual.",
     "Otevře tabulku klíčů a editor křivek nad stopovou lištou jako v 3ds Max - stopová lišta zůstává stejná."),
    ("T1BildfeldHint", "Current frame: type a number and press Enter to jump there (, and . step one frame).",
     "Aktuelles Bild: eine Zahl eintippen und Enter springt dorthin (, und . gehen ein Bild weiter).",
     "当前帧: 输入数字并按 Enter 跳到该帧 (, 和 . 前后移动一帧)。",
     "現在のフレーム: 数値を入力して Enter でそこへ移動します (, と . で 1 フレーム移動)。",
     "Fotograma actual: escriba un número y pulse Intro para saltar allí (, y . avanzan un fotograma).",
     "Aktuální snímek: napište číslo a Enter tam skočí (, a . posunou o jeden snímek)."),
    ("T1KeineAuswahl", "None Selected", "Nichts ausgewählt", "未选择", "選択なし", "Nada seleccionado", "Nic nevybráno"),
    ("T1AuswahlFlaechen", "%d Surfaces Selected", "%d Flächen ausgewählt", "已选择 %d 个表面", "%d 個のサーフェスを選択", "%d superficies seleccionadas", "Vybráno ploch: %d"),
    ("T1AuswahlEcken", "%d Vertices Selected", "%d Ecken ausgewählt", "已选择 %d 个顶点", "%d 個の頂点を選択", "%d vértices seleccionados", "Vybráno vrcholů: %d"),
    ("T1AuswahlKnochen", "Bone %s Selected", "Knochen %s ausgewählt", "已选择骨骼 %s", "ボーン %s を選択", "Hueso %s seleccionado", "Vybrána kost %s"),
    ("T1HinweisWaehlen", "Click or click-and-drag to select objects", "Klicken oder Rahmen ziehen, um Objekte auszuwählen", "单击或拖动以选择对象",
     "クリックまたはドラッグしてオブジェクトを選択", "Haga clic o arrastre para seleccionar objetos", "Klepnutím nebo tažením vyberte objekty"),
    ("T1HinweisBewegen", "Click and drag to select and move objects", "Klicken und ziehen, um Objekte auszuwählen und zu verschieben", "单击并拖动以选择并移动对象",
     "クリックしてドラッグし、オブジェクトを選択して移動", "Haga clic y arrastre para seleccionar y mover objetos", "Klepnutím a tažením vyberte a posuňte objekty"),
    ("T1HinweisDrehen", "Click and drag to select and rotate objects", "Klicken und ziehen, um Objekte auszuwählen und zu drehen", "单击并拖动以选择并旋转对象",
     "クリックしてドラッグし、オブジェクトを選択して回転", "Haga clic y arrastre para seleccionar y rotar objetos", "Klepnutím a tažením vyberte a otočte objekty"),
    ("T1HinweisSkalieren", "Click and drag to select and scale objects", "Klicken und ziehen, um Objekte auszuwählen und zu skalieren",
     "单击并拖动以选择并缩放对象", "クリックしてドラッグし、オブジェクトを選択して拡大縮小", "Haga clic y arrastre para seleccionar y escalar objetos",
     "Klepnutím a tažením vyberte a změňte měřítko objektů"),
    ("T1Raster", "Grid = %g", "Raster = %g", "栅格 = %g", "グリッド = %g", "Rejilla = %g", "Mřížka = %g"),
    ("T1Tastenzeile", "Key row under the timeline", "Tastenzeile unter der Zeitleiste", "时间轴下方的快捷键行", "タイムライン下のキー行",
     "Fila de teclas bajo la línea de tiempo", "Řádek kláves pod časovou osou"),
    ("T1TastenzeileHint", "Shows the keys of the area in a row under the status bar (off by default, as in 3ds Max).",
     "Zeigt die Tasten des Bereichs in einer Zeile unter der Statuszeile (ab Werk aus, wie in 3ds Max).",
     "在状态栏下方的一行中显示该区域的快捷键 (默认关闭, 与 3ds Max 相同)。",
     "ステータスバーの下の行にこのエリアのキーを表示します (既定はオフ、3ds Max と同じ)。",
     "Muestra las teclas del área en una fila bajo la barra de estado (desactivado de fábrica, como en 3ds Max).",
     "Zobrazí klávesy oblasti v řádku pod stavovým řádkem (ve výchozím stavu vypnuto, jako v 3ds Max)."),
    # --- J: Quad-Menue wie 3ds Max ---
    ("T1QuadTransform", "transform", "Transformieren", "变换", "変換", "transformar", "transformace"),
    ("T1QuadDisplay", "display", "Anzeige", "显示", "表示", "visualización", "zobrazení"),
    ("T1QuadTools1", "tools 1", "Werkzeuge 1", "工具 1", "ツール 1", "herramientas 1", "nástroje 1"),
    ("T1QuadTools2", "tools 2", "Werkzeuge 2", "工具 2", "ツール 2", "herramientas 2", "nástroje 2"),
    ("T1QuadMove", "Move", "Verschieben", "移动", "移動", "Mover", "Posunout"),
    ("T1QuadRotate", "Rotate", "Drehen", "旋转", "回転", "Rotar", "Otočit"),
    ("T1QuadScale", "Scale", "Skalieren", "缩放", "スケール", "Escalar", "Měřítko"),
    ("T1QuadSelect", "Select", "Auswählen", "选择", "選択", "Seleccionar", "Vybrat"),
    ("T1QuadCurveEditor", "Curve Editor...", "Kurveneditor...", "曲线编辑器...", "カーブエディタ...", "Editor de curvas...", "Editor křivek..."),
    ("T1QuadDopeSheet", "Dope Sheet...", "Dope Sheet...", "摄影表...", "ドープシート...", "Hoja de claves...", "Tabulka klíčů..."),
    ("T1QuadIsolate", "Isolate Selection", "Auswahl isolieren", "孤立当前选择", "選択を分離", "Aislar selección", "Izolovat výběr"),
    ("T1QuadEndIsolate", "End Isolate", "Isolieren beenden", "结束隔离", "分離を終了", "Finalizar aislamiento", "Ukončit izolaci"),
    ("T1QuadUnfreezeAll", "Unfreeze All", "Alle Fixierungen aufheben", "全部解冻", "すべてフリーズ解除", "Descongelar todo", "Uvolnit vše"),
    ("T1QuadFreezeSel", "Freeze Selection", "Auswahl fixieren", "冻结当前选择", "選択をフリーズ", "Congelar selección", "Zmrazit výběr"),
    ("T1QuadUnhideAll", "Unhide All", "Alle einblenden", "全部取消隐藏", "すべて表示", "Mostrar todo", "Zobrazit vše"),
    ("T1QuadHideUnsel", "Hide Unselected", "Nicht Ausgewähltes ausblenden", "隐藏未选定对象", "非選択を非表示", "Ocultar no seleccionado", "Skrýt nevybrané"),
    ("T1QuadHideSel", "Hide Selection", "Auswahl ausblenden", "隐藏选定对象", "選択を非表示", "Ocultar selección", "Skrýt výběr"),
    ("T1BlTransform", "Transform", "Transformieren", "变换", "トランスフォーム", "Transformar", "Transformace"),
    # --- N: Motion-Feld mit Setup- und Animationsmodus wie CAT (Blender: Bearbeitungs- und Posenmodus der Armatur) ---
    ("T1nSetup", "Setup Mode", "Setup-Modus", "设置模式", "セットアップモード", "Modo de configuración", "Režim nastavení"),
    ("T1nAnimation", "Animation Mode", "Animationsmodus", "动画模式", "アニメーションモード", "Modo de animación", "Režim animace"),
    ("T1nSetupHint",
     "Like CAT in 3ds Max: build the rig - create the rig and templates, skeleton and GLA, animation.cfg, attachments, build for the game. "
     "The figure stands in its setup pose (the base pose of the GLA), no keys are set.",
     "Wie CAT in 3ds Max: das Rig aufbauen - Rig und Vorlagen erstellen, Skelett und GLA, animation.cfg, Anbauten, für das Spiel bauen. "
     "Die Figur steht in ihrer Setup-Pose (der Grundstellung der GLA), es werden keine Schlüssel gesetzt.",
     "与 3ds Max 的 CAT 相同: 搭建绑定 - 创建绑定和模板、骨架和 GLA、animation.cfg、附件、为游戏构建。"
     "角色处于设置姿态 (GLA 的基础姿态), 不设置关键帧。",
     "3ds Max の CAT と同じ: リグを組み立てます - リグとテンプレートの作成、スケルトンと GLA、animation.cfg、アタッチメント、ゲーム用ビルド。"
     "フィギュアはセットアップポーズ (GLA の基本姿勢) で立ち、キーは設定されません。",
     "Como CAT en 3ds Max: construir el rig - crear el rig y las plantillas, esqueleto y GLA, animation.cfg, accesorios, compilar para el juego. "
     "La figura está en su pose de configuración (la pose base del GLA), no se ponen claves.",
     "Jako CAT v 3ds Max: sestavit rig - vytvořit rig a šablony, kostru a GLA, animation.cfg, přídavky, sestavit pro hru. "
     "Postava stojí ve své pozici nastavení (základní póze GLA), klíče se nenastavují."),
    ("T1nAnimationHint",
     "Like CAT in 3ds Max: animate - sequence, presets, editing, retargeting, IK/FK, auto physics, pose, motion paths, curves and the pose library. "
     "Turning bones and handles sets keys (Auto Key / Set Key).",
     "Wie CAT in 3ds Max: animieren - Sequenz, Vorlagen, Bearbeiten, Übertragen, IK/FK, Auto-Physik, Pose, Bewegungspfade, Kurven und die Posen-Bibliothek. "
     "Drehen an Knochen und Griffen setzt Schlüssel (Auto Key / Set Key).",
     "与 3ds Max 的 CAT 相同: 制作动画 - 序列、预设、编辑、重定向、IK/FK、自动物理、姿态、运动路径、曲线和姿态库。"
     "旋转骨骼和控制柄会设置关键帧 (自动关键点 / 设置关键点)。",
     "3ds Max の CAT と同じ: アニメーション - シーケンス、プリセット、編集、リターゲット、IK/FK、オート物理、ポーズ、モーションパス、カーブ、ポーズライブラリ。"
     "ボーンやハンドルを回すとキーが設定されます (オートキー / キーを設定)。",
     "Como CAT en 3ds Max: animar - secuencia, preajustes, edición, retargeting, IK/FK, física automática, pose, trayectorias, curvas y la biblioteca de poses. "
     "Girar huesos y tiradores pone claves (Auto Key / Set Key).",
     "Jako CAT v 3ds Max: animovat - sekvence, předvolby, úpravy, přenos, IK/FK, automatická fyzika, póza, dráhy pohybu, křivky a knihovna póz. "
     "Otáčení kostmi a úchyty nastavuje klíče (Auto Key / Set Key)."),
    ("T1nUmschalten", "Setup / Animation Mode", "Setup-/Animationsmodus", "设置 / 动画模式", "セットアップ / アニメーションモード",
     "Modo de configuración / animación", "Režim nastavení / animace"),
    ("T1nUmschaltenHint",
     "Switches the Motion panel like the Setup/Animation Mode toggle of CAT (Blender: Edit Mode / Pose Mode of the armature).",
     "Schaltet das Motion-Feld um wie der Schalter Setup-/Animationsmodus von CAT (Blender: Bearbeitungs-/Posenmodus der Armatur).",
     "像 CAT 的设置/动画模式开关一样切换运动面板 (Blender: 骨架的编辑模式/姿态模式)。",
     "CAT のセットアップ/アニメーションモード切り替えと同じようにモーションパネルを切り替えます (Blender: アーマチュアの編集モード/ポーズモード)。",
     "Cambia el panel Movimiento como el conmutador Configuración/Animación de CAT (Blender: Modo Edición / Modo Pose de la armadura).",
     "Přepne panel Pohyb jako přepínač Nastavení/Animace v CAT (Blender: režim úprav / režim pózy armatury)."),
    ("T1nGesperrt",
     "%s: no posing and no keys - the figure stands in its setup pose. %s (%s) poses and sets keys.",
     "%s: kein Posieren und keine Schlüssel - die Figur steht in ihrer Setup-Pose. %s (%s) posiert und setzt Schlüssel.",
     "%s: 不能摆姿态, 不设置关键帧 - 角色处于设置姿态。%s (%s) 可摆姿态并设置关键帧。",
     "%s: ポーズ付けもキー設定もできません - フィギュアはセットアップポーズで立っています。%s (%s) でポーズを付けてキーを設定します。",
     "%s: sin posar y sin claves - la figura está en su pose de configuración. %s (%s) posa y pone claves.",
     "%s: žádné pózování a žádné klíče - postava stojí v pozici nastavení. %s (%s) pózuje a nastavuje klíče."),
    ("T1nStatus", "%s - setup pose, no keys", "%s - Setup-Pose, keine Schlüssel", "%s - 设置姿态, 无关键帧", "%s - セットアップポーズ、キーなし",
     "%s - pose de configuración, sin claves", "%s - pozice nastavení, bez klíčů"),
    # --- O: Painting - das Ziel folgt der Flaeche unter dem Pinsel ---
    ("T1oNurZiel", "Paint target texture only", "Nur Ziel-Textur bemalen", "仅绘制目标纹理", "ターゲットテクスチャのみペイント",
     "Pintar solo la textura de destino", "Malovat jen cílovou texturu"),
    ("T1oNurZielHint",
     "Off (default): a stroke paints into the texture of every surface under the brush, across seams and images, like Viewport Canvas with "
     "Multi/Sub-Object in 3ds Max and Substance Painter. On: only the chosen target is painted (protects neighbouring surfaces; the mask works as well).",
     "Aus (Vorgabe): ein Strich malt in die Textur jeder Fläche unter dem Pinsel, über Nähte und Bilder hinweg, wie Viewport Canvas mit "
     "Multi/Sub-Object in 3ds Max und Substance Painter. An: nur das gewählte Ziel wird bemalt (schützt Nachbarflächen; die Maske wirkt zusätzlich).",
     "关闭 (默认): 一笔会绘制到画笔下每个表面的纹理中, 跨越接缝和图像, 与 3ds Max 中带多维/子对象的视口画布以及 Substance Painter 相同。"
     "打开: 只绘制选定的目标 (保护相邻表面; 遮罩同样有效)。",
     "オフ (既定): ストロークはブラシの下にあるすべてのサーフェスのテクスチャに、シームや画像をまたいでペイントします (3ds Max のマルチ/サブオブジェクト付き"
     "ビューポートキャンバスや Substance Painter と同じ)。オン: 選択したターゲットのみをペイントします (隣のサーフェスを保護、マスクも併用可)。",
     "Desactivado (predeterminado): un trazo pinta en la textura de cada superficie bajo el pincel, a través de costuras e imágenes, como Viewport Canvas "
     "con Multi/Subobjeto en 3ds Max y Substance Painter. Activado: solo se pinta el destino elegido (protege superficies vecinas; la máscara también actúa).",
     "Vypnuto (výchozí): tah maluje do textury každé plochy pod štětcem, přes švy i obrázky, jako Viewport Canvas s Multi/Sub-Object v 3ds Max a "
     "Substance Painter. Zapnuto: maluje se jen zvolený cíl (chrání sousední plochy; maska působí navíc)."),
    ("T1oTexturAufnehmen", "Pick texture", "Textur aufnehmen", "拾取纹理", "テクスチャを拾う", "Tomar textura", "Nabrat texturu"),
    ("T1oTexturAufnehmenHint",
     "Click on the figure: the texture of the surface becomes the target, like Sample in 3ds Max and Blender (Alt+click in the view does the same).",
     "Klick auf die Figur: die Textur der Fläche wird das Ziel, wie Sample in 3ds Max und Blender (Alt+Klick in die Ansicht tut dasselbe).",
     "单击角色: 该表面的纹理成为目标, 与 3ds Max 和 Blender 中的取样相同 (在视图中 Alt+单击效果相同)。",
     "フィギュアをクリック: そのサーフェスのテクスチャがターゲットになります (3ds Max と Blender のサンプルと同じ、ビューで Alt+クリックでも同じ)。",
     "Clic en la figura: la textura de la superficie pasa a ser el destino, como Muestrear en 3ds Max y Blender (Alt+clic en la vista hace lo mismo).",
     "Klik na postavu: textura plochy se stane cílem, jako Sample v 3ds Max a Blenderu (Alt+klik v pohledu dělá totéž)."),
    ("T1oZiel", "Target: %s", "Ziel: %s", "目标: %s", "ターゲット: %s", "Destino: %s", "Cíl: %s"),
    ("T1oVorschauHint", "%s\nClick: paint into this texture (the 2D canvas shows it).",
     "%s\nKlick: in diese Textur malen (die 2D-Leinwand zeigt sie).",
     "%s\n单击: 绘制到此纹理 (2D 画布显示它)。",
     "%s\nクリック: このテクスチャにペイントします (2D キャンバスに表示)。",
     "%s\nClic: pintar en esta textura (el lienzo 2D la muestra).",
     "%s\nKlik: malovat do této textury (2D plátno ji ukazuje)."),
    # --- P: Weights wie der Skin-Modifier von 3ds Max (Select Vertices/Element, Weight Tool, Weight Table, Mirror Paste) ---
    ("T1pAuswahl", "Select", "Auswählen", "选择", "選択", "Seleccionar", "Výběr"),
    ("T1pEcken", "Vertices", "Ecken", "顶点", "頂点", "Vértices", "Vrcholy"),
    ("T1pEckenHint", "Like Select > Vertices of the Skin modifier in 3ds Max: the left mouse selects vertices instead of painting (Ctrl adds, Alt removes, drag a box).", "Wie Auswählen > Ecken des Skin-Modifiers in 3ds Max: die linke Maustaste wählt Ecken statt zu malen (Strg dazu, Alt weg, Rahmen aufziehen).", "与 3ds Max 蒙皮修改器的 选择 > 顶点 相同: 鼠标左键选择顶点而不是绘制 (Ctrl 添加, Alt 移除, 拖出框选)。", "3ds Max のスキンモディファイヤの 選択 > 頂点 と同じ: 左ボタンでペイントせずに頂点を選択します (Ctrl で追加、Alt で除去、ドラッグで矩形)。", "Como Seleccionar > Vértices del modificador Piel de 3ds Max: el botón izquierdo selecciona vértices en lugar de pintar (Ctrl añade, Alt quita, arrastrar un marco).", "Jako Výběr > Vrcholy modifikátoru Skin v 3ds Max: levé tlačítko vybírá vrcholy místo malování (Ctrl přidá, Alt odebere, tažením obdélník)."),
    ("T1pElement", "Select Element", "Element wählen", "选择元素", "要素を選択", "Seleccionar elemento", "Vybrat prvek"),
    ("T1pElementHint", "A click selects all vertices of the part it hits (3ds Max Skin: Select Element).", "Ein Klick wählt alle Ecken des getroffenen Teils (3ds Max Skin: Select Element).", "单击选择被击中部分的所有顶点 (3ds Max 蒙皮: 选择元素)。", "クリックで当たった部分のすべての頂点を選択します (3ds Max スキン: 要素を選択)。", "Un clic selecciona todos los vértices de la parte alcanzada (3ds Max Piel: Seleccionar elemento).", "Klik vybere všechny vrcholy zasaženého dílu (3ds Max Skin: Select Element)."),
    ("T1pRueckseiten", "Backface Cull Vertices", "Rückseiten-Ecken ausblenden", "背面消隐顶点", "背面頂点をカリング", "Descartar vértices traseros", "Vynechat zadní vrcholy"),
    ("T1pRueckseitenHint", "A box selects only vertices facing the view (3ds Max Skin: Backface Cull Vertices).", "Ein Rahmen wählt nur Ecken, die zur Ansicht zeigen (3ds Max Skin: Backface Cull Vertices).", "框选只选择朝向视图的顶点 (3ds Max 蒙皮: 背面消隐顶点)。", "矩形選択はビューを向いた頂点だけを選択します (3ds Max スキン: 背面頂点をカリング)。", "Un marco selecciona solo los vértices orientados a la vista (3ds Max Piel: Backface Cull Vertices).", "Obdélník vybere jen vrcholy obrácené k pohledu (3ds Max Skin: Backface Cull Vertices)."),
    ("T1pListeLeer", "Select vertices (Vertices) - their bones and weights stand here, like the Weight Tool of 3ds Max.", "Ecken wählen (Ecken) - ihre Knochen und Gewichte stehen hier, wie im Weight Tool von 3ds Max.", "选择顶点 (顶点) - 它们的骨骼和权重显示在这里, 与 3ds Max 的权重工具相同。", "頂点を選択 (頂点) - そのボーンとウェイトがここに表示されます (3ds Max のウェイトツールと同じ)。", "Seleccione vértices (Vértices): sus huesos y pesos aparecen aquí, como en la Herramienta de peso de 3ds Max.", "Vyberte vrcholy (Vrcholy) - jejich kosti a váhy stojí zde, jako ve Weight Tool v 3ds Max."),
    ("T1pListeHint", "Click a bone: it becomes the active bone. Type its weight (Enter) or use the fixed values - the sum stays 1. Several vertices: the average.", "Klick auf einen Knochen: er wird der aktive. Sein Gewicht eintippen (Enter) oder die Festwerte nehmen - die Summe bleibt 1. Mehrere Ecken: das Mittel.", "单击骨骼: 它成为活动骨骼。输入其权重 (Enter) 或使用固定值 - 总和保持为 1。多个顶点: 平均值。", "ボーンをクリック: アクティブなボーンになります。ウェイトを入力 (Enter) するか固定値を使用 - 合計は 1 のまま。複数の頂点: 平均値。", "Clic en un hueso: pasa a ser el activo. Escriba su peso (Intro) o use los valores fijos; la suma sigue siendo 1. Varios vértices: la media.", "Klik na kost: stane se aktivní. Zadejte její váhu (Enter) nebo použijte pevné hodnoty - součet zůstane 1. Více vrcholů: průměr."),
    ("T1pKnochen", "Bone", "Knochen", "骨骼", "ボーン", "Hueso", "Kost"),
    ("T1pGewicht", "Weight", "Gewicht", "权重", "ウェイト", "Peso", "Váha"),
    ("T1pKopieren", "Copy", "Kopieren", "复制", "コピー", "Copiar", "Kopírovat"),
    ("T1pKopierenHint", "Copies the weights of the selected vertices (with their places) like Copy in the Weight Tool of 3ds Max.", "Kopiert die Gewichte der gewählten Ecken (mit ihren Orten) wie Copy im Weight Tool von 3ds Max.", "像 3ds Max 权重工具中的复制一样复制所选顶点的权重 (连同位置)。", "3ds Max のウェイトツールのコピーと同じく、選択した頂点のウェイトを (位置ごと) コピーします。", "Copia los pesos de los vértices seleccionados (con sus lugares) como Copiar en la Herramienta de peso de 3ds Max.", "Zkopíruje váhy vybraných vrcholů (s jejich místy) jako Copy ve Weight Tool v 3ds Max."),
    ("T1pEinfuegen", "Paste", "Einfügen", "粘贴", "貼り付け", "Pegar", "Vložit"),
    ("T1pEinfuegenHint", "Every selected vertex gets the copied weights (several copied vertices: their average).", "Jede gewählte Ecke bekommt die kopierten Gewichte (mehrere kopierte Ecken: ihr Mittel).", "每个所选顶点获得复制的权重 (复制了多个顶点: 它们的平均值)。", "選択した各頂点がコピーしたウェイトを受け取ります (複数の頂点をコピーした場合: その平均)。", "Cada vértice seleccionado recibe los pesos copiados (varios vértices copiados: su media).", "Každý vybraný vrchol dostane zkopírované váhy (více zkopírovaných vrcholů: jejich průměr)."),
    ("T1pEinfuegenOrt", "Paste-Pos", "Einfügen nach Ort", "按位置粘贴", "位置で貼り付け", "Pegar por posición", "Vložit podle místa"),
    ("T1pEinfuegenOrtHint", "Every selected vertex gets the weights of the nearest copied vertex within the mirror threshold (3ds Max: Paste-Pos). All surfaces share one skeleton, so Paste-Pos-Bone gives the same.", "Jede gewählte Ecke bekommt die Gewichte der nächsten kopierten Ecke innerhalb der Toleranz des Spiegelns (3ds Max: Paste-Pos). Alle Flächen teilen ein Skelett, Paste-Pos-Bone ergibt dasselbe.", "每个所选顶点获得镜像阈值内最近的复制顶点的权重 (3ds Max: 按位置粘贴)。所有表面共享一个骨架, 按位置和骨骼粘贴结果相同。", "選択した各頂点が、ミラーしきい値内で最も近いコピー済み頂点のウェイトを受け取ります (3ds Max: Paste-Pos)。全サーフェスが 1 つのスケルトンを共有するため Paste-Pos-Bone も同じ結果です。", "Cada vértice seleccionado recibe los pesos del vértice copiado más cercano dentro del umbral de simetría (3ds Max: Paste-Pos). Todas las superficies comparten un esqueleto: Paste-Pos-Bone da lo mismo.", "Každý vybraný vrchol dostane váhy nejbližšího zkopírovaného vrcholu v rámci tolerance zrcadlení (3ds Max: Paste-Pos). Všechny plochy sdílejí jednu kostru, Paste-Pos-Bone dává totéž."),
    ("T1pKopiert", "%d vertices copied", "%d Ecken kopiert", "已复制 %d 个顶点", "%d 個の頂点をコピーしました", "%d vértices copiados", "Zkopírováno %d vrcholů"),
    ("T1pTabelle", "Weight Table", "Gewichtstabelle", "权重表", "ウェイトテーブル", "Tabla de pesos", "Tabulka vah"),
    ("T1pTabelleHint", "Vertices x bones, every cell can be typed (3ds Max Skin: Weight Table). A window that can be docked.", "Ecken x Knochen, jede Zelle eintippbar (3ds Max Skin: Weight Table). Ein Fenster, das sich andocken lässt.", "顶点 x 骨骼, 每个单元格都可输入 (3ds Max 蒙皮: 权重表)。可停靠的窗口。", "頂点 x ボーン、各セルに入力できます (3ds Max スキン: ウェイトテーブル)。ドッキング可能なウィンドウです。", "Vértices x huesos, cada celda se puede escribir (3ds Max Piel: Tabla de pesos). Una ventana que se puede acoplar.", "Vrcholy x kosti, do každé buňky lze psát (3ds Max Skin: Weight Table). Okno, které lze ukotvit."),
    ("T1pNurGewaehlte", "Selected vertices only", "Nur gewählte Ecken", "仅所选顶点", "選択した頂点のみ", "Solo vértices seleccionados", "Jen vybrané vrcholy"),
    ("T1pEcke", "Vertex", "Ecke", "顶点", "頂点", "Vértice", "Vrchol"),
    ("T1pSpiegelPaste", "Mirror Paste", "Spiegeln einfügen", "镜像粘贴", "ミラー貼り付け", "Pegar simetría", "Zrcadlově vložit"),
    ("T1pSpiegelPasteHint", "Pastes the weights of the selected vertices onto their counterparts on the other side, bones swapped left/right (3ds Max Mirror Mode: Mirror Paste).", "Fügt die Gewichte der gewählten Ecken auf ihren Gegenstücken der anderen Seite ein, Knochen links/rechts getauscht (3ds Max Mirror Mode: Mirror Paste).", "将所选顶点的权重粘贴到另一侧的对应顶点上, 骨骼左右交换 (3ds Max 镜像模式: 镜像粘贴)。", "選択した頂点のウェイトを反対側の対応頂点に貼り付けます、ボーンは左右入れ替え (3ds Max ミラーモード: ミラー貼り付け)。", "Pega los pesos de los vértices seleccionados en sus homólogos del otro lado, con los huesos intercambiados izquierda/derecha (3ds Max Modo simetría: Pegar simetría).", "Vloží váhy vybraných vrcholů na jejich protějšky na druhé straně, kosti prohozené vlevo/vpravo (3ds Max Mirror Mode: Mirror Paste)."),
    ("T1pPaare", "Bone pairs", "Knochenpaare", "骨骼对", "ボーンのペア", "Pares de huesos", "Páry kostí"),
    # --- TEST1411e: Load mission - waehrend des Lesens springt nichts ---
    ("T1eNeuGelesen", "Just read (sorted into the groups when the search is done)", "Gerade gelesen (kommt am Ende der Suche in die Gruppen)",
     "刚读取 (搜索结束后归入分组)", "読み込んだばかり (検索終了後にグループへ整理)", "Recién leídas (se ordenan en los grupos al terminar la búsqueda)",
     "Právě načtené (po dokončení hledání se zařadí do skupin)"),
    # --- TEST1411e: Knochenpaare von Hand (wie die Zuordnung im Mirror Mode von 3ds Max) ---
    ("T1ePaarHint",
     "Mirror partner of this bone. Pick another bone to pair them by hand (both leave their old pairs), or - for none. Pairs set by hand go before the names.",
     "Spiegelpartner dieses Knochens. Einen anderen Knochen wählen, um beide von Hand zu paaren (beide verlassen ihre alten Paare), oder - für keinen. Paare von Hand gehen vor den Namen.",
     "该骨骼的镜像伙伴。选择另一根骨骼即可手动配对 (两者都离开原来的配对), 选择 - 表示无。手动设置的配对优先于名称。",
     "このボーンのミラー相手です。別のボーンを選ぶと手動でペアになります (両方とも元のペアから外れます)。- はなし。手動のペアは名前より優先されます。",
     "Pareja de espejo de este hueso. Elija otro hueso para emparejarlos a mano (ambos dejan sus parejas anteriores) o - para ninguno. Las parejas a mano van antes que los nombres.",
     "Zrcadlový partner této kosti. Vyberte jinou kost a spárujte je ručně (obě opustí své staré páry), nebo - pro žádnou. Ruční páry mají přednost před názvy."),
    ("T1ePaarHand", "Set by hand (goes before the names).", "Von Hand gesetzt (geht vor den Namen).", "手动设置 (优先于名称)。", "手動で設定 (名前より優先)。",
     "Puesto a mano (va antes que los nombres).", "Nastaveno ručně (má přednost před názvy)."),
    ("T1ePaarNeu", "New pair...", "Neues Paar...", "新配对...", "新しいペア...", "Nueva pareja...", "Nový pár..."),
    ("T1ePaarNeuHint",
     "Pair any two bones by hand: first the left (blue) bone, then its right (green) partner - also bones whose names the rule does not know (e.g. Bip01 L Hand).",
     "Zwei beliebige Knochen von Hand paaren: zuerst den linken (blauen) Knochen, dann seinen rechten (grünen) Partner - auch Knochen, deren Namen die Regel nicht kennt (z. B. Bip01 L Hand).",
     "手动配对任意两根骨骼: 先选左侧 (蓝色) 骨骼, 再选其右侧 (绿色) 伙伴 - 也适用于规则不认识名称的骨骼 (例如 Bip01 L Hand)。",
     "任意の 2 本のボーンを手動でペアにします: まず左 (青) のボーン、次に右 (緑) の相手 - 規則が名前を認識しないボーンにも使えます (例: Bip01 L Hand)。",
     "Emparejar a mano dos huesos cualesquiera: primero el hueso izquierdo (azul), luego su pareja derecha (verde); también huesos cuyos nombres la regla no conoce (p. ej. Bip01 L Hand).",
     "Ručně spárovat libovolné dvě kosti: nejprve levou (modrou) kost, pak jejího pravého (zeleného) partnera - i kosti, jejichž názvy pravidlo nezná (např. Bip01 L Hand)."),
    ("T1ePaareNamen", "Pairs from names", "Paare nach den Namen", "按名称配对", "名前でペア", "Parejas por nombre", "Páry podle názvů"),
    ("T1ePaareNamenHint",
     "Drops the pairs set by hand: the bones pair by their names again (l_/r_, _l/_r, lhand/rhand).",
     "Verwirft die Paare von Hand: die Knochen paaren sich wieder nach ihren Namen (l_/r_, _l/_r, lhand/rhand).",
     "放弃手动配对: 骨骼再次按名称配对 (l_/r_、_l/_r、lhand/rhand)。",
     "手動のペアを破棄します: ボーンは再び名前でペアになります (l_/r_、_l/_r、lhand/rhand)。",
     "Descarta las parejas a mano: los huesos vuelven a emparejarse por sus nombres (l_/r_, _l/_r, lhand/rhand).",
     "Zahodí ruční páry: kosti se opět párují podle názvů (l_/r_, _l/_r, lhand/rhand)."),
    ("T1ePaarGesetzt", "Mirror pair set by hand: %s <-> %s.", "Spiegelpaar von Hand gesetzt: %s <-> %s.", "已手动设置镜像配对: %s <-> %s。", "ミラーペアを手動で設定しました: %s <-> %s。",
     "Pareja de espejo puesta a mano: %s <-> %s.", "Zrcadlový pár nastaven ručně: %s <-> %s."),
    ("T1ePaareNamenFertig", "The bones pair by their names again.", "Die Knochen paaren sich wieder nach ihren Namen.", "骨骼再次按名称配对。", "ボーンは再び名前でペアになります。",
     "Los huesos vuelven a emparejarse por sus nombres.", "Kosti se opět párují podle názvů."),
    ("T1pBlVertexGewichte", "Vertex Weights", "Vertex-Gewichte", "顶点权重", "頂点ウェイト", "Pesos de vértice", "Váhy vrcholů"),
    # --- A: Mission laden ohne pk3-Dialog ---
    ("T1aOrdnerOeffnen", "Open folder...", "Ordner öffnen...", "打开文件夹...", "フォルダーを開く...", "Abrir carpeta...", "Otevřít složku..."),
    ("T1aOrdnerOeffnenHint", "A game or mod folder (e.g. GameData\\base, GameData Movie Duels\\MD or an unpacked mod): its missions, for this session only.", "Ein Spiel- oder Mod-Ordner (z. B. GameData\\base, GameData Movie Duels\\MD oder eine entpackte Mod): seine Missionen, nur für diese Sitzung.", "游戏或模组文件夹 (例如 GameData\\base、GameData Movie Duels\\MD 或解压的模组): 其任务, 仅限本次会话。", "ゲームまたは MOD のフォルダー (例: GameData\\base、GameData Movie Duels\\MD、展開した MOD): そのミッション、このセッションのみ。", "Una carpeta del juego o de un mod (p. ej. GameData\\base, GameData Movie Duels\\MD o un mod descomprimido): sus misiones, solo en esta sesión.", "Složka hry nebo modu (např. GameData\\base, GameData Movie Duels\\MD nebo rozbalený mod): její mise, jen pro tuto relaci."),
    ("T1aArchivOeffnen", "Open archive...", "Archiv öffnen...", "打开存档...", "アーカイブを開く...", "Abrir archivo...", "Otevřít archiv..."),
    ("T1aArchivOeffnenHint", "A single .pk3 (e.g. a downloaded mission): its missions stand first.", "Eine einzelne .pk3 (z. B. eine heruntergeladene Mission): ihre Missionen stehen vorn.", "单个 .pk3 (例如下载的任务): 其任务排在前面。", "単一の .pk3 (例: ダウンロードしたミッション): そのミッションが先頭に並びます。", "Un solo .pk3 (p. ej. una misión descargada): sus misiones aparecen primero.", "Jediný .pk3 (např. stažená mise): jeho mise stojí vpředu."),
    ("T1aEinrichten", "Set up game folders...", "Spielordner einrichten...", "设置游戏文件夹...", "ゲームフォルダーを設定...", "Configurar carpetas del juego...", "Nastavit složky hry..."),
    ("T1aNichtsEingerichtet", "No game folders set up yet. Open a game or mod folder, a single .pk3, or set up the game folders.", "Noch keine Spielordner eingerichtet. Einen Spiel- oder Mod-Ordner öffnen, eine einzelne .pk3, oder die Spielordner einrichten.", "尚未设置游戏文件夹。打开游戏或模组文件夹、单个 .pk3, 或设置游戏文件夹。", "ゲームフォルダーがまだ設定されていません。ゲームまたは MOD のフォルダー、単一の .pk3 を開くか、ゲームフォルダーを設定してください。", "Aún no hay carpetas del juego configuradas. Abra una carpeta del juego o de un mod, un solo .pk3, o configure las carpetas del juego.", "Zatím nejsou nastaveny žádné složky hry. Otevřete složku hry nebo modu, jediný .pk3, nebo nastavte složky hry."),
    ("T1aOhneSkripte", "Maps without scripts (duel maps)", "Karten ohne Skripte (Duellkarten)", "无脚本的地图 (对决地图)", "スクリプトのないマップ (デュエルマップ)", "Mapas sin scripts (mapas de duelo)", "Mapy bez skriptů (duelové mapy)"),
    ("T1aOrdnerWahl", "Choose a game or mod folder", "Spiel- oder Mod-Ordner wählen", "选择游戏或模组文件夹", "ゲームまたは MOD のフォルダーを選択", "Elegir una carpeta del juego o de un mod", "Vyberte složku hry nebo modu"),
    # --- E: Sequenzen als eigener Reiter mit Suche und Spalten ---
    ("T1eSequenzen", "Sequences", "Sequenzen", "序列", "シーケンス", "Secuencias", "Sekvence"),
    ("T1eSequenzenHint", "All sequences of the animation.cfg as their own list: search, group, columns name | frames | loop (drag the width, click a heading to sort). Click plays, right-click: play as secondary ...", "Alle Sequenzen der animation.cfg als eigene Liste: Suche, Gruppe, Spalten Name | Bilder | Schleife (Breite ziehen, Klick auf den Kopf sortiert). Klick spielt ab, Rechtsklick: als Sekundär abspielen ...", "animation.cfg 的所有序列作为独立列表: 搜索、分组、列 名称 | 帧数 | 循环 (拖动宽度, 单击列标题排序)。单击播放, 右键: 作为次要播放 ...", "animation.cfg のすべてのシーケンスを独立したリストで表示: 検索、グループ、列 名前 | フレーム | ループ (幅をドラッグ、見出しをクリックで並べ替え)。クリックで再生、右クリック: セカンダリとして再生 ...", "Todas las secuencias de animation.cfg como lista propia: búsqueda, grupo, columnas nombre | fotogramas | bucle (arrastrar el ancho, clic en la cabecera ordena). Clic reproduce, clic derecho: reproducir como secundaria ...", "Všechny sekvence animation.cfg jako vlastní seznam: hledání, skupina, sloupce název | snímky | smyčka (táhnout šířku, klik na hlavičku řadí). Klik přehraje, pravý klik: přehrát jako sekundární ..."),
    ("T1eSucheHint", "Part of the name, upper/lower case does not matter, filters while typing. Ctrl+F (mouse over the explorer) puts the cursor here.", "Ein Teil des Namens, Groß/Klein egal, filtert beim Tippen. Strg+F (Maus über dem Explorer) setzt den Cursor hierher.", "名称的一部分, 不区分大小写, 输入时即筛选。Ctrl+F (鼠标在资源管理器上) 将光标移到这里。", "名前の一部、大文字小文字は区別せず、入力中に絞り込みます。Ctrl+F (マウスをエクスプローラーの上に) でここにカーソル。", "Parte del nombre, sin distinguir mayúsculas, filtra al escribir. Ctrl+F (ratón sobre el explorador) pone el cursor aquí.", "Část názvu, na velikosti písmen nezáleží, filtruje při psaní. Ctrl+F (myš nad průzkumníkem) sem umístí kurzor."),
    ("T1eBilder", "Frames", "Bilder", "帧数", "フレーム", "Fotogramas", "Snímky"),
    ("T1eSchleife", "Loop", "Schleife", "循环", "ループ", "Bucle", "Smyčka"),
    # --- B: der Model-Tab ohne Modell wie mit Modell ---
    ("T1bLeerTitel", "No model loaded", "Kein Modell geladen", "未加载模型", "モデルが読み込まれていません", "No hay ningún modelo cargado", "Není načten žádný model"),
    ("T1bNpcBrowser", "NPC browser...", "NPC-Browser...", "NPC 浏览器...", "NPC ブラウザー...", "Navegador de NPC...", "Prohlížeč NPC..."),
    ("T1bFeldHinweis", "No model loaded - the groups of this area open once a model is loaded.", "Kein Modell geladen - die Gruppen dieses Bereichs öffnen sich, sobald ein Modell geladen ist.", "未加载模型 - 加载模型后即可展开此区域的组。", "モデルが読み込まれていません - モデルを読み込むと、この領域のグループを開けます。", "No hay ningún modelo cargado: los grupos de esta área se abren al cargar un modelo.", "Není načten žádný model - skupiny této oblasti se otevřou po načtení modelu."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
