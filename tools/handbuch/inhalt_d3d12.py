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
    "finish (then the setting changes to Direct3D 11 and the tooltip says so), and when the graphics card is lost while "
    "working and no new Direct3D 12 device comes.",
    "**{{MenuView}} → {{GrafikMenu}}** wählt, wie JK Studio mit der Grafikkarte spricht. **{{GrafikD3d12}}** zeichnet "
    "die Oberfläche und alle 3D-Ansichten; **{{GrafikD3d11}}** bleibt wählbar. Die Wahl "
    "gilt ab dem nächsten Start; der Tooltip nennt, was gerade läuft. JK Studio nimmt von selbst Direct3D 11, wenn Windows "
    "oder die Grafikkarte kein Direct3D 12 kann, wenn der Start von Direct3D 12 scheitert, wenn der letzte Start mit "
    "Direct3D 12 nicht fertig wurde (dann steht die Einstellung danach auf Direct3D 11, und der Tooltip sagt es) und wenn die "
    "Grafikkarte während der Arbeit verloren geht und kein neues Direct3D-12-Gerät kommt.",
    "**{{MenuView}} → {{GrafikMenu}}** 选择 JK Studio 与显卡通信的方式。**{{GrafikD3d12}}** 绘制界面和所有 3D 视图;"
    "**{{GrafikD3d11}}** 仍可选择。所选项在下次启动时生效;工具提示会显示当前正在运行的接口。在以下情况下 JK Studio "
    "会自动改用 Direct3D 11:Windows 或显卡不支持 Direct3D 12;Direct3D 12 启动失败;上次使用 Direct3D 12 的启动未能完成(此后设置会改为 "
    "Direct3D 11,工具提示会说明);工作中显卡丢失且无法获得新的 Direct3D 12 设备。",
    "**{{MenuView}} → {{GrafikMenu}}** で、JK Studio がグラフィックカードとやり取りする方法を選びます。**{{GrafikD3d12}}** が"
    "インターフェースとすべての 3D ビューを描画します。**{{GrafikD3d11}}** も引き続き選べます。選択は次回の"
    "起動から有効になり、ツールチップに現在動作中の方式が表示されます。次の場合、JK Studio は自動的に Direct3D 11 を使います:Windows "
    "またはグラフィックカードが Direct3D 12 に対応していない、Direct3D 12 の起動に失敗した、前回の Direct3D 12 での起動が完了しなかった"
    "(その後は設定が Direct3D 11 になり、ツールチップにそう表示されます)、作業中にグラフィックカードが失われ新しい Direct3D 12 デバイスが"
    "得られない。",
    "**{{MenuView}} → {{GrafikMenu}}** elige cómo habla JK Studio con la tarjeta gráfica. **{{GrafikD3d12}}** dibuja "
    "la interfaz y todas las vistas 3D; **{{GrafikD3d11}}** sigue disponible. "
    "La elección se aplica en el próximo inicio; la descripción emergente indica qué está en uso ahora. JK Studio pasa por sí "
    "solo a Direct3D 11 cuando Windows o la tarjeta gráfica no admiten Direct3D 12, cuando falla el inicio de Direct3D 12, "
    "cuando el último inicio con Direct3D 12 no terminó (entonces el ajuste pasa a Direct3D 11 y la descripción emergente lo "
    "indica) y cuando se pierde la tarjeta gráfica durante el trabajo y no llega un nuevo dispositivo Direct3D 12.",
    "**{{MenuView}} → {{GrafikMenu}}** určuje, jak JK Studio komunikuje s grafickou kartou. **{{GrafikD3d12}}** kreslí "
    "rozhraní i všechny 3D pohledy; **{{GrafikD3d11}}** zůstává k dispozici. Volba se projeví při příštím "
    "spuštění; popisek říká, co právě běží. JK Studio samo přejde na Direct3D 11, když Windows nebo grafická karta Direct3D 12 "
    "neumějí, když spuštění Direct3D 12 selže, když poslední spuštění s Direct3D 12 nebylo dokončeno (nastavení se pak přepne "
    "na Direct3D 11 a popisek to uvede) a když se grafická karta během práce ztratí a nové zařízení Direct3D 12 nepřijde.",
)


def bloecke():
    """Ueberschrift und Absatz fuer inhalt_referenz.py (Einstellungen und Updates)."""
    return [("h2", "grafik", T(*TITEL[:4])), ("p", T(*WAHL[:4]))]


ES = {TITEL[0]: TITEL[4], WAHL[0]: WAHL[4]}
CS = {TITEL[0]: TITEL[5], WAHL[0]: WAHL[5]}
