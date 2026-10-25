# -*- coding: utf-8 -*-
"""Mausbelegung von NetRadiant-Custom 20260114 (Stufe 3 Punkt 0c) fuer
tools/karte/nrc_inventar.py (Abschnitt "Mausbelegung" der Checkliste).

Quelle: docs/"mouse shortcuts.txt" des Tags 20260114, gegen den Code
geprueft (xywindow.cpp, camwindow.cpp, selection.cpp, selection_mtor_*.cpp,
selection_mtable_*.h, texwindow.cpp, xywindow.cpp EntityMenu) und um fest
verdrahtete Tasten ergaenzt, die nicht in der Tastentabelle stehen.

Je Eintrag: (Stand, Belegung, Ansicht, Wirkung bei NRC, Ort bei NRC, Ort bei
uns, Pruefung im Selbsttest "karteneditor"). Stand: x = nachgebaut und mit
echten Maus-/Tastenereignissen samt Wirkung geprueft.
"""

XY = 'gui/app_karte_xy.cpp xyZeichnen'
BEOB = 'gui/app_karte_auswahl.cpp beobachterDruck/-Bewegung/-Loslassen'
WAHL = 'src/karte/auswahlsystem.cpp punktWaehlen'
MANIP = 'src/karte/bearbeitung.cpp manipulatorWaehlen'
KAM = 'gui/app_karte_cam.cpp kameraZeichnen'
TEX = 'gui/app_karte_texturen.cpp texturBrowserZeichnen'
MENUE = 'gui/app_karte_entitaeten.cpp entitaetenMenueZeichnen'

MAUS = [
    # --- 2D und 3D (selection.cpp RadiantWindowObserver, Selector_, Manipulator_) ---
    ('x', 'M1', '2D+3D', 'Tunnel-Auswahl, derselbe Punkt noch einmal: der naechste Treffer', 'selection.cpp:2746 testSelect_simpleM1, :1386 SelectPoint eCycle', WAHL,
     'Maus 2D: M1 = Tunnel-Auswahl, derselbe Punkt noch einmal waehlt den naechsten Treffer (C, Licht, A, dann wieder C); Kamera: Tunnel-Auswahl waehlt den Pinsel'),
    ('x', 'M1 ziehen', '2D+3D', 'neuer Pinsel, Groesse aendern, verschieben', 'xywindow.cpp:977 NewBrushDrag_Begin, selection_mtor_drag.cpp', XY + ', ' + MANIP,
     'Maus 2D: M1 ziehen + Umschalt = neuer Pinsel quadratisch (64 x 64); Kamera: neuer Pinsel (DragNewBrush), gewaehlt, Rueckgaengig "dragTool"'),
    ('x', 'Umschalt+M1', '2D+3D', 'mehrfach auswaehlen (umschalten)', 'selection.cpp:2556 Selector_::mouseDown, SelectPoint_InitPaint', BEOB,
     'Maus 2D: Umschalt+M1 waehlt dazu (B und A/C)'),
    ('x', 'Umschalt+M1 ziehen', '2D+3D', 'Auswahl malen', 'selection.cpp:2565 Selector_::mouseMoved (m1selecting)', BEOB,
     'Maus 2D: Umschalt+M1 ziehen malt die Auswahl (A/C und B gewaehlt)'),
    ('x', 'Umschalt+M2', '2D+3D', 'Tunnel-Auswahl', 'selection.cpp:2511 m2testSelect', BEOB,
     'Maus 2D: Umschalt+M2 = Tunnel-Auswahl (B gewaehlt, kein Entitaetenmenue)'),
    ('x', 'Umschalt+M2 ziehen', '2D+3D', 'Rechteck-Auswahl (waehlen/abwaehlen/umschalten, ganze Tiefe, teilweise Treffer)', 'selection.cpp:2516 SelectArea', BEOB,
     'Rechteck: Pinsel und Licht gewaehlt'),
    ('x', 'Strg+M2', '2D+3D', 'Flaechen-Tunnel-Auswahl', 'selection.cpp:2524 SelectPoint( face )', BEOB,
     'Maus 2D: Strg+M2 = Flaechen-Tunnel (eine Flaeche von B gewaehlt)'),
    ('x', 'Strg+M2 ziehen', '2D+3D', 'Rechteck-Auswahl von Pinsel-Flaechen', 'selection.cpp:2521 SelectArea( face )', BEOB,
     'Maus 2D: Strg+M2 ziehen = Rechteck fuer Flaechen (Flaechen von A, B und C, keine Pinsel)'),
    ('x', 'Alt+M1', '2D+3D', 'im Komponenten-Modus Objekte auswaehlen', 'selection.cpp:2486, :1437 (Mode ePrimitive bei Alt)', BEOB + ', ' + WAHL,
     'Maus 2D: Alt+M1 waehlt im Komponenten-Modus Objekte (B und A/C gewaehlt, Ecken-Modus bleibt)'),
    ('x', 'Strg+Alt+M1', '2D+3D', 'gezeigte/gewaehlte Pinsel-Flaechen herausziehen', 'selection_mtor_drag.cpp:406 (extrude)', MANIP,
     'Maus 2D: Strg+Alt+M1 zieht Flaechen heraus (ein Pinsel mehr)'),
    ('x', 'M1 ziehen + Umschalt', '2D+3D', 'eingerastete Manipulatoren, Clipper; neuer Pinsel quadratisch', 'xywindow.cpp:1031 NewBrushDrag( shift ), selection_mtable_brush.h:63', XY,
     'Maus 2D: M1 ziehen + Umschalt = neuer Pinsel quadratisch (64 x 64)'),
    ('x', 'M1 ziehen + Strg', '2D+3D', 'Kasten einrasten (Verschieben/Skalieren); neuer Pinsel als Wuerfel', 'xywindow.cpp:1031 NewBrushDrag( ctrl ), selection_mtable_brush.h:65', XY,
     'Maus 2D: M1 ziehen + Strg = neuer Pinsel als Wuerfel (64 x 64 x 64)'),
    ('x', 'M1 x2 auf einem Clipper-Punkt', '2D+3D', 'Schnitt ausfuehren (On DoubleClick do)', 'selection.cpp:2756 Clipper_tryDoubleclickedCut', BEOB,
     'Doppelklick: Split'),
    # --- 2D (xywindow.cpp XY_MouseDown) ---
    ('x', 'M2 ziehen', '2D', 'Ansicht verschieben', 'xywindow.cpp:970 Move_Begin', XY, 'Pan: Ursprung x um 200 / Qt-Massstab / Massstab nach links'),
    ('x', 'M2', '2D', 'Entitaetenmenue', 'xywindow.cpp:1081 EntityCreate_MouseUp', XY, 'M2 ohne Bewegung: das Entitaetenmenue (Untermenues info, ...)'),
    ('x', 'Alt+M2 ziehen', '2D', 'schnell zoomen', 'xywindow.cpp:974 Zoom_Begin', XY, 'Alt+M2: 80 Bildpunkte = 40 logische nach unten: zwei Schritte hinaus (je ueber 16)'),
    ('x', 'Alt+M1', '2D', 'im QE-Werkzeug Flaechen/Ecken scheren', 'selection_mtor_drag.cpp:419 (alt vertices drag)', MANIP,
     'Maus 2D: Alt+M1 im QE-Werkzeug schert (eine Flaeche von B steht schraeg)'),
    ('x', 'Strg+M1', '2D', 'Clipper fuer einen Schnitt', 'selection.cpp:2702 ClipperModeQuick', BEOB, 'Strg+M1: Clipper eingeschaltet (schnell)'),
    ('x', 'M3', '2D', 'Blickrichtung der Kamera zum Punkt', 'xywindow.cpp:985 XYWnd_OrientCamera', XY, 'M3: yaw zeigt zum Zeiger (atan2)'),
    ('x', 'Strg+M3', '2D', 'Kamera an den Punkt setzen', 'xywindow.cpp:981 XYWnd_PositionCamera', XY, 'Ctrl+M3: Kamera an den Zeiger (eingerastet), z bleibt'),
    ('x', 'Umschalt+M3', '2D', 'Ursprung der Transformation setzen (Pivot)', 'xywindow.cpp:988 SetCustomPivotOrigin', XY,
     'Maus 2D: Umschalt+M3 setzt den Ursprung der Drehung (40 24, eingerastet), z bleibt'),
    ('x', 'Mausrad', '2D', 'zoomen (zum Zeiger)', 'xywindow.cpp:519 wheelEvent', XY, 'Mausrad: Massstab x 5/4, Weltpunkt unter dem Zeiger bleibt'),
    # --- 3D (camwindow.cpp) ---
    ('x', 'Alt+M1', '3D', 'andere Groessenaenderung (Ebenen nach Schnitt)', 'selection_mtor_drag.cpp:414 (alt resize)', MANIP,
     'Maus 3D: Alt+M1 = die andere Groessenaenderung (B ist danach anders gross)'),
    ('x', 'M1 + Alt', '3D', 'Hoehe des neuen Pinsels; Objekte senkrecht verschieben', 'selection_mtable_brush.h:49, selection_mtable_translate.h:347', MANIP,
     'Maus 3D: M1 + Alt verschiebt nur senkrecht (z anders, x und y gleich)'),
    ('x', 'Strg+M1', '3D', 'Pinsel-Flaeche waehlen', 'selection.cpp:2411 c_modifier_face', BEOB, 'Maus 3D: Strg+M1 waehlt eine Flaeche (die Oberseite von B)'),
    ('x', 'Strg+M1 ziehen', '3D', 'Flaechen-Auswahl malen', 'selection.cpp:2570 (m1selecting, face)', BEOB, 'Maus 3D: Strg+M1 ziehen malt Flaechen (die Oberseiten von B und C)'),
    ('x', 'M2', '3D', 'Freelook an/aus', 'camwindow.cpp:1132 enable_freelook_button_press, :1166 disable_freelook_button_release', KAM,
     'Freelook an (kurzer M2 = Umschalter); Freelook aus (kurzer M2 im Freelook)'),
    ('x', 'M2 ziehen', '3D', 'seitwaerts und hoch/runter (Strafe)', 'camwindow.cpp:733 m_strafe (RightButton)', KAM, 'Maus 3D: M2 ziehen = Strafe seitwaerts (y anders, Winkel gleich)'),
    ('x', 'Alt+M2', '3D', 'um den angeklickten Punkt kreisen', 'camwindow.cpp:1112 camera_orbit_init, :400 Orbit', KAM + ', src/karte/kamera.cpp orbitBeginnen',
     'Maus 3D: Alt+M2 kreist um den angeklickten Punkt (der Abstand zu ihm bleibt, die Kamera wandert, Blickrichtung anders)'),
    ('x', 'M2 x2', '3D', 'Entitaetenmenue (vor der Kamera)', 'camwindow.cpp:1097 context_menu_show (unter 250 ms)', KAM,
     'Doppel-M2 in der Kamera: das Entitaetenmenue (g_bCamEntityMenu), Freelook aus'),
    ('x', 'M3', '3D', 'Textur kopieren (Name, Ausrichtung, Farbe, Lichtstaerke), im Texturbrowser zeigen, Find/Replace fuellen', 'selection.cpp:2451 Scene_copyClosestTexture',
     'gui/app_karte_auswahl.cpp texturDruck', 'M3 (Scene_copyClosestTexture): der Shader der Flaeche im Browser, Status'),
    ('x', 'Alt+M3 / ziehen', '3D', 'Texturname auftragen (gezeigt und gewaehlt)', 'surfacedialog.cpp:1666 (paintTexture)', 'gui/app_karte_auswahl.cpp texturDruck',
     'Maus 3D: Alt+M3 (paintTexture)'),
    ('x', 'Umschalt+M3 / ziehen', '3D', 'Name, Ausrichtung, Lichtstaerke auftragen', 'surfacedialog.cpp:1674 (ePasteValues)', 'gui/app_karte_auswahl.cpp texturDruck',
     'Maus 3D: Umschalt+M3 (paintTexture,Values,LightPower)'),
    ('x', 'Strg+M3 / ziehen', '3D', 'nahtlos zwischen Flaechen und Patches, Lichtfarbe', 'surfacedialog.cpp:1676 (ePasteSeamless)', 'gui/app_karte_auswahl.cpp texturDruck',
     'Maus 3D: Strg+M3 (paintTextureSeamless,LightColor)'),
    ('x', 'Strg+Umschalt+M3 / ziehen', '3D', 'von der kopierten Flaeche projizieren, Lichtstaerke und -farbe', 'surfacedialog.cpp:1678 (ePasteProject)',
     'gui/app_karte_auswahl.cpp texturDruck', 'Maus 3D: Strg+Umschalt+M3 (projectTexture,LightColor&Power)'),
    ('x', 'Alt+Umschalt+M3', '3D', 'Ausrichtung ohne Texturname', 'surfacedialog.cpp:1674 (setShader = !alt)', 'gui/app_karte_auswahl.cpp texturDruck',
     'Maus 3D: Alt+Umschalt+M3 (paintTexDefValues)'),
    ('x', 'Alt+Strg+M3', '3D', 'nahtlose Ausrichtung ohne Texturname', 'surfacedialog.cpp:1676', 'gui/app_karte_auswahl.cpp texturDruck',
     'Maus 3D: Alt+Strg+M3 (paintTexDefValuesSeamless)'),
    ('x', 'Alt+Strg+Umschalt+M3', '3D', 'projizierte Ausrichtung ohne Texturname', 'surfacedialog.cpp:1678', 'gui/app_karte_auswahl.cpp texturDruck',
     'Maus 3D: Alt+Strg+Umschalt+M3 (projectTexDefValues)'),
    ('x', 'Strg + Maus (Freelook)', '3D', 'seitwaerts und hoch/runter', 'camwindow.cpp:733-743 (m_strafeMode)', KAM + ', src/karte/kamera.cpp strafeAusMaus',
     'Maus 3D: Strg + Maus im Freelook = Strafe (Ursprung anders, Winkel gleich)'),
    ('x', 'Strg+Umschalt + Maus (Freelook)', '3D', 'seitwaerts und vor/zurueck', 'camwindow.cpp:744 m_strafe_forward', KAM + ', src/karte/kamera.cpp strafeAusMaus',
     'Maus 3D: Strg+Umschalt + Maus im Freelook = Strafe vor/zurueck (x anders, y und Winkel gleich)'),
    ('x', 'Mausrad', '3D', 'vor/zurueck (zum Zeiger); im Orbit der Abstand', 'camwindow.cpp:1279 wheelmove_scroll, :1222 camera_orbit_scroll', KAM,
     'Mausrad: 100 vorwaerts (Zoom to Mouse pointer)'),
    # --- Texturbrowser (texwindow.cpp:1715-1765) ---
    ('x', 'M1', 'Texturbrowser', 'Textur waehlen und auf die Auswahl legen', 'texwindow.cpp:1726 SelectTexture', TEX,
     'M1: gewaehlt, auf den Pinsel gesetzt (textureNameSetSelected), Status "system/caulk W: ..."'),
    ('x', 'Umschalt+M1', 'Texturbrowser', 'Shader im eigenen Editor oeffnen', 'texwindow.cpp:1744 TextureBrowser_ViewShader', TEX,
     'Maus Texturbrowser: Umschalt+M1 oeffnet den Shader im eigenen Editor (system.shader)'),
    ('x', 'Strg+Umschalt+M1', 'Texturbrowser', 'Shader im externen Editor oeffnen (nur lose Dateien)', 'texwindow.cpp:899 DoShaderView( external )', TEX,
     'Maus Texturbrowser: Strg+Umschalt+M1 = externer Editor (loser Shader: editor.exe mit der Datei; im Paket der eigene)'),
    ('x', 'M2', 'Texturbrowser', 'Kontextmenue (Weg unter 16)', 'texwindow.cpp:1740 TextureBrowser_ContextMenu', TEX,
     'Kontextmenue: alle Eintraege da (Name, Open Shader, Shader Info, Select ..., Recent, Shader\'s Folder, system/)'),
    ('x', 'M2 ziehen', 'Texturbrowser', 'rollen', 'texwindow.cpp:874 TextureBrowser_Tracking_MouseDown', TEX,
     'M2 gezogen: nach oben gerollt (originy + dy), kein Kontextmenue (Weg >= 16)'),
    ('x', 'M2 ziehen + Umschalt', 'Texturbrowser', 'viermal so schnell rollen', 'texwindow.cpp:882 (scale 4)', TEX,
     'Maus Texturbrowser: Umschalt+M2 ziehen rollt viermal so schnell (20 logische Punkte -> 80), kein Kontextmenue'),
    ('x', 'M1 x2', 'Texturbrowser', 'Verzeichnis der Textur laden', 'texwindow.cpp:1731 TextureBrowser_ShowDirectoryOfShader', TEX,
     'Doppelklick: Verzeichnis des Shaders (common/)'),
    ('x', 'M3', 'Texturbrowser', 'Textur waehlen, nicht auftragen', 'texwindow.cpp:1726 SelectTexture( middle )', TEX, 'M3: gewaehlt, der Pinsel behaelt seinen Shader'),
    ('x', 'Mausrad', 'Texturbrowser', 'rollen', 'texwindow.cpp:1749 wheelEvent', TEX, 'Rad: um 64 gerollt (geklemmt)'),
    # --- Entitaetenmenue (xywindow.cpp:757-805 EntityMenu) ---
    ('x', 'M1', 'Entitaetenmenue', 'Entity erzeugen (aus gewaehlten Pinseln)', 'xywindow.cpp:742 entitycreate_activated', MENUE,
     'info_null erzeugt bei 64 64 (Mitte der Workzone auf dem Raster), gewaehlt, entityCreate -class info_null; Maus Entitaetenmenue: M1 auf func_group macht aus dem gewaehlten Welt-Pinsel eine Gruppe'),
    ('x', 'M1 auf worldspawn', 'Entitaetenmenue', 'gewaehlte Primitive aus der Gruppe loesen', 'entity.cpp:124 (m_2world)', MENUE,
     'Maus Entitaetenmenue: M1 auf worldspawn loest die gewaehlten Primitive aus der Gruppe (zurueck in die Welt, Gruppe weg)'),
    ('x', 'M2', 'Entitaetenmenue', 'gewaehlte Entities umwandeln (worldspawn: Gruppen aufloesen)', 'xywindow.cpp:778 Scene_EntitySetClassname_Selected', MENUE,
     'Maus Entitaetenmenue: M2 wandelt die gewaehlte Entity um (light -> info_null, wie NRC danach nicht gewaehlt), das Menue schliesst'),
    ('x', 'Strg+M1', 'Entitaetenmenue', 'erzeugen, das Menue bleibt offen', 'xywindow.cpp:769', MENUE, 'Strg+Klick: info_notnull erzeugt, das Menue bleibt offen'),
    ('x', 'Strg+M2', 'Entitaetenmenue', 'umwandeln, das Menue bleibt offen', 'xywindow.cpp:783 (m_hide_menu)', MENUE,
     'Maus Entitaetenmenue: Strg+M2 wandelt um (info_null -> info_notnull), das Menue bleibt offen'),
    # --- fest verdrahtet (nicht in der Tastentabelle) ---
    ('x', 'Alt / Umschalt / Strg druecken und loslassen', '2D+3D', 'g_modifiers, Hervorhebung des Manipulators neu (onModifierDown/Up)',
     'windowobservers.cpp:60-95, selection.cpp:2767', 'gui/app_karte_auswahl.cpp zusatztastenPruefen',
     'Maus: Alt gedrueckt -> g_modifiers mit Alt (onModifierDown); Maus: losgelassen -> g_modifiers leer (onModifierUp)'),
    ('x', 'Return / Umschalt+Return / Strg+Return', 'Clipper', 'schneiden / teilen / umdrehen', 'clippertool.cpp (ClipperClip, ClipperSplit, ClipperFlip)',
     'gui/app_karte_befehle.cpp', 'Return: Clip, es bleibt x 0..64 (Normale (p2-p0) x (p1-p0) = -x); Umschalt+Return: Split in zwei Pinsel; Strg+Return: Flip'),
    ('x', 'Esc', 'Entitaetenmenue', 'Untermenue und Menue schliessen, ohne Kuerzel', 'QMenu keyPressEvent', MENUE,
     'Esc schliesst Untermenue und Menue, ohne Kuerzel (UnSelectSelection2): der zweite bleibt gewaehlt'),
    ('x', 'Esc', 'Texturbrowser-Filter', 'Filter leeren, Auswahl bleibt', 'texwindow.cpp:1663 (ShortcutOverride)', TEX,
     'Esc im Filterfeld: leer (ShortcutOverride), keine Auswahl aufgehoben'),
    ('x', 'Return / Esc / Tab / Leertaste', 'Arbitrary rotation/scale', 'uebernehmen / abbrechen / anwenden', 'select.cpp:1637-1645, 1729-1737 (eventFilter)',
     'gui/app_karte_transform.cpp', 'Return: OK - noch einmal 90, zu, die Werte bleiben; Esc: Cancel - zu, Werte 0, die Auswahl bleibt; Arbitrary rotation: Tab und Leertaste gehen an den Dialog (kein Klonen, kein Kamera-Fokus), er bleibt offen'),
    ('x', 'Teiler ziehen / bis 0 / zurueck', 'alle Anordnungen', 'QSplitter (childrenCollapsible): Feld klappt ein, Griff bleibt am Rand, laesst sich herausziehen',
     'qsplitter.cpp moveSplitter, mainframe.cpp:1816-1862', 'src/karte/teiler.cpp, gui/app_karte.cpp teilerZeichnen',
     'Teiler Regular Griff:h:1: bis ans Ende gezogen klappt Kamera ein, der Griff bleibt am Rand; Teiler Regular Griff:h:1: vom Rand zurueckgezogen ist Kamera wieder da'),
    ('x', 'F12 (Maximize view) und noch einmal', 'Regular/Split', 'das Feld unter der Maus fuellt den Bereich; zurueck wie vorher', 'mainframe.cpp:1550-1595',
     'gui/app_karte.cpp maximierenUmschalten', 'Maximize view (F12): die Kamera unter der Maus fuellt den Bereich, XY, Konsole und Texturen sind eingeklappt; Maximize view (F12) noch einmal: alle Groessen wie vorher'),
]
