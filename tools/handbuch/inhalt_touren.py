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
          "tut-karte",
          # H1: eine Kameraeinstellung fuer die Zwischensequenz (inhalt_kurz.py)
          "k-kamera-ansicht",
          # H1: ein Figurenmodell ansehen (inhalt_kurz.py)
          "k-modell",
          # H2: mehr interaktive Touren mit echten Aktionen
          "k-gangart", "k-wegpunkte", "k-neu-timen", "tut-p1", "k-skins",
          "k-spielordner", "k-update", "tut-karte-tuer", "tut-menue"]
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


# --- H1: k-kamera-ansicht - eine Kameraeinstellung, wo du hinschaust ----------
VORSCHAU = "zahl:vorschau.bereit < knopf:ViewMap@modebar"
_OHNE_KARTE = ("zahl:vorschau.ohnekarte", T(
    "No map is open yet. Load one first: {{MenuFile}} > {{MapLoad}} - or open a mission.",
    "Noch ist keine Karte offen. Lade zuerst eine: {{MenuFile}} > {{MapLoad}} - oder öffne eine Mission.",
    "还没有打开地图。请先加载一张:{{MenuFile}} > {{MapLoad}},或打开一个任务。",
    "まだマップが開いていません。先に読み込んでください:{{MenuFile}} > {{MapLoad}}、またはミッションを開きます。"))
_OHNE_KAMERA = ("zahl:vorschau.ohnekamera", T(
    "This script has no camera yet. Insert a camera command first ({{GroupEvents}} list: double-click camera) - or open a mission with a cutscene.",
    "Dieses Skript hat noch keine Kamera. Füge zuerst einen camera-Befehl ein (Liste {{GroupEvents}}: camera doppelklicken) - oder öffne eine Mission mit Zwischensequenz.",
    "这个脚本还没有镜头。请先插入一个 camera 命令({{GroupEvents}} 列表中双击 camera),或打开一个带过场动画的任务。",
    "このスクリプトにはまだカメラがありません。先に camera コマンドを挿入するか({{GroupEvents}} 一覧で camera をダブルクリック)、カットシーンのあるミッションを開いてください。"))
_KAMERA_WAEHLEN = ("zahl:vorschau.nichtgewaehlt", T(
    "Select the camera first: click a camera shot on the map or the camera row in the timeline. Its buttons appear on the left.",
    "Wähl zuerst die Kamera: Klick auf eine Kameraeinstellung in der Karte oder auf die Kamerazeile der Zeitleiste. Links erscheinen dann ihre Knöpfe.",
    "请先选中镜头:点击地图上的某个镜头,或时间轴中的镜头行。左侧会出现它的按钮。",
    "まずカメラを選んでください:マップ上のカメラ ショットか、タイムラインのカメラ行をクリックします。左側にボタンが表示されます。"))
NACH_SCHRITT.update({
    ("k-kamera-ansicht", "classic", 1): A("karte3d", "zahl:vorschau.blick~", voraus=VORSCHAU, falsch=[_OHNE_KARTE]),
    ("k-kamera-ansicht", "classic", 2): A("knopf:CamNewShot", "zahl:vorschau.einstellungen+", voraus=VORSCHAU, falsch=[_OHNE_KARTE, _OHNE_KAMERA, _KAMERA_WAEHLEN]),
    ("k-kamera-ansicht", "classic", 3): A("knopf:CamSetToView", "aenderung+", voraus=VORSCHAU, falsch=[_KAMERA_WAEHLEN]),
    ("k-kamera-ansicht", "classic", 4): A("knopf:MapThroughCam", "zahl:vorschau.durchkamera", voraus=VORSCHAU),
})

# --- H1: k-modell - ein Figurenmodell ansehen -----------------------------------
MODELL = "modus:2 < knopf:ViewModel@modebar"
NACH_SCHRITT.update({
    ("k-modell", "classic", 1): A("knopf:ViewModel@modebar", "modus:2", einrichten="modus:2", sicher=True),
    ("k-modell", "classic", 2): A("knopf:NpcBrowser", "zahl:modell.figur~", voraus=MODELL),
    ("k-modell", "classic", 3): A("knopf:ModelTurntable;knopf:ModelResetView", "zahl:modell.blick~", voraus=MODELL),
    ("k-modell", "classic", 4): A("knopf:ModelSkin;knopf:ModelBones;knopf:ModelTextures", "zahl:modell.details~", voraus=MODELL),
})

# --- H2: mehr interaktive Touren mit echten Aktionen ---------------------------
# Jeder Schritt ist erst erledigt, wenn das Programm die Wirkung sieht
# (gui/tour_messwerte.cpp). Verkettete Vorbedingungen ("&&"): erst Modus,
# dann Fenster, dann das Feld.
HAUPTMENUE = "zahl:programm.hauptmenue < knopf:ViewEvents@modebar"
_OHNE_FIGUR = ("zahl:vorschau.ohnefigur", T(
    "This map has no figure yet. Open a mission with figures ({{MenuFile}} > {{LoadMission}}) or place one: the tile {{EntKindNpc}} on the left edge of the view.",
    "Diese Karte hat noch keine Figur. Öffne eine Mission mit Figuren ({{MenuFile}} > {{LoadMission}}) oder setz eine: die Kachel {{EntKindNpc}} am linken Rand der Ansicht.",
    "这张地图还没有角色。打开一个带角色的任务({{MenuFile}} > {{LoadMission}}),或放置一个:视图左边缘的 {{EntKindNpc}} 图块。",
    "このマップにはまだキャラクターがいません。キャラクターのいるミッションを開くか({{MenuFile}} > {{LoadMission}})、ビュー左端の {{EntKindNpc}} タイルで置いてください。"))
_OHNE_LAUFZIEL = ("zahl:vorschau.ohnelaufziel", T(
    "This figure does not walk yet - its path has no walk target. Pick a figure that walks, or let one walk first ([[tut-p3]]).",
    "Diese Figur läuft noch nicht - ihr Weg hat kein Laufziel. Wähl eine Figur, die läuft, oder lass zuerst eine laufen ([[tut-p3]]).",
    "这个角色还没有行走——它的路线上没有行走目标。选择一个会走的角色,或先让一个角色走起来([[tut-p3]])。",
    "このキャラクターはまだ歩きません。経路に歩く目標がありません。歩くキャラクターを選ぶか、先に歩かせてください([[tut-p3]])。"))
_WEG = "zahl:vorschau.wegfenster < gezeichnet:vorschau.wegknopf"
NACH_SCHRITT.update({
    # k-gangart: Figur waehlen -> Rechtsklick auf ein Laufziel -> Gangart -> abspielen
    ("k-gangart", "classic", 1): A("karte3d", "zahl:vorschau.figurgewaehlt", voraus=VORSCHAU, falsch=[_OHNE_KARTE, _OHNE_FIGUR]),
    ("k-gangart", "classic", 2): A("gezeichnet:vorschau.laufziel", "zahl:vorschau.gangartmenue",
                                   voraus=VORSCHAU + " && zahl:vorschau.figurgewaehlt < karte3d", falsch=[_OHNE_LAUFZIEL]),
    ("k-gangart", "classic", 3): A("gezeichnet:vorschau.gangartmenue", "aenderung+",
                                   voraus=VORSCHAU + " && zahl:vorschau.gangartmenue < gezeichnet:vorschau.laufziel"),
    ("k-gangart", "classic", 4): A("knopf:TlPlay", "zahl:vorschau.laeuft", voraus=VORSCHAU),
    # k-wegpunkte: Fenster -> erzeugen -> pruefen -> von Hand setzen
    ("k-wegpunkte", "classic", 1): A("gezeichnet:vorschau.wegknopf", "zahl:vorschau.wegfenster", voraus=VORSCHAU, falsch=[_OHNE_KARTE]),
    ("k-wegpunkte", "classic", 2): A("knopf:WegErzeugen@###wegpunkte", "zahl:vorschau.wegpunkte+", voraus=VORSCHAU + " && " + _WEG,
                                     falsch=[_OHNE_KARTE]),
    ("k-wegpunkte", "classic", 3): A("knopf:WegPruefen@###wegpunkte", "zahl:vorschau.weggeprueft", voraus=VORSCHAU + " && " + _WEG),
    ("k-wegpunkte", "classic", 4): A("knopf:WegSetzen@###wegpunkte;karte3d", "zahl:vorschau.wegpunkte+", voraus=VORSCHAU + " && " + _WEG),
    # k-neu-timen: abspielen, Block ziehen, Dauer, Bildschritt, Zeile
    ("k-neu-timen", "classic", 1): A("knopf:TlPlay", "zahl:vorschau.laeuft", voraus=VORSCHAU, falsch=[_OHNE_KARTE]),
    ("k-neu-timen", "classic", 2): A("gezeichnet:zeitleiste", "aenderung+", voraus=VORSCHAU),
    ("k-neu-timen", "classic", 3): A("gezeichnet:zeitleiste", "aenderung+", voraus=VORSCHAU),
    ("k-neu-timen", "classic", 4): A("gezeichnet:zeitleiste", "zahl:vorschau.tlschritte+", voraus=VORSCHAU),
    ("k-neu-timen", "classic", 5): A("gezeichnet:zeitleiste;baum", "auswahl", voraus=VORSCHAU),
    # tut-p1: Startseite -> neue Mission -> ansehen -> das alte Intro loeschen
    ("tut-p1", "classic", 1): A("knopf:ViewMap@modebar", "zahl:vorschau.startseite", falsch=[("zahl:vorschau.karte", T(
        "A map is open, so the tab shows the map instead of the start page. Close it with {{MenuFile}} > {{MapClose}}.",
        "Eine Karte ist offen, darum zeigt der Reiter die Karte statt der Startseite. Schließ sie mit {{MenuFile}} > {{MapClose}}.",
        "已经打开了一张地图,所以标签页显示地图而不是开始页。用 {{MenuFile}} > {{MapClose}} 关闭它。",
        "マップが開いているので、タブにはスタートページではなくマップが表示されています。{{MenuFile}} > {{MapClose}} で閉じてください。"))]),
    ("tut-p1", "classic", 2): A("knopf:StartCardNew;knopf:MissionCreate", "zahl:mission.name~",
                                voraus="zahl:vorschau.startseite|offen:##neuemission < knopf:ViewMap@modebar"),
    ("tut-p1", "classic", 3): A("karte3d", "manuell", voraus=VORSCHAU),
    ("tut-p1", "classic", 4): A("karte3d", "zahl:vorschau.ents-", voraus=VORSCHAU),
    # k-skins: Haut, Flaechenliste, Kappen, Skelett
    ("k-skins", "classic", 1): A("knopf:ModelSkin;knopf:OpenSkin", "zahl:modell.skin+", voraus=MODELL + " && zahl:modell.da < knopf:NpcBrowser"),
    ("k-skins", "classic", 2): A("gezeichnet:modell.flaechen", "zahl:modell.flaechenliste+", voraus=MODELL + " && zahl:modell.da < knopf:NpcBrowser"),
    ("k-skins", "classic", 3): A("knopf:ModelCaps", "zahl:modell.caps~", voraus=MODELL + " && zahl:modell.da < knopf:NpcBrowser"),
    ("k-skins", "classic", 4): A("knopf:ModelLoadGla", "zahl:modell.skelett~", voraus=MODELL + " && zahl:modell.da < knopf:NpcBrowser"),
    # k-spielordner: Pfade -> Spielordner dazu -> Liste -> Einstellungen -> Regelwerk
    ("k-spielordner", "classic", 1): A("menue:MenuEdit>PathsTitle", "zahl:programm.pfade", voraus=HAUPTMENUE),
    ("k-spielordner", "classic", 2): A("knopf:Pk3AddPath@###paths", "zahl:programm.spielordner+",
                                       voraus=HAUPTMENUE + " && zahl:programm.pfade < menue:MenuEdit>PathsTitle"),
    ("k-spielordner", "classic", 3): A("fenster:###paths", "manuell", voraus=HAUPTMENUE + " && zahl:programm.pfade < menue:MenuEdit>PathsTitle"),
    ("k-spielordner", "classic", 4): A("menue:MenuEdit>AppPrefs", "zahl:programm.einstellungen", voraus=HAUPTMENUE),
    ("k-spielordner", "classic", 5): A("knopf:PrefsRegelwerk", "zahl:programm.regelwerk~",
                                       voraus=HAUPTMENUE + " && zahl:programm.einstellungen < menue:MenuEdit>AppPrefs"),
    # k-update: suchen -> Antwort -> beim Start -> Aenderungen
    ("k-update", "classic", 1): A("menue:MenuHelp>UpdMenu", "zahl:update.fenster", voraus=HAUPTMENUE),
    ("k-update", "classic", 2): A("fenster:###update", "zahl:update.ergebnis", voraus=HAUPTMENUE + " && zahl:update.fenster < menue:MenuHelp>UpdMenu"),
    ("k-update", "classic", 3): A("menue:MenuHelp>UpdAutoCheck", "zahl:update.auto~", voraus=HAUPTMENUE),
    ("k-update", "classic", 4): A("knopf:ChangelogMenu@###update;menue:MenuHelp>ChangelogMenu", "offen:###changelog", voraus=HAUPTMENUE),
    # tut-karte-tuer (Karten-Editor): Brush -> func_door -> angle -> Trigger -> verbinden -> ausprobieren
    ("tut-karte-tuer", "classic", 1): A("knopf:ViewMapEditor@modebar", "modus:3", einrichten="modus:3", sicher=True),
    ("tut-karte-tuer", "classic", 2): A("fenster:##feldXY", "zahl:karte.pinsel+", voraus=KARTE, falsch=[
        ("zahl:karte.ent+", T(
            "That made an entity. Press Ctrl+Z, then drag with the LEFT mouse button in the 2D view.",
            "Das hat eine Entity gesetzt. Drück Strg+Z und zieh dann mit der LINKEN Maustaste in der 2D-Ansicht.",
            "这创建了一个实体。按 Ctrl+Z,然后在 2D 视图中用鼠标左键拖动。",
            "エンティティができました。Ctrl+Z を押してから、2D ビューでマウスの左ボタンでドラッグしてください。"))]),
    ("tut-karte-tuer", "classic", 3): A("fenster:##feldXY", "zahl:karte.ent.func_door+", voraus=KARTE, falsch=[
        _NICHTS_GEWAEHLT[0:1] + (T(
            "Nothing is selected. Click the door brush in a 2D view first, then right-click and choose func > func_door.",
            "Nichts ist gewählt. Klick zuerst den Tür-Brush in einer 2D-Ansicht an, dann rechts klicken und func > func_door wählen.",
            "没有选中任何东西。先在 2D 视图中点击门的画刷,然后右键点击并选择 func > func_door。",
            "何も選択されていません。先に 2D ビューでドアのブラシをクリックし、右クリックして func > func_door を選んでください。"),),
        ("zahl:karte.ent.light+", T(
            "That was a light. Press Ctrl+Z, select the door brush and choose func > func_door.",
            "Das war ein Licht. Drück Strg+Z, wähl den Tür-Brush und nimm func > func_door.",
            "这是一盏灯。按 Ctrl+Z,选中门的画刷,然后选择 func > func_door。",
            "それはライトです。Ctrl+Z を押し、ドアのブラシを選んで func > func_door を選んでください。"))]),
    ("tut-karte-tuer", "classic", 4): A("fenster:##feldGruppe", "zahl:karte.schluessel~", voraus=KARTE),
    ("tut-karte-tuer", "classic", 5): A("fenster:##feldXY", "zahl:karte.ent.trigger_multiple+", voraus=KARTE),
    ("tut-karte-tuer", "classic", 6): A("menue:KeLEntity>KeLConnectEntities", "zahl:karte.verbindungen+", voraus=KARTE),
    ("tut-karte-tuer", "classic", 7): A("menue:KeLBuild", "zahl:karte.gebaut+|zahl:karte.spielen", voraus=KARTE),
    # tut-menue (Menue-Editor): Modus -> aus dem Spiel -> verschieben -> bedienen -> exportieren
    ("tut-menue", "classic", 1): A("knopf:ViewMenuEditor@modebar", "modus:4", einrichten="modus:4", sicher=True),
    ("tut-menue", "classic", 2): A("knopf:MeAusSpiel", "zahl:menue.dokumente+", voraus="modus:4 < knopf:ViewMenuEditor@modebar"),
    ("tut-menue", "classic", 3): A("fenster:##mitte", "zahl:menue.text~", voraus="modus:4 < knopf:ViewMenuEditor@modebar", falsch=[
        ("zahl:menue.bedienen", T(
            "You are in {{MeBedienen}} mode - clicks press buttons there. Switch to {{MeBearbeiten}} to move elements.",
            "Du bist im Modus {{MeBedienen}} - dort drücken Klicks die Knöpfe. Schalt auf {{MeBearbeiten}}, um Elemente zu verschieben.",
            "你处于 {{MeBedienen}} 模式,点击会按下按钮。切换到 {{MeBearbeiten}} 才能移动元素。",
            "{{MeBedienen}} モードです。クリックするとボタンが押されます。要素を動かすには {{MeBearbeiten}} に切り替えてください。"))]),
    ("tut-menue", "classic", 4): A("knopf:MeBedienen", "zahl:menue.bedienen", voraus="modus:4 < knopf:ViewMenuEditor@modebar"),
    ("tut-menue", "classic", 5): A("knopf:MeExportPk3", "zahl:menue.exporte+", voraus="modus:4 < knopf:ViewMenuEditor@modebar"),
})
# k-kamera-ansicht (H1): ist die Kamera nicht gewaehlt, zeigt Schritt 2 jetzt
# auf die Kamerazeile der Zeitleiste (selbst gezeichnet, H2).
NACH_SCHRITT[("k-kamera-ansicht", "classic", 2)]["voraus"] = VORSCHAU + " && zahl:vorschau.ohnekamera|zahl:vorschau.kameragewaehlt < gezeichnet:zeitleiste.kamera"

def angaben(seite, ansicht, nr, bild):
    """Die Angaben eines Schritts oder None."""
    return NACH_SCHRITT.get((seite, ansicht, nr)) or (NACH_BILD.get(bild) if bild else None)


# --- Was das Programm kennt (include/bhed/tour.h) - der Bau prueft dagegen ---
ZIELARTEN = {"knopf", "knopfN", "menue", "gruppe", "eintrag", "zeile", "reiter", "fenster", "baum", "meldungen", "karte3d",
             "gezeichnet"}  # H2: selbst gezeichnete Ziele (gui/tour.h, tourZielMelden)
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
            name = p.rstrip("+~-").split(":")[0].strip()
            if name not in PRUEFUNGEN:
                raise SystemExit("%s: unbekannte Pruefung in %s: %s" % (wo, was, p))

    ziele(a["ziel"], "ziel")
    pruefungen(a["fertig"] or "manuell", "fertig")
    for stufe in [t for t in a["voraus"].split("&&") if t.strip()]:   # H2: verkettet
        p, _, o = stufe.partition("<")
        pruefungen(p, "voraus")
        ziele(o, "voraus")
    for p, _t in a.get("falsch", []):
        pruefungen(p, "falsch")
    for akt in [t.strip() for t in (a["einrichten"] + ";" + a.get("ueberspringen", "")).split(";") if t.strip()]:
        if akt.split(":")[0] not in AKTIONEN:
            raise SystemExit("%s: unbekannte Aktion: %s" % (wo, akt))
