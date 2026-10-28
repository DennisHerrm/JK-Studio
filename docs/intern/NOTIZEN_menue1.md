# Notizen Menü-Editor, Stufe M1 (Zweig menue1)

Auftrag: neuer Arbeitsbereich **Menu Editor** (Modus 4 der Modusleiste), in dem
man JKA/JK2/Movie-Duels-Menüs (.menu) als Text und visuell bearbeitet - gelesen,
gezeichnet und bedient wie im Spiel.

## Aufbau

| Teil | Datei | Herkunft |
|---|---|---|
| Wortleser SP (COM_ParseExt) und MP (botlib-Präprozessor: #include, #define mit Parametern, #/##, #if/#ifdef/#else/#elif/#endif, #undef, #pragma, #eval, Stringverkettung) | `src/menue/lexer.{h,cpp}` | OpenJK code/qcommon/q_shared.cpp, codemp/botlib/l_script.cpp, l_precomp.cpp |
| menuDef/itemDef/assetGlobalDef/loadMenu lesen, Schlüsseltabellen je Dialekt (SP-Namen ITEM_TYPE_* usw., MP-Zahlen über menudef.h), typeData-Pflicht, Menu_PostParse/Item_SetScreenCoords, Meldungen im Wortlaut | `src/menue/parser.cpp`, `include/bhed/menue/menue.h` | codemp/ui/ui_shared.c, ui_main.c, code/ui/ui_shared.cpp, ui_main.cpp, Movie Duels code/ui/ui_shared.cpp (cvarInt, cvarRotateScale, exec, ownerdrawID) |
| Spannen-Änderungen (rect/Eigenschaft setzen, einfügen) | `src/menue/bearbeiten.cpp` | eigen |
| @MENUS-Texte (.str JA/MD, .sp JK2) | `src/menue/texte.cpp` | OpenJK stringed/strip |
| Spieldaten (pk3 in Engine-Reihenfolge, nur lesen) | `src/menue/spieldaten.cpp` | eigen |
| Vorschau: Menu_Paint/Item_Paint/Window_Paint, Text (RE_Font_DrawString, StrLenPixels, HeightPixels, .fontdat), Ja/Nein, Mehrfachwahl, Schieber, Eingabefeld, Listbox (senkrecht), Wrapped/AutoWrapped, descText, cvarTest/showCvar | `src/menue/vorschau.cpp` | ui_shared.c, ui_main.c, code/rd-common/tr_font.cpp |
| Textpuffer des Code-Editors (Mehrfachcursor, Undo, Regex, Klammern, Falten, Farben, Änderungsrand, Vervollständigung) | `src/menue/textpuffer.cpp` | eigen (Rand nach geaendertGegen aus gui/app.cpp) |
| Bedienen: Item_RunScript mit open/close/show/hide/setfocus/setcvar/setitemtext/setitemcolor/setbackground/setitemrect/transition, Menüstapel, onOpen/onClose/onESC, Fokus, Ja/Nein/Mehrfachwahl/Schieber; gefundene Cvars; Seiten inkl. ui_tab-Unterseiten | `src/menue/skript.cpp` | ui_shared.c |
| Werkzeuge der Fläche (Einrasten, Ausrichten/Verteilen, Vorlagen, Einfügen/Kopieren/Löschen/Ebenen) | `src/menue/werkzeuge.cpp` | eigen |
| Oberfläche | `gui/app_menue.cpp`, `gui/menueeditor.h` | eigen; Rand über `zeichneRinne` des Skript-Editors |
| Selbsttest | `gui/selbsttest_menue.inc` (BHED_EDITORTEST=menueeditor) | eigen |
| Handbuch | `tools/handbuch/inhalt_menue.py`, Texte `tools/i18n_menue1.py` | eigen |

Quellen: OpenJK-Teile fehlten unter SP\ojk\oj; nur lesend von
github.com/JACoders/OpenJK nach `SP\ojk_ui` geholt (codemp/ui, code/ui,
codemp/botlib, code/qcommon). Lizenzprobe `karte_lizenz` prüft jetzt auch
`src/menue` und `include/bhed/menue` (eigene Dateien tragen „Eigener Code“).

## Ergebnisse

(siehe Abschnitt Tests unten)

## Abweichungen vom Spiel (bewusst, offen benannt)

- SP `PC_ParseString` läuft am Dateiende im Spiel endlos (COM_ParseString prüft einen Zeiger statt des Inhalts); hier: Fehler „unexpected EOF“. Ebenso Menu_Parse am Dateiende (prüft `!token2`, nie NULL): hier „End of file inside menu.“.
- SP-Schriftnummern (`font N` ist der Renderer-Griff): angenommen in der Anmeldereihenfolge von Asset_Parse (main.menu), 0 = mediumFont. Ob die Engine vorher Schriften anmeldet, ist nicht geprüft.
- `fullScreen` ohne `background`: das Spiel zeichnet den Standardshader (Griff 0); die Vorschau zeichnet nichts.
- Listbox nur senkrecht und mit Platzhalterzeilen (Feeder brauchen das Spiel), Ownerdraw/Modell/Video als beschriftete Platzhalter, `transition` springt sofort ans Ziel, Fade (fadeAmount/fadeClamp) nicht.
- Asiatische Schriftzweige (RE_Font_*) nicht portiert.
- `Item_ApplyHacks` (SP) nicht portiert.

## Offen

- RoQ-Decoder fehlt: `cinematic` nur als Platzhalter (WINDOW_STYLE_CINEMATIC).
- 3D-Modelle im Menü (ITEM_TYPE_MODEL) nur als Platzhalter, nicht über die Figurendarstellung.
- Code-Editor mit der Programmschrift (proportional), keine eigene Festbreitenschrift.
- Bildvergleich gegen echte Engine-Fotos (nur Layout-Rechnungen als Unit-Tests).

## Was M2 braucht

- Detailbereich in 3 Ebenen (ui_tab / ui_sel), Seitenlogik über mehrere Dateien eines Menüsatzes (menus.txt).
- Schriften inkl. TTF -> .fontdat.
- Mehrsprachigkeit (Texte je Sprache, @MENUS-Einträge anlegen/bearbeiten).
- Klänge (focusSound, play, soundLoop) abspielen.
- Übergänge onOpen/onClose/transition/fadein/fadeout animiert.
- Bauteil-Bibliothek (eigene Vorlagen).
- HUD-Editor (hud.menu, ownerdraws mit Spielwerten).
- Nur Änderungen als Mod exportieren.
- Exportprüfungen: Engine-Grenzen (MAX_MENUITEMS, MAX_MENUS, Skriptlänge 2048/4096, MAX_MULTI_CVARS), nie sichtbare Items, fehlende Assets, Verträglichkeit SP/MP/JK2/MD.
- Videos -> RoQ.
- „Im Spiel testen“ für Menüs.
