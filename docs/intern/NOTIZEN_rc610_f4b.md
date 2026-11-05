# Notizen rc610 / F4b: Kartenmechaniken wie im Spiel (Funde B4b-1 bis B4b-15)

Auftrag `auftrag_rc610_f4b.md`, Funde aus `pruefung_b4b6b_funde.md` (Bereich 4b). Direkt in `Y:\karte` auf `karte1` ab 99b0caf2 (F78), Build `build-test`, Laeufe `laeufe/f4b_*` (rot `*_rot`, gruen `*_g1`, Schluss `f4be_*`). kFassung bleibt rc609. Kein Spiel gestartet: das Verhalten ist aus den Quellen belegt (OpenJK `scratchpad\ojk`, Movie Duels Update 9 `scratchpad\md9`; die betroffenen Funktionen sind in beiden bis auf die Schreibweise gleich, verglichen mit normalisiertem Text). Fuer B4b-15 die echte `_humanoid/animation.cfg` aus `assets1.pk3` und `MD_Assets2.pk3` nur gelesen.

Neue Pruefungen: ctest `mover` Teil `f4bTueren` (B4b-1 bis B4b-7, B4b-13 MoverSim), ctest `ablauf` Teil `zielKette` (B4b-8 bis B4b-14), ctest `pmanim` (B4b-15). Rotstand: 99b0caf2 plus die Pruefungen, die neuen Felder ohne Wirkung (`MoverUse::entity/inaktiv`, `Ablauf::Benutzung::entity/inaktiv`) und der Baustein `beruehrtFelder` mit der alten Regel (`st.visible`). Rot 19 Fehler in `mover`, 15 in `ablauf`, 1 in `pmanim`; gruen alle.

Neue Schnittstellen: `startetInaktiv(MapEntity)` (bhed/mover.h, fuer MoverSim und den Nachbau), `beruehrtFelder(Actor, ActorState, ms)` und `Actor::nieGespawnt` (bhed/scene.h), `MoverUse::entity/inaktiv` und `Ablauf::Benutzung::entity/inaktiv` (ein use auf genau eine Entity bzw. SVF_INACTIVE an/aus - der Nachbau gibt sie in der Reihenfolge der Benutzungen an MoverSim).

## Punkt 1: Tueren

- B4b-1: Der Meister entscheidet beim Spawnen mit SEINEM targetname, health und spawnflags ueber das Feld (G_FindTeams laeuft nach dem Spawnen, nur das think des Meisters laeuft); danach bekommt er den targetname des letzten Slaves, die Slaves haben keinen mehr. Rot: (a) Feld fehlte, (b) "a" oeffnete, (c) Feld fehlte. Gruen alle drei.
- B4b-2: Umkehr wie Use_BinaryMover_Go - `trTime = level.time - (total - partial)`, nichtlinear auf derselben sin-Kurve, linear 50 ms zurueck. Dazu trDuration als int (InitMoverTrData). Messung (speed 50, Weg 120, D 2400, Umkehr bei 5650 mitten im Schliessen): nichtlinear trTime 5196, offen ab 7596 (Bild 7600) - vorher Bild 7350; Ort bei 6500 vorher 95,23, jetzt 90,43 = Engine. Linear trTime 4500, offen 6900 (vorher 6850), Ort bei 5700 vorher 62,5, jetzt 60,0 = Engine.
- B4b-5: `beruehrtFelder` statt `st.visible`: SET_INVISIBLE und der Spieler in der Skriptkamera beruehren das Feld, Tote (getoetetMs) nicht, ebenso nichts vor NPC_Begin, nach G_FreeEntity und ein NPC_type ohne .npc (`nieGespawnt`). Rot: alle vier falsch (die Tuer mit der Leiche im Feld blieb offen). Gruen: offen bei 900, zu bei 5000.
- B4b-6: Feld = Lump +-2 (CMod_LoadSubmodels + SV_LinkEntity), +-120 auf der duennsten Achse. Rot: Figur bei x -120 (1 ausserhalb von Lump - 120) ausserhalb; gruen im Feld.
- B4b-7: INACTIVE (128) und SVF_INACTIVE am Meister blocken use und Feld (Use_BinaryMover, auch aus Touch_DoorTrigger); target_activate gibt frei. Rot: use und Feld oeffneten bei 1000/500. Gruen: zu bis zur Freigabe bei 3000, use bei 4000 oeffnet.

## Punkt 2: andere Mover

- B4b-3: func_bobbing ohne START_OFF: Use_BinaryMover - ab dem use auf pos1 = (0,0,0), fuer immer; "target" wird benutzt; "delay" ohne mal 1000. Rot: z 132 (angehalten) statt 0, target nicht benutzt. Gruen: 0,0 bei 1050 und 3000, func_wall-Ziel sichtbar ab 1000. Die weitere Bahn (opentarget, target2, closetarget) aendert den Ort nicht und wird nicht gerechnet; eine Umkehr rechnet die Engine dort mit NaN.
- B4b-4: func_rotating aus = Spawnwinkel (EvaluateTrajectory auf TR_STATIONARY), an = trBase + speed * t (trTime 0). Rot: 180 bei 3000, 225 bei 5500. Gruen: 0 und 135.
- Geaenderte alte Erwartungen in ctest `mover` (Quelle im Test): "abgeschaltet bleibt der Winkel stehen" 90 -> 0, "nach use dreht es" 45 -> 135 (func_rotating_use aendert nur trType, cg_ents.cpp:1896).

## Punkt 3: target-Kette

- B4b-8: "delay" F_INT bei trigger_multiple/once, target_relay, func_door (MoverSim auch target_relay). Rot: trigger 500 statt 0, Relais 1500 statt 1000 ms nach dem use. Gruen: 0 und 1000.
- B4b-9: target_relay - mit delay nur das think (ein zweites use verschiebt, nie gesperrt, nie verbraucht), usescript erst nach dem delay, RANDOM ein Ziel (G_PickTarget), Flag 1/2 mit einem Client als Ausloeser: nichts. Rot: (a) nur 1000 statt 1000/6050, (b) 1000 statt 1550, (c) 3 von 3 Spawnern, (d) usescript bei 50 statt 2000, (e) feuerte. Gruen alle.
- B4b-10: target_delay hat ein think - zweimal benutzt feuert einmal (3000 ms nach dem zweiten use). Rot: 3000 und 4050; gruen 4050.
- B4b-11: target_counter mit bounceCount (und 128 nach dem Feuern). Rot: nur beim 3. use; gruen beim 3. und 6.
- B4b-12: target_scriptrunner sperrt ueber nextthink, count und wait erst im Lauf (scriptrunner_run). Rot: (a) Laeufe 2000 und 3050, (b) 2000 und 8100. Gruen: je ein Lauf bei 2000.
- B4b-13: GEntity_UseFunc - SVF_INACTIVE blockt jedes use im Nachbau (auch usescript, NPC_spawner, target_delay, target_counter, Figuren) und in MoverSim; die Anfangswerte aus 128 nur dort, wo die Spawnfunktion sie setzt (vorher auch target_delay). target_(de)activate fuehrt jetzt auch sein usescript aus. Rot: Spawn trotz target_deactivate, target_delay feuerte trotz SET_INACTIVE (Rotlauf mit script_targetname wiederholt: ohne ihn lief affect wie im Spiel ins Leere), func_wall schaltete. Gruen alle.
- B4b-14: target_random neu (usescript, USEONCE, ein Ziel immer, sonst eins). Zufall wie G_PickTarget/Q_irand, aber wiederholbar (xorshift ab festem Anfang). Rot: 0 Laeufe; gruen je 1.

## Punkt 4: B4b-15

- Nachgestellt, kein Verdacht mehr: BOTH_WALKBACK_STAFF und BOTH_RUNBACK_DUAL haben auch in der echten `_humanoid/animation.cfg` -20 fps (assets1.pk3, MD_Assets2.pk3, z_zzzz_MD_Jace-Update-8.pk3). PM_SetAnimFinal passt bei animSpeed < 0 nicht an; `pmFussTempo` nimmt frameLerp jetzt mit Vorzeichen, die Aufnahme reicht es durch. Rot: BOTH_WALKBACK_STAFF mit 64 -> 0,64; gruen 1,0. BOTH_RUNBACK_DUAL war schon vorher 1,0 (steht nicht in PM_RunningAnim).

## Schluss

- Sauberer Neubau (`--clean-first`, nach dem Neustart des PCs 16:31; der Lauf davor brach mit dem Neustart ab und zaehlt nicht): 0 Warnungen, 0 Fehler.
- ctest -j 16: 218 von 218 (darin `ablauf`, `mover`, `moveranim`, `pruefung4b`, `pmanim`, `pruefmission`). Danach nur `tests/mover.cpp` um `<algorithm>` ergaenzt (lint_portability), `mover_probe` neu gebaut: 0 Warnungen, alle Gegenproben bestanden.
- Selbsttests (parallel zu ctest, `laeufe/f4be_*`): `pruefung3` 9/0, `mission1` 98/0, `missionsbau` 95/0, Modus 1 2651/0.
- Lints: sources, fassung, i18n, i18n_erzeugt, headers, feldinit, gapp, settings, d3dbesitz, mapstate, imgui_context, rueckkopplung ok; portability 98 (F78: 99; die zwei neuen Funde in tests/mover.cpp behoben).
- Handbuch: nicht geaendert - es beschreibt Tueren und Mover allgemein ("stehen, wo das Spiel sie in diesem Moment hat"); das stimmt jetzt genauer, ein Text dazu fehlt nicht. Keine neuen Programmtexte.

## Offen

- Gegen das Spiel gemessen wurde nichts (keine Spielstarts); belegt aus den Quellen.
- currentOrigin bei der Umkehr: die Vorschau nimmt den Ort zur Zeit des use; die Engine den aus dem letzten G_RunMover (je nach Platz des Benutzers im Bild das vorige Bild).
- Neu gesehen, nicht geaendert (nicht in den Funden, nicht nachgestellt): UnLockDoors loescht den targetname einer nicht-TOGGLE-Tuer (ein zweites use per Name oeffnet im Spiel nicht, ctest `mover` erwartet das Gegenteil); ein zweites use einer Tuer mit delay verschiebt im Spiel das think, die Vorschau behaelt den ersten Termin; target_delay "random" bleibt 0; der Teammeister ist im Spiel die erste Entity mit dem "team"-Namen jeder Klasse (die Vorschau zaehlt nur Tueren); im Nachbau laeuft das usescript einer Tuer noch unter dem Namen eines Slaves; target_change_parm fehlt weiter (der Titel von B4b-14 nennt es, der Auftrag nur target_random); Movie Duels SP_func_rotating mit "spinangles"/health; LOCKED/GOODIE-Bits an einem func_bobbing.
- Die Kandidaten fuer G_PickTarget/target_random sind nur Karten-Entities, keine gespawnten NPCs mit gleichem targetname.

## Changelog

### English
- Map preview: door teams behave like the game - the trigger field and health of the first door decide, and only the last name given in the team opens it.
- Map preview: a door that reverses halfway keeps moving on the same curve and arrives when it does in the game; a door's trigger field has its full game size.
- Map preview: invisible figures and the player during a script camera open doors like in the game; dead figures no longer hold a door open.
- Map preview: entities switched off with INACTIVE, target_deactivate or SET_INACTIVE ignore every use until switched on again (doors, spawners, delays, counters and more).
- Map preview: a func_bobbing without START_OFF stops at its origin when used, and a switched-off func_rotating shows its spawn angles, as in the game.
- Script preview: "delay" of relays and triggers counts whole seconds; relays with delay, RANDOM and the player-only flags, target_delay used twice, target_counter with bounceCount and target_scriptrunner with count, wait and delay now fire as in the game; target_random is new.
- Recording: backwards walk and run animations with negative frame rates keep their own speed, as in the game.

### Deutsch
- Kartenvorschau: Tuerteams verhalten sich wie im Spiel - Ausloesefeld und health der ersten Tuer entscheiden, und nur der zuletzt im Team vergebene Name oeffnet sie.
- Kartenvorschau: Eine Tuer, die mitten im Weg umkehrt, faehrt auf derselben Kurve weiter und kommt an, wann sie es im Spiel tut; das Ausloesefeld einer Tuer hat seine volle Groesse aus dem Spiel.
- Kartenvorschau: Unsichtbare Figuren und der Spieler waehrend einer Skriptkamera oeffnen Tueren wie im Spiel; Tote halten eine Tuer nicht mehr offen.
- Kartenvorschau: Mit INACTIVE, target_deactivate oder SET_INACTIVE abgeschaltete Entities ueberhoeren jedes use, bis sie wieder an sind (Tueren, Spawner, Verzoegerungen, Zaehler und mehr).
- Kartenvorschau: Ein func_bobbing ohne START_OFF bleibt bei use an seinem Ursprung stehen, und ein abgeschaltetes func_rotating zeigt seine Spawnwinkel, wie im Spiel.
- Skriptvorschau: "delay" von Relais und Triggern zaehlt ganze Sekunden; Relais mit delay, RANDOM und den Nur-Spieler-Flags, zweimal benutzte target_delay, target_counter mit bounceCount und target_scriptrunner mit count, wait und delay feuern jetzt wie im Spiel; target_random ist neu.
- Aufnahme: Rueckwaerts gespielte Geh- und Laufanimationen mit negativer Bildrate behalten ihr eigenes Tempo, wie im Spiel.

### 中文
- 地图预览:门组的行为与游戏一致——由第一扇门的触发区域和 health 决定,只有组内最后指定的名称才能打开它。
- 地图预览:中途反向的门会沿同一条曲线继续移动,并在与游戏相同的时间到达;门的触发区域具有游戏中的完整大小。
- 地图预览:隐身角色和脚本镜头期间的玩家会像游戏中一样打开门;死亡的角色不再使门保持打开。
- 地图预览:通过 INACTIVE、target_deactivate 或 SET_INACTIVE 关闭的实体会忽略所有 use,直到重新开启(门、生成器、延迟、计数器等)。
- 地图预览:没有 START_OFF 的 func_bobbing 被使用时停在其原点,关闭的 func_rotating 显示其生成时的角度,与游戏一致。
- 脚本预览:中继和触发器的 "delay" 按整秒计算;带 delay、RANDOM 和仅玩家标志的中继、被使用两次的 target_delay、带 bounceCount 的 target_counter 以及带 count、wait 和 delay 的 target_scriptrunner 现在与游戏中一样触发;新增 target_random。
- 录制:帧率为负的倒走和倒跑动画保持其自身速度,与游戏一致。

### 日本語
- マッププレビュー: ドアのチームがゲームと同じように動作します。最初のドアのトリガー範囲と health で決まり、チーム内で最後に付けられた名前だけが開けます。
- マッププレビュー: 途中で反転したドアは同じカーブのまま動き続け、ゲームと同じ時刻に到着します。ドアのトリガー範囲はゲームと同じ大きさになりました。
- マッププレビュー: 不可視のキャラクターとスクリプトカメラ中のプレイヤーは、ゲームと同じようにドアを開けます。死んだキャラクターがドアを開けたままにすることはなくなりました。
- マッププレビュー: INACTIVE、target_deactivate、SET_INACTIVE で無効にしたエンティティは、再び有効になるまですべての use を無視します(ドア、スポナー、ディレイ、カウンターなど)。
- マッププレビュー: START_OFF のない func_bobbing は use で原点に止まり、停止した func_rotating はスポーン時の角度を表示します(ゲームと同じ)。
- スクリプトプレビュー: リレーとトリガーの "delay" は整数秒で数えます。delay、RANDOM、プレイヤー専用フラグ付きのリレー、2 回使われた target_delay、bounceCount 付きの target_counter、count・wait・delay 付きの target_scriptrunner がゲームと同じように発火します。target_random に対応しました。
- 録画: フレームレートが負の後ろ歩き・後ろ走りアニメーションは、ゲームと同じく本来の速度のまま再生されます。

### Español
- Vista previa del mapa: los equipos de puertas se comportan como en el juego: deciden el campo de activación y la salud de la primera puerta, y solo el último nombre asignado en el equipo la abre.
- Vista previa del mapa: una puerta que se invierte a mitad de camino sigue por la misma curva y llega cuando lo hace en el juego; el campo de activación de una puerta tiene su tamaño completo del juego.
- Vista previa del mapa: las figuras invisibles y el jugador durante una cámara de script abren puertas como en el juego; las figuras muertas ya no mantienen una puerta abierta.
- Vista previa del mapa: las entidades desactivadas con INACTIVE, target_deactivate o SET_INACTIVE ignoran todo use hasta que se reactivan (puertas, generadores, retardos, contadores y más).
- Vista previa del mapa: un func_bobbing sin START_OFF se detiene en su origen al usarlo, y un func_rotating apagado muestra sus ángulos iniciales, como en el juego.
- Vista previa de scripts: el "delay" de relés y disparadores cuenta segundos enteros; los relés con delay, RANDOM y las banderas solo para el jugador, target_delay usado dos veces, target_counter con bounceCount y target_scriptrunner con count, wait y delay se disparan ahora como en el juego; target_random es nuevo.
- Grabación: las animaciones de caminar y correr hacia atrás con frecuencia de fotogramas negativa mantienen su propia velocidad, como en el juego.

### Čeština
- Náhled mapy: týmy dveří se chovají jako ve hře - rozhoduje spouštěcí pole a health prvních dveří a otevře je jen jméno, které bylo v týmu zadáno jako poslední.
- Náhled mapy: dveře, které se v půli cesty obrátí, pokračují po stejné křivce a dorazí ve stejnou chvíli jako ve hře; spouštěcí pole dveří má plnou velikost ze hry.
- Náhled mapy: neviditelné postavy a hráč během skriptové kamery otevírají dveře jako ve hře; mrtvé postavy už dveře nedrží otevřené.
- Náhled mapy: entity vypnuté pomocí INACTIVE, target_deactivate nebo SET_INACTIVE ignorují každé use, dokud se znovu nezapnou (dveře, spawnery, zpoždění, počítadla a další).
- Náhled mapy: func_bobbing bez START_OFF se po use zastaví ve svém počátku a vypnutý func_rotating ukazuje své úhly ze spawnu, jako ve hře.
- Náhled skriptu: "delay" relé a triggerů počítá celé sekundy; relé s delay, RANDOM a příznaky jen pro hráče, dvakrát použitý target_delay, target_counter s bounceCount a target_scriptrunner s count, wait a delay se nyní spouštějí jako ve hře; target_random je nový.
- Záznam: animace chůze a běhu pozpátku se zápornou snímkovou frekvencí si zachovávají vlastní rychlost, jako ve hře.
