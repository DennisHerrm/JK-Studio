"""p_karte (07.11.): Karten-Editor-Reste. Ergaenzt den Abschnitt "Geometrie zu Brushes" der Seite karte_werkzeuge9 nach dem
Absatz "formate" von inhalt_aseb.py: Teile mit Shadern ausserhalb von textures/ werden unter Auto Modell + Clip (Brush-Seiten
tragen im .map nur Shader unter textures/, q3map2 map.cpp ParseRawBrush :1009).

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_aseb.py: die ersten vier gehen in T(...),
Spanisch und Tschechisch nach inhalt_es.py / inhalt_cs.py (ES, CS).
"""

X = {
    "textures": (
        "**Brushes only carry shaders under `textures/`:** q3map2 puts `textures/` in front of every brush shader of a .map. A part whose "
        "shaders lie elsewhere - typically `models/...` when the bitmap of an .ase sits next to the model without a folder - therefore "
        "becomes **Model + clip** under Auto: the model surface keeps the shader exactly as the misc_model would bake it, the clip brushes "
        "make it solid. A built test map with an .ase from 3ds Max shows the same shaders as the model itself.",
        "**Brushes tragen nur Shader unter `textures/`:** q3map2 setzt `textures/` vor jeden Brush-Shader einer .map. Ein Teil, dessen "
        "Shader woanders liegen - meist `models/...`, wenn das Bild einer .ase ohne Ordner neben dem Modell liegt -, wird darum unter Auto "
        "**Modell + Clip**: Die Modellfläche behält den Shader genau so, wie das misc_model ihn einbacken würde, die Clip-Brushes machen es "
        "fest. Eine gebaute Testkarte mit einer .ase aus 3ds Max zeigt dieselben Shader wie das Modell selbst.",
        "**画刷只能使用 `textures/` 下的着色器:** q3map2 会在 .map 中每个画刷着色器前加上 `textures/`。着色器位于别处的部件——通常是 "
        "`models/...`,即 .ase 的位图不带文件夹、放在模型旁边时——因此在“自动”下会成为**模型 + Clip**:模型表面保留与 misc_model 烘焙时完全相同的"
        "着色器,Clip 画刷使其成为实体。用 3ds Max 导出的 .ase 编译的测试地图显示的着色器与模型本身相同。",
        "**ブラシが持てるのは `textures/` 以下のシェーダーだけです:** q3map2 は .map のすべてのブラシシェーダーの前に `textures/` を付けます。"
        "シェーダーが別の場所にある部品(多くは `models/...`。.ase のビットマップがフォルダーなしでモデルの隣にある場合)は、自動では"
        "**モデル + Clip** になります: モデルの面は misc_model が焼き込むのとまったく同じシェーダーを保ち、Clip ブラシで固くなります。"
        "3ds Max の .ase でビルドしたテストマップは、モデルそのものと同じシェーダーを表示します。",
        "**Los brushes solo llevan shaders bajo `textures/`:** q3map2 antepone `textures/` a cada shader de brush de un .map. Una parte "
        "cuyos shaders están en otro sitio (normalmente `models/...`, cuando el bitmap de un .ase está junto al modelo sin carpeta) pasa por "
        "eso a **Modelo + clip** con Auto: la superficie del modelo conserva el shader tal como lo integraría el misc_model y los brushes "
        "clip la hacen sólida. Un mapa de prueba compilado con un .ase de 3ds Max muestra los mismos shaders que el propio modelo.",
        "**Brushe nesou jen shadery pod `textures/`:** q3map2 dává před každý shader brushe v .map `textures/`. Část, jejíž shadery leží "
        "jinde - obvykle `models/...`, když bitmapa souboru .ase leží vedle modelu bez složky -, se proto v režimu Auto stane "
        "**Model + clip**: plocha modelu si ponechá shader přesně tak, jak by ho zapekl misc_model, a clip brushe ji udělají pevnou. "
        "Sestavená testovací mapa s .ase z 3ds Max ukazuje stejné shadery jako samotný model."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def einhaengen(werkzeuge9):
    """Nach dem Absatz "formate" von inhalt_aseb.py (Abschnitt Geometrie zu Brushes der Seite karte_werkzeuge9)."""
    import inhalt_aseb
    b = werkzeuge9["bloecke"]
    formate = inhalt_aseb.X["formate"][0]
    i = next(n for n, x in enumerate(b) if x[0] == "p" and getattr(x[1], "t", {}).get("en") == formate)
    b.insert(i + 1, ("p", _t("textures")))
