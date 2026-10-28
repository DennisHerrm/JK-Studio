# -*- coding: utf-8 -*-
"""Oberflaechentexte des Karten-Editors, Stufe 9c, fuer tools/gen_i18n.py.

Werkzeugleisten verschiebbar wie NRCs QToolBars; Patch-Schatten (q3map2 -patchshadows)
im Echtzeit-Licht und beim Schnell-Backen.
"""

# (Kennung, English, Deutsch, 中文, 日本語)
KARTE9C = [
    ('KeLeisteGriffTipp',
     "Drag the handle to dock the toolbar at another edge (top, left, right, bottom). Right-click a toolbar to show or hide buttons: Open, Save, Undo, Redo and Select Touching are hidden by default, as in NetRadiant-Custom.",
     "Am Griff ziehen legt die Leiste an einen anderen Rand (oben, links, rechts, unten). Rechtsklick auf eine Leiste blendet Knöpfe ein oder aus: Öffnen, Speichern, Rückgängig, Wiederholen und Select Touching sind ab Werk versteckt, wie in NetRadiant-Custom.",
     "拖动把手可将工具栏停靠到另一边(上、左、右、下)。右键单击工具栏可显示或隐藏按钮:打开、保存、撤销、重做和 Select Touching 默认隐藏,与 NetRadiant-Custom 相同。",
     "ハンドルをドラッグするとツールバーを別の辺(上・左・右・下)にドッキングできます。ツールバーを右クリックするとボタンの表示/非表示を切り替えられます。開く・保存・元に戻す・やり直し・Select Touching は NetRadiant-Custom と同じく初期状態で非表示です。"),
    ('KePatchSchatten',
     "Patch shadows (-patchshadows)",
     "Patch-Schatten (-patchshadows)",
     "面片阴影(-patchshadows)",
     "パッチの影(-patchshadows)"),
    ('KePatchSchattenTip',
     "Patches cast shadows in the real-time light and in the quick bake (q3map2 -patchshadows). Off by default, like q3map2.",
     "Patches werfen Schatten im Echtzeit-Licht und beim Schnell-Backen (q3map2 -patchshadows). Ab Werk aus, wie in q3map2.",
     "面片在实时光照和快速烘焙中投射阴影(q3map2 -patchshadows)。默认关闭,与 q3map2 相同。",
     "パッチがリアルタイムライトとクイックベイクで影を落とします(q3map2 -patchshadows)。q3map2 と同じく初期状態はオフです。"),
]
