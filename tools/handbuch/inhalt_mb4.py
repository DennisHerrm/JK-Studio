"""Modell-Block Stufe 4: die Seite "Model-Tab: IK/FK, Fussrolle, Pfade und Raumwechsel" (Zwei-Glied-IK an Armen und Beinen,
Wechsel IK/FK ohne Sprung wie 3ds Max und wie Blender/Rigify, Pol-Ziele, Fussrolle, Pfade an Haenden, Fuessen und Saebelspitze
ziehen, Space Switching, Reduzieren mit Auswahl, Posen-Bibliothek als Datei).

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_mb3b.py.
"""

X = {
    "titel": ("Model tab: IK/FK, foot roll, paths and space switching", "Model-Tab: IK/FK, Fußrolle, Pfade und Raumwechsel",
              "模型选项卡: IK/FK、脚部滚动、路径和空间切换", "モデルタブ: IK/FK、フットロール、パス、スペース切り替え",
              "Pestaña Model: IK/FK, balanceo del pie, trayectorias y cambio de espacio", "Karta Model: IK/FK, odval chodidla, dráhy a přepínání prostoru"),
    "kurz": (
        "Move hands and feet with two-bone IK, switch every limb between IK and FK without a jump, roll the foot, drag the paths of hands, feet and the saber tip, and let a hand follow another space.",
        "Hände und Füße mit Zwei-Glied-IK bewegen, jedes Glied ohne Sprung zwischen IK und FK wechseln, den Fuß rollen, die Pfade von Händen, Füßen und Säbelspitze ziehen und eine Hand einem anderen Raum folgen lassen.",
        "用双骨骼 IK 移动手和脚, 每个肢体在 IK 与 FK 之间无跳变切换, 滚动脚部, 拖动手、脚和光剑尖端的路径, 并让一只手跟随另一个空间。",
        "2 ボーン IK で手と足を動かし、各肢を IK と FK の間で飛びなく切り替え、足を転がし、手・足・セーバー先端のパスをドラッグし、手を別のスペースに従わせます。",
        "Mueve manos y pies con IK de dos huesos, cambia cada miembro entre IK y FK sin salto, balancea el pie, arrastra las trayectorias de manos, pies y punta del sable y haz que una mano siga otro espacio.",
        "Pohybujte rukama a chodidly dvoukostním IK, přepínejte každou končetinu mezi IK a FK bez skoku, odvalujte chodidlo, táhněte dráhy rukou, chodidel a hrotu meče a nechte ruku sledovat jiný prostor."),
    "h_ik": ("IK and FK", "IK und FK", "IK 与 FK", "IK と FK", "IK y FK", "IK a FK"),
    "ik": (
        "After **{{MbRgAnlegen}}** every arm and leg of the rig has two-bone IK: the end bone (hand, foot) reaches its target, the elbow or knee lies in the plane of root, target and pole target and points to the pole - the rule Blender uses for its pole target, and the swivel target of the HI solver in 3ds Max. The IK is a change on the pose of the frame, as in Blender, so the twist of the limbs from the animation stays. The result is FK keys of upper limb, lower limb and end bone in the same layer: the dope sheet and the curves show it at once, and baking into the GLA works as before. **Like 3ds Max:** grab the hand or foot with W - that is IK, the switch is a key; rotate an upper or lower limb with E - that is FK. With **{{MbIkAuto}}** (on by default) every switch first snaps the other side, so nothing jumps. The group **{{MbIkRollout}}** has one line per limb whose button shows IK or FK and switches it, and **{{MbIkKnopfEinrasten}}** for the chosen limb ([[modell-animationsfeld#ikfk]]). **Like Blender:** each limb has the slider IK-FK of Rigify (0 = IK, 1 = FK) and the buttons **{{MbIkKnopfIkFk}}** (IK target and pole onto the FK pose) and **{{MbIkKnopfFkIk}}** (FK keys onto the IK pose); snap first, then move the slider, and the limb does not jump. G moves the IK target only in IK; otherwise a message says what to do. Every action is also in the menu **{{MbBereichAnimation}}** (group **{{MbIkGruppe}}**), the right-click of the view and of the control, and the search X / F3.",
        "Nach **{{MbRgAnlegen}}** hat jeder Arm und jedes Bein des Rigs Zwei-Glied-IK: der Endknochen (Hand, Fuß) erreicht sein Ziel, Ellbogen oder Knie liegen in der Ebene aus Ansatz, Ziel und Pol-Ziel und zeigen zum Pol - die Regel, nach der Blender sein Pol-Ziel setzt, und das Schwenkziel des HI-Solvers in 3ds Max. Die IK ist wie in Blender eine Änderung auf der Pose des Frames, die Verdrehung der Glieder aus der Animation bleibt. Das Ergebnis sind FK-Schlüssel von Ober-, Unterglied und Endknochen in derselben Schicht: Dopesheet und Kurven zeigen es sofort, das Backen in die GLA geht wie bisher. **Wie 3ds Max:** Hand oder Fuß mit W greifen ist IK, der Wechsel ist ein Schlüssel; ein Ober- oder Unterglied mit E drehen ist FK. Mit **{{MbIkAuto}}** (ab Werk an) rastet jeder Wechsel vorher die andere Seite ein, nichts springt. Die Gruppe **{{MbIkRollout}}** hat je Glied eine Zeile, deren Knopf IK oder FK zeigt und umschaltet, und für das gewählte Glied **{{MbIkKnopfEinrasten}}** ([[modell-animationsfeld#ikfk]]). **Wie Blender:** jedes Glied hat den Regler IK-FK von Rigify (0 = IK, 1 = FK) und die Knöpfe **{{MbIkKnopfIkFk}}** (IK-Ziel und Pol auf die FK-Pose) und **{{MbIkKnopfFkIk}}** (FK-Schlüssel auf die IK-Pose); erst einrasten, dann den Regler stellen, und das Glied springt nicht. G verschiebt das IK-Ziel nur bei IK, sonst sagt eine Meldung, was zu tun ist. Jede Aktion steht auch im Menü **{{MbBereichAnimation}}** (Gruppe **{{MbIkGruppe}}**), im Rechtsklick der Ansicht und des Griffs und in der Suche X / F3.",
        "**{{MbRgAnlegen}}** 之后, 绑定的每条手臂和腿都有双骨骼 IK: 末端骨骼 (手、脚) 到达其目标, 肘或膝位于根部、目标和极向目标构成的平面内并指向极向 - 这正是 Blender 设置极向目标的规则, 也就是 3ds Max HI 解算器的旋转目标。IK 与 Blender 一样是对该帧姿势的改动, 动画中肢体的扭转得以保留。结果是同一层中上肢段、下肢段和末端骨骼的 FK 关键帧: 摄影表和曲线立即显示, 烘焙到 GLA 与以前一样。**像 3ds Max:** 用 W 抓住手或脚就是 IK, 切换是一个关键帧; 用 E 旋转上肢段或下肢段就是 FK。开启 **{{MbIkAuto}}** (默认开启) 时每次切换都先捕捉另一侧, 不会跳变。**{{MbIkRollout}}** 分组中每个肢体一行, 行上的按钮显示并切换 IK 或 FK, 所选肢体另有 **{{MbIkKnopfEinrasten}}** ([[modell-animationsfeld#ikfk]])。**像 Blender:** 每个肢体都有 Rigify 的 IK-FK 滑块 (0 = IK, 1 = FK) 以及按钮 **{{MbIkKnopfIkFk}}** (IK 目标和极向到 FK 姿势) 和 **{{MbIkKnopfFkIk}}** (FK 关键帧到 IK 姿势); 先捕捉再调滑块, 肢体就不会跳变。G 只在 IK 下移动 IK 目标, 否则会有提示说明该怎么做。每个操作也在菜单 **{{MbBereichAnimation}}** (组 **{{MbIkGruppe}}**)、视图和控制器的右键以及搜索 X / F3 中。",
        "**{{MbRgAnlegen}}** の後、リグの各腕と脚に 2 ボーン IK があります: 末端のボーン (手、足) がターゲットに届き、肘や膝は付け根・ターゲット・ポールターゲットの平面にあってポールを向きます - Blender がポールターゲットに使う規則で、3ds Max の HI ソルバーのスイベルターゲットです。IK は Blender と同じくそのフレームのポーズへの変更なので、アニメーションの肢のねじれは残ります。結果は同じレイヤーの上肢・下肢・末端ボーンの FK キーです: ドープシートとカーブにすぐ表示され、GLA へのベイクも従来どおりです。**3ds Max 風:** W で手や足をつかむと IK、切り替えはキーです。E で上肢や下肢を回すと FK。**{{MbIkAuto}}** (既定でオン) なら切り替えのたびに反対側を先にスナップし、何も飛びません。グループ **{{MbIkRollout}}** には肢ごとに 1 行があり、行のボタンが IK か FK かを示して切り替えます。選んだ肢には **{{MbIkKnopfEinrasten}}** があります ([[modell-animationsfeld#ikfk]])。**Blender 風:** 各肢に Rigify の IK-FK スライダー (0 = IK、1 = FK) とボタン **{{MbIkKnopfIkFk}}** (IK ターゲットとポールを FK ポーズへ) と **{{MbIkKnopfFkIk}}** (FK キーを IK ポーズへ) があります。先にスナップしてからスライダーを動かせば肢は飛びません。G は IK のときだけ IK ターゲットを動かし、それ以外はメッセージが手順を示します。どの操作もメニュー **{{MbBereichAnimation}}** (グループ **{{MbIkGruppe}}**)、ビューとコントロールの右クリック、検索 X / F3 にあります。",
        "Tras **{{MbRgAnlegen}}** cada brazo y pierna del rig tiene IK de dos huesos: el hueso final (mano, pie) alcanza su objetivo, el codo o la rodilla quedan en el plano de raíz, objetivo y objetivo de polo y apuntan al polo, la regla con la que Blender coloca su objetivo de polo y el objetivo de giro del solucionador HI de 3ds Max. Como en Blender, la IK es un cambio sobre la pose del fotograma, así se conserva el giro de los miembros de la animación. El resultado son claves FK del segmento superior, el inferior y el hueso final en la misma capa: la hoja de claves y las curvas lo muestran al instante y hornear en el GLA funciona como antes. **Como 3ds Max:** agarrar la mano o el pie con W es IK y el cambio es una clave; girar un segmento con E es FK. Con **{{MbIkAuto}}** (activado de serie) cada cambio ajusta antes el otro lado y nada salta. El grupo **{{MbIkRollout}}** tiene una fila por miembro, cuyo botón muestra IK o FK y lo cambia, y **{{MbIkKnopfEinrasten}}** para el miembro elegido ([[modell-animationsfeld#ikfk]]). **Como Blender:** cada miembro tiene el deslizador IK-FK de Rigify (0 = IK, 1 = FK) y los botones **{{MbIkKnopfIkFk}}** (objetivo IK y polo a la pose FK) y **{{MbIkKnopfFkIk}}** (claves FK a la pose IK); primero ajusta y luego mueve el deslizador, y el miembro no salta. G mueve el objetivo IK solo en IK; si no, un mensaje dice qué hacer. Cada acción está también en el menú **{{MbBereichAnimation}}** (grupo **{{MbIkGruppe}}**), en el clic derecho de la vista y del control y en la búsqueda X / F3.",
        "Po **{{MbRgAnlegen}}** má každá paže a noha rigu dvoukostní IK: koncová kost (ruka, chodidlo) dosáhne cíle, loket nebo koleno leží v rovině z kořene, cíle a cíle pólu a míří k pólu - pravidlo, podle kterého Blender nastavuje cíl pólu, a cíl natočení HI řešiče v 3ds Max. IK je jako v Blenderu změna pózy snímku, takže zkrut končetin z animace zůstane. Výsledkem jsou klíče FK horní a dolní části a koncové kosti ve stejné vrstvě: dopesheet a křivky to ukážou hned a zapečení do GLA funguje jako dřív. **Jako 3ds Max:** uchopit ruku nebo chodidlo pomocí W je IK, přepnutí je klíč; otočit horní nebo dolní část pomocí E je FK. S **{{MbIkAuto}}** (výchozí zapnuto) každé přepnutí nejprve přichytí druhou stranu a nic neskočí. Skupina **{{MbIkRollout}}** má pro každou končetinu řádek, jehož tlačítko ukazuje IK nebo FK a přepíná je, a pro vybranou končetinu **{{MbIkKnopfEinrasten}}** ([[modell-animationsfeld#ikfk]]). **Jako Blender:** každá končetina má posuvník IK-FK z Rigify (0 = IK, 1 = FK) a tlačítka **{{MbIkKnopfIkFk}}** (cíl IK a pól na pózu FK) a **{{MbIkKnopfFkIk}}** (klíče FK na pózu IK); nejprve přichytit, pak posunout posuvník a končetina neskočí. G posouvá cíl IK jen v IK, jinak zpráva řekne, co dělat. Každá akce je i v nabídce **{{MbBereichAnimation}}** (skupina **{{MbIkGruppe}}**), v pravém tlačítku pohledu a ovladače a ve vyhledávání X / F3.",
    ),
    "bild_ik": ("IK on the right arm like 3ds Max: the target cross at the hand, the line from the shoulder, the pole target behind the elbow and the group IK/FK.",
                "IK am rechten Arm wie 3ds Max: das Zielkreuz an der Hand, die Linie von der Schulter, das Pol-Ziel hinter dem Ellbogen und die Gruppe IK/FK.",
                "像 3ds Max 一样在右臂上使用 IK: 手上的目标十字、来自肩部的线、肘后的极向目标以及 IK/FK 分组。",
                "3ds Max 風の右腕の IK: 手のターゲットの十字、肩からの線、肘の後ろのポールターゲット、IK/FK グループ。",
                "IK en el brazo derecho como 3ds Max: la cruz del objetivo en la mano, la línea desde el hombro, el objetivo de polo tras el codo y el grupo IK/FK.",
                "IK na pravé paži jako v 3ds Max: křížek cíle u ruky, čára od ramene, cíl pólu za loktem a skupina IK/FK."),
    "h_rolle": ("Foot roll", "Fußrolle", "脚部滚动", "フットロール", "Balanceo del pie", "Odval chodidla"),
    "rolle": (
        "Rotate the heel, ball or toe tip control (E like 3ds Max, R like Blender) or use **{{MbIkRolle}}** in the rollout: like the heel control of Rigify the "
        "foot rolls back on the heel below 0 and forward on the ball above 0, while the toes stay on the ground; **{{MbIkSpitze}}** puts a share of the forward "
        "roll on the toe tip. Skeletons without toe bones (Jedi Academy _humanoid) have no ball: the forward roll goes around the toe tip.",
        "Den Griff an Ferse, Ballen oder Zehenspitze drehen (E wie 3ds Max, R wie Blender) oder **{{MbIkRolle}}** im Rollout: wie beim Fersen-Regler von Rigify "
        "rollt der Fuß unter 0 nach hinten auf die Ferse und über 0 nach vorn über den Ballen, die Zehen bleiben liegen; **{{MbIkSpitze}}** legt einen Teil der "
        "Rolle nach vorn auf die Zehenspitze. Skelette ohne Zehenknochen (Jedi Academy _humanoid) haben keinen Ballen: die Rolle nach vorn geht um die Zehenspitze.",
        "旋转脚跟、脚掌或脚尖的控制器 (E 如 3ds Max, R 如 Blender) 或使用卷展栏中的 **{{MbIkRolle}}**: 像 Rigify 的脚跟控制器一样, 小于 0 时脚向后滚到"
        "脚跟, 大于 0 时向前越过脚掌, 脚趾保持着地; **{{MbIkSpitze}}** 把向前滚动的一部分放到脚尖上。没有脚趾骨骼的骨架 (Jedi Academy _humanoid) 没有"
        "脚掌: 向前滚动绕脚尖进行。",
        "かかと、母指球、つま先のコントロールを回す (3ds Max 風は E、Blender 風は R) か、ロールアウトの **{{MbIkRolle}}** を使います: Rigify のかかとコント"
        "ロールと同じく、0 未満でかかとへ後ろに、0 より大きいと母指球を越えて前に転がり、つま先は接地したままです。**{{MbIkSpitze}}** は前方へのロールの"
        "一部をつま先に乗せます。つま先のボーンのないスケルトン (Jedi Academy _humanoid) には母指球がなく、前方へのロールはつま先を中心に回ります。",
        "Gira el control del talón, la planta o la punta (E como 3ds Max, R como Blender) o usa **{{MbIkRolle}}** en el panel: como el control del talón de "
        "Rigify, por debajo de 0 el pie rueda hacia atrás sobre el talón y por encima de 0 hacia delante sobre la planta, mientras los dedos quedan en el suelo; "
        "**{{MbIkSpitze}}** pone una parte del balanceo hacia delante en la punta. Los esqueletos sin huesos de los dedos (Jedi Academy _humanoid) no tienen "
        "planta: el balanceo hacia delante gira sobre la punta.",
        "Otočte ovladač paty, bříška nebo špičky (E jako 3ds Max, R jako Blender) nebo použijte **{{MbIkRolle}}** v rolloutu: jako ovladač paty v Rigify se "
        "chodidlo pod 0 odvaluje dozadu na patu a nad 0 dopředu přes bříško, prsty zůstanou na zemi; **{{MbIkSpitze}}** dá část odvalu dopředu na špičku. "
        "Kostry bez kostí prstů (Jedi Academy _humanoid) nemají bříško: odval dopředu jde kolem špičky."),
    "bild_rolle": ("Foot roll back on the heel: the heel stays on the ground, the knee follows by IK.", "Fußrolle nach hinten auf die Ferse: die Ferse bleibt liegen, das Knie folgt per IK.",
                   "向后滚到脚跟: 脚跟保持着地, 膝盖通过 IK 跟随。", "かかとへ後ろに転がすフットロール: かかとは接地したまま、膝は IK で追従します。",
                   "Balanceo hacia atrás sobre el talón: el talón queda en el suelo y la rodilla sigue por IK.", "Odval dozadu na patu: pata zůstane na zemi, koleno sleduje pomocí IK."),
    "h_pfade": ("Dragging the paths of hands, feet and the saber tip", "Pfade an Händen, Füßen und Säbelspitze ziehen", "拖动手、脚和光剑尖端的路径",
                "手・足・セーバー先端のパスをドラッグ", "Arrastrar las trayectorias de manos, pies y punta del sable", "Tažení drah rukou, chodidel a hrotu meče"),
    "pfade": (
        "Add a path for a hand or a foot (**{{MbRgPfadHinzu}}**) or for the tip of the saber attached at *r_hand (**{{MbIkPfadSpitze}}**). A dot dragged in the "
        "view becomes the IK target of the limb at that frame, like moving keys on the trajectory in 3ds Max: the point stays under the mouse, the limb gets an "
        "IK key and FK keys, and one Ctrl+Z takes the whole drag back. Dragging the saber tip moves the saber hand by the same way. The paths of pelvis and root "
        "move the bone as before.",
        "Einen Pfad für eine Hand oder einen Fuß (**{{MbRgPfadHinzu}}**) oder für die Spitze des Säbels an *r_hand (**{{MbIkPfadSpitze}}**) anlegen. Ein Punkt, "
        "den man in der Ansicht zieht, wird an diesem Frame das IK-Ziel des Glieds, wie Schlüssel auf der Trajektorie in 3ds Max: der Punkt bleibt unter der "
        "Maus, das Glied bekommt einen IK-Schlüssel und FK-Schlüssel, und ein Strg+Z nimmt den ganzen Zug zurück. Die Säbelspitze ziehen bewegt die Säbelhand um "
        "denselben Weg. Die Pfade von Becken und Wurzel bewegen den Knochen wie bisher.",
        "为手或脚添加路径 (**{{MbRgPfadHinzu}}**), 或为挂在 *r_hand 上的光剑尖端添加路径 (**{{MbIkPfadSpitze}}**)。在视图中拖动的点会成为该帧肢体的 IK "
        "目标, 就像在 3ds Max 的轨迹上移动关键帧: 点保持在鼠标下, 肢体得到一个 IK 关键帧和 FK 关键帧, 一次 Ctrl+Z 撤销整个拖动。拖动光剑尖端会让持剑的手"
        "移动同样的距离。骨盆和根部的路径仍像以前一样移动骨骼。",
        "手や足のパス (**{{MbRgPfadHinzu}}**)、または *r_hand に付けたセーバーの先端のパス (**{{MbIkPfadSpitze}}**) を追加します。ビューでドラッグした点"
        "がそのフレームの肢の IK ターゲットになります。3ds Max の軌跡上でキーを動かすのと同じです: 点はマウスの下に留まり、肢に IK キーと FK キーが付き、"
        "Ctrl+Z 1 回でドラッグ全体を取り消します。セーバーの先端をドラッグすると、セーバーの手が同じだけ動きます。骨盤とルートのパスは従来どおりボーンを動か"
        "します。",
        "Añade una trayectoria para una mano o un pie (**{{MbRgPfadHinzu}}**) o para la punta del sable unido a *r_hand (**{{MbIkPfadSpitze}}**). Un punto que "
        "arrastras en la vista pasa a ser el objetivo IK del miembro en ese fotograma, como mover claves sobre la trayectoria en 3ds Max: el punto queda bajo el "
        "ratón, el miembro recibe una clave IK y claves FK, y un Ctrl+Z deshace todo el arrastre. Arrastrar la punta del sable mueve la mano del sable lo mismo. "
        "Las trayectorias de la pelvis y la raíz mueven el hueso como antes.",
        "Přidejte dráhu pro ruku nebo chodidlo (**{{MbRgPfadHinzu}}**) nebo pro hrot meče připojeného k *r_hand (**{{MbIkPfadSpitze}}**). Bod tažený v pohledu "
        "se na tom snímku stane cílem IK končetiny, jako posouvání klíčů po trajektorii v 3ds Max: bod zůstane pod myší, končetina dostane klíč IK a klíče FK a "
        "jedno Ctrl+Z vrátí celé tažení. Tažení hrotu meče posune ruku s mečem o stejnou dráhu. Dráhy pánve a kořene posouvají kost jako dřív."),
    "bild_pfade": ("The path of the saber tip dragged at a frame: the saber hand follows by IK.", "Der Pfad der Säbelspitze an einem Frame gezogen: die Säbelhand folgt per IK.",
                   "在某一帧拖动光剑尖端的路径: 持剑的手通过 IK 跟随。", "あるフレームでセーバー先端のパスをドラッグ: セーバーの手が IK で追従します。",
                   "La trayectoria de la punta del sable arrastrada en un fotograma: la mano del sable sigue por IK.", "Dráha hrotu meče tažená na snímku: ruka s mečem sleduje pomocí IK."),
    "h_raum": ("Space switching", "Raumwechsel", "空间切换", "スペース切り替え", "Cambio de espacio", "Přepínání prostoru"),
    "raum": (
        "The IK target of a limb can follow the world, the pelvis, the chest, the head or the other hand from a frame on (Child Of in Blender, the link "
        "constraint in 3ds Max): choose it under **{{MbIkRollout}}** or in the menu. The switch writes a key one frame before in the old space and one at the "
        "frame in the new space, so nothing jumps. **{{MbIkSaebelHand}}** lets the saber hand follow the other hand, for two-handed grips and handing the saber "
        "over; back with **{{MbIkRaumWelt}}**. The game attaches the saber to *r_hand by code, so the animation always moves the hands.",
        "Das IK-Ziel eines Glieds kann ab einem Frame der Welt, dem Becken, der Brust, dem Kopf oder der anderen Hand folgen (Child Of in Blender, die "
        "Verknüpfungs-Beschränkung in 3ds Max): unter **{{MbIkRollout}}** oder im Menü wählen. Der Wechsel schreibt einen Schlüssel ein Frame davor im alten Raum "
        "und einen am Frame im neuen, nichts springt. **{{MbIkSaebelHand}}** lässt die Säbelhand der anderen Hand folgen, für Griffe mit beiden Händen und das "
        "Übergeben des Säbels; zurück mit **{{MbIkRaumWelt}}**. Das Spiel hängt den Säbel per Code an *r_hand, die Animation bewegt also immer die Hände.",
        "肢体的 IK 目标可以从某一帧起跟随世界、骨盆、胸部、头部或另一只手 (Blender 的子级约束, 3ds Max 的链接约束): 在 **{{MbIkRollout}}** 或菜单中选择。"
        "切换会在前一帧以旧空间写一个关键帧, 在该帧以新空间写一个, 不会跳变。**{{MbIkSaebelHand}}** 让持剑的手跟随另一只手, 用于双手握持和交接光剑; 用 "
        "**{{MbIkRaumWelt}}** 返回。游戏用代码把光剑挂在 *r_hand 上, 所以动画始终移动的是双手。",
        "肢の IK ターゲットは、あるフレームからワールド、骨盤、胸、頭、反対の手に従えます (Blender の Child Of、3ds Max のリンク拘束): **{{MbIkRollout}}** "
        "またはメニューで選びます。切り替えは 1 フレーム前に古いスペースで、そのフレームに新しいスペースでキーを書くので、何も飛びません。"
        "**{{MbIkSaebelHand}}** でセーバーの手が反対の手に従い、両手持ちやセーバーの持ち替えに使えます。戻すには **{{MbIkRaumWelt}}**。ゲームはセーバーを"
        "コードで *r_hand に付けるので、アニメーションが動かすのは常に手です。",
        "El objetivo IK de un miembro puede seguir desde un fotograma al mundo, la pelvis, el pecho, la cabeza o la otra mano (Child Of en Blender, la restricción "
        "de vínculo en 3ds Max): elígelo en **{{MbIkRollout}}** o en el menú. El cambio escribe una clave un fotograma antes en el espacio anterior y otra en el "
        "fotograma en el nuevo, nada salta. **{{MbIkSaebelHand}}** hace que la mano del sable siga a la otra mano, para agarres a dos manos y para pasar el sable; "
        "se vuelve con **{{MbIkRaumWelt}}**. El juego une el sable a *r_hand por código, así que la animación siempre mueve las manos.",
        "Cíl IK končetiny může od snímku sledovat svět, pánev, hruď, hlavu nebo druhou ruku (Child Of v Blenderu, vazba Link v 3ds Max): zvolte ho v "
        "**{{MbIkRollout}}** nebo v nabídce. Přepnutí zapíše klíč o snímek dříve ve starém prostoru a klíč na snímku v novém, nic neskočí. **{{MbIkSaebelHand}}** "
        "nechá ruku s mečem sledovat druhou ruku, pro úchop oběma rukama a předání meče; zpět pomocí **{{MbIkRaumWelt}}**. Hra připojuje meč k *r_hand kódem, "
        "animace tedy vždy hýbe rukama."),
    "bild_raum": ("The right hand follows the left hand (dashed line to the space); the saber grip is selected, the group IK/FK shows the space.",
                  "Die rechte Hand folgt der linken (gestrichelte Linie zum Raum); der Säbelgriff ist gewählt, die Gruppe IK/FK zeigt den Raum.",
                  "右手跟随左手 (到空间的虚线); 光剑握把已选中, IK/FK 分组显示空间。", "右手が左手に従います (スペースへの破線)。セーバーグリップを選択中で、IK/FK グループにスペースが出ます。",
                  "La mano derecha sigue a la izquierda (línea discontinua hacia el espacio); el agarre del sable está seleccionado y el grupo IK/FK muestra el espacio.",
                  "Pravá ruka sleduje levou (přerušovaná čára k prostoru); úchop meče je vybrán, skupina IK/FK ukazuje prostor."),
    "h_mehr": ("Reduce a selection, pose library as a file", "Auswahl reduzieren, Posen-Bibliothek als Datei", "精简所选内容, 姿势库作为文件",
               "選択の削減、ファイルとしてのポーズライブラリ", "Reducir una selección, biblioteca de poses como archivo", "Redukce výběru, knihovna póz jako soubor"),
    "mehr": (
        "**{{MbKvReduzieren}}** can now work on **{{MbIkRedNurGewaehlte}}** and **{{MbIkRedSpanne}}**: keys outside stay bit for bit, and every bone frame stays "
        "within the compression bound. **{{MbIkPosenSpeichern}}** writes the pose library to Documents\\behaved\\posen, **{{MbIkPosenLaden}}** brings it back for "
        "another model or session.",
        "**{{MbKvReduzieren}}** geht jetzt auch für **{{MbIkRedNurGewaehlte}}** und **{{MbIkRedSpanne}}**: Schlüssel außerhalb bleiben Bit für Bit, jedes "
        "Knochen-Frame bleibt innerhalb der Kompressionsgrenze. **{{MbIkPosenSpeichern}}** schreibt die Posen-Bibliothek nach Dokumente\\behaved\\posen, "
        "**{{MbIkPosenLaden}}** holt sie für ein anderes Modell oder eine andere Sitzung zurück.",
        "**{{MbKvReduzieren}}** 现在也可用于 **{{MbIkRedNurGewaehlte}}** 和 **{{MbIkRedSpanne}}**: 范围外的关键帧逐位不变, 每个骨骼帧都保持在压缩界限内。"
        "**{{MbIkPosenSpeichern}}** 把姿势库写入 文档\\behaved\\posen, **{{MbIkPosenLaden}}** 为另一个模型或会话读回。",
        "**{{MbKvReduzieren}}** は **{{MbIkRedNurGewaehlte}}** と **{{MbIkRedSpanne}}** にも使えます: 範囲外のキーはビット単位でそのまま、各ボーンフレームは"
        "圧縮の上限内に収まります。**{{MbIkPosenSpeichern}}** はポーズライブラリを ドキュメント\\behaved\\posen に書き出し、**{{MbIkPosenLaden}}** で別の"
        "モデルやセッションに戻せます。",
        "**{{MbKvReduzieren}}** funciona ahora también con **{{MbIkRedNurGewaehlte}}** y **{{MbIkRedSpanne}}**: las claves de fuera quedan bit a bit y cada "
        "fotograma de hueso queda dentro del límite de compresión. **{{MbIkPosenSpeichern}}** escribe la biblioteca de poses en Documentos\\behaved\\posen y "
        "**{{MbIkPosenLaden}}** la recupera para otro modelo u otra sesión.",
        "**{{MbKvReduzieren}}** nyní funguje i pro **{{MbIkRedNurGewaehlte}}** a **{{MbIkRedSpanne}}**: klíče mimo zůstanou bit po bitu a každý snímek kosti "
        "zůstane v mezích komprese. **{{MbIkPosenSpeichern}}** zapíše knihovnu póz do Dokumenty\\behaved\\posen, **{{MbIkPosenLaden}}** ji vrátí pro jiný model "
        "nebo relaci."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    z = X[schluessel]
    return T(z[0], z[1], z[2], z[3])


def seite():
    """Die Seite modell-ik (Gruppe Missionen, nach modell-kurven)."""
    return {
        "id": "modell-ik",
        "gruppe": "mission",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("img", "h_mb6_ik", _t("bild_ik"), "breit"),
            ("h2", "ikfk", _t("h_ik")),
            ("p", _t("ik")),
            ("h2", "fussrolle", _t("h_rolle")),
            ("p", _t("rolle")),
            ("img", "h_mb6_fussrolle", _t("bild_rolle"), "breit"),
            ("h2", "pfade", _t("h_pfade")),
            ("p", _t("pfade")),
            ("img", "h_mb6_pfade", _t("bild_pfade"), "breit"),
            ("h2", "raum", _t("h_raum")),
            ("p", _t("raum")),
            ("img", "h_mb6_raum", _t("bild_raum"), "breit"),
            ("h2", "mehr", _t("h_mehr")),
            ("p", _t("mehr")),
        ],
    }
