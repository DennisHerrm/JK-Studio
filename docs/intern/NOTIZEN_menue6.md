# Notizen Menü-Editor, Stufe M6 (Zweig menue6 ab menue5)

Auftrag: SP\auftrag_m6.md mit SP\auftrag_m6b.md (Reihenfolge) und SP\auftrag_gemeinsam.md. Grundsätze
wie M5 (alles leicht bedienbar, normaler .menu-Code, nichts zuschneiden). Vorwissen NOTIZEN_menue3.md
(M4) und NOTIZEN_menue5.md (M5/M5b). Texte tools/i18n_menue6.py, Handbuch
tools/handbuch/inhalt_menue6.py, GUI gui/app_menue_m6.inc.

## Punkt 1 - Menüfluss

- Modul src/menue/fluss.cpp (include/bhed/menue/fluss.h): Knoten = alle menuDefs der offenen
  Dateien, Kanten = jedes `open` in onOpen/onClose/onESC/onAccept und in den Item-Ereignissen
  (action, onFocus, leaveFocus, mouseEnter/Exit, accept, selectNext/Prev, doubleClick), zerlegt wie
  Item_RunScript (ereignisse.cpp zerlege). Namen ohne Groß/Klein (Menus_FindByName), über Dateien
  hinweg; `close all` = Menus_CloseAll; `openGoToMenu` zählt als Ausgang (Ziel merkt ein uiScript).
- Einstiege = Menüs, die die Engine mit festem Namen öffnet: SP code/ui/ui_main.cpp (UI_MainMenu
  mainMenu, UI_SetActiveMenu, Datapad, Missions-/Hilfe-Menüs, error_popmenu, setup_menu2 ...) und
  code/cgame (loadscreen aus cg_main.cpp/cg_info.cpp, lefthud/righthud aus cg_draw.cpp); MP
  codemp/ui/ui_main.c (UI_SetActiveMenu main/team/endofgame/ingame..., Connect aus
  UI_DrawConnectScreen, password_request, bad_server).
- Befunde: unerreichbar (vom Einstieg über open nie erreicht), Sackgasse (nimmt Eingaben an - Knopf
  mit action, Eingabe-Typen oder onESC -, aber kein open eines anderen Menüs und kein close
  seiner selbst/all), ohne Weg zurück (wird geöffnet, aber kein close seiner selbst, kein onESC mit
  close/open und kein open eines Menüs, das es öffnet), fehlt (open-Ziel ohne menuDef).
  Menüs ohne Eingabe (HUD, Ladebildschirm) sind keine Sackgassen.
- Oberfläche: Knopf „Menüfluss“ in der Leiste, Fenster mit Spalten je Schritt vom Einstieg,
  Pfeilen, Farben (grüner Rahmen Einstieg, rot Sackgasse/fehlt, orange ohne Zurück, grau nie
  erreicht), Tooltip mit Herkunft der Kanten („knopf: action“), Problemliste in normaler Sprache
  mit Lösung; Klick springt in Dokument und Seite. Nur eine Datei offen: Hinweis + Knopf
  „Menüsatz öffnen“.
- ctest menue_fluss (Sackgasse, unerreichbar, ohne Zurück, onESC zurück, fehlendes Ziel, über
  Dateien, MP-Einstieg main). Selbsttest (BHED_MENUE_NUR=m6): Aufgabe „finde das Menü ohne Ausgang“
  2 Schritte ohne Text-Reiter.

## Punkt 2 - Tab-/Fokus-Reihenfolge

- Laufzeit::tabReihenfolge (skript.cpp): Menu_SetNextCursorItem 1:1 ab cursorItem -1 (Menu_Init) mit
  den Prüfungen von Item_SetFocus (keine Dekoration, sichtbar nach onOpen inkl. ownerDrawFlags und
  showCvar, SP nicht WINDOW_INACTIVE, enableCvar). Engine-Eigenheit: ab -1 setzt die Schleife
  cursor=0 und zählt vor der ersten Prüfung hoch - der erste Tab landet beim zweiten Element der
  Datei, Index 0 erst nach dem Umlauf. Text-Items nimmt Item_SetFocus an (gibt qtrue zurück), sie
  stehen also in der Folge. `item->disabled` (Script „disable“, Item_ApplyHacks) wird in der
  Vorschau nicht geführt.
- Neu ordnen: itemsNeuOrdnen (werkzeuge.cpp) setzt die itemDef-Blöcke der geklickten Folge auf die
  Plätze der jetzigen Tab-Folge; Elemente ohne Fokus bleiben stehen. Da das Spiel in derselben
  Reihenfolge zeichnet, sagt die Leiste das dazu.
- Oberfläche: Schalter „Tab-Reihenfolge“ (Ansicht), Nummern auf der Fläche, Klickfolge (grün =
  schon neu vergeben), „Neu beginnen“, „Fertig“; nach dem letzten Klick eine Ersetzung = ein
  Rückgängig-Schritt.
- ctest menue_fokus; Selbsttest: 4 echte Mausklicks auf die Fläche, Folge wie geklickt, ein
  Rückgängig byte-gleich.

## Punkt 4 - Stile zentral

- Modul src/menue/stile.cpp: Werte je Art (Farben aus forecolor/backcolor/bordercolor/outlinecolor/
  focuscolor/disablecolor/desccolor, font, textscale, background), einheitlich geschrieben (%g),
  häufigste zuerst; Ersetzen an allen Stellen der Hauptdatei (je Wort eine Ersetzung: Abstände,
  Kommentare, Anführungszeichen bleiben); Werte aus #include/Makro gezählt, nicht geändert.
- Fenster „Stile“ (Leiste): Farbwähler, Ziehfeld für Textgrößen, Textfeld für Schrift/Hintergrund;
  ersetzt erst beim Loslassen (ein Rückgängig-Schritt). ctest menue_stile.

## Punkt 5 - Vorlagen ganzer Menüs

- Modul src/menue/seitenvorlagen.cpp: Hauptmenü, Einstellungsseite mit drei Reitern (ui_jks_tab,
  cvarTest/showCvar - der Editor zeigt sie als Seiten), Credits, Ladebildschirm; nur Schlüssel, die
  alle Fassungen lesen. Namen wie die Engine sie sucht: SP mainMenu/loadscreen, MP main/Connect;
  das Hauptmenü öffnet settingsMenu und creditsMenu. Bereiche der Schieber wie in den Setup-Menüs
  (r_gamma 0.5-3, s_volume/s_musicvolume 0-1, sensitivity 2-30).
- Oberfläche: das „+“ neben den Seiten öffnet eine Auswahl (leere Seite oder Vorlage).
- ctest menue_seiten: JA/JK2/MD SP und JA/JK2 MP lesen alle vier ohne Meldung, Fluss ohne Befund,
  drei Reiter.

## Punkt 6 - Einstellungsseiten-Assistent

- Modul src/menue/assistent.cpp: Katalog aus den Original-Menüs des Spiels (Menüsatz des
  Mod-Profils): ITEM_TYPE_SLIDER mit cvarFloat-Bereich, YESNO, MULTI mit cvarFloatList/cvarStrList
  (Feeder-Listen nicht - die füllt der Code); erste Fundstelle gilt. Seite mit Titel, Steuerungen
  (ab 11 zwei Spalten) und Zurück-Knopf.
- Fenster „Einstellungs-Assistent“ (Leiste): Suche, Ankreuzen, Knopf „Seite mit N Einstellungen
  anlegen“. ctest in menue_seiten.

## Punkt 7 - Referenzbild

- Knopf „Referenzbild“ (Leiste): PNG/JPG/TGA, Deckkraft, unter oder über dem Menü; nur Anzeige
  (nie im Menü oder in der pk3), bei Gerätewechsel neu geladen.

## Punkt 8 - Vergleich mit dem Original

- itemUnterschiede/itemErsetzen (werkzeuge.cpp): Schlüssel, deren Werte (wie im Text) abweichen,
  fehlende und zusätzliche zählen mit. Eigenschaften-Abschnitt „Vergleich mit dem Original“ für
  Menüs, die es im Spiel gibt (gleicher Spielpfad, gleiches menuDef, gleicher Item-Name), Knopf
  „Auf Original zurücksetzen“. ctest menue_werkzeug.

## Punkt 9 - Lesbarkeit

- Modul src/menue/lesbarkeit.cpp: Texthöhe und -breite mit textRechteck der Vorschau
  (RE_Font_HeightPixels/StrLenPixels wie im Spiel), Kontrast nach WCAG 2 (relative Leuchtdichte)
  nur auf gefüllten Flächen (style FILLED, backcolor-Alpha >= 0.5); Textlänge je Sprache: ein
  Befund nur, wenn der Text in einer Sprache ins Feld (rect) passt und in einer anderen nicht -
  sonst ist das rect bewusst nur die Klickfläche (viele Original-Menüs). Schwellen 8 Punkte und
  3:1 sind Richtwerte des Editors, keine Engine-Grenzen (so im Tooltip).
- Oberfläche: „Lesbarkeit: N Hinweise“ in der Grenzen-Zeile, Tooltip mit Lösung je Element,
  Klick wählt das nächste Element. ctest menue_lesbar.

## Punkt 10 - Live-Grenzen

- auslastung() (pruefung.cpp) mit derselben Skriptlänge wie die Prüfung vor dem Export
  (skriptLaenge, PC_Script_Parse); Zeile unter den Seiten: Elemente/MAX_MENUITEMS, Menüs/MAX_MENUS,
  längstes Skript/Grenze, orange ab 80 %, rot an der Grenze, Tooltip mit dem längsten Skript.
  ctest menue_pruefung.

## Punkt 13 - Neu laden im laufenden Spiel (nur geprüft)

- Befehl `ui_load` belegt: OpenJK codemp/ui/ui_atoms.c (Befehlstabelle, UI_Load: UI_LoadMenus(menuSet,
  qtrue), Menus_CloseAll, Menus_ActivateByName(zuletzt fokussiertes Menü)), code/ui/ui_atoms.cpp
  (nur `#ifndef JK2_MODE` - in Jedi Outcast SP also nicht), Movie Duels code/ui/ui_atoms.cpp gleich.
  Für JK2 MP liegt kein Quellcode vor (nicht belegbar).
- Der Befehl liest die Menüs neu über das Dateisystem des Spiels: lose Dateien unter fs_basepath/base
  werden neu gelesen, eine während des Laufs ersetzte pk3 nicht (FS liest das Zip-Verzeichnis beim
  Start). Ein Befehl von außen an das laufende Spiel ist nicht vorgesehen (kein IPC; rcon nur an
  einen Server, ui_load ist ein Client-Befehl).
- Nicht umgesetzt (offen): „Im Spiel testen“ müsste zusätzlich lose Dateien in den eigenen
  fs_basepath schreiben und den Anwender `ui_load` in die Konsole tippen lassen.

## Rest aus M5b

- Reiter mit ungespeicherten Änderungen schließen fragt: Speichern / Ohne Speichern schließen /
  Abbrechen (Selbsttest).

## Punkt 15 - Handbuch und Selbsttest

- Handbuchseite „Menü-Editor: Fluss, Tab-Reihenfolge, Stile und Vorlagen“
  (tools/handbuch/inhalt_menue6.py) in vier Sprachen mit sieben Bildern aus dem Selbsttest
  (menue_fluss, menue_tabfolge, menue_stile, menue_vorlage, menue_assistent, menue_referenzbild,
  menue_vergleich; SP\m6\hb_fotos.sh, tools/handbuch/bilder.py). Erst hieß das Vergleichsbild
  menue_original - der Name war schon vom M1-Bild belegt und hätte es überschrieben; umbenannt,
  das alte Bild aus git zurückgeholt.
- Selbsttest `menueeditor` um M6 erweitert (BHED_MENUE_NUR=m6, auch im vollen Lauf): 16 Prüfungen
  mit Aufgabentests - Sackgasse finden (2 Klicks), Tab-Folge c b a zurueck (Schalter + 4 echte
  Mausklicks, ein Rückgängig byte-gleich), Vorlage über das + (2 Klicks), Assistent (159 Cvars mit
  Steuerung aus den Movie-Duels-Menüs, Seite mit 2 Einstellungen), Original (rect geändert,
  zurückgesetzt, Text wie im Spiel), Referenzbild (Text unverändert), Reiter schließen fragt.
- Fund beim ctest menue_lesbar (rot, behoben, grün): die Textlänge je Sprache maß den
  @-Schlüssel selbst statt des übersetzten Textes (textRechteck löst nicht auf) - jetzt wie das
  Spiel über Texte::imSpiel (unbekannt = nichts).
- Fund beim Handbuchbild: die Tab-Nummer lag links oben über dem Textanfang („A“, „B“, „C“
  verdeckt) - jetzt rechts oben.

## Schlussrunde

- Sauberer Neubau (`--clean-first`, alle Ziele, unter Last der anderen Agenten 07:46-08:10):
  0 Warnungen, 0 Fehler (/W4 /WX).
- lint_sources, lint_fassung: ok. karte_lizenz grün (fluss.h/.cpp nennen „Portiert aus OpenJK“ -
  zuerst rot, weil der Kopf nur die GPL nannte).
- ctest alle: 164/164 grün (darunter neu menue_fluss, menue_fokus, menue_stile, menue_seiten,
  menue_lesbar; erweitert menue_pruefung, menue_werkzeug). Hinweis: mit `-j 2` gelaufen (zwei
  Testprogramme gleichzeitig, nur kurze Konsolenprogramme).
- Selbsttest menueeditor voll (Movie-Duels-Daten): 119 OK, 0 FEHLER (M5b 103 + M6 16).
  Mit JKA-Daten aus Zeitgründen nicht wiederholt.
- karteneditor kurz (BHED_KARTE_NUR=fotos): 78 OK, 0 FEHLER.
- Modus 1 komplett: 2645 OK, 0 FEHLER.
- Gegen das Vorbild geprüft: Tab-Folge gegen Menu_SetNextCursorItem/Item_SetFocus (Quelltext
  OpenJK SP/MP, Movie Duels gleich; ctest mit der Eigenheit „erster Tab auf Index 1“), Einstiege
  des Flusses gegen ui_main/cgame, Vorlagen und Assistenten-Seiten gegen die Parser-Tabellen aller
  Fassungen (ohne Meldung gelesen), Grenzen gegen ui_shared.h/PC_Script_Parse (wie die Prüfung
  vor dem Export). Nicht im Spiel geprüft (kein Spielstart in M6): wie die Vorlagen im laufenden
  Spiel aussehen und ob `ui_load` sie neu lädt.
- Keine Spiel-exes gestartet, nichts in Spielordner geschrieben; keine eigenen Prozesse offen.

## Stand - fertig / offen / nächster Schritt

- Fertig (mit ctest und Selbsttest): 1 Menüfluss, 2 Tab-Reihenfolge, 4 Stile, 5 Vorlagen,
  6 Einstellungs-Assistent, 7 Referenzbild, 8 Vergleich/Zurücksetzen, 9 Lesbarkeit, 10 Live-Grenzen,
  Rest M5b „Reiter schließen fragt nach“; 13 geprüft und belegt (siehe oben), nicht umgesetzt.
- Offen (nicht begonnen): 3 Komponenten (verknüpfte Kopien mit Kommentar-Kennung), 11 Lade-,
  Verbindungs- und Ingame-Menüs mit Beispielwerten, 12 Modelle per Gizmo ausrichten, 13 lose
  Dateien + `ui_load`-Hinweis bei „Im Spiel testen“, 14 restliche Platzhalter aus M4 (UI_GLINFO,
  Datapad 203/204/206, MP UI_CHAT_*, Macht-Ränge, MD Pazaak ...). Aus M5b weiter offen: Startmenü
  eines neuen Projekts im MP-Format (die Projektanlage fragt noch nicht nach SP/MP), JK2
  Tastenbelegungen (Item_Bind_Paint zeigt „???“ - BindingFromName bräuchte die binds aus
  default.cfg/Config, für alle Spiele), Renderer-Cvars der JK2-Setup-Menüs, Logo-Chromglanz.
- Abweichungen/bewusst so: die Tab-Folge berücksichtigt `item->disabled` nicht (Script „disable“
  und Item_ApplyHacks führt die Vorschau nicht); die Lesbarkeits-Schwellen sind Richtwerte.
- Nächster Schritt: 14 (Platzhalter nach Engine-Code), dann 11/12, dann 3.

## M7 (Auftrag SP\auftrag_m7.md, 30.10.)

Reihenfolge: 14 Platzhalter, M5b-Reste (Tasten, MP-Startmenü, JK2-Setup, Chromglanz), 11 Lade-/
Verbindungs-/Ingame-Menüs, 13 ui_load bei „Im Spiel testen“, 3 Komponenten, 12 Gizmo, Selbsttest JKA +
Spielfotos der Vorlagen. Steam läuft seit 30.10. 09:30 (Original-JKA/JK2 erlaubt, SP\spiel.lock).

### Punkt 14 - Bestand (SP\m7\od_inventar.py über die Original-pk3s)

- JKA SP: 203/204/206 (Datapad), 236, 249 (UI_GLINFO), 250. JKA MP: UI_GLINFO, 250, ALLMAPS_SELECTION,
  Teamplätze, BOTNAME/BOTSKILL/REDBLUE, CHAT_* (7), FORCE_SIDE/RANK/POINTS/MASTERY_SET, 18 FORCE_RANK_*,
  JEDI_NONJEDI, JOINGAMETYPE, NETFILTER, NETSOURCE, NETGAMETYPE, SELECTEDPLAYER, SERVERREFRESHDATE,
  SKILL, SKIN_COLOR, STARTMAPCINEMATIC. JK2 MP zusätzlich UI_EFFECTS, UI_SERVERMOTD; JK2 SP 203-206.
- Movie Duels SP: 203-206, 236, 249, 250, 257 (Duellkarten), Pazaak 2001-2900 (sje_pazaak);
  MD-MP-Menüs (0_MD_Assets_MP_UI.pk3) wie JKA MP plus 340-350 (Kräfte der dritten Spalte).
- Feeder ohne Inhalt bisher: MP SABER_SINGLE/STAFF_INFO, PLAYER_LIST, SERVERSTATUS, FINDPLAYER, SIEGE_*;
  MD 85-89 (Modelle, Varianten, Duellkarten, Griffe).
- Befund aus dem Code: UI_CHAT_* hat in UI_OwnerDraw (codemp ui_main.c) keinen Fall - das Spiel zeichnet
  nichts (nur UI_OwnerDrawHandleKey); UI_EFFECTS zeichnet in JKA MP nichts.

### Punkt 14 - umgesetzt

- Modul src/menue/ownerdraw.cpp (ownerdraw.h): DC->ownerDrawItem als Liste von Zeichenbefehlen (Text mit
  item-Schrift oder cgame-Schrift beim Namen, Bild, Fläche); vorschau.cpp portiert Item_OwnerDraw_Paint
  (Farbe mit colorRanges/getValue 0, Fokus-Puls, Blinken, enableCvar; mit Text +8, MP nur bei nicht
  leerem Text). Die MP-Fälle aus FeederDaten::ownerDraw (M5) sind dorthin umgezogen (keine doppelte Logik).
- MP (codemp ui_main.c/ui_force.c): Kräfte aus der cvar forcepowers wie UI_UpdateForcePowers +
  UpdateForceUsed (Rang = uiMaxRank, Punkte nach bgForcePowerCost, freier Sprung/Säbel), UI_DrawForceStars
  (forcestar0/forcecircle<n>/forcestar<n>, 16x16, Abstand 20, y + 6), UI_FORCE_SIDE samt
  Menu_ShowItemByName (lightpowers/darkpowers..., in der Vorschau auf das ganze Menü angewandt),
  JEDI_NONJEDI (nur mit g_jediVmerc), FORCE_POINTS/RANK/MASTERY_SET, JOINGAMETYPE (joingametypes aus
  gameinfo.txt, Grenze "> num" wie die Engine), NETSOURCE (NET_SOURCES-Zeile + Nummer), NETFILTER,
  ALLMAPS_SELECTION, BOTNAME/BOTSKILL/REDBLUE (uiInfo nach dem Start 0), SELECTEDPLAYER,
  SERVERREFRESHDATE, SERVERMOTD (Jedi Outcast), KEYBINDSTATUS. UI_CHAT_* und UI_EFFECTS (JKA MP)
  zeichnen im Spiel nichts - so auch die Vorschau. Jedi Outcast MP UI_EFFECTS: fx_base + fxPic[gamecodetoui
  [color1-1]] (ui.qvm enthält die fx-Bilder und color1; ohne Quellcode, Q3TA-Erbe - im Spiel zu prüfen).
- UI_GLINFO SP/MP: glGetString des echten OpenGL-Treibers (platform::openGlTreiber, verstecktes Fenster,
  Kontext gleich wieder weg); Bits wie OpenJK GLimp_SetMode (24/24/8); MP kürzt auf 128 Erweiterungen
  (TruncateGLExtensionsString), zwei Spalten, SP eine Spalte mit der Zeilenzählung samt numLines--.
- Datapad JKA (datapad.cpp, hud.cpp zeichneDatapadHud): CG_DrawDataPadHUD (Striche aus ui/hud.menu,
  message_off, gfx/menus/datapad), Ziele (ObjectivePrint_Line inkl. Umbruch mit +2 je Zeichen,
  CT_BLUE3-Kreise), Waffen (weaponIcon aus ext_data/weapons.dat, Erschütterungs-Sonderfall), Kräfte
  (showDataPadPowers, FP der SP-Fassung), Inventar leer (SP_INGAME_EMPTY_INV, Breite mit ocr_a gemessen,
  mit ergoec gezeichnet). Zeilenumbruch von CG_DisplayBoxedText und Textscroll jetzt eine Funktion
  (naechsteZeile).
- Movie Duels: Pazaak (pazaak.cpp, ui_pazaak.cpp: Karten, Werte, Namen mit Schatten, Punkte mit ^5,
  Dialog, Zeit, Kartenwahl mit Anzahl, Anleitung) mit Beispielpartie; UI_DUELMAPS_SELECTION;
  Feeder 84-89 aus den Tabellen von ui_main.cpp (tools/gen_md_tabellen.py -> src/menue/md_tabellen.inc:
  727 Figuren, 154 Duellkarten, Griffe).
- Feeder: FeederDaten::art (0 = ohne Darstellung -> Platzhalter, 1 = wie im Spiel, auch leer, 2 = Beispiel).
  MP SABER_SINGLE/STAFF_INFO wie WP_SaberGetHiltInfo (Parser mit SkipBracedSection-Eigenheit),
  PLAYER_LIST als Beispiel; Serverliste/-status/Spielersuche/Siege-Listen leer wie im Hauptmenü.
- Beispielwerte beschriftet („Beispielwerte“ oben links, nur beim Bearbeiten), Abschnitt Beispielwerte in
  den Eigenschaften (Waffe, Kraft, Stufe, Ziele, HUD-Werte; MP g_jediVmerc, g_forceBasedTeams, Höchstrang,
  Serversuche, MOTD).
- Tests: ctest menue_ownerdraw (neu), menue_vorschau angepasst. Selbsttest BHED_MENUE_NUR=m7 zählt die
  Platzhalter aller Original-Menüs: Movie-Duels-Daten vorher (Stand menue6, Liste oben) alle MP-/Datapad-/
  Pazaak-/GLINFO-Nummern, jetzt 22: MD-MP 340-350 und das MD-Datapad (203-206).
- Offen mit Begründung: MD-MP 340-350 (UI_FORCE_RANK_STASIS .. DARK_STASIS laut ui/MD_MP/menudef.h aus
  z_zzzz_MD_Jace-Update-9.pk3) - kein Quellcode der MD-MP-Oberfläche; die DLL enthält eine 29-zeilige
  Kostentabelle, die von OpenJK abweicht (Sprung, Säbelwurf) - also zeichnet MD-MP auch die normalen
  Kraftsterne anders als OpenJK; nicht 1:1 portierbar ohne Quelle. Der Selbsttest zählt sie getrennt
  („davon ohne Quelle“) und wertet nur andere Platzhalter als Fehler. Datapad Movie Duels/Jedi Outcast:
  erledigt (Abschnitt „Punkt 14 - Datapad von Jedi Outcast und Movie Duels“ unten).

### M5b-Reste

- Tastenbelegung (tasten.cpp, tasten.h): Item_Bind_Paint mit BindingFromName/Controls_GetKeyAssignment
  (die ersten zwei Tasten in Tastennummer-Folge, Name per Key_KeynumToString bzw. KEYNAMES_KEYNAME_*,
  "<a> <MENUS_KEYBIND_OR> <b>", Jedi Outcast SP MENUS3_KEYBIND_OR; Befehl nicht in g_bindCommands oder
  ohne Taste: ???), Schrift bis zum Schirmrand in 0,05-Schritten kleiner (SP misst mit qhMediumFont).
  Bindungen aus der Vorgabe-Config, wie Com_Init sie ausführt: default.cfg / mpdefault.cfg, Movie Duels
  MovieDuels-SP-default.cfg / MovieDuels-MP-default.cfg (bind, unbind, unbindall, exec; Befehle durch ";"
  und Zeilen, Anführungszeichen, // wie Cbuf/Cmd_TokenizeString). Tastennamen (320, keynum nicht immer
  der Index: SHIFT_ENTER = A_ENTER) und die g_bindCommands-Listen (SP, JK2_MODE, MP, MD) erzeugt
  tools/gen_tastennamen.py aus den Quellen (src/menue/tasten.inc). ctest menue_tasten.
- Startmenü eines neuen Projekts im MP-Format: Wahl Einzel-/Mehrspieler im Projektmenü und im leeren
  Zustand; MP legt ui/jamp/ (JK2 ui/jk2mp/, MD ui/MD_MP/) mein_menue.menu an - dort liest die Engine
  menudef.h vorab, das Startmenü (WINDOW_STYLE_FILLED) liest ohne Meldung. Selbsttest m7.
- Renderer-Cvars der Setup-Menüs (alle Spiele, nicht nur JK2): tools/gen_menue_cvarvorgaben.py liest
  jetzt auch, was die Engine vor der Oberfläche anlegt (qcommon/common.cpp, shared/sdl/sdl_window.cpp,
  client/cl_main.cpp, client/snd_dma.cpp, rd-vanilla/tr_init.cpp; erstes Cvar_Get gewinnt): kEngineJa
  (207, auch für JK2 = OpenJK code/ mit JK2_MODE), kEngineMd (204), kEngineMp (253). Z. B. r_textureMode
  SP GL_LINEAR_MIPMAP_LINEAR, MP GL_LINEAR_MIPMAP_NEAREST. ctest in menue_ownerdraw.
- Logo-Chromglanz (Jedi Outcast jk2logo.md3, Shader models/map_objects/bespin/jk2logo mit zweiter Stufe
  gfx/menus/jk2logo, blendFunc GL_ONE GL_ONE, tcGen environment): die Menü-Figuren nehmen seit rc602/603
  alle Stufen des Shaderskripts (texturesFor -> ModelTextures::stufen, gpu::zeichneFigur mit tcGen
  environment). Prüfung im Selbsttest BHED_MENUE_NUR=m7jk2 (Zahl der Stufen der Logo-Flächen).

### Punkt 11 - Lade-, Verbindungs- und Ingame-Menüs mit Beispielwerten

- ladebild.cpp: SP-Menü „loadscreen“ wie CG_DrawInformation (cg_info.cpp): CG_DrawLoadingScreen setzt
  ui_missionbriefing (@BRIEFINGS_<karte>, sonst @BRIEFINGS_NONE), zeichnet loadScreen background/mappic
  (Levelshot, sonst menu/art/unknownmap) mit der Item-Farbe, die Waffensymbole (weaponIcon aus
  weapons.dat, Reihen weaponicons_singlerow/row1/row2, 60 + 12) und Kraftsymbole (showPowers, 40 + 12),
  dann das Menü mit forcePaint, CG_LoadBar (416 breit, Striche 40) und CS_MESSAGE bei (15, 20); erste Karte
  yavin1 ohne Spielstand: schwarz mit SP_INGAME_ALONGTIME. MP-Menü „Connect“ wie UI_DrawConnectScreen
  (Text_PaintCenter mit SHADOWEDMORE, Karte/Server/Zustand/Pakete/MOTD). Beispielwerte im Abschnitt
  Beispielwerte, Schalter wie beim HUD. Ingame-Menüs: Datapad, MP-Kraft/Spieler/Server (Punkt 14).
- ctest menue_ladebild (künstliche Schrift: Lagen von Bildern, Ladebalken, Text). Selbsttest m7 mit
  JKA-Daten: Ladebalken 4 Teile, Beispiel beschriftet; Bild menue_ladebild.

### Punkt 13 - ui_load im laufenden Spiel

- „Im Spiel testen“: kann der Editor ui_load im laufenden Spiel auslösen (MenueTestPlan::uiLoad: OpenJK
  und Movie Duels; nur im eigenen Ordner, nie im Mod-Ordner), kommen die Dateien lose in <test>/<spiel>/
  (eine alte pk3 dort wird entfernt, sie stünde darüber). Der Editor merkt sich die Prozessnummer
  (losgeloestStarten mit pid); nach jedem Speichern (Datei oder Projekt) schreibt er die Dateien neu und gibt
  `\ui_load` in die Konsole des Spiels (platform::spielKonsolenBefehl: PostMessage nur an das Fenster dieses
  Prozesses; Shift+Esc öffnet/schließt die Konsole - OpenJK CL_KeyDownEvent, Zeichen per WM_CHAR, Enter).
- Nachweis Movie Duels (SP\m7\uiload.py md): MovieDuels-SP mit der Befehlszeile von „Im Spiel testen“
  (fs_homepath eigener Ordner, fs_game MD, Fenster, stumm), Selbsttest m7uiload übernimmt die PID, fügt
  „JK STUDIO UI_LOAD“ ins Hauptmenü, speichert -> lose Datei + ui_load; screenshot_tga vorher/nachher:
  der Text erscheint im laufenden Spiel ohne Neustart (SP\m7\uiload_md_neben.png).
- Original-JKA (jasp.exe über Steam; SP\m7\orig_kopie.py kopiert die Original-Dateien in das leere GameData,
  hält SP\spiel.lock und räumt danach auf, „GameData wie vorher: True“): die per PostMessage gesendeten
  Tasten nimmt jasp.exe ohne Fokus nicht an - weder Shift+Esc (gibt es erst in OpenJK) noch die
  Konsolentaste über den Scancode 0x29 (kein screenshot, kein ui_load im qconsole.log). Die Engine selbst
  liest lose Dateien mit ui_load neu: über die Befehlszeile (+wait … +screenshot_tga … +ui_load …,
  SP\m7\uiload_befehl.py) erscheint der während des Laufs eingefügte Text (SP\m7\uiload_jasp_neben.png).
  Fokus nehmen oder fremde Fenster steuern darf der Editor nicht - bei den Originalen bleibt es bei der pk3
  (Neustart über „Im Spiel testen“); offen, falls ein Weg ohne Fokus gefunden wird.

### Punkt 3 - Komponenten

- komponenten.cpp: Original = itemDef mit Zeile „// jkstudio:komponente <name>“ davor, Kopie mit
  „// jkstudio:instanz <name>“; Abgleich nach jeder Änderung (mitKopienAbgleich vor dem Anwenden: neuer
  Text, abgeglichene Kopien, alles als eine Ersetzung = ein Rückgängig-Schritt). Kopie = Text des Originals
  mit eigenem Namen und eigener Lage (x, y; Breite/Höhe vom Original), über setzeRect/setzeEigenschaft.
- Eigenschaften-Abschnitt Komponente: Als Komponente anlegen (Name), Verknüpfte Kopie einfügen (+20/+20,
  Name <orig>_<n>), bei Kopien Hinweis und Original wählen. ctest menue_komponenten; Selbsttest per Klick
  (Original anlegen, Kopie, Text am Original -> Kopie folgt, ein Rückgängig nimmt beides zurück).

### Punkt 12 - Modelle per Gizmo

- Item_Model_Paint (OpenJK code/ui) nimmt die Lage aus model_g2mins/maxs (origin[0] = halbe z-Spanne /
  0,268, origin[2] = -Mitte z, origin[1] = Mitte y; Vorgabe -16 -16 -24 / 16 16 32), nicht aus
  model_origin. Griffe am gewählten Modell: Drehen (model_angle, 0,5° je Bildpunkt), Abstand (z-Spanne
  um die Mitte gestreckt), Höhe (z-Spanne verschoben), Blickwinkel (model_fovx, fovy anteilig); Vorschau
  beim Ziehen über eine Kopie der Datei, beim Loslassen eine Ersetzung. Selbsttest mit echten Mauszügen
  (model_angle 30, g2mins/maxs geschrieben). Jedi Outcast: Blickwinkel fest 45 und Drehung nach Zeit
  (JK2_MODE) - dort wirken nur Abstand und Höhe.

### Punkt 14 - Datapad von Jedi Outcast und Movie Duels

- Quellen: OpenJK codeJK2/cgame (SP\m3\ojq) und Movie Duels code/cgame (SP\mdgit). Beide zeichnen das Datapad
  nach Jedi-Outcast-Art: CG_DrawDataPadHUD mit Ringen statt hud.menu (hud.cpp zeichneDatapadHudJk2:
  CG_DrawHUDLeftFrame1(+2 MD), CG_DrawArmor (MD CG_DrawDataPadArmor), CG_DrawHealth mit zeroFill,
  CG_DrawHUDRightFrame1(+2 MD), CG_DrawForcePower/CG_DrawAmmo mit forceTicPos/ammoTicPos (rechte Hälfte mit
  negativer Breite = gespiegelt), Säbelstil 80x40 (JK2 drei, MD sieben Bilder gfx/hud/DataPad*), MessageLit,
  gfx/hud/datapad2). CG_DrawDataPadIconBackground (Band oben gespiegelt, Zinken) vor Waffen, Inventar, Kräften.
- Jedi Outcast: MissionInformation_Draw/MissionPrint_Line (Titel INGAME_OBJECTIVES bei 96, Kreise 88, Text
  108 in CT_BLUE3, Sicherheitscodes von kejim_post/doom_comm), Waffen 40/80 mit Pad 8, Seiten mit Deckung
  0,6, Text INGAME_*_DESC in ocr_a 0,7 (Farbe .875 .718 .121); Kräfte 30/60 (11 Kräfte, force_icon_files
  gfx/hud/f_icon_*), Text CT_ICON_BLUE; Inventar leer INGAME_EMPTY_INV; eigene Waffennamen in weapons.dat
  (WP_BRYAR_PISTOL, WP_STUN_BATON).
- Movie Duels: Ziele wie JA, aber Text CT_BLUE3, „keine“ CT_LTBLUE1, dazu die Codes der JK2-Ziele; Waffen und
  Kräfte mit einer Nachbarin je Seite, 22/45 groß, Pad 32/64, 25 Kräfte (showDataPadPowers hell/Kern/dunkel),
  Symbol nur wenn der Shader da ist, Texte SP_INGAME_* sonst SPMOD_INGAME_*, CG_DisplayBoxedText mit dem
  Schriftgriff 4 (die Movie-Duels-Griffe der Vorschau). KOTOR-Waffenwahl (cg_com_kotor/charKOTORWeapons)
  nicht nachgebildet - beides ist im Grundzustand 0.
- Beispielwerte je Spiel (datapadBeispiel: JK2 kejim_post-Ziele, Kraftwahl Sprung), Auswahl in den
  Eigenschaften mit den Waffen- und Kraftlisten des jeweiligen Spiels; Breiten mit Jk2Schrift.
- Tests: ctest menue_ownerdraw (Lagen, Farben, Texte, SPMOD-Rückfall, fehlendes Symbol, HUD-Ringe/Striche);
  Selbsttest m7 öffnet die vier Datapad-Seiten mit den Daten des Spiels: Platzhalter MD 22 -> 17 (nur noch
  MD-MP 340-350), JK2 0, JA 0. Im Spiel: Movie Duels mit `+map duel_tradefed +datapad` und `+uimenu
  datapadWeaponsMenu` fotografiert (SP\m7\datapad_md.py, SP\m7\spiel_md_dp.png) - Band, Mitte bei 320, Nachbar bei
  406, Text und Ringe stehen wie in der Vorschau (Werte vom Spieler der Karte: Säbel und Fäuste, keine
  Macht, also keine Macht-Striche). Jedi Outcast nur gegen die Quellen: versucht mit `+devmap kejim_post
  +datapad` (SP\m7\datapad_jk2.py) - die Karte beginnt mit der Zwischensequenz, in der das Datapad nicht
  aufgeht, und überspringen ginge nur mit einer Taste.

### M6-Vorlagen im laufenden Spiel

- Die vier Seitenvorlagen (bhed::menue::seitenVorlage, SP-Fassung; SP\m7\vorlagen\gen.cpp schreibt genau den
  Text, den der Editor einfügt, mit eindeutigen Namen jks_haupt/_einst/_credits/_lade) lose im eigenen Ordner:
  ui/jks_*.menu und ui/menus.txt (wirksame Liste des Spiels + loadMenu), je Vorlage ein Start mit
  `+uimenu <name>`, Foto per screenshot_tga (SP\m7\vorlagen_spiel.py; Original-JKA über orig_kopie.py,
  „GameData wie vorher: True“; Jedi Outcast aus „Jedi Outcast\GameData“ mit kurzem fs_basepath, Ordnerzeiten
  dort unverändert). Editorfotos derselben Dateien im Bedienmodus (BHED_MENUE_VERGLEICH, „ui/x.menu|<datei>“,
  SP\m7\vorlagen_editor.sh), mittlere Abweichung im 8x6-Raster (Grauwerte 0-255):
  - Original-JKA (jasp.exe): Haupt 0,5, Einstellungen 1,0 -> 0,8, Credits 1,3, Laden 0,6.
  - Jedi Outcast (jk2sp.exe): 0,6 / 0,9 / 1,3 / 0,6.
  - Movie Duels SP: 2,7 / 3,0 -> 2,9 / 3,6 / 2,5 - der Rest ist überall gleich: MD gibt dunkle Farben 2-3
    Stufen dunkler aus (Hintergrund 0,02 0,03 0,08: Editor 5 8 20, MD 3 5 18, JKA/JK2 5 7 20) - Renderer,
    nicht Menücode.
  - Mehrspieler (MP-Fassung der Vorlagen als Hauptmenü "main" mit dem assetGlobalDef des Originals in
    ui/<jamp|jk2mp>/main.menu, SP\m7\vorlagen_mp.py): Original-JA-MP (jamp.exe) 0,6 / 0,8 / 1,2 / 0,5,
    Jedi Outcast MP (jk2mp.exe) 0,6 / 1,0 / 1,3 / 0,6.
- Fund dabei: Schieber je Fassung verschieden (Item_Slider_Paint, code/ui gegen codemp/ui): JA/JK2 SP
  SLIDER_WIDTH 128, Leiste und Daumen bei y + 2, Daumen 16 hoch; Movie Duels SP 96 breit, sonst wie SP; MP
  96, Leiste bei y, Daumen y - 2 und 20 hoch. Der Editor zeichnete überall die MP-Maße (Original-JKA zeigte
  die Leiste 128 breit und 2 tiefer). Jetzt sliderMasse(Dialekt, Spiel), auch für den Klick im Bedienmodus
  (Item_Slider_HandleKey). ctest menue_vorschau: rot (JA SP, MD SP), grün; die tote Funktion sliderDaumen
  (seit M1 ohne Aufrufer) entfernt.

### Zusatzpunkte (Koordinator)

- Einstellungs-Assistent (aus Übersetzung 2): feste Spalten (SameLine fs*12 / fs*26) - lange MD-Schlüssel und
  Texte ragten in die nächste Spalte. Selbsttest m6 zählt Zeilen, deren sichtbarer Text über den Anfang der
  nächsten Spalte reicht (FlussInfo::assistentUeberlappung): rot 14 Zeilen (MD-Daten, Englisch). Jetzt eine
  Tabelle (Einstellung | Steuerung | Text, Spalten so breit wie der längste Eintrag, ziehbar, beschnitten,
  abgeschnittener Text als Tooltip, Fenster 44 statt 36 Zeichen breit): grün 0 in en, de, zh-Hans, ja.
  Handbuchbilder menue_assistent in vier Sprachen neu, Satz zu den Spalten im Handbuch. MeKompOriginal ohne
  Mehrzahl formuliert (trN gibt es in diesem Zweig noch nicht).
- Jedi-Outcast-Farbcodes (aus Integration 5): Testmenüs SP\m7\caret\jks_caret(2).menu in jk2sp.exe und
  jk2mp.exe fotografiert (SP\m7\caret_spiel.py). Ergebnis im Spiel - anders als OpenJK JK2_MODE:
  - Zeichnen (beide Programme): jedes Zeichen nach ^ ist eine Farbe, ColorIndex(c) = (c - '0') & 7 -
    „A^B^C^D^E“ zeigt nur A, „A^Bx^Cy^0z“ zeigt A, x grün, y gelb, z schwarz. OpenJK JK2_MODE (und karte1
    3cae6104) lässt nur das ^ weg und zeichnet das B.
  - Breite: jk2sp.exe zählt jedes Zeichen, auch ^ und Ziffer (wie OpenJK JK2_MODE); jk2mp.exe zählt von
    ^ und Ziffer die Ziffer (gemessen mit anewhope, ^ 15 / 1 7), ^ mit anderem Zeichen zählt beide.
  - Umsetzung mit derselben Funktion wie karte1: schriftZeichnen (aus dem Maler gezogen, Text wie
    karte1 3fd09a50/3cae6104) mit jk2Modus, aber mit dem Verhalten aus den Fotos; schriftBreite mit
    Jk2Schrift (Nein/Sp/Mp), Umgebung::jk2 (GUI: Spiel JK2 -> Sp bzw. Mp). Alle Breiten der Vorschau
    (Text_Width, Textscroll-Umbruch, Ladebild, Ownerdraw-Kontext) nehmen es mit.
  - Rot/grün: ctest menue_vorschau rot (4 Prüfungen), grün; Editor gegen Spiel (SP\m7\caret_editor.sh):
    vorher 4,3 / 4,5 (SP) und 3,8 (MP), nachher 0,9 / 0,8 / 0,8 / 0,7.
  - Für die Integration: karte1 untertitel::breite rechnet JK2 eigens (jedes Zeichen) - das ist
    schriftBreite(..., Jk2Schrift::Sp); und die Untertitel-Prüfung von 3cae6104 erwartet das B nach „^“ -
    jk2sp.exe nimmt es als Farbe (CG_DrawCaptionText geht über dieselbe RE_Font_DrawString).
- Portierungs-Lint: <cstddef> in den M6-Tests menuefluss/menuefokus/menuelesbar, dazu in den M7-Tests
  menueladebild/menueownerdraw und <algorithm>/<cstddef> in menuevorschau.

### Schlussrunde M7

- Sauberer Neubau (`--clean-first`, alle Ziele): 171 Projekte, 0 Warnungen, 0 Fehler (/W4 /WX).
- lint_sources, lint_fassung: ok.
- ctest alle: 168/168 grün (zuerst karte_lizenz rot: Köpfe der M7-Dateien ohne „Portiert aus OpenJK“ bzw. Komponenten ohne „Eigener Code“ am Anfang - behoben) (neu menue_ladebild, menue_tasten, menue_komponenten; erweitert menue_ownerdraw,
  menue_vorschau, menue_paket).
- Selbsttest menueeditor voll: Movie-Duels-Daten 131 OK, 0 FEHLER (vorher 127/5: der Abschnitt Komponente schob „Spacer einfügen“ aus dem Bild, und der Komponentenname blieb von einer anderen Datei stehen - beides behoben), Daten aus „GameData original“ 131 OK, 0 FEHLER.
- karteneditor kurz (BHED_KARTE_NUR=fotos): 78 OK, 0 FEHLER.
- Modus 1 komplett: 2645 OK, 0 FEHLER.
- Gegen das Spiel geprüft (Steam angemeldet, SP\spiel.lock, Original-Dateien nur kopiert und wieder entfernt,
  „GameData wie vorher: True“; Jedi-Outcast- und Movie-Duels-Ordner ohne Änderung): M6-Vorlagen in Movie Duels,
  jasp/jamp, jk2sp/jk2mp (Abschnitt oben), ui_load in Movie Duels und jasp (Befehlszeile), Farbcodes in
  jk2sp/jk2mp, Datapad in Movie Duels. Danach keine eigenen Prozesse offen.

### Stand M7 - fertig / offen / nächster Schritt

- Fertig: 14 (alle ownerdraws/Feeder der Original-Menüs mit Darstellung; JKA 0, JK2 0, MD nur MD-MP
  340-350), M5b-Reste (Tasten, MP-Startmenü, JK2-Tasten und Renderer-Cvars, Chromglanz), 11, 13, 3, 12,
  Selbsttest mit JKA-Daten, Vorlagen im Spiel; Zusatzpunkte Assistent, JK2-Farbcodes, <cstddef>.
- Offen mit Begründung: MD-MP 340-350 (keine Quelle der MD-MP-Oberfläche); ui_load bei den Original-
  Programmen auslösen (sie nehmen Tasten ohne Fokus nicht an - Engine selbst lädt neu, belegt); Datapad von
  Jedi Outcast nicht im Spiel fotografiert (Zwischensequenz am Kartenstart); KOTOR-Waffenwahl von Movie
  Duels (cg_com_kotor) nicht nachgebildet.
- Für die Integration: karte1 untertitel::breite und die JK2-Untertitelprüfung aus 3cae6104 (siehe
  Zusatzpunkte) - jk2sp.exe nimmt das Zeichen nach ^ als Farbe.
- Nächster Schritt: nach Auftrag (M7 ist vollständig bis auf die begründeten Punkte).

## Changelog

## English

- Menu editor: menu flow - see which menu opens which, which menus nobody reaches, dead ends and menus without a way back; click to jump there.
- Menu editor: tab order - numbers show the keyboard order as the game uses it; click the elements in a new order to rearrange it.
- Menu editor: styles - all colours, fonts, text sizes and backgrounds of a file at a glance, changed everywhere at once.
- Menu editor: templates for whole menus (main menu, settings page with tabs, credits, loading screen) and a settings wizard that knows the game's own sliders, yes/no switches and choices.
- Menu editor: reference image under the canvas, compare an element with the game's original and reset it, live limits (elements, menus, script length) while building.
- Menu editor: readability hints - text too small, too little contrast, translations that do not fit their field.
- Menu editor: closing a tab with unsaved changes asks first.
- Menu editor: elements drawn by the game's code are now shown as in the game instead of placeholders - datapad (Jedi Academy, Jedi Outcast, Movie Duels), multiplayer Force ranks, server and bot lists, graphics driver page, Movie Duels Pazaak and duel maps; sample values are labelled and can be changed.
- Menu editor: key binding elements show the keys from the game's default config instead of ???.
- Menu editor: loading and connect screens with sample level shot, weapons, Force powers and loading bar.
- Menu editor: Test in game reloads your menus in the running game after every save (OpenJK and Movie Duels).
- Menu editor: components - linked copies of an element that follow their original.
- Menu editor: handles to turn, move and zoom models; new projects can start in multiplayer format.
- Menu editor: sliders with the size of each game, Jedi Outcast colour codes as in the game, settings wizard as a clear table.

## Deutsch

- Menü-Editor: Menüfluss - sehen, welches Menü welches öffnet, welche Menüs man nie erreicht, Sackgassen und Menüs ohne Weg zurück; Klick springt dorthin.
- Menü-Editor: Tab-Reihenfolge - Nummern zeigen die Tastatur-Reihenfolge wie im Spiel; Elemente in neuer Reihenfolge anklicken stellt sie um.
- Menü-Editor: Stile - alle Farben, Schriften, Textgrößen und Hintergründe einer Datei auf einen Blick, überall auf einmal geändert.
- Menü-Editor: Vorlagen ganzer Menüs (Hauptmenü, Einstellungsseite mit Reitern, Credits, Ladebildschirm) und ein Einstellungs-Assistent, der die Schieber, Ja/Nein-Schalter und Auswahlen des Spiels kennt.
- Menü-Editor: Referenzbild unter der Fläche, Element mit dem Original vergleichen und zurücksetzen, Live-Grenzen (Elemente, Menüs, Skriptlänge) beim Bauen.
- Menü-Editor: Lesbarkeits-Hinweise - zu kleine Schrift, zu wenig Kontrast, Übersetzungen, die nicht ins Feld passen.
- Menü-Editor: Reiter mit ungespeicherten Änderungen fragt beim Schließen nach.
- Menü-Editor: Elemente, die der Code des Spiels zeichnet, erscheinen jetzt wie im Spiel statt als Platzhalter - Datapad (Jedi Academy, Jedi Outcast, Movie Duels), Machtränge im Mehrspieler, Server- und Botlisten, Grafiktreiber-Seite, Pazaak und Duellkarten von Movie Duels; Beispielwerte sind beschriftet und änderbar.
- Menü-Editor: Tastenbelegungs-Elemente zeigen die Tasten aus der Vorgabe-Config des Spiels statt ???.
- Menü-Editor: Lade- und Verbindungsbild mit Beispiel-Levelshot, Waffen, Machtkräften und Ladebalken.
- Menü-Editor: Im Spiel testen lädt deine Menüs nach jedem Speichern im laufenden Spiel neu (OpenJK und Movie Duels).
- Menü-Editor: Komponenten - verknüpfte Kopien eines Elements, die ihrem Original folgen.
- Menü-Editor: Griffe zum Drehen, Verschieben und Zoomen von Modellen; neue Projekte können im Mehrspieler-Format beginnen.
- Menü-Editor: Schieber in der Größe des jeweiligen Spiels, Farbcodes von Jedi Outcast wie im Spiel, Einstellungs-Assistent als übersichtliche Tabelle.

## 中文

- 菜单编辑器:菜单流程——查看哪个菜单打开哪个菜单、哪些菜单无法到达、死胡同以及无法返回的菜单;单击即可跳转。
- 菜单编辑器:Tab 顺序——编号显示游戏中的键盘顺序;按新顺序单击元素即可重新排列。
- 菜单编辑器:样式——一览文件中的所有颜色、字体、文字大小和背景,一次性全部修改。
- 菜单编辑器:整个菜单的模板(主菜单、带标签页的设置页、制作人员名单、加载画面)以及了解游戏自身滑块、是/否开关和选项的设置向导。
- 菜单编辑器:画布下的参考图、元素与游戏原版比较并重置、搭建时的实时上限(元素、菜单、脚本长度)。
- 菜单编辑器:可读性提示——文字太小、对比度不足、译文放不进字段。
- 菜单编辑器:关闭含未保存更改的标签页时会先询问。
- 菜单编辑器:由游戏代码绘制的元素现在与游戏中一致,不再是占位框——数据板(Jedi Academy、Jedi Outcast、Movie Duels)、多人游戏原力等级、服务器和机器人列表、显卡驱动页面、Movie Duels 的 Pazaak 和决斗地图;示例值有标注且可修改。
- 菜单编辑器:按键绑定元素显示游戏默认配置中的按键,而不是 ???。
- 菜单编辑器:加载画面和连接画面带示例关卡截图、武器、原力和加载条。
- 菜单编辑器:游戏内测试会在每次保存后在运行中的游戏里重新加载菜单(OpenJK 和 Movie Duels)。
- 菜单编辑器:组件——跟随原件的元素链接副本。
- 菜单编辑器:用于旋转、移动和缩放模型的手柄;新项目可以以多人游戏格式开始。
- 菜单编辑器:滑块采用各游戏的尺寸,Jedi Outcast 的颜色代码与游戏一致,设置向导改为清晰的表格。

## 日本語

- メニューエディター: メニューの流れ - どのメニューがどれを開くか、たどり着けないメニュー、行き止まり、戻れないメニューを表示し、クリックで移動。
- メニューエディター: Tab の順序 - ゲームと同じキーボード順を番号で表示し、新しい順にクリックすると並べ替え。
- メニューエディター: スタイル - ファイルのすべての色、フォント、文字サイズ、背景を一覧し、一度に全部変更。
- メニューエディター: メニュー全体のテンプレート(メインメニュー、タブ付き設定ページ、クレジット、ロード画面)と、ゲーム本来のスライダー、はい/いいえ、選択肢を知る設定ウィザード。
- メニューエディター: キャンバスの下の参考画像、要素をゲームのオリジナルと比較して戻す機能、作成中の上限表示(要素、メニュー、スクリプトの長さ)。
- メニューエディター: 読みやすさのヒント - 小さすぎる文字、足りないコントラスト、フィールドに収まらない訳文。
- メニューエディター: 保存していない変更のあるタブを閉じるときに確認。
- メニューエディター: ゲームのコードが描く要素を、プレースホルダーではなくゲームと同じように表示 - データパッド(Jedi Academy、Jedi Outcast、Movie Duels)、マルチプレイのフォースランク、サーバーとボットの一覧、グラフィックドライバーのページ、Movie Duels の Pazaak と決闘マップ。サンプル値はラベル付きで変更可能。
- メニューエディター: キー割り当て要素は ??? ではなく、ゲームの既定の設定にあるキーを表示。
- メニューエディター: ロード画面と接続画面に、サンプルのレベル画像、武器、フォース、ロードバー。
- メニューエディター: ゲームでテストすると、保存のたびに実行中のゲームでメニューを読み直し(OpenJK と Movie Duels)。
- メニューエディター: コンポーネント - 原本に追従する要素のリンクコピー。
- メニューエディター: モデルを回転・移動・ズームするハンドル。新しいプロジェクトはマルチプレイ形式でも開始可能。
- メニューエディター: スライダーは各ゲームの大きさ、Jedi Outcast の色コードはゲームどおり、設定ウィザードは見やすい表に。
