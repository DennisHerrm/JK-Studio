# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc609 F (Funde der Codepruefung) fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("LmsgL007",
     "the block of %s is nested deeper than 256 levels - the rest of the file is kept as text and not compiled",
     "der Block von %s ist tiefer als 256 Ebenen verschachtelt - der Rest der Datei bleibt als Text stehen und wird nicht übersetzt",
     "%s 的块嵌套超过 256 层 - 文件的其余部分保留为文本,不会被编译",
     "%s のブロックの入れ子が 256 段を超えています - ファイルの残りはテキストのまま残り、コンパイルされません",
     "el bloque de %s está anidado a más de 256 niveles - el resto del archivo se conserva como texto y no se compila",
     "blok %s je vnořen hlouběji než 256 úrovní - zbytek souboru zůstane jako text a nepřeloží se"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
