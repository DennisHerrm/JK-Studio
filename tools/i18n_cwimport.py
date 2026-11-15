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
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
