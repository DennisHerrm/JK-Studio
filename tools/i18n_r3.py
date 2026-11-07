# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc610 R3 (gespiegelte Modelle, Glow im Echtzeit-Licht) fuer tools/gen_i18n.py und die weiteren
Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("MeMdMpFehlt",
     "The Movie Duels multiplayer menus are not installed: ui/MD_MP (menudef.h, the menu list MovieDuels-MP-Menus.txt and their "
     "fonts) is missing from the game data, so this menu cannot be shown as in the game - texts stay empty. Movie Duels has no finished "
     "multiplayer menu set yet; the single player menus (ui/) work.",
     "Die Mehrspieler-Menüs von Movie Duels sind nicht installiert: ui/MD_MP (menudef.h, die Menüliste MovieDuels-MP-Menus.txt und "
     "ihre Schriften) fehlt in den Spieldaten, darum kann dieses Menü nicht wie im Spiel erscheinen - Texte bleiben leer. Movie Duels "
     "hat noch keinen fertigen Mehrspieler-Menüsatz; die Einzelspieler-Menüs (ui/) gehen.",
     "未安装 Movie Duels 多人菜单:游戏数据中缺少 ui/MD_MP(menudef.h、菜单列表 MovieDuels-MP-Menus.txt 及其字体),因此该菜单无法按游戏中的样子"
     "显示——文字保持为空。Movie Duels 还没有完成的多人菜单集;单人菜单(ui/)可以使用。",
     "Movie Duels のマルチプレイメニューがインストールされていません: ゲームデータに ui/MD_MP (menudef.h、メニュー一覧 "
     "MovieDuels-MP-Menus.txt とそのフォント) がないため、このメニューはゲームと同じには表示できません。テキストは空のままです。"
     "Movie Duels にはまだ完成したマルチプレイ用メニューセットがありません。シングルプレイのメニュー (ui/) は使えます。",
     "Los menús multijugador de Movie Duels no están instalados: falta ui/MD_MP (menudef.h, la lista de menús MovieDuels-MP-Menus.txt y "
     "sus fuentes) en los datos del juego, así que este menú no puede mostrarse como en el juego; los textos quedan vacíos. Movie Duels "
     "aún no tiene un conjunto de menús multijugador terminado; los menús de un jugador (ui/) funcionan.",
     "Menu Movie Duels pro více hráčů nejsou nainstalována: v herních datech chybí ui/MD_MP (menudef.h, seznam menu "
     "MovieDuels-MP-Menus.txt a jejich písma), proto toto menu nemůže vypadat jako ve hře - texty zůstanou prázdné. Movie Duels zatím "
     "nemá hotovou sadu menu pro více hráčů; menu pro jednoho hráče (ui/) fungují."),
    ("KeOptPrModellGespiegelt", "Mirrored model the game shows inside out", "Gespiegeltes Modell, das das Spiel innen zeigt",
     "游戏中显示为内侧的镜像模型", "ゲームで内側が見える反転モデル", "Modelo reflejado que el juego muestra por dentro",
     "Zrcadlený model, který hra ukáže zevnitř"),
    ("KeOptPrModellGespiegeltE",
     "These entities are loaded by the game at run time with a scale that mirrors them (modelscale_vec with one or three negative "
     "axes, e.g. after Flip X). The game draws them without turning the triangles around, so you see their inside - the camera shows "
     "it the same way. Only misc_model is turned around when q3map2 bakes it in. Fix: use a misc_model, or rotate instead of mirroring "
     "(two mirrored axes are a rotation).",
     "Diese Entities lädt das Spiel zur Laufzeit mit einer Skalierung, die sie spiegelt (modelscale_vec mit einer oder drei negativen "
     "Achsen, z. B. nach Flip X). Das Spiel zeichnet sie, ohne die Dreiecke umzudrehen - man sieht ihr Inneres; die Kamera zeigt es "
     "ebenso. Nur misc_model dreht q3map2 beim Einbacken um. Abhilfe: ein misc_model nehmen oder drehen statt spiegeln (zwei "
     "gespiegelte Achsen sind eine Drehung).",
     "这些实体由游戏在运行时加载,其缩放会把它们镜像(modelscale_vec 有一个或三个负轴,例如 Flip X 之后)。游戏绘制时不会翻转三角形,所以会看到"
     "它们的内侧——摄像机也同样显示。只有 misc_model 会在 q3map2 烘焙时被翻转。解决:改用 misc_model,或用旋转代替镜像(两个镜像轴等于一次旋转)。",
     "これらのエンティティはゲームが実行時に、反転するスケール (負の軸が 1 つまたは 3 つの modelscale_vec。例: Flip X の後) で読み込みます。"
     "ゲームは三角形を裏返さずに描くので内側が見えます。カメラも同じように表示します。q3map2 が焼き込むときに裏返すのは misc_model だけです。"
     "対処: misc_model を使うか、反転の代わりに回転します (反転した軸が 2 つなら回転と同じです)。",
     "El juego carga estas entidades en tiempo de ejecución con una escala que las refleja (modelscale_vec con uno o tres ejes "
     "negativos, p. ej. tras Flip X). El juego las dibuja sin dar la vuelta a los triángulos, así que se ve su interior; la cámara lo "
     "muestra igual. q3map2 solo da la vuelta a misc_model al integrarlo. Solución: usar un misc_model, o girar en lugar de reflejar "
     "(dos ejes reflejados son un giro).",
     "Tyto entity hra načítá za běhu s měřítkem, které je zrcadlí (modelscale_vec s jednou nebo třemi zápornými osami, např. po Flip X). "
     "Hra je kreslí bez otočení trojúhelníků, takže je vidět jejich vnitřek - kamera to ukazuje stejně. Jen misc_model otočí q3map2 při "
     "zapečení. Náprava: použít misc_model, nebo místo zrcadlení otočit (dvě zrcadlené osy jsou otočení)."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
