"""Karte: GPU-Backen (Stufe 8g) - Build > GPU bake, was auf der Grafikkarte laeuft, warum das
Ergebnis Byte fuer Byte gleich ist, die Messtabelle und der Abweichungsbericht.
Die Bilder nimmt BHED_HB_NUR=gpubacken auf (gui/selbsttest_karte8g.inc).
"""
from inhalt_basis import T
from messung8g import MESSZEILEN

KARTE_GPUBACKEN = {
    "id": "karte_gpubacken",
    "gruppe": "karte",
    "titel": T("Map editor: GPU bake", "Karten-Editor: GPU-Backen", "地图编辑器:GPU 烘焙", "マップエディター:GPU ベイク"),
    "kurz": T(
        "Compute the whole light of a map on the graphics card - lightmaps, bounce and light grid - byte for byte the same as on the CPU, and much faster on big maps.",
        "Das ganze Licht einer Karte auf der Grafikkarte rechnen - Lightmaps, Bounce und Lichtgitter -, Byte für Byte gleich wie auf der CPU und auf großen Karten viel schneller.",
        "在显卡上计算地图的全部光照——光照贴图、反弹和光照网格——与 CPU 逐字节相同,在大地图上快得多。",
        "マップの光をすべてグラフィックカードで計算します(ライトマップ、バウンス、ライトグリッド)。CPU とバイト単位で同じ結果で、大きなマップではずっと速くなります。"),
    "bloecke": [
        ("p", T(
            "**{{KeLBuild}} > {{KeGpuMenue}}** (not in NetRadiant-Custom) decides where bhed_q3map2 computes the light of every `-light` step: **{{KeGpuAuto}}** (the default), **{{KeGpuEin}}** (switch `-bhedgpubake`) or **{{KeGpuAus}}**. The compiled map is **the same, byte for byte** - the graphics card is only faster. The choice is kept (setting `BhedGpuBakeMode`).",
            "**{{KeLBuild}} > {{KeGpuMenue}}** (nicht in NetRadiant-Custom) bestimmt, wo bhed_q3map2 das Licht jedes `-light`-Schritts rechnet: **{{KeGpuAuto}}** (die Vorgabe), **{{KeGpuEin}}** (Schalter `-bhedgpubake`) oder **{{KeGpuAus}}**. Die gebaute Karte ist **Byte für Byte dieselbe** - die Grafikkarte ist nur schneller. Die Wahl bleibt gespeichert (Einstellung `BhedGpuBakeMode`).",
            "**{{KeLBuild}} > {{KeGpuMenue}}**(NetRadiant-Custom 中没有)决定 bhed_q3map2 在哪里计算每个 `-light` 步骤的光照:**{{KeGpuAuto}}**(默认)、**{{KeGpuEin}}**(开关 `-bhedgpubake`)或 **{{KeGpuAus}}**。编译出的地图**逐字节相同**——显卡只是更快。选择会被保存(设置 `BhedGpuBakeMode`)。",
            "**{{KeLBuild}} > {{KeGpuMenue}}**(NetRadiant-Custom にはありません)は、bhed_q3map2 が各 `-light` ステップの光をどこで計算するかを決めます:**{{KeGpuAuto}}**(既定)、**{{KeGpuEin}}**(スイッチ `-bhedgpubake`)、**{{KeGpuAus}}**。コンパイルされたマップは**バイト単位で同じ**で、グラフィックカードは速いだけです。選択は保存されます(設定 `BhedGpuBakeMode`)。")),
        ("p", T(
            "**{{KeGpuAuto}}** takes the graphics card only when its result is bit-identical (it computes with double precision) and it is expected to be faster: large maps and bounce rounds. Small maps, and `-samples 4` on large maps, stay on the processor; a graphics card without double precision is never chosen. It also remembers, per map and light switches, how long the last processor run and the last graphics card run took, and from then on takes the faster one (also for the quick bake). The console names the choice and the reason, for example \"GPU bake (Auto): processor - little light work (34490 super luxels)\".",
            "**{{KeGpuAuto}}** nimmt die Grafikkarte nur, wenn ihr Ergebnis bitgleich ist (sie rechnet mit double) und sie voraussichtlich schneller ist: große Karten und Bounce-Runden. Kleine Karten und `-samples 4` auf großen Karten bleiben auf dem Prozessor; eine Grafikkarte ohne double wird nie gewählt. Außerdem merkt sich Auto je Karte und Licht-Schaltern, wie lange der letzte Lauf auf dem Prozessor und auf der Grafikkarte gedauert hat, und nimmt danach das Schnellere (auch beim Schnell-Backen). Die Konsole nennt Wahl und Grund, zum Beispiel \"GPU-Backen (Auto): Prozessor - wenig Lichtarbeit (34490 Super-Luxel)\".",
            "**{{KeGpuAuto}}** 只有在显卡结果逐位相同(支持双精度计算)且预计更快时才使用显卡:大地图和反弹轮次。小地图以及大地图上的 `-samples 4` 留在处理器上;不支持双精度的显卡从不会被选中。此外,自动模式会按地图和光照开关记住上次在处理器和显卡上运行所用的时间,之后选择更快的一方(快速烘焙也一样)。控制台会显示选择和原因。",
            "**{{KeGpuAuto}}** は、結果がビット単位で同一(倍精度で計算できる)で、速いと見込まれる場合だけグラフィックカードを使います(大きなマップやバウンス)。小さなマップや大きなマップでの `-samples 4` はプロセッサーで計算し、倍精度に対応しないグラフィックカードは選ばれません。さらにマップとライトのスイッチごとに前回のプロセッサーとグラフィックカードの所要時間を覚え、以後は速い方を選びます(クイックベイクも同様)。選択と理由はコンソールに表示されます。")),
        ("img", "h_karte_gpubacken_menue", T(
            "{{KeLBuild}}: {{KeGpuMenue}}, next to the partial compiles.",
            "{{KeLBuild}}: {{KeGpuMenue}}, neben den Teil-Kompilierungen.",
            "{{KeLBuild}}:{{KeGpuMenue}},位于部分编译旁边。",
            "{{KeLBuild}}:部分コンパイルの隣にある {{KeGpuMenue}}。")),
        ("p", T(
            "While the map builds, the build progress window and the status line name the graphics card. After the build the console shows the **deviation report**: every 64th lightmap is computed a second time on the CPU and compared luxel by luxel. The last report is also in the tooltip of {{KeGpuMenue}}. The quick bake of the real-time light ([[karte_tempo]]) uses the graphics card too when the entry is on.",
            "Während die Karte gebaut wird, nennen das Fortschrittsfenster und die Statuszeile die Grafikkarte. Nach dem Bau zeigt die Konsole den **Abweichungsbericht**: jede 64. Lightmap rechnet die CPU ein zweites Mal und vergleicht Luxel für Luxel. Der letzte Bericht steht auch im Tooltip von {{KeGpuMenue}}. Das Schnell-Backen des Echtzeit-Lichts ([[karte_tempo]]) nutzt bei eingeschaltetem Eintrag ebenfalls die Grafikkarte.",
            "编译地图时,编译进度窗口和状态栏会显示显卡名称。编译完成后,控制台显示**偏差报告**:每第 64 张光照贴图由 CPU 再计算一次,并逐个光照像素比较。最近一次报告也显示在 {{KeGpuMenue}} 的工具提示中。该项打开时,实时光照的快速烘焙([[karte_tempo]])也会使用显卡。",
            "マップのビルド中は、ビルドの進行状況ウィンドウとステータスバーにグラフィックカードの名前が表示されます。ビルドの後、コンソールに**差分レポート**が出ます。64 枚に 1 枚のライトマップを CPU でもう一度計算し、ルクセルごとに比較します。最後のレポートは {{KeGpuMenue}} のツールチップにもあります。この項目がオンなら、リアルタイムライトのクイックベイク([[karte_tempo]])もグラフィックカードを使います。")),
        ("img", "h_karte_gpubacken_bericht", T(
            "After the build: the deviation report in the console.",
            "Nach dem Bau: der Abweichungsbericht in der Konsole.",
            "编译后:控制台中的偏差报告。",
            "ビルドの後:コンソールの差分レポート。")),

        ("h2", "was", T("What runs on the graphics card", "Was auf der Grafikkarte läuft", "在显卡上运行的部分", "グラフィックカードで行う処理")),
        ("list", [
            T("**Lightmaps**: for every luxel the contribution of every light with its shadow ray, the style check, the adaptive supersampling of `-samples` (also several levels), the filter of `-filter` and adding the lights in their order.",
              "**Lightmaps**: für jeden Luxel der Beitrag jedes Lichts mit seinem Schattenstrahl, die Stil-Prüfung, das adaptive Nachabtasten von `-samples` (auch mehrere Ebenen), der Filter von `-filter` und das Aufsummieren der Lichter in ihrer Reihenfolge.",
              "**光照贴图**:每个光照像素上每盏灯的贡献及其阴影射线、样式检查、`-samples` 的自适应超采样(也包括多级)、`-filter` 的滤波,以及按顺序累加各盏灯。",
              "**ライトマップ**:各ルクセルについて、各ライトの寄与とその影の光線、スタイルの確認、`-samples` の適応スーパーサンプリング(複数段も)、`-filter` のフィルター、ライトを順番どおりに足し合わせる処理。"),
            T("**Bounce** (`-bounce`): every round, with the surface lights the radiosity creates.",
              "**Bounce** (`-bounce`): jede Runde, mit den Flächenlichtern, die die Radiosity erzeugt.",
              "**反弹**(`-bounce`):每一轮,包括辐射度生成的面光源。",
              "**バウンス**(`-bounce`):ラジオシティが作る面光源を含めて、すべての回。"),
            T("**Light grid** (the light for models and players): every grid point with all lights.",
              "**Lichtgitter** (das Licht für Modelle und Spieler): jeder Gitterpunkt mit allen Lichtern.",
              "**光照网格**(模型和玩家的光照):每个网格点与所有灯光。",
              "**ライトグリッド**(モデルとプレイヤーの光):すべてのグリッド点とすべてのライト。"),
            T("On the processor as always: loading, the light lists, vertex light, creating the bounce lights, stitching, storing and writing the .bsp - and everything the graphics card cannot do exactly (see below).",
              "Auf dem Prozessor wie immer: Laden, die Lichtlisten, Vertexlicht, das Erzeugen der Bounce-Lichter, Nähte, Speichern und Schreiben der .bsp - und alles, was die Grafikkarte nicht genau kann (siehe unten).",
              "仍在处理器上:加载、灯光列表、顶点光照、生成反弹灯光、接缝、存储和写入 .bsp——以及显卡无法精确完成的一切(见下文)。",
              "これまでどおりプロセッサーで行う処理:読み込み、ライトのリスト、頂点ライティング、バウンス用ライトの作成、継ぎ目、保存と .bsp の書き出し、そしてグラフィックカードで正確にできないもの(下記)。"),
        ]),

        ("h2", "gleich", T("Why the result is the same", "Warum das Ergebnis gleich ist", "为什么结果相同", "結果が同じになる理由")),
        ("p", T(
            "Graphics cards usually compute a little differently from the processor - that is why GPU lightmappers elsewhere give slightly different light. bhed_q3map2 does the **same calculations in the same order** as q3map2 on the processor:",
            "Grafikkarten rechnen gewöhnlich etwas anders als der Prozessor - darum geben GPU-Lightmapper anderswo leicht anderes Licht. bhed_q3map2 macht **dieselben Rechnungen in derselben Reihenfolge** wie q3map2 auf dem Prozessor:",
            "显卡通常与处理器的计算方式略有不同——因此其他 GPU 光照烘焙器得到的光照会略有差异。bhed_q3map2 以与处理器上的 q3map2 **相同的计算、相同的顺序**进行:",
            "グラフィックカードは普通、プロセッサーと少し違う計算をします。そのため、ほかの GPU ライトマッパーでは光がわずかに変わります。bhed_q3map2 は、プロセッサー上の q3map2 と**同じ計算を同じ順序で**行います。")),
        ("list", [
            T("every operation is rounded exactly like on the processor (no fused multiply-add); divisions are rounded exactly in integer arithmetic;",
              "jede Rechnung wird genau wie auf dem Prozessor gerundet (kein zusammengefasstes Multiplizieren-Addieren); Divisionen werden in Ganzzahlen genau gerundet;",
              "每次运算都与处理器上完全相同地舍入(不合并乘加);除法用整数运算精确舍入;",
              "すべての演算をプロセッサーとまったく同じように丸めます(積和をまとめない)。割り算は整数演算で正確に丸めます。"),
            T("q3map2 normalizes vectors and takes square roots in double precision; the graphics card decides the same rounded result with extra-precise float arithmetic and, in the rare close cases, in double precision;",
              "q3map2 normalisiert Vektoren und zieht Wurzeln in doppelter Genauigkeit; die Grafikkarte entscheidet dasselbe gerundete Ergebnis mit besonders genauer float-Rechnung und in den seltenen knappen Fällen in doppelter Genauigkeit;",
              "q3map2 以双精度归一化向量和开平方;显卡用额外精确的浮点运算得出相同的舍入结果,在少数临界情况下使用双精度;",
              "q3map2 はベクトルの正規化と平方根を倍精度で行います。グラフィックカードは特に精密な float 演算で同じ丸め結果を決め、まれな際どい場合だけ倍精度を使います。"),
            T("the shadow rays walk q3map2's own trace tree with its tolerances, and the supersampling follows the same order of stamps as the processor;",
              "die Schattenstrahlen laufen durch q3map2s eigenen Trace-Baum mit seinen Toleranzen, und das Nachabtasten folgt derselben Reihenfolge der Stempel wie der Prozessor;",
              "阴影射线沿 q3map2 自己的追踪树及其容差进行,超采样遵循与处理器相同的采样块顺序;",
              "影の光線は q3map2 自身のトレースツリーをその許容値で進み、スーパーサンプリングはプロセッサーと同じ順序で行います。"),
            T("nothing is summed up in the order threads happen to finish - every luxel adds its lights in list order.",
              "nichts wird in der Reihenfolge aufsummiert, in der Threads zufällig fertig werden - jeder Luxel addiert seine Lichter in der Reihenfolge der Liste.",
              "不会按线程碰巧完成的顺序求和——每个光照像素都按列表顺序累加其灯光。",
              "スレッドがたまたま終わった順に足し合わせることはありません。各ルクセルはライトをリストの順番で足します。"),
        ]),
        ("p", T(
            "Whatever the graphics card cannot compute exactly - alpha shadows and light filters (they need the texture), a value outside the safe range, a result that is not finite - is computed for that lightmap (or grid point) on the processor. If the graphics card fails or is reset by Windows during the build, the processor computes the rest. A lightmap too big for the graphics memory is computed in parts of its light list.",
            "Was die Grafikkarte nicht genau rechnen kann - Alpha-Schatten und Lichtfilter (sie brauchen die Textur), ein Wert außerhalb des sicheren Bereichs, ein nicht endliches Ergebnis -, rechnet für diese Lightmap (bzw. diesen Gitterpunkt) der Prozessor. Fällt die Grafikkarte während des Baus aus oder setzt Windows sie zurück, rechnet der Prozessor den Rest. Eine Lightmap, die nicht in den Grafikspeicher passt, wird in Teilen ihrer Lichtliste gerechnet.",
            "显卡无法精确计算的部分——alpha 阴影和光照滤镜(需要纹理)、超出安全范围的数值、非有限的结果——该光照贴图(或网格点)由处理器计算。如果显卡在编译期间出错或被 Windows 重置,其余部分由处理器计算。放不进显存的光照贴图会按其灯光列表分批计算。",
            "グラフィックカードで正確に計算できないもの(テクスチャが必要なアルファ影とライトフィルター、安全な範囲を外れた値、有限でない結果)は、そのライトマップ(またはグリッド点)をプロセッサーが計算します。ビルド中にグラフィックカードが失敗したり Windows にリセットされたりした場合は、残りをプロセッサーが計算します。グラフィックメモリーに収まらないライトマップは、ライトのリストを分けて計算します。")),
        ("note", T(
            "Graphics cards without double precision (for example Intel UHD) also run the GPU bake, but there the rare close cases are only approximated: the light differs very slightly (the report shows how much). On such a computer leave {{KeGpuMenue}} off for the final build.",
            "Grafikkarten ohne doppelte Genauigkeit (zum Beispiel Intel UHD) können das GPU-Backen auch, aber dort werden die seltenen knappen Fälle nur genähert: das Licht weicht ganz leicht ab (der Bericht zeigt, wie viel). Auf so einem Rechner lass {{KeGpuMenue}} für den endgültigen Bau aus.",
            "没有双精度的显卡(例如 Intel UHD)也能进行 GPU 烘焙,但那里少数临界情况只能近似:光照会有极小差异(报告会显示多少)。在这样的电脑上,最终编译时请关闭 {{KeGpuMenue}}。",
            "倍精度のないグラフィックカード(例:Intel UHD)でも GPU ベイクはできますが、まれな際どい場合は近似になり、光がごくわずかに変わります(レポートでどの程度かわかります)。そのようなコンピューターでは、最終ビルドでは {{KeGpuMenue}} をオフにしてください。")),

        ("h2", "messung", T("How much faster", "Wie viel schneller", "快多少", "どれだけ速いか")),
        ("p", T(
            "Measured on a notebook with an Intel i9-14900HX (32 threads) and an NVIDIA RTX 4070 Laptop GPU, the whole `-light` step (median of three runs). The processor uses all 32 threads; the GPU bake still needs the processor for the parts listed above, so the gain is largest on big maps with many lights and bounce rounds.",
            "Gemessen auf einem Notebook mit Intel i9-14900HX (32 Threads) und NVIDIA RTX 4070 Laptop GPU, der ganze `-light`-Schritt (Median aus drei Läufen). Der Prozessor nutzt alle 32 Threads; das GPU-Backen braucht den Prozessor weiter für die oben genannten Teile, darum ist der Gewinn auf großen Karten mit vielen Lichtern und Bounce-Runden am größten.",
            "在搭载 Intel i9-14900HX(32 线程)和 NVIDIA RTX 4070 Laptop GPU 的笔记本上测量整个 `-light` 步骤(三次运行的中位数)。处理器使用全部 32 个线程;GPU 烘焙仍需处理器完成上述部分,因此在灯光多、有反弹轮次的大地图上收益最大。",
            "Intel i9-14900HX(32 スレッド)と NVIDIA RTX 4070 Laptop GPU のノートパソコンで、`-light` ステップ全体を測定しました(3 回の中央値)。プロセッサーは 32 スレッドすべてを使います。GPU ベイクでも上に挙げた部分にはプロセッサーが必要なので、ライトが多くバウンスのある大きなマップで効果が最も大きくなります。")),
        ("table", [T("Map", "Karte", "地图", "マップ"), T("Switches", "Schalter", "开关", "スイッチ"), T("Processor", "Prozessor", "处理器", "プロセッサー"),
                   T("Graphics card", "Grafikkarte", "显卡", "グラフィックカード"), T("Same luxels", "Gleiche Luxel", "相同的光照像素", "一致したルクセル")], MESSZEILEN),
        ("note", T(
            "Small maps gain little: starting the graphics card costs a few tenths of a second. One case is slower on the graphics card: duel_mygeeto with `-samples 4` (101 s against 67 s) - the deep supersampling at many shadow edges branches differently for every luxel, and graphics cards are slow at that. With `-samples 2` (the Build menu) every map measured was as fast or faster.",
            "Kleine Karten gewinnen wenig: der Start der Grafikkarte kostet einige Zehntelsekunden. Ein Fall ist auf der Grafikkarte langsamer: duel_mygeeto mit `-samples 4` (101 s gegen 67 s) - das tiefe Nachabtasten an vielen Schattenkanten verzweigt für jeden Luxel anders, und darin sind Grafikkarten langsam. Mit `-samples 2` (Build-Menü) war jede gemessene Karte gleich schnell oder schneller.",
            "小地图收益不大:启动显卡需要零点几秒。有一种情况在显卡上更慢:duel_mygeeto 使用 `-samples 4`(101 秒对 67 秒)——在大量阴影边缘处的深层超采样对每个光照像素的分支都不同,而显卡不擅长这种情况。使用 `-samples 2`(Build 菜单)时,测量的每张地图都一样快或更快。",
            "小さなマップでは効果はわずかです。グラフィックカードの起動に 0.数秒かかります。グラフィックカードの方が遅くなるケースが一つあります:`-samples 4` の duel_mygeeto(101 秒対 67 秒)。多くの影の境界での深いスーパーサンプリングはルクセルごとに分岐が異なり、グラフィックカードはそれが苦手です。`-samples 2`(Build メニュー)では、測定したどのマップも同じかそれ以上に速くなりました。")),

        ("h2", "befehle", T("For the command line", "Für die Kommandozeile", "命令行用法", "コマンドライン用")),
        ("list", [
            T("`-bhedgpubake` - the light on the graphics card (with `-light`).", "`-bhedgpubake` - das Licht auf der Grafikkarte (mit `-light`).",
              "`-bhedgpubake`——在显卡上计算光照(与 `-light` 一起使用)。", "`-bhedgpubake`:光をグラフィックカードで計算(`-light` と一緒に)。"),
            T("`-bhedgpubakeauto` - the faster of processor and graphics card, the graphics card only bit-identical; `-bhedgpubakezeiten <cpu ms> <gpu ms>` gives the measured times of the last runs (0 = unknown).",
              "`-bhedgpubakeauto` - das Schnellere von Prozessor und Grafikkarte, die Grafikkarte nur bitgleich; `-bhedgpubakezeiten <CPU ms> <GPU ms>` gibt die gemessenen Zeiten der letzten Läufe mit (0 = unbekannt).",
              "`-bhedgpubakeauto`——处理器和显卡中更快的一方,显卡仅在逐位相同时使用;`-bhedgpubakezeiten <CPU 毫秒> <GPU 毫秒>` 传入上次运行的测量时间(0 = 未知)。",
              "`-bhedgpubakeauto`:プロセッサーとグラフィックカードの速い方(グラフィックカードはビット単位で同一の場合のみ)。`-bhedgpubakezeiten <CPU ms> <GPU ms>` は前回の計測時間を渡します(0 = 不明)。"),
            T("`-bhedgpubakecheck N` - every N-th lightmap is computed on the CPU too (default 64, up to one million pairs of light and luxel; `1` checks all, `0` none). The report is also written to `<map>.bhedgpu.txt`.",
              "`-bhedgpubakecheck N` - jede N-te Lightmap rechnet auch die CPU (Vorgabe 64, bis eine Million Paare aus Licht und Luxel; `1` prüft alle, `0` keine). Der Bericht steht auch in `<Karte>.bhedgpu.txt`.",
              "`-bhedgpubakecheck N`——每第 N 张光照贴图也由 CPU 计算(默认 64,最多一百万个灯光-光照像素对;`1` 检查全部,`0` 不检查)。报告也写入 `<地图>.bhedgpu.txt`。",
              "`-bhedgpubakecheck N`:N 枚に 1 枚のライトマップを CPU でも計算(既定 64、ライトとルクセルの組が 100 万まで。`1` はすべて、`0` はなし)。レポートは `<マップ>.bhedgpu.txt` にも書かれます。"),
            T("`-bhedgpuadapter N` or a part of the name - which graphics card (otherwise the fastest).",
              "`-bhedgpuadapter N` oder ein Teil des Namens - welche Grafikkarte (sonst die schnellste).",
              "`-bhedgpuadapter N` 或名称的一部分——使用哪块显卡(否则使用最快的)。",
              "`-bhedgpuadapter N` または名前の一部:使うグラフィックカード(指定しなければ最速のもの)。"),
        ]),
        ("note", T(
            "Hardware ray tracing (DXR) was measured as a pre-test (`-bhedgpudxr` with `-bhedgpu`): it traces the shadow rays about twice as fast, but without q3map2's tolerances it misses about 0.15 % of the hits and cannot see the solid parts of the trace tree - and the shadow rays are only 7-8 % of the time of a GPU bake. It is therefore not used for baking.",
            "Hardware-Raytracing (DXR) wurde als Vortest gemessen (`-bhedgpudxr` mit `-bhedgpu`): es rechnet die Schattenstrahlen etwa doppelt so schnell, verfehlt aber ohne die Toleranzen von q3map2 etwa 0,15 % der Treffer und sieht die festen Teile des Trace-Baums nicht - und die Schattenstrahlen machen nur 7-8 % der Zeit eines GPU-Backens aus. Darum wird es zum Backen nicht genutzt.",
            "硬件光线追踪(DXR)作为预测试进行了测量(`-bhedgpudxr` 与 `-bhedgpu` 一起):它追踪阴影射线约快一倍,但没有 q3map2 的容差会漏掉约 0.15% 的命中,也看不到追踪树中的实体部分——而阴影射线只占 GPU 烘焙时间的 7–8%。因此烘焙时不使用它。",
            "ハードウェアレイトレーシング(DXR)は事前テストとして測定しました(`-bhedgpu` と一緒に `-bhedgpudxr`)。影の光線を約 2 倍速く処理しますが、q3map2 の許容値がないためヒットの約 0.15 % を見逃し、トレースツリーの固い部分も見えません。しかも影の光線は GPU ベイクの時間の 7〜8 % にすぎません。そのためベイクには使っていません。")),
    ],
}

SEITEN = [KARTE_GPUBACKEN]
