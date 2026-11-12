"""AP10 (MBENTWURF Teil B): die Seite "Model-Tab: das Feld im Bereich Animation" - das rechte Feld nach dem freigegebenen
Entwurf: Animation, Rig erstellen (Vorlagen), Presets, Vorhandene Animation bearbeiten, Uebertragen, IK/FK, Anbauten und
Vorschau, Fuer das Spiel bauen; dahinter die weiteren Gruppen.

Eigene Quelle des Pakets; AP14 haengt die Seite ins Handbuch (inhalt.py: `inhalt_ap10.seite()` hinter modell-zeitleiste,
ES/CS in inhalt_es.py / inhalt_cs.py, die Texte von i18n_ap3 und i18n_ap10 in gen_handbuch.py `_lade_ui_texte`) - wie AP6-AP9.
Die Bilder h_ap10_max und h_ap10_blender nimmt der Selbsttest auf (BHED_EDITORTEST=handbuch, BHED_HB_NUR=ap10). Jeder Text
einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_ap1.py; {{Kennung}} setzt den Text der Oberflaeche ein.
"""

X = {
    "titel": ("Model tab: the panel of the area Animation", "Model-Tab: das Feld im Bereich Animation", "模型选项卡: 动画区域的面板", "モデルタブ: アニメーションエリアのパネル",
              "Pestaña Model: el panel del área Animación", "Karta Model: panel oblasti Animace"),
    "kurz": (
        "The right panel of the area Animation shows its groups in the order of the work: the sequence and its target, the rig, presets, editing an existing animation, transfer from another skeleton, IK/FK, attachments and building for the game.",
        "Das rechte Feld im Bereich Animation zeigt seine Gruppen in der Reihenfolge der Arbeit: die Sequenz und ihr Ziel, das Rig, Presets, das Bearbeiten einer vorhandenen Animation, das Übertragen von einem anderen Skelett, IK/FK, Anbauten und das Bauen für das Spiel.",
        "动画区域的右侧面板按工作顺序显示各组: 序列及其目标、绑定、预设、编辑现有动画、从其他骨架传递、IK/FK、附加物以及为游戏生成。",
        "アニメーションエリアの右パネルは、作業の順にグループを表示します: シーケンスとその出力先、リグ、プリセット、既存アニメーションの編集、別のスケルトンからの転送、IK/FK、取り付け、ゲーム用の作成。",
        "El panel derecho del área Animación muestra sus grupos en el orden del trabajo: la secuencia y su destino, el rig, los preajustes, la edición de una animación existente, la transferencia desde otro esqueleto, IK/FK, los acoples y la creación para el juego.",
        "Pravý panel oblasti Animace zobrazuje skupiny v pořadí práce: sekvence a její cíl, rig, předvolby, úprava existující animace, přenos z jiné kostry, IK/FK, připojené modely a sestavení pro hru."),
    "bild_max": (
        "The area Animation like 3ds Max: the command panel on the tab Motion with the groups Animation, Create rig, Presets and the following ones; every group has a heading with a short info, buttons in a grid and one highlighted main button.",
        "Der Bereich Animation wie 3ds Max: das Befehlsfeld im Reiter Bewegung mit den Gruppen Animation, Rig erstellen, Presets und den folgenden; jede Gruppe hat eine Überschrift mit Kurzinfo, Knöpfe im Raster und einen hervorgehobenen Hauptknopf.",
        "像 3ds Max 的动画区域: 命令面板位于“运动”选项卡, 含有“动画”“创建绑定”“预设”及后续各组; 每个组都有带简要信息的标题、网格排列的按钮和一个突出显示的主按钮。",
        "3ds Max 風のアニメーションエリア: コマンドパネルの「モーション」タブに、アニメーション、リグを作成、プリセットなどのグループが並びます。各グループには短い情報付きの見出し、グリッドのボタン、強調されたメインボタンがあります。",
        "El área Animación como 3ds Max: el panel de comandos en la pestaña Movimiento con los grupos Animación, Crear rig, Presets y los siguientes; cada grupo tiene un encabezado con información breve, botones en cuadrícula y un botón principal resaltado.",
        "Oblast Animace jako 3ds Max: příkazový panel na kartě Pohyb se skupinami Animace, Vytvořit rig, Předvolby a dalšími; každá skupina má nadpis s krátkou informací, tlačítka v mřížce a jedno zvýrazněné hlavní tlačítko."),
    "bild_blender": (
        "The same area like Blender: the properties editor on the tab Bone with the same groups as rounded panels; IK/FK shows one slider per limb.",
        "Derselbe Bereich wie Blender: der Eigenschaften-Editor im Reiter Knochen mit denselben Gruppen als abgerundete Panels; IK/FK zeigt einen Regler je Glied.",
        "同一区域像 Blender 时: 属性编辑器位于“骨骼”选项卡, 相同的组显示为圆角面板; IK/FK 为每个肢体显示一个滑块。",
        "同じエリアの Blender 風: プロパティエディターの「ボーン」タブに、同じグループが角丸のパネルとして並びます。IK/FK は手足ごとにスライダーを 1 本表示します。",
        "La misma área como Blender: el editor de propiedades en la pestaña Hueso con los mismos grupos como paneles redondeados; IK/FK muestra un regulador por extremidad.",
        "Stejná oblast jako Blender: editor vlastností na kartě Kost se stejnými skupinami jako zaoblené panely; IK/FK ukazuje jeden posuvník na končetinu."),
    "h_gruppen": ("The groups and their tabs", "Die Gruppen und ihre Reiter", "各组及其选项卡", "グループとタブ", "Los grupos y sus pestañas", "Skupiny a jejich karty"),
    "gruppen": (
        "On the tab that the area chooses (**{{MfBewegung}}** like 3ds Max, **{{MfKnochen}}** like Blender) all groups stand one below the other: **{{AzRAnimation}}**, **{{AzRRig}}**, **{{PrRollout}}**, "
        "**{{AzRBearbeiten}}**, **{{AzRUebertragen}}**, **{{MbIkRollout}}**, **{{AzRAnbauten}}**, **{{AzRBauen}}**, and behind them **{{PhRollout}}**, **{{MbRgGruppePose}}**, **{{MbRgGhosts}}**, **{{MbRgPfade}}**, "
        "**{{MbKvRollout}}**, **{{MbKvGruppeBibliothek}}**, **{{MbRGla}}**, **{{MbRCfg}}** and **{{MbRKartenGla}}**. A click on a heading folds the group. The other tabs show their part: **{{MfHierarchie}}** the rig, "
        "IK/FK and the attachments, **{{MfDienste}}** building for the game, the pose library, GLA, animation.cfg and the slots (like Blender: **{{MfConstraints}}** and **{{MfDaten}}**). "
        "**{{AzRAnimation}}** names the sequence you work on, the target GLA and how many of its slots are taken. Dope sheet and curve editor open in the timeline.",
        "Im Reiter, den der Bereich vorgibt (**{{MfBewegung}}** wie 3ds Max, **{{MfKnochen}}** wie Blender), stehen alle Gruppen untereinander: **{{AzRAnimation}}**, **{{AzRRig}}**, **{{PrRollout}}**, "
        "**{{AzRBearbeiten}}**, **{{AzRUebertragen}}**, **{{MbIkRollout}}**, **{{AzRAnbauten}}**, **{{AzRBauen}}** und dahinter **{{PhRollout}}**, **{{MbRgGruppePose}}**, **{{MbRgGhosts}}**, **{{MbRgPfade}}**, "
        "**{{MbKvRollout}}**, **{{MbKvGruppeBibliothek}}**, **{{MbRGla}}**, **{{MbRCfg}}** und **{{MbRKartenGla}}**. Ein Klick auf eine Überschrift klappt die Gruppe zu. Die anderen Reiter zeigen ihren Teil: "
        "**{{MfHierarchie}}** das Rig, IK/FK und die Anbauten, **{{MfDienste}}** das Bauen für das Spiel, die Posen-Bibliothek, GLA, animation.cfg und die Plätze (wie Blender: **{{MfConstraints}}** und **{{MfDaten}}**). "
        "**{{AzRAnimation}}** nennt die Sequenz, an der man arbeitet, die Ziel-GLA und wie viele ihrer Plätze belegt sind. Dopesheet und Kurveneditor öffnet die Zeitleiste.",
        "在该区域指定的选项卡 (像 3ds Max 时为 **{{MfBewegung}}**, 像 Blender 时为 **{{MfKnochen}}**) 中, 所有组依次排列: **{{AzRAnimation}}**、**{{AzRRig}}**、**{{PrRollout}}**、**{{AzRBearbeiten}}**、"
        "**{{AzRUebertragen}}**、**{{MbIkRollout}}**、**{{AzRAnbauten}}**、**{{AzRBauen}}**, 其后是 **{{PhRollout}}**、**{{MbRgGruppePose}}**、**{{MbRgGhosts}}**、**{{MbRgPfade}}**、**{{MbKvRollout}}**、"
        "**{{MbKvGruppeBibliothek}}**、**{{MbRGla}}**、**{{MbRCfg}}** 和 **{{MbRKartenGla}}**。单击标题可折叠该组。其他选项卡显示各自的部分: **{{MfHierarchie}}** 显示绑定、IK/FK 和附加物, **{{MfDienste}}** "
        "显示为游戏生成、姿势库、GLA、animation.cfg 和槽位 (像 Blender 时为 **{{MfConstraints}}** 和 **{{MfDaten}}**)。**{{AzRAnimation}}** 给出正在处理的序列、目标 GLA 以及已占用的槽位数。摄影表和曲线编辑器在时间轴中打开。",
        "エリアが指定するタブ (3ds Max 風は **{{MfBewegung}}**、Blender 風は **{{MfKnochen}}**) には、すべてのグループが縦に並びます: **{{AzRAnimation}}**、**{{AzRRig}}**、**{{PrRollout}}**、**{{AzRBearbeiten}}**、"
        "**{{AzRUebertragen}}**、**{{MbIkRollout}}**、**{{AzRAnbauten}}**、**{{AzRBauen}}**、その後ろに **{{PhRollout}}**、**{{MbRgGruppePose}}**、**{{MbRgGhosts}}**、**{{MbRgPfade}}**、**{{MbKvRollout}}**、"
        "**{{MbKvGruppeBibliothek}}**、**{{MbRGla}}**、**{{MbRCfg}}**、**{{MbRKartenGla}}**。見出しをクリックするとグループが折りたたまれます。他のタブはそれぞれの部分を表示します: **{{MfHierarchie}}** はリグ、IK/FK、取り付け、"
        "**{{MfDienste}}** はゲーム用の作成、ポーズライブラリ、GLA、animation.cfg、スロット (Blender 風では **{{MfConstraints}}** と **{{MfDaten}}**)。**{{AzRAnimation}}** は作業中のシーケンス、出力先 GLA、"
        "使用中のスロット数を示します。ドープシートとカーブエディターはタイムラインで開きます。",
        "En la pestaña que elige el área (**{{MfBewegung}}** como 3ds Max, **{{MfKnochen}}** como Blender) todos los grupos aparecen uno debajo de otro: **{{AzRAnimation}}**, **{{AzRRig}}**, **{{PrRollout}}**, "
        "**{{AzRBearbeiten}}**, **{{AzRUebertragen}}**, **{{MbIkRollout}}**, **{{AzRAnbauten}}**, **{{AzRBauen}}** y, detrás, **{{PhRollout}}**, **{{MbRgGruppePose}}**, **{{MbRgGhosts}}**, **{{MbRgPfade}}**, "
        "**{{MbKvRollout}}**, **{{MbKvGruppeBibliothek}}**, **{{MbRGla}}**, **{{MbRCfg}}** y **{{MbRKartenGla}}**. Un clic en un encabezado pliega el grupo. Las demás pestañas muestran su parte: **{{MfHierarchie}}** "
        "el rig, IK/FK y los acoples; **{{MfDienste}}** la creación para el juego, la biblioteca de poses, GLA, animation.cfg y los huecos (como Blender: **{{MfConstraints}}** y **{{MfDaten}}**). "
        "**{{AzRAnimation}}** indica la secuencia en la que se trabaja, el GLA de destino y cuántos de sus huecos están ocupados. La hoja de claves y el editor de curvas se abren en la línea de tiempo.",
        "Na kartě, kterou oblast určuje (**{{MfBewegung}}** jako 3ds Max, **{{MfKnochen}}** jako Blender), jsou všechny skupiny pod sebou: **{{AzRAnimation}}**, **{{AzRRig}}**, **{{PrRollout}}**, **{{AzRBearbeiten}}**, "
        "**{{AzRUebertragen}}**, **{{MbIkRollout}}**, **{{AzRAnbauten}}**, **{{AzRBauen}}** a za nimi **{{PhRollout}}**, **{{MbRgGruppePose}}**, **{{MbRgGhosts}}**, **{{MbRgPfade}}**, **{{MbKvRollout}}**, "
        "**{{MbKvGruppeBibliothek}}**, **{{MbRGla}}**, **{{MbRCfg}}** a **{{MbRKartenGla}}**. Kliknutím na nadpis se skupina sbalí. Ostatní karty ukazují svou část: **{{MfHierarchie}}** rig, IK/FK a připojené modely, "
        "**{{MfDienste}}** sestavení pro hru, knihovnu póz, GLA, animation.cfg a sloty (jako Blender: **{{MfConstraints}}** a **{{MfDaten}}**). **{{AzRAnimation}}** uvádí sekvenci, na které pracujete, cílové GLA "
        "a kolik jeho slotů je obsazeno. Dopesheet a editor křivek se otevírají v časové ose."),
    "h_rig": ("Create rig: templates", "Rig erstellen: Vorlagen", "创建绑定: 模板", "リグを作成: テンプレート", "Crear rig: plantillas", "Vytvořit rig: šablony"),
    "rig": (
        "The table shows the skeleton, its scale (fScale) and the template it was recognised as. There are templates for the skeletons of the base game - human, rancor, wampa, tauntaun, howler, AT-ST, droids, "
        "vehicles and ships - and for rigs of other programs. The choice below sets another template or **{{AzVorEigen}}** (the chains are then suggested and can be changed in the **{{MbRgUebersicht}}**). "
        "The template also names the chains of the target when you transfer an animation. **{{AzRigErstellen}}** builds the rig with IK targets, poles and foot rolls; **{{AzRigFk}}** builds one with rotation "
        "controls only. Like a CAT rig, **{{AzRigErstellen}}** also puts an animation of the sequence onto the rig from FK to IK: the frames become keys on the controls, hands and feet follow their IK targets and poles, "
        "the motion looks the same (the message names the largest deviation). Then you work on the controls - move the hand or foot target, the pole (elbow, knee) or roll the foot - at any frame; "
        "**{{AzAufRig}}** does the same for an animation loaded later.",
        "Die Tabelle zeigt das Skelett, seinen Maßstab (fScale) und die Vorlage, als die es erkannt wurde. Vorlagen gibt es für die Skelette des Basisspiels - Mensch, Rancor, Wampa, Tauntaun, Howler, AT-ST, Droiden, "
        "Fahrzeuge und Schiffe - und für Rigs anderer Programme. Die Auswahl darunter stellt eine andere Vorlage ein oder **{{AzVorEigen}}** (die Ketten werden dann vorgeschlagen und lassen sich in der "
        "**{{MbRgUebersicht}}** ändern). Die Vorlage bestimmt auch die Ketten des Ziels beim Übertragen einer Animation. **{{AzRigErstellen}}** baut das Rig mit IK-Zielen, Polen und Fußrollen; **{{AzRigFk}}** baut "
        "eines nur mit Dreh-Griffen. Wie bei einem CAT-Rig legt **{{AzRigErstellen}}** dabei eine Animation der Sequenz von FK auf IK auf das Rig: aus den Frames werden Schlüssel an den Griffen, Hände und Füße "
        "folgen ihren IK-Zielen und Polen, die Bewegung sieht gleich aus (die Meldung nennt die größte Abweichung). Danach arbeitet man an den Griffen - Hand- oder Fußziel verschieben, den Pol (Ellbogen, Knie) "
        "oder den Fuß rollen - an jedem Frame; **{{AzAufRig}}** tut dasselbe für eine später geladene Animation.",
        "表格显示骨架、其比例 (fScale) 以及识别出的模板。基础游戏的骨架 (人类、Rancor、Wampa、Tauntaun、Howler、AT-ST、机器人、载具和飞船) 以及其他程序的绑定都有模板。下方的选择可设置其他模板或 "
        "**{{AzVorEigen}}** (此时程序会建议链, 可在 **{{MbRgUebersicht}}** 中修改)。传递动画时, 模板还决定目标的链。**{{AzRigErstellen}}** 创建带 IK 目标、极向和脚部滚动的绑定; **{{AzRigFk}}** 创建只有旋转控制器的绑定。"
        "与 CAT 绑定相同, **{{AzRigErstellen}}** 同时把序列的动画从 FK 转到 IK 放到绑定上: 帧变为控制器上的关键帧, 手脚跟随其 IK 目标和极向, 动作看起来不变 (消息会给出最大偏差)。之后在任意帧上操作控制器 - "
        "移动手或脚的目标、极向 (肘、膝) 或滚动脚部; **{{AzAufRig}}** 对之后加载的动画执行同样的操作。",
        "表にはスケルトン、そのスケール (fScale)、認識されたテンプレートが表示されます。基本ゲームのスケルトン (人型、ランコア、ワンパ、トーントーン、ハウラー、AT-ST、ドロイド、乗り物、船) と他のプログラムのリグ用の"
        "テンプレートがあります。下の選択で別のテンプレートまたは **{{AzVorEigen}}** を設定できます (その場合チェーンは提案され、**{{MbRgUebersicht}}** で変更できます)。テンプレートはアニメーション転送時の"
        "ターゲットのチェーンも決めます。**{{AzRigErstellen}}** は IK ターゲット、ポール、フットロール付きのリグを作り、**{{AzRigFk}}** は回転コントロールだけのリグを作ります。CAT リグと同じく、"
        "**{{AzRigErstellen}}** はシーケンスのアニメーションを FK から IK に移してリグに載せます: フレームはコントロールのキーになり、手足は IK ターゲットとポールに追従し、動きは同じに見えます (メッセージに最大のずれを表示)。"
        "その後、どのフレームでもコントロールを操作します - 手や足のターゲット、ポール (肘、膝) を動かし、足を転がします。**{{AzAufRig}}** は後から読み込んだアニメーションに同じことをします。",
        "La tabla muestra el esqueleto, su escala (fScale) y la plantilla con la que se reconoció. Hay plantillas para los esqueletos del juego base (humano, rancor, wampa, tauntaun, howler, AT-ST, droides, "
        "vehículos y naves) y para rigs de otros programas. La selección de debajo fija otra plantilla o **{{AzVorEigen}}** (entonces las cadenas se sugieren y se pueden cambiar en **{{MbRgUebersicht}}**). "
        "La plantilla también define las cadenas del destino al transferir una animación. **{{AzRigErstellen}}** crea el rig con objetivos IK, polos y giros de pie; **{{AzRigFk}}** crea uno solo con controles de giro. "
        "Como un rig de CAT, **{{AzRigErstellen}}** pone además la animación de la secuencia en el rig de FK a IK: los fotogramas pasan a ser claves de los controles, manos y pies siguen sus objetivos IK y polos "
        "y el movimiento se ve igual (el mensaje da la mayor desviación). Después se trabaja con los controles - mover el objetivo de la mano o del pie, el polo (codo, rodilla) o girar el pie - en cualquier "
        "fotograma; **{{AzAufRig}}** hace lo mismo con una animación cargada después.",
        "Tabulka ukazuje kostru, její měřítko (fScale) a šablonu, jako která byla rozpoznána. Šablony existují pro kostry základní hry - člověk, rancor, wampa, tauntaun, howler, AT-ST, droidi, vozidla a lodě - "
        "a pro rigy jiných programů. Výběr pod tabulkou nastaví jinou šablonu nebo **{{AzVorEigen}}** (řetězce se pak navrhnou a lze je změnit v okně **{{MbRgUebersicht}}**). Šablona určuje i řetězce cíle při "
        "přenosu animace. **{{AzRigErstellen}}** vytvoří rig s cíli IK, póly a odvalem chodidla; **{{AzRigFk}}** vytvoří rig jen s ovladači otáčení. Jako rig CAT položí **{{AzRigErstellen}}** zároveň animaci "
        "sekvence na rig z FK na IK: ze snímků se stanou klíče ovladačů, ruce a nohy sledují své cíle IK a póly a pohyb vypadá stejně (zpráva uvede největší odchylku). Pak se pracuje s ovladači - posunout cíl "
        "ruky nebo nohy, pól (loket, koleno) nebo odvalit chodidlo - na kterémkoli snímku; **{{AzAufRig}}** udělá totéž s animací načtenou později."),
    "h_presets": ("Presets and existing animations", "Presets und vorhandene Animationen", "预设和现有动画", "プリセットと既存のアニメーション", "Preajustes y animaciones existentes",
                  "Předvolby a existující animace"),
    "presets": (
        "**{{PrRollout}}**: the eight categories of the design ({{AiKatStehen}}, {{AzKatGehen}}, {{AzKatLaufen}}, {{AzKatRueck}}, {{AzKatDucken}}, {{AzKatSpringen}}, {{AiKatKampf}}, {{AzKatGesten}}) always stand there, "
        "behind them **{{AiKatSonst}}**; a category without presets yet shows a short note. Choose a preset in the list - source and licence stand next to it. In {{AzKatGehen}} and {{AzKatLaufen}} the list also has "
        "**{{EnGangGehen}}** and **{{EnGangLaufen}}** from the own generator: chosen, **{{PrTempo}}** and **{{PrArm}}** stand under the list, **{{AzGenerator}}** shows the rest (posture, step length, hip swing, "
        "bounce, measuring). **{{AzPresetVorschau}}** loads or generates it, transfers it to the skeleton of the figure and plays it; **{{AzPresetAuflegen}}** puts it on the **{{AzZielPlatz}}**; right-click on an "
        "entry offers the same. "
        "**{{AzRBearbeiten}}**: **{{MbKvReduzieren}}** turns the frames of the sequence into keys (the options limit it to the selected bones or a span), **{{MbRgBacken}}** writes the edited sequence back as frames "
        "of a GLA onto a slot of the target. **{{AzRUebertragen}}**: choose the source or import an animation, check the chains in the table, then the main button puts the result on the slot; "
        "**{{AzFussrutschen}}** shows how far the feet slide.",
        "**{{PrRollout}}**: die acht Kategorien des Entwurfs ({{AiKatStehen}}, {{AzKatGehen}}, {{AzKatLaufen}}, {{AzKatRueck}}, {{AzKatDucken}}, {{AzKatSpringen}}, {{AiKatKampf}}, {{AzKatGesten}}) stehen immer da, "
        "dahinter **{{AiKatSonst}}**; eine Kategorie ohne Presets zeigt einen kurzen Hinweis. Ein Preset in der Liste wählen - Quelle und Lizenz stehen daneben. In {{AzKatGehen}} und {{AzKatLaufen}} steht dazu "
        "**{{EnGangGehen}}** bzw. **{{EnGangLaufen}}** aus dem eigenen Generator: gewählt stehen **{{PrTempo}}** und **{{PrArm}}** unter der Liste, **{{AzGenerator}}** zeigt den Rest (Haltung, Schrittlänge, "
        "Hüftschwung, Hub, Messen). **{{AzPresetVorschau}}** lädt bzw. erzeugt es, überträgt es auf das Skelett der Figur und spielt es ab; **{{AzPresetAuflegen}}** legt es auf den **{{AzZielPlatz}}**; der "
        "Rechtsklick auf einen Eintrag bietet dasselbe. "
        "**{{AzRBearbeiten}}**: **{{MbKvReduzieren}}** macht aus den Frames der Sequenz Schlüssel (die Optionen begrenzen es auf die gewählten Knochen oder eine Spanne), **{{MbRgBacken}}** schreibt die bearbeitete "
        "Sequenz als Frames einer GLA zurück auf einen Platz des Ziels. **{{AzRUebertragen}}**: die Quelle wählen oder eine Animation importieren, die Ketten in der Tabelle prüfen, dann legt der Hauptknopf das "
        "Ergebnis auf den Platz; **{{AzFussrutschen}}** zeigt, wie weit die Füße rutschen.",
        "**{{PrRollout}}**: 设计中的八个类别 ({{AiKatStehen}}、{{AzKatGehen}}、{{AzKatLaufen}}、{{AzKatRueck}}、{{AzKatDucken}}、{{AzKatSpringen}}、{{AiKatKampf}}、{{AzKatGesten}}) 始终显示, 其后是 **{{AiKatSonst}}**; "
        "还没有预设的类别会显示简短提示。在列表中选择预设 - 旁边显示来源和许可。{{AzKatGehen}} 和 {{AzKatLaufen}} 中还有自带生成器的 **{{EnGangGehen}}** 或 **{{EnGangLaufen}}**: 选中后列表下方显示 **{{PrTempo}}** 和 "
        "**{{PrArm}}**, **{{AzGenerator}}** 显示其余设置 (姿态、步长、摆胯、起伏、测量)。**{{AzPresetVorschau}}** 加载或生成, 传递到角色的骨架并播放; **{{AzPresetAuflegen}}** 将其放到 **{{AzZielPlatz}}**; 右键单击条目也提供这些命令。**{{AzRBearbeiten}}**: **{{MbKvReduzieren}}** 把序列的帧变为关键帧 (选项可将其限制为所选骨骼或某个范围), **{{MbRgBacken}}** 把编辑后的序列作为 GLA 的帧写回目标的槽位。"
        "**{{AzRUebertragen}}**: 选择来源或导入动画, 在表格中检查链, 然后主按钮把结果放到槽位; **{{AzFussrutschen}}** 显示脚滑动的距离。",
        "**{{PrRollout}}**: デザインの 8 つのカテゴリ ({{AiKatStehen}}、{{AzKatGehen}}、{{AzKatLaufen}}、{{AzKatRueck}}、{{AzKatDucken}}、{{AzKatSpringen}}、{{AiKatKampf}}、{{AzKatGesten}}) は常に表示され、その後ろに "
        "**{{AiKatSonst}}** があります。まだプリセットのないカテゴリには短い案内が出ます。一覧からプリセットを選びます - 出典とライセンスが横に表示されます。{{AzKatGehen}} と {{AzKatLaufen}} には独自ジェネレーターの "
        "**{{EnGangGehen}}** / **{{EnGangLaufen}}** もあります: 選ぶと一覧の下に **{{PrTempo}}** と **{{PrArm}}** が出て、**{{AzGenerator}}** でその他 (姿勢、歩幅、腰の振り、上下動、計測) を表示します。"
        "**{{AzPresetVorschau}}** は読み込みまたは生成してフィギュアのスケルトンに転送し再生します。**{{AzPresetAuflegen}}** は **{{AzZielPlatz}}** に置きます。項目の右クリックでも同じ操作ができます。**{{AzRBearbeiten}}**: **{{MbKvReduzieren}}** はシーケンスのフレームをキーに変え "
        "(オプションで選択ボーンや範囲に限定)、**{{MbRgBacken}}** は編集したシーケンスを GLA のフレームとして出力先のスロットに書き戻します。**{{AzRUebertragen}}**: ソースを選ぶかアニメーションをインポートし、"
        "表でチェーンを確認してからメインボタンで結果をスロットに置きます。**{{AzFussrutschen}}** は足がどれだけ滑るかを示します。",
        "**{{PrRollout}}**: las ocho categorías del diseño ({{AiKatStehen}}, {{AzKatGehen}}, {{AzKatLaufen}}, {{AzKatRueck}}, {{AzKatDucken}}, {{AzKatSpringen}}, {{AiKatKampf}}, {{AzKatGesten}}) están siempre, "
        "detrás **{{AiKatSonst}}**; una categoría sin preajustes muestra una nota breve. Elige un preajuste de la lista; al lado figuran el origen y la licencia. En {{AzKatGehen}} y {{AzKatLaufen}} la lista tiene "
        "además **{{EnGangGehen}}** o **{{EnGangLaufen}}** del generador propio: al elegirlo, **{{PrTempo}}** y **{{PrArm}}** aparecen bajo la lista y **{{AzGenerator}}** muestra el resto (postura, longitud de "
        "paso, balanceo de cadera, rebote, medir). **{{AzPresetVorschau}}** lo carga o lo genera, lo transfiere al esqueleto de la figura y lo reproduce; **{{AzPresetAuflegen}}** lo pone en el **{{AzZielPlatz}}**; "
        "el clic derecho en una entrada ofrece lo mismo. **{{AzRBearbeiten}}**: **{{MbKvReduzieren}}** convierte los fotogramas de "
        "la secuencia en claves (las opciones lo limitan a los huesos elegidos o a un tramo); **{{MbRgBacken}}** escribe la secuencia editada como fotogramas de un GLA en un hueco del destino. "
        "**{{AzRUebertragen}}**: elige el origen o importa una animación, comprueba las cadenas en la tabla y el botón principal pone el resultado en el hueco; **{{AzFussrutschen}}** muestra cuánto resbalan los pies.",
        "**{{PrRollout}}**: osm kategorií návrhu ({{AiKatStehen}}, {{AzKatGehen}}, {{AzKatLaufen}}, {{AzKatRueck}}, {{AzKatDucken}}, {{AzKatSpringen}}, {{AiKatKampf}}, {{AzKatGesten}}) je vždy vidět, za nimi "
        "**{{AiKatSonst}}**; kategorie zatím bez předvoleb ukáže krátkou poznámku. Zvolte předvolbu v seznamu - vedle je uveden zdroj a licence. V {{AzKatGehen}} a {{AzKatLaufen}} je v seznamu navíc "
        "**{{EnGangGehen}}** nebo **{{EnGangLaufen}}** z vlastního generátoru: po zvolení jsou pod seznamem **{{PrTempo}}** a **{{PrArm}}**, **{{AzGenerator}}** ukáže zbytek (postoj, délka kroku, švih boků, "
        "pohupování, měření). **{{AzPresetVorschau}}** ji načte nebo vygeneruje, přenese na kostru postavy a přehraje; **{{AzPresetAuflegen}}** ji položí na **{{AzZielPlatz}}**; pravé tlačítko na položce nabídne totéž. **{{AzRBearbeiten}}**: **{{MbKvReduzieren}}** udělá ze snímků sekvence klíče (možnosti to omezí na zvolené kosti nebo "
        "rozsah), **{{MbRgBacken}}** zapíše upravenou sekvenci jako snímky GLA zpět na slot cíle. **{{AzRUebertragen}}**: zvolte zdroj nebo importujte animaci, zkontrolujte řetězce v tabulce a hlavní tlačítko "
        "položí výsledek na slot; **{{AzFussrutschen}}** ukazuje, jak moc chodidla kloužou."),
    "h_ikfk": ("IK/FK, attachments, building", "IK/FK, Anbauten, Bauen", "IK/FK、附加物、生成", "IK/FK、取り付け、作成", "IK/FK, acoples, creación", "IK/FK, připojené modely, sestavení"),
    "ikfk": (
        "**{{MbIkRollout}}** has one line per limb. Like 3ds Max the button on the right shows the state and changes it (grab a hand or a foot = IK, rotate a bone = FK) - like the IK/FK of a CAT limb from the current frame up to the next switch, and it "
        "first matches the other side (Match IK and FK), so the pose does not jump; like Blender there is a slider from IK to FK "
        "and the two buttons that snap without a jump. Below it you set the space, the foot roll and the keys of the chosen limb. **{{AzRAnbauten}}** lists what hangs on the figure and what it plays: the attached "
        "models, the primary and the secondary sequence. **{{AzRBauen}}** shows the **{{AzWurzelweg}}** - how far the pelvis travels over the floor in the sequence - and builds the target GLA with its animation.cfg. "
        "The game plays this movement but does not move the origin of the figure: a script has to set SET_ORIGIN together with the follow-up animation.",
        "**{{MbIkRollout}}** hat eine Zeile je Glied. Wie 3ds Max zeigt der Knopf rechts den Zustand und wechselt ihn (Hand oder Fuß greifen = IK, Knochen drehen = FK) - wie IK/FK eines CAT-Glieds ab dem aktuellen Frame bis zum nächsten Wechsel, "
        "vorher gleicht er die andere Seite an (Match IK and FK), die Pose springt nicht; wie Blender gibt es einen Regler von IK nach FK "
        "und die zwei Knöpfe, die ohne Sprung einrasten. Darunter stellt man Raum, Fußrolle und Schlüssel des gewählten Glieds ein. **{{AzRAnbauten}}** nennt, was an der Figur hängt und was sie spielt: die angebauten "
        "Modelle, die Primär- und die Sekundärsequenz. **{{AzRBauen}}** zeigt den **{{AzWurzelweg}}** - wie weit das Becken in der Sequenz über den Boden wandert - und baut die Ziel-GLA mit ihrer animation.cfg. "
        "Das Spiel spielt diese Bewegung, verschiebt aber den Ursprung der Figur nicht: ein Skript muss SET_ORIGIN zusammen mit der Folgeanimation setzen.",
        "**{{MbIkRollout}}** 每个肢体一行。像 3ds Max 时, 右侧的按钮显示状态并切换 (抓住手或脚 = IK, 旋转骨骼 = FK) - 与 CAT 肢体的 IK/FK 相同, 从当前帧起直到下一次切换, 并先匹配另一侧 (Match IK and FK), 姿势不会跳变; 像 Blender 时有一个从 IK 到 FK 的滑块和两个无跳变捕捉的按钮。下方可设置所选肢体的空间、脚部滚动和关键帧。"
        "**{{AzRAnbauten}}** 列出角色上挂接的内容及其播放的内容: 挂接的模型、主要序列和次要序列。**{{AzRBauen}}** 显示 **{{AzWurzelweg}}** (序列中骨盆在地面上移动的距离), 并生成带 animation.cfg 的目标 GLA。"
        "游戏会播放这段移动, 但不会移动角色的原点: 脚本必须同时设置 SET_ORIGIN 和后续动画。",
        "**{{MbIkRollout}}** は手足ごとに 1 行です。3ds Max 風では右のボタンが状態を示して切り替えます (手や足を掴む = IK、ボーンを回す = FK)。CAT の手足の IK/FK と同じく現在のフレームから次の切り替えまでで、先にもう一方を合わせる (Match IK and FK) ので、ポーズはジャンプしません。Blender 風では IK から FK へのスライダーと、ジャンプなしでスナップする"
        " 2 つのボタンがあります。その下で、選んだ手足のスペース、フットロール、キーを設定します。**{{AzRAnbauten}}** はフィギュアに付いているものと再生しているものを示します: 取り付けたモデル、プライマリとセカンダリのシーケンス。"
        "**{{AzRBauen}}** は **{{AzWurzelweg}}** (シーケンスで骨盤が床の上を移動する距離) を表示し、animation.cfg 付きの出力先 GLA を作成します。ゲームはこの動きを再生しますが、フィギュアの原点は動かしません: "
        "スクリプトで SET_ORIGIN と次のアニメーションを一緒に設定する必要があります。",
        "**{{MbIkRollout}}** tiene una línea por extremidad. Como 3ds Max, el botón de la derecha muestra el estado y lo cambia (agarrar una mano o un pie = IK, girar un hueso = FK): como el IK/FK de una extremidad de CAT, desde el fotograma actual hasta el siguiente "
        "cambio, e iguala antes el otro lado (Match IK and FK), así la pose no salta; como Blender hay un regulador de IK "
        "a FK y los dos botones que ajustan sin salto. Debajo se fijan el espacio, el giro del pie y las claves de la extremidad elegida. **{{AzRAnbauten}}** indica qué cuelga de la figura y qué reproduce: los modelos "
        "acoplados y las secuencias primaria y secundaria. **{{AzRBauen}}** muestra el **{{AzWurzelweg}}** (cuánto recorre la pelvis sobre el suelo en la secuencia) y crea el GLA de destino con su animation.cfg. "
        "El juego reproduce este movimiento, pero no mueve el origen de la figura: un script debe poner SET_ORIGIN junto con la animación siguiente.",
        "**{{MbIkRollout}}** má jeden řádek na končetinu. Jako 3ds Max ukazuje tlačítko vpravo stav a přepíná ho (uchopit ruku nebo nohu = IK, otočit kost = FK) - jako IK/FK končetiny CAT od aktuálního snímku do další změny a předtím srovná druhou stranu "
        "(Match IK and FK), takže póza neskočí; jako Blender je tu posuvník od IK k FK a dvě tlačítka, "
        "která přichytí bez skoku. Pod tím se nastavuje prostor, odval chodidla a klíče zvolené končetiny. **{{AzRAnbauten}}** uvádí, co na postavě visí a co hraje: připojené modely, primární a sekundární sekvenci. "
        "**{{AzRBauen}}** ukazuje **{{AzWurzelweg}}** - jak daleko se pánev v sekvenci posune po podlaze - a sestaví cílové GLA s jeho animation.cfg. Hra tento pohyb přehraje, ale počátek postavy neposune: "
        "skript musí nastavit SET_ORIGIN spolu s navazující animací."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    z = X[schluessel]
    return T(z[0], z[1], z[2], z[3])


def seite():
    """Die Seite modell-animationsfeld (Gruppe Missionen, nach modell-zeitleiste bzw. modell-bedienung)."""
    return {
        "id": "modell-animationsfeld",
        "gruppe": "mission",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("img", "h_ap10_max", _t("bild_max"), "breit"),
            ("h2", "gruppen", _t("h_gruppen")),
            ("p", _t("gruppen")),
            ("img", "h_ap10_blender", _t("bild_blender"), "breit"),
            ("h2", "rig", _t("h_rig")),
            ("p", _t("rig")),
            ("h2", "presets", _t("h_presets")),
            ("p", _t("presets")),
            ("h2", "ikfk", _t("h_ikfk")),
            ("p", _t("ikfk")),
        ],
    }
