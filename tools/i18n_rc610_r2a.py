# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc610 R2a (Karten-Editor-Reste vor rc610) fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("KeLumpsTeilBau",
     "While the part was compiling, the window \"Shaders and surfaces\" got changes.\n"
     "Keep them in the finished map, or replace them with the state of the file?",
     "Während der Teil gebaut wurde, hat sich im Fenster „Shader und Flächen“ etwas geändert.\n"
     "Sollen die Änderungen in der fertigen Karte bleiben oder durch den Stand der Datei ersetzt werden?",
     "在编译该部分时,“着色器和表面”窗口中有更改。\n要在完成的地图中保留这些更改,还是用文件的状态替换它们?",
     "パーツのコンパイル中に「シェーダーとサーフェス」ウィンドウで変更がありました。\n"
     "変更を完成したマップに残しますか、それともファイルの状態で置き換えますか?",
     "Mientras se compilaba la parte, la ventana «Shaders y superficies» tuvo cambios.\n"
     "¿Quieres conservarlos en el mapa terminado o sustituirlos por el estado del archivo?",
     "Během sestavování části se v okně „Shadery a plochy“ něco změnilo.\n"
     "Mají změny v hotové mapě zůstat, nebo je nahradit stav souboru?"),
    ("KeLichtGrenzenTip2",
     "The real-time light is a preview without a build: no bounce, no filter, no supersampling, shadows from shadow maps instead of traced rays, and of each shader only the first stage with its light - mirrors and portals with all their stages, surface sprites (grass) as sprites. What the build gives is shown as in the game by \"Built map as in the game\".",
     "Das Echtzeit-Licht ist eine Vorschau ohne Bau: kein Bounce, kein Filter, kein Supersampling, Schatten aus Schattenkarten statt aus Strahlen, und von jedem Shader nur die erste Stufe mit ihrem Licht - Spiegel und Portale mit allen Stufen, surfaceSprites (Gras) als Halme. Was der Bau ergibt, zeigt „Gebaute Karte wie im Spiel“ wie im Spiel.",
     "实时光照是无需构建的预览:没有反弹、没有滤波、没有超采样,阴影来自阴影贴图而非光线追踪,每个着色器只显示第一阶段及其光照——镜子和传送门显示全部阶段,表面精灵(草)显示为精灵。构建的结果由“已构建的地图如游戏中显示”按游戏中的样子显示。",
     "リアルタイムライトはビルドなしのプレビューです。バウンスなし、フィルターなし、スーパーサンプリングなし、影はレイではなくシャドウマップから、各シェーダーは最初のステージとその光だけです。ただし鏡とポータルは全ステージ、サーフェススプライト(草)はスプライトとして表示します。ビルドの結果は「ビルドしたマップをゲームと同じ表示に」でゲームと同じように表示されます。",
     "La luz en tiempo real es una vista previa sin compilar: sin rebote, sin filtro, sin supermuestreo, sombras de mapas de sombras en lugar de rayos, y de cada shader solo la primera etapa con su luz; los espejos y portales con todas sus etapas, los surface sprites (hierba) como sprites. Lo que da la compilación lo muestra como en el juego «Mapa compilado como en el juego».",
     "Světlo v reálném čase je náhled bez sestavení: bez odrazů, bez filtru, bez supersamplingu, stíny ze stínových map místo paprsků a z každého shaderu jen první fáze s jejím světlem - zrcadla a portály se všemi fázemi, surface sprity (tráva) jako sprity. Co dá sestavení, ukazuje jako ve hře „Sestavená mapa jako ve hře“."),
    ("KeLumpsBehalten", "Keep", "Behalten", "保留", "残す", "Conservar", "Ponechat"),
    ("KeLumpsErsetzen", "Replace", "Ersetzen", "替换", "置き換える", "Sustituir", "Nahradit"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
