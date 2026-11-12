"""D3D12 (rc612): Abschnitt "Grafikschnittstelle" auf der Seite "Einstellungen und Updates" - die Wahl
Ansicht > Grafikschnittstelle (Direct3D 12 Vorgabe, Direct3D 11 Rueckfall), wann sie gilt und wann JK Studio von selbst
Direct3D 11 nimmt (kein Direct3D 12, Start scheitert, letzter Start abgestuerzt, Geraeteverlust).

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_rest4.py.
"""
from inhalt_basis import T

TITEL = ("Graphics API", "Grafikschnittstelle", "图形接口", "グラフィックス API", "Interfaz gráfica", "Grafické rozhraní")

WAHL = (
    "**{{MenuView}} → {{GrafikMenu}}** chooses how JK Studio talks to the graphics card. **{{GrafikD3d12}}** draws the "
    "interface and all 3D views; **{{GrafikD3d11}}** stays available. The choice takes "
    "effect at the next start; the tooltip names what is running now. JK Studio switches to Direct3D 11 by itself when Windows "
    "or the graphics card cannot do Direct3D 12, when starting Direct3D 12 fails, when the last start with Direct3D 12 did not "
    "finish, and when the graphics card is lost while "
    "working and no new Direct3D 12 device comes. Closing JK Studio, logging off or shutting Windows down never counts as an "
    "unfinished start. After an unfinished start the menu says so and offers **{{GrafikWiederVersuchen}}** (from the next "
    "start); after an update JK Studio tries Direct3D 12 once by itself.",
    "**{{MenuView}} → {{GrafikMenu}}** wählt, wie JK Studio mit der Grafikkarte spricht. **{{GrafikD3d12}}** zeichnet "
    "die Oberfläche und alle 3D-Ansichten; **{{GrafikD3d11}}** bleibt wählbar. Die Wahl "
    "gilt ab dem nächsten Start; der Tooltip nennt, was gerade läuft. JK Studio nimmt von selbst Direct3D 11, wenn Windows "
    "oder die Grafikkarte kein Direct3D 12 kann, wenn der Start von Direct3D 12 scheitert, wenn der letzte Start mit "
    "Direct3D 12 nicht fertig wurde und wenn die "
    "Grafikkarte während der Arbeit verloren geht und kein neues Direct3D-12-Gerät kommt. Beenden, Abmelden und "
    "Herunterfahren zählen nie als nicht fertiger Start. Nach einem nicht fertigen Start sagt es das Menü und bietet "
    "**{{GrafikWiederVersuchen}}** an (ab dem nächsten Start); nach einem Update versucht JK Studio Direct3D 12 einmal von selbst.",
    "**{{MenuView}} → {{GrafikMenu}}** 选择 JK Studio 与显卡通信的方式。**{{GrafikD3d12}}** 绘制界面和所有 3D 视图;"
    "**{{GrafikD3d11}}** 仍可选择。所选项在下次启动时生效;工具提示会显示当前正在运行的接口。在以下情况下 JK Studio "
    "会自动改用 Direct3D 11:Windows 或显卡不支持 Direct3D 12;Direct3D 12 启动失败;上次使用 Direct3D 12 的启动未能完成;"
    "工作中显卡丢失且无法获得新的 Direct3D 12 设备。关闭 JK Studio、注销或关闭 Windows 绝不算作未完成的启动。启动未完成后,菜单会说明并提供"
    "**{{GrafikWiederVersuchen}}**(从下次启动起);更新后 JK Studio 会自动尝试一次 Direct3D 12。",
    "**{{MenuView}} → {{GrafikMenu}}** で、JK Studio がグラフィックカードとやり取りする方法を選びます。**{{GrafikD3d12}}** が"
    "インターフェースとすべての 3D ビューを描画します。**{{GrafikD3d11}}** も引き続き選べます。選択は次回の"
    "起動から有効になり、ツールチップに現在動作中の方式が表示されます。次の場合、JK Studio は自動的に Direct3D 11 を使います:Windows "
    "またはグラフィックカードが Direct3D 12 に対応していない、Direct3D 12 の起動に失敗した、前回の Direct3D 12 での起動が完了しなかった、"
    "作業中にグラフィックカードが失われ新しい Direct3D 12 デバイスが得られない。JK Studio の終了、サインアウト、Windows の"
    "シャットダウンは、完了しなかった起動とは見なされません。起動が完了しなかった後は、メニューにそう表示され、"
    "**{{GrafikWiederVersuchen}}** を選べます(次回の起動から)。アップデート後は JK Studio が一度だけ自動で Direct3D 12 を試します。",
    "**{{MenuView}} → {{GrafikMenu}}** elige cómo habla JK Studio con la tarjeta gráfica. **{{GrafikD3d12}}** dibuja "
    "la interfaz y todas las vistas 3D; **{{GrafikD3d11}}** sigue disponible. "
    "La elección se aplica en el próximo inicio; la descripción emergente indica qué está en uso ahora. JK Studio pasa por sí "
    "solo a Direct3D 11 cuando Windows o la tarjeta gráfica no admiten Direct3D 12, cuando falla el inicio de Direct3D 12, "
    "cuando el último inicio con Direct3D 12 no terminó y cuando se pierde la tarjeta gráfica durante el trabajo y no llega "
    "un nuevo dispositivo Direct3D 12. Cerrar JK Studio, cerrar la sesión o apagar Windows nunca cuenta como un inicio sin "
    "terminar. Tras un inicio sin terminar, el menú lo indica y ofrece **{{GrafikWiederVersuchen}}** (desde el próximo inicio); "
    "tras una actualización, JK Studio prueba Direct3D 12 una vez por sí solo.",
    "**{{MenuView}} → {{GrafikMenu}}** určuje, jak JK Studio komunikuje s grafickou kartou. **{{GrafikD3d12}}** kreslí "
    "rozhraní i všechny 3D pohledy; **{{GrafikD3d11}}** zůstává k dispozici. Volba se projeví při příštím "
    "spuštění; popisek říká, co právě běží. JK Studio samo přejde na Direct3D 11, když Windows nebo grafická karta Direct3D 12 "
    "neumějí, když spuštění Direct3D 12 selže, když poslední spuštění s Direct3D 12 nebylo dokončeno a když se grafická karta "
    "během práce ztratí a nové zařízení Direct3D 12 nepřijde. Zavření JK Studia, odhlášení ani vypnutí Windows se nikdy "
    "nepočítá jako nedokončené spuštění. Po nedokončeném spuštění to nabídka uvede a nabídne **{{GrafikWiederVersuchen}}** "
    "(od příštího spuštění); po aktualizaci JK Studio jednou samo zkusí Direct3D 12.",
)


def bloecke():
    """Ueberschrift und Absatz fuer inhalt_referenz.py (Einstellungen und Updates)."""
    return [("h2", "grafik", T(*TITEL[:4])), ("p", T(*WAHL[:4]))]


ES = {TITEL[0]: TITEL[4], WAHL[0]: WAHL[4]}
CS = {TITEL[0]: TITEL[5], WAHL[0]: WAHL[5]}
