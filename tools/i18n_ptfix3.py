# -*- coding: utf-8 -*-
"""Oberflaechentexte von PTFIX3 (08.11.: Painting-Funde P23-P31 der Pruefung seit rc610) fuer tools/gen_i18n.py und die
weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    # P24: "Auf neue UV umbacken" paart nur dieselben Dreiecke derselben Detailstufe
    ("PtUmbackenAnders", "The triangles of this image have changed (for example by Detach or a new part) - baking onto new UVs needs the same triangles with changed UVs.",
     "Die Dreiecke dieses Bilds haben sich geändert (zum Beispiel durch Detach oder einen neuen Teil) - Umbacken braucht dieselben Dreiecke mit geänderter UV.",
     "该图像的三角形已更改(例如通过分离或新部件)- 烘焙到新 UV 需要相同的三角形和更改后的 UV。",
     "この画像の三角形が変わりました (デタッチや新しいパーツなど) - 新しい UV への焼き付けには、UV だけが変わった同じ三角形が必要です。",
     "Los triángulos de esta imagen han cambiado (por ejemplo con Detach o una pieza nueva); hornear sobre UV nuevas necesita los mismos triángulos con UV cambiadas.",
     "Trojúhelníky tohoto obrázku se změnily (například přes Detach nebo novou část) - přepečení na nové UV potřebuje stejné trojúhelníky se změněnými UV."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
