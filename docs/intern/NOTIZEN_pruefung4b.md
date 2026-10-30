# Notizen pruefung4b - Entities, Effekte, Kamera gegen das Spiel

Zweig `pruefung4b` ab pruefung3 1b7b4bcd, Worktree `Y:\sprache_es`, Build
`Y:/sprache_es/build-sprache`. Auftrag: Liste 4b aus `auftrag_pruefung4.md`
(useBBox, Geschuetze/Schaden/trigger_hurt, liegende Schwerter, Fahrzeuge,
Remove/Bruch/target4, CAMERA_ACCEL+ZOOM), je Punkt rot/gruen, Abgleich mit dem
laufenden Spiel.

Tests: `tests/pruefung4b.cpp` (ctest `pruefung4b`, eigene Abschrift der
Engine-Formeln gegen das Programm). Rotlaeufe: die alte Logik in die
aktuellen .cpp gesetzt (Skripte im Scratchpad `p4b_rot*.py`), nur
pruefung4b gebaut, danach zurueck.

## Stand je Punkt

- 6. CAMERA_ACCEL und ZOOM (WIP-Commit 7d7ebc4d): `KameraFov` wie
  client_camera. pruefung4b Faelle a-e rot mit camtrack.cpp/h von 1b7b4bcd,
  f (ZOOM allein) war schon richtig; gruen.
- 3b. forced_rgb_saber_colours: in Movie Duels unerreichbar (die Zweige
  "saberColor"/"saber2Color" vergleichen 10/11 Zeichen und fangen die
  RGB-Schreibweise ab - Laenge passt nicht, "WARNING: bad saberColor",
  nichts gesetzt, NPC_stats.cpp:4215-4305 MD), OpenJK kennt sie nicht.
  scene.cpp: RGB aendert nichts; Jedi Outcast nur "saberColor". Rot mit
  scene.cpp von 1b7b4bcd, gruen.
- 5b. Zweites use in der Bruchverzoegerung (5121c79b): die Sollwerte des
  Vorgaengers waren falsch, nicht die Umsetzung - ein wait endet im ersten
  Bild NACH stempel + dauer (TaskManager.cpp:1141 "<"), "wait 1000" ab 0
  also bei 1050; der Bruch damit bei 3100 bzw. 1 s nach dem LETZTEN use.
  Dazu gefunden und gebaut: funcBBrushDieGo und G_FreeEntity sind thinks
  und laufen in G_RunFrame am Platz der Entity (G_RunThink vor ihrem
  ICARUS, g_main.cpp:1062-1088) - Nachbau: Geplant::Art::Bruch/Frei in
  Schritt 3 (`denke`), nicht mehr vor allen Entities; ein use vor dem Platz
  des func_breakable im Bild von G_FreeEntity bricht neu, eines danach
  findet nichts. Ablauf::Benutzung::reihe und MoverUse::reihe tragen den
  Platz zu MoverSim (dieselbe Entscheidung). pruefung4b 5b (8 Pruefungen)
  und ablauftest #12: rot mit der alten Logik (6 + 2 Fehler), gruen.
- 5a. Remove vor dem ersten Add (36019ed0): Ghoul2-Modell 0 (die Figur) ist
  fort - Koerper, Waffe und Griffe (Kinder in G2_Sort_Models) werden weder
  gezeichnet noch vom G2-Netztest getroffen (`ActorDraw::ohneKoerper`);
  ein Add danach haengt sich nicht an ("Empty Bolt List") und steht mit den
  Achsen des refEntity am Ursprung (`AnbauDraw::anWurzel`, `anbauWelt`).
  pruefung4b 5a gegen eine Abschrift des ghoul2-Felds (Init/Attach/Remove/
  Sort): rot mit dem alten ausruestung.cpp, gruen.
  - Nicht nachbildbar: haelt die Figur eine Waffe und kommt nach dem Remove
    ein Add, liegt das neue Modell in Fach 0, und die Waffe haengt wieder an
    einem gueltigen Modell - G2_GetBoltMatrixLow liest dann dessen LEERE
    Bolzenliste ohne Pruefung (tr_ghoul2.cpp:2165): undefiniert, im Spiel
    wohl ein Absturz. Die Vorschau zeichnet die Waffe dann nicht.
- 1. useBBox im Effektpfad: der Spurtest ist jetzt der von
  CParticle::UpdateOrigin - CG_PointContents am neuen Ort (Welt + Brush-
  Entities an s.origin; `kollisionsInhalt` = CM_PointContents), Spur nur,
  wenn dort MASK_SHOT|CONTENTS_WATER (ausser expensivePhysics, Vorgabe
  fx_expensivePhysics 1), CG_Trace mit mMin/mMax (useBBox) gegen
  Kollisionskarte, Brush-Entities am Stand der Mover-Simulation und Figuren
  (CM_TempBoxModel, `kollisionsSpurKasten`), startsolid/allsolid = kein
  Treffer, die Beschleunigung fuer den Weg bis zur Wand (VectorMA mit
  trace.fraction), SURF_NOIMPACT ohne impactFx. Die Schrittweite ist jetzt
  die Bildzeit der Vorgabe com_maxfps 125 (8 ms statt 2 ms) - davon haengt
  ab, ob ein Kasten beim Aufsetzen schon im Boden steckt (dann startsolid,
  er faellt durch; ohne expensivePhysics passiert das mit useBBox oft).
  `EffektSpurWelt`/`setzeEffektSpurWelt` (efxdraw.h), gesetzt von
  `effektSpurWeltSetzen` (app_bewegung.cpp) nach jeder Mover-Simulation.
  pruefung4b 1: efx::sim gegen eine Abschrift (Punkt, Kasten ohne/mit
  expensivePhysics) - rot mit 2 ms und ohne die Beschleunigung, gruen.
  efxtest: Boden-Probe mit Welt (Punkt liegt, Kasten faellt durch, mit
  expensivePhysics liegt er 5 hoeher), Effektnetz-Probe mit Strahl-Welt.

## Offen (Reihenfolge)

3a weapon_saber mit NPC_type + Leuchten, 5c target4, 4 Fahrzeuge, 2
Geschuetze/fx_target_beam/Sentry/trigger_hurt gegen den Spieler,
Spielabgleich (Geschuetz/Sentry, liegendes Schwert, ACCEL+ZOOM),
Selbsttest `pruefung4b`, Handbuch/i18n, Schlussrunde.

Gelesen fuer 4 (Fahrzeuge): .veh in assets1.pk3 - nur Swoops setzen gravity
(800 = g_gravity), hoverHeight > 0 nur Swoops (Jaeger 80, aber FLY_VEHICLE);
Laeufer/Tiere/Jaeger fallen mit g_gravity (Jaeger ohne Pilot: kein
SVF_CUSTOM_GRAVITY). Schweber: PM_HoverTrace nach PM_GroundTrace und nach
der Bewegung (zweimal je Bild), Schub (0.3 - f) * hoverStrength * frametime
* 60 bei f < 0.3 und normal[2] >= maxSlope; in der Luft schneidet
PM_ClipVelocity an der Ebene der Schwebespur fast alles ab (sinkt ~1 je
50-ms-Bild), am Boden PM_WalkMove (|v| erhalten, keine Bewegung ohne x/y);
PM_Friction mit der Fahrzeugreibung (0.1 bei VEH_FLYING, Tiere nicht).
Einzubauen in navlauf.cpp (bodenSpur/gehBewegung/luftBewegung) und
schwerkraft.cpp (pmoveSchritt), Werte aus der .veh (app_view3d_spiel.inc
liest schon name/model/skin).
