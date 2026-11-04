# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc609 W3a (Wegpunkt-Erzeugung in Innenraeumen und auf Treppen) fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("WegErzeugenTipp",
     "Walks like an NPC in the game from the player start, the characters, the nav goals and the waypoints already on the map (steps up to 18, slopes up to the game's limit, doors open) and spreads waypoints over all floor it reaches: on the grid, at the top and bottom of every stair and ramp, and in passages the grid misses. Connections pass the game's edge test and are written as target..target4. Replaces earlier created ones; one undo step.",
     "Läuft wie ein NPC im Spiel vom Spielerstart, den Figuren, den Laufzielen und den Wegpunkten auf der Karte aus (Stufen bis 18, Schrägen bis zur Grenze des Spiels, Türen offen) und verteilt Wegpunkte über allen Boden, den es erreicht: im Raster, oben und unten an jeder Treppe und Rampe und in Durchgängen, die das Raster verfehlt. Die Verbindungen bestehen die Kantenprüfung des Spiels und stehen als target..target4 darin. Ersetzt früher erzeugte; ein Rückgängig-Schritt.",
     "像游戏中的 NPC 一样,从玩家出生点、角色、导航目标和地图上已有的路径点出发行走(台阶最高 18,坡度不超过游戏的上限,门视为打开),并在它能到达的所有地面上分布路径点:按网格、在每段楼梯和坡道的顶部与底部,以及网格错过的通道中。连接都通过游戏的连线检查,并写为 target..target4。替换之前生成的;一步撤销。",
     "ゲームの NPC と同じように、プレイヤー開始位置・キャラクター・ナビゴール・マップ上の既存ウェイポイントから歩き(段差 18 まで、傾斜はゲームの上限まで、ドアは開いた状態)、たどり着けるすべての床にウェイポイントを置きます。グリッド上、各階段とスロープの上と下、グリッドが外す通路にも置きます。接続はゲームの辺チェックに通り、target..target4 として書かれます。以前生成したものを置き換え、元に戻すは 1 回です。",
     "Camina como un PNJ del juego desde el inicio del jugador, los personajes, los objetivos de navegación y los puntos de ruta que ya hay en el mapa (escalones de hasta 18, pendientes hasta el límite del juego, puertas abiertas) y reparte puntos de ruta por todo el suelo que alcanza: en la cuadrícula, arriba y abajo de cada escalera y rampa y en los pasos que la cuadrícula no alcanza. Las conexiones superan la prueba de aristas del juego y se escriben como target..target4. Sustituye los creados antes; un paso de deshacer.",
     "Prochází jako NPC ve hře od startu hráče, postav, navigačních cílů a bodů trasy, které už na mapě jsou (schody do 18, svahy do limitu hry, dveře otevřené), a rozmístí body trasy po celé podlaze, na kterou dojde: v mřížce, nahoře a dole u každého schodiště a rampy a v průchodech, které mřížka mine. Spojení projdou testem hran hry a zapíší se jako target..target4. Nahradí dříve vytvořené; jeden krok zpět."),
    ("WegRasterWeiter",
     "Spacing raised to %.0f: the game takes at most %d waypoints",
     "Abstand auf %.0f vergrößert: Das Spiel nimmt höchstens %d Wegpunkte",
     "间距已增大到 %.0f:游戏最多接受 %d 个路径点",
     "間隔を %.0f に広げました: ゲームが受け付けるウェイポイントは最大 %d 個です",
     "Espaciado aumentado a %.0f: el juego admite como máximo %d puntos de ruta",
     "Rozteč zvětšena na %.0f: hra přijme nejvýše %d bodů trasy"),
    ("WegNichtAngebunden",
     "%d {part|parts} of the network not connected (no straight connection the game keeps), the first at %.0f %.0f %.0f - place a waypoint there and connect it",
     "%d {Teil|Teile} des Netzes nicht angebunden (keine gerade Verbindung, die das Spiel behält), der erste bei %.0f %.0f %.0f - setz dort einen Wegpunkt und verbinde ihn",
     "有 %d 部分路网未连接(没有游戏会保留的直线连接),第一处位于 %.0f %.0f %.0f——在那里放一个路径点并连接",
     "ネットワークの %d 個の部分がつながっていません(ゲームが残す直線の接続がありません)。最初の場所 %.0f %.0f %.0f にウェイポイントを置いて接続してください",
     "%d {parte|partes} de la red sin conectar (ninguna conexión recta que el juego conserve), la primera en %.0f %.0f %.0f: coloca allí un punto de ruta y conéctalo",
     "%d {část|části|částí} sítě nepřipojeno (žádné rovné spojení, které by hra ponechala), první u %.0f %.0f %.0f - polož tam bod trasy a spoj ho"),
]


def zeilen_vier():
    """(Kennung, English, Deutsch, 中文, 日本語) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
