# Notizen pruefung4b - Entities, Effekte, Kamera gegen das Spiel

Zweig `pruefung4b` ab pruefung3 1b7b4bcd, Worktree `Y:\sprache_es`, Build
`Y:/sprache_es/build-sprache`. Auftrag: Liste 4b aus `auftrag_pruefung4.md`
(useBBox, Geschuetze/Schaden/trigger_hurt, liegende Schwerter, Fahrzeuge,
Remove/Bruch/target4, CAMERA_ACCEL+ZOOM), je Punkt rot/gruen, Abgleich mit dem
laufenden Spiel.

Tests: `tests/pruefung4b.cpp` (ctest `pruefung4b`, eigene Abschrift der
Engine-Formeln gegen das Programm). Selbsttest-Modus `pruefung4b` noch nicht
angelegt.

## Stand bei der Unterbrechung (30.10., Anhalten auf Wunsch des Koordinators)

Angehalten, weil shank die Zahl der Agenten begrenzt. Kein Spiel gestartet,
keine Sperrdatei angelegt, Spielordner nicht beruehrt. Gebaut nur bhed_core und
pruefung4b, ablauftest, mover_probe, camtracktest (0 Warnungen), nicht
jkstudio. Erster Lauf (nur gruen, rot noch nicht):
- pruefung4b: 6 (Faelle a-f) und 3 gruen; 5b 3 FEHLER - Nachbau: Ziele bei
  3100 statt 3000 und das use nach 20 ms bricht nicht neu; MoverSim: glas nur
  ein Bruch. Vermutung: der Takt (Nachbau 50 ms, "wait 1000 -> 1050"; MoverSim
  verarbeitet uses im 50-ms-Takt) - Sollwerte bzw. Umsetzung im naechsten
  Schritt klaeren.
- ablauftest: #12 (auf 1 s nach dem letzten use umgestellt) FEHLER - gehoert
  zu derselben Klaerung. mover_probe, camtracktest: ok.

Begonnen (im Commit "WIP"):
- 6. CAMERA_ACCEL und ZOOM: `KameraFov` (camtrack.h) = client_camera FOV, FOV2,
  FOV_time, FOV_duration, FOV_vel, FOV_acc, CAMERA_ACCEL/CAMERA_ZOOMING;
  `kameraFovBild`/`kameraFovBei` wie CGCam_Update (ACCEL vor ZOOMING, "<");
  ZOOM, fovzoom, fovaccel, fov, ENABLE schreiben den Stand wie die Engine
  (keiner loescht den Merker des anderen), jeder Zoom-Abschnitt traegt den
  Stand (`CamSegment::fovStand`), CameraTrack::at rechnet nur daraus; Shake
  mit FOV/FOV2 des Bilds. Test: pruefung4b Faelle a-f gegen eine Bild-fuer-
  Bild-Abschrift (1 ms). Rot noch zu laufen (alte camtrack.cpp/h aus HEAD~
  einsetzen, nur pruefung4b bauen).
- 3. forced_rgb_saber_colours: in Movie Duels unerreichbar (die Zweige
  "saberColor"/"saber2Color" fangen die RGB-Schreibweise ab, Laenge passt
  nicht -> Warnung, nichts gesetzt); OpenJK kennt sie nicht. scene.cpp: RGB
  aendert nichts mehr; Jedi Outcast nur "saberColor" (codeJK2 NPC_stats.cpp:
  1707). Test in pruefung4b.
- 5b. Zweites use in der Bruchverzoegerung: Nachbau (ablauf.cpp,
  Geplant::Art::Bruch, Wesen::freiUm) und MoverSim (Bruchstelle::freiUm):
  jedes use laeuft durch funcBBrushDie (use-Skript neu, Bruch neu geplant);
  ein use in den 50 ms vor G_FreeEntity bricht neu. ablauftest #12 auf das
  Engine-Verhalten umgestellt (Bruch 1 s nach dem LETZTEN use).
- 5a. Remove vor dem ersten Add: nur der Zustand (ActorState::kinoNieGesetzt,
  koerperEntfernt, KinoModell::anWurzel in ausruestung.cpp). Die Zeichnung
  wertet ihn noch NICHT aus.

## Naechster Schritt

1. 5b-Fehler klaeren (Takt), voller Bau, pruefung4b rot (alter Stand der
   geaenderten Dateien) / gruen, ctest.
2. 5a fertig: anbautenFuer (app_view3d.cpp) bei koerperEntfernt nur
   Kinomodelle mit anWurzel (neues Feld AnbauDraw::anWurzel, Einheitsmatrix
   statt Bolzen in den Verbrauchern bedienung.inc:3799, spiel.inc:308),
   ActorDraw-Merker -> Koerper nicht zeichnen (bedienung.inc:3696); Test.
3. 5c target4 (misc_model_throw_at_target4 + G_RunObject), 4 Fahrzeuge (.veh
   type/hoverHeight/hoverStrength/gravity; Laeufer/Tiere fallen mit
   veh-Schwerkraft, Jaeger PM_FlyVehicleMove mit Schwerkraft, Schweber
   PM_HoverTrace zweimal je Bild), 3a weapon_saber mit NPC_type (Modell aus der
   .sab, "player" -> g_saber single_1) und Leuchten (solidWhite_cull-Huelle,
   14 Sprites yellow_glow, Licht OJ wv*350+180 / MD wv*100), 1 useBBox
   (Kollisionskarte: CM_PointContents vor der Spur ausser expensivePhysics,
   Kastenspur MASK_SHOT|CONTENTS_WATER, startsolid/allsolid kein Treffer,
   VectorMA(mVel, dt*fraction, mAccel) vor dem Spiegeln - TraceFn braucht den
   Anteil), 2 Geschuetze/fx_target_beam/trigger_hurt gegen den Spieler.
4. Spielabgleich (Geschuetz/Sentry, liegendes Schwert, ACCEL+ZOOM), Selbsttest
   `pruefung4b`, Handbuch/i18n, Schlussrunde.
