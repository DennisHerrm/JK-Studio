#!/usr/bin/env python3
"""Welche Selbsttest-Bereiche muessen nach einer Aenderung laufen? (TESTTEMPO, rc612)

    python -I tools/test_betroffen.py <basis>          aus git diff <basis>..HEAD (und dem Arbeitsbaum)
    python -I tools/test_betroffen.py --dateien a b    fuer diese Dateien
    python -I tools/test_betroffen.py --repo <pfad> <basis>   fuer einen anderen Arbeitsbaum

Nennt die Bereiche (Stichworte der Tabelle in tools/testrunde.py) und ob Bau und ctest noetig sind.
Vorsichtig: eine Datei, die keine Regel kennt, heisst ALLES. `tools/testrunde.py --betroffen <basis>`
faehrt genau diese Bereiche (Regel "Schlussrunde je Zweig": nur, was der geaenderte Code beruehrt).
"""

from __future__ import annotations

import fnmatch
import os
import re
import shutil
import subprocess
import sys

ALLES = None
BEREICHE = ("programm", "vorschau", "karteneditor", "menue", "modell", "dock", "update", "handbuch")

# Selbsttest-Dateien gui/selbsttest_<x>.inc -> Bereich (Anfang des Namens)
SELBSTTEST = (
    ("menue", "menue"),
    (("karte", "hilfe1", "hilfe2", "tutgeo", "kreste", "optsicher", "modobj", "r2a", "r3", "aseb", "kemd", "lumpedit", "patchlicht",
      "texturliste", "bspmerge", "editorportal"), "karteneditor"),
    (("mb", "ph", "rt", "ai", "uv", "md", "pt", "pfix", "mr", "modell1"), "modell"),
    (("dock", "tabfenster"), "dock"),
    ("rest9", "update"),
    ("handbuch", "handbuch"),
    (("programm", "werkzeuge", "pruefung3"), "programm"),
    (("vorschau", "mission1", "teil", "geschuetze", "zeitleiste", "weg", "kampfpunkte", "deckung", "enttext", "dateiort", "modellsetzen", "leistung"),
     "vorschau"),
    # zusammengefuehrter Stand rc612: die Selbsttests der Zweige neben TESTTEMPO (mbkopf, mbraster, mbnav, mbv, mbefoto beginnen mit "mb")
    (("ap1", "ap2", "ap12", "ap3"), "modell"),
    ("ap9", "modell"),
    ("ap7", "modell"),
    ("ap6", "modell"),
    ("ap8", "modell"),
    ("ap10", "modell"),
    ("ap4", "modell"),
    ("ap5", "modell"),
    ("numpad", "karteneditor"),
    ("uiform", "programm"),
    ("gdpfad", "programm"),   # GDPFAD: das Programm ohne den eingerichteten Spielordner (Lauf spielordnerfehlt)
)

# Muster (fnmatch auf den Pfad mit /) -> Bereiche; die erste passende Regel gilt. () = kein Selbsttest noetig.
REGELN: tuple[tuple[str, tuple[str, ...] | None], ...] = (
    ("docs/*", ()), ("*.md", ()), ("LICENSE", ()), ("licenses/*", ()), (".gitignore", ()), (".github/*", ()),
    ("tests/*", ()),                                   # nur ctest
    ("tools/*", ()),                                   # Werkzeuge und Lints: ctest; erzeugte Quellen stehen selbst im Unterschied
    ("handbuch/*", ("handbuch",)),
    ("gui/selbsttest.cpp", ALLES),
    ("gui/app_kartemission.cpp", ("karteneditor", "vorschau")), ("src/kartemission.cpp", ("karteneditor", "vorschau")),
    ("gui/app_karte*", ("karteneditor",)), ("gui/karte*", ("karteneditor",)), ("gui/qttext_win32.cpp", ("karteneditor",)),
    ("gui/app_lumpedit.cpp", ("karteneditor",)), ("src/karte/*", ("karteneditor",)), ("include/bhed/karte/*", ("karteneditor",)),
    ("src/q3map2/*", ("karteneditor",)), ("third_party/q3map2/*", ("karteneditor",)),
    ("gui/app_menue.cpp", ("menue",)), ("gui/menueeditor.h", ("menue",)), ("gui/videoimport*", ("menue",)), ("src/menue/*", ("menue",)),
    ("include/bhed/menue/*", ("menue",)),
    ("gui/app_modelltab*", ("modell",)), ("gui/modelltab.h", ("modell",)), ("gui/wof/*", ("modell",)), ("src/modell/*", ("modell",)),
    ("gui/ui_rollout.*", ("modell",)),                 # AP3: der Rollout-Baustein - benutzt vom rechten Feld des Model-Tabs
    ("include/bhed/modell/*", ("modell",)),
    ("gui/fenster_dock.*", ("dock",)), ("gui/dockvorgabe.*", ("dock",)), ("gui/tabfenster.*", ("dock",)),
    ("gui/update*", ("update",)), ("src/update*", ("update",)), ("src/umzug.cpp", ("update",)),
    # der Mission-Tab; das Spielbild des Karten-Editors laeuft ueber denselben Vorschau-Code
    ("gui/app_view3d.cpp", ("vorschau", "karteneditor")), ("gui/app_mission.cpp", ("vorschau",)), ("gui/app_zeitleiste.cpp", ("vorschau",)),
    ("gui/app_wegpunkte.cpp", ("vorschau",)), ("gui/app_kamera.cpp", ("vorschau",)), ("gui/app_bewegung.cpp", ("vorschau",)),
    ("gui/app_saber.cpp", ("vorschau",)), ("gui/app_ton.cpp", ("vorschau",)), ("gui/app_wetter.cpp", ("vorschau", "karteneditor")),
    ("gui/app_gras.cpp", ("vorschau", "karteneditor")), ("gui/app_figuranim.cpp", ("vorschau", "modell")),
    ("gui/app_animation.cpp", ("vorschau",)), ("gui/app_deckung.cpp", ("vorschau",)), ("gui/app_enttext.cpp", ("vorschau",)),
    ("gui/app_dateiort.cpp", ("vorschau",)), ("gui/app_portalsicht.cpp", ("vorschau", "karteneditor")), ("gui/app_regelwerk.cpp", ("vorschau",)),
    ("gui/app_modellsetzen.cpp", ("vorschau",)),
)


def bereich_der_datei(pfad: str) -> tuple[tuple[str, ...] | None, str]:
    """(Bereiche oder ALLES, Grund)."""
    p = pfad.replace("\\", "/")
    name = os.path.basename(p)
    if p.startswith("gui/selbsttest_") and name.endswith(".inc"):
        kern = name[len("selbsttest_"):-len(".inc")]
        for anfang, bereich in SELBSTTEST:
            if kern.startswith(anfang):
                return (bereich,), "Selbsttest des Bereichs"
        return (), "Selbsttest ohne Regel (neu?): seinen Lauf selbst nennen"
    for muster, bereiche_ in REGELN:
        if fnmatch.fnmatch(p, muster):
            return bereiche_, "Regel " + muster
    return ALLES, "keine Regel (Kern oder gemeinsam benutzt)"


def geaenderte_dateien(basis: str, repo: str) -> list[str]:
    if shutil.which("git") is None:   # kein Rueckverfolg auf einem Rechner ohne git (tools/lint_fremdprogramm.py)
        sys.exit("git fehlt im Suchpfad - ohne den Unterschied laesst sich nichts eingrenzen: die volle Runde fahren")

    def git(*a: str) -> list[str]:
        r = subprocess.run(["git", "-C", repo, *a], capture_output=True, encoding="utf-8", errors="replace")
        if r.returncode != 0:
            sys.exit("git %s: %s" % (" ".join(a), r.stderr.strip()))
        return [z for z in r.stdout.splitlines() if z.strip()]
    return sorted(set(git("diff", "--name-only", basis + "..HEAD") + git("diff", "--name-only", "HEAD") + git("ls-files", "--others", "--exclude-standard")))


# gui/selbsttest.cpp beruehrt jeder Zweig, der eine Gruppe anhaengt. Kommt im Unterschied nur das dazu - Einbinden der .inc,
# der Schalter des Modus, ein ganzer neuer Zweig "} else if (nurX) {" der Verteilung - laufen die anderen Modi wie vorher.
_ANGEHAENGT = re.compile(r'\s*(#include "[^"]+"|static const bool nur\w+ = .*|//.*)?\s*(//.*)?$')


def selbsttest_nur_angehaengt(basis: str, repo: str) -> bool:
    r = subprocess.run(["git", "-C", repo, "diff", "-U0", basis, "--", "gui/selbsttest.cpp"], capture_output=True, encoding="utf-8", errors="replace")
    if r.returncode != 0:
        return False
    stuecke: list[list[str]] = []
    for z in r.stdout.splitlines():
        if z.startswith("@@"):
            stuecke.append([])
        elif z.startswith("-") and not z.startswith("---"):
            return False                      # etwas Bestehendes geaendert oder entfernt
        elif z.startswith("+") and not z.startswith("+++") and stuecke:
            stuecke[-1].append(z[1:])
    for st in stuecke:
        if st and re.match(r"\s*\} else if \(nur\w+( \|\| nur\w+)*\) \{", st[0]):
            continue                          # ein neuer Zweig der Verteilung: nur fuer den neuen Modus
        if not all(_ANGEHAENGT.match(z) for z in st):
            return False
    return True


# Sammeldateien, in die jeder Zweig eintraegt: kommen nur Zeilen dazu (neue Texte, neue Quelldateien, neue Tests), aendert sich
# fuer die bestehenden Modi nichts. Eine geaenderte oder entfernte Zeile (ein bestehender Text, ein Schalter) heisst ALLES.
SAMMEL = ("CMakeLists.txt", "include/bhed/i18n.h", "src/i18n.cpp", "src/hilfe_orte.inc", "src/hilfe_daten.inc", "src/changelog.cpp")
_SCHALTER = re.compile(r"set\(CMAKE_|_FLAGS|add_compile|target_compile|add_definitions|option\(")


def nur_zeilen_dazu(datei: str, basis: str, repo: str) -> bool:
    r = subprocess.run(["git", "-C", repo, "diff", "-U0", basis, "--", datei], capture_output=True, encoding="utf-8", errors="replace")
    if r.returncode != 0:
        return False
    for z in r.stdout.splitlines():
        if z.startswith("-") and not z.startswith("---"):
            return False
        if z.startswith("+") and not z.startswith("+++") and datei == "CMakeLists.txt" and _SCHALTER.search(z):
            return False
    return True


def auswerten(dateien: list[str], basis: str | None = None, repo: str | None = None) -> tuple[set[str] | None, list[str], bool]:
    """(Bereiche oder ALLES, Zeilen fuer den Bericht, Bau und ctest noetig)."""
    bereiche_: set[str] | None = set()
    je: dict[str, list[str]] = {}
    bau = False
    for d in dateien:
        b, grund = bereich_der_datei(d)
        if d.replace("\\", "/") == "gui/selbsttest.cpp" and basis is not None and selbsttest_nur_angehaengt(basis, repo):
            b, grund = (), "nur angehaengt (Einbinden, Schalter, neuer Zweig): andere Modi unberuehrt"
        if d.replace("\\", "/") in SAMMEL and basis is not None and nur_zeilen_dazu(d.replace("\\", "/"), basis, repo):
            b, grund = (), "Sammeldatei, nur Zeilen dazu: bestehende Modi unberuehrt"
        if not d.replace("\\", "/").startswith("docs/") and not d.endswith(".md"):
            bau = True
        schluessel = "ALLES" if b is ALLES else (", ".join(b) if b else "kein Selbsttest")
        je.setdefault(schluessel + " <- " + grund, []).append(d)
        if b is ALLES:
            bereiche_ = ALLES
        elif bereiche_ is not ALLES:
            bereiche_ |= set(b)
    zeilen = ["%s: %s%s" % (k, ", ".join(v[:6]), " ... (%d)" % len(v) if len(v) > 6 else "") for k, v in sorted(je.items())]
    zeilen.append("BETROFFEN: %s; %s" % ("ALLES" if bereiche_ is ALLES else (", ".join(sorted(bereiche_)) or "kein Selbsttest-Bereich"),
                                       "Bau und ctest noetig" if bau else "nur Doku: kein Bau, kein ctest"))
    return bereiche_, zeilen, bau


def bereiche(basis: str, repo: str) -> tuple[set[str] | None, list[str]]:
    b, zeilen, _ = auswerten(geaenderte_dateien(basis, repo), basis, repo)
    return b, zeilen


def main() -> int:
    repo = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    if len(sys.argv) >= 3 and sys.argv[1] == "--repo":   # ein anderer Arbeitsbaum (nur lesen)
        repo = sys.argv[2]
        del sys.argv[1:3]
    if len(sys.argv) >= 3 and sys.argv[1] == "--dateien":
        dateien = sys.argv[2:]
    elif len(sys.argv) == 2 and not sys.argv[1].startswith("-"):
        dateien = geaenderte_dateien(sys.argv[1], repo)
        print("\n".join(auswerten(dateien, sys.argv[1], repo)[1]))
        return 0
    else:
        print(__doc__)
        return 2
    _, zeilen, _ = auswerten(dateien)
    print("\n".join(zeilen))
    return 0


if __name__ == "__main__":
    sys.exit(main())
