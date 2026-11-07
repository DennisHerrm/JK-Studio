# Notizen Modell-Block Stufe 6: Auto-Physik (Schwerpunkt, Kontakte, Sprung, Drehimpuls, Federketten)

Stand 07.11.2026, Zweig `karte1`, auf 0ba76a31a (Stufe 7). Ein Agent. Laufnamen `ph_*` (Laufordner `laeufe/ph_*`), Laufskript
`lauf.py` wie Stufe 7. kFassung bleibt rc610.

## Quellen

- Massen: D. A. Winter, "Biomechanics and Motor Control of Human Movement", 4. Aufl. 2009, Tabelle 4.1 (nach Dempster 1955):
  Anteile und Schwerpunktlagen je Segment, Summe 1 (Stellen in `include/bhed/modell/physik.h`).
- Schwerkraft: g_gravity (Vorgabe 800, g_main.cpp), worldspawn "gravity" (g_spawn.cpp, SP_worldspawn) - im Mission-Tab Scene::schwerkraft;
  Pmove rechnet die Wurfbahn mit ps->gravity (bg_slidemove.cpp PM_SlideMove/PM_StepSlideMove). Takt: frameLerp der Sequenz.
- Prinzip wie Cascadeur "AutoPhysics" (geschlossen, nur die Idee): Physik als Filter, eigene Spur, Original bleibt.
- Bein-IK aus Stufe 4 (`ikAnwenden`), Knochen-Erkennung aus Stufe 3.

## 1. Vorab (Rest aus Stufe 7)

1. **Absturz mission1 "Zwischenpunkt"** (C++-Ausnahme, wenn keine laufende Figur gefunden wird). Nachgestellt ueber den neuen
   Testschalter `BHED_M1_MINI_ABSTAND=0` (m1_held wieder genau auf dem Spawner einer Figur der Grundkarte wie vor mb7):
   - rot `ph_m1rot` (BHED_M1_TEIL=mini): exit 0xE06D7363, kein "Knopftest fertig".
   - Ursache: die Schritte nach "eine laufende Figur gefunden" lasen `actors[-1]` und `map.entities[-1]`. Jetzt tun sie ohne Figur
     nichts (der FEHLER davor bleibt die Meldung).
   - gruen `ph_m1gruen`: exit 0, "Knopftest fertig" (die FEHLER-Zeilen sind die gewollten Folgen der blockierten Figur).
2. **Handbuchbild h_mb7_bauen**: Bauen (Knopf, Menue, Suche, Test) klappt den Rollout "Fuer das Spiel bauen" jetzt auf und rollt zu
   ihm (`bauenZeigen`). Dabei gefunden: das Fenster "Figur animieren" fehlte in allen Handbuchbildern der Laeufe - bei grosser Schrift
   war es 1440 hoch im 1384 hohen Hauptfenster und wurde (Viewports) ein eigenes Fenster; die Lage kam zudem aus `DisplaySize` statt
   aus dem Hauptfenster.
   - rot `ph_hb7b` (Lage 1278/144, Groesse 1224 x 1440, eigenes Fenster 1), gruen `ph_hbphd` (1224 x 1204, ganz innen, ohne Korrektur).
   - Bilder `h_mb7_*` in 6 Sprachen neu (`ph_hb7c`, 2538 x 1384), de angesehen; neue Pruefzeile in der Gruppe ph.

## 2. Gebaut

- Backend `include/bhed/modell/physik.h`, `src/modell/physik.cpp`: Massenmodell (14 Segmente aus der Knochen-Erkennung),
  Schwerpunkt, Stuetzflaeche (konvexe Huelle der Fusspunkte Ferse/Ballen/Spitze), Kontakte, Fluege, Parabel, Drehimpuls (Newton je
  Bild, L0 so, dass die Lage bei der Landung bleibt), Bein-IK fuer Kontakte, Federketten (gedaempfter Drehschwinger, 8 Teilschritte).
- `RigSchicht::physik` (rigdaten.h): Einstellungen im Dokument (Rueckgaengig), die erzeugte Spur je Knochen; `rigNachMalAuf` und
  `rigBacken` multiplizieren sie nach FK und IK (X'' = X' P). Aus oder Staerke 0: Anzeige und GLA Byte fuer Byte wie ohne.
- Model-Tab (`gui/app_modelltab_physik.cpp`): Rollout "Auto-Physik" unter IK/FK, Untermenue im Menue Animation und im Rechtsklick
  (Ansicht > Rig, Griff), Suche (X / F3), Tooltips; Ueberlagerung Schwerpunkt mit Lot, Weg (Flug orange), Stuetzflaeche (rot bei
  Warnung). Die Spur rechnet nach jeder Aenderung des Dokuments neu (nicht waehrend eines Zugs), 1,5-2,3 ms je Rechnung (251 Bilder).
- Fenster "Figur animieren": Rollout und Menue "Auto-Physik", Boden = traceRay der Karte, Schwerkraft der Karte; Zeile
  `figur+ "<figur>" physik ...` im Skript; gebacken in die Karten-GLA.
- Werkzeuge, die Posen lesen und Schluessel schreiben (Spiegeln, Handpose), nehmen die Schicht ohne die Physik-Spur (rot/gruen im
  ctest: sie kam sonst doppelt).
- Tests: ctest `modell_physik`, Selbsttest-Gruppen `ph`, `phspiel`, `phzeit` in modellblock, Handbuchseite `modell-physik`.

## 3. Rot -> gruen

ctest `modell_physik` (JKA-Skelett der Hand-Tabelle, Fusspunkte von Hand):
- Schwerpunkt der Grundstellung = Rechnung von Hand (-0,000 0,554 39,457); verschoben -> um dasselbe.
- Stuetzflaeche: Stand ohne Warnung; 90 Grad vorgebeugt -> Warnung ab Bild 5 (Rot-Probe "Schwerpunkt ausserhalb messbar").
- Sprung: Parabel ohne Physik 82,39 -> mit 0,000015; Drehimpuls ohne 83,5 % -> mit 0,007 %; Absprung/Landung bleiben (0,000);
  g 400 der Karte; Absprung/Landung von Hand; Staerke 0,5 dazwischen.
- Filter: aus und Staerke 0 -> gebacken Byte fuer Byte gleich; an -> die GLA traegt die Parabel (0,0116, Kompression 1/64).
- Fusskontakt: ohne 6,0 Einheiten Rutschen -> mit 0,000008. Federkette ltail: Nachlauf 19,1 Grad, nach 50 Bildern 0,007 Grad.
- Spiegeln/Handpose mit Physik: rot (doppelt) -> gruen.

Selbsttest `ph` (Sturmtruppler, ruhigste BOTH_STAND* mit Fusspunkten aus dem Modell; Mission mb7raum): `ph_ph7` 27/0.
- Erreichbarkeit: Menue Animation > Auto-Physik und Rechtsklick > Rig > Auto-Physik je 8 Aktionen mit Tooltip (keine Taste: weder
  max_hotkeys_de.txt noch blender_keys.txt haben eine), Suche X schaltet ein (ein Rueckgaengig-Schritt), Rollout geht auf; wie Blender
  F3 und Menue; Fenster "Figur animieren": Menue mit 6 Aktionen.
- Sprung: rot Parabel 72,01 / Drehimpuls 86,2 % -> gruen 0,0000 / 0,16 %; Staerke 0,5 -> 36,0; gebackene GLA 0,0127; Strg+Z aus,
  Strg+Y an.
- Kontakte: rot 4,0 Rutschen -> gruen 0,0001. Warnung ab Bild 20 (90 Grad vorgebeugt), vorher keine. Federkette 19,1 -> 0,0 Grad.
- Mission (mb7_a springt ueber den Wurzelweg): Schwerpunkt der gebackenen Karten-GLA rot 61,43 -> gruen 0,41 von der Parabel
  (Packung der GLA), Zeile `physik` im Skript, Strg+Z.

Kontakthoehe (Messgrund, Selbsttest ph): tiefster Fusspunkt ueber dem ModView-Boden -24: BOTH_STAND1 0,30, BOTH_STAND2 -0,31,
BOTH_WALK1 -2,41 bis 1,28, BOTH_RUN1 -1,54 bis 9,78; Kyle im Mission-Tab 0,86. Darum gilt der Kontakt relativ zum tiefsten Stand
in der Sequenz, Grenze 0,5.

## 4. Im Spiel

Szene mb7raum: mb7_a springt (Wurzelweg 40 hoch/40 vor bei Bild 20, 80 vor bei 30), Auto-Physik an, gebaut in die Karten-GLA
(`phspiel`, `ph_spiel2`), Kamera von der Seite (+Y, Blick -Y). Vorschau je 50 ms, Spiel per `ph/spiel.py` bzw. `ph/orig_spiel.py`
(timescale wirkte nicht - die Fotos liegen je ~0,15 s Echtzeit), Auswertung `ph/vergleich.py` in Bildpunkten (1920 x 1440).
- Movie Duels (`md3`): 6 Spielbilder in der Luft, Abstand zur Bahn der Vorschau Mittel 0,6 px, groesster 1,7 px; Flugdauer
  Vorschau 0,95 s, Spiel 0,93 s; hoechster Kopfpunkt 452 / 460 px (das Spiel traf den Scheitel nicht genau).
- Original-JKA (`orig1`; steam.exe lief, ActiveUser != 0, vor dem Start geprueft): 7 Bilder in der Luft, 0,5 / 1,7 px; Flugdauer
  0,95 / 1,03 s; Kopf 452 / 456 px.
- Paarbilder (oben Spiel, unten Vorschau, 1920 x 2880): `ph/paare_md`, `ph/paare_orig` im Scratchpad, angesehen.
- Spielordner: Movie Duels je Lauf gleich; Original-JKA "GameData" und "GameData original" gleich.

## 5. Bildzeit (allein, BHED_D3D_DEBUG=0)

- mb7zeit alt (Stufe 7, `laeufe/mb7e_leistung_neu`) -> neu, Bild-CPU Mittel/p99: drei Figuren 0,53/1,65 -> 0,54/1,52 ms; animiert
  0,69/2,76 -> 0,56/1,61; Posieren mit Kurven 1,12/1,68 -> 1,11/1,63. Bildabstand 4,2 ms (240 Hz).
- phzeit: drei Figuren springen, Kurven offen, Abspielen: Physik aus 0,70/2,47 ms, an 0,69/1,52 ms (die Spur ist gerechnet, das
  Anwenden kostet nichts Messbares). Model-Tab: eine Rechnung 1,5-2,3 ms nach jeder Aenderung (251 Bilder), nicht waehrend eines Zugs.

## 6. Schluss

Einmal, nach sauberem Neubau (`--clean-first`, /W4 /WX, 0 Warnungen; danach nur Folgebauten des Selbsttests, je 0 Warnungen):

- ctest 228 von 228 (154 s), darunter neu `modell_physik`.
- Laeufe `phe_*` (zwei Wellen, Summe `laeufe/phe_summe.txt`): modellblock funktionen 60/0, tasten 43/0, mb2 37/0, blender 44/0,
  mb3 21/0, rig 26/0, mb5 159/0, mb6 36/2 -> Testfehler (die Pruefung des Menues Animation zaehlte die Gruppe Auto-Physik mit, die im
  eigenen Untermenue steht), behoben, `phe_mb6b` 37/0; mb7 24/0, mb7spiel 4/0, mb7textur 1/0, ph 27/0; rundlauf jka/jk2/md je 2/0,
  rigrundlauf je 4/0, bild je 1/0, kurvenrundlauf je 2/0, ikrundlauf jka 1/0, jk2 1/0, md 2/0; modell 14/0, mission1 98/0,
  kuerzel 202/0, Modus 1 2635/0.
- Handbuch: Seite `modell-physik` (Gruppe Missionen, nach modell-ik) in 6 Sprachen, Bilder `h_ph_sprung` und `h_ph_figur` je Sprache
  (`phe_hb2_*`, je 13/0, 2538 x 1384; en, de, es, cs, ja angesehen), `h_mb7_*` neu (`ph_hb7c`); `gen_handbuch` alle Seiten x 6
  Sprachen, `--pruefen` ok, `gen_hilfe_orte` 206 Orte.
- Lints wie in der Basis (portability 99, pruefcodes 11, efxflags/fremdprogramm/link/tidy unveraendert), alle anderen ok, auch
  lint_sources, lint_i18n, lint_i18n_erzeugt, lint_fassung (rc610).
- Spielordner: Movie Duels je Lauf vorher/nachher gleich; Original-JKA "GameData" und "GameData original" gleich.

## 7. Abweichungen und Offen

- Massen nur fuer Menschenform (Becken, Arme, Beine, Kopf); Kreaturen melden den Grund. Segmente als Punktmassen (ohne
  Gyrationsradien); Hand bis zum entferntesten Fingerknochen, Kopf am Kopfknochen (Gehoergang).
- Federketten: Frequenz 2 Hz und Daempfung 0,3 sind Reglervorgaben ohne Spielwert (das Spiel kennt keine Federknochen).
- Kontakt relativ zum tiefsten Stand in der Sequenz: ganz in der Luft liegende Sequenzen finden ihre tiefsten Bilder als Kontakt -
  dann Absprung/Landung von Hand.
- Waehrend eines Zugs bleibt die alte Physik-Spur stehen (Bildzeit), beim Loslassen rechnet sie neu. Werkzeuge der Oberflaeche, die
  die sichtbare Pose lesen (Pose kopieren), nehmen die Physik mit; Spiegeln und Handpose nicht.
- Das Dopesheet zeigt die Physik-Spur nicht als eigene Zeile (sie steht in der Schicht, Befund im Rollout).
- timescale im Spiel wirkte nicht (Movie Duels und Original), die Spielfotos liegen daher ~0,15 s auseinander; verglichen wird die
  Bahn ohne Zeit (Abstand zur Polylinie der Vorschau) und die Flugdauer.
- Am Ende der Animation zeigt die Vorschau 100 ms lang den Uebergang zur Folgeanimation (Becken 80 vor -> 0, Ueberblendung); im
  Spiel traf kein Foto diese 100 ms - nicht geprueft.
- Die Figur ist in den Spielfotos klein (Kamera 340 Einheiten entfernt, damit der ganze Sprung im Bild ist).
- MD-MP: Daten von shank geloescht, nicht geprueft.

## 8. Vorschlag Stufe 5 (Retargeting)

- Grundlage: Knochen-Erkennung (Stufe 3) als Zuordnung zwischen Skeletten (Rolle + Seite), Bindeposen-Ausgleich je Glied (Drehung
  der Ruhelage), IK-Spuren (Stufe 4) fuer Haende/Fuesse mit den Kontakten aus Stufe 6 (Fuesse bleiben auf dem Boden des Ziels),
  Laengen ueber das Verhaeltnis der Beinlaengen fuer den Wurzelweg.
- Pruefung: GLAs des Spiels auf ein anderes Skelett (JK2 -> JKA, JKA -> MD-Figur mit Menschenform), Fusskontakte ohne Rutschen,
  Schwerpunkt im Stand innen; im Spiel mit Karten-GLA wie Stufe 7, Paarbilder.
- Rot zuerst: ohne Bindeposen-Ausgleich verdrehte Glieder messbar, ohne Laengenausgleich rutschende Fuesse.

## Changelog

### English
- Model tab: auto physics as a filter over the keys - the centre of mass lies on a real parabola with the gravity of the game (g_gravity, 800) between take-off and landing, the angular momentum stays constant in the air, feet on the ground do not slide, and cloaks, coat tails, hair or straps follow through as spring chains. Your keys stay untouched: the physics writes its own track, switch it off and the animation (and the baked GLA, byte for byte) is as before; a strength slider mixes it in.
- The view shows the centre of mass (masses of the body parts after Winter/Dempster), its path over the sequence and the support area of the feet; it turns red and the rollout names the frames when the figure stands with its centre of mass outside the support.
- Take-off and landing come from the foot contacts or are set at the time cursor; rollout "Auto physics" under IK/FK, submenu in the menu Animation and in the right-click, all actions in the search (X / F3).
- Window "Animate figure" in the mission tab: the same auto physics with the floor of the map (traceRay) and the gravity of the map; it goes into the map GLA when building for the game.
- Window "Animate figure": "Build for the game" opens its rollout and scrolls to it, so the result is visible.

### Deutsch
- Model-Tab: Auto-Physik als Filter über den Schlüsseln - der Schwerpunkt liegt zwischen Absprung und Landung auf einer echten Parabel mit der Schwerkraft des Spiels (g_gravity, 800), der Drehimpuls bleibt in der Luft konstant, Füße am Boden rutschen nicht, und Umhänge, Rockschöße, Haare oder Riemen schwingen als Federketten nach. Deine Schlüssel bleiben unangetastet: die Physik schreibt eine eigene Spur, ausgeschaltet ist die Animation (und die gebackene GLA, Byte für Byte) wie vorher; ein Stärke-Regler mischt sie hinein.
- Die Ansicht zeigt den Schwerpunkt (Massen der Körperteile nach Winter/Dempster), seinen Weg über die Sequenz und die Stützfläche der Füße; sie wird rot und das Rollout nennt die Bilder, wenn die Figur mit dem Schwerpunkt außerhalb der Stützfläche steht.
- Absprung und Landung kommen aus den Fußkontakten oder werden am Zeitzeiger gesetzt; Rollout „Auto-Physik“ unter IK/FK, Untermenü im Menü Animation und im Rechtsklick, alle Aktionen in der Suche (X / F3).
- Fenster „Figur animieren“ im Mission-Tab: dieselbe Auto-Physik mit dem Boden der Karte (traceRay) und der Schwerkraft der Karte; sie geht beim Bauen für das Spiel in die Karten-GLA.
- Fenster „Figur animieren“: „Für das Spiel bauen“ klappt sein Rollout auf und rollt zu ihm, damit das Ergebnis zu sehen ist.

### 中文
- 模型选项卡: 自动物理作为关键帧之上的过滤器 - 起跳与落地之间重心沿游戏重力 (g_gravity, 800) 的真实抛物线运动, 空中角动量保持不变, 着地的脚不打滑, 披风、衣摆、头发或带子作为弹簧链随动。你的关键帧保持不变: 物理写入自己的轨道, 关闭后动画 (以及烘焙的 GLA, 逐字节) 与之前相同; 强度滑块用于混合。
- 视图显示重心 (身体各部分质量依据 Winter/Dempster)、其在序列中的路径以及双脚的支撑面; 角色站立时重心在支撑面之外则变红, 卷展栏列出这些帧。
- 起跳与落地来自脚的接触, 或在时间指针处设置; “自动物理”卷展栏位于 IK/FK 下方, Animation 菜单和右键中有子菜单, 所有操作都可在搜索 (X / F3) 中找到。
- 任务选项卡的“为角色制作动画”窗口: 相同的自动物理, 使用地图的地面 (traceRay) 和地图的重力; 为游戏生成时写入地图 GLA。
- “为角色制作动画”窗口: “为游戏生成”会展开并滚动到其卷展栏, 以便看到结果。

### 日本語
- モデルタブ: キーの上のフィルターとしてのオートフィジックス - 踏み切りから着地まで重心がゲームの重力 (g_gravity、800) による本物の放物線をたどり、空中で角運動量が一定、接地した足は滑らず、マント・裾・髪・ストラップがスプリングチェーンとして揺れ残ります。キーはそのまま: 物理は独自のトラックに書き、オフにすればアニメーション (とベイクした GLA、バイト単位) は元どおり。強さスライダーで混ぜます。
- ビューに重心 (体の各部の質量は Winter/Dempster)、シーケンス上の軌跡、足の支持面を表示。重心が支持面の外にある立ち姿勢では赤くなり、ロールアウトがそのフレームを示します。
- 踏み切りと着地は足の接地から決まるか、タイムカーソルで設定。ロールアウト「オートフィジックス」は IK/FK の下、Animation メニューと右クリックにサブメニュー、全操作は検索 (X / F3) にも。
- ミッションタブのウィンドウ「フィギュアをアニメート」: 同じオートフィジックスをマップの床 (traceRay) とマップの重力で。ゲーム用ビルドでマップ GLA に入ります。
- ウィンドウ「フィギュアをアニメート」: 「ゲーム用にビルド」はロールアウトを開いてそこまでスクロールし、結果が見えるようにします。

### Español
- Pestaña Model: física automática como filtro sobre las claves: entre el despegue y el aterrizaje el centro de masa sigue una parábola real con la gravedad del juego (g_gravity, 800), el momento angular es constante en el aire, los pies en el suelo no resbalan, y capas, faldones, pelo o correas siguen el movimiento como cadenas elásticas. Tus claves no se tocan: la física escribe su propia pista; al desactivarla la animación (y el GLA horneado, byte a byte) queda como antes; un control de intensidad la mezcla.
- La vista muestra el centro de masa (masas de las partes del cuerpo según Winter/Dempster), su trayectoria en la secuencia y la superficie de apoyo de los pies; se vuelve roja y el panel indica los fotogramas cuando la figura está de pie con el centro de masa fuera del apoyo.
- Despegue y aterrizaje salen de los contactos de los pies o se fijan en el cursor de tiempo; panel «Física automática» bajo IK/FK, submenú en el menú Animation y en el clic derecho, todas las acciones en la búsqueda (X / F3).
- Ventana «Animar figura» de la pestaña Mission: la misma física automática con el suelo del mapa (traceRay) y la gravedad del mapa; entra en el GLA de mapa al crear para el juego.
- Ventana «Animar figura»: «Crear para el juego» abre su panel y se desplaza hasta él para que se vea el resultado.

### Čeština
- Karta Model: automatická fyzika jako filtr nad klíči - mezi odrazem a dopadem leží těžiště na skutečné parabole s gravitací hry (g_gravity, 800), moment hybnosti zůstává ve vzduchu stálý, chodidla na zemi neklouzají a pláště, šosy, vlasy nebo řemínky dokmitávají jako pružné řetězy. Vaše klíče zůstanou nedotčené: fyzika zapisuje vlastní stopu, po vypnutí je animace (i zapečené GLA, bajt po bajtu) jako předtím; posuvník síly ji přimíchá.
- Pohled ukazuje těžiště (hmotnosti částí těla podle Wintera/Dempstera), jeho dráhu přes sekvenci a opěrnou plochu chodidel; zčervená a rollout uvede snímky, když postava stojí s těžištěm mimo opěrnou plochu.
- Odraz a dopad vycházejí z dotyků chodidel nebo se nastaví na časovém ukazateli; rollout „Automatická fyzika“ pod IK/FK, podnabídka v nabídce Animation a v pravém kliknutí, všechny akce ve vyhledávání (X / F3).
- Okno „Animovat postavu“ v kartě Mission: stejná automatická fyzika s podlahou mapy (traceRay) a gravitací mapy; při sestavení pro hru jde do mapového GLA.
- Okno „Animovat postavu“: „Sestavit pro hru“ rozbalí svůj rollout a posune se k němu, aby byl výsledek vidět.
