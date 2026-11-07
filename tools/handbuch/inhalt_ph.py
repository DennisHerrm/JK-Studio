"""Modell-Block Stufe 6: die Seite "Model-Tab: Auto-Physik" (Schwerpunkt und Stuetzflaeche, Fuesse ohne Rutschen, Sprung auf der
Parabel mit konstantem Drehimpuls, Federketten, Filter ueber den Schluesseln; im Model-Tab und im Fenster "Figur animieren").

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_mb7.py.
"""

X = {
    "titel": ("Model tab: auto physics", "Model-Tab: Auto-Physik", "模型选项卡: 自动物理", "モデルタブ: オートフィジックス",
              "Pestaña Model: física automática", "Karta Model: automatická fyzika"),
    "kurz": (
        "Let physics check and correct an animation: the centre of mass, the support of the feet, feet that do not slide, a jump on a real parabola with the gravity of the game, constant spin in the air and spring chains - as a filter over your keys.",
        "Die Physik prüft und korrigiert eine Animation: Schwerpunkt, Stützfläche der Füße, Füße ohne Rutschen, ein Sprung auf einer echten Parabel mit der Schwerkraft des Spiels, gleichbleibender Drehimpuls in der Luft und Federketten - als Filter über deinen Schlüsseln.",
        "让物理检查并修正动画: 重心、双脚的支撑面、不打滑的脚、按游戏重力沿真实抛物线的跳跃、空中守恒的角动量以及弹簧链 - 作为关键帧之上的过滤器。",
        "物理でアニメーションを確認・修正します: 重心、足の支持面、滑らない足、ゲームの重力による本物の放物線のジャンプ、空中で一定の角運動量、スプリングチェーン - キーの上のフィルターとして。",
        "La física revisa y corrige una animación: el centro de masa, el apoyo de los pies, pies que no resbalan, un salto en una parábola real con la gravedad del juego, giro constante en el aire y cadenas elásticas, como filtro sobre tus claves.",
        "Fyzika zkontroluje a opraví animaci: těžiště, opěrnou plochu chodidel, chodidla bez klouzání, skok po skutečné parabole s gravitací hry, stálý moment hybnosti ve vzduchu a pružné řetězy - jako filtr nad vašimi klíči."),
    "h_filter": ("A filter over the keys", "Ein Filter über den Schlüsseln", "关键帧之上的过滤器", "キーの上のフィルター", "Un filtro sobre las claves", "Filtr nad klíči"),
    "filter": (
        "Create the control rig, then switch on **{{PhAn}}** in the rollout **{{PhRollout}}** of the command panel (under IK/FK), in the menu "
        "**Animation > {{PhRollout}}**, in the right-click of the view (**Control Rig > {{PhRollout}}**) or in the search (X, F3 in Blender mode). The physics reads "
        "the pose of every frame - your FK keys and IK - and writes its own track on top; your keys stay as they are. Switch it off and the animation is exactly "
        "as before, also in the baked GLA (byte for byte). **{{PhStaerke}}** mixes it in from 0 to 1. Every change of the settings is one undo step; after every "
        "change of the keys the track is computed again.",
        "Erst das Control Rig anlegen, dann **{{PhAn}}** einschalten - im Rollout **{{PhRollout}}** des Befehlsfelds (unter IK/FK), im Menü "
        "**Animation > {{PhRollout}}**, im Rechtsklick der Ansicht (**Control Rig > {{PhRollout}}**) oder in der Suche (X, im Blender-Modus F3). Die Physik liest "
        "die Pose jedes Bilds - deine FK-Schlüssel und die IK - und schreibt eine eigene Spur darüber; deine Schlüssel bleiben, wie sie sind. Ausgeschaltet ist die "
        "Animation genau wie vorher, auch in der gebackenen GLA (Byte für Byte). **{{PhStaerke}}** mischt sie von 0 bis 1 hinein. Jede Änderung der Einstellung "
        "ist ein Rückgängig-Schritt; nach jeder Änderung der Schlüssel wird die Spur neu gerechnet.",
        "先创建控制绑定, 然后打开 **{{PhAn}}** - 在命令面板的 **{{PhRollout}}** 卷展栏 (IK/FK 下方)、菜单 **Animation > {{PhRollout}}**、视图右键"
        " (**Control Rig > {{PhRollout}}**) 或搜索 (X, Blender 模式为 F3) 中。物理读取每一帧的姿势 - 你的 FK 关键帧和 IK - 并在其上写入自己的轨道; 你的"
        "关键帧保持不变。关闭后动画与之前完全相同, 烘焙的 GLA 也逐字节相同。**{{PhStaerke}}** 以 0 到 1 混合。每次修改设置都是一步撤销; 关键帧每次修改"
        "后轨道都会重新计算。",
        "先にコントロールリグを作成し、**{{PhAn}}** をオンにします - コマンドパネルのロールアウト **{{PhRollout}}** (IK/FK の下)、メニュー "
        "**Animation > {{PhRollout}}**、ビューの右クリック (**Control Rig > {{PhRollout}}**)、または検索 (X、Blender 風は F3) で。物理は各フレームのポーズ"
        " (FK キーと IK) を読み、その上に独自のトラックを書きます。キーはそのままです。オフにすればアニメーションは元どおりで、ベイクした GLA もバイト単位で"
        "同じです。**{{PhStaerke}}** で 0 から 1 まで混ぜます。設定の変更は元に戻す 1 回分、キーを変えるたびにトラックが再計算されます。",
        "Crea el rig de control y activa **{{PhAn}}**: en el panel **{{PhRollout}}** del panel de comandos (bajo IK/FK), en el menú **Animation > "
        "{{PhRollout}}**, en el clic derecho de la vista (**Control Rig > {{PhRollout}}**) o en la búsqueda (X, F3 en modo Blender). La física lee la pose de cada "
        "fotograma (tus claves FK y la IK) y escribe su propia pista encima; tus claves no cambian. Al desactivarla la animación queda exactamente como antes, "
        "también en el GLA horneado (byte a byte). **{{PhStaerke}}** la mezcla de 0 a 1. Cada cambio de ajuste es un paso de deshacer; tras cada cambio de claves "
        "la pista se recalcula.",
        "Nejprve vytvořte řídicí rig, pak zapněte **{{PhAn}}** - v rolloutu **{{PhRollout}}** panelu příkazů (pod IK/FK), v nabídce **Animation > "
        "{{PhRollout}}**, v pravém kliknutí pohledu (**Control Rig > {{PhRollout}}**) nebo ve vyhledávání (X, v režimu Blender F3). Fyzika čte pózu každého "
        "snímku - vaše klíče FK a IK - a zapisuje nad ni vlastní stopu; vaše klíče zůstávají, jak jsou. Po vypnutí je animace přesně jako předtím, i v zapečeném "
        "GLA (bajt po bajtu). **{{PhStaerke}}** ji přimíchá od 0 do 1. Každá změna nastavení je jeden krok zpět; po každé změně klíčů se stopa přepočítá."),
    "h_schwer": ("Centre of mass and support", "Schwerpunkt und Stützfläche", "重心与支撑面", "重心と支持面", "Centro de masa y apoyo", "Těžiště a opěrná plocha"),
    "schwer": (
        "The view shows the centre of mass (the round symbol with two filled quarters), a dashed line down to the floor, its path over the sequence (orange in "
        "the air) and the support area of the feet on the ground (green). The masses of the body parts come from the table of D. A. Winter (Biomechanics and Motor "
        "Control of Human Movement, after Dempster): head and neck 8.1 %, trunk 49.7 %, upper arm 2.8 %, forearm 1.6 %, hand 0.6 %, thigh 10 %, shank 4.65 %, "
        "foot 1.45 %. When the figure stands on both feet and the centre of mass leaves the support, everything turns red and the rollout names the frames - the "
        "figure would fall over. A foot touches the ground when its lowest point is at most **{{PhKontaktAbstand}}** above its lowest position in the sequence, measured "
        "above the floor (the floor of the model tab, in the mission tab the floor of the map below the foot) - models stand at different heights.",
        "Die Ansicht zeigt den Schwerpunkt (das runde Zeichen mit zwei gefüllten Vierteln), ein gestricheltes Lot auf den Boden, seinen Weg über die Sequenz "
        "(orange in der Luft) und die Stützfläche der Füße auf dem Boden (grün). Die Massen der Körperteile stammen aus der Tabelle von D. A. Winter (Biomechanics "
        "and Motor Control of Human Movement, nach Dempster): Kopf und Hals 8,1 %, Rumpf 49,7 %, Oberarm 2,8 %, Unterarm 1,6 %, Hand 0,6 %, Oberschenkel 10 %, "
        "Unterschenkel 4,65 %, Fuß 1,45 %. Steht die Figur auf beiden Füßen und verlässt der Schwerpunkt die Stützfläche, wird alles rot und das Rollout nennt die "
        "Bilder - die Figur würde umfallen. Ein Fuß berührt den Boden, wenn sein tiefster Punkt höchstens **{{PhKontaktAbstand}}** über seinem tiefsten Stand in der Sequenz "
        "liegt, über dem Boden gemessen (der Boden des Model-Tabs, im Mission-Tab der Boden der Karte unter dem Fuß) - Modelle stehen verschieden hoch.",
        "视图显示重心 (带两个填充四分之一的圆形符号)、到地面的虚线、其在序列中的路径 (空中为橙色) 以及双脚在地面上的支撑面 (绿色)。身体各部分的质量来自"
        " D. A. Winter 的表 (Biomechanics and Motor Control of Human Movement, 依据 Dempster): 头颈 8.1 %、躯干 49.7 %、上臂 2.8 %、前臂 1.6 %、手 0.6 %、"
        "大腿 10 %、小腿 4.65 %、脚 1.45 %。当角色双脚站立而重心离开支撑面时, 一切变红, 卷展栏会列出这些帧 - 角色会摔倒。脚的最低点不高于其在序列中最低位置加 "
        "**{{PhKontaktAbstand}}** 时视为着地, 相对地面测量 (模型选项卡的地面, 任务选项卡中为脚下地图的地面) - 各模型站立高度不同。",
        "ビューには重心 (2 つの塗りつぶした四分円のある丸い記号)、床への破線、シーケンス上の軌跡 (空中はオレンジ)、床上の足の支持面 (緑) が表示されます。"
        "体の各部の質量は D. A. Winter の表 (Biomechanics and Motor Control of Human Movement、Dempster による) から: 頭頸部 8.1 %、体幹 49.7 %、上腕 2.8 %、"
        "前腕 1.6 %、手 0.6 %、大腿 10 %、下腿 4.65 %、足 1.45 %。両足で立っていて重心が支持面から出ると全体が赤くなり、ロールアウトがそのフレームを示します"
        " - フィギュアは倒れます。足の最下点がシーケンス中の最も低い位置から **{{PhKontaktAbstand}}** 以内なら接地です。床から測ります (モデルタブの床、ミッションタブでは足の下のマップの床) - モデルごとに立つ高さが違います。",
        "La vista muestra el centro de masa (el símbolo redondo con dos cuartos rellenos), una línea discontinua hasta el suelo, su trayectoria en la secuencia "
        "(naranja en el aire) y la superficie de apoyo de los pies en el suelo (verde). Las masas de las partes del cuerpo vienen de la tabla de D. A. Winter "
        "(Biomechanics and Motor Control of Human Movement, según Dempster): cabeza y cuello 8,1 %, tronco 49,7 %, brazo 2,8 %, antebrazo 1,6 %, mano 0,6 %, muslo "
        "10 %, pierna 4,65 %, pie 1,45 %. Si la figura está de pie sobre ambos pies y el centro de masa sale del apoyo, todo se vuelve rojo y el panel indica los "
        "fotogramas: la figura se caería. Un pie toca el suelo si su punto más bajo está como mucho **{{PhKontaktAbstand}}** por encima de su posición más baja en la "
        "secuencia, medido sobre el suelo (el de la pestaña Model; en la pestaña Mission, el del mapa bajo el pie): los modelos están a alturas distintas.",
        "Pohled ukazuje těžiště (kulatou značku se dvěma vyplněnými čtvrtinami), čárkovanou svislici k podlaze, jeho dráhu přes sekvenci (ve vzduchu oranžově) a "
        "opěrnou plochu chodidel na zemi (zeleně). Hmotnosti částí těla pocházejí z tabulky D. A. Wintera (Biomechanics and Motor Control of Human Movement, podle "
        "Dempstera): hlava a krk 8,1 %, trup 49,7 %, paže 2,8 %, předloktí 1,6 %, ruka 0,6 %, stehno 10 %, bérec 4,65 %, chodidlo 1,45 %. Když postava stojí na "
        "obou nohách a těžiště opustí opěrnou plochu, vše zčervená a rollout uvede snímky - postava by upadla. Chodidlo se dotýká země, když je jeho nejnižší bod "
        "nejvýš **{{PhKontaktAbstand}}** nad jeho nejnižší polohou v sekvenci, měřeno nad podlahou (podlaha karty Model, v kartě Mission podlaha mapy pod chodidlem) - modely stojí různě vysoko."),
    "h_sprung": ("Jumps and feet", "Sprünge und Füße", "跳跃与双脚", "ジャンプと足", "Saltos y pies", "Skoky a chodidla"),
    "sprung": (
        "Between take-off (the last frame with a foot on the ground) and landing the centre of mass follows the parabola of the game's gravity - g_gravity, 800 "
        "units per second squared, in the mission tab the gravity of the map. Its start and end stay where you put them; how high the figure flies follows from "
        "how long it is in the air, just like in the game. In the air the whole body also turns so that its angular momentum stays constant: swing an arm forward "
        "and the body turns back a little. Take-off and landing come from the foot contacts; **{{PhAbsprungHier}}** and **{{PhLandungHier}}** set them at the time "
        "cursor, **{{PhFlugAutomatisch}}** takes them from the contacts again. With **{{PhKontakte}}** a foot on the ground does not slide: the leg IK holds its "
        "lowest point where the contact began, rolling over the ball and the height stay from your keys.",
        "Zwischen Absprung (dem letzten Bild mit einem Fuß am Boden) und Landung folgt der Schwerpunkt der Parabel der Schwerkraft des Spiels - g_gravity, 800 "
        "Einheiten pro Sekunde zum Quadrat, im Mission-Tab die Schwerkraft der Karte. Anfang und Ende bleiben, wo du sie hingesetzt hast; wie hoch die Figur fliegt, "
        "folgt daraus, wie lange sie in der Luft ist - genau wie im Spiel. In der Luft dreht sich der ganze Körper außerdem so, dass sein Drehimpuls gleich bleibt: "
        "schwingt ein Arm nach vorn, dreht sich der Körper ein wenig zurück. Absprung und Landung kommen aus den Fußkontakten; **{{PhAbsprungHier}}** und "
        "**{{PhLandungHier}}** setzen sie am Zeitzeiger, **{{PhFlugAutomatisch}}** nimmt sie wieder aus den Kontakten. Mit **{{PhKontakte}}** rutscht ein Fuß am "
        "Boden nicht: die Bein-IK hält seinen tiefsten Punkt dort, wo der Kontakt begann; Abrollen über den Ballen und die Höhe bleiben aus deinen Schlüsseln.",
        "在起跳 (最后一个有脚着地的帧) 与落地之间, 重心沿游戏重力的抛物线运动 - g_gravity, 每平方秒 800 单位, 任务选项卡中为地图的重力。起点和终点保持在你"
        "放置的位置; 角色飞多高取决于它在空中停留多久 - 与游戏完全一样。在空中整个身体还会转动, 使角动量保持不变: 手臂向前摆, 身体就稍向后转。起跳与"
        "落地来自脚的接触; **{{PhAbsprungHier}}** 和 **{{PhLandungHier}}** 在时间指针处设置它们, **{{PhFlugAutomatisch}}** 再次由接触决定。启用 "
        "**{{PhKontakte}}** 后着地的脚不会打滑: 腿部 IK 将其最低点固定在接触开始处; 经脚掌的滚动和高度保持你的关键帧。",
        "踏み切り (足が接地している最後のフレーム) から着地まで、重心はゲームの重力の放物線をたどります - g_gravity、毎秒毎秒 800 ユニット、ミッションタブでは"
        "マップの重力。始点と終点は置いた場所のまま。どれだけ高く飛ぶかは空中にいる時間で決まります - ゲームとまったく同じです。空中では体全体が角運動量を"
        "一定に保つよう回転もします: 腕を前に振ると体は少し後ろに回ります。踏み切りと着地は足の接地から決まり、**{{PhAbsprungHier}}** と "
        "**{{PhLandungHier}}** でタイムカーソルの位置に設定、**{{PhFlugAutomatisch}}** で再び接地から決めます。**{{PhKontakte}}** では接地した足が滑りません: "
        "脚の IK が最下点を接地開始位置に保ち、母指球でのロールと高さはキーのままです。",
        "Entre el despegue (el último fotograma con un pie en el suelo) y el aterrizaje, el centro de masa sigue la parábola de la gravedad del juego: g_gravity, "
        "800 unidades por segundo al cuadrado; en la pestaña Mission, la gravedad del mapa. El inicio y el final quedan donde los pusiste; la altura del vuelo sale "
        "del tiempo en el aire, igual que en el juego. En el aire todo el cuerpo gira además para que su momento angular sea constante: si un brazo se mueve "
        "adelante, el cuerpo gira un poco hacia atrás. Despegue y aterrizaje salen de los contactos de los pies; **{{PhAbsprungHier}}** y **{{PhLandungHier}}** los "
        "fijan en el cursor de tiempo y **{{PhFlugAutomatisch}}** los vuelve a tomar de los contactos. Con **{{PhKontakte}}** un pie en el suelo no resbala: la IK de "
        "la pierna mantiene su punto más bajo donde empezó el contacto; el rodamiento sobre la planta y la altura siguen tus claves.",
        "Mezi odrazem (posledním snímkem s chodidlem na zemi) a dopadem sleduje těžiště parabolu gravitace hry - g_gravity, 800 jednotek za sekundu na druhou, v "
        "kartě Mission gravitace mapy. Začátek a konec zůstanou, kam jste je dali; jak vysoko postava letí, plyne z toho, jak dlouho je ve vzduchu - přesně jako "
        "ve hře. Ve vzduchu se celé tělo navíc otáčí tak, aby moment hybnosti zůstal stejný: švihne-li paže dopředu, tělo se trochu otočí zpět. Odraz a dopad "
        "vycházejí z dotyků chodidel; **{{PhAbsprungHier}}** a **{{PhLandungHier}}** je nastaví na časovém ukazateli, **{{PhFlugAutomatisch}}** je opět vezme z "
        "dotyků. S **{{PhKontakte}}** chodidlo na zemi neklouže: IK nohy drží jeho nejnižší bod tam, kde dotyk začal; odvalování přes bříško a výška zůstanou z "
        "vašich klíčů."),
    "bild_sprung": ("A jump with auto physics: the path of the centre of mass is a parabola (orange), the rollout shows take-off, landing and the result.",
                    "Ein Sprung mit Auto-Physik: der Weg des Schwerpunkts ist eine Parabel (orange), das Rollout zeigt Absprung, Landung und das Ergebnis.",
                    "带自动物理的跳跃: 重心路径为抛物线 (橙色), 卷展栏显示起跳、落地和结果。",
                    "オートフィジックスのジャンプ: 重心の軌跡は放物線 (オレンジ)、ロールアウトに踏み切り・着地・結果が表示されます。",
                    "Un salto con física automática: la trayectoria del centro de masa es una parábola (naranja); el panel muestra despegue, aterrizaje y resultado.",
                    "Skok s automatickou fyzikou: dráha těžiště je parabola (oranžově), rollout ukazuje odraz, dopad a výsledek."),
    "h_federn": ("Spring chains", "Federketten", "弹簧链", "スプリングチェーン", "Cadenas elásticas", "Pružné řetězy"),
    "federn": (
        "Cloaks, coat tails, hair and straps can follow through: select a bone and click **{{PhKetteHier}}** (also in the right-click of a control), or let "
        "**{{PhKettenVorschlag}}** add the hanging bones the bone recognition found. Each chain swings like a damped spring behind the bone it hangs on: "
        "**{{PhFrequenz}}** sets how fast, **{{PhDaempfung}}** how quickly it comes to rest (1 without overshooting).",
        "Umhänge, Rockschöße, Haare und Riemen können nachschwingen: einen Knochen wählen und **{{PhKetteHier}}** klicken (auch im Rechtsklick eines Griffs), oder "
        "**{{PhKettenVorschlag}}** fügt die hängenden Knochen hinzu, die die Knochen-Erkennung gefunden hat. Jede Kette schwingt wie eine gedämpfte Feder hinter dem "
        "Knochen, an dem sie hängt: **{{PhFrequenz}}** sagt, wie schnell, **{{PhDaempfung}}**, wie bald sie zur Ruhe kommt (1 ohne Überschwingen).",
        "披风、衣摆、头发和带子可以随动: 选择一根骨骼并点击 **{{PhKetteHier}}** (控制器的右键中也有), 或让 **{{PhKettenVorschlag}}** 添加骨骼识别找到的悬挂"
        "骨骼。每条链像阻尼弹簧一样跟在其悬挂的骨骼后面摆动: **{{PhFrequenz}}** 决定快慢, **{{PhDaempfung}}** 决定多快静止 (1 为无过冲)。",
        "マント、裾、髪、ストラップを揺れ残らせられます: ボーンを選んで **{{PhKetteHier}}** をクリック (コントロールの右クリックにも)、または "
        "**{{PhKettenVorschlag}}** でボーン認識が見つけた垂れ下がるボーンを追加します。各チェーンはぶら下がっているボーンの後ろで減衰ばねのように揺れます: "
        "**{{PhFrequenz}}** は速さ、**{{PhDaempfung}}** は静止までの早さ (1 で行き過ぎなし)。",
        "Capas, faldones, pelo y correas pueden seguir el movimiento: elige un hueso y pulsa **{{PhKetteHier}}** (también en el clic derecho de un control), o deja "
        "que **{{PhKettenVorschlag}}** añada los huesos colgantes que encontró el reconocimiento. Cada cadena oscila como un muelle amortiguado tras el hueso del que "
        "cuelga: **{{PhFrequenz}}** indica lo rápido y **{{PhDaempfung}}** lo pronto que se detiene (1 sin pasarse).",
        "Pláště, šosy, vlasy a řemínky mohou dokmitávat: vyberte kost a klikněte na **{{PhKetteHier}}** (i v pravém kliknutí ovladače), nebo nechte "
        "**{{PhKettenVorschlag}}** přidat visící kosti, které našlo rozpoznání kostí. Každý řetěz kmitá jako tlumená pružina za kostí, na které visí: "
        "**{{PhFrequenz}}** určuje rychlost, **{{PhDaempfung}}**, jak brzy se uklidní (1 bez překmitu)."),
    "h_mission": ("In the mission tab", "Im Mission-Tab", "在任务选项卡中", "ミッションタブで", "En la pestaña Mission", "V kartě Mission"),
    "mission": (
        "The window **{{FaTitel}}** has the same rollout **{{PhRollout}}** and a menu of the same name. There the floor is the map under the feet (traceRay) and "
        "the gravity is the one of the map (worldspawn \"gravity\"). The physics goes into the map GLA when you build for the game; in Movie Duels and the original "
        "JKA the figure then flies the same parabola as in the preview.",
        "Das Fenster **{{FaTitel}}** hat dasselbe Rollout **{{PhRollout}}** und ein gleichnamiges Menü. Dort ist der Boden die Karte unter den Füßen (traceRay) und "
        "die Schwerkraft die der Karte (worldspawn \"gravity\"). Beim Bauen für das Spiel geht die Physik in die Karten-GLA; in Movie Duels und im Original-JKA fliegt "
        "die Figur dann dieselbe Parabel wie in der Vorschau.",
        "窗口 **{{FaTitel}}** 有相同的 **{{PhRollout}}** 卷展栏和同名菜单。在那里, 地面是脚下的地图 (traceRay), 重力是地图的重力 (worldspawn \"gravity\")。"
        "为游戏生成时物理进入地图 GLA; 在 Movie Duels 和原版 JKA 中, 角色随后沿与预览相同的抛物线飞行。",
        "ウィンドウ **{{FaTitel}}** には同じロールアウト **{{PhRollout}}** と同名のメニューがあります。そこでは床は足の下のマップ (traceRay)、重力はマップの"
        "重力 (worldspawn \"gravity\") です。ゲーム用にビルドすると物理はマップ GLA に入り、Movie Duels とオリジナル JKA でフィギュアはプレビューと同じ放物線を"
        "飛びます。",
        "La ventana **{{FaTitel}}** tiene el mismo panel **{{PhRollout}}** y un menú con ese nombre. Allí el suelo es el mapa bajo los pies (traceRay) y la "
        "gravedad es la del mapa (worldspawn \"gravity\"). Al crear para el juego la física entra en el GLA de mapa; en Movie Duels y en el JKA original la figura "
        "vuela entonces la misma parábola que en la vista previa.",
        "Okno **{{FaTitel}}** má stejný rollout **{{PhRollout}}** a stejnojmennou nabídku. Tam je podlahou mapa pod chodidly (traceRay) a gravitace je gravitace "
        "mapy (worldspawn \"gravity\"). Při sestavení pro hru jde fyzika do mapového GLA; v Movie Duels i v původním JKA pak postava letí stejnou parabolu jako v "
        "náhledu."),
    "bild_figur": ("The same jump on the map: centre of mass with its path, rollout Auto physics in the window Animate figure.",
                   "Derselbe Sprung auf der Karte: Schwerpunkt mit seinem Weg, Rollout Auto-Physik im Fenster Figur animieren.",
                   "地图上的同一跳跃: 重心及其路径, 窗口“为角色制作动画”中的“自动物理”卷展栏。",
                   "マップ上の同じジャンプ: 重心とその軌跡、ウィンドウ「フィギュアをアニメート」のロールアウト「オートフィジックス」。",
                   "El mismo salto en el mapa: centro de masa con su trayectoria, panel Física automática en la ventana Animar figura.",
                   "Tentýž skok na mapě: těžiště s dráhou, rollout Automatická fyzika v okně Animovat postavu."),
}


def _t(schluessel):
    from inhalt_basis import T
    z = X[schluessel]
    return T(z[0], z[1], z[2], z[3])


def seite():
    """Die Seite modell-physik (Gruppe Model, nach modell-ik)."""
    return {
        "id": "modell-physik",
        "gruppe": "mission",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("h2", "filter", _t("h_filter")),
            ("p", _t("filter")),
            ("img", "h_ph_sprung", _t("bild_sprung"), "breit"),
            ("h2", "schwerpunkt", _t("h_schwer")),
            ("p", _t("schwer")),
            ("h2", "sprung", _t("h_sprung")),
            ("p", _t("sprung")),
            ("h2", "federn", _t("h_federn")),
            ("p", _t("federn")),
            ("h2", "mission", _t("h_mission")),
            ("p", _t("mission")),
            ("img", "h_ph_figur", _t("bild_figur"), "breit"),
        ],
    }


ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}
