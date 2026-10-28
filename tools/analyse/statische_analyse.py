#!/usr/bin/env python3
"""Stufe 8d Punkt 2: wiederholbare statische Analyse des eigenen Codes (ohne third_party).

  1. MSVC /analyze: ein eigener Bauordner (Vorgabe build-analyse, Ninja, cl.exe aus
     vcvars64) mit -DBEHAVED_ANALYSE=ON - bhed_core und das Programm mit /analyze
     (Warnungen C6000-C28999), fremder Code ausgenommen (/analyze:external-).
  2. clang-tidy (das LLVM von Visual Studio) mit tools/analyse/clang-tidy-8d.yaml ueber jede
     eigene .cpp aus compile_commands.json dieses Bauordners (src, gui, tests; nicht
     third_party, nicht src/q3map2/kompat).
  3. Abgleich mit tools/analyse/unterdrueckt.txt: jede Zeile "<pruefung>|<datei>|<text>"
     ist ein begruendet hingenommener Fund (der Grund steht als Kommentar darueber).
     Neue Funde -> Rueckgabe 1 und die Liste; sonst 0.

Bericht: <bauordner>/analyse/bericht.txt (alle Funde) und neu.txt (nicht unterdrueckt).

  statische_analyse.py [--bau <ordner>] [--jobs N] [--nur-tidy | --nur-msvc] [--dateien muster]

Prioritaet Idle; nur die Standardbibliothek.
"""
import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

HIER = os.path.dirname(os.path.abspath(__file__))
WURZEL = os.path.dirname(os.path.dirname(HIER))
VS = r'C:\Program Files\Microsoft Visual Studio\18\Community'
VCVARS = VS + r'\VC\Auxiliary\Build\vcvars64.bat'
NINJA = VS + r'\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
TIDY = VS + r'\VC\Tools\Llvm\x64\bin\clang-tidy.exe'
IDLE = 0x40
EIGEN = re.compile(r'[\\/](src|gui|tests|include)[\\/]', re.I)
FREMD = re.compile(r'[\\/](third_party|imgui|kompat|build[^\\/]*)[\\/]', re.I)


def rel(pfad):
    p = os.path.normpath(pfad)
    try:
        r = os.path.relpath(p, WURZEL)
    except ValueError:
        return p.replace('\\', '/')
    return r.replace('\\', '/')


def eigen(pfad):
    return EIGEN.search(pfad) is not None and FREMD.search(pfad) is None


def vcvars_lauf(befehl, log):
    """Ein Befehl in der Umgebung von vcvars64 (cmd), Ausgabe nach log."""
    with open(log, 'w', encoding='utf-8', errors='replace') as f:
        r = subprocess.run('call "%s" >nul && %s' % (VCVARS, befehl), shell=True, stdout=f, stderr=subprocess.STDOUT,
                           creationflags=IDLE)
    return r.returncode


def msvc(bau, aus):
    if not os.path.exists(os.path.join(bau, 'build.ninja')):
        rc = vcvars_lauf('cmake -S "%s" -B "%s" -G Ninja -DCMAKE_MAKE_PROGRAM="%s" -DCMAKE_BUILD_TYPE=Release '
                         '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBEHAVED_ANALYSE=ON -DBEHAVED_Q3MAP2=OFF'
                         % (WURZEL, bau, NINJA), os.path.join(aus, 'konfig.txt'))
        if rc != 0:
            print('Konfiguration gescheitert (%s)' % os.path.join(aus, 'konfig.txt'))
            return None
    log = os.path.join(aus, 'msvc.txt')
    # Wiederholbar: die Objektdateien des eigenen Codes weg, sonst meldet ein zweiter Lauf nichts
    for ziel in ('bhed_core.dir', 'behaved.dir'):
        shutil.rmtree(os.path.join(bau, 'CMakeFiles', ziel), ignore_errors=True)
    rc = vcvars_lauf('"%s" -C "%s" -j 6 -k 0' % (NINJA, bau), log)
    funde = set()
    muster = re.compile(r'^(.*?)\((\d+)(?:,\d+)?\)\s*: warning (C\d+): (.*)$')
    for z in open(log, encoding='utf-8', errors='replace'):
        m = muster.match(z.strip())
        if not m or not eigen(m.group(1)):
            continue
        nummer = int(m.group(3)[1:])
        text = re.sub(r'\s*\[[^\]]*\]$', '', m.group(4)).strip()
        if 6000 <= nummer <= 28999:   # die Warnungen von /analyze
            funde.add(('msvc-' + m.group(3), rel(m.group(1)), m.group(2), text))
    print('MSVC /analyze: Bau %s, %d Funde im eigenen Code' % ('ok' if rc == 0 else 'mit Fehlern (rc %d)' % rc, len(funde)))
    return funde


def tidy(bau, aus, jobs, muster):
    db = os.path.join(bau, 'compile_commands.json')
    if not os.path.exists(db):
        print('compile_commands.json fehlt - erst ohne --nur-tidy laufen lassen')
        return None
    eintraege = json.load(open(db, encoding='utf-8'))
    dateien = sorted({os.path.normpath(e['file']) for e in eintraege
                      if e['file'].endswith('.cpp') and eigen(e['file']) and (muster is None or re.search(muster, e['file']))})
    konfig = os.path.join(HIER, 'clang-tidy-8d.yaml')
    zeile = re.compile(r'^(.*?):(\d+):\d+: (warning|error): (.*) \[([\w.,-]+)\]$')
    funde = set()
    fehler = []

    def eine(datei):
        r = subprocess.run([TIDY, '-p', bau, '--config-file=' + konfig, '--quiet', datei], capture_output=True,
                           creationflags=IDLE, timeout=3600)
        return datei, r.returncode, r.stdout.decode('utf-8', 'replace') + r.stderr.decode('utf-8', 'replace')

    with open(os.path.join(aus, 'tidy.txt'), 'w', encoding='utf-8') as log, ThreadPoolExecutor(jobs) as ex:
        for i, (datei, rc, text) in enumerate(ex.map(eine, dateien), 1):
            log.write('### %s (rc %d)\n%s\n' % (rel(datei), rc, text))
            for z in text.splitlines():
                m = zeile.match(z.strip())
                if not m or not eigen(m.group(1)):
                    continue
                if m.group(3) == 'error':
                    fehler.append('%s:%s: %s' % (rel(m.group(1)), m.group(2), m.group(4)))
                    continue
                for pruefung in m.group(5).split(','):
                    if '-' in pruefung:   # "[i]" am Ende einer Meldung ist kein Pruefungsname
                        funde.add((pruefung, rel(m.group(1)), m.group(2), m.group(4)))
            if i % 25 == 0:
                print('  clang-tidy %d/%d' % (i, len(dateien)), flush=True)
    print('clang-tidy: %d Dateien, %d Funde im eigenen Code, %d Uebersetzungsfehler' % (len(dateien), len(funde), len(fehler)))
    for f in fehler[:10]:
        print('  Fehler: ' + f)
    return funde


def unterdrueckt():
    regeln = []
    pfad = os.path.join(HIER, 'unterdrueckt.txt')
    if os.path.exists(pfad):
        for z in open(pfad, encoding='utf-8'):
            z = z.rstrip('\n')
            if not z or z.startswith('#'):
                continue
            teile = z.split('|', 2)
            if len(teile) == 3:
                regeln.append(teile)
    return regeln


def passt(fund, regeln):
    pruefung, datei, _zeile, text = fund
    for rp, rd, rt in regeln:
        if (rp == '*' or rp == pruefung) and (rd == '*' or datei.endswith(rd)) and (rt == '*' or rt in text):
            return True
    return False


def main():
    sys.stdout.reconfigure(errors='replace')   # Meldungen von cl in der Codepage der Konsole
    a = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    a.add_argument('--bau', default=os.path.join(WURZEL, 'build-analyse'))
    a.add_argument('--jobs', type=int, default=6)
    a.add_argument('--nur-tidy', action='store_true')
    a.add_argument('--nur-msvc', action='store_true')
    a.add_argument('--dateien', help='nur .cpp, deren Pfad zu diesem regulaeren Ausdruck passt')
    o = a.parse_args()
    bau = os.path.abspath(o.bau)
    aus = os.path.join(bau, 'analyse')
    os.makedirs(aus, exist_ok=True)
    funde = set()
    if not o.nur_tidy:
        m = msvc(bau, aus)
        if m is None:
            return 2
        funde |= m
    if not o.nur_msvc:
        t = tidy(bau, aus, o.jobs, o.dateien)
        if t is None:
            return 2
        funde |= t
    regeln = unterdrueckt()
    sortiert = sorted(funde, key=lambda f: (f[1], int(f[2]), f[0]))
    neu = [f for f in sortiert if not passt(f, regeln)]
    with open(os.path.join(aus, 'bericht.txt'), 'w', encoding='utf-8') as f:
        for p, d, z, t in sortiert:
            f.write('%s:%s: [%s] %s\n' % (d, z, p, t))
    with open(os.path.join(aus, 'neu.txt'), 'w', encoding='utf-8') as f:
        for p, d, z, t in neu:
            f.write('%s:%s: [%s] %s\n' % (d, z, p, t))
    zaehler = {}
    for f in sortiert:
        zaehler[f[0]] = zaehler.get(f[0], 0) + 1
    for p, n in sorted(zaehler.items(), key=lambda x: -x[1]):
        print('  %5d  %s' % (n, p))
    print('statische Analyse: %d Funde, %d hingenommen (unterdrueckt.txt), %d neu' % (len(sortiert), len(sortiert) - len(neu), len(neu)))
    for p, d, z, t in neu[:40]:
        print('  NEU %s:%s: [%s] %s' % (d, z, p, t))
    return 1 if neu else 0


if __name__ == '__main__':
    sys.exit(main())
