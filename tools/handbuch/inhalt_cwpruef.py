"""CW-PRUEF: die Pruefungen der Clone-Wars-Kette im Optimierer (Sicht gegen Kollision, fast deckungsgleiche Flaechen, Stufen, Tueren
und Gaenge, Texturen, Modelle, Requisiten) und der JSON-Bericht mit Koordinaten.

Haengt an die Seite karte_optimierer (inhalt_karte5.py): die Zeilen der Prueftabelle, einen Abschnitt nach der Tabelle und einen
Absatz zu den Einstellungen. Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_rest8.py.
"""

X = {
    "titel": (
        "Sight, collision and sizes", "Sicht, Kollision und Maße", "视觉、碰撞与尺寸", "見た目・コリジョン・寸法",
        "Vista, colisión y medidas", "Vzhled, kolize a rozměry"),
    "kurz": (
        "These checks come from the requirements of a Clone Wars mod for Jedi Academy and Movie Duels: what the player sees must be what he "
        "collides with, steps and passages must fit the player, and textures and models must load in the game. The player's size comes from "
        "the engine (OpenJK: 32 wide, 64 high, crouched 40, STEPSIZE 18; multiplayer 30 wide); the sizes of the list are adjustable.",
        "Diese Prüfungen stammen aus den Anforderungen eines Clone-Wars-Mods für Jedi Academy und Movie Duels: was man sieht, muss das "
        "sein, woran man stößt, Stufen und Durchgänge müssen zur Spielfigur passen, und Texturen und Modelle müssen im Spiel laden. Die "
        "Größe der Spielfigur kommt aus der Engine (OpenJK: 32 breit, 64 hoch, geduckt 40, STEPSIZE 18; Mehrspieler 30 breit); die Maße "
        "der Liste sind einstellbar.",
        "这些检查来自一个面向 Jedi Academy 和 Movie Duels 的 Clone Wars 模组的需求:看到的就必须是碰到的,台阶和通道必须适合玩家,纹理和模型必须"
        "能在游戏中加载。玩家尺寸取自引擎(OpenJK:宽 32、高 64、蹲下 40、STEPSIZE 18;多人模式宽 30);清单中的尺寸可以调整。",
        "これらのチェックは、Jedi Academy と Movie Duels 向けの Clone Wars MOD の要件から来ています。見えるものがぶつかるものと一致し、段差と"
        "通路がプレイヤーに合い、テクスチャとモデルがゲームで読み込めることが求められます。プレイヤーの大きさはエンジンから取ります(OpenJK: "
        "幅 32、高さ 64、しゃがみ 40、STEPSIZE 18。マルチプレイは幅 30)。リストの寸法は変更できます。",
        "Estas comprobaciones vienen de los requisitos de un mod de Clone Wars para Jedi Academy y Movie Duels: lo que se ve debe ser con lo "
        "que se choca, los escalones y los pasos deben ajustarse al jugador, y las texturas y los modelos deben cargar en el juego. El tamaño "
        "del jugador viene del motor (OpenJK: 32 de ancho, 64 de alto, agachado 40, STEPSIZE 18; multijugador 30 de ancho); las medidas de "
        "la lista son ajustables.",
        "Tyto kontroly pocházejí z požadavků módu Clone Wars pro Jedi Academy a Movie Duels: co je vidět, musí být to, do čeho se naráží, "
        "schody a průchody musí odpovídat postavě a textury a modely se musí ve hře načíst. Velikost postavy pochází z enginu (OpenJK: 32 "
        "široká, 64 vysoká, přikrčená 40, STEPSIZE 18; multiplayer 30 široká); rozměry ze seznamu jsou nastavitelné."),
    "kollision": (
        "**Collision is what the game uses:** the optimizer builds it like q3map2 - each brush carries the contents of one shader "
        "(liquid before fog before playerclip before nonsolid, so one nonsolid side makes the whole brush passable), patches with a solid "
        "shader, the triangles of a misc_model with a clip spawnflag, clip brushes (common/clip, system/clip, playerclip). Visible means "
        "drawn and not see-through (no glass, no alpha test, no liquid).",
        "**Kollision ist, was das Spiel benutzt:** der Optimierer baut sie wie q3map2 - jeder Pinsel trägt die Inhalte eines Shaders "
        "(Flüssigkeit vor Nebel vor playerclip vor nonsolid; eine nicht feste Seite macht also den ganzen Pinsel durchlässig), Patches mit "
        "festem Shader, die Dreiecke eines misc_model mit Clip-Spawnflag, Clip-Pinsel (common/clip, system/clip, playerclip). Sichtbar heißt "
        "gezeichnet und nicht durchsichtig (kein Glas, kein Alpha-Test, keine Flüssigkeit).",
        "**碰撞就是游戏所使用的碰撞:**优化器按 q3map2 的方式构建它——每个笔刷带有一个着色器的内容(液体优先于雾,雾优先于 playerclip,playerclip "
        "优先于 nonsolid;因此只要有一面是非实体,整个笔刷就可以穿过)、带实体着色器的面片、带 clip 生成标志的 misc_model 三角形,以及 clip 笔刷"
        "(common/clip、system/clip、playerclip)。可见是指会被绘制且不透明(不是玻璃、没有 alpha 测试、不是液体)。",
        "**コリジョンとはゲームが使うものです:**最適化ツールは q3map2 と同じように組み立てます。各ブラシは 1 つのシェーダーの内容を持ち(液体 > "
        "霧 > playerclip > nonsolid の順。つまり 1 面でも nonsolid ならブラシ全体を通り抜けられます)、固体シェーダーのパッチ、clip のスポーン"
        "フラグを持つ misc_model の三角形、clip ブラシ(common/clip、system/clip、playerclip)です。見えるとは、描画され透けないこと(ガラス、"
        "アルファテスト、液体ではない)です。",
        "**La colisión es la que usa el juego:** el optimizador la construye como q3map2: cada brush lleva el contenido de un shader "
        "(líquido antes que niebla antes que playerclip antes que nonsolid; una sola cara no sólida hace atravesable todo el brush), los "
        "patches con un shader sólido, los triángulos de un misc_model con un spawnflag de clip y los brushes clip (common/clip, "
        "system/clip, playerclip). Visible significa dibujado y no transparente (sin vidrio, sin prueba alfa, sin líquido).",
        "**Kolize je to, co používá hra:** optimalizátor ji sestaví jako q3map2 - každý brush nese obsah jednoho shaderu (kapalina před "
        "mlhou před playerclip před nonsolid; jediná nepevná strana tedy udělá celý brush průchozím), patche s pevným shaderem, trojúhelníky "
        "misc_model se spawnflagem clip a clip brushe (common/clip, system/clip, playerclip). Viditelné znamená kreslené a neprůhledné (ne "
        "sklo, ne alfa test, ne kapalina)."),
    "bau": (
        "**Only where the player gets to:** the optimizer follows the player from the player starts, NPC spawners, teleport destinations "
        "and items over walkable ground - steps up to STEPSIZE, jumps without the Force, drops, doors open. Visible floors, invisible "
        "collision, steps and passages count only there; roofs, the space behind fences and the inside of the sky box do not. Without any "
        "of these entities, a build without a leak decides (the clusters of the .bsp); invisible collision then needs that build.",
        "**Nur wo die Spielfigur hinkommt:** der Optimierer verfolgt die Spielfigur von den Startpunkten, NPC-Spawnern, Teleport-Zielen "
        "und Gegenständen über begehbaren Boden - Stufen bis STEPSIZE, Sprünge ohne Macht, Fallen, Türen offen. Sichtbare Böden, "
        "unsichtbare Kollision, Stufen und Durchgänge zählen nur dort; Dächer, der Raum hinter Zäunen und das Innere des Himmelskastens "
        "nicht. Ohne eine dieser Entities entscheidet ein Bau ohne Leck (die Cluster der .bsp); die unsichtbare Kollision braucht dann diesen Bau.",
        "**只在玩家能到达的地方:**优化器从玩家起点、NPC 生成器、传送目标和物品出发,沿可行走地面追踪玩家——台阶不超过 STEPSIZE、不用原力的跳跃、"
        "下落、门视为打开。可见地面、不可见碰撞、台阶和通道只在这些地方计算;屋顶、栅栏后面的空间以及天空盒内部不算。如果没有这些实体,则由一次"
        "无泄漏的编译决定(.bsp 的簇);此时不可见碰撞需要先编译。",
        "**プレイヤーが行ける場所だけ:**最適化ツールは、プレイヤーの開始点、NPC スポナー、テレポート先、アイテムから、歩ける床をたどってプレイヤーを"
        "追います(STEPSIZE までの段差、フォースなしのジャンプ、落下。ドアは開いているものとします)。見える床、見えないコリジョン、段差、通路は"
        "そこだけで数え、屋根、柵の向こう、スカイボックスの内側は数えません。これらのエンティティがないときは、リークのないビルドが決めます"
        "(.bsp のクラスター)。その場合、見えないコリジョンにはビルドが必要です。",
        "**Solo donde llega el jugador:** el optimizador sigue al jugador desde los puntos de inicio, los spawners de NPC, los destinos de "
        "teletransporte y los objetos por suelo transitable: escalones hasta STEPSIZE, saltos sin la Fuerza, caídas, puertas abiertas. Los "
        "suelos visibles, la colisión invisible, los escalones y los pasos solo cuentan allí; los tejados, el espacio tras las vallas y el "
        "interior de la caja de cielo no. Sin ninguna de esas entidades decide una compilación sin fugas (los clústeres del .bsp); entonces "
        "la colisión invisible necesita esa compilación.",
        "**Jen tam, kam se hráč dostane:** optimalizátor sleduje hráče od startovních bodů, spawnerů NPC, cílů teleportu a předmětů po "
        "pochozí podlaze - schody do STEPSIZE, skoky bez Síly, pády, dveře otevřené. Viditelné podlahy, neviditelná kolize, schody a "
        "průchody se počítají jen tam; střechy, prostor za ploty a vnitřek skyboxu ne. Bez těchto entit rozhoduje sestavení bez úniku "
        "(clustery .bsp); neviditelná kolize pak to sestavení potřebuje."),
    "masse": (
        "**Doors and corridors:** a func_door is measured by its brushes (the doors of one team together). On walkable ground the optimizer "
        "measures the height up to the collision and the width across in four directions at the player's middle height; a narrow place "
        "that stays narrow for a corridor width in both directions is a corridor, otherwise a passage, which only has to reach the door size.",
        "**Türen und Gänge:** eine func_door wird an ihren Pinseln gemessen (die Türen eines Teams zusammen). Auf begehbarem Boden misst "
        "der Optimierer die Höhe bis zur Kollision und die Breite quer in vier Richtungen auf halber Höhe der Spielfigur; eine Engstelle, "
        "die eine Gangbreite weit in beide Richtungen eng bleibt, ist ein Gang, sonst ein Durchgang, der nur das Türmaß erreichen muss.",
        "**门和走廊:**func_door 按其笔刷测量(同一 team 的门合在一起)。在可行走地面上,优化器测量到上方碰撞的高度,并在玩家半身高度处沿四个方向"
        "横向测量宽度;向两侧延伸一个走廊宽度仍然狭窄的地方是走廊,否则是通道,通道只需达到门的尺寸。",
        "**ドアと廊下:**func_door はそのブラシで測ります(同じ team のドアはまとめて)。歩ける床では、上のコリジョンまでの高さと、プレイヤーの中ほど"
        "の高さで 4 方向に横の幅を測ります。両方向に廊下の幅だけ狭いままの場所は廊下、それ以外は通路で、通路はドアの寸法を満たせば十分です。",
        "**Puertas y pasillos:** una func_door se mide por sus brushes (las puertas de un mismo team juntas). En el suelo transitable el "
        "optimizador mide la altura hasta la colisión y el ancho de lado en cuatro direcciones a media altura del jugador; un estrechamiento "
        "que sigue estrecho a lo largo de un ancho de pasillo en ambas direcciones es un pasillo; si no, es un paso, que solo debe alcanzar "
        "el tamaño de una puerta.",
        "**Dveře a chodby:** func_door se měří podle svých brushů (dveře jednoho týmu dohromady). Na pochozí podlaze optimalizátor měří "
        "výšku ke kolizi a šířku napříč ve čtyřech směrech v polovině výšky postavy; zúžení, které zůstává úzké o šířku chodby oběma směry, "
        "je chodba, jinak průchod, který musí dosáhnout jen velikosti dveří."),
    "json": (
        "**{{KeOptJson}}** writes the report as JSON for other tools: the limits, every finding with its objects and their position in map "
        "units (x y z, z up), and all shaders and images of the map with their sizes. The text report (**{{KeOptExport}}**) lists the "
        "shaders and images too. A right click on a finding selects and centers its objects or exports the JSON report.",
        "**{{KeOptJson}}** schreibt den Bericht als JSON für andere Werkzeuge: die Grenzen, jeden Befund mit seinen Objekten und deren Ort "
        "in Karteneinheiten (x y z, z oben) sowie alle Shader und Bilder der Karte mit ihrer Größe. Der Textbericht (**{{KeOptExport}}**) "
        "nennt die Shader und Bilder ebenfalls. Ein Rechtsklick auf einen Befund wählt und zentriert seine Objekte oder exportiert den "
        "JSON-Bericht.",
        "**{{KeOptJson}}** 把报告以 JSON 写出,供其他工具使用:各项限制、每个检查结果及其对象和以地图单位表示的位置(x y z,z 向上),以及地图的"
        "全部着色器和图像及其尺寸。文本报告(**{{KeOptExport}}**)同样会列出着色器和图像。右键单击某个检查结果可以选中并居中其对象,或导出 JSON "
        "报告。",
        "**{{KeOptJson}}** は、ほかのツール向けにレポートを JSON で書き出します: 上限、各検出結果とその対象およびマップ単位の位置(x y z、z が上)、"
        "マップのすべてのシェーダーと画像とその大きさ。テキストのレポート(**{{KeOptExport}}**)もシェーダーと画像を一覧にします。検出結果を"
        "右クリックすると、対象を選択して中央に表示するか、JSON レポートを書き出せます。",
        "**{{KeOptJson}}** escribe el informe como JSON para otras herramientas: los límites, cada hallazgo con sus objetos y su posición en "
        "unidades del mapa (x y z, z hacia arriba), y todos los shaders e imágenes del mapa con su tamaño. El informe de texto "
        "(**{{KeOptExport}}**) también enumera los shaders y las imágenes. Un clic derecho sobre un hallazgo selecciona y centra sus objetos "
        "o exporta el informe JSON.",
        "**{{KeOptJson}}** zapíše zprávu jako JSON pro jiné nástroje: limity, každý nález s jeho objekty a jejich polohou v jednotkách mapy "
        "(x y z, z nahoru) a všechny shadery a obrázky mapy s jejich velikostí. Textová zpráva (**{{KeOptExport}}**) shadery a obrázky "
        "uvádí také. Pravý klik na nález vybere a vycentruje jeho objekty nebo exportuje zprávu JSON."),
    "einstellungen": (
        "For these checks: **{{KePrOptToleranz}}** (largest gap between a visible floor and its collision, and between a prop and the "
        "floor; 1), **{{KePrOptDeckung}}** (2), **{{KePrOptTuerHoehe}}** and **{{KePrOptTuerBreite}}** (110 x 64), **{{KePrOptGangHoehe}}** and "
        "**{{KePrOptGangBreite}}** (128 x 96), **{{KePrOptStufenFlaeche}}** (96: the walkable area on top of a step, so that crates and "
        "consoles are no steps), **{{KePrOptBildMin}}** and **{{KePrOptBildMax}}** (64 to 2048) - the values of the list.",
        "Für diese Prüfungen: **{{KePrOptToleranz}}** (größter Abstand zwischen sichtbarem Boden und Kollision sowie zwischen Requisite und "
        "Boden; 1), **{{KePrOptDeckung}}** (2), **{{KePrOptTuerHoehe}}** und **{{KePrOptTuerBreite}}** (110 x 64), **{{KePrOptGangHoehe}}** und "
        "**{{KePrOptGangBreite}}** (128 x 96), **{{KePrOptStufenFlaeche}}** (96: die begehbare Fläche auf einer Stufe, damit Kisten und "
        "Pulte keine Stufen sind), **{{KePrOptBildMin}}** und **{{KePrOptBildMax}}** (64 bis 2048) - die Werte der Liste.",
        "用于这些检查的设置:**{{KePrOptToleranz}}**(可见地面与其碰撞之间、道具与地面之间的最大间距;1)、**{{KePrOptDeckung}}**(2)、"
        "**{{KePrOptTuerHoehe}}** 和 **{{KePrOptTuerBreite}}**(110 x 64)、**{{KePrOptGangHoehe}}** 和 **{{KePrOptGangBreite}}**(128 x 96)、"
        "**{{KePrOptStufenFlaeche}}**(96:台阶上方的可行走区域,使箱子和控制台不算台阶)、**{{KePrOptBildMin}}** 和 **{{KePrOptBildMax}}**"
        "(64 到 2048)——均为清单中的值。",
        "これらのチェック用: **{{KePrOptToleranz}}**(見える床とコリジョン、小道具と床の最大の差。1)、**{{KePrOptDeckung}}**(2)、"
        "**{{KePrOptTuerHoehe}}** と **{{KePrOptTuerBreite}}**(110 x 64)、**{{KePrOptGangHoehe}}** と **{{KePrOptGangBreite}}**(128 x 96)、"
        "**{{KePrOptStufenFlaeche}}**(96: 段差の上の歩ける面。箱や操作盤を段差にしないため)、**{{KePrOptBildMin}}** と "
        "**{{KePrOptBildMax}}**(64 から 2048)。いずれもリストの値です。",
        "Para estas comprobaciones: **{{KePrOptToleranz}}** (mayor separación entre un suelo visible y su colisión, y entre un accesorio y "
        "el suelo; 1), **{{KePrOptDeckung}}** (2), **{{KePrOptTuerHoehe}}** y **{{KePrOptTuerBreite}}** (110 x 64), **{{KePrOptGangHoehe}}** y "
        "**{{KePrOptGangBreite}}** (128 x 96), **{{KePrOptStufenFlaeche}}** (96: la zona transitable sobre un escalón, para que cajas y "
        "consolas no sean escalones), **{{KePrOptBildMin}}** y **{{KePrOptBildMax}}** (64 a 2048): los valores de la lista.",
        "Pro tyto kontroly: **{{KePrOptToleranz}}** (největší mezera mezi viditelnou podlahou a kolizí a mezi rekvizitou a podlahou; 1), "
        "**{{KePrOptDeckung}}** (2), **{{KePrOptTuerHoehe}}** a **{{KePrOptTuerBreite}}** (110 x 64), **{{KePrOptGangHoehe}}** a "
        "**{{KePrOptGangBreite}}** (128 x 96), **{{KePrOptStufenFlaeche}}** (96: pochozí plocha na schodu, aby bedny a pulty nebyly "
        "schody), **{{KePrOptBildMin}}** a **{{KePrOptBildMax}}** (64 až 2048) - hodnoty ze seznamu."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}

# Pruefung (Titel, Erklaerung = Titel + "E") fuer die Prueftabelle: keine hat eine Behebung
_PRUEFUNGEN = ["KeOptPrLaufOhneKoll", "KeOptPrUnsichtKoll", "KeOptPrKollAbstand", "KeOptPrDeckung", "KeOptPrStufe", "KeOptPrTuer",
               "KeOptPrDurchgang", "KeOptPrGang", "KeOptPrBildNpot", "KeOptPrBildFehlt", "KeOptPrBildGroesse", "KeOptPrModellFlaeche",
               "KeOptPrModellWelt", "KeOptPrMd3Grenzen", "KeOptPrSchwebt", "KeOptPrSteckt"]


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def einhaengen(optimierer):
    """Zeilen der Prueftabelle, ein Abschnitt nach der Tabelle, ein Absatz zu den Einstellungen (karte_optimierer)."""
    import inhalt_karte5
    b = optimierer["bloecke"]
    i = next(n for n, x in enumerate(b) if x[0] == "h2" and x[1] == "pruefungen")
    t = next(n for n in range(i, len(b)) if b[n][0] == "table")
    for p in _PRUEFUNGEN:
        b[t][2].append(inhalt_karte5._pruefzeile(p, None, False))
    neu = [("h3", _t("titel")), ("p", _t("kurz")), ("list", [_t("kollision"), _t("bau"), _t("masse")]), ("p", _t("json"))]
    b[t + 1:t + 1] = neu
    e = next(n for n, x in enumerate(b) if x[0] == "h2" and x[1] == "einstellungen")
    p = next(n for n in range(e, len(b)) if b[n][0] == "p")
    b.insert(p + 1, ("p", _t("einstellungen")))
