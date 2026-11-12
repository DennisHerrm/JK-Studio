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
     "The last start with Direct3D 12 did not finish, so JK Studio runs with Direct3D 11. \"Try Direct3D 12 again\" uses it "
     "from the next start; after an update JK Studio tries it once by itself.",
     "Der letzte Start mit Direct3D 12 wurde nicht fertig, deshalb läuft JK Studio mit Direct3D 11. \"Direct3D 12 wieder "
     "versuchen\" nimmt es ab dem nächsten Start; nach einem Update versucht JK Studio es einmal von selbst.",
     "上次使用 Direct3D 12 的启动未能完成,因此 JK Studio 使用 Direct3D 11 运行。\"重新尝试 Direct3D 12\"会在下次启动时"
     "使用它;更新后 JK Studio 会自动尝试一次。",
     "前回の Direct3D 12 での起動が完了しなかったため、JK Studio は Direct3D 11 で動作しています。「Direct3D 12 を再試行」"
     "で次回の起動から使います。アップデート後は JK Studio が一度だけ自動で試します。",
     "El último inicio con Direct3D 12 no terminó, así que JK Studio funciona con Direct3D 11. \"Volver a probar Direct3D 12\" "
     "lo usa desde el próximo inicio; tras una actualización, JK Studio lo intenta una vez por sí solo.",
     "Poslední spuštění s Direct3D 12 nebylo dokončeno, proto JK Studio běží s Direct3D 11. \"Zkusit znovu Direct3D 12\" ho "
     "použije od příštího spuštění; po aktualizaci to JK Studio jednou zkusí samo."),
    # B1 (Entscheidung 12.11.): Hinweis im Menue und "Direct3D 12 wieder versuchen"
    ("GrafikRueckfallKurz",
     "The last start with Direct3D 12 did not finish - Direct3D 11 is running.",
     "Der letzte Start mit Direct3D 12 wurde nicht fertig - es läuft Direct3D 11.",
     "上次使用 Direct3D 12 的启动未能完成,当前运行 Direct3D 11。",
     "前回の Direct3D 12 での起動が完了しませんでした。Direct3D 11 で動作中です。",
     "El último inicio con Direct3D 12 no terminó; funciona Direct3D 11.",
     "Poslední spuštění s Direct3D 12 nebylo dokončeno - běží Direct3D 11."),
    ("GrafikWiederVersuchen", "Try Direct3D 12 again", "Direct3D 12 wieder versuchen", "重新尝试 Direct3D 12",
     "Direct3D 12 を再試行", "Volver a probar Direct3D 12", "Zkusit znovu Direct3D 12"),
    ("GrafikWiederVersuchenHint",
     "Uses Direct3D 12 again from the next start. If that start does not finish either, JK Studio goes back to Direct3D 11 "
     "by itself. After an update JK Studio tries Direct3D 12 once on its own.",
     "Nimmt ab dem nächsten Start wieder Direct3D 12. Wird auch dieser Start nicht fertig, geht JK Studio von selbst zurück "
     "auf Direct3D 11. Nach einem Update versucht JK Studio Direct3D 12 einmal von selbst.",
     "从下次启动起重新使用 Direct3D 12。如果那次启动也未能完成,JK Studio 会自动退回 Direct3D 11。"
     "更新后 JK Studio 会自动尝试一次 Direct3D 12。",
     "次回の起動から再び Direct3D 12 を使います。その起動も完了しなかった場合、JK Studio は自動的に Direct3D 11 に"
     "戻ります。アップデート後は JK Studio が一度だけ自動で Direct3D 12 を試します。",
     "Vuelve a usar Direct3D 12 desde el próximo inicio. Si ese inicio tampoco termina, JK Studio vuelve solo a "
     "Direct3D 11. Tras una actualización, JK Studio prueba Direct3D 12 una vez por sí solo.",
     "Od příštího spuštění znovu použije Direct3D 12. Pokud ani toto spuštění nebude dokončeno, JK Studio se samo vrátí "
     "k Direct3D 11. Po aktualizaci JK Studio jednou samo zkusí Direct3D 12."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
