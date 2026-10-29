# -*- coding: utf-8 -*-
"""Oberflaechentexte des Zweigs dock1 (Fenster frei anordnen, mehrere Bildschirme), fuer tools/gen_i18n.py.

Spanisch und Tschechisch fallen vorerst auf Englisch zurueck (tools/sprachen.py);
eine spaetere Runde uebersetzt sie.
"""

# (Kennung, English, Deutsch, 中文, 日本語)
DOCK1 = [
    ("DockMenu", "Window", "Fenster", "窗口", "ウィンドウ"),
    ("DockPanels", "Show panels", "Panels anzeigen", "显示面板", "パネルを表示"),
    ("DockOwnWindow", "In its own window", "In eigenem Fenster", "在独立窗口中", "独立したウィンドウで"),
    ("DockOwnWindowHint",
     "Take the panel out of the main window. Drag its window anywhere - onto a second screen, or onto another panel "
     "window to combine them as tabs or side by side. Close it or drag it back onto its old place to dock it again.",
     "Das Panel aus dem Hauptfenster nehmen. Sein Fenster lässt sich beliebig ziehen - auf einen zweiten Bildschirm "
     "oder auf ein anderes Panelfenster, um beide als Reiter oder nebeneinander zu verbinden. Schließen oder auf den "
     "alten Platz ziehen dockt es wieder an.",
     "将面板移出主窗口。其窗口可随意拖动——拖到第二个屏幕上,或拖到另一个面板窗口上,以选项卡或并排方式组合。"
     "关闭它或拖回原位置即可重新停靠。",
     "パネルをメインウィンドウから取り出します。そのウィンドウはどこへでもドラッグできます。2 台目の画面へ、"
     "または別のパネルウィンドウへドラッグするとタブや横並びで組み合わせられます。閉じるか元の場所へドラッグすると再びドッキングします。"),
    ("DockNotInLayout", "Not part of the current map editor layout (Preferences > Layout)",
     "Nicht Teil der aktuellen Anordnung des Karten-Editors (Einstellungen > Layout)",
     "不属于当前地图编辑器布局(首选项 > 布局)",
     "現在のマップエディターのレイアウトにはありません(設定 > レイアウト)"),
    ("DockSaveAs", "Save layout as...", "Anordnung speichern unter...", "布局另存为...", "レイアウトに名前を付けて保存..."),
    ("DockSaveTitle", "Save layout", "Anordnung speichern", "保存布局", "レイアウトを保存"),
    ("DockSaveHint", "Name of the layout (for this area):", "Name der Anordnung (für diesen Bereich):",
     "布局名称(用于此区域):", "レイアウトの名前(この領域用):"),
    ("DockSave", "Save", "Speichern", "保存", "保存"),
    ("DockLoad", "Load layout", "Anordnung laden", "加载布局", "レイアウトを読み込む"),
    ("DockDelete", "Delete layout", "Anordnung löschen", "删除布局", "レイアウトを削除"),
    ("DockReset", "Reset layout", "Anordnung zurücksetzen", "重置布局", "レイアウトをリセット"),
    ("DockResetHint", "Every panel of this area back to its place in the main window, all shown - the standard layout.",
     "Jedes Panel dieses Bereichs zurück an seinen Platz im Hauptfenster, alle sichtbar - die Standardanordnung.",
     "此区域的每个面板都回到主窗口中的原位置,全部显示——即标准布局。",
     "この領域のすべてのパネルをメインウィンドウの元の場所に戻し、すべて表示します(標準レイアウト)。"),
    ("DockResetDone", "Layout reset", "Anordnung zurückgesetzt", "布局已重置", "レイアウトをリセットしました"),
    ("DockSaved", "Layout saved: %s", "Anordnung gespeichert: %s", "布局已保存:%s", "レイアウトを保存しました: %s"),
    ("DockLoaded", "Layout loaded: %s", "Anordnung geladen: %s", "布局已加载:%s", "レイアウトを読み込みました: %s"),
    ("DockDropHere", "Release to dock back here", "Loslassen zum Andocken", "松开以停靠到此处", "ここで離すとドッキングします"),
    ("DockOther", "Other windows", "Weitere Fenster", "其他窗口", "その他のウィンドウ"),
    ("DockPanelMapView", "Mission view", "Missionsansicht", "任务视图", "ミッションビュー"),
    ("DockPanelModelView", "Model view", "Modellansicht", "模型视图", "モデルビュー"),
    ("DockPanelTimeline", "Timeline", "Zeitleiste", "时间轴", "タイムライン"),
    ("DockPanelModelBar", "Playback bar", "Abspielleiste", "播放栏", "再生バー"),
    ("DockOutside", "%s is in its own window.", "%s steht in einem eigenen Fenster.", "%s 位于独立窗口中。",
     "%s は独立したウィンドウにあります。"),
    ("DockBack", "Dock back", "Andocken", "重新停靠", "ドッキングし直す"),
]
