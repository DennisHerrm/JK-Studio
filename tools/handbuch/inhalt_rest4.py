"""REST4: Hinweise auf der Seite "Die Karte" - was das Spiel vor der Welt laedt (Oberflaeche, HUD, NPC-Klassen, Fahrzeuge), wie
das Original-Jedi-Academy Bilddateien sucht (kein eigenes Regelwerk) und Haeute Wort fuer Wort wie die Engine.

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_rest3.py.
"""
from inhalt_basis import T

BILDER = (
    "Before the map itself the preview loads what a freshly started game loads first: the menus and the HUD, the fonts, the effects "
    "of the NPC classes and vehicles on the map and the weapons of its characters. A picture that the menus load without mipmaps "
    "keeps that on a wall that uses the same picture, as in the game. A sky shader whose picture stage is missing draws no sky but "
    "the grey default box, like the game. The original Jedi Academy (jasp.exe, jamp.exe) looks for picture files slightly "
    "differently than OpenJK and Movie Duels: it always tries .jpg, .png, .tga in this order and ignores the extension the shader "
    "names, and it cuts the name at its first dot. The preview follows OpenJK; if a picture exists twice with different "
    "extensions, the original game may show the other one.",
    "Vor der Karte selbst lädt die Vorschau, was ein frisch gestartetes Spiel zuerst lädt: die Menüs und das HUD, die Schriften, "
    "die Effekte der NPC-Klassen und Fahrzeuge der Karte und die Waffen ihrer Figuren. Ein Bild, das die Menüs ohne Mip-Stufen "
    "laden, bleibt so auch an einer Wand mit demselben Bild, wie im Spiel. Ein Himmelsshader, dem ein Stufenbild fehlt, zeichnet "
    "keinen Himmel, sondern den grauen Standardkasten, wie das Spiel. Das Original-Jedi-Academy (jasp.exe, jamp.exe) sucht "
    "Bilddateien etwas anders als OpenJK und Movie Duels: Es versucht immer .jpg, .png, .tga in dieser Reihenfolge, übergeht die "
    "Endung, die der Shader nennt, und schneidet den Namen am ersten Punkt ab. Die Vorschau folgt OpenJK; liegt ein Bild mit "
    "zwei Endungen vor, kann das Originalspiel das andere zeigen.",
    "在地图本身之前,预览会先加载刚启动的游戏最先加载的内容:菜单和 HUD、字体、地图上 NPC 类别和载具的特效,以及其角色的武器。"
    "菜单以无 mipmap 方式加载的图像,在使用同一图像的墙面上也保持如此,与游戏相同。缺少阶段图像的天空着色器不绘制天空,而是绘制"
    "灰色的默认方框,与游戏相同。原版 Jedi Academy (jasp.exe、jamp.exe) 查找图像文件的方式与 OpenJK 和 Movie Duels 略有不同:"
    "它总是按 .jpg、.png、.tga 的顺序尝试,忽略着色器指定的扩展名,并在第一个点处截断名称。预览遵循 OpenJK;如果同一图像有两个"
    "扩展名,原版游戏可能显示另一个。",
    "マップ本体の前に、プレビューは起動したばかりのゲームが最初に読み込むものを読み込みます。メニューと HUD、フォント、マップ上の "
    "NPC クラスと乗り物のエフェクト、そしてキャラクターの武器です。メニューがミップマップなしで読み込んだ画像は、同じ画像を使う壁でも"
    "ゲームと同じくそのままです。ステージの画像が欠けたスカイシェーダーは空を描かず、ゲームと同じく灰色の既定のボックスを描きます。"
    "オリジナルの Jedi Academy (jasp.exe、jamp.exe) は OpenJK や Movie Duels と少し違う方法で画像ファイルを探します。常に .jpg、"
    ".png、.tga の順に試し、シェーダーが指定する拡張子を無視し、名前を最初のドットで切ります。プレビューは OpenJK に従います。"
    "同じ画像が 2 つの拡張子で存在する場合、オリジナルのゲームはもう一方を表示することがあります。",
    "Antes del mapa en sí, la vista previa carga lo que un juego recién iniciado carga primero: los menús y el HUD, las fuentes, los "
    "efectos de las clases de NPC y vehículos del mapa y las armas de sus personajes. Una imagen que los menús cargan sin mipmaps se "
    "queda así también en una pared que usa la misma imagen, como en el juego. Un shader de cielo al que le falta la imagen de una "
    "etapa no dibuja cielo sino la caja gris por defecto, como el juego. El Jedi Academy original (jasp.exe, jamp.exe) busca los "
    "archivos de imagen de forma algo distinta a OpenJK y Movie Duels: siempre prueba .jpg, .png, .tga en este orden, ignora la "
    "extensión que nombra el shader y corta el nombre en su primer punto. La vista previa sigue a OpenJK; si una imagen existe con "
    "dos extensiones, el juego original puede mostrar la otra.",
    "Před samotnou mapou náhled načte to, co čerstvě spuštěná hra načítá jako první: nabídky a HUD, písma, efekty tříd NPC a vozidel "
    "na mapě a zbraně jejích postav. Obrázek, který nabídky načtou bez mipmap, zůstane takový i na stěně se stejným obrázkem, jako "
    "ve hře. Shader oblohy, kterému chybí obrázek fáze, nekreslí oblohu, ale šedý výchozí rámeček, jako hra. Původní Jedi Academy "
    "(jasp.exe, jamp.exe) hledá soubory obrázků trochu jinak než OpenJK a Movie Duels: vždy zkouší .jpg, .png, .tga v tomto pořadí, "
    "přehlíží příponu, kterou shader uvádí, a zkracuje název u první tečky. Náhled se řídí OpenJK; existuje-li obrázek se dvěma "
    "příponami, původní hra může zobrazit ten druhý.",
)

HAEUTE = (
    "Skins (.skin) are read word for word like the engine: a line with a tag_ surface and a shader, or a line without a comma, "
    "shifts all following words, so the surfaces after it get no skin entry - as in the game. Under the rules of Jedi Outcast a "
    "skin only sets the shaders like jk2sp.exe: \"*off\" does not switch a surface off (it gets a shader without a picture and "
    "draws nothing), a surface the skin does not name draws the grey default box, and there are no three-part skins.",
    "Häute (.skin) werden Wort für Wort wie in der Engine gelesen: Eine Zeile mit einer tag_-Fläche und einem Shader oder eine Zeile "
    "ohne Komma verschiebt alle folgenden Wörter, die Flächen danach bekommen dann keinen Eintrag - wie im Spiel. Unter den Regeln "
    "von Jedi Outcast setzt eine Haut wie jk2sp.exe nur die Shader: \"*off\" schaltet keine Fläche ab (sie bekommt einen Shader "
    "ohne Bild und zeichnet nichts), eine Fläche, die die Haut nicht nennt, zeichnet den grauen Standardkasten, und dreiteilige "
    "Häute gibt es nicht.",
    "皮肤 (.skin) 与引擎一样逐词读取:含有 tag_ 表面和着色器的一行,或没有逗号的一行,会使后面所有的词错位,此后的表面就得不到条目"
    "——与游戏相同。在 Jedi Outcast 规则下,皮肤与 jk2sp.exe 一样只设置着色器:\"*off\" 不会关闭表面(它得到一个没有图像的着色器,"
    "什么也不绘制),皮肤未提及的表面绘制灰色的默认方框,也没有三部分皮肤。",
    "スキン (.skin) はエンジンと同じく 1 語ずつ読み込まれます。tag_ サーフェスとシェーダーを含む行や、カンマのない行は後続のすべての"
    "語をずらすため、その後のサーフェスにはエントリがなくなります。ゲームと同じです。Jedi Outcast のルールでは、スキンは jk2sp.exe "
    "と同じくシェーダーだけを設定します。\"*off\" はサーフェスをオフにせず(画像のないシェーダーになり何も描きません)、スキンが"
    "指定しないサーフェスは灰色の既定のボックスを描き、3 部構成のスキンはありません。",
    "Las pieles (.skin) se leen palabra por palabra como el motor: una línea con una superficie tag_ y un shader, o una línea sin "
    "coma, desplaza todas las palabras siguientes, y las superficies posteriores se quedan sin entrada, como en el juego. Con las "
    "reglas de Jedi Outcast una piel solo fija los shaders como jk2sp.exe: \"*off\" no apaga una superficie (recibe un shader sin "
    "imagen y no dibuja nada), una superficie que la piel no nombra dibuja la caja gris por defecto y no hay pieles de tres partes.",
    "Skiny (.skin) se čtou slovo po slově jako v enginu: řádek s plochou tag_ a shaderem nebo řádek bez čárky posune všechna "
    "následující slova, takže plochy za ním nedostanou žádný záznam, jako ve hře. Podle pravidel Jedi Outcast skin jako jk2sp.exe "
    "jen nastavuje shadery: \"*off\" plochu nevypne (dostane shader bez obrázku a nic nekreslí), plocha, kterou skin neuvádí, "
    "kreslí šedý výchozí rámeček a trojdílné skiny neexistují.",
)


def bilder_notiz():
    return ("note", T(*BILDER[:4]))


def haeute_notiz():
    return ("note", T(*HAEUTE[:4]))


ES = {BILDER[0]: BILDER[4], HAEUTE[0]: HAEUTE[4]}
CS = {BILDER[0]: BILDER[5], HAEUTE[0]: HAEUTE[5]}
