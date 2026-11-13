# -*- coding: utf-8 -*-
"""Oberflaechentexte von ENTSCHEID3B (rc612, Entscheidungen 12.11. spaet B14-B17, C1, C3): IK/FK blendet zwischen zwei Schluesseln
ueber wie CAT (eine Spur je Glied im Dopesheet, Regler IK/FK auch wie 3ds Max), Tempo des Generators als Faktor, der Bodenkreis als
Griff der Wurzel (Plattform wie CAT), Control Rig erstellen nach nur FK, die Rig-Vorlage im Dokument - fuer tools/gen_i18n.py und die
weiteren Sprachen.

Begriffe wie die Vorbilder: "IK/FK" und "Match IK and FK" wie CAT (Autodesk-Hilfe "Limb Animation Controls"), CATParent als
"Plattform", Root wie Blender (Rigify "root").

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    # --- B14: IK/FK-Spur je Glied ---
    ("E3IkFkAnteil", "IK/FK ratio", "IK/FK-Verhältnis", "IK/FK 比例", "IK/FK 比率", "Proporción IK/FK", "Poměr IK/FK"),
    ("E3IkFkAnteilHint",
     "Like CAT: 0 = IK, 1 = FK - sets a key on the limb's IK/FK track at this frame. Between two keys of the track the limb blends over; the track has its own row in the dope sheet (move, delete, interpolation).",
     "Wie CAT: 0 = IK, 1 = FK - setzt einen Schlüssel der IK/FK-Spur des Glieds an diesem Frame. Zwischen zwei Schlüsseln der Spur blendet das Glied über; die Spur hat eine eigene Zeile im Dopesheet (verschieben, löschen, Interpolation).",
     "与 CAT 相同: 0 = IK, 1 = FK - 在当前帧为该肢体的 IK/FK 轨道设置关键帧。在轨道的两个关键帧之间肢体会平滑过渡; 该轨道在摄影表中有自己的一行 (移动、删除、插值)。",
     "CAT と同じく 0 = IK、1 = FK - このフレームで手足の IK/FK トラックにキーを打ちます。トラックの 2 つのキーの間で手足はブレンドします。トラックはドープシートに専用の行があります (移動、削除、補間)。",
     "Como CAT: 0 = IK, 1 = FK; pone una clave en la pista IK/FK de la extremidad en este fotograma. Entre dos claves de la pista la extremidad se mezcla; la pista tiene su propia fila en la hoja de claves (mover, borrar, interpolación).",
     "Jako CAT: 0 = IK, 1 = FK - nastaví klíč stopy IK/FK končetiny v tomto snímku. Mezi dvěma klíči stopy končetina plynule přechází; stopa má vlastní řádek v dope sheetu (posunout, smazat, interpolace)."),
    ("E3DsIkFk", "IK/FK %s", "IK/FK %s", "IK/FK %s", "IK/FK %s", "IK/FK %s", "IK/FK %s"),
    ("E3IkMisch", "IK %d %% · FK %d %%", "IK %d %% · FK %d %%", "IK %d %% · FK %d %%", "IK %d %% · FK %d %%", "IK %d %% · FK %d %%", "IK %d %% · FK %d %%"),
    ("E3IkMischHint", "The limb blends between two keys of its IK/FK track (dope sheet) - here it is this much IK and FK.",
     "Das Glied blendet zwischen zwei Schlüsseln seiner IK/FK-Spur über (Dopesheet) - hier ist es so viel IK und FK.",
     "该肢体正在其 IK/FK 轨道 (摄影表) 的两个关键帧之间过渡 - 此处 IK 与 FK 的比例如图所示。",
     "手足は IK/FK トラック (ドープシート) の 2 つのキーの間でブレンド中です - ここでの IK と FK の割合です。",
     "La extremidad se mezcla entre dos claves de su pista IK/FK (hoja de claves); aquí es tanto IK y tanto FK.",
     "Končetina přechází mezi dvěma klíči své stopy IK/FK (dope sheet) - zde je tolik IK a tolik FK."),
    # --- B16: Tempo als Faktor ---
    ("E3TempoHint",
     "Factor on the speed of the game: 1.00× = %.0f units per second (%s of the NPCs). Now %.0f units per second.",
     "Faktor auf das Tempo des Spiels: 1,00× = %.0f Einheiten je Sekunde (%s der NPCs). Jetzt %.0f Einheiten je Sekunde.",
     "相对游戏速度的倍数: 1.00× = 每秒 %.0f 单位 (NPC 的 %s)。当前每秒 %.0f 单位。",
     "ゲームの速度に対する倍率: 1.00× = 毎秒 %.0f ユニット (NPC の %s)。現在 毎秒 %.0f ユニット。",
     "Factor sobre la velocidad del juego: 1,00× = %.0f unidades por segundo (%s de los NPC). Ahora %.0f unidades por segundo.",
     "Násobek rychlosti hry: 1,00× = %.0f jednotek za sekundu (%s NPC). Nyní %.0f jednotek za sekundu."),
    # --- B17: Griff der Wurzel ---
    ("E3GriffWurzel", "Root (platform)", "Wurzel (Plattform)", "根 (平台)", "ルート (プラットフォーム)", "Raíz (plataforma)", "Kořen (plošina)"),
    ("E3GriffWurzelHint",
     "Moves and turns the whole figure like CAT's platform (CATParent): move with W (3ds Max) or G (Blender), turn with E or R - with a key at this frame and Ctrl+Z. IK targets in world space go along.",
     "Verschiebt und dreht die ganze Figur wie CATs Plattform (CATParent): verschieben mit W (3ds Max) oder G (Blender), drehen mit E oder R - mit einem Schlüssel an diesem Frame und Strg+Z. IK-Ziele im Raum Welt gehen mit.",
     "像 CAT 的平台 (CATParent) 一样移动和旋转整个角色: 用 W (3ds Max) 或 G (Blender) 移动, 用 E 或 R 旋转 - 在当前帧设置关键帧, 可用 Ctrl+Z 撤销。世界空间中的 IK 目标会一起移动。",
     "CAT のプラットフォーム (CATParent) と同じくフィギュア全体を移動・回転します: W (3ds Max) か G (Blender) で移動、E か R で回転 - このフレームにキーを打ち、Ctrl+Z で戻せます。ワールド空間の IK ターゲットも一緒に動きます。",
     "Mueve y gira toda la figura como la plataforma de CAT (CATParent): mover con W (3ds Max) o G (Blender), girar con E o R, con una clave en este fotograma y Ctrl+Z. Los objetivos IK en espacio del mundo van con ella.",
     "Posouvá a otáčí celou postavu jako plošina CAT (CATParent): posun W (3ds Max) nebo G (Blender), otočení E nebo R - s klíčem v tomto snímku a Ctrl+Z. Cíle IK v prostoru světa jdou s ní."),
    ("E3WurzelGewaehlt", "Root (platform) selected - move or turn it to move the whole figure.",
     "Wurzel (Plattform) gewählt - verschieben oder drehen bewegt die ganze Figur.",
     "已选择根 (平台) - 移动或旋转它即可移动整个角色。",
     "ルート (プラットフォーム) を選択しました - 移動・回転でフィギュア全体が動きます。",
     "Raíz (plataforma) seleccionada: moverla o girarla mueve toda la figura.",
     "Vybrán kořen (plošina) - posunutím nebo otočením se pohne celá postava."),
    ("E3WurzelGeloescht", "Key of the platform at frame %d deleted.", "Schlüssel der Plattform an Frame %d gelöscht.", "已删除第 %d 帧的平台关键帧。",
     "フレーム %d のプラットフォームのキーを削除しました。", "Clave de la plataforma en el fotograma %d borrada.", "Klíč plošiny ve snímku %d smazán."),
    # --- C1 ---
    ("E3RigWiederIk",
     "Control rig: %d limbs are IK again in %d sequences - their FK keys went to IK targets and poles like the first time; the motion stays within %.4f units.",
     "Control Rig: %d Glieder sind in %d Sequenzen wieder IK - ihre FK-Schlüssel gingen wie beim ersten Erstellen auf IK-Ziele und Pole; die Bewegung bleibt auf %.4f Einheiten gleich.",
     "控制绑定: %d 个肢体在 %d 个序列中重新为 IK - 其 FK 关键帧像首次创建时一样转到 IK 目标和极向; 动作偏差在 %.4f 单位以内。",
     "コントロールリグ: %d 本の手足が %d 個のシーケンスで再び IK になりました - FK キーは最初の作成時と同じく IK ターゲットとポールに移りました。動きのずれは %.4f ユニット以内です。",
     "Rig de control: %d extremidades vuelven a ser IK en %d secuencias; sus claves FK pasaron a objetivos IK y polos como la primera vez; el movimiento se mantiene dentro de %.4f unidades.",
     "Ovládací rig: %d končetin je v %d sekvencích znovu IK - jejich klíče FK přešly jako při prvním vytvoření na cíle IK a póly; pohyb se liší nejvýš o %.4f jednotek."),
    # --- C3 ---
    ("E3UndoVorlage", "Rig template", "Rig-Vorlage", "绑定模板", "リグテンプレート", "Plantilla de rig", "Šablona rigu"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
