"""REST9 (shank 09.11.): die Update-Leiste oben, wie im Programm G2C.

Haengt einen Absatz und das Bild h_update_leiste an die Seite einstellungen (inhalt_referenz.py), Abschnitt "Updates und
Aenderungsliste", direkt nach dem ersten Absatz. Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in
inhalt_rest8.py.
"""

X = {
    "leiste": (
        "**The update bar:** as soon as a newer version is found - at startup or with **{{UpdMenu}}** - a coloured bar appears right "
        "under the menu bar, above all tabs: **{{UpdBarNow}}** downloads and installs it (with a progress bar in the bar, then "
        "**{{UpdRestart}}**; if something goes wrong, the bar says what and offers **{{UpdBarRetry}}**), **{{UpdBarWhatsNew}}** opens the "
        "notes of the release, **{{UpdLater}}** or **x** hide the bar until the next start (the note in the status bar stays), and "
        "**{{UpdBarSkip}}** never shows it again for this version - a newer one shows it again. The work area moves down while the bar is "
        "there and back up when it closes. A failed check (no connection) does not show the bar.",
        "**Die Update-Leiste:** sobald eine neuere Fassung gefunden ist - beim Start oder mit **{{UpdMenu}}** -, erscheint direkt unter "
        "der Menüleiste, über allen Reitern, eine farbige Leiste: **{{UpdBarNow}}** lädt und installiert sie (mit Fortschrittsbalken in der "
        "Leiste, danach **{{UpdRestart}}**; geht etwas schief, nennt die Leiste den Grund und bietet **{{UpdBarRetry}}**), "
        "**{{UpdBarWhatsNew}}** öffnet die Notizen des Releases, **{{UpdLater}}** oder **x** blenden die Leiste bis zum nächsten Start aus "
        "(der Hinweis in der Statuszeile bleibt), und **{{UpdBarSkip}}** zeigt sie für diese Fassung nie wieder - eine neuere zeigt sie "
        "wieder. Der Arbeitsbereich rückt nach unten, solange die Leiste steht, und nach dem Schließen wieder hoch. Ein gescheitertes "
        "Nachsehen (keine Verbindung) zeigt keine Leiste.",
        "**更新栏:**一旦发现新版本(启动时或通过 **{{UpdMenu}}**),菜单栏正下方、所有标签页上方就会出现一条彩色栏:**{{UpdBarNow}}** 下载并安装"
        "(栏内显示进度条,之后显示 **{{UpdRestart}}**;出错时栏会说明原因并提供 **{{UpdBarRetry}}**),**{{UpdBarWhatsNew}}** 打开该版本的说明,"
        "**{{UpdLater}}** 或 **x** 在下次启动前隐藏此栏(状态栏中的提示保留),**{{UpdBarSkip}}** 对此版本不再显示——更新的版本会再次显示。"
        "此栏显示时工作区下移,关闭后恢复。检查失败(无连接)时不显示此栏。",
        "**アップデートバー:**新しいバージョンが見つかると(起動時または **{{UpdMenu}}** で)、メニューバーのすぐ下、すべてのタブの上に色付きのバーが"
        "表示されます。**{{UpdBarNow}}** でダウンロードしてインストールし(バー内に進行状況バー、その後 **{{UpdRestart}}**。問題があればバーが理由を示し "
        "**{{UpdBarRetry}}** を提示します)、**{{UpdBarWhatsNew}}** でリリースノートを開き、**{{UpdLater}}** または **x** で次の起動までバーを隠し"
        "(ステータスバーの通知は残ります)、**{{UpdBarSkip}}** でこのバージョンについては二度と表示しません。より新しいバージョンでは再び表示されます。"
        "バーがある間は作業領域が下にずれ、閉じると元に戻ります。確認に失敗した場合(接続なし)はバーを表示しません。",
        "**La barra de actualización:** en cuanto se encuentra una versión más nueva (al iniciar o con **{{UpdMenu}}**), aparece justo "
        "debajo de la barra de menús, encima de todas las pestañas, una barra de color: **{{UpdBarNow}}** la descarga e instala (con una "
        "barra de progreso en la barra y después **{{UpdRestart}}**; si algo falla, la barra dice qué y ofrece **{{UpdBarRetry}}**), "
        "**{{UpdBarWhatsNew}}** abre las notas de la versión, **{{UpdLater}}** o **x** ocultan la barra hasta el próximo inicio (el aviso de "
        "la barra de estado se mantiene) y **{{UpdBarSkip}}** no la vuelve a mostrar para esta versión; una más nueva la muestra otra vez. "
        "El área de trabajo baja mientras la barra está visible y vuelve a subir al cerrarla. Una comprobación fallida (sin conexión) no "
        "muestra la barra.",
        "**Pruh aktualizace:** jakmile se najde novější verze (při spuštění nebo přes **{{UpdMenu}}**), objeví se přímo pod nabídkovým "
        "řádkem, nad všemi kartami, barevný pruh: **{{UpdBarNow}}** ji stáhne a nainstaluje (s ukazatelem průběhu v pruhu, potom "
        "**{{UpdRestart}}**; když se něco nepovede, pruh řekne co a nabídne **{{UpdBarRetry}}**), **{{UpdBarWhatsNew}}** otevře poznámky "
        "k vydání, **{{UpdLater}}** nebo **x** skryjí pruh do příštího spuštění (upozornění ve stavovém řádku zůstane) a **{{UpdBarSkip}}** "
        "jej pro tuto verzi už nikdy nezobrazí - novější verze jej zobrazí znovu. Pracovní plocha se posune dolů, dokud je pruh vidět, a po "
        "zavření zase nahoru. Neúspěšná kontrola (bez připojení) pruh nezobrazí."),
    "leiste_bild": (
        "The update bar under the menu bar: a newer version is available.",
        "Die Update-Leiste unter der Menüleiste: eine neuere Fassung ist verfügbar.",
        "菜单栏下方的更新栏:有新版本可用。",
        "メニューバーの下のアップデートバー: 新しいバージョンがあります。",
        "La barra de actualización bajo la barra de menús: hay una versión más nueva disponible.",
        "Pruh aktualizace pod nabídkovým řádkem: je k dispozici novější verze."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def einhaengen(einstellungen):
    """Absatz und Bild nach dem ersten Absatz von "Updates und Aenderungsliste" (einstellungen)."""
    b = einstellungen["bloecke"]
    i = next(n for n, x in enumerate(b) if x[0] == "h2" and x[1] == "updates")
    b.insert(i + 2, ("p", _t("leiste")))
    b.insert(i + 3, ("img", "h_update_leiste", _t("leiste_bild"), "breit"))
