# -*- coding: utf-8 -*-
"""Oberflaechentexte von NUMPAD (rc612: der Ziffernblock setzt im Karten-Editor das Raster, Schalter wie NetRadiant-Custom).

Preferences > Grid und Menue Grid: "Numpad sets the grid" (gui/app_karte_dialoge.cpp gridSeite, gui/app_karte_menue.cpp
listeZeichnen "ProgrammGrid"); Edit > Shortcuts: der Hinweis an einer geaenderten Taste. Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("KeZbRaster", "Numpad sets the grid", "Ziffernblock setzt das Raster", "数字小键盘设置网格", "テンキーでグリッドを設定",
     "El teclado numérico ajusta la rejilla", "Numerická klávesnice nastavuje mřížku"),
    # Der Hinweis in kurzen Zeilen: in einer Zeile je Satz war er 1277 Punkte breit und ragte in Preferences > Grid ueber den
    # rechten Rand des Programms (Selbsttest numpad, 10.11.)
    ("KeZbRasterTip",
     "On (default): the numpad does what the number row does -\nNum 1 to Num 9 set the grid 1 to 256, Num 0 shows or hides "
     "the grid.\nOff: the numpad works as in NetRadiant-Custom -\nNum 1 front view (XZ), Num 3 side view (YZ), Num 7 top "
     "view (XY).\nThe views are always on Ctrl+Tab and in View > Orthographic.",
     "An (Vorgabe): der Ziffernblock tut, was die Ziffernreihe tut -\nNum 1 bis Num 9 setzen das Raster 1 bis 256, Num 0 "
     "blendet das Raster ein oder aus.\nAus: der Ziffernblock wirkt wie in NetRadiant-Custom -\nNum 1 Vorderansicht (XZ), "
     "Num 3 Seitenansicht (YZ), Num 7 Draufsicht (XY).\nDie Ansichten liegen immer auf Strg+Tab und unter Ansicht > 2D-Ansicht.",
     "开(默认):数字小键盘与数字行相同 -\nNum 1 到 Num 9 设置网格 1 到 256,Num 0 显示或隐藏网格。\n关:数字小键盘与 "
     "NetRadiant-Custom 相同 -\nNum 1 前视图 (XZ),Num 3 侧视图 (YZ),Num 7 顶视图 (XY)。\n视图始终可用 Ctrl+Tab 和 "
     "视图 > 正交视图 切换。",
     "オン(既定):テンキーは数字キーの列と同じ -\nNum 1 から Num 9 でグリッド 1 から 256、Num 0 でグリッドの表示/非表示。\n"
     "オフ:テンキーは NetRadiant-Custom と同じ -\nNum 1 前面ビュー (XZ)、Num 3 側面ビュー (YZ)、Num 7 上面ビュー (XY)。\n"
     "ビューはいつでも Ctrl+Tab と 表示 > 平行投影 で切り替えられます。",
     "Activado (predeterminado): el teclado numérico hace lo mismo que la fila de números:\nNum 1 a Num 9 ajustan la rejilla "
     "de 1 a 256, Num 0 muestra u oculta la rejilla.\nDesactivado: el teclado numérico funciona como en NetRadiant-Custom:\n"
     "Num 1 vista frontal (XZ), Num 3 vista lateral (YZ), Num 7 vista superior (XY).\nLas vistas siempre están en Ctrl+Tab y "
     "en View > Orthographic.",
     "Zapnuto (výchozí): numerická klávesnice dělá totéž co řada číslic -\nNum 1 až Num 9 nastavují mřížku 1 až 256, Num 0 "
     "zobrazí nebo skryje mřížku.\nVypnuto: numerická klávesnice funguje jako v NetRadiant-Custom -\nNum 1 přední pohled (XZ), "
     "Num 3 boční pohled (YZ), Num 7 pohled shora (XY).\nPohledy jsou vždy na Ctrl+Tab a ve View > Orthographic."),
    # Edit > Shortcuts: Hinweis an der Markierung "(x)", wenn die Vorgabe des Befehls vom Ziffernblock kommt (sonst KeKeyChanged)
    ("KeZbKeyChanged", "Differs from the default (the numpad sets the grid)", "Weicht von der Vorgabe ab (der Ziffernblock setzt das Raster)",
     "与默认值不同(数字小键盘设置网格)", "既定値と異なります(テンキーでグリッドを設定)",
     "Distinto del predeterminado (el teclado numérico ajusta la rejilla)", "Liší se od výchozího nastavení (numerická klávesnice nastavuje mřížku)"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
