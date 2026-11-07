"""Modell-Block Stufe 7: die Seite "Mission-Tab: Figuren animieren" (eine Figur auf der Karte posieren wie im Model-Tab, Wurzelweg
und SET_ORIGIN, Fuer das Spiel bauen mit Karten-GLA, animation.cfg und ICARUS, mehrere Figuren mit signal/waitsignal und Griffen an
anderen Figuren).

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_mb4.py.
"""

X = {
    "titel": ("Mission tab: animating figures", "Mission-Tab: Figuren animieren", "任务选项卡: 角色动画", "ミッションタブ: フィギュアのアニメーション",
              "Pestaña Mission: animar figuras", "Karta Mission: animace postav"),
    "kurz": (
        "Pose a figure of the scene right on the map with the same controls as in the model tab, let its pelvis path become its way, and build the map GLA, the animation.cfg and the ICARUS script for the game - for one figure or several that wait for each other.",
        "Eine Figur der Szene direkt auf der Karte mit derselben Bedienung wie im Model-Tab posieren, ihren Beckenweg zu ihrem Weg machen und Karten-GLA, animation.cfg und ICARUS-Skript für das Spiel bauen - für eine Figur oder mehrere, die aufeinander warten.",
        "直接在地图上用与模型选项卡相同的操作为场景中的角色摆姿势, 让骨盆路径成为它的行进路线, 并为游戏生成地图 GLA、animation.cfg 和 ICARUS 脚本 - 可用于一个角色, 或互相等待的多个角色。",
        "シーンのフィギュアをモデルタブと同じ操作でマップ上に直接ポーズし、骨盤の経路をその道筋にして、ゲーム用のマップ GLA・animation.cfg・ICARUS スクリプトをビルドします - 1 体でも、互いを待つ複数体でも。",
        "Posa una figura de la escena directamente en el mapa con los mismos controles que en la pestaña Model, haz que el trayecto de su pelvis sea su camino y crea el GLA de mapa, animation.cfg y el script ICARUS para el juego, para una figura o varias que se esperan.",
        "Pózujte postavu scény přímo na mapě stejným ovládáním jako v kartě Model, nechte dráhu její pánve být její cestou a sestavte mapové GLA, animation.cfg a skript ICARUS pro hru - pro jednu postavu nebo více, které na sebe čekají."),
    "h_eine": ("One figure", "Eine Figur", "一个角色", "1 体のフィギュア", "Una figura", "Jedna postava"),
    "eine": (
        "Select a figure (an NPC_spawner) and click **{{FaAnimieren}}** in the inspector, in its right-click menu or in the window **{{FaTitel}}**. From the time cursor on "
        "the figure gets its own animation: a layer over a base animation of its skeleton (**{{FaBasis}}**, e.g. BOTH_STAND1), with **{{FaBilder}}** frames at 20 per "
        "second. The controls of the control rig appear on the figure in the map. **Like 3ds Max** (default): Q selects, W moves, E rotates, F5/F6/F7 lock to X/Y/Z of "
        "the map, K sets a key. Moving a hand or foot is IK - its target moves in the plane of the view; moving the pelvis moves the whole figure; rotating a limb "
        "under IK switches to FK first without a jump (**{{MbIkAuto}}**). **Like Blender:** G and R work modally with X/Y/Z, a click takes it, Esc cancels; I sets a "
        "key. **{{FaFussBoden}}** puts the foot onto the floor of the map, **{{FaZielFlaeche}}** puts a hand onto a door, a console or any surface you click. Every "
        "drag is one undo step. The dope sheet in the window has a row for every animated bone and every IK limb; dragging a key moves the IK keys with the FK keys "
        "of their chain. All actions are also in the menu of the window, in the search (X, F3 in Blender mode) and in the right-click on a control.",
        "Eine Figur (einen NPC_spawner) wählen und **{{FaAnimieren}}** klicken - im Inspektor, im Rechtsklick-Menü oder im Fenster **{{FaTitel}}**. Ab dem Zeitzeiger "
        "bekommt die Figur eine eigene Animation: eine Schicht über einer Grundanimation ihres Skeletts (**{{FaBasis}}**, z. B. BOTH_STAND1) mit **{{FaBilder}}** "
        "Bildern, 20 je Sekunde. Die Griffe des Control Rigs erscheinen auf der Figur in der Karte. **Wie 3ds Max** (Vorgabe): Q wählt, W verschiebt, E dreht, "
        "F5/F6/F7 sperren auf X/Y/Z der Karte, K setzt einen Schlüssel. Hand oder Fuß verschieben ist IK - das Ziel wandert in der Bildebene; das Becken verschieben "
        "bewegt die ganze Figur; ein Glied unter IK drehen wechselt vorher ohne Sprung auf FK (**{{MbIkAuto}}**). **Wie Blender:** G und R modal mit X/Y/Z, ein "
        "Klick übernimmt, Esc bricht ab; I setzt einen Schlüssel. **{{FaFussBoden}}** setzt den Fuß auf den Boden der Karte, **{{FaZielFlaeche}}** legt eine Hand "
        "an eine Tür, eine Konsole oder jede Fläche, die man anklickt. Jeder Zug ist ein Rückgängig-Schritt. Das Dopesheet im Fenster hat eine Zeile je animiertem "
        "Knochen und je IK-Glied; ein gezogener Schlüssel nimmt die IK-Schlüssel mit den FK-Schlüsseln ihrer Kette mit. Alle Aktionen stehen auch im Menü des "
        "Fensters, in der Suche (X, im Blender-Modus F3) und im Rechtsklick auf einen Griff.",
        "选择一个角色 (NPC_spawner), 在检查器、右键菜单或窗口 **{{FaTitel}}** 中点击 **{{FaAnimieren}}**。从时间指针起角色获得自己的动画: 在其骨架的一个"
        "基础动画 (**{{FaBasis}}**, 例如 BOTH_STAND1) 上的一层, 共 **{{FaBilder}}** 帧, 每秒 20 帧。控制绑定的控制器显示在地图中的角色上。**像 3ds Max** "
        "(默认): Q 选择, W 移动, E 旋转, F5/F6/F7 锁定到地图的 X/Y/Z, K 设置关键帧。移动手或脚就是 IK - 目标在视图平面中移动; 移动骨盆则移动整个角色; 旋转"
        "处于 IK 的肢体会先无跳变地切换到 FK (**{{MbIkAuto}}**)。**像 Blender:** G 和 R 以模态方式配合 X/Y/Z, 点击确认, Esc 取消; I 设置关键帧。"
        "**{{FaFussBoden}}** 把脚放到地图地面上, **{{FaZielFlaeche}}** 把手放到你点击的门、控制台或任意表面上。每次拖动都是一步撤销。窗口中的摄影表为每个"
        "有动画的骨骼和每个 IK 肢体各有一行; 拖动关键帧时, IK 关键帧会随其链的 FK 关键帧一起移动。所有操作也在窗口菜单、搜索 (X, Blender 模式为 F3) 和"
        "控制器的右键中。",
        "フィギュア (NPC_spawner) を選び、インスペクター、右クリックメニュー、またはウィンドウ **{{FaTitel}}** で **{{FaAnimieren}}** をクリックします。"
        "タイムカーソルからフィギュアに独自のアニメーションが付きます: スケルトンのベースアニメーション (**{{FaBasis}}**、例 BOTH_STAND1) の上のレイヤーで、"
        "**{{FaBilder}}** フレーム、毎秒 20。コントロールリグのコントロールがマップ上のフィギュアに表示されます。**3ds Max 風** (既定): Q 選択、W 移動、E 回転、"
        "F5/F6/F7 でマップの X/Y/Z に固定、K でキー。手や足の移動は IK - ターゲットがビュー平面で動きます。骨盤の移動はフィギュア全体を動かし、IK 中の肢を"
        "回すと先に飛びなく FK に切り替わります (**{{MbIkAuto}}**)。**Blender 風:** G と R はモーダルで X/Y/Z、クリックで確定、Esc で中止。I でキー。"
        "**{{FaFussBoden}}** は足をマップの床に、**{{FaZielFlaeche}}** は手をクリックしたドア・コンソール・任意の面に置きます。各ドラッグは元に戻す 1 回分。"
        "ウィンドウのドープシートにはアニメーションのある各ボーンと各 IK 肢の行があり、キーをドラッグすると IK キーがチェーンの FK キーと一緒に動きます。"
        "すべての操作はウィンドウのメニュー、検索 (X、Blender 風は F3)、コントロールの右クリックにもあります。",
        "Elige una figura (un NPC_spawner) y pulsa **{{FaAnimieren}}** en el inspector, en su menú del clic derecho o en la ventana **{{FaTitel}}**. Desde el cursor de "
        "tiempo la figura tiene su propia animación: una capa sobre una animación base de su esqueleto (**{{FaBasis}}**, p. ej. BOTH_STAND1), con **{{FaBilder}}** "
        "fotogramas a 20 por segundo. Los controles del rig de control aparecen sobre la figura en el mapa. **Como 3ds Max** (por defecto): Q selecciona, W mueve, E "
        "rota, F5/F6/F7 bloquean en X/Y/Z del mapa, K fija una clave. Mover una mano o un pie es IK: su objetivo se mueve en el plano de la vista; mover la pelvis "
        "mueve toda la figura; rotar un miembro en IK pasa antes a FK sin salto (**{{MbIkAuto}}**). **Como Blender:** G y R funcionan en modo modal con X/Y/Z, un "
        "clic confirma y Esc cancela; I fija una clave. **{{FaFussBoden}}** pone el pie en el suelo del mapa, **{{FaZielFlaeche}}** pone una mano en una puerta, una "
        "consola o cualquier superficie que pulses. Cada arrastre es un paso de deshacer. La hoja de claves de la ventana tiene una fila por hueso animado y por "
        "miembro IK; al arrastrar una clave, las claves IK van con las claves FK de su cadena. Todas las acciones están también en el menú de la ventana, en la "
        "búsqueda (X, F3 en modo Blender) y en el clic derecho sobre un control.",
        "Vyberte postavu (NPC_spawner) a klepněte na **{{FaAnimieren}}** v inspektoru, v její nabídce pravého tlačítka nebo v okně **{{FaTitel}}**. Od časového "
        "ukazatele dostane postava vlastní animaci: vrstvu nad základní animací své kostry (**{{FaBasis}}**, např. BOTH_STAND1) s **{{FaBilder}}** snímky, 20 za "
        "sekundu. Ovladače ovládacího rigu se objeví na postavě v mapě. **Jako 3ds Max** (výchozí): Q vybírá, W posouvá, E otáčí, F5/F6/F7 zamknou na X/Y/Z mapy, "
        "K nastaví klíč. Posun ruky nebo chodidla je IK - cíl se pohybuje v rovině pohledu; posun pánve hýbe celou postavou; otočení končetiny v IK se nejprve bez "
        "skoku přepne na FK (**{{MbIkAuto}}**). **Jako Blender:** G a R modálně s X/Y/Z, klepnutí potvrdí, Esc zruší; I nastaví klíč. **{{FaFussBoden}}** postaví "
        "chodidlo na podlahu mapy, **{{FaZielFlaeche}}** položí ruku na dveře, konzoli nebo libovolnou plochu, na kterou klepnete. Každé tažení je jeden krok zpět. "
        "Dopesheet v okně má řádek pro každou animovanou kost a každou končetinu IK; tažený klíč vezme klíče IK s klíči FK jejich řetězce. Všechny akce jsou také v "
        "nabídce okna, ve vyhledávání (X, v režimu Blenderu F3) a v pravém tlačítku na ovladači."),
    "bild_eine": ("A figure posed on the map: controls (boxes like 3ds Max), the IK target of the right hand, the root path of the pelvis and the window with the dope sheet.",
                  "Eine Figur auf der Karte posiert: Griffe (Kästen wie 3ds Max), das IK-Ziel der rechten Hand, der Wurzelweg des Beckens und das Fenster mit dem Dopesheet.",
                  "在地图上摆姿势的角色: 控制器 (如 3ds Max 的方框)、右手的 IK 目标、骨盆的根路径以及带摄影表的窗口。",
                  "マップ上でポーズしたフィギュア: コントロール (3ds Max 風の箱)、右手の IK ターゲット、骨盤のルートパス、ドープシート付きのウィンドウ。",
                  "Una figura posada en el mapa: controles (cajas como en 3ds Max), el objetivo IK de la mano derecha, el trayecto raíz de la pelvis y la ventana con la hoja de claves.",
                  "Postava pózovaná na mapě: ovladače (rámečky jako v 3ds Max), cíl IK pravé ruky, kořenová dráha pánve a okno s dopesheetem."),
    "h_weg": ("Root path and SET_ORIGIN", "Wurzelweg und SET_ORIGIN", "根路径与 SET_ORIGIN", "ルートパスと SET_ORIGIN", "Trayecto raíz y SET_ORIGIN", "Kořenová dráha a SET_ORIGIN"),
    "weg": (
        "The yellow line in the map is the path of the pelvis over all frames: the way of the figure. Drag one of its dots to move the pelvis at that frame. In the game "
        "the origin of an NPC does not move with its animation - measured in Movie Duels and in the original JKA: without SET_ORIGIN the figure jumps back to its "
        "start as soon as the next animation begins. So the script block of the figure ends with **SET_ORIGIN** (start plus the way of the pelvis from the last frame "
        "to the first frame of **{{FaFolge}}**) together with that animation, and the figure stays where it is. Switch off **{{FaSetOrigin}}** to see the jump in the "
        "preview. The preview always plays what the game gets: every change bakes the map GLA again and hangs it on the skeleton like the game.",
        "Die gelbe Linie in der Karte ist der Weg des Beckens über alle Bilder: der Weg der Figur. Einen ihrer Punkte ziehen verschiebt das Becken an diesem Bild. Im "
        "Spiel wandert der Ursprung eines NPC nicht mit seiner Animation - in Movie Duels und im Original-JKA gemessen: ohne SET_ORIGIN springt die Figur an ihren "
        "Start zurück, sobald die nächste Animation beginnt. Deshalb endet der Skriptblock der Figur mit **SET_ORIGIN** (Start plus Weg des Beckens vom letzten Bild "
        "zum ersten Bild von **{{FaFolge}}**) zusammen mit dieser Animation, und die Figur bleibt stehen, wo sie ist. **{{FaSetOrigin}}** ausschalten zeigt den "
        "Sprung in der Vorschau. Die Vorschau spielt immer, was das Spiel bekommt: jede Änderung backt die Karten-GLA neu und hängt sie wie das Spiel an das Skelett.",
        "地图中的黄线是骨盆在所有帧上的路径: 角色的行进路线。拖动其中一个点可移动该帧的骨盆。在游戏中 NPC 的原点不会随动画移动 - 在 Movie Duels 和原版 JKA "
        "中测得: 没有 SET_ORIGIN 时, 下一个动画一开始角色就跳回起点。因此角色的脚本块以 **SET_ORIGIN** (起点加上骨盆从最后一帧到 **{{FaFolge}}** 第一帧的"
        "位移) 结尾, 并同时设置该动画, 角色就留在原地。关闭 **{{FaSetOrigin}}** 可在预览中看到跳变。预览始终播放游戏得到的内容: 每次改动都会重新烘焙地图 "
        "GLA, 并像游戏一样挂到骨架上。",
        "マップの黄色い線は全フレームにわたる骨盤の経路、つまりフィギュアの道筋です。点をドラッグするとそのフレームの骨盤が動きます。ゲームでは NPC の原点は"
        "アニメーションと一緒に動きません - Movie Duels とオリジナル JKA で測定: SET_ORIGIN がないと、次のアニメーションが始まった途端に開始位置へ戻ります。"
        "そのためフィギュアのスクリプトブロックは **SET_ORIGIN** (開始 + 最終フレームから **{{FaFolge}}** の最初のフレームまでの骨盤の移動) とそのアニメー"
        "ションで終わり、フィギュアはその場に留まります。**{{FaSetOrigin}}** をオフにするとプレビューで飛びが見えます。プレビューは常にゲームが受け取るもの"
        "を再生します: 変更のたびにマップ GLA をベイクし直し、ゲームと同じくスケルトンに付けます。",
        "La línea amarilla del mapa es el trayecto de la pelvis en todos los fotogramas: el camino de la figura. Arrastra uno de sus puntos para mover la pelvis en ese "
        "fotograma. En el juego el origen de un NPC no se mueve con su animación (medido en Movie Duels y en el JKA original): sin SET_ORIGIN la figura vuelve a su "
        "inicio en cuanto empieza la siguiente animación. Por eso el bloque de script de la figura termina con **SET_ORIGIN** (inicio más el trayecto de la pelvis "
        "del último fotograma al primero de **{{FaFolge}}**) junto con esa animación, y la figura se queda donde está. Desactiva **{{FaSetOrigin}}** para ver el "
        "salto en la vista previa. La vista previa reproduce siempre lo que recibe el juego: cada cambio vuelve a hornear el GLA de mapa y lo cuelga del esqueleto "
        "como el juego.",
        "Žlutá čára v mapě je dráha pánve přes všechny snímky: cesta postavy. Tažením jednoho z jejích bodů posunete pánev v tom snímku. Ve hře se počátek NPC s "
        "animací nepohybuje - změřeno v Movie Duels a v původním JKA: bez SET_ORIGIN postava skočí zpět na začátek, jakmile začne další animace. Proto blok skriptu "
        "postavy končí příkazem **SET_ORIGIN** (začátek plus dráha pánve od posledního snímku k prvnímu snímku **{{FaFolge}}**) spolu s touto animací a postava "
        "zůstane, kde je. Vypnutím **{{FaSetOrigin}}** uvidíte skok v náhledu. Náhled vždy přehrává to, co dostane hra: každá změna znovu zapeče mapové GLA a "
        "pověsí je na kostru jako hra."),
    "h_bauen": ("Build for the game", "Für das Spiel bauen", "为游戏生成", "ゲーム用にビルド", "Crear para el juego", "Sestavení pro hru"),
    "bauen": (
        "**{{FaBauen}}** bakes every animated figure of the script into its slot BOTH_CIN_n (50 in JKA, 125 in Movie Duels) and writes into the output folder: "
        "models/players/_humanoid_<map>/_humanoid_<map>.gla, its animation.cfg and the script as .txt and .IBI. A map GLA that already exists in the game data is "
        "merged - its slots stay, the figures take free ones (a full map GLA is reported). The target GLA is chosen only now: **{{FaZielKarte}}** (this map only), "
        "**{{FaZielHumanoid}}** in your mod folder (every map; never into the game folder) or **{{FaZielEigen}}**. Before writing, the limits of the game are "
        "checked (frames behind the end of the GLA, file size of the animation.cfg, name in the header, 53 bones); a finding asks before building. The script block "
        "of each figure is SET_ANIM_BOTH with the slot, SET_ANIM_HOLDTIME_BOTH with its length, a wait and SET_ORIGIN with the following animation.",
        "**{{FaBauen}}** backt jede animierte Figur des Skripts auf ihren Platz BOTH_CIN_n (50 in JKA, 125 in Movie Duels) und schreibt in den Ausgabeordner: "
        "models/players/_humanoid_<karte>/_humanoid_<karte>.gla, ihre animation.cfg und das Skript als .txt und .IBI. Eine Karten-GLA, die es in den Spieldaten schon "
        "gibt, wird zusammengeführt - ihre Plätze bleiben, die Figuren nehmen freie (eine volle Karten-GLA wird gemeldet). Die Ziel-GLA wählt man erst jetzt: "
        "**{{FaZielKarte}}** (nur diese Karte), **{{FaZielHumanoid}}** im eigenen Mod-Ordner (jede Karte; nie in den Spielordner) oder **{{FaZielEigen}}**. Vor dem "
        "Schreiben werden die Grenzen des Spiels geprüft (Bilder hinter dem Ende der GLA, Größe der animation.cfg, Name im Kopf, 53 Knochen); ein Befund fragt vor "
        "dem Bauen. Der Skriptblock jeder Figur ist SET_ANIM_BOTH mit dem Platz, SET_ANIM_HOLDTIME_BOTH mit ihrer Länge, ein wait und SET_ORIGIN mit der "
        "Folgeanimation.",
        "**{{FaBauen}}** 把脚本中每个有动画的角色烘焙到其槽位 BOTH_CIN_n (JKA 50 个, Movie Duels 125 个), 并写入输出文件夹: "
        "models/players/_humanoid_<地图>/_humanoid_<地图>.gla、其 animation.cfg 以及 .txt 和 .IBI 形式的脚本。游戏数据中已有的地图 GLA 会被合并 - 其槽位保留, "
        "角色使用空闲槽位 (地图 GLA 满时会提示)。目标 GLA 此时才选择: **{{FaZielKarte}}** (仅此地图)、你 mod 文件夹中的 **{{FaZielHumanoid}}** (所有地图; "
        "绝不放进游戏文件夹) 或 **{{FaZielEigen}}**。写入前会检查游戏的限制 (帧超出 GLA 末尾、animation.cfg 的大小、文件头中的名称、53 根骨骼); 发现问题时"
        "会在生成前询问。每个角色的脚本块为带槽位的 SET_ANIM_BOTH、带长度的 SET_ANIM_HOLDTIME_BOTH、一个 wait, 以及与后续动画一起的 SET_ORIGIN。",
        "**{{FaBauen}}** はスクリプトのアニメーション付きフィギュアをそれぞれのスロット BOTH_CIN_n (JKA 50、Movie Duels 125) にベイクし、出力フォルダーに "
        "models/players/_humanoid_<マップ>/_humanoid_<マップ>.gla、その animation.cfg、.txt と .IBI のスクリプトを書き出します。ゲームデータに既にあるマップ "
        "GLA は統合されます - そのスロットは残り、フィギュアは空きスロットを使います (満杯なら通知)。出力先 GLA はここで初めて選びます: **{{FaZielKarte}}** "
        "(このマップのみ)、自分の Mod フォルダーの **{{FaZielHumanoid}}** (全マップ。ゲームフォルダーには決して置かない)、または **{{FaZielEigen}}**。書き出す前"
        "にゲームの制限を確認します (GLA の終わりを越えるフレーム、animation.cfg のサイズ、ヘッダーの名前、53 ボーン)。問題があればビルド前に確認します。各フィ"
        "ギュアのスクリプトブロックは、スロット付き SET_ANIM_BOTH、長さ付き SET_ANIM_HOLDTIME_BOTH、wait、次のアニメーション付き SET_ORIGIN です。",
        "**{{FaBauen}}** hornea cada figura animada del script en su hueco BOTH_CIN_n (50 en JKA, 125 en Movie Duels) y escribe en la carpeta de salida: "
        "models/players/_humanoid_<mapa>/_humanoid_<mapa>.gla, su animation.cfg y el script como .txt e .IBI. Un GLA de mapa que ya existe en los datos del juego se "
        "une: sus huecos se conservan y las figuras toman los libres (un GLA de mapa lleno se avisa). El GLA de destino se elige solo ahora: **{{FaZielKarte}}** "
        "(solo este mapa), **{{FaZielHumanoid}}** en tu carpeta de mod (todos los mapas; nunca en la carpeta del juego) o **{{FaZielEigen}}**. Antes de escribir se "
        "revisan los límites del juego (fotogramas tras el final del GLA, tamaño de animation.cfg, nombre en la cabecera, 53 huesos); un hallazgo pregunta antes de "
        "crear. El bloque de script de cada figura es SET_ANIM_BOTH con el hueco, SET_ANIM_HOLDTIME_BOTH con su duración, un wait y SET_ORIGIN con la animación "
        "siguiente.",
        "**{{FaBauen}}** zapeče každou animovanou postavu skriptu do jejího slotu BOTH_CIN_n (50 v JKA, 125 v Movie Duels) a zapíše do výstupní složky: "
        "models/players/_humanoid_<mapa>/_humanoid_<mapa>.gla, její animation.cfg a skript jako .txt i .IBI. Mapové GLA, které už v datech hry existuje, se sloučí - "
        "jeho sloty zůstanou a postavy vezmou volné (plné mapové GLA se ohlásí). Cílové GLA se volí až teď: **{{FaZielKarte}}** (jen tato mapa), "
        "**{{FaZielHumanoid}}** ve vlastní složce modu (každá mapa; nikdy do složky hry) nebo **{{FaZielEigen}}**. Před zápisem se zkontrolují limity hry (snímky "
        "za koncem GLA, velikost animation.cfg, název v hlavičce, 53 kostí); nález se před sestavením zeptá. Blok skriptu každé postavy je SET_ANIM_BOTH se slotem, "
        "SET_ANIM_HOLDTIME_BOTH s její délkou, wait a SET_ORIGIN s následnou animací."),
    "bild_bauen": ("Build for the game: target GLA, output folder and the checks before writing.", "Für das Spiel bauen: Ziel-GLA, Ausgabeordner und die Prüfungen vor dem Schreiben.",
                   "为游戏生成: 目标 GLA、输出文件夹和写入前的检查。", "ゲーム用にビルド: 出力先 GLA、出力フォルダー、書き出し前のチェック。",
                   "Crear para el juego: GLA de destino, carpeta de salida y las comprobaciones antes de escribir.", "Sestavení pro hru: cílové GLA, výstupní složka a kontroly před zápisem."),
    "h_mehr": ("Several figures", "Mehrere Figuren", "多个角色", "複数のフィギュア", "Varias figuras", "Více postav"),
    "mehr": (
        "Each figure has its own layer and its own slot. **{{FaWarte}}** puts a waitsignal before its animation, **{{FaSende}}** a signal after it: the second figure "
        "starts exactly when the first one is done, in the preview as in the game. **{{FaFremdSetzen}}** lets the IK target of a hand or foot follow a bone of "
        "another animated figure from the time cursor on - a grip, a hand-over, two figures holding hands. This is baked into the GLA of the holding figure, so the "
        "game shows the same.",
        "Jede Figur hat ihre eigene Schicht und ihren eigenen Platz. **{{FaWarte}}** setzt ein waitsignal vor ihre Animation, **{{FaSende}}** ein signal danach: die "
        "zweite Figur beginnt genau, wenn die erste fertig ist, in der Vorschau wie im Spiel. **{{FaFremdSetzen}}** lässt das IK-Ziel einer Hand oder eines Fußes ab "
        "dem Zeitzeiger einem Knochen einer anderen animierten Figur folgen - ein Griff, eine Übergabe, zwei Figuren Hand in Hand. Das wird in die GLA der haltenden "
        "Figur gebacken, das Spiel zeigt also dasselbe.",
        "每个角色都有自己的层和自己的槽位。**{{FaWarte}}** 在其动画前加入 waitsignal, **{{FaSende}}** 在其后加入 signal: 第二个角色恰好在第一个完成时开始, "
        "预览与游戏中一样。**{{FaFremdSetzen}}** 让手或脚的 IK 目标从时间指针起跟随另一个有动画角色的骨骼 - 握持、交接、两个角色手牵手。这会烘焙到握持"
        "角色的 GLA 中, 因此游戏显示相同效果。",
        "各フィギュアに独自のレイヤーとスロットがあります。**{{FaWarte}}** はアニメーションの前に waitsignal、**{{FaSende}}** は後に signal を置きます: 2 体"
        "目は 1 体目が終わった瞬間に始まり、プレビューでもゲームでも同じです。**{{FaFremdSetzen}}** は手や足の IK ターゲットを、タイムカーソルから別のアニ"
        "メーション付きフィギュアのボーンに従わせます - つかみ、受け渡し、手をつなぐ 2 体。これはつかむ側の GLA にベイクされるので、ゲームでも同じに見えます。",
        "Cada figura tiene su propia capa y su propio hueco. **{{FaWarte}}** pone un waitsignal antes de su animación y **{{FaSende}}** un signal después: la segunda "
        "figura empieza justo cuando termina la primera, en la vista previa como en el juego. **{{FaFremdSetzen}}** hace que el objetivo IK de una mano o un pie siga "
        "desde el cursor a un hueso de otra figura animada: un agarre, una entrega, dos figuras de la mano. Se hornea en el GLA de la figura que sujeta, así que el "
        "juego muestra lo mismo.",
        "Každá postava má vlastní vrstvu a vlastní slot. **{{FaWarte}}** vloží před její animaci waitsignal, **{{FaSende}}** po ní signal: druhá postava začne přesně "
        "ve chvíli, kdy první skončí, v náhledu stejně jako ve hře. **{{FaFremdSetzen}}** nechá cíl IK ruky nebo chodidla od časového ukazatele sledovat kost jiné "
        "animované postavy - úchop, předání, dvě postavy ruku v ruce. Zapeče se to do GLA držící postavy, takže hra ukáže totéž."),
    "platz": (
        "Leave room between the figures: an NPC that would spawn inside another body (the boxes overlap) does not appear in the game. "
        "NPC_Begin tries again every 500 ms (the **wait** of the spawner) until the place is free - an **affect** on it at the start of the "
        "script finds nobody and is skipped. The preview does the same: the figure appears only then, and its block does not play.",
        "Zwischen den Figuren Platz lassen: Ein NPC, der in einem anderen Körper spawnen würde (die Kästen überlappen), erscheint im Spiel "
        "nicht. NPC_Begin versucht es alle 500 ms wieder (das **wait** des Spawners), bis der Platz frei ist - ein **affect** auf ihn am "
        "Anfang des Skripts findet niemanden und entfällt. Die Vorschau macht es genauso: Die Figur erscheint erst dann, ihr Block spielt nicht.",
        "角色之间要留出空间: 若 NPC 会在另一个身体内生成 (碰撞盒重叠), 它在游戏中不会出现。NPC_Begin 每 500 毫秒 (生成器的 **wait**) "
        "重试, 直到位置空出 - 脚本开始时对它的 **affect** 找不到任何人而被跳过。预览也是如此: 角色到那时才出现, 其脚本块不会播放。",
        "フィギュアの間に空間を空けてください: 別の体の中にスポーンする NPC (ボックスが重なる) はゲームに現れません。NPC_Begin は場所が空くまで "
        "500 ms ごと (スポナーの **wait**) に再試行し、スクリプト開始時のその NPC への **affect** は誰も見つからず飛ばされます。プレビューも同じで、"
        "フィギュアはその時点で初めて現れ、そのブロックは再生されません。",
        "Deja espacio entre las figuras: un NPC que aparecería dentro de otro cuerpo (las cajas se solapan) no aparece en el juego. NPC_Begin "
        "lo reintenta cada 500 ms (el **wait** del spawner) hasta que el sitio queda libre; un **affect** sobre él al inicio del script no "
        "encuentra a nadie y se omite. La vista previa hace lo mismo: la figura aparece solo entonces y su bloque no se reproduce.",
        "Nechte mezi postavami místo: NPC, který by se objevil uvnitř jiného těla (boxy se překrývají), se ve hře neobjeví. NPC_Begin to "
        "zkouší každých 500 ms (**wait** spawneru), dokud se místo neuvolní - **affect** na něj na začátku skriptu nikoho nenajde a vynechá se. "
        "Náhled to dělá stejně: postava se objeví až pak a její blok se nepřehraje.",
    ),
    "bild_mehr": ("Three figures: the first walks and frees the second (signal), the third holds the hand of the second.",
                  "Drei Figuren: die erste geht und gibt die zweite frei (signal), die dritte hält die Hand der zweiten.",
                  "三个角色: 第一个行走并放行第二个 (signal), 第三个握住第二个的手。", "3 体のフィギュア: 1 体目が歩いて 2 体目を開始させ (signal)、3 体目が 2 体目の手をつかみます。",
                  "Tres figuras: la primera camina y libera a la segunda (signal), la tercera sujeta la mano de la segunda.",
                  "Tři postavy: první jde a uvolní druhou (signal), třetí drží ruku druhé."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    z = X[schluessel]
    return T(z[0], z[1], z[2], z[3])


def seite():
    """Die Seite mission-animieren (Gruppe Missionen, nach modell-ik)."""
    return {
        "id": "mission-animieren",
        "gruppe": "mission",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("img", "h_mb7_posieren", _t("bild_eine"), "breit"),
            ("h2", "eine", _t("h_eine")),
            ("p", _t("eine")),
            ("h2", "wurzelweg", _t("h_weg")),
            ("p", _t("weg")),
            ("h2", "bauen", _t("h_bauen")),
            ("p", _t("bauen")),
            ("img", "h_mb7_bauen", _t("bild_bauen"), "breit"),
            ("h2", "mehrere", _t("h_mehr")),
            ("p", _t("mehr")),
            ("p", _t("platz")),
            ("img", "h_mb7_mehrere", _t("bild_mehr"), "breit"),
        ],
    }
