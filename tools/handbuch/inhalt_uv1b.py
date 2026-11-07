"""Modell-Block 1b: die Seite "Model-Tab: UV erstellen" (UV-Editor neben den Ansichten, Auswahl in beiden, Naehte, Abwickeln,
Projektionen, Entspannen, Begradigen, Stapeln, Dichte, Packen, Pruefen und Messen, Quick unwrap, Speichern als .glm/.md3,
Tasten wie 3ds Max oder Blender, Lizenzhinweis).

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_rt.py.
"""

X = {
    "titel": ("Model tab: create UVs", "Model-Tab: UV erstellen", "模型选项卡: 创建 UV", "モデルタブ: UV 作成", "Pestaña Model: crear UV", "Karta Model: vytvoření UV"),
    "kurz": (
        "Lay the surfaces of a .glm or .md3 flat over their texture: cut seams, unwrap, straighten, stack mirrored halves, match the texel density and pack the "
        "islands - by hand or in one step - and save the model for the game.",
        "Lege die Flächen einer .glm oder .md3 flach über ihre Textur: Nähte schneiden, abwickeln, begradigen, gespiegelte Hälften stapeln, die Texel-Dichte "
        "angleichen und die Inseln packen - von Hand oder in einem Schritt - und speichere das Modell für das Spiel.",
        "把 .glm 或 .md3 的面平铺到其纹理上: 切接缝、展开、拉直、堆叠镜像两半、匹配像素密度并打包岛 - 手动或一步完成 - 然后为游戏保存模型。",
        ".glm や .md3 の面をテクスチャの上に平らに広げます: シームを切り、展開し、直線化し、ミラーの半分を重ね、テクセル密度を揃え、アイランドをパック - 手作業でも 1 ステップでも - そしてゲーム用に保存します。",
        "Extiende las superficies de un .glm o .md3 sobre su textura: corta costuras, despliega, endereza, apila mitades reflejadas, iguala la densidad de téxeles "
        "y empaqueta las islas, a mano o en un paso, y guarda el modelo para el juego.",
        "Rozložte plochy souboru .glm nebo .md3 na jejich texturu: řežte švy, rozbalte, narovnejte, stohujte zrcadlové poloviny, sjednoťte hustotu texelů a "
        "zabalte ostrovy - ručně nebo jedním krokem - a uložte model pro hru."),
    "h_wo": ("The UV area", "Der Bereich UV", "UV 区域", "UV 領域", "El área UV", "Oblast UV"),
    "wo": (
        "Open a .glm or an .md3 in the model tab and choose the area **UV** at the top. The **{{UvEditorMax}}** (Blender mode: **{{UvEditorBlender}}**) stands "
        "left of the views and shows the islands over the texture of the surface, with the shader and skin of the preview; the edge on its right sets the width, "
        "the mouse wheel zooms, the middle button pans. **{{UvBild}}** in the rollout chooses which texture you work on: all surfaces that draw it are edited "
        "and packed together, at the LOD shown in the view; tag surfaces (*) have no UVs. The commands are in the rollouts of the command panel, in the menu "
        "**{{UvMenue}}**, in the right-click of the view (**{{UvMenue}}**) and of the UV editor, and in the search (X, F3 in Blender mode); every tooltip names its "
        "key. Every command is one undo step.",
        "Öffne eine .glm oder .md3 im Model-Tab und wähle oben den Bereich **UV**. Der **{{UvEditorMax}}** (im Blender-Modus **{{UvEditorBlender}}**) steht links "
        "neben den Ansichten und zeigt die Inseln über der Textur der Fläche, mit Shader und Skin der Vorschau; die Kante rechts ändert die Breite, das Mausrad "
        "zoomt, die mittlere Taste verschiebt. **{{UvBild}}** im Rollout wählt, an welcher Textur du arbeitest: alle Flächen, die sie zeichnen, werden zusammen "
        "bearbeitet und gepackt, auf der LOD-Stufe der Ansicht; Tag-Flächen (*) haben keine UV. Die Befehle stehen in den Rollouts des Befehlsfelds, im Menü "
        "**{{UvMenue}}**, im Rechtsklick der Ansicht (**{{UvMenue}}**) und des UV-Editors und in der Suche (X, im Blender-Modus F3); jeder Tooltip nennt seine "
        "Taste. Jeder Befehl ist ein Rückgängig-Schritt.",
        "在模型选项卡中打开 .glm 或 .md3, 在顶部选择 **UV** 区域。**{{UvEditorMax}}** (Blender 模式为 **{{UvEditorBlender}}**) 位于视图左侧, 以预览的着色器和皮肤在面的纹理上显示岛; "
        "右侧边缘调整宽度, 滚轮缩放, 中键平移。卷展栏中的 **{{UvBild}}** 选择要处理的纹理: 绘制它的所有面一起编辑和打包, 使用视图显示的 LOD; 标签面 (*) 没有 UV。命令位于命令面板的卷展栏、"
        "菜单 **{{UvMenue}}**、视图与 UV 编辑器的右键菜单 (**{{UvMenue}}**) 以及搜索 (X, Blender 模式为 F3) 中; 每个提示都写有按键。每个命令都是一个撤销步骤。",
        "モデルタブで .glm か .md3 を開き、上の領域 **UV** を選びます。**{{UvEditorMax}}** (Blender 風は **{{UvEditorBlender}}**) はビューの左にあり、プレビューと同じシェーダーとスキンで"
        "面のテクスチャの上にアイランドを表示します。右端で幅を変え、ホイールでズーム、中ボタンでパン。ロールアウトの **{{UvBild}}** で作業するテクスチャを選びます: それを描く全ての面を"
        "ビューの LOD で一緒に編集・パックします。タグ面 (*) には UV がありません。コマンドはコマンドパネルのロールアウト、メニュー **{{UvMenue}}**、ビューと UV エディターの右クリック "
        "(**{{UvMenue}}**)、検索 (X、Blender 風は F3) にあり、ツールチップにキーが書かれています。各コマンドは 1 回の元に戻す単位です。",
        "Abre un .glm o un .md3 en la pestaña Model y elige arriba el área **UV**. El **{{UvEditorMax}}** (modo Blender: **{{UvEditorBlender}}**) está a la "
        "izquierda de las vistas y muestra las islas sobre la textura de la superficie, con el shader y la skin de la vista previa; el borde derecho cambia el "
        "ancho, la rueda hace zoom, el botón central desplaza. **{{UvBild}}** en el panel elige en qué textura trabajas: todas las superficies que la dibujan se "
        "editan y empaquetan juntas, en el LOD de la vista; las superficies tag (*) no tienen UV. Los comandos están en los paneles del panel de comandos, en el "
        "menú **{{UvMenue}}**, en el clic derecho de la vista (**{{UvMenue}}**) y del editor UV, y en la búsqueda (X, F3 en modo Blender); cada ayuda nombra su "
        "tecla. Cada comando es un paso de deshacer.",
        "Otevřete .glm nebo .md3 v kartě Model a nahoře zvolte oblast **UV**. **{{UvEditorMax}}** (v režimu Blender **{{UvEditorBlender}}**) stojí vlevo od "
        "pohledů a ukazuje ostrovy nad texturou plochy, se shaderem a skinem náhledu; pravý okraj mění šířku, kolečko přibližuje, prostřední tlačítko posouvá. "
        "**{{UvBild}}** v rolloutu volí, na které textuře pracujete: všechny plochy, které ji kreslí, se upravují a balí společně, na LOD pohledu; plochy tagů (*) "
        "nemají UV. Příkazy jsou v rolloutech panelu příkazů, v nabídce **{{UvMenue}}**, v pravém kliknutí pohledu (**{{UvMenue}}**) a editoru UV a ve "
        "vyhledávání (X, v režimu Blender F3); každá nápověda uvádí klávesu. Každý příkaz je jeden krok zpět."),
    "bild_editor": ("The UV editor beside the view: the stormtrooper's islands over its texture, seams in red, also on the model.",
                    "Der UV-Editor neben der Ansicht: die Inseln des Sturmtrupplers über seiner Textur, Nähte rot, auch auf dem Modell.",
                    "视图旁的 UV 编辑器: 冲锋队员的岛覆盖在其纹理上, 接缝为红色, 模型上也显示。",
                    "ビュー横の UV エディター: ストームトルーパーのアイランドをテクスチャの上に、シームは赤、モデル上にも。",
                    "El editor UV junto a la vista: las islas del soldado de asalto sobre su textura, las costuras en rojo, también en el modelo.",
                    "Editor UV vedle pohledu: ostrovy stormtroopera nad jeho texturou, švy červeně, i na modelu."),
    "h_auswahl": ("Selection in both places", "Auswahl an beiden Stellen", "两处共用的选择", "両方で共通の選択", "Selección en ambos sitios", "Výběr na obou místech"),
    "auswahl": (
        "**{{UvEbeneEcke}}**, **{{UvEbeneKante}}**, **{{UvEbeneFlaeche}}** and **{{UvEbeneInsel}}** set what a click picks - in the UV editor and on the model at the "
        "same time, so a seam can be picked on the model and the island it makes moved in the editor. 3ds Max mode: 1, 2, 4, 5 (vertex, edge, polygon, element), "
        "Ctrl adds, Alt removes; Blender mode: 1, 2, 3, 4, Shift toggles. Drag a frame to pick many. **{{UvWerkzeugVerschieben}}**, **{{UvWerkzeugDrehen}}** and "
        "**{{UvWerkzeugSkalieren}}** (W E R; in Blender G R S, which follow the mouse until a click) work on the selection; **{{UvPixelFang}}** moves in whole "
        "texels of the texture.",
        "**{{UvEbeneEcke}}**, **{{UvEbeneKante}}**, **{{UvEbeneFlaeche}}** und **{{UvEbeneInsel}}** bestimmen, was ein Klick wählt - im UV-Editor und am Modell "
        "zugleich: eine Naht wählst du am Modell, die Insel, die sie ergibt, verschiebst du im Editor. Modus wie 3ds Max: 1, 2, 4, 5 (Scheitelpunkt, Kante, "
        "Polygon, Element), Strg fügt hinzu, Alt nimmt weg; wie Blender: 1, 2, 3, 4, Umschalt schaltet um. Ein Rahmen wählt viele. **{{UvWerkzeugVerschieben}}**, "
        "**{{UvWerkzeugDrehen}}** und **{{UvWerkzeugSkalieren}}** (W E R; in Blender G R S, die der Maus bis zum Klick folgen) wirken auf die Auswahl; "
        "**{{UvPixelFang}}** verschiebt in ganzen Texeln der Textur.",
        "**{{UvEbeneEcke}}**、**{{UvEbeneKante}}**、**{{UvEbeneFlaeche}}** 和 **{{UvEbeneInsel}}** 决定单击选择什么 - 同时在 UV 编辑器和模型上, 因此可以在模型上选接缝, 在编辑器中移动它形成的岛。"
        "3ds Max 模式: 1、2、4、5 (顶点、边、多边形、元素), Ctrl 添加, Alt 移除; Blender 模式: 1、2、3、4, Shift 切换。拖框可多选。**{{UvWerkzeugVerschieben}}**、**{{UvWerkzeugDrehen}}** 和 "
        "**{{UvWerkzeugSkalieren}}** (W E R; Blender 中为 G R S, 随鼠标移动直到单击) 作用于选择; **{{UvPixelFang}}** 按纹理整像素移动。",
        "**{{UvEbeneEcke}}**、**{{UvEbeneKante}}**、**{{UvEbeneFlaeche}}**、**{{UvEbeneInsel}}** はクリックで選ぶものを決めます - UV エディターとモデルで同時に。シームはモデルで選び、"
        "できたアイランドはエディターで動かせます。3ds Max 風: 1、2、4、5 (頂点、エッジ、ポリゴン、要素)、Ctrl で追加、Alt で除外。Blender 風: 1、2、3、4、Shift で切替。枠で複数選択。"
        "**{{UvWerkzeugVerschieben}}**、**{{UvWerkzeugDrehen}}**、**{{UvWerkzeugSkalieren}}** (W E R; Blender は G R S でクリックまでマウスに追従) は選択に作用し、**{{UvPixelFang}}** は"
        "テクスチャの整数テクセル単位で動かします。",
        "**{{UvEbeneEcke}}**, **{{UvEbeneKante}}**, **{{UvEbeneFlaeche}}** e **{{UvEbeneInsel}}** fijan qué elige un clic, en el editor UV y en el modelo a la vez: "
        "una costura se elige en el modelo y la isla que forma se mueve en el editor. Modo 3ds Max: 1, 2, 4, 5 (vértice, arista, polígono, elemento), Ctrl añade, "
        "Alt quita; modo Blender: 1, 2, 3, 4, Mayús alterna. Arrastra un marco para elegir muchos. **{{UvWerkzeugVerschieben}}**, **{{UvWerkzeugDrehen}}** y "
        "**{{UvWerkzeugSkalieren}}** (W E R; en Blender G R S, que siguen al ratón hasta un clic) actúan sobre la selección; **{{UvPixelFang}}** mueve en "
        "téxeles enteros de la textura.",
        "**{{UvEbeneEcke}}**, **{{UvEbeneKante}}**, **{{UvEbeneFlaeche}}** a **{{UvEbeneInsel}}** určují, co klik vybere - v editoru UV i na modelu zároveň: šev "
        "vyberete na modelu a ostrov, který vytvoří, posunete v editoru. Režim 3ds Max: 1, 2, 4, 5 (vrchol, hrana, polygon, prvek), Ctrl přidá, Alt ubere; "
        "režim Blender: 1, 2, 3, 4, Shift přepíná. Rámeček vybere mnoho. **{{UvWerkzeugVerschieben}}**, **{{UvWerkzeugDrehen}}** a **{{UvWerkzeugSkalieren}}** "
        "(W E R; v Blenderu G R S, které sledují myš až do kliknutí) působí na výběr; **{{UvPixelFang}}** posouvá po celých texelech textury."),
    "h_arbeit": ("Seams, unwrap, arrange", "Nähte, abwickeln, anordnen", "接缝、展开、排列", "シーム、展開、配置", "Costuras, desplegar, ordenar", "Švy, rozbalení, uspořádání"),
    "arbeit": (
        "A model from the game already has a layout: its seams are read as marks and shown in red. **{{UvNahtMarkieren}}** (C) cuts the selected edges or the "
        "border of the selected faces, **{{UvNahtLoeschen}}** (Shift+C) removes seams and sews the smaller island to the larger. **{{UvNaehteSkelett}}**, "
        "**{{UvNaehteForm}}** and **{{UvNaehteWinkel}}** find seams for you. **{{UvAbwickeln}}** (U) lays the selected faces flat as new pieces in free space at the "
        "density of the texture; **{{UvNeuLoesen}}** solves islands again where they lie. The method is **{{UvMethodeLscm}}** or **{{UvMethodeDehnung}}**. "
        "**{{UvProjEben}}**, **{{UvProjKasten}}**, **{{UvProjZylinder}}** and **{{UvProjKugel}}** project the selection. Then **{{UvEntspannen}}**, "
        "**{{UvBegradigen}}**, **{{UvPin}}**, **{{UvStapeln}}** (left and right halves share one patch), **{{UvDichte}}**, **{{UvAusrichten}}**, **{{UvSperren}}** and "
        "**{{UvPacken}}** (Shift+P: no island on another, the locked ones stay). JKA has one UV per vertex: where a seam splits UVs, the vertex is doubled with the "
        "same position, normal, bones and weights; a command that would give a surface 1000 vertices or more (SHADER_MAX_VERTEXES) changes nothing and says so.",
        "Ein Modell aus dem Spiel hat schon eine Anordnung: seine Nähte werden als Marken gelesen und rot gezeigt. **{{UvNahtMarkieren}}** (C) schneidet die "
        "gewählten Kanten oder den Rand der gewählten Flächen, **{{UvNahtLoeschen}}** (Umschalt+C) entfernt Nähte und näht die kleinere Insel an die größere. "
        "**{{UvNaehteSkelett}}**, **{{UvNaehteForm}}** und **{{UvNaehteWinkel}}** finden Nähte für dich. **{{UvAbwickeln}}** (U) legt die gewählten Flächen als neue "
        "Stücke flach in freien Platz, mit der Dichte der Textur; **{{UvNeuLoesen}}** löst Inseln dort neu, wo sie liegen. Die Methode ist **{{UvMethodeLscm}}** "
        "oder **{{UvMethodeDehnung}}**. **{{UvProjEben}}**, **{{UvProjKasten}}**, **{{UvProjZylinder}}** und **{{UvProjKugel}}** projizieren die Auswahl. Dann "
        "**{{UvEntspannen}}**, **{{UvBegradigen}}**, **{{UvPin}}**, **{{UvStapeln}}** (linke und rechte Hälfte teilen ein Stück Textur), **{{UvDichte}}**, "
        "**{{UvAusrichten}}**, **{{UvSperren}}** und **{{UvPacken}}** (Umschalt+P: keine Insel auf einer anderen, gesperrte bleiben). JKA hat eine UV je Ecke: wo eine "
        "Naht die UV trennt, wird die Ecke verdoppelt, mit gleichem Ort, gleicher Normale, gleichen Knochen und Gewichten; ein Befehl, der einer Fläche 1000 Ecken "
        "oder mehr gäbe (SHADER_MAX_VERTEXES), ändert nichts und sagt es.",
        "游戏中的模型已有布局: 其接缝作为标记读入并以红色显示。**{{UvNahtMarkieren}}** (C) 切开选中的边或选中面的边界, **{{UvNahtLoeschen}}** (Shift+C) 移除接缝并把较小的岛缝到较大的上。"
        "**{{UvNaehteSkelett}}**、**{{UvNaehteForm}}** 和 **{{UvNaehteWinkel}}** 自动查找接缝。**{{UvAbwickeln}}** (U) 把选中的面作为新块展平并按纹理密度放到空位; **{{UvNeuLoesen}}** "
        "在原位重新求解岛。方法为 **{{UvMethodeLscm}}** 或 **{{UvMethodeDehnung}}**。**{{UvProjEben}}**、**{{UvProjKasten}}**、**{{UvProjZylinder}}** 和 **{{UvProjKugel}}** 投影选择。之后使用 "
        "**{{UvEntspannen}}**、**{{UvBegradigen}}**、**{{UvPin}}**、**{{UvStapeln}}** (左右两半共用一块纹理)、**{{UvDichte}}**、**{{UvAusrichten}}**、**{{UvSperren}}** 和 **{{UvPacken}}** "
        "(Shift+P: 岛互不重叠, 锁定的保持不动)。JKA 每个顶点只有一组 UV: 接缝分开 UV 的地方, 顶点会被复制, 位置、法线、骨骼和权重相同; 若某命令会使一个面达到 1000 个或更多顶点 "
        "(SHADER_MAX_VERTEXES), 则不做更改并给出提示。",
        "ゲームのモデルには既に配置があります: そのシームはマークとして読まれ赤で表示されます。**{{UvNahtMarkieren}}** (C) は選択したエッジか選択した面の境界を切り、"
        "**{{UvNahtLoeschen}}** (Shift+C) はシームを消して小さいアイランドを大きい方に縫い合わせます。**{{UvNaehteSkelett}}**、**{{UvNaehteForm}}**、**{{UvNaehteWinkel}}** はシームを"
        "自動で探します。**{{UvAbwickeln}}** (U) は選択した面を新しいピースとして平らにし、テクスチャの密度で空き場所に置きます。**{{UvNeuLoesen}}** はアイランドをその場で解き直します。"
        "方式は **{{UvMethodeLscm}}** か **{{UvMethodeDehnung}}**。**{{UvProjEben}}**、**{{UvProjKasten}}**、**{{UvProjZylinder}}**、**{{UvProjKugel}}** は選択を投影します。その後 "
        "**{{UvEntspannen}}**、**{{UvBegradigen}}**、**{{UvPin}}**、**{{UvStapeln}}** (左右の半分で同じテクスチャを共有)、**{{UvDichte}}**、**{{UvAusrichten}}**、**{{UvSperren}}**、"
        "**{{UvPacken}}** (Shift+P: 重なりなし、ロックしたものは残る)。JKA は頂点ごとに UV が 1 つ: シームが UV を分ける所では、位置・法線・ボーン・ウェイトが同じ頂点を複製します。"
        "面の頂点が 1000 以上 (SHADER_MAX_VERTEXES) になるコマンドは何も変えずに知らせます。",
        "Un modelo del juego ya tiene una disposición: sus costuras se leen como marcas y se muestran en rojo. **{{UvNahtMarkieren}}** (C) corta las aristas elegidas "
        "o el borde de las caras elegidas, **{{UvNahtLoeschen}}** (Mayús+C) quita costuras y cose la isla menor a la mayor. **{{UvNaehteSkelett}}**, "
        "**{{UvNaehteForm}}** y **{{UvNaehteWinkel}}** buscan costuras por ti. **{{UvAbwickeln}}** (U) aplana las caras elegidas como piezas nuevas en espacio "
        "libre, con la densidad de la textura; **{{UvNeuLoesen}}** vuelve a resolver las islas donde están. El método es **{{UvMethodeLscm}}** o "
        "**{{UvMethodeDehnung}}**. **{{UvProjEben}}**, **{{UvProjKasten}}**, **{{UvProjZylinder}}** y **{{UvProjKugel}}** proyectan la selección. Después "
        "**{{UvEntspannen}}**, **{{UvBegradigen}}**, **{{UvPin}}**, **{{UvStapeln}}** (mitad izquierda y derecha comparten textura), **{{UvDichte}}**, "
        "**{{UvAusrichten}}**, **{{UvSperren}}** y **{{UvPacken}}** (Mayús+P: ninguna isla sobre otra, las bloqueadas se quedan). JKA tiene una UV por vértice: "
        "donde una costura separa las UV, el vértice se duplica con la misma posición, normal, huesos y pesos; un comando que daría a una superficie 1000 vértices "
        "o más (SHADER_MAX_VERTEXES) no cambia nada y lo dice.",
        "Model ze hry už rozložení má: jeho švy se načtou jako značky a ukáží se červeně. **{{UvNahtMarkieren}}** (C) rozřízne vybrané hrany nebo okraj vybraných "
        "ploch, **{{UvNahtLoeschen}}** (Shift+C) odstraní švy a přišije menší ostrov k většímu. **{{UvNaehteSkelett}}**, **{{UvNaehteForm}}** a "
        "**{{UvNaehteWinkel}}** najdou švy za vás. **{{UvAbwickeln}}** (U) rozloží vybrané plochy jako nové kusy do volného místa s hustotou textury; "
        "**{{UvNeuLoesen}}** znovu vyřeší ostrovy na místě. Metoda je **{{UvMethodeLscm}}** nebo **{{UvMethodeDehnung}}**. **{{UvProjEben}}**, **{{UvProjKasten}}**, "
        "**{{UvProjZylinder}}** a **{{UvProjKugel}}** promítnou výběr. Pak **{{UvEntspannen}}**, **{{UvBegradigen}}**, **{{UvPin}}**, **{{UvStapeln}}** (levá a pravá "
        "polovina sdílí kus textury), **{{UvDichte}}**, **{{UvAusrichten}}**, **{{UvSperren}}** a **{{UvPacken}}** (Shift+P: žádný ostrov na jiném, zamčené "
        "zůstanou). JKA má jedno UV na vrchol: kde šev UV rozdělí, vrchol se zdvojí se stejnou polohou, normálou, kostmi a vahami; příkaz, který by ploše dal "
        "1000 vrcholů nebo víc (SHADER_MAX_VERTEXES), nic nezmění a oznámí to."),
    "h_schnell": ("Quick unwrap, check and save", "Schnell abwickeln, prüfen und speichern", "快速展开、检查与保存", "クイック展開、チェックと保存",
                  "Despliegue rápido, comprobar y guardar", "Rychlé rozbalení, kontrola a uložení"),
    "schnell": (
        "**{{UvGrSchnell}}** does the whole sequence in one step: **{{UvSchnellFigur}}** (seams from the skeleton, limbs opened, mirrored halves stacked), "
        "**{{UvSchnellRequisit}}** (seams from the shape) and **{{UvSchnellHart}}** (seams at sharp edges, rectangles). With nothing selected it works on the whole "
        "texture. The rollout **{{UvGrPruefen}}** counts overlapping, flipped and stretched islands, rings and density outliers and measures the layout: angle "
        "error, conformality, area error and texel density per unit. **{{UvDehnungZeigen}}**, **{{UvUeberlappZeigen}}** and **{{UvSchachbrett}}** show the same in "
        "the editor. The texture was painted for the layout the model came with - move an island and the picture moves with it, Undo puts it back. Save with "
        "Ctrl+S: a .glm through the GLM writer, an .md3 with all its frames and tags; without a change both are byte for byte the file you opened.",
        "**{{UvGrSchnell}}** macht die ganze Folge in einem Schritt: **{{UvSchnellFigur}}** (Nähte aus dem Skelett, Glieder geöffnet, gespiegelte Hälften gestapelt), "
        "**{{UvSchnellRequisit}}** (Nähte nach der Form) und **{{UvSchnellHart}}** (Nähte an scharfen Kanten, Rechtecke). Ohne Auswahl wirkt es auf die ganze "
        "Textur. Das Rollout **{{UvGrPruefen}}** zählt überlappende, umgeklappte und gedehnte Inseln, Ringe und Ausreißer der Dichte und misst die Anordnung: "
        "Winkelfehler, Konformität, Flächenfehler und Texel-Dichte je Einheit. **{{UvDehnungZeigen}}**, **{{UvUeberlappZeigen}}** und **{{UvSchachbrett}}** zeigen "
        "dasselbe im Editor. Die Textur ist für die Anordnung gemalt, mit der das Modell kam - verschiebst du eine Insel, wandert das Bild mit, Rückgängig holt es "
        "zurück. Speichern mit Strg+S: eine .glm über den GLM-Schreiber, eine .md3 mit allen Bildern und Tags; ohne Änderung sind beide Byte für Byte die Datei, die "
        "du geöffnet hast.",
        "**{{UvGrSchnell}}** 一步完成整个流程: **{{UvSchnellFigur}}** (从骨架生成接缝、打开四肢、堆叠镜像两半)、**{{UvSchnellRequisit}}** (按形状生成接缝) 和 **{{UvSchnellHart}}** "
        "(在锐边生成接缝、矩形)。无选择时作用于整个纹理。卷展栏 **{{UvGrPruefen}}** 统计重叠、翻转和拉伸的岛、环和密度异常, 并测量布局: 角度误差、保角性、面积误差和每单位像素密度。"
        "**{{UvDehnungZeigen}}**、**{{UvUeberlappZeigen}}** 和 **{{UvSchachbrett}}** 在编辑器中显示同样内容。纹理是为模型原有布局绘制的 - 移动岛时图像随之移动, 撤销可恢复。用 Ctrl+S 保存: "
        ".glm 通过 GLM 写入器, .md3 包含全部帧和标签; 未修改时两者与打开的文件逐字节相同。",
        "**{{UvGrSchnell}}** は全工程を 1 ステップで行います: **{{UvSchnellFigur}}** (スケルトンからシーム、手足を開き、ミラーの半分を重ねる)、**{{UvSchnellRequisit}}** (形状からシーム)、"
        "**{{UvSchnellHart}}** (鋭いエッジにシーム、矩形)。選択なしならテクスチャ全体に作用します。ロールアウト **{{UvGrPruefen}}** は重なり・裏返り・伸びたアイランド、リング、密度の"
        "外れ値を数え、配置を測定します: 角度誤差、等角性、面積誤差、単位あたりのテクセル密度。**{{UvDehnungZeigen}}**、**{{UvUeberlappZeigen}}**、**{{UvSchachbrett}}** は同じものを"
        "エディターに表示します。テクスチャは元の配置に合わせて描かれています - アイランドを動かすと絵も動き、元に戻すで戻ります。Ctrl+S で保存: .glm は GLM ライター、.md3 は全フレームと"
        "タグ付き。変更がなければ、どちらも開いたファイルとバイト単位で同じです。",
        "**{{UvGrSchnell}}** hace toda la secuencia en un paso: **{{UvSchnellFigur}}** (costuras del esqueleto, extremidades abiertas, mitades reflejadas apiladas), "
        "**{{UvSchnellRequisit}}** (costuras por forma) y **{{UvSchnellHart}}** (costuras en aristas vivas, rectángulos). Sin selección actúa sobre toda la "
        "textura. El panel **{{UvGrPruefen}}** cuenta islas solapadas, volteadas y estiradas, anillos y densidades atípicas y mide la disposición: error de "
        "ángulo, conformidad, error de área y densidad de téxeles por unidad. **{{UvDehnungZeigen}}**, **{{UvUeberlappZeigen}}** y **{{UvSchachbrett}}** lo muestran "
        "en el editor. La textura se pintó para la disposición original del modelo: si mueves una isla, la imagen se mueve con ella; Deshacer la devuelve. Guarda "
        "con Ctrl+S: un .glm con el escritor GLM, un .md3 con todos sus fotogramas y tags; sin cambios ambos son byte a byte el archivo que abriste.",
        "**{{UvGrSchnell}}** provede celý postup jedním krokem: **{{UvSchnellFigur}}** (švy z kostry, otevřené končetiny, zrcadlové poloviny na stohu), "
        "**{{UvSchnellRequisit}}** (švy podle tvaru) a **{{UvSchnellHart}}** (švy na ostrých hranách, obdélníky). Bez výběru působí na celou texturu. Rollout "
        "**{{UvGrPruefen}}** počítá překrývající se, převrácené a roztažené ostrovy, prstence a odchylky hustoty a měří rozložení: úhlovou chybu, konformitu, "
        "chybu plochy a hustotu texelů na jednotku. **{{UvDehnungZeigen}}**, **{{UvUeberlappZeigen}}** a **{{UvSchachbrett}}** ukážou totéž v editoru. Textura je "
        "namalovaná pro rozložení, se kterým model přišel - posunete-li ostrov, obraz jde s ním, Zpět ho vrátí. Ukládání Ctrl+S: .glm přes zapisovač GLM, .md3 se "
        "všemi snímky a tagy; bez změny jsou obě bajt po bajtu soubor, který jste otevřeli."),
    "bild_schnell": ("Quick unwrap Character on the stormtrooper: the limbs opened, mirrored halves stacked, the rollout Check with the measurements.",
                     "Schnell abwickeln Figur am Sturmtruppler: die Glieder geöffnet, gespiegelte Hälften gestapelt, das Rollout Prüfen mit den Messwerten.",
                     "在冲锋队员上快速展开 '角色': 四肢已打开, 镜像两半已堆叠, 检查卷展栏显示测量值。",
                     "ストームトルーパーにクイック展開「キャラクター」: 手足を開き、ミラーの半分を重ね、チェックのロールアウトに測定値。",
                     "Despliegue rápido Personaje en el soldado de asalto: extremidades abiertas, mitades reflejadas apiladas, el panel Comprobar con las medidas.",
                     "Rychlé rozbalení Postava na stormtrooperovi: otevřené končetiny, zrcadlové poloviny na stohu, rollout Kontrola s měřeními."),
    "h_lizenz": ("Origin and licence", "Herkunft und Lizenz", "来源与许可", "出典とライセンス", "Origen y licencia", "Původ a licence"),
    "lizenz": (
        "The UV tools are derived from WhiteoutLib and WhiteoutFlakes (BSD 3-Clause licence with the AI-derived works notice, Copyright (c) 2026 Fernando Sahmkow); "
        "charts by shape use xatlas (MIT, Copyright (c) 2018-2020 Jonathan Young). They work only on Jedi Academy data: .glm and .md3 surfaces, .skin and .shader. "
        "The licence texts ship in licenses/ and are listed in THIRD-PARTY.md and in Help > About.",
        "Die UV-Werkzeuge sind abgeleitet aus WhiteoutLib und WhiteoutFlakes (BSD-3-Clause-Lizenz mit dem Hinweis zu KI-abgeleiteten Werken, Copyright (c) 2026 "
        "Fernando Sahmkow); die Karten nach der Form nutzen xatlas (MIT, Copyright (c) 2018-2020 Jonathan Young). Sie arbeiten nur auf Daten von Jedi Academy: "
        "Flächen von .glm und .md3, .skin und .shader. Die Lizenztexte liegen in licenses/ und stehen in THIRD-PARTY.md und unter Hilfe > Über.",
        "UV 工具源自 WhiteoutLib 和 WhiteoutFlakes (BSD 3-Clause 许可, 附 AI 衍生作品声明, Copyright (c) 2026 Fernando Sahmkow); 按形状生成图表使用 xatlas (MIT, Copyright (c) 2018-2020 "
        "Jonathan Young)。它们只处理 Jedi Academy 数据: .glm 和 .md3 的面、.skin 和 .shader。许可文本位于 licenses/, 并列在 THIRD-PARTY.md 和 帮助 > 关于 中。",
        "UV ツールは WhiteoutLib と WhiteoutFlakes に由来します (BSD 3-Clause ライセンス、AI 派生物の注記付き、Copyright (c) 2026 Fernando Sahmkow)。形状からのチャートは xatlas "
        "(MIT、Copyright (c) 2018-2020 Jonathan Young) を使います。扱うのは Jedi Academy のデータだけです: .glm と .md3 の面、.skin と .shader。ライセンス文は licenses/ にあり、"
        "THIRD-PARTY.md とヘルプ > 情報 に記載されています。",
        "Las herramientas UV derivan de WhiteoutLib y WhiteoutFlakes (licencia BSD 3-Clause con el aviso de obras derivadas por IA, Copyright (c) 2026 Fernando "
        "Sahmkow); las cartas por forma usan xatlas (MIT, Copyright (c) 2018-2020 Jonathan Young). Solo trabajan con datos de Jedi Academy: superficies de .glm y "
        ".md3, .skin y .shader. Las licencias están en licenses/ y en THIRD-PARTY.md y Ayuda > Acerca de.",
        "Nástroje UV jsou odvozeny z WhiteoutLib a WhiteoutFlakes (licence BSD 3-Clause s upozorněním na díla odvozená AI, Copyright (c) 2026 Fernando Sahmkow); "
        "mapy podle tvaru používají xatlas (MIT, Copyright (c) 2018-2020 Jonathan Young). Pracují jen s daty Jedi Academy: plochy .glm a .md3, .skin a .shader. "
        "Texty licencí jsou v licenses/ a v THIRD-PARTY.md a Nápověda > O programu."),
}


def _t(schluessel):
    from inhalt_basis import T
    z = X[schluessel]
    return T(z[0], z[1], z[2], z[3])


def seite():
    """Die Seite modell-uv (Gruppe Missionen, nach modell-retarget)."""
    return {
        "id": "modell-uv",
        "gruppe": "mission",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("h2", "wo", _t("h_wo")),
            ("p", _t("wo")),
            ("img", "h_uv_editor", _t("bild_editor"), "breit"),
            ("h2", "auswahl", _t("h_auswahl")),
            ("p", _t("auswahl")),
            ("h2", "arbeit", _t("h_arbeit")),
            ("p", _t("arbeit")),
            ("h2", "schnell", _t("h_schnell")),
            ("p", _t("schnell")),
            ("img", "h_uv_schnell", _t("bild_schnell"), "breit"),
            ("h2", "lizenz", _t("h_lizenz")),
            ("p", _t("lizenz")),
        ],
    }


ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}
