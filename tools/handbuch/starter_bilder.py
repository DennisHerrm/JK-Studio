#!/usr/bin/env python3
"""REL-TEMPO: die Handbuchbilder des Starters (Seite installation) - h_starter_laden und h_starter_delta in allen Sprachen.

Der Starter laeuft ausserhalb des Schirms im Fotomodus (BHED_STARTER_FOTO, BHED_STARTER_FOTO_BEI=laden:40,
BHED_STARTER_SKALA=4: 480 Punkte breit -> 1920 Pixel) gegen lokale Testserver (BHED_UPDATE_API, nur 127.0.0.1), die
ein Release nachbilden: das volle Paket so gross wie das von rc612 (470.475.290 Bytes), das Delta so gross wie das gemessene
rc611 -> rc612 (DELTA_BYTES). Die Bytes sind Fuellung - bei 40 % macht der Starter das Foto und bricht ab, er installiert und
startet nichts. Kein Pfad im Bild: waehrend des Ladens zeigt der Starter nur die Megabyte.

  h_starter_laden  erster Start in einem leeren Ordner: das volle Paket von rc613 (dem ersten Release mit Starter)
  h_starter_delta  installiert ist rc613 (version.txt): das Delta auf rc614. REL613: rc612 und aelter haben kein Manifest
                   (und keine version.txt) - ein Delta rc612 -> rc613 gibt es nicht, das erste Delta kommt mit rc614.

  python tools/handbuch/starter_bilder.py <JK-Studio-Starter.exe> [<handbuch/bilder>] [--sprachen en,de]
"""
import http.server
import json
import os
import shutil
import subprocess
import sys
import tempfile
import threading

from PIL import Image

HIER = os.path.dirname(os.path.abspath(__file__))
WURZEL = os.path.dirname(os.path.dirname(HIER))
SPRACHEN = ["en", "de", "zh", "ja", "es", "cs"]
ZIP_BYTES = 470475290       # das volle Paket von rc612 (Y:\werkzeuge\rel612\dl, 14.11.)
DELTA_BYTES = 137384236     # das Delta rc611 -> rc612 (gemessen 16.11.) (tools/release_paket.py delta, docs/intern/NOTIZEN_reltempo_1511.md)
STUECK = b"\0" * (1 << 20)


class Server(http.server.ThreadingHTTPServer):
    daemon_threads = True


def server_starten(neu, alt):
    """Testserver fuer ein Release v1.0.0-<neu> mit einem Delta von <alt>."""
    class H(http.server.BaseHTTPRequestHandler):
        def log_message(self, *a):
            pass

        def do_GET(self):
            basis = "http://127.0.0.1:%d" % self.server.server_port
            marke = "v1.0.0-" + neu
            dateien = {"JK-Studio-Starter.exe": 700000, "JK-Studio-%s-windows-x64.zip" % neu: ZIP_BYTES,
                       "JK-Studio-%s-manifest.txt" % neu: 600000, "JK-Studio-%s-from-%s.delta" % (neu, alt): DELTA_BYTES,
                       "SHA256SUMS.txt": 400}
            if self.path == "/repos/DennisHerrm/JK-Studio/releases/latest":
                text = json.dumps({"tag_name": marke, "name": "JK Studio 1.0.0-" + neu, "body": "", "html_url": basis,
                                   "assets": [{"name": n, "size": g, "url": basis + "/a/" + n, "browser_download_url": basis + "/dl/" + n}
                                              for n, g in dateien.items()]}).encode()
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(text)))
                self.end_headers()
                self.wfile.write(text)
                return
            name = self.path.rsplit("/", 1)[-1]
            if not self.path.startswith("/dl/") or name not in dateien:
                self.send_response(404)
                self.send_header("Content-Length", "0")
                self.end_headers()
                return
            self.send_response(200)
            self.send_header("Content-Type", "application/octet-stream")
            self.send_header("Content-Length", str(dateien[name]))
            self.end_headers()
            rest = dateien[name]
            try:
                while rest > 0:
                    n = min(rest, len(STUECK))
                    self.wfile.write(STUECK[:n])
                    rest -= n
            except (ConnectionError, OSError):
                pass   # der Starter bricht nach dem Foto ab

    s = Server(("127.0.0.1", 0), H)
    threading.Thread(target=s.serve_forever, daemon=True).start()
    return s


def foto(starter, ordner, sprache, bmp, api):
    env = {k: v for k, v in os.environ.items() if not k.startswith("BHED_")}
    env.update({"BHED_UPDATE_API": api, "BHED_STARTER_SPRACHE": sprache, "BHED_STARTER_SKALA": "4",
                "BHED_STARTER_FOTO": bmp, "BHED_STARTER_FOTO_BEI": "laden:40"})
    # Schutz: Fotos nur gegen den eigenen Testserver, nie gegen das echte GitHub
    if not env["BHED_UPDATE_API"].startswith("http://127.0.0.1:"):
        raise SystemExit("starter_bilder: BHED_UPDATE_API zeigt nicht auf 127.0.0.1")
    exe =os.path.join(ordner, "JK-Studio-Starter.exe")
    shutil.copyfile(starter, exe)
    subprocess.run([exe], env=env, timeout=120, creationflags=0x40)   # IDLE_PRIORITY_CLASS
    return os.path.isfile(bmp)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    sprachen = SPRACHEN
    for a in sys.argv[1:]:
        if a.startswith("--sprachen="):
            sprachen = a.split("=", 1)[1].split(",")
    if not args or not os.path.isfile(args[0]):
        print(__doc__)
        return 2
    starter = os.path.abspath(args[0])
    ziel = args[1] if len(args) > 1 else os.path.join(WURZEL, "handbuch", "bilder")
    # erster Start: rc613 voll; Update: installiert rc613, das Release rc614 bringt das Delta von rc613
    server = {"h_starter_laden": server_starten("rc613", "rc612"), "h_starter_delta": server_starten("rc614", "rc613")}
    installiert = {"h_starter_laden": None, "h_starter_delta": "rc613"}
    fehler = 0
    try:
        with tempfile.TemporaryDirectory(prefix="bhed_starterbilder_") as t:
            for sp in sprachen:
                for name in ("h_starter_laden", "h_starter_delta"):
                    api = "http://127.0.0.1:%d/repos/" % server[name].server_port
                    o = os.path.join(t, sp, name)
                    os.makedirs(o)
                    if installiert[name]:
                        prog = os.path.join(o, "JK-Studio")
                        os.makedirs(prog)
                        open(os.path.join(prog, "jkstudio.exe"), "wb").write(b"MZ")
                        open(os.path.join(prog, "version.txt"), "w", newline="\n").write(installiert[name] + "\n")
                    bmp = os.path.join(t, "%s_%s.bmp" % (sp, name))
                    if not foto(starter, o, sp, bmp, api):
                        print("FEHL  %s %s: kein Foto" % (sp, name))
                        fehler += 1
                        continue
                    b = Image.open(bmp).convert("RGB")
                    os.makedirs(os.path.join(ziel, sp), exist_ok=True)
                    b.save(os.path.join(ziel, sp, name + ".webp"), "WEBP", quality=84, method=6)
                    print("ok    %s/%s.webp %dx%d" % (sp, name, b.width, b.height))
                    if b.width < 1920:
                        print("FEHL  %s %s: nur %d Pixel breit" % (sp, name, b.width))
                        fehler += 1
    finally:
        for s in server.values():
            s.shutdown()
    print("starter_bilder: %d Fehler" % fehler)
    return 1 if fehler else 0


if __name__ == "__main__":
    sys.exit(main())
