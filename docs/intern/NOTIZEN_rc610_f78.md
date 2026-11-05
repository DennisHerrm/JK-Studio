# Notizen rc610 / F78: Funde der Codepruefung Bereich 7 und 8

Auftrag `auftrag_rc610_f78.md`, Funde aus `pruefung_b7b8_funde.md`. Direkt in `Y:\karte` auf `karte1` ab 28047b62 (F46), Build `build-test`, Laeufe `laeufe/f78_*` (rot `*_rot`, gruen `*_g1`, Schluss `f78e_*`). Laufskript `lauf.py` wie F46; fuer Umzug und Update eigene Treiber im Scratchpad (`f78_umzug.py`: Kopie mit `BHED_APPDATA`, erster Start `--pruefe-daten` mit gesperrter Datei, zweiter Start Selbsttest `umzug`; `f78_update.py`: Kopie mit `BHED_UPDATE_LOKAL=rc500`, eigenes `TMP`/`TEMP` je Lauf, `BHED_UPDATE_TEIL`). kFassung bleibt rc609. Kein Spiel gestartet.

Neue Pruefungen: ctest `diag_faeden` (B7-1), `bhed_cli` (B8-6, B8-7, B8-8), `sicherschreiben` Teil Einstellungen (B8-2), `update` drei Faelle (B8-4, B8-5), `umzug` gesperrte Datei (B8-1); Selbsttest-Modus `f78` (B7-2), Selbsttest `update` mit `BHED_UPDATE_TEIL` leer/`zweite`/`beenden` (B8-3, B8-9, B8-4). Testhilfe: `bhed::update::testHakenSetzen` (Haken nach jedem entpackten und getauschten Eintrag; nur ctest und Selbsttest). Die Rotlaeufe liefen auf 28047b62 plus den Pruefungen und der Testhilfe.

## B7-1: Protokoll aus Hintergrundfaeden

- Behoben: eine Sperre (rekursiv, nie freigegeben) fuer Ablauf- und Detailprotokoll, Schrittstapel und Zeilen; `detailOn` ueber ein atomares Merkmal; `lines()` gibt eine Kopie, neu `lineCount()`.
- Rot (ctest `diag_faeden`, 8 Faeden x 20 000 Zeilen): Absturz 0xc0000374 (Heap kaputt). Gruen: 160 000 Zeilen im Speicher und je 160 000 vollstaendige Zeilen in beiden Dateien. Nicht unter ASan wiederholt (der Release-Bau stuerzte schon ab).
- Bildzeit: nicht gemessen - das Protokoll schreibt waehrend der Bildmessung nicht (Detailprotokoll `r1_leistung_nach`: zwischen Beginn und Ergebnis einer 240-Bilder-Messung keine Zeile). Die Sperre kostet nur, wo ohnehin eine Zeile mit `flush` geschrieben wird.

## B7-2: karte_gpuzeiten.txt aus zwei Faeden

- Behoben: Lesen, Merken und Schreiben unter einer Sperre, geschrieben ueber `sicherSchreiben`.
- Rot (Selbsttest `f78`, 2 Faeden je 200 Karten): 388 von 400 Zeiten fehlten. Gruen: 0 fehlen, 0 falsch.

## B7-3: JPEG-Kosinustabelle

- Nicht nachstellbar, nicht geaendert: 300 frische Prozesse, je 32 Faeden als erste Entzifferung desselben JPEG, gegen eine einzelne Entzifferung verglichen - 0 Abweichungen (x64, MSVC). Der Versuch (`imagetest --faeden`) ist nicht im Repo.

## B8-1: gescheiterter Einstellungs-Umzug

- Behoben: `umzug::ordnerWaehlen` - scheitert die Kopie, entsteht kein `jkstudio`, die Sitzung arbeitet im alten Ordner `behaved`, der naechste Start kopiert (Zusage aus `umzug.h`). `settingsDirectory` waehlt einmal je Sitzung (fadenfest, auch aus dem Schnell-Backen gefragt).
- Rot (`f78_umzug_rot`): nach dem ersten Start mit gesperrter Datei `jkstudio` leer angelegt, der zweite Start (Selbsttest `umzug`) 2 OK / 4 FEHLER (Sprache, Thema, letzte Karte, Original). Gruen (`f78_umzug_g1`): kein `jkstudio` nach dem ersten Start, der zweite kopiert 3 Dateien, 6 OK / 0 FEHLER. ctest `umzug` dazu.

## B8-2: saveSettingsFile

- Behoben: ueber `sicherSchreiben` (Temp-Datei, Schliessen geprueft, atomar ersetzt).
- Rot (ctest `sicherschreiben`, eingebauter Fehler beim Schreiben/Schliessen/Umbenennen): Rueckgabe true, behaved.cfg 2676 statt 1335 Byte. Gruen: false, alte Datei unveraendert, keine Temp-Datei. Ein voller Datentraeger (VHD) wurde nicht aufgebaut.

## B8-3: nach der Installation noch einmal suchen

- Behoben: `pruefen` laesst `Fertig` (und `Laedt`) stehen - nach der Installation gilt das installierte Release als aktuell, "Jetzt neu starten" bleibt.
- Rot (`f78_upd_rot`): zweite Suche "Verfuegbar", zweites Installieren "jkstudio.exe cannot be replaced (error 5)" (5208 Dateien zurueckgestellt). Gruen (`f78_updz_g1`): beide Male `Fertig`, 4 OK / 0 FEHLER.

## B8-4: Beenden waehrend der Installation

- Behoben: `updater::beimBeenden` (aus main nach dem Fenster): Laden und Entpacken hoeren auf (es ist nichts getauscht), ein begonnener Tausch laeuft zu Ende. Das Programm wird zuletzt getauscht (auch wenn das .zip es frueher nennt). Ein trotzdem unterbrochener Tausch (Absturz, Task-Manager) wird beim naechsten Start zurueckgestellt (`update::resteAufraeumen`, vor dem Loeschen von `<exe>.alt`; die neu angelegten Dateien stehen in `neu_angelegt.txt`).
- Rot (`f78_updb_rot`, Beenden nach dem ersten getauschten Eintrag, 5 ms je Tausch): Ordner halb - `jkstudio-update.tmp` mit 5211 Dateien, kein `jkstudio.exe.alt`. Gruen (`f78_updb_g1`): wartet, 5210 Dateien ersetzt, `jkstudio.exe` und `.alt` da, kein Zwischenordner. ctest `update`: Tausch mitten abgebrochen (Haken wirft) -> Programm noch alt; `resteAufraeumen` -> Ordner wie vorher; nach fertigem Tausch nur aufgeraeumt.
- Bleibt: zwischen "exe -> exe.alt" und "neu.exe -> exe" (zwei Umbenennungen) gibt es keine jkstudio.exe - nur ein Stromausfall genau dort trifft das.

## B8-5: Rueckstellen gescheitert

- Behoben: jedes Zurueckstellen geprueft; was sich nicht zurueckstellen laesst, bleibt als Sicherung im Zwischenordner (`sicherungBleibt`), der naechste Start stellt es zurueck.
- Rot (ctest `update`: data/x.txt gesperrt, die eben getauschte bhc ohne Teilen offen): Ziel "neue bhc", Sicherung weg. Gruen: Sicherung "alte bhc", nach `resteAufraeumen` wieder "alte bhc", kein Zwischenordner.

## B8-9: Ablage im Temp-Ordner

- Behoben: das .zip liegt unter `%TEMP%\behaved_update\<pid>` und wird nach dem Einspielen geloescht; ein Start raeumt nur Ablagen beendeter Prozesse (und lose .zip bis rc609) weg.
- Rot (`f78_updz_rot`: A wartet nach dem ersten entpackten Eintrag, B startet aus einem zweiten Ordner mit demselben TEMP): B loeschte das .zip von A, A "Archiv nicht lesbar". Gruen (`f78_updz_g1`): das .zip bleibt, A ersetzt 5210 Dateien.

## B8-6, B8-7, B8-8: bhed (Befehlszeile)

- Behoben: Manifest mit UTF-8 als Codepage (`tools/bhed.manifest`), Ordnerdurchlauf mit Fehlercode, Ausnahmen gefangen (Rueckgabe 2); `spew` ueber `sicherSchreiben`; Befehlsmodell neben der eigenen .exe (`GetModuleFileNameW`).
- Rot (ctest `bhed_cli`): `check <ordner mit テスト.icarus>` 0xC0000409 (abort), `compile テスト_gut.icarus` "leer oder nicht lesbar: ???_gut.icarus", `compile -o <Pipe ohne Leser>` Rueckgabe 0 mit "3 Bloecke", `bhed info` ueber den PATH Rueckgabe 2. Gruen: 1 mit beiden Dateien, 0 mit .ibi, 1 "kann nicht schreiben", 0.

## B8-10: Windows 10 vor 1903

- Nicht geaendert (shank 05.11.: erst nach dem naechsten Release). Umfang grob ermittelt: rund 580 Kandidatenzeilen in 116 Dateien (schmale Pfade in fstream, fopen, std::remove/rename, A-Funktionen, std::filesystem mit std::string, `path::string()`), davon etwa 250 in Selbsttests; dazu assimp (char*-Pfade) und bhed_q3map2 (C, fopen). Liste je Datei: `laeufe/f78_b810_stellen.txt`.

## Schluss

- Sauberer Neubau (`--clean-first`): 0 Warnungen, 0 Fehler. ctest 218 von 218 (neu `diag_faeden`, `bhed_cli`).
- Parallel (`laeufe/f78e_*`): Selbsttest `update` aus einer Kopie (`BHED_UPDATE_LOKAL=rc500`, rc609 von GitHub) 4/0, `umzug` (mit gesperrter Datei im ersten Start) 6/0, `letzte` 4/0 (mit `BHED_MISSION_ARCHIV`/`BHED_MISSION_NAME` wie `alle_modi.sh`; ohne sie 2/5 wie in der Endrunde rc609 beschrieben), Modus 1 2651/0.
- Lints: lint_sources, lint_d3dbesitz, lint_mapstate, lint_gapp, lint_feldinit, lint_i18n, lint_settings, lint_imgui_context, lint_rueckkopplung, lint_fassung ok; lint_portability 99 (vorher 102; die neuen Dateien und update_einspielen/updatetest_echtheit mit `<cstddef>`). Handbuch `--pruefen` ok (es/cs 0 englisch).
- Handbuch (6 Sprachen): "Nach Updates suchen", Schritt 2 - nach der Installation bleibt "Jetzt neu starten", Schliessen waehrend der Installation laesst sie zuerst fertig werden. Keine neuen Bilder.
- `leistung` nicht gelaufen: das Protokoll schreibt waehrend der Bildmessung nicht (B7-1).

## Offen

- B8-10 als eigener Auftrag nach dem Release.
- B8-4: die Luecke zwischen zwei Umbenennungen beim Programm.
- B7-3 nicht nachstellbar (unveraendert).

## Changelog

### English
- Updater: after installing, "Check for updates" keeps offering "Restart now" instead of the same release again (installing it twice failed).
- Updater: closing the program during an installation no longer leaves a half-updated folder - loading stops, a started swap finishes first; an update interrupted anyway is undone at the next start.
- Updater: if old files cannot be put back after a failed update, their backups stay and the next start restores them; a second installation starting at the same time no longer deletes the download of the first.
- Settings: if copying the old settings folder (behaved -> jkstudio) fails, the program keeps using the old folder and tries again at the next start (before, it started with an empty folder for good).
- Settings: behaved.cfg is written safely - a write error (full disk) keeps the previous file with game folders and keys.
- Map editor: the remembered GPU bake times are no longer lost when a build and the quick bake finish together.
- Fixed a possible crash when several background tasks wrote to the log at the same time (for example damaged textures while loading a map).
- bhed (command line): file names outside the system code page work, write errors are reported, and data/base is found when bhed is called through PATH.

### Deutsch
- Updater: Nach der Installation bietet „Nach Updates suchen“ weiter „Jetzt neu starten“ an statt noch einmal dasselbe Release (ein zweites Installieren scheiterte).
- Updater: Schließen des Programms während einer Installation hinterlässt keinen halb erneuerten Ordner mehr - das Laden hört auf, ein begonnener Tausch wird zuerst fertig; ein trotzdem unterbrochenes Update wird beim nächsten Start zurückgenommen.
- Updater: Lassen sich nach einem gescheiterten Update alte Dateien nicht zurückstellen, bleiben ihre Sicherungen, und der nächste Start stellt sie zurück; eine zweite Installation, die gleichzeitig startet, löscht den Download der ersten nicht mehr.
- Einstellungen: Scheitert das Kopieren des alten Einstellungsordners (behaved -> jkstudio), arbeitet das Programm im alten Ordner weiter und versucht es beim nächsten Start erneut (vorher startete es für immer mit einem leeren Ordner).
- Einstellungen: behaved.cfg wird sicher geschrieben - ein Schreibfehler (volle Platte) lässt die vorige Datei mit Spielordnern und Tasten stehen.
- Karten-Editor: Die gemerkten GPU-Backzeiten gehen nicht mehr verloren, wenn ein Bau und das Schnell-Backen gleichzeitig enden.
- Ein möglicher Absturz ist behoben, wenn mehrere Hintergrundaufgaben gleichzeitig ins Protokoll schrieben (etwa beschädigte Texturen beim Laden einer Karte).
- bhed (Befehlszeile): Dateinamen außerhalb der Codepage des Systems gehen, Schreibfehler werden gemeldet, und data/base wird gefunden, wenn bhed über den PATH aufgerufen wird.

### 中文
- 更新程序:安装完成后,“检查更新”会继续提供“立即重新启动”,而不是再次提供同一版本(第二次安装会失败)。
- 更新程序:在安装过程中关闭程序不再留下更新了一半的文件夹——下载会停止,已开始的替换会先完成;如果更新仍被中断,下次启动时会撤销。
- 更新程序:更新失败后若旧文件无法还原,其备份会保留,下次启动时还原;同时启动的第二个安装不再删除第一个安装的下载文件。
- 设置:如果复制旧设置文件夹(behaved -> jkstudio)失败,程序会继续使用旧文件夹,并在下次启动时重试(以前会永久以空文件夹启动)。
- 设置:behaved.cfg 以安全方式写入——写入错误(磁盘已满)时保留包含游戏文件夹和按键的旧文件。
- 地图编辑器:构建和快速烘焙同时结束时,记住的 GPU 烘焙时间不再丢失。
- 修复了多个后台任务同时写入日志时可能出现的崩溃(例如加载地图时遇到损坏的纹理)。
- bhed(命令行):支持系统代码页之外的文件名,报告写入错误,并且通过 PATH 调用 bhed 时能找到 data/base。

### 日本語
- アップデーター: インストール後に「アップデートを確認...」を選んでも、同じリリースを再び提示せず「今すぐ再起動」を表示し続けます(2 回目のインストールは失敗していました)。
- アップデーター: インストール中にプログラムを閉じても、更新が半分だけのフォルダーは残りません。ダウンロードは止まり、始まった置き換えは先に完了します。それでも中断された更新は次回の起動時に元に戻されます。
- アップデーター: 更新の失敗後に古いファイルを戻せなかった場合、そのバックアップは残り、次回の起動時に戻されます。同時に起動した 2 つ目のインストールが 1 つ目のダウンロードを消すことはなくなりました。
- 設定: 古い設定フォルダー(behaved -> jkstudio)のコピーに失敗した場合、プログラムは古いフォルダーを使い続け、次回の起動時に再試行します(以前は空のフォルダーのまま起動し続けていました)。
- 設定: behaved.cfg は安全に書き込まれます。書き込みエラー(ディスクがいっぱい)のときは、ゲームフォルダーとキーを含む前のファイルが残ります。
- マップエディター: ビルドとクイックベイクが同時に終わっても、記憶した GPU ベイク時間が失われなくなりました。
- 複数のバックグラウンド処理が同時にログに書き込んだときに起こりうるクラッシュを修正しました(マップ読み込み中の破損したテクスチャなど)。
- bhed(コマンドライン): システムのコードページ外のファイル名が使え、書き込みエラーが報告され、PATH 経由で呼び出しても data/base が見つかります。

### Español
- Actualizador: después de instalar, «Buscar actualizaciones» sigue ofreciendo «Reiniciar ahora» en lugar de la misma versión otra vez (instalarla dos veces fallaba).
- Actualizador: cerrar el programa durante una instalación ya no deja una carpeta a medio actualizar; la descarga se detiene y un intercambio ya iniciado termina primero; una actualización interrumpida de todos modos se deshace en el siguiente inicio.
- Actualizador: si tras una actualización fallida no se pueden restaurar archivos antiguos, sus copias de seguridad se conservan y el siguiente inicio los restaura; una segunda instalación que arranca al mismo tiempo ya no borra la descarga de la primera.
- Ajustes: si falla la copia de la carpeta de ajustes antigua (behaved -> jkstudio), el programa sigue usando la carpeta antigua y lo vuelve a intentar en el siguiente inicio (antes arrancaba para siempre con una carpeta vacía).
- Ajustes: behaved.cfg se escribe de forma segura; un error de escritura (disco lleno) conserva el archivo anterior con las carpetas del juego y las teclas.
- Editor de mapas: los tiempos de horneado en GPU recordados ya no se pierden cuando una compilación y el horneado rápido terminan a la vez.
- Corregido un posible cierre inesperado cuando varias tareas en segundo plano escribían a la vez en el registro (por ejemplo, texturas dañadas al cargar un mapa).
- bhed (línea de comandos): funcionan los nombres de archivo fuera de la página de códigos del sistema, se informan los errores de escritura y se encuentra data/base cuando bhed se llama a través de PATH.

### Čeština
- Aktualizace: po instalaci nabízí „Vyhledat aktualizace“ dál „Restartovat nyní“ místo stejné verze znovu (druhá instalace selhala).
- Aktualizace: zavření programu během instalace už nezanechá napůl aktualizovanou složku - stahování se zastaví, započatá výměna se nejdřív dokončí; přesto přerušená aktualizace se při dalším spuštění vrátí.
- Aktualizace: pokud po neúspěšné aktualizaci nejde vrátit staré soubory, jejich zálohy zůstanou a další spuštění je vrátí; druhá instalace spuštěná ve stejnou chvíli už nesmaže stažený soubor první.
- Nastavení: pokud selže kopírování staré složky nastavení (behaved -> jkstudio), program dál používá starou složku a při dalším spuštění to zkusí znovu (dříve navždy startoval s prázdnou složkou).
- Nastavení: behaved.cfg se zapisuje bezpečně - chyba zápisu (plný disk) ponechá předchozí soubor se složkami hry a klávesami.
- Editor map: zapamatované časy zapékání na GPU se už neztratí, když build a rychlé zapékání skončí současně.
- Opraven možný pád, když do protokolu zapisovalo současně několik úloh na pozadí (například poškozené textury při načítání mapy).
- bhed (příkazový řádek): fungují názvy souborů mimo kódovou stránku systému, chyby zápisu se hlásí a data/base se najde, i když se bhed volá přes PATH.
