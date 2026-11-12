"""FIX612B (Funde der Code-Pruefung P2, Entscheidungen A13-A17): Abschnitt "Speichern, Detailstufe und Strg+Z beim Malen" auf
der Seite "Model-Tab: Weights" - Speichern schreibt, was die Ansicht zeigt (die Mallisten bleiben offen); Auto-Skin "Nur Auswahl"
spiegelt nur in gewaehlte Flaechen; Weights gelten der gezeigten Stufe, die waehrend eines Strichs oder Zugs stehen bleibt; die
Auswahl wird je Stufe gemerkt; Strg+Z mitten im Strich beendet ihn und nimmt ihn zurueck, Strg+Y bringt ihn wieder.

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_mb1a2.py.
"""
from inhalt_basis import T

TITEL = ("Saving, level of detail and Ctrl+Z while painting", "Speichern, Detailstufe und Strg+Z beim Malen", "保存、细节级别和绘制时的 Ctrl+Z",
         "保存、詳細レベル、ペイント中の Ctrl+Z", "Guardar, nivel de detalle y Ctrl+Z al pintar", "Uložení, úroveň detailu a Ctrl+Z při malování")

SPEICHERN = (
    "Saving writes the weights the view shows, also while paint lists are open (after painting or a command of this area, before "
    "**{{MbGwUebernehmen}}**): the file gets the four weights per vertex that the pose shows, normalized like the game reads them, and "
    "the paint lists stay open for further painting. The check before saving and the JKA limits in Modeling measure the same. With "
    "**Selected surfaces only** and **Mirror afterwards**, Auto-Skin mirrors only into the selected surfaces: to mirror one arm onto the other, select both.",
    "Speichern schreibt die Gewichte, die die Ansicht zeigt, auch solange Mallisten offen sind (nach dem Malen oder einem Befehl dieses "
    "Bereichs, vor **{{MbGwUebernehmen}}**): Die Datei bekommt die vier Gewichte je Ecke, die die Pose zeigt, normiert, wie das Spiel sie "
    "liest, und die Mallisten bleiben zum Weitermalen offen. Die Prüfung vor dem Speichern und die JKA-Grenzen im Modellieren messen dasselbe. "
    "Mit **Nur ausgewählte Flächen** und **Danach spiegeln** spiegelt Auto-Skin nur in die gewählten Flächen: um einen Arm auf den anderen zu spiegeln, beide wählen.",
    "即使绘制列表仍处于打开状态(绘制或执行本区域的命令之后、**{{MbGwUebernehmen}}** 之前),保存也会写入视图显示的权重:文件得到姿势所显示的"
    "每个顶点的四个权重,按游戏读取的方式归一化,绘制列表保持打开以便继续绘制。保存前的检查和建模中的 JKA 限制测量的也是同样的内容。"
    "勾选 **仅所选表面** 和 **之后镜像** 时,自动蒙皮只镜像到所选的面:要把一只手臂镜像到另一只,请两只都选中。",
    "ペイントリストが開いたまま(ペイントやこのエリアのコマンドの後、**{{MbGwUebernehmen}}** の前)でも、保存するとビューに表示されている"
    "ウェイトが書き込まれます。ファイルにはポーズに表示されている頂点ごとの 4 つのウェイトが、ゲームが読む形に正規化されて入り、"
    "ペイントリストは続けて塗れるよう開いたままです。保存前のチェックとモデリングの JKA 制限も同じものを測ります。"
    "**選択したサーフェスのみ** と **あとでミラー** では、オートスキンは選択したサーフェスにだけミラーします。片方の腕をもう片方にミラーするには両方を選択します。",
    "Guardar escribe los pesos que muestra la vista, también mientras hay listas de pintura abiertas (tras pintar o un comando de esta "
    "área, antes de **{{MbGwUebernehmen}}**): el archivo recibe los cuatro pesos por vértice que muestra la pose, normalizados como los "
    "lee el juego, y las listas de pintura siguen abiertas para seguir pintando. La comprobación antes de guardar y los límites de JKA "
    "en Modelado miden lo mismo. Con **Solo las superficies seleccionadas** y **Reflejar después**, Auto-Skin refleja solo en las superficies seleccionadas: para "
    "reflejar un brazo en el otro, selecciona ambos.",
    "Uložení zapíše váhy, které ukazuje pohled, i když jsou seznamy malování otevřené (po malování nebo příkazu této oblasti, před "
    "**{{MbGwUebernehmen}}**): soubor dostane čtyři váhy na vrchol, které ukazuje póza, normalizované tak, jak je čte hra, a seznamy "
    "malování zůstanou otevřené pro další malování. Kontrola před uložením a limity JKA v modelování měří totéž. S volbami **Jen vybrané povrchy** "
    "a **Potom zrcadlit** zrcadlí Auto-Skin jen do vybraných ploch: chcete-li zrcadlit jednu paži na druhou, vyberte obě.",
)

STUFE = (
    "Painting and the commands of this area apply to the level of detail shown (with **{{MbLodAuto}}** it follows the zoom); the other levels "
    "keep their weights - only Auto-Skin weights all levels. While a stroke, a gradient, a drag or a tool box is open, the view keeps "
    "the level it began in, even if the wheel zooms; the new level follows when you let go. A selection of vertices, edges or polygons "
    "belongs to its level: after a change of level it is empty, and it is back when the view returns to that level and its mesh was not "
    "changed in between (painting weights does not count). Ctrl+Z with the mouse button held ends the open stroke, gradient or drag and "
    "takes it back; Ctrl+Y brings it back.",
    "Malen und die Befehle dieses Bereichs gelten der gezeigten Detailstufe (mit **{{MbLodAuto}}** folgt sie dem Zoom); die anderen Stufen "
    "behalten ihre Gewichte - nur Auto-Skin gewichtet alle Stufen. Solange ein Strich, ein Verlauf, ein Zug oder ein Werkzeugkästchen "
    "offen ist, behält die Ansicht die Stufe, in der er begann, auch wenn das Rad zoomt; die neue Stufe folgt nach dem Loslassen. Eine "
    "Auswahl von Ecken, Kanten oder Vielecken gehört zu ihrer Stufe: Nach einem Stufenwechsel ist sie leer, und sie ist wieder da, wenn "
    "die Ansicht zu dieser Stufe zurückkehrt und ihr Netz inzwischen nicht geändert wurde (Gewichte malen zählt nicht). Strg+Z bei "
    "gehaltener Maustaste beendet den offenen Strich, Verlauf oder Zug und nimmt ihn zurück; Strg+Y bringt ihn wieder.",
    "绘制和本区域的命令作用于显示的细节级别(选 **{{MbLodAuto}}** 时随缩放变化);其他级别保留各自的权重——只有自动蒙皮会为所有级别加权。当笔划、渐变、拖动或工具框"
    "处于打开状态时,即使滚轮缩放,视图也保持开始时的级别;松开后才切换到新级别。顶点、边或多边形的选择属于其级别:切换级别后选择为空,当视图回到该级别且"
    "期间该级别的网格未被更改时(绘制权重不算),选择会恢复。按住鼠标键时按 Ctrl+Z 会结束打开的笔划、渐变或拖动并将其撤销;Ctrl+Y 可恢复。",
    "ペイントとこのエリアのコマンドは表示中の詳細レベルに適用されます(**{{MbLodAuto}}** ではズームに従います)。ほかのレベルはウェイトを保ちます。"
    "すべてのレベルにウェイトを付けるのはオートスキンだけです。ストローク、グラデーション、ドラッグ、ツールボックスが開いている間は、ホイールで"
    "ズームしてもビューは開始時のレベルを保ち、新しいレベルは離した後に切り替わります。頂点・エッジ・ポリゴンの選択はそのレベルに属します。"
    "レベルが変わると選択は空になり、ビューがそのレベルに戻り、その間にそのレベルのメッシュが変更されていなければ元に戻ります(ウェイトのペイントは"
    "数えません)。マウスボタンを押したまま Ctrl+Z を押すと、開いているストローク、グラデーション、ドラッグを終えて元に戻します。Ctrl+Y でやり直せます。",
    "Pintar y los comandos de esta área se aplican al nivel de detalle mostrado (con **{{MbLodAuto}}** sigue al zoom); los demás niveles "
    "conservan sus pesos: solo Auto-Skin pondera todos los niveles. Mientras hay un trazo, un degradado, un arrastre o un cuadro de "
    "herramienta abierto, la vista mantiene el nivel en el que empezó aunque la rueda haga zoom; el nuevo nivel llega al soltar. Una "
    "selección de vértices, aristas o polígonos pertenece a su nivel: tras un cambio de nivel está vacía, y vuelve cuando la vista regresa "
    "a ese nivel y su malla no ha cambiado entretanto (pintar pesos no cuenta). Ctrl+Z con el botón del ratón pulsado termina el trazo, "
    "degradado o arrastre abierto y lo deshace; Ctrl+Y lo rehace.",
    "Malování a příkazy této oblasti platí pro zobrazenou úroveň detailu (s volbou **{{MbLodAuto}}** sleduje přiblížení); ostatní úrovně si "
    "ponechají své váhy - jen Auto-Skin váží všechny úrovně. Dokud je otevřený tah, přechod, tažení nebo okénko nástroje, ponechá pohled "
    "úroveň, ve které začal, i když kolečko přibližuje; nová úroveň přijde po puštění. Výběr vrcholů, hran nebo mnohoúhelníků patří ke své "
    "úrovni: po změně úrovně je prázdný a vrátí se, když se pohled vrátí na tuto úroveň a její síť se mezitím nezměnila (malování vah se "
    "nepočítá). Ctrl+Z při držení tlačítka myši ukončí otevřený tah, přechod nebo tažení a vrátí ho; Ctrl+Y ho obnoví.",
)


def bloecke():
    """Ueberschrift und zwei Absaetze fuer inhalt_mb1a2.py (Seite modell-weights, nach "Gewichte, die das Spiel lesen kann")."""
    return [("h2", "speichern", T(*TITEL[:4])), ("p", T(*SPEICHERN[:4])), ("p", T(*STUFE[:4]))]


ES = {TITEL[0]: TITEL[4], SPEICHERN[0]: SPEICHERN[4], STUFE[0]: STUFE[4]}
CS = {TITEL[0]: TITEL[5], SPEICHERN[0]: SPEICHERN[5], STUFE[0]: STUFE[5]}
