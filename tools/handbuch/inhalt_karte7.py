"""Karte: Tempo und Echtzeit-Licht (Stufe 7).

Echtzeit-Licht in der Kamera (Vorschau auf der Grafikkarte), Schnell-Backen im
Hintergrund (echte Lightmaps aus bhed_q3map2) und was schneller geworden ist -
gemessen vorher und nachher, mit gleichem Ergebnis.
"""
from inhalt_basis import T

import tempo7


def _z(text):
    return T(text, text, text, text)   # Zahlen und Kennungen: in jeder Sprache gleich


_KOPF = [T("Step", "Schritt", "步骤", "処理"),
         T("Map", "Karte", "地图", "マップ"),
         T("NetRadiant-Custom", "NetRadiant-Custom", "NetRadiant-Custom", "NetRadiant-Custom"),
         T("before (Stage 6)", "vorher (Stufe 6)", "之前(第 6 阶段)", "以前(ステージ 6)"),
         T("now", "jetzt", "现在", "現在")]

_EDITOR_KOPF = [T("Step", "Schritt", "步骤", "処理"),
                T("Map", "Karte", "地图", "マップ"),
                T("before (Stage 6)", "vorher (Stufe 6)", "之前(第 6 阶段)", "以前(ステージ 6)"),
                T("now", "jetzt", "现在", "現在")]

KARTE_TEMPO = {
    "id": "karte_tempo",
    "gruppe": "karte",
    "titel": T("Map editor: speed and real-time light", "Karten-Editor: Tempo und Echtzeit-Licht",
               "地图编辑器:速度与实时光照", "マップエディター:速度とリアルタイムライト"),
    "kurz": T(
        "See the light while you build: {{KeLichtMenue}} lights the camera on the graphics card, {{KeBackenMenue}} builds the map in the background and shows the real lightmaps. Building and editing got faster - with the same result as before.",
        "Licht sehen, während man baut: {{KeLichtMenue}} beleuchtet die Kamera auf der Grafikkarte, {{KeBackenMenue}} baut die Karte im Hintergrund und zeigt die echten Lightmaps. Bauen und Bearbeiten wurden schneller - mit demselben Ergebnis wie vorher.",
        "边搭建边看光照:{{KeLichtMenue}} 在显卡上照亮摄像机视图,{{KeBackenMenue}} 在后台编译地图并显示真实的光照贴图。编译和编辑都变快了——结果与之前完全相同。",
        "作りながら光を確認できます。{{KeLichtMenue}} はグラフィックカードでカメラを照らし、{{KeBackenMenue}} はバックグラウンドでマップをビルドして本物のライトマップを表示します。ビルドと編集は速くなりましたが、結果は以前と同じです。"),
    "bloecke": [
        ("h2", "echtlicht", T("Real-time light", "Echtzeit-Licht", "实时光照", "リアルタイムライト")),
        ("img", "h_karte7_menue", T(
            "{{KeLBuild}}: {{KeLichtMenue}} and {{KeBackenMenue}} at the end of the menu (not in NetRadiant-Custom).",
            "{{KeLBuild}}: {{KeLichtMenue}} und {{KeBackenMenue}} am Ende des Menüs (nicht in NetRadiant-Custom).",
            "{{KeLBuild}}:菜单末尾的 {{KeLichtMenue}} 和 {{KeBackenMenue}}(NetRadiant-Custom 中没有)。",
            "{{KeLBuild}}:メニューの最後にある {{KeLichtMenue}} と {{KeBackenMenue}}(NetRadiant-Custom にはありません)。")),
        ("steps", [
            T("Switch on **{{KeLBuild}} > {{KeLichtMenue}}**. The camera (render mode textured) is lit by the lights of the map - point lights, spotlights, the sun of the sky shader (`q3map_sun`) and surface lights (`q3map_surfacelight`) - with the formulas of q3map2 -light and with shadows.",
              "Schalte **{{KeLBuild}} > {{KeLichtMenue}}** ein. Die Kamera (Darstellung texturiert) wird von den Lichtern der Karte beleuchtet - Punktlichter, Spotlichter, die Sonne des Himmels-Shaders (`q3map_sun`) und Flächenlicht (`q3map_surfacelight`) - mit den Formeln von q3map2 -light und mit Schatten.",
              "打开 **{{KeLBuild}} > {{KeLichtMenue}}**。摄像机(纹理渲染模式)会被地图中的光源照亮——点光源、聚光灯、天空着色器的太阳(`q3map_sun`)和面光源(`q3map_surfacelight`)——使用 q3map2 -light 的公式,并带有阴影。",
              "**{{KeLBuild}} > {{KeLichtMenue}}** をオンにします。カメラ(テクスチャ表示)がマップのライト、つまりポイントライト・スポットライト・空シェーダーの太陽(`q3map_sun`)・面光源(`q3map_surfacelight`)で、q3map2 -light の式を使い影付きで照らされます。"),
            T("Move a light or change its keys: the camera follows at once. Up to 64 lights light each frame (the strongest at the camera), 32 of them with shadow cubes.",
              "Verschiebe ein Licht oder ändere seine Schlüssel: die Kamera folgt sofort. Bis zu 64 Lichter beleuchten jedes Bild (die stärksten an der Kamera), 32 davon mit Schattenwürfeln.",
              "移动光源或修改其键值,摄像机会立即更新。每一帧最多由 64 个光源照亮(取对摄像机影响最强的),其中 32 个带阴影立方体。",
              "ライトを動かしたりキーを変えたりすると、カメラはすぐに追従します。1 フレームあたり最大 64 個のライト(カメラで最も強いもの)が照らし、そのうち 32 個は影キューブ付きです。"),
        ]),
        ("img", "h_karte7_licht", T(
            "Real-time light in a closed room with one light and a wall. Top right the camera says that this is a preview.",
            "Echtzeit-Licht in einem geschlossenen Raum mit einem Licht und einer Wand. Oben rechts sagt die Kamera, dass es eine Vorschau ist.",
            "在一个有一盏灯和一面墙的封闭房间中的实时光照。摄像机右上角注明这是预览。",
            "ライト 1 個と壁のある閉じた部屋でのリアルタイムライト。カメラの右上に、これがプレビューであることが表示されます。")),
        ("note", T(
            "The real-time light is a **preview** and is never saved as a result: no bounce light, no filter, no supersampling, shadows from shadow maps instead of rays. The final light always comes from bhed_q3map2 - {{KeBackenMenue}} or a build from the {{KeLBuild}} menu.",
            "Das Echtzeit-Licht ist eine **Vorschau** und wird nie als Ergebnis gespeichert: kein Bounce-Licht, kein Filter, kein Supersampling, Schatten aus Schattenkarten statt aus Strahlen. Das endgültige Licht kommt immer aus bhed_q3map2 - {{KeBackenMenue}} oder ein Bau aus dem Menü {{KeLBuild}}.",
            "实时光照只是**预览**,永远不会作为结果保存:没有反弹光、没有过滤、没有超采样,阴影来自阴影贴图而不是光线。最终光照始终来自 bhed_q3map2——{{KeBackenMenue}} 或 {{KeLBuild}} 菜单中的编译。",
            "リアルタイムライトは**プレビュー**で、結果として保存されることはありません。バウンス光・フィルター・スーパーサンプリングはなく、影はレイではなくシャドウマップで作られます。最終的なライトは常に bhed_q3map2 から作られます({{KeBackenMenue}} または {{KeLBuild}} メニューのビルド)。")),
        ("h2", "backen", T("Quick bake", "Schnell-Backen", "快速烘焙", "クイックベイク")),
        ("steps", [
            T("Switch on **{{KeLBuild}} > {{KeBackenMenue}}**. Two seconds after the last change bhed_q3map2 builds the map in a temporary folder - `-meta`, `-vis -fast` (when the map has no leak) and `-light -fast` - at low priority, so the editor stays smooth.",
              "Schalte **{{KeLBuild}} > {{KeBackenMenue}}** ein. Zwei Sekunden nach der letzten Änderung baut bhed_q3map2 die Karte in einem temporären Ordner - `-meta`, `-vis -fast` (wenn die Karte nicht leckt) und `-light -fast` - mit niedriger Priorität, der Editor bleibt flüssig.",
              "打开 **{{KeLBuild}} > {{KeBackenMenue}}**。最后一次修改两秒后,bhed_q3map2 会在临时文件夹中以低优先级编译地图——`-meta`、`-vis -fast`(地图没有泄漏时)和 `-light -fast`——编辑器保持流畅。",
              "**{{KeLBuild}} > {{KeBackenMenue}}** をオンにします。最後の変更から 2 秒後に、bhed_q3map2 が一時フォルダーで低優先度のままマップをビルドします(`-meta`、リークがなければ `-vis -fast`、`-light -fast`)。エディターは滑らかなままです。"),
            T("When it is done, the camera shows the real lightmaps of this build; top right it says **{{KeLichtGebacken}}**. Any change cancels a running build and starts a new one after the pause. Your map and a .bsp next to it are not touched.",
              "Ist er fertig, zeigt die Kamera die echten Lightmaps dieses Baus; oben rechts steht **{{KeLichtGebacken}}**. Jede Änderung bricht einen laufenden Bau ab und startet nach der Pause einen neuen. Deine Karte und eine .bsp daneben bleiben unberührt.",
              "完成后,摄像机显示这次编译的真实光照贴图,右上角显示 **{{KeLichtGebacken}}**。任何修改都会取消正在进行的编译,并在暂停后重新开始。你的地图及其旁边的 .bsp 不会被改动。",
              "完了すると、カメラにこのビルドの本物のライトマップが表示され、右上に **{{KeLichtGebacken}}** と表示されます。変更すると実行中のビルドは中止され、間を置いて新しいビルドが始まります。マップとその隣の .bsp には触れません。"),
        ]),
        ("img", "h_karte7_gebacken", T(
            "The same room after the quick bake: the lightmaps of bhed_q3map2.",
            "Derselbe Raum nach dem Schnell-Backen: die Lightmaps von bhed_q3map2.",
            "快速烘焙后的同一个房间:来自 bhed_q3map2 的光照贴图。",
            "クイックベイク後の同じ部屋:bhed_q3map2 のライトマップ。")),
        ("p", T(
            "The quick bake writes the same lightmaps, light grid and visibility as bhed_q3map2 on the command line with the same switches - the self-test compares the files. It is faster than a full build because of `-fast` and the missing bounce passes; for the release build use the {{KeLBuild}} menu.",
            "Das Schnell-Backen schreibt dieselben Lightmaps, dasselbe Lichtgitter und dieselbe Sichtbarkeit wie bhed_q3map2 auf der Kommandozeile mit denselben Schaltern - der Selbsttest vergleicht die Dateien. Schneller als ein voller Bau ist es durch `-fast` und die fehlenden Bounce-Durchgänge; für die fertige Karte nimm das Menü {{KeLBuild}}.",
            "快速烘焙写出的光照贴图、光照网格和可见性,与在命令行中用相同开关运行 bhed_q3map2 的结果完全相同——自检会比较这些文件。它之所以比完整编译快,是因为使用了 `-fast` 并且没有反弹过程;最终版本请使用 {{KeLBuild}} 菜单。",
            "クイックベイクは、同じスイッチでコマンドラインから bhed_q3map2 を実行した場合と同じライトマップ・ライトグリッド・可視性を書き出します(セルフテストがファイルを比較しています)。フルビルドより速いのは `-fast` を使いバウンスを行わないためです。完成版には {{KeLBuild}} メニューを使ってください。")),
        ("h2", "tempo", T("What got faster", "Was schneller wurde", "哪些变快了", "速くなった点")),
        ("p", T(
            "Every change was measured before and after (median of three runs, low priority, the same maps) and gives the same result: the .bsp of -bsp and -light is byte for byte the one of a 1-thread build of Stage 6, the saved maps are identical.",
            "Jede Änderung wurde vorher und nachher gemessen (Median aus drei Läufen, niedrige Priorität, dieselben Karten) und ergibt dasselbe: die .bsp von -bsp und -light ist Byte für Byte die eines Baus mit einem Thread der Stufe 6, die gespeicherten Karten sind gleich.",
            "每项改动都在前后进行了测量(三次运行的中位数、低优先级、相同的地图),结果完全相同:-bsp 和 -light 生成的 .bsp 与第 6 阶段单线程编译的结果逐字节一致,保存的地图也完全相同。",
            "すべての変更は前後で計測し(3 回の中央値、低優先度、同じマップ)、結果は同じです。-bsp と -light の .bsp はステージ 6 の 1 スレッドビルドとバイト単位で一致し、保存されるマップも同一です。")),
        ("list", [
            T("**-light** uses all cores for the lightmaps (light by light in blocks, the adaptive supersampling predicted and checked exactly) and for the light grid.",
              "**-light** nutzt alle Kerne für die Lightmaps (Licht für Licht in Blöcken, das adaptive Supersampling vorhergesagt und exakt geprüft) und für das Lichtgitter.",
              "**-light** 在光照贴图(按光源分块计算,自适应超采样先预测再精确核对)和光照网格上使用所有核心。",
              "**-light** はライトマップ(ライトごとにブロック単位で計算し、適応スーパーサンプリングは予測してから正確に確認)とライトグリッドに全コアを使います。"),
            T("**-bsp**: the T-junction fix searches in parallel and applies in order; finding equal light grid points uses buckets instead of a search through all points.",
              "**-bsp**: die T-Junction-Behebung sucht parallel und wendet der Reihe nach an; gleiche Punkte im Lichtgitter werden über Eimer gefunden statt durch eine Suche über alle Punkte.",
              "**-bsp**:T 形接缝修复并行搜索、按顺序应用;光照网格中相同的点通过分桶查找,而不是遍历所有点。",
              "**-bsp**:T ジャンクション修正は並列に探索して順に適用します。ライトグリッドの同じ点は全点の探索ではなくバケットで見つけます。"),
            T("**-vis** stays as in NetRadiant-Custom: with several threads it may differ slightly from build to build, with `-threads 1` it is always the same. Two ways to make it the same with several threads were tried and measured; both were slower.",
              "**-vis** bleibt wie in NetRadiant-Custom: mit mehreren Threads kann es von Bau zu Bau etwas abweichen, mit `-threads 1` ist es immer gleich. Zwei Wege, es mit mehreren Threads gleich zu machen, wurden ausprobiert und gemessen; beide waren langsamer.",
              "**-vis** 保持与 NetRadiant-Custom 相同:多线程时每次编译可能略有不同,使用 `-threads 1` 时总是相同。我们尝试并测量了两种让多线程结果一致的方法,两者都更慢。",
              "**-vis** は NetRadiant-Custom と同じです。複数スレッドではビルドごとにわずかに異なることがあり、`-threads 1` では常に同じです。複数スレッドで同じ結果にする方法を 2 つ試して計測しましたが、どちらも遅くなりました。"),
            T("**Editor**: invert selection, the selection test of the views and the NPC files of the optimizer (read in the background).",
              "**Editor**: Auswahl umkehren, die Auswahlabfrage der Ansichten und die NPC-Dateien des Optimierers (im Hintergrund gelesen).",
              "**编辑器**:反选、视图的选择查询,以及优化器的 NPC 文件(在后台读取)。",
              "**エディター**:選択の反転、ビューの選択判定、オプティマイザーの NPC ファイル(バックグラウンドで読み込み)。"),
        ]),
        ("table", _KOPF, tempo7.Q3MAP2),
        ("table", _EDITOR_KOPF, tempo7.EDITOR),
        ("p", T(
            "kejim_post \"Textures loaded\" was so fast before only because Stage 6 loaded Jedi Outcast without its folders in this run (0 shaders, every face with the replacement image - the error fixed in Stage 7); now the 1187 shaders and their images are loaded. Steps that were not changed vary by about 20 % from run to run.",
            "kejim_post „Texturen geladen“ war vorher nur so schnell, weil Stufe 6 Jedi Outcast in diesem Lauf ohne seine Ordner lud (0 Shader, jede Fläche mit dem Ersatzbild - der in Stufe 7 behobene Fehler); jetzt werden die 1187 Shader und ihre Bilder geladen. Nicht geänderte Schritte schwanken von Lauf zu Lauf um etwa 20 %.",
            "kejim_post 的“纹理加载完成”之前之所以这么快,只是因为第 6 阶段在这次运行中加载 Jedi Outcast 时没有其文件夹(0 个着色器,每个面都是替代图像——第 7 阶段修复的错误);现在会加载 1187 个着色器及其图像。未改动的步骤每次运行的波动约为 20%。",
            "kejim_post の「テクスチャの読み込み」が以前とても速かったのは、ステージ 6 がこの実行で Jedi Outcast をフォルダーなしで読み込んでいたため(シェーダー 0 個、すべての面が代替画像。ステージ 7 で修正した不具合)です。現在は 1187 個のシェーダーとその画像を読み込みます。変更していない処理は実行ごとに約 20 % 変動します。")),
        ("note", T(
            "Times on the test laptop (32 threads), median of three runs; \"-\" = not measured. Your computer will give other numbers, the ratio stays similar.",
            "Zeiten auf dem Test-Laptop (32 Threads), Median aus drei Läufen; \"-\" = nicht gemessen. Dein Rechner liefert andere Zahlen, das Verhältnis bleibt ähnlich.",
            "测试笔记本(32 线程)上的时间,三次运行的中位数;\"-\" = 未测量。你的电脑会得到不同的数字,但比例相近。",
            "時間はテスト用ノート PC(32 スレッド)での値で、3 回の中央値です。\"-\" は未計測です。お使いのコンピューターでは数値は異なりますが、比率は同程度です。")),
    ],
}

SEITEN = [KARTE_TEMPO]
