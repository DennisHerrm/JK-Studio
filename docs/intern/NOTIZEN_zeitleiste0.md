# Notizen zeitleiste0: Erkundung fuer die neue Zeitleiste im Mission-Tab

Zweig `zeitleiste0` ab karte1 39a2c83e, Worktree `Y:\mission`, Bau
`Y:/mission/build-mission`. Nur erkunden und belegen - das Programmverhalten
ist unveraendert. Einzige Codeaenderung: der Selbsttest-Modus
`BHED_EDITORTEST=zeitleiste0` (`gui/selbsttest_zeitleiste0.inc`) schreibt
echte Szenendaten als JSON und Kamerabilder fuer den klickbaren Entwurf
(`BHED_ZL0_AUS`), dazu die Deklaration von `useSoundFor` in
`gui/app_internal.h`. Keine neuen Programmtexte, darum keine
`tools/i18n_zeitleiste0.py`; kein Handbuch (keine Funktionsaenderung). Kein
Merge nach karte1.

Quellen der Spiel-Belege: OpenJK `code/game/Q3_Interface.cpp` (Teilkopie
`<SP>\ojk\oj`) und die Movie-Duels-Quellen `<SP>\mdgit\code` (OpenJK-Stand;
dort liegen `cgame/cg_camera.cpp`, `cgame/cg_text.cpp`,
`cgame/cg_servercmds.cpp`, `icarus/*`). Zeilennummern aus diesen Kopien.

## 1. Bestand

### Dateien und Datenfluss

- Modell: `include/bhed/timeline.h`, `src/timeline.cpp` (`buildTimeline`):
  laeuft durch ein Skript, je `affect`-Ziel eine `TimelineTrack`, darin
  `TimelineEvent` {startMs, endMs, path, fremd, label, sound, kanal, target,
  Kind: Camera/Sound/Wait/Set/Move/Other/Use/Saber}. Dauer haben nur wait,
  waitsignal, camera (letztes Argument), move/rotate. sound, set (auch
  SET_ANIM_*, SET_NAVGOAL), use, signal sind Ereignisse ohne Dauer.
- Zeiten: NICHT aus der Skript-Analyse, sondern aus dem ICARUS-Nachbau
  (`include/bhed/ablauf.h`, `src/ablauf.cpp`, `simuliereAblauf` /
  `simuliereAblaufMitKoerpern`): fuehrt alle Skripte Bild fuer Bild (50 ms)
  aus wie CSequencer/CTaskManager und schreibt ein FLACHES Skript (Traeger
  oben, je Entity ein affect-Block, dazwischen waits mit Herkunft).
  `prepareScene` (`gui/app_view3d.cpp:5735`) baut daraus `buildCameraTrack`
  (`src/camtrack.cpp`), `buildTimeline` (Zeile 5786, nach dem Koerperlauf
  noch einmal 5947) und die Szene (`src/scene.cpp`, Actor/ActorStep mit
  echten Dauern: Laeufe, Animationen bis Haltezeit-Ende, Flug, ROFF);
  `pfadeAufsSkript` (5580) biegt die Pfade aufs offene Skript zurueck
  (Befehle anderer Skripte: `fremd`, leerer Pfad).
- Zeichnen: `drawTimeline` in `gui/app_view3d_bedienung.inc:858-2600`
  (eine Funktion, rund 1750 Zeilen): Transportleiste 964-1223, Lineal 1405,
  Uebersichtsbalken 1594 (nur Kamera, nur bei Spuren aus), Kamerazeilen je
  Art 1829-2152, Skriptzeile und Figurengruppen 2390-2503 (seit rc597 je
  Figur eine Zusammenfassungszeile, aufgeklappt sechs Unterspuren
  Bewegung/Animation/Blick/Ton/Zustand/Ablauf, Zuordnung `kategorieVon`
  1754 per Textsuche im Label), Laufbaender 2296-2389.
- Zweite, unabhaengige Spurenansicht: `drawSpurenAnsicht` / `spurenBauen`
  (`gui/app_ansichten.cpp:1070-1446`, Feldansicht "Spuren"): Einstellungen
  MOVE->MOVE, Dialogspur, Figuren, Skript - eigenes Modell, eigene Regeln
  (Dialogbloecke fest mindestens 2,5 s statt der echten Klanglaenge, 1144).
- Zurueckschreiben: Kante ziehen (`kantenGriff` 649-721) ersetzt das
  Dauer-Argument per `Document::replaceAt` (bei wait 50 ms abgezogen, weil
  die Leiste den Nachbau-Takt zeigt); Block ziehen (`verschiebeImAblauf`
  725-820) aendert das wait DAVOR (Ripple) bzw. mit Alt auch das danach
  (`replaceMany`) oder fuegt ein wait ein (`insertBefore`). Danach
  `rebuildTree`, `camTrackValid=false` -> naechstes Bild `prepareScene`.
- Undo: `Document` speichert je Schritt das ganze Skript (`edit.h` Step);
  ein Ziehen = ein Schritt. Ausnahme (gelesen, nicht nachgestellt): Alt-
  Verschieben ohne wait davor, mit wait danach = `insertBefore` +
  `replaceAt` = zwei Schritte (725-811); `fasseZusammen` waere da.
- Leistung: `prepareScene` nur nach Aenderungen (Zeit im Protokoll
  "Szene vorbereitet"); `drawTimeline` je Bild ueber alle Ereignisse mehrfach
  (Kategorie per Grossschreib-Textsuche, Ueberlappung O(n^2) je
  Kamerazeile, Beschriftung per istringstream) - bei den Testszenen
  unkritisch (Messung unten).

### Was die Beobachtungen bedeuten

- Unbeschriftete Striche: jedes Ereignis ohne Dauer (sound, set, use,
  signal, SET_ANIM, SET_NAVGOAL) wird auf 3 Bildpunkte gezogen (2226-2229),
  Text nur ab zwei Schriftbreiten (2247). In der ZUGEKLAPPTEN Figurenzeile
  (Vorgabe) steht nie Text (2452-2483) - dort ist jedes Ereignis ein Strich
  in Kategorienfarbe. Die Klanglaenge ist bekannt (`soundLengthMs`), wird
  aber nicht in endMs uebernommen; Animations- und Laufdauern stehen nur in
  der Szene (ActorStep), nicht im TimelineEvent.
- Rote Ueberlappungen (2023-2053, 2131-2146): ein MOVE/PAN/ZOOM/ROLL
  beginnt, bevor der vorige derselben Gruppe fertig ist. Das ist ein echter
  Hinweis aufs Skript: CGCam_Update uebernimmt Ort/Winkel/FOV erst am Ende
  ("does not actually move the camera until the movement time is done",
  cg_camera.cpp:1200-1225), ein neuer Befehl rechnet vom alten ANFANG - die
  Kamera springt zurueck. Erklaert wird das nur im Tooltip.
- "0 0 0"-Kaestchen bei Fade: jeder Kamerablock zeigt `args[1]` (2056-2087).
  Bei `camera ( FADE, <r g b>, a, <r g b>, a, dauer )` ist das die
  STARTFARBE - bei einer Aufblende aus Schwarz "0 0 0".
- Weitere Funde: `camera ( PATH )` (Kamera-ROFF, CamSegment::Kind::Pfad)
  hat KEINE Zeile (camZeilen 1846-1854, camZeilenZahl 1728-1740); ENABLE/
  DISABLE (Balken, Beginn/Ende der Sequenz) sind unsichtbar; ruhige Spuren
  sind vorgegeben ausgeblendet; das Panel hat 22 % der Fensterhoehe
  (`timelineFrac` 0.22) fuer Transportleiste, Lineal, bis zu 8
  Kamerazeilen, Skript und alle Figuren.

### Gemessen (Export-Lauf, Vorgabe-Layout, Schrift 24 px, Fenster 2325 px Zeitachse)

| | academy1/intro (JKA) | md_am_sith/intro2_sith (MD) |
|---|---|---|
| Laenge | 91,55 s (Kartenwechsel yavin2) | 62,65 s |
| Spuren (affect-Ziele) | 19 (0 ruhig) | 30 (25 ruhig, ausgeblendet) |
| Kameraabschnitte / rot | 43 / 2 | 40 / 11 |
| gezeichnete Ereignisse | 356 | 183 |
| davon ohne Beschriftung | **356** (341 schmaler als 3 px, 320 in Zusammenfassungszeilen) | **183** (172 / 143) |
| Panelhoehe / Zeilenhoehe | 141 px / 30 px | 141 px / 30 px |
| Zeilen der Leiste | 24 | 10 |
| Klaenge / mit Laenge / mit Untertitel | 15 / 15 / 15 | 9 / 9 / 5 (die 4 ohne sind Schiffsklaenge) |
| prepareScene nach einer Aenderung | 175 ms (erstes Mal 728 ms mit Modellen) | 2834 / 6513 ms beim Laden (unter Last) |

Im Bild der heutigen Leiste (`bilder/*_heute.png`) sind nur die
Kamerazeilen zu sehen (Gruppe, Fahrt, Schwenk, Zoom, Fade); Skript, Figuren,
Dialog und das Schiff liegen unterhalb und sind nur per Rollen erreichbar -
das ist "man sieht zum Teil gar nichts". Alle Figurenereignisse sind
unbeschriftete Striche. Die beiden roten Stellen in academy1 sind MOVE und
PAN "cam3b" (6,15-15,15 s), in die "lukewalk2" bei 13,05 s hineinlaeuft. Jedes
Ziehen einer Kante kostet in academy1 einen Neuaufbau von rund 175 ms
(gemessen unter Last durch drei andere Laeufe).

## 2. Verfuegbare Daten je Zeitpunkt und Luecken

| Bereich | vorhanden (Quelle) | fehlt fuer die neuen Spuren | Aufwand |
|---|---|---|---|
| Kamera MOVE/PAN/ZOOM/ROLL/FADE/SHAKE/FOLLOW/PATH, ENABLE/DISABLE | CamSegment je Befehl (von/nach, Farben, Gruppe, Tempo), `CameraTrack::at(ms)` Ort/Winkel/FOV/Fade/Balken | camera TRACK und DISTANCE werden nicht nachgebaut (kein Fall in camtrack.cpp); SHAKE nur Staerke (Zufall, siehe 3) | TRACK/DISTANCE: mittel (CGCam_TrackUpdate 862-990 portieren, path_corner-Kette) |
| Figuren: Laufziele, Wege | ActorStep WalkTo (von/nach, LaufPlan, Teilwege, ankunftMs), Glide/Turn (move/rotate), Roff, Teleport; `Actor::at(ms)` | Zuordnung Ereignis -> Schritt nur ueber Pfad (fremde Skripte: leerer Pfad) | klein |
| Animationen Ober/Unterkoerper | ActorStep Anim mit Teil (both/upper/lower) und Ende = Animationslaenge (Haltezeit), `ActorState::animation/torsoAnimation` | nichts | - |
| Gesicht | Mundlauf aus der Lautstaerke (FACE_TALK), SET_FACE* in app_view3d.cpp | kein eigenes Ereignis im Modell | klein |
| Waffen, Klinge, Sichtbarkeit | ActorStep Waffe/Saber/Show/Hide/Gone, ActorState waffe/saberActive/visible | nichts | - |
| Mover, Tueren | `MoverSim::mover()` Bahnstuecke (ab, Dauer, von/nach, linear), Sichtbarkeit, `tuerZustand`, `klaenge()` | nicht in der Zeitleiste | klein (Daten da) |
| Sounds | Kanal, Datei (TimelineEvent), Laenge `soundLengthMs`, Abtastwerte `soundFor` (Wellenform) | Laenge nicht in endMs; Untertitel nirgends | klein: StringEd-Leser gibt es (`bhed::menue::Texte::ladeStr`, Menue-Editor), Schluessel wie CG_CaptionText (im Export geprueft); Jedi Outcast: StripEd `.sp` ueber `ladeSp`, Schluessel dort nicht geprueft |
| Musik | `use` -> target_play_music (`useSoundFor`, auch ueber relay/delay/trigger: `Ablauf::benutzt`), `weltMusik()` | Laenge/Schleife nicht im Modell | klein |
| wait / waitsignal / dowait / signal / task / do | waits mit Herkunft (Befehl + Zeile), Signalsender im Modell | WELCHER Befehl ein dowait zuletzt aufhielt (Nachbau weiss es intern, Eintrag hat kein Feld); haengende waits (ende -1) nur als Hinweistext | mittel |
| affect / run / Skripte | `Ablauf::skripte`, `skriptBeginn`, Herkunft je Eintrag | - | - |
| Kartenwechsel, Missionsende | `kartenwechselMs/-Karte`, `missionFailedMs`, `menueMs`, `skripteStoppMs` | nicht als Ereignis gezeichnet | klein |
| Fehler/Hinweise | `Ablauf::hinweise` (Text ohne Zeit), `Timeline::notes` (Zeile) | Zeitpunkt der Hinweise | klein |

## 3. Grenzen des Spiels (nichts anbieten, was im Spiel anders aussieht)

- Takt: ICARUS laeuft in Serverbildern zu 50 ms (sv_fps 20); ein wait ist
  erst im Bild NACH seinem Ablauf fertig (`TaskManager.cpp:1100`, siehe
  ablauf.h) - Raster 50 ms, wait(1000) dauert 1050 ms.
- Kamera: MOVE/PAN/ZOOM/ROLL/FADE interpolieren LINEAR
  (`cg_camera.cpp` CGCam_Update 1118-1230, CGCam_UpdateFade 1013-1036). Keine
  Ease-Kurven, keine Tangenten: Kurven nur als Rampe anzeigen, bearbeitbar
  sind Zielwert und Dauer. Ausnahme: `fovaccel` aus einer camera-PATH-ROFF-
  Notiz (CGCam_ZoomAccel 419). Ein neuer Befehl derselben Art vor dem Ende
  des alten springt zurueck (siehe 1). ROLL mit Dauer verdoppelt Nicken/
  Gieren (Raven-Fehler, camtrack.cpp ROLL). SHAKE: Zufall (Q_flrand) mit
  linear fallender Staerke (CGCam_UpdateShake 1353-1392) - nicht bildgenau
  vorhersagbar. FOLLOW/TRACK/DISTANCE: Tempo + initLerp, keine Schluessel.
  Befehle: Q3_Interface CameraPan..CameraShake (MD 10743-10814).
- Mover (move/rotate): TR_NONLINEAR_STOP (Sinus-Ein/Ausschwingen), linear
  nur mit alt_fire; Dauer 0 wird 1 ms (OpenJK Q3_Interface.cpp Lerp2Pos
  8034-8130, Lerp2Angles 8148).
- Animationen: SET_ANIM_UPPER/LOWER/BOTH = NPC_SetAnim mit
  RESTART|HOLD|OVERRIDE (MD Q3_Interface.cpp 2213-2229), Ueberblendung fest
  100 ms (SETANIM_BLEND_DEFAULT, bg_public.h:338), Haltezeit = Laenge oder
  SET_ANIM_HOLDTIME_* (OpenJK 2060, 8368-8387). Nur zwei Ebenen (Torso,
  Beine) plus Gesicht; keine Gewichte, kein Start mitten in der Animation,
  kein Tempo pro Befehl, keine eigenen Knochen-Schluessel.
- Laufen: SET_NAVGOAL auf Marken (waypoint_navgoal), Weg sucht die NPC-
  Navigation (A*), Tempo aus der .npc, SET_WALKING/RUNNING. Kein frei
  gezeichneter Weg, keine Zeitvorgabe pro Strecke.
- Klang: `sound ( KANAL, datei )`; CHAN_VOICE* setzt TID_CHAN_VOICE (dowait
  wartet bis Ende), PlayIcarusSound MD 8449 ff.; Untertitel per Servercommand
  "ct" nur mit g_subtitles bzw. in der Kamera (8486-8510), Text
  `<LEVEL>_<dateiname>` im StringEd-Paket (cg_text.cpp 199-260,
  cg_servercmds.cpp 44-104). Keine Lautstaerke- oder Blendkurven per
  Skript; Musik nur per use auf target_play_music (eine Musik, Intro+
  Schleife).
- Fade: Quell- und Zielfarbe RGBA, linear. Balken (ENABLE/DISABLE) mit
  fester Dauer.

## 4. Machbarkeit der Ideen

| Idee | Bewertung | Grundlage |
|---|---|---|
| Zwei Ebenen Uebersicht/Detail (SFM) | geht, mittel | reine Oberflaeche; Einstellungen MOVE->MOVE baut `spurenBauen` schon, Detail = heutige Spuren |
| Spuren je Darsteller, Zusammenfassung, Halte-Balken (Dope Sheet) | geht, klein-mittel | rc597-Gruppen vorhanden; Haltedauer aus ActorStep (Anim-Ende, Lauf bis Ankunft), Zuordnung ueber Pfad wie `szenenEnde` in spurenBauen |
| Clips mit Unterbahnen statt roter Ueberlappung | geht, klein | Ueberlappung bleibt als Warnung (Spielfehler!), aber als Text am Clip ("springt zurueck") + Unterbahn; Ereignisse ohne Dauer per Intervall-Packen |
| Ereignis-Symbole statt Striche | geht, klein | UiIcon vorhanden (Ton, Animieren, Zielen, Pfad ...); Momente als Raute, Dauer als Balken |
| Dialogspur mit Untertiteln | geht, klein | `soundLengthMs`, `menue::Texte::ladeStr`, Schluessel wie CG_CaptionText - im Export geprueft (Trefferzahl unten) |
| Sound-/Musikspuren mit Wellenform | geht, klein-mittel | `soundFor` liefert Abtastwerte (Export: Spitze je 50 ms); Musik ueber `Ablauf::benutzt` + `useSoundFor`; Wellenform einmal je Datei cachen |
| Skriptfluss-Spur (worauf gewartet wird, Klick -> Zeile) | geht, mittel | waits tragen Herkunft (wait/dowait/waitsignal + Zeile), Signalsender bekannt; fehlt: "fertig durch" aus dem Nachbau herausreichen |
| Kamera-Filmstreifen | geht, mittel-hoch | Export zeigt den Weg (Zeiger setzen, Bild, ausschneiden); im Programm: kleine Rendertexturen je 2 s, nach Aenderung ungueltig, im Hintergrund nachrendern; Risiko Bildzeit/VRAM |
| Kurven fuer Fade/Zoom | geht nur als Rampe | Engine linear (3.); Kurveneditor mit Tangenten NICHT anbieten; Werte aus CamSegment/`CameraTrack::at` |
| Uebersichtsleiste | geht, klein | Uebersichtsbalken gibt es (nur Kamera, nur bei Spuren aus) - auf alle Spuren mit Ausschnittrahmen erweitern |
| Aufnehmen durch Steuern einer Figur | geht mit Aufwand hoch | Pmove-Port `navSpielerPmove` + Steuerung aus Play in editor F6 (`gui/app_karte_spielen.cpp`), Kollision der Vorschau aus der NavWelt; Umsetzung: Figuren -> vereinfachte waypoint_navgoal + SET_NAVGOAL/dowait (+WALKING/RUNNING nach Tempo) - im Spiel laeuft die NPC-Wegsuche, die Vorschau zeigt deren Ergebnis, nicht die Aufnahme; Schiffe/Kamera exakt als ROFF (Schreiber seit rc599) bzw. move-/camera-Schluessel. Nicht moeglich: Spieler-Eingaben/Animationen 1:1 auf einen NPC |
| Pfad im 3D zeichnen/ziehen, nummerierte Punkte | geht, mittel | Laufbahnen, Zwischenpunkt mit einem Undo-Schritt (mission1 Punkt 3), gemeinsamer Undo-Stempel Skript+Entities (`naechsterAenderungsStempel`, mission1); Punkte = waypoint_navgoal in der .ent |

## 5. Export fuer den klickbaren Entwurf

Aufruf: `sh /y/mission_lauf.sh zeitleiste0 <name> 1500 BHED_ZL0_AUS=<Ordner>`
(laedt academy1 aus dem reinen JKA, Reiter intro, und md_am_sith aus dem
eingerichteten Movie-Duels-Ordner, aktiver Reiter intro2_sith mit dem Schiff
sta2). Lauf `zl2`: 4 OK, 0 FEHLER, 0 Warnungen im Bau.

Dateien (`<SP>\zeitleiste0\`):

- `academy1_intro.json` (1,0 MB), `md_am_sith.json` (1,3 MB)
- `bilder\<szene>_tNNNNNN.png`: Kamerabild durch die Szenenkamera alle 2 s
  (46 bzw. 32 Stueck, 225 x 155 px), `bilder\<szene>_heute.png`: die
  heutige Zeitleiste im Vorgabe-Layout. Zusammen 80 Bilder, 3,4 MB (mit PIL
  nachkomprimiert).

Aufbau (Version 1, Zeiten in ms ab Szenenbeginn, Herkunft je Eintrag
`skript` + `pfad` + `zeile` = Zeile im Skriptbaum mit allen Bloecken offen;
`skript` = fremdes Skript bei run/use, "(Nachbau)" fuer eingefuegte
Wartestuecke):

- Kopf: szene, spiel, karte, skript, reiter, dauerMs, praktischesEndeMs,
  kameraDauerMs, ablaufEndeMs, kartenwechselMs/-Karte, missionFailedMs,
  traeger, skripte (Datei, beginnMs), hinweise, anmerkungen, weltMusik,
  untertitelDateien, heute (Messwerte oben), bilder, bildHeute.
- `spuren[]` (je affect-Ziel, "" = Skript): `ereignisse[]` mit startMs,
  endMs, art (camera/sound/wait/set/move/other/use/saber), befehl,
  beschriftung; bei sound `ton` {datei, kanal, dauerMs, untertitel,
  untertitelSchluessel, pegel = Spitze je 50 ms in %}; bei use ziel, klang,
  musik; bei saber an; bei wait `wartetAuf` {befehl wait/dowait/waitsignal,
  argument, task + taskInhalt, signal + signalVon [{ms, darsteller}]};
  `heute` {breitePx, mitText}.
- `kamera`: `abschnitte[]` (art move/pan/zoom/roll/enable/disable/fade/
  shake/follow/path, start/end, lerpStartMs, von/nach, farbeVon/-Zu,
  vonWinkel/zuWinkel, gruppe, tempo, befehl, heuteText, ueberlapptAbMs),
  `ueberlappungen`, `proben[]` alle 100 ms {an, pos, winkel, fov, fade RGBA,
  balken, folgt}.
- `darsteller[]`: name, npcType, kartenFigur, brushModell, erscheintMs,
  `schritte[]` (art teleport/walkTo/anim/hold/glide/turn/roff/..., start/
  end, text, teil, von/nach, ankunftMs, linear, Herkunft), `zustand[]` alle
  250 ms {pos, gier, anim, torso, sichtbar, bewegt, klinge, waffe}.
- `mover[]` (Bahnstuecke, Umschaltzeiten), `moverKlaenge[]`, `benutzt[]`
  (use-Kette mit Musik: academy1 2, md_am_sith 11, darunter mus1
  music/md_am/am_intro2.mp3 bei 0 und mus2 bei 58,4 s).

Stichproben: alle 15 Saetze in academy1 haben Laenge und Untertitel
(ACADEMY1_01LUK011 "I'd like to welcome all our new students ..."); in
md_am_sith 5 von 9 (die 4 ohne sind Triebwerks- und Landeklaenge, fuer die
es keinen Eintrag gibt - wie im Spiel). Alle 45 bzw. 5 waitsignal haben
ihren Sender. Die rote Ueberlappung des Exports stimmt mit der Zaehlung der
Leiste ueberein (2 bzw. 11). FADE `heuteText` "0.000 0.000 0.000" belegt
die "0 0 0"-Kaestchen.

## 6. Empfehlung

### Architektur

- EIN Modell fuer beide Ansichten: `bhed/spuren.h` (Kern, ohne ImGui) baut
  aus Ablauf + CameraTrack + Scene + MoverSim eine Liste `Clip` {Spur
  (Darsteller | Kamera | Dialog | Musik | Skriptfluss | Welt), Art, startMs,
  endMs (echte Dauer: Klanglaenge, Animationsende, Ankunft), Haltebereich,
  Text, Symbol, Untertitel, Herkunft (Skript, Pfad), Bearbeitbar-Merkmale
  (welches Argument ist Dauer/Zielwert), Warnungen}. Ersetzt `kategorieVon`
  (Textsuche) und `spurenBauen`; `drawTimeline` und `drawSpurenAnsicht`
  zeichnen nur noch. Untertitel/Wellenform als Cache neben dem Modell.
- Zeichnen: Uebersicht (Einstellungen, Dialog, Figurenaktivitaet, Musik) und
  Detail (je Darsteller aufklappbar), Unterbahnen per Intervall-Packen,
  Symbole fuer Momente, Text immer (abgeschnitten) oder im Tooltip,
  Ueberlappung als lesbare Warnung am Clip.
- Bearbeiten: nur Operationen, die ICARUS ausdrueckt (Dauer-Argument, wait
  davor/danach, Zielwert, Kanal, Datei); jede ueber `Document` mit EINEM
  Undo-Schritt (`fasseZusammen` bzw. Stempel aus mission1 fuer Skript +
  Entities).
- Tests: Kern-Test (`tests/spurentest.cpp`) gegen eine eigene Mini-Szene mit
  jedem Befehl (Kamera aller Arten inkl. PATH und Ueberlappung, Dialog mit
  bekannter Laenge und Untertitel, Lauf, Animation mit Haltezeit, dowait,
  waitsignal/signal, use auf Musik, Mover) - Erwartungswerte aus den
  Engine-Formeln, nichts zugeschnitten; Selbsttest je Stufe mit Klick/Zieh/
  Undo; Export-Modus `zeitleiste0` als Gegenprobe auf academy1/intro und
  md_am_sith (gleiche Zahlen vorher/nachher, ausser bewusst geaenderten).

### Stufenplan

1. Modell `bhed/spuren.h` + Kern-Test + Mini-Szene; heutige Leiste darauf
   umstellen ohne sichtbare Aenderung (Gegenprobe Export).
2. Lesbarkeit: echte Dauern (Klang, Animation, Lauf), Symbole, Unterbahnen,
   Texte, FADE/PATH/ENABLE-Zeilen, Ueberlappung erklaert.
3. Dialog- und Musikspur (Untertitel, Wellenform), Uebersichtsleiste.
4. Zwei Ebenen (Uebersicht/Detail), Skriptfluss-Spur (Nachbau reicht
   "fertig durch" heraus), Klick -> Skriptzeile.
5. Kamera-Filmstreifen (Rendertexturen, Hintergrund).
6. Pfad im 3D (nummerierte Punkte, Ziehen) und Aufnehmen (Pmove ->
   Navgoals/ROFF).

### Risiken

- Zwei Wahrheiten: heute zeigen Leiste und Spurenansicht verschiedene Dauern
  - nur mit einem Modell loesbar.
- Bearbeiten in fremden Skripten (run, use): Pfad leer - nur Anzeige.
- Aufnehmen: Erwartung "spielt genau ab" ist im Spiel nicht erfuellbar
  (NPC-Wegsuche); klar sagen und das Spielergebnis zeigen.
- Filmstreifen kostet Bildzeit; nur bei Bedarf und begrenzt rendern.
- Movie Duels: Unterschiede (Regelwerk) muessen im Modell umschaltbar
  bleiben wie im Nachbau.

## Gegen das Spiel geprueft / nicht geprueft

- Gelesen und zitiert (OpenJK/MD-Quellen): Kamera-Interpolation, Fade,
  Shake, Mover-Bahnen, SET_ANIM-Flags und Ueberblendung, PlayIcarusSound und
  Untertitel-Schluessel.
- Untertitel-Schluessel im Export gegen die echten .str-Dateien geprueft
  (Trefferzahl unten), nicht gegen das laufende Spiel.
- Nicht im Spiel verglichen (keine Spielstarts in diesem Auftrag).

## Offen

- Alt-Verschieben ohne wait davor = zwei Undo-Schritte (gelesen, nicht
  nachgestellt).
- camera TRACK/DISTANCE nicht nachgebaut; Haeufigkeit in Ravens Skripten
  nicht gezaehlt.

## Changelog

## English

- Investigation only (no change for users): how the mission timeline is built today, which data exists for each moment, what ICARUS and the engine can express, and a plan for a clearer timeline.
- New self-test mode `zeitleiste0` (branch only): writes all tracks, events, camera values, figures, movers, sound lengths, subtitles and waveforms of a scene as JSON, plus camera thumbnails every 2 s.

## Deutsch

- Nur Erkundung (keine Aenderung fuer Anwender): wie die Zeitleiste im Mission-Tab heute gebaut ist, welche Daten es je Zeitpunkt gibt, was ICARUS und die Engine ausdruecken koennen, und ein Plan fuer eine uebersichtlichere Zeitleiste.
- Neuer Selbsttest-Modus `zeitleiste0` (nur im Zweig): schreibt alle Spuren, Ereignisse, Kamerawerte, Figuren, Mover, Klanglaengen, Untertitel und Wellenformen einer Szene als JSON, dazu Kamerabilder alle 2 s.

## 中文

- 仅为调研（用户无变化）：任务标签页时间轴目前的构建方式、每个时间点可用的数据、ICARUS 与引擎能表达的内容，以及更清晰的时间轴方案。
- 新的自检模式 `zeitleiste0`（仅在此分支）：将场景的所有轨道、事件、摄像机数值、角色、移动体、声音长度、字幕和波形写为 JSON，并每 2 秒生成摄像机缩略图。

## 日本語

- 調査のみ（ユーザー向けの変更なし）：ミッションタブのタイムラインの現在の構成、各時点で使えるデータ、ICARUS とエンジンが表現できること、より分かりやすいタイムラインの計画。
- 新しいセルフテストモード `zeitleiste0`（このブランチのみ）：シーンのすべてのトラック、イベント、カメラ値、キャラクター、ムーバー、サウンドの長さ、字幕、波形を JSON に書き出し、2 秒ごとにカメラのサムネイルを作成。
