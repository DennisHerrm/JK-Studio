"""Karte und Szene: Karte, Entities, Wegpunkte (dazu inhalt_szene)."""
from inhalt_basis import T
import inhalt_szene

KARTE = {
    "id": "karte",
    "gruppe": "karte",
    "titel": T("The map", "Die Karte", "地图", "マップ"),
    "kurz": T(
        "The map in 3D, drawn like the game draws it - with lights, glow, fog, effects and the figures of the script.",
        "Die Karte in 3D, gezeichnet wie im Spiel - mit Licht, Glühen, Nebel, Effekten und den Figuren des Skripts.",
        "以游戏的方式绘制的三维地图——包含光照、辉光、雾、特效以及脚本中的角色。",
        "ゲームと同じように描かれる 3D マップ。ライト、グロー、霧、エフェクト、スクリプトのキャラクター付き。"),
    "bloecke": [
        ("img", "h_karte", T(
            "The map view with its toolbar on top and the placement tiles on the left.",
            "Die Kartenansicht mit ihrer Leiste oben und den Platzier-Kacheln links.",
            "地图视图:顶部为工具栏,左侧为放置图块。",
            "マップビュー。上にツールバー、左に配置タイル。"), "breit"),
        ("h2", "laden", T("Opening a map", "Eine Karte öffnen", "打开地图", "マップを開く")),
        ("p", T(
            "Switch to **{{ViewMap}}** in the mode bar. A mission brings its map along ([[missionen]]). Without a mission, **{{MapMore}}** opens a map from disk or from a `.pk3`, or loads a separate `.ent` file. **Scripts (N)** lists the scripts the map's entities point to - pick one to load its cutscene.",
            "Schalte in der Modusleiste auf **{{ViewMap}}**. Eine Mission bringt ihre Karte mit ([[missionen]]). Ohne Mission öffnet **{{MapMore}}** eine Karte von der Platte oder aus einem `.pk3` oder lädt eine eigene `.ent`-Datei. **Scripts (N)** nennt die Skripte, auf die die Entities der Karte zeigen - eines wählen lädt seine Zwischensequenz.",
            "在模式栏切换到 **{{ViewMap}}**。任务会自带地图([[missionen]])。没有任务时,**{{MapMore}}** 可从磁盘或 `.pk3` 打开地图,或加载单独的 `.ent` 文件。**Scripts (N)** 列出地图实体所引用的脚本——选择一个即可加载其过场动画。",
            "モードバーで **{{ViewMap}}** に切り替えます。ミッションはマップも一緒に開きます([[missionen]])。ミッションなしなら **{{MapMore}}** でディスクや `.pk3` からマップを開くか、別の `.ent` ファイルを読み込みます。**Scripts (N)** はマップのエンティティが参照するスクリプトの一覧で、選ぶとそのカットシーンを読み込みます。")),
        ("h2", "bewegen", T("Moving around", "Sich bewegen", "视角操作", "視点の操作")),
        ("keys", [
            (T("W A S D (mouse over the view)", "W A S D (Maus über der Ansicht)", "W A S D(鼠标位于视图上)", "W A S D(マウスがビュー上)"), T("Fly; <kbd>Q</kbd> / <kbd>E</kbd> down and up, <kbd>Shift</kbd> faster", "Fliegen; <kbd>Q</kbd> / <kbd>E</kbd> runter und hoch, <kbd>Umschalt</kbd> schneller", "飞行;<kbd>Q</kbd> / <kbd>E</kbd> 下降和上升,<kbd>Shift</kbd> 加速", "飛行。<kbd>Q</kbd> / <kbd>E</kbd> で下降・上昇、<kbd>Shift</kbd> で高速")),
            (T("Right mouse drag", "Rechte Maus ziehen", "按住右键拖动", "右ドラッグ"), T("Look around", "Umsehen", "环顾四周", "見回す")),
            (T("Middle mouse drag", "Mittlere Maus ziehen", "按住中键拖动", "中ドラッグ"), T("Pan", "Verschieben", "平移", "パン")),
            (T("Alt + middle mouse", "Alt + mittlere Maus", "Alt + 鼠标中键", "Alt + 中ボタン"), T("Orbit", "Umkreisen", "环绕", "周回")),
            (T("Wheel", "Mausrad", "滚轮", "ホイール"), T("Zoom", "Zoomen", "缩放", "ズーム")),
            (T("Double-click an entity in the list", "Doppelklick auf eine Entity in der Liste", "在列表中双击实体", "一覧でエンティティをダブルクリック"), T("Fly there", "Hinfliegen", "飞到该处", "そこへ移動")),
            (T("Ctrl + 1 ... 9", "Strg + 1 ... 9", "Ctrl + 1 ... 9", "Ctrl + 1 ... 9"), T("Save the view; <kbd>1</kbd> ... <kbd>9</kbd> jumps back (kept per map)", "Ansicht merken; <kbd>1</kbd> ... <kbd>9</kbd> springt zurück (je Karte)", "保存视角;<kbd>1</kbd> ... <kbd>9</kbd> 跳回(按地图保存)", "視点を保存。<kbd>1</kbd>〜<kbd>9</kbd> で戻る(マップごと)")),
        ]),
        ("h2", "anzeige", T("What is shown", "Was gezeigt wird", "显示内容", "表示内容")),
        ("p", T(
            "The buttons on the right of the toolbar switch parts of the picture on and off: entities, names, the figures of the script, effects, glow, dynamic light, fog and sky. **{{MapTopView}}** looks straight down like a map and cuts away everything above the cut height - drag the height box to look at another floor. **{{MapViewSettings}}** (gear) holds brightness, ambient light, detail, character shadows, **{{MapScriptFocus}}**, which dims every entity the active script does not use, and **{{GameScreen}}**: the game's aspect ratio and cg_fovAspectAdjust, which decide how wide every camera sees ([[kamera#vorschau]]).",
            "Die Knöpfe rechts in der Leiste schalten Teile des Bildes an und aus: Entities, Namen, die Figuren des Skripts, Effekte, Glühen, dynamisches Licht, Nebel und Himmel. **{{MapTopView}}** schaut wie eine Landkarte senkrecht hinunter und schneidet alles über der Schnitthöhe weg - die Höhe ziehen zeigt ein anderes Stockwerk. **{{MapViewSettings}}** (Zahnrad) enthält Helligkeit, Umgebungslicht, Detail, Figurenschatten, **{{MapScriptFocus}}**, das alle Entities abblendet, die das aktive Skript nicht benutzt, und **{{GameScreen}}**: Seitenverhältnis und cg_fovAspectAdjust des Spiels, die bestimmen, wie weit jede Kamera sieht ([[kamera#vorschau]]).",
            "工具栏右侧的按钮用于开关画面中的各部分:实体、名称、脚本角色、特效、辉光、动态光、雾和天空。**{{MapTopView}}** 像地图一样垂直向下看,并剖去剖切高度以上的一切——拖动高度框可查看其他楼层。**{{MapViewSettings}}**(齿轮)包含亮度、环境光、细节、角色阴影、**{{MapScriptFocus}}**(淡化当前脚本未使用的所有实体)以及 **{{GameScreen}}**:游戏的宽高比和 cg_fovAspectAdjust,它们决定每台摄像机能看多宽([[kamera#vorschau]])。",
            "ツールバー右のボタンで画面の要素を切り替えます。エンティティ、名前、スクリプトのキャラクター、エフェクト、グロー、動的ライト、霧、空。**{{MapTopView}}** は地図のように真上から見下ろし、切断高さより上を取り除きます。高さの欄をドラッグすると別の階を見られます。**{{MapViewSettings}}**(歯車)には明るさ、環境光、詳細度、キャラクターの影、アクティブなスクリプトが使わないエンティティを暗くする **{{MapScriptFocus}}**、そして **{{GameScreen}}**(各カメラがどれだけ広く映すかを決める、ゲームのアスペクト比と cg_fovAspectAdjust)があります([[kamera#vorschau]])。")),
        ("img", "h_ansicht_einst", T("{{MapViewSettings}}.", "{{MapViewSettings}}.", "{{MapViewSettings}}。", "{{MapViewSettings}}。")),
        ("note", T(
            "The picture is meant to look like the game: shaders, lightmaps, glow (like `r_DynamicGlow` - on the map, on brush entities and models, on the figures' glowing parts such as droid eyes, on effects like fire and sparks and around lightsaber blades), fog (`r_drawfog`), dynamic lights, lens flares, grass, bushes and rain splashes on the ground (`surfaceSprites` - blade for blade where the game puts them, swaying and fading out with distance, also on brush entities such as a wall that appears when it is used; rain splashes only while it rains), weather (under Movie Duels also the lava of fx_lava) and saber trails, sparks and burn marks ([[figuren#schwerter]]). Weapons, medpacks, shields and ammo lie where the game drops them (items with a `targetname` appear only when used), and floor charging stations, security panels, wall cameras, turrets, emplaced guns, E-Webs, sentry guns and the ion cannon stand with the models the game gives them (a sentry gun unfolds one second after the map starts or after it is switched on and sweeps slowly from side to side while it has no target; sentry guns, turrets and - under Movie Duels - misc_turretG2 look for the figures of the scene like in the game, turn towards one, fire and hurt it: bolts with muzzle flash and impact, damage by where they hit (Jedi Outcast and Movie Duels by their own rules), and what they destroy stays destroyed - a figure dies, another turret, a breakable or a laser trip mine goes off and its target fires. SET_NOTARGET and SET_PLAYER_TEAM count; the player is a target only if the scene contains him. The ion cannon fires on its own - or after a use - with its muzzle flash and recoil, at twice its model size like in the game); laser trip mines hang on their wall and, when switched on, draw their beam and hum (with the warning sound when they arm); a script or trigger switches them on or off, the beam stops at doors and other moving parts where they are at that moment, at other trip mines and at solid breakable models, and a figure walking into the beam - or only the weapon in its hand - sets the mine off - explosion, then the mine is gone; like the game's blast damage it hurts the figures near it (the closer, the more; not through walls), and a figure whose health runs out dies like in a deadly area ([[figuren]]); like in the game the mine uses its targets, so doors, walls or a counter behind it switch, and scripts behind it run (the usescript of a counter, a script runner) - with one count for scripts and movers. An exploding mine also sets off the switched-on mines near it (mines hung close together go off together). Figures show every stage of their shader - shine, chrome (also on the weapon or saber hilt in their hand, turning with the figure as in the game), tinted parts such as the battle droids' paint, the black visors of the clone troopers, blinking and animated stages, moving texture stages (tcMod) such as the shimmer of the Emperor's hologram, and surfaces their shader moves with `deformVertexes` wave, bulge or move, such as the smoke around the shadow Yoda (md_yoda_shadow). Effects are drawn with every stage of their shader too - an explosion blends its two animated stages as in the game, and every stage, the first one included, takes the particle's colour and fading only where its shader asks for it (`rgbGen vertex`, `alphaGen vertex`); otherwise it shows its image as it is, like the game; an effect image without a shader script is drawn like the game's built-in shader for it - blended by its alpha with the particle's colour and fading, and visible through walls. Models that effects carry (`useModel`, such as the fog wisps on duel_exegol) are drawn too and turn towards the camera where their shader says `deformVertexes autosprite`. Beams of fx_target_beam fire towards their target like in the game, also when a script or trigger switches them on or off (a one-shot beam fires once per use); each shot aims at where the target is at that moment - a beam on a moving train follows it -, stops at doors, moving parts and figures in its way, and a beam switched back on waits out its pause (`wait`) like the game. **{{MapStats}}** shows frame rate, textures and graphics card values when something looks wrong.",
            "Das Bild soll aussehen wie im Spiel: Shader, Lightmaps, Glühen (wie `r_DynamicGlow` - auf der Karte, auf Brush-Entities und Modellen, an leuchtenden Teilen der Figuren wie Droidenaugen, an Effekten wie Feuer und Funken und um Lichtschwertklingen), Nebel (`r_drawfog`), dynamische Lichter, Lens-Flares, Gras, Büsche und Regenspritzer auf dem Boden (`surfaceSprites` - Halm für Halm dort, wo das Spiel sie hinsetzt, wiegend und mit der Entfernung ausblendend, auch auf Brush-Entities wie einer Wand, die erst beim Benutzen erscheint; Regenspritzer nur, solange es regnet), Wetter (unter Movie Duels auch die Lava von fx_lava) und Klingenspuren, Funken und Brandspuren ([[figuren#schwerter]]). Waffen, Medipacks, Schilde und Munition liegen dort, wo das Spiel sie fallen lässt (Gegenstände mit `targetname` erscheinen erst, wenn sie benutzt werden), und Ladestationen, Sicherheitspaneele, Wandkameras, Geschütze, Geschützstellungen, E-Webs, Wachgeschütze und die Ionenkanone stehen mit den Modellen, die das Spiel ihnen gibt (ein Wachgeschütz klappt eine Sekunde nach dem Kartenstart oder dem Einschalten auf und schwenkt langsam hin und her, solange es kein Ziel hat; Wachgeschütze, Geschütze und - unter Movie Duels - misc_turretG2 suchen wie im Spiel die Figuren der Szene, drehen sich zu einer hin, feuern und verletzen sie: Geschosse mit Mündungsblitz und Einschlag, Schaden nach Trefferort (Jedi Outcast und Movie Duels nach ihren eigenen Regeln), und was sie zerstören, bleibt zerstört - eine Figur stirbt, ein anderes Geschütz, etwas Zerbrechliches oder eine Laserfalle geht hoch und ihr Ziel wird ausgelöst. SET_NOTARGET und SET_PLAYER_TEAM zählen; der Spieler ist nur Ziel, wenn die Szene ihn enthält. Die Ionenkanone feuert von selbst - oder nach einem use - mit Mündungsfeuer und Rückstoß, wie im Spiel in doppelter Modellgröße); Laserfallen hängen an ihrer Wand und ziehen, wenn sie an sind, ihren Strahl und summen (beim Scharfwerden mit dem Warnklang); ein Skript oder Auslöser schaltet sie ein oder aus, der Strahl endet an Türen und anderen beweglichen Teilen dort, wo sie gerade sind, an anderen Laserfallen und an festen zerbrechlichen Modellen, und läuft eine Figur in den Strahl (oder nur die Waffe in ihrer Hand), geht die Falle hoch - Explosion, danach ist die Falle fort; wie der Explosionsschaden des Spiels verletzt sie die Figuren in ihrer Nähe (je näher, desto mehr; nicht durch Wände), und eine Figur, deren Lebenspunkte aufgebraucht sind, stirbt wie in einem tödlichen Bereich ([[figuren]]); wie im Spiel benutzt die Falle ihre Ziele: Türen, Wände oder ein Zähler dahinter schalten, und Skripte dahinter laufen (das usescript eines Zählers, ein Script-Runner) - mit einem Zählerstand für Skripte und Mover. Eine explodierende Falle sprengt auch die eingeschalteten Fallen in ihrer Nähe mit (dicht beieinander hängende Fallen gehen zusammen hoch). Figuren zeigen alle Stufen ihres Shaders - Glanz, Chrom (auch an Waffe oder Schwertgriff in der Hand, der wie im Spiel mit der Figur dreht), getönte Teile wie die Farbe der Kampfdroiden, die schwarzen Visiere der Klonkrieger, blinkende und animierte Stufen, bewegte Texturstufen (tcMod) wie das Flimmern des Imperator-Hologramms und Flächen, die ihr Shader mit `deformVertexes` wave, bulge oder move bewegt, etwa der Rauch um den Schatten-Yoda (md_yoda_shadow). Auch Effekte zeigen alle Stufen ihres Shaders - eine Explosion überblendet wie im Spiel ihre zwei animierten Stufen, und jede Stufe, auch die erste, nimmt Farbe und Ausblenden des Teilchens nur, wo ihr Shader es verlangt (`rgbGen vertex`, `alphaGen vertex`); sonst zeigt sie wie im Spiel ihr Bild, wie es ist; ein Effektbild ohne Shaderskript zeichnet die Vorschau wie den eingebauten Shader des Spiels dafür - nach seinem Alpha gemischt, mit Farbe und Ausblenden des Teilchens und durch Wände sichtbar. Modelle, die Effekte mitführen (`useModel`, etwa die Nebelschwaden in duel_exegol), werden ebenfalls gezeichnet und drehen sich zur Kamera, wo ihr Shader `deformVertexes autosprite` verlangt. Strahlen von fx_target_beam feuern wie im Spiel auf ihr Ziel, auch wenn ein Skript oder Auslöser sie ein- oder ausschaltet (ein Einzelschuss-Strahl feuert je Benutzung einmal); jeder Schuss zielt dorthin, wo das Ziel gerade ist - ein Strahl auf einen fahrenden Zug folgt ihm -, endet an Türen, beweglichen Teilen und Figuren im Weg, und ein wieder eingeschalteter Strahl wartet wie im Spiel seine Pause (`wait`) ab. **{{MapStats}}** zeigt Bildrate, Texturen und Werte der Grafikkarte, wenn etwas falsch aussieht.",
            "画面力求与游戏一致:着色器、光照贴图、辉光(如 `r_DynamicGlow`——地图、笔刷实体和模型上,角色的发光部位如机器人的眼睛,火焰和火花等特效,以及光剑剑刃周围)、雾(`r_drawfog`)、动态光、镜头光晕、地面上的草、灌木和雨滴溅花(`surfaceSprites`——每一株都在游戏放置的位置,随风摆动并随距离淡出,也出现在笔刷实体上,例如被使用后才出现的墙;雨滴溅花只在下雨时出现)、天气(在 Movie Duels 下也包括 fx_lava 的熔岩)以及光剑轨迹、火花和灼痕([[figuren#schwerter]])。武器、医疗包、护盾和弹药位于游戏让它们落下的位置(带 `targetname` 的物品要在被使用后才出现),充能站、安全面板、墙上摄像机、炮塔、固定炮位、E-Web、哨戒炮和离子炮使用游戏赋予它们的模型(哨戒炮在地图开始或被打开一秒后展开,没有目标时缓慢地左右转动;哨戒炮、炮塔以及 Movie Duels 下的 misc_turretG2 像游戏中一样搜索场景中的角色,转向目标、开火并造成伤害:弹丸带有炮口火光和命中效果,伤害取决于命中部位(Jedi Outcast 和 Movie Duels 各按自己的规则),被摧毁的东西不会恢复——角色死亡,其他炮塔、可破坏物体或激光绊雷会被引爆,并触发它们的目标。SET_NOTARGET 和 SET_PLAYER_TEAM 同样有效;只有场景中包含玩家时,玩家才是目标。离子炮会自行开火——或在 use 之后——带有炮口火光和后坐动作,并像游戏中一样以两倍模型尺寸显示);激光绊雷挂在墙上,开启时会射出光束并发出嗡鸣(启动时伴有警告声);脚本或触发器可以将其打开或关闭,光束会停在门和其他移动部件当时所在的位置、其他激光绊雷以及实体的可破坏模型处,角色走进光束时(哪怕只是手中的武器)绊雷会引爆——爆炸之后绊雷消失;它会像游戏的爆炸伤害一样伤害附近的角色(越近伤害越大;不会穿墙),生命值耗尽的角色会像在致命区域中一样死亡([[figuren]]);与游戏中一样,绊雷会触发它的目标,其后的门、墙或计数器随之切换,其后的脚本(计数器的 usescript、脚本运行器)也会运行——脚本和移动部件共用同一个计数。爆炸的绊雷还会引爆附近已开启的绊雷(挂得很近的绊雷会一起爆炸)。角色会显示其着色器的所有阶段——光泽、镀铬(也包括手中的武器或光剑剑柄,与游戏中一样随角色转动)、着色部分(例如战斗机器人的涂装)、克隆人士兵的黑色面罩以及闪烁和动画阶段、移动的纹理阶段(tcMod),例如皇帝全息影像的闪烁,以及其着色器用 `deformVertexes` wave、bulge 或 move 移动的表面,例如影子尤达(md_yoda_shadow)周围的烟雾。特效同样以其着色器的所有阶段绘制——爆炸会像游戏中一样交叉淡化它的两个动画阶段;每个阶段(包括第一个)只有在其着色器要求时(`rgbGen vertex`、`alphaGen vertex`)才采用粒子的颜色和淡出,否则会像游戏中一样按原样显示其图像;没有着色器脚本的特效图像会像游戏为其内置的着色器那样绘制——按其 alpha 混合,带有粒子的颜色和淡出,并且可以透过墙壁看到。特效携带的模型(`useModel`,例如 duel_exegol 的雾气)也会被绘制,并在其着色器要求 `deformVertexes autosprite` 时朝向镜头。fx_target_beam 的光束会像游戏中一样射向其目标,脚本或触发器将其打开或关闭时也是如此(单发光束每次使用发射一次);每一次射击都瞄准目标当时所在的位置——射向行驶中列车的光束会跟随列车——会停在路径上的门、移动部件和角色处,重新打开的光束会像游戏中一样等待其间隔(`wait`)结束。画面异常时,**{{MapStats}}** 会显示帧率、纹理和显卡参数。",
            "画面はゲームと同じに見えるように作られています。シェーダー、ライトマップ、グロー(`r_DynamicGlow` 相当。マップ、ブラシエンティティやモデル、ドロイドの目などキャラクターの発光部分、炎や火花などのエフェクト、ライトセーバーの刃の周り)、霧(`r_drawfog`)、動的ライト、レンズフレア、地面の草・茂み・雨の跳ね(`surfaceSprites`。ゲームと同じ場所に一本ずつ、風に揺れ、距離で薄れる。使用されて初めて現れる壁などのブラシエンティティ上にも表示され、雨の跳ねは雨が降っている間だけ)、天候(Movie Duels では fx_lava の溶岩も)、セイバーの軌跡・火花・焼け跡([[figuren#schwerter]])。武器、メディパック、シールド、弾薬はゲームが落とす位置に置かれ(`targetname` のあるアイテムは使われたときに現れます)、充電ステーション、セキュリティパネル、壁のカメラ、タレット、据え置き砲、E-Web、セントリーガン、イオン砲はゲームが与えるモデルで表示されます(セントリーガンはマップ開始またはオンにされてから 1 秒後に展開し、標的がない間はゆっくり左右に旋回します。セントリーガン、タレット、Movie Duels の misc_turretG2 はゲームと同じくシーンのキャラクターを探し、そちらを向いて射撃し、ダメージを与えます。弾には発射炎と着弾があり、ダメージは命中部位で変わります(Jedi Outcast と Movie Duels はそれぞれの規則)。破壊されたものは元に戻りません。キャラクターは死に、ほかのタレット、破壊可能物、レーザートリップマインは爆発してターゲットを発動します。SET_NOTARGET と SET_PLAYER_TEAM も有効です。プレイヤーはシーンに含まれているときだけ標的になります。イオン砲は自動で、または use の後に、発射炎と反動つきで発射します。ゲームと同じくモデルは 2 倍の大きさです)。レーザートリップマインは壁に付き、オンのときはビームを出してハム音を鳴らします(起動時には警告音)。スクリプトやトリガーでオン・オフでき、ビームはドアなどの動く部分のその時点の位置、ほかのレーザートリップマイン、固体の壊せるモデルで止まり、キャラクター(手に持った武器だけでも)がビームに入るとマインが爆発して消えます。ゲームの爆発ダメージと同じく近くのキャラクターを傷つけ(近いほど大きく、壁越しには届きません)、体力がなくなったキャラクターは致命的な領域と同じく死にます([[figuren]])。ゲームと同じく、マインはそのターゲットを使うので、その先のドアや壁、カウンターが切り替わり、その先のスクリプト(カウンターの usescript やスクリプトランナー)も実行されます。カウントはスクリプトと可動部で共通です。爆発したマインは近くのオンになっているマインも誘爆させます(近くに並べたマインは一緒に爆発します)。キャラクターはシェーダーのすべてのステージを表示します(光沢、クローム(手に持った武器やセーバーの柄にも。ゲームと同じくキャラクターと一緒に回ります)、バトル・ドロイドの塗装のような色付きの部分、クローン・トルーパーの黒いバイザー、点滅やアニメーションのステージ、皇帝のホログラムのゆらめきのように動くテクスチャステージ(tcMod)、シャドウ・ヨーダ(md_yoda_shadow)の周りの煙のようにシェーダーが `deformVertexes` の wave・bulge・move で動かす面)。エフェクトもシェーダーのすべてのステージで描かれ、爆発はゲームと同じく 2 つのアニメーションステージをクロスフェードします。最初のステージを含む各ステージは、シェーダーが求める場合(`rgbGen vertex`、`alphaGen vertex`)だけパーティクルの色とフェードを使い、それ以外はゲームと同じく画像をそのまま表示します。シェーダースクリプトのないエフェクト画像は、ゲームの組み込みシェーダーと同じく、アルファで合成され、パーティクルの色とフェードを使い、壁越しにも見えるように描画されます。エフェクトが持つモデル(`useModel`、たとえば duel_exegol の霧)も描画され、シェーダーが `deformVertexes autosprite` を指定している場合はカメラの方を向きます。fx_target_beam のビームはゲームと同じくターゲットに向けて発射されます。スクリプトやトリガーでオン・オフしたときも同様です(単発のビームは使用ごとに 1 回発射)。各発射はその時点のターゲットの位置を狙い(走行中の列車へのビームは列車を追います)、途中のドア、動く部分、キャラクターで止まります。再びオンにされたビームはゲームと同じく間隔(`wait`)が過ぎるまで待ちます。おかしく見えるときは **{{MapStats}}** でフレームレート、テクスチャ、グラフィックカードの値を確認できます。")),
        ("note", T(
            "The sky is drawn as in the game: the sky surfaces only decide which rectangle of the sky box is drawn, far behind everything. Through a hole in the map you see the sky if the hole lies in that rectangle, otherwise the empty picture; sky behind a closed door does not count. With a skybox portal the portal scene stays in the picture and the map draws no sky of its own.",
            "Der Himmel entsteht wie im Spiel: Die Himmelsflaechen bestimmen nur, welches Rechteck der Himmelsbox ganz hinten gezeichnet wird. Durch ein Loch in der Karte sieht man den Himmel, wenn es in diesem Rechteck liegt, sonst das leere Bild; Himmel hinter einer geschlossenen Tuer zaehlt nicht. Mit einem Himmelsportal bleibt dessen Szene im Bild, die Karte zeichnet keinen eigenen Himmel.",
            "天空的绘制方式与游戏相同:天空面只决定在最后方绘制天空盒的哪个矩形。透过地图上的洞,如果洞位于该矩形内就能看到天空,否则是空白画面;关闭的门后面的天空不计入。使用天空传送门时,其场景保留在画面中,地图不再绘制自己的天空。",
            "空はゲームと同じように描かれます。空のサーフェスは、スカイボックスのどの長方形を一番奥に描くかを決めるだけです。マップの穴がその長方形の中にあれば空が見え、そうでなければ空の画面になります。閉じたドアの向こうの空は数えません。スカイボックスポータルがある場合はそのシーンが画面に残り、マップは独自の空を描きません。",
        )),
        ("note", T(
            "Under Movie Duels the map's weather file (Weather/<map>.cfg) applies as in the game: embers in the air on duel_mustafar, snow on duel_hoth, rain on duel_kamino. Particles fall at the game's speed and only stay out of solid and water. Guns in holsters hang at the distance from the holster.cfg. Texture detail (r_picmip) halves the textures per step like the cvar and saves video memory; 0 is the default of OpenJK and Movie Duels, the original Jedi Academy starts at 1. Behind a closed door you also no longer see the sky of the area beyond.",
            "Unter Movie Duels gilt die Wetterdatei der Karte (Weather/<Karte>.cfg) wie im Spiel: Glut in der Luft auf duel_mustafar, Schnee auf duel_hoth, Regen auf duel_kamino. Teilchen fallen so schnell wie im Spiel und bleiben nur aus festem Material und Wasser draußen. Waffen im Holster hängen im Abstand aus der holster.cfg. Die Bildstufe (r_picmip) halbiert die Bilder je Stufe wie die Cvar und spart Grafikspeicher; 0 ist die Vorgabe von OpenJK und Movie Duels, das Original-Jedi-Academy beginnt bei 1. Hinter einer geschlossenen Tür sieht man auch den Himmel des Gebiets dahinter nicht mehr.",
            "在 Movie Duels 下,地图的天气文件 (Weather/<地图>.cfg) 与游戏中一样生效:duel_mustafar 上空气中的余烬、duel_hoth 的雪、duel_kamino 的雨。粒子以游戏中的速度下落,只避开固体和水。枪套中的武器按 holster.cfg 的距离悬挂。纹理细节 (r_picmip) 与该变量一样每级将纹理减半并节省显存;0 是 OpenJK 和 Movie Duels 的默认值,原版 Jedi Academy 从 1 开始。在关闭的门后面也不再看到那一区域的天空。",
            "Movie Duels では、マップの天候ファイル (Weather/<マップ>.cfg) がゲームと同じように適用されます。duel_mustafar の空中の火の粉、duel_hoth の雪、duel_kamino の雨です。粒子はゲームと同じ速さで落ち、固体と水の中だけを避けます。ホルスターの武器は holster.cfg の距離で下がります。テクスチャ詳細 (r_picmip) はこの変数と同じく 1 段階ごとにテクスチャを半分にし、ビデオメモリを節約します。0 が OpenJK と Movie Duels の既定値で、オリジナルの Jedi Academy は 1 から始まります。閉じたドアの向こうの区域の空も見えなくなります。",
        )),
        ("note", T(
            "**{{MapMsaa}}** in the view settings smooths the edges of surfaces with several samples per pixel, like r_ext_multisample in the game; textures stay as they are. Like the game is 8 under Movie Duels (asked in the game) and off under OpenJK, the original Jedi Academy and Jedi Outcast. It costs about 0.4 ms per picture at 1350 x 930.",
            "**{{MapMsaa}}** in den Ansichtseinstellungen glättet die Kanten der Flächen mit mehreren Proben je Bildpunkt, wie r_ext_multisample im Spiel; die Texturen bleiben, wie sie sind. Wie das Spiel heißt unter Movie Duels 8 (im Spiel abgefragt) und unter OpenJK, dem Original-Jedi-Academy und Jedi Outcast aus. Es kostet bei 1350 x 930 rund 0,4 ms je Bild.",
            "视图设置中的 **{{MapMsaa}}** 与游戏中的 r_ext_multisample 一样,以每像素多个采样平滑表面边缘;纹理保持不变。“与游戏相同”在 Movie Duels 下为 8(在游戏中查询),在 OpenJK、原版 Jedi Academy 和 Jedi Outcast 下关闭。在 1350 x 930 时每帧约多花 0.4 毫秒。",
            "表示設定の **{{MapMsaa}}** は、ゲームの r_ext_multisample と同じく、ピクセルごとに複数のサンプルで面の縁を滑らかにします。テクスチャはそのままです。「ゲームと同じ」は Movie Duels で 8(ゲーム内で確認)、OpenJK、オリジナルの Jedi Academy、Jedi Outcast ではオフです。1350 x 930 で 1 フレームあたり約 0.4 ms かかります。",
        )),
        ("note", T(
            "Like the game, the view does not draw what lies behind a closed door: doors, a func_wall or func_usable that is switched off and broken breakables open the area portals of the map, and figures behind a closed portal are not drawn either. Brush models without an entity are not drawn at all. Figures react to being hurt: under Movie Duels a shot makes them flinch according to where it hit, even with SET_INVINCIBLE, at most every two seconds; under OpenJK and Jedi Outcast the game's pain rules decide. Blasts push them away (the bolts of turrets never do). A corpse lies along the slope it fell on. Under Movie Duels the guns the player has collected with SET_WEAPON hang on his hips or back as the model's holster.cfg says, and his saber moves to the front of the hip when a gun hangs there. {{MapGlow}} starts on under Movie Duels and off under OpenJK and Jedi Outcast, like `r_DynamicGlow` there.",
            "Wie im Spiel zeichnet die Ansicht nicht, was hinter einer geschlossenen Tür liegt: Türen, ein abgeschaltetes func_wall oder func_usable und zerbrochene Breakables öffnen die Area-Portale der Karte, und Figuren hinter einem geschlossenen Portal fehlen ebenfalls. Brush-Modelle ohne Entity werden gar nicht gezeichnet. Figuren reagieren auf Treffer: Unter Movie Duels zucken sie je nach Trefferort, auch mit SET_INVINCIBLE, höchstens alle zwei Sekunden; unter OpenJK und Jedi Outcast entscheiden die Schmerzregeln des Spiels. Explosionen stoßen sie weg (die Bolzen von Geschützen nie). Eine Leiche liegt am Hang, auf den sie gefallen ist. Unter Movie Duels hängen die Waffen, die der Spieler mit SET_WEAPON gesammelt hat, an Hüften oder Rücken, wie es die holster.cfg des Modells sagt, und sein Schwert rückt nach vorn an die Hüfte, wenn dort eine Waffe hängt. {{MapGlow}} beginnt unter Movie Duels an und unter OpenJK und Jedi Outcast aus, wie `r_DynamicGlow` dort.",
            "与游戏一样,视图不绘制关闭的门后面的内容:门、被关闭的 func_wall 或 func_usable 以及被打碎的可破坏物会打开地图的区域门户,关闭的门户后面的角色也不绘制。没有实体的笔刷模型完全不绘制。角色会对受伤作出反应:在 Movie Duels 下,射击会使其按命中部位抽搐,即使使用 SET_INVINCIBLE 也一样,最多每两秒一次;在 OpenJK 和 Jedi Outcast 下由游戏的疼痛规则决定。爆炸会把他们推开(炮塔的弹丸不会)。尸体顺着所落的斜坡躺下。在 Movie Duels 下,玩家用 SET_WEAPON 收集的武器按模型的 holster.cfg 挂在臀部或背上;如果臀部挂有武器,光剑会移到臀部前方。{{MapGlow}} 在 Movie Duels 下默认开启,在 OpenJK 和 Jedi Outcast 下默认关闭,与那里的 `r_DynamicGlow` 相同。",
            "ゲームと同じく、閉じたドアの向こうにあるものは描画されません。ドア、オフにした func_wall や func_usable、壊れたブレイカブルがマップのエリアポータルを開き、閉じたポータルの向こうのキャラクターも描画されません。エンティティのないブラシモデルは描画されません。キャラクターは被弾に反応します。Movie Duels では、射撃が当たると SET_INVINCIBLE でも当たった場所に応じてびくっとします(最大で 2 秒に 1 回)。OpenJK と Jedi Outcast ではゲームの痛みのルールで決まります。爆発はキャラクターを押しのけます(タレットの弾は押しません)。死体は落ちた斜面に沿って横たわります。Movie Duels では、プレイヤーが SET_WEAPON で集めた武器がモデルの holster.cfg のとおり腰や背中に下がり、腰に武器があるとセーバーは腰の前側に移ります。{{MapGlow}} は Movie Duels では最初からオン、OpenJK と Jedi Outcast ではオフです(そこでの `r_DynamicGlow` と同じ)。",
        )),
        ("img", "geschuetz_feuer", T(
            "A turret and a misc_turretG2 fire at a figure: bolts, muzzle flash and impact like in the game.",
            "Ein Geschütz und ein misc_turretG2 feuern auf eine Figur: Geschosse, Mündungsblitz und Einschlag wie im Spiel.",
            "炮塔和 misc_turretG2 向一个角色开火:弹丸、炮口火光和命中效果与游戏一致。",
            "タレットと misc_turretG2 がキャラクターを撃っています。弾、発射炎、着弾はゲームと同じです。"), "breit"),
    ],
}

ENTITIES = {
    "id": "entities",
    "gruppe": "karte",
    "titel": T("Entities", "Entities", "实体", "エンティティ"),
    "kurz": T(
        "Everything in a map that a script can talk to - figures, walk targets, camera marks, triggers, ships - is an entity. Select it, change it, move it, place new ones.",
        "Alles in einer Karte, das ein Skript ansprechen kann - Figuren, Laufziele, Kameramarken, Auslöser, Schiffe -, ist eine Entity. Wählen, ändern, verschieben, neue setzen.",
        "地图中脚本能够操作的一切——角色、行走目标、摄像机标记、触发器、飞船——都是实体。可以选择、修改、移动,也可以放置新的实体。",
        "スクリプトが扱えるマップ内のもの(キャラクター、歩行目標、カメラマーク、トリガー、船)はすべてエンティティです。選択・変更・移動し、新しく配置できます。"),
    "bloecke": [
        ("img", "h_entity", T(
            "A selected ship: the list on the left, the inspector below it, the gizmo in the view.",
            "Ein gewähltes Schiff: links die Liste, darunter der Inspektor, in der Ansicht das Gizmo.",
            "选中的飞船:左侧是列表,其下是检查器,视图中是操纵器。",
            "選択した船。左に一覧、その下にインスペクター、ビューにギズモ。"), "breit"),
        ("h2", "liste", T("The entity list", "Die Entity-Liste", "实体列表", "エンティティ一覧")),
        ("p", T(
            "The left column sorts the entities into groups - camera marks, walk targets, look targets, characters, script runners, effects, sound, movables, breakables, NPC waypoints. The eye hides a group in the view (<kbd>Alt</kbd>+click: show only this one), the search box filters by name or class. Click selects, double-click flies there, right-click inserts a command for it: walk target, look target, affect block, camera to this mark.",
            "Die linke Spalte ordnet die Entities in Gruppen - Kameramarken, Laufziele, Blickziele, Figuren, Skriptstarter, Effekte, Klang, Bewegliches, Zerbrechliches, NPC-Wegpunkte. Das Auge blendet eine Gruppe in der Ansicht aus (<kbd>Alt</kbd>+Klick: nur diese zeigen), das Suchfeld filtert nach Name oder Klasse. Klick wählt, Doppelklick fliegt hin, Rechtsklick fügt einen Befehl für sie ein: Laufziel, Blickziel, affect-Block, Kamera zu dieser Marke.",
            "左栏把实体分组——摄像机标记、行走目标、注视目标、角色、脚本启动器、特效、声音、可移动物体、可破坏物体、NPC 路径点。眼睛图标可在视图中隐藏某组(<kbd>Alt</kbd>+单击:只显示该组),搜索框按名称或类别筛选。单击选择,双击飞过去,右键为其插入命令:行走目标、注视目标、affect 块、摄像机移到此标记。",
            "左の列はエンティティをグループに分けます。カメラマーク、歩行目標、視線目標、キャラクター、スクリプトランナー、エフェクト、サウンド、可動物、破壊可能物、NPC ウェイポイント。目のアイコンでビューから隠し(<kbd>Alt</kbd>+クリックでそのグループだけ表示)、検索欄で名前やクラスを絞り込みます。クリックで選択、ダブルクリックでそこへ移動、右クリックでそれ向けのコマンド(歩行目標、視線目標、affect ブロック、カメラをこのマークへ)を挿入します。")),
        ("h2", "waehlen", T("Selecting in the view", "In der Ansicht wählen", "在视图中选择", "ビューで選択")),
        ("p", T(
            "Click a marker, a figure or a model to select it; <kbd>Ctrl</kbd>/<kbd>Shift</kbd>+click adds more; a box dragged over empty space selects everything inside. Hovering shows a box and the name. The transport bar under the map says how often the selected entity is used in the script, with buttons that jump to those lines.",
            "Eine Marke, eine Figur oder ein Modell anklicken wählt es; <kbd>Strg</kbd>/<kbd>Umschalt</kbd>+Klick nimmt weitere dazu; ein über leere Fläche gezogener Rahmen wählt alles darin. Darüberfahren zeigt einen Kasten und den Namen. Die Leiste unter der Karte sagt, wie oft die gewählte Entity im Skript vorkommt, mit Knöpfen, die zu diesen Zeilen springen.",
            "单击标记、角色或模型即可选择;<kbd>Ctrl</kbd>/<kbd>Shift</kbd>+单击追加;在空白处拖出框可选择框内所有对象。悬停时显示边框和名称。地图下方的控制栏会显示所选实体在脚本中被使用的次数,并提供跳转到这些行的按钮。",
            "マーカー、キャラクター、モデルをクリックして選択。<kbd>Ctrl</kbd>/<kbd>Shift</kbd>+クリックで追加、何もない所をドラッグした枠で中のものをすべて選択します。マウスを乗せると枠と名前が表示されます。マップ下のバーには、選択中のエンティティがスクリプトで何回使われているかと、その行へ移動するボタンが表示されます。")),
        ("h2", "gizmo", T("Move and Rotate", "Verschieben und Drehen", "移动与旋转", "移動と回転")),
        ("p", T("A small bar in the corner of the view switches the gizmo between **{{GizmoMove}}** and **{{GizmoRotate}}**. In Move you drag the entity over the floor or along one of the coloured arrows; the grid set in the toolbar snaps, <kbd>Ctrl</kbd> turns it off while you drag - without a grid, <kbd>Ctrl</kbd> snaps an arrow drag to 8 units. In Rotate you get rings (<kbd>Ctrl</kbd>: 15-degree steps): blue turns left and right; ships and models also get green (pitch) and red (roll); figures only have the blue ring, because the game only knows `angle` for them. <kbd>Alt</kbd>+drag copies first. Several selected entities move and turn as a group.", "Eine kleine Leiste in der Ecke der Ansicht schaltet das Gizmo zwischen **{{GizmoMove}}** und **{{GizmoRotate}}**. Im Verschieben ziehst du die Entity über den Boden oder an einem der farbigen Pfeile; das in der Leiste eingestellte Raster rastet, <kbd>Strg</kbd> schaltet es beim Ziehen aus - ohne Raster rastet <kbd>Strg</kbd> einen Pfeilzug auf 8 Einheiten. Im Drehen gibt es Ringe (<kbd>Strg</kbd>: 15-Grad-Schritte): blau dreht links und rechts; Schiffe und Modelle haben dazu grün (nicken) und rot (rollen); Figuren nur den blauen, weil das Spiel für sie nur `angle` kennt. <kbd>Alt</kbd>+Ziehen kopiert zuerst. Mehrere gewählte Entities ziehen und drehen als Gruppe.", "视图角落的小工具栏在 **{{GizmoMove}}** 和 **{{GizmoRotate}}** 之间切换操纵器。移动模式下,可在地面上拖动实体或沿彩色箭头拖动;工具栏中设置的网格会吸附,拖动时按 <kbd>Ctrl</kbd> 可暂时关闭——未设置网格时,<kbd>Ctrl</kbd> 让箭头拖动吸附到 8 单位。旋转模式下显示圆环(<kbd>Ctrl</kbd>:15 度一步):蓝色左右旋转;飞船和模型还有绿色(俯仰)和红色(翻滚);角色只有蓝色圆环,因为游戏对角色只认 `angle`。<kbd>Alt</kbd>+拖动会先复制。多个选中的实体作为整体移动和旋转。", "ビューの隅の小さなバーでギズモを **{{GizmoMove}}** と **{{GizmoRotate}}** に切り替えます。移動では床の上をドラッグするか、色付きの矢印に沿ってドラッグします。ツールバーで設定したグリッドにスナップし、ドラッグ中に <kbd>Ctrl</kbd> を押すと解除されます。グリッドがないときは <kbd>Ctrl</kbd> で矢印のドラッグが 8 ユニットにスナップします。回転ではリングが表示されます(<kbd>Ctrl</kbd>:15 度刻み)。青は左右の回転、船とモデルには緑(ピッチ)と赤(ロール)もあります。キャラクターは青だけです。ゲームはキャラクターに `angle` しか使わないからです。<kbd>Alt</kbd>+ドラッグで先にコピーします。複数選択したエンティティはまとめて動き、回転します。")),
        ("imgs", [
            ("h_gizmo_verschieben", T("{{GizmoMove}}: arrows only.", "{{GizmoMove}}: nur Pfeile.", "{{GizmoMove}}:只有箭头。", "{{GizmoMove}}:矢印のみ。")),
            ("h_gizmo_drehen", T("{{GizmoRotate}}: three rings on a ship.", "{{GizmoRotate}}: drei Ringe an einem Schiff.", "{{GizmoRotate}}:飞船上的三个圆环。", "{{GizmoRotate}}:船の 3 つのリング。")),
        ]),
        ("note", T(
            "When the timeline stands in the middle of a movement, the gizmo does not change the start - it sets a new point at that moment. See [[figuren#griffe]] and [[animation#schluessel]].",
            "Steht die Zeitleiste mitten in einer Bewegung, ändert das Gizmo nicht den Anfang - es setzt an diesem Augenblick einen neuen Punkt. Siehe [[figuren#griffe]] und [[animation#schluessel]].",
            "当时间轴位于某段运动中间时,操纵器不会修改起点,而是在该时刻设置新的路径点。参见 [[figuren#griffe]] 和 [[animation#schluessel]]。",
            "タイムラインが動きの途中にあるとき、ギズモは始点を変えず、その瞬間に新しい点を設定します。[[figuren#griffe]] と [[animation#schluessel]] を参照。")),
        ("h2", "inspektor", T("The inspector", "Der Inspektor", "检查器", "インスペクター")),
        ("p", T(
            "Under the list the inspector shows the selected entity: its class with a description from the game code, **{{EntName}}**, **{{EntOrigin}}**, **{{EntFloor}}** (how high the feet stand above the floor - red when inside it) and **{{EntAngle}}**, the spawnflags as checkboxes and all other keys as fields. Script fields have **{{EntOpenScript}}**, `NPC_type` has a search over every NPC of the game; when the class sets the type itself (`NPC_Droid_R2D2` → `r2d2`), the empty field shows it in grey, and a note says when the game chooses at random or ignores the key. The icon bar flies there, puts it **{{EntDropFloor}}**, duplicates, deletes, animates ([[animation]]) and shows **{{EntRaw}}**. Entity changes have their own undo while the mouse is over the map or the list.",
            "Unter der Liste zeigt der Inspektor die gewählte Entity: ihre Klasse mit einer Beschreibung aus dem Spielcode, **{{EntName}}**, **{{EntOrigin}}**, **{{EntFloor}}** (wie hoch die Füße über dem Boden stehen - rot, wenn darin) und **{{EntAngle}}**, die Spawnflags als Kästchen und alle übrigen Keys als Felder. Skriptfelder haben **{{EntOpenScript}}**, `NPC_type` hat eine Suche über alle NPCs des Spiels; setzt die Klasse den Typ selbst (`NPC_Droid_R2D2` → `r2d2`), zeigt das leere Feld ihn grau, und ein Hinweis sagt, wenn das Spiel würfelt oder den Key nicht beachtet. Die Symbolleiste fliegt hin, setzt sie **{{EntDropFloor}}**, dupliziert, löscht, animiert ([[animation]]) und zeigt **{{EntRaw}}**. Änderungen an Entities haben ihr eigenes Rückgängig, solange die Maus über der Karte oder der Liste ist.",
            "列表下方的检查器显示所选实体:来自游戏代码的类别说明、**{{EntName}}**、**{{EntOrigin}}**、**{{EntFloor}}**(脚离地面的高度——陷入地面时显示红色)和 **{{EntAngle}}**,生成标志以复选框显示,其余所有键以字段显示。脚本字段带有 **{{EntOpenScript}}**,`NPC_type` 可搜索游戏中的所有 NPC;若类别自行设定类型(`NPC_Droid_R2D2` → `r2d2`),空字段会以灰色显示该类型,当游戏随机选择或忽略该键时会有提示。图标栏可以飞过去、**{{EntDropFloor}}**、复制、删除、制作动画([[animation]])以及显示 **{{EntRaw}}**。鼠标位于地图或列表上时,实体的修改有独立的撤销记录。",
            "一覧の下のインスペクターは選択中のエンティティを表示します。ゲームコード由来の説明付きのクラス、**{{EntName}}**、**{{EntOrigin}}**、**{{EntFloor}}**(足が床からどれだけ上にあるか。めり込むと赤)、**{{EntAngle}}**、チェックボックスのスポーンフラグ、その他すべてのキーの欄。スクリプト欄には **{{EntOpenScript}}**、`NPC_type` にはゲームの全 NPC の検索があります。クラスが自分でタイプを決める場合(`NPC_Droid_R2D2` → `r2d2`)、空の欄にそれが灰色で表示され、ゲームがランダムに選ぶときやキーを無視するときは注記が出ます。アイコンバーでそこへ移動、**{{EntDropFloor}}**、複製、削除、アニメーション([[animation]])、**{{EntRaw}}** を使えます。マウスがマップか一覧の上にある間、エンティティの変更は専用の元に戻す履歴を使います。")),
        ("h2", "setzen", T("Placing new entities", "Neue Entities setzen", "放置新实体", "新しいエンティティの配置")),
        ("p", T(
            "The tiles on the left edge of the view place the common kinds: character, walk target, camera/look mark, look target, script runner, effect, sound, NPC waypoint, player start. Click a tile, then click in the view - or drag the tile into the view; <kbd>Shift</kbd> places several. **{{EntNew}}** above the list and a right-click in the view (**{{EntNewHere}}**) offer every class. The **Grid** button snaps to 8, 16, 32 or 64 units.",
            "Die Kacheln am linken Rand der Ansicht setzen die üblichen Arten: Figur, Laufziel, Kamera-/Blickmarke, Blickziel, Skriptstarter, Effekt, Klang, NPC-Wegpunkt, Spielerstart. Kachel anklicken, dann in die Ansicht klicken - oder die Kachel in die Ansicht ziehen; <kbd>Umschalt</kbd> setzt mehrere. **{{EntNew}}** über der Liste und ein Rechtsklick in der Ansicht (**{{EntNewHere}}**) bieten jede Klasse an. Der Knopf **Raster** rastet auf 8, 16, 32 oder 64 Einheiten.",
            "视图左边缘的图块用于放置常见类型:角色、行走目标、摄像机/注视标记、注视目标、脚本启动器、特效、声音、NPC 路径点、玩家出生点。单击图块,再在视图中单击——或把图块拖进视图;按住 <kbd>Shift</kbd> 可连续放置。列表上方的 **{{EntNew}}** 和视图中的右键菜单(**{{EntNewHere}}**)提供所有类别。**网格**按钮可吸附到 8、16、32 或 64 单位。",
            "ビュー左端のタイルでよく使う種類を配置します。キャラクター、歩行目標、カメラ/視線マーク、視線目標、スクリプトランナー、エフェクト、サウンド、NPC ウェイポイント、プレイヤースタート。タイルをクリックしてからビューをクリックするか、タイルをビューへドラッグします。<kbd>Shift</kbd> で連続配置。一覧の上の **{{EntNew}}** とビューの右クリック(**{{EntNewHere}}**)ではすべてのクラスを選べます。**グリッド**ボタンで 8・16・32・64 ユニットにスナップします。")),
        # rc609 M0: vorhandene Modelle mit einem Klick in die Karte
        ("h2", "modelle", T(
            'Placing models',
            'Modelle setzen',
            '放置模型',
            'モデルの配置')),
        ("img", "h_m0_modelle", T(
            'A crate from the game standing on the floor of md_am_sith before the click - the model follows the mouse.',
            'Eine Kiste aus dem Spiel steht vor dem Klick auf dem Boden von md_am_sith - das Modell folgt der Maus.',
            '点击之前,游戏中的一个箱子立在 md_am_sith 的地面上——模型跟随鼠标。',
            'クリック前、ゲームの箱が md_am_sith の床に立っています。モデルはマウスに付いてきます。'), "breit"),
        ("p", T(
            "The tile **{{M0Kachel}}** opens the window **{{M0Fenster}}** with every `.md3` under `models/` of the game folders - from the `.pk3` files and loose files, in the version the game reads. Type words into the search box (all of them must appear in the path), click a model and then click into the map, or drag it in. Until then it stands under the mouse, drawn like the game draws it. It becomes a `misc_model_static` whose bottom (the frame 0 bounds the game uses) stands on the floor of the game's collision; on a wall it goes in front of the wall. <kbd>Shift</kbd> keeps placing, <kbd>Esc</kbd> stops. One undo step per model.",
            'Die Kachel **{{M0Kachel}}** öffnet das Fenster **{{M0Fenster}}** mit jedem `.md3` unter `models/` der Spielordner - aus den `.pk3`-Dateien und losen Dateien, in der Fassung, die das Spiel liest. Wörter ins Suchfeld tippen (alle müssen im Pfad stehen), ein Modell anklicken und dann in die Karte klicken oder es hineinziehen. Bis dahin steht es unter der Maus, gezeichnet wie im Spiel. Es wird ein `misc_model_static`, dessen Unterkante (die Grenzen von Bild 0, die das Spiel nimmt) auf dem Boden der Spielkollision steht; an einer Wand kommt es vor die Wand. <kbd>Umschalt</kbd> setzt weiter, <kbd>Esc</kbd> hört auf. Je Modell ein Rückgängig-Schritt.',
            '图块 **{{M0Kachel}}** 打开窗口 **{{M0Fenster}}**,其中列出游戏文件夹 `models/` 下的每个 `.md3`——来自 `.pk3` 文件和松散文件,并采用游戏实际读取的版本。在搜索框中输入词语(所有词都必须出现在路径中),点击一个模型,然后点击地图或将其拖入。在此之前,它会以游戏的绘制方式立在鼠标下。它会成为一个 `misc_model_static`,其底部(游戏使用的第 0 帧边界)立在游戏碰撞的地面上;在墙上时会放到墙前。<kbd>Shift</kbd> 可继续放置,<kbd>Esc</kbd> 停止。每个模型一步撤销。',
            'タイル **{{M0Kachel}}** はウィンドウ **{{M0Fenster}}** を開きます。ゲームフォルダーの `models/` 以下にあるすべての `.md3` が、`.pk3` ファイルとばらのファイルから、ゲームが読む版で並びます。検索欄に語を入力し(すべての語がパスに含まれる必要があります)、モデルをクリックしてからマップをクリックするか、ドラッグして入れます。それまではゲームと同じ描き方でマウスの下に立っています。モデルは `misc_model_static` になり、その下端(ゲームが使うフレーム 0 の境界)がゲームの衝突の床に立ちます。壁の上では壁の手前に置かれます。<kbd>Shift</kbd> で続けて置き、<kbd>Esc</kbd> で終わります。モデルごとに元に戻す 1 回です。')),
        ("p", T(
            "Selected, the gizmo bar has a third button **{{M0GizmoSkalieren}}**: the end squares scale along the model's own axes (`modelscale_vec`), the square in the middle scales evenly (`modelscale`); <kbd>Ctrl</kbd> snaps to 0.05. Turning writes `angle` (yaw only) or `angles`. The editor always writes exactly one angle key and one scale key - then the game (SP and MP), Movie Duels and q3map2 all read the same position.",
            'Gewählt hat die Gizmo-Leiste einen dritten Knopf **{{M0GizmoSkalieren}}**: die Endquadrate skalieren entlang der eigenen Achsen des Modells (`modelscale_vec`), das Quadrat in der Mitte gleichmäßig (`modelscale`); <kbd>Strg</kbd> rastet auf 0,05. Drehen schreibt `angle` (nur Gier) oder `angles`. Der Editor schreibt immer genau einen Winkel- und einen Größenschlüssel - dann lesen Spiel (SP und MP), Movie Duels und q3map2 dieselbe Lage.',
            '选中后,操纵器栏多出第三个按钮 **{{M0GizmoSkalieren}}**:末端方块沿模型自身的轴缩放(`modelscale_vec`),中间的方块等比缩放(`modelscale`);<kbd>Ctrl</kbd> 以 0.05 为步进。旋转会写入 `angle`(仅偏航)或 `angles`。编辑器总是只写一个角度键和一个缩放键——这样游戏(SP 和 MP)、Movie Duels 和 q3map2 读到的位置完全一致。',
            '選択するとギズモバーに 3 つ目のボタン **{{M0GizmoSkalieren}}** が現れます。端の四角はモデル自身の軸に沿って拡大縮小し(`modelscale_vec`)、中央の四角は均等に拡大縮小します(`modelscale`)。<kbd>Ctrl</kbd> で 0.05 刻み。回転は `angle`(ヨーのみ)か `angles` を書きます。エディターは常に角度キーと大きさキーを 1 つずつだけ書くので、ゲーム(SP と MP)、Movie Duels、q3map2 が同じ配置を読みます。')),
        ("p", T(
            '**{{DoEntSpeichern}}** writes the models into the `.ent`, **{{DoBspUebernehmen}}** into the map itself (see [[missionen#anort]]). A compiled map has no source, so `misc_model` (which only q3map2 bakes in) is not offered; Jedi Outcast has no `misc_model_static` at all - the window says so and places nothing. A model without clip is not solid in the game; clips for compiled maps follow later.',
            '**{{DoEntSpeichern}}** schreibt die Modelle in die `.ent`, **{{DoBspUebernehmen}}** in die Karte selbst (siehe [[missionen#anort]]). Eine fertige Karte hat keine Quelle, darum gibt es kein `misc_model` (das backt nur q3map2 ein); Jedi Outcast kennt `misc_model_static` gar nicht - das Fenster sagt es und setzt nichts. Ein Modell ohne Clip ist im Spiel nicht fest; Clips für fertige Karten kommen später.',
            '**{{DoEntSpeichern}}** 把模型写入 `.ent`,**{{DoBspUebernehmen}}** 写入地图本身(见 [[missionen#anort]])。已编译的地图没有源文件,因此不提供 `misc_model`(只有 q3map2 会把它烘焙进去);Jedi Outcast 根本没有 `misc_model_static`——窗口会说明并且不放置任何东西。没有 clip 的模型在游戏中不是实体;已编译地图的 clip 之后提供。',
            '**{{DoEntSpeichern}}** はモデルを `.ent` に、**{{DoBspUebernehmen}}** はマップそのものに書きます([[missionen#anort]] を参照)。コンパイル済みのマップにはソースがないため、`misc_model`(q3map2 だけが焼き込みます)は出てきません。Jedi Outcast には `misc_model_static` 自体がなく、ウィンドウがそう伝えて何も置きません。クリップのないモデルはゲームで固体ではありません。コンパイル済みマップのクリップは後で入ります。')),
        ("note", T(
            'When a newer file of a model appears (saved again, or a loose file in front of the `.pk3`), the preview loads it within a second; every copy keeps its position, rotation and scale.',
            'Liegt eine neuere Datei eines Modells vor (neu gespeichert oder eine lose Datei vor der `.pk3`), lädt die Vorschau sie innerhalb einer Sekunde; jedes Exemplar behält Lage, Drehung und Größe.',
            '当某个模型出现较新的文件时(重新保存,或在 `.pk3` 之前出现的松散文件),预览会在一秒内加载它;每个实例都保留位置、旋转和缩放。',
            'モデルの新しいファイルが現れると(保存し直した、または `.pk3` より前に読まれるばらのファイル)、プレビューは 1 秒以内に読み込みます。すべての配置は位置・回転・大きさを保ちます。')),
    ],
}

WEGPUNKTE = {
    "id": "wegpunkte",
    "gruppe": "karte",
    "titel": T("NPC waypoints", "NPC-Wegpunkte", "NPC 路径点", "NPC ウェイポイント"),
    "kurz": T(
        "The network NPCs walk on - to far walk targets in cutscenes, and when they chase or flee. Place it by hand or let the program build it, then check it.",
        "Das Netz, auf dem NPCs laufen - zu fernen Laufzielen in Zwischensequenzen und wenn sie verfolgen oder fliehen. Von Hand setzen oder vom Programm bauen lassen, dann prüfen.",
        "NPC 行走所用的路网——在过场中前往远处的行走目标,以及追击或逃跑时。可以手动放置,也可以让程序自动生成,然后进行检查。",
        "NPC が歩くネットワーク。カットシーンで遠い目標へ歩くときや、追いかけたり逃げたりするときに使います。手で置くか自動で作り、確認します。"),
    "bloecke": [
        ("img", "h_wegpunkte", T(
            "The {{WegTitel}} window and the network on the map.",
            "Das Fenster {{WegTitel}} und das Netz auf der Karte.",
            "{{WegTitel}} 窗口及地图上的路网。",
            "{{WegTitel}} ウィンドウとマップ上のネットワーク。"), "breit"),
        ("legende", [
            (T("**{{WegZeigen}}**"), T("Shows the network on the map.")),
            (T("**{{WegSetzen}}** / **{{WegVerbinden}}**"), T("Place points by clicking the floor / connect two points.")),
            (T("**{{WegErzeugen}}** / **{{WegEntfernen}}**"), T("Spread a network over the walkable floor / remove what was created.")),
            (T("**{{WegPruefen}}**"), T("Finds what the game would reject; click a problem to fly there.")),
        ]),
        ("p", T(
            "Open it with **{{WegTitel}}** in the map toolbar. **{{WegZeigen}}** draws the paths: green set by hand (`target`), blue automatic (nav goals and combat points), orange jumps (`targetJump`).",
            "Geöffnet wird es mit **{{WegTitel}}** in der Kartenleiste. **{{WegZeigen}}** zeichnet die Wege: grün von Hand gesetzt (`target`), blau automatisch (Navgoals und Kampfpunkte), orange Sprünge (`targetJump`).",
            "通过地图工具栏中的 **{{WegTitel}}** 打开。**{{WegZeigen}}** 绘制路径:绿色为手动设置(`target`),蓝色为自动(导航点和战斗点),橙色为跳跃(`targetJump`)。",
            "マップのツールバーの **{{WegTitel}}** で開きます。**{{WegZeigen}}** で経路を描きます。緑は手動(`target`)、青は自動(ナビゴールと戦闘ポイント)、オレンジはジャンプ(`targetJump`)。")),
        ("list", [
            T("**{{WegSetzen}}**: click the floor - each new point is connected to the one before. <kbd>Shift</kbd>+click starts a new path, <kbd>Esc</kbd> ends.",
              "**{{WegSetzen}}**: auf den Boden klicken - jeder neue Punkt wird mit dem vorigen verbunden. <kbd>Umschalt</kbd>+Klick beginnt einen neuen Weg, <kbd>Esc</kbd> beendet.",
              "**{{WegSetzen}}**:单击地面——每个新点都会与上一个相连。<kbd>Shift</kbd>+单击开始新路径,<kbd>Esc</kbd> 结束。",
              "**{{WegSetzen}}**:床をクリックすると、新しい点が直前の点とつながります。<kbd>Shift</kbd>+クリックで新しい経路、<kbd>Esc</kbd> で終了。"),
            T("**{{WegVerbinden}}**: click two points to connect them (<kbd>Ctrl</kbd>: a jump); clicking two connected points disconnects them. A point has at most four connections.",
              "**{{WegVerbinden}}**: zwei Punkte anklicken verbindet sie (<kbd>Strg</kbd>: ein Sprung); zwei verbundene anklicken trennt sie. Ein Punkt hat höchstens vier Verbindungen.",
              "**{{WegVerbinden}}**:单击两个点将其连接(<kbd>Ctrl</kbd>:跳跃连接);单击两个已连接的点会断开。每个点最多四个连接。",
              "**{{WegVerbinden}}**:2 点をクリックで接続(<kbd>Ctrl</kbd>:ジャンプ)、接続済みの 2 点をクリックで切断。1 点の接続は最大 4 本です。"),
            T("**{{WegErzeugen}}** spreads waypoints over the walkable floor (**{{WegAbstand}}**: the original levels use about 230) and connects them - one undo step. **{{WegEntfernen}}** removes only those, hand-placed ones stay.",
              "**{{WegErzeugen}}** verteilt Wegpunkte über den begehbaren Boden (**{{WegAbstand}}**: die Originalkarten nutzen etwa 230) und verbindet sie - ein Rückgängig-Schritt. **{{WegEntfernen}}** entfernt nur diese, von Hand gesetzte bleiben.",
              "**{{WegErzeugen}}** 在可行走的地面上分布路径点(**{{WegAbstand}}**:原版关卡约 230)并连接——可一步撤销。**{{WegEntfernen}}** 只删除这些点,手动放置的保留。",
              "**{{WegErzeugen}}** は歩ける床にウェイポイントを配置して接続します(**{{WegAbstand}}**:オリジナルのレベルは約 230)。1 回で元に戻せます。**{{WegEntfernen}}** は自動で作ったものだけを削除し、手動のものは残ります。"),
            T("**{{WegPruefen}}** finds what the game would reject or NPCs cannot use; click a problem to fly there.",
              "**{{WegPruefen}}** findet, was das Spiel ablehnen würde oder NPCs nicht nutzen können; ein Klick auf ein Problem fliegt hin.",
              "**{{WegPruefen}}** 找出游戏会拒绝或 NPC 无法使用的部分;单击某个问题即可飞过去。",
              "**{{WegPruefen}}** はゲームが拒否するものや NPC が使えないものを見つけます。問題をクリックするとそこへ移動します。"),
        ]),
        ("h2", "verbinden", T(
            'Delete, connect automatically, duplicate',
            'Löschen, automatisch verbinden, duplizieren',
            '删除、自动连接、复制',
            '削除・自動接続・複製')),
        ("img", "h_weg_netz", T(
            'A waypoint placed by hand has connected itself to its neighbours (generated network on md_am_sith).',
            'Ein von Hand gesetzter Wegpunkt hat sich mit seinen Nachbarn verbunden (erzeugtes Netz auf md_am_sith).',
            '手动放置的路径点已自动与相邻点相连(md_am_sith 上生成的路网)。',
            '手で置いたウェイポイントが近くの点と自動で接続されました(md_am_sith で生成したネットワーク)。'), "breit"),
        ("legende", [
            (T("**{{WegAutoVerbinden}}** / **{{WegLuecke}}** / **{{WegNachbarn}}**"), T(
                'Connect like the game - automatically, when deleting, or for the selection.',
                'Verbinden wie im Spiel - automatisch, beim Löschen oder für die Auswahl.',
                '像游戏一样连接——自动、删除时或针对所选内容。',
                'ゲームと同じように接続します。自動で、削除時に、または選択に対して。')),
            (T("**{{WegUmbenennen}}**"), T(
                'Rename the selected waypoints by a pattern.',
                'Die gewählten Wegpunkte nach Muster umbenennen.',
                '按模式重命名选中的路径点。',
                '選択したウェイポイントをパターンで名前変更します。')),
        ]),
        ("list", [
            T(
                "**Delete** (<kbd>Delete</kbd>, the button in the inspector or **{{EntDelete}}** in the right-click menu) also removes every `target` to `targetJump` that named the deleted waypoint - in the same undo step. If other entities are named, the program asks first. With **{{WegLuecke}}** the neighbours of a deleted waypoint connect to each other where the game's edge test allows it.",
                '**Löschen** (<kbd>Delete</kbd>, der Knopf im Inspektor oder **{{EntDelete}}** im Rechtsklickmenü) nimmt auch jedes `target` bis `targetJump` mit, das den gelöschten Wegpunkt nannte - im selben Rückgängig-Schritt. Nennen sie andere Entities, fragt das Programm erst. Mit **{{WegLuecke}}** verbinden sich die Nachbarn eines gelöschten Wegpunkts untereinander, wo die Kantenprüfung des Spiels es erlaubt.',
                '**删除**(<kbd>Delete</kbd>、检查器中的按钮或右键菜单中的 **{{EntDelete}}**)会同时移除所有指向被删路径点的 `target` 到 `targetJump`——在同一个撤销步骤中。如果指向的是其他实体,程序会先询问。启用 **{{WegLuecke}}** 后,被删路径点的相邻点会在游戏连线检查允许的地方彼此相连。',
                '**削除**(<kbd>Delete</kbd>、インスペクターのボタン、右クリックメニューの **{{EntDelete}}**)では、削除したウェイポイントを指していた `target` から `targetJump` もすべて同じ元に戻す 1 回で消えます。ほかのエンティティを指している場合は、先に確認します。**{{WegLuecke}}** をオンにすると、削除したウェイポイントの隣どうしが、ゲームの辺チェックが許す範囲で接続されます。'),
            T(
                "**{{WegAutoVerbinden}}** (on by default): a waypoint you place, move or duplicate connects to the waypoints within **{{WegAbstand}}** that the game connects too - with the game's own edge test (NAV::TestEdge, in Jedi Outcast HardConnect), nearest first, at most four `target` fields per point. After moving, connections the game would drop are removed; all in one undo step. **{{WegNachbarn}}** does the same for the selection. Nav goals and combat points are left out: the game connects them itself.",
                '**{{WegAutoVerbinden}}** (Vorgabe an): Ein Wegpunkt, den du setzt, verschiebst oder duplizierst, verbindet sich mit den Wegpunkten im Umkreis von **{{WegAbstand}}**, die auch das Spiel verbindet - mit der Kantenprüfung des Spiels selbst (NAV::TestEdge, in Jedi Outcast HardConnect), die nächsten zuerst, höchstens vier `target`-Felder je Punkt. Nach dem Verschieben fallen Verbindungen weg, die das Spiel verwerfen würde; alles in einem Rückgängig-Schritt. **{{WegNachbarn}}** tut dasselbe für die Auswahl. Navgoals und Kampfpunkte bleiben außen vor: Die verbindet das Spiel selbst.',
                '**{{WegAutoVerbinden}}**(默认开启):你放置、移动或复制的路径点会与 **{{WegAbstand}}** 范围内游戏同样会连接的路径点相连——使用游戏自身的连线检查(NAV::TestEdge,Jedi Outcast 中为 HardConnect),最近的优先,每个点最多四个 `target` 字段。移动后,游戏会丢弃的连接将被移除;全部在一个撤销步骤中。**{{WegNachbarn}}** 对所选内容执行相同操作。导航点和战斗点不参与:游戏会自行连接它们。',
                '**{{WegAutoVerbinden}}**(既定でオン): 置いた・移動した・複製したウェイポイントは、**{{WegAbstand}}** の範囲内でゲームも接続するウェイポイントと接続します。ゲーム自身の辺チェック(NAV::TestEdge、Jedi Outcast では HardConnect)を使い、近いものから、1 点あたり `target` 欄は最大 4 つです。移動後は、ゲームが捨てる接続を削除します。すべて元に戻す 1 回です。**{{WegNachbarn}}** は選択に対して同じことをします。ナビゴールと戦闘ポイントは対象外です。ゲームが自分で接続します。'),
            T(
                '**{{EntDuplicate}}** (<kbd>Ctrl</kbd>+<kbd>D</kbd> or the right-click menu) on several waypoints: the copies get free names and stay connected among themselves, connections to outside stay as they were; then they connect to their new neighbours. The field next to **{{WegUmbenennen}}** renames the selected waypoints by a pattern (`#` becomes a number); every `target` that named them follows.',
                '**{{EntDuplicate}}** (<kbd>Strg</kbd>+<kbd>D</kbd> oder das Rechtsklickmenü) auf mehreren Wegpunkten: Die Kopien bekommen freie Namen und bleiben untereinander verbunden, Verbindungen nach außen bleiben, wie sie waren; dann verbinden sie sich mit ihren neuen Nachbarn. Das Feld neben **{{WegUmbenennen}}** benennt die gewählten Wegpunkte nach Muster um (`#` wird eine Nummer); jedes `target`, das sie nannte, zieht mit.',
                '对多个路径点执行 **{{EntDuplicate}}**(<kbd>Ctrl</kbd>+<kbd>D</kbd> 或右键菜单):副本获得空闲名称并保持彼此相连,与外部的连接保持不变;随后它们会与新的相邻点相连。**{{WegUmbenennen}}** 旁边的输入框按模式重命名选中的路径点(`#` 变为数字);所有指向它们的 `target` 会一起更新。',
                '複数のウェイポイントに **{{EntDuplicate}}**(<kbd>Ctrl</kbd>+<kbd>D</kbd> または右クリックメニュー): コピーは空いている名前を受け取り、コピーどうしの接続は保たれ、外への接続はそのままです。そのあと新しい近くの点と接続します。**{{WegUmbenennen}}** の横の欄で、選択したウェイポイントをパターンで名前変更します(`#` は番号になります)。それらを指していた `target` もすべて追従します。'),
            T(
                '**{{WegErzeugen}}** also starts from the waypoints already on the map: place one in a room the generation missed and generate again - the room gets its network, connected to your waypoint.',
                '**{{WegErzeugen}}** beginnt auch an den Wegpunkten, die schon auf der Karte stehen: Setz einen in einen Raum, den die Erzeugung ausgelassen hat, und erzeuge noch einmal - der Raum bekommt sein Netz, verbunden mit deinem Wegpunkt.',
                '**{{WegErzeugen}}** 也会从地图上已有的路径点开始:在生成时遗漏的房间里放一个路径点,再生成一次——该房间就会得到自己的路网,并与你的路径点相连。',
                '**{{WegErzeugen}}** はマップにすでにあるウェイポイントからも始まります。生成で抜けた部屋に 1 つ置いてもう一度生成すると、その部屋にもネットワークができ、置いたウェイポイントにつながります。'),
        ]),
        ("img", "h_weg_kontext", T(
            'The right-click menu of the view with a waypoint selected: {{EntDuplicate}}, {{EntDelete}}, {{WegNachbarn}}.',
            'Das Rechtsklickmenü der Ansicht mit einem gewählten Wegpunkt: {{EntDuplicate}}, {{EntDelete}}, {{WegNachbarn}}.',
            '选中路径点时视图的右键菜单:{{EntDuplicate}}、{{EntDelete}}、{{WegNachbarn}}。',
            'ウェイポイントを選択したときのビューの右クリックメニュー: {{EntDuplicate}}、{{EntDelete}}、{{WegNachbarn}}。')),
        ("h2", "erzeugung", T(
            'Generation: rooms, stairs, doors',
            'Erzeugung: Räume, Treppen, Türen',
            '生成:房间、楼梯、门',
            '生成: 部屋・階段・ドア')),
        ("img", "h_weg_treppe", T(
            'A generated network on a stair (mp/duel8): one point at the bottom, one at the top, connected.',
            'Ein erzeugtes Netz an einer Treppe (mp/duel8): ein Punkt unten, einer oben, verbunden.',
            '楼梯上生成的路网(mp/duel8):底部一个点,顶部一个点,二者相连。',
            '階段で生成したネットワーク(mp/duel8): 下に 1 点、上に 1 点、つながっています。'), "breit"),
        ("legende", [
            (T("**{{WegErzeugen}}**"), T(
            'Spread waypoints over all floor an NPC can walk to.',
            'Wegpunkte über allen Boden verteilen, den ein NPC erreicht.',
            '在 NPC 能走到的所有地面上分布路径点。',
            'NPC が歩いて行けるすべての床にウェイポイントを置きます。')),
        ]),
        ("list", [
            T(
            "**{{WegErzeugen}}** walks like an NPC in the game from the player start, the characters, the nav goals and the waypoints on the map: 15 units per step (an NPC running for one frame), up and down steps of at most 18 (`STEPSIZE`), on floor no steeper than the game allows (`MIN_WALK_NORMAL`), against the game's collision - clip brushes block, decoration without collision does not. So it reaches every room a figure can walk into, also through narrow doors.",
            '**{{WegErzeugen}}** läuft wie ein NPC im Spiel vom Spielerstart, den Figuren, den Laufzielen und den Wegpunkten auf der Karte aus: 15 Einheiten je Schritt (ein NPC rennt so weit in einem Bild), Stufen bis 18 hinauf und hinab (`STEPSIZE`), auf Boden, der nicht steiler ist, als das Spiel erlaubt (`MIN_WALK_NORMAL`), gegen die Kollision des Spiels - Clip-Brushes halten auf, Deko ohne Kollision nicht. So erreicht es jeden Raum, in den eine Figur gehen kann, auch durch schmale Türen.',
            '**{{WegErzeugen}}** 会像游戏中的 NPC 一样,从玩家出生点、角色、导航目标和地图上的路径点出发行走:每步 15 单位(NPC 一帧奔跑的距离),上下台阶最高 18(`STEPSIZE`),地面坡度不超过游戏允许的范围(`MIN_WALK_NORMAL`),并使用游戏的碰撞——Clip 画刷会阻挡,没有碰撞的装饰不会。因此它能到达角色可以走进的每个房间,即使要穿过窄门。',
            '**{{WegErzeugen}}** はゲームの NPC と同じように、プレイヤー開始位置・キャラクター・ナビゴール・マップ上のウェイポイントから歩きます。1 歩 15 ユニット(NPC が 1 フレームで走る距離)、上り下りの段差は最大 18(`STEPSIZE`)、床の傾きはゲームが許す範囲まで(`MIN_WALK_NORMAL`)、当たり判定はゲームと同じです。クリップブラシは通れず、当たり判定のない装飾は通れます。こうして、キャラクターが歩いて入れる部屋には、狭いドアの先でもすべて届きます。'),
            T(
            'Points come on the grid of **{{WegAbstand}}**, at the top and bottom of every stair and ramp that climbs more than one step, and wherever the grid would leave a gap of more than one spacing - in doorways and small rooms. Ramps get points on the slope too.',
            'Punkte kommen ins Raster von **{{WegAbstand}}**, oben und unten an jede Treppe und Rampe, die mehr als eine Stufe steigt, und überall dorthin, wo das Raster eine Lücke von mehr als einem Abstand ließe - in Türen und kleine Räume. Rampen bekommen auch auf der Schräge Punkte.',
            '路径点会放在 **{{WegAbstand}}** 的网格上、每段高于一级台阶的楼梯和坡道的顶部与底部,以及网格会留下超过一个间距空缺的地方——门口和小房间。坡道的斜面上也会有路径点。',
            '点は **{{WegAbstand}}** のグリッド上、1 段より高く上る階段とスロープの上と下、そしてグリッドだと 1 間隔より大きな隙間ができる場所(ドアや小部屋)に置かれます。スロープの斜面にも点が置かれます。'),
            T(
            "A connection is kept only if a figure can walk the straight line and the game's own edge test (NAV::TestEdge, in Jedi Outcast HardConnect) keeps it in both directions. Doors count as open, as the game's network build opens a door that is in the way; whether NPCs may pass it is decided while playing, as in the game.",
            'Eine Verbindung bleibt nur, wenn eine Figur die gerade Linie gehen kann und die Kantenprüfung des Spiels selbst (NAV::TestEdge, in Jedi Outcast HardConnect) sie in beide Richtungen behält. Türen zählen als offen, wie der Netzbau des Spiels eine Tür öffnet, die im Weg steht; ob NPCs durch dürfen, entscheidet sich beim Spielen, wie im Spiel.',
            '只有当角色能沿直线走过、并且游戏自身的连线检查(NAV::TestEdge,Jedi Outcast 中为 HardConnect)在两个方向上都保留时,连接才会保留。门视为打开,就像游戏构建路网时会打开挡路的门;NPC 能否通过在游戏进行时决定,与游戏一致。',
            '接続が残るのは、キャラクターがその直線を歩けて、ゲーム自身の辺チェック(NAV::TestEdge、Jedi Outcast では HardConnect)が両方向で残す場合だけです。ドアは開いているものとして扱います。ゲームのネットワーク構築も、道をふさぐドアを開けて調べます。NPC が通れるかどうかは、ゲームと同じくプレイ中に決まります。'),
            T(
            'The game takes at most 1023 waypoints. If the grid needs more, the spacing grows (up to three times **{{WegAbstand}}**) and the message says so. Roofs and ledges that can only be reached by jumping get no points from walking; click there with **{{WegStartpunkt}}** (below).',
            'Das Spiel nimmt höchstens 1023 Wegpunkte. Braucht das Raster mehr, wird der Abstand größer (bis zum Dreifachen von **{{WegAbstand}}**), und die Meldung sagt es. Dächer und Vorsprünge, die man nur springend erreicht, bekommen beim Laufen keine Punkte; klick dort mit **{{WegStartpunkt}}** (unten).',
            '游戏最多接受 1023 个路径点。如果网格需要更多,间距会增大(最多为 **{{WegAbstand}}** 的三倍),并在消息中说明。只能跳上去的屋顶和平台在行走时不会得到路径点;用 **{{WegStartpunkt}}** 在那里点击(见下文)。',
            'ゲームが受け付けるウェイポイントは最大 1023 個です。グリッドがそれより多く必要なら、間隔が広がり(最大で **{{WegAbstand}}** の 3 倍)、メッセージで知らせます。ジャンプでしか届かない屋根や出っ張りには歩きでは点が置かれません。そこを **{{WegStartpunkt}}** でクリックしてください(下記)。'),
        ]),
        ("h2", "malen", T(
            'Areas, path, start point, jump edges',
            'Bereiche, Pfad, Startpunkt, Sprungkanten',
            '区域、路径、起点、跳跃连线',
            'エリア・パス・開始点・ジャンプ辺')),
        ("img", "h_weg_bereich", T(
            'Top view: a painted area (blue) and the waypoints "Generate here" placed inside it.',
            'Draufsicht: ein gemalter Bereich (blau) und die Wegpunkte, die "Hier erzeugen" darin gesetzt hat.',
            '俯视图:绘制的区域(蓝色)以及“在此生成”在其中放置的路径点。',
            '上面図: 描いたエリア(青)と、「ここに生成」がその中に置いたウェイポイント。'), "breit"),
        ("legende", [
            (T("**{{WegPinsel}}** **{{WegLasso}}** **{{WegPfad}}** **{{WegStartpunkt}}**"), T(
            'Paint an area (brush, lasso), draw a path, click a start point.',
            'Bereich malen (Pinsel, Lasso), Pfad ziehen, Startpunkt klicken.',
            '绘制区域(画笔、套索)、画路径、点击起点。',
            'エリアを描く(ブラシ、投げ縄)、パスを引く、開始点をクリック。')),
            (T("**{{WegSperre}}**"), T(
            'The next painted area is a no-go area.',
            'Der nächste gemalte Bereich ist ein Sperrbereich.',
            '下一个绘制的区域是禁止区域。',
            '次に描くエリアは禁止エリアになります。')),
            (T("**{{WegHierErzeugen}}**"), T(
            'Generate only in this area, with its own spacing.',
            'Nur in diesem Bereich erzeugen, mit eigenem Abstand.',
            '只在此区域内生成,使用其自己的间距。',
            'このエリアだけに、エリア独自の間隔で生成します。')),
        ]),
        ("list", [
            T(
            "**{{WegPinsel}}** and **{{WegLasso}}** paint on the floor with the left mouse button held - in the 3D view or in the top view. **{{WegHierErzeugen}}** walks like an NPC only inside the area and places waypoints at the area's spacing where none are yet; they connect to the waypoints around with the game's edge test, as when placing by hand. One undo step. Areas are kept for the map (in the settings folder, not in the .bsp) and come back when you open it again.",
            '**{{WegPinsel}}** und **{{WegLasso}}** malen mit gedrückter linker Maustaste auf den Boden - in der 3D-Ansicht oder in der Draufsicht. **{{WegHierErzeugen}}** läuft wie ein NPC nur im Bereich und setzt Wegpunkte im Abstand des Bereichs, wo noch keine sind; sie verbinden sich mit den Wegpunkten ringsum über die Kantenprüfung des Spiels, wie beim Setzen von Hand. Ein Rückgängig-Schritt. Bereiche bleiben für die Karte gespeichert (im Ordner der Einstellungen, nicht in der .bsp) und kommen beim nächsten Öffnen wieder.',
            '**{{WegPinsel}}** 和 **{{WegLasso}}** 按住鼠标左键在地面上绘制——在 3D 视图或俯视图中。**{{WegHierErzeugen}}** 像 NPC 一样只在区域内行走,在还没有路径点的地方按该区域的间距放置路径点;它们通过游戏的连线检查与周围的路径点相连,与手动放置时相同。一步撤销。区域会为该地图保存(在设置文件夹中,而不是在 .bsp 中),下次打开时会恢复。',
            '**{{WegPinsel}}** と **{{WegLasso}}** は、左ボタンを押したまま床に描きます(3D ビューでも上面図でも)。**{{WegHierErzeugen}}** は NPC のようにエリアの中だけを歩き、まだない所にエリアの間隔でウェイポイントを置きます。それらは手で置いたときと同じく、ゲームの辺チェックで周囲のウェイポイントと接続します。元に戻すは 1 回です。エリアはマップごとに保存され(設定フォルダー内、.bsp には入りません)、次に開いたときに戻ります。'),
            T(
            "With **{{WegSperre}}** ticked, the next area is a no-go area (red): the generation, a path and a start point never place a waypoint there. Points around it still connect across it where the game's edge test passes.",
            'Mit **{{WegSperre}}** ist der nächste Bereich ein Sperrbereich (rot): Erzeugung, Pfad und Startpunkt setzen dort nie einen Wegpunkt. Punkte ringsum verbinden sich trotzdem darüber hinweg, wo die Kantenprüfung des Spiels besteht.',
            '勾选 **{{WegSperre}}** 后,下一个区域是禁止区域(红色):生成、路径和起点永远不会在那里放置路径点。周围的点仍会在游戏连线检查通过的地方跨过它相连。',
            '**{{WegSperre}}** をオンにすると、次のエリアは禁止エリア(赤)になります。生成、パス、開始点はそこに決してウェイポイントを置きません。周りの点は、ゲームの辺チェックに通る所ではその上をまたいで接続します。'),
        ]),
        ("img", "h_weg_pfad", T(
            'A painted path: waypoints along the line at the spacing, set on the floor and connected.',
            'Ein gemalter Pfad: Wegpunkte entlang der Linie im Abstand, auf den Boden gesetzt und verbunden.',
            '绘制的路径:沿线按间距放置在地面上并相连的路径点。',
            '描いたパス: 線に沿って間隔ごとに床へ置かれ、接続されたウェイポイント。'), "breit"),
        ("list", [
            T(
            "**{{WegPfad}}**: draw a line over the floor. Waypoints appear along it every **{{WegAbstand}}**, set on the floor like the generation does, and neighbours are connected where the game's edge test keeps the edge in both directions. Where it does not, or where the game would not keep a waypoint, a red cross marks the place and the message names the first one. The path connects to the waypoints around. One undo step.",
            '**{{WegPfad}}**: eine Linie über den Boden ziehen. Entlang der Linie entstehen Wegpunkte alle **{{WegAbstand}}**, auf den Boden gesetzt wie bei der Erzeugung, und Nachbarn werden verbunden, wo die Kantenprüfung des Spiels die Kante in beide Richtungen behält. Wo nicht oder wo das Spiel keinen Wegpunkt behielte, markiert ein rotes Kreuz die Stelle, und die Meldung nennt die erste. Der Pfad schließt an die Wegpunkte ringsum an. Ein Rückgängig-Schritt.',
            '**{{WegPfad}}**:在地面上画一条线。沿线每隔 **{{WegAbstand}}** 生成路径点,像生成时一样放在地面上;在游戏连线检查双向保留连线的地方连接相邻点。未通过或游戏不会保留路径点的地方会用红色叉号标出,消息会给出第一处的位置。路径会与周围的路径点相连。一步撤销。',
            '**{{WegPfad}}**: 床に線を引きます。線に沿って **{{WegAbstand}}** ごとにウェイポイントが生成と同じように床に置かれ、ゲームの辺チェックが両方向で辺を残す所で隣どうしが接続されます。通らない所、またはゲームがウェイポイントを残さない所には赤い×が付き、メッセージが最初の場所を示します。パスは周囲のウェイポイントと接続します。元に戻すは 1 回です。'),
            T(
            '**{{WegStartpunkt}}**: click on floor the generation did not reach - a roof, a ledge, a room behind a drop. From there it walks on like an NPC, places waypoints where the network does not cover the floor yet and connects them: on foot if a figure can walk there, otherwise with jump edges (below). One undo step.',
            '**{{WegStartpunkt}}**: auf Boden klicken, den die Erzeugung nicht erreicht hat - ein Dach, ein Sims, einen Raum hinter einer Kante. Von dort läuft sie wie ein NPC weiter, setzt Wegpunkte, wo das Netz den Boden noch nicht abdeckt, und schließt sie an: zu Fuß, wenn eine Figur dorthin gehen kann, sonst mit Sprungkanten (unten). Ein Rückgängig-Schritt.',
            '**{{WegStartpunkt}}**:点击生成未到达的地面——屋顶、壁架、落差后面的房间。从那里它像 NPC 一样继续行走,在路网尚未覆盖的地面上放置路径点并连接它们:如果角色能走过去就步行连接,否则使用跳跃连线(见下文)。一步撤销。',
            '**{{WegStartpunkt}}**: 生成が届かなかった床(屋根、張り出し、段差の先の部屋)をクリックします。そこから NPC のように歩き、ネットワークがまだ覆っていない床にウェイポイントを置いて接続します。キャラクターが歩いて行けるなら徒歩で、そうでなければジャンプ辺で(下記)。元に戻すは 1 回です。'),
        ]),
        ("img", "h_weg_dach", T(
            'Jedi Temple Garden: waypoints on the roof from a start point and the jump edge (orange, dashed) from the floor below up to the roof.',
            'Jedi Temple Garden: Wegpunkte auf dem Dach von einem Startpunkt aus und die Sprungkante (orange gestrichelt) vom Boden darunter aufs Dach.',
            'Jedi Temple Garden:从起点生成的屋顶路径点,以及从下方地面到屋顶的跳跃连线(橙色虚线)。',
            'Jedi Temple Garden: 開始点から置いた屋根の上のウェイポイントと、下の床から屋根へのジャンプ辺(オレンジの破線)。'), "breit"),
        ("list", [
            T(
            "Jump edges are `targetJump`. The game only jumps on such an edge, does not test it when it loads the map, and only lets NPCs that can jump use it (force jump, or a class that jumps by itself). **{{WegSprungkanten}}** (also after a start point and **{{WegAnschliessen}}**) writes one only between parts of the network that are not connected otherwise, and only where an NPC really makes the jump in the game, in both directions - the same test the game runs before jumping (NPC_TryJump, NPC_Jump): at most 750 apart, at most 450 down, floor under the target, and an arc the NPC's box gets through. Each waypoint has one `targetJump` field. Jedi Outcast has no jump edges.",
            'Sprungkanten sind `targetJump`. Das Spiel springt nur auf einer solchen Kante, prüft sie beim Laden der Karte nicht und lässt nur NPCs darauf, die springen können (Machtsprung oder eine Klasse, die von selbst springt). **{{WegSprungkanten}}** (auch nach einem Startpunkt und **{{WegAnschliessen}}**) schreibt eine nur zwischen Teile des Netzes, die sonst nicht verbunden sind, und nur, wo ein NPC den Sprung im Spiel wirklich schafft, in beide Richtungen - mit derselben Prüfung, die das Spiel vor dem Sprung macht (NPC_TryJump, NPC_Jump): höchstens 750 weit, höchstens 450 hinab, Boden unter dem Ziel und ein Bogen, durch den der Kasten des NPCs passt. Jeder Wegpunkt hat ein `targetJump`-Feld. Jedi Outcast kennt keine Sprungkanten.',
            '跳跃连线就是 `targetJump`。游戏只会在这种连线上跳跃,载入地图时不检查它,并且只让会跳跃的 NPC 使用(原力跳跃,或本身会跳的职业)。**{{WegSprungkanten}}**(在起点和 **{{WegAnschliessen}}** 之后也会执行)只在路网中彼此不相连的部分之间写入,并且只在 NPC 在游戏中确实能双向完成跳跃的地方——使用游戏跳跃前的同一检查(NPC_TryJump、NPC_Jump):距离最多 750,向下最多 450,目标下方有地面,且 NPC 的碰撞盒能通过弧线。每个路径点有一个 `targetJump` 字段。Jedi Outcast 没有跳跃连线。',
            'ジャンプ辺は `targetJump` です。ゲームはこの辺でだけジャンプし、マップ読み込み時には調べず、跳べる NPC(フォースジャンプを持つか、自分で跳ぶクラス)だけに使わせます。**{{WegSprungkanten}}**(開始点や **{{WegAnschliessen}}** の後にも行われます)は、ほかではつながっていないネットワークの部分の間にだけ、しかも NPC がゲームで実際に両方向へ跳べる所にだけ書きます。ゲームがジャンプ前に行うのと同じチェック(NPC_TryJump、NPC_Jump)です: 距離 750 まで、下へ 450 まで、目標の下に床があり、NPC の箱が通れる弧があること。各ウェイポイントには `targetJump` 欄が 1 つあります。Jedi Outcast にはジャンプ辺がありません。'),
            T(
            'Waypoints the player start cannot reach over the network are drawn violet, and the window counts the parts. **{{WegAnschliessen}}** walks from each such part towards the network (as with a start point there, within two spacings around it) and connects it - on foot, otherwise with jump edges.',
            'Wegpunkte, die der Spielerstart über das Netz nicht erreicht, sind violett gezeichnet, und das Fenster zählt die Teile. **{{WegAnschliessen}}** läuft von jedem solchen Teil zum Netz (wie mit einem Startpunkt dort, in zwei Abständen um ihn) und schließt ihn an - zu Fuß, sonst mit Sprungkanten.',
            '从玩家出生点经路网无法到达的路径点以紫色绘制,窗口会统计这些部分的数量。**{{WegAnschliessen}}** 从每个这样的部分走向路网(如同在那里设起点,范围为其周围两个间距)并连接它——步行,否则用跳跃连线。',
            'プレイヤー開始位置からネットワークでたどり着けないウェイポイントは紫で描かれ、ウィンドウがその部分の数を数えます。**{{WegAnschliessen}}** はそうした各部分からネットワークへ歩き(そこに開始点を置いたように、周囲 2 間隔の範囲で)、接続します。徒歩で、無理ならジャンプ辺で。'),
        ]),
        ("img", "h_weg_fortschritt", T(
            'While the generation runs in the background, the window shows its progress and step; Cancel changes nothing.',
            'Während die Erzeugung im Hintergrund läuft, zeigt das Fenster Fortschritt und Schritt; Abbrechen ändert nichts.',
            '生成在后台运行时,窗口显示进度和当前步骤;取消不会更改任何内容。',
            '生成がバックグラウンドで動いている間、ウィンドウに進行状況と段階が表示されます。中止しても何も変わりません。'), "breit"),
        ("list", [
            T(
            'Generating takes a few seconds on big maps. It runs in the background: the window shows progress and step, and you can keep looking around or work in other windows. The result comes in one undo step at the end - the same network as computed directly. If the entities change in the meantime, the result is discarded with a message; **{{WegAbbrechen}}** stops it and changes nothing.',
            'Die Erzeugung dauert auf großen Karten einige Sekunden. Sie läuft im Hintergrund: Das Fenster zeigt Fortschritt und Schritt, und du kannst dich weiter umsehen oder in anderen Fenstern arbeiten. Das Ergebnis kommt am Ende in einem Rückgängig-Schritt - dasselbe Netz wie direkt gerechnet. Ändern sich die Entities inzwischen, wird es mit einer Meldung verworfen; **{{WegAbbrechen}}** hält sie an und ändert nichts.',
            '在大地图上生成需要几秒钟。它在后台运行:窗口显示进度和步骤,你可以继续环顾四周或在其他窗口中工作。结果在最后以一个撤销步骤出现——与直接计算得到的路网相同。如果期间实体发生变化,结果会被丢弃并给出消息;**{{WegAbbrechen}}** 会停止它且不做任何更改。',
            '大きなマップでは生成に数秒かかります。生成はバックグラウンドで動きます。ウィンドウに進行状況と段階が表示され、その間も見回したりほかのウィンドウで作業したりできます。結果は最後に元に戻す 1 回分として入り、直接計算した場合と同じネットワークです。その間にエンティティが変わると、メッセージとともに結果を破棄します。**{{WegAbbrechen}}** は止めて、何も変更しません。'),
        ]),
        ("h2", "kampf", T(
            'Combat points, test route, coverage',
            'Kampfpunkte, Testroute, Abdeckung',
            '战斗点、测试路线、覆盖',
            '戦闘ポイント・テストルート・カバー範囲')),
        ("img", "h_weg_kampf", T(
            'Top view: combat points (violet boxes). The dotted lines are the connections the game makes for them by itself; the point by the wall gets no point of its own in the game and is assigned the waypoint at the end of the solid line.',
            'Draufsicht: Kampfpunkte (violette Kästen). Die gepunkteten Linien sind die Verbindungen, die das Spiel selbst für sie anlegt; der Punkt an der Wand wird im Spiel kein eigener Punkt und bekommt den Wegpunkt am Ende der durchgezogenen Linie.',
            '俯视图:战斗点(紫色方框)。虚线是游戏自己为它们建立的连接;墙边的那个点在游戏中不会成为独立的点,而是被分配到实线末端的路径点。',
            '上面図: 戦闘ポイント(紫の箱)。点線はゲームが自分で作る接続です。壁際の点はゲームでは独立した点にならず、実線の先のウェイポイントが割り当てられます。'), "breit"),
        ("legende", [
            (T("**{{WegKampfSetzen}}**"), T(
            'Click the floor to place a combat point (point_combat).',
            'Auf den Boden klicken setzt einen Kampfpunkt (point_combat).',
            '单击地面放置战斗点(point_combat)。',
            '床をクリックで戦闘ポイント (point_combat) を置きます。')),
            (T("**{{WegKampfTitel}}**"), T(
            "Warnings exactly like the game's messages; click one to fly there.",
            'Warnungen genau wie die Meldungen des Spiels; ein Klick fliegt hin.',
            '警告与游戏的消息完全一致;单击即可飞到那里。',
            '警告はゲームのメッセージとまったく同じです。クリックするとそこへ飛びます。')),
        ]),
        ("list", [
            T(
            'Gunners move to combat points for cover or a better angle. **{{WegKampfSetzen}}** places one per click on the floor, 25 above it like a waypoint; several clicks place several. Delete, undo and Ctrl+D work as for every entity.',
            'Schützen gehen zu Kampfpunkten, um Deckung oder einen besseren Winkel zu finden. **{{WegKampfSetzen}}** setzt je Klick einen auf den Boden, 25 darüber wie einen Wegpunkt; mehrere Klicks setzen mehrere. Entf, Rückgängig und Strg+D wirken wie bei jeder Entity.',
            '枪手会移动到战斗点寻找掩护或更好的角度。**{{WegKampfSetzen}}** 每次单击在地面上放置一个,像路径点一样高出地面 25;多次单击就放置多个。Delete、撤销和 Ctrl+D 与所有实体一样有效。',
            '銃を持つ敵は、遮蔽やよい角度を求めて戦闘ポイントへ移動します。**{{WegKampfSetzen}}** はクリックごとに床へ 1 つ、ウェイポイントと同じく 25 上に置きます。何度もクリックすれば何個でも置けます。Delete、元に戻す、Ctrl+D はほかのエンティティと同じです。'),
            T(
            'A combat point needs no target. When the map loads, the game makes it a point of the network itself and connects it to up to four nearby waypoints on the same floor (dotted lines). Then it assigns each combat point the nearest point of the network it can see - normally the combat point itself. Only if the game cannot make it a point (a box around it touches a wall), the solid line leads to the waypoint it gets instead.',
            'Ein Kampfpunkt braucht kein target. Beim Laden der Karte macht das Spiel ihn selbst zu einem Punkt des Netzes und verbindet ihn mit bis zu vier Wegpunkten in der Nähe auf demselben Boden (gepunktete Linien). Danach ordnet es jedem Kampfpunkt den nächsten Punkt des Netzes zu, den es sieht - normalerweise den Kampfpunkt selbst. Nur wenn das Spiel ihn nicht zum Punkt machen kann (ein Kasten um ihn berührt eine Wand), führt die durchgezogene Linie zu dem Wegpunkt, den er stattdessen bekommt.',
            '战斗点不需要 target。载入地图时,游戏会把它本身变成路网中的一个点,并把它连接到同一地面上附近最多四个路径点(虚线)。随后游戏为每个战斗点分配它能看到的最近的路网点——通常就是战斗点本身。只有当游戏无法把它变成点时(它周围的方框碰到墙),实线才会指向它改为得到的路径点。',
            '戦闘ポイントに target は要りません。マップを読み込むと、ゲームはそれ自体をネットワークの点にし、同じ床の近くのウェイポイント最大 4 つと接続します(点線)。そのあと各戦闘ポイントに、見える中で一番近いネットワークの点を割り当てます - ふつうは戦闘ポイント自身です。ゲームが点にできない場合(周りの箱が壁に触れる)だけ、実線が代わりに割り当てられるウェイポイントへ伸びます。'),
            T(
            'Red: the game prints "Combat Point at ... has no waypoint" for it - it finds no point of the network for it at all (for example on a map without connections, or by a wall far above all waypoints). Orange: "not connected to anything" - it is a point, but no waypoint is close enough on the same floor.',
            'Rot: Das Spiel meldet für ihn "Combat Point at ... has no waypoint" - es findet überhaupt keinen Punkt des Netzes für ihn (etwa auf einer Karte ohne Verbindungen oder an einer Wand weit über allen Wegpunkten). Orange: "not connected to anything" - er ist ein Punkt, aber kein Wegpunkt liegt nah genug auf demselben Boden.',
            '红色:游戏会为它报告 "Combat Point at ... has no waypoint"——游戏完全找不到适合它的路网点(例如在没有连接的地图上,或在高于所有路径点的墙边)。橙色:"not connected to anything"——它是一个点,但同一地面上没有足够近的路径点。',
            '赤: ゲームが "Combat Point at ... has no waypoint" を出します - それに合うネットワークの点がまったく見つかりません(接続のないマップや、すべてのウェイポイントよりずっと高い壁際など)。オレンジ: "not connected to anything" - 点にはなっていますが、同じ床に十分近いウェイポイントがありません。'),
        ]),
        ("img", "h_weg_deckung", T(
            'Suggestions behind cover: ghost points (dashed) with an arrow towards the enemy spots they hide from; the list in the window gives the reason.',
            'Vorschläge hinter Deckung: Geisterpunkte (gestrichelt) mit einem Pfeil zu den Feindstellen, vor denen sie decken; die Liste im Fenster nennt den Grund.',
            '掩护后的建议:虚影点(虚线)带有指向其所躲避的敌人位置的箭头;窗口中的列表给出原因。',
            '遮蔽物の陰の提案: ゴーストポイント (破線) と、隠れる相手の敵位置へ向かう矢印。ウィンドウの一覧に理由が出ます。'), "breit"),
        ("list", [
            T(
            '**{{WegDeckVorschlagen}}** looks for combat points the way a gunner picks one in the game. A stormtrooper who loses sight of you, or whom you come too close to, asks for a combat point with cover and a clear shot: from the point itself he must not see you (neither body nor head), but from the muzzle of his blaster, standing, he must - he ducks behind the cover and shoots over it. Candidates are the walkable floor near the waypoints; enemy spots are the walkable floor within sight range (1024), not closer than 200. The points with cover against the most enemy spots come first. Nothing changes until you accept: **{{WegDeckAnnehmen}}** for one, **{{WegDeckAlle}}** for all in one undo step. Like a point placed by hand, the game connects them to the waypoints itself; a suggestion the game would not connect ("has no waypoint") is dropped.',
            '**{{WegDeckVorschlagen}}** sucht Kampfpunkte so, wie ein Schütze im Spiel einen wählt. Ein Sturmtruppler, der dich aus den Augen verliert oder dem du zu nahe kommst, verlangt einen Kampfpunkt mit Deckung und freiem Schuss: vom Punkt selbst darf er dich nicht sehen (weder Körper noch Kopf), von der Mündung seines Blasters im Stehen aber schon - er duckt sich hinter die Deckung und schießt darüber. Kandidaten sind der begehbare Boden nahe den Wegpunkten, Feindstellen der begehbare Boden in Sichtweite (1024), nicht näher als 200. Die Punkte mit Deckung gegen die meisten Feindstellen kommen zuerst. Geändert wird erst beim Annehmen: **{{WegDeckAnnehmen}}** für einen, **{{WegDeckAlle}}** für alle in einem Rückgängig-Schritt. Wie einen von Hand gesetzten verbindet das Spiel sie selbst mit den Wegpunkten; ein Vorschlag, den das Spiel nicht verbinden würde ("has no waypoint"), entfällt.',
            '**{{WegDeckVorschlagen}}** 按游戏中枪手选择战斗点的方式寻找战斗点。看不到你、或被你靠得太近的冲锋队员,会要求一个有掩护且射击无阻的战斗点:从点本身他不能看到你(身体和头部都看不到),但站立时从他爆能枪的枪口可以看到——他蹲在掩体后,越过掩体射击。候选位置是路径点附近可行走的地面;敌人位置是视距(1024)内、不近于 200 的可行走地面。对最多敌人位置有掩护的点排在最前。接受之前不做任何更改:**{{WegDeckAnnehmen}}** 接受一个,**{{WegDeckAlle}}** 在一个撤销步骤中接受全部。与手动放置的一样,游戏会自己把它们连接到路径点;游戏不会连接的建议("has no waypoint")会被丢弃。',
            '**{{WegDeckVorschlagen}}** は、ゲームの射手が戦闘ポイントを選ぶのと同じ方法で探します。あなたを見失ったり、近づかれすぎたりしたストームトルーパーは、遮蔽と射線のある戦闘ポイントを求めます: ポイント自体からはあなたが見えてはならず (体も頭も)、立ったときのブラスターの銃口からは見えなければなりません - 遮蔽物の陰にかがみ、その上から撃ちます。候補はウェイポイント近くの歩ける床、敵の位置は視程 (1024) 内で 200 より遠い歩ける床です。最も多くの敵位置に対して遮蔽のあるポイントが先に来ます。受け入れるまで何も変わりません: **{{WegDeckAnnehmen}}** で 1 つ、**{{WegDeckAlle}}** で全部を元に戻す 1 回分で。手で置いたものと同じく、ゲームが自分でウェイポイントにつなぎます。ゲームがつながない提案 ("has no waypoint") は外されます。'),
            T(
            '**{{WegDeckAbstand}}** and **{{WegDeckHoechstens}}** set the density. The defaults come from the original maps of the rule set: the usual distance to the next combat point (Jedi Academy 231, Movie Duels 200, Jedi Outcast 145) and as many combat points per waypoint as there (Jedi Academy 0.52, Movie Duels 1.0, Jedi Outcast 0.45). With **{{WegDeckNurBereich}}** only floor inside a painted area gets suggestions. The search runs in the background with a progress bar; **{{WegAbbrechen}}** stops it and changes nothing. In Jedi Outcast the gunner checks from the point itself: body hidden, head visible.',
            '**{{WegDeckAbstand}}** und **{{WegDeckHoechstens}}** stellen die Dichte ein. Die Vorgaben stammen aus den Originalkarten des Regelwerks: der übliche Abstand zum nächsten Kampfpunkt (Jedi Academy 231, Movie Duels 200, Jedi Outcast 145) und so viele Kampfpunkte je Wegpunkt wie dort (Jedi Academy 0,52, Movie Duels 1,0, Jedi Outcast 0,45). Mit **{{WegDeckNurBereich}}** bekommt nur Boden in einem gemalten Bereich Vorschläge. Die Suche läuft im Hintergrund mit Fortschrittsbalken; **{{WegAbbrechen}}** hält sie an und ändert nichts. In Jedi Outcast prüft der Schütze vom Punkt selbst aus: Körper verdeckt, Kopf sichtbar.',
            '**{{WegDeckAbstand}}** 和 **{{WegDeckHoechstens}}** 设置密度。默认值来自该规则集的原版地图:到最近战斗点的常见距离(Jedi Academy 231、Movie Duels 200、Jedi Outcast 145),以及与原版相同的每个路径点的战斗点数量(Jedi Academy 0.52、Movie Duels 1.0、Jedi Outcast 0.45)。勾选 **{{WegDeckNurBereich}}** 后,只有绘制区域内的地面会得到建议。搜索在后台运行并显示进度条;**{{WegAbbrechen}}** 会停止搜索且不做任何更改。在 Jedi Outcast 中,枪手从点本身检查:身体被遮挡,头部可见。',
            '**{{WegDeckAbstand}}** と **{{WegDeckHoechstens}}** で密度を決めます。既定値はルールセットのオリジナルマップから取っています: 次の戦闘ポイントまでの普通の距離 (Jedi Academy 231、Movie Duels 200、Jedi Outcast 145) と、そこでのウェイポイントあたりの戦闘ポイント数 (Jedi Academy 0.52、Movie Duels 1.0、Jedi Outcast 0.45) です。**{{WegDeckNurBereich}}** をオンにすると、描いた範囲内の床だけに提案が出ます。検索は進行バー付きでバックグラウンドで動き、**{{WegAbbrechen}}** で止めても何も変わりません。Jedi Outcast では射手はポイント自体から確かめます: 体は隠れ、頭は見えること。'),
        ]),
        ("img", "h_weg_route", T(
            'Test route: from the floor next to the player start up onto the roof - the route an NPC takes in the game, with the jump edge (dashed).',
            'Testroute: vom Boden beim Spielerstart hinauf aufs Dach - der Weg, den ein NPC im Spiel nimmt, mit der Sprungkante (gestrichelt).',
            '测试路线:从玩家出生点旁的地面到屋顶——NPC 在游戏中走的路线,包括跳跃连线(虚线)。',
            'テストルート: プレイヤー開始位置のそばの床から屋根の上まで - NPC がゲームで通る経路で、ジャンプ辺は破線です。'), "breit"),
        ("list", [
            T(
            '**{{WegRoute}}**: click two points on the floor. The editor shows the route an NPC would take in the game - from the point of the network the game picks for the start to the one for the goal, along the network as the game builds it. With **{{WegRouteSpringen}}** the NPC may use jump edges (as with force jump). If there is no route, the window says why. A missing connection shows up at once, before you look for it in the game.',
            '**{{WegRoute}}**: zwei Punkte auf dem Boden anklicken. Der Editor zeigt den Weg, den ein NPC im Spiel nähme - vom Punkt des Netzes, den das Spiel für den Start wählt, zu dem für das Ziel, entlang des Netzes, wie das Spiel es baut. Mit **{{WegRouteSpringen}}** darf der NPC Sprungkanten nehmen (wie mit Machtsprung). Gibt es keinen Weg, sagt das Fenster, warum. Eine fehlende Verbindung fällt so sofort auf, bevor man sie im Spiel sucht.',
            '**{{WegRoute}}**:在地面上单击两个点。编辑器显示 NPC 在游戏中会走的路线——从游戏为起点选择的路网点到为终点选择的路网点,沿着游戏构建的路网。勾选 **{{WegRouteSpringen}}** 后,NPC 可以使用跳跃连线(如同有原力跳跃)。如果没有路线,窗口会说明原因。这样缺失的连接会立即显现,不必到游戏中去找。',
            '**{{WegRoute}}**: 床の 2 点をクリックします。エディターは、NPC がゲームで通る経路を表示します - ゲームが出発点に選ぶネットワークの点から目的地の点まで、ゲームが作るネットワークに沿って。**{{WegRouteSpringen}}** をオンにすると NPC はジャンプ辺を使えます(フォースジャンプがある場合と同じ)。経路がなければ、ウィンドウが理由を示します。足りない接続がゲームで探す前にすぐ分かります。'),
        ]),
        ("img", "h_weg_abdeckung", T(
            'Coverage in the top view: walkable floor without a waypoint within reach is red.',
            'Abdeckung in der Draufsicht: begehbarer Boden ohne Wegpunkt in Reichweite ist rot.',
            '俯视图中的覆盖:附近没有路径点的可行走地面显示为红色。',
            '上面図のカバー範囲: 届く範囲にウェイポイントがない歩ける床は赤です。'), "breit"),
        ("list", [
            T(
            '**{{WegAbdeckung}}** switches to the top view and colours every patch of floor red (32 x 32) from which no waypoint can be reached on foot within **{{WegAbstand}}** - walking like the generation, from the player start, the characters and the waypoints. These are the spots a generation or a path missed. It is calculated in the background and again after every change.',
            '**{{WegAbdeckung}}** schaltet in die Draufsicht und färbt jedes Stück Boden (32 x 32) rot, von dem aus zu Fuß in **{{WegAbstand}}** kein Wegpunkt erreichbar ist - gelaufen wie bei der Erzeugung, vom Spielerstart, den Figuren und den Wegpunkten aus. Das sind die Stellen, die eine Erzeugung oder ein Pfad ausgelassen hat. Gerechnet wird im Hintergrund und nach jeder Änderung neu.',
            '**{{WegAbdeckung}}** 会切换到俯视图,并把每块地面(32 x 32)涂成红色——从那里步行在 **{{WegAbstand}}** 内到达不了任何路径点。行走方式与生成相同,从玩家出生点、角色和路径点出发。这些就是生成或路径遗漏的地方。它在后台计算,每次更改后重新计算。',
            '**{{WegAbdeckung}}** は上面図に切り替え、歩いて **{{WegAbstand}}** 以内にウェイポイントへ届かない床の区画(32 x 32)をすべて赤く塗ります。歩き方は生成と同じで、プレイヤー開始位置・キャラクター・ウェイポイントから歩きます。これが生成やパスが抜かした場所です。計算はバックグラウンドで行われ、変更のたびにやり直されます。'),
            T(
            '**{{WegVersetzen}}** (on by default): a waypoint you place by hand where the game would drop it - in solid, too close to a wall or floating - goes to the nearest spot on the same floor where the game keeps it, with the same rules as the generation. The status line says how far and where to. Without it, the waypoint stays where you clicked and the status line warns.',
            '**{{WegVersetzen}}** (voreingestellt an): Ein Wegpunkt, den du von Hand dort setzt, wo das Spiel ihn verwerfen würde - im Festen, zu nah an einer Wand oder schwebend -, kommt an die nächste Stelle auf demselben Boden, an der das Spiel ihn behält, mit denselben Regeln wie die Erzeugung. Die Statuszeile sagt, wie weit und wohin. Ohne bleibt der Wegpunkt, wo du geklickt hast, und die Statuszeile warnt.',
            '**{{WegVersetzen}}**(默认开启):如果你手动放置的路径点会被游戏丢弃——在实体内、离墙太近或悬空——它会被放到同一地面上游戏会保留它的最近位置,规则与生成相同。状态栏会显示移动了多远、移到哪里。关闭时,路径点留在你单击的位置,状态栏会发出警告。',
            '**{{WegVersetzen}}**(既定でオン): ゲームが捨てる場所(ソリッド内、壁に近すぎる、浮いている)に手で置いたウェイポイントは、同じ床でゲームが残す一番近い位置へ、生成と同じ規則で移ります。ステータス行にどれだけどこへ動いたかが出ます。オフなら、ウェイポイントはクリックした所に残り、ステータス行が警告します。'),
        ]),
        ("note", T(
            "The cutscene preview uses this network for walks exactly like the game: far targets need a path here, otherwise the figure stands still ([[figuren#wieimspiel]]). A network needs at least four waypoints. Doors on the network count while they are open; a closed door counts only if it opens by itself for NPCs.",
            "Die Szenenvorschau nutzt dieses Netz für Läufe genau wie das Spiel: ferne Ziele brauchen hier einen Weg, sonst bleibt die Figur stehen ([[figuren#wieimspiel]]). Für ein Netz braucht es mindestens vier Wegpunkte. Türen im Netz zählen, solange sie offen sind; eine geschlossene nur, wenn sie für NPCs von selbst aufgeht.",
            "过场预览会像游戏一样使用此网络来行走:远处的目标需要在这里有路径,否则角色会站着不动([[figuren#wieimspiel]])。一个网络至少需要四个路径点。网络上的门在打开时才算数;关闭的门只有在会为 NPC 自动打开时才算。",
            "カットシーンのプレビューは、ゲームとまったく同じようにこのネットワークを歩行に使います。遠い目標にはここに経路が必要で、なければキャラクターは立ち止まります([[figuren#wieimspiel]])。ネットワークには最低 4 つのウェイポイントが必要です。ネットワーク上のドアは開いている間だけ通れます。閉じたドアは、NPC のために自動で開く場合だけ通れます。")),
    ],
}

SEITEN = [KARTE, ENTITIES] + inhalt_szene.SEITEN + [WEGPUNKTE]
