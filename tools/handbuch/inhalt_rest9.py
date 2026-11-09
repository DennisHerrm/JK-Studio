"""REST9 (shank 09.11.): die Update-Leiste oben, wie im Programm G2C (REST10: Abbrechen, Spaeter, Suche von Hand, Symbole).

Haengt einen Absatz und das Bild h_update_leiste an die Seite einstellungen (inhalt_referenz.py), Abschnitt "Updates und
Aenderungsliste", direkt nach dem ersten Absatz. Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in
inhalt_rest8.py.
"""

X = {
    "leiste": (
        "**The update bar:** as soon as a newer version is found - at startup or with **{{UpdMenu}}** - a coloured bar appears right "
        "under the menu bar, above all tabs: **{{UpdBarNow}}** downloads and installs it (the bar shows how many MB have arrived; "
        "**{{UpdBarCancel}}** stops the download and installs nothing), then **{{UpdRestart}}** or **{{UpdLater}}** (the update takes effect "
        "at the next start); if something goes wrong, the bar says what and offers **{{UpdBarRetry}}**. **{{UpdBarWhatsNew}}** opens the "
        "notes of the release, **{{UpdLater}}** or **x** hide the bar until the next start (the note in the status bar stays), and "
        "**{{UpdBarSkip}}** never shows it again for this version - a newer one shows it again. When you look with **{{UpdMenu}}** yourself, "
        "the bar also shows the search, that JK Studio is up to date, or why the check failed (with **{{UpdBarReleases}}** and "
        "**{{UpdBarClose}}**); a symbol in front says which (i, tick, cross). The check at startup stays silent without a connection. The "
        "work area moves down while the bar is there and back up when it closes.",
        "**Die Update-Leiste:** sobald eine neuere Version gefunden ist - beim Start oder mit **{{UpdMenu}}** -, erscheint direkt unter "
        "der Menüleiste, über allen Reitern, eine farbige Leiste: **{{UpdBarNow}}** lädt und installiert sie (die Leiste zeigt, wie viele MB "
        "angekommen sind; **{{UpdBarCancel}}** bricht das Laden ab und installiert nichts), danach **{{UpdRestart}}** oder **{{UpdLater}}** "
        "(das Update wird beim nächsten Start wirksam); geht etwas schief, nennt die Leiste den Grund und bietet **{{UpdBarRetry}}**. "
        "**{{UpdBarWhatsNew}}** öffnet die Notizen des Releases, **{{UpdLater}}** oder **x** blenden die Leiste bis zum nächsten Start aus "
        "(der Hinweis in der Statuszeile bleibt), und **{{UpdBarSkip}}** zeigt sie für diese Version nie wieder - eine neuere zeigt sie "
        "wieder. Suchst du selbst mit **{{UpdMenu}}**, zeigt die Leiste auch die Suche, dass JK Studio aktuell ist, oder warum die Suche "
        "scheiterte (mit **{{UpdBarReleases}}** und **{{UpdBarClose}}**); ein Symbol davor sagt, was es ist (i, Haken, Kreuz). Die Suche "
        "beim Start bleibt ohne Verbindung still. Der Arbeitsbereich rückt nach unten, solange die Leiste steht, und nach dem Schließen "
        "wieder hoch.",
        "**更新栏:**一旦发现新版本(启动时或通过 **{{UpdMenu}}**),菜单栏正下方、所有标签页上方就会出现一条彩色栏:**{{UpdBarNow}}** 下载并安装"
        "(栏中显示已下载多少 MB;**{{UpdBarCancel}}** 停止下载,不安装任何内容),之后是 **{{UpdRestart}}** 或 **{{UpdLater}}**(更新在下次启动时生效);"
        "出错时栏会说明原因并提供 **{{UpdBarRetry}}**。**{{UpdBarWhatsNew}}** 打开该版本的说明,**{{UpdLater}}** 或 **x** 在下次启动前隐藏此栏"
        "(状态栏中的提示保留),**{{UpdBarSkip}}** 对此版本不再显示——更新的版本会再次显示。自己用 **{{UpdMenu}}** 检查时,栏还会显示正在检查、"
        "JK Studio 已是最新版本,或检查失败的原因(带 **{{UpdBarReleases}}** 和 **{{UpdBarClose}}**);前面的符号表明是哪一种(i、对勾、叉)。"
        "启动时的检查在没有网络时保持静默。此栏显示时工作区下移,关闭后恢复。",
        "**アップデートバー:**新しいバージョンが見つかると(起動時または **{{UpdMenu}}** で)、メニューバーのすぐ下、すべてのタブの上に色付きのバーが"
        "表示されます。**{{UpdBarNow}}** でダウンロードしてインストールし(バーに届いた MB 数が表示されます。**{{UpdBarCancel}}** でダウンロードを中止し、"
        "何もインストールしません)、その後 **{{UpdRestart}}** または **{{UpdLater}}**(アップデートは次回の起動で有効になります)。問題があればバーが"
        "理由を示し **{{UpdBarRetry}}** を提示します。**{{UpdBarWhatsNew}}** でリリースノートを開き、**{{UpdLater}}** または **x** で次の起動までバーを"
        "隠し(ステータスバーの通知は残ります)、**{{UpdBarSkip}}** でこのバージョンについては二度と表示しません。より新しいバージョンでは再び表示されます。"
        "自分で **{{UpdMenu}}** を使うと、確認中であること、JK Studio が最新であること、または確認に失敗した理由(**{{UpdBarReleases}}** と "
        "**{{UpdBarClose}}** 付き)もバーに表示されます。前の記号で種類がわかります(i、チェック、バツ)。起動時の確認は接続がなければ何も表示しません。"
        "バーがある間は作業領域が下にずれ、閉じると元に戻ります。",
        "**La barra de actualización:** en cuanto se encuentra una versión más nueva (al iniciar o con **{{UpdMenu}}**), aparece justo "
        "debajo de la barra de menús, encima de todas las pestañas, una barra de color: **{{UpdBarNow}}** la descarga e instala (la barra "
        "muestra cuántos MB han llegado; **{{UpdBarCancel}}** detiene la descarga y no instala nada) y después ofrece **{{UpdRestart}}** o "
        "**{{UpdLater}}** (la actualización se aplica en el próximo inicio); si algo falla, la barra dice qué y ofrece **{{UpdBarRetry}}**. "
        "**{{UpdBarWhatsNew}}** abre las notas de la versión, **{{UpdLater}}** o **x** ocultan la barra hasta el próximo inicio (el aviso de "
        "la barra de estado se mantiene) y **{{UpdBarSkip}}** no la vuelve a mostrar para esta versión; una más nueva la muestra otra vez. "
        "Si buscas tú mismo con **{{UpdMenu}}**, la barra también muestra la búsqueda, que JK Studio está actualizado o por qué falló "
        "(con **{{UpdBarReleases}}** y **{{UpdBarClose}}**); un símbolo delante indica cuál (i, marca, cruz). La comprobación al iniciar "
        "no dice nada si no hay conexión. El área de trabajo baja mientras la barra está visible y vuelve a subir al cerrarla.",
        "**Pruh aktualizace:** jakmile se najde novější verze (při spuštění nebo přes **{{UpdMenu}}**), objeví se přímo pod nabídkovým "
        "řádkem, nad všemi kartami, barevný pruh: **{{UpdBarNow}}** ji stáhne a nainstaluje (pruh ukazuje, kolik MB už dorazilo; "
        "**{{UpdBarCancel}}** stahování zastaví a nic nenainstaluje), potom nabídne **{{UpdRestart}}** nebo **{{UpdLater}}** (aktualizace se "
        "projeví při příštím spuštění); když se něco nepovede, pruh řekne co a nabídne **{{UpdBarRetry}}**. **{{UpdBarWhatsNew}}** otevře "
        "poznámky k vydání, **{{UpdLater}}** nebo **x** skryjí pruh do příštího spuštění (upozornění ve stavovém řádku zůstane) a "
        "**{{UpdBarSkip}}** jej pro tuto verzi už nikdy nezobrazí - novější verze jej zobrazí znovu. Když hledáte sami přes **{{UpdMenu}}**, "
        "pruh ukáže i hledání, že je JK Studio aktuální, nebo proč kontrola selhala (s **{{UpdBarReleases}}** a **{{UpdBarClose}}**); "
        "symbol vpředu říká, o co jde (i, fajfka, křížek). Kontrola při spuštění bez připojení nic neukáže. Pracovní plocha se posune dolů, "
        "dokud je pruh vidět, a po zavření zase nahoru."),
    "leiste_bild": (
        "The update bar under the menu bar: a newer version is available.",
        "Die Update-Leiste unter der Menüleiste: eine neuere Version ist verfügbar.",
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
