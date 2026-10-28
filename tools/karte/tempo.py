#!/usr/bin/env python3
"""Messliste fuer alles (Stufe 7 Punkt 1): wie schnell ist BehavEd beim Bauen
und im Karten-Editor - wiederholbar, Median aus N Laeufen, Prioritaet Idle.

Aufruf
------
  tempo.py q3map2 --exe nrc=<q3map2.exe> --exe uns=<bhed_q3map2.exe> [--karten a,b]
      [--threads 1,0] [--stufen bsp,vis,light] [--laeufe 3] [--arbeit <ordner>]
      [--dekomp <karte_dekomp>] --json <datei>
      BSP/VIS/LIGHT je Programm, Karte und Thread-Zahl (0 = Vorgabe: alle Kerne).
      Jede Stufe nimmt die Ausgabe der eigenen Stufe davor (wie ein Bau aus
      dem Build-Menue). Schalter wie das Build-Menue (docs/intern/NOTIZEN_karte3.md).

  tempo.py editor --exe <behaved.exe> --cfg <behaved.cfg> [--laeufe 3]
      [--dekomp <karte_dekomp>] [--karten a,b] [--arbeit <ordner>] --json <datei>
      Startet den Selbsttest BHED_EDITORTEST=karteneditor BHED_KARTE_NUR=tempo
      (gui/selbsttest_karte7.inc) N-mal und sammelt die Zeilen
      "Tempo: <schluessel> = <wert> ms".

  tempo.py vorschau --exe <behaved.exe> --cfg <behaved.cfg> [--laeufe 3]
      [--arbeit <ordner>] --json <datei>
      Szenen-Modus mit md_am_sith (Vorbereitung, Bildzeiten) wie die
      Leistungsmessung der Schlussrunden.

  tempo.py tabelle <vorher.json> [<nachher.json>] [--nrc <q3map2.json>]
      Markdown-Tabelle, nach Zeit sortiert (das Laengste zuerst).

Der Prozess laeuft mit Prioritaet Idle; ueber 8 GB Arbeitsspeicher oder
ueber der Zeitgrenze wird nur er (samt Kindern) beendet. Nur die
Standardbibliothek.
"""
import argparse
import ctypes
import ctypes.wintypes as wt
import json
import os
import re
import shutil
import statistics
import subprocess
import sys
import time

STEAM = r'C:\Program Files (x86)\Steam\steamapps\common'
JA = STEAM + '\\Jedi Academy\\GameData original\\'
MD = STEAM + '\\Jedi Academy\\GameData Movie Duels\\'
JK2 = STEAM + '\\Jedi Outcast\\GameData\\'
LICHT_JA = ['-light', '-fast', '-patchshadows', '-samples', '2', '-filter', '-bounce', '8']
LICHT_JK2 = ['-light', '-fast', '-filter', '-super', '2']

# name: (spiel-unterordner in karte_dekomp, -game, basepath, fs_game, light-Schalter)
KARTEN = {
    'duel_pit': ('jk2', 'jk2', JK2, None, LICHT_JK2),
    'mp__duel1': ('ja', 'ja', JA, None, LICHT_JA),
    't1_inter': ('ja', 'ja', JA, None, LICHT_JA),
    'md_ch_ben': ('md', 'ja', MD, 'MD', LICHT_JA),
    'kejim_post': ('jk2', 'jk2', JK2, None, LICHT_JK2),
    'duel_mos_espa': ('md', 'ja', MD, 'MD', LICHT_JA),
    'md_am_sith': ('md', 'ja', MD, 'MD', LICHT_JA),
}

IDLE = 0x40
OHNE_FENSTER = 0x08000000
GRENZE = 8 * 1024 ** 3


class _Speicher(ctypes.Structure):
    _fields_ = [('cb', wt.DWORD), ('PageFaultCount', wt.DWORD), ('PeakWorkingSetSize', ctypes.c_size_t),
                ('WorkingSetSize', ctypes.c_size_t), ('QuotaPeakPagedPoolUsage', ctypes.c_size_t),
                ('QuotaPagedPoolUsage', ctypes.c_size_t), ('QuotaPeakNonPagedPoolUsage', ctypes.c_size_t),
                ('QuotaNonPagedPoolUsage', ctypes.c_size_t), ('PagefileUsage', ctypes.c_size_t),
                ('PeakPagefileUsage', ctypes.c_size_t)]


def _arbeitsspeicher(pid):
    k32 = ctypes.windll.kernel32
    h = k32.OpenProcess(0x1000 | 0x0010, False, pid)   # QUERY_LIMITED_INFORMATION | VM_READ
    if not h:
        return 0
    try:
        m = _Speicher()
        m.cb = ctypes.sizeof(m)
        if ctypes.windll.psapi.GetProcessMemoryInfo(h, ctypes.byref(m), m.cb):
            return m.WorkingSetSize
        return 0
    finally:
        k32.CloseHandle(h)


def lauf(args, cwd=None, env=None, zeitgrenze=7200, ausgabe=None):
    """Startet args mit Prioritaet Idle; Rueckgabe (Sekunden, Rueckgabewert, Spitze in Byte)."""
    t = time.perf_counter()
    out = open(ausgabe, 'wb') if ausgabe else subprocess.DEVNULL
    try:
        p = subprocess.Popen(args, cwd=cwd, env=env, stdout=out, stderr=subprocess.STDOUT,
                             creationflags=IDLE | OHNE_FENSTER)
        spitze = 0
        while p.poll() is None:
            time.sleep(0.05)
            spitze = max(spitze, _arbeitsspeicher(p.pid))
            if spitze > GRENZE or time.perf_counter() - t > zeitgrenze:
                subprocess.run(['taskkill', '/PID', str(p.pid), '/T', '/F'], stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL)
                p.wait()
                return time.perf_counter() - t, -1, spitze
        return time.perf_counter() - t, p.returncode, spitze
    finally:
        if ausgabe:
            out.close()


def _median(v):
    return statistics.median(v) if v else None


# --- q3map2 ---------------------------------------------------------------------

def q3map2(a):
    progs = [x.split('=', 1) for x in a.exe]
    karten = a.karten.split(',')
    threads = [t for t in a.threads.split(',')]
    stufen = a.stufen.split(',')
    erg = {}
    if os.path.exists(a.json):
        with open(a.json, encoding='utf-8') as f:
            erg = json.load(f)
    for kname in karten:
        unter, spiel, basis, fsgame, licht = KARTEN[kname]
        quelle = os.path.join(a.dekomp, unter, 'q3', kname + '.map')
        for th in threads:
            for name, exe in progs:
                schl = f'{kname}|{name}|t{th}'
                if schl in erg and not a.neu:
                    print('schon gemessen:', schl, flush=True)
                    continue
                z = {s: [] for s in stufen}
                fehler = []
                for i in range(a.laeufe):
                    d = os.path.join(a.arbeit, kname, name + '_t' + th, 'maps')
                    if os.path.isdir(d):
                        shutil.rmtree(d)
                    os.makedirs(d)
                    ziel = os.path.join(d, kname + '.map')
                    shutil.copyfile(quelle, ziel)
                    grund = [exe, '-game', spiel, '-fs_basepath', basis, '-fs_homepath', '']
                    if fsgame:
                        grund += ['-fs_game', fsgame]
                    if th != '0':
                        grund += ['-threads', th]
                    for stufe, extra in (('bsp', ['-meta']), ('vis', ['-vis', '-saveprt']), ('light', licht)):
                        ziel_arg = ziel if stufe == 'bsp' else ziel[:-4] + '.bsp'
                        if stufe not in stufen:
                            if stufe == 'bsp' or (stufe == 'vis' and 'light' in stufen):
                                # Vorstufe ohne Messung (die Eingabe der gemessenen Stufe)
                                lauf(grund + extra + [ziel_arg], zeitgrenze=a.zeitgrenze)
                            continue
                        s, rc, sp = lauf(grund + extra + [ziel_arg], zeitgrenze=a.zeitgrenze,
                                         ausgabe=os.path.join(os.path.dirname(d), f'{stufe}_{i}.txt'))
                        z[stufe].append(s)
                        if rc != 0:
                            fehler.append(f'{stufe} lauf {i}: Rueckgabe {rc}')
                            print(kname, name, th, stufe, 'Rueckgabe', rc, flush=True)
                erg[schl] = {'karte': kname, 'prog': name, 'threads': th, 'fehler': fehler,
                             **{s: {'median': _median(v), 'alle': v} for s, v in z.items()}}
                print(schl, ' '.join(f"{s} {_median(v):.2f} s" for s, v in z.items() if v), flush=True)
                with open(a.json, 'w', encoding='utf-8') as f:
                    json.dump(erg, f, indent=1)
    return 0


# --- Selbsttest (Editor, Vorschau) ----------------------------------------------

def _behaved_lauf(a, name, modus, env_extra, zeitgrenze):
    d = os.path.join(a.arbeit, name)
    if os.path.isdir(d):
        shutil.rmtree(d)
    os.makedirs(os.path.join(d, 'einst'))
    quelle = os.path.dirname(os.path.abspath(a.exe))
    shutil.copyfile(a.exe, os.path.join(d, 'behaved.exe'))
    q3 = os.path.join(quelle, 'bhed_q3map2.exe')   # File > Open .bsp dekompiliert mit q3map2 -convert
    if os.path.exists(q3):
        shutil.copyfile(q3, os.path.join(d, 'bhed_q3map2.exe'))
    shutil.copytree(os.path.join(quelle, 'data'), os.path.join(d, 'data'))
    with open(a.cfg, encoding='utf-8', errors='replace') as f:
        cfg = f.read()
    if 'gamePathJk2=' not in cfg:
        # Jedi Outcast fuer die JK2-Karten (Spiel des Editors jk2.game)
        cfg = cfg.rstrip('\n') + '\ngamePathJk2=' + JK2 + 'base\n'
    with open(os.path.join(d, 'einst', 'behaved.cfg'), 'w', encoding='utf-8', newline='\n') as f:
        f.write(cfg)
    env = dict(os.environ)
    env.update({'BHED_EINSTELLUNGEN': os.path.join(d, 'einst'), 'BHED_EDITORTEST': modus, 'BHED_D3D_DEBUG': '0'})
    env.update(env_extra)
    s, rc, sp = lauf([os.path.join(d, 'behaved.exe')], cwd=d, env=env, zeitgrenze=zeitgrenze)
    log = os.path.join(d, 'jkstudio-detail.log')   # Stufe 8d (bis 8c behaved-detail.log)
    if not os.path.exists(log):
        log = os.path.join(d, 'behaved-detail.log')
    text = ''
    if os.path.exists(log):
        with open(log, 'rb') as f:
            text = f.read().decode('utf-8', 'replace')
    return s, rc, sp, text


TEMPO = re.compile(r'Tempo: (.+?) = ([0-9.]+) ms')


def editor(a):
    werte = {}
    info = []
    env = {'BHED_KARTE_NUR': 'tempo', 'BHED_TEMPO_DEKOMP': a.dekomp, 'BHED_TEMPO_KARTEN': a.karten}
    for i in range(a.laeufe):
        s, rc, sp, text = _behaved_lauf(a, f'tempo_editor_{i}', 'karteneditor', env, a.zeitgrenze)
        fehler = [z for z in text.splitlines() if 'Knopftest FEHLER' in z]
        fertig = [z for z in text.splitlines() if 'Knopftest fertig' in z]
        info.append({'lauf': i, 'sekunden': s, 'rc': rc, 'spitze_mb': sp // (1024 * 1024),
                     'fertig': fertig[-1] if fertig else None, 'fehler': fehler[:20]})
        for k, v in TEMPO.findall(text):
            werte.setdefault(k, []).append(float(v))
        print(f'editor lauf {i}: {s:.0f} s, rc {rc}, {len(fehler)} FEHLER, {fertig[-1] if fertig else "-"}', flush=True)
    erg = {'laeufe': info, 'werte': {k: {'median': _median(v), 'alle': v} for k, v in werte.items()}}
    with open(a.json, 'w', encoding='utf-8') as f:
        json.dump(erg, f, indent=1, ensure_ascii=False)
    return 0


def vorschau(a):
    werte = {}
    info = []
    for i in range(a.laeufe):
        d = os.path.join(a.arbeit, f'tempo_vorschau_{i}')
        os.makedirs(d, exist_ok=True)
        liste = os.path.join(a.arbeit, 'tempo_szenen.txt')
        with open(liste, 'w', encoding='utf-8', newline='\n') as f:
            f.write('maps/md_am_sith.bsp\tmd_am/intro2_sith\nmaps/md_am_sith.bsp\t-flug\n')
        s, rc, sp, text = _behaved_lauf(a, f'tempo_vorschau_{i}', 'szenen', {'BHED_SZENEN': liste}, a.zeitgrenze)
        vor = [float(x) for x in re.findall(r'^Szene vorbereitet: ([0-9.]+)', text, re.M)]
        if vor:
            werte.setdefault('Vorschau: Szene vorbereiten (md_am_sith)', []).append(vor[0])
        for z in text.splitlines():
            m = re.search(r'Szenenlauf: (md_am/intro2_sith \[|Flug).*?Bild max ([0-9.]+) ms, Mittel ([0-9.]+) ms', z)
            if m:
                art = 'Szene' if m.group(1).startswith('md_am') else 'Flug'
                werte.setdefault(f'Vorschau: Bild {art} max', []).append(float(m.group(2)))
                werte.setdefault(f'Vorschau: Bild {art} Mittel', []).append(float(m.group(3)))
        for k, v in TEMPO.findall(text):
            werte.setdefault(k, []).append(float(v))
        info.append({'lauf': i, 'sekunden': s, 'rc': rc, 'spitze_mb': sp // (1024 * 1024)})
        print(f'vorschau lauf {i}: {s:.0f} s, rc {rc}', flush=True)
    erg = {'laeufe': info, 'werte': {k: {'median': _median(v), 'alle': v} for k, v in werte.items()}}
    with open(a.json, 'w', encoding='utf-8') as f:
        json.dump(erg, f, indent=1, ensure_ascii=False)
    return 0


# --- Tabelle ------------------------------------------------------------------------

def _zeilen(datei):
    """{schluessel: millisekunden} aus einer JSON von q3map2, editor oder vorschau."""
    with open(datei, encoding='utf-8') as f:
        d = json.load(f)
    z = {}
    if 'werte' in d:
        for k, v in d['werte'].items():
            if v['median'] is not None:
                z[k] = v['median']
        return z
    for schl, e in d.items():
        for s in ('bsp', 'vis', 'light'):
            if s in e and e[s]['median'] is not None:
                th = 'alle Threads' if e['threads'] == '0' else e['threads'] + ' Thread'
                z[f"Bau {s.upper()} {e['karte']} ({th}) [{e['prog']}]"] = e[s]['median'] * 1000.0
    return z


def tabelle(a):
    vor = {}
    for d in a.vorher:
        vor.update(_zeilen(d))
    nach = {}
    for d in a.nachher or []:
        nach.update(_zeilen(d))
    zeilen = sorted(vor, key=lambda k: -vor[k])

    def fmt(ms):
        if ms is None:
            return '-'
        return f'{ms / 1000:.2f} s' if ms >= 1000 else f'{ms:.1f} ms'
    print('| Messung | vorher | nachher | Gewinn |')
    print('|---|---|---|---|')
    for k in zeilen:
        n = nach.get(k)
        g = '-' if n is None or vor[k] <= 0 else f'{(1 - n / vor[k]) * 100:+.0f} %'
        print(f'| {k} | {fmt(vor[k])} | {fmt(n)} | {g} |')
    return 0

# --- Handbuch (tools/handbuch/tempo7.py) ------------------------------------------

# Editor-Messungen fuer das Handbuch: Anfang des Schluessels -> (en, de, zh, ja)
_EDITOR_HB = [
    ('Alles auswaehlen (', ('Invert Selection (select all)', 'Auswahl umkehren (alles wählen)', '反选(全选)', '選択の反転(すべて選択)')),
    ('.bsp oeffnen, dekompilieren (', ('Open .bsp (decompile)', '.bsp öffnen (dekompilieren)', '打开 .bsp(反编译)', '.bsp を開く(逆コンパイル)')),
    ('.map laden (', ('Load .map', '.map laden', '加载 .map', '.map の読み込み')),
    ('Texturen bis fertig (', ('Textures loaded', 'Texturen geladen', '纹理加载完成', 'テクスチャの読み込み')),
    ('.map speichern (', ('Save .map', '.map speichern', '保存 .map', '.map の保存')),
    ('Auswahl verschieben, alles (', ('Move everything', 'Alles verschieben', '移动全部', 'すべて移動')),
    ('Rueckgaengig Verschieben, alles (', ('Undo the move', 'Verschieben rückgängig', '撤销移动', '移動を元に戻す')),
    ('Rueckgaengig CSG Subtract (', ('Undo CSG Subtract', 'CSG Subtract rückgängig', '撤销 CSG Subtract', 'CSG Subtract を元に戻す')),
    ('Optimierer Start (', ('Optimizer start', 'Optimierer Start', '优化器启动', 'オプティマイザーの起動')),
    ('Optimierer Pruefung (', ('Optimizer check', 'Optimierer Prüfung', '优化器检查', 'オプティマイザーのチェック')),
    ('Moduswechsel bis Spieldaten bereit', ('Switch to the map editor until the game data is ready', 'Wechsel in den Karten-Editor bis die Spieldaten bereit sind', '切换到地图编辑器直到游戏数据就绪', 'マップエディターへの切り替え(ゲームデータの準備まで)')),
    ('Vorschau: Szene vorbereiten', ('Preview: prepare the scene (md_am_sith)', 'Vorschau: Szene vorbereiten (md_am_sith)', '预览:准备场景(md_am_sith)', 'プレビュー:シーンの準備(md_am_sith)')),
    ('Vorschau: Bild Szene Mittel', ('Preview: frame time, scene (mean)', 'Vorschau: Bildzeit Szene (Mittel)', '预览:场景帧时间(平均)', 'プレビュー:シーンのフレーム時間(平均)')),
    ('Vorschau: Bild Flug Mittel', ('Preview: frame time, free flight (mean)', 'Vorschau: Bildzeit Flug (Mittel)', '预览:自由飞行帧时间(平均)', 'プレビュー:フリーフライトのフレーム時間(平均)')),
]

# im Handbuch nur die grossen Karten (die kleinen liegen alle unter 0,1 s)
_EDITOR_KARTEN = ('duel_mos_espa', 'md_am_sith', 'kejim_post')

_STUFE_HB = {'1': ('(1 thread)', '(1 Thread)', '(1 线程)', '(1 スレッド)'),
             '0': ('(all threads)', '(alle Threads)', '(所有线程)', '(全スレッド)')}


def _fmt_s(ms):
    if ms is None:
        return '-'
    return f'{ms / 1000:.2f} s' if ms >= 1000 else f'{ms:.1f} ms'


def handbuch(a):
    with open(a.q3map2, encoding='utf-8') as f:
        q = json.load(f)
    zeilen_q = []
    for kname in KARTEN:
        for th in ('0', '1'):
            for stufe in ('bsp', 'vis', 'light'):
                w = {}
                for prog in ('nrc', 'alt', 'neu'):
                    e = q.get(f'{kname}|{prog}|t{th}')
                    w[prog] = e[stufe]['median'] if e and stufe in e and e[stufe]['median'] is not None else None
                if all(v is None for v in w.values()):
                    continue
                zeilen_q.append((stufe.upper(), th, kname, w))
    with open(a.editor_alt, encoding='utf-8') as f:
        ea = json.load(f)['werte']
    with open(a.editor_neu, encoding='utf-8') as f:
        en = json.load(f)['werte']
    for datei, ziel in ((a.vorschau_alt, ea), (a.vorschau_neu, en)):
        if datei:
            with open(datei, encoding='utf-8') as f:
                ziel.update(json.load(f)['werte'])
    zeilen_e = []
    for anfang, namen in _EDITOR_HB:
        for k in sorted(ea):
            if not k.startswith(anfang) or k.endswith('Bild danach'):
                continue
            karte = k[len(anfang):].rstrip(')') if anfang.endswith('(') else ''
            if karte and karte not in _EDITOR_KARTEN:
                continue
            zeilen_e.append((namen, karte, ea[k]['median'], en.get(k, {}).get('median')))
    out = ['"""Tempo-Tabellen der Stufe 7 (Messliste vorher/nachher, tools/karte/tempo.py).', '',
           'Erzeugt von tools/karte/tempo.py handbuch; nicht von Hand aendern.', '"""',
           'from inhalt_basis import T', '', '', 'def _z(text):', '    return T(text, text, text, text)', '', '', 'Q3MAP2 = [']
    for stufe, th, kname, w in zeilen_q:
        t = _STUFE_HB[th]
        out.append(f'    [T({stufe + " " + t[0]!r}, {stufe + " " + t[1]!r}, {stufe + " " + t[2]!r}, {stufe + " " + t[3]!r}), _z({kname!r}), '
                   + ', '.join(f'_z({("-" if w[p] is None else f"{w[p]:.2f} s")!r})' for p in ('nrc', 'alt', 'neu')) + '],')
    out.append(']')
    out.append('')
    out.append('EDITOR = [')
    for namen, karte, va, vn in zeilen_e:
        out.append(f'    [T({namen[0]!r}, {namen[1]!r}, {namen[2]!r}, {namen[3]!r}), _z({(karte or "-")!r}), _z({_fmt_s(va)!r}), _z({_fmt_s(vn)!r})],')
    out.append(']')
    with open(a.aus, 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(out) + '\n')
    print(f'{len(zeilen_q)} q3map2-Zeilen, {len(zeilen_e)} Editor-Zeilen -> {a.aus}')
    return 0


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    u = p.add_subparsers(dest='was', required=True)
    sp_dir = os.environ.get('BHED_SP', '')
    q = u.add_parser('q3map2')
    q.add_argument('--exe', action='append', required=True)
    q.add_argument('--karten', default='duel_pit,mp__duel1,t1_inter,md_ch_ben,kejim_post')
    q.add_argument('--threads', default='1,0')
    q.add_argument('--stufen', default='bsp,vis,light')
    q.add_argument('--laeufe', type=int, default=3)
    q.add_argument('--zeitgrenze', type=int, default=7200)
    q.add_argument('--arbeit', default=r'Y:\karte\laeufe\tempo')
    q.add_argument('--dekomp', default=os.path.join(sp_dir, 'karte_dekomp'))
    q.add_argument('--neu', action='store_true', help='schon gemessene Eintraege der JSON neu messen')
    q.add_argument('--json', required=True)
    for n in ('editor', 'vorschau'):
        e = u.add_parser(n)
        e.add_argument('--exe', required=True)
        e.add_argument('--cfg', required=True)
        e.add_argument('--laeufe', type=int, default=3)
        e.add_argument('--zeitgrenze', type=int, default=1800)
        e.add_argument('--arbeit', default=r'Y:\karte\laeufe\tempo')
        e.add_argument('--dekomp', default=os.path.join(sp_dir, 'karte_dekomp'))
        e.add_argument('--karten', default='duel_pit,mp__duel1,t1_inter,kejim_post,duel_mos_espa,md_am_sith')
        e.add_argument('--json', required=True)
    t = u.add_parser('tabelle')
    t.add_argument('vorher', nargs='+')
    t.add_argument('--nachher', nargs='*')
    h = u.add_parser('handbuch', help='tools/handbuch/tempo7.py aus den Messungen')
    h.add_argument('--q3map2', required=True)
    h.add_argument('--editor-alt', required=True)
    h.add_argument('--editor-neu', required=True)
    h.add_argument('--vorschau-alt')
    h.add_argument('--vorschau-neu')
    h.add_argument('--aus', default=os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'handbuch', 'tempo7.py'))
    a = p.parse_args()
    return {'q3map2': q3map2, 'editor': editor, 'vorschau': vorschau, 'tabelle': tabelle, 'handbuch': handbuch}[a.was](a)


if __name__ == '__main__':
    sys.exit(main())
