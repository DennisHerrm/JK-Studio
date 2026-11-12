# -*- coding: utf-8 -*-
"""Oberflaechentexte von FIX612 (12.11.: Funde der Code-Pruefung seit rc611).

R3: "Bauen fortsetzen" nimmt keinen Befehl und keinen Pfad mehr aus dem Zwischenstand neben der Karte - die Gruende, aus
denen "Karte wie angehalten" nicht geht, der Hinweis auf geaenderte Schalter und auf eine verschobene Karte
(gui/app_karte_bau.cpp). R6: die Fehler beim Lesen eines ModView-Skripts in der Sprache der Oberflaeche
(gui/app_modelltab_pruefen.cpp, gui/app.cpp).
Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    # --- R3: Bauen fortsetzen ---
    ("KeBpAltBefehle", "These commands will run (Build menu entry \"%s\"):", "Diese Befehle laufen (Eintrag „%s“ des Build-Menüs):",
     "将运行以下命令(Build 菜单条目“%s”):", "次のコマンドを実行します(Build メニューのエントリー「%s」):",
     "Se ejecutarán estos comandos (entrada «%s» del menú Build):", "Spustí se tyto příkazy (položka „%s“ nabídky Build):"),
    ("KeBpAltNichtMoeglich", "\"Map as paused\" is not available: %s", "„Karte wie angehalten“ geht nicht: %s",
     "无法使用“暂停时的地图”:%s", "「一時停止時のマップ」は使えません: %s", "«Mapa como se pausó» no está disponible: %s",
     "„Mapa jako při pozastavení“ není k dispozici: %s"),
    ("KeBpGrundSchritte", "The Build menu entry \"%s\" has other steps today (%s) than when the build was paused (%s).",
     "Der Eintrag „%s“ des Build-Menüs hat heute andere Schritte (%s) als beim Anhalten (%s).",
     "Build 菜单条目“%s”现在的步骤(%s)与暂停时(%s)不同。",
     "Build メニューのエントリー「%s」の現在のステップ(%s)が一時停止時(%s)と違います。",
     "La entrada «%s» del menú Build tiene hoy otros pasos (%s) que cuando se pausó la compilación (%s).",
     "Položka „%s“ nabídky Build má dnes jiné kroky (%s) než při pozastavení (%s)."),
    ("KeBpGrundProgramm", "Step %d of the Build menu entry \"%s\" starts %s. \"Map as paused\" only starts the compiler of JK Studio (%s).",
     "Schritt %d des Eintrags „%s“ startet %s. „Karte wie angehalten“ startet nur den Compiler von JK Studio (%s).",
     "第 %d 步(Build 菜单条目“%s”)会启动 %s。“暂停时的地图”只启动 JK Studio 的编译器(%s)。",
     "ステップ %d(Build メニューのエントリー「%s」)は %s を起動します。「一時停止時のマップ」は JK Studio のコンパイラ(%s)だけを起動します。",
     "El paso %d de la entrada «%s» del menú Build inicia %s. «Mapa como se pausó» solo inicia el compilador de JK Studio (%s).",
     "Krok %d položky „%s“ nabídky Build spouští %s. „Mapa jako při pozastavení“ spouští jen kompilátor JK Studio (%s)."),
    ("KeBpGrundStandFremd",
     "The saved state names a program that is not the compiler of JK Studio (%s). It was not written by JK Studio or was changed afterwards. Nothing from it is started.",
     "Der Zwischenstand nennt ein Programm, das nicht der Compiler von JK Studio ist (%s). Er wurde nicht von JK Studio geschrieben oder danach verändert. Daraus wird nichts gestartet.",
     "保存的状态中写着一个不是 JK Studio 编译器的程序(%s)。它不是由 JK Studio 写入的,或者之后被改动过。不会启动其中的任何内容。",
     "保存した状態に JK Studio のコンパイラではないプログラム(%s)が書かれています。JK Studio が書いたものではないか、あとで変更されています。そこからは何も起動しません。",
     "El estado guardado nombra un programa que no es el compilador de JK Studio (%s). No lo escribió JK Studio o se modificó después. No se inicia nada de él.",
     "Uložený stav uvádí program, který není kompilátorem JK Studio (%s). Nezapsalo ho JK Studio, nebo byl později změněn. Nic se z něj nespustí."),
    ("KeBpAltRat", "Resume with \"Current map\" or discard the saved state.", "Setze mit „Aktuelle Karte“ fort oder verwirf den Zwischenstand.",
     "请用“当前地图”继续,或丢弃保存的状态。", "「現在のマップ」で再開するか、保存した状態を破棄してください。",
     "Reanuda con «Mapa actual» o descarta el estado guardado.", "Pokračujte volbou „Aktuální mapa“, nebo uložený stav zahoďte."),
    ("KeBpAltSchalter", "Step %d (%s) runs with the switches the Build menu has today; they differ from those of the paused build.",
     "Schritt %d (%s) läuft mit den Schaltern, die das Build-Menü heute hat; sie weichen von denen des angehaltenen Baus ab.",
     "第 %d 步(%s)使用 Build 菜单现在的开关运行;它们与被暂停的编译不同。",
     "ステップ %d(%s)は Build メニューの現在のスイッチで実行します。一時停止したビルドのものとは違います。",
     "El paso %d (%s) se ejecuta con las opciones que el menú Build tiene hoy; difieren de las de la compilación pausada.",
     "Krok %d (%s) běží s přepínači, které má nabídka Build dnes; liší se od přepínačů pozastaveného sestavení."),
    ("KeBpVerschoben", "The map has been moved since the build was paused (it was in %s). The saved state is used at the new place.",
     "Die Karte wurde seit dem Anhalten verschoben (sie lag unter %s). Der Zwischenstand gilt am neuen Ort.",
     "暂停之后地图已被移动(原位置为 %s)。保存的状态在新位置使用。",
     "一時停止のあとでマップが移動されています(元は %s)。保存した状態は新しい場所で使います。",
     "El mapa se ha movido desde que se pausó la compilación (estaba en %s). El estado guardado se usa en el lugar nuevo.",
     "Mapa byla od pozastavení přesunuta (byla v %s). Uložený stav se použije na novém místě."),
    ("KeBpVerwerfenFehler", "The saved state could not be deleted completely: %s", "Der Zwischenstand ließ sich nicht ganz löschen: %s",
     "无法完全删除保存的状态:%s", "保存した状態を完全には削除できませんでした: %s", "El estado guardado no se pudo borrar por completo: %s",
     "Uložený stav se nepodařilo úplně smazat: %s"),
    # --- R6: ModView-Skript lesen ---
    ("PqSzeneNichtOffen", "the file cannot be opened", "die Datei lässt sich nicht öffnen", "无法打开文件", "ファイルを開けません",
     "no se puede abrir el archivo", "soubor nelze otevřít"),
    ("PqSzeneKlammer", "the braces do not match (line %d)", "die Klammern passen nicht zusammen (Zeile %d)", "花括号不匹配(第 %d 行)",
     "波かっこが対応していません(%d 行目)", "las llaves no coinciden (línea %d)", "složené závorky si neodpovídají (řádek %d)"),
    ("PqSzeneKeinBlock", "the block \"%s\" is missing", "der Block „%s“ fehlt", "缺少块“%s”", "ブロック「%s」がありません",
     "falta el bloque «%s»", "chybí blok „%s“"),
    ("PqSzeneKeinSchluessel", "the block \"%s\" has no \"%s\"", "im Block „%s“ fehlt „%s“", "块“%s”中缺少“%s”",
     "ブロック「%s」に「%s」がありません", "al bloque «%s» le falta «%s»", "v bloku „%s“ chybí „%s“"),
    ("PqSzeneEltern", "the block \"%s\" names the unknown parent \"%s\"", "der Block „%s“ nennt das unbekannte Elternmodell „%s“",
     "块“%s”引用了未知的父模型“%s”", "ブロック「%s」が未知の親「%s」を指しています", "el bloque «%s» nombra el padre desconocido «%s»",
     "blok „%s“ uvádí neznámého rodiče „%s“"),
    ("MbNichtInSpieldateien", "not found in the game files", "nicht in den Spieldateien gefunden", "在游戏文件中未找到",
     "ゲームファイルに見つかりません", "no se encuentra en los archivos del juego", "nenalezeno v souborech hry"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
