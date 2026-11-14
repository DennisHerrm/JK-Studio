"""CW-IMPORT: Teil 2 der Clone-Wars-Kette (plaene/clonewars_kette.md) - das Pruefprofil "Clone-Wars-Kette" (ab Werk aus), die
Testfigur, Massstab und Achsen beim Import, der Massstab je Raum, Treppe zu Clip-Rampe, glatte Clip-Wand und die Ebene Collision.

Haengt an die Seite karte_optimierer (inhalt_karte5.py, nach dem Abschnitt von inhalt_cwpruef.py) einen Absatz zum Pruefprofil.
Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_cwpruef.py.
"""

X = {
    "profil": (
        "**Only with the check profile Clone Wars chain** (off by default; **Build > {{KeCwMenue}}** or **Preferences > Optimizer > "
        "{{KeCwProfil}}**). With **{{KeCwProfilStandard}}** the optimizer reports exactly what it reported before these checks; only the "
        "list of shaders and images stays in the text report, because it only informs. The profile also shows **{{KeOptJson}}** and the "
        "sizes below, and puts the clip brushes of \"Model + clip\" into the layer Collision.",
        "**Nur mit dem Prüfprofil Clone-Wars-Kette** (ab Werk aus; **Build > {{KeCwMenue}}** oder **Einstellungen > Optimizer > "
        "{{KeCwProfil}}**). Mit **{{KeCwProfilStandard}}** meldet der Optimierer genau, was er vor diesen Prüfungen meldete; nur die "
        "Liste der Shader und Bilder bleibt im Textbericht, weil sie nur informiert. Das Profil zeigt außerdem **{{KeOptJson}}** und die "
        "Maße unten und legt die Clip-Pinsel von \"Modell + Clip\" in die Ebene Collision.",
        "**仅在检查配置为 Clone Wars 流程时**(默认关闭;**Build > {{KeCwMenue}}** 或 **首选项 > Optimizer > {{KeCwProfil}}**)。"
        "选择 **{{KeCwProfilStandard}}** 时,优化器报告的内容与加入这些检查之前完全相同;只有文本报告中的着色器和图像列表保留,因为它只用于说明。"
        "该配置还会显示 **{{KeOptJson}}** 和下面的尺寸,并把“模型 + clip”的 clip 笔刷放入图层 Collision。",
        "**チェックプロファイルが Clone Wars チェーンのときだけ**(既定ではオフ。**Build > {{KeCwMenue}}** または **設定 > Optimizer > "
        "{{KeCwProfil}}**)。**{{KeCwProfilStandard}}** では、最適化ツールはこれらのチェックが入る前とまったく同じ内容を報告します。"
        "テキストレポートのシェーダーと画像の一覧だけは情報なので残ります。このプロファイルでは **{{KeOptJson}}** と下の寸法も表示され、"
        "「モデル + クリップ」のクリップブラシはレイヤー Collision に入ります。",
        "**Solo con el perfil de comprobación Cadena Clone Wars** (desactivado por defecto; **Build > {{KeCwMenue}}** o **Preferencias > "
        "Optimizer > {{KeCwProfil}}**). Con **{{KeCwProfilStandard}}** el optimizador informa exactamente lo mismo que antes de estas "
        "comprobaciones; solo la lista de shaders e imágenes queda en el informe de texto, porque solo informa. El perfil muestra además "
        "**{{KeOptJson}}** y las medidas de abajo, y pone los brushes clip de \"Modelo + clip\" en la capa Collision.",
        "**Jen s profilem kontrol Řetězec Clone Wars** (ve výchozím stavu vypnuto; **Build > {{KeCwMenue}}** nebo **Předvolby > "
        "Optimizer > {{KeCwProfil}}**). S **{{KeCwProfilStandard}}** hlásí optimalizátor přesně to, co hlásil před těmito kontrolami; v "
        "textové zprávě zůstane jen seznam shaderů a obrázků, protože jen informuje. Profil navíc zobrazí **{{KeOptJson}}** a rozměry níže "
        "a vloží clip brushe z \"Model + clip\" do vrstvy Collision."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def einhaengen(optimierer):
    """Pruefprofil: ein Absatz gleich nach der Ueberschrift des Abschnitts von CW-PRUEF (karte_optimierer)."""
    import inhalt_cwpruef
    b = optimierer["bloecke"]
    titel = inhalt_cwpruef.X["titel"][0]
    h = next(n for n, x in enumerate(b) if x[0] == "h3" and getattr(x[1], "t", {}).get("en") == titel)
    b.insert(h + 1, ("p", _t("profil")))
