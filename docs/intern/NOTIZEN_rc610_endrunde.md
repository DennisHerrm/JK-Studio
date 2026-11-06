# Notizen rc610 Endrunde: volle Laeufe, Version rc610

Auftrag `auftrag_rc610_endrunde.md`. Direkt in `Y:\karte` auf `karte1` ab bd269a46 (R2b), Build `build-test`, Laeufe `laeufe/e10_*` (Pool 12 parallel, Programm `laeufe/e10_exe`; nach dem Testfix `laeufe/e10_exe2`, Einzellaeufe `e10a_*`, Gegenproben mit alten Staenden `e10b_*`, `e10x_*`). Laufskript `lauf.py` wie R2b. Kein Spiel gestartet.

## Version und Changelog
- `kFassung` rc610; Changelog aller rc610-Teile (r1, f23, f46, f78, f4b, f6b, licht1, tut_geo, modobj, r2a, r2b) in 6 Sprachen nach Bereichen in `tools/changelog.txt` (Echtzeit-Licht, Modelle aus anderen Programmen, Karten-Editor, Missions-Vorschau, Skriptvorschau, NPC-Wegpunkte, Menue-Editor, Updates/Einstellungen/Befehlszeile, Handbuch), dazu der Hinweis "Windows 10 vor Version 1903 wird nicht mehr unterstuetzt" (shank 05.11.); `gen_changelog.py`, `AENDERUNGEN.md`.
- Vorsilben der Teile ("Karten-Editor, ...:") stehen jetzt als Bereich; MODOBJ nannte "Insert into finished map", die Oberflaeche heisst "Insert into compiled map" (en, ja angeglichen); F4B-Deutsch ohne Umlaute nachgezogen.

## Laeufe
- Sauberer Neubau (`--clean-first`): 0 Warnungen, 0 Fehler. ctest 220 von 220.
- Lints: lint_portability 99 (wie R2b), lint_pruefcodes 11, lint_fremdprogramm 1, lint_efxflags "keine Flagtabelle" (alle wie Basis); alle anderen ok (headers, msvclink ok; link und tidy brauchen die Linux-Werkzeuge). Handbuch `--pruefen` ok (es/cs 0 englisch); die 60 seit rc609 neuen oder geaenderten Bilder alle >= 1920 breit.
- Selbsttests (127 Laeufe, 12 parallel): Modus 1 en/es/cs je 2651/0, mp 2651/0, sp 2651/0, menueeditor JKA (`nur_ja.cfg`) 2781/0, mission1 (allein) 98/0, allekarten 283/0 (rc609: 1 FEHLER Speicher, seit R1 gruen), ablauf 14/0 und umriss 49/0 (rc608-Altlasten, seit R1/R2a gruen), wegerzeugung taspir1/jeditemple_garden/t1_danger je 10/0 (.ent de3d00ea, 8c603101, 0f9c13e7 wie R2b), licht1 15/0, modobj 25/0, r2a 21/0, ruhen 32/0, tutgeo en/es/cs 23/0, hilfe1/hilfe2 en/es/cs, editorportal 19/0, editorportal tester 4/0, teil13c 6/0, f78 1/0; update (drei Faelle aus einer Kopie) 4/0, 4/0 und "ganz neu", umzug mit gesperrter Datei 6/0.
- `karteneditor` voll (Endfassung `e10_exe4`, je allein wegen der festen Ports 39000-39063): en, es, cs je 3327/0. Parallel waren nur die Port-Pruefungen rot (3 bis 64 je Lauf) und Stufe 7 p99 (Bildzeit).
- Nach dem Beenden-Fix (`e10_exe3`): ctest 220/220, Modus 1 2651/0, mission1 98/0, missionsbau 95/0, r2a 22/0; `menueeditor` changelog (`BHED_MENUE_NUR=changelog`) 6/0 mit dem neuen Changelog.
- Unter Last rot, allein gruen: mission1 "Gizmo: Kamera" (97/1 -> 98/0), menueeditor es M6 Fluss (allein `BHED_MENUE_NUR=m6` 17/0), teil11 Lightmap-Verhaeltnis (2/1 -> 3/0).
- Wie rc609, kein Fehler: einzel 0/6 (braucht `BHED_EINZEL`), kartenpruefung/handbuch/icons/vergleich ohne eigene Pruefungen.

## Behoben
- Selbsttest `karteneditor` Stufe 2 (Modell-Browser): Absturz 0xC0000005 nach 222 s in en/es/cs. Rot nachgestellt allein (`e10x_stufe2`, `BHED_KARTE_NUR=stufe2`): "bespin geladen" 0 Modelle, danach liest die naechste Pruefung `d[0]` eines leeren Ordners. Ursache: im Movie-Duels-Spielordner hat `models/map_objects/bespin` seit 05.11. einen Unterordner (`impholo`, holo.md3/.glm in MD_Assets.pk3); der Doppelklick auf die Zeilenmitte traf in der 100 Punkte schmalen Baumspalte das Dreieck und klappte zweimal (wie Qt), statt den Ordner zu laden. Der Stand von F6b (`li1_exe_rot`, am 05.11. 18:15 gruen) und MODOBJ-Rot sind heute genauso rot - Spieldaten, nicht Programm. Behoben im Test: Doppelklick auf den Namen, keine Pruefung liest `d[0]` eines leeren Ordners. Gruen: `e10a_stufe2` 376/0.

- Beenden (Programm, rc610 R2a): `karteneditor` voll en/es/cs "R2a Schliessen: das Kreuz fragt nach der ungespeicherten Karte" (confirmQuit 1, keine Frage), allein `r2a` gruen. Ursache: die Antwort auf die Kartenfrage (`beendenBeantwortet`, ebenso `entFrageBeantwortet` der Missions-Entities) blieb stehen, wenn das Beenden danach bei "Exit?", einem Skript oder der Missionsfrage abgebrochen wurde - danach fragte das Kreuz nie wieder nach der Karte (im vollen Lauf ueber Stufe 4 File > Exit). Rot nachgestellt in `r2a` (neue Pruefung "nach abgebrochenem Beenden fragt das Kreuz wieder nach der geaenderten Karte", `e10r_r2a` 19/5). Behoben: `beendenAbgebrochen()` (app_ablage.inc) in jedem Abbruch-Weg (Skriptfrage, "Exit?" Nein, Missionsfrage) setzt Kartenfrage, Entities-Frage, Reiterkette und "Neu starten" zurueck; der Testhelfer von Stufe 4 bricht ueber denselben Weg ab. Gruen: `r2a` 22/0, Modus 1 2651/0, mission1 98/0, missionsbau 95/0.
- Selbsttest MODOBJ im vollen Lauf (11 FEHLER, allein gruen): Stufe 2 hatte models/ und models/map_objects/ aufgeklappt; die Ordner von map_objects schoben modobj/ aus dem Bild. Behoben im Test: models/ nur aufklappen, wenn zu, map_objects/ zuklappen, wenn offen. Gruen allein `e10g_modobj` 25/0 und im vollen Lauf (oben).

## Spieldaten (seit 05.11. 23:18 geaendert, nicht der Code - nur aufgeschrieben)
- menueeditor Movie Duels (en, es, cs): die drei bekannten Fehler M7 MP-Startmenue, rc608 MP-Schriften, 3b MP-Testpaket (MP-Archive fehlen im Spielordner).
- `gluehen` und `karte` "Gluehen Figur: die Leuchtstufe (rotes Auge von sbd1) liegt im Bild": SBD ist jetzt ein anderes Modell (Skelett `_humanoid_sbd`, 260 statt 1837 Abschnitte, keine roten Bildpunkte am Kopf); mit `li1_exe_rot` und `r2b_exe_vor` gleich rot.
- `teil6` zwei Wetterpruefungen (duel_mustafar r_weather 3, Endrunde-4-Nebel): `weather/duel_mustafar.cfg` in den Spieldaten sagt jetzt `r_weather 0`, die Vorschau zeigt richtig 0 Wolken; mit alten Staenden gleich rot.

## Dem Nutzer nennen (rc610, aus allen Notizen rc610)
- Nichts im Spiel gemessen (in rc610 kein Spielstart): Tueren, Mover, Teams, ICARUS, Menue-Vorschau, Geschuetze (B4-2), Echtzeit-Licht und umgewandelte Modelle sind gegen die Quellen (OpenJK, Movie Duels Update 9, q3map2/NetRadiant-Custom) und das gebackene Licht von bhed_q3map2 belegt.
- Windows 10 vor Version 1903 wird nicht mehr unterstuetzt; die Umstellung der Dateipfade (B8-10, rund 580 Stellen in 116 Dateien) kommt nach dem Release.
- Spieldaten Movie Duels seit 05.11. abends geaendert: drei Menue-Editor-Pruefungen (MP-Archive fehlen), SBD-Modell, Wetter von duel_mustafar - die Selbsttests dazu sind rot, bis die Daten wieder da sind oder die Pruefungen angepasst werden (Entscheidung Hauptchat/shank).
- Echtzeit-Licht: ueber dem Grafikspeicher-Budget kleinere Schattenwuerfel und weichere Schatten als rc609 fuer die 32 staerksten Lichter; Sichtbarkeit ueber vis nicht genutzt; Halme auf misc_model_static fehlen in der Echtzeit-Ansicht (das Spiel zeichnet sie), Halme an BSP-Schnitten an anderen Stellen als im Spiel, Wind von fx_wind nur in der gebauten Karte.
- Modelle: lange Dateinamen ueberlappen in den Zellen des Modell-Browsers (wie NRC); dass die umgewandelte .md3 im Spiel wie die eingebackene .obj aussieht, ist nur ueber q3map2 und das Format belegt.
- Mover: opentarget/closetarget eines Team-Slaves feuern im Spiel je Teil, die Vorschau nur die des Meisters; MOVER_GOODIE (Inventar) nicht nachgebaut; eine func_bobbing-Umkehr mitten in der Bahn rechnet die Engine mit NaN, die Vorschau faehrt die volle Bahn zurueck; G_PickTarget/target_random nur ueber Karten-Entities.
- Menue-Editor: Ownerdraws 340-350 (Movie-Duels-MP-Kraftraenge), `disabled`, Listbox-/Textscroll-mouseEnter nicht nachgebaut; MD-MP-Feeder nicht Zeile fuer Zeile gelesen; Jedi Outcast MP ohne eigene Quelle wie JKA MP.
- Speicher: nach allen Karten bleiben die nach Pfad abgelegten Klingen- und gfx-Bilder (6 Eintraege) und die .gla-Skelette (wie im Spiel ueber r_modelpoolmegs).
- Updater: die Luecke zwischen zwei Umbenennungen beim Programm (B8-4) bleibt; B7-3 nicht nachstellbar.
- Tutorial "Ein Modell zu Brushes machen": Schritt 6 (bauen, im Spiel ansehen) nur beschrieben; Clip-Brushes zeigen "SHADER IMAGE MISSING" (kein Editorbild in den Spieldaten, wie NRC).

## Changelog

### English
- Version rc610 with the changelog of all rc610 parts.
- Map editor: After quitting was cancelled at a later question ("Exit?", an unsaved script or mission), closing the program asks about a changed map again; before, the first answer counted for good.

### Deutsch
- Version rc610 mit dem Changelog aller rc610-Teile.
- Karten-Editor: Wurde das Beenden bei einer späteren Frage abgebrochen („Exit?“, ein ungespeichertes Skript oder eine Mission), fragt das Schließen wieder nach einer geänderten Karte; vorher galt die erste Antwort für immer.

### 中文
- 版本 rc610,包含所有 rc610 部分的更新日志。
- 地图编辑器:在后续问题(“Exit?”、未保存的脚本或任务)中取消退出后,再次关闭程序时会重新询问已更改的地图;以前第一次的回答会一直有效。

### 日本語
- バージョン rc610。rc610 の全パートの変更履歴を収録。
- マップエディター: 後の質問 (「Exit?」、未保存のスクリプトやミッション) で終了を取り消した後、プログラムを閉じると変更したマップについて再び確認します。以前は最初の答えがずっと有効でした。

### Español
- Versión rc610 con el registro de cambios de todas las partes de rc610.
- Editor de mapas: si se cancela la salida en una pregunta posterior («Exit?», un script o una misión sin guardar), al cerrar el programa se vuelve a preguntar por un mapa modificado; antes valía para siempre la primera respuesta.

### Čeština
- Verze rc610 se seznamem změn všech částí rc610.
- Editor map: když se ukončení zruší u pozdější otázky („Exit?“, neuložený skript nebo mise), zavření programu se znovu zeptá na změněnou mapu; dříve platila první odpověď navždy.
