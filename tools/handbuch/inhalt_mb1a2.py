"""Modell-Block 1a-2: die Seiten "Model-Tab: Weights" und "Model-Tab: Bedienung wie 3ds Max oder Blender" (mit dem
Fenster "Ins Skript" und dem Model-Tab ohne Skriptteile).

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_mb1a1.py.
"""

X = {
    "mr_blender": (
        "In Blender mode the keys of Blender's weight paint apply: a stroke with Ctrl subtracts, with Shift it smooths; Shift+X samples the weight under the mouse (**{{MrGwAufnehmen}}**), Ctrl+X sets it on the selection; F, Shift+F and Ctrl+F drag radius, strength and weight - a click confirms, Esc or a right-click cancels.",
        'Im Blender-Modus gelten die Tasten aus Blenders Weight Paint: ein Strich mit Strg zieht ab, mit Umschalt glättet er; Umschalt+X nimmt das Gewicht unter der Maus auf (**{{MrGwAufnehmen}}**), Strg+X setzt es auf die Auswahl; F, Umschalt+F und Strg+F ziehen Radius, Stärke und Gewicht - ein Klick übernimmt, Esc oder Rechtsklick bricht ab.',
        '在 Blender 模式下适用 Blender 权重绘制的按键: 按住 Ctrl 绘制为减去, 按住 Shift 为平滑; Shift+X 采样鼠标下的权重 (**{{MrGwAufnehmen}}**), Ctrl+X 把它设置到所选; F、Shift+F 和 Ctrl+F 拖动半径、强度和权重——单击确认, Esc 或右键取消。',
        'Blender 風では Blender のウェイトペイントのキーが使えます: Ctrl を押しながらのストロークは減算、Shift でスムーズ。Shift+X でマウスの下のウェイトを取得 (**{{MrGwAufnehmen}}**)、Ctrl+X で選択に設定。F、Shift+F、Ctrl+F で半径・強さ・ウェイトをドラッグし、クリックで確定、Esc か右クリックで取り消します。',
        'En modo Blender valen las teclas del weight paint de Blender: un trazo con Ctrl resta, con Mayús suaviza; Mayús+X muestrea el peso bajo el ratón (**{{MrGwAufnehmen}}**), Ctrl+X lo aplica a la selección; F, Mayús+F y Ctrl+F arrastran radio, intensidad y peso: un clic confirma, Esc o clic derecho cancela.',
        'V režimu Blender platí klávesy weight paint z Blenderu: tah s Ctrl odečítá, se Shift vyhlazuje; Shift+X odebere váhu pod myší (**{{MrGwAufnehmen}}**), Ctrl+X ji nastaví na výběr; F, Shift+F a Ctrl+F táhnou poloměr, sílu a váhu - klepnutí potvrdí, Esc nebo pravé tlačítko zruší.'),
    "w_titel": ("Model tab: Weights", "Model-Tab: Weights", "模型选项卡:权重", "モデルタブ: ウェイト", "Pestaña Model: Pesos", "Karta Model: Váhy"),
    "w_kurz": (
        "Paint the bone weights of a model while the animation runs - the pose shows at once what the game will do with them.",
        "Die Knochengewichte eines Modells malen, während die Animation läuft - die Pose zeigt sofort, was das Spiel daraus macht.",
        "在动画播放时绘制模型的骨骼权重——姿势会立即显示游戏将如何使用它们。",
        "アニメーションを再生したままモデルのボーンウェイトをペイントします。ポーズにはゲームでの結果がすぐ表示されます。",
        "Pinta los pesos de los huesos de un modelo mientras corre la animación: la pose muestra al instante lo que hará el juego con ellos.",
        "Malujte váhy kostí modelu, zatímco běží animace - póza hned ukáže, co s nimi udělá hra."),
    "w_bild": (
        "Weights on the stormtrooper: the right upper arm (rhumerus) from blue (0) to red (1), the brush circle over the arm, on the right the "
        "Weights rollout.",
        "Weights am Sturmtruppler: der rechte Oberarm (rhumerus) von Blau (0) bis Rot (1), der Pinselkreis über dem Arm, rechts das Rollout Gewichte.",
        "冲锋队员的权重:右上臂 (rhumerus) 从蓝色 (0) 到红色 (1),手臂上方是笔刷圆圈,右侧是权重卷展栏。",
        "ストームトルーパーのウェイト: 右上腕 (rhumerus) が青 (0) から赤 (1)、腕の上にブラシの円、右にウェイトのロールアウト。",
        "Pesos en el soldado de asalto: el brazo derecho (rhumerus) de azul (0) a rojo (1), el círculo del pincel sobre el brazo, a la derecha la persiana Pesos.",
        "Váhy na stormtrooperovi: pravá paže (rhumerus) od modré (0) po červenou (1), kruh štětce nad paží, vpravo roleta Váhy."),
    "w_text": (
        "Switch to **{{MbBereichWeights}}** and pick a bone in the list or in the scene explorer. The model shows its weights in the colours of "
        "Blender's weight paint. The left mouse button paints with **{{MbGwHinzu}}**, **{{MbGwAbziehen}}**, **{{MbGwGlaetten}}** or **{{MbGwSetzen}}**; "
        "radius, strength and weight are in the rollout, in 3ds Max mode also the fixed values of the Weight Tool (0, .1, .25, .5, .75, .9, 1). "
        "One stroke is one undo step (Ctrl+Z). **{{MbGwSpiegelMalen}}** paints the mirrored vertex on the mirrored bone (l_/r_, _l/_r, lhand/rhand); "
        "the buttons **{{MbGwSpiegelnPlus}}** and **{{MbGwSpiegelnMinus}}** mirror a whole side. In edit mode (vertices, edges, faces) "
        "**{{MbGwAufAuswahl}}** sets the weight on the selection.",
        "Wechsle zu **{{MbBereichWeights}}** und wähle einen Knochen in der Liste oder im Szenen-Explorer. Die Figur zeigt seine Gewichte in den "
        "Farben des Weight Paint von Blender. Die linke Maustaste malt mit **{{MbGwHinzu}}**, **{{MbGwAbziehen}}**, **{{MbGwGlaetten}}** oder "
        "**{{MbGwSetzen}}**; Radius, Stärke und Gewicht stehen im Rollout, im Modus wie 3ds Max auch die Festwerte des Weight Tools (0, .1, .25, .5, "
        ".75, .9, 1). Ein Strich ist ein Rückgängig-Schritt (Strg+Z). **{{MbGwSpiegelMalen}}** malt die gespiegelte Ecke auf den gespiegelten Knochen "
        "(l_/r_, _l/_r, lhand/rhand); die Knöpfe **{{MbGwSpiegelnPlus}}** und **{{MbGwSpiegelnMinus}}** spiegeln eine ganze Seite. Im "
        "Bearbeitungsmodus (Ecken, Kanten, Flächen) setzt **{{MbGwAufAuswahl}}** das Gewicht auf die Auswahl.",
        "切换到 **{{MbBereichWeights}}**,在列表或场景资源管理器中选择一根骨骼。模型以 Blender 权重绘制的颜色显示其权重。鼠标左键用 "
        "**{{MbGwHinzu}}**、**{{MbGwAbziehen}}**、**{{MbGwGlaetten}}** 或 **{{MbGwSetzen}}** 绘制;半径、强度和权重在卷展栏中,3ds Max 模式下还有"
        "权重工具的固定值 (0、.1、.25、.5、.75、.9、1)。一笔为一个撤销步骤 (Ctrl+Z)。**{{MbGwSpiegelMalen}}** 在镜像骨骼 (l_/r_、_l/_r、lhand/rhand) "
        "上绘制镜像顶点;**{{MbGwSpiegelnPlus}}** 和 **{{MbGwSpiegelnMinus}}** 按钮镜像整侧。在编辑模式 (顶点、边、面) 下,**{{MbGwAufAuswahl}}** 把权重设置到所选。",
        "**{{MbBereichWeights}}** に切り替え、リストかシーンエクスプローラーでボーンを選びます。モデルにそのウェイトが Blender のウェイトペイントの色で"
        "表示されます。左ボタンで **{{MbGwHinzu}}**、**{{MbGwAbziehen}}**、**{{MbGwGlaetten}}**、**{{MbGwSetzen}}** のいずれかでペイントします。半径・強さ・"
        "ウェイトはロールアウトにあり、3ds Max モードではウェイトツールの固定値 (0、.1、.25、.5、.75、.9、1) もあります。1 ストロークが元に戻す 1 回分 (Ctrl+Z) "
        "です。**{{MbGwSpiegelMalen}}** はミラー側の頂点をミラーのボーン (l_/r_、_l/_r、lhand/rhand) にペイントし、**{{MbGwSpiegelnPlus}}** と "
        "**{{MbGwSpiegelnMinus}}** は片側全体をミラーします。編集モード (頂点・辺・面) では **{{MbGwAufAuswahl}}** で選択にウェイトを設定します。",
        "Cambia a **{{MbBereichWeights}}** y elige un hueso en la lista o en el explorador de escena. El modelo muestra sus pesos con los colores de "
        "weight paint de Blender. El botón izquierdo pinta con **{{MbGwHinzu}}**, **{{MbGwAbziehen}}**, **{{MbGwGlaetten}}** o **{{MbGwSetzen}}**; "
        "radio, intensidad y peso están en la persiana, en modo 3ds Max también los valores fijos de la herramienta de pesos (0, .1, .25, .5, .75, .9, 1). "
        "Un trazo es un paso de deshacer (Ctrl+Z). **{{MbGwSpiegelMalen}}** pinta el vértice reflejado en el hueso reflejado (l_/r_, _l/_r, "
        "lhand/rhand); los botones **{{MbGwSpiegelnPlus}}** y **{{MbGwSpiegelnMinus}}** reflejan todo un lado. En modo edición (vértices, aristas, "
        "caras) **{{MbGwAufAuswahl}}** fija el peso en la selección.",
        "Přepněte na **{{MbBereichWeights}}** a vyberte kost v seznamu nebo v průzkumníku scény. Model ukáže její váhy v barvách weight paint z "
        "Blenderu. Levé tlačítko maluje s **{{MbGwHinzu}}**, **{{MbGwAbziehen}}**, **{{MbGwGlaetten}}** nebo **{{MbGwSetzen}}**; poloměr, síla a váha "
        "jsou v roletě, v režimu 3ds Max i pevné hodnoty nástroje vah (0, .1, .25, .5, .75, .9, 1). Jeden tah je jeden krok zpět (Ctrl+Z). "
        "**{{MbGwSpiegelMalen}}** maluje zrcadlový vrchol na zrcadlovou kost (l_/r_, _l/_r, lhand/rhand); tlačítka **{{MbGwSpiegelnPlus}}** a "
        "**{{MbGwSpiegelnMinus}}** zrcadlí celou stranu. V editačním režimu (vrcholy, hrany, plochy) nastaví **{{MbGwAufAuswahl}}** váhu na výběr."),
    "w_h_spiel": ("Weights the game can read", "Gewichte, die das Spiel lesen kann", "游戏可以读取的权重", "ゲームが読めるウェイト",
                  "Pesos que el juego puede leer", "Váhy, které hra umí přečíst"),
    "w_spiel": (
        "While you paint, a vertex may carry any number of bones. A vertex of the .glm carries at most four, packed as 10-bit values whose last one "
        "is the remainder to 1 (G2_GetVertBoneWeight). The pose already shows that result while you paint; **{{MbGwUebernehmen}}** writes it "
        "into the model: the four largest weights, normalized, largest first like in Raven's files. The check below the buttons counts vertices "
        "with more than four weights and surfaces with more than 32 bones - for the game chosen in **{{MbRPruefen}}**.",
        "Beim Malen darf eine Ecke beliebig viele Knochen tragen. Eine Ecke der .glm trägt höchstens vier, gepackt als 10-Bit-Werte, deren letzter "
        "der Rest zu 1 ist (G2_GetVertBoneWeight). Die Pose zeigt dieses Ergebnis schon beim Malen; **{{MbGwUebernehmen}}** schreibt es ins Modell: "
        "die vier größten Gewichte, normiert, das größte zuerst wie in den Dateien von Raven. Die Prüfung unter den Knöpfen zählt Ecken mit mehr als "
        "vier Gewichten und Flächen mit mehr als 32 Knochen - für das Spiel, das in **{{MbRPruefen}}** gewählt ist.",
        "绘制时一个顶点可以带任意多根骨骼。.glm 的一个顶点最多带四个,以 10 位数值打包,最后一个是到 1 的余数 (G2_GetVertBoneWeight)。绘制时姿势已经显示"
        "这个结果;**{{MbGwUebernehmen}}** 把它写入模型:最大的四个权重,归一化,像 Raven 的文件一样最大的在前。按钮下方的检查统计超过四个权重的顶点和"
        "超过 32 根骨骼的面——针对 **{{MbRPruefen}}** 中选择的游戏。",
        "ペイント中は頂点に何本でもボーンを持たせられます。.glm の頂点は最大 4 つで、10 ビット値に詰められ、最後の 1 つは 1 までの残りです "
        "(G2_GetVertBoneWeight)。ペイント中もポーズにはこの結果が表示され、**{{MbGwUebernehmen}}** でモデルに書き込みます。大きい 4 つのウェイトを"
        "正規化し、Raven のファイルと同じく大きい順に並べます。ボタンの下のチェックは、**{{MbRPruefen}}** で選んだゲームについて、4 つを超える"
        "ウェイトの頂点と 32 を超えるボーンのサーフェスを数えます。",
        "Mientras pintas, un vértice puede llevar cualquier número de huesos. Un vértice del .glm lleva como máximo cuatro, empaquetados como valores "
        "de 10 bits cuyo último es el resto hasta 1 (G2_GetVertBoneWeight). La pose ya muestra ese resultado al pintar; **{{MbGwUebernehmen}}** lo "
        "escribe en el modelo: los cuatro pesos mayores, normalizados, el mayor primero como en los archivos de Raven. La comprobación bajo los "
        "botones cuenta vértices con más de cuatro pesos y superficies con más de 32 huesos, para el juego elegido en **{{MbRPruefen}}**.",
        "Při malování smí vrchol nést libovolně mnoho kostí. Vrchol souboru .glm nese nejvýše čtyři, zabalené jako 10bitové hodnoty, z nichž poslední "
        "je zbytek do 1 (G2_GetVertBoneWeight). Póza tento výsledek ukazuje už při malování; **{{MbGwUebernehmen}}** ho zapíše do modelu: čtyři "
        "největší váhy, normalizované, největší první jako v souborech Ravenu. Kontrola pod tlačítky počítá vrcholy s více než čtyřmi vahami a "
        "povrchy s více než 32 kostmi - pro hru zvolenou v **{{MbRPruefen}}**."),
    "w_bild_kontext": (
        "Right-click in the view: selection level, the submenus Weights and Display, show/hide and Into script - each with a tooltip.",
        "Rechtsklick in die Ansicht: Auswahlart, die Untermenüs Weights und Anzeige, Aus- und Einblenden und Ins Skript - jede Zeile mit Tooltip.",
        "在视图中右键:选择级别、权重和显示子菜单、隐藏/显示以及插入脚本——每项都有提示。",
        "ビューで右クリック: 選択レベル、ウェイトと表示のサブメニュー、非表示/表示、スクリプトへ。どの行にもツールチップがあります。",
        "Clic derecho en la vista: nivel de selección, los submenús Pesos y Visualización, ocultar/mostrar y Al script, cada uno con su ayuda.",
        "Pravé tlačítko v pohledu: úroveň výběru, podnabídky Váhy a Zobrazení, skrýt/zobrazit a Do skriptu - každý řádek s popiskem."),
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
        "Switch between **{{MbWieMax}}** and **{{MbWieBlender}}** at the top of the command panel; the setting is remembered and acts at once. "
        "Like 3ds Max: four views (Alt+W), bone objects, Q/W/E/R with the gizmo, sub-object levels 1 vertex, 2 edge, 3 border, 4 polygon, 5 element, "
        "6 object, the timeline from 0. Like Blender: one view (Ctrl+Alt+Q for four), octahedral bones, G/R/S with X/Y/Z (twice: local, Shift: plane) "
        "and a typed number, Enter or left click confirms, Esc or right click cancels; Tab toggles object and edit mode, 1/2/3 vertex, edge, face, "
        "the numpad views 1/3/7 (Ctrl for the opposite side), 5 perspective, 2/4/6/8 orbit by 15°, Home frames all, Space plays, the timeline "
        "counts from 1 and shows the GLA frame in its tooltip. X (3ds Max) or F3 (Blender) searches all actions; the right mouse button opens the "
        "menu of the view.",
        "Zwischen **{{MbWieMax}}** und **{{MbWieBlender}}** wechselst du oben im Befehlsfeld; die Einstellung bleibt und wirkt sofort. Wie 3ds Max: "
        "vier Ansichten (Alt+W), Bone-Objekte, Q/W/E/R mit Gizmo, Unterobjekte 1 Scheitelpunkt, 2 Kante, 3 Rand, 4 Polygon, 5 Element, 6 Objekt, die "
        "Zeitleiste ab 0. Wie Blender: eine Ansicht (Strg+Alt+Q für vier), Knochen als Oktaeder, G/R/S mit X/Y/Z (zweimal: lokal, Umschalt: Ebene) und "
        "getippter Zahl, Enter oder Linksklick bestätigt, Esc oder Rechtsklick bricht ab; Tab wechselt Objekt- und Bearbeitungsmodus, 1/2/3 Ecke, "
        "Kante, Fläche, Ziffernblock 1/3/7 (Strg für die Gegenseite), 5 Perspektive, 2/4/6/8 kreisen um 15°, Pos1 zeigt alles, Leertaste spielt ab, "
        "die Zeitleiste zählt ab 1 und zeigt den GLA-Frame im Tooltip. X (3ds Max) bzw. F3 (Blender) sucht alle Aktionen; die rechte Maustaste "
        "öffnet das Menü der Ansicht.",
        "在命令面板顶部切换 **{{MbWieMax}}** 和 **{{MbWieBlender}}**;设置会被记住并立即生效。像 3ds Max:四个视图 (Alt+W)、骨骼对象、带 Gizmo 的 "
        "Q/W/E/R、子对象级别 1 顶点、2 边、3 边界、4 多边形、5 元素、6 对象,时间轴从 0 开始。像 Blender:一个视图 (Ctrl+Alt+Q 切换为四个)、八面体骨骼、"
        "G/R/S 配合 X/Y/Z (按两次: 局部, Shift: 平面) 和输入的数值,Enter 或左键确认,Esc 或右键取消;Tab 切换物体与编辑模式,1/2/3 顶点、边、面,"
        "小键盘 1/3/7 视图 (Ctrl 为对侧),5 透视,2/4/6/8 环绕 15°,Home 显示全部,空格播放,时间轴从 1 开始计数并在提示中显示 GLA 帧。X (3ds Max) 或 "
        "F3 (Blender) 搜索所有操作;鼠标右键打开视图菜单。",
        "コマンドパネル上部で **{{MbWieMax}}** と **{{MbWieBlender}}** を切り替えます。設定は保存され、すぐ反映されます。3ds Max と同じ: 4 つのビュー "
        "(Alt+W)、ボーンオブジェクト、ギズモ付きの Q/W/E/R、サブオブジェクト 1 頂点・2 エッジ・3 境界・4 ポリゴン・5 エレメント・6 オブジェクト、0 から"
        "のタイムライン。Blender と同じ: ビュー 1 つ (Ctrl+Alt+Q で 4 つ)、八面体のボーン、X/Y/Z (2 回でローカル、Shift で平面) と入力した数値付きの "
        "G/R/S、Enter か左クリックで確定、Esc か右クリックで取り消し。Tab でオブジェクト/編集モード、1/2/3 で頂点・辺・面、テンキー 1/3/7 (Ctrl で反対側)、"
        "5 で透視、2/4/6/8 で 15° 周回、Home で全体表示、Space で再生、タイムラインは 1 から数え、ツールチップに GLA フレームを表示します。X (3ds Max) "
        "または F3 (Blender) ですべての操作を検索、右ボタンでビューのメニューを開きます。",
        "Cambia entre **{{MbWieMax}}** y **{{MbWieBlender}}** arriba en el panel de comandos; el ajuste se recuerda y actúa al momento. Como 3ds Max: "
        "cuatro vistas (Alt+W), objetos hueso, Q/W/E/R con gizmo, subobjetos 1 vértice, 2 arista, 3 borde, 4 polígono, 5 elemento, 6 objeto, la "
        "línea de tiempo desde 0. Como Blender: una vista (Ctrl+Alt+Q para cuatro), huesos octaédricos, G/R/S con X/Y/Z (dos veces: local, Mayús: "
        "plano) y un número tecleado, Intro o clic izquierdo confirma, Esc o clic derecho cancela; Tab alterna modo objeto y edición, 1/2/3 vértice, "
        "arista, cara, las vistas del teclado numérico 1/3/7 (Ctrl para el lado opuesto), 5 perspectiva, 2/4/6/8 orbitar 15°, Inicio encuadra todo, "
        "Espacio reproduce, la línea de tiempo cuenta desde 1 y muestra el fotograma GLA en su ayuda. X (3ds Max) o F3 (Blender) busca todas las "
        "acciones; el botón derecho abre el menú de la vista.",
        "Mezi **{{MbWieMax}}** a **{{MbWieBlender}}** přepnete nahoře v panelu příkazů; nastavení se pamatuje a platí hned. Jako 3ds Max: čtyři "
        "pohledy (Alt+W), objekty kostí, Q/W/E/R s gizmem, podobjekty 1 vrchol, 2 hrana, 3 okraj, 4 polygon, 5 prvek, 6 objekt, časová osa od 0. "
        "Jako Blender: jeden pohled (Ctrl+Alt+Q pro čtyři), kosti jako osmistěny, G/R/S s X/Y/Z (dvakrát: lokálně, Shift: rovina) a napsaným číslem, "
        "Enter nebo levé tlačítko potvrdí, Esc nebo pravé tlačítko zruší; Tab přepíná objektový a editační režim, 1/2/3 vrchol, hrana, plocha, "
        "pohledy numerické klávesnice 1/3/7 (Ctrl pro opačnou stranu), 5 perspektiva, 2/4/6/8 kroužení o 15°, Home ukáže vše, mezerník přehraje, "
        "časová osa počítá od 1 a v popisku ukazuje snímek GLA. X (3ds Max) nebo F3 (Blender) hledá všechny akce; pravé tlačítko otevře nabídku pohledu."),
    "b_h_auge": ("Symbols and the eye", "Symbole und Auge", "符号和眼睛", "記号と目", "Símbolos y el ojo", "Symboly a oko"),
    "b_auge": (
        "Every row of the scene explorer shows what it is: model, tag, bone, root "
        "bone, attachment, rig handle, sequence; a surface shows its state in the game (dot = on, ring = off, struck ring = off with its children). The "
        "eye of a row (like 3ds Max in front of the name, like Blender in the column on the right) hides it in the view only - the file and the rules of the game stay as they "
        "are: a click hides it with its children, Ctrl or Alt+click shows only this one, **{{MbAllesEinblenden}}** is in the right-click menu "
        "and in the action search. Hidden rows are greyed out; selecting, painting and the photo pass them by. Like Blender: H, Alt+H, Shift+H.",
        "Jede Zeile des Szenen-Explorers zeigt, was sie ist: Modell, Tag, Knochen, "
        "Wurzelknochen, Anbau, Rig-Griff, Sequenz; eine Fläche zeigt ihren Status im Spiel (Punkt = an, Ring = aus, durchgestrichener Ring = aus samt Kindern). Das "
        "Auge einer Zeile (wie 3ds Max vor dem Namen, wie Blender rechts in der Spalte) blendet sie nur in der Ansicht aus - Datei und Spielregeln bleiben, wie sie sind: "
        "ein Klick blendet sie mit ihren Kindern aus, Strg- oder Alt+Klick zeigt nur diese, **{{MbAllesEinblenden}}** steht im Rechtsklick-Menü "
        "und in der Aktionssuche. Ausgeblendete Zeilen sind grau; Auswählen, Malen und das Foto gehen an ihnen vorbei. Wie Blender: H, Alt+H, Umschalt+H.",
        "场景资源管理器的每一行都显示其类型:模型、标记、骨骼、根骨骼、附件、绑定控制柄、序列;LOD 位于面的标题中。"
        "面前面的标记显示其在游戏中的状态 (圆点 = 开, 圆环 = 关, 带斜线的圆环 = 连同子级一起关)。行上的眼睛 (与 3ds Max 相同在名称前, 与 Blender 相同在右侧的列中) 只在视图中隐藏该行——文件和游戏规则保持不变:单击连同子级一起隐藏,Ctrl 或 Alt+单击只显示这一个,"
        "**{{MbAllesEinblenden}}** 在右键菜单和操作搜索中。隐藏的行显示为灰色;选择、绘制和照片会跳过它们。像 Blender 一样:H、Alt+H、Shift+H。",
        "シーンエクスプローラーの各行は種類を示します: モデル、タグ、ボーン、ルートボーン、"
        "アタッチ、リグのハンドル、シーケンス。LOD はサーフェスの見出しにあります。サーフェスの前の印はゲーム内の状態を示します (点 = オン、輪 = オフ、斜線付きの輪 = 子ごとオフ)。"
        "行の目 (3ds Max と同じく名前の前、Blender と同じく右の列) はビューだけで行を非表示にします。ファイルとゲームの規則はそのままです: クリックで子と一緒に非表示、Ctrl か Alt+クリックでこれだけ表示、"
        "**{{MbAllesEinblenden}}** は右クリックメニューと操作検索にあります。非表示の行は灰色で、選択・ペイント・写真はそれを飛ばします。Blender と同じ: H、Alt+H、Shift+H。",
        "Cada fila del explorador de escena muestra lo que es: modelo, tag, hueso, "
        "hueso raíz, acoplado, control del rig, secuencia; una superficie muestra su estado en el juego (punto = activada, anillo = desactivada, anillo tachado = desactivada con sus hijas). El "
        "ojo de una fila (como 3ds Max delante del nombre, como Blender en la columna de la derecha) la oculta solo en la vista; el archivo y las reglas del juego no "
        "cambian: un clic la oculta con sus hijos, Ctrl o Alt+clic muestra solo esa, **{{MbAllesEinblenden}}** está en el menú del clic derecho y "
        "en la búsqueda de acciones. Las filas ocultas aparecen en gris; seleccionar, pintar y la foto las saltan. Como Blender: H, Alt+H, Mayús+H.",
        "Každý řádek průzkumníku scény ukazuje, co je zač: model, tag, kost, kořenová kost, "
        "přídavek, úchyt rigu, sekvence; povrch ukazuje svůj stav ve hře (tečka = zapnuto, kroužek = vypnuto, přeškrtnutý kroužek = vypnuto i s potomky). Oko řádku (jako 3ds Max před názvem, jako Blender ve sloupci vpravo) skryje "
        "řádek jen v pohledu - soubor a pravidla hry zůstanou: klik ho skryje i s potomky, Ctrl nebo Alt+klik ukáže jen tento, "
        "**{{MbAllesEinblenden}}** je v nabídce pravého tlačítka a ve hledání akcí. Skryté řádky jsou šedé; výběr, malování a fotka je přeskočí. "
        "Jako Blender: H, Alt+H, Shift+H."),
    "b_bild_auge": (
        "The scene explorer with its symbols; the head is hidden with the eye - in the game it stays.",
        "Der Szenen-Explorer mit seinen Symbolen; der Kopf ist mit dem Auge ausgeblendet - im Spiel bleibt er.",
        "带符号的场景资源管理器;头部用眼睛隐藏——在游戏中它仍然存在。",
        "記号付きのシーンエクスプローラー。頭は目で非表示 - ゲームでは残ります。",
        "El explorador de escena con sus símbolos; la cabeza está oculta con el ojo; en el juego sigue ahí.",
        "Průzkumník scény se symboly; hlava je skrytá okem - ve hře zůstává."),
    "b_h_skript": ("Animations into a script", "Animationen ins Skript", "把动画放入脚本", "アニメーションをスクリプトへ", "Animaciones al script", "Animace do skriptu"),
    "b_skript": (
        "The Model tab is only for models: no script tree, no button column, no compile status; File > Open opens a model and Ctrl+S does not touch "
        "the script. To put an animation into a script, open **{{MbInsSkriptFenster}}** (button above the animations, right-click on a sequence, "
        "the menu File or Ctrl+Shift+I). Choose the script (open ones or a new one), the character for an affect block (empty: none) and the "
        "command - SET_ANIM_BOTH/UPPER/LOWER or a hold time with the length of the animation; the preview shows the line, **{{MbInsSkriptEinfuegen}}** "
        "puts it in with one undo step. The small button in the animation field takes the animation selected in the list; Enter inserts, "
        "Esc or **{{UfAbbrechen}}** closes the window. While the window is closed it takes no space.",
        "Der Model-Tab ist nur für Modelle: kein Skriptbaum, keine Knopfspalte, kein Kompilierstand; Datei > Öffnen öffnet ein Modell, und Strg+S "
        "berührt das Skript nicht. Um eine Animation in ein Skript zu setzen, öffne **{{MbInsSkriptFenster}}** (Knopf über den Animationen, Rechtsklick "
        "auf eine Sequenz, Menü Datei oder Strg+Umschalt+I). Wähle das Skript (offene oder ein neues), die Figur für einen affect-Block (leer: keiner) "
        "und den Befehl - SET_ANIM_BOTH/UPPER/LOWER oder eine Haltezeit mit der Länge der Animation; die Vorschau zeigt die Zeile, "
        "**{{MbInsSkriptEinfuegen}}** setzt sie mit einem Rückgängig-Schritt ein. Der kleine Knopf im Feld der Animation nimmt die in der Liste "
        "gewählte Animation; Enter fügt ein, Esc oder **{{UfAbbrechen}}** schließt das Fenster. Solange das Fenster zu ist, belegt es keinen Platz.",
        "模型选项卡只用于模型:没有脚本树、没有按钮列、没有编译状态;文件 > 打开会打开模型,Ctrl+S 不会动脚本。要把动画放进脚本,请打开 "
        "**{{MbInsSkriptFenster}}** (动画上方的按钮、右键单击序列、文件菜单或 Ctrl+Shift+I)。选择脚本 (已打开的或新建)、affect 块的角色 (留空: 无) "
        "和命令——SET_ANIM_BOTH/UPPER/LOWER 或以动画长度为保持时间;预览显示该行,**{{MbInsSkriptEinfuegen}}** 以一个撤销步骤插入。动画字段中的小按钮"
        "使用列表中选中的动画;Enter 插入,Esc 或 **{{UfAbbrechen}}** 关闭窗口。窗口关闭时不占空间。",
        "モデルタブはモデル専用です。スクリプトツリー、ボタン列、コンパイル状態はなく、ファイル > 開く はモデルを開き、Ctrl+S はスクリプトに触れません。"
        "アニメーションをスクリプトに入れるには **{{MbInsSkriptFenster}}** を開きます (アニメーションの上のボタン、シーケンスの右クリック、ファイル"
        "メニュー、Ctrl+Shift+I)。スクリプト (開いているものか新規)、affect ブロックのキャラクター (空欄: なし)、コマンド (SET_ANIM_BOTH/UPPER/LOWER、"
        "またはアニメーションの長さのホールド時間) を選ぶと、プレビューに行が表示され、**{{MbInsSkriptEinfuegen}}** で元に戻せる 1 回の操作として挿入"
        "します。アニメーション欄の小さなボタンは一覧で選択中のアニメーションを入れます。Enter で挿入、Esc または **{{UfAbbrechen}}** でウィンドウを"
        "閉じます。ウィンドウを閉じている間は場所を取りません。",
        "La pestaña Model es solo para modelos: sin árbol del script, sin columna de botones, sin estado de compilación; Archivo > Abrir abre un "
        "modelo y Ctrl+S no toca el script. Para poner una animación en un script, abre **{{MbInsSkriptFenster}}** (botón sobre las animaciones, clic "
        "derecho en una secuencia, el menú Archivo o Ctrl+Mayús+I). Elige el script (abiertos o uno nuevo), el personaje para un bloque affect "
        "(vacío: ninguno) y el comando: SET_ANIM_BOTH/UPPER/LOWER o un tiempo de espera con la duración de la animación; la vista previa muestra "
        "la línea y **{{MbInsSkriptEinfuegen}}** la inserta con un paso de deshacer. El botón pequeño del campo de animación toma la animación "
        "elegida en la lista; Intro inserta, Esc o **{{UfAbbrechen}}** cierra la ventana. Mientras la ventana está cerrada no ocupa sitio.",
        "Karta Model je jen pro modely: žádný strom skriptu, žádný sloupec tlačítek, žádný stav kompilace; Soubor > Otevřít otevře model a Ctrl+S "
        "se skriptu nedotkne. Chcete-li dát animaci do skriptu, otevřete **{{MbInsSkriptFenster}}** (tlačítko nad animacemi, pravé tlačítko na "
        "sekvenci, nabídka Soubor nebo Ctrl+Shift+I). Vyberte skript (otevřené nebo nový), postavu pro blok affect (prázdné: žádný) a příkaz - "
        "SET_ANIM_BOTH/UPPER/LOWER nebo dobu držení s délkou animace; náhled ukáže řádek a **{{MbInsSkriptEinfuegen}}** ho vloží jedním krokem zpět. "
        "Malé tlačítko v poli animace vezme animaci vybranou v seznamu; Enter vloží, Esc nebo **{{UfAbbrechen}}** okno zavře. "
        "Dokud je okno zavřené, nezabírá místo."),
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


def seiten():
    """Die Seiten modell-weights und modell-bedienung (Gruppe Missionen, nach modell-ansehen)."""
    return [
        {
            "id": "modell-weights",
            "gruppe": "mission",
            "titel": _t("w_titel"),
            "kurz": _t("w_kurz"),
            "bloecke": [
                ("img", "h_mb2_weights", _t("w_bild"), "breit"),
                ("p", _t("w_text")),
                ("p", _t("mr_blender")),
                ("h2", "spiel", _t("w_h_spiel")),
                ("p", _t("w_spiel")),
                ("img", "h_mb2_kontext", _t("w_bild_kontext")),
            ],
        },
        {
            "id": "modell-bedienung",
            "gruppe": "mission",
            "titel": _t("b_titel"),
            "kurz": _t("b_kurz"),
            "bloecke": [
                ("img", "h_mb2_blender", _t("b_bild"), "breit"),
                ("p", _t("b_text")),
                ("h2", "auge", _t("b_h_auge")),
                ("p", _t("b_auge")),
                ("img", "h_mb2_auge", _t("b_bild_auge"), "breit"),
                ("h2", "skript", _t("b_h_skript")),
                ("p", _t("b_skript")),
                ("img", "h_mb2_insskript", _t("b_bild_skript")),
            ],
        },
    ]
