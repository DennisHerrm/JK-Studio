# Notizen rc610 / F46: Funde der Codepruefung Bereich 4 und 6

Auftrag `auftrag_rc610_f46.md`, Funde aus `pruefung_b4b6_funde.md`. Direkt in `Y:\karte` auf `karte1` ab 38693009 (F23), Build `build-test`, Laeufe `laeufe/f46_*` (rot `*_rot`, gruen `*_g1`, Schluss `f46e_*`). Laufskript `lauf.py` wie F23. kFassung bleibt rc609. Kein Spiel gestartet; das Spielverhalten ist aus den Quellen belegt (OpenJK `scratchpad\ojk`, Movie Duels Update 9 `scratchpad\md9`).

Neue Pruefungen: ctest `menue_f46` (B6-2, B6-3, B6-4), `pruefung4b` Teil `geschuetzF46` (B4-1, B4-2), Selbsttest `menueeditor` Teil `seitenwechsel` (B6-1, auch im vollen Lauf), Teil `speichern` Abschnitt 6 (B6-5), `wegerzeugung` "keine Kante faellt ohne freies Feld weg" (B4-3). Die Rotlaeufe liefen auf 38693009 plus den Pruefungen (Testhilfen: Felder `SchussZiel::hatPilot/pilotTeam/pilotNotarget/reiterVerborgen`, `Umgebung::ownerDrawBreite`, Zaehler `wegKantenOhneFeld`). Zwei Rotfaelle, die erst nach dem ersten Rotlauf dazukamen (B6-1 a nach einer Korrektur des Tests, BLINK_DIVISOR der ownerdraws), liefen rot im Programm mit den Behebungen, die eine Testvariable `TEMPROT` abschaltete; diese ist wieder entfernt.

## B6-1: Seitenwechsel aus dem Code

- Ursache: Skript im Bedienen-Modus (`open`), Klick in der Hierarchie (ganze Datei), Menueknoten der Gliederung, Diagnose ohne Item und das Wechseln auf ein Menue einer anderen Datei setzten `d.seite`, ohne die Auswahl zu leeren (nur der Reiter tat es).
- Behoben: `zeigeSeite`/`zeigeMenue` - jede andere Seite leert die Auswahl wie der Reiter.
- Rot (`f46_me_rot2`): nach `open klein` Seite 1, Auswahl 10 (Menue mit 3 Elementen); Strg-Klick auf k1 in der Hierarchie: Auswahl "2,1", Entf liess nur k0 stehen. Gruen (`f46_me_g1`): Auswahl leer bzw. nur 1, danach k0,k2, gross behaelt 12. Der erste Rotlauf (`f46_me_rot`) klickte im selben Bild, in dem Bedienen anging (die Laufzeit startete danach neu, die Seite wechselte nicht); der Test wartet jetzt ein Bild.

## B6-2: Item_TextColor im SP

- OpenJK SP (`code/ui/ui_shared.cpp:6276-6327`) und Movie Duels SP: kein BLINK-Zweig, kein Fokus fuer autowrapped Text; MP blinkt.
- Dazu gefunden und behoben: der Puls `sin((float)(realTime / PULSE_DIVISOR))` teilt ganzzahlig (OpenJK SP und MP, Movie Duels MP, auch `ui_main.c:2857`), Movie Duels SP in float; BLINK_DIVISOR ist im SP 600 (`code/qcommon/q_shared.h:272`), im MP 200 - die ownerdraws blinken im SP mit 600.
- Rot (ctest, vorher): SP-Glyphe 0,9 * forecolor, autowrapped mit Fokus in focusColor, Puls bei 100 ms 0,8028 statt 0,8159 (OpenJK) - gruen: forecolor, forecolor, 0,8159 / Movie Duels SP 0,8028. ownerdraw SP bei 300 ms: rot 1,0, gruen 0,9757.

## B6-3: Item_SetTextExtents

- Zentriertes Eingabefeld mit cvar: rot Text ab 76 (SP und MP), gruen 41 (100 - (50 + 70) / 2 + 1). ownerdraw rechts mit ownerDrawWidth 70: rot 81, gruen 11.
- `ownerDrawBreite` = UI_OwnerDrawWidth: MP nach `codemp/ui/ui_main.c:2578-2797` (OpenJK) bzw. `:3643` ff. (Movie Duels: MD_MP_INGAME, Netzfilter "Movie Duels"); teamCount und aliasCount bleiben im Spiel 0, Teamnamen also ohne Text, Bots als "%i. (null)". SP: 0 (nur UI_KEYBINDSTATUS mit wartender Taste). Die Movie-Duels-Kraefte 340-350 (UI_FORCE_RANK_INDEX) kennt die Vorschau nicht (weder zum Zeichnen noch zur Breite).
- Dazu: Movie Duels SP teilt `originalWidth / static_cast<float>(2)`: rot gleiche Stelle wie OpenJK, gruen eine Spalte frueher bei ungerader Breite.
- Jedi Outcast MP: dieselbe Rechnung wie JKA MP (keine JK2-Quelle vorhanden, nicht belegt).

## B6-4: Menu_HandleMouseMove

- Port von Menu_HandleMouseMove, Item_MouseEnter/Leave, Item_SetFocus, Menu_ClearFocus (SP `:9333`, MP `codemp/ui/ui_shared.c:6640`; Movie Duels gleich, verglichen): mouseEnter fuer jedes Element unter dem Zeiger (auch Dekorationen), Fokus dem ersten, das ihn nimmt; SP ueberspringt WINDOW_INACTIVE; der Fokus bleibt nach dem Verlassen (Item_MouseLeave nimmt ihn nicht); MP fuehrt leaveFocus fuer jedes Element aus; mouseEnterText/mouseExitText im textRect (MP mit r.y -= r.h); Rect_ContainsPoint echt innerhalb.
- Ein Klick trifft das Element mit dem Fokus (Menu_HandleKey), nicht mehr das oberste unter dem Zeiger. Die Vorschau ruft die Mausbewegung nur, wenn sich der ganzzahlige Zeiger des 640x480-Schirms bewegt (UI_MouseEvent), und nach dem Oeffnen (Menus_OpenByName). Die Tests `menue_skript` und `menue_bauteile` bewegen deshalb die Maus vor dem Klick.
- Rot (ctest): 16 Fehler (nur b bekam mouseEnter und Fokus, Dekoration nie, inaktiver Knopf im SP mit mouseEnter). Gruen: alle.
- Nicht nachgebaut: `item->disabled`, Listbox-/Textscroll-MouseEnter (Pfeil-Flags), itemCapture.

## B6-5: imSpielordner ohne Trennzeichen

- Rot (`f46_me_rot`, Teil speichern): `GameData_x/ui/me2_nachbar.menu`, Strg+S fragte nach dem Ort. Gruen (`f46_me_g1`): an den Ort geschrieben, ohne Dialog.

## B4-1: Turbolaser-Klaenge (Movie Duels)

- `g_turret.cpp` MD 584 (shutdown), 634 (ping), 752 (startup, attackDebounceTime immer) nur ohne TURBO. OpenJK unveraendert (shutdown immer, der Turbo sucht keine Feinde).
- Rot: MD Turbo "startup,shutdown,ping". Gruen: keiner, feuert weiter; MD und OpenJK ohne Turbo alle drei, OpenJK-Turbo still.

## B4-2: Fahrzeuge als Ziel (Movie Duels, mp_rules)

- Nachgestellt (nicht verworfen): ein Fahrzeug mit Pilot ist ein Ziel mit dem Team des Piloten (`turret_target_team`, :201-208), ohne Pilot oder mit FL_NOTARGET des Piloten nicht; ein Reiter in einem Fahrzeug mit hideRider nicht (:681-704). Die Vorschau fuellt Pilot, Team und FL_NOTARGET aus den Ritten (`rittBei`).
- Rot: Fahrzeug mit feindlichem Pilot ohne Schuss, Reiter im hideRider-Fahrzeug beschossen. Gruen: umgekehrt; OpenJK nimmt den Reiter weiter.
- Nicht im Spiel geprueft (Auftrag: keine Spiele).

## B4-3: Wegerzeugung mit vorhandenen Wegpunkten

- Nachgestellt auf taspir1 (Karte mit eigenen Wegpunkten): rot 2 Kanten ohne freies Feld (914 Kanten). Behoben: faellt eine Kante weg, rechnet `verteileZiele` noch einmal mit den wahren freien Feldern der vorhandenen Wegpunkte und den schon verbundenen Paaren (die Waldkante traegt dann das andere Ende oder ein freigemachtes Feld; gleiche freie Felder: dieselbe Wahl wie vorher). Gruen: 0 ohne Feld, 941 Kanten.
- Ohne die Regel byte-gleich (`BHED_WE_ENT`, SHA-1): jeditemple_garden 8c603101... rot = gruen, t1_danger 0f9c13e7... rot = gruen (beide 0 Kanten ohne Feld).
- taspir1: "jeder Innenraum erreichbar" ist rot und gruen gleich falsch (7 Innenraeume ohne Weg vom Spielerstart, die Wegfindung ueber die Kanten -1) - nicht durch B4-3; offen.

## Schluss

- Sauberer Neubau (`--clean-first`): 0 Warnungen, 0 Fehler. ctest 216 von 216 (neu `menue_f46`).
- Parallel (`laeufe/f46e_*`): `menueeditor` Movie Duels 2779/1, JKA (`nur_ja.cfg`) 2780/0; der eine Fehler (M5b Projekt: Wiederherstellen, nur im Movie-Duels-Lauf unter Volllast) einzeln wiederholt (`f46e_me_m5b`, `BHED_MENUE_NUR=m5b`) 47/0. `geschuetze` 12/0, `wegerzeugung` jeditemple_garden mit `BHED_WE_SOLID="-320 1920 328"` 10/0, Modus 1 (`f46e_m1`) 2651/0.
- Lints: lint_sources, lint_d3dbesitz, lint_mapstate, lint_gapp, lint_feldinit, lint_i18n, lint_settings, lint_imgui_context, lint_rueckkopplung, lint_fassung ok; lint_portability 102 wie vorher (keine in geaenderten Dateien). Handbuch `--pruefen` ok (es/cs 0 englisch).
- Handbuch (6 Sprachen): Menue-Editor, Bedienen - was die Mausbewegung und ein Klick tun. Keine neuen Bilder.

## Offen

- taspir1: Innenraeume ohne Weg vom Spielerstart im Selbsttest wegerzeugung (vor und nach F46 gleich).
- Menue-Vorschau: `disabled`, Listbox-/Textscroll-MouseEnter und die Movie-Duels-Kraefte 340-350 bei UI_OwnerDrawWidth nicht nachgebaut; Jedi Outcast MP ohne eigene Quelle wie JKA MP.
- B4-2 nicht im Spiel gegengeprueft.

## Changelog

### English
- Menu editor, Use mode: moving the pointer runs mouseEnter for every item under it, decorations too, and gives the focus to the first item that takes it (in single player not to an inactive one), like the game; the focus stays until another item takes it, and a click acts on the item with the focus.
- Menu editor: centred edit fields and centred or right-aligned ownerdraws place their text like the game (the cvar value and the ownerdraw width count); Movie Duels single player centres with half pixels.
- Menu editor, single player: textstyle 1 (blink) no longer blinks, wrapped text keeps its colour with the focus, ownerdraws blink at the single-player rate; the focus pulse steps like the game.
- Menu editor: switching the page from a script, the hierarchy, the outline or the problems list clears the selection like the page tab (it could crash or delete an item of another menu).
- Menu editor: a folder next to GameData whose name starts with "GameData" is no longer treated as a game folder when saving.
- Mission preview, Movie Duels: the turbo laser (misc_turretG2) plays no startup, shutdown or ping sounds; a vehicle with a pilot is a turret target with the pilot's team, a rider hidden in a closed vehicle is not.
- Waypoint generation: an edge to an existing waypoint whose target fields are full no longer drops silently; the network stays connected.

### Deutsch
- Menü-Editor, Bedienen: Bewegen des Zeigers führt mouseEnter für jedes Element darunter aus, auch für Dekorationen, und gibt den Fokus dem ersten Element, das ihn nimmt (im Einzelspieler keinem inaktiven), wie im Spiel; der Fokus bleibt, bis ein anderes Element ihn nimmt, und ein Klick wirkt auf das Element mit dem Fokus.
- Menü-Editor: Zentrierte Eingabefelder und zentrierte oder rechtsbündige Ownerdraws setzen ihren Text wie das Spiel (der Wert der cvar und die Breite des Ownerdraws zählen mit); Movie Duels Einzelspieler zentriert auf halbe Bildpunkte.
- Menü-Editor, Einzelspieler: textstyle 1 (Blinken) blinkt nicht mehr, umbrochener Text behält mit dem Fokus seine Farbe, Ownerdraws blinken im Takt des Einzelspielers; der Fokuspuls springt wie im Spiel.
- Menü-Editor: Ein Seitenwechsel aus einem Skript, der Hierarchie, der Gliederung oder der Problemliste leert die Auswahl wie der Reiter (vorher konnte das Programm abstürzen oder ein Element eines anderen Menüs löschen).
- Menü-Editor: Ein Ordner neben GameData, dessen Name mit „GameData“ beginnt, gilt beim Speichern nicht mehr als Spielordner.
- Missions-Vorschau, Movie Duels: Der Turbolaser (misc_turretG2) spielt keine Klänge für Starten, Abschalten und Ping; ein Fahrzeug mit Pilot ist ein Ziel der Geschütze mit dem Team des Piloten, ein in einem geschlossenen Fahrzeug verborgener Reiter nicht.
- Wegpunkt-Erzeugung: Eine Kante zu einem vorhandenen Wegpunkt mit vollen target-Feldern fällt nicht mehr still weg; das Netz bleibt verbunden.

### 中文
- 菜单编辑器,操作模式:移动指针时,指针下的每个项(包括装饰项)都执行 mouseEnter,焦点交给第一个能接受它的项(单人模式下不交给非活动项),与游戏一致;焦点一直保留到另一个项取得它,单击作用于拥有焦点的项。
- 菜单编辑器:居中的输入框以及居中或右对齐的 ownerdraw 像游戏一样放置文本(cvar 的值和 ownerdraw 的宽度一并计入);Movie Duels 单人模式按半像素居中。
- 菜单编辑器,单人模式:textstyle 1(闪烁)不再闪烁,自动换行的文本在获得焦点时保持颜色,ownerdraw 按单人模式的节奏闪烁;焦点脉冲像游戏一样分级跳变。
- 菜单编辑器:由脚本、层级、大纲或问题列表切换页面时,像页面选项卡一样清空选择(以前可能崩溃或删除另一个菜单中的项)。
- 菜单编辑器:GameData 旁边名称以“GameData”开头的文件夹在保存时不再被视为游戏文件夹。
- 任务预览,Movie Duels:涡轮激光炮(misc_turretG2)不再播放启动、关闭和 ping 声音;有驾驶员的载具是炮塔的目标,队伍为驾驶员的队伍,隐藏在封闭载具中的骑乘者则不是。
- 路点生成:通往 target 字段已满的现有路点的边不再悄悄丢失;网络保持连通。

### 日本語
- メニューエディター、操作モード: ポインターを動かすと、その下のすべての項目(装飾も)で mouseEnter が実行され、フォーカスは受け取れる最初の項目に移ります(シングルプレイヤーでは非アクティブな項目には移りません)。ゲームと同じです。フォーカスは別の項目が取るまで残り、クリックはフォーカスのある項目に作用します。
- メニューエディター: 中央揃えの入力欄と、中央揃えまたは右揃えの ownerdraw はゲームと同じ位置にテキストを置きます(cvar の値と ownerdraw の幅も数えます)。Movie Duels のシングルプレイヤーは半ピクセル単位で中央揃えします。
- メニューエディター、シングルプレイヤー: textstyle 1(点滅)は点滅しなくなり、折り返したテキストはフォーカスがあっても色を保ち、ownerdraw はシングルプレイヤーの間隔で点滅します。フォーカスの脈動はゲームと同じように段階的に変わります。
- メニューエディター: スクリプト、階層、アウトライン、問題一覧からページを切り替えると、ページタブと同じく選択が解除されます(以前はクラッシュしたり、別のメニューの項目を削除したりすることがありました)。
- メニューエディター: GameData の隣にある、名前が「GameData」で始まるフォルダーは、保存時にゲームフォルダーとして扱われなくなりました。
- ミッションプレビュー、Movie Duels: ターボレーザー(misc_turretG2)は起動・停止・ping の音を鳴らしません。パイロットの乗った乗り物はパイロットのチームでタレットの標的になり、閉じた乗り物に隠れた乗り手は標的になりません。
- ウェイポイント生成: target 欄が埋まっている既存のウェイポイントへの辺が黙って消えることはなくなり、ネットワークはつながったままです。

### Español
- Editor de menús, modo Usar: mover el puntero ejecuta mouseEnter en cada elemento que hay debajo, también en las decoraciones, y da el foco al primer elemento que puede tomarlo (en un jugador no a uno inactivo), como el juego; el foco se queda hasta que otro elemento lo toma, y un clic actúa sobre el elemento con el foco.
- Editor de menús: los campos de texto centrados y los ownerdraw centrados o alineados a la derecha colocan su texto como el juego (cuentan el valor de la cvar y el ancho del ownerdraw); Movie Duels en un jugador centra con medios píxeles.
- Editor de menús, un jugador: textstyle 1 (parpadeo) ya no parpadea, el texto ajustado conserva su color con el foco, los ownerdraw parpadean al ritmo de un jugador; el pulso del foco avanza a saltos como en el juego.
- Editor de menús: cambiar de página desde un script, la jerarquía, el esquema o la lista de problemas vacía la selección como la pestaña de página (antes podía fallar o borrar un elemento de otro menú).
- Editor de menús: una carpeta junto a GameData cuyo nombre empieza por «GameData» ya no se trata como carpeta del juego al guardar.
- Vista previa de misiones, Movie Duels: el turboláser (misc_turretG2) no reproduce los sonidos de arranque, apagado ni ping; un vehículo con piloto es objetivo de las torretas con el equipo del piloto, un jinete oculto en un vehículo cerrado no.
- Generación de puntos de ruta: una arista hacia un punto de ruta existente con los campos target llenos ya no desaparece en silencio; la red sigue conectada.

### Čeština
- Editor menu, režim Ovládat: pohyb ukazatele spustí mouseEnter u každého prvku pod ním, i u dekorací, a dá fokus prvnímu prvku, který ho může převzít (v jednom hráči ne neaktivnímu), jako ve hře; fokus zůstane, dokud ho nepřevezme jiný prvek, a kliknutí působí na prvek s fokusem.
- Editor menu: vystředěná vstupní pole a vystředěné nebo doprava zarovnané ownerdraw umísťují text jako hra (počítá se hodnota cvar a šířka ownerdraw); Movie Duels v jednom hráči středí na poloviny pixelů.
- Editor menu, jeden hráč: textstyle 1 (blikání) už nebliká, zalomený text si s fokusem ponechá barvu, ownerdraw blikají v rytmu jednoho hráče; pulz fokusu se mění po skocích jako ve hře.
- Editor menu: přepnutí stránky ze skriptu, hierarchie, osnovy nebo seznamu problémů vyprázdní výběr jako karta stránky (dříve mohl program spadnout nebo smazat prvek jiného menu).
- Editor menu: složka vedle GameData, jejíž název začíná na „GameData“, se při ukládání už nepovažuje za složku hry.
- Náhled mise, Movie Duels: turbolaser (misc_turretG2) nehraje zvuky spuštění, vypnutí ani ping; vozidlo s pilotem je cílem věží s týmem pilota, jezdec skrytý v uzavřeném vozidle ne.
- Generování waypointů: hrana k existujícímu waypointu s plnými poli target už tiše nezmizí; síť zůstane propojená.
