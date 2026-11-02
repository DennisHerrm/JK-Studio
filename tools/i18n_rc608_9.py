# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc608 Teil 9 (Spieler in Cutscenes, Animationseinstellungen) fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("MapShowPlayer", "Show player", "Spieler zeigen", "显示玩家", "プレイヤーを表示", "Mostrar jugador", "Zobrazit hráče"),
    ("MapShowPlayerHint",
     "Off (default): in a scene with a camera track the preview does not draw the player figure, except in the parts outside the camera where the script itself addresses the player (affect player). This differs from the game on purpose. On: exactly like the game, hidden only while the script camera is on. Only the picture changes: aim, turrets, sound and the exported file stay the same.",
     "Aus (Vorgabe): In einer Szene mit Kameraspur zeichnet die Vorschau die Spielerfigur nicht, außer in den Abschnitten außerhalb der Kamera, in denen das Skript den Spieler selbst anspricht (affect player). Das weicht mit Absicht vom Spiel ab. An: genau wie im Spiel, verborgen nur, solange die Skriptkamera an ist. Nur das Bild ändert sich: Ziele, Geschütze, Ton und die exportierte Datei bleiben gleich.",
     "关闭(默认):在带有摄像机轨道的场景中,预览不绘制玩家角色,但摄像机之外由脚本亲自操作玩家(affect player)的部分除外。这是有意与游戏不同的。开启:与游戏完全相同,仅在脚本摄像机开启时隐藏。只有画面变化:瞄准、炮塔、声音和导出的文件保持不变。",
     "オフ(既定):カメラトラックのあるシーンでは、プレビューはプレイヤーを描画しません。ただし、カメラの外でスクリプトがプレイヤーを直接操作する部分(affect player)は除きます。これは意図的にゲームと異なります。オン:ゲームとまったく同じで、スクリプトカメラがオンの間だけ隠れます。変わるのは画面だけで、照準、タレット、サウンド、書き出したファイルは同じです。",
     "Desactivado (predeterminado): en una escena con pista de cámara, la vista previa no dibuja al jugador, salvo en las partes fuera de la cámara en las que el script se dirige a él (affect player). Esto difiere del juego a propósito. Activado: exactamente como en el juego, oculto solo mientras la cámara del script está activa. Solo cambia la imagen: los objetivos, las torretas, el sonido y el archivo exportado no cambian.",
     "Vypnuto (výchozí): ve scéně se stopou kamery náhled hráče nekreslí, kromě částí mimo kameru, ve kterých se skript na hráče sám obrací (affect player). To se od hry záměrně liší. Zapnuto: přesně jako ve hře, skrytý jen po dobu, kdy je zapnutá kamera skriptu. Mění se jen obraz: cíle, věže, zvuk a exportovaný soubor zůstávají stejné."),
    ("MapMotionBone", "Spine correction (cg_motionBoneComp)", "Rumpfausgleich (cg_motionBoneComp)", "脊柱校正 (cg_motionBoneComp)", "背骨の補正 (cg_motionBoneComp)",
     "Corrección de la espalda (cg_motionBoneComp)", "Korekce páteře (cg_motionBoneComp)"),
    ("MapMotionBoneHint",
     "Like cg_motionBoneComp in the game, when legs and upper body play different animations. 0: off. 1: from the Motion bone without roll. 2: with roll, the default of the games. 3: the expensive version, the pelvis against a dummy _humanoid (not in Jedi Outcast, it acts like 2 there).",
     "Wie cg_motionBoneComp im Spiel, wenn Beine und Oberkörper verschiedene Animationen spielen. 0: aus. 1: aus dem Motion-Knochen ohne Rollen. 2: mit Rollen, die Vorgabe der Spiele. 3: die teure Fassung, das Becken gegen einen Dummy-_humanoid (nicht in Jedi Outcast, dort wie 2).",
     "与游戏中的 cg_motionBoneComp 相同,用于双腿和上半身播放不同动画时。0:关闭。1:来自 Motion 骨骼,不含翻滚。2:含翻滚,游戏的默认值。3:高开销版本,骨盆对比虚拟 _humanoid(Jedi Outcast 中没有,等同于 2)。",
     "ゲームの cg_motionBoneComp と同じで、脚と上半身が別のアニメーションを再生するときに働きます。0: オフ。1: Motion ボーンからロールなし。2: ロールあり、ゲームの既定値。3: 高コスト版で、骨盤をダミーの _humanoid と比較します(Jedi Outcast にはなく、2 と同じ動作です)。",
     "Como cg_motionBoneComp en el juego, cuando las piernas y el torso reproducen animaciones distintas. 0: desactivado. 1: desde el hueso Motion sin balanceo. 2: con balanceo, el valor predeterminado de los juegos. 3: la versión costosa, la pelvis contra un _humanoid ficticio (no existe en Jedi Outcast, allí actúa como 2).",
     "Jako cg_motionBoneComp ve hře, když nohy a horní část těla hrají různé animace. 0: vypnuto. 1: z kosti Motion bez náklonu. 2: s náklonem, výchozí hodnota her. 3: náročná verze, pánev proti pomocnému _humanoid (v Jedi Outcast není, tam funguje jako 2)."),
    ("MapTurnAnims", "Turn animation (cg_turnAnims)", "Drehanimation (cg_turnAnims)", "转身动画 (cg_turnAnims)", "旋回アニメーション (cg_turnAnims)",
     "Animación de giro (cg_turnAnims)", "Animace otáčení (cg_turnAnims)"),
    ("MapTurnAnimsHint",
     "Like cg_turnAnims in the game (off by default): a figure that turns on the spot plays the turn animation on its hips, not while the script camera is on.",
     "Wie cg_turnAnims im Spiel (Vorgabe aus): Eine Figur, die sich auf der Stelle dreht, spielt die Drehanimation auf der Hüfte, nicht solange die Skriptkamera an ist.",
     "与游戏中的 cg_turnAnims 相同(默认关闭):原地转身的角色在髋部播放转身动画,脚本摄像机开启时除外。",
     "ゲームの cg_turnAnims と同じです(既定はオフ)。その場で向きを変えるキャラクターが腰で旋回アニメーションを再生します。スクリプトカメラがオンの間は再生しません。",
     "Como cg_turnAnims en el juego (desactivado de forma predeterminada): una figura que gira en el sitio reproduce la animación de giro en la cadera, salvo mientras la cámara del script está activa.",
     "Jako cg_turnAnims ve hře (výchozí vypnuto): postava, která se otáčí na místě, hraje animaci otáčení na bocích, ne po dobu, kdy je zapnutá kamera skriptu."),
    ("MapAnimStyle", "Animation style (g_ActivateAnimationStyle)", "Animationsstil (g_ActivateAnimationStyle)", "动画风格 (g_ActivateAnimationStyle)", "アニメーションスタイル (g_ActivateAnimationStyle)",
     "Estilo de animación (g_ActivateAnimationStyle)", "Styl animace (g_ActivateAnimationStyle)"),
    ("MapAnimStyleHint",
     "Movie Duels only, like g_ActivateAnimationStyle in the game (off by default): humanoid figures use the master animation set and play the variants of their animationstyle. The figures are loaded again. The preview follows the setting for the choice of stance and walk; the saber moves of the game's code are not reproduced.",
     "Nur Movie Duels, wie g_ActivateAnimationStyle im Spiel (Vorgabe aus): Menschenähnliche Figuren nehmen den Meistersatz der Animationen und spielen die Varianten ihres animationstyle. Die Figuren werden neu geladen. Die Vorschau folgt der Einstellung bei der Wahl von Haltung und Gang; die Schwerthiebe aus dem Spielcode sind nicht nachgebildet.",
     "仅限 Movie Duels,与游戏中的 g_ActivateAnimationStyle 相同(默认关闭):人形角色使用主动画集并播放其 animationstyle 的变体。角色会重新加载。预览在选择姿势和步态时遵循此设置;游戏代码中的光剑招式未被重现。",
     "Movie Duels のみ。ゲームの g_ActivateAnimationStyle と同じです(既定はオフ)。人型キャラクターはマスターアニメーションセットを使い、animationstyle のバリエーションを再生します。キャラクターは再読み込みされます。プレビューは構えと歩き方の選択でこの設定に従いますが、ゲームコードのセイバー攻撃は再現していません。",
     "Solo Movie Duels, como g_ActivateAnimationStyle en el juego (desactivado de forma predeterminada): las figuras humanoides usan el conjunto maestro de animaciones y reproducen las variantes de su animationstyle. Las figuras se vuelven a cargar. La vista previa sigue el ajuste para elegir postura y paso; los golpes de sable del código del juego no se reproducen.",
     "Jen Movie Duels, jako g_ActivateAnimationStyle ve hře (výchozí vypnuto): humanoidní postavy používají hlavní sadu animací a hrají varianty svého animationstyle. Postavy se znovu načtou. Náhled se řídí nastavením při volbě postoje a chůze; údery mečem z kódu hry nejsou napodobeny."),
    ("MapMsaa", "Edge smoothing (r_ext_multisample)", "Kantenglättung (r_ext_multisample)", "边缘平滑 (r_ext_multisample)", "エッジの平滑化 (r_ext_multisample)",
     "Suavizado de bordes (r_ext_multisample)", "Vyhlazování hran (r_ext_multisample)"),
    ("MapMsaaHint",
     "Like r_ext_multisample in the game: the edges of surfaces get several samples per pixel (MSAA), textures stay as they are. Like the game: Movie Duels 8, OpenJK, the original Jedi Academy and Jedi Outcast 0 (off). If the graphics card cannot do the number, the next smaller one applies.",
     "Wie r_ext_multisample im Spiel: Die Kanten der Flächen bekommen mehrere Proben je Bildpunkt (MSAA), die Texturen bleiben, wie sie sind. Wie das Spiel: Movie Duels 8, OpenJK, das Original-Jedi-Academy und Jedi Outcast 0 (aus). Kann die Grafikkarte die Zahl nicht, gilt die nächstkleinere.",
     "与游戏中的 r_ext_multisample 相同:表面的边缘每个像素获得多个采样(MSAA),纹理保持不变。与游戏相同:Movie Duels 为 8,OpenJK、原版 Jedi Academy 和 Jedi Outcast 为 0(关闭)。如果显卡不支持该数值,则使用下一个较小的数值。",
     "ゲームの r_ext_multisample と同じです。面の縁がピクセルごとに複数のサンプルを持ち(MSAA)、テクスチャはそのままです。ゲームと同じ:Movie Duels は 8、OpenJK、オリジナルの Jedi Academy と Jedi Outcast は 0(オフ)。グラフィックカードがその数に対応していない場合は、次に小さい数になります。",
     "Como r_ext_multisample en el juego: los bordes de las superficies reciben varias muestras por píxel (MSAA), las texturas quedan igual. Como el juego: Movie Duels 8; OpenJK, el Jedi Academy original y Jedi Outcast 0 (desactivado). Si la tarjeta gráfica no admite el número, se usa el siguiente menor.",
     "Jako r_ext_multisample ve hře: hrany ploch dostanou několik vzorků na pixel (MSAA), textury zůstanou, jak jsou. Jako hra: Movie Duels 8, OpenJK, původní Jedi Academy a Jedi Outcast 0 (vypnuto). Pokud grafická karta číslo nezvládne, platí nejbližší menší."),
    ("MapMsaaGame", "Like the game (%d)", "Wie das Spiel (%d)", "与游戏相同 (%d)", "ゲームと同じ (%d)", "Como el juego (%d)", "Jako hra (%d)"),
    ("MapMsaaOff", "Off", "Aus", "关闭", "オフ", "Desactivado", "Vypnuto"),
]


def zeilen_vier():
    """(Kennung, English, Deutsch, 中文, 日本語) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
