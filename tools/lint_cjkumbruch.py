#!/usr/bin/env python3
"""Bricht jeder umbrochene Text der Oberflaeche chinesische und japanische Saetze zwischen den Zeichen um?

Warum das ein eigener Pruefer ist:

ImGui bricht nur an Leerzeichen und nach `.,;!?"` sowie `、。` um (ImFontCalcWordWrapPositionEx, imgui_draw.cpp,
die Zeichenklassen sind dort fest). Ein japanischer oder chinesischer Satz hat keine Leerzeichen: die erste Zeile
endet am ersten Komma, der Rest wird zeichenweise am Rand geteilt (p_render, NOTIZEN_render_p.md Abschnitt 4).
`gui/textumbruch.h` bricht nach JIS X 4051 / GB/T 15834 um - aber nur dort, wo es aufgerufen wird. Der Uebersetzer
merkt nicht, wenn ein neues Fenster wieder `ImGui::TextWrapped` nimmt; im Englischen sieht es gleich aus.

Geprueft wird in gui/ (ausser textumbruch.cpp):
- `ImGui::TextWrapped(` - immer (bricht selbst um), statt dessen `cjkTextWrapped`;
- `ImGui::Text(`, `TextDisabled(`, `TextColored(`, `TextUnformatted(` zwischen `PushTextWrapPos(` und
  `PopTextWrapPos(` (in der Reihenfolge der Datei) - statt dessen die `cjk...`-Gegenstuecke;
- `CalcTextSize(text, ende, verstecken, breite)` mit Umbruchbreite (vier Argumente) - statt dessen `cjkTextSize`,
  sonst passt die reservierte Hoehe nicht zum gezeichneten Text;
- `AddText(schrift, groesse, ort, farbe, text, ende, breite)` mit Umbruchbreite (sieben Argumente) - statt dessen
  `cjkAddText`.
`--selbsttest` prueft den Pruefer an eingebauten Beispielen.
"""
import re
import sys
from pathlib import Path

HIER = Path(__file__).resolve().parent
GUI = HIER.parent / "gui"
AUSGENOMMEN = {"textumbruch.cpp", "textumbruch.h"}

TEXT_IM_UMBRUCH = re.compile(r"\bImGui::(Text|TextDisabled|TextColored|TextUnformatted)\s*\(")
TEXT_WRAPPED = re.compile(r"\bImGui::TextWrapped\s*\(")
PUSH = re.compile(r"\bPushTextWrapPos\s*\(")
POP = re.compile(r"\bPopTextWrapPos\s*\(")
CALC = re.compile(r"\bCalcTextSize\s*\(")
ADDTEXT = re.compile(r"(?:->|\.)\s*AddText\s*\(")


def ohne_kommentare(t: str) -> str:
    """Kommentare, Zeichenketten und Zeichen durch Leerzeichen ersetzen (Zeilenumbrueche bleiben)."""
    aus = []
    i = 0
    n = len(t)
    while i < n:
        c = t[i]
        if t.startswith("//", i):
            j = t.find("\n", i)
            j = n if j < 0 else j
            aus.append(" " * (j - i))
            i = j
        elif t.startswith("/*", i):
            j = t.find("*/", i + 2)
            j = n if j < 0 else j + 2
            aus.append("".join(ch if ch == "\n" else " " for ch in t[i:j]))
            i = j
        elif t.startswith('R"', i) and (i == 0 or not (t[i - 1].isalnum() or t[i - 1] == "_")):
            k = t.find("(", i)
            trenner = ")" + t[i + 2:k] + '"'
            j = t.find(trenner, k)
            j = n if j < 0 else j + len(trenner)
            aus.append('"' + "".join(ch if ch == "\n" else " " for ch in t[i + 1:j - 1]) + '"')
            i = j
        elif c in "\"'":
            j = i + 1
            while j < n and t[j] != c and t[j] != "\n":
                j += 2 if t[j] == "\\" else 1
            j = min(j + 1, n)
            aus.append(c + " " * (j - i - 2) + c if j - i >= 2 else c)
            i = j
        else:
            aus.append(c)
            i += 1
    return "".join(aus)


def argumente(t: str, auf: int) -> int:
    """Zahl der Argumente des Aufrufs, dessen Klammer bei t[auf] oeffnet (-1, wenn sie nicht schliesst)."""
    tiefe = 0
    zahl = 0
    leer = True
    for k in range(auf, len(t)):
        c = t[k]
        if c in "([{":
            tiefe += 1
            if tiefe == 1:
                continue
        elif c in ")]}":
            tiefe -= 1
            if tiefe == 0:
                return 0 if leer and zahl == 0 else zahl + 1
        elif c == "," and tiefe == 1:
            zahl += 1
            continue
        if tiefe >= 1 and not c.isspace():
            leer = False
    return -1


def pruefe_text(name: str, quelle: str) -> list[str]:
    t = ohne_kommentare(quelle)
    funde = []
    ereignisse = []
    for rx, art in ((PUSH, "push"), (POP, "pop"), (TEXT_IM_UMBRUCH, "text"), (TEXT_WRAPPED, "wrapped"), (CALC, "calc"),
                    (ADDTEXT, "add")):
        for m in rx.finditer(t):
            ereignisse.append((m.start(), art, m))
    ereignisse.sort(key=lambda e: e[0])
    tiefe = 0
    for pos, art, m in ereignisse:
        zeile = t.count("\n", 0, pos) + 1
        if art == "push":
            tiefe += 1
        elif art == "pop":
            tiefe = max(0, tiefe - 1)
        elif art == "wrapped":
            funde.append(f"{name}:{zeile}: ImGui::TextWrapped bricht CJK nur an Satzzeichen um - cjkTextWrapped")
        elif art == "text" and tiefe > 0:
            funde.append(f"{name}:{zeile}: ImGui::{m.group(1)} im Umbruch (PushTextWrapPos) - cjk{m.group(1)}")
        elif art == "calc" and argumente(t, m.end() - 1) >= 4:
            funde.append(f"{name}:{zeile}: CalcTextSize mit Umbruchbreite - cjkTextSize")
        elif art == "add" and argumente(t, m.end() - 1) >= 7:
            funde.append(f"{name}:{zeile}: AddText mit Umbruchbreite - cjkAddText")
    return funde


def pruefe_ordner(ordner: Path) -> list[str]:
    funde = []
    for p in sorted(ordner.iterdir()):
        if p.suffix not in (".cpp", ".inc", ".h") or p.name in AUSGENOMMEN:
            continue
        funde += pruefe_text(p.name, p.read_text(encoding="utf-8", errors="replace"))
    return funde


def selbsttest() -> int:
    faelle = [
        ("ImGui::TextWrapped(\"%s\", t);", 1),
        ("cjkTextWrapped(\"%s\", t);", 0),
        ("ImGui::PushTextWrapPos(0.0F);\nImGui::TextUnformatted(t);\nImGui::PopTextWrapPos();\nImGui::Text(\"x\");", 1),
        ("ImGui::PushTextWrapPos(0.0F);\ncjkTextDisabled(\"%s\", t);\nImGui::PopTextWrapPos();", 0),
        ("ImGui::Text(\"PushTextWrapPos(\");  // ImGui::TextWrapped(\nImGui::Text(\"a\");", 0),
        ("const float h = ImGui::CalcTextSize(t, nullptr, false, breite).y;", 1),
        ("const float h = ImGui::CalcTextSize(f(a, b), nullptr).x;", 0),
        ("dl->AddText(ImGui::GetFont(), fs, p, c, t, nullptr, breite);", 1),
        ("dl->AddText(font, fs, ImVec2{x, y}, c, t, t + n);", 0),
        ("dl->AddText(ImVec2{x, y}, c, t);", 0),
        ("ImGui::PushTextWrapPos(0.0F);\nImGui::TextColored(ImVec4(1, 0, 0, 1), \"%s\", t);\nImGui::PopTextWrapPos();", 1),
    ]
    fehler = 0
    for code, erwartet in faelle:
        n = len(pruefe_text("beispiel", code))
        if n != erwartet:
            print(f"FEHLER: {n} statt {erwartet} Funde in: {code!r}")
            fehler += 1
    print(f"lint_cjkumbruch --selbsttest: {len(faelle) - fehler}/{len(faelle)} gut")
    return 1 if fehler else 0


def main() -> int:
    if "--selbsttest" in sys.argv[1:]:
        return selbsttest()
    funde = pruefe_ordner(GUI)
    for f in funde:
        print(f)
    print(f"lint_cjkumbruch: {len(funde)} Stellen ohne CJK-Umbruch")
    return 1 if funde else 0


if __name__ == "__main__":
    sys.exit(main())
