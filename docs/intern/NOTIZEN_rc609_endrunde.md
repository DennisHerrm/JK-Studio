# Notizen rc609 Endrunde 1: Reste, Messungen allein, Spielvergleiche

Auftrag `auftrag_rc609_endrunde1.md`. Direkt in `Y:\karte` auf `karte1` ab 4b8e3d29, Build `build-test`, Laeufe `laeufe/e1_*`.

## A. Reste

### 1: dock1 2 FEHLER (mission.enttext / mission.lumps "wieder im Hauptfenster")
- Ursache: Die Panels aus W2b (Entity-Text) und J (Shader und Flaechen) haben keinen Platz im Hauptfenster; "Andocken" schliesst sie (`dock::zusatzOffen`, offen = eigenes Fenster). `dock::zuhause` ist dann falsch (versteckt), der Test erwartete das Hauptfenster.
- Rot: T1-Schluss 99 OK / 2 FEHLER (auch mit der exe von 6e4892d2). Behebung im Test: fuer diese Panels heisst "zurueck" zu und nicht mehr draussen. Gruen `e1_dock1` 99 OK / 0 FEHLER.

### 2: Echtzeit-Licht, Boden von bm_duel1 sehr dunkel (10 7 7)
- Gemessen an derselben Stelle (Kamera wie im Befund, Filter > Entities), Selbsttest `kemd` K2 erweitert (`e1_kemd2`):

| | Movie Duels | OpenJK |
|---|---|---|
| Editor Textured (ohne Licht) | 49 52 48 | 49 52 48 |
| Editor Echtzeit-Licht | 10 7 7 | 10 7 7 |
| Schnell-Backen (q3map2-Lightmaps) | 8 4 4 | 10 6 6 |
| Spielbild der gebackenen Karte | 8 4 4 | 10 6 6 |

- Spiel (Movie Duels, eigener fs_homepath, `e1_mdhome_bm*`): dieselbe .map mit `bhed_q3map2 -meta` und `-light -fast` gebaut, am Spielerstart (768 40 24, Blick 90): Boden 8,8 4,9 5,4. Das Original `bm_duel1.bsp` (Raven-Lightmaps): 28,7 19,4 16,9. Bild `laeufe/e1_vgl_bm_duel1_spiel_gebaut_original.png` (links gebaut, rechts Original, je 1920 x 1440).
- Ursache: kein Fehler des Echtzeit-Lichts. Die zurueckgewandelte Karte hat keine `light`-Entities (q3map2 nimmt sie nicht in die .bsp, nur mit `-keeplights`) und keine Lampenmodelle; Licht kommt nur aus worldspawn `ambient 60` mal `_color .77 .48 .51` (= 46 29 31) und `_minlight 30`. Textur mal 46/255 ergibt die 9 6 6, die Editor, q3map2 und Spiel zeigen.
- Neue Pruefung in K2: Echtzeit gegen Schnell-Backen und Spielbild, je Kanal hoechstens 4 von 255 auseinander (Vorschau ohne Filter und Supersampling, Kopf von `karte_gpu_win32.cpp`). Gruen 12/0.

### 3: Karten-Editor zeichnet im ruhenden eigenen Fenster jedes Bild neu
- Nicht gebaut, nach rc610 (Liste unten). NRCs `queueDraw` heisst: jede Aenderung ruft "neu zeichnen". Im Editor sind das die 35 Stellen mit `sichtStand`, Rueckgaengig, Auswahl, Hover, Gizmo, Filter, nachgeladene Texturen und Modelle - und die Kamera ist zeitabhaengig (Shader mit tcMod/animMap, fx_runner, Echtzeit-Schatten), die NRC nicht hat. Eine vergessene Stelle zeigte ein altes Bild. Groesser als 2 h.

### 4: Alte Endrunden-Liste
- **h_enttext**: das Panel bekam seine Lage im Bild, in dem es aufging (wirkt in ImGui nicht) und stand mitten ueber der gewaehlten Entity. Jetzt nach dem ersten Bild gesetzt, breit genug fuer die Kopfzeile in allen Sprachen (angesehen: alle Knoepfe ganz); der Schritt prueft, dass die Entity in 3D ausserhalb des Panels liegt. Bilder 2429 bis 2469 breit, 6 Sprachen.
- **h_wegpunkte**: zeigte das Fenster ohne Netz und war 1881 breit. Jetzt mit Netz, ab dem linken Rand 1941 breit, 6 Sprachen (voller Handbuchlauf mit `BHED_FOTO_NUR=h_wegpunkte`).
- **Mittelklick "x y z an der Schreibmarke"** fehlte (W2b offen). Gebaut wie BSP-Entity-Edit: bei offenem Entity-Text setzt ein Mittelklick (kein Ziehen - Ziehen verschiebt weiter die Ansicht) in die 3D-Ansicht die gerundeten Koordinaten des Punkts unter der Maus an die letzte Schreibmarke; Live-Schluessel wirken sofort. Selbsttest `enttext` Teil I: Ziehen fuegt nichts ein, Klick fuegt genau die Spur unter der Maus ein (`-928 5127 -320` an Byte 27723). 27/0. Handbuch: Legende in 6 Sprachen.
- **Mehrzahl-Texte**: 17 rc609-Texte setzten ein Mehrzahl-Wort hinter eine Zahl, die 1 sein kann (Brush-Entities, Clip-Brushes, Blaetter, Patches, Texturen, Luecken, Flaechen, Brush-Seiten, Entities, target-Felder, Kandidaten, Feindstellen). Jetzt Zaehltexte `{Einzahl|Mehrzahl}` (cs drei Formen) mit `trN`/`trKN`; lint_i18n ok.
- **Speichern unter (W2a Weg 3)**: Der Dateidialog bleibt im Selbsttest unterdrueckt; ein Test kann ihm die Antwort vorgeben (`platform::dialogAntwortImSelbsttest`). `dateiort` G: Datei aus der .pk3, Weg 3 -> die .ent liegt am gewaehlten Ort, der Dialog schlug `w2a_pk3.ent` vor, .pk3 Byte-gleich, das zweite Speichern geht ohne Frage dorthin. Gleich gruen (21/0). Der echte Windows-Dialog ist nicht bedient (kein Dialog auf shanks Schirm).

### 5: Menue-Editor, Reitername `ui\name.menu`
- `spielPfadVon` nahm den Plattenpfad ab `\ui\` mit Backslash. Jetzt Schraegstriche wie die Spielpfade. Rot `e1_me2rot` 9/1, gruen `e1_me2gruen` 10/0.

## B. Messungen (jede allein, BHED_D3D_DEBUG=0)
- K3 Asset-Browser (`e1_k3`): laengstes Bild 34 ms (< 100), volle Liste nach 947 ms, 26730 Eintraege, `kemd_assets.txt` Byte-gleich zu `kemd_k3gruen`.
- Wegerzeugung jtg (`e1_we_zeit`): 787 Wegpunkte, 1621 Verbindungen in 4,7 s (Hintergrund) bzw. 4,78 s (ohne Hintergrund); Messproben 1521 ms.
- F15 `topologicalOrder`: ctest `skelett` samt Kette aus 4096 Knochen 0,10 s.
- **Fund beim Vergleich mit rc608**: Mission ruhend und Editor brauchten rund 1 ms mehr Prozessorzeit je Bild. Gegen die exe von rc608 (`l12w6`) unter gleichen Bedingungen und ueber gespeicherte exes eingegrenzt (duel_mustafar ruhend: w2agruen 0,97 ms -> et1 2,18 ms): seit W2b vergleicht jedes `MapEntity::find` den Schluessel mit der ganzen Feldtabelle (rund 140 Namen, `std::tolower` je Zeichen). Behoben: Tabellen nach dem Anfangsbuchstaben, Vergleich nur ASCII wie Q_stricmpn - gleiches Ergebnis (ctest enttext, bsp, losedateien gruen).

### Tempotabelle (Mittel CPU / GPU in ms, 240 Bilder; rc608 = exe `l12w6` heute gemessen, zwei Laeufe)
| Messpunkt | rc608 | rc609 vor der Behebung (2 Laeufe) | rc609 Endrunde 1 |
|---|---|---|---|
| Mission mp/duel1 ruhend | 0,60 / 0,16 ; 0,50 / 0,16 | 0,72 ; 0,60 / 0,16 | 0,45 / 0,16 |
| Mission md_toaj_jedi ruhend | 0,84 ; 0,55 / 0,18 | 0,74 ; 0,86 / 0,18 | 0,63 / 0,18 |
| Mission md_toaj_jedi Flug | 3,06 ; 2,99 / 1,29 | 3,13 ; 3,32 / 1,28 ; 1,80 | 3,15 / 1,84 |
| Mission Szene angehalten | 1,03 ; 1,11 / 0,42 | 1,65 ; 1,80 / 0,42 | 0,92 / 0,35 |
| Mission Szene spielt | 4,38 ; 4,21 / 2,87 | 4,75 ; 5,69 / 2,95 ; 3,37 | 3,80 / 2,61 |
| Mission duel_mustafar ruhend | 1,20 ; 1,50 / 0,39 | 2,27 ; 2,43 / 0,41 | 1,13 / 0,37 |
| Editor Vorlage wie im Spiel ruhend | 2,62 ; 1,75 / 2,85 | 3,21 ; 2,94 / 2,91 | 2,47 / 2,83 |
| Editor Vorlage dreht | 2,03 ; 2,76 / 2,45 | 2,70 ; 2,94 / 2,40 | 2,44 / 2,42 |
| Editor Echtzeit-Licht ruhend (duel_jeditemple) | 2,02 ; 2,13 / 2,66 | 2,44 ; 2,92 / 2,64 | 2,43 / 2,64 |
| Editor Echtzeit-Licht dreht | 2,80 ; 2,71 / 5,65 | 2,68 ; 2,76 / 5,60 | 2,51 / 5,71 |
| Model ruhend / Animation | 0,58 / 0,68 | 0,54 / 0,75 | 0,57 / 0,55 |
| Menue-Editor ruhend / Maus | 0,61 / 0,74 | 0,64 / 0,61 | 0,48 / 0,47 |
| Map Editor oeffnen duel_jeditemple.map | 3714 ; 3250 ms | 3483 ; 3367 ms | 3207 ms |

- Tempo P (Spiegel im Editor): ohne sichtbares Portal ist die Grafikzeit gleich (Echtzeit 2,64 / 5,71 gegen 2,66 / 5,65; Vorlage 2,83 gegen 2,85). Prozessor Echtzeit ruhend 2,43 gegen 2,02/2,13 - die Wiederholungen streuen von 1,85 bis 2,92 ms (auch Staende vor P); nicht sicher P zuzuordnen.
- Die alte Tabelle aus rc608 (Notizen rc608, 1,84 / 2,67 ms usw.) lag niedriger: heute laeuft shanks Absturztest, CPU 6 und 7 sind aus. Darum der Vergleich mit derselben rc608-exe heute.

## C. Spielvergleiche (Movie Duels, Popen, je eigener fs_homepath unter `laeufe/e1_mdhome_*`, fs_game MD, developer 1, Fenster ausser Sicht)
- **wpauto_398 (jtg)**: das Netz der vollen Erzeugung des jetzigen Stands (787 Wegpunkte, `BHED_WE_ENT`) als Entity-Lump von `maps/e1we.bsp`: im Protokoll keine Zeile "in solid" (weder "Waypoint ... in solid" noch "Nav(n) in solid"); der Editor setzt den Punkt bei -320 1920 328 nicht mehr. Dass solche Zeilen kommen, belegen die W2a-Laeufe (unten).
- **ME2**: `ui/quit.menu` lose im eigenen fs_homepath, im Menue-Editor aus dem Spiel geoeffnet, Text ersetzt, Strg+S an den Ort (.bak daneben; `BHED_MENUE_NUR=spielbeleg`). Das Spiel zeigt "E1 ME2 SAVED" (`laeufe/e1_spiel_me2.png`, 1920 x 1440).
- **W1 Sprungkante**: Netz mit Dach und 4 Sprungkanten (`BHED_WE_ENT_DACH`) als `maps/e1wedach.bsp`, `nav show edges` und `nav show nodes`: die Kante wpauto_345 (2240 2560 403) -> wpauto_970 (2400 2720 766) steht blau (EDGE_JUMP, g_navigator.cpp:3770 MD9) im Bild (`laeufe/e1_spiel_wedach2.png`; von hinten `e1_spiel_wedach.png`).
- **W2a Datei laden**: `dateiort` mit `BHED_DO_BELEG=1` (ein benannter Wegpunkt 4 unter dem Boden am Spielerstart). Lose `maps/w2a_lose.bsp` (In die BSP uebernommen, ohne .ent): "ERROR: Waypoint w2a_beleg_lose at (72.00 6472.00 -4.00) in solid!". Aus `zz_w2a.pk3` (Weg 1): "Loaded entities from maps/w2a_pk3.ent" und dieselbe Zeile fuer w2a_beleg_pk3. Die .pk3 des Tests enthaelt ein unechtes Ladebild (4096 x 'B'); damit stuerzt Movie Duels beim Laden ab (JPEG-Leser) - fuer den Lauf eine Kopie ohne `levelshots/`, die anderen Eintraege roh kopiert.

## Tests
- Selbsttests einzeln, siehe oben; Schluss: siehe Bericht.
- Der Bau lief einmal waehrend der Handbuchlaeufe (Bau 7); die Laeufe hatten ihre exe schon kopiert.

## Offen (nach rc610)
- Punkt 3 (queueDraw im Karten-Editor), Grund oben.
- Echtzeit-Licht gegen Schnell-Backen auf bm_duel1 (MD): 10 7 7 gegen 8 4 4 - innerhalb der Grenze, Ursache der 2 bis 3 Stufen nicht untersucht.
- Im Schnell-Backen-Bild bleibt mit Filter > Entities eine beleuchtete Flaeche links stehen, die das Echtzeit-Bild ausblendet (Brush-Entity im gebackenen Bild?) - gesehen, nicht untersucht.
- Editor Echtzeit-Licht ruhend +0,3 ms Prozessor gegen rc608, Streuung groesser.

## Changelog

### English
- Entity text: a middle click in the 3D view (not a drag, which still moves the view) inserts `x y z` of the point under the mouse at the text cursor, like BSP-Entity-Edit.
- Menu editor: a menu opened with Open file shows `ui/name.menu` in its tab (it showed a backslash).
- Counted messages from this version use the right singular or plural (for example "1 brush entity", "1 patch").
- Mission tab and map editor: about 1 ms less processor time per frame again - looking up an entity key compared it with the whole field table each time.

### Deutsch
- Entity-Text: Ein Mittelklick in die 3D-Ansicht (kein Ziehen, das verschiebt weiter die Ansicht) setzt `x y z` des Punkts unter der Maus an die Schreibmarke, wie BSP-Entity-Edit.
- Menü-Editor: Ein über "Datei öffnen" geöffnetes Menü heißt im Reiter `ui/name.menu` (vorher mit Backslash).
- Zähltexte dieser Version stehen in Einzahl oder Mehrzahl, wie die Zahl es verlangt (z. B. "1 Brush-Entity", "1 Patch").
- Mission-Tab und Karten-Editor: wieder rund 1 ms weniger Prozessorzeit je Bild - das Nachschlagen eines Entity-Schlüssels verglich ihn jedes Mal mit der ganzen Feldtabelle.

### 中文
- 实体文本:在 3D 视图中单击中键(不是拖动,拖动仍然移动视图)会把鼠标下那一点的 `x y z` 插入到文本光标处,与 BSP-Entity-Edit 相同。
- 菜单编辑器:通过"打开文件"打开的菜单在标签上显示为 `ui/name.menu`(以前带反斜杠)。
- 本版本中带数量的提示会按数量使用单数或复数(例如 "1 brush entity"、"1 patch")。
- 任务选项卡和地图编辑器:每帧的处理器时间重新减少约 1 毫秒——查找实体键时每次都会与整个字段表比较。

### 日本語
- エンティティテキスト: 3D ビューで中クリック (ドラッグではなく。ドラッグは今までどおりビューを動かします) すると、マウスの下の点の `x y z` をテキストのカーソル位置に挿入します (BSP-Entity-Edit と同じ)。
- メニューエディター: 「ファイルを開く」で開いたメニューのタブ名が `ui/name.menu` になりました (以前はバックスラッシュ)。
- このバージョンの件数付きメッセージは、数に応じて単数形・複数形を使い分けます (例: "1 brush entity"、"1 patch")。
- ミッションタブとマップエディター: 1 フレームあたりのプロセッサ時間が再び約 1 ms 減りました。エンティティのキーを探すたびにフィールド表全体と比べていました。

### Español
- Texto de entidades: un clic central en la vista 3D (no un arrastre, que sigue moviendo la vista) inserta `x y z` del punto bajo el ratón en el cursor del texto, como BSP-Entity-Edit.
- Editor de menús: un menú abierto con Abrir archivo se llama `ui/name.menu` en su pestaña (antes con barra invertida).
- Los mensajes con cantidades de esta versión usan el singular o el plural según el número (por ejemplo "1 entidad de brush", "1 patch").
- Pestaña Misión y editor de mapas: de nuevo cerca de 1 ms menos de procesador por fotograma; buscar una clave de entidad la comparaba cada vez con toda la tabla de campos.

### Čeština
- Text entit: kliknutí prostředním tlačítkem ve 3D pohledu (ne tažení, to dál posouvá pohled) vloží `x y z` bodu pod myší na místo kurzoru v textu, jako BSP-Entity-Edit.
- Editor menu: menu otevřené přes Otevřít soubor se na kartě jmenuje `ui/name.menu` (dříve se zpětným lomítkem).
- Zprávy s počty z této verze používají jednotné nebo množné číslo podle čísla (např. „1 brush entita“, „1 patch“).
- Karta Mise a editor map: opět asi o 1 ms méně času procesoru na snímek – hledání klíče entity ho pokaždé porovnávalo s celou tabulkou polí.
