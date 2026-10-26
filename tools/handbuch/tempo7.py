"""Tempo-Tabellen der Stufe 7 (Messliste vorher/nachher, tools/karte/tempo.py).

Erzeugt von tools/karte/tempo.py handbuch; nicht von Hand aendern.
"""
from inhalt_basis import T


def _z(text):
    return T(text, text, text, text)


Q3MAP2 = [
    [T('BSP (all threads)', 'BSP (alle Threads)', 'BSP (所有线程)', 'BSP (全スレッド)'), _z('duel_pit'), _z('0.20 s'), _z('0.20 s'), _z('0.20 s')],
    [T('VIS (all threads)', 'VIS (alle Threads)', 'VIS (所有线程)', 'VIS (全スレッド)'), _z('duel_pit'), _z('0.15 s'), _z('0.10 s'), _z('0.10 s')],
    [T('LIGHT (all threads)', 'LIGHT (alle Threads)', 'LIGHT (所有线程)', 'LIGHT (全スレッド)'), _z('duel_pit'), _z('0.66 s'), _z('0.66 s'), _z('0.61 s')],
    [T('BSP (1 thread)', 'BSP (1 Thread)', 'BSP (1 线程)', 'BSP (1 スレッド)'), _z('duel_pit'), _z('0.20 s'), _z('0.20 s'), _z('0.20 s')],
    [T('VIS (1 thread)', 'VIS (1 Thread)', 'VIS (1 线程)', 'VIS (1 スレッド)'), _z('duel_pit'), _z('0.15 s'), _z('0.15 s'), _z('0.15 s')],
    [T('LIGHT (1 thread)', 'LIGHT (1 Thread)', 'LIGHT (1 线程)', 'LIGHT (1 スレッド)'), _z('duel_pit'), _z('3.02 s'), _z('3.33 s'), _z('3.08 s')],
    [T('BSP (all threads)', 'BSP (alle Threads)', 'BSP (所有线程)', 'BSP (全スレッド)'), _z('mp__duel1'), _z('0.36 s'), _z('0.31 s'), _z('0.31 s')],
    [T('VIS (all threads)', 'VIS (alle Threads)', 'VIS (所有线程)', 'VIS (全スレッド)'), _z('mp__duel1'), _z('0.15 s'), _z('0.11 s'), _z('0.10 s')],
    [T('LIGHT (all threads)', 'LIGHT (alle Threads)', 'LIGHT (所有线程)', 'LIGHT (全スレッド)'), _z('mp__duel1'), _z('2.53 s'), _z('2.99 s'), _z('1.92 s')],
    [T('BSP (1 thread)', 'BSP (1 Thread)', 'BSP (1 线程)', 'BSP (1 スレッド)'), _z('mp__duel1'), _z('0.35 s'), _z('0.31 s'), _z('0.31 s')],
    [T('VIS (1 thread)', 'VIS (1 Thread)', 'VIS (1 线程)', 'VIS (1 スレッド)'), _z('mp__duel1'), _z('0.20 s'), _z('0.15 s'), _z('0.16 s')],
    [T('LIGHT (1 thread)', 'LIGHT (1 Thread)', 'LIGHT (1 线程)', 'LIGHT (1 スレッド)'), _z('mp__duel1'), _z('8.72 s'), _z('9.68 s'), _z('9.63 s')],
    [T('BSP (all threads)', 'BSP (alle Threads)', 'BSP (所有线程)', 'BSP (全スレッド)'), _z('t1_inter'), _z('0.31 s'), _z('0.26 s'), _z('0.26 s')],
    [T('VIS (all threads)', 'VIS (alle Threads)', 'VIS (所有线程)', 'VIS (全スレッド)'), _z('t1_inter'), _z('0.15 s'), _z('0.10 s'), _z('0.10 s')],
    [T('LIGHT (all threads)', 'LIGHT (alle Threads)', 'LIGHT (所有线程)', 'LIGHT (全スレッド)'), _z('t1_inter'), _z('0.31 s'), _z('0.26 s'), _z('0.31 s')],
    [T('BSP (1 thread)', 'BSP (1 Thread)', 'BSP (1 线程)', 'BSP (1 スレッド)'), _z('t1_inter'), _z('0.30 s'), _z('0.25 s'), _z('0.26 s')],
    [T('VIS (1 thread)', 'VIS (1 Thread)', 'VIS (1 线程)', 'VIS (1 スレッド)'), _z('t1_inter'), _z('0.15 s'), _z('0.11 s'), _z('0.10 s')],
    [T('LIGHT (1 thread)', 'LIGHT (1 Thread)', 'LIGHT (1 线程)', 'LIGHT (1 スレッド)'), _z('t1_inter'), _z('0.31 s'), _z('0.31 s'), _z('0.31 s')],
    [T('BSP (all threads)', 'BSP (alle Threads)', 'BSP (所有线程)', 'BSP (全スレッド)'), _z('md_ch_ben'), _z('0.81 s'), _z('0.71 s'), _z('0.76 s')],
    [T('VIS (all threads)', 'VIS (alle Threads)', 'VIS (所有线程)', 'VIS (全スレッド)'), _z('md_ch_ben'), _z('2.77 s'), _z('2.27 s'), _z('2.23 s')],
    [T('LIGHT (all threads)', 'LIGHT (alle Threads)', 'LIGHT (所有线程)', 'LIGHT (全スレッド)'), _z('md_ch_ben'), _z('3.73 s'), _z('4.24 s'), _z('2.58 s')],
    [T('BSP (1 thread)', 'BSP (1 Thread)', 'BSP (1 线程)', 'BSP (1 スレッド)'), _z('md_ch_ben'), _z('0.86 s'), _z('0.71 s'), _z('0.71 s')],
    [T('VIS (1 thread)', 'VIS (1 Thread)', 'VIS (1 线程)', 'VIS (1 スレッド)'), _z('md_ch_ben'), _z('30.53 s'), _z('24.90 s'), _z('24.60 s')],
    [T('LIGHT (1 thread)', 'LIGHT (1 Thread)', 'LIGHT (1 线程)', 'LIGHT (1 スレッド)'), _z('md_ch_ben'), _z('12.66 s'), _z('12.65 s'), _z('12.76 s')],
    [T('BSP (all threads)', 'BSP (alle Threads)', 'BSP (所有线程)', 'BSP (全スレッド)'), _z('kejim_post'), _z('4.74 s'), _z('4.59 s'), _z('4.14 s')],
    [T('VIS (all threads)', 'VIS (alle Threads)', 'VIS (所有线程)', 'VIS (全スレッド)'), _z('kejim_post'), _z('745.40 s'), _z('621.47 s'), _z('573.70 s')],
    [T('LIGHT (all threads)', 'LIGHT (alle Threads)', 'LIGHT (所有线程)', 'LIGHT (全スレッド)'), _z('kejim_post'), _z('9.18 s'), _z('10.28 s'), _z('7.21 s')],
    [T('BSP (all threads)', 'BSP (alle Threads)', 'BSP (所有线程)', 'BSP (全スレッド)'), _z('duel_mos_espa'), _z('53.35 s'), _z('44.86 s'), _z('19.57 s')],
]

EDITOR = [
    [T('Invert Selection (select all)', 'Auswahl umkehren (alles wählen)', '反选(全选)', '選択の反転(すべて選択)'), _z('duel_mos_espa'), _z('29.74 s'), _z('5.2 ms')],
    [T('Invert Selection (select all)', 'Auswahl umkehren (alles wählen)', '反选(全选)', '選択の反転(すべて選択)'), _z('kejim_post'), _z('1.34 s'), _z('0.7 ms')],
    [T('Invert Selection (select all)', 'Auswahl umkehren (alles wählen)', '反选(全选)', '選択の反転(すべて選択)'), _z('md_am_sith'), _z('1.36 s'), _z('0.8 ms')],
    [T('Open .bsp (decompile)', '.bsp öffnen (dekompilieren)', '打开 .bsp(反编译)', '.bsp を開く(逆コンパイル)'), _z('duel_mos_espa'), _z('7.71 s'), _z('7.98 s')],
    [T('Open .bsp (decompile)', '.bsp öffnen (dekompilieren)', '打开 .bsp(反编译)', '.bsp を開く(逆コンパイル)'), _z('kejim_post'), _z('2.01 s'), _z('2.64 s')],
    [T('Open .bsp (decompile)', '.bsp öffnen (dekompilieren)', '打开 .bsp(反编译)', '.bsp を開く(逆コンパイル)'), _z('md_am_sith'), _z('3.29 s'), _z('3.42 s')],
    [T('Load .map', '.map laden', '加载 .map', '.map の読み込み'), _z('duel_mos_espa'), _z('1.31 s'), _z('1.29 s')],
    [T('Load .map', '.map laden', '加载 .map', '.map の読み込み'), _z('kejim_post'), _z('662.5 ms'), _z('717.4 ms')],
    [T('Load .map', '.map laden', '加载 .map', '.map の読み込み'), _z('md_am_sith'), _z('805.6 ms'), _z('827.7 ms')],
    [T('Textures loaded', 'Texturen geladen', '纹理加载完成', 'テクスチャの読み込み'), _z('duel_mos_espa'), _z('1.64 s'), _z('1.80 s')],
    [T('Textures loaded', 'Texturen geladen', '纹理加载完成', 'テクスチャの読み込み'), _z('kejim_post'), _z('4.2 ms'), _z('821.6 ms')],
    [T('Textures loaded', 'Texturen geladen', '纹理加载完成', 'テクスチャの読み込み'), _z('md_am_sith'), _z('1.93 s'), _z('2.16 s')],
    [T('Save .map', '.map speichern', '保存 .map', '.map の保存'), _z('duel_mos_espa'), _z('815.8 ms'), _z('745.4 ms')],
    [T('Save .map', '.map speichern', '保存 .map', '.map の保存'), _z('kejim_post'), _z('142.5 ms'), _z('173.7 ms')],
    [T('Save .map', '.map speichern', '保存 .map', '.map の保存'), _z('md_am_sith'), _z('175.9 ms'), _z('189.6 ms')],
    [T('Move everything', 'Alles verschieben', '移动全部', 'すべて移動'), _z('duel_mos_espa'), _z('285.1 ms'), _z('329.4 ms')],
    [T('Move everything', 'Alles verschieben', '移动全部', 'すべて移動'), _z('kejim_post'), _z('53.2 ms'), _z('72.0 ms')],
    [T('Move everything', 'Alles verschieben', '移动全部', 'すべて移動'), _z('md_am_sith'), _z('62.9 ms'), _z('68.0 ms')],
    [T('Undo the move', 'Verschieben rückgängig', '撤销移动', '移動を元に戻す'), _z('duel_mos_espa'), _z('294.1 ms'), _z('353.0 ms')],
    [T('Undo the move', 'Verschieben rückgängig', '撤销移动', '移動を元に戻す'), _z('kejim_post'), _z('26.2 ms'), _z('36.4 ms')],
    [T('Undo the move', 'Verschieben rückgängig', '撤销移动', '移動を元に戻す'), _z('md_am_sith'), _z('33.3 ms'), _z('32.6 ms')],
    [T('Undo CSG Subtract', 'CSG Subtract rückgängig', '撤销 CSG Subtract', 'CSG Subtract を元に戻す'), _z('duel_mos_espa'), _z('171.1 ms'), _z('207.4 ms')],
    [T('Undo CSG Subtract', 'CSG Subtract rückgängig', '撤销 CSG Subtract', 'CSG Subtract を元に戻す'), _z('kejim_post'), _z('8.6 ms'), _z('7.9 ms')],
    [T('Undo CSG Subtract', 'CSG Subtract rückgängig', '撤销 CSG Subtract', 'CSG Subtract を元に戻す'), _z('md_am_sith'), _z('12.1 ms'), _z('10.1 ms')],
    [T('Optimizer start', 'Optimierer Start', '优化器启动', 'オプティマイザーの起動'), _z('duel_mos_espa'), _z('48.6 ms'), _z('39.2 ms')],
    [T('Optimizer start', 'Optimierer Start', '优化器启动', 'オプティマイザーの起動'), _z('kejim_post'), _z('10.2 ms'), _z('13.1 ms')],
    [T('Optimizer start', 'Optimierer Start', '优化器启动', 'オプティマイザーの起動'), _z('md_am_sith'), _z('14.7 ms'), _z('11.8 ms')],
    [T('Optimizer check', 'Optimierer Prüfung', '优化器检查', 'オプティマイザーのチェック'), _z('duel_mos_espa'), _z('1.47 s'), _z('1.34 s')],
    [T('Optimizer check', 'Optimierer Prüfung', '优化器检查', 'オプティマイザーのチェック'), _z('kejim_post'), _z('162.4 ms'), _z('201.8 ms')],
    [T('Optimizer check', 'Optimierer Prüfung', '优化器检查', 'オプティマイザーのチェック'), _z('md_am_sith'), _z('303.7 ms'), _z('257.4 ms')],
    [T('Switch to the map editor until the game data is ready', 'Wechsel in den Karten-Editor bis die Spieldaten bereit sind', '切换到地图编辑器直到游戏数据就绪', 'マップエディターへの切り替え(ゲームデータの準備まで)'), _z('-'), _z('273.3 ms'), _z('258.1 ms')],
    [T('Preview: prepare the scene (md_am_sith)', 'Vorschau: Szene vorbereiten (md_am_sith)', '预览:准备场景(md_am_sith)', 'プレビュー:シーンの準備(md_am_sith)'), _z('-'), _z('1.44 s'), _z('1.40 s')],
    [T('Preview: frame time, scene (mean)', 'Vorschau: Bildzeit Szene (Mittel)', '预览:场景帧时间(平均)', 'プレビュー:シーンのフレーム時間(平均)'), _z('-'), _z('15.0 ms'), _z('15.0 ms')],
    [T('Preview: frame time, free flight (mean)', 'Vorschau: Bildzeit Flug (Mittel)', '预览:自由飞行帧时间(平均)', 'プレビュー:フリーフライトのフレーム時間(平均)'), _z('-'), _z('8.2 ms'), _z('8.1 ms')],
]
