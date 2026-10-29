# Notizen Menü-Editor, Stufe M3 (Zweig menue3)

Auftrag: SP\auftrag_m3.md mit SP\auftrag_gemeinsam.md - die offenen Punkte aus M2:
MP-Menüs gegen das Original, Haltung der Figur, Klänge, echter Start über „Im Spiel
testen“, HUD-Editor, Movie-Duels-Säbelstile und schwarze Klingen, asiatische
Kodierungen, Handbuch/Selbsttest/Notizen.

## Vorfall Steam (29.10., 13:26)

Der erste Start der originalen `jamp.exe` (GameData original, per CreateProcess, mit
`+set fs_homepath`) beendete sich sofort mit 0 und startete über den Kopierschutz
(SteamStub, Abschnitt `.bind` in jamp.exe und jasp.exe) Steam selbst:
`steam.exe -applaunch 6020 +set fs_homepath …` (bootstrap_log.txt). Steam aktualisierte
sich und zeigte das Anmeldefenster. Das Beenden dieser Steam-Kette wurde vom
Rechte-System abgelehnt; der Hauptstrang wurde informiert. Danach: keine Steam-exe mehr
gestartet, Steam nicht angefasst. Die Original-Referenz kommt aus den Movie-Duels-exes
(OpenJK-Abkömmlinge ohne Steam) mit `fs_game base`.

- `jasp.exe` kennt kein `fs_homepath` (nur fs_basepath/fs_cdpath), `jamp.exe` schon.
- MovieDuels-MP (64 Bit) lädt nur `ui` + `x86_64` + `.dll` (OpenJK cl_uiapi.cpp
  VM_Create/VM_CreateLegacy, sys_main.cpp Sys_LoadGameDll); in base liegt nur die 32-Bit
  `uix86.dll` (assets2/3.pk3). Mit `fs_game base` gibt es also kein UI-Modul - die
  originale MP-Oberfläche lässt sich ohne die Steam-exe nicht zeigen. Mit `fs_game MD`
  zeichnet das Movie-Duels-UI-Modul (MovieDuels-uix86_64.dll) seine eigenen MP-Menüs.
- Ein eigener OpenJK-Bau aus den Quellen als Referenz wurde vom Rechte-System abgelehnt
  (fremder Code); die OpenJK-Quellen dienen nur zum Lesen (SP\m3\ojq\src).

## Weitere Grenzen der Spielstarts (gemessen)

- Eingaben per PostMessage erreichen die OpenJK-Abkömmlinge nur teilweise: Klicks kommen an,
  aber immer an der Stelle des Spielzeigers. Ohne Fokus verwirft OpenJK jede Mausbewegung
  (sdl_input.cpp IN_Frame: Fenster ohne Fokus -> IN_DeactivateMouse, SDL_MOUSEMOTION nur bei
  mouseActive); mit Fokus nimmt es relative Rohdaten und fängt die Maus (SDL_SetWindowGrab) -
  dann bewegt shanks echte Maus den Zeiger. Gepostete Tasten (Konsole, F9-bind) kamen nicht an.
  Nach den Säbelaufnahmen (davor ohne) immer `+set in_nograb 1` (kein Einfangen der Maus).
- Bedienzustände im Spiel (aufklappen, wählen, Schieber) sind damit nicht anfahrbar, ohne das
  Fenster in den Vordergrund zu holen (Fokus stehlen). Zustände, die das Spiel ohne Eingabe
  zeigt, gehen: Menü per `+wait N +uimenu <menü>` (Konsole und Tasten gehen nicht), Cvars per
  `+set`, Karte per `+devmap`.
- Movie Duels SP mit `fs_game base` lädt Menüs, aber keine Karte („Couldn't find item for
  inventory 1“ - das MD-Spielmodul passt nicht zu den base-Daten); ein HUD im Spiel gibt es
  so nur mit den MD-Daten (MD-HUD).
- Erster Start mit leerem fs_homepath: MD zeigt den Willkommensbildschirm von Update 7
  (`g_update7firststartup`), `fs_game base` stürzt ohne die Vorgabe-cfgs ab (aus M2 kopiert).

## Ergebnisse je Punkt

1. **MP-Menüs gegen das Original** - nicht geprüft, Grund: nur Steam-exe (jamp.exe startet
   Steam neu); MovieDuels-MP kann die base-UI nicht laden (32-Bit-Modul). Siehe „Offen“.
2. **Haltung der Figur** - UI_G2SetAnim und G2_TimingModel 1:1 portiert (`g2Bild`): Bild,
   Folgebild und Anteil ab dem Zeitpunkt des Setzens (beim Laden bzw. `uiScript character`,
   `Laufzeit::animGesetzt`), frameLerp ceil/floor wie UI_ParseAnimFileSet, Schleife, Ende ohne
   Schleife -> G2 nimmt die Animation weg (Bild 0); Mischung der lokalen Matrizen
   (worldMatricesLerp). Vorher: Bild ohne Mischung ab Programmstart. ctest menue_vorschau
   (G2-Takt, Schleife, Ende, ceil). Ein Bildvergleich mit dem Spiel war nicht möglich: der
   Zeitpunkt des Öffnens im Spiel ist nicht messbar (Konsole/Tasten erreichen das Spiel nicht,
   `+wait` zählt Bilder), und die Drehung hängt an der absoluten Spielzeit.
3. **Klänge** - im Spiel (stumm, `s_show 1`, `developer 1`): JKA-Hauptmenü
   `0 : sound/interface/menuroam for ent 0, chan=9` (setfocus newgamebutton -> FocusSound,
   .wav fehlt -> .mp3); MD-Hauptmenü `S_StartBackgroundTrack ... CTCMusic` (exec "music ...").
   Rot: die Vorschau spielte die MD-Musik nicht (exec nur gemeldet). Grün: `exec "music X"`
   startet die Hintergrundmusik (Selbsttest M3 Klänge: Liste gleich dem Spiel). Klänge beim
   Fokus/Klick einzelner Items im Spiel nicht prüfbar (keine Mausbewegung, siehe oben).
4. **Echter Start über „Im Spiel testen“** - Testmenü (Hauptmenü + Text „JK STUDIO TEST“)
   mit der Befehlszeile des Editors gestartet (fs_homepath im Laufordner, MD-Launcher-Umgebung
   wie in M2, `g_update7firststartup 0`): Movie Duels zeigt das bearbeitete Menü (Foto
   SP\m3\vergleich\spiel_imspiel_md_1.png). Rot bei JKA-Daten: das MD-Programm nahm ohne
   fs_game seine Mod MD, deren main.menu stand über dem Testpaket (spiel_imspiel_ja_1.png).
   Grün: `+set fs_game base` bei Movie-Duels-Programmen (ctest menue_paket;
   spiel_imspiel_ja2_0.png zeigt das bearbeitete JKA-Hauptmenü). Original-jasp.exe: nicht
   gestartet (Steam).
5. **HUD-Editor** - `ui/hud.menu` wie jedes Menü; CG_DrawHUD (JA SP) 1:1: lefthud/righthud
   per Menu_Paint, frame, Striche (voll weiß, angebrochen in der Itemfarbe mit Alpha * Anteil),
   Zahlenfelder (CG_DrawNumField, gfx/2d/numbers/t_*), Rüstung blinkt unter einem Viertel,
   Macht (übervoll mit sin-Pulsen), Säbelstil, Munition (0 rot), Fadenkreuz; Regler im
   Eigenschaften-Reiter. MP-cgame und Movie Duels zeichnen anders -> dort nur die Menüs mit
   Hinweis. ctest menue_vorschau (8 Prüfungen), Selbsttest M3 HUD. Bildvergleich mit dem Spiel
   nicht möglich: MD-SP lädt mit base keine Karte, mit MD-Daten zeigt es das MD-HUD.
6. **Movie-Duels-Säbelstile** - alle UI_Do*Saber (cg_SFXSabers 0..11, UI_DoSaberUnstable für
   instabile Säbelarten) tabellarisch aus dem MD-Code (Shader je Art und Farbe per Skript aus
   ui_saber.cpp erzeugt; Glanz-/Kernfaktoren, flrand-Mitten bei OT, Versatz bei Rebels/CW/Maul,
   zweiter Kern bei SABER_RGB, Größen-Cvars, g_SerenityJediEngineMode); schwarze Klingen mit
   blendFunc blend (vormultipliziert aufgelegt), black_glow (GL_DST_ALPHA GL_ONE) mit Ziel-Alpha 1.
   Rot: ctest „nicht portierte Art zeichnet nichts“ (vorher Arten 1-3, 5-11 ohne Klinge).
   Bildvergleich mit MD-Spielbildern (fs_game MD) siehe Tabelle unten.
7. **Asiatische Kodierungen** - Koreanisch (KSC 5601), Taiwanesisch (Big5), Japanisch
   (Shift-JIS), Chinesisch (GB 2312) wie tr_font.cpp sie liest, beim Schreiben über die
   Windows-Codepages 949/950/932/936 mit der Gültigkeitsprüfung der Engine (nicht Lesbares wird
   ? und gemeldet), Lesen zurück nach UTF-8; Vorschau zeichnet die Glyphen aus fonts/kor_32_1024_*,
   jap/chi/tai_16_1024_* mit fScaleAsian 0.75, fAsianYAdjust, Vorschub je Sprache. Rot im
   Selbsttest (Paket mit ?), grün danach; ctest menue_parser (Kodierung, Glyphen, Breiten).
   Thai (TIS-620 mit tha_codes.dat) bleibt Latin-1.
8. Handbuch-Seite „Menü-Editor: HUD, asiatische Texte und Lichtschwerter“ (4 Sprachen),
   M2-Seite angepasst (Säbelarten); Selbsttest `menueeditor` um M3 erweitert
   (`BHED_MENUE_NUR=m3` für die gezielten Läufe).

Dazu gefunden und behoben (rot -> grün):

- **Skriptwörter**: `uiScript` las bis zum nächsten `;` - Movie-Duels-Skripte ohne `;`
  verloren alles nach dem ersten uiScript (saber.menu onOpen: hide jkanewgame & Co. liefen
  nicht, Platten sichtbar, Griff falsch). Jetzt wie Item_RunScript/UI_RunMenuScript: Name plus
  die Wörter des Skripts (Tabelle aus OpenJK SP/MP und MD erzeugt), `exec` & Co. mit den Wörtern
  ihres Handlers, unbekannte Befehle wie DC->runScript. ctest menue_skript.
- **„Im Spiel testen“ mit Movie-Duels-Programm und base-Daten** (Punkt 4).
- **Start-Cvars**: die Vorschau kannte nur einzelne Vorgaben; jetzt alle aus UI_Init
  (ui_atoms.cpp Cvar_Create) und der cvarTable (ui_main.cpp) je Spiel, erzeugt mit
  tools/gen_menue_cvarvorgaben.py (src/menue/cvarvorgaben.inc). Rot: MD-Säbelmenü mit zweitem
  Griff (g_saber_type leer -> ui_saber_type leer -> saber2 nicht versteckt). ctest menue_vorschau.
- **setfocus**: spielt wie Script_SetFocus (OpenJK SP/MP, MD) den itemFocusSound; rot im
  Selbsttest „M3 Klänge“ mit JKA-Daten (leere Liste statt menuroam), ctest menue_skript.
- MD-Säbelmenü nach den Skript- und Cvar-Korrekturen: mittlere Abweichung je Klingenart von
  6.1-11.7 auf 3.5-4.8 (Grauwerte 0-255, 8x6-Raster; 9/10 höher, weil im Spielbild durch den
  Spielzeiger Felder aufgeklappt waren), Bildpaare SP\m3\vergleich
eben_saebel_*.png.
- **Spielordner**: `Weitere Grenzen` oben; die Steam-Kette (Vorfall) bleibt beim Hauptstrang.

## Tests (Schlussrunde)

- Neubau (`--clean-first`, alle Ziele): 0 Warnungen, 0 Fehler (/W4 /WX).
- ctest: 155/155 grün (erweitert: menue_parser Kodierung, menue_vorschau HUD/G2-Takt/Säbelarten/
  Start-Cvars, menue_skript Skriptwörter/setfocus/music, menue_paket fs_game base).
- Selbsttest menueeditor: 52 OK, 0 FEHLER mit Movie-Duels-Daten und 52 OK, 0 FEHLER mit den
  Daten von „GameData original“ (nur gelesen).
- Selbsttest karteneditor (BHED_KARTE_NUR=fotos): 78 OK, 0 FEHLER.
- Modus 1: 2645 OK, 0 FEHLER.
- Säbelmenü (MD, Klingenart 4 blau) nach den Start-Cvars: kein zweiter Griff mehr, mittlere
  Abweichung 3.3 (vorher 6.1); der Griff selbst wirkt im Spiel schlanker (offen).
- Spielordner (Movie Duels und „GameData original“) vorher/nachher: 4047 Einträge gleich
  (Name, Größe, Zeit). Keine eigenen Spielprozesse mehr (ein hängendes MovieDuels-MP wurde
  per eigener PID beendet und geprüft).

## Offen

- MP-Menüs (JKA) gegen das Original: nicht geprüft, Grund: nur Steam-exe (jamp.exe startet
  Steam neu; MovieDuels-MP lädt kein 32-Bit-UI-Modul aus base). Movie-Duels-MP-Menüs: siehe
  Vergleichstabelle.
- Bedienzustände im Spiel (aufklappen, wählen, Schieber, Farben, Reiter): nicht anfahrbar
  ohne Fokus/Mausfang (siehe „Weitere Grenzen“). Möglicher Weg ohne Eingabe: ein Testpaket,
  dessen onOpen die Aktionen des Zustands ausführt (wie bei den MP-Menüs mit `open`).
- Haltung der Figur: Bildvergleich mit dem Spiel offen (Zeitpunkt nicht messbar).
- HUD des MP-cgame und von Movie Duels (eigene Darstellungen) nicht nachgebildet.
- Thai-Texte (TIS-620, tha_codes.dat) nicht nachgebildet.
- Systematischer Vergleich aller Menüs der Menüsätze mit Rangliste (Wunsch shank/Hauptstrang)
  und die Oberflächenpunkte (Mindestschrift, Augen-/Schloss-Symbole statt „o“/„.“, Leisten-
  Tooltips/Beschriftung, Fotos bei 100 % und 150 %): nicht mehr geschafft. Werkzeuge liegen
  bereit: SP\m3\spiel.py (Start mit `+wait N +uimenu`), mp_oeffne_pk3.py (MP-Menüs per
  onOpen-`open`), bildvergleich.py, BHED_MENUE_VERGLEICH_MENUES im Selbsttest.
- MD-Säbelmenü: Griffmodell im Spiel schlanker; Felder, die im Spiel durch den Spielzeiger
  aufgeklappt waren, nicht vergleichbar.
- „Im Spiel testen“ startet Movie-Duels-Programme ohne die Launcher-Umgebung
  (MOVIEDUELS_LAUNCHER); das Programm startet dann seinen Launcher - nicht geprüft (Launcher
  nie starten), die Tests setzten die Umgebung wie in M2.

## M4 (Fortsetzung, Auftrag SP\auftrag_m4.md)

Unterbrochen 29.10. 16:58 (Pause, Rechner neu gestartet), weiter ab 17:28.

### Spielstarts, Steam

- Steam: 16:17-16:47 jede Minute geprüft - steam.exe lief nicht, ActiveUser 0. Danach gilt
  shanks Gedächtnisregel „Kein Steam-Start“ (Agenten starten nie Steam-JKA-exes); die
  Original-exes (jamp/jasp aus GameData original) wurden nie gestartet. Original-JKA-MP/SP über
  Steam: nicht geprüft.
- Movie Duels nur per spiel.py (Popen, Fenster, stumm, in_nograb 1, eigene PIDs).

### Punkt 5 - Video-Import (rot -> grün)

Belege aus dem Code (OpenJK, Movie Duels gleich):
- `RE_StretchRaw` (tr_draw.cpp) bricht mit ERR_DROP „size not a power of 2“ ab - so laufen
  Vollbild-Videos (SCR_DrawCinematic) und cinematic-Items der MP-Menüs (CIN_DrawCinematic).
  `RE_UploadCinematic` (videoMap) prüft das nicht.
- `cin.linbuf[512*512*4*2]`: mehr als 512*512 Punkte schreiben über den Puffer.
- `sbuf[32768]` je Tonstück, `cin.file[65536]` je Stück.
- `UI_PlayCinematic` (codemp ui_main.c) und `tr_shader.cpp` videoMap starten mit `CIN_silent`
  -> Menüvideos sind immer stumm; `CG_PlayCinematic` und `cinematic <name>` (CL_PlayCinematic_f)
  spielen mit Ton.
- SP-`Window_Paint` (OpenJK code/ui, MD ui_shared) hat keinen Fall für WINDOW_STYLE_CINEMATIC
  (und keine TEAMCOLOR-Fläche): im Einzelspieler zeigt ein cinematic-Item nichts.
  JKA und MD zeigen ihre Menüvideos über Shader mit videoMap als background
  (gfx/menus/videologo, gfx/menus/mdmainmenuvideo).
- Alle 63 Videos von JKA und MD: 512x512, 30 fps; Ton 0x1021 (stereo); ja01.roq: Tonstück 5888
  Proben vor ROQ_QUAD_INFO, dann 736/735 Proben je Bild (SP\m4\skripte\roq_groessen.py).

Umgesetzt:
- Zielgröße Zweierpotenzen 16..1024 mit Breite*Höhe <= 512*512, Vorgabe 512x512; Strecken /
  Balken / Zuschneiden; Verkleinern mit Flächenmittel (anteilig je Achse), Vergrößern bilinear;
  Bildrate 5-60 (Vorgabe 30); Länge, Bildzahl und geschätzte Größe vorher (die ersten 6 Takte
  im Hintergrund kodiert und hochgerechnet; Ton exakt).
- Kodieren im Fluss (RoqImport): vorher hielt der Import alle Bilder in Quellgröße (1080p: 8 MB je
  Bild, 1 min = 14 GB). Blende (MF_MT_MINIMUM_DISPLAY_APERTURE) beachtet.
- Ton: Media Foundation -> PCM, Fenster-Sinc (Hann, 16 Nullstellen) auf 22050 Hz, mono/stereo,
  DPCM wie RllDecodeMonoToStereo/StereoToStereo (Startwert im arg, sqrTable), Lage wie ja01.roq;
  „Ton übernehmen“ ab Werk aus mit Erklärung; Hinweis ohne Tonspur.
- Fehlende Codecs: Meldung je Gruppe (HEVC, AV1, VP8/VP9, DivX/Xvid, unbekannter FourCC, nicht
  lesbar, Media Foundation fehlt) in 4 Sprachen; nichts wird abgelegt. RoQ unverändert übernommen.
- Shader `menu/video/<name>` { nopicmip nomipmaps { videoMap video/<name>.roq rgbGen identity } }
  in `shaders/<paket>.shader` (Paket = Textpaket, sonst mymenu); Verwendung: Hintergrund des
  gewählten Elements / neues Vollbild-Element (ganz hinten) / cinematic (nur MP) / nur Paket.
  Die Vorschau liest Paket-Shader mit (shaderLaden).
- Prüfung: Befund CinematicSp. Vorschau: SP zeichnet kein cinematic/TEAMCOLOR (51d4bff5).
- Ton-Schalter der Leiste („Ton an/aus“, Symbol TonAus rot): Einstellung menueTon; vorher sperrte
  der Knopf nur neue Klänge, laufende Musik spielte weiter (audio.play ohne Nummer) - jetzt
  laufen Klänge mit Nummer und werden angehalten.

Tests:
- ctest menue_roq: Größenprüfung, Flächenmittel (Schachbrett halbiert = 127/128, 6->4, 7->4
  anteilig), Balken/Zuschneiden/Strecken, RoqImport-Takte, Ton 44,1->22,05 kHz (1 kHz Fehler
  0,4, 15 kHz Rest 1,2 von 7071), Rundlauf mono 48,1 dB / stereo 47,3 dB über den 1:1-Decoder,
  Lage der Tonstücke, Codec-Gruppen. menue_vorschau SP/MP cinematic (rot mit altem Stand),
  menue_pruefung CinematicSp.
- Selbsttest `BHED_MENUE_NUR=m4`: JKA-Daten 12/15 (die 3 Fehler: Klick auf den Ton-Schalter
  kam nicht an - HoveredWindow leer, im nächsten Lauf mit denselben Schritten grün), MD-Daten
  13/13, danach mit Punkt 3: 18/18. H.264 1920x1080 mit Ton -> 512x512 an background_video (JKA)
  bzw. background_CTCvideo/background_mainvideo (MD), Shader von unserem Leser gelesen,
  19 Videobilder in der Vorschau, Export-pk3 mit Menü+Video+Shader, Ton 440 Hz / 3,0 s zurück
  (439,8 Hz), ZZZZ-AVI -> Meldung, kaputt.mp4 -> Meldung, RoQ bytegleich übernommen.
  HEVC, VP9, AV1 und Xvid dekodiert dieser Rechner (Erweiterungen installiert) - deren
  Meldungen prüft nur ctest (Zuordnung) und der Selbsttest bedingt.
- Echter Spielstart: „Im Spiel testen“ (MD-Daten) -> MovieDuels-SP mit fs_homepath
  …\einst\menuetest und fs_game MD (Befehlszeile des Editors), per spiel.py: das MD-Hauptmenü
  zeigt das importierte Testbild-Video als Hintergrund, bewegt (SP\m3\vergleich\
  spiel_m4_video_md_0.png/_1.png, nebeneinander SP\m4\spiel_video_md.png). Mit JKA-Daten aus
  „GameData original“ gibt „Im Spiel testen“ kein Programm (jasp.exe kann kein fs_homepath).

### Punkt 3 - Lesbarkeit

- Ebenenliste: Auge (Sichtbar/Versteckt) und Schloss (zu/offen) als Symbole statt „o“/„-“,
  „L“/„.“; Hinweise wie vorher.
- Alle Leistenknöpfe mit Hinweis (Rückgängig/Wiederholen hatten keinen); Einstellung „Leistenknöpfe
  mit Beschriftung“ (menueLeisteText) zeigt Symbol + Text.
- Mindestschrift: Oberflächengröße nie unter 75 % (16 px * 0,75 = 12 px; vorher Strg+Rad bis 50 %
  = 8 px), auch aus alten Einstellungen. „Window“ in der Auflösungsliste übersetzt.
- Selbsttest: Auge/Schloss per Klick (verborgen 1, gesperrt 1), Fotos 100 % und 150 %
  (menue_oberflaeche_100/150), mit Beschriftung (Knopf breiter als 2x hoch), Mindestschrift.

### Punkt 4 - Griff im MD-Säbelmenü

Geprüft: Item_Model_Paint (MD ui_shared.cpp) gegen menueModellBild - fov_x/fov_y 87/87,
rect -80 -182 710 710, g2mins/maxs 0/22 -> origin (41,04, 11, -11), Winkel (curYaw, 0, 90),
gleiche Projektion (tan je Achse). LOD: das Spiel setzt ent.radius = 1000 -> G2_ComputeLOD
ergibt LOD 0; der Editor lädt nur LOD 0 - gleich. Gemessen im Bildpaar neben_saebel_4_blue_neu
(640x480): Klinge auf halber Höhe 13/13, 13/12, 16/15 Zeilen, Griff 9/8, 11/11, 11/11 Zeilen
(Editor/Spiel, SP\m4\skripte\profil.py). Maßstab und Sichtfeld stimmen; der Eindruck „dicker“
kommt von der Drehphase (der Griff dreht sich mit model_rotation 20 um seine Längsachse, sein
Querschnitt ist nicht rund) und dem Lichtglanz - kein Fehler im Maßstab.

## Changelog

## English
- Menu editor: a HUD editor - ui/hud.menu is drawn like Jedi Academy draws it (tics, numbers, saber style, ammo, crosshair) with example values on sliders.
- Own texts in Korean, Japanese and Chinese (Traditional and Simplified) in the encodings the game reads, drawn from the game's glyph pages.
- All Movie Duels blade styles (cg_SFXSabers 0-11, unstable blades) and black blades; the character's animation runs like the game; scripts read their words like the game (fixes Movie Duels menus without semicolons); exec "music" plays the background music.
- Test in game: Movie Duels programs with Jedi Academy data now get fs_game base, so the edited menus are shown.
- Menu editor, video import: choose the size (powers of two up to 512 x 512, default 512 x 512 like all game videos), stretch, black bars or crop, frames per second; pictures are reduced by averaging, length and file size are shown beforehand; long and Full HD videos no longer need all frames in memory.
- The import writes the shader the menu needs (menu/video/<name> with videoMap in shaders/<package>.shader) and sets it as background of the selected element or as a new full-screen video element; shader, video and menu go into the exported .pk3 and into Test in game.
- Sound from the video (22050 Hz, mono or stereo, stored like the game's videos) - off by default, because menus play videos silently; a note for videos without a sound track; RoQ files are taken over unchanged.
- Clear messages in four languages when Windows lacks a codec (HEVC, AV1, VP8/VP9, DivX/Xvid, unknown) instead of an empty video.
- A visible Sound on/off switch in the toolbar for all preview sounds; it also stops sounds that are playing and is remembered.
- Single-player menus no longer show cinematic items and teamcolor fills the game does not draw; the check points this out.
- Real eye and lock symbols in the hierarchy, tooltips for every toolbar button and optional labels, text never below 12 pixels.

## Deutsch
- Menü-Editor: ein HUD-Editor - ui/hud.menu wird so gezeichnet wie in Jedi Academy (Striche, Zahlen, Säbelstil, Munition, Fadenkreuz), mit Beispielwerten an Reglern.
- Eigene Texte auf Koreanisch, Japanisch und Chinesisch (traditionell und vereinfacht) in den Kodierungen, die das Spiel liest, gezeichnet aus den Glyphenseiten des Spiels.
- Alle Klingenarten von Movie Duels (cg_SFXSabers 0-11, instabile Klingen) und schwarze Klingen; die Animation der Figur läuft wie im Spiel; Skripte lesen ihre Wörter wie das Spiel (behebt Movie-Duels-Menüs ohne Semikolon); exec "music" spielt die Hintergrundmusik.
- Im Spiel testen: Movie-Duels-Programme bekommen bei Jedi-Academy-Daten fs_game base, damit die bearbeiteten Menüs erscheinen.
- Menü-Editor, Video-Import: Größe wählbar (Zweierpotenzen bis 512 x 512, Vorgabe 512 x 512 wie alle Videos des Spiels), Strecken, schwarze Balken oder Zuschneiden, Bilder pro Sekunde; verkleinert wird durch Mitteln, Länge und Dateigröße stehen vorher da; lange und Full-HD-Videos brauchen nicht mehr alle Bilder im Speicher.
- Der Import schreibt den Shader, den das Menü braucht (menu/video/<Name> mit videoMap in shaders/<Paket>.shader), und setzt ihn als Hintergrund des gewählten Elements oder als neues Vollbild-Video-Element; Shader, Video und Menü kommen in die exportierte .pk3 und in „Im Spiel testen“.
- Ton aus dem Video (22050 Hz, mono oder stereo, gespeichert wie in den Videos des Spiels) - ab Werk aus, weil Menüs Videos stumm spielen; Hinweis bei Videos ohne Tonspur; RoQ-Dateien werden unverändert übernommen.
- Klare Meldungen in vier Sprachen, wenn Windows ein Codec fehlt (HEVC, AV1, VP8/VP9, DivX/Xvid, unbekannt), statt eines leeren Videos.
- Ein gut sichtbarer Schalter „Ton an/aus“ in der Leiste für alle Klänge der Vorschau; er hält auch laufende Klänge an und bleibt gespeichert.
- Einzelspieler-Menüs zeigen keine cinematic-Elemente und TEAMCOLOR-Flächen mehr, die das Spiel nicht zeichnet; die Prüfung weist darauf hin.
- Echte Augen- und Schloss-Symbole in der Hierarchie, Hinweise für jeden Leistenknopf und wahlweise Beschriftung, Schrift nie unter 12 Pixel.

## 中文
- 菜单编辑器:HUD 编辑器——ui/hud.menu 按 Jedi Academy 的方式绘制(刻度、数字、光剑风格、弹药、准星),示例数值可用滑块调整。
- 以游戏读取的编码编写韩语、日语和中文(繁体与简体)自定义文本,并使用游戏的字形页绘制。
- Movie Duels 的全部光刃样式(cg_SFXSabers 0-11、不稳定光刃)与黑色光刃;角色动画与游戏一致;脚本像游戏一样读取参数(修复无分号的 Movie Duels 菜单);exec "music" 播放背景音乐。
- 游戏内测试:在使用 Jedi Academy 数据时,Movie Duels 程序会获得 fs_game base,从而显示编辑后的菜单。
- 菜单编辑器视频导入:可选择尺寸(2 的幂,最大 512 x 512,默认与游戏所有视频一样为 512 x 512)、拉伸、黑边或裁剪以及每秒帧数;缩小时取平均,事先显示时长和文件大小;长视频和全高清视频不再需要把所有帧放进内存。
- 导入会写入菜单所需的着色器(shaders/<包>.shader 中带 videoMap 的 menu/video/<名称>),并将其设为所选元素的背景或新的全屏视频元素;着色器、视频和菜单一起进入导出的 .pk3 和“在游戏中测试”。
- 可保留视频的声音(22050 Hz,单声道或立体声,按游戏视频的方式保存)——默认关闭,因为菜单静音播放视频;没有音轨的视频会给出提示;RoQ 文件原样采用。
- Windows 缺少编解码器(HEVC、AV1、VP8/VP9、DivX/Xvid、未知)时,用四种语言给出明确提示,而不是生成空视频。
- 工具栏中醒目的“声音开/关”开关控制所有预览声音;关闭时也会停止正在播放的声音,并会被记住。
- 单人菜单不再显示游戏不会绘制的 cinematic 元素和 TEAMCOLOR 填充;检查会指出这一点。
- 层级中使用真正的眼睛和锁图标,每个工具栏按钮都有提示并可选择显示文字,文字永远不小于 12 像素。

## 日本語
- メニューエディター: HUD エディター。ui/hud.menu を Jedi Academy と同じように描きます(目盛り、数字、セーバースタイル、弾薬、照準)。例の値はスライダーで調整できます。
- 韓国語・日本語・中国語(繁体字・簡体字)の独自テキストを、ゲームが読むエンコーディングで書き、ゲームの字形ページで描きます。
- Movie Duels の全ブレードスタイル(cg_SFXSabers 0〜11、不安定なブレード)と黒いブレード。キャラクターのアニメーションはゲームと同じに動き、スクリプトはゲームと同じ語を読みます(セミコロンのない Movie Duels のメニューを修正)。exec "music" で BGM が鳴ります。
- ゲーム内テスト: Jedi Academy のデータで Movie Duels のプログラムを使うと fs_game base が付き、編集したメニューが表示されます。
- メニューエディターの動画取り込み: サイズ(2 のべき乗で最大 512 x 512、既定はゲームの全動画と同じ 512 x 512)、引き伸ばし・黒帯・切り抜き、フレームレートを選べます。縮小は平均で行い、長さとファイルサイズを先に表示します。長い動画やフル HD 動画でも全フレームをメモリに持たなくなりました。
- 取り込みはメニューに必要なシェーダー(shaders/<パッケージ>.shader 内の videoMap 付き menu/video/<名前>)を書き、選択した要素の背景か新しい全画面動画要素に設定します。シェーダー・動画・メニューは書き出しの .pk3 と「ゲームでテスト」に入ります。
- 動画の音声(22050 Hz、モノラル/ステレオ、ゲームの動画と同じ形式)を取り込めます。メニューは動画を無音で再生するため既定はオフです。音声トラックのない動画には注意を表示し、RoQ ファイルはそのまま取り込みます。
- Windows にコーデックがない場合(HEVC、AV1、VP8/VP9、DivX/Xvid、不明)、空の動画を作らずに 4 言語で分かりやすく知らせます。
- ツールバーに目立つ「音オン/オフ」スイッチを追加し、プレビューのすべての音を切り替えます。再生中の音も止め、設定は保存されます。
- シングルプレイヤーのメニューでは、ゲームが描かない cinematic 要素と TEAMCOLOR の塗りを表示しなくなりました。チェックがそれを指摘します。
- 階層に本物の目と鍵のアイコン、すべてのツールバーボタンにツールチップと任意の名前表示、文字は 12 ピクセル未満になりません。
