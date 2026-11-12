# -*- coding: utf-8 -*-
"""Oberflaechentexte der Wahl der Grafikschnittstelle im Menue Ansicht (D3D12, rc612).

Direct3D 12 ist Vorgabe (Oberflaeche und 3D-Ansichten), Direct3D 11 der Rueckfall; die Wahl wirkt beim naechsten Start
(gui/main_win32.cpp, gui/backend_d3d12_win32.cpp, gui/abgeber12.h). Je Zeile: Kennung, English, Deutsch, 中文, 日本語,
Español, Čeština.
"""

ZEILEN = [
    ("GrafikMenu", "Graphics API", "Grafikschnittstelle", "图形接口", "グラフィックス API", "Interfaz gráfica", "Grafické rozhraní"),
    ("GrafikD3d11", "Direct3D 11 (fallback)", "Direct3D 11 (Rückfall)", "Direct3D 11(备用)", "Direct3D 11(フォールバック)",
     "Direct3D 11 (alternativa)", "Direct3D 11 (záložní)"),
    ("GrafikD3d12", "Direct3D 12 (default)", "Direct3D 12 (Vorgabe)", "Direct3D 12(默认)", "Direct3D 12(既定)",
     "Direct3D 12 (predeterminado)", "Direct3D 12 (výchozí)"),
    ("GrafikHint",
     "Takes effect at the next start. Direct3D 12 draws the interface and the 3D views. If Direct3D 12 is missing, "
     "fails to start or stops working, JK Studio uses Direct3D 11 by itself. Running now: %s",
     "Wirkt beim nächsten Start. Direct3D 12 zeichnet die Oberfläche und die 3D-Ansichten. Fehlt Direct3D 12, scheitert "
     "der Start oder fällt es aus, nimmt JK Studio von selbst Direct3D 11. Läuft jetzt: %s",
     "下次启动时生效。Direct3D 12 绘制界面和 3D 视图。如果缺少 Direct3D 12、启动失败或运行中出错,"
     "JK Studio 会自动改用 Direct3D 11。当前运行:%s",
     "次回の起動から有効です。Direct3D 12 がインターフェースと 3D ビューを描画します。Direct3D 12 がない、"
     "起動に失敗した、または動作しなくなった場合、JK Studio は自動的に Direct3D 11 を使います。現在: %s",
     "Se aplica en el próximo inicio. Direct3D 12 dibuja la interfaz y las vistas 3D. Si falta Direct3D 12, no logra "
     "iniciarse o deja de funcionar, JK Studio usa Direct3D 11 por sí mismo. En uso ahora: %s",
     "Projeví se při příštím spuštění. Direct3D 12 kreslí rozhraní i 3D pohledy. Pokud Direct3D 12 chybí, nespustí se "
     "nebo přestane fungovat, JK Studio samo použije Direct3D 11. Nyní běží: %s"),
    ("GrafikRueckfall",
     "The last start with Direct3D 12 did not finish, so JK Studio started with Direct3D 11 and switched this setting to "
     "Direct3D 11. Choose Direct3D 12 to try it again.",
     "Der letzte Start mit Direct3D 12 wurde nicht fertig. JK Studio ist deshalb mit Direct3D 11 gestartet und hat diese "
     "Einstellung auf Direct3D 11 gestellt. Wähle Direct3D 12, um es noch einmal zu versuchen.",
     "上次使用 Direct3D 12 启动未能完成,因此 JK Studio 以 Direct3D 11 启动,并将此设置改为 Direct3D 11。"
     "选择 Direct3D 12 可再次尝试。",
     "前回の Direct3D 12 での起動が完了しなかったため、JK Studio は Direct3D 11 で起動し、この設定を "
     "Direct3D 11 に切り替えました。もう一度試すには Direct3D 12 を選んでください。",
     "El último inicio con Direct3D 12 no terminó, así que JK Studio se inició con Direct3D 11 y cambió este ajuste a "
     "Direct3D 11. Elige Direct3D 12 para volver a intentarlo.",
     "Poslední spuštění s Direct3D 12 nebylo dokončeno, proto se JK Studio spustilo s Direct3D 11 a přepnulo toto "
     "nastavení na Direct3D 11. Zvolte Direct3D 12, chcete-li to zkusit znovu."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
