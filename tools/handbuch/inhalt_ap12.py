"""AP12 (Auto-Skin, Kern): der Abschnitt "Model-Tab: Auto-Skin" - Gewichte eines Netzes automatisch aus dem Skelett, drei
Verfahren, die Grenzen des Spiels, was das Ergebnis taugt (gemessen an Ravens Figuren, docs/intern/NOTIZEN_ap12_1011.md).

Eigene Quelle des Pakets; AP14 haengt die Seite ins Handbuch (inhalt.py, inhalt_es.py, inhalt_cs.py) und nimmt die Bilder
beider Bedienmodi auf, sobald AP8 die Gruppe ins rechte Feld des Bereichs Weights gesetzt hat (bis dahin zeigt sie das
Fenster aus "Auto-Skin: Einstellungen"). Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in
inhalt_ph.py; {{Kennung}} setzt den Text der Oberflaeche ein.
"""

X = {
    "titel": ("Model tab: Auto-skin", "Model-Tab: Auto-Skin", "模型选项卡: 自动蒙皮", "モデルタブ: オートスキン", "Pestaña Model: Auto-skin",
              "Karta Model: Auto-skin"),
    "kurz": (
        "Give a mesh its bone weights with one click: from an original of the game with the same skeleton, or computed from the skeleton through the inside of the mesh. The result always fits the game: at most 4 weights per vertex, at most 32 bones per surface, stored in 10 bits.",
        "Einem Netz mit einem Klick seine Knochengewichte geben: von einem Original des Spiels mit demselben Skelett, oder aus dem Skelett gerechnet, durch das Innere des Netzes. Das Ergebnis passt immer zum Spiel: höchstens 4 Gewichte je Ecke, höchstens 32 Knochen je Fläche, in 10 Bit abgelegt.",
        "一键为网格计算骨骼权重: 取自具有相同骨架的游戏原版模型, 或根据骨架穿过网格内部计算。结果始终符合游戏: 每个顶点最多 4 个权重, 每个表面最多 32 根骨骼, 以 10 位存储。",
        "ワンクリックでメッシュにボーンウェイトを付けます: 同じスケルトンを持つゲームのオリジナルから、またはスケルトンからメッシュの内部を通して計算します。結果は常にゲームに合います: 頂点ごとに最大 4 個、サーフェスごとに最大 32 ボーン、10 ビットで保存。",
        "Da a una malla sus pesos de hueso con un clic: de un original del juego con el mismo esqueleto, o calculados a partir del esqueleto por el interior de la malla. El resultado siempre encaja con el juego: como máximo 4 pesos por vértice, como máximo 32 huesos por superficie, guardados en 10 bits.",
        "Dejte síti váhy kostí jedním kliknutím: z originálu ze hry se stejnou kostrou, nebo spočítané z kostry vnitřkem sítě. Výsledek vždy odpovídá hře: nejvýše 4 váhy na vrchol, nejvýše 32 kostí na povrch, uložené v 10 bitech."),
    "h_wo": ("Where to find it", "Wo es steht", "在哪里", "場所", "Dónde está", "Kde to je"),
    "wo": (
        "Open a model that has a skeleton (a .glm whose .gla is found). **{{AsAusfuehren}}** and **{{AsOeffnen}}** are in the search of all actions (X, F3 in "
        "Blender mode) and in the right-click of the view under Weights. **{{AsOeffnen}}** shows the window **{{AsGruppe}}** with all settings, the main button "
        "and the result of the last run. A run is one undo step; the animation keeps playing, so you see the new weights in the pose at once.",
        "Ein Modell mit Skelett öffnen (eine .glm, deren .gla gefunden wird). **{{AsAusfuehren}}** und **{{AsOeffnen}}** stehen in der Suche aller Aktionen "
        "(X, im Blender-Modus F3) und im Rechtsklick der Ansicht unter Weights. **{{AsOeffnen}}** zeigt das Fenster **{{AsGruppe}}** mit allen Einstellungen, "
        "dem Hauptknopf und dem Ergebnis des letzten Laufs. Ein Lauf ist ein Rückgängig-Schritt; die Animation läuft weiter, die neuen Gewichte sind also "
        "sofort in der Pose zu sehen.",
        "打开带骨架的模型 (能找到其 .gla 的 .glm)。**{{AsAusfuehren}}** 和 **{{AsOeffnen}}** 位于全部动作的搜索 (X, Blender 模式为 F3) 以及视图右键的 "
        "Weights 之下。**{{AsOeffnen}}** 显示窗口 **{{AsGruppe}}**, 其中有全部设置、主按钮和上次运行的结果。一次运行是一步撤销; 动画继续播放, 因此新的权重"
        "会立即显示在姿势中。",
        "スケルトン付きのモデル (.gla が見つかる .glm) を開きます。**{{AsAusfuehren}}** と **{{AsOeffnen}}** は、すべてのアクションの検索 (X、Blender 風は "
        "F3) と、ビューの右クリックの Weights の下にあります。**{{AsOeffnen}}** でウィンドウ **{{AsGruppe}}** が開き、すべての設定、メインボタン、前回の結果が"
        "表示されます。1 回の実行は元に戻す 1 回分です。アニメーションは再生し続けるので、新しいウェイトがすぐにポーズで見えます。",
        "Abre un modelo con esqueleto (un .glm cuyo .gla se encuentre). **{{AsAusfuehren}}** y **{{AsOeffnen}}** están en la búsqueda de todas las acciones "
        "(X, F3 en modo Blender) y en el clic derecho de la vista, bajo Weights. **{{AsOeffnen}}** muestra la ventana **{{AsGruppe}}** con todos los ajustes, el "
        "botón principal y el resultado de la última ejecución. Una ejecución es un paso de deshacer; la animación sigue reproduciéndose, así que los pesos "
        "nuevos se ven enseguida en la pose.",
        "Otevřete model s kostrou (.glm, jehož .gla se najde). **{{AsAusfuehren}}** a **{{AsOeffnen}}** jsou ve vyhledávání všech akcí (X, v režimu Blender "
        "F3) a v pravém kliknutí pohledu pod Weights. **{{AsOeffnen}}** ukáže okno **{{AsGruppe}}** se všemi nastaveními, hlavním tlačítkem a výsledkem "
        "posledního běhu. Běh je jeden krok zpět; animace běží dál, takže nové váhy jsou hned vidět v póze."),
    "h_verfahren": ("Three methods", "Drei Verfahren", "三种方法", "3 つの方式", "Tres métodos", "Tři metody"),
    "verfahren": (
        "**{{AsVerfUebertragen}}** takes the weights from the nearest point of an original with the same skeleton, surface by surface - type its path under "
        "**{{AsVorlage}}** (models/players/kyle/model.glm is preset). For a new player figure this is usually the best start, because the weights are the ones "
        "Raven's artists set, including the face. Points farther from the original than **{{AsAbstand}}** (a cloak, a piece of armour) get geodesic weights. "
        "**{{AsVerfVoxel}}** needs no original: it fills the model with voxels, including the caps, and measures the way from every point to every bone through "
        "the inside of the mesh, not through the air - an arm that lies close to the body keeps the weights of the arm. This is the method for creatures, props "
        "and figures with their own skeleton. **{{AsVerfHuelle}}** uses the straight distance; it is fast but mixes up parts that lie close together, a stopgap.",
        "**{{AsVerfUebertragen}}** nimmt die Gewichte vom nächsten Punkt eines Originals mit demselben Skelett, Fläche für Fläche - den Pfad unter "
        "**{{AsVorlage}}** eintragen (models/players/kyle/model.glm steht ab Werk da). Für eine neue Spielerfigur ist das meist der beste Anfang, denn die "
        "Gewichte sind die, die Ravens Grafiker gesetzt haben, samt Gesicht. Punkte, die weiter als **{{AsAbstand}}** vom Original liegen (ein Umhang, ein "
        "Rüstungsteil), bekommen geodätische Gewichte. **{{AsVerfVoxel}}** braucht kein Original: es füllt das Modell mit Voxeln, samt den Kappen, und misst "
        "den Weg von jedem Punkt zu jedem Knochen durch das Innere des Netzes, nicht durch die Luft - ein Arm, der nah am Körper liegt, behält die Gewichte "
        "des Arms. Das ist das Verfahren für Kreaturen, Requisiten und Figuren mit eigenem Skelett. **{{AsVerfHuelle}}** nimmt den geraden Abstand; es ist "
        "schnell, verwechselt aber Teile, die nah beieinander liegen - eine Notlösung.",
        "**{{AsVerfUebertragen}}** 从具有相同骨架的原版模型的最近点逐表面取得权重 - 在 **{{AsVorlage}}** 下填写其路径 (默认是 "
        "models/players/kyle/model.glm)。对于新的玩家角色, 这通常是最好的起点, 因为这些权重是 Raven 的美工设置的, 包括面部。距离原版超过 "
        "**{{AsAbstand}}** 的点 (披风、盔甲部件) 使用测地权重。**{{AsVerfVoxel}}** 不需要原版: 它用体素填充模型 (包括封盖), 并测量每个点到每根骨骼穿过网格"
        "内部而不是空气的路径 - 靠近身体的手臂保留手臂的权重。这是用于生物、道具和自带骨架的角色的方法。**{{AsVerfHuelle}}** 使用直线距离; 它很快, 但会"
        "混淆靠得很近的部位, 只是权宜之计。",
        "**{{AsVerfUebertragen}}** は、同じスケルトンを持つオリジナルの最も近い点から、サーフェスごとにウェイトを取ります。パスは **{{AsVorlage}}** に"
        "入力します (既定は models/players/kyle/model.glm)。新しいプレイヤーフィギュアにはたいてい最良の出発点です。Raven のアーティストが設定した"
        "ウェイトで、顔も含まれるからです。オリジナルから **{{AsAbstand}}** より遠い点 (マント、鎧のパーツ) には測地ウェイトが付きます。"
        "**{{AsVerfVoxel}}** はオリジナルを必要としません: キャップも含めてモデルをボクセルで埋め、各点から各ボーンまでの道のりを、空中ではなくメッシュの内部を"
        "通って測ります。体の近くにある腕も腕のウェイトを保ちます。クリーチャー、プロップ、独自のスケルトンを持つフィギュア向けの方式です。"
        "**{{AsVerfHuelle}}** は直線距離を使います。高速ですが、近くにあるパーツを取り違えます。応急処置です。",
        "**{{AsVerfUebertragen}}** toma los pesos del punto más cercano de un original con el mismo esqueleto, superficie por superficie; escribe su ruta en "
        "**{{AsVorlage}}** (de fábrica models/players/kyle/model.glm). Para una figura de jugador nueva suele ser el mejor comienzo, porque los pesos son los "
        "que pusieron los artistas de Raven, cara incluida. Los puntos a más de **{{AsAbstand}}** del original (una capa, una pieza de armadura) reciben "
        "pesos geodésicos. **{{AsVerfVoxel}}** no necesita original: llena el modelo de vóxeles, tapas incluidas, y mide el camino de cada punto a cada hueso "
        "por el interior de la malla, no por el aire; un brazo pegado al cuerpo conserva los pesos del brazo. Es el método para criaturas, objetos y figuras "
        "con esqueleto propio. **{{AsVerfHuelle}}** usa la distancia en línea recta; es rápido pero confunde partes cercanas, una solución de emergencia.",
        "**{{AsVerfUebertragen}}** vezme váhy z nejbližšího bodu originálu se stejnou kostrou, povrch po povrchu - cestu zapište do **{{AsVorlage}}** (z "
        "výroby models/players/kyle/model.glm). Pro novou postavu hráče je to většinou nejlepší začátek, protože váhy jsou ty, které nastavili grafici Ravenu, "
        "včetně obličeje. Body dál než **{{AsAbstand}}** od originálu (plášť, díl zbroje) dostanou geodetické váhy. **{{AsVerfVoxel}}** originál nepotřebuje: "
        "vyplní model voxely včetně záslepek a měří cestu od každého bodu ke každé kosti vnitřkem sítě, ne vzduchem - paže ležící blízko těla si ponechá váhy "
        "paže. To je metoda pro tvory, rekvizity a postavy s vlastní kostrou. **{{AsVerfHuelle}}** bere přímou vzdálenost; je rychlá, ale plete díly, které "
        "leží blízko sebe - nouzové řešení."),
    "h_einstellungen": ("Settings", "Einstellungen", "设置", "設定", "Ajustes", "Nastavení"),
    "einstellungen": (
        "**{{AsAufloesung}}** is the number of voxels along the longest side of the mesh (32 to 512); 256 suits a player figure. **{{AsSteifigkeit}}** sets how "
        "wide the blends between two bones are: 0 is soft, 1 puts each point almost entirely on its nearest bone. **{{AsGlaetten}}** averages each point with its "
        "neighbours, as many passes as set. **{{AsNaehte}}** gives vertices at the same place the same weights, also across surfaces - the .glm splits "
        "vertices at UV seams and where surfaces meet, and without this the surfaces can tear apart in motion. **{{AsDreh}}** lets the twist bones (lhumerusX, "
        "lradiusX, lfemurX) carry the half of their limb towards the next joint, as in Raven's figures. **{{AsDetail}}** also weights the face bones and coat "
        "tails; it is off by default, because their small areas cannot be found from the skeleton alone - for faces use **{{AsVerfUebertragen}}** or paint by "
        "hand. **{{AsSpiegeln}}** copies the side +X onto -X after the run. With **{{AsNurAuswahl}}** only the surfaces selected in the scene explorer are "
        "weighted, for example a new piece of armour on a finished figure; the number in brackets is the number of selected surfaces.",
        "**{{AsAufloesung}}** ist die Zahl der Voxel entlang der längsten Seite des Netzes (32 bis 512); 256 passt für eine Spielerfigur. "
        "**{{AsSteifigkeit}}** bestimmt, wie breit die Übergänge zwischen zwei Knochen sind: 0 ist weich, 1 legt jeden Punkt fast ganz an seinen nächsten "
        "Knochen. **{{AsGlaetten}}** mittelt jeden Punkt mit seinen Nachbarn, so viele Durchgänge wie eingestellt. **{{AsNaehte}}** gibt Ecken an derselben "
        "Stelle dieselben Gewichte, auch über Flächen hinweg - die .glm teilt Ecken an UV-Nähten und Flächengrenzen, und ohne das können die Flächen in der "
        "Bewegung aufreißen. **{{AsDreh}}** lässt die Drehknochen (lhumerusX, lradiusX, lfemurX) die Hälfte ihres Glieds zum nächsten Gelenk hin tragen, wie "
        "bei Ravens Figuren. **{{AsDetail}}** gewichtet auch die Gesichtsknochen und Rockschöße; es ist ab Werk aus, weil sich ihre kleinen Bereiche nicht "
        "allein aus dem Skelett finden lassen - für Gesichter **{{AsVerfUebertragen}}** nehmen oder von Hand malen. **{{AsSpiegeln}}** spiegelt nach dem Lauf "
        "die Seite +X auf -X. Mit **{{AsNurAuswahl}}** werden nur die im Szenen-Explorer gewählten Flächen gewichtet, zum Beispiel ein neues Rüstungsteil an "
        "einer fertigen Figur; die Zahl in Klammern ist die Zahl der gewählten Flächen.",
        "**{{AsAufloesung}}** 是沿网格最长边的体素数 (32 到 512); 256 适合玩家角色。**{{AsSteifigkeit}}** 决定两根骨骼之间过渡的宽度: 0 柔和, 1 使每个点"
        "几乎完全属于最近的骨骼。**{{AsGlaetten}}** 将每个点与其相邻点取平均, 次数如所设。**{{AsNaehte}}** 让同一位置的顶点得到相同的权重, 跨表面也是如此 - "
        ".glm 在 UV 接缝和表面交界处拆分顶点, 否则表面在运动中可能裂开。**{{AsDreh}}** 让扭转骨骼 (lhumerusX、lradiusX、lfemurX) 承载其肢体靠近下一关节的"
        "一半, 与 Raven 的角色相同。**{{AsDetail}}** 也为面部骨骼和衣摆计算权重; 默认关闭, 因为仅凭骨架无法找出它们的小范围 - 面部请使用 "
        "**{{AsVerfUebertragen}}** 或手工绘制。**{{AsSpiegeln}}** 在运行后将 +X 一侧复制到 -X。启用 **{{AsNurAuswahl}}** 时只为场景资源管理器中选中的表面计算"
        "权重, 例如成品角色上的一件新盔甲; 括号中的数字是所选表面的数量。",
        "**{{AsAufloesung}}** はメッシュの最も長い辺に沿ったボクセル数です (32 から 512)。プレイヤーフィギュアには 256 が合います。"
        "**{{AsSteifigkeit}}** は 2 本のボーン間のブレンドの幅を決めます: 0 は柔らかく、1 は各点をほぼ最も近いボーンだけに付けます。**{{AsGlaetten}}** は"
        "各点を隣の点と平均します。回数は設定どおりです。**{{AsNaehte}}** は同じ位置の頂点に、サーフェスをまたいでも同じウェイトを与えます。.glm は UV の"
        "継ぎ目とサーフェスの境界で頂点を分けるので、オフにすると動きの中でサーフェスが裂けることがあります。**{{AsDreh}}** は、Raven のフィギュアと同じく、"
        "ツイストボーン (lhumerusX、lradiusX、lfemurX) に手足の次の関節側の半分を持たせます。**{{AsDetail}}** は顔のボーンとコートの裾にもウェイトを付けます。"
        "これらの小さな範囲はスケルトンだけからは求められないため既定はオフです。顔には **{{AsVerfUebertragen}}** を使うか手で塗ってください。"
        "**{{AsSpiegeln}}** は実行後に +X 側を -X 側へコピーします。**{{AsNurAuswahl}}** をオンにすると、シーンエクスプローラーで選択したサーフェスだけに"
        "ウェイトを付けます (完成したフィギュアの新しい鎧のパーツなど)。括弧内の数字は選択したサーフェスの数です。",
        "**{{AsAufloesung}}** es el número de vóxeles a lo largo del lado más largo de la malla (de 32 a 512); 256 va bien para una figura de jugador. "
        "**{{AsSteifigkeit}}** fija lo anchas que son las transiciones entre dos huesos: 0 es blando, 1 deja cada punto casi por completo en su hueso más "
        "cercano. **{{AsGlaetten}}** promedia cada punto con sus vecinos, tantas pasadas como se indique. **{{AsNaehte}}** da los mismos pesos a los vértices "
        "que están en el mismo lugar, también entre superficies: el .glm divide los vértices en las costuras UV y donde se unen las superficies, y sin esto las "
        "superficies pueden abrirse al moverse. **{{AsDreh}}** hace que los huesos de torsión (lhumerusX, lradiusX, lfemurX) lleven la mitad de su miembro "
        "hacia la articulación siguiente, como en las figuras de Raven. **{{AsDetail}}** pesa también los huesos de la cara y los faldones; viene desactivado "
        "porque sus zonas pequeñas no se pueden deducir solo del esqueleto: para caras usa **{{AsVerfUebertragen}}** o pinta a mano. **{{AsSpiegeln}}** copia "
        "el lado +X sobre -X tras la ejecución. Con **{{AsNurAuswahl}}** solo se pesan las superficies seleccionadas en el explorador de escena, por ejemplo "
        "una pieza de armadura nueva en una figura terminada; el número entre paréntesis es el de superficies seleccionadas.",
        "**{{AsAufloesung}}** je počet voxelů podél nejdelší strany sítě (32 až 512); pro postavu hráče se hodí 256. **{{AsSteifigkeit}}** určuje, jak "
        "široké jsou přechody mezi dvěma kostmi: 0 je měkké, 1 přiřadí každý bod téměř celý jeho nejbližší kosti. **{{AsGlaetten}}** průměruje každý bod s "
        "jeho sousedy, tolik průchodů, kolik je nastaveno. **{{AsNaehte}}** dá vrcholům na stejném místě stejné váhy, i napříč povrchy - .glm dělí vrcholy "
        "na UV švech a na hranicích povrchů a bez toho se povrchy mohou v pohybu roztrhnout. **{{AsDreh}}** nechá otočné kosti (lhumerusX, lradiusX, lfemurX) "
        "nést polovinu jejich končetiny směrem k dalšímu kloubu, jako u postav od Ravenu. **{{AsDetail}}** váží i kosti obličeje a šosy; z výroby je vypnuto, "
        "protože jejich malé oblasti nelze najít jen z kostry - pro obličeje použijte **{{AsVerfUebertragen}}** nebo malujte ručně. **{{AsSpiegeln}}** po běhu "
        "zkopíruje stranu +X na -X. S **{{AsNurAuswahl}}** se váží jen povrchy vybrané v průzkumníku scény, například nový díl zbroje na hotové postavě; číslo "
        "v závorce je počet vybraných povrchů."),
    "h_ergebnis": ("The result and the limits of the game", "Das Ergebnis und die Grenzen des Spiels", "结果与游戏的限制", "結果とゲームの制限",
                   "El resultado y los límites del juego", "Výsledek a meze hry"),
    "ergebnis": (
        "Under **{{AsErgebnis}}** the window lists what the run produced. **{{AsErgGewichte}}**: a .glm carries at most 4 weights per vertex; where more bones "
        "pulled at a vertex, the smallest were dropped and the rest brought back to a sum of 1. **{{AsErgKnochen}}**: a surface can refer to at most 32 bones; "
        "if a surface had more, its weakest bones were dropped - at a seam the neighbouring surface changes with it, so the seam stays closed. "
        "**{{AsErgBit}}**: the weights are stored in 10 bits and the last one as the rest to 1, exactly as the game reads them - what you see is what the game "
        "shows. **{{AsErgAbweichung}}** compares with the weights the model had before the run: how far a vertex lies from its old place, over 200 frames "
        "spread across the animations of the skeleton, and at how many vertices the strongest bone stayed the same. For an original of the game this is the "
        "distance to Raven's weights. Highlighted lines name bones that lie outside the mesh and loose parts that no bone reaches through the mesh.",
        "Unter **{{AsErgebnis}}** nennt das Fenster, was der Lauf ergab. **{{AsErgGewichte}}**: eine .glm trägt höchstens 4 Gewichte je Ecke; wo mehr Knochen "
        "an einer Ecke zogen, fielen die kleinsten weg und der Rest wurde wieder auf die Summe 1 gebracht. **{{AsErgKnochen}}**: eine Fläche kann auf höchstens "
        "32 Knochen verweisen; hatte eine Fläche mehr, fielen ihre schwächsten Knochen weg - an einer Naht ändert sich die Nachbarfläche mit, die Naht bleibt "
        "also zu. **{{AsErgBit}}**: die Gewichte liegen in 10 Bit und das letzte als Rest zu 1, genau wie das Spiel sie liest - was du siehst, zeigt auch das "
        "Spiel. **{{AsErgAbweichung}}** vergleicht mit den Gewichten, die das Modell vor dem Lauf hatte: wie weit eine Ecke von ihrem alten Ort liegt, über "
        "200 Bilder, verteilt über die Animationen des Skeletts, und an wie vielen Ecken der stärkste Knochen gleich blieb. Bei einem Original des Spiels ist "
        "das der Abstand zu Ravens Gewichten. Hervorgehobene Zeilen nennen Knochen, die außerhalb des Netzes liegen, und lose Teile, die kein Knochen durch das Netz "
        "erreicht.",
        "窗口在 **{{AsErgebnis}}** 下列出运行的结果。**{{AsErgGewichte}}**: .glm 每个顶点最多承载 4 个权重; 当更多骨骼作用于一个顶点时, 最小的被去掉, 其余"
        "重新归一为总和 1。**{{AsErgKnochen}}**: 一个表面最多引用 32 根骨骼; 超过时去掉该表面最弱的骨骼 - 接缝处相邻表面随之改变, 因此接缝保持闭合。"
        "**{{AsErgBit}}**: 权重以 10 位存储, 最后一个为到 1 的余数, 与游戏读取的方式完全相同 - 所见即游戏所显示。**{{AsErgAbweichung}}** 与模型运行前的权重"
        "比较: 顶点离其旧位置有多远 (取骨架动画中均匀分布的 200 帧), 以及有多少顶点的最强骨骼保持不变。对于游戏原版模型, 这就是与 Raven 权重的距离。高亮的行"
        "指出位于网格之外的骨骼, 以及没有骨骼能通过网格到达的松散部件。",
        "ウィンドウの **{{AsErgebnis}}** に実行の結果が並びます。**{{AsErgGewichte}}**: .glm は頂点ごとに最大 4 個のウェイトしか持てません。それより多くの"
        "ボーンが頂点を引いていた場合は、最小のものを削除し、残りを合計 1 に戻します。**{{AsErgKnochen}}**: サーフェスが参照できるボーンは最大 32 本です。"
        "それより多い場合は、そのサーフェスの最も弱いボーンを削除します。継ぎ目では隣のサーフェスも一緒に変わるので、継ぎ目は閉じたままです。"
        "**{{AsErgBit}}**: ウェイトは 10 ビットで、最後の 1 つは 1 までの残りとして保存されます。ゲームが読むとおりなので、見えているものがゲームでも表示されます。"
        "**{{AsErgAbweichung}}** は、実行前にモデルが持っていたウェイトと比べます: 頂点が元の位置からどれだけ離れているか (スケルトンのアニメーション全体に"
        "散らした 200 フレーム)、そして最も強いボーンが同じままの頂点がどれだけあるか。ゲームのオリジナルでは Raven のウェイトとの差です。強調表示された行は、"
        "メッシュの外にあるボーンと、どのボーンもメッシュを通って届かない離れたパーツを示します。",
        "Bajo **{{AsErgebnis}}** la ventana muestra lo que dio la ejecución. **{{AsErgGewichte}}**: un .glm lleva como máximo 4 pesos por vértice; donde "
        "tiraban más huesos de un vértice, se quitaron los menores y el resto se llevó de nuevo a suma 1. **{{AsErgKnochen}}**: una superficie puede "
        "referirse como máximo a 32 huesos; si tenía más, se quitaron sus huesos más débiles: en una costura la superficie vecina cambia con ella, así que la "
        "costura sigue cerrada. **{{AsErgBit}}**: los pesos se guardan en 10 bits y el último como resto hasta 1, exactamente como los lee el juego: lo que ves "
        "es lo que muestra el juego. **{{AsErgAbweichung}}** compara con los pesos que tenía el modelo antes de la ejecución: a qué distancia queda un vértice "
        "de su lugar anterior, en 200 fotogramas repartidos por las animaciones del esqueleto, y en cuántos vértices el hueso más fuerte sigue siendo el mismo. "
        "En un original del juego es la distancia a los pesos de Raven. Las líneas resaltadas nombran huesos fuera de la malla y piezas sueltas a las que "
        "ningún hueso llega por la malla.",
        "Pod **{{AsErgebnis}}** okno uvádí, co běh přinesl. **{{AsErgGewichte}}**: .glm nese nejvýše 4 váhy na vrchol; kde za vrchol tahalo víc kostí, "
        "nejmenší odpadly a zbytek byl znovu upraven na součet 1. **{{AsErgKnochen}}**: povrch může odkazovat nejvýše na 32 kostí; měl-li jich víc, odpadly "
        "jeho nejslabší kosti - na švu se sousední povrch změní s ním, šev tedy zůstane zavřený. **{{AsErgBit}}**: váhy jsou uloženy v 10 bitech a poslední "
        "jako zbytek do 1, přesně jak je hra čte - co vidíte, ukáže i hra. **{{AsErgAbweichung}}** porovnává s váhami, které model měl před během: jak daleko "
        "leží vrchol od svého starého místa přes 200 snímků rozložených po animacích kostry a u kolika vrcholů zůstala nejsilnější kost stejná. U originálu "
        "ze hry je to vzdálenost od vah Ravenu. Zvýrazněné řádky jmenují kosti ležící mimo síť a volné díly, ke kterým žádná kost sítí nedosáhne."),
    "bild_fenster": ("The Auto-skin window after a run on kyle: settings, main button and the result with the limits of the game.",
                     "Das Fenster Auto-Skin nach einem Lauf an kyle: Einstellungen, Hauptknopf und das Ergebnis mit den Grenzen des Spiels.",
                     "在 kyle 上运行后的自动蒙皮窗口: 设置、主按钮以及带游戏限制的结果。",
                     "kyle で実行したあとのオートスキンのウィンドウ: 設定、メインボタン、ゲームの制限を含む結果。",
                     "La ventana Auto-skin tras una ejecución en kyle: ajustes, botón principal y el resultado con los límites del juego.",
                     "Okno Auto-skin po běhu na kyleovi: nastavení, hlavní tlačítko a výsledek s mezemi hry."),
}


def _t(schluessel):
    return X[schluessel][:4]


def seite():
    """Die Seite modell-autoskin (Gruppe Model, nach den Gewichten). Das Bild h_ap12_fenster nimmt AP14 auf."""
    return {
        "id": "modell-autoskin",
        "gruppe": "mission",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("h2", "wo", _t("h_wo")),
            ("p", _t("wo")),
            ("img", "h_ap12_fenster", _t("bild_fenster"), "breit"),
            ("h2", "verfahren", _t("h_verfahren")),
            ("p", _t("verfahren")),
            ("h2", "einstellungen", _t("h_einstellungen")),
            ("p", _t("einstellungen")),
            ("h2", "ergebnis", _t("h_ergebnis")),
            ("p", _t("ergebnis")),
        ],
    }


ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}
