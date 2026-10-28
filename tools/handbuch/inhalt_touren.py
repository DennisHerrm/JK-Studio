"""Die Tutorials im Programm (rc601, Paket tour): worauf jeder Schritt zeigt
und wann er erledigt ist.

Der Betreuer wuenschte Tutorials, bei denen das Programm selbst zeigt, welche
Knoepfe man druecken muss, und Schritt fuer Schritt fuehrt ("Erst ohne KI, KI
spaeter"). Text, Bild und Bildunterschrift jedes Schritts kommen aus den
Handbuchquellen (inhalt_schritte.py) - hier steht nur, was das Programm dazu
braucht. gen_handbuch.py schreibt daraus handbuch/<sprache>/touren.json; ein
unbekannter Name bricht den Bau ab, wie eine unbekannte {{Kennung}}.

Schluessel: der Bildname des Schritts (steht in inhalt_schritte.py schon an
jedem Schritt), oder (Seite, Ansicht, Schrittnummer) fuer Schritte ohne Bild
oder mit einem Bild, das mehrere Schritte teilen.

Felder (Grammatik in include/bhed/tour.h):
  ziel        worauf gezeigt wird, ";" fuer mehrere (nummerierte Rahmen)
  fertig      wann der Schritt erledigt ist, "|" = oder; "manuell" = Knopf Weiter
  voraus      "<pruefung> < <ziel>": gilt sie nicht, zeigt die Tour auf den Oeffner
  einrichten  was "Mach es" ausfuehrt, ";" fuer mehrere - es muss die
              Pruefung `fertig` erfuellen (tests/tourtest.cpp prueft das)
  sicher      "Mach es" erlaubt - nie bei Dateidialogen (Speichern,
              Uebersetzen eines unbenannten Skripts): ein Windows-Dialog haelt
              die Nachrichtenschleife an (gui/selbsttest.cpp, Kopf); nie bei
              Schritten, die man nur ansieht (schwebt)
  ueber       was "Ueberspringen" ausfuehrt (rc601, Code-Pruefung) - nur, was
              der naechste Schritt braucht. Leer: `einrichten` ohne befehl und
              wert (include/bhed/tour.h, ueberspringAktionen) - Ueberspringen
              aendert das Skript nie (vorher setzte es bei k-befehl-einfuegen
              ein streunendes `wait` ein)

Erste Fassung: die fuenf Skript-Tutorials ohne Spieldaten, Ansicht Classic.
"""

from inhalt_basis import T  # noqa: E402  (H1: Hinweise in vier Sprachen)


def A(ziel="", fertig="manuell", voraus="", einrichten="", sicher=False, ueber="", falsch=None):
    """falsch (H1): [(pruefung, T)] - freundliche Hinweise, wenn statt des
    Schritts etwas anderes passiert ("Das war ein Licht - nimm ...")."""
    return {"ziel": ziel, "fertig": fertig, "voraus": voraus, "einrichten": einrichten, "sicher": sicher,
            "ueberspringen": ueber, "falsch": list(falsch or [])}


# Die Touren dieser Fassung (Handbuchseiten) und ihre Ansichten.
TOUREN = ["k-neues-skript", "k-befehl-einfuegen", "k-werte-bearbeiten", "k-zeilen-ordnen", "k-uebersetzen",
          # H1: die erste Karte im Karten-Editor (inhalt_karte8c.py)
          "tut-karte"]
ANSICHTEN = ["classic"]

# Die Ereignisliste gibt es nur im Modus Events - sonst zeigt die Tour auf
# dessen Knopf in der Modusleiste.
EVENTS = "modus:0 < knopf:ViewEvents@modebar"
# Der Ereigniseditor muss offen sein - sonst zeigt die Tour auf den Baum.
EDITOR = "editor < baum"
AUSWAHL = "auswahl < baum"

NACH_BILD = {
    # --------------------------------------------------------- k-neues-skript
    # "+" oder File -> New: beides gibt ein leeres Skript (New nimmt dafuer
    # auch einen leeren Reiter, den es schon gibt - deshalb "leer"). Beide
    # Wege werden gezeigt (1, 2); die Menuekette zeigt erst "File", aufgeklappt
    # "New".
    "s_neu_plus": A("reiter:+;menue:MenuFile>FileNew", "reiter+|leer", einrichten="neuerReiter", sicher=True),
    # "Hover over it to read what it does" - oder gleich doppelklicken. Kein
    # "Mach es": Schweben kann das Programm nicht fuer dich (Code-Pruefung:
    # "Do it for me ... cannot complete the step"); Ueberspringen stellt Events.
    "s_neu_affect_liste": A("eintrag:affect", "schwebt|befehl:affect", EVENTS, "modus:0"),
    "s_neu_affect_baum": A("eintrag:affect", "befehl:affect", EVENTS, "modus:0;befehl:affect", True),
    # Erledigt, wenn affect einen Namen hat (DEFAULT ist Ravens Platzhalter) -
    # also nach Ok im Editor.
    # Ueberspringen schreibt keinen Wert, es schliesst nur den Editor (der
    # naechste Schritt braucht den Baum).
    "s_neu_editor": A("knopfN:##v@###editor#0;knopf:EditorOk@###editor", "wert:affect#0:0",
                      "editor:affect < zeile:affect#0", "wert:affect#0:0=kyle", True, ueber="editorzu"),
    "s_neu_wait_liste": A("zeile:affect#0;eintrag:wait", "kind:affect>wait", EVENTS,
                          "modus:0;waehle:affect#0;befehl:wait", True),
    "s_neu_ergebnis": A("zeile:wait#0", "manuell"),
    # Speichern fragt beim ersten Mal nach einem Namen (Dateidialog): nie "Mach es".
    "s_neu_speichern": A("knopf:FileSave", "gespeichert"),

    # --------------------------------------------------------- k-befehl-einfuegen
    # Jeder Weg (Doppelklick, Einfg, Ziehen, Rechtsklick) erfuellt dieselbe
    # Pruefung: ein Befehl mehr. So zaehlt jeder Weg, den das Handbuch nennt.
    # "Mach es" fuegt ein wait ein; Ueberspringen stellt nur Events (kein
    # streunendes wait im Skript).
    "s_ein_doppel": A("baum;eintrag:wait", "knoten+", EVENTS, "modus:0;befehl:wait", True),
    "s_ein_einfg": A("eintrag:wait;baum", "knoten+", EVENTS, "modus:0;befehl:wait", True),
    "s_ein_ziehen": A("eintrag:wait;baum", "knoten+", EVENTS, "modus:0;befehl:wait", True),
    "s_ein_kontext": A("baum", "knoten+", "", "befehl:wait", True),

    # --------------------------------------------------------- k-werte-bearbeiten
    "h_editor": A("baum", "editor", "", "editor", True),
    "s_wert_felder": A("knopfN:##v@###editor#0;knopfN:##v@###editor#1", "manuell", EDITOR),
    "s_wert_liste": A("knopfN:##combo@###editor#0;knopfN:##combo@###editor#1", "manuell", EDITOR),
    "s_wert_datei": A("knopf:EditorBrowse@###editor;knopf:EditorPlay@###editor", "manuell", EDITOR),
    "s_wert_helper": A("knopf:EditorHelper@###editor", "manuell", EDITOR),
    "s_wert_expr": A("knopf:EditorExpr@###editor", "manuell", EDITOR),
    "s_wert_ok": A("knopf:EditorOk@###editor;knopf:EditorCancel@###editor", "editorzu", "", "editorzu", True),

    # --------------------------------------------------------- k-zeilen-ordnen
    "s_auswahl": A("baum", "auswahl", "", "waehle:*", True),
    "s_verschieben": A("gruppe:ActMoveUp,ActMoveDown", "aenderung+", AUSWAHL),
    "s_clone": A("knopf:ActClone", "knoten+", AUSWAHL),
    "s_copy": A("knopf:ActCopy;knopf:ActPaste", "knoten+", AUSWAHL),
    "s_rem": A("knopf:ActRem", "rem+", AUSWAHL),

    # --------------------------------------------------------- k-uebersetzen
    # Uebersetzen sichert zuerst - ein unbenanntes Skript fragt nach dem
    # Namen (Dateidialog): nie "Mach es".
    "t1_6": A("knopf:Compile", "uebersetzt+"),
    "s_comp_fehler": A("knopf:Compile", "manuell"),
    "s_comp_meldung": A("meldungen", "fenster:messages", "", "fenster:messages", True),
    "s_comp_editor": A("knopf:Compile", "uebersetzt+"),
}

# (Seite, Ansicht, Nummer ab 1) -> Angaben; gehen den Angaben je Bild vor.
NACH_SCHRITT = {}


# --- H1: tut-karte - die erste eigene Karte im Karten-Editor ------------------
# Gepruefte Messwerte (gui/tour.cpp, karteMesswerte): karte.spiel (Spiel
# geladen), karte.leer, karte.pinsel, karte.hoehe (Oberkante aller Pinsel),
# karte.texturen (Pruefsumme der Flaechentexturen), karte.keineauswahl,
# karte.ent.<klasse>, karte.gespeichert, karte.caulk, karte.gebaut,
# karte.spielen, karte.pk3.
KARTE = "modus:3 < knopf:ViewMapEditor@modebar"
_NICHTS_GEWAEHLT = ("zahl:karte.keineauswahl", T(
    "Nothing is selected yet. Click the brush in a 2D view first, then the texture.",
    "Noch ist nichts gewählt. Klick zuerst den Brush in einer 2D-Ansicht an, dann die Textur.",
    "还没有选中任何东西。先在 2D 视图中点击画刷,再点击纹理。",
    "まだ何も選択されていません。先に 2D ビューでブラシをクリックしてから、テクスチャをクリックしてください。"))
NACH_BILD.update({
    "h_tut_karte_pfade": A("menue:KeLEdit>KeLPreferences", "zahl:karte.spiel", voraus=KARTE),
    "h_tut_karte_neu": A("menue:KeLFile>KeLNewMap", "zahl:karte.leer", voraus=KARTE),
    "h_tut_karte_brush": A("fenster:##feldXY", "zahl:karte.pinsel+", voraus=KARTE, falsch=[
        ("zahl:karte.ent+", T(
            "That made an entity. Press Ctrl+Z, then drag with the LEFT mouse button in the 2D view.",
            "Das hat eine Entity gesetzt. Drück Strg+Z und zieh dann mit der LINKEN Maustaste in der 2D-Ansicht.",
            "这创建了一个实体。按 Ctrl+Z,然后在 2D 视图中用鼠标左键拖动。",
            "エンティティができました。Ctrl+Z を押してから、2D ビューでマウスの左ボタンでドラッグしてください。"))]),
    "h_tut_karte_hoehe": A("fenster:##feldXY", "zahl:karte.hoehe+", voraus=KARTE, falsch=[
        ("zahl:karte.pinsel+", T(
            "That drew a second brush. Press Ctrl+Z and start the drag just above the top edge of the first one.",
            "Das hat einen zweiten Brush gezogen. Drück Strg+Z und beginne knapp über der Oberkante des ersten.",
            "这画出了第二个画刷。按 Ctrl+Z,然后从第一个画刷上边缘的正上方开始拖动。",
            "2 つ目のブラシができました。Ctrl+Z を押して、1 つ目の上端のすぐ上からドラッグを始めてください。"))]),
    "h_tut_karte_textur": A("fenster:##feldTexturen", "zahl:karte.texturen~", voraus=KARTE, falsch=[_NICHTS_GEWAEHLT]),
    "h_tut_karte_csg": A("knopfN:##knopfCSGTool", "zahl:karte.pinsel+", voraus=KARTE),
    "h_tut_karte_hohl": A("fenster:##feldTexturen", "zahl:karte.texturen~", voraus=KARTE, falsch=[_NICHTS_GEWAEHLT]),
    "h_tut_karte_licht": A("fenster:##feldXY", "zahl:karte.ent.light+", voraus=KARTE, falsch=[
        ("zahl:karte.pinsel+", T(
            "That drew a brush. Press Ctrl+Z and click with the RIGHT mouse button.",
            "Das hat einen Brush gezogen. Drück Strg+Z und klick mit der RECHTEN Maustaste.",
            "这画出了一个画刷。按 Ctrl+Z,然后用鼠标右键点击。",
            "ブラシができました。Ctrl+Z を押して、マウスの右ボタンでクリックしてください。"))]),
    "h_tut_karte_spawn": A("fenster:##feldXY", "zahl:karte.ent.info_player_start", voraus=KARTE, falsch=[
        ("zahl:karte.ent.light+", T(
            "That was another light. Right-click again and choose info > info_player_start.",
            "Das war noch ein Licht. Klick noch einmal rechts und wähle info > info_player_start.",
            "这又是一盏灯。再右键点击一次,选择 info > info_player_start。",
            "それはもう 1 つのライトです。もう一度右クリックして info > info_player_start を選んでください。"))]),
    "h_tut_karte_gespeichert": A("menue:KeLFile>KeLSaveAs", "zahl:karte.gespeichert+", voraus=KARTE),
    "h_tut_karte_caulk": A("fenster:##feldXY", "zahl:karte.caulk+", voraus=KARTE),
    "h_tut_karte_optimierer": A("menue:KeLBuild>KeOptMenue", "offen:karteoptimierer", voraus=KARTE),
    "h_tut_karte_fortschritt": A("menue:KeLBuild", "zahl:karte.gebaut+", voraus=KARTE),
    "h_tut_karte_spielen": A("menue:KeLView>KeSpMenue", "zahl:karte.spielen", voraus=KARTE),
    "h_tut_karte_pk3menue": A("menue:KeLBuild>KePkMenue", "zahl:karte.pk3+", voraus=KARTE),
})


def angaben(seite, ansicht, nr, bild):
    """Die Angaben eines Schritts oder None."""
    return NACH_SCHRITT.get((seite, ansicht, nr)) or (NACH_BILD.get(bild) if bild else None)


# --- Was das Programm kennt (include/bhed/tour.h) - der Bau prueft dagegen ---
ZIELARTEN = {"knopf", "knopfN", "menue", "gruppe", "eintrag", "zeile", "reiter", "fenster", "baum", "meldungen"}
PRUEFUNGEN = {"reiter", "knoten", "aenderung", "rem", "uebersetzt", "gespeichert", "befehl", "leer", "kind", "wert",
              "auswahl", "editor", "editorzu", "fenster", "modus", "ansicht", "schwebt", "geklickt", "manuell",
              "zahl", "offen"}
AKTIONEN = {"neuerReiter", "befehl", "waehle", "wert", "modus", "ansicht", "editor", "editorzu", "fenster"}


def pruefe(seite, ansicht, nr, a, kennung_bekannt):
    """Bricht den Bau bei einem unbekannten Namen ab (wie eine unbekannte
    {{Kennung}}, LIESMICH.md). Die volle Grammatik prueft tests/tourtest.cpp
    an der erzeugten Datei."""
    wo = "%s/%s Schritt %d" % (seite, ansicht, nr)

    def ziele(text, was):
        for z in [t.strip() for t in text.split(";") if t.strip()]:
            art, _, arg = z.partition(":")
            if art not in ZIELARTEN:
                raise SystemExit("%s: unbekannte Zielart in %s: %s" % (wo, was, z))
            if art in ("knopf", "menue", "gruppe"):
                namen = arg.split("@")[0]
                for k in namen.replace(">", ",").split(","):
                    if not kennung_bekannt(k.strip()):
                        raise SystemExit("%s: unbekannte Kennung {{%s}} in %s" % (wo, k.strip(), was))

    def pruefungen(text, was):
        for p in [t.strip() for t in text.split("|") if t.strip()]:
            name = p.rstrip("+~").split(":")[0].strip()
            if name not in PRUEFUNGEN:
                raise SystemExit("%s: unbekannte Pruefung in %s: %s" % (wo, was, p))

    ziele(a["ziel"], "ziel")
    pruefungen(a["fertig"] or "manuell", "fertig")
    if a["voraus"]:
        p, _, o = a["voraus"].partition("<")
        pruefungen(p, "voraus")
        ziele(o, "voraus")
    for p, _t in a.get("falsch", []):
        pruefungen(p, "falsch")
    for akt in [t.strip() for t in (a["einrichten"] + ";" + a.get("ueberspringen", "")).split(";") if t.strip()]:
        if akt.split(":")[0] not in AKTIONEN:
            raise SystemExit("%s: unbekannte Aktion: %s" % (wo, akt))
