# -*- coding: utf-8 -*-
"""Oberflaechentexte von CW-PRUEF (Pruefungen der Clone-Wars-Kette im Optimierer des Karten-Editors, plaene/clonewars_kette.md):
Sicht gegen Kollision, fast deckungsgleiche Flaechen, Stufen, Tuer- und Gangmasse, Texturen, Modelle, Requisiten, JSON-Bericht.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština. Die englischen KeOptD39..54 sind genau die Vorlagen in
src/karte/optimierer_pruefen.cpp (kOptimiererDetailVorlagen, Mehrzahl), ctest karte_optimierer prueft das.
"""

ZEILEN = [
    # --- Pruefungen: Titel und Erklaerung ---
    ("KeOptPrLaufOhneKoll", "Visible floor without collision", "Sichtbarer Boden ohne Kollision", "没有碰撞的可见地面",
     "コリジョンのない見える床", "Suelo visible sin colisión", "Viditelná podlaha bez kolize"),
    ("KeOptPrLaufOhneKollE",
     "A player could walk on these visible faces (they face up; steeper than MIN_WALK_NORMAL 0.7 is a wall), but there is no collision within "
     "the player's height below them: the player falls through. Collision is what q3map2 writes into the .bsp: the content shader of each "
     "brush (SetBrushContents: liquid before fog before playerclip before nonsolid), patches with a solid shader and misc_model triangles "
     "with a clip spawnflag. Glass, alpha-tested surfaces and liquids are left out. Fix: make the face solid or put a clip brush under it.",
     "Auf diesen sichtbaren Flächen könnte man gehen (sie zeigen nach oben; steiler als MIN_WALK_NORMAL 0,7 ist eine Wand), doch innerhalb "
     "der Größe der Spielfigur liegt keine Kollision darunter: man fällt durch. Kollision ist, was q3map2 in die .bsp schreibt: der "
     "Inhalts-Shader jedes Pinsels (SetBrushContents: Flüssigkeit vor Nebel vor playerclip vor nonsolid), Patches mit festem Shader und "
     "Dreiecke von misc_model mit Clip-Spawnflag. Glas, Flächen mit Alpha-Test und Flüssigkeiten bleiben außen vor. Abhilfe: die Fläche "
     "fest machen oder einen Clip-Pinsel darunter legen.",
     "玩家可以在这些可见面上行走(它们朝上;比 MIN_WALK_NORMAL 0.7 更陡的是墙),但在玩家身高范围内其下方没有碰撞:玩家会掉下去。碰撞是 q3map2 "
     "写入 .bsp 的内容:每个笔刷的内容着色器(SetBrushContents:液体优先于雾,雾优先于 playerclip,playerclip 优先于 nonsolid)、带实体着色器的"
     "面片,以及带 clip 生成标志的 misc_model 三角形。玻璃、带 alpha 测试的表面和液体不在检查范围内。解决方法:让该面成为实体,或在其下方放一个 "
     "clip 笔刷。",
     "これらの見える面はプレイヤーが歩けます(上向き。MIN_WALK_NORMAL 0.7 より急なものは壁)が、プレイヤーの身長の範囲内でその下にコリジョンが"
     "ありません。プレイヤーは落ちます。コリジョンとは q3map2 が .bsp に書くもので、各ブラシの内容シェーダー(SetBrushContents: 液体 > 霧 > "
     "playerclip > nonsolid の順)、固体シェーダーのパッチ、clip のスポーンフラグを持つ misc_model の三角形です。ガラス、アルファテストの面、"
     "液体は対象外です。対処: 面を固体にするか、下に clip ブラシを置きます。",
     "Un jugador podría caminar sobre estas caras visibles (miran hacia arriba; más empinado que MIN_WALK_NORMAL 0,7 es una pared), pero "
     "no hay colisión debajo dentro de la altura del jugador: el jugador cae a través. La colisión es lo que q3map2 escribe en el .bsp: el "
     "shader de contenido de cada brush (SetBrushContents: líquido antes que niebla antes que playerclip antes que nonsolid), los patches "
     "con un shader sólido y los triángulos de misc_model con un spawnflag de clip. El vidrio, las superficies con prueba alfa y los "
     "líquidos quedan fuera. Solución: haz la cara sólida o pon un brush clip debajo.",
     "Po těchto viditelných plochách by se dalo chodit (míří nahoru; strmější než MIN_WALK_NORMAL 0,7 je stěna), ale v rozsahu výšky "
     "postavy pod nimi není žádná kolize: hráč propadne. Kolize je to, co q3map2 zapíše do .bsp: obsahový shader každého brushe "
     "(SetBrushContents: kapalina před mlhou před playerclip před nonsolid), patche s pevným shaderem a trojúhelníky misc_model se "
     "spawnflagem clip. Sklo, plochy s alfa testem a kapaliny se nepočítají. Náprava: udělejte plochu pevnou, nebo pod ni dejte clip brush."),
    ("KeOptPrUnsichtKoll", "Invisible collision", "Unsichtbare Kollision", "不可见的碰撞", "見えないコリジョン", "Colisión invisible",
     "Neviditelná kolize"),
    ("KeOptPrUnsichtKollE",
     "Faces of collision brushes that are not drawn (clip, caulk, nodraw), where the player can get to them, with no visible surface close "
     "behind them: an invisible floor (nothing visible within STEPSIZE 18 below - a clip ramp over stairs is fine) or an invisible wall "
     "(nothing visible within half the player's width). Where the player gets to is followed from the player starts, spawners, teleport "
     "destinations and items over walkable ground (steps up to STEPSIZE, jumps without the Force, drops; doors open); without any of them "
     "only after a build without a leak (clusters of the .bsp).",
     "Flächen von Kollisionspinseln, die nicht gezeichnet werden (clip, caulk, nodraw), wo die Spielfigur hinkommt und dicht dahinter keine "
     "sichtbare Fläche liegt: ein unsichtbarer Boden (innerhalb von STEPSIZE 18 darunter nichts Sichtbares - eine Clip-Rampe über einer "
     "Treppe ist in Ordnung) oder eine unsichtbare Wand (innerhalb der halben Breite der Spielfigur nichts Sichtbares). Wohin die Spielfigur "
     "kommt, verfolgt der Optimierer von den Startpunkten, Spawnern, Teleport-Zielen und Gegenständen über begehbaren Boden (Stufen bis "
     "STEPSIZE, Sprünge ohne Macht, Fallen; Türen offen); ohne einen davon erst nach einem Bau ohne Leck (Cluster der .bsp).",
     "不被绘制的碰撞笔刷面(clip、caulk、nodraw),玩家能到达那里,而其后方近处没有可见表面:不可见的地面(下方 STEPSIZE 18 以内没有可见物体——"
     "楼梯上方的 clip 斜坡没有问题)或不可见的墙(玩家半个宽度内没有可见物体)。玩家能到达的地方从玩家起点、生成器、传送目标和物品出发,沿可行走地面追踪(台阶不超过 STEPSIZE、"
     "不用原力的跳跃、下落;门视为打开);如果都没有,则只在一次无泄漏的编译之后(.bsp 的簇)。",
     "描画されないコリジョンブラシの面(clip、caulk、nodraw)で、プレイヤーが行ける場所にあり、すぐ後ろに見える面がないもの: 見えない床"
     "(下 STEPSIZE 18 以内に見えるものがない。階段の上の clip スロープは問題ありません)または見えない壁(プレイヤーの幅の半分以内に見えるもの"
     "がない)。プレイヤーが行ける場所は、プレイヤーの開始点、スポナー、テレポート先、アイテムから歩ける床をたどって求めます(STEPSIZE までの段差、"
     "フォースなしのジャンプ、落下。ドアは開いているものとします)。どれもない場合は、リークのないビルドの後だけです(.bsp のクラスター)。",
     "Caras de brushes de colisión que no se dibujan (clip, caulk, nodraw), adonde el jugador puede llegar, sin una superficie visible "
     "cerca detrás: un suelo invisible (nada visible dentro de STEPSIZE 18 por debajo; una rampa clip sobre una escalera está bien) o una "
     "pared invisible (nada visible dentro de la mitad del ancho del jugador). Adónde llega el jugador se sigue desde los puntos de "
     "inicio, los spawners, los destinos de teletransporte y los objetos por suelo transitable (escalones hasta STEPSIZE, saltos sin la "
     "Fuerza, caídas; puertas abiertas); sin ninguno de ellos, solo tras una compilación sin fugas (clústeres del .bsp).",
     "Plochy kolizních brushů, které se nekreslí (clip, caulk, nodraw), kam se hráč dostane a těsně za nimi není viditelná plocha: "
     "neviditelná podlaha (do STEPSIZE 18 pod ní nic viditelného - clip rampa nad schody je v pořádku) nebo neviditelná stěna (do poloviny "
     "šířky postavy nic viditelného). Kam se hráč dostane, sleduje optimalizátor od startovních bodů, spawnerů, cílů teleportu a "
     "předmětů po pochozí podlaze (schody do STEPSIZE, skoky bez Síly, pády; dveře otevřené); bez nich až po sestavení bez úniku "
     "(clustery .bsp)."),
    ("KeOptPrKollAbstand", "Visible floor and collision apart", "Sichtbarer Boden und Kollision auseinander", "可见地面与碰撞分离",
     "見える床とコリジョンのずれ", "Suelo visible y colisión separados", "Viditelná podlaha a kolize od sebe"),
    ("KeOptPrKollAbstandE",
     "The visible floor and the collision under or over it are more than the tolerance apart (1 unit in the list, adjustable): with the "
     "collision below, the feet sink into the visible floor; with an invisible collision brush on top, the player floats above it.",
     "Sichtbarer Boden und Kollision darunter oder darüber liegen weiter als die Toleranz auseinander (1 Einheit laut Liste, einstellbar): "
     "liegt die Kollision darunter, stecken die Füße im sichtbaren Boden; liegt ein unsichtbarer Kollisionspinsel darauf, schwebt die "
     "Spielfigur darüber.",
     "可见地面与其下方或上方的碰撞相距超过容差(清单为 1 单位,可调):碰撞在下方时,脚会陷入可见地面;上面有不可见的碰撞笔刷时,玩家会悬浮在地面之上。",
     "見える床とその下または上のコリジョンが許容値(リストでは 1 ユニット、変更可)より離れています。コリジョンが下にあると足が見える床に埋まり、"
     "見えないコリジョンブラシが上にあるとプレイヤーは浮きます。",
     "El suelo visible y la colisión por debajo o por encima están más separados que la tolerancia (1 unidad según la lista, ajustable): "
     "con la colisión debajo, los pies se hunden en el suelo visible; con un brush de colisión invisible encima, el jugador flota sobre él.",
     "Viditelná podlaha a kolize pod ní nebo nad ní jsou od sebe dál než tolerance (podle seznamu 1 jednotka, nastavitelné): je-li kolize "
     "pod ní, nohy se boří do viditelné podlahy; leží-li na ní neviditelný kolizní brush, hráč nad ní levituje."),
    ("KeOptPrDeckung", "Nearly coincident faces (z-fighting)", "Fast deckungsgleiche Flächen (Z-Fighting)", "几乎重合的面(深度冲突)",
     "ほぼ重なった面(Z ファイティング)", "Caras casi coincidentes (z-fighting)", "Téměř shodné plochy (z-fighting)"),
    ("KeOptPrDeckungE",
     "Drawn faces of two different brushes face the same way, overlap and lie less than 2 units apart (adjustable): the game flickers "
     "between them (z-fighting), at a distance even when they are a little apart. q3map2 removes only exactly identical faces (CullSides) "
     "and faces completely inside another brush (SideInBrush); structural brushes cut away what lies inside them. Faces with polygonOffset "
     "(decals) and pairs with the same shader (the same picture on both - the flicker cannot be seen) are left out. The check for overlapping brushes covers only volumes overlapping by more than 1 unit.",
     "Gezeichnete Flächen zweier verschiedener Pinsel zeigen in dieselbe Richtung, überdecken sich und liegen weniger als 2 Einheiten "
     "auseinander (einstellbar): das Spiel flackert zwischen ihnen (Z-Fighting), aus der Entfernung auch dann, wenn sie etwas auseinander "
     "liegen. q3map2 entfernt nur genau gleiche Flächen (CullSides) und Flächen ganz in einem anderen Pinsel (SideInBrush); Strukturpinsel "
     "schneiden weg, was in ihnen liegt. Flächen mit polygonOffset (Decals) und Paare mit demselben Shader (auf beiden dasselbe Bild - das Flackern sieht man nicht) "
     "bleiben außen vor. Die Prüfung auf überlappende Pinsel erfasst "
     "nur Körper, die sich um mehr als 1 Einheit überlappen.",
     "两个不同笔刷的已绘制面朝向相同、相互重叠且相距不到 2 单位(可调):游戏会在它们之间闪烁(深度冲突),在远处即使稍有间隔也会出现。q3map2 只会移除"
     "完全相同的面(CullSides)和完全位于另一个笔刷内部的面(SideInBrush);结构笔刷会切掉位于其内部的部分。带 polygonOffset 的面(贴花)以及使用相同着色器的面对(两者显示相同的图像——看不出闪烁)不在检查范围内。"
     "笔刷重叠检查只涵盖重叠超过 1 单位的体积。",
     "異なる 2 つのブラシの描画される面が同じ向きで重なり、2 ユニット未満(変更可)しか離れていません。ゲームではその間がちらつきます(Z ファイ"
     "ティング)。遠くからだと少し離れていても起こります。q3map2 が消すのは完全に同じ面(CullSides)と別のブラシの中にすっぽり入った面"
     "(SideInBrush)だけで、構造ブラシは内部にあるものを切り取ります。polygonOffset の面(デカール)と、同じシェーダーの組(両方に同じ画像が出るのでちらつきは見えません)は対象外です。重なったブラシのチェックが扱う"
     "のは 1 ユニットを超えて重なる立体だけです。",
     "Caras dibujadas de dos brushes distintos miran hacia el mismo lado, se solapan y están a menos de 2 unidades (ajustable): el juego "
     "parpadea entre ellas (z-fighting), a distancia incluso cuando están algo separadas. q3map2 solo elimina caras exactamente iguales "
     "(CullSides) y caras completamente dentro de otro brush (SideInBrush); los brushes estructurales recortan lo que queda dentro de "
     "ellos. Las caras con polygonOffset (decals) y los pares con el mismo shader (la misma imagen en "
     "ambas: el parpadeo no se ve) quedan fuera. La comprobación de brushes solapados solo cubre volúmenes que se solapan "
     "más de 1 unidad.",
     "Kreslené plochy dvou různých brushů míří stejným směrem, překrývají se a jsou od sebe méně než 2 jednotky (nastavitelné): hra mezi "
     "nimi bliká (z-fighting), zdálky i tehdy, když jsou trochu od sebe. q3map2 odstraní jen přesně shodné plochy (CullSides) a plochy "
     "celé uvnitř jiného brushe (SideInBrush); strukturní brushe odříznou, co leží uvnitř nich. Plochy s polygonOffset (decaly) a páry se stejným shaderem (na obou stejný "
     "obrázek - blikání není vidět) se nepočítají. Kontrola překrývajících se brushů zachytí jen tělesa překrytá o víc než 1 jednotku."),
    # --- Angaben (KeOptD39..54, englisch = kOptimiererDetailVorlagen) ---
    ("KeOptD39", "%s visible walkable {face|faces} without collision below {it|them} (the player falls through)",
     "%s {sichtbare begehbare Fläche|sichtbare begehbare Flächen} ohne Kollision darunter (man fällt durch)",
     "%s 个可见的可行走面下方没有碰撞(玩家会掉下去)", "下にコリジョンがない、歩ける見える面 %s 個(プレイヤーは落ちます)",
     "%s {cara visible transitable|caras visibles transitables} sin colisión debajo (el jugador cae a través)",
     "%s {viditelná pochozí plocha|viditelné pochozí plochy|viditelných pochozích ploch} bez kolize pod sebou (hráč propadne)"),
    ("KeOptD40", "%s collision {face|faces} the player cannot see (invisible floor or wall)",
     "%s {Kollisionsfläche|Kollisionsflächen}, die man nicht sieht (unsichtbarer Boden oder unsichtbare Wand)",
     "%s 个玩家看不到的碰撞面(不可见的地面或墙)", "プレイヤーに見えないコリジョン面 %s 個(見えない床または壁)",
     "%s {cara de colisión|caras de colisión} que el jugador no ve (suelo o pared invisible)",
     "%s {kolizní plocha|kolizní plochy|kolizních ploch}, které hráč nevidí (neviditelná podlaha nebo stěna)"),
    ("KeOptD41", "%s visible walkable {face|faces} more than %s units away from the collision (largest gap %s)",
     "%s {sichtbare begehbare Fläche|sichtbare begehbare Flächen} mehr als %s Einheiten von der Kollision entfernt (größter Abstand %s)",
     "%s 个可见的可行走面与碰撞相距超过 %s 单位(最大间距 %s)", "歩ける見える面 %s 個がコリジョンから %s ユニットを超えて離れています(最大の間隔 %s)",
     "%s {cara visible transitable|caras visibles transitables} a más de %s unidades de la colisión (mayor separación %s)",
     "%s {viditelná pochozí plocha|viditelné pochozí plochy|viditelných pochozích ploch} víc než %s jednotek od kolize (největší mezera %s)"),
    ("KeOptD42", "%s {pair|pairs} of drawn faces of different parts face the same way less than %s units apart (z-fighting)",
     "%s {Paar|Paare} gezeichneter Flächen verschiedener Teile zeigen in dieselbe Richtung, weniger als %s Einheiten auseinander (Z-Fighting)",
     "%s 对不同部件的已绘制面朝向相同、相距不到 %s 单位(深度冲突)", "異なる部品の描画面の組 %s 個が同じ向きで %s ユニット未満しか離れていません(Z ファイティング)",
     "%s {par|pares} de caras dibujadas de partes distintas miran al mismo lado a menos de %s unidades (z-fighting)",
     "%s {pár|páry|párů} kreslených ploch různých částí míří stejným směrem méně než %s jednotek od sebe (z-fighting)"),
    # --- Einstellungen (Seite Optimizer) ---
    ("KePrOptToleranz", "Visible/collision gap", "Abstand Sicht/Kollision", "可见/碰撞间距", "見た目とコリジョンの差",
     "Separación visible/colisión", "Mezera vidět/kolize"),
    ("KePrOptDeckung", "Coincident under", "Deckungsgleich unter", "重合距离小于", "重なり判定の距離", "Coincidentes a menos de",
     "Shodné pod"),
    ("KePrOptTuerHoehe", "Door height", "Türhöhe", "门高", "ドアの高さ", "Alto de puerta", "Výška dveří"),
    ("KePrOptTuerBreite", "Door width", "Türbreite", "门宽", "ドアの幅", "Ancho de puerta", "Šířka dveří"),
    ("KePrOptGangHoehe", "Corridor height", "Ganghöhe", "走廊高", "廊下の高さ", "Alto de pasillo", "Výška chodby"),
    ("KePrOptGangBreite", "Corridor width", "Gangbreite", "走廊宽", "廊下の幅", "Ancho de pasillo", "Šířka chodby"),
    ("KePrOptStufenFlaeche", "Step: area at least", "Stufe: Fläche mindestens", "台阶:区域至少", "段差: 面の最小", "Escalón: zona mínima",
     "Schod: plocha aspoň"),
    ("KePrOptStufenFlaecheTip",
     "A step higher than STEPSIZE only counts when the walkable area on top, connected on foot, is at least this long and wide (units). "
     "The small tops of crates and consoles stay out.",
     "Eine Stufe höher als STEPSIZE zählt nur, wenn die zu Fuß verbundene begehbare Fläche darauf mindestens so lang und breit ist "
     "(Einheiten). Die kleinen Deckflächen von Kisten und Pulten bleiben außen vor.",
     "高于 STEPSIZE 的台阶只有在其上方可步行连通的可行走区域至少有这么长和宽(单位)时才计算。箱子和控制台的小顶面不计。",
     "STEPSIZE より高い段差は、その上の歩いてつながる歩ける面が少なくともこの長さと幅(ユニット)のときだけ数えます。箱や操作盤の小さな"
     "上面は対象外です。",
     "Un escalón más alto que STEPSIZE solo cuenta si la zona transitable de arriba, conectada a pie, mide al menos esto de largo y de "
     "ancho (unidades). Las tapas pequeñas de cajas y consolas quedan fuera.",
     "Schod vyšší než STEPSIZE se počítá jen tehdy, když je pochozí plocha nahoře spojená chůzí aspoň takto dlouhá a široká (jednotky). "
     "Malé vrchy beden a pultů se nepočítají."),
    ("KePrOptBildMin", "Smallest image side", "Kleinste Bildseite", "最小图像边长", "画像の最小辺", "Lado mínimo de imagen",
     "Nejmenší strana obrázku"),
    ("KePrOptBildMax", "Largest image side", "Größte Bildseite", "最大图像边长", "画像の最大辺", "Lado máximo de imagen",
     "Největší strana obrázku"),
    ("KePrOptToleranzTip",
     "Largest gap in units between a visible floor and its collision, and between a prop and the floor (the Clone Wars list: 1).",
     "Größter Abstand in Einheiten zwischen sichtbarem Boden und seiner Kollision sowie zwischen Requisite und Boden (Clone-Wars-Liste: 1).",
     "可见地面与其碰撞之间、道具与地面之间允许的最大间距(单位;Clone Wars 清单:1)。",
     "見える床とそのコリジョンの間、および小道具と床の間の最大の差(ユニット。Clone Wars のリスト: 1)。",
     "Mayor separación en unidades entre un suelo visible y su colisión, y entre un accesorio y el suelo (lista de Clone Wars: 1).",
     "Největší mezera v jednotkách mezi viditelnou podlahou a její kolizí a mezi rekvizitou a podlahou (seznam Clone Wars: 1)."),
    ("KePrOptDeckungTip",
     "Drawn faces of different parts that face the same way closer than this flicker (z-fighting); the list says 2 units.",
     "Gezeichnete Flächen verschiedener Teile, die in dieselbe Richtung zeigen und näher beieinander liegen, flackern (Z-Fighting); laut "
     "Liste 2 Einheiten.",
     "朝向相同且距离小于此值的不同部件已绘制面会闪烁(深度冲突);清单规定为 2 单位。",
     "同じ向きでこれより近い、異なる部品の描画面はちらつきます(Z ファイティング)。リストでは 2 ユニットです。",
     "Las caras dibujadas de partes distintas que miran al mismo lado y están más cerca que esto parpadean (z-fighting); la lista indica 2 "
     "unidades.",
     "Kreslené plochy různých částí, které míří stejným směrem a jsou blíž než tato hodnota, blikají (z-fighting); seznam uvádí 2 jednotky."),
    ("KePrOptMasseTip",
     "Minimum sizes of doors and corridors in units (the list: door 110 x 64, corridor 128 x 96, height x width). The player is 64 high "
     "(crouched 40) and 32 wide.",
     "Mindestmaße von Türen und Gängen in Einheiten (Liste: Tür 110 x 64, Gang 128 x 96, Höhe x Breite). Die Spielfigur ist 64 hoch "
     "(geduckt 40) und 32 breit.",
     "门和走廊的最小尺寸(单位;清单:门 110 x 64,走廊 128 x 96,高 x 宽)。玩家高 64(蹲下 40)、宽 32。",
     "ドアと廊下の最小寸法(ユニット。リスト: ドア 110 x 64、廊下 128 x 96、高さ x 幅)。プレイヤーは高さ 64(しゃがみ 40)、幅 32 です。",
     "Tamaños mínimos de puertas y pasillos en unidades (lista: puerta 110 x 64, pasillo 128 x 96, alto x ancho). El jugador mide 64 de "
     "alto (agachado 40) y 32 de ancho.",
     "Minimální velikosti dveří a chodeb v jednotkách (seznam: dveře 110 x 64, chodba 128 x 96, výška x šířka). Postava je 64 vysoká "
     "(přikrčená 40) a 32 široká."),
    ("KePrOptBildTip",
     "Image sides outside this range are noted (the list: 64 to 2048). A side that is not a power of two is always an error: the game quits.",
     "Bildseiten außerhalb dieses Bereichs werden als Hinweis genannt (Liste: 64 bis 2048). Eine Seite, die keine Zweierpotenz ist, ist "
     "immer ein Fehler: das Spiel beendet sich.",
     "超出此范围的图像边长会作为提示列出(清单:64 到 2048)。边长不是 2 的幂始终是错误:游戏会退出。",
     "この範囲外の画像の辺は注意として表示します(リスト: 64 から 2048)。2 のべき乗でない辺は常にエラーです。ゲームが終了します。",
     "Los lados de imagen fuera de este rango se indican como aviso (lista: 64 a 2048). Un lado que no es potencia de dos siempre es un "
     "error: el juego se cierra.",
     "Strany obrázků mimo tento rozsah se uvádějí jako upozornění (seznam: 64 až 2048). Strana, která není mocninou dvou, je vždy chyba: "
     "hra se ukončí."),
    # --- Fenster des Optimierers: JSON-Bericht, Rechtsklick ---
    ("KeOptJson", "JSON...", "JSON...", "JSON...", "JSON...", "JSON...", "JSON..."),
    ("KeOptJsonTip",
     "Writes the report as JSON with coordinates for other tools: the limits, every finding with its objects and their position (x y z, "
     "map units), and the shaders and images of the map.",
     "Schreibt den Bericht als JSON mit Koordinaten für andere Werkzeuge: die Grenzen, jeden Befund mit seinen Objekten und deren Ort "
     "(x y z, Karteneinheiten) sowie die Shader und Bilder der Karte.",
     "把报告以带坐标的 JSON 写出,供其他工具使用:各项限制、每个检查结果及其对象和位置(x y z,地图单位),以及地图的着色器和图像。",
     "ほかのツール向けに、座標付きの JSON でレポートを書き出します: 上限、各検出結果とその対象および位置(x y z、マップ単位)、マップのシェーダー"
     "と画像。",
     "Escribe el informe como JSON con coordenadas para otras herramientas: los límites, cada hallazgo con sus objetos y su posición (x y "
     "z, unidades del mapa) y los shaders e imágenes del mapa.",
     "Zapíše zprávu jako JSON se souřadnicemi pro jiné nástroje: limity, každý nález s jeho objekty a jejich polohou (x y z, jednotky "
     "mapy) a shadery a obrázky mapy."),
    ("KeOptJsonTitel", "Export optimizer report as JSON", "Bericht des Optimierers als JSON exportieren", "将优化器报告导出为 JSON",
     "最適化ツールのレポートを JSON で書き出す", "Exportar el informe del optimizador como JSON", "Exportovat zprávu optimalizátoru jako JSON"),
    ("KeOptJsonFehler", "Could not write %s", "%s konnte nicht geschrieben werden", "无法写入 %s", "%s を書き込めませんでした",
     "No se pudo escribir %s", "Nelze zapsat %s"),
    ("KeOptRkZentrieren", "Select and center", "Auswählen und zentrieren", "选中并居中", "選択して中央に表示", "Seleccionar y centrar",
     "Vybrat a vycentrovat"),
    ("KeOptBerichtTexturen", "Shaders and images", "Shader und Bilder", "着色器和图像", "シェーダーと画像", "Shaders e imágenes",
     "Shadery a obrázky"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
