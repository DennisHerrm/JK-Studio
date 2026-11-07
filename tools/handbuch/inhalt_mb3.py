"""Modell-Block Stufe 3: die Seite "Model-Tab: Control Rig, Posen, Ghosts und Pfade" (Knochen-Erkennung, Rig mit einem
Klick, Uebersicht zum Anklicken, FK-Griffe wie 3ds Max und Blender, Posen und Handposen, Ghosts, Silhouette,
Bewegungspfade, Backen in die GLA).

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_mb2.py.
"""

X = {
    "titel": ("Model tab: control rig, poses, ghosts and paths", "Model-Tab: Control Rig, Posen, Ghosts und Pfade", "模型选项卡: 控制绑定、姿势、残影和路径",
              "モデルタブ: コントロールリグ、ポーズ、ゴーストとパス", "Pestaña Model: rig de control, poses, fantasmas y trayectorias",
              "Karta Model: ovládací rig, pózy, duchové a dráhy"),
    "kurz": (
        "Recognise the bones, create a control rig with one click, pose with FK, copy and mirror poses, see ghosts and motion paths and bake the result into a GLA.",
        "Die Knochen erkennen, mit einem Klick ein Control Rig anlegen, mit FK posieren, Posen kopieren und spiegeln, Ghosts und Bewegungspfade sehen und das Ergebnis in eine GLA backen.",
        "识别骨骼, 一键创建控制绑定, 用 FK 摆姿势, 复制和镜像姿势, 查看残影和运动路径, 并把结果烘焙到 GLA。",
        "ボーンを認識し、ワンクリックでコントロールリグを作り、FK でポーズを付け、ポーズをコピー・反転し、ゴーストとモーションパスを見て、結果を GLA にベイクします。",
        "Reconoce los huesos, crea un rig de control con un clic, posa con FK, copia y refleja poses, ve fantasmas y trayectorias y hornea el resultado en un GLA.",
        "Rozpoznejte kosti, jedním kliknutím vytvořte ovládací rig, pózujte s FK, kopírujte a zrcadlete pózy, sledujte duchy a dráhy pohybu a výsledek zapečte do GLA."),
    "h_rig": ("The rig with one click", "Das Rig mit einem Klick", "一键创建绑定", "ワンクリックでリグ", "El rig con un clic", "Rig jedním kliknutím"),
    "rig": (
        "In the area **{{MbBereichAnimation}}**, **{{MbRgAnlegen}}** (also in the menu **{{MbBereichAnimation}}**, in the right-click of the view and in "
        "the search X/F3) recognises every bone: its role (pelvis, spine, neck, head, clavicle, upper arm, forearm, hand, fingers, thigh, calf, foot, toes, "
        "twist bones, tag bones, face) and its side. The name gives a first answer (pelvis, lhumerus, r_d1_j1, L_thigh, leg_left_bone01 ...); the hierarchy "
        "and the bind pose check it - legs hang where the limbs reach lowest, the parts of a limb follow each other outwards, the spine rises from the pelvis "
        "to the head, the side comes from the side axis of the skeleton. Where name and position disagree, the position wins and the bone is marked unclear. "
        "It works for the skeletons of Jedi Academy (53 bones), Jedi Outcast (72, everything flat under pelvis and thoracic) and the creatures and droids of "
        "Movie Duels. The rig then has controls for every part of the body, pole targets for elbows and knees, the foot roll (heel, ball, toe tip) and the "
        "saber grip at *r_hand; pole targets and foot roll work with IK (page IK/FK).",
        "Im Bereich **{{MbBereichAnimation}}** erkennt **{{MbRgAnlegen}}** (auch im Menü **{{MbBereichAnimation}}**, im Rechtsklick der Ansicht und in der "
        "Suche X/F3) jeden Knochen: seine Rolle (Becken, Wirbel, Hals, Kopf, Schlüsselbein, Ober- und Unterarm, Hand, Finger, Ober- und Unterschenkel, Fuß, "
        "Zehen, Drehknochen, Tag-Knochen, Gesicht) und seine Seite. Der Name gibt eine erste Antwort (pelvis, lhumerus, r_d1_j1, L_thigh, leg_left_bone01 ...); "
        "Hierarchie und Bindepose prüfen sie - Beine hängen dort, wo die Glieder am tiefsten reichen, die Teile eines Glieds folgen nach außen aufeinander, "
        "die Wirbelsäule steigt vom Becken zum Kopf, die Seite kommt von der Seitenachse des Skeletts. Wo Name und Lage sich widersprechen, gilt die Lage, "
        "und der Knochen ist als unklar markiert. Das geht für die Skelette von Jedi Academy (53 Knochen), Jedi Outcast (72, alles flach unter pelvis und "
        "thoracic) und die Kreaturen und Droiden von Movie Duels. Das Rig hat dann Griffe für jeden Teil des Körpers, Pol-Ziele für Ellbogen und Knie, die "
        "Fußrolle (Ferse, Ballen, Zehenspitze) und den Säbelgriff an *r_hand; Pol-Ziele und Fußrolle wirken mit IK (Seite IK/FK).",
        "在 **{{MbBereichAnimation}}** 区域, **{{MbRgAnlegen}}** (也在菜单 **{{MbBereichAnimation}}**、视图右键和搜索 X/F3 中) 识别每根骨骼: "
        "它的角色 (骨盆、脊柱、颈、头、锁骨、上臂、前臂、手、手指、大腿、小腿、脚、脚趾、扭转骨骼、标签骨骼、面部) 和它的侧别。名称给出第一个答案 "
        "(pelvis、lhumerus、r_d1_j1、L_thigh、leg_left_bone01 ...); 层次和绑定姿势进行检查——腿挂在肢体伸得最低的地方, 肢体的各部分向外依次相连, "
        "脊柱从骨盆升到头, 侧别来自骨架的侧轴。名称与位置矛盾时以位置为准, 并把骨骼标记为不确定。这适用于 Jedi Academy (53 根骨骼)、"
        "Jedi Outcast (72 根, 全部平铺在 pelvis 和 thoracic 下) 以及 Movie Duels 的生物和机器人的骨架。绑定随后为身体每个部分提供控制器、"
        "肘和膝的极向目标、脚部滚动 (脚跟、脚掌、脚尖) 以及 *r_hand 处的光剑握把; 极向目标和脚部滚动随 IK 生效 (见 IK/FK 页)。",
        "**{{MbBereichAnimation}}** 領域で **{{MbRgAnlegen}}** (メニュー **{{MbBereichAnimation}}**、ビューの右クリック、検索 X/F3 にもあります) が"
        "すべてのボーンを認識します: 役割 (骨盤、背骨、首、頭、鎖骨、上腕、前腕、手、指、太もも、すね、足、つま先、ツイストボーン、タグボーン、顔) と左右。"
        "名前が最初の答えを出し (pelvis、lhumerus、r_d1_j1、L_thigh、leg_left_bone01 ...)、階層とバインドポーズがそれを確かめます - 脚は肢が最も"
        "低く届く所に付き、肢の各部は外側へ順に続き、背骨は骨盤から頭へ上り、左右はスケルトンの左右軸から決まります。名前と位置が食い違う所では位置を"
        "優先し、ボーンを不明として示します。Jedi Academy (ボーン 53)、Jedi Outcast (72、すべて pelvis と thoracic の下に平らに並ぶ)、Movie Duels "
        "のクリーチャーとドロイドのスケルトンで動きます。リグは体の各部のコントロール、肘と膝のポールターゲット、フットロール (かかと、母指球、つま先の先)、"
        "*r_hand のセーバーグリップを持ちます。ポールターゲットとフットロールは IK で働きます (IK/FK のページ)。",
        "En el área **{{MbBereichAnimation}}**, **{{MbRgAnlegen}}** (también en el menú **{{MbBereichAnimation}}**, en el clic derecho de la vista y en la "
        "búsqueda X/F3) reconoce cada hueso: su función (pelvis, columna, cuello, cabeza, clavícula, brazo, antebrazo, mano, dedos, muslo, pierna, pie, dedos "
        "del pie, huesos de giro, huesos de tag, cara) y su lado. El nombre da una primera respuesta (pelvis, lhumerus, r_d1_j1, L_thigh, leg_left_bone01 ...); "
        "la jerarquía y la pose de unión la comprueban: las piernas cuelgan donde las extremidades llegan más abajo, las partes de una extremidad se suceden "
        "hacia fuera, la columna sube de la pelvis a la cabeza y el lado sale del eje lateral del esqueleto. Donde nombre y posición no coinciden, manda la "
        "posición y el hueso se marca como dudoso. Funciona con los esqueletos de Jedi Academy (53 huesos), Jedi Outcast (72, todo plano bajo pelvis y "
        "thoracic) y las criaturas y droides de Movie Duels. El rig tiene entonces controles para cada parte del cuerpo, objetivos de polo para codos y "
        "rodillas, el balanceo del pie (talón, planta, punta) y el agarre del sable en *r_hand; los polos y el balanceo funcionan con IK (página IK/FK).",
        "V oblasti **{{MbBereichAnimation}}** rozpozná **{{MbRgAnlegen}}** (také v nabídce **{{MbBereichAnimation}}**, v pravém tlačítku pohledu a ve "
        "vyhledávání X/F3) každou kost: její roli (pánev, páteř, krk, hlava, klíční kost, nadloktí, předloktí, ruka, prsty, stehno, lýtko, chodidlo, prsty "
        "nohy, kroutící kosti, tagové kosti, obličej) a stranu. Jméno dá první odpověď (pelvis, lhumerus, r_d1_j1, L_thigh, leg_left_bone01 ...); hierarchie "
        "a vazná póza ji ověří - nohy visí tam, kam končetiny sahají nejníž, části končetiny jdou ven za sebou, páteř stoupá od pánve k hlavě a strana plyne "
        "z boční osy kostry. Kde si jméno a poloha odporují, platí poloha a kost je označena jako nejasná. Funguje to pro kostry Jedi Academy (53 kostí), "
        "Jedi Outcast (72, vše plošně pod pelvis a thoracic) a pro tvory a droidy Movie Duels. Rig má pak ovladače pro každou část těla, cíle pólů loktů "
        "a kolen, odval chodidla (pata, bříško, špička) a úchop meče na *r_hand; póly a odval působí s IK (stránka IK/FK)."),
    "h_uebersicht": ("The overview: assign by clicking", "Die Übersicht: zuordnen per Klick", "概览: 点击指定", "概要: クリックで割り当て",
                     "La vista general: asignar con clics", "Přehled: přiřazení kliknutím"),
    "uebersicht": (
        "**{{MbRgUebersicht}}** shows a figure: green spots are recognised, yellow ones unclear, grey ones empty; a number says how many bones share the "
        "role (fingers, spine). Hover a spot for its bones and the reason when it is unclear. Click a spot, then a bone in **{{MbRgUnklarListe}}** or in "
        "**{{MbRgZuordnen}}** - the bone takes that role. The assignment is part of the model document (Ctrl+Z) and the rig is rebuilt at once.",
        "**{{MbRgUebersicht}}** zeigt eine Figur: grüne Flecken sind erkannt, gelbe unklar, graue leer; eine Zahl sagt, wie viele Knochen sich die Rolle "
        "teilen (Finger, Wirbel). Über einem Fleck stehen seine Knochen und, wenn er unklar ist, der Grund. Einen Fleck anklicken, dann einen Knochen in "
        "**{{MbRgUnklarListe}}** oder in **{{MbRgZuordnen}}** - der Knochen bekommt diese Rolle. Die Zuordnung gehört zum Modelldokument (Strg+Z), und das "
        "Rig wird sofort neu angelegt.",
        "**{{MbRgUebersicht}}** 显示一个人形: 绿点已识别, 黄点不确定, 灰点为空; 数字表示有多少骨骼共享此角色 (手指、脊柱)。悬停在点上可看到它的骨骼, "
        "不确定时还有原因。点击一个点, 再点击 **{{MbRgUnklarListe}}** 或 **{{MbRgZuordnen}}** 中的骨骼——该骨骼获得此角色。指定属于模型文档 (Ctrl+Z), "
        "绑定会立即重建。",
        "**{{MbRgUebersicht}}** は人体図を表示します: 緑の点は認識済み、黄は不明、灰は空。数字はその役割を共有するボーンの数です (指、背骨)。点に"
        "カーソルを置くとボーンと、不明ならその理由が出ます。点をクリックしてから **{{MbRgUnklarListe}}** または **{{MbRgZuordnen}}** のボーンを"
        "クリックすると、そのボーンがその役割になります。割り当てはモデルドキュメントの一部で (Ctrl+Z)、リグはすぐに作り直されます。",
        "**{{MbRgUebersicht}}** muestra una figura: los puntos verdes están reconocidos, los amarillos son dudosos y los grises están vacíos; un número dice "
        "cuántos huesos comparten la función (dedos, columna). Al pasar sobre un punto se ven sus huesos y, si es dudoso, el motivo. Haz clic en un punto y "
        "luego en un hueso de **{{MbRgUnklarListe}}** o de **{{MbRgZuordnen}}**: el hueso recibe esa función. La asignación forma parte del documento del "
        "modelo (Ctrl+Z) y el rig se rehace al momento.",
        "**{{MbRgUebersicht}}** ukazuje postavu: zelené body jsou rozpoznané, žluté nejasné, šedé prázdné; číslo říká, kolik kostí roli sdílí (prsty, "
        "páteř). Nad bodem se ukážou jeho kosti a u nejasného i důvod. Klikněte na bod a pak na kost v **{{MbRgUnklarListe}}** nebo v **{{MbRgZuordnen}}** "
        "- kost dostane tuto roli. Přiřazení patří do dokumentu modelu (Ctrl+Z) a rig se hned znovu vytvoří."),
    "bild_uebersicht": (
        "The rig overview of the stormtrooper: every role recognised, the spot Hand R selected with its bone rhand.",
        "Die Rig-Übersicht des Sturmtrupplers: jede Rolle erkannt, der Fleck Hand R gewählt mit seinem Knochen rhand.",
        "冲锋队员的绑定概览: 每个角色都已识别, 已选择点 Hand R 及其骨骼 rhand。",
        "ストームトルーパーのリグ概要: すべての役割を認識、点 Hand R とそのボーン rhand を選択。",
        "La vista general del rig del soldado de asalto: cada función reconocida, el punto Mano D seleccionado con su hueso rhand.",
        "Přehled rigu stormtroopera: každá role rozpoznána, vybrán bod Ruka P s kostí rhand."),
    "h_posieren": ("Posing: FK like 3ds Max and like Blender", "Posieren: FK wie 3ds Max und wie Blender", "摆姿势: 像 3ds Max 和像 Blender 的 FK",
                   "ポーズ付け: 3ds Max 風と Blender 風の FK", "Posar: FK como 3ds Max y como Blender", "Pózování: FK jako 3ds Max a jako Blender"),
    "posieren": (
        "The controls stand in the scene explorer under **Control rig** (own symbols, the eye hides one in the view) and in the views - like 3ds Max as "
        "boxes, like Blender as rings, left blue, right green, middle yellow. Click one in the view or in the explorer. Like 3ds Max, E shows the rotation "
        "gizmo and W moves the pelvis; like Blender, R rotates (X/Y/Z for an axis, click or Enter to confirm, Esc to cancel) and G moves the pelvis. Every "
        "rotation is FK the way the game turns a head or a torso (G2API_SetBoneAngles), and a key at the current frame of the selected sequence; between "
        "two keys the pose is interpolated. Bones that belong to the hand but do not hang below it in the skeleton (the fingers of Jedi Academy under "
        "rradius, the whole arm of Jedi Outcast) turn with it, and the twist bone (rradiusX) takes the share of the twist that matches its place on the "
        "forearm. Keys do not rebuild the mesh; Ctrl+Z takes each one back.",
        "Die Griffe stehen im Szenen-Explorer unter **Control Rig** (eigene Symbole, das Auge blendet einen in der Ansicht aus) und in den Ansichten - wie "
        "3ds Max als Kästen, wie Blender als Ringe, links blau, rechts grün, Mitte gelb. Einen in der Ansicht oder im Explorer anklicken. Wie 3ds Max zeigt "
        "E das Dreh-Gizmo und W verschiebt das Becken; wie Blender dreht R (X/Y/Z für eine Achse, Klick oder Enter bestätigt, Esc bricht ab), und G "
        "verschiebt das Becken. Jede Drehung ist FK, so wie das Spiel einen Kopf oder Rumpf dreht (G2API_SetBoneAngles), und ein Schlüssel am aktuellen "
        "Frame der gewählten Sequenz; zwischen zwei Schlüsseln wird die Pose übergeblendet. Knochen, die zur Hand gehören, aber im Skelett nicht unter ihr "
        "hängen (die Finger von Jedi Academy unter rradius, der ganze Arm von Jedi Outcast), drehen mit, und der Drehknochen (rradiusX) nimmt den Anteil "
        "der Drehung, der seiner Lage am Unterarm entspricht. Schlüssel bauen das Netz nicht neu; Strg+Z nimmt jeden zurück.",
        "控制器位于场景资源管理器的 **控制绑定** 下 (有自己的符号, 眼睛可在视图中隐藏) 以及视图中——像 3ds Max 时为方框, 像 Blender 时为圆环, "
        "左蓝、右绿、中黄。在视图或资源管理器中点击一个。像 3ds Max 时 E 显示旋转小部件, W 移动骨盆; 像 Blender 时 R 旋转 (X/Y/Z 选轴, 点击或 Enter "
        "确认, Esc 取消), G 移动骨盆。每次旋转都是 FK, 与游戏转动头部或躯干的方式相同 (G2API_SetBoneAngles), 并在所选序列的当前帧生成关键帧; "
        "两个关键帧之间的姿势会插值。属于手但在骨架中不在手下面的骨骼 (Jedi Academy 中 rradius 下的手指, Jedi Outcast 的整条手臂) 会一起转动, "
        "扭转骨骼 (rradiusX) 按其在前臂上的位置承担相应比例的扭转。关键帧不会重建网格; Ctrl+Z 可逐个撤销。",
        "コントロールはシーンエクスプローラーの **コントロールリグ** の下 (専用の記号、目でビューから隠せます) とビューにあります - 3ds Max 風では箱、"
        "Blender 風では輪で、左は青、右は緑、中央は黄。ビューかエクスプローラーでクリックします。3ds Max 風では E で回転ギズモ、W で骨盤を移動。"
        "Blender 風では R で回転 (X/Y/Z で軸、クリックか Enter で確定、Esc で取り消し)、G で骨盤を移動。回転はすべて、ゲームが頭や胴を回すのと同じ "
        "FK (G2API_SetBoneAngles) で、選択中のシーケンスの現在フレームのキーになります。2 つのキーの間のポーズは補間されます。手に属するがスケルトン上は"
        "手の下にないボーン (Jedi Academy の rradius の下の指、Jedi Outcast の腕全体) は一緒に回り、ツイストボーン (rradiusX) は前腕上の位置に"
        "応じた割合のねじれを受け持ちます。キーはメッシュを作り直しません。Ctrl+Z で 1 つずつ戻せます。",
        "Los controles están en el explorador de escena bajo **Rig de control** (símbolos propios, el ojo oculta uno en la vista) y en las vistas: como en "
        "3ds Max son cajas, como en Blender anillos; izquierda azul, derecha verde, centro amarillo. Haz clic en uno en la vista o en el explorador. Como en "
        "3ds Max, E muestra el gizmo de giro y W mueve la pelvis; como en Blender, R gira (X/Y/Z para un eje, clic o Intro confirma, Esc cancela) y G mueve "
        "la pelvis. Cada giro es FK, como el juego gira una cabeza o un torso (G2API_SetBoneAngles), y una clave en el fotograma actual de la secuencia "
        "elegida; entre dos claves la pose se interpola. Los huesos que pertenecen a la mano pero no cuelgan bajo ella en el esqueleto (los dedos de Jedi "
        "Academy bajo rradius, todo el brazo de Jedi Outcast) giran con ella, y el hueso de giro (rradiusX) toma la parte del giro que corresponde a su lugar "
        "en el antebrazo. Las claves no rehacen la malla; Ctrl+Z deshace cada una.",
        "Ovladače jsou v průzkumníku scény pod **Ovládací rig** (vlastní symboly, oko jeden skryje v pohledu) a v pohledech - jako v 3ds Max kvádry, jako v "
        "Blenderu kroužky, vlevo modrá, vpravo zelená, uprostřed žlutá. Klikněte na jeden v pohledu nebo v průzkumníku. Jako v 3ds Max ukáže E otáčecí "
        "gizmo a W posouvá pánev; jako v Blenderu otáčí R (X/Y/Z pro osu, klik nebo Enter potvrdí, Esc zruší) a G posouvá pánev. Každé otočení je FK, "
        "jak hra otáčí hlavou nebo trupem (G2API_SetBoneAngles), a klíč na aktuálním snímku zvolené sekvence; mezi dvěma klíči se póza prolíná. Kosti, "
        "které patří k ruce, ale v kostře pod ní nevisí (prsty Jedi Academy pod rradius, celá paže Jedi Outcast), se otáčejí s ní a kroutící kost "
        "(rradiusX) přebírá podíl zkrutu, který odpovídá jejímu místu na předloktí. Klíče netvoří síť znovu; Ctrl+Z vrátí každý zpět."),
    "h_posen": ("Poses, mirroring and hand poses", "Posen, Spiegeln und Handposen", "姿势、镜像和手部姿势", "ポーズ、反転、ハンドポーズ",
                "Poses, reflejo y poses de mano", "Pózy, zrcadlení a pózy ruky"),
    "posen": (
        "**{{MbRgKopieren}}** remembers the pose of every bone as you see it, **{{MbRgEinfuegen}}** sets it as keys at another frame, "
        "**{{MbRgGespiegelt}}** with left and right swapped and mirrored at the side axis of the skeleton (like Blender: Ctrl+C, Ctrl+V, Ctrl+Shift+V). "
        "**{{MbRgBindeAuswahl}}** (Blender Alt+R) and **{{MbRgBindeAlle}}** go back to the bind pose of the GLA. The hand poses come from the animations of "
        "the game itself: open is the bind pose, fist the first frame of BOTH_MELEE1, saber grip the first frame of BOTH_STAND1. To select, "
        "**{{MbRgEltern}}** and **{{MbRgKinder}}** walk the skeleton (3ds Max Page Up / Page Down, Blender [ and ]), **{{MbRgGegenseite}}** jumps to the "
        "bone of the other side (Blender Ctrl+Shift+M).",
        "**{{MbRgKopieren}}** merkt sich die Pose jedes Knochens, wie sie zu sehen ist, **{{MbRgEinfuegen}}** setzt sie als Schlüssel an einem anderen "
        "Frame, **{{MbRgGespiegelt}}** mit vertauschtem Links und Rechts und an der Seitenachse des Skeletts gespiegelt (wie Blender: Strg+C, Strg+V, "
        "Strg+Umschalt+V). **{{MbRgBindeAuswahl}}** (Blender Alt+R) und **{{MbRgBindeAlle}}** gehen zurück in die Bindepose der GLA. Die Handposen kommen "
        "aus den Animationen des Spiels selbst: offen ist die Bindepose, Faust der erste Frame von BOTH_MELEE1, Säbelgriff der erste Frame von BOTH_STAND1. "
        "Zum Auswählen gehen **{{MbRgEltern}}** und **{{MbRgKinder}}** durch das Skelett (3ds Max Bild auf / Bild ab, Blender [ und ]), "
        "**{{MbRgGegenseite}}** springt zum Knochen der anderen Seite (Blender Strg+Umschalt+M).",
        "**{{MbRgKopieren}}** 记住每根骨骼当前所见的姿势, **{{MbRgEinfuegen}}** 把它设为另一帧的关键帧, **{{MbRgGespiegelt}}** 则交换左右并以骨架侧轴镜像 "
        "(像 Blender: Ctrl+C、Ctrl+V、Ctrl+Shift+V)。**{{MbRgBindeAuswahl}}** (Blender Alt+R) 和 **{{MbRgBindeAlle}}** 回到 GLA 的绑定姿势。手部姿势来自"
        "游戏本身的动画: 张开是绑定姿势, 握拳是 BOTH_MELEE1 的第一帧, 光剑握把是 BOTH_STAND1 的第一帧。选择时 **{{MbRgEltern}}** 和 **{{MbRgKinder}}** "
        "在骨架中移动 (3ds Max 的 Page Up / Page Down, Blender 的 [ 和 ]), **{{MbRgGegenseite}}** 跳到另一侧的骨骼 (Blender Ctrl+Shift+M)。",
        "**{{MbRgKopieren}}** は見えているとおりの全ボーンのポーズを覚え、**{{MbRgEinfuegen}}** は別のフレームにキーとして置き、**{{MbRgGespiegelt}}** "
        "は左右を入れ替えてスケルトンの左右軸で反転して置きます (Blender 風: Ctrl+C、Ctrl+V、Ctrl+Shift+V)。**{{MbRgBindeAuswahl}}** (Blender Alt+R) と "
        "**{{MbRgBindeAlle}}** は GLA のバインドポーズに戻します。ハンドポーズはゲーム自身のアニメーションから: 開くはバインドポーズ、拳は BOTH_MELEE1 の"
        "最初のフレーム、セーバーグリップは BOTH_STAND1 の最初のフレーム。選択では **{{MbRgEltern}}** と **{{MbRgKinder}}** がスケルトンをたどり "
        "(3ds Max の Page Up / Page Down、Blender の [ と ])、**{{MbRgGegenseite}}** が反対側のボーンへ移ります (Blender Ctrl+Shift+M)。",
        "**{{MbRgKopieren}}** recuerda la pose de cada hueso tal como se ve, **{{MbRgEinfuegen}}** la pone como claves en otro fotograma y "
        "**{{MbRgGespiegelt}}** con izquierda y derecha intercambiadas, reflejada en el eje lateral del esqueleto (como en Blender: Ctrl+C, Ctrl+V, "
        "Ctrl+Mayús+V). **{{MbRgBindeAuswahl}}** (Blender Alt+R) y **{{MbRgBindeAlle}}** vuelven a la pose de unión del GLA. Las poses de mano salen de las "
        "animaciones del propio juego: abierta es la pose de unión, puño el primer fotograma de BOTH_MELEE1, agarre del sable el primer fotograma de "
        "BOTH_STAND1. Para seleccionar, **{{MbRgEltern}}** y **{{MbRgKinder}}** recorren el esqueleto (3ds Max Re Pág / Av Pág, Blender [ y ]) y "
        "**{{MbRgGegenseite}}** salta al hueso del otro lado (Blender Ctrl+Mayús+M).",
        "**{{MbRgKopieren}}** si zapamatuje pózu každé kosti, jak je vidět, **{{MbRgEinfuegen}}** ji nastaví jako klíče na jiném snímku a "
        "**{{MbRgGespiegelt}}** s prohozenou levou a pravou stranou, zrcadlenou podle boční osy kostry (jako Blender: Ctrl+C, Ctrl+V, Ctrl+Shift+V). "
        "**{{MbRgBindeAuswahl}}** (Blender Alt+R) a **{{MbRgBindeAlle}}** vrátí vaznou pózu GLA. Pózy ruky pocházejí z animací samotné hry: otevřená je "
        "vazná póza, pěst první snímek BOTH_MELEE1, úchop meče první snímek BOTH_STAND1. Pro výběr procházejí **{{MbRgEltern}}** a **{{MbRgKinder}}** "
        "kostru (3ds Max Page Up / Page Down, Blender [ a ]), **{{MbRgGegenseite}}** skočí na kost druhé strany (Blender Ctrl+Shift+M)."),
    "bild_menue": (
        "The menu Animation of the model tab with every rig function and its keys in the chosen controls.",
        "Das Menü Animation des Model-Tabs mit jeder Rig-Funktion und ihren Tasten in der gewählten Bedienung.",
        "模型选项卡的动画菜单, 列出每个绑定功能及其在所选操作方式下的按键。",
        "モデルタブのアニメーションメニュー。リグの全機能と、選んだ操作方式でのキーが並びます。",
        "El menú Animación de la pestaña Model con cada función del rig y sus teclas en el manejo elegido.",
        "Nabídka Animace karty Model s každou funkcí rigu a jejími klávesami ve zvoleném ovládání."),
    "h_geister": ("Ghosts, silhouette and motion paths", "Ghosts, Silhouette und Bewegungspfade", "残影、剪影和运动路径", "ゴースト、シルエット、モーションパス",
                  "Fantasmas, silueta y trayectorias", "Duchové, silueta a dráhy pohybu"),
    "geister": (
        "**{{MbRgGhosts}}** shows the poses before (blue) and after (orange) the current frame see-through - how many and how far apart is set under "
        "**{{MbRgDavor}}**, **{{MbRgDanach}}** and **{{MbRgAbstand}}**, the colours below. **{{MbRgSilhouette}}** draws the figure black on a light "
        "background to judge how readable a pose is. **{{MbRgPfadHinzu}}** shows the path of the selected bone or tag (*r_hand for the saber, a foot, the "
        "root) over the frames before and after: a dot per frame, the current frame white, keys as squares. Ghosts and paths stay inside the sequence and "
        "follow every key at once.",
        "**{{MbRgGhosts}}** zeigt die Posen vor (blau) und nach (orange) dem aktuellen Frame durchscheinend - wie viele und wie weit auseinander steht unter "
        "**{{MbRgDavor}}**, **{{MbRgDanach}}** und **{{MbRgAbstand}}**, die Farben darunter. **{{MbRgSilhouette}}** zeichnet die Figur schwarz auf hellem "
        "Grund, um zu beurteilen, wie lesbar eine Pose ist. **{{MbRgPfadHinzu}}** zeigt die Bahn des gewählten Knochens oder Tags (*r_hand für den Säbel, "
        "ein Fuß, die Wurzel) über die Frames davor und danach: ein Punkt je Frame, der aktuelle weiß, Schlüssel als Quadrate. Ghosts und Pfade bleiben "
        "in der Sequenz und folgen jedem Schlüssel sofort.",
        "**{{MbRgGhosts}}** 以半透明显示当前帧之前 (蓝) 和之后 (橙) 的姿势——数量和间隔在 **{{MbRgDavor}}**、**{{MbRgDanach}}** 和 **{{MbRgAbstand}}** "
        "中设置, 颜色在其下方。**{{MbRgSilhouette}}** 在浅色背景上把角色画成黑色, 用于判断姿势的可读性。**{{MbRgPfadHinzu}}** 显示所选骨骼或标签 "
        "(光剑用 *r_hand、脚、根) 在前后各帧的轨迹: 每帧一个点, 当前帧为白色, 关键帧为方块。残影和路径保持在序列内, 并立即跟随每个关键帧。",
        "**{{MbRgGhosts}}** は現在フレームの前 (青) と後 (橙) のポーズを半透明で表示します - 数と間隔は **{{MbRgDavor}}**、**{{MbRgDanach}}**、"
        "**{{MbRgAbstand}}** で、色はその下で設定します。**{{MbRgSilhouette}}** はフィギュアを明るい背景に黒で描き、ポーズの読みやすさを判断できます。"
        "**{{MbRgPfadHinzu}}** は選択中のボーンまたはタグ (セーバーなら *r_hand、足、ルート) の前後のフレームの軌跡を表示します: フレームごとに点、"
        "現在フレームは白、キーは四角。ゴーストとパスはシーケンス内にとどまり、キーにすぐ追従します。",
        "**{{MbRgGhosts}}** muestra translúcidas las poses antes (azul) y después (naranja) del fotograma actual; cuántas y a qué distancia se fija en "
        "**{{MbRgDavor}}**, **{{MbRgDanach}}** y **{{MbRgAbstand}}**, y los colores debajo. **{{MbRgSilhouette}}** dibuja la figura en negro sobre fondo "
        "claro para juzgar lo legible que es una pose. **{{MbRgPfadHinzu}}** muestra la trayectoria del hueso o tag seleccionado (*r_hand para el sable, un "
        "pie, la raíz) en los fotogramas anteriores y posteriores: un punto por fotograma, el actual en blanco, las claves como cuadrados. Fantasmas y "
        "trayectorias se quedan dentro de la secuencia y siguen cada clave al momento.",
        "**{{MbRgGhosts}}** ukazuje průsvitně pózy před (modře) a po (oranžově) aktuálním snímku - kolik a jak daleko od sebe se nastaví v "
        "**{{MbRgDavor}}**, **{{MbRgDanach}}** a **{{MbRgAbstand}}**, barvy pod tím. **{{MbRgSilhouette}}** kreslí postavu černě na světlém pozadí, aby šlo "
        "posoudit čitelnost pózy. **{{MbRgPfadHinzu}}** ukáže dráhu vybrané kosti nebo tagu (*r_hand pro meč, chodidlo, kořen) přes snímky před a po: tečka "
        "za každý snímek, aktuální bíle, klíče jako čtverce. Duchové a dráhy zůstávají v sekvenci a hned sledují každý klíč."),
    "bild_geister": (
        "Stormtrooper in BOTH_WALK1 with three ghosts before and after and the motion paths of the right hand and both feet; the selected control Hand R in the view.",
        "Sturmtruppler in BOTH_WALK1 mit drei Ghosts davor und danach und den Bewegungspfaden der rechten Hand und beider Füße; der gewählte Griff Hand R in der Ansicht.",
        "BOTH_WALK1 中的冲锋队员, 前后各三个残影, 以及右手和双脚的运动路径; 视图中为所选控制器 Hand R。",
        "BOTH_WALK1 のストームトルーパー。前後に 3 つずつのゴーストと、右手と両足のモーションパス。ビューには選択中のコントロール Hand R。",
        "Soldado de asalto en BOTH_WALK1 con tres fantasmas antes y después y las trayectorias de la mano derecha y ambos pies; el control seleccionado Mano D en la vista.",
        "Stormtrooper v BOTH_WALK1 se třemi duchy před a po a dráhami pravé ruky a obou chodidel; vybraný ovladač Ruka P v pohledu."),
    "h_backen": ("Baking into the GLA", "In die GLA backen", "烘焙到 GLA", "GLA へのベイク", "Hornear en el GLA", "Zapečení do GLA"),
    "backen": (
        "**{{MbRgBacken}}** writes the selected sequence with the keys of the rig as GLA frames and puts it on the next free slot of the target in "
        "**{{MbRKartenGla}}** (map GLA, _humanoid.gla or an own GLA). Bones without a key keep the 14 bytes of the original GLA; changed ones are "
        "compressed like Carcass (compressBone). A rig without changes bakes byte for byte the original - checked with every GLA of Jedi Academy, Jedi "
        "Outcast and Movie Duels; compressing everything anew stays within two quantisation steps per quaternion component.",
        "**{{MbRgBacken}}** schreibt die gewählte Sequenz mit den Schlüsseln des Rigs als GLA-Frames und legt sie auf den nächsten freien Platz des Ziels "
        "in **{{MbRKartenGla}}** (Karten-GLA, _humanoid.gla oder eigene GLA). Knochen ohne Schlüssel behalten die 14 Bytes der Original-GLA; geänderte "
        "werden wie von Carcass komprimiert (compressBone). Ein Rig ohne Änderung backt Byte für Byte das Original - geprüft an jeder GLA von Jedi Academy, "
        "Jedi Outcast und Movie Duels; alles neu komprimiert bleibt innerhalb von zwei Quantisierungsstufen je Quaternion-Anteil.",
        "**{{MbRgBacken}}** 将所选序列连同绑定的关键帧写为 GLA 帧, 并放到 **{{MbRKartenGla}}** 中目标 (地图 GLA、_humanoid.gla 或自有 GLA) 的下一个空闲"
        "槽位。没有关键帧的骨骼保留原 GLA 的 14 字节; 改动的骨骼按 Carcass 的方式压缩 (compressBone)。未改动的绑定会逐字节烘焙出原件——已在 Jedi Academy、"
        "Jedi Outcast 和 Movie Duels 的每个 GLA 上检查; 全部重新压缩时每个四元数分量的差异不超过两个量化级。",
        "**{{MbRgBacken}}** は選択中のシーケンスをリグのキー込みで GLA フレームとして書き、**{{MbRKartenGla}}** の出力先 (マップ GLA、_humanoid.gla、"
        "独自 GLA) の次の空きスロットに置きます。キーのないボーンは元の GLA の 14 バイトのまま、変更したボーンは Carcass と同じく圧縮されます "
        "(compressBone)。変更のないリグは元のものをバイト単位でそのままベイクします - Jedi Academy、Jedi Outcast、Movie Duels のすべての GLA で確認済み。"
        "すべて圧縮し直してもクォータニオン成分ごとの差は量子化 2 段以内です。",
        "**{{MbRgBacken}}** escribe la secuencia seleccionada con las claves del rig como fotogramas GLA y la pone en el siguiente hueco libre del destino de "
        "**{{MbRKartenGla}}** (GLA de mapa, _humanoid.gla o GLA propio). Los huesos sin clave conservan los 14 bytes del GLA original; los cambiados se "
        "comprimen como en Carcass (compressBone). Un rig sin cambios hornea byte a byte el original (comprobado con cada GLA de Jedi Academy, Jedi Outcast "
        "y Movie Duels); al comprimirlo todo de nuevo queda dentro de dos pasos de cuantización por componente del cuaternión.",
        "**{{MbRgBacken}}** zapíše vybranou sekvenci s klíči rigu jako snímky GLA a položí ji na další volný slot cíle v **{{MbRKartenGla}}** (mapové GLA, "
        "_humanoid.gla nebo vlastní GLA). Kosti bez klíče si ponechají 14 bajtů původního GLA; změněné se komprimují jako v Carcassu (compressBone). Rig beze "
        "změny zapeče bajt po bajtu originál - ověřeno na každém GLA z Jedi Academy, Jedi Outcast a Movie Duels; vše znovu zkomprimované zůstane do dvou "
        "kvantizačních kroků na složku kvaternionu."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def seite():
    """Die Seite modell-rig (Gruppe Missionen, nach modell-dateien)."""
    return {
        "id": "modell-rig",
        "gruppe": "mission",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("img", "h_mb4_ghosts", _t("bild_geister"), "breit"),
            ("h2", "rig", _t("h_rig")),
            ("p", _t("rig")),
            ("h2", "uebersicht", _t("h_uebersicht")),
            ("p", _t("uebersicht")),
            ("img", "h_mb4_uebersicht", _t("bild_uebersicht"), "breit"),
            ("h2", "posieren", _t("h_posieren")),
            ("p", _t("posieren")),
            ("h2", "posen", _t("h_posen")),
            ("p", _t("posen")),
            ("img", "h_mb4_menue", _t("bild_menue")),
            ("h2", "geister", _t("h_geister")),
            ("p", _t("geister")),
            ("h2", "backen", _t("h_backen")),
            ("p", _t("backen")),
        ],
    }
