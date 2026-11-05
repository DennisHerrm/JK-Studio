"""rc610 TUT-GEO: das Tutorial "Ein Modell zu Brushes machen" (k-geometrie).

shank (05.11.): "wie benutzt man das model zu brush? gibt es dafuer ein Tutorial
bereits?" - es gab nur den Abschnitt "Geometrie zu Brushes" der Seite
karte_werkzeuge9. Das Tutorial fuehrt mit einem Beispielmodell (Kiste, Saeule
mit Sockel, offene Pflanzenwand; selbst erzeugt im Selbsttest) durch den Import.
Die Tour im Programm steht in inhalt_touren.py, der Selbsttest mit den Bildern in
gui/selbsttest_tutgeo.inc (BHED_KARTE_NUR=tutgeo, Bilder BHED_HB_NUR=tutgeo).

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in
inhalt_tabfenster.py: die ersten vier gehen in T(...), Spanisch und Tschechisch
nach inhalt_es.py / inhalt_cs.py (ES, CS). Die Seite entsteht erst in seite() -
T liest beim Anlegen die Tabellen der weiteren Sprachen, und die lesen ES/CS aus
dieser Datei.
"""

X = {
    "titel": (
        "Turn a model into brushes", "Ein Modell zu Brushes machen", "把模型变成画刷", "モデルをブラシにする",
        "Convertir un modelo en brushes", "Udělat z modelu brushe"),
    "kurz": (
        "Export a model from Blender, turn its parts into brushes or into a model with clip brushes in the map editor, and check the result.",
        "Exportiere ein Modell aus Blender, mach im Karten-Editor aus seinen Teilen Brushes oder ein Modell mit Clip-Brushes und prüfe das Ergebnis.",
        "从 Blender 导出模型,在地图编辑器中把它的部件变成画刷或带碰撞画刷的模型,并检查结果。",
        "Blender からモデルを書き出し、マップエディターでその部品をブラシか、クリップブラシ付きのモデルにして、結果を確認します。",
        "Exporta un modelo desde Blender, convierte sus partes en brushes o en un modelo con brushes clip en el editor de mapas y comprueba el resultado.",
        "Exportujte model z Blenderu, v editoru map z jeho částí udělejte brushe nebo model s clip brushi a zkontrolujte výsledek."),
    "meta": (
        "Map editor · six steps", "Karten-Editor · sechs Schritte", "地图编辑器 · 六个步骤", "マップエディター · 6 手順",
        "Editor de mapas · seis pasos", "Editor map · šest kroků"),
    "schritt1": (
        "**Prepare the model.** Build it in Blender at the size it should have in the game - the player is 64 units tall, and one Blender unit becomes one unit (or set **{{KeGiSkala}}** later). Every object becomes one part; close the solid ones (no holes) - only closed parts become solid brushes. Name the materials like the textures, for example `crate01`. Export with **File > Export** as .obj, .fbx or .glb/.gltf with the default settings (Y up). Then switch to **{{ViewMapEditor}}** in the mode bar.",
        "**Bereite das Modell vor.** Bau es in Blender in der Größe, die es im Spiel haben soll - der Spieler ist 64 Einheiten groß, und eine Blender-Einheit wird eine Einheit (oder stell später **{{KeGiSkala}}** ein). Jedes Objekt wird ein Teil; schließ die festen (ohne Löcher) - nur geschlossene Teile werden feste Brushes. Benenne die Materialien wie die Texturen, zum Beispiel `crate01`. Exportiere mit **File > Export** als .obj, .fbx oder .glb/.gltf mit den Vorgaben (Y oben). Dann schalte in der Modusleiste auf **{{ViewMapEditor}}**.",
        "**准备模型。** 在 Blender 中按它在游戏中应有的大小制作——玩家高 64 个单位,一个 Blender 单位就是一个单位(也可以之后设置 **{{KeGiSkala}}**)。每个物体成为一个部件;把实心的部件封闭(没有洞)——只有封闭的部件才会成为实心画刷。按纹理给材质命名,例如 `crate01`。用 **File > Export** 以默认设置(Y 轴向上)导出为 .obj、.fbx 或 .glb/.gltf。然后在模式栏中切换到 **{{ViewMapEditor}}**。",
        "**モデルを用意します。** Blender で、ゲーム内で必要な大きさで作ります。プレイヤーの高さは 64 ユニットで、Blender の 1 単位が 1 ユニットになります(あとで **{{KeGiSkala}}** を設定してもかまいません)。オブジェクトごとに 1 つの部品になります。固体にする部品は穴のない閉じた形にしてください。閉じた部品だけが固体のブラシになります。マテリアルにはテクスチャと同じ名前(例: `crate01`)を付けます。**File > Export** で既定の設定(Y 上)のまま .obj、.fbx、.glb/.gltf として書き出します。次にモードバーで **{{ViewMapEditor}}** に切り替えます。",
        "**Prepara el modelo.** Constrúyelo en Blender al tamaño que debe tener en el juego: el jugador mide 64 unidades, y una unidad de Blender se convierte en una unidad (o ajusta **{{KeGiSkala}}** después). Cada objeto se convierte en una parte; cierra las sólidas (sin agujeros): solo las partes cerradas se convierten en brushes sólidos. Nombra los materiales como las texturas, por ejemplo `crate01`. Exporta con **File > Export** como .obj, .fbx o .glb/.gltf con los ajustes predeterminados (Y arriba). Después cambia a **{{ViewMapEditor}}** en la barra de modos.",
        "**Připravte model.** Postavte ho v Blenderu ve velikosti, jakou má mít ve hře - hráč je vysoký 64 jednotek a jedna jednotka Blenderu se stane jednou jednotkou (nebo později nastavte **{{KeGiSkala}}**). Každý objekt se stane jednou částí; pevné části uzavřete (bez děr) - jen uzavřené části se stanou pevnými brushi. Materiály pojmenujte jako textury, například `crate01`. Exportujte přes **File > Export** jako .obj, .fbx nebo .glb/.gltf s výchozím nastavením (Y nahoru). Pak v liště režimů přepněte na **{{ViewMapEditor}}**."),
    "schritt2": (
        "**Open the import.** **{{KeLFile}} > {{KeGiMenue}}** and choose the file. The window **{{KeGiTitel}}** lists every part of the file with its triangles and whether it is closed. A .bsp chosen here opens like **{{KeLOpenBsp}}**.",
        "**Öffne den Import.** **{{KeLFile}} > {{KeGiMenue}}** und wähl die Datei. Das Fenster **{{KeGiTitel}}** listet jeden Teil der Datei mit seinen Dreiecken und ob er geschlossen ist. Eine hier gewählte .bsp öffnet sich wie mit **{{KeLOpenBsp}}**.",
        "**打开导入。** 选择 **{{KeLFile}} > {{KeGiMenue}}** 并选取文件。窗口 **{{KeGiTitel}}** 列出文件中的每个部件、它的三角形数量以及是否封闭。在这里选择 .bsp 时,会像 **{{KeLOpenBsp}}** 一样打开。",
        "**読み込みを開きます。** **{{KeLFile}} > {{KeGiMenue}}** でファイルを選びます。ウィンドウ **{{KeGiTitel}}** に、ファイルの各部品とその三角形の数、閉じているかどうかが並びます。ここで .bsp を選ぶと **{{KeLOpenBsp}}** と同じように開きます。",
        "**Abre la importación.** **{{KeLFile}} > {{KeGiMenue}}** y elige el archivo. La ventana **{{KeGiTitel}}** lista cada parte del archivo con sus triángulos y si está cerrada. Un .bsp elegido aquí se abre como con **{{KeLOpenBsp}}**.",
        "**Otevřete import.** **{{KeLFile}} > {{KeGiMenue}}** a vyberte soubor. Okno **{{KeGiTitel}}** vypíše každou část souboru s jejími trojúhelníky a s tím, zda je uzavřená. Soubor .bsp vybraný zde se otevře jako přes **{{KeLOpenBsp}}**."),
    "schritt3": (
        "**Choose for each part.** In the column **{{KeGiArt}}**: **{{KeGiAuto}}** (brushes for a closed part of up to 256 brushes, otherwise a model with clip brushes), **{{KeGiPinsel}}** or **{{KeGiModellClip}}**. The column **{{KeGiErgebnis}}** shows at once how many brushes it becomes and whether they are detail or structural. **{{KeGiVorsatz}}** goes in front of every material name: with `textures/yavin/` the material `crate01` becomes the texture `textures/yavin/crate01`. In the example the crate (48 units) becomes one detail brush, the pillar with its base two structural brushes (base and shaft), the open plant panel a model + clip.",
        "**Wähl für jeden Teil.** In der Spalte **{{KeGiArt}}**: **{{KeGiAuto}}** (Brushes für einen geschlossenen Teil mit bis zu 256 Brushes, sonst ein Modell mit Clip-Brushes), **{{KeGiPinsel}}** oder **{{KeGiModellClip}}**. Die Spalte **{{KeGiErgebnis}}** zeigt sofort, wie viele Brushes daraus werden und ob sie detail oder strukturell sind. **{{KeGiVorsatz}}** kommt vor jeden Materialnamen: Mit `textures/yavin/` wird das Material `crate01` zur Textur `textures/yavin/crate01`. Im Beispiel wird die Kiste (48 Einheiten) ein Detail-Brush, die Säule mit Sockel zwei strukturelle Brushes (Sockel und Schaft), die offene Pflanzenwand ein Modell + Clip.",
        "**为每个部件选择。** 在 **{{KeGiArt}}** 列中选择:**{{KeGiAuto}}**(封闭且不超过 256 个画刷的部件生成画刷,否则生成带碰撞画刷的模型)、**{{KeGiPinsel}}** 或 **{{KeGiModellClip}}**。**{{KeGiErgebnis}}** 列会立即显示它会变成多少个画刷,以及它们是 detail 还是结构画刷。**{{KeGiVorsatz}}** 会加在每个材质名前面:使用 `textures/yavin/` 时,材质 `crate01` 会成为纹理 `textures/yavin/crate01`。在示例中,箱子(48 个单位)成为一个 detail 画刷,带底座的柱子成为两个结构画刷(底座和柱身),开放的植物面板成为模型 + 碰撞。",
        "**部品ごとに選びます。** 列 **{{KeGiArt}}** で、**{{KeGiAuto}}**(閉じていて 256 個以下のブラシになる部品はブラシ、それ以外はクリップブラシ付きのモデル)、**{{KeGiPinsel}}**、**{{KeGiModellClip}}** のどれかを選びます。列 **{{KeGiErgebnis}}** に、いくつのブラシになるか、detail か構造かがすぐに表示されます。**{{KeGiVorsatz}}** は各マテリアル名の前に付きます。`textures/yavin/` なら、マテリアル `crate01` はテクスチャ `textures/yavin/crate01` になります。例では、箱(48 ユニット)が detail ブラシ 1 個、台座付きの柱が構造ブラシ 2 個(台座と柱身)、開いた植物のパネルがモデル + クリップになります。",
        "**Elige para cada parte.** En la columna **{{KeGiArt}}**: **{{KeGiAuto}}** (brushes para una parte cerrada de hasta 256 brushes; si no, un modelo con brushes clip), **{{KeGiPinsel}}** o **{{KeGiModellClip}}**. La columna **{{KeGiErgebnis}}** muestra al instante cuántos brushes salen y si son de detalle o estructurales. **{{KeGiVorsatz}}** va delante de cada nombre de material: con `textures/yavin/` el material `crate01` se convierte en la textura `textures/yavin/crate01`. En el ejemplo, la caja (48 unidades) se convierte en un brush de detalle, la columna con su base en dos brushes estructurales (base y fuste) y el panel abierto de plantas en un modelo + colisión.",
        "**Zvolte pro každou část.** Ve sloupci **{{KeGiArt}}**: **{{KeGiAuto}}** (brushe pro uzavřenou část s nejvýše 256 brushi, jinak model s clip brushi), **{{KeGiPinsel}}** nebo **{{KeGiModellClip}}**. Sloupec **{{KeGiErgebnis}}** hned ukáže, kolik brushů vznikne a zda jsou detail, nebo strukturální. **{{KeGiVorsatz}}** se přidá před každý název materiálu: s `textures/yavin/` se z materiálu `crate01` stane textura `textures/yavin/crate01`. V příkladu se bedna (48 jednotek) stane jedním detail brushem, sloup s podstavcem dvěma strukturálními brushi (podstavec a dřík) a otevřený panel s rostlinami modelem + clip."),
    "schritt4": (
        "**Insert.** **{{KeGiEinfuegen}}** puts everything into the map as one undo step - <kbd>Ctrl</kbd>+<kbd>Z</kbd> takes it all back. With **{{KeGiOptimierer}}** (on by default) the optimizer opens next and checks the map with the new brushes against the limits of the game.",
        "**Füge ein.** **{{KeGiEinfuegen}}** setzt alles als einen Undo-Schritt in die Karte - <kbd>Strg</kbd>+<kbd>Z</kbd> nimmt alles zurück. Mit **{{KeGiOptimierer}}** (vorgegeben an) öffnet sich danach der Optimierer und prüft die Karte mit den neuen Brushes gegen die Grenzen des Spiels.",
        "**插入。** **{{KeGiEinfuegen}}** 把所有内容作为一个撤销步骤放入地图——<kbd>Ctrl</kbd>+<kbd>Z</kbd> 会全部撤回。勾选 **{{KeGiOptimierer}}**(默认开启)时,随后会打开优化器,按游戏的限制检查加入新画刷后的地图。",
        "**挿入します。** **{{KeGiEinfuegen}}** ですべてを 1 回の元に戻す操作としてマップに入れます。<kbd>Ctrl</kbd>+<kbd>Z</kbd> で全部元に戻ります。**{{KeGiOptimierer}}**(既定でオン)なら、続いて最適化が開き、新しいブラシを含むマップをゲームの制限と照らし合わせます。",
        "**Inserta.** **{{KeGiEinfuegen}}** pone todo en el mapa como un solo paso de deshacer: <kbd>Ctrl</kbd>+<kbd>Z</kbd> lo retira todo. Con **{{KeGiOptimierer}}** (activado por defecto) se abre después el optimizador y comprueba el mapa con los brushes nuevos frente a los límites del juego.",
        "**Vložte.** **{{KeGiEinfuegen}}** vloží vše do mapy jako jeden krok zpět - <kbd>Ctrl</kbd>+<kbd>Z</kbd> vše vrátí. Se zapnutou volbou **{{KeGiOptimierer}}** (výchozí) se potom otevře optimalizátor a zkontroluje mapu s novými brushi vůči limitům hry."),
    "schritt5": (
        "**Check the result.** Sides on the surface of the model have its material, aligned from its UV; sides inside it, which nobody sees, are caulk. Small parts (under 64 units) and open parts are detail, so they do not cut the map into BSP leaves. A **{{KeGiModellClip}}** part is a misc_model with clip brushes around it; its .obj lies in the import folder next to the settings, never in the game folder - the console says where.",
        "**Prüf das Ergebnis.** Seiten auf der Oberfläche des Modells haben sein Material, ausgerichtet nach seiner UV; Seiten im Innern, die niemand sieht, sind caulk. Kleine Teile (unter 64 Einheiten) und offene Teile sind detail, damit sie die Karte nicht in BSP-Blätter zerschneiden. Ein Teil als **{{KeGiModellClip}}** ist ein misc_model mit Clip-Brushes darum; seine .obj liegt im Import-Ordner neben den Einstellungen, nie im Spielordner - die Konsole sagt, wo.",
        "**检查结果。** 位于模型表面的面使用模型的材质,并按其 UV 对齐;模型内部、谁也看不到的面是 caulk。小部件(小于 64 个单位)和开放部件是 detail,因此不会把地图切成 BSP 叶。作为 **{{KeGiModellClip}}** 的部件是一个 misc_model,周围有碰撞画刷;它的 .obj 位于设置旁的导入文件夹中,绝不在游戏文件夹里——控制台会告诉你位置。",
        "**結果を確認します。** モデルの表面にある面はそのマテリアルを持ち、UV に合わせて配置されます。内側の誰にも見えない面は caulk です。小さな部品(64 ユニット未満)と開いた部品は detail なので、マップを BSP のリーフに切り分けません。**{{KeGiModellClip}}** の部品は、周りにクリップブラシのある misc_model です。その .obj は設定の隣の読み込みフォルダーにあり、ゲームフォルダーには入りません。場所はコンソールに表示されます。",
        "**Comprueba el resultado.** Las caras en la superficie del modelo llevan su material, alineado según su UV; las caras de dentro, que nadie ve, son caulk. Las partes pequeñas (menos de 64 unidades) y las abiertas son de detalle, para que no corten el mapa en hojas BSP. Una parte como **{{KeGiModellClip}}** es un misc_model con brushes clip alrededor; su .obj está en la carpeta de importación junto a los ajustes, nunca en la carpeta del juego: la consola dice dónde.",
        "**Zkontrolujte výsledek.** Stěny na povrchu modelu mají jeho materiál, zarovnaný podle jeho UV; stěny uvnitř, které nikdo nevidí, jsou caulk. Malé části (pod 64 jednotek) a otevřené části jsou detail, takže mapu nerozřežou na listy BSP. Část jako **{{KeGiModellClip}}** je misc_model s clip brushi kolem; jeho .obj leží ve složce importu vedle nastavení, nikdy ve složce hry - konzole řekne kde."),
    "schritt6": (
        "**Build and look at it in the game.** **{{KeLBuild}}** compiles the map as in [[tut-karte]]; **{{KeLBuild}} > {{KeStMenue}}** starts the game with it. {{KeStMenue}} and **{{KePkMenue}}** pack the import folder into the archive, so the game also finds the model of a {{KeGiModellClip}} part.",
        "**Bau und sieh es dir im Spiel an.** **{{KeLBuild}}** kompiliert die Karte wie in [[tut-karte]]; **{{KeLBuild}} > {{KeStMenue}}** startet das Spiel damit. {{KeStMenue}} und **{{KePkMenue}}** packen den Import-Ordner mit ins Archiv, damit das Spiel auch das Modell eines Teils als {{KeGiModellClip}} findet.",
        "**编译并在游戏中查看。** **{{KeLBuild}}** 像 [[tut-karte]] 中那样编译地图;**{{KeLBuild}} > {{KeStMenue}}** 用它启动游戏。{{KeStMenue}} 和 **{{KePkMenue}}** 会把导入文件夹一起打包进存档,这样游戏也能找到作为 {{KeGiModellClip}} 的部件的模型。",
        "**ビルドしてゲームで見ます。** **{{KeLBuild}}** で [[tut-karte]] と同じようにマップをコンパイルし、**{{KeLBuild}} > {{KeStMenue}}** でそのマップを使ってゲームを起動します。{{KeStMenue}} と **{{KePkMenue}}** は読み込みフォルダーもアーカイブに入れるので、{{KeGiModellClip}} の部品のモデルもゲームが見つけられます。",
        "**Compila y míralo en el juego.** **{{KeLBuild}}** compila el mapa como en [[tut-karte]]; **{{KeLBuild}} > {{KeStMenue}}** inicia el juego con él. {{KeStMenue}} y **{{KePkMenue}}** meten la carpeta de importación en el archivo, para que el juego encuentre también el modelo de una parte como {{KeGiModellClip}}.",
        "**Sestavte mapu a podívejte se na ni ve hře.** **{{KeLBuild}}** zkompiluje mapu jako v [[tut-karte]]; **{{KeLBuild}} > {{KeStMenue}}** s ní spustí hru. {{KeStMenue}} a **{{KePkMenue}}** zabalí do archivu i složku importu, aby hra našla i model části jako {{KeGiModellClip}}."),
    "tipp_bsp": (
        "**A compiled map (.bsp)** opens with **{{KeLFile}} > {{KeLOpenBsp}}**: the models it recognizes come back as misc_model, not as brushes. **{{KeRkFlaechenOpt}}** keeps the faces of models it does not recognize as thin brushes in the layer **{{KeRkEbene}}** - see [[karte_oeffnen#modelle]].",
        "**Eine fertige Karte (.bsp)** öffnet **{{KeLFile}} > {{KeLOpenBsp}}**: Die Modelle, die es erkennt, kommen als misc_model zurück, nicht als Brushes. **{{KeRkFlaechenOpt}}** behält die Flächen nicht erkannter Modelle als dünne Brushes in der Ebene **{{KeRkEbene}}** - siehe [[karte_oeffnen#modelle]].",
        "**已编译的地图(.bsp)** 用 **{{KeLFile}} > {{KeLOpenBsp}}** 打开:能识别的模型会作为 misc_model 恢复,而不是画刷。**{{KeRkFlaechenOpt}}** 会把未识别模型的面作为薄画刷保留在图层 **{{KeRkEbene}}** 中——参见 [[karte_oeffnen#modelle]]。",
        "**コンパイル済みのマップ(.bsp)** は **{{KeLFile}} > {{KeLOpenBsp}}** で開きます。認識できたモデルはブラシではなく misc_model として戻ります。**{{KeRkFlaechenOpt}}** は、認識できなかったモデルの面を薄いブラシとしてレイヤー **{{KeRkEbene}}** に残します。[[karte_oeffnen#modelle]] を参照してください。",
        "**Un mapa compilado (.bsp)** se abre con **{{KeLFile}} > {{KeLOpenBsp}}**: los modelos que reconoce vuelven como misc_model, no como brushes. **{{KeRkFlaechenOpt}}** conserva las caras de los modelos no reconocidos como brushes finos en la capa **{{KeRkEbene}}**; consulta [[karte_oeffnen#modelle]].",
        "**Zkompilovaná mapa (.bsp)** se otevře přes **{{KeLFile}} > {{KeLOpenBsp}}**: rozpoznané modely se vrátí jako misc_model, ne jako brushe. **{{KeRkFlaechenOpt}}** ponechá stěny nerozpoznaných modelů jako tenké brushe ve vrstvě **{{KeRkEbene}}** - viz [[karte_oeffnen#modelle]]."),
    "tipp_spiel": (
        "**Game models (.md3, .glm)** cannot be imported as brushes - the import reads .obj, .fbx, .glb and .gltf. Put them into the map as misc_model, for example from the [[karte_werkzeuge9#assets|{{KeAbMenue}}]].",
        "**Spielmodelle (.md3, .glm)** lassen sich nicht als Brushes importieren - der Import liest .obj, .fbx, .glb und .gltf. Setz sie als misc_model in die Karte, zum Beispiel aus dem [[karte_werkzeuge9#assets|{{KeAbMenue}}]].",
        "**游戏模型(.md3、.glm)** 不能作为画刷导入——导入只读取 .obj、.fbx、.glb 和 .gltf。请把它们作为 misc_model 放进地图,例如从 [[karte_werkzeuge9#assets|{{KeAbMenue}}]] 中拖入。",
        "**ゲームのモデル(.md3、.glm)** はブラシとして読み込めません。読み込めるのは .obj、.fbx、.glb、.gltf です。たとえば [[karte_werkzeuge9#assets|{{KeAbMenue}}]] から、misc_model としてマップに置いてください。",
        "**Los modelos del juego (.md3, .glm)** no se pueden importar como brushes: la importación lee .obj, .fbx, .glb y .gltf. Ponlos en el mapa como misc_model, por ejemplo desde el [[karte_werkzeuge9#assets|{{KeAbMenue}}]].",
        "**Herní modely (.md3, .glm)** nelze importovat jako brushe - import čte .obj, .fbx, .glb a .gltf. Vložte je do mapy jako misc_model, například z [[karte_werkzeuge9#assets|{{KeAbMenue}}]]."),
    "tipp_ziehen": (
        "Just the model, without brushes: drag the file onto the map editor ([[karte_werkzeuge9#ziehen]]). Back to Blender: **{{KeLFile}} > {{KeBlMenue}}** ([[karte_werkzeuge9#blender]]).",
        "Nur das Modell, ohne Brushes: Zieh die Datei auf den Karten-Editor ([[karte_werkzeuge9#ziehen]]). Zurück zu Blender: **{{KeLFile}} > {{KeBlMenue}}** ([[karte_werkzeuge9#blender]]).",
        "只要模型、不要画刷:把文件拖到地图编辑器上([[karte_werkzeuge9#ziehen]])。返回 Blender:**{{KeLFile}} > {{KeBlMenue}}**([[karte_werkzeuge9#blender]])。",
        "ブラシなしでモデルだけを使うには、ファイルをマップエディターにドラッグします([[karte_werkzeuge9#ziehen]])。Blender に戻すには **{{KeLFile}} > {{KeBlMenue}}**([[karte_werkzeuge9#blender]])。",
        "Solo el modelo, sin brushes: arrastra el archivo al editor de mapas ([[karte_werkzeuge9#ziehen]]). De vuelta a Blender: **{{KeLFile}} > {{KeBlMenue}}** ([[karte_werkzeuge9#blender]]).",
        "Jen model, bez brushů: přetáhněte soubor na editor map ([[karte_werkzeuge9#ziehen]]). Zpět do Blenderu: **{{KeLFile}} > {{KeBlMenue}}** ([[karte_werkzeuge9#blender]])."),
    "weiter": (
        "More in [[karte_werkzeuge9#geometrie]].", "Mehr in [[karte_werkzeuge9#geometrie]].", "更多内容见 [[karte_werkzeuge9#geometrie]]。",
        "詳しくは [[karte_werkzeuge9#geometrie]]。", "Más en [[karte_werkzeuge9#geometrie]].", "Více v [[karte_werkzeuge9#geometrie]]."),
    # Querverweis von "Karten-Editor: Werkzeuge 9", Abschnitt Geometrie zu Brushes
    "verweis": (
        "Step by step with an example model: [[k-geometrie]].", "Schritt für Schritt mit einem Beispielmodell: [[k-geometrie]].",
        "带示例模型的分步说明:[[k-geometrie]]。", "サンプルモデルを使った手順: [[k-geometrie]]。",
        "Paso a paso con un modelo de ejemplo: [[k-geometrie]].", "Krok za krokem s ukázkovým modelem: [[k-geometrie]]."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def seite():
    """Die Tutorialseite k-geometrie (Gruppe Tutorials, bei den Karten-Tutorials)."""
    import inhalt_hilfe2
    import inhalt_kurz
    s = inhalt_kurz.tut(
        "k-geometrie", "tut", _t("titel"), _t("kurz"), _t("meta"), "h_tut_geo_5",
        [_t("schritt%d" % i) for i in range(1, 7)],
        [_t("tipp_bsp"), _t("tipp_spiel"), _t("tipp_ziehen")],
        weiter=_t("weiter"))
    # jeder Schritt mit seinem Bild aus dem Programm: die Tour an diesem Schritt (h_tut_geo_<n>)
    inhalt_hilfe2._schrittbilder(s, "h_tut_geo")
    return s


def einhaengen(werkzeuge9, tutorials):
    """Querverweise: der Abschnitt Geometrie zu Brushes verweist auf das Tutorial, die
    Uebersicht der Tutorials fuehrt es bei den Karten-Tutorials."""
    b = werkzeuge9["bloecke"]
    i = next(n for n, x in enumerate(b) if x[0] == "h2" and x[1] == "geometrie")
    i = next(n for n in range(i + 1, len(b)) if b[n][0] == "list")
    b.insert(i + 1, ("note", _t("verweis")))
    for x in tutorials["bloecke"]:
        if x[0] == "index":
            for _titel, ids in x[1]:
                if "tut-karte-tuer" in ids:
                    ids.append("k-geometrie")
        if x[0] == "cards" and "tut-karte-tuer" in x[1]:
            x[1].append("k-geometrie")
