#!/usr/bin/env python3
"""
Jedes Menue geht ueber ui::menue, jedes Rechtsklick-Menue ueber ui::popup / popupKontext / popupKontextFenster (gui/ui_menue.h),
nicht direkt ueber ImGui::BeginMenu / BeginPopup / BeginPopupContextItem / BeginPopupContextWindow.

Anlass (KLEIN2, 13.11.): das Untermenue Animation > Animation tools ragte unten aus dem Hauptfenster und wurde ein eigenes
Windows-Fenster - ImGui rechnet die Lage eines Popups mit mehreren Viewports im Rechteck des Bildschirms, nicht des Fensters.
ui::menue legt das Menue in den Viewport des Fensters, aus dem es aufgeht: passt es unten nicht, geht es nach oben auf und bleibt
im Fenster, wie ein Windows-Menue am Rand. Das gilt nur, wenn ALLE Menues darueber laufen - ein neues ImGui::BeginMenu haette den
alten Fehler wieder. Erlaubte Stellen tragen den Kommentar "lint-menue:" in derselben Zeile (ui_menue.cpp, der Stil von
NetRadiant im Karten-Editor). Die Popups des Karten-Editors (gui/app_karte*, gui/karte_*) bilden Qt nach (Menues bis zum
Bildschirmrand) und sind fuer BeginPopup* ausgenommen; ihre Menues pruefe ich trotzdem.

Mit `--gegentest` laufen Textproben mit und ohne Fehler durch dieselbe Pruefung.
"""
import pathlib
import re
import sys

MENUE = re.compile(r"\bImGui::BeginMenu(?:Ex)?\s*\(")
POPUP = re.compile(r"\bImGui::BeginPopup(?:ContextItem|ContextWindow|ContextVoid)?\s*\(")


def karteneditor(name):
    return name.startswith("app_karte") or name.startswith("karte_")


def funde(text, popups=True):
    """Zeilennummern (ab 1) mit einem direkten ImGui::BeginMenu (und BeginPopup*, wenn `popups`) ohne Freigabe; Kommentare zaehlen
    nicht."""
    out = []
    for i, zeile in enumerate(text.split("\n"), 1):
        code = zeile.split("//", 1)[0]
        if (MENUE.search(code) or (popups and POPUP.search(code))) and "lint-menue:" not in zeile:
            out.append(i)
    return out


def gegentest():
    proben = [
        ("    if (ImGui::BeginMenu(tr(Str::MenuFile))) {", 1),
        ("    const bool offen = ImGui::BeginMenu(text, an);   // lint-menue: die eine Stelle", 0),
        ("    if (ui::menue(tr(Str::MenuFile))) {", 0),
        ("    // frueher: ImGui::BeginMenu(tr(Str::MenuFile))", 0),
        ("    if (ImGui::BeginMenuEx(\"x\", nullptr, true)) {", 1),
        ("    if (ImGui::BeginMenuBar()) {", 0),
        ("    if (ImGui::BeginPopup(\"##mzmenue\")) {", 1),
        ("    if (ImGui::BeginPopupContextItem(\"##m\")) {", 1),
        ("    if (ImGui::BeginPopupContextWindow(\"##x\", 1)) {", 1),
        ("    if (ImGui::BeginPopupModal(\"Frage\")) {", 0),
        ("    if (ui::popup(\"##mzmenue\")) {", 0),
    ]
    schlecht = 0
    for text, soll in proben:
        ist = len(funde(text))
        if ist != soll:
            schlecht += 1
            print("  %-70s erwartet %d, bekam %d" % (text.strip(), soll, ist))
    if schlecht:
        print("Gegentest: %d von %d Proben falsch" % (schlecht, len(proben)))
        return 1
    print("Gegentest: %d von %d Proben richtig" % (len(proben), len(proben)))
    return 0


def main():
    if "--gegentest" in sys.argv:
        return gegentest()
    wurzel = pathlib.Path(__file__).resolve().parent.parent
    schlecht = 0
    for f in sorted((wurzel / "gui").rglob("*")):
        if f.suffix not in (".cpp", ".h", ".inc"):
            continue
        for n in funde(f.read_text(encoding="utf-8", errors="replace"), popups=not karteneditor(f.name) and not f.name.startswith("selbsttest")):
            print("%s:%d: ImGui::BeginMenu/BeginPopup* direkt - ui::menue / ui::popup* nehmen (gui/ui_menue.h)" % (f.relative_to(wurzel).as_posix(), n))
            schlecht += 1
    if schlecht:
        print("lint_menue: %d Stellen" % schlecht)
        return 1
    print("lint_menue: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
