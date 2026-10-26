# -*- coding: utf-8 -*-
"""Oberflaechentexte des Karten-Editors, Stufe 7 (Tempo und Echtzeit-Licht), fuer
tools/gen_i18n.py.

Echtzeit-Licht und Schnell-Backen sind eigene Funktionen (nicht in
NetRadiant-Custom): eigene Eintraege im Build-Menue mit Tooltip, ein Hinweis
in der Kamera-Ecke. Mit "Begriffe wie NetRadiant" bleiben sie englisch.
"""

# (Kennung, English, Deutsch, 中文, 日本語)
KARTE7 = [
    ("KeLichtMenue", "Real-time light", "Echtzeit-Licht", "实时光照", "リアルタイムライト"),
    ("KeLichtMenueTip",
     "Light the camera view on the graphics card with q3map2's formulas for point lights, spotlights, the sun and surface lights, with shadows. A preview: no bounce, no filter - the final light comes from the build (not in NetRadiant-Custom)",
     "Die Kamera auf der Grafikkarte beleuchten, mit den Formeln von q3map2 für Punktlichter, Spotlichter, die Sonne und Flächenlicht, mit Schatten. Eine Vorschau: ohne Bounce und Filter - das endgültige Licht kommt aus dem Bau (nicht in NetRadiant-Custom)",
     "在显卡上用 q3map2 的点光源、聚光灯、太阳和面光源公式照亮摄像机视图,并带阴影。仅为预览:没有反弹和过滤——最终光照来自编译(NetRadiant-Custom 中没有)",
     "グラフィックカードで、q3map2 のポイントライト・スポットライト・太陽・面光源の式を使いカメラビューを影付きで照らします。プレビューです:バウンスとフィルターはありません。最終的なライトはビルドで作られます(NetRadiant-Custom にはありません)"),
    ("KeBackenMenue", "Quick bake", "Schnell-Backen", "快速烘焙", "クイックベイク"),
    ("KeBackenMenueTip",
     "After a short pause without changes, bhed_q3map2 builds the map quickly (-meta, -vis -fast, -light -fast) in a temporary folder at low priority and the camera shows the real lightmaps; a new change cancels the build. Your map and its .bsp are not touched (not in NetRadiant-Custom)",
     "Nach einer kurzen Pause ohne Änderung baut bhed_q3map2 die Karte schnell (-meta, -vis -fast, -light -fast) in einem temporären Ordner mit niedriger Priorität, und die Kamera zeigt die echten Lightmaps; eine neue Änderung bricht den Bau ab. Ihre Karte und ihre .bsp bleiben unberührt (nicht in NetRadiant-Custom)",
     "在一段时间没有改动后,bhed_q3map2 以低优先级在临时文件夹中快速编译地图(-meta、-vis -fast、-light -fast),摄像机显示真实的光照贴图;新的改动会取消编译。您的地图和其 .bsp 不会被改动(NetRadiant-Custom 中没有)",
     "変更のない短い間が空くと、bhed_q3map2 が一時フォルダーで低優先度のままマップを素早くビルドし(-meta、-vis -fast、-light -fast)、カメラが本物のライトマップを表示します。新しい変更でビルドは中止されます。マップとその .bsp には触れません(NetRadiant-Custom にはありません)"),
    # Punkt 6b: Menuestil und Knopfgroesse (eigene Einstellungen)
    ("KeKnopfGroesse", "Toolbar Button Size", "Knopfgröße der Werkzeugleisten", "工具栏按钮大小", "ツールバーのボタンサイズ"),
    ("KeKnopfKlein", "Small", "Klein", "小", "小"),
    ("KeKnopfNormal", "Normal (like NetRadiant-Custom)", "Normal (wie NetRadiant-Custom)", "正常(与 NetRadiant-Custom 相同)",
     "標準(NetRadiant-Custom と同じ)"),
    ("KeKnopfGross", "Large", "Groß", "大", "大"),
    ("KeMenueNrc", "Menus like NetRadiant-Custom", "Menüs wie NetRadiant-Custom", "菜单外观同 NetRadiant-Custom", "メニューを NetRadiant-Custom 風にする"),
    ("KeMenueNrcTip",
     "Draw the menu bar and the menus like NetRadiant-Custom (Qt). Off: in the style of this program - the entries, shortcuts and effects stay the same (not in NetRadiant-Custom)",
     "Menüleiste und Menüs wie NetRadiant-Custom (Qt) zeichnen. Aus: im Stil dieses Programms - Einträge, Tastenkürzel und Wirkung bleiben gleich (nicht in NetRadiant-Custom)",
     "像 NetRadiant-Custom(Qt)那样绘制菜单栏和菜单。关闭时使用本程序的风格——条目、快捷键和功能保持不变(NetRadiant-Custom 中没有)",
     "メニューバーとメニューを NetRadiant-Custom(Qt)のように描画します。オフ:このプログラムのスタイルで表示し、項目・ショートカット・動作は同じです(NetRadiant-Custom にはありません)"),
    ("KeKnopfGroesseTip",
     "Size of the buttons in the main, filter and plugin toolbars; Normal is NetRadiant-Custom's size (icon 24, button 31) (not in NetRadiant-Custom)",
     "Größe der Knöpfe in der Haupt-, Filter- und Plugin-Leiste; Normal ist die Größe von NetRadiant-Custom (Symbol 24, Knopf 31) (nicht in NetRadiant-Custom)",
     "主工具栏、过滤器工具栏和插件工具栏中按钮的大小;“正常”即 NetRadiant-Custom 的大小(图标 24,按钮 31)(NetRadiant-Custom 中没有)",
     "メイン・フィルター・プラグインの各ツールバーのボタンの大きさ。「標準」は NetRadiant-Custom と同じ大きさ(アイコン 24、ボタン 31)です(NetRadiant-Custom にはありません)"),
    ("KeLichtVorschau", "Real-time light: preview (no bounce, no filter)", "Echtzeit-Licht: Vorschau (ohne Bounce und Filter)",
     "实时光照:预览(无反弹、无过滤)", "リアルタイムライト:プレビュー(バウンス・フィルターなし)"),
    ("KeLichtBaeckt", "quick bake running", "Schnell-Backen läuft", "快速烘焙进行中", "クイックベイク実行中"),
    ("KeLichtGebacken", "Quick bake: real lightmaps from bhed_q3map2 (%.1f s)", "Schnell-Backen: echte Lightmaps aus bhed_q3map2 (%.1f s)",
     "快速烘焙:来自 bhed_q3map2 的真实光照贴图(%.1f 秒)", "クイックベイク:bhed_q3map2 による本物のライトマップ(%.1f 秒)"),
]
