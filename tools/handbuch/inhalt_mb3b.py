"""Modell-Block Stufe 3b: die Seite "Model-Tab: Dopesheet, Kurveneditor und Animationswerkzeuge" (Rauten je Schluessel,
Kurven je Kanal mit Interpolation und Tangenten, Schieber, Timing, Werte, Anim Offset, Spiegeln, Posen-Bibliothek,
Marker, Aufraeumen und Reduzieren, Bewegungspfade ziehen).

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_mb3.py.
"""

X = {
    "titel": ("Model tab: dope sheet, curve editor and animation tools", "Model-Tab: Dopesheet, Kurveneditor und Animationswerkzeuge",
              "模型选项卡: 摄影表、曲线编辑器和动画工具", "モデルタブ: ドープシート、カーブエディター、アニメーションツール",
              "Pestaña Model: hoja de claves, editor de curvas y herramientas de animación", "Karta Model: dopesheet, editor křivek a nástroje animace"),
    "kurz": (
        "Edit the keys of the control rig in time and as curves, blend and ease them, change timing and values, mirror whole sequences, keep poses in a library, reduce baked frames to few keys and drag the pelvis path in the view.",
        "Die Schlüssel des Control Rigs in der Zeit und als Kurven bearbeiten, mischen und ausgleichen, Timing und Werte ändern, ganze Sequenzen spiegeln, Posen in einer Bibliothek ablegen, gebackene Frames zu wenigen Schlüsseln reduzieren und den Beckenpfad in der Ansicht ziehen.",
        "在时间上和曲线上编辑控制绑定的关键帧, 混合和缓动, 修改时间和数值, 镜像整个序列, 把姿势存入库, 将烘焙帧精简为少量关键帧, 并在视图中拖动骨盆路径。",
        "コントロールリグのキーを時間とカーブで編集し、ブレンドやイーズ、タイミングと値の変更、シーケンス全体のミラー、ポーズのライブラリ保存、ベイク済みフレームの少数キーへの削減、ビューでの骨盤パスのドラッグを行います。",
        "Edita las claves del rig de control en el tiempo y como curvas, mézclalas y suavízalas, cambia tiempos y valores, refleja secuencias enteras, guarda poses en una biblioteca, reduce los fotogramas horneados a pocas claves y arrastra la trayectoria de la pelvis en la vista.",
        "Upravujte klíče ovládacího rigu v čase i jako křivky, míchejte je a vyhlazujte, měňte časování a hodnoty, zrcadlete celé sekvence, ukládejte pózy do knihovny, redukujte upečené snímky na několik klíčů a táhněte dráhu pánve v pohledu."),
    "hinweis_ap1": (
        "Since rc612 the dope sheet and the curve editor are no longer a window of their own below the views: they are the tracks of the timeline ([[modell-zeitleiste]]). **{{MzSpuren}}** or the handle above the timeline opens them, the button next to it switches between the two, and their menus are in the **{{MzMenue}}**. In the controls **{{MbWieMax}}** keys are coloured bars, in **{{MbWieBlender}}** diamonds.",
        "Seit rc612 sind Dopesheet und Kurveneditor kein eigenes Fenster unter den Ansichten mehr: sie sind die Spuren der Zeitleiste ([[modell-zeitleiste]]). **{{MzSpuren}}** oder der Griff über der Zeitleiste öffnet sie, der Knopf daneben wechselt zwischen beiden, ihre Menüs stehen im **{{MzMenue}}**. Im Bedienmodus **{{MbWieMax}}** sind Schlüssel farbige Striche, in **{{MbWieBlender}}** Rauten.",
        "自 rc612 起, 摄影表和曲线编辑器不再是视图下方的独立窗口: 它们是时间轴的轨道 ([[modell-zeitleiste]])。**{{MzSpuren}}** 或时间轴上方的手柄可打开它们, 旁边的按钮在两者之间切换, 它们的菜单位于 **{{MzMenue}}**。操作方式为 **{{MbWieMax}}** 时关键帧是彩色竖条, 为 **{{MbWieBlender}}** 时是菱形。",
        "rc612 から、ドープシートとカーブエディターはビュー下の独立ウィンドウではなく、タイムラインのトラックです ([[modell-zeitleiste]])。**{{MzSpuren}}** またはタイムライン上のハンドルで開き、隣のボタンで両者を切り替え、メニューは **{{MzMenue}}** にあります。操作モード **{{MbWieMax}}** ではキーは色付きの棒、**{{MbWieBlender}}** ではひし形です。",
        "Desde rc612 la hoja de claves y el editor de curvas ya no son una ventana propia bajo las vistas: son las pistas de la línea de tiempo ([[modell-zeitleiste]]). **{{MzSpuren}}** o el tirador sobre la línea de tiempo las abre, el botón de al lado cambia entre ambas y sus menús están en el **{{MzMenue}}**. Con los controles **{{MbWieMax}}** las claves son barras de color, con **{{MbWieBlender}}** rombos.",
        "Od rc612 už dopesheet a editor křivek nejsou samostatné okno pod pohledy: jsou to stopy časové osy ([[modell-zeitleiste]]). **{{MzSpuren}}** nebo úchyt nad časovou osou je otevře, tlačítko vedle přepíná mezi oběma a jejich nabídky jsou v **{{MzMenue}}**. S ovládáním **{{MbWieMax}}** jsou klíče barevné proužky, s **{{MbWieBlender}}** kosočtverce."),
    "h_editor": ("Dope sheet", "Dopesheet", "摄影表", "ドープシート", "Hoja de claves", "Dopesheet"),
    "editor": (
        "**{{MbKvDsOeffnen}}** (menu **{{MbBereichAnimation}}** > **{{MbKvMenue}}** > **{{MbKvGruppeEditor}}**, the right-click of the view, the search X/F3 or the group **{{MbKvRollout}}**) opens the tracks of the timeline as dope sheet; like 3ds Max it is called **{{MbKvDsMax}}**, like Blender **{{MbKvDsBl}}**. The handle above the timeline sets their height. On the left are the rows: **{{MbKvZusammenfassung}}**, one summary row per limb (click to fold) and one row per bone; on the right every key of the selected sequence is a diamond. Click a diamond to select it (Shift adds), drag a frame around keys to select them, drag selected keys to move them in time - always by whole frames, because the GLA stores whole frames. A click on a summary row selects all keys of that frame in the limb. The light band is the sequence of the animation.cfg line, the orange line the current frame; click the ruler to set it. Wheel zooms, the middle button pans. **{{MbKvNurAnimierte}}** and **{{MbKvNurGewaehlte}}** filter the rows, **{{MbKvAllesZeigen}}** fits the sequence. Like Blender, X deletes, G moves and S scales the selected keys while the mouse is over the editor.",
        "**{{MbKvDsOeffnen}}** (Menü **{{MbBereichAnimation}}** > **{{MbKvMenue}}** > **{{MbKvGruppeEditor}}**, der Rechtsklick der Ansicht, die Suche X/F3 oder die Gruppe **{{MbKvRollout}}**) öffnet die Spuren der Zeitleiste als Dopesheet; wie 3ds Max heißt es **{{MbKvDsMax}}**, wie Blender **{{MbKvDsBl}}**. Der Griff über der Zeitleiste stellt ihre Höhe ein. Links stehen die Zeilen: **{{MbKvZusammenfassung}}**, eine Sammelzeile je Glied (Klick klappt zu) und eine Zeile je Knochen; rechts ist jeder Schlüssel der gewählten Sequenz eine Raute. Eine Raute anklicken wählt sie (Umschalt fügt hinzu), ein Rahmen um Schlüssel wählt sie, gewählte Schlüssel ziehen verschiebt sie in der Zeit - immer um ganze Frames, weil die GLA ganze Frames speichert. Ein Klick auf eine Sammelzeile wählt alle Schlüssel dieses Frames im Glied. Das helle Band ist die Sequenz der animation.cfg-Zeile, die orange Linie der aktuelle Frame; ein Klick ins Lineal setzt ihn. Das Rad zoomt, die mittlere Taste schiebt. **{{MbKvNurAnimierte}}** und **{{MbKvNurGewaehlte}}** filtern die Zeilen, **{{MbKvAllesZeigen}}** passt die Sequenz ein. Wie Blender löscht X, verschiebt G und skaliert S die gewählten Schlüssel, solange die Maus über dem Editor steht.",
        "**{{MbKvDsOeffnen}}** (菜单 **{{MbBereichAnimation}}** > **{{MbKvMenue}}** > **{{MbKvGruppeEditor}}**、视图右键、搜索 X/F3 或分组 **{{MbKvRollout}}**) 把时间轴的轨道作为摄影表打开; 像 3ds Max 时称为 **{{MbKvDsMax}}**, 像 Blender 时称为 **{{MbKvDsBl}}**。时间轴上方的手柄设置轨道的高度。左侧是各行: **{{MbKvZusammenfassung}}**、每个肢体一行汇总 (单击折叠) 以及每根骨骼一行; 右侧所选序列的每个关键帧都是一个菱形。单击菱形选中 (Shift 追加), 围绕关键帧拖框选择, 拖动所选关键帧在时间上移动——始终以整帧移动, 因为 GLA 存储整帧。单击汇总行选中该肢体在该帧的所有关键帧。浅色带是 animation.cfg 行的序列, 橙线是当前帧; 单击标尺设置它。滚轮缩放, 中键平移。**{{MbKvNurAnimierte}}** 和 **{{MbKvNurGewaehlte}}** 过滤各行, **{{MbKvAllesZeigen}}** 适配序列。像 Blender 时, 鼠标在编辑器上方, X 删除、G 移动、S 缩放所选关键帧。",
        "**{{MbKvDsOeffnen}}** (メニュー **{{MbBereichAnimation}}** > **{{MbKvMenue}}** > **{{MbKvGruppeEditor}}**、ビューの右クリック、検索 X/F3、グループ **{{MbKvRollout}}**) でタイムラインのトラックがドープシートとして開きます。3ds Max 風では **{{MbKvDsMax}}**、Blender 風では **{{MbKvDsBl}}** という名前です。タイムラインの上のハンドルでトラックの高さを変えます。左に行が並びます: **{{MbKvZusammenfassung}}**、肢ごとの集約行 (クリックで折りたたみ)、ボーンごとの行。右では選択中のシーケンスの各キーがひし形です。ひし形をクリックで選択 (Shift で追加)、キーを枠で囲んで選択、選択キーをドラッグで時間移動 - GLA は整数フレームを保存するので常に整数フレーム単位です。集約行のクリックはその肢のそのフレームの全キーを選びます。明るい帯は animation.cfg 行のシーケンス、オレンジの線は現在フレームで、定規をクリックすると設定します。ホイールでズーム、中ボタンでパン。**{{MbKvNurAnimierte}}** と **{{MbKvNurGewaehlte}}** で行を絞り、**{{MbKvAllesZeigen}}** でシーケンスに合わせます。Blender 風ではマウスがエディター上にある間、X で削除、G で移動、S でスケールします。",
        "**{{MbKvDsOeffnen}}** (menú **{{MbBereichAnimation}}** > **{{MbKvMenue}}** > **{{MbKvGruppeEditor}}**, el clic derecho de la vista, la búsqueda X/F3 o el grupo **{{MbKvRollout}}**) abre las pistas de la línea de tiempo como hoja de claves; como en 3ds Max se llama **{{MbKvDsMax}}**, como en Blender **{{MbKvDsBl}}**. El tirador sobre la línea de tiempo ajusta su altura. A la izquierda están las filas: **{{MbKvZusammenfassung}}**, una fila de resumen por miembro (clic para plegar) y una fila por hueso; a la derecha cada clave de la secuencia seleccionada es un rombo. Clic en un rombo lo selecciona (Mayús añade), un marco alrededor de claves las selecciona, arrastrar claves seleccionadas las mueve en el tiempo, siempre en fotogramas enteros, porque el GLA guarda fotogramas enteros. Un clic en una fila de resumen selecciona todas las claves de ese fotograma en el miembro. La banda clara es la secuencia de la línea de animation.cfg y la línea naranja el fotograma actual; clic en la regla lo fija. La rueda acerca y el botón central desplaza. **{{MbKvNurAnimierte}}** y **{{MbKvNurGewaehlte}}** filtran las filas, **{{MbKvAllesZeigen}}** encuadra la secuencia. Como en Blender, con el ratón sobre el editor X borra, G mueve y S escala las claves seleccionadas.",
        "**{{MbKvDsOeffnen}}** (nabídka **{{MbBereichAnimation}}** > **{{MbKvMenue}}** > **{{MbKvGruppeEditor}}**, pravé tlačítko pohledu, vyhledávání X/F3 nebo skupina **{{MbKvRollout}}**) otevře stopy časové osy jako dopesheet; jako v 3ds Max se jmenuje **{{MbKvDsMax}}**, jako v Blenderu **{{MbKvDsBl}}**. Úchyt nad časovou osou nastaví jejich výšku. Vlevo jsou řádky: **{{MbKvZusammenfassung}}**, souhrnný řádek pro každou končetinu (kliknutím se sbalí) a řádek pro každou kost; vpravo je každý klíč vybrané sekvence kosočtverec. Kliknutí na kosočtverec ho vybere (Shift přidá), rámeček kolem klíčů je vybere, tažení vybraných klíčů je posune v čase - vždy o celé snímky, protože GLA ukládá celé snímky. Kliknutí na souhrnný řádek vybere všechny klíče toho snímku v končetině. Světlý pruh je sekvence řádku animation.cfg, oranžová čára aktuální snímek; kliknutí do pravítka ho nastaví. Kolečko přibližuje, prostřední tlačítko posouvá. **{{MbKvNurAnimierte}}** a **{{MbKvNurGewaehlte}}** filtrují řádky, **{{MbKvAllesZeigen}}** přizpůsobí sekvenci. Jako v Blenderu maže X, posouvá G a škáluje S vybrané klíče, dokud je myš nad editorem.",
    ),
    "bild_ds": (
        "Dope sheet like 3ds Max (Track View - Dope Sheet) under the views: summary, limbs and bones with their keys of BOTH_WALK1, the selected key orange.",
        "Dopesheet wie 3ds Max (Spuransicht - Dope Sheet) unter den Ansichten: Zusammenfassung, Glieder und Knochen mit ihren Schlüsseln in BOTH_WALK1, der gewählte Schlüssel orange.",
        "视图下方像 3ds Max 的摄影表 (轨迹视图 - 摄影表): BOTH_WALK1 的汇总、肢体和骨骼及其关键帧, 所选关键帧为橙色。",
        "ビューの下の 3ds Max 風ドープシート (トラック ビュー - ドープ シート): BOTH_WALK1 のサマリー、肢、ボーンとそのキー。選択キーはオレンジ。",
        "Hoja de claves como en 3ds Max (Vista de pistas - Hoja de claves) bajo las vistas: resumen, miembros y huesos con sus claves de BOTH_WALK1; la clave seleccionada en naranja.",
        "Dopesheet jako v 3ds Max (Track View - Dope Sheet) pod pohledy: souhrn, končetiny a kosti s jejich klíči v BOTH_WALK1, vybraný klíč oranžově."),
    "h_kurven": ("Curve editor", "Kurveneditor", "曲线编辑器", "カーブエディター", "Editor de curvas", "Editor křivek"),
    "kurven": (
        "**{{MbKvKeOeffnen}}** (like 3ds Max **{{MbKvKeMax}}**, like Blender **{{MbKvKeBl}}**) shows one curve per channel: the rotation as quaternion "
        "**{{MbKvKanalQW}}**, **{{MbKvKanalQX}}**, **{{MbKvKanalQY}}**, **{{MbKvKanalQZ}}** and the location X, Y, Z. Click a bone on the left to unfold its "
        "channels and switch single curves on or off. Drag a key up or down to change the value of that channel, sideways to move it in whole frames; drag a "
        "handle to shape the curve. **{{MbKvGruppeInterp}}** sets constant, linear (turns on the shortest arc, slerp) or Bezier from the selected key to the "
        "next. **{{MbKvGruppeTangenten}}** sets the handles: like Blender Auto Clamped (default of new keys, never overshoots), Auto, Vector, Aligned and "
        "Free; like 3ds Max the same types are called Auto, Smooth, Linear, Custom and Custom (broken). The Bezier curves are computed exactly like the F-curves "
        "of Blender (handle rule, Continuous Acceleration, evaluation); a dragged Auto handle becomes Aligned and a Vector handle Free, as there. Ctrl+wheel "
        "zooms the values. The quaternion is normalised for the pose, so the figure always turns rigidly.",
        "**{{MbKvKeOeffnen}}** (wie 3ds Max **{{MbKvKeMax}}**, wie Blender **{{MbKvKeBl}}**) zeigt eine Kurve je Kanal: die Drehung als Quaternion "
        "**{{MbKvKanalQW}}**, **{{MbKvKanalQX}}**, **{{MbKvKanalQY}}**, **{{MbKvKanalQZ}}** und die Verschiebung X, Y, Z. Ein Klick auf einen Knochen links "
        "klappt seine Kanäle auf, dort lassen sich einzelne Kurven ein- und ausblenden. Einen Schlüssel nach oben oder unten ziehen ändert den Wert dieses "
        "Kanals, zur Seite verschiebt ihn um ganze Frames; einen Henkel ziehen formt die Kurve. **{{MbKvGruppeInterp}}** setzt konstant, linear (dreht auf dem "
        "kürzesten Bogen, slerp) oder Bezier ab dem gewählten Schlüssel bis zum nächsten. **{{MbKvGruppeTangenten}}** setzt die Henkel: wie Blender "
        "Automatisch geklemmt (Vorgabe neuer Schlüssel, schießt nie über), Automatisch, Vektor, Ausgerichtet und Frei; wie 3ds Max heißen dieselben Arten "
        "Automatisch, Glatt, Linear, Benutzerdefiniert und Benutzerdefiniert (getrennt). Die Bezier-Kurven rechnen genau wie die F-Kurven von Blender "
        "(Henkelregel, Continuous Acceleration, Auswertung); ein gezogener Auto-Henkel wird wie dort Ausgerichtet, ein Vektor-Henkel Frei. Strg+Rad zoomt die "
        "Werte. Für die Pose wird das Quaternion normiert, die Figur dreht also immer starr.",
        "**{{MbKvKeOeffnen}}** (像 3ds Max 时为 **{{MbKvKeMax}}**, 像 Blender 时为 **{{MbKvKeBl}}**) 每个通道显示一条曲线: 旋转为四元数 "
        "**{{MbKvKanalQW}}**、**{{MbKvKanalQX}}**、**{{MbKvKanalQY}}**、**{{MbKvKanalQZ}}**, 以及位置 X、Y、Z。单击左侧的骨骼展开其通道, 可在那里显示或"
        "隐藏单条曲线。上下拖动关键帧改变该通道的值, 横向拖动则按整帧移动; 拖动手柄塑造曲线。**{{MbKvGruppeInterp}}** 设置从所选关键帧到下一个关键帧的"
        "常量、线性 (沿最短弧旋转, slerp) 或贝塞尔。**{{MbKvGruppeTangenten}}** 设置手柄: 像 Blender 时为自动钳制 (新关键帧的默认值, 从不超调)、自动、"
        "矢量、对齐和自由; 像 3ds Max 时同样的类型称为自动、平滑、线性、自定义和自定义 (断开)。贝塞尔曲线的计算与 Blender 的 F 曲线完全相同 (手柄规则、"
        "Continuous Acceleration、求值); 被拖动的自动手柄会像那里一样变为对齐, 矢量手柄变为自由。Ctrl+滚轮缩放数值。姿势使用归一化的四元数, 因此人物始终刚性旋转。",
        "**{{MbKvKeOeffnen}}** (3ds Max 風では **{{MbKvKeMax}}**、Blender 風では **{{MbKvKeBl}}**) はチャンネルごとに 1 本のカーブを表示します: "
        "回転はクォータニオン **{{MbKvKanalQW}}**、**{{MbKvKanalQX}}**、**{{MbKvKanalQY}}**、**{{MbKvKanalQZ}}**、位置は X、Y、Z。左のボーンをクリック"
        "するとチャンネルが開き、各カーブの表示を切り替えられます。キーを上下にドラッグするとそのチャンネルの値が変わり、横にドラッグすると整数フレーム"
        "単位で移動します。ハンドルをドラッグしてカーブを整えます。**{{MbKvGruppeInterp}}** は選択キーから次のキーまでを一定、リニア (最短の弧で回転、"
        "slerp)、ベジェにします。**{{MbKvGruppeTangenten}}** はハンドルを設定します: Blender 風では自動クランプ (新しいキーの既定値、オーバーシュート"
        "しない)、自動、ベクトル、整列、フリー。3ds Max 風では同じ種類が自動、スムーズ、リニア、カスタム、カスタム (分離) という名前です。ベジェカーブは"
        "Blender の F カーブとまったく同じに計算します (ハンドル規則、Continuous Acceleration、評価)。ドラッグした自動ハンドルは整列に、ベクトル"
        "ハンドルはフリーになります。Ctrl+ホイールで値をズーム。ポーズではクォータニオンを正規化するので、フィギュアは常に剛体として回転します。",
        "**{{MbKvKeOeffnen}}** (como en 3ds Max **{{MbKvKeMax}}**, como en Blender **{{MbKvKeBl}}**) muestra una curva por canal: la rotación como "
        "cuaternión **{{MbKvKanalQW}}**, **{{MbKvKanalQX}}**, **{{MbKvKanalQY}}**, **{{MbKvKanalQZ}}** y la posición X, Y, Z. Un clic en un hueso a la "
        "izquierda despliega sus canales, donde se muestran u ocultan curvas sueltas. Arrastrar una clave arriba o abajo cambia el valor de ese canal y de lado "
        "la mueve en fotogramas enteros; arrastrar un manejador da forma a la curva. **{{MbKvGruppeInterp}}** pone constante, lineal (gira por el arco más "
        "corto, slerp) o Bézier desde la clave seleccionada hasta la siguiente. **{{MbKvGruppeTangenten}}** fija los manejadores: como en Blender Automático "
        "limitado (por defecto en claves nuevas, nunca sobrepasa), Automático, Vector, Alineado y Libre; como en 3ds Max los mismos tipos se llaman "
        "Automático, Suave, Lineal, Personalizada y Personalizada (rota). Las curvas Bézier se calculan exactamente como las curvas F de Blender (regla de "
        "manejadores, Continuous Acceleration, evaluación); un manejador automático arrastrado pasa a Alineado y uno Vector a Libre, como allí. Ctrl+rueda "
        "acerca los valores. Para la pose el cuaternión se normaliza, así que la figura gira siempre rígida.",
        "**{{MbKvKeOeffnen}}** (jako v 3ds Max **{{MbKvKeMax}}**, jako v Blenderu **{{MbKvKeBl}}**) ukazuje jednu křivku na kanál: rotaci jako kvaternion "
        "**{{MbKvKanalQW}}**, **{{MbKvKanalQX}}**, **{{MbKvKanalQY}}**, **{{MbKvKanalQZ}}** a polohu X, Y, Z. Kliknutí na kost vlevo rozbalí její kanály, kde "
        "lze jednotlivé křivky skrýt a zobrazit. Tažení klíče nahoru či dolů mění hodnotu kanálu, do strany ho posune o celé snímky; tažením úchytu tvarujete "
        "křivku. **{{MbKvGruppeInterp}}** nastaví konstantní, lineární (otáčí po nejkratším oblouku, slerp) nebo Bézier od vybraného klíče k dalšímu. "
        "**{{MbKvGruppeTangenten}}** nastaví úchyty: jako v Blenderu automatická omezená (výchozí u nových klíčů, nikdy nepřestřelí), automatická, vektor, "
        "zarovnaná a volná; jako v 3ds Max se tytéž typy jmenují automatická, hladká, lineární, vlastní a vlastní (rozdělená). Bézierovy křivky se počítají "
        "přesně jako F-křivky Blenderu (pravidlo úchytů, Continuous Acceleration, vyhodnocení); tažený automatický úchyt se stane zarovnaným a vektorový "
        "volným, jako tam. Ctrl+kolečko přibližuje hodnoty. Pro pózu se kvaternion normalizuje, postava se tedy vždy otáčí jako tuhé těleso."),
    "bild_kurven": (
        "Curve editor with the channels of rhand: Bezier keys with Auto Clamped handles, the selected key with its handles.",
        "Kurveneditor mit den Kanälen von rhand: Bezier-Schlüssel mit automatisch geklemmten Henkeln, der gewählte Schlüssel mit seinen Henkeln.",
        "曲线编辑器显示 rhand 的通道: 带自动钳制手柄的贝塞尔关键帧, 所选关键帧带有手柄。",
        "rhand のチャンネルを表示したカーブエディター: 自動クランプのハンドルを持つベジェキーと、ハンドル付きの選択キー。",
        "Editor de curvas con los canales de rhand: claves Bézier con manejadores automáticos limitados; la clave seleccionada con sus manejadores.",
        "Editor křivek s kanály rhand: Bézierovy klíče s automaticky omezenými úchyty, vybraný klíč s úchyty."),
    "h_werkzeuge": ("Animation tools", "Animationswerkzeuge", "动画工具", "アニメーションツール", "Herramientas de animación", "Nástroje animace"),
    "werkzeuge": (
        "Every tool is in the menu **{{MbBereichAnimation}}** > **{{MbKvMenue}}**, in the right-click of the editor and of the view and in the search, each "
        "with a tooltip. The **{{MbKvGruppeWerkzeuge}}** work on the selected keys, channel by channel: **{{MbKvWzBreakdown}}**, **{{MbKvWzNachbar}}**, "
        "**{{MbKvWzVorgabe}}**, **{{MbKvWzPushPull}}**, **{{MbKvWzEase}}** and **{{MbKvWzGlaetten}}**. The **{{MbKvGruppePose}}** set the pose at the current "
        "frame from the previous and next key: **{{MbKvPsBreakdown}}**, **{{MbKvPsPush}}**, **{{MbKvPsRelax}}**, **{{MbKvPsBlend}}**, **{{MbKvPsRuhe}}**. "
        "Each opens a small slider; the view shows the result while you drag, **{{MbKvAnwenden}}** keeps it as one undo step, **{{MbKvAbbrechen}}** restores. "
        "**{{MbKvGruppeTiming}}**: snap the selection to the current frame (or move it so that the first or last key lands there), insert or remove frames "
        "at the current frame, time offset, share key times (also exactly those of the bone selected last). **{{MbKvGruppeWerte}}**: zero, to the current "
        "value, negate, reverse, mirror at the current frame; **{{MbKvAnimOffset}}** carries a change at one key over to the other keys of the bone. "
        "**{{MbKvAnimSpiegeln}}** mirrors the whole sequence over the l_/r_ bones. The **{{MbKvGruppeBibliothek}}** keeps named poses in the model document "
        "and applies them, also mirrored. The **{{MbKvGruppeMarker}}** use the animation.cfg line: keys at its first and last frame, close the loop, select "
        "the keys inside.",
        "Jedes Werkzeug steht im Menü **{{MbBereichAnimation}}** > **{{MbKvMenue}}**, im Rechtsklick des Editors und der Ansicht und in der Suche, jedes mit "
        "Tooltip. Die **{{MbKvGruppeWerkzeuge}}** wirken auf die gewählten Schlüssel, Kanal für Kanal: **{{MbKvWzBreakdown}}**, **{{MbKvWzNachbar}}**, "
        "**{{MbKvWzVorgabe}}**, **{{MbKvWzPushPull}}**, **{{MbKvWzEase}}** und **{{MbKvWzGlaetten}}**. Die **{{MbKvGruppePose}}** setzen die Pose am "
        "aktuellen Frame aus dem vorigen und nächsten Schlüssel: **{{MbKvPsBreakdown}}**, **{{MbKvPsPush}}**, **{{MbKvPsRelax}}**, **{{MbKvPsBlend}}**, "
        "**{{MbKvPsRuhe}}**. Jedes öffnet einen kleinen Regler; die Ansicht zeigt das Ergebnis schon beim Ziehen, **{{MbKvAnwenden}}** behält es als einen "
        "Rückgängig-Schritt, **{{MbKvAbbrechen}}** stellt es wieder her. **{{MbKvGruppeTiming}}**: die Auswahl auf den aktuellen Frame einrasten (oder so "
        "verschieben, dass der erste bzw. letzte Schlüssel dort liegt), am aktuellen Frame Frames einfügen oder entfernen, Zeitversatz, Schlüsselzeiten teilen "
        "(auch genau die des zuletzt gewählten Knochens). **{{MbKvGruppeWerte}}**: Null, auf den aktuellen Wert, negieren, umkehren, am aktuellen Frame "
        "spiegeln; **{{MbKvAnimOffset}}** überträgt eine Änderung an einem Schlüssel auf die anderen Schlüssel des Knochens. **{{MbKvAnimSpiegeln}}** "
        "spiegelt die ganze Sequenz über die l_/r_-Knochen. Die **{{MbKvGruppeBibliothek}}** legt benannte Posen im Modelldokument ab und setzt sie wieder, "
        "auch gespiegelt. Die **{{MbKvGruppeMarker}}** nutzen die animation.cfg-Zeile: Schlüssel an ihrem ersten und letzten Frame, Schleife schließen, die "
        "Schlüssel darin wählen.",
        "每个工具都在菜单 **{{MbBereichAnimation}}** > **{{MbKvMenue}}**、编辑器和视图的右键以及搜索中, 每个都有提示。**{{MbKvGruppeWerkzeuge}}** 逐通道"
        "作用于所选关键帧: **{{MbKvWzBreakdown}}**、**{{MbKvWzNachbar}}**、**{{MbKvWzVorgabe}}**、**{{MbKvWzPushPull}}**、**{{MbKvWzEase}}** 和 "
        "**{{MbKvWzGlaetten}}**。**{{MbKvGruppePose}}** 根据前后关键帧设置当前帧的姿势: **{{MbKvPsBreakdown}}**、**{{MbKvPsPush}}**、**{{MbKvPsRelax}}**、"
        "**{{MbKvPsBlend}}**、**{{MbKvPsRuhe}}**。每个都会打开一个小滑块; 拖动时视图即显示结果, **{{MbKvAnwenden}}** 将其保留为一个撤销步骤, "
        "**{{MbKvAbbrechen}}** 恢复。**{{MbKvGruppeTiming}}**: 把所选吸附到当前帧 (或平移使第一个或最后一个关键帧落在那里)、在当前帧插入或删除帧、时间"
        "偏移、共享关键帧时间 (也可采用最后所选骨骼的时间)。**{{MbKvGruppeWerte}}**: 归零、设为当前值、取反、反向、以当前帧镜像; **{{MbKvAnimOffset}}** "
        "把某个关键帧上的改动传到该骨骼的其他关键帧。**{{MbKvAnimSpiegeln}}** 沿 l_/r_ 骨骼镜像整个序列。**{{MbKvGruppeBibliothek}}** 把命名姿势存入"
        "模型文档并再次应用, 也可镜像应用。**{{MbKvGruppeMarker}}** 使用 animation.cfg 行: 在其首帧和末帧设置关键帧、闭合循环、选择其中的关键帧。",
        "すべてのツールはメニュー **{{MbBereichAnimation}}** > **{{MbKvMenue}}**、エディターとビューの右クリック、検索にあり、それぞれツールチップ"
        "付きです。**{{MbKvGruppeWerkzeuge}}** は選択キーにチャンネルごとに働きます: **{{MbKvWzBreakdown}}**、**{{MbKvWzNachbar}}**、"
        "**{{MbKvWzVorgabe}}**、**{{MbKvWzPushPull}}**、**{{MbKvWzEase}}**、**{{MbKvWzGlaetten}}**。**{{MbKvGruppePose}}** は前後のキーから現在"
        "フレームのポーズを設定します: **{{MbKvPsBreakdown}}**、**{{MbKvPsPush}}**、**{{MbKvPsRelax}}**、**{{MbKvPsBlend}}**、**{{MbKvPsRuhe}}**。"
        "それぞれ小さなスライダーが開き、ドラッグ中からビューに結果が出ます。**{{MbKvAnwenden}}** で取り消し 1 回分として確定、**{{MbKvAbbrechen}}** "
        "で元に戻します。**{{MbKvGruppeTiming}}**: 選択を現在フレームへスナップ (または最初か最後のキーがそこに来るよう移動)、現在フレームでフレームの"
        "挿入・削除、タイムオフセット、キー時間の共有 (最後に選んだボーンの時間に合わせることも)。**{{MbKvGruppeWerte}}**: ゼロ、現在の値にする、反転 "
        "(符号)、逆再生、現在フレームでミラー。**{{MbKvAnimOffset}}** はあるキーの変更をそのボーンの他のキーへ移します。**{{MbKvAnimSpiegeln}}** は"
        "シーケンス全体を l_/r_ のボーンでミラーします。**{{MbKvGruppeBibliothek}}** は名前付きポーズをモデル文書に保存し、ミラーしても適用できます。"
        "**{{MbKvGruppeMarker}}** は animation.cfg 行を使います: 最初と最後のフレームにキー、ループを閉じる、範囲内のキーを選択。",
        "Cada herramienta está en el menú **{{MbBereichAnimation}}** > **{{MbKvMenue}}**, en el clic derecho del editor y de la vista y en la búsqueda, todas "
        "con descripción emergente. Las **{{MbKvGruppeWerkzeuge}}** actúan sobre las claves seleccionadas canal a canal: **{{MbKvWzBreakdown}}**, "
        "**{{MbKvWzNachbar}}**, **{{MbKvWzVorgabe}}**, **{{MbKvWzPushPull}}**, **{{MbKvWzEase}}** y **{{MbKvWzGlaetten}}**. Los **{{MbKvGruppePose}}** "
        "ponen la pose del fotograma actual a partir de la clave anterior y la siguiente: **{{MbKvPsBreakdown}}**, **{{MbKvPsPush}}**, **{{MbKvPsRelax}}**, "
        "**{{MbKvPsBlend}}**, **{{MbKvPsRuhe}}**. Cada una abre un deslizador pequeño; la vista muestra el resultado mientras arrastras, **{{MbKvAnwenden}}** "
        "lo conserva como un paso de deshacer y **{{MbKvAbbrechen}}** lo restaura. **{{MbKvGruppeTiming}}**: ajustar la selección al fotograma actual (o "
        "moverla para que la primera o la última clave quede allí), insertar o quitar fotogramas en el fotograma actual, desfase de tiempo, compartir tiempos "
        "de clave (también exactamente los del último hueso seleccionado). **{{MbKvGruppeWerte}}**: cero, al valor actual, negar, invertir, reflejar en el "
        "fotograma actual; **{{MbKvAnimOffset}}** lleva un cambio en una clave a las demás claves del hueso. **{{MbKvAnimSpiegeln}}** refleja toda la "
        "secuencia sobre los huesos l_/r_. La **{{MbKvGruppeBibliothek}}** guarda poses con nombre en el documento del modelo y las aplica, también "
        "reflejadas. Los **{{MbKvGruppeMarker}}** usan la línea de animation.cfg: claves en su primer y último fotograma, cerrar el bucle, seleccionar las "
        "claves de dentro.",
        "Každý nástroj je v nabídce **{{MbBereichAnimation}}** > **{{MbKvMenue}}**, v pravém tlačítku editoru i pohledu a ve vyhledávání, každý s nápovědou. "
        "**{{MbKvGruppeWerkzeuge}}** působí na vybrané klíče kanál po kanálu: **{{MbKvWzBreakdown}}**, **{{MbKvWzNachbar}}**, **{{MbKvWzVorgabe}}**, "
        "**{{MbKvWzPushPull}}**, **{{MbKvWzEase}}** a **{{MbKvWzGlaetten}}**. **{{MbKvGruppePose}}** nastaví pózu na aktuálním snímku z předchozího a dalšího "
        "klíče: **{{MbKvPsBreakdown}}**, **{{MbKvPsPush}}**, **{{MbKvPsRelax}}**, **{{MbKvPsBlend}}**, **{{MbKvPsRuhe}}**. Každý otevře malý posuvník; "
        "pohled ukazuje výsledek už při tažení, **{{MbKvAnwenden}}** ho ponechá jako jeden krok zpět, **{{MbKvAbbrechen}}** ho vrátí. "
        "**{{MbKvGruppeTiming}}**: přichytit výběr na aktuální snímek (nebo ho posunout, aby tam ležel první či poslední klíč), na aktuálním snímku vložit "
        "nebo odebrat snímky, časový posun, sdílet časy klíčů (i přesně časy naposledy vybrané kosti). **{{MbKvGruppeWerte}}**: nula, na aktuální hodnotu, "
        "negovat, obrátit, zrcadlit podle aktuálního snímku; **{{MbKvAnimOffset}}** přenese změnu na jednom klíči na ostatní klíče kosti. "
        "**{{MbKvAnimSpiegeln}}** zrcadlí celou sekvenci přes kosti l_/r_. **{{MbKvGruppeBibliothek}}** ukládá pojmenované pózy do dokumentu modelu a znovu "
        "je použije, i zrcadlově. **{{MbKvGruppeMarker}}** používají řádek animation.cfg: klíče na jeho prvním a posledním snímku, uzavřít smyčku, vybrat "
        "klíče uvnitř."),
    "bild_werkzeuge": (
        "The menu Animation > Animation tools > Key tools with every slider tool and its tooltip.",
        "Das Menü Animation > Animationswerkzeuge > Schlüssel-Werkzeuge mit jedem Schieber und seinem Tooltip.",
        "菜单 动画 > 动画工具 > 关键帧工具, 包含每个滑块工具及其提示。",
        "メニュー アニメーション > アニメーションツール > キーツール。各スライダーツールとツールチップ。",
        "El menú Animación > Herramientas de animación > Herramientas de claves con cada deslizador y su descripción.",
        "Nabídka Animace > Nástroje animace > Nástroje klíčů s každým posuvníkem a jeho nápovědou."),
    "h_aufraeumen": ("Clean up and reduce", "Aufräumen und Reduzieren", "清理和精简", "クリーンアップと削減", "Limpiar y reducir", "Úklid a redukce"),
    "aufraeumen": (
        "A GLA stores every frame. **{{MbKvReduzieren}}** turns the frames of the selected sequence (with the rig keys on top) into few keys you can edit: per "
        "bone keys are added where the curve deviates most, then every key that is not needed is removed again; linear and Bezier are both tried and the one "
        "with fewer keys stays. The keys then describe the whole motion over the bind pose. The rule is the bound of compressBone from stage 3: baked again, "
        "every bone frame turns within the rounding that the GLA format itself allows, and the location differs by at most one 1/64 step. "
        "{ZAHLEN} **{{MbKvUeberfluessig}}** removes keys equal to their neighbours, **{{MbKvStatisch}}** keeps one key for bones that do not move, "
        "**{{MbKvQuatFilter}}** gives every quaternion the sign of its neighbour, so the curves do not jump (the channels are quaternions, an Euler filter is "
        "not needed). Bake the result with **{{MbRgBacken}}** onto the target slot as before.",
        "Eine GLA speichert jeden Frame. **{{MbKvReduzieren}}** macht aus den Frames der gewählten Sequenz (mit den Rig-Schlüsseln darüber) wenige bearbeitbare "
        "Schlüssel: je Knochen kommen Schlüssel dorthin, wo die Kurve am weitesten abweicht, danach fällt jeder unnötige wieder weg; linear und Bezier werden "
        "beide versucht, das mit weniger Schlüsseln bleibt. Die Schlüssel beschreiben dann die ganze Bewegung über der Bindepose. Die Regel ist die Grenze von "
        "compressBone aus Stufe 3: wieder gebacken dreht jedes Knochen-Frame innerhalb der Rundung, die das GLA-Format selbst zulässt, und die Verschiebung "
        "weicht höchstens um eine 1/64-Stufe ab. {ZAHLEN} **{{MbKvUeberfluessig}}** entfernt Schlüssel, die ihren Nachbarn gleichen, **{{MbKvStatisch}}** "
        "behält einen Schlüssel für Knochen, die sich nicht bewegen, **{{MbKvQuatFilter}}** gibt jedem Quaternion das Vorzeichen seines Nachbarn, damit die "
        "Kurven nicht springen (die Kanäle sind Quaternionen, ein Euler-Filter ist nicht nötig). Das Ergebnis wie bisher mit **{{MbRgBacken}}** auf den "
        "Platz des Ziels backen.",
        "GLA 存储每一帧。**{{MbKvReduzieren}}** 把所选序列的帧 (叠加绑定关键帧) 变成少量可编辑的关键帧: 每根骨骼在曲线偏差最大处加关键帧, 然后再删除"
        "所有不需要的关键帧; 线性和贝塞尔都会尝试, 保留关键帧更少的那种。之后关键帧在绑定姿势之上描述整个运动。规则是第 3 阶段 compressBone 的界限: "
        "再次烘焙后, 每个骨骼帧的旋转都在 GLA 格式本身允许的舍入范围内, 位置最多相差一个 1/64 级。{ZAHLEN} **{{MbKvUeberfluessig}}** 删除与相邻相同的"
        "关键帧, **{{MbKvStatisch}}** 为不动的骨骼保留一个关键帧, **{{MbKvQuatFilter}}** 让每个四元数采用相邻的符号, 使曲线不跳变 (通道是四元数, "
        "不需要欧拉过滤)。像以前一样用 **{{MbRgBacken}}** 把结果烘焙到目标槽位。",
        "GLA はすべてのフレームを保存します。**{{MbKvReduzieren}}** は選択中のシーケンスのフレーム (リグのキーを重ねたもの) を編集できる少数のキーに"
        "します: ボーンごとにカーブのずれが最大の所へキーを足し、そのあと不要なキーをまた削ります。リニアとベジェの両方を試し、キーの少ない方を残し"
        "ます。キーはその後バインドポーズ基準で動き全体を表します。規則はステージ 3 の compressBone の上限です: 再ベイクしても各ボーンフレームの回転は"
        "GLA 形式自体が許す丸めの範囲内で、位置の違いは 1/64 の 1 段までです。{ZAHLEN} **{{MbKvUeberfluessig}}** は隣と同じキーを削除し、"
        "**{{MbKvStatisch}}** は動かないボーンにキーを 1 つだけ残し、**{{MbKvQuatFilter}}** は各クォータニオンの符号を隣に合わせてカーブの跳ねを防ぎ"
        "ます (チャンネルはクォータニオンなのでオイラーフィルターは不要)。結果はこれまで通り **{{MbRgBacken}}** で出力先のスロットへベイクします。",
        "Un GLA guarda cada fotograma. **{{MbKvReduzieren}}** convierte los fotogramas de la secuencia seleccionada (con las claves del rig encima) en pocas "
        "claves editables: por hueso se añaden claves donde la curva más se desvía y después se vuelve a quitar cada clave innecesaria; se prueban lineal y "
        "Bézier y se queda la que tiene menos claves. Las claves describen entonces todo el movimiento sobre la pose de unión. La regla es el límite de "
        "compressBone de la etapa 3: horneado de nuevo, cada fotograma de hueso gira dentro del redondeo que el propio formato GLA permite y la posición difiere "
        "como mucho un paso de 1/64. {ZAHLEN} **{{MbKvUeberfluessig}}** quita claves iguales a sus vecinas, **{{MbKvStatisch}}** deja una clave en los huesos "
        "que no se mueven, **{{MbKvQuatFilter}}** da a cada cuaternión el signo de su vecino para que las curvas no salten (los canales son cuaterniones; no "
        "hace falta filtro de Euler). Hornea el resultado con **{{MbRgBacken}}** en el hueco del destino como antes.",
        "GLA ukládá každý snímek. **{{MbKvReduzieren}}** udělá ze snímků vybrané sekvence (s klíči rigu navrch) několik upravitelných klíčů: pro každou kost "
        "se klíče přidají tam, kde se křivka nejvíc odchyluje, pak každý nepotřebný zase zmizí; zkusí se lineární i Bézier a zůstane to s méně klíči. Klíče "
        "pak popisují celý pohyb nad vaznou pózou. Pravidlem je mez compressBone z fáze 3: po novém zapečení se každý snímek kosti otáčí v mezích zaokrouhlení, "
        "které dovoluje sám formát GLA, a poloha se liší nejvýš o jeden krok 1/64. {ZAHLEN} **{{MbKvUeberfluessig}}** odebere klíče shodné se sousedy, "
        "**{{MbKvStatisch}}** ponechá jeden klíč kostem, které se nehýbou, **{{MbKvQuatFilter}}** dá každému kvaternionu znaménko souseda, aby křivky "
        "neskákaly (kanály jsou kvaterniony, Eulerův filtr není třeba). Výsledek zapečte jako dřív pomocí **{{MbRgBacken}}** na slot cíle."),
    "bild_aufraeumen": (
        "BOTH_WALK1 after reducing: few Bezier keys on the pelvis curves instead of one key per frame.",
        "BOTH_WALK1 nach dem Reduzieren: wenige Bezier-Schlüssel auf den Kurven des Beckens statt eines Schlüssels je Frame.",
        "精简后的 BOTH_WALK1: 骨盆曲线上只有少量贝塞尔关键帧, 而不是每帧一个。",
        "削減後の BOTH_WALK1: 骨盤のカーブに、フレームごとのキーの代わりに少数のベジェキー。",
        "BOTH_WALK1 tras reducir: pocas claves Bézier en las curvas de la pelvis en lugar de una clave por fotograma.",
        "BOTH_WALK1 po redukci: na křivkách pánve několik Bézierových klíčů místo klíče za každý snímek."),
    "h_pfade": ("Dragging motion paths", "Bewegungspfade ziehen", "拖动运动路径", "モーションパスのドラッグ", "Arrastrar trayectorias", "Tažení drah pohybu"),
    "pfade": (
        "On the path of the pelvis or the root (**{{MbRgPfadHinzu}}**) every dot can be dragged in the view: the bone moves there at that frame, in the plane "
        "of the view through the dot, and gets a key (Ctrl+Z undoes it). With **{{MbKvAnimOffset}}** the other keys move along. The paths of hands, feet and "
        "the saber tip move their IK target instead (page IK/FK).",
        "Auf dem Pfad des Beckens oder der Wurzel (**{{MbRgPfadHinzu}}**) lässt sich jeder Punkt in der Ansicht ziehen: der Knochen geht an diesem Frame "
        "dorthin, in der Bildebene durch den Punkt, und bekommt einen Schlüssel (Strg+Z nimmt ihn zurück). Mit **{{MbKvAnimOffset}}** gehen die anderen "
        "Schlüssel mit. An den Pfaden von Händen, Füßen und Säbelspitze bewegt der Punkt das IK-Ziel (Seite IK/FK).",
        "在骨盆或根部的路径上 (**{{MbRgPfadHinzu}}**), 视图中的每个点都可以拖动: 骨骼在该帧移到那里 (在穿过该点的视图平面内) 并获得一个关键帧 (Ctrl+Z "
        "撤销)。开启 **{{MbKvAnimOffset}}** 时其他关键帧随之移动。在手、脚和光剑尖端的路径上, 点移动的是 IK 目标 (见 IK/FK 页)。",
        "骨盤またはルートのパス (**{{MbRgPfadHinzu}}**) では、ビューで各点をドラッグできます: そのフレームでボーンがそこへ動き (点を通るビューの平面"
        "内)、キーが付きます (Ctrl+Z で取り消し)。**{{MbKvAnimOffset}}** がオンなら他のキーも一緒に動きます。手、足、セーバーの先端のパスでは点が IK "
        "ターゲットを動かします (IK/FK のページ)。",
        "En la trayectoria de la pelvis o la raíz (**{{MbRgPfadHinzu}}**) cada punto se puede arrastrar en la vista: el hueso va allí en ese fotograma, en el "
        "plano de la vista que pasa por el punto, y recibe una clave (Ctrl+Z la deshace). Con **{{MbKvAnimOffset}}** las demás claves acompañan. En las "
        "trayectorias de manos, pies y punta del sable el punto mueve su objetivo IK (página IK/FK).",
        "Na dráze pánve nebo kořene (**{{MbRgPfadHinzu}}**) lze každý bod v pohledu táhnout: kost se na tom snímku přesune tam, v rovině pohledu procházející "
        "bodem, a dostane klíč (Ctrl+Z ho vrátí). Se **{{MbKvAnimOffset}}** jdou ostatní klíče s ní. Na drahách rukou, chodidel a hrotu meče bod posouvá "
        "cíl IK (stránka IK/FK)."),
}

# Die Messwerte der Reduktion (Schluss mb5e_kr_<spiel>) in jeder Sprache
ZAHLEN = (
    "Measured on every sequence of _humanoid: Jedi Academy keeps 58 % of the bone frames as keys, Jedi Outcast 66 %, Movie Duels 43 %, every bone frame within the bound.",
    "Gemessen an jeder Sequenz von _humanoid: Jedi Academy behält 58 % der Knochen-Frames als Schlüssel, Jedi Outcast 66 %, Movie Duels 43 %, jedes Knochen-Frame innerhalb der Grenze.",
    "在 _humanoid 的每个序列上测得: Jedi Academy 保留 58% 的骨骼帧作为关键帧, Jedi Outcast 66%, Movie Duels 43%, 每个骨骼帧都在界限内。",
    "_humanoid の全シーケンスで測定: Jedi Academy はボーンフレームの 58 %、Jedi Outcast は 66 %、Movie Duels は 43 % をキーとして残し、すべてのボーンフレームが上限内です。",
    "Medido en cada secuencia de _humanoid: Jedi Academy conserva el 58 % de los fotogramas de hueso como claves, Jedi Outcast el 66 % y Movie Duels el 43 %, todos dentro del límite.",
    "Změřeno na každé sekvenci _humanoid: Jedi Academy ponechá jako klíče 58 % snímků kostí, Jedi Outcast 66 %, Movie Duels 43 %, každý snímek kosti v mezích.",
)


def _ersetzt(t):
    return tuple(s.replace("{ZAHLEN}", ZAHLEN[i]) for i, s in enumerate(t))


ES = {_ersetzt(t)[0]: _ersetzt(t)[4] for t in X.values()}
CS = {_ersetzt(t)[0]: _ersetzt(t)[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    z = _ersetzt(X[schluessel])
    return T(z[0], z[1], z[2], z[3])


def seite():
    """Die Seite modell-kurven (Gruppe Missionen, nach modell-rig)."""
    return {
        "id": "modell-kurven",
        "gruppe": "mission",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("note", _t("hinweis_ap1")),   # AP1: Dopesheet und Kurveneditor sind die Spuren der Zeitleiste
            ("img", "h_mb5_dopesheet", _t("bild_ds"), "breit"),
            ("h2", "dopesheet", _t("h_editor")),
            ("p", _t("editor")),
            ("h2", "kurven", _t("h_kurven")),
            ("p", _t("kurven")),
            ("img", "h_mb5_kurven", _t("bild_kurven"), "breit"),
            ("h2", "werkzeuge", _t("h_werkzeuge")),
            ("p", _t("werkzeuge")),
            ("img", "h_mb5_werkzeuge", _t("bild_werkzeuge")),
            ("h2", "aufraeumen", _t("h_aufraeumen")),
            ("p", _t("aufraeumen")),
            ("img", "h_mb5_aufraeumen", _t("bild_aufraeumen"), "breit"),
            ("h2", "pfade", _t("h_pfade")),
            ("p", _t("pfade")),
        ],
    }
