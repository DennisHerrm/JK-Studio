#!/usr/bin/env python3
"""Verlaesst ein `return` die Funktion zwischen BeginTable und EndTable?

Warum das ein eigener Pruefer ist:

rc501 stuerzte beim Start ab:

    IMGUI-ZUSICHERUNG: (0) && "Missing EndTable()"

`drawEventsList` hat ZWEI Ausstiege - einen fuer Karte und Modell, einen am
Ende. Beim Umbau auf eine Tabelle habe ich nur den letzten gesehen und in
den Kommentar geschrieben, es gaebe keinen anderen. Gesucht hatte ich nicht.

Der Uebersetzer sagt dazu nichts: `return` ist gueltig, das fehlende
`EndTable` faellt erst zur Laufzeit auf - und dann sofort und hart.

Geprueft wird: in jeder Funktion, die `BeginTable` aufruft, muss auf jedes
`return` nach dem BeginTable ein `EndTable()` in den drei Zeilen davor
kommen. Der Ausstieg direkt bei fehlgeschlagenem BeginTable ist
ausgenommen - dort gibt es keine offene Tabelle.
"""
import re
import sys
from pathlib import Path

HIER = Path(__file__).resolve().parent


# rc609 F: ein `return` in einem Lambda auf derselben Zeile verlaesst nicht die
# Funktion (app_karte_entitaeten.cpp: std::all_of(..., [](char c) { return ...; })).
LAMBDA = re.compile(r"\[[^\]\[]*\]\s*\([^()]*\)\s*(?:mutable\s*)?(?:->\s*[\w:<>]+\s*)?\{[^{}]*\}")


def pruefe(t: str) -> list:
    """Zeilennummern (ab 1) der `return` zwischen BeginTable und EndTable."""
    t = re.sub(r"//[^\n]*", "", t)
    zeilen = t.split("\n")
    funde = []
    offen = False
    seitBegin = 0
    for nr, zeile in enumerate(zeilen):
        if "BeginTable(" in zeile:
            offen = True
            # Der Aufruf kann ueber mehrere Zeilen laufen. Der Ausstieg
            # bei GESCHEITERTEM BeginTable steht direkt hinter der
            # schliessenden Klammer - ab dort wird gezaehlt, nicht ab
            # dem Namen.
            tiefe = zeile.count("(") - zeile.count(")")
            seitBegin = nr
            k = nr
            while tiefe > 0 and k + 1 < len(zeilen):
                k += 1
                tiefe += zeilen[k].count("(") - zeilen[k].count(")")
            seitBegin = k
            continue
        if "EndTable()" in zeile:
            offen = False
            continue
        if not offen or "return" not in LAMBDA.sub("", zeile):
            continue
        # Der Ausstieg unmittelbar nach einem gescheiterten BeginTable
        # zaehlt nicht - dort ist keine Tabelle offen.
        if nr - seitBegin <= 3:
            continue
        umfeld = "\n".join(zeilen[max(0, nr - 3) : nr])
        if "EndTable()" in umfeld:
            continue
        funde.append(nr + 1)
    return funde


def selbsttest() -> int:
    # Ein echter Ausstieg muss gemeldet werden, ein Lambda-return nicht.
    kopf = ('void f() {\n    if (!ImGui::BeginTable("##t", 2)) {\n        return;\n    }\n'
            '    a();\n    b();\n    c();\n')
    echt = kopf + "    if (x) {\n        return;\n    }\n    ImGui::EndTable();\n}\n"
    lam = kopf + ("    const bool d = std::all_of(t.begin(), t.end(), [](char c) { return c != 0; });\n"
                  "    ImGui::EndTable();\n}\n")
    if pruefe(echt) != [9]:
        print("lint_endtable --selbsttest: echter Ausstieg NICHT erkannt", pruefe(echt))
        return 1
    if pruefe(lam):
        print("lint_endtable --selbsttest: return im Lambda faelschlich gemeldet", pruefe(lam))
        return 1
    print("lint_endtable --selbsttest: echter Ausstieg erkannt, Lambda nicht gemeldet")
    return 0


def main() -> int:
    if "--selbsttest" in sys.argv[1:]:
        return selbsttest()
    fehler = 0
    for datei in sorted((HIER.parent / "gui").glob("*.cpp")):
        for nr in pruefe(datei.read_text(encoding="utf-8")):
            print(
                "  %s:%d - `return` zwischen BeginTable und EndTable, ohne "
                "EndTable() davor. ImGui bricht dann zur Laufzeit ab: "
                '"Missing EndTable()".' % (datei.name, nr)
            )
            fehler += 1
    if fehler:
        print("EndTable-Pruefung: %d Beanstandung(en)" % fehler)
        return 1
    print("EndTable-Pruefung: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
