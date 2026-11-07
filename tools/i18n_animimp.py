# -*- coding: utf-8 -*-
"""Oberflaechentexte des Animations-Imports (FBX/glTF/BVH/COLLADA) und der freien Preset-Bibliothek (Quaternius
Universal Animation Library, 100STYLE) fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("AiImport", "Import animation (FBX, glTF, BVH, DAE)...", "Animation importieren (FBX, glTF, BVH, DAE)...", "导入动画 (FBX、glTF、BVH、DAE)...",
     "アニメーションを読み込む (FBX、glTF、BVH、DAE)...", "Importar animación (FBX, glTF, BVH, DAE)...", "Importovat animaci (FBX, glTF, BVH, DAE)..."),
    ("AiImportHint",
     "Reads skeleton and keys from a file of another program (e.g. a Mixamo FBX you downloaded yourself, glTF, BVH motion capture) as the source "
     "for retargeting. Known skeletons (Mixamo, Biped, CAT ...) are mapped at once, the preview plays on the figure. Units, axes and frame rate "
     "come from the file.",
     "Liest Skelett und Schlüssel aus der Datei eines anderen Programms (z. B. ein selbst geladenes Mixamo-FBX, glTF, BVH-Motion-Capture) als "
     "Quelle für das Retargeting. Bekannte Skelette (Mixamo, Biped, CAT ...) werden sofort zugeordnet, die Vorschau spielt auf der Figur. "
     "Einheiten, Achsen und Bildrate kommen aus der Datei.",
     "从其他程序的文件 (例如你自己下载的 Mixamo FBX、glTF、BVH 动作捕捉) 读取骨架和关键帧, 作为重定向的来源。已知骨架 (Mixamo、Biped、CAT ...) "
     "会立即映射, 预览在角色上播放。单位、轴向和帧率取自文件。",
     "他のプログラムのファイル (自分でダウンロードした Mixamo の FBX、glTF、BVH モーションキャプチャなど) から骨格とキーを読み込み、リターゲットの"
     "ソースにします。既知の骨格 (Mixamo、Biped、CAT ...) はすぐに対応付けられ、プレビューがフィギュア上で再生されます。単位・軸・フレームレートは"
     "ファイルから取ります。",
     "Lee el esqueleto y las claves de un archivo de otro programa (p. ej. un FBX de Mixamo que hayas descargado tú, glTF, captura de movimiento "
     "BVH) como origen del retargeting. Los esqueletos conocidos (Mixamo, Biped, CAT ...) se asignan al instante y la vista previa se reproduce "
     "en la figura. Unidades, ejes y velocidad de fotogramas salen del archivo.",
     "Načte kostru a klíče ze souboru jiného programu (např. sami stažený Mixamo FBX, glTF, BVH motion capture) jako zdroj pro retargeting. "
     "Známé kostry (Mixamo, Biped, CAT ...) se přiřadí hned a náhled se přehraje na postavě. Jednotky, osy a snímková frekvence jsou ze souboru."),
    ("AiImportInfo", "%s, up %s, front %s, %.2f cm per unit, %d fps", "%s, oben %s, vorn %s, %.2f cm je Einheit, %d fps",
     "%s, 上 %s, 前 %s, 每单位 %.2f cm, %d fps", "%s、上 %s、前 %s、1 単位 %.2f cm、%d fps",
     "%s, arriba %s, delante %s, %.2f cm por unidad, %d fps", "%s, nahoru %s, vpřed %s, %.2f cm na jednotku, %d fps"),
    ("AiImportGeladen", "Imported %s: %d bones, %d animations, %s (%s)", "%s importiert: %d Knochen, %d Animationen, %s (%s)",
     "已导入 %s: %d 根骨骼, %d 个动画, %s (%s)", "%s を読み込みました: ボーン %d、アニメーション %d、%s (%s)",
     "%s importado: %d huesos, %d animaciones, %s (%s)", "%s importováno: %d kostí, %d animací, %s (%s)"),
    ("AiBibliothek", "Library", "Bibliothek", "动画库", "ライブラリ", "Biblioteca", "Knihovna"),
    ("AiBibliothekHint",
     "Free animations from other authors, transferred to the figure with the retargeting: pick a category and an animation, put it on the "
     "figure (live preview), choose the slot and bake. Source and licence are shown for every entry.",
     "Freie Animationen anderer Urheber, mit dem Retargeting auf die Figur übertragen: Kategorie und Animation wählen, auf die Figur legen "
     "(Vorschau live), Platz wählen und backen. Quelle und Lizenz stehen bei jedem Eintrag.",
     "来自其他作者的免费动画, 通过重定向传到角色上: 选择类别和动画, 放到角色上 (实时预览), 选择槽位并烘焙。每个条目都显示来源和许可。",
     "他の作者による無料アニメーションをリターゲットでフィギュアに移します。カテゴリとアニメーションを選び、フィギュアに載せ (ライブプレビュー)、"
     "スロットを選んでベイクします。各項目に出典とライセンスを表示します。",
     "Animaciones libres de otros autores, pasadas a la figura con el retargeting: elige categoría y animación, ponla en la figura (vista "
     "previa en vivo), elige el hueco y hornea. Cada entrada muestra origen y licencia.",
     "Volné animace jiných autorů přenesené na postavu retargetingem: vyberte kategorii a animaci, položte ji na postavu (živý náhled), "
     "zvolte pozici a zapečte. U každé položky je uveden zdroj a licence."),
    ("AiKategorie", "Category", "Kategorie", "类别", "カテゴリ", "Categoría", "Kategorie"),
    ("AiAuflegen", "Put on figure", "Auf Figur legen", "放到角色上", "フィギュアに載せる", "Poner en la figura", "Položit na postavu"),
    ("AiAuflegenHint",
     "Transfers the selected library animation to the figure and plays it (nothing is changed yet; bake to a slot to keep it).",
     "Überträgt die gewählte Animation der Bibliothek auf die Figur und spielt sie ab (noch ändert sich nichts; zum Behalten auf einen Platz backen).",
     "把所选的库动画传到角色上并播放 (尚未更改任何内容; 要保留请烘焙到槽位)。",
     "選んだライブラリのアニメーションをフィギュアに移して再生します (まだ何も変わりません。残すにはスロットへベイクします)。",
     "Pasa la animación elegida de la biblioteca a la figura y la reproduce (aún no cambia nada; para conservarla, hornéala en un hueco).",
     "Přenese vybranou animaci z knihovny na postavu a přehraje ji (zatím se nic nemění; pro uložení ji zapečte do pozice)."),
    ("AiQuelle", "Source: %s - licence %s", "Quelle: %s - Lizenz %s", "来源: %s - 许可 %s", "出典: %s - ライセンス %s", "Origen: %s - licencia %s",
     "Zdroj: %s - licence %s"),
    ("AiKeineBibliothek", "Library not found (data/presets/presets.txt).", "Bibliothek nicht gefunden (data/presets/presets.txt).",
     "未找到动画库 (data/presets/presets.txt)。", "ライブラリが見つかりません (data/presets/presets.txt)。",
     "No se encontró la biblioteca (data/presets/presets.txt).", "Knihovna nenalezena (data/presets/presets.txt)."),
    ("AiBibliothekOeffnen", "Open preset library", "Preset-Bibliothek öffnen", "打开预设库", "プリセットライブラリを開く",
     "Abrir biblioteca de presets", "Otevřít knihovnu předvoleb"),
    ("AiNahtlos", "Cycle: %d frames at %d fps, seam %.2f x the largest step", "Zyklus: %d Bilder bei %d fps, Naht %.2f x der größte Schritt",
     "循环: %d 帧, %d fps, 接缝为最大步长的 %.2f 倍", "サイクル: %d フレーム (%d fps)、継ぎ目は最大ステップの %.2f 倍",
     "Ciclo: %d fotogramas a %d fps, costura %.2f x el paso mayor", "Cyklus: %d snímků při %d fps, šev %.2f x největší krok"),
    ("AiKatStehen", "Idle", "Stehen", "站立", "待機", "Reposo", "Stání"),
    ("AiKatKampf", "Combat", "Kampf", "战斗", "戦闘", "Combate", "Boj"),
    ("AiKatSonst", "Other", "Sonstiges", "其他", "その他", "Otros", "Ostatní"),
    ("AboutAnimLizenz",
     "Preset library (Model tab > Animation > Presets): Quaternius - Universal Animation Library (CC0 1.0) and \"The 100STYLE Dataset - "
     "Ian Mason\" (CC BY 4.0), on their own skeletons; see data/presets/CREDITS.txt.",
     "Preset-Bibliothek (Model-Tab > Animation > Presets): Quaternius - Universal Animation Library (CC0 1.0) und \"The 100STYLE Dataset - "
     "Ian Mason\" (CC BY 4.0), auf ihren eigenen Skeletten; siehe data/presets/CREDITS.txt.",
     "预设库 (Model 选项卡 > 动画 > 预设): Quaternius - Universal Animation Library (CC0 1.0) 和 \"The 100STYLE Dataset - Ian Mason\" "
     "(CC BY 4.0), 使用各自的骨架; 见 data/presets/CREDITS.txt。",
     "プリセットライブラリ (Model タブ > アニメーション > プリセット): Quaternius - Universal Animation Library (CC0 1.0) と \"The 100STYLE "
     "Dataset - Ian Mason\" (CC BY 4.0)、それぞれ元の骨格のまま。data/presets/CREDITS.txt を参照。",
     "Biblioteca de presets (pestaña Model > Animación > Presets): Quaternius - Universal Animation Library (CC0 1.0) y \"The 100STYLE "
     "Dataset - Ian Mason\" (CC BY 4.0), en sus propios esqueletos; ver data/presets/CREDITS.txt.",
     "Knihovna předvoleb (karta Model > Animace > Předvolby): Quaternius - Universal Animation Library (CC0 1.0) a \"The 100STYLE Dataset - "
     "Ian Mason\" (CC BY 4.0), na vlastních kostrách; viz data/presets/CREDITS.txt."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
