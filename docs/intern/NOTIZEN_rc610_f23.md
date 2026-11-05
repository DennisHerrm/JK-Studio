# Notizen rc610 / F23: Funde der Codepruefung Bereich 2 und 3

Auftrag `auftrag_rc610_f23.md`, Funde aus `pruefung_b2b3_funde.md`. Direkt in `Y:\karte` auf `karte1` ab fc539779 (R1), Build `build-test`, Laeufe `laeufe/f23_*` (Rotlaeufe `f23_*_rot`, gruen `f23_*_g1`, Schluss `f23e_*`). Laufskript wie R1 (`lauf.py`, eigener Laufordner, Leerlauf-Prioritaet). kFassung bleibt rc609. Kein Spiel gestartet: Befehlszeilen nur gemerkt (`befehlStarten`).

Die Rotlaeufe liefen auf fc539779 plus den neuen Pruefungen (Commit 1e599523, nur Testhilfen: `arbeitsDateiSchreiben` aus dem anonymen Namensraum, `gpu::kartenPlaetze`, Plaetze im `SpielbildStand`). Neue Pruefungen: Selbsttest `bspmerge` Teil J (B2-1 bis B2-5), `karteneditor` mit `BHED_KARTE_NUR=ruhen` (B3-1, B3-2), ctest `bspmerge_einheit` (Punkt-Entities).

## B2-1: Nach dem Bauen das Spiel starten

- Ursache: `schrittBeginnen` und `bauSchritt` starteten das Spiel nach jedem Bauende, auch nach eigenen Auftraegen mit Rueckruf (`bauEigeneBefehle` leert `bspName`: `+devmap ` ohne Namen) und nach dem Bau von "Im Spiel testen" (dort startet der Rueckruf das Spiel noch einmal).
- Vorbild: NRC `CWatchBSP::DoEBeginStep` startet mit `g_WatchBSP_RunQuake`, `BuildMonitor_Run` ruft nur `RunBSP` (qe3.cpp:256), also das Build-Menue. Behoben: `spielNachDemBau()` = Schalter an und kein Rueckruf, vor `zusammenfassung` abgefragt (der Rueckruf kann dort schon den naechsten Auftrag beginnen).
- Rot (`f23_bm_rot`): nach dem Teil-Compile 1 Befehl `... +devmap ` (ohne Kartennamen), nach Update entities only 1, Im Spiel testen 2 Befehle (`+devmap f23spiel` zweimal, einer davon ohne Testordner). Gruen (`f23_bm_g1`): 0, 0, genau 1 mit `fs_cdpath` auf den Testordner und `+devmap f23spiel`.
- Zusammenfuehren (`app_karte_zusammen.cpp`) laeuft ueber denselben `bauEigeneBefehle` - nicht eigens getestet.

## B2-2: Verschobene Punkt-Entity doppelt in der .bsp

- Ursache: `neuePunktEntitaeten` verglich nur Abdruecke (Schluessel); eine geaenderte Entity war "neu", kam in den Teil und stand danach zweimal im Entity-Lump, entgegen der Warnung `KeBmEinfuegenPunkte` ("bleiben wie in der fertigen Karte") und dem Handbuch.
- Behoben nach Warnung und Handbuch (Plan `bsp_merge_entitaeten.md`: Punkt-Entities aendert "Update entities only"): `karte::PunktStand` merkt je Punkt-Entity das Objekt des Editors (weak_ptr) und den Abdruck. Neu ist nur ein anderes Objekt, dessen Abdruck keinem fehlenden oder geaenderten von damals gleicht (geloescht und gleich wieder angelegt ist nicht neu). Nach dem Einfuegen kommen nur die neuen dazu (`punktStandErgaenzen`); geaenderte behalten ihren alten Abdruck, die Warnung bleibt also beim naechsten Einfuegen (vorher uebernahm der neue Stand sie still).
- Rot: das verschobene `target_print` (Ziel des Triggers aus B, 64 nach x) stand 2x im Lump. Gruen: 1x, an der alten Stelle, das neue Ziel 1x, Warnung in der Konsole. ctest `bspmerge_einheit`: verschobener NPC nicht neu, neues `target_print` neu, Licht geloescht und neu angelegt nicht neu, nach dem Einfuegen nichts neu und der NPC weiter geaendert.
- Gegenprobe statt NPC mit `target_print` (duel1 hat keine NPCs); dieselbe Regel fuer jede Punkt-Entity.

## B2-3: Ungespeicherte Aenderungen in "Shader und Flaechen"

- Ursache: Einfuegen baut auf `_original.bsp` auf und setzt danach das Dokument des Fensters neu; eine andere .bsp (jedes `karteFreigeben`) setzt es ebenso zurueck. Gefragt wurde nur nach der Karte (`geaendert()`).
- Behoben: dieselbe Rueckfrage wie ConfirmModified (Speichern / Verwerfen / Abbrechen) mit eigenem Text `KeLumpsUngespeichert`, nach der Kartenfrage in `mitRueckfrage` (Neue Karte, Oeffnen, .bsp oeffnen, Zuletzt, Neustart, Beenden, Entity-Definitionen neu) und vor dem Teil-Compile (`lumpRueckfrage`). Speichern laeuft wie der Knopf des Fensters (bei einer .pk3 nach dem Dialog der Speicherwege), danach geht es weiter; Verwerfen setzt das Fenster auf den Stand der Datei. Das Dokument merkt nach dem Speichern die gespeicherten Bytes als seinen Stand.
- Rot: keine Frage, die gespeicherte Datei hatte nach dem Einfuegen den alten Shader an Flaeche 0; bei File > Open .bsp... ging der Dialog sofort auf. Gruen: Frage, Speichern -> Datei mit dem getauschten Shader und dem neuen Trigger; Abbrechen: kein Dialog, Lumps weiter ungespeichert; Verwerfen: Dialog offen, Lumps verworfen.

## B2-4: Eingefuegt, obwohl das Speichern scheiterte

- Ursache: Abdruecke und Lump-Dokument galten vor dem Schreiben als eingefuegt, und `_original.bsp` wurde vor dem Ziel geschrieben.
- Behoben: `lumpBspSpeichern` schreibt erst die Datei, aus der die Karte kam, dann die Arbeitskopie; erst danach (Rueckruf) gelten Abdruecke, Lump-Dokument und Meldungen. Scheitert das Ziel oder wird der Speicherweg abgebrochen, bleibt alles beim Stand vorher, und ein neues Einfuegen baut wieder auf der unveraenderten Arbeitskopie auf.
- Nachgestellt mit einer losen Datei, die ein anderes Programm offen haelt (wie das laufende Spiel: `std::ifstream` teilt kein Loeschen, das Ersetzen scheitert mit "gesperrt"). Rot: Datei unveraendert, aber Arbeitskopie ueberschrieben und 0 neue Brush-Entities - ein zweites Einfuegen meldete "keine neuen". Gruen: Datei und Arbeitskopie unveraendert, der Trigger weiter neu; nach dem Freigeben eingefuegt, 1x im Lump, ein Modell mehr. Der Weg ueber eine .pk3 mit abgebrochenem Speicherweg laeuft ueber denselben Rueckruf (nicht eigens getestet).

## B2-5: Arbeitsdateien ohne Temp-Datei

- Behoben: `arbeitsDateiSchreiben` (frueher `dateiSchreibenRoh`) schreibt ueber `sicherSchreiben` (Temp-Datei, Schreiben und Schliessen geprueft, umbenennen) wie die uebrigen Schreibwege.
- Rot: mit eingebautem Schreibfehler (`sicherSchreibenFehlerEinbauen(1)`) Ergebnis true und 4096 neue Bytes (der Fehler ging am Weg vorbei). Gruen: false, der alte Inhalt (12 Bytes) bleibt.

## B3-1: Spielbild des Karten-Editors beim Wechsel der .bsp - nicht nachstellbar

- Selbsttest `ruhen`: duel_jeditemple (456 Bilder, 138 Lightmaps; belegt bis Platz 451 / 135), dann maps/mp/duel1 (64 / 46): danach hoechster belegter Platz 61 / 44 - schon im Rotlauf. Grund: `aufbauen` laedt die Bilder ueber `loadMapTexturesFuer` -> `loadMapTextures`, und das ruft `gpu::vergissKarte()` fuer die gerade gewaehlte Szene 1 (`app_ablage.inc`), bevor die neuen Bilder hochgeladen werden. Damit laeuft auch der R1-Zweig fuer die Saetze 6 und 7.
- Teil b (Bild der vorigen Karte bei gleicher Adresse) kann daher nicht auftreten: die Plaetze sind vor dem ersten Hochladen der neuen Karte leer. Nichts geaendert; die Pruefung bleibt als Wache im Test.

## B3-2: Ansichten nach dem Neuladen des Spiels im Hintergrund

- Ursache: `spielUebernehmen` setzt Klassen und Texturen der Karte auf das neue Spiel, ohne dass Szene oder Ansicht neu zeichnen (Zeichnen nach Bedarf seit R1).
- Behoben: `++sichtStand` beim Wechsel auf das neu geladene Spiel - Szenen und Ansichten wie nach einem Befehl (NRC: Shader_Realise, die Flaechen holen ihre Shader neu).
- Nachgestellt im Test `ruhen` (streng, Map Editor im eigenen Fenster): Spielordner der Sitzung auf einen leeren `base` im Laufordner (Regelwerk JKA statt Movie Duels, wie der Reiter einer JKA-Mission). Rot (`f23_ruhen_rot2`): Kamera 0, XY 0 Bilder nach dem Laden. Gruen (`f23_ruhen_g1`): Kamera 1, XY 1, danach Ruhe. Der erste Rotlauf (`f23_ruhen_rot`) waehlte die Ordner ohne "Movie Duels" im Pfad - beide eingerichteten liegen unter "GameData Movie Duels", die Sitzung blieb gleich; deshalb der leere Ordner.

## B3-3: Bilder behaltener Figurenmodelle - nicht nachgestellt, offen

- Am Quelltext bestaetigt: OpenJK `RE_RegisterModels_Malloc` (code/rd-vanilla/tr_model.cpp) registriert bei einem schon geladenen Modell alle seine Shader neu (`R_FindShader`), `R_FindImageFile` setzt dabei `iLastLevelUsedOn` - die Bilder eines behaltenen Modells bleiben im Level. Hier meldet ein Treffer in `modelCache`, `hiltCache` und `gegenstandGlmCache` seine Bilder nicht als genutzt, `medienLevelEnde` loescht sie aus `texCache`.
- Folge nur Ladezeit (das Modell behaelt seine eigene Kopie). Nachstellen braucht zwei Missionen mit derselben Figur und einem zweiten Modell mit derselben Haut; das habe ich nicht gebaut. Nach der Regel nicht geaendert.

## Schluss

- Sauberer Neubau (`--clean-first`): 0 Warnungen, 0 Fehler. ctest 215 von 215.
- Parallel (`laeufe/f23e_*`): `bspmerge` 23/0, `lumpedit` 18/0, `karteneditor` kurz (`fotos`) 78/0, `ruhen` 32/0. Modus 1 (`f23e_m1`) lief bei Abgabe noch (Zeitgrenze 5400 s im Laufskript).
- Lints: lint_sources, lint_d3dbesitz, lint_mapstate, lint_gapp, lint_feldinit, lint_i18n, lint_settings, lint_imgui_context, lint_rueckkopplung, lint_fassung ok; lint_portability 102 wie vorher (keine in geaenderten Dateien). Handbuch `--pruefen` ok (es/cs 0 englisch).
- Handbuch (6 Sprachen): "Brush-Entities in fertige Karten einfuegen" (Rueckfrage zu den Lumps, gescheitertes Speichern), Karten-Werkzeuge 9 (Nach dem Bauen das Spiel starten nur nach dem Build-Menue). Keine neuen Bilder.

## Offen

- B3-3 (siehe oben).
- Werden waehrend des Teil-Compiles (q3map2 laeuft) Lumps im Fenster "Shader und Flaechen" geaendert, ersetzt das Einfuegen sie danach weiter ohne Frage (die Frage kommt nur vor dem Bau).
- Das Schliessen des Hauptfensters fragt nicht nach der Karte des Karten-Editors (wie vorher; nur File > Exit fragt).

## Changelog

### English
- Map editor, Insert into compiled map: a moved or changed existing point entity is no longer added a second time; it stays as in the compiled map, as the warning says.
- Map editor: unsaved changes in the window "Shaders and surfaces" are asked about (Save / Discard / Cancel) before Insert into compiled map and before another map is opened; they were lost without a question.
- Map editor, Insert into compiled map: if saving fails (the game has the file open) or the save path is cancelled, nothing counts as inserted, and inserting again works.
- Map editor: the files in the work folder of a compiled map are written via a temporary file, so a failed write keeps the previous state.
- Map editor: "Run Engine After Compile" starts the game only after a build from the Build menu - no longer after Insert into compiled map, Update entities only or merging maps (it started without a map name), and Test in game starts it once instead of twice.
- Map editor: after the game is reloaded in the background (switching to a mission tab with other game folders), the camera and 2D views show the new game at once.

### Deutsch
- Karten-Editor, In fertige Karte einfügen: Eine verschobene oder geänderte vorhandene Punkt-Entity wird nicht mehr ein zweites Mal angehängt; sie bleibt wie in der fertigen Karte, wie die Warnung sagt.
- Karten-Editor: Ungespeicherte Änderungen im Fenster „Shader und Flächen“ werden vor dem Einfügen in die fertige Karte und vor dem Öffnen einer anderen Karte abgefragt (Speichern / Verwerfen / Abbrechen); vorher gingen sie ohne Frage verloren.
- Karten-Editor, In fertige Karte einfügen: Scheitert das Speichern (das Spiel hat die Datei offen) oder wird der Speicherweg abgebrochen, gilt nichts als eingefügt, und ein neues Einfügen geht.
- Karten-Editor: Die Dateien im Arbeitsordner einer fertigen Karte werden über eine Temp-Datei geschrieben; ein gescheitertes Schreiben lässt den vorigen Stand stehen.
- Karten-Editor: „Nach dem Bauen das Spiel starten“ startet das Spiel nur nach einem Bau aus dem Menü Build – nicht mehr nach In fertige Karte einfügen, Nur Entities aktualisieren oder dem Zusammenführen (dort startete es ohne Kartennamen), und Im Spiel testen startet es einmal statt zweimal.
- Karten-Editor: Wird das Spiel im Hintergrund neu geladen (Wechsel auf den Reiter einer Mission mit anderen Spielordnern), zeigen Kamera und 2D-Ansichten sofort das neue Spiel.

### 中文
- 地图编辑器,插入到已编译地图:移动或修改过的已有点实体不再被第二次追加;它保持已编译地图中的样子,与警告所说一致。
- 地图编辑器:在插入到已编译地图之前以及打开另一张地图之前,会询问“着色器和表面”窗口中未保存的更改(保存 / 放弃 / 取消);以前它们会在没有询问的情况下丢失。
- 地图编辑器,插入到已编译地图:如果保存失败(游戏打开了该文件)或取消了保存方式,则不算已插入,可以再次插入。
- 地图编辑器:已编译地图工作文件夹中的文件通过临时文件写入,写入失败时保留之前的状态。
- 地图编辑器:“编译后运行游戏”只在从 Build 菜单编译后启动游戏——插入到已编译地图、仅更新实体或合并地图之后不再启动(那时启动时没有地图名),“在游戏中测试”只启动一次而不是两次。
- 地图编辑器:游戏在后台重新加载后(切换到使用其他游戏文件夹的任务选项卡),摄像机和 2D 视图立即显示新的游戏。

### 日本語
- マップエディター、コンパイル済みマップに挿入: 移動または変更した既存のポイントエンティティが 2 回目に追加されることはなくなりました。警告のとおり、コンパイル済みマップのままです。
- マップエディター: コンパイル済みマップへの挿入の前と別のマップを開く前に、「シェーダーとサーフェス」ウィンドウの未保存の変更を確認します (保存 / 破棄 / キャンセル)。以前は確認なしに失われていました。
- マップエディター、コンパイル済みマップに挿入: 保存に失敗したとき (ゲームがファイルを開いている) や保存方法を取り消したときは何も挿入されたことにならず、もう一度挿入できます。
- マップエディター: コンパイル済みマップの作業フォルダーのファイルは一時ファイル経由で書き込まれ、書き込みに失敗しても前の状態が残ります。
- マップエディター: 「コンパイル後にゲームを起動」は Build メニューからのビルドの後だけゲームを起動します。コンパイル済みマップへの挿入、エンティティのみ更新、マップの結合の後には起動しなくなりました (マップ名なしで起動していました)。ゲーム内でテストは 2 回ではなく 1 回だけ起動します。
- マップエディター: ゲームがバックグラウンドで読み込み直されたとき (別のゲームフォルダーを使うミッションのタブへの切り替え)、カメラと 2D ビューはすぐに新しいゲームを表示します。

### Español
- Editor de mapas, Insertar en el mapa compilado: una entidad de punto existente movida o cambiada ya no se añade por segunda vez; queda como en el mapa compilado, como dice el aviso.
- Editor de mapas: antes de insertar en el mapa compilado y antes de abrir otro mapa se pregunta por los cambios sin guardar en la ventana «Shaders y superficies» (Guardar / Descartar / Cancelar); antes se perdían sin preguntar.
- Editor de mapas, Insertar en el mapa compilado: si el guardado falla (el juego tiene el archivo abierto) o se cancela la forma de guardar, nada cuenta como insertado y se puede volver a insertar.
- Editor de mapas: los archivos de la carpeta de trabajo de un mapa compilado se escriben mediante un archivo temporal; si la escritura falla, queda el estado anterior.
- Editor de mapas: «Run Engine After Compile» solo inicia el juego tras una compilación desde el menú Build; ya no tras Insertar en el mapa compilado, Actualizar solo las entidades o combinar mapas (allí se iniciaba sin nombre de mapa), y Probar en el juego lo inicia una vez en lugar de dos.
- Editor de mapas: cuando el juego se vuelve a cargar en segundo plano (al cambiar a la pestaña de una misión con otras carpetas del juego), la cámara y las vistas 2D muestran el juego nuevo enseguida.

### Čeština
- Editor map, Vložit do hotové mapy: přesunutá nebo změněná existující bodová entita se už nepřidá podruhé; zůstane jako v hotové mapě, jak říká upozornění.
- Editor map: na neuložené změny v okně „Shadery a plochy“ se program zeptá před vložením do hotové mapy a před otevřením jiné mapy (Uložit / Zahodit / Zrušit); dříve se bez dotazu ztratily.
- Editor map, Vložit do hotové mapy: když se uložení nepodaří (hra má soubor otevřený) nebo se zruší způsob uložení, nic se nepočítá jako vložené a vložit lze znovu.
- Editor map: soubory v pracovní složce hotové mapy se zapisují přes dočasný soubor; nepovedený zápis ponechá předchozí stav.
- Editor map: „Po kompilaci spustit engine“ spustí hru jen po sestavení z nabídky Build – už ne po Vložit do hotové mapy, Aktualizovat jen entity ani po slučování map (tam se spouštěla bez názvu mapy), a Testovat ve hře ji spustí jednou místo dvakrát.
- Editor map: po novém načtení hry na pozadí (přepnutí na kartu mise s jinými složkami hry) ukážou kamera i 2D pohledy hned novou hru.
