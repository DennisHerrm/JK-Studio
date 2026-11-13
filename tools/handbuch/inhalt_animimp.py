"""Animations-Import: die Seite "Model-Tab: Animationen importieren und Preset-Bibliothek" (FBX/glTF/BVH/COLLADA als Quelle des
Retargetings, Mixamo auf einen Klick, Einheiten/Achsen/Bildrate, freie Presets von Quaternius und 100STYLE mit Lizenz).

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_rt.py.
"""

X = {
    "titel": ("Model tab: import animations and preset library", "Model-Tab: Animationen importieren und Preset-Bibliothek", "模型选项卡: 导入动画和预设库",
              "モデルタブ: アニメーションの読み込みとプリセットライブラリ", "Pestaña Model: importar animaciones y biblioteca de presets",
              "Karta Model: import animací a knihovna předvoleb"),
    "kurz": (
        "Bring motion from other programs onto your figure - a Mixamo FBX you downloaded yourself, glTF, BVH motion capture or COLLADA - and use the free preset library (Quaternius, 100STYLE) with one click.",
        "Hol Bewegungen aus anderen Programmen auf deine Figur - ein selbst geladenes Mixamo-FBX, glTF, BVH-Motion-Capture oder COLLADA - und nutze die freie Preset-Bibliothek (Quaternius, 100STYLE) mit einem Klick.",
        "把其他程序的动作带到你的角色上 - 你自己下载的 Mixamo FBX、glTF、BVH 动作捕捉或 COLLADA - 并一键使用免费的预设库 (Quaternius、100STYLE)。",
        "他のプログラムの動き (自分でダウンロードした Mixamo の FBX、glTF、BVH モーションキャプチャ、COLLADA) をフィギュアに載せ、無料のプリセットライブラリ (Quaternius、100STYLE) をワンクリックで使えます。",
        "Trae movimiento de otros programas a tu figura (un FBX de Mixamo que hayas descargado tú, glTF, captura BVH o COLLADA) y usa la biblioteca de presets libre (Quaternius, 100STYLE) con un clic.",
        "Přeneste pohyb z jiných programů na postavu - sami stažený Mixamo FBX, glTF, BVH motion capture nebo COLLADA - a používejte volnou knihovnu předvoleb (Quaternius, 100STYLE) jedním klikem."),
    "h_import": ("Import an animation", "Eine Animation importieren", "导入动画", "アニメーションを読み込む", "Importar una animación", "Import animace"),
    "import": (
        "In the area **Animation** of the model tab choose **{{AiImport}}** - in the group **{{AzRUebertragen}}** of the panel on the right, in the menu **Animation > {{RtRollout}}**, in the right-click of the view (**Control Rig > {{RtRollout}}**) or in the search (X, F3 in Blender mode). JK Studio reads skeleton and keys of the file, recognises known skeletons (Mixamo/HumanIK, Biped, CAT) and maps their chains, transfers the motion to your figure and plays it: a Mixamo FBX is on your figure with one click. Every animation of the file becomes a sequence of the source. Then **{{RtBackenMax}}** writes it into the slot you choose, as with every retargeting.",
        "Wähle im Bereich **Animation** des Model-Tabs **{{AiImport}}** - in der Gruppe **{{AzRUebertragen}}** des rechten Felds, im Menü **Animation > {{RtRollout}}**, im Rechtsklick der Ansicht (**Control Rig > {{RtRollout}}**) oder in der Suche (X, im Blender-Modus F3). JK Studio liest Skelett und Schlüssel der Datei, erkennt bekannte Skelette (Mixamo/HumanIK, Biped, CAT) und ordnet ihre Ketten zu, überträgt die Bewegung auf deine Figur und spielt sie ab: ein Mixamo-FBX ist mit einem Klick auf deiner Figur. Jede Animation der Datei wird eine Sequenz der Quelle. Danach schreibt **{{RtBackenMax}}** sie auf den Platz deiner Wahl, wie bei jedem Retargeting.",
        "在模型选项卡的 **Animation** 区域选择 **{{AiImport}}** - 在右侧面板的 **{{AzRUebertragen}}** 分组、菜单 **Animation > {{RtRollout}}**、视图右键 (**Control Rig > {{RtRollout}}**) 或搜索 (X, Blender 模式为 F3) 中。JK Studio 读取文件的骨架和关键帧, 识别已知骨架 (Mixamo/HumanIK、Biped、CAT) 并映射其链条, 把动作传到角色上并播放: Mixamo FBX 一键即可放到角色上。文件中的每个动画都成为来源的一个序列。然后 **{{RtBackenMax}}** 把它写入你选择的槽位, 与所有重定向相同。",
        "モデルタブの **Animation** の領域で **{{AiImport}}** を選びます - 右側のパネルのグループ **{{AzRUebertragen}}**、メニュー **Animation > {{RtRollout}}**、ビューの右クリック (**Control Rig > {{RtRollout}}**)、または検索 (X、Blender 風は F3)。JK Studio はファイルの骨格とキーを読み、既知の骨格 (Mixamo/HumanIK、Biped、CAT) を認識してチェーンを対応付け、動きをフィギュアに移して再生します: Mixamo の FBX はワンクリックでフィギュアに載ります。ファイルの各アニメーションはソースのシーケンスになります。そのあと **{{RtBackenMax}}** で、他のリターゲットと同じく選んだスロットに書き込みます。",
        "En el área **Animation** de la pestaña Model elige **{{AiImport}}** (en el grupo **{{AzRUebertragen}}** del panel de la derecha, en el menú **Animation > {{RtRollout}}**, en el clic derecho de la vista (**Control Rig > {{RtRollout}}**) o en la búsqueda (X, F3 en modo Blender)). JK Studio lee el esqueleto y las claves del archivo, reconoce esqueletos conocidos (Mixamo/HumanIK, Biped, CAT) y asigna sus cadenas, pasa el movimiento a tu figura y lo reproduce: un FBX de Mixamo está en tu figura con un clic. Cada animación del archivo es una secuencia del origen. Después **{{RtBackenMax}}** la escribe en el hueco que elijas, como en todo retargeting.",
        "V oblasti **Animation** karty Model zvolte **{{AiImport}}** - ve skupině **{{AzRUebertragen}}** panelu vpravo, v nabídce **Animation > {{RtRollout}}**, v pravém kliknutí pohledu (**Control Rig > {{RtRollout}}**) nebo ve vyhledávání (X, v režimu Blender F3). JK Studio načte kostru a klíče souboru, rozpozná známé kostry (Mixamo/HumanIK, Biped, CAT) a přiřadí jejich řetězy, přenese pohyb na postavu a přehraje ho: Mixamo FBX je na postavě jedním klikem. Každá animace souboru se stane sekvencí zdroje. Potom ji **{{RtBackenMax}}** zapíše do zvolené pozice jako u každého retargetingu.",
    ),
    "h_einheiten": ("Units, axes and frame rate", "Einheiten, Achsen und Bildrate", "单位、轴向和帧率", "単位・軸・フレームレート", "Unidades, ejes y fotogramas", "Jednotky, osy a snímková frekvence"),
    "einheiten": (
        "Each format brings its own conventions, and JK Studio takes them from the file: FBX names its up, front and side axis and its unit (centimetres per unit) in "
        "its global settings; glTF is always metres with +Y up and the front towards +Z; COLLADA states its unit and up axis; BVH states neither, so the up axis is "
        "the one along which the body stands and left comes from the bone names. Everything is turned into the frame of the game (Z up, figure looking along -Y). "
        "The frame rate is the one of the file (FBX time mode, BVH frame time); glTF and COLLADA have none, there it follows from the spacing of the keys. Below the "
        "source the rollout shows what was read. Different sizes do not matter: the retargeting scales the steps with the leg length.",
        "Jedes Format bringt eigene Regeln mit, und JK Studio nimmt sie aus der Datei: FBX nennt Achse oben, vorn und seitlich und seine Einheit (Zentimeter je Einheit) "
        "in den globalen Einstellungen; glTF ist immer Meter mit +Y oben und der Vorderseite nach +Z; COLLADA nennt Einheit und Achse oben; BVH nennt keins von beiden, "
        "dort ist oben die Achse, entlang der der Körper steht, und links kommt aus den Knochennamen. Alles wird in den Rahmen des Spiels gedreht (Z oben, Figur blickt "
        "nach -Y). Die Bildrate ist die der Datei (FBX-Zeitmodus, BVH-Frame-Time); glTF und COLLADA haben keine, dort folgt sie aus dem Abstand der Schlüssel. Unter der "
        "Quelle zeigt das Rollout, was gelesen wurde. Verschiedene Größen stören nicht: das Retargeting skaliert die Schritte mit der Beinlänge.",
        "每种格式都有自己的约定, JK Studio 从文件中读取: FBX 在全局设置中给出上、前、侧轴及单位 (每单位厘米); glTF 始终为米, +Y 向上, 正面朝 +Z; COLLADA 给出单位和上轴; "
        "BVH 两者都不给, 此时上轴是身体站立所沿的轴, 左侧由骨骼名称得出。所有内容都转到游戏的坐标系 (Z 向上, 角色朝 -Y)。帧率取自文件 (FBX 时间模式、BVH 帧时间); glTF "
        "和 COLLADA 没有帧率, 由关键帧间距得出。卷展栏在来源下方显示读到的内容。大小不同无妨: 重定向按腿长缩放步幅。",
        "形式ごとに独自の決まりがあり、JK Studio はそれをファイルから取ります: FBX はグローバル設定に上・前・横の軸と単位 (1 単位あたりの cm) を持ち、glTF は常にメートルで "
        "+Y が上、正面は +Z、COLLADA は単位と上の軸を示し、BVH はどちらも示さないので、体が立つ方向の軸を上とし、左はボーン名から決めます。すべてゲームの座標 (Z が上、"
        "フィギュアは -Y を向く) に回します。フレームレートはファイルのもの (FBX のタイムモード、BVH の Frame Time)、glTF と COLLADA には無いのでキーの間隔から求めます。"
        "ロールアウトのソースの下に読み取った内容が出ます。大きさの違いは問題ありません: リターゲットが脚の長さで歩幅を拡縮します。",
        "Cada formato trae sus reglas y JK Studio las toma del archivo: FBX indica en sus ajustes globales los ejes arriba, delante y lateral y su unidad (centímetros por "
        "unidad); glTF es siempre metros con +Y arriba y el frente hacia +Z; COLLADA indica unidad y eje arriba; BVH no indica ninguna, así que arriba es el eje a lo largo "
        "del que está el cuerpo y la izquierda sale de los nombres de los huesos. Todo se gira al marco del juego (Z arriba, la figura mira hacia -Y). Los fotogramas por "
        "segundo son los del archivo (modo de tiempo FBX, Frame Time de BVH); glTF y COLLADA no tienen, ahí salen de la separación de las claves. Bajo el origen el panel "
        "muestra lo leído. Los tamaños distintos no importan: el retargeting escala los pasos con la longitud de las piernas.",
        "Každý formát má svá pravidla a JK Studio je bere ze souboru: FBX uvádí v globálním nastavení osu nahoru, vpřed a do strany a jednotku (centimetry na jednotku); "
        "glTF je vždy v metrech s +Y nahoru a přední stranou k +Z; COLLADA uvádí jednotku a osu nahoru; BVH neuvádí nic, proto je nahoru osa, podél které tělo stojí, a "
        "levá strana plyne z názvů kostí. Vše se otočí do soustavy hry (Z nahoru, postava hledí k -Y). Snímková frekvence je ze souboru (časový režim FBX, Frame Time "
        "BVH); glTF a COLLADA ji nemají, tam plyne z rozestupu klíčů. Pod zdrojem rollout ukazuje, co bylo načteno. Různé velikosti nevadí: retargeting škáluje kroky "
        "délkou nohou."),
    "h_bibliothek": ("The preset library", "Die Preset-Bibliothek", "预设库", "プリセットライブラリ", "La biblioteca de presets", "Knihovna předvoleb"),
    "bibliothek": (
        "The group **{{PrRollout}}** starts with the library: choose a category, click an animation (a double-click shows it on your figure at once, like "
        "**{{AzPresetVorschau}}**), check the **{{AzZielPlatz}}** (a game name such as BOTH_WALK1 is suggested) and click **{{AzPresetAuflegen}}**. The eight "
        "categories of the design always show; one without presets yet shows a short note. In {{AzKatGehen}} and {{AzKatLaufen}} the list also has **{{EnGangGehen}}** and "
        "**{{EnGangLaufen}}** from the own generator (**{{PrTempo}}** and **{{PrArm}}** under the list, the rest with **{{AzGenerator}}**). Loops are cut "
        "to one seamless cycle and play in place like the walks of the game. The animations come from Quaternius (Universal Animation Library, CC0) and from "
        "\"The 100STYLE Dataset - Ian Mason\" (CC BY 4.0, motion capture); every entry names its source and licence, all of it is listed in data/presets/CREDITS.txt "
        "and in the About window. JK Studio ships them on their own skeletons, never with a file of the game.",
        "Die Gruppe **{{PrRollout}}** beginnt mit der Bibliothek: eine Kategorie wählen, eine Animation anklicken (ein Doppelklick zeigt sie sofort auf deiner Figur, wie "
        "**{{AzPresetVorschau}}**), den **{{AzZielPlatz}}** prüfen (ein Name des Spiels wie BOTH_WALK1 ist vorgeschlagen) und **{{AzPresetAuflegen}}** klicken. "
        "Die acht Kategorien des Entwurfs stehen immer da; eine ohne Presets zeigt einen kurzen Hinweis. In {{AzKatGehen}} und {{AzKatLaufen}} stehen dazu **{{EnGangGehen}}** und "
        "**{{EnGangLaufen}}** aus dem eigenen Generator (**{{PrTempo}}** und **{{PrArm}}** unter der Liste, der Rest über **{{AzGenerator}}**). Schleifen sind auf einen nahtlosen Zyklus geschnitten und spielen auf der Stelle wie die Gänge des Spiels. Die Animationen stammen von "
        "Quaternius (Universal Animation Library, CC0) und aus \"The 100STYLE Dataset - Ian Mason\" (CC BY 4.0, Motion-Capture); jeder Eintrag nennt Quelle und Lizenz, "
        "alles steht in data/presets/CREDITS.txt und im Über-Fenster. JK Studio liefert sie auf ihren eigenen Skeletten aus, nie mit einer Datei des Spiels.",
        "**{{PrRollout}}** 组以动画库开头: 选择一个类别, 点击一个动画 (双击会立即在角色上显示, 与 **{{AzPresetVorschau}}** 相同), 检查 **{{AzZielPlatz}}** "
        "(会建议游戏中的名称如 BOTH_WALK1), 然后点击 **{{AzPresetAuflegen}}**。设计中的八个类别始终显示; 还没有预设的类别会显示简短提示。{{AzKatGehen}} 和 {{AzKatLaufen}} 中还有自带生成器的 **{{EnGangGehen}}** 和 **{{EnGangLaufen}}** (列表下方为 **{{PrTempo}}** 和 **{{PrArm}}**, 其余在 **{{AzGenerator}}** 中)。循环被剪成一个无缝周期, 像游戏中的步行一样原地播放。动画来自 Quaternius (Universal "
        "Animation Library, CC0) 和 \"The 100STYLE Dataset - Ian Mason\" (CC BY 4.0, 动作捕捉); 每个条目都标明来源和许可, 全部列在 data/presets/CREDITS.txt 和关于窗口中。"
        "JK Studio 以它们自己的骨架提供, 从不附带游戏文件。",
        "グループ **{{PrRollout}}** はライブラリから始まります: カテゴリを選び、アニメーションをクリック (ダブルクリックで **{{AzPresetVorschau}}** と同じくすぐフィギュアに表示)、"
        "**{{AzZielPlatz}}** を確認 (BOTH_WALK1 などゲームの名前を提案) して **{{AzPresetAuflegen}}** をクリックします。デザインの 8 つのカテゴリは常に表示され、まだプリセットのないカテゴリには短い案内が出ます。{{AzKatGehen}} と {{AzKatLaufen}} には独自ジェネレーターの **{{EnGangGehen}}** と **{{EnGangLaufen}}** もあります (一覧の下に **{{PrTempo}}** と **{{PrArm}}**、その他は **{{AzGenerator}}**)。ループは継ぎ目"
        "のない 1 サイクルに切られ、ゲームの歩きのようにその場で再生します。アニメーションは Quaternius (Universal Animation Library、CC0) と \"The 100STYLE Dataset - Ian "
        "Mason\" (CC BY 4.0、モーションキャプチャ) のものです。各項目に出典とライセンスがあり、すべて data/presets/CREDITS.txt と「バージョン情報」にあります。JK Studio "
        "は元の骨格のまま同梱し、ゲームのファイルは含みません。",
        "El grupo **{{PrRollout}}** empieza con la biblioteca: elige una categoría, haz clic en una animación (un doble clic la muestra al instante en tu figura, como "
        "**{{AzPresetVorschau}}**), revisa el **{{AzZielPlatz}}** (se sugiere un nombre del juego como BOTH_WALK1) y pulsa **{{AzPresetAuflegen}}**. Las ocho "
        "categorías del diseño están siempre; una sin preajustes muestra una nota breve. En {{AzKatGehen}} y {{AzKatLaufen}} la lista tiene además **{{EnGangGehen}}** y "
        "**{{EnGangLaufen}}** del generador propio (**{{PrTempo}}** y **{{PrArm}}** bajo la lista, el resto con **{{AzGenerator}}**). Los bucles "
        "están cortados a un ciclo sin costura y se reproducen en el sitio como los pasos del juego. Las animaciones son de Quaternius (Universal Animation Library, CC0) y "
        "de \"The 100STYLE Dataset - Ian Mason\" (CC BY 4.0, captura de movimiento); cada entrada indica origen y licencia, todo figura en data/presets/CREDITS.txt y en la "
        "ventana Acerca de. JK Studio las incluye en sus propios esqueletos, nunca con un archivo del juego.",
        "Skupina **{{PrRollout}}** začíná knihovnou: zvolte kategorii, klikněte na animaci (dvojklik ji hned ukáže na postavě, stejně jako **{{AzPresetVorschau}}**), "
        "zkontrolujte **{{AzZielPlatz}}** (navrhne se název hry jako BOTH_WALK1) a klikněte na **{{AzPresetAuflegen}}**. Osm kategorií návrhu je vždy vidět; kategorie bez předvoleb ukáže krátkou poznámku. V {{AzKatGehen}} a {{AzKatLaufen}} je navíc **{{EnGangGehen}}** a "
        "**{{EnGangLaufen}}** z vlastního generátoru (**{{PrTempo}}** a **{{PrArm}}** pod seznamem, zbytek přes **{{AzGenerator}}**). Smyčky jsou střiženy na jeden "
        "plynulý cyklus a hrají na místě jako chůze ve hře. Animace pocházejí od Quaterniuse (Universal Animation Library, CC0) a z \"The 100STYLE Dataset - Ian Mason\" "
        "(CC BY 4.0, motion capture); každá položka uvádí zdroj a licenci, vše je v data/presets/CREDITS.txt a v okně O programu. JK Studio je dodává na jejich vlastních "
        "kostrách, nikdy se souborem hry."),
    "bild_bibliothek": ("A motion capture walk from the library (100STYLE) on the stormtrooper, group Presets with the library at the top.",
                        "Ein Motion-Capture-Gang aus der Bibliothek (100STYLE) auf dem Sturmtruppler, Gruppe Presets mit der Bibliothek oben.",
                        "库中的动作捕捉步行 (100STYLE) 应用在冲锋队员上, 预设组顶部是动画库。",
                        "ライブラリのモーションキャプチャの歩き (100STYLE) をストームトルーパーに、グループ「プリセット」の上部にライブラリ。",
                        "Un paso de captura de movimiento de la biblioteca (100STYLE) en el soldado de asalto, grupo Presets con la biblioteca arriba.",
                        "Chůze z motion capture z knihovny (100STYLE) na stormtrooperovi, skupina Předvolby s knihovnou nahoře."),
}


def _t(schluessel):
    from inhalt_basis import T
    z = X[schluessel]
    return T(z[0], z[1], z[2], z[3])


def seite():
    """Die Seite modell-animimport (Gruppe Missionen, nach modell-retarget)."""
    return {
        "id": "modell-animimport",
        "gruppe": "mission",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("h2", "import", _t("h_import")),
            ("p", _t("import")),
            ("h2", "einheiten", _t("h_einheiten")),
            ("p", _t("einheiten")),
            ("h2", "bibliothek", _t("h_bibliothek")),
            ("p", _t("bibliothek")),
            ("img", "h_ai_bibliothek", _t("bild_bibliothek"), "breit"),
        ],
    }


ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}
