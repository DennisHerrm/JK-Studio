"""AP13K (MBENTWURF Teil B): die Seite "Model-Tab: Knochen, Rig-Stile und Durchsicht" - die Darstellung der Knochen je Bedienmodus
(Bone-Kasten wie 3ds Max; Oktaeder, Stick, B-Bone, Huelle, Draht wie Blender), Knochen und Griffe im Vordergrund, die Durchsicht
der Figur (Alt+X wie 3ds Max, Alt+Z wie Blender) und die vier Rig-Stile (Standard, Control Rig mit Ringen, CAT-Kaesten,
Blender-Oktaeder), die nur das Aussehen aendern.

Eigene Quelle des Pakets; AP14 haengt die Seite ins Handbuch (inhalt.py: `inhalt_ap13k.seite()` hinter modell-blender bzw. der Seite
des Bereichs Animation, ES/CS in inhalt_es.py / inhalt_cs.py, die Texte von i18n_ap13k in gen_handbuch.py `_lade_ui_texte`) - wie
AP4-AP10. Die Bilder h_ap13k_arten, h_ap13k_durchsicht, h_ap13k_control, h_ap13k_cat und h_ap13k_blender nimmt der Selbsttest auf
(BHED_EDITORTEST=handbuch, BHED_HB_NUR=ap13k). Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs); {{Kennung}} setzt
den Text der Oberflaeche ein.
"""

X = {
    "titel": ("Model tab: bones, rig styles and see-through", "Model-Tab: Knochen, Rig-Stile und Durchsicht", "模型选项卡: 骨骼、绑定样式和透明显示",
              "モデルタブ: ボーン、リグのスタイル、シースルー", "Pestaña Model: huesos, estilos de rig y ver a través", "Karta Model: kosti, styly rigu a průhlednost"),
    "kurz": (
        "Bones look like in the program you come from: bone boxes like 3ds Max, octahedra, sticks, B-bones, envelopes or wires like Blender. The figure can become see-through, bones and controls can lie in front of it or inside it, and the control rig comes in four looks. None of this changes what the rig calculates.",
        "Knochen sehen aus wie in dem Programm, aus dem du kommst: Bone-Kästen wie 3ds Max, Oktaeder, Sticks, B-Bones, Hüllen oder Drähte wie Blender. Die Figur lässt sich durchscheinend schalten, Knochen und Griffe liegen vor ihr oder in ihr, und das Control Rig gibt es in vier Stilen. Nichts davon ändert, was das Rig rechnet.",
        "骨骼的外观与你熟悉的程序相同: 同 3ds Max 的骨骼方盒, 同 Blender 的八面锥、棍形、B-骨骼、封套或线框。角色可以设为半透明, 骨骼和控制器可以位于其前面或内部, 控制绑定有四种外观。这些都不改变绑定的计算。",
        "ボーンは使い慣れたソフトと同じ見た目になります: 3ds Max 風のボーンボックス、Blender 風の八面体、スティック、Bボーン、エンベロープ、ワイヤー。フィギュアを半透明にでき、ボーンとコントロールを手前にも内側にも置け、コントロールリグには 4 つのスタイルがあります。どれもリグの計算は変えません。",
        "Los huesos se ven como en el programa del que vienes: cajas como 3ds Max, octaedros, varillas, B-Bones, envolventes o alambres como Blender. La figura puede volverse translúcida, los huesos y controles pueden quedar delante o dentro de ella, y el rig de control tiene cuatro aspectos. Nada de esto cambia lo que calcula el rig.",
        "Kosti vypadají jako v programu, ze kterého přicházíte: kvádry jako 3ds Max, osmistěny, tyčinky, B-Bone, obálky nebo dráty jako Blender. Postavu lze zprůsvitnit, kosti a ovladače mohou ležet před ní nebo v ní a ovládací rig má čtyři vzhledy. Nic z toho nemění, co rig počítá."),
    "bild_arten": (
        "Like Blender, bones drawn as sticks: one line per bone with a dot at each joint. The choice is in the Overlays menu of the view header under **{{KdAnzeigenAls}}**.",
        "Wie Blender, Knochen als Sticks: je Knochen eine Linie mit einem Punkt an jedem Gelenk. Die Wahl steht im Menü Overlays der Kopfzeile der Ansicht unter **{{KdAnzeigenAls}}**.",
        "同 Blender, 骨骼显示为棍形: 每根骨骼一条线, 每个关节一个点。在视图标题栏的叠加层菜单的 **{{KdAnzeigenAls}}** 下选择。",
        "Blender 風でボーンをスティック表示: ボーンごとに 1 本の線、関節ごとに点。ビューのヘッダーのオーバーレイメニューの **{{KdAnzeigenAls}}** で選びます。",
        "Como Blender, huesos como varillas: una línea por hueso con un punto en cada articulación. La elección está en el menú Superposiciones de la cabecera de la vista, en **{{KdAnzeigenAls}}**.",
        "Jako Blender, kosti jako tyčinky: pro každou kost čára s tečkou v každém kloubu. Volba je v nabídce Překryvy v záhlaví pohledu pod **{{KdAnzeigenAls}}**."),
    "h_knochen": ("How bones are drawn", "Wie Knochen gezeichnet werden", "骨骼的绘制方式", "ボーンの描き方", "Cómo se dibujan los huesos", "Jak se kreslí kosti"),
    "knochen": (
        "Bones show with **{{MbAnKnochen}}** (view header; in the area Weights they are on by default). Like 3ds Max a bone is a bone box that narrows towards its end, filled as in the design; **{{MkFlossen}}** adds its fins. "
        "Like Blender **{{KdAnzeigenAls}}** chooses the display as in Blender's Armature > Viewport Display: **{{KdArtOktaeder}}** (default), **{{KdArtStick}}** (a line of constant width with dots at the joints), "
        "**{{KdArtBBone}}** (a box of constant width), **{{KdArtHuelle}}** (a sphere at each joint, joined along the bone) and **{{KdArtDraht}}** (a thin line). The choice is kept. The selected bone is highlighted; "
        "bones that the game code looks for (cranium, pelvis, the hands ...) keep their orange mark. With the tool **{{BzKnochenWaehlen}}** a click anywhere on the drawn bone selects it, not only near its joint.",
        "Knochen zeigen sich mit **{{MbAnKnochen}}** (Kopfzeile der Ansicht; im Bereich Weights sind sie ab Werk an). Wie 3ds Max ist ein Knochen ein Bone-Kasten, der zum Ende schmaler wird, gefüllt wie im Entwurf; **{{MkFlossen}}** gibt ihm Flossen. "
        "Wie Blender wählt **{{KdAnzeigenAls}}** die Darstellung wie in Blenders Armature > Viewport Display: **{{KdArtOktaeder}}** (Vorgabe), **{{KdArtStick}}** (eine Linie fester Breite mit Punkten an den Gelenken), "
        "**{{KdArtBBone}}** (ein Kasten gleicher Breite), **{{KdArtHuelle}}** (an jedem Gelenk eine Kugel, entlang des Knochens verbunden) und **{{KdArtDraht}}** (eine dünne Linie). Die Wahl bleibt erhalten. Der gewählte Knochen ist hervorgehoben; "
        "Knochen, die der Spielcode sucht (cranium, pelvis, die Hände ...), behalten ihre orange Marke. Mit dem Werkzeug **{{BzKnochenWaehlen}}** wählt ein Klick irgendwo auf den gezeichneten Knochen ihn aus, nicht nur nahe am Gelenk.",
        "用 **{{MbAnKnochen}}** 显示骨骼 (视图标题栏; 在权重区域默认开启)。同 3ds Max 时骨骼是向末端变窄的骨骼方盒, 按设计填充; **{{MkFlossen}}** 为其添加鳍。"
        "同 Blender 时, **{{KdAnzeigenAls}}** 像 Blender 的骨架 > 视图显示那样选择显示方式: **{{KdArtOktaeder}}** (默认)、**{{KdArtStick}}** (关节处带点的等宽线)、"
        "**{{KdArtBBone}}** (等宽方盒)、**{{KdArtHuelle}}** (每个关节一个球, 沿骨骼相连) 和 **{{KdArtDraht}}** (细线)。所选方式会保留。选中的骨骼会突出显示; "
        "游戏代码查找的骨骼 (cranium、pelvis、双手等) 保留其橙色标记。使用 **{{BzKnochenWaehlen}}** 工具时, 单击所绘骨骼上的任意位置即可选中它, 而不仅是关节附近。",
        "ボーンは **{{MbAnKnochen}}** で表示します (ビューのヘッダー。ウェイトエリアでは既定でオン)。3ds Max 風ではボーンは先細りのボーンボックスで、デザインどおり塗りつぶされ、**{{MkFlossen}}** でフィンが付きます。"
        "Blender 風では **{{KdAnzeigenAls}}** が Blender のアーマチュア > ビューポート表示と同じ表示を選びます: **{{KdArtOktaeder}}** (既定)、**{{KdArtStick}}** (関節に点の付いた一定幅の線)、"
        "**{{KdArtBBone}}** (一定幅のボックス)、**{{KdArtHuelle}}** (各関節の球をボーンに沿ってつなぐ)、**{{KdArtDraht}}** (細い線)。選択は保存されます。選択中のボーンは強調表示され、"
        "ゲームのコードが探すボーン (cranium、pelvis、両手など) はオレンジのマークを保ちます。**{{BzKnochenWaehlen}}** ツールでは、関節の近くだけでなく、描かれたボーンのどこをクリックしても選択できます。",
        "Los huesos se muestran con **{{MbAnKnochen}}** (cabecera de la vista; en el área Weights están activados de fábrica). Como 3ds Max, un hueso es una caja que se estrecha hacia el extremo, rellena como en el diseño; **{{MkFlossen}}** le añade aletas. "
        "Como Blender, **{{KdAnzeigenAls}}** elige la visualización como en Esqueleto > Visualización en vistas de Blender: **{{KdArtOktaeder}}** (predeterminado), **{{KdArtStick}}** (una línea de ancho constante con puntos en las articulaciones), "
        "**{{KdArtBBone}}** (una caja de ancho constante), **{{KdArtHuelle}}** (una esfera en cada articulación, unidas a lo largo del hueso) y **{{KdArtDraht}}** (una línea fina). La elección se conserva. El hueso seleccionado se resalta; "
        "los huesos que busca el código del juego (cranium, pelvis, las manos ...) conservan su marca naranja. Con la herramienta **{{BzKnochenWaehlen}}**, un clic en cualquier punto del hueso dibujado lo selecciona, no solo cerca de la articulación.",
        "Kosti se zobrazí pomocí **{{MbAnKnochen}}** (záhlaví pohledu; v oblasti Weights jsou z výroby zapnuté). Jako 3ds Max je kost kvádr, který se ke konci zužuje, vyplněný jako v návrhu; **{{MkFlossen}}** mu přidá ploutve. "
        "Jako Blender volí **{{KdAnzeigenAls}}** zobrazení jako v Blenderu Armatura > Zobrazení v pohledu: **{{KdArtOktaeder}}** (výchozí), **{{KdArtStick}}** (čára stálé šířky s tečkami v kloubech), "
        "**{{KdArtBBone}}** (kvádr stálé šířky), **{{KdArtHuelle}}** (v každém kloubu koule, spojené podél kosti) a **{{KdArtDraht}}** (tenká čára). Volba zůstává uložená. Vybraná kost je zvýrazněná; "
        "kosti, které hledá kód hry (cranium, pelvis, ruce ...), si drží oranžovou značku. S nástrojem **{{BzKnochenWaehlen}}** vybere kost kliknutí kdekoli na nakreslenou kost, nejen u kloubu."),
    "bild_durchsicht": (
        "See-through with bones not in front: the figure lies half transparent over what is behind it, the bones inside it show through.",
        "Durchsicht mit Knochen nicht im Vordergrund: die Figur liegt halb durchsichtig über dem, was hinter ihr ist, die Knochen in ihr scheinen durch.",
        "透明显示且骨骼不在前面: 角色以半透明覆盖其后面的内容, 其内部的骨骼透出来。",
        "シースルーでボーンを最前面にしない場合: フィギュアが後ろのものの上に半透明で重なり、内側のボーンが透けて見えます。",
        "Ver a través con los huesos no al frente: la figura queda semitransparente sobre lo que hay detrás, y los huesos de su interior se ven a través de ella.",
        "Průhlednost s kostmi mimo popředí: postava leží poloprůhledně nad tím, co je za ní, kosti uvnitř prosvítají."),
    "h_durchsicht": ("In front and see-through", "Im Vordergrund und Durchsicht", "在前面与透明显示", "最前面とシースルー", "Al frente y ver a través", "V popředí a průhlednost"),
    "durchsicht": (
        "**{{KdVorn}}** draws bones and controls over the figure, as Blender's In Front does; this is the default. Switched off, the figure hides what lies inside or behind it, as in 3ds Max. "
        "To look inside then, make the figure see-through: like 3ds Max **{{KdDurchsicht}}** with Alt+X - it belongs to the figure and shows in all views; like Blender **{{KdRoentgen}}** with Alt+Z - it belongs to the active view, "
        "and bones stand in front of the figure there. The figure is drawn with half opacity, its front surface once. Both switches are in the shading menu of the view header ([Standard] like 3ds Max, Viewport Shading like Blender), "
        "in the group **{{KdGruppe}}** on the right in the area Animation, in the right-click menu of the view and in Search all actions.",
        "**{{KdVorn}}** zeichnet Knochen und Griffe über die Figur, wie In Front in Blender; das ist die Vorgabe. Ausgeschaltet verdeckt die Figur, was in oder hinter ihr liegt, wie in 3ds Max. "
        "Um dann hineinzusehen, schaltest du die Figur durchscheinend: wie 3ds Max **{{KdDurchsicht}}** mit Alt+X - sie gehört der Figur und zeigt sich in allen Ansichten; wie Blender **{{KdRoentgen}}** mit Alt+Z - es gehört der aktiven Ansicht, "
        "und die Knochen stehen dort vor der Figur. Die Figur wird mit halber Deckkraft gezeichnet, ihre vordere Fläche einmal. Beide Schalter stehen im Menü der Schattierung der Kopfzeile ([Standard] wie 3ds Max, Viewport Shading wie Blender), "
        "in der Gruppe **{{KdGruppe}}** rechts im Bereich Animation, im Rechtsklick-Menü der Ansicht und in Alle Aktionen suchen.",
        "**{{KdVorn}}** 将骨骼和控制器绘制在角色之上, 同 Blender 的“在前面”; 这是默认设置。关闭后, 角色会遮住其内部或后面的内容, 同 3ds Max。"
        "此时要看到内部, 可将角色设为半透明: 同 3ds Max 时用 Alt+X 切换 **{{KdDurchsicht}}** - 它属于角色, 在所有视图中显示; 同 Blender 时用 Alt+Z 切换 **{{KdRoentgen}}** - 它属于活动视图, "
        "该视图中骨骼位于角色前面。角色以一半的不透明度绘制, 其正面只绘制一次。两个开关都在视图标题栏的着色菜单中 (同 3ds Max 为 [标准], 同 Blender 为视口着色), "
        "也在动画区域右侧的 **{{KdGruppe}}** 组、视图的右键菜单和“搜索所有操作”中。",
        "**{{KdVorn}}** はボーンとコントロールをフィギュアの上に描きます (Blender の最前面と同じ)。これが既定です。オフにすると、3ds Max と同じくフィギュアが内側や後ろにあるものを隠します。"
        "そのとき内側を見るにはフィギュアを半透明にします: 3ds Max 風は Alt+X の **{{KdDurchsicht}}** - フィギュアの設定で、すべてのビューに表示されます。Blender 風は Alt+Z の **{{KdRoentgen}}** - アクティブなビューの設定で、"
        "そのビューではボーンがフィギュアの手前に出ます。フィギュアは不透明度の半分で、手前の面を 1 回だけ描きます。どちらのスイッチもビューのヘッダーのシェーディングメニュー (3ds Max 風は [標準]、Blender 風はビューポートシェーディング)、"
        "アニメーションエリアの右にある **{{KdGruppe}}** グループ、ビューの右クリックメニュー、「すべてのアクションを検索」にあります。",
        "**{{KdVorn}}** dibuja huesos y controles sobre la figura, como Al frente en Blender; es el valor de fábrica. Desactivado, la figura oculta lo que hay dentro o detrás de ella, como en 3ds Max. "
        "Para mirar dentro, haz la figura translúcida: como 3ds Max, **{{KdDurchsicht}}** con Alt+X - pertenece a la figura y se ve en todas las vistas; como Blender, **{{KdRoentgen}}** con Alt+Z - pertenece a la vista activa, "
        "y allí los huesos quedan delante de la figura. La figura se dibuja con media opacidad, su superficie frontal una sola vez. Ambos interruptores están en el menú de sombreado de la cabecera de la vista ([Estándar] como 3ds Max, Viewport Shading como Blender), "
        "en el grupo **{{KdGruppe}}** a la derecha en el área Animación, en el menú del clic derecho de la vista y en Buscar todas las acciones.",
        "**{{KdVorn}}** kreslí kosti a ovladače přes postavu jako V popředí v Blenderu; to je výchozí nastavení. Vypnuto: postava zakryje, co leží v ní nebo za ní, jako ve 3ds Max. "
        "Chcete-li pak vidět dovnitř, zprůsvitněte postavu: jako 3ds Max **{{KdDurchsicht}}** pomocí Alt+X - patří postavě a je vidět ve všech pohledech; jako Blender **{{KdRoentgen}}** pomocí Alt+Z - patří aktivnímu pohledu "
        "a kosti tam stojí před postavou. Postava se kreslí s polovičním krytím, její přední plocha jednou. Oba přepínače jsou v nabídce stínování v záhlaví pohledu ([Standard] jako 3ds Max, Viewport Shading jako Blender), "
        "ve skupině **{{KdGruppe}}** vpravo v oblasti Animace, v nabídce pravého tlačítka pohledu a ve Vyhledat všechny akce."),
    "bild_control": (
        "Rig style **{{KdStilControl}}**: rings around head, neck, chest, spine, hips and limbs, foot plates on the ground, poles as spheres; right red, left blue, IK or FK at each limb.",
        "Rig-Stil **{{KdStilControl}}**: Ringe um Kopf, Hals, Brust, Wirbel, Becken und Glieder, Fußplatten auf dem Boden, Pole als Kugeln; rechts rot, links blau, IK oder FK an jedem Glied.",
        "绑定样式 **{{KdStilControl}}**: 环绕头、颈、胸、脊柱、骨盆和四肢的圆环, 地面上的脚底板, 球形极向目标; 右侧红色, 左侧蓝色, 每个肢体旁标有 IK 或 FK。",
        "リグのスタイル **{{KdStilControl}}**: 頭、首、胸、背骨、骨盤、手足を囲むリング、地面の足のプレート、球のポール。右は赤、左は青、各手足に IK か FK。",
        "Estilo de rig **{{KdStilControl}}**: anillos alrededor de cabeza, cuello, pecho, columna, cadera y extremidades, placas de pie en el suelo, polos como esferas; derecha rojo, izquierda azul, IK o FK en cada extremidad.",
        "Styl rigu **{{KdStilControl}}**: kruhy kolem hlavy, krku, hrudi, páteře, pánve a končetin, desky chodidel na zemi, póly jako koule; vpravo červená, vlevo modrá, u každé končetiny IK nebo FK."),
    "bild_cat": (
        "Rig style **{{KdStilCat}}**: one box per bone in the measures and colours of a CAT rig, foot platforms, IK targets at the hands, crosses as pole targets, the shape on the ground.",
        "Rig-Stil **{{KdStilCat}}**: je Knochen ein Kasten in den Maßen und Farben eines CAT-Rigs, Fußplatten, IK-Ziele an den Händen, Kreuze als Pol-Ziele, die Form auf dem Boden.",
        "绑定样式 **{{KdStilCat}}**: 每根骨骼一个方盒, 采用 CAT 绑定的尺寸和颜色, 脚部平台, 手部 IK 目标, 十字形极向目标, 地面上的形状。",
        "リグのスタイル **{{KdStilCat}}**: ボーンごとに CAT リグの寸法と色のボックス、足のプラットフォーム、手の IK ターゲット、十字のポールターゲット、地面のシェイプ。",
        "Estilo de rig **{{KdStilCat}}**: una caja por hueso con las medidas y colores de un rig de CAT, plataformas de pie, objetivos IK en las manos, cruces como objetivos de polo, la forma en el suelo.",
        "Styl rigu **{{KdStilCat}}**: pro každou kost kvádr v mírách a barvách rigu CAT, plošiny chodidel, cíle IK u rukou, kříže jako cíle pólů, tvar na zemi."),
    "bild_blender": (
        "Rig style **{{KdStilBlender}}**: bones as octahedra, circles around hips and spine, squares at hands and feet, circles as pole targets with a dashed line, a circle on the ground.",
        "Rig-Stil **{{KdStilBlender}}**: Knochen als Oktaeder, Kreise um Becken und Wirbel, Quadrate an Händen und Füßen, Kreise als Pol-Ziele mit gestrichelter Linie, ein Kreis auf dem Boden.",
        "绑定样式 **{{KdStilBlender}}**: 骨骼为八面锥, 骨盆和脊柱周围的圆, 手脚处的方形, 带虚线的圆形极向目标, 地面上的圆。",
        "リグのスタイル **{{KdStilBlender}}**: ボーンは八面体、骨盤と背骨を囲む円、手足の四角、破線付きの円のポールターゲット、地面の円。",
        "Estilo de rig **{{KdStilBlender}}**: huesos como octaedros, círculos alrededor de cadera y columna, cuadrados en manos y pies, círculos como objetivos de polo con línea discontinua, un círculo en el suelo.",
        "Styl rigu **{{KdStilBlender}}**: kosti jako osmistěny, kruhy kolem pánve a páteře, čtverce u rukou a nohou, kruhy jako cíle pólů s čárkovanou čarou, kruh na zemi."),
    "h_stile": ("Rig styles", "Rig-Stile", "绑定样式", "リグのスタイル", "Estilos de rig", "Styly rigu"),
    "stile": (
        "**{{KdRigStil}}** chooses how the control rig and the skeleton look - in the group **{{KdGruppe}}** on the right in the area Animation, in the bone menu of the view header, in the right-click menu of the view and in Search all actions. "
        "**{{KdStilStandard}}** is the look of the design: squares (like 3ds Max) or rings (like Blender) at the joints, coloured by side. **{{KdStilControl}}** looks like a control rig in 3ds Max: rings around the body parts that fit the figure, "
        "foot plates, poles as spheres with a line to the joint, a circle on the ground; right is red, left is blue, the middle green and yellow, and IK or FK stands at each limb. **{{KdStilCat}}** looks like a CAT rig: a box per bone in the measures "
        "and colours of CAT (hips orange, chest yellow, head red, left arm green, right arm blue, legs dark green and dark blue), foot platforms, IK targets at the hands, crosses as pole targets and the shape on the ground; a box is the FK control "
        "of its bone. **{{KdStilBlender}}** looks like an IK rig in Blender: octahedra, circles around hips and spine, squares at hands and feet, circles as pole targets; a bone is its own FK control. "
        "A click selects the shape you see - the ring, the box (its edges and its inside), the circle; then you move or rotate the control as always. The style changes only the look: IK, FK, keys and the files written for the game stay the same.",
        "**{{KdRigStil}}** wählt, wie Control Rig und Skelett aussehen - in der Gruppe **{{KdGruppe}}** rechts im Bereich Animation, im Menü der Knochen in der Kopfzeile der Ansicht, im Rechtsklick-Menü der Ansicht und in Alle Aktionen suchen. "
        "**{{KdStilStandard}}** ist das Aussehen des Entwurfs: Kästen (wie 3ds Max) oder Ringe (wie Blender) an den Gelenken, nach Seite gefärbt. **{{KdStilControl}}** sieht aus wie ein Control Rig in 3ds Max: Ringe um die Körperteile, die sich der Figur anpassen, "
        "Fußplatten, Pole als Kugeln mit einer Linie zum Gelenk, ein Kreis auf dem Boden; rechts ist rot, links blau, die Mitte grün und gelb, und an jedem Glied steht IK oder FK. **{{KdStilCat}}** sieht aus wie ein CAT-Rig: je Knochen ein Kasten in den Maßen "
        "und Farben von CAT (Becken orange, Brust gelb, Kopf rot, linker Arm grün, rechter Arm blau, Beine dunkelgrün und dunkelblau), Fußplatten, IK-Ziele an den Händen, Kreuze als Pol-Ziele und die Form auf dem Boden; ein Kasten ist der FK-Griff "
        "seines Knochens. **{{KdStilBlender}}** sieht aus wie ein IK-Rig in Blender: Oktaeder, Kreise um Becken und Wirbel, Quadrate an Händen und Füßen, Kreise als Pol-Ziele; ein Knochen ist sein eigener FK-Griff. "
        "Ein Klick wählt die Form, die du siehst - den Ring, den Kasten (seine Kanten und sein Inneres), den Kreis; danach verschiebst oder drehst du den Griff wie immer. Der Stil ändert nur das Aussehen: IK, FK, Schlüssel und die Dateien für das Spiel bleiben gleich.",
        "**{{KdRigStil}}** 选择控制绑定和骨架的外观 - 位于动画区域右侧的 **{{KdGruppe}}** 组、视图标题栏的骨骼菜单、视图的右键菜单和“搜索所有操作”中。"
        "**{{KdStilStandard}}** 是设计中的外观: 关节处的方框 (同 3ds Max) 或圆环 (同 Blender), 按左右侧着色。**{{KdStilControl}}** 类似 3ds Max 中的控制绑定: 贴合角色的身体各部位圆环、"
        "脚底板、带连线的球形极向目标、地面上的圆; 右侧红色, 左侧蓝色, 中间绿色和黄色, 每个肢体旁标有 IK 或 FK。**{{KdStilCat}}** 类似 CAT 绑定: 每根骨骼一个方盒, 采用 CAT 的尺寸"
        "和颜色 (骨盆橙色, 胸部黄色, 头部红色, 左臂绿色, 右臂蓝色, 双腿深绿和深蓝)、脚部平台、手部 IK 目标、十字形极向目标和地面上的形状; 方盒就是其骨骼的 FK 控制器。"
        "**{{KdStilBlender}}** 类似 Blender 中的 IK 绑定: 八面锥、骨盆和脊柱周围的圆、手脚处的方形、圆形极向目标; 骨骼本身就是它的 FK 控制器。"
        "单击会选中你看到的形状 - 圆环、方盒 (边和内部)、圆; 之后照常移动或旋转控制器。样式只改变外观: IK、FK、关键帧和为游戏写出的文件保持不变。",
        "**{{KdRigStil}}** でコントロールリグとスケルトンの見た目を選びます - アニメーションエリアの右にある **{{KdGruppe}}** グループ、ビューのヘッダーのボーンのメニュー、ビューの右クリックメニュー、「すべてのアクションを検索」にあります。"
        "**{{KdStilStandard}}** はデザインどおりの見た目です: 関節の四角 (3ds Max 風) またはリング (Blender 風) を左右で色分けします。**{{KdStilControl}}** は 3ds Max のコントロールリグ風: フィギュアに合わせた体の各部のリング、"
        "足のプレート、関節まで線の付いた球のポール、地面の円。右は赤、左は青、中央は緑と黄で、各手足に IK か FK が表示されます。**{{KdStilCat}}** は CAT リグ風: ボーンごとに CAT の寸法と色のボックス "
        "(骨盤はオレンジ、胸は黄、頭は赤、左腕は緑、右腕は青、脚は濃い緑と濃い青)、足のプラットフォーム、手の IK ターゲット、十字のポールターゲット、地面のシェイプ。ボックスがそのボーンの FK コントロールです。"
        "**{{KdStilBlender}}** は Blender の IK リグ風: 八面体、骨盤と背骨を囲む円、手足の四角、円のポールターゲット。ボーンそのものが FK コントロールです。"
        "クリックすると見えているシェイプ (リング、ボックスの辺と内側、円) が選択され、その後はいつもどおりコントロールを移動・回転します。スタイルで変わるのは見た目だけで、IK、FK、キー、ゲーム用に書き出すファイルは同じです。",
        "**{{KdRigStil}}** elige el aspecto del rig de control y del esqueleto: en el grupo **{{KdGruppe}}** a la derecha en el área Animación, en el menú de huesos de la cabecera de la vista, en el menú del clic derecho de la vista y en Buscar todas las acciones. "
        "**{{KdStilStandard}}** es el aspecto del diseño: cuadros (como 3ds Max) o anillos (como Blender) en las articulaciones, coloreados por lado. **{{KdStilControl}}** se ve como un rig de control en 3ds Max: anillos alrededor de las partes del cuerpo que se ajustan a la figura, "
        "placas de pie, polos como esferas con una línea a la articulación, un círculo en el suelo; la derecha es roja, la izquierda azul, el centro verde y amarillo, y en cada extremidad figura IK o FK. **{{KdStilCat}}** se ve como un rig de CAT: una caja por hueso con las medidas "
        "y colores de CAT (cadera naranja, pecho amarillo, cabeza roja, brazo izquierdo verde, brazo derecho azul, piernas verde oscuro y azul oscuro), plataformas de pie, objetivos IK en las manos, cruces como objetivos de polo y la forma en el suelo; una caja es el control FK "
        "de su hueso. **{{KdStilBlender}}** se ve como un rig IK en Blender: octaedros, círculos alrededor de cadera y columna, cuadrados en manos y pies, círculos como objetivos de polo; un hueso es su propio control FK. "
        "Un clic selecciona la forma que ves: el anillo, la caja (sus aristas y su interior), el círculo; después mueves o giras el control como siempre. El estilo solo cambia el aspecto: IK, FK, claves y los archivos escritos para el juego siguen igual.",
        "**{{KdRigStil}}** volí, jak vypadá ovládací rig a kostra - ve skupině **{{KdGruppe}}** vpravo v oblasti Animace, v nabídce kostí v záhlaví pohledu, v nabídce pravého tlačítka pohledu a ve Vyhledat všechny akce. "
        "**{{KdStilStandard}}** je vzhled návrhu: čtverce (jako 3ds Max) nebo kruhy (jako Blender) u kloubů, obarvené podle strany. **{{KdStilControl}}** vypadá jako ovládací rig ve 3ds Max: kruhy kolem částí těla, které se přizpůsobí postavě, "
        "desky chodidel, póly jako koule s čárou ke kloubu, kruh na zemi; vpravo je červená, vlevo modrá, střed zelený a žlutý a u každé končetiny stojí IK nebo FK. **{{KdStilCat}}** vypadá jako rig CAT: pro každou kost kvádr v mírách "
        "a barvách CAT (pánev oranžová, hruď žlutá, hlava červená, levá paže zelená, pravá paže modrá, nohy tmavě zelená a tmavě modrá), plošiny chodidel, cíle IK u rukou, kříže jako cíle pólů a tvar na zemi; kvádr je ovladač FK "
        "své kosti. **{{KdStilBlender}}** vypadá jako rig IK v Blenderu: osmistěny, kruhy kolem pánve a páteře, čtverce u rukou a nohou, kruhy jako cíle pólů; kost je sama svým ovladačem FK. "
        "Kliknutí vybere tvar, který vidíte - kruh, kvádr (jeho hrany i vnitřek), kružnici; pak ovladač posouváte nebo otáčíte jako vždy. Styl mění jen vzhled: IK, FK, klíče a soubory zapsané pro hru zůstávají stejné."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    z = X[schluessel]
    return T(z[0], z[1], z[2], z[3])


def seite():
    """Die Seite modell-knochen (Gruppe Missionen, nach modell-blender bzw. der Seite des Bereichs Animation)."""
    return {
        "id": "modell-knochen",
        "gruppe": "mission",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("img", "h_ap13k_arten", _t("bild_arten"), "breit"),
            ("h2", "knochen", _t("h_knochen")),
            ("p", _t("knochen")),
            ("img", "h_ap13k_durchsicht", _t("bild_durchsicht"), "breit"),
            ("h2", "durchsicht", _t("h_durchsicht")),
            ("p", _t("durchsicht")),
            ("img", "h_ap13k_control", _t("bild_control"), "breit"),
            ("img", "h_ap13k_cat", _t("bild_cat"), "breit"),
            ("img", "h_ap13k_blender", _t("bild_blender"), "breit"),
            ("h2", "rigstile", _t("h_stile")),
            ("p", _t("stile")),
        ],
    }
