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
- „Im Spiel testen“ startet Movie-Duels-Programme ohne die Launcher-Umgebung
  (MOVIEDUELS_LAUNCHER); das Programm startet dann seinen Launcher - nicht geprüft (Launcher
  nie starten), die Tests setzten die Umgebung wie in M2.

## Changelog

## English
- Menu editor: a HUD editor - ui/hud.menu is drawn like Jedi Academy draws it (tics, numbers, saber style, ammo, crosshair) with example values on sliders.
- Own texts in Korean, Japanese and Chinese (Traditional and Simplified) in the encodings the game reads, drawn from the game's glyph pages.
- All Movie Duels blade styles (cg_SFXSabers 0-11, unstable blades) and black blades; the character's animation runs like the game; scripts read their words like the game (fixes Movie Duels menus without semicolons); exec "music" plays the background music.
- Test in game: Movie Duels programs with Jedi Academy data now get fs_game base, so the edited menus are shown.

## Deutsch
- Menü-Editor: ein HUD-Editor - ui/hud.menu wird so gezeichnet wie in Jedi Academy (Striche, Zahlen, Säbelstil, Munition, Fadenkreuz), mit Beispielwerten an Reglern.
- Eigene Texte auf Koreanisch, Japanisch und Chinesisch (traditionell und vereinfacht) in den Kodierungen, die das Spiel liest, gezeichnet aus den Glyphenseiten des Spiels.
- Alle Klingenarten von Movie Duels (cg_SFXSabers 0-11, instabile Klingen) und schwarze Klingen; die Animation der Figur läuft wie im Spiel; Skripte lesen ihre Wörter wie das Spiel (behebt Movie-Duels-Menüs ohne Semikolon); exec "music" spielt die Hintergrundmusik.
- Im Spiel testen: Movie-Duels-Programme bekommen bei Jedi-Academy-Daten fs_game base, damit die bearbeiteten Menüs erscheinen.

## 中文
- 菜单编辑器:HUD 编辑器——ui/hud.menu 按 Jedi Academy 的方式绘制(刻度、数字、光剑风格、弹药、准星),示例数值可用滑块调整。
- 以游戏读取的编码编写韩语、日语和中文(繁体与简体)自定义文本,并使用游戏的字形页绘制。
- Movie Duels 的全部光刃样式(cg_SFXSabers 0-11、不稳定光刃)与黑色光刃;角色动画与游戏一致;脚本像游戏一样读取参数(修复无分号的 Movie Duels 菜单);exec "music" 播放背景音乐。
- 游戏内测试:在使用 Jedi Academy 数据时,Movie Duels 程序会获得 fs_game base,从而显示编辑后的菜单。

## 日本語
- メニューエディター: HUD エディター。ui/hud.menu を Jedi Academy と同じように描きます(目盛り、数字、セーバースタイル、弾薬、照準)。例の値はスライダーで調整できます。
- 韓国語・日本語・中国語(繁体字・簡体字)の独自テキストを、ゲームが読むエンコーディングで書き、ゲームの字形ページで描きます。
- Movie Duels の全ブレードスタイル(cg_SFXSabers 0〜11、不安定なブレード)と黒いブレード。キャラクターのアニメーションはゲームと同じに動き、スクリプトはゲームと同じ語を読みます(セミコロンのない Movie Duels のメニューを修正)。exec "music" で BGM が鳴ります。
- ゲーム内テスト: Jedi Academy のデータで Movie Duels のプログラムを使うと fs_game base が付き、編集したメニューが表示されます。
