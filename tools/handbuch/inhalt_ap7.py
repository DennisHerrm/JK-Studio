"""AP7 (MBENTWURF Teil B): das rechte Feld des Bereichs UV im Model-Tab nach dem freigegebenen Entwurf.

Eigene Quelle fuer die Seite modell-uv (inhalt_uv1b.py): ein Abschnitt "Das Feld rechts" mit zwei Bildern (wie 3ds Max, wie
Blender) hinter dem Bild des UV-Editors, ein Absatz zur Gruppe Pruefen und ein Hinweis zu den neuen Tasten. AP14 fuehrt das
Handbuch zusammen: `inhalt_ap7.einhaengen(<Seite modell-uv>)` in inhalt.py, ES/CS in inhalt_es.py / inhalt_cs.py - wie
inhalt_ap2.py. Bilder: BHED_EDITORTEST=handbuch, BHED_HB_NUR=ap7 (h_ap7_max, h_ap7_blender).
Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_uv1b.py.
"""

X = {
    "h_feld": ("The panel on the right: selection and three steps", "Das Feld rechts: Auswahl und drei Schritte", "右侧面板:选择与三个步骤",
               "右のパネル: 選択と 3 つの手順", "El panel de la derecha: selección y tres pasos", "Panel vpravo: výběr a tři kroky"),
    "feld": (
        "The panel on the right follows the order of the work (3ds Max mode: command panel, tab **{{MfAendern}}**; Blender mode: properties "
        "editor, tab **{{MfDaten}}**). **{{A7RAuswahl}}** names in its heading the texture and the number of islands; it holds the choice of "
        "the texture (**{{UvBild}}**), the selection level with the key of the control mode, **{{A7InselWaehlen}}** (extends the selection to "
        "its whole islands) and **{{A7AllesWaehlen}}**. The steps are numbered. **1 {{A7RNaehte}}**: **{{UvNahtMarkieren}}**, "
        "**{{UvNahtLoeschen}}** and, for figures, **{{UvNaehteSkelett}}**. **2 {{UvGrAbwickeln}}**: the highlighted main button "
        "**{{UvAbwickeln}}**; next to it **{{UvGrSchnell}}** runs the template chosen under **{{A7Vorlage}}** and **{{A7Projizieren}}** the "
        "projection chosen under **{{A7Projektion}}**. **3 {{A7RAufraeumen}}**: **{{UvEntspannen}}**, **{{UvDichte}}** and **{{UvPacken}}**, "
        "with the margin as a slider and **{{A7Drehen}}**. Every button shows its symbol and the key of the control mode. What is needed "
        "less often is folded away in each step under **{{A7Mehr}}**: seams from the shape and by angle, cut ring, mirror seams; re-solve, "
        "pins, method and axis; straighten, rectangle, stack, orient, lock and what is packed.",
        "Das Feld rechts folgt der Reihenfolge der Arbeit (wie 3ds Max: Befehlsfeld, Reiter **{{MfAendern}}**; wie Blender: "
        "Eigenschaften-Editor, Reiter **{{MfDaten}}**). **{{A7RAuswahl}}** nennt in der Überschrift die Textur und die Zahl der Inseln; darin "
        "stehen die Wahl der Textur (**{{UvBild}}**), die Auswahlebene mit der Taste des Bedienmodus, **{{A7InselWaehlen}}** (erweitert die "
        "Auswahl auf ihre ganzen Inseln) und **{{A7AllesWaehlen}}**. Die Schritte sind nummeriert. **1 {{A7RNaehte}}**: "
        "**{{UvNahtMarkieren}}**, **{{UvNahtLoeschen}}** und für Figuren **{{UvNaehteSkelett}}**. **2 {{UvGrAbwickeln}}**: der hervorgehobene "
        "Hauptknopf **{{UvAbwickeln}}**; daneben führt **{{UvGrSchnell}}** die unter **{{A7Vorlage}}** gewählte Vorlage aus und "
        "**{{A7Projizieren}}** die unter **{{A7Projektion}}** gewählte Projektion. **3 {{A7RAufraeumen}}**: **{{UvEntspannen}}**, "
        "**{{UvDichte}}** und **{{UvPacken}}**, dazu der Abstand als Regler und **{{A7Drehen}}**. Jeder Knopf zeigt sein Symbol und die "
        "Taste des Bedienmodus. Was seltener gebraucht wird, steht in jedem Schritt eingeklappt unter **{{A7Mehr}}**: Nähte nach Form und "
        "nach Winkel, Ring aufschneiden, Nähte spiegeln; Neu lösen, Pins, Verfahren und Achse; Begradigen, Rechteck, Stapeln, Ausrichten, "
        "Sperren und der Umfang des Packens.",
        "右侧面板按工作顺序排列(3ds Max 模式:命令面板的 **{{MfAendern}}** 选项卡;Blender 模式:属性编辑器的 **{{MfDaten}}** 选项卡)。"
        "**{{A7RAuswahl}}** 在标题中给出纹理和岛的数量;其中有纹理的选择(**{{UvBild}}**)、带当前操作模式按键的选择层级、**{{A7InselWaehlen}}**"
        "(把选择扩展到整个岛)和 **{{A7AllesWaehlen}}**。各步骤带编号。**1 {{A7RNaehte}}**:**{{UvNahtMarkieren}}**、**{{UvNahtLoeschen}}**,"
        "角色可用 **{{UvNaehteSkelett}}**。**2 {{UvGrAbwickeln}}**:突出显示的主按钮 **{{UvAbwickeln}}**;旁边的 **{{UvGrSchnell}}** 执行在 "
        "**{{A7Vorlage}}** 下选择的模板,**{{A7Projizieren}}** 执行在 **{{A7Projektion}}** 下选择的投影。**3 {{A7RAufraeumen}}**:"
        "**{{UvEntspannen}}**、**{{UvDichte}}** 和 **{{UvPacken}}**,另有间距滑块和 **{{A7Drehen}}**。每个按钮都显示图标和当前操作模式的按键。"
        "不常用的功能折叠在每个步骤的 **{{A7Mehr}}** 下:按形状和按角度生成接缝、切开环、镜像接缝;重新求解、钉住、方法和轴;拉直、矩形、堆叠、"
        "对齐、锁定以及打包范围。",
        "右のパネルは作業の順に並びます(3ds Max モード: コマンドパネルの **{{MfAendern}}** タブ、Blender モード: プロパティエディターの "
        "**{{MfDaten}}** タブ)。**{{A7RAuswahl}}** は見出しにテクスチャとアイランドの数を表示し、テクスチャの選択(**{{UvBild}}**)、操作モードの"
        "キー付きの選択レベル、**{{A7InselWaehlen}}**(選択をアイランド全体に広げる)と **{{A7AllesWaehlen}}** があります。手順には番号が付きます。"
        "**1 {{A7RNaehte}}**: **{{UvNahtMarkieren}}**、**{{UvNahtLoeschen}}**、フィギュアには **{{UvNaehteSkelett}}**。"
        "**2 {{UvGrAbwickeln}}**: 強調されたメインボタン **{{UvAbwickeln}}**。隣の **{{UvGrSchnell}}** は **{{A7Vorlage}}** で選んだテンプレートを、"
        "**{{A7Projizieren}}** は **{{A7Projektion}}** で選んだ投影を実行します。**3 {{A7RAufraeumen}}**: **{{UvEntspannen}}**、"
        "**{{UvDichte}}**、**{{UvPacken}}** と、余白のスライダー、**{{A7Drehen}}**。どのボタンにもアイコンと操作モードのキーが表示されます。"
        "あまり使わないものは各手順の **{{A7Mehr}}** に折りたたまれています: 形状・角度からのシーム、リングを切る、シームのミラー、再計算、ピン、"
        "方式と軸、直線化、矩形、スタック、整列、ロック、パック対象。",
        "El panel de la derecha sigue el orden del trabajo (modo 3ds Max: panel de comandos, pestaña **{{MfAendern}}**; modo Blender: editor "
        "de propiedades, pestaña **{{MfDaten}}**). **{{A7RAuswahl}}** indica en su cabecera la textura y el número de islas; contiene la "
        "elección de la textura (**{{UvBild}}**), el nivel de selección con la tecla del modo de control, **{{A7InselWaehlen}}** (amplía la "
        "selección a sus islas enteras) y **{{A7AllesWaehlen}}**. Los pasos están numerados. **1 {{A7RNaehte}}**: **{{UvNahtMarkieren}}**, "
        "**{{UvNahtLoeschen}}** y, para figuras, **{{UvNaehteSkelett}}**. **2 {{UvGrAbwickeln}}**: el botón principal destacado "
        "**{{UvAbwickeln}}**; a su lado **{{UvGrSchnell}}** ejecuta la plantilla elegida en **{{A7Vorlage}}** y **{{A7Projizieren}}** la "
        "proyección elegida en **{{A7Projektion}}**. **3 {{A7RAufraeumen}}**: **{{UvEntspannen}}**, **{{UvDichte}}** y **{{UvPacken}}**, con "
        "el margen como deslizador y **{{A7Drehen}}**. Cada botón muestra su símbolo y la tecla del modo de control. Lo que se usa menos "
        "queda plegado en cada paso bajo **{{A7Mehr}}**: costuras por forma y por ángulo, cortar anillo, reflejar costuras; recalcular, "
        "fijados, método y eje; enderezar, rectángulo, apilar, orientar, bloquear y qué se empaqueta.",
        "Panel vpravo sleduje pořadí práce (režim 3ds Max: příkazový panel, karta **{{MfAendern}}**; režim Blender: editor vlastností, karta "
        "**{{MfDaten}}**). **{{A7RAuswahl}}** uvádí v záhlaví texturu a počet ostrovů; obsahuje volbu textury (**{{UvBild}}**), úroveň výběru "
        "s klávesou režimu ovládání, **{{A7InselWaehlen}}** (rozšíří výběr na celé ostrovy) a **{{A7AllesWaehlen}}**. Kroky jsou očíslované. "
        "**1 {{A7RNaehte}}**: **{{UvNahtMarkieren}}**, **{{UvNahtLoeschen}}** a pro postavy **{{UvNaehteSkelett}}**. "
        "**2 {{UvGrAbwickeln}}**: zvýrazněné hlavní tlačítko **{{UvAbwickeln}}**; vedle něj **{{UvGrSchnell}}** provede šablonu zvolenou pod "
        "**{{A7Vorlage}}** a **{{A7Projizieren}}** projekci zvolenou pod **{{A7Projektion}}**. **3 {{A7RAufraeumen}}**: **{{UvEntspannen}}**, "
        "**{{UvDichte}}** a **{{UvPacken}}**, k tomu odstup jako posuvník a **{{A7Drehen}}**. Každé tlačítko ukazuje svůj symbol a klávesu "
        "režimu ovládání. Co je potřeba méně často, je v každém kroku sbalené pod **{{A7Mehr}}**: švy podle tvaru a podle úhlu, rozříznout "
        "prstenec, zrcadlit švy; znovu vyřešit, připíchnutí, metoda a osa; narovnat, obdélník, stohovat, zarovnat, zamknout a co se balí."),
    "bild_max": (
        "The UV area like 3ds Max: command panel, tab Modify, with Selection and the steps 1 to 3; the main button Unwrap is highlighted.",
        "Der Bereich UV wie 3ds Max: Befehlsfeld, Reiter Ändern, mit Auswahl und den Schritten 1 bis 3; der Hauptknopf Abwickeln ist hervorgehoben.",
        "3ds Max 模式下的 UV 区域:命令面板的“修改”选项卡,包含“选择”和步骤 1 到 3;主按钮“展开”突出显示。",
        "3ds Max 風の UV エリア: コマンドパネルの「修正」タブに「選択」と手順 1〜3。メインボタン「展開」が強調されています。",
        "El área UV como 3ds Max: panel de comandos, pestaña Modificar, con Selección y los pasos 1 a 3; el botón principal Desplegar está destacado.",
        "Oblast UV jako 3ds Max: příkazový panel, karta Upravit, s výběrem a kroky 1 až 3; hlavní tlačítko Rozbalit je zvýrazněné."),
    "bild_blender": (
        "The UV area like Blender: properties editor, tab Data - the same groups as rounded panels, with Blender's keys on the buttons.",
        "Der Bereich UV wie Blender: Eigenschaften-Editor, Reiter Daten - dieselben Gruppen als abgerundete Panels, mit Blenders Tasten an den Knöpfen.",
        "Blender 模式下的 UV 区域:属性编辑器的“数据”选项卡——相同的分组以圆角面板显示,按钮上是 Blender 的按键。",
        "Blender 風の UV エリア: プロパティエディターの「データ」タブ。同じグループが角丸のパネルになり、ボタンには Blender のキーが表示されます。",
        "El área UV como Blender: editor de propiedades, pestaña Datos; los mismos grupos como paneles redondeados, con las teclas de Blender en los botones.",
        "Oblast UV jako Blender: editor vlastností, karta Data - stejné skupiny jako zaoblené panely, s klávesami Blenderu na tlačítkách."),
    "pruefen": (
        "**{{UvGrPruefen}}** says in its heading at a glance whether something needs attention: green when all is good, otherwise the "
        "number of findings. Under the switches **{{UvSchachbrett}}**, **{{UvDehnungZeigen}}**, **{{UvUeberlappZeigen}}** and "
        "**{{A7NahtDick}}** (seams with a thick line in the UV editor) the values stand as a table: **{{A7Dehnung}}** (low, or the number "
        "of islands stretched over 1.5), **{{A7Ueberlappungen}}**, **{{A7Ausnutzung}}** (the share of the texture's texels the islands "
        "cover) and **{{A7Dichte}}** (even, with the largest deviation from the median). **{{A7Mehr}}** lists islands, mirrored stacks, "
        "open rings and the measurements: angle error, conformality, area deviation, density.",
        "**{{UvGrPruefen}}** sagt in der Überschrift auf einen Blick, ob etwas zu tun ist: grün, wenn alles gut ist, sonst die Zahl der "
        "Befunde. Unter den Schaltern **{{UvSchachbrett}}**, **{{UvDehnungZeigen}}**, **{{UvUeberlappZeigen}}** und **{{A7NahtDick}}** "
        "(Nähte mit dickem Strich im UV-Editor) stehen die Werte als Tabelle: **{{A7Dehnung}}** (gering, oder die Zahl der über 1,5 "
        "gedehnten Inseln), **{{A7Ueberlappungen}}**, **{{A7Ausnutzung}}** (der Anteil der Texel der Textur, den die Inseln bedecken) und "
        "**{{A7Dichte}}** (gleich, mit der größten Abweichung vom Median). **{{A7Mehr}}** nennt Inseln, gespiegelte Stapel, offene Ringe "
        "und die Messwerte: Winkelfehler, Konformität, Flächenabweichung, Dichte.",
        "**{{UvGrPruefen}}** 在标题中一眼显示是否需要处理:一切正常时为绿色,否则显示问题数量。在开关 **{{UvSchachbrett}}**、"
        "**{{UvDehnungZeigen}}**、**{{UvUeberlappZeigen}}** 和 **{{A7NahtDick}}**(在 UV 编辑器中用粗线绘制接缝)下方,数值以表格显示:"
        "**{{A7Dehnung}}**(低,或拉伸超过 1.5 的岛的数量)、**{{A7Ueberlappungen}}**、**{{A7Ausnutzung}}**(岛覆盖的纹理纹素所占比例)和 "
        "**{{A7Dichte}}**(均匀,并给出与中位数的最大偏差)。**{{A7Mehr}}** 列出岛、镜像堆叠、开口环以及测量值:角度误差、保角性、面积偏差、密度。",
        "**{{UvGrPruefen}}** は見出しで対応が必要かどうかをひと目で示します: 問題がなければ緑、あれば指摘の件数。スイッチ **{{UvSchachbrett}}**、"
        "**{{UvDehnungZeigen}}**、**{{UvUeberlappZeigen}}**、**{{A7NahtDick}}**(UV エディターでシームを太線で描く)の下に、値が表として並びます: "
        "**{{A7Dehnung}}**(小、または 1.5 を超えて伸びたアイランドの数)、**{{A7Ueberlappungen}}**、**{{A7Ausnutzung}}**(アイランドが覆う"
        "テクスチャのテクセルの割合)、**{{A7Dichte}}**(均一、中央値からの最大のずれ付き)。**{{A7Mehr}}** にはアイランド、ミラースタック、開いた"
        "リングと計測値(角度誤差、等角性、面積偏差、密度)が並びます。",
        "**{{UvGrPruefen}}** indica en su cabecera de un vistazo si hay algo que atender: verde cuando todo está bien; si no, el número de "
        "avisos. Bajo los interruptores **{{UvSchachbrett}}**, **{{UvDehnungZeigen}}**, **{{UvUeberlappZeigen}}** y **{{A7NahtDick}}** "
        "(costuras con trazo grueso en el editor UV) los valores aparecen como tabla: **{{A7Dehnung}}** (bajo, o el número de islas "
        "estiradas más de 1,5), **{{A7Ueberlappungen}}**, **{{A7Ausnutzung}}** (la parte de los téxeles de la textura que cubren las "
        "islas) y **{{A7Dichte}}** (uniforme, con la mayor desviación respecto a la mediana). **{{A7Mehr}}** enumera islas, pilas "
        "reflejadas, anillos abiertos y las medidas: error de ángulo, conformidad, desviación de área, densidad.",
        "**{{UvGrPruefen}}** v záhlaví na první pohled řekne, zda je co řešit: zeleně, když je vše v pořádku, jinak počet nálezů. Pod "
        "přepínači **{{UvSchachbrett}}**, **{{UvDehnungZeigen}}**, **{{UvUeberlappZeigen}}** a **{{A7NahtDick}}** (švy tlustou čarou v "
        "editoru UV) jsou hodnoty jako tabulka: **{{A7Dehnung}}** (nízké, nebo počet ostrovů natažených nad 1,5), **{{A7Ueberlappungen}}**, "
        "**{{A7Ausnutzung}}** (podíl texelů textury, které ostrovy pokrývají) a **{{A7Dichte}}** (rovnoměrná, s největší odchylkou od "
        "mediánu). **{{A7Mehr}}** uvádí ostrovy, zrcadlené stohy, otevřené prstence a měření: chybu úhlu, konformitu, odchylku plochy, hustotu."),
    "tasten": (
        "New keys over the UV editor. 3ds Max mode: Ctrl+A selects everything. Blender mode: A selects everything, L adds the island under "
        "the mouse, Ctrl+A matches the density and Ctrl+P packs (in Blender these two are the keys of the classic key map).",
        "Neue Tasten über dem UV-Editor. Wie 3ds Max: Strg+A wählt alles. Wie Blender: A wählt alles, L nimmt die Insel unter der Maus "
        "dazu, Strg+A gleicht die Dichte an und Strg+P packt (in Blender sind das die Tasten der klassischen Belegung).",
        "UV 编辑器上的新按键。3ds Max 模式:Ctrl+A 全选。Blender 模式:A 全选,L 加入鼠标下的岛,Ctrl+A 统一密度,Ctrl+P 打包"
        "(在 Blender 中这两个是经典键位的按键)。",
        "UV エディター上の新しいキー。3ds Max モード: Ctrl+A ですべて選択。Blender モード: A ですべて選択、L でマウス下のアイランドを追加、"
        "Ctrl+A で密度をそろえ、Ctrl+P でパック(Blender ではこの 2 つはクラシックキーマップのキーです)。",
        "Teclas nuevas sobre el editor UV. Modo 3ds Max: Ctrl+A selecciona todo. Modo Blender: A selecciona todo, L añade la isla bajo el "
        "ratón, Ctrl+A iguala la densidad y Ctrl+P empaqueta (en Blender estas dos son las teclas del mapa de teclas clásico).",
        "Nové klávesy nad editorem UV. Režim 3ds Max: Ctrl+A vybere vše. Režim Blender: A vybere vše, L přidá ostrov pod myší, Ctrl+A "
        "sjednotí hustotu a Ctrl+P zabalí (v Blenderu jsou to klávesy klasického rozložení kláves)."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def einhaengen(uv):
    """Den Abschnitt "Das Feld rechts" in die Seite modell-uv, vor "Auswahl an beiden Stellen" (hinter das Bild des UV-Editors)."""
    b = uv["bloecke"]
    i = next(n for n, x in enumerate(b) if x[0] == "h2" and x[1] == "auswahl")
    b[i:i] = [
        ("h2", "feld", _t("h_feld")),
        ("p", _t("feld")),
        ("img", "h_ap7_max", _t("bild_max"), "breit"),
        ("img", "h_ap7_blender", _t("bild_blender"), "breit"),
        ("p", _t("pruefen")),
        ("note", _t("tasten")),
    ]
