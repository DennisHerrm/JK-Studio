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
        "Start the game with the current map at the press of a button, and turn models from Blender and other games into brushes.",
        "Das Spiel auf Knopfdruck mit der aktuellen Karte starten und Modelle aus Blender und anderen Spielen zu Brushes machen.",
        "一键用当前地图启动游戏,并把来自 Blender 和其他游戏的模型转换为画刷。",
        "ボタン一つで現在のマップを使ってゲームを起動し、Blender やほかのゲームのモデルをブラシにします。"),
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
        ("h2", "geometrie", T("Geometry to brushes", "Geometrie zu Brushes", "几何体转画刷", "ジオメトリをブラシに")),
        ("p", T(
            "**{{KeLFile}} > {{KeGiMenue}}** reads a model (.obj, .fbx, .glb or .gltf, for example from Blender) and turns it into brushes. A window lists the parts of the file; for each part choose **{{KeGiAuto}}**, **{{KeGiPinsel}}** or **{{KeGiModellClip}}** - the column **{{KeGiErgebnis}}** shows at once how many brushes it becomes and whether they are detail. **{{KeGiEinfuegen}}** inserts everything as one undo step and selects it; then the optimizer opens.",
            "**{{KeLFile}} > {{KeGiMenue}}** liest ein Modell (.obj, .fbx, .glb oder .gltf, etwa aus Blender) und macht Brushes daraus. Ein Fenster listet die Teile der Datei; wähle je Teil **{{KeGiAuto}}**, **{{KeGiPinsel}}** oder **{{KeGiModellClip}}** - die Spalte **{{KeGiErgebnis}}** zeigt sofort, wie viele Brushes daraus werden und ob sie detail sind. **{{KeGiEinfuegen}}** setzt alles als einen Undo-Schritt ein und wählt es; danach öffnet sich der Optimierer.",
            "**{{KeLFile}} > {{KeGiMenue}}** 读取模型(.obj、.fbx、.glb 或 .gltf,例如来自 Blender)并把它转换为画刷。窗口会列出文件中的部件;为每个部件选择 **{{KeGiAuto}}**、**{{KeGiPinsel}}** 或 **{{KeGiModellClip}}**——**{{KeGiErgebnis}}** 列会立即显示会生成多少个画刷以及它们是否为 detail。**{{KeGiEinfuegen}}** 以一步撤销插入全部内容并选中;之后会打开优化器。",
            "**{{KeLFile}} > {{KeGiMenue}}** はモデル(.obj、.fbx、.glb、.gltf。たとえば Blender から)を読み込み、ブラシにします。ウィンドウにファイルの部品が並ぶので、部品ごとに **{{KeGiAuto}}**・**{{KeGiPinsel}}**・**{{KeGiModellClip}}** を選びます。**{{KeGiErgebnis}}** の列に、ブラシがいくつできるか、detail かどうかがすぐ表示されます。**{{KeGiEinfuegen}}** で全部を 1 回の「元に戻す」単位で挿入して選択し、その後で最適化が開きます。")),
        ("list", [
            T("**Closed parts** are split into convex brushes (a BSP over the faces of the model, like q3map2 builds one). Faces that lie in one plane become one brush side. Sides on the surface of the model get its material as shader (**{{KeGiVorsatz}}** + material name) and its UV as texture alignment; sides inside the model, which nobody sees, get caulk.",
              "**Geschlossene Teile** werden in konvexe Brushes zerlegt (ein BSP über die Flächen des Modells, wie q3map2 eines baut). Flächen in einer Ebene werden eine Brush-Seite. Seiten auf der Oberfläche des Modells bekommen dessen Material als Shader (**{{KeGiVorsatz}}** + Materialname) und seine UV als Texturausrichtung; Seiten im Inneren, die niemand sieht, bekommen caulk.",
              "**封闭部件**会被分解为凸画刷(对模型的面构建 BSP,与 q3map2 的做法相同)。同一平面上的面合并为一个画刷面。位于模型表面的画刷面使用其材质作为着色器(**{{KeGiVorsatz}}** + 材质名),并以其 UV 作为纹理对齐;模型内部看不到的面使用 caulk。",
              "**閉じた部品**は凸ブラシに分解されます(q3map2 と同じように、モデルの面で BSP を作ります)。同じ平面上の面は 1 つのブラシ面になります。モデルの表面にある面にはその素材がシェーダー(**{{KeGiVorsatz}}** + 素材名)として付き、UV がテクスチャの向きになります。内側の誰にも見えない面は caulk です。"),
            T("**Open parts** (single surfaces, decoration) become thin brushes behind each flat face; small and open parts become detail, so they do not cut the map into BSP leaves.",
              "**Offene Teile** (einzelne Flächen, Deko) werden dünne Brushes hinter jeder ebenen Fläche; kleine und offene Teile werden detail und zerschneiden die Karte so nicht in BSP-Blätter.",
              "**开放部件**(单个面、装饰)会在每个平面后方生成薄画刷;小部件和开放部件设为 detail,这样不会把地图切成 BSP 叶。",
              "**開いた部品**(単独の面や装飾)は、平らな面ごとにその裏側の薄いブラシになります。小さい部品と開いた部品は detail になるので、マップを BSP の葉に切り刻みません。"),
            T("**{{KeGiModellClip}}** keeps the part as a model: it is written as .obj into the program's import folder (next to the settings, never into the game), a misc_model points to it, and clip brushes from the same decomposition make it solid. The editor and q3map2 (`-fs_pakpath`) search this folder. **{{KeGiAuto}}** takes brushes for closed parts with at most 256 brushes, otherwise model + clip.",
              "**{{KeGiModellClip}}** behält den Teil als Modell: er wird als .obj in den Import-Ordner des Programms geschrieben (neben den Einstellungen, nie ins Spiel), ein misc_model zeigt darauf, und Clip-Brushes aus derselben Zerlegung machen ihn fest. Editor und q3map2 (`-fs_pakpath`) durchsuchen diesen Ordner. **{{KeGiAuto}}** nimmt Brushes für geschlossene Teile mit höchstens 256 Brushes, sonst Modell + Clip.",
              "**{{KeGiModellClip}}** 将部件保留为模型:它以 .obj 写入程序的导入文件夹(在设置旁边,绝不写入游戏),由 misc_model 引用,并用同一分解得到的 clip 画刷使其成为实体。编辑器和 q3map2(`-fs_pakpath`)都会搜索该文件夹。**{{KeGiAuto}}** 对最多 256 个画刷的封闭部件使用画刷,否则使用模型 + 碰撞。",
              "**{{KeGiModellClip}}** は部品をモデルのまま残します。.obj としてプログラムのインポートフォルダー(設定の隣。ゲームには書き込みません)に書き出し、misc_model がそれを指し、同じ分解から作った clip ブラシで当たり判定を付けます。エディターと q3map2(`-fs_pakpath`)はこのフォルダーを検索します。**{{KeGiAuto}}** は、ブラシが 256 個以下の閉じた部品ならブラシ、それ以外はモデル + クリップを選びます。"),
            T("**{{KeGiSkala}}** and **{{KeGiYOben}}**: Blender and glTF use Y as up, the game Z; one Blender metre is one unit unless you scale it. A **.bsp** of another id Tech 3 game (Quake III, Elite Force, Return to Castle Wolfenstein, Enemy Territory, Warsow) opens like **{{KeLOpenBsp}}**, with the reader of that game.",
              "**{{KeGiSkala}}** und **{{KeGiYOben}}**: Blender und glTF nehmen Y als oben, das Spiel Z; ein Blender-Meter ist eine Einheit, wenn du nicht skalierst. Eine **.bsp** eines anderen id-Tech-3-Spiels (Quake III, Elite Force, Return to Castle Wolfenstein, Enemy Territory, Warsow) öffnet wie **{{KeLOpenBsp}}**, mit dem Leser dieses Spiels.",
              "**{{KeGiSkala}}** 和 **{{KeGiYOben}}**:Blender 和 glTF 以 Y 轴为上,游戏以 Z 轴为上;不缩放时一个 Blender 米等于一个单位。其他 id Tech 3 游戏(Quake III、Elite Force、Return to Castle Wolfenstein、Enemy Territory、Warsow)的 **.bsp** 会像 **{{KeLOpenBsp}}** 一样用该游戏的读取器打开。",
              "**{{KeGiSkala}}** と **{{KeGiYOben}}**:Blender と glTF は Y が上、ゲームは Z が上です。スケールしなければ Blender の 1 メートルが 1 ユニットです。ほかの id Tech 3 ゲーム(Quake III、Elite Force、Return to Castle Wolfenstein、Enemy Territory、Warsow)の **.bsp** は **{{KeLOpenBsp}}** と同じように、そのゲームの読み込みで開きます。"),
        ]),
        ("h2", "ziehen", T("Drag a model into the map", "Ein Modell in die Karte ziehen", "把模型拖入地图", "モデルをマップにドラッグする")),
        ("p", T(
            "Drag an .obj, .fbx, .glb or .gltf file from the Explorer onto the map editor. The program converts it to an MD3 with its own shader and copies the textures (embedded in the file or next to it); all of it goes into the import folder next to the settings, never into the game. A misc_model appears under the mouse pointer in the 2D view (or in front of the camera) and is selected; one undo removes it. Models larger than 511 units are scaled down in the MD3 and get `modelscale`.",
            "Ziehe eine .obj-, .fbx-, .glb- oder .gltf-Datei aus dem Explorer auf den Karten-Editor. Das Programm macht daraus ein MD3 mit eigenem Shader und kopiert die Texturen (in der Datei eingebettet oder daneben); alles kommt in den Import-Ordner neben den Einstellungen, nie ins Spiel. Ein misc_model erscheint unter dem Mauszeiger in der 2D-Ansicht (sonst vor der Kamera) und ist gewählt; ein Undo nimmt es zurück. Modelle über 511 Einheiten werden im MD3 verkleinert und bekommen `modelscale`.",
            "把 .obj、.fbx、.glb 或 .gltf 文件从资源管理器拖到地图编辑器上。程序会把它转换为带自己着色器的 MD3,并复制纹理(嵌入在文件中或位于文件旁);所有内容都放入设置旁的导入文件夹,绝不写入游戏。一个 misc_model 会出现在二维视图中的鼠标指针下(否则在摄像机前方)并被选中;撤销一次即可移除。超过 511 单位的模型会在 MD3 中缩小并获得 `modelscale`。",
            ".obj・.fbx・.glb・.gltf ファイルをエクスプローラーからマップエディターへドラッグします。プログラムが独自のシェーダー付きの MD3 に変換し、テクスチャ(ファイルに埋め込まれたもの、または隣にあるもの)をコピーします。すべて設定の隣のインポートフォルダーに入り、ゲームには書き込みません。2D ビューのマウスポインターの下(それ以外ではカメラの前)に misc_model が現れて選択されます。元に戻す 1 回で消えます。511 ユニットを超えるモデルは MD3 内で縮小され、`modelscale` が付きます。")),
        ("note", T(
            "**{{KeStMenue}}** packs the import folder into the test archive, so the game finds the models, shaders and textures. For a release, copy the folder `models/import` and the `scripts` of the import folder into your mod or pk3.",
            "**{{KeStMenue}}** packt den Import-Ordner ins Testarchiv, damit das Spiel Modelle, Shader und Texturen findet. Für eine Veröffentlichung kopiere den Ordner `models/import` und die `scripts` des Import-Ordners in deine Mod oder .pk3.",
            "**{{KeStMenue}}** 会把导入文件夹打包进测试存档,使游戏能找到模型、着色器和纹理。发布时,请把导入文件夹中的 `models/import` 文件夹和 `scripts` 复制到你的模组或 pk3 中。",
            "**{{KeStMenue}}** はインポートフォルダーをテスト用アーカイブに入れるので、ゲームがモデル・シェーダー・テクスチャを見つけられます。公開するときは、インポートフォルダーの `models/import` フォルダーと `scripts` を MOD または pk3 にコピーしてください。")),
    ],
}

SEITEN = [KARTE_WERKZEUGE9]
