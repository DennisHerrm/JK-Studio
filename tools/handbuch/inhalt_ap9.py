"""AP9 (MBENTWURF Teil B): das rechte Feld des Bereichs Painting nach dem freigegebenen Entwurf - Gruppen Ziel, Werkzeuge,
Pinsel, Ebenen, Fuer das Spiel; Tasten [ und ] fuer die Pinselgroesse.

Eigene Quelle fuer die Seite modell-painting (inhalt_paint.py); AP14 fuehrt das Handbuch zusammen:
    import inhalt_ap9
    inhalt_ap9.einhaengen(next(s for s in SEITEN if s["id"] == "modell-painting"))
dazu in inhalt_es.py / inhalt_cs.py `from inhalt_ap9 import ES / CS` wie bei inhalt_ap2 und in gen_handbuch.py die Texte aus
tools/i18n_ap9.py (wie i18n_ap1). Bilder: h_ap9_max, h_ap9_blender (BHED_EDITORTEST=handbuch, BHED_HB_NUR=ap9).
Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_ap2.py.
"""

X = {
    "h_feld": ("The right panel", "Das rechte Feld", "右侧面板", "右のパネル", "El panel derecho", "Pravý panel"),
    "feld": (
        "The groups of the right panel follow the order of the work. **{{PtGrZiel}}** shows what you paint on: the shader (choose another "
        "one to paint another image), the stage of the shader if it draws several images, and the image with its size and format - both "
        "stay as in the original. **{{PtEditorZeigen}}** shows or hides the flat image beside the view. **{{PtGrWerkzeuge}}** holds the "
        "tools as buttons with a symbol; the chosen one is highlighted. **{{PtGrPinsel}}** sets the colour (two colour fields, swap, "
        "sample), radius, strength and spacing; hardness, mirroring, pen pressure, back faces and bleed are under **{{PtErweitert}}**. "
        "**{{PtGrEbenen}}** lists the layers from the top: the eye shows or hides a layer, the original at the bottom carries a lock, and "
        "below the list are the blend mode and the opacity of the selected layer. **{{PtGrAusgabe}}** writes the painted images; its "
        "heading counts them. Like 3ds Max the groups stand in the tab **{{MfAendern}}** of the command panel (**{{PtGrAusgabe}}** also "
        "under **{{MfDienste}}**), like Blender in the tab **{{MfWerkzeug}}** of the properties editor (**{{PtGrZiel}}** also under "
        "**{{MfMaterial}}**). A click on a heading folds a group.",
        "Die Gruppen des rechten Felds folgen der Reihenfolge der Arbeit. **{{PtGrZiel}}** zeigt, worauf du malst: den Shader (einen "
        "anderen wählen malt auf ein anderes Bild), die Stufe des Shaders, wenn er mehrere Bilder zeichnet, und das Bild mit Größe und "
        "Format - beides bleibt wie im Original. **{{PtEditorZeigen}}** blendet das flache Bild neben der Ansicht ein und aus. "
        "**{{PtGrWerkzeuge}}** enthält die Werkzeuge als Knöpfe mit Symbol; das gewählte ist hervorgehoben. **{{PtGrPinsel}}** stellt Farbe "
        "(zwei Farbfelder, tauschen, aufnehmen), Radius, Stärke und Abstand ein; Härte, Spiegeln, Stiftdruck, Rückseiten und Ausbluten "
        "stehen unter **{{PtErweitert}}**. **{{PtGrEbenen}}** listet die Ebenen von oben: das Auge blendet eine Ebene ein und aus, das "
        "Original ganz unten trägt ein Schloss, und unter der Liste stehen Mischart und Deckkraft der gewählten Ebene. "
        "**{{PtGrAusgabe}}** schreibt die bemalten Bilder; die Überschrift zählt sie. Wie 3ds Max stehen die Gruppen im Reiter "
        "**{{MfAendern}}** des Befehlsfelds (**{{PtGrAusgabe}}** auch unter **{{MfDienste}}**), wie Blender im Reiter **{{MfWerkzeug}}** "
        "des Eigenschaften-Editors (**{{PtGrZiel}}** auch unter **{{MfMaterial}}**). Ein Klick auf eine Überschrift klappt die Gruppe zu.",
        "右侧面板的分组按工作顺序排列。**{{PtGrZiel}}** 显示绘制的对象: 着色器 (选择另一个即可绘制另一张图像)、着色器的阶段 (当它绘制多张图像时) 以及图像及其"
        "大小和格式 - 两者都与原图保持一致。**{{PtEditorZeigen}}** 显示或隐藏视图旁的平面图像。**{{PtGrWerkzeuge}}** 以带符号的按钮列出工具; 所选工具会高亮。"
        "**{{PtGrPinsel}}** 设置颜色 (两个色块、交换、取色)、半径、强度和间距; 硬度、镜像、笔压、背面和出血位于 **{{PtErweitert}}** 下。**{{PtGrEbenen}}** "
        "自上而下列出图层: 眼睛显示或隐藏图层, 最底部的原图带有锁, 列表下方是所选图层的混合模式和不透明度。**{{PtGrAusgabe}}** 写出已绘制的图像; 其标题会"
        "统计数量。与 3ds Max 相同时, 这些分组位于命令面板的 **{{MfAendern}}** 选项卡 (**{{PtGrAusgabe}}** 也在 **{{MfDienste}}** 下); 与 Blender 相同时, "
        "位于属性编辑器的 **{{MfWerkzeug}}** 选项卡 (**{{PtGrZiel}}** 也在 **{{MfMaterial}}** 下)。单击标题可折叠分组。",
        "右のパネルのグループは作業の順に並びます。**{{PtGrZiel}}** は塗る対象を示します: シェーダー (別のものを選ぶと別の画像を塗ります)、複数の画像を描く"
        "シェーダーではそのステージ、そして画像とそのサイズ・形式 - どちらもオリジナルのままです。**{{PtEditorZeigen}}** はビューの横の平面画像を表示/非表示に"
        "します。**{{PtGrWerkzeuge}}** にはシンボル付きのボタンとしてツールが並び、選択中のものが強調されます。**{{PtGrPinsel}}** で色 (2 つの色欄、入れ替え、"
        "取得)、半径、強さ、間隔を設定します。硬さ、ミラー、筆圧、裏面、にじみは **{{PtErweitert}}** の下にあります。**{{PtGrEbenen}}** はレイヤーを上から"
        "並べます: 目でレイヤーを表示/非表示にし、一番下のオリジナルには錠が付き、リストの下に選択中のレイヤーの描画モードと不透明度があります。"
        "**{{PtGrAusgabe}}** は塗った画像を書き出し、見出しがその数を示します。3ds Max 風ではグループはコマンドパネルの **{{MfAendern}}** タブに "
        "(**{{PtGrAusgabe}}** は **{{MfDienste}}** にも)、Blender 風ではプロパティエディターの **{{MfWerkzeug}}** タブに (**{{PtGrZiel}}** は "
        "**{{MfMaterial}}** にも) あります。見出しをクリックするとグループを折りたためます。",
        "Los grupos del panel derecho siguen el orden del trabajo. **{{PtGrZiel}}** muestra sobre qué pintas: el shader (elige otro para "
        "pintar otra imagen), la etapa del shader si dibuja varias imágenes y la imagen con su tamaño y formato, que se mantienen como en "
        "el original. **{{PtEditorZeigen}}** muestra u oculta la imagen plana junto a la vista. **{{PtGrWerkzeuge}}** contiene las "
        "herramientas como botones con símbolo; la elegida queda resaltada. **{{PtGrPinsel}}** ajusta el color (dos campos de color, "
        "intercambiar, tomar), el radio, la fuerza y el espaciado; dureza, simetría, presión del lápiz, caras traseras y sangrado están en "
        "**{{PtErweitert}}**. **{{PtGrEbenen}}** lista las capas desde arriba: el ojo muestra u oculta una capa, el original al fondo lleva "
        "un candado y bajo la lista están el modo de fusión y la opacidad de la capa elegida. **{{PtGrAusgabe}}** escribe las imágenes "
        "pintadas; su título las cuenta. Como 3ds Max, los grupos están en la pestaña **{{MfAendern}}** del panel de comandos "
        "(**{{PtGrAusgabe}}** también en **{{MfDienste}}**); como Blender, en la pestaña **{{MfWerkzeug}}** del editor de propiedades "
        "(**{{PtGrZiel}}** también en **{{MfMaterial}}**). Un clic en un título pliega el grupo.",
        "Skupiny pravého panelu sledují pořadí práce. **{{PtGrZiel}}** ukazuje, na co malujete: shader (volbou jiného malujete na jiný "
        "obrázek), fázi shaderu, pokud kreslí více obrázků, a obrázek s velikostí a formátem - obojí zůstává jako v originálu. "
        "**{{PtEditorZeigen}}** zobrazí nebo skryje plochý obrázek vedle pohledu. **{{PtGrWerkzeuge}}** obsahuje nástroje jako tlačítka se "
        "symbolem; zvolený je zvýrazněn. **{{PtGrPinsel}}** nastavuje barvu (dvě barevná pole, prohodit, nabrat), poloměr, sílu a rozestup; "
        "tvrdost, zrcadlení, přítlak pera, zadní strany a přesah jsou pod **{{PtErweitert}}**. **{{PtGrEbenen}}** vypisuje vrstvy shora: "
        "oko vrstvu zobrazí nebo skryje, originál úplně dole nese zámek a pod seznamem jsou režim prolnutí a krytí zvolené vrstvy. "
        "**{{PtGrAusgabe}}** zapisuje namalované obrázky; nadpis je počítá. Jako 3ds Max stojí skupiny na kartě **{{MfAendern}}** příkazového "
        "panelu (**{{PtGrAusgabe}}** také pod **{{MfDienste}}**), jako Blender na kartě **{{MfWerkzeug}}** editoru vlastností "
        "(**{{PtGrZiel}}** také pod **{{MfMaterial}}**). Kliknutí na nadpis skupinu sbalí."),
    "bild_max": (
        "Painting like 3ds Max: command panel, tab Modify, with the groups Target, Tools, Brush and Layers",
        "Painting wie 3ds Max: Befehlsfeld, Reiter Ändern, mit den Gruppen Ziel, Werkzeuge, Pinsel und Ebenen",
        "与 3ds Max 相同的绘制: 命令面板的“修改”选项卡, 包含目标、工具、画笔和图层分组",
        "3ds Max 風のペイント: コマンドパネルの「修正」タブに、ターゲット、ツール、ブラシ、レイヤーのグループ",
        "Pintura como 3ds Max: panel de comandos, pestaña Modificar, con los grupos Destino, Herramientas, Pincel y Capas",
        "Malování jako 3ds Max: příkazový panel, karta Upravit, se skupinami Cíl, Nástroje, Štětec a Vrstvy"),
    "bild_blender": (
        "Painting like Blender: properties editor, tab Tool, with the same groups as rounded panels",
        "Painting wie Blender: Eigenschaften-Editor, Reiter Werkzeug, mit denselben Gruppen als abgerundete Panels",
        "与 Blender 相同的绘制: 属性编辑器的“工具”选项卡, 相同的分组显示为圆角面板",
        "Blender 風のペイント: プロパティエディターの「ツール」タブに、同じグループが角丸のパネルとして並びます",
        "Pintura como Blender: editor de propiedades, pestaña Herramienta, con los mismos grupos como paneles redondeados",
        "Malování jako Blender: editor vlastností, karta Nástroj, se stejnými skupinami jako zaoblené panely"),
    # ersetzt den Absatz "werkzeuge" der Seite (dort stand "im Rollout Viewport Canvas")
    "werkzeuge": (
        "**{{PtWzPinsel}}**, **{{PtWzFuellen}}**, **{{PtWzKlonen}}** (Ctrl+click sets the source), **{{PtWzWischen}}**, **{{PtWzMaske}}**, "
        "**{{PtWzWeich}}** and **{{PtWzRadierer}}** are in the group **{{PtGrWerkzeuge}}** of the right panel - what **{{PtRolloutMax}}** is "
        "in 3ds Max and **{{PtRolloutBlender}}** in Blender -, in the menu **{{PtMenue}}**, in the right-click of the view and in the "
        "search. Radius, strength and spacing set the brush, hardness (falloff) is under **{{PtErweitert}}**; with a pen the pressure can "
        "change size and strength. The keys <kbd>[</kbd> and <kbd>]</kbd> make the brush one step smaller or larger (hold to go on); in "
        "the Painting area they belong to the brush in both control modes. In the view the dabs follow the line you draw on the screen, "
        "each one on the surface visible there, like in Substance Painter and Blender - a fast stroke from one hand to the other paints "
        "only where the brush passed over the model, nothing over the background. In Blender mode F and Shift+F drag the radius and the "
        "strength, S samples a colour, X swaps the two colours and 1 shows the mask. **{{PtSpiegeln}}** paints every dab a second time on "
        "the mirrored side of the model (axis x for characters). Faces that point away from the hit one by more than 80 degrees stay "
        "untouched unless **{{PtRueckseiten}}** is on.",
        "**{{PtWzPinsel}}**, **{{PtWzFuellen}}**, **{{PtWzKlonen}}** (Strg+Klick setzt die Quelle), **{{PtWzWischen}}**, **{{PtWzMaske}}**, "
        "**{{PtWzWeich}}** und **{{PtWzRadierer}}** stehen in der Gruppe **{{PtGrWerkzeuge}}** des rechten Felds - das, was in 3ds Max "
        "**{{PtRolloutMax}}** und in Blender **{{PtRolloutBlender}}** ist -, im Menü **{{PtMenue}}**, im Rechtsklick der Ansicht und in der "
        "Suche. Radius, Stärke und Abstand stellen den Pinsel ein, die Härte (Abfall) steht unter **{{PtErweitert}}**; mit einem Stift "
        "ändert der Druck Größe und Stärke. Die Tasten <kbd>[</kbd> und <kbd>]</kbd> machen den Pinsel einen Schritt kleiner oder größer "
        "(halten: weiter); im Bereich Painting gehören sie in beiden Bedienmodi dem Pinsel. In der Ansicht folgen die Tupfer der Linie, "
        "die du auf dem Bildschirm ziehst, jeder auf der dort sichtbaren Fläche, wie in Substance Painter und Blender - ein schneller "
        "Strich von einer Hand zur anderen malt nur dort, wo der Pinsel über das Modell ging, nichts über dem Hintergrund. Im Blender-Modus "
        "ziehen F und Umschalt+F Radius und Stärke, S nimmt eine Farbe auf, X tauscht die beiden Farben, 1 zeigt die Maske. "
        "**{{PtSpiegeln}}** malt jeden Tupfer ein zweites Mal auf der gespiegelten Seite des Modells (Achse x bei Figuren). Flächen, die "
        "mehr als 80 Grad von der getroffenen weg zeigen, bleiben unberührt, außer **{{PtRueckseiten}}** ist an.",
        "**{{PtWzPinsel}}**、**{{PtWzFuellen}}**、**{{PtWzKlonen}}** (Ctrl+单击设置源点)、**{{PtWzWischen}}**、**{{PtWzMaske}}**、**{{PtWzWeich}}** 和 "
        "**{{PtWzRadierer}}** 位于右侧面板的 **{{PtGrWerkzeuge}}** 分组 - 相当于 3ds Max 的 **{{PtRolloutMax}}** 和 Blender 的 **{{PtRolloutBlender}}** -, "
        "也在菜单 **{{PtMenue}}**、视图右键菜单和搜索中。半径、强度和间距设置画笔, 硬度 (衰减) 位于 **{{PtErweitert}}** 下; 使用手写笔时压力可改变大小和强度。"
        "按键 <kbd>[</kbd> 和 <kbd>]</kbd> 将画笔缩小或放大一级 (按住可连续); 在绘制区域中, 两种操作模式下它们都属于画笔。在视图中, 点按沿着你在屏幕上画出"
        "的线分布, 每一个都落在那里可见的表面上, 与 Substance Painter 和 Blender 相同 - 从一只手快速划到另一只手, 只在画笔经过模型的地方绘制, 背景上不会绘制"
        "任何内容。Blender 模式下 F 和 Shift+F 拖动半径和强度, S 取色, X 交换两种颜色, 1 显示遮罩。**{{PtSpiegeln}}** 会在模型的镜像一侧再画一次每个点按 "
        "(角色为 x 轴)。与命中面朝向相差超过 80 度的面不受影响, 除非打开 **{{PtRueckseiten}}**。",
        "**{{PtWzPinsel}}**、**{{PtWzFuellen}}**、**{{PtWzKlonen}}** (Ctrl+クリックでソース)、**{{PtWzWischen}}**、**{{PtWzMaske}}**、**{{PtWzWeich}}**、"
        "**{{PtWzRadierer}}** は右のパネルのグループ **{{PtGrWerkzeuge}}** にあります (3ds Max の **{{PtRolloutMax}}**、Blender の "
        "**{{PtRolloutBlender}}** にあたります)。メニュー **{{PtMenue}}**、ビューの右クリック、検索にもあります。半径、強さ、間隔でブラシを設定し、硬さ (減衰) は "
        "**{{PtErweitert}}** の下にあります。ペンでは筆圧でサイズと強さを変えられます。キー <kbd>[</kbd> と <kbd>]</kbd> はブラシを 1 段階小さく / 大きく"
        "します (押し続けると連続)。ペイント領域ではどちらの操作モードでもブラシのキーです。ビューではダブは画面上に描いた線に沿って並び、それぞれがそこに"
        "見えるサーフェスに置かれます (Substance Painter や Blender と同じ) - 片手からもう一方の手への素早いストロークは、ブラシがモデルの上を通ったところ"
        "だけを塗り、背景の上には何も塗りません。Blender モードでは F と Shift+F で半径と強さをドラッグ、S で色を取得、X で 2 色を入れ替え、1 でマスクを表示"
        "します。**{{PtSpiegeln}}** は各ダブをモデルの鏡像側にもう一度塗ります (キャラクターは x 軸)。当たった面から 80 度以上外を向く面は "
        "**{{PtRueckseiten}}** がオンでない限り塗られません。",
        "**{{PtWzPinsel}}**, **{{PtWzFuellen}}**, **{{PtWzKlonen}}** (Ctrl+clic fija el origen), **{{PtWzWischen}}**, **{{PtWzMaske}}**, "
        "**{{PtWzWeich}}** y **{{PtWzRadierer}}** están en el grupo **{{PtGrWerkzeuge}}** del panel derecho - lo que en 3ds Max es "
        "**{{PtRolloutMax}}** y en Blender **{{PtRolloutBlender}}** -, en el menú **{{PtMenue}}**, en el clic derecho de la vista y en la "
        "búsqueda. Radio, fuerza y espaciado ajustan el pincel; la dureza (caída) está en **{{PtErweitert}}**; con un lápiz la presión "
        "puede cambiar tamaño y fuerza. Las teclas <kbd>[</kbd> y <kbd>]</kbd> hacen el pincel un paso más pequeño o más grande "
        "(mantener: sigue); en el área Pintura pertenecen al pincel en ambos modos de control. En la vista los toques siguen la línea que "
        "trazas en la pantalla, cada uno sobre la superficie visible allí, como en Substance Painter y Blender: un trazo rápido de una mano "
        "a la otra pinta solo donde el pincel pasó sobre el modelo, nada sobre el fondo. En modo Blender F y Mayús+F arrastran radio y "
        "fuerza, S toma un color, X intercambia los dos colores y 1 muestra la máscara. **{{PtSpiegeln}}** pinta cada toque una segunda vez "
        "en el lado reflejado del modelo (eje x en personajes). Las caras que se apartan más de 80 grados de la tocada quedan intactas "
        "salvo que **{{PtRueckseiten}}** esté activo.",
        "**{{PtWzPinsel}}**, **{{PtWzFuellen}}**, **{{PtWzKlonen}}** (Ctrl+klik nastaví zdroj), **{{PtWzWischen}}**, **{{PtWzMaske}}**, "
        "**{{PtWzWeich}}** a **{{PtWzRadierer}}** jsou ve skupině **{{PtGrWerkzeuge}}** pravého panelu - to, co je ve 3ds Max "
        "**{{PtRolloutMax}}** a v Blenderu **{{PtRolloutBlender}}** -, v nabídce **{{PtMenue}}**, v nabídce pravého tlačítka v pohledu a ve "
        "vyhledávání. Poloměr, síla a rozestup nastavují štětec, tvrdost (útlum) je pod **{{PtErweitert}}**; s perem může přítlak měnit "
        "velikost a sílu. Klávesy <kbd>[</kbd> a <kbd>]</kbd> štětec o krok zmenší nebo zvětší (držením pokračujete); v oblasti Malování "
        "patří štětci v obou režimech ovládání. V pohledu doteky sledují čáru, kterou táhnete na obrazovce, každý na ploše, která je tam "
        "vidět, jako v Substance Painteru a Blenderu - rychlý tah z jedné ruky do druhé maluje jen tam, kde štětec přešel přes model, přes "
        "pozadí nic. V režimu Blender F a Shift+F táhnou poloměr a sílu, S nabere barvu, X prohodí obě barvy a 1 ukáže masku. "
        "**{{PtSpiegeln}}** maluje každý dotek podruhé na zrcadlové straně modelu (osa x u postav). Plochy odvrácené od zasažené o více "
        "než 80 stupňů zůstanou nedotčené, pokud není zapnuto **{{PtRueckseiten}}**."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def einhaengen(painting):
    """Die Seite modell-painting auf den Stand von AP9: nach dem ersten Bild der Abschnitt "Das rechte Feld" mit den Bildern
    beider Bedienmodi; der Absatz "Werkzeuge und Pinsel" nennt die Gruppe Werkzeuge statt des Rollouts und die Tasten [ ]."""
    b = painting["bloecke"]
    i = next(n for n, x in enumerate(b) if x[0] == "h2" and x[1] == "werkzeuge")
    if b[i + 1][0] != "p":
        raise SystemExit("inhalt_ap9: nach der Ueberschrift 'werkzeuge' steht kein Absatz")
    b[i + 1] = ("p", _t("werkzeuge"))
    b[i:i] = [
        ("h2", "feld", _t("h_feld")),
        ("p", _t("feld")),
        ("img", "h_ap9_max", _t("bild_max"), "breit"),
        ("img", "h_ap9_blender", _t("bild_blender"), "breit"),
    ]
