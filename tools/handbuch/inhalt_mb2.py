"""Modell-Block Stufe 2: die Seite "Model-Tab: Speichern, GLA und Karten-GLA" (GLM speichern, GLA/animation.cfg mit
Pruefungen, Marker in der Zeitleiste, Karten-GLA mit Plaetzen BOTH_CIN, XSI aufnehmen).

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_mb1a2.py.
"""

X = {
    "titel": ("Model tab: saving, GLA and map GLA", "Model-Tab: Speichern, GLA und Karten-GLA", "模型选项卡:保存、GLA 和地图 GLA",
              "モデルタブ: 保存、GLA とマップ GLA", "Pestaña Model: guardar, GLA y GLA de mapa", "Karta Model: ukládání, GLA a mapové GLA"),
    "kurz": (
        "Save the model as .glm, the skeleton as .gla and its animation.cfg, edit the sequences in the timeline and build a map GLA for cutscenes.",
        "Das Modell als .glm, das Skelett als .gla und seine animation.cfg speichern, die Sequenzen in der Zeitleiste bearbeiten und eine Karten-GLA für Zwischensequenzen bauen.",
        "将模型保存为 .glm、骨架保存为 .gla 及其 animation.cfg,在时间轴中编辑序列,并为过场动画生成地图 GLA。",
        "モデルを .glm、スケルトンを .gla とその animation.cfg として保存し、タイムラインでシーケンスを編集し、カットシーン用のマップ GLA を作ります。",
        "Guarda el modelo como .glm, el esqueleto como .gla y su animation.cfg, edita las secuencias en la línea de tiempo y crea un GLA de mapa para cinemáticas.",
        "Uložte model jako .glm, kostru jako .gla a její animation.cfg, upravujte sekvence na časové ose a sestavte mapové GLA pro filmové scény."),
    "h_speichern": ("Saving the model", "Das Modell speichern", "保存模型", "モデルの保存", "Guardar el modelo", "Uložení modelu"),
    "speichern": (
        "**{{MbDateiSpeichern}}** (Ctrl+S) writes the model as .glm with every level of detail, the hierarchy, tags, weights, flags and shaders. A model "
        "from a file is written back there, a model from the archives asks for a new file (**{{MbDateiSpeichernUnter}}**, Ctrl+Shift+S). An unchanged model "
        "becomes the same file byte for byte - every .glm of Jedi Academy, Jedi Outcast and Movie Duels was checked. Changed weights are written the way the "
        "game reads them. Before writing, the limits of the target game are checked (vertices and triangles per surface, 32 bones per surface, 4 weights per "
        "vertex); above a limit JK Studio asks in its own window, and **{{MbSpTrotzdem}}** still writes the file.",
        "**{{MbDateiSpeichern}}** (Strg+S) schreibt das Modell als .glm mit allen Detailstufen, der Hierarchie, Tags, Gewichten, Flaggen und Shadern. Ein Modell "
        "aus einer Datei wird dorthin zurückgeschrieben, ein Modell aus den Archiven fragt nach einer neuen Datei (**{{MbDateiSpeichernUnter}}**, "
        "Strg+Umschalt+S). Ein unverändertes Modell wird Byte für Byte dieselbe Datei - geprüft an jeder .glm von Jedi Academy, Jedi Outcast und Movie Duels. "
        "Geänderte Gewichte werden so geschrieben, wie das Spiel sie liest. Vor dem Schreiben prüft JK Studio die Grenzen des Zielspiels (Ecken und Dreiecke "
        "je Fläche, 32 Knochen je Fläche, 4 Gewichte je Ecke); darüber fragt es in einem eigenen Fenster, und **{{MbSpTrotzdem}}** schreibt die Datei trotzdem.",
        "**{{MbDateiSpeichern}}** (Ctrl+S) 将模型写为 .glm,包含所有细节级别、层次、标签、权重、标志和着色器。来自文件的模型写回原处,来自归档的模型会要求新文件 "
        "(**{{MbDateiSpeichernUnter}}**,Ctrl+Shift+S)。未修改的模型会逐字节成为同一文件——已在 Jedi Academy、Jedi Outcast 和 Movie Duels 的每个 .glm 上检查。"
        "更改后的权重按游戏读取的方式写出。写入前会检查目标游戏的限制 (每个面的顶点和三角形、每个面 32 根骨骼、每个顶点 4 个权重);超出时 JK Studio 在自己的窗口中询问,"
        "**{{MbSpTrotzdem}}** 仍会写出文件。",
        "**{{MbDateiSpeichern}}** (Ctrl+S) はモデルを .glm として、すべての詳細レベル、階層、タグ、ウェイト、フラグ、シェーダー付きで書き出します。ファイルから開いた"
        "モデルはそこへ書き戻し、アーカイブのモデルは新しいファイルを尋ねます (**{{MbDateiSpeichernUnter}}**、Ctrl+Shift+S)。変更していないモデルはバイト単位で"
        "同じファイルになります - Jedi Academy、Jedi Outcast、Movie Duels のすべての .glm で確認済みです。変えたウェイトはゲームが読む形で書かれます。書き出す前に"
        "対象ゲームの上限 (サーフェスごとの頂点と三角形、サーフェスごとにボーン 32、頂点ごとにウェイト 4) を確認し、超えると JK Studio 自身のウィンドウで尋ねます。"
        "**{{MbSpTrotzdem}}** でもファイルは書き出されます。",
        "**{{MbDateiSpeichern}}** (Ctrl+S) escribe el modelo como .glm con todos los niveles de detalle, la jerarquía, tags, pesos, marcas y shaders. Un modelo "
        "de un archivo se vuelve a escribir allí; uno de los archivos del juego pide un archivo nuevo (**{{MbDateiSpeichernUnter}}**, Ctrl+Mayús+S). Un modelo "
        "sin cambios es el mismo archivo byte a byte; se comprobó con cada .glm de Jedi Academy, Jedi Outcast y Movie Duels. Los pesos cambiados se escriben "
        "como los lee el juego. Antes de escribir se comprueban los límites del juego de destino (vértices y triángulos por superficie, 32 huesos por "
        "superficie, 4 pesos por vértice); por encima de un límite JK Studio pregunta en su propia ventana, y **{{MbSpTrotzdem}}** escribe el archivo igualmente.",
        "**{{MbDateiSpeichern}}** (Ctrl+S) zapíše model jako .glm se všemi úrovněmi detailu, hierarchií, tagy, váhami, příznaky a shadery. Model ze souboru se "
        "zapíše zpět, model z archivů se zeptá na nový soubor (**{{MbDateiSpeichernUnter}}**, Ctrl+Shift+S). Nezměněný model bude bajt po bajtu stejný soubor - "
        "ověřeno na každém .glm z Jedi Academy, Jedi Outcast a Movie Duels. Změněné váhy se zapíší tak, jak je čte hra. Před zápisem se zkontrolují limity "
        "cílové hry (vrcholy a trojúhelníky na povrch, 32 kostí na povrch, 4 váhy na vrchol); nad limitem se JK Studio zeptá ve vlastním okně a "
        "**{{MbSpTrotzdem}}** soubor přesto zapíše."),
    "bild_speichern": (
        "Saving for Jedi Academy single player: the surface hips has 1001 vertices, the game stops at 1000.",
        "Speichern für den Einzelspieler von Jedi Academy: die Fläche hips hat 1001 Ecken, das Spiel bricht bei 1000 ab.",
        "为 Jedi Academy 单人模式保存:面 hips 有 1001 个顶点,游戏在 1000 时中止。",
        "Jedi Academy シングルプレイ向けの保存: サーフェス hips の頂点は 1001、ゲームは 1000 で中断します。",
        "Guardar para el modo un jugador de Jedi Academy: la superficie hips tiene 1001 vértices, el juego se detiene en 1000.",
        "Ukládání pro hru jednoho hráče Jedi Academy: povrch hips má 1001 vrcholů, hra skončí při 1000."),
    "h_gla": ("GLA, animation.cfg and the markers", "GLA, animation.cfg und die Marker", "GLA、animation.cfg 和标记", "GLA、animation.cfg とマーカー",
              "GLA, animation.cfg y los marcadores", "GLA, animation.cfg a značky"),
    "gla": (
        "In the area **{{MbBereichAnimation}}** the command panel shows the GLA of the model (frames, bones, pool) and its animation.cfg. **{{MbGlaSpeichern}}** and **{{MbCfgSpeichern}}** write them; unchanged they are the same files byte for byte. Every line of animation.cfg is a marker in the timeline: the **{{MzZeitKonfig}}** ([[modell-zeitleiste#zeitkonfig]]) holds first frame, count, loopFrames and fps of the selected sequence - change the numbers or drag the ends of the bar, each change is one undo step. The checks list what the target game does with the lines: a sequence behind the end of the GLA (T-pose in Movie Duels and OpenJK, stand pose in the original), frames above 65535, loopFrames outside -1..127, a file too long for the reader (79998 bytes in JKA single player, 59998 in multiplayer, 39998 in Jedi Outcast) and names the game does not know.",
        "Im Bereich **{{MbBereichAnimation}}** zeigt das Befehlsfeld die GLA des Modells (Frames, Knochen, Vorrat) und ihre animation.cfg. **{{MbGlaSpeichern}}** und **{{MbCfgSpeichern}}** schreiben sie; unverändert sind es Byte für Byte dieselben Dateien. Jede Zeile der animation.cfg ist ein Marker in der Zeitleiste: in der **{{MzZeitKonfig}}** ([[modell-zeitleiste#zeitkonfig]]) stehen erster Frame, Anzahl, loopFrames und fps der gewählten Sequenz - Zahlen ändern oder die Enden des Balkens ziehen, jede Änderung ist ein Rückgängig-Schritt. Die Prüfungen nennen, was das Zielspiel mit den Zeilen macht: eine Sequenz hinter dem Ende der GLA (T-Pose in Movie Duels und OpenJK, Standpose im Original), Frames über 65535, loopFrames außerhalb -1..127, eine Datei zu lang für den Leser (79998 Bytes im Einzelspieler von JKA, 59998 im Mehrspieler, 39998 in Jedi Outcast) und Namen, die das Spiel nicht kennt.",
        "在 **{{MbBereichAnimation}}** 区域中,命令面板显示模型的 GLA (帧、骨骼、池) 及其 animation.cfg。**{{MbGlaSpeichern}}** 和 **{{MbCfgSpeichern}}** 写出它们;未修改时逐字节是同样的文件。animation.cfg 的每一行都是时间轴中的一个标记:**{{MzZeitKonfig}}** ([[modell-zeitleiste#zeitkonfig]]) 中是所选序列的第一帧、帧数、loopFrames 和 fps——修改数字或拖动条的两端,每次更改都是一个撤销步骤。检查列出目标游戏如何处理这些行:超出 GLA 末尾的序列 (Movie Duels 和 OpenJK 中为 T 姿势,原版中为站姿)、超过 65535 的帧、-1..127 以外的 loopFrames、对读取器来说太长的文件 (JKA 单人 79998 字节、多人 59998、Jedi Outcast 39998) 以及游戏不认识的名称。",
        "**{{MbBereichAnimation}}** 領域では、コマンドパネルにモデルの GLA (フレーム、ボーン、プール) とその animation.cfg が表示されます。**{{MbGlaSpeichern}}** と **{{MbCfgSpeichern}}** で書き出し、変更していなければバイト単位で同じファイルです。animation.cfg の各行はタイムラインのマーカーです。**{{MzZeitKonfig}}** ([[modell-zeitleiste#zeitkonfig]]) に選択シーケンスの最初のフレーム、フレーム数、loopFrames、fps があり、数値を変えるかバーの端をドラッグします。変更ごとに元に戻す 1 回分です。チェックは対象ゲームが行をどう扱うかを示します: GLA の末尾を超えるシーケンス (Movie Duels と OpenJK では T ポーズ、オリジナルでは立ちポーズ)、65535 を超えるフレーム、-1..127 以外の loopFrames、リーダーには長すぎるファイル (JKA シングル 79998 バイト、マルチ 59998、Jedi Outcast 39998)、ゲームが知らない名前。",
        "En el área **{{MbBereichAnimation}}** el panel de comandos muestra el GLA del modelo (fotogramas, huesos, pool) y su animation.cfg. **{{MbGlaSpeichern}}** y **{{MbCfgSpeichern}}** los escriben; sin cambios son los mismos archivos byte a byte. Cada línea de animation.cfg es un marcador en la línea de tiempo: en la **{{MzZeitKonfig}}** ([[modell-zeitleiste#zeitkonfig]]) están el primer fotograma, el número, loopFrames y fps de la secuencia elegida; cambia los números o arrastra los extremos de la barra, cada cambio es un paso de deshacer. Las comprobaciones dicen qué hace el juego de destino con las líneas: una secuencia más allá del final del GLA (pose T en Movie Duels y OpenJK, pose de pie en el original), fotogramas por encima de 65535, loopFrames fuera de -1..127, un archivo demasiado largo para el lector (79998 bytes en un jugador de JKA, 59998 en multijugador, 39998 en Jedi Outcast) y nombres que el juego no conoce.",
        "V oblasti **{{MbBereichAnimation}}** panel příkazů ukáže GLA modelu (snímky, kosti, zásobu) a jeho animation.cfg. **{{MbGlaSpeichern}}** a **{{MbCfgSpeichern}}** je zapíší; beze změny jsou to bajt po bajtu stejné soubory. Každý řádek animation.cfg je značka na časové ose: v okně **{{MzZeitKonfig}}** ([[modell-zeitleiste#zeitkonfig]]) je první snímek, počet, loopFrames a fps vybrané sekvence - změňte čísla nebo táhněte konce pruhu, každá změna je jeden krok zpět. Kontroly říkají, co cílová hra s řádky udělá: sekvence za koncem GLA (T-póza v Movie Duels a OpenJK, stoj v originálu), snímky nad 65535, loopFrames mimo -1..127, soubor příliš dlouhý pro čtečku (79998 bajtů v JKA pro jednoho hráče, 59998 ve hře více hráčů, 39998 v Jedi Outcast) a jména, která hra nezná.",
    ),
    "bild_befund": (
        "The last sequence ends ten frames behind the end of the GLA: the group animation.cfg names it; the window Time configuration shows the line.",
        "Die letzte Sequenz endet zehn Frames hinter dem Ende der GLA: die Gruppe animation.cfg nennt sie; das Fenster Zeitkonfiguration zeigt die Zeile.",
        "最后一个序列在 GLA 末尾之后十帧结束:animation.cfg 分组会指出它; 时间配置窗口显示该行。",
        "最後のシーケンスが GLA の末尾から 10 フレーム後ろで終わっています: animation.cfg グループがそれを示します。時間設定ウィンドウにその行が表示されます。",
        "La última secuencia termina diez fotogramas después del final del GLA: el grupo animation.cfg la nombra; la ventana Configuración de tiempo muestra la línea.",
        "Poslední sekvence končí deset snímků za koncem GLA: skupina animation.cfg ji uvede; okno Nastavení času ukazuje řádek."),
    "h_karte": ("Map GLA and slots", "Karten-GLA und Plätze", "地图 GLA 和槽位", "マップ GLA とスロット", "GLA de mapa y huecos", "Mapové GLA a sloty"),
    "karte": (
        "Cutscenes play BOTH_CIN_1 to BOTH_CIN_50 (Movie Duels 125) from the GLA models/players/_humanoid_<map>/, which the single player of Jedi Academy and "
        "Movie Duels loads on that map. Enter the map, drag sequences from the animation list onto the slots or use the right-click "
        "(**{{MbKgAufFreien}}**); **{{MbKgVorhanden}}** takes the slots of an existing map GLA. **{{MbKgBauen}}** puts the frames of all taken slots one after "
        "another into one GLA with 53 bones whose header name is its folder, and writes animation.cfg next to it. The pool entries of the sources are taken "
        "over unchanged, so a sequence looks exactly as in its source. An animation from Softimage, 3ds Max or Blender as dotXSI is recorded with "
        "**{{MbXsiAufnehmen}}** the way Carcass does it and lands on the next free slot. The field **{{MbZiel}}** chooses where the slots go and can be changed at any time: **{{MbZielKarte}}**, **{{MbZielHumanoid}}** (every line of its animation.cfg is a slot, the new frames are appended; only into a mod folder, never into the game folder) or **{{MbZielEigen}}** (the .glm then names it). The checks follow the target.",
        "Zwischensequenzen spielen BOTH_CIN_1 bis BOTH_CIN_50 (Movie Duels 125) aus der GLA models/players/_humanoid_<karte>/, die der Einzelspieler von "
        "Jedi Academy und Movie Duels auf dieser Karte lädt. Karte eintragen, Sequenzen aus der Animationsliste auf die Plätze ziehen oder den Rechtsklick "
        "nehmen (**{{MbKgAufFreien}}**); **{{MbKgVorhanden}}** übernimmt die Plätze einer vorhandenen Karten-GLA. **{{MbKgBauen}}** legt die Frames aller "
        "belegten Plätze hintereinander in eine GLA mit 53 Knochen, deren Name im Kopf ihr Ordner ist, und schreibt die animation.cfg daneben. Die Einträge "
        "des Vorrats werden unverändert übernommen, eine Sequenz sieht also genau aus wie in ihrer Quelle. Eine Animation aus Softimage, 3ds Max oder Blender "
        "als dotXSI nimmt **{{MbXsiAufnehmen}}** auf wie Carcass, sie landet auf dem nächsten freien Platz. Das Feld **{{MbZiel}}** wählt, wohin die Plätze gehen, und lässt sich jederzeit wechseln: **{{MbZielKarte}}**, **{{MbZielHumanoid}}** (jede Zeile ihrer animation.cfg ist ein Platz, die neuen Frames kommen hinten dazu; nur in einen Mod-Ordner, nie ins Spielverzeichnis) oder **{{MbZielEigen}}** (die .glm nennt sie dann). Die Prüfungen richten sich nach dem Ziel.",
        "过场动画从 GLA models/players/_humanoid_<地图>/ 播放 BOTH_CIN_1 到 BOTH_CIN_50 (Movie Duels 为 125),Jedi Academy 和 Movie Duels 的单人模式会在该地图上加载它。"
        "输入地图,把动画列表中的序列拖到槽位上或使用右键 (**{{MbKgAufFreien}}**);**{{MbKgVorhanden}}** 读取现有地图 GLA 的槽位。**{{MbKgBauen}}** 把所有已占用槽位的帧"
        "依次放入一个 53 根骨骼、头部名称为其文件夹的 GLA,并在旁边写出 animation.cfg。来源的池条目原样沿用,因此序列与其来源完全一致。来自 Softimage、3ds Max 或 "
        "Blender 的 dotXSI 动画由 **{{MbXsiAufnehmen}}** 按 Carcass 的方式录入,并放到下一个空闲槽位。 **{{MbZiel}}** 字段选择槽位写到哪里, 并且随时可以更改: **{{MbZielKarte}}**、**{{MbZielHumanoid}}** (其 animation.cfg 的每一行都是槽位, 新帧追加在末尾; 只能放入 Mod 文件夹, 绝不放入游戏目录) 或 **{{MbZielEigen}}** (之后 .glm 会引用它)。检查随目标而定。",
        "カットシーンは GLA models/players/_humanoid_<マップ>/ から BOTH_CIN_1 から BOTH_CIN_50 (Movie Duels は 125) を再生し、Jedi Academy と Movie Duels の"
        "シングルプレイがそのマップで読み込みます。マップを入力し、アニメーション一覧のシーケンスをスロットへドラッグするか右クリック (**{{MbKgAufFreien}}**) を使います。"
        "**{{MbKgVorhanden}}** は既存のマップ GLA のスロットを取り込みます。**{{MbKgBauen}}** は使用中のスロットのフレームを順に、ボーン 53 でヘッダー名がフォルダー名の"
        " 1 つの GLA に並べ、横に animation.cfg を書き出します。ソースのプール項目はそのまま使うので、シーケンスはソースとまったく同じに見えます。Softimage、3ds Max、"
        "Blender の dotXSI アニメーションは **{{MbXsiAufnehmen}}** で Carcass と同じように取り込まれ、次の空きスロットに入ります。 **{{MbZiel}}** 欄でスロットの行き先を選び、いつでも切り替えられます: **{{MbZielKarte}}**、**{{MbZielHumanoid}}** (その animation.cfg の各行がスロットで、新しいフレームは末尾に追加。Mod フォルダーにだけ、ゲームフォルダーには入れません)、**{{MbZielEigen}}** (その後 .glm がそれを参照)。チェックは出力先に合わせて変わります。",
        "Las cinemáticas reproducen BOTH_CIN_1 a BOTH_CIN_50 (Movie Duels 125) del GLA models/players/_humanoid_<mapa>/, que el modo un jugador de Jedi Academy "
        "y Movie Duels carga en ese mapa. Escribe el mapa, arrastra secuencias de la lista de animaciones a los huecos o usa el clic derecho "
        "(**{{MbKgAufFreien}}**); **{{MbKgVorhanden}}** toma los huecos de un GLA de mapa existente. **{{MbKgBauen}}** pone los fotogramas de todos los huecos "
        "ocupados uno tras otro en un GLA de 53 huesos cuyo nombre de cabecera es su carpeta, y escribe animation.cfg al lado. Las entradas del pool de las "
        "fuentes se toman sin cambios, así que una secuencia se ve exactamente como en su fuente. Una animación de Softimage, 3ds Max o Blender en dotXSI se "
        "graba con **{{MbXsiAufnehmen}}** como lo hace Carcass y va al siguiente hueco libre. El campo **{{MbZiel}}** elige adónde van los huecos y se puede cambiar en cualquier momento: **{{MbZielKarte}}**, **{{MbZielHumanoid}}** (cada línea de su animation.cfg es un hueco, los fotogramas nuevos se añaden al final; solo en una carpeta de mod, nunca en la carpeta del juego) o **{{MbZielEigen}}** (el .glm lo nombra después). Las comprobaciones siguen al destino.",
        "Filmové scény přehrávají BOTH_CIN_1 až BOTH_CIN_50 (Movie Duels 125) z GLA models/players/_humanoid_<mapa>/, které hra jednoho hráče Jedi Academy a "
        "Movie Duels na té mapě načte. Zadejte mapu, přetáhněte sekvence ze seznamu animací na sloty nebo použijte pravé tlačítko (**{{MbKgAufFreien}}**); "
        "**{{MbKgVorhanden}}** převezme sloty existujícího mapového GLA. **{{MbKgBauen}}** uloží snímky všech obsazených slotů za sebou do jednoho GLA s 53 kostmi, "
        "jehož jméno v hlavičce je jeho složka, a vedle zapíše animation.cfg. Položky zásoby zdrojů se převezmou beze změny, takže sekvence vypadá přesně jako "
        "ve svém zdroji. Animaci ze Softimage, 3ds Max nebo Blenderu jako dotXSI zaznamená **{{MbXsiAufnehmen}}** tak jako Carcass a skončí na dalším volném slotu. Pole **{{MbZiel}}** volí, kam sloty půjdou, a lze ho kdykoli změnit: **{{MbZielKarte}}**, **{{MbZielHumanoid}}** (každý řádek jeho animation.cfg je slot, nové snímky se připojí na konec; jen do složky modu, nikdy do složky hry) nebo **{{MbZielEigen}}** (.glm ho pak uvede). Kontroly se řídí cílem."),
    "bild_karte": (
        "The area Animation: GLA, animation.cfg without findings and the slots of the map GLA mb3test - BOTH_RUN1 on BOTH_CIN_1, BOTH_WALK1 on BOTH_CIN_3.",
        "Der Bereich Animation: GLA, animation.cfg ohne Befund und die Plätze der Karten-GLA mb3test - BOTH_RUN1 auf BOTH_CIN_1, BOTH_WALK1 auf BOTH_CIN_3.",
        "动画区域:GLA、无问题的 animation.cfg 以及地图 GLA mb3test 的槽位——BOTH_RUN1 在 BOTH_CIN_1,BOTH_WALK1 在 BOTH_CIN_3。",
        "アニメーション領域: GLA、問題のない animation.cfg、マップ GLA mb3test のスロット - BOTH_CIN_1 に BOTH_RUN1、BOTH_CIN_3 に BOTH_WALK1。",
        "El área Animación: GLA, animation.cfg sin problemas y los huecos del GLA de mapa mb3test: BOTH_RUN1 en BOTH_CIN_1, BOTH_WALK1 en BOTH_CIN_3.",
        "Oblast Animace: GLA, animation.cfg bez nálezů a sloty mapového GLA mb3test - BOTH_RUN1 na BOTH_CIN_1, BOTH_WALK1 na BOTH_CIN_3."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def seite():
    """Die Seite modell-dateien (Gruppe Missionen, nach modell-bedienung)."""
    return {
        "id": "modell-dateien",
        "gruppe": "mission",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("img", "h_mb3_animation", _t("bild_karte"), "breit"),
            ("h2", "speichern", _t("h_speichern")),
            ("p", _t("speichern")),
            ("img", "h_mb3_speichern", _t("bild_speichern")),
            ("h2", "gla", _t("h_gla")),
            ("p", _t("gla")),
            ("img", "h_mb3_befund", _t("bild_befund"), "breit"),
            ("h2", "karte", _t("h_karte")),
            ("p", _t("karte")),
        ],
    }
