# Notizen Menü-Editor, Stufe M5 (Zweig menue5 ab menue3)

Auftrag: SP\auftrag_m5.md mit SP\auftrag_gemeinsam.md. Vorwissen NOTIZEN_menue1/2/3.md (M4 steht in
NOTIZEN_menue3.md).

## Punkt 0 - Original-JKA (Steam-Fassung) gegen den Editor

### Spielstarts

- Steam-Ausnahme (shank, 29.10., siehe auftrag_gemeinsam.md und Gedächtnis „kein-steam-start“): vor
  jedem Start geprüft, dass steam.exe läuft und `HKCU\Software\Valve\Steam\ActiveProcess\ActiveUser`
  != 0 ist (SP\m5\orig.py bricht sonst ab). Gestartet per Python Popen (Idle, `SW_SHOWNOACTIVATE`),
  Fenster 960x720, `s_initsound 0`, `in_mouse 0`, `in_joystick 0`, `SteamAppId/SteamGameId=6020`
  in der Umgebung des eigenen Prozesses (wie Steam es seinen Spielen setzt). Steam hat nie neu
  gestartet und keinen Dialog gezeigt; nach jedem Lauf keine eigenen jasp/jamp-Prozesse mehr.
- Ordner (shank: „der normale game data folder ist leer“): Steam startet aus `Jedi Academy\GameData`.
  Vorher enthielt GameData nur `base\saves` (Steam-Cloud: auto.sav, auto_yavin1.sav,
  auto_yavin1b.sav, steam_autocloud.vdf). Für die Läufe kopiert: aus „GameData original“ EaxMan.dll,
  IFC22.dll, OpenAL32.dll, jagamex86.dll, jamp.exe, jasp.exe, version.inf und base\assets0-3.pk3.
  Nicht kopiert (nicht original bzw. Anwenderdaten): base\jaconfig.cfg, base\jampconfig.cfg
  (Einstellungen von shank, 2024), base\saves (Spielstände), base\shaders\shaderlist.txt (21.10.2026,
  nicht von JKA), leerer Ordner base\maps.
- Geschrieben hat das Spiel nur in eigene Ordner im Scratchpad: `+set fs_basepath <SP\m5\home_*>`
  `+set fs_cdpath <GameData>` (jamp zusätzlich fs_homepath; jamp lädt uix86.dll nur aus
  fs_basepath/fs_cdpath - mit fs_homepath allein blieb es mit „Failed to load dll“ in einer
  Fehlermeldung stehen, eigene PID sofort beendet). Fotos: PrintWindow liefert beim alten
  GL-Fenster nur Schwarz; deshalb `screenshot_tga` des Spiels (landet im eigenen fs_basepath).
- Danach alle kopierten Dateien wieder entfernt (vorher mit dem Original verglichen), Ordnerzeiten
  zurückgesetzt: `find` über GameData und „GameData original“ (Typ, Größe, Zeit, Pfad) gleich der
  Liste vor dem ersten Start, Hashes der Spielstände und der Configs in „GameData original“ gleich
  (SP\m5\spielordner_vorher.txt / _nachher.txt, hash_vorher.txt).

### Rangliste (Editor gegen Original)

Werkzeuge SP\m5: orig_rang.sh (Spielfotos, SP `+uimenu <menü>`, MP Testpaket mit
`close all ; open <menü>` am Ende von onOpen des MP-Hauptmenüs, Foto ~213 Bilder nach dem Öffnen),
rang_editor.sh <lauf> (Editorfotos mit den Daten aus „GameData original“, Bildpaare
SP\m3\vergleich\neben_orig_{sp,mp}_*.png, Ergebnis rang_ergebnis_{sp,mp}_<lauf>.txt). Mittlere
Abweichung wie in M4 (Grauwerte 0-255, 8x6-Raster).

Wichtig für die Methode: In einem ersten Durchgang behielt das Spiel seine Config zwischen den
Menüs (fs_basepath je Spiel ein Ordner) - archivierte Cvars, die ein Menü beim Öffnen setzt, galten
dann für die folgenden. Zweiter Durchgang: Config vor jedem Start gelöscht (orig_rang.sh); alle
Zahlen unten aus diesem Durchgang.

Ergebnis (Mittel der Menüs; v1 = Stand menue3, v2 = nach den Korrekturen, v3 = mit Ersatzkasten):
- SP (11 Menüs): 11,74 -> 11,67 -> 11,64. Oben bleiben videodriverMenu 30,9 (UI_GLINFO zeigt die
  GL-Treiberdaten des laufenden Spiels), error_popmenu 24,1 und videowarningMenu 19,5 (das Spiel
  zeigt das Hauptmenü dahinter, weil `uimenu` es nicht schließt - der Editor zeigt die Datei allein),
  characterMenu 10,9 -> 10,5 (Farben/Rumpf jetzt von jedi_tf; Rest: Drehphase der Figur).
- MP (28 Menüs): 8,60 -> 7,05 -> 7,04. playerMenu 19,6 -> 5,9, createserver 20,8 -> 8,6,
  playerMenu2 22,1 -> 13,0 (Rest: Drehphase des Kopfmodells), quickbots 13,3 -> 5,5, quickgame
  12,5 -> 10,6. Die übrigen ändern sich um höchstens 0,3 (Zeitpunkt der Fotos).
- quickgame2 14,4 -> 15,9: das Original zeigt im Kartenbild (UI_STARTMAPCINEMATIC) ein bewegtes
  Planetenbild statt des Levelshots von mp/ctf1. Direkt geöffnet (sonst kommt man über quickgame
  dorthin) ist `mapList[0].cinematic` noch -1, `CIN_PlayCinematic("mp/ctf1.roq")` - die Datei gibt
  es nicht. OpenJK gibt dann -1 zurück und zeichnet den Levelshot (so der Editor); das Original
  1.01 zeigt offenbar das laufende Video des Hauptmenüs (Griff wiederverwendet). Nicht übernommen:
  Movie Duels und OpenJK verhalten sich wie der Editor.
- twj (Movie Duels): fehlende Schlüssel `@MD_MENU_MAIN_TWJ`/`@MD_BRIEFINGS_TCW_TWJ_DESC`. Belege:
  SE_GetString (OpenJK code/qcommon/stringed_ingame.cpp, Movie Duels gleich) gibt bei unbekanntem
  Schlüssel `""` zurück, JK2SP_GetStringTextString (strip.cpp) ebenso, die MP-UI über
  CL_SE_GetStringTextString (cl_uiapi.cpp) auch `""`; Item_Text_Paint kehrt bei `'\0'` zurück. Das
  Spiel zeigt also nichts - so jetzt die Vorschau (Texte::imSpiel); die Eigenschaften nennen den
  fehlenden Schlüssel mit Lösung. Der größere Teil der Abweichung (M4: 79) war der Hintergrund:
  das Bild fehlt, RE_RegisterShaderNoMip gibt Griff 0 zurück, WINDOW_STYLE_SHADER zeichnet ohne
  Prüfung tr.shaders[0] = den "<default>"-Kasten (R_CreateDefaultImage: 16x16 Grau 32, weißer
  Rand, gestreckt), WINDOW_STYLE_FILLED prüft `if (w->background)` und füllt. Jetzt ebenso.
- challengecharacterMenu (Movie Duels): kein fehlendes Bild. Das Anakin-Bild im Spielfoto ist der
  Hover-Zustand: onOpen schiebt charchoose_light nach x 641 und blendet es aus; das mouseEnter des
  Rebellen-Knopfes (rect 201 173 110 135) setzt es auf 0 0 640 480 und blendet es ein - der
  Spielzeiger stand im Foto über dem Knopf (bei etwa 245, 295). Im Editor ohne Maus darüber
  richtig ohne Bild.

Behoben (rot = Bildpaar bzw. Test vorher, grün = derselbe Test danach):
- Spezies der Figurenlisten: Farben/Köpfe/Rumpf/Beine kamen immer von der ersten Spezies, jetzt aus
  `ui_char_model` wie UI_GetCharacterCvars (SP letzter Treffer, MP erster). ctest menue_vorschau.
- MP-Start-Cvars: die Vorschau kannte für MP keine Vorgaben (Name leer, `models/players//model.glm`).
  Jetzt aus SV_Init/SV_BotInitCvars, CL_Init und ui_xcvar.h (tools/gen_menue_cvarvorgaben.py,
  259 Cvars; Movie Duels hat keine MP-Quellen - MD-MP nimmt dieselben); MP-getcharcvars wie codemp
  UI_GetCharacterCvars (char_color_*, `model` mit `|`). ctest menue_skript.
- MP-Kopfliste: UI_BuildQ3Model_List 1:1 (.skin-Dateien, Name `<ordner>/<rest>`, ohne Doppelte) und
  UI_SelectedTeamHead (nur `/default` bei uiSkinColor TEAM_FREE); MP-Bildlisten breiter als zwei
  Elemente in Zeilen und Spalten (Item_ListBox_Paint). ctest menue_vorschau.
- MP-Ownerdraws: UI_SKIN_COLOR, UI_GAMETYPE/UI_NETGAMETYPE (gameinfo.txt), UI_SKILL, UI_BLUETEAM1-8/
  UI_REDTEAM1-8 (sv_maxClients, Bots aus botfiles/bots.txt + scripts/*.bot), UI_MAPCINEMATIC/
  UI_STARTMAPCINEMATIC (Levelshot, sonst unknownmap_mp); Lage wie Item_OwnerDraw_Paint
  (Text + 8). Noch Platzhalter: UI_FORCE_MASTERY_SET und die übrigen Macht-Ownerdraws
  (uiForceRank hängt an ui_force.c), UI_GLINFO, Datapad, Chat, Pazaak.

## Punkt 1 - Layouts wie Qt

- Modul src/menue/layout.cpp (ohne Oberfläche): Waagrecht (hbox), Senkrecht (vbox), Raster
  (Spalten), Teiler (zwei Spalten, Anteil + Teilerbreite), Abstand, Ränder, Ausrichtung je Achse
  (Anfang/Mitte/Ende/Füllen), Spacer mit Dehnung, verschachtelte Layouts (`@name`), Fläche aus
  einem Element (frame) oder festem rect; Wunschbreite = max(rect, breitester Text aller Sprachen),
  zu eng -> anteilig kleiner + Warnung; hidden=skip: nach onOpen versteckte Kinder nehmen keinen
  Platz (Sichtbarkeit aus einer eigenen Laufzeit, die das Menü öffnet).
- Speicherung: eine Zeile `// jkstudio:layout name=… type=… frame=… spacing=… margins=… halign=…
  valign=… columns=… ratio=… handle=… hidden=skip items=a,*,b,@innen` vor dem ersten itemDef
  (Einrückung und Zeilenende der Datei). Die Engine überliest //-Kommentare; ohne Layout bleibt die
  Datei byte-gleich (ctest menue_layout inkl. Rundlauf, Einfügen/Ersetzen/Löschen).
- Oberfläche (gui/app_menue_layout.inc): Abschnitt „Layout“ in den Eigenschaften - leer: Hinweis und
  „Zeig mir wie“; ab zwei gewählten Elementen vier Knöpfe (Strg+Umschalt+1..4); ein umschließendes
  gewähltes Element wird zur Fläche (Rand 8). Im Layout: Art, Abstand, Ränder, Ausrichtung, Spalten,
  Teiler, versteckte, Kinderliste (Reihenfolge, entfernen), Spacer, Neu anordnen, Auflösen, Warnungen
  in normaler Sprache. Bewegt man die Fläche, rechnet das Layout die Kinder mit (eine Ersetzung).
- Selbsttest BHED_MENUE_NUR=m5, Aufgabe „Panel + drei Knöpfe nebeneinander“: 5 Schritte (4 Klicks
  Auswahl, 1 Knopf), ohne Text-Reiter, rects 108/176/244; Spacer schiebt den dritten an den Rand;
  Fläche verschieben; ein Rückgängig-Schritt; Auflösen; Rückgängig bis byte-gleich.
- Offen: „show/hide je Zustand vorrechnen“ ist nur als hidden=skip für den Grundzustand umgesetzt;
  setitemrect-Befehle in die show/hide-Skripte zu schreiben (Layout je Zustand) fehlt noch.

## Punkt 2 - Ereignisse wie Unreal UMG

- Modul src/menue/ereignisse.cpp: Ereignisse je Dialekt aus den itemDef-Schlüsseln (mouseEnter,
  mouseExit, action, onFocus, leaveFocus, accept), Zerlegen wie Item_RunScript (mit und ohne `;`,
  Wortzahlen der Script_*-Handler, uiScript mit den Wörtern des Skripts), Zusammensetzen, Skript
  eines Ereignisses setzen/anlegen/entfernen, Bausteinliste aus den commandLists der beiden
  ui_shared-Fassungen (MP: setcvartocvar, SP: delay ...), Hover-Effekte Leuchten (Glow-Element
  gfx/menus/menu_buttonback wie die Original-Menüs), Farbe, Rahmen, Größer (transition).
  Neue Schlüssel eines Effekts kommen gesammelt in einer Einfügung vor die } (mehrere Einfügungen an
  derselben Stelle wären nicht eindeutig geordnet).
- Oberfläche (gui/app_menue_ereignisse.inc): Abschnitt „Ereignisse“: Hover-Effekte mit Farbe,
  Hover-Klang (focusSound) und Klick-Klang (play am Anfang von action) mit Probehören über den
  Ton-Schalter, je Ereignis die Bausteine (Elementnamen als Liste, Farben mit Farbwähler, play mit
  Probehören, Reihenfolge, entfernen, „+ Aktion“ nur mit Befehlen der Engine), leerer Zustand mit
  „Zeig mir wie“ (setzt den Farbeffekt). Jede Änderung über den Quelltext: Text-Reiter synchron,
  ein Rückgängig-Schritt.
- ctest menue_ereignisse; Selbsttest-Aufgabe „knopf1 leuchtet beim Hover und klickt“: 5 Schritte
  ohne Text-Reiter, Ergebnis vom Parser ohne Meldung gelesen, eine Laufzeit zeigt das Glow-Element
  erst beim Hover.
- Nebenbei gefunden: Laufzeit::anwenden überträgt keine Flags - die Sichtbarkeit nach onOpen steht in
  Laufzeit::items (grundSichtbar nutzte vorher die gelesenen Flags).

## Punkte 3 bis 5 und Tour - nicht begonnen (für M6)

Aus Zeit- bzw. Kontextgründen nicht angefangen; Stand und Vorarbeit:
- Punkt 3 (Projektordner + `*.jksmenu`, Autosave, pk3-Export): nichts begonnen. Vorhanden sind
  Paket (Dokument::bilder, Texte, Schriften), Export (exportierePk3/exportiereAenderungen) und
  Einstellungen; die Projektdatei müsste Zielspiel, Mod-Profil, Menüs, Layouts (stehen schon im
  .menu), Vorschau-Einstellungen, offene Reiter und Export-Einstellungen aufnehmen.
- Punkt 4 (Mod-Profile): nichts begonnen. Die Schlüsselwort-, Befehls- und Cvar-Tabellen kommen
  heute aus festen Tabellen bzw. Generatoren (tools/gen_menue_cvarvorgaben.py, parser.cpp,
  bausteinBefehle); ein Profil müsste diese je Mod tragen. Quellcode „Star Wars Galactic Legacy“
  nicht gesucht.
- Punkt 5 (JK2 echt): nicht gestartet. Hinweis des Hauptstrangs: Jedi Outcast liegt als
  Steam-Fassung unter `Jedi Outcast\GameData` (jk2sp.exe, jk2mp.exe; base: assets0/1/2/5.pk3,
  jk2config.cfg, jk2mpconfig.cfg(.bak), loser Ordner shaders und shaders.rar - vermutlich nicht
  original, nicht anfassen). Steam-Ausnahme gilt auch für JKO. Weg wie bei jasp in Punkt 0:
  `+set fs_basepath <eigener Ordner> +set fs_cdpath <GameData>` (dann schreibt das Spiel nichts in
  den Spielordner und das Testpaket liegt im eigenen Ordner), Configs vorher sichern, Ordner vorher/
  nachher vergleichen; SP\m5\orig.py ist dafür die Vorlage (Pfad und exe-Namen anpassen).
- Tour „Dein erstes eigenes Menü“ (Punkt 6): nicht gebaut; die Aufgabentests im Selbsttest (Layout,
  Ereignisse) sind die Schritte, die sie zeigen sollte.

## Punkt 6 - Handbuch und Selbsttest

- Handbuchseite „Menü-Editor: Layouts und Ereignisse“ (tools/handbuch/inhalt_menue5.py) in vier
  Sprachen mit den Bildern menue_layout und menue_ereignisse aus dem Selbsttest (BHED_SPRACHE
  en/de/zh-Hans/ja, SP\m5\hb_fotos.sh, tools/handbuch/bilder.py); gen_handbuch kennt die M5-Texte.
- Selbsttest `menueeditor` um M5 erweitert (Layout- und Ereignis-Aufgabe, BHED_MENUE_NUR=m5).
- Tour „Dein erstes eigenes Menü“: nicht gebaut (siehe oben).

## Schlussrunde

- Sauberer Neubau (`--clean-first`, alle Ziele): 0 Warnungen, 0 Fehler (/W4 /WX).
- ctest: im ersten Lauf 156/157 - karte_lizenz rot: layout.h nannte „Eigener Code“ erst nach den
  ersten 600 Zeichen des Kopfes; Kopf umgestellt, danach 157/157 grün.
- Selbsttest menueeditor voll: Movie-Duels-Daten 79 OK / 0 FEHLER, JKA-Daten („GameData original“,
  nur gelesen) 79 OK / 0 FEHLER.
- karteneditor kurz (BHED_KARTE_NUR=fotos): 78 OK, 0 FEHLER.
- Modus 1 komplett (in M4 nach der letzten Korrektur nicht wiederholt): 2645 OK, 0 FEHLER.
- lint_sources, lint_fassung: ok.
- Spielordner: `Jedi Academy\GameData` nach beiden Durchgängen wieder genau wie vorher (Liste mit
  Typ/Größe/Zeit gleich, Spielstände per Hash gleich), „GameData original“ unverändert (Hashes der
  Configs und Spielstände gleich). Movie Duels und Jedi Outcast in M5 nicht gestartet und nicht
  angefasst. Keine eigenen Spiel- oder jkstudio-Prozesse mehr.
- Gegen das Vorbild geprüft: alle 39 JKA-Menüs außerhalb des Spiels gegen die Original-exes
  (Bildpaare, Rangliste), twjMenu gegen Movie Duels (2,6); Layouts und Ereignisse gegen den Parser
  (liest ohne Meldung) und die Laufzeit (Hover zeigt das Glow-Element). Nicht gegen das Spiel
  geprüft: die Ereignis-Bausteine im laufenden Spiel (Mausbewegung erreicht die Spiele ohne Fokus
  nicht, siehe NOTIZEN_menue3), die Ownerdraws mit anderen als den Vorgabe-Cvars, ingame-Menüs.

## M5b (Rest von M5, Auftrag SP/auftrag_m5b.md)

### Punkt 1 - show/hide je Zustand (Rest Layouts)

- layoutZustaende (src/menue/layout.cpp): ein Zustand = Grundzustand nach onOpen + ein Skript des
  Menüs. Ob ein Skript Layout-Kinder umschaltet, entscheidet die Laufzeit (Item_RunScript, show/hide
  auf Name oder group): einmal von „alle Kinder sichtbar“, einmal von „alle versteckt“ ausgeführt.
  Jedes umschaltende Skript (auch onOpen, damit ein erneutes Öffnen die Lage zurücksetzt - das Spiel
  setzt rects beim Öffnen nicht neu, Menu_UpdatePosition läuft nur nach Menu_PostParse/Ziehen)
  bekommt am Ende `setitemrect <kind> x y w h` für jedes sichtbare Kind, das in irgendeinem Zustand
  anders liegt als im Grundzustand. Alte setitemrect auf Layout-Kinder in diesen Skripten werden
  ersetzt (Rundlauf: zweites Rechnen ändert nichts), beim Auflösen entfernt; der übrige Skripttext
  bleibt Zeichen für Zeichen (eine Ersetzung je Skriptkörper, mehrzeilig mit der Einrückung des
  letzten Befehls). Kinder, die das Skript nicht nennt, gelten als im Grundzustand.
- Script_SetItemRect: SP (code/ui, Movie Duels) setzt window.rect direkt, MP addiert
  menu->window.rect.x/y. Die Laufzeit addierte den Versatz auch in SP - nachgestellt (ctest
  menue_skript rot), behoben, grün. Geschrieben wird je Dialekt so, dass window.rect = rectClient +
  Menü (+ Rand, Item_SetScreenCoords) herauskommt.
- Dialekte: Jedi Outcast SP (jk2sp.exe 1.04 aus Steam, nur gelesen: Namen der commandList im
  Programm) hat kein setitemrect, setitemtext, setitembackground, transition2/3, delay,
  playVoice/stopVoice; dafür playlooped. In JA SP (code/ui) und Movie Duels ist playlooped
  auskommentiert - die Bausteinliste bot es an und die Laufzeit spielte die Musik (ctest
  menue_ereignisse/menue_skript rot, behoben, grün; unbekannte Befehle gehen wie im Spiel an
  DC->runScript). Ohne setitemrect: nichts schreiben, Warnung mit Lösung („Versteckte Elemente
  geben ihren Platz frei“ aus oder Elemente außerhalb des Layouts).
- Oberfläche: Zeile „N Skripte … rücken auch die anderen zurecht (setitemrect)“ mit Liste im
  Tooltip; Tooltip des Schalters erklärt es. ctest menue_layout (SP und MP: Befehle, Laufzeit legt
  nach Klick wie das Layout, Rundlauf, ohne setitemrect, Auflösen), Selbsttest BHED_MENUE_NUR=m5b
  (Aufgabe mit zeig/weg-Knopf, Laufzeit, zwei Rückgängig bis byte-gleich): 2 OK, 0 FEHLER.

### Punkt 3 - Projektordner und Projektdatei *.jksmenu

- Modul src/menue/projekt.cpp (ohne Oberfläche): Textformat „schlüssel wert“ (jksmenu 1, game,
  dialect, profile, menu, asset <menü> <datei>, open, active, preview.resolution/letterbox/language,
  cvar, export.pk3/changesOnly), Pfade nur innerhalb des Projektordners (.., absolute abgelehnt),
  Fehler als Art + Zeile (Oberfläche sagt es in vier Sprachen mit Lösung), Sicherungsliste,
  Plan der Dateizuordnung (HKCU\Software\Classes\.jksmenu -> JKStudio.MenuProject, open-Befehl
  mit "%1"). ctest menue_projekt.
- Oberfläche (gui/app_menue_projekt.inc): Leistenknopf „Projekt“ (Neu, Öffnen, Speichern, als pk3
  ausgeben mit „nur Änderungen“ und Ziel, Ordner zeigen, Dateizuordnung). Neues Projekt legt ui/,
  gfx/, video/, shaders/, strings/ an, die offenen Menüs und ihre Paketdateien kommen hinein; im
  Projekt speichert der Speichern-Knopf das ganze Projekt. Öffnen liest alle Menüs, alle übrigen
  Dateien des Ordners sind das Paket (beim Menü aus „asset“, sonst beim ersten), Reiter, Vorschau
  (Auflösung, Balken, Sprache, Cvars) wie gespeichert; anderes Zielspiel als die geladenen Daten
  wird mit Lösung gemeldet. Leerer Zustand mit „Neues Projekt“/„Projekt öffnen“.
- Sicherung alle 30 s: ungespeicherte Dokumente nach <Projekt>/.jksautosave bzw. ohne Projekt nach
  <Einstellungen>/menue_autosave; alles gespeichert -> Sicherung weg; beim Beenden noch einmal.
  Beim Öffnen eines Projekts bzw. beim ersten Bild des Editors: Frage „Wiederherstellen/Verwerfen“
  mit Liste und Zeit; Wiederherstellen setzt den Text als Änderung (Rückgängig = gespeicherte
  Fassung). Vor dem Öffnen eines anderen Projekts wird die bisherige Arbeit gesichert.
- Doppelklick/Kommandozeile/zweite Instanz: loadPath erkennt .jksmenu, schaltet in den
  Menü-Editor und merkt das Projekt fürs nächste Bild vor. Dateizuordnung nur nach Rückfrage im
  Programm (nur HKCU, SHChangeNotify).
- Selbsttest (BHED_MENUE_NUR=m5b): Projekt anlegen (Ordner, Datei), ändern + sichern, „Absturz“
  (alles zu ohne Speichern), Projekt öffnen -> Frage -> Wiederherstellen, Rückgängig, speichern,
  pk3 (2 Dateien), Doppelklick-Weg (dateiVonAussen -> loadPath), Zuordnungsfrage mit Nein:
  11 OK, 0 FEHLER; HKCU\Software\Classes\.jksmenu danach nicht vorhanden (nie geschrieben).
- Offen/bewusst so: Menüs = offene Reiter (ein geschlossener Reiter fällt beim nächsten Speichern
  aus der Menüliste, seine Datei bleibt als Paketdatei im Ordner). Schließen eines Reiters fragt
  weiterhin nicht nach (vorher schon so) - die Sicherung fängt nur bis zum Schließen Geändertes.

### Punkt 4 - Mod-Profile

- Modul src/menue/profil.cpp: Profildatei *.jksprofile (name, game, menulist sp|mp, itemkey/menukey
  <name> <art>, ownerdraw/feeder <nr> <NAME>, uiscript <name> <wörter>, cvar <name> <vorgabe>);
  nicht verstandene Zeilen bleiben erhalten (Rundlauf). Eingebaute Profile Jedi Academy, Jedi
  Outcast, Movie Duels tragen die vorher fest eingetragenen Menülisten (ui/menus.txt,
  ui/jampmenus.txt + jampingame.txt, ui/jk2mpmenus.txt + jk2mpingame.txt,
  ui/MovieDuels-MP-Menus.txt); Menüsatz-Knopf und Schriftnamen nehmen sie aus dem Profil
  (Schriften bei eingebauten Profilen wie bisher Jedi Academy zuerst).
- Parser: ein Schlüssel, den die Tabellen nicht kennen, das Profil aber, wird mit der Art aus dem
  Profil gelesen (ohne Meldung, Eigenschaft.ausProfil); ohne Profil meldet er wie die Engine. Text
  bleibt byte-gleich. Laufzeit: Wortzahl der uiScripts und Cvar-Vorgaben aus dem Profil. Vorschau:
  Ownerdraws/Feeder ohne Inhalt als Platzhalter mit dem Namen aus dem Profil. Eigenschaften:
  Abschnitt „Mod-Schlüssel (Profil …)“ mit dem Wert wie im Text.
- Auswahl im Projekt-Menü (wird im Projekt gespeichert), eigene Profile aus
  <Einstellungen>/menue_profile und dem Projektordner, „Profil-Vorlage anlegen“.
- Generator tools/gen_menue_profil.py (Mod-Quellen gegen OpenJK code/ui bzw. codemp/ui). An den
  Movie-Duels-Quellen (SP\mdgit) geprüft: findet genau die vier MD-Schlüssel des Parsers (cvarInt,
  cvarRotateScale mit Hinweis „liest mehrere Werte“, exec, ownerdrawID), die MD-uiScripts mit den
  Wortzahlen, die der Editor von Hand hat (mdUpdate 1, md_wheelpick 1, giveammo 3,
  addpistolselection 7), Feeder 84-89, Ownerdraw 257, 58 Cvar-Vorgaben, Menülisten. OpenJK gegen
  sich selbst: keine Zusätze.
- Fund dabei: Movie Duels MenuParse_ownerdrawID hat den Rumpf auskommentiert und gibt qfalse zurück;
  der Parser las im menuDef eine Zahl. ctest menue_profil rot, jetzt Art ImmerFalsch („Couldn't parse
  menu keyword ownerdrawID as ownerdrawID“ wie das Spiel), grün; Menu::ownerdrawID entfernt.
- Galactic Legacy: kein veröffentlichter Quellcode gefunden (Websuche: nur Mod-Seiten auf ModDB,
  JKHub, GamePressure; keine Quelle/Lizenz genannt), auf dem Rechner nicht installiert. Nichts
  heruntergeladen. Dafür die Vorlage zum Ausfüllen (profilVorlage, Knopf „Profil-Vorlage anlegen“).
- ctest menue_profil; Selbsttest (m5b): ohne Profil Meldung, mit Profil keine und der Abschnitt
  Mod-Schlüssel, Vorlage galactic_legacy anlegen und wählen - zusammen mit Punkt 1/3: 14 OK, 0 FEHLER.

## Changelog

## English
- Menu editor: layouts like Qt - arrange the selected elements side by side, stacked, in a grid or in two columns with a divider with one click (Ctrl+Shift+1 to 4), with spacing, margins, spacers and alignment; an enclosing panel becomes the area and moving it moves the elements. The .menu only gets fixed rects; the layout is kept as a comment the game skips, menus without a layout stay byte-identical. The longest text of all languages counts, too narrow elements are named.
- Menu editor: an Events tab per element like Unreal UMG - hover, hover ends, click, focus, focus lost and Enter as action blocks that write the script (only commands the game's engine knows), ready-made hover effects (glow, colour, border, bigger), hover and click sounds with listening; every change is one undo step and shows up in the Text tab at once.
- Menu editor preview checked menu by menu against the original Jedi Academy (single and multiplayer): missing @ texts show nothing like the game, missing images of shader elements show the game's grey default box, the character lists follow the chosen species, the multiplayer start values (player name, model, server name, limits), the multiplayer head list in rows and columns and the multiplayer owner-draws for team slots, game type, skill, team colour and map preview.

## Deutsch
- Menü-Editor: Layouts wie in Qt - die gewählten Elemente mit einem Klick nebeneinander, untereinander, im Raster oder in zwei Spalten mit Teiler anordnen (Strg+Umschalt+1 bis 4), mit Abstand, Rändern, Spacern und Ausrichtung; ein umschließendes Panel wird zur Fläche, verschiebt man es, ziehen die Elemente mit. Ins .menu kommen nur feste rects; das Layout bleibt als Kommentar erhalten, den das Spiel überliest, Menüs ohne Layout bleiben byte-gleich. Es zählt der längste Text aller Sprachen, zu schmale Elemente werden genannt.
- Menü-Editor: ein Reiter Ereignisse je Element wie in Unreal UMG - Hover, Hover verlassen, Klick, Fokus, Fokus verlassen und Enter als Aktions-Bausteine, die das Skript schreiben (nur Befehle, die die Engine des Spiels kennt), fertige Hover-Effekte (Leuchten, Farbe, Rahmen, Größer), Hover- und Klick-Klang mit Probehören; jede Änderung ist ein Rückgängig-Schritt und steht sofort im Text-Reiter.
- Vorschau des Menü-Editors Menü für Menü gegen das Original-Jedi-Academy geprüft (Einzel- und Mehrspieler): fehlende @-Texte zeigen wie im Spiel nichts, fehlende Bilder von Shader-Elementen den grauen Ersatzkasten des Spiels, die Figurenlisten folgen der gewählten Spezies, dazu die Startwerte des Mehrspielers (Spielername, Modell, Servername, Grenzen), die Kopfliste des Mehrspielers in Zeilen und Spalten und die Mehrspieler-Ownerdraws für Teamplätze, Spielart, Schwierigkeit, Teamfarbe und Kartenvorschau.

## 中文
- 菜单编辑器:类似 Qt 的布局——一键将所选元素并排、上下、网格或带分隔条的两栏排列(Ctrl+Shift+1 至 4),可设置间距、边距、弹性空白和对齐;包围其他元素的面板会成为区域,移动它时元素随之移动。.menu 中只写入固定的 rect;布局以游戏会跳过的注释保留,没有布局的菜单保持逐字节相同。以所有语言中最长的文本为准,过窄的元素会被指出。
- 菜单编辑器:类似 Unreal UMG 的每个元素的“事件”标签页——悬停、离开悬停、单击、获得焦点、失去焦点和回车,以写入脚本的操作块表示(只提供游戏引擎支持的命令),现成的悬停效果(发光、颜色、边框、放大),悬停与单击音效可试听;每次修改都是一个撤销步骤,并立即显示在文本标签页中。
- 菜单编辑器预览已逐个菜单与原版 Jedi Academy(单人与多人)对照:缺失的 @ 文本与游戏一样不显示,着色器元素缺失的图像显示游戏的灰色默认方框,角色列表跟随所选种族,另有多人模式初始值(玩家名、模型、服务器名、限制)、多人模式头像列表的多行多列显示,以及多人模式队伍位置、游戏类型、难度、队伍颜色和地图预览的 ownerdraw。

## 日本語
- メニューエディター: Qt のようなレイアウト。選んだ要素をワンクリックで横並び・縦並び・グリッド・仕切り付き 2 列に配置できます(Ctrl+Shift+1〜4)。間隔・余白・スペーサー・配置を設定でき、ほかを囲むパネルが領域になり、動かすと要素も付いてきます。.menu には固定の rect だけが入り、レイアウトはゲームが読み飛ばすコメントとして残ります。レイアウトのないメニューはバイト単位で同じままです。全言語で最も長いテキストを基準にし、狭すぎる要素は表示されます。
- メニューエディター: Unreal UMG のような要素ごとの「イベント」タブ。ホバー、ホバー終了、クリック、フォーカス、フォーカス喪失、Enter を、スクリプトを書く操作ブロックとして扱います(ゲームのエンジンが知っているコマンドのみ)。既製のホバー効果(光る、色、枠、大きく)、試聴付きのホバー音とクリック音もあります。変更はそれぞれ 1 回の元に戻す操作で、すぐにテキストタブにも反映されます。
- メニューエディターのプレビューをオリジナルの Jedi Academy(シングル/マルチ)とメニューごとに照合しました: 存在しない @ テキストはゲームと同じく何も表示せず、シェーダー要素の画像がない場合はゲームの灰色の既定の箱を表示し、キャラクターリストは選んだ種族に従います。さらにマルチプレイの初期値(プレイヤー名、モデル、サーバー名、上限)、マルチプレイの頭部リストの行列表示、チーム枠・ゲームタイプ・難易度・チームカラー・マッププレビューの ownerdraw に対応しました。
