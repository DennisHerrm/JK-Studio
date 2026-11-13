"""AP11 (MBENTWURF Teil B, 13.11.): mehrere Modelle in Reitern ueber der Ansicht, Kopieren und Einfuegen von Flaechen zwischen den
Modellen (Abgleich R5, X19, P1) - Handbuch des Model-Tabs.

Ein Abschnitt auf der Seite modell-ansehen hinter dem Absatz "aufbau" von inhalt_mb1a1 (vor dem Szenen-Explorer), eine Zeile in
QUELLEN von tools/handbuch/inhalt_modell.py:
    Anhang("inhalt_ap11", "modell-ansehen"),
Die Oberflaechentexte ({{Mx...}}) kommen aus tools/i18n_ap11.py (in tools/gen_i18n.py eingetragen). Jeder Text in sechs Sprachen
(en, de, zh, ja, es, cs). Bilder: handbuch/bilder/<sprache>/h_ap11_max_reiter.webp, h_ap11_blender_reiter.webp, h_ap11_max_menue.webp
(Selbsttest BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=ap11hb, je Sprache; Y:\\werkzeuge\\ap11\\hb.sh).
"""

X = {
    "h_reiter": ("Several models in tabs", "Mehrere Modelle in Reitern", "多个模型的标签页", "複数のモデルをタブで", "Varios modelos en pestañas",
                 "Více modelů na kartách"),
    "reiter": (
        "Above the view every open model has a tab, named like **kyle/model.glm** (folder and file); the row appears as soon as a second model is open or surfaces are on the clipboard - with one model the tab looks as before. A click on a tab switches to that "
        "model; each tab keeps everything of its model: file, GLA and animation with the current frame, selection, undo history, "
        "modifier stack and companion file, rig and control rig, painting, timeline, camera and views. **+**, <kbd>Ctrl</kbd>+<kbd>O</kbd> "
        "and **{{MxNeuerTab}}** in the group Model open another model in a new tab - a model file or one from the game files. A model "
        "that is already open is not opened twice: its tab becomes active. **{{PqImport}}** replaces the model of the active tab, as "
        "before. The **x** of a tab closes it; with unsaved changes you are asked first, and on exit every model with unsaved changes "
        "asks. The last tab does not disappear, it becomes empty. Right-click on a tab: **{{MxSchliessen}}**, **{{MxAndereSchliessen}}**.",
        "Über der Ansicht hat jedes geöffnete Modell einen Reiter, benannt wie **kyle/model.glm** (Ordner und Datei); die Zeile erscheint, sobald ein zweites Modell offen ist oder Flächen in der Ablage liegen - mit einem Modell sieht der Tab aus wie bisher. Ein Klick auf einen "
        "Reiter wechselt zu diesem Modell; jeder Reiter behält alles von seinem Modell: Datei, GLA und Animation mit dem aktuellen Bild, "
        "Auswahl, Rückgängig-Verlauf, Modifier-Stapel und Begleitdatei, Rig und Control Rig, Bemalung, Zeitleiste, Kamera und Ansichten. "
        "**+**, <kbd>Strg</kbd>+<kbd>O</kbd> und **{{MxNeuerTab}}** in der Gruppe Modell öffnen ein weiteres Modell in einem neuen "
        "Reiter - eine Modelldatei oder eines aus den Spieldateien. Ein schon offenes Modell öffnet nicht doppelt: sein Reiter wird "
        "aktiv. **{{PqImport}}** ersetzt wie bisher das Modell des aktiven Reiters. Das **x** eines Reiters schließt ihn; bei "
        "ungespeicherten Änderungen kommt zuerst die Frage, und beim Beenden fragt jedes Modell mit ungespeicherten Änderungen. Der "
        "letzte Reiter verschwindet nicht, er wird leer. Rechtsklick auf einen Reiter: **{{MxSchliessen}}**, **{{MxAndereSchliessen}}**.",
        "视图上方每个打开的模型都有一个标签页, 名称如 **kyle/model.glm** (文件夹和文件); 一旦打开第二个模型或剪贴板中有面, 就会显示这一行 - 只有一个模型时标签和以前一样。单击标签页即切换到该模型; 每个标签页都保留其模型的全部内容: "
        "文件、GLA 和动画及当前帧、选择、撤销历史、修改器堆栈和伴随文件、绑定和控制绑定、绘制、时间轴、相机和视图。**+**、<kbd>Ctrl</kbd>+<kbd>O</kbd> "
        "以及“模型”组中的 **{{MxNeuerTab}}** 会在新标签页中打开另一个模型 - 模型文件或游戏文件中的模型。已打开的模型不会重复打开: 会切换到它的标签页。"
        "**{{PqImport}}** 和以前一样替换当前标签页的模型。标签页上的 **x** 关闭它; 有未保存的更改时会先询问, 退出时每个有未保存更改的模型都会询问。"
        "最后一个标签页不会消失, 而是变为空。右键单击标签页: **{{MxSchliessen}}**、**{{MxAndereSchliessen}}**。",
        "ビューの上には開いているモデルごとにタブがあり、**kyle/model.glm** のように (フォルダーとファイル) 名前が付きます。この行は 2 つ目のモデルを開くか、クリップボードにサーフェスがあると表示されます (モデルが 1 つならこれまでと同じ表示)。タブをクリックするとその"
        "モデルに切り替わります。各タブはモデルのすべてを保持します: ファイル、GLA とアニメーション (現在のフレーム)、選択、元に戻す履歴、"
        "モディファイアスタックと付随ファイル、リグとコントロール リグ、ペイント、タイムライン、カメラ、ビュー。**+**、<kbd>Ctrl</kbd>+<kbd>O</kbd>、"
        "「モデル」グループの **{{MxNeuerTab}}** で別のモデルを新しいタブで開きます (モデル ファイルまたはゲーム ファイルから)。すでに開いているモデルは"
        "二重に開かず、そのタブがアクティブになります。**{{PqImport}}** はこれまでどおりアクティブなタブのモデルを置き換えます。タブの **x** で閉じます。"
        "未保存の変更がある場合は先に確認し、終了時には未保存の変更があるモデルごとに確認します。最後のタブは消えず、空になります。タブの右クリック: "
        "**{{MxSchliessen}}**、**{{MxAndereSchliessen}}**。",
        "Encima de la vista cada modelo abierto tiene una pestaña con un nombre como **kyle/model.glm** (carpeta y archivo); la fila aparece en cuanto hay un segundo modelo abierto o superficies en el portapapeles; con un solo modelo la pestaña se ve como antes. Un clic en una "
        "pestaña cambia a ese modelo; cada pestaña conserva todo lo de su modelo: archivo, GLA y animación con el fotograma actual, "
        "selección, historial de deshacer, pila de modificadores y archivo acompañante, rig y control rig, pintura, línea de tiempo, cámara "
        "y vistas. **+**, <kbd>Ctrl</kbd>+<kbd>O</kbd> y **{{MxNeuerTab}}** en el grupo Modelo abren otro modelo en una pestaña nueva: un "
        "archivo de modelo o uno de los archivos del juego. Un modelo ya abierto no se abre dos veces: se activa su pestaña. "
        "**{{PqImport}}** sustituye, como antes, el modelo de la pestaña activa. La **x** de una pestaña la cierra; con cambios sin guardar se "
        "pregunta antes, y al salir pregunta cada modelo con cambios sin guardar. La última pestaña no desaparece, queda vacía. Clic derecho "
        "en una pestaña: **{{MxSchliessen}}**, **{{MxAndereSchliessen}}**.",
        "Nad pohledem má každý otevřený model kartu s názvem jako **kyle/model.glm** (složka a soubor); řádek se objeví, jakmile je otevřen druhý model nebo jsou ve schránce plochy - s jedním modelem vypadá karta jako dosud. Kliknutí na kartu přepne na tento "
        "model; každá karta si ponechá vše ze svého modelu: soubor, GLA a animaci s aktuálním snímkem, výběr, historii zpět, zásobník "
        "modifikátorů a doprovodný soubor, rig a control rig, malbu, časovou osu, kameru a pohledy. **+**, <kbd>Ctrl</kbd>+<kbd>O</kbd> a "
        "**{{MxNeuerTab}}** ve skupině Model otevřou další model na nové kartě - soubor modelu nebo model ze souborů hry. Už otevřený model se "
        "neotevře dvakrát: aktivuje se jeho karta. **{{PqImport}}** nahradí jako dosud model aktivní karty. **x** karty ji zavře; při "
        "neuložených změnách se nejdřív zeptá a při ukončení se zeptá každý model s neuloženými změnami. Poslední karta nezmizí, zůstane "
        "prázdná. Pravé tlačítko na kartě: **{{MxSchliessen}}**, **{{MxAndereSchliessen}}**."),
    "bild_reiter": (
        "3ds Max mode: two models in tabs above the view, on the right the clipboard hint",
        "3ds-Max-Modus: zwei Modelle in Reitern über der Ansicht, rechts der Hinweis der Ablage",
        "3ds Max 模式: 视图上方的两个模型标签页, 右侧为剪贴板提示",
        "3ds Max 風: ビュー上部のタブに 2 つのモデル、右側にクリップボードの表示",
        "Modo 3ds Max: dos modelos en pestañas encima de la vista, a la derecha el aviso del portapapeles",
        "Režim 3ds Max: dva modely na kartách nad pohledem, vpravo upozornění schránky"),
    "bild_reiter_bl": (
        "Blender mode: the same tabs above the view, right of the toolbar",
        "Blender-Modus: dieselben Reiter über der Ansicht, rechts der Werkzeugleiste",
        "Blender 模式: 视图上方相同的标签页, 位于工具栏右侧",
        "Blender 風: ツールバーの右、ビュー上部に同じタブ",
        "Modo Blender: las mismas pestañas encima de la vista, a la derecha de la barra de herramientas",
        "Režim Blender: stejné karty nad pohledem, vpravo od panelu nástrojů"),
    "ablage": (
        "**Copy surfaces from one model into another:** right-click a surface in the Scene Explorer and choose **{{MxKopieren}}** "
        "(<kbd>Ctrl</kbd>+<kbd>C</kbd> copies the selected surfaces). The tab row shows on the right what is on the clipboard. In the same "
        "or another model, **{{MxEinfuegenKind}}** in the right-click menu of a surface (<kbd>Ctrl</kbd>+<kbd>V</kbd>: below the selected "
        "surface) pastes them as its children, as one undo step of that model. Weights go to the bones of the same name; a bone the "
        "model does not have gives its weight to the nearest parent bone it has - the message names each one, nothing is lost silently. "
        "All LODs come along (more than the model has are left out, fewer are filled with the coarsest), as do the shader shown by the "
        "source and the modifier stack; a name that already exists gets _1, _2. The limits of the game (vertices per surface, 32 bones "
        "per surface, 4 weights per vertex) are checked and named in the message. Into an .md3 prop the surfaces are pasted rigid. In "
        "the Animation area <kbd>Ctrl</kbd>+<kbd>C</kbd> and <kbd>Ctrl</kbd>+<kbd>V</kbd> copy and paste the pose, as in Blender.",
        "**Flächen von einem Modell in ein anderes kopieren:** Rechtsklick auf eine Fläche im Szenen-Explorer und **{{MxKopieren}}** "
        "(<kbd>Strg</kbd>+<kbd>C</kbd> kopiert die gewählten Flächen). Die Reiterzeile zeigt rechts, was in der Ablage liegt. Im selben "
        "oder einem anderen Modell fügt **{{MxEinfuegenKind}}** im Rechtsklick-Menü einer Fläche (<kbd>Strg</kbd>+<kbd>V</kbd>: unter der "
        "gewählten Fläche) sie als ihre Kinder ein, als ein Rückgängig-Schritt dieses Modells. Gewichte gehen an die gleichnamigen "
        "Knochen; ein Knochen, den das Modell nicht hat, gibt sein Gewicht an den nächsten vorhandenen Elternknochen - die Meldung nennt "
        "jeden, nichts geht still verloren. Alle Detailstufen kommen mit (mehr als das Modell hat fallen weg, fehlende werden mit der "
        "gröbsten gefüllt), ebenso der Shader, den die Quelle zeigt, und der Modifier-Stapel; ein Name, den es schon gibt, bekommt _1, _2. "
        "Die Grenzen des Spiels (Ecken je Fläche, 32 Knochen je Fläche, 4 Gewichte je Ecke) werden geprüft und in der Meldung genannt. In "
        "ein .md3-Requisit kommen die Flächen starr. Im Bereich Animation kopieren und fügen <kbd>Strg</kbd>+<kbd>C</kbd> und "
        "<kbd>Strg</kbd>+<kbd>V</kbd> die Pose ein, wie in Blender.",
        "**将面从一个模型复制到另一个模型:** 在场景资源管理器中右键单击一个面并选择 **{{MxKopieren}}** (<kbd>Ctrl</kbd>+<kbd>C</kbd> 复制所选面)。"
        "标签行右侧显示剪贴板中的内容。在同一模型或另一个模型中, 面的右键菜单中的 **{{MxEinfuegenKind}}** (<kbd>Ctrl</kbd>+<kbd>V</kbd>: 粘贴到所选面下) "
        "会将它们粘贴为该面的子项, 作为该模型的一个撤销步骤。权重转到同名骨骼; 模型中没有的骨骼会把权重交给其最近的已有父骨骼 - 消息会逐一列出, "
        "不会悄悄丢失任何内容。所有 LOD 一并带入 (多于模型的部分被省略, 不足的用最粗的填充), 源模型显示的着色器和修改器堆栈也一并带入; 已存在的名称会加上 _1、_2。"
        "会检查游戏的限制 (每个面的顶点数、每个面 32 根骨骼、每个顶点 4 个权重) 并在消息中列出。粘贴到 .md3 道具中的面是刚性的。"
        "在“动画”区域中, <kbd>Ctrl</kbd>+<kbd>C</kbd> 和 <kbd>Ctrl</kbd>+<kbd>V</kbd> 复制和粘贴姿势, 与 Blender 相同。",
        "**サーフェスを別のモデルにコピー:** シーン エクスプローラでサーフェスを右クリックして **{{MxKopieren}}** を選びます "
        "(<kbd>Ctrl</kbd>+<kbd>C</kbd> は選択したサーフェスをコピー)。タブの行の右側にクリップボードの内容が表示されます。同じモデルまたは別のモデルで、"
        "サーフェスの右クリック メニューの **{{MxEinfuegenKind}}** (<kbd>Ctrl</kbd>+<kbd>V</kbd>: 選択したサーフェスの下に) を選ぶと、その子として"
        "貼り付けます (そのモデルの元に戻す 1 ステップ)。ウェイトは同名のボーンへ。モデルにないボーンは、そのウェイトを最も近い既存の親ボーンに渡し、"
        "メッセージにすべて表示されます - 黙って失われるものはありません。すべての LOD も移ります (モデルより多い分は省き、足りない分は最も粗い LOD で埋めます)。"
        "コピー元に表示されていたシェーダーとモディファイアスタックも一緒です。すでにある名前には _1、_2 が付きます。ゲームの制限 (サーフェスあたりの頂点数、"
        "サーフェスあたり 32 ボーン、頂点あたり 4 ウェイト) を確認し、メッセージに表示します。.md3 小道具には固定で貼り付けます。アニメーション エリアでは "
        "<kbd>Ctrl</kbd>+<kbd>C</kbd> と <kbd>Ctrl</kbd>+<kbd>V</kbd> は Blender と同じくポーズをコピー・貼り付けします。",
        "**Copiar superficies de un modelo a otro:** clic derecho en una superficie del Explorador de escena y **{{MxKopieren}}** "
        "(<kbd>Ctrl</kbd>+<kbd>C</kbd> copia las superficies seleccionadas). La fila de pestañas muestra a la derecha lo que hay en el "
        "portapapeles. En el mismo modelo o en otro, **{{MxEinfuegenKind}}** en el menú contextual de una superficie (<kbd>Ctrl</kbd>+"
        "<kbd>V</kbd>: bajo la superficie seleccionada) las pega como hijas suyas, en un solo paso de deshacer de ese modelo. Los pesos van "
        "a los huesos del mismo nombre; un hueso que el modelo no tiene pasa su peso al hueso padre más cercano que sí tenga; el mensaje "
        "los nombra todos, nada se pierde sin aviso. Vienen todos los LOD (los que sobran se omiten, los que faltan se rellenan con el más "
        "simple), así como el sombreador que muestra el origen y la pila de modificadores; un nombre que ya existe recibe _1, _2. Se "
        "comprueban los límites del juego (vértices por superficie, 32 huesos por superficie, 4 pesos por vértice) y se nombran en el "
        "mensaje. En un accesorio .md3 las superficies se pegan rígidas. En el área Animación, <kbd>Ctrl</kbd>+<kbd>C</kbd> y "
        "<kbd>Ctrl</kbd>+<kbd>V</kbd> copian y pegan la pose, como en Blender.",
        "**Kopírování ploch z jednoho modelu do jiného:** klikněte pravým tlačítkem na plochu v Průzkumníku scény a zvolte **{{MxKopieren}}** "
        "(<kbd>Ctrl</kbd>+<kbd>C</kbd> zkopíruje vybrané plochy). Řádek karet vpravo ukazuje, co je ve schránce. Ve stejném nebo jiném modelu "
        "**{{MxEinfuegenKind}}** v nabídce pravého tlačítka plochy (<kbd>Ctrl</kbd>+<kbd>V</kbd>: pod vybranou plochu) vloží plochy jako její "
        "potomky, jako jeden krok zpět tohoto modelu. Váhy přejdou na stejnojmenné kosti; kost, kterou model nemá, předá svou váhu nejbližší "
        "existující rodičovské kosti - zpráva každou uvede, nic se tiše neztratí. Přejdou všechny LOD (přebývající vypadnou, chybějící se "
        "vyplní nejhrubším), stejně jako shader, který zdroj ukazoval, a zásobník modifikátorů; existující název dostane _1, _2. Limity hry "
        "(vrcholy na plochu, 32 kostí na plochu, 4 váhy na vrchol) se zkontrolují a uvedou ve zprávě. Do rekvizity .md3 se plochy vloží "
        "pevně. V oblasti Animace <kbd>Ctrl</kbd>+<kbd>C</kbd> a <kbd>Ctrl</kbd>+<kbd>V</kbd> kopírují a vkládají pózu, jako v Blenderu."),
    "bild_menue": (
        "Right-click on a surface: Info, Copy, Paste as child of this, status, root",
        "Rechtsklick auf eine Fläche: Info, Kopieren, Einfügen als Kind hiervon, Status, Wurzel",
        "右键单击面: 信息、复制、粘贴为此项的子项、状态、根",
        "サーフェスの右クリック: 情報、コピー、これの子として貼り付け、状態、ルート",
        "Clic derecho en una superficie: Información, Copiar, Pegar como hijo de esta, estado, raíz",
        "Pravé tlačítko na ploše: Informace, Kopírovat, Vložit jako potomka této, stav, kořen"),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def einhaengen(ansehen):
    """Der Abschnitt "Mehrere Modelle in Reitern" der Seite modell-ansehen: hinter den Absatz "aufbau" von inhalt_mb1a1 (vor dem
    Szenen-Explorer)."""
    import inhalt_mb1a1
    b = ansehen["bloecke"]
    ziel = inhalt_mb1a1._t("aufbau").t["en"]
    stellen = [n for n, x in enumerate(b) if x[0] == "p" and getattr(x[1], "t", {}).get("en") == ziel]
    if len(stellen) != 1:
        raise SystemExit("inhalt_ap11: der Absatz aufbau steht %d-mal auf modell-ansehen" % len(stellen))
    neu = [
        ("h2", "modelltabs", _t("h_reiter")),
        ("p", _t("reiter")),
        ("img", "h_ap11_max_reiter", _t("bild_reiter"), "breit"),
        ("img", "h_ap11_blender_reiter", _t("bild_reiter_bl"), "breit"),
        ("p", _t("ablage")),
        ("img", "h_ap11_max_menue", _t("bild_menue"), "breit"),
    ]
    b[stellen[0] + 1:stellen[0] + 1] = neu
