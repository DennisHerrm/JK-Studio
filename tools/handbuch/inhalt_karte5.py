"""Karte: der Optimierer und die Grenzen der Spiele (Stufe 5).

Die Tabellen der Grenzen kommen aus tools/handbuch/grenzen.json, die
kartegrenzen --json aus der Tabelle im Code schreibt (src/karte/grenzen.cpp);
der ctest karte_grenzen prueft, dass beide gleich sind.
"""
import json
import os

from inhalt_basis import T

_GRENZEN = json.load(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "grenzen.json"), encoding="utf-8"))

# Pruefung (Titel, Erklaerung = Titel + "E"), Behebung, sicher
_PRUEFUNGEN = [
    ("KeOptPrLeck", None, False),
    ("KeOptPrDetailLeck", "KeOptBhStruktur", False),
    ("KeOptPrVerdeckt", "KeOptBhCaulk", True),
    ("KeOptPrEntartet", "KeOptBhPinsel", True),
    ("KeOptPrMikro", "KeOptBhPinsel", False),
    ("KeOptPrOffGrid", "KeOptBhEinrasten", True),
    ("KeOptPrMisch", "KeOptBhStruktur", True),
    ("KeOptPrPortalEnt", None, False),
    ("KeOptPrDeko", "KeOptBhDetail", False),
    ("KeOptPrBlock", None, False),
    ("KeOptPrUeberlapp", None, False),
    ("KeOptPrTJunction", None, False),
    ("KeOptPrPatch", "KeOptBhPatch", False),
    ("KeOptPrPatchGitter", "KeOptBhPatch", False),
    ("KeOptPrClip", None, False),
    ("KeOptPrTextur", None, False),
    ("KeOptPrLeereEnt", "KeOptBhEntitaet", False),
    ("KeOptPrLichtgitter", "KeOptBhWelt", False),
    ("KeOptPrLightmaps", "KeOptBhWelt", False),
    ("KeOptPrVis", None, False),
    ("KeOptPrDreiecke", None, False),
    ("KeOptPrSpawn", None, False),
    ("KeOptPrModelle", "KeOptBhModelle", False),
]

_OFFEN = T("open: no Jedi Outcast source - checked against the Jedi Academy value",
           "offen: keine Quelle für Jedi Outcast - geprüft gegen den Wert von Jedi Academy",
           "待定:没有 Jedi Outcast 的源码——按 Jedi Academy 的值检查",
           "未確定:Jedi Outcast のソースがないため、Jedi Academy の値で確認")


def _pruefzeile(titel, behebung, sicher):
    return [T("**{{%s}}**" % titel), T("{{%sE}}" % titel), T("{{%s}}" % (behebung or "KeOptBhKeine")),
            T("{{KeOptSicher}}" if sicher else "-")]


def _grenzzeile(g):
    name = "`%s`" % g["name"] if g["name"] else "-"
    erlaubt = T("%d" % g["erlaubt"])
    if g["vanilla"] and g["vanilla"] != g["erlaubt"] and g["vanilla"] != g["wert"]:
        v = g["vanilla"]
        erlaubt = T("%d (vanilla %d)" % (g["erlaubt"], v), "%d (Vanilla %d)" % (g["erlaubt"], v), "%d(原版 %d)" % (g["erlaubt"], v),
                    "%d(バニラ %d)" % (g["erlaubt"], v))
    quellen = "; ".join("`%s`" % s for s in g["stellen"]) or "-"
    folge = T("{{KeOptUeber}}" if g["hart"] else "{{KeOptHinweis}}")
    zeile = [T("{{%s}}" % g["titel"]), T(name + " = %d" % g["wert"] if g["name"] else "-"), erlaubt, T("{{%s}}" % g["artTitel"]), folge, T(quellen)]
    if g["offen"]:
        zeile[5] = T(quellen + " - " + _OFFEN.get("en"), quellen + " - " + _OFFEN.get("de"), quellen + " - " + _OFFEN.get("zh"),
                     quellen + " - " + _OFFEN.get("ja"))
    return zeile


_KOPF_GRENZEN = [T("Limit", "Grenze", "限制", "上限"), T("In the source", "Im Quelltext", "源码中", "ソース上"),
                 T("Allowed", "Erlaubt", "允许", "許容"), T("Kind", "Art", "类型", "種類"),
                 T("Exceeded", "Überschritten", "超出时", "超えた場合"), T("Source (file:line)", "Quelle (Datei:Zeile)", "来源(文件:行)", "出典(ファイル:行)")]


def _spieltabellen():
    bloecke = []
    for schluessel in ("jkasp", "jkamp", "md", "jk2"):
        spiel = _GRENZEN[schluessel]
        bloecke.append(("h3", T(spiel["name"], spiel["name"], spiel["name"], spiel["name"])))
        bloecke.append(("table", _KOPF_GRENZEN, [_grenzzeile(g) for g in spiel["grenzen"]]))
    return bloecke


OPTIMIERER = {
    "id": "karte_optimierer",
    "gruppe": "karte",
    "titel": T("Map editor: optimizer and limits", "Karten-Editor: Optimierer und Grenzen", "地图编辑器:优化器与限制", "マップエディター:最適化ツールと上限"),
    "kurz": T(
        "**{{KeLBuild}} > {{KeOptMenue}}** counts what the map uses against every limit of Jedi Academy, Movie Duels and Jedi Outcast - each limit taken from the source code of the engine -, finds build problems such as leaks, uncaulked hidden faces and patches with too many collision planes, and fixes many of them at the push of a button.",
        "**{{KeLBuild}} > {{KeOptMenue}}** zählt, was die Karte verbraucht, gegen jede Grenze von Jedi Academy, Movie Duels und Jedi Outcast - jede Grenze aus dem Quelltext der Engine -, findet Bauprobleme wie Lecks, nicht gecaulkte verdeckte Flächen und Patches mit zu vielen Kollisionsebenen und behebt viele davon auf Knopfdruck.",
        "**{{KeLBuild}} > {{KeOptMenue}}** 把地图的用量与 Jedi Academy、Movie Duels 和 Jedi Outcast 的每一项限制进行比较(每项限制都取自引擎源码),查找泄漏、未用 caulk 处理的隐藏面、碰撞平面过多的曲面片等编译问题,并能一键修复其中许多问题。",
        "**{{KeLBuild}} > {{KeOptMenue}}** は、マップが使っている量を Jedi Academy・Movie Duels・Jedi Outcast のすべての上限(いずれもエンジンのソースコードから取得)と比べ、リーク、caulk されていない隠れた面、衝突平面が多すぎるパッチなどのビルド上の問題を見つけ、その多くをボタン一つで修正します。"),
    "bloecke": [
        ("h2", "oeffnen", T("Opening the optimizer", "Den Optimierer öffnen", "打开优化器", "最適化ツールを開く")),
        ("img", "h_karte5_menue", T(
            "{{KeLBuild}} > **{{KeOptMenue}}** - its own entry, NetRadiant-Custom has no optimizer.",
            "{{KeLBuild}} > **{{KeOptMenue}}** - ein eigener Eintrag, NetRadiant-Custom hat keinen Optimierer.",
            "{{KeLBuild}} > **{{KeOptMenue}}**——独立的菜单项,NetRadiant-Custom 没有优化器。",
            "{{KeLBuild}} > **{{KeOptMenue}}**:独自の項目です。NetRadiant-Custom には最適化ツールがありません。")),
        ("steps", [
            T("**{{KeLBuild}} > {{KeOptMenue}}** opens the window **{{KeOptTitel}}** and analyzes the map in the background - also the largest maps; the editor stays usable meanwhile. The console shows a summary at the end.",
              "**{{KeLBuild}} > {{KeOptMenue}}** öffnet das Fenster **{{KeOptTitel}}** und prüft die Karte im Hintergrund - auch die größten Karten; der Editor bleibt währenddessen bedienbar. Am Ende steht eine Zusammenfassung in der Konsole.",
              "**{{KeLBuild}} > {{KeOptMenue}}** 打开 **{{KeOptTitel}}** 窗口,并在后台分析地图——即使是最大的地图也可以;分析期间编辑器仍可正常使用。结束时控制台会显示摘要。",
              "**{{KeLBuild}} > {{KeOptMenue}}** で **{{KeOptTitel}}** ウィンドウが開き、バックグラウンドでマップを分析します(最大級のマップでも可)。その間もエディターは操作できます。最後にコンソールに要約が表示されます。"),
            T("**{{KeOptRegelwerk}}** chooses the limits: **{{KeOptAuto}}** takes the game the editor works with (Jedi Academy, Movie Duels or Jedi Outcast); Jedi Academy multiplayer can be chosen as well.",
              "**{{KeOptRegelwerk}}** wählt die Grenzen: **{{KeOptAuto}}** nimmt das Spiel, mit dem der Editor arbeitet (Jedi Academy, Movie Duels oder Jedi Outcast); Jedi Academy Mehrspieler lässt sich ebenfalls wählen.",
              "**{{KeOptRegelwerk}}** 选择要使用的限制:**{{KeOptAuto}}** 采用编辑器当前使用的游戏(Jedi Academy、Movie Duels 或 Jedi Outcast);也可以选择 Jedi Academy 多人模式。",
              "**{{KeOptRegelwerk}}** で上限の種類を選びます。**{{KeOptAuto}}** はエディターが使っているゲーム(Jedi Academy、Movie Duels、Jedi Outcast)に合わせます。Jedi Academy のマルチプレイも選べます。"),
            T("If the map was built (see [[karte_bauen]]) or opened as a compiled map ([[karte_oeffnen]]), the .bsp is analyzed too: light grid, lightmaps, visibility, triangles per view, the leak line of the last BSP stage. Without a .bsp only the map is counted.",
              "Wurde die Karte gebaut (siehe [[karte_bauen]]) oder als fertige Karte geöffnet ([[karte_oeffnen]]), wird die .bsp mitgeprüft: Lichtgitter, Lightmaps, Sichtbarkeit, Dreiecke je Ansicht, die Leck-Linie der letzten BSP-Stufe. Ohne .bsp wird nur die Karte gezählt.",
              "如果地图已编译(见 [[karte_bauen]])或是作为已编译地图打开的([[karte_oeffnen]]),也会一并分析 .bsp:光照网格、光照贴图、可见性、每个视角的三角形数,以及上次 BSP 阶段的泄漏线。没有 .bsp 时只统计地图本身。",
              "マップをビルドした場合([[karte_bauen]] を参照)や、コンパイル済みマップとして開いた場合([[karte_oeffnen]])は、.bsp も分析します。ライトグリッド、ライトマップ、可視性、1 視点あたりの三角形数、最後の BSP 段階のリークラインです。.bsp がなければマップだけを数えます。"),
            T("On the page **{{KePgOptimizer}}** of **{{KeLPreferences}}** the optimizer can run by itself after every build; it opens only when there are problems.",
              "Auf der Seite **{{KePgOptimizer}}** der **{{KeLPreferences}}** kann der Optimierer nach jedem Bau von selbst laufen; er geht nur auf, wenn es Probleme gibt.",
              "在 **{{KeLPreferences}}** 的 **{{KePgOptimizer}}** 页中,可以让优化器在每次编译后自动运行;只有发现问题时才会打开窗口。",
              "**{{KeLPreferences}}** の **{{KePgOptimizer}}** ページで、ビルドのたびに最適化ツールを自動で実行できます。問題があるときだけウィンドウが開きます。"),
        ]),
        ("h2", "grenzen", T("Limits", "Grenzen", "限制", "上限")),
        ("img", "h_karte5_grenzen", T(
            "The tab **{{KeOptReiterGrenzen}}** with mp/duel1: each limit with value, allowed value, share and traffic light; on the right where the limit is in the source and the largest contributors.",
            "Der Reiter **{{KeOptReiterGrenzen}}** mit mp/duel1: jede Grenze mit Wert, erlaubtem Wert, Anteil und Ampel; rechts, wo die Grenze im Quelltext steht, und die größten Verbraucher.",
            "mp/duel1 的 **{{KeOptReiterGrenzen}}** 选项卡:每项限制都显示用量、允许值、占比和信号灯;右侧显示该限制在源码中的位置以及占用最多的对象。",
            "mp/duel1 を開いた **{{KeOptReiterGrenzen}}** タブ。各上限について値・許容値・割合・信号を表示し、右側にはソース上の位置と、多く使っている対象を表示します。")),
        ("list", [
            T("**{{KeOptOk}}** - well below the limit. **{{KeOptKnapp}}** - from 90 % of it (adjustable). **{{KeOptUeber}}** - over a limit the game enforces: the map does not load, crashes or loses something.",
              "**{{KeOptOk}}** - deutlich unter der Grenze. **{{KeOptKnapp}}** - ab 90 % davon (einstellbar). **{{KeOptUeber}}** - über einer Grenze, die das Spiel durchsetzt: die Karte lädt nicht, stürzt ab oder verliert etwas.",
              "**{{KeOptOk}}**——远低于限制。**{{KeOptKnapp}}**——达到限制的 90%(可调)。**{{KeOptUeber}}**——超出了游戏强制执行的限制:地图无法加载、崩溃或丢失内容。",
              "**{{KeOptOk}}**:上限を十分に下回っています。**{{KeOptKnapp}}**:上限の 90% 以上(変更可)。**{{KeOptUeber}}**:ゲームが強制する上限を超えています。マップが読み込めない、クラッシュする、または何かが失われます。"),
            T("**{{KeOptHinweis}}** - a format limit of the .bsp file (`MAX_MAP_*` in `qfiles.h`) that neither the engine nor q3map2 checks. Raven's own maps exceed several of them, so the optimizer never calls them over - it only notes them.",
              "**{{KeOptHinweis}}** - eine Formatgrenze der .bsp-Datei (`MAX_MAP_*` in `qfiles.h`), die weder die Engine noch q3map2 prüft. Ravens eigene Karten überschreiten mehrere davon, deshalb nennt der Optimierer sie nie „über“ - er weist nur darauf hin.",
              "**{{KeOptHinweis}}**——.bsp 文件的格式限制(`qfiles.h` 中的 `MAX_MAP_*`),引擎和 q3map2 都不会检查。Raven 自己的地图就超出了其中好几项,所以优化器从不把它们判为超出,只作提示。",
              "**{{KeOptHinweis}}**:.bsp ファイルの形式上の上限(`qfiles.h` の `MAX_MAP_*`)で、エンジンも q3map2 もチェックしません。Raven 自身のマップがいくつも超えているため、最適化ツールはこれを「超過」とは判定せず、注意として示すだけです。"),
            T("Values marked * are estimates (for example the entities that remain after spawning, or what a model costs). The column **{{KeOptSpAus}}** says whether a value was counted in the map or in the .bsp.",
              "Mit * markierte Werte sind Schätzungen (zum Beispiel die Entities, die nach dem Spawnen bleiben, oder was ein Modell kostet). Die Spalte **{{KeOptSpAus}}** sagt, ob ein Wert in der Karte oder in der .bsp gezählt wurde.",
              "带 * 的数值是估算值(例如生成后仍保留的实体数,或一个模型的开销)。**{{KeOptSpAus}}** 列表示该数值是在地图中还是在 .bsp 中统计的。",
              "* の付いた値は推定値です(例:スポーン後に残るエンティティ数や、モデル 1 つのコスト)。**{{KeOptSpAus}}** 列は、値をマップで数えたか .bsp で数えたかを示します。"),
            T("A click on a limit selects the largest contributors in the map and centers the views on them.",
              "Ein Klick auf eine Grenze wählt die größten Verbraucher in der Karte und zentriert die Ansichten auf sie.",
              "单击某项限制会选中地图中占用最多的对象,并让各视图居中显示它们。",
              "上限をクリックすると、マップ内で多く使っている対象が選択され、ビューがその位置に移動します。"),
        ]),
        ("note", T(
            "Every limit is taken from the source code, not estimated: OpenJK for Jedi Academy (single player `code/`, multiplayer `codemp/`), the source of Movie Duels for Movie Duels, OpenJK with `JK2_MODE` and `codeJK2/` for Jedi Outcast. Where a mod raises a limit, the optimizer still checks against the stricter vanilla value (for example 2048 instead of 4096 patch planes). The tables at the end of the page list each limit with file and line; the test `karte_grenzen` reads these lines from the sources again.",
            "Jede Grenze stammt aus dem Quelltext, nicht geschätzt: OpenJK für Jedi Academy (Einzelspieler `code/`, Mehrspieler `codemp/`), der Quelltext von Movie Duels für Movie Duels, OpenJK mit `JK2_MODE` und `codeJK2/` für Jedi Outcast. Wo ein Mod eine Grenze anhebt, prüft der Optimierer trotzdem gegen den strengeren Vanilla-Wert (zum Beispiel 2048 statt 4096 Patch-Ebenen). Die Tabellen am Ende der Seite nennen jede Grenze mit Datei und Zeile; der Test `karte_grenzen` liest diese Zeilen erneut aus den Quellen.",
            "每项限制都取自源码,而非估算:Jedi Academy 取自 OpenJK(单人 `code/`,多人 `codemp/`),Movie Duels 取自 Movie Duels 的源码,Jedi Outcast 取自带 `JK2_MODE` 的 OpenJK 和 `codeJK2/`。即使某个模组提高了限制,优化器仍按更严格的原版值检查(例如曲面片平面按 2048 而不是 4096)。本页末尾的表格列出了每项限制所在的文件和行号;测试 `karte_grenzen` 会再次从源码中读取这些行。",
            "上限はすべて推測ではなくソースコードから取っています。Jedi Academy は OpenJK(シングルプレイ `code/`、マルチプレイ `codemp/`)、Movie Duels は Movie Duels のソース、Jedi Outcast は `JK2_MODE` 付きの OpenJK と `codeJK2/` です。Mod が上限を引き上げている場合でも、最適化ツールはより厳しいバニラの値で確認します(例:パッチ平面は 4096 ではなく 2048)。ページ末尾の表に、各上限のファイルと行を載せています。テスト `karte_grenzen` がこれらの行をソースから読み直します。")),
        ("h2", "pruefungen", T("Checks and fixes", "Prüfungen und Behebungen", "检查与修复", "チェックと修正")),
        ("img", "h_karte5_pruefungen", T(
            "The tab **{{KeOptReiterBefunde}}**: each finding with severity, count and fix; on the right what it means, its source and the objects.",
            "Der Reiter **{{KeOptReiterBefunde}}**: jeder Befund mit Schwere, Anzahl und Behebung; rechts, was er bedeutet, seine Quelle und die Objekte.",
            "**{{KeOptReiterBefunde}}** 选项卡:每个检查结果都显示严重程度、数量和修复方式;右侧显示其含义、来源以及相关对象。",
            "**{{KeOptReiterBefunde}}** タブ。各検出結果を重大度・件数・修正方法とともに表示し、右側に意味・出典・対象オブジェクトを表示します。")),
        ("steps", [
            T("A click on a finding selects its objects and centers the 2D views and the camera on them.",
              "Ein Klick auf einen Befund wählt seine Objekte und zentriert die 2D-Ansichten und die Kamera auf sie.",
              "单击某个检查结果会选中相关对象,并让 2D 视图和摄像机居中显示它们。",
              "検出結果をクリックすると対象オブジェクトが選択され、2D ビューとカメラがその位置に移動します。"),
            (T("**{{KeOptBeheben}}** shows what would change; **{{KeOptAnwenden}}** does it. Every fix is one step of **Undo**, and the map is counted again afterwards - the finding disappears or shrinks.",
               "**{{KeOptBeheben}}** zeigt, was sich ändern würde; **{{KeOptAnwenden}}** tut es. Jede Behebung ist ein Schritt im **Rückgängig**, danach wird die Karte neu gezählt - der Befund verschwindet oder wird kleiner.",
               "**{{KeOptBeheben}}** 显示将要发生的更改;**{{KeOptAnwenden}}** 执行更改。每次修复都是 **撤销** 中的一步,修复后会重新统计地图——该检查结果会消失或减少。",
               "**{{KeOptBeheben}}** で何が変わるかを表示し、**{{KeOptAnwenden}}** で実行します。修正はそれぞれ **元に戻す** の 1 ステップで、修正後にマップを数え直します。検出結果は消えるか少なくなります。"),
             "h_karte5_beheben",
             T("The preview of a fix before it is applied.", "Die Vorschau einer Behebung, bevor sie angewendet wird.", "应用修复之前的预览。", "適用する前の修正のプレビュー。")),
            T("**{{KeOptAllesSichere}}** applies only the fixes marked {{KeOptSicher}} - the ones that cannot change how the map looks or plays - in a single undo step.",
              "**{{KeOptAllesSichere}}** wendet nur die als {{KeOptSicher}} markierten Behebungen an - die, die weder Aussehen noch Spiel der Karte ändern können - in einem einzigen Rückgängig-Schritt.",
              "**{{KeOptAllesSichere}}** 只应用标为 {{KeOptSicher}} 的修复(这些修复不会改变地图的外观或玩法),并合并为一个撤销步骤。",
              "**{{KeOptAllesSichere}}** は、{{KeOptSicher}} と示された修正(マップの見た目や遊び方を変えないもの)だけを、1 回の元に戻すステップで適用します。"),
            T("Fixes that set a worldspawn key (light grid, lightmaps) take effect with the next build; the console says so. **{{KeOptExport}}** writes everything with sources into a text file.",
              "Behebungen, die einen worldspawn-Schlüssel setzen (Lichtgitter, Lightmaps), wirken mit dem nächsten Bau; die Konsole sagt es. **{{KeOptExport}}** schreibt alles samt Quellen in eine Textdatei.",
              "设置 worldspawn 键的修复(光照网格、光照贴图)要在下次编译后才生效,控制台会给出提示。**{{KeOptExport}}** 会把全部内容连同来源写入一个文本文件。",
              "worldspawn のキーを設定する修正(ライトグリッド、ライトマップ)は次のビルドで反映されます。コンソールにもそう表示されます。**{{KeOptExport}}** は、出典を含むすべての内容をテキストファイルに書き出します。"),
        ]),
        ("table", [T("Check", "Prüfung", "检查", "チェック"), T("What it means", "Was es bedeutet", "含义", "意味"),
                   T("Fix", "Behebung", "修复", "修正"), T("Safe", "Sicher", "安全", "安全")],
         [_pruefzeile(*p) for p in _PRUEFUNGEN]),
        ("note", T(
            "Hint suggestions are only shown, never applied: where a hint brush helps depends on the gameplay. A detail suggestion for decoration is an estimate too - never make brushes detail that seal the map.",
            "Hint-Vorschläge werden nur gezeigt, nie angewendet: wo ein Hint-Pinsel hilft, hängt vom Spielablauf ab. Auch ein Detail-Vorschlag für Dekoration ist eine Schätzung - Pinsel, die die Karte abdichten, nie zu detail machen.",
            "Hint 建议只会显示,不会自动应用:hint 笔刷在哪里有用取决于玩法。对装饰物的 detail 建议同样只是估算——绝不要把封闭地图的笔刷设为 detail。",
            "hint の提案は表示するだけで、適用はしません。hint ブラシがどこで役立つかはゲームプレイ次第だからです。装飾に対する detail の提案も推定にすぎません。マップを密閉しているブラシは決して detail にしないでください。")),
        ("h2", "einstellungen", T("Settings", "Einstellungen", "设置", "設定")),
        ("img", "h_karte5_einstellungen", T(
            "{{KeLPreferences}}, page **{{KePgOptimizer}}** - a page of its own, NetRadiant-Custom has none.",
            "{{KeLPreferences}}, Seite **{{KePgOptimizer}}** - eine eigene Seite, NetRadiant-Custom hat keine.",
            "{{KeLPreferences}} 的 **{{KePgOptimizer}}** 页——独立的页面,NetRadiant-Custom 中没有。",
            "{{KeLPreferences}} の **{{KePgOptimizer}}** ページ。独自のページで、NetRadiant-Custom にはありません。")),
        ("p", T(
            "**{{KePrOptKnapp}}** sets from which share a limit counts as near, **{{KePrOptNachBau}}** runs the optimizer after every build, **{{KePrOptRegelwerk}}** fixes the rule set, **{{KePrOptPortale}}** and **{{KePrOptDreiecke}}** are the thresholds for the VIS and triangle notes.",
            "**{{KePrOptKnapp}}** legt fest, ab welchem Anteil eine Grenze als knapp gilt, **{{KePrOptNachBau}}** lässt den Optimierer nach jedem Bau laufen, **{{KePrOptRegelwerk}}** legt das Regelwerk fest, **{{KePrOptPortale}}** und **{{KePrOptDreiecke}}** sind die Schwellen für die Hinweise zu VIS und Dreiecken.",
            "**{{KePrOptKnapp}}** 设定占比达到多少时视为接近限制,**{{KePrOptNachBau}}** 让优化器在每次编译后运行,**{{KePrOptRegelwerk}}** 固定所用的规则集,**{{KePrOptPortale}}** 和 **{{KePrOptDreiecke}}** 是 VIS 和三角形提示的阈值。",
            "**{{KePrOptKnapp}}** は上限に近いとみなす割合、**{{KePrOptNachBau}}** はビルドのたびに最適化ツールを実行するかどうか、**{{KePrOptRegelwerk}}** は使うルールセット、**{{KePrOptPortale}}** と **{{KePrOptDreiecke}}** は VIS と三角形の注意を出すしきい値です。")),
        ("h2", "tabellen", T("The limits of each game", "Die Grenzen je Spiel", "各游戏的限制", "ゲームごとの上限")),
        ("p", T(
            "**{{KeOptSpErlaubt}}** is the value the optimizer checks against - one less than the define where the engine itself takes a slot (for example the world model or the box model), and the stricter vanilla value where a mod raised it. The source column names every place: the define and the line that stops the game.",
            "**{{KeOptSpErlaubt}}** ist der Wert, gegen den der Optimierer prüft - eins weniger als das Define, wo die Engine selbst einen Platz belegt (zum Beispiel das Weltmodell oder das Box-Modell), und der strengere Vanilla-Wert, wo ein Mod ihn angehoben hat. Die Spalte Quelle nennt jede Stelle: das Define und die Zeile, die das Spiel anhält.",
            "**{{KeOptSpErlaubt}}** 是优化器用来比较的值:如果引擎自己占用了一个位置(例如世界模型或盒子模型),就比定义值少一;如果模组提高了限制,则采用更严格的原版值。来源列列出所有相关位置:定义本身以及让游戏停止的那一行。",
            "**{{KeOptSpErlaubt}}** は最適化ツールが比較に使う値です。エンジン自身が 1 枠を使う場合(ワールドモデルやボックスモデルなど)は定義値より 1 少なく、Mod が引き上げている場合はより厳しいバニラの値になります。出典の列には、定義とゲームを停止させる行の両方を載せています。")),
    ] + _spieltabellen(),
}

SEITEN = [OPTIMIERER]
