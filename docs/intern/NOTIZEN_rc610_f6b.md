# Notizen rc610 / F6b: Menue-Editor wie im Spiel (B6b-1 bis B6b-5) und Mover-Reste aus F4b

Auftrag `auftrag_rc610_f6b.md`, Funde aus `pruefung_b4b6b_funde.md` (Bereich 6b) und dem Abschnitt Offen von `NOTIZEN_rc610_f4b.md`. Direkt in `Y:\karte` auf `karte1` ab f150f330 (F4b), Build `build-test`, Laeufe `laeufe/f6b_*` (rot `*_rot`, gruen `*_g1`, Schluss `f6be_*`), Laufskript `lauf.py` wie F4b. kFassung bleibt rc609. Kein Spiel gestartet: das Verhalten ist aus den Quellen belegt (OpenJK `scratchpad\ojk`, Movie Duels Update 9 `scratchpad\md9`, fuer Movie Duels MP `md9/codemp`).

Neue Pruefungen: ctest `menue_f6b` (neu, `tests/menuef6b.cpp`, B6b-1 bis B6b-5), Selbsttest `menueeditor` Teil M5b (B6b-5), ctest `mover` Teil `f6bMover` (6, 7, 8, 11, 12) und `ablauf` Teil `f6bKette` (6, 8, 9, 10, 11). Rotstand: f150f330 plus die Pruefungen und die neuen Felder ohne Wirkung (`Item::range`, `ITEM_TYPE_MODEL_ITEM`, `AblaufWelt::zufallStart`, `findeTeams`). Rot: `menue_f6b` 18 Fehler, M5b 47/1, `mover` 10 Fehler (darin die berichtigte alte Erwartung), `ablauf` 6 Fehler; gruen alle (M5b 48/0).

Neue Schnittstellen: `findeTeams(ents)` / `KartenTeams` (bhed/mover.h: G_FindTeams fuer MoverSim, den Nachbau und die Wegsuche - `navTeams` nimmt sie jetzt auch), `Item::range` (editFieldDef_t.range, cvarRotateScale schrieb vorher in maxVal), `ITEM_TYPE_MODEL_ITEM`, `AblaufWelt::zufallStart` (holdrand fuer Q_flrand).

## Teil A: Menue-Editor

- B6b-1: Movie Duels MP nimmt die Tabellen aus md9 `codemp/ui/ui_shared.c` (itemParseKeywords :9617 ff., menuParseKeywords): die von OpenJK MP plus cvarInt, cvarRotateScale und cvarSubString; exec und ownerdrawID gibt es dort nicht (unbekannt: weiterlesen, die erste `}` von exec beendet das Item). Item_ValidateTypeData (:7868) gibt ITEM_TYPE_MODEL_ITEM (17) ein modelDef. ItemParse_cvarInt (:9257) prueft seine drei PC_Int_Parse nicht (fehlt eine Zahl, bleibt 0, das Item geht weiter). Dazu, beim Lesen gefunden und mit nachgestellt: PC_Script_Parse von Movie Duels MP (:656-674) setzt "-" und eine folgende Zahl wieder zu "\"-204\"" zusammen. Rot: (a) type 17 + asset_model brach das Menue ab, (b) ownerdrawID brach ab, (c) exec las ein Skript (text "t" kam ins Item), (d) kein CVAR_SUBSTRING, (e) cvarInt mit zwei Zahlen brach ab, (f) `- "204"` getrennt; gruen alle, Gegenproben (OpenJK MP type 17 bricht ab, MD SP exec liest ein Skript, OpenJK MP trennt "-") unveraendert.
- B6b-2: Item_EnableShowViaCvar mit cvarSubString - im SP (OJ :6120, MD :6109) ist buff der ERSTE Wert der Liste und durchsucht werden die Woerter der cvar; Movie Duels MP (:2334) vergleicht zusaetzlich mit strstr(cvar, wert). Dazu die Puffergroessen (SP 1024, MP 2048). Rot: SP "push" in "pull push grip" verborgen, MD MP "pull pushgrip" verborgen, disableCvar mit Liste "b ; x" schaltete bei "x" ab; gruen alle.
- B6b-3: PC_Script_Parse mit char script[4096] (SP) bzw. [2048] (MP) und Q_strcat (shared/qcommon/q_string.c:199): passt ein Wort nicht ganz, kommt es GAR NICHT dazu (kein Kuerzen - der Fund nahm 4095 Zeichen an); kleinere Woerter danach koennen noch passen. Rot: SP 372 Woerter + "yyyyyyyy z" ergab 4105 Zeichen, MP kuerzte mitten im Wort auf 2047; gruen SP 4095 (`... " z "`), MP 2047 (186 Woerter und ein " ").
- B6b-4: ItemParse_cvar setzt min/max/def -1 nur fuer EDITFIELD, NUMERICFIELD, YESNO, BIND, SLIDER, TEXT (SP und MD SP auch TEXTSCROLL; Movie Duels auch SLIDER_INTEGER, SLIDER_ROTATE dazu range 720). Rot: SP-Slider blieb 0..1, SP TEXTSCROLL ohne -1, OpenJK MP setzte -1 fuer MULTI/LISTBOX/TEXTSCROLL, MD SP SLIDER_ROTATE: cvarRotateScale schrieb range nach maxVal; gruen alle.
- B6b-5: `paketPfad` laesst jede `.pk3` aus (FS_AddGameDirectory, OJ code/qcommon/files.cpp:2730, liest nur die pk3 des Spielordners, keine in einer pk3). Selbsttest M5b: nach dem Wiederoeffnen noch einmal ausgeben - rot: die alte zz_menu.pk3 lag im neuen Paket (47/1), gruen 48/0 (gleich viele Dateien wie beim ersten Export).

## Teil B: Mover und Kette

- 6: UnLockDoors (g_mover.cpp:924-938 OpenJK, :948-963 MD) loescht ohne TOGGLE den targetname des ganzen Teams; mit TOGGLE (24) bleibt er. Berichtigte alte Erwartung in ctest `mover` ("zweiter use oeffnet" -> "oeffnet nicht", Quelle im Test). Nach dem Entsperren oeffnet das Feld. Im Nachbau laeuft beim entsperrenden use kein usescript (G_ActivateBehavior steht nach dem LOCKED-Zweig). Rot: zweites use oeffnete; usescript lief bei 50 und 600 (soll nie bzw. nur 600). Gruen alle.
- 7: Die Tuer hat EIN think (Use_BinaryMover :1017-1021, Reached_BinaryMover :745-763, Use_BinaryMover_Go POS2 :841-852): ein use mit delay setzt Use_BinaryMover_Go neu (ein zweites verschiebt den Termin) und ersetzt in POS2 ReturnToPos1; Reached_BinaryMover ersetzt ein offenes Go. Dazu die Reihenfolge von G_RunMover: erst G_MoverTeam (blockiert/angekommen), dann G_RunThink. Messung (delay 1, Weg 56, D 140): uses 1000/1500 - rot y bei 2100 29,8 (offen ab 2000), gruen 0,0, offen ab 2550 (2700: 56). use 0 und 3100 - rot 3500 0,0 (zurueck bei 3190), gruen 56,0, zu erst ab 6100 (6500: 0,0).
- 8: G_FindTeams ueber alle Klassen (`findeTeams`): die erste Entity mit dem team-Namen ist Meister. Ist sie keine func_door, denkt keine Tuer des Teams (kein Feld) und keine heisst noch wie vorher - sie stehen auf pos1. G_Find/G_UseTargets finden im Nachbau und in MoverSim nur noch den Namen nach G_FindTeams; das usescript einer Tuer laeuft am Meister. Rot: func_static als Meister - use d oeffnete die Tuer; Nachbau lief das usescript des Slaves (m2 bei 50, m1 nie). Gruen: Tuer steht, m1 bei 50, m2 nie.
- 9: target_delay "random": nextthink = level.time + (wait + random * Q_flrand(-1, 1)) * 1000, Q_flrand wie q_math.c:231-245 (holdrand * 214013 + 2531011, >> 17, / 32768) ab `AblaufWelt::zufallStart` (Vorgabe 0x89abcdef, der Startwert der Bibliothek; im Spiel zieht jede Stelle daraus). Test wait 2 random 1: Q_flrand = -0,12787 -> 1872 ms -> Bild 1900. Rot 2000, gruen 1900.
- 10: target_change_parm (g_target.cpp:1131-1158 OpenJK, :1217-1245 MD): ohne activator nichts, sonst jeder nicht leere parm an den Benutzer, kein usescript. Rot: affect ( $get( STRING, "SET_PARM1" )$ ) traf nichts; gruen: marke_parm bei 150.
- 11: Movie Duels SP_func_rotating (g_mover.cpp:2441-2490 MD): "spinangles" ist trDelta (alle drei Achsen) und dreht ohne targetname auch ohne START_ON; mit health ist es ein func_breakable (MoverSim und Nachbau), das sich ohne targetname dreht und auf use bricht. OpenJK kennt beides nicht (Gegenprobe gruen). func_bobbing LOCKED: ohne START_OFF gilt Use_BinaryMover - das erste use entsperrt nur und loescht den Namen. Rot: spinangles stand, health-Rotor drehte und brach nicht (Nachbau: kein target), LOCKED-func_bobbing sprang beim ersten use auf (0,0,0); gruen alle. GOODIE (32): nicht geaendert, siehe Offen.
- 12: func_bobbing ohne START_OFF ist in MoverSim ein Team wie eine Tuer mit pos1 = pos2 = (0,0,0) und trDuration = speed * 1000: target beim Go, opentarget beim Ankommen, wait (ms, ohne mal 1000; 0 = ReturnToPos1 im selben Bild), closetarget in POS1, target2 in POS2, delay und LOCKED. Gezeichnet wie in F4b (ab dem ersten Go auf (0,0,0)). Rot (speed 1, uses 1000/4000): w1 nur einmal, opentarget/closetarget nie; gruen: w1 bei 1000 und 4000, w2 bei 2050, w3 bei 3050.

## Zusatzpunkt 13 (Meldung 05.11. abends): Esc bei offenem Surface Inspector

- Gemeldet: Flaechen gewaehlt, S oeffnet den Surface Inspector, Esc hebt die Flaechenauswahl nicht auf. Ursache: der Nachbau von g_pressedKeysFilter (surfacedialog.cpp:594-631 NRC) nahm Return, Enter, Esc, Tab und ohne Zusatztasten Pfeil hoch/runter, Bild hoch/runter den Kuerzeln weg, sobald das Fenster den Fokus hatte - nach S immer. NRC tut dasselbe: der Filter sitzt am ganzen Dialog, und beim Zeigen loescht NRC das Fokus-Widget ("to keep global shortcuts working"), so dass genau diese Tasten am Dialog ankommen und verschluckt werden. Als NRC-Fehler nicht uebernommen: die Tasten gehen jetzt nur dann nicht an die Kuerzel, wenn ein Feld oder Knopf des Inspectors aktiv ist (beim Tippen sperrt ohnehin io.WantTextInput; Esc bricht dort nur die Eingabe ab). Gleiche Ursache, mit behoben: Tab (CameraFocusOnSelected), Return (ClipperClip), Pfeil hoch/runter (CameraForward/CameraBack), Bild hoch/runter (UpFloor/DownFloor). Der Entity Inspector filtert nur, solange seine Liste/Zeile den Fokus hat (unveraendert).
- Selbsttest `karteneditor` Stufe 2 (Texturen): Flaechen per SelectTextured, S, Tab, Esc. Rot (`f6b_si_rot`, BHED_KARTE_NUR=stufe2): 374/2 - Tab bewegte die Kamera nicht, die 6 Flaechen blieben gewaehlt. Erster Gruenlauf (`f6be_stufe2`) 375/1: Tab ging jetzt an CameraFocusOnSelected, machte aber dazu (ImGui-Tabbing) das Texturfeld aktiv, und Esc brach nur dessen Eingabe ab. Da g_pressedKeysFilter Tab auch den Feldern nimmt (im NRC-Inspector wandert der Fokus nie mit Tab), tragen die Felder jetzt ImGuiItemFlags_NoTabStop. Gruen (`f6be_stufe2b`) 376/0.

## Schluss

- Sauberer Neubau (`--clean-first`) 0 Warnungen, 0 Fehler - dreimal: nach Teil A/B (`f6be_neubau`), nach Punkt 13 (`f6be_neubau2`), nach NoTabStop (`f6be_neubau3`, das gepruefte Programm).
- ctest -j 16 bzw. 12: 219 von 219 (nach `f6be_neubau` und noch einmal nach `f6be_neubau3`; darin `menue_f6b`, `mover`, `ablauf`, die nav-Tests mit `navTeams` auf `findeTeams`).
- Selbsttests parallel (`laeufe/f6be_*`, 7 gleichzeitig): `menueeditor` Movie Duels 2781/0, JKA (`nur_ja.cfg`) 2781/0, `pruefung3` 9/0, `mission1` 98/0, `missionsbau` 95/0, `karteneditor` Stufe 2 375/1 (Punkt 13, siehe oben), Modus 1 2648/3 - die drei im Kuerzelfenster der Ereignisse ("Delete: die Zeile wartet auf eine Taste" und zwei Folgefehler), ausserhalb des Geaenderten, unter Volllast. Nach `f6be_neubau3` zu zweit wiederholt: `karteneditor` Stufe 2 376/0, Modus 1 (`f6be_modus1b`) 2651/0 - die drei Fehler waren ein Ausreisser unter Last.
- Lints: sources, fassung, i18n, i18n_erzeugt, headers, feldinit, gapp, settings, d3dbesitz, mapstate, imgui_context, rueckkopplung ok; portability 98 (wie F4b, keine Beanstandung in den geaenderten Dateien).
- Handbuch nicht geaendert: es beschreibt weder die Woerter von Movie Duels MP noch den Inhalt der Export-pk3, die Mover oder das Verschlucken von Tasten im Surface Inspector; die Texte stimmen jetzt genauer. Keine neuen Programmtexte.

## Offen

- Gegen das Spiel gemessen wurde nichts (keine Spielstarts); belegt aus den Quellen.
- B6b-1: Movie Duels MP zeichnet ITEM_TYPE_MODEL_ITEM mit Item_Model_Paint_Item (:6145) - die Vorschau liest es jetzt, zeichnet es aber wie einen unbekannten Typ. Die uebrigen MD-MP-Funktionen (Malen, Tasten, Skriptbefehle) sind weiter nicht Zeile fuer Zeile gegen md9/codemp gelesen.
- MOVER_GOODIE (32) an Tueren und func_bobbing: ob es aufgeht, haengt am Inventar des Benutzers (INV_GoodieKeyCheck, g_inventory.cpp:60) - das hat die Vorschau nicht; nicht nachstellbar, nicht geaendert.
- func_bobbing ohne START_OFF: eine Umkehr mitten in der Bahn rechnet die Engine mit 0/0 (NaN) und einem undefinierten int - nicht nachstellbar; die Vorschau faehrt dann die volle Bahn zurueck. Ein func_bobbing in einem Team (G_FindTeams) bekommt kein eigenes Team.
- Neu gesehen, nicht geaendert (nicht in den Punkten): Eine Entity mit usescript, aber ohne targetname und script_targetname bekommt im Spiel eine ICARUS-Kennung (ValidEntity/InitEntity, Q3_Interface.cpp:7517-7590 OpenJK) und fuehrt ihr usescript aus - im Nachbau nicht (`Wesen::gibt` nur mit ICARUS-Namen). `use ( $get( STRING, "SET_PARM1" )$ )` setzt der Nachbau nicht ein (der Test fuer Punkt 10 nimmt deshalb affect). Slaves anderer Klassen (func_static, func_wall) einer Tuer bewegen sich mit MatchTeam nicht mit (bei ihnen ist pos1 = pos2, ohne sichtbaren Unterschied).

## Changelog

### English
- Menu editor: Movie Duels multiplayer menus are read with the keywords of the Movie Duels multiplayer code - cvarInt, cvarRotateScale, cvarSubString and the model item type load, exec and ownerdrawID are skipped like in the game, and negative numbers in scripts stay numbers.
- Menu editor: cvarSubString now works in the preview for show, hide, enable and disable; long scripts end where the game's buffer ends; cvar resets the slider range only where the game does.
- Menu editor: exporting a project no longer packs the previous pk3 from the project folder into the new one.
- Map preview: an unlocked door can no longer be used by its name unless it is a toggle door; a second use of a door with delay moves the start, as in the game.
- Map preview: door teams follow the first entity of the team of any class; only the team master answers to the name and runs its use script.
- Script preview: target_delay uses its "random" value, target_change_parm passes its parms to whoever used it, and Movie Duels func_rotating with spinangles or health behaves like in Movie Duels.
- Map preview: a func_bobbing without START_OFF runs the full mover path after its first use - target, opentarget, closetarget, wait, delay and LOCKED.
- Map editor: with the Surface Inspector open, Esc deselects again and Tab, Return, the arrow keys and Page Up/Down reach their shortcuts, as long as no field of the inspector is being edited.

### Deutsch
- Menü-Editor: Mehrspieler-Menüs von Movie Duels werden mit den Schlüsselwörtern des Movie-Duels-Mehrspielercodes gelesen - cvarInt, cvarRotateScale, cvarSubString und der Modell-Elementtyp laden, exec und ownerdrawID werden wie im Spiel übersprungen, und negative Zahlen in Skripten bleiben Zahlen.
- Menü-Editor: cvarSubString wirkt jetzt in der Vorschau für Zeigen, Verbergen, Ein- und Ausschalten; lange Skripte enden, wo der Puffer des Spiels endet; cvar setzt den Bereich eines Schiebers nur dort zurück, wo das Spiel es tut.
- Menü-Editor: Beim Export eines Projekts kommt die vorige pk3 aus dem Projektordner nicht mehr in die neue.
- Kartenvorschau: Eine entsperrte Tür lässt sich nicht mehr über ihren Namen benutzen, außer sie ist eine Umschalttür; ein zweites use einer Tür mit delay verschiebt den Start, wie im Spiel.
- Kartenvorschau: Türteams richten sich nach der ersten Entity des Teams, gleich welcher Klasse; nur der Teammeister hört auf den Namen und führt sein usescript aus.
- Skriptvorschau: target_delay nimmt seinen "random"-Wert, target_change_parm gibt seine parms an den, der es benutzt, und func_rotating mit spinangles oder health verhält sich in Movie Duels wie dort.
- Kartenvorschau: Ein func_bobbing ohne START_OFF fährt nach dem ersten use die ganze Mover-Bahn - target, opentarget, closetarget, wait, delay und LOCKED.
- Karten-Editor: Bei offenem Surface Inspector hebt Esc die Auswahl wieder auf, und Tab, Return, die Pfeiltasten und Bild hoch/runter erreichen ihre Kürzel, solange kein Feld des Inspectors bearbeitet wird.

### 中文
- 菜单编辑器:Movie Duels 多人菜单按 Movie Duels 多人代码的关键字读取——cvarInt、cvarRotateScale、cvarSubString 和模型元素类型可以加载,exec 和 ownerdrawID 像游戏中一样被跳过,脚本中的负数仍是数字。
- 菜单编辑器:cvarSubString 现在在预览中对显示、隐藏、启用和禁用生效;长脚本在游戏缓冲区结束处截止;cvar 只在游戏会重置的地方重置滑块范围。
- 菜单编辑器:导出项目时不再把项目文件夹中之前的 pk3 打包进新的 pk3。
- 地图预览:解锁后的门不再能通过名称使用,除非它是切换门;带 delay 的门被第二次 use 时会推迟开始,与游戏一致。
- 地图预览:门组以组内第一个实体为准,不论其类别;只有组长响应名称并运行其 usescript。
- 脚本预览:target_delay 使用其 "random" 值,target_change_parm 把其 parms 交给使用者,带 spinangles 或 health 的 func_rotating 在 Movie Duels 中的行为与该游戏一致。
- 地图预览:没有 START_OFF 的 func_bobbing 在第一次 use 后执行完整的移动路径——target、opentarget、closetarget、wait、delay 和 LOCKED。
- 地图编辑器:Surface Inspector 打开时,只要没有在编辑其中的字段,Esc 会再次取消选择,Tab、回车、方向键和 Page Up/Down 也会触发各自的快捷键。

### 日本語
- メニューエディター: Movie Duels のマルチプレイヤーメニューを Movie Duels マルチプレイヤーコードのキーワードで読み込みます。cvarInt、cvarRotateScale、cvarSubString とモデル項目タイプが読み込まれ、exec と ownerdrawID はゲームと同じく読み飛ばされ、スクリプト内の負の数は数のままです。
- メニューエディター: cvarSubString がプレビューの表示・非表示・有効・無効で働くようになりました。長いスクリプトはゲームのバッファーが終わるところで終わり、cvar はゲームと同じ場合にだけスライダーの範囲をリセットします。
- メニューエディター: プロジェクトを書き出すとき、プロジェクトフォルダーにある前の pk3 を新しい pk3 に入れなくなりました。
- マッププレビュー: ロック解除したドアは、トグルドアでなければ名前で使えなくなります。delay 付きドアへの 2 回目の use は開始を遅らせます(ゲームと同じ)。
- マッププレビュー: ドアのチームはクラスに関係なくチームの最初のエンティティに従います。名前に応えて usescript を実行するのはチームマスターだけです。
- スクリプトプレビュー: target_delay は "random" の値を使い、target_change_parm は使った相手に parms を渡し、spinangles や health 付きの func_rotating は Movie Duels と同じように動きます。
- マッププレビュー: START_OFF のない func_bobbing は最初の use の後、target、opentarget、closetarget、wait、delay、LOCKED を含むムーバーの経路をすべて実行します。
- マップエディター: Surface Inspector を開いていても、そのフィールドを編集していなければ、Esc で再び選択が解除され、Tab、Return、矢印キー、Page Up/Down もそれぞれのショートカットに届きます。

### Español
- Editor de menús: los menús multijugador de Movie Duels se leen con las palabras clave del código multijugador de Movie Duels: cvarInt, cvarRotateScale, cvarSubString y el tipo de elemento de modelo se cargan, exec y ownerdrawID se omiten como en el juego, y los números negativos de los scripts siguen siendo números.
- Editor de menús: cvarSubString funciona ahora en la vista previa para mostrar, ocultar, activar y desactivar; los scripts largos terminan donde termina el búfer del juego; cvar restablece el rango de un deslizador solo donde lo hace el juego.
- Editor de menús: al exportar un proyecto ya no se mete la pk3 anterior de la carpeta del proyecto en la nueva.
- Vista previa del mapa: una puerta desbloqueada ya no se puede usar por su nombre, salvo que sea una puerta de alternancia; un segundo use de una puerta con delay retrasa el inicio, como en el juego.
- Vista previa del mapa: los equipos de puertas siguen a la primera entidad del equipo, de cualquier clase; solo el maestro del equipo responde al nombre y ejecuta su usescript.
- Vista previa de scripts: target_delay usa su valor "random", target_change_parm pasa sus parms a quien lo usa, y func_rotating con spinangles o health se comporta en Movie Duels como allí.
- Vista previa del mapa: un func_bobbing sin START_OFF recorre tras su primer use toda la trayectoria de mover: target, opentarget, closetarget, wait, delay y LOCKED.
- Editor de mapas: con el Surface Inspector abierto, Esc vuelve a deseleccionar y Tab, Intro, las flechas y Re Pág/Av Pág llegan a sus atajos, mientras no se esté editando un campo del inspector.

### Čeština
- Editor menu: menu Movie Duels pro více hráčů se čtou s klíčovými slovy kódu Movie Duels pro více hráčů - cvarInt, cvarRotateScale, cvarSubString a typ prvku modelu se načtou, exec a ownerdrawID se přeskočí jako ve hře a záporná čísla ve skriptech zůstanou čísly.
- Editor menu: cvarSubString nyní v náhledu funguje pro zobrazení, skrytí, zapnutí a vypnutí; dlouhé skripty končí tam, kde končí vyrovnávací paměť hry; cvar vrací rozsah posuvníku jen tam, kde to dělá hra.
- Editor menu: export projektu už do nové pk3 nebalí předchozí pk3 ze složky projektu.
- Náhled mapy: odemčené dveře už nelze použít podle jména, pokud to nejsou přepínací dveře; druhé use dveří s delay posune začátek, jako ve hře.
- Náhled mapy: týmy dveří se řídí první entitou týmu libovolné třídy; na jméno reaguje a svůj usescript spouští jen vedoucí týmu.
- Náhled skriptu: target_delay používá svou hodnotu "random", target_change_parm předá své parms tomu, kdo ho použil, a func_rotating se spinangles nebo health se v Movie Duels chová jako tam.
- Náhled mapy: func_bobbing bez START_OFF po prvním use projede celou dráhu moveru - target, opentarget, closetarget, wait, delay a LOCKED.
- Editor map: při otevřeném Surface Inspectoru Esc opět zruší výběr a Tab, Enter, šipky a Page Up/Down dojdou ke svým zkratkám, pokud se neupravuje žádné pole inspektoru.
