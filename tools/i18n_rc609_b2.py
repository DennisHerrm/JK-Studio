# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc609 B2 (Mission <-> Map ohne Verlust) fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("KmMenue", "Edit map of the mission", "Karte der Mission bearbeiten", "编辑任务的地图", "ミッションのマップを編集",
     "Editar el mapa de la misión", "Upravit mapu mise"),
    ("KmMenueTip",
     "Opens the map of the mission open in the Mission tab - its source if it lies next to the .bsp, otherwise the .bsp converted back. "
     "Building writes into exactly the mission's file or .pk3, and the Mission tab reloads the map and keeps its unsaved changes.",
     "Öffnet die Karte der Mission im Mission-Tab - ihre Quelle, wenn sie neben der .bsp liegt, sonst die zurückgewandelte .bsp. "
     "Der Bau schreibt in genau die Datei bzw. .pk3 der Mission, und der Mission-Tab lädt die Karte nach und behält seine ungespeicherten Änderungen.",
     "打开任务选项卡中任务的地图——若源文件在 .bsp 旁边则打开源文件,否则打开反编译的 .bsp。"
     "编译会写入任务原本的文件或 .pk3,任务选项卡随后重新载入地图并保留未保存的更改。",
     "ミッションタブで開いているミッションのマップを開きます。.bsp の隣にソースがあればソースを、なければ逆変換した .bsp を開きます。"
     "ビルドはミッションのファイルまたは .pk3 にそのまま書き込み、ミッションタブはマップを読み直して未保存の変更を保持します。",
     "Abre el mapa de la misión abierta en la pestaña Misión: su fuente si está junto al .bsp; si no, el .bsp convertido. "
     "La compilación escribe exactamente en el archivo o .pk3 de la misión, y la pestaña Misión recarga el mapa y conserva sus cambios sin guardar.",
     "Otevře mapu mise otevřené na kartě Mise - její zdroj, pokud leží vedle .bsp, jinak zpětně převedenou .bsp. "
     "Sestavení zapíše přesně do souboru nebo .pk3 mise a karta Mise mapu znovu načte a ponechá neuložené změny."),
    ("KmKeineMission", "No map is open in the Mission tab.", "Im Mission-Tab ist keine Karte offen.", "任务选项卡中没有打开地图。",
     "ミッションタブでマップが開かれていません。", "No hay ningún mapa abierto en la pestaña Misión.", "Na kartě Mise není otevřená žádná mapa."),
    ("KmNichtGelesen", "The map of the mission could not be read: %s", "Die Karte der Mission lässt sich nicht lesen: %s",
     "无法读取任务的地图:%s", "ミッションのマップを読み込めません: %s", "No se puede leer el mapa de la misión: %s", "Mapu mise nelze přečíst: %s"),
    ("KmVerbunden", "Map of the mission %s: building writes into %s, then the Mission tab reloads it.",
     "Karte der Mission %s: der Bau schreibt in %s, danach lädt der Mission-Tab sie nach.",
     "任务地图 %s:编译写入 %s,之后任务选项卡会重新载入。", "ミッションのマップ %s: ビルドは %s に書き込み、その後ミッションタブが読み直します。",
     "Mapa de la misión %s: la compilación escribe en %s y luego la pestaña Misión lo recarga.",
     "Mapa mise %s: sestavení zapíše do %s, potom ji karta Mise znovu načte."),
    ("KmNachgeladen", "Map rebuilt and reloaded: %s - the unsaved changes of the mission are kept.",
     "Karte neu gebaut und nachgeladen: %s - die ungespeicherten Änderungen der Mission bleiben.",
     "地图已重新编译并重新载入:%s——任务的未保存更改已保留。", "マップを再ビルドして読み直しました: %s - ミッションの未保存の変更は保持されています。",
     "Mapa recompilado y recargado: %s; se conservan los cambios sin guardar de la misión.",
     "Mapa znovu sestavena a načtena: %s - neuložené změny mise zůstávají."),
    ("KmGebrochen", "References broken by the new build: %s - see the list.", "Durch den neuen Bau gebrochene Bezüge: %s - siehe Liste.",
     "新编译导致引用失效:%s——请查看列表。", "新しいビルドで壊れた参照: %s - 一覧を参照してください。",
     "Referencias rotas por la nueva compilación: %s; consulta la lista.", "Odkazy porušené novým sestavením: %s - viz seznam."),
    ("KmTitel", "Mission and map: references", "Mission und Karte: Bezüge", "任务与地图:引用", "ミッションとマップ: 参照",
     "Misión y mapa: referencias", "Mise a mapa: odkazy"),
    ("KmSpalteArt", "Problem", "Problem", "问题", "問題", "Problema", "Problém"),
    ("KmSpalteWas", "Reference", "Bezug", "引用", "参照", "Referencia", "Odkaz"),
    ("KmSpalteWo", "Where", "Wo", "位置", "場所", "Dónde", "Kde"),
    ("KmArtNameFehlt", "name no longer in the map", "Name nicht mehr in der Karte", "名称已不在地图中", "名前がマップにありません",
     "el nombre ya no está en el mapa", "název už v mapě není"),
    ("KmArtModellFehlt", "brush model missing", "Brush-Modell fehlt", "画刷模型缺失", "ブラシモデルがありません", "falta el modelo de brush",
     "brush model chybí"),
    ("KmArtModellVerschoben", "brush model is other geometry now", "Brush-Modell ist jetzt andere Geometrie", "画刷模型现在是另一块几何体",
     "ブラシモデルが別のジオメトリになりました", "el modelo de brush ahora es otra geometría", "brush model je teď jiná geometrie"),
    ("KmArtModellAngepasst", "brush model renumbered to %s (adjusted)", "Brush-Modell neu nummeriert zu %s (angepasst)",
     "画刷模型已重新编号为 %s(已调整)", "ブラシモデルの番号が %s に変わりました (調整済み)", "modelo de brush renumerado a %s (ajustado)",
     "brush model přečíslován na %s (upraveno)"),
    ("KmArtEntityWeg", "removed in the map, kept from the mission", "in der Karte entfernt, aus der Mission behalten",
     "已在地图中移除,按任务保留", "マップでは削除済み、ミッションの内容を保持", "eliminada en el mapa, se conserva la de la misión",
     "v mapě odstraněna, ponechána z mise"),
    ("KmZusammenfassung", "%d unchanged, %d changed, %d own added, %d own already in the map, %d removed.",
     "%d unverändert, %d geändert, %d eigene angehängt, %d eigene schon in der Karte, %d entfernt.",
     "%d 个未更改,%d 个已更改,%d 个自有实体已添加,%d 个自有实体已在地图中,%d 个已移除。",
     "変更なし %d、変更 %d、独自を追加 %d、独自が既にマップにある %d、削除 %d。",
     "%d sin cambios, %d cambiadas, %d propias añadidas, %d propias ya en el mapa, %d eliminadas.",
     "%d beze změny, %d změněno, %d vlastních přidáno, %d vlastních už v mapě, %d odstraněno."),
    ("KmRegel",
     "Same entity: worldspawn; else same classname and name (targetname, script_targetname, NPC_targetname); brush entities without a name: "
     "same brush bounds; else same classname and origin (1 unit).",
     "Dieselbe Entity: worldspawn; sonst gleicher classname und Name (targetname, script_targetname, NPC_targetname); Brush-Entities ohne Namen: "
     "gleiche Brush-Grenzen; sonst gleicher classname und Ort (1 Einheit).",
     "同一实体:worldspawn;否则 classname 与名称相同(targetname、script_targetname、NPC_targetname);无名称的画刷实体:"
     "画刷边界相同;否则 classname 与位置相同(1 个单位)。",
     "同じエンティティ: worldspawn。それ以外は classname と名前 (targetname, script_targetname, NPC_targetname) が同じ。名前のないブラシエンティティ: "
     "ブラシの範囲が同じ。それ以外は classname と位置 (1 単位) が同じ。",
     "Misma entidad: worldspawn; si no, mismo classname y nombre (targetname, script_targetname, NPC_targetname); entidades de brush sin nombre: "
     "mismos límites de brush; si no, mismo classname y origen (1 unidad).",
     "Stejná entita: worldspawn; jinak stejný classname a název (targetname, script_targetname, NPC_targetname); brush entity bez názvu: "
     "stejné hranice brushe; jinak stejný classname a počátek (1 jednotka)."),
    ("KmSchliessen", "Close", "Schließen", "关闭", "閉じる", "Cerrar", "Zavřít"),
    ("KmLumpsVerworfen", "The unsaved shader and surface edits of %s belonged to the old build and are discarded.",
     "Die ungespeicherten Shader- und Flächenänderungen von %s gehörten zum alten Bau und sind verworfen.",
     "%s 未保存的着色器和表面修改属于旧的编译结果,已丢弃。", "%s の未保存のシェーダーとサーフェスの変更は古いビルドのものなので破棄しました。",
     "Los cambios sin guardar de shaders y superficies de %s eran de la compilación anterior y se descartan.",
     "Neuložené změny shaderů a ploch %s patřily ke starému sestavení a jsou zahozeny."),
    ("KmBauOhneBsp", "The build did not produce %s - the Mission tab keeps the old map.",
     "Der Bau hat %s nicht erzeugt - der Mission-Tab behält die alte Karte.", "编译未生成 %s——任务选项卡保留旧地图。",
     "ビルドで %s が作成されませんでした - ミッションタブは古いマップのままです。",
     "La compilación no generó %s; la pestaña Misión conserva el mapa anterior.", "Sestavení nevytvořilo %s - karta Mise ponechá starou mapu."),
    ("KmArbeitsordner", "The mission has no own map file yet; the Mission tab uses the built %s.",
     "Die Mission hat noch keine eigene Kartendatei; der Mission-Tab nimmt die gebaute %s.",
     "该任务还没有自己的地图文件;任务选项卡使用编译出的 %s。", "このミッションにはまだ独自のマップファイルがありません。ミッションタブはビルドした %s を使います。",
     "La misión aún no tiene archivo de mapa propio; la pestaña Misión usa el %s compilado.",
     "Mise zatím nemá vlastní soubor mapy; karta Mise použije sestavený %s."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
