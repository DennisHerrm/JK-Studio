"""Prueft die Suche des Handbuchs (assets/handbuch.js) in einem echten Browser.

Aufruf:  python tools/handbuch/suche_pruefen.py [handbuch.js]

Die Suche faltet Gross/Klein und Akzente ("animacion" findet "animación",
"cestina" findet "čeština", "uber" findet "über"), laesst japanische Dakuten
stehen und markiert die Treffer im Original. Das laeuft nur im Browser
(String.normalize); darum oeffnet dieses Skript eine Probeseite mit dem echten
handbuch.js und einem kleinen Suchindex in Edge oder Chrome ohne Fenster
(--headless, eigener Wegwerf-Profilordner, per CreateProcess) und liest das
Ergebnis aus dem DOM. Ohne Browser: "uebersprungen" (Erfolg).
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

HIER = os.path.dirname(os.path.abspath(__file__))
BROWSER = [r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
           r"C:\Program Files\Microsoft\Edge\Application\msedge.exe",
           r"C:\Program Files\Google\Chrome\Application\chrome.exe"]

# (Suche, erwartete Seite des ersten Treffers, erwartete Markierung im Original)
FAELLE = [
    ("animacion", "es1", "Animación"),
    ("ANIMACIÓN", "es1", "Animación"),
    ("cestina", "cs1", "Čeština"),
    ("pruvodce", "cs1", "průvodce"),
    ("uber", "de1", "Über"),
    ("ñandu", "es2", "Ñandú"),
    ("ガイド", "ja1", "ガイド"),
]

SEITE = """<!doctype html><html><head><meta charset="utf-8"></head><body>
<input id="suchfeld"><div id="treffer" hidden></div><article><p>x</p></article>
<pre id="ergebnis"></pre>
<script>
window.HANDBUCH_INDEX = [
  {"p": "es1.html", "a": "", "s": "Animación de la cámara", "t": "Guía", "x": "La animación mueve la cámara."},
  {"p": "es2.html", "a": "", "s": "Ñandú", "t": "Guía", "x": "Un ave."},
  {"p": "cs1.html", "a": "", "s": "Čeština", "t": "Nápověda", "x": "Jazyk průvodce."},
  {"p": "de1.html", "a": "", "s": "Über das Programm", "t": "Handbuch", "x": "Fassung und Lizenz."},
  {"p": "ja1.html", "a": "", "s": "ガイド", "t": "ヘルプ", "x": "ガイドの使い方。"}
];
window.HANDBUCH_TEXTE = {"keine_treffer": "-", "treffer_auf": "en"};
</script>
<script src="handbuch.js"></script>
<script>
var faelle = %s, aus = [];
var feld = document.getElementById("suchfeld");
faelle.forEach(function (f) {
  feld.value = f[0];
  feld.dispatchEvent(new Event("input"));
  var a = document.querySelector("#treffer a");
  var seite = a ? a.getAttribute("href").split("?")[0] : "-";
  var mark = a ? a.querySelector("mark") : null;
  aus.push(f[0] + "|" + seite + "|" + (mark ? mark.textContent : "-"));
});
document.getElementById("ergebnis").textContent = "ERGEBNIS[" + aus.join(";") + "]";
</script></body></html>
"""


def main():
    browser = next((b for b in BROWSER if os.path.exists(b)), None)
    if browser is None:
        print("handbuch_suche: uebersprungen (kein Edge/Chrome)")
        return 0
    ordner = tempfile.mkdtemp(prefix="bhed_suche_")
    try:
        quelle = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HIER, "assets", "handbuch.js")
        shutil.copyfile(quelle, os.path.join(ordner, "handbuch.js"))
        import json
        with open(os.path.join(ordner, "probe.html"), "w", encoding="utf-8", newline="\n") as f:
            f.write(SEITE % json.dumps([list(x) for x in FAELLE], ensure_ascii=False))
        profil = os.path.join(ordner, "profil")
        url = "file:///" + os.path.join(ordner, "probe.html").replace("\\", "/")
        lauf = subprocess.Popen([browser, "--headless", "--disable-gpu", "--no-first-run", "--no-default-browser-check",
                                 "--disable-extensions", "--user-data-dir=" + profil, "--allow-file-access-from-files",
                                 "--dump-dom", url], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
        try:
            dom, _ = lauf.communicate(timeout=60)
        except subprocess.TimeoutExpired:
            subprocess.run(["taskkill", "/PID", str(lauf.pid), "/T", "/F"], capture_output=True)
            print("handbuch_suche: FEHLER (Browser antwortet nicht)")
            return 1
        m = re.search(r"ERGEBNIS\[(.*?)\]", dom.decode("utf-8", "replace"))
        if m is None:
            print("handbuch_suche: FEHLER (kein Ergebnis im DOM)")
            return 1
        ist = {z.split("|")[0]: z.split("|")[1:] for z in m.group(1).split(";")}
        fehler = 0
        for frage, seite, mark in FAELLE:
            ok = ist.get(frage) == [seite + ".html", mark]
            fehler += 0 if ok else 1
            print("  %s  %s -> %s" % ("ok  " if ok else "FEHL", frage.encode("ascii", "replace").decode(),
                                      "|".join(ist.get(frage, ["-"])).encode("ascii", "replace").decode()))
        print("handbuch_suche: " + ("ok" if fehler == 0 else "FEHLER (%d)" % fehler))
        return 1 if fehler else 0
    finally:
        shutil.rmtree(ordner, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
