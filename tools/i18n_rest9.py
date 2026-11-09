# -*- coding: utf-8 -*-
"""Oberflaechentexte von REST9 (09.11.: die Update-Leiste oben, wie im Programm G2C).

Sobald ein neueres Release gefunden ist, steht unter der Menueleiste eine Leiste mit Jetzt aktualisieren, Was ist neu?,
Spaeter, Diese Fassung ueberspringen und x (gui/update_win32.cpp, zeichneLeiste).
Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("UpdBarAvail", "New update %s available (installed: %s)", "Neues Update %s verfügbar (installiert: %s)",
     "有新的更新 %s(已安装:%s)", "新しいアップデート %s があります(インストール済み: %s)",
     "Nueva actualización %s disponible (instalada: %s)", "Nová aktualizace %s je k dispozici (nainstalováno: %s)"),
    ("UpdBarNow", "Update now", "Jetzt aktualisieren", "立即更新", "今すぐ更新", "Actualizar ahora", "Aktualizovat nyní"),
    ("UpdBarNowHint",
     "Downloads %s, checks it against SHA256SUMS.txt and replaces the program files. Your settings, scripts and maps stay.",
     "Lädt %s, prüft es gegen SHA256SUMS.txt und ersetzt die Programmdateien. Deine Einstellungen, Skripte und Karten bleiben.",
     "下载 %s,用 SHA256SUMS.txt 校验后替换程序文件。你的设置、脚本和地图保持不变。",
     "%s をダウンロードし、SHA256SUMS.txt で確認してからプログラムファイルを置き換えます。設定、スクリプト、マップはそのままです。",
     "Descarga %s, lo comprueba con SHA256SUMS.txt y reemplaza los archivos del programa. Tus ajustes, scripts y mapas se conservan.",
     "Stáhne %s, ověří ji podle SHA256SUMS.txt a nahradí soubory programu. Vaše nastavení, skripty a mapy zůstanou."),
    ("UpdBarWhatsNew", "What's new?", "Was ist neu?", "有什么新内容?", "新機能は?", "¿Qué hay de nuevo?", "Co je nového?"),
    ("UpdBarWhatsNewHint", "Opens the update window with the notes of this release and the changelog.",
     "Öffnet das Update-Fenster mit den Notizen dieses Releases und der Änderungsliste.",
     "打开更新窗口,显示此版本的说明和更新日志。",
     "このリリースのノートと変更履歴を表示するアップデートウィンドウを開きます。",
     "Abre la ventana de actualización con las notas de esta versión y el registro de cambios.",
     "Otevře okno aktualizace s poznámkami k tomuto vydání a seznamem změn."),
    ("UpdBarLaterHint", "Hides this bar until the next start. The note in the status bar stays.",
     "Blendet diese Leiste bis zum nächsten Start aus. Der Hinweis in der Statuszeile bleibt.",
     "在下次启动前隐藏此栏。状态栏中的提示会保留。",
     "次の起動までこのバーを隠します。ステータスバーの通知は残ります。",
     "Oculta esta barra hasta el próximo inicio. El aviso de la barra de estado se mantiene.",
     "Skryje tento pruh do příštího spuštění. Upozornění ve stavovém řádku zůstane."),
    ("UpdBarSkip", "Skip this version", "Diese Fassung überspringen", "跳过此版本", "このバージョンをスキップ", "Omitir esta versión",
     "Přeskočit tuto verzi"),
    ("UpdBarSkipHint", "Don't show this bar for %s again, not even after a restart. A newer version shows it again.",
     "Diese Leiste für %s nicht mehr zeigen, auch nicht nach einem Neustart. Eine neuere Fassung zeigt sie wieder.",
     "不再为 %s 显示此栏,重启后也不显示。更新的版本会再次显示。",
     "%s についてはこのバーを再起動後も表示しません。より新しいバージョンでは再び表示されます。",
     "No volver a mostrar esta barra para %s, ni siquiera tras reiniciar. Una versión más nueva la vuelve a mostrar.",
     "Tento pruh pro %s už nezobrazovat, ani po restartu. Novější verze jej zobrazí znovu."),
    ("UpdBarCloseHint", "Closes the bar until the next start (like Later).",
     "Schließt die Leiste bis zum nächsten Start (wie Später).",
     "关闭此栏直到下次启动(同“稍后”)。",
     "次の起動までバーを閉じます(「後で」と同じ)。",
     "Cierra la barra hasta el próximo inicio (como Más tarde).",
     "Zavře pruh do příštího spuštění (jako Později)."),
    ("UpdBarLoading", "Downloading update %s...", "Lade Update %s...", "正在下载更新 %s...", "アップデート %s をダウンロード中...",
     "Descargando la actualización %s...", "Stahuji aktualizaci %s..."),
    ("UpdBarDone", "Update %s is installed. Restart to use it.", "Update %s ist installiert. Neu starten, um es zu benutzen.",
     "更新 %s 已安装。重新启动后即可使用。", "アップデート %s をインストールしました。再起動すると使えます。",
     "La actualización %s está instalada. Reinicia para usarla.", "Aktualizace %s je nainstalována. Pro použití restartujte."),
    ("UpdBarRestartHint", "Closes JK Studio and starts the new version. Unsaved tabs are asked about first.",
     "Schließt JK Studio und startet die neue Fassung. Nach ungesicherten Reitern wird vorher gefragt.",
     "关闭 JK Studio 并启动新版本。未保存的标签页会先询问。",
     "JK Studio を閉じて新しいバージョンを起動します。未保存のタブは先に確認されます。",
     "Cierra JK Studio e inicia la nueva versión. Antes se pregunta por las pestañas sin guardar.",
     "Zavře JK Studio a spustí novou verzi. Na neuložené karty se nejprve zeptá."),
    ("UpdBarError", "Update %s could not be installed: %s", "Update %s ließ sich nicht installieren: %s",
     "无法安装更新 %s:%s", "アップデート %s をインストールできませんでした: %s",
     "No se pudo instalar la actualización %s: %s", "Aktualizaci %s se nepodařilo nainstalovat: %s"),
    ("UpdBarRetry", "Try again", "Erneut versuchen", "重试", "再試行", "Reintentar", "Zkusit znovu"),
    ("UpdBarRetryHint", "Downloads and installs %s again.",
     "Lädt und installiert %s noch einmal.",
     "重新下载并安装 %s。",
     "%s をもう一度ダウンロードしてインストールします。",
     "Descarga e instala %s de nuevo.",
     "Znovu stáhne a nainstaluje %s."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
