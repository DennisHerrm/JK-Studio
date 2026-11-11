"""AP3 (MBENTWURF Teil B): das rechte Feld des Model-Tabs - Befehlsfeld mit Reitern (wie 3ds Max) bzw. Eigenschaften-Editor mit
senkrechter Reiterspalte (wie Blender), Gruppen in einer Form, Breite, die Reihe der Bereiche ueber dem Tab.

Haengt Absaetze und Bilder an die Seite modell-ansehen (inhalt_mb1a1.py), direkt hinter die Ueberschrift "Befehlsfeld". Jeder
Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_ap2.py.
"""

X = {
    "feld": (
        "**The panel on the right follows the control mode.** **{{MbWieMax}}**: the **{{MbBefehlsfeld}}** with the tabs "
        "**{{MfErstellen}}**, **{{MfAendern}}**, **{{MfHierarchie}}**, **{{MfBewegung}}**, **{{MfAnzeige}}** and **{{MfDienste}}** "
        "and groups with flat heading bars. **{{MbWieBlender}}**: **{{MfEigenschaften}}** with a vertical column of symbols "
        "(**{{MfWerkzeug}}**, **{{MfObjekt}}**, **{{MfModifier}}**, **{{MfDaten}}**, **{{MfKnochen}}**, **{{MfConstraints}}**, "
        "**{{MfMaterial}}** - the name stands in the tooltip) and rounded panels. Every area of the tab chooses its tab - "
        "**{{MbBereichAnsehen}}** opens **{{MfAnzeige}}** or **{{MfObjekt}}**, **{{MbBereichAnimation}}** opens **{{MfBewegung}}** or "
        "**{{MfKnochen}}** - and shows all its groups there, one below the other. The other tabs show their part: in "
        "**{{MbBereichAnsehen}}**, **{{MfHierarchie}}** holds the attached models and **{{MfDienste}}** the check for the game. A tab that "
        "has nothing in the current area is greyed out.",
        "**Das Feld rechts folgt dem Bedienmodus.** **{{MbWieMax}}**: das **{{MbBefehlsfeld}}** mit den Reitern **{{MfErstellen}}**, "
        "**{{MfAendern}}**, **{{MfHierarchie}}**, **{{MfBewegung}}**, **{{MfAnzeige}}** und **{{MfDienste}}** und Gruppen mit flachen "
        "Überschriftsbalken. **{{MbWieBlender}}**: **{{MfEigenschaften}}** mit einer senkrechten Spalte aus Symbolen (**{{MfWerkzeug}}**, "
        "**{{MfObjekt}}**, **{{MfModifier}}**, **{{MfDaten}}**, **{{MfKnochen}}**, **{{MfConstraints}}**, **{{MfMaterial}}** - der Name "
        "steht im Tooltip) und abgerundeten Kästen. Jeder Bereich des Tabs wählt seinen Reiter - **{{MbBereichAnsehen}}** öffnet "
        "**{{MfAnzeige}}** bzw. **{{MfObjekt}}**, **{{MbBereichAnimation}}** öffnet **{{MfBewegung}}** bzw. **{{MfKnochen}}** - und zeigt "
        "dort alle seine Gruppen untereinander. Die anderen Reiter zeigen ihren Teil: in **{{MbBereichAnsehen}}** stehen unter "
        "**{{MfHierarchie}}** die angebauten Modelle und unter **{{MfDienste}}** die Prüfung für das Spiel. Ein Reiter, der im "
        "laufenden Bereich nichts hat, ist ausgegraut.",
        "**右侧面板随操作方式而变。** **{{MbWieMax}}**:带有选项卡 **{{MfErstellen}}**、**{{MfAendern}}**、**{{MfHierarchie}}**、"
        "**{{MfBewegung}}**、**{{MfAnzeige}}** 和 **{{MfDienste}}** 的 **{{MbBefehlsfeld}}**,分组使用平直的标题条。**{{MbWieBlender}}**:"
        "**{{MfEigenschaften}}**,左侧是一列竖排图标(**{{MfWerkzeug}}**、**{{MfObjekt}}**、**{{MfModifier}}**、**{{MfDaten}}**、"
        "**{{MfKnochen}}**、**{{MfConstraints}}**、**{{MfMaterial}}**——名称显示在工具提示中),分组是圆角面板。选项卡的每个区域会选择自己的"
        "选项卡——**{{MbBereichAnsehen}}** 打开 **{{MfAnzeige}}** 或 **{{MfObjekt}}**,**{{MbBereichAnimation}}** 打开 **{{MfBewegung}}** 或 "
        "**{{MfKnochen}}**——并在那里自上而下显示它的全部分组。其他选项卡显示各自的部分:在 **{{MbBereichAnsehen}}** 中,"
        "**{{MfHierarchie}}** 下是挂接的模型,**{{MfDienste}}** 下是针对游戏的检查。当前区域中没有内容的选项卡显示为灰色。",
        "**右のパネルは操作モードに従います。** **{{MbWieMax}}**: **{{MfErstellen}}**、**{{MfAendern}}**、**{{MfHierarchie}}**、"
        "**{{MfBewegung}}**、**{{MfAnzeige}}**、**{{MfDienste}}** のタブを持つ **{{MbBefehlsfeld}}** で、グループは平らな見出しバーです。"
        "**{{MbWieBlender}}**: **{{MfEigenschaften}}** で、左に縦一列のアイコン(**{{MfWerkzeug}}**、**{{MfObjekt}}**、**{{MfModifier}}**、"
        "**{{MfDaten}}**、**{{MfKnochen}}**、**{{MfConstraints}}**、**{{MfMaterial}}** - 名前はツールチップに表示)が並び、グループは角の丸い"
        "パネルです。タブの各エリアは自分のタブを選びます。**{{MbBereichAnsehen}}** は **{{MfAnzeige}}** または **{{MfObjekt}}** を、"
        "**{{MbBereichAnimation}}** は **{{MfBewegung}}** または **{{MfKnochen}}** を開き、そこにそのエリアのすべてのグループを縦に並べます。"
        "ほかのタブにはそれぞれの担当分が表示されます。**{{MbBereichAnsehen}}** では **{{MfHierarchie}}** に取り付けたモデル、"
        "**{{MfDienste}}** にゲーム向けチェックがあります。現在のエリアに項目がないタブはグレー表示になります。",
        "**El panel de la derecha sigue el modo de control.** **{{MbWieMax}}**: el **{{MbBefehlsfeld}}** con las pestañas "
        "**{{MfErstellen}}**, **{{MfAendern}}**, **{{MfHierarchie}}**, **{{MfBewegung}}**, **{{MfAnzeige}}** y **{{MfDienste}}** y grupos "
        "con barras de título planas. **{{MbWieBlender}}**: **{{MfEigenschaften}}** con una columna vertical de símbolos "
        "(**{{MfWerkzeug}}**, **{{MfObjekt}}**, **{{MfModifier}}**, **{{MfDaten}}**, **{{MfKnochen}}**, **{{MfConstraints}}**, "
        "**{{MfMaterial}}**; el nombre aparece en la descripción emergente) y paneles redondeados. Cada área de la pestaña elige su "
        "pestaña: **{{MbBereichAnsehen}}** abre **{{MfAnzeige}}** u **{{MfObjekt}}**, **{{MbBereichAnimation}}** abre **{{MfBewegung}}** o "
        "**{{MfKnochen}}**, y muestra allí todos sus grupos, uno debajo de otro. Las demás pestañas muestran su parte: en "
        "**{{MbBereichAnsehen}}**, **{{MfHierarchie}}** contiene los modelos acoplados y **{{MfDienste}}** la comprobación para el juego. "
        "Una pestaña que no tiene nada en el área actual aparece atenuada.",
        "**Panel vpravo se řídí režimem ovládání.** **{{MbWieMax}}**: **{{MbBefehlsfeld}}** s kartami **{{MfErstellen}}**, "
        "**{{MfAendern}}**, **{{MfHierarchie}}**, **{{MfBewegung}}**, **{{MfAnzeige}}** a **{{MfDienste}}** a skupiny s plochými pruhy "
        "nadpisů. **{{MbWieBlender}}**: **{{MfEigenschaften}}** se svislým sloupcem symbolů (**{{MfWerkzeug}}**, **{{MfObjekt}}**, "
        "**{{MfModifier}}**, **{{MfDaten}}**, **{{MfKnochen}}**, **{{MfConstraints}}**, **{{MfMaterial}}** - název je v bublině) a "
        "zaoblenými panely. Každá oblast karty si volí svou kartu - **{{MbBereichAnsehen}}** otevře **{{MfAnzeige}}** nebo "
        "**{{MfObjekt}}**, **{{MbBereichAnimation}}** otevře **{{MfBewegung}}** nebo **{{MfKnochen}}** - a ukáže tam všechny své skupiny "
        "pod sebou. Ostatní karty ukazují svou část: v oblasti **{{MbBereichAnsehen}}** jsou pod **{{MfHierarchie}}** připojené modely a "
        "pod **{{MfDienste}}** kontrola pro hru. Karta, která v aktuální oblasti nic nemá, je zašedlá."),
    "bild_max": (
        "Like 3ds Max: the command panel with its tabs; groups with a short info on the right of the heading, buttons in a grid, values as a table.",
        "Wie 3ds Max: das Befehlsfeld mit seinen Reitern; Gruppen mit Kurzinfo rechts in der Überschrift, Knöpfe im Raster, Werte als Tabelle.",
        "与 3ds Max 相同:带选项卡的命令面板;分组标题右侧有简要信息,按钮排成网格,数值以表格显示。",
        "3ds Max と同じ: タブ付きのコマンドパネル。見出しの右に短い情報、ボタンはグリッド、値は表で表示されます。",
        "Como 3ds Max: el panel de comandos con sus pestañas; grupos con un dato breve a la derecha del título, botones en cuadrícula, valores en tabla.",
        "Jako 3ds Max: panel příkazů s kartami; skupiny s krátkým údajem vpravo v nadpisu, tlačítka v mřížce, hodnoty jako tabulka."),
    "gruppen": (
        "**Every group has the same form.** A click on the heading folds the group; on the right of the heading stands a short info "
        "(triangles, the drawn LOD, the number of attached models, the findings of the check - green **{{MfInfoGut}}** or red with their "
        "number). Buttons stand in a grid of two columns of equal width, each with a symbol and - where there is one - the key of the "
        "control mode as a small key cap; the main button of a group is highlighted in the accent colour. Values stand as a table "
        "(name on the left, value on the right), a choice such as LOD 1-10 or picmip 0-3 as one connected row, hints in a box with an "
        "info symbol. Nothing is cut off in a narrow panel: texts wrap, and the grid uses one column.",
        "**Jede Gruppe hat dieselbe Form.** Ein Klick auf die Überschrift klappt die Gruppe zu; rechts in der Überschrift steht eine "
        "Kurzinfo (Dreiecke, die gezeichnete LOD, die Zahl der angebauten Modelle, die Befunde der Prüfung - grün **{{MfInfoGut}}** oder "
        "rot mit ihrer Zahl). Knöpfe stehen im Raster aus zwei gleich breiten Spalten, jeder mit Symbol und - wo es eine gibt - der Taste "
        "des Bedienmodus als kleinem Tastenfeld; der Hauptknopf einer Gruppe ist in der Betonungsfarbe hervorgehoben. Werte stehen als "
        "Tabelle (Name links, Wert rechts), eine Wahl wie LOD 1-10 oder Picmip 0-3 als eine zusammenhängende Reihe, Hinweise in einem "
        "Kasten mit Info-Symbol. In einem schmalen Feld wird nichts abgeschnitten: Texte brechen um, und das Raster nimmt eine Spalte.",
        "**每个分组的形式都相同。** 单击标题可折叠分组;标题右侧有简要信息(三角形数、已绘制的 LOD、挂接模型的数量、检查的结果——绿色的 "
        "**{{MfInfoGut}}** 或带数量的红色)。按钮排成两列等宽的网格,每个按钮带有图标,并在有快捷键时以小键帽显示当前操作方式的按键;分组的"
        "主按钮以强调色突出显示。数值以表格显示(名称在左,数值在右),LOD 1-10 或 Picmip 0-3 这样的选择是一条相连的按钮行,提示放在带信息图标的"
        "方框里。面板较窄时不会截断任何内容:文字会换行,网格改用一列。",
        "**どのグループも同じ形です。** 見出しをクリックするとグループが折りたたまれ、見出しの右には短い情報(三角形数、描画中の LOD、取り付けた"
        "モデルの数、チェックの結果 - 緑の **{{MfInfoGut}}** または件数付きの赤)が表示されます。ボタンは同じ幅の 2 列のグリッドに並び、それぞれに"
        "アイコンと、キーがある場合は操作モードのキーが小さなキーキャップで付きます。グループの主ボタンはアクセントカラーで強調されます。値は表"
        "(左に名前、右に値)、LOD 1-10 や Picmip 0-3 のような選択はつながった 1 行、ヒントは情報アイコン付きの枠に表示されます。パネルが狭くても"
        "何も切れません。テキストは折り返し、グリッドは 1 列になります。",
        "**Todos los grupos tienen la misma forma.** Un clic en el título pliega el grupo; a la derecha del título hay un dato breve "
        "(triángulos, el LOD dibujado, el número de modelos acoplados, los avisos de la comprobación: **{{MfInfoGut}}** en verde o su "
        "número en rojo). Los botones están en una cuadrícula de dos columnas de igual ancho, cada uno con un símbolo y, cuando existe, "
        "la tecla del modo de control como una pequeña tecla; el botón principal de un grupo se resalta con el color de acento. Los "
        "valores aparecen como tabla (nombre a la izquierda, valor a la derecha), una elección como LOD 1-10 o picmip 0-3 como una fila "
        "unida, y las indicaciones en un recuadro con símbolo de información. En un panel estrecho no se corta nada: los textos se "
        "ajustan y la cuadrícula usa una columna.",
        "**Každá skupina má stejnou podobu.** Kliknutí na nadpis skupinu sbalí; vpravo v nadpisu je krátký údaj (trojúhelníky, "
        "vykreslené LOD, počet připojených modelů, nálezy kontroly - zeleně **{{MfInfoGut}}** nebo červeně s jejich počtem). Tlačítka "
        "stojí v mřížce dvou stejně širokých sloupců, každé se symbolem a - pokud existuje - s klávesou režimu ovládání jako malou "
        "klávesou; hlavní tlačítko skupiny je zvýrazněné barvou zvýraznění. Hodnoty jsou v tabulce (název vlevo, hodnota vpravo), volba "
        "jako LOD 1-10 nebo picmip 0-3 jako jedna spojená řada, rady v rámečku se symbolem informace. V úzkém panelu se nic neořízne: "
        "texty se zalomí a mřížka použije jeden sloupec."),
    "bild_blender": (
        "Like Blender: the properties editor with the column of symbols on its left and rounded panels; labels stand right-aligned in front of their fields.",
        "Wie Blender: der Eigenschaften-Editor mit der Symbolspalte links und abgerundeten Kästen; Beschriftungen stehen rechtsbündig vor ihren Feldern.",
        "与 Blender 相同:属性编辑器左侧是图标列,分组为圆角面板;标签在字段前右对齐。",
        "Blender と同じ: 左にアイコン列を持つプロパティエディターと角の丸いパネル。ラベルはフィールドの前に右揃えで並びます。",
        "Como Blender: el editor de propiedades con la columna de símbolos a la izquierda y paneles redondeados; las etiquetas se alinean a la derecha delante de sus campos.",
        "Jako Blender: editor vlastností se sloupcem symbolů vlevo a zaoblenými panely; popisky stojí zarovnané vpravo před svými poli."),
    "rahmen": (
        "**Width, areas and control mode.** Drag the edge between the views and the panel to make the panel wider or narrower (a "
        "double-click restores the default width); the program remembers the width. The areas of the tab - **{{MbBereichAnsehen}}**, "
        "**{{MbBereichModellieren}}**, **{{MbBereichUv}}**, **{{MbBereichWeights}}**, **{{MbBereichPainting}}**, "
        "**{{MbBereichAnimation}}** - stand as one row of equally wide fields across the whole tab, above the explorer, the views and the "
        "panel; next to them stands a short hint with the keys and the mouse navigation of the control mode for this area. The control "
        "mode itself is switched with a click on **{{MbWieMax}}** / **{{MbWieBlender}}** at the left of the status line of the tab, in "
        "the menu of the header of a view (**{{MbBedienung}}**), in the settings or with the search for all actions.",
        "**Breite, Bereiche und Bedienmodus.** Ziehe die Kante zwischen den Ansichten und dem Feld, um das Feld breiter oder schmaler zu "
        "machen (ein Doppelklick stellt die Vorgabe wieder her); das Programm merkt sich die Breite. Die Bereiche des Tabs - "
        "**{{MbBereichAnsehen}}**, **{{MbBereichModellieren}}**, **{{MbBereichUv}}**, **{{MbBereichWeights}}**, **{{MbBereichPainting}}**, "
        "**{{MbBereichAnimation}}** - stehen als eine Reihe gleich breiter Felder über dem ganzen Tab, über Explorer, Ansichten und Feld; "
        "daneben steht ein kurzer Hinweis mit den Tasten und der Mausnavigation des Bedienmodus für diesen Bereich. Den Bedienmodus "
        "selbst schaltest du mit einem Klick auf **{{MbWieMax}}** / **{{MbWieBlender}}** links in der Statuszeile des Tabs um, im Menü "
        "der Kopfzeile einer Ansicht (**{{MbBedienung}}**), in den Einstellungen oder über die Suche nach allen Aktionen.",
        "**宽度、区域和操作方式。** 拖动视图与面板之间的边缘可加宽或收窄面板(双击恢复默认宽度);程序会记住该宽度。选项卡的区域——"
        "**{{MbBereichAnsehen}}**、**{{MbBereichModellieren}}**、**{{MbBereichUv}}**、**{{MbBereichWeights}}**、**{{MbBereichPainting}}**、"
        "**{{MbBereichAnimation}}**——作为一行等宽的格子横跨整个选项卡,位于资源管理器、视图和面板的上方;旁边是一条简短提示,列出当前区域在该操作"
        "方式下的按键和鼠标导航。操作方式本身可以这样切换:单击选项卡状态栏左侧的 **{{MbWieMax}}** / **{{MbWieBlender}}**,或在视图标题的菜单"
        "(**{{MbBedienung}}**)、设置中切换,也可以通过搜索所有操作来切换。",
        "**幅、エリア、操作モード。** ビューとパネルの境目をドラッグするとパネルの幅を変えられます(ダブルクリックで既定の幅に戻ります)。"
        "幅はプログラムが記憶します。タブのエリア - **{{MbBereichAnsehen}}**、**{{MbBereichModellieren}}**、**{{MbBereichUv}}**、"
        "**{{MbBereichWeights}}**、**{{MbBereichPainting}}**、**{{MbBereichAnimation}}** - は、同じ幅のフィールドが並ぶ 1 行として、エクスプローラー、"
        "ビュー、パネルの上にタブ全体の幅で表示されます。その横には、このエリアでの操作モードのキーとマウス操作の短いヒントが表示されます。"
        "操作モード自体は、タブのステータス行の左にある **{{MbWieMax}}** / **{{MbWieBlender}}** をクリックするか、ビューのヘッダーのメニュー"
        "(**{{MbBedienung}}**)、設定、またはすべてのアクションの検索から切り替えます。",
        "**Ancho, áreas y modo de control.** Arrastra el borde entre las vistas y el panel para hacerlo más ancho o más estrecho (un "
        "doble clic restablece el ancho predeterminado); el programa recuerda el ancho. Las áreas de la pestaña - "
        "**{{MbBereichAnsehen}}**, **{{MbBereichModellieren}}**, **{{MbBereichUv}}**, **{{MbBereichWeights}}**, **{{MbBereichPainting}}**, "
        "**{{MbBereichAnimation}}** - forman una fila de campos de igual ancho a lo largo de toda la pestaña, sobre el explorador, las "
        "vistas y el panel; al lado hay una indicación breve con las teclas y la navegación con el ratón del modo de control para esa "
        "área. El modo de control se cambia con un clic en **{{MbWieMax}}** / **{{MbWieBlender}}** a la izquierda de la línea de estado "
        "de la pestaña, en el menú de la cabecera de una vista (**{{MbBedienung}}**), en los ajustes o con la búsqueda de todas las "
        "acciones.",
        "**Šířka, oblasti a režim ovládání.** Tažením hrany mezi pohledy a panelem panel rozšíříte nebo zúžíte (dvojklik obnoví "
        "výchozí šířku); program si šířku pamatuje. Oblasti karty - **{{MbBereichAnsehen}}**, **{{MbBereichModellieren}}**, "
        "**{{MbBereichUv}}**, **{{MbBereichWeights}}**, **{{MbBereichPainting}}**, **{{MbBereichAnimation}}** - stojí jako jedna řada "
        "stejně širokých polí přes celou kartu, nad průzkumníkem, pohledy a panelem; vedle nich je krátká nápověda s klávesami a "
        "navigací myší v daném režimu ovládání pro tuto oblast. Samotný režim ovládání přepnete kliknutím na **{{MbWieMax}}** / "
        "**{{MbWieBlender}}** vlevo ve stavovém řádku karty, v nabídce záhlaví pohledu (**{{MbBedienung}}**), v nastavení nebo "
        "hledáním všech akcí."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def einhaengen(ansehen):
    """Absaetze und Bilder direkt hinter die Ueberschrift "Befehlsfeld" der Seite modell-ansehen (vor die Gruppen im Einzelnen)."""
    b = ansehen["bloecke"]
    i = next(n for n, x in enumerate(b) if x[0] == "h2" and x[1] == "befehlsfeld") + 1
    neu = [
        ("p", _t("feld")),
        ("img", "h_ap3_max", _t("bild_max"), "breit"),
        ("p", _t("gruppen")),
        ("img", "h_ap3_blender", _t("bild_blender"), "breit"),
        ("p", _t("rahmen")),
    ]
    b[i:i] = neu
