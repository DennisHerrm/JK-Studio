"""REST8: doppelte Shadernamen - das Echtzeit-Licht nimmt den Eintrag von bhed_q3map2, das Spiel einen anderen.

Haengt eine Notiz an die Seite karte_tempo (inhalt_karte7.py) vor den Abschnitt "Gebaute Karte wie im Spiel". Jeder Text einmal in
allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_r2a.py.
"""

X = {
    "vorrang": (
        "**The same shader name in several shader files:** the real-time light shows what the build makes of the map, so it takes the "
        "definition bhed_q3map2 takes: loose files before every .pk3, the .pk3 of base before those of the mod, the first definition wins. "
        "The game decides otherwise - it joins the files in reverse search order, and the first entry of that text wins, so the lower-ranked "
        ".pk3 wins. **{{KeSpielbildMenue}}** and the Mission tab preview draw the game's definition. Where a shader of the map is resolved "
        "differently by the build and the game, the check in [[karte_optimierer]] warns (**{{KeOptPrShaderVorrang}}**).",
        "**Derselbe Shadername in mehreren Shader-Dateien:** das Echtzeit-Licht zeigt, was der Bau aus der Karte macht, und nimmt deshalb "
        "die Definition, die bhed_q3map2 nimmt: lose Dateien vor jeder .pk3, die .pk3 von base vor denen des Mods, die erste Definition "
        "gilt. Das Spiel entscheidet anders - es hängt die Dateien in umgekehrter Suchfolge aneinander, und der erste Eintrag dieses Textes "
        "gilt, also gewinnt die .pk3 mit dem niedrigeren Rang. **{{KeSpielbildMenue}}** und die Vorschau im Mission-Tab zeichnen die "
        "Definition des Spiels. Löst der Bau einen Shader der Karte anders auf als das Spiel, warnt die Prüfung in [[karte_optimierer]] "
        "(**{{KeOptPrShaderVorrang}}**).",
        "**多个着色器文件中的同名着色器:**实时光照显示构建对地图的处理结果,因此采用 bhed_q3map2 所用的定义:松散文件先于任何 .pk3,base 的 .pk3 "
        "先于模组的,第一个定义生效。游戏的规则不同——它按相反的搜索顺序拼接文件,该文本中的第一个条目生效,因此优先级较低的 .pk3 获胜。"
        "**{{KeSpielbildMenue}}**和任务选项卡中的预览绘制游戏的定义。如果构建与游戏对地图中某个着色器的解析不同,[[karte_optimierer]] 中的检查会发出警告"
        "(**{{KeOptPrShaderVorrang}}**)。",
        "**複数のシェーダーファイルにある同じシェーダー名:**リアルタイムライトはビルドがマップから作るものを表示するため、bhed_q3map2 と同じ定義を"
        "使います: ばらのファイルがどの .pk3 よりも先、base の .pk3 が MOD のものより先で、最初の定義が有効です。ゲームの決め方は異なり、ファイルを"
        "検索順の逆につなげ、そのテキストの最初のエントリが有効なので、順位の低い .pk3 が勝ちます。**{{KeSpielbildMenue}}** とミッションタブの"
        "プレビューはゲームの定義を描きます。マップのシェーダーをビルドとゲームが異なって解決する場合、[[karte_optimierer]] のチェックが警告します"
        "(**{{KeOptPrShaderVorrang}}**)。",
        "**El mismo nombre de shader en varios archivos de shaders:** la luz en tiempo real muestra lo que la compilación hace del mapa, así "
        "que toma la definición que toma bhed_q3map2: los archivos sueltos antes que cualquier .pk3, los .pk3 de base antes que los del mod, "
        "y vale la primera definición. El juego decide de otra forma: une los archivos en el orden de búsqueda inverso y vale la primera "
        "entrada de ese texto, así que gana el .pk3 de menor rango. **{{KeSpielbildMenue}}** y la vista previa de la pestaña Mission dibujan "
        "la definición del juego. Si la compilación y el juego resuelven distinto un shader del mapa, la comprobación de [[karte_optimierer]] "
        "avisa (**{{KeOptPrShaderVorrang}}**).",
        "**Stejný název shaderu ve více souborech shaderů:** světlo v reálném čase ukazuje, co sestavení z mapy udělá, a proto bere definici, "
        "kterou bere bhed_q3map2: volné soubory před každým .pk3, .pk3 z base před těmi z modu, platí první definice. Hra rozhoduje jinak - "
        "spojí soubory v obráceném pořadí hledání a platí první záznam tohoto textu, takže vyhraje .pk3 s nižší prioritou. "
        "**{{KeSpielbildMenue}}** a náhled na kartě Mission kreslí definici hry. Pokud sestavení a hra řeší shader mapy různě, kontrola v "
        "[[karte_optimierer]] varuje (**{{KeOptPrShaderVorrang}}**)."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def einhaengen(tempo):
    """Die Notiz vor den Abschnitt "Gebaute Karte wie im Spiel" (karte_tempo)."""
    b = tempo["bloecke"]
    i = next(n for n, x in enumerate(b) if x[0] == "h2" and x[1] == "gebautwiespiel")
    b.insert(i, ("note", _t("vorrang")))
