"""Der Model-Tab im Handbuch: alle Quellen, ihre Reihenfolge und ihr Platz - an EINER Stelle (AP14, rc612).

Bis rc611 und in den Paketen des Umbaus nach dem Entwurf (AP1-AP12, ENT1, ENT2) brauchte jede Quelle vier Stellen: die
Seitenliste in inhalt.py, je eine Zeile in inhalt_es.py und inhalt_cs.py und die Oberflaechentexte in gen_handbuch.py. Jetzt
steht jede Quelle mit EINER Zeile in QUELLEN:

    Seite("inhalt_neu"),                         # eigene Seite: <quelle>.seite(); die Stelle in der Liste ist die Stelle
                                                 # im Handbuch (Seitenleiste und Weiter/Zurueck)
    Anhang("inhalt_neu", "modell-ansehen"),      # ergaenzt eine vorhandene Seite: <quelle>.einhaengen(<Seite>); die Stelle
                                                 # in der Liste ist die Reihenfolge des Einhaengens
    Hinter("inhalt_neu", "bloecke", "modell-blender", "inhalt_ap4", "seite"),
                                                 # Bloecke <quelle>.<funktion>() hinter den Absatz ("p", _t("seite")) der
                                                 # Quelle inhalt_ap4 auf der Seite modell-blender

Mehr braucht es nicht:
  - Spanisch und Tschechisch (ES / CS der Quelle) holen inhalt_es.py und inhalt_cs.py ueber weitere() von hier,
  - die Oberflaechentexte fuer {{Kennung}} laedt gen_handbuch.py aus allem, was tools/gen_i18n.py in seine Liste nimmt
    (eine Quelle tools/i18n_<kuerzel>.py, die dort eingetragen ist, ist damit auch im Handbuch bekannt),
  - die Bilder nimmt der Selbsttest auf (BHED_EDITORTEST=handbuch, BHED_HB_NUR=<kuerzel>; tools/handbuch/LIESMICH.md).

Reihenfolge der Seiten: erst der Rahmen des Tabs (Ansichten, Explorer, rechtes Feld; Bedienmodus; Aufbau wie Blender;
Zeitleiste; Dateien), dann die Bereiche in der Reihenfolge der Bereichsleiste des Programms (Ansehen/Pruefen, Modellieren,
UV, Weights, Painting, Animation), zuletzt das Animieren im Mission-Tab.
"""
import importlib


class Seite:
    """Eine Quelle mit eigener Seite (seite())."""

    def __init__(self, quelle):
        self.quelle = quelle


class Anhang:
    """Eine Quelle, die Bausteine in eine vorhandene Seite haengt (einhaengen(seite))."""

    def __init__(self, quelle, seite):
        self.quelle = quelle
        self.seite = seite


class Hinter:
    """Eine Quelle, deren Bloecke (<funktion>() liefert eine Liste) hinter einen Absatz einer anderen Quelle gehoeren: hinter den
    Block ("p", <von>._t(<schluessel>)) der Seite <seite>."""

    def __init__(self, quelle, funktion, seite, von, schluessel):
        self.quelle = quelle
        self.funktion = funktion
        self.seite = seite
        self.von = von
        self.schluessel = schluessel


class Teil:
    """Eine Quelle, deren Bausteine eine andere Quelle selbst einsetzt (bloecke()) - hier nur wegen ES / CS genannt."""

    def __init__(self, quelle):
        self.quelle = quelle


QUELLEN = [
    # --- der Rahmen des Tabs --------------------------------------------------------------------------------------------
    Seite("inhalt_mb1a1"),                          # modell-ansehen: Ueberblick - Ansichten, Explorer, rechtes Feld, Navigation
    Teil("inhalt_mbkopf"),                          #   Abschnitt Kopfzeile der Ansicht, ViewCube und Gizmo (in inhalt_mb1a1)
    Teil("inhalt_mbraster"),                        #   Abschnitt Boden-Raster (in inhalt_mb1a1)
    Teil("inhalt_mbnav"),                           #   Abschnitt Navigation in den Ansichten (in inhalt_mb1a1)
    Anhang("inhalt_ap2", "modell-ansehen"),         #   Abschnitt Szenen-Explorer / Outliner
    Anhang("inhalt_ap3", "modell-ansehen"),         #   Abschnitt rechtes Feld: Befehlsfeld / Eigenschaften-Editor
    Hinter("inhalt_ap14", "bloecke", "modell-ansehen", "inhalt_ap3", "feld"),
                                                    #   Reiter bei Mindestgroesse als Symbole (Entscheidung B6)
    Seite("inhalt_mb1a2"),                          # modell-bedienung: wie 3ds Max oder wie Blender, Tasten, Ins Skript
    Seite("inhalt_ap4"),                            # modell-blender: der Aufbau wie Blender (Werkzeugleiste, Seitenleiste)
    Hinter("inhalt_fix612c", "bloecke_aufbau", "modell-blender", "inhalt_ap4", "seite"),
                                                    #   Knochen waehlen wie 3ds Max als Umschalter (hinter Seitenleiste/Messen)
    Seite("inhalt_ap1"),                            # modell-zeitleiste
    Seite("inhalt_mb2"),                            # modell-dateien: Speichern, GLA, Karten-GLA
    # --- die Bereiche in der Reihenfolge der Bereichsleiste -------------------------------------------------------------
    Seite("inhalt_ap5"),                            # modell-pruefen: Bereich Ansehen/Pruefen
    Seite("inhalt_md1d"),                           # modell-modellieren
    Anhang("inhalt_ap6", "modell-modellieren"),     #   das rechte Feld des Bereichs Modellieren
    Anhang("inhalt_ap13s", "modell-modellieren"),   #   Abschnitt Modifier-Stapel (vor "grenzen")
    Seite("inhalt_uv1b"),                           # modell-uv
    Anhang("inhalt_ap7", "modell-uv"),              #   das rechte Feld des Bereichs UV
    Seite("inhalt_ap8"),                            # modell-weights (ersetzt die Seite von inhalt_mb1a2 bis rc611)
    Hinter("inhalt_fix612b", "bloecke", "modell-weights", "inhalt_ap8", "sperren"),
                                                    #   Speichern, Detailstufe und Strg+Z beim Malen
    Seite("inhalt_ap12"),                           # modell-autoskin
    Seite("inhalt_paint"),                          # modell-painting
    Anhang("inhalt_ap9", "modell-painting"),        #   das rechte Feld des Bereichs Painting
    Seite("inhalt_ap10"),                           # modell-animationsfeld: das rechte Feld des Bereichs Animation
    Hinter("inhalt_fix612c", "bloecke_animationsfeld", "modell-animationsfeld", "inhalt_ap10", "rig"),
                                                    #   Animation-Rig (nur FK) backt die IK in FK-Schluessel
    Seite("inhalt_ap13k"),                          # modell-knochen: Knochen-Darstellung, Rig-Stile, Durchsicht
    Seite("inhalt_ent3b"),                          # modell-ent3b: IK/FK-Spur je Glied wie CAT, Griff der Wurzel, Rig-Vorlage, Ducken, Tempo
    Seite("inhalt_mb3"),                            # modell-rig
    Seite("inhalt_mb3b"),                           # modell-kurven
    Seite("inhalt_mb4"),                            # modell-ik
    Seite("inhalt_ph"),                             # modell-physik
    Seite("inhalt_rt"),                             # modell-retarget
    Seite("inhalt_animimp"),                        # modell-animimport
    # --- ueber den Tab hinaus -------------------------------------------------------------------------------------------
    Seite("inhalt_mb7"),                            # mission-animieren: Figuren im Mission-Tab animieren
    # --- ENTSCHEID3A (Entscheidungen B7-B13, C4): Bausteine hinter Absaetzen anderer Quellen ------------------------------
    Hinter("inhalt_ent3a", "bloecke_explorer", "modell-ansehen", "inhalt_ap2", "ebenen"),       # B7 eingefrorene Zeile waehlbar
    Hinter("inhalt_ent3a", "bloecke_bedienung", "modell-bedienung", "inhalt_mb1a2", "b_text"),  # C4 Esc schliesst das Rechtsklick-Menue
    Hinter("inhalt_ent3a", "bloecke_leiste", "modell-blender", "inhalt_ap4", "gruppen"),        # B8 Leiste breiter ziehen
    Hinter("inhalt_ent3a", "bloecke_seite", "modell-blender", "inhalt_ap4", "seite"),           # B9 Seitenleiste bei Knochen
    Hinter("inhalt_ent3a", "bloecke_stapel", "modell-modellieren", "inhalt_ap13s", "bedienen"), # B11-B13 im Modifier-Stapel
    Anhang("inhalt_ap11", "modell-ansehen"),        # AP11: Abschnitt Mehrere Modelle in Reitern, Flaechen kopieren und einfuegen
]


def _modul(q):
    return importlib.import_module(q.quelle)


def seiten():
    """Die Seiten des Model-Tabs in Lesereihenfolge, mit allen Anhaengen."""
    aus = []
    for q in QUELLEN:
        if isinstance(q, Seite):
            aus.append(_modul(q).seite())
    nach_id = {s["id"]: s for s in aus}
    if len(nach_id) != len(aus):
        raise SystemExit("inhalt_modell: eine Seiten-Kennung des Model-Tabs steht doppelt")
    for q in QUELLEN:
        if isinstance(q, Anhang):
            if q.seite not in nach_id:
                raise SystemExit("inhalt_modell: %s will an die Seite %s, die es nicht gibt" % (q.quelle, q.seite))
            _modul(q).einhaengen(nach_id[q.seite])
    for q in QUELLEN:
        if isinstance(q, Hinter):
            if q.seite not in nach_id:
                raise SystemExit("inhalt_modell: %s will an die Seite %s, die es nicht gibt" % (q.quelle, q.seite))
            ziel = importlib.import_module(q.von)._t(q.schluessel).t["en"]
            b = nach_id[q.seite]["bloecke"]
            stellen = [n for n, x in enumerate(b) if x[0] == "p" and getattr(x[1], "t", {}).get("en") == ziel]
            if len(stellen) != 1:
                raise SystemExit("inhalt_modell: %s findet den Absatz %s.%s auf %s %d-mal" % (q.quelle, q.von, q.schluessel, q.seite, len(stellen)))
            b[stellen[0] + 1:stellen[0] + 1] = getattr(_modul(q), q.funktion)()
    return aus


def weitere(spalte):
    """Die Uebersetzungen der Quellen fuer eine weitere Sprache (spalte: "ES" oder "CS") als ein {englisch: Uebersetzung}."""
    aus = {}
    for q in QUELLEN:
        texte = getattr(_modul(q), spalte, None)
        if texte is None:
            raise SystemExit("inhalt_modell: %s hat kein %s (sechs Sprachen je Text)" % (q.quelle, spalte))
        aus.update(texte)
    return aus
