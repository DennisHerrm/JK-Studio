"""ENTSCHEID3B (13.11., plaene/entscheidungen_1211.md B14-B17, C1, C3): die Seite "Model-Tab: IK/FK ueberblenden, Plattform und
Rig-Vorlage" - die IK/FK-Spur je Glied wie CAT (zwischen zwei Schluesseln ueberblendet, im Dopesheet), der Griff der Wurzel
(Bodenkreis/Bodenform wie CATs Plattform), Control Rig erstellen nach nur FK, die Rig-Vorlage mit Rueckgaengig, Ducken und das Tempo
des Generators als Faktor.

Eigene Quelle; im Handbuch-Register (tools/handbuch/inhalt_modell.py, QUELLEN) eine Zeile `Seite("inhalt_ent3b")` hinter
inhalt_ap13k. Die Bilder h_ent3b_ikfk und h_ent3b_wurzel nimmt der Selbsttest auf (BHED_EDITORTEST=handbuch, BHED_HB_NUR=ent3b).
Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs); {{Kennung}} setzt den Text der Oberflaeche ein.
"""

X = {
    "titel": ("Model tab: IK/FK blend, root platform and rig template", "Model-Tab: IK/FK überblenden, Plattform und Rig-Vorlage",
              "模型选项卡: IK/FK 过渡、根平台和绑定模板", "モデルタブ: IK/FK のブレンド、ルートのプラットフォーム、リグテンプレート",
              "Pestaña Model: mezcla IK/FK, plataforma raíz y plantilla de rig", "Karta Model: přechod IK/FK, plošina kořene a šablona rigu"),
    "kurz": (
        "Like a CAT rig, every limb has its own IK/FK track that blends between two keys, the ground circle moves and turns the whole figure, and the rig template, the FK-only rig and the generated walks behave as you would expect from 3ds Max and Blender.",
        "Wie bei einem CAT-Rig hat jedes Glied eine eigene IK/FK-Spur, die zwischen zwei Schlüsseln überblendet, der Bodenkreis verschiebt und dreht die ganze Figur, und Rig-Vorlage, Rig nur mit FK und erzeugte Gänge verhalten sich, wie man es aus 3ds Max und Blender erwartet.",
        "与 CAT 绑定相同, 每个肢体都有自己的 IK/FK 轨道, 在两个关键帧之间过渡; 地面圆环可移动和旋转整个角色; 绑定模板、仅 FK 绑定和生成的步态的行为与 3ds Max 和 Blender 中的预期一致。",
        "CAT リグと同じく、手足ごとに 2 つのキーの間でブレンドする IK/FK トラックがあり、地面の円がフィギュア全体を移動・回転し、リグテンプレート、FK のみのリグ、生成した歩きは 3ds Max や Blender で期待するとおりに動きます。",
        "Como en un rig de CAT, cada extremidad tiene su propia pista IK/FK que se mezcla entre dos claves, el círculo del suelo mueve y gira toda la figura, y la plantilla de rig, el rig solo FK y las marchas generadas se comportan como se espera de 3ds Max y Blender.",
        "Jako u rigu CAT má každá končetina vlastní stopu IK/FK, která přechází mezi dvěma klíči, kruh na zemi posouvá a otáčí celou postavu a šablona rigu, rig jen s FK a generované chůze se chovají, jak to znáte z 3ds Max a Blenderu."),
    "bild_ikfk": (
        "The dope sheet of the area Animation: one row **IK/FK** per limb. The arm on the right blends from IK (key at frame 5) to FK (key at frame 15).",
        "Das Dopesheet des Bereichs Animation: je Glied eine Zeile **IK/FK**. Der rechte Arm blendet von IK (Schlüssel an Frame 5) nach FK (Schlüssel an Frame 15) über.",
        "动画区域的摄影表: 每个肢体一行 **IK/FK**。右臂从 IK (第 5 帧的关键帧) 过渡到 FK (第 15 帧的关键帧)。",
        "アニメーションエリアのドープシート: 手足ごとに **IK/FK** の行があります。右腕は IK (フレーム 5 のキー) から FK (フレーム 15 のキー) へブレンドします。",
        "La hoja de claves del área Animación: una fila **IK/FK** por extremidad. El brazo derecho se mezcla de IK (clave en el fotograma 5) a FK (clave en el fotograma 15).",
        "Dope sheet oblasti Animace: pro každou končetinu řádek **IK/FK**. Pravá paže přechází z IK (klíč ve snímku 5) do FK (klíč ve snímku 15)."),
    "h_ikfk": ("IK/FK per limb like CAT", "IK/FK je Glied wie CAT", "与 CAT 相同的每肢体 IK/FK", "CAT と同じ手足ごとの IK/FK", "IK/FK por extremidad como CAT",
               "IK/FK pro každou končetinu jako CAT"),
    "ikfk": (
        "Every arm and leg has its own IK/FK track, like the IK/FK ratio of a CAT limb: 1 is IK, 0 is FK. Between two keys of the track the limb blends over, with the interpolation of the key on the left (Bezier like Max and Blender; Constant gives a hard switch). "
        "Like Blender the slider of the limb in **{{MbIkRollout}}** sets a key at the current frame; like 3ds Max **{{E3IkFkAnteil}}** under the selected limb does the same as CAT's spinner. The button IK/FK still switches from the current frame up to the next switch and matches the other side first. "
        "In the dope sheet each track has a row **IK/FK** with its keys: click, drag, delete and change the interpolation like any other key - drag the key in front of a switch to the left to blend over more frames. "
        "Before you blend, match IK and FK (CAT: Match IK and FK) - after **{{AzRigErstellen}}** they already match at every frame, then nothing moves while the limb blends. Older mission files load unchanged: their IK/FK value per IK key becomes keys with Constant interpolation, so every frame looks as before.",
        "Jeder Arm und jedes Bein hat eine eigene IK/FK-Spur, wie das IK/FK-Verhältnis eines CAT-Glieds: 1 ist IK, 0 ist FK. Zwischen zwei Schlüsseln der Spur blendet das Glied über, mit der Interpolation des linken Schlüssels (Bezier wie Max und Blender; Konstant schaltet hart um). "
        "Wie Blender setzt der Regler des Glieds in **{{MbIkRollout}}** einen Schlüssel am aktuellen Frame; wie 3ds Max tut **{{E3IkFkAnteil}}** unter dem gewählten Glied dasselbe wie der Regler von CAT. Der Knopf IK/FK schaltet weiterhin ab dem aktuellen Frame bis zum nächsten Wechsel um und gleicht vorher die andere Seite an. "
        "Im Dopesheet hat jede Spur eine Zeile **IK/FK** mit ihren Schlüsseln: anklicken, ziehen, löschen und die Interpolation ändern wie bei jedem anderen Schlüssel - den Schlüssel vor einem Wechsel nach links ziehen, um über mehr Frames überzublenden. "
        "Vor dem Überblenden IK und FK angleichen (CAT: Match IK and FK) - nach **{{AzRigErstellen}}** sind sie an jedem Frame schon gleich, dann bewegt sich beim Überblenden nichts. Ältere Missions-Dateien laden unverändert: ihr IK/FK-Wert je IK-Schlüssel wird zu Schlüsseln mit Interpolation Konstant, jeder Frame sieht aus wie vorher.",
        "每条手臂和腿都有自己的 IK/FK 轨道, 与 CAT 肢体的 IK/FK 比例相同: 1 为 IK, 0 为 FK。在轨道的两个关键帧之间肢体会过渡, 使用左侧关键帧的插值 (与 Max 和 Blender 相同为贝塞尔; 常量则硬切换)。"
        "同 Blender 时, **{{MbIkRollout}}** 中肢体的滑块在当前帧设置关键帧; 同 3ds Max 时, 所选肢体下方的 **{{E3IkFkAnteil}}** 的作用与 CAT 的微调器相同。IK/FK 按钮仍然从当前帧切换到下一次切换, 并先匹配另一侧。"
        "在摄影表中每条轨道有一行 **IK/FK** 及其关键帧: 像其他关键帧一样单击、拖动、删除和更改插值 - 将切换前的关键帧向左拖动即可在更多帧上过渡。"
        "过渡之前先匹配 IK 和 FK (CAT: Match IK and FK) - 在 **{{AzRigErstellen}}** 之后它们在每一帧都已一致, 过渡时不会移动。旧的任务文件不变地加载: 每个 IK 关键帧中的 IK/FK 值变为常量插值的关键帧, 每一帧看起来都与以前相同。",
        "腕と脚にはそれぞれ CAT の手足の IK/FK 比率と同じ IK/FK トラックがあります: 1 が IK、0 が FK です。トラックの 2 つのキーの間で手足は左のキーの補間でブレンドします (Max や Blender と同じベジェ。一定にするとはっきり切り替わります)。"
        "Blender 風では **{{MbIkRollout}}** の手足のスライダーが現在のフレームにキーを打ちます。3ds Max 風では選択した手足の下の **{{E3IkFkAnteil}}** が CAT のスピナーと同じことをします。IK/FK ボタンはこれまでどおり現在のフレームから次の切り替えまで切り替え、先にもう一方を合わせます。"
        "ドープシートでは各トラックに **IK/FK** の行があり、そのキーを他のキーと同じくクリック、ドラッグ、削除、補間の変更ができます - 切り替えの前のキーを左にドラッグすると、より多くのフレームでブレンドします。"
        "ブレンドの前に IK と FK を合わせます (CAT: Match IK and FK) - **{{AzRigErstellen}}** の後は各フレームですでに一致しているので、ブレンド中も何も動きません。古いミッションファイルはそのまま読み込めます: IK キーごとの IK/FK 値は補間が一定のキーになり、どのフレームも以前と同じに見えます。",
        "Cada brazo y pierna tiene su propia pista IK/FK, como la proporción IK/FK de una extremidad de CAT: 1 es IK, 0 es FK. Entre dos claves de la pista la extremidad se mezcla, con la interpolación de la clave de la izquierda (Bezier como Max y Blender; Constante cambia en seco). "
        "Como Blender, el deslizador de la extremidad en **{{MbIkRollout}}** pone una clave en el fotograma actual; como 3ds Max, **{{E3IkFkAnteil}}** bajo la extremidad elegida hace lo mismo que el control de CAT. El botón IK/FK sigue cambiando desde el fotograma actual hasta el siguiente cambio e iguala antes el otro lado. "
        "En la hoja de claves cada pista tiene una fila **IK/FK** con sus claves: pulsar, arrastrar, borrar y cambiar la interpolación como cualquier otra clave; arrastra a la izquierda la clave anterior a un cambio para mezclar durante más fotogramas. "
        "Antes de mezclar, iguala IK y FK (CAT: Match IK and FK); después de **{{AzRigErstellen}}** ya coinciden en cada fotograma y no se mueve nada al mezclar. Los archivos de misión antiguos se cargan sin cambios: su valor IK/FK por clave IK se convierte en claves con interpolación Constante y cada fotograma se ve como antes.",
        "Každá paže a noha má vlastní stopu IK/FK jako poměr IK/FK končetiny CAT: 1 je IK, 0 je FK. Mezi dvěma klíči stopy končetina přechází s interpolací levého klíče (Bezier jako Max a Blender; Konstantní přepne naostro). "
        "Jako Blender nastaví posuvník končetiny v **{{MbIkRollout}}** klíč v aktuálním snímku; jako 3ds Max dělá **{{E3IkFkAnteil}}** pod vybranou končetinou totéž co číselník CAT. Tlačítko IK/FK dál přepíná od aktuálního snímku do další změny a předtím srovná druhou stranu. "
        "V dope sheetu má každá stopa řádek **IK/FK** se svými klíči: klikat, táhnout, mazat a měnit interpolaci jako u každého jiného klíče - klíč před přepnutím táhněte doleva, aby přechod trval více snímků. "
        "Před přechodem srovnejte IK a FK (CAT: Match IK and FK) - po **{{AzRigErstellen}}** se shodují v každém snímku, takže se při přechodu nic nepohne. Starší soubory misí se načtou beze změny: jejich hodnota IK/FK u každého klíče IK se stane klíči s interpolací Konstantní a každý snímek vypadá jako dřív."),
    "bild_wurzel": (
        "Like 3ds Max, style Control rig: the ground circle is selected - the gizmo moves the whole figure, the hands and feet go along with their IK targets.",
        "Wie 3ds Max, Stil Control Rig: der Bodenkreis ist gewählt - das Gizmo verschiebt die ganze Figur, Hände und Füße gehen mit ihren IK-Zielen mit.",
        "同 3ds Max, 控制绑定样式: 已选择地面圆环 - 操纵器移动整个角色, 手脚随其 IK 目标一起移动。",
        "3ds Max 風、スタイル コントロールリグ: 地面の円を選択 - ギズモでフィギュア全体が動き、手足も IK ターゲットと一緒に動きます。",
        "Como 3ds Max, estilo rig de control: el círculo del suelo está seleccionado; el gizmo mueve toda la figura y las manos y los pies van con sus objetivos IK.",
        "Jako 3ds Max, styl ovládací rig: kruh na zemi je vybraný - gizmo posune celou postavu, ruce a nohy jdou se svými cíli IK."),
    "h_wurzel": ("The ground circle moves the whole figure", "Der Bodenkreis bewegt die ganze Figur", "地面圆环移动整个角色", "地面の円でフィギュア全体を動かす",
                 "El círculo del suelo mueve toda la figura", "Kruh na zemi pohne celou postavou"),
    "wurzel": (
        "In the rig styles **{{KdStilControl}}**, **{{KdStilCat}}** and **{{KdStilBlender}}** the circle or shape on the ground is the control **{{E3GriffWurzel}}**, like CAT's platform (CATParent) and Rigify's root: click it, then move it (W like 3ds Max, G like Blender) or turn it (E, R) - the whole figure goes along, "
        "with the IK targets of hands and feet in world space, so nothing stretches. It sets a key at the current frame (one key moves the whole animation, more keys animate the platform), it has its own row in the dope sheet, and Ctrl+Z takes it back. "
        "**{{MbRgSchluesselLoeschen}}** deletes its key at the current frame. The mark above the head stays decoration. Baking into a GLA writes the platform into the root bone.",
        "In den Rig-Stilen **{{KdStilControl}}**, **{{KdStilCat}}** und **{{KdStilBlender}}** ist der Kreis bzw. die Form am Boden der Griff **{{E3GriffWurzel}}**, wie CATs Plattform (CATParent) und die Wurzel von Rigify: anklicken, dann verschieben (W wie 3ds Max, G wie Blender) oder drehen (E, R) - die ganze Figur geht mit, "
        "auch die IK-Ziele von Händen und Füßen im Raum Welt, damit sich nichts streckt. Er setzt einen Schlüssel am aktuellen Frame (ein Schlüssel verschiebt die ganze Animation, mehrere Schlüssel animieren die Plattform), hat eine eigene Zeile im Dopesheet, und Strg+Z nimmt ihn zurück. "
        "**{{MbRgSchluesselLoeschen}}** löscht seinen Schlüssel am aktuellen Frame. Die Marke über dem Kopf bleibt Schmuck. Das Backen in eine GLA schreibt die Plattform in den Wurzelknochen.",
        "在绑定样式 **{{KdStilControl}}**、**{{KdStilCat}}** 和 **{{KdStilBlender}}** 中, 地面上的圆环或形状是控制器 **{{E3GriffWurzel}}**, 与 CAT 的平台 (CATParent) 和 Rigify 的根相同: 单击它, 然后移动 (同 3ds Max 用 W, 同 Blender 用 G) 或旋转 (E、R) - 整个角色一起移动, "
        "包括世界空间中手脚的 IK 目标, 因此不会拉伸。它在当前帧设置关键帧 (一个关键帧移动整个动画, 多个关键帧使平台动画化), 在摄影表中有自己的一行, Ctrl+Z 可撤销。"
        "**{{MbRgSchluesselLoeschen}}** 删除其在当前帧的关键帧。头顶的标记仍是装饰。烘焙到 GLA 时会把平台写入根骨骼。",
        "リグのスタイル **{{KdStilControl}}**、**{{KdStilCat}}**、**{{KdStilBlender}}** では、地面の円や図形が CAT のプラットフォーム (CATParent) や Rigify のルートと同じコントロール **{{E3GriffWurzel}}** です: クリックして移動 (3ds Max 風は W、Blender 風は G) または回転 (E、R) すると、フィギュア全体が一緒に動きます。"
        "ワールド空間の手足の IK ターゲットも動くので、何も伸びません。現在のフレームにキーを打ち (1 つのキーでアニメーション全体が動き、複数のキーでプラットフォームをアニメーションできます)、ドープシートに専用の行があり、Ctrl+Z で戻せます。"
        "**{{MbRgSchluesselLoeschen}}** は現在のフレームのキーを削除します。頭の上のマークは飾りのままです。GLA へのベイクではプラットフォームがルートボーンに書き込まれます。",
        "En los estilos de rig **{{KdStilControl}}**, **{{KdStilCat}}** y **{{KdStilBlender}}** el círculo o la forma del suelo es el control **{{E3GriffWurzel}}**, como la plataforma de CAT (CATParent) y la raíz de Rigify: púlsalo y luego muévelo (W como 3ds Max, G como Blender) o gíralo (E, R); toda la figura va con él, "
        "también los objetivos IK de manos y pies en espacio del mundo, así que nada se estira. Pone una clave en el fotograma actual (una clave mueve toda la animación, varias claves animan la plataforma), tiene su propia fila en la hoja de claves y Ctrl+Z lo deshace. "
        "**{{MbRgSchluesselLoeschen}}** borra su clave en el fotograma actual. La marca sobre la cabeza sigue siendo decoración. Al hornear en una GLA la plataforma se escribe en el hueso raíz.",
        "Ve stylech rigu **{{KdStilControl}}**, **{{KdStilCat}}** a **{{KdStilBlender}}** je kruh nebo tvar na zemi ovladač **{{E3GriffWurzel}}**, jako plošina CAT (CATParent) a kořen Rigify: klikněte na něj a pak ho posuňte (W jako 3ds Max, G jako Blender) nebo otočte (E, R) - celá postava jde s ním, "
        "i cíle IK rukou a nohou v prostoru světa, takže se nic nenatahuje. Nastaví klíč v aktuálním snímku (jeden klíč posune celou animaci, více klíčů plošinu animuje), má vlastní řádek v dope sheetu a Ctrl+Z ho vrátí. "
        "**{{MbRgSchluesselLoeschen}}** smaže jeho klíč v aktuálním snímku. Značka nad hlavou zůstává ozdobou. Zapečení do GLA zapíše plošinu do kořenové kosti."),
    "h_rig": ("Rig template and FK-only rig", "Rig-Vorlage und Rig nur mit FK", "绑定模板与仅 FK 绑定", "リグテンプレートと FK のみのリグ", "Plantilla de rig y rig solo FK",
              "Šablona rigu a rig jen s FK"),
    "rig": (
        "The choice of the rig template is part of the model document: Ctrl+Z and Ctrl+Y take a change of the template back and forth, and the rig follows. The chain mapping of the retargeting you set for a template stays remembered when you switch to another template and back. "
        "**{{AzRigErstellen}}** after **{{AzRigFk}}** puts the limbs back on IK in every sequence: their FK keys go to IK targets and poles like the first time, the motion stays the same, and one Ctrl+Z takes everything back.",
        "Die Wahl der Rig-Vorlage gehört zum Modelldokument: Strg+Z und Strg+Y nehmen einen Wechsel der Vorlage zurück und wieder vor, das Rig folgt. Die Zuordnung der Ketten beim Übertragen, die du für eine Vorlage gesetzt hast, bleibt gemerkt, wenn du zu einer anderen Vorlage und zurück wechselst. "
        "**{{AzRigErstellen}}** nach **{{AzRigFk}}** stellt die Glieder in jeder Sequenz wieder auf IK: ihre FK-Schlüssel gehen wie beim ersten Erstellen auf IK-Ziele und Pole, die Bewegung bleibt gleich, und ein Strg+Z nimmt alles zurück.",
        "绑定模板的选择属于模型文档: Ctrl+Z 和 Ctrl+Y 可撤销和重做模板的更改, 绑定随之更新。你为某个模板设置的重定向链映射在切换到其他模板再切回时仍会保留。"
        "在 **{{AzRigFk}}** 之后使用 **{{AzRigErstellen}}** 会在每个序列中把肢体重新设为 IK: 其 FK 关键帧像首次创建时一样转到 IK 目标和极向, 动作保持不变, 一次 Ctrl+Z 即可全部撤销。",
        "リグテンプレートの選択はモデルドキュメントの一部です: Ctrl+Z と Ctrl+Y でテンプレートの変更を戻したりやり直したりでき、リグも追従します。テンプレートに設定したリターゲットのチェーン対応は、別のテンプレートに切り替えて戻しても保持されます。"
        "**{{AzRigFk}}** の後の **{{AzRigErstellen}}** は、すべてのシーケンスで手足を再び IK にします: FK キーは最初の作成時と同じく IK ターゲットとポールに移り、動きは変わらず、1 回の Ctrl+Z ですべて戻ります。",
        "La elección de la plantilla de rig forma parte del documento del modelo: Ctrl+Z y Ctrl+Y deshacen y rehacen un cambio de plantilla, y el rig lo sigue. La asignación de cadenas del retargeting que pusiste para una plantilla se recuerda al cambiar a otra plantilla y volver. "
        "**{{AzRigErstellen}}** después de **{{AzRigFk}}** vuelve a poner las extremidades en IK en cada secuencia: sus claves FK pasan a objetivos IK y polos como la primera vez, el movimiento queda igual y un Ctrl+Z lo deshace todo.",
        "Volba šablony rigu patří do dokumentu modelu: Ctrl+Z a Ctrl+Y vrátí a zopakují změnu šablony a rig ji následuje. Přiřazení řetězců při přenosu, které jste pro šablonu nastavili, zůstane zapamatované, když přepnete na jinou šablonu a zpět. "
        "**{{AzRigErstellen}}** po **{{AzRigFk}}** vrátí končetiny ve všech sekvencích na IK: jejich klíče FK přejdou jako při prvním vytvoření na cíle IK a póly, pohyb zůstane stejný a jedno Ctrl+Z vrátí vše."),
    "h_presets": ("Crouch and the speed of the generator", "Ducken und das Tempo des Generators", "蹲伏与生成器的速度", "しゃがみとジェネレーターの速度",
                  "Agachado y la velocidad del generador", "Přikrčení a rychlost generátoru"),
    "presets": (
        "**{{AzKatDucken}}** holds Crouch idle and Crouch walk (Crouch walk goes to BOTH_CROUCH1WALK, the slot the game plays while crouching). **{{PrTempo}}** of Walk (generated) and Run (generated) is a factor like in the design: 1.00× is the speed of the NPCs of the game - 90 units per second walking, 300 running; "
        "the tooltip names the units per second. The same factor gives the same motion as before.",
        "**{{AzKatDucken}}** enthält Crouch idle und Crouch walk (Crouch walk geht auf BOTH_CROUCH1WALK, den Platz, den das Spiel beim Ducken spielt). **{{PrTempo}}** von Gehen (erzeugt) und Laufen (erzeugt) ist ein Faktor wie im Entwurf: 1,00× ist das Tempo der NPCs des Spiels - 90 Einheiten je Sekunde beim Gehen, 300 beim Laufen; "
        "der Tooltip nennt die Einheiten je Sekunde. Derselbe Faktor gibt dieselbe Bewegung wie vorher.",
        "**{{AzKatDucken}}** 包含 Crouch idle 和 Crouch walk (Crouch walk 放到 BOTH_CROUCH1WALK, 即游戏在蹲伏时播放的槽位)。行走 (生成) 和跑步 (生成) 的 **{{PrTempo}}** 与设计中一样是倍数: 1.00× 是游戏 NPC 的速度 - 步行每秒 90 单位, 跑步 300; "
        "工具提示会给出每秒单位数。相同的倍数得到与以前相同的动作。",
        "**{{AzKatDucken}}** には Crouch idle と Crouch walk があります (Crouch walk はゲームがしゃがみ中に再生するスロット BOTH_CROUCH1WALK に入ります)。歩き (生成) と走り (生成) の **{{PrTempo}}** はデザインどおり倍率です: 1.00× はゲームの NPC の速度 - 歩きで毎秒 90 ユニット、走りで 300。"
        "ツールチップに毎秒のユニット数が出ます。同じ倍率なら以前と同じ動きになります。",
        "**{{AzKatDucken}}** contiene Crouch idle y Crouch walk (Crouch walk va a BOTH_CROUCH1WALK, el hueco que el juego reproduce al agacharse). **{{PrTempo}}** de Andar (generado) y Correr (generado) es un factor como en el diseño: 1,00× es la velocidad de los NPC del juego, 90 unidades por segundo al andar y 300 al correr; "
        "la información emergente da las unidades por segundo. El mismo factor da el mismo movimiento que antes.",
        "**{{AzKatDucken}}** obsahuje Crouch idle a Crouch walk (Crouch walk jde na BOTH_CROUCH1WALK, pozici, kterou hra hraje při přikrčení). **{{PrTempo}}** u Chůze (generovaná) a Běh (generovaný) je násobek jako v návrhu: 1,00× je rychlost NPC hry - 90 jednotek za sekundu při chůzi, 300 při běhu; "
        "popisek uvádí jednotky za sekundu. Stejný násobek dává stejný pohyb jako dřív."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    z = X[schluessel]
    return T(z[0], z[1], z[2], z[3])


def seite():
    """Die Seite modell-ent3b (Gruppe Missionen, hinter modell-knochen)."""
    return {
        "id": "modell-ent3b",
        "gruppe": "mission",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("img", "h_ent3b_ikfk", _t("bild_ikfk"), "breit"),
            ("h2", "ikfk", _t("h_ikfk")),
            ("p", _t("ikfk")),
            ("img", "h_ent3b_wurzel", _t("bild_wurzel"), "breit"),
            ("h2", "wurzel", _t("h_wurzel")),
            ("p", _t("wurzel")),
            ("h2", "rig", _t("h_rig")),
            ("p", _t("rig")),
            ("h2", "presets", _t("h_presets")),
            ("p", _t("presets")),
        ],
    }
