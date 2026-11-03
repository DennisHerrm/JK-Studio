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

## W2a: Dateien an Ort und Stelle speichern, Eintrag in .pk3 ersetzen, Funde zum Speichern

Auftrag `auftrag_rc609_w2a.md` (Plan I, Mike 03.11.; Funde F1-F8, F18 aus `pruefung_b5b1_funde.md`). Patch- und Rotskripte in `<SP neu>/w2a` (`rot.py`).

### 1: Herkunft je Datei
- `DateiHerkunft` (`gui/app_internal.h`): lose Datei oder .pk3 + Eintrag. `kartenHerkunft` (setzen `loadMap`, `loadMapFromArchive` - das gewinnende Archiv wie die Engine, `karteAusArchivOeffnen`, pk3-Browser, `loadMission`; `loadMapFromBytes` setzt sie zurueck, eine neue Mission hat keine eigene Datei), `entHerkunft` + `entHerkunftKarte` (".ent oeffnen", .ent einer Mission), `skriptHerkunft` je Reiter (auch in `Parked`, mit Reiterwechsel). Reiter merken die Herkunft ihrer Karte: eine .pk3 ausserhalb der Spielordner laedt beim Zurueckwechseln wieder (`karteAusPk3Nachladen`).

### 2: Getrennte Befehle
- Strg+S je Reiter wie bisher; ein Skript aus einer .pk3 (ohne Pfad) bietet die Wege an statt "Speichern unter".
- "Entities speichern" (Menue Karten und Dateien, Datei-Menue): .ent an den geoeffneten Ort, sonst neben die .bsp (lose) bzw. als `maps/x.ent` der .pk3; neue Mission: ihr Ordner.
- "Entities in die BSP uebernehmen": `karte::bspNurEntitaetenEinsetzen` - OnlyEnts aus q3map2 1:1 portiert (ParseEPair mit StripTrailing und Wegfall leerer Werte, SetModelNumbers, SetLightStyles mit -keeplights wie der Karten-Editor, `_q3map2_cmdline`/`_q3map2_version`/`gridsize` aus dem Original, UnparseEntities mit NUL), dann `bspEntitaetenEinsetzen`. Vorher `x.bsp.bak`. Liegt eine .ent daneben, sagt eine Meldung, dass das Spiel sie vorzieht (cm_load.cpp:470-483).
- Gegenprobe gegen das echte `bhed_q3map2 -onlyents`: ctest `karte_bspoeffnen` auf raum.nrc.bsp (18516 Bytes) und md_am_sith aus Movie Duels (16613144 Bytes) mit geaenderter worldspawn, verschobenem Start, neuem info_null, schaltbarem Licht (style 5), Licht ohne targetname, misc_model, Schluessel mit Leerzeichen am Ende und leerem Wert: Byte-gleich. Selbsttest `dateiort` B: 16612848 / 16612848 Bytes, gleich.

### 3: Alle Wege anbieten
- Frage im Programm (ImGui, `drawSpeicherWeg`, "###speicherweg"): 1 in der .pk3 aendern, 2 lose Datei im Mod-Ordner (der Ordner der .pk3), 3 Speichern unter, 4 Missionsordner + Pack-Dialog (nur Skripte/Entities mit offener Mission). "Diese Wahl merken" -> Einstellung `pk3SpeicherWeg` (Einstellungen > Dateien speichern, Vorgabe fragen).
- Skript aus .pk3: geoeffnet die .IBI -> uebersetzt; steht die andere Fassung (.txt/.IBI) auch in der .pk3, gehen beide in einem Durchgang mit (`pk3EintraegeErsetzen`), sonst liefe die alte .IBI bzw. zeigte das naechste Oeffnen die alte .txt.
- Lose geoeffnete Dateien: ohne Rueckfrage an ihren Pfad. "Save mission" speichert jede Datei der Mission an ihren Ort (`missionAnOrtSpeichern`); "Als .pk3 packen..." (Menue Karten und Dateien) ist der freiwillige Pack-Dialog. Packen kopiert nur noch in den Missionsordner, wenn er der Ort der Mission ist (neu angelegt/aus dem Ordner) oder mit der Einstellung "Bisheriger Missionsordner-Ablauf" - die stellt auch "Save mission" auf den alten Dialog und alle .pk3-Dateien auf Weg 4.
- Skriptvorlage `//Generated by BehavEd` nur mit der Einstellung "Neue Skripte bekommen die BehavEd-Vorlage" (Vorgabe aus): ohne sie ein leerer Reiter auf `scripts/<ref>.txt` im Missionsordner, die Datei entsteht beim ersten Speichern.
- Befund fuer Weg 2 (Engine, OJK `files.cpp:2789`, MD9 `:2861`): mit `fs_dirbeforepak 0` (Vorgabe) liest das Spiel die .pk3 eines Ordners VOR seinen losen Dateien. Eine lose Datei neben der .pk3 wirkt nur, wenn kein Archiv sie hat, im fs_homepath oder mit `+set fs_dirbeforepak 1`. Steht im Hinweis der Frage und im Handbuch.

### 4: Eintrag in einer vorhandenen .pk3 ersetzen
- `pk3EintraegeErsetzen` / `pk3EintragErsetzen` (`src/pk3write.cpp`): lokale Bloecke (Kopf, komprimierte Daten, Datenbeschreibung, Luecken) roh kopiert, Zentraleintraege roh mit neuem Versatz, Ordnereintraege und Archivkommentar bleiben; neuer Eintrag ohne Packung mit CRC-32, Zeit/Datum/UTF-8-Flag/Attribute des alten; fehlende werden angehaengt. Zip64 und Verschluesselung abgelehnt. Vorher `<pk3>.bak` (Stand vor diesem Speichern), dann `<pk3>.tmp` und `MoveFileExW`. Gesperrt (Spiel offen, `CreateFileW` mit DELETE scheitert an der Freigabe) -> Meldung im Programm, nichts geaendert.
- ctest `pk3ersetzen` (neu): 5 Eintraege (2 ohne Packung, 3 deflate, einer mit Zusatzfeld, einer mit Datenbeschreibung) + Ordner + Kommentar. Rot (Stub): 7 Fehler. Gruen: 4 von 4 anderen roh gleich, `readPk3File` liest alle, CRC lokal und im Verzeichnis, .bak, Anhaengen (6 Eintraege), zwei Eintraege in einem Durchgang, gesperrt: false mit Meldung, Original Byte-gleich.

### 5: Sicheres Schreiben (F5)
- `sicherSchreiben` (`src/sicherschreiben.cpp`): `<pfad>.tmp`, Schreiben und `close()` geprueft, `MoveFileExW(MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)`, bei Fehler Temp weg, alte Datei unveraendert. `spew` (app.cpp), `schreibe` (app_mission.cpp) und damit Skript-, Missions-, .ent-, .IBI- und mission.txt-Schreiber gehen darueber; `doSave` zeigt bei Fehler eine Meldung im Programm statt eines Windows-Dialogs und meldet nicht "gespeichert".
- ctest `sicherschreiben` (neu, simulierter Fehler beim Schreiben/Schliessen/Umbenennen, gesperrte Datei): rot mit dem alten Weg 12 Fehler (Datei nach dem Fehler 9545 statt 29 Bytes, Fehler beim Schliessen meldete true, eine offene Datei wurde ueberschrieben), gruen 0.

### 6: Funde
- **F1** run-Skripte fehlten im Missions-pk3: jetzt jedes Skript im Skriptordner der Mission und die Huelle ueber `run`/`set(SET_*SCRIPT)` (`skriptFolgeVerweise`). Selbsttest rot: teil2.IBI fehlt; gruen: da.
- **F2** leere .ent: Rueckfall auf die .ent im Missionsordner bzw. Archiv, sonst den Entity-Lump der .bsp. Rot 0 Entities, gruen 397 von 397.
- **F3** Neuladen derselben Karte: Frage im Programm bei ungespeicherten Entities, danach Verlauf leer. Rot: keine Frage, Strg+Z moeglich; gruen: Frage, frisch, kein Strg+Z.
- **F4** ".ent oeffnen": `collectNames` (auch Skriptliste), `entityNummernVergessen`, Verlauf leer. Rot: Auswahl 2, Undo ja, Skript fehlt; gruen: 0, nein, ja.
- **F6** `..`, `::`, Laufwerk, fuehrender Strich in Skript-/Dateipfaden: Fehler S007, kein Eintrag (ctest `missionbau`: rot 0 Befunde und ein `..`-Eintrag, gruen 3 Befunde).
- **F7** mission.txt, .pk3 und .bak nicht mehr ins veroeffentlichte pk3 (Selbsttest rot: mission.txt im pk3).
- **F8** 64 Paare / 2048 Zeichen je Entity (OJK `g_spawn.cpp:1188, 1255`, `g_local.h:221-222`; MD9 `g_local.h:246-247`, `g_spawn.cpp:1353, 1427`; JK2 gleich): Fehler S008/S009 beim Packen, "Entities speichern" und "In die BSP uebernehmen". ctest `missionbau` rot 3 Fehler, gruen; Grenze genau 2048 erlaubt, 2049 nicht.
- **F18** Archivnamen mit fuehrendem `/`: im Index nicht mehr gekuerzt (FS_FilenameCompare), der gesuchte Name weiter (files.cpp:1276). ctest `archiv` rot -> gruen. Der gesuchte Name verliert wie bisher auch ein fuehrendes `./`, das die Engine nicht entfernt - unveraendert gelassen (kein Fund, keine Pruefung).

### Tests
- Selbsttest `dateiort` (neu, `gui/selbsttest_dateiort.inc`, Mod-Ordner im Laufordner, md_am_sith nur gelesen): **rot** (`w2arot`, `rot.py`, sonst dieselbe exe) 3 OK, 23 FEHLER - A: nur das Skript neu, .bsp/.ent/.bak nein; B: 16612820 statt 16612848 Bytes (unveraendert); C/D/E: keine Frage, .pk3 unveraendert, lose .ent 0 Bytes; F1, F7 (mission.txt im pk3), E Vorlage, F2 0 von 397, F3, F4. **Gruen** (`w2agruen2`) 19 OK, 0 FEHLER. Ein Testfehler im ersten Gruenlauf (die Aenderung war ein `rem`, das wie bei IBIze nicht in die .IBI kommt) - im Test auf `wait` umgestellt.
- ctest: alle bis auf `hilfe_orte` (veraltet, neu erzeugt) und `handbuch_sprachen` (einmal rot unter Last waehrend die Handbuchbilder noch fehlten, danach gruen).
- Im Spiel: nicht gestartet (gemeinsame Regeln: nie Spiel-exes starten). Die geaenderte lose .bsp ist Byte-gleich zum q3map2 -onlyents-Ergebnis; Lumps 1-17 Byte-gleich zum Original.
- Handbuch: Seite Missionen, Abschnitt "Dateien dort speichern, wo sie geoeffnet wurden", "Eine Mission speichern" angepasst, Bilder `h_speichern_menue`, `h_speicherweg` (2560 breit) aus `BHED_EDITORTEST=handbuch BHED_HB_NUR=speichern` je Sprache parallel.

### Offen
- Weg 3 (Speichern unter) oeffnet den Windows-Dateidialog des Nutzers - im Selbsttest nicht geprueft.
- `readFromArchives` kennt keine losen Dateien (F17, nicht Teil von W2a): nach Weg 2 zeigt ein erneutes Oeffnen aus der .pk3 den alten Stand, wie das Spiel ohne fs_dirbeforepak.
- Werte ueber 1023 Zeichen bricht q3map2 beim Lesen der .map ab (MAXTOKEN); der Port tut das nicht.

## English
- NPC waypoints: deleting an entity also removes every target to targetJump that named it, in the same undo step; for entities other than waypoints the program asks first. "Close the gap when deleting" connects the neighbours of a deleted waypoint.
- NPC waypoints: a waypoint you place, move or duplicate connects to its neighbours with the game's own edge test (NAV::TestEdge, HardConnect in Jedi Outcast); connections the game would drop after a move are removed. Switch "Connect automatically" and button "Connect to neighbours".
- NPC waypoints: Ctrl+D and the right-click menu duplicate waypoints with free names, the copies stay connected among themselves; rename the selection by a pattern. The right-click menu of the view now has Duplicate, Delete and Connect to neighbours.
- NPC waypoints: the generation also starts from waypoints already on the map, so a room it missed gets its network after you place one waypoint there.
- Saving: files are saved where they were opened - a loose .bsp, .ent or script without a question, no copy under Documents, no new .pk3. New commands "Save entities" and "Write entities into the BSP" (byte for byte like the map editor's -onlyents, backup .bsp.bak). A file from a .pk3 asks: change it in the .pk3 (only this entry, backup .pk3.bak, message if the game has it open), loose file in the mod folder, save as, or the previous mission folder way; the choice can be remembered. "Save mission" saves in place, "Pack as .pk3..." is a separate command; the script template and the previous workflow are settings.
- Saving: every file is written through a temporary file and renamed only when complete; on an error the old file stays and no "saved" message appears. Packing a mission now includes scripts started only by run, never writes an empty .ent, leaves out mission.txt and old .pk3 files, and refuses paths with ".." and entities over the game's limits (64 keys, 2048 characters). Reloading the same map asks before discarding entity changes; opening a .ent clears undo, selection and the script list.

## Deutsch
- NPC-Wegpunkte: Beim Löschen einer Entity verschwindet jedes target bis targetJump, das sie nannte, im selben Rückgängig-Schritt; bei anderen Entities als Wegpunkten fragt das Programm erst. "Lücke beim Löschen schließen" verbindet die Nachbarn eines gelöschten Wegpunkts.
- NPC-Wegpunkte: Ein Wegpunkt, den du setzt, verschiebst oder duplizierst, verbindet sich mit seinen Nachbarn nach der Kantenprüfung des Spiels (NAV::TestEdge, in Jedi Outcast HardConnect); Verbindungen, die das Spiel nach dem Verschieben verwerfen würde, fallen weg. Schalter "Automatisch verbinden" und Knopf "Mit Nachbarn verbinden".
- NPC-Wegpunkte: Strg+D und das Rechtsklickmenü duplizieren Wegpunkte mit freien Namen, die Kopien bleiben untereinander verbunden; die Auswahl lässt sich nach Muster umbenennen. Das Rechtsklickmenü der Ansicht hat jetzt Duplizieren, Löschen und Mit Nachbarn verbinden.
- NPC-Wegpunkte: Die Erzeugung beginnt auch an Wegpunkten, die schon auf der Karte stehen - ein ausgelassener Raum bekommt sein Netz, wenn du dort einen Wegpunkt setzt.
- Speichern: Dateien werden dort gespeichert, wo sie geöffnet wurden - eine lose .bsp, .ent oder ein Skript ohne Rückfrage, keine Kopie unter Dokumente, keine neue .pk3. Neue Befehle "Entities speichern" und "Entities in die BSP übernehmen" (Byte für Byte wie -onlyents des Karten-Editors, Sicherung .bsp.bak). Eine Datei aus einer .pk3 fragt: in der .pk3 ändern (nur dieser Eintrag, Sicherung .pk3.bak, Meldung, wenn das Spiel sie offen hat), lose Datei im Mod-Ordner, Speichern unter oder der bisherige Missionsordner-Weg; die Wahl lässt sich merken. "Save mission" speichert an Ort und Stelle, "Als .pk3 packen..." ist ein eigener Befehl; Skriptvorlage und bisheriger Ablauf sind Einstellungen.
- Speichern: Jede Datei wird über eine Temp-Datei geschrieben und erst fertig umbenannt; bei einem Fehler bleibt die alte Datei, und es erscheint kein "gespeichert". Das Packen einer Mission nimmt jetzt auch nur per run gestartete Skripte mit, schreibt nie eine leere .ent, lässt mission.txt und alte .pk3 weg und weist Pfade mit ".." und Entities über den Grenzen des Spiels (64 Schlüssel, 2048 Zeichen) ab. Neuladen derselben Karte fragt vor dem Verwerfen von Entity-Änderungen; .ent öffnen leert Rückgängig, Auswahl und Skriptliste.

## 中文
- NPC 路径点:删除实体时,所有指向它的 target 到 targetJump 会在同一个撤销步骤中一并移除;对于路径点以外的实体,程序会先询问。"删除时填补空缺"会连接被删路径点的相邻点。
- NPC 路径点:你放置、移动或复制的路径点会按游戏自身的连线检查(NAV::TestEdge,Jedi Outcast 中为 HardConnect)与相邻点相连;移动后游戏会丢弃的连接将被移除。新增"自动连接"开关和"连接到邻近点"按钮。
- NPC 路径点:Ctrl+D 和右键菜单可复制路径点并赋予空闲名称,副本之间保持相连;所选路径点可按模式重命名。视图的右键菜单现在有复制、删除和连接到邻近点。
- NPC 路径点:生成也会从地图上已有的路径点开始——在遗漏的房间里放一个路径点,该房间就会得到路网。
- 保存:文件保存在打开它的位置——散装的 .bsp、.ent 或脚本不经询问直接保存,不会在“文档”下复制,也不会生成新的 .pk3。新增命令“保存实体”和“把实体写入 BSP”(与地图编辑器的 -onlyents 逐字节相同,备份 .bsp.bak)。来自 .pk3 的文件会询问:在 .pk3 中修改(只改这个条目,备份 .pk3.bak,游戏打开着时会提示)、模组文件夹中的散装文件、另存为,或以前的任务文件夹方式;可以记住选择。“Save mission”就地保存,“打包为 .pk3...”是单独的命令;脚本模板和以前的流程是设置项。
- 保存:每个文件先写入临时文件,完成后才重命名;出错时旧文件保留,也不会显示“已保存”。打包任务时现在会包含仅通过 run 启动的脚本,不会再写出空的 .ent,不再包含 mission.txt 和旧 .pk3,并拒绝含 “..” 的路径以及超出游戏上限(64 个键、2048 个字符)的实体。重新加载同一地图前会询问是否放弃实体修改;打开 .ent 会清空撤销、选择和脚本列表。

## 日本語
- NPC ウェイポイント: エンティティを削除すると、それを指していた target から targetJump も同じ元に戻す 1 回で消えます。ウェイポイント以外のエンティティでは先に確認します。「削除時に隙間を埋める」で削除したウェイポイントの隣どうしを接続します。
- NPC ウェイポイント: 置いた・移動した・複製したウェイポイントは、ゲーム自身の辺チェック(NAV::TestEdge、Jedi Outcast では HardConnect)で近くの点と接続します。移動後にゲームが捨てる接続は削除します。「自動で接続」スイッチと「近くの点と接続」ボタンを追加。
- NPC ウェイポイント: Ctrl+D と右クリックメニューでウェイポイントを空いている名前で複製し、コピーどうしの接続は保たれます。選択をパターンで名前変更できます。ビューの右クリックメニューに複製・削除・近くの点と接続を追加。
- NPC ウェイポイント: 生成はマップにすでにあるウェイポイントからも始まります。抜けた部屋に 1 つ置けば、その部屋にもネットワークができます。
- 保存: ファイルは開いた場所に保存されます。単体の .bsp、.ent、スクリプトは確認なしで保存し、「ドキュメント」へのコピーも新しい .pk3 もできません。新しいコマンド「エンティティを保存」と「エンティティを BSP に書き込む」(マップエディターの -onlyents とバイト単位で同じ、バックアップ .bsp.bak)。.pk3 から開いたファイルは保存先を尋ねます: .pk3 の中で変更 (このエントリーだけ、バックアップ .pk3.bak、ゲームが開いているときはメッセージ)、Mod フォルダーに単体ファイルとして、名前を付けて保存、従来のミッションフォルダー方式。選択は記憶できます。「Save mission」はその場で保存し、「.pk3 にまとめる...」は別のコマンドです。スクリプトのテンプレートと従来の方式は設定で選べます。
- 保存: どのファイルも一時ファイルを経由して書き、完成してから名前を変えます。エラー時は元のファイルが残り、「保存しました」も出ません。ミッションのパックは run だけで始まるスクリプトも含め、空の .ent を書かず、mission.txt と古い .pk3 を入れず、".." を含むパスとゲームの上限 (キー 64、2048 文字) を超えるエンティティを拒否します。同じマップを読み込み直す前にエンティティの変更を破棄するか確認し、.ent を開くと元に戻す・選択・スクリプト一覧をリセットします。

## Español
- Puntos de ruta NPC: al borrar una entidad desaparece cada target a targetJump que la nombraba, en el mismo paso de deshacer; para entidades que no son puntos de ruta el programa pregunta antes. "Cerrar el hueco al borrar" conecta los vecinos de un punto borrado.
- Puntos de ruta NPC: un punto que colocas, mueves o duplicas se conecta con sus vecinos según la propia prueba de aristas del juego (NAV::TestEdge, HardConnect en Jedi Outcast); tras moverlo se quitan las conexiones que el juego descartaría. Interruptor "Conectar automáticamente" y botón "Conectar con los vecinos".
- Puntos de ruta NPC: Ctrl+D y el menú del botón derecho duplican puntos con nombres libres, las copias siguen conectadas entre sí; la selección se puede renombrar con un patrón. El menú del botón derecho de la vista tiene ahora Duplicar, Borrar y Conectar con los vecinos.
- Puntos de ruta NPC: la generación también empieza en los puntos que ya están en el mapa; una sala omitida recibe su red cuando colocas allí un punto.
- Guardar: los archivos se guardan donde se abrieron; un .bsp, .ent o script suelto sin preguntar, sin copia en Documentos ni .pk3 nuevo. Comandos nuevos "Guardar entidades" y "Escribir las entidades en el BSP" (byte a byte como -onlyents del editor de mapas, copia .bsp.bak). Un archivo de un .pk3 pregunta: cambiarlo dentro del .pk3 (solo esta entrada, copia .pk3.bak, aviso si el juego lo tiene abierto), archivo suelto en la carpeta del mod, guardar como o la forma anterior con carpeta de misión; la elección se puede recordar. "Save mission" guarda en su sitio, "Empaquetar como .pk3..." es un comando aparte; la plantilla de scripts y el flujo anterior son opciones.
- Guardar: cada archivo se escribe a través de un archivo temporal y se renombra solo al terminar; si hay un error, el archivo anterior se queda y no aparece "guardado". Empaquetar una misión incluye ahora los scripts que solo se inician con run, nunca escribe un .ent vacío, deja fuera mission.txt y los .pk3 antiguos y rechaza rutas con ".." y entidades que superan los límites del juego (64 claves, 2048 caracteres). Recargar el mismo mapa pregunta antes de descartar cambios de entidades; abrir un .ent vacía deshacer, selección y la lista de scripts.

## Čeština
- Body trasy NPC: při mazání entity zmizí každé target až targetJump, které ji jmenovalo, v tomtéž kroku zpět; u jiných entit než bodů trasy se program nejdřív zeptá. "Při mazání uzavřít mezeru" spojí sousedy smazaného bodu.
- Body trasy NPC: bod, který položíš, přesuneš nebo zduplikuješ, se spojí se sousedy podle vlastního testu hran hry (NAV::TestEdge, v Jedi Outcast HardConnect); po přesunu zmizí spojení, která by hra zahodila. Přepínač "Automaticky spojovat" a tlačítko "Spojit se sousedy".
- Body trasy NPC: Ctrl+D a nabídka pravého tlačítka duplikují body s volnými jmény, kopie zůstanou navzájem spojené; výběr lze přejmenovat podle vzoru. Nabídka pravého tlačítka v pohledu má nyní Duplikovat, Smazat a Spojit se sousedy.
- Body trasy NPC: generování začíná i u bodů, které už na mapě jsou; vynechaná místnost dostane síť, když do ní položíš jeden bod.
- Ukládání: soubory se ukládají tam, odkud byly otevřeny - volný .bsp, .ent nebo skript bez dotazu, žádná kopie v Dokumentech, žádný nový .pk3. Nové příkazy "Uložit entity" a "Zapsat entity do BSP" (bajt po bajtu jako -onlyents editoru map, záloha .bsp.bak). Soubor z .pk3 se zeptá: změnit v .pk3 (jen tuto položku, záloha .pk3.bak, zpráva, když ho má hra otevřený), volný soubor ve složce modu, uložit jako, nebo dosavadní cesta přes složku mise; volbu lze zapamatovat. "Save mission" ukládá na místě, "Zabalit jako .pk3..." je samostatný příkaz; šablona skriptu a dosavadní postup jsou volby.
- Ukládání: každý soubor se zapisuje přes dočasný soubor a přejmenuje se, až je hotový; při chybě zůstane starý soubor a zpráva o uložení se neobjeví. Balení mise nyní zahrne i skripty spouštěné jen přes run, nikdy nezapíše prázdný .ent, vynechá mission.txt a staré .pk3 a odmítne cesty s ".." a entity nad limity hry (64 klíčů, 2048 znaků). Opětovné načtení téže mapy se před zahozením změn entit zeptá; otevření .ent vyprázdní zpět, výběr a seznam skriptů.
