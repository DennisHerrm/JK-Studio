# Notizen rc610 / R1: rote Altlasten, Wegpunkt-Tempo, Karten-Editor ruhend

Auftrag `auftrag_rc610_r1.md`. Direkt in `Y:\karte` auf `karte1` ab 50f8f0bd (rc609), Build `build-test`, Laeufe `laeufe/r1_*` (Rotlaeufe `r1_*_rot`, Schluss `r1e_*`). Laufskript wie Endrunde 2 (`lauf.py`, eigener Laufordner, Leerlauf-Prioritaet). kFassung bleibt rc609.

## Punkt 1: allekarten - Speicher nach allen Karten
- Rot: `r1_ak_rot` (Teilliste academy1, academy6, deathstar_trench_v1, duel_carbon, duel_cloudcity, yavin2 - academy1 ist die erste und wird am Ende neu geladen): Grafik +501 MB, Prozess +1316 MB. Endrunde 2 (alle 278 Karten): +716 MB / +3,4 GB.
- Ursachen (je Speicherzeile eingegrenzt, eine Aufteilung der Grafikpuffer nur im Versuch):
  - Figurenmodelle: `modelCache` wurde nur in `loadActorModels` aufgeraeumt, und das kehrt bei einer Szene ohne Figuren vorher zurueck - academy1 behielt die sechs Modelle von academy6.
  - Griffe (`hiltCache`), Gegenstaende (`gegenstandGlmCache`) und die Figurenbilder in `texCache` (Schluessel @256) wurden nie freigegeben.
  - Die Figurenpuffer der Grafik (`g_figuren`, Schluessel = Adresse des Modells) blieben, wenn das Modell starb (18 Puffer ohne Modell nach der Rueckkehr).
  - Der Zielsatz des Himmelsportals (yavin2, 82 MB) und die Netzpuffer (Effekte, Gras wuchsen nur) blieben nach dem Kartenwechsel.
  - Der Treiber behielt freigegebene Speicherbloecke (ohne Trim +120 MB, mit +1 MB).
- Behebung wie im Spiel: `RE_RegisterMedia_LevelLoadBegin/-End` (code/rd-vanilla/tr_model.cpp:257-311, 420-435, tr_image.cpp:889-922 OpenJK): jeder Kartenwechsel beginnt ein Level (`medienLevelBeginnen`), am Ende des ersten Szenenaufbaus (`prepareScene` -> `medienLevelEnde`) gehen Figurenmodelle, Griffe, Gegenstaende und Figurenbilder, die dieses Level nicht genommen hat (iLastLevelUsedOn). Die Grafik folgt ueber Wachen: `GlmModel::wache` (neu, `include/bhed/wache.h`, dieselbe Art wie die `TexWache` aus reste2) loest den Figurenpuffer mit dem Modell. `vergissKarte` gibt die Netzpuffer und die Zielsaetze von Himmelsportal und Spiegel der Karte frei (im Spiel zeichnen beide in den Bildpuffer). Nach dem Aufraeumen einmal `IDXGIDevice3::Trim` nach dem Present.
- Gruen: Teilliste `r1_ak_g2` Grafik +1,0 MB, Prozess +127 MB (7/0); alle Karten im Schluss (unten).
- Gegenprobe Leck oder Hochwasser: drei Runden der Teilliste (`r1_ak_r2`, vor dem Trim) - academy1 627 / 627 MB in Runde 2 und 3, also kein Wachsen je Runde.

## Punkt 2: ablauf - 41 gegen 47 Brush-Netze
- Richtig ist 41: duel_carbon hat 56 Brush-Modelle, 9 davon trigger_*, 6 func_usable mit START_OFF (spawnflags 1: SVF_NOCLIENT, im Spiel nie gezeichnet) - 56 - 9 - 6 = 41 (aus der .bsp gelesen).
- Ursache: Nach einem Kartenwechsel zeichnete die Kartenansicht ein Bild, bevor `prepareScene` die Mover-Simulation baute (das tat erst die Zeitleiste, die nach der Ansicht kommt). Mit leerer Simulation galten die START_OFF-Modelle als sichtbar: sie standen ein Bild lang da, und ihre Netze blieben im Zwischenspeicher. Der Test erwartet die richtige Zahl, er bleibt.
- Behebung: `drawMapView` baut die Szene vor dem Zeichnen, wenn sie fehlt (dieselbe eine Stelle `prepareScene` wie die Zeitleiste).
- Rot `r1_ablauf_rot` 11/3, gruen `r1_ablauf_g1` 14/0 (41 nach jedem Wechsel).

## Punkt 3: umriss - keine orange Linie am Brush-Objekt
- Ursache im Test: Er stellte die Kamera vor die Mitte der Modellgrenzen. Bei Brush-Modellen mit origin-Brush sind die Grenzen relativ zum origin der Entity (q3map2 zieht ihn ab, die Engine stellt das Modell dorthin) - die Kamera blickte auf den Weltnullpunkt, das Objekt lag ausserhalb des Bildes. Bis rc607 traf zufaellig das vierte Objekt aus der vierten Richtung; seit rc608 (Brush-Modelle wie im Spiel) ist die Liste eine andere. Der Umriss selbst war richtig (4 Umrissbilder gezeichnet).
- Behebung im Test: die Mitte in der Welt (Stellung aus `brushModelleImSpiel`, Drehung wie R_RotateForEntity), nur Modelle, die das Bild an ihrer Entity zeichnet.
- Rot `r1_umriss_rot` 45/1, gruen `r1_umriss_g1` 46/0 (func_static Entity 293: 166 -> 4439 orange Punkte). Foto `umriss_brush2` angesehen: die Linie liegt um die Plattform.

## Punkt 4: zeitleiste0 - PNG der alten Leiste
- Ursache: Programmfehler im Selbsttest, kein veralteter Bezug. Das Bild der heutigen Leiste kommt vor dem JSON-Schritt, der den Ordner `bilder` bisher erst anlegte - bei der ersten Szene (academy1_intro) fehlte er, die zweite (md_am_sith_heute.png) ging. Darum auch mit `l12w6` rot.
- Behebung: der Ordner wird vor dem Schreiben angelegt. Rot `r1_zl0_rot` 4/1, gruen `r1_zl0_g1` 4/0 (academy1_intro_heute.png da).

## Punkt 5: Wegpunkt-Erzeugung auf grossen Karten
- Gemessen allein, Selbsttest `wegerzeugung` (Hintergrund und ohne Hintergrund). Fuer die Messung brauchte der Test eine Behebung: `wegFertig` hat eine eigene Zeitgrenze (600 s), die allgemeine von 4000 Bildern (rund 18 s) meldete die laufende Erzeugung auf taspir1 als "haengt". "Vorher" ist die exe von rc609 mit nur dieser Testbehebung (`basis609t`).
- Profil (Fortschrittszeilen alle 5 s): Schritt 1-3 (Laufgitter, Punkte, Kanten) unter 15 s, der Rest in Schritt 4 (Ketten), ueber 9 Durchgaenge (die Rasterweite steigt bis 576, die Grenze des Spiels sind 1023 Punkte) und 3241 nicht anzubindende Teile. Jede Kette fragt dieselben Zellen und Paare wieder (Spuren, Knoten- und Kantenpruefung des Spiels), und ihr Weg zum Ziel lief jedes Mal ueber das ganze Laufgitter.
- Behebung bei gleicher Rechnung: der origin je Zelle (`ortFuer`) und das Ergebnis je Zellenpaar mit seinen origins (`kanteGrund`) werden gemerkt; der Weg zum Ziel der Kette (Dijkstra) endet, sobald er weiter ist als der vom Anfang der Kette - die Kette nimmt nur Zellen, die dem Ziel naeher liegen, und deren Werte sind dann schon endgueltig.
- Tabelle (allein, Sekunden Hintergrund / ohne Hintergrund):

| Karte | vorher | nachher | Wegpunkte, Kanten | Datei (SHA-256) |
|---|---|---|---|---|
| taspir1 | 265,3 / 258,5 | 18,0 / 17,8 | 470, 1257 | 2f99b082... beide gleich |
| jeditemple_garden (MD) | 4,7 / 4,6 | 3,0 / 3,1 | 787, 1621 | b4165856... beide gleich |
| t1_danger | 11,8 / 11,3 | 5,4 / 5,4 | 1018, 2661 | f477cb79... beide gleich |

- Rundlauf: die geschriebenen .ent (`BHED_WE_ENT`, 199388 Bytes bei taspir1) sind bytegleich; im Test "Hintergrund = ohne Hintergrund" ja. taspir1 hat vorher wie nachher denselben einen FEHLER ("Innenraum ... Wegfindung vom Spielerstart": 1 von 8 Innenraeumen) - ein Befund der Karte, nicht des Tempos.

## Punkt 6: Karten-Editor zeichnet im ruhenden Fenster
- Wie NRCs queueDraw (`ZeichenAnlass`, `karteneditor::neuZeichnen`, `ansichtZeichnen`): Kamera und 2D-Ansichten zeichnen nur, wenn
  - eine Eingabe im Editor kam (Maus ueber einem seiner Fenster, Tasten im Fenster mit dem Fokus; seine Fenster, Menues, Dialoge und Popups werden je Bild gesammelt) - damit jede Einstellung, die man im Editor aendert, auch Tastenkuerzel; nach den Ansichten geaenderte Werte zeigt das naechste Bild,
  - sich die Szene aenderte (`szeneStand`, `geometrieStand`: Befehl, Rueckgaengig, Auswahl, Verstecken, neue Karte - die 35 Stellen mit `sichtStand` laufen hier zusammen),
  - sich die Lage der Ansicht aenderte (Matrix, Groesse, Kantenglaettung, Hintergrund- und Akzentfarbe; im 2D auch Kamera und Zeiger),
  - ein Hintergrundlauf fertig ist (Schnell-Backen, Sprungpruefung: `neuZeichnen`), das Grafikgeraet kein Bild der Ansicht hat (verloren, neu),
  - das Bild an der Zeit haengt: Kamerafahrt, Freelook, ladende Texturen, gezeigtes Spielbild, Effekte der Echtzeit-Ansicht, Im Editor spielen, laufender Bau, Rand-Rollen im 2D (danach noch ein Bild).
  - Sonst bleibt das gezeichnete Bild stehen; die Statistikzeile zeigt die Werte dieses Bilds.
- Selbsttests aendern den Zustand ohne Eingabe: dort zeichnet der Editor wie bisher jedes Bild, ausser streng eingeschaltet (`zeichnenNachBedarfImTest`) oder fuer ganze Laeufe mit `BHED_KARTE_NACHBEDARF=1` (dann zaehlt jeder Testschritt als Eingabe).
- Neuer Selbsttest `karteneditor` mit `BHED_KARTE_NUR=ruhen` (`gui/selbsttest_kartenruhe.inc`, Map Editor im eigenen Fenster): 120 Bilder ohne Eingabe - 0 Bilder der Ansichten, 0 gezeigte Bilder des Fensters; das stehende Kamerabild ist Byte fuer Byte das neu gezeichnete (848 x 442); sofort neu und danach wieder Ruhe bei Maus ueber der Kamera, Kamerafahrt ohne Eingabe, Auswahl, Befehl, Rueckgaengig, Taste, groesserem Fenster, neuZeichnen. Erster Lauf rot 13/17 (die Kamera zeichnete 120 von 120: "Built map as in the game" ist eingeschaltet, zaehlte als laufend, auch ohne gebaute Karte), gruen 30/0.
- `tabfenster` mit `BHED_KARTE_NACHBEDARF=1`: ruhendes Map-Editor-Fenster 4 Bilder in 90, waehrend man im Hauptfenster arbeitet (vorher 95 in 90).
- Fotos `karteneditor` `BHED_KARTE_NUR=fotos`: rc609 (`r1_fotos_vor`) gegen jetzt mit `BHED_KARTE_NACHBEDARF=1` (`r1_fotos_nach`): 121 von 129 Dateien gleich; die 8 anderen nur im Laufordner im Pfad (Entity-Liste, Dialoge) und in der Mauskoordinate der Statuszeile - die ist auch zwischen zwei rc609-Laeufen anders (`e2_karteneditor` = `r1_fotos_nach` dort Punkt fuer Punkt).
- Bildzeit `leistung` allein: unten.

## Schluss
- Sauberer Neubau (`--clean-first`) zweimal (nach Punkt 6 und nach der letzten Aenderung an der Eingabe): 0 Warnungen, 0 Fehler. ctest 215 von 215 (auf dem ersten Neubau; danach nur `gui/app_karte.cpp`).
- Die vier Modi mit der Endfassung (parallel, `laeufe/r1e_*`): `allekarten` alle 278 Karten 279 OK / 0 FEHLER, Speicher Grafik +1,9 MB, Prozess +311 MB (vorher +716 MB / +3,4 GB; Spitze 3345 statt 3596 MB); `ablauf` 14/0 (mit dem Trim nach jedem Wechsel, Bild und Kennzahlen wie frisch); `umriss` 46/0; `zeitleiste0` 4/0.
- `karteneditor` kurz (`BHED_KARTE_NUR=fotos`) 78/0, `ruhen` 30/0, `tabfenster` 21/0 (auch mit `BHED_KARTE_NACHBEDARF=1`: Map-Editor-Fenster 4 Bilder in 90 - die Grenzen der Testschritte zaehlen dort als Eingabe; streng im Test `ruhen` 0), Modus 1 2651/0.
- Lints: lint_sources, lint_d3dbesitz, lint_mapstate, lint_gapp, lint_feldinit, lint_i18n, lint_settings, lint_imgui_context, lint_rueckkopplung, lint_fassung ok; lint_portability 102 wie rc609. Handbuch `--pruefen` ok (es/cs 0 englisch).
- Handbuch: "Fenster anordnen", Punkt "Ein ruhendes Fenster zeichnet nicht" in 6 Sprachen (die Ansichten des Karten-Editors zeichnen jetzt nur bei Aenderung); erzeugt mit `gen_handbuch.py`. Sonst kein geaendertes Verhalten, das das Handbuch beschreibt.
- Bildzeit `leistung` allein: je allein, CPU / GPU Mittel in ms (240 Bilder), Leerlauf in % eines Kerns; rc609 (`r1_leistung_vor`), jetzt (`r1_leistung_nach`, im Selbsttest zeichnet der Editor jedes Bild), jetzt mit `BHED_KARTE_NACHBEDARF=1` (`r1_leistung_nb`). Alles im Rahmen der Streuung (Endrunde 1: bis 1 ms Prozessor zwischen zwei Laeufen derselben exe). Die Map-Editor-Messungen zeigen "Vorlage wie im Spiel" (Spielbild, laeuft nach der Zeit) bzw. eine Kamera, die sich dreht - dort zeichnet der Editor auch bei Bedarf jedes Bild; die Ruhe selbst misst `ruhen`.

| Messpunkt | rc609 | jetzt | jetzt, bei Bedarf |
|---|---|---|---|
| Mission mp/duel1 ruhend | 0,41 / 0,16 | 0,43 / 0,16 | 0,55 / 0,16 |
| Mission md_toaj_jedi ruhend | 0,69 / 0,18 | 0,68 / 0,19 | 0,69 / 0,19 |
| Mission md_toaj_jedi Flug | 3,05 / 1,82 | 2,77 / 1,29 | 2,92 / 1,40 |
| Mission Szene angehalten | 0,94 / 0,37 | 1,21 / 0,41 | 1,00 / 0,41 |
| Mission Szene spielt | 3,84 / 2,62 | 3,74 / 2,57 | 3,74 / 2,60 |
| Mission duel_mustafar ruhend | 1,13 / 0,38 | 1,07 / 0,39 | 1,15 / 0,39 |
| Map Editor Vorlage wie im Spiel ruhend | 2,43 / 2,90 | 2,36 / 2,84 | 2,31 / 2,62 |
| Map Editor Vorlage Kamera dreht | 2,38 / 2,38 | 2,45 / 2,42 | 2,27 / 2,35 |
| Map Editor duel_yavin ohne / mit Halmen | 2,20 / 3,30 ; 2,06 / 3,45 | 1,99 / 3,19 ; 2,25 / 3,46 | 2,05 / 3,13 ; 2,12 / 3,41 |
| Map Editor md_am_sith ohne / mit Effekten | 1,97 / 3,07 ; 2,79 / 3,14 | 1,96 / 3,07 ; 2,76 / 3,18 | 2,05 / 2,90 ; 2,76 / 2,96 |
| Model ruhend / Animation | 0,43 ; 0,50 | 0,55 ; 0,54 | 0,45 ; 0,51 |
| Menu Editor ruhend / Maus | 0,55 ; 0,53 | 0,54 ; 0,57 | 0,50 ; 0,51 |
| Leerlauf Mission / Map Editor / Model / Menu Editor | 1,9 / 3,5 / 2,7 / 0,4 % | 0,6 / 1,9 / 0,2 / 1,0 % | 0,6 / 1,7 / 0,8 / 1,9 % |

## Offen
- Punkt 1: nach allen Karten bleiben 6 Eintraege in `texCache` und 6 Figurenbilder mehr als beim ersten Laden von academy1 (eine Karte ohne Figuren) - die nach Pfad abgelegten Klingen- und gfx-Bilder (`bladeTexture`, `spurBild`, `gfxBild`), die bleiben wie bisher; nicht einzeln nachgeprueft. `App::skeletons` (.gla) bleibt je Name (im Spiel ueber r_modelpoolmegs, hier im Prozessrahmen). Gegenstaende und Griffe, die eine Karte erst beim Zeichnen nimmt, werden beim Wechsel auf eine Karte, die sie auch hat, einmal neu gelesen. Ein Modell, das in einem anderen Faden stirbt, gibt seinen Figurenpuffer erst bei Wiederverwendung der Adresse frei (wie die Bilder seit reste2).
- Punkt 6: Was die Ansichten ohne Eingabe im Editor aendert und nicht ueber Szene, Lage, Hintergrundlauf oder Animation laeuft, zeigt erst die naechste Eingabe - gefunden keine solche Stelle (Kartenwechsel aus der Mission laufen ueber `ladeZaehler`, Farben des Themas ueber die Lage). Das Spielbild, das nach einem Bau ohne Eingabe fertig wird, zeigt das Bild nach dem Bau (ein Bild nach dem Ende des Baus wird noch gezeichnet). Im Hauptfenster zeichnet der Editor bei jeder Eingabe dort, wenn er der Tab des Hauptfensters ist.
- Punkt 3: Der Umriss eines Brush-Modells umrandet auch Teile, die im Bild unter einer gleich hohen Flaeche liegen (Plattform in md_am_sith: der Rahmen unter der Lava) - gesehen, nicht geaendert.
- taspir1: "Innenraum ... Wegfindung vom Spielerstart" 1 von 8 vorher wie nachher (Befund der Erzeugung, nicht des Tempos).

## Changelog

### English
- Mission tab: changing the map now frees the characters, saber hilts, items and character images the new map does not use, like the game at the end of loading a level. After all maps the graphics and process memory are back where they started (before: +716 MB graphics, +3.4 GB process).
- Mission tab: right after a map change, func_usable and func_wall with START_OFF were drawn for one frame; the scene is now built before the first frame.
- NPC waypoints: creating the network on large maps is much faster with the same result, byte for byte (taspir1: 265 s -> 18 s).
- Map editor: the camera and 2D views draw only when something changes (input in the editor, the map, the view, a finished background job, animation), like NetRadiant-Custom. A map editor window nobody works in shows no new frames.

### Deutsch
- Mission-Tab: Ein Kartenwechsel gibt jetzt die Figuren, Schwertgriffe, Gegenstände und Figurenbilder frei, die die neue Karte nicht benutzt – wie das Spiel am Ende des Ladens eines Levels. Nach allen Karten sind Grafik- und Prozessspeicher wieder auf dem Anfangsstand (vorher +716 MB Grafik, +3,4 GB Prozess).
- Mission-Tab: Direkt nach einem Kartenwechsel standen func_usable und func_wall mit START_OFF ein Bild lang da; die Szene entsteht jetzt vor dem ersten Bild.
- NPC-Wegpunkte: Das Netz entsteht auf großen Karten viel schneller, mit demselben Ergebnis Byte für Byte (taspir1: 265 s -> 18 s).
- Karten-Editor: Kamera und 2D-Ansichten zeichnen nur noch, wenn sich etwas ändert (Eingabe im Editor, die Karte, die Ansicht, ein fertiger Hintergrundlauf, Animation), wie NetRadiant-Custom. Ein Map-Editor-Fenster, in dem niemand arbeitet, zeigt keine neuen Bilder.

### 中文
- 任务选项卡:切换地图时,现在会释放新地图不使用的角色、光剑剑柄、物品和角色贴图,与游戏在关卡加载结束时的做法相同。遍历所有地图后,显存和进程内存回到起始水平(之前:显存 +716 MB,进程 +3.4 GB)。
- 任务选项卡:切换地图后,带 START_OFF 的 func_usable 和 func_wall 会被绘制一帧;现在场景在第一帧之前就已建立。
- NPC 路径点:在大型地图上生成网络快得多,结果逐字节相同(taspir1:265 秒 -> 18 秒)。
- 地图编辑器:摄像机视图和 2D 视图只在有变化时才重绘(编辑器中的输入、地图、视图、完成的后台任务、动画),与 NetRadiant-Custom 相同。无人操作的地图编辑器窗口不再显示新帧。

### 日本語
- ミッションタブ: マップを切り替えると、新しいマップで使わないキャラクター、セーバーの柄、アイテム、キャラクター画像を解放するようになりました (ゲームがレベル読み込みの最後に行うのと同じ)。すべてのマップの後、グラフィックとプロセスのメモリは最初の状態に戻ります (以前: グラフィック +716 MB、プロセス +3.4 GB)。
- ミッションタブ: マップ切り替えの直後、START_OFF の func_usable と func_wall が 1 フレームだけ表示されていました。最初のフレームの前にシーンを作るようになりました。
- NPC ウェイポイント: 大きなマップでのネットワーク作成が大幅に速くなり、結果はバイト単位で同じです (taspir1: 265 秒 -> 18 秒)。
- マップエディター: カメラと 2D ビューは、何かが変わったときだけ描画します (エディター内の入力、マップ、ビュー、終わったバックグラウンド処理、アニメーション)。NetRadiant-Custom と同じです。誰も操作していないマップエディターのウィンドウは新しいフレームを表示しません。

### Español
- Pestaña Misión: al cambiar de mapa se liberan ahora los personajes, empuñaduras de sable, objetos e imágenes de personajes que el mapa nuevo no usa, como hace el juego al terminar de cargar un nivel. Después de todos los mapas, la memoria gráfica y la del proceso vuelven al nivel inicial (antes: +716 MB gráficos, +3,4 GB proceso).
- Pestaña Misión: justo después de un cambio de mapa, func_usable y func_wall con START_OFF se dibujaban durante un fotograma; ahora la escena se crea antes del primer fotograma.
- Waypoints de NPC: crear la red en mapas grandes es mucho más rápido, con el mismo resultado byte a byte (taspir1: 265 s -> 18 s).
- Editor de mapas: la cámara y las vistas 2D solo se dibujan cuando algo cambia (entrada en el editor, el mapa, la vista, un trabajo en segundo plano terminado, animación), como NetRadiant-Custom. Una ventana del editor de mapas en la que nadie trabaja no muestra fotogramas nuevos.

### Čeština
- Karta Mise: změna mapy teď uvolní postavy, jílce mečů, předměty a obrázky postav, které nová mapa nepoužívá – jako hra na konci načítání úrovně. Po všech mapách jsou grafická paměť i paměť procesu zpět na počátečním stavu (dříve +716 MB grafika, +3,4 GB proces).
- Karta Mise: hned po změně mapy se func_usable a func_wall se START_OFF na jeden snímek vykreslily; scéna teď vzniká před prvním snímkem.
- Navigační body NPC: vytvoření sítě na velkých mapách je mnohem rychlejší a výsledek je bajt po bajtu stejný (taspir1: 265 s -> 18 s).
- Editor map: kamera a 2D pohledy se kreslí jen tehdy, když se něco změní (vstup v editoru, mapa, pohled, dokončená úloha na pozadí, animace), jako v NetRadiant-Custom. Okno editoru map, ve kterém nikdo nepracuje, nezobrazuje nové snímky.
