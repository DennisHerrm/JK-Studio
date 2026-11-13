# -*- coding: utf-8 -*-
"""Ortskatalog fuer den Helfer (Stufe H1): wo im Programm etwas steht.

Liest die Stellen, an denen die Oberflaeche ihre Menues und Einstellungen
zeichnet, und schreibt src/hilfe_orte.inc - je Menuepunkt bzw. Einstellung
eine Zeile mit Zielangabe (include/bhed/tour.h), der Kennungskette fuer die
Anzeige, der Kennung des Hinweistexts und einer Vorbedingung.

  Hauptmenue      gui/app_anordnung.inc (BeginMenuBar ... ImGui::EndMenuBar)
  Modusleiste     gui/app_anordnung.inc ({n, Str::ViewX, Str::ModeXHint})
  Einstellungen   gui/app.cpp (drawPrefs: Checkbox, SliderFloat, Combo, Button)
  H2:
  Pfade           gui/app.cpp (Fenster "###paths": Knoepfe; Abschnitte als Inhalt)
  Leistenknoepfe  iconKnopf/iconTextKnopf/leistenKnopf(tr(Str::X), UiIcon::.., tr(Str::Hinweis))
                  der Vorschau, Zeitleiste, Kamera, Wegpunkte und des Modell-Tabs
  Karten-Editor   gui/app_karte_dialoge.cpp: die Seiten der Einstellungen
                  (kSeiten), Ziel ist die Seite (selbst gezeichnet gemeldet:
                  gezeichnet:karte.einstseite.<n>), die Felder der Seite als Inhalt
  Menue-Editor    gui/app_menue.cpp: Knoepfe, Umschalter und Haken der Leiste
                  (seit M2 Symbolknoepfe iconKnopf(tr(Str::X), UiIcon::.., tr(Str::Hinweis)))
  AP14:
  Model-Tab       das rechte Feld (Baustein gui/ui_rollout.h): je Gruppe (ui::rolloutBeginn) ein Ort mit allem, was in ihr
                  steht, als Inhalt, und je Knopf, Schalter und Regler mit Hinweis ein eigener Ort. Die Dateien und ihr
                  Bereich stehen in MODELL_FELD - eine neue Datei des Felds kommt mit einer Zeile dazu. "Zeig es mir" fuehrt
                  ueber den Model-Tab und den Bereich zur Gruppe und, wenn sie offen ist, zum Element.
Spalten: bereich, ziel, kette, hinweis, voraus, inhalt (H2: Kennungen, die im
Untermenue/Dialog stehen - nur Suchwoerter).
H2: Vorbedingungen verkettet ("&&", include/bhed/tour.h): eine Einstellung des
Hauptprogramms, gesucht aus dem Karten-Editor, zeigt erst auf Events, dann auf
Bearbeiten > Einstellungen, dann auf das Feld.

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
EINSTELLUNGEN = HAUPTMENUE + " && zahl:programm.einstellungen < menue:MenuEdit>AppPrefs"
PFADE = HAUPTMENUE + " && zahl:programm.pfade < menue:MenuEdit>PathsTitle"
VORSCHAU = "zahl:vorschau.bereit < knopf:ViewMap@modebar"
MODELL = "modus:2 < knopf:ViewModel@modebar"
KARTE = "modus:3 < knopf:ViewMapEditor@modebar"
MENUE = "modus:4 < knopf:ViewMenuEditor@modebar"


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
        m = re.search(r"(?:ImGui::BeginMenu|ui::menue)\(tr\(Str::(\w+)\)", z)   # KLEIN2: Menues ueber ui::menue
        if m:
            stapel.append(m.group(1))
            # das Untermenue selbst ist auch ein Ort ("View > Language")
            aus.append([bereich_von(stapel), "menue:" + ">".join(stapel), ">".join(stapel), "", HAUPTMENUE, ""])
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
            letzter = [bereich_von(kette), "menue:" + ">".join(kette), ">".join(kette), "", HAUPTMENUE, ""]
            aus.append(letzter)
            continue
        m = re.search(r"SetTooltip\(\"%s\", tr\(Str::(\w+)\)\)", z)
        if m and letzter is not None and not letzter[3]:
            letzter[3] = m.group(1)
    return aus


def fenstermenue():
    """dock1/dock2: das Menue "Fenster" (gui/fenster_dock.cpp, menueInhalt) - in
    jedem Bereich hinter den Menues des Hauptfensters bzw. NRCs."""
    zeilen = lies("gui/fenster_dock.cpp")
    start = next(i for i, z in enumerate(zeilen) if z.startswith("void menueInhalt(int bereich) {"))
    aus = []
    stapel = ["DockMenu"]
    aus.append(["programm", "menue:DockMenu", "DockMenu", "", "", ""])
    letzter = None
    for z in zeilen[start + 1:]:
        if z.startswith("}"):
            break
        m = re.search(r"(?:ImGui::BeginMenu|ui::menue)\(tr\(Str::(\w+)\)", z)   # KLEIN2: Menues ueber ui::menue
        if m:
            kette = stapel + [m.group(1)]
            aus.append(["programm", "menue:" + ">".join(kette), ">".join(kette), "", "", ""])
            letzter = None
            continue
        m = re.search(r"ImGui::MenuItem\(tr\(Str::(\w+)\)", z)
        if m:
            kette = stapel + [m.group(1)]
            letzter = ["programm", "menue:" + ">".join(kette), ">".join(kette), "", "", ""]
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
            aus.append(["programm", "knopf:%s@modebar" % m.group(2), m.group(2), m.group(3), "", ""])
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
            letzter = ["einstellungen", "knopf:" + m.group(1), ">".join(kette), "", EINSTELLUNGEN, ""]
            aus.append(letzter)
            continue
        m = re.search(r"SetTooltip\(\"%s\", tr\(Str::(\w+)\)\)", z)
        if m and letzter is not None and not letzter[3]:
            letzter[3] = m.group(1)
    return aus


def pfade(zeilen):
    """H2: das Fenster Edit > Paths - seine Knoepfe als Orte, die Abschnitte als Inhalt."""
    aus = []
    inhalt = []
    start = next(i for i, z in enumerate(zeilen) if '"###paths"' in z)
    tiefe = 0
    for i in range(start, len(zeilen)):
        z = zeilen[i]
        if z.startswith("}"):
            break
        # uebersetzung1: Zaehltexte kommen ueber trN(Str::X, zahl, ...)
        # MR4 Punkt 5: umbrochene Texte gehen ueber gui/textumbruch.h (cjkTextDisabled, cjkTextUnformatted, ...)
        for m in re.finditer(r"(?:ImGui::|cjk)(?:SeparatorText|TextDisabled|TextUnformatted|TextWrapped)\((?:\"%s\", )?trN?\(Str::(\w+)[,)]", z):
            inhalt.append(m.group(1))
        m = re.search(r"ImGui::(?:Button|Checkbox)\(tr\(Str::(\w+)\)", z)
        if m:
            aus.append(["programm", "knopf:%s@###paths" % m.group(1), "MenuEdit>PathsTitle>" + m.group(1), "", PFADE, ""])
            inhalt.append(m.group(1))
    return aus, inhalt


# H2: Leistenknoepfe - Datei, Bereich, Vorbedingung, Kette (Anzeige vor dem Knopf).
LEISTEN = [
    ("gui/app_view3d.cpp", "vorschau", VORSCHAU, "ViewMap"),
    ("gui/app_view3d_bedienung.inc", "vorschau", VORSCHAU, "ViewMap"),
    ("gui/app_kamera.cpp", "vorschau", VORSCHAU + " && zahl:vorschau.kameragewaehlt < gezeichnet:zeitleiste.kamera", "ViewMap>ZlOrt"),
    ("gui/app_wegpunkte.cpp", "vorschau", VORSCHAU + " && zahl:vorschau.wegfenster < gezeichnet:vorschau.wegknopf", "ViewMap>WegTitel"),
    ("gui/app_modelltab_explorer.cpp", "modell", MODELL, "ViewModel"),   # AP2: Namen kopieren, Ins Skript
    ("gui/app_modell.cpp", "modell", MODELL, "ViewModel"),
    ("gui/app_modelltab_zeit.cpp", "modell", MODELL, "ViewModel"),   # AP1: die Zeitleiste des Model-Tabs
]


# Knoepfe, deren Beschriftung auch anderswo steht: als selbst gemeldetes Ziel.
EINDEUTIG = {"WegTitel": "gezeichnet:vorschau.wegknopf"}


def leisten():
    aus = []
    for datei, bereich, voraus, vor in LEISTEN:
        for z in lies(datei):
            m = re.search(r"icon(?:Text)?Knopf\(tr\(Str::(\w+)\), *UiIcon::\w+, *tr\(Str::(\w+)\)", z)
            if not m and datei.endswith("app_modelltab_zeit.cpp"):
                # AP1: knopf(tr(Str::X), <Symbol>, <Text>, tr(Str::Hinweis), ...) - die Knoepfe der Zeitleiste
                m = re.search(r"\bknopf\(tr\(Str::(\w+)\), *[^,]+, *[^,]+, *tr\(Str::(\w+)\)", z)
            if not m:
                continue
            ziel = EINDEUTIG.get(m.group(1), "knopf:%s" % m.group(1))
            if datei.endswith("app_wegpunkte.cpp"):
                ziel += "@###wegpunkte"
            aus.append([bereich, ziel, vor + ">" + m.group(1), m.group(2), voraus, ""])
    return aus


# AP14: das rechte Feld des Model-Tabs - Datei und Bereich (ModellBereich: 0 Ansehen/Pruefen, 1 Modellieren, 2 UV, 3 Weights,
# 4 Painting, 5 Animation). Eine Datei, die Gruppen des Felds zeichnet, kommt mit einer Zeile dazu.
MODELL_FELD = [
    ("gui/app_modelltab.cpp", 0),
    ("gui/app_modelltab_pruefen.cpp", 0),
    ("gui/wof/app_modelltab_mesh.cpp", 1),
    ("gui/wof/app_modelltab_uv.cpp", 2),
    ("gui/app_modelltab_gewichte.cpp", 3),
    ("gui/app_modelltab_autoskin.cpp", 3),
    ("gui/app_modelltab_malen.cpp", 4),
    ("gui/app_modelltab_mb3.cpp", 5),
    ("gui/app_modelltab_rig.cpp", 5),
    ("gui/app_modelltab_retarget.cpp", 5),
    ("gui/app_modelltab_kurven.cpp", 5),
    ("gui/app_modelltab_ik.cpp", 5),
    ("gui/app_modelltab_anim.cpp", 5),
    ("gui/app_modelltab_physik.cpp", 5),
    ("gui/app_modelltab_stapel.cpp", 1),     # AP13S: Modifier-Stapel (auch im Bereich Weights; der Ort fuehrt nach Modellieren)
    ("gui/app_modelltab_knochen.cpp", 5),    # AP13K: Knochen- und Rig-Darstellung (in der Gruppe Rig des Bereichs Animation)
]
MODELL_BEREICHE = ["MbBereichAnsehen", "MbBereichModellieren", "MbBereichUv", "MbBereichWeights", "MbBereichPainting", "MbBereichAnimation"]


def modellFeld():
    """AP14: die Gruppen des rechten Felds und ihre Knoepfe, Schalter und Regler.

    Eine Gruppe ist eine Funktion mit ui::rolloutBeginn(tr(Str::Titel) ...). Ihr gehoert, was sie selbst zeichnet und was
    Hilfsfunktionen zeichnen, die sie aufruft (auch aus einer anderen Datei: drawModellAutoSkinGruppe). Ein Element, das in
    mehreren Gruppen steht (wie 3ds Max in "Gewichts-Werkzeug", wie Blender in "Pinsel"), ist EIN Ort; seine Vorbedingung
    nennt alle diese Gruppen als Oeffner - die Tour zeigt auf die, die im Bedienmodus da ist.
    Vorbedingungen: Model-Tab, Bereich (zahl:modell.bereich<n>, gemeldet von gui/app_modelltab_feld.cpp), und fuer ein Element
    zahl:modell.feld.<kennung> (klein: Namen von zahl: nur aus a-z, 0-9, '.', '_'; gui/tour.cpp loest sie ohne Gross-/Kleinschreibung
    auf - das Element steht gerade im Feld) - sonst zeigt die Tour auf die Ueberschrift
    seiner Gruppe."""
    kennungen = bekannte_kennungen()
    funktion = re.compile(r"^(?:static )?[\w:<>&\*,\[\] ]+?[ \*&](\w+)\([^;]*\) (?:const )?\{$")
    funktionen = []   # (datei, bereich, name, rumpf)
    for datei, bereich in MODELL_FELD:
        zeilen = lies(datei)
        i = 0
        while i < len(zeilen):
            m = funktion.match(zeilen[i])
            if not m:
                i += 1
                continue
            j = i + 1
            while j < len(zeilen) and not zeilen[j].startswith("}"):
                j += 1
            funktionen.append((datei, bereich, m.group(1), "\n".join(zeilen[i + 1:j])))
            i = j + 1
    titel_re = re.compile(r"rolloutBeginn\(\s*(?:tr|begriff)\(Str::(\w+)")
    gruppen = []      # (funktion, titel, hinweis)
    for f in funktionen:
        m = titel_re.search(f[3])
        if not m:
            # der Titel als Variable: "const std::string kopf = titel(Str::X, ...); ... rolloutBeginn(kopf.c_str(), ...)"
            v = re.search(r"rolloutBeginn\(\s*(\w+)\.c_str\(\)", f[3])
            m = re.search(r"\b%s = [^;]*?Str::(\w+)" % v.group(1), f[3]) if v else None
        if m:
            h = re.search(r"rolloutBeginn\([^;]*?\.hinweis = tr\(Str::(\w+)\)", f[3], re.S)
            gruppen.append((f, m.group(1), h.group(1) if h else ""))
    gruppen_namen = {g[0][2] for g in gruppen}

    def ruft(rumpf, name):
        return re.search(r"(?<![\w:.>])" + re.escape(name) + r"\(", rumpf) is not None

    def helfer_von(g):
        """Die Hilfsfunktionen einer Gruppe (keine Gruppe selbst), bis in die dritte Ebene; zuerst die der eigenen Datei."""
        gefunden = []
        offen = [g[0]]
        for _ in range(3):
            neu = []
            for f in offen:
                for h in funktionen:
                    if h in gefunden or h is g[0] or h[2] in gruppen_namen or not ruft(f[3], h[2]):
                        continue
                    # gleichnamige Helfer (feldKnopf, hauptKnopf) gibt es je Datei: nur der der eigenen Datei, sonst ein
                    # von aussen sichtbarer (drawModell..., modell...)
                    if h[0] != f[0] and not (h[2].startswith("drawModell") or h[2].startswith("modell")):
                        continue
                    gefunden.append(h)
                    neu.append(h)
            offen = neu
        return gefunden

    element_re = [
        re.compile(r"werkzeugKnopf\(tr\(Str::(\w+)\), \{[^;]*?\.hinweis = tr\(Str::(\w+)\)"),
        re.compile(r"ui::schalter\(tr\(Str::(\w+)\), [^,]+, tr\(Str::(\w+)\)"),
        re.compile(r"ui::regler(?:Int)?\(tr\(Str::(\w+)\), [^;]*?, tr\(Str::(\w+)\)"),
    ]
    aktion_re = re.compile(r"\bfeldKnopf\(Str::(\w+)")   # Knopf einer Aktion der Tabelle: ihr Hinweis heisst <Kennung>Hint

    orte = []
    elemente = {}     # Kennung -> [bereich, hinweis, [titel ...]]
    for g in gruppen:
        (datei, bereich, name, rumpf), titel, hinweis = g
        rumpf_alle = rumpf + "".join("\n" + h[3] for h in helfer_von(g))
        inhalt = []
        for k in re.findall(r"Str::(\w+)", rumpf_alle):
            if k != titel and k in kennungen and not k.endswith("Hint") and k not in inhalt:
                inhalt.append(k)
        voraus = MODELL + " && zahl:modell.bereich%d < gezeichnet:modell.bereich%d" % (bereich, bereich)
        paare = []
        for r in element_re:
            paare += r.findall(rumpf_alle)
        paare += [(k, k + "Hint") for k in aktion_re.findall(rumpf_alle) if (k + "Hint") in kennungen]
        # Die Knoepfe, Schalter und Regler sind eigene Orte - als Inhalt der Gruppe zaehlt nur der uebrige Text (Tabellen, Tipps,
        # Meldungen). Sonst stuenden ihre Woerter doppelt im Index und verschoeben die Gewichte aller anderen Treffer
        # (ctest hilfe, H2-Liste: "像在游戏里一样点击菜单按钮" fiel aus den ersten drei).
        eigene = {k for k, _ in paare}
        inhalt = [k for k in inhalt if k not in eigene]
        orte.append(["modell", "knopf:%s@mtrollouts" % titel, "ViewModel>%s>%s" % (MODELL_BEREICHE[bereich], titel), hinweis, voraus, ",".join(inhalt)])
        for k, h in paare:
            if k == titel or k not in kennungen or h not in kennungen:
                continue
            e = elemente.setdefault(k, [bereich, h, []])
            if e[0] == bereich and titel not in e[2]:
                e[2].append(titel)
    for k, (bereich, h, titel) in elemente.items():
        voraus = (MODELL + " && zahl:modell.bereich%d < gezeichnet:modell.bereich%d" % (bereich, bereich) +
                  " && zahl:modell.feld.%s < %s" % (k.lower(), ";".join("knopf:%s@mtrollouts" % t for t in titel)))
        orte.append(["modell", "knopf:%s@mtrollouts" % k, "ViewModel>%s>%s>%s" % (MODELL_BEREICHE[bereich], titel[0], k), h, voraus, ""])
    return orte


def karteEinstellungen():
    """H2: die Seiten der Einstellungen des Karten-Editors (kSeiten) mit ihren Feldern als Inhalt."""
    zeilen = lies("gui/app_karte_dialoge.cpp")
    text = "\n".join(zeilen)
    tabelle = re.search(r"const Seite kSeiten\[\] = \{(.*?)\n\};", text, re.S).group(1)
    seiten = re.findall(r"\{Str::(\w+), Str::(\w+), (\d)\}", tabelle)
    konst = {k: int(v) for k, v in re.findall(r"constexpr int (kSeite\w+) = (\d+);", text)}
    funktion = {}
    for k, f in re.findall(r"g_entwurf\.seite == (kSeite\w+)\) \{\s*(\w+)\(\);", text):
        if k in konst:
            funktion[konst[k]] = f
    aus = []
    voraus = KARTE + " && zahl:karte.einstellungen < menue:KeLEdit>KeLPreferences"
    oberseite = ""
    for i, (baum, titel, tiefe) in enumerate(seiten):
        if tiefe == "0":
            oberseite = baum
        felder = [titel]
        f = funktion.get(i)
        if f:
            m = re.search(r"\nvoid " + f + r"\(\) \{(.*?)\n\}", text, re.S)
            if m:
                for n in re.findall(r"trK\(Str::(\w+)\)", m.group(1)):
                    if n not in felder:
                        felder.append(n)
        kette = "KeLEdit>KeLPreferences>" + (oberseite + ">" if tiefe == "1" and oberseite != baum else "") + baum
        aus.append(["karte", "gezeichnet:karte.einstseite.%d" % i, kette, "", voraus, ",".join(felder)])
    return aus


def bekannte_kennungen():
    """Die Kennungen aus tools/gen_i18n.py (samt i18n_*.py) - ohne den Generator auszufuehren."""
    namen = set()
    for datei in os.listdir(os.path.join(WURZEL, "tools")):
        if datei == "gen_i18n.py" or (datei.startswith("i18n_") and datei.endswith(".py")):
            with open(os.path.join(WURZEL, "tools", datei), encoding="utf-8") as f:
                namen.update(re.findall(r'^\s*\("(\w+)",', f.read(), re.M))
    return namen


# H2: die Leiste des Menue-Editors; Knoepfe, die erst mit offenem Menue da sind.
MIT_MENUE = {"MeSpeichern", "MeVideoImport", "MeSchriftBauen", "MeTexte", "MePruefen", "MeExportPk3",
             "MeImSpielTesten"}


def menueEditor():
    """Knoepfe, Umschalter und Haken; seit M2 die Leiste aus Symbolknoepfen (iconKnopf mit Hinweis
    wie in leisten()). Die Leiste zuerst: heisst ein Knopf eines Fensters gleich (Pruefen, Schrift
    bauen), gilt der Leistenknopf mit seinem Hinweis."""
    leiste, knoepfe = [], []
    # rc608 Teil 3a: Die neuen Leisten liegen in eigenen Dateien
    zeilen = []
    for datei in ("gui/app_menue.cpp", "gui/app_menue_leiste.inc", "gui/app_menue_links.inc", "gui/app_menue_bildwahl.inc"):
        zeilen += lies(datei)
    for z in zeilen:
        # M4: leistenKnopf (Symbol oder Symbol + Text); Hinweis auch als "tr(a ? Str::X : Str::Y)" (X gilt)
        m = re.search(r"(?:icon(?:Text)?|leisten)Knopf\(tr\(Str::(Me\w+)\), *UiIcon::\w+, *"
                      r"tr\((?:[^()?]*(?:\([^()]*\))?[^()?]*\? *)?Str::(\w+)", z)
        if m:
            n, hinweis, liste = m.group(1), m.group(2), leiste
        else:
            m = re.search(r"ImGui::(?:Button|RadioButton|Checkbox)\(tr\(Str::(Me\w+)\)", z)
            if not m:
                continue
            n = m.group(1)
            hinweis, liste = (n + "Hint" if (n + "Hint") in bekannte_kennungen() else ""), knoepfe
        voraus = MENUE + (" && zahl:menue.dokumente < knopf:MeAusSpiel" if n in MIT_MENUE else "")
        liste.append(["menue", "knopf:" + n, "ViewMenuEditor>" + n, hinweis, voraus, ""])
    return leiste + knoepfe


def text():
    anordnung = lies("gui/app_anordnung.inc")
    app = lies("gui/app.cpp")
    pf, pfInhalt = pfade(app)
    orte = (hauptmenue(anordnung) + modusleiste(anordnung) + einstellungen(app) + pf + leisten() + karteEinstellungen() + menueEditor() + fenstermenue() +
            modellFeld())
    for o in orte:
        if o[1] == "menue:MenuEdit>PathsTitle":
            o[5] = ",".join(pfInhalt)
    gesehen = set()
    zeilen = ["// Erzeugt von tools/gen_hilfe_orte.py - nicht von Hand aendern.",
              "// Ortskatalog des Helfers (Stufe H1/H2): bereich, ziel, kette, hinweis, voraus, inhalt."]
    for o in orte:
        if o[1] in gesehen:
            continue
        gesehen.add(o[1])
        if len(o) < 6:
            o = o + [""]
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
