# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc609 B1 (Brush-Entities in fertige Karten einfuegen) fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("KeBmEinfuegen", "Insert into compiled map", "In fertige Karte einfügen", "插入到已编译地图", "コンパイル済みマップに挿入",
     "Insertar en el mapa compilado", "Vložit do hotové mapy"),
    ("KeBmEinfuegenTip",
     "Builds only the new brush entities (triggers, func_usable, doors, trains, func_static) and new clip brushes with q3map2 and appends them to the open .bsp: "
     "entities as new models *N, visible ones lit with the world as shadow caster, clips into the leaves of the world. "
     "BSP tree, VIS, lightmaps and surfaces of the world stay byte for byte.",
     "Baut nur die neuen Brush-Entities (Trigger, func_usable, Türen, Züge, func_static) und neue Clip-Brushes mit q3map2 und hängt sie an die offene .bsp: "
     "Entities als neue Modelle *N, sichtbare mit der Welt als Schattenwerfer beleuchtet, Clips in die Blätter der Welt. "
     "BSP-Baum, VIS, Lightmaps und Flächen der Welt bleiben Byte für Byte.",
     "只用 q3map2 编译新的画刷实体(触发器、func_usable、门、列车、func_static)和新的裁剪画刷,并追加到打开的 .bsp:"
     "实体作为新模型 *N,可见实体以世界作为投影体进行光照,裁剪画刷进入世界的叶节点。"
     "世界的 BSP 树、VIS、光照贴图和表面逐字节保持不变。",
     "新しいブラシエンティティ (トリガー、func_usable、ドア、トレイン、func_static) と新しいクリップブラシだけを q3map2 でビルドし、開いている .bsp に追加します: "
     "エンティティは新しいモデル *N として、見えるものはワールドを影の投影元にしてライティングし、クリップはワールドのリーフに入れます。"
     "ワールドの BSP ツリー、VIS、ライトマップ、サーフェスはバイト単位でそのままです。",
     "Compila con q3map2 solo las entidades de brush nuevas (triggers, func_usable, puertas, trenes, func_static) y los brushes clip nuevos y los añade al .bsp abierto: "
     "las entidades como modelos nuevos *N, las visibles iluminadas con el mundo como proyector de sombras, los clips en las hojas del mundo. "
     "El árbol BSP, VIS, lightmaps y superficies del mundo quedan byte a byte.",
     "Sestaví q3map2 jen nové brush entity (triggery, func_usable, dveře, vlaky, func_static) a nové clip brushe a připojí je k otevřenému .bsp: "
     "entity jako nové modely *N, viditelné osvětlené se světem jako vrhačem stínů, clipy do listů světa. "
     "Strom BSP, VIS, lightmapy a plochy světa zůstanou bajt po bajtu."),
    ("KeBmEinfuegenKeine",
     "There are no new brush entities or clip brushes. Build triggers, a door or clip brushes first.",
     "Es gibt keine neuen Brush-Entities oder Clip-Brushes. Zuerst Trigger, eine Tür oder Clip-Brushes bauen.",
     "没有新的画刷实体或裁剪画刷。请先建立触发器、门或裁剪画刷。",
     "新しいブラシエンティティもクリップブラシもありません。先にトリガー、ドア、クリップブラシを作ってください。",
     "No hay entidades de brush ni brushes clip nuevos. Crea primero triggers, una puerta o brushes clip.",
     "Nejsou žádné nové brush entity ani clip brushe. Nejprve vytvořte triggery, dveře nebo clip brushe."),
    ("KeBmEinfuegenGeaendert",
     "World brushes or existing brush entities were changed or removed. Only new brush entities and new clip brushes can be inserted into a compiled map; everything else needs a full build.",
     "Welt-Brushes oder vorhandene Brush-Entities wurden geändert oder entfernt. In eine fertige Karte lassen sich nur neue Brush-Entities und neue Clip-Brushes einfügen; alles andere braucht einen vollen Bau.",
     "世界画刷或已有的画刷实体已被修改或删除。只能向已编译地图插入新的画刷实体和新的裁剪画刷;其他修改需要完整编译。",
     "ワールドブラシか既存のブラシエンティティが変更または削除されています。コンパイル済みマップに挿入できるのは新しいブラシエンティティと新しいクリップブラシだけです。それ以外はフルビルドが必要です。",
     "Se cambiaron o eliminaron brushes del mundo o entidades de brush existentes. En un mapa compilado solo se pueden insertar entidades de brush nuevas y brushes clip nuevos; lo demás necesita una compilación completa.",
     "Byly změněny nebo odstraněny brushe světa nebo existující brush entity. Do hotové mapy lze vložit jen nové brush entity a nové clip brushe; vše ostatní potřebuje úplné sestavení."),
    ("KeBmEinfuegenFertig", "Inserted %d {brush entity|brush entities} as *%d to *%d into %s.", "%d {Brush-Entity|Brush-Entities} als *%d bis *%d in %s eingefügt.",
     "已将 %d 个画刷实体作为 *%d 到 *%d 插入 %s。", "%d 個のブラシエンティティを *%d から *%d として %s に挿入しました。",
     "{Se insertó|Se insertaron} %d {entidad de brush|entidades de brush} como *%d a *%d en %s.", "{Vložena|Vloženy|Vloženo} %d {brush entita|brush entity|brush entit} jako *%d až *%d do %s."),
    ("KeBmEinfuegenPunkte",
     "%d existing point {entity was|entities were} changed or removed; {it stays|they stay} as in the compiled map. Use Build > Update entities only for {it|them}.",
     "%d {vorhandene Punkt-Entity wurde|vorhandene Punkt-Entities wurden} geändert oder entfernt; {sie bleibt|sie bleiben} wie in der fertigen Karte. Dafür Build > Nur Entities aktualisieren.",
     "%d 个已有点实体被修改或删除;它们保持已编译地图中的样子。请对它们使用 Build > 仅更新实体。",
     "%d 個の既存ポイントエンティティが変更または削除されました。コンパイル済みマップのままです。それには Build > エンティティのみ更新 を使ってください。",
     "%d {entidad de punto existente se cambió o eliminó; queda|entidades de punto existentes se cambiaron o eliminaron; quedan} como en el mapa compilado. Para {ella|ellas} usa Build > Actualizar solo las entidades.",
     "%d {existující bodová entita byla změněna nebo odstraněna; zůstává|existující bodové entity byly změněny nebo odstraněny; zůstávají|existujících bodových entit bylo změněno nebo odstraněno; zůstávají} jako v hotové mapě. Pro {ni|ně|ně} použijte Build > Aktualizovat jen entity."),
    ("KeBmEinfuegenFehler", "Not inserted: %s", "Nicht eingefügt: %s", "未插入:%s", "挿入されませんでした: %s", "No insertado: %s", "Nevloženo: %s"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
