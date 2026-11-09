# -*- coding: utf-8 -*-
"""Oberflaechentexte von REST10 (09.11.: die Update-Leiste wie im Programm G2C ergaenzt).

Beim Laden [Abbrechen], nach dem Laden auch [Spaeter], die Suche von Hand in der Leiste ("Suche ...", "aktuell",
Fehler mit [Releases-Seite] und [Schliessen]) - gui/update_win32.cpp, zeichneLeiste.
Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("UpdBarCancel", "Cancel", "Abbrechen", "取消", "キャンセル", "Cancelar", "Zrušit"),
    ("UpdBarCancelHint", "Stops the download. Nothing is installed; the bar shows the available update again.",
     "Bricht das Laden ab. Nichts wird installiert, die Leiste zeigt wieder das verfügbare Update.",
     "停止下载。不会安装任何内容,此栏重新显示可用的更新。",
     "ダウンロードを中止します。何もインストールされず、バーは利用可能なアップデートの表示に戻ります。",
     "Detiene la descarga. No se instala nada y la barra vuelve a mostrar la actualización disponible.",
     "Zastaví stahování. Nic se nenainstaluje a pruh znovu ukáže dostupnou aktualizaci."),
    ("UpdBarDoneLaterHint", "Hides the bar. The update takes effect at the next start; the note in the status bar stays.",
     "Blendet die Leiste aus. Das Update wird beim nächsten Start wirksam; der Hinweis in der Statuszeile bleibt.",
     "隐藏此栏。更新将在下次启动时生效;状态栏中的提示会保留。",
     "バーを隠します。アップデートは次回の起動で有効になります。ステータスバーの通知は残ります。",
     "Oculta la barra. La actualización se aplica en el próximo inicio; el aviso de la barra de estado se mantiene.",
     "Skryje pruh. Aktualizace se projeví při příštím spuštění; upozornění ve stavovém řádku zůstane."),
    ("UpdBarChecking", "Checking for updates...", "Suche nach Updates...", "正在检查更新...", "アップデートを確認しています...",
     "Buscando actualizaciones...", "Hledám aktualizace..."),
    ("UpdBarCurrent", "JK Studio is up to date (%s).", "JK Studio ist aktuell (%s).", "JK Studio 已是最新版本(%s)。",
     "JK Studio は最新です(%s)。", "JK Studio está actualizado (%s).", "JK Studio je aktuální (%s)."),
    ("UpdBarCheckFailed", "Could not check for updates: %s", "Suche nach Updates fehlgeschlagen: %s", "无法检查更新:%s",
     "アップデートを確認できませんでした: %s", "No se pudieron buscar actualizaciones: %s",
     "Aktualizace se nepodařilo zkontrolovat: %s"),
    ("UpdBarReleases", "Releases page", "Releases-Seite", "发布页面", "リリースページ", "Página de versiones", "Stránka vydání"),
    ("UpdBarReleasesHint", "Opens the releases of JK Studio on GitHub in your browser - to download a version by hand.",
     "Öffnet die Releases von JK Studio auf GitHub im Browser - zum Herunterladen von Hand.",
     "在浏览器中打开 GitHub 上 JK Studio 的发布页面,可手动下载。",
     "ブラウザーで GitHub の JK Studio のリリースを開きます。手動でダウンロードできます。",
     "Abre las versiones de JK Studio en GitHub en el navegador, para descargarlas a mano.",
     "Otevře vydání JK Studia na GitHubu v prohlížeči - ke stažení ručně."),
    ("UpdBarClose", "Close", "Schließen", "关闭", "閉じる", "Cerrar", "Zavřít"),
    ("UpdBarCloseResultHint", "Closes this message.", "Schließt diese Meldung.", "关闭此消息。", "このメッセージを閉じます。",
     "Cierra este mensaje.", "Zavře tuto zprávu."),
    ("UpdBarCloseCheckHint", "Closes the bar. The search goes on quietly; a new version still shows the bar.",
     "Schließt die Leiste. Die Suche läuft still weiter; eine neue Version zeigt die Leiste trotzdem.",
     "关闭此栏。检查会在后台继续;发现新版本时仍会显示此栏。",
     "バーを閉じます。確認は裏で続き、新しいバージョンが見つかればバーが表示されます。",
     "Cierra la barra. La búsqueda sigue en silencio; una versión nueva vuelve a mostrar la barra.",
     "Zavře pruh. Hledání tiše pokračuje; nová verze pruh přesto zobrazí."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
