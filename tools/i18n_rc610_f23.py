# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc610 F23 (Funde der Codepruefung Bereich 2 und 3) fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("KeLumpsUngespeichert",
     "The compiled map has unsaved changes in the window \"Shaders and surfaces\".\nDo you want to save them before continuing?",
     "Die fertige Karte hat ungespeicherte Änderungen im Fenster „Shader und Flächen“.\nMöchtest du sie speichern, bevor es weitergeht?",
     "已编译地图在“着色器和表面”窗口中有未保存的更改。\n要在继续之前保存它们吗?",
     "コンパイル済みマップの「シェーダーとサーフェス」ウィンドウに未保存の変更があります。\n続行する前に保存しますか?",
     "El mapa compilado tiene cambios sin guardar en la ventana «Shaders y superficies».\n¿Quieres guardarlos antes de continuar?",
     "Hotová mapa má neuložené změny v okně „Shadery a plochy“.\nChcete je před pokračováním uložit?"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
