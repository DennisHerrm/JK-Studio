#!/usr/bin/env python3
"""REL-TEMPO: der Updater im Programm (gui/update_win32.cpp, WinHTTP) gegen einen lokalen Testserver - mit Delta und Rueckfall.

Der Selbsttest `update` (gui/selbsttest_programm.inc: nachsehen, installieren, pruefen, noch einmal suchen) laeuft gegen den Server,
den dieses Skript startet (BHED_UPDATE_API=http://127.0.0.1:<port>/repos/ - nur 127.0.0.1 nimmt das Programm an), mit
BHED_UPDATE_LOKAL=rc9998. Das Release v1.0.0-rc9999 hat ein Delta from-rc9998, das Manifest, das volle Paket und SHA256SUMS.txt.
Je Fall eine eigene Kopie von jkstudio.exe und data/ in einem leeren Ordner (nie die installierte Fassung):

  delta  das Delta passt: geladen werden nur Delta, Manifest und SHA256SUMS.txt (nie das .zip); danach sind jkstudio.exe und
         behaved.exe die neue Fassung, rt_delta_probe.txt ist da, jkstudio.exe.alt die alte
  voll   das Delta ist kaputt (ein Byte): das Programm nimmt das volle Paket; danach rt_voll_probe.txt

  updater_gui_delta.py <jkstudio.exe-Ordner> [<arbeitsordner>]
"""
import hashlib
import http.server
import io
import json
import os
import shutil
import subprocess
import sys
import tempfile
import threading
import time
import zipfile

ALT, NEU, TAG = "rc9998", "rc9999", "v1.0.0-rc9999"


def sha(b):
    return hashlib.sha256(b).hexdigest()


def zip_bytes(eintraege):
    puffer = io.BytesIO()
    with zipfile.ZipFile(puffer, "w", zipfile.ZIP_DEFLATED) as z:
        for n, b in eintraege:
            z.writestr(zipfile.ZipInfo(n, date_time=(2026, 11, 15, 12, 0, 0)), b, compress_type=zipfile.ZIP_DEFLATED)
    return puffer.getvalue()


class Server(http.server.ThreadingHTTPServer):
    daemon_threads = True


def main():
    if len(sys.argv) < 2 or not os.path.isfile(os.path.join(sys.argv[1], "jkstudio.exe")):
        print(__doc__)
        return 2
    quelle = sys.argv[1]
    arbeit = sys.argv[2] if len(sys.argv) > 2 else tempfile.mkdtemp(prefix="bhed_guidelta_")
    exe_alt = open(os.path.join(quelle, "jkstudio.exe"), "rb").read()
    exe_neu = exe_alt + b"REL-TEMPO neue Fassung\0"   # laeuft wie die alte (Anhang hinter dem Abbild)
    anims = open(os.path.join(quelle, "data", "base", "anims.h"), "rb").read()
    neu = {"jkstudio.exe": exe_neu, "behaved.exe": exe_neu, "data/base/anims.h": anims, "rt_delta_probe.txt": b"aus dem Delta\n",
           "version.txt": (NEU + "\n").encode()}
    manifest = ("# JK Studio manifest 1\nfassung %s\n" % NEU + "".join(
        "%s %d %s\n" % (sha(b), len(b), p) for p, b in sorted(neu.items()))).encode()
    inhalte = {sha(b): b for p, b in neu.items() if p != "data/base/anims.h"}
    delta = zip_bytes([("delta.txt", ("# JK Studio delta 1\nvon %s\nnach %s\n" % (ALT, NEU)).encode())] +
                      [("inhalt/" + s, b) for s, b in sorted(inhalte.items())])
    voll = zip_bytes([("JK-Studio/rt_voll_probe.txt", b"aus dem vollen Paket\n"), ("JK-Studio/version.txt", (NEU + "\n").encode()),
                      ("JK-Studio/jkstudio.exe", exe_neu), ("JK-Studio/behaved.exe", exe_neu)])
    dateien = {"JK-Studio-%s-from-%s.delta" % (NEU, ALT): delta, "JK-Studio-%s-manifest.txt" % NEU: manifest,
               "JK-Studio-%s-windows-x64.zip" % NEU: voll}
    dateien["SHA256SUMS.txt"] = "".join("%s  %s\n" % (sha(b), n) for n, b in sorted(dateien.items())).encode()
    stand = {"kaputt": None, "anfragen": []}

    class H(http.server.BaseHTTPRequestHandler):
        def log_message(self, *a):
            pass

        def antwort(self, b, art="application/octet-stream"):
            self.send_response(200)
            self.send_header("Content-Type", art)
            self.send_header("Content-Length", str(len(b)))
            self.end_headers()
            self.wfile.write(b)

        def do_GET(self):
            stand["anfragen"].append(self.path)
            basis = "http://127.0.0.1:%d" % self.server.server_port
            if self.path == "/repos/DennisHerrm/JK-Studio/releases/latest":
                self.antwort(json.dumps({"tag_name": TAG, "name": "JK Studio 1.0.0-" + NEU, "body": "- test", "html_url": basis,
                                         "assets": [{"name": n, "size": len(b), "url": basis + "/api/" + n,
                                                     "browser_download_url": basis + "/dl/" + n} for n, b in dateien.items()]}).encode(),
                             "application/json")
                return
            n = self.path.rsplit("/", 1)[-1]
            if self.path.startswith("/dl/") and n in dateien:
                b = dateien[n]
                if stand["kaputt"] == n:
                    b = b[:len(b) // 2] + bytes([b[len(b) // 2] ^ 0x5A]) + b[len(b) // 2 + 1:]
                self.antwort(b)
                return
            self.send_response(404)
            self.send_header("Content-Length", "0")
            self.end_headers()

    server = Server(("127.0.0.1", 0), H)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    api = "http://127.0.0.1:%d/repos/" % server.server_port
    fehler = 0

    def melde(gut, text):
        nonlocal fehler
        print(("ok    " if gut else "FEHL  ") + text)
        fehler += 0 if gut else 1

    try:
        for fall in ("delta", "voll"):
            o = os.path.join(arbeit, fall)
            shutil.rmtree(o, ignore_errors=True)
            os.makedirs(os.path.join(o, "einst"))
            shutil.copy(os.path.join(quelle, "jkstudio.exe"), o)
            shutil.copy(os.path.join(quelle, "jkstudio.exe"), os.path.join(o, "behaved.exe"))
            shutil.copytree(os.path.join(quelle, "data"), os.path.join(o, "data"))
            stand["kaputt"] = "JK-Studio-%s-from-%s.delta" % (NEU, ALT) if fall == "voll" else None
            stand["anfragen"].clear()
            env = {k: v for k, v in os.environ.items() if not k.startswith("BHED_")}
            env.update({"BHED_EDITORTEST": "update", "BHED_UPDATE_LOKAL": ALT, "BHED_UPDATE_API": api,
                        "BHED_EINSTELLUNGEN": os.path.join(o, "einst"), "BHED_KEIN_MELDUNGSFENSTER": "1"})
            # Schutz: der Selbsttest update darf nie das echte GitHub fragen - nur diesen Server auf 127.0.0.1
            if not env["BHED_UPDATE_API"].startswith("http://127.0.0.1:"):
                raise SystemExit("updater_gui_delta: BHED_UPDATE_API zeigt nicht auf 127.0.0.1")
            a = time.time()
            p = subprocess.Popen([os.path.join(o, "jkstudio.exe")], cwd=o, env=env, creationflags=0x40)   # IDLE_PRIORITY_CLASS
            try:
                p.wait(timeout=600)
            except subprocess.TimeoutExpired:
                subprocess.run(["taskkill", "/T", "/F", "/PID", str(p.pid)], capture_output=True)
                melde(False, "%s: Zeitgrenze" % fall)
            log = open(os.path.join(o, "jkstudio-detail.log"), encoding="utf-8", errors="replace").read()
            ergebnis = [z for z in log.splitlines() if "Knopftest fertig" in z]
            fe = [z for z in log.splitlines() if "Knopftest FEHLER" in z]
            print("  [%s] %.0f s, %s, Anfragen: %s" % (fall, time.time() - a, ergebnis[-1] if ergebnis else "kein Ende",
                                                       ", ".join(x.rsplit("/", 1)[-1] for x in stand["anfragen"])))
            for z in fe[:5]:
                print("    " + z[:200])
            for z in [z for z in log.splitlines() if z.startswith("Update:")][:12]:
                print("    " + z[:200])
            neu_da = open(os.path.join(o, "jkstudio.exe"), "rb").read() == exe_neu and \
                open(os.path.join(o, "behaved.exe"), "rb").read() == exe_neu
            melde(not fe and bool(ergebnis), "%s: Selbsttest update ohne FEHLER" % fall)
            if fall == "delta":
                melde(neu_da and os.path.isfile(os.path.join(o, "rt_delta_probe.txt")) and
                      open(os.path.join(o, "version.txt"), "rb").read() == (NEU + "\n").encode() and
                      os.path.isfile(os.path.join(o, "jkstudio.exe.alt")),
                      "delta: jkstudio.exe/behaved.exe neu, rt_delta_probe.txt und version.txt da, jkstudio.exe.alt die alte")
                melde(not any(x.endswith(".zip") for x in stand["anfragen"]) and any(x.endswith(".delta") for x in stand["anfragen"]),
                      "delta: nur das Delta geladen, nie das .zip")
                melde("aus dem Delta" in log, "delta: das Protokoll sagt 'aus dem Delta'")
            else:
                melde(neu_da and os.path.isfile(os.path.join(o, "rt_voll_probe.txt")) and not os.path.isfile(os.path.join(o, "rt_delta_probe.txt")),
                      "voll: Delta kaputt -> das volle Paket (rt_voll_probe.txt, neue jkstudio.exe)")
                melde(any(x.endswith(".zip") for x in stand["anfragen"]) and "das Delta ging nicht" in log,
                      "voll: das .zip geladen, das Protokoll nennt den Rueckfall")
    finally:
        server.shutdown()
    print("updater_gui_delta: %d Fehler" % fehler)
    return 1 if fehler else 0


if __name__ == "__main__":
    sys.exit(main())
