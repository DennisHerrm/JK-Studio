# -*- coding: utf-8 -*-
"""Oberflaechentexte von pfix (Funde der Code-Pruefung Modell-Block) fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("MbVerwerfenFrage",
     "The model %s in the Model tab has unsaved changes.\nDiscard them and open the other one?\n(No: back to the Model tab)",
     "Das Modell %s im Model-Tab hat ungespeicherte Änderungen.\nVerwerfen und das andere öffnen?\n(Nein: zurück zum Model-Tab)",
     "Model 标签中的模型 %s 有未保存的更改。\n放弃更改并打开另一个?\n(否: 返回 Model 标签)",
     "Model タブのモデル %s に未保存の変更があります。\n変更を破棄して別のモデルを開きますか?\n(いいえ: Model タブに戻る)",
     "El modelo %s de la pestaña Model tiene cambios sin guardar.\n¿Descartarlos y abrir el otro?\n(No: volver a la pestaña Model)",
     "Model %s na kartě Model má neuložené změny.\nZahodit je a otevřít jiný?\n(Ne: zpět na kartu Model)"),
    ("MbBeendenFrage",
     "The model %s in the Model tab has unsaved changes.\nExit anyway and discard them?\n(No: back to the Model tab)",
     "Das Modell %s im Model-Tab hat ungespeicherte Änderungen.\nTrotzdem beenden und sie verwerfen?\n(Nein: zurück zum Model-Tab)",
     "Model 标签中的模型 %s 有未保存的更改。\n仍然退出并放弃更改?\n(否: 返回 Model 标签)",
     "Model タブのモデル %s に未保存の変更があります。\n変更を破棄して終了しますか?\n(いいえ: Model タブに戻る)",
     "El modelo %s de la pestaña Model tiene cambios sin guardar.\n¿Salir de todos modos y descartarlos?\n(No: volver a la pestaña Model)",
     "Model %s na kartě Model má neuložené změny.\nPřesto ukončit a zahodit je?\n(Ne: zpět na kartu Model)"),
    ("FaNameUngueltig",
     "No quotation marks or line breaks in names - ICARUS cannot write them: %s",
     "Keine Anführungszeichen oder Zeilenumbrüche in Namen - ICARUS kann sie nicht schreiben: %s",
     "名称中不能有引号或换行 - ICARUS 无法写入: %s",
     "名前に引用符や改行は使えません - ICARUS では書けません: %s",
     "Sin comillas ni saltos de línea en los nombres - ICARUS no puede escribirlos: %s",
     "Žádné uvozovky ani zalomení řádku v názvech - ICARUS je neumí zapsat: %s"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
