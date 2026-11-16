#!/usr/bin/env python3
"""REL-TEMPO: JK-Studio-Starter.exe gegen einen lokalen Testserver (ctest starter) - ohne Netz zu GitHub.

Der Server spielt GitHub nach: /repos/DennisHerrm/JK-Studio/releases/latest nennt die Dateien des neuesten Releases
(der Starter, das volle Paket, Manifest, Deltas, SHA256SUMS.txt), die Downloads kommen von /dl/<marke>/<name>. Die
Pakete baut der Test selbst, klein, in der Form von tools/release_paket.py (JK-Studio/ oben, die Programme zuletzt,
version.txt); Manifest und Delta mit dessen Funktionen. jkstudio.exe ist eine Attrappe (tests/starter_attrappe.cpp),
die nur aufschreibt, dass sie lief. Der Starter laeuft mit BHED_UPDATE_API (nur 127.0.0.1) und BHED_STARTER_TEST
(kein sichtbares Fenster, Rueckgabe statt Rueckfrage).

  A  leerer Ordner, kein Netz zu GitHub noetig ausser dem Testserver: das volle Paket, gestartet, nichts uebrig
  B  dieselbe Fassung: nur nachgesehen, nichts geladen, gestartet
  C  neues Release: das Delta (nicht das .zip), danach genau die neue Fassung, die weggefallene Datei weg
  D  das Delta ist kaputt: das volle Paket, danach trotzdem genau die neue Fassung
  E  ohne Server, mit Installation: die installierte Fassung startet, nichts geaendert
  F  ohne Server, ohne Installation: Rueckgabe 1, nichts installiert
  G  Abbrechen beim Laden: mit Installation bleibt sie und startet (2), ohne Installation nichts (2)
  H  der Ordner laesst sich nicht anlegen: Rueckgabe 3
  I  der Starter liegt im Programmordner selbst: er nimmt diesen Ordner

  starter_test.py <JK-Studio-Starter.exe> <starter_attrappe.exe>
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
import time
import zipfile

HIER = os.path.dirname(os.path.abspath(__file__))
WURZEL = os.path.dirname(os.path.dirname(HIER))
OBER = "JK-Studio"


def release_paket():
    spec = importlib.util.spec_from_file_location("release_paket", os.path.join(WURZEL, "tools", "release_paket.py"))
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


RP = release_paket()


def paket(rc, attrappe, extra):
    """{pfad: bytes} einer Fassung - die Attrappe als jkstudio.exe und behaved.exe."""
    exe = open(attrappe, "rb").read()
    d = {"jkstudio.exe": exe, "behaved.exe": exe, "version.txt": (rc + "\n").encode(), "data/base/behaved.bhc": b"// bhc " + rc.encode(),
         "data/base/anims.h": b"enum {BOTH_A};\r\n", "handbuch/index.html": b"<p>" + rc.encode() + b"</p>"}
    d.update(extra)
    return d


def zip_schreiben(pfad, dateien):
    rang = {"jkstudio.exe": 1, "behaved.exe": 2}
    with zipfile.ZipFile(pfad, "w", zipfile.ZIP_DEFLATED) as z:
        for p in sorted(dateien, key=lambda x: (rang.get(x, 0), x)):
            z.writestr(zipfile.ZipInfo(OBER + "/" + p, date_time=(2026, 11, 15, 12, 0, 0)), dateien[p], compress_type=zipfile.ZIP_DEFLATED)


class Server(http.server.ThreadingHTTPServer):
    daemon_threads = True


def server_starten(stand):
    """stand: {'latest': marke, 'dateien': {marke: {name: bytes}}, 'kaputt': name|None, 'langsam': bool, 'anfragen': []}"""

    class H(http.server.BaseHTTPRequestHandler):
        def log_message(self, *a):
            pass

        def senden(self, code, inhalt, art="application/octet-stream"):
            self.send_response(code)
            self.send_header("Content-Type", art)
            self.send_header("Content-Length", str(len(inhalt)))
            self.end_headers()
            try:
                if stand.get("langsam") and len(inhalt) > 4096:
                    for i in range(0, len(inhalt), 4096):
                        self.wfile.write(inhalt[i:i + 4096])
                        time.sleep(0.02)
                else:
                    self.wfile.write(inhalt)
            except (ConnectionError, OSError):
                pass

        def do_GET(self):
            stand["anfragen"].append(self.path)
            basis = "http://127.0.0.1:%d" % self.server.server_port
            if self.path == "/repos/DennisHerrm/JK-Studio/releases/latest":
                marke = stand["latest"]
                assets = [{"url": basis + "/api/assets/%d" % i, "name": n, "size": len(b),
                           "browser_download_url": basis + "/dl/%s/%s" % (marke, n)}
                          for i, (n, b) in enumerate(sorted(stand["dateien"][marke].items()))]
                text = json.dumps({"tag_name": marke, "name": "JK Studio 1.0.0-" + marke[7:], "body": "test",
                                   "html_url": "https://github.com/DennisHerrm/JK-Studio/releases/tag/" + marke,
                                   "draft": False, "prerelease": False, "assets": assets}).encode()
                self.senden(200, text, "application/json")
                return
            if self.path.startswith("/dl/"):
                _, _, marke, name = self.path.split("/", 3)
                b = stand["dateien"].get(marke, {}).get(name)
                if b is None:
                    self.senden(404, b"")
                    return
                if stand.get("kaputt") == name:
                    b = b[:len(b) // 2] + bytes([b[len(b) // 2] ^ 0x5A]) + b[len(b) // 2 + 1:]
                self.senden(200, b)
                return
            self.senden(404, b"")

    s = Server(("127.0.0.1", 0), H)
    threading.Thread(target=s.serve_forever, daemon=True).start()
    return s


def bild(ordner, ohne_protokoll=False):
    """{pfad: bytes}; mit ohne_protokoll ohne das Protokoll des Starters (starter.log, starter.log.vorher)."""
    aus = {}
    for o, _, fs in os.walk(ordner):
        for f in fs:
            if ohne_protokoll and f.startswith("starter.log"):
                continue
            p = os.path.join(o, f)
            aus[os.path.relpath(p, ordner).replace("\\", "/")] = open(p, "rb").read()
    return aus


def main():
    if len(sys.argv) != 3 or not all(os.path.isfile(a) for a in sys.argv[1:]):
        print(__doc__)
        return 2
    starter, attrappe = (os.path.abspath(a) for a in sys.argv[1:])
    fehler = 0

    def melde(gut, text):
        nonlocal fehler
        print(("ok    " if gut else "FEHL  ") + text)
        fehler += 0 if gut else 1

    groesse = os.path.getsize(starter)
    melde(groesse <= RP.STARTER_MAX, "Starter %d KB (hoechstens %d KB)" % (groesse // 1024, RP.STARTER_MAX // 1024))
    with tempfile.TemporaryDirectory(prefix="bhed_starter_") as t:
        # --- die Releases rc9001 und rc9002 -------------------------------------------------------
        v1 = paket("rc9001", attrappe, {"handbuch/alt.html": b"faellt weg", "data/base/bleibt.txt": b"gleich"})
        # rc9002 bringt dazu 1 MB, das sich nicht packen laesst - so dauert das Delta lang genug fuer G (Abbrechen)
        v2 = paket("rc9002", attrappe, {"handbuch/neu.html": b"neu in rc9002", "data/base/bleibt.txt": b"gleich",
                                        "handbuch/bilder/gross.bin": os.urandom(1 << 20)})
        dateien = {}
        for rc, d in (("rc9001", v1), ("rc9002", v2)):
            z = os.path.join(t, "pakete", RP.NAME + "-%s-windows-x64.zip" % rc)
            os.makedirs(os.path.dirname(z), exist_ok=True)
            zip_schreiben(z, d)
            man = RP.manifest_aus_zip(z)
            rel = {os.path.basename(z): open(z, "rb").read(), "JK-Studio-%s-manifest.txt" % rc: man.encode(),
                   RP.STARTER: open(starter, "rb").read()}
            if rc == "rc9002":
                dp = os.path.join(t, "pakete", "JK-Studio-rc9002-from-rc9001.delta")
                RP.delta_bauen(z, RP.manifest_aus_zip(os.path.join(t, "pakete", "JK-Studio-rc9001-windows-x64.zip")), dp)
                rel[os.path.basename(dp)] = open(dp, "rb").read()
            rel["SHA256SUMS.txt"] = "".join("%s  %s\n" % (hashlib.sha256(b).hexdigest(), n) for n, b in sorted(rel.items())).encode()
            dateien["v1.0.0-" + rc] = rel
        print("Delta rc9001 -> rc9002: %d Bytes, volles Paket %d Bytes" %
              (len(dateien["v1.0.0-rc9002"]["JK-Studio-rc9002-from-rc9001.delta"]), len(dateien["v1.0.0-rc9002"]["JK-Studio-rc9002-windows-x64.zip"])))
        stand = {"latest": "v1.0.0-rc9001", "dateien": dateien, "kaputt": None, "langsam": False, "anfragen": []}
        server = server_starten(stand)
        api = "http://127.0.0.1:%d/repos/" % server.server_port

        def lauf(ordner, name, extra=None, api_=None):
            """Startet den Starter in `ordner`, wartet auf sein Ende; Rueckgabe (code, protokoll, gestartet-Zeilen)."""
            os.makedirs(ordner, exist_ok=True)
            exe = os.path.join(ordner, RP.STARTER)
            if not os.path.isfile(exe):
                shutil.copyfile(starter, exe)
            prot = os.path.join(t, "prot_%s.txt" % name)
            env = dict(os.environ)
            for k in list(env):
                if k.startswith("BHED_"):
                    env.pop(k)
            env.update({"BHED_UPDATE_API": api_ or api, "BHED_STARTER_TEST": prot, "BHED_STARTER_SPRACHE": "en"})
            env.update(extra or {})
            # Schutz: ein Testaufruf fragt nie das echte GitHub (der Starter nimmt ausser 127.0.0.1 nur api.github.com)
            if not env["BHED_UPDATE_API"].startswith("http://127.0.0.1:"):
                raise SystemExit("starter_test: BHED_UPDATE_API zeigt nicht auf 127.0.0.1")
            stand["anfragen"].clear()
            gestartet_vorher = set()
            for o, _, fs in os.walk(ordner):
                if "gestartet.txt" in fs:
                    gestartet_vorher.add(os.path.join(o, "gestartet.txt"))
            r = subprocess.run([exe], env=env, timeout=120, capture_output=True)
            text = open(prot, encoding="utf-8", errors="replace").read() if os.path.isfile(prot) else ""
            # die Attrappe laeuft nach dem Starter weiter: auf ihre Zeile warten
            zeilen = []
            if "gestartet " in text:
                for _ in range(100):
                    zeilen = []
                    for o, _, fs in os.walk(ordner):
                        if "gestartet.txt" in fs:
                            zeilen += open(os.path.join(o, "gestartet.txt"), encoding="utf-8").read().splitlines()
                    if zeilen:
                        break
                    time.sleep(0.05)
            for p in gestartet_vorher | {os.path.join(o, "gestartet.txt") for o, _, fs in os.walk(ordner) if "gestartet.txt" in fs}:
                if os.path.exists(p):
                    os.remove(p)
            print("  [%s] Rueckgabe %d, Anfragen: %s" % (name, r.returncode, ", ".join(a.rsplit("/", 1)[-1] for a in stand["anfragen"])))
            for z in text.splitlines():
                print("      " + z)
            return r.returncode, text, zeilen, list(stand["anfragen"])

        def installiert_wie(ordner, d):
            b = bild(ordner)
            fremd = {k for k in b if k not in d and k not in ("jkstudio.exe.alt", "gestartet.txt") and not k.startswith("starter.log")}
            falsch = [k for k in d if b.get(k) != d[k]]
            return not fremd and not falsch, fremd, falsch

        def keine_reste(ordner):
            return not os.path.exists(os.path.join(ordner, "jkstudio-update.tmp"))

        try:
            # --- A: erster Start ------------------------------------------------------------------
            a = os.path.join(t, "A")
            code, text, gest, anfr = lauf(a, "A")
            ok, fremd, falsch = installiert_wie(os.path.join(a, OBER), v1)
            melde(code == 0 and ok, "A: leerer Ordner -> %s\\ hat genau rc9001 (fremd %s, falsch %s)" % (OBER, sorted(fremd), falsch))
            melde(any(x.endswith(".zip") for x in anfr) and not any(x.endswith(".delta") for x in anfr) and
                  any(x.endswith("SHA256SUMS.txt") for x in anfr), "A: das volle Paket und SHA256SUMS.txt geladen, kein Delta")
            melde(gest == ["gestartet rc9001 argumente 0 arbeitsordner_gleich 1"], "A: jkstudio.exe gestartet, im Programmordner (%s)" % gest)
            melde(keine_reste(os.path.join(a, OBER)), "A: kein Zwischenordner")
            # --- B: dieselbe Fassung --------------------------------------------------------------
            code, text, gest, anfr = lauf(a, "B")
            melde(code == 0 and anfr == ["/repos/DennisHerrm/JK-Studio/releases/latest"] and gest[:1] == ["gestartet rc9001 argumente 0 arbeitsordner_gleich 1"],
                  "B: aktuell -> nur nachgesehen, nichts geladen, gestartet")
            # --- C: neues Release, Delta ---------------------------------------------------------
            sicherung = os.path.join(t, "A_rc9001")
            shutil.copytree(a, sicherung)
            stand["latest"] = "v1.0.0-rc9002"
            code, text, gest, anfr = lauf(a, "C")
            ok, fremd, falsch = installiert_wie(os.path.join(a, OBER), v2)
            melde(code == 0 and ok and not os.path.exists(os.path.join(a, OBER, "handbuch", "alt.html")),
                  "C: rc9001 -> rc9002 genau, handbuch/alt.html weg (fremd %s, falsch %s)" % (sorted(fremd), falsch))
            melde(any(x.endswith("from-rc9001.delta") for x in anfr) and not any(x.endswith(".zip") for x in anfr) and "Delta" in text,
                  "C: nur das Delta geladen (kein .zip)")
            melde(gest == ["gestartet rc9002 argumente 0 arbeitsordner_gleich 1"], "C: die neue Fassung gestartet")
            # --- D: kaputtes Delta -> volles Paket ---------------------------------------------------
            shutil.rmtree(a)
            shutil.copytree(sicherung, a)
            stand["kaputt"] = "JK-Studio-rc9002-from-rc9001.delta"
            code, text, gest, anfr = lauf(a, "D")
            stand["kaputt"] = None
            ok, fremd, falsch = installiert_wie(os.path.join(a, OBER), dict(v2, **{"handbuch/alt.html": b"faellt weg"}))
            melde(code == 0 and ok and "Delta ging nicht" in text and any(x.endswith(".zip") for x in anfr),
                  "D: Delta kaputt -> das volle Paket, danach rc9002 (fremd %s, falsch %s)" % (sorted(fremd), falsch))
            # --- E: ohne Server, mit Installation -----------------------------------------------------
            vorher = bild(os.path.join(a, OBER), True)
            tot = "http://127.0.0.1:9/repos/"   # Port 9 (discard): niemand antwortet
            code, text, gest, anfr = lauf(a, "E", api_=tot)
            nachher = bild(os.path.join(a, OBER), True)
            melde(code == 0 and gest[:1] == ["gestartet rc9002 argumente 0 arbeitsordner_gleich 1"] and "offline" in text and
                  nachher == vorher,
                  "E: ohne Verbindung -> die installierte Fassung startet, nichts geaendert")
            # --- F: ohne Server, ohne Installation ----------------------------------------------------
            f = os.path.join(t, "F")
            code, text, gest, anfr = lauf(f, "F", api_=tot)
            melde(code == 1 and not gest and not os.path.exists(os.path.join(f, OBER, "jkstudio.exe")),
                  "F: ohne Verbindung und ohne Installation -> Rueckgabe 1, nichts installiert")
            # --- G: Abbrechen -------------------------------------------------------------------------
            g = os.path.join(t, "G")
            shutil.copytree(sicherung, g)
            stand["langsam"] = True
            vorher = bild(os.path.join(g, OBER), True)
            code, text, gest, anfr = lauf(g, "G", extra={"BHED_STARTER_ABBRUCH_NACH": "8192"})
            nachher = bild(os.path.join(g, OBER), True)
            melde(code == 2 and gest[:1] == ["gestartet rc9001 argumente 0 arbeitsordner_gleich 1"] and nachher == vorher and
                  not any(x.endswith(".zip") or x.endswith("SHA256SUMS.txt") for x in anfr),
                  "G: Abbrechen beim Update -> nichts geaendert, die installierte Fassung startet (Rueckgabe 2)")
            g2 = os.path.join(t, "G2")
            code, text, gest, anfr = lauf(g2, "G2", extra={"BHED_STARTER_ABBRUCH_NACH": "8192"})
            stand["langsam"] = False
            melde(code == 2 and not gest and not os.path.exists(os.path.join(g2, OBER, "jkstudio.exe")) and
                  not os.path.exists(os.path.join(g2, OBER, "jkstudio-update.tmp")),
                  "G: Abbrechen beim ersten Laden -> nichts installiert (Rueckgabe 2)")
            # --- H: Ordner laesst sich nicht anlegen (an seiner Stelle liegt eine Datei) ----------------
            h = os.path.join(t, "H")
            os.makedirs(h)
            open(os.path.join(h, OBER), "wb").write(b"eine Datei")
            code, text, gest, anfr = lauf(h, "H")
            melde(code == 3 and not anfr and open(os.path.join(h, OBER), "rb").read() == b"eine Datei",
                  "H: Ordner nicht beschreibbar -> Rueckgabe 3, nichts geladen, nichts geaendert")
            # --- I: Starter im Programmordner -----------------------------------------------------------
            i = os.path.join(t, "I")
            shutil.copytree(os.path.join(sicherung, OBER), i)
            stand["latest"] = "v1.0.0-rc9002"
            code, text, gest, anfr = lauf(i, "I")
            ok, fremd, falsch = installiert_wie(i, dict(v2, **{RP.STARTER: open(starter, "rb").read()}))
            melde(code == 0 and ok and not os.path.exists(os.path.join(i, OBER)),
                  "I: Starter neben jkstudio.exe -> dieser Ordner wird aktuell, kein Unterordner (fremd %s, falsch %s)" % (sorted(fremd), falsch))
        finally:
            server.shutdown()
    print("starter_test: %d Fehler" % fehler)
    return 1 if fehler else 0


if __name__ == "__main__":
    sys.exit(main())
