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

## Ergebnisse je Punkt

1. **Parser + Modell** - fertig. Rundlauf über alle Originalmenüs (ctest `menue_rundlauf`, nur lesen):
   JA 95 Dateien / 96 Menüs / 3325 Items, JK2 62 / 63 / 2062, MD 264 / 278 / 14857 - Schreiben ohne
   Änderung byte-gleich, kein Menü scheitert, je Datei eine rect-Probe (nur die Zahl ändert sich, neu
   gelesen stimmt sie). JA und JK2: 0 Meldungen; MD: 185 Warnungen, alle aus ui/pazaak/sje_pazaak_mp.menu
   (Zahlen statt Namen bei type/style im SP-Leser - meldet das Spiel genauso). Gefunden und behoben über die
   Originale: Movie-Duels-Typen ITEM_TYPE_SLIDER_INTEGER/ROTATE, Feeder-Nummern je Dialekt (SP 0x10/0x17,
   MP 0x13/0x28), `forecolor -1` (Spielerfarbe), MD-Wörter in MD-MP, `precacheSound`, sizeof(dfontdat_t) = 7180.
   Unit-Test `menue_parser`.
2. **Vorschau** - fertig für Text, Knopf, Ja/Nein, Mehrfachwahl, Schieber (auch MD SLIDER_INTEGER),
   Eingabefeld, Bind (Taste "???"), Listbox senkrecht (Platzhalterzeilen), Shader/Bilder aus den pk3 (additive
   Shader additiv), Schriften aus .fontdat, Farben ^0-^9, Schatten/Blinken, Rahmen, descText. Strecken wie das
   Original oder 4:3 mit Balken, Auflösungs-Voreinstellungen. Ownerdraw, Modell, Video, Textscroll als
   beschriftete Platzhalter. Layout-Rechnungen als Unit-Tests (`menue_vorschau`).
3. **Code-Editor** - fertig: Farben, Zeilennummern, Minimap (mit Such- und Fehlermarken, Klick springt),
   Klammerpaare, Falten (menuDef/itemDef/Skriptblöcke), Gliederung, Suchen/Ersetzen (Regex, Groß/Klein, Wort,
   alle offenen Menüs), Wort unter dem Cursor hervorgehoben, Mehrfachcursor (Alt+Klick, Strg+D,
   Strg+Umschalt+L), Vervollständigung (Schlüsselwörter, Skriptbefehle, Konstanten, Defines, Cvars, Itemnamen,
   Shader-Pfade, @MENUS), Fehlermarken mit Meldung, F12/Strg+Klick, Lesezeichen (Strg+F2/F2/Umschalt+F2, Klick
   in den Rand) und Änderungsrand mit dem Rand des Skript-Editors (`zeichneRinne`, orange/grün/blau).
   Unit-Test `menue_text`; Selbsttest tippt mit echter Eingabe und nimmt mit Strg+Z zurück.
4. **Synchron** - fertig: Ziehen/Größe/Pfeiltasten ändern nur die Zahlen von `rect`, Eigenschaften nur ihre
   Wörter (fehlende kommen als Zeile mit passender Einrückung dazu), Text ändern liest sofort neu, Klick auf ein
   Element springt zum itemDef und der Cursor wählt das Item. Lineale mit Hilfslinien zum Herausziehen, Raster,
   Einrasten an Kanten/Mitten/Hilfslinien mit Linien, Ausrichten/Verteilen, Undo/Redo (ein Puffer für Text und
   Fläche), Gruppieren (group), Ebenen (Reihenfolge), Sperren, Palette, Duplizieren (Strg+D), Löschen (Entf),
   Kopieren/Einfügen (Strg+C/V, Kopien bekommen freie Namen). `menue_werkzeug`, Selbsttest.
5. **Seiten** - fertig: alle menuDefs plus ui_tab-artige Unterseiten (cvarTest + showCvar mit mehreren
   Werten) als Reiter mit Blättern, "+" legt ein menuDef an. `menue_skript`.
6. **Bedienen + Cvars** - fertig: Item_RunScript mit open/close/show/hide/setfocus/setcvar/setcvartocvar/
   setitemtext/setitemcolor/setbackground/setitemrect/transition, Menüstapel, onOpen/onClose/onESC,
   mouseEnter/Exit, onFocus/leaveFocus, Klick auf Knopf/Ja-Nein/Mehrfachwahl/Schieber; nicht Ausführbares
   (play, exec, uiScript ...) im Skriptprotokoll. Cvar-Panel findet alle Cvars (Sichtbarkeit zuerst) mit den
   vorkommenden Werten, nur Vorschau. `menue_skript`, Selbsttest.
7. **Grafiken + Export** - PNG/JPG/TGA als gfx/menus/<name> (Bild-Item, Textur sofort in der Vorschau),
   Speichern als .menu (aus dem Spiel geöffnete nur woanders), Export .pk3 mit Menü und Bildern; Schreiben in
   Spielordner wird abgelehnt (Selbsttest prüft beides). Eigene Shader-Datei wird nicht erzeugt (Bilder gehen
   ohne Shader; reicht für die Engine).
8. **Handbuch** - Seite `menue_editor` (Gruppe Einstieg) in 4 Sprachen mit 4 Bildern aus dem Selbsttest;
   Texte in `tools/i18n_menue1.py`. ctest `handbuch_sprachen` grün.
9. **Schlussrunde** - siehe Tests.

## Tests (Schlussrunde)

- ctest: 136/136 grün (davon neu menue_parser, menue_vorschau, menue_text, menue_skript, menue_werkzeug,
  menue_rundlauf; karte_lizenz prüft src/menue und include/bhed/menue).
- Selbsttest `BHED_EDITORTEST=menueeditor`: 25 OK, 0 FEHLER (en/de/zh/ja je ein Lauf für die Bilder).
- Modus 1: 2645 OK, 0 FEHLER (mit dem Handbuch neben der .exe; ohne es scheitern nur die Touren).
- Neubau: 0 Warnungen (/W4 /WX).

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

## Changelog

## English
- New **Menu Editor** workspace: edit Jedi Academy, Jedi Outcast and Movie Duels menus (.menu) as text and on a 640x480 canvas at the same time. Files are read exactly like the game (single player and multiplayer precompiler with #include/#define/macros), with the game's warnings word for word; saving without changes is byte-identical, and moving or resizing an item only changes its rect numbers.
- Preview drawn like the game (fonts, shaders, item types), pages as tabs including ui_tab pages, an Interact mode that runs the menu scripts, and a cvar panel that only affects the preview.
- Code editor with colours, minimap, folding, multi-cursor, regex find/replace, completion, go to definition, bookmarks and the change margin of the script editor; export as .menu or .pk3 with your own images (never into the game folders).

## Deutsch
- Neuer Arbeitsbereich **Menü-Editor**: Menüs von Jedi Academy, Jedi Outcast und Movie Duels (.menu) gleichzeitig als Text und auf einer 640x480-Fläche bearbeiten. Die Dateien werden genau wie im Spiel gelesen (Einzelspieler und Mehrspieler-Präprozessor mit #include/#define/Makros), mit den Warnungen des Spiels im Wortlaut; Speichern ohne Änderung ist byte-gleich, Verschieben oder Größe ändern ändert nur die rect-Zahlen.
- Vorschau gezeichnet wie im Spiel (Schriften, Shader, Item-Typen), Seiten als Reiter samt ui_tab-Seiten, ein Bedienmodus, der die Menü-Skripte ausführt, und ein Cvar-Feld, das nur die Vorschau ändert.
- Code-Editor mit Farben, Minimap, Falten, Mehrfachcursor, Suchen/Ersetzen mit Regex, Vervollständigung, Gehe zu Definition, Lesezeichen und dem Änderungsrand des Skript-Editors; Export als .menu oder .pk3 mit eigenen Bildern (nie in die Spielordner).

## 中文
- 新增 **菜单编辑器** 工作区:同时以文本和 640x480 画布编辑 Jedi Academy、Jedi Outcast 和 Movie Duels 的菜单(.menu)。文件的读取方式与游戏完全一致(单人模式以及带 #include/#define/宏的多人模式预处理器),并逐字显示游戏的警告;未修改时保存的内容逐字节相同,移动或调整项的大小只改变其 rect 数字。
- 预览按游戏方式绘制(字体、着色器、项类型),页面(包括 ui_tab 页面)显示为标签页,交互模式运行菜单脚本,cvar 面板只影响预览。
- 代码编辑器支持配色、小地图、折叠、多光标、正则查找/替换、自动补全、跳转到定义、书签以及与脚本编辑器相同的修改边栏;可导出为 .menu 或带自定义图像的 .pk3(绝不写入游戏文件夹)。

## 日本語
- 新しい作業領域 **メニューエディター**:Jedi Academy、Jedi Outcast、Movie Duels のメニュー(.menu)をテキストと 640x480 のキャンバスで同時に編集できます。ファイルはゲームとまったく同じように読み込み(シングルプレイと、#include/#define/マクロに対応したマルチプレイのプリプロセッサー)、ゲームの警告を原文のまま表示します。変更せずに保存するとバイト単位で同じになり、項目の移動やサイズ変更では rect の数値だけが変わります。
- プレビューはゲームと同じように描画(フォント、シェーダー、項目の種類)し、ui_tab のページを含むページをタブで表示します。操作モードではメニューのスクリプトが動き、cvar パネルはプレビューだけに影響します。
- コードエディターは配色、ミニマップ、折りたたみ、マルチカーソル、正規表現の検索/置換、補完、定義へ移動、ブックマーク、スクリプトエディターと同じ変更マーカーに対応。自作の画像と一緒に .menu または .pk3 としてエクスポートできます(ゲームのフォルダーには書き込みません)。
