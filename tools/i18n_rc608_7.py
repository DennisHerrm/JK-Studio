# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc608 Teil 7 (Bildstufe wie r_picmip) fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("MapPicmip", "Texture detail (r_picmip)", "Bildstufe (r_picmip)", "纹理细节 (r_picmip)", "テクスチャ詳細 (r_picmip)",
     "Detalle de texturas (r_picmip)", "Detail textur (r_picmip)"),
    ("MapPicmipHint",
     "Like r_picmip in the game: every step halves the textures and saves video memory. 0 is the default of OpenJK and Movie Duels. The textures are read again.",
     "Wie r_picmip im Spiel: Jede Stufe halbiert die Bilder und spart Grafikspeicher. 0 ist die Vorgabe von OpenJK und Movie Duels. Die Bilder werden neu gelesen.",
     "与游戏中的 r_picmip 相同:每一级将纹理减半并节省显存。0 是 OpenJK 和 Movie Duels 的默认值。纹理会重新读取。",
     "ゲームの r_picmip と同じです。1 段階ごとにテクスチャを半分にし、ビデオメモリを節約します。0 が OpenJK と Movie Duels の既定値です。テクスチャは読み直されます。",
     "Como r_picmip en el juego: cada nivel reduce las texturas a la mitad y ahorra memoria de vídeo. 0 es el valor predeterminado de OpenJK y Movie Duels. Las texturas se vuelven a leer.",
     "Jako r_picmip ve hře: každý stupeň zmenší textury na polovinu a šetří grafickou paměť. 0 je výchozí hodnota v OpenJK a Movie Duels. Textury se načtou znovu."),
]


def zeilen_vier():
    """(Kennung, English, Deutsch, 中文, 日本語) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
