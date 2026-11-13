"""AP14 (12.11., Entscheidung B6): ein Absatz fuer die Seite modell-ansehen - bei Mindestgroesse stehen die Reiter des Befehlsfelds
wie 3ds Max nur als Symbole in einer Reihe, der Name steht im Tooltip (wie Blenders Eigenschaften-Reiter).

`bloecke()`: hinter den Absatz "feld" (inhalt_ap3.py) der Seite modell-ansehen; eingehaengt im Register
tools/handbuch/inhalt_modell.py (Zeile `Hinter("inhalt_ap14", "bloecke", "modell-ansehen", "inhalt_ap3", "feld")`).
Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs); {{Kennung}} setzt den Text der Oberflaeche ein.
"""

X = {
    "symbole": (
        "In a small window (below about 50 lines of text in height, for example the smallest size of the program) the tabs of the "
        "**{{MbBefehlsfeld}}** need two or three rows as text. There they stand **{{MbWieMax}}** as symbols in one row, like the "
        "column of **{{MfEigenschaften}}** **{{MbWieBlender}}**; the name of a tab stands in its tooltip. The panel keeps the room "
        "for its groups.",
        "In einem kleinen Fenster (niedriger als etwa 50 Textzeilen, zum Beispiel bei der kleinsten Größe des Programms) brauchen "
        "die Reiter des **{{MbBefehlsfeld}}** als Text zwei oder drei Reihen. Dort stehen sie **{{MbWieMax}}** als Symbole in einer "
        "Reihe, wie die Spalte der **{{MfEigenschaften}}** **{{MbWieBlender}}**; der Name eines Reiters steht in seinem Tooltip. Das "
        "Feld behält so den Platz für seine Gruppen.",
        "在小窗口中 (高度低于约 50 行文字, 例如程序的最小尺寸), **{{MbBefehlsfeld}}** 的选项卡以文字显示需要两到三行。"
        "此时 **{{MbWieMax}}** 选项卡只以图标排成一行, 与 **{{MbWieBlender}}** 时 **{{MfEigenschaften}}** 的图标列相同; "
        "选项卡的名称显示在其工具提示中。面板因此为分组保留空间。",
        "小さいウィンドウ (高さが約 50 行より低いとき、たとえばプログラムの最小サイズ) では、**{{MbBefehlsfeld}}** の"
        "タブを文字で並べると 2 段か 3 段になります。そのため **{{MbWieMax}}** ではタブをアイコンだけで 1 列に並べます。"
        "**{{MbWieBlender}}** の **{{MfEigenschaften}}** の列と同じで、タブの名前はツールチップに表示されます。"
        "パネルはグループのための場所を保てます。",
        "En una ventana pequeña (de menos de unas 50 líneas de texto de alto, por ejemplo con el tamaño más pequeño del programa) "
        "las pestañas del **{{MbBefehlsfeld}}** necesitan dos o tres filas como texto. Allí aparecen **{{MbWieMax}}** como símbolos "
        "en una sola fila, como la columna de **{{MfEigenschaften}}** **{{MbWieBlender}}**; el nombre de cada pestaña está en su "
        "información emergente. Así el panel conserva el espacio para sus grupos.",
        "V malém okně (nižším než asi 50 řádků textu, například při nejmenší velikosti programu) potřebují karty "
        "**{{MbBefehlsfeld}}** jako text dvě až tři řady. Tam stojí **{{MbWieMax}}** jako symboly v jedné řadě, jako sloupec "
        "**{{MfEigenschaften}}** **{{MbWieBlender}}**; název karty je v její bublinové nápovědě. Panel si tak ponechá místo pro "
        "své skupiny."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    z = X[schluessel]
    return T(z[0], z[1], z[2], z[3])


def bloecke():
    """Hinter ("p", _t("feld")) von inhalt_ap3 auf der Seite modell-ansehen."""
    return [("p", _t("symbole"))]
