"""rc609 T1: ganze Tabs in eigenen Fenstern - Abschnitt der Seite "Fenster und
Bildschirme anordnen" (inhalt_dock.py). Der Selbsttest gui/selbsttest_tabfenster.inc
(BHED_EDITORTEST=tabfenster, tabfensterneustart, tabfensterleistung) prueft jede
Funktion; die Bilder nimmt BHED_EDITORTEST=tabfenster mit BHED_TF_NUR=fotos auf.

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs): die ersten
vier gehen in T(...), Spanisch und Tschechisch nach inhalt_es.py / inhalt_cs.py
(ES, CS). Die Bloecke entstehen erst in bloecke() - T liest beim Anlegen die
Tabellen der weiteren Sprachen, und die lesen ES/CS aus dieser Datei.
"""

X = {
    "titel": (
        "A whole tab in its own window", "Einen ganzen Tab in ein eigenes Fenster", "将整个标签页放入独立窗口", "タブ全体を別ウィンドウに",
        "Una pestaña entera en su propia ventana", "Celá karta ve vlastním okně"),
    "einleitung": (
        "A whole tab - **{{ViewEvents}}**, **{{ViewMap}}**, **{{ViewModel}}**, **{{ViewMapEditor}}** or **{{ViewMenuEditor}}** - can become a window of its own, for example to write the script flow in Events on one screen while the mission preview runs on the second, or to fill a gap in the map editor next to the mission. Both tabs are drawn and work at the same time.",
        "Ein ganzer Tab - **{{ViewEvents}}**, **{{ViewMap}}**, **{{ViewModel}}**, **{{ViewMapEditor}}** oder **{{ViewMenuEditor}}** - kann ein eigenes Fenster werden, etwa um auf einem Bildschirm den Skriptablauf in Ereignisse zu schreiben, während auf dem zweiten die Missionsvorschau läuft, oder um neben der Mission eine Lücke im Karten-Editor zu füllen. Beide Tabs werden gezeichnet und sind gleichzeitig bedienbar.",
        "整个标签页——**{{ViewEvents}}**、**{{ViewMap}}**、**{{ViewModel}}**、**{{ViewMapEditor}}** 或 **{{ViewMenuEditor}}**——都可以成为独立窗口,例如在一个屏幕上编写事件中的脚本流程,同时在第二个屏幕上运行任务预览,或在任务旁边于地图编辑器中补上缺口。两个标签页同时绘制,也都可以同时操作。",
        "タブ全体(**{{ViewEvents}}**、**{{ViewMap}}**、**{{ViewModel}}**、**{{ViewMapEditor}}**、**{{ViewMenuEditor}}**)を独立したウィンドウにできます。たとえば一方の画面でイベントのスクリプトフローを書きながら、もう一方の画面でミッションのプレビューを動かしたり、ミッションの横でマップエディターの穴を埋めたりできます。両方のタブが描画され、同時に操作できます。",
        "Una pestaña entera - **{{ViewEvents}}**, **{{ViewMap}}**, **{{ViewModel}}**, **{{ViewMapEditor}}** o **{{ViewMenuEditor}}** - puede convertirse en una ventana propia, por ejemplo para escribir el flujo del script en Eventos en una pantalla mientras la vista previa de la misión se ejecuta en la segunda, o para rellenar un hueco en el editor de mapas junto a la misión. Ambas pestañas se dibujan y funcionan a la vez.",
        "Celá karta - **{{ViewEvents}}**, **{{ViewMap}}**, **{{ViewModel}}**, **{{ViewMapEditor}}** nebo **{{ViewMenuEditor}}** - se může stát samostatným oknem, třeba abyste na jedné obrazovce psali průběh skriptu v Událostech, zatímco na druhé běží náhled mise, nebo abyste vedle mise doplnili mezeru v editoru map. Obě karty se kreslí a dají se ovládat současně."),
    "bild_menue": (
        "Right-click a tab in the mode bar: {{TfLoesen}}.", "Rechtsklick auf einen Tab in der Modusleiste: {{TfLoesen}}.",
        "在模式栏中右键单击标签页:{{TfLoesen}}。", "モードバーのタブを右クリック:{{TfLoesen}}。",
        "Clic derecho en una pestaña de la barra de modos: {{TfLoesen}}.", "Pravé kliknutí na kartu v liště režimů: {{TfLoesen}}."),
    "schritt1": (
        "Right-click the tab in the mode bar and choose **{{TfLoesen}}**. Or drag its button out of the window and let go outside. Or use **{{DockMenu}} > {{TfMenuEigenes}}** for the tab you are in.",
        "Klicke den Tab in der Modusleiste mit der rechten Maustaste an und wähle **{{TfLoesen}}**. Oder ziehe seinen Knopf aus dem Fenster und lass ihn draußen los. Oder nimm **{{DockMenu}} > {{TfMenuEigenes}}** für den Tab, in dem du gerade bist.",
        "在模式栏中右键单击该标签页并选择 **{{TfLoesen}}**。也可以把它的按钮拖出窗口并在外面松开,或者对当前所在的标签页使用 **{{DockMenu}} > {{TfMenuEigenes}}**。",
        "モードバーのタブを右クリックし、**{{TfLoesen}}** を選びます。ボタンをウィンドウの外へドラッグして離しても、今いるタブなら **{{DockMenu}} > {{TfMenuEigenes}}** でも同じです。",
        "Haz clic derecho en la pestaña de la barra de modos y elige **{{TfLoesen}}**. O arrastra su botón fuera de la ventana y suéltalo fuera. O usa **{{DockMenu}} > {{TfMenuEigenes}}** para la pestaña en la que estás.",
        "Klikněte pravým tlačítkem na kartu v liště režimů a zvolte **{{TfLoesen}}**. Nebo přetáhněte její tlačítko ven z okna a pusťte ho venku. Nebo použijte **{{DockMenu}} > {{TfMenuEigenes}}** pro kartu, ve které právě jste."),
    "schritt2": (
        "The tab opens as a separate Windows window next to the mode bar of the main window. Drag it by its title bar wherever you like - also onto a second screen. If it was the tab of the main window, the main window shows the next free tab.",
        "Der Tab öffnet sich als eigenes Windows-Fenster neben der Modusleiste des Hauptfensters. Zieh es an der Titelleiste, wohin du willst - auch auf einen zweiten Bildschirm. War es der Tab des Hauptfensters, zeigt dieses den nächsten freien Tab.",
        "该标签页会作为独立的 Windows 窗口在主窗口的模式栏旁打开。拖动标题栏把它放到任意位置——也可以放到第二个屏幕上。如果它原本是主窗口当前的标签页,主窗口会显示下一个空闲的标签页。",
        "タブはメインウィンドウのモードバーの横に独立した Windows ウィンドウとして開きます。タイトルバーをつかんで好きな場所へ(2 台目の画面にも)移せます。メインウィンドウで表示中のタブだった場合、メインウィンドウは次の空いているタブを表示します。",
        "La pestaña se abre como una ventana de Windows aparte junto a la barra de modos de la ventana principal. Arrástrala por su barra de título adonde quieras, también a una segunda pantalla. Si era la pestaña de la ventana principal, esta muestra la siguiente pestaña libre.",
        "Karta se otevře jako samostatné okno Windows vedle lišty režimů hlavního okna. Přetáhněte ho za titulek, kam chcete - i na druhou obrazovku. Pokud to byla karta hlavního okna, hlavní okno ukáže další volnou kartu."),
    "schritt3": (
        "In the mode bar, a tab that has its own window is shown pale with a frame. A click brings its window to the front.",
        "In der Modusleiste steht ein Tab mit eigenem Fenster blass und mit Rahmen. Ein Klick holt sein Fenster nach vorn.",
        "在模式栏中,拥有独立窗口的标签页显示为浅色并带边框。单击它会把其窗口置于前面。",
        "モードバーでは、独立したウィンドウを持つタブが淡い色で枠付きで表示されます。クリックするとそのウィンドウが前面に来ます。",
        "En la barra de modos, una pestaña con ventana propia aparece pálida y con marco. Un clic trae su ventana al frente.",
        "V liště režimů je karta s vlastním oknem bledá a s rámečkem. Kliknutí přenese její okno dopředu."),
    "bild_zwei": (
        "Events in the main window, the mission with its preview in its own window - both at work at the same time.",
        "Ereignisse im Hauptfenster, die Mission mit ihrer Vorschau in einem eigenen Fenster - beide arbeiten gleichzeitig.",
        "主窗口中是事件,任务及其预览位于独立窗口中——两者同时工作。",
        "メインウィンドウにイベント、別ウィンドウにプレビュー付きのミッション。両方が同時に動作します。",
        "Eventos en la ventana principal, la misión con su vista previa en su propia ventana: ambas trabajando a la vez.",
        "Události v hlavním okně, mise s náhledem ve vlastním okně - obě pracují současně."),
    "skripte": (
        "**The same scripts in both windows.** Events and Mission work on the same open scripts: a change in one window is in the other at once, the tabs of the scripts are the same.",
        "**Dieselben Skripte in beiden Fenstern.** Ereignisse und Mission arbeiten auf denselben offenen Skripten: eine Änderung in einem Fenster steht sofort im anderen, die Reiter der Skripte sind dieselben.",
        "**两个窗口中是同一批脚本。** 事件和任务处理的是同一批已打开的脚本:在一个窗口中的修改会立即出现在另一个窗口中,脚本的选项卡也相同。",
        "**両方のウィンドウで同じスクリプト。** イベントとミッションは同じ開いているスクリプトを扱います。一方のウィンドウでの変更はすぐにもう一方にも反映され、スクリプトのタブも同じです。",
        "**Los mismos scripts en ambas ventanas.** Eventos y Misión trabajan con los mismos scripts abiertos: un cambio en una ventana aparece al instante en la otra, y las pestañas de los scripts son las mismas.",
        "**Stejné skripty v obou oknech.** Události i Mise pracují se stejnými otevřenými skripty: změna v jednom okně je hned i v druhém a karty skriptů jsou stejné."),
    "undo": (
        "**Shortcuts and undo go to the window you work in.** <kbd>Ctrl</kbd>+<kbd>Z</kbd> in the map editor window undoes a step of the map, in the Events or Mission window a step of the script. Each document has one undo history, whichever window made the change.",
        "**Tastenkürzel und Rückgängig gelten dem Fenster, in dem du arbeitest.** <kbd>Strg</kbd>+<kbd>Z</kbd> im Fenster des Karten-Editors nimmt einen Schritt der Karte zurück, im Fenster von Ereignisse oder Mission einen Schritt des Skripts. Jedes Dokument hat einen Rückgängig-Verlauf, egal in welchem Fenster geändert wurde.",
        "**快捷键和撤销作用于你正在操作的窗口。** 在地图编辑器窗口中按 <kbd>Ctrl</kbd>+<kbd>Z</kbd> 会撤销地图的一步,在事件或任务窗口中则撤销脚本的一步。无论在哪个窗口中修改,每个文档都只有一份撤销历史。",
        "**ショートカットと元に戻すは、作業中のウィンドウに働きます。** マップエディターのウィンドウで <kbd>Ctrl</kbd>+<kbd>Z</kbd> を押すとマップの 1 ステップが、イベントやミッションのウィンドウではスクリプトの 1 ステップが元に戻ります。どのウィンドウで変更しても、各ドキュメントの元に戻す履歴は 1 つです。",
        "**Los atajos y deshacer van a la ventana en la que trabajas.** <kbd>Ctrl</kbd>+<kbd>Z</kbd> en la ventana del editor de mapas deshace un paso del mapa; en la ventana de Eventos o Misión, un paso del script. Cada documento tiene un solo historial de deshacer, sea cual sea la ventana en la que se cambió.",
        "**Klávesové zkratky a Zpět platí pro okno, ve kterém pracujete.** <kbd>Ctrl</kbd>+<kbd>Z</kbd> v okně editoru map vrátí krok mapy, v okně Událostí nebo Mise krok skriptu. Každý dokument má jednu historii Zpět bez ohledu na to, ve kterém okně se měnil."),
    "dialoge": (
        "**Dialogs** such as the question before closing open in the main window; it then comes to the front.",
        "**Dialoge** wie die Frage vor dem Schließen erscheinen im Hauptfenster; es kommt dann nach vorn.",
        "**对话框**(例如关闭前的询问)会出现在主窗口中,主窗口随之置于前面。",
        "閉じる前の確認などの**ダイアログ**はメインウィンドウに表示され、そのときメインウィンドウが前面に来ます。",
        "Los **diálogos**, como la pregunta antes de cerrar, se abren en la ventana principal, que entonces pasa al frente.",
        "**Dialogy**, například dotaz před zavřením, se otevírají v hlavním okně; to se pak přesune dopředu."),
    "ruhe": (
        "**A resting window does not draw.** A window in which nothing changes shows no new picture, while you work in another one. The camera and 2D views of the map editor, like NetRadiant-Custom, draw only when something changes in them - input in the editor, the map, the view, a finished background job or an animation.",
        "**Ein ruhendes Fenster zeichnet nicht.** Ein Fenster, in dem sich nichts ändert, zeigt kein neues Bild, während du in einem anderen arbeitest. Kamera und 2D-Ansichten des Karten-Editors zeichnen wie in NetRadiant-Custom nur, wenn sich in ihnen etwas ändert - eine Eingabe im Editor, die Karte, die Ansicht, ein fertiger Hintergrundlauf oder eine Animation.",
        "**静止的窗口不会绘制。** 当你在另一个窗口中工作时,内容没有变化的窗口不会显示新画面。与 NetRadiant-Custom 相同,地图编辑器的摄像机视图和 2D 视图只在其中有变化时才绘制——编辑器中的输入、地图、视图、完成的后台任务或动画。",
        "**静止しているウィンドウは描画しません。** 別のウィンドウで作業している間、何も変化しないウィンドウは新しい画像を表示しません。マップエディターのカメラと 2D ビューは、NetRadiant-Custom と同じく、その中で何かが変わったとき (エディター内の入力、マップ、ビュー、終わったバックグラウンド処理、アニメーション) だけ描画します。",
        "**Una ventana en reposo no dibuja.** Una ventana en la que nada cambia no muestra una imagen nueva mientras trabajas en otra. La cámara y las vistas 2D del editor de mapas, como en NetRadiant-Custom, solo dibujan cuando algo cambia en ellas: una entrada en el editor, el mapa, la vista, un trabajo en segundo plano terminado o una animación.",
        "**Okno v klidu nekreslí.** Okno, ve kterém se nic nemění, neukazuje nový obraz, zatímco pracujete v jiném. Kamera a 2D pohledy editoru map kreslí jako v NetRadiant-Custom jen tehdy, když se v nich něco změní - vstup v editoru, mapa, pohled, dokončená úloha na pozadí nebo animace."),
    "qualitaet": (
        "**Every view keeps its quality.** A second window costs extra time per frame, but no view is drawn smaller or simpler because of it.",
        "**Jede Ansicht behält ihre Qualität.** Ein zweites Fenster kostet zusätzlich Zeit je Bild, aber keine Ansicht wird deshalb kleiner oder einfacher gezeichnet.",
        "**每个视图都保持原有质量。** 第二个窗口会增加每帧的耗时,但不会因此把任何视图绘制得更小或更简单。",
        "**どのビューも品質はそのままです。** 2 つ目のウィンドウの分だけ 1 フレームの時間は増えますが、そのためにビューが小さく、または簡略に描かれることはありません。",
        "**Cada vista conserva su calidad.** Una segunda ventana cuesta tiempo adicional por fotograma, pero ninguna vista se dibuja por ello más pequeña ni más simple.",
        "**Každý pohled si zachová kvalitu.** Druhé okno stojí čas navíc na každý snímek, ale žádný pohled se kvůli tomu nekreslí menší ani jednodušší."),
    "bild_fenstermenue": (
        "{{DockMenu}}: {{TfMenuEigenes}} and {{TfMenuAlleZurueck}}.", "{{DockMenu}}: {{TfMenuEigenes}} und {{TfMenuAlleZurueck}}.",
        "{{DockMenu}}:{{TfMenuEigenes}} 和 {{TfMenuAlleZurueck}}。", "{{DockMenu}}:{{TfMenuEigenes}} と {{TfMenuAlleZurueck}}。",
        "{{DockMenu}}: {{TfMenuEigenes}} y {{TfMenuAlleZurueck}}.", "{{DockMenu}}: {{TfMenuEigenes}} a {{TfMenuAlleZurueck}}."),
    "zurueck_titel": (
        "Bringing a tab back", "Einen Tab zurückholen", "取回标签页", "タブを元に戻す", "Devolver una pestaña", "Vrácení karty"),
    "zurueck1": (
        "Close its window (the cross in its title bar, or <kbd>Alt</kbd>+<kbd>F4</kbd>): the tab is back in the main window and is shown there.",
        "Schließe sein Fenster (das Kreuz in der Titelleiste oder <kbd>Alt</kbd>+<kbd>F4</kbd>): Der Tab ist zurück im Hauptfenster und wird dort gezeigt.",
        "关闭它的窗口(标题栏中的叉号,或 <kbd>Alt</kbd>+<kbd>F4</kbd>):该标签页回到主窗口并在那里显示。",
        "そのウィンドウを閉じます(タイトルバーの × または <kbd>Alt</kbd>+<kbd>F4</kbd>)。タブはメインウィンドウに戻り、そこに表示されます。",
        "Cierra su ventana (la cruz de su barra de título o <kbd>Alt</kbd>+<kbd>F4</kbd>): la pestaña vuelve a la ventana principal y se muestra allí.",
        "Zavřete jeho okno (křížek v titulku nebo <kbd>Alt</kbd>+<kbd>F4</kbd>): karta je zpět v hlavním okně a zobrazí se tam."),
    "zurueck2": (
        "Drag the window by its title bar onto the mode bar of the main window: the mode bar lights up, let go there.",
        "Zieh das Fenster an der Titelleiste auf die Modusleiste des Hauptfensters: Die Modusleiste leuchtet auf, dort loslassen.",
        "拖动窗口的标题栏到主窗口的模式栏上:模式栏会高亮,在那里松开即可。",
        "タイトルバーをつかんでウィンドウをメインウィンドウのモードバーへドラッグします。モードバーが光ったら、そこで離します。",
        "Arrastra la ventana por su barra de título hasta la barra de modos de la ventana principal: la barra de modos se ilumina; suéltala ahí.",
        "Přetáhněte okno za titulek na lištu režimů hlavního okna: lišta režimů se rozsvítí, tam pusťte."),
    "zurueck3": (
        "Right-click the tab in the mode bar and choose **{{TfZurueck}}**, or **{{DockMenu}} > {{TfMenuAlleZurueck}}** for all of them.",
        "Rechtsklick auf den Tab in der Modusleiste und **{{TfZurueck}}**, oder **{{DockMenu}} > {{TfMenuAlleZurueck}}** für alle.",
        "在模式栏中右键单击该标签页并选择 **{{TfZurueck}}**,或使用 **{{DockMenu}} > {{TfMenuAlleZurueck}}** 一次取回全部。",
        "モードバーのタブを右クリックして **{{TfZurueck}}** を選ぶか、すべてなら **{{DockMenu}} > {{TfMenuAlleZurueck}}** を使います。",
        "Clic derecho en la pestaña de la barra de modos y **{{TfZurueck}}**, o **{{DockMenu}} > {{TfMenuAlleZurueck}}** para todas.",
        "Pravé kliknutí na kartu v liště režimů a **{{TfZurueck}}**, nebo **{{DockMenu}} > {{TfMenuAlleZurueck}}** pro všechny."),
    "bild_zurueck": (
        "Dragged back: over the mode bar the main window shows where the tab can go.",
        "Zurückziehen: Über der Modusleiste zeigt das Hauptfenster, wohin der Tab kann.",
        "拖回:在模式栏上方,主窗口会显示标签页可以放回的位置。",
        "ドラッグで戻す:モードバーの上で、メインウィンドウがタブを戻せる場所を示します。",
        "Arrastrar de vuelta: sobre la barra de modos, la ventana principal muestra adónde puede ir la pestaña.",
        "Přetažení zpět: nad lištou režimů hlavní okno ukazuje, kam může karta jít."),
    "merken": (
        "Which tabs have their own window, their position, size and screen are kept for the next start. **{{DockSaveAs}}** saves them with the layout; loading the layout brings them back. If a screen is missing at the next start, the window comes onto the screen of the main window.",
        "Welche Tabs ein eigenes Fenster haben, ihre Lage, Größe und ihr Bildschirm bleiben für den nächsten Start. **{{DockSaveAs}}** speichert sie mit der Anordnung; das Laden der Anordnung holt sie zurück. Fehlt beim nächsten Start ein Bildschirm, kommt das Fenster auf den Bildschirm des Hauptfensters.",
        "哪些标签页拥有独立窗口,以及它们的位置、大小和所在屏幕,都会保留到下次启动。**{{DockSaveAs}}** 会把它们与布局一起保存;加载该布局即可恢复。如果下次启动时缺少某个屏幕,窗口会移到主窗口所在的屏幕上。",
        "どのタブが別ウィンドウか、その位置・大きさ・画面は次回の起動まで保持されます。**{{DockSaveAs}}** はそれらをレイアウトと一緒に保存し、レイアウトを読み込むと元に戻ります。次回の起動時に画面が見つからない場合、ウィンドウはメインウィンドウの画面に移ります。",
        "Qué pestañas tienen ventana propia, su posición, tamaño y pantalla se conservan para el próximo inicio. **{{DockSaveAs}}** las guarda con la disposición; al cargar la disposición vuelven. Si en el próximo inicio falta una pantalla, la ventana pasa a la pantalla de la ventana principal.",
        "Které karty mají vlastní okno, jejich poloha, velikost a obrazovka zůstanou pro příští spuštění. **{{DockSaveAs}}** je uloží s rozvržením; načtení rozvržení je vrátí. Chybí-li při příštím spuštění obrazovka, okno se přesune na obrazovku hlavního okna."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def bloecke():
    """Die Bloecke des Abschnitts (fuer inhalt_dock.FENSTER_ANORDNEN)."""
    from inhalt_basis import T

    def t(k):
        return T(*X[k][:4])

    return [
        ("h2", "tabfenster", t("titel")),
        ("p", t("einleitung")),
        ("img", "h_tabfenster_menue", t("bild_menue"), "breit"),
        ("steps", [t("schritt1"), t("schritt2"), t("schritt3")]),
        ("img", "h_tabfenster_zwei", t("bild_zwei"), "breit"),
        ("list", [t("skripte"), t("undo"), t("dialoge"), t("ruhe"), t("qualitaet")]),
        ("img", "h_tabfenster_fenstermenue", t("bild_fenstermenue"), "breit"),
        ("h3", t("zurueck_titel")),
        ("list", [t("zurueck1"), t("zurueck2"), t("zurueck3")]),
        ("img", "h_tabfenster_zurueck", t("bild_zurueck"), "breit"),
        ("note", t("merken")),
    ]
