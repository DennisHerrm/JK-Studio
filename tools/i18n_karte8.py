# -*- coding: utf-8 -*-
"""Oberflaechentexte des Karten-Editors, Stufe 8a (Kompilier-Fortschritt,
BSP-Import ohne Datenverlust, Teil-Kompilierungen), fuer tools/gen_i18n.py.

Alles hier sind eigene Funktionen (nicht in NetRadiant-Custom): eigene
Eintraege im Build-Menue mit Tooltip. Mit "Begriffe wie NetRadiant" bleiben
sie englisch.
"""

# (Kennung, English, Deutsch, 中文, 日本語)
KARTE8 = [
    # Punkt 1: Fortschritt mit Restzeit
    ("KeFsTitel", "Build progress", "Baufortschritt", "编译进度", "ビルドの進行状況"),
    ("KeFsSchritt", "%s - step %d of %d: %s", "%s - Schritt %d von %d: %s", "%s - 第 %d 步,共 %d 步:%s", "%s - ステップ %d / %d:%s"),
    ("KeFsBounce", "bounce %d of %d", "Bounce %d von %d", "反弹 %d / %d", "バウンス %d / %d"),
    ("KeFsWarten", "starting ...", "startet ...", "正在启动 ...", "開始中 ..."),
    ("KeFsGesamt", "All steps", "Alle Schritte", "所有步骤", "全ステップ"),
    ("KeFsRestVerlauf", "about %s left (from the last build of this map)", "noch etwa %s (aus dem letzten Bau dieser Karte)",
     "大约还剩 %s(根据此地图上次的编译)", "残り約 %s(このマップの前回のビルドから)"),
    ("KeFsRestGeschaetzt", "at least %s left (estimated from this phase; exact from the second build)",
     "noch mindestens %s (geschätzt aus dieser Phase; genau ab dem zweiten Bau)",
     "至少还剩 %s(根据此阶段估算;第二次编译起更准确)", "残り %s 以上(このフェーズから推定。2 回目のビルドから正確になります)"),
    ("KeFsRestUnbekannt", "remaining time: known from the second build of this map", "Restzeit: ab dem zweiten Bau dieser Karte",
     "剩余时间:从此地图的第二次编译起可知", "残り時間:このマップの 2 回目のビルドからわかります"),
    ("KeFsVergangen", "%s elapsed", "%s vergangen", "已用 %s", "経過 %s"),
    ("KeFsHinweis", "Progress from q3map2's own output (not in NetRadiant-Custom)", "Fortschritt aus der Ausgabe von q3map2 (nicht in NetRadiant-Custom)",
     "进度来自 q3map2 自身的输出(NetRadiant-Custom 中没有)", "進行状況は q3map2 自身の出力から(NetRadiant-Custom にはありません)"),
    ("KeFsZeitenKopf", "Time per step:", "Zeit je Schritt:", "每步用时:", "ステップごとの時間:"),
    ("KeFsZeitJetzt", "  %d %s: %s s", "  %d %s: %s s", "  %d %s:%s 秒", "  %d %s:%s 秒"),
    ("KeFsZeitVergleich", " (last build %s s, %s)", " (letzter Bau %s s, %s)", "(上次编译 %s 秒,%s)", "(前回のビルド %s 秒、%s)"),
    ("KeFsMelden", "Notify when a build is done", "Melden, wenn der Bau fertig ist", "编译完成时通知", "ビルド完了時に通知"),
    ("KeFsMeldenTip",
     "When a build ends while BehavEd is not the front window, its taskbar button flashes until you switch to it - no sound, no dialog (not in NetRadiant-Custom)",
     "Endet ein Bau, während BehavEd nicht das vordere Fenster ist, blinkt sein Knopf in der Taskleiste, bis Sie dorthin wechseln - ohne Ton, ohne Dialog (nicht in NetRadiant-Custom)",
     "编译结束时如果 BehavEd 不是前台窗口,它在任务栏上的按钮会闪烁,直到您切换过去——没有声音,没有对话框(NetRadiant-Custom 中没有)",
     "ビルドが終わったときに BehavEd が前面のウィンドウでなければ、切り替えるまでタスクバーのボタンが点滅します。音もダイアログもありません(NetRadiant-Custom にはありません)"),
]
