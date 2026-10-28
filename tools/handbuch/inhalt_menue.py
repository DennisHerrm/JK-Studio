"""Menue-Editor, Stufe M1: .menu-Dateien von Jedi Academy, Jedi Outcast und
Movie Duels als Text und visuell bearbeiten. Der Selbsttest
gui/selbsttest_menue.inc (BHED_EDITORTEST=menueeditor) prueft jede Funktion.
"""
from inhalt_basis import T

MENUE_EDITOR = {
    "id": "menue_editor",
    "gruppe": "start",
    "titel": T("Menu editor", "Menü-Editor", "菜单编辑器", "メニューエディター"),
    "kurz": T(
        "Edit the game's menus (.menu) as text and on a 640x480 canvas at the same time - read, drawn and operated like in the game.",
        "Die Menüs des Spiels (.menu) gleichzeitig als Text und auf einer 640x480-Fläche bearbeiten - gelesen, gezeichnet und bedient wie im Spiel.",
        "同时以文本和 640x480 画布编辑游戏菜单(.menu)——读取、绘制和操作都与游戏中一致。",
        "ゲームのメニュー(.menu)をテキストと 640x480 のキャンバスで同時に編集します。読み込み・描画・操作はゲームと同じです。"),
    "bloecke": [
        ("h2", "oeffnen", T("Open a menu", "Ein Menü öffnen", "打开菜单", "メニューを開く")),
        ("p", T(
            "Click **{{ViewMenuEditor}}** in the mode bar. **{{MeAusSpiel}}** lists every `ui/` menu of the game set up in the preferences (read from the .pk3, never changed); **{{MeDateiOeffnen}}** or dropping a file opens one from disk. The path decides how the file is read, exactly like the game: `ui/jamp/`, `ui/jk2mp/` and `ui/MD_MP/` with the multiplayer precompiler (#include, #define, macros, menudef.h), everything else like single player.",
            "Klicke **{{ViewMenuEditor}}** in der Modusleiste. **{{MeAusSpiel}}** listet jedes `ui/`-Menü des eingerichteten Spiels (aus den .pk3 gelesen, nie verändert); **{{MeDateiOeffnen}}** oder eine abgelegte Datei öffnet eines von der Platte. Der Pfad bestimmt, wie die Datei gelesen wird, genau wie im Spiel: `ui/jamp/`, `ui/jk2mp/` und `ui/MD_MP/` mit dem Mehrspieler-Präprozessor (#include, #define, Makros, menudef.h), alles andere wie im Einzelspieler.",
            "在模式栏中单击 **{{ViewMenuEditor}}**。**{{MeAusSpiel}}** 列出首选项中设置的游戏的所有 `ui/` 菜单(从 .pk3 读取,绝不修改);**{{MeDateiOeffnen}}** 或拖放文件可从磁盘打开。路径决定读取方式,与游戏完全一致:`ui/jamp/`、`ui/jk2mp/` 和 `ui/MD_MP/` 使用多人模式预处理器(#include、#define、宏、menudef.h),其余按单人模式读取。",
            "モードバーで **{{ViewMenuEditor}}** をクリックします。**{{MeAusSpiel}}** は設定したゲームの `ui/` メニューをすべて一覧にします(.pk3 から読み込むだけで変更しません)。**{{MeDateiOeffnen}}** かファイルのドロップでディスクから開きます。読み方はパスで決まり、ゲームと同じです:`ui/jamp/`、`ui/jk2mp/`、`ui/MD_MP/` はマルチプレイのプリプロセッサー(#include、#define、マクロ、menudef.h)、それ以外はシングルプレイと同じです。")),
        ("p", T(
            "**{{MeMeldungen}}** shows what the game would print while loading the file, word for word and with the line (for example `Unknown item keyword 'foo' Line #21`). A click jumps to the line; the code editor marks it red.",
            "**{{MeMeldungen}}** zeigt, was das Spiel beim Laden der Datei ausgeben würde, im Wortlaut und mit Zeile (etwa `Unknown item keyword 'foo' Line #21`). Ein Klick springt zur Zeile; der Code-Editor markiert sie rot.",
            "**{{MeMeldungen}}** 逐字显示游戏加载该文件时会输出的内容及行号(例如 `Unknown item keyword 'foo' Line #21`)。单击跳转到该行;代码编辑器将其标为红色。",
            "**{{MeMeldungen}}** には、ゲームがファイルを読み込むときに出す内容が原文のまま行番号つきで表示されます(例:`Unknown item keyword 'foo' Line #21`)。クリックするとその行へ移動し、コードエディターで赤く表示されます。")),
        ("h2", "flaeche", T("Canvas and pages", "Fläche und Seiten", "画布与页面", "キャンバスとページ")),
        ("p", T(
            "Every `menuDef` of the file is a tab above the canvas; items that are shown by a cvar with several values (`cvarTest` + `showCvar`, like `ui_tab` pages) get a tab per value. **+** adds a new `menuDef`. The canvas is the game's virtual 640x480 screen: **{{MeStrecken}}** like the original game, **{{MeBalken}}**, and preset resolutions to check the aspect ratio.",
            "Jeder `menuDef` der Datei ist ein Reiter über der Fläche; Items, die ein Cvar mit mehreren Werten zeigt (`cvarTest` + `showCvar`, wie `ui_tab`-Seiten), bekommen je Wert einen Reiter. **+** fügt einen neuen `menuDef` an. Die Fläche ist der virtuelle 640x480-Bildschirm des Spiels: **{{MeStrecken}}** wie das Originalspiel, **{{MeBalken}}** und feste Auflösungen, um das Seitenverhältnis zu prüfen.",
            "文件中的每个 `menuDef` 都是画布上方的一个标签页;由具有多个值的 cvar 显示的项(`cvarTest` + `showCvar`,如 `ui_tab` 页面)每个值各有一个标签页。**+** 添加新的 `menuDef`。画布是游戏的虚拟 640x480 屏幕:**{{MeStrecken}}** 与原版游戏相同,**{{MeBalken}}**,以及用于检查宽高比的预设分辨率。",
            "ファイルの `menuDef` はそれぞれキャンバスの上のタブになります。複数の値を持つ cvar で表示される項目(`cvarTest` + `showCvar`、`ui_tab` のようなページ)は値ごとにタブになります。**+** で新しい `menuDef` を追加します。キャンバスはゲームの仮想 640x480 画面です:元のゲームと同じ **{{MeStrecken}}**、**{{MeBalken}}**、縦横比を確かめるための解像度プリセットがあります。")),
        ("list", [
            T("Click selects (Shift/Ctrl adds), dragging moves, the white handles resize. Only the numbers of `rect` change in the text - comments and indentation stay.",
              "Klick wählt (Umschalt/Strg fügt hinzu), Ziehen verschiebt, die weißen Griffe ändern die Größe. Im Text ändern sich nur die Zahlen von `rect` - Kommentare und Einrückung bleiben.",
              "单击选择(Shift/Ctrl 追加),拖动移动,白色手柄调整大小。文本中只改变 `rect` 的数字——注释和缩进保持不变。",
              "クリックで選択(Shift/Ctrl で追加)、ドラッグで移動、白いハンドルでサイズ変更します。テキストで変わるのは `rect` の数値だけで、コメントやインデントはそのままです。"),
            T("**{{MeAnKanten}}** snaps to edges and centres of the other items, the screen and your guides (drag them out of the rulers); **{{MeAnRaster}}** to the grid. Alt switches snapping off while dragging, Shift keeps the direction. Arrow keys move by 1, with Shift by 10.",
              "**{{MeAnKanten}}** rastet an Kanten und Mitten der anderen Items, des Bildschirms und deiner Hilfslinien ein (aus den Linealen ziehen); **{{MeAnRaster}}** am Raster. Alt schaltet das Einrasten beim Ziehen ab, Umschalt hält die Richtung. Pfeiltasten verschieben um 1, mit Umschalt um 10.",
              "**{{MeAnKanten}}** 吸附到其他项、屏幕和参考线(从标尺中拖出)的边缘和中心;**{{MeAnRaster}}** 吸附到网格。拖动时按 Alt 关闭吸附,Shift 保持方向。方向键移动 1,按住 Shift 移动 10。",
              "**{{MeAnKanten}}** はほかの項目・画面・ガイド(ルーラーから引き出す)の端と中央にスナップし、**{{MeAnRaster}}** はグリッドにスナップします。ドラッグ中に Alt でスナップを切り、Shift で方向を固定します。矢印キーで 1、Shift 併用で 10 移動します。"),
            T("**{{MePalette}}** inserts new items (text, button, image, yes/no, multi choice, slider, edit field, list box, model, key bind) at the end of the page's `menuDef`. **{{MeBildDazu}}** takes a PNG, JPG or TGA as `gfx/menus/<name>`; it is packed into the .pk3 on export.",
              "**{{MePalette}}** fügt neue Items ein (Text, Knopf, Bild, Ja/Nein, Mehrfachwahl, Schieber, Eingabefeld, Liste, Modell, Tastenbelegung) - ans Ende des `menuDef` der Seite. **{{MeBildDazu}}** nimmt ein PNG, JPG oder TGA als `gfx/menus/<name>`; beim Export kommt es in die .pk3.",
              "**{{MePalette}}** 在该页 `menuDef` 的末尾插入新项(文本、按钮、图像、是/否、多选、滑块、输入框、列表框、模型、按键绑定)。**{{MeBildDazu}}** 将 PNG、JPG 或 TGA 作为 `gfx/menus/<name>` 加入;导出时打包进 .pk3。",
              "**{{MePalette}}** は新しい項目(テキスト、ボタン、画像、はい/いいえ、複数選択、スライダー、入力欄、リスト、モデル、キー割り当て)をページの `menuDef` の最後に挿入します。**{{MeBildDazu}}** は PNG・JPG・TGA を `gfx/menus/<name>` として取り込み、エクスポート時に .pk3 に入れます。"),
            T("**{{MeEigenschaften}}** edits the selected item; with several items it aligns, distributes and groups them. **{{MeNachVorn}}**/**{{MeNachHinten}}** change the drawing order (the game draws in file order). **{{MeSperren}}** keeps an item from moving.",
              "**{{MeEigenschaften}}** bearbeitet das gewählte Item; bei mehreren richtet es aus, verteilt und gruppiert. **{{MeNachVorn}}**/**{{MeNachHinten}}** ändern die Zeichenreihenfolge (das Spiel zeichnet in Dateireihenfolge). **{{MeSperren}}** hält ein Item fest.",
              "**{{MeEigenschaften}}** 编辑所选项;选中多个时可对齐、分布和编组。**{{MeNachVorn}}**/**{{MeNachHinten}}** 改变绘制顺序(游戏按文件顺序绘制)。**{{MeSperren}}** 防止项被移动。",
              "**{{MeEigenschaften}}** で選んだ項目を編集し、複数なら揃え・均等配置・グループ化ができます。**{{MeNachVorn}}**/**{{MeNachHinten}}** で描画順(ゲームはファイル順に描画)を変えます。**{{MeSperren}}** で項目を動かないようにします。"),
        ]),
        ("h2", "bedienen", T("Interact and cvars", "Bedienen und Cvars", "交互与 Cvar", "操作と Cvar")),
        ("p", T(
            "**{{MeBedienen}}** runs the menu like the game: hovering gives focus (`mouseEnter`, `onFocus`), a click runs the item's `action`, yes/no, multi choice and sliders change their cvar, and `open`, `close`, `show`, `hide`, `setfocus`, `setcvar`, `setitemtext`, `transition` ... work like in ui_shared. What needs the game (sounds, `exec`, `uiScript`) appears in **{{MeProtokoll}}**.",
            "**{{MeBedienen}}** führt das Menü wie das Spiel aus: Darüberfahren gibt den Fokus (`mouseEnter`, `onFocus`), ein Klick führt die `action` des Items aus, Ja/Nein, Mehrfachwahl und Schieber ändern ihren Cvar, und `open`, `close`, `show`, `hide`, `setfocus`, `setcvar`, `setitemtext`, `transition` ... wirken wie in ui_shared. Was das Spiel braucht (Klänge, `exec`, `uiScript`), erscheint im **{{MeProtokoll}}**.",
            "**{{MeBedienen}}** 像游戏一样运行菜单:悬停获得焦点(`mouseEnter`、`onFocus`),单击执行项的 `action`,是/否、多选和滑块改变其 cvar,`open`、`close`、`show`、`hide`、`setfocus`、`setcvar`、`setitemtext`、`transition` 等与 ui_shared 中的行为一致。需要游戏的内容(声音、`exec`、`uiScript`)显示在 **{{MeProtokoll}}** 中。",
            "**{{MeBedienen}}** ではゲームと同じようにメニューが動きます:マウスを重ねるとフォーカス(`mouseEnter`、`onFocus`)、クリックで項目の `action` を実行し、はい/いいえ・複数選択・スライダーは cvar を変え、`open`、`close`、`show`、`hide`、`setfocus`、`setcvar`、`setitemtext`、`transition` などは ui_shared と同じように働きます。ゲームが必要なもの(サウンド、`exec`、`uiScript`)は **{{MeProtokoll}}** に表示されます。")),
        ("p", T(
            "**{{MeCvars}}** lists every cvar the menu reads or sets, the ones that control visibility first, with the values that occur. Change them to see other states. They exist only in the preview - your game configuration is never touched.",
            "**{{MeCvars}}** listet jeden Cvar, den das Menü liest oder setzt, die für die Sichtbarkeit zuerst, mit den vorkommenden Werten. Ändere sie, um andere Zustände zu sehen. Sie gelten nur in der Vorschau - deine Spielkonfiguration wird nie angefasst.",
            "**{{MeCvars}}** 列出菜单读取或设置的所有 cvar,控制可见性的排在前面,并附有出现的值。修改它们可查看其他状态。它们只存在于预览中——绝不会改动你的游戏配置。",
            "**{{MeCvars}}** にはメニューが読む・設定するすべての cvar が、表示を制御するものを先頭に、出てくる値とともに並びます。変えるとほかの状態を確認できます。プレビューの中だけのもので、ゲームの設定には一切触れません。")),
        ("h2", "code", T("Code editor", "Code-Editor", "代码编辑器", "コードエディター")),
        ("keys", [
            ("Ctrl+F / Ctrl+H", T("find / replace (regex, case, whole word, all open menus); matches are marked in the minimap",
                                  "Suchen / Ersetzen (Regex, Groß/Klein, ganzes Wort, alle offenen Menüs); Treffer stehen in der Minimap",
                                  "查找/替换(正则、大小写、全字匹配、所有打开的菜单);匹配项在小地图中标出",
                                  "検索/置換(正規表現、大文字小文字、単語単位、開いているすべてのメニュー)。一致箇所はミニマップに表示")),
            ("Ctrl+D / Ctrl+Shift+L / Alt+Click", T("next occurrence / all occurrences / extra cursor (multi-cursor)",
                                                    "nächstes Vorkommen / alle Vorkommen / weiterer Cursor (Mehrfachcursor)",
                                                    "下一个匹配/所有匹配/额外光标(多光标)",
                                                    "次の一致/すべての一致/カーソル追加(マルチカーソル)")),
            ("Ctrl+Space", T("completion: keywords, cvars, item names, shader paths, @MENUS strings",
                             "Vervollständigung: Schlüsselwörter, Cvars, Itemnamen, Shader-Pfade, @MENUS-Texte",
                             "自动补全:关键字、cvar、项名称、着色器路径、@MENUS 字符串",
                             "補完:キーワード、cvar、項目名、シェーダーのパス、@MENUS 文字列")),
            ("F12 / Ctrl+Click", T("go to the menuDef or itemDef of the name under the cursor",
                                   "zum menuDef oder itemDef des Namens unter dem Cursor",
                                   "跳转到光标下名称的 menuDef 或 itemDef",
                                   "カーソル位置の名前の menuDef/itemDef へ移動")),
            ("Ctrl+F2 / F2 / Shift+F2", T("bookmark like in the script editor (also a click in the margin)",
                                          "Lesezeichen wie im Skript-Editor (auch Klick in den Rand)",
                                          "书签,与脚本编辑器相同(也可单击边栏)",
                                          "スクリプトエディターと同じブックマーク(余白のクリックでも)")),
            ("Ctrl+/ , Tab / Shift+Tab", T("comment lines, indent / outdent", "Zeilen auskommentieren, ein-/ausrücken",
                                            "注释行,缩进/取消缩进", "行のコメント化、インデント/解除")),
        ]),
        ("p", T(
            "The margin works like in the script editor: orange = changed since saving, green = changed and saved, blue = back to the state when opened. `menuDef`, `itemDef` and script blocks fold with the +/- next to the line numbers. Clicking an item on the canvas jumps to its `itemDef`, and the cursor in an `itemDef` selects the item.",
            "Der Rand arbeitet wie im Skript-Editor: orange = seit dem Speichern geändert, grün = geändert und gespeichert, blau = wieder wie beim Öffnen. `menuDef`, `itemDef` und Skriptblöcke falten mit dem +/- neben den Zeilennummern. Ein Klick auf ein Item der Fläche springt zu seinem `itemDef`, und der Cursor in einem `itemDef` wählt das Item.",
            "边栏与脚本编辑器相同:橙色 = 保存后已更改,绿色 = 已更改并已保存,蓝色 = 恢复到打开时的状态。`menuDef`、`itemDef` 和脚本块可用行号旁的 +/- 折叠。单击画布上的项会跳到其 `itemDef`,光标位于某个 `itemDef` 中时会选中该项。",
            "余白はスクリプトエディターと同じです:オレンジ = 保存後に変更、緑 = 変更して保存済み、青 = 開いたときの状態に戻った行。`menuDef`、`itemDef`、スクリプトのブロックは行番号の横の +/- で折りたためます。キャンバスの項目をクリックするとその `itemDef` へ移動し、`itemDef` 内のカーソルでその項目が選ばれます。")),
        ("h2", "speichern", T("Save and export", "Speichern und Export", "保存与导出", "保存とエクスポート")),
        ("p", T(
            "**{{MeSpeichern}}** writes the text exactly as it is in the editor; a file opened from the game is saved elsewhere. **{{MeExportPk3}}** packs the menu under its game path plus your images into a .pk3. Neither ever writes into the game folders.",
            "**{{MeSpeichern}}** schreibt den Text genau so, wie er im Editor steht; eine Datei aus dem Spiel wird woanders gespeichert. **{{MeExportPk3}}** packt das Menü unter seinem Spielpfad und deine Bilder in eine .pk3. Beides schreibt nie in die Spielordner.",
            "**{{MeSpeichern}}** 按编辑器中的原样写入文本;从游戏打开的文件会保存到其他位置。**{{MeExportPk3}}** 将菜单(按其游戏路径)和你的图像打包为 .pk3。两者都绝不会写入游戏文件夹。",
            "**{{MeSpeichern}}** はエディターのテキストをそのまま書き込みます。ゲームから開いたファイルは別の場所に保存します。**{{MeExportPk3}}** はメニューをゲーム内のパスで、画像とともに .pk3 にまとめます。どちらもゲームのフォルダーには書き込みません。")),
        ("note", T(
            "Drawn like the game: fonts from the .fontdat files, text positions as in RE_Font_DrawString, item types as in ui_shared. Placeholders show what needs the running game: ownerdraw items (with their number), list feeders, 3D models and cinematics.",
            "Gezeichnet wie im Spiel: Schriften aus den .fontdat-Dateien, Textpositionen wie in RE_Font_DrawString, Item-Typen wie in ui_shared. Platzhalter zeigen, was das laufende Spiel braucht: Ownerdraw-Items (mit Nummer), Listen-Feeder, 3D-Modelle und Videos.",
            "按游戏方式绘制:字体来自 .fontdat 文件,文本位置与 RE_Font_DrawString 一致,项类型与 ui_shared 一致。占位符表示需要运行中的游戏才能显示的内容:ownerdraw 项(带编号)、列表 feeder、3D 模型和过场动画。",
            "ゲームと同じように描画します:フォントは .fontdat、文字の位置は RE_Font_DrawString、項目の種類は ui_shared と同じです。実行中のゲームが必要なもの(番号つきの ownerdraw 項目、リストのフィーダー、3D モデル、ムービー)はプレースホルダーで示します。")),
    ],
}

SEITEN = [MENUE_EDITOR]
