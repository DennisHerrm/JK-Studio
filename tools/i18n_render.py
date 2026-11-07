# -*- coding: utf-8 -*-
"""Oberflaechentexte von p_render (bewegte Stufen und Gluehen im Echtzeit-Licht) fuer tools/gen_i18n.py und die weiteren
Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("KeLichtGrenzenTip4",
     "The real-time light is a preview without a build: no bounce, no filter, no supersampling, shadows from shadow maps instead of "
     "traced rays. The first stage of a shader (or $lightmap with its texture) gets the light; the further stages follow as in the game, "
     "also with tcMod, tcGen environment/vector, rgbGen/alphaGen wave and animMap (at the shader time of \"Built map as in the game\"). "
     "Stages with glow glow like in the game, also seen in mirrors and portals and on effects (blur and overlay from \"Built map as in "
     "the game\"). Not drawn: video stages and stages with rgbGen vertex/lightingDiffuse/entity. Surface sprites (grass) as sprites. "
     "What the build gives is shown as in the game by \"Built map as in the game\".",
     "Das Echtzeit-Licht ist eine Vorschau ohne Bau: kein Bounce, kein Filter, kein Supersampling, Schatten aus Schattenkarten statt "
     "aus Strahlen. Die erste Stufe eines Shaders (bzw. $lightmap mit ihrer Textur) bekommt das Licht; die weiteren Stufen folgen wie im "
     "Spiel, auch mit tcMod, tcGen environment/vector, rgbGen/alphaGen wave und animMap (zur Shaderzeit von „Gebaute Karte wie im "
     "Spiel“). Stufen mit glow leuchten wie im Spiel, auch im Spiegel, im Portal und an Effekten (Weichzeichnen und Auflegen aus "
     "„Gebaute Karte wie im Spiel“). Nicht gezeichnet: Videostufen und Stufen mit rgbGen vertex/lightingDiffuse/entity. surfaceSprites "
     "(Gras) als Halme. Was der Bau ergibt, zeigt „Gebaute Karte wie im Spiel“ wie im Spiel.",
     "实时光照是无需构建的预览:没有反弹、没有滤波、没有超采样,阴影来自阴影贴图而非光线追踪。着色器的第一阶段(或 $lightmap 及其纹理)获得光照;"
     "后续阶段与游戏中一样叠加,也包括 tcMod、tcGen environment/vector、rgbGen/alphaGen wave 和 animMap(使用“已构建的地图如游戏中显示”的"
     "着色器时间)。带 glow 的阶段与游戏中一样发光,在镜子、传送门中和特效上也是如此(模糊和叠加与“已构建的地图如游戏中显示”相同)。"
     "不绘制:视频阶段以及使用 rgbGen vertex/lightingDiffuse/entity 的阶段。表面精灵(草)显示为精灵。构建的结果由“已构建的地图如游戏中显示”"
     "按游戏中的样子显示。",
     "リアルタイムライトはビルドなしのプレビューです。バウンスなし、フィルターなし、スーパーサンプリングなし、影はレイではなくシャドウマップから。"
     "シェーダーの最初のステージ (または $lightmap とそのテクスチャ) が光を受け、以降のステージはゲームと同じように重なります。tcMod、"
     "tcGen environment/vector、rgbGen/alphaGen wave、animMap も含みます (「ビルドしたマップをゲームと同じ表示に」のシェーダー時間)。"
     "glow 付きのステージはゲームと同じように光り、鏡やポータルの中、エフェクトでも同じです (ぼかしと合成は「ビルドしたマップをゲームと"
     "同じ表示に」と共通)。描かないもの: ビデオステージと rgbGen vertex/lightingDiffuse/entity のステージ。サーフェススプライト (草) は"
     "スプライトとして表示します。ビルドの結果は「ビルドしたマップをゲームと同じ表示に」でゲームと同じように表示されます。",
     "La luz en tiempo real es una vista previa sin compilar: sin rebote, sin filtro, sin supermuestreo, sombras de mapas de sombras en "
     "lugar de rayos. La primera etapa de un shader (o $lightmap con su textura) recibe la luz; las etapas siguientes se suman como en el "
     "juego, también con tcMod, tcGen environment/vector, rgbGen/alphaGen wave y animMap (con el tiempo de shader de «Mapa compilado como "
     "en el juego»). Las etapas con glow brillan como en el juego, también vistas en espejos y portales y en efectos (desenfoque y "
     "superposición de «Mapa compilado como en el juego»). No se dibujan: etapas de vídeo y etapas con rgbGen vertex/lightingDiffuse/entity. "
     "Los surface sprites (hierba) como sprites. Lo que da la compilación lo muestra como en el juego «Mapa compilado como en el juego».",
     "Světlo v reálném čase je náhled bez sestavení: bez odrazů, bez filtru, bez supersamplingu, stíny ze stínových map místo paprsků. "
     "První fáze shaderu (nebo $lightmap s její texturou) dostane světlo; další fáze se přidají jako ve hře, i s tcMod, tcGen "
     "environment/vector, rgbGen/alphaGen wave a animMap (v čase shaderu z „Sestavená mapa jako ve hře“). Fáze s glow září jako ve hře, "
     "i v zrcadle, v portálu a na efektech (rozmazání a překrytí z „Sestavená mapa jako ve hře“). Nekreslí se: video fáze a fáze s rgbGen "
     "vertex/lightingDiffuse/entity. Surface sprity (tráva) jako sprity. Co dá sestavení, ukazuje jako ve hře „Sestavená mapa jako ve hře“."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
