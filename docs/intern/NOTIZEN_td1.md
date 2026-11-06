# Notizen TD1: Selbsttests an den neuen Stand der Movie-Duels-Daten (05.11.)

Auftrag `auftrag_testdaten_md.md` (im Auftrag R3). Direkt in `Y:\karte` auf `karte1`, Build `build-test`, Laeufe `laeufe/td1_*` (rot mit `laeufe/r3_exe_rot` = d93fad798 plus R3-Zaehler: `td1_me_rot`, `td1_gl_rot`/`td1_gl_foto`, `td1_t6_rot`; Hinweis rot `td1_m7_rot`; gruen `td1_m7`, `td1_satz`, `td1_mps*`, `td1_t6b`, `td1_gl2`, `td1_karte`). Kein Spiel gestartet, Spielordner nur gelesen; die MD-MP-Archive nicht wiederhergestellt.

## Grundlage

- `uebersprungen (Daten fehlen)`: neu `meldeUebersprungen`, `pruefeMitDaten`, `spielDateiFehlt` (gui/selbsttest.cpp). Eine Pruefung, deren feste Spieldaten fehlen, schreibt `Knopftest uebersprungen (Daten fehlen): <Pruefung> - <Grund>` und zaehlt weder OK noch FEHLER; am Ende `Knopftest: N Pruefungen uebersprungen (Daten fehlen)` vor `Knopftest fertig`.

## 1. Menue-Editor Movie Duels MP

- Rot (`td1_me_rot`, voller Lauf 2358 OK / 3 FEHLER): M7 MP-Startmenue (Meldung "global defines ui/MD_MP/menudef.h not found"), rc608 MP-Schriften (font 2/3 nicht aus dem MD-Satz), 3b Satz MP (kein ui/MD_MP/main.menu fuer das Testpaket).
- Jetzt: fehlt `ui/MD_MP/menudef.h` in den Spieldaten, meldet jede der drei "uebersprungen (Daten fehlen)" fuer den Satz des Spiels und laeuft dieselbe Pruefung mit einem eigenen kleinen MP-Menuesatz aus dem Laufordner (`td1_mdmp/zz_td1_mdmp.pk3` als Spielordner der Sitzung: `ui/MD_MP/menudef.h`, `ui/MD_MP/main.menu` mit assetGlobalDef font anewhope / bigFont ergoec, `ui/MovieDuels-MP-Menus.txt`). Mit installiertem Satz laufen sie wie bisher gegen das Spiel.
- Programm: fuer ein MD-MP-Menue ohne MD-MP-Menuesatz stand nur "global defines ... not found" in den Meldungen, die Vorschau blieb leer (MP-Texte ohne die Schriften des Satzes). Neu der Hinweis `MeMdMpFehlt` oben in den Meldungen und oben in der Vorschau (`mdMpSatzFehlt`, je Stand der Spieldaten einmal geprueft). Rot `td1_m7_rot` (Hinweis im Bild -1), gruen `td1_m7` 13/0 (Hinweis da; mit dem eigenen Satz weg).

## 2. gluehen / karte: rotes Auge von sbd1

- Rot `td1_gl_rot` 8/1 (0 rote Punkte), Foto `td1_gl_foto`: die feste Kamera (28 vor dem Fuss) stand im neuen, groesseren SBD.
- Aus den Daten: `shaders/sbd.shader` (MD_Models_PT.pk3) traegt in body und body_cortosis weiter `{ map models/players/SBD/body_e blendFunc GL_ONE GL_ONE glow rgbGen identity }`, `Body_E.jpg` ist schwarz bis auf das rote Auge (84 Bildpunkte bei 181..192, 271..273). Die Erwartung bleibt also (rotes Auge, Leuchtstufe im Gluehen); gesucht wird die Blickrichtung (8 Richtungen, drei Hoehen, 40 Einheiten), in der die Stufen der Figur die meisten kraeftig roten Punkte hinzufuegen (Bild mit gegen ohne Stufen, `gpu::figurStufenAus`).
- Gruen `td1_gl2` 9/0 (40 rote Punkte, Richtung 18), `td1_karte` 233/0.

## 3. teil6 Wetter

- Rot `td1_t6_rot` 13/2: Weather/duel_mustafar.cfg und duel_carbon.cfg sagen jetzt "r_weather 0" (die Vorschau zeigt richtig keine Wolke).
- Jetzt mit Karten, deren .cfg Wetter setzt (aus den Daten gesucht): duel_mus_both2 "r_weather 3" (MD_Maps_Ep3.pk3; MD9 tr_WorldEffects.cpp:2274-2286 der Lava-Sturm, snowflake2) und duel_ahs_wbw "r_weather 6" (MD_Maps_Canonical.pk3; "fog", alpha_smoke2b). Dazu zaehlt die Pruefung, was fx_lava der Karte schickt (MD9 g_fx.cpp:318-345, je Befehl einmal): duel_mus_both2 hat ein fx_lava ohne spawnflags -> "lava" -> zweite Wolke snowflake2, wie im Spiel. Fehlt Karte oder .cfg oder sagt die .cfg etwas anderes: uebersprungen (Daten fehlen).
- Gruen `td1_t6b` 15/0 (2 erwartet 2; 1 erwartet 1).

## 4. Liste: Pruefungen, die jetzt bei fehlenden MD-Daten "uebersprungen (Daten fehlen)" melden

- menueeditor: "M7 MP-Startmenue", "rc608 MP-Schriften", "3b Satz: MP" - je eine Zeile uebersprungen fuer den Satz des Spiels, dieselbe Pruefung mit dem eigenen Satz (heute so: ui/MD_MP fehlt).
- menueeditor: "TD1 MD MP: ohne MD-MP-Menuesatz zeigt der Editor ... einen Hinweis" - uebersprungen, wenn der Satz installiert ist oder kein Movie Duels; "TD1 MD MP: mit dem eigenen MD-MP-Menuesatz kein Hinweis mehr" - uebersprungen ohne eigenen Satz.
- teil6: "Weather/duel_mus_both2.cfg gilt unter Movie Duels" und "Weather/duel_ahs_wbw.cfg gilt unter Movie Duels" - uebersprungen, wenn Karte/.cfg fehlen oder die .cfg nicht r_weather 3 / 6 sagt (heute beide geprueft).
- Nicht umgestellt: die uebrigen Pruefungen mit festen MD-Dateien laufen mit den Daten vom 05.11. gruen (voller Lauf menueeditor rot nur in den drei oben); sie nennen weiter FEHLER, wenn ihre Datei verschwindet.

## Changelog

### English
- Menu editor: for a Movie Duels multiplayer menu without the Movie Duels multiplayer menu set (ui/MD_MP) in the game data, a hint says so in the messages and above the preview instead of an empty preview.

### Deutsch
- Menü-Editor: für ein Mehrspieler-Menü von Movie Duels ohne den Mehrspieler-Menüsatz (ui/MD_MP) in den Spieldaten sagt ein Hinweis das in den Meldungen und über der Vorschau, statt einer leeren Vorschau.

### 中文
- 菜单编辑器:如果游戏数据中没有 Movie Duels 多人菜单集(ui/MD_MP),编辑 Movie Duels 多人菜单时会在消息和预览上方显示提示,而不是空白预览。

### 日本語
- メニューエディター: ゲームデータに Movie Duels のマルチプレイ用メニューセット (ui/MD_MP) がない場合、Movie Duels のマルチプレイメニューでは空のプレビューの代わりにメッセージとプレビューの上にその旨のヒントが表示されます。

### Español
- Editor de menús: para un menú multijugador de Movie Duels sin el conjunto de menús multijugador (ui/MD_MP) en los datos del juego, un aviso lo indica en los mensajes y sobre la vista previa en lugar de una vista previa vacía.

### Čeština
- Editor menu: u menu Movie Duels pro více hráčů bez sady menu pro více hráčů (ui/MD_MP) v herních datech to oznámí upozornění ve zprávách a nad náhledem místo prázdného náhledu.
