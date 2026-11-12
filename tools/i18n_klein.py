# -*- coding: utf-8 -*-
"""Oberflaechentexte von KLEIN (rc612, Welle 12.11., Kleinkram): der Tooltip von "Aktuelle Ansicht als Home setzen" (das gesetzte
Home bleibt jetzt je Figur, auch ueber den Neustart) und die zweite Taste im Dialog Edit > Shortcuts des Karten-Editors - fuer
tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    # --- Model-Tab: ViewCube, Rechtsklick "Aktuelle Ansicht als Home setzen" (auch in "Alle Aktionen suchen") ---
    ("MkHomeSetzenHint",
     "Home flies to this view from now on. JK Studio remembers it for this figure, also after a restart (right-click menu of the ViewCube).",
     "Home fliegt ab jetzt zu dieser Ansicht. JK Studio merkt sie sich für diese Figur, auch über den Neustart (Rechtsklick-Menü des ViewCube).",
     "此后 Home 会飞到此视图。JK Studio 会为该角色记住它, 重新启动后仍然有效 (ViewCube 的右键菜单)。",
     "これ以降、ホームはこのビューへ移動します。JK Studio はこのフィギュア用に記憶し、再起動後も保持します (ViewCube の右クリックメニュー)。",
     "A partir de ahora Inicio vuela a esta vista. JK Studio la recuerda para esta figura, también tras reiniciar (menú contextual del ViewCube).",
     "Domů od teď přeletí do tohoto pohledu. JK Studio si ho pamatuje pro tuto postavu, i po restartu (kontextová nabídka ViewCube)."),
    ("MkHomeZurueck", "Reset Home", "Home zurücksetzen", "重置 Home", "ホームをリセット", "Restablecer Inicio", "Obnovit Domů"),
    ("MkHomeZurueckHint",
     "Home is the start view again (figure from the front, fitted). JK Studio forgets the view set as Home for this figure.",
     "Home ist wieder die Startansicht (Figur von vorne, eingepasst). JK Studio vergisst die als Home gesetzte Ansicht dieser Figur.",
     "Home 再次成为起始视图 (角色正面, 适配)。JK Studio 会忘记为该角色设为 Home 的视图。",
     "ホームは再び開始ビュー (フィギュアを正面から、フィット) になります。JK Studio はこのフィギュアにホームとして設定したビューを忘れます。",
     "Inicio vuelve a ser la vista inicial (figura de frente, ajustada). JK Studio olvida la vista establecida como Inicio para esta figura.",
     "Domů je opět výchozí pohled (postava zepředu, přizpůsobená). JK Studio zapomene pohled nastavený jako Domů pro tuto postavu."),
    # --- Karten-Editor: Edit > Shortcuts, die zweite Taste eines Befehls einzeln belegen (bis rc612 nur ueber Yes, Clear, Reset) ---
    ("KeEditZweit", "Edit 2nd", "2. Taste", "第二键", "2 つ目", "2.ª tecla", "2. klávesa"),
    ("KeEditZweitHint",
     "Sets the second key of the selected command - by default the numpad key of the grid commands. The first button of this column sets the first key.",
     "Belegt die zweite Taste des gewählten Befehls - ab Werk die Ziffernblock-Taste der Raster-Befehle. Der erste Knopf dieser Spalte belegt die erste Taste.",
     "设置所选命令的第二个按键——默认是网格命令的小键盘按键。本列的第一个按钮设置第一个按键。",
     "選択したコマンドの 2 つ目のキーを割り当てます - 既定ではグリッドコマンドのテンキーです。この列の最初のボタンは 1 つ目のキーを割り当てます。",
     "Asigna la segunda tecla de la orden seleccionada: de fábrica, la tecla del teclado numérico de las órdenes de rejilla. El primer botón de esta columna asigna la primera tecla.",
     "Nastaví druhou klávesu vybraného příkazu - z výroby klávesu numerického bloku u příkazů mřížky. První tlačítko tohoto sloupce nastaví první klávesu."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
