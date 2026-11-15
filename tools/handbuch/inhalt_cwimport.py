"""CW-IMPORT: Teil 2 der Clone-Wars-Kette (plaene/clonewars_kette.md) - das Pruefprofil "Clone-Wars-Kette" (ab Werk aus), die
Testfigur, Massstab und Achsen beim Import, der Massstab je Raum, Treppe zu Clip-Rampe, glatte Clip-Wand und die Ebene Collision.

Haengt an die Seite karte_optimierer (inhalt_karte5.py, nach dem Abschnitt von inhalt_cwpruef.py) einen Absatz zum Pruefprofil.
Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_cwpruef.py.
"""

X = {
    "profil": (
        "**Only with the check profile Clone Wars chain** (off by default; **Build > {{KeCwMenue}}** or **Preferences > Optimizer > "
        "{{KeCwProfil}}**). With **{{KeCwProfilStandard}}** the optimizer reports exactly what it reported before these checks; only the "
        "list of shaders and images stays in the text report, because it only informs. The profile also shows **{{KeOptJson}}** and the "
        "sizes below, and puts the clip brushes of \"Model + clip\" into the layer Collision.",
        "**Nur mit dem Prüfprofil Clone-Wars-Kette** (ab Werk aus; **Build > {{KeCwMenue}}** oder **Einstellungen > Optimizer > "
        "{{KeCwProfil}}**). Mit **{{KeCwProfilStandard}}** meldet der Optimierer genau, was er vor diesen Prüfungen meldete; nur die "
        "Liste der Shader und Bilder bleibt im Textbericht, weil sie nur informiert. Das Profil zeigt außerdem **{{KeOptJson}}** und die "
        "Maße unten und legt die Clip-Pinsel von \"Modell + Clip\" in die Ebene Collision.",
        "**仅在检查配置为 Clone Wars 流程时**(默认关闭;**Build > {{KeCwMenue}}** 或 **首选项 > Optimizer > {{KeCwProfil}}**)。"
        "选择 **{{KeCwProfilStandard}}** 时,优化器报告的内容与加入这些检查之前完全相同;只有文本报告中的着色器和图像列表保留,因为它只用于说明。"
        "该配置还会显示 **{{KeOptJson}}** 和下面的尺寸,并把“模型 + clip”的 clip 笔刷放入图层 Collision。",
        "**チェックプロファイルが Clone Wars チェーンのときだけ**(既定ではオフ。**Build > {{KeCwMenue}}** または **設定 > Optimizer > "
        "{{KeCwProfil}}**)。**{{KeCwProfilStandard}}** では、最適化ツールはこれらのチェックが入る前とまったく同じ内容を報告します。"
        "テキストレポートのシェーダーと画像の一覧だけは情報なので残ります。このプロファイルでは **{{KeOptJson}}** と下の寸法も表示され、"
        "「モデル + クリップ」のクリップブラシはレイヤー Collision に入ります。",
        "**Solo con el perfil de comprobación Cadena Clone Wars** (desactivado por defecto; **Build > {{KeCwMenue}}** o **Preferencias > "
        "Optimizer > {{KeCwProfil}}**). Con **{{KeCwProfilStandard}}** el optimizador informa exactamente lo mismo que antes de estas "
        "comprobaciones; solo la lista de shaders e imágenes queda en el informe de texto, porque solo informa. El perfil muestra además "
        "**{{KeOptJson}}** y las medidas de abajo, y pone los brushes clip de \"Modelo + clip\" en la capa Collision.",
        "**Jen s profilem kontrol Řetězec Clone Wars** (ve výchozím stavu vypnuto; **Build > {{KeCwMenue}}** nebo **Předvolby > "
        "Optimizer > {{KeCwProfil}}**). S **{{KeCwProfilStandard}}** hlásí optimalizátor přesně to, co hlásil před těmito kontrolami; v "
        "textové zprávě zůstane jen seznam shaderů a obrázků, protože jen informuje. Profil navíc zobrazí **{{KeOptJson}}** a rozměry níže "
        "a vloží clip brushe z \"Model + clip\" do vrstvy Collision."),
}


# --- Seite "Karten-Editor: Clone-Wars-Kette" (CW-IMPORT): Testfigur, Import mit Massstab und Achsen, Werkzeuge, Ebene Collision ---
S = {
    "titel": (
        "Map editor: test figure, scale and clip tools", "Karten-Editor: Testfigur, Maßstab und Clip-Werkzeuge", "地图编辑器:测试人物、比例与 clip 工具",
        "マップエディター:テストフィギュア、縮尺、クリップツール", "Editor de mapas: figura de prueba, escala y herramientas clip",
        "Editor map: testovací postava, měřítko a clip nástroje"),
    "kurz": (
        "Tools from the requirements of a Clone Wars mod: a player figure in original size, models imported with the right scale and axes, "
        "rooms scaled with a note in the map, smooth collision for stairs and walls. Everything is off by default or acts only when you call it.",
        "Werkzeuge aus den Anforderungen eines Clone-Wars-Mods: eine Spielfigur in Originalgröße, Modelle mit dem richtigen Maßstab und den "
        "richtigen Achsen importieren, Räume mit einem Vermerk in der Karte skalieren, glatte Kollision für Treppen und Wände. Alles ist ab "
        "Werk aus oder wirkt nur, wenn man es aufruft.",
        "来自一个 Clone Wars 模组需求的工具:原始尺寸的玩家人物、以正确比例和坐标轴导入模型、在地图中加注缩放房间、为楼梯和墙提供平滑碰撞。"
        "所有功能默认关闭,或只在调用时才起作用。",
        "Clone Wars MOD の要件から来たツールです: 実物大のプレイヤーフィギュア、正しい縮尺と軸でのモデルのインポート、マップに記録を残す部屋の"
        "拡大縮小、階段と壁の滑らかなコリジョン。どれも既定ではオフか、呼び出したときだけ働きます。",
        "Herramientas de los requisitos de un mod de Clone Wars: una figura del jugador a tamaño original, modelos importados con la escala "
        "y los ejes correctos, salas escaladas con una nota en el mapa, colisión lisa para escaleras y paredes. Todo está desactivado por "
        "defecto o actúa solo cuando lo llamas.",
        "Nástroje z požadavků módu Clone Wars: postava hráče ve skutečné velikosti, import modelů se správným měřítkem a osami, změna měřítka "
        "místností s poznámkou v mapě, hladká kolize pro schody a stěny. Vše je ve výchozím stavu vypnuté nebo působí jen při vyvolání."),
    "h_figur": ("Test figure", "Testfigur", "测试人物", "テストフィギュア", "Figura de prueba", "Testovací postava"),
    "figur": (
        "**{{KeLView}} > {{KeCwFigurMenue}} > {{KeCwFigur}}** (F7, or the button at the end of the toolbar) shows the player in original "
        "size in the 2D views and the camera: the box the engine moves (32 x 32 x 64 in single player and Movie Duels, 30 wide in "
        "multiplayer; the origin at hip height, the feet 24 below; crouched the top is at 16 instead of 40 - OpenJK g_client.cpp and "
        "bg_public.h), the STEPSIZE mark 18 above the feet, the other height, and frames for the minimum door and corridor of the "
        "optimizer. In the camera the game's player model stands in it, posed like the game when the player stands still (crouched the "
        "held last frame of the crouch animation).",
        "**{{KeLView}} > {{KeCwFigurMenue}} > {{KeCwFigur}}** (F7 oder der Knopf am Ende der Werkzeugleiste) zeigt die Spielfigur in "
        "Originalgröße in den 2D-Ansichten und in der Kamera: den Kasten, den die Engine bewegt (32 x 32 x 64 im Einzelspieler und in "
        "Movie Duels, 30 breit im Mehrspieler; der Ursprung in Hüfthöhe, die Füße 24 darunter; geduckt liegt die Oberkante bei 16 statt 40 - "
        "OpenJK g_client.cpp und bg_public.h), die Marke STEPSIZE 18 über den Füßen, die andere Höhe und Rahmen für die Mindestmaße von Tür "
        "und Gang aus dem Optimierer. In der Kamera steht das Spielermodell des Spiels darin, in der Pose, die das Spiel im Stillstand zeigt "
        "(geduckt das gehaltene letzte Bild der Duck-Animation).",
        "**{{KeLView}} > {{KeCwFigurMenue}} > {{KeCwFigur}}**(F7,或工具栏末尾的按钮)在 2D 视图和相机中以原始尺寸显示玩家:引擎移动的盒子(单人和 "
        "Movie Duels 为 32 x 32 x 64,多人模式宽 30;原点在髋部高度,脚在其下 24;蹲下时顶部在 16 而不是 40——OpenJK g_client.cpp 和 "
        "bg_public.h)、脚上方 18 的 STEPSIZE 标记、另一种高度,以及优化器中门和走廊最小尺寸的框。在相机中,游戏的玩家模型站在其中,姿势与玩家静止时"
        "游戏中的姿势相同(蹲下时为蹲下动画保持的最后一帧)。",
        "**{{KeLView}} > {{KeCwFigurMenue}} > {{KeCwFigur}}**(F7、またはツールバー末尾のボタン)で、2D ビューとカメラに実物大のプレイヤーを"
        "表示します: エンジンが動かす箱(シングルプレイと Movie Duels は 32 x 32 x 64、マルチプレイは幅 30。原点は腰の高さで足はその 24 下。"
        "しゃがむと上端は 40 ではなく 16 - OpenJK g_client.cpp と bg_public.h)、足の 18 上の STEPSIZE の印、もう一方の高さ、最適化ツールの"
        "ドアと廊下の最小寸法の枠です。カメラではゲームのプレイヤーモデルが、立ち止まったときのゲームと同じポーズで立ちます(しゃがみはしゃがむ"
        "アニメーションの保持された最後のフレーム)。",
        "**{{KeLView}} > {{KeCwFigurMenue}} > {{KeCwFigur}}** (F7 o el botón al final de la barra de herramientas) muestra al jugador a "
        "tamaño original en las vistas 2D y en la cámara: la caja que mueve el motor (32 x 32 x 64 en un jugador y en Movie Duels, 30 de "
        "ancho en multijugador; el origen a la altura de la cadera, los pies 24 por debajo; agachado la parte superior está a 16 en lugar de "
        "40; OpenJK g_client.cpp y bg_public.h), la marca STEPSIZE 18 sobre los pies, la otra altura y marcos para las medidas mínimas de "
        "puerta y pasillo del optimizador. En la cámara, el modelo del jugador del juego está dentro, en la pose que muestra el juego cuando "
        "el jugador está quieto (agachado, el último fotograma mantenido de la animación de agacharse).",
        "**{{KeLView}} > {{KeCwFigurMenue}} > {{KeCwFigur}}** (F7 nebo tlačítko na konci panelu nástrojů) zobrazí hráče ve skutečné velikosti "
        "ve 2D pohledech a v kameře: kvádr, kterým engine pohybuje (32 x 32 x 64 v single playeru a Movie Duels, 30 široký v multiplayeru; "
        "počátek ve výšce boků, chodidla 24 pod ním; přikrčená postava má horní hranu ve 16 místo 40 - OpenJK g_client.cpp a bg_public.h), "
        "značku STEPSIZE 18 nad chodidly, druhou výšku a rámy minimálních rozměrů dveří a chodby z optimalizátoru. V kameře v něm stojí model "
        "hráče ze hry v póze, jakou hra ukazuje, když hráč stojí (přikrčená postava v podrženém posledním snímku animace přikrčení)."),
    "figur_platz": (
        "**Shift+F7** puts the figure under the mouse pointer: in a 2D view on the two axes of the view (the depth stays), in the camera with "
        "its feet on the surface. A right click in a view offers **{{KeCwFigurHierMenue}}**, a right click on the toolbar button the whole "
        "menu. Off by default; the choices are remembered.",
        "**Shift+F7** stellt die Figur unter den Mauszeiger: in einer 2D-Ansicht auf die beiden Achsen der Ansicht (die Tiefe bleibt), in der "
        "Kamera mit den Füßen auf die Fläche. Ein Rechtsklick in einer Ansicht bietet **{{KeCwFigurHierMenue}}**, ein Rechtsklick auf den "
        "Knopf der Werkzeugleiste das ganze Menü. Ab Werk aus; die Wahl wird gemerkt.",
        "**Shift+F7** 把人物放到鼠标指针下:在 2D 视图中放到视图的两条轴上(深度不变),在相机中双脚落在表面上。在视图中右键单击会提供 "
        "**{{KeCwFigurHierMenue}}**,在工具栏按钮上右键单击会显示整个菜单。默认关闭;所做选择会被记住。",
        "**Shift+F7** でフィギュアをマウスポインターの下に置きます: 2D ビューではビューの 2 軸上(奥行きはそのまま)、カメラでは足を面の上に。"
        "ビューを右クリックすると **{{KeCwFigurHierMenue}}**、ツールバーのボタンを右クリックするとメニュー全体が出ます。既定ではオフ。選択は記憶されます。",
        "**Mayús+F7** pone la figura bajo el puntero: en una vista 2D sobre los dos ejes de la vista (la profundidad se mantiene), en la "
        "cámara con los pies sobre la superficie. Un clic derecho en una vista ofrece **{{KeCwFigurHierMenue}}**, un clic derecho en el "
        "botón de la barra todo el menú. Desactivada por defecto; las opciones se recuerdan.",
        "**Shift+F7** postaví postavu pod ukazatel myši: ve 2D pohledu na obě osy pohledu (hloubka zůstane), v kameře chodidly na plochu. "
        "Pravé kliknutí v pohledu nabídne **{{KeCwFigurHierMenue}}**, pravé kliknutí na tlačítko panelu celou nabídku. Ve výchozím stavu "
        "vypnuto; volby se pamatují."),
    "h_import": ("Import with scale and axes", "Import mit Maßstab und Achsen", "按比例和坐标轴导入", "縮尺と軸を指定したインポート",
                 "Importar con escala y ejes", "Import s měřítkem a osami"),
    "import": (
        "The dialog of **File > Import geometry** has a **{{KeCwVorlage}}**: **{{KeCwVorHeute}}** reads the file exactly as before (the "
        "default), **{{KeCwVorBf3}}** turns Y up into Z up as (x, -z, y), **{{KeCwVorBf2}}** keeps the axes, **{{KeCwVorSource}}** keeps them "
        "and scales by 64 / 72 (the player is 72 units tall in Source SDK 2013, 64 here), **{{KeCwVorEigene}}** lets you choose the file axis "
        "for X, Y and Z. **{{KeCwUmdrehen}}** is for models whose triangles run the other way round (Empire at War). The dialog shows the "
        "size in units and in test figures, **{{KeCwAufFigur}}** scales a character model to 64, and a front view shows the model next to "
        "the test figure. The template is remembered; it also applies when you drop a file onto a view.",
        "Der Dialog von **Datei > Geometrie importieren** hat eine **{{KeCwVorlage}}**: **{{KeCwVorHeute}}** liest die Datei genau wie "
        "bisher (ab Werk), **{{KeCwVorBf3}}** macht Y oben zu Z oben als (x, -z, y), **{{KeCwVorBf2}}** lässt die Achsen, **{{KeCwVorSource}}** "
        "lässt sie und skaliert mit 64 / 72 (die Spielfigur ist im Source SDK 2013 72 Einheiten hoch, hier 64), **{{KeCwVorEigene}}** lässt "
        "die Achse der Datei für X, Y und Z wählen. **{{KeCwUmdrehen}}** ist für Modelle, deren Dreiecke andersherum laufen (Empire at War). "
        "Der Dialog zeigt die Größe in Einheiten und in Testfiguren, **{{KeCwAufFigur}}** skaliert ein Figurenmodell auf 64, und eine "
        "Vorderansicht zeigt das Modell neben der Testfigur. Die Vorlage wird gemerkt; sie gilt auch, wenn man eine Datei auf eine Ansicht zieht.",
        "**文件 > 导入几何体** 对话框中有 **{{KeCwVorlage}}**:**{{KeCwVorHeute}}** 完全按以前的方式读取文件(默认),**{{KeCwVorBf3}}** 把 Y 向上"
        "转换为 Z 向上,即 (x, -z, y),**{{KeCwVorBf2}}** 保持坐标轴不变,**{{KeCwVorSource}}** 保持坐标轴并按 64 / 72 缩放(Source SDK 2013 中玩家高"
        " 72 单位,这里是 64),**{{KeCwVorEigene}}** 允许为 X、Y、Z 选择文件的轴。**{{KeCwUmdrehen}}** 用于三角形环绕方向相反的模型(Empire at War)。"
        "对话框显示以单位和测试人物计的尺寸,**{{KeCwAufFigur}}** 把角色模型缩放到 64,正视图显示模型和旁边的测试人物。模板会被记住;把文件拖到视图上时也会使用它。",
        "**ファイル > ジオメトリをインポート** のダイアログには **{{KeCwVorlage}}** があります: **{{KeCwVorHeute}}** はこれまでどおりに読み(既定)、"
        "**{{KeCwVorBf3}}** は Y が上を Z が上に (x, -z, y) として変え、**{{KeCwVorBf2}}** は軸をそのまま、**{{KeCwVorSource}}** は軸をそのままにして "
        "64 / 72 倍します(Source SDK 2013 ではプレイヤーは 72 単位、ここでは 64)。**{{KeCwVorEigene}}** では X、Y、Z に使うファイルの軸を選べます。"
        "**{{KeCwUmdrehen}}** は三角形が逆向きに巡るモデル向けです(Empire at War)。ダイアログは単位とテストフィギュア何体分かでサイズを示し、"
        "**{{KeCwAufFigur}}** はキャラクターモデルを 64 にし、正面図でモデルをテストフィギュアの横に表示します。テンプレートは記憶され、ファイルを"
        "ビューにドロップしたときにも使われます。",
        "El diálogo de **Archivo > Importar geometría** tiene una **{{KeCwVorlage}}**: **{{KeCwVorHeute}}** lee el archivo exactamente como "
        "antes (por defecto), **{{KeCwVorBf3}}** convierte Y arriba en Z arriba como (x, -z, y), **{{KeCwVorBf2}}** deja los ejes, "
        "**{{KeCwVorSource}}** los deja y escala por 64 / 72 (el jugador mide 72 unidades en Source SDK 2013, aquí 64), **{{KeCwVorEigene}}** "
        "permite elegir el eje del archivo para X, Y y Z. **{{KeCwUmdrehen}}** es para modelos cuyos triángulos van al revés (Empire at War). "
        "El diálogo muestra el tamaño en unidades y en figuras de prueba, **{{KeCwAufFigur}}** escala un modelo de personaje a 64 y una vista "
        "frontal muestra el modelo junto a la figura de prueba. La plantilla se recuerda; también vale al soltar un archivo sobre una vista.",
        "Dialog **Soubor > Importovat geometrii** má **{{KeCwVorlage}}**: **{{KeCwVorHeute}}** čte soubor přesně jako dosud (výchozí), "
        "**{{KeCwVorBf3}}** převede Y nahoru na Z nahoru jako (x, -z, y), **{{KeCwVorBf2}}** ponechá osy, **{{KeCwVorSource}}** je ponechá a "
        "změní měřítko o 64 / 72 (hráč je v Source SDK 2013 vysoký 72 jednotek, zde 64), **{{KeCwVorEigene}}** dovolí zvolit osu souboru pro "
        "X, Y a Z. **{{KeCwUmdrehen}}** je pro modely, jejichž trojúhelníky běží opačně (Empire at War). Dialog ukáže velikost v jednotkách a "
        "v testovacích postavách, **{{KeCwAufFigur}}** změní model postavy na 64 a pohled zepředu ukáže model vedle testovací postavy. "
        "Šablona se pamatuje; platí i při přetažení souboru na pohled."),
    "h_werkzeuge": ("Clip tools and the layer Collision", "Clip-Werkzeuge und die Ebene Collision", "Clip 工具与图层 Collision",
                    "クリップツールとレイヤー Collision", "Herramientas clip y la capa Collision", "Clip nástroje a vrstva Collision"),
    "werkzeuge": (
        "**{{KeLModify}} > {{KeCwWzMenue}}** (also at the top of the right-click menu of a view when something is selected) works on the "
        "selection, one undo step each:",
        "**{{KeLModify}} > {{KeCwWzMenue}}** (auch oben im Rechtsklickmenü einer Ansicht, wenn etwas gewählt ist) arbeitet auf der Auswahl, "
        "je eine Undo-Stufe:",
        "**{{KeLModify}} > {{KeCwWzMenue}}**(选中内容时也在视图右键菜单顶部)作用于所选内容,每次一个撤销步骤:",
        "**{{KeLModify}} > {{KeCwWzMenue}}**(何かを選択しているときはビューの右クリックメニューの上部にもあります)は選択範囲に働き、それぞれ 1 回の元に戻す操作です:",
        "**{{KeLModify}} > {{KeCwWzMenue}}** (también arriba en el menú contextual de una vista cuando hay algo seleccionado) trabaja sobre "
        "la selección, un paso de deshacer cada uno:",
        "**{{KeLModify}} > {{KeCwWzMenue}}** (také nahoře v nabídce pravého kliknutí v pohledu, když je něco vybráno) pracuje s výběrem, vždy "
        "jeden krok zpět:"),
    "rampe": (
        "**{{KeCwRampe}}:** select stairs made of brushes or a misc_model. A smooth ramp of textures/system/clip (player and NPCs, shots still "
        "hit the steps) runs through the front edges of the steps, from the bottom of the stairs to the top step. The visible steps lose "
        "their collision the way q3map2 understands it: a hidden side (caulk, otherwise the bottom) becomes system/nodraw, which is "
        "non-solid, and one non-solid side makes the whole brush passable; a model loses its clip spawnflags; pure collision brushes of the "
        "selection are removed. Built with q3map2, the player walks up without stepping; the optimizer check sight against collision "
        "reports nothing there. A ramp steeper than walkable (MIN_WALK_NORMAL 0.7) gets a warning.",
        "**{{KeCwRampe}}:** eine Treppe aus Brushes oder ein misc_model wählen. Eine glatte Rampe aus textures/system/clip (Spielfigur und "
        "NPCs; Schüsse treffen weiter die Stufen) läuft durch die Vorderkanten der Stufen, von der Unterkante der Treppe bis zum obersten "
        "Tritt. Die sichtbaren Stufen verlieren ihre Kollision so, wie q3map2 es versteht: eine verdeckte Seite (caulk, sonst die untere) "
        "wird system/nodraw, das nicht fest ist, und eine nicht feste Seite macht den ganzen Pinsel durchlässig; ein Modell verliert seine "
        "Clip-Spawnflags; reine Kollisionspinsel der Auswahl fallen weg. Mit q3map2 gebaut läuft die Spielfigur ohne Stufensprünge hinauf; "
        "die Prüfung Sicht gegen Kollision des Optimierers meldet dort nichts. Eine Rampe, steiler als begehbar (MIN_WALK_NORMAL 0,7), "
        "bekommt eine Warnung.",
        "**{{KeCwRampe}}:**选择由画刷构成的楼梯或一个 misc_model。一个由 textures/system/clip 构成的平滑坡道(玩家和 NPC;射击仍会打到台阶)从楼梯"
        "底部穿过各级台阶的前缘到最上面的踏面。可见台阶按 q3map2 的理解失去碰撞:一个被遮挡的面(caulk,否则为底面)变为非实体的 system/nodraw,"
        "而只要有一面非实体,整个画刷就可穿过;模型失去其 clip 生成标志;所选内容中的纯碰撞画刷被删除。用 q3map2 编译后,玩家会平滑地走上去而不是一级"
        "级跳;优化器的视觉与碰撞检查在那里不再报告问题。比可行走更陡(MIN_WALK_NORMAL 0.7)的坡道会得到警告。",
        "**{{KeCwRampe}}:** ブラシの階段か misc_model を選びます。textures/system/clip の滑らかなスロープ(プレイヤーと NPC。弾は引き続き段に当たり"
        "ます)が、階段の下端から一番上の踏み面まで段の前縁を通ります。見える段は q3map2 の解釈どおりにコリジョンを失います: 隠れた面(caulk、"
        "なければ底面)が非固体の system/nodraw になり、非固体の面が 1 つあればブラシ全体を通り抜けられます。モデルは clip のスポーンフラグを失い、"
        "選択範囲内の純粋なコリジョンブラシは削除されます。q3map2 でビルドすると、プレイヤーは段ごとに跳ねずに上ります。最適化ツールの見た目と"
        "コリジョンのチェックもそこでは何も報告しません。歩ける勾配(MIN_WALK_NORMAL 0.7)より急なスロープには警告が出ます。",
        "**{{KeCwRampe}}:** selecciona una escalera de brushes o un misc_model. Una rampa lisa de textures/system/clip (jugador y NPC; los "
        "disparos siguen dando en los escalones) pasa por los bordes delanteros de los escalones, desde la base de la escalera hasta el "
        "escalón superior. Los escalones visibles pierden su colisión como lo entiende q3map2: una cara oculta (caulk, si no la inferior) "
        "pasa a system/nodraw, que no es sólido, y una cara no sólida hace atravesable todo el brush; un modelo pierde sus spawnflags de "
        "clip; los brushes de solo colisión de la selección se eliminan. Compilado con q3map2, el jugador sube sin saltos de escalón; la "
        "comprobación vista frente a colisión del optimizador no informa nada allí. Una rampa más empinada de lo transitable "
        "(MIN_WALK_NORMAL 0,7) recibe una advertencia.",
        "**{{KeCwRampe}}:** vyberte schody z brushů nebo misc_model. Hladká rampa z textures/system/clip (hráč a NPC; střely dál zasahují "
        "schody) vede přes přední hrany schodů, od spodku schodiště po nejvyšší stupeň. Viditelné schody ztratí kolizi tak, jak to chápe "
        "q3map2: jedna skrytá strana (caulk, jinak spodní) se změní na nepevnou system/nodraw a jediná nepevná strana udělá celý brush "
        "průchozím; model ztratí své clip spawnflagy; čistě kolizní brushe výběru se odstraní. Po sestavení q3map2 jde hráč nahoru bez "
        "skoků po schodech; kontrola vzhledu proti kolizi v optimalizátoru tam nic nehlásí. Rampa strmější, než se dá chodit "
        "(MIN_WALK_NORMAL 0,7), dostane varování."),
    "wand": (
        "**{{KeCwWandEbene}}** / **{{KeCwWandHuelle}}:** select an irregular wall (brushes, patches or a misc_model). The plane variant puts "
        "a clip block along the wall whose front lies at the point that sticks out most, on the side of the camera; the hull variant uses "
        "the convex hull of the selection.",
        "**{{KeCwWandEbene}}** / **{{KeCwWandHuelle}}:** eine unregelmäßige Wand wählen (Brushes, Patches oder ein misc_model). Die Ebene "
        "legt einen Clip-Quader entlang der Wand, dessen Vorderseite am Punkt liegt, der am weitesten vorsteht, auf der Seite der Kamera; die "
        "Hülle nimmt die konvexe Hülle der Auswahl.",
        "**{{KeCwWandEbene}}** / **{{KeCwWandHuelle}}:**选择一面不规则的墙(画刷、面片或 misc_model)。平面方式沿墙放置一个 clip 长方体,其前面位于"
        "最突出的点,在相机一侧;凸包方式使用所选内容的凸包。",
        "**{{KeCwWandEbene}}** / **{{KeCwWandHuelle}}:** 不規則な壁(ブラシ、パッチ、misc_model)を選びます。平面は壁に沿ってクリップの直方体を"
        "置き、その前面はカメラ側で最も出っ張った点に合わせます。凸包は選択範囲の凸包を使います。",
        "**{{KeCwWandEbene}}** / **{{KeCwWandHuelle}}:** selecciona una pared irregular (brushes, patches o un misc_model). El plano pone un "
        "bloque clip a lo largo de la pared con el frente en el punto que más sobresale, del lado de la cámara; la envolvente usa la "
        "envolvente convexa de la selección.",
        "**{{KeCwWandEbene}}** / **{{KeCwWandHuelle}}:** vyberte nepravidelnou stěnu (brushe, patche nebo misc_model). Rovina položí podél "
        "stěny clip kvádr, jehož přední strana leží v nejvíce vystupujícím bodě na straně kamery; obal použije konvexní obal výběru."),
    "raum": (
        "**{{KeCwRaum}}:** scales the selection (a room) by a factor about the center of the selection at the bottom; point entities move "
        "with it, models keep their size. An info_notnull at that point gets cw_scale (and cw_room), and the .map gets a comment line "
        "\"// cw_scale ...\" - data only, the game and q3map2 ignore both. **{{KeCwRaumSpeichern}}** then saves the room with its marker as "
        "its own part (File > Save selected).",
        "**{{KeCwRaum}}:** skaliert die Auswahl (einen Raum) mit einem Faktor um die Mitte der Auswahl unten; Punkt-Entities wandern mit, "
        "Modelle behalten ihre Größe. Ein info_notnull an diesem Punkt bekommt cw_scale (und cw_room), und die .map eine Kommentarzeile "
        "\"// cw_scale ...\" - nur Daten, Spiel und q3map2 übergehen beides. **{{KeCwRaumSpeichern}}** speichert danach den Raum samt Marker "
        "als eigenes Teil (Datei > Auswahl speichern).",
        "**{{KeCwRaum}}:**以所选内容底部中心为基准按系数缩放所选内容(一个房间);点实体随之移动,模型保持大小。该点处的 info_notnull 获得 cw_scale"
        "(和 cw_room),.map 中得到一行注释 \"// cw_scale ...\"——只是数据,游戏和 q3map2 都会忽略。**{{KeCwRaumSpeichern}}** 随后把房间连同标记另存为"
        "单独的部分(文件 > 保存所选)。",
        "**{{KeCwRaum}}:** 選択範囲(部屋)を、選択範囲の下端中央を中心に係数で拡大縮小します。ポイントエンティティは一緒に動き、モデルは大きさを"
        "保ちます。その点の info_notnull に cw_scale(と cw_room)が付き、.map にはコメント行 \"// cw_scale ...\" が入ります。データだけで、ゲームも"
        " q3map2 もどちらも無視します。**{{KeCwRaumSpeichern}}** はそのあと部屋を印ごと独立した部品として保存します(ファイル > 選択範囲を保存)。",
        "**{{KeCwRaum}}:** escala la selección (una sala) por un factor alrededor del centro de la selección abajo; las entidades de punto se "
        "mueven con ella, los modelos conservan su tamaño. Un info_notnull en ese punto recibe cw_scale (y cw_room), y el .map una línea de "
        "comentario \"// cw_scale ...\"; solo datos, el juego y q3map2 ignoran ambos. **{{KeCwRaumSpeichern}}** guarda después la sala con "
        "su marcador como pieza propia (Archivo > Guardar selección).",
        "**{{KeCwRaum}}:** změní měřítko výběru (místnosti) o faktor kolem středu výběru dole; bodové entity se posunou s ním, modely si "
        "ponechají velikost. Entita info_notnull v tom bodě dostane cw_scale (a cw_room) a .map řádek komentáře \"// cw_scale ...\" - jen "
        "data, hra i q3map2 obojí přeskočí. **{{KeCwRaumSpeichern}}** pak uloží místnost i se značkou jako samostatný díl (Soubor > Uložit "
        "výběr)."),
    "kollision": (
        "**Layer Collision:** the clip brushes of these tools - and of \"Model + clip\" with the check profile Clone Wars chain - go into "
        "their own layer named Collision. It only appears when one of them is used; **{{KeCwKollision}}** in the same menu (or the Layers "
        "browser) shows and hides it.",
        "**Ebene Collision:** die Clip-Pinsel dieser Werkzeuge - und von \"Modell + Clip\" mit dem Prüfprofil Clone-Wars-Kette - kommen in "
        "eine eigene Ebene namens Collision. Sie entsteht erst, wenn eines davon benutzt wird; **{{KeCwKollision}}** im selben Menü (oder der "
        "Ebenen-Browser) blendet sie ein und aus.",
        "**图层 Collision:**这些工具的 clip 画刷——以及检查配置为 Clone Wars 流程时“模型 + clip”的 clip 画刷——放入名为 Collision 的单独图层。只有"
        "使用其中之一时它才会出现;同一菜单中的 **{{KeCwKollision}}**(或图层浏览器)可以显示和隐藏它。",
        "**レイヤー Collision:** これらのツールのクリップブラシ(とチェックプロファイルが Clone Wars チェーンのときの「モデル + クリップ」のもの)は"
        "専用のレイヤー Collision に入ります。どれかを使ったときに初めて作られ、同じメニューの **{{KeCwKollision}}**(またはレイヤーブラウザー)で"
        "表示/非表示を切り替えます。",
        "**Capa Collision:** los brushes clip de estas herramientas (y de \"Modelo + clip\" con el perfil de comprobación Cadena Clone Wars) "
        "van a su propia capa llamada Collision. Solo aparece cuando se usa una de ellas; **{{KeCwKollision}}** en el mismo menú (o el "
        "navegador de capas) la muestra y la oculta.",
        "**Vrstva Collision:** clip brushe těchto nástrojů - a z \"Model + clip\" s profilem kontrol Řetězec Clone Wars - jdou do vlastní "
        "vrstvy jménem Collision. Vznikne teprve, když se některý z nich použije; **{{KeCwKollision}}** ve stejné nabídce (nebo prohlížeč "
        "vrstev) ji zobrazí a skryje."),
}

ES = {t[0]: t[4] for t in list(X.values()) + list(S.values())}
CS = {t[0]: t[5] for t in list(X.values()) + list(S.values())}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel] if schluessel in X else S[schluessel]
    return T(t[0], t[1], t[2], t[3])


def seite():
    """Die Seite karte_clonewars: Testfigur, Import mit Massstab und Achsen, Clip-Werkzeuge und die Ebene Collision."""
    return {
        "id": "karte_clonewars",
        "gruppe": "karte",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("h2", "testfigur", _t("h_figur")),
            ("p", _t("figur")),
            ("img", "h_cwimport_figur", _t("h_figur")),
            ("p", _t("figur_platz")),
            ("h2", "import", _t("h_import")),
            ("p", _t("import")),
            ("img", "h_cwimport_import", _t("h_import")),
            ("h2", "werkzeuge", _t("h_werkzeuge")),
            ("p", _t("werkzeuge")),
            ("list", [_t("rampe"), _t("wand"), _t("raum"), _t("kollision")]),
            ("img", "h_cwimport_rampe", _t("h_werkzeuge")),
        ],
    }


def einhaengen(optimierer):
    """Pruefprofil: ein Absatz gleich nach der Ueberschrift des Abschnitts von CW-PRUEF (karte_optimierer)."""
    import inhalt_cwpruef
    b = optimierer["bloecke"]
    titel = inhalt_cwpruef.X["titel"][0]
    h = next(n for n, x in enumerate(b) if x[0] == "h3" and getattr(x[1], "t", {}).get("en") == titel)
    b.insert(h + 1, ("p", _t("profil")))
