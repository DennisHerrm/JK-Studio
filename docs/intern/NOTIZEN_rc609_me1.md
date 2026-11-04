# Notizen rc609 ME1: Menü-Editor - Löschen eines Buttons, "Menü öffnen" auf dem Startbildschirm

Auftrag `auftrag_rc609_me1.md` (Tester über shank, 04.11.: "it crashed when I deleted a button", "not sure why the open menu button isn't displayed nicely with the create and open project buttons").

## ME1: Löschen in der Mitte des Menüs

### Befund (Selbsttest `menueloeschen`, Movie Duels und JKA)
- **Absturz:** Entf, während die Maus noch zieht (Skalieren am Griff). Das Loslassen las `d.auswahl[0]` der eben geleerten Auswahl (`app_menue.cpp`, Zweig `Ziehen::Groesse` beim Loslassen, ohne Prüfung). Im normalen Release-Bau liest das hinter den Puffer (das alte Element steht noch im Speicher, `setzeRects` verwirft die Nummer) und stürzt nur je nach Speicherlage ab; mit STL-Härtung (`/D_MSVC_STL_HARDENING=1`, eigener Bau `build-menuefix-h`) bricht das Programm genau dort ab: **rot** `me1_hrot2_md`, exit 127 im Schritt "am Griff ziehen, Entf". Kein Dump: `__fastfail` geht am Absturzfilter von `wache_win32.cpp` vorbei; die Stelle ist die einzige ungeprüfte Nummer in diesem Pfad. Ebenso ungeprüft: Klick ohne Ziehen in eine Mehrfachauswahl + Entf (`klickWahl` setzte beim Loslassen eine gelöschte Nummer), `gizmoVorschau` (Modell-Griff + Entf).
- **Falsche Merker (Verdacht des Hauptchats bestätigt):** Schloss und Auge zogen über Name und Vorkommen mit (`elementImNeuen`). Unbenannte Elemente verloren sie beim Löschen davor, ein doppelter Name gab sie an das falsche Element oder verlor sie, `hierarchieAnker` und `umbenennen` blieben auf der alten Nummer (Umbenennen hätte das nächste Element umbenannt). Rot in allen sechs Probe-Wegen, z. B. Entf auf der Fläche: soll `||T4|z ... ||T8|v`, ist `||T4 ... ||T8` (`me1_rot_md`, `me1_rot_ja`: je 12 FEHLER der Merker).
- **Rückgängig vertauschte gelöschte Nachbarn:** Zwei benachbarte Elemente sind nach dem Löschen zwei Einfügungen an derselben Stelle; `Textpuffer::rueckgaengig` sortierte nach der Stelle (instabil) und setzte sie vertauscht ein. Rot `me1_gr_md/ja` (Umschalt-Auswahl, Umschalt in die Gruppe: Text nach Rückgängig nicht byte-gleich, T9 vor T8).
- In echten Menüs (JKA/Movie Duels `ui/main.menu`, `ui/ingame.menu`, Mehrspieler `ui/jamp/main.menu` mit `#include`) über Fläche, Leiste, Rechtsklick in der Hierarchie, Strg+X mit Events, Entf im Bedienmodus: kein Absturz, auch nicht im gehärteten Bau des alten Codes (`me1_hrot_md/ja`), Merker stimmten dort (Elemente mit eigenem Namen).

### Behebung
- `Textpuffer::stelleSeit` (neu): Jede Version merkt sich ihre Spannen (`wege_`, 1024 Schritte); eine Stelle des Texts wandert damit genau mit, auch über Rückgängig/Wiederholen. `nachfolger()` ordnet jedem alten Element sein neues über die Stelle seines `itemDef` zu; nur ersetzte oder eingebundene suchen Name und Vorkommen, nie eines, das schon einen Vorgänger hat. Schloss, Auge, Hierarchie-Anker und Umbenennen ziehen darüber mit (`merkerNachziehen`); was gelöscht ist, verliert sie.
- Schloss und Auge stehen im Beiwerk jedes Rückgängig-Schritts (`anzeigeStand`): Ein Element, das mit Rückgängig wiederkommt, bekommt sie zurück. Die Auswahl aus dem Beiwerk wird gegen die Größe geprüft.
- `rueckgaengig`/`wiederholen` wenden die Spannen in ihrer gespeicherten Folge von hinten an (wie `anwenden`), `anwenden` sortiert stabil.
- `loescheAuswahl` beendet einen laufenden Zug (Ziehen, Griff, Klickwahl, Modell-Griff). Geprüft gegen die Größe: Loslassen nach Skalieren/Verschieben/Klickwahl, `gizmoVorschau`, `zeichneEbene`, `nameSetzen` (Rückruf eines Felds nach dem Löschen), `zeichneKomponente`, `sperreItem`/`verbergeItem`.

### Tests
- Selbsttest `menueloeschen` (neu, `gui/selbsttest_menue_loeschen.inc`): Probe mit zehn Elementen (unbenannte, doppelter Name, Gruppe), Schloss/Auge gesetzt; Entf auf der Fläche, Entf in der Hierarchie, Rechtsklick > Delete, Knopf der Leiste bei offenen Properties, Umschalt-Auswahl über ein gesperrtes Element, Umschalt-Auswahl in die Gruppe; Entf beim Skalieren, beim Verschieben, beim Klick in die Mehrfachauswahl; echte Menüs wie oben. Nach jedem Löschen: genau die Auswahl ohne Gesperrte ist weg, jedes übrige Element hat sein Schloss/Auge; Rückgängig byte-gleich mit den Merkern von vorher, Wiederholen wie nach dem Löschen. Dazu der Startbildschirm (unten).
  - Rot: `me1_rot_md` 34/16, `me1_rot_ja` 33/18 (Merker, Testfehler der Klickwege), `me1_gr_md/ja` 82/2 (Rückgängig der Nachbarn), `me1_hrot2_md` exit 127 (Absturz beim Skalieren).
  - Grün: `me1_gr3_md` 96/0, `me1_gr3_ja` 96/0, gehärtet `me1_hgr_md` 96/0, `me1_hgr_ja` 96/0.
- ctest `menue_text`: Nachbarn kommen mit Rückgängig in ihrer Folge zurück, `stelleSeit` (weg, nachrückend, Rückgängig, vor `setzeText` unbekannt).

## ME1: "Menü öffnen" auf dem Startbildschirm
- Dritte Karte **Menü öffnen** neben "Neues Projekt..." und "Projekt öffnen..." (gleich groß, in ihrer Reihe; schmal untereinander wie die anderen). Sie öffnet eine Auswahl: Aus dem Spiel öffnen (die Liste der Spielmenüs), Datei öffnen... (Platte, auch Mod-Ordner), Ganzen Menüsatz öffnen. Die Knöpfe oben in der Leiste bleiben.
- Ohne Projekt: bearbeiten und Strg+S. Eine Datei außerhalb der Spielordner wird an ihren Ort zurückgeschrieben (wie bisher `speichernWunsch`); ein Menü aus dem Spiel (oder aus einem Spiel-/Mod-Ordner unter GameData) fragt einmal nach dem Ort der Kopie, danach speichert Strg+S dorthin. Die Wege aus W2a (in die .pk3 schreiben, lose daneben) sind nicht übernommen: Der Menü-Editor schreibt nach seiner Regel nie in Spielordner (`imSpielordner`, `MeNieSpielordner`), und eine lose Datei neben der .pk3 wirkt im Spiel nicht (fs_dirbeforepak 0, Befund W2a) - siehe Offen.
- Test (im Modus `menueloeschen` und als `BHED_MENUE_NUR=startoeffnen`): Karte in der Reihe und so groß wie die Projektkarten, Leistenknöpfe da, Klick > Datei öffnen... öffnet die Probe ohne Projekt, Löschen + Strg+S schreibt die Datei an ihren Ort (Inhalt gleich dem Puffer), Klick > Aus dem Spiel öffnen > `ui/main.menu` öffnet das Spielmenü. Der rc608-Starttest (`startAbtasten`, alle Sprachen, 100/200 %, kleinstes Fenster) zählt die dritte Karte mit (7 Bauteile).
- Handbuch: Seite Menü-Editor, Abschnitt "Ein Menü öffnen": Absatz "Ohne Projekt" und Bild `menue_start_oeffnen` (2538 breit, je Sprache aus `BHED_MENUE_NUR=startoeffnen BHED_MENUE_BILDER=1`), 6 Sprachen. Texte in `tools/i18n_rc609_me1.py`.

## Schluss (einmal, parallel)
- Bau aller Ziele 0 Warnungen; ctest 213/213 (`handbuch_sprachen` war im Lauf rot, weil Bilder und Handbuch erst danach entstanden; danach grün).
- Lints: lint_sources, lint_fassung, lint_i18n, lint_i18n_erzeugt, lint_headers, lint_feldinit, lint_gapp, lint_settings, lint_imgui_context, lint_doppelanzeige ok; lint_tabellenid 1 Fund in `app_kartemission.cpp` (nicht berührt, schon im Stand 8f2cfb48); lint_portability 112 (keiner in geänderten Zeilen). `hilfe_orte.inc` unverändert.
- Selbsttests: `menueeditor` Movie Duels 2775/0 (`me1_me2_md`), JKA (`nur_ja.cfg`) 2775/0 (`me1_me2_ja`), `mission1` 98/0, Handbuchbilder 6 Sprachen je 5/0. Der erste `menueeditor`-Lauf hatte 33 Tour-Fehler, weil der Laufordner ausserhalb des Worktrees lag und `touren.json` fehlte (Testumgebung); mit `BHED_HANDBUCH` gruen.

## Offen
- Speichern eines Menüs aus dem Spiel an seinen Ort (.pk3 ersetzen wie W2a Weg 1) bewusst nicht: Widerspricht der Regel "schreibt nie in Spielordner"; Entscheidung bei shank.
- Ein Absturz-Dump nach `__fastfail` (STL-Härtung) entsteht nicht; im ausgelieferten Bau ist die Stelle ein Lesen hinter den Puffer, ob es beim Tester genau dieser Weg war, ist nicht belegt (er nannte nur "deleted a button").

## English
- Menu editor: deleting while the mouse is still dragging (resizing at a handle, moving, clicking into a multiple selection) could crash; the drag now ends with the delete.
- Menu editor: lock and eye stay with their elements when something before them is deleted, also for unnamed elements and duplicate names; Undo gives a deleted element its lock and eye back.
- Menu editor: Undo after deleting neighbouring elements restores them in their original order.
- Menu editor: the start screen has "Open menu" next to "New project..." and "Open project...": open a menu from the game, a file or a whole menu set, edit it and save it without a project.

## Deutsch
- Menü-Editor: Löschen, während die Maus noch zieht (Skalieren am Griff, Verschieben, Klick in eine Mehrfachauswahl), konnte abstürzen; der Zug endet jetzt mit dem Löschen.
- Menü-Editor: Schloss und Auge bleiben an ihren Elementen, wenn davor etwas gelöscht wird, auch bei unbenannten Elementen und doppelten Namen; Rückgängig gibt einem gelöschten Element Schloss und Auge zurück.
- Menü-Editor: Rückgängig nach dem Löschen benachbarter Elemente stellt sie in ihrer Reihenfolge wieder her.
- Menü-Editor: Der Startbildschirm hat "Menü öffnen" neben "Neues Projekt..." und "Projekt öffnen...": ein Menü aus dem Spiel, einer Datei oder einen ganzen Menüsatz öffnen, bearbeiten und ohne Projekt speichern.

## 中文
- 菜单编辑器:在鼠标仍在拖动时删除(拖动手柄缩放、移动、在多选中单击)可能崩溃;现在删除会结束拖动。
- 菜单编辑器:删除前面的元素时,锁和眼睛保留在各自的元素上,未命名元素和重名元素也一样;撤销会把锁和眼睛还给被删除的元素。
- 菜单编辑器:删除相邻元素后撤销,会按原来的顺序恢复它们。
- 菜单编辑器:开始屏幕在"新建项目..."和"打开项目..."旁边新增"打开菜单":从游戏、文件或整个菜单集打开菜单,无需项目即可编辑和保存。

## 日本語
- メニューエディター: マウスでドラッグ中 (ハンドルでの拡大縮小、移動、複数選択内のクリック) に削除するとクラッシュすることがありました。削除でドラッグを終了するようにしました。
- メニューエディター: 前にある要素を削除しても、ロックと目のアイコンはそれぞれの要素に残ります。名前のない要素や同じ名前の要素でも同様です。元に戻すと、削除した要素のロックと目も戻ります。
- メニューエディター: 隣り合う要素を削除して元に戻すと、元の順序で復元されます。
- メニューエディター: スタート画面の「新しいプロジェクト...」と「プロジェクトを開く...」の隣に「メニューを開く」を追加しました。ゲーム、ファイル、メニューセット全体からメニューを開き、プロジェクトなしで編集・保存できます。

## Español
- Editor de menús: borrar mientras el ratón aún arrastra (escalar con un tirador, mover, clic en una selección múltiple) podía cerrar el programa; ahora el borrado termina el arrastre.
- Editor de menús: el candado y el ojo se quedan con sus elementos cuando se borra algo antes, también con elementos sin nombre y nombres repetidos; Deshacer devuelve a un elemento borrado su candado y su ojo.
- Editor de menús: Deshacer tras borrar elementos vecinos los restaura en su orden original.
- Editor de menús: la pantalla de inicio tiene "Abrir menú" junto a "Nuevo proyecto..." y "Abrir proyecto...": abre un menú del juego, de un archivo o un conjunto de menús entero, edítalo y guárdalo sin proyecto.

## Čeština
- Editor menu: smazání během tažení myší (změna velikosti úchytem, přesun, klik do vícenásobného výběru) mohlo shodit program; smazání nyní tažení ukončí.
- Editor menu: zámek a oko zůstávají u svých prvků, když se smaže něco před nimi, i u prvků bez jména a se stejnými jmény; Zpět vrátí smazanému prvku zámek i oko.
- Editor menu: Zpět po smazání sousedních prvků je obnoví v původním pořadí.
- Editor menu: úvodní obrazovka má "Otevřít menu" vedle "Nový projekt..." a "Otevřít projekt...": otevřete menu ze hry, ze souboru nebo celou sadu menu, upravte ho a uložte bez projektu.
