# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc609 B1 (Brush-Entities in fertige Karten einfuegen) fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("KeBmEinfuegen", "Insert into compiled map", "In fertige Karte einfügen", "插入到已编译地图", "コンパイル済みマップに挿入",
     "Insertar en el mapa compilado", "Vložit do hotové mapy"),
    ("KeBmEinfuegenTip",
     "Builds only the new brush entities (triggers, func_usable with nodraw, origin brushes) with q3map2 and appends them to the open .bsp as new models *N. "
     "The world stays byte for byte: BSP tree, VIS, lightmaps and surfaces.",
     "Baut nur die neuen Brush-Entities (Trigger, func_usable mit nodraw, origin-Brushes) mit q3map2 und hängt sie als neue Modelle *N an die offene .bsp. "
     "Die Welt bleibt Byte für Byte: BSP-Baum, VIS, Lightmaps und Flächen.",
     "只用 q3map2 编译新的画刷实体(触发器、带 nodraw 的 func_usable、origin 画刷),并把它们作为新模型 *N 追加到打开的 .bsp。"
     "世界逐字节保持不变:BSP 树、VIS、光照贴图和表面。",
     "新しいブラシエンティティ (トリガー、nodraw の func_usable、origin ブラシ) だけを q3map2 でビルドし、開いている .bsp に新しいモデル *N として追加します。"
     "ワールドはバイト単位でそのままです: BSP ツリー、VIS、ライトマップ、サーフェス。",
     "Compila con q3map2 solo las entidades de brush nuevas (triggers, func_usable con nodraw, brushes origin) y las añade al .bsp abierto como modelos nuevos *N. "
     "El mundo queda byte a byte: árbol BSP, VIS, lightmaps y superficies.",
     "Sestaví q3map2 jen nové brush entity (triggery, func_usable s nodraw, origin brushe) a připojí je k otevřenému .bsp jako nové modely *N. "
     "Svět zůstane bajt po bajtu: strom BSP, VIS, lightmapy a plochy."),
    ("KeBmEinfuegenKeine",
     "There are no new brush entities. Build triggers or a func_usable with nodraw brushes first.",
     "Es gibt keine neuen Brush-Entities. Zuerst Trigger oder einen func_usable mit nodraw-Brushes bauen.",
     "没有新的画刷实体。请先建立触发器或带 nodraw 画刷的 func_usable。",
     "新しいブラシエンティティがありません。先にトリガーか nodraw ブラシの func_usable を作ってください。",
     "No hay entidades de brush nuevas. Crea primero triggers o un func_usable con brushes nodraw.",
     "Nejsou žádné nové brush entity. Nejprve vytvořte triggery nebo func_usable s nodraw brushi."),
    ("KeBmEinfuegenGeaendert",
     "World brushes or existing brush entities were changed. Only new brush entities can be inserted into a compiled map; everything else needs a full build.",
     "Welt-Brushes oder vorhandene Brush-Entities wurden geändert. In eine fertige Karte lassen sich nur neue Brush-Entities einfügen; alles andere braucht einen vollen Bau.",
     "世界画刷或已有的画刷实体已被修改。只能向已编译地图插入新的画刷实体;其他修改需要完整编译。",
     "ワールドブラシか既存のブラシエンティティが変更されています。コンパイル済みマップに挿入できるのは新しいブラシエンティティだけです。それ以外はフルビルドが必要です。",
     "Se cambiaron brushes del mundo o entidades de brush existentes. En un mapa compilado solo se pueden insertar entidades de brush nuevas; lo demás necesita una compilación completa.",
     "Byly změněny brushe světa nebo existující brush entity. Do hotové mapy lze vložit jen nové brush entity; vše ostatní potřebuje úplné sestavení."),
    ("KeBmEinfuegenFertig", "Inserted %d brush entities as *%d to *%d into %s.", "%d Brush-Entities als *%d bis *%d in %s eingefügt.",
     "已将 %d 个画刷实体作为 *%d 到 *%d 插入 %s。", "%d 個のブラシエンティティを *%d から *%d として %s に挿入しました。",
     "Se insertaron %d entidades de brush como *%d a *%d en %s.", "Vloženo %d brush entit jako *%d až *%d do %s."),
    ("KeBmEinfuegenPunkte",
     "%d existing point entities were changed or removed; they stay as in the compiled map. Use Build > Update entities only for them.",
     "%d vorhandene Punkt-Entities wurden geändert oder entfernt; sie bleiben wie in der fertigen Karte. Dafür Build > Nur Entities aktualisieren.",
     "%d 个已有点实体被修改或删除;它们保持已编译地图中的样子。请对它们使用 Build > 仅更新实体。",
     "%d 個の既存ポイントエンティティが変更または削除されました。コンパイル済みマップのままです。それには Build > エンティティのみ更新 を使ってください。",
     "%d entidades de punto existentes se cambiaron o eliminaron; quedan como en el mapa compilado. Para ellas usa Build > Actualizar solo las entidades.",
     "%d existujících bodových entit bylo změněno nebo odstraněno; zůstávají jako v hotové mapě. Pro ně použijte Build > Aktualizovat jen entity."),
    ("KeBmEinfuegenFehler", "Not inserted: %s", "Nicht eingefügt: %s", "未插入:%s", "挿入されませんでした: %s", "No insertado: %s", "Nevloženo: %s"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
