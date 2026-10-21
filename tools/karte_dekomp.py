# -*- coding: utf-8 -*-
"""
Daten fuer ctest karte_massenlast: alle BSPs eines Spiels mit q3map2
dekompilieren, einmal als Brush Primitives und einmal als Quake3-Texdef.

  python -I tools/karte_dekomp.py <q3map2.exe> <ziel> <name>:<spiel>:<basepath>[:<mod>] ...

Beispiel (Pfade anpassen, die Spielordner werden nur gelesen):

  python -I tools/karte_dekomp.py q3map2.exe D:/dekomp \
      "ja:ja:C:/.../Jedi Academy/GameData original" \
      "jk2:jk2:C:/.../Jedi Outcast/GameData" \
      "md:ja:C:/.../Jedi Academy/GameData Movie Duels:MD"

Holt maps/*.bsp aus allen .pk3 von <basepath>/<mod oder base> nach
<ziel>/<name>/bsp (spaetere .pk3 zuerst; aeltere, verdeckte Fassungen
derselben Karte bekommen den Paketnamen angehaengt und werden mitgeprueft)
und ruft fuer jede

  q3map2 -game <spiel> -fs_basepath <basepath>/ [-fs_game <mod>] -convert -format map_bp|map <bsp>

auf. q3map2 schreibt <karte>_converted.map neben die .bsp; die Datei wandert
nach <ziel>/<name>/bp bzw. <ziel>/<name>/q3. Schon vorhandene Ergebnisse
werden uebersprungen. Danach:

  kartemassenlast <ziel>      (bzw. cmake -DBEHAVED_KARTE_DEKOMP=<ziel>)
"""
import os
import shutil
import subprocess
import sys
import time
import zipfile


def bsps_holen(ordner, bspdir):
    gesehen = set()
    for pk in sorted(os.listdir(ordner), key=str.lower, reverse=True):   # spaetere zuerst: deren Fassung gilt
        if not pk.lower().endswith(".pk3"):
            continue
        with zipfile.ZipFile(os.path.join(ordner, pk)) as z:
            for n in z.namelist():
                nl = n.lower()
                if not (nl.startswith("maps/") and nl.endswith(".bsp")):
                    continue
                rel = nl[5:].replace("/", "__")
                if rel in gesehen:
                    rel = rel[:-4] + "__" + pk[:-4].lower() + ".bsp"
                    if rel in gesehen:
                        continue
                gesehen.add(rel)
                with open(os.path.join(bspdir, rel), "wb") as f:
                    f.write(z.read(n))
    return sorted(gesehen)


def main():
    if len(sys.argv) < 4:
        print(__doc__)
        return 2
    q3map2, ziel = sys.argv[1], sys.argv[2]
    fehler_gesamt = 0
    for angabe in sys.argv[3:]:
        teile = angabe.split(":")
        # Laufwerksbuchstabe im Pfad ("C:/...") wieder zusammensetzen
        if len(teile) >= 4 and len(teile[2]) == 1:
            teile = teile[:2] + [teile[2] + ":" + teile[3]] + teile[4:]
        name, spiel, basis = teile[0], teile[1], teile[2]
        mod = teile[3] if len(teile) > 3 else None
        bspdir = os.path.join(ziel, name, "bsp")
        os.makedirs(bspdir, exist_ok=True)
        bsps = bsps_holen(os.path.join(basis, mod or "base"), bspdir)
        print(name, len(bsps), "BSPs", flush=True)
        for fmt, unter in (("map_bp", "bp"), ("map", "q3")):
            aus = os.path.join(ziel, name, unter)
            os.makedirs(aus, exist_ok=True)
            t0 = time.time()
            fehler = 0
            for b in bsps:
                if os.path.exists(os.path.join(aus, b[:-4] + ".map")):
                    continue
                cmd = [q3map2, "-game", spiel, "-fs_basepath", basis + "/"]
                if mod:
                    cmd += ["-fs_game", mod]
                cmd += ["-convert", "-format", fmt, os.path.join(bspdir, b)]
                r = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=900)
                erzeugt = os.path.join(bspdir, b[:-4] + "_converted.map")
                if r.returncode != 0 or not os.path.exists(erzeugt):
                    fehler += 1
                    print("  FEHLER", fmt, b, r.returncode, flush=True)
                    continue
                shutil.move(erzeugt, os.path.join(aus, b[:-4] + ".map"))
            print(name, fmt, "fertig", round(time.time() - t0), "s, Fehler", fehler, flush=True)
            fehler_gesamt += fehler
    return 1 if fehler_gesamt else 0


if __name__ == "__main__":
    sys.exit(main())
