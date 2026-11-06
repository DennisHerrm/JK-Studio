# Notizen rc610 / R2b: letzte Reste vor rc610 (Wegpunkte taspir1, Teams, ICARUS, MD-MP-Menues, B4-2)

Auftrag `auftrag_rc610_r2b.md`. Direkt in `Y:\karte` auf `karte1` ab 22de5b62 (R2a), Build `build-test`, Laeufe `laeufe/r2b_*` (vorher `r2b_exe_vor` = 22de5b62, rot `r2b_exe_rot` = 22de5b62 plus die neuen Pruefungen, gruen `r2b_exe_g1`, Schluss `r2be_*`), Laufskript `lauf.py` wie R2a. kFassung bleibt rc609. Kein Spiel gestartet (Punkt 6 siehe dort).

## 1. Wegpunkte taspir1: "1 von 8 Innenraeumen vom Spielerstart erreichbar"

- Ursache gemessen (neue Zeilen im Selbsttest `wegerzeugung`: je Bodenprobe der Startpunkt, von dem aus sie zu Fuss erreichbar ist - erst der Spielerstart, dann je Figur): auf taspir1 erreicht der Spieler zu Fuss nur einen Innenraum (Gebiet 5 im Startstockwerk, z 538..689). Alle anderen Innenraeume liegen in Stockwerken ueber dem Start (z 1300, 1944, 2600), die nur ihre Figuren zu Fuss erreichen (125 NPC auf allen Stockwerken); der Spieler kommt im Spiel per Machtsprung bzw. Aufzug hinauf. Die Pruefung verlangte trotzdem einen Weg der Wegfindung vom Spielerstart - einen, den es zu Fuss nicht gibt. Schon die Wegpunkte der Karte selbst (318, 24 Teile ueber die target-Felder) verbinden die Stockwerke nicht.
- Die Erzeugung war richtig: jeder Raum ist von der Stelle aus, von der er zu Fuss erreichbar ist, ueber die Wegfindung des Spiels (baueNavNetz, navAStar) erreichbar - 25 von 25 (Raum, Startpunkt)-Paaren.
- Behoben: die Pruefung (Name jetzt "... den die Wegfindung des Spiels von dort erreicht, von wo aus er zu Fuss erreichbar ist") prueft je Raum und Startpunkt; der Spielerstart ist mitgezaehlt ("davon zu Fuss vom Spielerstart 1 von 1").
- Unterwegs gefunden: die Meldung der Erzeugung "N Teile des Netzes nicht angebunden" zaehlte jede gescheiterte Kette, auch die, deren Teil eine spaetere Kette doch verband, und viele Teile scheitern an derselben Stelle. taspir1 meldete 3241 Teile (das Netz hat 26 Teile ueber die target-Felder), jeditemple_garden 9 (4 Teile), t1_danger 5 (das Netz haengt ganz zusammen). Jetzt je Teil, der am Ende nicht am Hauptteil haengt, die erste Stelle (`lueckenJeTeil`, `src/wegpunkte.cpp`). Das Netz selbst ist unveraendert.

## 2. Team-Slaves anderer Klassen

- Vorbild: MatchTeam (g_mover.cpp:691-697 OpenJK, :711-717 MD) ruft SetMoverState fuer die ganze teamchain, mit pos1/pos2/trDuration des Slaves; G_RunMover (:565-584) laesst Slaves nicht selbst laufen, G_MoverTeam (:497-562) bewegt jedes Teil nach seiner eigenen Bahn; das cgame zeichnet s.pos/s.apos (cg_ents.cpp). SP_func_bobbing und SP_func_rotating setzen pos1/pos2 nie ((0,0,0)), SP_func_static/func_wall/func_usable beide auf s.origin. Use_BinaryMover behaelt (InitMover :1113): func_door, func_bobbing ohne START_OFF bzw. mit START_OFF ohne targetname beim Spawnen, func_rotating ohne targetname beim Spawnen (:2341-2344; MD :2533-2536).
- Im Nachbau (MoverSim) bestand ein Team nur aus func_door; ein Tuer-Slave eines Meisters anderer Klasse stand immer.
- Behoben (`src/mover.cpp`, `teamSlave`, `binaerTeam`): Slaves anderer Klassen gehoeren zum Team (MatchTeam faehrt sie: func_bobbing und func_rotating auf (0,0,0), func_rotating dreht weiter, func_bobbing pendelt nicht mehr), Meister func_bobbing/func_rotating mit Use_BinaryMover fahren ihre Tueren; das Ausloesefeld nimmt die Kaesten aller Teile (Think_SpawnNewDoorTrigger :1237-1262); die Wegsuche fragt weiter nur func_door.
- Gleich geblieben: func_static/func_wall/func_usable-Slaves (pos1 = pos2), ein func_static-Meister (func_static_use faehrt nichts).

## 3. ICARUS: usescript ohne Namen

- ValidEntity (Q3_Interface.cpp:7572-7592 OpenJK, :8105-8123 MD): jede Entity mit einem behaviorSet bekommt ICARUS (InitEntity), auch ohne targetname/script_targetname (G_NewString(NULL) ist NULL, AssociateEntity traegt sie nicht ein). Ihr usescript laeuft also (G_ActivateBehavior), z. B. bei einem namenlosen Teammeister, dem G_FindTeams den Namen eines Slaves gibt.
- Behoben (`src/ablauf.cpp`, `hatVerhaltensSkript`): `Wesen::gibt` auch ohne ICARUS-Namen, wenn ein Verhaltensskript da ist (NPC-Spawner wie bisher).

## 4. ICARUS: $get$ in jedem Befehl

- Quelle: CTaskManager::Get/GetFloat/GetVector (TaskManager.cpp:382-650 OpenJK) werten ID_GET fuer jedes Argument aus; so lesen Use, Kill, Remove, Signal, WaitSignal, Sound, Set, DeclareVariable, FreeVariable, Play, Print (Get) und Wait, Move, Rotate, Camera (GetFloat/GetVector). Nur run nimmt den Text selbst (CSequencer::ParseRun :421-431), affect/if werten ihn im Sequencer aus (war schon da). MD gleich.
- Behoben: `argText`/`argMs` fuer use, kill, remove, signal, waitsignal, declare, free, sound, set (Name), wait und die Dauer von move/rotate; die Szene bekommt ein Argument, das selbst ein $get$ ist, mit dem Wert in der Art des Typs (`zeichne`).

## 5. Menue-Editor Movie Duels MP

- ITEM_TYPE_MODEL_ITEM (17): Item_Paint (md9 codemp/ui/ui_shared.c:7466) ruft Item_Model_Paint_Item (:6145-6200): fov 45 x 45, Gier realTime / 20, nur ent.hModel = item->asset (ein .glm zeichnet dort nichts), ohne model_angle, model_fov und Saeber-Hack. Die Vorschau zeichnet es jetzt (`vorschau.cpp`, `modell.cpp`, `app_menue.cpp`).
- Gelesen gegen md9 codemp/ui (Zeile fuer Zeile die Tabellen, die Malroutinen der Typen, die Feeder- und Ownerdraw-Listen):
  - itemParseKeywords (86) und menuParseKeywords (35): alle wie im Nachbau (xoffset mit Tabs im Namen, wie bekannt).
  - commandList (:2156-2189): im Nachbau galt jeder Befehl in jeder Fassung. MP hat kein delay, activate, showMenu, openGoToMenu, stopVoice, setitemflag, setitemexec, transition3, increment-/decrementfeeder; MD MP hat playvoice dazu; SP (code/ui/ui_shared.cpp) kein setcvartocvar, setitemrectcvar, setitemcolorcvar, setteamcolor, setplayermodel, scale, disable. Unbekanntes geht an DC->runScript, das das naechste Wort als uiScript liest (:2324-2327). Behoben (`inCommandList`, je Fassung die Liste der Quelle; JK2 MP ohne Quelle wie bisher).
  - Item_Paint: dazu ITEM_TYPE_SLIDER_ROTATE (zeichnet nichts) und INTSLIDER = SLIDER_INTEGER - wie im Nachbau.
  - Sicherer Fund, nicht behoben: die Ownerdraws UI_FORCE_RANK_STASIS .. UI_FORCE_RANK_DARK_STASIS (340-350, menudef.h:349-360, ui_main.c UI_OwnerDraw mit UI_FORCE_RANK_INDEX) - die in MD MP portierten SP-Maechte. Der Nachbau kennt nur 259-276 und 18 Maechte; es braucht die Kraftliste und das Format der Kraft-Zeichenkette von MD MP (ui_force.c, NUM_FORCE_POWERS 29). Offen.
  - Verdachtsfall: die MD-Feeder 0x54-0x59 nimmt der Nachbau aus code/ui (SP); MD MP hat sie in codemp/ui/ui_main.c eigens, dazu FEEDER_HOLOCRON_LIST (0x2d) - nicht Zeile fuer Zeile verglichen.

## 6. B4-2 im Spiel

- Nicht gemacht. Ein Vergleich braucht eine eigene Testkarte (misc_turretG2, Fahrzeug, Reiter per SET_VEHICLE), den Spiellauf mit Bildern zu festen Zeiten und dieselben Bilder aus der Vorschau; das passte nicht mehr in diese Runde. Die Regel ist weiter nur aus der Quelle belegt (F46). Offen.

## 7. Kleinreste aus R2a

- Handbuchbild `h_r2a_spiegel`: ueber dem Spiegel schwebt ein heller Wuerfel (eigene Karte `r2b_spiegel_hb.map`, Bild `textures/r2b/hell` im .pk3 des Testers), im Spiegel deutlich zu sehen; die Karte der Pruefung bleibt.
- misc_model_static: das Spiel zeichnet Halme auch auf Modellflaechen - RB_StageIteratorGeneric ruft RB_DrawSurfaceSprites fuer jede Flaeche mit einer Halm-Stufe (tr_shade.cpp:2173-2183), fuer eine Entity im Raum des Modells (tr_surfacesprites.cpp:1471-1489: ssViewOrigin = ori.viewOrigin, ssViewRight/Up ueber R_WorldNormalToEntity, tr_main.cpp:238-243 - mit vertauschten Achsen und Vorzeichen (-x, -y, +z) gegenueber einer echten Umkehrung). misc_model_static: CG_CreateMiscEnts/CG_DrawMiscEnts (cg_main.cpp:1965-2050), Achsen aus angles mal modelScale (ScaleModelAxis). Nicht nachgebaut: braucht die Halme im Raum des Modells mit diesen Sichtvektoren und eine Testkarte mit .md3 - offen.

## Rot und gruen

| Punkt | Pruefung | rot | gruen |
|---|---|---|---|
| 1 | `wegerzeugung` taspir1, Innenraeume (alte Pruefung: vom Spielerstart) | `r2b_we_vor_taspir1`: 1 von 8 | `r2b_we_diag1`: 25 von 25 (Raum, Startpunkt), vom Spielerstart 1 von 1 |
| 1 | `wegerzeugung` "jede gemeldete Stelle in einem eigenen Teil" (taspir1, jeditemple_garden, t1_danger) | `r2b_we_rot_*`: 3241 Stellen in 1 Teil (26 Teile), 9 in 1 (4), 5 in 1 (1) | `r2b_we_g1_*` 10/0 je Karte: 9 in 9 (26), 3 in 3 (4), 0 (1) |
| 1 | .ent der Erzeugung (SHA-1), vorher/nachher | taspir1 de3d00ea, jeditemple_garden 8c603101, t1_danger 0f9c13e7 | dieselben |
| 2 | ctest `mover` Teil r2bTeams (a)-(d) | 4 FEHL: Slave-Rotation bleibt bei x 100, Bobbing-Slave pendelt (z -32 bei 3000), Tuer-Slave eines func_bobbing/func_rotating steht | 0: x 0 ab dem use (Gier 150 dreht weiter), z 0, Tuer offen bei 1500 und zu bei 6000 bzw. 5000 |
| 3 | ctest `ablauf` Teil r2bIcarus (3) | marke_m1 nie | marke_m1 bei 50 |
| 4 | ctest `ablauf` Teil r2bIcarus (4) | use/signal mit $get$ ins Leere, wait ( $get( FLOAT )$ ) 50 statt 550 | marke_get 0, marke_sig 0, wait wie wait ( 500.000 ): 550 |
| 5 | ctest `menue_f6b` Teil r2bMdMp | 4 FEHL (kein Modell fuer Typ 17, Kamera, delay/showMenu im MD MP, setcvartocvar im SP) | 0 Fehler; Gegenproben MD SP showMenu, OpenJK MP setcvartocvar gruen in beiden |

## Erzeugungszeit taspir1 (allein)

| | vorher (22de5b62) | nachher |
|---|---|---|
| bis zur Übernahme (Hintergrund) | 18,33 s (`r2b_zvor_taspir1`) | 17,91 s (`r2b_znach_taspir1`) |
| ohne Hintergrund | 17,40 s | 17,21 s |

Das Netz ist dasselbe (470 Wegpunkte, 1259 Verbindungen; .ent wie oben), die Zeit unveraendert.

## Schluss

- Sauberer Neubau (`--clean-first`, alle Ziele) 0 Warnungen, 0 Fehler (`r2b/bau_ende.log`).
- ctest -j 16: 220 von 220 (`r2be_ctest.txt`).
- Selbsttests parallel (`r2be_*`): `wegerzeugung` taspir1, jeditemple_garden (MD-pk3), t1_danger je 10/0 (.ent wie vorher: de3d00ea, 8c603101, 0f9c13e7); `karteneditor` fotos 78/0, r2a 21/0; `editorportal` 19/0; `mission1` 98/0; `missionsbau` 95/0; Modus 1 2651/0; ctests `ablauf`, `mover`, `moveranim`, `pruefung3`, `pruefung4b` gruen im ctest-Lauf.
- `menueeditor` JKA (`nur_ja.cfg`) 2778/4: M6 Fluss (ein Menue statt vier offen, unter Volllast) - einzeln wiederholt (`r2be_me_ja_m6`, `BHED_MENUE_NUR=m6`) 17/0.
- `menueeditor` Movie Duels 2778/3: "M7 MP-Startmenue ... ohne Meldung", "rc608 MP-Schriften", "3b Satz: MP: Nur das Testpaket ...". Alle drei haengen am MP-Menuesatz von Movie Duels in den Spieldaten; der Spielordner hat heute kein ui/MD_MP (nur ui/menudef.h in MD_Assets.pk3 und ui/jamp in assets1.pk3). M7 allein (`BHED_MENUE_NUR=m7`) ist mit 22de5b62 (`r2b_me_md_m7_vor`), MODOBJ-Rot (`r2b_m7_mo`) und LICHT1 (`r2b_m7_li1`) genauso rot - nicht von R2b, nicht geaendert.
- Handbuch: `h_r2a_spiegel` in 6 Sprachen neu (BHED_HB_FENSTER=3840x2160@1.5, 1989 bis 2055 breit, angesehen). Kein Text geaendert (Verhalten der Oberflaeche gleich).
- Lints: sources, fassung, i18n, i18n_erzeugt, headers, feldinit, gapp, settings, d3dbesitz, mapstate, imgui_context, rueckkopplung, cbuffer, tabellenid ok; portability 99 (wie R2a).
- Gegen das Vorbild geprueft: alles aus den Quellen (OpenJK code/codemp, Movie Duels Update 9 code/codemp: g_mover.cpp, Q3_Interface.cpp, TaskManager.cpp, Sequencer.cpp, ui_shared.c/.cpp, ui_main.c, menudef.h, tr_shade.cpp, tr_surfacesprites.cpp, tr_main.cpp, cg_main.cpp). Nichts im Spiel (kein Spielstart).

## Offen

- Punkt 5: Ownerdraws 340-350 (MD MP Kraftraenge der portierten SP-Maechte) nicht nachgebaut; MD-MP-Feeder (0x54-0x59, FEEDER_HOLOCRON_LIST) nicht Zeile fuer Zeile gegen codemp gelesen; JK2 MP commandList ohne Quelle.
- Punkt 6: B4-2 nicht im Spiel gegengeprueft.
- Punkt 7: Halme auf misc_model_static in der Echtzeit-Ansicht (das Spiel zeichnet sie) nicht nachgebaut.
- Punkt 2: Reached_BinaryMover laeuft im Spiel je Teil (G_MoverTeam :546-555) - opentarget/closetarget eines Slaves feuern dort; der Nachbau feuert nur die des Meisters, und der Tuerzustand fuer die Wegsuche folgt dem Meister.

## Changelog

### English
- NPC waypoints: after creating a network, the message counts each part that stays unconnected once (taspir1 reported 3241 parts, the network has 26).
- Mission preview: team members of other classes follow their team master like in the game - a func_bobbing or func_rotating in a door's team jumps to its own positions (0 0 0) when the door moves, and a door in the team of a func_bobbing or of a func_rotating without targetname opens when that one is used.
- Mission preview (ICARUS): an entity with a usescript but without a name runs it (for example a nameless team master), and $get( ... )$ works in use, kill, remove, signal, waitsignal, wait, sound, set, declare, free and in the duration of move and rotate.
- Menu editor (Movie Duels multiplayer): ITEM_TYPE_MODEL_ITEM is drawn like in the game (fixed 45 degree view, turning); script commands a game version does not know (for example delay or showMenu in multiplayer, setcvartocvar in single player) do nothing there, like in the game.
- Manual: the mirror picture shows a bright cube in the mirror.

### Deutsch
- NPC-Wegpunkte: Nach dem Erzeugen zählt die Meldung jeden Teil, der ohne Verbindung bleibt, einmal (taspir1 meldete 3241 Teile, das Netz hat 26).
- Missionsvorschau: Teammitglieder anderer Klassen folgen ihrem Teammeister wie im Spiel - ein func_bobbing oder func_rotating im Team einer Tür springt auf seine eigenen Positionen (0 0 0), wenn die Tür fährt, und eine Tür im Team eines func_bobbing oder eines func_rotating ohne targetname öffnet, wenn dieser benutzt wird.
- Missionsvorschau (ICARUS): Eine Entity mit usescript, aber ohne Namen führt es aus (zum Beispiel ein namenloser Teammeister), und $get( ... )$ wirkt in use, kill, remove, signal, waitsignal, wait, sound, set, declare, free und in der Dauer von move und rotate.
- Menü-Editor (Movie Duels Mehrspieler): ITEM_TYPE_MODEL_ITEM wird wie im Spiel gezeichnet (fester Blickwinkel 45 Grad, dreht sich); Skriptbefehle, die eine Spielfassung nicht kennt (zum Beispiel delay oder showMenu im Mehrspieler, setcvartocvar im Einzelspieler), tun dort nichts, wie im Spiel.
- Handbuch: Das Spiegelbild zeigt einen hellen Würfel im Spiegel.

### 中文
- NPC 路径点:生成路网后,提示对每个仍未连接的部分只计一次(taspir1 曾报告 3241 个部分,而路网只有 26 个)。
- 任务预览:其他类别的队伍成员像游戏中一样跟随队长——门所在队伍中的 func_bobbing 或 func_rotating 会在门移动时跳到自己的位置 (0 0 0);func_bobbing 或没有 targetname 的 func_rotating 所在队伍中的门,会在其被使用时打开。
- 任务预览(ICARUS):带 usescript 但没有名称的实体会执行它(例如没有名称的队长),$get( ... )$ 在 use、kill、remove、signal、waitsignal、wait、sound、set、declare、free 以及 move 和 rotate 的时长中生效。
- 菜单编辑器(Movie Duels 多人模式):ITEM_TYPE_MODEL_ITEM 像游戏中一样绘制(固定 45 度视角,旋转);某个游戏版本不认识的脚本命令(例如多人模式中的 delay 或 showMenu、单人模式中的 setcvartocvar)在那里不起作用,与游戏相同。
- 手册:镜子图片在镜中显示一个明亮的立方体。

### 日本語
- NPC ウェイポイント: ネットワーク生成後のメッセージは、接続されないまま残る部分をそれぞれ 1 回だけ数えます(taspir1 は 3241 部分と報告していましたが、ネットワークは 26 部分です)。
- ミッションプレビュー: 他のクラスのチームメンバーがゲームと同じくチームマスターに従います。ドアのチームにある func_bobbing や func_rotating はドアが動くと自身の位置 (0 0 0) に移り、func_bobbing または targetname のない func_rotating のチームにあるドアは、それが使われると開きます。
- ミッションプレビュー(ICARUS): usescript を持つが名前のないエンティティもそれを実行します(例: 名前のないチームマスター)。$get( ... )$ は use、kill、remove、signal、waitsignal、wait、sound、set、declare、free と move・rotate の時間で働きます。
- メニューエディター(Movie Duels マルチプレイ): ITEM_TYPE_MODEL_ITEM をゲームと同じく描画します(45 度の固定視野、回転)。ゲームの版が知らないスクリプトコマンド(例: マルチプレイの delay や showMenu、シングルプレイの setcvartocvar)は、ゲームと同じく何もしません。
- マニュアル: ミラーの画像で、鏡に明るい立方体が映ります。

### Español
- Puntos de ruta de NPC: tras crear una red, el mensaje cuenta una sola vez cada parte que queda sin conectar (taspir1 indicaba 3241 partes; la red tiene 26).
- Vista previa de misión: los miembros de equipo de otras clases siguen a su jefe de equipo como en el juego; un func_bobbing o func_rotating en el equipo de una puerta salta a sus propias posiciones (0 0 0) cuando la puerta se mueve, y una puerta en el equipo de un func_bobbing o de un func_rotating sin targetname se abre cuando este se usa.
- Vista previa de misión (ICARUS): una entidad con usescript pero sin nombre lo ejecuta (por ejemplo un jefe de equipo sin nombre), y $get( ... )$ funciona en use, kill, remove, signal, waitsignal, wait, sound, set, declare, free y en la duración de move y rotate.
- Editor de menús (Movie Duels multijugador): ITEM_TYPE_MODEL_ITEM se dibuja como en el juego (vista fija de 45 grados, girando); los comandos de script que una versión del juego no conoce (por ejemplo delay o showMenu en multijugador, setcvartocvar en un jugador) no hacen nada allí, como en el juego.
- Manual: la imagen del espejo muestra un cubo claro en el espejo.

### Čeština
- Body trasy NPC: po vytvoření sítě zpráva počítá každou část, která zůstane nepřipojená, jen jednou (taspir1 hlásil 3241 částí, síť jich má 26).
- Náhled mise: členové týmu jiných tříd následují vedoucího týmu jako ve hře - func_bobbing nebo func_rotating v týmu dveří skočí na své vlastní pozice (0 0 0), když se dveře pohnou, a dveře v týmu func_bobbing nebo func_rotating bez targetname se otevřou, když je použit.
- Náhled mise (ICARUS): entita s usescript, ale bez jména, ho spustí (například bezejmenný vedoucí týmu), a $get( ... )$ funguje v use, kill, remove, signal, waitsignal, wait, sound, set, declare, free a v délce move a rotate.
- Editor menu (Movie Duels multiplayer): ITEM_TYPE_MODEL_ITEM se kreslí jako ve hře (pevný pohled 45 stupňů, otáčí se); příkazy skriptu, které verze hry nezná (například delay nebo showMenu v multiplayeru, setcvartocvar v singleplayeru), tam nedělají nic, jako ve hře.
- Příručka: obrázek zrcadla ukazuje v zrcadle světlou krychli.
