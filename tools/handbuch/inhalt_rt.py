"""Modell-Block Stufe 5: die Seite "Model-Tab: Retargeting" (Quelle waehlen, Ketten-Zuordnung und Rig-Vorlagen, Retarget-Pose,
Schritte Becken/FK/IK/Wurzel mit Reach und Fusskontakten, Fussrutsch-Messer und Spruenge, Backen auf einen Platz, im Spiel).

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_ph.py.
"""

X = {
    "titel": ("Model tab: retargeting", "Model-Tab: Retargeting", "模型选项卡: 重定向", "モデルタブ: リターゲット", "Pestaña Model: retargeting",
              "Karta Model: retargeting"),
    "kurz": (
        "Give your figure the animation of another skeleton - JK2 to JKA, JKA to a Movie Duels figure, a human walk onto a creature or your own rig - with the same pose, mapped chains, scaled steps, planted feet and the result baked into the GLA of your choice.",
        "Gib deiner Figur die Animation eines anderen Skeletts - JK2 auf JKA, JKA auf eine Movie-Duels-Figur, einen Menschengang auf eine Kreatur oder ein eigenes Rig - mit gleicher Pose, zugeordneten Ketten, skalierten Schritten, festen Füßen und dem Ergebnis in der GLA deiner Wahl.",
        "让你的角色获得另一个骨架的动画 - JK2 到 JKA、JKA 到 Movie Duels 角色、人的步行到生物或自己的绑定 - 姿势相同、链条对应、步幅缩放、脚踏实地, 结果烘焙到你选择的 GLA。",
        "別のスケルトンのアニメーションをフィギュアに与えます - JK2 から JKA、JKA から Movie Duels のフィギュア、人の歩きをクリーチャーや自作リグへ - 同じポーズ、対応付けたチェーン、拡縮した歩幅、地面に着いた足で、結果は選んだ GLA にベイクします。",
        "Da a tu figura la animación de otro esqueleto (JK2 a JKA, JKA a una figura de Movie Duels, un paso humano a una criatura o a tu propio rig) con la misma pose, cadenas emparejadas, pasos escalados, pies firmes y el resultado horneado en el GLA que elijas.",
        "Dejte postavě animaci jiné kostry - JK2 na JKA, JKA na postavu Movie Duels, lidskou chůzi na tvora nebo vlastní rig - se stejnou pózou, přiřazenými řetězy, škálovanými kroky, pevnými chodidly a výsledkem zapečeným do GLA podle volby."),
    "h_quelle": ("Choose the source", "Die Quelle wählen", "选择来源", "ソースを選ぶ", "Elegir el origen", "Výběr zdroje"),
    "quelle": (
        "Open your figure in the model tab and go to the area **Animation**. In the group **{{AzRUebertragen}}** of the panel on the right, in the menu **Animation > {{RtRollout}}**, in the right-click of the view (**Control Rig > {{RtRollout}}**) or in the search (X, F3 in Blender mode) choose **{{RtQuelleWaehlen}}**: any GLA of the game data or a .gla on disk. Then pick the **{{RtSequenz}}** of the source (a line of its animation.cfg). **{{RtUebertragen}}** shows the result on your figure at once and the source as a skeleton beside it; **{{RtAbspielen}}** plays both together. The figure shows the result packed like the game (compressBone): what you see is what the game plays.",
        "Öffne deine Figur im Model-Tab und gehe in den Bereich **Animation**. In der Gruppe **{{AzRUebertragen}}** des rechten Felds, im Menü **Animation > {{RtRollout}}**, im Rechtsklick der Ansicht (**Control Rig > {{RtRollout}}**) oder in der Suche (X, im Blender-Modus F3) wählst du **{{RtQuelleWaehlen}}**: jede GLA der Spieldaten oder eine .gla auf der Platte. Dann die **{{RtSequenz}}** der Quelle (eine Zeile ihrer animation.cfg). **{{RtUebertragen}}** zeigt das Ergebnis sofort auf deiner Figur und die Quelle als Skelett daneben; **{{RtAbspielen}}** spielt beide zusammen. Die Figur zeigt das Ergebnis gepackt wie im Spiel (compressBone): du siehst, was das Spiel spielt.",
        "在模型选项卡中打开角色, 进入 **Animation** 区域。在右侧面板的 **{{AzRUebertragen}}** 分组、菜单 **Animation > {{RtRollout}}**、视图右键 (**Control Rig > {{RtRollout}}**) 或搜索 (X, Blender 模式为 F3) 中选择 **{{RtQuelleWaehlen}}**: 游戏数据中的任意 GLA 或磁盘上的 .gla。然后选择来源的 **{{RtSequenz}}** (其 animation.cfg 中的一行)。**{{RtUebertragen}}** 立即在角色上显示结果, 并在旁边以骨架显示来源; **{{RtAbspielen}}** 同时播放两者。角色显示按游戏方式压缩 (compressBone) 的结果: 所见即游戏所播放。",
        "モデルタブでフィギュアを開き、**Animation** の領域へ。右側のパネルのグループ **{{AzRUebertragen}}**、メニュー **Animation > {{RtRollout}}**、ビューの右クリック (**Control Rig > {{RtRollout}}**)、または検索 (X、Blender 風は F3) で **{{RtQuelleWaehlen}}** を選びます: ゲームデータの任意の GLA かディスク上の .gla。次にソースの **{{RtSequenz}}** (animation.cfg の 1 行) を選びます。**{{RtUebertragen}}** で結果がすぐフィギュアに表示され、ソースはスケルトンとして隣に表示されます。**{{RtAbspielen}}** で両方を一緒に再生します。フィギュアはゲームと同じく圧縮 (compressBone) した結果を表示します: 見えるものがゲームの再生です。",
        "Abre tu figura en la pestaña Model y ve al área **Animation**. En el grupo **{{AzRUebertragen}}** del panel de la derecha, en el menú **Animation > {{RtRollout}}**, en el clic derecho de la vista (**Control Rig > {{RtRollout}}**) o en la búsqueda (X, F3 en modo Blender) elige **{{RtQuelleWaehlen}}**: cualquier GLA de los datos del juego o un .gla del disco. Después la **{{RtSequenz}}** del origen (una línea de su animation.cfg). **{{RtUebertragen}}** muestra el resultado al instante en tu figura y el origen como esqueleto al lado; **{{RtAbspielen}}** reproduce ambos a la vez. La figura muestra el resultado empaquetado como el juego (compressBone): lo que ves es lo que reproduce el juego.",
        "Otevřete postavu v kartě Model a přejděte do oblasti **Animation**. Ve skupině **{{AzRUebertragen}}** panelu vpravo, v nabídce **Animation > {{RtRollout}}**, v pravém kliknutí pohledu (**Control Rig > {{RtRollout}}**) nebo ve vyhledávání (X, v režimu Blender F3) zvolte **{{RtQuelleWaehlen}}**: libovolné GLA z dat hry nebo soubor .gla na disku. Pak **{{RtSequenz}}** zdroje (řádek jeho animation.cfg). **{{RtUebertragen}}** ukáže výsledek hned na postavě a zdroj jako kostru vedle ní; **{{RtAbspielen}}** přehraje oba spolu. Postava ukazuje výsledek zabalený jako ve hře (compressBone): vidíte, co hra přehraje.",
    ),
    "bild_uebertragen": ("The JK2 walk on a Movie Duels figure, the JK2 skeleton beside it, the rollout Retargeting with foot sliding meter.",
                         "Der JK2-Gang auf einer Movie-Duels-Figur, das JK2-Skelett daneben, das Rollout Retargeting mit Fußrutsch-Messer.",
                         "JK2 的步行在 Movie Duels 角色上, 旁边是 JK2 骨架, 重定向卷展栏带脚打滑计。",
                         "Movie Duels のフィギュアに JK2 の歩き、隣に JK2 のスケルトン、足の滑りメーター付きのロールアウト「リターゲット」。",
                         "El paso de JK2 en una figura de Movie Duels, el esqueleto de JK2 al lado, el panel Retargeting con el medidor de deslizamiento.",
                         "Chůze z JK2 na postavě Movie Duels, kostra JK2 vedle, rollout Retargeting s měřičem klouzání."),
    "h_ketten": ("Chains and rig templates", "Ketten und Rig-Vorlagen", "链条与绑定模板", "チェーンとリグのテンプレート", "Cadenas y plantillas de rig",
                 "Řetězy a šablony rigu"),
    "ketten": (
        "Both skeletons are described by chains - spine, neck, head, clavicles, arms (upper arm, forearm, hand), legs (thigh, shank, foot, toes), fingers and tails. "
        "In 3ds Max mode the table is called **{{RtKettenMax}}** (as in MotionBuilder), in Blender mode **{{RtKettenBlender}}**. It is filled from a template when the "
        "skeleton is known: JKA _humanoid (53 bones), JK2 _humanoid (72), the 34 own skeletons of the base game (rancor, wampa, AT-ST, howler, tauntaun, protocol droid "
        "and all others, recognised by file name and bone names) and for rigs from other programs HumanIK/Mixamo (mixamorig:...), 3ds Max Biped and CAT. Any other "
        "skeleton gets chains suggested by the bone recognition. Chains with the same name are mapped; a different number of bones is spread over the length of the "
        "chain, fingers by their number. Change any row by hand (it gets a *); **{{RtAutoLeer}}** keeps your rows. Twist bones (lhumerusX, lradiusX, lfemurX) take "
        "their share of the turn of the forearm or thigh; fingers and the saber tag that hang under the forearm stay with the hand. Mixamo files are not shipped - "
        "download them yourself with your account, the template maps them.",
        "Beide Skelette werden durch Ketten beschrieben - Wirbelsäule, Hals, Kopf, Schlüsselbeine, Arme (Oberarm, Unterarm, Hand), Beine (Oberschenkel, "
        "Unterschenkel, Fuß, Zehen), Finger und Anhänge. Im 3ds-Max-Modus heißt die Tabelle **{{RtKettenMax}}** (wie in MotionBuilder), im Blender-Modus "
        "**{{RtKettenBlender}}**. Sie wird aus einer Vorlage gefüllt, wenn das Skelett bekannt ist: JKA _humanoid (53 Knochen), JK2 _humanoid (72), die 34 eigenen "
        "Skelette des Basisspiels (Rancor, Wampa, AT-ST, Howler, Tauntaun, Protokolldroide und alle übrigen, erkannt an Dateiname und Knochennamen) und für Rigs aus "
        "anderen Programmen HumanIK/Mixamo (mixamorig:...), 3ds Max Biped und CAT. Jedes andere Skelett bekommt Ketten aus der Knochen-Erkennung vorgeschlagen. "
        "Ketten gleichen Namens werden zugeordnet; eine andere Zahl Knochen wird über die Länge der Kette verteilt, Finger über ihre Nummer. Jede Zeile änderst du "
        "von Hand (sie bekommt ein *); **{{RtAutoLeer}}** lässt deine Zeilen stehen. Drehknochen (lhumerusX, lradiusX, lfemurX) nehmen ihren Anteil an der Drehung "
        "von Unterarm oder Oberschenkel; Finger und der Säbel-Tag, die unter dem Unterarm hängen, bleiben bei der Hand. Mixamo-Dateien liegen nicht bei - lade sie "
        "selbst mit deinem Konto, die Vorlage ordnet sie zu.",
        "两个骨架都用链条描述 - 脊柱、颈、头、锁骨、手臂 (上臂、前臂、手)、腿 (大腿、小腿、脚、脚趾)、手指和附属物。3ds Max 模式下表格称为 **{{RtKettenMax}}** "
        "(与 MotionBuilder 相同), Blender 模式下称为 **{{RtKettenBlender}}**。已知骨架会用模板填充: JKA _humanoid (53 块骨骼)、JK2 _humanoid (72)、基础游戏的 34 个"
        "自有骨架 (rancor、wampa、AT-ST、howler、tauntaun、礼仪机器人及其他, 按文件名和骨骼名识别), 以及来自其他软件的 HumanIK/Mixamo (mixamorig:...)、3ds Max "
        "Biped 和 CAT。其他骨架由骨骼识别建议链条。同名链条相互对应; 骨骼数不同时按链长分布, 手指按编号。任何一行都可手动更改 (标记 *); **{{RtAutoLeer}}** 保留你的行。"
        "扭转骨 (lhumerusX、lradiusX、lfemurX) 分担前臂或大腿的旋转; 挂在前臂下的手指和光剑挂点随手移动。不附带 Mixamo 文件 - 请用自己的账号下载, 模板会映射它们。",
        "両方のスケルトンはチェーンで記述されます - 背骨、首、頭、鎖骨、腕 (上腕、前腕、手)、脚 (太もも、すね、足、つま先)、指、付属。3ds Max 風では表は "
        "**{{RtKettenMax}}** (MotionBuilder と同じ)、Blender 風では **{{RtKettenBlender}}** と呼びます。既知のスケルトンはテンプレートから埋まります: JKA _humanoid "
        "(ボーン 53)、JK2 _humanoid (72)、ベースゲームの 34 の独自スケルトン (ランコア、ワンパ、AT-ST、ハウラー、トーントーン、プロトコルドロイドほか全部、ファイル名と"
        "ボーン名で認識)、他のソフトのリグ用に HumanIK/Mixamo (mixamorig:...)、3ds Max Biped、CAT。その他のスケルトンにはボーン認識からチェーンを提案します。同じ名前の"
        "チェーンが対応付けられ、ボーン数が違えばチェーンの長さに沿って配分、指は番号で。どの行も手で変えられ (* が付く)、**{{RtAutoLeer}}** は手で決めた行を残します。"
        "ツイストボーン (lhumerusX、lradiusX、lfemurX) は前腕や太ももの回転を分け合い、前腕の下にある指とセーバーのタグは手と一緒に動きます。Mixamo のファイルは"
        "同梱しません - 自分のアカウントでダウンロードしてください。テンプレートが対応付けます。",
        "Ambos esqueletos se describen con cadenas: columna, cuello, cabeza, clavículas, brazos (brazo, antebrazo, mano), piernas (muslo, pierna, pie, dedos del pie), "
        "dedos y apéndices. En modo 3ds Max la tabla se llama **{{RtKettenMax}}** (como en MotionBuilder), en modo Blender **{{RtKettenBlender}}**. Se rellena con "
        "una plantilla si el esqueleto es conocido: JKA _humanoid (53 huesos), JK2 _humanoid (72), los 34 esqueletos propios del juego base (rancor, wampa, AT-ST, "
        "howler, tauntaun, droide de protocolo y todos los demás, reconocidos por nombre de archivo y de huesos) y, para rigs de otros programas, HumanIK/Mixamo "
        "(mixamorig:...), Biped de 3ds Max y CAT. Cualquier otro esqueleto recibe cadenas sugeridas por el reconocimiento de huesos. Las cadenas del mismo nombre se "
        "emparejan; un número distinto de huesos se reparte a lo largo de la cadena, los dedos por su número. Cambia cualquier fila a mano (lleva un *); "
        "**{{RtAutoLeer}}** mantiene tus filas. Los huesos de giro (lhumerusX, lradiusX, lfemurX) toman su parte del giro del antebrazo o del muslo; los dedos y la "
        "etiqueta del sable que cuelgan del antebrazo siguen a la mano. No se incluyen archivos de Mixamo: descárgalos tú con tu cuenta, la plantilla los empareja.",
        "Obě kostry jsou popsány řetězy - páteř, krk, hlava, klíční kosti, paže (paže, předloktí, ruka), nohy (stehno, bérec, chodidlo, prsty), prsty ruky a "
        "přívěsky. V režimu 3ds Max se tabulka jmenuje **{{RtKettenMax}}** (jako v MotionBuilderu), v režimu Blender **{{RtKettenBlender}}**. Vyplní se ze šablony, "
        "když je kostra známá: JKA _humanoid (53 kostí), JK2 _humanoid (72), 34 vlastních koster základní hry (rancor, wampa, AT-ST, howler, tauntaun, protokolární "
        "droid a všechny ostatní, rozpoznané podle názvu souboru a kostí) a pro rigy z jiných programů HumanIK/Mixamo (mixamorig:...), 3ds Max Biped a CAT. Každá "
        "jiná kostra dostane řetězy navržené rozpoznáním kostí. Řetězy stejného názvu se přiřadí; jiný počet kostí se rozloží po délce řetězu, prsty podle čísla. "
        "Každý řádek můžete změnit ručně (dostane *); **{{RtAutoLeer}}** vaše řádky ponechá. Kroutící kosti (lhumerusX, lradiusX, lfemurX) přebírají svůj podíl "
        "rotace předloktí nebo stehna; prsty a tag meče, které visí pod předloktím, zůstávají u ruky. Soubory Mixamo nejsou přiloženy - stáhněte si je sami svým "
        "účtem, šablona je přiřadí."),
    "h_pose": ("The common pose", "Die gemeinsame Pose", "共同姿势", "共通のポーズ", "La pose común", "Společná póza"),
    "pose": (
        "Before transferring, source and figure must stand in the same pose ({{RtPoseMax}} in 3ds Max mode, {{RtPoseBlender}} in Blender mode) - otherwise arms and "
        "legs come out twisted. The JKA skeleton stands in an A pose (upper arms about 48° down), other rigs often in a T pose. **{{RtHaltungT}}** and "
        "**{{RtHaltungA}}** turn spine and neck upright, arms sideways or 45° down and legs straight down, each limb with the smallest rotation. The rollout shows how "
        "far the two poses still differ. Fine-tune a bone of the figure with the buttons X/Y/Z ±5, save the pair under a name and use it again later.",
        "Vor dem Übertragen müssen Quelle und Figur in derselben Pose stehen ({{RtPoseMax}} im 3ds-Max-Modus, {{RtPoseBlender}} im Blender-Modus) - sonst kommen Arme "
        "und Beine verdreht heraus. Das JKA-Skelett steht in einer A-Pose (Oberarme etwa 48° nach unten), andere Rigs oft in einer T-Pose. **{{RtHaltungT}}** und "
        "**{{RtHaltungA}}** stellen Wirbelsäule und Hals senkrecht, die Arme zur Seite oder 45° nach unten und die Beine senkrecht nach unten, jedes Glied mit der "
        "kleinsten Drehung. Das Rollout zeigt, wie weit die beiden Posen noch abweichen. Einen Knochen der Figur stellst du mit den Knöpfen X/Y/Z ±5 nach, das Paar "
        "speicherst du unter einem Namen und nimmst es später wieder.",
        "传递前, 来源和角色必须处于同一姿势 (3ds Max 模式为 {{RtPoseMax}}, Blender 模式为 {{RtPoseBlender}}) - 否则手臂和腿会扭曲。JKA 骨架为 A 姿势 (上臂约向下 48°), "
        "其他绑定常为 T 姿势。**{{RtHaltungT}}** 和 **{{RtHaltungA}}** 将脊柱和颈部竖直, 手臂侧平或向下 45°, 腿竖直向下, 每个肢体以最小旋转。卷展栏显示两个姿势还相差"
        "多少。用 X/Y/Z ±5 按钮微调角色的骨骼, 以名称保存这组姿势, 以后再用。",
        "転送の前に、ソースとフィギュアは同じポーズで立つ必要があります (3ds Max 風は {{RtPoseMax}}、Blender 風は {{RtPoseBlender}}) - そうでないと腕や脚がねじれます。"
        "JKA のスケルトンは A ポーズ (上腕が約 48° 下向き)、他のリグはよく T ポーズです。**{{RtHaltungT}}** と **{{RtHaltungA}}** は背骨と首を垂直に、腕を横または 45° "
        "下に、脚を真下にし、各部位を最小の回転で合わせます。ロールアウトに 2 つのポーズのずれが表示されます。フィギュアのボーンは X/Y/Z ±5 のボタンで微調整し、組を"
        "名前を付けて保存して後で使えます。",
        "Antes de transferir, origen y figura deben estar en la misma pose ({{RtPoseMax}} en modo 3ds Max, {{RtPoseBlender}} en modo Blender); si no, brazos y "
        "piernas salen retorcidos. El esqueleto de JKA está en pose A (brazos unos 48° hacia abajo), otros rigs a menudo en pose T. **{{RtHaltungT}}** y "
        "**{{RtHaltungA}}** ponen columna y cuello rectos, los brazos a los lados o 45° hacia abajo y las piernas rectas hacia abajo, cada miembro con el giro "
        "mínimo. El panel muestra cuánto difieren aún las dos poses. Ajusta un hueso de la figura con los botones X/Y/Z ±5, guarda el par con un nombre y úsalo "
        "más tarde.",
        "Před přenosem musí zdroj a postava stát ve stejné póze ({{RtPoseMax}} v režimu 3ds Max, {{RtPoseBlender}} v režimu Blender) - jinak vyjdou paže a nohy "
        "zkroucené. Kostra JKA stojí v A-póze (paže asi 48° dolů), jiné rigy často v T-póze. **{{RtHaltungT}}** a **{{RtHaltungA}}** narovnají páteř a krk, paže "
        "do stran nebo 45° dolů a nohy svisle dolů, každou končetinu nejmenší rotací. Rollout ukazuje, jak moc se obě pózy ještě liší. Kost postavy doladíte "
        "tlačítky X/Y/Z ±5, dvojici uložíte pod názvem a použijete později."),
    "h_schritte": ("Pelvis, FK, IK, root", "Becken, FK, IK, Wurzel", "骨盆、FK、IK、根", "骨盤、FK、IK、ルート", "Pelvis, FK, IK, raíz", "Pánev, FK, IK, kořen"),
    "schritte": (
        "The steps run in the order of the list, as in Unreal: **{{RtSchrittBecken}}** moves the pelvis like the source, scaled by k = leg length of the figure / "
        "of the source - longer legs take longer steps and the feet do not slide; **{{RtSchrittFk}}** turns every bone like its source bone relative to the common "
        "pose, the lengths stay your figure's; **{{RtSchrittIk}}** pulls hands and feet onto the scaled places of the source (**{{RtReachFuss}}** and "
        "**{{RtReachHand}}** from 0 = FK only to 1 = exactly there, as Reach in HumanIK); **{{RtKontakte}}** keeps a foot where it touched down while the source foot "
        "stands; **{{RtSchrittWurzel}}** moves model_root and Motion like the source, scaled. Move a step with the arrows to try another order. Set k by hand for "
        "another stride.",
        "Die Schritte laufen in der Reihenfolge der Liste, wie in Unreal: **{{RtSchrittBecken}}** bewegt das Becken wie die Quelle, skaliert mit k = Beinlänge der "
        "Figur / der Quelle - längere Beine machen längere Schritte, und die Füße rutschen nicht; **{{RtSchrittFk}}** dreht jeden Knochen wie seinen Quellknochen "
        "gegenüber der gemeinsamen Pose, die Längen bleiben die deiner Figur; **{{RtSchrittIk}}** zieht Hände und Füße auf die skalierten Orte der Quelle "
        "(**{{RtReachFuss}}** und **{{RtReachHand}}** von 0 = nur FK bis 1 = genau dort, wie Reach in HumanIK); **{{RtKontakte}}** hält einen Fuß dort, wo er "
        "aufsetzte, solange der Fuß der Quelle steht; **{{RtSchrittWurzel}}** bewegt model_root und Motion wie die Quelle, skaliert. Mit den Pfeilen stellst du einen "
        "Schritt um. k von Hand ergibt eine andere Schrittweite.",
        "步骤按列表顺序执行, 与 Unreal 相同: **{{RtSchrittBecken}}** 像来源一样移动骨盆, 按 k = 角色腿长 / 来源腿长缩放 - 腿越长步子越大, 脚不打滑; "
        "**{{RtSchrittFk}}** 使每块骨骼相对共同姿势像其来源骨骼一样旋转, 长度保持角色自己的; **{{RtSchrittIk}}** 将手脚拉到来源缩放后的位置 (**{{RtReachFuss}}** "
        "和 **{{RtReachHand}}** 从 0 = 仅 FK 到 1 = 完全到达, 如 HumanIK 的 Reach); **{{RtKontakte}}** 在来源的脚站立时让脚停在落地处; **{{RtSchrittWurzel}}** 像来源"
        "一样移动 model_root 和 Motion 并缩放。用箭头调整步骤顺序。手动设定 k 可得到不同步幅。",
        "ステップはリストの順に、Unreal と同じように動きます: **{{RtSchrittBecken}}** は骨盤をソースと同様に動かし、k = フィギュアの脚の長さ / ソースの脚の長さで"
        "拡縮します - 長い脚は大きく歩き、足は滑りません。**{{RtSchrittFk}}** は各ボーンを共通ポーズに対してソースのボーンと同じように回し、長さはフィギュアのまま。"
        "**{{RtSchrittIk}}** は手足をソースの拡縮した位置へ引き寄せます (**{{RtReachFuss}}** と **{{RtReachHand}}** は 0 = FK のみから 1 = ぴったりまで、HumanIK の "
        "Reach と同じ)。**{{RtKontakte}}** はソースの足が立っている間、足を着地点に保ちます。**{{RtSchrittWurzel}}** は model_root と Motion をソースと同様に拡縮して"
        "動かします。矢印でステップの順を変えられます。k を手で決めれば歩幅が変わります。",
        "Los pasos se ejecutan en el orden de la lista, como en Unreal: **{{RtSchrittBecken}}** mueve la pelvis como el origen, escalada por k = longitud de pierna "
        "de la figura / del origen: piernas más largas dan pasos más largos y los pies no resbalan; **{{RtSchrittFk}}** gira cada hueso como su hueso de origen "
        "respecto a la pose común, las longitudes son las de tu figura; **{{RtSchrittIk}}** lleva manos y pies a los lugares escalados del origen "
        "(**{{RtReachFuss}}** y **{{RtReachHand}}** de 0 = solo FK a 1 = exactamente ahí, como Reach en HumanIK); **{{RtKontakte}}** mantiene un pie donde apoyó "
        "mientras el pie del origen está apoyado; **{{RtSchrittWurzel}}** mueve model_root y Motion como el origen, escalados. Con las flechas cambias el orden. "
        "Con k a mano obtienes otra zancada.",
        "Kroky běží v pořadí seznamu jako v Unrealu: **{{RtSchrittBecken}}** pohybuje pánví jako zdroj, škálovanou k = délka nohou postavy / zdroje - delší nohy "
        "dělají delší kroky a chodidla neklouzají; **{{RtSchrittFk}}** otáčí každou kost jako její zdrojovou kost vůči společné póze, délky zůstávají vaší postavy; "
        "**{{RtSchrittIk}}** přitáhne ruce a chodidla na škálovaná místa zdroje (**{{RtReachFuss}}** a **{{RtReachHand}}** od 0 = jen FK po 1 = přesně tam, jako "
        "Reach v HumanIK); **{{RtKontakte}}** drží chodidlo tam, kde došláplo, dokud chodidlo zdroje stojí; **{{RtSchrittWurzel}}** pohybuje model_root a Motion jako "
        "zdroj, škálovaně. Šipkami pořadí změníte. Ruční k dá jinou délku kroku."),
    "bild_ketten": ("Character definition (chains of figure and source), common pose and steps.",
                    "Charakter-Definition (Ketten von Figur und Quelle), gemeinsame Pose und Schritte.",
                    "角色定义 (角色与来源的链条)、共同姿势和步骤。", "キャラクター定義 (フィギュアとソースのチェーン)、共通ポーズ、ステップ。",
                    "Definición del personaje (cadenas de figura y origen), pose común y pasos.", "Definice postavy (řetězy postavy a zdroje), společná póza a kroky."),
    "h_pruefen": ("Check and bake", "Prüfen und backen", "检查与烘焙", "確認とベイク", "Comprobar y hornear", "Kontrola a zapečení"),
    "pruefen": (
        "Under the steps the rollout measures the result: **foot sliding** is the largest horizontal way of a foot while it stands, in game units, next to the "
        "same value of the source (scaled); the bars show it per frame. **Jumps** are frames in which a bone of the figure turns or moves much more than its "
        "source - click a frame number to see it. **{{RtBackenMax}}** (Blender mode: **{{RtBackenBlender}}**) packs the frames like the game with the fps of the "
        "source's animation.cfg and puts them on a **{{RtPlatz}}** (BOTH_WALK1, BOTH_CIN_3 ...). In **GLA and slots** you then build the map GLA (cutscenes, "
        "BOTH_CIN_1...), your own mod's _humanoid.gla or your own GLA - an own GLA with a new skeleton loads in single player only. The figure's own animation and "
        "its baked GLA stay byte for byte as before when you switch the preview off.",
        "Unter den Schritten misst das Rollout das Ergebnis: **Fußrutschen** ist der größte waagrechte Weg eines Fußes, während er steht, in Einheiten des Spiels, "
        "neben demselben Wert der Quelle (skaliert); die Balken zeigen ihn je Frame. **Sprünge** sind Frames, in denen sich ein Knochen der Figur viel mehr dreht "
        "oder bewegt als seine Quelle - ein Klick auf die Nummer zeigt den Frame. **{{RtBackenMax}}** (im Blender-Modus **{{RtBackenBlender}}**) packt die Frames wie "
        "das Spiel mit den fps der animation.cfg der Quelle und legt sie auf einen **{{RtPlatz}}** (BOTH_WALK1, BOTH_CIN_3 ...). Unter **GLA und Plätze** baust du "
        "dann die Karten-GLA (Zwischensequenzen, BOTH_CIN_1...), die _humanoid.gla deines Mods oder eine eigene GLA - eine eigene GLA mit neuem Skelett lädt nur der "
        "Einzelspieler. Die eigene Animation der Figur und ihre gebackene GLA bleiben Byte für Byte wie vorher, wenn du die Vorschau ausschaltest.",
        "卷展栏在步骤下方测量结果: **脚打滑** 是脚站立期间最大的水平移动 (游戏单位), 旁边是来源的同一数值 (缩放后); 柱状图逐帧显示。**跳变** 是角色骨骼旋转或"
        "移动远大于其来源的帧 - 点击帧号即可查看。**{{RtBackenMax}}** (Blender 模式为 **{{RtBackenBlender}}**) 按游戏方式以来源 animation.cfg 的 fps 压缩各帧, 放到"
        "一个 **{{RtPlatz}}** (BOTH_WALK1、BOTH_CIN_3 ...)。然后在 **GLA 与槽位** 中生成地图 GLA (过场, BOTH_CIN_1...)、你的模组的 _humanoid.gla 或自己的 GLA - 带新"
        "骨架的自有 GLA 仅单人模式加载。关闭预览后, 角色自己的动画及其烘焙 GLA 与之前逐字节相同。",
        "ステップの下でロールアウトが結果を測ります: **足の滑り** は足が立っている間の最大の水平移動 (ゲーム単位) で、ソースの同じ値 (拡縮後) と並べて表示し、棒が"
        "フレームごとに示します。**ジャンプ** はフィギュアのボーンがソースよりずっと大きく回ったり動いたりするフレームで、番号をクリックすると表示します。"
        "**{{RtBackenMax}}** (Blender 風は **{{RtBackenBlender}}**) はフレームをゲームと同じくソースの animation.cfg の fps で圧縮し、**{{RtPlatz}}** (BOTH_WALK1、"
        "BOTH_CIN_3 ...) に置きます。その後 **GLA とスロット** でマップ GLA (カットシーン、BOTH_CIN_1...)、MOD の _humanoid.gla、または独自 GLA をビルドします - 新しい"
        "スケルトンの独自 GLA はシングルプレイでのみ読み込まれます。プレビューをオフにすれば、フィギュア自身のアニメーションとベイクした GLA はバイト単位で元どおりです。",
        "Bajo los pasos el panel mide el resultado: el **deslizamiento de pies** es el mayor recorrido horizontal de un pie mientras está apoyado, en unidades del "
        "juego, junto al mismo valor del origen (escalado); las barras lo muestran por fotograma. Los **saltos** son fotogramas en los que un hueso de la figura gira "
        "o se mueve mucho más que su origen: haz clic en el número para verlo. **{{RtBackenMax}}** (en modo Blender **{{RtBackenBlender}}**) empaqueta los "
        "fotogramas como el juego con los fps del animation.cfg del origen y los pone en un **{{RtPlatz}}** (BOTH_WALK1, BOTH_CIN_3 ...). En **GLA y huecos** creas "
        "luego el GLA de mapa (cinemáticas, BOTH_CIN_1...), el _humanoid.gla de tu mod o un GLA propio; un GLA propio con esqueleto nuevo solo lo carga el modo de "
        "un jugador. La animación propia de la figura y su GLA horneado quedan byte a byte como antes al desactivar la vista previa.",
        "Pod kroky rollout měří výsledek: **klouzání chodidel** je největší vodorovná dráha chodidla, zatímco stojí, v jednotkách hry, vedle stejné hodnoty zdroje "
        "(škálovaně); sloupce ji ukazují po snímcích. **Skoky** jsou snímky, v nichž se kost postavy otáčí nebo pohybuje o moc víc než její zdroj - klik na číslo "
        "snímek ukáže. **{{RtBackenMax}}** (v režimu Blender **{{RtBackenBlender}}**) zabalí snímky jako hra s fps z animation.cfg zdroje a položí je do pozice "
        "**{{RtPlatz}}** (BOTH_WALK1, BOTH_CIN_3 ...). V **GLA a pozice** pak sestavíte mapové GLA (cutscény, BOTH_CIN_1...), _humanoid.gla svého modu nebo vlastní "
        "GLA - vlastní GLA s novou kostrou načte jen hra pro jednoho hráče. Vlastní animace postavy a její zapečené GLA zůstanou po vypnutí náhledu bajt po bajtu "
        "jako předtím."),
    "h_presets": ("Presets: walk and run generator", "Presets: Geh- und Laufgenerator", "预设: 步行与跑步生成器", "プリセット: 歩きと走りのジェネレーター",
                  "Presets: generador de andar y correr", "Předvolby: generátor chůze a běhu"),
    "presets": (
        "The group **{{PrRollout}}** (also in the menu **Animation > {{PrRollout}}** and in the search) makes walk and run cycles with our own generator, so there "
        "is no licence question: in the categories {{AzKatGehen}} and {{AzKatLaufen}} choose **{{EnGangGehen}}** or **{{EnGangLaufen}}** in the list. **{{PrTempo}}** (the "
        "NPCs of the game walk with 90 and run with 300 units per second) and **{{PrArm}}** stand under the list; **{{AzGenerator}}** shows the rest: **{{PrHaltung}}** "
        "({{PrHaltNormal}}, {{PrHaltSchwer}}, {{PrHaltSchleichend}}, {{PrHaltVerletzt}}, {{PrHaltSaebel}}), **{{PrSchritt}}** - measured from the figure's own BOTH_WALK1 / "
        "BOTH_RUN1 - hip swing and foot lift. **{{AzPresetVorschau}}** plays the cycle at once on the figure; the standing foot lies exactly on the ground and glides "
        "evenly backwards like the walks of the game (the game moves the figure), the cycle joins without a seam, the pelvis sinks where both feet have to reach the "
        "ground. **{{AzPresetAuflegen}}** bakes it to the slot BOTH_WALK1 or BOTH_RUN1 (or another target slot) like a retargeted animation. The free animations of "
        "other authors stand in the same list (see Import animation).",
        "Die Gruppe **{{PrRollout}}** (auch im Menü **Animation > {{PrRollout}}** und in der Suche) erzeugt Geh- und Laufzyklen mit unserem eigenen Generator, deshalb "
        "gibt es keine Lizenzfrage: in den Kategorien {{AzKatGehen}} und {{AzKatLaufen}} **{{EnGangGehen}}** bzw. **{{EnGangLaufen}}** in der Liste wählen. **{{PrTempo}}** "
        "(die NPCs des Spiels gehen mit 90 und laufen mit 300 Einheiten je Sekunde) und **{{PrArm}}** stehen unter der Liste; **{{AzGenerator}}** zeigt den Rest: "
        "**{{PrHaltung}}** ({{PrHaltNormal}}, {{PrHaltSchwer}}, {{PrHaltSchleichend}}, {{PrHaltVerletzt}}, {{PrHaltSaebel}}), **{{PrSchritt}}** - gemessen am eigenen "
        "BOTH_WALK1 / BOTH_RUN1 der Figur - sowie Hüftschwung und Fußhub. **{{AzPresetVorschau}}** spielt den Zyklus sofort auf der Figur; der Standfuß liegt genau auf "
        "dem Boden und gleitet gleichmäßig nach hinten wie bei den Gängen des Spiels (die Figur bewegt das Spiel), der Zyklus schließt ohne Naht, das Becken senkt sich "
        "dort, wo beide Füße den Boden erreichen müssen. **{{AzPresetAuflegen}}** backt ihn wie eine übertragene Animation auf den Platz BOTH_WALK1 oder BOTH_RUN1 (oder "
        "einen anderen Ziel-Platz). Die freien Animationen anderer Urheber stehen in derselben Liste (siehe Animation importieren).",
        "**{{PrRollout}}** 组 (也在菜单 **Animation > {{PrRollout}}** 和搜索中) 用我们自己的生成器制作步行和跑步循环, 因此没有许可问题: 在类别 {{AzKatGehen}} 和 {{AzKatLaufen}} "
        "中选择列表里的 **{{EnGangGehen}}** 或 **{{EnGangLaufen}}**。**{{PrTempo}}** (游戏 NPC 步行 90、跑步 300 单位/秒) 和 **{{PrArm}}** 位于列表下方; **{{AzGenerator}}** 显示其余设置: "
        "**{{PrHaltung}}** ({{PrHaltNormal}}、{{PrHaltSchwer}}、{{PrHaltSchleichend}}、{{PrHaltVerletzt}}、{{PrHaltSaebel}})、**{{PrSchritt}}** (从角色自己的 BOTH_WALK1 / BOTH_RUN1 测得) "
        "以及髋部摆动和抬脚。**{{AzPresetVorschau}}** 立即在角色上播放循环; 支撑脚精确贴地, 并像游戏中的步行一样匀速向后滑动 (由游戏移动角色), 循环无缝衔接, 在两脚都需着地处骨盆下沉。"
        "**{{AzPresetAuflegen}}** 像重定向的动画一样把它烘焙到 BOTH_WALK1 或 BOTH_RUN1 槽位 (或其他目标槽位)。其他作者的免费动画位于同一列表中 (见“导入动画”)。",
        "グループ **{{PrRollout}}** (メニュー **Animation > {{PrRollout}}** と検索にも) は独自のジェネレーターで歩きと走りのサイクルを作るので、ライセンスの問題はありません: "
        "カテゴリ {{AzKatGehen}} と {{AzKatLaufen}} で一覧の **{{EnGangGehen}}** または **{{EnGangLaufen}}** を選びます。**{{PrTempo}}** (ゲームの NPC は歩き 90、走り 300 ユニット/秒) "
        "と **{{PrArm}}** は一覧の下にあり、**{{AzGenerator}}** でその他を表示します: **{{PrHaltung}}** ({{PrHaltNormal}}、{{PrHaltSchwer}}、{{PrHaltSchleichend}}、{{PrHaltVerletzt}}、"
        "{{PrHaltSaebel}})、フィギュア自身の BOTH_WALK1 / BOTH_RUN1 から測った **{{PrSchritt}}**、腰の振り、足の上げ。**{{AzPresetVorschau}}** でサイクルがすぐフィギュアで再生されます。"
        "立ち足は正確に地面にあり、ゲームの歩きと同じく等速で後ろへ滑ります (フィギュアを動かすのはゲーム)。継ぎ目なくつながり、両足が地面に届く必要がある所で骨盤が下がります。"
        "**{{AzPresetAuflegen}}** はリターゲットしたアニメーションと同じく BOTH_WALK1 または BOTH_RUN1 (または別の出力先スロット) にベイクします。他の作者の無料アニメーションも同じ一覧にあります (「アニメーションをインポート」を参照)。",
        "El grupo **{{PrRollout}}** (también en el menú **Animation > {{PrRollout}}** y en la búsqueda) crea ciclos de andar y correr con nuestro propio generador, así "
        "que no hay cuestión de licencia: en las categorías {{AzKatGehen}} y {{AzKatLaufen}} elige **{{EnGangGehen}}** o **{{EnGangLaufen}}** en la lista. **{{PrTempo}}** "
        "(los NPC del juego andan a 90 y corren a 300 unidades por segundo) y **{{PrArm}}** están bajo la lista; **{{AzGenerator}}** muestra el resto: **{{PrHaltung}}** "
        "({{PrHaltNormal}}, {{PrHaltSchwer}}, {{PrHaltSchleichend}}, {{PrHaltVerletzt}}, {{PrHaltSaebel}}), **{{PrSchritt}}** (medida en el BOTH_WALK1 / BOTH_RUN1 propio de la "
        "figura), balanceo de cadera y altura del pie. **{{AzPresetVorschau}}** reproduce el ciclo al instante en la figura; el pie de apoyo queda justo en el suelo y se "
        "desliza uniformemente hacia atrás como los pasos del juego (el juego mueve la figura), el ciclo cierra sin costura y la pelvis baja donde ambos pies deben llegar al "
        "suelo. **{{AzPresetAuflegen}}** lo hornea como una animación transferida en el hueco BOTH_WALK1 o BOTH_RUN1 (u otro hueco de destino). Las animaciones libres de "
        "otros autores están en la misma lista (ver Importar animación).",
        "Skupina **{{PrRollout}}** (také v nabídce **Animation > {{PrRollout}}** a ve vyhledávání) vytváří cykly chůze a běhu naším vlastním generátorem, takže neexistuje "
        "licenční otázka: v kategoriích {{AzKatGehen}} a {{AzKatLaufen}} zvolte v seznamu **{{EnGangGehen}}** nebo **{{EnGangLaufen}}**. **{{PrTempo}}** (NPC hry chodí "
        "rychlostí 90 a běhají 300 jednotek za sekundu) a **{{PrArm}}** jsou pod seznamem; **{{AzGenerator}}** ukáže zbytek: **{{PrHaltung}}** ({{PrHaltNormal}}, "
        "{{PrHaltSchwer}}, {{PrHaltSchleichend}}, {{PrHaltVerletzt}}, {{PrHaltSaebel}}), **{{PrSchritt}}** - změřenou na vlastních BOTH_WALK1 / BOTH_RUN1 postavy - a švih boků "
        "a zvednutí chodidla. **{{AzPresetVorschau}}** hned přehraje cyklus na postavě; stojné chodidlo leží přesně na zemi a rovnoměrně klouže dozadu jako chůze hry "
        "(postavu posouvá hra), cyklus navazuje bez švu a pánev klesne tam, kde musí obě chodidla dosáhnout země. **{{AzPresetAuflegen}}** ho zapeče jako přenesenou "
        "animaci do pozice BOTH_WALK1 nebo BOTH_RUN1 (nebo jiné cílové pozice). Volné animace jiných autorů jsou ve stejném seznamu (viz Import animace)."),
    "bild_presets": ("A walk from the generator on the figure (posture saber drawn), rollout Presets with its check.",
                     "Ein Gang aus dem Generator auf der Figur (Haltung Säbel gezogen), Rollout Presets mit Befund.",
                     "生成器生成的步行在角色上 (姿态: 拔出光剑), 预设卷展栏及检查结果。", "ジェネレーターの歩きをフィギュアに (姿勢: セーバーを抜く)、検査結果付きのロールアウト「プリセット」。",
                     "Un paso del generador en la figura (postura sable desenvainado), panel Presets con su comprobación.",
                     "Chůze z generátoru na postavě (držení: tasený meč), rollout Předvolby s kontrolou."),
}


def _t(schluessel):
    from inhalt_basis import T
    z = X[schluessel]
    return T(z[0], z[1], z[2], z[3])


def seite():
    """Die Seite modell-retarget (Gruppe Missionen, nach modell-physik)."""
    return {
        "id": "modell-retarget",
        "gruppe": "mission",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("h2", "quelle", _t("h_quelle")),
            ("p", _t("quelle")),
            ("img", "h_rt_uebertragen", _t("bild_uebertragen"), "breit"),
            ("h2", "ketten", _t("h_ketten")),
            ("p", _t("ketten")),
            ("h2", "pose", _t("h_pose")),
            ("p", _t("pose")),
            ("h2", "schritte", _t("h_schritte")),
            ("p", _t("schritte")),
            ("img", "h_rt_ketten", _t("bild_ketten"), "breit"),
            ("h2", "pruefen", _t("h_pruefen")),
            ("p", _t("pruefen")),
            ("h2", "presets", _t("h_presets")),
            ("p", _t("presets")),
            ("img", "h_pr_gang", _t("bild_presets"), "breit"),
        ],
    }


ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}
