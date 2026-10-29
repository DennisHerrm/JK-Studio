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
