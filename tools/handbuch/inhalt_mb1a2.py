"""Modell-Block 1a-2: die Seiten "Model-Tab: Weights" und "Model-Tab: Bedienung wie 3ds Max oder Blender" (mit dem
Fenster "Ins Skript" und dem Model-Tab ohne Skriptteile).

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_mb1a1.py.
"""

X = {
    # --- Bedienung ---
    "b_titel": ("Model tab: controls like 3ds Max or Blender", "Model-Tab: Bedienung wie 3ds Max oder Blender", "模型选项卡:像 3ds Max 或 Blender 一样操作",
                "モデルタブ: 3ds Max または Blender と同じ操作", "Pestaña Model: controles como 3ds Max o Blender", "Karta Model: ovládání jako 3ds Max nebo Blender"),
    "b_kurz": (
        "Choose the controls you know - the Model tab then uses the keys, views and bone display of 3ds Max or of Blender. It has no script parts; "
        "animations go into a script through the window Into script.",
        "Wähle die Bedienung, die du kennst - der Model-Tab nimmt dann Tasten, Ansichten und Knochenanzeige von 3ds Max oder von Blender. Er hat keine "
        "Skriptteile; Animationen kommen über das Fenster Ins Skript in ein Skript.",
        "选择你熟悉的操作方式——模型选项卡随后使用 3ds Max 或 Blender 的按键、视图和骨骼显示。它没有脚本部分;动画通过 \"插入脚本\" 窗口进入脚本。",
        "慣れた操作を選ぶと、モデルタブは 3ds Max または Blender のキー、ビュー、ボーン表示を使います。スクリプト部分はなく、アニメーションは "
        "\"スクリプトへ\" ウィンドウからスクリプトに入れます。",
        "Elige los controles que conoces: la pestaña Model usa entonces las teclas, vistas y huesos de 3ds Max o de Blender. No tiene partes de "
        "script; las animaciones van a un script con la ventana Al script.",
        "Zvolte ovládání, které znáte - karta Model pak používá klávesy, pohledy a zobrazení kostí z 3ds Max nebo z Blenderu. Nemá žádné části "
        "skriptu; animace se do skriptu dostanou oknem Do skriptu."),
    "b_bild": (
        "Controls like Blender: one large view, bones as octahedra, vertex select, the timeline counted from 1.",
        "Bedienung wie Blender: eine große Ansicht, Knochen als Oktaeder, Ecken wählen, die Zeitleiste ab 1 gezählt.",
        "像 Blender 一样操作:一个大视图,骨骼为八面体,顶点选择,时间轴从 1 开始计数。",
        "Blender と同じ操作: 大きなビュー 1 つ、八面体のボーン、頂点選択、1 から数えるタイムライン。",
        "Controles como Blender: una vista grande, huesos como octaedros, selección de vértices, la línea de tiempo contada desde 1.",
        "Ovládání jako Blender: jeden velký pohled, kosti jako osmistěny, výběr vrcholů, časová osa počítaná od 1."),
    "b_text": (
        "Switch between **{{MbWieMax}}** and **{{MbWieBlender}}** with a click on the control mode at the left of the status line of the tab, in the menu of the header of a view (**{{MbBedienung}}**), in the settings or with the search for all actions; the setting is remembered and acts at once - on the keys, the mouse and the layout ([[modell-blender]]). Like 3ds Max: four views (Alt+W), bone objects, Q/W/E/R with the gizmo, sub-object levels 1 vertex, 2 edge, 3 border, 4 polygon, 5 element, 6 object, the timeline from 0. Like Blender: one view (Ctrl+Alt+Q for four), octahedral bones, G/R/S with X/Y/Z (twice: local, Shift: plane) and a typed number, Enter or left click confirms, Esc or right click cancels; Tab toggles object and edit mode, 1/2/3 vertex, edge, face, the numpad views 1/3/7 (Ctrl for the opposite side), 5 perspective, 2/4/6/8 orbit by 15°, Home frames all, Space plays, the timeline counts from 1 and shows the GLA frame in its tooltip. X (3ds Max) or F3 (Blender) searches all actions; the right mouse button opens the menu of the view.",
        "Zwischen **{{MbWieMax}}** und **{{MbWieBlender}}** wechselst du mit einem Klick auf den Bedienmodus links in der Statuszeile des Tabs, im Menü der Kopfzeile einer Ansicht (**{{MbBedienung}}**), in den Einstellungen oder über die Suche nach allen Aktionen; die Einstellung bleibt und wirkt sofort - auf Tasten, Maus und Aufbau ([[modell-blender]]). Wie 3ds Max: vier Ansichten (Alt+W), Bone-Objekte, Q/W/E/R mit Gizmo, Unterobjekte 1 Scheitelpunkt, 2 Kante, 3 Rand, 4 Polygon, 5 Element, 6 Objekt, die Zeitleiste ab 0. Wie Blender: eine Ansicht (Strg+Alt+Q für vier), Knochen als Oktaeder, G/R/S mit X/Y/Z (zweimal: lokal, Umschalt: Ebene) und getippter Zahl, Enter oder Linksklick bestätigt, Esc oder Rechtsklick bricht ab; Tab wechselt Objekt- und Bearbeitungsmodus, 1/2/3 Ecke, Kante, Fläche, Ziffernblock 1/3/7 (Strg für die Gegenseite), 5 Perspektive, 2/4/6/8 kreisen um 15°, Pos1 zeigt alles, Leertaste spielt ab, die Zeitleiste zählt ab 1 und zeigt den GLA-Frame im Tooltip. X (3ds Max) bzw. F3 (Blender) sucht alle Aktionen; die rechte Maustaste öffnet das Menü der Ansicht.",
        "单击选项卡状态栏左侧的操作模式、在视图标题栏的菜单 (**{{MbBedienung}}**) 中、在设置中或通过搜索所有操作, 可在 **{{MbWieMax}}** 和 **{{MbWieBlender}}** 之间切换;设置会被记住并立即生效 - 包括按键、鼠标和布局 ([[modell-blender]])。像 3ds Max:四个视图 (Alt+W)、骨骼对象、带 Gizmo 的 Q/W/E/R、子对象级别 1 顶点、2 边、3 边界、4 多边形、5 元素、6 对象,时间轴从 0 开始。像 Blender:一个视图 (Ctrl+Alt+Q 切换为四个)、八面体骨骼、G/R/S 配合 X/Y/Z (按两次: 局部, Shift: 平面) 和输入的数值,Enter 或左键确认,Esc 或右键取消;Tab 切换物体与编辑模式,1/2/3 顶点、边、面,小键盘 1/3/7 视图 (Ctrl 为对侧),5 透视,2/4/6/8 环绕 15°,Home 显示全部,空格播放,时间轴从 1 开始计数并在提示中显示 GLA 帧。X (3ds Max) 或 F3 (Blender) 搜索所有操作;鼠标右键打开视图菜单。",
        "**{{MbWieMax}}** と **{{MbWieBlender}}** は、タブのステータス行の左にある操作モードのクリック、ビューのヘッダーのメニュー (**{{MbBedienung}}**)、設定、またはすべての操作の検索で切り替えます。設定は保存され、キー、マウス、レイアウト ([[modell-blender]]) にすぐ反映されます。3ds Max と同じ: 4 つのビュー (Alt+W)、ボーンオブジェクト、ギズモ付きの Q/W/E/R、サブオブジェクト 1 頂点・2 エッジ・3 境界・4 ポリゴン・5 エレメント・6 オブジェクト、0 からのタイムライン。Blender と同じ: ビュー 1 つ (Ctrl+Alt+Q で 4 つ)、八面体のボーン、X/Y/Z (2 回でローカル、Shift で平面) と入力した数値付きの G/R/S、Enter か左クリックで確定、Esc か右クリックで取り消し。Tab でオブジェクト/編集モード、1/2/3 で頂点・辺・面、テンキー 1/3/7 (Ctrl で反対側)、5 で透視、2/4/6/8 で 15° 周回、Home で全体表示、Space で再生、タイムラインは 1 から数え、ツールチップに GLA フレームを表示します。X (3ds Max) または F3 (Blender) ですべての操作を検索、右ボタンでビューのメニューを開きます。",
        "Cambia entre **{{MbWieMax}}** y **{{MbWieBlender}}** con un clic en el modo de control a la izquierda de la línea de estado de la pestaña, en el menú de la cabecera de una vista (**{{MbBedienung}}**), en los ajustes o con la búsqueda de todas las acciones; el ajuste se recuerda y actúa al momento: en las teclas, el ratón y la disposición ([[modell-blender]]). Como 3ds Max: cuatro vistas (Alt+W), objetos hueso, Q/W/E/R con gizmo, subobjetos 1 vértice, 2 arista, 3 borde, 4 polígono, 5 elemento, 6 objeto, la línea de tiempo desde 0. Como Blender: una vista (Ctrl+Alt+Q para cuatro), huesos octaédricos, G/R/S con X/Y/Z (dos veces: local, Mayús: plano) y un número tecleado, Intro o clic izquierdo confirma, Esc o clic derecho cancela; Tab alterna modo objeto y edición, 1/2/3 vértice, arista, cara, las vistas del teclado numérico 1/3/7 (Ctrl para el lado opuesto), 5 perspectiva, 2/4/6/8 orbitar 15°, Inicio encuadra todo, Espacio reproduce, la línea de tiempo cuenta desde 1 y muestra el fotograma GLA en su ayuda. X (3ds Max) o F3 (Blender) busca todas las acciones; el botón derecho abre el menú de la vista.",
        "Mezi **{{MbWieMax}}** a **{{MbWieBlender}}** přepnete klikem na režim ovládání vlevo ve stavovém řádku karty, v nabídce záhlaví pohledu (**{{MbBedienung}}**), v nastavení nebo hledáním všech akcí; nastavení se pamatuje a platí hned - pro klávesy, myš i rozvržení ([[modell-blender]]). Jako 3ds Max: čtyři pohledy (Alt+W), objekty kostí, Q/W/E/R s gizmem, podobjekty 1 vrchol, 2 hrana, 3 okraj, 4 polygon, 5 prvek, 6 objekt, časová osa od 0. Jako Blender: jeden pohled (Ctrl+Alt+Q pro čtyři), kosti jako osmistěny, G/R/S s X/Y/Z (dvakrát: lokálně, Shift: rovina) a napsaným číslem, Enter nebo levé tlačítko potvrdí, Esc nebo pravé tlačítko zruší; Tab přepíná objektový a editační režim, 1/2/3 vrchol, hrana, plocha, pohledy numerické klávesnice 1/3/7 (Ctrl pro opačnou stranu), 5 perspektiva, 2/4/6/8 kroužení o 15°, Home ukáže vše, mezerník přehraje, časová osa počítá od 1 a v popisku ukazuje snímek GLA. X (3ds Max) nebo F3 (Blender) hledá všechny akce; pravé tlačítko otevře nabídku pohledu.",
    ),
    "h_max": (
        "Controls like 3ds Max",
        "Bedienung wie 3ds Max",
        "像 3ds Max 一样操作",
        "3ds Max と同じ操作",
        "Controles como 3ds Max",
        "Ovládání jako 3ds Max",
    ),
    "max": (
        "**{{MbWieMax}}** is the default. The keys work after a click into the view, the explorer, the command panel or the timeline; <kbd>X</kbd> lists them all. The gizmo moves, rotates or scales the selected surfaces in the model document (all levels of detail) - <kbd>Ctrl</kbd>+<kbd>Z</kbd> takes it back.",
        "**{{MbWieMax}}** ist die Vorgabe. Die Tasten wirken nach einem Klick in die Ansicht, den Explorer, das Befehlsfeld oder die Zeitleiste; <kbd>X</kbd> listet sie alle. Das Gizmo verschiebt, dreht oder skaliert die gewählten Flächen im Modelldokument (alle Detailstufen) - <kbd>Strg</kbd>+<kbd>Z</kbd> nimmt es zurück.",
        "默认为 **{{MbWieMax}}**。在视图、资源管理器、命令面板或时间轴中单击后按键生效;<kbd>X</kbd> 列出全部按键。Gizmo 在模型文档中移动、旋转或缩放所选的面(所有细节级别)——<kbd>Ctrl</kbd>+<kbd>Z</kbd> 可撤销。",
        "既定は **{{MbWieMax}}** です。キーはビュー、エクスプローラー、コマンドパネル、タイムラインのいずれかをクリックした後に効きます。<kbd>X</kbd> で一覧を表示します。ギズモはモデルドキュメント内の選択サーフェスを移動・回転・スケールします (すべての詳細レベル)。<kbd>Ctrl</kbd>+<kbd>Z</kbd> で元に戻せます。",
        "**{{MbWieMax}}** es el predeterminado. Las teclas funcionan tras un clic en la vista, el explorador, el panel de comandos o la línea de tiempo; <kbd>X</kbd> las muestra todas. El gizmo mueve, rota o escala las superficies seleccionadas en el documento del modelo (todos los niveles de detalle); <kbd>Ctrl</kbd>+<kbd>Z</kbd> lo deshace.",
        "**{{MbWieMax}}** je výchozí. Klávesy fungují po kliknutí do pohledu, průzkumníku, panelu příkazů nebo časové osy; <kbd>X</kbd> je vypíše všechny. Gizmo posouvá, otáčí nebo mění měřítko vybraných povrchů v dokumentu modelu (všechny úrovně detailu) - <kbd>Ctrl</kbd>+<kbd>Z</kbd> to vrátí.",
    ),
    "k_maus": (
        "Middle mouse / Alt + middle / wheel",
        "Mittlere Maus / Alt + Mitte / Rad",
        "鼠标中键 / Alt + 中键 / 滚轮",
        "中ボタン / Alt + 中ボタン / ホイール",
        "Botón central / Alt + central / rueda",
        "Prostřední tlačítko / Alt + prostřední / kolečko",
    ),
    "k_maus_t": (
        "Pan / orbit / zoom (Ctrl + Alt + middle: zoom by dragging)",
        "Schieben / kreisen / zoomen (Strg + Alt + Mitte: zoomen durch Ziehen)",
        "平移 / 环绕 / 缩放 (Ctrl + Alt + 中键:拖动缩放)",
        "パン / 周回 / ズーム (Ctrl + Alt + 中ボタン: ドラッグでズーム)",
        "Desplazar / orbitar / zoom (Ctrl + Alt + central: zoom arrastrando)",
        "Posun / kroužení / zoom (Ctrl + Alt + prostřední: zoom tažením)",
    ),
    "k_links": (
        "Left click / drag",
        "Linksklick / ziehen",
        "左键单击 / 拖动",
        "左クリック / ドラッグ",
        "Clic izquierdo / arrastrar",
        "Levý klik / tažení",
    ),
    "k_links_t": (
        "Select a surface / select with a frame (Ctrl adds, Alt removes)",
        "Fläche wählen / mit Rahmen wählen (Strg dazu, Alt weg)",
        "选择面 / 框选 (Ctrl 添加,Alt 移除)",
        "サーフェスを選択 / 枠で選択 (Ctrl で追加、Alt で除外)",
        "Seleccionar una superficie / seleccionar con un marco (Ctrl añade, Alt quita)",
        "Vybrat povrch / vybrat rámečkem (Ctrl přidá, Alt odebere)",
    ),
    "k_qwer": (
        "Q / W / E / R",
        "Q / W / E / R",
        "Q / W / E / R",
        "Q / W / E / R",
        "Q / W / E / R",
        "Q / W / E / R",
    ),
    "k_qwer_t": (
        "Select, move, rotate, scale (F5 / F6 / F7 restrict to X / Y / Z, F8 planes, + / - gizmo size)",
        "Auswählen, verschieben, drehen, skalieren (F5 / F6 / F7 auf X / Y / Z beschränken, F8 Ebenen, + / - Gizmo-Größe)",
        "选择、移动、旋转、缩放 (F5 / F6 / F7 限制到 X / Y / Z,F8 平面,+ / - Gizmo 大小)",
        "選択、移動、回転、スケール (F5 / F6 / F7 で X / Y / Z に拘束、F8 平面、+ / - ギズモの大きさ)",
        "Seleccionar, mover, rotar, escalar (F5 / F6 / F7 restringen a X / Y / Z, F8 planos, + / - tamaño del gizmo)",
        "Výběr, posun, otočení, měřítko (F5 / F6 / F7 omezí na X / Y / Z, F8 roviny, + / - velikost gizma)",
    ),
    "k_ansicht": (
        "Alt+W / P T F L B / Z",
        "Alt+W / P T F L B / Z",
        "Alt+W / P T F L B / Z",
        "Alt+W / P T F L B / Z",
        "Alt+W / P T F L B / Z",
        "Alt+W / P T F L B / Z",
    ),
    "k_ansicht_t": (
        "Four views or one / perspective, top, front, left, bottom / zoom extents (selection)",
        "Vier Ansichten oder eine / Perspektive, oben, vorn, links, unten / Zoom Grenzen (Auswahl)",
        "四个视图或一个 / 透视、顶、前、左、底 / 最大化显示 (所选)",
        "4 ビューか 1 ビュー / パース、上、前、左、下 / 全体表示 (選択)",
        "Cuatro vistas o una / perspectiva, superior, frontal, izquierda, inferior / encuadrar (selección)",
        "Čtyři pohledy nebo jeden / perspektiva, shora, zepředu, zleva, zespodu / přiblížit vše (výběr)",
    ),
    "k_anzeige": (
        "F3 / F4 / G / J / 7",
        "F3 / F4 / G / J / 7",
        "F3 / F4 / G / J / 7",
        "F3 / F4 / G / J / 7",
        "F3 / F4 / G / J / 7",
        "F3 / F4 / G / J / 7",
    ),
    "k_anzeige_t": (
        "Wireframe, edged faces, grid, selection bracket, statistics",
        "Drahtgitter, Flächen mit Kanten, Raster, Auswahlklammer, Statistik",
        "线框、带边面、网格、选择框、统计",
        "ワイヤーフレーム、エッジ面、グリッド、選択ブラケット、統計",
        "Alambre, caras con aristas, rejilla, corchetes de selección, estadísticas",
        "Drátový model, plochy s hranami, mřížka, závorky výběru, statistika",
    ),
    "k_zeit": (
        "/ , . Home End",
        "/ , . Pos1 Ende",
        "/ , . Home End",
        "/ , . Home End",
        "/ , . Inicio Fin",
        "/ , . Home End",
    ),
    "k_zeit_t": (
        "Play/stop, one frame back/forward, first/last frame",
        "Abspielen/anhalten, ein Bild zurück/vor, erstes/letztes Bild",
        "播放/停止、后退/前进一帧、首帧/末帧",
        "再生/停止、1 フレーム戻る/進む、最初/最後のフレーム",
        "Reproducir/detener, un fotograma atrás/adelante, primer/último fotograma",
        "Přehrát/zastavit, o snímek zpět/vpřed, první/poslední snímek",
    ),
    "k_auswahl": (
        "Ctrl+A / Ctrl+D / Ctrl+I / H",
        "Strg+A / Strg+D / Strg+I / H",
        "Ctrl+A / Ctrl+D / Ctrl+I / H",
        "Ctrl+A / Ctrl+D / Ctrl+I / H",
        "Ctrl+A / Ctrl+D / Ctrl+I / H",
        "Ctrl+A / Ctrl+D / Ctrl+I / H",
    ),
    "k_auswahl_t": (
        "Select all, none, invert; select by name",
        "Alles, nichts, umkehren; nach Namen auswählen",
        "全选、取消、反选;按名称选择",
        "すべて選択、解除、反転。名前で選択",
        "Seleccionar todo, nada, invertir; seleccionar por nombre",
        "Vybrat vše, nic, obrátit; vybrat podle názvu",
    ),
    "bild_vier": (
        "Four views (Alt+W) with the bones as bone objects.",
        "Vier Ansichten (Alt+W) mit den Knochen als Bone-Objekte.",
        "四个视图 (Alt+W),骨骼显示为骨骼对象。",
        "4 ビュー (Alt+W)。ボーンはボーンオブジェクトとして表示。",
        "Cuatro vistas (Alt+W) con los huesos como objetos hueso.",
        "Čtyři pohledy (Alt+W) s kostmi jako objekty kostí.",
    ),
    "b_h_skript": ("Animations into a script", "Animationen ins Skript", "把动画放入脚本", "アニメーションをスクリプトへ", "Animaciones al script", "Animace do skriptu"),
    "b_skript": (
        "The Model tab is only for models: no script tree, no button column, no compile status; File > Open opens a model and Ctrl+S does not touch the script. To put an animation into a script, open **{{MbInsSkriptFenster}}** (the button with the script symbol in the head of the scene explorer, right-click on a sequence, the menu File or Ctrl+Shift+I). Choose the script (open ones or a new one), the character for an affect block (empty: none) and the command - SET_ANIM_BOTH/UPPER/LOWER or a hold time with the length of the animation; the preview shows the line, **{{MbInsSkriptEinfuegen}}** puts it in with one undo step. The small button in the animation field takes the animation selected in the scene explorer; Enter inserts, Esc or **{{UfAbbrechen}}** closes the window. While the window is closed it takes no space.",
        "Der Model-Tab ist nur für Modelle: kein Skriptbaum, keine Knopfspalte, kein Kompilierstand; Datei > Öffnen öffnet ein Modell, und Strg+S berührt das Skript nicht. Um eine Animation in ein Skript zu setzen, öffne **{{MbInsSkriptFenster}}** (der Knopf mit dem Skript-Symbol im Kopf des Szenen-Explorers, Rechtsklick auf eine Sequenz, Menü Datei oder Strg+Umschalt+I). Wähle das Skript (offene oder ein neues), die Figur für einen affect-Block (leer: keiner) und den Befehl - SET_ANIM_BOTH/UPPER/LOWER oder eine Haltezeit mit der Länge der Animation; die Vorschau zeigt die Zeile, **{{MbInsSkriptEinfuegen}}** setzt sie mit einem Rückgängig-Schritt ein. Der kleine Knopf im Feld der Animation nimmt die im Szenen-Explorer gewählte Animation; Enter fügt ein, Esc oder **{{UfAbbrechen}}** schließt das Fenster. Solange das Fenster zu ist, belegt es keinen Platz.",
        "模型选项卡只用于模型:没有脚本树、没有按钮列、没有编译状态;文件 > 打开会打开模型,Ctrl+S 不会动脚本。要把动画放进脚本,请打开 **{{MbInsSkriptFenster}}** (场景资源管理器顶部带脚本图标的按钮、右键单击序列、文件菜单或 Ctrl+Shift+I)。选择脚本 (已打开的或新建)、affect 块的角色 (留空: 无) 和命令——SET_ANIM_BOTH/UPPER/LOWER 或以动画长度为保持时间;预览显示该行,**{{MbInsSkriptEinfuegen}}** 以一个撤销步骤插入。动画字段中的小按钮使用场景资源管理器中选中的动画;Enter 插入,Esc 或 **{{UfAbbrechen}}** 关闭窗口。窗口关闭时不占空间。",
        "モデルタブはモデル専用です。スクリプトツリー、ボタン列、コンパイル状態はなく、ファイル > 開く はモデルを開き、Ctrl+S はスクリプトに触れません。アニメーションをスクリプトに入れるには **{{MbInsSkriptFenster}}** を開きます (シーンエクスプローラー上部のスクリプトのアイコンのボタン、シーケンスの右クリック、ファイルメニュー、Ctrl+Shift+I)。スクリプト (開いているものか新規)、affect ブロックのキャラクター (空欄: なし)、コマンド (SET_ANIM_BOTH/UPPER/LOWER、またはアニメーションの長さのホールド時間) を選ぶと、プレビューに行が表示され、**{{MbInsSkriptEinfuegen}}** で元に戻せる 1 回の操作として挿入します。アニメーション欄の小さなボタンはシーンエクスプローラーで選択中のアニメーションを入れます。Enter で挿入、Esc または **{{UfAbbrechen}}** でウィンドウを閉じます。ウィンドウを閉じている間は場所を取りません。",
        "La pestaña Model es solo para modelos: sin árbol del script, sin columna de botones, sin estado de compilación; Archivo > Abrir abre un modelo y Ctrl+S no toca el script. Para poner una animación en un script, abre **{{MbInsSkriptFenster}}** (el botón con el símbolo de script en la cabecera del explorador de escena, clic derecho en una secuencia, el menú Archivo o Ctrl+Mayús+I). Elige el script (abiertos o uno nuevo), el personaje para un bloque affect (vacío: ninguno) y el comando: SET_ANIM_BOTH/UPPER/LOWER o un tiempo de espera con la duración de la animación; la vista previa muestra la línea y **{{MbInsSkriptEinfuegen}}** la inserta con un paso de deshacer. El botón pequeño del campo de animación toma la animación elegida en el explorador de escena; Intro inserta, Esc o **{{UfAbbrechen}}** cierra la ventana. Mientras la ventana está cerrada no ocupa sitio.",
        "Karta Model je jen pro modely: žádný strom skriptu, žádný sloupec tlačítek, žádný stav kompilace; Soubor > Otevřít otevře model a Ctrl+S se skriptu nedotkne. Chcete-li dát animaci do skriptu, otevřete **{{MbInsSkriptFenster}}** (tlačítko se symbolem skriptu v záhlaví průzkumníku scény, pravé tlačítko na sekvenci, nabídka Soubor nebo Ctrl+Shift+I). Vyberte skript (otevřené nebo nový), postavu pro blok affect (prázdné: žádný) a příkaz - SET_ANIM_BOTH/UPPER/LOWER nebo dobu držení s délkou animace; náhled ukáže řádek a **{{MbInsSkriptEinfuegen}}** ho vloží jedním krokem zpět. Malé tlačítko v poli animace vezme animaci vybranou v průzkumníku scény; Enter vloží, Esc nebo **{{UfAbbrechen}}** okno zavře. Dokud je okno zavřené, nezabírá místo.",
    ),
    "b_bild_skript": (
        "The window Into script: one column of labels, all fields on one edge, the line in the code box, Insert (highlighted) and Cancel at the "
        "bottom right.",
        "Das Fenster Ins Skript: eine Spalte für die Beschriftungen, alle Felder an einer Kante, die Zeile im Code-Kasten, Einfügen "
        "(hervorgehoben) und Abbrechen unten rechts.",
        "\"插入脚本\" 窗口:标签一列,所有字段左边对齐,代码框中的该行,右下角的插入 (突出显示) 和取消。",
        "\"スクリプトへ\" ウィンドウ: ラベルは 1 列、すべての欄が同じ端にそろい、コードボックスに行、右下に挿入 (強調) とキャンセル。",
        "La ventana Al script: una columna de etiquetas, todos los campos en un mismo borde, la línea en el cuadro de código, Insertar "
        "(resaltado) y Cancelar abajo a la derecha.",
        "Okno Do skriptu: jeden sloupec popisků, všechna pole u jedné hrany, řádek v rámečku s kódem, Vložit (zvýrazněné) a Zrušit vpravo "
        "dole."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def seite():
    """Die Seite modell-bedienung (Gruppe Missionen, nach modell-ansehen). Die Seite modell-weights dieser Quelle (bis rc611)
    ist in inhalt_ap8.py aufgegangen, der Abschnitt "Symbole und Auge" steht beim Szenen-Explorer (inhalt_mb1a1.py)."""
    return {
        "id": "modell-bedienung",
        "gruppe": "mission",
        "titel": _t("b_titel"),
        "kurz": _t("b_kurz"),
        "bloecke": [
            ("img", "h_mb2_blender", _t("b_bild"), "breit"),
            ("p", _t("b_text")),
            ("h2", "max", _t("h_max")),
            ("p", _t("max")),
            ("keys", [
                (_t("k_maus"), _t("k_maus_t")),
                (_t("k_links"), _t("k_links_t")),
                (_t("k_qwer"), _t("k_qwer_t")),
                (_t("k_ansicht"), _t("k_ansicht_t")),
                (_t("k_anzeige"), _t("k_anzeige_t")),
                (_t("k_zeit"), _t("k_zeit_t")),
                (_t("k_auswahl"), _t("k_auswahl_t")),
            ]),
            ("img", "h_mb1_vier", _t("bild_vier"), "breit"),
            ("h2", "skript", _t("b_h_skript")),
            ("p", _t("b_skript")),
            ("img", "h_mb2_insskript", _t("b_bild_skript")),
        ],
    }
