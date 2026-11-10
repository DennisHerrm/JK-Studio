"""Modell-Block 1a-1: die Seite "Model-Tab: Ansehen und Pruefen" (Dokument, Gestell, Bereich Ansehen/Pruefen wie ModView,
Bedienung wie 3ds Max).

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_modobj.py: die ersten vier gehen in
T(...), Spanisch und Tschechisch nach inhalt_es.py / inhalt_cs.py (ES, CS).
"""

import inhalt_mbkopf  # noqa: E402  (MBKOPF: Kopfzeile der Ansicht, ViewCube und Gizmo)
import inhalt_mbraster  # noqa: E402  (MBRASTER: Boden-Raster wie 3ds Max / Blender)

X = {
    "titel": ("Model tab: view and check", "Model-Tab: Ansehen und Prüfen", "模型选项卡:查看与检查", "モデルタブ: 表示とチェック",
              "Pestaña Model: ver y comprobar", "Karta Model: prohlížení a kontrola"),
    "kurz": (
        "Look at a model like in ModView - surfaces, tags, bones, LODs, skins, attached sabers, primary and secondary animation - drawn the way "
        "the game draws it, and check it against the limits of Jedi Academy, Jedi Outcast and Movie Duels. Mouse and keys work like 3ds Max.",
        "Ein Modell ansehen wie in ModView - Flächen, Tags, Knochen, LODs, Skins, angebaute Säbel, Primär- und Sekundäranimation - so gezeichnet, "
        "wie das Spiel es zeichnet, und gegen die Grenzen von Jedi Academy, Jedi Outcast und Movie Duels prüfen. Maus und Tasten wie in 3ds Max.",
        "像在 ModView 中一样查看模型——面、标记、骨骼、LOD、皮肤、附加的光剑、主要和次要动画——按游戏的方式绘制,并对照 Jedi Academy、Jedi Outcast "
        "和 Movie Duels 的限制进行检查。鼠标和按键与 3ds Max 相同。",
        "ModView のようにモデルを表示します。サーフェス、タグ、ボーン、LOD、スキン、取り付けたセイバー、プライマリとセカンダリのアニメーションを"
        "ゲームと同じ方法で描画し、Jedi Academy・Jedi Outcast・Movie Duels の制限に照らしてチェックします。マウスとキーは 3ds Max と同じです。",
        "Mira un modelo como en ModView (superficies, tags, huesos, LOD, skins, sables acoplados, animación primaria y secundaria) dibujado como "
        "lo dibuja el juego, y compruébalo con los límites de Jedi Academy, Jedi Outcast y Movie Duels. El ratón y las teclas funcionan como en 3ds Max.",
        "Prohlížejte model jako v ModView - povrchy, tagy, kosti, LOD, skiny, připojené meče, primární a sekundární animaci - vykreslený tak, jak "
        "ho kreslí hra, a zkontrolujte ho proti limitům Jedi Academy, Jedi Outcast a Movie Duels. Myš a klávesy fungují jako v 3ds Max."),
    "bild": (
        "Kyle with a saber at *r_hand: on the left the scene explorer and the animations, in the middle the view, on the right the command panel "
        "with its rollouts, below the timeline counted from 0.",
        "Kyle mit einem Säbel an *r_hand: links Szenen-Explorer und Animationen, in der Mitte die Ansicht, rechts das Befehlsfeld mit seinen "
        "Rollouts, unten die Zeitleiste, ab 0 gezählt.",
        "手持光剑 (*r_hand) 的 Kyle:左侧是场景资源管理器和动画,中间是视图,右侧是带卷展栏的命令面板,下方是从 0 开始计数的时间轴。",
        "*r_hand にセイバーを付けた Kyle。左にシーンエクスプローラーとアニメーション、中央にビュー、右にロールアウト付きのコマンドパネル、"
        "下に 0 から数えるタイムライン。",
        "Kyle con un sable en *r_hand: a la izquierda el explorador de escena y las animaciones, en el centro la vista, a la derecha el panel de "
        "comandos con sus persianas, abajo la línea de tiempo contada desde 0.",
        "Kyle s mečem na *r_hand: vlevo průzkumník scény a animace, uprostřed pohled, vpravo panel příkazů s rolety, dole časová osa počítaná od 0."),
    "aufbau": (
        "Open a model as before ({{NpcBrowser}}, `.glm`, `.pk3`). The tab starts in **{{MbBereichAnsehen}}**; the other areas "
        "(**{{MbBereichModellieren}}**, **{{MbBereichUv}}**, **{{MbBereichWeights}}**, **{{MbBereichPainting}}**, **{{MbBereichAnimation}}**) come "
        "in the next stages of the model block - their views, explorer and timeline already work. The model is loaded into its own document with "
        "every level of detail; drawing an unchanged model gives exactly the same picture as before (checked with every model under "
        "`models/players` of Jedi Academy, Jedi Outcast and Movie Duels).",
        "Ein Modell öffnest du wie bisher ({{NpcBrowser}}, `.glm`, `.pk3`). Der Tab beginnt in **{{MbBereichAnsehen}}**; die anderen Bereiche "
        "(**{{MbBereichModellieren}}**, **{{MbBereichUv}}**, **{{MbBereichWeights}}**, **{{MbBereichPainting}}**, **{{MbBereichAnimation}}**) kommen "
        "in den nächsten Stufen des Modell-Blocks - Ansichten, Explorer und Zeitleiste gehen dort schon. Das Modell kommt mit allen Detailstufen "
        "in ein eigenes Dokument; ein unverändertes Modell ergibt genau dasselbe Bild wie vorher (geprüft mit jedem Modell unter `models/players` "
        "von Jedi Academy, Jedi Outcast und Movie Duels).",
        "像以前一样打开模型({{NpcBrowser}}、`.glm`、`.pk3`)。选项卡从 **{{MbBereichAnsehen}}** 开始;其他区域(**{{MbBereichModellieren}}**、"
        "**{{MbBereichUv}}**、**{{MbBereichWeights}}**、**{{MbBereichPainting}}**、**{{MbBereichAnimation}}**)将在模型模块的后续阶段提供——"
        "它们的视图、资源管理器和时间轴已经可用。模型连同所有细节级别载入自己的文档;未修改的模型绘制出的画面与以前完全相同(已用 Jedi Academy、"
        "Jedi Outcast 和 Movie Duels 中 `models/players` 下的每个模型检查)。",
        "モデルはこれまでどおり開きます ({{NpcBrowser}}、`.glm`、`.pk3`)。タブは **{{MbBereichAnsehen}}** で始まります。ほかの領域 "
        "(**{{MbBereichModellieren}}**、**{{MbBereichUv}}**、**{{MbBereichWeights}}**、**{{MbBereichPainting}}**、**{{MbBereichAnimation}}**) は"
        "モデルブロックの次のステージで追加されます。ビュー、エクスプローラー、タイムラインはそこでも使えます。モデルはすべての詳細レベルと一緒に"
        "専用のドキュメントに読み込まれ、変更していないモデルは以前とまったく同じ画像になります (Jedi Academy、Jedi Outcast、Movie Duels の "
        "`models/players` にある全モデルで確認)。",
        "Abre un modelo como antes ({{NpcBrowser}}, `.glm`, `.pk3`). La pestaña empieza en **{{MbBereichAnsehen}}**; las demás áreas "
        "(**{{MbBereichModellieren}}**, **{{MbBereichUv}}**, **{{MbBereichWeights}}**, **{{MbBereichPainting}}**, **{{MbBereichAnimation}}**) llegan "
        "en las próximas etapas del bloque de modelos; sus vistas, explorador y línea de tiempo ya funcionan. El modelo se carga en su propio "
        "documento con todos los niveles de detalle; un modelo sin cambios da exactamente la misma imagen que antes (comprobado con cada modelo "
        "de `models/players` de Jedi Academy, Jedi Outcast y Movie Duels).",
        "Model otevřete jako dřív ({{NpcBrowser}}, `.glm`, `.pk3`). Karta začíná v **{{MbBereichAnsehen}}**; ostatní oblasti "
        "(**{{MbBereichModellieren}}**, **{{MbBereichUv}}**, **{{MbBereichWeights}}**, **{{MbBereichPainting}}**, **{{MbBereichAnimation}}**) přijdou "
        "v dalších fázích bloku modelů - jejich pohledy, průzkumník a časová osa už fungují. Model se načte do vlastního dokumentu se všemi "
        "úrovněmi detailu; nezměněný model dává přesně stejný obraz jako dřív (ověřeno s každým modelem v `models/players` z Jedi Academy, "
        "Jedi Outcast a Movie Duels)."),
    "h_explorer": ("Scene explorer", "Szenen-Explorer", "场景资源管理器", "シーンエクスプローラー", "Explorador de escena", "Průzkumník scény"),
    "explorer": (
        "The **{{MbExplorer}}** shows the surfaces (**{{MbNachHierarchie}}** or **{{MbNachEbenen}}**), the tags and the bones of the skeleton; "
        "**{{MbSuche}}** filters by name. Grey surfaces are not drawn. A right-click works like in ModView and like the game "
        "(`G2_SetSurfaceOnOff`, `G2_SetRootSurface`): **{{MbFlAus}}** hides the surface but keeps its children, **{{MbFlAusOhneKinder}}** hides "
        "the whole branch, **{{MbFlWurzel}}** draws only this branch, **{{MbFlUrsprung}}** undoes it all. A skin line `name,*off` switches a "
        "surface off the same way. Bones with an orange dot are looked up by the game code.",
        "Der **{{MbExplorer}}** zeigt die Flächen (**{{MbNachHierarchie}}** oder **{{MbNachEbenen}}**), die Tags und die Knochen des Skeletts; "
        "**{{MbSuche}}** filtert nach Namen. Graue Flächen werden nicht gezeichnet. Ein Rechtsklick wirkt wie in ModView und wie im Spiel "
        "(`G2_SetSurfaceOnOff`, `G2_SetRootSurface`): **{{MbFlAus}}** blendet die Fläche aus, ihre Kinder bleiben, **{{MbFlAusOhneKinder}}** "
        "blendet den ganzen Zweig aus, **{{MbFlWurzel}}** zeichnet nur diesen Zweig, **{{MbFlUrsprung}}** nimmt alles zurück. Eine Skin-Zeile "
        "`name,*off` schaltet eine Fläche genauso ab. Knochen mit orangem Punkt sucht der Spielcode.",
        "**{{MbExplorer}}** 显示面(**{{MbNachHierarchie}}** 或 **{{MbNachEbenen}}**)、标记和骨架的骨骼;**{{MbSuche}}** 按名称过滤。灰色的面不会"
        "被绘制。右键单击的作用与 ModView 和游戏相同 (`G2_SetSurfaceOnOff`、`G2_SetRootSurface`):**{{MbFlAus}}** 隐藏该面但保留其子级,"
        "**{{MbFlAusOhneKinder}}** 隐藏整个分支,**{{MbFlWurzel}}** 只绘制此分支,**{{MbFlUrsprung}}** 全部撤销。皮肤中的 `name,*off` 行也以同样方式"
        "关闭一个面。带橙色圆点的骨骼会被游戏代码查找。",
        "**{{MbExplorer}}** にはサーフェス (**{{MbNachHierarchie}}** または **{{MbNachEbenen}}**)、タグ、スケルトンのボーンが表示されます。"
        "**{{MbSuche}}** で名前を絞り込みます。灰色のサーフェスは描画されません。右クリックは ModView やゲームと同じように働きます "
        "(`G2_SetSurfaceOnOff`、`G2_SetRootSurface`)。**{{MbFlAus}}** はサーフェスを隠して子は残し、**{{MbFlAusOhneKinder}}** は枝全体を隠し、"
        "**{{MbFlWurzel}}** はこの枝だけを描画し、**{{MbFlUrsprung}}** ですべて元に戻します。スキンの `name,*off` 行も同じようにサーフェスを"
        "オフにします。オレンジの点が付いたボーンはゲームコードが参照します。",
        "El **{{MbExplorer}}** muestra las superficies (**{{MbNachHierarchie}}** o **{{MbNachEbenen}}**), los tags y los huesos del esqueleto; "
        "**{{MbSuche}}** filtra por nombre. Las superficies grises no se dibujan. El clic derecho funciona como en ModView y como en el juego "
        "(`G2_SetSurfaceOnOff`, `G2_SetRootSurface`): **{{MbFlAus}}** oculta la superficie pero mantiene sus hijas, **{{MbFlAusOhneKinder}}** "
        "oculta toda la rama, **{{MbFlWurzel}}** dibuja solo esa rama, **{{MbFlUrsprung}}** lo deshace todo. Una línea de skin `name,*off` "
        "apaga una superficie de la misma forma. Los huesos con un punto naranja los busca el código del juego.",
        "**{{MbExplorer}}** ukazuje povrchy (**{{MbNachHierarchie}}** nebo **{{MbNachEbenen}}**), tagy a kosti kostry; **{{MbSuche}}** filtruje "
        "podle názvu. Šedé povrchy se nekreslí. Pravé tlačítko funguje jako v ModView a jako ve hře (`G2_SetSurfaceOnOff`, `G2_SetRootSurface`): "
        "**{{MbFlAus}}** skryje povrch, jeho potomci zůstanou, **{{MbFlAusOhneKinder}}** skryje celou větev, **{{MbFlWurzel}}** kreslí jen tuto "
        "větev, **{{MbFlUrsprung}}** vše vrátí. Řádek skinu `name,*off` vypne povrch stejně. Kosti s oranžovou tečkou hledá kód hry."),
    "bild_menue": ("The right-click menu of a surface.", "Das Rechtsklick-Menü einer Fläche.", "面的右键菜单。", "サーフェスの右クリックメニュー。",
                   "El menú contextual de una superficie.", "Kontextová nabídka povrchu."),
    "anbauen": (
        "Right-click a tag (for example `*r_hand`) or a bone and choose **{{MbAnbauen}}** to attach a model from the game archives - a saber, "
        "a weapon, a head - like `G2API_AttachG2Model`: on a tag it follows the tag triangle, on a bone the bone. The rollout "
        "**{{MbRAnbauten}}** lists them and detaches them again.",
        "Rechtsklick auf einen Tag (etwa `*r_hand`) oder einen Knochen und **{{MbAnbauen}}** baut ein Modell aus den Spielarchiven an - einen "
        "Säbel, eine Waffe, einen Kopf - wie `G2API_AttachG2Model`: an einem Tag folgt es dem Tag-Dreieck, an einem Knochen dem Knochen. Das "
        "Rollout **{{MbRAnbauten}}** listet sie und löst sie wieder.",
        "右键单击标记(例如 `*r_hand`)或骨骼并选择 **{{MbAnbauen}}**,即可从游戏存档附加模型——光剑、武器、头部——与 `G2API_AttachG2Model` 相同:"
        "附加在标记上时跟随标记三角形,附加在骨骼上时跟随骨骼。卷展栏 **{{MbRAnbauten}}** 列出它们并可再次分离。",
        "タグ (たとえば `*r_hand`) かボーンを右クリックして **{{MbAnbauen}}** を選ぶと、ゲームアーカイブからモデル (セイバー、武器、頭など) を "
        "`G2API_AttachG2Model` と同じように取り付けます。タグに付けるとタグの三角形に、ボーンに付けるとボーンに従います。ロールアウト "
        "**{{MbRAnbauten}}** に一覧が出て、取り外せます。",
        "Haz clic derecho en un tag (por ejemplo `*r_hand`) o un hueso y elige **{{MbAnbauen}}** para acoplar un modelo de los archivos del "
        "juego (un sable, un arma, una cabeza) como `G2API_AttachG2Model`: en un tag sigue el triángulo del tag, en un hueso sigue el hueso. "
        "La persiana **{{MbRAnbauten}}** los muestra y los vuelve a desacoplar.",
        "Pravým tlačítkem na tag (třeba `*r_hand`) nebo kost a **{{MbAnbauen}}** připojí model z archivů hry - meč, zbraň, hlavu - jako "
        "`G2API_AttachG2Model`: na tagu sleduje trojúhelník tagu, na kosti kost. Roleta **{{MbRAnbauten}}** je vypíše a znovu odpojí."),
    "h_befehl": ("Command panel", "Befehlsfeld", "命令面板", "コマンドパネル", "Panel de comandos", "Panel příkazů"),
    "befehl": (
        "**{{MbRModell}}**: file, skeleton, numbers, **{{MbAndereGla}}** (GLA override from the archives), **{{MbTexNeu}}** "
        "(<kbd>Alt</kbd>+<kbd>R</kbd>) and **{{MbFoto}}** (the active view, at least 1920 pixels wide). **{{MbRLod}}**: LOD 1-10 or "
        "**{{MbLodAuto}}** - as in the game the distance picks the level (single player: radius 40, r_lodscale 10; multiplayer: radius 64, r_lodscale 5); picmip and "
        "the skins. The display options are in the header of the view (see below).",
        "**{{MbRModell}}**: Datei, Skelett, Zahlen, **{{MbAndereGla}}** (GLA-Override aus den Archiven), **{{MbTexNeu}}** "
        "(<kbd>Alt</kbd>+<kbd>R</kbd>) und **{{MbFoto}}** (die aktive Ansicht, mindestens 1920 Punkte breit). **{{MbRLod}}**: LOD 1-10 oder "
        "**{{MbLodAuto}}** - wie im Spiel wählt der Abstand die Stufe (Einzelspieler: Radius 40, r_lodscale 10; Mehrspieler: Radius 64, r_lodscale 5); Picmip "
        "und die Skins. Die Anzeige-Optionen stehen in der Kopfzeile der Ansicht (siehe unten).",
        "**{{MbRModell}}**:文件、骨架、数字、**{{MbAndereGla}}**(从存档覆盖 GLA)、**{{MbTexNeu}}** (<kbd>Alt</kbd>+<kbd>R</kbd>) 和 "
        "**{{MbFoto}}**(当前视图,至少 1920 像素宽)。**{{MbRLod}}**:LOD 1-10 或 **{{MbLodAuto}}**——与游戏相同,由距离选择级别"
        "(单人模式:半径 40,r_lodscale 10;多人模式:半径 64,r_lodscale 5);picmip 和皮肤。显示选项位于视图标题栏 (见下文)。",
        "**{{MbRModell}}**: ファイル、スケルトン、数値、**{{MbAndereGla}}** (アーカイブからの GLA オーバーライド)、**{{MbTexNeu}}** "
        "(<kbd>Alt</kbd>+<kbd>R</kbd>)、**{{MbFoto}}** (アクティブなビュー、幅 1920 ピクセル以上)。**{{MbRLod}}**: LOD 1-10 か "
        "**{{MbLodAuto}}**。ゲームと同じく距離でレベルが決まります (シングルプレイ: 半径 40、r_lodscale 10、マルチプレイ: 半径 64、r_lodscale 5)。picmip と"
        "スキン。表示オプションはビューのヘッダーにあります (下記参照)。",
        "**{{MbRModell}}**: archivo, esqueleto, cifras, **{{MbAndereGla}}** (GLA override desde los archivos), **{{MbTexNeu}}** "
        "(<kbd>Alt</kbd>+<kbd>R</kbd>) y **{{MbFoto}}** (la vista activa, al menos 1920 píxeles de ancho). **{{MbRLod}}**: LOD 1-10 o "
        "**{{MbLodAuto}}**: como en el juego, la distancia elige el nivel (un jugador: radio 40, r_lodscale 10; multijugador: radio 64, r_lodscale 5); "
        "picmip y las skins. Las opciones de visualización están en la cabecera de la vista (ver más abajo).",
        "**{{MbRModell}}**: soubor, kostra, čísla, **{{MbAndereGla}}** (přepsání GLA z archivů), **{{MbTexNeu}}** (<kbd>Alt</kbd>+<kbd>R</kbd>) "
        "a **{{MbFoto}}** (aktivní pohled, alespoň 1920 bodů široký). **{{MbRLod}}**: LOD 1-10 nebo **{{MbLodAuto}}** - jako ve hře volí "
        "úroveň vzdálenost (jeden hráč: poloměr 40, r_lodscale 10; více hráčů: poloměr 64, r_lodscale 5); picmip a skiny. Možnosti zobrazení jsou v záhlaví pohledu (viz níže)."),
    "sequenzen": (
        "Click an animation in **{{ModelAnimations}}** to play it as **{{MbPrimaer}}** (whole body). Right-click another one and choose "
        "**{{MbSekundaer}}** to play it on the upper body from `lower_lumbar` up, like `SET_ANIM_UPPER` in the game (another start bone: "
        "right-click a bone). **{{MbInterpolation}}** blends between two frames like the game. The timeline counts frames from 0 like 3ds Max "
        "and the `animation.cfg`.",
        "Ein Klick auf eine Animation in **{{ModelAnimations}}** spielt sie als **{{MbPrimaer}}** (ganzer Körper). Rechtsklick auf eine andere "
        "und **{{MbSekundaer}}** spielt sie auf dem Oberkörper ab `lower_lumbar`, wie `SET_ANIM_UPPER` im Spiel (ein anderer Start-Knochen: "
        "Rechtsklick auf einen Knochen). **{{MbInterpolation}}** mischt zwischen zwei Bildern wie das Spiel. Die Zeitleiste zählt die Bilder ab 0 "
        "wie 3ds Max und die `animation.cfg`.",
        "单击 **{{ModelAnimations}}** 中的动画即可作为 **{{MbPrimaer}}**(全身)播放。右键单击另一个动画并选择 **{{MbSekundaer}}**,即可像游戏中的 "
        "`SET_ANIM_UPPER` 一样在 `lower_lumbar` 以上的上半身播放(其他起始骨骼:右键单击骨骼)。**{{MbInterpolation}}** 像游戏一样在两帧之间混合。"
        "时间轴像 3ds Max 和 `animation.cfg` 一样从 0 开始计帧。",
        "**{{ModelAnimations}}** のアニメーションをクリックすると **{{MbPrimaer}}** (全身) として再生します。別のアニメーションを右クリックして "
        "**{{MbSekundaer}}** を選ぶと、ゲームの `SET_ANIM_UPPER` と同じく `lower_lumbar` から上の上半身で再生します (別の開始ボーン: ボーンを"
        "右クリック)。**{{MbInterpolation}}** はゲームと同じく 2 つのフレームの間を補間します。タイムラインは 3ds Max や `animation.cfg` と同じく"
        "フレームを 0 から数えます。",
        "Un clic en una animación de **{{ModelAnimations}}** la reproduce como **{{MbPrimaer}}** (todo el cuerpo). Clic derecho en otra y "
        "**{{MbSekundaer}}** la reproduce en el torso desde `lower_lumbar`, como `SET_ANIM_UPPER` en el juego (otro hueso de inicio: clic "
        "derecho en un hueso). **{{MbInterpolation}}** mezcla entre dos fotogramas como el juego. La línea de tiempo cuenta los fotogramas "
        "desde 0 como 3ds Max y el `animation.cfg`.",
        "Klik na animaci v **{{ModelAnimations}}** ji přehraje jako **{{MbPrimaer}}** (celé tělo). Pravé tlačítko na jinou a **{{MbSekundaer}}** "
        "ji přehraje na horní části těla od `lower_lumbar`, jako `SET_ANIM_UPPER` ve hře (jiná počáteční kost: pravé tlačítko na kost). "
        "**{{MbInterpolation}}** prolíná mezi dvěma snímky jako hra. Časová osa počítá snímky od 0 jako 3ds Max a `animation.cfg`."),
    "h_pruefen": ("Check for the game", "Prüfen fürs Spiel", "针对游戏检查", "ゲーム向けチェック", "Comprobar para el juego", "Kontrola pro hru"),
    "pruefen": (
        "**{{MbRPruefen}}** checks the model against the target game: vertices and triangles per surface (Jedi Academy and Jedi Outcast stop "
        "with an error at 1000 vertices, Movie Duels at 4000), 32 bones per surface, 4 weights per vertex, the Ghoul2 instances "
        "(`MAX_G2_MODELS`), every line of the `animation.cfg` against the frames of the GLA (behind the end the figure stands in T-pose in "
        "OpenJK-based games, the original keeps it standing) and surfaces without a texture or shader.",
        "**{{MbRPruefen}}** prüft das Modell gegen das Zielspiel: Ecken und Dreiecke je Fläche (Jedi Academy und Jedi Outcast brechen bei 1000 "
        "Ecken mit einem Fehler ab, Movie Duels bei 4000), 32 Knochen je Fläche, 4 Gewichte je Ecke, die Ghoul2-Instanzen (`MAX_G2_MODELS`), "
        "jede Zeile der `animation.cfg` gegen die Bilder der GLA (dahinter steht die Figur in Spielen auf OpenJK-Basis in T-Pose, das Original "
        "lässt sie stehen) und Flächen ohne Textur oder Shader.",
        "**{{MbRPruefen}}** 针对目标游戏检查模型:每个面的顶点和三角形(Jedi Academy 和 Jedi Outcast 在 1000 个顶点时报错中止,Movie Duels 为 "
        "4000)、每个面 32 根骨骼、每个顶点 4 个权重、Ghoul2 实例 (`MAX_G2_MODELS`)、`animation.cfg` 的每一行对照 GLA 的帧数(超出末尾时,在基于 "
        "OpenJK 的游戏中角色会呈 T 姿势,原版则让角色保持站立),以及没有纹理或着色器的面。",
        "**{{MbRPruefen}}** は対象ゲームに照らしてモデルをチェックします。サーフェスごとの頂点と三角形 (Jedi Academy と Jedi Outcast は頂点 1000 で"
        "エラー停止、Movie Duels は 4000)、サーフェスごとにボーン 32、頂点ごとにウェイト 4、Ghoul2 インスタンス (`MAX_G2_MODELS`)、"
        "`animation.cfg` の各行と GLA のフレーム数 (末尾を超えると OpenJK ベースのゲームではフィギュアが T ポーズになり、オリジナルでは立ったまま)、"
        "テクスチャやシェーダーのないサーフェス。",
        "**{{MbRPruefen}}** comprueba el modelo con el juego de destino: vértices y triángulos por superficie (Jedi Academy y Jedi Outcast se "
        "detienen con un error a los 1000 vértices, Movie Duels a los 4000), 32 huesos por superficie, 4 pesos por vértice, las instancias "
        "Ghoul2 (`MAX_G2_MODELS`), cada línea del `animation.cfg` frente a los fotogramas del GLA (más allá del final la figura queda en pose T "
        "en los juegos basados en OpenJK, el original la deja de pie) y superficies sin textura ni shader.",
        "**{{MbRPruefen}}** kontroluje model proti cílové hře: vrcholy a trojúhelníky na povrch (Jedi Academy a Jedi Outcast skončí chybou při "
        "1000 vrcholech, Movie Duels při 4000), 32 kostí na povrch, 4 váhy na vrchol, instance Ghoul2 (`MAX_G2_MODELS`), každý řádek "
        "`animation.cfg` proti snímkům GLA (za koncem stojí postava ve hrách založených na OpenJK v pozici T, originál ji nechá stát) a povrchy "
        "bez textury nebo shaderu."),
    "h_max": ("Controls like 3ds Max", "Bedienung wie 3ds Max", "像 3ds Max 一样操作", "3ds Max と同じ操作", "Controles como 3ds Max",
              "Ovládání jako 3ds Max"),
    "max": (
        "**{{MbBedienung}}** **{{MbWieMax}}** is the default; the key map **{{MbWieBlender}}** comes in stage 1a-2. The keys work after a click "
        "into the view, the explorer, the command panel or the timeline; <kbd>X</kbd> lists them all. The gizmo moves, rotates or scales the "
        "selected surfaces in the model document (all levels of detail) - <kbd>Ctrl</kbd>+<kbd>Z</kbd> takes it back.",
        "**{{MbBedienung}}** **{{MbWieMax}}** ist die Vorgabe; die Belegung **{{MbWieBlender}}** kommt in Stufe 1a-2. Die Tasten wirken nach "
        "einem Klick in die Ansicht, den Explorer, das Befehlsfeld oder die Zeitleiste; <kbd>X</kbd> listet sie alle. Das Gizmo verschiebt, "
        "dreht oder skaliert die gewählten Flächen im Modelldokument (alle Detailstufen) - <kbd>Strg</kbd>+<kbd>Z</kbd> nimmt es zurück.",
        "**{{MbBedienung}}** 默认为 **{{MbWieMax}}**;**{{MbWieBlender}}** 的键位将在 1a-2 阶段提供。在视图、资源管理器、命令面板或时间轴中单击后按键"
        "生效;<kbd>X</kbd> 列出全部按键。Gizmo 在模型文档中移动、旋转或缩放所选的面(所有细节级别)——<kbd>Ctrl</kbd>+<kbd>Z</kbd> 可撤销。",
        "**{{MbBedienung}}** は **{{MbWieMax}}** が既定です。**{{MbWieBlender}}** のキー割り当てはステージ 1a-2 で追加されます。キーはビュー、"
        "エクスプローラー、コマンドパネル、タイムラインのいずれかをクリックした後に効きます。<kbd>X</kbd> で一覧を表示します。ギズモはモデル"
        "ドキュメント内の選択サーフェスを移動・回転・スケールします (すべての詳細レベル)。<kbd>Ctrl</kbd>+<kbd>Z</kbd> で元に戻せます。",
        "**{{MbBedienung}}** **{{MbWieMax}}** es el predeterminado; el mapa de teclas **{{MbWieBlender}}** llega en la etapa 1a-2. Las teclas "
        "funcionan tras un clic en la vista, el explorador, el panel de comandos o la línea de tiempo; <kbd>X</kbd> las muestra todas. El "
        "gizmo mueve, rota o escala las superficies seleccionadas en el documento del modelo (todos los niveles de detalle); "
        "<kbd>Ctrl</kbd>+<kbd>Z</kbd> lo deshace.",
        "**{{MbBedienung}}** **{{MbWieMax}}** je výchozí; klávesy **{{MbWieBlender}}** přijdou ve fázi 1a-2. Klávesy fungují po kliknutí do "
        "pohledu, průzkumníku, panelu příkazů nebo časové osy; <kbd>X</kbd> je vypíše všechny. Gizmo posouvá, otáčí nebo mění měřítko vybraných "
        "povrchů v dokumentu modelu (všechny úrovně detailu) - <kbd>Ctrl</kbd>+<kbd>Z</kbd> to vrátí."),
    "k_maus": ("Middle mouse / Alt + middle / wheel", "Mittlere Maus / Alt + Mitte / Rad", "鼠标中键 / Alt + 中键 / 滚轮", "中ボタン / Alt + 中ボタン / ホイール",
               "Botón central / Alt + central / rueda", "Prostřední tlačítko / Alt + prostřední / kolečko"),
    "k_maus_t": ("Pan / orbit / zoom (Ctrl + Alt + middle: zoom by dragging)", "Schieben / kreisen / zoomen (Strg + Alt + Mitte: zoomen durch Ziehen)",
                 "平移 / 环绕 / 缩放 (Ctrl + Alt + 中键:拖动缩放)", "パン / 周回 / ズーム (Ctrl + Alt + 中ボタン: ドラッグでズーム)",
                 "Desplazar / orbitar / zoom (Ctrl + Alt + central: zoom arrastrando)", "Posun / kroužení / zoom (Ctrl + Alt + prostřední: zoom tažením)"),
    "k_links": ("Left click / drag", "Linksklick / ziehen", "左键单击 / 拖动", "左クリック / ドラッグ", "Clic izquierdo / arrastrar", "Levý klik / tažení"),
    "k_links_t": ("Select a surface / select with a frame (Ctrl adds, Alt removes)", "Fläche wählen / mit Rahmen wählen (Strg dazu, Alt weg)",
                  "选择面 / 框选 (Ctrl 添加,Alt 移除)", "サーフェスを選択 / 枠で選択 (Ctrl で追加、Alt で除外)",
                  "Seleccionar una superficie / seleccionar con un marco (Ctrl añade, Alt quita)", "Vybrat povrch / vybrat rámečkem (Ctrl přidá, Alt odebere)"),
    "k_qwer": ("Q / W / E / R", "Q / W / E / R", "Q / W / E / R", "Q / W / E / R", "Q / W / E / R", "Q / W / E / R"),
    "k_qwer_t": ("Select, move, rotate, scale (F5 / F6 / F7 restrict to X / Y / Z, F8 planes, + / - gizmo size)",
                 "Auswählen, verschieben, drehen, skalieren (F5 / F6 / F7 auf X / Y / Z beschränken, F8 Ebenen, + / - Gizmo-Größe)",
                 "选择、移动、旋转、缩放 (F5 / F6 / F7 限制到 X / Y / Z,F8 平面,+ / - Gizmo 大小)",
                 "選択、移動、回転、スケール (F5 / F6 / F7 で X / Y / Z に拘束、F8 平面、+ / - ギズモの大きさ)",
                 "Seleccionar, mover, rotar, escalar (F5 / F6 / F7 restringen a X / Y / Z, F8 planos, + / - tamaño del gizmo)",
                 "Výběr, posun, otočení, měřítko (F5 / F6 / F7 omezí na X / Y / Z, F8 roviny, + / - velikost gizma)"),
    "k_ansicht": ("Alt+W / P T F L B / Z", "Alt+W / P T F L B / Z", "Alt+W / P T F L B / Z", "Alt+W / P T F L B / Z", "Alt+W / P T F L B / Z",
                  "Alt+W / P T F L B / Z"),
    "k_ansicht_t": ("Four views or one / perspective, top, front, left, bottom / zoom extents (selection)",
                    "Vier Ansichten oder eine / Perspektive, oben, vorn, links, unten / Zoom Grenzen (Auswahl)",
                    "四个视图或一个 / 透视、顶、前、左、底 / 最大化显示 (所选)", "4 ビューか 1 ビュー / パース、上、前、左、下 / 全体表示 (選択)",
                    "Cuatro vistas o una / perspectiva, superior, frontal, izquierda, inferior / encuadrar (selección)",
                    "Čtyři pohledy nebo jeden / perspektiva, shora, zepředu, zleva, zespodu / přiblížit vše (výběr)"),
    "k_anzeige": ("F3 / F4 / G / J / 7", "F3 / F4 / G / J / 7", "F3 / F4 / G / J / 7", "F3 / F4 / G / J / 7", "F3 / F4 / G / J / 7", "F3 / F4 / G / J / 7"),
    "k_anzeige_t": ("Wireframe, edged faces, grid, selection bracket, statistics", "Drahtgitter, Flächen mit Kanten, Raster, Auswahlklammer, Statistik",
                    "线框、带边面、网格、选择框、统计", "ワイヤーフレーム、エッジ面、グリッド、選択ブラケット、統計",
                    "Alambre, caras con aristas, rejilla, corchetes de selección, estadísticas", "Drátový model, plochy s hranami, mřížka, závorky výběru, statistika"),
    "k_zeit": ("/ , . Home End", "/ , . Pos1 Ende", "/ , . Home End", "/ , . Home End", "/ , . Inicio Fin", "/ , . Home End"),
    "k_zeit_t": ("Play/stop, one frame back/forward, first/last frame", "Abspielen/anhalten, ein Bild zurück/vor, erstes/letztes Bild",
                 "播放/停止、后退/前进一帧、首帧/末帧", "再生/停止、1 フレーム戻る/進む、最初/最後のフレーム",
                 "Reproducir/detener, un fotograma atrás/adelante, primer/último fotograma", "Přehrát/zastavit, o snímek zpět/vpřed, první/poslední snímek"),
    "k_auswahl": ("Ctrl+A / Ctrl+D / Ctrl+I / H", "Strg+A / Strg+D / Strg+I / H", "Ctrl+A / Ctrl+D / Ctrl+I / H", "Ctrl+A / Ctrl+D / Ctrl+I / H",
                  "Ctrl+A / Ctrl+D / Ctrl+I / H", "Ctrl+A / Ctrl+D / Ctrl+I / H"),
    "k_auswahl_t": ("Select all, none, invert; select by name", "Alles, nichts, umkehren; nach Namen auswählen", "全选、取消、反选;按名称选择",
                    "すべて選択、解除、反転。名前で選択", "Seleccionar todo, nada, invertir; seleccionar por nombre", "Vybrat vše, nic, obrátit; vybrat podle názvu"),
    "bild_vier": ("Four views (Alt+W) with the bones as bone objects.", "Vier Ansichten (Alt+W) mit den Knochen als Bone-Objekte.",
                  "四个视图 (Alt+W),骨骼显示为骨骼对象。", "4 ビュー (Alt+W)。ボーンはボーンオブジェクトとして表示。",
                  "Cuatro vistas (Alt+W) con los huesos como objetos hueso.", "Čtyři pohledy (Alt+W) s kostmi jako objekty kostí."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def seite():
    """Die Seite modell-ansehen (Gruppe Missionen, nach "Modellansicht und Figuren")."""
    return {
        "id": "modell-ansehen",
        "gruppe": "mission",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("img", "h_mb1_ansehen", _t("bild"), "breit"),
            ("p", _t("aufbau")),
            ("h2", "explorer", _t("h_explorer")),
            ("p", _t("explorer")),
            ("img", "h_mb1_menue", _t("bild_menue")),
            ("p", _t("anbauen")),
            ("h2", "befehlsfeld", _t("h_befehl")),
            ("p", _t("befehl")),
            ("p", _t("sequenzen")),
        ] + inhalt_mbkopf.bloecke() + inhalt_mbraster.bloecke() + [
            ("h2", "pruefen", _t("h_pruefen")),
            ("p", _t("pruefen")),
            ("h2", "max", _t("h_max")),
            ("p", _t("max")),
            ("keys", [
                (_t("k_maus"), _t("k_maus_t")),
                (_t("k_links"), _t("k_links_t")),
                (_t("k_qwer"), _t("k_qwer_t")),
                (_t("k_ansicht"), _t("k_ansicht_t")),
                (_t("k_anzeige"), _t("k_anzeige_t")),
                (_t("k_zeit"), _t("k_zeit_t")),
                (_t("k_auswahl"), _t("k_auswahl_t")),
            ]),
            ("img", "h_mb1_vier", _t("bild_vier"), "breit"),
        ],
    }
