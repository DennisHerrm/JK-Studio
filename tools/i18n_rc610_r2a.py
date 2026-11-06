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
