# -*- coding: utf-8 -*-
"""Oberflaechentexte des Zweigs mbvers (Verschieben, Drehen, Skalieren und Modellieren in einer Pose) fuer
tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("MbvEntartet",
     "%d vertices have a bone mix that cannot be inverted in this pose: they follow their strongest bone (vertices without a valid bone stay put).",
     "%d Ecken haben eine Knochenmischung, die sich in dieser Pose nicht umkehren lässt: Sie folgen ihrem stärksten Knochen (Ecken ohne gültigen Knochen bleiben stehen).",
     "%d 个顶点的骨骼混合在此姿势下不可逆: 它们跟随其最强的骨骼 (没有有效骨骼的顶点保持不动)。",
     "%d 個の頂点はこのポーズでボーンの混合を逆算できません: 最も強いボーンに従います (有効なボーンのない頂点は動きません)。",
     "%d vértices tienen una mezcla de huesos que no se puede invertir en esta pose: siguen a su hueso más fuerte (los vértices sin hueso válido no se mueven).",
     "%d vrcholů má směs kostí, kterou v této póze nelze obrátit: řídí se svou nejsilnější kostí (vrcholy bez platné kosti zůstanou na místě)."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
