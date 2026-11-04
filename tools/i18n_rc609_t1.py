# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc609 T1 (ganze Tabs in eigenen Fenstern) fuer
tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("TfLoesen", "Open in own window", "In eigenem Fenster öffnen", "在单独窗口中打开", "別ウィンドウで開く",
     "Abrir en ventana propia", "Otevřít ve vlastním okně"),
    ("TfZurueck", "Back to the main window", "Zurück ins Hauptfenster", "返回主窗口", "メインウィンドウに戻す",
     "Volver a la ventana principal", "Zpět do hlavního okna"),
    ("TfLetzter", "The main window keeps at least one tab.", "Das Hauptfenster behält mindestens einen Tab.",
     "主窗口至少保留一个标签页。", "メインウィンドウには少なくとも 1 つのタブが残ります。",
     "La ventana principal conserva al menos una pestaña.", "Hlavní okno si ponechá alespoň jednu kartu."),
    ("TfZiehHint", "Release here to open the tab in its own window", "Hier loslassen: der Tab öffnet sich in einem eigenen Fenster",
     "在此松开:该标签页将在单独窗口中打开", "ここで離すとタブが別ウィンドウで開きます",
     "Suelta aquí para abrir la pestaña en su propia ventana", "Pusťte zde a karta se otevře ve vlastním okně"),
    ("TfEigenesHint", "This tab is in its own window. Click to bring it to the front; right-click to bring it back.",
     "Dieser Tab steht in einem eigenen Fenster. Klick holt es nach vorn, Rechtsklick holt den Tab zurück.",
     "此标签页位于单独的窗口中。单击将其置于前面,右键单击将其取回。",
     "このタブは別ウィンドウにあります。クリックで前面に、右クリックで元に戻せます。",
     "Esta pestaña está en su propia ventana. Haz clic para traerla al frente; clic derecho para devolverla.",
     "Tato karta je ve vlastním okně. Kliknutím ho přenesete dopředu, pravým tlačítkem kartu vrátíte."),
    ("TfMenuEigenes", "This tab in its own window", "Diesen Tab in eigenem Fenster", "此标签页使用单独窗口",
     "このタブを別ウィンドウに", "Esta pestaña en ventana propia", "Tuto kartu ve vlastním okně"),
    ("TfMenuEigenesHint",
     "Opens the whole tab as a separate window you can move to a second screen. Both tabs work at the same time. Close the window or drag its title bar onto the mode bar to bring the tab back.",
     "Öffnet den ganzen Tab als eigenes Fenster, das du auf einen zweiten Bildschirm ziehen kannst. Beide Tabs arbeiten gleichzeitig. Schließe das Fenster oder zieh seine Titelleiste auf die Modusleiste, um den Tab zurückzuholen.",
     "将整个标签页作为单独窗口打开,可拖到第二个屏幕。两个标签页可同时使用。关闭该窗口或将其标题栏拖到模式栏即可取回标签页。",
     "タブ全体を別ウィンドウとして開き、2 台目の画面へ移動できます。両方のタブを同時に使えます。ウィンドウを閉じるか、タイトルバーをモードバーにドラッグするとタブが戻ります。",
     "Abre toda la pestaña como ventana aparte que puedes llevar a una segunda pantalla. Ambas pestañas funcionan a la vez. Cierra la ventana o arrastra su barra de título a la barra de modos para devolver la pestaña.",
     "Otevře celou kartu jako samostatné okno, které můžete přesunout na druhou obrazovku. Obě karty fungují zároveň. Kartu vrátíte zavřením okna nebo přetažením jeho titulku na lištu režimů."),
    ("TfMenuAlleZurueck", "All tabs back to the main window", "Alle Tabs zurück ins Hauptfenster", "所有标签页返回主窗口",
     "すべてのタブをメインウィンドウに戻す", "Todas las pestañas a la ventana principal", "Všechny karty zpět do hlavního okna"),
]


def zeilen_vier():
    """(Kennung, English, Deutsch, 中文, 日本語) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
