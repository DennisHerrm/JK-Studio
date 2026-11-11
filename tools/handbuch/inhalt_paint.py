"""Modell-Block 1c: die Seite "Model-Tab: Texture Painting" (Malen in 3D und 2D, Werkzeuge, Pinsel, Ebenen und Maske,
Spiegeln und Ausbluten, Umbacken, Fuer das Spiel bauen).

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_uv1b.py.
"""

X = {
    "titel": ("Model tab: texture painting", "Model-Tab: Texturen bemalen", "模型选项卡: 纹理绘制", "モデルタブ: テクスチャペイント", "Pestaña Model: pintar texturas",
              "Karta Model: malování textur"),
    "kurz": (
        "Paint directly on the images behind a model's shaders - on the model in 3D, also while the animation plays, or flat in the image - and write them back "
        "for the game in the size and format of the original.",
        "Male direkt auf die Bilder hinter den Shadern eines Modells - auf dem Modell in 3D, auch während die Animation läuft, oder flach im Bild - und schreibe "
        "sie für das Spiel in Größe und Format des Originals zurück.",
        "直接在模型着色器背后的图像上绘制 - 在 3D 模型上 (动画播放时也可以) 或在平面图像中 - 并以原图的大小和格式写回供游戏使用。",
        "モデルのシェーダーの背後にある画像に直接描きます - 3D のモデル上で (アニメーション再生中も) または平面の画像で - そしてオリジナルのサイズと形式でゲーム用に書き戻します。",
        "Pinta directamente en las imágenes detrás de los shaders de un modelo - sobre el modelo en 3D, también mientras se reproduce la animación, o plano en la "
        "imagen - y escríbelas para el juego con el tamaño y formato del original.",
        "Malujte přímo na obrázky za shadery modelu - na modelu ve 3D, i když běží animace, nebo naplocho v obrázku - a zapište je zpět pro hru ve velikosti a "
        "formátu originálu."),
    "h_wo": ("The Painting area", "Der Bereich Painting", "绘制区域", "ペイント領域", "El área Pintura", "Oblast Malování"),
    "wo": (
        "Choose the area **{{MbBereichPainting}}** at the top of the model tab. The image of each surface is the one the game loads: the skin or the shader of "
        "the surface, otherwise the first map stage of the shader script of the same name. On the left, **{{PtEditorMax}}** (Blender mode: "
        "**{{PtEditorBlender}}**) shows that image flat with the UV triangles; the view on the right shows the model with the whole shader as in the game. As the game loads each image only once, every stage of every surface that uses the image shows what you paint, also under another shader (for example torso and torso_cull). Names that differ only in the extension (hair.tga in the skin, hair in the shader) are one image as well, the file the game loads first. In the view the brush paints only on surfaces whose shader shows the image with their own UVs, not through stages with tcGen or tcMod. A "
        "stroke in the view paints the image of the surface under the brush - across seams and UV islands without a break, because the dab is a sphere on the "
        "model. A stroke in the image paints in 2D. The circle at the mouse shows the radius. Every stroke is one undo step (Ctrl+Z, Ctrl+Y).",
        "Wähle oben im Model-Tab den Bereich **{{MbBereichPainting}}**. Das Bild einer Fläche ist das, welches das Spiel lädt: die Haut oder der Shader der "
        "Fläche, sonst die erste map-Stufe des gleichnamigen Shaderskripts. Links zeigt **{{PtEditorMax}}** (Blender-Modus: **{{PtEditorBlender}}**) dieses "
        "Bild flach mit den UV-Dreiecken, rechts zeigt die Ansicht das Modell mit dem ganzen Shader wie im Spiel. Weil das Spiel jedes Bild nur einmal lädt, zeigt jede Stufe jeder Fläche mit diesem Bild das Gemalte, auch unter einem anderen Shader (etwa torso und torso_cull). Namen, die sich nur in der Endung unterscheiden (hair.tga in der Haut, hair im Shader), sind ebenfalls ein Bild, die Datei, die das Spiel zuerst lädt. In der Ansicht malt der Pinsel nur auf Flächen, deren Shader das Bild mit ihrer eigenen UV zeigt, nicht über Stufen mit tcGen oder tcMod. Ein Strich in der Ansicht bemalt das Bild der "
        "Fläche unter dem Pinsel - über Nähte und UV-Inseln hinweg ohne Bruch, weil der Tupfer eine Kugel auf dem Modell ist. Ein Strich im Bild malt in 2D. "
        "Der Kreis an der Maus zeigt den Radius. Jeder Strich ist ein Rückgängig-Schritt (Strg+Z, Strg+Y).",
        "在模型选项卡顶部选择 **{{MbBereichPainting}}** 区域。每个表面的图像就是游戏加载的那张: 表面的皮肤或着色器, 否则为同名着色器脚本的第一个 map 阶段。左侧 **{{PtEditorMax}}** "
        "(Blender 模式: **{{PtEditorBlender}}**) 以平面方式显示该图像和 UV 三角形; 右侧视图像游戏中一样显示带完整着色器的模型。由于游戏每张图像只加载一次, 使用该图像的每个表面的每个阶段都会显示所绘内容, 即使在另一个着色器下 (例如 torso 和 torso_cull)。只有扩展名不同的名称 (皮肤中的 hair.tga, 着色器中的 hair) 也是同一张图像, 即游戏最先加载的文件。在视图中, 画笔只绘制着色器以其自身 UV 显示该图像的表面, 不经过带 tcGen 或 tcMod 的阶段。在视图中的笔画会绘制画笔下表面的图像 - "
        "跨越接缝和 UV 岛也不会断开, 因为点按是模型上的一个球体。在图像中的笔画则以 2D 绘制。鼠标处的圆圈表示半径。每一笔都是一步撤销 (Ctrl+Z, Ctrl+Y)。",
        "モデルタブ上部で **{{MbBereichPainting}}** 領域を選びます。各サーフェスの画像はゲームが読み込むもの: サーフェスのスキンかシェーダー、なければ同名シェーダースクリプトの最初の "
        "map ステージです。左の **{{PtEditorMax}}** (Blender モード: **{{PtEditorBlender}}**) はその画像を UV 三角形とともに平面で表示し、右のビューはゲームと同じくシェーダー全体で"
        "モデルを表示します。ゲームは各画像を一度だけ読み込むため、その画像を使うすべてのサーフェスのすべてのステージに塗った内容が表示されます (別のシェーダーでも、例: torso と torso_cull)。拡張子だけが違う名前 (スキンの hair.tga とシェーダーの hair) も 1 枚の画像で、ゲームが最初に読み込むファイルです。ビューではブラシは、シェーダーが自身の UV でその画像を表示するサーフェスにだけ塗り、tcGen や tcMod のあるステージ越しには塗りません。ビューでのストロークはブラシの下のサーフェスの画像に塗ります - ダブはモデル上の球なので、シームや UV アイランドをまたいでも途切れません。画像でのストロークは 2D "
        "で塗ります。マウス位置の円が半径です。各ストロークが 1 回の取り消し単位です (Ctrl+Z、Ctrl+Y)。",
        "Elige el área **{{MbBereichPainting}}** arriba en la pestaña Model. La imagen de cada superficie es la que carga el juego: la skin o el shader de la "
        "superficie y, si no, la primera etapa map del script de shader del mismo nombre. A la izquierda **{{PtEditorMax}}** (modo Blender: "
        "**{{PtEditorBlender}}**) muestra esa imagen plana con los triángulos UV; la vista de la derecha muestra el modelo con el shader completo como en el "
        "juego. Como el juego carga cada imagen una sola vez, cada etapa de cada superficie que usa la imagen muestra lo pintado, también bajo otro shader (por ejemplo torso y torso_cull). Los nombres que solo se diferencian en la extensión (hair.tga en la skin, hair en el shader) también son una sola imagen, el archivo que el juego carga primero. En la vista el pincel solo pinta en superficies cuyo shader muestra la imagen con sus propias UV, no a través de etapas con tcGen o tcMod. Un trazo en la vista pinta la imagen de la superficie bajo el pincel, sin cortes sobre costuras e islas UV, porque el toque es una esfera "
        "sobre el modelo. Un trazo en la imagen pinta en 2D. El círculo junto al ratón muestra el radio. Cada trazo es un paso de deshacer (Ctrl+Z, Ctrl+Y).",
        "Nahoře na kartě Model zvolte oblast **{{MbBereichPainting}}**. Obrázek každé plochy je ten, který načítá hra: skin nebo shader plochy, jinak první "
        "fáze map stejnojmenného skriptu shaderu. Vlevo **{{PtEditorMax}}** (režim Blender: **{{PtEditorBlender}}**) ukazuje tento obrázek naplocho s UV "
        "trojúhelníky, vpravo pohled ukazuje model s celým shaderem jako ve hře. Protože hra načítá každý obrázek jen jednou, ukáže namalované každá fáze každé plochy s tímto obrázkem, i pod jiným shaderem (třeba torso a torso_cull). Názvy, které se liší jen příponou (hair.tga ve skinu, hair v shaderu), jsou také jeden obrázek, soubor, který hra načte jako první. V pohledu štětec maluje jen na plochy, jejichž shader ukazuje obrázek s jejich vlastními UV, ne přes fáze s tcGen nebo tcMod. Tah v pohledu maluje obrázek plochy pod štětcem - přes švy a UV ostrovy "
        "bez přerušení, protože dotek je koule na modelu. Tah v obrázku maluje ve 2D. Kruh u myši ukazuje poloměr. Každý tah je jeden krok zpět (Ctrl+Z, "
        "Ctrl+Y)."),
    "bild_malen": ("Painting in the view and in the image, with a second layer", "Malen in der Ansicht und im Bild, mit einer zweiten Ebene", "在视图和图像中绘制, 带第二个图层",
                   "ビューと画像でのペイント (2 枚目のレイヤー付き)", "Pintar en la vista y en la imagen, con una segunda capa", "Malování v pohledu a v obrázku, s druhou vrstvou"),
    "h_werkzeuge": ("Tools and brush", "Werkzeuge und Pinsel", "工具与画笔", "ツールとブラシ", "Herramientas y pincel", "Nástroje a štětec"),
    "werkzeuge": (
        "**{{PtWzPinsel}}**, **{{PtWzWeich}}**, **{{PtWzWischen}}**, **{{PtWzKlonen}}** (Ctrl+click sets the source), **{{PtWzFuellen}}**, **{{PtWzMaske}}** "
        "and **{{PtWzRadierer}}** are in the rollout **{{PtRolloutMax}}** (Blender: **{{PtRolloutBlender}}**), in the menu **{{PtMenue}}**, in the right-click "
        "of the view and in the search. Radius, strength, hardness (falloff) and spacing set the brush; with a pen the pressure can change size and strength. In the view the dabs follow the line you draw on the screen, each one on the surface visible there, like in Substance Painter and Blender - a fast stroke from one hand to the other paints only where the brush passed over the model, nothing over the background. "
        "In Blender mode F and Shift+F drag the radius and the strength, S samples a colour, X swaps the two colours and 1 shows the mask. **{{PtSpiegeln}}** "
        "paints every dab a second time on the mirrored side of the model (axis x for characters). Faces that point away from the hit one by more than 80 "
        "degrees stay untouched unless **{{PtRueckseiten}}** is on.",
        "**{{PtWzPinsel}}**, **{{PtWzWeich}}**, **{{PtWzWischen}}**, **{{PtWzKlonen}}** (Strg+Klick setzt die Quelle), **{{PtWzFuellen}}**, **{{PtWzMaske}}** "
        "und **{{PtWzRadierer}}** stehen im Rollout **{{PtRolloutMax}}** (Blender: **{{PtRolloutBlender}}**), im Menü **{{PtMenue}}**, im Rechtsklick der "
        "Ansicht und in der Suche. Radius, Stärke, Härte (Abfall) und Abstand stellen den Pinsel ein; mit einem Stift ändert der Druck Größe und Stärke. In der Ansicht folgen die Tupfer der Linie, die du auf dem Bildschirm ziehst, jeder auf der dort sichtbaren Fläche, wie in Substance Painter und Blender - ein schneller Strich von einer Hand zur anderen malt nur dort, wo der Pinsel über das Modell ging, nichts über dem Hintergrund. Im "
        "Blender-Modus ziehen F und Umschalt+F Radius und Stärke, S nimmt eine Farbe auf, X tauscht die beiden Farben, 1 zeigt die Maske. **{{PtSpiegeln}}** "
        "malt jeden Tupfer ein zweites Mal auf der gespiegelten Seite des Modells (Achse x bei Figuren). Flächen, die mehr als 80 Grad von der getroffenen weg "
        "zeigen, bleiben unberührt, außer **{{PtRueckseiten}}** ist an.",
        "**{{PtWzPinsel}}**、**{{PtWzWeich}}**、**{{PtWzWischen}}**、**{{PtWzKlonen}}** (Ctrl+单击设置源点)、**{{PtWzFuellen}}**、**{{PtWzMaske}}** 和 **{{PtWzRadierer}}** "
        "位于卷展栏 **{{PtRolloutMax}}** (Blender: **{{PtRolloutBlender}}**)、菜单 **{{PtMenue}}**、视图右键菜单和搜索中。半径、强度、硬度 (衰减) 和间距设置画笔; 使用手写笔时压力可改变大小和强度。在视图中, 点按沿着你在屏幕上画出的线分布, 每一个都落在那里可见的表面上, 与 Substance Painter 和 Blender 相同 - 从一只手快速划到另一只手只会绘制画笔经过模型的地方, 背景上什么也不画。"
        "Blender 模式下 F 和 Shift+F 拖动半径和强度, S 取色, X 交换两种颜色, 1 显示遮罩。**{{PtSpiegeln}}** 会在模型的镜像一侧再画一次每个点按 (角色为 x 轴)。"
        "与命中面朝向相差超过 80 度的面不受影响, 除非打开 **{{PtRueckseiten}}**。",
        "**{{PtWzPinsel}}**、**{{PtWzWeich}}**、**{{PtWzWischen}}**、**{{PtWzKlonen}}** (Ctrl+クリックでソース)、**{{PtWzFuellen}}**、**{{PtWzMaske}}**、**{{PtWzRadierer}}** "
        "はロールアウト **{{PtRolloutMax}}** (Blender: **{{PtRolloutBlender}}**)、メニュー **{{PtMenue}}**、ビューの右クリック、検索にあります。半径、強さ、硬さ (減衰)、間隔でブラシを"
        "設定し、ペンでは筆圧でサイズと強さを変えられます。ビューではダブは画面上に描いた線に沿って並び、それぞれがそこに見えるサーフェスに置かれます (Substance Painter や Blender と同じ) - 片手からもう一方の手への素早いストロークは、ブラシがモデルの上を通った所だけに塗り、背景の上には何も塗りません。Blender モードでは F と Shift+F で半径と強さをドラッグ、S で色を拾い、X で 2 色を入れ替え、1 でマスクを表示します。"
        "**{{PtSpiegeln}}** は各ダブをモデルの鏡像側にもう一度塗ります (キャラクターは x 軸)。当たった面から 80 度以上外を向く面は **{{PtRueckseiten}}** がオンでない限り塗られません。",
        "**{{PtWzPinsel}}**, **{{PtWzWeich}}**, **{{PtWzWischen}}**, **{{PtWzKlonen}}** (Ctrl+clic fija el origen), **{{PtWzFuellen}}**, **{{PtWzMaske}}** y "
        "**{{PtWzRadierer}}** están en el despliegue **{{PtRolloutMax}}** (Blender: **{{PtRolloutBlender}}**), en el menú **{{PtMenue}}**, en el clic derecho "
        "de la vista y en la búsqueda. Radio, fuerza, dureza (caída) y espaciado ajustan el pincel; con un lápiz la presión puede cambiar tamaño y fuerza. En la vista los toques siguen la línea que trazas en la pantalla, cada uno sobre la superficie visible allí, como en Substance Painter y Blender: un trazo rápido de una mano a la otra solo pinta donde el pincel pasó sobre el modelo, nada sobre el fondo. En "
        "modo Blender F y Mayús+F arrastran radio y fuerza, S toma un color, X intercambia los dos colores y 1 muestra la máscara. **{{PtSpiegeln}}** pinta "
        "cada toque una segunda vez en el lado reflejado del modelo (eje x en personajes). Las caras que se apartan más de 80 grados de la tocada quedan "
        "intactas salvo que **{{PtRueckseiten}}** esté activo.",
        "**{{PtWzPinsel}}**, **{{PtWzWeich}}**, **{{PtWzWischen}}**, **{{PtWzKlonen}}** (Ctrl+klik nastaví zdroj), **{{PtWzFuellen}}**, **{{PtWzMaske}}** a "
        "**{{PtWzRadierer}}** jsou v roletě **{{PtRolloutMax}}** (Blender: **{{PtRolloutBlender}}**), v menu **{{PtMenue}}**, v pravém kliknutí pohledu a ve "
        "hledání. Poloměr, síla, tvrdost (útlum) a rozestup nastaví štětec; s perem může přítlak měnit velikost a sílu. V pohledu doteky sledují čáru, kterou táhnete na obrazovce, každý na ploše, která je tam vidět, jako v Substance Painter a Blenderu - rychlý tah z jedné ruky do druhé maluje jen tam, kde štětec přešel přes model, nic přes pozadí. V režimu Blender F a Shift+F táhnou "
        "poloměr a sílu, S nabere barvu, X prohodí obě barvy a 1 zobrazí masku. **{{PtSpiegeln}}** namaluje každý dotek podruhé na zrcadlové straně modelu "
        "(osa x u postav). Plochy odvrácené od zasažené o víc než 80 stupňů zůstanou nedotčené, pokud není zapnuto **{{PtRueckseiten}}**."),
    "h_ebenen": ("Layers, mask and seams", "Ebenen, Maske und Nähte", "图层、遮罩与接缝", "レイヤー、マスク、シーム", "Capas, máscara y costuras", "Vrstvy, maska a švy"),
    "ebenen": (
        "The original image of the game is the bottom layer and stays untouched as long as you paint on a layer above it (**{{PtEbeneNeu}}**). Each layer has "
        "an opacity, a blend mode (normal, multiply, add, screen, overlay) and can be hidden; **{{PtEbeneVereinen}}** merges it into the one below. The mask "
        "protects texels from every tool; **{{PtMaskeAn}}** shows it in red in the image. After each dab the texels just outside the UV islands take the colour "
        "of the nearest painted texel (**{{PtAusbluten}}**, 4 texels by default), so that no dark seams appear in the distance. When the UVs were changed in "
        "the UV area, **{{PtUmbacken}}** moves every layer and the mask from the old UVs onto the new ones.",
        "Das Originalbild des Spiels ist die unterste Ebene und bleibt unberührt, solange du auf einer Ebene darüber malst (**{{PtEbeneNeu}}**). Jede Ebene hat "
        "eine Deckkraft, eine Mischart (normal, multiplizieren, addieren, negativ multiplizieren, ineinanderkopieren) und lässt sich ausblenden; "
        "**{{PtEbeneVereinen}}** vereint sie mit der darunter. Die Maske schützt Texel vor jedem Werkzeug; **{{PtMaskeAn}}** zeigt sie rot im Bild. Nach jedem "
        "Tupfer bekommen die Texel knapp außerhalb der UV-Inseln die Farbe des nächsten bemalten Texels (**{{PtAusbluten}}**, Vorgabe 4 Texel), damit in der "
        "Ferne keine dunklen Nähte entstehen. Wurden die UV im Bereich UV geändert, bringt **{{PtUmbacken}}** jede Ebene und die Maske von der alten UV auf "
        "die neue.",
        "游戏原图是最底层, 只要在其上方的图层 (**{{PtEbeneNeu}}**) 上绘制, 原图就保持不变。每个图层有不透明度和混合模式 (正常、正片叠底、相加、滤色、叠加), 可隐藏; "
        "**{{PtEbeneVereinen}}** 将其合并到下方图层。遮罩保护纹素不受任何工具影响; **{{PtMaskeAn}}** 在图像中以红色显示。每次点按后, UV 岛外侧的纹素会获得最近绘制纹素的颜色 "
        "(**{{PtAusbluten}}**, 默认 4 个纹素), 这样远处不会出现深色接缝。在 UV 区域更改 UV 后, **{{PtUmbacken}}** 会把每个图层和遮罩从旧 UV 转到新 UV。",
        "ゲームのオリジナル画像は一番下のレイヤーで、その上のレイヤー (**{{PtEbeneNeu}}**) に塗る限り変わりません。各レイヤーには不透明度と描画モード (通常、乗算、加算、スクリーン、"
        "オーバーレイ) があり、非表示にもできます。**{{PtEbeneVereinen}}** で下のレイヤーに結合します。マスクはテクセルをすべてのツールから保護し、**{{PtMaskeAn}}** で画像に赤く"
        "表示します。各ダブの後、UV アイランドのすぐ外側のテクセルは最寄りの塗られたテクセルの色を受けます (**{{PtAusbluten}}**、既定 4 テクセル)。こうして遠くで暗いシームが出ません。"
        "UV 領域で UV を変えた場合は、**{{PtUmbacken}}** で各レイヤーとマスクを古い UV から新しい UV へ移します。",
        "La imagen original del juego es la capa inferior y queda intacta mientras pintes en una capa por encima (**{{PtEbeneNeu}}**). Cada capa tiene "
        "opacidad, un modo de fusión (normal, multiplicar, sumar, trama, superponer) y se puede ocultar; **{{PtEbeneVereinen}}** la combina con la de abajo. "
        "La máscara protege téxeles de todas las herramientas; **{{PtMaskeAn}}** la muestra en rojo en la imagen. Tras cada toque, los téxeles justo fuera de "
        "las islas UV toman el color del téxel pintado más cercano (**{{PtAusbluten}}**, 4 téxeles por defecto), para que no aparezcan costuras oscuras a lo "
        "lejos. Si cambiaste las UV en el área UV, **{{PtUmbacken}}** lleva cada capa y la máscara de las UV viejas a las nuevas.",
        "Původní obrázek hry je spodní vrstva a zůstává nedotčen, dokud malujete na vrstvu nad ním (**{{PtEbeneNeu}}**). Každá vrstva má krytí, režim "
        "prolnutí (normální, násobit, přičíst, závoj, překrýt) a lze ji skrýt; **{{PtEbeneVereinen}}** ji sloučí se spodní. Maska chrání texely před všemi "
        "nástroji; **{{PtMaskeAn}}** ji ukáže v obrázku červeně. Po každém doteku dostanou texely těsně za UV ostrovy barvu nejbližšího namalovaného texelu "
        "(**{{PtAusbluten}}**, výchozí 4 texely), aby v dálce nevznikly tmavé švy. Pokud jste změnili UV v oblasti UV, **{{PtUmbacken}}** přenese každou "
        "vrstvu a masku ze starých UV na nové."),
    "h_spiel": ("For the game", "Für das Spiel", "用于游戏", "ゲーム用", "Para el juego", "Pro hru"),
    "spiel": (
        "**{{PtBauen}}** writes every painted image under its game path (for example models/players/stormtrooper/...) into the output folder - a mod folder "
        "or fs_homepath, never the game folder. Size and format stay as in the original: a .tga keeps its depth and compression, a .jpg is written with the "
        "quantisation tables of the original, a .png stays .png. Images you did not paint are not written, so the game keeps loading the original. Movie "
        "Duels looks for .jpg before .tga, which is why the file keeps the name and the ending of the original. The layers stay in JK Studio.",
        "**{{PtBauen}}** schreibt jedes bemalte Bild unter seinem Spielpfad (etwa models/players/stormtrooper/...) in den Ausgabeordner - einen Mod-Ordner oder "
        "fs_homepath, nie den Spielordner. Größe und Format bleiben wie im Original: eine .tga behält Tiefe und Kompression, eine .jpg wird mit den "
        "Quantisierungstabellen des Originals geschrieben, eine .png bleibt .png. Nicht bemalte Bilder werden nicht geschrieben, das Spiel lädt weiter das "
        "Original. Movie Duels sucht .jpg vor .tga, darum behält die Datei Namen und Endung des Originals. Die Ebenen bleiben in JK Studio.",
        "**{{PtBauen}}** 会把每张已绘制的图像按其游戏路径 (例如 models/players/stormtrooper/...) 写入输出文件夹 - 模组文件夹或 fs_homepath, 绝不是游戏文件夹。大小和格式与原图相同: "
        ".tga 保持位深和压缩, .jpg 使用原图的量化表写入, .png 仍为 .png。未绘制的图像不会写入, 游戏继续加载原图。Movie Duels 先找 .jpg 再找 .tga, 因此文件保留原图的名称和扩展名。图层保留在 JK Studio 中。",
        "**{{PtBauen}}** は塗った各画像をゲーム内パス (例: models/players/stormtrooper/...) で出力フォルダーに書き出します - MOD フォルダーか fs_homepath で、ゲームフォルダーには"
        "書きません。サイズと形式はオリジナルのまま: .tga は深度と圧縮を保ち、.jpg はオリジナルの量子化テーブルで書かれ、.png は .png のままです。塗っていない画像は書き出されず、ゲームは"
        "オリジナルを読み続けます。Movie Duels は .tga より先に .jpg を探すため、ファイルはオリジナルの名前と拡張子を保ちます。レイヤーは JK Studio に残ります。",
        "**{{PtBauen}}** escribe cada imagen pintada con su ruta del juego (por ejemplo models/players/stormtrooper/...) en la carpeta de salida: una carpeta de "
        "mod o fs_homepath, nunca la carpeta del juego. Tamaño y formato quedan como en el original: un .tga conserva profundidad y compresión, un .jpg se "
        "escribe con las tablas de cuantización del original y un .png sigue siendo .png. Las imágenes sin pintar no se escriben y el juego sigue cargando el "
        "original. Movie Duels busca .jpg antes que .tga, por eso el archivo conserva el nombre y la extensión del original. Las capas se quedan en JK Studio.",
        "**{{PtBauen}}** zapíše každý namalovaný obrázek pod jeho herní cestou (třeba models/players/stormtrooper/...) do výstupní složky - složky modu nebo "
        "fs_homepath, nikdy do složky hry. Velikost a formát zůstávají jako v originálu: .tga si ponechá hloubku a kompresi, .jpg se zapíše s kvantizačními "
        "tabulkami originálu, .png zůstane .png. Nenamalované obrázky se nezapisují a hra dál načítá originál. Movie Duels hledá .jpg před .tga, proto si "
        "soubor ponechá název a příponu originálu. Vrstvy zůstávají v JK Studiu."),
}


def _t(schluessel):
    from inhalt_basis import T
    z = X[schluessel]
    return T(z[0], z[1], z[2], z[3])


def seite():
    """Die Seite modell-painting (Gruppe Missionen, nach modell-modellieren)."""
    return {
        "id": "modell-painting",
        "gruppe": "mission",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("h2", "wo", _t("h_wo")),
            ("p", _t("wo")),
            ("img", "h_pt_malen", _t("bild_malen"), "breit"),
            ("h2", "werkzeuge", _t("h_werkzeuge")),
            ("p", _t("werkzeuge")),
            ("h2", "ebenen", _t("h_ebenen")),
            ("p", _t("ebenen")),
            ("h2", "spiel", _t("h_spiel")),
            ("p", _t("spiel")),
        ],
    }


ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}
