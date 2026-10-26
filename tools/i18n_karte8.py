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
