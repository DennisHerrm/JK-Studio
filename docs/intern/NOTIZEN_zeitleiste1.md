# Notizen zeitleiste1: neue Zeitleiste im Mission-Tab

Zweig `zeitleiste1` ab zeitleiste0 (23ffa48b), Worktree `Y:\mission`, Bau
`Y:/mission/build-mission`, Texte `tools/i18n_zeitleiste1.py`. Auftrag:
`<SP>\auftrag_zeitleiste1.md` + `<SP>\auftrag_gemeinsam.md`, Vorlage
`<SP>\zeitleiste_entwurf\entwurf_freigegeben.html`. Korrektur von shank (nach
der Pause am 29.10.): der obere Teil des Entwurfs (Kamerabild, Draufsicht,
"Was passiert hier?") wird nicht als eigene Fenster gebaut - uebernommen wird
die Zeitleiste; die Erklaerung kommt in die rechte Seitenleiste
(Script-Flow-Spalte), die Vorschau ist die vorhandene 3D-Ansicht, Punkt 9
(Draufsicht) entfaellt.

## Aufbau

- **Modell** `include/bhed/spuren.h`, `src/spuren.cpp` (ohne ImGui): baut aus
  Ablauf, Kamerabahn, Szene und flacher Zeitleiste je Zeile die Bloecke mit
  echten Dauern (Satz = Klanglaenge, Lauf bis Ankunft, Animation bis Ende der
  Haltezeit, Kamera = letztes Argument), Unterbahnen, Knickpunkte fuer
  Sichtfeld/Blende, "springt zurueck", Ziehbarkeit, "nicht nachgebildet".
  Gebaut in `prepareScene` (`zeitleisteModellBauen`). Klanglaenge, Untertitel
  (StringEd-Leser `menue::Texte::ladeStr`, Schluessel wie CG_CaptionText) und
  Musik hinter einem use kommen ueber Rueckrufe.
- **Zeichnen** `gui/app_zeitleiste.cpp`: Zeilen, Bloecke, Marken (Beschriftung
  nur, wo sie nichts ueberdeckt), Filmstreifen, Navigationsleiste, Ziehen,
  Erklaerung. `drawTimeline` behaelt Transportleiste, Abspielen und Lineal.
  `kategorieVon` (Textsuche) ist weg; die Feldansicht "Spuren"
  (`spurenBauen`) liest dasselbe Modell (Dialog jetzt mit echter Klanglaenge
  statt fest 2,5 s).
- **Erklaerung** unten in der Script-Flow-Spalte (`drawFlowArea`), so hoch wie
  ihr Inhalt (hoechstens 75 %); Skriptzeile und "Skriptzeile oeffnen" (Zeile
  waehlen + Bearbeiten-Dialog) direkt unter Titel und Zeit.

## Ergebnisse je Punkt

1. **Gemeinsames Modell** - ctest `spuren` (eigene Mini-Szene: Karte mit
   NPC_spawner, waypoint_navgoal, zwei ref_tag, target_play_music,
   target_level_change; Skript mit jeder Befehlsart): gruen. Erwartungswerte
   aus den Regeln der Engine (wait +50 ms, CAMERA_DEFAULT_FOV 90, lineare
   Zoom-/Blendenrampe, Klanglaenge, dowait auf CHAN_VOICE, Signal-Sender,
   Kartenwechsel, print/camera TRACK nicht nachgebildet, Schnitt bricht eine
   Fahrt ab). Gegenprobe mit dem Export zeitleiste0 im Selbsttest (unten).
2. **Uebersicht/Detail** wie im Entwurf: Kamerabilder, Kamera - Ort
   (Einstellung je MOVE bis zum naechsten, Name der Markierung bzw. "(x, y,
   z)"), Kamera - Drehung (Schnitt markiert), Sprache, Musik, Skriptablauf,
   Darsteller (oben Animationen ohne STAND/IDLE/BOTH_SITn, unten "laeuft n" /
   "rennt n", gestrichelt "steht still", schraffiert unsichtbar),
   aufklappbar (Animation, Befehle). Detail: Kamerafahrt, Schwenk, Zoom,
   Blende (+ Rollen, Wackeln, Folgen, Kamerabahn, wenn vorhanden), Sichtfeld
   und Blende als lineare Linien. affect-Ziele ohne Figur bekommen trotzdem
   eine Zeile.
3. **Lesbarkeit**: Zeilen- und Blockhoehen aus dem Entwurf im Massstab
   Schrift/14 (kleinster Block 25,7 px bei Schrift 24); der Selbsttest prueft
   Mindesthoehe und keine ueberlappenden Beschriftungen (academy1 und
   md_am_sith in beiden Ansichten, Mini-Szene). Ueberlappungen als Unterbahn
   mit "springt zurueck" und Erklaerung aus cg_camera.cpp; abgeschnittene
   Namen haben einen Tooltip.
4. **Sprache** mit Untertitel, Sprecher, Wellenform (Spitze je 50 ms);
   **Musik** (Levelmusik und target_play_music ueber Ablauf::benutzt);
   **Skriptablauf** mit schraffierten, beschrifteten Wartezeiten ("wartet auf
   ‚x‘", Sender und Zeit in der Erklaerung), Kartenwechsel-Marke.
5. **Filmstreifen**: alle 2 s ein Bild der Skriptkamera (sichtFovX; Blende
   und Kinobalken aus CameraState darueber). Aufgenommen in einem Durchgang,
   je ruhigem Bild der Ansicht eines: drawMapView zeichnet dann nur das
   Filmbild (224 px breit, Zielsatz 4 - kZielSaetze 4 -> 5) zur Zeit des
   Filmbilds und stellt die Zeit des Zeigers gleich danach zurueck; Ansicht,
   Vorschau und Ueberlagerung bleiben wie im Bild davor. "Ruhig" heisst: die
   Ansicht haette sonst nichts zu zeichnen (mapDirty aus), keine Maus, kein
   Abspielen, und in der Ansicht ist nichts gewaehlt - jede Aenderung zeichnet
   zuerst die Ansicht neu, der Durchgang wartet. Laeuft nach jedem Modellbau
   neu. Messung (unter Last): academy1 45 Bilder im Mittel 8,55 ms gegen
   7,73 ms ohne Durchgang, md_am_sith 31 Bilder 30,6 gegen 30,0 ms - keine
   laengeren Bilder. **Fund (mission1)**: in der ersten Fassung verdraengten
   die Filmbilder normale Bilder und verstellten die Zeit des Zeigers fuer
   alle; im Selbsttest mission1 gingen Zuege an Figur-Griffen ins Leere
   ("Undo Figur ...", Laeufe zle_mission1, zl1m1, zl1m2 rot). Gegenprobe ohne
   Filmdurchgang: gruen (zl1m3). Behoben wie oben beschrieben, mission1 gruen
   (zl1m4: 98 OK, 0 FEHLER).
6. **"Was passiert hier?"** in 4 Sprachen fuer jede Blockart (Einstellung,
   Fahrt, Schwenk, Zoom, Blende, Rollen, Wackeln, Folgen, Kamerabahn, an/aus,
   Satz/Klang, Musik, Warten auf Zeit/Signal/Aufgabe, Signal, use, set,
   Kartenwechsel, nicht nachgebildet, Lauf, Fahrt, Drehen, ROFF, Animation,
   Kamerabild); "Skriptzeile oeffnen" waehlt die Zeile und oeffnet den
   Bearbeiten-Dialog (Selbsttest prueft editorOpen und editorPath).
7. **Navigation**: Anfangsausschnitt 30 s (academy1: 30 000 von 91 550 ms),
   "Alles", -/+ mit Faktor 1,6, Strg+Rad um den Zeiger (die Zeit unter der
   Maus bleibt auf 0,03 ms), Umschalt+Rad rollt, Navigationsleiste nur
   hineingezoomt, Filter "Nur aktive"/"Probleme", Suche, Leertaste, Tab,
   Pos1. Abweichung: die Pfeile bleiben 50 ms / Umschalt 1 s (Entwurf: 1 s /
   5 s) - 50 ms ist das Raster der Spiellogik und steht so in der Tour
   k-neu-timen.
8. **Ziehen**: Kamerabloecke verschieben und an der rechten Kante die Dauer
   aendern (Detail und Drehung; Ort zeigt den Hinweis "in der Ansicht
   Detail"), Saetze verschieben (Laenge bleibt), Wartezeiten an der Kante
   (Rest rueckt nach), Raster 50 ms, Hinweis mit der Skriptaenderung,
   geaenderte Bloecke gestrichelt bis Strg+Z, Laeufe nicht in der Laenge
   (Hinweis). Zurueckschreiben ueber replaceAt/replaceMany/insertBefore
   (Zahlen im Format des alten Werts). **Doppeltes Undo beim
   Alt-Verschieben**: nachgestellt (Lauf zl1c: "Alt-Verschieben: 2
   Rueckgaengig-Schritte", ein Strg+Z nicht bytegleich), behoben mit
   `fasseZusammen` in `verschiebeImAblauf`, gruen (zl1d: 1 Schritt,
   bytegleich). Jeder Zug im Selbsttest: ein Schritt, nach Strg+Z bytegleich.
   **Kante unter einer Marke**: lag eine Marke genau am Ende eines wait (ein
   set danach), gewann zwar die Kante, aber der Block des Zugs war die Marke -
   beim Loslassen wurde nichts geschrieben (Handbuch-Lauf zlhb_de: "Kante von
   wait gepackt" ohne "Kante losgelassen"; nachgestellt im Selbsttest zl1g:
   "wait ( 500 ) jetzt 500, Schritte 0"). Behoben: die Kante merkt sich ihren
   Block (`zlGriffClip`), gruen (zl1h: 600, ein Schritt, bytegleich).
9. **Draufsicht**: entfaellt (shank).
10. **Spiel-Regeln**: die Zeiten kommen aus dem Nachbau (OpenJK/Movie Duels
    umschaltbar wie bisher). Kamera nach cg_camera.cpp: ein MOVE/PAN mit
    Dauer 0 bricht den laufenden Uebergang ab und setzt das Ziel
    (CGCam_Move/CGCam_Pan "if (!duration)") - er springt NICHT zurueck; nur
    ein Befehl mit Dauer, waehrend der vorige derselben Gruppe noch laeuft.
    ROFF auf Entities als Bloecke "ROFF name" in der Darstellerzeile; Befehle,
    die der Nachbau nicht kennt, sowie camera TRACK/DISTANCE und print als
    rote Marke "wird in der Vorschau noch nicht nachgebildet".
11. **Tests**: Kern-Test mit eigener Mini-Szene; Selbsttest `zeitleiste1`
    mit Mini-Szene (eigene Namen, ohne Karte), academy1/intro und
    md_am_sith/intro2_sith; der alte Selbsttest `zeitleiste` auf die neuen
    Zeilen umgestellt.
12. **Aufnehmen**: offen.

## Gegenprobe mit dem Export zeitleiste0

| | academy1/intro | md_am_sith |
|---|---|---|
| Saetze / mit Laenge / mit Untertitel | 15 / 15 / 15 (Export 15/15/15) | 9 / 9 / 5 (Export 9/9/5) |
| Dauer | 91 550 ms (Export 91,55 s) | 62 650 ms (Export 62,65 s) |
| Kamerabilder | 46 (Export 46) | 32 (Export 32) |
| Ueberlappungen Export / Modell | 2 / 0 | 11 / 8 |

Die Ueberlappungen weichen bewusst ab: der Export (und die alte Leiste) zaehlte
jeden Abschnitt, in den der naechste derselben Gruppe hineinbeginnt - auch
wenn der naechste ein Schnitt (Dauer 0) ist. Im Spiel bricht ein Schnitt den
Uebergang ab (CGCam_Move: `info_state &= ~CAMERA_MOVING; CGCam_SetPosition`),
die Kamera springt nicht zurueck. In academy1 waren beide alten roten Stellen
(cam3b -> lukewalk2) solche Schnitte. Kern-Test "ein Schnitt mitten in einer
Fahrt springt nicht zurueck".

## Bildzeit des Mission-Tabs (unter Last durch andere Laeufe)

| | vorher (alte Leiste, 23ffa48b) | nachher |
|---|---|---|
| academy1 Uebersicht | 31,95 / 17,43 ms | 8,27 / 9,29 ms (Zeichnen der Zeilen 0,45 / 0,61 ms) |
| academy1 Detail | - | 9,50 / 9,78 ms (Zeilen 1,2 / 1,4 ms) |
| md_am_sith Uebersicht | 26,77 / 21,90 ms | 23,47 / 27,42 ms (Zeilen 0,52 / 0,63 ms) |
| md_am_sith Detail | - | 22,26 / 32,28 ms (Zeilen 0,79 / 1,05 ms) |

Je zwei Laeufe, Mittel ueber 120 Bilder, Idle-Prioritaet, Leiste 55 % hoch.
"Vorher" mit dem ausgepackten Stand 23ffa48b (nicht im Repo, Messschritt im
Export-Modus zeitleiste0 eingefuegt, danach geloescht), Ansicht wie dort
(ganze Szene, Spuren an). Die Zahlen schwanken stark mit der Last der anderen
Laeufe; academy1 ist mit der neuen Leiste deutlich schneller, md_am_sith im
Rahmen der Schwankung gleich. Das Zeichnen der neuen Zeilen selbst kostet
0,4 bis 1,4 ms je Bild.

## Handbuch

Seite `zeitleiste` neu (inhalt_szene.py, 4 Sprachen) mit den Bildern
h_zeitleiste, h_zl_detail, h_zl_erklaerung, h_zl_probleme, h_zl_ziehen
(`BHED_HB_NUR=zeitleiste1`, md_am_sith, Oberflaeche 1,3, je Sprache ein Lauf,
`tools/handbuch/bilder.py`). Texte auf anderen Seiten nachgezogen (Knoepfe,
kurz, kurz2, kurz3, befehle, schritte, schritte2, ROFF-Hinweis); nicht mehr
benutzte Oberflaechentexte entfernt (TlTracks, TlWaits, TlIdleTracks,
TlKat*, Zoom-Menue ...); TlKamera bleibt (Ortskatalog des Helfers).

## Gegen das Spiel geprueft / nicht geprueft

- Gelesen und nachgebaut: CGCam_Move/CGCam_Pan (Schnitt bricht ab),
  CAMERA_DEFAULT_FOV, lineare Rampen (CGCam_Update/UpdateFade), wait +1 Takt
  (TaskManager), Untertitel-Schluessel (cg_text/cg_servercmds).
- Nicht im laufenden Spiel verglichen (keine Spielstarts in diesem Auftrag).

## Offen

- Aufnehmen (Figur steuern -> Laufziele): nicht begonnen.
- Jedi-Outcast-Untertitel (StripEd .sp, `ladeSp`): Schluessel nicht geprueft,
  nicht angeschlossen.
- "Skript oeffnen" bei Befehlen aus anderen Skripten oeffnet das Skript, aber
  nicht die Zeile (der Pfad im fremden Skript geht in pfadeAufsSkript
  verloren).
- Halte ziehen (Punkt 9, "wenn Zeit bleibt") entfaellt mit der Draufsicht;
  die Griffe der 3D-Ansicht (mission1) bleiben.
- Andere Handbuchbilder, die die alte Leiste zeigen (Touren c_tl_*,
  s_sig_zeit, k_waits ...), entstehen erst im naechsten grossen Bildlauf
  (`BHED_EDITORTEST=handbuch`); die Schritte dafuer sind umgestellt.

## Befehle

- Bauen: `cmake --build Y:/mission/build-mission --config Release --parallel 4 --target behaved`
- Selbsttest: `sh /y/mission_lauf.sh zeitleiste1 <name> 1500` (nur Mini-Szene: `BHED_ZL1_NUR=mini`)
- Handbuchbilder: `sh /y/zl1_hb.sh`; Schlussrunde: `sh /y/zl1_ende.sh`

## Schlussrunde

SCHLUSSRUNDE

## Changelog

## English

- New timeline in the Mission tab, built after the approved design: **Overview** shows camera position (one block per shot, named after its mark or as readable coordinates) and rotation, speech with speaker, subtitle and waveform, music, the script flow with labelled waits ("waits for 'camera 2'") and one row per actor (named animations on top, numbered walks, dashed where it stands still, hatched where it is not visible); **Detail** splits the camera into move, pan, zoom and fade and draws field of view and fade as straight lines (the game interpolates linearly).
  - Real durations everywhere: a line lasts as long as its sound file, a walk until arrival, an animation until its hold time ends - the timeline and the "Tracks" view now read one shared model.
  - Camera moves that start before the previous one has finished get their own lane and the note "jumps back", with the reason from cg_camera.cpp.
  - "What happens here?": click any block or mark and the side bar explains it in plain words, with time, command and script line; "Open script line" opens the command in the editor.
  - Camera frames every 2 s as a film strip in the timeline, taken in a short pass that does not slow the view down.
  - Navigation: a scene starts with about 30 s, "All" shows everything, Ctrl+wheel zooms around the pointer, a slim navigation bar appears only when zoomed in; filters "Only active" and "Problems", a search field; Space plays, Tab switches Overview/Detail.
  - Dragging: move camera blocks and lines, drag the right edge of camera blocks and waits; snaps to 50 ms; a note says what changes in the script; changed blocks are outlined; every drag is exactly one undo step (the double undo step of Alt-dragging is fixed). Walks cannot be stretched - their duration comes from path finding.
  - Commands the preview does not simulate yet (camera TRACK/DISTANCE, print) appear as red marks.

## Deutsch

- Neue Zeitleiste im Mission-Tab nach dem freigegebenen Entwurf: **Übersicht** zeigt Kamera-Ort (je Einstellung ein Block mit dem Namen der Markierung oder lesbaren Koordinaten) und -Drehung, Sprache mit Sprecher, Untertitel und Wellenform, Musik, den Skriptablauf mit beschrifteten Wartezeiten ("wartet auf ‚camera 2‘") und je Darsteller eine Zeile (oben Animationen mit Namen, nummerierte Läufe, gestrichelt, wo er still steht, schraffiert, wo er nicht zu sehen ist); **Detail** trennt Kamerafahrt, Schwenk, Zoom und Blende und zeichnet Sichtfeld und Blende als gerade Linien (das Spiel rechnet linear).
  - Überall echte Dauern: ein Satz so lang wie seine Sounddatei, ein Lauf bis zur Ankunft, eine Animation bis zum Ende ihrer Haltezeit - Zeitleiste und Ansicht "Spuren" lesen jetzt ein gemeinsames Modell.
  - Kamerafahrten, die beginnen, bevor die vorige fertig ist, bekommen eine eigene Bahn und den Hinweis "springt zurück", mit dem Grund aus cg_camera.cpp.
  - "Was passiert hier?": ein Klick auf einen Block oder eine Marke erklärt ihn in der Seitenleiste in normaler Sprache, mit Zeit, Befehl und Skriptzeile; "Skriptzeile öffnen" öffnet den Befehl im Editor.
  - Kamerabilder alle 2 s als Filmstreifen in der Zeitleiste, aufgenommen in einem kurzen Durchgang, der die Ansicht nicht verlangsamt.
  - Bewegen: eine Szene beginnt mit etwa 30 s, "Alles" zeigt alles, Strg+Mausrad zoomt um den Zeiger, eine schlichte Navigationsleiste erscheint nur beim Hineinzoomen; Filter "Nur aktive" und "Probleme", ein Suchfeld; Leertaste spielt ab, Tab schaltet Übersicht/Detail.
  - Ziehen: Kamerablöcke und Sätze verschieben, die rechte Kante von Kamerablöcken und Wartezeiten ziehen; Raster 50 ms; ein Hinweis sagt, was sich im Skript ändert; geänderte Blöcke sind umrandet; jedes Ziehen ist genau ein Rückgängig-Schritt (der doppelte Schritt beim Alt-Ziehen ist behoben). Läufe lassen sich nicht strecken - ihre Dauer kommt aus der Wegsuche.
  - Befehle, die die Vorschau noch nicht nachbildet (camera TRACK/DISTANCE, print), erscheinen als rote Marken.

## 中文

- 任务标签页中按已批准的设计重做的新时间轴：**概览**显示摄像机位置（每个镜头一个块，以标记名称或易读坐标命名）和旋转、带说话者、字幕和波形的语音、音乐、带标注等待的脚本流程（“等待“camera 2””），以及每个角色一行（上方是带名称的动画，编号的行走，静止处为虚线，不可见处为斜线）；**详细**把摄像机分为移动、平移、缩放和淡入淡出，并把视野和淡入淡出画成直线（游戏按线性插值）。
  - 处处使用真实时长：台词与其声音文件一样长，行走持续到到达，动画持续到保持时间结束——时间轴和“轨道”视图现在读取同一个模型。
  - 在上一个移动结束前就开始的摄像机移动会获得自己的轨道和“跳回”提示，并给出来自 cg_camera.cpp 的原因。
  - “这里发生了什么？”：单击任意块或标记，侧栏会用通俗的话解释它，并给出时间、命令和脚本行；“打开脚本行”会在编辑器中打开该命令。
  - 时间轴中每 2 秒一张摄像机画面的胶片条，在不拖慢视图的短暂过程中拍摄。
  - 浏览：场景以约 30 秒开始，“全部”显示整个场景，Ctrl+滚轮以指针为中心缩放，只有放大时才显示简洁的导航栏；“仅活动”和“问题”筛选，搜索框；空格播放，Tab 切换概览/详细。
  - 拖动：移动摄像机块和台词，拖动摄像机块和等待的右边缘；对齐到 50 毫秒；提示说明脚本中将改变什么；更改过的块带边框；每次拖动恰好是一个撤销步骤（修复了 Alt 拖动时的双重撤销步骤）。行走无法拉长——其时长来自寻路。
  - 预览尚未模拟的命令（camera TRACK/DISTANCE、print）显示为红色标记。

## 日本語

- 承認されたデザインに基づくミッションタブの新しいタイムライン：**概要**はカメラの位置（ショットごとに 1 ブロック、マーク名または読みやすい座標）と回転、話者・字幕・波形付きの台詞、音楽、ラベル付きの待機（「「camera 2」を待機」）を含むスクリプトの流れ、キャラクターごとの行（上に名前付きアニメーション、番号付きの移動、静止は破線、非表示は斜線）を表示し、**詳細**はカメラを移動・パン・ズーム・フェードに分け、視野とフェードを直線で描きます（ゲームは直線的に補間）。
  - どこでも実際の長さ：台詞はサウンドファイルの長さ、移動は到着まで、アニメーションは保持時間の終わりまで。タイムラインと「トラック」表示は同じモデルを読みます。
  - 前の移動が終わる前に始まるカメラ移動は専用のレーンと「戻る」の注記が付き、cg_camera.cpp による理由が示されます。
  - 「ここで何が起きる？」：ブロックやマークをクリックすると、サイドバーが時間・コマンド・スクリプト行とともにわかりやすく説明し、「スクリプト行を開く」でエディタが開きます。
  - タイムラインに 2 秒ごとのカメラ画像をフィルムストリップとして表示。ビューを遅くしない短い処理で撮影します。
  - 移動：シーンは約 30 秒の範囲で始まり、「全体」で全体表示、Ctrl+ホイールでポインタ中心にズーム、拡大時だけ細いナビゲーションバー。「動作中のみ」「問題」フィルターと検索欄。スペースで再生、Tab で概要/詳細を切り替え。
  - ドラッグ：カメラのブロックと台詞を移動、カメラのブロックと待機の右端で長さを変更。50 ms にスナップし、スクリプトで何が変わるかを表示。変更したブロックは枠で示され、ドラッグごとにちょうど 1 回の元に戻す操作になります（Alt ドラッグの二重のステップを修正）。移動は伸ばせません。長さは経路探索で決まります。
  - プレビューがまだ再現しないコマンド（camera TRACK/DISTANCE、print）は赤いマークで表示されます。
