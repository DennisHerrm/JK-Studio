# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc610 LICHT1 (Echtzeit-Licht der ganzen Karte) fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("KeLichtWuerfelFortschritt", "shadows %d / %d", "Schatten %d / %d", "阴影 %d / %d", "影 %d / %d", "sombras %d / %d", "stíny %d / %d"),
    ("KeLichtWuerfelGrenze",
     "Real-time light: %d of %d lights without shadows - the shadow cubes use their whole graphics memory budget (%d MB)",
     "Echtzeit-Licht: %d von %d Lichtern ohne Schatten - die Schattenwürfel nutzen ihr ganzes Budget im Grafikspeicher (%d MB)",
     "实时光照:%d / %d 个光源没有阴影——阴影立方体已用完其全部显存预算(%d MB)",
     "リアルタイムライト: %d / %d 個のライトに影がありません - 影キューブがグラフィックメモリの予算をすべて使っています(%d MB)",
     "Luz en tiempo real: %d de %d luces sin sombras; los cubos de sombra usan todo su presupuesto de memoria gráfica (%d MB)",
     "Světlo v reálném čase: %d z %d světel bez stínů - stínové krychle využívají celý svůj rozpočet grafické paměti (%d MB)"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
