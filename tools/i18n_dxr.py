# -*- coding: utf-8 -*-
"""Oberflaechentexte der Raytracing-Schatten im Echtzeit-Licht des Karten-Editors (DXR, rc612).

Build > Ray-traced shadows (DXR): die zweite Schattenart neben den Schattenkarten - nur unter Direct3D 12 mit DXR 1.1;
sonst ist der Menuepunkt ausgegraut und der Tooltip nennt den Grund (gui/app_karte_dxr.cpp, gui/karte_dxr.h).
Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("KeStrahlenMenue", "Ray-traced shadows (DXR)", "Raytracing-Schatten (DXR)", "光线追踪阴影(DXR)", "レイトレーシングの影(DXR)",
     "Sombras con trazado de rayos (DXR)", "Stíny sledováním paprsků (DXR)"),
    ("KeStrahlenTip",
     "Shadow type of the real-time light. Off (default): shadow maps. On: the graphics card sends one shadow ray per light "
     "and pixel (DirectX Raytracing). The shadows lie at the same place, but with sharp edges and without the steps of the "
     "shadow maps, and no light waits for its shadow map. Shaders with alphashadow and lightfilter tint the light as in "
     "q3map2. Needs Direct3D 12 and a graphics card with DXR 1.1.",
     "Schattenart des Echtzeit-Lichts. Aus (Vorgabe): Schattenkarten. An: Die Grafikkarte schickt je Licht und Bildpunkt "
     "einen Schattenstrahl (DirectX Raytracing). Die Schatten liegen an derselben Stelle, aber mit scharfen Kanten und ohne "
     "die Stufen der Schattenkarten, und kein Licht wartet auf seine Schattenkarte. Shader mit alphashadow und lightfilter "
     "färben das Licht wie in q3map2. Braucht Direct3D 12 und eine Grafikkarte mit DXR 1.1.",
     "实时光照的阴影类型。关闭(默认):阴影贴图。开启:显卡为每个光源和每个像素发出一条阴影光线(DirectX Raytracing)。"
     "阴影位置相同,但边缘锐利,没有阴影贴图的锯齿,也没有光源需要等待自己的阴影贴图。带 alphashadow 和 lightfilter "
     "的着色器会像 q3map2 那样给光线着色。需要 Direct3D 12 和支持 DXR 1.1 的显卡。",
     "リアルタイムライトの影の種類です。オフ(既定):シャドウマップ。オン:グラフィックカードがライトとピクセルごとに"
     "シャドウレイを 1 本飛ばします(DirectX Raytracing)。影の位置は同じですが、輪郭がくっきりしてシャドウマップの段差がなく、"
     "シャドウマップの完成を待つライトもありません。alphashadow と lightfilter のシェーダーは q3map2 と同じように光に色を付けます。"
     "Direct3D 12 と DXR 1.1 対応のグラフィックカードが必要です。",
     "Tipo de sombra de la luz en tiempo real. Desactivado (predeterminado): mapas de sombras. Activado: la tarjeta gráfica "
     "lanza un rayo de sombra por luz y píxel (DirectX Raytracing). Las sombras quedan en el mismo sitio, pero con bordes "
     "nítidos y sin los escalones de los mapas de sombras, y ninguna luz espera a su mapa de sombras. Los shaders con "
     "alphashadow y lightfilter tiñen la luz como en q3map2. Requiere Direct3D 12 y una tarjeta gráfica con DXR 1.1.",
     "Typ stínů světla v reálném čase. Vypnuto (výchozí): stínové mapy. Zapnuto: grafická karta vyšle pro každé světlo a "
     "pixel jeden stínový paprsek (DirectX Raytracing). Stíny leží na stejném místě, ale mají ostré hrany bez schodů "
     "stínových map a žádné světlo nečeká na svou stínovou mapu. Shadery s alphashadow a lightfilter barví světlo jako "
     "v q3map2. Vyžaduje Direct3D 12 a grafickou kartu s DXR 1.1."),
    ("KeStrahlenGrundD3d12",
     "Not available now: ray-traced shadows need Direct3D 12, and Direct3D 11 is running. Choose Direct3D 12 in the main "
     "menu under View → Graphics API (Events tab) and restart.",
     "Jetzt nicht verfügbar: Raytracing-Schatten brauchen Direct3D 12, es läuft aber Direct3D 11. Wähle im Hauptmenü unter "
     "Ansicht → Grafikschnittstelle (Tab Events) Direct3D 12 und starte neu.",
     "当前不可用:光线追踪阴影需要 Direct3D 12,而现在运行的是 Direct3D 11。请在主菜单的“视图 → 图形接口”(Events 选项卡)中"
     "选择 Direct3D 12,然后重新启动。",
     "現在は使えません:レイトレーシングの影には Direct3D 12 が必要ですが、Direct3D 11 で動作しています。メインメニューの"
     "「表示 → グラフィックス API」(Events タブ)で Direct3D 12 を選び、再起動してください。",
     "No disponible ahora: las sombras con trazado de rayos necesitan Direct3D 12 y se está usando Direct3D 11. Elige "
     "Direct3D 12 en el menú principal, en Ver → Interfaz gráfica (pestaña Events), y reinicia.",
     "Nyní není k dispozici: stíny sledováním paprsků potřebují Direct3D 12, ale běží Direct3D 11. V hlavní nabídce zvolte "
     "v Zobrazení → Grafické rozhraní (karta Events) položku Direct3D 12 a spusťte program znovu."),
    ("KeStrahlenGrundGeraet",
     "Not available: this graphics card or its driver cannot do DirectX Raytracing 1.1 (inline raytracing with Shader "
     "Model 6.5). The real-time light uses shadow maps.",
     "Nicht verfügbar: Diese Grafikkarte oder ihr Treiber kann kein DirectX Raytracing 1.1 (Inline-Raytracing mit Shader "
     "Model 6.5). Das Echtzeit-Licht nimmt Schattenkarten.",
     "不可用:此显卡或其驱动程序不支持 DirectX Raytracing 1.1(使用 Shader Model 6.5 的内联光线追踪)。实时光照使用阴影贴图。",
     "使えません:このグラフィックカードまたはドライバーは DirectX Raytracing 1.1(Shader Model 6.5 のインラインレイトレーシング)に"
     "対応していません。リアルタイムライトはシャドウマップを使います。",
     "No disponible: esta tarjeta gráfica o su controlador no admite DirectX Raytracing 1.1 (trazado de rayos en línea con "
     "Shader Model 6.5). La luz en tiempo real usa mapas de sombras.",
     "Není k dispozici: tato grafická karta nebo její ovladač neumí DirectX Raytracing 1.1 (inline raytracing se Shader "
     "Model 6.5). Světlo v reálném čase používá stínové mapy."),
    ("KeStrahlenGrundShader",
     "Not available: this build of JK Studio contains no raytracing shader (it was built without dxc from the Windows SDK). "
     "The real-time light uses shadow maps.",
     "Nicht verfügbar: Diese Fassung von JK Studio enthält keinen Raytracing-Shader (sie wurde ohne dxc aus dem Windows SDK "
     "gebaut). Das Echtzeit-Licht nimmt Schattenkarten.",
     "不可用:此版本的 JK Studio 不包含光线追踪着色器(构建时没有 Windows SDK 的 dxc)。实时光照使用阴影贴图。",
     "使えません:このビルドの JK Studio にはレイトレーシング用シェーダーが含まれていません(Windows SDK の dxc なしで"
     "ビルドされました)。リアルタイムライトはシャドウマップを使います。",
     "No disponible: esta compilación de JK Studio no contiene el shader de trazado de rayos (se compiló sin dxc del "
     "Windows SDK). La luz en tiempo real usa mapas de sombras.",
     "Není k dispozici: toto sestavení JK Studia neobsahuje shader pro sledování paprsků (bylo sestaveno bez dxc z Windows "
     "SDK). Světlo v reálném čase používá stínové mapy."),
    ("KeStrahlenGrundFehler",
     "Not available: the graphics card did not accept the raytracing shaders or data (details in the log). The real-time "
     "light uses shadow maps.",
     "Nicht verfügbar: Die Grafikkarte hat die Raytracing-Shader oder -Daten nicht angenommen (Einzelheiten im Protokoll). "
     "Das Echtzeit-Licht nimmt Schattenkarten.",
     "不可用:显卡未接受光线追踪着色器或数据(详情见日志)。实时光照使用阴影贴图。",
     "使えません:グラフィックカードがレイトレーシングのシェーダーまたはデータを受け付けませんでした(詳細はログ)。"
     "リアルタイムライトはシャドウマップを使います。",
     "No disponible: la tarjeta gráfica no aceptó los shaders o los datos de trazado de rayos (detalles en el registro). "
     "La luz en tiempo real usa mapas de sombras.",
     "Není k dispozici: grafická karta nepřijala shadery nebo data pro sledování paprsků (podrobnosti v protokolu). Světlo "
     "v reálném čase používá stínové mapy."),
    ("KeStrahlenHinweis", "ray-traced shadows", "Raytracing-Schatten", "光线追踪阴影", "レイトレーシングの影",
     "sombras con trazado de rayos", "stíny sledováním paprsků"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
