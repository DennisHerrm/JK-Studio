# Notizen zeitleiste1: neue Zeitleiste im Mission-Tab

Zweig `zeitleiste1` ab zeitleiste0 (23ffa48b), Worktree `Y:\mission`, Bau
`Y:/mission/build-mission`, Texte `tools/i18n_zeitleiste1.py`. Auftrag:
`<SP>\auftrag_zeitleiste1.md` + `<SP>\auftrag_gemeinsam.md`, Vorlage
`<SP>\zeitleiste_entwurf\entwurf_freigegeben.html`. Korrektur von shank (nach
der Pause am 29.10.): der obere Teil des Entwurfs (Kamerabild, Draufsicht,
"Was passiert hier?") wird nicht als eigene Fenster gebaut - uebernommen wird
die Zeitleiste; die Erklaerung kommt in die rechte Seitenleiste
(Script-Flow-Spalte), die Vorschau ist die vorhandene 3D-Ansicht, Punkt 9
(Draufsicht) entfaellt.

## Aufbau

- **Modell** `include/bhed/spuren.h`, `src/spuren.cpp` (ohne ImGui): baut aus
  Ablauf, Kamerabahn, Szene und flacher Zeitleiste je Zeile die Bloecke mit
  echten Dauern (Satz = Klanglaenge, Lauf bis Ankunft, Animation bis Ende der
  Haltezeit, Kamera = letztes Argument), Unterbahnen, Knickpunkte fuer
  Sichtfeld/Blende, "springt zurueck", Ziehbarkeit, "nicht nachgebildet".
  Gebaut in `prepareScene` (`zeitleisteModellBauen`). Klanglaenge, Untertitel
  (StringEd-Leser `menue::Texte::ladeStr`, Schluessel wie CG_CaptionText) und
  Musik hinter einem use kommen ueber Rueckrufe.
- **Zeichnen** `gui/app_zeitleiste.cpp`: Zeilen, Bloecke, Marken (Beschriftung
  nur, wo sie nichts ueberdeckt), Filmstreifen, Navigationsleiste, Ziehen,
  Erklaerung. `drawTimeline` behaelt Transportleiste, Abspielen und Lineal.
  `kategorieVon` (Textsuche) ist weg; die Feldansicht "Spuren"
  (`spurenBauen`) liest dasselbe Modell (Dialog jetzt mit echter Klanglaenge
  statt fest 2,5 s).
- **Erklaerung** unten in der Script-Flow-Spalte (`drawFlowArea`), so hoch wie
  ihr Inhalt (hoechstens 75 %); Skriptzeile und "Skriptzeile oeffnen" (Zeile
  waehlen + Bearbeiten-Dialog) direkt unter Titel und Zeit.

## Ergebnisse je Punkt

1. **Gemeinsames Modell** - ctest `spuren` (eigene Mini-Szene: Karte mit
   NPC_spawner, waypoint_navgoal, zwei ref_tag, target_play_music,
   target_level_change; Skript mit jeder Befehlsart): gruen. Erwartungswerte
   aus den Regeln der Engine (wait +50 ms, CAMERA_DEFAULT_FOV 90, lineare
   Zoom-/Blendenrampe, Klanglaenge, dowait auf CHAN_VOICE, Signal-Sender,
   Kartenwechsel, print/camera TRACK nicht nachgebildet, Schnitt bricht eine
   Fahrt ab). Gegenprobe mit dem Export zeitleiste0 im Selbsttest (unten).
2. **Uebersicht/Detail** wie im Entwurf: Kamerabilder, Kamera - Ort
   (Einstellung je MOVE bis zum naechsten, Name der Markierung bzw. "(x, y,
   z)"), Kamera - Drehung (Schnitt markiert), Sprache, Musik, Skriptablauf,
   Darsteller (oben Animationen ohne STAND/IDLE/BOTH_SITn, unten "laeuft n" /
   "rennt n", gestrichelt "steht still", schraffiert unsichtbar),
   aufklappbar (Animation, Befehle). Detail: Kamerafahrt, Schwenk, Zoom,
   Blende (+ Rollen, Wackeln, Folgen, Kamerabahn, wenn vorhanden), Sichtfeld
   und Blende als lineare Linien. affect-Ziele ohne Figur bekommen trotzdem
   eine Zeile.
3. **Lesbarkeit**: Zeilen- und Blockhoehen aus dem Entwurf im Massstab
   Schrift/14 (kleinster Block 25,7 px bei Schrift 24); der Selbsttest prueft
   Mindesthoehe und keine ueberlappenden Beschriftungen (academy1 und
   md_am_sith in beiden Ansichten, Mini-Szene). Ueberlappungen als Unterbahn
   mit "springt zurueck" und Erklaerung aus cg_camera.cpp; abgeschnittene
   Namen haben einen Tooltip.
4. **Sprache** mit Untertitel, Sprecher, Wellenform (Spitze je 50 ms);
   **Musik** (Levelmusik und target_play_music ueber Ablauf::benutzt);
   **Skriptablauf** mit schraffierten, beschrifteten Wartezeiten ("wartet auf
   ‚x‘", Sender und Zeit in der Erklaerung), Kartenwechsel-Marke.
5. **Filmstreifen**: alle 2 s ein Bild der Skriptkamera (sichtFovX; Blende
   und Kinobalken aus CameraState darueber). Aufgenommen in einem Durchgang,
   je ruhigem Bild der Ansicht eines: drawMapView zeichnet dann nur das
   Filmbild (224 px breit, Zielsatz 4 - kZielSaetze 4 -> 5) zur Zeit des
   Filmbilds und stellt die Zeit des Zeigers gleich danach zurueck; Ansicht,
   Vorschau und Ueberlagerung bleiben wie im Bild davor. "Ruhig" heisst: die
   Ansicht haette sonst nichts zu zeichnen (mapDirty aus), keine Maus, kein
   Abspielen, und in der Ansicht ist nichts gewaehlt - jede Aenderung zeichnet
   zuerst die Ansicht neu, der Durchgang wartet. Laeuft nach jedem Modellbau
   neu. Messung (unter Last): academy1 45 Bilder im Mittel 8,55 ms gegen
   7,73 ms ohne Durchgang, md_am_sith 31 Bilder 30,6 gegen 30,0 ms - keine
   laengeren Bilder. **Fund (mission1)**: in der ersten Fassung verdraengten
   die Filmbilder normale Bilder und verstellten die Zeit des Zeigers fuer
   alle; im Selbsttest mission1 gingen Zuege an Figur-Griffen ins Leere
   ("Undo Figur ...", Laeufe zle_mission1, zl1m1, zl1m2 rot). Gegenprobe ohne
   Filmdurchgang: gruen (zl1m3). Behoben wie oben beschrieben, mission1 gruen
   (zl1m4: 98 OK, 0 FEHLER).
6. **"Was passiert hier?"** in 4 Sprachen fuer jede Blockart (Einstellung,
   Fahrt, Schwenk, Zoom, Blende, Rollen, Wackeln, Folgen, Kamerabahn, an/aus,
   Satz/Klang, Musik, Warten auf Zeit/Signal/Aufgabe, Signal, use, set,
   Kartenwechsel, nicht nachgebildet, Lauf, Fahrt, Drehen, ROFF, Animation,
   Kamerabild); "Skriptzeile oeffnen" waehlt die Zeile und oeffnet den
   Bearbeiten-Dialog (Selbsttest prueft editorOpen und editorPath).
7. **Navigation**: Anfangsausschnitt 30 s (academy1: 30 000 von 91 550 ms),
   "Alles", -/+ mit Faktor 1,6, Strg+Rad um den Zeiger (die Zeit unter der
   Maus bleibt auf 0,03 ms), Umschalt+Rad rollt, Navigationsleiste nur
   hineingezoomt, Filter "Nur aktive"/"Probleme", Suche, Leertaste, Tab,
   Pos1. Abweichung: die Pfeile bleiben 50 ms / Umschalt 1 s (Entwurf: 1 s /
   5 s) - 50 ms ist das Raster der Spiellogik und steht so in der Tour
   k-neu-timen.
8. **Ziehen**: Kamerabloecke verschieben und an der rechten Kante die Dauer
   aendern (Detail und Drehung; Ort zeigt den Hinweis "in der Ansicht
   Detail"), Saetze verschieben (Laenge bleibt), Wartezeiten an der Kante
   (Rest rueckt nach), Raster 50 ms, Hinweis mit der Skriptaenderung,
   geaenderte Bloecke gestrichelt bis Strg+Z, Laeufe nicht in der Laenge
   (Hinweis). Zurueckschreiben ueber replaceAt/replaceMany/insertBefore
   (Zahlen im Format des alten Werts). **Doppeltes Undo beim
   Alt-Verschieben**: nachgestellt (Lauf zl1c: "Alt-Verschieben: 2
   Rueckgaengig-Schritte", ein Strg+Z nicht bytegleich), behoben mit
   `fasseZusammen` in `verschiebeImAblauf`, gruen (zl1d: 1 Schritt,
   bytegleich). Jeder Zug im Selbsttest: ein Schritt, nach Strg+Z bytegleich.
   **Kante unter einer Marke**: lag eine Marke genau am Ende eines wait (ein
   set danach), gewann zwar die Kante, aber der Block des Zugs war die Marke -
   beim Loslassen wurde nichts geschrieben (Handbuch-Lauf zlhb_de: "Kante von
   wait gepackt" ohne "Kante losgelassen"; nachgestellt im Selbsttest zl1g:
   "wait ( 500 ) jetzt 500, Schritte 0"). Behoben: die Kante merkt sich ihren
   Block (`zlGriffClip`), gruen (zl1h: 600, ein Schritt, bytegleich).
9. **Draufsicht**: entfaellt (shank).
10. **Spiel-Regeln**: die Zeiten kommen aus dem Nachbau (OpenJK/Movie Duels
    umschaltbar wie bisher). Kamera nach cg_camera.cpp: ein MOVE/PAN mit
    Dauer 0 bricht den laufenden Uebergang ab und setzt das Ziel
    (CGCam_Move/CGCam_Pan "if (!duration)") - er springt NICHT zurueck; nur
    ein Befehl mit Dauer, waehrend der vorige derselben Gruppe noch laeuft.
    ROFF auf Entities als Bloecke "ROFF name" in der Darstellerzeile; Befehle,
    die der Nachbau nicht kennt, sowie camera TRACK/DISTANCE und print als
    rote Marke "wird in der Vorschau noch nicht nachgebildet".
11. **Tests**: Kern-Test mit eigener Mini-Szene; Selbsttest `zeitleiste1`
    mit Mini-Szene (eigene Namen, ohne Karte), academy1/intro und
    md_am_sith/intro2_sith; der alte Selbsttest `zeitleiste` auf die neuen
    Zeilen umgestellt.
12. **Aufnehmen**: offen.

## Gegenprobe mit dem Export zeitleiste0

| | academy1/intro | md_am_sith |
|---|---|---|
| Saetze / mit Laenge / mit Untertitel | 15 / 15 / 15 (Export 15/15/15) | 9 / 9 / 5 (Export 9/9/5) |
| Dauer | 91 550 ms (Export 91,55 s) | 62 650 ms (Export 62,65 s) |
| Kamerabilder | 46 (Export 46) | 32 (Export 32) |
| Ueberlappungen Export / Modell | 2 / 0 | 11 / 8 |

Die Ueberlappungen weichen bewusst ab: der Export (und die alte Leiste) zaehlte
jeden Abschnitt, in den der naechste derselben Gruppe hineinbeginnt - auch
wenn der naechste ein Schnitt (Dauer 0) ist. Im Spiel bricht ein Schnitt den
Uebergang ab (CGCam_Move: `info_state &= ~CAMERA_MOVING; CGCam_SetPosition`),
die Kamera springt nicht zurueck. In academy1 waren beide alten roten Stellen
(cam3b -> lukewalk2) solche Schnitte. Kern-Test "ein Schnitt mitten in einer
Fahrt springt nicht zurueck".

## Bildzeit des Mission-Tabs (unter Last durch andere Laeufe)

| | vorher (alte Leiste, 23ffa48b) | nachher |
|---|---|---|
| academy1 Uebersicht | 31,95 / 17,43 ms | 8,27 / 9,29 ms (Zeichnen der Zeilen 0,45 / 0,61 ms) |
| academy1 Detail | - | 9,50 / 9,78 ms (Zeilen 1,2 / 1,4 ms) |
| md_am_sith Uebersicht | 26,77 / 21,90 ms | 23,47 / 27,42 ms (Zeilen 0,52 / 0,63 ms) |
| md_am_sith Detail | - | 22,26 / 32,28 ms (Zeilen 0,79 / 1,05 ms) |

Je zwei Laeufe, Mittel ueber 120 Bilder, Idle-Prioritaet, Leiste 55 % hoch.
"Vorher" mit dem ausgepackten Stand 23ffa48b (nicht im Repo, Messschritt im
Export-Modus zeitleiste0 eingefuegt, danach geloescht), Ansicht wie dort
(ganze Szene, Spuren an). Die Zahlen schwanken stark mit der Last der anderen
Laeufe; academy1 ist mit der neuen Leiste deutlich schneller, md_am_sith im
Rahmen der Schwankung gleich. Das Zeichnen der neuen Zeilen selbst kostet
0,4 bis 1,4 ms je Bild.

## Handbuch

Seite `zeitleiste` neu (inhalt_szene.py, 4 Sprachen) mit den Bildern
h_zeitleiste, h_zl_detail, h_zl_erklaerung, h_zl_probleme, h_zl_ziehen
(`BHED_HB_NUR=zeitleiste1`, md_am_sith, Oberflaeche 1,3, je Sprache ein Lauf,
`tools/handbuch/bilder.py`). Texte auf anderen Seiten nachgezogen (Knoepfe,
kurz, kurz2, kurz3, befehle, schritte, schritte2, ROFF-Hinweis); nicht mehr
benutzte Oberflaechentexte entfernt (TlTracks, TlWaits, TlIdleTracks,
TlKat*, Zoom-Menue ...); TlKamera bleibt (Ortskatalog des Helfers).

## Gegen das Spiel geprueft / nicht geprueft

- Gelesen und nachgebaut: CGCam_Move/CGCam_Pan (Schnitt bricht ab),
  CAMERA_DEFAULT_FOV, lineare Rampen (CGCam_Update/UpdateFade), wait +1 Takt
  (TaskManager), Untertitel-Schluessel (cg_text/cg_servercmds).
- Nicht im laufenden Spiel verglichen (keine Spielstarts in diesem Auftrag).

## Offen

- Aufnehmen, Jedi-Outcast-Untertitel, "Skriptzeile oeffnen" bei fremden
  Skripten: erledigt in Zeitleiste 2 (unten).
- Halte ziehen (Punkt 9, "wenn Zeit bleibt") entfaellt mit der Draufsicht;
  die Griffe der 3D-Ansicht (mission1) bleiben.
- Andere Handbuchbilder, die die alte Leiste zeigen (Touren c_tl_*,
  s_sig_zeit, k_waits ...), entstehen erst im naechsten grossen Bildlauf
  (`BHED_EDITORTEST=handbuch`); die Schritte dafuer sind umgestellt.

## Befehle

- Bauen: `cmake --build Y:/mission/build-mission --config Release --parallel 4 --target behaved`
- Selbsttest: `sh /y/mission_lauf.sh zeitleiste1 <name> 1500` (nur Mini-Szene: `BHED_ZL1_NUR=mini`)
- Handbuchbilder: `sh /y/zl1_hb.sh`; Schlussrunde: `sh /y/zl1_ende.sh`

## Schlussrunde

- Sauberer Neubau (`--clean-first`, alle Ziele): 0 Fehler, 0 Warnungen.
- ctest: 165 von 165 bestanden (neu: `spuren`).
- Selbsttests (`/y/zl1_ende.sh`): zeitleiste1 42/42, zeitleiste 47/47,
  mission1 97/98, umriss 44/45, kamera 95/95, missionsbau 95/95, szenen
  26/26, ton 34/34, npcklasse 12/12, laserfalle 8/8, geschuetzanim 3/3,
  zielstrahl 2/2, effektgrund 1/1, figurdeform 1/1, karte 229/229,
  karteneditor (fotos) 78/78, Modus 1 (mit BHED_HANDBUCH) 2646/2646.
- umriss: "den Laufbalken ziehen" zog an einem Lauf ausserhalb des
  Anfangsausschnitts von 30 s - tlBalken bekam das Rechteck des vorigen
  Blocks. Behoben (nur gezeichnete Laeufe, der Test zeigt die ganze Szene).
  mission1: "Fadenkreuz nach Stop" (0,21 statt 0,79) - im Lauf davor mit
  demselben Stand gruen (zl1m4 98/98). Nachlauf nach der Behebung (inkrementeller Bau, 0 Warnungen): umriss 45/45,
  mission1 98/98, zeitleiste1 42/42.
- lint_sources, lint_fassung (rc606), lint_gapp, lint_imgui_context,
  lint_layout, lint_abstand, lint_doppelanzeige: ok. Handbuch
  `gen_handbuch.py --pruefen`: handbuch_sprachen ok.
- Keine eigenen Restprozesse; Laufordner unter `Y:\mission_laeufe`, der
  ausgepackte Altstand fuer die Messung (`Y:\mission_alt`) ist geloescht;
  Spielordner nur gelesen.

## Zeitleiste 2

Auftrag `<SP>uftrag_zeitleiste2.md` (Rest von Zeitleiste 1) plus Zusatzpunkt 7
(Untertitel im Vorschau-Bild, vom Koordinator nach dem Fund des Pruef-Agenten).
Laeufe unter `Y:\mission_laeufe\zl2*`; Schnelllaeufe mit `BHED_ZL1_NUR=mini`,
`=jk2`, `=untertitel`.

- **Punkt 4 Pfeiltasten** (d9e3ab82): Pfeil 50 ms, Umschalt 1 s, Strg 5 s;
  Legende nennt die Tasten, Tooltip ueber der Legende mit der ganzen Liste;
  Handbuch (Seite zeitleiste, Tour k-neu-timen, 4 Sprachen). Rot zl2p4rot
  (29/3), gruen zl2p4gr (32/0).
- **Punkt 3 fremde Skripte** (25c825b2): Bloecke aus run/use/spawnscript
  tragen `fremdPfad` (CamSegment, TimelineEvent, ActorStep, Clip); Erklaerung
  zeigt "Skript X, Zeile N" und den Befehlstext (Modell-Rueckruf
  fremdesSkript), "Skriptzeile oeffnen" oeffnet den Reiter
  (reiterMitAblaufSkript, auch fuer skriptFuerAblauf) oder das Archiv und
  waehlt die Zeile. Fund dabei: der alte Knopf fand Reiter aus dem Speicher
  nie (reiterMitPfad vergleicht Dateipfade). Rot zl2p3rot (33/3), gruen
  zl2p3gr (35/0); ctest spuren mit fremdem Skript.
- **Punkt 2 JK2-Untertitel** (c0f67536): Kernmodul `bhed/untertitel.h`
  (CG_ParseServerinfo je Spiel: JKA YAVIN1B, MD zusaetzlich die JK2-Karten
  ohne Nachfrage, JK2 Umbenennung nur ohne eigenes Paket, Zusatzpakete nur
  mit; Schluessel wie PlayIcarusSound/CG_CaptionText, '#'-Referenzen,
  FixIllegalChars). Test `untertitel` mit eigener .str/.sp. Selbsttest
  kejim_post/start_level (Original-JK2): rot zl2p2rot (6 Saetze, 0 Text),
  gruen zl2p2gr ("Shhh", "I think I saw someone.", Pakete KEJIM_POST +
  ARTUS_MINE). Abweichung: ladeSp schreibt Referenzen gross, JK2 vergleicht
  den Teil nach dem Paketnamen genau - Originaldateien sind durchweg gross.
- **Punkt 7 Untertitel im Bild** (3fd09a50): PlayIcarusSound (g_subtitles,
  SCF_USE_SUBTITLES, in_camera, Abstand 1200/350, Rundfunk), CG_CaptionText
  (Umbruch bei 640 mit ergoec, Buchstabenzeit = Klanglaenge/strlen),
  CG_DrawCaptionText (je zwei Zeilen, Mitte des unteren Balkens 456, sonst
  0.88*(480-1.5*Hoehe)), SET_CAPTIONTEXTCOLOR (colorTable), MD
  cg_textprintscale "0.5f" mit den Aenderungen vom 2026-10-13 aus mdgit,
  JK2 Breite mit ^n. Schalter "Untertitel" in der Transportleiste (keine /
  in Zwischensequenzen / alle, Vorgabe wie im Spiel: JKA/MD 0, JK2 2).
  RE_Font_DrawString als gemeinsame Funktion `menue::schriftZeichnen`.
  Selbsttest Mini-Szene (New mission zl2unter, eigenes .str, stumme WAV):
  rot zl2p7rot (2/4), gruen zl2p7c (6/0, Balken 84 px, Text darin).
  Abweichungen: die Seiten wechseln genau an captionNextTextTime (das Spiel
  im ersten Bild danach); Klanglaenge aus der Datei statt aus dem
  22-kHz-Puffer (+-1 ms); keine _sharp-Schriftvarianten; ob die installierte
  MD-DLL (27.10.) schon "0.5f" hat, war nicht pruefbar (die Zeichenkette
  fehlt in der DLL) - umgesetzt ist mdgit.
  Testfund: ein Selbsttest, der nur playMs setzt, zeichnet das Bild nicht
  neu (Balken vom alten Stand) - die Schritte setzen jetzt mapDirty.

- **Punkt 1 Aufnehmen**: Knopf "Aufnehmen" in der Transportleiste (nur fuer
  einen NPC, Q3_SetNavGoal kennt nur NPCs), Steuerung W/S/A/D, Umschalt
  geht, Q/E oder rechte Maustaste drehen, Enter fertig, Esc bricht ab;
  Bewegung mit navSpielerPmove (Kollision der Vorschau ohne CONTENTS_BODY -
  die anderen Figuren stehen beim Aufnehmen still), Tempo aus ablaufTempo
  (die .npc-Werte beim Spawnen, sonst 300/90). Steckt der Kasten im Boden,
  wird bis 18 hoeher angefangen. Kern `bhed/aufnahme.h`: Douglas-Peucker mit
  Toleranz (einstellbar, Vorgabe 24), dazu frei fuer den Kasten (NAV::
  ClearPathToPoint mit STEPSIZE) und Gangwechsel; Test `aufnahme`.
  Geschrieben: je Punkt ein ref_tag und `task ( p ) { set ( SET_NAVGOAL, p ); }
  dowait ( p );` (SET_NAVGOAL selbst wartet nicht - so rechnet der Nachbau
  seit rc601, und so stehen Laeufe in den Skripten), Gangart davor; in die
  Folge der Figur zur Zeit des Zeigers (befehlZurZeit), sonst ein neuer
  affect-Block; ein Rueckgaengig-Schritt (aenderung + ersetzeSkript +
  verbindeMitSkriptSchritt). Fund beim Testen: die erste Fassung schrieb
  SET_NAVGOAL hintereinander - die Figur waere nur zum letzten Punkt
  gelaufen. Waehrend der Aufnahme ist die Figur ein Umriss (Kasten,
  Blickpfeil, Spur, nummerierte Laufziele); das Modell bleibt am Ort zur
  Startzeit, nach dem Schreiben laeuft es im Bild wie im Spiel.
  Selbsttest (BHED_ZL1_NUR=aufnahme): Esc aendert nichts, Mini-Szene mit
  eigener Figur (Ecke, Gangwechsel), Strg+Z bytegleich; im vollen Lauf
  zusaetzlich academy1 (OpenJK) und md_am_sith (Movie Duels).

- **Punkt 5 Handbuch-/Tourbilder**: die Seite zeitleiste ist neu
  aufgenommen (h_zeitleiste, h_zl_detail, h_zl_erklaerung, h_zl_probleme,
  h_zl_ziehen, neu h_zl_aufnahme, h_zl_untertitel; 4 Sprachen,
  `BHED_HB_NUR=zeitleiste1`, `/y/zl1_hb.sh`). Texte geprueft: k-neu-timen
  (Pfeiltasten), Touren mit den alten Zeilen "Ablauf"/"Blick" zeigen jetzt
  auf die Zeile Befehle der Figur. **Offen**: die uebrigen Bilder mit der
  alten Leiste (c_tl_play, c_tl_ton, c_tl_folgen, c_tl_tempo, c_tl_zeit,
  c_tl_ruhig, c_tl_kante, c_tl_klick, c_zeit_kamera, c_pilot_taste,
  g_ansehen_zeit, g_gang_zeit, g_moment, h_figur_bahn, k_saber, k_waits,
  s_sig_zeit, s_task_zeit, t3_4, a_k_timen) entstehen nur im vollen
  Bildlauf `BHED_EDITORTEST=handbuch`; `/y/zl2_hbvoll.sh <en|de|zh-Hans|ja>`
  macht den Lauf und uebernimmt nur diese Bilder (bilder.py auf einer
  Auswahl). Der englische Lauf war gestartet, als Windows Update den
  Rechner um 00:29 neu startete - kein Ergebnis. es/cs-Bildsaetze gibt es in
  diesem Zweig nicht.
- **Punkt 6 Schlussrunde (verkuerzt, Zeitgrenze 06:40 nach dem Neustart)**:
  sauberer Neubau 0 Fehler / 0 Warnungen; ctest 167/167 (neu: untertitel,
  aufnahme); lint_sources, lint_fassung, lint_gapp, lint_imgui_context,
  lint_layout, lint_abstand, lint_doppelanzeige ok, Handbuch
  handbuch_sprachen ok (lint_i18n meldet 700 alte Befunde im ganzen
  Programm, keinen in den neuen Zeilen - es liest keine .inc-Dateien).
  zeitleiste1 voll: erster Lauf zl2k_zeitleiste1 68/9 - Aufnahme auf den
  Originalkarten (academy1: npc_kyle1 um 2 s steht hinter einem waitsignal,
  das Schreiben lehnt ab - richtig, aber der Test waehlte schlecht;
  md_am_sith: bd3 lief gegen eine Konsole) und der JK2-Teil nach den neuen
  Missionen ("kejim_post" nicht in der Liste). Test geaendert: Figur zur
  Zeit ihres ersten eigenen Befehls mit freiem Weg, JK2 vor den neuen
  Missionen. Danach zl2k2 62/17 (nach dem JK2-Teil gelten dessen Regeln und
  Spielordner - Untertitel/Aufnahme auf der MD-Karte scheitern; ausserdem
  fand figurNamens("guard1") die Figur "bigguard1"). Zurueck: JK2 zuletzt,
  genauer Namensvergleich. **zl2k3: 73 OK, 4 FEHLER** - gruen: Mini-Szene,
  fremde Skripte, Untertitel (Mini), Aufnahme Mini + academy1 (OpenJK,
  npc_kyle1, Laufende 5,9 vom Ziel) + md_am_sith (Movie Duels), academy1-
  und md_am_sith-Szenen, Untertitel-Foto academy1. Rot nur der JK2-Teil
  im vollen Lauf: nach den Schritten "New mission" (zl2unter) zeigt die
  Missionsliste kejim_post nicht ("auch nach dem Rollen nicht da") - der
  JK2-Teil allein (BHED_ZL1_NUR=jk2, zl2k4jk2) ist gruen (6 Saetze, 2 mit
  Untertitel, Pakete KEJIM_POST + ARTUS_MINE). Offen: die Missionsliste
  nach einer eigenen neuen Mission im Selbsttest (Ursache nicht
  untersucht).
  **Nicht gelaufen** (Zeit): zeitleiste, kamera, missionsbau, umriss,
  mission1, szenen, ton, karte, Modus 1 - Skript `/y/zl2_ende.sh` liegt
  bereit.

## Changelog

## English

- Record a walk: select a figure, press "Record" in the timeline's transport bar and steer it in the preview with the game's movement (W/S/A/D, Shift walks, Q/E turns); Enter writes walk targets the way scripts let an NPC walk - one ref_tag and one task with SET_NAVGOAL plus dowait per point, only where the path needs one (adjustable tolerance, walls, gait changes), with SET_WALKING/SET_RUNNING - as one undo step; Esc cancels.
- Subtitles in the preview picture exactly as the game draws them: line breaks, two lines at a time for as long as the sound, in the lower black bar, colour from SET_CAPTIONTEXTCOLOR, Movie Duels at half size; a "Subtitles" switch offers the game's choices with the game's default (none in Jedi Academy and Movie Duels, in cinematics in Jedi Outcast).
- Jedi Outcast: the speech row now shows the subtitles from the strip/*.sp files, including the extra packages some maps read.
- "Open script line" also works for commands from other scripts (run, use, spawnscript): it opens that script and selects the line.
- Arrow keys over the timeline: 50 ms (one game frame), Shift 1 s, Ctrl 5 s; the legend lists the keys.
- New timeline in the Mission tab, built after the approved design: **Overview** shows camera position (one block per shot, named after its mark or as readable coordinates) and rotation, speech with speaker, subtitle and waveform, music, the script flow with labelled waits ("waits for 'camera 2'") and one row per actor (named animations on top, numbered walks, dashed where it stands still, hatched where it is not visible); **Detail** splits the camera into move, pan, zoom and fade and draws field of view and fade as straight lines (the game interpolates linearly).
  - Real durations everywhere: a line lasts as long as its sound file, a walk until arrival, an animation until its hold time ends - the timeline and the "Tracks" view now read one shared model.
  - Camera moves that start before the previous one has finished get their own lane and the note "jumps back", with the reason from cg_camera.cpp.
  - "What happens here?": click any block or mark and the side bar explains it in plain words, with time, command and script line; "Open script line" opens the command in the editor.
  - Camera frames every 2 s as a film strip in the timeline, taken in a short pass that does not slow the view down.
  - Navigation: a scene starts with about 30 s, "All" shows everything, Ctrl+wheel zooms around the pointer, a slim navigation bar appears only when zoomed in; filters "Only active" and "Problems", a search field; Space plays, Tab switches Overview/Detail.
  - Dragging: move camera blocks and lines, drag the right edge of camera blocks and waits; snaps to 50 ms; a note says what changes in the script; changed blocks are outlined; every drag is exactly one undo step (the double undo step of Alt-dragging is fixed). Walks cannot be stretched - their duration comes from path finding.
  - Commands the preview does not simulate yet (camera TRACK/DISTANCE, print) appear as red marks.

## Deutsch

- Einen Lauf aufnehmen: eine Figur wählen, in der Transportleiste der Zeitleiste "Aufnehmen" drücken und sie in der Vorschau mit der Bewegung des Spiels steuern (W/S/A/D, Umschalt geht, Q/E dreht); Enter schreibt Laufziele so, wie Skripte einen NPC laufen lassen - je Punkt ein ref_tag und eine Aufgabe mit SET_NAVGOAL samt dowait, nur wo der Weg einen braucht (einstellbare Toleranz, Wände, Gangwechsel), mit SET_WALKING/SET_RUNNING - als ein Rückgängig-Schritt; Esc bricht ab.
- Untertitel im Vorschau-Bild genau so, wie das Spiel sie zeichnet: Zeilenumbruch, je zwei Zeilen so lange wie der Klang, im unteren schwarzen Balken, Farbe aus SET_CAPTIONTEXTCOLOR, Movie Duels halb so groß; ein Schalter "Untertitel" bietet die Wahl des Spiels mit dessen Vorgabe (keine in Jedi Academy und Movie Duels, in Zwischensequenzen in Jedi Outcast).
- Jedi Outcast: die Sprach-Zeile zeigt jetzt die Untertitel aus den strip/*.sp-Dateien, samt der Zusatzpakete mancher Karten.
- "Skriptzeile öffnen" geht auch bei Befehlen aus anderen Skripten (run, use, spawnscript): es öffnet jenes Skript und wählt die Zeile.
- Pfeiltasten über der Zeitleiste: 50 ms (ein Spielbild), Umschalt 1 s, Strg 5 s; die Legende nennt die Tasten.
- Neue Zeitleiste im Mission-Tab nach dem freigegebenen Entwurf: **Übersicht** zeigt Kamera-Ort (je Einstellung ein Block mit dem Namen der Markierung oder lesbaren Koordinaten) und -Drehung, Sprache mit Sprecher, Untertitel und Wellenform, Musik, den Skriptablauf mit beschrifteten Wartezeiten ("wartet auf ‚camera 2‘") und je Darsteller eine Zeile (oben Animationen mit Namen, nummerierte Läufe, gestrichelt, wo er still steht, schraffiert, wo er nicht zu sehen ist); **Detail** trennt Kamerafahrt, Schwenk, Zoom und Blende und zeichnet Sichtfeld und Blende als gerade Linien (das Spiel rechnet linear).
  - Überall echte Dauern: ein Satz so lang wie seine Sounddatei, ein Lauf bis zur Ankunft, eine Animation bis zum Ende ihrer Haltezeit - Zeitleiste und Ansicht "Spuren" lesen jetzt ein gemeinsames Modell.
  - Kamerafahrten, die beginnen, bevor die vorige fertig ist, bekommen eine eigene Bahn und den Hinweis "springt zurück", mit dem Grund aus cg_camera.cpp.
  - "Was passiert hier?": ein Klick auf einen Block oder eine Marke erklärt ihn in der Seitenleiste in normaler Sprache, mit Zeit, Befehl und Skriptzeile; "Skriptzeile öffnen" öffnet den Befehl im Editor.
  - Kamerabilder alle 2 s als Filmstreifen in der Zeitleiste, aufgenommen in einem kurzen Durchgang, der die Ansicht nicht verlangsamt.
  - Bewegen: eine Szene beginnt mit etwa 30 s, "Alles" zeigt alles, Strg+Mausrad zoomt um den Zeiger, eine schlichte Navigationsleiste erscheint nur beim Hineinzoomen; Filter "Nur aktive" und "Probleme", ein Suchfeld; Leertaste spielt ab, Tab schaltet Übersicht/Detail.
  - Ziehen: Kamerablöcke und Sätze verschieben, die rechte Kante von Kamerablöcken und Wartezeiten ziehen; Raster 50 ms; ein Hinweis sagt, was sich im Skript ändert; geänderte Blöcke sind umrandet; jedes Ziehen ist genau ein Rückgängig-Schritt (der doppelte Schritt beim Alt-Ziehen ist behoben). Läufe lassen sich nicht strecken - ihre Dauer kommt aus der Wegsuche.
  - Befehle, die die Vorschau noch nicht nachbildet (camera TRACK/DISTANCE, print), erscheinen als rote Marken.

## 中文

- 录制行走：选择一个角色，在时间轴的传输栏中按“录制”，并在预览中用游戏的移动方式操控它（W/S/A/D，Shift 走路，Q/E 转身）；Enter 按脚本让 NPC 行走的方式写入行走目标——每个点一个 ref_tag 和一个带 SET_NAVGOAL 的 task 加 dowait，只在路径需要的地方（可调容差、墙、步态变化），并带 SET_WALKING/SET_RUNNING——作为一个撤销步骤；Esc 取消。
- 预览画面中的字幕与游戏绘制的完全一致：换行、每次两行且持续声音的时长、显示在下方黑条中、颜色来自 SET_CAPTIONTEXTCOLOR、Movie Duels 为一半大小；“字幕”开关提供游戏的选项及其默认值（Jedi Academy 和 Movie Duels 不显示，Jedi Outcast 在过场动画中显示）。
- Jedi Outcast：语音行现在显示来自 strip/*.sp 文件的字幕，包括某些地图读取的附加包。
- “打开脚本行”也适用于来自其他脚本（run、use、spawnscript）的命令：它会打开那个脚本并选中该行。
- 时间轴上的方向键：50 毫秒（一个游戏帧），Shift 1 秒，Ctrl 5 秒；图例列出了这些按键。
- 任务标签页中按已批准的设计重做的新时间轴：**概览**显示摄像机位置（每个镜头一个块，以标记名称或易读坐标命名）和旋转、带说话者、字幕和波形的语音、音乐、带标注等待的脚本流程（“等待“camera 2””），以及每个角色一行（上方是带名称的动画，编号的行走，静止处为虚线，不可见处为斜线）；**详细**把摄像机分为移动、平移、缩放和淡入淡出，并把视野和淡入淡出画成直线（游戏按线性插值）。
  - 处处使用真实时长：台词与其声音文件一样长，行走持续到到达，动画持续到保持时间结束——时间轴和“轨道”视图现在读取同一个模型。
  - 在上一个移动结束前就开始的摄像机移动会获得自己的轨道和“跳回”提示，并给出来自 cg_camera.cpp 的原因。
  - “这里发生了什么？”：单击任意块或标记，侧栏会用通俗的话解释它，并给出时间、命令和脚本行；“打开脚本行”会在编辑器中打开该命令。
  - 时间轴中每 2 秒一张摄像机画面的胶片条，在不拖慢视图的短暂过程中拍摄。
  - 浏览：场景以约 30 秒开始，“全部”显示整个场景，Ctrl+滚轮以指针为中心缩放，只有放大时才显示简洁的导航栏；“仅活动”和“问题”筛选，搜索框；空格播放，Tab 切换概览/详细。
  - 拖动：移动摄像机块和台词，拖动摄像机块和等待的右边缘；对齐到 50 毫秒；提示说明脚本中将改变什么；更改过的块带边框；每次拖动恰好是一个撤销步骤（修复了 Alt 拖动时的双重撤销步骤）。行走无法拉长——其时长来自寻路。
  - 预览尚未模拟的命令（camera TRACK/DISTANCE、print）显示为红色标记。

## 日本語

- 歩きの記録：キャラクターを選び、タイムラインのトランスポートバーで「記録」を押して、プレビューでゲームの移動のまま操作します（W/S/A/D、Shift で歩く、Q/E で向き）。Enter で、スクリプトが NPC を歩かせる方法で移動目標を書き込みます。点ごとに ref_tag と SET_NAVGOAL の task と dowait を、経路が必要とする所だけ（調整できる許容差、壁、歩き方の変化）、SET_WALKING/SET_RUNNING 付きで、元に戻す 1 回分として。Esc でキャンセル。
- プレビュー画像の字幕をゲームの描き方どおりに表示：改行、音の長さだけ 2 行ずつ、下の黒帯に、色は SET_CAPTIONTEXTCOLOR、Movie Duels は半分の大きさ。「字幕」スイッチでゲームの選択肢とその既定値（Jedi Academy と Movie Duels はなし、Jedi Outcast はムービー中）を選べます。
- Jedi Outcast：台詞の行に strip/*.sp ファイルの字幕（一部のマップが読む追加パッケージを含む）が表示されるようになりました。
- 「スクリプト行を開く」が他のスクリプト（run、use、spawnscript）から来たコマンドでも使えます。そのスクリプトを開いて行を選択します。
- タイムライン上の矢印キー：50 ms（1 ゲームフレーム）、Shift で 1 秒、Ctrl で 5 秒。凡例にキーが載っています。
- 承認されたデザインに基づくミッションタブの新しいタイムライン：**概要**はカメラの位置（ショットごとに 1 ブロック、マーク名または読みやすい座標）と回転、話者・字幕・波形付きの台詞、音楽、ラベル付きの待機（「「camera 2」を待機」）を含むスクリプトの流れ、キャラクターごとの行（上に名前付きアニメーション、番号付きの移動、静止は破線、非表示は斜線）を表示し、**詳細**はカメラを移動・パン・ズーム・フェードに分け、視野とフェードを直線で描きます（ゲームは直線的に補間）。
  - どこでも実際の長さ：台詞はサウンドファイルの長さ、移動は到着まで、アニメーションは保持時間の終わりまで。タイムラインと「トラック」表示は同じモデルを読みます。
  - 前の移動が終わる前に始まるカメラ移動は専用のレーンと「戻る」の注記が付き、cg_camera.cpp による理由が示されます。
  - 「ここで何が起きる？」：ブロックやマークをクリックすると、サイドバーが時間・コマンド・スクリプト行とともにわかりやすく説明し、「スクリプト行を開く」でエディタが開きます。
  - タイムラインに 2 秒ごとのカメラ画像をフィルムストリップとして表示。ビューを遅くしない短い処理で撮影します。
  - 移動：シーンは約 30 秒の範囲で始まり、「全体」で全体表示、Ctrl+ホイールでポインタ中心にズーム、拡大時だけ細いナビゲーションバー。「動作中のみ」「問題」フィルターと検索欄。スペースで再生、Tab で概要/詳細を切り替え。
  - ドラッグ：カメラのブロックと台詞を移動、カメラのブロックと待機の右端で長さを変更。50 ms にスナップし、スクリプトで何が変わるかを表示。変更したブロックは枠で示され、ドラッグごとにちょうど 1 回の元に戻す操作になります（Alt ドラッグの二重のステップを修正）。移動は伸ばせません。長さは経路探索で決まります。
  - プレビューがまだ再現しないコマンド（camera TRACK/DISTANCE、print）は赤いマークで表示されます。
