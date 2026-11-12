# -*- coding: utf-8 -*-
"""Oberflaechentexte von ENTSCHEID2 (rc612, Entscheidungen von shank 12.11. zu AP10): Control Rig wie CAT (die Animation geht beim
Erstellen von FK auf IK, IK/FK je Glied ab dem Bild umschalten ohne Sprung), die acht Preset-Kategorien des Entwurfs (auch leer)
und die Gaenge des Generators als Eintraege der Preset-Liste - fuer tools/gen_i18n.py und die weiteren Sprachen.

Begriffe wie die Vorbilder: "Match IK and FK" und "IK/FK" wie CAT (Autodesk-Hilfe "Limb Animation Controls"), Pose/IK/FK wie
Blenders Uebersetzung (Blender 5.0 blender.mo, wie ENT1 nur gelesen).

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    # --- Control Rig wie CAT ---
    ("EnAufRigMeldung",
     "%s is on the rig from FK to IK: %d keys from %d frames; %d limbs follow their IK targets and poles, the motion stays within %.3f units.",
     "%s liegt von FK auf IK auf dem Rig: %d Schlüssel aus %d Frames; %d Glieder folgen ihren IK-Zielen und Polen, die Bewegung bleibt auf %.3f Einheiten gleich.",
     "%s 已从 FK 转到 IK 放到绑定上: %d 个关键帧 (来自 %d 帧); %d 个肢体跟随其 IK 目标和极向, 动作偏差在 %.3f 单位以内。",
     "%s を FK から IK に移してリグに載せました: キー %d 個 (%d フレームから)。%d 本の手足が IK ターゲットとポールに追従し、動きのずれは %.3f ユニット以内です。",
     "%s está en el rig de FK a IK: %d claves de %d fotogramas; %d extremidades siguen sus objetivos IK y polos, el movimiento se mantiene dentro de %.3f unidades.",
     "%s je na rigu z FK na IK: %d klíčů z %d snímků; %d končetin sleduje své cíle IK a póly, pohyb se liší nejvýš o %.3f jednotek."),
    ("EnAufRigNurFk", "%s is on the FK-only rig: %d keys from %d frames, the motion stays within %.3f units.",
     "%s liegt auf dem Rig nur mit FK: %d Schlüssel aus %d Frames, die Bewegung bleibt auf %.3f Einheiten gleich.",
     "%s 已放到仅 FK 的绑定上: %d 个关键帧 (来自 %d 帧), 动作偏差在 %.3f 单位以内。",
     "%s を FK のみのリグに載せました: キー %d 個 (%d フレームから)、動きのずれは %.3f ユニット以内です。",
     "%s está en el rig solo FK: %d claves de %d fotogramas, el movimiento se mantiene dentro de %.3f unidades.",
     "%s je na rigu jen s FK: %d klíčů z %d snímků, pohyb se liší nejvýš o %.3f jednotek."),
    ("EnIkAbIk", "%s is IK from frame %d to %d - the IK target and pole were matched to the pose, nothing jumps.",
     "%s ist IK von Frame %d bis %d - IK-Ziel und Pol wurden an die Pose angeglichen, nichts springt.",
     "%s 从第 %d 帧到第 %d 帧为 IK - IK 目标和极向已匹配到姿势, 不会跳变。",
     "%s はフレーム %d から %d まで IK です - IK ターゲットとポールをポーズに合わせたので、ジャンプしません。",
     "%s es IK del fotograma %d al %d: el objetivo IK y el polo se igualaron a la pose, nada salta.",
     "%s je IK od snímku %d do %d - cíl IK a pól byly srovnány s pózou, nic neskočí."),
    ("EnIkAbFk", "%s is FK from frame %d to %d - the FK keys took the visible pose, nothing jumps.",
     "%s ist FK von Frame %d bis %d - die FK-Schlüssel haben die sichtbare Pose übernommen, nichts springt.",
     "%s 从第 %d 帧到第 %d 帧为 FK - FK 关键帧已采用可见姿势, 不会跳变。",
     "%s はフレーム %d から %d まで FK です - FK キーが見えているポーズを受け取ったので、ジャンプしません。",
     "%s es FK del fotograma %d al %d: las claves FK tomaron la pose visible, nada salta.",
     "%s je FK od snímku %d do %d - klíče FK převzaly viditelnou pózu, nic neskočí."),
    ("EnIkKnopfHint",
     "Like the IK/FK of a CAT limb: switches this limb from the current frame up to the next switch and matches the other side first (Match IK and FK) - the pose does not jump.",
     "Wie IK/FK eines CAT-Glieds: schaltet dieses Glied ab dem aktuellen Frame bis zum nächsten Wechsel um und gleicht vorher die andere Seite an (Match IK and FK) - die Pose springt nicht.",
     "与 CAT 肢体的 IK/FK 相同: 从当前帧起直到下一次切换, 切换此肢体, 并先匹配另一侧 (Match IK and FK) - 姿势不会跳变。",
     "CAT の手足の IK/FK と同じ: 現在のフレームから次の切り替えまでこの手足を切り替え、先にもう一方を合わせます (Match IK and FK) - ポーズはジャンプしません。",
     "Como el IK/FK de una extremidad de CAT: cambia esta extremidad desde el fotograma actual hasta el siguiente cambio e iguala antes el otro lado (Match IK and FK); la pose no salta.",
     "Jako IK/FK končetiny CAT: přepne tuto končetinu od aktuálního snímku do další změny a předtím srovná druhou stranu (Match IK and FK) - póza neskočí."),
    # --- Presets: Kategorien des Entwurfs, auch leer ---
    ("EnPresetsLeer",
     "No presets in this category yet. More will come, only with a free licence (CC0, CC BY); your own animations come in with Import animation.",
     "In dieser Kategorie gibt es noch keine Presets. Weitere kommen, nur mit freier Lizenz (CC0, CC BY); eigene Animationen kommen über Animation importieren.",
     "此类别中还没有预设。以后会添加, 但只限自由许可 (CC0、CC BY); 自己的动画请通过“导入动画”加入。",
     "このカテゴリにはまだプリセットがありません。今後追加しますが、自由なライセンス (CC0、CC BY) のものだけです。自分のアニメーションは「アニメーションをインポート」から取り込めます。",
     "Aún no hay presets en esta categoría. Llegarán más, solo con licencia libre (CC0, CC BY); tus propias animaciones entran con Importar animación.",
     "V této kategorii zatím nejsou žádné předvolby. Další přibudou, jen se svobodnou licencí (CC0, CC BY); vlastní animace se přidají přes Importovat animaci."),
    # --- Generator-Gaenge in der Liste ---
    ("EnGangGehen", "Walk (generated)", "Gehen (erzeugt)", "行走 (生成)", "歩き (生成)", "Andar (generado)", "Chůze (generovaná)"),
    ("EnGangLaufen", "Run (generated)", "Laufen (erzeugt)", "跑步 (生成)", "走り (生成)", "Correr (generado)", "Běh (generovaný)"),
    ("EnGangQuelle", "Generator", "Generator", "生成器", "ジェネレーター", "Generador", "Generátor"),
    ("EnGangHint",
     "Made by JK Studio's own gait generator on this figure's skeleton (no file of anyone else). Speed and arm swing stand under the list, more under Generator...",
     "Vom eigenen Gang-Generator von JK Studio auf dem Skelett dieser Figur erzeugt (keine fremde Datei). Tempo und Armschwung stehen unter der Liste, mehr unter Generator ...",
     "由 JK Studio 自带的步态生成器在此角色的骨架上生成 (不使用他人的文件)。速度和摆臂在列表下方, 更多设置在“生成器...”中。",
     "JK Studio 独自の歩容ジェネレーターがこのフィギュアのスケルトン上で作ります (他人のファイルは使いません)。速度と腕の振りは一覧の下、その他は「ジェネレーター...」にあります。",
     "Creado por el generador de marcha propio de JK Studio sobre el esqueleto de esta figura (sin archivos ajenos). Velocidad y balanceo de brazos están bajo la lista, más en Generador...",
     "Vytvořeno vlastním generátorem chůze JK Studia na kostře této postavy (žádný cizí soubor). Rychlost a švih paží jsou pod seznamem, další pod Generátor..."),
    ("EnGeneratorHint",
     "More settings of the generated walk or run: posture, step length, hip swing, bounce and measuring the figure. Choose Walk (generated) or Run (generated) in the list.",
     "Weitere Einstellungen des erzeugten Gangs: Haltung, Schrittlänge, Hüftschwung, Hub und die Figur messen. In der Liste Gehen (erzeugt) oder Laufen (erzeugt) wählen.",
     "生成步态的更多设置: 姿态、步长、摆胯、起伏以及测量角色。在列表中选择“行走 (生成)”或“跑步 (生成)”。",
     "生成した歩き・走りのその他の設定: 姿勢、歩幅、腰の振り、上下動、フィギュアの計測。一覧で「歩き (生成)」か「走り (生成)」を選びます。",
     "Más ajustes de la marcha generada: postura, longitud de paso, balanceo de cadera, rebote y medir la figura. Elige Andar (generado) o Correr (generado) en la lista.",
     "Další nastavení generované chůze nebo běhu: postoj, délka kroku, švih boků, pohupování a změření postavy. V seznamu zvolte Chůze (generovaná) nebo Běh (generovaný)."),
    ("EnGangWaehlen", "Generated walk in the preset list", "Erzeugten Gang in der Preset-Liste wählen", "在预设列表中选择生成的步态",
     "プリセット一覧で生成した歩きを選ぶ", "Elegir la marcha generada en la lista de presets", "Zvolit generovanou chůzi v seznamu předvoleb"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
