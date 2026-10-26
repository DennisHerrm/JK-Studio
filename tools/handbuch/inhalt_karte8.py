"""Karte: Bau-Fortschritt, Teil-Kompilierungen, Zur fertigen Karte hinzufuegen (Stufe 8a).

Das Fenster mit der Restzeit, VIS und Licht wiederverwenden (gleiches Ergebnis wie ein
voller Bau, vom ctest q3map2_teilbau Byte fuer Byte verglichen) und das Einfuegen einer
Ebene in eine fertige .bsp mit q3map2 -mergebsp samt den Pruefungen.
Die Teile zum Oeffnen fertiger Karten (Editor-Daten, misc_model, Lichter) stehen auf der
Seite karte_oeffnen (inhalt_karte4.py).
"""
from inhalt_basis import T

KARTE_TEILBAU = {
    "id": "karte_teilbau",
    "gruppe": "karte",
    "titel": T("Map editor: build progress and partial compiles", "Karten-Editor: Bau-Fortschritt und Teil-Kompilierungen",
               "地图编辑器:编译进度与部分编译", "マップエディター:ビルドの進行状況と部分コンパイル"),
    "kurz": T(
        "See how far a build is and how long it still takes, build again only what changed - with the same result as a full build - and add a new part to a compiled map without compiling the map again.",
        "Sehen, wie weit ein Bau ist und wie lange er noch dauert, nur neu bauen, was sich geändert hat - mit demselben Ergebnis wie ein voller Bau -, und einer fertigen Karte einen neuen Teil hinzufügen, ohne die Karte neu zu bauen.",
        "查看编译进行到哪一步、还需要多久;只重新编译改动过的部分——结果与完整编译相同;以及在不重新编译地图的情况下,向已编译的地图添加新部分。",
        "ビルドがどこまで進み、あとどれくらいかかるかを確認し、変わった部分だけを再ビルドし(結果はフルビルドと同じ)、マップを再コンパイルせずにコンパイル済みマップへ新しい部分を追加します。"),
    "bloecke": [
        ("h2", "fortschritt", T("Build progress", "Bau-Fortschritt", "编译进度", "ビルドの進行状況")),
        ("img", "h_karte8_fortschritt", T(
            "The window **{{KeFsTitel}}** in the second build of a map: step 3 (LIGHT), bounce 3 of 16, all steps at 43 % and the remaining time from the last build of this map.",
            "Das Fenster **{{KeFsTitel}}** im zweiten Bau einer Karte: Schritt 3 (LIGHT), Bounce 3 von 16, alle Schritte bei 43 % und die Restzeit aus dem letzten Bau dieser Karte.",
            "地图第二次编译时的 **{{KeFsTitel}}** 窗口:第 3 步(LIGHT)、第 3/16 次反弹、全部步骤完成 43%,剩余时间来自这张地图的上一次编译。",
            "マップ 2 回目のビルド中の **{{KeFsTitel}}** ウィンドウ:ステップ 3(LIGHT)、バウンス 3/16、全ステップ 43 %、このマップの前回のビルドから求めた残り時間。")),
        ("list", [
            T("While a build from the {{KeLBuild}} menu runs, the window **{{KeFsTitel}}** shows the entry, the step and its stage (BSP, VIS, LIGHT), the phase q3map2 is in with its own progress - the dots and digits q3map2 prints, or the bounce - a bar over all steps and the time.",
              "Während ein Bau aus dem Menü {{KeLBuild}} läuft, zeigt das Fenster **{{KeFsTitel}}** den Eintrag, den Schritt und seine Stufe (BSP, VIS, LIGHT), die Phase, in der q3map2 gerade ist, mit ihrem eigenen Fortschritt - die Punkte und Ziffern, die q3map2 ausgibt, oder den Bounce -, einen Balken über alle Schritte und die Zeit.",
              "当 {{KeLBuild}} 菜单中的编译运行时,**{{KeFsTitel}}** 窗口显示条目、步骤及其阶段(BSP、VIS、LIGHT)、q3map2 当前所处的阶段及其自身进度——即 q3map2 输出的点和数字,或反弹次数——一个覆盖全部步骤的进度条以及时间。",
              "{{KeLBuild}} メニューのビルドの実行中、**{{KeFsTitel}}** ウィンドウにはエントリー、ステップとその段階(BSP、VIS、LIGHT)、q3map2 が今いるフェーズとその進み具合(q3map2 が出力する点と数字、またはバウンス)、全ステップのバー、そして時間が表示されます。"),
            T("**Remaining time**: in the first build of a map with an entry it is estimated from the running phase and shown as \"at least\". From the second build on it comes from the times of the last build of this map with this entry - the step times are kept in `karte_bauzeiten.json` in the settings folder.",
              "**Restzeit**: im ersten Bau einer Karte mit einem Eintrag wird sie aus der laufenden Phase geschätzt und als „mindestens“ gezeigt. Ab dem zweiten Bau kommt sie aus den Zeiten des letzten Baus dieser Karte mit diesem Eintrag - die Schrittzeiten stehen in `karte_bauzeiten.json` im Einstellungsordner.",
              "**剩余时间**:用某个条目第一次编译地图时,根据正在运行的阶段估算,并显示为“至少”。从第二次编译起,剩余时间来自这张地图用这个条目上一次编译的时间——各步骤的时间保存在设置文件夹的 `karte_bauzeiten.json` 中。",
              "**残り時間**:あるエントリーでマップを初めてビルドするときは、実行中のフェーズから見積もって「少なくとも」と表示します。2 回目以降は、このマップをこのエントリーで前回ビルドしたときの時間から求めます。ステップごとの時間は設定フォルダーの `karte_bauzeiten.json` に保存されます。"),
            T("When the build is done, the console lists the time of each step and compares it with the last build: `1 BSP: 0.4 s (last build 0.5 s, -20 %)`.",
              "Ist der Bau fertig, nennt die Konsole die Zeit jedes Schritts und vergleicht sie mit dem letzten Bau: `1 BSP: 0.4 s (letzter Bau 0.5 s, -20 %)`.",
              "编译完成后,控制台会列出每个步骤的时间,并与上一次编译比较:`1 BSP: 0.4 s(上次编译 0.5 秒,-20 %)`。",
              "ビルドが終わると、コンソールに各ステップの時間が前回のビルドとの比較つきで表示されます:`1 BSP: 0.4 s(前回のビルド 0.5 秒、-20 %)`。"),
            T("**{{KeBmAbbrechen}}** in the window stops q3map2, like {{KeLBuild}} > {{KeBmAbbrechen}}.",
              "**{{KeBmAbbrechen}}** im Fenster beendet q3map2, wie {{KeLBuild}} > {{KeBmAbbrechen}}.",
              "窗口中的 **{{KeBmAbbrechen}}** 会停止 q3map2,与 {{KeLBuild}} > {{KeBmAbbrechen}} 相同。",
              "ウィンドウの **{{KeBmAbbrechen}}** は、{{KeLBuild}} > {{KeBmAbbrechen}} と同じく q3map2 を停止します。"),
            T("**{{KeLBuild}} > {{KeFsMelden}}** (on by default): if BehavEd is not the front window when a build ends, its taskbar button flashes - no sound, no dialog.",
              "**{{KeLBuild}} > {{KeFsMelden}}** (vorgegeben an): ist BehavEd nicht das vordere Fenster, wenn ein Bau endet, blinkt sein Knopf in der Taskleiste - ohne Ton, ohne Dialog.",
              "**{{KeLBuild}} > {{KeFsMelden}}**(默认开启):编译结束时如果 BehavEd 不是前台窗口,它在任务栏上的按钮会闪烁——没有声音,没有对话框。",
              "**{{KeLBuild}} > {{KeFsMelden}}**(既定でオン):ビルドが終わったときに BehavEd が前面のウィンドウでなければ、タスクバーのボタンが点滅します。音もダイアログもありません。"),
        ]),
        ("note", T(
            "The progress comes from q3map2's own output; a phase with many lightmaps or a long bounce moves in steps. The remaining time is only as good as the last build was comparable - another entry, other switches or a busy computer change it.",
            "Der Fortschritt kommt aus der Ausgabe von q3map2 selbst; eine Phase mit vielen Lightmaps oder ein langer Bounce bewegt sich in Sprüngen. Die Restzeit ist nur so gut, wie der letzte Bau vergleichbar war - ein anderer Eintrag, andere Schalter oder ein beschäftigter Rechner ändern sie.",
            "进度来自 q3map2 自身的输出;光照贴图很多的阶段或很长的反弹会跳跃式前进。剩余时间的准确度取决于上一次编译的可比性——换了条目、换了开关或电脑繁忙都会改变它。",
            "進行状況は q3map2 自身の出力から得ています。ライトマップの多いフェーズや長いバウンスは飛び飛びに進みます。残り時間の精度は前回のビルドとどれだけ比べられるかで決まり、別のエントリー・別のスイッチ・コンピューターの負荷で変わります。")),
        ("h2", "teilbau", T("Partial compiles", "Teil-Kompilierungen", "部分编译", "部分コンパイル")),
        ("img", "h_karte8_menue", T(
            "{{KeLBuild}}: {{KeEdMenue}}, {{KeTkVis}} and {{KeTkLicht}} switched on, below them {{KeZfMenue}} and {{KeFsMelden}} (not in NetRadiant-Custom).",
            "{{KeLBuild}}: {{KeEdMenue}}, {{KeTkVis}} und {{KeTkLicht}} eingeschaltet, darunter {{KeZfMenue}} und {{KeFsMelden}} (nicht in NetRadiant-Custom).",
            "{{KeLBuild}}:已打开 {{KeEdMenue}}、{{KeTkVis}} 和 {{KeTkLicht}},下面是 {{KeZfMenue}} 和 {{KeFsMelden}}(NetRadiant-Custom 中没有)。",
            "{{KeLBuild}}:{{KeEdMenue}}、{{KeTkVis}}、{{KeTkLicht}} をオンにした状態。その下に {{KeZfMenue}} と {{KeFsMelden}}(NetRadiant-Custom にはありません)。")),
        ("p", T(
            "Two switches in the {{KeLBuild}} menu let every build reuse what has not changed since the last one. The result is **byte for byte the .bsp of a full build** - the ctest `q3map2_teilbau` builds maps both ways and compares the files.",
            "Zwei Schalter im Menü {{KeLBuild}} lassen jeden Bau wiederverwenden, was sich seit dem letzten nicht geändert hat. Das Ergebnis ist **Byte für Byte die .bsp eines vollen Baus** - der ctest `q3map2_teilbau` baut Karten auf beide Arten und vergleicht die Dateien.",
            "{{KeLBuild}} 菜单中的两个开关让每次编译都复用自上次以来没有变化的部分。结果与**完整编译的 .bsp 逐字节相同**——ctest `q3map2_teilbau` 会用两种方式编译地图并比较文件。",
            "{{KeLBuild}} メニューの 2 つのスイッチで、前回から変わっていない部分を毎回のビルドで再利用します。結果は**フルビルドの .bsp とバイト単位で同じ**です。ctest `q3map2_teilbau` が両方の方法でマップをビルドし、ファイルを比較しています。")),
        ("list", [
            T("**{{KeTkVis}}**: `-vis` runs with `-bhedvisreuse`. bhed_q3map2 keeps the visibility in `<map>.bhedvis` together with the portal file and the switches. If the portals (`.prt`), the switches and the far plane key (`_farplanedist`) are the same, it copies the visibility instead of calculating it - the console says **visibility reused**. Portals change only with structural brushes; entities, detail brushes, patches and textures do not change them.",
              "**{{KeTkVis}}**: `-vis` läuft mit `-bhedvisreuse`. bhed_q3map2 behält die Sichtbarkeit in `<Karte>.bhedvis` zusammen mit der Portaldatei und den Schaltern. Sind die Portale (`.prt`), die Schalter und der Fernebenen-Schlüssel (`_farplanedist`) gleich, kopiert es die Sichtbarkeit, statt sie zu berechnen - die Konsole sagt **visibility reused**. Portale ändern sich nur mit strukturellen Brushes; Entities, Detail-Brushes, Patches und Texturen ändern sie nicht.",
              "**{{KeTkVis}}**:`-vis` 带 `-bhedvisreuse` 运行。bhed_q3map2 把可见性连同门户文件和开关一起保存在 `<地图>.bhedvis` 中。如果门户(`.prt`)、开关和 `_farplanedist` 键都相同,它会直接复制可见性而不重新计算——控制台显示 **visibility reused**。门户只会因结构笔刷而改变;实体、detail 笔刷、曲面片和纹理都不会改变门户。",
              "**{{KeTkVis}}**:`-vis` が `-bhedvisreuse` 付きで実行されます。bhed_q3map2 は可視性をポータルファイルとスイッチと一緒に `<マップ>.bhedvis` に保存します。ポータル(`.prt`)、スイッチ、`_farplanedist` キーが同じなら、計算せずに可視性をコピーし、コンソールに **visibility reused** と表示します。ポータルが変わるのは構造ブラシを変えたときだけで、エンティティ・detail ブラシ・パッチ・テクスチャでは変わりません。"),
            T("**{{KeTkLicht}}**: `-light` runs with `-bhedlightcache`. bhed_q3map2 keeps every lightmap and the light grid in `<map>.bhedlight` with a fingerprint of everything that goes into it - the geometry, the lights that reach it, the switches. The next run calculates only the lightmaps whose fingerprint changed: move one light, and only the lightmaps it reaches are lit again. The console says how many came from the cache: `light cache: 9 of 14 lightmaps …`.",
              "**{{KeTkLicht}}**: `-light` läuft mit `-bhedlightcache`. bhed_q3map2 behält jede Lightmap und das Lichtgitter in `<Karte>.bhedlight` mit einem Fingerabdruck von allem, was hineingeht - die Geometrie, die Lichter, die sie erreichen, die Schalter. Der nächste Lauf berechnet nur die Lightmaps, deren Fingerabdruck sich geändert hat: verschiebst du ein Licht, werden nur die Lightmaps neu beleuchtet, die es erreicht. Die Konsole sagt, wie viele aus dem Cache kamen: `light cache: 9 of 14 lightmaps …`.",
              "**{{KeTkLicht}}**:`-light` 带 `-bhedlightcache` 运行。bhed_q3map2 把每张光照贴图和光照网格连同其全部输入的指纹——几何体、照到它的灯光、开关——保存在 `<地图>.bhedlight` 中。下一次运行只计算指纹发生变化的光照贴图:移动一盏灯,只有它照到的光照贴图会重新计算。控制台会显示有多少来自缓存:`light cache: 9 of 14 lightmaps …`。",
              "**{{KeTkLicht}}**:`-light` が `-bhedlightcache` 付きで実行されます。bhed_q3map2 はすべてのライトマップとライトグリッドを、入力のすべて(ジオメトリ、そこに届くライト、スイッチ)の指紋と一緒に `<マップ>.bhedlight` に保存します。次の実行では指紋が変わったライトマップだけを計算します。ライトを 1 つ動かすと、そのライトが届くライトマップだけが再計算されます。コンソールにキャッシュから取った数が表示されます:`light cache: 9 of 14 lightmaps …`。"),
            T("**Only the post-processing changed** - `-gamma`, `-exposure`, `-compensate`, `-saturation`, `-contrast`, `-gridscale`, `-gridambientscale` or the worldspawn keys `_minlight`, `_minvertexlight`, `_mingridlight`: every lightmap and the grid come from the cache, only the post-processing runs again, without tracing a single ray.",
              "**Nur die Nachbearbeitung geändert** - `-gamma`, `-exposure`, `-compensate`, `-saturation`, `-contrast`, `-gridscale`, `-gridambientscale` oder die worldspawn-Schlüssel `_minlight`, `_minvertexlight`, `_mingridlight`: jede Lightmap und das Gitter kommen aus dem Cache, nur die Nachbearbeitung läuft neu, ohne einen einzigen Strahl.",
              "**只改了后期处理**——`-gamma`、`-exposure`、`-compensate`、`-saturation`、`-contrast`、`-gridscale`、`-gridambientscale` 或 worldspawn 键 `_minlight`、`_minvertexlight`、`_mingridlight`:所有光照贴图和网格都来自缓存,只重新运行后期处理,不追踪任何光线。",
              "**後処理だけを変えた場合**(`-gamma`、`-exposure`、`-compensate`、`-saturation`、`-contrast`、`-gridscale`、`-gridambientscale`、または worldspawn のキー `_minlight`、`_minvertexlight`、`_mingridlight`):すべてのライトマップとグリッドをキャッシュから取り、レイを 1 本も飛ばさずに後処理だけを実行し直します。"),
        ]),
        ("note", T(
            "With `-bounce` the light cache is not used: bounce light couples every lightmap with every other one. With several threads `-vis` itself can differ slightly from run to run (see [[karte_tempo]]); the reused visibility is the one of the last run - for a comparison with a full build use `-vis -threads 1`. The cache files lie next to the map; delete them to force a full build. They do not belong into the .pk3.",
            "Mit `-bounce` wird der Licht-Cache nicht benutzt: Bounce-Licht verbindet jede Lightmap mit jeder anderen. Mit mehreren Threads kann `-vis` selbst von Lauf zu Lauf leicht abweichen (siehe [[karte_tempo]]); die wiederverwendete Sichtbarkeit ist die des letzten Laufs - für einen Vergleich mit einem vollen Bau nimm `-vis -threads 1`. Die Cache-Dateien liegen neben der Karte; lösche sie, um einen vollen Bau zu erzwingen. In die .pk3 gehören sie nicht.",
            "使用 `-bounce` 时不使用光照缓存:反弹光把每张光照贴图与其他所有光照贴图联系在一起。多线程时 `-vis` 本身每次运行都可能略有不同(参见 [[karte_tempo]]);复用的可见性是上一次运行的结果——如需与完整编译比较,请使用 `-vis -threads 1`。缓存文件位于地图旁边;删除它们即可强制完整编译。它们不应放入 .pk3。",
            "`-bounce` を使うとライトキャッシュは使われません。バウンス光はすべてのライトマップを互いに結び付けるためです。複数スレッドでは `-vis` 自体が実行ごとにわずかに異なることがあります([[karte_tempo]] を参照)。再利用される可視性は前回の実行のものです。フルビルドと比べるときは `-vis -threads 1` を使ってください。キャッシュファイルはマップの隣にあり、削除するとフルビルドになります。.pk3 には入れません。")),
        ("h2", "hinzufuegen", T("Adding to a compiled map", "Zu einer fertigen Karte hinzufügen", "添加到已编译的地图", "コンパイル済みマップに追加する")),
        ("img", "h_karte8_zusammen", T(
            "The dialog **{{KeZfTitel}}**: the compiled test map is the template (drawn in the camera, not editable), the new pillar in the layer \"Neu\" is the part to add.",
            "Der Dialog **{{KeZfTitel}}**: die gebaute Testkarte ist die Vorlage (in der Kamera gezeichnet, nicht bearbeitbar), die neue Säule in der Ebene „Neu“ ist der Teil, der hinzukommt.",
            "**{{KeZfTitel}}** 对话框:已编译的测试地图是模板(在摄像机中绘制,不可编辑),图层“Neu”中的新柱子是要添加的部分。",
            "**{{KeZfTitel}}** ダイアログ:コンパイル済みのテストマップがテンプレート(カメラに描画され、編集不可)で、レイヤー「Neu」の新しい柱が追加する部分です。")),
        ("steps", [
            T("Open the source map you build the addition in - or a new map - and choose **{{KeLBuild}} > {{KeZfMenue}}**. As the template take a map of the game (**{{KeZfAusSpiel}}**) or any .bsp (**{{KeBspDatei}}**). It is drawn in the camera and the 2D views and cannot be selected or changed.",
              "Öffne die Quellkarte, in der du den Teil baust - oder eine neue Karte - und wähle **{{KeLBuild}} > {{KeZfMenue}}**. Als Vorlage nimm eine Karte des Spiels (**{{KeZfAusSpiel}}**) oder irgendeine .bsp (**{{KeBspDatei}}**). Sie wird in der Kamera und den 2D-Ansichten gezeichnet und lässt sich weder wählen noch ändern.",
              "打开你要在其中制作新增部分的源地图(或新地图),选择 **{{KeLBuild}} > {{KeZfMenue}}**。模板可以是游戏中的地图(**{{KeZfAusSpiel}}**)或任意 .bsp(**{{KeBspDatei}}**)。它会在摄像机和 2D 视图中绘制,无法选择或修改。",
              "追加部分を作るソースマップ(または新しいマップ)を開き、**{{KeLBuild}} > {{KeZfMenue}}** を選びます。テンプレートにはゲームのマップ(**{{KeZfAusSpiel}}**)または任意の .bsp(**{{KeBspDatei}}**)を使います。テンプレートはカメラと 2D ビューに描画され、選択も変更もできません。"),
            T("Build the addition - brushes, patches, entities - in a layer of its own and pick it under **{{KeZfEbene}}** (the current layer is preselected).",
              "Baue den Teil - Brushes, Patches, Entities - in einer eigenen Ebene und wähle sie unter **{{KeZfEbene}}** (vorgewählt ist die aktuelle Ebene).",
              "在单独的图层中制作新增部分——笔刷、曲面片、实体——并在 **{{KeZfEbene}}** 中选择该图层(默认选中当前图层)。",
              "追加部分(ブラシ、パッチ、エンティティ)を専用のレイヤーに作り、**{{KeZfEbene}}** でそのレイヤーを選びます(現在のレイヤーが選ばれています)。"),
            T("**{{KeZfPruefen}}** builds the layer and runs the checks below without adding anything. **{{KeZfBauen}}** builds the layer (`-meta`, `-light -fast -samples 2`) and inserts it with `q3map2 -mergebsp -world -fixnames`: as detail into the world of the template, whose BSP tree, visibility and lightmaps stay. The result is `<template>_merged.bsp` in the folder `karte_zusammen` in the settings folder; the template itself is not changed.",
              "**{{KeZfPruefen}}** baut die Ebene und führt die Prüfungen unten aus, ohne etwas hinzuzufügen. **{{KeZfBauen}}** baut die Ebene (`-meta`, `-light -fast -samples 2`) und fügt sie mit `q3map2 -mergebsp -world -fixnames` ein: als Detail in die Welt der Vorlage, deren BSP-Baum, Sichtbarkeit und Lightmaps bleiben. Das Ergebnis ist `<Vorlage>_merged.bsp` im Ordner `karte_zusammen` im Einstellungsordner; die Vorlage selbst bleibt unverändert.",
              "**{{KeZfPruefen}}** 会编译该图层并执行下面的检查,但不添加任何内容。**{{KeZfBauen}}** 编译该图层(`-meta`、`-light -fast -samples 2`),并用 `q3map2 -mergebsp -world -fixnames` 插入:作为 detail 插入模板的世界中,模板的 BSP 树、可见性和光照贴图保持不变。结果是设置文件夹内 `karte_zusammen` 文件夹中的 `<模板>_merged.bsp`;模板本身不会被修改。",
              "**{{KeZfPruefen}}** はレイヤーをビルドして下の確認を行いますが、何も追加しません。**{{KeZfBauen}}** はレイヤーをビルドし(`-meta`、`-light -fast -samples 2`)、`q3map2 -mergebsp -world -fixnames` で挿入します。テンプレートのワールドに detail として入り、テンプレートの BSP ツリー・可視性・ライトマップはそのままです。結果は設定フォルダー内の `karte_zusammen` フォルダーにある `<テンプレート>_merged.bsp` で、テンプレート自体は変更されません。"),
        ]),
        ("img", "h_karte8_zusammen_fertig", T(
            "After {{KeZfBauen}}: the light of the addition from the template, the collision check and the merged .bsp.",
            "Nach {{KeZfBauen}}: das Licht des Teils aus der Vorlage, die Kollisionsprüfung und die zusammengeführte .bsp.",
            "{{KeZfBauen}} 之后:来自模板的新增部分光照、碰撞检查以及合并后的 .bsp。",
            "{{KeZfBauen}} の後:テンプレートから求めた追加部分のライト、衝突の確認、結合した .bsp。")),
        ("h3", T("What is checked", "Was geprüft wird", "检查哪些内容", "確認する内容")),
        ("list", [
            T("**Weather brushes**: `system/inside` and `system/outside` must not be mixed - after merging, a map with both does not load. If it would, nothing is added.",
              "**Wetter-Brushes**: `system/inside` und `system/outside` dürfen nicht gemischt sein - nach dem Zusammenführen lädt eine Karte mit beiden nicht. Wäre es so, wird nichts hinzugefügt.",
              "**天气笔刷**:不能混用 `system/inside` 和 `system/outside`——合并后同时含有两者的地图无法加载。如果会出现这种情况,则不添加任何内容。",
              "**天候ブラシ**:`system/inside` と `system/outside` を混ぜてはいけません。結合後に両方を含むマップは読み込めません。そうなる場合は何も追加しません。"),
            T("**Weather zones**: if the template or the addition has `misc_weather_zone` entities, inside and outside brushes only count inside a zone; weather brushes of the addition outside every zone get a warning, because they have no effect.",
              "**Wetterzonen**: hat die Vorlage oder der Teil `misc_weather_zone`-Entities, gelten Inside- und Outside-Brushes nur innerhalb einer Zone; Wetter-Brushes des Teils außerhalb jeder Zone bekommen eine Warnung, denn sie wirken nicht.",
              "**天气区域**:如果模板或新增部分含有 `misc_weather_zone` 实体,inside 和 outside 笔刷只在区域内有效;新增部分中位于所有区域之外的天气笔刷会收到警告,因为它们不起作用。",
              "**天候ゾーン**:テンプレートか追加部分に `misc_weather_zone` エンティティがあると、inside/outside ブラシはゾーンの中でだけ有効です。どのゾーンにも入っていない追加部分の天候ブラシは効果がないため、警告が出ます。"),
            T("**Names**: a `target` or `targetname` of the addition that the template uses too is renamed by `-fixnames` - links between template and addition are lost. The dialog lists these names.",
              "**Namen**: ein `target` oder `targetname` des Teils, den die Vorlage auch benutzt, benennt `-fixnames` um - Verbindungen zwischen Vorlage und Teil gehen verloren. Der Dialog nennt diese Namen.",
              "**名称**:新增部分中与模板相同的 `target` 或 `targetname` 会被 `-fixnames` 重命名——模板与新增部分之间的关联会丢失。对话框会列出这些名称。",
              "**名前**:テンプレートでも使われている追加部分の `target` や `targetname` は `-fixnames` によって名前が変わり、テンプレートと追加部分のつながりは失われます。ダイアログにその名前が表示されます。"),
            T("**Limits**: the merged .bsp is counted like in the [[karte_optimierer|optimizer]]; a limit over the maximum is an error.",
              "**Grenzen**: die zusammengeführte .bsp wird gezählt wie im [[karte_optimierer|Optimierer]]; eine Grenze über dem Höchstwert ist ein Fehler.",
              "**限制**:合并后的 .bsp 会像在[[karte_optimierer|优化器]]中一样计数;超过上限即为错误。",
              "**上限**:結合した .bsp は[[karte_optimierer|オプティマイザー]]と同じように数えられます。上限を超えるとエラーです。"),
            T("**Collision**: the merged .bsp is loaded into the game's collision model (OpenJK `CM_BoxTrace`, ported); every new solid brush must stop the player box when it walks into it from outside.",
              "**Kollision**: die zusammengeführte .bsp wird in das Kollisionsmodell des Spiels geladen (OpenJK `CM_BoxTrace`, portiert); jeder neue feste Brush muss den Spielerkasten stoppen, wenn er von außen hineinläuft.",
              "**碰撞**:合并后的 .bsp 会载入游戏的碰撞模型(移植自 OpenJK `CM_BoxTrace`);每个新的实体笔刷都必须在玩家碰撞盒从外面走进来时挡住它。",
              "**衝突**:結合した .bsp をゲームの衝突モデル(移植した OpenJK `CM_BoxTrace`)に読み込みます。新しい固体ブラシはどれも、外から入ってくるプレイヤーの箱を止めなければなりません。"),
        ]),
        ("warn", T(
            "The light of the addition is **approximated**: q3map2 cannot light the addition against the geometry of the template. {{KeZfLichtGitter}} takes the average of the template's light grid around the addition as ambient light, {{KeZfLichtLichter}} the lights without a targetname that stayed in the template - only a map built with {{KeEdMenue}} (`-keeplights`) has them. The light grid of the template is not updated - models and players near the addition are lit as before. For the final map build everything in full.",
            "Das Licht des Teils ist **angenähert**: q3map2 kann den Teil nicht gegen die Geometrie der Vorlage beleuchten. {{KeZfLichtGitter}} nimmt den Mittelwert des Lichtgitters der Vorlage um den Teil als Umgebungslicht, {{KeZfLichtLichter}} die Lichter ohne targetname, die in der Vorlage geblieben sind - die hat nur eine mit {{KeEdMenue}} gebaute Karte (`-keeplights`). Das Lichtgitter der Vorlage wird nicht erneuert - Modelle und Spieler nahe am Teil werden beleuchtet wie vorher. Für die fertige Karte alles ganz bauen.",
            "新增部分的光照是**近似的**:q3map2 无法针对模板的几何体为新增部分计算光照。{{KeZfLichtGitter}} 取新增部分周围模板光照网格的平均值作为环境光,{{KeZfLichtLichter}} 使用模板中保留下来的、没有 targetname 的灯光——只有用 {{KeEdMenue}}(`-keeplights`)编译的地图才有。模板的光照网格不会更新——新增部分附近的模型和玩家的光照与之前相同。最终版本请完整编译。",
            "追加部分のライトは**近似**です。q3map2 はテンプレートのジオメトリを考慮して追加部分を照らせません。{{KeZfLichtGitter}} は追加部分の周りのテンプレートのライトグリッドの平均を環境光にし、{{KeZfLichtLichter}} はテンプレートに残っている targetname のないライトを使います(これがあるのは {{KeEdMenue}}(`-keeplights`)でビルドしたマップだけです)。テンプレートのライトグリッドは更新されないので、追加部分の近くのモデルやプレイヤーの明るさは以前のままです。完成版はすべてをフルビルドしてください。")),
        ("note", T(
            "Weather rules from the JKHub thread on weather brushes: never mix inside and outside brushes in one map; with weather zones the brushes only work inside a zone; the game makes the `.weather` cache file again when the map loads, so an old one need not be deleted.",
            "Wetter-Regeln aus dem JKHub-Thread zu Wetter-Brushes: Inside- und Outside-Brushes nie in einer Karte mischen; mit Wetterzonen wirken die Brushes nur innerhalb einer Zone; das Spiel erzeugt die Cache-Datei `.weather` beim Laden der Karte neu, eine alte muss man nicht löschen.",
            "来自 JKHub 天气笔刷讨论帖的天气规则:一张地图中绝不要混用 inside 和 outside 笔刷;有天气区域时,笔刷只在区域内有效;游戏在载入地图时会重新生成 `.weather` 缓存文件,因此不必删除旧文件。",
            "JKHub の天候ブラシのスレッドにある天候のルール:1 つのマップで inside と outside のブラシを混ぜないこと。天候ゾーンがある場合、ブラシはゾーンの中でだけ働くこと。ゲームはマップの読み込み時に `.weather` キャッシュファイルを作り直すので、古いファイルを消す必要はないこと。")),
    ],
}

SEITEN = [KARTE_TEILBAU]
