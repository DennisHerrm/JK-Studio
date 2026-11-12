# -*- coding: utf-8 -*-
"""Oberflaechentexte von FIX612B (Funde der Code-Pruefung P2: Gewichte, Malen, Auto-Skin) fuer tools/gen_i18n.py und die
weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    # P2-2: Speichern schreibt, was die Ansicht zeigt - auch mit offenen Mallisten
    ("FxSpMalListen",
     "Painted weights are in the file as the game reads them (4 per vertex, sum 1); the paint lists stay open.",
     "Gemalte Gewichte stehen in der Datei, wie das Spiel sie liest (4 je Ecke, Summe 1); die Mallisten bleiben offen.",
     "绘制的权重已按游戏读取的方式写入文件 (每个顶点 4 个, 总和为 1); 绘制列表保持打开。",
     "ペイントしたウェイトはゲームが読む形でファイルに入りました (頂点ごとに 4 つ、合計 1)。ペイントリストは開いたままです。",
     "Los pesos pintados están en el archivo como los lee el juego (4 por vértice, suma 1); las listas de pintura siguen abiertas.",
     "Namalované váhy jsou v souboru tak, jak je čte hra (4 na vrchol, součet 1); seznamy malování zůstávají otevřené."),
    ("FxUebernehmenSpeichern",
     "Saving writes this result to the file even without Apply.",
     "Speichern schreibt dieses Ergebnis auch ohne Übernehmen in die Datei.",
     "即使不点“应用”, 保存也会把这个结果写入文件。",
     "「適用」しなくても、保存するとこの結果がファイルに書き込まれます。",
     "Guardar escribe este resultado en el archivo aunque no se pulse Aplicar.",
     "Uložení zapíše tento výsledek do souboru i bez tlačítka Použít."),
    # P2-1: Auto-Skin "Nur Auswahl" mit "Spiegeln" (Entscheidung A16)
    ("FxAsSpiegelnAuswahl",
     "With Selected surfaces only, it writes only into the selected surfaces; to mirror one arm onto the other, select both.",
     "Mit „Nur ausgewählte Flächen“ schreibt es nur in die gewählten Flächen; um einen Arm auf den anderen zu spiegeln, beide wählen.",
     "勾选“仅所选表面”时只写入所选的面; 要把一只手臂镜像到另一只, 请两只都选中。",
     "「選択したサーフェスのみ」では選択したサーフェスにだけ書き込みます。片方の腕をもう片方にミラーするには両方を選択します。",
     "Con «Solo las superficies seleccionadas» escribe solo en las superficies seleccionadas; para reflejar un brazo en el otro, selecciona ambos.",
     "S volbou „Jen vybrané povrchy“ zapisuje jen do vybraných ploch; chcete-li zrcadlit jednu paži na druhou, vyberte obě."),
    # P2-9 / V3: Gewichte und Auswahl gelten der gezeigten Stufe (Entscheidungen A14, A15)
    ("FxLodGilt",
     "Painting, the weight commands and the selection of vertices apply to the level shown; each level keeps its own selection. During a stroke or drag the level stays.",
     "Malen, die Gewichtsbefehle und die Auswahl von Ecken gelten der gezeigten Stufe; jede Stufe behält ihre eigene Auswahl. Während eines Strichs oder Zugs bleibt die Stufe stehen.",
     "绘制、权重命令和顶点选择作用于显示的级别; 每个级别保留自己的选择。笔划或拖动期间级别保持不变。",
     "ペイント、ウェイトのコマンド、頂点の選択は表示中のレベルに適用され、レベルごとに選択が保たれます。ストロークやドラッグの間はレベルが変わりません。",
     "Pintar, los comandos de pesos y la selección de vértices se aplican al nivel mostrado; cada nivel conserva su propia selección. Durante un trazo o arrastre el nivel se mantiene.",
     "Malování, příkazy vah a výběr vrcholů platí pro zobrazenou úroveň; každá úroveň si drží vlastní výběr. Během tahu nebo tažení úroveň zůstává."),
    # P2-10: Strg+Z im Strich (Entscheidung A17)
    ("FxStrichStrgZ",
     "Ctrl+Z during a stroke, gradient or drag cancels it and takes it back; Ctrl+Y brings it back.",
     "Strg+Z während eines Strichs, Verlaufs oder Zugs bricht ihn ab und nimmt ihn zurück; Strg+Y bringt ihn wieder.",
     "在笔划、渐变或拖动过程中按 Ctrl+Z 会取消并撤销它; Ctrl+Y 可恢复。",
     "ストローク、グラデーション、ドラッグの最中に Ctrl+Z を押すと中止して元に戻します。Ctrl+Y でやり直せます。",
     "Ctrl+Z durante un trazo, degradado o arrastre lo cancela y lo deshace; Ctrl+Y lo rehace.",
     "Ctrl+Z během tahu, přechodu nebo tažení ho zruší a vrátí; Ctrl+Y ho obnoví."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
