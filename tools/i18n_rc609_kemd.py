# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc609 KEMD (Karten-Editor mit Movie-Duels-Daten: Asset-Browser sammelt im Hintergrund)
fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("KeAbSammelt",
     "Collecting models and sounds - the list fills when done",
     "Modelle und Klänge werden gesammelt - die Liste erscheint, sobald es fertig ist",
     "正在收集模型和声音——完成后列表会出现",
     "モデルとサウンドを収集中 - 完了するとリストが表示されます",
     "Recopilando modelos y sonidos: la lista aparece al terminar",
     "Shromažďují se modely a zvuky - seznam se objeví po dokončení"),
]


def zeilen_vier():
    """(Kennung, English, Deutsch, 中文, 日本語) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
