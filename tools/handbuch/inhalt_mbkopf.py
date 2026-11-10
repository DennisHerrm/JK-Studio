"""MBKOPF (rc612): der Abschnitt "Kopfzeile der Ansicht, ViewCube und Gizmo" der Seite modell-ansehen.

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_mb1a1.py.
"""

X = {
    "h_kopf": ("View header, ViewCube and gizmo", "Kopfzeile der Ansicht, ViewCube und Gizmo", "视图标题栏、ViewCube 与 Gizmo",
               "ビューのヘッダー、ViewCube とギズモ", "Cabecera de la vista, ViewCube y gizmo", "Záhlaví pohledu, ViewCube a gizmo"),
    "kopf_max": (
        "Every view has its header at the top left, like the viewport label menus of 3ds Max - each field lights up under the mouse and opens its "
        "menu with a left or right click. **[+]**: **{{MkMaximieren}}** (<kbd>Alt</kbd>+<kbd>W</kbd>), **{{MkEinpassen}}** (<kbd>Z</kbd>), what "
        "the view shows (grid <kbd>G</kbd>, floor, origin, bounding box, selection bracket <kbd>J</kbd>, statistics <kbd>7</kbd>, height ruler, "
        "plain background), the submenu **{{MkKnochenPruefen}}** (bones, fins, normals, tags, crack finder, vertex and triangle numbers), "
        "**{{MkViewCube}}** and the field of view. **[{{MbAnsichtPersp}}]**: the point of view with the keys of 3ds Max (P, U, T, B, F, L) and "
        "**{{MkStartansicht}}**. **[{{MbAnsichtStandard}}]**: the shading of this view - wireframe (<kbd>F3</kbd>), edged faces "
        "(<kbd>F4</kbd>), two-sided, white textures, vertex weights, silhouette. In the four views every view keeps its own shading, grid and "
        "statistics. The same menus are in the right-click menu of the view, and **{{MbAktionen}}** (<kbd>X</kbd>) finds every switch; its "
        "tooltip says where it is. Switches that need a selected surface (numbers, crack finder) say so in the view until you select one.",
        "Jede Ansicht hat oben links ihre Kopfzeile wie die Viewport Label Menus von 3ds Max - jedes Feld leuchtet unter der Maus auf und "
        "öffnet mit Klick links oder rechts sein Menü. **[+]**: **{{MkMaximieren}}** (<kbd>Alt</kbd>+<kbd>W</kbd>), **{{MkEinpassen}}** "
        "(<kbd>Z</kbd>), was die Ansicht zeigt (Raster <kbd>G</kbd>, Boden, Ursprung, Hüllkasten, Auswahlklammer <kbd>J</kbd>, Statistik "
        "<kbd>7</kbd>, Höhenlineal, einfarbiger Hintergrund), das Untermenü **{{MkKnochenPruefen}}** (Knochen, Flossen, Normalen, Tags, Risse "
        "finden, Ecken- und Dreieck-Nummern), **{{MkViewCube}}** und der Blickwinkel. **[{{MbAnsichtPersp}}]**: der Blickpunkt mit den Tasten "
        "von 3ds Max (P, U, T, B, F, L) und **{{MkStartansicht}}**. **[{{MbAnsichtStandard}}]**: die Schattierung dieser Ansicht - "
        "Drahtgitter (<kbd>F3</kbd>), Kanten über Flächen (<kbd>F4</kbd>), zweiseitig, weiße Texturen, Gewichte, Silhouette. In den vier "
        "Ansichten behält jede ihre eigene Schattierung, ihr Raster und ihre Statistik. Dieselben Menüs hat das Rechtsklick-Menü der Ansicht, "
        "und **{{MbAktionen}}** (<kbd>X</kbd>) findet jeden Schalter; sein Tooltip nennt den Ort. Schalter, die eine gewählte Fläche brauchen "
        "(Nummern, Risse finden), sagen das in der Ansicht, bis eine gewählt ist.",
        "每个视图左上角都有标题栏, 如 3ds Max 的视口标签菜单——鼠标悬停时字段高亮, 左键或右键单击打开其菜单。**[+]**: **{{MkMaximieren}}** "
        "(<kbd>Alt</kbd>+<kbd>W</kbd>)、**{{MkEinpassen}}** (<kbd>Z</kbd>)、视图显示的内容 (网格 <kbd>G</kbd>、地面、原点、包围盒、选择框 "
        "<kbd>J</kbd>、统计 <kbd>7</kbd>、高度标尺、纯色背景)、子菜单 **{{MkKnochenPruefen}}** (骨骼、鳍、法线、标记、裂缝查找、顶点与三角形编号)、"
        "**{{MkViewCube}}** 和视野。**[{{MbAnsichtPersp}}]**: 观察点, 使用 3ds Max 的按键 (P、U、T、B、F、L) 以及 **{{MkStartansicht}}**。"
        "**[{{MbAnsichtStandard}}]**: 此视图的着色——线框 (<kbd>F3</kbd>)、边面 (<kbd>F4</kbd>)、双面、白色纹理、顶点权重、剪影。四视图中每个视图"
        "保留自己的着色、网格和统计。视图的右键菜单中有相同的菜单, **{{MbAktionen}}** (<kbd>X</kbd>) 能找到每个开关, 其提示说明它的位置。需要选中面的"
        "开关 (编号、裂缝查找) 在选中面之前会在视图中提示。",
        "各ビューの左上には 3ds Max のビューポートラベルメニューと同じヘッダーがあります。各フィールドはマウスを重ねると明るくなり、左クリックか"
        "右クリックでメニューを開きます。**[+]**: **{{MkMaximieren}}** (<kbd>Alt</kbd>+<kbd>W</kbd>)、**{{MkEinpassen}}** (<kbd>Z</kbd>)、"
        "ビューに表示するもの (グリッド <kbd>G</kbd>、床、原点、バウンディングボックス、選択ブラケット <kbd>J</kbd>、統計 <kbd>7</kbd>、高さ定規、"
        "単色の背景)、サブメニュー **{{MkKnochenPruefen}}** (ボーン、フィン、法線、タグ、クラック検出、頂点番号と三角形番号)、**{{MkViewCube}}**、"
        "視野角。**[{{MbAnsichtPersp}}]**: 3ds Max のキー (P、U、T、B、F、L) で視点を切り替え、**{{MkStartansicht}}** もあります。"
        "**[{{MbAnsichtStandard}}]**: このビューのシェーディング - ワイヤーフレーム (<kbd>F3</kbd>)、エッジ面 (<kbd>F4</kbd>)、両面、白いテクスチャ、"
        "頂点ウェイト、シルエット。4 ビューではビューごとにシェーディング、グリッド、統計を持ちます。ビューの右クリックメニューにも同じメニューがあり、"
        "**{{MbAktionen}}** (<kbd>X</kbd>) で各スイッチを検索でき、ツールチップに場所が表示されます。選択サーフェスが必要なスイッチ (番号、クラック検出) "
        "は、選択するまでビューにその旨を表示します。",
        "Cada vista tiene arriba a la izquierda su cabecera, como los menús de etiqueta de los visores de 3ds Max: cada campo se ilumina bajo el "
        "ratón y abre su menú con clic izquierdo o derecho. **[+]**: **{{MkMaximieren}}** (<kbd>Alt</kbd>+<kbd>W</kbd>), **{{MkEinpassen}}** "
        "(<kbd>Z</kbd>), lo que muestra la vista (rejilla <kbd>G</kbd>, suelo, origen, caja envolvente, corchetes de selección <kbd>J</kbd>, "
        "estadísticas <kbd>7</kbd>, regla de altura, fondo de un color), el submenú **{{MkKnochenPruefen}}** (huesos, aletas, normales, tags, "
        "buscar grietas, números de vértices y triángulos), **{{MkViewCube}}** y el campo de visión. **[{{MbAnsichtPersp}}]**: el punto de vista "
        "con las teclas de 3ds Max (P, U, T, B, F, L) y **{{MkStartansicht}}**. **[{{MbAnsichtStandard}}]**: el sombreado de esta vista: alambre "
        "(<kbd>F3</kbd>), caras con aristas (<kbd>F4</kbd>), doble cara, texturas blancas, pesos de vértices, silueta. En las cuatro vistas cada "
        "una conserva su sombreado, su rejilla y sus estadísticas. El menú contextual de la vista tiene los mismos menús, y **{{MbAktionen}}** "
        "(<kbd>X</kbd>) encuentra cada interruptor; su información emergente dice dónde está. Los que necesitan una superficie seleccionada "
        "(números, buscar grietas) lo indican en la vista hasta que eliges una.",
        "Každý pohled má vlevo nahoře záhlaví jako nabídky popisků pohledů v 3ds Max - každé pole se pod myší rozsvítí a levým nebo pravým "
        "tlačítkem otevře svou nabídku. **[+]**: **{{MkMaximieren}}** (<kbd>Alt</kbd>+<kbd>W</kbd>), **{{MkEinpassen}}** (<kbd>Z</kbd>), co "
        "pohled ukazuje (mřížka <kbd>G</kbd>, podlaha, počátek, obalový kvádr, závorky výběru <kbd>J</kbd>, statistika <kbd>7</kbd>, výškové "
        "pravítko, jednobarevné pozadí), podnabídka **{{MkKnochenPruefen}}** (kosti, ploutve, normály, tagy, hledání prasklin, čísla vrcholů a "
        "trojúhelníků), **{{MkViewCube}}** a zorné pole. **[{{MbAnsichtPersp}}]**: hledisko s klávesami 3ds Max (P, U, T, B, F, L) a "
        "**{{MkStartansicht}}**. **[{{MbAnsichtStandard}}]**: stínování tohoto pohledu - drátový model (<kbd>F3</kbd>), plochy s hranami "
        "(<kbd>F4</kbd>), oboustranné, bílé textury, váhy vrcholů, silueta. Ve čtyřech pohledech si každý ponechá vlastní stínování, mřížku a "
        "statistiku. Stejné nabídky má kontextová nabídka pohledu a **{{MbAktionen}}** (<kbd>X</kbd>) najde každý přepínač; jeho popisek uvádí "
        "místo. Přepínače, které potřebují vybraný povrch (čísla, hledání prasklin), to v pohledu oznámí, dokud žádný nevyberete."),
    "bild_max": ("The view header in 3ds Max mode: the menu [+] with Bones and checks, at the top right the ViewCube.",
                 "Die Kopfzeile im Modus wie 3ds Max: das Menü [+] mit Knochen und Prüfen, oben rechts der ViewCube.",
                 "3ds Max 模式下的视图标题栏: [+] 菜单及骨骼与检查, 右上角为 ViewCube。",
                 "3ds Max モードのヘッダー: [+] メニューとボーンとチェック、右上に ViewCube。",
                 "La cabecera en modo 3ds Max: el menú [+] con Huesos y comprobaciones, arriba a la derecha el ViewCube.",
                 "Záhlaví v režimu 3ds Max: nabídka [+] s Kosti a kontroly, vpravo nahoře ViewCube."),
    "bild_cube": ("The ViewCube under the mouse: the corner lights up, below it the compass ring with N, E, S and W, at the top left Home.",
                  "Der ViewCube unter der Maus: die Ecke leuchtet auf, darunter der Kompassring mit N, O, S und W, oben links Home.",
                  "鼠标下的 ViewCube: 角高亮, 下方是带北、东、南、西的指南针环, 左上角是 Home。",
                  "マウスを重ねた ViewCube: 角が明るくなり、下に北・東・南・西のコンパスリング、左上に Home。",
                  "El ViewCube bajo el ratón: la esquina se ilumina, debajo el anillo de brújula con N, E, S y O, arriba a la izquierda Inicio.",
                  "ViewCube pod myší: roh se rozsvítí, pod ním kompasový kruh se S, V, J a Z, vlevo nahoře Domů."),
    "cube": (
        "The **ViewCube** at the top right of the active view shows where you look from and behaves like the one of 3ds Max. It has 26 targets - 6 faces, 12 edges, 8 corners; the one under the mouse lights up, and a click turns the view there in a short animation. The projection stays: a perspective view stays perspective; in an orthographic view a face gives its view (Front, Top ...), an edge or corner the free **{{MkOrtho}}** view. Dragging the cube orbits. The **compass ring** below it carries N, E, S and W: drag the ring to turn about the vertical axis only, click a letter to look from there. Looking straight at a face, four arrows lead to the neighbouring faces and two curved arrows roll the view by 90 degrees. The house at the top left is **Home**: the camera flies to the figure from the front, centred. A right click offers Home, **{{MkHomeSetzen}}** and **{{MkOrtho}}**. The cube is half transparent until the mouse is over it. A figure looks along -Y in its .glm (the game turns it to +X), so Front shows its face - also after loading a figure, after Alt+W and with reset view; as in 3ds Max north lies behind the Front face. **[+]** > **{{MkViewCube}}** switches the cube or the compass off, shows the cube in all four views and sets the **{{MkKompassNord}}**.",
        "Der **ViewCube** oben rechts in der aktiven Ansicht zeigt, von wo man schaut, und verhält sich wie der von 3ds Max. Er hat 26 Ziele - 6 Flächen, 12 Kanten, 8 Ecken; das Ziel unter der Maus leuchtet auf, ein Klick dreht die Ansicht in einer kurzen Animation dorthin. Die Projektion bleibt: Eine Perspektive bleibt Perspektive; in einer Orthoansicht gibt eine Fläche ihre Ansicht (Vorne, Oben ...), eine Kante oder Ecke die freie Ansicht **{{MkOrtho}}**. Ziehen am Würfel kreist. Der **Kompassring** darunter trägt N, O, S und W: Ziehen am Ring dreht nur um die Hochachse, ein Klick auf einen Buchstaben schaut von dort. Schaut man gerade auf eine Fläche, führen vier Pfeile zu den Nachbarflächen, und zwei gebogene Pfeile rollen die Ansicht um 90 Grad. Das Haus oben links ist **Home**: Die Kamera fliegt zur Figur von vorne, mittig. Ein Rechtsklick bietet Home, **{{MkHomeSetzen}}** und **{{MkOrtho}}**. Der Würfel ist halb durchsichtig, bis die Maus darüber ist. Eine Figur schaut in ihrer .glm nach -Y (das Spiel dreht sie nach +X), darum zeigt Vorne ihr Gesicht - auch nach dem Laden einer Figur, nach Alt+W und beim Zurücksetzen der Ansicht; wie in 3ds Max liegt Norden hinter der Fläche Vorne. **[+]** > **{{MkViewCube}}** schaltet den Würfel oder den Kompass aus, zeigt den Würfel in allen vier Ansichten und stellt den **{{MkKompassNord}}** ein.",
        "活动视图右上角的 **ViewCube** 显示观察方向, 其行为与 3ds Max 的相同。它有 26 个目标——6 个面、12 条边、8 个角; 鼠标下的目标会高亮, 单击后视图以简短动画转到该方向。投影保持不变: 透视视图仍为透视; 在正交视图中, 面切换到其视图 (前、顶 ...), 边或角得到自由的 **{{MkOrtho}}** 视图。拖动立方体可环绕。下方的**指南针环**标有北、东、南、西: 拖动圆环只绕垂直轴旋转, 单击字母则从该方向观察。正对某个面时, 四个箭头通向相邻的面, 两个弯曲箭头将视图滚动 90 度。左上角的房子是 **Home**: 相机飞到角色正面居中的位置。右键菜单提供 Home、**{{MkHomeSetzen}}** 和 **{{MkOrtho}}**。鼠标不在其上时立方体为半透明。角色在 .glm 中朝向 -Y (游戏会把它转向 +X), 所以前视图显示其面部——加载角色后、Alt+W 之后以及重置视图时也是如此; 与 3ds Max 相同, 北方位于“前”面之后。**[+]** > **{{MkViewCube}}** 可关闭立方体或指南针、在全部四个视图中显示立方体, 并设置 **{{MkKompassNord}}**。",
        "アクティブなビューの右上の **ViewCube** は、どこから見ているかを示し、3ds Max のものと同じように動作します。ターゲットは 26 個 (6 面、12 辺、8 角) で、マウスの下のターゲットが明るくなり、クリックすると短いアニメーションでそこへ回転します。投影は変わりません。パースビューはパースのまま、平行投影ビューでは面がそのビュー (前、上 ...)、辺や角が自由な **{{MkOrtho}}** ビューになります。立方体をドラッグすると周回します。下の**コンパスリング**には北、東、南、西があり、リングをドラッグすると垂直軸まわりだけ回転し、文字をクリックするとその方向から見ます。面を正面から見ているときは、4 つの矢印が隣の面へ、2 つの曲がった矢印がビューを 90 度ロールします。左上の家は **Home** で、カメラがフィギュアの正面・中央へ移動します。右クリックで Home、**{{MkHomeSetzen}}**、**{{MkOrtho}}** を選べます。マウスを重ねるまで立方体は半透明です。フィギュアは .glm の中で -Y を向いている (ゲームでは +X に回転) ため、前ビューに顔が映ります。フィギュアの読み込み後、Alt+W の後、ビューのリセット時も同じです。3ds Max と同じく北は「前」の面の後ろにあります。**[+]** > **{{MkViewCube}}** で立方体やコンパスをオフにしたり、4 ビューすべてに表示したり、**{{MkKompassNord}}** を設定したりできます。",
        "El **ViewCube** arriba a la derecha de la vista activa muestra desde dónde miras y se comporta como el de 3ds Max. Tiene 26 destinos: 6 caras, 12 aristas y 8 esquinas; el que está bajo el ratón se ilumina y un clic gira la vista hacia él con una animación corta. La proyección se mantiene: una perspectiva sigue en perspectiva; en una vista ortográfica una cara da su vista (Frontal, Superior ...) y una arista o esquina la vista libre **{{MkOrtho}}**. Arrastrar el cubo orbita. El **anillo de brújula** de debajo lleva N, E, S y O: arrastrar el anillo gira solo sobre el eje vertical y un clic en una letra mira desde ahí. Mirando de frente a una cara, cuatro flechas llevan a las caras vecinas y dos flechas curvas giran la vista 90 grados. La casa de arriba a la izquierda es **Inicio**: la cámara vuela a la figura de frente y centrada. El clic derecho ofrece Inicio, **{{MkHomeSetzen}}** y **{{MkOrtho}}**. El cubo es semitransparente hasta que el ratón está encima. Una figura mira hacia -Y en su .glm (el juego la gira hacia +X), por eso Frontal muestra su cara, también al cargar una figura, tras Alt+W y al restablecer la vista; como en 3ds Max, el norte queda detrás de la cara Frontal. **[+]** > **{{MkViewCube}}** desactiva el cubo o la brújula, muestra el cubo en las cuatro vistas y ajusta el **{{MkKompassNord}}**.",
        "**ViewCube** vpravo nahoře v aktivním pohledu ukazuje, odkud se díváte, a chová se jako ten v 3ds Max. Má 26 cílů - 6 stěn, 12 hran, 8 rohů; cíl pod myší se rozsvítí a klepnutí otočí pohled krátkou animací tam. Projekce zůstává: perspektiva zůstane perspektivou; v ortografickém pohledu dá stěna svůj pohled (Zepředu, Shora ...), hrana nebo roh volný pohled **{{MkOrtho}}**. Tažením krychle obíháte. **Kompasový kruh** pod ní nese S, V, J a Z: tažením kruhu otáčíte jen kolem svislé osy, klepnutím na písmeno se díváte odtud. Při pohledu přímo na stěnu vedou čtyři šipky k sousedním stěnám a dvě zahnuté šipky natočí pohled o 90 stupňů. Domeček vlevo nahoře je **Domů**: kamera přeletí k postavě zepředu a uprostřed. Pravé tlačítko nabízí Domů, **{{MkHomeSetzen}}** a **{{MkOrtho}}**. Krychle je poloprůhledná, dokud nad ní není myš. Postava se v .glm dívá ve směru -Y (hra ji otočí na +X), proto Zepředu ukazuje obličej - také po načtení postavy, po Alt+W a při obnovení pohledu; jako v 3ds Max leží sever za stěnou Zepředu. **[+]** > **{{MkViewCube}}** vypne krychli nebo kompas, zobrazí krychli ve všech čtyřech pohledech a nastaví **{{MkKompassNord}}**."),
    "kopf_bl": (
        "In Blender mode the header is laid out like Blender's 3D view: on the left the menu **{{MkBlAnsicht}}** (Frame Selected, Frame All, "
        "Perspective/Orthographic, **{{MkBlBlickpunkt}}** with the numpad keys, **{{MkBlNavigation}}** with orbit, roll and the opposite side, "
        "Toggle Quad View), on the right **{{MkBlOverlays}}** and the three buttons of **{{MkBlShading}}** - Wireframe, Solid, Material Preview "
        "- with the drop-down for backface culling and the silhouette. Below them is the **navigation gizmo**: a click on an axis looks along it, "
        "a second click looks from the opposite side, dragging orbits (an axis view turns into the perspective, like Blender's Auto "
        "Perspective); below it zoom, move and perspective/orthographic. The numpad works as in Blender: 4/6/8/2 orbit by 15 degrees, "
        "<kbd>Shift</kbd>+4/6 roll, 9 the opposite side, <kbd>Ctrl</kbd>+1/3/7 back, left and bottom, <kbd>Shift</kbd>+<kbd>Z</kbd> wireframe, "
        "<kbd>Ctrl</kbd>+<kbd>`</kbd> the gizmo.",
        "Im Modus wie Blender ist die Kopfzeile aufgebaut wie Blenders 3D-Ansicht: links das Menü **{{MkBlAnsicht}}** (Auswahl zeigen, Alles "
        "zeigen, Perspektive/Orthographisch, **{{MkBlBlickpunkt}}** mit den Ziffernblock-Tasten, **{{MkBlNavigation}}** mit Kreisen, Rollen und "
        "der Gegenseite, vier Ansichten), rechts **{{MkBlOverlays}}** und die drei Knöpfe von **{{MkBlShading}}** - Drahtgitter, Solid, "
        "Materialvorschau - mit dem Ausklappmenü für Rückseiten und Silhouette. Darunter steht das **Navigations-Gizmo**: ein Klick auf eine "
        "Achse schaut entlang ihr, ein zweiter Klick von der Gegenseite, Ziehen kreist (eine Achsenansicht wird zur Perspektive, wie Blenders "
        "Auto Perspective); darunter Zoomen, Verschieben und Perspektive/Orthographisch. Der Ziffernblock wirkt wie in Blender: 4/6/8/2 kreisen "
        "um 15 Grad, <kbd>Umschalt</kbd>+4/6 rollen, 9 die Gegenseite, <kbd>Strg</kbd>+1/3/7 Hinten, Links und Unten, "
        "<kbd>Umschalt</kbd>+<kbd>Z</kbd> Drahtgitter, <kbd>Strg</kbd>+<kbd>`</kbd> das Gizmo.",
        "Blender 模式下标题栏的布局与 Blender 的 3D 视图相同: 左侧为 **{{MkBlAnsicht}}** 菜单 (框显所选、框显全部、透视/正交、带小键盘按键的 "
        "**{{MkBlBlickpunkt}}**、带环绕、滚动和对面的 **{{MkBlNavigation}}**、切换四视图), 右侧为 **{{MkBlOverlays}}** 和 **{{MkBlShading}}** "
        "的三个按钮——线框、实体、材质预览——以及用于背面剔除和剪影的下拉菜单。其下方是**导航 Gizmo**: 单击轴沿其观察, 再次单击从对面观察, 拖动可环绕 "
        "(轴视图会变为透视, 如 Blender 的自动透视); 下方为缩放、移动和透视/正交。小键盘与 Blender 相同: 4/6/8/2 环绕 15 度, <kbd>Shift</kbd>+4/6 "
        "滚动, 9 为对面, <kbd>Ctrl</kbd>+1/3/7 为后、左、底, <kbd>Shift</kbd>+<kbd>Z</kbd> 线框, <kbd>Ctrl</kbd>+<kbd>`</kbd> 切换 Gizmo。",
        "Blender モードのヘッダーは Blender の 3D ビューと同じ配置です: 左にメニュー **{{MkBlAnsicht}}** (選択を表示、すべて表示、透視/平行投影、"
        "テンキー付きの **{{MkBlBlickpunkt}}**、周回・ロール・反対側の **{{MkBlNavigation}}**、4 分割表示)、右に **{{MkBlOverlays}}** と "
        "**{{MkBlShading}}** の 3 つのボタン (ワイヤーフレーム、ソリッド、マテリアルプレビュー) と、裏面非表示とシルエットのドロップダウン。その下に"
        "**ナビゲーションギズモ**があります。軸をクリックするとその方向から見て、もう一度クリックすると反対側から、ドラッグで周回します (Blender の "
        "Auto Perspective のように軸ビューは透視になります)。その下にズーム、移動、透視/平行投影。テンキーは Blender と同じです: 4/6/8/2 で 15 度"
        "周回、<kbd>Shift</kbd>+4/6 でロール、9 で反対側、<kbd>Ctrl</kbd>+1/3/7 で後・左・下、<kbd>Shift</kbd>+<kbd>Z</kbd> でワイヤーフレーム、"
        "<kbd>Ctrl</kbd>+<kbd>`</kbd> でギズモ。",
        "En modo Blender la cabecera está organizada como la vista 3D de Blender: a la izquierda el menú **{{MkBlAnsicht}}** (encuadrar "
        "selección, encuadrar todo, perspectiva/ortográfica, **{{MkBlBlickpunkt}}** con las teclas del teclado numérico, **{{MkBlNavigation}}** "
        "con orbitar, girar y el lado opuesto, vista cuádruple), a la derecha **{{MkBlOverlays}}** y los tres botones de **{{MkBlShading}}** "
        "(alambre, sólido, vista previa del material) con el desplegable para caras traseras y silueta. Debajo está el **gizmo de navegación**: "
        "un clic en un eje mira a lo largo de él, un segundo clic desde el lado opuesto, arrastrar orbita (una vista de eje pasa a perspectiva, "
        "como la perspectiva automática de Blender); debajo zoom, mover y perspectiva/ortográfica. El teclado numérico funciona como en Blender: "
        "4/6/8/2 orbitan 15 grados, <kbd>Mayús</kbd>+4/6 giran, 9 el lado opuesto, <kbd>Ctrl</kbd>+1/3/7 atrás, izquierda e inferior, "
        "<kbd>Mayús</kbd>+<kbd>Z</kbd> alambre, <kbd>Ctrl</kbd>+<kbd>`</kbd> el gizmo.",
        "V režimu Blender je záhlaví uspořádáno jako 3D pohled Blenderu: vlevo nabídka **{{MkBlAnsicht}}** (zobrazit výběr, zobrazit vše, "
        "perspektiva/ortografický, **{{MkBlBlickpunkt}}** s klávesami numerické klávesnice, **{{MkBlNavigation}}** s obíháním, natočením a "
        "opačnou stranou, čtyři pohledy), vpravo **{{MkBlOverlays}}** a tři tlačítka **{{MkBlShading}}** - drátový model, plné, náhled "
        "materiálu - s rozbalovací nabídkou pro zadní strany a siluetu. Pod nimi je **navigační gizmo**: klepnutí na osu se dívá podél ní, "
        "druhé klepnutí z opačné strany, tažení obíhá (pohled podél osy se změní na perspektivu, jako Auto Perspective v Blenderu); pod ním "
        "přiblížení, posun a perspektiva/ortografický. Numerická klávesnice funguje jako v Blenderu: 4/6/8/2 obíhají o 15 stupňů, "
        "<kbd>Shift</kbd>+4/6 natáčí, 9 opačná strana, <kbd>Ctrl</kbd>+1/3/7 zezadu, zleva a zespodu, <kbd>Shift</kbd>+<kbd>Z</kbd> drátový "
        "model, <kbd>Ctrl</kbd>+<kbd>`</kbd> gizmo."),
    "lizenz": (
        "The ViewCube is derived from WhiteoutDex (max_viewcube.h, BSD 3-Clause licence, Copyright (c) 2026 WhiteoutDex Contributors) and WhiteoutFlakes (BSD 3-Clause licence with AI-derived works notice, Copyright (c) 2026 Fernando Sahmkow); the navigation gizmo is drawn by JK Studio itself - no graphics of Autodesk or Blender.",
        "Der ViewCube ist aus WhiteoutDex (max_viewcube.h, BSD-3-Clause-Lizenz, Copyright (c) 2026 WhiteoutDex Contributors) und WhiteoutFlakes (BSD-3-Clause-Lizenz mit Hinweis zu KI-abgeleiteten Werken, Copyright (c) 2026 Fernando Sahmkow) abgeleitet; das Navigations-Gizmo zeichnet JK Studio selbst - keine Grafiken von Autodesk oder Blender.",
        "ViewCube 源自 WhiteoutDex (max_viewcube.h, BSD 3-Clause 许可, Copyright (c) 2026 WhiteoutDex Contributors) 和 WhiteoutFlakes (BSD 3-Clause 许可, 附 AI 衍生作品声明, Copyright (c) 2026 Fernando Sahmkow); 导航 Gizmo 由 JK Studio 自行绘制——不含 Autodesk 或 Blender 的图形。",
        "ViewCube は WhiteoutDex (max_viewcube.h、BSD 3-Clause ライセンス、Copyright (c) 2026 WhiteoutDex Contributors) と WhiteoutFlakes (BSD 3-Clause ライセンス、AI 派生物の注記付き、Copyright (c) 2026 Fernando Sahmkow) に由来します。ナビゲーションギズモは JK Studio 独自の描画で、Autodesk や Blender のグラフィックは使っていません。",
        "El ViewCube deriva de WhiteoutDex (max_viewcube.h, licencia BSD 3-Clause, Copyright (c) 2026 WhiteoutDex Contributors) y de WhiteoutFlakes (licencia BSD 3-Clause con aviso de obras derivadas por IA, Copyright (c) 2026 Fernando Sahmkow); el gizmo de navegación lo dibuja JK Studio: sin gráficos de Autodesk ni de Blender.",
        "ViewCube je odvozen z WhiteoutDex (max_viewcube.h, licence BSD 3-Clause, Copyright (c) 2026 WhiteoutDex Contributors) a z WhiteoutFlakes (licence BSD 3-Clause s upozorněním na díla odvozená AI, Copyright (c) 2026 Fernando Sahmkow); navigační gizmo kreslí JK Studio samo - žádná grafika Autodesku ani Blenderu."),
    "bild_bl": ("Blender mode: the menu Overlays open, the shading buttons beside it, below them the navigation gizmo.",
                "Modus wie Blender: das Menü Overlays offen, daneben die Knöpfe der Schattierung, darunter das Navigations-Gizmo.",
                "Blender 模式: 叠加层菜单已打开, 旁边是着色按钮, 下方是导航 Gizmo。",
                "Blender モード: オーバーレイメニューを開いたところ。横にシェーディングのボタン、その下にナビゲーションギズモ。",
                "Modo Blender: el menú Superposiciones abierto, al lado los botones de sombreado, debajo el gizmo de navegación.",
                "Režim Blender: otevřená nabídka Překryvy, vedle tlačítka stínování, pod nimi navigační gizmo."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def bloecke():
    """Die Bloecke des Abschnitts (in inhalt_mb1a1.seite nach dem Befehlsfeld)."""
    return [
        ("h2", "kopfzeile", _t("h_kopf")),
        ("p", _t("kopf_max")),
        ("img", "h_mbk_max", _t("bild_max"), "breit"),
        ("p", _t("cube")),
        ("img", "h_mbk_cube", _t("bild_cube"), "breit"),
        ("p", _t("kopf_bl")),
        ("img", "h_mbk_blender", _t("bild_bl"), "breit"),
        ("p", _t("lizenz")),
    ]
