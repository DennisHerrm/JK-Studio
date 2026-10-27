#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Baut und prueft das Release-Paket (rc601 release).

Warum es das gibt
-----------------
shank, 17.10.2026: "bei jedem Release sollte es auch eine .exe geben, die
gebaut wird, dass die Leute sie runterladen und nutzen koennen". Bis rc600
entstanden Zip und Notizen von Hand, ausserhalb des Repos. Dabei ging ab rc598
die Zeile "Install: unzip and start BehavEd-Remake/behaved.exe" verloren, und
niemand pruefte die ausgelieferte .exe: sie brauchte die VC-Laufzeit, die
nicht im Zip lag (tools/check_exe_imports.py am rc600: "Importpruefung: 16
Beanstandung(en)").

Jetzt baut .github/workflows/release.yml bei jeder Marke v1.0.0-rcNNN das
Paket mit diesem Werkzeug und prueft es, bevor etwas hochgeladen wird.

Aufrufe
-------
  release_paket.py fassung [--marke v1.0.0-rcNNN]
      kFassung ausgeben. Mit --marke: Abbruch, wenn die Marke nicht
      v1.0.0-<kFassung> heisst - sonst boete der Updater das Release ewig neu
      an (istNeuer vergleicht die rc-Nummer der Marke mit kFassung).
  release_paket.py zip <jkstudio.exe> <ziel.zip> [--q3map2 <bhed_q3map2.exe>]
      JK-Studio/jkstudio.exe, JK-Studio/behaved.exe (dieselbe Datei, fuer den
      Uebergang der alten Updater, Stufe 8c), JK-Studio/data/base/* und jede Datei,
      die git unter handbuch/ fuehrt (git ls-files: Seiten, die nur im
      Arbeitsordner liegen, bleiben draussen). Deflate, '/' als Trenner,
      sortiert, keine Ordnereintraege - die Form von rc600.
      Stufe 3: mit --q3map2 dazu JK-Studio/bhed_q3map2.exe (Karten bauen)
      und JK-Studio/licenses/ (GPL v2 und die Lizenztexte der
      Bibliotheken in bhed_q3map2.exe, siehe LIZENZEN).
  release_paket.py pruefe <zip> --fassung rcNNN [--exe <eigenstaendige.exe>] [--q3map2]
      alles, was Updater und Anwender vom Zip erwarten (siehe pruefe()); mit
      --q3map2 muss bhed_q3map2.exe samt Lizenztexten dabei sein.
  release_paket.py notizen rcNNN <ziel.md>
      die Release-Notizen: zuerst die Download-Zeilen, dann der [en]-Block der
      obersten Fassung in tools/changelog.txt - oder release-notes/rcNNN.md,
      wenn es die Datei gibt. Schreibt selbst UTF-8 (eine Umleitung durch
      PowerShell machte aus Gedankenstrich und Pfeil Fragezeichen).
  release_paket.py pruefsummen <ziel.txt> <datei>...
      SHA256SUMS im Format von sha256sum, LF, nach Namen sortiert.
  release_paket.py selbsttest
      prueft das Werkzeug an kuenstlichen Paketen (ctest release_paket).

Ueberall gilt --wurzel <ordner> statt des Repos, in dem dieses Werkzeug liegt.

Nur die Standardbibliothek. git wird vorher mit shutil.which gesucht - ein
fehlendes Programm soll eine klare Meldung geben, keinen Rueckverfolg
(tools/lint_fremdprogramm.py).
"""

import argparse
import hashlib
import importlib.util
import io
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import time
import warnings
import zipfile

HIER = os.path.dirname(os.path.abspath(__file__))
WURZEL = os.path.dirname(HIER)
# Stufe 8c: das Programm heisst JK Studio. Ordner im Zip und Namen der Dateien
# des Releases; die Programmdatei jkstudio.exe und - fuer die Updater bis rc605,
# die genau einen Eintrag "behaved.exe" ganz oben an die Stelle der laufenden .exe
# setzen und neu starten (src/update.cpp) - behaved.exe als dieselbe Datei.
OBER = "JK-Studio"
NAME = "JK-Studio"
PROGRAMM = "jkstudio.exe"
UEBERGANG = "behaved.exe"
SPRACHEN = ("en", "de", "zh", "ja")
# Die Grenzen des Lesers, mit dem der Updater das Zip oeffnet (src/pk3.cpp):
# die Eintragszahl steht dort als u16, Groessen und Versatz als u32 (kein
# ZIP64), und er entpackt nur "gespeichert" (0) und Deflate (8).
PACKVERFAHREN = (0, 8)
MAX_EINTRAEGE = 0xFFFF
MAX_GROESSE = 0xFFFFFFFF


class Fehler(Exception):
    pass


def _importpruefung():
    # Ueber den Pfad laden, nicht ueber sys.path: das Werkzeug soll auch mit
    # "python -I" laufen (dann steht tools/ nicht im Suchpfad).
    spec = importlib.util.spec_from_file_location(
        "check_exe_imports", os.path.join(HIER, "check_exe_imports.py"))
    modul = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(modul)
    return modul


# --- Fassung und Marke ------------------------------------------------------

def fassung_lesen(wurzel):
    pfad = os.path.join(wurzel, "include", "bhed", "fassung.h")
    with open(pfad, encoding="utf-8") as f:
        m = re.search(r'kFassung\s*=\s*"(rc\d+)"', f.read())
    if m is None:
        raise Fehler("kFassung nicht gefunden in %s" % pfad)
    return m.group(1)


def marke_passt(marke, rc):
    return marke == "v1.0.0-" + rc


# --- Das Zip ------------------------------------------------------------------

def handbuch_dateien(wurzel):
    git = shutil.which("git")
    if git is None:
        raise Fehler("git nicht gefunden - ohne 'git ls-files' laesst sich nicht "
                     "sagen, welche Handbuchseiten ins Paket gehoeren")
    r = subprocess.run([git, "-C", wurzel, "ls-files", "-z", "--", "handbuch"],
                       capture_output=True)
    if r.returncode != 0:
        raise Fehler("git ls-files: " + r.stderr.decode("utf-8", "replace").strip())
    return sorted(n for n in r.stdout.decode("utf-8").split("\0") if n)


def _zeitstempel():
    # SOURCE_DATE_EPOCH (gesetzt im Ablauf auf die Zeit des Commits) macht das
    # Zip wiederholbar: zweimal gebaut, zweimal dieselben Bytes.
    wert = os.environ.get("SOURCE_DATE_EPOCH", "").strip()
    if not wert.isdigit():
        return None
    return max(time.gmtime(int(wert))[:6], (1980, 1, 1, 0, 0, 0))


def daten_ordner(wurzel):
    """Die Datenordner unter data/, die ins Zip gehoeren: base, jk2 (rc604) und
    je Spiel data/karte/<spiel> (Karten-Editor, Stufe 1b: die .def)."""
    ordner = ["base", "jk2"]
    karte = os.path.join(wurzel, "data", "karte")
    if os.path.isdir(karte):
        for s in sorted(os.listdir(karte)):
            if os.path.isdir(os.path.join(karte, s)):
                ordner.append("karte/" + s)
    return ordner


# Stufe 3 Punkt 1: bhed_q3map2.exe enthaelt q3map2 (GPL v2) und zlib, libpng,
# libjpeg-turbo, libxml2, assimp, crunch, ddslib, tiny_webp, rapidjson. Wer die
# .exe weitergibt, gibt die Lizenztexte mit (BSD/MIT/IJG verlangen das fuer
# Binaerpakete). Ganze Dateien, oder (Anfang, Ende) eines Kommentarblocks, wenn
# die Lizenz im Kopf einer Quelldatei steht. Boost, zlib und Unlicense
# verlangen das fuer Binaerpakete nicht; mitgegeben wird trotzdem, was als
# eigene Datei vorliegt.
LIZENZEN = [
    ("GPL-2.0.txt", "LICENSE", None),
    # Stufe 8c: die Schriftzuege des Logos (res/jkstudio_logo.png, jkstudio.ico) aus Cinzel und Michroma
    ("OFL-Cinzel.txt", "licenses/OFL-Cinzel.txt", None),
    ("OFL-Michroma.txt", "licenses/OFL-Michroma.txt", None),
    ("q3map2-README.md", "third_party/q3map2/README.md", None),
    ("zlib.txt", "third_party/zlib/LICENSE", None),
    ("libpng.txt", "third_party/libpng/LICENSE", None),
    ("libjpeg-turbo.md", "third_party/libjpeg-turbo/LICENSE.md", None),
    ("libjpeg-turbo-README.ijg.txt", "third_party/libjpeg-turbo/README.ijg", None),
    ("libxml2.txt", "third_party/libxml2/Copyright", None),
    ("assimp.txt", "third_party/assimp/LICENSE", None),
    ("assimp-clipper.txt", "third_party/assimp/contrib/clipper/License.txt", None),
    ("assimp-earcut.txt", "third_party/assimp/contrib/earcut-hpp/LICENSE", None),
    ("assimp-openddlparser.txt", "third_party/assimp/contrib/openddlparser/LICENSE", None),
    ("assimp-poly2tri.txt", "third_party/assimp/contrib/poly2tri/LICENSE", None),
    ("assimp-pugixml.md", "third_party/assimp/contrib/pugixml/LICENSE.md", None),
    ("assimp-rapidjson.txt", "third_party/assimp/contrib/rapidjson/license.txt", None),
    ("assimp-zip.txt", "third_party/assimp/contrib/zip/UNLICENSE", None),
    ("assimp-open3dgc.txt", "third_party/assimp/contrib/Open3DGC/o3dgcCommon.h",
     ("Copyright (c) 2013 Khaled Mammou", "THE SOFTWARE.")),
    ("assimp-stb.txt", "third_party/assimp/contrib/stb/stb_image.h",
     ("This software is available under 2 licenses", "WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.")),
    ("crunch.txt", "third_party/q3map2/libs/crnlib/crunch/inc/crnlib.h",
     ("crnlib uses the ZLIB license", "This notice may not be removed or altered from any source distribution.")),
    ("ddslib.txt", "third_party/q3map2/libs/ddslib/ddslib.c",
     ("Copyright (c) 2003 Randy Reddig", "SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.")),
    ("tiny_webp.txt", "third_party/q3map2/libs/webplib/tiny_webp.h",
     ("Copyright (c) 2010, 2011, Google Inc.", "OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.")),
    ("rapidjson.txt", "third_party/q3map2/libs/rapidjson/rapidjson.h",
     ("Tencent is pleased", "specific language governing permissions and limitations under the License.")),
]


def lizenz_eintraege(wurzel):
    """[(Name im Zip, Bytes)] fuer JK-Studio/licenses/."""
    aus = []
    for name, rel, block in LIZENZEN:
        pfad = os.path.join(wurzel, *rel.split("/"))
        try:
            with open(pfad, "rb") as f:
                inhalt = f.read()
        except OSError:
            raise Fehler("Lizenztext %s fehlt (%s)" % (rel, name))
        if block is not None:
            text = inhalt.decode("utf-8", "replace")
            a = text.find(block[0])
            e = text.find(block[1], a)
            if a < 0 or e < 0:
                raise Fehler("%s: Lizenzblock '%s' ... '%s' nicht gefunden" % (rel, block[0], block[1]))
            zeilen = text[a:e + len(block[1])].replace("\r\n", "\n").split("\n")
            # Kommentarzeichen am Zeilenanfang weg ("// ", " * ", "   ")
            sauber = [re.sub(r"^\s*(//|\*)? ?", "", z).rstrip() for z in zeilen]
            inhalt = ("From %s (part of bhed_q3map2.exe):\n\n" % rel + "\n".join(sauber) + "\n").encode("utf-8")
        aus.append((OBER + "/licenses/" + name, inhalt))
    return aus


def zip_bauen(exe, ziel, wurzel, handbuch=None, q3map2=None):
    eintraege = [(OBER + "/" + PROGRAMM, exe), (OBER + "/" + UEBERGANG, exe)]
    if q3map2 is not None:
        # Stufe 3: das Programm fuer Build-Menue (Karten bauen) neben jkstudio.exe,
        # dazu die Lizenztexte (Bytes statt Pfad)
        eintraege.append((OBER + "/bhed_q3map2.exe", q3map2))
        eintraege += lizenz_eintraege(wurzel)
    # rc604: data/jk2 (Befehlssatz fuer Jedi Outcast) neben data/base;
    # Stufe 1b: data/karte/<spiel> (Entitaetsklassen des Karten-Editors).
    for ordner in daten_ordner(wurzel):
        daten = os.path.join(wurzel, "data", *ordner.split("/"))
        if not os.path.isdir(daten):
            continue
        for n in sorted(os.listdir(daten)):
            if os.path.isfile(os.path.join(daten, n)):
                eintraege.append((OBER + "/data/" + ordner + "/" + n, os.path.join(daten, n)))
    if handbuch is None:
        handbuch = handbuch_dateien(wurzel)
    if not handbuch:
        raise Fehler("handbuch/: keine Datei unter Versionsverwaltung - das Zip "
                     "waere ohne Handbuch")
    for rel in handbuch:
        quelle = os.path.join(wurzel, *rel.split("/"))
        if not os.path.isfile(quelle):
            raise Fehler("%s steht in git, fehlt aber im Arbeitsordner" % rel)
        eintraege.append((OBER + "/" + rel, quelle))
    # Sortiert, aber die Programme ZULETZT: der Updater spielt die Eintraege der
    # Reihe nach ein und bricht beim ersten Fehler ab. Scheitert er vor der
    # .exe, laeuft weiter die alte Fassung - und bietet das Update beim
    # naechsten Start wieder an. Stuende die .exe vorn, meldete sich nach
    # einem halben Update schon die neue Fassung, und data/ oder handbuch/
    # blieben halb alt. Stufe 8c: jkstudio.exe vorletzt, behaved.exe zuletzt -
    # fuer die alten Updater ist behaved.exe das Programm, fuer die neuen
    # jkstudio.exe (behaved.exe ist dann eine Datei daneben).
    rang = {OBER + "/" + PROGRAMM: 1, OBER + "/" + UEBERGANG: 2}
    eintraege.sort(key=lambda e: (rang.get(e[0], 0), e[0]))
    ordner = os.path.dirname(os.path.abspath(ziel))
    os.makedirs(ordner, exist_ok=True)
    fest = _zeitstempel()
    with zipfile.ZipFile(ziel, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for name, quelle in eintraege:
            if isinstance(quelle, bytes):
                inhalt = quelle
                zeit = fest or time.localtime()[:6]
            else:
                with open(quelle, "rb") as f:
                    inhalt = f.read()
                zeit = fest or max(time.localtime(os.path.getmtime(quelle))[:6], (1980, 1, 1, 0, 0, 0))
            info = zipfile.ZipInfo(name, date_time=zeit)
            info.compress_type = zipfile.ZIP_DEFLATED
            z.writestr(info, inhalt, compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)
    return len(eintraege)


# --- Die Pruefung -------------------------------------------------------------

def _pe_maengel(exe, daten, imp, name=PROGRAMM):
    """Was an der jkstudio.exe (oder bhed_q3map2.exe) im Zip nicht stimmt (Liste von Saetzen)."""
    maengel = []
    if len(exe) < 0x40 or exe[:2] != b"MZ":
        return ["%s ist keine Windows-exe (MZ fehlt)" % name]
    pe = struct.unpack_from("<I", exe, 0x3C)[0]
    if pe + 26 > len(exe) or exe[pe:pe + 4] != b"PE\0\0":
        return ["%s: PE-Kopf fehlt" % name]
    maschine = struct.unpack_from("<H", exe, pe + 4)[0]
    if maschine != 0x8664:
        maengel.append("%s ist keine 64-Bit-exe (Machine 0x%04x, erwartet 0x8664 = x64)" % (name, maschine))
    if struct.unpack_from("<H", exe, pe + 24)[0] != 0x20B:
        maengel.append("%s: kein PE32+-Kopf" % name)
    try:
        dlls = imp.imports_aus_daten(exe)
    except (struct.error, ValueError, IndexError, UnicodeDecodeError) as e:
        return maengel + ["%s: Importtabelle nicht lesbar (%s)" % (name, e)]
    for dll in sorted(dlls, key=str.lower):
        klein = dll.lower()
        if klein in imp.VERBOTEN:
            maengel.append("%s bindet %s fest - ohne die DLL startet das Programm nicht "
                           "(Rueckfall auf OpenGL kaeme nie zum Zug)" % (name, dll))
        elif klein not in imp.ERLAUBT:
            maengel.append("%s braucht %s - die liegt nicht auf jedem Windows "
                           "(VC-Laufzeit? CMAKE_MSVC_RUNTIME_LIBRARY)" % (name, dll))
    fehlend = [n for n, b in daten if b and b not in exe]
    if fehlend:
        maengel.append("%s enthaelt die eingebauten Daten nicht (%s) - allein liefe "
                       "sie nicht (bhed/eingebaut.h)" % (name, ", ".join(fehlend)))
    return maengel


def pruefe(zip_pfad, rc, wurzel, exe_pfad=None, ausgabe=print, q3map2=False):
    """Gibt die Liste der Beanstandungen zurueck (leer = in Ordnung)."""
    maengel = []

    def melde(text):
        maengel.append(text)
        ausgabe("  FEHL  " + text)

    def gut(text):
        ausgabe("  ok    " + text)

    if not re.fullmatch(r"rc\d+", rc or ""):
        melde("Fassung %r hat nicht die Form rcNNN" % rc)
        return maengel
    soll = "%s-%s-windows-x64.zip" % (NAME, rc)
    name = os.path.basename(zip_pfad)
    if name == soll:
        gut("Name " + name)
    else:
        melde("Name %s - erwartet %s (Marke des Releases, -windows-x64)" % (name, soll))

    try:
        z = zipfile.ZipFile(zip_pfad)
    except (zipfile.BadZipFile, OSError) as e:
        melde("kein lesbares Zip: %s" % e)
        return maengel
    with z:
        infos = z.infolist()
        if len(infos) >= MAX_EINTRAEGE:
            melde("%d Eintraege - der Updater liest hoechstens %d (src/pk3.cpp, u16)"
                  % (len(infos), MAX_EINTRAEGE - 1))
        if os.path.getsize(zip_pfad) >= MAX_GROESSE:
            melde("das Zip ist groesser als 4 GB (ZIP64 kann der Updater nicht)")
        # orig_filename: unter Windows macht zipfile aus "\" in filename ein
        # "/" - der Rueckstrich waere so unsichtbar.
        roh = [i.orig_filename for i in infos]
        gesehen = set()
        doppelt = set()
        for n in roh:
            (doppelt if n in gesehen else gesehen).add(n)
        for n in sorted(doppelt):
            melde("doppelter Eintrag %s (der Updater nimmt dann irgendeinen)" % n)
        schlecht = 0
        for i in infos:
            n = i.orig_filename
            teile = n.split("/")
            if "\\" in n:
                melde("Rueckstrich im Namen %r - im Zip trennt '/'" % n)
            if n.startswith("/") or re.match(r"^[A-Za-z]:", n):
                melde("absoluter Name %r - der Updater ueberspringt ihn" % n)
            if ".." in teile:
                melde("'..' im Namen %r - der Updater ueberspringt ihn" % n)
            if n.endswith("/"):
                melde("Ordnereintrag %r" % n)
            elif "" in teile:
                melde("leerer Namensteil in %r" % n)
            if i.compress_type not in PACKVERFAHREN:
                melde("%s: Packverfahren %d - der Updater entpackt nur 0 und 8 (src/pk3.cpp)"
                      % (n, i.compress_type))
            if i.flag_bits & 0x1:
                melde("%s ist verschluesselt" % n)
            if i.file_size >= MAX_GROESSE or i.compress_size >= MAX_GROESSE:
                melde("%s braucht ZIP64 (groesser als 4 GB)" % n)
            schlecht += 0
        oben = sorted({n.replace("\\", "/").split("/")[0] for n in roh})
        lose = [n for n in roh if "/" not in n.replace("\\", "/")]
        if oben == [OBER] and not lose:
            gut("genau ein oberster Ordner %s/ (%d Eintraege)" % (OBER, len(infos)))
        else:
            melde("erwartet genau einen obersten Ordner %s/, gefunden: %s"
                  % (OBER, ", ".join(oben[:6]) + (" ..." if len(oben) > 6 else "")))

        nach_name = {}
        for i in infos:
            nach_name.setdefault(i.orig_filename, i)

        def lies(rel):
            info = nach_name.get(OBER + "/" + rel)
            if info is None or info.compress_type not in PACKVERFAHREN:
                return None
            try:
                return z.read(info)
            except (zipfile.BadZipFile, OSError, NotImplementedError, RuntimeError) as e:
                melde("%s nicht lesbar: %s" % (rel, e))
                return None

        exe = lies(PROGRAMM)
        uebergang = lies(UEBERGANG)
        if exe is None:
            melde("%s fehlt" % PROGRAMM)
        if uebergang is None:
            melde("%s fehlt - die Updater bis rc605 setzen genau diesen Eintrag an die Stelle der "
                  "laufenden .exe; ohne ihn bleiben alte Installationen auf der alten Fassung" % UEBERGANG)
        elif exe is not None and uebergang != exe:
            melde("%s weicht von %s ab - beide muessen dieselbe Datei sein" % (UEBERGANG, PROGRAMM))
        if exe is not None and uebergang is not None:
            if roh[-2:] != [OBER + "/" + PROGRAMM, OBER + "/" + UEBERGANG]:
                melde("%s und %s sind nicht die letzten Eintraege (in dieser Reihenfolge) - scheitert das "
                      "Update danach, meldet sich schon die neue Fassung, und der Rest bleibt halb alt "
                      "(zip_bauen legt sie zuletzt)" % (PROGRAMM, UEBERGANG))
            else:
                gut("%s und %s (dieselbe Datei) sind die letzten Eintraege (ein abgebrochenes Update "
                    "laesst die alte Fassung)" % (PROGRAMM, UEBERGANG))
        # Alle Datenordner fuer die Pruefung, dass ihre Bytes in der .exe
        # stecken (vorher nur die des letzten Ordners der Schleife).
        alle_daten = []
        for ordner in daten_ordner(wurzel):   # rc604: data/jk2, Stufe 1b: data/karte/<spiel>
            pfad_ordner = os.path.join(wurzel, "data", *ordner.split("/"))
            if ordner != "base" and not os.path.isdir(pfad_ordner):
                continue
            daten = []
            for n in sorted(os.listdir(pfad_ordner)):
                p = os.path.join(pfad_ordner, n)
                if not os.path.isfile(p):
                    continue
                with open(p, "rb") as f:
                    daten.append((n, f.read()))
            abweichend = []
            for n, b in daten:
                drin = lies("data/%s/%s" % (ordner, n))
                if drin is None:
                    melde("data/%s/%s fehlt" % (ordner, n))
                elif drin != b:
                    abweichend.append(n)
            if abweichend:
                melde("data/%s weicht vom Repo ab: %s" % (ordner, ", ".join(abweichend)))
            elif daten and all(nach_name.get(OBER + "/data/%s/%s" % (ordner, n)) for n, _ in daten):
                gut("data/%s: %d Dateien, Byte fuer Byte wie im Repo" % (ordner, len(daten)))
            alle_daten += daten
        seiten = ["handbuch/index.html"] + ["handbuch/%s/index.html" % s for s in SPRACHEN]
        fehlen = [s for s in seiten if OBER + "/" + s not in nach_name]
        if fehlen:
            melde("Handbuch unvollstaendig, es fehlt: %s" % ", ".join(fehlen))
        else:
            gut("Handbuch: Startseite und alle vier Sprachen")
        if exe is not None:
            m = _pe_maengel(exe, alle_daten, _importpruefung())
            for t in m:
                melde(t)
            if not m:
                gut("%s: x64, nur Windows-DLLs fest gebunden, Daten eingebaut" % PROGRAMM)
        # Stufe 3: bhed_q3map2.exe (Karten bauen) mit den Lizenztexten
        q3 = lies("bhed_q3map2.exe")
        if q3 is None:
            if q3map2:
                melde("bhed_q3map2.exe fehlt - ohne sie baut das Build-Menue keine Karten")
        else:
            m = _pe_maengel(q3, [], _importpruefung(), "bhed_q3map2.exe")
            for t in m:
                melde(t)
            if not m:
                gut("bhed_q3map2.exe: x64, nur Windows-DLLs fest gebunden")
            try:
                soll_liz = lizenz_eintraege(wurzel)
            except Fehler as e:
                soll_liz = []
                melde(str(e))
            fehl_liz = [n for n, b in soll_liz if lies(n[len(OBER) + 1:]) != b]
            if fehl_liz:
                melde("Lizenztexte zu bhed_q3map2.exe fehlen oder weichen ab: %s"
                      % ", ".join(n.split("/")[-1] for n in fehl_liz))
            elif soll_liz:
                gut("licenses/: %d Lizenztexte zu bhed_q3map2.exe" % len(soll_liz))
        if exe_pfad is not None:
            soll_exe = "%s-%s-windows-x64.exe" % (NAME, rc)
            if os.path.basename(exe_pfad) != soll_exe:
                melde("eigenstaendige exe heisst %s - erwartet %s" % (os.path.basename(exe_pfad), soll_exe))
            try:
                with open(exe_pfad, "rb") as f:
                    allein = f.read()
            except OSError as e:
                allein = None
                melde("eigenstaendige exe nicht lesbar: %s" % e)
            if allein is not None and exe is not None:
                if allein == exe:
                    gut("eigenstaendige exe gleicht %s im Zip" % PROGRAMM)
                else:
                    melde("eigenstaendige exe weicht von %s im Zip ab" % PROGRAMM)
    ausgabe("Paketpruefung: " + ("ok" if not maengel else "%d Beanstandung(en)" % len(maengel)))
    return maengel


# --- Die Notizen --------------------------------------------------------------

# Die ersten Zeilen jedes Releases. Die Updateanzeige im Programm zeigt den
# Text unformatiert (gui/update_win32.cpp, TextUnformatted) - er muss also
# auch ohne Markdown gut lesbar sein. Gedankenstrich, Pfeil und Auslassung als
# \u-Folgen, damit diese Datei reines ASCII bleibt.
KOPF = (
    "**Download:** `JK-Studio-{rc}-windows-x64.zip` (under **Assets** below) \u2013 "
    "Windows 10/11, 64-bit. JK Studio \u2013 mod tools for Jedi Knight: Jedi Academy and Jedi Outcast "
    "(BehavEd scripts, cutscene preview, map editor). Not affiliated with Lucasfilm, Disney or Raven Software.\n"
    "\n"
    "- Unzip it (right-click \u2192 Extract All\u2026), open the `JK-Studio` folder and "
    "start `jkstudio.exe`. Nothing is installed; keep `jkstudio.exe` next to its `data` folder.\n"
    "- Just the program: `JK-Studio-{rc}-windows-x64.exe` also runs on its own "
    "(the offline guide and the map compiler `bhed_q3map2.exe` are only in the zip).\n"
    "- Already using BehavEd-Remake? It updates itself to JK Studio (Help \u2192 Check for updates\u2026) "
    "and keeps your settings; `behaved.exe` stays as a starter for old shortcuts.\n"
    "- \"Source code (zip)\" and \"Source code (tar.gz)\" are for developers \u2013 they "
    "contain no program.\n"
    "- Windows SmartScreen may warn because the program is not code-signed: "
    "More info \u2192 Run anyway.\n"
)


def changelog_en(wurzel, rc):
    pfad = os.path.join(wurzel, "tools", "changelog.txt")
    oben = None
    sprache = None
    block = []
    with open(pfad, encoding="utf-8") as f:
        for zeile in f.read().splitlines():
            m = re.match(r"^## (\S+)", zeile)
            if m:
                if oben is not None:
                    break
                oben = m.group(1)
                continue
            if oben is None or zeile.startswith("#"):
                continue
            m = re.match(r"^\[(en|de|zh|ja)\]\s*$", zeile)
            if m:
                sprache = m.group(1)
                continue
            if sprache == "en":
                block.append(zeile)
    if oben != rc:
        raise Fehler("oberste Fassung in tools/changelog.txt ist %s, nicht %s" % (oben, rc))
    while block and not block[-1].strip():
        block.pop()
    if not block:
        raise Fehler("tools/changelog.txt: %s hat keinen [en]-Block" % rc)
    return "\n".join(block)


def notizen_text(rc, wurzel):
    kfassung = fassung_lesen(wurzel)
    if kfassung != rc:
        raise Fehler("kFassung ist %s, nicht %s" % (kfassung, rc))
    eigen = os.path.join(wurzel, "release-notes", rc + ".md")
    if os.path.isfile(eigen):
        with open(eigen, encoding="utf-8") as f:
            rumpf = f.read().strip("\n")
    else:
        rumpf = "## What's new in %s\n\n%s" % (rc, changelog_en(wurzel, rc))
    return KOPF.format(rc=rc) + "\n" + rumpf + "\n"


def notizen(rc, ziel, wurzel):
    text = notizen_text(rc, wurzel)
    with open(ziel, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    return text


# --- Pruefsummen ----------------------------------------------------------------

def pruefsummen(ziel, dateien):
    zeilen = []
    for p in sorted(dateien, key=lambda d: os.path.basename(d)):
        h = hashlib.sha256()
        with open(p, "rb") as f:
            for stueck in iter(lambda: f.read(1 << 20), b""):
                h.update(stueck)
        zeilen.append("%s  %s\n" % (h.hexdigest(), os.path.basename(p)))
    with open(ziel, "w", encoding="ascii", newline="\n") as f:
        f.writelines(zeilen)
    return zeilen


# --- Selbsttest -----------------------------------------------------------------

def _probe_exe(dlls, maschine=0x8664, anhang=b""):
    """Die kleinste PE32+-Datei, die imports() lesen kann: DOS-Kopf, PE-Kopf,
    ein Abschnitt mit der Importtabelle - und hinten die Bytes `anhang`."""
    pe = 0x40
    optgroesse = 240                       # PE32+: 112 Byte + 16 Verzeichnisse
    kopf = bytearray(0x200)
    kopf[0:2] = b"MZ"
    struct.pack_into("<I", kopf, 0x3C, pe)
    kopf[pe:pe + 4] = b"PE\0\0"
    struct.pack_into("<HH", kopf, pe + 4, maschine, 1)
    struct.pack_into("<H", kopf, pe + 20, optgroesse)
    struct.pack_into("<H", kopf, pe + 24, 0x20B)
    rva = 0x1000
    tabelle = 20 * (len(dlls) + 1)
    namen = b""
    abschnitt = bytearray(tabelle)
    for k, dll in enumerate(dlls):
        struct.pack_into("<IIIII", abschnitt, 20 * k, 0, 0, 0, rva + tabelle + len(namen), 0)
        namen += dll.encode("ascii") + b"\0"
    abschnitt += namen
    roh = (len(abschnitt) + 0x1FF) & ~0x1FF
    abschnitt += bytes(roh - len(abschnitt))
    struct.pack_into("<II", kopf, pe + 24 + 112 + 8, rva, tabelle)
    struct.pack_into("<8sIIII", kopf, pe + 24 + optgroesse, b".idata", len(abschnitt), rva, roh, 0x200)
    return bytes(kopf) + bytes(abschnitt) + anhang


def _zip_aus(pfad, eintraege):
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")      # "Duplicate name" ist hier Absicht
        with zipfile.ZipFile(pfad, "w", zipfile.ZIP_DEFLATED) as z:
            for name, inhalt in eintraege:
                z.writestr(zipfile.ZipInfo(name, date_time=(2026, 10, 17, 12, 0, 0)), inhalt,
                           compress_type=zipfile.ZIP_DEFLATED)


def _packverfahren_setzen(pfad, name, verfahren):
    """Traegt fuer einen Eintrag ein anderes Packverfahren ein (lokaler Kopf
    und Inhaltsverzeichnis) - zipfile selbst schreibt kein Deflate64 (9)."""
    b = bytearray(open(pfad, "rb").read())
    with zipfile.ZipFile(io.BytesIO(bytes(b))) as z:
        info = z.getinfo(name)
    struct.pack_into("<H", b, info.header_offset + 8, verfahren)
    p = b.find(b"PK\x01\x02")
    while p >= 0:
        laenge, extra, kommentar = struct.unpack_from("<HHH", b, p + 28)
        if bytes(b[p + 46:p + 46 + laenge]) == name.encode("ascii"):
            struct.pack_into("<H", b, p + 10, verfahren)
        p = b.find(b"PK\x01\x02", p + 46 + laenge + extra + kommentar)
    open(pfad, "wb").write(bytes(b))


def selbsttest():
    ergebnisse = []

    def erwarte(was, ok, maengel=None):
        print(("  ok    " if ok else "  FEHL  ") + was)
        for t in (maengel or [])[:3]:
            print("          -> " + t)
        ergebnisse.append(bool(ok))

    def still(_text):
        pass

    def schreibe(pfad, inhalt):
        os.makedirs(os.path.dirname(pfad), exist_ok=True)
        with open(pfad, "wb") as f:
            f.write(inhalt if isinstance(inhalt, bytes) else inhalt.encode("utf-8"))

    with tempfile.TemporaryDirectory(prefix="bhed_release_") as tmp:
        wurzel = os.path.join(tmp, "repo")
        rc = "rc900"
        schreibe(os.path.join(wurzel, "include", "bhed", "fassung.h"),
                 'inline constexpr const char* kFassung = "%s";\n' % rc)
        schreibe(os.path.join(wurzel, "tools", "changelog.txt"),
                 "# Kommentar\n\n## rc900 2026-10-17\n[en]\n- New: a thing.\n- Fixed: another one.\n"
                 "[de]\n- Neu: etwas.\n[zh]\n- \u65b0\n[ja]\n- \u65b0\u3057\u3044\n\n"
                 "## rc899 2026-10-16\n[en]\n- Old.\n")
        daten = {"anims.h": b"enum {BOTH_A};\r\n" * 30, "behaved.bhc": b"// bhc\r\n" * 50,
                 "supplement.bhc": b"// supp\n" * 20}
        for n, b in daten.items():
            schreibe(os.path.join(wurzel, "data", "base", n), b)
        handbuch = (["handbuch/assets/stil.css", "handbuch/index.html"] +
                    ["handbuch/%s/index.html" % s for s in SPRACHEN])
        for h in handbuch:
            schreibe(os.path.join(wurzel, *h.split("/")), "<!doctype html>" + h)
        schreibe(os.path.join(wurzel, "handbuch", "en", "entwurf.html"), "nur im Arbeitsordner")
        alle_daten = b"".join(daten[n] for n in sorted(daten))
        # Stufe 3: die Lizenzquellen (ganze Datei oder Kommentarblock)
        for _name, rel, block in LIZENZEN:
            inhalt = "Lizenz " + rel + "\n" if block is None else \
                "/* Kopf */\n// %s irgendwas\n// Mitte\n// %s\n// Rest\n" % block
            schreibe(os.path.join(wurzel, *rel.split("/")), inhalt)
        exe = os.path.join(tmp, "behaved.exe")
        schreibe(exe, _probe_exe(["KERNEL32.dll", "USER32.dll", "WINHTTP.dll"], anhang=alle_daten))

        # --- Fassung und Marke
        erwarte("fassung: kFassung = rc900", fassung_lesen(wurzel) == rc)
        erwarte("marke: v1.0.0-rc900 passt zu rc900", marke_passt("v1.0.0-rc900", rc))
        erwarte("marke: v1.0.0-rc901 zu rc900 wird abgewiesen (Updater boete es ewig an)",
                not marke_passt("v1.0.0-rc901", rc) and not marke_passt("rc900", rc))
        erwarte("aufruf: fassung --marke v1.0.0-rc901 endet mit 1",
                main(["fassung", "--marke", "v1.0.0-rc901", "--wurzel", wurzel], still) == 1)

        # --- Das Zip bauen
        gut_zip = os.path.join(tmp, "JK-Studio-rc900-windows-x64.zip")
        git = shutil.which("git")
        if git is not None:
            subprocess.run([git, "init", "-q", wurzel], check=True, capture_output=True)
            subprocess.run([git, "-C", wurzel, "add", "--"] + handbuch, check=True, capture_output=True)
            n = zip_bauen(exe, gut_zip, wurzel)
        else:
            print("          (git fehlt - das Handbuch kommt aus der festen Liste)")
            n = zip_bauen(exe, gut_zip, wurzel, handbuch=handbuch)
        with zipfile.ZipFile(gut_zip) as z:
            namen = z.namelist()
            verfahren = {i.compress_type for i in z.infolist()}
        erwarte("zip: %d Eintraege, sortiert, jkstudio.exe und behaved.exe zuletzt, Deflate, ohne Ordnereintraege" % n,
                n == 2 + len(daten) + len(handbuch) and namen[:-2] == sorted(namen[:-2])
                and namen[-2:] == [OBER + "/" + PROGRAMM, OBER + "/" + UEBERGANG] and verfahren == {8}
                and not any(x.endswith("/") for x in namen))
        with zipfile.ZipFile(gut_zip) as z:
            gleich = z.read(OBER + "/" + PROGRAMM) == z.read(OBER + "/" + UEBERGANG)
        erwarte("zip: alles unter JK-Studio/, jkstudio.exe und behaved.exe dieselbe Datei",
                all(x.startswith(OBER + "/") for x in namen) and gleich)
        erwarte("zip: eine Seite, die git nicht kennt, bleibt draussen",
                OBER + "/handbuch/en/entwurf.html" not in namen)
        m = pruefe(gut_zip, rc, wurzel, ausgabe=still)
        erwarte("pruefe: das gebaute Zip besteht", not m, m)
        allein = os.path.join(tmp, "JK-Studio-rc900-windows-x64.exe")
        shutil.copyfile(exe, allein)
        m = pruefe(gut_zip, rc, wurzel, exe_pfad=allein, ausgabe=still)
        erwarte("pruefe --exe: die eigenstaendige exe gleicht der im Zip", not m, m)

        # --- Stufe 3: Zip mit bhed_q3map2.exe und Lizenztexten
        q3 = os.path.join(tmp, "bhed_q3map2.exe")
        schreibe(q3, _probe_exe(["KERNEL32.dll", "WS2_32.dll", "bcrypt.dll"]))
        q3_zip = os.path.join(tmp, "q3", "JK-Studio-rc900-windows-x64.zip")
        n3 = zip_bauen(exe, q3_zip, wurzel, handbuch=handbuch, q3map2=q3)
        with zipfile.ZipFile(q3_zip) as z:
            namen3 = z.namelist()
            ddslib = z.read(OBER + "/licenses/ddslib.txt").decode("utf-8")
        erwarte("zip --q3map2: bhed_q3map2.exe und %d Lizenztexte dabei, die Programme weiter zuletzt" % len(LIZENZEN),
                n3 == n + 1 + len(LIZENZEN) and OBER + "/bhed_q3map2.exe" in namen3
                and namen3[-1] == OBER + "/" + UEBERGANG and OBER + "/licenses/GPL-2.0.txt" in namen3)
        erwarte("zip --q3map2: aus einem Quelltextkopf nur der Lizenzblock, ohne Kommentarzeichen",
                "Mitte" in ddslib and "Kopf" not in ddslib and "Rest" not in ddslib and "//" not in ddslib)
        m = pruefe(q3_zip, rc, wurzel, ausgabe=still, q3map2=True)
        erwarte("pruefe --q3map2: das Zip mit bhed_q3map2.exe besteht", not m, m)
        with zipfile.ZipFile(q3_zip) as z:
            grund3 = [(i.filename, z.read(i)) for i in z.infolist()]

        # --- Kaputte Pakete: jedes muss auffallen
        with zipfile.ZipFile(gut_zip) as z:
            grund = [(i.filename, z.read(i)) for i in z.infolist()]

        def kaputt(was, eintraege, stichwort, name="JK-Studio-rc900-windows-x64.zip",
                   nachher=None, exe_pfad=None):
            ordner = tempfile.mkdtemp(dir=tmp)
            pfad = os.path.join(ordner, name)
            _zip_aus(pfad, eintraege)
            if nachher is not None:
                nachher(pfad)
            m = pruefe(pfad, rc, wurzel, exe_pfad=exe_pfad, ausgabe=still)
            treffer = [t for t in m if stichwort in t]
            erwarte("pruefe weist ab: " + was, bool(treffer), treffer + [t for t in m if stichwort not in t])

        def ersetze(name, inhalt):
            return [(n, inhalt if n == name else b) for n, b in grund]

        kaputt("Name ohne -windows-x64 (wie rc600)", grund, "erwartet JK-Studio-rc900-windows-x64.zip",
               name="JK-Studio-rc900.zip")
        kaputt("zwei oberste Ordner", grund + [("Anderes/liesmich.txt", b"x")], "obersten Ordner")
        kaputt("eine Datei ganz oben", grund + [("liesmich.txt", b"x")], "obersten Ordner")
        kaputt("Rueckstrich im Namen", grund + [(OBER + "/handbuch/a#b.html", b"x")], "Rueckstrich",
               nachher=lambda p: open(p, "r+b").write(open(p, "rb").read().replace(b"a#b", b"a\\b")))
        kaputt("'..' im Namen", grund + [(OBER + "/../boese.exe", b"x")], "'..'")
        kaputt("absoluter Name", grund + [("/abs/boese.exe", b"x")], "absoluter Name")
        kaputt("Laufwerksbuchstabe", grund + [("C:/boese.exe", b"x")], "absoluter Name")
        kaputt("Packverfahren 9 (Deflate64)", grund + [(OBER + "/handbuch/gross.bin", b"y" * 100)],
               "Packverfahren 9",
               nachher=lambda p: _packverfahren_setzen(p, OBER + "/handbuch/gross.bin", 9))
        kaputt("Ordnereintrag", grund + [(OBER + "/data/", b"")], "Ordnereintrag")
        kaputt("doppelter Eintrag", grund + [(OBER + "/handbuch/index.html", b"zwei")], "doppelter Eintrag")
        kaputt("die Programme vorn statt zuletzt (wie sortiert)",
               [e for e in grund if e[0].endswith(".exe")] + [e for e in grund if not e[0].endswith(".exe")],
               "nicht die letzten Eintraege")
        kaputt("jkstudio.exe fehlt", [(n, b) for n, b in grund if not n.endswith(PROGRAMM)], "jkstudio.exe fehlt")
        kaputt("behaved.exe (Uebergang fuer die alten Updater) fehlt", [(n, b) for n, b in grund if not n.endswith(UEBERGANG)],
               "behaved.exe fehlt")
        kaputt("behaved.exe ist nicht dieselbe Datei wie jkstudio.exe", ersetze(OBER + "/" + UEBERGANG, b"MZ anders"),
               "weicht von jkstudio.exe ab")
        kaputt("Handbuch ohne Japanisch", [(n, b) for n, b in grund if "/ja/" not in n], "handbuch/ja/index.html")
        kaputt("data/base weicht ab", ersetze(OBER + "/data/base/anims.h", b"anders"), "weicht vom Repo ab")
        kaputt("data/base unvollstaendig", [(n, b) for n, b in grund if not n.endswith("supplement.bhc")],
               "data/base/supplement.bhc fehlt")
        kaputt("exe mit VC-Laufzeit (wie rc600)",
               ersetze(OBER + "/" + PROGRAMM,
                       _probe_exe(["KERNEL32.dll", "MSVCP140.dll", "VCRUNTIME140.dll"], anhang=alle_daten)),
               "MSVCP140.dll")
        kaputt("exe mit fest gebundenem d3d11.dll",
               ersetze(OBER + "/" + PROGRAMM, _probe_exe(["KERNEL32.dll", "d3d11.dll"], anhang=alle_daten)),
               "d3d11.dll")
        kaputt("32-Bit-exe",
               ersetze(OBER + "/" + PROGRAMM, _probe_exe(["KERNEL32.dll"], maschine=0x14C, anhang=alle_daten)),
               "keine 64-Bit-exe")
        kaputt("exe ohne eingebaute Daten (wie rc600)",
               ersetze(OBER + "/" + PROGRAMM, _probe_exe(["KERNEL32.dll"])), "eingebauten Daten")
        kaputt("keine exe", ersetze(OBER + "/" + PROGRAMM, b"kein Programm"), "keine Windows-exe")
        fremd = os.path.join(tmp, "anders", "JK-Studio-rc900-windows-x64.exe")
        schreibe(fremd, _probe_exe(["KERNEL32.dll"], anhang=alle_daten + b"!"))
        kaputt("eigenstaendige exe weicht ab", grund, "weicht von jkstudio.exe im Zip ab", exe_pfad=fremd)
        falsch = os.path.join(tmp, "behaved-rc900.exe")
        shutil.copyfile(exe, falsch)
        kaputt("eigenstaendige exe falsch benannt", grund, "erwartet JK-Studio-rc900-windows-x64.exe",
               exe_pfad=falsch)
        m = pruefe(os.path.join(tmp, "behaved.exe"), rc, wurzel, ausgabe=still)
        erwarte("pruefe weist ab: gar kein Zip", any("kein lesbares Zip" in t for t in m), m)

        def kaputt3(was, eintraege, stichwort):
            ordner = tempfile.mkdtemp(dir=tmp)
            pfad = os.path.join(ordner, "JK-Studio-rc900-windows-x64.zip")
            _zip_aus(pfad, eintraege)
            m = pruefe(pfad, rc, wurzel, ausgabe=still, q3map2=True)
            treffer = [t for t in m if stichwort in t]
            erwarte("pruefe --q3map2 weist ab: " + was, bool(treffer), treffer + [t for t in m if stichwort not in t])

        kaputt3("bhed_q3map2.exe fehlt", grund, "bhed_q3map2.exe fehlt")
        kaputt3("Lizenztext fehlt", [(n, b) for n, b in grund3 if not n.endswith("/libpng.txt")], "libpng.txt")
        kaputt3("Lizenztext veraltet", [(n, b"alt" if n.endswith("/GPL-2.0.txt") else b) for n, b in grund3],
                "GPL-2.0.txt")
        kaputt3("bhed_q3map2.exe mit VC-Laufzeit",
                [(n, _probe_exe(["KERNEL32.dll", "VCRUNTIME140.dll"]) if n.endswith("/bhed_q3map2.exe") else b)
                 for n, b in grund3], "bhed_q3map2.exe braucht VCRUNTIME140.dll")

        # --- Die Notizen
        ziel = os.path.join(tmp, "notizen.md")
        text = notizen(rc, ziel, wurzel)
        erste = text.splitlines()[0]
        erwarte("notizen: die erste Zeile ist die Download-Zeile mit dem Zip",
                erste.startswith("**Download:** `JK-Studio-rc900-windows-x64.zip`"))
        erwarte("notizen: nennen die eigenstaendige exe, 'Source code' und SmartScreen",
                "JK-Studio-rc900-windows-x64.exe" in text and "Source code" in text and "Not affiliated" in text
                and "SmartScreen" in text)
        erwarte("notizen: der [en]-Block von rc900, nicht [de] und nicht rc899",
                "- New: a thing." in text and "- Fixed: another one." in text and "Neu: etwas" not in text
                and "- Old." not in text)
        with open(ziel, "rb") as f:
            roh = f.read()
        erwarte("notizen: UTF-8 mit LF (Gedankenstrich und Pfeil heil)",
                b"\r\n" not in roh and "\u2013".encode("utf-8") in roh and "\u2192".encode("utf-8") in roh)
        try:
            notizen_text("rc901", wurzel)
            erwarte("notizen: rc901 zu kFassung rc900 wird abgewiesen", False)
        except Fehler:
            erwarte("notizen: rc901 zu kFassung rc900 wird abgewiesen", True)
        schreibe(os.path.join(wurzel, "release-notes", "rc900.md"), "## Handgeschrieben\n\n- Text.\n")
        text2 = notizen_text(rc, wurzel)
        erwarte("notizen: release-notes/rc900.md geht vor, die Download-Zeile bleibt vorn",
                text2.startswith("**Download:**") and "## Handgeschrieben" in text2 and "New: a thing" not in text2)
        schreibe(os.path.join(wurzel, "tools", "changelog.txt"), "## rc899 2026-10-16\n[en]\n- Old.\n")
        os.remove(os.path.join(wurzel, "release-notes", "rc900.md"))
        try:
            notizen_text(rc, wurzel)
            erwarte("notizen: oberste Fassung im Changelog != rc900 wird abgewiesen", False)
        except Fehler:
            erwarte("notizen: oberste Fassung im Changelog != rc900 wird abgewiesen", True)

        # --- Pruefsummen
        summen = os.path.join(tmp, "SHA256SUMS.txt")
        zeilen = pruefsummen(summen, [gut_zip, allein])
        with open(summen, "rb") as f:
            roh = f.read()
        erwarte("pruefsummen: sha256sum-Format, nach Namen sortiert, LF",
                len(zeilen) == 2 and all(re.fullmatch(r"[0-9a-f]{64}  \S+\n", z) for z in zeilen)
                and zeilen[0].endswith("windows-x64.exe\n") and b"\r" not in roh
                and hashlib.sha256(open(gut_zip, "rb").read()).hexdigest() in zeilen[1])

    falsch = ergebnisse.count(False)
    print("Selbsttest release_paket: " + ("%d von %d richtig" % (len(ergebnisse), len(ergebnisse))
                                          if not falsch else "%d von %d FALSCH" % (falsch, len(ergebnisse))))
    return 1 if falsch else 0


# --- Aufruf -----------------------------------------------------------------------

def main(argv=None, ausgabe=print):
    p = argparse.ArgumentParser(description="Release-Paket von JK Studio bauen und pruefen")
    p.add_argument("--wurzel", default=WURZEL, help="Repo-Ordner (Vorgabe: der dieses Werkzeugs)")
    unter = p.add_subparsers(dest="befehl", required=True)
    f = unter.add_parser("fassung")
    f.add_argument("--marke")
    z = unter.add_parser("zip")
    z.add_argument("exe")
    z.add_argument("ziel")
    z.add_argument("--q3map2")
    q = unter.add_parser("pruefe")
    q.add_argument("zip")
    q.add_argument("--fassung", required=True)
    q.add_argument("--exe")
    q.add_argument("--q3map2", action="store_true")
    n = unter.add_parser("notizen")
    n.add_argument("rc")
    n.add_argument("ziel")
    s = unter.add_parser("pruefsummen")
    s.add_argument("ziel")
    s.add_argument("dateien", nargs="+")
    unter.add_parser("selbsttest")
    # --wurzel darf auch hinter dem Befehl stehen.
    argv = list(sys.argv[1:] if argv is None else argv)
    if "--wurzel" in argv:
        k = argv.index("--wurzel")
        argv = argv[k:k + 2] + argv[:k] + argv[k + 2:]
    a = p.parse_args(argv)
    wurzel = os.path.abspath(a.wurzel)
    try:
        if a.befehl == "fassung":
            rc = fassung_lesen(wurzel)
            if a.marke is not None and not marke_passt(a.marke, rc):
                ausgabe("Marke %s passt nicht zu kFassung %s - erwartet v1.0.0-%s (sonst boete der "
                        "Updater das Release ewig an)" % (a.marke, rc, rc))
                return 1
            ausgabe(rc)
            return 0
        if a.befehl == "zip":
            anzahl = zip_bauen(a.exe, a.ziel, wurzel, q3map2=a.q3map2)
            ausgabe("Zip: %s (%d Eintraege)" % (a.ziel, anzahl))
            return 0
        if a.befehl == "pruefe":
            return 1 if pruefe(a.zip, a.fassung, wurzel, exe_pfad=a.exe, ausgabe=ausgabe, q3map2=a.q3map2) else 0
        if a.befehl == "notizen":
            notizen(a.rc, a.ziel, wurzel)
            ausgabe("Notizen: %s" % a.ziel)
            return 0
        if a.befehl == "pruefsummen":
            for zeile in pruefsummen(a.ziel, a.dateien):
                ausgabe(zeile.rstrip("\n"))
            return 0
        if a.befehl == "selbsttest":
            return selbsttest()
    except (Fehler, OSError) as e:
        ausgabe("release_paket: %s" % e)
        return 2
    return 2


if __name__ == "__main__":
    sys.exit(main())
