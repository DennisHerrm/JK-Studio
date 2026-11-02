# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc608 Teil 5 (Figurenlicht) fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("MapBrightnessHint", "Brightens the map and the characters on it so you can see in dark places - 1.0x is exactly like the game",
     "Hellt die Karte und die Figuren darauf auf, damit man im Dunkeln etwas sieht - 1,0x ist genau wie im Spiel",
     "提亮地图及其上的角色,以便在黑暗处看清 - 1.0x 与游戏完全相同",
     "暗い場所でも見えるようにマップとキャラクターを明るくします - 1.0x はゲームとまったく同じです",
     "Aclara el mapa y los personajes para ver en las zonas oscuras; 1,0x es exactamente como el juego",
     "Zesvětlí mapu a postavy na ní, abyste viděli v tmavých místech - 1,0x je přesně jako ve hře"),
]


def zeilen_vier():
    """(Kennung, English, Deutsch, 中文, 日本語) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
