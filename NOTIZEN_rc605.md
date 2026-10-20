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
