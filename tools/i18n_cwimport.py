# -*- coding: utf-8 -*-
"""Oberflaechentexte von CW-IMPORT (Clone-Wars-Kette Teil 2, plaene/clonewars_kette.md): das Pruefprofil "Clone-Wars-Kette" (ab Werk
aus), die Testfigur, Massstab und Achsen beim Import, der Massstab je Raum, Treppe zu Clip-Rampe, glatte Clip-Wand und die Ebene
Collision.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    # --- Pruefprofil (shank 14.11.: "das ist wichtig das es zwar drin ist aber nicht standard") ---
    ("KeCwProfil", "Check profile", "Prüfprofil", "检查配置", "チェックプロファイル", "Perfil de comprobación", "Profil kontrol"),
    ("KeCwProfilStandard", "Standard", "Standard", "标准", "標準", "Estándar", "Standardní"),
    ("KeCwProfilClonewars", "Clone Wars chain", "Clone-Wars-Kette", "Clone Wars 流程", "Clone Wars チェーン", "Cadena Clone Wars",
     "Řetězec Clone Wars"),
    ("KeCwMenue", "Clone Wars chain (extended checks)", "Clone-Wars-Kette (erweiterte Prüfungen)", "Clone Wars 流程(扩展检查)",
     "Clone Wars チェーン(拡張チェック)", "Cadena Clone Wars (comprobaciones ampliadas)", "Řetězec Clone Wars (rozšířené kontroly)"),
    ("KeCwProfilTip",
     "Off by default. On: the optimizer also runs the 16 checks from the requirements of a Clone Wars mod (visible floor against collision, "
     "nearly coincident faces, steps, doors and corridors, textures, models, props), offers the report as JSON with coordinates, and "
     "\"Model + clip\" puts its clip brushes into the layer Collision. Off: everything behaves as without these additions.",
     "Ab Werk aus. An: der Optimierer führt zusätzlich die 16 Prüfungen aus den Anforderungen eines Clone-Wars-Mods aus (sichtbarer Boden "
     "gegen Kollision, fast deckungsgleiche Flächen, Stufen, Türen und Gänge, Texturen, Modelle, Requisiten), bietet den Bericht als JSON "
     "mit Koordinaten an, und \"Modell + Clip\" legt seine Clip-Pinsel in die Ebene Collision. Aus: alles verhält sich wie ohne diese "
     "Ergänzungen.",
     "默认关闭。开启时:优化器还会运行来自一个 Clone Wars 模组需求的 16 项检查(可见地面与碰撞、几乎重合的面、台阶、门和走廊、纹理、模型、道具),"
     "提供带坐标的 JSON 报告,并且“模型 + clip”会把它的 clip 笔刷放入图层 Collision。关闭时:一切都与没有这些补充时相同。",
     "既定ではオフ。オン: 最適化ツールは Clone Wars MOD の要件から来た 16 のチェック(見える床とコリジョン、ほぼ重なった面、段差、ドアと廊下、"
     "テクスチャ、モデル、小道具)も実行し、座標付きの JSON レポートを書き出せ、「モデル + クリップ」はクリップブラシをレイヤー Collision に"
     "入れます。オフ: すべてこれらの追加がないときと同じに動きます。",
     "Desactivado por defecto. Activado: el optimizador ejecuta además las 16 comprobaciones de los requisitos de un mod de Clone Wars "
     "(suelo visible frente a colisión, caras casi coincidentes, escalones, puertas y pasillos, texturas, modelos, accesorios), ofrece el "
     "informe como JSON con coordenadas, y \"Modelo + clip\" pone sus brushes clip en la capa Collision. Desactivado: todo se comporta "
     "como sin estas ampliaciones.",
     "Ve výchozím stavu vypnuto. Zapnuto: optimalizátor navíc provede 16 kontrol z požadavků módu Clone Wars (viditelná podlaha proti "
     "kolizi, téměř shodné plochy, schody, dveře a chodby, textury, modely, rekvizity), nabídne zprávu jako JSON se souřadnicemi a "
     "\"Model + clip\" vloží své clip brushe do vrstvy Collision. Vypnuto: vše se chová jako bez těchto doplňků."),
    # --- Punkt 1: Testfigur ---
    ("KeCwFigurMenue", "Test figure", "Testfigur", "测试人物", "テストフィギュア", "Figura de prueba", "Testovací postava"),
    ("KeCwFigur", "Show test figure", "Testfigur zeigen", "显示测试人物", "テストフィギュアを表示", "Mostrar figura de prueba",
     "Zobrazit testovací postavu"),
    ("KeCwFigurTip",
     "The player in original size in all views: the box the engine moves (32 x 32 x 64, origin at hip height; multiplayer 30 wide), the "
     "player model in its idle pose, the STEPSIZE mark (18) and the minimum door and corridor sizes of the optimizer. Off by default.",
     "Die Spielfigur in Originalgröße in allen Ansichten: der Kasten, den die Engine bewegt (32 x 32 x 64, Ursprung in Hüfthöhe; "
     "Mehrspieler 30 breit), das Spielermodell in seiner Ruhepose, die Marke STEPSIZE (18) und die Mindestmaße für Türen und Gänge "
     "aus dem Optimierer. Ab Werk aus.",
     "在所有视图中以原始尺寸显示玩家:引擎移动的盒子(32 x 32 x 64,原点在髋部高度;多人模式宽 30)、处于待机姿势的玩家模型、STEPSIZE 标记(18)"
     "以及优化器中门和走廊的最小尺寸。默认关闭。",
     "すべてのビューに実物大のプレイヤーを表示します: エンジンが動かす箱(32 x 32 x 64、原点は腰の高さ。マルチプレイは幅 30)、待機ポーズの"
     "プレイヤーモデル、STEPSIZE の印(18)、最適化ツールのドアと廊下の最小寸法。既定ではオフ。",
     "El jugador a tamaño original en todas las vistas: la caja que mueve el motor (32 x 32 x 64, origen a la altura de la cadera; "
     "multijugador 30 de ancho), el modelo del jugador en su pose de reposo, la marca STEPSIZE (18) y las medidas mínimas de puertas y "
     "pasillos del optimizador. Desactivado por defecto.",
     "Postava hráče ve skutečné velikosti ve všech pohledech: kvádr, kterým engine pohybuje (32 x 32 x 64, počátek ve výšce boků; "
     "multiplayer 30 široký), model hráče v klidové póze, značka STEPSIZE (18) a minimální rozměry dveří a chodeb z optimalizátoru. Ve "
     "výchozím stavu vypnuto."),
    ("KeCwFigurGeduckt", "Crouched", "Geduckt", "蹲下", "しゃがみ", "Agachado", "Přikrčená"),
    ("KeCwFigurGeducktTip", "Top of the box at 16 instead of 40 (CROUCH_MAXS_2); the model crouches (BOTH_CROUCH1).",
     "Oberkante des Kastens bei 16 statt 40 (CROUCH_MAXS_2); das Modell duckt sich (BOTH_CROUCH1).",
     "盒子顶部在 16 而不是 40(CROUCH_MAXS_2);模型蹲下(BOTH_CROUCH1)。",
     "箱の上端を 40 ではなく 16 にします(CROUCH_MAXS_2)。モデルはしゃがみます(BOTH_CROUCH1)。",
     "Parte superior de la caja a 16 en lugar de 40 (CROUCH_MAXS_2); el modelo se agacha (BOTH_CROUCH1).",
     "Horní hrana kvádru ve 16 místo 40 (CROUCH_MAXS_2); model se přikrčí (BOTH_CROUCH1)."),
    ("KeCwFigurMarken", "Marks: step, door, corridor", "Marken: Stufe, Tür, Gang", "标记:台阶、门、走廊", "印: 段差・ドア・廊下",
     "Marcas: escalón, puerta, pasillo", "Značky: schod, dveře, chodba"),
    ("KeCwFigurMarkenTip",
     "STEPSIZE 18 above the feet, the other height (crouched or standing), and frames for the minimum door (110 x 64) and corridor "
     "(128 x 96) from the optimizer settings.",
     "STEPSIZE 18 über den Füßen, die andere Höhe (geduckt bzw. stehend) und Rahmen für die Mindestmaße von Tür (110 x 64) und Gang "
     "(128 x 96) aus den Einstellungen des Optimierers.",
     "脚上方的 STEPSIZE 18、另一种高度(蹲下或站立),以及优化器设置中门(110 x 64)和走廊(128 x 96)最小尺寸的框。",
     "足の上の STEPSIZE 18、もう一方の高さ(しゃがみ/立ち)、最適化ツールの設定にあるドア(110 x 64)と廊下(128 x 96)の最小寸法の枠。",
     "STEPSIZE 18 sobre los pies, la otra altura (agachado o de pie) y marcos para las medidas mínimas de puerta (110 x 64) y pasillo "
     "(128 x 96) de los ajustes del optimizador.",
     "STEPSIZE 18 nad chodidly, druhá výška (přikrčená nebo stojící) a rámy minimálních rozměrů dveří (110 x 64) a chodby (128 x 96) z "
     "nastavení optimalizátoru."),
    ("KeCwFigurQuer", "Turn frames", "Rahmen drehen", "旋转框", "枠を回転", "Girar marcos", "Otočit rámy"),
    ("KeCwFigurQuerTip", "Door and corridor frames across the other axis (along X instead of Y).",
     "Tür- und Gangrahmen quer zur anderen Achse (entlang X statt Y).", "门和走廊的框改为沿另一轴(沿 X 而不是 Y)。",
     "ドアと廊下の枠をもう一方の軸に沿わせます(Y ではなく X)。", "Marcos de puerta y pasillo a lo largo del otro eje (X en lugar de Y).",
     "Rámy dveří a chodby podél druhé osy (podél X místo Y)."),
    ("KeCwFigurModell", "Player model", "Spielermodell", "玩家模型", "プレイヤーモデル", "Modelo del jugador", "Model hráče"),
    ("KeCwFigurModellTip",
     "The player model of the game (models/players/kyle) in the camera, posed like the game when the player stands still.",
     "Das Spielermodell des Spiels (models/players/kyle) in der Kamera, in der Pose, die das Spiel im Stillstand zeigt.",
     "在相机视图中显示游戏的玩家模型(models/players/kyle),姿势与玩家静止不动时游戏中的姿势相同。",
     "カメラにゲームのプレイヤーモデル(models/players/kyle)を、立ち止まっているときのゲームと同じポーズで表示します。",
     "El modelo del jugador del juego (models/players/kyle) en la cámara, en la pose que muestra el juego cuando el jugador está quieto.",
     "Model hráče ze hry (models/players/kyle) v kameře, v póze, jakou hra ukazuje, když hráč stojí."),
    ("KeCwFigurHier", "Place in front of the camera", "Vor die Kamera stellen", "放到相机前方", "カメラの前に置く",
     "Colocar delante de la cámara", "Postavit před kameru"),
    ("KeCwFigurHierMenue", "Test figure here", "Testfigur hierher", "测试人物放到这里", "テストフィギュアをここへ", "Figura de prueba aquí",
     "Testovací postava sem"),
    ("KeCwFigurHierTip",
     "Shift+F7 puts the figure under the mouse pointer: in a 2D view on the two axes of the view (the depth stays), in the camera with "
     "its feet on the surface. A right click in a view offers the same.",
     "Shift+F7 stellt die Figur unter den Mauszeiger: in einer 2D-Ansicht auf die beiden Achsen der Ansicht (die Tiefe bleibt), in der "
     "Kamera mit den Füßen auf die Fläche. Ein Rechtsklick in einer Ansicht bietet dasselbe.",
     "Shift+F7 把人物放到鼠标指针下:在 2D 视图中放到视图的两条轴上(深度不变),在相机中双脚落在表面上。在视图中右键单击也提供同样的功能。",
     "Shift+F7 でフィギュアをマウスポインターの下に置きます: 2D ビューではビューの 2 軸上(奥行きはそのまま)、カメラでは足を面の上に。ビューを"
     "右クリックしても同じことができます。",
     "Mayús+F7 pone la figura bajo el puntero: en una vista 2D sobre los dos ejes de la vista (la profundidad se mantiene), en la cámara "
     "con los pies sobre la superficie. Un clic derecho en una vista ofrece lo mismo.",
     "Shift+F7 postaví postavu pod ukazatel myši: ve 2D pohledu na obě osy pohledu (hloubka zůstane), v kameře chodidly na plochu. Totéž "
     "nabízí pravé kliknutí v pohledu."),
    ("KeCwFigurKnopf", "Test figure", "Testfigur", "测试人物", "テストフィギュア", "Figura de prueba", "Testovací postava"),
    ("KeCwFigurKnopf2", "RightClick: menu", "Rechtsklick: Menü", "右键:菜单", "右クリック: メニュー", "Clic derecho: menú",
     "Pravé kliknutí: nabídka"),
    # --- Punkt 2: Import mit Massstab und Achsen ---
    ("KeCwVorlage", "Template", "Vorlage", "模板", "テンプレート", "Plantilla", "Šablona"),
    ("KeCwVorHeute", "Default (as before)", "Vorgabe (wie bisher)", "默认(与以前相同)", "既定(従来どおり)", "Predeterminado (como antes)",
     "Výchozí (jako dosud)"),
    ("KeCwVorBf3", "Battlefield 3 / Y up", "Battlefield 3 / Y oben", "Battlefield 3 / Y 向上", "Battlefield 3 / Y が上",
     "Battlefield 3 / Y arriba", "Battlefield 3 / Y nahoru"),
    ("KeCwVorBf2", "Battlefront 2 / Z up", "Battlefront 2 / Z oben", "Battlefront 2 / Z 向上", "Battlefront 2 / Z が上",
     "Battlefront 2 / Z arriba", "Battlefront 2 / Z nahoru"),
    ("KeCwVorSource", "Source / GMod (x 0.889)", "Source / GMod (x 0,889)", "Source / GMod(x 0.889)", "Source / GMod(x 0.889)",
     "Source / GMod (x 0,889)", "Source / GMod (x 0,889)"),
    ("KeCwVorEigene", "Custom axes", "Eigene Achsen", "自定义坐标轴", "カスタム軸", "Ejes propios", "Vlastní osy"),
    ("KeCwVorlageTip",
     "How the file's axes become the map's (Z up): Battlefield 3 and other Y-up formats as (x, -z, y), Battlefront 2 and Source with Z up "
     "as they are, Source/GMod also scaled by 64 / 72 (the player is 72 units tall there, 64 here). Custom: choose the source axis for X, Y "
     "and Z. The choice is remembered; the default reads the file as before.",
     "Wie die Achsen der Datei zu denen der Karte werden (Z oben): Battlefield 3 und andere Formate mit Y oben als (x, -z, y), Battlefront 2 "
     "und Source mit Z oben wie sie sind, Source/GMod dazu mit 64 / 72 skaliert (die Spielfigur ist dort 72 Einheiten hoch, hier 64). "
     "Eigene: die Achse der Datei für X, Y und Z wählen. Die Wahl wird gemerkt; die Vorgabe liest die Datei wie bisher.",
     "文件的坐标轴如何变成地图的坐标轴(Z 向上):Battlefield 3 及其他 Y 向上的格式按 (x, -z, y),Battlefront 2 和 Source 为 Z 向上,保持不变,"
     "Source/GMod 另外按 64 / 72 缩放(那里的玩家高 72 单位,这里是 64)。自定义:为 X、Y、Z 选择文件的轴。选择会被记住;默认按以前的方式读取文件。",
     "ファイルの軸をマップの軸(Z が上)にする方法: Battlefield 3 などの Y が上の形式は (x, -z, y)、Battlefront 2 と Source は Z が上のまま、"
     "Source/GMod はさらに 64 / 72 で拡大縮小します(そちらのプレイヤーは 72 単位、こちらは 64)。カスタム: X、Y、Z に使うファイルの軸を選びます。"
     "選択は記憶されます。既定ではこれまでどおりに読みます。",
     "Cómo los ejes del archivo pasan a ser los del mapa (Z arriba): Battlefield 3 y otros formatos con Y arriba como (x, -z, y), "
     "Battlefront 2 y Source con Z arriba tal cual, Source/GMod además escalado por 64 / 72 (allí el jugador mide 72 unidades, aquí 64). "
     "Propios: elige el eje del archivo para X, Y y Z. La elección se recuerda; el predeterminado lee el archivo como antes.",
     "Jak se osy souboru stanou osami mapy (Z nahoru): Battlefield 3 a jiné formáty s Y nahoru jako (x, -z, y), Battlefront 2 a Source se "
     "Z nahoru beze změny, Source/GMod navíc v měřítku 64 / 72 (hráč je tam vysoký 72 jednotek, zde 64). Vlastní: zvolte osu souboru pro "
     "X, Y a Z. Volba se pamatuje; výchozí čte soubor jako dosud."),
    ("KeCwUmdrehen", "Flip winding / normals", "Umlauf/Normalen umdrehen", "翻转环绕方向/法线", "巻き方向/法線を反転",
     "Invertir orden / normales", "Obrátit pořadí/normály"),
    ("KeCwUmdrehenTip",
     "For models whose triangles run the other way round (for example from Empire at War): the faces would point inwards and show the "
     "wrong side in the game.",
     "Für Modelle, deren Dreiecke andersherum laufen (zum Beispiel aus Empire at War): die Flächen zeigten sonst nach innen und im Spiel "
     "die falsche Seite.",
     "用于三角形环绕方向相反的模型(例如来自 Empire at War):否则面会朝内,在游戏中显示错误的一面。",
     "三角形が逆向きに巡るモデル向けです(たとえば Empire at War): そのままだと面が内側を向き、ゲームで裏側が見えます。",
     "Para modelos cuyos triángulos van al revés (por ejemplo de Empire at War): si no, las caras mirarían hacia dentro y en el juego se "
     "vería el lado equivocado.",
     "Pro modely, jejichž trojúhelníky běží opačně (například z Empire at War): plochy by jinak mířily dovnitř a ve hře by byla vidět "
     "špatná strana."),
    ("KeCwGroesse", "Size %s x %s x %s units = %s test figures high", "Größe %s x %s x %s Einheiten = %s Testfiguren hoch",
     "尺寸 %s x %s x %s 单位 = %s 个测试人物高", "サイズ %s x %s x %s 単位 = テストフィギュア %s 体分の高さ",
     "Tamaño %s x %s x %s unidades = %s figuras de prueba de alto", "Velikost %s x %s x %s jednotek = %s testovacích postav vysoko"),
    ("KeCwAufFigur", "Height = test figure", "Höhe = Testfigur", "高度 = 测试人物", "高さ = テストフィギュア", "Altura = figura de prueba",
     "Výška = testovací postava"),
    ("KeCwAufFigurTip", "Sets the scale so that the model is as tall as the player (64 units) - for a character model.",
     "Setzt den Maßstab so, dass das Modell so hoch ist wie die Spielfigur (64 Einheiten) - für ein Figurenmodell.",
     "设置缩放,使模型与玩家一样高(64 单位)——适用于角色模型。", "モデルがプレイヤーと同じ高さ(64 単位)になるように倍率を設定します。キャラクターモデル向けです。",
     "Ajusta la escala para que el modelo sea tan alto como el jugador (64 unidades), para un modelo de personaje.",
     "Nastaví měřítko tak, aby byl model vysoký jako hráč (64 jednotek) - pro model postavy."),
    ("KeCwVorschauTip", "Front view (X to the right, Z up) of the model next to the test figure (32 x 64, the STEPSIZE mark at 18).",
     "Vorderansicht (X nach rechts, Z nach oben) des Modells neben der Testfigur (32 x 64, die Marke STEPSIZE bei 18).",
     "模型的正视图(X 向右,Z 向上),旁边是测试人物(32 x 64,STEPSIZE 标记在 18)。",
     "モデルの正面図(X が右、Z が上)とその横のテストフィギュア(32 x 64、STEPSIZE の印は 18)。",
     "Vista frontal (X a la derecha, Z arriba) del modelo junto a la figura de prueba (32 x 64, la marca STEPSIZE en 18).",
     "Pohled zepředu (X doprava, Z nahoru) na model vedle testovací postavy (32 x 64, značka STEPSIZE ve výšce 18)."),
    # --- Punkte 3 bis 6: Werkzeuge ---
    ("KeCwWzMenue", "Clone Wars tools", "Clone-Wars-Werkzeuge", "Clone Wars 工具", "Clone Wars ツール", "Herramientas Clone Wars",
     "Nástroje Clone Wars"),
    ("KeCwRaum", "Scale room...", "Raum skalieren...", "缩放房间...", "部屋を拡大縮小...", "Escalar sala...", "Změnit měřítko místnosti..."),
    ("KeCwRaumTip",
     "Scales the selection (a room) about a point by a factor and marks it with an info_notnull with cw_scale (and a comment in the .map) - "
     "data only, the game ignores both. Save it as its own part with File > Save selected.",
     "Skaliert die Auswahl (einen Raum) um einen Punkt mit einem Faktor und markiert sie mit einem info_notnull mit cw_scale (und einem "
     "Kommentar in der .map) - nur Daten, das Spiel übergeht beides. Als eigenes Teil speichern mit Datei > Auswahl speichern.",
     "按一个系数围绕某点缩放所选内容(一个房间),并用带 cw_scale 的 info_notnull(以及 .map 中的注释)标记——只是数据,游戏会忽略两者。"
     "用 文件 > 保存所选 将其另存为单独的部分。",
     "選択範囲(部屋)をある点を中心に係数で拡大縮小し、cw_scale 付きの info_notnull(と .map 内のコメント)で印を付けます。データだけで、"
     "ゲームはどちらも無視します。ファイル > 選択範囲を保存 で独立した部品として保存します。",
     "Escala la selección (una sala) alrededor de un punto por un factor y la marca con un info_notnull con cw_scale (y un comentario en el "
     ".map); solo datos, el juego ignora ambos. Guárdala como pieza propia con Archivo > Guardar selección.",
     "Změní měřítko výběru (místnosti) kolem bodu o faktor a označí ji entitou info_notnull s cw_scale (a komentářem v .map) - jen data, "
     "hra obojí přeskočí. Jako samostatný díl ji uložte přes Soubor > Uložit výběr."),
    ("KeCwRaumFaktor", "Factor", "Faktor", "系数", "係数", "Factor", "Faktor"),
    ("KeCwRaumName", "Room name", "Raumname", "房间名", "部屋名", "Nombre de la sala", "Název místnosti"),
    ("KeCwRaumPunkt", "About", "Um", "围绕", "中心", "Alrededor de", "Kolem"),
    ("KeCwRaumMitte", "center of the selection, bottom", "Mitte der Auswahl, unten", "所选内容的中心,底部", "選択範囲の中心、下端",
     "centro de la selección, abajo", "střed výběru, dole"),
    ("KeCwRaumOk", "Scale", "Skalieren", "缩放", "拡大縮小", "Escalar", "Změnit měřítko"),
    ("KeCwRaumSpeichern", "Scale and save as part...", "Skalieren und als Teil speichern...", "缩放并另存为部分...", "拡大縮小して部品として保存...",
     "Escalar y guardar como pieza...", "Změnit měřítko a uložit jako díl..."),
    ("KeCwRaumFertig", "Room scaled by %s: %d primitives, %d entities moved, marker info_notnull with cw_scale",
     "Raum mit %s skaliert: %d Primitive, %d Entities verschoben, Marker info_notnull mit cw_scale",
     "房间已按 %s 缩放:%d 个图元,移动了 %d 个实体,标记 info_notnull 带 cw_scale",
     "部屋を %s 倍にしました: プリミティブ %d、移動したエンティティ %d、cw_scale 付きの印 info_notnull",
     "Sala escalada por %s: %d primitivas, %d entidades movidas, marcador info_notnull con cw_scale",
     "Místnost změněna o %s: %d primitiv, %d entit posunuto, značka info_notnull s cw_scale"),
    ("KeCwRampe", "Stairs to clip ramp", "Treppe zu Clip-Rampe", "楼梯转为 clip 坡道", "階段をクリップスロープに", "Escalera a rampa clip",
     "Schody na clip rampu"),
    ("KeCwRampeTip",
     "Select stairs (brushes or a misc_model). A smooth ramp of textures/system/clip goes through the front edges of the steps, from the "
     "bottom of the stairs to the top step, in the layer Collision; the visible steps lose their collision (a hidden side becomes "
     "system/nodraw, which q3map2 makes non-solid; a model loses its clip spawnflags; pure collision brushes of the selection are removed).",
     "Eine Treppe wählen (Brushes oder ein misc_model). Eine glatte Rampe aus textures/system/clip läuft durch die Vorderkanten der Stufen, "
     "von der Unterkante der Treppe bis zum obersten Tritt, in der Ebene Collision; die sichtbaren Stufen verlieren ihre Kollision (eine "
     "verdeckte Seite wird system/nodraw, das q3map2 nicht fest macht; ein Modell verliert seine Clip-Spawnflags; reine Kollisionspinsel "
     "der Auswahl fallen weg).",
     "选择楼梯(画刷或一个 misc_model)。一个由 textures/system/clip 构成的平滑坡道穿过各级台阶的前缘,从楼梯底部到最上面的踏面,放在图层 "
     "Collision 中;可见的台阶失去碰撞(一个被遮挡的面变成 system/nodraw,q3map2 会使其非实体;模型失去其 clip 生成标志;所选内容中的纯碰撞画刷被删除)。",
     "階段(ブラシまたは misc_model)を選びます。textures/system/clip の滑らかなスロープが段の前縁を通り、階段の下端から一番上の踏み面まで、"
     "レイヤー Collision に作られます。見える段はコリジョンを失います(隠れた面が system/nodraw になり q3map2 が非固体にします。モデルは clip "
     "のスポーンフラグを失い、選択範囲内の純粋なコリジョンブラシは削除されます)。",
     "Selecciona una escalera (brushes o un misc_model). Una rampa lisa de textures/system/clip pasa por los bordes delanteros de los "
     "escalones, desde la base de la escalera hasta el escalón superior, en la capa Collision; los escalones visibles pierden su colisión "
     "(una cara oculta pasa a system/nodraw, que q3map2 hace no sólida; un modelo pierde sus spawnflags de clip; los brushes de solo "
     "colisión de la selección se eliminan).",
     "Vyberte schody (brushe nebo misc_model). Hladká rampa z textures/system/clip vede přes přední hrany schodů, od spodku schodiště po "
     "nejvyšší stupeň, ve vrstvě Collision; viditelné schody ztratí kolizi (jedna skrytá strana se změní na system/nodraw, kterou q3map2 "
     "udělá nepevnou; model ztratí své clip spawnflagy; čistě kolizní brushe výběru se odstraní)."),
    ("KeCwRampeFertig", "Clip ramp: %d steps, slope %s, %d step brushes non-solid, %d models without clip, %d collision brushes removed",
     "Clip-Rampe: %d Stufen, Steigung %s, %d Stufen-Pinsel nicht fest, %d Modelle ohne Clip, %d Kollisionspinsel entfernt",
     "Clip 坡道:%d 级台阶,坡度 %s,%d 个台阶画刷设为非实体,%d 个模型去掉 clip,删除 %d 个碰撞画刷",
     "クリップスロープ: 段 %d、勾配 %s、非固体にした段ブラシ %d、clip を外したモデル %d、削除したコリジョンブラシ %d",
     "Rampa clip: %d escalones, pendiente %s, %d brushes de escalón no sólidos, %d modelos sin clip, %d brushes de colisión eliminados",
     "Clip rampa: %d schodů, sklon %s, %d brushů schodů nepevných, %d modelů bez clipu, %d kolizních brushů odstraněno"),
    ("KeCwRampeSteil", "The ramp is steeper than walkable (MIN_WALK_NORMAL 0.7): the player slides down - make the steps deeper.",
     "Die Rampe ist steiler als begehbar (MIN_WALK_NORMAL 0,7): die Spielfigur rutscht ab - die Stufen tiefer machen.",
     "坡道比可行走的更陡(MIN_WALK_NORMAL 0.7):玩家会滑下来——请把台阶做得更深。",
     "スロープが歩ける勾配より急です(MIN_WALK_NORMAL 0.7): プレイヤーは滑り落ちます。段を深くしてください。",
     "La rampa es más empinada de lo transitable (MIN_WALK_NORMAL 0,7): el jugador resbala; haz los escalones más profundos.",
     "Rampa je strmější, než se dá chodit (MIN_WALK_NORMAL 0,7): hráč sklouzne - prohlubte schody."),
    ("KeCwWandEbene", "Smooth clip wall (plane)", "Glatte Clip-Wand (Ebene)", "平滑 clip 墙(平面)", "滑らかなクリップ壁(平面)",
     "Pared clip lisa (plano)", "Hladká clip stěna (rovina)"),
    ("KeCwWandHuelle", "Smooth clip wall (convex hull)", "Glatte Clip-Wand (Hülle)", "平滑 clip 墙(凸包)", "滑らかなクリップ壁(凸包)",
     "Pared clip lisa (envolvente)", "Hladká clip stěna (obal)"),
    ("KeCwWandTip",
     "Select an irregular wall (brushes, patches or a misc_model). Plane: a clip block along the wall, its front at the point that sticks "
     "out most, on the side of the camera. Hull: the convex hull of the selection. In the layer Collision.",
     "Eine unregelmäßige Wand wählen (Brushes, Patches oder ein misc_model). Ebene: ein Clip-Quader entlang der Wand, vorn am Punkt, der "
     "am weitesten vorsteht, auf der Seite der Kamera. Hülle: die konvexe Hülle der Auswahl. In der Ebene Collision.",
     "选择一面不规则的墙(画刷、面片或 misc_model)。平面:沿墙放置一个 clip 长方体,其前面位于最突出的点,在相机一侧。凸包:所选内容的凸包。放在图层 Collision 中。",
     "不規則な壁(ブラシ、パッチ、misc_model)を選びます。平面: 壁に沿ったクリップの直方体で、前面はカメラ側で最も出っ張った点に合わせます。凸包: 選択範囲の凸包です。"
     "レイヤー Collision に作られます。",
     "Selecciona una pared irregular (brushes, patches o un misc_model). Plano: un bloque clip a lo largo de la pared, con el frente en el "
     "punto que más sobresale, del lado de la cámara. Envolvente: la envolvente convexa de la selección. En la capa Collision.",
     "Vyberte nepravidelnou stěnu (brushe, patche nebo misc_model). Rovina: clip kvádr podél stěny, s přední stranou v nejvíce "
     "vystupujícím bodě, na straně kamery. Obal: konvexní obal výběru. Ve vrstvě Collision."),
    ("KeCwKollision", "Show layer Collision", "Ebene Collision zeigen", "显示图层 Collision", "レイヤー Collision を表示",
     "Mostrar capa Collision", "Zobrazit vrstvu Collision"),
    ("KeCwKollisionTip", "Shows or hides the clip brushes of these tools (they are in their own layer, see the Layers browser).",
     "Blendet die Clip-Pinsel dieser Werkzeuge ein oder aus (sie liegen in einer eigenen Ebene, siehe den Ebenen-Browser).",
     "显示或隐藏这些工具的 clip 画刷(它们在单独的图层中,参见图层浏览器)。",
     "これらのツールのクリップブラシを表示/非表示にします(専用のレイヤーにあります。レイヤーブラウザーを参照)。",
     "Muestra u oculta los brushes clip de estas herramientas (están en su propia capa, ver el navegador de capas).",
     "Zobrazí nebo skryje clip brushe těchto nástrojů (jsou ve vlastní vrstvě, viz prohlížeč vrstev)."),
    ("KeCwFehler", "Clone Wars tool: %s", "Clone-Wars-Werkzeug: %s", "Clone Wars 工具:%s", "Clone Wars ツール: %s", "Herramienta Clone Wars: %s",
     "Nástroj Clone Wars: %s"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
