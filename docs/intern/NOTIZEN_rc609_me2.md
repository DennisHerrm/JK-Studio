# Notizen rc609 ME2: Menü-Editor speichert ein loses Menü im Mod-Ordner an seinen Ort

Auftrag `auftrag_rc609_me2.md`. Entscheidung shank (04.11.): Ein Menü, das lose in einem Mod-Ordner (oder base) unter GameData liegt, darf an seinen Ort zurück, mit Sicherung. Ein Menü in einer .pk3 wird nicht ersetzt (eine lose Datei daneben wirkt im Spiel nicht, Befund W2a), dort weiter "Speichern unter" mit Hinweis.

## Befund
- `speichernWunsch` (gui/app_menue.cpp) schrieb ohne Projekt nur an `d.platte`, wenn `imSpielordner` nein sagte. Ein Menü aus dem Spiel hat keine `platte`, eines über "Datei öffnen..." aus base/MD liegt im Spielordner: beide fragten nach einem Ort, auch wenn die Datei lose im Mod-Ordner lag.
- Rot (`me2_rot_md`, `me2_rot_ja`, Selbsttest `BHED_MENUE_NUR=speichern`): 2 OK, 7 FEHLER. Strg+S fragte in allen vier Fällen (`gefragt 1`): lose aus dem Spiel, lose über Datei öffnen, lose in base neben einer .pk3 mit derselben Datei, in der .pk3 (dort ohne Hinweis); `MePrjLeer` nannte die Karte "Menü öffnen" nicht (en, de, zh, es, cs; ja zufällig ja: "ゲームのメニューを開く").

## Behebung
- `loseImModOrdner`: eine vorhandene lose Datei (keine .pk3) in einem Unterordner des GameData-Ordners (eins über einem eingestellten Spielordner: base, MD, jeder Mod-Ordner, auch Jedi Outcast). Der Programmordner selbst (GameData mit exe und dll) bleibt gesperrt; `imSpielordner` gilt unverändert für Speichern unter, den Export und alle anderen Wege.
- `speichernWunsch`: ohne Projekt und ohne "unter" schreibt ein Dokument mit so einer `platte` an seinen Ort. Ein Menü aus dem Spiel (`Dokument::ausSpiel`, gesetzt nur von "Aus dem Spiel öffnen" und dem Menüsatz, nie für neu angelegte Texte gleichen Namens) nimmt den Ort, an dem das Spiel es findet (`Spieldaten::herkunft`, dieselbe Suche wie `liesWieSpiel`, fs_dirbeforepak 0); danach ist das die `platte`.
- `speichernAmOrt`: die alte Datei als `<pfad>.bak` (Stand vor diesem Speichern, wie W2a `sicherungAnlegenD`), dann `sicherSchreiben` (Temp-Datei, atomar; hält das laufende Spiel sie offen, bleibt sie und die Meldung sagt es mit `DoGesperrt`). Meldung `MeOrtGespeichert`.
- Liest das Spiel für diesen Spielpfad etwas anderes (eine .pk3 im selben Ordner oder ein späterer Ordner), hängt `MeOrtVerdeckt` den Ort an (`verdecktVon`, nur wenn die Datei genau an ihrem Spielpfad in einem eingestellten Ordner liegt).
- Menü in einer .pk3: weiter Speichern unter; die Meldung nennt die .pk3 (`MeInPk3Kopie`), auch bei Abbruch.
- Die Leiste hat nur eine Zeile (lange Meldungen enden dort am Rand, wie schon vorher z. B. "Gespeichert <Pfad>"). Ort, .bak und Hinweis stehen darum ganz in der Einblendung unten rechts (wie W2a `meldung`, umbrochen, mit "Im Explorer zeigen"; mit Hinweis als Warnung). Ein erster Versuch, die Meldung in der Leiste umzubrechen, nahm bei 1920 x 1080 und 200 % der Fläche Höhe weg (voller Lauf: "3b Eng: Beide zu: die Flaeche hat allen Platz", 377 statt >= 400 breit) und ist verworfen.
- Texte: `MePrjLeer` nennt die Karte "Menü öffnen" (statt "zweiter Knopf oben"), Tooltips `MeStartAusSpielTip`/`MeStartDateiTip` beschreiben die neue Regel (6 Sprachen, an ihrer Stelle geändert); neue Texte in `tools/i18n_rc609_me2.py`.

## Tests
- Selbsttest `gui/selbsttest_menue_speichern.inc` (`BHED_MENUE_NUR=speichern` im Modus `menueeditor`, auch am Ende von `menueloeschen`): Kopie eines Spielordners neben der exe (`me2_spiel/GameData/base` mit `me2probe.pk3`, `/MD`, eine Datei im Programmordner), als Spielordner eingetragen und am Ende ausgetragen; nie ein echter Spielordner. Je Fall bearbeiten (Element löschen) und Strg+S über die Taste, der Speichern-Dialog ist ersetzt und zählt, wie oft er gefragt wurde.
  - Aus dem Spiel, lose in MD: fragt nicht, Datei byte-gleich zum Text-Editor, `.bak` = alter Inhalt, Meldung nennt .bak; zweites Strg+S wieder dorthin, `.bak` = Stand nach dem ersten.
  - Datei öffnen, lose in MD: an den Ort, `.bak`.
  - Datei öffnen, lose in base neben `me2probe.pk3` mit derselben Datei: an den Ort, `.bak`, Meldung nennt die .pk3.
  - Aus dem Spiel, in der .pk3: fragt einmal, Kopie geschrieben, .pk3 byte-gleich, Meldung nennt die .pk3.
  - Programmordner: fragt, Datei unverändert, keine `.bak`.
  - `MePrjLeer` enthält in allen 6 Sprachen den Namen der Karte.
- Rot `me2_rot_md`/`me2_rot_ja` 2/7, grün `me2_gr_md`/`me2_gr_ja` 9/0. Die Einblendungen werden mit geprüft (Text mit .bak bzw. der .pk3) und angesehen (`me2_gr4_md`, Foto `me2_meldung_verdeckt`); der Test räumt sie am Ende weg, sonst lägen sie über den Knöpfen der nächsten Schritte (Kette `datei+speichern+eng`: erst 24/7, dann 27/0 MD und JKA).
- Handbuchbild `menue_start_oeffnen` neu (2538 breit, 6 Sprachen, `BHED_MENUE_NUR=startoeffnen BHED_MENUE_BILDER=1`, je 5/0), weil der Text `MePrjLeer` darauf steht.

## Schluss
- Bau aller Ziele 0 Warnungen; ctest 214/214.
- Lints: lint_sources, lint_fassung, lint_i18n, lint_i18n_erzeugt, lint_headers, lint_feldinit, lint_gapp, lint_settings, lint_imgui_context, lint_doppelanzeige ok; lint_tabellenid 1 Fund in `app_kartemission.cpp:319` (schon vorher, nicht berührt).
- `menueeditor` voll: Movie Duels 2774/1 (`me2_voll_md`), JKA (`nur_ja.cfg`) 2774/1 (`me2_voll_ja`) - der eine Fehler war der verworfene Umbruch (oben); nach dem Zurücknehmen der fehlgeschlagene Teil als Kette `datei+speichern+eng` 27/0 (MD, JKA).
- `menueloeschen`: Movie Duels 105/0 (`me2_loe2_md`, nach der letzten Änderung), JKA 105/0 (`me2_loe_ja`).

## Offen
- Nicht im echten Spiel geprüft, ob das Spiel die zurückgeschriebene lose Datei liest (kein Spielstart in diesem Auftrag); die Regel (.pk3 vor losen Dateien eines Ordners) ist die von W2a/`liesWieSpiel`.
- Ein über "Datei öffnen..." geöffnetes Menü zeigt im Reiter `ui\name.menu` mit Backslash (`spielPfadVon` aus dem Plattenpfad, schon vorher so).

## English
- Menu editor: Ctrl+S saves a loose menu in a mod folder or base under GameData back to its place (opened from the game or as a file); the previous version stays next to it as .bak. The whole message (place, .bak, hints) appears in the notification at the bottom right. If the game reads the same menu from a .pk3 first, the message says so. A menu inside a .pk3 still asks where your copy goes, and the message names the .pk3. The game's program folder stays locked.
- Menu editor: the start screen text names the "Open menu" card.

## Deutsch
- Menü-Editor: Strg+S speichert ein loses Menü in einem Mod-Ordner oder base unter GameData an seinen Ort zurück (aus dem Spiel oder als Datei geöffnet); der vorige Stand bleibt daneben als .bak. Die ganze Meldung (Ort, .bak, Hinweise) steht in der Einblendung unten rechts. Liest das Spiel dasselbe Menü zuerst aus einer .pk3, sagt es die Meldung. Ein Menü in einer .pk3 fragt weiter, wohin die Kopie kommt, und die Meldung nennt die .pk3. Der Programmordner des Spiels bleibt gesperrt.
- Menü-Editor: Der Text des Startbildschirms nennt die Karte "Menü öffnen".

## 中文
- 菜单编辑器:Ctrl+S 会把 GameData 下模组文件夹或 base 中的散装菜单保存回原处(无论是从游戏还是作为文件打开);之前的版本以 .bak 留在旁边。完整的消息(位置、.bak、提示)显示在右下角的通知中。如果游戏会先从 .pk3 读取同一菜单,提示信息会说明。.pk3 中的菜单仍会询问副本保存到哪里,提示信息会写出该 .pk3。游戏的程序文件夹保持锁定。
- 菜单编辑器:开始屏幕的文字指向"打开菜单"卡片。

## 日本語
- メニューエディター: GameData の MOD フォルダーや base にある単体のメニューは、Ctrl+S で元の場所に保存されます (ゲームから開いた場合もファイルとして開いた場合も)。以前の版は隣に .bak として残ります。メッセージ全体 (場所、.bak、注意) は右下の通知に表示されます。ゲームが同じメニューを先に .pk3 から読み込む場合は、メッセージでそう伝えます。.pk3 の中のメニューは引き続きコピーの保存先を尋ね、メッセージに .pk3 の名前を表示します。ゲームのプログラムフォルダーには書き込みません。
- メニューエディター: スタート画面の文章が「メニューを開く」カードを案内します。

## Español
- Editor de menús: Ctrl+S guarda en su sitio un menú suelto de una carpeta de mod o de base dentro de GameData (abierto desde el juego o como archivo); la versión anterior queda al lado como .bak. El mensaje completo (sitio, .bak, avisos) aparece en la notificación abajo a la derecha. Si el juego lee antes el mismo menú de un .pk3, el mensaje lo dice. Un menú dentro de un .pk3 sigue preguntando dónde va tu copia, y el mensaje nombra el .pk3. La carpeta de programa del juego sigue bloqueada.
- Editor de menús: el texto de la pantalla de inicio nombra la tarjeta "Abrir menú".

## Čeština
- Editor menu: Ctrl+S uloží volné menu ve složce modu nebo v base pod GameData zpět na jeho místo (otevřené ze hry i jako soubor); předchozí verze zůstane vedle jako .bak. Celá zpráva (místo, .bak, upozornění) se zobrazí v oznámení vpravo dole. Pokud hra čte stejné menu nejdřív z .pk3, zpráva to řekne. Menu uvnitř .pk3 se dál ptá, kam dát kopii, a zpráva uvede ten .pk3. Programová složka hry zůstává zamčená.
- Editor menu: text úvodní obrazovky odkazuje na kartu "Otevřít menu".
