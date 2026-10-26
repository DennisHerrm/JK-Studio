# -*- coding: utf-8 -*-
"""Oberflaechentexte des Karten-Editors, Stufe 8a (Kompilier-Fortschritt,
BSP-Import ohne Datenverlust, Teil-Kompilierungen), fuer tools/gen_i18n.py.

Alles hier sind eigene Funktionen (nicht in NetRadiant-Custom): eigene
Eintraege im Build-Menue mit Tooltip. Mit "Begriffe wie NetRadiant" bleiben
sie englisch.
"""

# (Kennung, English, Deutsch, 中文, 日本語)
KARTE8 = [
    # Punkt 1: Fortschritt mit Restzeit
    ("KeFsTitel", "Build progress", "Baufortschritt", "编译进度", "ビルドの進行状況"),
    ("KeFsSchritt", "%s - step %d of %d: %s", "%s - Schritt %d von %d: %s", "%s - 第 %d 步,共 %d 步:%s", "%s - ステップ %d / %d:%s"),
    ("KeFsBounce", "bounce %d of %d", "Bounce %d von %d", "反弹 %d / %d", "バウンス %d / %d"),
    ("KeFsWarten", "starting ...", "startet ...", "正在启动 ...", "開始中 ..."),
    ("KeFsGesamt", "All steps", "Alle Schritte", "所有步骤", "全ステップ"),
    ("KeFsRestVerlauf", "about %s left (from the last build of this map)", "noch etwa %s (aus dem letzten Bau dieser Karte)",
     "大约还剩 %s(根据此地图上次的编译)", "残り約 %s(このマップの前回のビルドから)"),
    ("KeFsRestGeschaetzt", "at least %s left (estimated from this phase; exact from the second build)",
     "noch mindestens %s (geschätzt aus dieser Phase; genau ab dem zweiten Bau)",
     "至少还剩 %s(根据此阶段估算;第二次编译起更准确)", "残り %s 以上(このフェーズから推定。2 回目のビルドから正確になります)"),
    ("KeFsRestUnbekannt", "remaining time: known from the second build of this map", "Restzeit: ab dem zweiten Bau dieser Karte",
     "剩余时间:从此地图的第二次编译起可知", "残り時間:このマップの 2 回目のビルドからわかります"),
    ("KeFsVergangen", "%s elapsed", "%s vergangen", "已用 %s", "経過 %s"),
    ("KeFsHinweis", "Progress from q3map2's own output (not in NetRadiant-Custom)", "Fortschritt aus der Ausgabe von q3map2 (nicht in NetRadiant-Custom)",
     "进度来自 q3map2 自身的输出(NetRadiant-Custom 中没有)", "進行状況は q3map2 自身の出力から(NetRadiant-Custom にはありません)"),
    ("KeFsZeitenKopf", "Time per step:", "Zeit je Schritt:", "每步用时:", "ステップごとの時間:"),
    ("KeFsZeitJetzt", "  %d %s: %s s", "  %d %s: %s s", "  %d %s:%s 秒", "  %d %s:%s 秒"),
    ("KeFsZeitVergleich", " (last build %s s, %s)", " (letzter Bau %s s, %s)", "(上次编译 %s 秒,%s)", "(前回のビルド %s 秒、%s)"),
    ("KeFsMelden", "Notify when a build is done", "Melden, wenn der Bau fertig ist", "编译完成时通知", "ビルド完了時に通知"),
    ("KeFsMeldenTip",
     "When a build ends while BehavEd is not the front window, its taskbar button flashes until you switch to it - no sound, no dialog (not in NetRadiant-Custom)",
     "Endet ein Bau, während BehavEd nicht das vordere Fenster ist, blinkt sein Knopf in der Taskleiste, bis Sie dorthin wechseln - ohne Ton, ohne Dialog (nicht in NetRadiant-Custom)",
     "编译结束时如果 BehavEd 不是前台窗口,它在任务栏上的按钮会闪烁,直到您切换过去——没有声音,没有对话框(NetRadiant-Custom 中没有)",
     "ビルドが終わったときに BehavEd が前面のウィンドウでなければ、切り替えるまでタスクバーのボタンが点滅します。音もダイアログもありません(NetRadiant-Custom にはありません)"),
]

# Punkt 2: Editor-Daten behalten
KARTE8 += [
    ("KeEdMenue", "Keep editor data", "Editor-Daten behalten", "保留编辑器数据", "エディターデータを保持"),
    ("KeEdMenueTip",
     "Build with -keeplights (all lights stay in the .bsp; the game removes lights without targetname when the map starts) and write <map>.bhed next to the .bsp: the exact source map. File > Open .bsp uses it when it fits the .bsp - misc_model, func_group, texture alignment, detail and layers come back. Put the .bhed into your .pk3 next to the .bsp. Lightmaps, light grid and geometry stay byte-identical; only the entity lump grows (not in NetRadiant-Custom)",
     "Mit -keeplights bauen (alle Lichter bleiben in der .bsp; das Spiel entfernt Lichter ohne targetname beim Kartenstart) und <Karte>.bhed neben die .bsp schreiben: die genaue Quellkarte. File > Open .bsp nimmt sie, wenn sie zur .bsp passt - misc_model, func_group, Texturausrichtung, detail und Ebenen kommen zurück. Die .bhed neben der .bsp mit in die .pk3 legen. Lightmaps, Lichtgitter und Geometrie bleiben Byte für Byte gleich; nur der Entity-Lump wächst (nicht in NetRadiant-Custom)",
     "使用 -keeplights 编译(所有灯光保留在 .bsp 中;地图启动时游戏会移除没有 targetname 的灯光),并在 .bsp 旁写入 <地图>.bhed:完整的源地图。File > Open .bsp 在它与 .bsp 匹配时使用它——misc_model、func_group、纹理对齐、detail 和图层都会恢复。请把 .bhed 与 .bsp 一起放入 .pk3。光照贴图、光照网格和几何体逐字节不变;只有实体块变大(NetRadiant-Custom 中没有)",
     "-keeplights 付きでビルドし(すべてのライトが .bsp に残ります。targetname のないライトはマップ開始時にゲームが削除します)、.bsp の隣に <マップ>.bhed(元のマップそのもの)を書き出します。File > Open .bsp は .bsp に合う場合にそれを使い、misc_model・func_group・テクスチャの配置・detail・レイヤーが戻ります。.bhed は .bsp と一緒に .pk3 に入れてください。ライトマップ・ライトグリッド・ジオメトリはバイト単位で同じままで、エンティティランプだけが大きくなります(NetRadiant-Custom にはありません)"),
    ("KeEdLichter", "Keep editor data: -keeplights for %d BSP command(s)", "Editor-Daten behalten: -keeplights für %d BSP-Befehl(e)",
     "保留编辑器数据:为 %d 个 BSP 命令添加 -keeplights", "エディターデータを保持:%d 個の BSP コマンドに -keeplights"),
    ("KeEdGeschrieben", "Editor data written: %s", "Editor-Daten geschrieben: %s", "已写入编辑器数据:%s", "エディターデータを書き出しました:%s"),
    ("KeEdGefunden", "Editor data found (%s): the exact source map is restored instead of a decompile",
     "Editor-Daten gefunden (%s): statt einer Dekompilierung kommt die genaue Quellkarte zurück",
     "找到编辑器数据(%s):恢复完整的源地图,而不是反编译", "エディターデータが見つかりました(%s):逆コンパイルの代わりに元のマップをそのまま復元します"),
    ("KeEdAndere", "Editor data %s belong to another build of this map - decompiling instead",
     "Editor-Daten %s gehören zu einem anderen Bau dieser Karte - es wird dekompiliert",
     "编辑器数据 %s 属于此地图的另一次编译——改为反编译", "エディターデータ %s はこのマップの別のビルドのものです。代わりに逆コンパイルします"),
    ("KeEdKaputt", "Editor data %s are damaged - decompiling instead", "Editor-Daten %s sind beschädigt - es wird dekompiliert",
     "编辑器数据 %s 已损坏——改为反编译", "エディターデータ %s は壊れています。代わりに逆コンパイルします"),
    ("KeOptD33", "%s keys or values longer than 1023 characters (q3map2 stops with \"Token too large\", the game cuts them off)",
     "%s Schlüssel oder Werte länger als 1023 Zeichen (q3map2 bricht mit \"Token too large\" ab, das Spiel schneidet sie ab)",
     "%s 个键或值超过 1023 个字符(q3map2 会以 \"Token too large\" 中止,游戏会截断它们)",
     "1023 文字を超えるキーまたは値 %s 個(q3map2 は \"Token too large\" で停止し、ゲームは切り詰めます)"),
    ("KeOptD34", "entity text about %s bytes of %s (%s bytes from kept lights)", "Entity-Text etwa %s Byte von %s (%s Byte durch behaltene Lichter)",
     "实体文本约 %s 字节,上限 %s(其中 %s 字节来自保留的灯光)", "エンティティテキスト 約 %s バイト / %s(うち保持したライトによるもの %s バイト)"),
    ("KeOptPrEnttext", "Entity lump and its limits", "Entity-Lump und seine Grenzen", "实体块及其限制", "エンティティランプとその上限"),
    ("KeOptPrEnttextE",
     "A key or value of 1024 characters or more stops q3map2 (\"Token too large\") and is cut off by the game. The whole entity text should stay below MAX_MAP_ENTSTRING (262144 bytes, the format limit of the tools; the engine itself does not check it). With \"Keep editor data\" the kept lights are counted, and the finding says how many bytes they add.",
     "Ein Schlüssel oder Wert ab 1024 Zeichen lässt q3map2 abbrechen (\"Token too large\") und wird vom Spiel abgeschnitten. Der ganze Entity-Text sollte unter MAX_MAP_ENTSTRING bleiben (262144 Byte, die Formatgrenze der Werkzeuge; die Engine selbst prüft sie nicht). Mit \"Editor-Daten behalten\" zählen die behaltenen Lichter mit, und der Befund nennt, wie viele Byte sie beitragen.",
     "1024 个字符或更长的键或值会让 q3map2 中止(\"Token too large\"),并会被游戏截断。整个实体文本应保持在 MAX_MAP_ENTSTRING(262144 字节,工具的格式限制;引擎本身不检查)以下。启用“保留编辑器数据”时会计入保留的灯光,结果会说明它们增加了多少字节。",
     "1024 文字以上のキーや値は q3map2 を停止させ(\"Token too large\")、ゲームでは切り詰められます。エンティティテキスト全体は MAX_MAP_ENTSTRING(262144 バイト、ツールのフォーマット上限。エンジン自体はチェックしません)未満に収めてください。「エディターデータを保持」では保持したライトも数え、それが何バイト増やすかを示します。"),
]

# Punkt 3: misc_model aus der Geometrie zurueckgewinnen
KARTE8 += [
    ("KeRkModelle", "Recover misc_model from the geometry", "misc_model aus der Geometrie zurückgewinnen", "从几何体恢复 misc_model", "ジオメトリから misc_model を復元"),
    ("KeRkModelleTip",
     "After opening, the models baked into the .bsp are searched in the game's model files (bhed_q3map2 loads them exactly as the compiler did) and placed as misc_model with origin, angle(s) and scale - only exact matches. Runs in the background (not in NetRadiant-Custom)",
     "Nach dem Öffnen werden die in die .bsp eingebackenen Modelle in den Modelldateien des Spiels gesucht (bhed_q3map2 lädt sie genau wie der Compiler) und als misc_model mit origin, angle(s) und Skalierung gesetzt - nur genaue Treffer. Läuft im Hintergrund (nicht in NetRadiant-Custom)",
     "打开后,会在游戏的模型文件中查找烘焙进 .bsp 的模型(bhed_q3map2 按编译器的方式加载它们),并以 origin、angle(s) 和缩放放置为 misc_model——只接受精确匹配。在后台运行(NetRadiant-Custom 中没有)",
     "開いた後、.bsp に焼き込まれたモデルをゲームのモデルファイルから探し(bhed_q3map2 がコンパイラーと同じ方法で読み込みます)、origin・angle(s)・スケール付きの misc_model として配置します。正確に一致したものだけです。バックグラウンドで動きます(NetRadiant-Custom にはありません)"),
    ("KeRkFlaechenOpt", "Keep unrecognized model faces as brushes", "Nicht erkannte Modellflächen als Pinsel behalten", "将未识别的模型面保留为笔刷",
     "認識できなかったモデル面をブラシとして残す"),
    ("KeRkFlaechenOptTip",
     "Faces that belong to no recognized model and lie on no brush (a model whose file is missing, or faces of non-solid brushes) become thin detail brushes in the layer \"Unrecognized model faces\" - check them; they can duplicate geometry (not in NetRadiant-Custom)",
     "Flächen, die zu keinem erkannten Modell gehören und auf keinem Pinsel liegen (ein Modell, dessen Datei fehlt, oder Flächen nicht fester Pinsel), werden dünne Detail-Pinsel in der Ebene „Nicht erkannte Modellflächen“ - prüfen Sie sie; sie können Geometrie doppeln (nicht in NetRadiant-Custom)",
     "不属于任何已识别模型、也不在任何笔刷上的面(模型文件缺失,或非实体笔刷的面)会成为图层“未识别的模型面”中的细薄 detail 笔刷——请检查它们;它们可能与几何体重复(NetRadiant-Custom 中没有)",
     "認識されたモデルにも属さず、どのブラシ上にもない面(ファイルがないモデル、または非ソリッドブラシの面)は、レイヤー「認識できなかったモデル面」の薄い detail ブラシになります。確認してください。ジオメトリが重複することがあります(NetRadiant-Custom にはありません)"),
    ("KeRkEbene", "Unrecognized model faces", "Nicht erkannte Modellflächen", "未识别的模型面", "認識できなかったモデル面"),
    ("KeRkTitel", "Recover misc_model", "misc_model zurückgewinnen", "恢复 misc_model", "misc_model の復元"),
    ("KeRkLaden", "loading %d candidate models with bhed_q3map2 ...", "lade %d Kandidaten-Modelle mit bhed_q3map2 ...", "正在用 bhed_q3map2 加载 %d 个候选模型 ...",
     "bhed_q3map2 で候補モデル %d 個を読み込み中 ..."),
    ("KeRkSuchen", "searching the models in the geometry", "suche die Modelle in der Geometrie", "正在几何体中查找模型", "ジオメトリ内でモデルを検索中"),
    ("KeRkHinweis", "Only exact matches become misc_model (not in NetRadiant-Custom)", "Nur genaue Treffer werden misc_model (nicht in NetRadiant-Custom)",
     "只有精确匹配才会成为 misc_model(NetRadiant-Custom 中没有)", "正確に一致したものだけが misc_model になります(NetRadiant-Custom にはありません)"),
    ("KeRkFertig", "Recovered %d misc_model (%d of them forcemeta), largest vertex error %.4f units; %d of %d candidate models loaded, %.1f s",
     "%d misc_model zurückgewonnen (davon %d forcemeta), größter Eckenfehler %.4f Einheiten; %d von %d Kandidaten-Modellen geladen, %.1f s",
     "已恢复 %d 个 misc_model(其中 %d 个为 forcemeta),最大顶点误差 %.4f 单位;已加载 %d / %d 个候选模型,%.1f 秒",
     "misc_model を %d 個復元しました(うち forcemeta %d 個)。最大頂点誤差 %.4f 単位。候補モデル %d / %d 個を読み込み、%.1f 秒"),
    ("KeRkFlaechen", "%d triangles belong to no recognized model and no brush: %d thin brushes in the layer \"Unrecognized model faces\" - check them, they can duplicate geometry",
     "%d Dreiecke gehören zu keinem erkannten Modell und keinem Pinsel: %d dünne Pinsel in der Ebene „Nicht erkannte Modellflächen“ - bitte prüfen, sie können Geometrie doppeln",
     "%d 个三角形不属于任何已识别的模型或笔刷:图层“未识别的模型面”中有 %d 个细薄笔刷——请检查,它们可能与几何体重复",
     "%d 個の三角形は認識されたモデルにもブラシにも属しません。レイヤー「認識できなかったモデル面」に薄いブラシ %d 個 - 確認してください。ジオメトリが重複することがあります"),
    ("KeRkFlaechenAus", "%d triangles belong to no recognized model and no brush; they are not in the map (%d brushes, option off)",
     "%d Dreiecke gehören zu keinem erkannten Modell und keinem Pinsel; sie fehlen in der Karte (%d Pinsel, Option aus)",
     "%d 个三角形不属于任何已识别的模型或笔刷;它们不在地图中(%d 个笔刷,选项已关闭)",
     "%d 個の三角形は認識されたモデルにもブラシにも属さず、マップにはありません(ブラシ %d 個、オプションはオフ)"),
    ("KeRkAbgebrochen", "Recover misc_model: cancelled", "misc_model zurückgewinnen: abgebrochen", "恢复 misc_model:已取消", "misc_model の復元:中止しました"),
    ("KeRkAndereKarte", "Recover misc_model: another map is open now, the result is dropped", "misc_model zurückgewinnen: inzwischen ist eine andere Karte offen, das Ergebnis verfällt",
     "恢复 misc_model:现在打开的是另一张地图,结果已丢弃", "misc_model の復元:別のマップが開かれたため、結果は破棄されました"),
]

# Punkt 4: Lichter schaetzen (experimentell)
KARTE8 += [
    ("KeLsOpt", "Estimate lights (experimental)", "Lichter schätzen (experimentell)", "估算灯光(实验性)", "ライトを推定(実験的)"),
    ("KeLsOptTip",
     "q3map2 removes every light without targetname. This estimates lights from the light grid (where the light directions of the grid cells meet) and puts them in the layer \"Estimated lights\" with the key _bhed_geschaetzt 1. Good for simple rooms with few lights; with many small or spot lights (below the 64x64x128 grid) mostly wrong - an estimate, not the original (not in NetRadiant-Custom)",
     "q3map2 entfernt jedes Licht ohne targetname. Das schätzt Lichter aus dem Lichtgitter (wo sich die Lichtrichtungen der Gitterzellen treffen) und legt sie mit dem Schlüssel _bhed_geschaetzt 1 in die Ebene „Geschätzte Lichter“. Gut für einfache Räume mit wenigen Lichtern; bei vielen kleinen oder Spot-Lichtern (unter dem 64x64x128-Gitter) meist falsch - eine Schätzung, nicht das Original (nicht in NetRadiant-Custom)",
     "q3map2 会移除每个没有 targetname 的灯光。此功能根据光照网格(网格单元的光照方向交汇处)估算灯光,并以键 _bhed_geschaetzt 1 放入图层“估算的灯光”。适合灯光较少的简单房间;对于大量小灯或聚光灯(低于 64x64x128 网格精度)大多不准——这是估算,不是原始灯光(NetRadiant-Custom 中没有)",
     "q3map2 は targetname のないライトをすべて削除します。これはライトグリッド(グリッドセルの光の方向が交わる場所)からライトを推定し、キー _bhed_geschaetzt 1 を付けてレイヤー「推定ライト」に置きます。ライトの少ない単純な部屋には向いていますが、小さなライトやスポットライトが多い場合(64x64x128 のグリッドより細かい)はたいてい外れます。推定であり、元のライトではありません(NetRadiant-Custom にはありません)"),
    ("KeLsEbene", "Estimated lights", "Geschätzte Lichter", "估算的灯光", "推定ライト"),
    ("KeLsFertig", "%d lights estimated (experimental) in the layer \"Estimated lights\" - an estimate from the light grid, not the original lights",
     "%d Lichter geschätzt (experimentell) in der Ebene „Geschätzte Lichter“ - eine Schätzung aus dem Lichtgitter, nicht die Originallichter",
     "已估算 %d 个灯光(实验性),位于图层“估算的灯光”——这是根据光照网格的估算,不是原始灯光",
     "%d 個のライトを推定しました(実験的)。レイヤー「推定ライト」にあります。ライトグリッドからの推定で、元のライトではありません"),
]

# Punkt 5: Teil-Kompilierungen
KARTE8 += [
    ("KeTkVis", "Partial compile: reuse VIS", "Teil-Kompilierung: VIS wiederverwenden", "部分编译:重用 VIS", "部分コンパイル:VIS を再利用"),
    ("KeTkVisTip",
     "Every -vis step gets -bhedvisreuse: if the portal file (.prt), the vis switches and the far plane key are unchanged since the last -vis of this map, bhed_q3map2 takes the visibility from <map>.bhedvis instead of computing it - changes to detail brushes, patches, models and entities need no new VIS. Structural changes compute as usual. The .bsp is byte-identical to a full -vis (with -threads 1; with more threads -vis itself varies between runs) (not in NetRadiant-Custom)",
     "Jeder -vis-Schritt bekommt -bhedvisreuse: sind die Portaldatei (.prt), die VIS-Schalter und der Schlüssel der Fernebene seit dem letzten -vis dieser Karte gleich, nimmt bhed_q3map2 die Sichtbarkeit aus <Karte>.bhedvis statt sie zu rechnen - Änderungen an Detail-Pinseln, Patches, Modellen und Entities brauchen kein neues VIS. Strukturänderungen werden wie gewohnt gerechnet. Die .bsp ist Byte für Byte gleich einem vollen -vis (mit -threads 1; mit mehreren Threads schwankt -vis selbst von Lauf zu Lauf) (nicht in NetRadiant-Custom)",
     "每个 -vis 步骤都会加上 -bhedvisreuse:如果自此地图上次 -vis 以来门户文件(.prt)、vis 开关和远平面键都未改变,bhed_q3map2 会从 <地图>.bhedvis 取得可见性而不重新计算——修改 detail 笔刷、曲面片、模型和实体都不需要重新 VIS。结构性修改照常计算。生成的 .bsp 与完整 -vis 逐字节相同(使用 -threads 1;多线程时 -vis 本身每次运行都会略有不同)(NetRadiant-Custom 中没有)",
     "各 -vis ステップに -bhedvisreuse が付きます。このマップの前回の -vis からポータルファイル(.prt)・vis スイッチ・ファープレーンのキーが変わっていなければ、bhed_q3map2 は可視性を計算せず <マップ>.bhedvis から取得します。detail ブラシ・パッチ・モデル・エンティティの変更には新しい VIS が要りません。構造の変更は通常どおり計算します。.bsp は完全な -vis とバイト単位で同じです(-threads 1 の場合。複数スレッドでは -vis 自体が実行ごとに変わります)(NetRadiant-Custom にはありません)"),
]
