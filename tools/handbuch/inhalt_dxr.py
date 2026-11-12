"""DXR (rc612): Abschnitt "Raytracing-Schatten (DXR)" auf der Seite "Karten-Editor: Tempo und Echtzeit-Licht" -
Build > Ray-traced shadows (DXR) als zweite Schattenart des Echtzeit-Lichts neben den Schattenkarten: was sich aendert,
was gleich bleibt, wann es geht (Direct3D 12 mit DXR 1.1) und was der Menuepunkt sonst sagt.

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_vis2c.py.
"""
from inhalt_basis import T

TITEL = ("Ray-traced shadows (DXR)", "Raytracing-Schatten (DXR)", "光线追踪阴影(DXR)", "レイトレーシングの影(DXR)",
         "Sombras con trazado de rayos (DXR)", "Stíny sledováním paprsků (DXR)")

EINLEITUNG = (
    "**{{KeLBuild}} > {{KeStrahlenMenue}}** chooses the shadow type of the real-time light. Off - the default - the shadows "
    "come from shadow maps as before: a shadow cube for every light and shadow maps for the sun. On, the graphics card sends "
    "one shadow ray for every light and every pixel (DirectX Raytracing). The lights, their colours and their brightness "
    "stay exactly the same; only whether a spot is in shadow is decided differently.",
    "**{{KeLBuild}} > {{KeStrahlenMenue}}** wählt die Schattenart des Echtzeit-Lichts. Aus - das ist die Vorgabe - kommen "
    "die Schatten wie bisher aus Schattenkarten: ein Schattenwürfel je Licht und Schattenkarten für die Sonne. An schickt "
    "die Grafikkarte für jedes Licht und jeden Bildpunkt einen Schattenstrahl (DirectX Raytracing). Die Lichter, ihre "
    "Farben und ihre Helligkeit bleiben genau gleich; nur ob eine Stelle im Schatten liegt, wird anders entschieden.",
    "**{{KeLBuild}} > {{KeStrahlenMenue}}** 用来选择实时光照的阴影类型。关闭(默认)时,阴影和以前一样来自阴影贴图:每个光源一个阴影立方体,"
    "太阳使用阴影贴图。开启时,显卡为每个光源和每个像素发出一条阴影光线(DirectX Raytracing)。光源、颜色和亮度完全不变;"
    "只是判断某处是否在阴影中的方式不同。",
    "**{{KeLBuild}} > {{KeStrahlenMenue}}** でリアルタイムライトの影の種類を選びます。オフ(既定)では、影はこれまでどおり"
    "シャドウマップから作られます。ライトごとにシャドウキューブ、太陽にはシャドウマップです。オンにすると、グラフィックカードが"
    "ライトとピクセルごとにシャドウレイを 1 本飛ばします(DirectX Raytracing)。ライト、色、明るさはまったく同じで、"
    "ある場所が影に入るかどうかの判定だけが変わります。",
    "**{{KeLBuild}} > {{KeStrahlenMenue}}** elige el tipo de sombra de la luz en tiempo real. Desactivado - el valor "
    "predeterminado - las sombras salen como hasta ahora de mapas de sombras: un cubo de sombras por luz y mapas de sombras "
    "para el sol. Activado, la tarjeta gráfica lanza un rayo de sombra por cada luz y cada píxel (DirectX Raytracing). Las "
    "luces, sus colores y su brillo siguen exactamente igual; solo cambia cómo se decide si un punto está en sombra.",
    "**{{KeLBuild}} > {{KeStrahlenMenue}}** volí typ stínů světla v reálném čase. Vypnuto - to je výchozí stav - vznikají "
    "stíny jako dosud ze stínových map: stínová krychle pro každé světlo a stínové mapy pro slunce. Zapnuto vyšle grafická "
    "karta pro každé světlo a každý pixel jeden stínový paprsek (DirectX Raytracing). Světla, jejich barvy a jas zůstávají "
    "přesně stejné; jinak se jen rozhoduje, zda místo leží ve stínu.",
)

BILD_KARTEN = (
    "Shadow maps (the default): the edge of the shadow is blurred over a few texels of the map and shows its steps.",
    "Schattenkarten (die Vorgabe): Die Kante des Schattens ist über einige Texel der Karte verwischt und zeigt deren Stufen.",
    "阴影贴图(默认):阴影边缘在贴图的几个纹素范围内模糊,并显出锯齿。",
    "シャドウマップ(既定):影の輪郭はマップの数テクセル分ぼやけ、段差が見えます。",
    "Mapas de sombras (el valor predeterminado): el borde de la sombra se difumina a lo largo de varios téxeles del mapa y "
    "muestra sus escalones.",
    "Stínové mapy (výchozí): hrana stínu je rozmazaná přes několik texelů mapy a jsou na ní vidět schody.",
)

BILD_STRAHLEN = (
    "The same view with ray-traced shadows: the shadow lies at the same place, its edge is sharp. The corner of the camera "
    "names the shadow type.",
    "Derselbe Blick mit Raytracing-Schatten: Der Schatten liegt an derselben Stelle, seine Kante ist scharf. Die Ecke der "
    "Kamera nennt die Schattenart.",
    "同一视角下的光线追踪阴影:阴影位置相同,边缘锐利。摄像机视图的角落会显示阴影类型。",
    "同じ視点のレイトレーシングの影:影の位置は同じで、輪郭はくっきりしています。カメラの隅に影の種類が表示されます。",
    "La misma vista con sombras con trazado de rayos: la sombra está en el mismo sitio y su borde es nítido. La esquina de la "
    "cámara indica el tipo de sombra.",
    "Stejný pohled se stíny sledováním paprsků: stín leží na stejném místě a jeho hrana je ostrá. Roh kamery uvádí typ stínů.",
)

GLEICH = (
    "**Same place, sharper:** the rays follow the same rules of q3map2 -light as the shadow maps (what casts shadows, the ray "
    "to the sun ends at the sky). They start 1 unit above the surface, where q3map2 puts its lightmap sample, and ignore what "
    "is closer than 1.5 units - as q3map2 does. The shadow lies where the built lightmap has it, without the steps and the "
    "blur of the shadow maps.",
    "**Gleiche Stelle, schärfer:** Die Strahlen folgen denselben Regeln von q3map2 -light wie die Schattenkarten (was "
    "Schatten wirft, der Strahl zur Sonne endet am Himmel). Sie beginnen 1 Einheit über der Fläche, dort, wo q3map2 seinen "
    "Lightmap-Punkt setzt, und übergehen, was näher als 1,5 Einheiten liegt - wie q3map2. Der Schatten liegt dort, wo ihn die "
    "gebaute Lightmap hat, ohne die Stufen und die Unschärfe der Schattenkarten.",
    "**位置相同,更锐利:**光线遵循与阴影贴图相同的 q3map2 -light 规则(哪些物体投射阴影、射向太阳的光线在天空处结束)。"
    "光线从表面上方 1 个单位处出发,也就是 q3map2 放置光照贴图采样点的位置,并忽略距离小于 1.5 个单位的物体,与 q3map2 相同。"
    "阴影位于烘焙出的光照贴图中的位置,但没有阴影贴图的锯齿和模糊。",
    "**同じ位置で、よりくっきり:**レイはシャドウマップと同じ q3map2 -light の規則に従います(何が影を落とすか、太陽へのレイは空で"
    "終わること)。q3map2 がライトマップのサンプルを置く、面から 1 ユニット上の位置から出発し、1.5 ユニットより近いものは "
    "q3map2 と同じく無視します。影は焼き込んだライトマップと同じ位置にでき、シャドウマップの段差やぼけはありません。",
    "**Mismo sitio, más nítido:** los rayos siguen las mismas reglas de q3map2 -light que los mapas de sombras (qué proyecta "
    "sombra, el rayo hacia el sol termina en el cielo). Parten 1 unidad por encima de la superficie, donde q3map2 coloca su "
    "muestra del lightmap, e ignoran lo que está a menos de 1,5 unidades, como q3map2. La sombra queda donde la tiene el "
    "lightmap compilado, sin los escalones ni el desenfoque de los mapas de sombras.",
    "**Stejné místo, ostřejší:** paprsky se řídí stejnými pravidly q3map2 -light jako stínové mapy (co vrhá stín, paprsek ke "
    "slunci končí na obloze). Začínají 1 jednotku nad povrchem, kam q3map2 klade svůj vzorek lightmapy, a přehlížejí vše, co "
    "je blíž než 1,5 jednotky - jako q3map2. Stín leží tam, kde ho má sestavená lightmapa, bez schodů a rozmazání stínových "
    "map.",
)

FILTER = (
    "**alphashadow and lightfilter as in q3map2:** a triangle of such a shader lets through (255 - alpha) / 255 of the light, "
    "a `lightfilter` shader tints the light with the colour of its texel, and several layers multiply. The shadow maps only "
    "know all or nothing at half alpha. As in q3map2, `q3map_alphashadow` counts like `surfaceparm alphashadow` (and "
    "any other `q3map_<surfaceparm>` like that surfaceparm) - many grates of Jedi Academy are written that way.",
    "**alphashadow und lightfilter wie in q3map2:** Ein Dreieck eines solchen Shaders lässt (255 - Alpha) / 255 des Lichts "
    "durch, ein Shader mit `lightfilter` färbt das Licht mit der Farbe seines Texels, mehrere Schichten multiplizieren sich. "
    "Die Schattenkarten kennen hier nur alles oder nichts bei halbem Alpha. Wie in q3map2 zählt `q3map_alphashadow` wie "
    "`surfaceparm alphashadow` (und jedes andere `q3map_<surfaceparm>` wie dieser surfaceparm) - so sind viele Gitter von "
    "Jedi Academy geschrieben.",
    "**alphashadow 和 lightfilter 与 q3map2 相同:**这类着色器的三角形让 (255 - alpha) / 255 的光通过,带 `lightfilter` 的着色器"
    "用其纹素的颜色给光着色,多层相乘。阴影贴图在这里只有在一半 alpha 处的“全有或全无”。与 q3map2 一样,`q3map_alphashadow` "
    "等同于 `surfaceparm alphashadow`(其他 `q3map_<surfaceparm>` 也等同于对应的 surfaceparm)——Jedi Academy 的许多格栅就是这样写的。",
    "**alphashadow と lightfilter は q3map2 と同じ:**このようなシェーダーの三角形は光の (255 - アルファ) / 255 を通し、"
    "`lightfilter` のシェーダーはテクセルの色で光に色を付け、複数の層は掛け合わされます。シャドウマップはアルファ半分を境にした"
    "「全部か無か」しか扱えません。q3map2 と同じく、`q3map_alphashadow` は "
    "`surfaceparm alphashadow` と同じに扱われます(ほかの `q3map_<surfaceparm>` も対応する surfaceparm と同じです)。Jedi Academy の"
    "多くの格子はこの書き方です。",
    "**alphashadow y lightfilter como en q3map2:** un triángulo de un shader así deja pasar (255 - alfa) / 255 de la luz, un "
    "shader con `lightfilter` tiñe la luz con el color de su téxel y varias capas se multiplican. Los mapas de sombras solo "
    "conocen aquí todo o nada a media alfa. Como en q3map2, `q3map_alphashadow` cuenta como `surfaceparm alphashadow` "
    "(y cualquier otro `q3map_<surfaceparm>` como ese surfaceparm): así están escritas muchas rejillas de Jedi Academy.",
    "**alphashadow a lightfilter jako v q3map2:** trojúhelník takového shaderu propustí (255 - alfa) / 255 světla, shader "
    "s `lightfilter` obarví světlo barvou svého texelu a více vrstev se násobí. Stínové mapy tu znají jen vše, nebo nic při "
    "poloviční alfě. Stejně jako v q3map2 platí `q3map_alphashadow` jako `surfaceparm alphashadow` (a každé "
    "jiné `q3map_<surfaceparm>` jako daný surfaceparm) - tak je zapsáno mnoho mřížek Jedi Academy.",
)

WARTEN = (
    "**No waiting, no limit:** after switching on and after an edit no light waits for its shadow cube, and no light stays "
    "without a shadow because the graphics memory for shadow cubes is used up. While ray tracing is on, the shadow cubes are "
    "released.",
    "**Kein Warten, keine Grenze:** Nach dem Einschalten und nach einer Änderung wartet kein Licht auf seinen Schattenwürfel, "
    "und kein Licht bleibt ohne Schatten, weil der Grafikspeicher für Schattenwürfel aufgebraucht ist. Solange Raytracing an "
    "ist, sind die Schattenwürfel freigegeben.",
    "**无需等待,没有上限:**开启后以及编辑后,没有光源需要等待自己的阴影立方体,也不会因为阴影立方体的显存用尽而有光源没有阴影。"
    "开启光线追踪期间,阴影立方体占用的显存会被释放。",
    "**待ち時間なし、上限なし:**オンにした直後や編集のあとにシャドウキューブを待つライトはなく、シャドウキューブ用の"
    "グラフィックメモリが尽きて影のないライトが残ることもありません。レイトレーシングがオンの間、シャドウキューブは解放されます。",
    "**Sin esperas, sin límite:** tras activarlo y tras una edición ninguna luz espera a su cubo de sombras, y ninguna luz se "
    "queda sin sombra porque se agotó la memoria gráfica para cubos de sombras. Mientras el trazado de rayos está activado, "
    "los cubos de sombras se liberan.",
    "**Bez čekání, bez limitu:** po zapnutí ani po úpravě žádné světlo nečeká na svou stínovou krychli a žádné světlo "
    "nezůstane bez stínu proto, že došla grafická paměť pro stínové krychle. Dokud je sledování paprsků zapnuté, jsou stínové "
    "krychle uvolněné.",
)

BEARBEITEN = (
    "**While you edit:** the shadow geometry is kept in cells of 512 units, and an edit rebuilds only the cells it touches. "
    "A cell that changes again within half a second - you are dragging something - is built for speed and rebuilt once for "
    "quality as soon as it rests.",
    "**Beim Bearbeiten:** Die Schattengeometrie liegt in Zellen zu 512 Einheiten, und eine Änderung baut nur die Zellen neu, "
    "die sie berührt. Eine Zelle, die sich binnen einer halben Sekunde wieder ändert - du ziehst gerade etwas -, wird auf "
    "Tempo gebaut und einmal auf Qualität nachgezogen, sobald sie ruht.",
    "**编辑时:**阴影几何体按 512 单位的单元保存,一次编辑只重建它触及的单元。在半秒内再次变化的单元(你正在拖动某物)"
    "会以速度优先的方式构建,一旦静止就按质量优先重建一次。",
    "**編集中:**影のジオメトリは 512 ユニットのセルごとに保持され、編集で作り直されるのは触れたセルだけです。半秒以内にまた"
    "変化したセル(何かをドラッグしているとき)は速度優先で構築され、動きが止まると品質優先で一度だけ作り直されます。",
    "**Mientras editas:** la geometría de sombras se guarda en celdas de 512 unidades, y una edición solo reconstruye las "
    "celdas que toca. Una celda que vuelve a cambiar en medio segundo - estás arrastrando algo - se construye primando la "
    "velocidad y se reconstruye una vez primando la calidad en cuanto queda en reposo.",
    "**Při úpravách:** stínová geometrie je uložena v buňkách po 512 jednotkách a úprava znovu sestaví jen buňky, kterých se "
    "dotkne. Buňka, která se do půl sekundy změní znovu - právě něco táhnete -, se sestaví na rychlost a jakmile se zastaví, "
    "jednou se přestaví na kvalitu.",
)

SONNE = (
    "**Sun samples:** every sample of `q3map_sunExt` and of `_samples` is traced in its own direction, as q3map2 does (the "
    "shadow maps use the direction of the main sun for all of them).",
    "**Sonnenproben:** Jede Probe von `q3map_sunExt` und von `_samples` wird in ihrer eigenen Richtung verfolgt, wie in "
    "q3map2 (die Schattenkarten nehmen für alle die Richtung der Hauptsonne).",
    "**太阳采样:**`q3map_sunExt` 和 `_samples` 的每个采样都沿自己的方向追踪,与 q3map2 相同(阴影贴图对所有采样都使用主太阳的方向)。",
    "**太陽のサンプル:**`q3map_sunExt` と `_samples` の各サンプルは、q3map2 と同じくそれぞれの方向でたどります"
    "(シャドウマップはすべてにメインの太陽の方向を使います)。",
    "**Muestras del sol:** cada muestra de `q3map_sunExt` y de `_samples` se traza en su propia dirección, como en q3map2 "
    "(los mapas de sombras usan para todas la dirección del sol principal).",
    "**Vzorky slunce:** každý vzorek `q3map_sunExt` a `_samples` se sleduje ve vlastním směru jako v q3map2 (stínové mapy "
    "berou pro všechny směr hlavního slunce).",
)

BILD_MENUE = (
    "{{KeLBuild}} > {{KeStrahlenMenue}}, next to the other shadow settings of the real-time light. If it is not available, "
    "the entry is greyed out and the tooltip says why.",
    "{{KeLBuild}} > {{KeStrahlenMenue}}, bei den anderen Schatten-Einstellungen des Echtzeit-Lichts. Ist es nicht verfügbar, "
    "ist der Eintrag ausgegraut, und der Tooltip nennt den Grund.",
    "{{KeLBuild}} > {{KeStrahlenMenue}},位于实时光照的其他阴影设置旁边。不可用时,该菜单项变灰,工具提示会说明原因。",
    "{{KeLBuild}} > {{KeStrahlenMenue}}。リアルタイムライトのほかの影の設定の隣にあります。使えないときは項目がグレー表示になり、"
    "ツールチップに理由が表示されます。",
    "{{KeLBuild}} > {{KeStrahlenMenue}}, junto a los demás ajustes de sombras de la luz en tiempo real. Si no está "
    "disponible, la entrada aparece atenuada y la descripción emergente indica el motivo.",
    "{{KeLBuild}} > {{KeStrahlenMenue}}, u ostatních nastavení stínů světla v reálném čase. Není-li k dispozici, je položka "
    "zašedlá a popisek uvede důvod.",
)

WANN = (
    "**When it is available:** ray-traced shadows need Direct3D 12 as the graphics API ([[einstellungen#grafik]], "
    "**{{MenuView}} > {{GrafikMenu}}**) and a graphics card whose driver supports DirectX Raytracing 1.1. JK Studio checks "
    "this when the map editor first needs it. With Direct3D 11, with a graphics card without DXR 1.1 or if the graphics card "
    "refuses the data, the entry is greyed out, its tooltip names the reason, and the real-time light uses shadow maps - "
    "nothing changes there. The choice is remembered and applies again as soon as it is possible.",
    "**Wann es verfügbar ist:** Raytracing-Schatten brauchen Direct3D 12 als Grafikschnittstelle ([[einstellungen#grafik]], "
    "**{{MenuView}} > {{GrafikMenu}}**) und eine Grafikkarte, deren Treiber DirectX Raytracing 1.1 kann. JK Studio prüft das, "
    "wenn der Karten-Editor es zum ersten Mal braucht. Mit Direct3D 11, mit einer Grafikkarte ohne DXR 1.1 oder wenn die "
    "Grafikkarte die Daten ablehnt, ist der Eintrag ausgegraut, sein Tooltip nennt den Grund, und das Echtzeit-Licht nimmt "
    "Schattenkarten - dort ändert sich nichts. Die Wahl bleibt gemerkt und gilt wieder, sobald es geht.",
    "**何时可用:**光线追踪阴影需要以 Direct3D 12 作为图形接口([[einstellungen#grafik]],**{{MenuView}} > {{GrafikMenu}}**),"
    "并且显卡驱动支持 DirectX Raytracing 1.1。JK Studio 在地图编辑器第一次需要时进行检查。使用 Direct3D 11、显卡不支持 DXR 1.1 "
    "或显卡拒绝数据时,该菜单项变灰,工具提示说明原因,实时光照使用阴影贴图——那里没有任何变化。所选项会被记住,一旦可用就再次生效。",
    "**使える条件:**レイトレーシングの影には、グラフィックス API として Direct3D 12([[einstellungen#grafik]]、"
    "**{{MenuView}} > {{GrafikMenu}}**)と、ドライバーが DirectX Raytracing 1.1 に対応したグラフィックカードが必要です。"
    "JK Studio はマップエディターが最初に必要としたときに確認します。Direct3D 11 の場合、DXR 1.1 非対応のグラフィックカードの場合、"
    "グラフィックカードがデータを受け付けなかった場合は、項目がグレー表示になってツールチップに理由が表示され、リアルタイムライトは"
    "シャドウマップを使います(そこは何も変わりません)。選択は記憶され、使えるようになればまた有効になります。",
    "**Cuándo está disponible:** las sombras con trazado de rayos necesitan Direct3D 12 como interfaz gráfica "
    "([[einstellungen#grafik]], **{{MenuView}} > {{GrafikMenu}}**) y una tarjeta gráfica cuyo controlador admita DirectX Raytracing "
    "1.1. JK Studio lo comprueba cuando el editor de mapas lo necesita por primera vez. Con Direct3D 11, con una tarjeta "
    "gráfica sin DXR 1.1 o si la tarjeta gráfica rechaza los datos, la entrada aparece atenuada, su descripción emergente "
    "indica el motivo y la luz en tiempo real usa mapas de sombras; ahí no cambia nada. La elección se recuerda y vuelve a "
    "aplicarse en cuanto sea posible.",
    "**Kdy je k dispozici:** stíny sledováním paprsků potřebují Direct3D 12 jako grafické rozhraní ([[einstellungen#grafik]], "
    "**{{MenuView}} > {{GrafikMenu}}**) a grafickou kartu, jejíž ovladač umí DirectX Raytracing 1.1. JK Studio to ověří, když "
    "to editor map poprvé potřebuje. S Direct3D 11, s grafickou kartou bez DXR 1.1 nebo když grafická karta data odmítne, je "
    "položka zašedlá, její popisek uvede důvod a světlo v reálném čase používá stínové mapy - tam se nic nemění. Volba "
    "zůstává zapamatovaná a platí znovu, jakmile to jde.",
)

ALLE = [TITEL, EINLEITUNG, BILD_KARTEN, BILD_STRAHLEN, GLEICH, FILTER, WARTEN, BEARBEITEN, SONNE, BILD_MENUE, WANN]


def _t(x):
    return T(*x[:4])


def bloecke():
    """die Bloecke fuer die Seite karte_tempo (inhalt_karte7.py), nach dem Absatz "Licht durch Patches" - als Funktion:
    T() schlaegt die weiteren Sprachen nach, und inhalt_es.py / inhalt_cs.py laden dieses Modul fuer ES / CS"""
    return [
        ("h2", "dxr", _t(TITEL)),
        ("p", _t(EINLEITUNG)),
        ("img", "h_dxr_karten", _t(BILD_KARTEN)),
        ("img", "h_dxr_strahlen", _t(BILD_STRAHLEN)),
        ("list", [_t(GLEICH), _t(FILTER), _t(WARTEN), _t(BEARBEITEN), _t(SONNE)]),
        ("img", "h_dxr_menue", _t(BILD_MENUE)),
        ("p", _t(WANN)),
    ]


ES = {x[0]: x[4] for x in ALLE}
CS = {x[0]: x[5] for x in ALLE}
