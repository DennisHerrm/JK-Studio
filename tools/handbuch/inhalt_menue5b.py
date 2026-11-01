# -*- coding: utf-8 -*-
"""Handbuchseiten des Menue-Editors, Stufe M5b: Projekte (*.jksmenu) mit Sicherung, Mod-Profile,
Layouts je show/hide-Zustand, Jedi Outcast echt - und das Tutorial "Dein erstes eigenes Menue"
(tut-menue-eigen, Tour im Helfer). Die Bilder nimmt der Selbsttest menueeditor auf (BHED_MENUE_NUR=m5b,
BHED_MENUE_NUR=tour)."""
from inhalt_basis import T

MENUE_EDITOR_M5B = {
    "id": "menue_editor_m5b",
    "gruppe": "start",
    "titel": T("Menu editor: projects, mod profiles, Jedi Outcast", "Menü-Editor: Projekte, Mod-Profile, Jedi Outcast",
               "菜单编辑器:项目、Mod 配置、Jedi Outcast", "メニューエディター: プロジェクト、Mod プロファイル、Jedi Outcast"),
    "kurz": T(
        "Keep a menu mod in a project folder laid out like the pk3, with automatic backup and restore after a crash; tell the editor what a mod's engine knows with a mod profile; layouts that follow show and hide; Jedi Outcast menus tested in the real game.",
        "Eine Menü-Mod in einem Projektordner wie die pk3, mit automatischer Sicherung und Wiederherstellen nach einem Absturz; mit einem Mod-Profil sagst du dem Editor, was die Engine einer Mod kennt; Layouts, die show und hide folgen; Jedi-Outcast-Menüs im echten Spiel geprüft.",
        "把菜单 Mod 放在结构与 pk3 相同的项目文件夹中,带自动备份和崩溃后恢复;用 Mod 配置告诉编辑器该 Mod 的引擎认识什么;布局会跟随 show 和 hide;Jedi Outcast 菜单已在真实游戏中检验。",
        "メニュー Mod を pk3 と同じ構成のプロジェクトフォルダーにまとめ、自動バックアップとクラッシュ後の復元付き。Mod プロファイルで Mod のエンジンが知っているものをエディターに伝えます。show と hide に従うレイアウト。Jedi Outcast のメニューは実際のゲームで確認済みです。"),
    "bloecke": [
        ("h2", "projekt", T("Projects", "Projekte", "项目", "プロジェクト")),
        ("p", T(
            "**{{MePrjProjekt}} > {{MePrjNeu}}** creates a project file `*.jksmenu` with the folders `ui`, `gfx`, `video`, `shaders` and `strings` next to it - the same layout as the finished pk3. Menus that are open come along; without one, the project starts with an empty full-screen menu. In a project, **{{MeSpeichern}}** saves everything: the menus, their images, videos, texts and fonts, and the project file. The project file remembers the game, the mod profile, the menus and which one was open, the preview (resolution, letterbox, language, cvars) and the export (pk3 name, only changes). **{{MePrjExport}}** packs the pk3.",
            "**{{MePrjProjekt}} > {{MePrjNeu}}** legt eine Projektdatei `*.jksmenu` an, daneben die Ordner `ui`, `gfx`, `video`, `shaders` und `strings` - aufgebaut wie die fertige pk3. Offene Menüs kommen mit; ohne beginnt das Projekt mit einem leeren Vollbild-Menü. Im Projekt speichert **{{MeSpeichern}}** alles: die Menüs, ihre Bilder, Videos, Texte und Schriften und die Projektdatei. Die Projektdatei merkt sich das Spiel, das Mod-Profil, die Menüs und welches offen war, die Vorschau (Auflösung, Balken, Sprache, Cvars) und den Export (Name der pk3, nur Änderungen). **{{MePrjExport}}** packt die pk3.",
            "**{{MePrjProjekt}} > {{MePrjNeu}}** 会创建项目文件 `*.jksmenu`,旁边是 `ui`、`gfx`、`video`、`shaders` 和 `strings` 文件夹——与最终的 pk3 结构相同。已打开的菜单会一并加入;若没有,项目从一个空的全屏菜单开始。在项目中,**{{MeSpeichern}}** 会保存全部内容:菜单及其图像、视频、文本和字体,以及项目文件。项目文件会记住游戏、Mod 配置、菜单和当时打开的菜单、预览(分辨率、黑边、语言、cvar)以及导出设置(pk3 名称、仅更改)。**{{MePrjExport}}** 会打包 pk3。",
            "**{{MePrjProjekt}} > {{MePrjNeu}}** でプロジェクトファイル `*.jksmenu` を作り、隣に `ui`、`gfx`、`video`、`shaders`、`strings` フォルダーができます。完成した pk3 と同じ構成です。開いているメニューも入り、なければ空の全画面メニューから始まります。プロジェクトでは **{{MeSpeichern}}** がすべてを保存します: メニューとその画像・動画・テキスト・フォント、プロジェクトファイル。プロジェクトファイルはゲーム、Mod プロファイル、メニューと開いていたメニュー、プレビュー(解像度、黒帯、言語、cvar)、書き出し(pk3 名、変更のみ)を覚えています。**{{MePrjExport}}** で pk3 を作ります。")),
        ("p", T(
            "Every 30 seconds the editor backs up menus that are not saved yet. If JK Studio was not closed properly, it asks the next time whether to restore them - the restored text counts as an unsaved change, so Undo goes back to the saved version. A double-click on a `.jksmenu` opens JK Studio once you allowed it with **{{MePrjZuordnen}}** (only for your Windows account).",
            "Alle 30 Sekunden sichert der Editor Menüs, die noch nicht gespeichert sind. Wurde JK Studio nicht ordentlich beendet, fragt es beim nächsten Mal, ob sie zurückkommen sollen - der wiederhergestellte Text gilt als ungespeicherte Änderung, Rückgängig führt also zur gespeicherten Fassung. Ein Doppelklick auf eine `.jksmenu` öffnet JK Studio, sobald du es mit **{{MePrjZuordnen}}** erlaubt hast (nur für dein Windows-Konto).",
            "编辑器每 30 秒备份一次尚未保存的菜单。如果 JK Studio 没有正常关闭,下次会询问是否恢复——恢复的文本算作未保存的更改,因此撤销会回到已保存的版本。用 **{{MePrjZuordnen}}** 允许之后(仅限你的 Windows 账户),双击 `.jksmenu` 就会打开 JK Studio。",
            "エディターは保存されていないメニューを 30 秒ごとにバックアップします。JK Studio が正しく終了しなかった場合、次回に復元するか尋ねます。復元したテキストは保存されていない変更として扱われるので、元に戻すと保存済みの版に戻ります。**{{MePrjZuordnen}}** で許可すると(あなたの Windows アカウントのみ)、`.jksmenu` をダブルクリックで JK Studio が開きます。")),
        ("img", "menue_projekt_sicherung", T("After a crash: the editor offers the backed-up menus.", "Nach einem Absturz: der Editor bietet die gesicherten Menüs an.",
                                             "崩溃之后:编辑器提供已备份的菜单。", "クラッシュの後: エディターがバックアップしたメニューを提示します。")),
        ("h2", "profil", T("Mod profiles", "Mod-Profile", "Mod 配置", "Mod プロファイル")),
        ("p", T(
            "A mod's engine may know more than the game: extra keywords, owner-draw and feeder numbers, uiScripts, cvars and menu lists. Choose the mod's profile under **{{MePrjProjekt}} > {{MeProfil}}**: the editor then reads these keywords without a message, shows owner-draws as labelled placeholders and lists the values under **Mod keywords** - the file stays exactly as it is. With the mod's source code, `tools/gen_menue_profil.py` writes the profile; without it, **{{MeProfilVorlage}}** writes a template to fill in. Jedi Academy, Jedi Outcast and Movie Duels have built-in profiles.",
            "Die Engine einer Mod kann mehr als das Spiel: zusätzliche Schlüsselwörter, Ownerdraw- und Feeder-Nummern, uiScripts, Cvars und Menülisten. Wähle das Profil der Mod unter **{{MePrjProjekt}} > {{MeProfil}}**: dann liest der Editor diese Schlüsselwörter ohne Meldung, zeigt Ownerdraws als beschriftete Platzhalter und listet die Werte unter **Mod-Schlüssel** - die Datei bleibt genau so, wie sie ist. Mit dem Quellcode der Mod schreibt `tools/gen_menue_profil.py` das Profil; ohne schreibt **{{MeProfilVorlage}}** eine Vorlage zum Ausfüllen. Jedi Academy, Jedi Outcast und Movie Duels haben eingebaute Profile.",
            "Mod 的引擎可能比游戏认识更多:额外的关键字、ownerdraw 和 feeder 编号、uiScript、cvar 和菜单列表。在 **{{MePrjProjekt}} > {{MeProfil}}** 中选择该 Mod 的配置:编辑器会不再报错地读取这些关键字,把 ownerdraw 显示为带标签的占位框,并在 **Mod 关键字** 下列出其值——文件保持原样。有 Mod 源代码时,`tools/gen_menue_profil.py` 会生成配置;没有时,**{{MeProfilVorlage}}** 会写出一个可填写的模板。Jedi Academy、Jedi Outcast 和 Movie Duels 有内置配置。",
            "Mod のエンジンはゲームより多くを知っていることがあります: 追加のキーワード、ownerdraw と feeder の番号、uiScript、cvar、メニューリスト。**{{MePrjProjekt}} > {{MeProfil}}** で Mod のプロファイルを選ぶと、エディターはこれらのキーワードを警告なしで読み、ownerdraw を名前付きの枠で表示し、値を **Mod のキーワード** に並べます。ファイルはそのままです。Mod のソースコードがあれば `tools/gen_menue_profil.py` がプロファイルを書き、なければ **{{MeProfilVorlage}}** が記入用のひな形を書きます。Jedi Academy、Jedi Outcast、Movie Duels には内蔵のプロファイルがあります。")),
        ("img", "menue_profil", T("A mod keyword from the profile, labelled in the properties.", "Ein Mod-Schlüssel aus dem Profil, beschriftet in den Eigenschaften.",
                                  "来自配置的 Mod 关键字,在属性中带有标签。", "プロファイルの Mod キーワード。プロパティに名前付きで表示されます。")),
        ("h2", "zustaende", T("Layouts that follow show and hide", "Layouts, die show und hide folgen", "跟随 show 和 hide 的布局", "show と hide に従うレイアウト")),
        ("p", T(
            "With **{{MeLayoutVersteckt}}** switched on, every script that shows or hides elements of the layout (a click, a hover, onOpen) also gets `setitemrect` lines that move the other elements into place - calculated for the state after that script and kept up to date with every layout change. Jedi Outcast has no `setitemrect`; there the editor explains what to do instead.",
            "Ist **{{MeLayoutVersteckt}}** an, bekommt jedes Skript, das Elemente des Layouts zeigt oder versteckt (ein Klick, ein Hover, onOpen), auch `setitemrect`-Zeilen, die die anderen Elemente zurechtrücken - für den Zustand nach diesem Skript gerechnet und bei jeder Layout-Änderung neu. Jedi Outcast kennt kein `setitemrect`; dort sagt der Editor, was du stattdessen tun kannst.",
            "打开 **{{MeLayoutVersteckt}}** 后,每个显示或隐藏布局元素的脚本(单击、悬停、onOpen)都会加入 `setitemrect` 行,把其他元素移到正确位置——按该脚本执行后的状态计算,并随每次布局修改更新。Jedi Outcast 不支持 `setitemrect`,编辑器会说明可以改用什么办法。",
            "**{{MeLayoutVersteckt}}** をオンにすると、レイアウトの要素を表示・非表示にするスクリプト(クリック、ホバー、onOpen)すべてに、ほかの要素を正しい位置に動かす `setitemrect` 行が入ります。そのスクリプトの後の状態で計算し、レイアウトを変えるたびに更新します。Jedi Outcast には `setitemrect` がないので、代わりの方法をエディターが説明します。")),
        ("img", "menue_zustaende", T("Button 2 is hidden after opening; Show and Hide move button 3 like the layout.",
                                     "Knopf 2 ist nach dem Öffnen versteckt; Zeig und Weg rücken Knopf 3 wie das Layout.",
                                     "按钮 2 打开后被隐藏;“显示”和“隐藏”会像布局一样移动按钮 3。", "ボタン 2 は開いた直後は非表示です。表示と非表示でボタン 3 がレイアウトどおりに動きます。")),
        ("h2", "jk2", T("Jedi Outcast", "Jedi Outcast", "Jedi Outcast", "Jedi Outcast")),
        ("p", T(
            "The Jedi Outcast menus were compared with the real game (Steam version). The editor now follows its engine: no `setitemrect`, `setitemtext`, `transition2` or `delay`, no `accept` event, the logo models (`.md3`) with the game's camera. **{{MeImSpielTesten}}** starts `jk2sp.exe` with your own folder as `fs_basepath` and the game data via `fs_cdpath` - nothing is written into the game folder.",
            "Die Menüs von Jedi Outcast wurden mit dem echten Spiel (Steam-Fassung) verglichen. Der Editor folgt jetzt dessen Engine: kein `setitemrect`, `setitemtext`, `transition2` oder `delay`, kein Ereignis `accept`, die Logo-Modelle (`.md3`) mit der Kamera des Spiels. **{{MeImSpielTesten}}** startet `jk2sp.exe` mit deinem eigenen Ordner als `fs_basepath` und den Spieldaten über `fs_cdpath` - in den Spielordner wird nichts geschrieben.",
            "Jedi Outcast 的菜单已与真实游戏(Steam 版)对照。编辑器现在遵循其引擎:没有 `setitemrect`、`setitemtext`、`transition2` 或 `delay`,没有 `accept` 事件,标志模型(`.md3`)使用游戏的相机。**{{MeImSpielTesten}}** 会以你自己的文件夹作为 `fs_basepath`、通过 `fs_cdpath` 读取游戏数据来启动 `jk2sp.exe`——不会向游戏文件夹写入任何内容。",
            "Jedi Outcast のメニューを実際のゲーム(Steam 版)と比較しました。エディターはそのエンジンに従います: `setitemrect`、`setitemtext`、`transition2`、`delay` はなく、`accept` イベントもなく、ロゴのモデル(`.md3`)はゲームのカメラで描きます。**{{MeImSpielTesten}}** は自分のフォルダーを `fs_basepath`、ゲームデータを `fs_cdpath` にして `jk2sp.exe` を起動します。ゲームフォルダーには何も書き込みません。")),
    ],
}

TUTORIAL_MENUE = {
    "id": "tut-menue-eigen",
    "gruppe": "tut",
    "titel": T("Your first own menu", "Dein erstes eigenes Menü", "你的第一个菜单", "はじめての自作メニュー"),
    "kurz": T(
        "From an empty editor to a project with three buttons side by side that glow on hover - saved, ready to pack as a pk3.",
        "Vom leeren Editor zum Projekt mit drei Knöpfen nebeneinander, die beim Hover leuchten - gespeichert und bereit für die pk3.",
        "从空的编辑器到一个项目:三个并排的按钮,悬停时发光——已保存,可打包为 pk3。",
        "空のエディターから、ホバーで光る 3 つのボタンが横に並んだプロジェクトまで。保存して pk3 にできる状態にします。"),
    "titelbild": "h_tut_menue_5",
    "meta": T("Menu editor · about 5 minutes", "Menü-Editor · etwa 5 Minuten", "菜单编辑器 · 约 5 分钟", "メニューエディター · 約 5 分"),
    "bloecke": [
        ("p", T(
            "The pictures were taken while the program did exactly these steps; the self-test checks after each step that it really happened. In the program, the helper (F1) runs this tutorial as a tour and moves on by itself when a step is done.",
            "Die Bilder entstanden, während das Programm genau diese Schritte ausführte; der Selbsttest prüft nach jedem Schritt, dass er wirklich geschehen ist. Im Programm führt der Helfer (F1) dieses Tutorial als Tour vor und schaltet selbst weiter, sobald ein Schritt erledigt ist.",
            "这些图片是在程序执行这些步骤时拍摄的;自检会在每一步之后检查它确实完成了。在程序中,帮助(F1)会以导览形式演示本教程,完成一步后自动进入下一步。",
            "画像はプログラムがまさにこの手順を実行しているときに撮影したものです。セルフテストは各手順の後に本当に実行されたかを確認します。プログラムではヘルパー(F1)がこのチュートリアルをツアーとして案内し、手順が終わると自動で次へ進みます。")),
        ("h2", "schritte", T("Steps", "Schritte", "步骤", "手順")),
        ("steps", [
            (T("**Open the menu editor.** Click **{{ViewMenuEditor}}** in the mode bar.",
               "**Menü-Editor öffnen.** Klicke in der Modusleiste auf **{{ViewMenuEditor}}**.",
               "**打开菜单编辑器。** 在模式栏中单击 **{{ViewMenuEditor}}**。",
               "**メニューエディターを開く。** モードバーで **{{ViewMenuEditor}}** をクリックします。"),
             "h_tut_menue_1", T("The empty menu editor with the project buttons.", "Der leere Menü-Editor mit den Projekt-Knöpfen.", "带有项目按钮的空菜单编辑器。", "プロジェクトボタンのある空のメニューエディター。")),
            (T("**Start a project.** Click **{{MePrjNeu}}** and choose a folder for the project file. The editor creates the folders like in a pk3 and an empty full-screen menu.",
               "**Projekt anlegen.** Klicke auf **{{MePrjNeu}}** und wähle einen Ordner für die Projektdatei. Der Editor legt die Ordner wie in einer pk3 an und ein leeres Vollbild-Menü.",
               "**新建项目。** 单击 **{{MePrjNeu}}**,为项目文件选择一个文件夹。编辑器会像 pk3 那样创建文件夹,并新建一个空的全屏菜单。",
               "**プロジェクトを作る。** **{{MePrjNeu}}** をクリックし、プロジェクトファイルのフォルダーを選びます。エディターは pk3 と同じフォルダーと空の全画面メニューを作ります。"),
             "h_tut_menue_2", T("The new project with its first menu.", "Das neue Projekt mit seinem ersten Menü.", "新项目及其第一个菜单。", "最初のメニューがある新しいプロジェクト。")),
            (T("**Three buttons.** Double-click **{{MlTypButton}}** in the building blocks three times, or drag it onto the canvas.",
              "**Drei Knöpfe.** Doppelklicke bei den Bausteinen dreimal auf **{{MlTypButton}}** oder zieh ihn auf die Fläche.",
              "**三个按钮。** 在构件中双击三次 **{{MlTypButton}}**,或把它拖到画布上。",
              "**3 つのボタン。** パーツの **{{MlTypButton}}** を 3 回ダブルクリックするか、キャンバスへドラッグします。"),
             "h_tut_menue_3", T("Three new buttons next to each other, not yet evenly spaced.",
              "Drei neue Knöpfe nebeneinander, noch nicht gleichmäßig verteilt.",
              "三个新按钮彼此相邻,间距尚不均匀。",
              "隣り合った 3 つの新しいボタン。間隔はまだそろっていません。")),
            (T("**Side by side.** Select the three buttons (Ctrl+click) and click **{{MeLayoutWaagrecht}}** under Layout in the properties.",
               "**Nebeneinander.** Wähle die drei Knöpfe (Strg+Klick) und klicke in den Eigenschaften unter Layout auf **{{MeLayoutWaagrecht}}**.",
               "**并排。** 选中这三个按钮(Ctrl+单击),然后在属性的“布局”下单击 **{{MeLayoutWaagrecht}}**。",
               "**横に並べる。** 3 つのボタンを選び(Ctrl+クリック)、プロパティのレイアウトで **{{MeLayoutWaagrecht}}** をクリックします。"),
             "h_tut_menue_4", T("The buttons side by side with even spacing.", "Die Knöpfe nebeneinander mit gleichem Abstand.", "按钮等距并排。", "等間隔で横に並んだボタン。")),
            (T("**Glow on hover.** Select the first button, open the **Events** tab and click **{{MeEvLeuchten}}**.",
               "**Leuchten beim Hover.** Wähle den ersten Knopf, öffne den Reiter **Ereignisse** und klicke auf **{{MeEvLeuchten}}**.",
               "**悬停发光。** 选中第一个按钮,打开 **事件** 标签页,单击 **{{MeEvLeuchten}}**。",
               "**ホバーで光る。** 最初のボタンを選び、**イベント** タブを開いて **{{MeEvLeuchten}}** をクリックします。"),
             "h_tut_menue_5", T("The glow effect: show on hover, hide when the mouse leaves.", "Der Leucht-Effekt: zeigen beim Hover, verstecken beim Verlassen.",
                                "发光效果:悬停时显示,鼠标离开时隐藏。", "光る効果: ホバーで表示、離れると非表示。")),
            (T("**Save.** Click **{{MeSpeichern}}** - in a project it saves everything. **{{MePrjProjekt}} > {{MePrjExport}}** then packs the pk3 for the game.",
               "**Speichern.** Klicke auf **{{MeSpeichern}}** - im Projekt speichert das alles. **{{MePrjProjekt}} > {{MePrjExport}}** packt danach die pk3 fürs Spiel.",
               "**保存。** 单击 **{{MeSpeichern}}**——在项目中会保存全部内容。之后 **{{MePrjProjekt}} > {{MePrjExport}}** 会为游戏打包 pk3。",
               "**保存する。** **{{MeSpeichern}}** をクリックします。プロジェクトではすべてを保存します。その後 **{{MePrjProjekt}} > {{MePrjExport}}** でゲーム用の pk3 を作ります。"),
             "h_tut_menue_6", T("Saved: the project file and the menu in the project folder.", "Gespeichert: die Projektdatei und das Menü im Projektordner.",
                                "已保存:项目文件和项目文件夹中的菜单。", "保存済み: プロジェクトファイルとプロジェクトフォルダーのメニュー。")),
        ]),
    ],
}

SEITEN = [MENUE_EDITOR_M5B, TUTORIAL_MENUE]
