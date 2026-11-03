# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc608 Teil 11 (Karten-Editor wie im Spiel) fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("KeSpielbildMenue", "Built map as in the game", "Gebaute Karte wie im Spiel", "已构建的地图如游戏中显示",
     "ビルドしたマップをゲームと同じ表示に", "Mapa compilado como en el juego", "Sestavená mapa jako ve hře"),
    ("KeSpielbildMenueTip",
     "After the quick bake (or with a template .bsp) the camera shows the built map through the same renderer as the preview in the Mission tab: every shader stage, the sky box, glow, fog, what the game hides through the PVS and the game's gamma, as in the game. Selection, handles and lines stay on top. Map models (misc_model_static), the sky portal (misc_skyportal) and weather come through the same code as in the preview; figures, effects and surface sprites are not part of this picture, and doors and platforms stand where q3map2 put them.",
     "Nach dem Schnell-Backen (oder mit einer Vorlage-.bsp) zeigt die Kamera die gebaute Karte durch denselben Zeichner wie die Vorschau im Mission-Tab: alle Stufen der Shader, die Himmelsbox, Glühen, Nebel, was das Spiel über das PVS weglässt, und das Gamma des Spiels, wie im Spiel. Auswahl, Griffe und Linien bleiben darüber. Kartenmodelle (misc_model_static), das Himmelsportal (misc_skyportal) und Wetter kommen über denselben Code wie in der Vorschau; Figuren, Effekte und Surface-Sprites gehören nicht zu diesem Bild, und Türen und Plattformen stehen, wo q3map2 sie abgelegt hat.",
     "快速烘焙之后(或使用模板 .bsp 时),摄像机通过与任务选项卡预览相同的渲染器显示已构建的地图:所有着色器阶段、天空盒、辉光、雾、游戏通过 PVS 隐藏的部分以及游戏的伽马,与游戏中一致。选择、手柄和线条保持在上方。地图模型(misc_model_static)、天空传送门(misc_skyportal)和天气通过与预览相同的代码显示;角色、特效和表面精灵不在此画面中,门和平台位于 q3map2 放置它们的位置。",
     "クイックベイクの後(またはテンプレート .bsp があるとき)、カメラはミッションタブのプレビューと同じレンダラーでビルド済みマップを表示します。シェーダーの全ステージ、スカイボックス、グロー、フォグ、ゲームが PVS で隠す部分、ゲームのガンマがゲームと同じになります。選択、ハンドル、線はその上に残ります。マップモデル(misc_model_static)、スカイポータル(misc_skyportal)、天候はプレビューと同じコードで描かれます。キャラクター、エフェクト、サーフェススプライトはこの画像に含まれず、ドアやプラットフォームは q3map2 が置いた位置にあります。",
     "Tras el horneado rápido (o con un .bsp de plantilla) la cámara muestra el mapa compilado con el mismo renderizador que la vista previa de la pestaña Misión: todas las etapas de los shaders, la caja de cielo, el brillo, la niebla, lo que el juego oculta mediante el PVS y la gamma del juego, como en el juego. La selección, los tiradores y las líneas quedan encima. Los modelos del mapa (misc_model_static), el portal de cielo (misc_skyportal) y el clima pasan por el mismo código que en la vista previa; las figuras, los efectos y los surface sprites no forman parte de esta imagen, y las puertas y plataformas están donde las dejó q3map2.",
     "Po rychlém zapečení (nebo se šablonou .bsp) ukazuje kamera sestavenou mapu stejným vykreslovačem jako náhled na kartě Mise: všechny fáze shaderů, skybox, záři, mlhu, co hra skrývá přes PVS, a gamu hry, jako ve hře. Výběr, úchyty a čáry zůstávají nahoře. Modely mapy (misc_model_static), portál oblohy (misc_skyportal) a počasí jdou stejným kódem jako v náhledu; postavy, efekty a surface sprity nejsou součástí tohoto obrazu a dveře a plošiny stojí tam, kam je umístil q3map2."),
    ("KeLichtGrenzenTip",
     "The real-time light is a preview without a build: no bounce, no filter, no supersampling, shadows from shadow maps instead of traced rays, and of each shader only the first stage with its light. What the build gives is shown as in the game by \"Built map as in the game\".",
     "Das Echtzeit-Licht ist eine Vorschau ohne Bau: kein Bounce, kein Filter, kein Supersampling, Schatten aus Schattenkarten statt aus Strahlen, und von jedem Shader nur die erste Stufe mit ihrem Licht. Was der Bau ergibt, zeigt „Gebaute Karte wie im Spiel“ wie im Spiel.",
     "实时光照是无需构建的预览:没有反弹、没有滤波、没有超采样,阴影来自阴影贴图而非光线追踪,每个着色器只显示第一阶段及其光照。构建的结果由“已构建的地图如游戏中显示”按游戏中的样子显示。",
     "リアルタイムライトはビルドなしのプレビューです。バウンスなし、フィルターなし、スーパーサンプリングなし、影はレイではなくシャドウマップから、各シェーダーは最初のステージとその光だけです。ビルドの結果は「ビルドしたマップをゲームと同じ表示に」でゲームと同じように表示されます。",
     "La luz en tiempo real es una vista previa sin compilar: sin rebote, sin filtro, sin supermuestreo, sombras de mapas de sombras en lugar de rayos, y de cada shader solo la primera etapa con su luz. Lo que da la compilación lo muestra como en el juego «Mapa compilado como en el juego».",
     "Světlo v reálném čase je náhled bez sestavení: bez odrazů, bez filtru, bez supersamplingu, stíny ze stínových map místo paprsků a z každého shaderu jen první fáze s jejím světlem. Co dá sestavení, ukazuje jako ve hře „Sestavená mapa jako ve hře“."),
    ("KeLichtSpielbild", "Built map as in the game (quick bake %.1f s)", "Gebaute Karte wie im Spiel (Schnell-Backen %.1f s)",
     "已构建的地图如游戏中显示(快速烘焙 %.1f 秒)", "ビルドしたマップをゲームと同じ表示(クイックベイク %.1f 秒)",
     "Mapa compilado como en el juego (horneado rápido %.1f s)", "Sestavená mapa jako ve hře (rychlé zapečení %.1f s)"),
    ("KeLichtSpielbildVorlage", "Template .bsp as in the game", "Vorlage-.bsp wie im Spiel", "模板 .bsp 如游戏中显示",
     "テンプレート .bsp をゲームと同じ表示", ".bsp de plantilla como en el juego", "Šablona .bsp jako ve hře"),
]


def zeilen_vier():
    """(Kennung, English, Deutsch, 中文, 日本語) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
