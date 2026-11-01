"""Stufe H2: neue kurze Tutorials, die auch als interaktive Tour im Programm laufen.

shank: "das ganze Programm hat Tutorials, ist leicht zu verstehen, man findet
alles, es fuehlt sich interaktiv an". Jede Seite hier hat eine Tour
(inhalt_touren.py, NACH_SCHRITT): ein Schritt ist erst erledigt, wenn das
Programm die echte Wirkung sieht (gui/tour_messwerte.cpp).

Leitlinien wie in H1 (docs/intern/NOTIZEN_hilfe1.md): kurze Saetze, aktiv,
"du", ein Schritt = eine Handlung, Fachbegriffe beim ersten Mal erklaert.
"""
from inhalt_basis import T

_IM_PROGRAMM = T("This tutorial also runs inside the program: **{{MenuHelp}} → {{TourMenu}}** - or ask the helper (<kbd>F1</kbd>).",
                 "Dieses Tutorial läuft auch im Programm: **{{MenuHelp}} → {{TourMenu}}** - oder frag den Helfer (<kbd>F1</kbd>).",
                 "本教程也可以在程序中运行:**{{MenuHelp}} → {{TourMenu}}**,或询问助手(<kbd>F1</kbd>)。",
                 "このチュートリアルはプログラムの中でも動きます:**{{MenuHelp}} → {{TourMenu}}**、またはヘルパー(<kbd>F1</kbd>)に聞いてください。")


def tut(sid, gruppe, titel, kurz, meta, bild, schritte, tipps=None):
    bloecke = [("h2", "schritte", T("Steps", "Schritte", "步骤", "手順")), ("steps", schritte)]
    if tipps:
        bloecke.append(("h2", "tipps", T("Good to know", "Gut zu wissen", "须知", "知っておくと便利")))
        bloecke.append(("list", tipps))
    bloecke.append(("note", _IM_PROGRAMM))
    return {"id": sid, "gruppe": gruppe, "titel": titel, "kurz": kurz, "meta": meta, "titelbild": bild, "bloecke": bloecke}


SPIELORDNER = tut(
    "k-spielordner", "tut_mission",
    T("Set up the game folder and the preferences", "Spielordner und Einstellungen einrichten",
      "设置游戏文件夹和首选项", "ゲームフォルダーと設定を整える"),
    T("Tell the program where Jedi Academy is installed, check what it found, and choose the rules of the preview.",
      "Sag dem Programm, wo Jedi Academy installiert ist, prüf, was es gefunden hat, und wähl die Regeln der Vorschau.",
      "告诉程序 Jedi Academy 安装在哪里,检查它找到了什么,并选择预览的规则。",
      "Jedi Academy のインストール先を教え、見つかったものを確認し、プレビューのルールを選びます。"),
    T("Settings · five steps", "Einstellungen · fünf Schritte", "设置 · 五个步骤", "設定 · 5 手順"), "h_spielordner",
    [
        T("**Open the paths.** **{{MenuEdit}} → {{PathsTitle}}**. In the map editor switch to **{{ViewEvents}}** first - the menus **{{MenuFile}}**, **{{MenuEdit}}** and **{{MenuView}}** are there.",
          "**Öffne die Pfade.** **{{MenuEdit}} → {{PathsTitle}}**. Im Karten-Editor wechsle zuerst zu **{{ViewEvents}}** - dort stehen die Menüs **{{MenuFile}}**, **{{MenuEdit}}** und **{{MenuView}}**.",
          "**打开路径。** **{{MenuEdit}} → {{PathsTitle}}**。在地图编辑器中请先切换到 **{{ViewEvents}}**,**{{MenuFile}}**、**{{MenuEdit}}** 和 **{{MenuView}}** 菜单在那里。",
          "**パスを開きます。** **{{MenuEdit}} → {{PathsTitle}}**。マップエディターでは先に **{{ViewEvents}}** に切り替えてください。**{{MenuFile}}**・**{{MenuEdit}}**・**{{MenuView}}** メニューはそこにあります。"),
        (T("**Add the game folder.** Press **{{Pk3AddPath}}** and choose the `base` folder of Jedi Academy - the one with `assets0.pk3`. For Movie Duels add its `MD` folder too.",
          "**Füge den Spielordner hinzu.** Drück **{{Pk3AddPath}}** und wähl den Ordner `base` von Jedi Academy - den mit `assets0.pk3`. Für Movie Duels nimm auch dessen Ordner `MD` dazu.",
          "**添加游戏文件夹。** 按 **{{Pk3AddPath}}**,选择 Jedi Academy 的 `base` 文件夹,也就是包含 `assets0.pk3` 的那个。使用 Movie Duels 时再添加它的 `MD` 文件夹。",
          "**ゲームフォルダーを追加します。** **{{Pk3AddPath}}** を押し、Jedi Academy の `base` フォルダー(`assets0.pk3` があるもの)を選びます。Movie Duels なら `MD` フォルダーも追加します。"),
         "h_tour_spielordner", T("The tutorial in the program: the button is highlighted.", "Das Tutorial im Programm: der Knopf ist hervorgehoben.", "程序中的教程:按钮被高亮显示。", "プログラムの中のチュートリアル:ボタンが強調表示されています。")),
        T("**Check the list.** Every folder shows how many archives it found. If it says none, you picked the wrong folder - remove it and try again. Then close the window.",
          "**Prüf die Liste.** Jeder Ordner zeigt, wie viele Archive er gefunden hat. Steht dort keins, war es der falsche Ordner - entfern ihn und versuch es noch einmal. Dann schließ das Fenster.",
          "**检查列表。** 每个文件夹都会显示找到了多少个压缩包。如果一个都没有,说明选错了文件夹——删除它再试一次。然后关闭窗口。",
          "**一覧を確認します。** 各フォルダーに見つかったアーカイブの数が表示されます。0 なら別のフォルダーです。削除してやり直してください。終わったらウィンドウを閉じます。"),
        T("**Open the preferences.** **{{MenuEdit}} → {{AppPrefs}}**: sound levels, the game rules of the preview, difficulty and how the program behaves.",
          "**Öffne die Einstellungen.** **{{MenuEdit}} → {{AppPrefs}}**: Lautstärken, die Spielregeln der Vorschau, Schwierigkeit und wie sich das Programm verhält.",
          "**打开首选项。** **{{MenuEdit}} → {{AppPrefs}}**:音量、预览的游戏规则、难度以及程序的行为。",
          "**設定を開きます。** **{{MenuEdit}} → {{AppPrefs}}**:音量、プレビューのゲームルール、難易度、プログラムの動作。"),
        T("**Choose the rules.** **{{PrefsRegelwerk}}**: *auto* follows your game folder; OpenJK or Movie Duels make the preview behave exactly like that game - walking, doors, timing.",
          "**Wähl die Regeln.** **{{PrefsRegelwerk}}**: *auto* richtet sich nach deinem Spielordner; OpenJK oder Movie Duels lassen die Vorschau genau wie dieses Spiel laufen - Gehen, Türen, Zeiten.",
          "**选择规则。** **{{PrefsRegelwerk}}**:*auto* 跟随你的游戏文件夹;选择 OpenJK 或 Movie Duels 时,预览会完全按该游戏运行——行走、门、时间。",
          "**ルールを選びます。** **{{PrefsRegelwerk}}**:*auto* はゲームフォルダーに合わせます。OpenJK か Movie Duels を選ぶと、歩き方・ドア・タイミングがそのゲームとまったく同じになります。"),
    ],
    [T("The program only reads the game folder. It never writes into it - your own files go into your mission or mod folder.",
       "Das Programm liest den Spielordner nur. Es schreibt nie hinein - deine eigenen Dateien kommen in deinen Missions- oder Mod-Ordner.",
       "程序只读取游戏文件夹,从不写入。你自己的文件会放到任务或模组文件夹中。",
       "プログラムはゲームフォルダーを読むだけで、書き込みはしません。自分のファイルはミッションや MOD のフォルダーに入ります。")])

UPDATE = tut(
    "k-update", "tut_mission",
    T("Check for updates", "Nach Updates suchen", "检查更新", "アップデートを確認する"),
    T("Look for a new version, read what changed, and decide whether the program checks at start.",
      "Such nach einer neuen Fassung, lies, was sich geändert hat, und entscheide, ob das Programm beim Start nachsieht.",
      "查找新版本,阅读更改内容,并决定程序是否在启动时检查。",
      "新しいバージョンを探し、変更点を読み、起動時に確認するかどうかを決めます。"),
    T("Settings · four steps", "Einstellungen · vier Schritte", "设置 · 四个步骤", "設定 · 4 手順"), "h_ueber",
    [
        (T("**Look for a new version.** **{{MenuHelp}} → {{UpdMenu}}**. The program asks GitHub for the newest release - only now, because you chose it.",
          "**Such nach einer neuen Fassung.** **{{MenuHelp}} → {{UpdMenu}}**. Das Programm fragt GitHub nach der neuesten Fassung - nur jetzt, weil du es gewählt hast.",
          "**查找新版本。** **{{MenuHelp}} → {{UpdMenu}}**。程序会向 GitHub 查询最新版本——只在你选择时才这样做。",
          "**新しいバージョンを探します。** **{{MenuHelp}} → {{UpdMenu}}**。あなたが選んだときだけ、プログラムが GitHub に最新版を問い合わせます。"),
         "h_tour_update", T("The tutorial points at the Help menu.", "Das Tutorial zeigt auf das Menü Hilfe.", "教程指向“帮助”菜单。", "チュートリアルがヘルプ メニューを示しています。")),
        T("**Read the answer.** The window says that you have the newest version - or shows the new one with its notes and **{{UpdInstall}}**. Without a connection it says so; **{{UpdRetry}}** tries again.",
          "**Lies die Antwort.** Das Fenster sagt, dass du die neueste Fassung hast - oder zeigt die neue mit ihren Notizen und **{{UpdInstall}}**. Ohne Verbindung sagt es das; **{{UpdRetry}}** versucht es noch einmal.",
          "**阅读答复。** 窗口会告诉你已是最新版本,或显示新版本及其说明和 **{{UpdInstall}}**。没有网络时它会说明;**{{UpdRetry}}** 会再试一次。",
          "**答えを読みます。** 最新版であること、または新しいバージョンとその説明と **{{UpdInstall}}** が表示されます。接続がないときはそう表示され、**{{UpdRetry}}** で再試行します。"),
        T("**At start or not.** **{{MenuHelp}} → {{UpdAutoCheck}}** switches the check at start on or off. Nothing else is ever sent.",
          "**Beim Start oder nicht.** **{{MenuHelp}} → {{UpdAutoCheck}}** schaltet die Suche beim Start ein oder aus. Sonst wird nie etwas gesendet.",
          "**启动时是否检查。** **{{MenuHelp}} → {{UpdAutoCheck}}** 开启或关闭启动时的检查。除此之外不会发送任何内容。",
          "**起動時に確認するか。** **{{MenuHelp}} → {{UpdAutoCheck}}** で起動時の確認をオン/オフします。それ以外に何かを送ることはありません。"),
        T("**What changed?** **{{ChangelogMenu}}** in the window (or in **{{MenuHelp}}**) lists the changes of every version.",
          "**Was ist neu?** **{{ChangelogMenu}}** im Fenster (oder im Menü **{{MenuHelp}}**) zeigt die Änderungen jeder Fassung.",
          "**有哪些变化?** 窗口中(或 **{{MenuHelp}}** 菜单中)的 **{{ChangelogMenu}}** 列出每个版本的更改。",
          "**何が変わった?** ウィンドウ(または **{{MenuHelp}}** メニュー)の **{{ChangelogMenu}}** に各バージョンの変更が並びます。"),
    ])

KARTE_TUER = tut(
    "tut-karte-tuer", "tut",
    T("A door and a trigger in the map editor", "Eine Tür und ein Trigger im Karten-Editor", "在地图编辑器中制作门和触发器",
      "マップエディターでドアとトリガーを作る"),
    T("Turn a brush into a door, tell it which way to open, and let a trigger open it when the player walks in.",
      "Mach aus einem Brush eine Tür, sag ihr, wohin sie aufgeht, und lass sie von einem Trigger öffnen, wenn der Spieler hineinläuft.",
      "把画刷变成门,设置它的打开方向,并让玩家走进触发器时把它打开。",
      "ブラシをドアにして開く向きを決め、プレイヤーがトリガーに入ったら開くようにします。"),
    T("Map editor · seven steps", "Karten-Editor · sieben Schritte", "地图编辑器 · 七个步骤", "マップエディター · 7 手順"), "h_tut_karte_entities",
    [
        T("**Open the map editor.** **{{ViewMapEditor}}** in the mode bar. Open your map from [[tut-karte]] - or any map with a room.",
          "**Öffne den Karten-Editor.** **{{ViewMapEditor}}** in der Modusleiste. Öffne deine Karte aus [[tut-karte]] - oder eine andere mit einem Raum.",
          "**打开地图编辑器。** 在模式栏中选择 **{{ViewMapEditor}}**。打开 [[tut-karte]] 中的地图,或任何有房间的地图。",
          "**マップエディターを開きます。** モードバーの **{{ViewMapEditor}}**。[[tut-karte]] のマップか、部屋のある別のマップを開きます。"),
        T("**Draw the door.** Drag a thin brush with the LEFT mouse button in a 2D view, where the door should be - for example 16 units thick.",
          "**Zieh die Tür.** Zieh mit der LINKEN Maustaste in einer 2D-Ansicht einen dünnen Brush, wo die Tür hin soll - zum Beispiel 16 Einheiten dick.",
          "**画出门。** 在 2D 视图中门所在的位置,用鼠标左键拖出一个薄画刷,例如 16 个单位厚。",
          "**ドアを描きます。** 2D ビューのドアを置きたい場所で、マウスの左ボタンで薄いブラシをドラッグします(例えば厚さ 16 ユニット)。"),
        T("**Make it a door.** Keep it selected, right-click in the 2D view and choose **func → func_door**. The brush is now an entity that moves.",
          "**Mach eine Tür daraus.** Lass ihn gewählt, klick rechts in die 2D-Ansicht und wähl **func → func_door**. Der Brush ist jetzt eine Entity, die sich bewegt.",
          "**把它变成门。** 保持选中,在 2D 视图中右键点击,选择 **func → func_door**。这个画刷现在是会移动的实体。",
          "**ドアにします。** 選択したまま 2D ビューで右クリックし、**func → func_door** を選びます。ブラシが動くエンティティになります。"),
        T("**Which way it opens.** Press <kbd>N</kbd> for the entity inspector. Set the key `angle` to `90` (it slides north; `-1` is up, `-2` down) and press <kbd>Enter</kbd>.",
          "**Wohin sie aufgeht.** Drück <kbd>N</kbd> für den Entity-Inspektor. Setz den Schlüssel `angle` auf `90` (sie gleitet nach Norden; `-1` ist hoch, `-2` runter) und drück <kbd>Enter</kbd>.",
          "**打开的方向。** 按 <kbd>N</kbd> 打开实体检查器。把键 `angle` 设为 `90`(向北滑开;`-1` 向上,`-2` 向下),然后按 <kbd>Enter</kbd>。",
          "**開く向き。** <kbd>N</kbd> でエンティティインスペクターを開きます。キー `angle` を `90` にして(北へ滑ります。`-1` は上、`-2` は下)<kbd>Enter</kbd> を押します。"),
        T("**Draw the trigger.** Draw a second brush in front of the door, right-click and choose **trigger → trigger_multiple**. Triggers are invisible in the game.",
          "**Zieh den Trigger.** Zieh einen zweiten Brush vor der Tür, klick rechts und wähl **trigger → trigger_multiple**. Trigger sind im Spiel unsichtbar.",
          "**画出触发器。** 在门前画第二个画刷,右键点击并选择 **trigger → trigger_multiple**。触发器在游戏中是看不见的。",
          "**トリガーを描きます。** ドアの前に 2 つ目のブラシを描き、右クリックして **trigger → trigger_multiple** を選びます。トリガーはゲーム内では見えません。"),
        T("**Connect them.** Select the trigger first, then the door (<kbd>Shift</kbd>+click), and press <kbd>Ctrl</kbd>+<kbd>K</kbd> - **{{KeLEntity}} → {{KeLConnectEntities}}**. The trigger's `target` now names the door's `targetname`.",
          "**Verbinde sie.** Wähl zuerst den Trigger, dann die Tür (<kbd>Umschalt</kbd>+Klick), und drück <kbd>Strg</kbd>+<kbd>K</kbd> - **{{KeLEntity}} → {{KeLConnectEntities}}**. Der `target` des Triggers nennt jetzt den `targetname` der Tür.",
          "**连接它们。** 先选中触发器,再选中门(<kbd>Shift</kbd>+点击),然后按 <kbd>Ctrl</kbd>+<kbd>K</kbd>,即 **{{KeLEntity}} → {{KeLConnectEntities}}**。触发器的 `target` 现在指向门的 `targetname`。",
          "**つなぎます。** 先にトリガー、次にドアを選び(<kbd>Shift</kbd>+クリック)、<kbd>Ctrl</kbd>+<kbd>K</kbd> を押します(**{{KeLEntity}} → {{KeLConnectEntities}}**)。トリガーの `target` がドアの `targetname` を指すようになります。"),
        T("**Try it.** Build the map and walk through it as in [[tut-karte]]: the door opens when you step into the trigger.",
          "**Probier es aus.** Bau die Karte und lauf durch wie in [[tut-karte]]: Die Tür geht auf, wenn du in den Trigger trittst.",
          "**试一试。** 像 [[tut-karte]] 中那样编译地图并走一遍:你走进触发器时门就会打开。",
          "**試します。** [[tut-karte]] と同じようにマップをビルドして歩きます。トリガーに入るとドアが開きます。"),
    ],
    [T("A door without a name opens by itself when the player touches it. Once a trigger names it, it waits for the trigger.",
       "Eine Tür ohne Namen geht von selbst auf, wenn der Spieler sie berührt. Sobald ein Trigger sie nennt, wartet sie auf den Trigger.",
       "没有名字的门在玩家碰到时会自己打开。一旦有触发器指向它,它就会等待触发器。",
       "名前のないドアはプレイヤーが触れると自分で開きます。トリガーに指定されると、トリガーを待つようになります。")])

MENUE = tut(
    "tut-menue", "tut",
    T("Change a game menu", "Ein Menü des Spiels ändern", "修改游戏菜单", "ゲームのメニューを変える"),
    T("Open a menu of the game, move a button, try it like in the game and pack it for your mod.",
      "Öffne ein Menü des Spiels, verschieb einen Knopf, probier es wie im Spiel aus und pack es für deinen Mod.",
      "打开游戏中的一个菜单,移动一个按钮,像在游戏中一样试用,然后为你的模组打包。",
      "ゲームのメニューを開き、ボタンを動かし、ゲームと同じように試して、MOD 用にまとめます。"),
    T("Menu editor · five steps", "Menü-Editor · fünf Schritte", "菜单编辑器 · 五个步骤", "メニューエディター · 5 手順"), "menue_original",
    [
        T("**Open the menu editor.** **{{ViewMenuEditor}}** in the mode bar.",
          "**Öffne den Menü-Editor.** **{{ViewMenuEditor}}** in der Modusleiste.",
          "**打开菜单编辑器。** 在模式栏中选择 **{{ViewMenuEditor}}**。",
          "**メニューエディターを開きます。** モードバーの **{{ViewMenuEditor}}**。"),
        T("**Open a menu of the game.** **{{MeAusSpiel}}** and pick one, for example `ui/jamp/main.menu`. It is read from the game files and drawn like the game - the game folder is not changed.",
          "**Öffne ein Menü des Spiels.** **{{MeAusSpiel}}** und wähl eins, zum Beispiel `ui/jamp/main.menu`. Es wird aus den Spieldateien gelesen und wie im Spiel gezeichnet - der Spielordner bleibt unverändert.",
          "**打开游戏中的菜单。** 点击 **{{MeAusSpiel}}** 并选择一个,例如 `ui/jamp/main.menu`。它从游戏文件读取并按游戏的方式绘制,游戏文件夹不会被修改。",
          "**ゲームのメニューを開きます。** **{{MeAusSpiel}}** を押して、例えば `ui/jamp/main.menu` を選びます。ゲームのファイルから読み込まれ、ゲームと同じように描かれます。ゲームフォルダーは変わりません。"),
        T("**Move an element.** Click a button on the canvas to select it, then press it again and drag it - or nudge it with the arrow keys. Its `rect` in {{MeReiterEigenschaften}} and in the {{MeReiterText}} changes with it.",
              "**Verschieb ein Element.** Klick einen Knopf auf der Fläche an, um ihn auszuwählen, drück ihn dann erneut und zieh ihn - oder schieb ihn mit den Pfeiltasten. Sein `rect` in {{MeReiterEigenschaften}} und im {{MeReiterText}} ändert sich mit.",
              "**移动一个元素。** 在画布上单击一个按钮将其选中,再次按住并拖动它,或用方向键微调。{{MeReiterEigenschaften}} 和 {{MeReiterText}} 中的 `rect` 会随之改变。",
              "**要素を動かします。** キャンバスのボタンをクリックして選択し、もう一度押してドラッグするか、矢印キーで少しずつ動かします。{{MeReiterEigenschaften}} と {{MeReiterText}} の `rect` も一緒に変わります。"),
        (T("**Try it like in the game.** **{{MeBedienen}}**: clicks now press buttons and change cvars - only in the preview, never in your game. **{{MeBearbeiten}}** brings you back.",
          "**Probier es wie im Spiel.** **{{MeBedienen}}**: Klicks drücken jetzt Knöpfe und ändern Cvars - nur in der Vorschau, nie in deinem Spiel. **{{MeBearbeiten}}** bringt dich zurück.",
          "**像在游戏中一样试用。** **{{MeBedienen}}**:现在点击会按下按钮并更改 cvar,只在预览中,绝不会影响你的游戏。**{{MeBearbeiten}}** 返回编辑。",
          "**ゲームと同じように試します。** **{{MeBedienen}}**:クリックでボタンが押され、cvar が変わります。プレビューの中だけで、ゲームには影響しません。**{{MeBearbeiten}}** で戻ります。"),
         "h_tour_menue", T("Step 4 of the tutorial in the menu editor.", "Schritt 4 des Tutorials im Menü-Editor.", "菜单编辑器中教程的第 4 步。", "メニューエディターでのチュートリアルの手順 4。")),
        T("**Pack it.** **{{MeExportPk3}}** writes the changed menu into a .pk3 for your mod folder, for example `zz_menu.pk3`. The game folder itself is never written.",
          "**Pack es.** **{{MeExportPk3}}** schreibt das geänderte Menü in eine .pk3 für deinen Mod-Ordner, zum Beispiel `zz_menu.pk3`. In den Spielordner selbst wird nie geschrieben.",
          "**打包。** **{{MeExportPk3}}** 把修改后的菜单写入一个用于模组文件夹的 .pk3,例如 `zz_menu.pk3`。绝不会写入游戏文件夹本身。",
          "**まとめます。** **{{MeExportPk3}}** で変更したメニューを MOD フォルダー用の .pk3(例えば `zz_menu.pk3`)に書き出します。ゲームフォルダー自体には書き込みません。"),
    ],
    [T("More in [[menue_editor]].", "Mehr in [[menue_editor]].", "更多内容见 [[menue_editor]]。", "詳しくは [[menue_editor]]。")])

def _schrittbilder(seite, name):
    """Abschluss rc607: jeder Schritt mit seinem Bild aus dem Programm (wie die anderen Tutorials) - die Tour
    an diesem Schritt mit ihrer Hervorhebung (Selbsttest hilfe2, BHED_HB_NUR=hilfe2: <name>_<Schritt>)."""
    for b in seite["bloecke"]:
        if b[0] == "steps":
            b[1][:] = [x if isinstance(x, tuple) else
                       (x, "%s_%d" % (name, i),
                        T("Step %d in the program: the tutorial highlights where to go next." % i,
                          "Schritt %d im Programm: das Tutorial hebt hervor, wo es weitergeht." % i,
                          "程序中的第 %d 步:教程高亮显示下一步的位置。" % i,
                          "プログラムでの手順 %d: チュートリアルが次の操作場所を強調表示します。" % i))
                       for i, x in enumerate(b[1], 1)]


_schrittbilder(SPIELORDNER, "h_tut_spielordner")
_schrittbilder(UPDATE, "h_tut_update")
_schrittbilder(KARTE_TUER, "h_tut_karte_tuer")
_schrittbilder(MENUE, "h_tut_menue_h2")

SEITEN = [SPIELORDNER, UPDATE, KARTE_TUER, MENUE]


# --- H2: die Seite "Der Helfer und die Touren" ergaenzen (inhalt_hilfe1.HELFER) ----------
# Hier statt in inhalt_hilfe1.py, damit die Merges nur Ergaenzungen sind.
import inhalt_hilfe1  # noqa: E402

_HELFER_NEU = [
    ("h2", "finden", T("Finding things faster", "Schneller finden", "更快地找到", "もっと早く見つける")),
    ("list", [
        T("**Where you work counts.** In the map editor, results of the map editor come first; in the menu editor, those of the menu editor. \"Settings\" in the map editor finds its own preferences.",
          "**Wo du arbeitest, zählt.** Im Karten-Editor stehen Treffer des Karten-Editors oben, im Menü-Editor die des Menü-Editors. „Einstellungen“ findet im Karten-Editor dessen eigene.",
          "**你在哪里工作很重要。** 在地图编辑器中,地图编辑器的结果排在前面;在菜单编辑器中则是菜单编辑器的结果。在地图编辑器中搜索“设置”会找到它自己的首选项。",
          "**作業している場所が大事です。** マップエディターではマップエディターの結果が、メニューエディターではメニューエディターの結果が上に来ます。マップエディターで「設定」と書くと、その設定が見つかります。"),
        T("**Your own words, any form.** \"runs\", \"ran\", \"running\" or \"rennt\", \"läuft\" find the same. Small typos are fine; with no match at all the helper offers **Did you mean ...?**",
          "**Eigene Worte, jede Form.** „rennt“, „läuft“, „gesetzt“ oder „running“, „placed“ finden dasselbe. Kleine Tippfehler machen nichts; findet er gar nichts, fragt der Helfer **Meintest du ...?**",
          "**用自己的话,任何形式都行。** 不同的词形也能找到同样的内容。小的拼写错误没关系;如果完全找不到,助手会提示 **你是不是要找……?**",
          "**自分の言葉で、どんな形でも。** 活用形が違っても同じものが見つかります。小さな打ち間違いも大丈夫です。何も見つからないと、ヘルパーが **もしかして…?** と提案します。"),
        T("**It also knows what is inside.** The buttons of the toolbars, the pages of the map editor's preferences, the menu editor and what a dialog contains are all part of the search.",
          "**Er kennt auch das Innere.** Die Knöpfe der Leisten, die Seiten der Einstellungen des Karten-Editors, der Menü-Editor und was in einem Dialog steht, gehören alle zur Suche.",
          "**它也知道里面有什么。** 工具栏按钮、地图编辑器首选项的各个页面、菜单编辑器以及对话框中的内容都可以搜索。",
          "**中身も知っています。** ツールバーのボタン、マップエディターの設定の各ページ、メニューエディター、ダイアログの中身もすべて検索の対象です。"),
        T("**Show me, step by step.** If the control is not visible yet, **{{HlfShowMe}}** first points at what opens it - for a setting of the main program searched in the map editor: first **{{ViewEvents}}**, then **{{MenuEdit}} → {{AppPrefs}}**, then the field itself.",
          "**Zeig es mir, Schritt für Schritt.** Ist das Bedienelement noch nicht zu sehen, zeigt **{{HlfShowMe}}** zuerst auf das, was es öffnet - bei einer Einstellung des Hauptprogramms, gesucht im Karten-Editor: erst **{{ViewEvents}}**, dann **{{MenuEdit}} → {{AppPrefs}}**, dann das Feld selbst.",
          "**一步一步演示。** 如果控件还看不到,**{{HlfShowMe}}** 会先指向打开它的地方——例如在地图编辑器中搜索主程序的某项设置:先指向 **{{ViewEvents}}**,然后是 **{{MenuEdit}} → {{AppPrefs}}**,最后是该字段本身。",
          "**一歩ずつ見せます。** 操作部分がまだ見えないときは、**{{HlfShowMe}}** がまずそれを開く場所を示します。マップエディターでメインプログラムの設定を探したなら、まず **{{ViewEvents}}**、次に **{{MenuEdit}} → {{AppPrefs}}**、最後にその項目です。"),
    ]),
]

_KI_NEU = [
    ("p", T(
        "Ask follow-up questions: **{{KiAskMore}}** keeps the conversation, so Claude knows what you asked before. The answer appears while it is written; **{{KiStop}}** stops it at once - nothing of that question stays. **{{KiHistoryClear}}** deletes the conversation from your computer.",
        "Frag einfach weiter: **{{KiAskMore}}** behält das Gespräch, Claude weiß also, was du vorher gefragt hast. Die Antwort erscheint, während sie geschrieben wird; **{{KiStop}}** bricht sofort ab - von dieser Frage bleibt nichts. **{{KiHistoryClear}}** löscht das Gespräch von deinem Computer.",
        "可以继续追问:**{{KiAskMore}}** 会保留对话,所以 Claude 知道你之前问过什么。回答会边写边显示;**{{KiStop}}** 会立即停止,这个问题不会留下任何内容。**{{KiHistoryClear}}** 会从你的电脑中删除对话。",
        "続けて質問できます。**{{KiAskMore}}** は会話を保つので、Claude は前の質問を覚えています。回答は書かれながら表示され、**{{KiStop}}** ですぐに止まります(その質問は何も残りません)。**{{KiHistoryClear}}** で会話をコンピューターから削除します。")),
    ("p", T(
        "In the menu editor Claude can also select, move, duplicate or delete elements and switch to **{{MeBedienen}}** - again only after **{{KiRun}}**, and the whole job is one undo step. Opening, saving or exporting a menu is not possible for the AI.",
        "Im Menü-Editor kann Claude außerdem Elemente wählen, verschieben, verdoppeln oder löschen und zu **{{MeBedienen}}** wechseln - auch hier erst nach **{{KiRun}}**, und der ganze Auftrag ist ein Rückgängig-Schritt. Ein Menü öffnen, speichern oder exportieren kann die KI nicht.",
        "在菜单编辑器中,Claude 还可以选择、移动、复制或删除元素,并切换到 **{{MeBedienen}}**——同样只有在你按 **{{KiRun}}** 之后,整个任务是一个撤销步骤。AI 不能打开、保存或导出菜单。",
        "メニューエディターでは、Claude は要素の選択・移動・複製・削除や **{{MeBedienen}}** への切り替えもできます。ここでも **{{KiRun}}** の後だけで、依頼全体が 1 つの元に戻す手順です。メニューを開く・保存する・エクスポートすることは AI にはできません。")),
]

_SICHER_NEU = [
    T("**The conversation stays on your PC.** It is stored encrypted for your Windows user and deleted with **{{KiHistoryClear}}**. After each request the program also wipes the key from its memory.",
      "**Das Gespräch bleibt auf deinem PC.** Es liegt verschlüsselt für deinen Windows-Benutzer und verschwindet mit **{{KiHistoryClear}}**. Nach jeder Anfrage löscht das Programm den Schlüssel auch aus seinem Speicher.",
      "**对话保存在你的电脑上。** 它针对你的 Windows 用户加密保存,用 **{{KiHistoryClear}}** 即可删除。每次请求后,程序还会把密钥从内存中清除。",
      "**会話はあなたの PC に残ります。** Windows ユーザーごとに暗号化して保存され、**{{KiHistoryClear}}** で削除されます。リクエストのたびに、プログラムはキーをメモリーからも消去します。"),
]


def _einfuegen_vor(bloecke, anker, neu):
    for i, b in enumerate(bloecke):
        if b[0] == "h2" and b[1] == anker:
            bloecke[i:i] = neu
            return
    raise SystemExit("inhalt_hilfe2: Abschnitt %s fehlt auf der Seite helfer" % anker)


_B = inhalt_hilfe1.HELFER["bloecke"]
if not any(b[0] == "h2" and b[1] == "finden" for b in _B):
    _einfuegen_vor(_B, "touren", _HELFER_NEU)
    _einfuegen_vor(_B, "mcp", _KI_NEU)
    for _b in _B:
        if _b[0] == "list" and _B[_B.index(_b) - 1][:2] == ("h2", "sicher"):
            _b[1].extend(_SICHER_NEU)
            break
