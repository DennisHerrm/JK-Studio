# -*- coding: utf-8 -*-
"""Oberflaechentexte von ENTSCHEID3A (12.11., verbindliche Entscheidungen B7-B13 und C4 aus plaene/entscheidungen_1211.md) im
Model-Tab: eingefrorene Zeile im Szenen-Explorer waehlbar wie 3ds Max, Werkzeugnamen durch Breiterziehen der Leiste wie Blender,
Seitenleiste bei Knochen wie Blender (lokale Werte, Drehmodus), Strg+Umschalt+Z wie 3ds Max, Auge fuer Edit Poly und Skin,
Modifier-Stapel in einer Begleitdatei, Zeile "Unwrap UVW" - fuer tools/gen_i18n.py und die weiteren Sprachen.

Blender-Begriffe nach Blenders eigener Uebersetzung (Blender 5.0 datafiles/locale, gelesen mit Y:\\werkzeuge\\ent1\\mo_lesen.py):
Quaternion (WXYZ), XYZ Euler ..., Axis Angle, Rotation Mode, Location, Rotation, Scale. Fehlt dort ein Eintrag (cs: Quaternion,
Rotation Mode), steht unser Text. Die Namen der Modifier von 3ds Max (Edit Poly, Skin, Unwrap UVW) bleiben in allen Sprachen
englisch wie die Zeilen von AP13S.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    # --- B7: eingefrorene Zeile im Szenen-Explorer (wie 3ds Max) ---
    ("EaFrostListeHint",
     "Frozen: a click in the view goes through it. In the Scene Explorer it can still be selected, like in 3ds Max - for example to unfreeze the selection.",
     "Eingefroren: ein Klick in der Ansicht geht hindurch. Im Szenen-Explorer lässt es sich weiter wählen wie in 3ds Max - etwa um die Auswahl aufzutauen.",
     "已冻结: 在视图中单击会穿过它。在场景资源管理器中仍可选择它, 与 3ds Max 相同 - 例如用来解冻所选内容。",
     "フリーズ中: ビューでのクリックは通り抜けます。シーン エクスプローラでは 3ds Max と同じく選択できます (選択をフリーズ解除するときなど)。",
     "Congelado: un clic en la vista lo atraviesa. En el Explorador de escena se puede seleccionar igualmente, como en 3ds Max; por ejemplo, para descongelar la selección.",
     "Zmrazeno: kliknutí v pohledu jím projde. V Průzkumníku scény jej lze dál vybrat jako ve 3ds Max - třeba pro rozmrazení výběru."),
    ("EaAuswahlAuftauen", "Unfreeze selection", "Auswahl auftauen", "解冻所选内容", "選択をフリーズ解除", "Descongelar selección", "Rozmrazit výběr"),
    ("EaAuswahlAuftauenHint",
     "Unfreezes the surfaces, tags and bones selected in the Scene Explorer (3ds Max: Unfreeze Scene Explorer Selection).",
     "Taut die im Szenen-Explorer gewählten Flächen, Tags und Knochen auf (3ds Max: Unfreeze Scene Explorer Selection).",
     "解冻在场景资源管理器中选择的面、标签和骨骼 (3ds Max: Unfreeze Scene Explorer Selection)。",
     "シーン エクスプローラで選択したサーフェス、タグ、ボーンのフリーズを解除します (3ds Max: Unfreeze Scene Explorer Selection)。",
     "Descongela las superficies, etiquetas y huesos seleccionados en el Explorador de escena (3ds Max: Unfreeze Scene Explorer Selection).",
     "Rozmrazí plochy, tagy a kosti vybrané v Průzkumníku scény (3ds Max: Unfreeze Scene Explorer Selection)."),
    ("EaMeldAufgetaut", "Unfrozen: %d", "Aufgetaut: %d", "已解冻: %d", "フリーズ解除: %d", "Descongelados: %d", "Rozmrazeno: %d"),
    ("EaMeldNichtsGefroren", "Nothing selected is frozen.", "Nichts Gewähltes ist eingefroren.", "所选内容均未冻结。", "選択したものはフリーズしていません。",
     "Nada de lo seleccionado está congelado.", "Nic z vybraného není zmrazeno."),

    # --- B8: Werkzeugleiste breiter ziehen (wie Blender) ---
    ("EaLeisteKanteHint",
     "Drag to make the toolbar wider or narrower. Wider than 120 points it shows the tool names, like Blender. Double-click: back to the narrow toolbar.",
     "Ziehen macht die Werkzeugleiste breiter oder schmaler. Breiter als 120 Punkte zeigt sie die Werkzeugnamen, wie Blender. Doppelklick: zurück zur schmalen Leiste.",
     "拖动可加宽或缩窄工具栏。宽度超过 120 点时显示工具名称, 与 Blender 相同。双击: 恢复窄工具栏。",
     "ドラッグでツールバーの幅を変えます。120 ポイントより広いとツール名を表示します (Blender と同じ)。ダブルクリック: 細いツールバーに戻します。",
     "Arrastra para ensanchar o estrechar la barra de herramientas. Con más de 120 puntos muestra los nombres de las herramientas, como Blender. Doble clic: vuelve a la barra estrecha.",
     "Tažením se panel nástrojů rozšíří nebo zúží. Při šířce nad 120 bodů ukazuje názvy nástrojů jako Blender. Dvojklik: zpět na úzký panel."),

    # --- B9: Seitenleiste bei Knochen (wie Blender) ---
    ("EaDrehModus", "Rotation Mode", "Rotationsmodus", "旋转模式", "回転モード", "Modo de rotación", "Režim rotace"),
    ("EaDrehModusHint",
     "How this bone shows its rotation, like Blender: the fields above show and change it in this mode. The game stores the pose as matrices - the mode only changes the display and the input.",
     "Wie dieser Knochen seine Drehung zeigt, wie Blender: die Felder darüber zeigen und ändern sie in diesem Modus. Das Spiel speichert die Pose als Matrizen - der Modus ändert nur Anzeige und Eingabe.",
     "此骨骼如何显示其旋转, 与 Blender 相同: 上方的字段以此模式显示和修改旋转。游戏以矩阵保存姿态 - 模式只改变显示和输入。",
     "このボーンの回転の表し方です (Blender と同じ)。上のフィールドはこのモードで回転を表示・変更します。ゲームはポーズを行列で保存するため、モードは表示と入力だけを変えます。",
     "Cómo muestra este hueso su rotación, como Blender: los campos de arriba la muestran y la cambian en este modo. El juego guarda la pose como matrices; el modo solo cambia la visualización y la entrada.",
     "Jak tato kost ukazuje svou rotaci, jako Blender: pole nad tím ji v tomto režimu ukazují a mění. Hra ukládá pózu jako matice - režim mění jen zobrazení a zadávání."),
    ("EaModusQuat", "Quaternion (WXYZ)", "Quaternion (WXYZ)", "四元数 (WXYZ)", "クォータニオン(WXYZ)", "Cuaternio (WXYZ)", "Quaternion (WXYZ)"),
    ("EaModusXyz", "XYZ Euler", "XYZ Euler", "XYZ 欧拉", "XYZ オイラー角", "Euler XYZ", "XYZ Euler"),
    ("EaModusXzy", "XZY Euler", "XZY Euler", "XZY 欧拉", "XZY オイラー角", "Euler XZY", "XZY Euler"),
    ("EaModusYxz", "YXZ Euler", "YXZ Euler", "YXZ 欧拉", "YXZ オイラー角", "Euler YXZ", "YXZ Euler"),
    ("EaModusYzx", "YZX Euler", "YZX Euler", "YZX 欧拉", "YZX オイラー角", "Euler YZX", "YZX Euler"),
    ("EaModusZxy", "ZXY Euler", "ZXY Euler", "ZXY 欧拉", "ZXY オイラー角", "Euler ZXY", "ZXY Euler"),
    ("EaModusZyx", "ZYX Euler", "ZYX Euler", "ZYX 欧拉", "ZYX オイラー角", "Euler ZYX", "ZYX Euler"),
    ("EaModusAchse", "Axis Angle", "Achsenwinkel", "坐标轴角度", "軸の角度", "Eje y ángulo", "Úhel Osy"),
    ("EaSkalierung", "Scale", "Skalieren", "缩放", "スケール", "Escala", "Velikost"),
    ("EaLokalHint",
     "Local values like Blender: relative to the rest position of the bone (the base pose of the .gla), in its own axes.",
     "Lokale Werte wie Blender: bezogen auf die Ruhelage des Knochens (die Grundpose der .gla), in seinen eigenen Achsen.",
     "与 Blender 相同的局部值: 相对于骨骼的静止位置 (.gla 的基础姿态), 使用骨骼自身的轴。",
     "Blender と同じローカル値: ボーンのレスト位置 (.gla の基本ポーズ) を基準に、ボーン自身の軸で表します。",
     "Valores locales como Blender: respecto a la posición de reposo del hueso (la pose base del .gla), en sus propios ejes.",
     "Místní hodnoty jako Blender: vztažené ke klidové poloze kosti (základní póze .gla), v jejích vlastních osách."),
    ("EaSkalierungHint",
     "Only to read: the rig of the Model tab does not scale bones.",
     "Nur zum Ablesen: das Rig des Model-Tabs skaliert Knochen nicht.",
     "仅供查看: 模型选项卡的绑定不缩放骨骼。", "表示のみ: モデルタブのリグはボーンを拡大縮小しません。",
     "Solo para leer: el rig de la pestaña Model no escala huesos.", "Jen ke čtení: rig karty Model kosti neškáluje."),

    # --- B10: Strg+Umschalt+Z ---
    ("EaRedoBl", "Redo (Ctrl+Shift+Z)", "Wiederholen (Strg+Umschalt+Z)", "重做 (Ctrl+Shift+Z)", "やり直し (Ctrl+Shift+Z)", "Rehacer (Ctrl+Mayús+Z)", "Znovu (Ctrl+Shift+Z)"),

    # --- B11: Auge fuer Edit Poly und Skin ---
    ("EaPolyAn", "Edit Poly on/off", "Edit Poly an/aus", "Edit Poly 开/关", "Edit Poly オン/オフ", "Edit Poly activado/desactivado", "Edit Poly zap./vyp."),
    ("EaPolyAnHint",
     "Off: the view shows this surface as it was loaded, without the changes made since. Only the view - the model, undo and saving stay as they are.",
     "Aus: die Ansicht zeigt diese Fläche, wie sie geladen wurde, ohne die Änderungen seither. Nur die Anzeige - Modell, Rückgängig und Speichern bleiben, wie sie sind.",
     "关: 视图按加载时的样子显示此面, 不含此后的更改。仅影响显示 - 模型、撤销和保存保持不变。",
     "オフ: ビューはこのサーフェスを読み込んだときの状態で、その後の変更なしに表示します。表示だけで、モデル・元に戻す・保存はそのままです。",
     "Desactivado: la vista muestra esta superficie tal como se cargó, sin los cambios posteriores. Solo la vista: el modelo, deshacer y guardar no cambian.",
     "Vyp.: pohled ukazuje tuto plochu tak, jak byla načtena, bez pozdějších změn. Jen zobrazení - model, Zpět a ukládání zůstávají beze změny."),
    ("EaPolyNeu",
     "This surface did not exist when the model was loaded - without Edit Poly there is nothing to show.",
     "Diese Fläche gab es beim Laden des Modells noch nicht - ohne Edit Poly gäbe es nichts zu zeigen.",
     "加载模型时还没有这个面 - 关闭 Edit Poly 后将没有可显示的内容。",
     "このサーフェスはモデルを読み込んだときにはありませんでした。Edit Poly をオフにすると表示するものがありません。",
     "Esta superficie no existía al cargar el modelo; sin Edit Poly no habría nada que mostrar.",
     "Tato plocha při načtení modelu ještě neexistovala - bez Edit Poly by nebylo co ukázat."),
    ("EaSkinAn", "Skin on/off", "Skin an/aus", "Skin 开/关", "Skin オン/オフ", "Skin activado/desactivado", "Skin zap./vyp."),
    ("EaSkinAnHint",
     "Off: the view shows the figure in its bind pose, the bones keep their pose (like 3ds Max). For the whole figure - all its surfaces hang on the same skeleton. Only the view - keys, model and saving stay as they are.",
     "Aus: die Ansicht zeigt die Figur in ihrer Bindepose, die Knochen behalten ihre Pose (wie 3ds Max). Für die ganze Figur - alle ihre Flächen hängen am selben Skelett. Nur die Anzeige - Schlüssel, Modell und Speichern bleiben, wie sie sind.",
     "关: 视图以绑定姿态显示角色, 骨骼保持其姿态 (与 3ds Max 相同)。作用于整个角色 - 其所有面都挂在同一骨架上。仅影响显示 - 关键帧、模型和保存保持不变。",
     "オフ: ビューはフィギュアをバインドポーズで表示し、ボーンはポーズを保ちます (3ds Max と同じ)。フィギュア全体に適用されます (すべてのサーフェスが同じスケルトンに付いているため)。表示だけで、キー・モデル・保存はそのままです。",
     "Desactivado: la vista muestra la figura en su pose de enlace y los huesos conservan su pose (como 3ds Max). Para toda la figura: todas sus superficies cuelgan del mismo esqueleto. Solo la vista: claves, modelo y guardado no cambian.",
     "Vyp.: pohled ukazuje postavu ve vazební póze, kosti si drží svou pózu (jako 3ds Max). Pro celou postavu - všechny její plochy visí na stejné kostře. Jen zobrazení - klíče, model a ukládání zůstávají beze změny."),
    ("EaMeldPolyAus", "Edit Poly of %s is off: the view shows the surface as it was loaded.",
     "Edit Poly von %s ist aus: die Ansicht zeigt die Fläche, wie sie geladen wurde.",
     "%s 的 Edit Poly 已关闭: 视图按加载时的样子显示该面。", "%s の Edit Poly はオフです: ビューはサーフェスを読み込んだときの状態で表示します。",
     "Edit Poly de %s está desactivado: la vista muestra la superficie tal como se cargó.", "Edit Poly plochy %s je vypnutý: pohled ukazuje plochu tak, jak byla načtena."),
    ("EaMeldPolyAn", "Edit Poly of %s is on again.", "Edit Poly von %s ist wieder an.", "%s 的 Edit Poly 已重新打开。", "%s の Edit Poly は再びオンです。",
     "Edit Poly de %s vuelve a estar activado.", "Edit Poly plochy %s je znovu zapnutý."),
    ("EaMeldSkinAus", "Skin is off: the view shows the figure in its bind pose.", "Skin ist aus: die Ansicht zeigt die Figur in ihrer Bindepose.",
     "Skin 已关闭: 视图以绑定姿态显示角色。", "Skin はオフです: ビューはフィギュアをバインドポーズで表示します。",
     "Skin está desactivado: la vista muestra la figura en su pose de enlace.", "Skin je vypnutý: pohled ukazuje postavu ve vazební póze."),
    ("EaMeldSkinAn", "Skin is on again: the figure follows the bones.", "Skin ist wieder an: die Figur folgt den Knochen.", "Skin 已重新打开: 角色跟随骨骼。",
     "Skin は再びオンです: フィギュアはボーンに従います。", "Skin vuelve a estar activado: la figura sigue a los huesos.", "Skin je znovu zapnutý: postava sleduje kosti."),

    # --- B12: Modifier-Stapel in einer Begleitdatei ---
    ("EaBegleitGelesen", "Modifier stack restored from %s (%d modifiers).", "Modifier-Stapel aus %s wiederhergestellt (%d Modifier).",
     "已从 %s 恢复修改器堆栈 (%d 个修改器)。", "%s からモディファイアスタックを復元しました (モディファイア %d 個)。",
     "Pila de modificadores restaurada desde %s (%d modificadores).", "Zásobník modifikátorů obnoven z %s (%d modifikátorů)."),
    ("EaBegleitFehler", "The modifier stack in %s could not be read (%s) - the model is loaded without it.",
     "Der Modifier-Stapel in %s ließ sich nicht lesen (%s) - das Modell ist ohne ihn geladen.",
     "无法读取 %s 中的修改器堆栈 (%s) - 模型已在没有它的情况下加载。", "%s のモディファイアスタックを読み込めませんでした (%s)。モデルはスタックなしで読み込まれました。",
     "No se pudo leer la pila de modificadores de %s (%s); el modelo se cargó sin ella.", "Zásobník modifikátorů v %s nešel přečíst (%s) - model je načten bez něj."),
    ("EaBegleitGrundAlt", "the model file was changed outside JK Studio after saving", "die Modelldatei wurde nach dem Speichern außerhalb von JK Studio geändert",
     "保存后模型文件在 JK Studio 之外被修改过", "保存後に JK Studio 以外でモデルファイルが変更されました", "el archivo del modelo se cambió fuera de JK Studio después de guardar",
     "soubor modelu byl po uložení změněn mimo JK Studio"),
    ("EaBegleitGrundKaputt", "the file is damaged", "die Datei ist beschädigt", "文件已损坏", "ファイルが壊れています", "el archivo está dañado", "soubor je poškozený"),
    ("EaBegleitGrundFassung", "written by a newer version of JK Studio", "von einer neueren Fassung von JK Studio geschrieben", "由较新版本的 JK Studio 写入",
     "より新しいバージョンの JK Studio で書かれました", "escrito por una versión más reciente de JK Studio", "zapsán novější verzí JK Studia"),
    ("EaBegleitGrundFlaechen", "the surfaces do not match the model", "die Flächen passen nicht zum Modell", "面与模型不匹配", "サーフェスがモデルと一致しません",
     "las superficies no coinciden con el modelo", "plochy neodpovídají modelu"),
    ("EaBegleitGeschrieben", "Modifier stack saved in %s.", "Modifier-Stapel in %s gesichert.", "修改器堆栈已保存到 %s。", "モディファイアスタックを %s に保存しました。",
     "Pila de modificadores guardada en %s.", "Zásobník modifikátorů uložen do %s."),
    ("EaBegleitSchreibFehler", "The modifier stack could not be saved next to the model (%s).", "Der Modifier-Stapel ließ sich nicht neben dem Modell sichern (%s).",
     "无法在模型旁保存修改器堆栈 (%s)。", "モデルの隣にモディファイアスタックを保存できませんでした (%s)。",
     "No se pudo guardar la pila de modificadores junto al modelo (%s).", "Zásobník modifikátorů nešel uložit vedle modelu (%s)."),

    # --- B13: Zeile "Unwrap UVW" (wie 3ds Max) ---
    ("EaUnwrap", "Unwrap UVW", "Unwrap UVW", "Unwrap UVW", "Unwrap UVW", "Unwrap UVW", "Unwrap UVW"),
    ("EaUnwrapHint", "Unwrap UVW: the UVs of the surface - switches to the UV area with the same model.",
     "Unwrap UVW: die UVs der Fläche - wechselt mit demselben Modell in den Bereich UV.",
     "Unwrap UVW: 面的 UV - 带着同一模型切换到 UV 区域。", "Unwrap UVW: サーフェスの UV - 同じモデルのまま UV エリアに切り替えます。",
     "Unwrap UVW: las UV de la superficie; cambia al área UV con el mismo modelo.", "Unwrap UVW: UV souřadnice plochy - přepne se stejným modelem do oblasti UV."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
