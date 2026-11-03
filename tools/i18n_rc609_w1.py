# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc609 W1 (Wegpunkte loeschen, verbinden, duplizieren) fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("WegKantenTitel", "Connections", "Verbindungen", "连接", "接続", "Conexiones", "Spojení"),
    ("WegAutoVerbinden", "Connect automatically", "Automatisch verbinden", "自动连接", "自動で接続", "Conectar automáticamente",
     "Automaticky spojovat"),
    ("WegAutoVerbindenHint",
     "A waypoint you set, move or duplicate connects to the waypoints within the grid spacing that the game connects too (its edge test, nearest first). After moving, connections the game would drop are removed. One undo step.",
     "Ein Wegpunkt, den du setzt, verschiebst oder duplizierst, verbindet sich mit den Wegpunkten im Umkreis der Rasterweite, die auch das Spiel verbindet (seine Kantenprüfung, die nächsten zuerst). Nach dem Verschieben fallen Verbindungen weg, die das Spiel verwerfen würde. Ein Rückgängig-Schritt.",
     "你放置、移动或复制的路径点会与网格间距范围内、游戏同样会连接的路径点相连(游戏的连线检查,最近的优先)。移动后,游戏会丢弃的连接将被移除。一次撤销步骤。",
     "置いた・移動した・複製したウェイポイントは、グリッド間隔の範囲内でゲームも接続するウェイポイントと接続します(ゲームの辺チェック、近いものから)。移動後は、ゲームが捨てる接続を削除します。元に戻すは 1 回です。",
     "Un punto de ruta que colocas, mueves o duplicas se conecta con los puntos dentro del espaciado de la cuadrícula que el juego también conecta (su prueba de aristas, los más cercanos primero). Tras moverlo se quitan las conexiones que el juego descartaría. Un paso de deshacer.",
     "Bod trasy, který položíš, přesuneš nebo zduplikuješ, se spojí s body v okruhu rozteče mřížky, které spojuje i hra (její test hran, nejbližší první). Po přesunu zmizí spojení, která by hra zahodila. Jeden krok zpět."),
    ("WegLuecke", "Close the gap when deleting", "Lücke beim Löschen schließen", "删除时填补空缺", "削除時に隙間を埋める",
     "Cerrar el hueco al borrar", "Při mazání uzavřít mezeru"),
    ("WegLueckeHint",
     "When you delete a waypoint, its neighbours connect to each other where the game's edge test allows it.",
     "Löschst du einen Wegpunkt, verbinden sich seine Nachbarn untereinander, wo die Kantenprüfung des Spiels es erlaubt.",
     "删除路径点时,其相邻点会在游戏连线检查允许的地方彼此相连。",
     "ウェイポイントを削除すると、ゲームの辺チェックが許す範囲で隣どうしが接続されます。",
     "Al borrar un punto de ruta, sus vecinos se conectan entre sí donde la prueba de aristas del juego lo permite.",
     "Když smažeš bod trasy, jeho sousedé se navzájem spojí tam, kde to test hran hry dovolí."),
    ("WegNachbarn", "Connect to neighbours", "Mit Nachbarn verbinden", "连接到邻近点", "近くの点と接続", "Conectar con los vecinos",
     "Spojit se sousedy"),
    ("WegNachbarnHint",
     "Connects the selected waypoints to the waypoints within the grid spacing that pass the game's edge test - also when automatic connecting is off.",
     "Verbindet die gewählten Wegpunkte mit den Wegpunkten im Umkreis der Rasterweite, die die Kantenprüfung des Spiels bestehen - auch wenn automatisches Verbinden aus ist.",
     "将选中的路径点与网格间距范围内通过游戏连线检查的路径点相连——即使自动连接已关闭。",
     "選択したウェイポイントを、グリッド間隔の範囲内でゲームの辺チェックに通るウェイポイントと接続します。自動接続がオフでも動作します。",
     "Conecta los puntos de ruta seleccionados con los puntos dentro del espaciado de la cuadrícula que superan la prueba de aristas del juego, también con la conexión automática desactivada.",
     "Spojí vybrané body trasy s body v okruhu rozteče mřížky, které projdou testem hran hry - i když je automatické spojování vypnuté."),
    ("WegUmbenennen", "Rename", "Umbenennen", "重命名", "名前を変更", "Renombrar", "Přejmenovat"),
    ("WegMusterHint",
     "Renames the selected waypoints by this pattern: # becomes a number (wp_# gives wp_1, wp_2, ...), names in use are skipped. Every target that named them follows.",
     "Benennt die gewählten Wegpunkte nach diesem Muster um: # wird eine Nummer (wp_# ergibt wp_1, wp_2, ...), vergebene Namen werden übersprungen. Jedes target, das sie nannte, zieht mit.",
     "按此模式重命名选中的路径点:# 变为数字(wp_# 得到 wp_1、wp_2……),已用的名称会跳过。所有指向它们的 target 会一起更新。",
     "選択したウェイポイントをこのパターンで名前変更します。# は番号になり(wp_# なら wp_1, wp_2, ...)、使用中の名前は飛ばします。それらを指していた target もすべて追従します。",
     "Renombra los puntos de ruta seleccionados con este patrón: # se convierte en un número (wp_# da wp_1, wp_2, ...); se omiten los nombres en uso. Todos los target que los nombraban se actualizan.",
     "Přejmenuje vybrané body trasy podle tohoto vzoru: # se změní na číslo (wp_# dá wp_1, wp_2, ...), použitá jména se přeskočí. Každý target, který je jmenoval, se změní s nimi."),
    ("WegUmbenannt", "%d {waypoint renamed|waypoints renamed}", "%d {Wegpunkt umbenannt|Wegpunkte umbenannt}", "已重命名 %d 个路径点",
     "%d 個のウェイポイントの名前を変更しました", "%d {punto de ruta renombrado|puntos de ruta renombrados}", "Přejmenováno bodů trasy: %d"),
    ("WegAutoVoll",
     "%d {neighbour|neighbours} not connected: all four target fields are taken on both points",
     "%d {Nachbar|Nachbarn} nicht verbunden: an beiden Punkten sind alle vier target-Felder belegt",
     "%d 个邻近点未连接:两个点的四个 target 字段都已占用",
     "%d 個の近くの点を接続できません: 両方の点で 4 つの target 欄がすべて使用中です",
     "%d {vecino no conectado|vecinos no conectados}: en ambos puntos están ocupados los cuatro campos target",
     "Nespojení sousedé: %d - u obou bodů jsou obsazena všechna čtyři pole target"),
    ("WegImFesten",
     "%d {waypoint|waypoints} not connected: the game drops it as in solid (too close to a wall or step)",
     "%d {Wegpunkt|Wegpunkte} nicht verbunden: das Spiel verwirft ihn als im Festen (zu nah an Wand oder Stufe)",
     "%d 个路径点未连接:游戏会把它当作位于实体内而丢弃(离墙或台阶太近)",
     "%d 個のウェイポイントを接続できません: ゲームは固体の中にあるとして捨てます(壁や段差に近すぎます)",
     "%d {punto de ruta no conectado|puntos de ruta no conectados}: el juego lo descarta por estar dentro de un sólido (demasiado cerca de una pared o un escalón)",
     "Nespojené body trasy: %d - hra je zahodí jako uvnitř pevné hmoty (příliš blízko stěny nebo schodu)"),
    ("WegAutoErgebnis", "Connections like the game: %d new, %d removed", "Verbindungen wie im Spiel: %d neu, %d getrennt",
     "与游戏一致的连接:新增 %d,移除 %d", "ゲームと同じ接続: 新規 %d、削除 %d", "Conexiones como en el juego: %d nuevas, %d quitadas",
     "Spojení jako ve hře: %d nových, %d odebraných"),
    ("EntVerweiseFrage",
     "%d target fields (target, target2-4, targetJump) name the deleted entities. Remove these references too? No deletes only the entities.",
     "%d target-Felder (target, target2-4, targetJump) nennen die gelöschten Entities. Diese Verweise mit entfernen? Nein löscht nur die Entities.",
     "有 %d 个 target 字段(target、target2-4、targetJump)指向被删除的实体。是否同时移除这些引用?选「否」只删除实体。",
     "%d 個の target 欄(target、target2-4、targetJump)が削除するエンティティを指しています。これらの参照も削除しますか? 「いいえ」はエンティティだけを削除します。",
     "%d campos target (target, target2-4, targetJump) nombran las entidades borradas. ¿Quitar también estas referencias? No borra solo las entidades.",
     "Polí target (target, target2-4, targetJump), která jmenují mazané entity: %d. Odebrat i tyto odkazy? Ne smaže jen entity."),
]


def zeilen_vier():
    """(Kennung, English, Deutsch, 中文, 日本語) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
