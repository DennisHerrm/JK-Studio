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
  Weitere Selbsttests (`/y/zl2_rest.sh`, hintereinander, unter Last durch
  die anderen Agenten): kamera 95/0, umriss 45/0, missionsbau 95/0, szenen
  26/0, karte 229/0; mission1 97/1 ("Gizmo: Kamera - beim Abspielen kein
  stehengebliebener Gizmo": Achsenfarben beim Abspielen 216/0, nach Stop
  45 statt ~1338), zeitleiste 46/1 und ton 33/1 (beide "der Klang kommt,
  sobald sie gezeichnet wird"). ton allein wiederholt: diese Pruefung
  gruen (Klang bei 1054 ms), dafuer "ein Halt ohne Stopknopf beendet die
  Schleifen" rot - zeitabhaengige Tonpruefungen unter Last, keine der
  Aenderungen dieses Zweigs beruehrt den Ton. Allein wiederholt:
  mission1 98/0 (die Gizmo-Pruefung gruen), zeitleiste 47/0.
  **Nicht gelaufen** (Zeitgrenze): Modus 1 (90 min), alle Modi,
  Kartenpruefung - Skript `/y/zl2_ende.sh` liegt bereit.

### Stand beim Anhalten (30.10., 06:40)

- Fertig und committet: Punkte 1, 2, 3, 4, 7; Punkt 5 bis auf die Bilder
  aus dem vollen Bildlauf; Punkt 6 verkuerzt (oben).
- Offen: (1) Bilder mit der alten Leiste aus `BHED_EDITORTEST=handbuch`
  (`/y/zl2_hbvoll.sh` je Sprache, Dauer unbekannt); (2) Modus 1 und die
  restliche Schlussrunde (`/y/zl2_ende.sh`); (3) JK2-Teil im vollen
  zeitleiste1-Lauf nach "New mission" (Missionsliste); (4) Aufnahme:
  waehrend der Aufnahme zeigt die Vorschau einen Umriss statt des Modells;
  der Hinweis ist bei schmaler Ansicht abgeschnitten; (5) MD
  cg_textprintscale "0.5f" gegen die installierte DLL nicht pruefbar.
- Naechster Schritt: `/y/zl2_hbvoll.sh en` (Dauer messen), dann de,
  zh-Hans, ja; danach `/y/zl2_ende.sh`; mission1/zeitleiste/ton allein
  wiederholen, wenn sie dort wieder rot sind.

## Zeitleiste 3

Auftrag `<SP>/auftrag_zeitleiste3.md` (Reste von Zeitleiste 2 und volle
Schlussrunde). Laeufe unter `Y:\mission_laeufe\zl3*`.

- **Punkt 1 JK2-Teil im vollen Lauf** (2c670fb0): Ursache war nicht die neue
  Mission, sondern die Missionsauswahl: ein neu geoeffnetes Fenster behielt
  die Rollposition der vorigen Liste (Movie Duels, md_am_sith weit unten),
  kejim_post lag darueber, waehleInListe rollt nur nach unten (Foto
  zl2k3/zl2_jk2_kejim_post). Behoben in drawMissionPicker
  (IsWindowAppearing -> SetNextWindowScroll 0). Neue Pruefung "die
  Missionsauswahl beginnt oben": rot zl3p1rot (72/6), gruen zl3p1gr (76/1,
  der Rest siehe unten), voll zl3gr 80/0.
- **Punkt 3 Aufnahme** (2c670fb0):
  - Hinweis: er war nicht vom Fensterrand, sondern von der Platzierleiste
    links (platzierLeiste, je nach Hoehe der Ansicht verschieden hoch)
    verdeckt. Jetzt mittig im sichtbaren Teil, mit Rand Knopfhoehe + 1,4 fs,
    umbrochen bei schmaler Ansicht. Pruefung "der Hinweis steht ganz im Bild,
    frei von Knoepfen" (Fensterrechteck aus ImGui, kein Element darunter,
    die Spalte der Platzierleiste frei): rot zl3p3rot2 (14/1), gruen zl3gr.
  - Modell statt Umriss: aufnahmeFigurZustand setzt beim Zeichnen Ort und
    Blick der gesteuerten Figur, beim Bewegen BOTH_RUN1/BOTH_WALK1 mit dem
    Tempo der Szene (Geschwindigkeit / kAnimSpeedRun bzw. Walk, begrenzt,
    hoechstens 1,5 * frameLerp / 50) an der Zeit der Aufnahme; steht sie,
    bleibt die Haltung zur Startzeit. Der Kasten bleibt als Umriss dazu.
    Pruefung ueber figurGezeichnet: rot zl3p3rot (Modell 0 vom Ort der
    Szene), gruen zl3p3rot2/zl3gr (119 .. 391 Einheiten). Abweichung: kein
    Uebergang (PM_SetAnim-Blend) beim Wechsel Stehen/Laufen, keine
    Rueckwaerts-/Seitwaertsanimation (die Szene nimmt BOTH_RUNBACK1 nur fuer
    geschriebene Laeufe).
  - Untertitel-Seitenwechsel (c3f4887d): wie CG_DrawCaptionText im ersten
    Clientbild nach captionNextTextTime, die naechste Seite zaehlt ab diesem
    Bild; Clientbilder alle 8 ms ab der Ankunft des Untertitels
    (com_maxfps 125, wie kSaberClientTaktMs). ctest untertitel: rot (2
    Fehler), gruen.
  - Testfund: guard1 (md_am_sith) lief unter Last in 45 Bildern 443 statt
    331 Einheiten weit, hinter das Wegenetz ("finds no path", steht wie im
    Spiel) - die Pruefung "die Vorschau laeuft den Weg" war rot (zl3p1rot,
    zl3p1gr). Die Tasten halten jetzt bis 45 Proben; richtig war das
    Programm (Statuszeile meldet den fehlenden Weg).
- **Punkt 2 Bilder mit der alten Leiste**: Seite zeitleiste in 4 Sprachen
  neu (`/y/zl1_hb.sh`, je 2-8 min unter Last; 39b3e3fd: h_zl_aufnahme mit
  Modell und freiem Hinweis, h_zl_untertitel mit MD 0.66, weitere wo sich
  der Inhalt aenderte). Voller Bildlauf `/y/zl3_hbvoll.sh en 2100` (wie
  zl2_hbvoll.sh, Zeitgrenze als Argument): nach 27 min waren alle 19
  ausgewaehlten Bilder und a_k_timen_* da (660 Fotos), der Rest des Laufs
  wurde abgebrochen; 6055731a uebernimmt 21 englische Bilder. Der volle Lauf
  meldet unter Last 66 Klickfehler in anderen Teilen (Cancel, ##entsuche,
  ROFF file ...), die ausgewaehlten Bilder sind durchgesehen (k_waits,
  c_tl_play: neue Leiste). **Offen: de, zh-Hans, ja** (je ~30 min,
  `sh /y/zl3_hbvoll.sh de 2400` usw.).
- **Punkt 4 MD cg_textprintscale** (c3f4887d): in der installierten
  MovieDuels-gamex86_64.dll (SP, 27.10.2026) steht der Name doch (vorher
  wohl in der falschen DLL gesucht); der Eintrag der Cvar-Tabelle (Zeiger auf
  Name, Vorgabe, Flags) lautet "0.66f", CVAR_ARCHIVE. Die 32-Bit-DLL
  (31.12.2025) und die MP-cgame (02.08.2026) haben "0.5f", mdgit (13.10.)
  auch. Uebernommen: 0.66 (kMdTextprintscale); keine MD-Config ueberschreibt
  den Wert. Gelesen mit einem eigenen PE-Leser (nur lesend), Handbuch- und
  Changelog-Text angepasst ("kleiner" statt "halb so gross").

- **Punkt 5 Schlussrunde (verkuerzt, Zeitgrenze 08:40)**: sauberer Neubau
  aller Ziele (`--clean-first`, waehrend des Bildlaufs): 0 Fehler, 0
  Warnungen. ctest 167/167 (521 s unter Last). lint_sources, lint_fassung
  (rc606), lint_gapp, lint_imgui_context, lint_layout, lint_abstand,
  lint_doppelanzeige ok, Handbuch handbuch_sprachen ok. Selbsttests nach dem
  Neubau (`/y/zl3_ende.sh`, nacheinander, unter Last): zeitleiste 47/0,
  mission1 98/0, ton 34/0
  (die zeitabhaengigen Tonpruefungen diesmal gruen), umriss 45/0, missionsbau
  95/0, kamera 95/0, szenen 26/0, karte 229/0. zeitleiste1 voll mit demselben Code vor dem Neubau: zl3gr 80/0.
  **Nicht gelaufen**: Modus 1 (90 min), alle Modi, Kartenpruefung - die
  Zeit reichte nach den Punkten 1-4 und dem Bildlauf nicht.
  Gegen das Vorbild geprueft: Seitenwechsel (cg_text.cpp mdgit:
  CG_DrawCaptionText), Vorgabe cg_textprintscale (installierte DLL),
  Lauf-/Gehanimation (Regeln der Szene, scene.cpp); nicht im laufenden
  Spiel verglichen (keine Spielstarts).

### Stand beim Anhalten (30.10., 08:40)

- Fertig und committet: Punkt 1 (Missionsauswahl oben), Punkt 3 (Hinweis,
  Modell, Seitenwechsel), Punkt 4 (MD 0.66 aus der DLL), Punkt 2 fuer die
  Seite zeitleiste (4 Sprachen) und die englischen Bilder des vollen
  Bildlaufs; Schlussrunde verkuerzt (oben).
- Offen: (1) die Bilder mit der alten Leiste in de, zh-Hans, ja
  (`sh /y/zl3_hbvoll.sh <sprache> 2400`, je ~30 min unter Last; die
  ausgewaehlten Bilder entstehen nach ~25 min, der Rest des Laufs wird nicht
  gebraucht); (2) Modus 1 mit BHED_HANDBUCH, alle Modi, Kartenpruefung;
  (3) Aufnahme: kein Animationsuebergang beim Wechsel Stehen/Laufen, keine
  Rueckwaertsanimation beim Steuern, der Oberkoerper laeuft mit (die Szene
  behaelt eine Oberkoerper-Skriptanimation).
- Naechster Schritt: `/y/zl3_hbvoll.sh de 2400`, dann zh-Hans, ja; danach
  Modus 1 (`/y/zl2_ende.sh` enthaelt ihn) und die uebrigen Modi.

## Zeitleiste 4

Auftrag `<SP>/auftrag_zeitleiste4.md`. Laeufe unter `Y:\mission_laeufe\zl4*`,
voller Bildlauf jetzt `sh /y/zl4_hbvoll.sh <sprache> [sekunden]` (wie
LIESMICH: taspir1 aus assets0.pk3, Missionen und Archiv auf X: per subst).

- **Punkt 1: die 66 Klickfehler des vollen Bildlaufs** (a501e946). Zwei
  Ursachen, keine verlorenen Klicks unter Last:
  - Im Programm: die Einstellungen sind bei Oberflaeche 1,3 (der Bildlauf
    stellt sie ein) hoeher als das Fenster (1462 von 1472 px). ImGui schnitt
    das ganze Fenster ab, Ok und Cancel lagen unter dem Rand (Cancel unten bei
    1487); der Klick auf Cancel traf nichts, die Einstellungen blieben offen
    und sperrten als Modal jeden weiteren Klick ("Fenster -", 62 von 62
    Klicks bis zum Ende des Laufs). Jetzt rollt nur der Inhalt (Kind mit
    Hoehenbegrenzung), Ok/Cancel bleiben stehen; das Fenster wird in den
    ersten Bildern mittig gesetzt (das Kind misst sich ein Bild spaeter -
    ohne das stand es an der Mitte der ersten, zu kleinen Hoehe und wuchs
    nach unten aus dem Bild). Pruefung zeitleiste1 `BHED_ZL1_NUR=einstellungen`:
    rot zl4p1rot (0 OK, 4 Fehler: Cancel nicht ganz sichtbar, Klick kam nicht
    an, Einstellungen offen), gruen zl4p1gr (2/0).
  - Im Test/Aufruf: zl2/zl3_hbvoll.sh setzten BHED_MISSION_ARCHIV/NAME nicht;
    die Vorgabe war md_am_sith ohne Archiv - die Uebungsmission taspir_tut
    entstand auf md_am_sith (BSP 16,6 MB statt taspir1 15,5 MB), die Raven's
    Claw fehlte ("ROFF file nicht gefunden", Kyles Lauf ohne Dauer). Auch die
    englischen Bilder aus 6055731a stammen von dieser falschen Karte (t3_4
    ohne Schiff und Animationsfenster). Vorgabe jetzt taspir1 aus assets0.pk3
    der Spielordner, LIESMICH angepasst, neues Skript /y/zl4_hbvoll.sh.
  - Voller Lauf en danach: 0 Klickfehler, 0 FEHLER, in 7 min ganz durch
    (zl4hb_en, 669 Fotos; vorher 66 Fehler und nach 35 min abgebrochen).
- **Punkt 2: Handbuch- und Tourbilder mit der neuen Leiste** (a913f423,
  249b7add, c735f2cd, Bilder siehe unten). Jedes englische Bild einzeln
  angesehen - die Bilder aus 6055731a waren nicht fehlerfrei (falsche Karte,
  siehe Punkt 1) und sind ersetzt. Weitere Fehler in den Bildern, behoben im
  Bildlauf:
  - c_tl_klick: Kyles Zeile lag in der Detailansicht ausserhalb der Leiste,
    der Klick traf nichts, das Bild zeigte die Erklaerung eines anderen Blocks.
    Jetzt wird Kyle gewaehlt (die Leiste rollt zu ihm), Pruefung "Kyles
    Laufbalken ganz in der Zeitleiste".
  - c_tl_kante: die Marke lag auf Kyles Laufbalken (ausserhalb, und ein Lauf
    behaelt laut Text seine Laenge) - jetzt die rechte Kante der Kamerafahrt.
  - c_zeit_kamera: endete bei "Blende"; fuer das Bild ist die Leiste hoeher
    (Wackeln, Bildwinkel, Schwarz im Bild).
  - k_waits: Marke auf "Detail" und die Leiste noch bei Kyle - jetzt oben, die
    Marke auf dem ersten Warteblock.
  - s_sig_zeit: war leer (00:00.00) - talk lief nie, das waitsignal hing; auch
    das alte Bild vor 6055731a war leer. Jetzt laeuft talk auf Kyle (affect +
    run; ein run im Intro liefe an Ort und Stelle, CSequencer::ParseRun, das
    Signal kaeme vor dem waitsignal), der Warteblock wird angeklickt, die
    Seitenleiste nennt Kyle bei 0:02.0. Pruefung "das waitsignal steht als
    Block".
  - a_k_timen: die dritte Zeile des Hinweises beim Ziehen lag unter dem
    Bildrand; jetzt endet das Bild knapp unter der Leiste, in jedem Bild
    gleich hoch (sonst baut bilder.py kein bewegtes Bild).
  - h_einst_vorschau: mit dem neuen Rollbereich ans Ende gerollt, Ok/Cancel
    ganz im Bild.
  - Uebernommen in vier Sprachen: c_pilot_taste, c_tl_folgen, c_tl_kante,
    c_tl_klick, c_tl_play, c_tl_ruhig, c_tl_tempo, c_tl_ton, c_tl_zeit,
    c_zeit_kamera, g_ansehen_zeit, g_gang_zeit, g_moment, h_einst_vorschau,
    h_figur_bahn, k_saber, k_waits, s_sig_zeit, s_task_zeit, t3_4, a_k_timen
    (+ _stand). de/zh/ja auf Kontaktboegen gegen en geprueft. Jeder Lauf: 0
    Klickfehler, 0 FEHLER, 6 OK (2 neue Pruefungen), ganz durch in 7-8 min.
  - Gesehen, nicht geaendert: im Klappfeld "Spielregeln" der Einstellungen ist
    "Automatic (detected: Movie Duels)" abgeschnitten (Breite des Felds, auch
    vorher so).
- **Punkt 3: das Modell waehrend der Aufnahme wie im Spiel** (3826604e,
  ba0d58e4). Neuer Kern `include/bhed/pmanim.h`, `src/pmanim.cpp`, ctest
  `pmanim` (Erwartungen aus den Quellen mit Zeilen):
  - PM_Footsteps, Zweig am Boden ohne Ducken/Wasser/Leiter (so steuert die
    Aufnahme): Stand (STAND1, STAND3 fuer NPC mit Waffe ausser den
    Wachhaltungen, STAND4 Imperial, Kampfhaltungen nach saberAnimLevel - OpenJK
    eigene Tabelle, MD PM_ReadyPoseForSaberAnimLevel), vorwaerts/seitwaerts
    laufen und gehen, rueckwaerts (PMF_BACKWARDS_RUN aus PmoveSingle), Klinge
    (RUN2/RUN_DUAL/RUN_STAFF, WALK2 ..., MD fuer NPCs RUNBACK1/WALK1), Movie
    Duels mit g_ActivateAnimationStyle 0 die Waffenvarianten (JOG_/WALK_/
    RUNBACK_/WALKBACK_ PISTOL, DOUBLE_PISTOL, BLASTER, HEAVY, GRENADE), Klassen
    (ATST/Droideka mit Anlauf RUN1START und Auslauf RUN1STOP samt HOLD-Timer,
    Galak, Jawa, Wampa nach runSpeed, Rancor, Hazard Trooper, SBD, Yoda/Vader/
    Sithlord). Seitwaerts gibt es in beiden Spielen keine eigene Animation: die
    Laufanimation, und cgame dreht die Beine.
  - PM_SetAnimFinal: laeuft der Timer (Skriptanimation mit HOLD/HOLDTIME), setzt
    PM_Footsteps nichts; dieselbe Animation mit neuem Tempo laeuft an derselben
    Stelle weiter; eine neue blendet 100 ms vom eingefrorenen Bild
    (SETANIM_BLEND_DEFAULT); g_noFootSlide (resultspeed/moveSpeedOfAnim 150/50,
    Stab 250/100, Doppel 100, Hazard 50, Rancor 173, geduckt 75; 0,01..1,5;
    MD in der Kamerafahrt erst ab resultspeed 1); kennt das Modell die
    Animation nicht (numFrames 0), bleibt die alte. PM_WalkingAnim/
    PM_RunningAnim als Listen 1:1 (OpenJK, MD).
  - PM_TorsoAnimation: der Oberkoerper folgt den Beinen oder zeigt die
    Waffenhaltung (npcWaffenTorso, der Port der Szene), solange kein
    Torso-Timer haelt; Uebergang auch beim Zurueckfallen auf die Beine.
  - cgame: CG_PlayerLegsYawFromMovement je Clientbild (8 ms, Anteil zwischen
    zwei 50-ms-Serverbildern als frameInterpolation; +-60, rueckwaerts
    gespiegelt, ueber 150 nicht, 5 bzw. 10 Grad je Bild) und
    CG_G2ClientSpineAngles (Rumpf zurueck zum Blick: lower_lumbar 0,45,
    upper_lumbar 0,35, thoracic 0,20) - neues Feld rumpfGier in ActorState
    und ActorDraw, gedreht in mapview.cpp nach dem Uebergang wie der Kopf.
  - Die Szene gibt die Anim-Timer heraus (beineTimerBisMs, torsoTimerBisMs);
    die Aufnahme startet mit Beinen, Oberkoerper und Timern der Szene. Dazu
    "If Have Not Been Moving For A While, Stop" (nach 1 s ohne Bewegung am
    Boden keine Eingabe mehr fuer die Animation) und resultspeed je Pmove.
  - Selbsttest zeitleiste1 (`BHED_ZL1_NUR=aufnahme`, Stormtrooper mit
    WP_BLASTER, Movie Duels): rot zl4p3rot 14/5 (ueberall BOTH_RUN1, kein
    Uebergang, Beine nicht gedreht, kein Waffen-Oberkoerper), gruen zl4p3gr
    19/0 (BOTH_JOG_BLASTER und BOTH_RUNBACK_BLASTER mit Uebergang aus
    BOTH_STAND3, seitwaerts BOTH_JOG_BLASTER mit 60 Grad, TORSO_WEAPONREST3).
    Die Pruefung "Modell am gesteuerten Ort" hielt die Tasten eine feste
    Bildzahl (ohne Last nur 85 Einheiten bei Tempo 60); dafuer gilt jetzt die
    Fassung aus Integration 5 (karte1 daf88e15, 0a544a42, hier als 8385d9c7,
    1e2eda71 uebernommen): Vergleich mit dem gesteuerten Ort selbst.
  - Nicht portiert (bewusst, siehe pmanim.h): PM_AdjustStandAnimForSlope (die
    Fusspuren am Modell), Sprint (BUTTON_BLOCK eines Spielers), die
    Motion-Bone-Korrektur von CG_G2ClientSpineAngles (cg_motionBoneComp 2,
    nur wenn Beine und Oberkoerper verschiedene Animationen spielen),
    PM_TorsoAnimLightsaber (der Oberkoerper folgt mit Klinge den Beinen, wie
    in der Szene). g_SerenityJediEngineMode: mdgit-Vorgabe 0 genommen (die
    Kommentare in scene.h nennen 2 fuer die Mod - in den MD-Configs steht
    nichts; betrifft hier nur Stab rueckwaerts gehend).
  - Beobachtet, nicht geaendert: die Szene laesst einen stehenden NPC mit
    Gewehr in BOTH_STAND1; PM_Footsteps setzt BOTH_STAND3 (die Aufnahme zeigt
    es, sobald sie laeuft).
- **Punkt 4: wait ( "name" )** (df3650ce). Geklaert an CTaskManager::Wait
  (code/icarus/TaskManager.cpp, mdgit und OpenJK - OpenJK per GitHub
  gelesen): wait mit einem Namen wartet auf die Taskgruppe dieses Namens
  desselben Traegers, nicht auf das Ende einer Bewegung. Die Gruppe entsteht
  beim Laden (CSequencer::Route -> ParseTask -> AddTaskGroup); vor dem ersten
  do ist sie leer und gilt als fertig (CTaskGroup::Complete: m_numCompleted
  == m_completedTasks.size()); ohne task des Namens liefert GetTaskGroup
  NULL, completed bleibt false, der wait kommt jedes Bild wieder (PUSH_BACK):
  ewig.
  - Der Nachbau (ablauf.cpp) kannte das schon fuer do/wait, hing aber bei
    wait vor dem ersten do: jetzt gruppeWarteFertig (eine Stelle fuer beide
    Abfragen). ablauftest: rot 1 Fehler ("vor dem ersten do"), gruen.
  - clock.cpp:132 (die Naeherung "bis zum Ende des eigenen Zugs") kam in der
    Oberflaeche nie mehr an: Kamerabahn, Zeitleiste und Szene bekommen das
    flache Skript des Ablaufs, in dem jedes Warten schon eine Zahl ist. Die
    Naeherung ist weg (advance ohne ownEndMs, timeline.cpp ohne das nie
    gesetzte lastEndMs, scene.cpp ohne eigene Rechnung), clock.h erklaert es.
  - Die Erklaerung der Zeitleiste nannte wait ( "name" ) ein Warten auf ein
    Signal (spuren.cpp) - jetzt wie dowait "wartet auf die Aufgabe".
    spurentest: rot 1 (Art Signal), gruen.

- **Punkt 5: Schlussrunde** (`/y/zl4_ende.sh`, ohne die alte Last): sauberer
  Neubau aller Ziele 0 Fehler, 0 Warnungen; ctest 168/168 (neu `pmanim`);
  lint_sources, lint_fassung (rc606), lint_gapp, lint_imgui_context,
  lint_layout, lint_abstand, lint_doppelanzeige ok, lint_i18n wie vorher 700
  alte Befunde; Handbuch handbuch_sprachen ok. Selbsttests nach dem Neubau:
  zeitleiste1 voll 86/0, zeitleiste 47/0, mission1 98/0, ton 34/0, szenen
  26/0, kamera 95/0, missionsbau 95/0. Voller Bildlauf in vier Sprachen je
  6/0 und 0 Klickfehler. Modus 1, alle Modi und die Kartenpruefung macht die
  Schlussrunde nach Integration 6.
- **Gegen das Vorbild geprueft**: wait ( "name" ) an CTaskManager::Wait/
  GetTaskGroup/CTaskGroup::Complete und CSequencer::Route/ParseTask (mdgit,
  OpenJK-Wait per GitHub); PM_Footsteps, PmoveSingle (PMF_BACKWARDS_RUN),
  PM_SetAnimFinal, PM_WalkingAnim/PM_RunningAnim, PM_GetRunAnim,
  PM_GetWalkWithSaberOnAnim, PM_SaberDrawPutawayAnim, Pmove (resultspeed,
  Stopp nach 1 s), CG_PlayerLegsYawFromMovement und CG_G2ClientSpineAngles
  (OpenJK SP code/game und mdgit code/game, cgame aus mdgit), die
  Vorgaben g_ActivateAnimationStyle/g_noIgniteTwirl/g_SerenityJediEngineMode
  aus mdgit g_main.cpp. **Nicht** im laufenden Spiel verglichen (Steam lief
  zwar seit 09:30, ein NPC laesst sich im Spiel aber nicht mit W/A/S/D
  steuern - ein Vergleich haette die Spieler-Zweige von PM_Footsteps gezeigt,
  nicht die des NPC).
- **Offen**: PM_AdjustStandAnimForSlope, Sprint, Motion-Bone-Korrektur,
  PM_TorsoAnimLightsaber (siehe Punkt 3); der Stand eines NPC mit Gewehr in
  der Szene (BOTH_STAND1 statt BOTH_STAND3); das Klappfeld "Spielregeln" ist
  fuer "Automatic (detected: Movie Duels)" zu schmal; im Undo-Verlauf der
  Statuszeile heisst das Einfuegen einer Kommentarzeile "inserted ( )".

## Zeitleiste 5

Auftrag `<SP>/auftrag_zeitleiste5.md`, direkt auf karte1 (Y:\karte, ab 2dd08ced).
Laeufe unter `Y:\karte\laeufe\zl5*` (Skript `<SP>/zl5/lauf.sh`, Bildlauf `<SP>/zl5/hb.sh`).

- **Punkt 0: pruefung3 bis 97e61de2** (Merge 9517c1ee). Konflikte:
  - `src/scene.cpp`: ein Timer der Beine (`beineFest ? beineBis : -1`, wie Zeitleiste 4 /
    Integration 6); die Hang-Beine von Pruefung 3 fragen genau diesen Timer.
  - `src/mapview.cpp`: beide Includes.
  - `src/efxdraw.cpp/.h`: inhaltlich. Pruefung 3 legt Decals in den gemeinsamen
    Markenspeicher (buildEffectMesh baut keine mehr); karte1 hatte dem Karten-Editor
    fuer Decals eine eigene Welt an buildEffectMesh gegeben (markenWeltAusKarte). Jetzt ist
    die Welt ein Parameter von effektMarkenIn, die Echtzeit-Ansicht des Karten-Editors
    fuehrt einen eigenen Markenspeicher (ein Clientbild je Editorbild, cg.time ganze ms;
    neue Welt -> leerer Speicher), und das Netz eines Markenspeichers baut eine Funktion
    (markenNetz) fuer die 3D-Ansicht und den Karten-Editor.
  - Nach dem Merge: Neubau 0 Fehler/0 Warnungen, ctest 189/189, pruefung3 9/0, szenen 26/0,
    mission1 98/0, karteneditor `BHED_KARTE_NUR=reste1` (Decal im Karten-Editor) 13/0.
- **Punkt 1: Beine am Hang bei der gesteuerten Figur** (39887c2e). Eine Rechnung fuer
  Szene und Aufnahme: `hangStandBeine` (scene.h) = PM_FootSlopeTrace unter den Fussbolzen
  + `hangBeinSchritt` (Pruefung 3). pmanim ruft sie ueber `PmBeinStand::hang` genau dort,
  wo PM_Footsteps `if ( !PM_AdjustStandAnimForSlope() )` schreibt (ATST/MD-Droideka, mit
  Klinge, mit Waffe ausser den Wachhaltungen, sonst; nicht Galak, nicht in Bewegung) -
  nur dann stellt es slopeRecalcTime. Die Aufnahme rechnet die Fussbolzen aus ihrer
  eigenen Pose zur Uhr der Aufnahme (`figurFussPunkte` mit Zustand, `aufnahmeHangBeine`).
  - ctest pmanim: rot (kein `PmBeinStand::hang`, Bau), gruen 10 neue Faelle.
  - Selbsttest zeitleiste1 `BHED_ZL1_NUR=aufnahme`: sucht nahe dem Start eine Stelle mit
    6..20 Einheiten Hoehenunterschied unter den Fuessen (md_am_sith hat dort keine Schraege
    mit 0,80 <= normal.z <= 0,97, aber eine Stufe bei 292 7100), stellt zl2_laeufer quer
    darauf und nimmt auf: rot zl5p1rot 20/1 (Beine BOTH_STAND3), gruen zl5p1gr 21/0
    (LEGS_S3_RUP3, dann je 100 ms eine Stufe bis LEGS_S3_RUP5).
- **Punkt 2: Sprint, PM_TorsoAnimLightsaber, Motion-Bone-Korrektur**:
  - PM_TorsoAnimLightsaber (39887c2e): `pmTorsoLichtschwert` fuer die Figur am Boden mit
    Klinge. Laeuft sie (PM_RunningAnim) oder geht sie mit Stab/zwei Klingen, folgt der
    Oberkoerper den Beinen; sonst PM_SetSaberMove(LS_READY) mit der Kampfhaltung und
    saberMoveData[LS_READY].blendTime 350 ms. OpenJK: STAND2/SABERDUAL/FAST/SLOW_STANCE
    nehmen beim Gehen und Laufen die Beinanimation ("match torso anim to walk/run anim",
    bg_pmove.cpp:9241); Movie Duels: Beine in einer Kampfhaltung -> SETANIM_BOTH, ein
    laufender Oberkoerper bleibt (bg_pmove.cpp:23274 ff. mdgit). ctest pmanim 7 Faelle.
    Nicht abgebildet: .sab readyAnim, SABER_ARC/LANCE/TRIDENT, manuelles Blocken
    (saberBlockingTime), die Figurenvarianten von g_ActivateAnimationStyle 1 und die
    Tabelle PM_ApplyTorsoMap von MD (die gilt nur fuer Animationen, die die Aufnahme nicht
    setzt) - PmFigur kennt die Schwertart nicht.
  - Sprint: **nicht zutreffend**. OpenJK SP hat keinen; in Movie Duels sprintet in
    PM_Footsteps nur, wer BUTTON_BLOCK drueckt oder MBF_HOLDINGBLOCK haelt (mdgit
    bg_pmove.cpp:15365 ff.). Kein NPC-Code setzt BUTTON_BLOCK (nur `&= ~`, bg_pmove.cpp:5853),
    MBF_HOLDINGBLOCK setzt g_active.cpp:10151 nur mit `ent->NPC == NULL`, und den
    Sprint-Tempozuschlag gibt ClientAlterSpeed nur Spielern (g_active.cpp:8158 ff.). Die
    Aufnahme steuert einen NPC (Q3_SetNavGoal), und ein Skript kann ihn nicht sprinten
    lassen - im Spiel sprintet die aufgenommene Figur nie, die Vorschau also auch nicht.
  - Motion-Bone-Korrektur (cg_motionBoneComp 2, CG_G2ClientSpineAngles): **offen**. Sie
    gilt fuer JEDE Figur, deren Beine und Oberkoerper verschiedene Animationen spielen
    (jeder Schuetze beim Gehen), nicht nur fuer die Aufnahme, und verteilt Pitch, Yaw und
    Roll des Motion-Bolzens ueber BG_G2SetBoneAngles (POSTMULT, Achsen je Knochen und
    Klasse, G_BoneOrientationsForClass) auf lower_lumbar, upper_lumbar und thoracic. Die
    Vorschau dreht diese Knochen bisher nur um die Senkrechte (rumpfGier, Kopf); Pitch und
    Roll am Knochen fehlen ganz, und Achsen und Vorzeichen lassen sich nur gegen das Spiel
    pruefen (kein Spielvergleich in diesem Auftrag). Eine halbe Fassung (nur Yaw) waere
    eine Naeherung - deshalb nicht eingebaut.
- **Punkt 3: g_SerenityJediEngineMode** (39887c2e), nur gelesen (eigener PE-Leser,
  `<SP>/zl5/cvar_vorgabe.py`, `cvar_tabelle.py`):
  - MovieDuels-gamex86_64.dll (27.10.2026): `gi.cvar("g_SerenityJediEngineMode", "2",
    CVAR_ARCHIVE)` (lea rdx "2", lea rcx Name, r8d 1); mdgit g_main.cpp:873 hat "0".
  - MovieDuels-SP.x86_64.exe: die cvarTable der Oberflaeche (ui_main.cpp:1906 mdgit) mit
    "0". CL_StartHunkUsers -> CL_InitUI laeuft vor SV_SpawnServer -> SV_InitGameProgs, und
    Cvar_Get (qcommon/cvar.cpp) behaelt den Wert einer schon angelegten Cvar - wirksam
    ist "0". Dazu setzt ui/main.menu (z_zzzz_MD_Jace-Update-9.pk3, "clickToContinueButton-
    FirstStartup") beim ersten Start `seta g_SerenityJediEngineMode 0`; keine Config setzt
    sie. Die Vorgabe 0 in pmanim stimmt also (Stab rueckwaerts gehend BOTH_WALKBACK_STAFF).
  - Falsch war der Kommentar in scene.h (Modus 2, "Cosmetic mode") - und mit ihm die
    Tabelle der Kampfhaltungen: mit 0 nimmt PM_ReadyPoseForSaberAnimLevelNPC die "JKA
    fallback"-Tabelle (BOTH_SABERFAST_STANCE_JKA, ..._SLOW_, _DESANN_, _TAVION_, _DUAL_,
    _STAFF_STANCE_JKA, sonst BOTH_STAND2; alle in MDs _humanoid/animation.cfg). Fuer
    OpenJK gilt der switch in PM_Footsteps (SS_TAVION wie SS_FAST, SS_DESANN BOTH_STAND2) -
    vorher bekamen auch OpenJK-Figuren die Tabelle von MD-Modus 2. Jetzt eine Tabelle
    `saberHaltungFuerStil(stil, md)` fuer Szene und Aufnahme (pmanims eigene ist weg),
    scene.h erklaert die Herkunft. ctest waffentest/pmanim: rot (Bau bzw. 1 Fehler), gruen.
- **Punkt 4: stehender NPC mit Gewehr** (39887c2e): die Szene setzt in der Grundhaltung
  (BOTH_STAND1 ohne Timer, nicht in Bewegung, NPC) die Wahl von PM_Footsteps
  (`pmFootstepsBeine` mit `pmFigurVon`, dieselbe wie die Aufnahme): mit Gewehr BOTH_STAND3,
  CLASS_IMPERIAL BOTH_STAND4 usw., wenn das Modell die Animation hat. ctest waffentest
  "Grundhaltung": rot 1 Fehler, gruen. Die Aufnahme beginnt jetzt in BOTH_STAND3
  (zl5p1gr "Stand BOTH_STAND3").
- **Punkt 5: Klappfeld "Spielregeln"** (8d3c3c78): so breit wie der laengste Eintrag -
  "automatisch" mit jedem erkennbaren Spiel, damit es nicht springt; das Fenster waechst
  mit. zeitleiste1 `BHED_ZL1_NUR=einstellungen` prueft alle 6 Sprachen (Rahmen >=
  FramePadding + Text + Pfeil; das Element-Rechteck ist der Rahmen, ItemAdd meldet NavRect):
  rot zl5p5rot (alle 6 Sprachen FEHLER, der Rahmen war die halbe Breite der
  Pfadfelder), gruen zl5p5gr 8/0 (en 514 >= 507, de 526
  >= 519, zh 435 >= 428, ja 411 >= 404, es 420 >= 413, cs 430 >= 423).
- **Punkt 6: Undo-Text einer Kommentarzeile** (8d3c3c78): `kurzZeile` nennt eine
  Kommentarzeile nach ihrem Text, eine auskommentierte (//(BHVDREM)) "// " und den Befehl.
  zeitleiste1 `BHED_ZL1_NUR=undo`: rot zl5p6rot ("Undo: inserted  (  ); ..."), gruen
  zl5p6gr ("Undo: inserted // print ( "zl5" );  deleted print ( zl5 )").
- **Punkt 7: Handbuchbild h_zl_aufnahme** (662d81a1): D bleibt fuer das Bild gedrueckt,
  neue Pruefung "die Figur laeuft beim Bild (Laufanimation)". Rot im ersten Lauf
  (`BHED_HB_NUR=zeitleiste1`, alle 4 Sprachen 1/1): die gewaehlte Figur stand an der
  Konsole - BOTH_CONSOLE1 mit Timer - und lief beim Aufnehmen gar nicht los. Jetzt wird
  eine Figur mit freien Beinen gewaehlt; gruen in en, de, zh, ja je 2/0 (Beine BOTH_RUN1,
  in Bewegung). Dazu h_einst_vorschau aus dem vollen Bildlauf (je Sprache 6/0) mit dem
  breiteren Klappfeld; beide mit `tools/handbuch/bilder.py` uebernommen.
- **Schlussrunde** (`<SP>/zl5/ende.sh`): sauberer Neubau aller Ziele 0 Fehler, 0 Warnungen;
  ctest 189/189; lint_sources, lint_fassung (rc606), lint_gapp, lint_imgui_context,
  lint_layout, lint_abstand, lint_doppelanzeige, lint_i18n ok; handbuch_sprachen ok.
  Selbsttests nach dem Neubau: zeitleiste1 96/0, zeitleiste 47/0, mission1 98/0, szenen
  26/0, kamera 96/0, missionsbau 95/0, pruefung3 9/0. Modus 1, alle Modi und die
  Kartenpruefung macht der Abschluss-Agent.
- **Gegen das Vorbild geprueft**: PM_AdjustStandAnimForSlope/PM_Footsteps-Aufrufstellen
  (OJ bg_pmove.cpp:8165-8330, MD :14246-14540), PM_TorsoAnimLightsaber und
  PM_SetSaberMove(LS_READY) (OJ bg_panimate.cpp:5160-5505, bg_pmove.cpp:9208-9262; MD
  bg_pmove.cpp:22582 ff., 23274 ff.), PM_ReadyPoseForSaberAnimLevel(NPC) (MD :17504,
  :18789), der Sprint (MD bg_pmove.cpp:15360-15750, PM_HandleSprint, g_active.cpp:8158,
  10151), die Cvar-Vorgaben in der installierten DLL und exe (nur gelesen), Cvar_Get und
  die Startreihenfolge (mdgit qcommon/cvar.cpp, client/cl_main.cpp, server/sv_init.cpp).
  **Nicht** im laufenden Spiel verglichen (kein Spielvergleich in diesem Auftrag).
- **Offen**: die Motion-Bone-Korrektur (siehe Punkt 2); PM_TorsoAnimLightsaber ohne
  .sab readyAnim, Schwertarten ARC/LANCE/TRIDENT, Blocken und die Figurenvarianten von
  g_ActivateAnimationStyle 1.

## Zeitleiste 6

Auftrag `<SP>/auftrag_zeitleiste6.md`, direkt auf karte1 (Y:\karte, ab 1274434d). Skripte und
Spielfotos unter `<SP>/zl6/` (motion.py liest den Motion-Knochen aus einer .gla, spiel.py startet
Movie Duels, faelle.py liest case-Listen aus den Engine-Quellen, Testkarte `karte/zl6raum.map`).

- **Punkt 1: Motion-Bone-Korrektur** (df16af9e, d44de3d9, Handbuch ac74115d). Neu `bhed/rumpfwinkel.h`: der Port von
  CG_G2ClientSpineAngles und CG_G2ClientNeckAngles (OJ cg_players.cpp:2022-2401, MD :2366 ff. mit
  CLASS_DROIDEKA und PM_InLedgeMove) samt G_ClassHasBadBones/G_BoneOrientationsForClass,
  PM_FlippingAnim/PM_SpinningSaberAnim und cg_motionBoneComp 2 (Vorgabe beider Spiele). Die Vorschau
  dreht Kopf, Hals und Rumpf jetzt um die Achsen des Knochens mit Nick, Gier und Roll:
  `g2WinkelMatrix` (G2_Generate_Matrix POSTMULT samt der +180) und `knochenNachMal` (X(b)*M, Teilbaum
  mit X M X^-1; ersetzt rotateBoneSubtreeYaw, die Lider nutzen dieselbe Matrix). Der thoracic mischt
  Rumpf und Blick zur Haelfte ("already been set above, blend them") - vorher addierte die Vorschau
  beides. Der Motion-Bolzen: G2API_GetBoltMatrix ohne Drehung, die Engine-Wurzel ist um 90 Grad
  gedreht (RootMatrix/identityMatrix), Zeilen normiert.
  - Wichtig, erst im Spielvergleich gefunden: PM_SetAnimFinal setzt die **Oberkoerperanimation auch
    auf den Knochen "Motion"** ("special case motion bone - to match up split anims", bg_panimate.cpp:
    4974 OJ, :7081 MD). Die Korrektur gleicht also das Becken der Beine gegen den Motion-Knochen der
    Oberkoerperanimation aus. Mit dem Motion-Knochen der Beine (erste Fassung) lehnte die Vorschau
    einen zurueckgelehnten Rumpf noch weiter (Selbsttest: 51,3 Grad statt aufrecht).
  - ctest rumpfwinkel (neu, 42 Pruefungen), gla (knochenNachMal), pmanim.
  - Selbsttest zeitleiste1 `BHED_ZL1_NUR=rumpf` (drei Figuren mit gehaltenen Beinanimationen und
    BOTH_STAND1 oben, Movie Duels md_am_sith): rot zl6p1rot 6/1 (die Wirbel waren gar nicht gedreht:
    0,00 statt 25,30/25,30/6,33 Grad), gruen zl6p1n 10/0: je Wirbel gleich der Matrix aus
    G2_Generate_Matrix (groesste Abweichung 0,00004), BOTH_SIT2HEADTILTLSTART mit Korrektur 7,0 Grad
    gegen die Senkrechte, ohne 35,5 (zurueckgelehnt).
  - **Im Spiel geprueft** (Movie Duels SP, fs_game MD, eigene Karte zl6raum mit drei
    NPC_Stormtrooper, Skript per runscript, Kamera per Skript, Fotos per screenshot_tga):
    cg_motionBoneComp 2 - die sitzende Figur sitzt aufrecht; cg_motionBoneComp 0 - sie lehnt sich
    zurueck. Die Vorschau zeigt mit derselben Aufstellung und Kamera dasselbe (Fotos zl6_rumpf_vorn/
    _seite im Selbsttest, `<SP>/zl6/tga_seite_2.png`, `tga_seite_0.png`, `vorschau_seite.png`).
    Original-JKA nicht: Steam lief nicht (ActiveUser 0).
- **Punkt 2: PM_TorsoAnimLightsaber, Luecken** (9267dbbb). PmFigur kennt jetzt die
  Schwerter (Typ, readyAnim beider, zwei Schwerter, zweites an, Stab, singleBladeStyle, zweite Klinge,
  Blocken, MD animationstyle) und PmTorsoLage die Eingabe (cmd), ps->speed 0 (MD), PMF_DUCKED und
  Rolle/Liegen. Neu und gegen die Quellen:
  - TorsoAnimLightsaber: springen/landen/Rad/Salto -> Oberkoerper folgt; Laufen und (MD: die lange
    Liste bg_panimate.cpp:9244 samt BOTH_WALK1) Gehen mit Stab/zwei Klingen nur ohne Blocken.
  - LS_READY OpenJK: Blocken BOTH_P1_S1_T_ (Klaue/zweites Schwert BOTH_INAIR1), readyAnim[0/1],
    SABER_ARC -> BOTH_SABERFAST_STANCE, zwei Schwerter -> DUAL, Stab (ohne singleBladeStyle oder mit
    zweiter Klinge) -> STAFF, LANCE/TRIDENT -> BOTH_STAND1, sonst die Haltung des Stils.
  - LS_READY Movie Duels (NPC, nie MBF_HOLDINGBLOCK): Blocken mit Klaue/zwei/Stab/ARC die Haltung,
    sonst animToUse BOTH_STAND2; readyAnim; sonst PM_ReadyPoseForSaberAnimLevelNPC (geduckt
    _Ducked) - mit g_ActivateAnimationStyle 1 die Figurenvarianten fuer SS_DUAL/SS_STAFF und
    geduckt (Galen, MD, KOTOR, DF2, Kampfdroide, Cal, Grievous, Palpatine, Maul; Anakin, Vader,
    Obi-Wan ...).
  - Angleich an Gehen/Laufen (OJ-Liste, MD 52 Haltungen und 162 Beinanimationen) und SETANIM_BOTH
    ohne Eingabe (MD auch bei Tempo 0) mit ruhigen Beinen (kein Salto, Sprung, Schmerz, Sondersprung,
    Hang, nicht geduckt), bei Drehschwung (MD auch der Beine) und beim Blocken.
  - Falsch war in Zeitleiste 5: "MD setzt die Haltung ueber beide Knochen, wenn die Beine in einer
    Kampfhaltung stehen, und laesst einen laufenden Oberkoerper stehen" - das ist der LS_PUTAWAY-Zweig
    (bg_pmove.cpp:23086-23309 mdgit), nicht LS_READY; und MD gleicht beim Gehen an (:23405 ff.).
  - .sab `readyAnim` (Saber_ParseReadyAnim) und NPC `animationstyle` (Kurzname oder CS_-Name) werden
    gelesen. Die Szene nimmt fuer einen stehenden NPC mit Klinge dieselbe Wahl (saberGrundhaltung).
  - ctest pmanim: rot (Bau: neue Schnittstelle), gruen mit 39 Faellen gegen die Formeln (davon 2 Lesefaelle).
  - g_ActivateAnimationStyle bleibt in der Vorschau 0: so steht es in der installierten DLL
    (g_activateanimationstyle "0"), und nur die MP-Einstellungsmenues bieten es an.
- **Fenster** (shank 30.10.): das Spielfenster wird sofort rechts neben den virtuellen Bildschirm
  geschoben (SetWindowPos HWND_BOTTOM, ohne Aktivierung; vid_xpos allein half nicht, Python
  DPI-bewusst), Fotos per screenshot_tga. jkstudio-Selbsttests lagen schon ganz hinten; gemessen
  (`<SP>/zl6/vordergrund.py`, alle 50 ms): kein Fenster im Vordergrund, nie oberstes Fenster. Der
  Vollbild-Umschalter legt ein Selbsttestfenster jetzt ebenfalls nach hinten (HWND_BOTTOM statt
  HWND_TOP).
- **Gegen das Vorbild geprueft**: die Formeln oben gegen OpenJK und mdgit (Zeilen genannt); die
  Rumpfkorrektur im laufenden Movie Duels (Foto). **Nicht** im Spiel: Punkt 2 (nur ctest gegen die
  Formeln), Original-JKA (Steam aus), cg_motionBoneComp 3 (nicht Vorgabe, nicht portiert).
- **Schlussrunde** (`<SP>/zl6/ende.sh`): sauberer Neubau aller Ziele 0 Fehler, 0 Warnungen; ctest
  190/190; lint_sources, lint_fassung (rc606), lint_gapp, lint_imgui_context, lint_layout,
  lint_abstand, lint_doppelanzeige, lint_i18n ok; handbuch_sprachen ok. Selbsttests nach dem Neubau:
  zeitleiste1 100/0, zeitleiste 47/0, szenen 26/0, pruefung3 9/0. Waehrenddessen war kein
  jkstudio-Fenster oberstes Fenster (vordergrund.py, keine Meldung). Spielordner unveraendert
  (Dateiliste vorher/nachher gleich), keine Sperrdatei, kein Spielprozess.
- **Offen**: CG_UpdateLookAngles (das Nachziehen des Blicks) und der Nickwinkel des Blicks bleiben
  wie bisher; Reiten (G_IsRidingVehicle) und die Wendeanimation der Huefte (cg_turnAnims 0) nicht
  abgebildet; g_ActivateAnimationStyle 1 (MP-Einstellung) waere eine Einstellung der Vorschau.

## Changelog

## English

- Preview: when legs and upper body play different animations, the spine turns like the game (motion bone correction, cg_motionBoneComp 2) - with pitch, yaw and roll about the bones' own axes; head and torso now also turn about their bones instead of the vertical. Checked against Movie Duels.
- Lightsaber stance like the game: the readyAnim of the .sab, arc saber, two sabers, staff, lance and trident, blocking; standing still without input the stance covers the whole body, walking it follows the legs (Movie Duels with its own lists and the character styles of g_ActivateAnimationStyle).
- Recording a walk: standing on a slope or a step, the figure puts its feet like the game (the slope leg animations); with a lightsaber its upper body takes the ready stance or follows the legs as the game decides.
- Preview: an NPC with a gun stands in BOTH_STAND3 and an Imperial officer in BOTH_STAND4 - the standing pose the game picks.
- Movie Duels: a figure with a lit saber stands in the stances the installed game really uses (the "_JKA" stances, g_SerenityJediEngineMode is 0); under OpenJK the Tavion style stands like the fast style and the Desann style in BOTH_STAND2, as in the game.
- Preferences: the "Game rules" list is wide enough for "Automatic (detected: ...)" in every language.
- Undo/redo: commenting out a line is now shown as "// <command>" instead of "( )".
- Map editor: decals of effects in the real-time view live in the game's mark store (fade out and give way like the game's marks).
- Recording a walk: the figure's model moves as in the game - legs run, walk and step backwards as PM_Footsteps picks them (with the weapon, saber and class variants of Jedi Academy and Movie Duels), turn up to 60° toward the direction it moves while the upper body keeps looking ahead, blend from one animation to the next, and a weapon stance or held animation of the upper body stays.
- wait ( "name" ) waits for the task "name" of the same figure, as in the game: right away if the task has not been started with do yet, never if there is no such task; the timeline explains it as waiting for the task, not for a signal.
- Preferences: Ok and Cancel always stay in view - with a large interface on a low screen only the settings scroll.
- Record a walk: select a figure, press "Record" in the timeline's transport bar and steer it in the preview with the game's movement (W/S/A/D, Shift walks, Q/E turns); Enter writes walk targets the way scripts let an NPC walk - one ref_tag and one task with SET_NAVGOAL plus dowait per point, only where the path needs one (adjustable tolerance, walls, gait changes), with SET_WALKING/SET_RUNNING - as one undo step; Esc cancels. While recording, the figure's model runs or walks along and the hint stays clear of the toolbar.
- Load mission: the list opens at the top (it kept the scroll position of the previous list, so the first missions of another game could be out of view).
- Subtitles in the preview picture exactly as the game draws them: line breaks, two lines at a time for as long as the sound, in the lower black bar, pages turning on the game's next frame, colour from SET_CAPTIONTEXTCOLOR, Movie Duels smaller (0.66, the default of the installed game); a "Subtitles" switch offers the game's choices with the game's default (none in Jedi Academy and Movie Duels, in cinematics in Jedi Outcast).
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

- Vorschau: spielen Beine und Oberkörper verschiedene Animationen, dreht sich die Wirbelsäule wie im Spiel (Motion-Bone-Korrektur, cg_motionBoneComp 2) - mit Nick, Gier und Roll um die Achsen der Knochen; Kopf und Rumpf drehen jetzt ebenfalls um ihre Knochen statt um die Senkrechte. Gegen Movie Duels geprüft.
- Kampfhaltung mit Lichtschwert wie im Spiel: readyAnim der .sab, Bogenklinge, zwei Schwerter, Stab, Lanze und Dreizack, Blocken; im Stand ohne Eingabe gilt die Haltung für den ganzen Körper, beim Gehen folgt sie den Beinen (Movie Duels mit eigenen Listen und den Figurenstilen von g_ActivateAnimationStyle).
- Lauf aufnehmen: steht die Figur an einem Hang oder auf einer Stufe, stellt sie die Füße wie im Spiel (die Hang-Beinanimationen); mit Lichtschwert nimmt ihr Oberkörper die Kampfhaltung ein oder folgt den Beinen, wie das Spiel es entscheidet.
- Vorschau: ein NPC mit Schusswaffe steht in BOTH_STAND3, ein imperialer Offizier in BOTH_STAND4 - die Grundhaltung, die das Spiel wählt.
- Movie Duels: eine Figur mit brennender Klinge steht in den Haltungen, die das installierte Spiel wirklich nimmt (die "_JKA"-Haltungen, g_SerenityJediEngineMode ist 0); unter OpenJK steht der Tavion-Stil wie der schnelle und der Desann-Stil in BOTH_STAND2, wie im Spiel.
- Einstellungen: die Liste "Spielregeln" ist in jeder Sprache breit genug für "Automatisch (erkannt: ...)".
- Rückgängig/Wiederholen: das Auskommentieren einer Zeile heißt jetzt "// <Befehl>" statt "( )".
- Karten-Editor: Decals der Effekte liegen in der Echtzeit-Ansicht im Markenspeicher des Spiels (sie blenden aus und weichen wie die Marken des Spiels).
- Lauf aufnehmen: das Modell der Figur bewegt sich wie im Spiel - die Beine laufen, gehen und gehen rückwärts, wie PM_Footsteps es wählt (mit den Waffen-, Lichtschwert- und Klassenvarianten von Jedi Academy und Movie Duels), drehen sich bis 60° in die Laufrichtung, während der Oberkörper nach vorn schaut, blenden von einer Animation in die nächste, und eine Waffenhaltung oder gehaltene Animation des Oberkörpers bleibt.
- wait ( "name" ) wartet wie im Spiel auf die Aufgabe "name" derselben Figur: sofort weiter, wenn die Aufgabe noch nicht mit do gestartet ist, nie, wenn es sie nicht gibt; die Zeitleiste erklärt es als Warten auf die Aufgabe, nicht auf ein Signal.
- Einstellungen: Ok und Abbrechen bleiben immer im Bild - bei großer Oberfläche auf einem niedrigen Bildschirm rollen nur die Einstellungen.
- Einen Lauf aufnehmen: eine Figur wählen, in der Transportleiste der Zeitleiste "Aufnehmen" drücken und sie in der Vorschau mit der Bewegung des Spiels steuern (W/S/A/D, Umschalt geht, Q/E dreht); Enter schreibt Laufziele so, wie Skripte einen NPC laufen lassen - je Punkt ein ref_tag und eine Aufgabe mit SET_NAVGOAL samt dowait, nur wo der Weg einen braucht (einstellbare Toleranz, Wände, Gangwechsel), mit SET_WALKING/SET_RUNNING - als ein Rückgängig-Schritt; Esc bricht ab. Beim Aufnehmen läuft oder geht das Modell der Figur mit, der Hinweis bleibt frei von der Werkzeugleiste.
- Mission laden: die Liste geht oben auf (sie stand so weit gerollt wie die vorige, die ersten Missionen eines anderen Spiels lagen dann außerhalb).
- Untertitel im Vorschau-Bild genau so, wie das Spiel sie zeichnet: Zeilenumbruch, je zwei Zeilen so lange wie der Klang, im unteren schwarzen Balken, Seitenwechsel im nächsten Bild des Spiels, Farbe aus SET_CAPTIONTEXTCOLOR, Movie Duels kleiner (0,66, die Vorgabe des installierten Spiels); ein Schalter "Untertitel" bietet die Wahl des Spiels mit dessen Vorgabe (keine in Jedi Academy und Movie Duels, in Zwischensequenzen in Jedi Outcast).
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

- 预览:双腿和上半身播放不同动画时,脊柱像游戏一样转动(Motion 骨骼修正,cg_motionBoneComp 2)——绕骨骼自身的轴进行俯仰、偏航和翻滚;头部和躯干现在也绕各自骨骼转动,而不是绕竖直轴。已与 Movie Duels 对照。
- 光剑架势与游戏一致:.sab 的 readyAnim、弧形剑、双剑、双刃剑、长枪和三叉戟、格挡;无输入静止时架势作用于全身,行走时跟随双腿(Movie Duels 使用自己的列表和 g_ActivateAnimationStyle 的角色风格)。
- 录制行走：角色站在斜坡或台阶上时会像游戏中一样放脚（斜坡腿部动画）；持光剑时上半身按游戏的判断摆出战斗姿势或跟随双腿。
- 预览：持枪的 NPC 以 BOTH_STAND3 站立，帝国军官以 BOTH_STAND4 站立——即游戏选择的站姿。
- Movie Duels：点亮光剑的角色使用已安装游戏实际采用的姿势（"_JKA" 姿势，g_SerenityJediEngineMode 为 0）；在 OpenJK 下 Tavion 风格与快速风格站姿相同，Desann 风格为 BOTH_STAND2，与游戏一致。
- 设置："游戏规则"列表在每种语言下都足够宽，能完整显示"自动（检测到：...）"。
- 撤销/重做：注释掉一行现在显示为"// <命令>"，而不是"( )"。
- 地图编辑器：实时视图中特效的贴花存放在游戏的标记存储中（像游戏的标记一样淡出并让位）。
- 录制行走：角色模型的动作与游戏一致——双腿按 PM_Footsteps 的选择奔跑、行走和后退（包括 Jedi Academy 和 Movie Duels 中武器、光剑和类别的变体），朝移动方向最多转 60°，同时上身保持朝前；动画之间平滑过渡，上身的武器姿势或保持的动画会保留。
- wait ( "name" ) 与游戏一致，等待同一角色的任务 "name"：若该任务尚未用 do 启动则立即继续，若不存在该任务则永远等待；时间轴将其解释为等待任务，而不是等待信号。
- 首选项：“确定”和“取消”始终可见——在低屏幕上使用大界面时只有设置部分滚动。
- 录制行走：选择一个角色，在时间轴的传输栏中按“录制”，并在预览中用游戏的移动方式操控它（W/S/A/D，Shift 走路，Q/E 转身）；Enter 按脚本让 NPC 行走的方式写入行走目标——每个点一个 ref_tag 和一个带 SET_NAVGOAL 的 task 加 dowait，只在路径需要的地方（可调容差、墙、步态变化），并带 SET_WALKING/SET_RUNNING——作为一个撤销步骤；Esc 取消。录制时角色的模型会跟着奔跑或行走，提示不会被工具栏遮住。
- 加载任务：列表从顶部打开（以前保留上一个列表的滚动位置，另一个游戏的前几个任务可能在视野之外）。
- 预览画面中的字幕与游戏绘制的完全一致：换行、每次两行且持续声音的时长、显示在下方黑条中、在游戏的下一帧翻页、颜色来自 SET_CAPTIONTEXTCOLOR、Movie Duels 较小（0.66，已安装游戏的默认值）；“字幕”开关提供游戏的选项及其默认值（Jedi Academy 和 Movie Duels 不显示，Jedi Outcast 在过场动画中显示）。
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

- プレビュー:脚と上半身が別のアニメーションを再生すると、ゲームと同じように背骨が回ります(Motion ボーン補正、cg_motionBoneComp 2)。ボーン自身の軸でピッチ・ヨー・ロール。頭と胴体も鉛直軸ではなく各ボーンの軸で回ります。Movie Duels と照合済み。
- ライトセーバーの構えをゲームどおりに:.sab の readyAnim、アーク、二刀、スタッフ、ランス、トライデント、ブロック。入力なしで立つと構えは全身に、歩くと脚に従います(Movie Duels は独自のリストと g_ActivateAnimationStyle のキャラクタースタイル)。
- 歩きの記録：坂や段の上に立つと、キャラクターはゲームと同じように足を置きます（坂用の脚アニメーション）。ライトセーバーを持つと、上半身はゲームの判断どおり構えを取るか脚に従います。
- プレビュー：銃を持つ NPC は BOTH_STAND3、帝国士官は BOTH_STAND4 で立ちます――ゲームが選ぶ立ち姿です。
- Movie Duels：光る刃を持つキャラクターは、インストール済みのゲームが実際に使う構え（"_JKA" の構え、g_SerenityJediEngineMode は 0）で立ちます。OpenJK では Tavion スタイルは高速スタイルと同じ、Desann スタイルは BOTH_STAND2 で、ゲームと同じです。
- 設定：「ゲームルール」の一覧はどの言語でも「自動（検出: ...）」が収まる幅になりました。
- 元に戻す/やり直し：行のコメント化は「( )」ではなく「// <コマンド>」と表示されます。
- マップエディター：リアルタイム表示のエフェクトのデカールはゲームのマーク置き場に入ります（ゲームのマークと同じように薄れて入れ替わります）。
- 歩きの記録：キャラクターのモデルがゲームと同じように動きます。脚は PM_Footsteps が選ぶとおりに走る・歩く・後ろへ歩き（Jedi Academy と Movie Duels の武器・ライトセーバー・クラスごとの違いを含む）、上半身が前を向いたまま進行方向へ最大 60° 向きを変え、アニメーションの間はブレンドし、上半身の武器の構えや保持されたアニメーションは残ります。
- wait ( "name" ) はゲームと同じく同じキャラクターのタスク "name" を待ちます。タスクがまだ do で開始されていなければすぐに続行し、そのタスクがなければ永久に待ちます。タイムラインはシグナルではなくタスクの待機として説明します。
- 設定：OK とキャンセルは常に表示されます。低い画面で大きなインターフェースを使うと、設定部分だけがスクロールします。
- 歩きの記録：キャラクターを選び、タイムラインのトランスポートバーで「記録」を押して、プレビューでゲームの移動のまま操作します（W/S/A/D、Shift で歩く、Q/E で向き）。Enter で、スクリプトが NPC を歩かせる方法で移動目標を書き込みます。点ごとに ref_tag と SET_NAVGOAL の task と dowait を、経路が必要とする所だけ（調整できる許容差、壁、歩き方の変化）、SET_WALKING/SET_RUNNING 付きで、元に戻す 1 回分として。Esc でキャンセル。記録中はキャラクターのモデルが一緒に走るか歩き、ヒントはツールバーに隠れません。
- ミッションの読み込み：リストは先頭から開きます（以前は前のリストのスクロール位置のままで、別のゲームの最初のミッションが見えないことがありました）。
- プレビュー画像の字幕をゲームの描き方どおりに表示：改行、音の長さだけ 2 行ずつ、下の黒帯に、ページはゲームの次のフレームで切り替わり、色は SET_CAPTIONTEXTCOLOR、Movie Duels は小さめ（0.66、インストール済みゲームの既定値）。「字幕」スイッチでゲームの選択肢とその既定値（Jedi Academy と Movie Duels はなし、Jedi Outcast はムービー中）を選べます。
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
