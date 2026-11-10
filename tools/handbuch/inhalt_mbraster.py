"""MBRASTER (rc612): der Abschnitt "Boden-Raster" der Seite modell-ansehen (nach "Kopfzeile der Ansicht, ViewCube und Gizmo").

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_mbkopf.py.
"""

X = {
    "h_raster": ("Floor grid", "Boden-Raster", "地面栅格", "床グリッド", "Rejilla del suelo", "Podlahová mřížka"),
    "max": (
        "Every view shows a grid from the start, like the home grid of 3ds Max: thin lines at the grid spacing, darker major lines and the "
        "world axes in black. In the perspective view it lies on the floor and ends after 7 lines (Max: Perspective View Grid Extent); in the "
        "views Top, Front, Left ... it lies in the view plane, fills the whole view and grows with the zoom - zoomed out, the major lines "
        "become the new lines; zoomed in it never gets finer than the spacing, as in Max. The figure hides the grid. **[+] > {{MkRaster}}** "
        "(<kbd>G</kbd>) switches it for this view. **[+] > {{MrRasterEinst}}** sets **{{MrAbstand}}** (8 JKA units, like the grid of the "
        "map editor), **{{MrHaupt}}** (8: a major line every 64 units - the height of a player), **{{MrAusdehnung}}** and the "
        "**{{MbAnBodenZ}}**: by default -24, the feet of a JKA figure. 3ds Max and Blender put their grid at Z = 0, but a JKA figure has its "
        "origin at the hips. **{{MrTiefster}}** puts the floor under the lowest point of the figure in its pose (props with their origin on "
        "the floor: 0). Spacing and major lines are also in the settings (**{{MrEinstGruppe}}**). The grey floor of ModView "
        "(**{{MrGrauBoden}}**) stays a switch of its own and can be shown together with the grid.",
        "Jede Ansicht zeigt ab Werk ein Raster wie das Home Grid von 3ds Max: feine Linien im Rasterabstand, dunklere Hauptlinien und die "
        "Weltachsen schwarz. In der Perspektive liegt es auf dem Boden und endet nach 7 Linien (Max: Perspective View Grid Extent); in den "
        "Ansichten Oben, Vorne, Links ... liegt es in der Bildebene, füllt die ganze Ansicht und wächst mit dem Zoom - weiter weg werden die "
        "Hauptlinien zu den neuen Linien, näher heran wird es nie feiner als der Abstand, wie in Max. Die Figur verdeckt das Raster. "
        "**[+] > {{MkRaster}}** (<kbd>G</kbd>) schaltet es für diese Ansicht. **[+] > {{MrRasterEinst}}** stellt **{{MrAbstand}}** (8 "
        "JKA-Einheiten wie das Raster des Karten-Editors), **{{MrHaupt}}** (8: eine Hauptlinie alle 64 Einheiten - die Höhe eines Spielers), "
        "**{{MrAusdehnung}}** und die **{{MbAnBodenZ}}** ein: ab Werk -24, die Füße einer JKA-Figur. 3ds Max und Blender legen ihr Raster "
        "auf Z = 0, eine JKA-Figur hat ihren Ursprung aber in der Hüfte. **{{MrTiefster}}** legt den Boden unter den tiefsten Punkt der "
        "Figur in ihrer Pose (Requisiten mit Ursprung am Boden: 0). Abstand und Hauptlinien stehen auch in den Einstellungen "
        "(**{{MrEinstGruppe}}**). Der graue Boden von ModView (**{{MrGrauBoden}}**) bleibt ein eigener Schalter und lässt sich zusammen mit "
        "dem Raster zeigen.",
        "每个视图默认显示栅格, 如 3ds Max 的主栅格: 按栅格间距的细线、较深的主线、黑色的世界坐标轴。透视图中它位于地面上, 7 条线后结束 (Max: 透视视图"
        "栅格范围); 在顶、前、左等视图中它位于视图平面内, 填满整个视图并随缩放变化——缩小时主线成为新的线, 放大时不会比间距更细, 与 Max 相同。角色会遮挡"
        "栅格。**[+] > {{MkRaster}}** (<kbd>G</kbd>) 为此视图切换。**[+] > {{MrRasterEinst}}** 设置 **{{MrAbstand}}** (8 个 JKA 单位, 与地图编辑器"
        "的栅格相同)、**{{MrHaupt}}** (8: 每 64 单位一条主线——玩家的高度)、**{{MrAusdehnung}}** 和 **{{MbAnBodenZ}}**: 默认 -24, 即 JKA 角色的脚。"
        "3ds Max 和 Blender 将栅格放在 Z = 0, 但 JKA 角色的原点在髋部。**{{MrTiefster}}** 将地面放到角色当前姿势的最低点之下 (原点在地面的道具: 0)。"
        "间距和主线也在设置中 (**{{MrEinstGruppe}}**)。ModView 的灰色地面 (**{{MrGrauBoden}}**) 仍是单独的开关, 可与栅格同时显示。",
        "どのビューにも最初から 3ds Max のホームグリッドと同じグリッドが表示されます: グリッド間隔の細い線、濃いメジャーライン、黒いワールド軸。パース"
        "では床の上にあり、7 本で終わります (Max: Perspective View Grid Extent)。上、前、左などのビューではビュー平面にあり、ビュー全体を埋め、ズームで"
        "変わります - 引くとメジャーラインが新しい線になり、寄っても間隔より細かくなりません (Max と同じ)。フィギュアはグリッドを隠します。"
        "**[+] > {{MkRaster}}** (<kbd>G</kbd>) でこのビューの表示を切り替えます。**[+] > {{MrRasterEinst}}** で **{{MrAbstand}}** (8 JKA 単位、"
        "マップエディタのグリッドと同じ)、**{{MrHaupt}}** (8: 64 単位ごとにメジャーライン - プレイヤーの高さ)、**{{MrAusdehnung}}**、"
        "**{{MbAnBodenZ}}** を設定します。既定は -24 で、JKA フィギュアの足の位置です。3ds Max と Blender はグリッドを Z = 0 に置きますが、JKA の"
        "フィギュアは原点が腰にあります。**{{MrTiefster}}** は床をポーズ中のフィギュアの最低点の下に置きます (原点が床にある小道具なら 0)。間隔と"
        "メジャーラインは設定 (**{{MrEinstGruppe}}**) にもあります。ModView の灰色の床 (**{{MrGrauBoden}}**) は別のスイッチのままで、グリッドと"
        "一緒に表示できます。",
        "Cada vista muestra de serie una rejilla como la rejilla de inicio de 3ds Max: líneas finas a la distancia de la rejilla, líneas "
        "principales más oscuras y los ejes del mundo en negro. En perspectiva está sobre el suelo y termina tras 7 líneas (Max: Perspective "
        "View Grid Extent); en las vistas Superior, Frontal, Izquierda ... está en el plano de la vista, llena toda la vista y cambia con el "
        "zoom: al alejarte, las líneas principales pasan a ser las nuevas líneas; al acercarte nunca es más fina que la separación, como en "
        "Max. La figura tapa la rejilla. **[+] > {{MkRaster}}** (<kbd>G</kbd>) la activa en esta vista. **[+] > {{MrRasterEinst}}** ajusta "
        "**{{MrAbstand}}** (8 unidades de JKA, como la rejilla del editor de mapas), **{{MrHaupt}}** (8: una línea principal cada 64 unidades, "
        "la altura de un jugador), **{{MrAusdehnung}}** y la **{{MbAnBodenZ}}**: por defecto -24, los pies de una figura de JKA. 3ds Max y "
        "Blender ponen su rejilla en Z = 0, pero una figura de JKA tiene su origen en la cadera. **{{MrTiefster}}** pone el suelo bajo el punto "
        "más bajo de la figura en su pose (accesorios con el origen en el suelo: 0). Separación y líneas principales también están en los "
        "ajustes (**{{MrEinstGruppe}}**). El suelo gris de ModView (**{{MrGrauBoden}}**) sigue siendo un interruptor propio y se puede mostrar "
        "junto con la rejilla.",
        "Každý pohled od začátku ukazuje mřížku jako domovská mřížka 3ds Max: tenké čáry po rozteči mřížky, tmavší hlavní čáry a osy světa "
        "černě. V perspektivě leží na podlaze a končí po 7 čarách (Max: Perspective View Grid Extent); v pohledech Shora, Zepředu, Zleva ... "
        "leží v rovině pohledu, vyplní celý pohled a mění se s přiblížením - při oddálení se z hlavních čar stanou nové čáry, při přiblížení "
        "nikdy není jemnější než rozteč, jako v Max. Postava mřížku zakrývá. **[+] > {{MkRaster}}** (<kbd>G</kbd>) ji přepíná pro tento "
        "pohled. **[+] > {{MrRasterEinst}}** nastavuje **{{MrAbstand}}** (8 jednotek JKA jako mřížka editoru map), **{{MrHaupt}}** (8: hlavní "
        "čára každých 64 jednotek - výška hráče), **{{MrAusdehnung}}** a **{{MbAnBodenZ}}**: výchozí -24, chodidla postavy JKA. 3ds Max a "
        "Blender dávají mřížku na Z = 0, postava JKA má ale počátek v bocích. **{{MrTiefster}}** položí podlahu pod nejnižší bod postavy v její "
        "póze (rekvizity s počátkem na podlaze: 0). Rozteč a hlavní čáry jsou i v nastavení (**{{MrEinstGruppe}}**). Šedá podlaha z ModView "
        "(**{{MrGrauBoden}}**) zůstává samostatným přepínačem a lze ji zobrazit spolu s mřížkou."),
    "bild_max": ("3ds Max mode: the home grid in the perspective view with the black world axes, the menu [+] with the grid settings.",
                 "Modus wie 3ds Max: das Home Grid in der Perspektive mit den schwarzen Weltachsen, das Menü [+] mit den Rastereinstellungen.",
                 "3ds Max 模式: 透视图中带黑色世界坐标轴的主栅格, [+] 菜单中的栅格设置。",
                 "3ds Max モード: パースのホームグリッドと黒いワールド軸、[+] メニューのグリッド設定。",
                 "Modo 3ds Max: la rejilla de inicio en perspectiva con los ejes del mundo en negro, el menú [+] con los ajustes de la rejilla.",
                 "Režim 3ds Max: domovská mřížka v perspektivě s černými osami světa, nabídka [+] s nastavením mřížky."),
    "bild_vier": ("The four views: in Top, Front and Left the grid fills the view behind the figure, in the perspective it lies on the floor.",
                  "Die vier Ansichten: in Oben, Vorne und Links füllt das Raster die Ansicht hinter der Figur, in der Perspektive liegt es auf dem Boden.",
                  "四视图: 顶、前、左视图中栅格在角色后方填满视图, 透视图中位于地面上。",
                  "4 ビュー: 上・前・左ではグリッドがフィギュアの後ろでビューを埋め、パースでは床の上にあります。",
                  "Las cuatro vistas: en Superior, Frontal e Izquierda la rejilla llena la vista detrás de la figura; en perspectiva está sobre el suelo.",
                  "Čtyři pohledy: v pohledech Shora, Zepředu a Zleva mřížka vyplní pohled za postavou, v perspektivě leží na podlaze."),
    "blender": (
        "In Blender mode the grid looks and behaves like Blender's overlays: the floor fades out towards the horizon and seems endless, the X "
        "axis is red and the Y axis green (Z blue when you switch it on); in the side views the subdivisions change with the zoom. "
        "**{{MkBlOverlays}} > {{MrHilfslinien}}** has **{{MkRaster}}** (the side views), **{{MrBoden}}** (perspective), **{{MrAchsen}}** X Y Z, "
        "**{{MrMassstab}}** and **{{MrUnterteil}}** - each view keeps its own switches; scale and subdivisions are the spacing and the major "
        "lines from above.",
        "Im Modus wie Blender sieht das Raster aus und verhält sich wie Blenders Overlays: der Boden blendet zum Horizont aus und wirkt "
        "unendlich, die X-Achse ist rot, die Y-Achse grün (Z blau, wenn du sie einschaltest); in den Seitenansichten wechseln die "
        "Unterteilungen mit dem Zoom. **{{MkBlOverlays}} > {{MrHilfslinien}}** hat **{{MkRaster}}** (die Seitenansichten), **{{MrBoden}}** "
        "(Perspektive), **{{MrAchsen}}** X Y Z, **{{MrMassstab}}** und **{{MrUnterteil}}** - jede Ansicht behält ihre eigenen Schalter; "
        "Maßstab und Unterteilungen sind der Abstand und die Hauptlinien von oben.",
        "在 Blender 模式下, 栅格的外观和行为与 Blender 的叠加层相同: 地面向地平线淡出, 看起来无边无际, X 轴为红色, Y 轴为绿色 (开启后 Z 为蓝色); "
        "侧视图中细分随缩放变化。**{{MkBlOverlays}} > {{MrHilfslinien}}** 包含 **{{MkRaster}}** (侧视图)、**{{MrBoden}}** (透视)、**{{MrAchsen}}** "
        "X Y Z、**{{MrMassstab}}** 和 **{{MrUnterteil}}**——每个视图保留自己的开关; 缩放和细分即上文的间距和主线。",
        "Blender モードでは、グリッドの見た目と動作は Blender のオーバーレイと同じです: 床は地平線に向かってフェードアウトして果てしなく見え、X 軸は赤、"
        "Y 軸は緑 (オンにすると Z は青)。側面ビューでは細分化がズームで変わります。**{{MkBlOverlays}} > {{MrHilfslinien}}** には **{{MkRaster}}** "
        "(側面ビュー)、**{{MrBoden}}** (パース)、**{{MrAchsen}}** X Y Z、**{{MrMassstab}}**、**{{MrUnterteil}}** があります - ビューごとに"
        "スイッチを持ち、スケールと細分化は上の間隔とメジャーラインです。",
        "En modo Blender la rejilla se ve y se comporta como las superposiciones de Blender: el suelo se desvanece hacia el horizonte y parece "
        "infinito, el eje X es rojo y el eje Y verde (Z azul si lo activas); en las vistas laterales las subdivisiones cambian con el zoom. "
        "**{{MkBlOverlays}} > {{MrHilfslinien}}** tiene **{{MkRaster}}** (las vistas laterales), **{{MrBoden}}** (perspectiva), **{{MrAchsen}}** "
        "X Y Z, **{{MrMassstab}}** y **{{MrUnterteil}}**; cada vista conserva sus propios interruptores; escala y subdivisiones son la "
        "separación y las líneas principales de arriba.",
        "V režimu Blender mřížka vypadá a chová se jako překryvy Blenderu: podlaha k horizontu slábne a působí nekonečně, osa X je červená a "
        "osa Y zelená (Z modrá, když ji zapnete); v bočních pohledech se dělení mění s přiblížením. **{{MkBlOverlays}} > {{MrHilfslinien}}** "
        "má **{{MkRaster}}** (boční pohledy), **{{MrBoden}}** (perspektiva), **{{MrAchsen}}** X Y Z, **{{MrMassstab}}** a **{{MrUnterteil}}** - "
        "každý pohled si drží své přepínače; měřítko a dělení jsou rozteč a hlavní čáry z výše uvedeného."),
    "bild_bl": ("Blender mode: the floor with the red X and the green Y axis fades out towards the horizon, the menu Overlays with the guides.",
                "Modus wie Blender: der Boden mit roter X- und grüner Y-Achse blendet zum Horizont aus, das Menü Overlays mit den Hilfslinien.",
                "Blender 模式: 带红色 X 轴和绿色 Y 轴的地面向地平线淡出, 叠加层菜单中的参考线。",
                "Blender モード: 赤い X 軸と緑の Y 軸のある床が地平線に向かってフェードアウト、オーバーレイメニューのガイド。",
                "Modo Blender: el suelo con el eje X rojo y el eje Y verde se desvanece hacia el horizonte; el menú Superposiciones con las guías.",
                "Režim Blender: podlaha s červenou osou X a zelenou osou Y k horizontu slábne, nabídka Překryvy s vodítky."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def bloecke():
    """Die Bloecke des Abschnitts (in inhalt_mb1a1.seite nach dem Abschnitt von inhalt_mbkopf)."""
    return [
        ("h2", "raster", _t("h_raster")),
        ("p", _t("max")),
        ("img", "h_mbr_max", _t("bild_max"), "breit"),
        ("img", "h_mbr_vier", _t("bild_vier"), "breit"),
        ("p", _t("blender")),
        ("img", "h_mbr_blender", _t("bild_bl"), "breit"),
    ]
