"""Oberflaechentexte von DEF1 (08.11.: deformVertexes-Reste, Wolken, Chamfer).

P45: der Optimierer des Karten-Editors meldet Shader, an denen das Spiel mit ERR_DROP abbricht (TableForFunc, ParseTexMod).
Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("KeOptPrSpielAbbruch", "Shaders that stop the game", "Shader, an denen das Spiel abbricht", "会使游戏中止的着色器", "ゲームを停止させるシェーダー",
     "Shaders que detienen el juego", "Shadery, na kterých se hra zastaví"),
    ("KeOptPrSpielAbbruchE",
     "The game stops the map with an error (ERR_DROP) when it loads one of these shaders or draws a face with it: a wave without a valid wave "
     "function (deformVertexes move, rgbGen/alphaGen wave, tcMod stretch without a function; noise or random in move or in a wave with a "
     "frequency) or more than 4 tcMod in one stage. Give the wave a function (sin, triangle, square, sawtooth, inversesawtooth) or remove lines. deformVertexes autosprite on a patch stops the game as soon as the patch is drawn with more than 2 x 2 points: use autosprite2 or brush faces.",
     "Das Spiel bricht die Karte mit einem Fehler (ERR_DROP) ab, wenn es einen dieser Shader lädt oder eine Fläche damit zeichnet: eine Welle ohne gültige "
     "Wellenform (deformVertexes move, rgbGen/alphaGen wave, tcMod stretch ohne Form; noise oder random bei move oder einer Welle mit "
     "Frequenz) oder mehr als 4 tcMod in einer Stufe. Der Welle eine Form geben (sin, triangle, square, sawtooth, inversesawtooth) oder Zeilen entfernen. deformVertexes autosprite auf einem Patch bricht das Spiel ab, sobald der Patch mit mehr als 2 x 2 Punkten gezeichnet wird: autosprite2 oder Pinselflächen nehmen.",
     "游戏在加载这些着色器之一或绘制使用它的面时, 会以错误 (ERR_DROP) 中止地图: 没有有效波形函数的波 (deformVertexes move、rgbGen/alphaGen wave、"
     "没有函数的 tcMod stretch; move 中或带频率的波中使用 noise 或 random), 或一个阶段中超过 4 个 tcMod。请为波指定函数 (sin、triangle、"
     "square、sawtooth、inversesawtooth) 或删除这些行。面片上的 deformVertexes autosprite 会在面片以超过 2 x 2 个点绘制时中止游戏: 请改用 autosprite2 或笔刷面。",
     "これらのシェーダーのいずれかを読み込むか、それを使う面を描画した時点で、ゲームはエラー (ERR_DROP) でマップを中止します: 有効な波形のない波 "
     "(deformVertexes move、rgbGen/alphaGen wave、波形のない tcMod stretch、move や周波数のある波での noise や random) または 1 つのステージに "
     "5 つ以上の tcMod。波に波形 (sin、triangle、square、sawtooth、inversesawtooth) を指定するか、行を削除してください。パッチ上の deformVertexes autosprite は、パッチが 2 x 2 を超える点で描画された時点でゲームを中止します: autosprite2 かブラシ面を使ってください。",
     "El juego detiene el mapa con un error (ERR_DROP) al cargar uno de estos shaders o al dibujar una cara con él: una onda sin función de onda "
     "válida (deformVertexes move, rgbGen/alphaGen wave, tcMod stretch sin función; noise o random en move o en una onda con frecuencia) "
     "o más de 4 tcMod en una etapa. Dale a la onda una función (sin, triangle, square, sawtooth, inversesawtooth) o quita líneas. deformVertexes autosprite en un parche detiene el juego en cuanto el parche se dibuja con más de 2 x 2 puntos: usa autosprite2 o caras de brush.",
     "Hra zastaví mapu s chybou (ERR_DROP), když načte některý z těchto shaderů nebo s ním vykreslí plochu: vlna bez platné vlnové funkce "
     "(deformVertexes move, rgbGen/alphaGen wave, tcMod stretch bez funkce; noise nebo random u move nebo u vlny s frekvencí) nebo více "
     "než 4 tcMod v jedné fázi. Dejte vlně funkci (sin, triangle, square, sawtooth, inversesawtooth), nebo řádky odstraňte. deformVertexes autosprite na patchi zastaví hru, jakmile se patch vykreslí s více než 2 x 2 body: použijte autosprite2 nebo plochy brushů."),
    ("KeOptD37",
     "%s {shader|shaders} stop the game with ERR_DROP when it loads or draws them (%s)",
     "%s Shader brechen das Spiel mit ERR_DROP ab, wenn es sie lädt oder zeichnet (%s)",
     "%s 个着色器在游戏加载或绘制时会以 ERR_DROP 中止游戏 (%s)",
     "読み込みまたは描画の時点で ERR_DROP でゲームを中止させるシェーダー %s 個 (%s)",
     "%s {shader detiene|shaders detienen} el juego con ERR_DROP al cargarlos o dibujarlos (%s)",
     "%s {shader zastaví|shadery zastaví|shaderů zastaví} hru s ERR_DROP, když je načte nebo vykreslí (%s)"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
