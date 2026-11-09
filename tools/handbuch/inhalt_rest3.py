"""REST3: Hinweise auf der Seite "Die Karte" - deformVertexes autosprite/autosprite2 auf dem Prozessor wie im Spiel, und wie weit
die Bitgleichheit mit dem Spiel auf anderen Grafikkarten zugesichert ist (NVIDIA/AMD: aus dem Bytecode belegt, nicht gemessen).

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_merge2.py.
"""
from inhalt_basis import T

SPRITE = (
    "Surfaces whose shader turns them towards the camera (`deformVertexes autosprite` and `autosprite2`, such as flames, smoke and "
    "light beams) are computed on the processor exactly like the game, together with every other `deformVertexes` line before and "
    "after them - also on brush entities, models and in mirrors, and on curved patches in the detail level the game draws them in. "
    "`autosprite` on a patch stops the game as soon as the patch is drawn with more than 2 x 2 points (\"Autosprite shader ... had odd "
    "vertex count\"); the preview then draws it unturned, and the map check reports it - use `autosprite2` or brush faces.",
    "Flächen, die ihr Shader zur Kamera dreht (`deformVertexes autosprite` und `autosprite2`, etwa Flammen, Rauch und Lichtstrahlen), "
    "rechnet die Vorschau wie das Spiel auf dem Prozessor, zusammen mit allen anderen `deformVertexes`-Zeilen davor und danach - auch "
    "an Brush-Entities, Modellen, im Spiegel und auf gebogenen Patches in der Detailstufe, in der das Spiel sie zeichnet. "
    "`autosprite` auf einem Patch bricht das Spiel ab, sobald der Patch mit mehr als 2 x 2 Punkten gezeichnet wird (\"Autosprite shader "
    "... had odd vertex count\"); die Vorschau zeichnet ihn dann ungedreht, und die Kartenprüfung meldet es - `autosprite2` oder "
    "Pinselflächen nehmen.",
    "着色器使其朝向镜头的表面(`deformVertexes autosprite` 和 `autosprite2`,例如火焰、烟雾和光束)由预览像游戏一样在处理器上计算,"
    "连同其前后所有其他 `deformVertexes` 行——在笔刷实体、模型上、镜子中以及弯曲的面片上(按游戏绘制时的细节级别)也是如此。"
    "面片上的 `autosprite` 一旦以超过 2 x 2 个点绘制就会使游戏中止(\"Autosprite shader ... had odd vertex count\");"
    "预览此时不转动地绘制它,地图检查会报告此问题——请改用 `autosprite2` 或笔刷面。",
    "シェーダーがカメラの方へ向ける面 (`deformVertexes autosprite` と `autosprite2`、たとえば炎、煙、光の筋) は、ゲームと同じく"
    "プロセッサーで計算されます。その前後にある他の `deformVertexes` 行もすべて一緒に計算され、ブラシエンティティ、モデル、鏡の中、"
    "そしてゲームが描く詳細レベルの曲面パッチでも同じです。パッチ上の `autosprite` は、パッチが 2 x 2 点より多く描かれた時点で"
    "ゲームを停止させます (\"Autosprite shader ... had odd vertex count\")。プレビューはその場合回転させずに描き、マップチェックが"
    "報告します。`autosprite2` かブラシ面を使ってください。",
    "Las superficies que su shader gira hacia la cámara (`deformVertexes autosprite` y `autosprite2`, como llamas, humo y rayos de "
    "luz) se calculan en el procesador exactamente como en el juego, junto con todas las demás líneas `deformVertexes` antes y "
    "después de ellas, también en entidades de brush, modelos, espejos y parches curvos en el nivel de detalle con que el juego los "
    "dibuja. `autosprite` en un parche detiene el juego en cuanto el parche se dibuja con más de 2 x 2 puntos (\"Autosprite shader ... "
    "had odd vertex count\"); la vista previa lo dibuja entonces sin girar y la comprobación del mapa lo avisa: use `autosprite2` o "
    "caras de brush.",
    "Plochy, které jejich shader otáčí ke kameře (`deformVertexes autosprite` a `autosprite2`, například plameny, kouř a světelné "
    "paprsky), počítá náhled na procesoru přesně jako hra, spolu se všemi ostatními řádky `deformVertexes` před nimi i za nimi - "
    "také na brush entitách, modelech, v zrcadle a na zakřivených patchích v úrovni detailů, ve které je hra kreslí. `autosprite` na "
    "patchi zastaví hru, jakmile se patch kreslí s více než 2 x 2 body (\"Autosprite shader ... had odd vertex count\"); náhled ho pak "
    "kreslí neotočený a kontrola mapy to hlásí - použijte `autosprite2` nebo plochy brushů.",
)

GRAFIKKARTE = (
    "Where the preview computes exactly like the game - moving vertices (`deformVertexes`), texture movement (`tcMod`, `tcGen vector`) "
    "of the map, the figures and the clouds, and the same in real-time light - the result is bit-identical on any graphics card: it is "
    "measured on the software renderer WARP and on an Intel UHD Graphics; for NVIDIA and AMD it follows from the shader code itself "
    "(every step marked `precise`, no merged multiply-add, only the rounding Direct3D 11 prescribes), not from a measurement. Numbers "
    "smaller than 2^-126 would become 0 on the graphics card - coordinates and waves never get that small. On graphics cards below "
    "feature level 11_0 (vertex shader 4.0) the same is not guaranteed.",
    "Wo die Vorschau genau wie das Spiel rechnet - bewegte Ecken (`deformVertexes`), bewegte Texturen (`tcMod`, `tcGen vector`) der "
    "Karte, der Figuren und der Wolken, ebenso im Echtzeit-Licht -, ist das Ergebnis auf jeder Grafikkarte bitgleich: gemessen auf dem "
    "Software-Renderer WARP und einer Intel UHD Graphics; für NVIDIA und AMD folgt es aus dem Shader-Code selbst (jeder Schritt "
    "`precise`, keine zusammengezogene Multiplikation und Addition, nur die Rundung, die Direct3D 11 vorschreibt), nicht aus einer "
    "Messung. Zahlen unter 2^-126 würden auf der Grafikkarte zu 0 - so klein werden Koordinaten und Wellen nie. Auf Grafikkarten unter "
    "Feature-Level 11_0 (Vertex-Shader 4.0) ist das nicht zugesichert.",
    "凡是预览与游戏完全一致地计算的部分——地图、角色和云的移动顶点(`deformVertexes`)与移动纹理(`tcMod`、`tcGen vector`),"
    "实时光照中也一样——在任何显卡上结果都逐位相同:已在软件渲染器 WARP 和 Intel UHD Graphics 上测量;对于 NVIDIA 和 AMD,"
    "这一点由着色器代码本身得出(每一步都标记为 `precise`,没有合并的乘加,只有 Direct3D 11 规定的舍入),而不是来自测量。"
    "小于 2^-126 的数在显卡上会变成 0——坐标和波形永远不会这么小。在特性级别 11_0 以下(顶点着色器 4.0)的显卡上不作此保证。",
    "プレビューがゲームとまったく同じに計算する部分 (マップ、フィギュア、雲の動く頂点 (`deformVertexes`) と動くテクスチャ "
    "(`tcMod`、`tcGen vector`)、リアルタイムライトでも同じ) は、どのグラフィックカードでもビット単位で同じ結果になります。"
    "ソフトウェアレンダラー WARP と Intel UHD Graphics で測定済みです。NVIDIA と AMD については測定ではなく、シェーダーのコード"
    "そのもの (すべての手順が `precise`、乗算と加算の結合なし、Direct3D 11 が定める丸めのみ) から導かれます。2^-126 より小さい数は"
    "グラフィックカードでは 0 になりますが、座標や波がそこまで小さくなることはありません。フィーチャーレベル 11_0 未満 "
    "(頂点シェーダー 4.0) のグラフィックカードでは保証されません。",
    "Donde la vista previa calcula exactamente como el juego (vértices en movimiento con `deformVertexes`, texturas en movimiento con "
    "`tcMod` y `tcGen vector` del mapa, las figuras y las nubes, y lo mismo en la luz en tiempo real), el resultado es idéntico bit a "
    "bit en cualquier tarjeta gráfica: medido en el renderizador por software WARP y en una Intel UHD Graphics; para NVIDIA y AMD se "
    "deduce del propio código del shader (cada paso marcado `precise`, sin multiplicación y suma fusionadas, solo el redondeo que "
    "prescribe Direct3D 11), no de una medición. Los números menores que 2^-126 se volverían 0 en la tarjeta gráfica; las coordenadas "
    "y las ondas nunca son tan pequeñas. En tarjetas gráficas por debajo del nivel de características 11_0 (vertex shader 4.0) no está "
    "garantizado.",
    "Tam, kde náhled počítá přesně jako hra - pohyblivé vrcholy (`deformVertexes`), pohyblivé textury (`tcMod`, `tcGen vector`) mapy, "
    "postav a mraků, stejně tak ve světle v reálném čase -, je výsledek na každé grafické kartě bitově stejný: změřeno na softwarovém "
    "rendereru WARP a na Intel UHD Graphics; pro NVIDIA a AMD to plyne ze samotného kódu shaderu (každý krok označený `precise`, žádné "
    "sloučené násobení a sčítání, jen zaokrouhlení, které předepisuje Direct3D 11), ne z měření. Čísla menší než 2^-126 by se na "
    "grafické kartě stala nulou - souřadnice a vlny nikdy tak malé nejsou. Na grafických kartách pod úrovní funkcí 11_0 (vertex shader "
    "4.0) to zaručeno není.",
)


def sprite_notiz():
    return ("note", T(*SPRITE[:4]))


def grafikkarte_notiz():
    return ("note", T(*GRAFIKKARTE[:4]))


ES = {SPRITE[0]: SPRITE[4], GRAFIKKARTE[0]: GRAFIKKARTE[4]}
CS = {SPRITE[0]: SPRITE[5], GRAFIKKARTE[0]: GRAFIKKARTE[5]}
