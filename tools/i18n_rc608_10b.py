# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc608 Teil 10b (Schwertlicht unter Movie Duels) fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("MapSaberLightIntensity", "Saber light (cg_saberLightIntensity)", "Schwertlicht (cg_saberLightIntensity)",
     "光剑光照 (cg_saberLightIntensity)", "セイバーの光 (cg_saberLightIntensity)", "Luz del sable (cg_saberLightIntensity)",
     "Světlo meče (cg_saberLightIntensity)"),
    ("MapSaberLightIntensityHint",
     "Movie Duels only, like cg_saberLightIntensity in the game: multiplies the colour of the light a lightsaber casts on figures and surfaces. A fresh installation starts at 1.07; the game's own settings menu offers 0.5 to 1.5. Above 1 the light on surfaces can turn darker near the blade, as in the game.",
     "Nur Movie Duels, wie cg_saberLightIntensity im Spiel: vervielfacht die Farbe des Lichts, das ein Lichtschwert auf Figuren und Flächen wirft. Eine frische Installation beginnt bei 1,07; das Einstellungsmenü des Spiels bietet 0,5 bis 1,5. Über 1 kann das Licht auf Flächen nahe der Klinge dunkler werden, wie im Spiel.",
     "仅限 Movie Duels,与游戏中的 cg_saberLightIntensity 相同:放大光剑投射到角色和表面上的光的颜色。全新安装从 1.07 开始;游戏自身的设置菜单提供 0.5 到 1.5。大于 1 时,表面上靠近剑刃的光可能会变暗,与游戏中相同。",
     "Movie Duels のみ。ゲームの cg_saberLightIntensity と同じで、ライトセーバーがキャラクターや面に落とす光の色を何倍にもします。新規インストールでは 1.07 から始まり、ゲーム自身の設定メニューでは 0.5 から 1.5 を選べます。1 を超えると、ゲームと同じように刃の近くの面の光が暗くなることがあります。",
     "Solo Movie Duels, como cg_saberLightIntensity en el juego: multiplica el color de la luz que un sable de luz proyecta sobre figuras y superficies. Una instalación nueva empieza en 1,07; el menú de ajustes del juego ofrece de 0,5 a 1,5. Por encima de 1, la luz sobre las superficies puede oscurecerse cerca de la hoja, como en el juego.",
     "Jen Movie Duels, jako cg_saberLightIntensity ve hře: násobí barvu světla, které světelný meč vrhá na postavy a plochy. Čerstvá instalace začíná na 1,07; nabídka nastavení hry nabízí 0,5 až 1,5. Nad 1 může světlo na plochách u čepele ztmavnout, jako ve hře."),
    ("MapSaberLightRadius", "Saber light reach (cg_saberLightRadius)", "Reichweite Schwertlicht (cg_saberLightRadius)",
     "光剑光照范围 (cg_saberLightRadius)", "セイバーの光の範囲 (cg_saberLightRadius)", "Alcance de la luz del sable (cg_saberLightRadius)",
     "Dosah světla meče (cg_saberLightRadius)"),
    ("MapSaberLightRadiusHint",
     "Movie Duels only, like cg_saberLightRadius in the game: multiplies the radius of a lightsaber's light. A fresh installation starts at 0.70; the game's own settings menu offers 0.5 to 2.5. A larger radius lights more of the floor and turns the light on a figure towards the blade.",
     "Nur Movie Duels, wie cg_saberLightRadius im Spiel: vervielfacht den Radius des Lichts eines Lichtschwerts. Eine frische Installation beginnt bei 0,70; das Einstellungsmenü des Spiels bietet 0,5 bis 2,5. Ein größerer Radius hellt mehr vom Boden auf und dreht das Licht auf einer Figur zur Klinge hin.",
     "仅限 Movie Duels,与游戏中的 cg_saberLightRadius 相同:放大光剑光照的半径。全新安装从 0.70 开始;游戏自身的设置菜单提供 0.5 到 2.5。半径越大,照亮的地面越多,角色身上的光也越偏向剑刃。",
     "Movie Duels のみ。ゲームの cg_saberLightRadius と同じで、ライトセーバーの光の半径を何倍にもします。新規インストールでは 0.70 から始まり、ゲーム自身の設定メニューでは 0.5 から 2.5 を選べます。半径が大きいほど床の広い範囲が明るくなり、キャラクターに当たる光が刃の方向に向きます。",
     "Solo Movie Duels, como cg_saberLightRadius en el juego: multiplica el radio de la luz de un sable de luz. Una instalación nueva empieza en 0,70; el menú de ajustes del juego ofrece de 0,5 a 2,5. Un radio mayor ilumina más suelo y gira la luz sobre una figura hacia la hoja.",
     "Jen Movie Duels, jako cg_saberLightRadius ve hře: násobí poloměr světla světelného meče. Čerstvá instalace začíná na 0,70; nabídka nastavení hry nabízí 0,5 až 2,5. Větší poloměr osvítí víc podlahy a natočí světlo na postavě k čepeli."),
]


def zeilen_vier():
    """(Kennung, English, Deutsch, 中文, 日本語) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
