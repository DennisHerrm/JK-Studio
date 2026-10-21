# NOTIZEN rc605 (Branch fix605, auf rc604 8cc7bd1)

Wege/Bewegung: die neuen nie-angekommen-/Pendel-Fälle der rc604-Übersicht, die Welt-/STEER-Fälle der offenen
Liste und die Auslöser-Parameter von vjun3. Ein Prozess zur Zeit (Proben `tuerlauf` mit `BHED_MISSION` /
`BHED_PROBE` / `BHED_PROBE_TAKTE`), je Punkt gezielter Rot/Grün-Test und ctest (67/67).

Commits: f0abd7c (Parameter des Ausführenden, move zum TAG), 6b8a7fe (Übersicht nennt Szenenende/Kampf),
e34d515 (Handbuch), danach diese Notizen.

## 1. Neue Fälle der rc604-Übersicht (md_gd_jedi/sith, t3_stamp, kor2, taspir2, t3_rift, md_fh1_jedi)

Vergleich rc604-Vorabend (Lauf `alleend`) mit rc604-Ende (`finalle`): neu waren Figuren, die vorher standen und
seit den NPC-Sprüngen/`$tag()` laufen. Proben pgd1, pstamp, ptasp, prift, pkor2, pfh1.

### md_gd_jedi / md_gd_sith ani1 (start_cin1, cin1_jedi, cin1_sith) - Startzustand des Reiters, sonst wie im Spiel

- cin1_*: `SET_BEHAVIOR_STATE BS_JUMP`, `SET_NAVGOAL ani_nav10` (-1184 -477 -25). Im Reiter steht ani1 an seinem
  Spawner (821 609 -28): 2280 Einheiten.
- Der Bogen ist genau NPC_BSJump (NPC_behavior.cpp:717-850 MD): Ziel höher -> p1 = Ziel, apexHeight 100,
  z = sqrt(100 + 6,88) - 10 = 0,338, xy = (2280 - 0,338) / 2 = 1139,8, height 106,9, time = sqrt(106,9 / 400) =
  0,517 s, forward 2205, vz 413,6. Probe: vel -1938,6 -1050,0, vz 373,8 nach einem Bild Schwerkraft - gleich.
- Er trifft bei x ~ 318 die Wand des Gangs (ani_nav0-2 bei y 611), rutscht bis y 132 und fällt in die Grube
  (z -962). Im Spiel töte ihn dort trigger_hurt *1 (dmg 10000, z -302..-254) -> deathscript (Jedi:
  md_gd/ani_death = SET_MISSIONFAILED, Sith: md_gd/start_cin2). trigger_hurt bildet die Vorschau nicht nach (offen).
- Ursache ist der Startzustand: in der Mission haben intro_jedi/intro_sith ani1 vorher über ani_nav2 -> 6 -> 7
  -> 8 -> 9 (-1608 -14 16) geführt, cin1 springt von dort ~630 Einheiten. Der Reiter beginnt beim Kartenstart.
- cin1_jedi: der Spieler übernimmt 2 s nach dem Sprung den Ort von ani1 und `remove ( "ani1" )` (8550 ms) - die
  Übersicht zählte 120 s Rutschen in der Grube nach dem Entfernen als Pendeln. Jetzt markiert
  ("; entfernt bei 8550").

### t3_stamp cannon_fodder, taspir2 jumper3, kor2 jumper2/jumper4, t3_rift npc_jumpgoon1 - bis SET_ENEMY wie im Spiel

- Alle Skripte: `BS_JUMP` + Navgoal, `wait 777` (t3_rift 2100), `BS_DEFAULT`, `SET_ENEMY player` (t3_stamp:
  `SET_ENEMY beast`, der Rancor).
- Der Bogen: t3_stamp vom Spawn (728 408 664) zu fj (672 552 448): p1 = NPC, z = sqrt(316) - 10 = 7,78,
  xy = (154,5 - 7,78) / 2 = 73,4, time 0,5 s, forward 146,8, vz 400 - Probe vel -53,2 136,8 (|v| 146,8). Er fliegt
  über fj hinweg (bei fj in z 643) und landet in der Grube (z 88): fj ist nur der Zielpunkt
  (SET_NO_FALLTODEATH true - das Skript rechnet mit dem Fall). Ebenso nav_jumpgoon1 (105 über dem Boden), j3 (+104),
  j2 (+672): die Klasse "Navgoal in der Luft" aus rc605 FU3.
- Nach `SET_ENEMY` wählt die Kampf-KI den Weg: Reborn/Cultist sind Jedi-Klassen, NPC_BehaviorSet_Jedi ->
  NPC_BSJedi_Default -> Jedi_Attack -> Jedi_Combat; Jedi_Advance setzt goalEntity = enemy (AI_Jedi.cpp) - abhängig
  vom Spieler bzw. vom Rancor und von Q_irand, nicht nachgebildet. Die Vorschau geht weiter zum alten Navgoal und
  pendelt darunter. Jetzt markiert ("; Feind (Kampf-KI) bei ...").

### md_fh1_jedi starkiller (transition) - Kartenwechsel vor der Ankunft, wie im Spiel

- transition: `use ( "levelchange" )` nach 11500 ms Skriptzeit (im Ablauf 11750 ms) -> target_level_change
  (mapname md_fh2_jedi) -> G_ChangeMap schickt `maptransition` (g_target.cpp target_level_change_use / g_utils.cpp
  G_ChangeMap OpenJK): die Karte endet. starkiller ist dann bei y ~ 2076 unterwegs.
- Was die Vorschau danach zeigt (nach dem Kartenwechsel, im Spiel unsichtbar): bei y 2100 (Knoten 18) blockiert,
  nach 2 s NPC_TryJump (SCF_NAV_CAN_JUMP, g_navigator.cpp:4216-4310 MD) auf den Absatz, landet über nav3 und springt
  immer wieder (64 Absprünge, 27 Umkehren) - siehe Punkt 2. Jetzt markiert ("; Kartenwechsel bei 11750").

## 2. Welt-/STEER-Fälle der offenen Liste

| Fall | Befund | Ergebnis |
|---|---|---|
| md_fh1_jedi starkiller, Knoten 18 -> nav3 | Gegen die Brushes über die Pmove-Spur selbst: unterer Boden Ursprung z -83,9 (Füße -107,9) bei Knoten 18, der Absatz Ursprung z -4,9 (nach dem Sprung gemessen): 79 Einheiten, mehr als STEPSIZE 18 (PM_StepSlideMove) - nur per Sprung. NAV::GoTo hat keine Kante dorthin -> STEER::Stop wie NPC_MoveToGoal (NPC_move.cpp:934-950 MD). nav3 (0 2270 -46,875) liegt 18 Einheiten IM Boden des Absatzes (SP_waypoint_navgoal meldet das per G_CheckInSolid, g_nav.cpp:260-283 MD) und 42 unter dem Ursprung oben: STEER::Reached (g_navigator.cpp:5369-5386 MD: 3D-Abstand < Radius 12 oder Punkt im absmin/absmax) wird nie wahr. | Wie im Spiel; zudem wechselt die Karte bei 11,75 s, bevor er Knoten 18 erreicht. |
| t2_rancor npc_rancor1 | Letztes Stück STEER::GoTo direkt (SafeToGoTo wahr), Persue mit Bremsweg goalRadius x 4 = 320 (escape_nav1 radius 80; STEER::GoTo g_navigator.cpp:4407-4470 MD): ps.speed 55 -> 23. Bei 46450 scheitert SafeToGoTo (der nächste Knoten wechselt), NAV::GoTo findet für SC_LARGE (Radius 60) keine Kante -> STEER::Stop, 137,6 vor dem Ziel (erreicht erst unter 80). | Codepfad gleich; im Spiel nähme der Rancor außerdem den Spieler wahr (Rancor-KI). Keine Abweichung gefunden. |
| md_ta_sith vader2 (vader_storage_spawn) | spawnscript-Reiter: SET_WALKING, SET_NAVGOAL vader2_nav1 (144 weit). Probe pvad: er geht ab 0 (STEER::GoTo, weg 1) und steht ab 550 bei x 4895 - vor der func_door storage_door (*34, x 4876-4879, spawnflags 8). Eine Tuer mit targetname bekommt kein Ausloesefeld (SP_func_door: Think_MatchTeam statt Think_SpawnNewDoorTrigger, g_mover.cpp:1597-1607 MD) und oeffnet nur per use; das tut storage_scene (`use storage_door` und `use vader_storage` im selben Bild) - dort kommt vader2 an. | Wie im Spiel (Startzustand des einzelnen Reiters: Tuer zu). |
| md_ta_sith rebel5_intro | Probe 11900-12400: blockiert seit 10700 an der Wand x 3106, gleitet in der Kehle Wand/Rampe mit vy = -vz (45 Grad), boden 0 auf der Rampe, zurück auf den Boden z 16,1, Takt 0,4 s, kommt bei 66650 an. Eine 45-Grad-Rampe (n_z 0,707) wäre begehbar (MIN_WALK_NORMAL 0,7, bg_pmove.cpp:7439/7524 MD); die Bodenspur liefert in der Kehle eine steilere Ebene (vermutlich eine Abschrägungsseite des Brushes) - dieselben CM-Daten wie im Spiel. | Keine Abweichung im Code gefunden, ohne Spiel nicht weiter zu prüfen. |

## 3. vjun3 elevator_up - Parameter des Auslösers (behoben)

Der Scriptrunner t133 (`parm1` "first_elevator", usescript vjun3/elevator_up, ausgelöst vom trigger_once *137)
führt das Skript auf sich selbst aus. `affect ( $get( STRING, "SET_PARM1" )$ )` liest seinen parm1; im Block liest
`move ( $tag( get( STRING, "SET_PARM1" ), ORIGIN )$, ... )` den parm1 des Aufzugs (func_static first_elevator,
`parm1` "first_tag", ref_tag bei z 1336). Deterministisch.

Engine: G_ParseField "parm1".."parm16" -> Q3_SetParm (g_spawn.cpp:382-406 / 1187 MD, Q3_Interface.cpp:4331);
NPC_Spawn_Do kopiert die parms des Spawners (NPC_spawn.cpp:2761-2767 MD); CQuake3GameInterface::GetString
SET_PARM1..16 (Q3_Interface.cpp:11598-11625 MD); affect wertet mit dem Besitzer des Sequencers aus
(CSequencer::ParseAffect), get in einem Task mit dem Besitzer des Task-Managers (CTaskManager::Get,
TaskManager.cpp:637-652 OpenJK); move mit TAG über GetTag.

Behebung (src/ablauf.cpp, src/scene.cpp): jede Entity bekommt ihre parm-Schlüssel (ladeParms), der Träger ist
genau die Entity mit dem usescript (ablaufTraeger liefert sie; in vjun3 heißen auch NPCs "t133"), affect mit
Ausdruck wird ausgewertet, get innerhalb eines Ausdrucks beim Ausführen mit den parms des Ausführenden eingesetzt
(Eintrag::eigen), und move nimmt `$tag( "name", ORIGIN )$` als Ziel einer Brush-Entity.

| Test | Rot (vorher) | Grün (nachher) |
|---|---|---|
| ablauftest "Ausloeser-Parameter" | 3 FEHLER (kein affect auf first_elevator, kein move) | ok: Träger t133, move tag( "first_tag", ORIGIN ) bei 350 ms (= mit festen Namen) |
| scenetest "move zum TAG" | FEHL: z 100 -> 100 | ok: z 100 -> 228 |
| GUI tuerlauf vjun3 (pvj1) | Aufzug fuhr nicht | Aufzug fährt (Mover-Plan "3 Brush-Entities bewegen sich"); die drei Soldaten kommen trotzdem nicht an - siehe offen (sie haben ab 9750 ms SET_ENEMY player und SET_CHASE_ENEMIES true, also ohnehin Kampf-KI) |
| Übersicht md_dotf (Nebenwirkung, gewollt) | Kraftfelder (move zu TAGs) standen | fahren wie im Spiel; mau6 geht seinen ganzen Weg (siehe Übersicht) |
| ctest | - | 67 / 67 |

Offen dazu: die Soldaten stehen auf dem Aufzug (Spawn z 1240) und laufen erst bei 9800 ms los; der Wegplan
(planeLauf) trägt Figuren nicht mit einem fahrenden Brush (G_MoverPush, g_mover.cpp) - sie starten am Spawnort
und fallen auf z 1224 (Aufzug am Spawnort). Ein Port von G_MoverPush in den Wegplan ist ein eigener Punkt.

Handbuch (figuren, vier Sprachen): Skripte lesen die parm-Schlüssel ihrer Entity, move zu einem TAG;
`gen_handbuch.py --pruefen` ok.

## 4. Übersicht: Szenenende und Kampf benennen (Werkzeug)

Die Übersicht (`tuerlauf`) markiert nie angekommene/pendelnde Figuren, deren Auswertung über einen dieser
Zeitpunkte hinausgeht: `remove` der Figur, `use` auf einen target_level_change, `SET_ENEMY` (ab da Kampf-KI). Die
Summenzeile zählt die drei Gruppen zusätzlich; die bisherigen Zähler bleiben, damit die Summen vergleichbar sind.
Geprüft mit md_fh1_jedi, md_gd_jedi, t3_stamp (Lauf ueb1, 8 OK / 0 FEHLER): starkiller "Kartenwechsel bei 11750",
ani1 "entfernt bei 8550/8600", cannon_fodder "Feind (Kampf-KI) bei 800/63850".

## Changelog

- EN: Scripts started by an entity read that entity's parm keys: `affect ( $get( STRING, "SET_PARM1" )$ )` and `move ( $tag( get( STRING, "SET_PARM1" ), ORIGIN )$, ... )` move the entity and tag named there, like the game (the first elevator in vjun3 now rises); `move` also takes a tag as its destination.
- DE: Skripte, die eine Entity startet, lesen deren parm-Schlüssel: `affect ( $get( STRING, "SET_PARM1" )$ )` und `move ( $tag( get( STRING, "SET_PARM1" ), ORIGIN )$, ... )` bewegen die dort genannte Entity zum genannten Tag, wie im Spiel (der erste Aufzug in vjun3 fährt jetzt hoch); `move` nimmt auch ein Tag als Ziel.
- ZH: 由实体启动的脚本会读取该实体的 parm 键:`affect ( $get( STRING, "SET_PARM1" )$ )` 和 `move ( $tag( get( STRING, "SET_PARM1" ), ORIGIN )$, ... )` 会像游戏中一样把其中指定的实体移到指定的标记(vjun3 的第一部电梯现在会上升);`move` 也接受标记作为目标。
- JA: エンティティが開始したスクリプトは、そのエンティティの parm キーを読みます。`affect ( $get( STRING, "SET_PARM1" )$ )` と `move ( $tag( get( STRING, "SET_PARM1" ), ORIGIN )$, ... )` はゲームと同じく、指定されたエンティティを指定のタグへ動かします(vjun3 の最初のエレベーターが上昇するようになりました)。`move` はタグも目的地として受け付けます。

## Übersicht über alle Missionen (vorher / nachher)

Lauf ue605end (`tuerlauf`, BHED_MISSION=alle, eine Instanz, Spitze 2,7 GB, rund 30 min), verglichen mit dem
rc604-Endlauf (finalle):

| | rc604 (finalle) | rc605 (ue605end) |
|---|---|---|
| Missionen / Reiter | 163 / 1781 | 163 / 1781 |
| Figuren mit Weg | 983 | 983 |
| nie angekommen | 174 | 174 |
| pendelnd | 25 | 29 |
| Denkbild-Sprünge (Prüfung) | 0 | 0 |
| NPC-Absprünge | 191 | 191 |
| Knopftest | 326 OK / 3 FEHLER (vor dem Fix in rc604) | 329 OK / 0 FEHLER |

Einzige Änderung je Mission: md_dotf_jedi und md_dotf_sith pendelnd 0 -> 2 (je zwei Reiter cin5/start_cin5,
mau6, Klasse "Skript wechselt Ziele" - Umkehren nur zwischen Plänen, wie im Spiel). Ursache ist Punkt 3: die
Kraftfelder (forcefields1: `move ( $tag( "al_closed", ORIGIN )$, 10.000 )` ...) fahren jetzt wie im Spiel zu ihren
TAGs; vorher standen sie am Spawnort, mau6 blieb nach 5 Wegstücken "ohne Weg (nie)" 193 vor seinem Ziel stehen,
jetzt geht er alle 21 Stücke (20 angekommen, das letzte 15 vor dem Ziel beim Schnitt) und kehrt dabei mit den
Zielen des Skripts um. Die Zahl der nie angekommenen bleibt 174 (mau6 zählte vorher wie nachher nicht als nie
angekommen - er kam vorher in 3 Stücken an).

Neu markiert (von den nie angekommenen / pendelnden, Punkt 4): vorher entfernt 4 (md_gd_jedi ani1 x2, md_dotf_jedi
qui2 x2), nach Kartenwechsel 21 (md_jt_intro 16 - die zehn Soldaten mit demselben Ziel u. a., md_po_jedi/sith 4,
md_fh1_jedi starkiller), mit Feind 20 (taspir2 5, vjun3 3 - elevator_up setzt SET_ENEMY player vor den Navgoals,
t3_stamp 2, md_ta_sith 2, md_toaj_jedi 2, hoth2, kor1, kor2, t1_rail, t2_dpred, t3_rift je 1). Befund-Klassen
(Zeilen): Zeit zu kurz 79, bleibt stehen 61, langsamer als gerade 33, Skript wechselt Ziele 8, pendelt im Plan 6,
steht 1.

## Noch offen

- trigger_hurt tötet keine Figuren (md_gd ani1 in der Grube; im Spiel deathscript/Mission gescheitert).
- Wegplan ohne G_MoverPush: Figuren auf einem fahrenden Brush (vjun3 intro_trooper*) werden nicht mitgetragen.
- Kampf-KI nach SET_ENEMY (Jedi_Attack, Rancor) - spielerabhängig, nicht nachgebildet; die Vorschau geht zum alten
  Navgoal weiter.
- Ein Kartenwechsel beendet die Vorschau nicht (die Übersicht markiert ihn nur).
- rebel5_intro: Ebene der Bodenspur in der Kehle Wand/Rampe nicht gegen das Spiel geprüft.
- Unverändert aus rc604: Sprung-Animationsreste, JK2-Laufzeitzustand des Navigators, 15-s-Teleport, Geschütze,
  Laserfallen-Strahlen, Effekt-/Gras-Reste (siehe NOTIZEN_rc604.md).

## 5. Zusammenführung mit fix605r (t_rest, 50f0237)

`git merge` von Y:/t_rest fix605r in fix605 (a82a52b). Konflikte: `gui/selbsttest.cpp` (Summenzeile des Tür-Laufs:
beide Seiten hängten Zähler an [6]..; jetzt [6] vorher entfernt, [7] nach Kartenwechsel, [8] mit Feind, [9]
Landeanimationen ohne Sprung, [10] Luftanimationen ohne Sprung), `tools/handbuch/inhalt_szene.py` (Absatz "Kanten,
Kurven und andere NPCs": Landesatz von t_rest + parm-Sätze von fix605, alle 4 Sprachen), erzeugte Handbuchseiten
neu erzeugt (`gen_handbuch.py`, `--pruefen` ok), `gen_i18n.py` neu ausgeführt (1021 Texte). Bau ok, ctest 67/67.

Prüfung der Laufzeiten (Lauf mrg1, `tuerlauf` md_am_sith, taspir2, md_acc_sith, gegen ue605end):
- md_am_sith: alle Übersichtszeilen gleich (nur die Summenzeile hat die zwei neuen Zähler, beide 0).
- taspir2: alle Übersichtszeilen gleich (Landeanimationen ohne Sprung 21).
- md_acc_sith: 10 Zeilen um 50-300 ms / 1-2 Einheiten anders (rey_run1/2, kylo3, kylo4_walk, rey4_walk; rey4 in
  rey3defeated/cliff 3 -> 2 Wegstücke, letztes Stück 57500-57800 statt 57800-57850, beide abgeschnitten, nie
  angekommen wie vorher). Ursache: die Landung nach dem Spawn in der Luft (NPC_Begin setzt groundEntityNum =
  ENTITYNUM_NONE, NPC_spawn.cpp:2053 MD) läuft jetzt durch PM_CrashLand; delta = |vz| / 10 > 10 wählt eine
  Landeanimation, und PM_GetLandingAnim halbiert dabei velocity x/y ("stick landings some", bg_pmove.cpp:5398-5403
  MD, ohne g_allowBunnyhopping). Eine Landeanimation hält das Gehen NICHT an (sie steht nicht in PM_LockedAnim, kein
  Zweig in PM_WalkMove/NPC_MoveToGoal wartet auf legsAnimTimer); nur NPC_BSJump wartet in JS_LANDING auf
  legsAnimTimer (NPC_behavior.cpp:934-940 MD). Die Verschiebungen sind also Folge der Geschwindigkeitshalbierung, wie
  im Spiel.

## 6. trigger_hurt tötet Figuren (Punkt 1)

`hurtTodeBerechnen` (gui/app_view3d.cpp) läuft nach Wegen, Schwerkraft und Mover-Simulation über die fertige Szene im
50-ms-Takt: SP_trigger_hurt (dmg 0 -> 5, MD dmg < 0 -> 9999999 + Sturz, delay/wait * 1000, START_OFF, INACTIVE),
hurt_use (use schaltet ein/aus), target_(de)activate, G_TouchTriggersLerped (Weg vom letzten Ort in Schritten
maxs[1]/2, Kasten gegen das Brush-Modell = EntityContact/startsolid, nicht im noclip, nicht tot), hurt_touch (wait/
painDebounceTime, MULTIPLE, PLAYERONLY, NPC_targetname, delay-Rampe mit trigger_hurt_reset, Sturz: OpenJK Boba/
Raketentruppler fliegen los, MD vergleicht mit 59|66|67|81|60 = 127 - keine Klasse, JK2 keine Ausnahme) und G_Damage
soweit es zählt (FL_GODMODE, FL_UNDYING -> deathscript + health 1, health -= take). Lebenspunkte wie NPC_Begin je
Regelwerk (Karte `health`, sonst .npc `health` nach g_spskill, sonst 100), SET_HEALTH/SET_INVINCIBLE/SET_UNDYING aus
der Szene. Ein Tod geht als AblaufWelt::hurtTode in einen zweiten Nachbau (wie die Fallen-Uses): Leiche,
deathscript, ein `kill` im flachen Ablauf - die Szene zeigt den Tod wie bei kill (BOTH_DEATH1, Weg abgeschnitten).
Nicht nachgebildet: der Spieler, Rüstung (CheckArmor), PM_InForceFall (MD), Reihenfolge der Entities im Bild,
wiederholtes deathscript bei FL_UNDYING.

| Test | Rot (vorher) | Grün (nachher) |
|---|---|---|
| tuerlauf md_gd_jedi (ue605end / hurt1) | start_cin1/cin1_jedi ani1: 1 Wegstück 6500-126500 abgeschnitten, Umkehren 21, NPC-Absprünge 24, kein Tod | ani1 stirbt bei 7650 (cin1_jedi) bzw. 7700 (start_cin1) im trigger_hurt (Entity 2, dmg 10000), Szene zeigt den Tod zur selben Zeit, Nachbau 2 Skripte (md_gd/ani_death läuft), Weg 6500-7650, NPC-Absprünge 1; Summe pendelnd 2 -> 0 |
| ctest | – | 67/67 |

Changelog:
- EN: Figures that walk or fall into a `trigger_hurt` take its damage like in the game and die when their health runs out - they play their death and their `deathscript` runs (md_gd: Anakin falling into the pit).
- DE: Figuren, die in einen `trigger_hurt` laufen oder fallen, nehmen dessen Schaden wie im Spiel und sterben, wenn ihre Lebenspunkte aufgebraucht sind - sie spielen ihren Tod, und ihr `deathscript` läuft (md_gd: Anakin stürzt in die Grube).
- ZH: 走进或掉进 `trigger_hurt` 的角色会像游戏中一样受到伤害,生命值耗尽时死亡——播放死亡动画并运行其 `deathscript`(md_gd:Anakin 掉进深坑)。
- JA: `trigger_hurt` に歩いて入ったり落ちたりしたキャラクターはゲームと同じくダメージを受け、体力がなくなると死にます。死亡アニメーションを再生し、`deathscript` が実行されます(md_gd: 穴に落ちるアナキン)。

## 7. Figuren fahren auf bewegten Brushes mit (Punkt 2)

Der Wegplan merkt sich ps.groundEntityNum (LaufZustand::bodenEnt, aus der Bodenspur von PM_GroundTrace) und
nimmt die Figur in jedem Bild VOR ihrem Denkbild mit, wenn dieser Boden im Mover-Plan steht (navMoverTraegt =
G_MoverPush -> G_TryPushingEntity, g_mover.cpp:159-262 MD: trBase += move, Drehung um den neuen Ursprung mit
AngleVectors(-amove), delta_angles[YAW] += amove[YAW]); die Mover der Karte denken vor den später gespawnten NPCs.
Da der Plan ab dem Spawn (Vorlauf) rechnet, gilt das auch fürs Stehen bis zum Laufbefehl; nach einem Plan (Stand
auf einem Mover) holt der nächste Plan das Mitfahren bis zu seinem Anfang nach. bodenEnt/bodenAbMs stehen im
Planschlüssel und in gleicherZustand. Nicht nachgebildet: G_TestEntityPosition (ein blockierter Mitfahrer bleibt
im Spiel stehen bzw. hält den Mover an - der Mover-Plan steht fest).

| Test | Rot (vorher) | Grün (nachher) |
|---|---|---|
| tuerlauf vjun3 (pvj1 / mov1) elevator_up intro_troopera/f/c | bleiben unten (z 1240), Ziel +162/+154 höher, "ohne Weg (nie)", Umkehren 29/36/28 | fahren mit dem Aufzug (*59, +128) auf z 1367,7; Ziel +18/+10 höher; danach stecken sie in `lift_waypoint_hack` (func_usable *46 bei z 1340-1348), das im Spiel intro.ibi beim Kartenstart entfernt - der Reiter allein spielt intro nicht; Summe pendelnd 3 -> 0 |
| tuerlauf md_am_sith, taspir2, md_acc_sith (mrg1 / mov4) | – | alle Übersichtszeilen gleich |
| ctest | – | 67/67 |

Changelog:
- EN: Figures standing on a moving lift or platform ride along with it, also while they walk (vjun3: the troopers on the first lift).
- DE: Figuren auf einem fahrenden Aufzug oder einer Plattform fahren mit, auch während sie gehen (vjun3: die Soldaten auf dem ersten Aufzug).
- ZH: 站在移动的电梯或平台上的角色会随之移动,行走时也是如此(vjun3:第一部电梯上的士兵)。
- JA: 動くエレベーターや足場の上に立つキャラクターは、歩いている間も一緒に動きます(vjun3: 最初のエレベーターの兵士)。

## 8. Ein Kartenwechsel beendet die Vorschau (Punkt 3)

Entscheidung (kleinste treue Form): target_level_change_use -> G_ActivateBehavior(BSET_USE), G_ChangeMap ->
"maptransition" in den Befehlspuffer; der läuft nach dem Bild, die Karte ist fort. Der Nachbau (Nachbau::benutze)
merkt die Zeit des ersten Wechsels (Ablauf::kartenwechselMs, -karte = mapname), rechnet dieses Bild zu Ende und
hört dann auf (takte). Die Oberfläche kappt danach Zeitleiste (practicalEndMs, durationMs), Kamerafahrt und Szene
(szeneEndetBei: Schritte danach fallen weg, ein laufender Weg endet am Ort zu dieser Zeit) und meldet den Wechsel
in der Statusliste (Str::BewKartenwechsel). Ein ICARUS-Befehl "levelchange" gibt es nicht - Skripte wechseln die
Karte über `use` auf einen target_level_change (md_fh1_jedi transition: `use ( "levelchange" )`). SET_MISSIONFAILED
(Mission gescheitert) bleibt ohne Wirkung auf die Vorschau.

| Test | Rot (vorher) | Grün (nachher) |
|---|---|---|
| tuerlauf md_fh1_jedi (ue605end / lvl2, lvl3) transition.ibi | starkiller: 4 Wegstücke bis 129750, Umkehren 27, NPC-Absprünge 64 nach dem Wechsel bei 11750 | Nachbau bis 11750, Zeitleiste 11750/11750, Kamera 11650; starkiller 3 Wegstücke, letztes 9750-11750 abgeschnitten, Umkehren 0, Absprünge 0; Statuszeile "Level change at 11.75 s (target_level_change to md_fh2_jedi)" |
| ctest | – | 67/67 |

Changelog:
- EN: When a script uses a `target_level_change`, the preview ends there like the game: timeline, camera and figures stop at that moment, and the status list names the next map.
- DE: Benutzt ein Skript einen `target_level_change`, endet die Vorschau dort wie im Spiel: Zeitleiste, Kamera und Figuren hören in diesem Moment auf, und die Statusliste nennt die nächste Karte.
- ZH: 当脚本使用 `target_level_change` 时,预览会像游戏一样在此结束:时间轴、摄像机和角色在那一刻停止,状态列表会显示下一张地图。
- JA: スクリプトが `target_level_change` を使うと、プレビューはゲームと同じくそこで終わります。タイムライン、カメラ、キャラクターはその瞬間に止まり、ステータス一覧に次のマップが表示されます。

## 9. Laserfallen sprengen einander (Punkt 5)

fallenEffekteSetzen rechnet nach den natürlichen Explosionen (Figur im Strahl, laserfalleLauf) die Kette:
WP_Explode nimmt takedamage weg und ruft G_RadiusDamage(currentOrigin, 90, 256) (LT_SPLASH_DAM/LT_SPLASH_RAD,
weapons.h OpenJK/MD/codeJK2; weapons.dat von base und MD setzen nichts). Je andere Falle mit takedamage (ab dem
Spawn bzw. bei targetname + START_OFF ab dem ersten use, misc_trip_mine_activate) der Abstand zu absmin/absmax
(+-4 +1), points = 90 * (1 - dist/256), CanDamage (Punktspuren MASK_SOLID gegen Welt und Mover zur Zeit, Mitte und
+-15 in x/y), health 15 - (int)points; <= 0 -> WP_ExplosiveDie -> WP_Explode sofort (rekursiv, Reihenfolge der
Entities). laserfalleLauf nimmt die Zwangszeit (LaserfalleEingabe::zwangMs): Explosion, Strahl/Summen aus, frei
50 ms später; die Runden (bis 4) rechnen Strahlen, Freigaben und Kette, bis sie stehen. Figuren nehmen den
Explosionsschaden in der Vorschau weiterhin nicht (offen).

| Test | Rot (vorher) | Grün (nachher) |
|---|---|---|
| Selbsttest laserfalle (kette1 mit alter Erwartung / kette2) t3_byss #168 + #747 (44 auseinander) | #168 bei 600, #747 erst bei 1600 durch die Figur; t419 bei 1600 | #747 von #168 gesprengt bei 600 (G_RadiusDamage), t420 2x, t419 1x bei 600, *36 nach der ersten fort; 5 OK |
| rest605 BHED_REST=fallen | – | 1 OK (Strahl-Hindernisse unverändert) |
| ctest | – | 67/67 |

Changelog:
- EN: An exploding laser trip mine sets off the mines near it like the game's blast damage - mines hung close together go off together (t3_byss).
- DE: Eine explodierende Laserfalle sprengt die Fallen in ihrer Nähe mit wie der Explosionsschaden des Spiels - dicht beieinander hängende Fallen gehen zusammen hoch (t3_byss).
- ZH: 爆炸的激光绊雷会像游戏的爆炸伤害一样引爆附近的绊雷——挂得很近的绊雷会一起爆炸(t3_byss)。
- JA: 爆発したレーザートラップは、ゲームの爆発ダメージと同じく近くのマインを誘爆させます。近くに並べたマインは一緒に爆発します(t3_byss)。

## 10. Bildzeit duel_exegol mit 377 Nebelmodellen (Punkt 7)

Gemessen mit BHED_D3D_DEBUG=0 (Lauf perf2, rest605 BHED_REST=moversprite, neue Messschritte "Effektmodell-Leistung":
je 120 gezeichnete Bilder an derselben Stelle beim ersten Nebel-Runner): bei 12 s mit 377 Nebelmodellen im Flug
4,28 / 4,30 ms je Bild (max 7,5 / 8,8), bei 0 s ohne Nebelmodelle 4,20 ms (max 7,9). Flug über ganze Karten
(perf1, szenen -flug, Szenenzeit 0): md_am_sith 5,1 / 5,8 ms, duel_tower 4,6 ms, duel_exegol 4,2 ms. Die
Nebelmodelle kosten rund 0,1 ms je Bild - nicht langsamer als andere Karten, keine Optimierung nötig (das Ergebnis
bleibt damit ohnehin gleich).

## 11. Waffenhaltung des Oberkörpers (Punkt 6)

Ein NPC ohne Feind steht in WEAPON_IDLE (NPC_Think: READY -> IDLE ohne Feind, NPC.cpp:2239-2249 OpenJK /
2101-2111 MD / codeJK2 1860-1870). npcWaffenTorso (src/scene.cpp) portiert den Zweig WEAPON_IDLE von
PM_TorsoAnimation für NPCs ohne weaponBusy: OpenJK code/game/bg_panimate.cpp:6116-6416, Jedi Outcast
codeJK2/game/bg_panimate.cpp:3430-3694, Movie Duels bg_panimate.cpp:11340-13205 mit g_ActivateAnimationStyle 0 und
cg.renderingThirdPerson (cg_thirdPerson "1", cg_view.cpp:3080-3175 MD). Beine, denen der Oberkörper folgt
(GUARD/STAND*IDLE*, MD die lange Liste), PM_InSpecialJump, "full-body" beim Laufen/Springen (OpenJK/JK2 ohne die
schweren Waffen; Gehen nur der Spieler), dann je Waffe (z. B. OpenJK WP_BLASTER TORSO_WEAPONIDLE3, WP_BRYAR_PISTOL
TORSO_WEAPONIDLE2 im Stand; MD Gewehre TORSO_WEAPONREST3 bzw. mit BUTTON_WALKING TORSO_WEAPONIDLE3, Pistolen
TORSO_WEAPONREST2, Doppelpistolen IDLE2/IDLE2P, Thermal/Minen BOTH_STAND9, WP_MELEE BOTH_MD_CIN_1).
Actor::at setzt sie nur ohne Skriptanimation (Beine-Timer abgelaufen, kein SET_ANIM_UPPER), nicht beim Schießen;
BUTTON_WALKING aus SET_WALKING/SET_RUNNING (NPC_ApplyScriptFlags, NPC.cpp:800-813 MD), beim Gehen aus der Gehanimation.
Fehlt die Animation in der .cfg, folgt der Oberkörper wie im Spiel den Beinen.
Nicht portiert (vermerkt): der Zweig WEAPON_READY (nach SET_ENEMY, Kampf), weaponBusy (Schusszeiten, weaponTime nach
dem Waffenwechsel ~250 ms, Ducken), die Waffen-Beinanimationen von PM_Footsteps in Movie Duels (BOTH_WALK_BLASTER ...)
und dessen Torso-Tabelle (g_torsoMap, nur im READY-Zweig), die Blende beim Wechsel; der Takt der Schleife beginnt
beim letzten Waffenwechsel.

| Test | Rot (vorher) | Grün (nachher) |
|---|---|---|
| waffentest torsoProben (Szene: md_trooper WP_BLASTER steht) | MD ohne BUTTON_WALKING, MD mit SET_WALKING, OpenJK im Stand: FEHL (Oberkörper leer) | TORSO_WEAPONREST3 / TORSO_WEAPONIDLE3 / TORSO_WEAPONIDLE3; Skriptanimation oben bleibt, Schwert ohne Haltung, Funktionsproben je Waffe ok |
| tuerlauf vjun3 (tor1) | – | 4 OK, keine Meldung "Oberkoerperanimation steht nicht in der .cfg" |
| ctest | – | 67/67 |

Changelog:
- EN: NPCs with a gun hold it with their upper body like the game when no script animation plays there (rifle at the ready, pistol in front; under Movie Duels a lowered rifle, carried after SET_WALKING).
- DE: NPCs mit einer Schusswaffe halten sie wie im Spiel mit dem Oberkörper, solange dort keine Skriptanimation läuft (Gewehr im Anschlag, Pistole vor sich; unter Movie Duels ein gesenktes Gewehr, nach SET_WALKING getragen).
- ZH: 手持枪械的 NPC 在上半身没有脚本动画时,会像游戏中一样用上半身持枪(步枪戒备、手枪在身前;在 Movie Duels 下步枪放低,SET_WALKING 后端着)。
- JA: 銃を持つ NPC は、上半身でスクリプトのアニメーションが再生されていない間、ゲームと同じく上半身で銃を構えます(ライフルは構え、ピストルは前に。Movie Duels ではライフルを下げ、SET_WALKING のあとは抱えて持ちます)。

## 12. Jedi Outcast: Laufzeitstand des Navigators im gemeinsamen Takt (Punkt 4)

navKoerperLauf (der gemeinsame Takt, unter JK2 schon benutzt) führt jetzt für Jedi Outcast einen Laufzeitstand je
Figur (NavJk2Laufzeit über NavWelt::jk2Laufzeit, die rechnende Figur in NavWelt::ich):
- NPC_SetBlocked / NPC_ClearBlocked (codeJK2/game/g_navnew.cpp:56-73): blockingEntNum wird in NAVNEW_AvoidCollision
  (consecutiveBlockedMoves über der Grenze, 487-497) und nach einer gescheiterten Auflösung gesetzt, beim freien Weg
  in NAVNEW_MoveToGoal gelöscht (737).
- NAVNEW_Bypass (380-396): nach DanceWithBlocker und SidestepBlocker NAVNEW_PushBlocker (106-192): shoveCount > 30
  nicht mehr, nicht wenn ein anderer schon schiebt; Spur mit dem Kasten des Blockers (mins[2] + STEPSIZE), seiner
  clipmask | CONTENTS_BOTCLIP, ohne ihn, erst -right, dann +right, moveamt = (maxs[1] + seine maxs[1]) * 1.2;
  pushVec für 2000 ms, beide Seiten zu: pushVecTime = 0 (d_patched); shoveCount = 0 in NAVNEW_MoveToGoal (846).
- NAVNEW_CheckDoubleBlock (405-412): blockiert der Blocker mich auch, gehe ich weiter (443-446).
- BG_AddPushVecToUcmd (codeJK2/game/g_active.cpp:1502-1535, aus ClientThink 2653) im Bild der geschobenen Figur: der
  ucmd plus pushVec ergibt Richtung und ps.speed; ist pushVecTime vorbei, wird pushVec gelöscht. Eine geschobene
  Figur, die noch ihrem Plan ohne die anderen folgt, weicht davon ab (abweichen).
Offen (genau): AddFailedEdge zur Laufzeit (NAV_TestBestNode bei verschlossener Tür/Zerbrechlichem/entfernbarem
func_usable, g_nav.cpp:1015-1047; NAVNEW_MoveToGoal bei einem Körper zwischen zwei Wegpunkten, g_navnew.cpp:779-786)
mit SetEdgeCost(Q3_INFINITE)/FlagAllNodes(NF_RECALC) für alle NPCs - dafür müssten die A*-Ränge im gemeinsamen Takt
je Figur neu gerechnet und der Stand der gescheiterten Kanten (heute je Plan nachgerechnet, jk2Fehl) über die
Figuren geteilt werden; eine stehende Figur ohne Plan (nur ihr Ort) kann nicht geschoben werden (im Spiel denkt auch
sie in jedem Bild); der Spieler als Blocker (wird im Spiel nie geschoben) ist im gemeinsamen Takt nicht gekennzeichnet.

| Test | Rot (vorher) | Grün (nachher) |
|---|---|---|
| bewegungtest KJ1 (Gang mit Nische, B steht ohne Weg vor der Nische, A geht vorbei) | A kommt nie an (bleibt bei x 91 stehen), B y 0 | B wird in die Nische geschoben (y 47,9), A kommt bei 3700 ms an |
| Selbsttest jk2 (jk2a) | – | 15 OK / 0 FEHLER |
| ctest | – | 67/67 |

Changelog:
- EN: Jedi Outcast: an NPC that cannot get around another walking NPC tells it to step aside, and two NPCs blocking each other keep going, like the game.
- DE: Jedi Outcast: ein NPC, der an einem anderen gehenden NPC nicht vorbeikommt, schiebt ihn beiseite, und zwei NPCs, die sich gegenseitig blockieren, gehen weiter - wie im Spiel.
- ZH: Jedi Outcast:绕不过另一个正在行走的 NPC 时,NPC 会让对方让开;两个互相阻挡的 NPC 会继续前进——与游戏中一样。
- JA: Jedi Outcast:歩いているほかの NPC をよけられない NPC は相手を脇へ押しやり、互いにふさぎ合う 2 体はそのまま進みます(ゲームと同じ)。

## Noch offen nach diesem Durchgang

- trigger_hurt: Spieler, Rüstung (CheckArmor), PM_InForceFall (MD), Entity-Reihenfolge im Bild, wiederholtes
  deathscript bei FL_UNDYING; Explosionsschaden der Laserfallen an Figuren (G_RadiusDamage trifft auch NPCs).
- Mover-Mitfahren: G_TestEntityPosition (blockierter Mitfahrer, angehaltener Mover); vjun3 elevator_up allein steckt
  die Soldaten in lift_waypoint_hack (im Spiel entfernt intro.ibi es beim Kartenstart - der Reiter spielt intro nicht).
- Waffenhaltung: WEAPON_READY (Kampf), weaponBusy, MD-Waffen-Beinanimationen in PM_Footsteps (BOTH_WALK_BLASTER ...)
  und g_torsoMap, Blende beim Wechsel.
- JK2-Navigator: AddFailedEdge zur Laufzeit global für alle NPCs (NF_RECALC); stehende Figuren ohne Plan schieben;
  Spieler als Blocker kennzeichnen.
- Kampf-KI nach SET_ENEMY, SET_MISSIONFAILED, rebel5_intro-Bodenspur (unverändert aus der Liste oben).

Commits dieses Durchgangs: 57a48c2 Merge fix605r · 70b72e1 trigger_hurt · 3f5953a Mover-Mitfahren · f201aa9
Kartenwechsel · f1d03f3 Laserfallen-Kette · db23273 Bildzeit duel_exegol · 2bda191 Waffenhaltung · c7431c6 JK2-Laufzeitstand.

## 13. SET_MISSIONFAILED (Punkt 5 der letzten Liste)

Engine: Q3_SetMissionFailed (Q3_Interface.cpp:811-832 OpenJK, 1029-1053 MD, codeJK2 776-797): G_PlayerGuiltDeath
(STAT_HEALTH 0, Schwert aus, BOTH_FORCEHEAL_START OVERRIDE|HOLD, Timer -1; codeJK2 g_main.cpp:1172-1189), health 0,
statusTextIndex aus der missionFailedTable, cg.missionStatusShow (der Bildschirm), respawnTime + 2000. Die Skripte
hält ClientThink_real an: `pm_type == PM_DEAD && cg.missionStatusDeadTime < level.time` außerhalb einer Kamerafahrt
-> stop_icarus (g_active.cpp:4855-4865 OpenJK, 9706-9715 MD, codeJK2 2097-2107); pm_type wird im selben
ClientThink_real erst danach PM_DEAD (5205-5208), also beim zweiten usercmd - bei mehr als 40 Bildern/s des Clients
vor dem nächsten G_RunFrame. Danach ruft G_RunFrame keinen taskManager->Update mehr; Wege, Mover, NPC-Denken laufen
weiter. missionStatusDeadTime: 0 nach dem Kartenstart, SET_MISSIONSTATUSTIME = level.time + n.
Vorschau (src/ablauf.cpp): Nachbau merkt in_camera (camera ENABLE/DISABLE) und SET_MISSIONSTATUSTIME; nach dem Bild,
in dem die Bedingung gilt, hört takte auf (Ablauf::missionFailedMs/-Text/skripteStoppMs). Begonnene Wege werden
nicht abgeschnitten (die Welt läuft weiter). Der Spieler bekommt, wenn es ihn in der Vorschau gibt,
SET_ANIM_BOTH BOTH_FORCEHEAL_START + HOLDTIME -1. Statusliste: Str::BewMissionGescheitert.

| Test | Rot (vorher) | Grün (nachher) |
|---|---|---|
| ablauftest missionGescheitert (ohne Kamera / in der Kamerafahrt / Spieler) | 5 FEHLER: kein Stopp, camera ENABLE bei 1600 und der affect bei 2050 liefen | Stopp nach 550, nichts danach; in der Kamerafahrt bis zum DISABLE bei 1100; Spieler BOTH_FORCEHEAL_START gehalten |
| tuerlauf md_gd_jedi (hurt1 / mf2) | ani_death ohne Wirkung, Nachbau lief weiter | cin1_jedi: gescheitert 7650 (ani1 im trigger_hurt, deathscript ani_death), Skripte bis 8500 (camera DISABLE); start_cin1 7700 / 8550; Übersichtszeilen und Summe gleich; 4 OK |
| ctest | – | 67/67 |

Changelog:
- EN: `SET_MISSIONFAILED` works like the game: the player plays his guilt animation, and every script stops as soon as no camera is running (after a cutscene's `camera ( DISABLE )`); figures already walking finish their walk, and the status list names both times.
- DE: `SET_MISSIONFAILED` wirkt wie im Spiel: der Spieler spielt seine Schuld-Animation, und alle Skripte halten an, sobald keine Kamera läuft (nach dem `camera ( DISABLE )` einer Zwischensequenz); Figuren, die schon gehen, gehen ihren Weg zu Ende, und die Statusliste nennt beide Zeiten.
- ZH: `SET_MISSIONFAILED` 与游戏中一样生效:玩家播放愧疚动画,只要没有摄像机在运行(过场的 `camera ( DISABLE )` 之后),所有脚本就会停止;已在行走的角色会走完路程,状态列表会显示这两个时间。
- JA: `SET_MISSIONFAILED` はゲームと同じく働きます。プレイヤーは後悔のアニメーションを再生し、カメラが動いていなくなると(カットシーンの `camera ( DISABLE )` のあと)すべてのスクリプトが止まります。すでに歩いているキャラクターは道のりを歩き終え、ステータス一覧に両方の時刻が表示されます。

## 14. Jedi Outcast: AddFailedEdge zur Laufzeit, stehende Figuren schieben, Spieler nie (Punkt 1 der letzten Liste)

Schieben (src/navlauf.cpp, navKoerperLauf):
- NAVNEW_PushBlocker "never push the player" (codeJK2/game/g_navnew.cpp:113-116): NavFigurKoerper::spieler /
  NavKoerperKasten::spieler; der Ablauf setzt es fuer den echten Spieler.
- Eine stehende Figur denkt im Spiel jedes Bild (NPC_Think, ucmd ohne Bewegung) und BG_AddPushVecToUcmd
  (codeJK2/game/g_active.cpp:1502-1535, aus ClientThink 2653) bewegt sie mit pushVec. Im gemeinsamen Takt:
  (a) Plan zu Ende (Art Fertig: angekommen, in Ruhe) -> abweichen, ohne Ziel weiterrechnen, der Plan des
  Abschnitts geht weiter (Ablauf und Szene nehmen ihn ueber koerperPlaene); (b) ohne laufenden Abschnitt
  (noch keiner, oder geschnitten) -> ein eigener LaufRechner mit NavFigurKoerper::stehWerte (Ablauf::grundWerte,
  fuer Kartenfiguren ohne Wesen grundWerteName) ab ihrem Ort bis zum Stillstand -> NavKoerperErgebnis::schub
  (NavSchub). Der Ablauf (AblaufWelt::schubWege, schubNehmen) und die Szene (SzenenHilfe::schubWege:
  schubNehmen vor dem naechsten Plan, sonst ein WalkTo-Stueck am Ende) beginnen den naechsten Plan dort, wo
  der Schub die Figur hat; (c) in einer Luecke des Plans (Vorlauf bis zum Befehl) reagiert sie ebenso, und die
  Szene zeigt die Strecke im Vorlauf als WalkTo-Stueck (nur Jedi Outcast, nur Takte mit ucmd-Bewegung).
  Nach einem Schub gilt der Start des naechsten Abschnitts (schubDazwischen).
Gescheiterte Kanten (NavJk2FehlWelt, navlauf.h):
- AddFailedEdge (g_navigator.cpp:1886-2016): EdgeFailed ueber m_edgeLookupMap in Einfuegefolge (Eintraege
  bleiben nach ClearFailedEdge; Load traegt die 32 aus der .nav ein, NavNetzJk2::fehlLadeStart), freier Platz,
  checkTime + 1500 (Q_irand-Mitte), SetEdgeCost(Q3_INFINITE) (CNode::AddEdge: vorhandene Kante neu, sonst eine
  neue), FlagAllNodes(NF_RECALC); CalculatePath je Knoten erst in GetBestPathBetweenEnts (g_navigator.cpp:
  1374-1395, jk2RangNeu) - der Zweig mit gemerkten Wegpunkten (GetBestNodeAltRoute) sieht alte Raenge.
- Ausloeser: NAV_TestBestNode bei verschlossener Tuer, Zerbrechlichem, entfernbarem func_usable, Brush mit
  targetname und Monster-/Botclip (g_nav.cpp:1015-1047; NavNetzJk2::kanteSperrt; beim Brush die contents der
  getroffenen Flaeche statt blocker->contents) und NAVNEW_MoveToGoal (g_navnew.cpp:774-791: keine Nachbarn oder
  NAVNEW_TestNodeConnectionBlocked mit CONTENTS_BODY).
- CheckAllFailedEdges zu Beginn jedes Bilds (g_main.cpp:1274-1277; g_navigator.cpp:2018-2128): Kasten der Figur
  (entID lebt) bzw. DEFAULT_MINS/MAXS + MASK_NPCSOLID, offene Tuer und der Markierende zaehlen nicht,
  ClearFailedEdge = SetEdgeCost(-1) + NF_RECALC.
- Allein rechnet jeder Plan seine Welt ab dem Kartenstart (ohne Figuren; LaufPlan::jk2Dazu). Im gemeinsamen Takt
  gibt es EINE Welt (NavWelt::jk2FehlWelt) mit den Koerpern in den Spuren; folgt eine Figur ihrem Plan, traegt
  der gemeinsame Takt dessen AddFailedEdge nach (fehlNachtragen). Sobald zur Laufzeit eine Kante dazukam oder
  eine Kartenstart-Kante anders frei wird als ohne Figuren (ein Koerper in der Spur), rechnen alle Figuren
  selbst (fehlAnders). Auch eine einzelne Figur geht durch den gemeinsamen Takt, wenn ein Plan eine Kante
  markierte (sie gilt fuer ihre naechsten Plaene).

| Test | Rot (vorher) | Grün (nachher) |
|---|---|---|
| bewegungtest KJ2 (B ohne Plan im Gang) | FEHL: B 0 Schub-Wege, A kommt nie an | B in die Nische (y 47,9, Ruhe ab 2700), A bei 3700 |
| bewegungtest KJ3 (B ist der Spieler; rot = Schub ohne Spieler-Pruefung) | FEHL: der Spieler wird geschoben (y 47,9) | 0 Schub-Wege, A kommt nicht vorbei |
| bewegungtest KJ4 (B angekommen, Art Fertig) | FEHL: B y 0, A kommt nie an | B y 43,9, A bei 3850 |
| bewegungtest KJ5/KJ6/KJ7 (Ablauf + Szene: b im Skript, Kartenfigur, b geht nach dem Schub los) | – | geschoben in der Szene, dowait = Ankunft, kein Sprung zurueck (groesster Schritt 3,4) |
| jk2navtest 5a (Kiste zwischen B und C) | FEHL: bleibt bei x 213,8, 0 AddFailedEdge, nie angekommen | AddFailedEdge(B,C) bei 100, (E,C) bei 1100, Umweg ueber E, an bei 5200 |
| jk2navtest 6a/6b (Kante einer anderen Figur / aus dem eigenen ersten Plan) | (alter Navigator: keine Kante zur Laufzeit) | gemeinsam keine eigene (B,C), beide kommen an |
| bewegungtest gesamt, jk2navtest, ctest | – | 0 Fehlschlaege, 21 ok, 67/67 |
| Selbsttest jk2 (jk2d), tuerlauf md_am_sith (jkr1 / mrg1) | – | 15 OK; Uebersichtszeilen gleich |

Changelog:
- EN: Jedi Outcast: an NPC that is in another NPC's way is pushed aside even when it stands without a goal or has already arrived (never the player), and a connection blocked by a locked door, something breakable or another NPC counts as failed for all NPCs, who route around it until it is free again - like the game.
- DE: Jedi Outcast: ein NPC, der einem anderen im Weg steht, wird beiseitegeschoben, auch wenn er ohne Ziel steht oder schon angekommen ist (nie der Spieler), und eine Verbindung, die eine verschlossene Tür, etwas Zerbrechliches oder ein anderer NPC sperrt, gilt für alle NPCs als gescheitert - sie gehen außen herum, bis sie wieder frei ist, wie im Spiel.
- ZH: Jedi Outcast:挡住另一个 NPC 的 NPC 会被推到一旁,即使它没有目标站着不动或已经到达(绝不推开玩家);被上锁的门、可破坏物或其他 NPC 挡住的连接对所有 NPC 都算作失败,它们会绕行直到连接重新畅通——与游戏中一样。
- JA: Jedi Outcast:ほかの NPC の道をふさぐ NPC は、目的地なしで立っていても、すでに到着していても脇へ押しやられます(プレイヤーは押されません)。鍵のかかった扉や壊せる物、ほかの NPC にふさがれた接続はすべての NPC にとって失敗扱いになり、空くまで迂回します(ゲームと同じ)。
