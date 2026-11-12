"""AP8 (Bereich Weights nach dem Entwurf): der Abschnitt "Model-Tab: Weights" - das rechte Feld wie 3ds Max und wie Blender,
die Werkzeuge (Gewichts-Werkzeug, Pinsel mit Verlauf und Verwischen, Bereinigen), Spiegeln, Sperren, JKA-Grenzen
(docs/intern/NOTIZEN_ap8_1111.md).

Eigene Quelle des Pakets; AP14 haengt die Seite ins Handbuch (inhalt.py, inhalt_es.py, inhalt_cs.py). Die Bilder h_ap8_max,
h_ap8_spiegeln und h_ap8_blender nimmt der Selbsttest auf (BHED_EDITORTEST=handbuch, BHED_HB_NUR=ap8). Jeder Text einmal in
allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_ap12.py; {{Kennung}} setzt den Text der Oberflaeche ein.
"""

X = {
    "titel": ("Model tab: Weights", "Model-Tab: Weights", "模型选项卡: Weights", "モデルタブ: Weights", "Pestaña Model: Weights", "Karta Model: Weights"),
    "kurz": (
        "Paint and edit the bone weights of a model while the animation keeps running - with the tools of 3ds Max or of Blender, depending on the control mode. Mirror one side to the other across surfaces, lock bones, and see at any time whether the model fits the limits of the game.",
        "Die Knochengewichte eines Modells malen und bearbeiten, während die Animation weiterläuft - mit den Werkzeugen von 3ds Max oder von Blender, je nach Bedienmodus. Eine Seite über Flächen hinweg auf die andere spiegeln, Knochen sperren und jederzeit sehen, ob das Modell in die Grenzen des Spiels passt.",
        "在动画继续播放的同时绘制和编辑模型的骨骼权重 - 根据操作模式使用 3ds Max 或 Blender 的工具。可跨表面将一侧镜像到另一侧、锁定骨骼, 并随时查看模型是否符合游戏的限制。",
        "アニメーションを再生したまま、モデルのボーンウェイトをペイント・編集します。操作モードに応じて 3ds Max または Blender の道具を使います。サーフェスをまたいで片側をもう一方にミラーし、ボーンをロックし、モデルがゲームの制限に収まるかをいつでも確認できます。",
        "Pinta y edita los pesos de hueso de un modelo mientras la animación sigue en marcha, con las herramientas de 3ds Max o de Blender según el modo de control. Refleja un lado en el otro a través de las superficies, bloquea huesos y comprueba en todo momento si el modelo cabe en los límites del juego.",
        "Malujte a upravujte váhy kostí modelu, zatímco animace běží dál - nástroji 3ds Max nebo Blenderu podle režimu ovládání. Zrcadlete jednu stranu na druhou i přes povrchy, zamykejte kosti a kdykoli uvidíte, zda se model vejde do mezí hry."),
    "h_aufbau": ("The panel on the right", "Das Feld rechts", "右侧面板", "右のパネル", "El panel de la derecha", "Panel vpravo"),
    "aufbau": (
        "Pick a bone in the scene explorer or in the group **{{MgRKnochen}}**; the model then shows its weights from blue (0) to red (1), and the bones are visible. "
        "Like 3ds Max the command panel shows on the Modify tab the groups **{{MgRStapel}}**, **{{MgRWerkzeug}}**, **{{MgRSpiegeln}}**, **{{AsGruppe}}**, **{{MgRKnochen}}**, "
        "**{{MgRGrenzen}}** and, closed, **{{MgRErweitert}}**. Like Blender the properties editor shows on the Data tab **{{MgRPinsel}}**, **{{MgROptionen}}**, **{{MgRBereinigen}}**, "
        "**{{MgRSpiegeln}}**, **{{AsGruppe}}**, **{{MgRKnochen}}** and **{{MgRGrenzen}}**. Every function is also in the search of all actions (X, F3 like Blender) and in the "
        "right-click of the view under **{{MgMenue}}**.",
        "Einen Knochen im Szenen-Explorer oder in der Gruppe **{{MgRKnochen}}** wählen; das Modell zeigt dann seine Gewichte von Blau (0) bis Rot (1), und die Knochen sind zu sehen. "
        "Wie 3ds Max zeigt das Befehlsfeld im Reiter Ändern die Gruppen **{{MgRStapel}}**, **{{MgRWerkzeug}}**, **{{MgRSpiegeln}}**, **{{AsGruppe}}**, **{{MgRKnochen}}**, "
        "**{{MgRGrenzen}}** und zugeklappt **{{MgRErweitert}}**. Wie Blender zeigt der Eigenschaften-Editor im Reiter Daten **{{MgRPinsel}}**, **{{MgROptionen}}**, **{{MgRBereinigen}}**, "
        "**{{MgRSpiegeln}}**, **{{AsGruppe}}**, **{{MgRKnochen}}** und **{{MgRGrenzen}}**. Jede Funktion steht auch in der Suche aller Aktionen (X, wie Blender F3) und im "
        "Rechtsklick der Ansicht unter **{{MgMenue}}**.",
        "在场景资源管理器或 **{{MgRKnochen}}** 组中选择一根骨骼; 模型随即以蓝色 (0) 到红色 (1) 显示其权重, 骨骼可见。与 3ds Max 相同, 命令面板在 \"修改\" 选项卡中显示 "
        "**{{MgRStapel}}**、**{{MgRWerkzeug}}**、**{{MgRSpiegeln}}**、**{{AsGruppe}}**、**{{MgRKnochen}}**、**{{MgRGrenzen}}** 以及折叠的 **{{MgRErweitert}}**。与 Blender 相同, 属性编辑器在 "
        "\"数据\" 选项卡中显示 **{{MgRPinsel}}**、**{{MgROptionen}}**、**{{MgRBereinigen}}**、**{{MgRSpiegeln}}**、**{{AsGruppe}}**、**{{MgRKnochen}}** 和 **{{MgRGrenzen}}**。每个功能也都在全部动作的"
        "搜索 (X, Blender 模式为 F3) 以及视图右键的 **{{MgMenue}}** 之下。",
        "シーンエクスプローラまたはグループ **{{MgRKnochen}}** でボーンを選びます。モデルはそのウェイトを青 (0) から赤 (1) で表示し、ボーンが見えるようになります。3ds Max 風では、コマンドパネルの"
        "修正タブに **{{MgRStapel}}**、**{{MgRWerkzeug}}**、**{{MgRSpiegeln}}**、**{{AsGruppe}}**、**{{MgRKnochen}}**、**{{MgRGrenzen}}** と、閉じた状態の **{{MgRErweitert}}** が並びます。Blender 風では、"
        "プロパティエディタのデータタブに **{{MgRPinsel}}**、**{{MgROptionen}}**、**{{MgRBereinigen}}**、**{{MgRSpiegeln}}**、**{{AsGruppe}}**、**{{MgRKnochen}}**、**{{MgRGrenzen}}** が並びます。どの機能も、"
        "すべてのアクションの検索 (X、Blender 風は F3) とビューの右クリックの **{{MgMenue}}** にもあります。",
        "Elige un hueso en el explorador de escena o en el grupo **{{MgRKnochen}}**; el modelo muestra entonces sus pesos de azul (0) a rojo (1) y los huesos se ven. Como 3ds Max, "
        "el panel de comandos muestra en la pestaña Modificar los grupos **{{MgRStapel}}**, **{{MgRWerkzeug}}**, **{{MgRSpiegeln}}**, **{{AsGruppe}}**, **{{MgRKnochen}}**, "
        "**{{MgRGrenzen}}** y, cerrado, **{{MgRErweitert}}**. Como Blender, el editor de propiedades muestra en la pestaña Datos **{{MgRPinsel}}**, **{{MgROptionen}}**, "
        "**{{MgRBereinigen}}**, **{{MgRSpiegeln}}**, **{{AsGruppe}}**, **{{MgRKnochen}}** y **{{MgRGrenzen}}**. Cada función está también en la búsqueda de todas las acciones "
        "(X, F3 como Blender) y en el clic derecho de la vista, bajo **{{MgMenue}}**.",
        "Vyberte kost v průzkumníku scény nebo ve skupině **{{MgRKnochen}}**; model pak ukáže její váhy od modré (0) po červenou (1) a kosti jsou vidět. Jako 3ds Max ukazuje "
        "příkazový panel na kartě Změnit skupiny **{{MgRStapel}}**, **{{MgRWerkzeug}}**, **{{MgRSpiegeln}}**, **{{AsGruppe}}**, **{{MgRKnochen}}**, **{{MgRGrenzen}}** a zavřenou "
        "**{{MgRErweitert}}**. Jako Blender ukazuje editor vlastností na kartě Data **{{MgRPinsel}}**, **{{MgROptionen}}**, **{{MgRBereinigen}}**, **{{MgRSpiegeln}}**, "
        "**{{AsGruppe}}**, **{{MgRKnochen}}** a **{{MgRGrenzen}}**. Každá funkce je také ve vyhledávání všech akcí (X, jako Blender F3) a v pravém kliknutí pohledu pod **{{MgMenue}}**."),
    "h_malen": ("Painting", "Malen", "绘制", "ペイント", "Pintar", "Malování"),
    "malen": (
        "The main button **{{MgMalenMax}}** (like Blender: **{{MgMalenBl}}**) switches painting on and off. On, the left mouse button paints the weight of the picked bone; off, it selects - "
        "vertices at vertex level (1) - for the tools that work on a selection. The brush adds, subtracts, smooths or sets the weight; like Blender there are also **{{MgVerlauf}}** "
        "(drag a line: full weight at its start, nothing at its end, linear or radial) and **{{MgVerwischen}}** (drags the weights along with the brush). Like 3ds Max these and "
        "radius, strength and mirroring while painting are under **{{MgMalOptionen}}**. With **{{MgAutoNorm}}** (like 3ds Max: **{{MgNormMax}}**, on from the start) the weights of a "
        "vertex always add up to 1: the painted bone keeps its value and the others share the rest. One stroke is one undo step.",
        "Der Hauptknopf **{{MgMalenMax}}** (wie Blender: **{{MgMalenBl}}**) schaltet das Malen an und aus. An malt die linke Maustaste das Gewicht des gewählten Knochens; aus wählt sie "
        "aus - Ecken auf der Ebene Ecke (1) - für die Werkzeuge, die auf einer Auswahl arbeiten. Der Pinsel fügt hinzu, zieht ab, glättet oder setzt das Gewicht; wie Blender gibt es "
        "dazu **{{MgVerlauf}}** (eine Strecke ziehen: an ihrem Anfang das ganze Gewicht, an ihrem Ende nichts, linear oder radial) und **{{MgVerwischen}}** (zieht die Gewichte mit dem "
        "Pinsel mit). Wie 3ds Max stehen diese sowie Radius, Stärke und das Spiegeln beim Malen unter **{{MgMalOptionen}}**. Mit **{{MgAutoNorm}}** (wie 3ds Max: **{{MgNormMax}}**, ab "
        "Werk an) ergeben die Gewichte einer Ecke immer 1: der gemalte Knochen behält seinen Wert, die übrigen teilen sich den Rest. Ein Strich ist ein Rückgängig-Schritt.",
        "主按钮 **{{MgMalenMax}}** (Blender 模式: **{{MgMalenBl}}**) 用于开关绘制。开启时, 鼠标左键绘制所选骨骼的权重; 关闭时左键用于选择 - 在顶点层级 (1) 选择顶点 - 供作用于选择的工具使用。"
        "笔刷可添加、减去、平滑或设置权重; 与 Blender 相同, 还有 **{{MgVerlauf}}** (拖出一条线: 起点为完整权重, 终点为零, 线性或径向) 和 **{{MgVerwischen}}** (随笔刷拖动权重)。与 3ds Max 相同, "
        "这些以及半径、强度和绘制时镜像位于 **{{MgMalOptionen}}** 之下。启用 **{{MgAutoNorm}}** (3ds Max 模式: **{{MgNormMax}}**, 默认开启) 时, 顶点的权重总和始终为 1: 所绘制的骨骼保持其值, "
        "其余骨骼分配剩余部分。一笔为一个撤销步骤。",
        "メインボタン **{{MgMalenMax}}** (Blender 風: **{{MgMalenBl}}**) でペイントをオン・オフします。オンでは左ボタンで選択したボーンのウェイトをペイントし、オフでは左ボタンが選択になります "
        "(頂点レベル (1) で頂点を選択)。選択に働く道具のためです。ブラシは加算・減算・スムーズ・設定を行います。Blender と同じく **{{MgVerlauf}}** (線をドラッグ: 始点はウェイト全体、終点はゼロ、"
        "リニアまたは放射状) と **{{MgVerwischen}}** (ブラシの動きに合わせてウェイトを引きずる) もあります。3ds Max 風では、これらと半径・強さ・ペイント時のミラーは **{{MgMalOptionen}}** の下にあります。"
        "**{{MgAutoNorm}}** (3ds Max 風: **{{MgNormMax}}**、最初からオン) では、頂点のウェイトの合計が常に 1 になります。ペイントしたボーンは値を保ち、ほかのボーンが残りを分け合います。"
        "1 ストロークが元に戻す 1 回分です。",
        "El botón principal **{{MgMalenMax}}** (como Blender: **{{MgMalenBl}}**) activa y desactiva la pintura. Activado, el botón izquierdo pinta el peso del hueso elegido; desactivado, "
        "selecciona - vértices en el nivel de vértice (1) - para las herramientas que trabajan sobre una selección. El pincel añade, resta, suaviza o fija el peso; como en Blender hay además "
        "**{{MgVerlauf}}** (arrastre una línea: peso completo al inicio, nada al final, lineal o radial) y **{{MgVerwischen}}** (arrastra los pesos con el pincel). Como en 3ds Max, estos y "
        "el radio, la intensidad y el reflejo al pintar están bajo **{{MgMalOptionen}}**. Con **{{MgAutoNorm}}** (como 3ds Max: **{{MgNormMax}}**, activado de fábrica) los pesos de un vértice "
        "suman siempre 1: el hueso pintado conserva su valor y los demás se reparten el resto. Un trazo es un paso de deshacer.",
        "Hlavní tlačítko **{{MgMalenMax}}** (jako Blender: **{{MgMalenBl}}**) zapíná a vypíná malování. Zapnuté: levé tlačítko maluje váhu vybrané kosti; vypnuté: vybírá - vrcholy na "
        "úrovni vrcholů (1) - pro nástroje, které pracují s výběrem. Štětec přidává, odečítá, vyhlazuje nebo nastavuje váhu; jako v Blenderu je tu i **{{MgVerlauf}}** (táhněte čáru: na "
        "začátku plná váha, na konci nic, lineárně nebo radiálně) a **{{MgVerwischen}}** (táhne váhy se štětcem). Jako ve 3ds Max jsou tyto a poloměr, síla a zrcadlení při malování pod "
        "**{{MgMalOptionen}}**. S **{{MgAutoNorm}}** (jako 3ds Max: **{{MgNormMax}}**, od začátku zapnuto) dávají váhy vrcholu vždy 1: malovaná kost si hodnotu ponechá a ostatní si rozdělí "
        "zbytek. Jeden tah je jeden krok zpět."),
    "h_werkzeug": ("Weight tool and clean-up", "Gewichts-Werkzeug und Bereinigen", "权重工具与清理", "ウェイトツールとクリーンアップ", "Herramienta de pesos y limpieza",
                   "Nástroj vah a vyčištění"),
    "werkzeug": (
        "Like the Weight Tool of 3ds Max: with painting off, the fixed values 0 to 1 set the weight of the picked bone on the selected vertices at once; **{{MgSkalieren}}** multiplies it "
        "by the scale factor (0.95 from the start), **{{MgMischen}}** evens the selected vertices out with their neighbours, and **{{MgRing}}**, **{{MgSchleife}}**, **{{MgWachsen}}** and "
        "**{{MgSchrumpfen}}** extend or reduce the selection (ring and loop run over quads; a mesh of triangles is paired into quads for that without changing the model). The clean-up "
        "commands of Blender - **{{MgNormAlle}}**, **{{MgSaeubern}}**, smoothing all bones and **{{MgBegrenzen}}** - act on every vertex, or on the selected ones when vertices are "
        "selected; like 3ds Max they are in **{{MgRErweitert}}** as **{{MgNormAlle}}**, **{{MgSaeubernMax}}** and **{{MgBegrenzenMax}}**. Vertices at the same place - at a UV seam or "
        "where two surfaces meet - always get the same weights, so the mesh does not tear when it moves.",
        "Wie das Weight Tool von 3ds Max: ist das Malen aus, setzen die Festwerte 0 bis 1 das Gewicht des gewählten Knochens sofort auf die gewählten Ecken; **{{MgSkalieren}}** "
        "multipliziert es mit dem Skalierfaktor (ab Werk 0,95), **{{MgMischen}}** gleicht die gewählten Ecken an ihre Nachbarn an, und **{{MgRing}}**, **{{MgSchleife}}**, **{{MgWachsen}}** "
        "und **{{MgSchrumpfen}}** erweitern oder verkleinern die Auswahl (Ring und Schleife laufen über Vierecke; ein Netz aus Dreiecken wird dafür zu Vierecken gepaart, ohne das Modell zu "
        "ändern). Die Bereinigen-Befehle von Blender - **{{MgNormAlle}}**, **{{MgSaeubern}}**, das Glätten aller Knochen und **{{MgBegrenzen}}** - gelten jeder Ecke, oder den gewählten, "
        "wenn Ecken gewählt sind; wie 3ds Max stehen sie in **{{MgRErweitert}}** als **{{MgNormAlle}}**, **{{MgSaeubernMax}}** und **{{MgBegrenzenMax}}**. Ecken an derselben Stelle - an "
        "einer UV-Naht oder wo zwei Flächen aneinanderstoßen - bekommen immer dieselben Gewichte, damit das Netz in der Bewegung nicht reißt.",
        "与 3ds Max 的 Weight Tool 相同: 关闭绘制时, 固定值 0 到 1 立即把所选骨骼的权重设置到所选顶点上; **{{MgSkalieren}}** 将其乘以缩放系数 (默认 0.95), **{{MgMischen}}** 使所选顶点与相邻顶点"
        "趋于一致, **{{MgRing}}**、**{{MgSchleife}}**、**{{MgWachsen}}** 和 **{{MgSchrumpfen}}** 扩大或缩小选择 (环形和循环沿四边形进行; 三角形网格会为此配对成四边形, 不改变模型)。Blender 的清理命令 - "
        "**{{MgNormAlle}}**、**{{MgSaeubern}}**、平滑所有骨骼以及 **{{MgBegrenzen}}** - 作用于每个顶点, 选择了顶点时只作用于所选顶点; 3ds Max 模式下它们位于 **{{MgRErweitert}}** 中, 名为 "
        "**{{MgNormAlle}}**、**{{MgSaeubernMax}}** 和 **{{MgBegrenzenMax}}**。位于同一位置的顶点 - 在 UV 接缝处或两个表面相接处 - 始终得到相同的权重, 这样网格在运动时不会撕裂。",
        "3ds Max の Weight Tool と同じです。ペイントがオフのとき、固定値 0～1 は選択したボーンのウェイトを選択した頂点にすぐ設定します。**{{MgSkalieren}}** はスケール係数 (最初は 0.95) を掛け、"
        "**{{MgMischen}}** は選択した頂点を隣の頂点になじませ、**{{MgRing}}**、**{{MgSchleife}}**、**{{MgWachsen}}**、**{{MgSchrumpfen}}** は選択を広げたり狭めたりします (リングとループは四角形の上を"
        "進みます。三角形のメッシュはそのために四角形に組み合わせますが、モデルは変更しません)。Blender のクリーンアップコマンド - **{{MgNormAlle}}**、**{{MgSaeubern}}**、全ボーンのスムーズ、"
        "**{{MgBegrenzen}}** - はすべての頂点に、頂点を選択しているときは選択した頂点に働きます。3ds Max 風では **{{MgRErweitert}}** に **{{MgNormAlle}}**、**{{MgSaeubernMax}}**、"
        "**{{MgBegrenzenMax}}** として並びます。同じ位置にある頂点 (UV の継ぎ目や 2 つのサーフェスの境目) は常に同じウェイトになるので、動かしてもメッシュが裂けません。",
        "Como el Weight Tool de 3ds Max: con la pintura desactivada, los valores fijos de 0 a 1 fijan de inmediato el peso del hueso elegido en los vértices seleccionados; "
        "**{{MgSkalieren}}** lo multiplica por el factor de escala (0,95 de fábrica), **{{MgMischen}}** iguala los vértices seleccionados con sus vecinos, y **{{MgRing}}**, "
        "**{{MgSchleife}}**, **{{MgWachsen}}** y **{{MgSchrumpfen}}** amplían o reducen la selección (anillo y bucle recorren cuadriláteros; una malla de triángulos se empareja en "
        "cuadriláteros para ello sin cambiar el modelo). Las órdenes de limpieza de Blender - **{{MgNormAlle}}**, **{{MgSaeubern}}**, suavizar todos los huesos y **{{MgBegrenzen}}** - "
        "actúan sobre cada vértice, o sobre los seleccionados si hay vértices seleccionados; como en 3ds Max están en **{{MgRErweitert}}** como **{{MgNormAlle}}**, **{{MgSaeubernMax}}** "
        "y **{{MgBegrenzenMax}}**. Los vértices en el mismo lugar - en una costura UV o donde se tocan dos superficies - reciben siempre los mismos pesos, para que la malla no se rasgue "
        "al moverse.",
        "Jako Weight Tool ve 3ds Max: při vypnutém malování nastaví pevné hodnoty 0 až 1 váhu vybrané kosti hned na vybrané vrcholy; **{{MgSkalieren}}** ji vynásobí měřítkem (výchozí "
        "0,95), **{{MgMischen}}** srovná vybrané vrcholy se sousedy a **{{MgRing}}**, **{{MgSchleife}}**, **{{MgWachsen}}** a **{{MgSchrumpfen}}** výběr rozšíří nebo zmenší (prstenec a "
        "smyčka běží přes čtyřúhelníky; síť z trojúhelníků se pro to spáruje do čtyřúhelníků, aniž by se model změnil). Čisticí příkazy Blenderu - **{{MgNormAlle}}**, **{{MgSaeubern}}**, "
        "vyhlazení všech kostí a **{{MgBegrenzen}}** - platí pro každý vrchol, nebo pro vybrané, jsou-li vrcholy vybrané; jako ve 3ds Max jsou v **{{MgRErweitert}}** jako "
        "**{{MgNormAlle}}**, **{{MgSaeubernMax}}** a **{{MgBegrenzenMax}}**. Vrcholy na stejném místě - na UV švu nebo kde se stýkají dva povrchy - dostanou vždy stejné váhy, aby se síť "
        "při pohybu netrhala."),
    "h_spiegeln": ("Mirroring", "Spiegeln", "镜像", "ミラー", "Reflejar", "Zrcadlení"),
    "spiegeln": (
        "Bones are paired by their names (lhumerus and rhumerus), vertices by their place, across surfaces - in JKA the left and the right arm are surfaces of their own. The table of "
        "the group shows the bone pairs, the vertices without a counterpart and the surfaces that mirror each other; **{{MgToleranz}}** says how far a counterpart may lie from the "
        "mirrored place. Like 3ds Max, **{{MgSpiegelModus}}** marks the vertices in the view (yellow: no counterpart), **{{MgBlauGruen}}** and **{{MgGruenBlau}}** copy one side to the "
        "other, **{{MgTauschen}}** swaps them, and **{{MgNurAuswahl}}** limits all three to the selected vertices. Like Blender, **{{MgGruppeSpiegeln}}** mirrors the picked bone and "
        "**{{MgAlleGruppen}}** all bones, with **{{MgBlGewichte}}** and **{{MgBlNamen}}**: without a vertex selection both sides swap, with one the selected side receives the other. "
        "**{{MgBlTopologie}}** finds the counterparts by the structure of the mesh when its halves are built alike but do not lie exactly alike. After mirroring the message names a "
        "surface that now uses more bones than the game reads (32).",
        "Knochen werden über ihre Namen gepaart (lhumerus und rhumerus), Ecken über ihre Lage, über Flächen hinweg - in JKA sind linker und rechter Arm eigene Flächen. Die Tabelle der "
        "Gruppe zeigt die Knochenpaare, die Ecken ohne Gegenstück und die Flächen, die einander spiegeln; **{{MgToleranz}}** sagt, wie weit ein Gegenstück vom gespiegelten Ort liegen "
        "darf. Wie 3ds Max markiert **{{MgSpiegelModus}}** die Ecken in der Ansicht (gelb: kein Gegenstück), **{{MgBlauGruen}}** und **{{MgGruenBlau}}** kopieren eine Seite auf die "
        "andere, **{{MgTauschen}}** tauscht sie, und **{{MgNurAuswahl}}** beschränkt alle drei auf die gewählten Ecken. Wie Blender spiegelt **{{MgGruppeSpiegeln}}** den gewählten "
        "Knochen und **{{MgAlleGruppen}}** alle Knochen, mit **{{MgBlGewichte}}** und **{{MgBlNamen}}**: ohne Auswahl von Ecken tauschen beide Seiten, mit einer bekommt die gewählte "
        "Seite die andere. **{{MgBlTopologie}}** findet die Gegenstücke über den Aufbau des Netzes, wenn seine Hälften gleich gebaut sind, aber nicht genau gleich liegen. Nach dem "
        "Spiegeln nennt die Meldung eine Fläche, die jetzt mehr Knochen benutzt, als das Spiel liest (32).",
        "骨骼按名称配对 (lhumerus 与 rhumerus), 顶点按位置配对, 可跨表面 - 在 JKA 中左臂和右臂是各自独立的表面。该组的表格显示骨骼对、没有对应点的顶点以及互为镜像的表面; "
        "**{{MgToleranz}}** 表示对应点可以偏离镜像位置多远。与 3ds Max 相同, **{{MgSpiegelModus}}** 在视图中标记顶点 (黄色: 没有对应点), **{{MgBlauGruen}}** 和 **{{MgGruenBlau}}** 把一侧复制到"
        "另一侧, **{{MgTauschen}}** 交换两侧, **{{MgNurAuswahl}}** 把这三者限制在所选顶点上。与 Blender 相同, **{{MgGruppeSpiegeln}}** 镜像所选骨骼, **{{MgAlleGruppen}}** 镜像所有骨骼, 配合 "
        "**{{MgBlGewichte}}** 和 **{{MgBlNamen}}**: 没有顶点选择时两侧互换, 有选择时所选一侧得到另一侧的权重。当网格两半结构相同但位置不完全对称时, **{{MgBlTopologie}}** 按网格结构查找对应点。"
        "镜像之后, 消息会指出现在使用的骨骼数超过游戏读取数量 (32) 的表面。",
        "ボーンは名前で (lhumerus と rhumerus)、頂点は位置でペアにします。サーフェスをまたぎます - JKA では左腕と右腕は別々のサーフェスです。グループの表には、ボーンのペア、対のない頂点、"
        "互いにミラーになるサーフェスが表示されます。**{{MgToleranz}}** は、対になる頂点がミラー位置からどれだけ離れてよいかです。3ds Max 風では、**{{MgSpiegelModus}}** がビューで頂点に印を付け "
        "(黄色: 対がない)、**{{MgBlauGruen}}** と **{{MgGruenBlau}}** が片側をもう一方にコピーし、**{{MgTauschen}}** が左右を入れ替え、**{{MgNurAuswahl}}** がこの 3 つを選択した頂点だけに限ります。"
        "Blender 風では、**{{MgGruppeSpiegeln}}** が選択したボーンを、**{{MgAlleGruppen}}** がすべてのボーンをミラーします (**{{MgBlGewichte}}** と **{{MgBlNamen}}**)。頂点を選択していなければ左右が"
        "入れ替わり、選択していれば選択した側がもう一方を受け取ります。**{{MgBlTopologie}}** は、左右の作りは同じでも位置が正確に対称でないとき、メッシュの構造から対を探します。ミラーのあと、"
        "ゲームが読む数 (32) より多くのボーンを使うようになったサーフェスがあればメッセージに表示されます。",
        "Los huesos se emparejan por sus nombres (lhumerus y rhumerus), los vértices por su lugar, a través de las superficies: en JKA el brazo izquierdo y el derecho son superficies "
        "propias. La tabla del grupo muestra los pares de huesos, los vértices sin pareja y las superficies que se reflejan entre sí; **{{MgToleranz}}** indica a qué distancia del "
        "lugar reflejado puede estar la pareja. Como 3ds Max, **{{MgSpiegelModus}}** marca los vértices en la vista (amarillo: sin pareja), **{{MgBlauGruen}}** y **{{MgGruenBlau}}** "
        "copian un lado en el otro, **{{MgTauschen}}** los intercambia y **{{MgNurAuswahl}}** limita los tres a los vértices seleccionados. Como Blender, **{{MgGruppeSpiegeln}}** refleja "
        "el hueso elegido y **{{MgAlleGruppen}}** todos los huesos, con **{{MgBlGewichte}}** y **{{MgBlNamen}}**: sin selección de vértices se intercambian ambos lados, con una el lado "
        "seleccionado recibe el otro. **{{MgBlTopologie}}** busca las parejas por la estructura de la malla cuando sus mitades están construidas igual pero no colocadas exactamente "
        "igual. Tras reflejar, el mensaje nombra una superficie que ahora usa más huesos de los que lee el juego (32).",
        "Kosti se párují podle jmen (lhumerus a rhumerus), vrcholy podle polohy, i přes povrchy - v JKA jsou levá a pravá paže samostatné povrchy. Tabulka skupiny ukazuje páry kostí, "
        "vrcholy bez protějšku a povrchy, které se navzájem zrcadlí; **{{MgToleranz}}** říká, jak daleko od zrcadleného místa smí protějšek ležet. Jako 3ds Max označí "
        "**{{MgSpiegelModus}}** vrcholy v pohledu (žlutě: bez protějšku), **{{MgBlauGruen}}** a **{{MgGruenBlau}}** kopírují jednu stranu na druhou, **{{MgTauschen}}** je prohodí a "
        "**{{MgNurAuswahl}}** omezí všechny tři na vybrané vrcholy. Jako Blender zrcadlí **{{MgGruppeSpiegeln}}** vybranou kost a **{{MgAlleGruppen}}** všechny kosti, s "
        "**{{MgBlGewichte}}** a **{{MgBlNamen}}**: bez výběru vrcholů se obě strany prohodí, s výběrem dostane vybraná strana tu druhou. **{{MgBlTopologie}}** hledá protějšky podle "
        "stavby sítě, když jsou její poloviny stejně stavěné, ale neleží přesně stejně. Po zrcadlení zpráva jmenuje povrch, který teď používá víc kostí, než hra čte (32)."),
    "h_sperren": ("Bone, locks and the limits of the game", "Knochen, Sperren und die Grenzen des Spiels", "骨骼、锁定与游戏的限制", "ボーン、ロック、ゲームの制限",
                  "Hueso, bloqueos y los límites del juego", "Kost, zámky a meze hry"),
    "sperren": (
        "The group **{{MgRKnochen}}** shows the picked bone and the pose (sequence and frame); the animation keeps running while you work. **{{MgSperre}}** locks the picked bone: no "
        "brush and no command changes its weights, and normalising works around it. Locked bones are listed with an x to unlock them; **{{MgSperrMenue}}** (like Blender: K) opens the "
        "menu to lock or unlock all bones, the picked one or all others. **{{MgKnochenZeigen}}** switches the bones in the view; in this area they are on from the start. The group "
        "**{{MgRGrenzen}}** shows as bars what the game reads: vertices and bones per surface (of the selected surface, otherwise of the one closest to the limit) and the most weights "
        "on a vertex. **{{MbGwUebernehmen}}** writes the painted weights into the model as the game reads them: the four largest, normalised, in 10 bits.",
        "Die Gruppe **{{MgRKnochen}}** zeigt den gewählten Knochen und die Pose (Sequenz und Bild); die Animation läuft weiter, während man arbeitet. **{{MgSperre}}** sperrt den "
        "gewählten Knochen: kein Pinsel und kein Befehl ändert seine Gewichte, und das Normalisieren arbeitet um ihn herum. Gesperrte Knochen stehen als Liste mit einem x zum "
        "Entsperren da; **{{MgSperrMenue}}** (wie Blender: K) öffnet das Menü, das alle Knochen, den gewählten oder alle anderen sperrt oder entsperrt. **{{MgKnochenZeigen}}** schaltet "
        "die Knochen in der Ansicht; in diesem Bereich sind sie ab Werk an. Die Gruppe **{{MgRGrenzen}}** zeigt als Balken, was das Spiel liest: Ecken und Knochen je Fläche (der "
        "gewählten Fläche, sonst der, die der Grenze am nächsten kommt) und die meisten Gewichte an einer Ecke. **{{MbGwUebernehmen}}** schreibt die gemalten Gewichte so ins Modell, "
        "wie das Spiel sie liest: die vier größten, normiert, in 10 Bit.",
        "**{{MgRKnochen}}** 组显示所选骨骼和姿势 (序列和帧); 工作时动画继续播放。**{{MgSperre}}** 锁定所选骨骼: 任何笔刷和命令都不会更改其权重, 归一化会绕开它。锁定的骨骼以列表显示, 带有用于"
        "解锁的 x; **{{MgSperrMenue}}** (Blender 模式: K) 打开菜单, 可锁定或解锁所有骨骼、所选骨骼或其他所有骨骼。**{{MgKnochenZeigen}}** 开关视图中的骨骼; 在此区域默认开启。**{{MgRGrenzen}}** "
        "组以条形显示游戏读取的内容: 每个表面的顶点数和骨骼数 (所选表面, 否则为最接近限制的表面) 以及单个顶点上最多的权重数。**{{MbGwUebernehmen}}** 把绘制的权重按游戏读取的方式写入模型: "
        "最大的四个, 归一化, 10 位。",
        "グループ **{{MgRKnochen}}** には、選択したボーンとポーズ (シーケンスとフレーム) が表示されます。作業中もアニメーションは再生し続けます。**{{MgSperre}}** は選択したボーンをロックします。"
        "ブラシもコマンドもそのウェイトを変更せず、正規化はそのボーンを避けて働きます。ロックしたボーンは、解除用の x が付いたリストで表示されます。**{{MgSperrMenue}}** (Blender 風: K) は、"
        "すべてのボーン、選択したボーン、それ以外をロックまたは解除するメニューを開きます。**{{MgKnochenZeigen}}** はビューのボーン表示を切り替えます。このエリアでは最初からオンです。"
        "グループ **{{MgRGrenzen}}** は、ゲームが読む内容をバーで表示します: サーフェスごとの頂点とボーン (選択したサーフェス、なければ制限に最も近いサーフェス) と、1 頂点あたりの最大ウェイト数。"
        "**{{MbGwUebernehmen}}** は、ペイントしたウェイトをゲームが読む形でモデルに書き込みます: 大きい 4 つ、正規化、10 ビット。",
        "El grupo **{{MgRKnochen}}** muestra el hueso elegido y la pose (secuencia y fotograma); la animación sigue en marcha mientras se trabaja. **{{MgSperre}}** bloquea el hueso "
        "elegido: ningún pincel ni orden cambia sus pesos, y la normalización trabaja a su alrededor. Los huesos bloqueados aparecen en una lista con una x para desbloquearlos; "
        "**{{MgSperrMenue}}** (como Blender: K) abre el menú que bloquea o desbloquea todos los huesos, el elegido o todos los demás. **{{MgKnochenZeigen}}** activa los huesos en la "
        "vista; en esta área están activados de fábrica. El grupo **{{MgRGrenzen}}** muestra en barras lo que lee el juego: vértices y huesos por superficie (de la superficie "
        "seleccionada, o si no de la más cercana al límite) y el máximo de pesos en un vértice. **{{MbGwUebernehmen}}** escribe los pesos pintados en el modelo tal como los lee el "
        "juego: los cuatro mayores, normalizados, en 10 bits.",
        "Skupina **{{MgRKnochen}}** ukazuje vybranou kost a pózu (sekvenci a snímek); animace při práci běží dál. **{{MgSperre}}** zamkne vybranou kost: žádný štětec ani příkaz její "
        "váhy nezmění a normalizace ji obchází. Zamčené kosti jsou v seznamu s x pro odemknutí; **{{MgSperrMenue}}** (jako Blender: K) otevře nabídku, která zamkne nebo odemkne všechny "
        "kosti, vybranou nebo všechny ostatní. **{{MgKnochenZeigen}}** přepíná kosti v pohledu; v této oblasti jsou od začátku zapnuté. Skupina **{{MgRGrenzen}}** ukazuje pruhy, co hra "
        "čte: vrcholy a kosti na povrch (vybraného povrchu, jinak toho, který je mezi nejblíž) a nejvíce vah na jednom vrcholu. **{{MbGwUebernehmen}}** zapíše namalované váhy do modelu "
        "tak, jak je hra čte: čtyři největší, normalizované, v 10 bitech."),
    "bild_max": ("The Weights area like 3ds Max: command panel with modifier stack, weight tool, mirror, auto-skin, bone and JKA limits.",
                 "Der Bereich Weights wie 3ds Max: Befehlsfeld mit Modifier-Stapel, Gewichts-Werkzeug, Spiegeln, Auto-Skin, Knochen und JKA-Grenzen.",
                 "3ds Max 模式下的 Weights 区域: 命令面板包含修改器堆栈、权重工具、镜像、自动蒙皮、骨骼和 JKA 限制。",
                 "3ds Max 風の Weights エリア: モディファイアスタック、ウェイトツール、ミラー、オートスキン、ボーン、JKA の制限を持つコマンドパネル。",
                 "El área Weights como 3ds Max: panel de comandos con pila de modificadores, herramienta de pesos, reflejo, auto-skin, hueso y límites de JKA.",
                 "Oblast Weights jako 3ds Max: příkazový panel se zásobníkem modifikátorů, nástrojem vah, zrcadlením, auto-skinem, kostí a mezemi JKA."),
    "bild_spiegeln": ("Mirror mode: vertices on the left blue, on the right green, without a counterpart yellow.",
                      "Spiegelmodus: Ecken links blau, rechts grün, ohne Gegenstück gelb.", "镜像模式: 左侧顶点为蓝色, 右侧为绿色, 没有对应点的为黄色。",
                      "ミラーモード: 左の頂点は青、右は緑、対のない頂点は黄色。", "Modo espejo: vértices a la izquierda en azul, a la derecha en verde, sin pareja en amarillo.",
                      "Režim zrcadlení: vrcholy vlevo modře, vpravo zeleně, bez protějšku žlutě."),
    "bild_blender": ("The Weights area like Blender: properties editor with brush, options, clean up, mirror, auto-skin, bone and JKA limits.",
                     "Der Bereich Weights wie Blender: Eigenschaften-Editor mit Pinsel, Optionen, Bereinigen, Spiegeln, Auto-Skin, Knochen und JKA-Grenzen.",
                     "Blender 模式下的 Weights 区域: 属性编辑器包含笔刷、选项、清理、镜像、自动蒙皮、骨骼和 JKA 限制。",
                     "Blender 風の Weights エリア: ブラシ、オプション、クリーンアップ、ミラー、オートスキン、ボーン、JKA の制限を持つプロパティエディタ。",
                     "El área Weights como Blender: editor de propiedades con pincel, opciones, limpieza, reflejo, auto-skin, hueso y límites de JKA.",
                     "Oblast Weights jako Blender: editor vlastností se štětcem, možnostmi, vyčištěním, zrcadlením, auto-skinem, kostí a mezemi JKA."),
}


def _t(schluessel):
    return X[schluessel][:4]


def seite():
    """Die Seite modell-weights (Gruppe Model, vor Auto-Skin). Die Bilder h_ap8_* nimmt BHED_HB_NUR=ap8 auf; AP14 haengt ein."""
    return {
        "id": "modell-weights",
        "gruppe": "mission",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("h2", "aufbau", _t("h_aufbau")),
            ("p", _t("aufbau")),
            ("img", "h_ap8_max", _t("bild_max"), "breit"),
            ("img", "h_ap8_blender", _t("bild_blender"), "breit"),
            ("h2", "malen", _t("h_malen")),
            ("p", _t("malen")),
            ("h2", "werkzeug", _t("h_werkzeug")),
            ("p", _t("werkzeug")),
            ("h2", "spiegeln", _t("h_spiegeln")),
            ("p", _t("spiegeln")),
            ("img", "h_ap8_spiegeln", _t("bild_spiegeln"), "breit"),
            ("h2", "sperren", _t("h_sperren")),
            ("p", _t("sperren")),
        ],
    }


ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}
