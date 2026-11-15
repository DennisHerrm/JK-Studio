"""MBNAV (rc612): der Abschnitt "Navigation in den Ansichten" der Seite modell-ansehen (nach dem Boden-Raster).

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_mbkopf.py; je Bedienmodus eine Tabelle.
"""


X = {
    "h_nav": ("Navigating the views", "Navigation in den Ansichten", "视图导航", "ビューのナビゲーション", "Navegar por las vistas", "Navigace v pohledech"),
    "einleitung": (
        "Mouse and keys move a view exactly as in the program whose mode is set (**Settings > Model tab > Controls**): like 3ds Max or like Blender. "
        "The line above the views names the most important ones, every command is also in the menu of the view header ([View name] > "
        "**{{MkBlNavigation}}**, in Blender mode **View > {{MkBlNavigation}}**), in the right-click menu and in the search (<kbd>X</kbd> / <kbd>F3</kbd>). "
        "Panning moves the scene exactly with the mouse, zooming and orbiting keep the point in the middle of the view.",
        "Maus und Tasten bewegen eine Ansicht genau wie in dem Programm, dessen Modus eingestellt ist (**Einstellungen > Model-Tab > Bedienung**): wie "
        "3ds Max oder wie Blender. Die Zeile über den Ansichten nennt das Wichtigste, jeder Befehl steht auch im Menü der Kopfzeile ([Name der Ansicht] > "
        "**{{MkBlNavigation}}**, im Blender-Modus **View > {{MkBlNavigation}}**), im Rechtsklick-Menü und in der Suche (<kbd>X</kbd> / <kbd>F3</kbd>). "
        "Schieben bewegt die Szene genau mit der Maus, Zoomen und Kreisen halten den Punkt in der Bildmitte fest.",
        "鼠标和按键移动视图的方式与所设模式的程序完全相同 (**设置 > 模型选项卡 > 操作方式**): 如 3ds Max 或如 Blender。视图上方的一行列出最重要的操作, "
        "每个命令也在视图标题栏的菜单中 ([视图名] > **{{MkBlNavigation}}**, Blender 模式下为 **View > {{MkBlNavigation}}**)、右键菜单和搜索 "
        "(<kbd>X</kbd> / <kbd>F3</kbd>) 中。平移时场景精确跟随鼠标, 缩放和环绕保持视图中央的点不动。",
        "マウスとキーは、設定したモードのプログラムとまったく同じようにビューを動かします (**設定 > モデルタブ > 操作**): 3ds Max 風または Blender 風。"
        "ビューの上の行に主な操作が並び、すべてのコマンドはビューヘッダーのメニュー ([ビュー名] > **{{MkBlNavigation}}**、Blender モードでは "
        "**View > {{MkBlNavigation}}**)、右クリックメニュー、検索 (<kbd>X</kbd> / <kbd>F3</kbd>) にもあります。パンではシーンがマウスに正確に追従し、"
        "ズームとオービットはビュー中央の点を保ちます。",
        "El ratón y las teclas mueven una vista exactamente como en el programa cuyo modo está elegido (**Ajustes > Pestaña Modelo > Controles**): como "
        "3ds Max o como Blender. La línea sobre las vistas nombra lo más importante; cada orden está también en el menú de la cabecera de la vista "
        "([nombre de la vista] > **{{MkBlNavigation}}**, en modo Blender **View > {{MkBlNavigation}}**), en el menú contextual y en la búsqueda "
        "(<kbd>X</kbd> / <kbd>F3</kbd>). El encuadre mueve la escena exactamente con el ratón; el zoom y la órbita mantienen el punto del centro de la vista.",
        "Myš a klávesy pohybují pohledem přesně jako v programu, jehož režim je nastaven (**Nastavení > Karta Model > Ovládání**): jako 3ds Max nebo jako "
        "Blender. Řádek nad pohledy uvádí to nejdůležitější, každý příkaz je také v nabídce záhlaví pohledu ([název pohledu] > **{{MkBlNavigation}}**, "
        "v režimu Blender **View > {{MkBlNavigation}}**), v nabídce pravého tlačítka a ve hledání (<kbd>X</kbd> / <kbd>F3</kbd>). Posun hýbe scénou přesně "
        "s myší, zoom a obíhání drží bod uprostřed pohledu."),
    "h_max": ("Like 3ds Max", "Wie 3ds Max", "如 3ds Max", "3ds Max 風", "Como 3ds Max", "Jako 3ds Max"),
    "h_bl": ("Like Blender", "Wie Blender", "如 Blender", "Blender 風", "Como Blender", "Jako Blender"),
    "sp_taste": ("Mouse / key", "Maus / Taste", "鼠标 / 按键", "マウス / キー", "Ratón / tecla", "Myš / klávesa"),
    "sp_was": ("What it does", "Was es tut", "作用", "内容", "Qué hace", "Co dělá"),
    "mitte": ("middle mouse button", "Mitteltaste", "鼠标中键", "中ボタン", "botón central", "prostřední tlačítko"),
    "rad": ("mouse wheel", "Mausrad", "滚轮", "ホイール", "rueda", "kolečko"),
    # --- 3ds Max ---
    "m1": ("Pan: the view follows the mouse. With <kbd>Shift</kbd> only along the direction you move first.",
           "Schieben: die Ansicht folgt der Maus. Mit <kbd>Shift</kbd> nur entlang der Richtung, in die zuerst gezogen wird.",
           "平移: 视图跟随鼠标。按住 <kbd>Shift</kbd> 时仅沿最先拖动的方向。",
           "パン: ビューがマウスに追従します。<kbd>Shift</kbd> を押すと最初に動かした方向だけ。",
           "Encuadre: la vista sigue al ratón. Con <kbd>Shift</kbd> solo en la dirección en la que se mueve primero.",
           "Posun: pohled sleduje myš. Se <kbd>Shift</kbd> jen ve směru prvního pohybu."),
    "m2": ("Orbit about the point chosen under **{{MkBlNavigation}} > {{MnOrbitArt}}** (from the start the selection, without one the centre of the view). "
           "A view Top, Front, Left ... becomes the free view Orthographic. With <kbd>Shift</kbd> only about the axis you use first.",
           "Kreisen um den Punkt, der unter **{{MkBlNavigation}} > {{MnOrbitArt}}** gewählt ist (ab Werk die Auswahl, ohne Auswahl die Ansichtsmitte). "
           "Eine Ansicht Oben, Vorne, Links ... wird dabei zur freien Ansicht Orthographisch. Mit <kbd>Shift</kbd> nur um die zuerst benutzte Achse.",
           "围绕 **{{MkBlNavigation}} > {{MnOrbitArt}}** 中选择的点环绕 (默认是选择, 无选择时为视图中心)。顶、前、左等视图会变为自由的正交视图。"
           "按住 <kbd>Shift</kbd> 时仅绕最先使用的轴。",
           "**{{MkBlNavigation}} > {{MnOrbitArt}}** で選んだ点を中心にオービット (既定は選択、選択がなければビューの中心)。上・前・左などのビューは"
           "自由な平行投影ビューになります。<kbd>Shift</kbd> で最初に使った軸だけ。",
           "Orbitar alrededor del punto elegido en **{{MkBlNavigation}} > {{MnOrbitArt}}** (de fábrica la selección; sin ella, el centro de la vista). "
           "Una vista Superior, Frontal, Izquierda ... pasa a ser la vista libre Ortográfica. Con <kbd>Shift</kbd> solo sobre el eje que se usa primero.",
           "Obíhání kolem bodu zvoleného v **{{MkBlNavigation}} > {{MnOrbitArt}}** (z výroby výběr, bez výběru střed pohledu). Pohled Shora, Zepředu, "
           "Zleva ... se přitom stane volným pohledem Ortografický. Se <kbd>Shift</kbd> jen kolem osy použité jako první."),
    "m3": ("Zoom: up closer, down further away.", "Zoom: nach oben näher heran, nach unten weiter weg.", "缩放: 向上拉近, 向下拉远。", "ズーム: 上で近づき、下で離れます。",
           "Zoom: arriba acerca, abajo aleja.", "Zoom: nahoru blíž, dolů dál."),
    "m4": ("Zoom in steps about the point under the mouse (**{{MnZoomMausOrtho}}** / **{{MnZoomMausPersp}}**; off: about the centre of the view).",
           "Zoom in Schritten um den Punkt unter der Maus (**{{MnZoomMausOrtho}}** / **{{MnZoomMausPersp}}**; aus: um die Ansichtsmitte).",
           "围绕鼠标下的点分级缩放 (**{{MnZoomMausOrtho}}** / **{{MnZoomMausPersp}}**; 关闭时围绕视图中心)。",
           "マウスの下の点を中心に段階的にズーム (**{{MnZoomMausOrtho}}** / **{{MnZoomMausPersp}}**。オフならビューの中心)。",
           "Zoom por pasos sobre el punto bajo el ratón (**{{MnZoomMausOrtho}}** / **{{MnZoomMausPersp}}**; desactivado: sobre el centro de la vista).",
           "Zoom po krocích kolem bodu pod myší (**{{MnZoomMausOrtho}}** / **{{MnZoomMausPersp}}**; vypnuto: kolem středu pohledu)."),
    "m5": ("Zoom extents: <kbd>Z</kbd> fits the selection (without one the whole figure) into all views, <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>Z</kbd> the "
           "whole figure into the active view, <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>Z</kbd> the whole figure into all views (**{{MnGrenzenAlle}}**) - "
           "as in 3ds Max; Redo is <kbd>Ctrl</kbd>+<kbd>Y</kbd> there. In the mode like Blender <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>Z</kbd> stays Redo.",
           "Zoom Grenzen: <kbd>Z</kbd> passt die Auswahl (ohne Auswahl die ganze Figur) in alle Ansichten ein, <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>Z</kbd> "
           "die ganze Figur in die aktive Ansicht, <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>Z</kbd> die ganze Figur in alle Ansichten (**{{MnGrenzenAlle}}**) - "
           "wie in 3ds Max; Wiederholen ist dort <kbd>Ctrl</kbd>+<kbd>Y</kbd>. Im Modus wie Blender bleibt <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>Z</kbd> Wiederholen.",
           "最大化显示: <kbd>Z</kbd> 将选择 (无选择时为整个角色) 适配到所有视图, <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>Z</kbd> 将整个角色适配到活动视图, "
           "<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>Z</kbd> 将整个角色适配到所有视图 (**{{MnGrenzenAlle}}**) - 与 3ds Max 相同; 此时重做为 <kbd>Ctrl</kbd>+<kbd>Y</kbd>。"
           "在 Blender 模式下 <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>Z</kbd> 仍为重做。",
           "全体表示: <kbd>Z</kbd> は選択 (なければフィギュア全体) をすべてのビューに、<kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>Z</kbd> はフィギュア全体を"
           "アクティブビューに、<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>Z</kbd> はフィギュア全体をすべてのビューに合わせます (**{{MnGrenzenAlle}}**) - 3ds Max と同じです。"
           "やり直しはそこでは <kbd>Ctrl</kbd>+<kbd>Y</kbd> です。Blender 風モードでは <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>Z</kbd> はやり直しのままです。",
           "Encuadrar: <kbd>Z</kbd> ajusta la selección (sin ella, toda la figura) en todas las vistas, <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>Z</kbd> toda la "
           "figura en la vista activa, <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>Z</kbd> toda la figura en todas las vistas (**{{MnGrenzenAlle}}**), como en 3ds Max; "
           "allí Rehacer es <kbd>Ctrl</kbd>+<kbd>Y</kbd>. En el modo como Blender, <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>Z</kbd> sigue siendo Rehacer.",
           "Přiblížit vše: <kbd>Z</kbd> přizpůsobí výběr (bez něj celou postavu) všem pohledům, <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>Z</kbd> celou postavu "
           "aktivnímu pohledu, <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>Z</kbd> celou postavu všem pohledům (**{{MnGrenzenAlle}}**) - jako ve 3ds Max; Znovu je tam "
           "<kbd>Ctrl</kbd>+<kbd>Y</kbd>. V režimu jako Blender zůstává <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>Z</kbd> Znovu."),
    "m6": ("Twice as close / twice as far.", "Doppelt so nah / doppelt so weit.", "放大 2 倍 / 缩小 2 倍。", "2 倍に拡大 / 2 倍に縮小。", "El doble de cerca / el doble de lejos.",
           "Dvakrát blíž / dvakrát dál."),
    "m7": ("Navigation modes like the buttons at the bottom right of 3ds Max - Zoom, Pan, Orbit, Zoom Region: drag with the left button, the name of the "
           "mode stands at the bottom of the view; right-click or <kbd>Esc</kbd> ends it. Orbit shows the yellow circle: inside it the view turns freely, "
           "on one of the four handles about one axis only, outside it rolls. Zoom Region: the rectangle you drag fills the view afterwards.",
           "Navigationsmodi wie die Knöpfe unten rechts in 3ds Max - Zoom, Schieben, Orbit, Bereichszoom: mit der linken Taste ziehen, der Name des Modus "
           "steht unten in der Ansicht; Rechtsklick oder <kbd>Esc</kbd> beendet ihn. Orbit zeigt den gelben Kreis: innen dreht die Ansicht frei, an einem "
           "der vier Griffe nur um eine Achse, außerhalb rollt sie. Bereichszoom: das aufgezogene Rechteck füllt danach die Ansicht.",
           "与 3ds Max 右下角按钮相同的导航模式——缩放、平移、环绕、区域缩放: 用左键拖动, 模式名称显示在视图底部; 右键单击或 <kbd>Esc</kbd> 结束。"
           "环绕模式显示黄色圆圈: 圆内自由旋转, 在四个手柄之一上只绕一个轴, 圆外滚动。区域缩放: 拖出的矩形随后填满视图。",
           "3ds Max 右下のボタンと同じナビゲーションモード - ズーム、パン、オービット、領域ズーム: 左ボタンでドラッグします。モード名はビューの下に"
           "表示され、右クリックか <kbd>Esc</kbd> で終了します。オービットは黄色い円を表示: 内側は自由に回転、4 つのハンドル上は 1 軸のみ、外側はロール。"
           "領域ズーム: ドラッグした四角形がビュー全体に広がります。",
           "Modos de navegación como los botones de abajo a la derecha de 3ds Max - Zoom, Encuadre, Orbitar, Zoom de región: arrastra con el botón "
           "izquierdo; el nombre del modo aparece abajo en la vista; clic derecho o <kbd>Esc</kbd> lo termina. Orbitar muestra el círculo amarillo: "
           "dentro la vista gira libre, en uno de los cuatro tiradores solo sobre un eje, fuera gira sobre sí misma. Zoom de región: el rectángulo "
           "arrastrado llena después la vista.",
           "Navigační režimy jako tlačítka vpravo dole v 3ds Max - Zoom, Posun, Obíhání, Zoom oblasti: táhněte levým tlačítkem, název režimu stojí dole "
           "v pohledu; pravé tlačítko nebo <kbd>Esc</kbd> jej ukončí. Obíhání ukazuje žlutý kruh: uvnitř se pohled otáčí volně, na jednom ze čtyř úchytů "
           "jen kolem jedné osy, vně se natáčí. Zoom oblasti: natažený obdélník pak vyplní pohled."),
    "m8": ("Pan to the pointer: the point under the mouse comes into the middle of the view.", "Zum Mauszeiger schieben: der Punkt unter der Maus kommt in die Bildmitte.",
           "平移到指针: 鼠标下的点移到视图中央。", "ポインタへパン: マウスの下の点がビューの中央に来ます。", "Encuadre hacia el puntero: el punto bajo el ratón pasa al centro de la vista.",
           "Posun ke kurzoru: bod pod myší se dostane do středu pohledu."),
    "m9": ("Undo / redo the last view change of the active view (pan, zoom, turn, ViewCube) - a history of its own for each view, independent of "
           "<kbd>Ctrl</kbd>+<kbd>Z</kbd>.",
           "Die letzte Ansichtsänderung der aktiven Ansicht (Schieben, Zoom, Drehen, ViewCube) zurücknehmen / wiederholen - ein eigener Verlauf je Ansicht, "
           "unabhängig von <kbd>Ctrl</kbd>+<kbd>Z</kbd>.",
           "撤销 / 重做活动视图的上一次视图更改 (平移、缩放、旋转、ViewCube)——每个视图各有历史, 与 <kbd>Ctrl</kbd>+<kbd>Z</kbd> 无关。",
           "アクティブビューの直前のビュー変更 (パン、ズーム、回転、ViewCube) を元に戻す / やり直す - ビューごとの履歴で、<kbd>Ctrl</kbd>+<kbd>Z</kbd> とは独立。",
           "Deshacer / rehacer el último cambio de la vista activa (encuadre, zoom, giro, ViewCube): un historial propio de cada vista, independiente de "
           "<kbd>Ctrl</kbd>+<kbd>Z</kbd>.",
           "Vrátit / znovu provést poslední změnu aktivního pohledu (posun, zoom, otočení, ViewCube) - vlastní historie každého pohledu, nezávislá na "
           "<kbd>Ctrl</kbd>+<kbd>Z</kbd>."),
    "m10": ("Views Top, Bottom, Front, Left, Perspective, Orthographic; <kbd>V</kbd> opens the menu of all views at the mouse pointer.",
            "Ansichten Oben, Unten, Vorne, Links, Perspektive, Orthographisch; <kbd>V</kbd> öffnet das Menü aller Ansichten am Mauszeiger.",
            "顶、底、前、左、透视、正交视图; <kbd>V</kbd> 在鼠标指针处打开所有视图的菜单。", "上・下・前・左・パース・平行投影ビュー。<kbd>V</kbd> でマウス位置に全ビューのメニューを開きます。",
            "Vistas Superior, Inferior, Frontal, Izquierda, Perspectiva, Ortográfica; <kbd>V</kbd> abre el menú de todas las vistas en el puntero.",
            "Pohledy Shora, Zdola, Zepředu, Zleva, Perspektiva, Ortografický; <kbd>V</kbd> otevře nabídku všech pohledů u kurzoru."),
    "m11": ("One view or four; Home view (the ViewCube's Home); show or hide the ViewCube.", "Eine Ansicht oder vier; Home-Ansicht (Home des ViewCube); ViewCube zeigen oder ausblenden.",
            "单视图或四视图; 主视图 (ViewCube 的 Home); 显示或隐藏 ViewCube。", "1 ビューまたは 4 ビュー。ホームビュー (ViewCube のホーム)。ViewCube の表示 / 非表示。",
            "Una vista o cuatro; vista de inicio (Inicio del ViewCube); mostrar u ocultar el ViewCube.", "Jeden pohled nebo čtyři; výchozí pohled (Domů ViewCube); zobrazit nebo skrýt ViewCube."),
    # --- Blender ---
    "b1": ("Orbit (turntable): left and right about the vertical axis, up and down over the figure - also over the top. A view Front, Right, Top ... becomes "
           "the perspective (Auto Perspective). Hold <kbd>Alt</kbd> while dragging to snap to the views along the axes and diagonals.",
           "Kreisen (Turntable): links und rechts um die Hochachse, hoch und runter über die Figur - auch über den Pol. Eine Ansicht Vorne, Rechts, Oben ... "
           "wird dabei zur Perspektive (Auto Perspective). <kbd>Alt</kbd> beim Ziehen rastet auf die Ansichten entlang der Achsen und Diagonalen.",
           "环绕 (转盘): 左右绕垂直轴, 上下越过角色——也可越过顶点。前、右、顶等视图会变为透视 (自动透视)。拖动时按住 <kbd>Alt</kbd> 可吸附到沿轴和对角线的视图。",
           "オービット (ターンテーブル): 左右は垂直軸まわり、上下はフィギュアの上を越えて - 真上も越えられます。前・右・上などのビューはパースになります "
           "(自動パース)。ドラッグ中に <kbd>Alt</kbd> を押すと軸や対角方向のビューにスナップします。",
           "Orbitar (plato giratorio): izquierda y derecha sobre el eje vertical, arriba y abajo por encima de la figura, también pasando por el polo. Una vista "
           "Frontal, Derecha, Superior ... pasa a perspectiva (Auto Perspective). Mantén <kbd>Alt</kbd> al arrastrar para ajustarte a las vistas de los ejes y diagonales.",
           "Obíhání (točna): vlevo a vpravo kolem svislé osy, nahoru a dolů přes postavu - i přes pól. Pohled Zepředu, Zprava, Shora ... se přitom stane "
           "perspektivou (Auto Perspective). Držením <kbd>Alt</kbd> při tažení se přichytíte k pohledům podél os a úhlopříček."),
    "b2": ("Pan: the view follows the mouse.", "Schieben: die Ansicht folgt der Maus.", "平移: 视图跟随鼠标。", "パン: ビューがマウスに追従します。", "Encuadre: la vista sigue al ratón.",
           "Posun: pohled sleduje myš."),
    "b3": ("Zoom by dragging up and down (Blender's Dolly zoom).", "Zoom durch Ziehen nach oben und unten (Blenders Dolly-Zoom).", "上下拖动缩放 (Blender 的 Dolly 缩放)。",
           "上下ドラッグでズーム (Blender のドリーズーム)。", "Zoom arrastrando arriba y abajo (el zoom Dolly de Blender).", "Zoom tažením nahoru a dolů (Dolly zoom Blenderu)."),
    "b4": ("Zoom in steps: one step divides or multiplies the distance by 1.2, about the centre of the view (**{{MnBlZoomMaus}}** on: towards the mouse).",
           "Zoom in Schritten: ein Schritt teilt oder vervielfacht den Abstand um 1,2, um die Ansichtsmitte (**{{MnBlZoomMaus}}** an: zur Maus).",
           "分级缩放: 每级将距离除以或乘以 1.2, 围绕视图中心 (开启 **{{MnBlZoomMaus}}** 时朝向鼠标)。",
           "段階ズーム: 1 段で距離を 1.2 で割るか掛けます。ビューの中心が基準 (**{{MnBlZoomMaus}}** オンでマウス位置へ)。",
           "Zoom por pasos: un paso divide o multiplica la distancia por 1,2, sobre el centro de la vista (**{{MnBlZoomMaus}}** activado: hacia el ratón).",
           "Zoom po krocích: jeden krok dělí nebo násobí vzdálenost 1,2, kolem středu pohledu (**{{MnBlZoomMaus}}** zapnuto: k myši)."),
    "b5": ("Dragging turns the view a quarter turn on to the neighbouring axis view (right, left, up, down); a click puts the point of the figure under the "
           "mouse into the middle of the view.",
           "Ziehen dreht die Ansicht eine Vierteldrehung weiter auf die benachbarte Achsenansicht (rechts, links, hoch, runter); ein Klick legt den Punkt der "
           "Figur unter der Maus in die Bildmitte.",
           "拖动将视图转四分之一圈到相邻的轴向视图 (右、左、上、下); 单击将鼠标下角色上的点移到视图中央。",
           "ドラッグでビューを 4 分の 1 回転させ、隣の軸ビューへ (右・左・上・下)。クリックでマウスの下のフィギュア上の点をビューの中央に。",
           "Arrastrar gira la vista un cuarto de vuelta hasta la vista de eje vecina (derecha, izquierda, arriba, abajo); un clic lleva el punto de la figura "
           "bajo el ratón al centro de la vista.",
           "Tažení otočí pohled o čtvrt otáčky na sousední osový pohled (vpravo, vlevo, nahoru, dolů); klepnutí přesune bod postavy pod myší do středu pohledu."),
    "b6": ("Front, Right, Top (with <kbd>Ctrl</kbd> the opposite side), orthographic and turned smoothly; <kbd>Numpad 5</kbd> switches between perspective and "
           "orthographic in the same direction; <kbd>Numpad 9</kbd> the opposite side.",
           "Vorne, Rechts, Oben (mit <kbd>Ctrl</kbd> die Gegenseite), orthographisch und weich gedreht; <kbd>Numpad 5</kbd> schaltet zwischen Perspektive und "
           "orthographisch in derselben Richtung um; <kbd>Numpad 9</kbd> die Gegenseite.",
           "前、右、顶 (按 <kbd>Ctrl</kbd> 为对面), 正交并平滑旋转; <kbd>Numpad 5</kbd> 在同一方向上切换透视与正交; <kbd>Numpad 9</kbd> 为对面。",
           "前・右・上 (<kbd>Ctrl</kbd> で反対側)。平行投影でなめらかに回転します。<kbd>Numpad 5</kbd> は同じ向きのままパースと平行投影を切り替え、"
           "<kbd>Numpad 9</kbd> は反対側。",
           "Frontal, Derecha, Superior (con <kbd>Ctrl</kbd> el lado opuesto), ortográficas y con giro suave; <kbd>Numpad 5</kbd> cambia entre perspectiva y "
           "ortográfica en la misma dirección; <kbd>Numpad 9</kbd> el lado opuesto.",
           "Zepředu, Zprava, Shora (s <kbd>Ctrl</kbd> opačná strana), ortograficky a plynule; <kbd>Numpad 5</kbd> přepíná mezi perspektivou a ortografickým "
           "pohledem ve stejném směru; <kbd>Numpad 9</kbd> opačná strana."),
    "b7": ("Orbit in steps of 15 degrees (4 and 6: the camera goes left and right, 8 and 2: up and down); with <kbd>Shift</kbd> 4 and 6 roll the view, with "
           "<kbd>Ctrl</kbd> the four keys pan it.",
           "In 15-Grad-Schritten kreisen (4 und 6: die Kamera geht nach links und rechts, 8 und 2: hoch und runter); mit <kbd>Shift</kbd> rollen 4 und 6 die "
           "Ansicht, mit <kbd>Ctrl</kbd> schieben die vier Tasten sie.",
           "以 15 度为步长环绕 (4 和 6: 相机向左和向右, 8 和 2: 向上和向下); 按 <kbd>Shift</kbd> 时 4 和 6 滚动视图, 按 <kbd>Ctrl</kbd> 时四个键平移视图。",
           "15 度ずつオービット (4 と 6: カメラが左右へ、8 と 2: 上下へ)。<kbd>Shift</kbd> で 4 と 6 はロール、<kbd>Ctrl</kbd> で 4 つのキーはパン。",
           "Orbitar en pasos de 15 grados (4 y 6: la cámara va a izquierda y derecha, 8 y 2: arriba y abajo); con <kbd>Shift</kbd> 4 y 6 giran la vista sobre "
           "sí misma, con <kbd>Ctrl</kbd> las cuatro teclas la encuadran.",
           "Obíhání po 15 stupních (4 a 6: kamera jde vlevo a vpravo, 8 a 2: nahoru a dolů); se <kbd>Shift</kbd> 4 a 6 pohled natáčejí, s <kbd>Ctrl</kbd> jej "
           "čtyři klávesy posouvají."),
    "b8": ("Frame Selected (nothing selected: nothing happens), Frame All, and Frame All including the origin. The direction stays.",
           "Auswahl zeigen (nichts gewählt: nichts geschieht), Alles zeigen und Alles zeigen samt Ursprung. Die Richtung bleibt.",
           "框显所选 (未选择时无操作)、框显全部, 以及包含原点的框显全部。方向不变。", "選択を表示 (選択なしでは何もしません)、すべて表示、原点を含めてすべて表示。向きはそのまま。",
           "Encuadrar selección (sin selección no ocurre nada), Encuadrar todo y Encuadrar todo incluido el origen. La dirección se mantiene.",
           "Zobrazit výběr (nic nevybráno: nic se nestane), Zobrazit vše a Zobrazit vše včetně počátku. Směr zůstane."),
    "b9": ("The View pie menu at the mouse pointer (Left, Right, Bottom, Top, Front, Back, View Selected); Zoom Region - drag a rectangle that then fills "
           "the view; the quad view with Top, the perspective, Front and Right.",
           "Das Kuchenmenü View am Mauszeiger (Left, Right, Bottom, Top, Front, Back, View Selected); Bereich zoomen - ein Rechteck aufziehen, das danach "
           "die Ansicht füllt; die Vierer-Ansicht mit Oben, der Perspektive, Vorne und Rechts.",
           "鼠标指针处的视图饼菜单 (左、右、底、顶、前、后、框显所选); 缩放区域——拖出一个矩形, 之后它填满视图; 四视图: 顶、透视、前、右。",
           "マウス位置のビューパイメニュー (左・右・下・上・前・後・選択を表示)。領域ズーム - 四角形をドラッグするとビュー全体に広がります。"
           "4 分割表示は上、パース、前、右。",
           "El menú circular Vista en el puntero (Izquierda, Derecha, Inferior, Superior, Frontal, Trasera, Encuadrar selección); Zoom de región: arrastra un "
           "rectángulo que luego llena la vista; la vista cuádruple con Superior, la perspectiva, Frontal y Derecha.",
           "Kruhová nabídka Pohled u kurzoru (Zleva, Zprava, Zdola, Shora, Zepředu, Zezadu, Zobrazit výběr); Zoom oblasti - natáhněte obdélník, který pak "
           "vyplní pohled; čtyři pohledy: Shora, perspektiva, Zepředu a Zprava."),
    "b10": ("The gizmo at the top right: click an axis for its view (looking straight along an axis, the middle gives the opposite side), drag it to orbit; "
            "the buttons below zoom and pan by dragging and switch between perspective and orthographic.",
            "Das Gizmo oben rechts: eine Achse anklicken für ihre Ansicht (schaut man genau entlang einer Achse, gibt die Mitte die Gegenseite), daran ziehen "
            "zum Kreisen; die Knöpfe darunter zoomen und schieben durch Ziehen und schalten zwischen Perspektive und orthographisch um.",
            "右上角的 Gizmo: 单击轴切换到其视图 (正对某轴时, 单击中间为对面), 拖动它可环绕; 下方的按钮通过拖动缩放和平移, 并在透视与正交之间切换。",
            "右上のギズモ: 軸をクリックでそのビュー (軸をまっすぐ見ているとき、中央は反対側)、ドラッグでオービット。下のボタンはドラッグでズームとパン、"
            "パースと平行投影の切り替え。",
            "El gizmo arriba a la derecha: clic en un eje para su vista (mirando justo a lo largo de un eje, el centro da el lado opuesto), arrástralo para "
            "orbitar; los botones de debajo hacen zoom y encuadre arrastrando y cambian entre perspectiva y ortográfica.",
            "Gizmo vpravo nahoře: klepnutím na osu její pohled (při pohledu přesně podél osy dá střed opačnou stranu), tažením obíhání; tlačítka pod ním "
            "tažením přibližují a posouvají a přepínají mezi perspektivou a ortografickým pohledem."),
    "einst": (
        "**{{MkBlNavigation}}** also holds the settings: like 3ds Max **{{MnOrbitArt}}** ({{MnOrbitMitte}}, {{MnOrbitAuswahl}}, {{MnOrbitUnter}}, "
        "{{MnOrbitPunkt}}) and the two switches **{{MnZoomMausOrtho}}** / **{{MnZoomMausPersp}}**; like Blender **{{MnBlOrbitAuswahl}}** and "
        "**{{MnBlZoomMaus}}** (both off from the start, as in Blender). Not built: the SteeringWheels and the walkthrough mode of 3ds Max, Walk/Fly "
        "navigation and the trackball orbit of Blender, camera views.",
        "Unter **{{MkBlNavigation}}** stehen auch die Einstellungen: wie 3ds Max **{{MnOrbitArt}}** ({{MnOrbitMitte}}, {{MnOrbitAuswahl}}, {{MnOrbitUnter}}, "
        "{{MnOrbitPunkt}}) und die beiden Schalter **{{MnZoomMausOrtho}}** / **{{MnZoomMausPersp}}**; wie Blender **{{MnBlOrbitAuswahl}}** und "
        "**{{MnBlZoomMaus}}** (beide ab Werk aus wie in Blender). Nicht nachgebaut: die SteeringWheels und der Durchgangsmodus von 3ds Max, Gehen/Fliegen "
        "und das Trackball-Kreisen von Blender, Kameraansichten.",
        "**{{MkBlNavigation}}** 中还有设置: 如 3ds Max 的 **{{MnOrbitArt}}** ({{MnOrbitMitte}}、{{MnOrbitAuswahl}}、{{MnOrbitUnter}}、{{MnOrbitPunkt}}) "
        "和两个开关 **{{MnZoomMausOrtho}}** / **{{MnZoomMausPersp}}**; 如 Blender 的 **{{MnBlOrbitAuswahl}}** 和 **{{MnBlZoomMaus}}** (与 Blender 一样默认关闭)。"
        "未实现: 3ds Max 的 SteeringWheels 和穿行模式, Blender 的行走/飞行导航和轨迹球环绕, 摄像机视图。",
        "**{{MkBlNavigation}}** には設定もあります: 3ds Max 風では **{{MnOrbitArt}}** ({{MnOrbitMitte}}、{{MnOrbitAuswahl}}、{{MnOrbitUnter}}、{{MnOrbitPunkt}}) "
        "と 2 つのスイッチ **{{MnZoomMausOrtho}}** / **{{MnZoomMausPersp}}**、Blender 風では **{{MnBlOrbitAuswahl}}** と **{{MnBlZoomMaus}}** "
        "(Blender と同じく既定でオフ)。未実装: 3ds Max の SteeringWheels とウォークスルー、Blender のウォーク/フライとトラックボール回転、カメラビュー。",
        "En **{{MkBlNavigation}}** están también los ajustes: como 3ds Max **{{MnOrbitArt}}** ({{MnOrbitMitte}}, {{MnOrbitAuswahl}}, {{MnOrbitUnter}}, "
        "{{MnOrbitPunkt}}) y los dos interruptores **{{MnZoomMausOrtho}}** / **{{MnZoomMausPersp}}**; como Blender **{{MnBlOrbitAuswahl}}** y "
        "**{{MnBlZoomMaus}}** (ambos desactivados de fábrica, como en Blender). No incluidos: los SteeringWheels y el modo de recorrido de 3ds Max, la "
        "navegación Caminar/Volar y la órbita trackball de Blender, las vistas de cámara.",
        "V **{{MkBlNavigation}}** jsou i nastavení: jako 3ds Max **{{MnOrbitArt}}** ({{MnOrbitMitte}}, {{MnOrbitAuswahl}}, {{MnOrbitUnter}}, {{MnOrbitPunkt}}) "
        "a dva přepínače **{{MnZoomMausOrtho}}** / **{{MnZoomMausPersp}}**; jako Blender **{{MnBlOrbitAuswahl}}** a **{{MnBlZoomMaus}}** (oba z výroby "
        "vypnuté jako v Blenderu). Nepostaveno: SteeringWheels a režim procházení 3ds Max, chůze/let a trackball Blenderu, pohledy kamery."),
    "bild_max": ("3ds Max mode: the menu Navigation of the view header with the commands of the viewport controls and the settings of the mouse.",
                 "Modus wie 3ds Max: das Menü Navigation der Kopfzeile mit den Befehlen der Ansichtssteuerung und den Einstellungen der Maus.",
                 "3ds Max 模式: 视图标题栏的导航菜单, 含视口控制命令和鼠标设置。", "3ds Max モード: ビューヘッダーのナビゲーションメニュー。ビューポート操作のコマンドとマウス設定。",
                 "Modo 3ds Max: el menú Navegación de la cabecera con las órdenes de los controles de vista y los ajustes del ratón.",
                 "Režim 3ds Max: nabídka Navigace v záhlaví pohledu s příkazy ovládání pohledu a nastavením myši."),
    "bild_orbit": ("The orbit mode (Ctrl+R): the yellow circle with its four handles; the name of the mode at the bottom of the view.",
                   "Der Orbit-Modus (Strg+R): der gelbe Kreis mit seinen vier Griffen; unten in der Ansicht der Name des Modus.",
                   "环绕模式 (Ctrl+R): 带四个手柄的黄色圆圈; 视图底部显示模式名称。", "オービットモード (Ctrl+R): 4 つのハンドルが付いた黄色い円。ビューの下にモード名。",
                   "El modo orbitar (Ctrl+R): el círculo amarillo con sus cuatro tiradores; abajo en la vista, el nombre del modo.",
                   "Režim obíhání (Ctrl+R): žlutý kruh se čtyřmi úchyty; dole v pohledu název režimu."),
    "bild_bl": ("Blender mode: the View pie menu (`) at the mouse pointer.", "Modus wie Blender: das Kuchenmenü View (`) am Mauszeiger.", "Blender 模式: 鼠标指针处的视图饼菜单 (`)。",
                "Blender モード: マウス位置のビューパイメニュー (`)。", "Modo Blender: el menú circular Vista (`) en el puntero.", "Režim Blender: kruhová nabídka Pohled (`) u kurzoru."),
}

# Die Tabellenzeilen: Maus/Taste (in allen Sprachen gleich bis auf Mitteltaste und Rad) und der Text
_MAX = [
    ("{M}", "m1"),
    ("<kbd>Alt</kbd>+{M}", "m2"),
    ("<kbd>Ctrl</kbd>+<kbd>Alt</kbd>+{M}", "m3"),
    ("{R}, <kbd>[</kbd> <kbd>]</kbd>, <kbd>Ctrl</kbd>+<kbd>=</kbd> <kbd>Ctrl</kbd>+<kbd>-</kbd>", "m4"),
    ("<kbd>Z</kbd>, <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>Z</kbd>", "m5"),
    ("<kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>Shift</kbd>+<kbd>Z</kbd>, <kbd>Alt</kbd>+<kbd>Shift</kbd>+<kbd>Z</kbd>", "m6"),
    ("<kbd>Alt</kbd>+<kbd>Z</kbd>, <kbd>Ctrl</kbd>+<kbd>P</kbd>, <kbd>Ctrl</kbd>+<kbd>R</kbd>, <kbd>Ctrl</kbd>+<kbd>W</kbd>", "m7"),
    ("<kbd>I</kbd>", "m8"),
    ("<kbd>Shift</kbd>+<kbd>Z</kbd>, <kbd>Shift</kbd>+<kbd>Y</kbd>", "m9"),
    ("<kbd>T</kbd> <kbd>B</kbd> <kbd>F</kbd> <kbd>L</kbd> <kbd>P</kbd> <kbd>U</kbd>, <kbd>V</kbd>", "m10"),
    ("<kbd>Alt</kbd>+<kbd>W</kbd>, <kbd>Alt</kbd>+<kbd>Home</kbd>, <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>V</kbd>", "m11"),
]
_BLENDER = [
    ("{M}", "b1"),
    ("<kbd>Shift</kbd>+{M}", "b2"),
    ("<kbd>Ctrl</kbd>+{M}", "b3"),
    ("{R}, <kbd>Numpad +</kbd> <kbd>Numpad -</kbd>, <kbd>Ctrl</kbd>+<kbd>=</kbd> <kbd>Ctrl</kbd>+<kbd>-</kbd>", "b4"),
    ("<kbd>Alt</kbd>+{M}", "b5"),
    ("<kbd>Numpad 1</kbd> <kbd>Numpad 3</kbd> <kbd>Numpad 7</kbd>, <kbd>Numpad 5</kbd>, <kbd>Numpad 9</kbd>", "b6"),
    ("<kbd>Numpad 4</kbd> <kbd>Numpad 6</kbd> <kbd>Numpad 8</kbd> <kbd>Numpad 2</kbd>", "b7"),
    ("<kbd>Numpad .</kbd>, <kbd>Home</kbd>, <kbd>Shift</kbd>+<kbd>C</kbd>", "b8"),
    ("<kbd>`</kbd>, <kbd>Shift</kbd>+<kbd>B</kbd>, <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>Q</kbd>", "b9"),
    ("Gizmo", "b10"),
]


# Tastennamen wie auf der Tastatur des Landes (im Fliesstext uebersetzt der Generator sie nicht): Deutsch und Spanisch
_TASTEN = {1: (("Ctrl", "Strg"), ("Shift", "Umschalt"), ("Home", "Pos1")), 4: (("Shift", "Mayús"), ("Home", "Inicio"))}


def _lok(text, i):
    for alt, neu in _TASTEN.get(i, ()):
        text = text.replace("<kbd>%s</kbd>" % alt, "<kbd>%s</kbd>" % neu)
    return text


X = {k: tuple(_lok(t, i) for i, t in enumerate(v)) for k, v in X.items()}


def _taste(vorlage, i):
    return _lok(vorlage.replace("{M}", X["mitte"][i]).replace("{R}", X["rad"][i]), i)


def _tabellen_es_cs(i):
    return {_taste(v, 0): _taste(v, i) for v, _ in _MAX + _BLENDER}


# "rad" setzt _taste in die Tastenzeilen je Sprache selbst ein - als Eintrag der Tabelle sucht ihn keine Seite (TEST1411d:
# Hinweis "2 Eintraege in uebersetzung/ werden nicht mehr gebraucht" von gen_handbuch, es und cs)
ES = {t[0]: t[4] for k, t in X.items() if k != "rad"}
ES.update(_tabellen_es_cs(4))
CS = {t[0]: t[5] for k, t in X.items() if k != "rad"}
CS.update(_tabellen_es_cs(5))


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def _zeilen(liste):
    from inhalt_basis import T
    return [[T(_taste(v, 0), _taste(v, 1), _taste(v, 2), _taste(v, 3)), _t(k)] for v, k in liste]


def bloecke():
    """Die Bloecke des Abschnitts (in inhalt_mb1a1.seite nach dem Abschnitt von inhalt_mbraster)."""
    kopf = [_t("sp_taste"), _t("sp_was")]
    return [
        ("h2", "navigation", _t("h_nav")),
        ("p", _t("einleitung")),
        ("h3", _t("h_max")),
        ("table", kopf, _zeilen(_MAX)),
        ("img", "h_mbn_max", _t("bild_max"), "breit"),
        ("img", "h_mbn_orbit", _t("bild_orbit"), "breit"),
        ("h3", _t("h_bl")),
        ("table", kopf, _zeilen(_BLENDER)),
        ("img", "h_mbn_blender", _t("bild_bl"), "breit"),
        ("p", _t("einst")),
    ]
