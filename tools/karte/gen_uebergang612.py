#!/usr/bin/env python3
"""REL-TEMPO: erzeugt tests/uebergang/alt612_*.inc - den Updater von rc612 (git 348f0e345) fuer tests/uebergangtest612.cpp.

Die installierten Fassungen rc606 bis rc612 tragen den Updater der Stufe 8d (Pruefung gegen SHA256SUMS.txt, Signatur, das
Einspielen ueber den Zwischenordner; rc610 F78 und REST9/10 aenderten nur Ablauf und Oberflaeche). Er nimmt das ERSTE .zip
des Releases. REL-TEMPO legt neben das volle Paket den Starter, das Manifest und Deltas (ohne .zip-Endung) - dieser Test
zeigt, dass rc612 trotzdem das volle Paket findet, prueft und einspielt (ctest updater_uebergang, tools/karte/updater_uebergang.py).

Wie tests/uebergang/alt_*.inc (rc604): der Code Zeile fuer Zeile aus git, ersetzt nur, was der Test umlenken muss (je
Ersetzung steht im Kopf der erzeugten Datei; fehlt eine Stelle, bricht der Generator ab).

  python tools/karte/gen_uebergang612.py           -> tests/uebergang/alt612_update.inc, alt612_einspielen.inc, alt612_install.inc
  python tools/karte/gen_uebergang612.py --check   nur pruefen, ob die Dateien zum Commit passen
"""
import os
import re
import shutil
import subprocess
import sys

HIER = os.path.dirname(os.path.abspath(__file__))
WURZEL = os.path.dirname(os.path.dirname(HIER))
COMMIT = "348f0e345"
ZIEL = os.path.join(WURZEL, "tests", "uebergang")


def git_show(pfad):
    git = shutil.which("git")
    if git is None:
        raise SystemExit("gen_uebergang612: git nicht gefunden")
    r = subprocess.run([git, "-C", WURZEL, "show", "%s:%s" % (COMMIT, pfad)], capture_output=True)
    if r.returncode != 0:
        raise SystemExit("gen_uebergang612: git show %s:%s - %s" % (COMMIT, pfad, r.stderr.decode("utf-8", "replace").strip()))
    return r.stdout.decode("utf-8").replace("\r\n", "\n")


def ersetze(text, alt, neu, wie_oft=1):
    n = text.count(alt)
    if n != wie_oft:
        raise SystemExit("gen_uebergang612: %r kommt %d-mal vor (erwartet %d)" % (alt[:60], n, wie_oft))
    return text.replace(alt, neu)


def ohne_includes(text):
    return "".join(z for z in text.splitlines(True) if not z.startswith("#include"))


def stueck(text, anfang, ende):
    a = text.find(anfang)
    e = text.find(ende, a)
    if a < 0 or e < 0:
        raise SystemExit("gen_uebergang612: Stueck %r ... %r nicht gefunden" % (anfang[:40], ende[:40]))
    return text[a:e]


def kopf(datei, quelle, ersetzungen):
    z = ["// %s - ERZEUGT von tools/karte/gen_uebergang612.py aus %s des Updaters von rc612 (git %s; so steckt er in" % (datei, quelle, COMMIT),
         "// den installierten Fassungen rc606 bis rc612). Nur fuer tests/uebergangtest612.cpp (REL-TEMPO: alte Updater und das",
         "// neue Release mit Starter, Manifest und Deltas). Ersetzt wurde nur, was der Test umlenken muss - alles andere ist",
         "// Zeile fuer Zeile das Original:"]
    z += ["//   - " + e for e in ersetzungen]
    return "\n".join(z) + "\n"


def erzeugen():
    aus = {}
    # 1. src/update.cpp -> namespace bhed::alt612_update
    t = ohne_includes(git_show("src/update.cpp"))
    # in bhed: wie im Original findet der Code Pk3, diag und readPk3File ueber den umgebenden Namensraum
    t = ersetze(t, "namespace bhed::update {\n", "namespace bhed::alt612_update {\nusing namespace ::bhed::update;   // die Typen (Release, Asset, Einspielen)\n")
    t = ersetze(t, "}  // namespace bhed::update", "}  // namespace bhed::alt612_update")
    aus["alt612_update.inc"] = kopf("alt612_update.inc", "src/update.cpp",
                                    ['"namespace bhed::update" -> "namespace bhed::alt612_update" (die Typen aus bhed/update.h)',
                                     "die #include (stehen in uebergangtest612.cpp)"]) + t
    # 2. src/update_einspielen.cpp -> namespace bhed::alt612_update
    t = ohne_includes(git_show("src/update_einspielen.cpp"))
    t = ersetze(t, "namespace bhed::update {\n", "namespace bhed::alt612_update {\nusing namespace ::bhed::update;\n")
    t = ersetze(t, "}  // namespace bhed::update", "}  // namespace bhed::alt612_update")
    # klein() steht wortgleich schon in alt612_update.inc - im Original zwei Uebersetzungseinheiten, hier eine
    t = ersetze(t, '''std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}
''', "")
    aus["alt612_einspielen.inc"] = kopf("alt612_einspielen.inc", "src/update_einspielen.cpp",
                                        ['"namespace bhed::update" -> "namespace bhed::alt612_update"',
                                         "klein() entfaellt: wortgleich in alt612_update.inc (dort eine Uebersetzungseinheit mit dieser)",
                                         "die #include (stehen in uebergangtest612.cpp)"]) + t
    # 3. gui/update_win32.cpp: Geteilt ... installAuftrag
    g = git_show("gui/update_win32.cpp")
    t = stueck(g, "// Absichtlich nie freigegeben: der Hintergrundfaden kann beim Beenden noch", "// Erst den Platz belegen, dann den Zustand setzen")
    t = re.sub(r"std::wstring exePfad\(\) \{\n.*?\n\}\n", "std::wstring exePfad() { return g_simExe; }   // ERSETZT: die simulierte laufende .exe\n", t,
               count=1, flags=re.S)
    if "return g_simExe;" not in t:
        raise SystemExit("gen_uebergang612: exePfad nicht ersetzt")
    a = t.find("// --- Der Schluessel fuer ein privates Repository")
    e = t.find("// --- HTTP ---")
    if a < 0 or e < 0:
        raise SystemExit("gen_uebergang612: schluessel() nicht gefunden")
    t = t[:a] + "// ERSETZT: kein Schluessel (der Test braucht keine Anmeldung)\nstd::string schluessel() { return {}; }\n\n" + t[e:]
    a = t.find("bool prozessLaeuft(const std::wstring& name) {")
    e = t.find("// --- Die beiden Hintergrundauftraege")
    if a < 0 or e < 0:
        raise SystemExit("gen_uebergang612: prozessLaeuft nicht gefunden")
    t = t[:a] + t[e:]   # nur beimStart braucht es (nicht im Test)
    t = re.sub(r"fs::path ablageOrdner\(\) \{\n.*?\n\}\n", "fs::path ablageOrdner() { return g_simTemp; }   // ERSETZT: die Ablage des Tests\n", t,
               count=1, flags=re.S)
    if "return g_simTemp;" not in t:
        raise SystemExit("gen_uebergang612: ablageOrdner nicht ersetzt")
    t = ersetze(t, 'L"https://api.github.com/repos/" + breit(bhed::update::kRepo)', "g_simApi + breit(bhed::update::kRepo)")
    t = ersetze(t, "breit(bhed::kFassung)", "breit(kAlteFassung)")
    t = t.replace("bhed::update::", "alt612_update::")
    aus["alt612_install.inc"] = kopf("alt612_install.inc", "gui/update_win32.cpp (Geteilt ... installAuftrag)",
                                     ["exePfad() -> g_simExe (die laufende .exe der simulierten Installation)",
                                      "schluessel() -> leer (der Test braucht keine Anmeldung), ausgabeVon entfaellt mit ihm",
                                      "prozessLaeuft entfaellt (nur beimStart, nicht im Test)",
                                      "ablageOrdner() (%TEMP%\\\\behaved_update) -> g_simTemp",
                                      '"https://api.github.com/repos/" -> g_simApi (der lokale Testserver)',
                                      "bhed::kFassung -> kAlteFassung (\"rc612\")",
                                      "bhed::update:: -> alt612_update:: (die Funktionen von rc612)"]) + t
    return aus


def main():
    aus = erzeugen()
    if "--check" in sys.argv:
        falsch = [n for n, t in aus.items()
                  if not os.path.isfile(os.path.join(ZIEL, n)) or open(os.path.join(ZIEL, n), encoding="utf-8", newline="").read() != t]
        if falsch:
            print("nicht aktuell: %s - python tools/karte/gen_uebergang612.py" % ", ".join(falsch))
            return 1
        print("tests/uebergang/alt612_*.inc passen zu %s" % COMMIT)
        return 0
    for n, t in aus.items():
        with open(os.path.join(ZIEL, n), "w", encoding="utf-8", newline="\n") as f:
            f.write(t)
        print("%s: %d Zeilen" % (n, t.count("\n")))
    return 0


if __name__ == "__main__":
    sys.exit(main())
