#!/usr/bin/env python3
"""Stufe 8c Punkt 3e: alte Installationen (rc604, Updater von rc601) kommen zu JK Studio.

Ohne Netz zu GitHub: ein lokaler Testserver spielt GitHub nach der Umbenennung des
Repositorys - /repos/DennisHerrm/BehavEd-Remake/releases/latest antwortet mit 301 auf
/repositories/<id>/releases/latest (so leitet GitHub umbenannte Repositorys weiter),
das Release v1.0.0-rc9999 nennt als erstes .zip das echte Paket JK-Studio-rc9999-
windows-x64.zip (tools/release_paket.py zip, aus jkstudio.exe und bhed_q3map2.exe), der
Download leitet mit 302 auf einen Speicherort um (wie objects.githubusercontent.com).
Der Updater ist der von rc604 (tests/uebergangtest.cpp mit dem Code aus git 8cc7bd1e).

  A  alte Ordner-Installation: BehavEd-Remake\\behaved.exe mit data\\base daneben
     -> behaved.exe ist danach JK Studio (dieselben Bytes wie jkstudio.exe), jkstudio.exe,
        data, handbuch, bhed_q3map2.exe und licenses liegen daneben, behaved.exe.alt ist
        die alte; der Neustart (die eingespielte behaved.exe, hier mit --pruefe-daten)
        reicht an jkstudio.exe weiter ("Programm: jkstudio.exe") und endet mit 0.
  B  alte eigenstaendige .exe (BehavEd-Remake-rc604-windows-x64.exe, ohne data daneben)
     -> genau diese Datei ist danach JK Studio, sonst entsteht nichts daneben; der
        Neustart laeuft selbst ("Programm: BehavEd-Remake-rc604-windows-x64.exe", 0).

Mit --ohne-uebergang baut der Test ein Paket OHNE behaved.exe (eine schlichte Umbenennung)
- dann muessen A und B scheitern (die Rot-Probe: der alte Updater fasst die laufende
.exe nicht an, die Installation bliebe alt und boete das Update ewig an).

  updater_uebergang.py <jkstudio.exe> <bhed_q3map2.exe> <uebergangtest.exe> [--ohne-uebergang]
"""
import hashlib
import http.server
import importlib.util
import json
import os
import shutil
import subprocess
import sys
import tempfile
import threading
import zipfile

HIER = os.path.dirname(os.path.abspath(__file__))
WURZEL = os.path.dirname(os.path.dirname(HIER))
RC = 'rc9999'
TAG = 'v1.0.0-' + RC
ZIPNAME = 'JK-Studio-%s-windows-x64.zip' % RC


def release_paket():
    spec = importlib.util.spec_from_file_location('release_paket', os.path.join(WURZEL, 'tools', 'release_paket.py'))
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


class Server(http.server.ThreadingHTTPServer):
    daemon_threads = True


def server_starten(zip_bytes):
    anfragen = []

    class H(http.server.BaseHTTPRequestHandler):
        def log_message(self, *a):
            pass

        def do_GET(self):
            anfragen.append(self.path)
            basis = 'http://127.0.0.1:%d' % self.server.server_port
            if self.path == '/repos/DennisHerrm/BehavEd-Remake/releases/latest':
                self.send_response(301)
                self.send_header('Location', basis + '/repositories/4711/releases/latest')
                self.send_header('Content-Length', '0')
                self.end_headers()
                return
            if self.path == '/repositories/4711/releases/latest':
                dl = basis + '/DennisHerrm/JK-Studio/releases/download/%s/' % TAG
                text = json.dumps({
                    'url': basis + '/repos/DennisHerrm/JK-Studio/releases/1', 'tag_name': TAG, 'name': 'JK Studio 1.0.0-' + RC,
                    'html_url': 'https://github.com/DennisHerrm/JK-Studio/releases/tag/' + TAG, 'draft': False, 'prerelease': False,
                    'body': 'JK Studio test release',
                    'assets': [
                        {'url': basis + '/repos/DennisHerrm/JK-Studio/releases/assets/1', 'name': ZIPNAME, 'size': len(zip_bytes),
                         'browser_download_url': dl + ZIPNAME},
                        {'url': basis + '/repos/DennisHerrm/JK-Studio/releases/assets/2', 'name': 'JK-Studio-%s-windows-x64.exe' % RC,
                         'size': 1, 'browser_download_url': dl + 'JK-Studio-%s-windows-x64.exe' % RC},
                        {'url': basis + '/repos/DennisHerrm/JK-Studio/releases/assets/3', 'name': 'SHA256SUMS.txt', 'size': 1,
                         'browser_download_url': dl + 'SHA256SUMS.txt'}]}).encode('utf-8')
                self.send_response(200)
                self.send_header('Content-Type', 'application/json')
                self.send_header('Content-Length', str(len(text)))
                self.end_headers()
                self.wfile.write(text)
                return
            if self.path == '/DennisHerrm/JK-Studio/releases/download/%s/%s' % (TAG, ZIPNAME):
                self.send_response(302)
                self.send_header('Location', basis + '/objects/' + ZIPNAME)
                self.send_header('Content-Length', '0')
                self.end_headers()
                return
            if self.path == '/objects/' + ZIPNAME:
                self.send_response(200)
                self.send_header('Content-Type', 'application/octet-stream')
                self.send_header('Content-Length', str(len(zip_bytes)))
                self.end_headers()
                self.wfile.write(zip_bytes)
                return
            self.send_response(404)
            self.send_header('Content-Length', '0')
            self.end_headers()

    s = Server(('127.0.0.1', 0), H)
    threading.Thread(target=s.serve_forever, daemon=True).start()
    return s, anfragen


def sha(p):
    with open(p, 'rb') as f:
        return hashlib.sha256(f.read()).hexdigest()


def pruefe_daten(exe, einst):
    env = dict(os.environ)
    for k in ('BHED_EDITORTEST', 'BHED_APPDATA'):
        env.pop(k, None)
    env['BHED_EINSTELLUNGEN'] = einst
    env['BHED_KEIN_MELDUNGSFENSTER'] = '1'
    r = subprocess.run([exe, '--pruefe-daten'], capture_output=True, timeout=120, env=env, cwd=os.path.dirname(exe))
    return r.returncode, r.stdout.decode('utf-8', 'replace')


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    ohne = '--ohne-uebergang' in sys.argv
    if len(args) != 3 or not all(os.path.isfile(a) for a in args):
        print(__doc__)
        return 2
    jk, q3, harness = (os.path.abspath(a) for a in args)
    fehler = 0

    def melde(gut, text):
        nonlocal fehler
        print(('ok    ' if gut else 'FEHL  ') + text)
        fehler += 0 if gut else 1

    rp = release_paket()
    with tempfile.TemporaryDirectory(prefix='bhed_uebergang_') as t:
        zip_pfad = os.path.join(t, ZIPNAME)
        rp.zip_bauen(jk, zip_pfad, WURZEL, q3map2=q3)
        if ohne:   # die Rot-Probe: schlicht umbenannt, ohne behaved.exe
            neu = os.path.join(t, 'ohne.zip')
            with zipfile.ZipFile(zip_pfad) as a, zipfile.ZipFile(neu, 'w', zipfile.ZIP_DEFLATED) as b:
                for i in a.infolist():
                    if not i.filename.endswith('/' + rp.UEBERGANG):
                        b.writestr(i, a.read(i))
            os.replace(neu, zip_pfad)
        with zipfile.ZipFile(zip_pfad) as z:
            namen = z.namelist()
        print('Paket: %s, %d Eintraege, letzte: %s' % (ZIPNAME, len(namen), ', '.join(n.split('/')[-1] for n in namen[-2:])))
        zip_bytes = open(zip_pfad, 'rb').read()
        server, anfragen = server_starten(zip_bytes)
        api = 'http://127.0.0.1:%d/repos/' % server.server_port
        jk_sha = sha(jk)
        try:
            # --- A: alte Ordner-Installation -------------------------------------
            a = os.path.join(t, 'A', 'BehavEd-Remake')
            os.makedirs(os.path.join(a, 'data', 'base'))
            os.makedirs(os.path.join(a, 'handbuch'))
            with open(os.path.join(a, 'behaved.exe'), 'wb') as f:
                f.write(b'MZ alte Fassung rc604')
            with open(os.path.join(a, 'data', 'base', 'behaved.bhc'), 'wb') as f:
                f.write(b'// alt\r\n')
            with open(os.path.join(a, 'handbuch', 'index.html'), 'w') as f:
                f.write('alt')
            r = subprocess.run([harness, api, os.path.join(a, 'behaved.exe'), os.path.join(t, 'ablageA')], capture_output=True, timeout=300)
            aus = r.stdout.decode('utf-8', 'replace')
            print('  ' + aus.strip().replace('\n', '\n  '))
            melde(r.returncode == 0 and 'v1.0.0-' + RC in aus and ZIPNAME in aus,
                  'A: der Updater von rc604 findet ueber den alten Namen (301) das Release %s und spielt %s ein' % (TAG, ZIPNAME))
            melde('/repositories/4711/releases/latest' in anfragen and '/objects/' + ZIPNAME in anfragen,
                  'A: Weiterleitungen gefolgt (301 des umbenannten Repositorys, 302 des Downloads): %s' % ', '.join(anfragen[:4]))
            ist = os.path.join(a, 'behaved.exe')
            melde(os.path.isfile(ist) and sha(ist) == jk_sha, 'A: behaved.exe ist jetzt JK Studio (dieselben Bytes wie jkstudio.exe)')
            melde(os.path.isfile(os.path.join(a, 'jkstudio.exe')) and sha(os.path.join(a, 'jkstudio.exe')) == jk_sha,
                  'A: jkstudio.exe liegt daneben')
            melde(os.path.isfile(os.path.join(a, 'behaved.exe.alt')) and open(os.path.join(a, 'behaved.exe.alt'), 'rb').read() == b'MZ alte Fassung rc604',
                  'A: die alte behaved.exe liegt als behaved.exe.alt daneben (der neue Start raeumt sie weg)')
            bhc_repo = open(os.path.join(WURZEL, 'data', 'base', 'behaved.bhc'), 'rb').read()
            melde(open(os.path.join(a, 'data', 'base', 'behaved.bhc'), 'rb').read() == bhc_repo and
                  os.path.isfile(os.path.join(a, 'bhed_q3map2.exe')) and os.path.isfile(os.path.join(a, 'licenses', 'OFL-Cinzel.txt')) and
                  open(os.path.join(a, 'handbuch', 'index.html'), encoding='utf-8', errors='replace').read() != 'alt',
                  'A: data, handbuch, bhed_q3map2.exe und licenses sind neu')
            if os.path.isfile(ist) and sha(ist) == jk_sha:
                rc, txt = pruefe_daten(ist, os.path.join(t, 'einstA'))
                melde(rc == 0 and 'Programm: jkstudio.exe' in txt,
                      'A: Neustart der eingespielten behaved.exe reicht an jkstudio.exe weiter (%s, Rueckgabe %d)' %
                      (next((z for z in txt.splitlines() if z.startswith('Programm:')), 'keine Ausgabe'), rc))
            else:
                melde(False, 'A: Neustart - es laeuft weiter die alte Fassung (behaved.exe unveraendert)')
            # --- B: alte eigenstaendige .exe ------------------------------------
            b = os.path.join(t, 'B', 'Downloads')
            os.makedirs(b)
            allein = os.path.join(b, 'BehavEd-Remake-rc604-windows-x64.exe')
            with open(allein, 'wb') as f:
                f.write(b'MZ eigenstaendig rc604')
            r = subprocess.run([harness, api, allein, os.path.join(t, 'ablageB')], capture_output=True, timeout=300)
            aus = r.stdout.decode('utf-8', 'replace')
            print('  ' + aus.strip().replace('\n', '\n  '))
            melde(r.returncode == 0, 'B: der Updater von rc604 spielt das Paket bei einer eigenstaendigen .exe ein')
            melde(sha(allein) == jk_sha, 'B: BehavEd-Remake-rc604-windows-x64.exe ist jetzt JK Studio')
            neben = sorted(os.listdir(b))
            melde(neben == ['BehavEd-Remake-rc604-windows-x64.exe', 'BehavEd-Remake-rc604-windows-x64.exe.alt'],
                  'B: neben der eigenstaendigen .exe entsteht sonst nichts (nur die .alt): %s' % ', '.join(neben))
            if sha(allein) == jk_sha:
                rc, txt = pruefe_daten(allein, os.path.join(t, 'einstB'))
                melde(rc == 0 and 'Programm: BehavEd-Remake-rc604-windows-x64.exe' in txt,
                      'B: Neustart: JK Studio laeuft unter dem alten Namen selbst, mit seinen eingebauten Daten (Rueckgabe %d)' % rc)
            else:
                melde(False, 'B: Neustart - es laeuft weiter die alte Fassung')
        finally:
            server.shutdown()
    print('updater_uebergang: %d Fehler' % fehler)
    return 1 if fehler else 0


if __name__ == '__main__':
    sys.exit(main())
