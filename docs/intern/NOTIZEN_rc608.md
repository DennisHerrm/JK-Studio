# Notizen rc608

## Teil 1

### Punkt 1: Startbildschirm des Menue-Editors
- Ohne offenes Menue zeigt der Menue-Editor jetzt dieselbe Seite wie der Mission-Tab: Logo, Titel, Text, Karten, Zeile "Neu hier?". Die Bauteile sind dieselben Funktionen (`startKopf`, `startKarten`, `startKnopfZeile` in `gui/app_mission.cpp`), `drawStartflaeche` baut sich ebenfalls daraus.
- Die Bauteile koennen jetzt, was der Menue-Editor brauchte und der Mission-Tab auch gebrauchen kann: lange Titel brechen um, Karten sind so hoch wie ihr Text, im schmalen Fenster stehen sie untereinander, Knopfzeilen brechen um, bei wenig Hoehe (unter 24 Schrifthoehen) entfaellt das Logo.
- Einzelspieler/Mehrspieler steht ueber den Karten mit einem Satz, was die Wahl festlegt: Ordner (`startMenuePfad`), Format (SP-Namen, MP-Zahlen ueber menudef.h, NOTIZEN_menue1) und die vom Spiel gezeichneten Inhalte (Ownerdraws und Feeder je Dialekt).
- Befund: Der Satz "open a menu from the game with the first button above" war falsch. Der erste Knopf der Leiste ist "Projekt", "Aus dem Spiel" der zweite. Text in allen 6 Sprachen auf "zweiter Knopf" gestellt, der Selbsttest prueft die Reihenfolge der Knoepfe.
- "Neues Projekt" und "Projekt oeffnen" laufen ueber `neuesProjektWaehlen`/`projektOeffnenWaehlen`, gemeinsam fuer Leiste und Startbildschirm. Der Selbsttest ersetzt den Windows-Dialog (`setzeProjektDialog`), sonst waere er nicht ohne Fenster zu pruefen gewesen.
- Tour-Anker: `knopf:MePrjNeu` (tut-menue-eigen, Schritt 2) trifft die Karte, `knopf:MeAusSpiel` (tut-menue, Schritt 2) die Leiste; beides prueft der Selbsttest. Helfer-Orte: unveraendert (195), der Startbildschirm stand nie im Katalog.
- Prueflauf `BHED_MENUE_NUR=start`: 6 Sprachen x (100 %, 200 %) x (grosses, kleinstes erlaubtes Fenster), je Rollen von oben nach unten: jedes Bauteil einmal ganz im Bild, nichts ragt seitlich hinaus, Karten fassen ihre Texte. Rot vorher (kein Bauteil im Fenster `##menuestart`), gruen 30/30.
- Eine Liste "Zuletzt bearbeitet" wie im Mission-Tab gibt es fuer Menue-Projekte nicht, weil das Programm keine fuehrt. Nicht erfunden; waere ein eigener Punkt.

### Punkt 2: Unbekannte Schalter
- Ursache: `argv[1]` galt immer als Datei (`main_win32.cpp`, fuenf eigene Auswertungen). Jetzt `bhed/befehlszeile.h` mit ctest `befehlszeile`: rot 8 Fehler (`--version` -> Datei), gruen.
- Unbekanntes mit `--` wird nie geoeffnet, das Programm geht nicht auf, die Meldung nennt den Schalter, die bekannten Schalter und den Weg ueber einen Pfad (`.\--name`). `--version` nennt die Fassung. Ausgabe in die Umleitung, sonst in die Konsole des Aufrufers, sonst ein Meldungsfenster (nie mit `BHED_KEIN_MELDUNGSFENSTER`).
- Die echte exe wurde nach Auftrag nicht mit diesen Schaltern gestartet: Geprueft ist die Auswertung, nicht die Ausgabe in die Konsole.

### Punkt 6: Auswaehlen und Ziehen
- Der erste Druck waehlt nur aus, auch wenn die Maus wandert. Erst ein neuer Druck auf Gewaehltes zieht, ab einer Schwelle von einer Viertel-Schrifthoehe; ein Griff genauso.
- Strg/Umschalt nehmen nur hinzu oder heraus. Ein Druck auf ein Element der Auswahl zieht die ganze Auswahl; ein Klick ohne Ziehen laesst nur dieses gewaehlt.
- Ueberlappung: Ein Druck meint das vorderste Element, das mit Rahmen und Namen unter dem Zeiger. Alt+Klick geht beim Loslassen durch den Stapel wie bisher; neu: Alt+Ziehen bewegt das Gewaehlte auch verdeckt. Sonst waere ein verdecktes Element nach der neuen Regel nur noch mit den Pfeiltasten zu bewegen.
- Prueflauf `BHED_MENUE_NUR=ziehen`: rot 24 Fehler (frei sprang beim ersten Druck auf 429,312), gruen 26/26.

### Tester: "when I click off, it resets"
- Nachgestellt: Das Ziehen auf der Flaeche blieb immer stehen (MD, JKA, neu, je Dialekt). Der Fehler sass in Properties: `float r[4]` wird in jedem Bild neu aus dem Text gelesen. Ziehen an `rect` behielt nur das letzte Bild (rot 404 statt 444), getippte Zahlen und Texte gingen beim Klick daneben verloren (nur Enter schrieb), und ein Klick auf die Flaeche nahm dem Zugfeld die Aktivitaet, bevor es seinen Wert abliefern konnte.
- Behoben mit einem Halteplatz (`zahlenFeld`, `textFeld`, `haltPruefen`) in Properties, Events und Layout: Der Wert lebt dort, bis er fertig ist, dann ein Schritt, und zwar bevor der Klick Auswahl oder Reiter wechselt. Esc verwirft.
- `Dokument::altStaende`: `neuParsen` gab mitten im Bild die alte `Datei` frei, die Panels lasen danach aus freigegebenem Speicher weiter. Sie lebt jetzt bis zum naechsten Bild.
- Kann ein Zug nicht geschrieben werden (`rect` aus einem Makro), kommt eine Einblendung statt der grauen Zeile in der Leiste.
- Prueflauf `BHED_MENUE_NUR=bleiben`: Ziehen, Klick daneben, alle Reiter, Fokus Text-Editor und Properties, Speichern und neu Laden, auf Originalen und neuen Menues. JK2-Menues nur mit `CFG=m5b/behaved_jk2.cfg`.

### Punkt 0: Undo
- Rot: Strg+Z ging in keinem einzigen Fall der Tabelle (41 von 91 Zeilen rot, dann Absturz), nur die Pfeile der Leiste, und auch die nicht mit offenem Umbenennen-Feld.
- Ursachen: Tasten nur mit Fokus auf der Flaeche und mit Auswahl; das Umbenennen-Feld holte sich in jedem Bild den Fokus zurueck und sperrte damit alles; sein Index galt ueber Reiter hinweg (daher "description" auf der falschen Zeile).
- Absturz: Rueckgaengig nach dem Einfuegen liess die Auswahl auf dem entfernten Element stehen. `neuParsen` prueft die Auswahl jetzt gegen das Menue.
- Neu: `editorTasten` im ganzen Menue-Editor vor den Panels. Ausnahmen: echte Eingabefelder, Text-Editor (gleicher Puffer, eigene Tasten, wirkt erst vor dem naechsten Bild), offene Fenster. Pfeiltasten, Entf, Strg+D/C/V gelten, wo kein Panel mit Werten den Fokus hat.
- Seite und Auswahl gehoeren zum Schritt (`Textpuffer::beiwerkFesthalten`, ctest `menue_text`). Einfuegen aus der Bibliothek war bis zu drei Schritte, jetzt einer.
- Umbenennen (auch Punkt 6 aus Teil 2, damit erledigt): nimmt den Fokus nur am Anfang, endet mit Enter, Esc (ohne Aenderung), Klick daneben, Auswahl- und Reiterwechsel; Getipptes zaehlt, und es trifft das richtige Menue.
- Prueflauf `BHED_MENUE_NUR=undo`: Tabelle 15 Aktionen x 8 Wege, Zeilen `Knopftest: Undo-Tabelle | ...` im Protokoll.
- Das Auge und das Schloss sind kein Schritt: Sie stehen nicht in der Datei.

### Punkt 0b: Typ
- Ursache: Die Schluessel des Modells gelten bei einer Checkbox nicht. `Menu_Parse` liefert false, und `Menu_New` verwirft im Spiel das ganze Menue (OpenJK `code/ui/ui_shared.cpp` und `codemp/ui/ui_shared.c`: kein `Menu_PostParse`, `menuCount` bleibt). Der Leser des Editors hoerte dort ebenfalls auf und liess die Bildschirmlage aus; darum lagen alle Elemente auf 0,0. Die Datei selbst aenderte sich nur in der `type`-Zeile, Rueckgaengig stellte alles her.
- `type` ist in Properties nur noch Anzeige mit Tooltip. `style` und `textalign` koennen ein Menue so nicht zerstoeren (geprueft, alle Werte, beide Dialekte), sie bleiben Auswahl.
- `LeseOptionen::weiterlesen` (nur der Editor): gleiche Meldungen, gleiches Urteil (`Menu::ok` false), aber das Lesen geht in der naechsten Zeile weiter und die Lage stimmt. Ueber der Flaeche steht rot, dass das Spiel das Menue verwerfen wuerde, mit Knopf zur Zeile. Das Lesen wie im Spiel bleibt der Vorgabewert (ctest).
- Prueflauf `BHED_MENUE_NUR=typ`: alle 15 Typen, SP und MP, rot (alle auf 0,0), gruen 88/88.

### Punkt 3: Selbsttest update
- Die Pruefung sucht jetzt `<laufende exe>.alt` und die laufende exe, `behaved.exe` einzeln als Starter. Rot: `alt nein` (Stand 5, 4968 Dateien). Gruen: `alt ja, exe ja, Starter ja`, je einmal mit dem echten rc607 (rund 55 s).

### Punkt 4: Selbsttest Zwischenablage
- Ursache bestaetigt: Das Lesen danach (`echtLesen`) hatte keine Wiederholung; ImGui liefert nichts, solange ein anderes Programm die Ablage offen hat.
- Nachstellen war schwerer als gedacht: `OpenClipboard(NULL)` sperrt andere gar nicht, erst ein Oeffnen mit Fenster. Und auf Zufall gewinnt der Mitleser fast nie. Darum `SP/ab/stoerer.py leser` mit Fenster, gerufen genau nach dem Zurueckschreiben (`platform::zwischenablageStoererRufen`, ohne Stoerer ohne Wirkung). Rot: "danach 0 Bytes", wie am 31.10. Gruen: "danach 85 Bytes". Die Pruefung ist nicht aufgeweicht: Der Inhalt muss weiter gleich sein.
- Die Probe laeuft nur mit `BHED_ABLAGE_PROBE=1` (TESTING.md, Release builds). Ohne Schalter schreibt kein Selbsttest in die Ablage; die Pruefung "liess die Ablage in Ruhe" laeuft immer.

### Punkt 5: JK2 im laufenden Spiel
Steam war angemeldet, vor jedem Start geprueft. `SP/rc608/jk2spiel.py`: Fenster rechts ausserhalb aller Bildschirme, kein Fokus, stumm, in_mouse 0, geschrieben nur nach `Y:\menue_log\rc608jk2`. Der Spielordner ist danach unveraendert (Vergleich der Dateiliste). Leere Konfigs im eigenen Ordner gehen vor denen in GameData: Verglichen wird der Stand des Spiels, nicht shanks Einstellungen.

Wege, die vorher nicht gingen:
- Jedes JK2-MP-Menue: loses `ui/jk2mp/main.menu` im eigenen Ordner, dessen onOpen das Menue oeffnet. Die Ingame-Menues laedt das Spiel sonst erst in einer Partie, darum ein loses `ui/jk2mpmenus.txt` mit ihnen. Gleich fuer SP.
- Datapad: `kejim_base` und `bespin_platform` starten ohne Zwischensequenz davor (kejim_post und yavin_swamp nicht).

Alle 35 JK2-MP-Menues und 17 SP-Menues plus Datapad gegen die Vorschau. Gefunden und behoben (jeweils "rot" auf den Kontaktbogen, ctest wo moeglich):
- Texte: JK2 legt sie in `MENUS0..3` und `INGAMETEXT` ab, die Vorschau fragt nach den Namen von JKA. Darum leer: Ja/Nein ("Lightsaber Only:"), Spieltyp, Skill, Force Mastery, "Server Refresh Time". `jk2Namen` in `spieldaten.cpp`. HUMAN/CLOSED sind in ui.qvm feste Woerter.
- Cvars: Die Vorschau nahm fuer JK2-MP die Werte von JKA ("*Jedi*" statt "noname", ui_netSource leer, darum "Add Favorite" und "Del Favorite" aufeinander). Neu: `cvarvorgaben_jk2.inc`, erzeugt aus dem laufenden Spiel (`cvarlist`), `tools/gen_menue_cvarvorgaben_jk2.py`.
- `uiScript getvideosetup` fehlte in der Vorschau: Die Setup-Seiten zeigten "High Quality", "Off" statt "Custom", "On" (gilt fuer alle Spiele, UI_GetVideoSetup).
- `uiScript updateForceStatus` fehlte: In ingame_player standen "The Force" und "Force Disabled", Apply und Join Game aufeinander. Portiert fuer den Zustand ohne Partie; jk2mp legt dabei ui_about_gametype an.
- Saebelbild: jk2mp zeichnet `menu/art/saber_<farbe>` ueber das ganze Feld, die Nummer ist color1 (0 rot bis 5 lila, im Spiel je Wert nachgesehen). Die Vermutung aus M7 (fx_base mit fx_*, Q3-Erbe) war falsch: ui.qvm laedt sie, zeichnet sie aber nicht.
- Kartenliste leer: JK2 hat seine Karten in `scripts/arenas.txt`.
- Listen abgeschnitten ("Bes" statt "Bespin Streets"): Die Grenze beim Text zaehlt in jk2mp Zeichen (Q3), in JKA Bildpunkte.
- Tastennamen: "R or MWHEELDOWN" (Spiel) statt "R  MWHEELDOWN".
- Name der gewaehlten Figur unter der Kopfliste ("rebel/default"): in JKA steht das in `#if`, jk2mp zeichnet es.
- Force Mastery beim Server: g_maxForceRank (Jedi Knight), nicht der Rang des Spielers.

Kein Unterschied, nur der Augenblick: die Griffe mit dem Ring (im Spiel dreht er sich genauso, Zeitreihe `SP/rc608/jk2/reihe.png`). Kein Vergleich moeglich: `Video Mode` (der Lauf setzt r_mode -1), Spielwerte im Datapad (Beispielwerte der Vorschau), Text und Werte in videodriver (Treiber).

## Offen
Aus dem JK2-Vergleich, nicht behoben (Kontaktboegen in `SP/rc608/jk2/`):
- Datapad: Die ausgeschnittenen Knopfflaechen (WEAPONS, INVENTORY, FORCE) sind in der Vorschau grau mit Verlauf, im Spiel schwarz. `datapad2.tga` ist dort voll durchsichtig (Alpha 0), `frame_pic` zeichnet schwarz davor; was das Grau zeichnet, ist nicht gefunden.
- Datapad: Der Text bricht eine Spur frueher um als im Spiel ("... energy blade / of great" statt "... energy blade of / great").
- Datapad Force-Seite: Nachgeholt auf kejim_base; im Spiel ohne Kraefte leer, in der Vorschau mit Beispielkraeften. Rahmen gleich, die Kraefte selbst sind so nicht verglichen.
- jk2mp Listen: Die Zeilen stehen im Spiel rund 2,7 virtuelle Punkte hoeher (createserver, Kartenliste). Ursache nicht gefunden, darum nicht geschoben.
- jk2mp createserver: "CLOSED" ist im Spiel dunkler als in der Vorschau.
- jk2mp ingame: Die Leiste ist auf dem Foto weiss verrauscht, weil das Menue ohne Partie geoeffnet wurde; im Spiel so nicht pruefbar ohne Tastendruck (kein Fokus).
- JK2-Ingame-Menues mit Spielzustand (Abstimmung, Spielerliste) sind ohne Partie verglichen.

Weiter offen:
- Die Ausgabe von `--version` und unbekannten Schaltern ist nach Auftrag nicht mit der echten exe gelaufen (Konsole, Umleitung, Meldungsfenster).
- Liste "Zuletzt bearbeitet" fuer Menue-Projekte: gibt es nicht, waere ein eigener Wunsch.
- Teil 2, Punkt 6 (Umbenennen) ist hier erledigt, die uebrigen Punkte von Teil 2 nicht begonnen.

## English
- Menu editor: new start screen in the style of the Mission tab, with the format choice explained.
- Menu editor: a first click only selects an element; a new press on it drags it, so a click no longer shifts anything. Alt+click goes through overlapping elements, Alt+drag moves the selected one even when covered.
- Menu editor: undo and redo work everywhere (Ctrl+Z, Ctrl+Y and the arrows) and also restore the selection. Undoing an insert no longer crashes.
- Menu editor: values changed in Properties, Events and Layout stay when you click elsewhere. Dragging rect in Properties works.
- Menu editor: an element keeps its type. A wrong line in the text no longer throws all elements to 0,0; the editor says which line would make the game drop the menu.
- Menu editor: rename in the hierarchy ends when you click elsewhere and renames the right element.
- Menu editor: all Jedi Outcast multiplayer menus compared with the running game: texts, values, saber picture, map list, force templates and setup pages now match.
- Command line: unknown switches are no longer opened as files; --version names the version.

## Deutsch
- Menü-Editor: neuer Startbildschirm im Stil des Mission-Tabs, die Formatwahl ist erklärt.
- Menü-Editor: Der erste Klick wählt ein Element nur aus, erst ein neuer Druck zieht es; ein Klick versetzt nichts mehr. Alt+Klick geht durch überlappende Elemente, Alt+Ziehen bewegt das gewählte auch verdeckt.
- Menü-Editor: Rückgängig und Wiederholen wirken überall (Strg+Z, Strg+Y und die Pfeile) und stellen auch die Auswahl wieder her. Rückgängig nach dem Einfügen stürzt nicht mehr ab.
- Menü-Editor: Werte in Properties, Events und Layout bleiben, wenn du daneben klickst. Ziehen an rect in Properties geht.
- Menü-Editor: Ein Element behält seinen Typ. Eine falsche Zeile im Text wirft nicht mehr alle Elemente auf 0,0; der Editor nennt die Zeile, wegen der das Spiel das Menü verwerfen würde.
- Menü-Editor: Umbenennen in der Hierarchie endet beim Klick daneben und trifft das richtige Element.
- Menü-Editor: alle Mehrspieler-Menüs von Jedi Outcast mit dem laufenden Spiel verglichen: Texte, Werte, Säbelbild, Kartenliste, Force-Vorlagen und Setup-Seiten stimmen jetzt.
- Befehlszeile: Unbekannte Schalter werden nicht mehr als Datei geöffnet, --version nennt die Fassung.

## 中文
- 菜单编辑器:新的起始界面,风格与任务标签页一致,并说明了格式选择。
- 菜单编辑器:第一次单击只选中元素,再次按住才会拖动,单击不再造成移动。Alt+单击可在重叠元素间切换,Alt+拖动可移动被遮挡的已选元素。
- 菜单编辑器:撤销和重做随处可用(Ctrl+Z、Ctrl+Y 和箭头),并会恢复选择。插入后撤销不再崩溃。
- 菜单编辑器:在属性、事件和布局中更改的值在点击别处后会保留。可以在属性中拖动 rect。
- 菜单编辑器:元素保持其类型。文本中的错误行不再把所有元素移到 0,0;编辑器会指出导致游戏丢弃菜单的那一行。
- 菜单编辑器:层级中的重命名在点击别处时结束,并作用于正确的元素。
- 菜单编辑器:所有 Jedi Outcast 多人菜单已与运行中的游戏对比:文本、数值、光剑图片、地图列表、原力模板和设置页现在一致。
- 命令行:未知开关不再被当作文件打开,--version 显示版本。

## 日本語
- メニューエディター: ミッションタブと同じスタイルの新しいスタート画面。形式の選択に説明を付けました。
- メニューエディター: 最初のクリックは選択だけで、もう一度押すとドラッグします。クリックで動くことはなくなりました。Alt+クリックで重なった要素を順に選び、Alt+ドラッグで隠れた選択済み要素も動かせます。
- メニューエディター: 元に戻す・やり直しがどこでも働き(Ctrl+Z、Ctrl+Y、矢印)、選択も戻します。挿入後に元に戻しても落ちなくなりました。
- メニューエディター: プロパティ、イベント、レイアウトで変えた値は、別の場所をクリックしても残ります。プロパティの rect のドラッグも動きます。
- メニューエディター: 要素の型は変わりません。テキストの誤った行で全要素が 0,0 に飛ぶことはなくなり、ゲームがメニューを破棄する原因の行を示します。
- メニューエディター: 階層での名前変更は別の場所のクリックで終わり、正しい要素に効きます。
- メニューエディター: Jedi Outcast のマルチプレイメニューをすべて実際のゲームと比較し、テキスト、値、セイバー画像、マップ一覧、フォーステンプレート、設定ページを一致させました。
- コマンドライン: 不明なスイッチをファイルとして開かなくなり、--version でバージョンを表示します。

## Español
- Editor de menús: nueva pantalla de inicio al estilo de la pestaña Misión, con la elección de formato explicada.
- Editor de menús: el primer clic solo selecciona; otra pulsación arrastra, así que un clic ya no mueve nada. Alt+clic recorre los elementos superpuestos y Alt+arrastrar mueve el seleccionado aunque esté tapado.
- Editor de menús: deshacer y rehacer funcionan en todas partes (Ctrl+Z, Ctrl+Y y las flechas) y restauran la selección. Deshacer tras insertar ya no bloquea el programa.
- Editor de menús: los valores de Propiedades, Eventos y Diseño se conservan al hacer clic fuera. Arrastrar rect en Propiedades funciona.
- Editor de menús: un elemento conserva su tipo. Una línea errónea ya no lleva todos los elementos a 0,0; el editor indica la línea por la que el juego descartaría el menú.
- Editor de menús: renombrar en la jerarquía termina al hacer clic fuera y afecta al elemento correcto.
- Editor de menús: todos los menús multijugador de Jedi Outcast comparados con el juego en marcha: textos, valores, imagen del sable, lista de mapas, plantillas de la Fuerza y páginas de configuración ya coinciden.
- Línea de comandos: los interruptores desconocidos ya no se abren como archivos; --version indica la versión.

## Čeština
- Editor menu: nová úvodní obrazovka ve stylu karty Mise, volba formátu je vysvětlena.
- Editor menu: první kliknutí prvek jen vybere, teprve nový stisk ho táhne, takže kliknutí už nic neposune. Alt+kliknutí prochází překrývajícími se prvky, Alt+tažení posune vybraný prvek, i když je zakrytý.
- Editor menu: zpět a znovu fungují všude (Ctrl+Z, Ctrl+Y a šipky) a obnovují i výběr. Zpět po vložení už program neshodí.
- Editor menu: hodnoty ve Vlastnostech, Událostech a Rozložení zůstanou po kliknutí jinam. Tažení rect ve Vlastnostech funguje.
- Editor menu: prvek si zachovává svůj typ. Chybný řádek už nepřesune všechny prvky na 0,0; editor uvede řádek, kvůli kterému by hra menu zahodila.
- Editor menu: přejmenování v hierarchii končí kliknutím jinam a týká se správného prvku.
- Editor menu: všechna menu pro více hráčů Jedi Outcast porovnána s běžící hrou: texty, hodnoty, obrázek meče, seznam map, šablony Síly a stránky nastavení nyní souhlasí.
- Příkazový řádek: neznámé přepínače se už neotevírají jako soubory, --version uvede verzi.
