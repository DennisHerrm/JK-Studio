# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc608 Teil 13b (Patches wie im Spiel) fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("MapDetailSpiel", "as in the game", "wie im Spiel", "与游戏相同", "ゲームと同じ", "como en el juego", "jako ve hře"),
    ("MapDetailTip",
     "Far right (the default): curved surfaces (patches) are divided like the game does it - finer where they bend more (r_subdivisions 4), and in every frame the far ones drop rows and columns (r_lodCurveError 250). 1 to 10: an even mesh with that many parts per patch piece.",
     "Ganz rechts (Vorgabe): gekrümmte Flächen (Patches) werden wie im Spiel unterteilt - feiner, wo sie stärker gebogen sind (r_subdivisions 4), und ferne lassen in jedem Bild Zeilen und Spalten weg (r_lodCurveError 250). 1 bis 10: ein gleichmäßiges Netz mit so vielen Teilen je Patchfeld.",
     "最右侧(默认):曲面(补丁)按游戏的方式细分——弯曲越大的地方越细(r_subdivisions 4),远处的补丁在每一帧省略行和列(r_lodCurveError 250)。1 到 10:每个补丁块分成这么多份的均匀网格。",
     "右端(既定):曲面(パッチ)をゲームと同じように分割します。曲がりの強いところほど細かく(r_subdivisions 4)、遠くのパッチは毎フレーム行と列を省きます(r_lodCurveError 250)。1 から 10:パッチの区画ごとにその数で分けた均等なメッシュ。",
     "A la derecha del todo (por defecto): las superficies curvas (patches) se dividen como en el juego, más finas donde se curvan más (r_subdivisions 4), y las lejanas omiten filas y columnas en cada fotograma (r_lodCurveError 250). De 1 a 10: una malla uniforme con tantas partes por tramo de patch.",
     "Úplně vpravo (výchozí): zakřivené plochy (patche) se dělí jako ve hře – jemněji tam, kde se víc ohýbají (r_subdivisions 4), a vzdálené v každém snímku vynechávají řádky a sloupce (r_lodCurveError 250). 1 až 10: rovnoměrná síť s tolika díly na pole patche."),
]


def zeilen_vier():
    """(Kennung, English, Deutsch, 中文, 日本語) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
