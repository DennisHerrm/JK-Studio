# -*- coding: utf-8 -*-
"""Ortskatalog fuer den Helfer (Stufe H1): wo im Programm etwas steht.

Liest die Stellen, an denen die Oberflaeche ihre Menues und Einstellungen
zeichnet, und schreibt src/hilfe_orte.inc - je Menuepunkt bzw. Einstellung
eine Zeile mit Zielangabe (include/bhed/tour.h), der Kennungskette fuer die
Anzeige, der Kennung des Hinweistexts und einer Vorbedingung.

  Hauptmenue      gui/app_anordnung.inc (BeginMenuBar ... ImGui::EndMenuBar)
  Modusleiste     gui/app_anordnung.inc ({n, Str::ViewX, Str::ModeXHint})
  Einstellungen   gui/app.cpp (drawPrefs: Checkbox, SliderFloat, Combo, Button)

Die Menues des Karten-Editors kommen zur Laufzeit aus
karte::befehle::menues() (src/hilfe.cpp) - sie stehen schon als Tabelle da.

Aufruf:  python tools/gen_hilfe_orte.py           (schreibt die Datei)
         python tools/gen_hilfe_orte.py --check   (bricht ab, wenn sie veraltet ist)
"""
import os
import re
import sys

WURZEL = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ZIEL = os.path.join(WURZEL, "src", "hilfe_orte.inc")

BEREICH = {
    "MenuFile": "programm", "MenuEdit": "programm", "MenuView": "einstellungen",
    "MenuDebug": "programm", "MenuHelp": "hilfe",
}


# Das Hauptmenue steht nicht im Karten-Editor (dort dessen eigene Menues):
# dann zeigt "Zeig es mir" zuerst auf den Knopf Events der Modusleiste.
HAUPTMENUE = "zahl:programm.hauptmenue < knopf:ViewEvents@modebar"


def lies(p):
    with open(os.path.join(WURZEL, p), encoding="utf-8") as f:
        return f.read().split("\n")


def bereich_von(kette):
    k = ">".join(kette)
    if "Upd" in k:
        return "updates"
    if "Model" in k or "Glm" in k or "Skin" in k or "Gla" in k:
        return "modell"
    if "Mission" in k:
        return "mission"
    if "Map" in k or "Ent" in k:
        return "vorschau"
    return BEREICH.get(kette[0], "programm")


def hauptmenue(zeilen):
    aus = []
    start = next(i for i, z in enumerate(zeilen) if "if (!ImGui::BeginMenuBar()) { return; }" in z)
    stapel = []
    letzter = None
    for i in range(start + 1, len(zeilen)):
        z = zeilen[i]
        if "ImGui::EndMenuBar()" in z:
            break
        m = re.search(r"ImGui::BeginMenu\(tr\(Str::(\w+)\)", z)
        if m:
            stapel.append(m.group(1))
            # das Untermenue selbst ist auch ein Ort ("View > Language")
            aus.append([bereich_von(stapel), "menue:" + ">".join(stapel), ">".join(stapel), "", HAUPTMENUE])
            letzter = None
            continue
        if "ImGui::EndMenu()" in z:
            if stapel:
                stapel.pop()
            letzter = None
            continue
        m = re.search(r"ImGui::MenuItem\(tr\(Str::(\w+)\)", z)
        if m and stapel:
            kette = stapel + [m.group(1)]
            letzter = [bereich_von(kette), "menue:" + ">".join(kette), ">".join(kette), "", HAUPTMENUE]
            aus.append(letzter)
            continue
        m = re.search(r"SetTooltip\(\"%s\", tr\(Str::(\w+)\)\)", z)
        if m and letzter is not None and not letzter[3]:
            letzter[3] = m.group(1)
    return aus


def modusleiste(zeilen):
    aus = []
    for z in zeilen:
        m = re.match(r"\s*\{(\d), Str::(View\w+), Str::(Mode\w+Hint)\},", z)
        if m:
            aus.append(["programm", "knopf:%s@modebar" % m.group(2), m.group(2), m.group(3), ""])
    return aus


def einstellungen(zeilen):
    aus = []
    start = next(i for i, z in enumerate(zeilen) if z.startswith("void drawPrefs() {"))
    letzter = None
    abschnitt = ""
    for i in range(start + 1, len(zeilen)):
        z = zeilen[i]
        if z.startswith("}"):
            break
        m = re.search(r"SeparatorText\(tr\(Str::(\w+)\)\)", z)
        if m:
            abschnitt = m.group(1)
            continue
        m = re.search(r"ImGui::(?:Checkbox|SliderFloat|SliderInt|Combo|Button)\(tr\(Str::(\w+)\)", z)
        if m and m.group(1) not in ("EditorOk", "EditorCancel"):
            kette = ["MenuEdit", "AppPrefs"] + ([abschnitt] if abschnitt else []) + [m.group(1)]
            letzter = ["einstellungen", "knopf:" + m.group(1), ">".join(kette), "",
                       "zahl:programm.einstellungen < menue:MenuEdit>AppPrefs"]
            aus.append(letzter)
            continue
        m = re.search(r"SetTooltip\(\"%s\", tr\(Str::(\w+)\)\)", z)
        if m and letzter is not None and not letzter[3]:
            letzter[3] = m.group(1)
    return aus


def text():
    anordnung = lies("gui/app_anordnung.inc")
    app = lies("gui/app.cpp")
    orte = hauptmenue(anordnung) + modusleiste(anordnung) + einstellungen(app)
    gesehen = set()
    zeilen = ["// Erzeugt von tools/gen_hilfe_orte.py - nicht von Hand aendern.",
              "// Ortskatalog des Helfers (Stufe H1): bereich, ziel, kette, hinweis, voraus."]
    for o in orte:
        if o[1] in gesehen:
            continue
        gesehen.add(o[1])
        zeilen.append("{" + ", ".join('"%s"' % x for x in o) + "},")
    return "\n".join(zeilen) + "\n"


def main():
    neu = text()
    if "--check" in sys.argv:
        with open(ZIEL, encoding="utf-8", newline="") as f:
            if f.read() != neu:
                sys.exit("src/hilfe_orte.inc ist veraltet - python tools/gen_hilfe_orte.py")
        return
    with open(ZIEL, "w", encoding="utf-8", newline="") as f:
        f.write(neu)
    print("src/hilfe_orte.inc: %d Orte" % (neu.count("\n") - 2))


if __name__ == "__main__":
    main()
