# -*- coding: utf-8 -*-
"""Oberflaechentexte von TEST1411 (14.11.: Testrueckmeldungen von shank zu rc612).

I: im Bereich Animation ist ein Knochen von Anfang an waehlbar (wie 3ds Max), auch ohne Control Rig.
Begriffe wie die vorhandenen Texte (AP4 "Knochen waehlen"/"Select bone", AP10 Control Rig, ENT2 IK/FK).
Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    # --- I: Knochen im Bereich Animation ---
    ("T1KnochenGewaehlt",
     "Bone %s selected - E rotates it, K sets a key (no control rig needed).",
     "Knochen %s gewählt - E dreht ihn, K setzt einen Schlüssel (ohne Control Rig).",
     "已选择骨骼 %s - E 旋转, K 设置关键帧 (无需控制绑定)。",
     "ボーン %s を選択しました - E で回転、K でキーを設定 (コントロールリグは不要)。",
     "Hueso %s seleccionado: E lo gira, K pone una clave (sin control rig).",
     "Kost %s vybrána - E ji otáčí, K nastaví klíč (bez control rigu)."),
    # --- H: Knochen als Kette je Glied ---
    ("T1Hilfsknochen", "Show helper bones", "Hilfsknochen zeigen", "显示辅助骨骼", "補助ボーンを表示", "Mostrar huesos auxiliares", "Zobrazit pomocné kosti"),
    ("T1HilfsknochenHint",
     "Twist helpers (e.g. lradiusX: same axis as the parent, in front of a sibling, no chain of their own) as a box instead of a small point. "
     "Every other bone is one box from its joint along its own axis to the next joint of the chain, like 3ds Max bones.",
     "Drehhilfen (z. B. lradiusX: gleiche Achse wie der Elternknochen, vor einem Geschwister, ohne eigene Kette) als Kasten statt als kleiner Punkt. "
     "Jeder andere Knochen ist ein Kasten von seinem Gelenk entlang seiner Achse bis zum nächsten Gelenk der Kette, wie Bones in 3ds Max.",
     "将扭转辅助骨骼 (例如 lradiusX: 与父骨骼同轴、位于同级骨骼之前、没有自己的骨骼链) 显示为方框而不是小点。"
     "其他每根骨骼都是一个方框, 从其关节沿自身轴线延伸到骨骼链的下一个关节, 与 3ds Max 的骨骼相同。",
     "ねじれ補助ボーン (例: lradiusX - 親と同じ軸、兄弟ボーンの手前、独自のチェーンなし) を小さな点ではなくボックスで表示します。"
     "他のボーンはすべて、関節から自身の軸に沿ってチェーンの次の関節まで伸びる 1 つのボックスです (3ds Max のボーンと同じ)。",
     "Ayudantes de torsión (p. ej. lradiusX: mismo eje que el padre, delante de un hermano, sin cadena propia) como caja en lugar de un punto pequeño. "
     "Cada uno de los demás huesos es una caja desde su articulación a lo largo de su eje hasta la siguiente articulación de la cadena, como los huesos de 3ds Max.",
     "Pomocné kosti pro natočení (např. lradiusX: stejná osa jako rodič, před sourozencem, bez vlastního řetězce) jako kvádr místo malého bodu. "
     "Každá jiná kost je jeden kvádr od svého kloubu podél vlastní osy k dalšímu kloubu řetězce, jako kosti v 3ds Max."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
