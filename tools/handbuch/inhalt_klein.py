"""KLEIN (rc612): der Absatz "Zweite Taste" im Abschnitt Tastenkuerzel der Seite Karten-Editor - die zweite Taste eines Befehls
laesst sich in Edit > Shortcuts einzeln belegen (Knopf "Edit 2nd").

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_d3d12.py. Die uebrigen Aenderungen von KLEIN am
Handbuch stehen in inhalt_mbkopf.py (ein gesetztes Home bleibt bei der Figur; der Kamera-Knopf des Navigations-Gizmos).
"""
from inhalt_basis import T

ZWEITE = (
    "**Second key.** A command can have two keys - the grid commands have the number row and the numpad. In "
    "**{{KeLEdit}} > {{KeLShortcuts}}** the button **{{KeEdit}}** sets the first key of the selected command and **{{KeEditZweit}}** "
    "the second one; if another command already has the key, the dialog asks as usual. **{{KeClear}}** empties both keys, "
    "**{{KeReset}}** brings both defaults back. A right click on a row offers the same buttons for that command.",
    "**Zweite Taste.** Ein Befehl kann zwei Tasten haben - die Raster-Befehle haben die Ziffernreihe und den Ziffernblock. In "
    "**{{KeLEdit}} > {{KeLShortcuts}}** belegt der Knopf **{{KeEdit}}** die erste Taste des gewählten Befehls und **{{KeEditZweit}}** "
    "die zweite; hat ein anderer Befehl die Taste schon, fragt der Dialog wie gewohnt. **{{KeClear}}** leert beide Tasten, "
    "**{{KeReset}}** holt beide Vorgaben zurück. Ein Rechtsklick auf eine Zeile bietet dieselben Knöpfe für diesen Befehl.",
    "**第二个按键。** 一个命令可以有两个按键——网格命令有数字行和小键盘。在 **{{KeLEdit}} > {{KeLShortcuts}}** 中,按钮 **{{KeEdit}}** "
    "设置所选命令的第一个按键,**{{KeEditZweit}}** 设置第二个;如果该按键已属于另一个命令,对话框会照常询问。**{{KeClear}}** 清空两个按键,"
    "**{{KeReset}}** 恢复两个默认值。右键单击某一行可为该命令提供相同的按钮。",
    "**2 つ目のキー。** コマンドにはキーを 2 つ割り当てられます。グリッドのコマンドには数字キーの列とテンキーが割り当てられています。"
    "**{{KeLEdit}} > {{KeLShortcuts}}** で、ボタン **{{KeEdit}}** は選択したコマンドの 1 つ目のキーを、**{{KeEditZweit}}** は 2 つ目のキーを"
    "割り当てます。そのキーを別のコマンドが使っている場合は、いつものように確認されます。**{{KeClear}}** は両方のキーを空にし、"
    "**{{KeReset}}** は両方を既定に戻します。行を右クリックすると、そのコマンドに同じボタンが表示されます。",
    "**Segunda tecla.** Una orden puede tener dos teclas: las órdenes de rejilla tienen la fila de números y el teclado numérico. En "
    "**{{KeLEdit}} > {{KeLShortcuts}}**, el botón **{{KeEdit}}** asigna la primera tecla de la orden seleccionada y **{{KeEditZweit}}** "
    "la segunda; si otra orden ya tiene la tecla, el diálogo pregunta como de costumbre. **{{KeClear}}** vacía ambas teclas y "
    "**{{KeReset}}** recupera los dos valores de fábrica. Un clic derecho en una fila ofrece los mismos botones para esa orden.",
    "**Druhá klávesa.** Příkaz může mít dvě klávesy - příkazy mřížky mají řadu číslic a numerický blok. V "
    "**{{KeLEdit}} > {{KeLShortcuts}}** nastaví tlačítko **{{KeEdit}}** první klávesu vybraného příkazu a **{{KeEditZweit}}** druhou; "
    "má-li klávesu už jiný příkaz, dialog se zeptá jako obvykle. **{{KeClear}}** vymaže obě klávesy, **{{KeReset}}** vrátí obě "
    "výchozí. Pravé tlačítko na řádku nabízí stejná tlačítka pro tento příkaz.",
)


def zweite_taste():
    """Der Absatz fuer inhalt_karteneditor.py (Abschnitt Tastenkuerzel, nach dem Absatz Ziffernblock)."""
    return [("p", T(*ZWEITE[:4]))]


ES = {ZWEITE[0]: ZWEITE[4]}
CS = {ZWEITE[0]: ZWEITE[5]}
