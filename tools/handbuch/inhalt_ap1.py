"""AP1 (MBENTWURF Teil B): die Seite "Model-Tab: die Zeitleiste" - die Zeitleiste nach dem freigegebenen Entwurf, je
Bedienmodus wie 3ds Max (Zeitschieber, Spurleiste, Auto Key / Set Key) bzw. wie Blender (Timeline), in jedem Bereich.

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_mb3b.py. Eingehaengt im Register
tools/handbuch/inhalt_modell.py (Zeile `Seite("inhalt_ap1")`).
"""

X = {
    "titel": ("Model tab: the timeline", "Model-Tab: die Zeitleiste", "模型选项卡: 时间轴", "モデルタブ: タイムライン",
              "Pestaña Model: la línea de tiempo", "Karta Model: časová osa"),
    "kurz": (
        "The timeline under the views works in every area of the Model tab and looks like the one of the chosen controls: time slider, track bar, Auto Key and Set Key like 3ds Max, or the Timeline like Blender.",
        "Die Zeitleiste unter den Ansichten arbeitet in jedem Bereich des Model-Tabs und sieht aus wie die des gewählten Bedienmodus: Zeitschieber, Spurleiste, Auto Key und Set Key wie 3ds Max oder die Timeline wie Blender.",
        "视图下方的时间轴在模型选项卡的每个区域都可用, 外观与所选操作方式一致: 像 3ds Max 的时间滑块、轨迹栏、Auto Key 和 Set Key, 或像 Blender 的时间线。",
        "ビューの下のタイムラインはモデルタブのどのエリアでも使え、選んだ操作モードと同じ見た目になります: 3ds Max 風のタイムスライダ、トラックバー、Auto Key と Set Key、または Blender 風のタイムライン。",
        "La línea de tiempo bajo las vistas funciona en todas las áreas de la pestaña Model y tiene el aspecto de los controles elegidos: regulador de tiempo, barra de pistas, Auto Key y Set Key como 3ds Max, o la línea de tiempo como Blender.",
        "Časová osa pod pohledy funguje v každé oblasti karty Model a vypadá jako u zvoleného ovládání: posuvník času, lišta stop, Auto Key a Set Key jako 3ds Max, nebo časová osa jako Blender."),
    "bild_max": (
        "The full timeline like 3ds Max in the area Animation: Auto Key and Set Key in the button row, the scale with a fine line per frame and the keys of the selection as coloured bars, the tracks per limb and bone, legend and status line.",
        "Die volle Zeitleiste wie 3ds Max im Bereich Animation: Auto Key und Set Key in der Knopfreihe, die Skala mit einem feinen Strich je Frame und den Schlüsseln der Auswahl als farbige Striche, die Spuren je Glied und Knochen, Legende und Statuszeile.",
        "动画区域中像 3ds Max 的完整时间轴: 按钮行中的 Auto Key 和 Set Key, 每帧一条细线的标尺和以彩色竖条显示的所选关键帧, 每个肢体和骨骼的轨道, 图例和状态行。",
        "アニメーションエリアの 3ds Max 風フルタイムライン: ボタン行の Auto Key と Set Key、フレームごとの細い線と選択のキー (色付きの棒) があるスケール、肢とボーンごとのトラック、凡例とステータス行。",
        "La línea de tiempo completa como 3ds Max en el área Animación: Auto Key y Set Key en la fila de botones, la escala con una línea fina por fotograma y las claves de la selección como barras de color, las pistas por miembro y hueso, leyenda y línea de estado.",
        "Úplná časová osa jako 3ds Max v oblasti Animace: Auto Key a Set Key v řadě tlačítek, stupnice s jemnou čárkou na snímek a klíči výběru jako barevnými proužky, stopy pro končetiny a kosti, legenda a stavový řádek."),
    "bild_blender": (
        "The same timeline like Blender: jumps to the previous and next key, the record button for auto keying, Start / End / Frame counted from 1, keys as diamonds.",
        "Dieselbe Zeitleiste wie Blender: Sprünge zum vorigen und nächsten Schlüssel, der Aufnahme-Knopf für Auto-Keying, Start / Ende / Bild ab 1 gezählt, Schlüssel als Rauten.",
        "同一时间轴像 Blender 时: 跳到上一个和下一个关键帧, 自动关键帧的录制按钮, 从 1 开始计数的起始 / 结束 / 帧, 关键帧为菱形。",
        "同じタイムラインの Blender 風: 前後のキーへのジャンプ、自動キー挿入の録画ボタン、1 から数える開始 / 終了 / フレーム、ひし形のキー。",
        "La misma línea de tiempo como Blender: saltos a la clave anterior y siguiente, el botón de grabación de claves automáticas, Inicio / Fin / Fotograma contados desde 1, claves como rombos.",
        "Stejná časová osa jako Blender: skoky na předchozí a další klíč, tlačítko nahrávání pro automatické klíčování, Začátek / Konec / Snímek počítané od 1, klíče jako kosočtverce."),
    "bild_pruefen": (
        "The area View/Check: speed, interpolation and force loop in the button row, below the scale the primary and the secondary sequence.",
        "Der Bereich Ansehen/Prüfen: Tempo, Interpolation und Schleife erzwingen in der Knopfreihe, unter der Skala die Primär- und die Sekundärsequenz.",
        "“查看/检查”区域: 按钮行中的速度、插值和强制循环, 标尺下方是主要序列和次要序列。",
        "「表示/確認」エリア: ボタン行に速度、補間、ループを強制、スケールの下にプライマリとセカンダリのシーケンス。",
        "El área Ver/Comprobar: velocidad, interpolación y forzar bucle en la fila de botones; bajo la escala, la secuencia primaria y la secundaria.",
        "Oblast Zobrazit/Zkontrolovat: rychlost, interpolace a vynucená smyčka v řadě tlačítek, pod stupnicí primární a sekundární sekvence."),
    "h_aufbau": ("Layout in every area", "Aufbau in jedem Bereich", "各区域中的布局", "各エリアでの構成", "Disposición en cada área", "Uspořádání v každé oblasti"),
    "aufbau": (
        '**Like 3ds Max in every area:** directly above the track bar lies the time slider - a wide handle with *frame / length* and the arrows < and > at its ends. Drag it to scrub, click into the track bar to jump, type a frame into the frame field; <kbd>,</kbd> and <kbd>.</kbd> step one frame. Keys in the track bar are selected with a click or a frame, dragged to move them and dragged with <kbd>Shift</kbd> to copy them; a right-click on a key opens its menu (delete, properties). Below lies one status line as in 3ds Max: on the left the selection and a hint for the mouse, in the middle X/Y/Z of the selection - with nothing selected the point under the mouse on the grid of that view - and the grid size, on the right the playback buttons, Auto Key / Set Key, the frame field and the time configuration (fps, length, interpolation, force loop). The buttons stand still while the frame number grows. The primary and the secondary sequence stand at the left of the track bar and in the tooltip of the time slider; **{{T1MiniKurve}}** opens the Dope Sheet and the curve editor. There is no explanatory text under the timeline - the key row can be switched on in the settings. Like Blender the timeline has its header and status line as in Blender.',
        '**Wie 3ds Max in jedem Bereich:** direkt über der Spurleiste liegt der Zeitschieber - ein breiter Griff mit *Bild / Länge* und den Pfeilen < und > an den Enden. Ziehen spult, ein Klick in die Spurleiste springt, ins Bildfeld lässt sich ein Bild tippen; <kbd>,</kbd> und <kbd>.</kbd> gehen ein Bild weiter. Schlüssel in der Spurleiste wählt man per Klick oder Rahmen, Ziehen verschiebt sie, Ziehen mit <kbd>Umschalt</kbd> kopiert sie; ein Rechtsklick auf einen Schlüssel öffnet sein Menü (Löschen, Eigenschaften). Darunter liegt eine Statuszeile wie in 3ds Max: links die Auswahl und ein Hinweis zur Maus, in der Mitte X/Y/Z der Auswahl - ohne Auswahl der Punkt unter der Maus auf dem Raster dieser Ansicht - und die Rastergröße, rechts die Abspielknöpfe, Auto Key / Set Key, das Bildfeld und die Zeitkonfiguration (fps, Länge, Interpolation, Schleife erzwingen). Die Knöpfe stehen still, wenn die Bildzahl wächst. Primär- und Sekundärsequenz stehen links an der Spurleiste und im Tooltip des Zeitschiebers; **{{T1MiniKurve}}** öffnet Dope Sheet und Kurveneditor. Unter der Zeitleiste steht kein erklärender Text - die Tastenzeile lässt sich in den Einstellungen einschalten. Wie Blender hat die Zeitleiste Kopf und Statuszeile wie in Blender.',
        '**每个区域都与 3ds Max 相同:** 轨迹栏正上方是时间滑块 - 一个宽滑块, 显示 *帧 / 长度*, 两端有箭头 < 和 >。拖动可拖拽播放, 在轨迹栏中单击可跳转, 可在帧字段中输入帧; <kbd>,</kbd> 和 <kbd>.</kbd> 前后移动一帧。轨迹栏中的关键帧可单击或框选, 拖动可移动, 按住 <kbd>Shift</kbd> 拖动可复制; 右键单击关键帧打开其菜单 (删除、属性)。下方是与 3ds Max 相同的一行状态栏: 左侧为选择和鼠标提示, 中间为所选对象的 X/Y/Z (未选择时为鼠标在该视图网格上的点) 和网格大小, 右侧为播放按钮、Auto Key / Set Key、帧字段和时间配置 (fps、长度、插值、强制循环)。帧数增加时按钮保持不动。主要和次要序列位于轨迹栏左侧和时间滑块的工具提示中; **{{T1MiniKurve}}** 打开摄影表和曲线编辑器。时间轴下方没有说明文字 - 按键行可在设置中打开。像 Blender 时, 时间轴的标题和状态栏与 Blender 相同。',
        '**どのエリアでも 3ds Max と同じ:** トラックバーのすぐ上にタイムスライダー - *フレーム / 長さ* を表示し両端に < と > の矢印がある幅広のつまみがあります。ドラッグでスクラブ、トラックバーのクリックでジャンプ、フレーム欄に入力して移動、<kbd>,</kbd> と <kbd>.</kbd> で 1 フレームずつ移動します。トラックバーのキーはクリックまたは範囲で選択、ドラッグで移動、<kbd>Shift</kbd>+ドラッグでコピー、キーの右クリックでメニュー (削除、プロパティ) が開きます。その下に 3ds Max と同じ 1 行のステータス バー: 左に選択とマウスのヒント、中央に選択の X/Y/Z (何も選択していないときはそのビューのグリッド上のマウス位置) とグリッド サイズ、右に再生ボタン、Auto Key / Set Key、フレーム欄、時間設定 (fps、長さ、補間、ループを強制)。フレーム番号が増えてもボタンは動きません。プライマリとセカンダリのシーケンスはトラックバーの左とタイムスライダーのツールチップにあり、**{{T1MiniKurve}}** でドープ シートとカーブ エディタを開きます。タイムラインの下に説明文はありません - キー行は設定で表示できます。Blender 風ではヘッダーとステータス行が Blender と同じです。',
        '**Como 3ds Max en todas las áreas:** justo encima de la barra de pistas está el control de tiempo, un tirador ancho con *fotograma / longitud* y las flechas < y > en sus extremos. Arrástrelo para recorrer, haga clic en la barra de pistas para saltar o escriba un fotograma en el campo de fotograma; <kbd>,</kbd> y <kbd>.</kbd> avanzan un fotograma. Las claves de la barra de pistas se seleccionan con un clic o un marco, se mueven arrastrando y se copian arrastrando con <kbd>Mayús</kbd>; un clic derecho en una clave abre su menú (eliminar, propiedades). Debajo hay una sola línea de estado como en 3ds Max: a la izquierda la selección y una indicación para el ratón, en el centro X/Y/Z de la selección (sin selección, el punto bajo el ratón en la cuadrícula de esa vista) y el tamaño de la cuadrícula, a la derecha los botones de reproducción, Auto Key / Set Key, el campo de fotograma y la configuración de tiempo (fps, longitud, interpolación, forzar bucle). Los botones no se mueven cuando crece el número de fotograma. La secuencia primaria y la secundaria están a la izquierda de la barra de pistas y en la descripción del control de tiempo; **{{T1MiniKurve}}** abre la hoja de claves y el editor de curvas. Bajo la línea de tiempo no hay texto explicativo; la fila de teclas se puede activar en la configuración. Como Blender, la línea de tiempo tiene la cabecera y la línea de estado de Blender.',
        '**Jako 3ds Max v každé oblasti:** přímo nad lištou stop je časový posuvník - široký úchyt s *snímek / délka* a šipkami < a > na koncích. Tažením se převíjí, klik do lišty stop skočí, do pole snímku lze snímek napsat; <kbd>,</kbd> a <kbd>.</kbd> posunou o jeden snímek. Klíče v liště stop se vybírají klikem nebo rámečkem, tažením se posouvají a tažením s <kbd>Shift</kbd> kopírují; pravý klik na klíč otevře jeho menu (smazat, vlastnosti). Pod ní je jeden stavový řádek jako v 3ds Max: vlevo výběr a nápověda k myši, uprostřed X/Y/Z výběru (bez výběru bod pod myší na mřížce daného pohledu) a velikost mřížky, vpravo tlačítka přehrávání, Auto Key / Set Key, pole snímku a nastavení času (fps, délka, interpolace, vynutit smyčku). Tlačítka stojí, i když číslo snímku roste. Primární a sekundární sekvence stojí vlevo u lišty stop a v bublině časového posuvníku; **{{T1MiniKurve}}** otevře dope sheet a editor křivek. Pod časovou osou není vysvětlující text - řádek kláves lze zapnout v nastavení. Jako Blender má časová osa záhlaví a stavový řádek jako v Blenderu.'),
    "h_max": ("Like 3ds Max", "Wie 3ds Max", "像 3ds Max", "3ds Max 風", "Como 3ds Max", "Jako 3ds Max"),
    "max": (
        "With the controls **{{MbWieMax}}** the button row has To start, Previous frame, Play, Next frame, To end, the frame display counted from 0 (12 / 119), **{{MzAutoKey}}** (N), **{{MzSetKey}}** (K) and the button "
        "with the frame rate, which opens the **{{MzZeitKonfig}}**. The scale and every track show a fine line per frame, like the track bar of 3ds Max. Keys are bars in the colours of the track bar: red for "
        "movement, green for rotation, both halves for a key that does both - JKA stores no scaling, so there is no blue. The row **{{MzAuswahlSpur}}** in the scale carries the keys of the shown bones: click a key to "
        "select it, drag it to move it in time, drag a frame around keys to select several. With **{{MzKeyModus}}** (timeline menu or time configuration) Previous frame, Next frame and the keys , and . jump from "
        "key to key.",
        "Im Bedienmodus **{{MbWieMax}}** hat die Knopfreihe Anfang, Bild zurück, Abspielen, Bild vor, Ende, die Bildanzeige ab 0 gezählt (12 / 119), **{{MzAutoKey}}** (N), **{{MzSetKey}}** (K) und den Knopf mit der "
        "Bildrate, der die **{{MzZeitKonfig}}** öffnet. Die Skala und jede Spur zeigen einen feinen Strich je Frame, wie die Spurleiste von 3ds Max. Schlüssel sind Striche in den Farben der Spurleiste: Rot für "
        "Bewegung, Grün für Drehung, beide Hälften für einen Schlüssel mit beidem - JKA speichert keine Skalierung, darum gibt es kein Blau. Die Zeile **{{MzAuswahlSpur}}** in der Skala trägt die Schlüssel der "
        "gezeigten Knochen: Klick wählt einen Schlüssel, Ziehen verschiebt ihn in der Zeit, ein Rahmen um Schlüssel wählt mehrere. Mit **{{MzKeyModus}}** (Menü der Zeitleiste oder Zeitkonfiguration) springen "
        "Bild zurück, Bild vor und die Tasten , und . von Schlüssel zu Schlüssel.",
        "操作方式为 **{{MbWieMax}}** 时, 按钮行有到开头、上一帧、播放、下一帧、到结尾、从 0 开始计数的帧显示 (12 / 119)、**{{MzAutoKey}}** (N)、**{{MzSetKey}}** (K) 以及显示帧率并打开 **{{MzZeitKonfig}}** 的按钮。"
        "标尺和每条轨道每帧显示一条细线, 与 3ds Max 的轨迹栏相同。关键帧是轨迹栏颜色的竖条: 红色表示移动, 绿色表示旋转, 两者都有时上下各半 - JKA 不保存缩放, 所以没有蓝色。标尺中的 **{{MzAuswahlSpur}}** "
        "行显示所显示骨骼的关键帧: 单击选择关键帧, 拖动在时间上移动, 围绕关键帧拖框可选择多个。开启 **{{MzKeyModus}}** (时间轴菜单或时间配置) 后, 上一帧、下一帧以及 , 和 . 键在关键帧之间跳转。",
        "操作モード **{{MbWieMax}}** では、ボタン行に先頭へ、前のフレーム、再生、次のフレーム、末尾へ、0 から数えるフレーム表示 (12 / 119)、**{{MzAutoKey}}** (N)、**{{MzSetKey}}** (K)、フレームレートを示し "
        "**{{MzZeitKonfig}}** を開くボタンがあります。スケールと各トラックには 3ds Max のトラックバーのようにフレームごとの細い線があります。キーはトラックバーの色の棒です: 赤は移動、緑は回転、両方の場合は"
        "上下半分ずつ - JKA はスケールを保存しないので青はありません。スケール内の **{{MzAuswahlSpur}}** 行は表示中のボーンのキーを示します: クリックで選択、ドラッグで時間移動、キーを枠で囲んで複数選択。"
        "**{{MzKeyModus}}** (タイムラインメニューまたは時間設定) をオンにすると、前のフレーム、次のフレーム、, と . のキーがキーからキーへ移動します。",
        "Con los controles **{{MbWieMax}}** la fila de botones tiene Al inicio, Fotograma anterior, Reproducir, Fotograma siguiente, Al final, el indicador de fotograma contado desde 0 (12 / 119), **{{MzAutoKey}}** "
        "(N), **{{MzSetKey}}** (K) y el botón con la velocidad de fotogramas, que abre la **{{MzZeitKonfig}}**. La escala y cada pista muestran una línea fina por fotograma, como la barra de pistas de 3ds Max. Las "
        "claves son barras con los colores de la barra de pistas: rojo para movimiento, verde para rotación, ambas mitades para una clave con las dos cosas; JKA no guarda escalado, por eso no hay azul. La fila "
        "**{{MzAuswahlSpur}}** de la escala lleva las claves de los huesos mostrados: clic selecciona una clave, arrastrar la mueve en el tiempo, un marco alrededor de claves selecciona varias. Con **{{MzKeyModus}}** "
        "(menú de la línea de tiempo o configuración de tiempo), Fotograma anterior, Fotograma siguiente y las teclas , y . saltan de clave en clave.",
        "S ovládáním **{{MbWieMax}}** má řada tlačítek Na začátek, Předchozí snímek, Přehrát, Další snímek, Na konec, zobrazení snímku počítané od 0 (12 / 119), **{{MzAutoKey}}** (N), **{{MzSetKey}}** (K) a tlačítko "
        "se snímkovou frekvencí, které otevře **{{MzZeitKonfig}}**. Stupnice i každá stopa ukazují jemnou čárku na snímek jako lišta stop ve 3ds Max. Klíče jsou proužky v barvách lišty stop: červená pro posun, "
        "zelená pro rotaci, obě poloviny pro klíč s obojím - JKA neukládá měřítko, proto chybí modrá. Řádek **{{MzAuswahlSpur}}** ve stupnici nese klíče zobrazených kostí: kliknutím klíč vyberete, tažením ho "
        "posunete v čase, rámečkem kolem klíčů jich vyberete víc. S **{{MzKeyModus}}** (nabídka časové osy nebo nastavení času) skáčou Předchozí snímek, Další snímek a klávesy , a . z klíče na klíč."),
    "h_blender": ("Like Blender", "Wie Blender", "像 Blender", "Blender 風", "Como Blender", "Jako Blender"),
    "blender": (
        "With the controls **{{MbWieBlender}}** the row has To start, Previous key (↓), Play (Space), Next key (↑), To end, the record button for **{{MzAutoKeying}}**, the fields **{{MzStart}}**, **{{MzEndeFeld}}** and "
        "**{{MzBild}}** counted from 1, and the frame rate of the animation.cfg. Keys are diamonds, the row in the scale is called **{{MbKvZusammenfassung}}**. **{{MzStart}}** and **{{MzEndeFeld}}** are the first "
        "frame and the count of the sequence in the animation.cfg - a change there changes that line (undo with Ctrl+Z). In the area **{{MbBereichAnimation}}** K opens the keying menu and I sets a key; with the "
        "mouse over the tracks Home shows the whole sequence. The timeline menu is grouped like the header of Blender's timeline: **{{MzMenuePlayback}}**, **{{MzKeying}}**, **{{MzMenueView}}**, **{{MzMenueMarker}}**.",
        "Im Bedienmodus **{{MbWieBlender}}** hat die Reihe Anfang, Schlüssel zurück (↓), Abspielen (Leertaste), Schlüssel vor (↑), Ende, den Aufnahme-Knopf für **{{MzAutoKeying}}**, die Felder **{{MzStart}}**, "
        "**{{MzEndeFeld}}** und **{{MzBild}}** ab 1 gezählt und die Bildrate der animation.cfg. Schlüssel sind Rauten, die Zeile in der Skala heißt **{{MbKvZusammenfassung}}**. **{{MzStart}}** und **{{MzEndeFeld}}** "
        "sind das erste Bild und die Anzahl der Sequenz in der animation.cfg - eine Änderung dort ändert diese Zeile (Rückgängig mit Strg+Z). Im Bereich **{{MbBereichAnimation}}** öffnet K das Keying-Menü und I "
        "setzt einen Schlüssel; steht die Maus über den Spuren, zeigt Pos1 die ganze Sequenz. Das Menü der Zeitleiste ist gegliedert wie die Kopfzeile der Timeline von Blender: **{{MzMenuePlayback}}**, **{{MzKeying}}**, **{{MzMenueView}}**, **{{MzMenueMarker}}**.",
        "操作方式为 **{{MbWieBlender}}** 时, 按钮行有到开头、上一个关键帧 (↓)、播放 (空格)、下一个关键帧 (↑)、到结尾、**{{MzAutoKeying}}** 的录制按钮、从 1 开始计数的 **{{MzStart}}**、**{{MzEndeFeld}}** 和 "
        "**{{MzBild}}** 字段, 以及 animation.cfg 的帧率。关键帧是菱形, 标尺中的行称为 **{{MbKvZusammenfassung}}**。**{{MzStart}}** 和 **{{MzEndeFeld}}** 是序列在 animation.cfg 中的第一帧和帧数 - 在此更改会"
        "修改该行 (用 Ctrl+Z 撤销)。在 **{{MbBereichAnimation}}** 区域, K 打开关键帧菜单, I 设置关键帧; 鼠标位于轨道上方时, Home 显示整个序列。时间轴菜单的分组与 Blender 时间线的标题栏相同: **{{MzMenuePlayback}}**, **{{MzKeying}}**, **{{MzMenueView}}**, **{{MzMenueMarker}}**。",
        "操作モード **{{MbWieBlender}}** では、行に先頭へ、前のキー (↓)、再生 (Space)、次のキー (↑)、末尾へ、**{{MzAutoKeying}}** の録画ボタン、1 から数える **{{MzStart}}**、**{{MzEndeFeld}}**、**{{MzBild}}** "
        "のフィールド、animation.cfg のフレームレートがあります。キーはひし形で、スケール内の行は **{{MbKvZusammenfassung}}** です。**{{MzStart}}** と **{{MzEndeFeld}}** は animation.cfg のシーケンスの最初の"
        "フレームと数で、ここを変えるとその行が変わります (Ctrl+Z で元に戻す)。**{{MbBereichAnimation}}** エリアでは K でキーイングメニュー、I でキーを設定。マウスがトラックの上にあるとき Home でシーケンス全体を"
        "表示します。タイムラインメニューは Blender のタイムラインのヘッダーと同じ分類です: **{{MzMenuePlayback}}**, **{{MzKeying}}**, **{{MzMenueView}}**, **{{MzMenueMarker}}**。",
        "Con los controles **{{MbWieBlender}}** la fila tiene Al inicio, Clave anterior (↓), Reproducir (Espacio), Clave siguiente (↑), Al final, el botón de grabación de **{{MzAutoKeying}}**, los campos "
        "**{{MzStart}}**, **{{MzEndeFeld}}** y **{{MzBild}}** contados desde 1, y la velocidad de fotogramas de animation.cfg. Las claves son rombos y la fila de la escala se llama **{{MbKvZusammenfassung}}**. "
        "**{{MzStart}}** y **{{MzEndeFeld}}** son el primer fotograma y el número de la secuencia en animation.cfg: cambiarlos modifica esa línea (deshacer con Ctrl+Z). En el área **{{MbBereichAnimation}}**, K abre "
        "el menú de claves e I fija una clave; con el ratón sobre las pistas, Inicio muestra toda la secuencia. El menú de la línea de tiempo se agrupa como la cabecera de la línea de tiempo de Blender: "
        "**{{MzMenuePlayback}}**, **{{MzKeying}}**, **{{MzMenueView}}**, **{{MzMenueMarker}}**.",
        "S ovládáním **{{MbWieBlender}}** má řada Na začátek, Předchozí klíč (↓), Přehrát (mezerník), Další klíč (↑), Na konec, tlačítko nahrávání pro **{{MzAutoKeying}}**, pole **{{MzStart}}**, **{{MzEndeFeld}}** a "
        "**{{MzBild}}** počítaná od 1 a snímkovou frekvenci z animation.cfg. Klíče jsou kosočtverce, řádek ve stupnici se jmenuje **{{MbKvZusammenfassung}}**. **{{MzStart}}** a **{{MzEndeFeld}}** jsou první snímek "
        "a počet snímků sekvence v animation.cfg - změna tam změní ten řádek (zpět pomocí Ctrl+Z). V oblasti **{{MbBereichAnimation}}** otevře K nabídku klíčování a I nastaví klíč; s myší nad stopami ukáže Home "
        "celou sekvenci. Nabídka časové osy je členěná jako záhlaví časové osy v Blenderu: **{{MzMenuePlayback}}**, **{{MzKeying}}**, **{{MzMenueView}}**, **{{MzMenueMarker}}**."),
    "h_autokey": ("Auto Key and Set Key", "Auto Key und Set Key", "Auto Key 和 Set Key", "Auto Key と Set Key", "Auto Key y Set Key", "Auto Key a Set Key"),
    "autokey": (
        "**{{MzAutoKey}}** (like Blender **{{MzAutoKeying}}**) is off from the start, as in 3ds Max and Blender: you pose first - the pose stays without a key, the line under the timeline says so, and "
        "**{{MzSetKey}}** (K, like Blender I) keeps it. If you change the frame or the sequence before that, the pose is discarded - as in the Set Key mode of 3ds Max and in Blender without auto keying. "
        "Switched on (the button, N like 3ds Max, the record button like Blender), every change at a control - the gizmo, G/R, the sliders of IK/FK and of the foot roll - sets a key at the current frame. "
        "The program remembers your choice for the next start. Commands such as paste pose, bind pose and the hand poses always set their keys.",
        "**{{MzAutoKey}}** (wie Blender **{{MzAutoKeying}}**) ist ab Werk aus, wie in 3ds Max und Blender: man stellt erst die Pose - sie bleibt ohne Schlüssel, die Zeile unter der Zeitleiste sagt es, und "
        "**{{MzSetKey}}** (K, wie Blender I) behält sie. Wechselt man vorher das Bild oder die Sequenz, wird die Pose verworfen - wie im Set-Key-Modus von 3ds Max und in Blender ohne Auto-Keying. "
        "Eingeschaltet (der Knopf, N wie 3ds Max, der Aufnahme-Knopf wie Blender) setzt jede Änderung an einem Griff - das Gizmo, G/R, die Regler von IK/FK und der Fußrolle - einen Schlüssel am aktuellen Bild. "
        "Das Programm merkt sich die Wahl für den nächsten Start. Befehle wie Pose einfügen, Bindepose und die Handposen setzen ihre Schlüssel immer.",
        "**{{MzAutoKey}}** (像 Blender 时为 **{{MzAutoKeying}}**) 默认关闭, 与 3ds Max 和 Blender 相同: 先摆姿势 - 姿势没有关键帧, 时间轴下方的行会提示, **{{MzSetKey}}** (K, 像 Blender 时为 I) 将其保留。"
        "如果在此之前更改帧或序列, 姿势会被丢弃 - 与 3ds Max 的 Set Key 模式以及未开启自动关键帧的 Blender 相同。开启后 (按钮、像 3ds Max 时按 N、像 Blender 时为录制按钮), 对控制器的每次更改 - 变换控件、G/R、"
        "IK/FK 和脚部滚动的滑块 - 都会在当前帧设置关键帧。程序会为下次启动记住此选择。粘贴姿势、绑定姿势和手部姿势等命令始终设置关键帧。",
        "**{{MzAutoKey}}** (Blender 風では **{{MzAutoKeying}}**) は 3ds Max や Blender と同じく最初はオフです: 先にポーズを作ります - ポーズはキーなしのまま、タイムライン下の行がそれを示し、"
        "**{{MzSetKey}}** (K、Blender 風では I) で保持します。その前にフレームやシーケンスを変えるとポーズは破棄されます - 3ds Max の Set Key モードや自動キー挿入なしの Blender と同じです。"
        "オンにすると (ボタン、3ds Max 風では N、Blender 風では録画ボタン)、コントロールの変更 - ギズモ、G/R、IK/FK とフットロールのスライダー - はそのたびに現在のフレームにキーを設定します。"
        "選択は次回の起動のために記憶されます。ポーズの貼り付け、バインドポーズ、手のポーズなどのコマンドは常にキーを設定します。",
        "**{{MzAutoKey}}** (como Blender **{{MzAutoKeying}}**) viene desactivado, como en 3ds Max y Blender: primero se hace la pose - queda sin clave, la línea bajo la línea de tiempo lo indica y "
        "**{{MzSetKey}}** (K, como Blender I) la conserva. Si antes cambias de fotograma o de secuencia, la pose se descarta, como en el modo Set Key de 3ds Max y en Blender sin claves automáticas. "
        "Activado (el botón, N como 3ds Max, el botón de grabación como Blender), cada cambio en un control - el gizmo, G/R, los reguladores de IK/FK y del balanceo del pie - fija una clave en el fotograma actual. "
        "El programa recuerda tu elección para el próximo inicio. Órdenes como pegar pose, pose de enlace y las poses de mano siempre fijan sus claves.",
        "**{{MzAutoKey}}** (jako Blender **{{MzAutoKeying}}**) je od začátku vypnutý jako ve 3ds Max a v Blenderu: nejdřív se dělá póza - zůstane bez klíče, řádek pod časovou osou to říká a "
        "**{{MzSetKey}}** (K, jako Blender I) ji zachová. Změníte-li předtím snímek nebo sekvenci, póza se zahodí - jako v režimu Set Key ve 3ds Max a v Blenderu bez automatického klíčování. "
        "Zapnutý (tlačítko, N jako 3ds Max, nahrávací tlačítko jako Blender) nastaví každá změna ovladače - gizmo, G/R, posuvníky IK/FK a odvalu chodidla - klíč na aktuálním snímku. "
        "Program si volbu zapamatuje pro příští spuštění. Příkazy jako vložit pózu, vazební póza a pózy rukou své klíče nastavují vždy."),
    "h_zeitkonfig": ("Time configuration", "Zeitkonfiguration", "时间配置", "時間設定", "Configuración de tiempo", "Nastavení času"),
    "zeitkonfig": (
        "The **{{MzZeitKonfig}}** shows the frame rate, switches the time display between **{{MzFrames}}** and **{{MzSekunden}}** (scale and playhead), holds the speed, **{{MzSchleife}}**, **{{MbInterpolation}}** and "
        "**{{MzKeyModus}}**, and the line of the sequence in the animation.cfg: first frame, count, loop frame and fps with the bar whose ends you can drag. The skin list and the surface counter that stood in the "
        "timeline until rc611 are in the group **{{MbRLod}}** of the area **{{MbBereichAnsehen}}**; the surface list for the detail log is also in the search.",
        "Die **{{MzZeitKonfig}}** zeigt die Bildrate, schaltet die Zeitanzeige zwischen **{{MzFrames}}** und **{{MzSekunden}}** (Skala und Abspielkopf), hält Tempo, **{{MzSchleife}}**, **{{MbInterpolation}}** und "
        "**{{MzKeyModus}}** und die Zeile der Sequenz in der animation.cfg: Anfang, Anzahl, Schleifenbild und fps mit dem Balken, dessen Enden man ziehen kann. Die Hautliste und der Flächenzähler, die bis rc611 in "
        "der Zeitleiste standen, stehen in der Gruppe **{{MbRLod}}** des Bereichs **{{MbBereichAnsehen}}**; die Flächenliste fürs Detailprotokoll gibt es auch in der Suche.",
        "**{{MzZeitKonfig}}** 显示帧率, 在 **{{MzFrames}}** 和 **{{MzSekunden}}** 之间切换时间显示 (标尺和播放头), 包含速度、**{{MzSchleife}}**、**{{MbInterpolation}}** 和 **{{MzKeyModus}}**, 以及序列在 "
        "animation.cfg 中的那一行: 起始帧、帧数、循环帧和 fps, 并带有可拖动两端的条。rc611 之前位于时间轴中的皮肤列表和面计数现在位于 **{{MbBereichAnsehen}}** 区域的 **{{MbRLod}}** 组中; 写入详细日志的面列表"
        "也可在搜索中找到。",
        "**{{MzZeitKonfig}}** はフレームレートを表示し、時間表示を **{{MzFrames}}** と **{{MzSekunden}}** で切り替え (スケールと再生ヘッド)、速度、**{{MzSchleife}}**、**{{MbInterpolation}}**、"
        "**{{MzKeyModus}}**、そして animation.cfg のシーケンスの行を持ちます: 開始フレーム、数、ループフレーム、fps と、両端をドラッグできるバー。rc611 までタイムラインにあったスキン一覧とサーフェス数は "
        "**{{MbBereichAnsehen}}** エリアの **{{MbRLod}}** グループにあります。詳細ログへのサーフェス一覧は検索にもあります。",
        "La **{{MzZeitKonfig}}** muestra la velocidad de fotogramas, cambia la visualización del tiempo entre **{{MzFrames}}** y **{{MzSekunden}}** (escala y cabezal), contiene la velocidad, **{{MzSchleife}}**, "
        "**{{MbInterpolation}}** y **{{MzKeyModus}}**, y la línea de la secuencia en animation.cfg: primer fotograma, número, fotograma de bucle y fps con la barra cuyos extremos se pueden arrastrar. La lista de "
        "skins y el contador de superficies que hasta rc611 estaban en la línea de tiempo están en el grupo **{{MbRLod}}** del área **{{MbBereichAnsehen}}**; la lista de superficies para el registro detallado "
        "también está en la búsqueda.",
        "**{{MzZeitKonfig}}** ukazuje snímkovou frekvenci, přepíná zobrazení času mezi **{{MzFrames}}** a **{{MzSekunden}}** (stupnice a přehrávací hlava), obsahuje rychlost, **{{MzSchleife}}**, "
        "**{{MbInterpolation}}** a **{{MzKeyModus}}** a řádek sekvence v animation.cfg: první snímek, počet, snímek smyčky a fps s pruhem, jehož konce lze táhnout. Seznam skinů a počítadlo povrchů, které byly do "
        "rc611 v časové ose, jsou ve skupině **{{MbRLod}}** oblasti **{{MbBereichAnsehen}}**; seznam povrchů do podrobného protokolu je i v hledání."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    z = X[schluessel]
    return T(z[0], z[1], z[2], z[3])


def seite():
    """Die Seite modell-zeitleiste (Gruppe Missionen, nach modell-bedienung)."""
    return {
        "id": "modell-zeitleiste",
        "gruppe": "mission",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("img", "h_ap1_max", _t("bild_max"), "breit"),
            ("h2", "aufbau", _t("h_aufbau")),
            ("p", _t("aufbau")),
            ("img", "h_ap1_pruefen", _t("bild_pruefen"), "breit"),
            ("h2", "max", _t("h_max")),
            ("p", _t("max")),
            ("h2", "blender", _t("h_blender")),
            ("p", _t("blender")),
            ("img", "h_ap1_blender", _t("bild_blender"), "breit"),
            ("h2", "autokey", _t("h_autokey")),
            ("p", _t("autokey")),
            ("h2", "zeitkonfig", _t("h_zeitkonfig")),
            ("p", _t("zeitkonfig")),
        ],
    }
