# -*- coding: utf-8 -*-
"""Oberflaechentexte von AP13K (rc612, MBENTWURF Teil B): Knochen-Darstellung (Abgleich R14), Rig-Stile (Wunsch shank 12.11.) und
Durchsicht der Figur - fuer tools/gen_i18n.py und die weiteren Sprachen.

Begriffe wie die Vorbilder: "Display As", "Octahedral", "Stick", "B-Bone", "Envelope", "Wire", "In Front" und "X-Ray" wie Blender
(Armature > Viewport Display, View3DShading; deutsch nach dem Entwurf: Oktaeder, Stick, B-Bone, Huelle, Roentgen), "See-Through"
wie 3ds Max (Objekteigenschaften > Display Properties, Alt+X; deutsch nach dem Entwurf: Durchsicht).

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    # --- die Gruppe im rechten Feld, das Untermenue der Kopfzeile ---
    ("KdGruppe", "Bone and rig display", "Knochen- und Rig-Darstellung", "骨骼和绑定的显示", "ボーンとリグの表示", "Visualización de huesos y rig",
     "Zobrazení kostí a rigu"),
    ("KdRigStil", "Rig style", "Rig-Stil", "绑定样式", "リグのスタイル", "Estilo del rig", "Styl rigu"),
    ("KdRigStilHint",
     "How bones and controls of the rig look. Only the look changes - IK, FK, keys and files stay the same in every style.",
     "Wie Knochen und Griffe des Rigs aussehen. Nur das Aussehen ändert sich - IK, FK, Schlüssel und Dateien bleiben in jedem Stil gleich.",
     "骨骼和绑定控制器的外观。只改变外观 - IK、FK、关键帧和文件在每种样式下都相同。",
     "ボーンとリグのコントロールの見た目です。変わるのは見た目だけで、IK、FK、キー、ファイルはどのスタイルでも同じです。",
     "El aspecto de los huesos y controles del rig. Solo cambia el aspecto: IK, FK, claves y archivos son iguales en todos los estilos.",
     "Jak vypadají kosti a ovladače rigu. Mění se jen vzhled - IK, FK, klíče a soubory zůstávají v každém stylu stejné."),
    ("KdStilStandard", "Standard", "Standard", "标准", "標準", "Estándar", "Standardní"),
    ("KdStilControl", "Control rig (rings)", "Control Rig (Ringe)", "控制绑定 (环)", "コントロールリグ (リング)", "Rig de control (anillos)",
     "Ovládací rig (kruhy)"),
    ("KdStilCat", "CAT boxes", "CAT-Kästen", "CAT 方盒", "CAT ボックス", "Cajas CAT", "Kvádry CAT"),
    ("KdStilBlender", "Blender octahedra", "Blender-Oktaeder", "Blender 八面锥", "Blender 八面体", "Octaedros de Blender", "Osmistěny Blenderu"),
    ("KdStilStandardHint",
     "As in the design: bones as bone boxes (like 3ds Max) or as in Blender, controls as squares or rings at the joints, coloured by side.",
     "Wie im Entwurf: Knochen als Bone-Kästen (wie 3ds Max) oder wie in Blender, Griffe als Kästen oder Ringe an den Gelenken, nach Seite gefärbt.",
     "与设计相同: 骨骼显示为骨骼方盒 (同 3ds Max) 或同 Blender, 控制器为关节处的方框或圆环, 按左右侧着色。",
     "デザインどおり: ボーンはボーンボックス (3ds Max 風) または Blender 風、コントロールは関節の四角かリングで、左右で色分けします。",
     "Como en el diseño: huesos como cajas (como 3ds Max) o como en Blender, controles como cuadros o anillos en las articulaciones, coloreados por lado.",
     "Jako v návrhu: kosti jako kvádry (jako 3ds Max) nebo jako v Blenderu, ovladače jako čtverce nebo kruhy u kloubů, obarvené podle strany."),
    ("KdStilControlHint",
     "Like a control rig in 3ds Max: rings around head, neck, chest, spine, hips and limbs, foot plates, poles as spheres with a line, a circle on the ground. Right red, left blue, middle green and yellow; IK or FK stands at each limb.",
     "Wie ein Control Rig in 3ds Max: Ringe um Kopf, Hals, Brust, Wirbel, Becken und Glieder, Fußplatten, Pole als Kugeln mit Linie, ein Kreis auf dem Boden. Rechts rot, links blau, Mitte grün und gelb; an jedem Glied steht IK oder FK.",
     "类似 3ds Max 中的控制绑定: 环绕头、颈、胸、脊柱、骨盆和四肢的圆环, 脚底板, 带连线的球形极向目标, 地面上的圆。右侧红色, 左侧蓝色, 中间绿色和黄色; 每个肢体旁标有 IK 或 FK。",
     "3ds Max のコントロールリグ風: 頭、首、胸、背骨、骨盤、手足を囲むリング、足のプレート、線付きの球のポール、地面の円。右は赤、左は青、中央は緑と黄。各手足に IK か FK を表示します。",
     "Como un rig de control en 3ds Max: anillos alrededor de cabeza, cuello, pecho, columna, cadera y extremidades, placas de pie, polos como esferas con línea, un círculo en el suelo. Derecha rojo, izquierda azul, centro verde y amarillo; en cada extremidad figura IK o FK.",
     "Jako ovládací rig ve 3ds Max: kruhy kolem hlavy, krku, hrudi, páteře, pánve a končetin, desky chodidel, póly jako koule s čárou, kruh na zemi. Vpravo červená, vlevo modrá, střed zelená a žlutá; u každé končetiny stojí IK nebo FK."),
    ("KdStilCatHint",
     "Like a CAT rig: one box per bone in the measures and colours of CAT, foot platforms, IK targets at the hands, crosses as pole targets, the shape on the ground. A box is the FK control of its bone.",
     "Wie ein CAT-Rig: je Knochen ein Kasten in den Maßen und Farben von CAT, Fußplatten, IK-Ziele an den Händen, Kreuze als Pol-Ziele, die Form auf dem Boden. Ein Kasten ist der FK-Griff seines Knochens.",
     "类似 CAT 绑定: 每根骨骼一个方盒, 采用 CAT 的尺寸和颜色, 脚部平台, 手部 IK 目标, 十字形极向目标, 地面上的形状。方盒就是其骨骼的 FK 控制器。",
     "CAT リグ風: ボーンごとに CAT の寸法と色のボックス、足のプラットフォーム、手の IK ターゲット、十字のポールターゲット、地面のシェイプ。ボックスがそのボーンの FK コントロールです。",
     "Como un rig de CAT: una caja por hueso con las medidas y colores de CAT, plataformas de pie, objetivos IK en las manos, cruces como objetivos de polo, la forma en el suelo. Una caja es el control FK de su hueso.",
     "Jako rig CAT: pro každou kost kvádr v mírách a barvách CAT, plošiny chodidel, cíle IK u rukou, kříže jako cíle pólů, tvar na zemi. Kvádr je ovladač FK své kosti."),
    ("KdStilBlenderHint",
     "Like an IK rig in Blender: bones as octahedra, circles around hips and spine, squares at hands and feet, circles as pole targets with a dashed line, a circle on the ground. A bone is its own FK control.",
     "Wie ein IK-Rig in Blender: Knochen als Oktaeder, Kreise um Becken und Wirbel, Quadrate an Händen und Füßen, Kreise als Pol-Ziele mit gestrichelter Linie, ein Kreis auf dem Boden. Ein Knochen ist sein eigener FK-Griff.",
     "类似 Blender 中的 IK 绑定: 骨骼为八面锥, 骨盆和脊柱周围的圆, 手脚处的方形, 带虚线的圆形极向目标, 地面上的圆。骨骼本身就是它的 FK 控制器。",
     "Blender の IK リグ風: ボーンは八面体、骨盤と背骨を囲む円、手足の四角、破線付きの円のポールターゲット、地面の円。ボーンそのものが FK コントロールです。",
     "Como un rig IK en Blender: huesos como octaedros, círculos alrededor de cadera y columna, cuadrados en manos y pies, círculos como objetivos de polo con línea discontinua, un círculo en el suelo. Un hueso es su propio control FK.",
     "Jako rig IK v Blenderu: kosti jako osmistěny, kruhy kolem pánve a páteře, čtverce u rukou a nohou, kruhy jako cíle pólů s čárkovanou čarou, kruh na zemi. Kost je sama svým ovladačem FK."),
    # --- Knochen wie Blender: Display As ---
    ("KdAnzeigenAls", "Display as", "Anzeigen als", "显示为", "表示方法", "Mostrar como", "Zobrazit jako"),
    ("KdAnzeigenAlsHint", "How bones are drawn, as in Blender (Armature > Viewport Display > Display As).",
     "Wie Knochen gezeichnet werden, wie in Blender (Armature > Viewport Display > Display As).",
     "骨骼的绘制方式, 同 Blender (骨架 > 视图显示 > 显示为)。", "ボーンの描き方です。Blender (アーマチュア > ビューポート表示 > 表示方法) と同じです。",
     "Cómo se dibujan los huesos, como en Blender (Esqueleto > Visualización en vistas > Mostrar como).",
     "Jak se kreslí kosti, jako v Blenderu (Armatura > Zobrazení v pohledu > Zobrazit jako)."),
    ("KdArtOktaeder", "Octahedral", "Oktaeder", "八面锥", "八面体", "Octaédrico", "Osmistěn"),
    ("KdArtStick", "Stick", "Stick", "棍形", "スティック", "Varilla", "Tyčinka"),
    ("KdArtBBone", "B-Bone", "B-Bone", "B-骨骼", "Bボーン", "B-Bone", "B-Bone"),
    ("KdArtHuelle", "Envelope", "Hülle", "封套", "エンベロープ", "Envolvente", "Obálka"),
    ("KdArtDraht", "Wire", "Draht", "线框", "ワイヤー", "Alambre", "Drát"),
    ("KdArtOktaederHint", "Bones as octahedral shapes (default in Blender).", "Knochen als Oktaeder (Vorgabe in Blender).", "骨骼显示为八面锥 (Blender 的默认值)。",
     "ボーンを八面体で表示します (Blender の既定)。", "Huesos como octaedros (predeterminado en Blender).", "Kosti jako osmistěny (výchozí v Blenderu)."),
    ("KdArtStickHint", "Bones as simple lines with dots at the joints, always the same width on screen.",
     "Knochen als einfache Linien mit Punkten an den Gelenken, auf dem Schirm immer gleich breit.", "骨骼显示为简单的线条, 关节处有圆点, 在屏幕上宽度不变。",
     "ボーンを関節に点の付いた単純な線で表示します。画面上の太さは常に同じです。", "Huesos como líneas simples con puntos en las articulaciones, siempre del mismo ancho en pantalla.",
     "Kosti jako jednoduché čáry s tečkami v kloubech, na obrazovce vždy stejně široké."),
    ("KdArtBBoneHint", "Bones as boxes of constant width.", "Knochen als Kästen gleicher Breite.", "骨骼显示为等宽的方盒。", "ボーンを一定幅のボックスで表示します。",
     "Huesos como cajas de ancho constante.", "Kosti jako kvádry stálé šířky."),
    ("KdArtHuelleHint", "Bones as extruded spheres: a sphere at each joint, joined along the bone.",
     "Knochen als gezogene Kugeln: an jedem Gelenk eine Kugel, entlang des Knochens verbunden.", "骨骼显示为拉伸的球体: 每个关节一个球, 沿骨骼相连。",
     "ボーンを押し出した球で表示します: 各関節に球があり、ボーンに沿ってつながります。", "Huesos como esferas extruidas: una esfera en cada articulación, unidas a lo largo del hueso.",
     "Kosti jako protažené koule: v každém kloubu koule, spojené podél kosti."),
    ("KdArtDrahtHint", "Bones as thin wires.", "Knochen als dünne Drähte.", "骨骼显示为细线。", "ボーンを細いワイヤーで表示します。", "Huesos como alambres finos.", "Kosti jako tenké dráty."),
    ("KdArtBraucht",
     "For bones drawn as in Blender: operating mode like Blender, or the rig style Blender octahedra. Like 3ds Max, bones are bone boxes; Bone fins switches their fins.",
     "Für Knochen, die wie in Blender gezeichnet werden: Bedienmodus wie Blender oder der Rig-Stil Blender-Oktaeder. Wie 3ds Max sind Knochen Bone-Kästen; Knochenflossen schaltet ihre Flossen.",
     "用于按 Blender 方式绘制的骨骼: 操作模式为同 Blender, 或绑定样式为 Blender 八面锥。同 3ds Max 时骨骼为骨骼方盒; “骨骼鳍”可切换其鳍。",
     "Blender 風に描くボーン用です: 操作モードが Blender 風、またはリグのスタイルが Blender 八面体のとき。3ds Max 風ではボーンはボーンボックスで、「ボーンフィン」でフィンを切り替えます。",
     "Para huesos dibujados como en Blender: modo de manejo como Blender o el estilo de rig Octaedros de Blender. Como 3ds Max, los huesos son cajas; Aletas de hueso cambia sus aletas.",
     "Pro kosti kreslené jako v Blenderu: režim ovládání jako Blender nebo styl rigu Osmistěny Blenderu. Jako 3ds Max jsou kosti kvádry; Ploutve kostí přepínají jejich ploutve."),
    # --- im Vordergrund, Durchsicht ---
    ("KdVorn", "In front", "Im Vordergrund", "在前面", "最前面", "Al frente", "V popředí"),
    ("KdVornHint",
     "Bones and controls are drawn over the figure (Blender: In Front). Off: the figure hides what lies inside or behind it, as in 3ds Max - see-through (Alt+X like 3ds Max, Alt+Z like Blender) then shows it.",
     "Knochen und Griffe liegen über der Figur (Blender: In Front). Aus: die Figur verdeckt, was in oder hinter ihr liegt, wie in 3ds Max - die Durchsicht (Alt+X wie 3ds Max, Alt+Z wie Blender) zeigt es dann.",
     "骨骼和控制器绘制在角色之上 (Blender: 在前面)。关闭: 角色会遮住其内部或后面的部分, 同 3ds Max - 此时可用透明显示 (同 3ds Max 为 Alt+X, 同 Blender 为 Alt+Z) 查看。",
     "ボーンとコントロールをフィギュアの上に描きます (Blender: 最前面)。オフ: 3ds Max と同じく、フィギュアが内側や後ろにあるものを隠します - そのときはシースルー (3ds Max 風は Alt+X、Blender 風は Alt+Z) で見えます。",
     "Los huesos y controles se dibujan sobre la figura (Blender: Al frente). Desactivado: la figura oculta lo que hay dentro o detrás de ella, como en 3ds Max; entonces ver a través (Alt+X como 3ds Max, Alt+Z como Blender) lo muestra.",
     "Kosti a ovladače se kreslí přes postavu (Blender: V popředí). Vypnuto: postava zakryje, co leží v ní nebo za ní, jako ve 3ds Max - průhlednost (Alt+X jako 3ds Max, Alt+Z jako Blender) to pak ukáže."),
    ("KdDurchsicht", "See-through", "Durchsicht", "透明显示", "シースルー", "Ver a través", "Průhlednost"),
    ("KdDurchsichtHint",
     "Like See-Through in 3ds Max (Alt+X): the figure becomes translucent in all views, so that bones and controls inside it show.",
     "Wie See-Through in 3ds Max (Alt+X): die Figur wird in allen Ansichten durchscheinend, damit Knochen und Griffe in ihr zu sehen sind.",
     "同 3ds Max 的透明显示 (Alt+X): 角色在所有视图中变为半透明, 以便看到其内部的骨骼和控制器。",
     "3ds Max のシースルー (Alt+X) と同じ: すべてのビューでフィギュアが半透明になり、内側のボーンとコントロールが見えます。",
     "Como See-Through en 3ds Max (Alt+X): la figura se vuelve translúcida en todas las vistas para que se vean los huesos y controles de su interior.",
     "Jako See-Through ve 3ds Max (Alt+X): postava je ve všech pohledech průsvitná, takže jsou vidět kosti a ovladače uvnitř."),
    ("KdRoentgen", "X-ray", "Röntgen", "透视模式", "透過表示", "Rayos X", "Rentgen"),
    ("KdRoentgenHint",
     "Like Toggle X-Ray in Blender (Alt+Z): this view shows the figure translucent (opacity 0.5); bones and controls stand in front of it.",
     "Wie Toggle X-Ray in Blender (Alt+Z): diese Ansicht zeigt die Figur durchscheinend (Deckkraft 0,5); Knochen und Griffe stehen davor.",
     "同 Blender 的切换透视模式 (Alt+Z): 此视图以半透明显示角色 (不透明度 0.5); 骨骼和控制器位于其前面。",
     "Blender の透過表示の切り替え (Alt+Z) と同じ: このビューでフィギュアを半透明 (不透明度 0.5) で表示し、ボーンとコントロールはその手前に出ます。",
     "Como Alternar rayos X en Blender (Alt+Z): esta vista muestra la figura translúcida (opacidad 0,5); los huesos y controles quedan delante.",
     "Jako Toggle X-Ray v Blenderu (Alt+Z): tento pohled ukazuje postavu průsvitně (krytí 0,5); kosti a ovladače stojí před ní."),
    ("KdTippMax", "Alt+X switches see-through for the figure. The style changes only the look.", "Alt+X schaltet die Durchsicht der Figur. Der Stil ändert nur das Aussehen.",
     "Alt+X 切换角色的透明显示。样式只改变外观。", "Alt+X でフィギュアのシースルーを切り替えます。スタイルで変わるのは見た目だけです。",
     "Alt+X cambia ver a través de la figura. El estilo solo cambia el aspecto.", "Alt+X přepíná průhlednost postavy. Styl mění jen vzhled."),
    ("KdTippBl", "Alt+Z switches X-ray for the active view. The style changes only the look.", "Alt+Z schaltet Röntgen für die aktive Ansicht. Der Stil ändert nur das Aussehen.",
     "Alt+Z 切换活动视图的透视模式。样式只改变外观。", "Alt+Z でアクティブなビューの透過表示を切り替えます。スタイルで変わるのは見た目だけです。",
     "Alt+Z cambia los rayos X de la vista activa. El estilo solo cambia el aspecto.", "Alt+Z přepíná rentgen aktivního pohledu. Styl mění jen vzhled."),
    ("KdOrt", "Right panel in the area Animation: Bone and rig display. Also in the view header and in the right-click menu of the view.",
     "Rechtes Feld im Bereich Animation: Knochen- und Rig-Darstellung. Auch in der Kopfzeile der Ansicht und im Rechtsklick-Menü der Ansicht.",
     "动画区域的右侧面板: 骨骼和绑定的显示。也在视图标题栏和视图的右键菜单中。",
     "アニメーションエリアの右パネル: ボーンとリグの表示。ビューのヘッダーとビューの右クリックメニューにもあります。",
     "Panel derecho del área Animación: Visualización de huesos y rig. También en la cabecera de la vista y en el menú del clic derecho de la vista.",
     "Pravý panel v oblasti Animace: Zobrazení kostí a rigu. Také v záhlaví pohledu a v nabídce pravého tlačítka pohledu."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
