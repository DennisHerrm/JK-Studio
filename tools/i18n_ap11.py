# -*- coding: utf-8 -*-
"""Oberflaechentexte von AP11 (MBENTWURF Teil B, 13.11.): mehrere Modelle in Reitern ueber der Ansicht, Kopieren und Einfuegen von
Flaechen zwischen den Modellen (Abgleich R5, X19, P1) - fuer tools/gen_i18n.py und die weiteren Sprachen.

Wortlaut nach dem freigegebenen Entwurf (plaene/modellblock_stufe0/entwurf_modelltab.html): "In neuem Tab oeffnen", "Importieren ...",
"Kopieren (Strg+C)", "Einfuegen als Kind hiervon (Strg+V)", "Ablage: <Flaeche> aus <Modell> · Strg+V fuegt ein".

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    # --- Reiter (R5, P1) ---
    ("MxNeuerTab", "Open in new tab ...", "In neuem Tab öffnen ...", "在新标签页中打开...", "新しいタブで開く...", "Abrir en pestaña nueva...",
     "Otevřít na nové kartě..."),
    ("MxNeuerTabHint",
     "Opens another model in its own tab above the view (Ctrl+O). Every tab keeps its own file, animation, selection, undo history, "
     "modifier stack, rig, timeline and views.",
     "Öffnet ein weiteres Modell in einem eigenen Reiter über der Ansicht (Strg+O). Jeder Reiter behält Datei, Animation, Auswahl, "
     "Rückgängig-Verlauf, Modifier-Stapel, Rig, Zeitleiste und Ansichten.",
     "在视图上方的独立标签页中打开另一个模型 (Ctrl+O)。每个标签页都保留自己的文件、动画、选择、撤销历史、修改器堆栈、绑定、时间轴和视图。",
     "別のモデルをビュー上部の専用タブで開きます (Ctrl+O)。各タブはファイル、アニメーション、選択、元に戻す履歴、モディファイアスタック、リグ、"
     "タイムライン、ビューをそれぞれ保持します。",
     "Abre otro modelo en su propia pestaña sobre la vista (Ctrl+O). Cada pestaña conserva su archivo, animación, selección, historial "
     "de deshacer, pila de modificadores, rig, línea de tiempo y vistas.",
     "Otevře další model na vlastní kartě nad pohledem (Ctrl+O). Každá karta si ponechá soubor, animaci, výběr, historii zpět, zásobník "
     "modifikátorů, rig, časovou osu a pohledy."),
    ("MxNeuerTabDatei", "Model file (.glm, .md3) ...", "Modelldatei (.glm, .md3) ...", "模型文件 (.glm, .md3)...", "モデル ファイル (.glm, .md3)...",
     "Archivo de modelo (.glm, .md3)...", "Soubor modelu (.glm, .md3)..."),
    ("MxNeuerTabPk3", "From the game files ...", "Aus den Spieldateien ...", "从游戏文件...", "ゲーム ファイルから...", "De los archivos del juego...",
     "Ze souborů hry..."),
    ("MxPlusHint", "Open a model in a new tab (Ctrl+O)", "Ein Modell in einem neuen Reiter öffnen (Strg+O)", "在新标签页中打开模型 (Ctrl+O)",
     "モデルを新しいタブで開く (Ctrl+O)", "Abrir un modelo en una pestaña nueva (Ctrl+O)", "Otevřít model na nové kartě (Ctrl+O)"),
    ("MxSchliessen", "Close tab", "Reiter schließen", "关闭标签页", "タブを閉じる", "Cerrar pestaña", "Zavřít kartu"),
    ("MxSchliessenHint", "Closes this model. With unsaved changes you are asked first.",
     "Schließt dieses Modell. Bei ungespeicherten Änderungen kommt zuerst die Frage.", "关闭此模型。有未保存的更改时会先询问。",
     "このモデルを閉じます。未保存の変更がある場合は先に確認します。", "Cierra este modelo. Si hay cambios sin guardar, se pregunta antes.",
     "Zavře tento model. Při neuložených změnách se nejdřív zeptá."),
    ("MxAndereSchliessen", "Close other tabs", "Andere Reiter schließen", "关闭其他标签页", "他のタブを閉じる", "Cerrar las demás pestañas",
     "Zavřít ostatní karty"),
    ("MxKeinModell", "(no model)", "(kein Modell)", "(无模型)", "(モデルなし)", "(sin modelo)", "(žádný model)"),
    ("MxUngespeichert", "Unsaved changes", "Ungespeicherte Änderungen", "未保存的更改", "未保存の変更", "Cambios sin guardar", "Neuložené změny"),
    ("MxSchliessenFrage",
     "The model %s has unsaved changes.\nClose the tab and discard them?\n(No: keep the tab)",
     "Das Modell %s hat ungespeicherte Änderungen.\nReiter schließen und sie verwerfen?\n(Nein: der Reiter bleibt)",
     "模型 %s 有未保存的更改。\n关闭标签页并放弃更改?\n(否: 保留标签页)",
     "モデル %s に未保存の変更があります。\nタブを閉じて変更を破棄しますか?\n(いいえ: タブを残す)",
     "El modelo %s tiene cambios sin guardar.\n¿Cerrar la pestaña y descartarlos?\n(No: la pestaña se queda)",
     "Model %s má neuložené změny.\nZavřít kartu a zahodit je?\n(Ne: karta zůstane)"),
    ("MxSchonOffen", "%s is already open in a tab - switched to it.", "%s ist schon in einem Reiter offen - dorthin gewechselt.",
     "%s 已在一个标签页中打开 - 已切换到该标签页。", "%s はすでにタブで開いています - そのタブに切り替えました。",
     "%s ya está abierto en una pestaña; se ha cambiado a ella.", "%s je už otevřen na kartě - přepnuto na ni."),

    # --- Ablage (R5, X19) ---
    ("MxAblageHinweis", "Clipboard: %s from %s · Ctrl+V pastes", "Ablage: %s aus %s · Strg+V fügt ein", "剪贴板: %s (来自 %s) · Ctrl+V 粘贴",
     "クリップボード: %s (%s から) · Ctrl+V で貼り付け", "Portapapeles: %s de %s · Ctrl+V pega", "Schránka: %s z %s · Ctrl+V vloží"),
    ("MxAblageMehr", "%s +%d", "%s +%d", "%s +%d", "%s +%d", "%s +%d", "%s +%d"),
    ("MxKopieren", "Copy", "Kopieren", "复制", "コピー", "Copiar", "Kopírovat"),
    ("MxKopierenHint",
     "Copies the selected surfaces with all LODs, weights, shader and modifier stack - to paste into this or another open model.",
     "Kopiert die gewählten Flächen mit allen Detailstufen, Gewichten, Shader und Modifier-Stapel - zum Einfügen in dieses oder ein "
     "anderes offenes Modell.",
     "复制所选面及其所有 LOD、权重、着色器和修改器堆栈 - 可粘贴到此模型或其他打开的模型中。",
     "選択したサーフェスをすべての LOD、ウェイト、シェーダー、モディファイアスタックごとコピーします - このモデルまたは開いている別の"
     "モデルに貼り付けられます。",
     "Copia las superficies seleccionadas con todos los LOD, pesos, sombreador y pila de modificadores, para pegarlas en este u otro "
     "modelo abierto.",
     "Zkopíruje vybrané plochy se všemi LOD, vahami, shaderem a zásobníkem modifikátorů - k vložení do tohoto nebo jiného otevřeného modelu."),
    ("MxEinfuegen", "Paste surfaces", "Flächen einfügen", "粘贴面", "サーフェスを貼り付け", "Pegar superficies", "Vložit plochy"),
    ("MxEinfuegenKind", "Paste as child of this", "Einfügen als Kind hiervon", "粘贴为此项的子项", "これの子として貼り付け", "Pegar como hijo de esta",
     "Vložit jako potomka této"),
    ("MxEinfuegenKindHint",
     "Pastes the surfaces from the clipboard as children of this surface (one undo step). Weights go to the bones of the same name; "
     "where this model has no such bone, to the nearest parent bone it has - the message names each one. LODs, shader and modifier "
     "stack come along; the limits of the game are checked.",
     "Fügt die Flächen der Ablage als Kinder dieser Fläche ein (ein Rückgängig-Schritt). Gewichte gehen an die gleichnamigen Knochen; "
     "fehlt einer in diesem Modell, an den nächsten vorhandenen Elternknochen - die Meldung nennt jeden. Detailstufen, Shader und "
     "Modifier-Stapel kommen mit; die Grenzen des Spiels werden geprüft.",
     "将剪贴板中的面粘贴为此面的子项 (一个撤销步骤)。权重转到同名骨骼; 此模型缺少的骨骼, 转到其最近的已有父骨骼 - 消息会逐一列出。"
     "LOD、着色器和修改器堆栈一并带入; 会检查游戏的限制。",
     "クリップボードのサーフェスをこのサーフェスの子として貼り付けます (元に戻す 1 ステップ)。ウェイトは同名のボーンへ、このモデルに"
     "ないボーンは最も近い既存の親ボーンへ移り、メッセージにすべて表示されます。LOD、シェーダー、モディファイアスタックも一緒に移り、"
     "ゲームの制限を確認します。",
     "Pega las superficies del portapapeles como hijas de esta superficie (un paso de deshacer). Los pesos van a los huesos del mismo "
     "nombre; si este modelo no tiene alguno, al hueso padre más cercano que sí tenga; el mensaje los nombra todos. Los LOD, el sombreador "
     "y la pila de modificadores vienen incluidos; se comprueban los límites del juego.",
     "Vloží plochy ze schránky jako potomky této plochy (jeden krok zpět). Váhy přejdou na stejnojmenné kosti; kde tento model kost nemá, "
     "na nejbližší existující rodičovskou kost - zpráva každou uvede. LOD, shader a zásobník modifikátorů jdou s nimi; kontrolují se limity hry."),
    ("MxKopiert", "Copied: %s from %s.", "Kopiert: %s aus %s.", "已复制: %s (来自 %s)。", "コピーしました: %s (%s から)。",
     "Copiado: %s de %s.", "Zkopírováno: %s z %s."),
    ("MxNichtsGewaehlt", "Select a surface first (Scene Explorer or view).", "Zuerst eine Fläche wählen (Szenen-Explorer oder Ansicht).",
     "请先选择一个面 (场景资源管理器或视图)。", "先にサーフェスを選択してください (シーン エクスプローラまたはビュー)。",
     "Selecciona primero una superficie (Explorador de escena o vista).", "Nejdřív vyberte plochu (Průzkumník scény nebo pohled)."),
    ("MxQuelleOhneGla",
     "The skeleton (GLA) of this model is not loaded - its weights cannot be assigned by bone name. Load the GLA first.",
     "Das Skelett (GLA) dieses Modells ist nicht geladen - seine Gewichte lassen sich nicht nach Knochennamen zuordnen. Zuerst die GLA laden.",
     "此模型的骨架 (GLA) 未加载 - 无法按骨骼名称分配其权重。请先加载 GLA。",
     "このモデルのスケルトン (GLA) が読み込まれていません - ウェイトをボーン名で割り当てられません。先に GLA を読み込んでください。",
     "El esqueleto (GLA) de este modelo no está cargado; sus pesos no se pueden asignar por nombre de hueso. Carga primero el GLA.",
     "Kostra (GLA) tohoto modelu není načtena - jeho váhy nelze přiřadit podle názvů kostí. Nejdřív načtěte GLA."),
    ("MxAblageLeer", "The clipboard is empty - copy a surface first (Ctrl+C).", "Die Ablage ist leer - zuerst eine Fläche kopieren (Strg+C).",
     "剪贴板为空 - 请先复制一个面 (Ctrl+C)。", "クリップボードは空です - 先にサーフェスをコピーしてください (Ctrl+C)。",
     "El portapapeles está vacío; copia primero una superficie (Ctrl+C).", "Schránka je prázdná - nejdřív zkopírujte plochu (Ctrl+C)."),
    ("MxZielLeer", "Open a model first.", "Zuerst ein Modell öffnen.", "请先打开一个模型。", "先にモデルを開いてください。", "Abre primero un modelo.",
     "Nejdřív otevřete model."),
    ("MxZielOhneGla",
     "This model has no skeleton loaded (GLA) - the weights cannot be assigned by bone name. Load the GLA first.",
     "Dieses Modell hat kein Skelett geladen (GLA) - die Gewichte lassen sich nicht nach Knochennamen zuordnen. Zuerst die GLA laden.",
     "此模型未加载骨架 (GLA) - 无法按骨骼名称分配权重。请先加载 GLA。",
     "このモデルにはスケルトン (GLA) が読み込まれていません - ウェイトをボーン名で割り当てられません。先に GLA を読み込んでください。",
     "Este modelo no tiene esqueleto cargado (GLA); los pesos no se pueden asignar por nombre de hueso. Carga primero el GLA.",
     "Tento model nemá načtenou kostru (GLA) - váhy nelze přiřadit podle názvů kostí. Nejdřív načtěte GLA."),
    ("MxEingefuegt", "Pasted: %s from %s as child of %s.", "Eingefügt: %s aus %s als Kind von %s.", "已粘贴: %s (来自 %s), 作为 %s 的子项。",
     "貼り付けました: %s (%s から) を %s の子として。", "Pegado: %s de %s como hijo de %s.", "Vloženo: %s z %s jako potomek %s."),
    ("MxEingefuegtOben", "Pasted: %s from %s at the top level.", "Eingefügt: %s aus %s auf oberster Ebene.", "已粘贴: %s (来自 %s), 位于最顶层。",
     "貼り付けました: %s (%s から) を最上位に。", "Pegado: %s de %s en el nivel superior.", "Vloženo: %s z %s na nejvyšší úroveň."),
    ("MxUmbenannt", "Renamed (the name is already in this model): %s.", "Umbenannt (der Name ist schon im Modell): %s.",
     "已重命名 (此模型中已有该名称): %s。", "名前を変更しました (このモデルにすでにある名前): %s。",
     "Renombrado (el nombre ya está en este modelo): %s.", "Přejmenováno (název už v tomto modelu je): %s."),
    ("MxErsatz", "Bones missing here - their weights went to: %s.", "Knochen fehlen hier - ihre Gewichte gingen an: %s.",
     "此处缺少骨骼 - 其权重已转到: %s。", "ここにないボーン - そのウェイトの移動先: %s。",
     "Faltan huesos aquí; sus pesos pasaron a: %s.", "Zde chybí kosti - jejich váhy přešly na: %s."),
    ("MxStufenWeg", "%d LODs of the source are left out (this model has %d).", "%d Detailstufen der Quelle fallen weg (dieses Modell hat %d).",
     "源模型的 %d 个 LOD 被省略 (此模型有 %d 个)。", "コピー元の LOD %d 個は省きました (このモデルは %d 個)。",
     "Se omiten %d LOD del origen (este modelo tiene %d).", "%d LOD zdroje vypadne (tento model jich má %d)."),
    ("MxStufenAuf", "%d LODs filled with the coarsest LOD of the source.", "%d Detailstufen mit der gröbsten der Quelle gefüllt.",
     "%d 个 LOD 已用源模型最粗的 LOD 填充。", "LOD %d 個をコピー元の最も粗い LOD で埋めました。",
     "%d LOD rellenados con el LOD más simple del origen.", "%d LOD vyplněno nejhrubším LOD zdroje."),
    ("MxStarrZiel", "This model has no skeleton (.md3): the surfaces are rigid.", "Dieses Modell hat kein Skelett (.md3): die Flächen sind starr.",
     "此模型没有骨架 (.md3): 这些面是刚性的。", "このモデルにはスケルトンがありません (.md3): サーフェスは固定です。",
     "Este modelo no tiene esqueleto (.md3): las superficies son rígidas.", "Tento model nemá kostru (.md3): plochy jsou pevné."),
    ("MxStarrQuelle", "The source has no skeleton: the surfaces hang entirely on bone %s.", "Die Quelle hat kein Skelett: die Flächen hängen ganz an Knochen %s.",
     "源模型没有骨架: 这些面完全绑定在骨骼 %s 上。", "コピー元にはスケルトンがありません: サーフェスはすべてボーン %s に付きます。",
     "El origen no tiene esqueleto: las superficies dependen por completo del hueso %s.", "Zdroj nemá kostru: plochy visí celé na kosti %s."),
    ("MxGrenzenOk", "Limits of %s kept.", "Grenzen von %s eingehalten.", "符合 %s 的限制。", "%s の制限内です。", "Se respetan los límites de %s.",
     "Limity %s dodrženy."),
    ("MxGrenzen", "Over the limits of %s: %s", "Über den Grenzen von %s: %s", "超出 %s 的限制: %s", "%s の制限を超えています: %s", "Supera los límites de %s: %s",
     "Nad limity %s: %s"),
    ("MxUndoEinfuegen", "Paste surfaces", "Flächen einfügen", "粘贴面", "サーフェスの貼り付け", "Pegar superficies", "Vložit plochy"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
