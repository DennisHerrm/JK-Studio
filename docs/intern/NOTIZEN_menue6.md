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

## Changelog

## English

- Menu editor: menu flow - see which menu opens which, which menus nobody reaches, dead ends and menus without a way back; click to jump there.
- Menu editor: tab order - numbers show the keyboard order as the game uses it; click the elements in a new order to rearrange it.
- Menu editor: styles - all colours, fonts, text sizes and backgrounds of a file at a glance, changed everywhere at once.
- Menu editor: templates for whole menus (main menu, settings page with tabs, credits, loading screen) and a settings wizard that knows the game's own sliders, yes/no switches and choices.
- Menu editor: reference image under the canvas, compare an element with the game's original and reset it, live limits (elements, menus, script length) while building.
- Menu editor: readability hints - text too small, too little contrast, translations that do not fit their field.
- Menu editor: closing a tab with unsaved changes asks first.

## Deutsch

- Menü-Editor: Menüfluss - sehen, welches Menü welches öffnet, welche Menüs man nie erreicht, Sackgassen und Menüs ohne Weg zurück; Klick springt dorthin.
- Menü-Editor: Tab-Reihenfolge - Nummern zeigen die Tastatur-Reihenfolge wie im Spiel; Elemente in neuer Reihenfolge anklicken stellt sie um.
- Menü-Editor: Stile - alle Farben, Schriften, Textgrößen und Hintergründe einer Datei auf einen Blick, überall auf einmal geändert.
- Menü-Editor: Vorlagen ganzer Menüs (Hauptmenü, Einstellungsseite mit Reitern, Credits, Ladebildschirm) und ein Einstellungs-Assistent, der die Schieber, Ja/Nein-Schalter und Auswahlen des Spiels kennt.
- Menü-Editor: Referenzbild unter der Fläche, Element mit dem Original vergleichen und zurücksetzen, Live-Grenzen (Elemente, Menüs, Skriptlänge) beim Bauen.
- Menü-Editor: Lesbarkeits-Hinweise - zu kleine Schrift, zu wenig Kontrast, Übersetzungen, die nicht ins Feld passen.
- Menü-Editor: Reiter mit ungespeicherten Änderungen fragt beim Schließen nach.

## 中文

- 菜单编辑器:菜单流程——查看哪个菜单打开哪个菜单、哪些菜单无法到达、死胡同以及无法返回的菜单;单击即可跳转。
- 菜单编辑器:Tab 顺序——编号显示游戏中的键盘顺序;按新顺序单击元素即可重新排列。
- 菜单编辑器:样式——一览文件中的所有颜色、字体、文字大小和背景,一次性全部修改。
- 菜单编辑器:整个菜单的模板(主菜单、带标签页的设置页、制作人员名单、加载画面)以及了解游戏自身滑块、是/否开关和选项的设置向导。
- 菜单编辑器:画布下的参考图、元素与游戏原版比较并重置、搭建时的实时上限(元素、菜单、脚本长度)。
- 菜单编辑器:可读性提示——文字太小、对比度不足、译文放不进字段。
- 菜单编辑器:关闭含未保存更改的标签页时会先询问。

## 日本語

- メニューエディター: メニューの流れ - どのメニューがどれを開くか、たどり着けないメニュー、行き止まり、戻れないメニューを表示し、クリックで移動。
- メニューエディター: Tab の順序 - ゲームと同じキーボード順を番号で表示し、新しい順にクリックすると並べ替え。
- メニューエディター: スタイル - ファイルのすべての色、フォント、文字サイズ、背景を一覧し、一度に全部変更。
- メニューエディター: メニュー全体のテンプレート(メインメニュー、タブ付き設定ページ、クレジット、ロード画面)と、ゲーム本来のスライダー、はい/いいえ、選択肢を知る設定ウィザード。
- メニューエディター: キャンバスの下の参考画像、要素をゲームのオリジナルと比較して戻す機能、作成中の上限表示(要素、メニュー、スクリプトの長さ)。
- メニューエディター: 読みやすさのヒント - 小さすぎる文字、足りないコントラスト、フィールドに収まらない訳文。
- メニューエディター: 保存していない変更のあるタブを閉じるときに確認。
