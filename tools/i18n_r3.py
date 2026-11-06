# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc610 R3 (gespiegelte Modelle, Glow im Echtzeit-Licht) fuer tools/gen_i18n.py und die weiteren
Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("MeMdMpFehlt",
     "The Movie Duels multiplayer menus are not installed: ui/MD_MP (menudef.h, the menu list MovieDuels-MP-Menus.txt and their "
     "fonts) is missing from the game data, so this menu cannot be shown as in the game - texts stay empty. Movie Duels has no finished "
     "multiplayer menu set yet; the single player menus (ui/) work.",
     "Die Mehrspieler-Menüs von Movie Duels sind nicht installiert: ui/MD_MP (menudef.h, die Menüliste MovieDuels-MP-Menus.txt und "
     "ihre Schriften) fehlt in den Spieldaten, darum kann dieses Menü nicht wie im Spiel erscheinen - Texte bleiben leer. Movie Duels "
     "hat noch keinen fertigen Mehrspieler-Menüsatz; die Einzelspieler-Menüs (ui/) gehen.",
     "未安装 Movie Duels 多人菜单:游戏数据中缺少 ui/MD_MP(menudef.h、菜单列表 MovieDuels-MP-Menus.txt 及其字体),因此该菜单无法按游戏中的样子"
     "显示——文字保持为空。Movie Duels 还没有完成的多人菜单集;单人菜单(ui/)可以使用。",
     "Movie Duels のマルチプレイメニューがインストールされていません: ゲームデータに ui/MD_MP (menudef.h、メニュー一覧 "
     "MovieDuels-MP-Menus.txt とそのフォント) がないため、このメニューはゲームと同じには表示できません。テキストは空のままです。"
     "Movie Duels にはまだ完成したマルチプレイ用メニューセットがありません。シングルプレイのメニュー (ui/) は使えます。",
     "Los menús multijugador de Movie Duels no están instalados: falta ui/MD_MP (menudef.h, la lista de menús MovieDuels-MP-Menus.txt y "
     "sus fuentes) en los datos del juego, así que este menú no puede mostrarse como en el juego; los textos quedan vacíos. Movie Duels "
     "aún no tiene un conjunto de menús multijugador terminado; los menús de un jugador (ui/) funcionan.",
     "Menu Movie Duels pro více hráčů nejsou nainstalována: v herních datech chybí ui/MD_MP (menudef.h, seznam menu "
     "MovieDuels-MP-Menus.txt a jejich písma), proto toto menu nemůže vypadat jako ve hře - texty zůstanou prázdné. Movie Duels zatím "
     "nemá hotovou sadu menu pro více hráčů; menu pro jednoho hráče (ui/) fungují."),
    ("KeOptPrModellGespiegelt", "Mirrored model the game shows inside out", "Gespiegeltes Modell, das das Spiel innen zeigt",
     "游戏中显示为内侧的镜像模型", "ゲームで内側が見える反転モデル", "Modelo reflejado que el juego muestra por dentro",
     "Zrcadlený model, který hra ukáže zevnitř"),
    ("KeOptPrModellGespiegeltE",
     "These entities are loaded by the game at run time with a scale that mirrors them (modelscale_vec with one or three negative "
     "axes, e.g. after Flip X). The game draws them without turning the triangles around, so you see their inside - the camera shows "
     "it the same way. Only misc_model is turned around when q3map2 bakes it in. Fix: use a misc_model, or rotate instead of mirroring "
     "(two mirrored axes are a rotation).",
     "Diese Entities lädt das Spiel zur Laufzeit mit einer Skalierung, die sie spiegelt (modelscale_vec mit einer oder drei negativen "
     "Achsen, z. B. nach Flip X). Das Spiel zeichnet sie, ohne die Dreiecke umzudrehen - man sieht ihr Inneres; die Kamera zeigt es "
     "ebenso. Nur misc_model dreht q3map2 beim Einbacken um. Abhilfe: ein misc_model nehmen oder drehen statt spiegeln (zwei "
     "gespiegelte Achsen sind eine Drehung).",
     "这些实体由游戏在运行时加载,其缩放会把它们镜像(modelscale_vec 有一个或三个负轴,例如 Flip X 之后)。游戏绘制时不会翻转三角形,所以会看到"
     "它们的内侧——摄像机也同样显示。只有 misc_model 会在 q3map2 烘焙时被翻转。解决:改用 misc_model,或用旋转代替镜像(两个镜像轴等于一次旋转)。",
     "これらのエンティティはゲームが実行時に、反転するスケール (負の軸が 1 つまたは 3 つの modelscale_vec。例: Flip X の後) で読み込みます。"
     "ゲームは三角形を裏返さずに描くので内側が見えます。カメラも同じように表示します。q3map2 が焼き込むときに裏返すのは misc_model だけです。"
     "対処: misc_model を使うか、反転の代わりに回転します (反転した軸が 2 つなら回転と同じです)。",
     "El juego carga estas entidades en tiempo de ejecución con una escala que las refleja (modelscale_vec con uno o tres ejes "
     "negativos, p. ej. tras Flip X). El juego las dibuja sin dar la vuelta a los triángulos, así que se ve su interior; la cámara lo "
     "muestra igual. q3map2 solo da la vuelta a misc_model al integrarlo. Solución: usar un misc_model, o girar en lugar de reflejar "
     "(dos ejes reflejados son un giro).",
     "Tyto entity hra načítá za běhu s měřítkem, které je zrcadlí (modelscale_vec s jednou nebo třemi zápornými osami, např. po Flip X). "
     "Hra je kreslí bez otočení trojúhelníků, takže je vidět jejich vnitřek - kamera to ukazuje stejně. Jen misc_model otočí q3map2 při "
     "zapečení. Náprava: použít misc_model, nebo místo zrcadlení otočit (dvě zrcadlené osy jsou otočení)."),
    ("KeLichtGrenzenTip3",
     "The real-time light is a preview without a build: no bounce, no filter, no supersampling, shadows from shadow maps instead of "
     "traced rays. The first stage of a shader (or $lightmap with its texture) gets the light; the further stages follow as in the game "
     "when they have a fixed image without tcMod, tcGen and waves (e.g. GL_ONE GL_ONE), stages with glow also glow like in the game "
     "(blur and overlay from \"Built map as in the game\"). Mirrors and portals with all their stages, surface sprites (grass) as sprites. "
     "What the build gives is shown as in the game by \"Built map as in the game\".",
     "Das Echtzeit-Licht ist eine Vorschau ohne Bau: kein Bounce, kein Filter, kein Supersampling, Schatten aus Schattenkarten statt "
     "aus Strahlen. Die erste Stufe eines Shaders (bzw. $lightmap mit ihrer Textur) bekommt das Licht; die weiteren Stufen folgen wie im "
     "Spiel, wenn sie ein festes Bild ohne tcMod, tcGen und Wellen haben (z. B. GL_ONE GL_ONE), Stufen mit glow leuchten auch wie im "
     "Spiel (Weichzeichnen und Auflegen aus „Gebaute Karte wie im Spiel“). Spiegel und Portale mit allen Stufen, surfaceSprites (Gras) "
     "als Halme. Was der Bau ergibt, zeigt „Gebaute Karte wie im Spiel“ wie im Spiel.",
     "实时光照是无需构建的预览:没有反弹、没有滤波、没有超采样,阴影来自阴影贴图而非光线追踪。着色器的第一阶段(或 $lightmap 及其纹理)获得光照;"
     "后续阶段若使用固定图像且没有 tcMod、tcGen 和波形(例如 GL_ONE GL_ONE),则与游戏中一样叠加,带 glow 的阶段也与游戏中一样发光"
     "(模糊和叠加与“已构建的地图如游戏中显示”相同)。镜子和传送门显示全部阶段,表面精灵(草)显示为精灵。构建的结果由“已构建的地图如游戏中显示”"
     "按游戏中的样子显示。",
     "リアルタイムライトはビルドなしのプレビューです。バウンスなし、フィルターなし、スーパーサンプリングなし、影はレイではなくシャドウマップから。"
     "シェーダーの最初のステージ (または $lightmap とそのテクスチャ) が光を受け、以降のステージは tcMod・tcGen・波形のない固定画像なら "
     "(例: GL_ONE GL_ONE) ゲームと同じように重なり、glow 付きのステージもゲームと同じように光ります (ぼかしと合成は「ビルドしたマップを"
     "ゲームと同じ表示に」と共通)。鏡とポータルは全ステージ、サーフェススプライト (草) はスプライトとして表示します。ビルドの結果は"
     "「ビルドしたマップをゲームと同じ表示に」でゲームと同じように表示されます。",
     "La luz en tiempo real es una vista previa sin compilar: sin rebote, sin filtro, sin supermuestreo, sombras de mapas de sombras en "
     "lugar de rayos. La primera etapa de un shader (o $lightmap con su textura) recibe la luz; las etapas siguientes se suman como en el "
     "juego cuando tienen una imagen fija sin tcMod, tcGen ni ondas (p. ej. GL_ONE GL_ONE), y las etapas con glow brillan también como en "
     "el juego (desenfoque y superposición de «Mapa compilado como en el juego»). Los espejos y portales con todas sus etapas, los surface "
     "sprites (hierba) como sprites. Lo que da la compilación lo muestra como en el juego «Mapa compilado como en el juego».",
     "Světlo v reálném čase je náhled bez sestavení: bez odrazů, bez filtru, bez supersamplingu, stíny ze stínových map místo paprsků. "
     "První fáze shaderu (nebo $lightmap s její texturou) dostane světlo; další fáze se přidají jako ve hře, pokud mají pevný obrázek bez "
     "tcMod, tcGen a vln (např. GL_ONE GL_ONE), fáze s glow také září jako ve hře (rozmazání a překrytí z „Sestavená mapa jako ve hře“). "
     "Zrcadla a portály se všemi fázemi, surface sprity (tráva) jako sprity. Co dá sestavení, ukazuje jako ve hře „Sestavená mapa jako ve "
     "hře“."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
