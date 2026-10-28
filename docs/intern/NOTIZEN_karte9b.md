# Stufe 9, Teil 2: die restlichen Karten-Werkzeuge (Punkte 4 bis 12)

Arbeitskopie `Y:\karte`, Zweig `karte1`, ab `0afc2050` (Teil 1 + Merge der Release-Korrekturen
aus main). Teil 1 (Punkte 1-3) steht in `NOTIZEN_karte9.md`. Erledigt: **4, 5, 6, 7, 8, 10, 12**,
**9 zum Teil** (Formen ja, Texturen überblenden nein), dazu „Pack map as .pk3 nimmt den
Import-Ordner mit“. Offen: **11** (Asset-Browser) und der Rest von 9 (unten).

## Commits

| Punkt | Commit | Inhalt |
|---|---|---|
| 4 | c1ec7fde, da7843e5 | Machtsprung im Spieler-Pmove (OpenJK 1:1), Sprungweite und Platz (Kern, View-Menü, Anzeige) |
| 5 | 430fe4a7, 40e3abd7, 66fe7fc5, bd1a3725 | NPC-Wege: Netz und Testlauf (Kern, CMake doppelt raus, Menü/Anzeige, Lizenzkopf) |
| – | 46cb322b | Pack map as .pk3 nimmt den Import-Ordner mit |
| 6 | 76f01084 | Verknüpfte Kopien (PrefabVerbund, ein Befehl über vorAbschluss) |
| 7 | 74da92c8 | Kurven-Werkzeug (Weg, Rohr, Geländer, Brücke, path_corner, Wegpunkte) |
| 8 | 8d13f0dd, eecafb42 | Modelle verteilen; Grenze MAX_MISC_ENTS im Optimierer |
| 9 | d74a6a3f | Gelände formen auf Patches (heben, senken, glätten, ebnen) |
| 10 | 92ecf53d | Für Blender exportieren (.glb neu, .obj aus brushexport) und zurück |
| 12 | 1ec12647 | Nach einem Absturz das Autosave anbieten |
| – | (dieser) | Notizen, Schlussrunde |

## Punkt 4: Machtsprung, Sprungweite und Platz

**Machtsprung 1:1 aus OpenJK** in `navSpielerPmove` (src/navlauf.cpp, Quellen aus
`JACoders/OpenJK` master gelesen, nur gelesen):
- `PM_CheckJump`, Teil `METROID_JUMP` (bg_pmove.cpp:1090-1416) am Anfang von `PM_AirMove`:
  `PM_ForceJumpingUp` (in der Luft, PMF_JUMPING, velocity[2] > 0, FP_LEVITATION > 0, ohne
  forceJumpCharge/Sonderanimation/Zwischensequenz) und PMF_JUMP_HELD; `curHeight` gegen
  `forceJumpHeight` {32, 96, 192, 384}, Tempo `(H - h)/H * forceJumpStrength {225, 420, 590, 840} / 10 + 225`,
  sonst Kappe auf 225 bzw. 0; `upmove = 0` (wirkt auf PM_CmdScale).
- Die Kappe vor `PM_StepSlideMove` in `PM_AirMove` (bg_pmove.cpp:2998-3008, codeJK2 1811-1829).
- `forceJumpZStart` beim Absprung, am Boden 0; FP_LEVITATION endet am Boden.
- **Jedi Outcast**: codeJK2 hat denselben Code und dieselben Tabellen. **Movie Duels** liest
  die Tabellen mit `FP_TableLevel` (höchstens 3) – das Feld ist 0..3.
- Die Kraftleiste (forcePower) gilt als voll; der Verbrauch je Sprung wird nicht gerechnet.

**View > Check jumps and space** (`sprungpruefung`): im Raster fällt der geduckte Kasten;
begehbare Böden (Normale z ≥ 0,7) sind Stehplätze (voller Kasten frei) oder „nur geduckt“;
Kanten = ein Nachbar ohne Boden in Stufenhöhe; von höchstens 160 Kanten läuft der Spieler mit
250 in 8 Richtungen an und springt mit gehaltener Taste – ohne und mit Machtsprung der gewählten
Stufe; gemerkt wird, was höher oder über eine Lücke landet. Im Hintergrund (Faden), Anzeige in
2D und Kamera (grün/gelb/blau/violett). Die Stufe gilt auch für „Im Editor spielen“.

Tests: ctest `karte_sprung` (Scheitel 31,6 / ~96 / ~192 / ~384, losgelassen niedriger, JK2 gleich,
Lücke normal, Sims nur mit Machtsprung; rot ohne Machtsprung: 5 Fehler), Selbsttest.

## Punkt 5: NPC-Wege

`npcwege`: die Entities der offenen Karte wie im Entity-Lump, das Netz aus `baueNavNetz` mit
Spuren gegen die Kollision der Brushes, der Testlauf mit `planeLauf` (rennend 300 / gehend 90).
**Das Spiel baut erst ab mehr als drei Wegpunkten ein Netz** (`size_nodes() <= 3`) – das zeigte
der erste rote Lauf; die Konsole sagt es jetzt. View > NPC paths: Netz zeigen (türkis, Sprung
orange), Testlauf vom ersten zum letzten gewählten Entity (gelbe Bahn, ein Kasten in Echtzeit).

Tests: ctest `karte_npcwege` (vier Wegpunkte, Lücke in der Wand, ohne Wegpunkte bleibt die Figur
stehen), Selbsttest (rot zuerst: Wegpunkte außerhalb des Raums).

## Punkt 6: Verknüpfte Kopien

func_groups mit `_bhed_link` / `_bhed_link_origin`. `PrefabVerbund` nach dem Muster des
Spiegel-Modus: Inhalt je Exemplar relativ zum Ursprung (primitivText nach Verschieben mit Texture
Lock); eine Änderung schreibt die anderen **im selben Befehl** (`Rueckgaengig::vorAbschluss`),
also ein Undo; ein ganz verschobenes Exemplar bewegt nur seinen Ursprung; außerhalb eines Befehls
und während eines Ziehens wird nicht abgeglichen (nur gemerkt bzw. gar nicht). Nur Verschiebung,
keine Drehung der Kopien.

Tests: ctest `karte_verbund` (rot ohne Abgleich: 5), Selbsttest.

## Punkt 7: Kurven-Werkzeug

Catmull-Rom durch die gewählten Punkt-Entities (Reihenfolge der Wahl), gleich weit abgetastet
(ungerade Zahl wie Patch-Zeilen, Abstand = Raster × 8). Weg (3 Spalten), Rohr (Kreis aus 9
Kontrollpunkten wie eCylinder), Geländer (zwei Rohre in 40), Brücke (Deck 7 Spalten, Unterseite 3)
als Patches mit der Textur des Texturbrowsers; path_corner- bzw. waypoint-Kette mit
targetname/target. Die Vorderseite des Wegs wie NRCs ebener Patch (der erste Testlauf war rot:
nach unten).

## Punkt 8: Modelle verteilen

Vorlage = gewähltes misc_model_static/misc_model, Bereich = gewählte Brushes (sonst ±256);
senkrechte Punktspur auf die Kollision, begehbar, ≥ 48 auseinander, zufälliger Winkel und
Maßstab. **Neue Grenze** `laufzeit.miscents` = MAX_MISC_ENTS 2000 (OpenJK code/cgame/cg_main.cpp
:1949/:1967, Movie Duels :3147/:3166 – im lokalen MD-Quelltext geprüft; für den Grenzen-Test liegt
OpenJKs cg_main.cpp jetzt in `SP/ojk/oj/code/cgame/`), Jedi Outcast ohne. Der Optimierer zählt
jedes misc_model_static; das Verteilen hält an der Grenze an. `tools/handbuch/grenzen.json` neu.

## Punkt 9: Gelände formen (Teil)

View > Terrain sculpt: M1 in der Kamera formt die Patches unter dem Zeiger (Scene_Intersect):
heben, senken, glätten, ebnen; Radius 64/128/256, Abfall (1 − d/r)²; ein Strich = ein Befehl
(`Patch::punkteSetzen`). **Offen**: Terrain-Brushes und Texturen überblenden (q3map2 `_indexmap`
/ Alpha-Map malen).

## Punkt 10: Blender hin und zurück

File > Export selection for Blender: .glb (neuer Schreiber `glbSchreiben`: glTF 2.0 binär, je
Brush Knoten und Netz, POSITION/TEXCOORD_0/Indizes, Material = Shader, Y oben) oder .obj (NRCs
brushexport schreibt schon Y oben – der erste Selbsttest war rot, weil doppelt umgerechnet).
Zurück mit Import geometry as brushes (Y oben): am selben Ort (Selbsttest: Grenzen gleich).
Patches exportiert brushexport nicht.

## Punkt 12: Nach einem Absturz wiederherstellen

Sitzungsdatei `<Einstellungen>/karte_sitzung.txt` mit dem Namen der offenen Karte, beim sauberen
Beenden gelöscht. Beim Start: liegt sie noch da und ist das Autosave (NRC:
`<Karte>.autosave.map`, ohne Namen `maps/autosave.map`) neuer als die Karte, fragt der Editor;
Ja lädt es unter dem Namen der Karte, als geändert.

## Schlussrunde

(folgt)

## Offene Punkte

- **Punkt 11, Asset-Browser** (Modelle, Prefabs, Klänge mit Vorschaubildern, Suche, Ziehen):
  nicht begonnen.
- **Punkt 9**: Texturen überblenden (q3map2 `_indexmap`, Alpha-Map im Import-Ordner malen) und
  Terrain-Brushes formen.
- Machtsprung: die Kraftleiste wird nicht verbraucht; Salto-Animationen und weitere Machtsprünge
  (Wandlauf, Flip) sind nicht dabei – für die Sprungweite zählt nur die Höhe/Weite.
- Sprungprüfung: Startpunkte höchstens 160 je Lauf, 8 Richtungen.
- NPC-Testlauf: Standardwerte (runSpeed 300 / walkSpeed 90), keine NPC-Datei; Türen/Mover zur
  Zeit sind nicht eingebunden (Kollision nur aus den Brushes).
- Verknüpfte Kopien: keine Drehung; die Menüs „View“ sind dadurch höher (Menügrößen-Tabelle
  nachgezogen).
- Handtests im Spiel für Punkt 1 (siehe Teil 1) stehen weiter aus.
