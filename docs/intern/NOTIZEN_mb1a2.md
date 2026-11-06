# Notizen Modell-Block 1a-2: Weights live, Bedienung wie Blender, Mission-Tab auf g2.h, ModView-Reste, Model-Tab ohne Skriptteile

Auftrag `auftrag_mb1a2.md` (mit den Nachtraegen vom 06.11.: Model-Tab nur zum Modellieren, Fenster "Ins Skript"; "Schoen,
schnell, leicht zu finden"). Direkt in `Y:\karte` auf `karte1` ab d8d08e285, Build `build-test`, Laeufe `laeufe/mb2_*`
(rot `mb2_rot_*` mit einer voruebergehenden Schaltung `BHED_MB2_ROT`, die die neuen Wege abschaltete und wieder entfernt
ist; gruen `mb2_g*`, Bildvergleiche `mb2_bild_*`/`mb2_b*`, Schluss `mb2e_*`). kFassung bleibt rc610. Kein Spiel, kein 3ds
Max, kein Blender gestartet.

## Umsetzung

- **Gewichte wie im Spiel** (`include/bhed/modell/gewichte.h`, `src/modell/gewichte.cpp`): je Ecke waehrend des Malens eine
  Malliste (`Netz::mal`) mit beliebig vielen Knochen; `fuersSpiel` macht daraus, was die .glm traegt: die vier groessten
  (iMAX_G2_BONEWEIGHTS_PER_VERT), Summe 1, absteigend, die ersten n-1 auf 10 Bit gerundet, das letzte als Rest, gelesen wie
  G2_GetVertBoneWeight (mdx_format.h:334-352). Die Reihenfolge absteigend ist gemessen: in allen 77 Modellen unter
  models/players von JKA liegen 116729 von 117100 Ecken mit mehr als einem Gewicht absteigend, 331 gemischt (gleiche
  Werte), 40 aufsteigend. Ein Gewicht unter einem halben 1023stel faellt weg (die Datei kann es nicht tragen).
  `zuGlm` zeichnet waehrend des Malens schon die Gewichte des Spiels - die Pose zeigt live, was das Spiel daraus macht.
- **readGlm liest Gewichte jetzt bitgleich wie das Spiel**: `fG2_BONEWEIGHT_RECIPROCAL_MULT * iTemp` statt `iTemp / 1023`
  (24 der 1024 Werte ein Bit anders, mit ihnen der Rest des letzten Gewichts).
- **Bereich Weights**: Knochen waehlen (Liste oder Explorer), die Figur in der Farbe des Gewichts (Blender weight_to_rgb:
  blau-tuerkis-gruen-gelb-rot, die Farbleiste als Bild, das Gewicht als UV - die Grafikkarte mischt das Gewicht ueber das
  Dreieck), Pinsel Hinzufuegen/Abziehen/Setzen/Glaetten (Abfall Blender "Smooth" 3p^2-2p^3, ein neuer Tupfer nach 10 %
  des Radius Weg wie Blenders Spacing, nur Ecken zur Kamera), Gewicht/Staerke/Radius, im Max-Modus die Festwerte des Weight
  Tools (0 .1 .25 .5 .75 .9 1), Spiegeln beim Malen und ganze Seite (Knochen l_/r_, _l/_r, lhand/rhand; Ecke x -> -x),
  Gewicht auf die Auswahl, Pruefhilfen live (Ecken ueber 4, Flaechen ueber 32 Knochen), Uebernehmen, ein Rueckgaengig je
  Strich. Spiegeltoleranz 0.1 Einheiten: am Sturmtruppler findet sie 1241 Partner (gemessen, `mb2_g2_f`).
- **Unterobjekte**: Ecke, Kante, Dreieck (Blender 1/2/3), Rand und Element (Max 3/5), Objekt (Max 6, Blender Tab). Klick und
  Rahmen, Max Strg/Alt, Blender Umschalt. W/E/R und G/R/S wirken auf die gewaehlten Ecken der gezeichneten Stufe.
- **Bedienung wie Blender** (`blenderTasten()`, je Taste die Zeile aus `belege/blender_keys.txt` als Quelle; Ziffernblock,
  Pos1 und Strg+Alt+Q aus blender_default.py, im Auszug nicht enthalten, als "Doku" markiert): G/R/S modal (X/Y/Z, zweimal
  lokal, Umschalt Ebene, Zahl, Enter/Linksklick, Esc/Rechtsklick ohne Rueckgaengig-Schritt), Tab, Strg+Tab (Modus-Menue;
  ImGuis Fensterwechsel auf Strg+Tab ist dann aus), 1/2/3, A/Alt+A/Strg+I, B, Ziffernblock 1/3/7 (Strg Gegenseite), 5,
  2/4/6/8 (15 Grad), Komma, Pos1, Strg+Alt+Q, Leertaste und Umschalt+Leertaste, Pfeile, F3, Strg+Z/Strg+Umschalt+Z.
  Knochen als Oktaeder (Blender bone_octahedral_verts: Ring bei 10 % der Laenge, 10 % Abstand), eine grosse Ansicht, die
  Zeitleiste ab 1 mit dem GLA-Frame im Tooltip. Der Schalter wirkt sofort.
- **3ds Max**: dazu die Unterobjekt-Ebenen 1-6 (Autodesk-Karte).
- **Mission-Tab und Karten-Figuren auf g2.h** (`figurG2Regeln`, app_view3d_spiel.inc): Haut (G2API_SetSurfaceOnOffFromSkin),
  surfOff/surfOn der .npc (G_SetG2PlayerModelInfo, NPC_GetSurfaceParms - neu gelesen), Schluesselaermel l_arm_key fuer
  CLASS_IMPERIAL mit message, Movie Duels SP danach `force_shield` mit TURN_OFF 0x100 (MD9 g_client.cpp:2381, 2282; OpenJK
  hat die Zeile nicht), gezeichnet wie RenderSurfaces. Die Flaechennamen wie R_LoadMDXM (klein, "_off" gestrichen; unter
  JK2_MODE nur fuer ein JKA-Modell); G2_IsSurfaceLegal nimmt den ersten passenden Namen.
- **Droideka**: `force_shield_off` hat in der Datei die Flagge 0 und heisst im Spiel `force_shield`. RenderSurfaces zeichnet
  es; Movie Duels schaltet es beim Erzeugen jeder Figur ab (g_client.cpp:2381 `gi.G2API_SetSurfaceOnOff(&ent->ghoul2[...],
  "force_shield", TURN_OFF)`, TURN_OFF = 0x100 in Zeile 2282), AI_AssassinDroid.cpp:51/64 schaltet es mit dem Schild an und
  aus. Der Mission-Tab zeigt es daher in Movie Duels aus (wie vorher, jetzt aus der Regel), in OpenJK an.
- **ModView-Reste**: einfarbiger Hintergrund (ModView glClearColor mit AppVars._R/_G/_B, Vorgabe 256/5 = 51), Ecken- und
  Dreiecknummern der gewaehlten Flaeche (r_surface.cpp:864-935, Farben wie GetWeightColour), Hoehenlineal (model.cpp:2431-
  2462: 160 Einheiten, je Einheit ein Strich und "%d __"), die Gewichtsfarben der Ueberlagerung wie GetWeightColour, das
  Foto mit der Ueberlagerung (die ImGui-Zeichenliste der Ansicht in Bildpunkten des Fotos ueber ImGui_ImplDX11 auf das
  Modellziel). .mvs/.ideal und Stapelumwandlung: offen (eigenes Dateiformat bzw. GLM-Schreiber aus Stufe 2).
- **Model-Tab ohne Skriptteile** (Nachtrag shank): keine Knopfspalte, kein Script Flow (auch nicht als Panel), keine
  Skript-Statuszeile (statt dessen der naechste Rueckgaengig-Schritt des Modells); Datei-Menue mit Modell oeffnen/aus Paket/
  Foto/Ins Skript, "Modell speichern" grau mit Hinweis auf Stufe 2; Strg+O oeffnet ein Modell, Strg+S sagt, dass .glm
  speichern mit Stufe 2 kommt, Strg+Z/Y gelten dem Modell. Ein gespeicherter DockSpace des Model-Tabs mit den alten Knoten
  wird durch die Standardanordnung ersetzt.
- **Fenster "Ins Skript"** (ab Werk zu, eigenes Fenster wie die herausgenommenen Panels, `insSkriptOffen`): Animation,
  Befehl (SET_ANIM_BOTH/UPPER/LOWER, SET_ANIM_HOLDTIME_* mit der Laenge Bilder mal frameLerp), Figur (affect-Block, Liste der
  Figuren der Szene und player), Zielskript (offene Reiter oder neues), Vorschau der Zeile, Einfuegen als ein Rueckgaengig-
  Schritt. Offen ueber den Knopf ueber den Animationen, Doppelklick, Rechtsklick auf eine Sequenz, Datei-Menue, Strg+Umschalt+I,
  Rechtsklick-Menue der Ansicht, Suche.
- **Symbole und Auge im Szenen-Explorer** (Nachtrag shank 06.11.): eigene Symbole (`UiIcon::Mb*`, selbst gezeichnet, je Art
  eine Farbe, im hellen Schema dunkler) fuer Modell, Flaeche, Flaeche mit Kindern, Kappe, Tag, Knochen, Wurzelknochen, Anbau,
  Sequenz (Animationsliste), LOD (Kopf der Flaechen); das Tag-Symbol auch an den Tags in der Ansicht. Im Spiel abgeschaltete
  Flaechen tragen einen roten Kreis (Ghoul2), getrennt vom Auge. Das Auge links je Zeile (Flaechen, Tags, Knochen, Anbauten)
  blendet nur in der Ansicht aus: Klick mit den Kindern, Strg/Alt+Klick solo, "Alle einblenden" im Rechtsklick-Menue und in der
  Aktionssuche, Blender H/Alt+H/Umschalt+H. Die Tastenkarte von 3ds Max (`max_hotkeys_de.txt`) hat keine Taste dafuer - dort
  ueber Auge, Rechtsklick und X. Auswahl, Rahmen, Pinsel, Grenzen und Foto uebergehen Ausgeblendetes; es bleibt beim Wechsel
  von Sequenz und LOD.
- **Leicht zu finden**: Rechtsklick-Menue der Ansicht (Auswahlart, Untermenues Weights und Anzeige, Aus-/Einblenden, Ins
  Skript), "Alle Aktionen suchen" (X bzw. F3) mit allen Funktionen, Tooltips an Knoepfen und Schaltern.

## Rot und gruen

- **Gewichte lesen wie das Spiel** (ctest `modell_gewichte`, neu): rot 12 von 3651 Gewichten der Probe `data/fixtures/model.glm`
  und 78 an einer Kopie mit allen 1024 Werten anders als G2_GetVertBoneWeight; gruen 0 / 0.
- **Neue Funktionen**: Selbsttest `modellblock` mit `BHED_MODELL_NUR=mb2` und `blender` zuerst mit der voruebergehenden
  Schaltung `BHED_MB2_ROT` (neue Wege aus): `mb2_rot_f` 13 OK / 14 FEHLER (Spalten, Strg+S, Ins Skript, Weights, Foto),
  `mb2_rot_b` 15 / 27 (Tasten, Zaehlung). Gruen nach den Testkorrekturen siehe Schluss.
- **Symbole und Auge** (Nachtrag): rot mit `BHED_MB2_ROT` (kein Symbol, kein Auge, nichts ausgeblendet) `mb2_rot_auge`
  31 OK / 5 FEHLER (Symbole/Augen, hips ausblenden, Bild ohne hips, solo, H/Alt+H). Gruen siehe Schluss. Zwei Testfehler
  unterwegs: der Strg+Klick auf das Auge von head ging an eine andere Zeile, weil head tief im Baum unter dem sichtbaren Teil
  des Explorers stand (`mb2_g6`: Zeile y 1588, sichtbar bis 719) - der Test rollt jetzt erst dorthin.
- **Weights-Farbe**: die Pruefung "an der gemalten Ecke rot" blieb gruen-tuerkis (`mb2_g4_f`/`mb2_g5`: 0/213/91). Das Foto
  zeigt: die Ecke mit Gewicht 1 an rhand folgt der Hand und liegt hinter dem Arm, sichtbar ist ihre Umgebung. Die
  Farbleiste prueft der Test jetzt am Kopf an seinem eigenen Knochen (Gewicht 1.000 -> 130/0/0 rot, Fuss 0/0/135 blau,
  `mb2_g6`); an der gemalten Stelle, dass sie nicht mehr blau ist. Die Farbleiste liegt dazu in der mittleren Haelfte des
  Bildes mit neu abgetasteten statt gemittelten Stufen (sonst mischt WRAP am Rand Blau und Rot).
- **Beim Ansehen der Handbuchfotos** (Regel "schoen, schnell, leicht zu finden"), je rot im Foto bzw. Protokoll, gruen danach:
  - Rechtsklick-Menue in einer Spalte 669 x 1151 Bildpunkte (uiScale 1.5): ueber den Rand des Hauptfensters hinaus macht
    ImGui daraus ein eigenes Fenster, auf dem Foto fehlte es (`mb2_hbk_de`: "eigenes Fenster 1"). Jetzt Weights und Anzeige
    als Untermenues: 645 hoch, im Hauptfenster (`mb2_hb_mb1a2_*`: "eigenes Fenster 0" in allen 6 Sprachen).
  - Fenster "Ins Skript": die Startlage der schwebenden Panels war niedriger als der Inhalt, Einfuegen nur halb zu sehen.
    Schwebend waechst es jetzt auf die Hoehe seines Inhalts (Foto: Knopf ganz da).
  - Foto "Auge": die ausgeblendete Zeile head stand unter dem sichtbaren Teil des Explorers - das Foto rollt jetzt dorthin.
- **Mission-Tab vorher/nachher** (`BHED_MODELL_NUR=bild2`, je NPC-Typ der .npc-Dateien Figur wie der Mission-Tab, Regel bis
  rc610 gegen die Ghoul2-Regeln, Bild und Flaechen): JKA 176 NPC-Typen, 158 gleich, 10 anders, 8 ohne Modell; Jedi Outcast 75,
  56 gleich, 17 anders, 2 ohne; Movie Duels 1338, 1190 gleich, 140 anders, 8 ohne. Jede Abweichung mit Flaechen und Beleg im
  Protokoll (`laeufe/mb2_b3_*`, Zeile "Mission vorher/nachher anders"). Die Arten:
  - surfOff/surfOn der .npc (G_SetG2PlayerModelInfo): z. B. JKA imperial `surfOff l_arm_key` (imperial.npc), rodian2
    `surfOff "hips_belt torso_vest"`, `surfOn torso_augment` (rodian.npc), weequay2-4, ugnaught2, JK2 jedi2/prisoner2
    (`surfOff "head head_face"`, `surfOn "head_off head_face_off"` in NPCs.cfg), jedif (jan). Bis rc610 las JK Studio
    surfOff/surfOn gar nicht.
  - Flaechen mit "_off" im Namen, aber Flagge 0 (Kappen und Hilfsflaechen vieler Movie-Duels-Modelle, z. B.
    `torso_cap_l_arm_off`, `stupidtriangle_off`): RenderSurfaces zeichnet sie; meist 0 Bildpunkte (innen liegend).
  - Movie Duels imperial/jedi_zm: die Haut sagt `hips,*off`, die Hierarchie hat zuerst `hips_off` (Flagge OFF, im Spiel
    "hips") und dann `hips`. G2_IsSurfaceLegal nimmt den ersten Namen, G2API_SetSurfaceOnOffFromSkin laesst ihn (schon
    OFF) - die zweite Flaeche `hips` bleibt an (6645 bzw. 6981 Bildpunkte). Im Spiel zeichnet sie mit dem Shader "*off",
    also dem Default-Shader; JK Studio zeichnet sie in der Ersatzfarbe (siehe Offen).
  - StOfficerAlt (JKA): zwei Bloecke dieses Namens (stofficer.npc mit auskommentiertem surfOn/surfOff, stofficeralt.npc mit
    aktivem); JK Studio nimmt den aus stofficeralt.npc.
  - Droideka (Movie Duels): `force_shield_off` bleibt wie bisher unsichtbar, jetzt aus g_client.cpp:2381.
- **Skinning: Pose ueber den Model-Tab gegen den Mission-Tab-Weg** (gleiche Figur, Animation mitten im Lauf, halbes Bild
  weiter; Knochen ueber figurKnochen bzw. die Pose des Model-Tabs, gleicher Zeichner): JKA 77 / 77 gleich, Jedi Outcast
  41 / 41, Movie Duels 552 / 552. Der erste Lauf meldete Modelle ohne animation.cfg (interrogator, tie_fighter,
  Gleiter) - der Test rechnete dort keine Knochen fuer den Mission-Weg; korrigiert.
- **F46 B6-1 Hierarchie** (menueeditor, nur unter Last rot): rot nachgestellt im vollen Lauf mit Prozessorlast zwischen "3b Eng:
  aufraeumen" und dem Ende von F46 (`mb2_mevoll_last`: dieselben 2 FEHLER wie in R3, "Seite 0, Auswahl 2" - der Strg-Klick
  auf k1 traf keine Zeile); allein (`mb2_f46_*`, auch mit BHED_TEST_BREMSE 300/600 und kleinem Fenster) gruen. Ein zweiter
  Lastlauf mit Protokoll der Klickstelle (`mb2_me_last`) war gruen: k1 steht bei y 1361 am unteren Rand der Hierarchie. Ursache
  nach dem Befund: Zeitabhaengigkeit im Test - die Pause nach dem Waehlen zaehlt Zeit, nicht Bilder; unter Last ist sie nach
  einem Bild um, und der Klick ging an den Ort der Zeile aus einem Bild, in dem die Hierarchie noch nicht fertig stand. Behoben
  im Test: geklickt wird erst, wenn k1 drei Bilder lang am selben Ort steht (die Zahl der Bilder steht im Protokoll). Kein
  Fehler im Programm gefunden.
- **modellleistung** (Bildzeit allein, vorher): der erste Lauf auf d8d08e285 meldete "Figur steht: die Schleife ruht" 127 Bilder
  in 2.56 s (FEHLER); wiederholt (`mb2_leist0b`) 11 Bilder in 2.90 s, 10 / 0 - einmalig, vermutlich gleichzeitige Arbeit am
  Rechner (der erste Lauf startete waehrend des Kopierens), nicht nachstellbar.

- **Alter Model-Tab-Test `modell`** (auf dem Stand vor den Schlusskorrekturen, `mb2e_modell`): rot 8 OK / 12 FEHLER - die Zeilen
  der Animationsliste hiessen "     NAME" (Platz fuer das Sequenz-Symbol), der Test fand BOTH_A1_BL_TR nicht, ohne gewaehlte
  Animation blieben Schleife/Abspielen grau; und "Ins Skript" setzte bis rc610 sofort ein. Jetzt steht das Symbol vor der
  Zeile, die Beschriftung ist der Name; der Test oeffnet das Fenster Ins Skript und fuegt ueber Einfuegen ein: `mb2e_modell2`
  14 / 0.
- **Modus 1, Teiler Model** (`mb2e_mod1`): rot 3 FEHLER - der Test zog am linken Teiler des Model-Tabs, den es mit dem
  Nachtrag (keine Skriptteile) nicht mehr gibt ("Breiten 2527 | 0 | 0"). Jetzt prueft er im Model-Tab: eine Spalte, kein
  Script Flow, keine Actions.

- **Bildzeit** (`modellleistung`, je allein, BHED_D3D_DEBUG=0; Bild-CPU Mittel/p99 in ms): rot `mb2e_leist` gegen die exe
  von d8d08e285 im selben Zustand des Rechners (`mb2e_leist_basis`): Summe der 21 Mittel 19.69 gegen 15.94, z. B.
  Sturmtruppler Animation 0.59 gegen 0.40, jedi_hm ruhig 0.77 gegen 0.58. Ursache: Symbole und Augen fuer alle ~100 Zeilen
  des Explorers gezeichnet, zu sehen ist ein Dutzend. Jetzt nur sichtbare Zeilen: `mb2e_leist2` Summe 15.80 (Sturmtruppler
  Animation 0.40/1.01, jedi_hf ruhig 0.79/2.08, clonetrooper_p2 Animation 1.17/1.75 gegen 1.24/2.11) - wie vorher.

## Schluss

- Sauberer Neubau (`--clean-first`, zuletzt nach allen Korrekturen): 0 Warnungen, 0 Fehler. ctest -j16: 222 / 222 (vorher rot: `handbuch_sprachen` -
  Bilder fehlten, `hilfe_orte` - veraltet; beide nachgezogen).
- Lints: alle ok bis auf portability 99 (wie mb1a1; tests/modell_gewichte.cpp hatte die hundertste, <cstddef> ergaenzt),
  efxflags, fremdprogramm und pruefcodes (11) - unveraendert, keine Datei dieses Auftrags betroffen. i18n: ein
  verwaister Text (ModelAnimToScriptHint) entfernt.
- Selbsttests (parallel): modellblock funktionen 60/0, tasten 43/0, mb2 37/0, blender 44/0, bild jka 77/77, jk2 41/41,
  md 547 gleich + 5 anders wie im Spiel (RenderSurfaces, wie mb1a1), bild2 jka/jk2/md je 2/0, mission1 98/0, modell 14/0
  (nach der Korrektur oben), funktionen und mb2 auf dem Endstand nochmals 60/0 und 37/0, menueeditor 2783/0 (F46 mit
  Last aus den parallelen Laeufen gruen).
- Modus 1: ausserhalb der Tastenkuerzel-Tabelle alles gruen. Die Kuerzel-Tabelle ("die Zeile wartet auf eine Taste", danach
  "aufgenommen"/"loest aus") fiel heute in jedem Lauf unterschiedlich: parallel 51, allein 25, und im Gegenlauf mit der
  exe von mb1a1 (`mb1e_exe4`, Stand d8d08e285) gleichzeitig: mb1a1 34 FEHLER, dieser Stand 39 - dieselbe Art, auf beiden
  Staenden. Nicht durch diesen Auftrag; nicht geaendert (Regel: nicht nachstellbar als eigener Fehler). Frueher einmal
  gesehen (`f6be_modus1`, 1 FEHLER).
- Nach der letzten Korrektur (sichtbare Zeilen) sauber neu gebaut und auf diesem Stand nochmals: modellblock mb2 37/0,
  funktionen 60/0, tasten 43/0, blender 44/0, modell 14/0, ctest 222/222 (`mb2z_*`).
- Danach nur Kommentare: GPL-Hinweise (OpenJK, Movie Duels, ModView, Blender) in gewichte.h/.cpp, g2.cpp und
  app_modelltab_ansicht.cpp, wie im Karten-Editor.
- Handbuch: Seiten modell-weights und modell-bedienung in 6 Sprachen, Bilder h_mb2_weights/kontext/insskript/auge/blender
  und neu h_mb1_* (die Symbole und Augen im Explorer), 2538 x 1472, alle angesehen.

## Offen

- Movie Duels imperial/jedi_zm: die zweite Flaeche `hips` zeichnet das Spiel mit dem Default-Shader ("*off" ist kein
  Shader), JK Studio in der Ersatzfarbe.
- StOfficerAlt (JKA) steht in zwei .npc-Dateien; welche das Spiel nimmt, haengt an der Lesereihenfolge der Dateien - nicht
  im Spiel geprueft.
- .mvs/.ideal (ModView-Dateien) und Stapelumwandlung: nicht klein - eigener Lese-/Schreibweg, die Umwandlung braucht den
  GLM-Schreiber (Stufe 2).
- 3ds Max: die Tastenkarte (`max_hotkeys_de.txt`) hat keine Taste fuer Ausblenden - dort ueber Auge, Rechtsklick und X.
- Nichts davon im Spiel verglichen (kein Spielstart).

## Changelog

### English
- Model tab, Weights: paint bone weights while the animation runs - the model shows the picked bone from blue (0) to red (1); Add, Subtract, Smooth and Set with radius and strength, the fixed values of the 3ds Max Weight Tool, mirror while painting or a whole side (l_/r_ bone names), set the weight on the selection, live checks (more than 4 weights, more than 32 bones per surface) and Apply, which writes exactly what the game reads (4 largest, normalized, 10 bits). One stroke is one undo step.
- Model tab, controls like Blender: G/R/S with X/Y/Z and a typed number, Tab and Ctrl+Tab, 1/2/3 vertex/edge/face, numpad views, Space plays, bones as octahedra, one large view and the timeline counted from 1 (GLA frame in the tooltip). The 3ds Max mode gets the sub-object levels 1-6.
- Model tab only for models: no script tree, no button column and no script status; File and Ctrl+O/S work on the model. Animations go into a script through the new window "Into script" (target script, character, command, preview, insert with undo), closed by default.
- Like ModView: plain background colour, vertex and triangle numbers, height ruler, and the photo now contains the overlays. The right-click menu of the view and "Search all actions" reach every new function; tooltips everywhere.
- Mission tab: characters follow the Ghoul2 rules of the game - surfOff/surfOn of the .npc files, the key sleeve of imperials with a key, Movie Duels turns force_shield off on spawn; weights are read bit for bit like the game.
- Model tab, scene explorer: own icons for model, surface, surface with children, cap, tag, bone, root bone, add-on, sequence and LOD; an eye per row hides only in the view (Ctrl/Alt+click solo, "Show all" in the right-click menu and the action search, Blender H/Alt+H/Shift+H); the Ghoul2 switch of the game stays a separate symbol.
- Fix: the menu editor test "F46 B6-1 hierarchy" waits until the row stands still before clicking (failed under load).

### Deutsch
- Model-Tab, Weights: Knochengewichte malen, während die Animation läuft - die Figur zeigt den gewählten Knochen von Blau (0) bis Rot (1); Hinzufügen, Abziehen, Glätten und Setzen mit Radius und Stärke, die Festwerte des Weight Tools von 3ds Max, Spiegeln beim Malen oder einer ganzen Seite (Knochennamen l_/r_), Gewicht auf die Auswahl, Prüfungen live (mehr als 4 Gewichte, mehr als 32 Knochen je Fläche) und Übernehmen, das genau schreibt, was das Spiel liest (4 größte, normiert, 10 Bit). Ein Strich ist ein Rückgängig-Schritt.
- Model-Tab, Bedienung wie Blender: G/R/S mit X/Y/Z und getippter Zahl, Tab und Strg+Tab, 1/2/3 Ecke/Kante/Fläche, Ziffernblock-Ansichten, Leertaste spielt ab, Knochen als Oktaeder, eine große Ansicht und die Zeitleiste ab 1 (GLA-Frame im Tooltip). Der Modus wie 3ds Max bekommt die Unterobjekte 1-6.
- Model-Tab nur für Modelle: kein Skriptbaum, keine Knopfspalte, kein Skriptstand; Datei und Strg+O/S gelten dem Modell. Animationen kommen über das neue Fenster "Ins Skript" in ein Skript (Zielskript, Figur, Befehl, Vorschau, Einfügen mit Rückgängig), ab Werk zu.
- Wie ModView: einfarbiger Hintergrund, Ecken- und Dreiecknummern, Höhenlineal, und das Foto enthält jetzt die Überlagerungen. Das Rechtsklick-Menü der Ansicht und "Alle Aktionen suchen" erreichen jede neue Funktion; überall Tooltips.
- Mission-Tab: Figuren folgen den Ghoul2-Regeln des Spiels - surfOff/surfOn der .npc-Dateien, der Schlüsselärmel von Imperialen mit Schlüssel, Movie Duels schaltet force_shield beim Erscheinen ab; Gewichte werden bitgleich wie im Spiel gelesen.
- Model-Tab, Szenen-Explorer: eigene Symbole für Modell, Fläche, Fläche mit Kindern, Kappe, Tag, Knochen, Wurzelknochen, Anbau, Sequenz und LOD; ein Auge je Zeile blendet nur in der Ansicht aus (Strg/Alt+Klick solo, "Alle einblenden" im Rechtsklick-Menü und in der Aktionssuche, Blender H/Alt+H/Umschalt+H); der Ghoul2-Schalter des Spiels bleibt ein eigenes Symbol.
- Behoben: die Prüfung "F46 B6-1 Hierarchie" des Menü-Editors wartet, bis die Zeile ruhig steht, bevor sie klickt (unter Last rot).

### 中文
- 模型选项卡,权重:在动画播放时绘制骨骼权重——模型以蓝色 (0) 到红色 (1) 显示所选骨骼;添加、减去、平滑和设置,带半径和强度,3ds Max 权重工具的固定值,绘制时镜像或镜像整侧 (骨骼名 l_/r_),将权重设置到所选,实时检查 (超过 4 个权重、每个面超过 32 根骨骼),以及应用——准确写入游戏读取的内容 (最大的 4 个、归一化、10 位)。一笔为一个撤销步骤。
- 模型选项卡,像 Blender 一样操作:G/R/S 配合 X/Y/Z 和输入的数值,Tab 与 Ctrl+Tab,1/2/3 顶点/边/面,小键盘视图,空格播放,八面体骨骼,一个大视图,时间轴从 1 开始 (提示中显示 GLA 帧)。3ds Max 模式增加子对象级别 1-6。
- 模型选项卡只用于模型:没有脚本树、没有按钮列、没有脚本状态;文件和 Ctrl+O/S 作用于模型。动画通过新的 "插入脚本" 窗口进入脚本 (目标脚本、角色、命令、预览、可撤销插入),默认关闭。
- 与 ModView 相同:纯色背景、顶点和三角形编号、高度标尺,照片现在包含叠加内容。视图的右键菜单和 "搜索所有操作" 可以到达每个新功能;处处有提示。
- 任务选项卡:角色遵循游戏的 Ghoul2 规则——.npc 文件的 surfOff/surfOn、带钥匙的帝国军官的钥匙袖套,Movie Duels 在生成时关闭 force_shield;权重按游戏逐位读取。
- 模型选项卡,场景浏览器:模型、面、带子级的面、封盖、标签、骨骼、根骨骼、附件、序列和 LOD 都有自己的图标;每行一个眼睛,只在视图中隐藏 (Ctrl/Alt+单击为独显,右键菜单和操作搜索中的 "全部显示",Blender H/Alt+H/Shift+H);游戏的 Ghoul2 开关仍是单独的符号。
- 修复:菜单编辑器测试 "F46 B6-1 层次" 在点击前等待行稳定 (负载下失败)。

### 日本語
- モデルタブ、ウェイト: アニメーションを再生したままボーンウェイトをペイント。選択ボーンを青 (0) から赤 (1) で表示。加算・減算・スムーズ・設定 (半径と強さ)、3ds Max のウェイトツールの固定値、ペイント時のミラーまたは片側全体のミラー (ボーン名 l_/r_)、選択へのウェイト設定、ライブチェック (ウェイト 4 つ超、サーフェスごとのボーン 32 超)、そしてゲームが読む形そのまま (大きい 4 つ、正規化、10 ビット) を書く適用。1 ストロークが元に戻す 1 回分。
- モデルタブ、Blender と同じ操作: X/Y/Z と入力値付きの G/R/S、Tab と Ctrl+Tab、1/2/3 頂点/辺/面、テンキーの視点、Space で再生、八面体のボーン、大きなビュー 1 つ、1 から数えるタイムライン (ツールチップに GLA フレーム)。3ds Max モードにはサブオブジェクト 1-6。
- モデルタブはモデル専用: スクリプトツリー、ボタン列、スクリプト状態なし。ファイルと Ctrl+O/S はモデルに作用。アニメーションは新しい "スクリプトへ" ウィンドウ (対象スクリプト、キャラクター、コマンド、プレビュー、元に戻せる挿入) からスクリプトへ。既定では閉じています。
- ModView と同じ: 単色の背景、頂点と三角形の番号、高さ定規。写真にオーバーレイも入ります。ビューの右クリックメニューと "すべての操作を検索" から新機能すべてに届き、ツールチップも付きます。
- ミッションタブ: キャラクターはゲームの Ghoul2 規則に従います。.npc の surfOff/surfOn、鍵を持つ帝国兵の鍵袖、Movie Duels は出現時に force_shield をオフ。ウェイトはゲームとビット単位で同じに読みます。
- モデルタブ、シーンエクスプローラー: モデル、サーフェス、子を持つサーフェス、キャップ、タグ、ボーン、ルートボーン、アタッチ、シーケンス、LOD に専用アイコン。各行の目のアイコンはビューでのみ非表示 (Ctrl/Alt+クリックでソロ、右クリックメニューと操作検索に "すべて表示"、Blender の H/Alt+H/Shift+H)。ゲームの Ghoul2 スイッチは別の記号のまま。
- 修正: メニューエディターのテスト "F46 B6-1 階層" はクリック前に行が止まるまで待ちます (負荷時に失敗していました)。

### Español
- Pestaña Model, Pesos: pinta los pesos de los huesos mientras corre la animación; el modelo muestra el hueso elegido de azul (0) a rojo (1); Añadir, Restar, Suavizar y Fijar con radio e intensidad, los valores fijos de la herramienta de pesos de 3ds Max, reflejar al pintar o todo un lado (nombres de hueso l_/r_), fijar el peso en la selección, comprobaciones en vivo (más de 4 pesos, más de 32 huesos por superficie) y Aplicar, que escribe exactamente lo que lee el juego (los 4 mayores, normalizados, 10 bits). Un trazo es un paso de deshacer.
- Pestaña Model, controles como Blender: G/R/S con X/Y/Z y un número tecleado, Tab y Ctrl+Tab, 1/2/3 vértice/arista/cara, vistas del teclado numérico, Espacio reproduce, huesos como octaedros, una vista grande y la línea de tiempo contada desde 1 (fotograma GLA en la ayuda). El modo 3ds Max recibe los subobjetos 1-6.
- La pestaña Model solo para modelos: sin árbol del script, sin columna de botones, sin estado del script; Archivo y Ctrl+O/S actúan sobre el modelo. Las animaciones van a un script con la nueva ventana "Al script" (script de destino, personaje, comando, vista previa, insertar con deshacer), cerrada por defecto.
- Como ModView: fondo de un color, números de vértices y triángulos, regla de altura, y la foto contiene ahora las superposiciones. El menú del clic derecho de la vista y "Buscar todas las acciones" llegan a cada función nueva; ayudas en todas partes.
- Pestaña Mission: los personajes siguen las reglas Ghoul2 del juego: surfOff/surfOn de los .npc, la manga de llave de los imperiales con llave, Movie Duels apaga force_shield al aparecer; los pesos se leen bit a bit como en el juego.
- Pestaña Model, explorador de escena: iconos propios para modelo, superficie, superficie con hijos, tapa, tag, hueso, hueso raíz, anexo, secuencia y LOD; un ojo por fila oculta solo en la vista (Ctrl/Alt+clic en solitario, "Mostrar todo" en el menú del clic derecho y en la búsqueda de acciones, Blender H/Alt+H/Mayús+H); el interruptor Ghoul2 del juego sigue siendo un símbolo aparte.
- Corregido: la prueba "F46 B6-1 jerarquía" del editor de menús espera a que la fila esté quieta antes de hacer clic (fallaba con carga).

### Čeština
- Karta Model, Váhy: malujte váhy kostí, zatímco běží animace - model ukazuje vybranou kost od modré (0) po červenou (1); Přidat, Odečíst, Vyhladit a Nastavit s poloměrem a silou, pevné hodnoty nástroje vah z 3ds Max, zrcadlení při malování nebo celé strany (jména kostí l_/r_), nastavení váhy na výběr, živé kontroly (více než 4 váhy, více než 32 kostí na povrch) a Použít, které zapíše přesně to, co čte hra (4 největší, normalizované, 10 bitů). Jeden tah je jeden krok zpět.
- Karta Model, ovládání jako Blender: G/R/S s X/Y/Z a napsaným číslem, Tab a Ctrl+Tab, 1/2/3 vrchol/hrana/plocha, pohledy z numerické klávesnice, mezerník přehraje, kosti jako osmistěny, jeden velký pohled a časová osa od 1 (snímek GLA v popisku). Režim 3ds Max dostal podobjekty 1-6.
- Karta Model jen pro modely: žádný strom skriptu, žádný sloupec tlačítek, žádný stav skriptu; Soubor a Ctrl+O/S pracují s modelem. Animace se do skriptu dostanou novým oknem "Do skriptu" (cílový skript, postava, příkaz, náhled, vložení s vrácením), ve výchozím stavu zavřeným.
- Jako ModView: jednobarevné pozadí, čísla vrcholů a trojúhelníků, výškové pravítko a fotka nyní obsahuje překryvy. Nabídka pravého tlačítka v pohledu a "Hledat všechny akce" dosáhnou na každou novou funkci; všude popisky.
- Karta Mission: postavy se řídí pravidly Ghoul2 hry - surfOff/surfOn ze souborů .npc, rukáv s klíčem u imperiálů s klíčem, Movie Duels při objevení vypne force_shield; váhy se čtou bit po bitu jako ve hře.
- Karta Model, průzkumník scény: vlastní ikony pro model, plochu, plochu s potomky, čepičku, tag, kost, kořenovou kost, přídavek, sekvenci a LOD; oko u každého řádku skrývá jen v pohledu (Ctrl/Alt+klik sólo, "Zobrazit vše" v nabídce pravého tlačítka a ve hledání akcí, Blender H/Alt+H/Shift+H); přepínač Ghoul2 hry zůstává samostatným symbolem.
- Opraveno: test editoru nabídek "F46 B6-1 hierarchie" před kliknutím počká, až řádek stojí (pod zátěží selhával).
