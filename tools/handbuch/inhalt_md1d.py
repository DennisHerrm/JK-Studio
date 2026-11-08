"""Modell-Block 1d: die Seite "Model-Tab: Modellieren" (Unterebenen, Werkzeugkaestchen, Werkzeuge, Grundkoerper, JKA-Grenzen,
LODs, Requisite mit einem Klick in die Karte, Rueckweg, Lizenzhinweis).

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_uv1b.py.
"""

X = {
    "mr_blender": (
        "In Blender mode the keys of Blender's edit mode apply as well: Alt+click selects the edge loop under the mouse, Ctrl+Alt+click the edge ring (on polygons Alt+click selects the face loop), Ctrl+L grows the selection to everything linked (**{{MrVerbunden}}**), J connects two vertices and Ctrl+X dissolves edges (**{{MdRemove}}**). **{{MdChamfer}}** has **{{MdSegmente}}** and **{{MrProfilMax}}** (Blender: **{{MrProfilBlender}}**) in its caddy: 0.5 rounds the edge in a circle, 0.25 cuts it straight; every segment is one row of quads, other values space the points along the curve in equal steps. Where three or more chamfered edges meet, the corner is filled with a grid of quads like Blender's bevel (with an odd number of segments a small polygon sits in the middle). With profile 1 the corner stays square (its old point stays), the end of a single chamfered edge at a vertex with four edges becomes a fan of triangles, and the corner grid takes its texture coordinates from the neighbouring faces, all like Blender.",
        'Im Blender-Modus gelten dazu die Tasten aus Blenders Edit Mode: Alt+Klick wählt die Kantenschleife unter der Maus, Strg+Alt+Klick den Kantenring (auf Polygonen wählt Alt+Klick die Flächenschleife), Strg+L erweitert die Auswahl auf alles Verbundene (**{{MrVerbunden}}**), J verbindet zwei Punkte und Strg+X löst Kanten auf (**{{MdRemove}}**). **{{MdChamfer}}** hat im Kästchen **{{MdSegmente}}** und **{{MrProfilMax}}** (Blender: **{{MrProfilBlender}}**): 0,5 rundet die Kante im Kreis, 0,25 schneidet sie gerade; jedes Segment ist eine Reihe Vierecke, andere Werte setzen die Punkte in gleichen Schritten auf die Kurve. Wo drei oder mehr gefaste Kanten zusammenkommen, füllt ein Gitter aus Vierecken die Ecke wie Blenders Bevel (bei ungerader Segmentzahl sitzt in der Mitte ein kleines Vieleck). Mit Profil 1 bleibt die Ecke eckig (ihr alter Punkt bleibt), das Ende einer einzelnen gefasten Kante an einem Punkt mit vier Kanten wird ein Fächer aus Dreiecken, und das Gitter der Ecke nimmt seine Texturkoordinaten aus den Nachbarflächen, alles wie in Blender.',
        '在 Blender 模式下还适用 Blender 编辑模式的按键: Alt+单击选择鼠标下的边循环, Ctrl+Alt+单击选择边环 (在多边形上 Alt+单击选择面循环), Ctrl+L 把选择扩展到所有相连部分 (**{{MrVerbunden}}**), J 连接两个顶点, Ctrl+X 融并边 (**{{MdRemove}}**)。**{{MdChamfer}}** 的工具框中有 **{{MdSegmente}}** 和 **{{MrProfilMax}}** (Blender: **{{MrProfilBlender}}**): 0.5 把边倒成圆弧, 0.25 直切; 每个分段是一排四边形, 其他数值让顶点在曲线上等距分布。三条或更多倒角边相交处, 角落像 Blender 的倒角一样用四边形网格填充 (分段数为奇数时中间是一个小多边形)。轮廓为 1 时角保持方角 (原来的顶点保留), 单条倒角边在有四条边的顶点处的末端变成三角形扇, 角落网格的纹理坐标取自相邻的面, 都与 Blender 相同。',
        'Blender 風では Blender の編集モードのキーも使えます: Alt+クリックでマウスの下の辺ループ、Ctrl+Alt+クリックで辺リング (ポリゴンでは Alt+クリックで面ループ)、Ctrl+L でつながっている部分すべてに選択を広げ (**{{MrVerbunden}}**)、J で 2 つの頂点を接続、Ctrl+X で辺を溶解します (**{{MdRemove}}**)。**{{MdChamfer}}** のボックスには **{{MdSegmente}}** と **{{MrProfilMax}}** (Blender: **{{MrProfilBlender}}**) があり、0.5 で辺を円く、0.25 でまっすぐ面取りします。セグメント 1 つが四角形 1 列です。その他の値では点が曲線上に等間隔で並びます。面取りした辺が 3 本以上集まる角は、Blender のベベルと同じく四角形の格子で埋まります (セグメント数が奇数なら中央に小さなポリゴン)。プロファイル 1 では角は四角いまま (元の頂点が残ります)、4 本の辺を持つ頂点で終わる 1 本だけの面取り辺の端は三角形のファンになり、角の格子は隣の面からテクスチャ座標を取ります。いずれも Blender と同じです。',
        'En modo Blender valen además las teclas del modo edición de Blender: Alt+clic selecciona el bucle de aristas bajo el ratón, Ctrl+Alt+clic el anillo de aristas (en polígonos Alt+clic selecciona el bucle de caras), Ctrl+L amplía la selección a todo lo conectado (**{{MrVerbunden}}**), J une dos vértices y Ctrl+X disuelve aristas (**{{MdRemove}}**). **{{MdChamfer}}** tiene en su caja **{{MdSegmente}}** y **{{MrProfilMax}}** (Blender: **{{MrProfilBlender}}**): 0,5 redondea la arista en círculo, 0,25 la corta recta; cada segmento es una fila de cuadriláteros, otros valores reparten los puntos a pasos iguales sobre la curva. Donde se juntan tres o más aristas biseladas, una rejilla de cuadriláteros rellena la esquina como el bisel de Blender (con un número impar de segmentos queda un pequeño polígono en el centro). Con perfil 1 la esquina queda en ángulo recto (su punto antiguo se mantiene), el extremo de una sola arista biselada en un vértice con cuatro aristas se convierte en un abanico de triángulos y la rejilla de la esquina toma sus coordenadas de textura de las caras vecinas, todo como en Blender.',
        'V režimu Blender platí navíc klávesy režimu úprav Blenderu: Alt+klepnutí vybere smyčku hran pod myší, Ctrl+Alt+klepnutí prstenec hran (na polygonech vybere Alt+klepnutí smyčku ploch), Ctrl+L rozšíří výběr na vše propojené (**{{MrVerbunden}}**), J spojí dva vrcholy a Ctrl+X rozpustí hrany (**{{MdRemove}}**). **{{MdChamfer}}** má v okénku **{{MdSegmente}}** a **{{MrProfilMax}}** (Blender: **{{MrProfilBlender}}**): 0,5 zaoblí hranu do kruhu, 0,25 ji seřízne rovně; každý segment je jedna řada čtyřúhelníků, jiné hodnoty rozloží body po křivce ve stejných krocích. Kde se setkají tři a více zkosených hran, vyplní roh mřížka čtyřúhelníků jako u zkosení v Blenderu (při lichém počtu segmentů je uprostřed malý mnohoúhelník). S profilem 1 zůstane roh hranatý (jeho původní bod zůstane), konec jediné zkosené hrany ve vrcholu se čtyřmi hranami se stane vějířem trojúhelníků a mřížka rohu převezme texturové souřadnice ze sousedních ploch, vše jako v Blenderu.'),
    "titel": ("Model tab: modelling and props for the map", "Model-Tab: Modellieren und Requisiten für die Karte", "模型选项卡: 建模与地图道具",
              "モデルタブ: モデリングとマップ用小道具", "Pestaña Model: modelado y accesorios para el mapa", "Karta Model: modelování a rekvizity pro mapu"),
    "kurz": (
        "Build and change the surfaces of a .glm or .md3 with polygon tools like 3ds Max Editable Poly or Blender's Edit Mode - and put a prop such as a chair "
        "or a crate into the open map with one click.",
        "Baue und ändere die Flächen einer .glm oder .md3 mit Polygon-Werkzeugen wie in 3ds Max (Editable Poly) oder im Edit Mode von Blender - und setze ein "
        "Requisit wie einen Stuhl oder eine Kiste mit einem Klick in die offene Karte.",
        "像 3ds Max 的可编辑多边形或 Blender 的编辑模式一样, 用多边形工具构建和修改 .glm 或 .md3 的面 - 并一键把椅子或箱子等道具放入打开的地图。",
        "3ds Max の編集可能ポリゴンや Blender の編集モードのようなポリゴンツールで .glm や .md3 の面を作り変え、椅子や箱などの小道具をワンクリックで開いているマップに置きます。",
        "Construye y cambia las superficies de un .glm o .md3 con herramientas de polígonos como Editable Poly de 3ds Max o el modo edición de Blender, y pon un "
        "accesorio como una silla o una caja en el mapa abierto con un clic.",
        "Stavte a měňte plochy souboru .glm nebo .md3 polygonovými nástroji jako Editable Poly v 3ds Max nebo režim úprav v Blenderu - a vložte rekvizitu, "
        "třeba židli nebo bednu, jedním kliknutím do otevřené mapy."),
    "h_wo": ("The Modeling area", "Der Bereich Modellieren", "建模区域", "モデリング領域", "El área Modelado", "Oblast Modelování"),
    "wo": (
        "Choose the area **{{MbBereichModellieren}}** at the top of the model tab. The keys 1 to 5 (Blender mode 1 to 3) choose the level: **{{MdEbenePunkt}}**, "
        "**{{MdEbeneKante}}**, **{{MdEbeneRand}}**, **{{MdEbeneVieleck}}** and **{{MdEbeneElement}}**; each level keeps its own selection, as in 3ds Max. A click "
        "selects, Ctrl adds and Alt removes (Blender: Shift adds or removes), a drag selects everything it crosses. The view draws the polygons over the model - "
        "two triangles of the file that lie flat together are one quad, and **{{MdToQuads}}** joins them. W/E/R (Blender G/R/S) move, turn and scale the "
        "selection. The rollouts **{{MdRolloutMax}}** (Blender: **{{MdRolloutBlender}}**) list the tools of the level; the same tools are in the menu "
        "**{{MdMenue}}**, in the right-click of the view and in the search (X, F3 in Blender mode), and every tooltip names its key (3ds Max mode: the default keys of Autodesk's table, e.g. Shift+E Extrude, Ctrl+Shift+B Bevel, Ctrl+Shift+C Chamfer, Alt+L/Alt+R Loop/Ring) or, for a tool without a key, its way through the menu. Without a model, "
        "**{{MdNeuesRequisit}}** starts an empty .md3 with the texture currently chosen in the map editor.",
        "Wähle oben im Model-Tab den Bereich **{{MbBereichModellieren}}**. Die Tasten 1 bis 5 (Blender-Modus 1 bis 3) wählen die Ebene: **{{MdEbenePunkt}}**, "
        "**{{MdEbeneKante}}**, **{{MdEbeneRand}}**, **{{MdEbeneVieleck}}** und **{{MdEbeneElement}}**; jede Ebene behält ihre Auswahl wie in 3ds Max. Ein Klick "
        "wählt, Strg fügt hinzu, Alt nimmt weg (Blender: Umschalt fügt hinzu oder nimmt weg), ein Rahmen wählt alles, was er kreuzt. Die Ansicht zeichnet die "
        "Polygone über das Modell - zwei Dreiecke der Datei, die eben nebeneinander liegen, sind ein Viereck, **{{MdToQuads}}** fügt sie zusammen. W/E/R "
        "(Blender G/R/S) verschieben, drehen und skalieren die Auswahl. Die Rollouts **{{MdRolloutMax}}** (Blender: **{{MdRolloutBlender}}**) zeigen die "
        "Werkzeuge der Ebene; dieselben stehen im Menü **{{MdMenue}}**, im Rechtsklick der Ansicht und in der Suche (X, im Blender-Modus F3), und jeder Tooltip "
        "nennt seine Taste (3ds-Max-Modus: die Vorgaben der Autodesk-Tabelle, etwa Umschalt+E Extrude, Strg+Umschalt+B Bevel, Strg+Umschalt+C Chamfer, Alt+L/Alt+R Schleife/Ring) oder, bei einem Werkzeug ohne Taste, den Weg über das Menü. Ohne Modell beginnt **{{MdNeuesRequisit}}** eine leere .md3 mit der Textur, die im Karten-Editor gerade gewählt ist.",
        "在模型选项卡顶部选择 **{{MbBereichModellieren}}** 区域。按键 1 到 5 (Blender 模式 1 到 3) 选择层级: **{{MdEbenePunkt}}**、**{{MdEbeneKante}}**、**{{MdEbeneRand}}**、"
        "**{{MdEbeneVieleck}}** 和 **{{MdEbeneElement}}**; 与 3ds Max 一样, 每个层级保留自己的选择。单击选择, Ctrl 添加, Alt 移除 (Blender: Shift 添加或移除), 拖框选择所有经过的元素。"
        "视图在模型上绘制多边形 - 文件中平铺相邻的两个三角形就是一个四边形, **{{MdToQuads}}** 会合并它们。W/E/R (Blender G/R/S) 移动、旋转和缩放选择。卷展栏 **{{MdRolloutMax}}** "
        "(Blender: **{{MdRolloutBlender}}**) 列出该层级的工具; 同样的工具在菜单 **{{MdMenue}}**、视图右键菜单和搜索 (X, Blender 模式为 F3) 中, 每个提示都写有按键 (3ds Max 模式: Autodesk 表中的默认按键, 例如 Shift+E 挤出、Ctrl+Shift+B 倒角、Ctrl+Shift+C 切角、Alt+L/Alt+R 循环/环形), 没有按键的工具则写出菜单中的路径。没有模型时, "
        "**{{MdNeuesRequisit}}** 会以地图编辑器当前选择的纹理新建一个空 .md3。",
        "モデルタブ上部で領域 **{{MbBereichModellieren}}** を選びます。キー 1〜5 (Blender 風は 1〜3) でレベルを選択: **{{MdEbenePunkt}}**、**{{MdEbeneKante}}**、**{{MdEbeneRand}}**、"
        "**{{MdEbeneVieleck}}**、**{{MdEbeneElement}}**。3ds Max と同じく各レベルが自分の選択を保ちます。クリックで選択、Ctrl で追加、Alt で除外 (Blender: Shift で追加/除外)、"
        "ドラッグで交差する全てを選択。ビューはモデルの上にポリゴンを描きます - ファイルの平らに並ぶ 2 つの三角形は 1 つの四角形で、**{{MdToQuads}}** で結合します。"
        "W/E/R (Blender G/R/S) で選択を移動・回転・拡縮。ロールアウト **{{MdRolloutMax}}** (Blender: **{{MdRolloutBlender}}**) にレベルのツールがあり、同じものがメニュー "
        "**{{MdMenue}}**、ビューの右クリック、検索 (X、Blender 風は F3) にもあり、ツールチップにキーが書かれています (3ds Max 風: Autodesk の表の既定キー、例: Shift+E 押し出し、Ctrl+Shift+B ベベル、Ctrl+Shift+C 面取り、Alt+L/Alt+R ループ/リング)。キーのないツールはメニューでの場所が書かれています。モデルがなければ **{{MdNeuesRequisit}}** が、"
        "マップエディターで選択中のテクスチャで空の .md3 を始めます。",
        "Elige arriba en la pestaña Model el área **{{MbBereichModellieren}}**. Las teclas 1 a 5 (modo Blender 1 a 3) eligen el nivel: **{{MdEbenePunkt}}**, "
        "**{{MdEbeneKante}}**, **{{MdEbeneRand}}**, **{{MdEbeneVieleck}}** y **{{MdEbeneElement}}**; cada nivel guarda su selección como en 3ds Max. Un clic "
        "selecciona, Ctrl añade y Alt quita (Blender: Mayús añade o quita), un recuadro selecciona todo lo que cruza. La vista dibuja los polígonos sobre el modelo: "
        "dos triángulos del archivo que están planos uno junto a otro son un cuadrilátero, y **{{MdToQuads}}** los une. W/E/R (Blender G/R/S) mueven, giran y "
        "escalan la selección. Los paneles **{{MdRolloutMax}}** (Blender: **{{MdRolloutBlender}}**) muestran las herramientas del nivel; las mismas están en el "
        "menú **{{MdMenue}}**, en el clic derecho de la vista y en la búsqueda (X, F3 en modo Blender), y cada ayuda nombra su tecla (modo 3ds Max: las teclas por defecto de la tabla de Autodesk, p. ej. Mayús+E Extruir, Ctrl+Mayús+B Bisel, Ctrl+Mayús+C Chaflán, Alt+L/Alt+R Bucle/Anillo) o, si la herramienta no tiene tecla, su camino por el menú. Sin modelo, "
        "**{{MdNeuesRequisit}}** empieza un .md3 vacío con la textura elegida ahora en el editor de mapas.",
        "Nahoře v kartě Model zvolte oblast **{{MbBereichModellieren}}**. Klávesy 1 až 5 (režim Blender 1 až 3) volí úroveň: **{{MdEbenePunkt}}**, **{{MdEbeneKante}}**, "
        "**{{MdEbeneRand}}**, **{{MdEbeneVieleck}}** a **{{MdEbeneElement}}**; každá úroveň si drží svůj výběr jako v 3ds Max. Kliknutí vybírá, Ctrl přidává, "
        "Alt ubírá (Blender: Shift přidá nebo ubere), tažení vybere vše, co protne. Pohled kreslí polygony přes model - dva trojúhelníky souboru, které leží rovně "
        "vedle sebe, jsou jeden čtyřúhelník a **{{MdToQuads}}** je spojí. W/E/R (Blender G/R/S) posouvají, otáčejí a mění měřítko výběru. Rollouty "
        "**{{MdRolloutMax}}** (Blender: **{{MdRolloutBlender}}**) ukazují nástroje úrovně; stejné jsou v menu **{{MdMenue}}**, v pravém kliknutí pohledu a ve "
        "vyhledávání (X, v režimu Blender F3) a každá nápověda uvádí klávesu (režim 3ds Max: výchozí klávesy z tabulky Autodesk, např. Shift+E vysunutí, Ctrl+Shift+B zkosení, Ctrl+Shift+C sražení, Alt+L/Alt+R smyčka/prstenec), u nástroje bez klávesy cestu v nabídce. Bez modelu začne **{{MdNeuesRequisit}}** prázdné .md3 s texturou, která je právě "
        "zvolena v editoru map."),
    "bild_modell": ("A box with an extruded and inset top; the tool caddy of Extrude is open in the view.",
                    "Ein Kasten mit extrudiertem und eingefügtem Deckel; das Werkzeugkästchen von Extrudieren ist in der Ansicht offen.",
                    "一个顶面被挤出并插入的盒子; 视图中打开了挤出的工具框。", "上面を押し出してインセットしたボックス。ビューに押し出しのツールボックスが開いています。",
                    "Una caja con la tapa extruida e insertada; la caja de herramientas de Extruir está abierta en la vista.",
                    "Kvádr s vysunutým a vsazeným víkem; v pohledu je otevřena schránka nástroje Vysunout."),
    "h_werkzeuge": ("Tools and the tool caddy", "Werkzeuge und das Werkzeugkästchen", "工具与工具框", "ツールとツールボックス", "Herramientas y la caja de herramientas",
                    "Nástroje a schránka nástroje"),
    "werkzeuge": (
        "A tool with values opens a small caddy in the top left of the view: the result shows at once and changes live while you drag a value; **{{MdUebernehmen}}** "
        "(Enter) keeps it, **+** keeps it and runs the same tool again on the result, **{{MdAbbrechen}}** (Esc) puts everything back. Each tool is one undo step. "
        "**{{MdExtrude}}** lifts polygons (group, local normal or by polygon) or grows a strip from a border; **{{MdInset}}** puts a smaller copy inside, every "
        "edge the same distance in; **{{MdBevel}}** extrudes and scales the outline; **{{MdChamfer}}** replaces edges by strips or cuts vertices off; "
        "**{{MdConnect}}** and **{{MdLoopCut}}** add edges across polygons; **{{MdCut}}** cuts by clicking edges and vertices one after another; **{{MdWeld}}**, "
        "**{{MdTargetWeld}}** and **{{MdMerge}}** join vertices; **{{MdEben}}** (Alt+M) moves the selection onto one plane (average normal, through its centre); **{{MdCollapse}}** (Ctrl+Alt+C) turns each connected group into one vertex; **{{MdSlice}}** cuts along a plane across an axis and **{{MdQuickSlice}}** (Ctrl+Shift+Q) along the line of two clicks in a view; **{{MdHinge}}** swings polygons about one of their border edges; **{{MdBridge}}** joins two borders or two polygons with a band of quads; **{{MdRemove}}** "
        "dissolves edges and vertices, **{{MdDelete}}** leaves a hole; **{{MdDetach}}** splits polygons off or into a new surface with its own texture; "
        "**{{MdSymmetrize}}** mirrors one half; **{{MdCreate}}** adds a box, plane, cylinder, sphere, cone, tube or torus with UVs. A tool that cannot work says "
        "why (for example: an edge would get a third polygon) and changes nothing. New vertices take the UVs of their polygon and blend the bone weights of their "
        "neighbours (at most four, summing to 1).",
        "Ein Werkzeug mit Werten öffnet oben links in der Ansicht ein kleines Kästchen: das Ergebnis ist sofort zu sehen und ändert sich live, während du einen "
        "Wert ziehst; **{{MdUebernehmen}}** (Enter) behält es, **+** behält es und startet dasselbe Werkzeug noch einmal auf dem Ergebnis, **{{MdAbbrechen}}** "
        "(Esc) stellt alles zurück. Jedes Werkzeug ist ein Rückgängig-Schritt. **{{MdExtrude}}** hebt Polygone an (Gruppe, lokale Normale oder je Polygon) oder "
        "lässt einen Streifen am Rand wachsen; **{{MdInset}}** setzt eine kleinere Kopie hinein, jede Kante gleich weit; **{{MdBevel}}** extrudiert und ändert den "
        "Umriss; **{{MdChamfer}}** ersetzt Kanten durch Streifen oder schneidet Punkte ab; **{{MdConnect}}** und **{{MdLoopCut}}** legen Kanten quer durch "
        "Polygone; **{{MdCut}}** schneidet durch Klicks auf Kanten und Punkte nacheinander; **{{MdWeld}}**, **{{MdTargetWeld}}** und **{{MdMerge}}** legen Punkte "
        "zusammen; **{{MdEben}}** (Alt+M) schiebt die Auswahl in eine Ebene (gemittelte Normale, durch ihre Mitte); **{{MdCollapse}}** (Strg+Alt+C) macht jede zusammenhängende Gruppe zu einem Punkt; **{{MdSlice}}** schneidet an einer Ebene quer zu einer Achse, **{{MdQuickSlice}}** (Strg+Umschalt+Q) an der Linie zweier Klicks in einer Ansicht; **{{MdHinge}}** dreht Polygone um eine ihrer Randkanten; **{{MdBridge}}** verbindet zwei Ränder oder zwei Polygone mit einem Band aus Vierecken; **{{MdRemove}}** löst Kanten und Punkte auf, "
        "**{{MdDelete}}** lässt ein Loch; **{{MdDetach}}** trennt Polygone ab oder in eine neue Fläche mit eigener Textur; **{{MdSymmetrize}}** spiegelt eine "
        "Hälfte; **{{MdCreate}}** fügt Kasten, Ebene, Zylinder, Kugel, Kegel, Rohr oder Torus mit UV ein. Ein Werkzeug, das nicht arbeiten kann, sagt warum "
        "(zum Beispiel: eine Kante bekäme ein drittes Polygon) und ändert nichts. Neue Punkte nehmen die UV ihres Polygons und mischen die Knochen-Gewichte ihrer "
        "Nachbarn (höchstens vier, Summe 1).",
        "带数值的工具会在视图左上角打开一个小工具框: 结果立即显示, 拖动数值时实时变化; **{{MdUebernehmen}}** (Enter) 保留, **+** 保留并在结果上再次运行同一工具, **{{MdAbbrechen}}** "
        "(Esc) 恢复原状。每个工具是一个撤销步骤。**{{MdExtrude}}** 抬起多边形 (组、局部法线或按多边形) 或从边界长出一条带; **{{MdInset}}** 在内部放入较小副本, 每条边同样距离; "
        "**{{MdBevel}}** 挤出并缩放轮廓; **{{MdChamfer}}** 用带替换边或切掉顶点; **{{MdConnect}}** 和 **{{MdLoopCut}}** 在多边形中加入边; **{{MdCut}}** 依次点击边和顶点进行切割; "
        "**{{MdWeld}}**、**{{MdTargetWeld}}** 和 **{{MdMerge}}** 合并顶点; **{{MdEben}}** (Alt+M) 把选择移到同一平面 (平均法线, 经过其中心); **{{MdCollapse}}** (Ctrl+Alt+C) 把每个相连的组变为一个顶点; **{{MdSlice}}** 沿垂直于某轴的平面切开, **{{MdQuickSlice}}** (Ctrl+Shift+Q) 沿视图中两次点击的线切开; **{{MdHinge}}** 让多边形绕其一条边界边旋转; **{{MdBridge}}** 用四边形带连接两个边界或两个多边形; **{{MdRemove}}** 溶解边和顶点, **{{MdDelete}}** "
        "留下孔洞; **{{MdDetach}}** 分离多边形或放入有独立纹理的新表面; **{{MdSymmetrize}}** 镜像一半; **{{MdCreate}}** 添加带 UV 的盒子、平面、圆柱、球、圆锥、管或圆环。"
        "无法执行的工具会说明原因 (例如: 某条边会出现第三个多边形), 且不做任何更改。新顶点采用其多边形的 UV, 并混合相邻顶点的骨骼权重 (最多四个, 总和为 1)。",
        "値のあるツールはビューの左上に小さなボックスを開きます: 結果はすぐに見え、値をドラッグするとライブで変わります。**{{MdUebernehmen}}** (Enter) で確定、**+** は確定して結果に"
        "同じツールをもう一度、**{{MdAbbrechen}}** (Esc) で元通り。各ツールは 1 回の元に戻す単位です。**{{MdExtrude}}** はポリゴンを持ち上げ (グループ、ローカル法線、ポリゴンごと)、"
        "または境界から帯を伸ばします。**{{MdInset}}** は内側に小さなコピーを作り、各エッジを同じ距離だけ内へ。**{{MdBevel}}** は押し出して輪郭を変えます。**{{MdChamfer}}** は"
        "エッジを帯に置き換えるか頂点を切り落とします。**{{MdConnect}}** と **{{MdLoopCut}}** はポリゴンを横切るエッジを追加。**{{MdCut}}** はエッジや頂点を順にクリックして切ります。"
        "**{{MdWeld}}**、**{{MdTargetWeld}}**、**{{MdMerge}}** は頂点を結合。**{{MdEben}}** (Alt+M) は選択を一つの平面に移します (平均法線、その中心を通る)。**{{MdCollapse}}** (Ctrl+Alt+C) はつながった各グループを一つの頂点にし、**{{MdSlice}}** は軸に垂直な平面で、**{{MdQuickSlice}}** (Ctrl+Shift+Q) はビューで 2 回クリックした線で切ります。**{{MdHinge}}** はポリゴンを境界エッジの一つを軸に回します。**{{MdBridge}}** は 2 つの境界か 2 つのポリゴンを四角形の帯でつなぎます。**{{MdRemove}}** はエッジや"
        "頂点を溶解し、**{{MdDelete}}** は穴を残します。**{{MdDetach}}** はポリゴンを切り離すか、別テクスチャの新しいサーフェスへ。**{{MdSymmetrize}}** は半分をミラー。"
        "**{{MdCreate}}** は UV 付きのボックス、平面、円柱、球、円錐、チューブ、トーラスを追加します。動作できないツールは理由を示し (例: エッジに 3 つ目のポリゴンが付く)、何も変えません。"
        "新しい頂点はポリゴンの UV を受け継ぎ、隣接頂点のボーンウェイトを混ぜます (最大 4、合計 1)。",
        "Una herramienta con valores abre una pequeña caja arriba a la izquierda de la vista: el resultado se ve enseguida y cambia en vivo mientras arrastras un "
        "valor; **{{MdUebernehmen}}** (Intro) lo conserva, **+** lo conserva y vuelve a aplicar la herramienta sobre el resultado, **{{MdAbbrechen}}** (Esc) lo "
        "deja todo como estaba. Cada herramienta es un paso de deshacer. **{{MdExtrude}}** eleva polígonos (grupo, normal local o por polígono) o hace crecer una "
        "franja desde un borde; **{{MdInset}}** pone dentro una copia más pequeña, cada arista a la misma distancia; **{{MdBevel}}** extruye y cambia el contorno; "
        "**{{MdChamfer}}** sustituye aristas por franjas o corta vértices; **{{MdConnect}}** y **{{MdLoopCut}}** añaden aristas a través de polígonos; **{{MdCut}}** "
        "corta haciendo clic en aristas y vértices uno tras otro; **{{MdWeld}}**, **{{MdTargetWeld}}** y **{{MdMerge}}** unen vértices; **{{MdEben}}** (Alt+M) lleva la selección a un plano (normal media, por su centro); **{{MdCollapse}}** (Ctrl+Alt+C) convierte cada grupo conectado en un vértice; **{{MdSlice}}** corta por un plano perpendicular a un eje y **{{MdQuickSlice}}** (Ctrl+Mayús+Q) por la línea de dos clics en una vista; **{{MdHinge}}** gira polígonos alrededor de una de sus aristas de borde; **{{MdBridge}}** une dos "
        "bordes o dos polígonos con una banda de cuadriláteros; **{{MdRemove}}** disuelve aristas y vértices, **{{MdDelete}}** deja un agujero; **{{MdDetach}}** "
        "separa polígonos o los pasa a una nueva superficie con su propia textura; **{{MdSymmetrize}}** refleja una mitad; **{{MdCreate}}** añade caja, plano, "
        "cilindro, esfera, cono, tubo o toro con UV. Una herramienta que no puede actuar dice por qué (por ejemplo: una arista tendría un tercer polígono) y no "
        "cambia nada. Los vértices nuevos toman las UV de su polígono y mezclan los pesos de hueso de sus vecinos (como máximo cuatro, suma 1).",
        "Nástroj s hodnotami otevře vlevo nahoře v pohledu malou schránku: výsledek je hned vidět a mění se živě, když táhnete hodnotu; **{{MdUebernehmen}}** "
        "(Enter) jej ponechá, **+** jej ponechá a spustí stejný nástroj znovu na výsledku, **{{MdAbbrechen}}** (Esc) vše vrátí. Každý nástroj je jeden krok zpět. "
        "**{{MdExtrude}}** zvedne polygony (skupina, lokální normála nebo po polygonech) nebo z okraje vytáhne pruh; **{{MdInset}}** vloží dovnitř menší kopii, "
        "každá hrana stejně daleko; **{{MdBevel}}** vysune a změní obrys; **{{MdChamfer}}** nahradí hrany pruhy nebo uřízne vrcholy; **{{MdConnect}}** a "
        "**{{MdLoopCut}}** přidají hrany přes polygony; **{{MdCut}}** řeže postupným klikáním na hrany a vrcholy; **{{MdWeld}}**, **{{MdTargetWeld}}** a "
        "**{{MdMerge}}** slučují vrcholy; **{{MdEben}}** (Alt+M) přesune výběr do jedné roviny (průměrná normála, přes jeho střed); **{{MdCollapse}}** (Ctrl+Alt+C) udělá z každé souvislé skupiny jeden vrchol; **{{MdSlice}}** řeže rovinou kolmou k ose a **{{MdQuickSlice}}** (Ctrl+Shift+Q) podél čáry ze dvou kliknutí v pohledu; **{{MdHinge}}** otočí polygony kolem jedné z jejich okrajových hran; **{{MdBridge}}** spojí dva okraje nebo dva polygony pásem čtyřúhelníků; **{{MdRemove}}** rozpustí hrany a vrcholy, "
        "**{{MdDelete}}** zanechá díru; **{{MdDetach}}** oddělí polygony nebo je přesune do nové plochy s vlastní texturou; **{{MdSymmetrize}}** zrcadlí polovinu; "
        "**{{MdCreate}}** přidá kvádr, rovinu, válec, kouli, kužel, trubku nebo torus s UV. Nástroj, který nemůže pracovat, řekne proč (například: hrana by "
        "dostala třetí polygon) a nic nezmění. Nové vrcholy převezmou UV svého polygonu a smíchají váhy kostí sousedů (nejvýše čtyři, součet 1)."),
    "h_grenzen": ("JKA limits and LODs", "JKA-Grenzen und LODs", "JKA 限制与 LOD", "JKA の上限と LOD", "Límites de JKA y LOD", "Limity JKA a LOD"),
    "grenzen": (
        "The rollout **{{MdGrGrenzen}}** counts each surface as it would be written: fewer than 1000 vertices (SHADER_MAX_VERTEXES), at most 32 bones per surface "
        "and four weights per vertex, names and textures up to 63 characters (MAX_QPATH), and each tag exactly one triangle; a line turns orange as soon as a "
        "limit is crossed, and the caddy shows it during the preview. Too many vertices: split the surface with **{{MdDetach}}**. A .glm keeps its other LOD "
        "levels; **{{MdLods}}** gives all levels the mesh of the drawn one.",
        "Das Rollout **{{MdGrGrenzen}}** zählt jede Fläche so, wie sie geschrieben würde: weniger als 1000 Ecken (SHADER_MAX_VERTEXES), höchstens 32 Knochen je "
        "Fläche und vier Gewichte je Ecke, Namen und Texturen bis 63 Zeichen (MAX_QPATH), jeder Tag genau ein Dreieck; eine Zeile wird orange, sobald eine "
        "Grenze überschritten ist, und das Kästchen zeigt es schon während der Vorschau. Zu viele Ecken: die Fläche mit **{{MdDetach}}** teilen. Eine .glm behält "
        "ihre anderen LOD-Stufen; **{{MdLods}}** gibt allen Stufen das Netz der gezeichneten.",
        "卷展栏 **{{MdGrGrenzen}}** 按写入文件的方式统计每个表面: 少于 1000 个顶点 (SHADER_MAX_VERTEXES)、每表面最多 32 根骨骼、每顶点四个权重、名称和纹理最多 63 个字符 "
        "(MAX_QPATH), 每个标签正好一个三角形; 一旦超限该行变为橙色, 工具框在预览时就会显示。顶点过多: 用 **{{MdDetach}}** 拆分表面。.glm 保留其他 LOD 层级; **{{MdLods}}** "
        "让所有层级使用当前绘制层级的网格。",
        "ロールアウト **{{MdGrGrenzen}}** は各サーフェスを書き出す形で数えます: 頂点 1000 未満 (SHADER_MAX_VERTEXES)、サーフェスごとのボーン最大 32、頂点ごとのウェイト 4、"
        "名前とテクスチャは 63 文字まで (MAX_QPATH)、タグはちょうど三角形 1 つ。上限を超えると行がオレンジになり、プレビュー中もボックスに表示されます。頂点が多すぎるときは "
        "**{{MdDetach}}** でサーフェスを分けます。.glm の他の LOD レベルはそのまま。**{{MdLods}}** で全レベルを表示中のメッシュにします。",
        "El panel **{{MdGrGrenzen}}** cuenta cada superficie tal como se escribiría: menos de 1000 vértices (SHADER_MAX_VERTEXES), como máximo 32 huesos por "
        "superficie y cuatro pesos por vértice, nombres y texturas de hasta 63 caracteres (MAX_QPATH) y cada etiqueta exactamente un triángulo; una línea se vuelve "
        "naranja en cuanto se supera un límite, y la caja lo muestra ya durante la vista previa. Demasiados vértices: divide la superficie con **{{MdDetach}}**. Un "
        ".glm conserva sus otros niveles LOD; **{{MdLods}}** da a todos los niveles la malla del nivel mostrado.",
        "Rollout **{{MdGrGrenzen}}** počítá každou plochu tak, jak by se zapsala: méně než 1000 vrcholů (SHADER_MAX_VERTEXES), nejvýše 32 kostí na plochu a čtyři "
        "váhy na vrchol, názvy a textury do 63 znaků (MAX_QPATH) a každý tag přesně jeden trojúhelník; řádek zoranžoví, jakmile je limit překročen, a schránka to "
        "ukáže už během náhledu. Příliš mnoho vrcholů: rozdělte plochu pomocí **{{MdDetach}}**. Soubor .glm si ponechá ostatní úrovně LOD; **{{MdLods}}** dá všem "
        "úrovním síť zobrazené úrovně."),
    "h_karte": ("A prop into the map with one click", "Ein Requisit mit einem Klick in die Karte", "一键把道具放入地图", "ワンクリックで小道具をマップへ",
                "Un accesorio en el mapa con un clic", "Rekvizita do mapy jedním kliknutím"),
    "karte": (
        "Give the prop a name in **{{MdGrRequisit}}** and click **{{MdInDieKarte}}**: JK Studio writes it as .md3 to models/map_objects/<map>/<name>.md3 in its "
        "own import folder (never the game folder), switches to the map tab and places it as misc_model on the floor in the middle of the camera; there you move, "
        "turn and scale it like any entity. The import folder is in the search path of the editor and of q3map2 and goes into the .pk3 of the map when you test "
        "or package it, so the prop is baked into the map with lightmaps like every misc_model. To change it, click the edit button beside the model key of the "
        "entity (or start from the model tab again) - after Ctrl+S the map editor reloads every copy and they keep their position, angles and scale.",
        "Gib dem Requisit in **{{MdGrRequisit}}** einen Namen und klicke **{{MdInDieKarte}}**: JK Studio schreibt es als .md3 nach models/map_objects/<karte>/"
        "<name>.md3 in seinen eigenen Import-Ordner (nie in den Spielordner), wechselt in den Map-Tab und setzt es als misc_model auf den Boden in der Mitte der "
        "Kamera; dort verschiebst, drehst und skalierst du es wie jede Entity. Der Import-Ordner steht im Suchpfad des Editors und von q3map2 und kommt beim Testen "
        "oder Packen mit in die .pk3 der Karte, das Requisit wird also wie jedes misc_model mit Lightmaps in die Karte gebacken. Zum Ändern den Bearbeiten-Knopf "
        "neben dem Schlüssel model der Entity klicken (oder wieder im Model-Tab anfangen) - nach Strg+S lädt der Karten-Editor jedes Exemplar neu, und sie "
        "behalten Lage, Winkel und Größe.",
        "在 **{{MdGrRequisit}}** 中为道具命名并点击 **{{MdInDieKarte}}**: JK Studio 将其作为 .md3 写入自己的导入文件夹中的 models/map_objects/<地图>/<名称>.md3 (绝不写入游戏文件夹), "
        "切换到地图选项卡, 并作为 misc_model 放在摄像机中心的地面上; 在那里像任何实体一样移动、旋转和缩放。导入文件夹位于编辑器和 q3map2 的搜索路径中, 测试或打包时会进入地图的 .pk3, "
        "因此道具像每个 misc_model 一样带光照贴图烘焙进地图。要修改它, 点击实体 model 键旁的编辑按钮 (或再次从模型选项卡开始) - Ctrl+S 之后地图编辑器会重新加载所有副本, 它们保持位置、角度和缩放。",
        "**{{MdGrRequisit}}** で小道具に名前を付け、**{{MdInDieKarte}}** をクリック: JK Studio は自身のインポートフォルダー (ゲームフォルダーには決して書かない) の "
        "models/map_objects/<マップ>/<名前>.md3 に .md3 として書き、マップタブに切り替えてカメラ中央の床に misc_model として置きます。そこで他のエンティティと同じく移動・回転・拡縮できます。"
        "インポートフォルダーはエディターと q3map2 の検索パスにあり、テストやパッケージ時にマップの .pk3 に入るので、小道具は他の misc_model と同じくライトマップ付きでマップに焼き込まれます。"
        "変更するには、エンティティの model キーの横の編集ボタンをクリック (またはモデルタブから再開) - Ctrl+S の後、マップエディターが全コピーを読み直し、位置・角度・スケールは保たれます。",
        "Pon un nombre al accesorio en **{{MdGrRequisit}}** y haz clic en **{{MdInDieKarte}}**: JK Studio lo escribe como .md3 en models/map_objects/<mapa>/"
        "<nombre>.md3 dentro de su propia carpeta de importación (nunca en la del juego), pasa a la pestaña Map y lo coloca como misc_model en el suelo en el "
        "centro de la cámara; allí lo mueves, giras y escalas como cualquier entidad. La carpeta de importación está en la ruta de búsqueda del editor y de "
        "q3map2 y entra en el .pk3 del mapa al probarlo o empaquetarlo, así que el accesorio se integra en el mapa con lightmaps como cualquier misc_model. Para "
        "cambiarlo, pulsa el botón de editar junto a la clave model de la entidad (o empieza otra vez en la pestaña Model): tras Ctrl+S el editor de mapas "
        "recarga cada copia y conservan posición, ángulos y escala.",
        "Pojmenujte rekvizitu v **{{MdGrRequisit}}** a klikněte na **{{MdInDieKarte}}**: JK Studio ji zapíše jako .md3 do models/map_objects/<mapa>/<název>.md3 "
        "ve své vlastní složce importu (nikdy do složky hry), přepne na kartu Map a vloží ji jako misc_model na podlahu do středu kamery; tam ji posouváte, otáčíte "
        "a měníte měřítko jako každou entitu. Složka importu je v cestě editoru i q3map2 a při testu nebo zabalení jde do .pk3 mapy, takže se rekvizita zapeče do "
        "mapy s lightmapami jako každý misc_model. Pro změnu klikněte na tlačítko úprav vedle klíče model entity (nebo začněte znovu v kartě Model) - po Ctrl+S "
        "editor map znovu načte každou kopii a ty si ponechají polohu, úhly a měřítko."),
    "bild_karte": ("The prop from the model tab as misc_model in the map editor.", "Das Requisit aus dem Model-Tab als misc_model im Karten-Editor.",
                   "来自模型选项卡的道具在地图编辑器中作为 misc_model。", "モデルタブの小道具がマップエディターに misc_model として。",
                   "El accesorio de la pestaña Model como misc_model en el editor de mapas.", "Rekvizita z karty Model jako misc_model v editoru map."),
    "h_lizenz": ("Origin and licence", "Herkunft und Lizenz", "来源与许可", "由来とライセンス", "Origen y licencia", "Původ a licence"),
    "lizenz": (
        "The modelling tools are derived from WhiteoutLib and WhiteoutFlakes (BSD 3-Clause licence with AI-derived works notice, Copyright (c) 2026 Fernando "
        "Sahmkow). They work only with Jedi Academy data: the surfaces of .glm and .md3. The licence text is in licenses/, THIRD-PARTY.md and Help > About. Chamfer with more than one segment is ported from Blender's Bevel (bmesh_bevel.cc, GPL-2.0-or-later, "
        "Copyright Blender Authors); these parts are used under the GPL v2 like JK Studio.",
        "Die Modellier-Werkzeuge sind aus WhiteoutLib und WhiteoutFlakes abgeleitet (BSD-3-Clause-Lizenz mit Hinweis zu KI-abgeleiteten Werken, Copyright (c) "
        "2026 Fernando Sahmkow). Sie arbeiten nur mit Daten von Jedi Academy: den Flächen von .glm und .md3. Der Lizenztext steht in licenses/, THIRD-PARTY.md und "
        "Hilfe > Über. Chamfer mit mehr als einem Segment ist aus Blenders Bevel portiert (bmesh_bevel.cc, GPL-2.0-or-later, Copyright "
        "Blender Authors); diese Teile stehen wie JK Studio unter der GPL v2.",
        "建模工具源自 WhiteoutLib 和 WhiteoutFlakes (BSD 3-Clause 许可, 附 AI 衍生作品声明, Copyright (c) 2026 Fernando Sahmkow)。它们只处理 Jedi Academy 的数据: .glm 和 .md3 的面。"
        "许可文本见 licenses/、THIRD-PARTY.md 和 帮助 > 关于。多于一个分段的倒角移植自 Blender 的倒角 (bmesh_bevel.cc, GPL-2.0-or-later, "
        "Copyright Blender Authors); 这些部分与 JK Studio 一样按 GPL v2 使用。",
        "モデリングツールは WhiteoutLib と WhiteoutFlakes に由来します (BSD 3-Clause ライセンス、AI 派生物の注記付き、Copyright (c) 2026 Fernando Sahmkow)。Jedi Academy のデータ "
        "(.glm と .md3 のサーフェス) だけを扱います。ライセンス文は licenses/、THIRD-PARTY.md、ヘルプ > バージョン情報 にあります。"
        "2 セグメント以上の面取りは Blender のベベルから移植しています (bmesh_bevel.cc、GPL-2.0-or-later、Copyright Blender Authors)。"
        "これらの部分は JK Studio と同じく GPL v2 で使用します。",
        "Las herramientas de modelado derivan de WhiteoutLib y WhiteoutFlakes (licencia BSD 3-Clause con aviso de obras derivadas por IA, Copyright (c) 2026 "
        "Fernando Sahmkow). Solo trabajan con datos de Jedi Academy: las superficies de .glm y .md3. El texto de la licencia está en licenses/, THIRD-PARTY.md y "
        "Ayuda > Acerca de. El chaflán con más de un segmento está portado del Bevel de Blender (bmesh_bevel.cc, GPL-2.0-or-later, "
        "Copyright Blender Authors); esas partes se usan bajo la GPL v2 como JK Studio.",
        "Modelovací nástroje jsou odvozeny z WhiteoutLib a WhiteoutFlakes (licence BSD 3-Clause s upozorněním na díla odvozená AI, Copyright (c) 2026 Fernando "
        "Sahmkow). Pracují jen s daty Jedi Academy: plochami .glm a .md3. Text licence je v licenses/, THIRD-PARTY.md a Nápověda > O programu. "
        "Zkosení s více než jedním segmentem je převzato z Bevel v Blenderu (bmesh_bevel.cc, GPL-2.0-or-later, Copyright Blender Authors); "
        "tyto části se používají pod GPL v2 jako JK Studio."),
}


def _t(schluessel):
    from inhalt_basis import T
    z = X[schluessel]
    return T(z[0], z[1], z[2], z[3])


def seite():
    """Die Seite modell-modellieren (Gruppe Missionen, nach modell-uv)."""
    return {
        "id": "modell-modellieren",
        "gruppe": "mission",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("h2", "wo", _t("h_wo")),
            ("p", _t("wo")),
            ("img", "h_md_modell", _t("bild_modell"), "breit"),
            ("h2", "werkzeuge", _t("h_werkzeuge")),
            ("p", _t("werkzeuge")),
            ("p", _t("mr_blender")),
            ("h2", "grenzen", _t("h_grenzen")),
            ("p", _t("grenzen")),
            ("h2", "karte", _t("h_karte")),
            ("p", _t("karte")),
            ("img", "h_md_karte", _t("bild_karte"), "breit"),
            ("h2", "lizenz", _t("h_lizenz")),
            ("p", _t("lizenz")),
        ],
    }


ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}
