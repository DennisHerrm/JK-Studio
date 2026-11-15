"""TEST1411 (14.11.2026): Handbuch zu den Testrueckmeldungen von shank zu rc612 - der Model-Tab nach den Punkten A bis P.

Bausteine hinter Absaetzen anderer Quellen (Register tools/handbuch/inhalt_modell.py, je eine Zeile in QUELLEN):
    Hinter("inhalt_test1411", "bloecke_leer", "modell-ansehen", "inhalt_mb1a1", "aufbau"),         # B leerer Model-Tab
    Hinter("inhalt_test1411", "bloecke_sequenzen", "modell-ansehen", "inhalt_ap2", "baum"),         # E Sequenzen im eigenen Reiter
    Hinter("inhalt_test1411", "bloecke_ansicht", "modell-ansehen", "inhalt_mbkopf", "cube"),        # D, L, M Ansicht und ViewCube
    Hinter("inhalt_test1411", "bloecke_quad", "modell-bedienung", "inhalt_mb1a2", "b_text"),        # J Quad-Menue
    Hinter("inhalt_test1411", "bloecke_motion", "modell-animationsfeld", "inhalt_ap10", "gruppen"), # N, I Setup/Animation, Knochen
    Hinter("inhalt_test1411", "bloecke_knochen", "modell-knochen", "inhalt_ap13k", "knochen"),      # H, K Kette je Glied, Fussplattform
    Hinter("inhalt_test1411", "bloecke_painting", "modell-painting", "inhalt_ap9", "feld"),         # O Ziel folgt dem Pinsel
    Hinter("inhalt_test1411", "bloecke_weights", "modell-weights", "inhalt_ap8", "werkzeug"),       # P wie der Skin-Modifier
    Hinter("inhalt_test1411", "bloecke_gla", "modell-dateien", "inhalt_mb2", "gla"),                # C Load .gla aus den pk3
F und G stehen im Absatz "aufbau" von inhalt_ap1.py (dort neu), E zum Baum im Absatz "baum" von inhalt_ap2.py, A in inhalt_mission.py.
Jeder Text in sechs Sprachen (en, de, zh, ja, es, cs).
"""

X = {
    # --- B ---
    "leer": (
        "**Without a model** the tab already looks as it does with one: the scene explorer with its tabs, search and filters (greyed out; the hint and "
        "**{{EmptyShowHow}}** stand in the empty tree), the views with grid, ViewCube and header, and the command panel with its tabs and the groups of the "
        "area, closed and greyed out. In the middle of the active view **{{T1bLeerTitel}}** offers **{{T1bNpcBrowser}}** and **{{OpenGlm}}**.",
        "**Ohne Modell** sieht der Tab schon so aus wie mit einem: der Szenen-Explorer mit Reitern, Suche und Filtern (ausgegraut; der Hinweis und "
        "**{{EmptyShowHow}}** stehen im leeren Baum), die Ansichten mit Raster, ViewCube und Kopfzeile und das Befehlsfeld mit seinen Reitern und den Gruppen "
        "des Bereichs, zugeklappt und ausgegraut. Mitten in der aktiven Ansicht bietet **{{T1bLeerTitel}}** die Knöpfe **{{T1bNpcBrowser}}** und **{{OpenGlm}}** an.",
        "**未加载模型时**, 选项卡的布局已与加载后相同: 场景资源管理器带选项卡、搜索和筛选 (灰显; 提示和 **{{EmptyShowHow}}** 位于空树中), 视图带网格、ViewCube "
        "和标题栏, 命令面板带选项卡和该区域的组 (折叠并灰显)。活动视图中央的 **{{T1bLeerTitel}}** 提供 **{{T1bNpcBrowser}}** 和 **{{OpenGlm}}**。",
        "**モデルがなくても**、タブは読み込み後と同じ配置です: タブ・検索・フィルター付きのシーン エクスプローラ (グレー表示、ヒントと **{{EmptyShowHow}}** は空のツリー"
        "に表示)、グリッド・ViewCube・ヘッダー付きのビュー、タブとエリアのグループ (折りたたまれてグレー表示) を持つコマンド パネル。アクティブなビューの中央の "
        "**{{T1bLeerTitel}}** に **{{T1bNpcBrowser}}** と **{{OpenGlm}}** があります。",
        "**Sin modelo** la pestaña ya tiene el mismo aspecto que con uno: el explorador de escena con sus pestañas, búsqueda y filtros (en gris; la indicación y "
        "**{{EmptyShowHow}}** están en el árbol vacío), las vistas con cuadrícula, ViewCube y cabecera, y el panel de comandos con sus pestañas y los grupos del "
        "área, cerrados y en gris. En el centro de la vista activa, **{{T1bLeerTitel}}** ofrece **{{T1bNpcBrowser}}** y **{{OpenGlm}}**.",
        "**Bez modelu** vypadá karta stejně jako s ním: průzkumník scény s kartami, hledáním a filtry (zašedlé; nápověda a **{{EmptyShowHow}}** stojí v prázdném "
        "stromu), pohledy s mřížkou, ViewCube a záhlavím a příkazový panel s kartami a skupinami oblasti, sbalenými a zašedlými. Uprostřed aktivního pohledu "
        "nabízí **{{T1bLeerTitel}}** tlačítka **{{T1bNpcBrowser}}** a **{{OpenGlm}}**."),
    # --- E ---
    "sequenzen": (
        "**Sequences in their own tab:** the third tab **{{T1eSequenzen}}** next to **{{MbNachHierarchie}}** and **{{MbNachEbenen}}** lists all sequences of the "
        "`animation.cfg` with a search field (part of the name, any case, filters while typing; <kbd>Ctrl</kbd>+<kbd>F</kbd> with the mouse over the explorer), "
        "the group (All, BOTH, TORSO, LEGS, FACE) and the columns name, frames and loop - drag the width, click a heading to sort. A click or double-click plays "
        "a sequence, the right-click menu plays it as the secondary one; the primary and the secondary sequence carry their lock with **P** and **S**. A row can "
        "be dragged onto a slot of the map GLA. In the tree one entry with the number of sequences jumps to this tab.",
        "**Sequenzen im eigenen Reiter:** der dritte Reiter **{{T1eSequenzen}}** neben **{{MbNachHierarchie}}** und **{{MbNachEbenen}}** listet alle Sequenzen der "
        "`animation.cfg` mit einem Suchfeld (ein Teil des Namens, Groß/Klein egal, filtert beim Tippen; <kbd>Strg</kbd>+<kbd>F</kbd> mit der Maus über dem "
        "Explorer), der Gruppe (Alle, BOTH, TORSO, LEGS, FACE) und den Spalten Name, Bilder und Schleife - Breite ziehen, ein Klick auf den Kopf sortiert. Klick "
        "oder Doppelklick spielt eine Sequenz ab, das Rechtsklick-Menü spielt sie als Sekundärsequenz; Primär- und Sekundärsequenz tragen ihr Schloss mit **P** "
        "und **S**. Eine Zeile lässt sich auf einen Platz der Karten-GLA ziehen. Im Baum springt ein Eintrag mit der Zahl der Sequenzen zu diesem Reiter.",
        "**序列位于独立选项卡:** **{{MbNachHierarchie}}** 和 **{{MbNachEbenen}}** 旁边的第三个选项卡 **{{T1eSequenzen}}** 列出 `animation.cfg` 的所有序列, 带搜索框 "
        "(名称的一部分, 不区分大小写, 输入即筛选; 鼠标在资源管理器上时按 <kbd>Ctrl</kbd>+<kbd>F</kbd>)、分组 (全部、BOTH、TORSO、LEGS、FACE) 以及名称、帧数和循环列 - "
        "可拖动宽度, 单击标题排序。单击或双击播放序列, 右键菜单将其作为次要序列播放; 主要和次要序列带锁以及 **P** 和 **S**。行可拖到地图 GLA 的槽位上。树中带序列数量"
        "的条目可跳到此选项卡。",
        "**シーケンスは独自のタブに:** **{{MbNachHierarchie}}** と **{{MbNachEbenen}}** の隣の 3 番目のタブ **{{T1eSequenzen}}** に `animation.cfg` のすべての"
        "シーケンスが、検索欄 (名前の一部、大文字小文字を区別せず入力中に絞り込み; エクスプローラ上で <kbd>Ctrl</kbd>+<kbd>F</kbd>)、グループ (すべて、BOTH、TORSO、"
        "LEGS、FACE)、名前・フレーム・ループの列付きで並びます - 幅はドラッグ、見出しのクリックで並べ替え。クリックまたはダブルクリックで再生、右クリック メニューで"
        "セカンダリとして再生; プライマリとセカンダリには **P** と **S** の錠が付きます。行はマップ GLA のスロットへドラッグできます。ツリーではシーケンス数の付いた"
        "項目からこのタブへ移れます。",
        "**Las secuencias en su propia pestaña:** la tercera pestaña **{{T1eSequenzen}}**, junto a **{{MbNachHierarchie}}** y **{{MbNachEbenen}}**, lista todas las "
        "secuencias del `animation.cfg` con un campo de búsqueda (parte del nombre, sin distinguir mayúsculas, filtra al escribir; <kbd>Ctrl</kbd>+<kbd>F</kbd> con "
        "el ratón sobre el explorador), el grupo (Todas, BOTH, TORSO, LEGS, FACE) y las columnas nombre, fotogramas y bucle: arrastre el ancho, un clic en la "
        "cabecera ordena. Un clic o doble clic reproduce una secuencia, el menú contextual la reproduce como secundaria; la primaria y la secundaria llevan su "
        "candado con **P** y **S**. Una fila se puede arrastrar a una ranura de la GLA del mapa. En el árbol, una entrada con el número de secuencias salta a "
        "esta pestaña.",
        "**Sekvence na vlastní kartě:** třetí karta **{{T1eSequenzen}}** vedle **{{MbNachHierarchie}}** a **{{MbNachEbenen}}** uvádí všechny sekvence "
        "`animation.cfg` s polem hledání (část názvu, bez ohledu na velikost písmen, filtruje při psaní; <kbd>Ctrl</kbd>+<kbd>F</kbd> s myší nad průzkumníkem), "
        "skupinou (Vše, BOTH, TORSO, LEGS, FACE) a sloupci název, snímky a smyčka - šířku lze táhnout, klik na záhlaví řadí. Klik nebo dvojklik sekvenci přehraje, "
        "kontextové menu ji přehraje jako sekundární; primární a sekundární sekvence nesou zámek s **P** a **S**. Řádek lze přetáhnout na slot GLA mapy. Ve "
        "stromu vede položka s počtem sekvencí na tuto kartu."),
    # --- D, L, M ---
    "ansicht": (
        "The ViewCube hides what lies behind it: compass letters behind the cube are covered and cannot be clicked, a click on a visible face, edge or corner "
        "always wins over the ring, and the view turns smoothly without swinging. All labels over the view - header, Home, ViewCube with compass, hints and "
        "measurements - turn white or black by the brightness of the background and stay readable on silhouette, white or any colour. On the object level the "
        "model is shaded without a wireframe, as in 3ds Max and Blender; edges show on the sub-object levels (like Blender's Edit Mode thin and dark). Ghosts "
        "are drawn only in the area **{{MbBereichAnimation}}**; their switch also sits in the **[+]** menu of the view.",
        "Der ViewCube verdeckt, was hinter ihm liegt: Kompassbuchstaben hinter dem Würfel sind verdeckt und nicht anklickbar, ein Klick auf eine sichtbare "
        "Fläche, Kante oder Ecke geht immer vor dem Ring, und die Ansicht dreht weich, ohne nachzuschwingen. Alle Beschriftungen über der Ansicht - Kopfzeile, "
        "Home, ViewCube mit Kompass, Hinweise und Messwerte - werden je nach Helligkeit des Grunds weiß oder schwarz und bleiben auf Silhouette, Weiß oder jeder "
        "Farbe lesbar. Auf der Objektebene ist das Modell wie in 3ds Max und Blender schattiert ohne Drahtgitter; Kanten zeigen die Unterobjekt-Ebenen (wie "
        "Blenders Bearbeitungsmodus dünn und dunkel). Geister zeichnet nur der Bereich **{{MbBereichAnimation}}**; ihr Schalter steht auch im Menü **[+]** der Ansicht.",
        "ViewCube 会遮挡其后方的内容: 立方体后面的指南针字母被遮挡且无法单击, 单击可见的面、边或角总是优先于圆环, 视图平滑旋转而不来回摆动。视图上方的所有标签 - "
        "标题栏、主页、带指南针的 ViewCube、提示和测量值 - 会根据背景亮度变为白色或黑色, 在剪影、白色或任何颜色上都清晰可读。在对象层级, 模型与 3ds Max 和 Blender "
        "相同, 以着色方式显示而无线框; 边在子对象层级显示 (与 Blender 的编辑模式相同, 细而暗)。残影只在 **{{MbBereichAnimation}}** 区域绘制; 其开关也在视图的 "
        "**[+]** 菜单中。",
        "ViewCube は後ろにあるものを隠します: キューブの後ろの方位文字は隠れてクリックできず、見えている面・エッジ・コーナーのクリックは常にリングより優先され、ビューは"
        "揺り戻しなく滑らかに回ります。ビュー上のすべてのラベル - ヘッダー、ホーム、方位付き ViewCube、ヒント、計測値 - は背景の明るさに応じて白または黒になり、"
        "シルエット・白・どの色の上でも読めます。オブジェクト レベルではモデルは 3ds Max や Blender と同じくワイヤーフレームなしでシェーディング表示され、エッジは"
        "サブオブジェクト レベルで表示されます (Blender の編集モードと同じく細く暗く)。ゴーストは **{{MbBereichAnimation}}** エリアだけで描かれ、そのスイッチはビューの "
        "**[+]** メニューにもあります。",
        "El ViewCube oculta lo que hay detrás: las letras de la brújula detrás del cubo quedan tapadas y no se pueden pulsar, un clic en una cara, arista o "
        "esquina visible siempre tiene prioridad sobre el anillo, y la vista gira suavemente sin oscilar. Todas las etiquetas sobre la vista (cabecera, Inicio, "
        "ViewCube con brújula, indicaciones y medidas) se vuelven blancas o negras según el brillo del fondo y se leen sobre la silueta, el blanco o cualquier "
        "color. En el nivel de objeto el modelo se sombrea sin malla de alambre, como en 3ds Max y Blender; las aristas aparecen en los subniveles de objeto "
        "(como en el modo Edición de Blender, finas y oscuras). Los fantasmas solo se dibujan en el área **{{MbBereichAnimation}}**; su interruptor también está "
        "en el menú **[+]** de la vista.",
        "ViewCube zakrývá, co leží za ním: písmena kompasu za krychlí jsou zakrytá a nelze na ně kliknout, klik na viditelnou stěnu, hranu nebo roh má vždy "
        "přednost před prstencem a pohled se otáčí plynule bez kmitání. Všechny popisky nad pohledem - záhlaví, Domů, ViewCube s kompasem, nápovědy a měření - "
        "jsou podle jasu pozadí bílé nebo černé a zůstávají čitelné na siluetě, bílé i jakékoli barvě. Na úrovni objektu je model jako v 3ds Max a Blenderu "
        "stínovaný bez drátového modelu; hrany ukazují podúrovně objektu (jako režim úprav Blenderu tenké a tmavé). Duchové se kreslí jen v oblasti "
        "**{{MbBereichAnimation}}**; jejich přepínač je také v menu **[+]** pohledu."),
    # --- J ---
    "quad": (
        "**Right-click in a view like 3ds Max: the quad menu.** Four quadrants meet at the mouse: **transform** at the bottom right (Move, Rotate, Scale, Select, "
        "Clone, Curve Editor, Dope Sheet ...), **display** at the top right (Isolate Selection, Unfreeze All, Freeze Selection, Unhide All, Hide Unselected, Hide "
        "Selection), **tools 1** at the top left (the sub-object levels and the tools of the area) and **tools 2** at the bottom left (display, weights, "
        "animation, script, measure). Move, Rotate and Scale set the tool like W, E and R; the active tool is marked. Submenus open outwards - to the left in "
        "the left quadrants, to the right in the right ones; at the edge of the window the whole quad menu moves inwards. Like Blender the right-click opens the "
        "context menu of the mode, with **{{T1BlTransform}}** (G, R, S) at the top.",
        "**Rechtsklick in eine Ansicht wie 3ds Max: das Quad-Menü.** Vier Viertel treffen sich an der Maus: **transform** rechts unten (Move, Rotate, Scale, Select, "
        "Clone, Curve Editor, Dope Sheet ...), **display** rechts oben (Isolate Selection, Unfreeze All, Freeze Selection, Unhide All, Hide Unselected, Hide "
        "Selection), **tools 1** links oben (die Unterobjekt-Ebenen und die Werkzeuge des Bereichs) und **tools 2** links unten (Anzeige, Weights, Animation, "
        "Skript, Messen). Move, Rotate und Scale setzen das Werkzeug wie W, E und R; das aktive Werkzeug ist markiert. Untermenüs gehen nach außen auf - in den "
        "linken Vierteln nach links, in den rechten nach rechts; am Rand des Fensters rückt das ganze Quad-Menü nach innen. Wie Blender öffnet der Rechtsklick "
        "das Kontextmenü des Modus, oben mit **{{T1BlTransform}}** (G, R, S).",
        "**像 3ds Max 一样在视图中右键: 四元菜单。** 四个象限在鼠标处相交: 右下为 **transform** (移动、旋转、缩放、选择、克隆、曲线编辑器、摄影表 ……), 右上为 "
        "**display** (孤立当前选择、全部解冻、冻结当前选择、全部取消隐藏、隐藏未选定对象、隐藏当前选择), 左上为 **tools 1** (子对象层级和该区域的工具), 左下为 "
        "**tools 2** (显示、权重、动画、脚本、测量)。移动、旋转和缩放与 W、E、R 一样设置工具; 当前工具会被标记。子菜单向外展开 - 左侧象限向左, 右侧象限向右; 在窗口"
        "边缘整个四元菜单会向内移动。像 Blender 时, 右键打开当前模式的上下文菜单, 顶部为 **{{T1BlTransform}}** (G、R、S)。",
        "**3ds Max と同じくビューで右クリック: クアッド メニュー。** 4 つの区画がマウス位置で接します: 右下が **transform** (移動、回転、スケール、選択、クローン、"
        "カーブ エディタ、ドープ シート …)、右上が **display** (選択を分離、すべてフリーズ解除、選択をフリーズ、すべて表示、非選択を隠す、選択を隠す)、左上が "
        "**tools 1** (サブオブジェクト レベルとエリアのツール)、左下が **tools 2** (表示、ウェイト、アニメーション、スクリプト、計測)。移動・回転・スケールは W・E・R と"
        "同じくツールを設定し、アクティブなツールには印が付きます。サブメニューは外側に開きます - 左の区画は左へ、右の区画は右へ。ウィンドウの端ではクアッド メニュー"
        "全体が内側へ移動します。Blender 風では右クリックでモードのコンテキスト メニューが開き、先頭に **{{T1BlTransform}}** (G、R、S) があります。",
        "**Clic derecho en una vista como en 3ds Max: el menú cuádruple.** Cuatro cuadrantes se juntan en el ratón: **transform** abajo a la derecha (Mover, Rotar, "
        "Escalar, Seleccionar, Clonar, Editor de curvas, Hoja de claves...), **display** arriba a la derecha (Aislar selección, Descongelar todo, Congelar "
        "selección, Mostrar todo, Ocultar no seleccionados, Ocultar selección), **tools 1** arriba a la izquierda (los subniveles de objeto y las herramientas "
        "del área) y **tools 2** abajo a la izquierda (visualización, pesos, animación, script, medir). Mover, Rotar y Escalar fijan la herramienta como W, E y "
        "R; la herramienta activa está marcada. Los submenús se abren hacia fuera: a la izquierda en los cuadrantes izquierdos y a la derecha en los derechos; en "
        "el borde de la ventana todo el menú cuádruple se desplaza hacia dentro. Como en Blender, el clic derecho abre el menú contextual del modo, con "
        "**{{T1BlTransform}}** (G, R, S) arriba.",
        "**Pravý klik do pohledu jako v 3ds Max: čtyřdílné menu.** Čtyři kvadranty se stýkají u myši: **transform** vpravo dole (Move, Rotate, Scale, Select, "
        "Clone, Curve Editor, Dope Sheet ...), **display** vpravo nahoře (Isolate Selection, Unfreeze All, Freeze Selection, Unhide All, Hide Unselected, Hide "
        "Selection), **tools 1** vlevo nahoře (podúrovně objektu a nástroje oblasti) a **tools 2** vlevo dole (zobrazení, váhy, animace, skript, měření). Move, "
        "Rotate a Scale nastaví nástroj jako W, E a R; aktivní nástroj je označen. Podmenu se otevírají směrem ven - v levých kvadrantech doleva, v pravých "
        "doprava; u okraje okna se celé menu posune dovnitř. Jako v Blenderu otevře pravý klik kontextové menu režimu, nahoře s **{{T1BlTransform}}** (G, R, S)."),
    # --- N, I ---
    "motion": (
        "**{{T1nSetup}} and {{T1nAnimation}} like CAT:** the switch at the top of **{{MfBewegung}}** (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>M</kbd>; like Blender "
        "Edit Mode / Pose Mode with <kbd>Tab</kbd>) splits the groups. Setup holds creating the rig, attachments, building for the game, GLA, `animation.cfg`, "
        "target GLA and slots; the figure stands in the base pose of the GLA and no keys are set - K, Auto Key, G and R say why. Animation holds the sequence, "
        "presets, editing, transfer, IK/FK, auto physics, pose, motion paths, curves and the pose library. Ghosts and the bone display are on the tab "
        "**{{MfAnzeige}}**. After loading, the mode is Animation when the figure has sequences or a rig, otherwise Setup. As in 3ds Max, bones are visible, "
        "selectable and rotatable from the start, without a control rig: a click selects the bone (also through the skin), E rotates it, Set Key and Auto Key "
        "key it.",
        "**{{T1nSetup}} und {{T1nAnimation}} wie CAT:** der Schalter oben im Feld **{{MfBewegung}}** (<kbd>Strg</kbd>+<kbd>Umschalt</kbd>+<kbd>M</kbd>; wie Blender "
        "Bearbeitungs-/Posenmodus mit <kbd>Tab</kbd>) teilt die Gruppen. Setup enthält Rig erstellen, Anbauten, Für das Spiel bauen, GLA, `animation.cfg`, "
        "Ziel-GLA und Plätze; die Figur steht in der Grundstellung der GLA, und es entstehen keine Schlüssel - K, Auto Key, G und R nennen den Grund. Animation "
        "enthält die Sequenz, Vorlagen, Bearbeiten, Übertragen, IK/FK, Auto-Physik, Pose, Bewegungspfade, Kurven und die Posen-Bibliothek. Geister und die "
        "Knochen-Darstellung stehen im Reiter **{{MfAnzeige}}**. Nach dem Laden gilt Animation, wenn die Figur Sequenzen oder ein Rig hat, sonst Setup. Wie in "
        "3ds Max sind Knochen von Anfang an sichtbar, wählbar und drehbar, ohne Control Rig: ein Klick wählt den Knochen (auch durch die Haut), E dreht ihn, "
        "Set Key und Auto Key setzen seine Schlüssel.",
        "**像 CAT 一样的 {{T1nSetup}} 和 {{T1nAnimation}}:** **{{MfBewegung}}** 面板顶部的开关 (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>M</kbd>; 像 Blender 时为编辑模式 / "
        "姿态模式, 按 <kbd>Tab</kbd>) 将组分开。设置模式包含创建绑定、附件、为游戏构建、GLA、`animation.cfg`、目标 GLA 和槽位; 角色处于 GLA 的基础姿势, 不会设置关键帧 - "
        "K、Auto Key、G 和 R 会说明原因。动画模式包含序列、预设、编辑、传输、IK/FK、自动物理、姿势、运动路径、曲线和姿势库。残影和骨骼显示位于 **{{MfAnzeige}}** "
        "选项卡。加载后, 角色有序列或绑定时为动画模式, 否则为设置模式。与 3ds Max 相同, 骨骼从一开始就可见、可选、可旋转, 无需控制绑定: 单击选择骨骼 (也可穿过蒙皮), "
        "E 旋转, Set Key 和 Auto Key 设置关键帧。",
        "**CAT と同じ {{T1nSetup}} と {{T1nAnimation}}:** **{{MfBewegung}}** パネル上部のスイッチ (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>M</kbd>; Blender 風では <kbd>Tab</kbd> "
        "で編集モード / ポーズモード) がグループを分けます。セットアップにはリグ作成、アタッチメント、ゲーム用にビルド、GLA、`animation.cfg`、ターゲット GLA とスロットがあり、"
        "フィギュアは GLA の基本姿勢で立ち、キーは設定されません - K、Auto Key、G、R は理由を表示します。アニメーションにはシーケンス、プリセット、編集、転送、IK/FK、"
        "オート物理、ポーズ、モーション パス、カーブ、ポーズ ライブラリがあります。ゴーストとボーン表示は **{{MfAnzeige}}** タブにあります。読み込み後は、フィギュアに"
        "シーケンスかリグがあればアニメーション、なければセットアップです。3ds Max と同じく、ボーンはコントロール リグなしで最初から表示・選択・回転できます: クリックで"
        "ボーンを選択 (スキン越しにも)、E で回転、Set Key と Auto Key でキーを設定します。",
        "**{{T1nSetup}} y {{T1nAnimation}} como CAT:** el interruptor de arriba del panel **{{MfBewegung}}** (<kbd>Ctrl</kbd>+<kbd>Mayús</kbd>+<kbd>M</kbd>; como "
        "Blender, modo Edición / modo Pose con <kbd>Tab</kbd>) separa los grupos. Configuración contiene crear el rig, accesorios, construir para el juego, GLA, "
        "`animation.cfg`, GLA de destino y ranuras; la figura está en la pose base de la GLA y no se ponen claves: K, Auto Key, G y R dicen por qué. Animación "
        "contiene la secuencia, presets, editar, transferir, IK/FK, física automática, pose, trayectorias, curvas y la biblioteca de poses. Los fantasmas y la "
        "visualización de huesos están en la pestaña **{{MfAnzeige}}**. Tras cargar, el modo es Animación si la figura tiene secuencias o un rig, si no "
        "Configuración. Como en 3ds Max, los huesos se ven, se seleccionan y se giran desde el principio, sin control rig: un clic selecciona el hueso (también a "
        "través de la piel), E lo gira, Set Key y Auto Key le ponen claves.",
        "**{{T1nSetup}} a {{T1nAnimation}} jako CAT:** přepínač nahoře v panelu **{{MfBewegung}}** (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>M</kbd>; jako Blender režim "
        "úprav / režim pózy klávesou <kbd>Tab</kbd>) rozdělí skupiny. Nastavení obsahuje vytvoření rigu, doplňky, sestavení pro hru, GLA, `animation.cfg`, cílovou "
        "GLA a sloty; postava stojí v základní póze GLA a nevznikají klíče - K, Auto Key, G a R řeknou proč. Animace obsahuje sekvenci, předvolby, úpravy, "
        "přenos, IK/FK, automatickou fyziku, pózu, dráhy pohybu, křivky a knihovnu póz. Duchové a zobrazení kostí jsou na kartě **{{MfAnzeige}}**. Po načtení "
        "platí Animace, má-li postava sekvence nebo rig, jinak Nastavení. Jako v 3ds Max jsou kosti od začátku viditelné, vybíratelné a otočné, bez control rigu: "
        "klik kost vybere (i skrz kůži), E ji otočí, Set Key a Auto Key nastaví klíče."),
    # --- H, K ---
    "knochen": (
        "**One clean chain per limb:** every bone is exactly one box (an octahedron like Blender) from its joint along its own axis to the next joint of its "
        "chain, as 3ds Max bones; twist helpers such as `lradiusX` and the root are small points (**{{T1Hilfsknochen}}** shows them as boxes), and a thin line "
        "links the start of a chain to its parent. **Foot platform like CAT:** with a control rig each leg has a flat plate under the foot on the floor; moving "
        "and turning it leads the IK leg, the knee follows its target, and the pivots heel, ball and toe roll the foot.",
        "**Eine saubere Kette je Glied:** jeder Knochen ist genau ein Kasten (wie Blender ein Oktaeder) von seinem Gelenk entlang seiner Achse bis zum nächsten "
        "Gelenk seiner Kette, wie Bones in 3ds Max; Drehhilfen wie `lradiusX` und die Wurzel sind kleine Punkte (**{{T1Hilfsknochen}}** zeigt sie als Kasten), "
        "eine dünne Linie verbindet den Anfang einer Kette mit dem Elternknochen. **Fußplattform wie CAT:** mit Control Rig hat jedes Bein eine flache Platte "
        "unter dem Fuß am Boden; Verschieben und Drehen führt das IK-Bein, das Knie folgt seinem Ziel, und die Drehpunkte Ferse, Ballen und Zehe rollen den Fuß ab.",
        "**每个肢体一条整洁的骨骼链:** 每根骨骼正好是一个方框 (像 Blender 时为八面体), 从其关节沿自身轴线延伸到骨骼链的下一个关节, 与 3ds Max 的骨骼相同; "
        "`lradiusX` 等扭转辅助骨骼和根骨骼显示为小点 (**{{T1Hilfsknochen}}** 将其显示为方框), 一条细线将骨骼链的起点连到父骨骼。**像 CAT 一样的脚部平台:** 有控制绑定时, "
        "每条腿在脚下的地面上有一块平板; 移动和旋转它可带动 IK 腿, 膝盖跟随其目标, 脚跟、脚掌和脚尖支点可使脚滚动。",
        "**肢ごとに整ったチェーン 1 本:** 各ボーンは 3ds Max のボーンと同じく、関節から自身の軸に沿ってチェーンの次の関節までの 1 つのボックス (Blender 風では八面体) "
        "です。`lradiusX` などのねじれ補助ボーンとルートは小さな点で表示され (**{{T1Hilfsknochen}}** でボックス表示)、細い線がチェーンの始点と親を結びます。**CAT と同じ"
        "フット プラットフォーム:** コントロール リグがあると各脚の足の下の床に平らなプレートがあり、移動・回転で IK の脚を動かし、膝はターゲットに従い、かかと・母指球・"
        "つま先のピボットで足を転がします。",
        "**Una cadena limpia por miembro:** cada hueso es exactamente una caja (un octaedro como en Blender) desde su articulación a lo largo de su propio eje hasta "
        "la siguiente articulación de su cadena, como los huesos de 3ds Max; los ayudantes de torsión como `lradiusX` y la raíz son puntos pequeños "
        "(**{{T1Hilfsknochen}}** los muestra como cajas) y una línea fina une el inicio de una cadena con su padre. **Plataforma de pie como CAT:** con un control "
        "rig cada pierna tiene una placa plana bajo el pie en el suelo; moverla y girarla guía la pierna IK, la rodilla sigue a su objetivo y los pivotes talón, "
        "bola y punta hacen rodar el pie.",
        "**Jeden čistý řetězec na končetinu:** každá kost je právě jeden kvádr (jako v Blenderu osmistěn) od svého kloubu podél vlastní osy k dalšímu kloubu "
        "svého řetězce, jako kosti v 3ds Max; pomocné kosti natočení jako `lradiusX` a kořen jsou malé body (**{{T1Hilfsknochen}}** je ukáže jako kvádry) a tenká "
        "čára spojuje začátek řetězce s rodičem. **Plošina chodidla jako CAT:** s control rigem má každá noha plochou desku pod chodidlem na zemi; posun a otočení "
        "vede IK nohu, koleno sleduje svůj cíl a otočné body pata, bříško a špička chodidlo odvalují."),
    # --- O ---
    "painting": (
        "**The target follows the brush:** a stroke in the 3D view paints into the texture of every surface under the brush, across images and UV seams; the "
        "layers stay per image and one undo takes back the whole stroke. **{{PtGrZiel}}** follows the last painted surface; its thumbnails, Alt+click on the "
        "figure, the picker **{{T1oTexturAufnehmen}}** or a click on a surface in the scene explorer choose a texture. **{{T1oNurZiel}}** keeps the old way "
        "(only the chosen texture).",
        "**Das Ziel folgt dem Pinsel:** ein Strich in der 3D-Ansicht malt in die Textur jeder Fläche unter dem Pinsel, über Bilder und UV-Nähte hinweg; die Ebenen "
        "bleiben je Bild getrennt, und ein Rückgängig nimmt den ganzen Strich zurück. **{{PtGrZiel}}** folgt der zuletzt bemalten Fläche; seine Vorschaubilder, "
        "Alt+Klick auf die Figur, die Pipette **{{T1oTexturAufnehmen}}** oder ein Klick auf eine Fläche im Szenen-Explorer wählen eine Textur. "
        "**{{T1oNurZiel}}** hält den alten Weg (nur die gewählte Textur).",
        "**目标跟随画笔:** 在 3D 视图中的一笔会绘制到画笔下每个表面的纹理中, 跨越图像和 UV 接缝; 图层按图像分开, 一次撤销撤回整笔。**{{PtGrZiel}}** 跟随最后绘制的表面; "
        "其缩略图、在角色上 Alt+单击、吸管 **{{T1oTexturAufnehmen}}** 或在场景资源管理器中单击表面均可选择纹理。**{{T1oNurZiel}}** 保留旧方式 (仅所选纹理)。",
        "**ターゲットがブラシに従う:** 3D ビューのストロークは、画像や UV の継ぎ目をまたいで、ブラシの下にあるすべてのサーフェスのテクスチャにペイントします。レイヤーは"
        "画像ごとに分かれ、1 回の元に戻すでストローク全体が戻ります。**{{PtGrZiel}}** は最後にペイントしたサーフェスに従い、そのサムネイル、フィギュアの Alt+クリック、"
        "スポイト **{{T1oTexturAufnehmen}}**、シーン エクスプローラのサーフェスのクリックでテクスチャを選べます。**{{T1oNurZiel}}** で従来の方法 (選んだテクスチャのみ) になります。",
        "**El destino sigue al pincel:** un trazo en la vista 3D pinta en la textura de cada superficie bajo el pincel, a través de imágenes y costuras UV; las "
        "capas siguen siendo por imagen y un deshacer quita el trazo entero. **{{PtGrZiel}}** sigue a la última superficie pintada; sus miniaturas, Alt+clic sobre "
        "la figura, el cuentagotas **{{T1oTexturAufnehmen}}** o un clic en una superficie del explorador de escena eligen una textura. **{{T1oNurZiel}}** mantiene "
        "el modo anterior (solo la textura elegida).",
        "**Cíl sleduje štětec:** tah ve 3D pohledu maluje do textury každé plochy pod štětcem, přes obrázky i UV švy; vrstvy zůstávají pro každý obrázek zvlášť a "
        "jedno zpět vrátí celý tah. **{{PtGrZiel}}** sleduje naposledy malovanou plochu; texturu vybírají jeho náhledy, Alt+klik na postavu, kapátko "
        "**{{T1oTexturAufnehmen}}** nebo klik na plochu v průzkumníku scény. **{{T1oNurZiel}}** zachová starý způsob (jen zvolená textura)."),
    # --- P ---
    "weights": (
        "**Vertices like the Skin modifier of 3ds Max:** the group **{{MgRWerkzeug}}** has *Select: Vertices, Select Element, Backface Cull Vertices*; click or "
        "drag vertices, Ctrl adds, Alt removes. Its list shows each bone with the weight of the selected vertices (the average for several); click a bone and "
        "type a value or use .1/.25/.5/.75/.9/1 - the sum stays 1. Copy, Paste and Paste-Pos copy weights between vertices, the Weight Table (dockable) lists "
        "vertices × bones, and Mirror Paste copies the selected vertices to the other side with left and right bones swapped; the pairs stand under "
        "**{{T1pPaare}}**. Like Blender the panel **{{T1pBlVertexGewichte}}** shows the same list. Paint weights remains one tool among them; saving keeps the "
        "format of the game (4 weights, 10 bits).",
        "**Ecken wie der Skin-Modifier von 3ds Max:** die Gruppe **{{MgRWerkzeug}}** hat *Auswahl: Ecken, Element wählen, Rückseiten-Ecken ausblenden*; Ecken "
        "anklicken oder aufziehen, Strg fügt hinzu, Alt nimmt weg. Ihre Liste zeigt jeden Knochen mit dem Gewicht der gewählten Ecken (bei mehreren den "
        "Mittelwert); einen Knochen anklicken und einen Wert tippen oder .1/.25/.5/.75/.9/1 nehmen - die Summe bleibt 1. Kopieren, Einfügen und Einfügen nach "
        "Ort übertragen Gewichte zwischen Ecken, die Gewichtstabelle (andockbar) listet Ecken × Knochen, und Spiegeln einfügen überträgt die gewählten Ecken auf "
        "die andere Seite mit getauschten Knochen links/rechts; die Paare stehen unter **{{T1pPaare}}**. Wie Blender zeigt das Panel **{{T1pBlVertexGewichte}}** "
        "dieselbe Liste. Gewichte malen bleibt ein Werkzeug unter mehreren; gespeichert wird im Format des Spiels (4 Gewichte, 10 Bit).",
        "**像 3ds Max 蒙皮修改器一样的顶点:** **{{MgRWerkzeug}}** 组有 *选择: 顶点、选择元素、背面消隐顶点*; 单击或框选顶点, Ctrl 添加, Alt 移除。其列表显示每根骨骼及"
        "所选顶点的权重 (多个顶点时为平均值); 单击骨骼后输入数值或使用 .1/.25/.5/.75/.9/1 - 总和保持为 1。复制、粘贴和按位置粘贴可在顶点之间复制权重, 权重表 (可停靠) "
        "列出顶点 × 骨骼, 镜像粘贴将所选顶点复制到另一侧并交换左右骨骼; 骨骼对位于 **{{T1pPaare}}** 下。像 Blender 时, **{{T1pBlVertexGewichte}}** 面板显示相同的列表。"
        "绘制权重仍是工具之一; 保存时保持游戏格式 (4 个权重, 10 位)。",
        "**3ds Max のスキン モディファイヤと同じ頂点:** **{{MgRWerkzeug}}** グループに *選択: 頂点、要素を選択、背面頂点をカリング* があり、頂点をクリックまたはドラッグで"
        "選択、Ctrl で追加、Alt で除外します。リストには各ボーンと選択頂点のウェイト (複数なら平均) が並び、ボーンをクリックして値を入力するか .1/.25/.5/.75/.9/1 を使います - "
        "合計は 1 のままです。コピー・貼り付け・位置で貼り付けで頂点間のウェイトを写し、ウェイト テーブル (ドッキング可能) は頂点 × ボーンを一覧し、ミラー貼り付けは選択頂点を"
        "左右のボーンを入れ替えて反対側へ写します。ペアは **{{T1pPaare}}** にあります。Blender 風では **{{T1pBlVertexGewichte}}** パネルに同じリストがあります。"
        "ウェイト ペイントはツールの 1 つで、保存はゲームの形式 (4 ウェイト、10 ビット) のままです。",
        "**Vértices como el modificador Piel de 3ds Max:** el grupo **{{MgRWerkzeug}}** tiene *Seleccionar: Vértices, Seleccionar elemento, Descartar vértices "
        "traseros*; haga clic o arrastre sobre vértices, Ctrl añade, Alt quita. Su lista muestra cada hueso con el peso de los vértices seleccionados (la media si "
        "son varios); haga clic en un hueso y escriba un valor o use .1/.25/.5/.75/.9/1: la suma sigue siendo 1. Copiar, Pegar y Pegar por posición copian pesos "
        "entre vértices, la tabla de pesos (acoplable) lista vértices × huesos y Pegar simetría copia los vértices seleccionados al otro lado con los huesos "
        "izquierdo y derecho intercambiados; los pares están en **{{T1pPaare}}**. Como en Blender, el panel **{{T1pBlVertexGewichte}}** muestra la misma lista. "
        "Pintar pesos sigue siendo una herramienta más; al guardar se mantiene el formato del juego (4 pesos, 10 bits).",
        "**Vrcholy jako modifikátor Skin v 3ds Max:** skupina **{{MgRWerkzeug}}** má *Výběr: vrcholy, vybrat prvek, vynechat zadní vrcholy*; vrcholy se vybírají "
        "klikem nebo tažením, Ctrl přidá, Alt odebere. Její seznam ukazuje každou kost s váhou vybraných vrcholů (u více vrcholů průměr); klikněte na kost a napište "
        "hodnotu nebo použijte .1/.25/.5/.75/.9/1 - součet zůstane 1. Kopírovat, Vložit a Vložit podle místa přenášejí váhy mezi vrcholy, tabulka vah (ukotvitelná) "
        "uvádí vrcholy × kosti a Zrcadlově vložit přenese vybrané vrcholy na druhou stranu s prohozenými kostmi vlevo/vpravo; páry stojí pod **{{T1pPaare}}**. Jako "
        "v Blenderu ukazuje panel **{{T1pBlVertexGewichte}}** stejný seznam. Malování vah zůstává jedním z nástrojů; ukládá se ve formátu hry (4 váhy, 10 bitů)."),
    # --- C ---
    "gla": (
        "**{{ModelLoadGla}}** in the toolbar and in the menu opens the same list as **Other GLA...** in the command panel: every .gla in the archives of the game, "
        "with a search; **Open file...** in the list loads a loose .gla.",
        "**{{ModelLoadGla}}** in der Werkzeugleiste und im Menü öffnet dieselbe Liste wie **Other GLA...** im Befehlsfeld: jede .gla in den Archiven des Spiels, "
        "mit Suche; **Datei öffnen...** in der Liste lädt eine lose .gla.",
        "工具栏和菜单中的 **{{ModelLoadGla}}** 打开与命令面板中 **Other GLA...** 相同的列表: 游戏存档中的每个 .gla, 带搜索; 列表中的 **打开文件...** 加载单独的 .gla。",
        "ツールバーとメニューの **{{ModelLoadGla}}** はコマンド パネルの **Other GLA...** と同じ一覧を開きます: ゲームのアーカイブ内のすべての .gla と検索欄。一覧の"
        "**ファイルを開く...** で単独の .gla を読み込みます。",
        "**{{ModelLoadGla}}** en la barra de herramientas y en el menú abre la misma lista que **Other GLA...** en el panel de comandos: cada .gla de los archivos "
        "del juego, con búsqueda; **Abrir archivo...** en la lista carga un .gla suelto.",
        "**{{ModelLoadGla}}** na liště nástrojů a v menu otevře stejný seznam jako **Other GLA...** v příkazovém panelu: každou .gla v archivech hry, s hledáním; "
        "**Otevřít soubor...** v seznamu načte volnou .gla."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def bloecke_leer():
    return [("p", _t("leer"))]


def bloecke_sequenzen():
    return [("p", _t("sequenzen"))]


def bloecke_ansicht():
    return [("p", _t("ansicht"))]


def bloecke_quad():
    return [("p", _t("quad"))]


def bloecke_motion():
    return [("p", _t("motion"))]


def bloecke_knochen():
    return [("p", _t("knochen"))]


def bloecke_painting():
    return [("p", _t("painting"))]


def bloecke_weights():
    return [("p", _t("weights"))]


def bloecke_gla():
    return [("p", _t("gla"))]
