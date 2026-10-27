"""Karte: Werkzeuge der Stufe 8b - Terrain Generator, freie Split-Fenster, Im Editor
spielen, Logik-Ansicht, Ansichtsmodi der Kamera, Spiegel-Modus, Undo-Verlauf,
misc_model aus allen Modellordnern und die (abgeschalteten) GPU-Schattentests.
Die Bilder nimmt BHED_HB_NUR=karte8b auf (gui/selbsttest.cpp karte8bHandbuchSchritte).
"""
from inhalt_basis import T

KARTE_WERKZEUGE8B = {
    "id": "karte_werkzeuge8b",
    "gruppe": "karte",
    "titel": T("Map editor: terrain, playing, logic, mirror and more", "Karten-Editor: Terrain, Spielen, Logik, Spiegel und mehr",
               "地图编辑器:地形、试玩、逻辑、镜像及更多", "マップエディター:地形・プレイ・ロジック・ミラーなど"),
    "kurz": T(
        "Generate terrain, walk through the map without compiling, see the links between entities, check lightmap density and collision, edit symmetric maps with the mirror mode and jump through the undo history.",
        "Terrain erzeugen, ohne Kompilieren durch die Karte laufen, die Verknüpfungen der Entities sehen, Lightmap-Dichte und Kollision prüfen, symmetrische Karten mit dem Spiegel-Modus bearbeiten und durch den Undo-Verlauf springen.",
        "生成地形,无需编译即可在地图中行走,查看实体之间的连接,检查光照贴图密度和碰撞,用镜像模式编辑对称地图,并在撤销历史中跳转。",
        "地形を生成し、コンパイルせずにマップの中を歩き、エンティティ同士のつながりを確認し、ライトマップ密度と衝突を調べ、ミラーモードで対称なマップを編集し、元に戻す履歴の中を移動します。"),
    "bloecke": [
        ("p", T(
            "Everything on this page except the Terrain Generator is **not in NetRadiant-Custom**: each function has its own menu entry with that hint, and with all of them off the editor behaves like NetRadiant-Custom. The new entries are at the end of **{{KeLView}}**, above the program settings.",
            "Alles auf dieser Seite außer dem Terrain Generator gibt es **nicht in NetRadiant-Custom**: jede Funktion hat ihren eigenen Menüeintrag mit diesem Hinweis, und sind alle aus, verhält sich der Editor wie NetRadiant-Custom. Die neuen Einträge stehen am Ende von **{{KeLView}}**, über den Programm-Einstellungen.",
            "本页除 Terrain Generator 外的所有功能都**不在 NetRadiant-Custom 中**:每个功能都有带此提示的独立菜单项;全部关闭时,编辑器的行为与 NetRadiant-Custom 相同。新菜单项位于 **{{KeLView}}** 的末尾、程序设置的上方。",
            "このページの機能は Terrain Generator を除いてすべて **NetRadiant-Custom にはありません**。どの機能にもその旨を示す専用のメニュー項目があり、すべてオフならエディターは NetRadiant-Custom と同じように動きます。新しい項目は **{{KeLView}}** の最後、プログラム設定の上にあります。")),
        ("img", "h_karte8b_menue", T(
            "{{KeLView}}: {{KeSpMenue}}, {{KeSpSpawn}}, {{KeAmMenue}}, {{KeLgMenue}}, {{KeSgMenue}} and {{KeUlMenue}}.",
            "{{KeLView}}: {{KeSpMenue}}, {{KeSpSpawn}}, {{KeAmMenue}}, {{KeLgMenue}}, {{KeSgMenue}} und {{KeUlMenue}}.",
            "{{KeLView}}:{{KeSpMenue}}、{{KeSpSpawn}}、{{KeAmMenue}}、{{KeLgMenue}}、{{KeSgMenue}} 和 {{KeUlMenue}}。",
            "{{KeLView}}:{{KeSpMenue}}、{{KeSpSpawn}}、{{KeAmMenue}}、{{KeLgMenue}}、{{KeSgMenue}}、{{KeUlMenue}}。")),

        ("h2", "terrain", T("Terrain Generator", "Terrain Generator", "Terrain Generator(地形生成器)", "Terrain Generator(地形ジェネレーター)")),
        ("p", T(
            "**Plugins > TerrainGenerator > Generate Terrain** is the plugin from the current NetRadiant-Custom source (master, after the release 20260114), ported line by line: the same heights, triangles and textures as the original for the same values. The dialog stays open while you work in the views.",
            "**Plugins > TerrainGenerator > Generate Terrain** ist das Plugin aus dem aktuellen Quelltext von NetRadiant-Custom (master, nach dem Release 20260114), Zeile für Zeile übernommen: bei gleichen Werten dieselben Höhen, Dreiecke und Texturen wie das Original. Der Dialog bleibt offen, während du in den Ansichten arbeitest.",
            "**Plugins > TerrainGenerator > Generate Terrain** 是 NetRadiant-Custom 当前源代码(master,20260114 版本之后)中的插件,逐行移植:相同的数值得到与原版相同的高度、三角形和纹理。在视图中工作时对话框保持打开。",
            "**Plugins > TerrainGenerator > Generate Terrain** は NetRadiant-Custom の現在のソース(master、リリース 20260114 より後)にあるプラグインを 1 行ずつ移植したものです。同じ値なら元のものと同じ高さ・三角形・テクスチャになります。ビューで作業している間もダイアログは開いたままです。")),
        ("imgs", [
            ("h_karte8b_terrain_dialog", T("The dialog with **{{KeTgUseSel}}**: the size comes from the selected brush.",
                                            "Der Dialog mit **{{KeTgUseSel}}**: die Größe kommt vom gewählten Brush.",
                                            "选择 **{{KeTgUseSel}}** 的对话框:尺寸来自所选画刷。",
                                            "**{{KeTgUseSel}}** を選んだダイアログ:大きさは選択したブラシから取られます。")),
            ("h_karte8b_terrain", T("A **{{KeTgShHill}}** made from the selected brush - one func_group of detail brushes.",
                                    "Ein **{{KeTgShHill}}** aus dem gewählten Brush - eine func_group aus Detail-Brushes.",
                                    "由所选画刷生成的 **{{KeTgShHill}}**——一个由 detail 画刷组成的 func_group。",
                                    "選択したブラシから作った **{{KeTgShHill}}**。detail ブラシでできた func_group です。")),
        ]),
        ("list", [
            T("**{{KeTgTarget}}** {{KeTgUseSel}} takes the size of the selected brush and replaces it; {{KeTgManual}} builds the terrain around the origin.",
              "**{{KeTgTarget}}** {{KeTgUseSel}} nimmt die Größe des gewählten Brushes und ersetzt ihn; {{KeTgManual}} baut das Terrain um den Ursprung.",
              "**{{KeTgTarget}}** {{KeTgUseSel}} 使用所选画刷的尺寸并替换它;{{KeTgManual}} 在原点周围生成地形。",
              "**{{KeTgTarget}}** {{KeTgUseSel}} は選択したブラシの大きさを使ってそれを置き換え、{{KeTgManual}} は原点の周りに地形を作ります。"),
            T("**{{KeTgBaseShape}}** (hill, crater, ridge, slope, volcano, valley, tunnel ...), **{{KeTgNoiseType}}**, **{{KeTgVariance}}**, **{{KeTgFrequency}}**, terraces and the grid step as in the original; **{{KeTgGenerate}}** is one undo step.",
              "**{{KeTgBaseShape}}** (Hügel, Krater, Grat, Hang, Vulkan, Tal, Tunnel ...), **{{KeTgNoiseType}}**, **{{KeTgVariance}}**, **{{KeTgFrequency}}**, Terrassen und das Raster wie im Original; **{{KeTgGenerate}}** ist ein Rückgängig-Schritt.",
              "**{{KeTgBaseShape}}**(山丘、陨石坑、山脊、斜坡、火山、山谷、隧道……)、**{{KeTgNoiseType}}**、**{{KeTgVariance}}**、**{{KeTgFrequency}}**、阶梯和网格步长与原版相同;**{{KeTgGenerate}}** 是一个撤销步骤。",
              "**{{KeTgBaseShape}}**(丘、クレーター、尾根、斜面、火山、谷、トンネルなど)、**{{KeTgNoiseType}}**、**{{KeTgVariance}}**、**{{KeTgFrequency}}**、段差、グリッド間隔は元のものと同じです。**{{KeTgGenerate}}** は元に戻す 1 ステップです。"),
            T("Own addition: **{{KeTgSeed}} {{KeTgSeedFix}}** starts the random numbers with a fixed value - the same values then give the same terrain again. Off (default), the random sequence runs on as in NetRadiant-Custom.",
              "Eigene Ergänzung: **{{KeTgSeed}} {{KeTgSeedFix}}** startet die Zufallszahlen mit einem festen Wert - dieselben Werte geben dann wieder dasselbe Terrain. Aus (Vorgabe) läuft die Zufallsfolge weiter wie in NetRadiant-Custom.",
              "自有补充:**{{KeTgSeed}} {{KeTgSeedFix}}** 用固定值开始随机数——相同的数值会再次得到相同的地形。关闭时(默认),随机序列像 NetRadiant-Custom 中一样继续。",
              "独自の追加:**{{KeTgSeed}} {{KeTgSeedFix}}** は乱数を固定値から始めます。同じ値なら同じ地形がもう一度できます。オフ(既定)では NetRadiant-Custom と同じく乱数の列が続きます。"),
        ]),

        ("h2", "split", T("Free split windows", "Freie Split-Fenster", "自由分屏窗口", "自由な分割ウィンドウ")),
        ("p", T(
            "**Preferences > Interface > Layout > {{KeSfSchalter}}** (layout Split): every one of the four windows gets a small menu in its corner - camera, XY, XZ or YZ, also two windows with the same view. Ctrl+Tab changes the type of the active 2D window, the splitter positions are kept. Off (default), the four windows are fixed as in NetRadiant-Custom.",
            "**Preferences > Interface > Layout > {{KeSfSchalter}}** (Layout Split): jedes der vier Fenster bekommt in seiner Ecke ein kleines Menü - Kamera, XY, XZ oder YZ, auch zwei Fenster mit derselben Ansicht. Strg+Tab wechselt den Typ des aktiven 2D-Fensters, die Teiler behalten ihre Lage. Aus (Vorgabe) sind die vier Fenster fest wie in NetRadiant-Custom.",
            "**Preferences > Interface > Layout > {{KeSfSchalter}}**(Split 布局):四个窗口的角落都会出现一个小菜单——摄像机、XY、XZ 或 YZ,也可以两个窗口显示同一视图。Ctrl+Tab 切换当前 2D 窗口的类型,分隔条位置会保留。关闭时(默认),四个窗口像 NetRadiant-Custom 中一样固定。",
            "**Preferences > Interface > Layout > {{KeSfSchalter}}**(レイアウト Split):4 つのウィンドウの角に小さなメニューが付き、カメラ・XY・XZ・YZ を選べます。同じビューを 2 つのウィンドウに出すこともできます。Ctrl+Tab でアクティブな 2D ウィンドウの種類が変わり、仕切りの位置は保たれます。オフ(既定)では NetRadiant-Custom と同じく 4 つのウィンドウは固定です。")),

        ("h2", "spielen", T("Play in the editor", "Im Editor spielen", "在编辑器中试玩", "エディター内でプレイ")),
        ("img", "h_karte8b_spielen", T(
            "**{{KeSpMenue}}** (F6): walking through the open map - position and speed at the top, the keys at the bottom.",
            "**{{KeSpMenue}}** (F6): durch die offene Karte laufen - oben Ort und Geschwindigkeit, unten die Tasten.",
            "**{{KeSpMenue}}**(F6):在打开的地图中行走——上方显示位置和速度,下方显示按键。",
            "**{{KeSpMenue}}**(F6):開いているマップの中を歩きます。上に位置と速度、下にキーが表示されます。")),
        ("list", [
            T("No compile: the collision comes straight from the brushes and patches with the contents q3map2 would give them (surfaceparms of the game, clip brushes, nonsolid, water ...). Walking, jumping, stairs and slopes use the port of the game's movement code (Pmove) - the same as the preview of the program: JKA and Movie Duels, or the Jedi Outcast rules.",
              "Ohne Kompilieren: die Kollision kommt direkt aus den Brushes und Patches mit den Inhalten, die q3map2 ihnen gäbe (surfaceparms des Spiels, Clip-Brushes, nonsolid, Wasser ...). Gehen, Springen, Treppen und Schrägen nutzen den Port der Bewegung des Spiels (Pmove) - derselbe wie in der Vorschau des Programms: JKA und Movie Duels oder die Regeln von Jedi Outcast.",
              "无需编译:碰撞直接来自画刷和面片,其内容与 q3map2 会赋予的相同(游戏的 surfaceparm、clip 画刷、nonsolid、水……)。行走、跳跃、楼梯和斜坡使用游戏移动代码(Pmove)的移植——与程序预览中的相同:JKA 和 Movie Duels,或 Jedi Outcast 的规则。",
              "コンパイル不要:衝突はブラシとパッチから直接作られ、その内容は q3map2 が与えるもの(ゲームの surfaceparm、clip ブラシ、nonsolid、水など)と同じです。歩く・跳ぶ・階段・坂はゲームの移動コード(Pmove)の移植を使います。プログラムのプレビューと同じで、JKA と Movie Duels、または Jedi Outcast のルールです。"),
            T("W/A/S/D move, Space jumps, C or Ctrl crouches, Shift walks, the mouse looks, N switches noclip, **Esc ends** and puts the camera back where it was.",
              "W/A/S/D bewegen, Leertaste springt, C oder Strg duckt, Umschalt geht, die Maus schaut, N schaltet Noclip, **Esc beendet** und stellt die Kamera zurück, wo sie war.",
              "W/A/S/D 移动,空格跳跃,C 或 Ctrl 蹲下,Shift 慢走,鼠标观察,N 切换穿墙模式,**Esc 结束**并把摄像机放回原处。",
              "W/A/S/D で移動、スペースでジャンプ、C か Ctrl でしゃがみ、Shift で歩き、マウスで見回し、N でノークリップ切り替え、**Esc で終了**してカメラを元の位置に戻します。"),
            T("**{{KeSpSpawn}}**: on, the start is the first info_player_start (its origin + 9 and its angle, as the game picks it); off, the start is the camera.",
              "**{{KeSpSpawn}}**: an, ist der Start das erste info_player_start (sein origin + 9 und sein angle, wie das Spiel ihn wählt); aus, ist der Start die Kamera.",
              "**{{KeSpSpawn}}**:开启时从第一个 info_player_start 开始(其 origin + 9 和 angle,与游戏的选择相同);关闭时从摄像机位置开始。",
              "**{{KeSpSpawn}}**:オンなら最初の info_player_start(その origin + 9 と angle。ゲームと同じ選び方)から、オフならカメラの位置から始まります。"),
        ]),
        ("note", T(
            "Only the player moves: no doors, movers, triggers, NPCs or weapons. The map is not changed - playing is not an undo step.",
            "Nur der Spieler bewegt sich: keine Türen, Mover, Trigger, NPCs oder Waffen. Die Karte wird nicht geändert - Spielen ist kein Rückgängig-Schritt.",
            "只有玩家会移动:没有门、移动体、触发器、NPC 或武器。地图不会被修改——试玩不是撤销步骤。",
            "動くのはプレイヤーだけで、ドア・ムーバー・トリガー・NPC・武器は動きません。マップは変更されず、プレイは元に戻すステップになりません。")),

        ("h2", "logik", T("Logic view", "Logik-Ansicht", "逻辑视图", "ロジックビュー")),
        ("img", "h_karte8b_logik", T(
            "**{{KeLgTitel}}**: trigger_multiple fires target_relay, which opens the door and spawns the NPC; the NPC's scripts below it.",
            "**{{KeLgTitel}}**: trigger_multiple löst target_relay aus, das die Tür öffnet und den NPC erscheinen lässt; darunter die Skripte des NPCs.",
            "**{{KeLgTitel}}**:trigger_multiple 触发 target_relay,后者打开门并生成 NPC;其下方是 NPC 的脚本。",
            "**{{KeLgTitel}}**:trigger_multiple が target_relay を起動し、それがドアを開けて NPC を出現させます。その下に NPC のスクリプトがあります。")),
        ("list", [
            T("**{{KeLgMenue}}** draws every link between the entities as a graph from left to right: target, target2 ... target5, NPC_target, the targets of doors and every *target* key of the entity definitions; the script keys (spawnscript, usescript, deathscript ...) are listed in the box of the entity.",
              "**{{KeLgMenue}}** zeichnet jede Verknüpfung zwischen den Entities als Graph von links nach rechts: target, target2 ... target5, NPC_target, die Ziele der Türen und jeden *target*-Schlüssel der Entity-Definitionen; die Skript-Schlüssel (spawnscript, usescript, deathscript ...) stehen im Kasten der Entity.",
              "**{{KeLgMenue}}** 将实体之间的每个连接从左到右绘制成图:target、target2……target5、NPC_target、门的目标以及实体定义中的每个 *target* 键;脚本键(spawnscript、usescript、deathscript……)列在实体的框中。",
              "**{{KeLgMenue}}** はエンティティ間のつながりをすべて左から右へのグラフとして描きます:target、target2 … target5、NPC_target、ドアのターゲット、エンティティ定義のすべての *target* キー。スクリプトキー(spawnscript、usescript、deathscript など)はエンティティの枠の中に並びます。"),
            T("A click on a box selects the entity and centers the 2D views and the camera on it; a double-click on a script line opens the ICARUS script in the script editor. Mouse wheel zooms, right or middle mouse button moves.",
              "Ein Klick auf einen Kasten wählt die Entity und zentriert die 2D-Ansichten und die Kamera darauf; ein Doppelklick auf eine Skript-Zeile öffnet das ICARUS-Skript im Skript-Editor. Das Mausrad zoomt, die rechte oder mittlere Maustaste verschiebt.",
              "单击某个框会选中该实体,并让 2D 视图和摄像机居中显示它;双击脚本行会在脚本编辑器中打开 ICARUS 脚本。鼠标滚轮缩放,右键或中键拖动平移。",
              "枠をクリックするとそのエンティティが選択され、2D ビューとカメラがその位置に合わせられます。スクリプトの行をダブルクリックすると ICARUS スクリプトがスクリプトエディターで開きます。マウスホイールでズーム、右または中ボタンで移動します。"),
            T("A target without a matching targetname appears as a red box - a link that leads nowhere in the game.",
              "Ein Ziel ohne passenden targetname erscheint als roter Kasten - eine Verknüpfung, die im Spiel ins Leere führt.",
              "没有对应 targetname 的目标显示为红框——在游戏中这是一个无效的连接。",
              "対応する targetname のないターゲットは赤い枠で表示されます。ゲームではどこにもつながらないリンクです。"),
        ]),

        ("h2", "ansichtsmodi", T("Camera view modes", "Ansichtsmodi der Kamera", "摄像机视图模式", "カメラの表示モード")),
        ("imgs", [
            ("h_karte8b_dichte", T("**{{KeAmDichte}}**: one square is one lightmap luxel.", "**{{KeAmDichte}}**: ein Feld ist ein Lightmap-Luxel.",
                                   "**{{KeAmDichte}}**:一个方格就是一个光照贴图像素。", "**{{KeAmDichte}}**:1 マスがライトマップの 1 ルクセルです。")),
            ("h_karte8b_kollision", T("**{{KeAmKollision}}**: the trigger brush in green - it does not block the player.", "**{{KeAmKollision}}**: der Trigger-Brush grün - er hält den Spieler nicht auf.",
                                      "**{{KeAmKollision}}**:触发器画刷为绿色——它不会阻挡玩家。", "**{{KeAmKollision}}**:トリガーのブラシは緑。プレイヤーを止めません。")),
        ]),
        ("list", [
            T("**{{KeAmDichte}}**: a checkerboard of the luxel size per face, computed like q3map2 (q3map_lightmapSampleSize, _samplesize and _lightmapscale of the entity, -samplesize; enlarged until the face fits into 128 luxels). Finer than 16 reddish, coarser bluish, faces without lightmap grey.",
              "**{{KeAmDichte}}**: ein Schachbrett in Luxel-Größe je Fläche, gerechnet wie q3map2 (q3map_lightmapSampleSize, _samplesize und _lightmapscale der Entity, -samplesize; vergrößert, bis die Fläche in 128 Luxel passt). Feiner als 16 rötlich, gröber bläulich, Flächen ohne Lightmap grau.",
              "**{{KeAmDichte}}**:每个面显示光照像素大小的棋盘格,计算方式与 q3map2 相同(q3map_lightmapSampleSize、实体的 _samplesize 和 _lightmapscale、-samplesize;放大到面能放进 128 个像素为止)。比 16 细为偏红,粗为偏蓝,没有光照贴图的面为灰色。",
              "**{{KeAmDichte}}**:面ごとにルクセルの大きさの市松模様を表示します。計算は q3map2 と同じです(q3map_lightmapSampleSize、エンティティの _samplesize と _lightmapscale、-samplesize。面が 128 ルクセルに収まるまで拡大)。16 より細かいと赤み、粗いと青み、ライトマップのない面は灰色です。"),
            T("**{{KeAmKollision}}**: brushes by the contents q3map2 gives them - playerclip magenta, monsterclip orange, botclip yellow, shotclip cyan, nodrop dark red, water, slime, lava, fog, triggers. Visible walls the player walks through (no player collision) glow bright red.",
              "**{{KeAmKollision}}**: Brushes nach dem Inhalt, den q3map2 ihnen gibt - playerclip magenta, monsterclip orange, botclip gelb, shotclip türkis, nodrop dunkelrot, Wasser, Schleim, Lava, Nebel, Trigger. Sichtbare Wände, durch die der Spieler geht (ohne Spieler-Kollision), leuchten rot.",
              "**{{KeAmKollision}}**:按 q3map2 赋予的内容给画刷着色——playerclip 品红、monsterclip 橙色、botclip 黄色、shotclip 青色、nodrop 暗红,以及水、黏液、岩浆、雾和触发器。玩家能穿过的可见墙(没有玩家碰撞)显示为亮红色。",
              "**{{KeAmKollision}}**:q3map2 が与える内容でブラシを色分けします。playerclip はマゼンタ、monsterclip はオレンジ、botclip は黄、shotclip はシアン、nodrop は暗い赤、ほかに水・スライム・溶岩・霧・トリガー。プレイヤーが通り抜けられる見える壁(プレイヤー衝突なし)は明るい赤になります。"),
            T("**{{KeAmSicht}}** and **{{KeAmLast}}** need the compiled .bsp next to the map (with VIS): what the engine draws from the camera's cluster in green, or a heat map of the triangles drawn from each place. The numbers are shown at the top left of the camera.",
              "**{{KeAmSicht}}** und **{{KeAmLast}}** brauchen die gebaute .bsp neben der Karte (mit VIS): was die Engine vom Cluster der Kamera aus zeichnet, grün, oder eine Wärmekarte der Dreiecke, die von jedem Ort aus gezeichnet werden. Die Zahlen stehen oben links in der Kamera.",
              "**{{KeAmSicht}}** 和 **{{KeAmLast}}** 需要地图旁边已编译的 .bsp(含 VIS):前者用绿色显示引擎从摄像机所在簇绘制的内容,后者显示从各位置绘制的三角形数量热图。数值显示在摄像机视图左上角。",
              "**{{KeAmSicht}}** と **{{KeAmLast}}** にはマップの隣にあるコンパイル済みの .bsp(VIS 付き)が必要です。前者はカメラのクラスターからエンジンが描くものを緑で、後者は各場所から描かれる三角形の数をヒートマップで示します。数値はカメラの左上に出ます。"),
        ]),

        ("h2", "spiegel", T("Mirror mode", "Spiegel-Modus", "镜像模式", "ミラーモード")),
        ("img", "h_karte8b_spiegel", T(
            "**{{KeSgMenue}} > {{KeSgX}}**: the pillar was moved 64 units to the right - its counterpart on the other side of the magenta plane followed.",
            "**{{KeSgMenue}} > {{KeSgX}}**: die Säule wurde 64 Einheiten nach rechts geschoben - ihr Gegenstück jenseits der magentafarbenen Ebene ist mitgegangen.",
            "**{{KeSgMenue}} > {{KeSgX}}**:柱子向右移动了 64 个单位——品红色平面另一侧的对应物也随之移动。",
            "**{{KeSgMenue}} > {{KeSgX}}**:柱を右へ 64 ユニット動かすと、マゼンタの平面の反対側にある対の柱も一緒に動きました。")),
        ("list", [
            T("**{{KeSgMenue}}**: {{KeSgAus}}, {{KeSgX}}, {{KeSgY}} or {{KeSgZ}}; the plane goes through the origin or, with **{{KeSgMitte}}**, through the center of the selection on the grid. It is drawn as a magenta line in the 2D views.",
              "**{{KeSgMenue}}**: {{KeSgAus}}, {{KeSgX}}, {{KeSgY}} oder {{KeSgZ}}; die Ebene geht durch den Ursprung oder, mit **{{KeSgMitte}}**, durch die Mitte der Auswahl auf dem Raster. Sie erscheint als magentafarbene Linie in den 2D-Ansichten.",
              "**{{KeSgMenue}}**:{{KeSgAus}}、{{KeSgX}}、{{KeSgY}} 或 {{KeSgZ}};平面穿过原点,或者使用 **{{KeSgMitte}}** 穿过选择在网格上的中心。它在 2D 视图中显示为品红色线条。",
              "**{{KeSgMenue}}**:{{KeSgAus}}、{{KeSgX}}、{{KeSgY}}、{{KeSgZ}}。平面は原点を通るか、**{{KeSgMitte}}** で選択のグリッド上の中心を通ります。2D ビューにはマゼンタの線で表示されます。"),
            T("Every change to the selection is written to its counterpart in the same undo step - live while dragging. Brushes and patches count one by one, point entities and groups as a whole; origin, angle and angles are mirrored, textures like **Flip** (with Texture Lock as set).",
              "Jede Änderung an der Auswahl wird im selben Rückgängig-Schritt auf das Gegenstück geschrieben - beim Ziehen live. Brushes und Patches zählen einzeln, Punkt-Entities und Gruppen als Ganzes; origin, angle und angles werden gespiegelt, Texturen wie bei **Flip** (mit Texture Lock wie eingestellt).",
              "对选择的每次修改都会在同一个撤销步骤中写入其对应物——拖动时实时进行。画刷和面片逐个处理,点实体和组整体处理;origin、angle 和 angles 会被镜像,纹理按 **Flip** 的方式处理(Texture Lock 按设置)。",
              "選択への変更はすべて同じ元に戻すステップの中で対のオブジェクトにも書き込まれ、ドラッグ中もリアルタイムです。ブラシとパッチは 1 つずつ、ポイントエンティティとグループは全体で扱います。origin・angle・angles はミラーされ、テクスチャは **Flip** と同じ扱いです(Texture Lock は設定どおり)。"),
            T("If the exact mirror image already exists, it becomes the counterpart; otherwise the first change creates a mirrored copy. Deleting removes both; undo brings back both exactly. Objects that lie symmetric on the plane get no counterpart.",
              "Gibt es das genaue Spiegelbild schon, wird es zum Gegenstück; sonst legt die erste Änderung eine gespiegelte Kopie an. Löschen entfernt beide; Rückgängig bringt beide genau zurück. Objekte, die symmetrisch auf der Ebene liegen, bekommen kein Gegenstück.",
              "如果精确的镜像已经存在,它就成为对应物;否则第一次修改会创建一个镜像副本。删除会同时删除两者;撤销会精确恢复两者。对称位于平面上的对象没有对应物。",
              "正確な鏡像がすでにあればそれが対になり、なければ最初の変更でミラーしたコピーが作られます。削除すると両方が消え、元に戻すと両方が正確に戻ります。平面上で対称なオブジェクトには対はできません。"),
        ]),
        ("note", T(
            "Copied entities keep their keys, also targetname and target - rename them where the two sides should act separately.",
            "Kopierte Entities behalten ihre Schlüssel, auch targetname und target - benenne sie um, wo die beiden Seiten getrennt wirken sollen.",
            "复制的实体保留其键,包括 targetname 和 target——若两侧应分别起作用,请重命名。",
            "コピーされたエンティティは targetname と target を含めてキーをそのまま持ちます。両側を別々に動かしたい場合は名前を変えてください。")),

        ("h2", "undo", T("Undo history", "Undo-Verlauf", "撤销历史", "元に戻す履歴")),
        ("img", "h_karte8b_undo", T(
            "**{{KeUlTitel}}**: the start, three steps with the command names NetRadiant-Custom records, the current state highlighted.",
            "**{{KeUlTitel}}**: der Anfang, drei Schritte mit den Befehlsnamen, die NetRadiant-Custom führt, der aktuelle Stand hervorgehoben.",
            "**{{KeUlTitel}}**:开始状态、三个使用 NetRadiant-Custom 命令名称的步骤,当前状态高亮显示。",
            "**{{KeUlTitel}}**:開始状態と、NetRadiant-Custom が記録するコマンド名の 3 つのステップ。現在の状態が強調表示されています。")),
        ("p", T(
            "**{{KeUlMenue}}** lists all undo steps (oldest first) and below them the redo steps in grey. A click jumps to the state after that step with as many Undo or Redo as needed - each with its console line, as with Ctrl+Z and Ctrl+Y. The list goes back as far as the undo queue (**Preferences > Undo Queue Size**, default 512).",
            "**{{KeUlMenue}}** listet alle Rückgängig-Schritte (der älteste zuerst) und darunter grau die Wiederholen-Schritte. Ein Klick springt zum Stand nach diesem Schritt, mit so vielen Undo oder Redo wie nötig - jedes mit seiner Konsolenzeile, wie mit Strg+Z und Strg+Y. Die Liste reicht so weit zurück wie der Undo-Stapel (**Preferences > Undo Queue Size**, Vorgabe 512).",
            "**{{KeUlMenue}}** 列出所有撤销步骤(最早的在前),下方以灰色列出重做步骤。单击会通过所需次数的撤销或重做跳到该步骤之后的状态——每一步都有控制台输出,与 Ctrl+Z 和 Ctrl+Y 相同。列表能回溯的范围与撤销队列相同(**Preferences > Undo Queue Size**,默认 512)。",
            "**{{KeUlMenue}}** は元に戻すステップをすべて(古い順に)並べ、その下にやり直しのステップを灰色で表示します。クリックすると必要な回数だけ元に戻す/やり直しを行ってそのステップの後の状態に移ります。Ctrl+Z と Ctrl+Y と同じく、それぞれコンソールに 1 行出ます。さかのぼれる範囲は元に戻すキューと同じです(**Preferences > Undo Queue Size**、既定 512)。")),

        ("h2", "modelle", T("misc_model from all model folders", "misc_model aus allen Modellordnern", "从所有模型文件夹恢复 misc_model", "すべてのモデルフォルダーからの misc_model")),
        ("p", T(
            "**{{KeRkModelle}}** ([[karte_oeffnen#editordaten]]) now looks for models in all model folders of the game, not only in the folders of the map's shaders. A model index (per model and mesh: vertex count, triangle set and shader) lies in the settings folder as `karte_modellindex_<…>.txt`. The first time - and for new or changed .pk3 files - bhed_q3map2 loads the missing models once (about half a minute for a big install); after that only the models whose meshes match a triangle surface of the .bsp, or whose shader the .bsp uses, are loaded. The console names the index and the candidates.",
            "**{{KeRkModelle}}** ([[karte_oeffnen#editordaten]]) sucht Modelle jetzt in allen Modellordnern des Spiels, nicht nur in den Ordnern der Shader der Karte. Ein Modell-Index (je Modell und Netz: Eckenzahl, Dreiecksmenge und Shader) liegt im Einstellungsordner als `karte_modellindex_<…>.txt`. Beim ersten Mal - und für neue oder geänderte .pk3 - lädt bhed_q3map2 die fehlenden Modelle einmal (bei einer großen Installation etwa eine halbe Minute); danach werden nur die Modelle geladen, deren Netze zu einer Dreiecksfläche der .bsp passen oder deren Shader die .bsp nutzt. Die Konsole nennt Index und Kandidaten.",
            "**{{KeRkModelle}}**([[karte_oeffnen#editordaten]])现在会在游戏的所有模型文件夹中查找模型,而不只是地图着色器所在的文件夹。模型索引(每个模型和网格的顶点数、三角形集合和着色器)以 `karte_modellindex_<…>.txt` 保存在设置文件夹中。第一次——以及有新的或修改过的 .pk3 时——bhed_q3map2 会把缺少的模型加载一次(大型安装约需半分钟);之后只加载网格与 .bsp 中三角形面匹配、或其着色器被 .bsp 使用的模型。控制台会列出索引和候选模型。",
            "**{{KeRkModelle}}**([[karte_oeffnen#editordaten]])は、マップのシェーダーのフォルダーだけでなく、ゲームのすべてのモデルフォルダーからモデルを探すようになりました。モデルインデックス(モデルとメッシュごとの頂点数・三角形の集合・シェーダー)は設定フォルダーに `karte_modellindex_<…>.txt` として保存されます。最初の 1 回と、新しい・変更された .pk3 があるときは、bhed_q3map2 が足りないモデルを一度読み込みます(大きなインストールで約 30 秒)。その後は、メッシュが .bsp の三角形の面に合うモデルか、シェーダーを .bsp が使っているモデルだけを読み込みます。コンソールにインデックスと候補が表示されます。")),
        ("table", [T("Map", "Karte", "地图", "マップ"), T("before", "vorher", "之前", "以前"), T("now", "jetzt", "现在", "現在")], [
            ["t2_trip", "0", "13"],
            ["yavin1", "18", "32"],
            ["hoth2", "5", "32"],
            ["kor1", "52", "52"],
            [T("jeditemple_garden (source known)", "jeditemple_garden (Quelle bekannt)", "jeditemple_garden(源文件已知)", "jeditemple_garden(ソースあり)"), "265 / 265", "265 / 265"],
        ]),

        ("h2", "gpu", T("GPU shadow tests (switched off)", "GPU-Schattentests (abgeschaltet)", "GPU 阴影测试(已关闭)", "GPU による影のテスト(オフ)")),
        ("p", T(
            "bhed_q3map2 has the command-line switch `-bhedgpu` for `-light`: the test \"does the ray from the luxel to the light hit something?\" runs on the graphics card (Direct3D 11), the light itself stays on the CPU. The result is **byte for byte the same** as without it - checked on five maps and by the ctest `q3map2_gpu`. On the measured computer (32 threads, RTX 4070 Laptop) it was not faster (mp/duel1 2.3 s → 3.2 s, kejim_post 10.0 s → 11.3 s), so it is not in the Build menu and stays off.",
            "bhed_q3map2 hat für `-light` den Schalter `-bhedgpu`: die Prüfung „trifft der Strahl vom Luxel zum Licht etwas?“ läuft auf der Grafikkarte (Direct3D 11), das Licht selbst bleibt auf der CPU. Das Ergebnis ist **Byte für Byte gleich** wie ohne - geprüft auf fünf Karten und vom ctest `q3map2_gpu`. Auf dem gemessenen Rechner (32 Threads, RTX 4070 Laptop) war es nicht schneller (mp/duel1 2,3 s → 3,2 s, kejim_post 10,0 s → 11,3 s), darum steht es nicht im Build-Menü und bleibt aus.",
            "bhed_q3map2 为 `-light` 提供了命令行开关 `-bhedgpu`:“从光照像素到灯光的射线是否被遮挡?”的测试在显卡上运行(Direct3D 11),光照计算本身仍在 CPU 上。结果与不使用时**逐字节相同**——已在五张地图和 ctest `q3map2_gpu` 中验证。在测量所用的电脑(32 线程,RTX 4070 Laptop)上它并没有更快(mp/duel1 2.3 秒 → 3.2 秒,kejim_post 10.0 秒 → 11.3 秒),因此不在 Build 菜单中,保持关闭。",
            "bhed_q3map2 には `-light` 用のコマンドラインスイッチ `-bhedgpu` があります。「ルクセルからライトへの光線が何かに当たるか」のテストをグラフィックカード(Direct3D 11)で行い、ライトの計算自体は CPU のままです。結果はなしの場合と**バイト単位で同じ**で、5 つのマップと ctest `q3map2_gpu` で確認しています。測定したコンピューター(32 スレッド、RTX 4070 Laptop)では速くならなかった(mp/duel1 2.3 秒 → 3.2 秒、kejim_post 10.0 秒 → 11.3 秒)ため、Build メニューには入れず、オフのままです。")),
    ],
}

SEITEN = [KARTE_WERKZEUGE8B]
