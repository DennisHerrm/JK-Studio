# Notizen rc609

## W1: Wegpunkte loeschen, verbinden, duplizieren

Auftrag `auftrag_rc609_w1.md` (Plan A, C, D aus `plaene/wegpunkte_und_enttext.md`, Mikes Rueckmeldung vom 03.11.). Skripte in `<SP neu>/w1` (Patchskripte, Rotlauf `rot.py`).

### 2: Kantenpruefung wie im Spiel (eigene Funktion)
- `NAV::TestEdge` (PHASE III) und das Spawnen der Knoten aus `baueNavNetz` herausgeloest (`src/navlauf.cpp`): `NavSpawnSicht` (Brush-Entities zur Zeit des Spawnens, G_CheckInSolid), `navKnotenSpawnen` (SP_waypoint, _small, _navgoal, point_combat bis NAV::SpawnedPoint), `navKnotenAbsetzen` (Drop To Floor), `navTeams` (G_FindTeams), `navTestEdge` und `navTestEdgeGroessen` (gross, dann mittel). JK2: `jk2KnotenSpawnen`, `jk2HardConnectSpur`, `jk2Blocker` (aus `baueNavNetzJk2`, auch fuer CheckBlockedEdges und `jk2KanteSperrt`).
- Neu oeffentlich: `NavKantenPruefer` (`include/bhed/navlauf.h`). Simulation und Editor benutzen dieselben Teile; die Simulation ist unveraendert (ctest bewegung, jk2nav, wegpunkte gruen nach dem Herausloesen).
- `gui/app_bewegung.cpp`: `navBauTeile()` (Brush-Entities, Spuren PHASE 0) aus `navWeltBasis` herausgeloest, dazu `navBauHilfeKarte()` fuer den Editor.
- **OpenJK gegen Movie Duels 9:** TestEdge (OJ `code/game/g_navigator.cpp:1211`, MD `:1197`) und MoveTrace (OJ `:861`, MD `:853`) sind nach Abzug der Schreibweise gleich. MD hat den toten Block `CHECK_JUMP` (in beiden `CHECK_JUMP = 0`) entfernt. Die Groessenschleife (MD `:1843-1862`, OJ `:1914-1923`) ist gleich. **Kein Umschalter noetig.** JK2 hat kein TestEdge: HardConnect (`codeJK2/game/g_navigator.cpp:1083-1114`) verbindet immer und setzt nur EFLAG_BLOCKED; der Editor nimmt als begehbar, was nicht gesperrt ist oder nur von einer Tuer/etwas Zerbrechlichem gesperrt wird (wie NAV_TestBestNode zur Laufzeit).
- Nicht portiert (wie vorher): der Zweig `hit_character` von TestEdge (ein NPC oder der Spieler im Weg beim Netzbau) - die Spuren des Netzbaus kennen keine Koerper.
- ctest `wegkanten` (neu, `tests/wegkantentest.cpp`): echte Brushes (kollisionsSpur), Pruefer gegen Netzbau der Simulation, je Richtung, je Spiel. Ergebnis JKA: Boden 150 gross, Treppe Fuss -> 2. Stufe gross, Treppe ganz weg, Kante 60 weg, Wand weg, Rampe 0,7 mittel, Gang 60 mittel. JK2: Boden, Treppe kurz, Rampe, Gang frei; Treppe ganz, Kante 60, Wand gesperrt. 28/28.
- Befund am Rand: Ein Wegpunkt 25 ueber einer 0,7-Rampe ist im Spiel "in solid" (die Ecke des Kastens -16..16 beruehrt die Schraege, G_CheckInSolid mit fix, g_utils.cpp:1637 MD). Im Test stehen die Rampenpunkte darum 42 ueber der Schraege.

### 1: Loeschen raeumt auf
- `entitiesLoeschen` (`gui/app_mission.cpp`): Entf, Knopf im Inspektor, Mehrfachauswahl, neu Kontextmenue "Delete". Alle `target`..`target4`/`targetJump` auf geloeschte Namen gehen im selben Rueckgaengig-Schritt mit (`verweiseEntfernen`, dieselbe Funktion nutzt jetzt `wegGenerierteEntfernen`). Ein Name, den eine bleibende Entity traegt, bleibt.
- Andere Entities (nicht Wegpunkte) mit Verweisen: Rueckfrage im Programm (`frage()`, ImGui): Ja entfernt die Verweise, Nein laesst sie. `entityLoeschen(i)` (ohne Frage, fuer Selbsttests/Fuzzer) nimmt nur die Verweise auf Wegpunkte mit.
- "Luecke schliessen" (Fenster, Vorgabe aus): Nachbarn eines geloeschten Wegpunkts verbinden sich, wo der Pruefer es erlaubt (kuerzeste Paare zuerst).

### 3: Automatisch verbinden
- `wegKantenAbgleichen`: beim Setzen, nach dem Ziehen (Abschluss `entityZiehen`, mit Gruppe und Alt-Kopie) und nach dem Duplizieren. Nach dem Ziehen erst trennen, was TestEdge nicht mehr besteht (Sprungkanten prueft das Spiel nicht), dann mit den Wegpunkten im Umkreis verbinden (Umkreis = Rasterweite `wegAbstand`, Regler im Fenster), naechste zuerst. Alles in einem Rueckgaengig-Schritt.
- Wie das Spiel Kanten bildet: nur aus target..target4/targetJump (in beide Richtungen, eine Kante genuegt); Auto-Connect gibt es nur fuer Kampf- und Zielpunkte (MD `g_navigator.cpp:1688-1800`). Darum schreibt der Editor nur Kanten zwischen waypoint/waypoint_small, keine doppelten, und nur eine Seite traegt das Feld (zuerst der neue Punkt, sind seine vier Felder voll, der Nachbar). targetJump wird nie automatisch gesetzt.
- Statuszeile: "Verbindungen wie im Spiel: n neu, m getrennt", "n Nachbarn nicht verbunden: ... alle vier target-Felder belegt", "n Wegpunkte nicht verbunden: das Spiel verwirft ihn als im Festen".
- Schalter "Automatisch verbinden" (Vorgabe an), Knopf "Mit Nachbarn verbinden" (Fenster und Kontextmenue).

### 4: Duplizieren ohne Umbenennen
- Strg+D (ueber Karte oder Entity-Liste; in der BehavEd-Belegung ist Strg+D frei) und Kontextmenue "Duplicate". `wegKopienUmschreiben`: Verweise innerhalb der Kopie zeigen auf die neuen Namen, Verweise nach aussen bleiben; danach Auto-Verbinden ohne die Quellen (sonst haengt die Kopie an ihren Vorlagen). Gilt auch fuer Alt+Ziehen.
- `kopieAnhaengen` ohne eigenen Rueckgaengig-Schritt ist **kein Fehler**: beide Aufrufer (Mehrfach-Duplizieren, Alt+Ziehen) legen den Schritt selbst an bzw. nehmen die Kopien bei Alt ohne Zug wieder weg.
- "Umbenennen nach Muster" (`#` = Nummer, vergebene Namen uebersprungen) fuer gewaehlte waypoint/waypoint_small; alle Verweise ziehen mit. Navgoals nicht (Skripte nennen sie).

### 5: Startpunkte der Erzeugung
- Rot: Die Erzeugung nahm nur `waypoint_navgoal`, nicht waypoint/waypoint_small. Jetzt zaehlen sie; ein Knoten aus einem vorhandenen Wegpunkt ist dieser Wegpunkt (keine Kopie an derselben Stelle), die erzeugten Nachbarn nennen ihn (`NeuerWegpunkt::start`).
- Selbsttest `wegpunkte`: die Pruefung "kein target fehlt" nimmt die vorhandenen Wegpunkte dazu, die erzeugte nennen; Setzen/Verbinden von Hand dort mit Auto-Verbinden aus (das prueft `wegpunkte2`).

### Tests
- Selbsttest `wegpunkte2` (neu, `gui/selbsttest_wegpunkte2.inc`) auf md_am_sith. **Rot** (`w2rot`, Behebungen mit `rot.py` zurueckgenommen, sonst dieselbe exe): 16 OK, 16 FEHLER - Start: 0 von 165 Rasterpunkten im abgetrennten Bereich erzeugt; Loeschen: danach 6 von 6 Verweisen noch da; Luecke: 0 neue Kanten; Frage: keine Rueckfrage; Setzen: 0 Nachbarn; Duplizieren: Kopien zeigen 2x auf die Quellen, innen 0 Kanten.
- **Gruen** (`w2d`, 29 OK, 0 FEHLER): Start 164 von 165 Rasterpunkten im abgetrennten Bereich, der gesetzte Wegpunkt hat 3 Nachbarn; Loeschen 6 -> 0 Verweise, +1 Schritt, Rueckgaengig 6, Wiederholen 0; Luecke 13 neue Kanten unter 18 offenen Nachbarpaaren, 0 nicht begehbar; Frage: Ja entfernt den 1 Verweis auf `armshields` (func_usable), Nein laesst ihn; Setzen: 4 Nachbarn, alle begehbar (Pruefer in beide Richtungen), Undo/Redo; Ziehen hinter eine Wand: 1 Kante getrennt, 3 bleiben, 0 nicht begehbar, Undo stellt Ort und Kanten her; Strg+D: 3 Kopien, innen 2 Kanten wie die Quellen (3/3 Paare gleich), 0 Verweise auf Quellen, Namen frei; Umbenennen `w2kopie_#` mit Verweisen; Kontextmenue Delete nimmt die drei samt Verweisen weg. Je Aktion genau ein Schritt.
- Dabei im Test gefunden und in den Test genommen: Ein von Hand gesetzter Punkt nahe einer Wand ist im Spiel "in solid" (G_CheckInSolid) und bekommt keine Kanten - jetzt mit Meldung in der Statuszeile; der Test sucht nur Stellen, die das Spiel behaelt.
- Handbuchbilder `h_weg_netz` (2560 x 1248..1324) und `h_weg_kontext` (2560 x 911) in en, de, zh, ja, es, cs (`hbw_*`, `hbk_*`, je Sprache parallel, 0 FEHLER); `gen_handbuch.py`: es/cs 0 Texte englisch. `hilfe_orte.inc` mit `gen_hilfe_orte.py` neu.
- Schluss: Bau aller Ziele 0 Warnungen. ctest 197: 196 gruen, `hilfe_orte` rot (veraltet) -> neu erzeugt, gruen. Lints sources, fassung, i18n, i18n_erzeugt, headers, feldinit, gapp, settings ok, portability 106 (Basis; der neue Fund in wegkantentest.cpp behoben).
- `wegpunkte` (`wpend`): 48 OK, 5 FEHLER bei Gruppen (Rahmen waehlt 1 statt 3, Strg-Klick, Strg+Z der Gruppe, Alt+Ziehen). **Mit der rc608-exe (`laeufe/rel608`) dieselben 5 FEHLER** (`wp608`, 48 OK) - kein Rueckschritt von W1, offen.
- `mission1` (`m1end`, parallel zu ctest -j 12 und `wegpunkte`): 96 OK, 2 FEHLER (Strg+Y nach dem Kamera-Undo academy1). Gegenprobe rc608-exe 98/0; einmal wiederholt mit derselben exe (`m1end2`, ohne Last): 98 OK, 0 FEHLER. Einmaliger Ausfall unter Volllast, als zeitabhaengig notiert.
- Handbuch: Seite NPC-Wegpunkte, neuer Abschnitt "Loeschen, automatisch verbinden, duplizieren" in 6 Sprachen, Bilder `h_weg_netz`, `h_weg_kontext` aus `BHED_EDITORTEST=handbuch BHED_HB_NUR=wegpunkte` (neu, md_am_sith, `BHED_HB_FENSTER=3840x2160@2`), je Sprache parallel.

### Offen
- Das Setzen per Klick legt den Punkt 25 ueber den Boden; an Waenden und auf Schraegen verwirft das Spiel ihn (jetzt mit Meldung). Ein Setzen, das den Punkt selbst an eine gueltige Stelle rueckt, waere ein eigener Wunsch.
- Kanten zwischen einer Kopie (Strg+D) und Punkten, die nach einem spaeteren Ziehen noch begehbar sind, bleiben (TestEdge kennt keine Laenge).

## English
- NPC waypoints: deleting an entity also removes every target to targetJump that named it, in the same undo step; for entities other than waypoints the program asks first. "Close the gap when deleting" connects the neighbours of a deleted waypoint.
- NPC waypoints: a waypoint you place, move or duplicate connects to its neighbours with the game's own edge test (NAV::TestEdge, HardConnect in Jedi Outcast); connections the game would drop after a move are removed. Switch "Connect automatically" and button "Connect to neighbours".
- NPC waypoints: Ctrl+D and the right-click menu duplicate waypoints with free names, the copies stay connected among themselves; rename the selection by a pattern. The right-click menu of the view now has Duplicate, Delete and Connect to neighbours.
- NPC waypoints: the generation also starts from waypoints already on the map, so a room it missed gets its network after you place one waypoint there.

## Deutsch
- NPC-Wegpunkte: Beim Löschen einer Entity verschwindet jedes target bis targetJump, das sie nannte, im selben Rückgängig-Schritt; bei anderen Entities als Wegpunkten fragt das Programm erst. "Lücke beim Löschen schließen" verbindet die Nachbarn eines gelöschten Wegpunkts.
- NPC-Wegpunkte: Ein Wegpunkt, den du setzt, verschiebst oder duplizierst, verbindet sich mit seinen Nachbarn nach der Kantenprüfung des Spiels (NAV::TestEdge, in Jedi Outcast HardConnect); Verbindungen, die das Spiel nach dem Verschieben verwerfen würde, fallen weg. Schalter "Automatisch verbinden" und Knopf "Mit Nachbarn verbinden".
- NPC-Wegpunkte: Strg+D und das Rechtsklickmenü duplizieren Wegpunkte mit freien Namen, die Kopien bleiben untereinander verbunden; die Auswahl lässt sich nach Muster umbenennen. Das Rechtsklickmenü der Ansicht hat jetzt Duplizieren, Löschen und Mit Nachbarn verbinden.
- NPC-Wegpunkte: Die Erzeugung beginnt auch an Wegpunkten, die schon auf der Karte stehen - ein ausgelassener Raum bekommt sein Netz, wenn du dort einen Wegpunkt setzt.

## 中文
- NPC 路径点:删除实体时,所有指向它的 target 到 targetJump 会在同一个撤销步骤中一并移除;对于路径点以外的实体,程序会先询问。"删除时填补空缺"会连接被删路径点的相邻点。
- NPC 路径点:你放置、移动或复制的路径点会按游戏自身的连线检查(NAV::TestEdge,Jedi Outcast 中为 HardConnect)与相邻点相连;移动后游戏会丢弃的连接将被移除。新增"自动连接"开关和"连接到邻近点"按钮。
- NPC 路径点:Ctrl+D 和右键菜单可复制路径点并赋予空闲名称,副本之间保持相连;所选路径点可按模式重命名。视图的右键菜单现在有复制、删除和连接到邻近点。
- NPC 路径点:生成也会从地图上已有的路径点开始——在遗漏的房间里放一个路径点,该房间就会得到路网。

## 日本語
- NPC ウェイポイント: エンティティを削除すると、それを指していた target から targetJump も同じ元に戻す 1 回で消えます。ウェイポイント以外のエンティティでは先に確認します。「削除時に隙間を埋める」で削除したウェイポイントの隣どうしを接続します。
- NPC ウェイポイント: 置いた・移動した・複製したウェイポイントは、ゲーム自身の辺チェック(NAV::TestEdge、Jedi Outcast では HardConnect)で近くの点と接続します。移動後にゲームが捨てる接続は削除します。「自動で接続」スイッチと「近くの点と接続」ボタンを追加。
- NPC ウェイポイント: Ctrl+D と右クリックメニューでウェイポイントを空いている名前で複製し、コピーどうしの接続は保たれます。選択をパターンで名前変更できます。ビューの右クリックメニューに複製・削除・近くの点と接続を追加。
- NPC ウェイポイント: 生成はマップにすでにあるウェイポイントからも始まります。抜けた部屋に 1 つ置けば、その部屋にもネットワークができます。

## Español
- Puntos de ruta NPC: al borrar una entidad desaparece cada target a targetJump que la nombraba, en el mismo paso de deshacer; para entidades que no son puntos de ruta el programa pregunta antes. "Cerrar el hueco al borrar" conecta los vecinos de un punto borrado.
- Puntos de ruta NPC: un punto que colocas, mueves o duplicas se conecta con sus vecinos según la propia prueba de aristas del juego (NAV::TestEdge, HardConnect en Jedi Outcast); tras moverlo se quitan las conexiones que el juego descartaría. Interruptor "Conectar automáticamente" y botón "Conectar con los vecinos".
- Puntos de ruta NPC: Ctrl+D y el menú del botón derecho duplican puntos con nombres libres, las copias siguen conectadas entre sí; la selección se puede renombrar con un patrón. El menú del botón derecho de la vista tiene ahora Duplicar, Borrar y Conectar con los vecinos.
- Puntos de ruta NPC: la generación también empieza en los puntos que ya están en el mapa; una sala omitida recibe su red cuando colocas allí un punto.

## Čeština
- Body trasy NPC: při mazání entity zmizí každé target až targetJump, které ji jmenovalo, v tomtéž kroku zpět; u jiných entit než bodů trasy se program nejdřív zeptá. "Při mazání uzavřít mezeru" spojí sousedy smazaného bodu.
- Body trasy NPC: bod, který položíš, přesuneš nebo zduplikuješ, se spojí se sousedy podle vlastního testu hran hry (NAV::TestEdge, v Jedi Outcast HardConnect); po přesunu zmizí spojení, která by hra zahodila. Přepínač "Automaticky spojovat" a tlačítko "Spojit se sousedy".
- Body trasy NPC: Ctrl+D a nabídka pravého tlačítka duplikují body s volnými jmény, kopie zůstanou navzájem spojené; výběr lze přejmenovat podle vzoru. Nabídka pravého tlačítka v pohledu má nyní Duplikovat, Smazat a Spojit se sousedy.
- Body trasy NPC: generování začíná i u bodů, které už na mapě jsou; vynechaná místnost dostane síť, když do ní položíš jeden bod.
