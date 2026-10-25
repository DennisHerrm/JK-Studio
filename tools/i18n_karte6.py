# -*- coding: utf-8 -*-
"""Oberflaechentexte des Karten-Editors, Stufe 6 (Pruefstufe), fuer
tools/gen_i18n.py.

NRC#279 (ABSICHT: die Kappen gehen nur fuer bestimmte Patch-Groessen): NRC
schreibt nur die englische Konsolenzeile "cannot create cylinder-cap - neither
patch width, nor height is 9". Unter der unveraenderten englischen Zeile steht
ein Hinweis in der Programmsprache (die Begriffe wie im Kappen-Dialog).
"""

# (Kennung, English, Deutsch, 中文, 日本語)
KARTE6 = [
    # --- NRC#279: Kappen-Hinweise ------------------------------------------------
    ("KeCapHintBevel",
     "Bevel caps need a patch that is 3 or 5 control points wide or high.",
     "Eine Fase braucht einen Patch, der 3 oder 5 Kontrollpunkte breit oder hoch ist.",
     "斜面封盖需要宽度或高度为 3 或 5 个控制点的面片。",
     "ベベルには幅または高さが制御点 3 つか 5 つのパッチが必要です。"),
    ("KeCapHintEndcap",
     "Endcaps need a patch that is 5 control points wide or high.",
     "Eine Endkappe braucht einen Patch, der 5 Kontrollpunkte breit oder hoch ist.",
     "端盖需要宽度或高度为 5 个控制点的面片。",
     "エンドキャップには幅または高さが制御点 5 つのパッチが必要です。"),
    ("KeCapHintCylinder",
     "Cylinder caps need a patch that is 9 control points wide or high, such as Patch > Cylinder.",
     "Eine Zylinderkappe braucht einen Patch, der 9 Kontrollpunkte breit oder hoch ist, etwa Patch > Zylinder.",
     "圆柱封盖需要宽度或高度为 9 个控制点的面片,例如 Patch > 圆柱。",
     "円柱キャップには幅または高さが制御点 9 つのパッチ(例: Patch > 円柱)が必要です。"),
]
