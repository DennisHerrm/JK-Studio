"""AP0 + AP2 (MBENTWURF Teil B): der Szenen-Explorer (wie 3ds Max) / Outliner (wie Blender) des Model-Tabs.

Haengt Absaetze und Bilder an die Seite modell-ansehen (inhalt_mb1a1.py), Abschnitt "Szenen-Explorer", hinter das Bild des
Rechtsklick-Menues. Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_rest9.py.
"""

X = {
    "baum": (
        "**One tree for everything:** at the top the model, below it **{{MbXArtFlaechen}}** with their hierarchy, **{{MbXArtTags}}** with the "
        "models attached to them, **{{MbXArtKnochen}}**, the handles of the control rig and the sequences of the `animation.cfg` - the same "
        "tree in every area of the tab. A click on a sequence plays it as the primary sequence (lock with **P**), a right-click locks it as "
        "the secondary one (**S**); a red warning sign marks sequences whose frames lie behind the end of the GLA. The list under the node "
        "shows only one group (BOTH_, TORSO_, LEGS_, FACE_), the two buttons next to it copy the name and open **{{ModelAnimToScript}}** "
        "(also: double-click). **{{MbSuche}}** filters all rows by name.",
        "**Ein Baum für alles:** oben das Modell, darunter **{{MbXArtFlaechen}}** mit ihrer Hierarchie, **{{MbXArtTags}}** mit den daran "
        "angebauten Modellen, **{{MbXArtKnochen}}**, die Griffe des Control Rigs und die Sequenzen der `animation.cfg` - derselbe Baum in "
        "jedem Bereich des Tabs. Ein Klick auf eine Sequenz spielt sie als Primärsequenz (Schloss mit **P**), ein Rechtsklick sperrt sie als "
        "Sekundärsequenz (**S**); ein rotes Warnzeichen markiert Sequenzen, deren Bilder hinter dem Ende der GLA liegen. Die Liste unter dem "
        "Knoten zeigt nur eine Gruppe (BOTH_, TORSO_, LEGS_, FACE_), die zwei Knöpfe daneben kopieren den Namen und öffnen "
        "**{{ModelAnimToScript}}** (auch: Doppelklick). **{{MbSuche}}** filtert alle Zeilen nach dem Namen.",
        "**一棵树包含一切:**最上面是模型,其下是带层级的 **{{MbXArtFlaechen}}**、带挂接模型的 **{{MbXArtTags}}**、**{{MbXArtKnochen}}**、控制绑定的控制柄以及 "
        "`animation.cfg` 的序列——选项卡的每个区域都是同一棵树。单击序列将其作为主序列播放(带 **P** 的锁),右键可将其锁定为次序列(**S**);"
        "红色警告标志标出帧位于 GLA 末尾之后的序列。节点下的列表只显示一个分组(BOTH_、TORSO_、LEGS_、FACE_),旁边的两个按钮复制名称并打开 "
        "**{{ModelAnimToScript}}**(也可双击)。**{{MbSuche}}** 按名称筛选所有行。",
        "**すべてを 1 本のツリーに:**一番上がモデル、その下に階層付きの **{{MbXArtFlaechen}}**、取り付けたモデル付きの **{{MbXArtTags}}**、"
        "**{{MbXArtKnochen}}**、コントロールリグのハンドル、`animation.cfg` のシーケンスが並びます。タブのどのエリアでも同じツリーです。シーケンスを"
        "クリックするとプライマリとして再生し(**P** 付きの錠)、右クリックでセカンダリ(**S**)に固定できます。フレームが GLA の終わりを超える"
        "シーケンスには赤い警告マークが付きます。ノードの下のリストは 1 つのグループ(BOTH_、TORSO_、LEGS_、FACE_)だけを表示し、隣の 2 つのボタンは"
        "名前のコピーと **{{ModelAnimToScript}}** です(ダブルクリックでも可)。**{{MbSuche}}** はすべての行を名前で絞り込みます。",
        "**Un árbol para todo:** arriba el modelo y debajo **{{MbXArtFlaechen}}** con su jerarquía, **{{MbXArtTags}}** con los modelos "
        "acoplados, **{{MbXArtKnochen}}**, los controles del control rig y las secuencias del `animation.cfg`; el mismo árbol en todas las "
        "áreas de la pestaña. Un clic en una secuencia la reproduce como primaria (candado con **P**), un clic derecho la fija como "
        "secundaria (**S**); una señal roja marca las secuencias cuyos fotogramas quedan detrás del final del GLA. La lista bajo el nodo "
        "muestra solo un grupo (BOTH_, TORSO_, LEGS_, FACE_); los dos botones de al lado copian el nombre y abren **{{ModelAnimToScript}}** "
        "(también: doble clic). **{{MbSuche}}** filtra todas las filas por nombre.",
        "**Jeden strom pro všechno:** nahoře model, pod ním **{{MbXArtFlaechen}}** s hierarchií, **{{MbXArtTags}}** s připojenými modely, "
        "**{{MbXArtKnochen}}**, úchyty control rigu a sekvence z `animation.cfg` - stejný strom v každé oblasti karty. Kliknutí na sekvenci ji "
        "přehraje jako primární (zámek s **P**), pravé tlačítko ji zamkne jako sekundární (**S**); červená výstraha označuje sekvence, jejichž "
        "snímky leží za koncem GLA. Seznam pod uzlem ukáže jen jednu skupinu (BOTH_, TORSO_, LEGS_, FACE_), dvě tlačítka vedle zkopírují "
        "název a otevřou **{{ModelAnimToScript}}** (také: dvojklik). **{{MbSuche}}** filtruje všechny řádky podle názvu."),
    "bild_max": (
        "Like 3ds Max: arrow, eye and symbol stand directly in front of the name; the column with the snowflake freezes.",
        "Wie 3ds Max: Pfeil, Auge und Symbol stehen direkt vor dem Namen; die Spalte mit der Schneeflocke friert ein.",
        "与 3ds Max 相同:箭头、眼睛和符号紧挨在名称前;带雪花的列用于冻结。",
        "3ds Max と同じ: 矢印・目・記号が名前のすぐ前に並び、雪の結晶の列でフリーズします。",
        "Como 3ds Max: la flecha, el ojo y el símbolo están justo delante del nombre; la columna del copo de nieve congela.",
        "Jako 3ds Max: šipka, oko a symbol stojí přímo před názvem; sloupec s vločkou zmrazuje."),
    "zeile": (
        "**The row follows the control mode.** **{{MbWieMax}}** (scene explorer): the arrow that folds, the eye and the symbol stand "
        "directly in front of the name, each level is indented by one symbol; the column **{{MbXSpalteFrost}}** (snowflake) freezes. The "
        "column headings can be dragged in width, and a click on **{{MbXSpalteName}}** sorts ascending, descending and back to the order of "
        "the hierarchy. The row of symbols under the search shows or hides kinds of rows (surfaces, caps, tags, bones, sequences, attached "
        "models, rig handles; <kbd>Ctrl</kbd>+click: only this kind). **{{MbWieBlender}}** (outliner, switch **{{MbXHierarchie}}** / "
        "**{{MbXSammlungen}}**): the tree on the left, the eye and the arrow \"selectable\" in fixed columns on the right, the kinds in the "
        "**{{MbXFilter}}** popover next to the search (with **{{MbXKnochenFlach}}** and **{{MbXSortName}}**). The tree never scrolls "
        "sideways: deep bones move closer together, and a name that does not fit ends with \"...\" and stands in full in the tooltip, "
        "together with the info of the row (vertices, triangles, shader, parent, state in the game - also as **{{MbXInfo}}** in the "
        "right-click menu). <kbd>Shift</kbd>+click selects all surfaces from the last clicked one to this one. In front of a surface: a dot "
        "= on, a ring = off, a struck ring = off with its children, an anchor behind the name = root surface.",
        "**Die Zeile folgt dem Bedienmodus.** **{{MbWieMax}}** (Szenen-Explorer): der Pfeil zum Aufklappen, das Auge und das Symbol stehen "
        "direkt vor dem Namen, jede Ebene ist um ein Symbol eingerückt; die Spalte **{{MbXSpalteFrost}}** (Schneeflocke) friert ein. Die "
        "Spaltenköpfe lassen sich in der Breite ziehen, und ein Klick auf **{{MbXSpalteName}}** sortiert aufsteigend, absteigend und zurück "
        "zur Reihenfolge der Hierarchie. Die Symbolreihe unter der Suche zeigt Arten von Zeilen oder lässt sie weg (Flächen, Kappen, Tags, "
        "Knochen, Sequenzen, angebaute Modelle, Rig-Griffe; <kbd>Strg</kbd>+Klick: nur diese Art). **{{MbWieBlender}}** (Outliner, "
        "Umschalter **{{MbXHierarchie}}** / **{{MbXSammlungen}}**): der Baum links, das Auge und der Pfeil „wählbar“ rechts in festen "
        "Spalten, die Arten im Ausklapper **{{MbXFilter}}** neben der Suche (mit **{{MbXKnochenFlach}}** und **{{MbXSortName}}**). Der Baum "
        "rollt nie seitwärts: tiefe Knochen rücken enger zusammen, und ein Name, der nicht passt, endet mit „...“ und steht ganz im Tooltip, "
        "zusammen mit der Info der Zeile (Ecken, Dreiecke, Shader, Eltern, Status im Spiel - auch als **{{MbXInfo}}** im Rechtsklick-Menü). "
        "<kbd>Umschalt</kbd>+Klick wählt alle Flächen von der zuletzt angeklickten bis zu dieser. Vor einer Fläche: ein Punkt = an, ein "
        "Ring = aus, ein durchgestrichener Ring = aus samt Kindern, ein Anker hinter dem Namen = Wurzelfläche.",
        "**行的布局随操作模式而变。****{{MbWieMax}}**(场景资源管理器):折叠箭头、眼睛和符号紧挨在名称前,每一级缩进一个符号的宽度;"
        "**{{MbXSpalteFrost}}** 列(雪花)用于冻结。列标题可以拖动宽度,单击 **{{MbXSpalteName}}** 可按升序、降序排序并恢复层级顺序。搜索框下方的"
        "符号行可显示或隐藏各种行(面、封口、标签、骨骼、序列、挂接的模型、绑定控制柄;<kbd>Ctrl</kbd>+单击:仅此种类)。**{{MbWieBlender}}**"
        "(大纲视图,切换 **{{MbXHierarchie}}** / **{{MbXSammlungen}}**):树在左侧,眼睛和“可选”箭头在右侧的固定列中,种类位于搜索框旁的 "
        "**{{MbXFilter}}** 弹出面板中(含 **{{MbXKnochenFlach}}** 和 **{{MbXSortName}}**)。树从不横向滚动:较深的骨骼会排得更紧,放不下的名称以"
        "“...”结尾并在工具提示中完整显示,同时显示该行的信息(顶点、三角形、着色器、父级、游戏中的状态——右键菜单中的 **{{MbXInfo}}** 也可查看)。"
        "<kbd>Shift</kbd>+单击可选择从上次单击的面到此面的所有面。面前面的标记:圆点 = 开,圆环 = 关,带斜线的圆环 = 连同子级一起关,"
        "名称后面的锚 = 根面。",
        "**行の並びは操作モードに従います。****{{MbWieMax}}**(シーンエクスプローラー):折りたたみの矢印・目・記号が名前のすぐ前に並び、"
        "階層ごとに記号 1 つ分だけ字下げされます。**{{MbXSpalteFrost}}** 列(雪の結晶)でフリーズします。列見出しは幅をドラッグでき、"
        "**{{MbXSpalteName}}** をクリックすると昇順・降順・階層の順に切り替わります。検索の下の記号の行で行の種類(サーフェス、キャップ、タグ、"
        "ボーン、シーケンス、取り付けたモデル、リグのハンドル)を表示/非表示にします(<kbd>Ctrl</kbd>+クリック: この種類のみ)。"
        "**{{MbWieBlender}}**(アウトライナー、切り替えは **{{MbXHierarchie}}** / **{{MbXSammlungen}}**):ツリーは左、目と「選択可能」の矢印は右の"
        "固定列、種類は検索の隣の **{{MbXFilter}}** ポップオーバーにあります(**{{MbXKnochenFlach}}** と **{{MbXSortName}}** 付き)。ツリーは横に"
        "スクロールしません。深いボーンは間隔が詰まり、収まらない名前は「...」で終わってツールチップに全体が表示され、行の情報(頂点、三角形、"
        "シェーダー、親、ゲーム内の状態。右クリックメニューの **{{MbXInfo}}** でも表示)も出ます。<kbd>Shift</kbd>+クリックで、最後にクリックした"
        "サーフェスからここまでをすべて選択します。サーフェスの前の印: 点 = オン、輪 = オフ、斜線付きの輪 = 子ごとオフ、名前の後ろの錨 = "
        "ルートサーフェス。",
        "**La fila sigue al modo de control.** **{{MbWieMax}}** (explorador de escena): la flecha que pliega, el ojo y el símbolo están "
        "justo delante del nombre y cada nivel se sangra un símbolo; la columna **{{MbXSpalteFrost}}** (copo de nieve) congela. Los "
        "encabezados de columna se pueden arrastrar en anchura, y un clic en **{{MbXSpalteName}}** ordena de forma ascendente, descendente "
        "y de vuelta al orden de la jerarquía. La fila de símbolos bajo la búsqueda muestra u oculta tipos de filas (superficies, tapas, "
        "tags, huesos, secuencias, modelos acoplados, controles del rig; <kbd>Ctrl</kbd>+clic: solo este tipo). **{{MbWieBlender}}** "
        "(listado, conmutador **{{MbXHierarchie}}** / **{{MbXSammlungen}}**): el árbol a la izquierda, el ojo y la flecha «seleccionable» en "
        "columnas fijas a la derecha, los tipos en el desplegable **{{MbXFilter}}** junto a la búsqueda (con **{{MbXKnochenFlach}}** y "
        "**{{MbXSortName}}**). El árbol nunca se desplaza de lado: los huesos profundos se juntan más, y un nombre que no cabe termina en "
        "«...» y aparece entero en la descripción emergente, junto con la información de la fila (vértices, triángulos, shader, padre, "
        "estado en el juego; también como **{{MbXInfo}}** en el menú del clic derecho). <kbd>Mayús</kbd>+clic selecciona todas las "
        "superficies desde la última pulsada hasta esta. Delante de una superficie: un punto = activada, un anillo = desactivada, un anillo "
        "tachado = desactivada con sus hijas, un ancla detrás del nombre = superficie raíz.",
        "**Řádek se řídí režimem ovládání.** **{{MbWieMax}}** (průzkumník scény): šipka pro sbalení, oko a symbol stojí přímo před názvem, "
        "každá úroveň je odsazena o jeden symbol; sloupec **{{MbXSpalteFrost}}** (vločka) zmrazuje. Záhlaví sloupců lze táhnout do šířky a "
        "kliknutí na **{{MbXSpalteName}}** řadí vzestupně, sestupně a zpět na pořadí hierarchie. Řada symbolů pod hledáním zobrazuje nebo "
        "skrývá druhy řádků (povrchy, záslepky, tagy, kosti, sekvence, připojené modely, úchyty rigu; <kbd>Ctrl</kbd>+klik: jen tento druh). "
        "**{{MbWieBlender}}** (outliner, přepínač **{{MbXHierarchie}}** / **{{MbXSammlungen}}**): strom vlevo, oko a šipka „lze vybrat“ v "
        "pevných sloupcích vpravo, druhy ve vyskakovacím panelu **{{MbXFilter}}** vedle hledání (s **{{MbXKnochenFlach}}** a "
        "**{{MbXSortName}}**). Strom se nikdy neposouvá do strany: hluboké kosti se srazí blíž k sobě a název, který se nevejde, končí "
        "„...“ a celý stojí v bublině spolu s informacemi o řádku (vrcholy, trojúhelníky, shader, rodič, stav ve hře - také jako "
        "**{{MbXInfo}}** v nabídce pravého tlačítka). <kbd>Shift</kbd>+klik vybere všechny povrchy od naposledy kliknutého po tento. Před "
        "povrchem: tečka = zapnuto, kroužek = vypnuto, přeškrtnutý kroužek = vypnuto i s potomky, kotva za názvem = kořenový povrch."),
    "bild_blender": (
        "Like Blender: the outliner with the eye and the arrow \"selectable\" in columns on the right.",
        "Wie Blender: der Outliner mit Auge und Pfeil „wählbar“ rechts in Spalten.",
        "与 Blender 相同:大纲视图,眼睛和“可选”箭头位于右侧的列中。",
        "Blender と同じ: アウトライナー。目と「選択可能」の矢印は右の列にあります。",
        "Como Blender: el listado con el ojo y la flecha «seleccionable» en columnas a la derecha.",
        "Jako Blender: outliner s okem a šipkou „lze vybrat“ ve sloupcích vpravo."),
    "ebenen": (
        "**{{MbNachEbenen}}** (like Blender: **{{MbXSammlungen}}**) sorts the same rows into layers: **{{MbXEbeneKoerper}}**, "
        "**{{MbEbeneKappen}}**, **{{MbXArtTags}}**, **{{MbXArtKnochen}}**, **{{MbXEbeneAnbauten}}** and the further levels of detail (LOD 2, "
        "LOD 3 ...). The eye of a layer hides all its rows in the view only - the game stays as it is; what the game draws is still "
        "switched with on/off of the surfaces. The snowflake (like Blender: the arrow) freezes: frozen rows stay visible, but a click in the "
        "tree does not select them, a click in the view goes through to the surface behind (like 3ds Max), the selection frame passes "
        "them by, and if nothing lies behind, the status line says why. The caps are frozen "
        "from the start, so a click on the figure never picks a cap. The eye of a LOD layer shows that level of detail. Every single row "
        "has the same two switches; <kbd>Ctrl</kbd>+click on an eye shows only this row, <kbd>Shift</kbd>+click on the snowflake takes the "
        "children along.",
        "**{{MbNachEbenen}}** (wie Blender: **{{MbXSammlungen}}**) ordnet dieselben Zeilen in Ebenen: **{{MbXEbeneKoerper}}**, "
        "**{{MbEbeneKappen}}**, **{{MbXArtTags}}**, **{{MbXArtKnochen}}**, **{{MbXEbeneAnbauten}}** und die weiteren Detailstufen (LOD 2, "
        "LOD 3 ...). Das Auge einer Ebene blendet alle ihre Zeilen nur in der Ansicht aus - das Spiel bleibt, wie es ist; was das Spiel "
        "zeichnet, schaltet weiter an/aus der Flächen. Die Schneeflocke (wie Blender: der Pfeil) friert ein: Eingefrorenes bleibt sichtbar, "
        "aber ein Klick im Baum wählt es nicht, ein Klick in der Ansicht geht hindurch auf die Fläche dahinter (wie 3ds Max), der "
        "Auswahlrahmen lässt es aus, und liegt dahinter nichts, sagt die Statuszeile, warum. Die "
        "Kappen sind von Anfang an eingefroren, damit ein Klick auf die Figur nie eine Kappe trifft. Das Auge einer LOD-Ebene zeigt diese "
        "Detailstufe. Jede einzelne Zeile hat dieselben zwei Schalter; <kbd>Strg</kbd>+Klick auf ein Auge zeigt nur diese Zeile, "
        "<kbd>Umschalt</kbd>+Klick auf die Schneeflocke nimmt die Kinder mit.",
        "**{{MbNachEbenen}}**(与 Blender 相同:**{{MbXSammlungen}}**)把同样的行归入图层:**{{MbXEbeneKoerper}}**、**{{MbEbeneKappen}}**、"
        "**{{MbXArtTags}}**、**{{MbXArtKnochen}}**、**{{MbXEbeneAnbauten}}** 以及其他细节级别(LOD 2、LOD 3 ……)。图层的眼睛只在视图中隐藏其所有行"
        "——游戏保持不变;游戏绘制什么仍由面的开/关决定。雪花(与 Blender 相同:箭头)用于冻结:冻结的行保持可见,但在树中单击不会选中,"
        "在视图中单击会穿过它选中后面的面(与 3ds Max 相同),选择框会跳过它们,若后面没有面,状态栏会说明原因。封口从一开始就是冻结的,因此单击角色时不会选中封口。LOD 图层的眼睛显示该细节级别。每一行都有"
        "同样的两个开关;<kbd>Ctrl</kbd>+单击眼睛只显示这一行,<kbd>Shift</kbd>+单击雪花会连同子级一起处理。",
        "**{{MbNachEbenen}}**(Blender と同じ: **{{MbXSammlungen}}**)は同じ行をレイヤーに分けます: **{{MbXEbeneKoerper}}**、"
        "**{{MbEbeneKappen}}**、**{{MbXArtTags}}**、**{{MbXArtKnochen}}**、**{{MbXEbeneAnbauten}}**、その他の詳細レベル(LOD 2、LOD 3 …)。"
        "レイヤーの目はその行すべてをビューでのみ隠します。ゲームはそのままで、ゲームが描くものは引き続きサーフェスのオン/オフで切り替えます。"
        "雪の結晶(Blender と同じ: 矢印)はフリーズです。フリーズした行は表示されたままですが、ツリーではクリックで選択されず、"
        "ビューでのクリックは後ろのサーフェスに抜けます (3ds Max と同じ)。選択枠も通り過ぎ、後ろに何もなければステータス行に理由が表示されます。キャップは最初からフリーズされているので、フィギュアをクリックしてもキャップは"
        "選ばれません。LOD レイヤーの目はその詳細レベルを表示します。どの行にも同じ 2 つのスイッチがあり、目を <kbd>Ctrl</kbd>+クリックすると"
        "その行だけを表示し、雪の結晶を <kbd>Shift</kbd>+クリックすると子も一緒に切り替えます。",
        "**{{MbNachEbenen}}** (como Blender: **{{MbXSammlungen}}**) ordena las mismas filas en capas: **{{MbXEbeneKoerper}}**, "
        "**{{MbEbeneKappen}}**, **{{MbXArtTags}}**, **{{MbXArtKnochen}}**, **{{MbXEbeneAnbauten}}** y los demás niveles de detalle (LOD 2, "
        "LOD 3 ...). El ojo de una capa oculta todas sus filas solo en la vista; el juego sigue como está: lo que dibuja el juego se sigue "
        "cambiando con activar/desactivar las superficies. El copo de nieve (como Blender: la flecha) congela: lo congelado sigue visible, "
        "pero un clic en el árbol no lo selecciona, un clic en la vista pasa a través hasta la superficie de detrás (como 3ds Max), el marco "
        "de selección lo pasa por alto y, si detrás no hay nada, la línea de estado dice por qué. "
        "Las tapas están congeladas desde el principio, para que un clic en la figura nunca elija una tapa. El ojo de una capa LOD muestra "
        "ese nivel de detalle. Cada fila tiene los mismos dos interruptores; <kbd>Ctrl</kbd>+clic en un ojo muestra solo esa fila, "
        "<kbd>Mayús</kbd>+clic en el copo de nieve incluye a las hijas.",
        "**{{MbNachEbenen}}** (jako Blender: **{{MbXSammlungen}}**) řadí stejné řádky do vrstev: **{{MbXEbeneKoerper}}**, "
        "**{{MbEbeneKappen}}**, **{{MbXArtTags}}**, **{{MbXArtKnochen}}**, **{{MbXEbeneAnbauten}}** a další úrovně detailu (LOD 2, LOD 3 "
        "...). Oko vrstvy skryje všechny její řádky jen v pohledu - hra zůstává, jak je; co hra kreslí, se dál přepíná zapnutím/vypnutím "
        "povrchů. Vločka (jako Blender: šipka) zmrazí: zmrazené zůstává viditelné, ale kliknutí ve stromu je nevybere, kliknutí v pohledu "
        "projde na plochu za ním (jako 3ds Max), výběrový rámeček je vynechá, a když za ním nic není, stavový řádek řekne proč. Záslepky jsou zmrazené od začátku, aby kliknutí na postavu nikdy "
        "nevybralo záslepku. Oko vrstvy LOD zobrazí tuto úroveň detailu. Každý jednotlivý řádek má tytéž dva přepínače; <kbd>Ctrl</kbd>+klik "
        "na oko ukáže jen tento řádek, <kbd>Shift</kbd>+klik na vločku vezme i potomky."),
    "bild_ebenen": (
        "By layers: the caps are frozen (snowflake), every layer has its eye.",
        "Nach Ebenen: die Kappen sind eingefroren (Schneeflocke), jede Ebene hat ihr Auge.",
        "按图层:封口已冻结(雪花),每个图层都有自己的眼睛。",
        "レイヤー順: キャップはフリーズ(雪の結晶)、各レイヤーに目があります。",
        "Por capas: las tapas están congeladas (copo de nieve) y cada capa tiene su ojo.",
        "Podle vrstev: záslepky jsou zmrazené (vločka), každá vrstva má své oko."),
    "leiste": (
        "The tooltips of the mode bar on the left (**{{ViewEvents}}**, **{{ViewMap}}**, **{{ViewModel}}** ...) appear after the mouse has "
        "rested on a button for a moment, next to the bar at the height of the button - they no longer lie over the head of the panel "
        "beside it.",
        "Die Tooltips der Modusleiste links (**{{ViewEvents}}**, **{{ViewMap}}**, **{{ViewModel}}** ...) erscheinen, nachdem die Maus einen "
        "Moment auf einem Knopf geruht hat, neben der Leiste auf Höhe des Knopfs - sie liegen nicht mehr über dem Kopf des Felds daneben.",
        "左侧模式栏的工具提示(**{{ViewEvents}}**、**{{ViewMap}}**、**{{ViewModel}}** ……)在鼠标在按钮上停留片刻后出现,位于栏旁、与按钮等高的位置"
        "——不再盖住旁边面板的标题。",
        "左のモードバー(**{{ViewEvents}}**、**{{ViewMap}}**、**{{ViewModel}}** …)のツールチップは、マウスがボタンの上でしばらく止まってから、"
        "バーの横・ボタンの高さに表示されます。隣のパネルの見出しに重なることはなくなりました。",
        "Las descripciones emergentes de la barra de modos de la izquierda (**{{ViewEvents}}**, **{{ViewMap}}**, **{{ViewModel}}** ...) "
        "aparecen cuando el ratón ha descansado un momento sobre un botón, junto a la barra a la altura del botón; ya no quedan sobre la "
        "cabecera del panel de al lado.",
        "Bubliny lišty režimů vlevo (**{{ViewEvents}}**, **{{ViewMap}}**, **{{ViewModel}}** ...) se objeví, až myš chvíli spočine na "
        "tlačítku, vedle lišty ve výšce tlačítka - už neleží přes záhlaví sousedního panelu."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def einhaengen(ansehen):
    """Absaetze und Bilder in den Abschnitt "Szenen-Explorer" der Seite modell-ansehen: hinter das Rechtsklick-Menue und das
    Anbauen (Bild h_mb1_menue, ein Absatz), vor die Absaetze zu Sekundaersequenz, Symbolen und Auge."""
    b = ansehen["bloecke"]
    i = next(n for n, x in enumerate(b) if x[0] == "img" and x[1] == "h_mb1_menue") + 2
    if b[i - 1][0] != "p":
        raise SystemExit("inhalt_ap2: hinter dem Bild h_mb1_menue steht kein Absatz")
    neu = [
        ("p", _t("baum")),
        ("img", "h_ap2_max", _t("bild_max"), "breit"),
        ("p", _t("zeile")),
        ("img", "h_ap2_blender", _t("bild_blender"), "breit"),
        ("p", _t("ebenen")),
        ("img", "h_ap2_ebenen", _t("bild_ebenen"), "breit"),
        ("note", _t("leiste")),
    ]
    b[i:i] = neu
