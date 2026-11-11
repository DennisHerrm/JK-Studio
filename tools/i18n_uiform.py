# -*- coding: utf-8 -*-
"""Oberflaechentexte von UIFORM (Formular-Baustein, Mindestgroessen aller Fenster) fuer tools/gen_i18n.py und die weiteren
Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("UfAbbrechen", "Cancel", "Abbrechen", "取消", "キャンセル", "Cancelar", "Zrušit"),
    ("UfSchliessen", "Close", "Schließen", "关闭", "閉じる", "Cerrar", "Zavřít"),
    ("UfGewaehlteHint",
     "Puts the animation selected in the animation list into this field.",
     "Setzt die in der Animationsliste gewählte Animation in dieses Feld.",
     "将动画列表中选中的动画填入此字段。",
     "アニメーション一覧で選択中のアニメーションをこの欄に入れます。",
     "Pone en este campo la animación elegida en la lista de animaciones.",
     "Vloží do tohoto pole animaci vybranou v seznamu animací."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
