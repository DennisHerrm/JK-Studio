"""rc610 R3: weitere Shaderstufen und Glow im Echtzeit-Licht, gespiegelte Modelle (Tester 06.11.).

Tester: "glow texture should be seeable too" und "some models randomly renders inside (... when i flip the model vertically or
horizontal)". Haengt einen Absatz und ein Bild an die Seite karte_tempo (inhalt_karte7.py, nach dem Gras-Bild von R2a) und eine
Notiz zu gespiegelten Modellen an dieselbe Seite.

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_r2a.py.
"""

X = {
    "glow": (
        "**Further stages and glow:** besides the first stage with its light, the real-time light draws the further stages of a shader "
        "like the game (RB_StageIteratorGeneric) - for example `{ map $lightmap } { map texture blendFunc GL_DST_COLOR GL_ZERO } "
        "{ map texture_glw blendFunc GL_ONE GL_ONE glow }`: the first two become the lit texture, the third is added on top. Stages with "
        "`glow` also glow like in the game: they go into a glow buffer with the depth of the scene, which is blurred and laid over the "
        "picture with the same code as **{{KeSpielbildMenue}}**. Drawn are stages with a fixed image and without tcMod, tcGen and waves; "
        "the others stay away until the build. The editor light (without real-time light) shows only the editor image, like NetRadiant-Custom.",
        "**Weitere Stufen und Glow:** neben der ersten Stufe mit ihrem Licht zeichnet das Echtzeit-Licht die weiteren Stufen eines Shaders "
        "wie das Spiel (RB_StageIteratorGeneric) - zum Beispiel `{ map $lightmap } { map textur blendFunc GL_DST_COLOR GL_ZERO } "
        "{ map textur_glw blendFunc GL_ONE GL_ONE glow }`: die ersten beiden werden die beleuchtete Textur, die dritte kommt obendrauf. "
        "Stufen mit `glow` leuchten auch wie im Spiel: sie gehen mit der Tiefe der Szene in einen Leuchtpuffer, der mit demselben Code wie "
        "**{{KeSpielbildMenue}}** weichgezeichnet und über das Bild gelegt wird. Gezeichnet werden Stufen mit festem Bild ohne tcMod, tcGen "
        "und Wellen; die übrigen fehlen bis zum Bau. Das Editor-Licht (ohne Echtzeit-Licht) zeigt nur das Editorbild, wie NetRadiant-Custom.",
        "**后续阶段和发光:**除了带光照的第一阶段,实时光照还会像游戏一样(RB_StageIteratorGeneric)绘制着色器的后续阶段——例如 "
        "`{ map $lightmap } { map texture blendFunc GL_DST_COLOR GL_ZERO } { map texture_glw blendFunc GL_ONE GL_ONE glow }`:前两个阶段成为受光照的纹理,"
        "第三个叠加在上面。带 `glow` 的阶段也像游戏中一样发光:它们带着场景深度进入发光缓冲区,再用与 **{{KeSpielbildMenue}}** 相同的代码模糊并叠加到画面上。"
        "绘制的是使用固定图像、没有 tcMod、tcGen 和波形的阶段;其他阶段要到构建后才显示。编辑器光照(不开实时光照)只显示编辑器图像,与 NetRadiant-Custom 相同。",
        "**後続ステージとグロー:**リアルタイムライトは、光を受ける最初のステージに加えて、シェーダーの後続ステージもゲームと同じように"
        "(RB_StageIteratorGeneric)描きます。たとえば `{ map $lightmap } { map texture blendFunc GL_DST_COLOR GL_ZERO } "
        "{ map texture_glw blendFunc GL_ONE GL_ONE glow }` では、最初の 2 つが照らされたテクスチャになり、3 つ目が上に加わります。`glow` 付きのステージも"
        "ゲームと同じように光ります。シーンの深度を使ってグローバッファーに描かれ、**{{KeSpielbildMenue}}** と同じコードでぼかされて画面に重ねられます。"
        "描かれるのは固定画像で tcMod・tcGen・波形のないステージで、それ以外はビルドまで表示されません。エディターライト(リアルタイムライトなし)は"
        "NetRadiant-Custom と同じくエディター画像だけを表示します。",
        "**Etapas siguientes y glow:** además de la primera etapa con su luz, la luz en tiempo real dibuja las etapas siguientes de un shader "
        "como el juego (RB_StageIteratorGeneric); por ejemplo `{ map $lightmap } { map textura blendFunc GL_DST_COLOR GL_ZERO } "
        "{ map textura_glw blendFunc GL_ONE GL_ONE glow }`: las dos primeras forman la textura iluminada y la tercera se suma encima. Las "
        "etapas con `glow` también brillan como en el juego: van con la profundidad de la escena a un búfer de brillo que se desenfoca y se "
        "superpone a la imagen con el mismo código que **{{KeSpielbildMenue}}**. Se dibujan las etapas con imagen fija y sin tcMod, tcGen ni "
        "ondas; las demás faltan hasta compilar. La luz del editor (sin luz en tiempo real) muestra solo la imagen del editor, como NetRadiant-Custom.",
        "**Další fáze a glow:** kromě první fáze s jejím světlem kreslí světlo v reálném čase další fáze shaderu jako hra "
        "(RB_StageIteratorGeneric) - například `{ map $lightmap } { map textura blendFunc GL_DST_COLOR GL_ZERO } "
        "{ map textura_glw blendFunc GL_ONE GL_ONE glow }`: první dvě tvoří osvětlenou texturu, třetí se přidá navrch. Fáze s `glow` také "
        "září jako ve hře: jdou s hloubkou scény do zářícího bufferu, který se rozmaže a položí přes obraz stejným kódem jako "
        "**{{KeSpielbildMenue}}**. Kreslí se fáze s pevným obrázkem bez tcMod, tcGen a vln; ostatní chybí až do sestavení. Světlo editoru (bez "
        "světla v reálném čase) ukazuje jen obrázek editoru, jako NetRadiant-Custom."),
    "glow_bild": (
        "A panel with a glow shader in real-time light: the added glow texture and the soft glow around it.",
        "Eine Platte mit Glow-Shader im Echtzeit-Licht: die aufaddierte Leuchttextur und der weiche Schein darum.",
        "实时光照中使用发光着色器的面板:叠加的发光纹理及其周围柔和的光晕。",
        "リアルタイムライトでグローシェーダーを使ったパネル:加算されたグローテクスチャと、その周りの柔らかな光。",
        "Un panel con un shader de glow en la luz en tiempo real: la textura de brillo sumada y el resplandor suave alrededor.",
        "Deska se shaderem glow ve světle v reálném čase: přičtená zářící textura a měkká záře kolem ní."),
    "spiegeln": (
        "**Mirrored models:** after **Flip X/Y/Z** a model gets `modelscale_vec` with negative axes. A `misc_model` is turned around by "
        "q3map2 when it bakes it in, so the camera shows its outside in every view. Models the game loads at run time "
        "(`misc_model_static`, `misc_model_breakable`) are drawn by the game without turning them around - you see their inside; the camera "
        "shows them the same way, and the map check warns about them. Two mirrored axes are a rotation and look right everywhere.",
        "**Gespiegelte Modelle:** nach **Flip X/Y/Z** bekommt ein Modell `modelscale_vec` mit negativen Achsen. Ein `misc_model` dreht "
        "q3map2 beim Einbacken um, die Kamera zeigt es darum in jeder Ansicht von außen. Modelle, die das Spiel zur Laufzeit lädt "
        "(`misc_model_static`, `misc_model_breakable`), zeichnet das Spiel ohne Umdrehen - man sieht ihr Inneres; die Kamera zeigt sie "
        "ebenso, und die Kartenprüfung warnt davor. Zwei gespiegelte Achsen sind eine Drehung und sehen überall richtig aus.",
        "**镜像模型:**执行 **Flip X/Y/Z** 后,模型会得到带负轴的 `modelscale_vec`。q3map2 在烘焙 `misc_model` 时会把它翻转过来,因此摄像机在每个视图中"
        "都显示其外侧。游戏在运行时加载的模型(`misc_model_static`、`misc_model_breakable`)由游戏直接绘制而不翻转——会看到其内侧;摄像机也同样显示,"
        "地图检查会对此发出警告。两个镜像轴等于一次旋转,在任何地方看起来都正确。",
        "**反転したモデル:****Flip X/Y/Z** の後、モデルには負の軸を持つ `modelscale_vec` が付きます。`misc_model` は q3map2 が焼き込むときに裏返すので、"
        "カメラはどのビューでも外側を表示します。ゲームが実行時に読み込むモデル(`misc_model_static`、`misc_model_breakable`)は、ゲームが裏返さずに描くため"
        "内側が見えます。カメラも同じように表示し、マップチェックが警告します。反転した軸が 2 つなら回転と同じで、どこでも正しく見えます。",
        "**Modelos reflejados:** tras **Flip X/Y/Z** un modelo recibe `modelscale_vec` con ejes negativos. q3map2 da la vuelta a un "
        "`misc_model` al integrarlo, así que la cámara muestra su exterior en todas las vistas. Los modelos que el juego carga en tiempo de "
        "ejecución (`misc_model_static`, `misc_model_breakable`) los dibuja el juego sin darles la vuelta: se ve su interior; la cámara los "
        "muestra igual y la comprobación del mapa avisa de ellos. Dos ejes reflejados son un giro y se ven bien en todas partes.",
        "**Zrcadlené modely:** po **Flip X/Y/Z** dostane model `modelscale_vec` se zápornými osami. `misc_model` při zapečení otočí "
        "q3map2, kamera ho proto v každém pohledu ukazuje zvenku. Modely, které hra načítá za běhu (`misc_model_static`, "
        "`misc_model_breakable`), kreslí hra bez otočení - je vidět jejich vnitřek; kamera je ukazuje stejně a kontrola mapy na ně upozorní. "
        "Dvě zrcadlené osy jsou otočení a vypadají všude správně."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def einhaengen(tempo):
    """Glow (mit Bild) und gespiegelte Modelle nach dem Gras-Bild h_r2a_gras (karte_tempo)."""
    b = tempo["bloecke"]
    i = next(n for n, x in enumerate(b) if x[0] == "img" and x[1] == "h_r2a_gras")
    b.insert(i + 1, ("p", _t("glow")))
    b.insert(i + 2, ("img", "h_r3_glow", _t("glow_bild")))
    b.insert(i + 3, ("note", _t("spiegeln")))
