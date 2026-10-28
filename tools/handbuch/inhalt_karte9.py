"""Stufe 9 des Karten-Editors: die restlichen Karten-Werkzeuge (eigene Funktionen, nicht in
NetRadiant-Custom). Je Punkt ein Abschnitt; der Selbsttest gui/selbsttest_karte9.inc prueft
jeden (BHED_KARTE_NUR=stufe9).
"""
from inhalt_basis import T

KARTE_WERKZEUGE9 = {
    "id": "karte_werkzeuge9",
    "gruppe": "karte",
    "titel": T("Map editor: test in game and more tools", "Karten-Editor: im Spiel testen und weitere Werkzeuge",
               "地图编辑器:在游戏中测试及更多工具", "マップエディター:ゲーム内テストとその他のツール"),
    "kurz": T(
        "Start the game with the current map at the press of a button - without writing into the game folder.",
        "Das Spiel auf Knopfdruck mit der aktuellen Karte starten - ohne in den Spielordner zu schreiben.",
        "一键用当前地图启动游戏——无需写入游戏文件夹。",
        "ボタン一つで現在のマップを使ってゲームを起動します。ゲームのフォルダーには書き込みません。"),
    "bloecke": [
        ("h2", "imspiel", T("Test in game", "Im Spiel testen", "在游戏中测试", "ゲーム内でテスト")),
        ("p", T(
            "**{{KeLBuild}} > {{KeStMenue}}** lists the engine from the preferences and every game program found in the engine folder: Jedi Academy SP/MP, OpenJK, Movie Duels and, for Jedi Outcast maps, Jedi Outcast SP/MP and OpenJO. A click starts the chosen program with the current map (`+devmap`). The choice is remembered.",
            "**{{KeLBuild}} > {{KeStMenue}}** listet die Engine aus den Einstellungen und jedes Spielprogramm im Engine-Ordner: Jedi Academy SP/MP, OpenJK, Movie Duels und bei Jedi-Outcast-Karten Jedi Outcast SP/MP und OpenJO. Ein Klick startet das gewählte Programm mit der aktuellen Karte (`+devmap`). Die Wahl bleibt gespeichert.",
            "**{{KeLBuild}} > {{KeStMenue}}** 列出首选项中的引擎以及在引擎文件夹中找到的所有游戏程序:Jedi Academy SP/MP、OpenJK、Movie Duels;对于 Jedi Outcast 地图则是 Jedi Outcast SP/MP 和 OpenJO。单击即可用当前地图(`+devmap`)启动所选程序。所做的选择会被记住。",
            "**{{KeLBuild}} > {{KeStMenue}}** には、設定のエンジンと、エンジンのフォルダーで見つかったすべてのゲームプログラム(Jedi Academy SP/MP、OpenJK、Movie Duels、Jedi Outcast のマップなら Jedi Outcast SP/MP と OpenJO)が並びます。クリックすると、選んだプログラムが現在のマップ(`+devmap`)で起動します。選択は記憶されます。")),
        ("list", [
            T("**Which .bsp:** the quick bake of the current map if it is up to date (**{{KeBackenMenue}}**), otherwise a .bsp that is newer than the saved, unchanged map, otherwise the program builds first with the first **[Test]** entry of the Build menu and starts the game when the build is done.",
              "**Welche .bsp:** das Schnell-Backen der aktuellen Karte, wenn es aktuell ist (**{{KeBackenMenue}}**), sonst eine .bsp, die neuer ist als die gespeicherte, unveränderte Karte, sonst baut das Programm zuerst mit dem ersten **[Test]**-Eintrag des Build-Menüs und startet das Spiel, wenn der Bau fertig ist.",
              "**使用哪个 .bsp:**如果当前地图的快速烘焙是最新的(**{{KeBackenMenue}}**)就用它;否则使用比已保存且未更改的地图更新的 .bsp;再否则程序先用 Build 菜单中第一个 **[Test]** 条目编译,编译完成后启动游戏。",
              "**どの .bsp を使うか:**現在のマップのクイックベイクが最新ならそれ(**{{KeBackenMenue}}**)、そうでなければ保存済みで未変更のマップより新しい .bsp、それもなければ Build メニューの最初の **[Test]** 項目でまずビルドし、終わったらゲームを起動します。"),
            T("**Where it goes:** the .bsp is packed as `zzz_jkstudio_test.pk3` into the folder `spieltest` next to the program settings - never into the game folder. The game learns about the folder from a start parameter: `fs_cdpath` (all engines of this family; your game settings stay as they are) or, if the game already has a map with the same name, `fs_homepath` (Jedi Academy MP, Jedi Outcast MP, OpenJK, Movie Duels), because that folder takes precedence. Which parameter a program knows is read from its .exe.",
              "**Wohin sie kommt:** die .bsp wird als `zzz_jkstudio_test.pk3` in den Ordner `spieltest` neben den Programmeinstellungen gepackt - nie in den Spielordner. Das Spiel erfährt den Ordner über einen Startparameter: `fs_cdpath` (alle Engines dieser Familie; deine Spieleinstellungen bleiben, wie sie sind) oder, wenn das Spiel schon eine Karte mit diesem Namen hat, `fs_homepath` (Jedi Academy MP, Jedi Outcast MP, OpenJK, Movie Duels), weil dieser Ordner Vorrang hat. Welchen Parameter ein Programm kennt, liest das Programm aus seiner .exe.",
              "**放到哪里:**.bsp 会被打包为 `zzz_jkstudio_test.pk3`,放在程序设置旁边的 `spieltest` 文件夹中——绝不会放入游戏文件夹。游戏通过启动参数得知该文件夹:`fs_cdpath`(该系列的所有引擎;你的游戏设置保持不变),或者当游戏已有同名地图时使用 `fs_homepath`(Jedi Academy MP、Jedi Outcast MP、OpenJK、Movie Duels),因为该文件夹优先。程序支持哪个参数是从其 .exe 中读取的。",
              "**置き場所:**.bsp は `zzz_jkstudio_test.pk3` として、プログラムの設定の隣にある `spieltest` フォルダーにまとめられます。ゲームのフォルダーには決して置きません。ゲームは起動パラメーターでそのフォルダーを知ります:`fs_cdpath`(この系統のすべてのエンジン。ゲームの設定はそのまま)か、同じ名前のマップがすでにある場合は優先される `fs_homepath`(Jedi Academy MP、Jedi Outcast MP、OpenJK、Movie Duels)です。プログラムがどちらのパラメーターを知っているかは .exe から読み取ります。"),
            T("**Only if neither works** (for example Jedi Academy SP with a map name the game already has), the program asks before it writes the archive into the game: into a mod folder `jkstudio_test` (started with `+set fs_game jkstudio_test`), or into the folder of the active mod. You can delete the file later.",
              "**Nur wenn beides nicht geht** (etwa Jedi Academy SP mit einem Kartennamen, den das Spiel schon hat), fragt das Programm, bevor es das Archiv ins Spiel schreibt: in einen Mod-Ordner `jkstudio_test` (gestartet mit `+set fs_game jkstudio_test`) bzw. in den Ordner der aktiven Mod. Die Datei kannst du später löschen.",
              "**只有两者都不可用时**(例如 Jedi Academy SP 且地图名称游戏中已存在),程序才会在把存档写入游戏前询问:写入模组文件夹 `jkstudio_test`(以 `+set fs_game jkstudio_test` 启动),或写入当前模组的文件夹。之后可以删除该文件。",
              "**どちらも使えない場合だけ**(たとえば Jedi Academy SP で、すでにある名前のマップ)、アーカイブをゲームに書き込む前に確認します:MOD フォルダー `jkstudio_test`(`+set fs_game jkstudio_test` で起動)か、有効な MOD のフォルダーです。ファイルは後で削除できます。"),
            T("The console shows the way that was chosen and the whole command line. Map names with spaces cannot be loaded with `devmap`; save the map under a name without spaces.",
              "Die Konsole zeigt den gewählten Weg und die ganze Befehlszeile. Kartennamen mit Leerzeichen lassen sich mit `devmap` nicht laden; speichere die Karte unter einem Namen ohne Leerzeichen.",
              "控制台会显示所选方式和完整的命令行。带空格的地图名称无法用 `devmap` 加载;请用不含空格的名称保存地图。",
              "コンソールには選ばれた方法とコマンドライン全体が表示されます。空白を含むマップ名は `devmap` で読み込めないので、空白のない名前で保存してください。"),
        ]),
        ("note", T(
            "Not in NetRadiant-Custom. NetRadiant-Custom's own **Run Engine After Compile** (preferences, page Build) stays as it is: it starts the engine from the game folder after every build.",
            "Nicht in NetRadiant-Custom. Das eigene **Nach dem Bauen das Spiel starten** von NetRadiant-Custom (Einstellungen, Seite Build) bleibt, wie es ist: es startet die Engine nach jedem Bau aus dem Spielordner.",
            "NetRadiant-Custom 中没有。NetRadiant-Custom 自带的 **编译后运行游戏**(首选项,Build 页)保持不变:它在每次编译后从游戏文件夹启动引擎。",
            "NetRadiant-Custom にはありません。NetRadiant-Custom 自身の **コンパイル後にゲームを起動**(設定の Build ページ)はそのままで、ビルドのたびにゲームのフォルダーからエンジンを起動します。")),
    ],
}

SEITEN = [KARTE_WERKZEUGE9]
