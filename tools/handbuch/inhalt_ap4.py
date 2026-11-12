"""AP4 (MBENTWURF Teil B): die Seite "Model-Tab: der Aufbau wie Blender" - im Bedienmodus wie Blender die Werkzeugleiste (T)
links mit den Werkzeugen des Bereichs, der Modus im Kopf der Ansicht, der Outliner oben rechts ueber dem Eigenschaften-Editor, die
Seitenleiste (N), das Werkzeug Messen und die Fensteranordnung je Bedienmodus; wie 3ds Max bleibt der Aufbau, wie er war.

Eigene Quelle des Pakets; AP14 haengt die Seite ins Handbuch (inhalt.py: `inhalt_ap4.seite()` hinter modell-bedienung bzw.
modell-zeitleiste, ES/CS in inhalt_es.py / inhalt_cs.py, die Texte von i18n_ap4 in gen_handbuch.py `_lade_ui_texte`; die von
i18n_ent1 - Werkzeugnamen wie Blender, Gruppen, Seitenleiste eingebbar - laedt gen_handbuch.py schon) - wie
AP6-AP10. Die Bilder h_ap4_max, h_ap4_blender und h_ap4_seitenleiste nimmt der Selbsttest auf (BHED_EDITORTEST=handbuch,
BHED_HB_NUR=ap4). Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs); {{Kennung}} setzt den Text der Oberflaeche ein.
"""

X = {
    "titel": ("Model tab: the layout like Blender", "Model-Tab: der Aufbau wie Blender", "模型选项卡: 像 Blender 的布局", "モデルタブ: Blender 風のレイアウト",
              "Pestaña Model: la disposición como Blender", "Karta Model: rozvržení jako Blender"),
    "kurz": (
        "The control mode changes not only the keys but also the look: like 3ds Max the scene explorer is on the left and the command panel on the right; like Blender the toolbar is on the left, the outliner at the top right above the properties, and the sidebar opens with N.",
        "Der Bedienmodus ändert nicht nur die Tasten, sondern auch das Aussehen: wie 3ds Max steht links der Szenen-Explorer und rechts das Befehlsfeld; wie Blender steht links die Werkzeugleiste, oben rechts der Outliner über den Eigenschaften, und N öffnet die Seitenleiste.",
        "操作模式不仅改变按键, 也改变外观: 像 3ds Max 时左侧是场景资源管理器, 右侧是命令面板; 像 Blender 时左侧是工具栏, 右上方是大纲视图, 其下是属性, 按 N 打开侧栏。",
        "操作モードはキーだけでなく見た目も変えます: 3ds Max 風では左にシーンエクスプローラー、右にコマンドパネル。Blender 風では左にツールバー、右上にアウトライナー、その下にプロパティがあり、N でサイドバーが開きます。",
        "El modo de control no solo cambia las teclas, también el aspecto: como 3ds Max, a la izquierda está el explorador de escena y a la derecha el panel de comandos; como Blender, a la izquierda la barra de herramientas, arriba a la derecha el Outliner sobre las propiedades, y N abre la barra lateral.",
        "Režim ovládání nemění jen klávesy, ale i vzhled: jako 3ds Max je vlevo průzkumník scény a vpravo příkazový panel; jako Blender je vlevo panel nástrojů, vpravo nahoře Outliner nad vlastnostmi a N otevře postranní panel."),
    "bild_max": (
        "Like 3ds Max (area Modeling): scene explorer on the left, the view in the middle, the command panel with its tabs on the right - unchanged.",
        "Wie 3ds Max (Bereich Modellieren): links der Szenen-Explorer, in der Mitte die Ansicht, rechts das Befehlsfeld mit seinen Reitern - unverändert.",
        "像 3ds Max (建模区域): 左侧场景资源管理器, 中间视图, 右侧带选项卡的命令面板 - 保持不变。",
        "3ds Max 風 (モデリングエリア): 左にシーンエクスプローラー、中央にビュー、右にタブ付きのコマンドパネル - 変更なし。",
        "Como 3ds Max (área Modelado): a la izquierda el explorador de escena, en el centro la vista, a la derecha el panel de comandos con sus pestañas - sin cambios.",
        "Jako 3ds Max (oblast Modelování): vlevo průzkumník scény, uprostřed pohled, vpravo příkazový panel s kartami - beze změny."),
    "bild_blender": (
        "The same area like Blender: the toolbar on the left with **{{BzAuswaehlen}}**, **{{BzBewegen}}**, **{{BzDrehen}}**, **{{BzSkalieren}}** and the tools of Modeling, the mode in the header of the view, the outliner at the top right and the properties editor with its column of tabs below it.",
        "Derselbe Bereich wie Blender: links die Werkzeugleiste mit **{{BzAuswaehlen}}**, **{{BzBewegen}}**, **{{BzDrehen}}**, **{{BzSkalieren}}** und den Werkzeugen des Modellierens, im Kopf der Ansicht der Modus, oben rechts der Outliner und darunter der Eigenschaften-Editor mit seiner Reiterspalte.",
        "同一区域像 Blender 时: 左侧工具栏含 **{{BzAuswaehlen}}**、**{{BzBewegen}}**、**{{BzDrehen}}**、**{{BzSkalieren}}** 和建模工具, 视图标题中显示模式, 右上方是大纲视图, 其下是带选项卡列的属性编辑器。",
        "同じエリアの Blender 風: 左のツールバーに **{{BzAuswaehlen}}**、**{{BzBewegen}}**、**{{BzDrehen}}**、**{{BzSkalieren}}** とモデリングのツール、ビューのヘッダーにモード、右上にアウトライナー、その下にタブ列付きのプロパティエディター。",
        "La misma área como Blender: a la izquierda la barra de herramientas con **{{BzAuswaehlen}}**, **{{BzBewegen}}**, **{{BzDrehen}}**, **{{BzSkalieren}}** y las herramientas de Modelado, en la cabecera de la vista el modo, arriba a la derecha el Outliner y debajo el editor de propiedades con su columna de pestañas.",
        "Stejná oblast jako Blender: vlevo panel nástrojů s **{{BzAuswaehlen}}**, **{{BzBewegen}}**, **{{BzDrehen}}**, **{{BzSkalieren}}** a nástroji modelování, v záhlaví pohledu režim, vpravo nahoře Outliner a pod ním editor vlastností se sloupcem karet."),
    "h_leiste": ("Toolbar (T)", "Werkzeugleiste (T)", "工具栏 (T)", "ツールバー (T)", "Barra de herramientas (T)", "Panel nástrojů (T)"),
    "leiste": (
        "Like Blender the column on the left holds the **{{BzWerkzeugleiste}}**. At the top are the four tools of every area: **{{BzAuswaehlen}}**, **{{BzBewegen}}**, **{{BzDrehen}}** and **{{BzSkalieren}}** "
        "(they choose the gizmo at the selection; G, R and S work without it). Below a line follow the tools of the area: View/Check **{{BzMessen}}**, **{{BzFlaecheWaehlen}}**, **{{BzKnochenWaehlen}}** and "
        "**{{MbFoto}}**; Modeling **{{BeExtrude}}**, **{{BeInset}}**, **{{BzFase}}**, **{{BeLoopCut}}**, **{{BzMesser}}**, **{{MdMerge}}**; UV **{{BeNaht}}**, **{{UvAbwickeln}}**, **{{BeEntspannen}}**, "
        "**{{BePacken}}**; Weights **{{MgRPinsel}}**, **{{MgWeich}}**, **{{BeVerlauf}}**, **{{BeGewichtHolen}}**; Painting **{{MgRPinsel}}**, **{{BeFuellen}}**, **{{PtWzKlonen}}**, **{{BeWischen}}**, "
        "**{{BeMaske}}**; Animation **{{BeKopieren}}**, **{{BeGespiegelt}}**, **{{MbKvReduzieren}}**. The chosen tool is highlighted; the tooltip names the key. T hides the toolbar - a small arrow "
        "brings it back - and T shows it again. Every tool also stays where it was: in the panel on the right, in the menus, in Search all actions and in the context menu of the view.",
        "Wie Blender steht in der Spalte links die **{{BzWerkzeugleiste}}**. Oben stehen die vier Werkzeuge jedes Bereichs: **{{BzAuswaehlen}}**, **{{BzBewegen}}**, **{{BzDrehen}}** und **{{BzSkalieren}}** "
        "(sie wählen das Gizmo an der Auswahl; G, R und S gehen ohne). Unter einem Strich folgen die Werkzeuge des Bereichs: Ansehen/Prüfen **{{BzMessen}}**, **{{BzFlaecheWaehlen}}**, **{{BzKnochenWaehlen}}** und "
        "**{{MbFoto}}**; Modellieren **{{BeExtrude}}**, **{{BeInset}}**, **{{BzFase}}**, **{{BeLoopCut}}**, **{{BzMesser}}**, **{{MdMerge}}**; UV **{{BeNaht}}**, **{{UvAbwickeln}}**, **{{BeEntspannen}}**, "
        "**{{BePacken}}**; Weights **{{MgRPinsel}}**, **{{MgWeich}}**, **{{BeVerlauf}}**, **{{BeGewichtHolen}}**; Painting **{{MgRPinsel}}**, **{{BeFuellen}}**, **{{PtWzKlonen}}**, **{{BeWischen}}**, "
        "**{{BeMaske}}**; Animation **{{BeKopieren}}**, **{{BeGespiegelt}}**, **{{MbKvReduzieren}}**. Das gewählte Werkzeug ist hervorgehoben; der Tooltip nennt die Taste. T blendet die Leiste aus - ein kleiner "
        "Pfeil holt sie zurück - und T wieder ein. Jedes Werkzeug bleibt auch, wo es war: im Feld rechts, in den Menüs, in Alle Aktionen suchen und im Rechtsklick der Ansicht.",
        "像 Blender 时, 左侧一栏是 **{{BzWerkzeugleiste}}**。顶部是每个区域都有的四个工具: **{{BzAuswaehlen}}**、**{{BzBewegen}}**、**{{BzDrehen}}** 和 **{{BzSkalieren}}** (它们选择所选内容处的操纵器; G、R、S "
        "无需操纵器)。分隔线下面是该区域的工具: 查看/检查为 **{{BzMessen}}**、**{{BzFlaecheWaehlen}}**、**{{BzKnochenWaehlen}}** 和 **{{MbFoto}}**; 建模为 **{{BeExtrude}}**、**{{BeInset}}**、**{{BzFase}}**、"
        "**{{BeLoopCut}}**、**{{BzMesser}}**、**{{MdMerge}}**; UV 为 **{{BeNaht}}**、**{{UvAbwickeln}}**、**{{BeEntspannen}}**、**{{BePacken}}**; 权重为 **{{MgRPinsel}}**、**{{MgWeich}}**、**{{BeVerlauf}}**、"
        "**{{BeGewichtHolen}}**; 绘制为 **{{MgRPinsel}}**、**{{BeFuellen}}**、**{{PtWzKlonen}}**、**{{BeWischen}}**、**{{BeMaske}}**; 动画为 **{{BeKopieren}}**、**{{BeGespiegelt}}**、**{{MbKvReduzieren}}**。"
        "所选工具会突出显示; 工具提示给出按键。按 T 隐藏工具栏 (一个小箭头可将其找回), 再按 T 显示。每个工具也仍在原处: 右侧面板、菜单、“搜索所有操作”和视图的右键菜单中。",
        "Blender 風では左の列に **{{BzWerkzeugleiste}}** があります。上にはどのエリアにもある 4 つのツール: **{{BzAuswaehlen}}**、**{{BzBewegen}}**、**{{BzDrehen}}**、**{{BzSkalieren}}** (選択にギズモを表示します。"
        "G、R、S はギズモなしで使えます)。線の下にエリアのツールが続きます: 表示/チェックは **{{BzMessen}}**、**{{BzFlaecheWaehlen}}**、**{{BzKnochenWaehlen}}**、**{{MbFoto}}**、モデリングは **{{BeExtrude}}**、"
        "**{{BeInset}}**、**{{BzFase}}**、**{{BeLoopCut}}**、**{{BzMesser}}**、**{{MdMerge}}**、UV は **{{BeNaht}}**、**{{UvAbwickeln}}**、**{{BeEntspannen}}**、**{{BePacken}}**、ウェイトは **{{MgRPinsel}}**、"
        "**{{MgWeich}}**、**{{BeVerlauf}}**、**{{BeGewichtHolen}}**、ペイントは **{{MgRPinsel}}**、**{{BeFuellen}}**、**{{PtWzKlonen}}**、**{{BeWischen}}**、**{{BeMaske}}**、アニメーションは **{{BeKopieren}}**、"
        "**{{BeGespiegelt}}**、**{{MbKvReduzieren}}**。選んだツールは強調表示され、ツールチップがキーを示します。T でツールバーを隠し (小さな矢印で戻せます)、もう一度 T で表示します。どのツールも元の場所にも残ります: "
        "右のパネル、メニュー、「すべてのアクションを検索」、ビューの右クリックメニュー。",
        "Como Blender, la columna de la izquierda contiene la **{{BzWerkzeugleiste}}**. Arriba están las cuatro herramientas de cada área: **{{BzAuswaehlen}}**, **{{BzBewegen}}**, **{{BzDrehen}}** y "
        "**{{BzSkalieren}}** (eligen el gizmo en la selección; G, R y S funcionan sin él). Bajo una línea siguen las herramientas del área: Ver/Comprobar **{{BzMessen}}**, **{{BzFlaecheWaehlen}}**, "
        "**{{BzKnochenWaehlen}}** y **{{MbFoto}}**; Modelado **{{BeExtrude}}**, **{{BeInset}}**, **{{BzFase}}**, **{{BeLoopCut}}**, **{{BzMesser}}**, **{{MdMerge}}**; UV **{{BeNaht}}**, **{{UvAbwickeln}}**, "
        "**{{BeEntspannen}}**, **{{BePacken}}**; Weights **{{MgRPinsel}}**, **{{MgWeich}}**, **{{BeVerlauf}}**, **{{BeGewichtHolen}}**; Painting **{{MgRPinsel}}**, **{{BeFuellen}}**, **{{PtWzKlonen}}**, "
        "**{{BeWischen}}**, **{{BeMaske}}**; Animación **{{BeKopieren}}**, **{{BeGespiegelt}}**, **{{MbKvReduzieren}}**. La herramienta elegida aparece resaltada; la información emergente indica la tecla. "
        "T oculta la barra (una pequeña flecha la recupera) y T la vuelve a mostrar. Cada herramienta sigue también donde estaba: en el panel de la derecha, en los menús, en Buscar todas las acciones y en el menú contextual de la vista.",
        "Jako Blender je ve sloupci vlevo **{{BzWerkzeugleiste}}**. Nahoře jsou čtyři nástroje každé oblasti: **{{BzAuswaehlen}}**, **{{BzBewegen}}**, **{{BzDrehen}}** a **{{BzSkalieren}}** (volí manipulátor u výběru; "
        "G, R a S fungují i bez něj). Pod čarou následují nástroje oblasti: Prohlížet/Kontrola **{{BzMessen}}**, **{{BzFlaecheWaehlen}}**, **{{BzKnochenWaehlen}}** a **{{MbFoto}}**; Modelování **{{BeExtrude}}**, "
        "**{{BeInset}}**, **{{BzFase}}**, **{{BeLoopCut}}**, **{{BzMesser}}**, **{{MdMerge}}**; UV **{{BeNaht}}**, **{{UvAbwickeln}}**, **{{BeEntspannen}}**, **{{BePacken}}**; Weights **{{MgRPinsel}}**, "
        "**{{MgWeich}}**, **{{BeVerlauf}}**, **{{BeGewichtHolen}}**; Painting **{{MgRPinsel}}**, **{{BeFuellen}}**, **{{PtWzKlonen}}**, **{{BeWischen}}**, **{{BeMaske}}**; Animace **{{BeKopieren}}**, "
        "**{{BeGespiegelt}}**, **{{MbKvReduzieren}}**. Zvolený nástroj je zvýrazněný; bublinová nápověda uvádí klávesu. T panel skryje (malá šipka ho vrátí) a T ho znovu zobrazí. Každý nástroj zůstává i tam, kde byl: "
        "v panelu vpravo, v nabídkách, ve Vyhledat všechny akce a v místní nabídce pohledu."),
    "bild_gruppen": (
        "Like Blender in Modeling with the tool names shown and the group of Extrude Region held open: Extrude Along Normals and Extrude Individual.",
        "Wie Blender im Modellieren mit den Werkzeugnamen und der gedrückt gehaltenen Gruppe von Region extrudieren: Entlang der Normalen extrudieren und Individuell extrudieren.",
        "像 Blender 时的建模区域, 显示工具名称, 按住打开挤出选区组: 沿法向挤出和挤出各个面。",
        "Blender 風のモデリングでツール名を表示し、押し出し（領域）のグループを長押しで開いた状態: 押し出し（法線方向）と押し出し（個別）。",
        "Como Blender en Modelado con los nombres de las herramientas y el grupo de Extruir región abierto manteniendo pulsado: Extruir según normales y Extruir individualmente.",
        "Jako Blender v Modelování se zobrazenými názvy nástrojů a podržením otevřenou skupinou Vysunutí oblasti: Vysunutí Podél Normál a Vysunout Jednotlivé."),
    "h_gruppen": ("Tool groups and tool names", "Werkzeuggruppen und Werkzeugnamen", "工具组与工具名称", "ツールグループとツール名", "Grupos de herramientas y nombres", "Skupiny nástrojů a názvy"),
    "gruppen": (
        "The tools are named as in Blender and, in the languages Blender is translated into, use its own words. Like Blender some tools form a group: a small triangle in the corner of the "
        "button shows it. A short click takes the tool shown; holding the button for 0.2 seconds or dragging from it opens the group, and releasing on an entry chooses it - the button then "
        "shows that tool. Modeling has the groups **{{BeExtrude}}** (with **{{BeExtrudeNormalen}}** and **{{BeExtrudeEinzeln}}**) and **{{BzMesser}}** (with **{{BeHalbieren}}**). "
        "A right-click on the toolbar offers **{{BeWerkzeugnamen}}**: the toolbar gets wider and shows the name next to each tool. The program remembers the choice.",
        "Die Werkzeuge heißen wie in Blender, in den Sprachen, in die Blender übersetzt ist, mit Blenders eigenen Wörtern. Wie in Blender bilden manche Werkzeuge eine Gruppe: ein kleines "
        "Dreieck in der Ecke des Knopfs zeigt es. Ein kurzer Klick nimmt das gezeigte Werkzeug; 0,2 Sekunden gedrückt halten oder vom Knopf wegziehen öffnet die Gruppe, und Loslassen auf "
        "einem Eintrag wählt ihn - der Knopf zeigt danach dieses Werkzeug. Modellieren hat die Gruppen **{{BeExtrude}}** (mit **{{BeExtrudeNormalen}}** und **{{BeExtrudeEinzeln}}**) und "
        "**{{BzMesser}}** (mit **{{BeHalbieren}}**). Ein Rechtsklick auf die Werkzeugleiste bietet **{{BeWerkzeugnamen}}**: die Leiste wird breiter und zeigt neben jedem Werkzeug den "
        "Namen. Das Programm merkt sich die Wahl.",
        "工具名称与 Blender 相同, 在 Blender 已翻译的语言中使用 Blender 自己的用词。与 Blender 一样, 有些工具组成一个组: 按钮角上的小三角表示这一点。短按单击使用显示的工具; 按住按钮 0.2 秒或从按钮拖开会打开该组, "
        "在某个条目上松开即选择它 - 之后按钮显示该工具。建模中有 **{{BeExtrude}}** 组 (含 **{{BeExtrudeNormalen}}** 和 **{{BeExtrudeEinzeln}}**) 和 **{{BzMesser}}** 组 (含 **{{BeHalbieren}}**)。"
        "右键单击工具栏可选择 **{{BeWerkzeugnamen}}**: 工具栏变宽, 每个工具旁显示名称。程序会记住此选择。",
        "ツールの名前は Blender と同じで、Blender が翻訳されている言語では Blender 自身の用語を使います。Blender と同じく一部のツールはグループになっていて、ボタンの角の小さな三角がそれを示します。"
        "短いクリックで表示中のツールを使い、ボタンを 0.2 秒押し続けるかボタンからドラッグするとグループが開き、項目の上で離すとそれを選びます - 以後ボタンにはそのツールが表示されます。"
        "モデリングには **{{BeExtrude}}** グループ (**{{BeExtrudeNormalen}}** と **{{BeExtrudeEinzeln}}**) と **{{BzMesser}}** グループ (**{{BeHalbieren}}**) があります。"
        "ツールバーを右クリックすると **{{BeWerkzeugnamen}}** があり、ツールバーが広がって各ツールの横に名前を表示します。選択は記憶されます。",
        "Las herramientas se llaman como en Blender y, en los idiomas a los que Blender está traducido, con sus propias palabras. Como en Blender, algunas herramientas forman un grupo: lo indica "
        "un pequeño triángulo en la esquina del botón. Un clic corto toma la herramienta mostrada; mantener pulsado el botón 0,2 segundos o arrastrar desde él abre el grupo, y soltar sobre "
        "una entrada la elige: después el botón muestra esa herramienta. Modelado tiene los grupos **{{BeExtrude}}** (con **{{BeExtrudeNormalen}}** y **{{BeExtrudeEinzeln}}**) y "
        "**{{BzMesser}}** (con **{{BeHalbieren}}**). Un clic derecho en la barra de herramientas ofrece **{{BeWerkzeugnamen}}**: la barra se ensancha y muestra el nombre junto a cada "
        "herramienta. El programa recuerda la elección.",
        "Nástroje se jmenují jako v Blenderu a v jazycích, do kterých je Blender přeložen, jeho vlastními slovy. Jako v Blenderu tvoří některé nástroje skupinu: ukazuje to malý trojúhelník "
        "v rohu tlačítka. Krátké kliknutí vezme zobrazený nástroj; podržení tlačítka 0,2 sekundy nebo tažení z něj otevře skupinu a puštění na položce ji vybere - tlačítko pak ukazuje "
        "tento nástroj. Modelování má skupiny **{{BeExtrude}}** (s **{{BeExtrudeNormalen}}** a **{{BeExtrudeEinzeln}}**) a **{{BzMesser}}** (s **{{BeHalbieren}}**). Kliknutí pravým "
        "tlačítkem na panel nástrojů nabídne **{{BeWerkzeugnamen}}**: panel se rozšíří a vedle každého nástroje ukáže název. Program si volbu zapamatuje."),
    "h_kopf": ("Mode, outliner and properties", "Modus, Outliner und Eigenschaften", "模式、大纲视图和属性", "モード、アウトライナー、プロパティ", "Modo, Outliner y propiedades", "Režim, Outliner a vlastnosti"),
    "kopf": (
        "The header of the view names it like Blender, followed by the mode of the area: **{{MbBlObjektModus}}** in View/Check, **{{MbBlEditModus}}** in Modeling and UV, **{{BzModusWeight}}**, **{{BzModusTexture}}** "
        "and **{{BzModusPose}}** in Animation. A click on the mode opens the choice of modes (like Ctrl+Tab) and changes the area. On the right the **{{MbXOutliner}}** shows the same tree as the scene explorer, "
        "with the switches in columns on the right; below it the **{{MfEigenschaften}}** editor shows the groups of the area with its column of tabs. The edge between both can be dragged.",
        "Der Kopf der Ansicht nennt sie wie Blender, dahinter den Modus des Bereichs: **{{MbBlObjektModus}}** in Ansehen/Prüfen, **{{MbBlEditModus}}** in Modellieren und UV, **{{BzModusWeight}}**, **{{BzModusTexture}}** "
        "und **{{BzModusPose}}** in Animation. Ein Klick auf den Modus öffnet die Wahl der Modi (wie Strg+Tab) und wechselt den Bereich. Rechts zeigt der **{{MbXOutliner}}** denselben Baum wie der Szenen-Explorer, "
        "mit den Umschaltern in Spalten rechts; darunter zeigt der Editor **{{MfEigenschaften}}** die Gruppen des Bereichs mit seiner Reiterspalte. Die Kante zwischen beiden lässt sich ziehen.",
        "视图标题像 Blender 一样给出视图名称, 其后是该区域的模式: 查看/检查为 **{{MbBlObjektModus}}**, 建模和 UV 为 **{{MbBlEditModus}}**, 以及 **{{BzModusWeight}}**、**{{BzModusTexture}}**, 动画为 **{{BzModusPose}}**。"
        "单击模式可打开模式选择 (与 Ctrl+Tab 相同) 并切换区域。右侧的 **{{MbXOutliner}}** 显示与场景资源管理器相同的树, 开关位于右侧的列中; 其下的 **{{MfEigenschaften}}** 编辑器以选项卡列显示该区域的各组。两者之间的边界可以拖动。",
        "ビューのヘッダーは Blender と同じくビュー名を示し、その後にエリアのモードが続きます: 表示/チェックは **{{MbBlObjektModus}}**、モデリングと UV は **{{MbBlEditModus}}**、**{{BzModusWeight}}**、**{{BzModusTexture}}**、"
        "アニメーションは **{{BzModusPose}}**。モードをクリックするとモードの選択 (Ctrl+Tab と同じ) が開き、エリアが切り替わります。右の **{{MbXOutliner}}** はシーンエクスプローラーと同じツリーを表示し、スイッチは右の列にあります。"
        "その下の **{{MfEigenschaften}}** エディターはタブ列とともにエリアのグループを表示します。両者の境界はドラッグできます。",
        "La cabecera de la vista la nombra como Blender y después el modo del área: **{{MbBlObjektModus}}** en Ver/Comprobar, **{{MbBlEditModus}}** en Modelado y UV, **{{BzModusWeight}}**, **{{BzModusTexture}}** "
        "y **{{BzModusPose}}** en Animación. Un clic en el modo abre la elección de modos (como Ctrl+Tab) y cambia el área. A la derecha, el **{{MbXOutliner}}** muestra el mismo árbol que el explorador de escena, "
        "con los interruptores en columnas a la derecha; debajo, el editor **{{MfEigenschaften}}** muestra los grupos del área con su columna de pestañas. El borde entre ambos se puede arrastrar.",
        "Záhlaví pohledu ho pojmenuje jako Blender a za ním uvede režim oblasti: **{{MbBlObjektModus}}** v Prohlížet/Kontrola, **{{MbBlEditModus}}** v Modelování a UV, **{{BzModusWeight}}**, **{{BzModusTexture}}** "
        "a **{{BzModusPose}}** v Animaci. Kliknutí na režim otevře volbu režimů (jako Ctrl+Tab) a přepne oblast. Vpravo **{{MbXOutliner}}** ukazuje stejný strom jako průzkumník scény, s přepínači ve sloupcích vpravo; "
        "pod ním editor **{{MfEigenschaften}}** ukazuje skupiny oblasti se sloupcem karet. Hranu mezi nimi lze táhnout."),
    "bild_seite": (
        "Like Blender with the sidebar (N) and a measurement from the torso to the head: the distance in game units is in the view and in the sidebar.",
        "Wie Blender mit der Seitenleiste (N) und einer Messung vom Rumpf zum Kopf: der Abstand in Spieleinheiten steht in der Ansicht und in der Seitenleiste.",
        "像 Blender 时打开侧栏 (N), 并从躯干测量到头部: 以游戏单位表示的距离显示在视图和侧栏中。",
        "Blender 風でサイドバー (N) を開き、胴体から頭まで測定した状態: ゲーム単位の距離がビューとサイドバーに表示されます。",
        "Como Blender con la barra lateral (N) y una medición del torso a la cabeza: la distancia en unidades del juego aparece en la vista y en la barra lateral.",
        "Jako Blender s postranním panelem (N) a měřením od trupu k hlavě: vzdálenost v herních jednotkách je v pohledu i v postranním panelu."),
    "h_seite": ("Sidebar (N) and measuring", "Seitenleiste (N) und Messen", "侧栏 (N) 与测量", "サイドバー (N) と測定", "Barra lateral (N) y medición", "Postranní panel (N) a měření"),
    "seite": (
        "N opens the **{{BzSeitenleiste}}** on the right of the view. The tab **{{BzReiterObjekt}}** shows the selection: for a surface its position and dimensions, for a bone or a control of the rig its location "
        "and rotation in the pose, and the last measurement. Like Blender the values are number fields: drag left or right to change them, click to type a value (Enter takes it, Esc cancels); "
        "Ctrl+Z undoes one input in one step. A surface moves or scales with them, a control of the rig in the area Animation (pose mode) moves and rotates as far as it can with the gizmo; a bone "
        "outside the pose mode is only shown, like in Blender. The tab **{{BzReiterAnsicht}}** sets the field of view and switches perspective and orthographic. The tool **{{BzMessen}}** measures on the model: click "
        "two points, or drag from one to the other. Like 3ds Max the same tool is in Search all actions and in the context menu as **{{BzMessenMax}}**. Each control mode keeps its own arrangement of the windows: "
        "what you move, hide or take out like Blender stays as it is when you switch to 3ds Max and back. Selection, sequence and frame stay when you switch.",
        "N öffnet die **{{BzSeitenleiste}}** rechts in der Ansicht. Der Reiter **{{BzReiterObjekt}}** zeigt die Auswahl: bei einer Fläche Position und Abmessungen, bei einem Knochen oder einem Griff des Rigs Position "
        "und Drehung in der Pose, dazu die letzte Messung. Wie in Blender sind die Werte Zahlenfelder: nach links oder rechts ziehen ändert sie, ein Klick tippt einen Wert ein (Enter übernimmt, "
        "Esc bricht ab); Strg+Z nimmt eine Eingabe in einem Schritt zurück. Eine Fläche wandert bzw. skaliert damit, ein Griff des Rigs im Bereich Animation (Posenmodus) bewegt und dreht sich, so weit er "
        "es mit dem Gizmo kann; ein Knochen außerhalb des Posenmodus steht wie in Blender nur zum Ablesen da. Der Reiter **{{BzReiterAnsicht}}** stellt das Sichtfeld ein und wechselt zwischen Perspektive und orthographisch. Das Werkzeug **{{BzMessen}}** misst auf dem "
        "Modell: zwei Punkte anklicken oder von einem zum anderen ziehen. Wie 3ds Max steht dasselbe Werkzeug in Alle Aktionen suchen und im Rechtsklick als **{{BzMessenMax}}**. Jeder Bedienmodus behält seine eigene "
        "Anordnung der Fenster: was man wie Blender verschiebt, ausblendet oder herausnimmt, bleibt so, wenn man zu 3ds Max und zurück wechselt. Auswahl, Sequenz und Bild bleiben beim Wechsel erhalten.",
        "按 N 在视图右侧打开 **{{BzSeitenleiste}}**。**{{BzReiterObjekt}}** 选项卡显示所选内容: 面为位置和尺寸, 骨骼或绑定控制器为姿势中的位置和旋转, 另外还有最近一次测量。与 Blender 一样, 这些值是数字字段: "
        "左右拖动可更改, 单击可输入数值 (Enter 确认, Esc 取消); Ctrl+Z 一步撤销一次输入。面会随之移动或缩放, 动画区域 (姿态模式) 中的绑定控制器会在变换控件允许的范围内移动和旋转; 姿态模式之外的骨骼与 Blender 一样仅供查看。**{{BzReiterAnsicht}}** 选项卡设置视野, "
        "并在透视和正交之间切换。**{{BzMessen}}** 工具在模型上测量: 单击两个点, 或从一点拖到另一点。像 3ds Max 时, 同一工具以 **{{BzMessenMax}}** 出现在“搜索所有操作”和右键菜单中。每种操作模式都保留自己的窗口布局: "
        "在 Blender 模式下移动、隐藏或取出的内容, 切换到 3ds Max 再切回后保持不变。切换时所选内容、序列和帧保持不变。",
        "N でビューの右に **{{BzSeitenleiste}}** が開きます。**{{BzReiterObjekt}}** タブは選択を示します: サーフェスなら位置と寸法、ボーンやリグのコントロールならポーズでの位置と回転、さらに最後の測定。"
        "Blender と同じく値は数値フィールドです: 左右にドラッグで変更、クリックで値を入力します (Enter で確定、Esc で取り消し)。Ctrl+Z で 1 回の入力を 1 段階で元に戻します。サーフェスはそれに合わせて移動・拡大縮小し、"
        "アニメーションエリア (ポーズモード) のリグのコントロールはギズモでできる範囲で移動・回転します。ポーズモード以外のボーンは Blender と同じく表示のみです。"
        "**{{BzReiterAnsicht}}** タブで視野角を設定し、透視と平行投影を切り替えます。**{{BzMessen}}** ツールはモデル上で測定します: 2 点をクリックするか、一方から他方へドラッグします。3ds Max 風では同じツールが "
        "**{{BzMessenMax}}** として「すべてのアクションを検索」と右クリックメニューにあります。各操作モードはウィンドウの配置をそれぞれ保持します: Blender 風で移動・非表示・切り出したものは、3ds Max に切り替えて戻ってもそのままです。"
        "切り替えても選択、シーケンス、フレームは保持されます。",
        "N abre la **{{BzSeitenleiste}}** a la derecha de la vista. La pestaña **{{BzReiterObjekt}}** muestra la selección: para una superficie, su posición y dimensiones; para un hueso o un control del rig, su "
        "ubicación y rotación en la pose; además, la última medición. La pestaña **{{BzReiterAnsicht}}** ajusta el campo de visión y cambia entre perspectiva y ortográfica. La herramienta **{{BzMessen}}** mide sobre el "
        "modelo: haz clic en dos puntos o arrastra de uno a otro. Como en Blender, los valores de la pestaña son campos numéricos: arrastra a izquierda o derecha para cambiarlos, haz clic para escribir un "
        "valor (Enter lo acepta, Esc cancela); Ctrl+Z deshace una entrada en un paso. Una superficie se mueve o escala con ellos, un control del rig en el área Animación (modo pose) se mueve y gira "
        "hasta donde puede con el gizmo; un hueso fuera del modo pose solo se muestra, como en Blender. Como 3ds Max, la misma herramienta está en Buscar todas las acciones y en el menú contextual como **{{BzMessenMax}}**. Cada modo de control conserva su "
        "propia disposición de ventanas: lo que mueves, ocultas o sacas como Blender sigue igual al cambiar a 3ds Max y volver. La selección, la secuencia y el fotograma se mantienen al cambiar.",
        "N otevře **{{BzSeitenleiste}}** vpravo v pohledu. Karta **{{BzReiterObjekt}}** ukazuje výběr: u plochy polohu a rozměry, u kosti nebo ovladače rigu umístění a rotaci v póze a k tomu poslední měření. "
        "Jako v Blenderu jsou hodnoty číselná pole: tažením doleva nebo doprava je změníte, kliknutím napíšete hodnotu (Enter potvrdí, Esc zruší); Ctrl+Z vrátí jedno zadání v jednom kroku. Plocha se jimi "
        "posune nebo změní velikost, ovladač rigu v oblasti Animace (režim pózy) se posune a otočí, kam to umí gizmem; kost mimo režim pózy je jako v Blenderu jen ke čtení. "
        "Karta **{{BzReiterAnsicht}}** nastaví zorné pole a přepíná mezi perspektivou a ortografickým pohledem. Nástroj **{{BzMessen}}** měří na modelu: klikněte na dva body nebo táhněte z jednoho do druhého. "
        "Jako 3ds Max je stejný nástroj ve Vyhledat všechny akce a v místní nabídce jako **{{BzMessenMax}}**. Každý režim ovládání si drží vlastní rozvržení oken: co jako Blender posunete, skryjete nebo vyjmete, "
        "zůstane tak i po přepnutí na 3ds Max a zpět. Výběr, sekvence a snímek se při přepnutí zachovají."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    z = X[schluessel]
    return T(z[0], z[1], z[2], z[3])


def seite():
    """Die Seite modell-blender (Gruppe Missionen, nach modell-bedienung bzw. modell-zeitleiste)."""
    return {
        "id": "modell-blender",
        "gruppe": "mission",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("img", "h_ap4_max", _t("bild_max"), "breit"),
            ("img", "h_ap4_blender", _t("bild_blender"), "breit"),
            ("h2", "werkzeugleiste", _t("h_leiste")),
            ("p", _t("leiste")),
            ("img", "h_ap4_gruppen", _t("bild_gruppen"), "breit"),
            ("h2", "werkzeuggruppen", _t("h_gruppen")),
            ("p", _t("gruppen")),
            ("h2", "modus", _t("h_kopf")),
            ("p", _t("kopf")),
            ("img", "h_ap4_seitenleiste", _t("bild_seite"), "breit"),
            ("h2", "seitenleiste", _t("h_seite")),
            ("p", _t("seite")),
        ],
    }
