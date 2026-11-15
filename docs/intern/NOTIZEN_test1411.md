# NOTIZEN TEST1411 (14.11.2026): Testrückmeldungen von shank zu rc612

Auftrag `JKStudio_Uebergabe/auftraege/auftrag_test1411.md`, Fotos von shank `Y:\werkzeuge\test1411\fotos_shank\`. Zweig `p_test1411`
(Worktree `Y:\p_ap6`), Basis karte1 `348f0e345` (rc612). Erster Teil (D, F, I, H, G, J, K, L, M, C) von Opus 5.5 bis `fc44ff56d`, zweiter
Teil (N, O, P, A, E) als TEST1411b. Selbsttest `gui/selbsttest_test1411.inc`:
`BHED_EDITORTEST=modellblock BHED_MODELL_NUR=test1411 BHED_T1411=<punkte>` (Fenster 3840x2160 bei 1,5, alora).

## Ergebnis kurz

| Punkt | Was | Rot (vorher, gemessen) | Grün (nachher) |
|---|---|---|---|
| D | Modell doppelt beim Zoomen, Drahtgitter ab Werk | Ansichtsbild eine Raste hinter der Kamera: bis 116 px gegen frisch gezeichnet; Objektebene 4458 Kanten; Geister in allen Bereichen | 0 px; 0 Kanten auf der Objektebene; Geister nur im Bereich Animation |
| F | Zeitleiste: Knöpfe rutschen | Auto Key bei 611/625/640/654 px (Bild 0/10/100/659) | fest 661 px |
| I | Animation: Knochen erst mit Control Rig | Klick wählte die Fläche torso, kein Knochen, Drehen/Key ohne Wirkung | Knochen gewählt, 30,0° gedreht, Key gesetzt, Abspielen 30,0° |
| H | Knochen ohne saubere Hierarchie | 196 sich schneidende Kastenpaare (_humanoid MD) | 0 (auch rancor), Ketten-Enden stimmen, Drehhilfen als Punkt |
| G | Zeitleiste und Text darunter wie 3ds Max | 12 Prüfungen rot (Fließtext unter der Leiste, kein Zeitschieber, Fußzeile im Explorer) | grün; ap1 134/0, ap2 68/0 |
| J | Rechtsklick: Move/Rotate | kein Move/Rotate/Scale im Menü | Quad-Menü wie 3ds Max, Rotate setzt das Werkzeug; Blender: Transform |
| K | Fußplattform wie CAT | keine Plattform | Plattform am Boden, Fuß folgt, Knie 164° -> 106°, Abrollen um die Zehe |
| L | ViewCube: Buchstabe scheint durch | Klick am verdeckten "W" traf den Ring | trifft den Würfel, Übergang ohne Überschwingen |
| M | Silhouette: Beschriftung unlesbar | Kontrast 1,1:1 | >= 5,3:1 auf jedem Grund |
| C | Load .gla nur lose Dateien | nur Windows-Dialog | Liste der .gla aus den pk3 + "Datei öffnen..." |
| N | Motion-Feld zu voll | 17 Gruppen untereinander (Foto 26; ap10 A14 zählte sie als Soll) | Setup-Modus 6, Animationsmodus 9 Gruppen; Ghosts und Knochen-Darstellung im Reiter Anzeige; K im Setup-Modus 0 Schlüssel |
| O | Painting: Ziel-Textur von Hand | ein Strich Rumpf -> Gesicht malte nur ins erste Bild (der alte Weg = Schalter "Nur Ziel-Textur bemalen": 1 Bild) | Strich malt Rumpf, Kopf und Gesicht (3 Bilder), Strg+Z nimmt alle drei, Ziel folgt (Explorer, Alt+Klick, Vorschaubild) |
| P | Weights: Ecken einzeln wie Skin | Auswahl nicht im Rollout, keine Liste, keine Weight Table, kein Mirror Paste (Foto 28) | Ecke klicken -> Liste lradius 0,500 / lradiusX 0,500 -> 0,6 getippt, Summe 1 -> Mirror Paste -> Gegenseite gleich; Rahmen ohne Rückseiten 2488 -> 1619 Ecken |
| A | Load mission öffnet den pk3-Dialog | Dateidialog (im Selbsttest unterdrückt: Auswahl ging nicht auf); aus einer Liste ohne Archiv kein Eintrag "zuletzt geöffnet" | Liste des Spiels sofort, nach Archiv gruppiert; Eintrag mit vollem Pfad |
| E | Sequenzen im Baum ohne Suche | Ast mit 2635 Zeilen im Baum, Namen abgeschnitten | eigener Reiter mit Suche (Strg+F), Gruppe, Spalten Name/Bilder/Schleife sortierbar |
| B | Model-Tab leer sieht anders aus | ohne Modell nur "No model yet" und "Load a model", kein Raster, kein ViewCube, keine Kopfzeile, kein Befehlsfeld (Foto 15; Lauf t1411c_b_rot: 5 von 8 Pruefungen rot) | je Bereich und Bedienmodus dieselben Rahmen, Reiter und Gruppen (zu, gesperrt), ViewCube und Kopfzeile, Ladehinweis mit NPC-Browser und .glm oeffnen, Explorer mit Reitern/Suche/Filter (t1411c_b3) |

## Entscheidungen

- D: Objektebene ohne Kanten (Max und Blender); Unterobjekt-Ebenen wie Blenders Edit Mode schwarz, Alpha 170, 1 px; Max hell wie bisher. Ghosts nur im Bereich Animation, Schalter auch im [+]-Menü, ab Werk aus.
- I: ohne Rig dreht der Knochen selbst (FK, Rig mit einem Griff), kein automatisches Control Rig; mit Rig wählt der Klick den Griff. "hips is frozen" frisch geladen nicht nachstellbar.
- H: Kasten entlang der eigenen X-Achse der GLA-Grundstellung bis zum Folgegelenk; Drehhilfen als Hilfspunkt (Schalter "Hilfsknochen zeigen"); Verbindungslinie nur am Anfang einer Kette.
- G: eine einzeilige Statuszeile; Tempo/Interpolation/Schleife in der Zeitkonfiguration; Mini Curve Editor links vor dem Zeitschieber; Tastenzeile ab Werk aus; Umschalt-Ziehen kopiert Keys nur wie 3ds Max.
- J: Quad-Menü aus vier Kindfenstern um die Klickstelle; Isolate = nicht Gewähltes ausblenden. Blender: Untermenü "Transform" (G/R/S).
- K: Plattform im Stil Standard zwischen den Drehpunkten Ferse und Zehe (Breite 45 % der Länge), sie ist der IK-Fuß-Griff.
- L: Klick auf einen Buchstaben schaut von dort (wie bisher), nur wo der Würfel ihn nicht verdeckt; Übergang smoothstep.
- M: Weiß oder Schwarz nach WCAG-Leuchtdichte; Hover gelb nur, solange es 4,5:1 hat.
- C: "Open .skin..." geprüft: die Skins der pk3 stehen schon im Rollout "Level of detail and skin" - nicht umgebaut.
- N (CAT, Autodesk-Hilfe 2015 GUID-275009EE "Setup mode and Animation mode", GUID-D6D4CD17 Hierarchy Panel for CATRigs): Setup-Modus = Rig erstellen, Anbauten und Vorschau, Für das Spiel bauen, GLA, animation.cfg, Ziel-GLA und Plätze; Animationsmodus = Animation, Presets, Vorhandene Animation bearbeiten, Übertragen, IK/FK, Auto-Physik, Pose, Bewegungspfade, Kurven und Animationswerkzeuge, Posen-Bibliothek. IK/FK steht im Animationsmodus, weil es Schlüssel setzt (CAT: Limb Animation im Animationsmodus). Ghosts und "Knochen- und Rig-Darstellung" nur im Reiter Anzeige (Max: Display-Panel bzw. Views > Show Ghosting). Die Gruppen zum Aufbauen bleiben im Animationsmodus in ihrem eigenen Reiter (Hierarchie, Dienstprogramme); die Gruppen zum Animieren gibt es im Setup-Modus gar nicht (sie setzen Schlüssel). Setup-Modus: die Figur steht in der Grundstellung der GLA (CAT: "Setup mode is completely static"), kein Gizmo, K/Auto Key/G/R melden den Grund statt zu wirken; nur im Bereich Animation. Taste: Max kennt für CAT keine (max_hotkeys_de.txt) - Strg+Umschalt+M (im Max-Modus frei); Blender: Tab (Edit Mode / Pose Mode der Armatur), Modus-Feld im Kopf der Ansicht. Vorgabe nach dem Laden: Animationsmodus, wenn die GLA Sequenzen hat oder ein Rig da ist, sonst Setup.
- O: wie Viewport Canvas mit Multi/Sub-Object und Substance Painter malt ein Strich in jedes Bild unter dem Pinsel (Stufe 0 der Fläche), das Ziel folgt der zuletzt bemalten Fläche; ein Strich über mehrere Bilder trägt eine Kennung, Rückgängig und Wiederholen nehmen alle seine Schritte zusammen. Alt+Klick (frei: die Navigation nutzt Alt nur mit der Mitteltaste) und die Pipette "Textur aufnehmen" wählen die Textur; ein Klick auf eine Fläche im Explorer ebenso. Vorschaubilder aus der Leinwand bzw. dem Bild der Vorschau, deckend (ohne Alpha), höchstens alle 15 Bilder neu beim Malen. Die Ausklappliste der Shader bleibt darunter (ap9 prüft sie).
- P: Auswahl im Rollout wie Skin "Select: Vertices, Select Element, Backface Cull Vertices"; Backface Cull nach dem Umlauf am Schirm (im Uhrzeigersinn = Vorderseite, FrontCounterClockwise FALSE wie im Spiel). Liste des Weight Tools mit Mittelwert, eintippbar (Enter setzt an allen gewählten Ecken, die Summe bleibt 1). Copy/Paste/Paste-Pos; Paste-Pos-Bone wie Paste-Pos, weil alle Flächen ein Skelett teilen. Mirror Paste: Quelle ist die Seite mit den meisten gewählten Punkten. Die Knochenpaare stehen sichtbar (blau/grün) unter "Knochenpaare"; ändern lassen sie sich wie in Max nur über die Namen bzw. Toleranz (Max paart nach Ort und Schwelle, ohne Liste von Hand). Gewählte Ecken größer (0,2 Zeichenhöhen) mit dunklem Rand. Blender: Panel "Vertex-Gewichte".
- A: "Load mission..." (Menü, Werkzeugknopf, Startfläche, zuletzt geöffnet) ohne Dateidialog: die Missionen der geltenden Spielordner. Gruppen je Archiv der spielbaren Missionen (ab Werk offen, alphabetisch), am Ende "Karten ohne Skripte (Duellkarten)" (ab Werk zu). Die Gruppe ist das Archiv der .ent, ohne .ent das der .bsp - dort liegt die Karte, die die Engine lädt (`Mission::gruppe`). Bei Movie Duels liegen die Karten der Episoden in MD_Maps_Ep1 ... und manche Missionen in MD_Missions_Ep1 ... (Lauf t1411_a2: 23 Gruppen); die Skripte nennt die Gruppe nicht. "Ordner öffnen..." nutzt `spielordnerFuerOrdner` (dieselben Regeln wie für ein Archiv, Unit-Test missiontest); "Archiv öffnen..." ist der bisherige Weg. Ohne Spielordner: Hinweis und drei Knöpfe ("Spielordner einrichten..." öffnet Bearbeiten > Spielordner). `Mission::archivPfad` merkt den vollen Pfad des Archivs der .bsp; er geht in "zuletzt geöffnet" und in `missionArchive` (Save mission), alte Einträge `pk3|...` bleiben lesbar. Lose Dateien (maps/, scripts/ unverpackt) kennt `GamePath` nicht - die Liste nennt nur Karten aus pk3 (nicht nebenbei gebaut). Zwischenspeicher auf der Platte (empfohlen) nicht gebaut.
- E: dritter Reiter "Sequenzen" im Explorer (Max und Blender), im Baum ein Eintrag "Sequenzen (n)", der dorthin springt. Klick spielt ab wie bisher (Doppelklick ebenso), Rechtsklick das bisherige Menü. Strg+F setzt den Fokus, wenn die Maus über dem Explorer steht (wie Blenders Outliner; Max belegt Strg+F sonst mit dem Auswahlbereich). Zeigen (Laden, NPC-Browser) rollt in der Liste, ohne den Reiter zu wechseln; ein ausdrückliches Zeigen (Aktionssuche, Selbsttest) wechselt hin.

## TEST1411c (Abschluss, 14./15.11.2026)

Testfixes und Programmfehler aus den Laeufen der beruehrten Gruppen (Einzelheiten und Laufnamen in `Y:\werkzeuge\test1411\UEBERGABE.md`):

| Gruppe | Befund | Art | Rot -> Gruen |
|---|---|---|---|
| ap2 X17 | Klick auf eine TORSO_-Zeile kam nicht an | Testfehler: der Klickort lag nach dem Rollen hinter dem festen Kopf der Sequenztabelle (Mitte y 459, Koerper ab 472) | Test klickt die Zeile, zu der Zeigen rollt -> 68/0 |
| ap10 A2 | Tooltip "Auf Rig legen" fehlt | Testfehler: der Knopf steht im Setup-Modus, der Test stand seit TEST1411b im Animationsmodus (Maus ueber BOTH_CIN_1) | Gruppe wieder zeigen -> 72/0 |
| ap13k K1-K6 | Formen je Knochen | Testfehler (H: Drehhilfen und Wurzel sind Hilfspunkte) | Kasten + Hilfspunkte -> gruen |
| ap13k T7 | Knochen waehlen trifft nichts | Testfehler (I: Bones-Schalter je Bereich; H: Form Knie-Fuss ist ltibia) | -> gruen |
| ap13k T2 | Klick auf die Kante des Unterschenkels waehlt den Oberschenkel | Testfehler: der Punkt lag auf der Kante, an der seit H zwei Kaesten am Knie zusammenstossen | modellKdFormPunkt nimmt nur Punkte, die 2 px daneben dieselbe Form treffen |
| ap13k S8, pt Rechtsklick | Untermenue des Quad-Menues nicht erreichbar | Programmfehler (J): Untermenues gingen ueber das Nachbarviertel bzw. die leere Flaeche des Quad-Menues auf, am oberen Rand lag das Quad-Menue bei y -208 | Untermenues nach aussen (links/rechts wie 3ds Max), Quad im Fenster, leere Flaeche ohne Maus -> ap13k 66/0, pt gruen |
| ent3b B17 | Verschieben der Wurzel wirkt nicht | Testfehler (N: der Test stand im Setup-Modus) | Animationsmodus -> 38/0 |
| mb3 Plaetze | Ziehen auf einen Platz wirkt nicht | Programmfehler (E: die Sequenzliste war keine Ziehquelle) | modellSequenzZiehen in der Liste -> 23/0 |
| ap9 T1 | Bild zeigen / Auf neue UVs backen nicht in einer Reihe | Programmfehler (O: die Pipette stand davor, gegen den Entwurf AP9) | Pipette dahinter -> 98/0 |
| ap9 T7 Blender | Bild zeigen nicht ganz sichtbar | Testfehler (O: die Gruppe Ziel ist hoeher) | hinrollen -> gruen |

Gruen ohne Aenderung: ent2 23/0, fix612 27/0, ptspiel 2/0, missionfoto 2/0, letzte 6/0, missionsliste 13/0, missionsbau 97/0, karte (vorschau_b) 235/0,
ap11 70/0, test1411 J/O/E 17/0.

Entscheidungen TEST1411c:
- B: die Gruppen ohne Modell aus einer Liste je Bereich (Titel und Reiter wie die Gruppen mit Modell; der Test vergleicht beides je Bereich und
  Bedienmodus), zu und gesperrt, mit eigener Kennung (die Gruppe mit Modell behaelt ihren Zustand). Modellieren zeichnet seine Gruppen selbst
  (Neues Requisit). Auto-Physik fehlt ohne Modell, weil sie erst mit einem Rig erscheint. Die leere Szene ist auf den Platz einer stehenden
  Figur des Spiels eingepasst (OpenJK playerMins/playerMaxs -16/-16/-24 bis 16/16/40), damit beim Laden nichts springt. Ohne Modell bleibt in der
  Ansicht nur die Navigation; der Ladehinweis steht in der aktiven Ansicht. UV-Editor und 2D-Bild stehen als leerer Rahmen an ihrem Platz.
- Quad-Menue: die linken Viertel klappen nach links, die rechten nach rechts aus (wie 3ds Max); am Fensterrand rueckt das ganze Menue nach innen.

## TEST1411d (Reste, 15.11.2026)

Laeufe `Y:\p_ap6\build\laeufe\t1411d_*`, Skripte `Y:\werkzeuge\test1411\d\` (`hb.sh` mit den Daten des Modell-Blocks wie `tools/testrunde.py`
und nur den Bildern der Gruppe).

| Gruppe | Befund | Art | Rot -> Gruen |
|---|---|---|---|
| mb5 Dopesheet | "Dope sheet" ueber Animation > Animationswerkzeuge > Editoren (und die Suche) oeffnete im Modus 3ds Max nichts | Programmfehler (G): `editorOeffnen` setzte nur `kvOffen`, die Zeitleiste stellt es dort in jedem Bild auf den Stand des Mini Curve Editors zurueck | Menue und Suche gehen ueber `modellZeitMiniKurve` wie Knopf und Quad-Menue; t1411d_hb_mb5_en 146/17 -> 167/0 |
| mb5 Quad-Menue | nach Quad > Dope Sheet nahm der naechste Klick (Knopf Mini Curve Editor) nur das Schliessen des Quad-Menues | Programmfehler (J): Eintraege der Viertel (Kindfenster) schliessen das Popup nicht - ImGui-Selectable schliesst nur das Popup-Fenster, in dem es steht | neue Pruefung "der gewaehlte Eintrag schliesst das Quad-Menue" rot (151/16) -> gruen |
| mb5 Alt+D | Regler "Zum Nachbarn mischen" ging nicht auf | Testfehler: nach "Cancel" stand die Maus auf dem Knopf des Reglers (117/1303, ueber dem Editor 0), die Tasten des Graph Editors gelten wie in Blender nur ueber dem Editor | Maus wie vor Alt+S zurueck ueber den Editor -> gruen |
| mb5 Tasten | BHED_MB_BELEGE nicht gesetzt | Umgebung (Testkatalog) | mit der Variable gruen |
| ph | "PH Mission: kein Flug" | Umgebung: ohne BHED_MB7_BSP baut MB7 die Szene auf md_am_sith statt auf der Testkarte mb7raum | mit den Variablen 15/0 |
| rt | Gruppe nicht gelaufen | Umgebung: BHED_RT_GLA | mit der Variable 2/0 |

Dazu: der mb5-Test prueft jetzt alle drei Wege zum Dope Sheet/Kurveneditor (Menue, Knopf des Mini Curve Editors zu/auf, Quad-Menue Curve
Editor/Dope Sheet). Handbuchbild `h_mission_wahl` aus "Load mission" (Gruppe statt "In assets0.pk3"), 1920 statt 1084 breit, die Marke umfasst
die ganze Zeile (der Pfeil verdeckte die Zahl der Skripte). Anleitung "Eine Zwischensequenz des Spiels aendern" (inhalt_kurz2/inhalt_schritte2)
an den neuen Weg. Hinweis von gen_handbuch "2 Eintraege ... nicht mehr gebraucht": "mouse wheel" (es, cs) aus inhalt_mbnav (`X["rad"]` steht nur
per Index in `_taste`) - nicht mehr in der Tabelle, das erzeugte Handbuch byte-gleich. h_ent3a_stapel zeigte in allen Sprachen den Tooltip
"Filter: Tags" (die Maus wurde im selben Schritt gesetzt, in dem Bereich und Stapel wechselten) - Schritt geteilt, Bild ohne Tooltip.

Rest aus Punkt 5 (A), erledigt: die Missionsliste bleibt auf der Platte (`missionsverweise.txt` im Einstellungsordner, Format und Pruefung
in `src/mission.cpp` `missionsVerweiseText`/`missionsVerweiseLesen`, ctest mission), Karten veraenderter oder fehlender Archive fallen beim
Sichern heraus; die Auswahl zeigt gelesene Karten schon waehrend der Hintergrundsuche (`MissionsSuche::istFertig`, Freigabe-Semantik).
Selbsttest `BHED_T1411=z` (t1411d_z1, 5/0): Liste aus der Datei == frisch gelesen (282 Karten Movie Duels), MD_Assets.pk3 mit anderer Zeit ->
genau ihre 5 Karten neu gelesen, alte Zeit aus der Datei; waehrend des Lesens bis 281 Karten in 24 Gruppen. Messung
(`Y:\werkzeuge\test1411\messung_missionsliste.txt`): kalt 2956 ms bis zur vollen Liste, aus der Datei 11.9 ms (Platten-Cache warm, daneben
ein Bildlauf - Richtwerte). missionsliste: zwei Wartezeilen ("t1_sour/md_am_sith in der Auswahl") warten jetzt auch auf das Ende der Suche -
Testfehler durch das gewollte neue Verhalten (rot 10/3, gruen 13/0). app_view3d.cpp braucht /bigobj (C1128, wie selbsttest.cpp).

## TEST1411e (Reste, 15.11.2026)

Laeufe `Y:\p_ap6\build\laeufe\t1411e_*` und `t1411c_e_*`, Skripte `Y:\werkzeuge\test1411\e\` (`hb.sh` uebernimmt nur Bilder des Model-Tabs).

| Punkt | Befund | Art | Rot -> Gruen |
|---|---|---|---|
| stufe9 (5 FEHLER der CW-Laeufe, 7 im Lauf cw_s9_karte1) | "Im Spiel testen" x2, "Modell ziehen ... .pk3", "Pack map as .pk3 ... Import-Ordner", NPC-Wege (x2), Verteilen | Testaufruf: stufe9 arbeitet auf der gebauten Karte des Tutorials (stufe8c/8g); allein gewaehlt lief es auf der offenen Karte des gemeinsamen Anfangs (MAP LEAKED, kein Boden). Schon auf rc612 so (Paket rel612: stufe9 allein 48/7, mit stufe8c,stufe8g 86/0) - weder TEST1411 noch CW | BHED_KARTE_GRUPPEN nimmt fuer stufe9 stufe8g, fuer stufe8g stufe8c dazu; stufe9 allein 86/0 |
| Missionsliste (Rueckfrage Hauptchat) | waehrend die Suche im Hintergrund Karten SORTIERT einfuegte, rutschten die Zeilen - ein Klick traf die falsche Mission (Gruppe jk2: md_afif_sith statt md_am_sith) | Programmfehler (TEST1411d) | Selbsttest `BHED_T1411=s`: rot 387 von 394 Bildern eine andere Zeile unter der Maus (37 -> 281 Karten) -> gruen 0 von 390 (40 -> 282), am Ende dieselbe Zeile, beim Verlassen geordnet (23 Gruppen) |
| JK2-Wartezeilen (Punkt 5) | rot t1411e_jk2_rot 17/5 (Klicks in die wachsende Liste); die 25-Zaehlung war gruen (alle Karten in assets0) | Test an das Verhalten von Load mission: warten auf "fertig gelesen und geordnet", die Maus vorher weg von der Liste | JK2-Daten vorhanden (Steam Jedi Outcast, nur gelesen) |
| Zeitleiste der Vorschau (Punkt 3) | eigener Selbsttest (Modus zeitleiste, vor zeitleisteSchritte; auch in Modus 1) | Test neu | mit zurueckgenommener F-Behebung rot: Knoepfe 198 -> 208 -> 217 -> 227 px bei Bild 0/10/100/1000 (49/1) -> gruen 50/0 |
| Mauspunkt (Punkt 2) | ohne Auswahl X/Y/Z des Punkts unter der Maus auf der Rasterebene der Ansicht darunter, ausserhalb der Ansichten bleibt der letzte Punkt | neu | Selbsttest `BHED_T1411=x`: 7 Faelle (eine Ansicht oben/vorne/links/Perspektive, vier Ansichten), Abweichung hoechstens 0,00497 (Rundung 0,005); ImGui rundet die Maus auf ganze Bildpunkte. Beim Ansehen der Bilder gefunden: mit einem gewaehlten Knochen ohne Mitte (Bereich Ansehen, h_ap1_pruefen "Bone rhand Selected") stand der Mauspunkt da - rot t1411e_x_rot 5/1, jetzt nur bei "None Selected": gruen 6/0 |
| Knochenpaare von Hand (Punkt 4) | Klappliste je Zeile, neues Paar aus zwei beliebigen Knochen, "Paare nach den Namen"; gilt fuer Spiegeln, Mirror Paste, Blender-Spiegeln und Spiegeln beim Malen | neu (modell::KnochenSpiegel) | ctest modell_gewichte2 (10 neue Pruefungen), Selbsttest `BHED_T1411=w` |
| ap8 | Klicks auf Knoepfe unter dem sichtbaren Teil des Befehlsfelds kamen nicht an (90/40) | Testfehler seit TEST1411 P (das Feld wurde hoeher; ap8 lief seitdem nicht) | vor jedem Klick ins Feld ap3::insFeld |
| ap13s | 4 FEHLER "kein Ordner zum Speichern" | Umgebung: ohne BHED_FOTOS | mit BHED_FOTOS |
| Dialogbilder (Punkt 6) | h_start 1171, h_spielordner 1798, h_mission_neu 1054, h_mission_speichern 1240, k_pk3/s_pk3_art 1240 breit | hb::breit1920: das Fenster in voller Groesse, links und rechts das Programm (wie h_mission_wahl) | 6 Sprachen neu |
| Dialogbilder, Nacharbeit (Hauptchat) | 1. h_mission_neu/_speichern zeigten den Laufordner ("Y:\p_ap6\build\laeufe\t1411e_voll_de\missionen\...") - e/voll.sh setzte BHED_MISSIONEN/BHED_HB_ZIEL nicht; 2. Ziffernkreise und Rahmen 1,75-mal so gross (bilder.py bemass sie nach der Bildbreite 1920) und verdeckten die Feldbeschriftungen | Bildlauf (Aufruf) bzw. bilder.py | neuer Pruefschritt im grossen Bildlauf "zeigt keinen Pfad des Laufs" (Y:\, p_ap6, laeufe, Users\): rot ohne X: (beide Bilder FEHLER), gruen mit X: wie skripte/ab/bilder.sh; die .rect traegt die Fensterbreite als Bezug der Markierungen (Kreise wie vor TEST1411e), die Plakette steht bei breiten Ausschnitten links neben dem Rahmen statt auf der Beschriftung; 7 Bilder (mit h_mission_wahl) x 6 Sprachen neu |

Entscheidungen: Die Paare von Hand gelten fuer alle Wege des Spiegelns (auch im Modus Blender, der sie nicht anzeigt) - sie sind eine Eigenschaft
des Skeletts. Max paart Knochen im Mirror Mode ueber die Lage; eine Liste von Hand gibt es dort nicht (shank P: "sichtbar und aenderbar").
Waehrend gelesen wird, stehen neu gelesene Karten am Ende ("Just read", mit ihrer Gruppe); die Zeile der Suche bleibt bis zum Ordnen stehen.
Die Statuszeile behaelt den letzten Mauspunkt, wenn die Maus die Ansichten verlaesst (wie Max die Felder der Koordinatenanzeige).

Handbuchbilder: alle 33 Model-Tab-Gruppen in 6 Sprachen neu aufgenommen (198 Laeufe, 0 FEHLER; `e/hb.sh` uebernimmt nur Bilder des Model-Tabs,
hilfe1 schreibt keine Bilder anderer Seiten). Byte-gleich blieben alle bis auf die, deren Statuszeile jetzt den Mauspunkt zeigt: h_ai_bibliothek,
h_mbn_orbit, h_ph_sprung, h_pr_gang, h_rt_ketten, h_rt_uebertragen, h_uv_schnell (je 6 Sprachen, angesehen). Nicht uebernommen (Unterschied nicht
aus TEST1411e): h_mb7_* (mit BHED_MB7_BSP die Testkarte mb7raum statt md_am_sith - fast schwarz), h_mb3_animation (Bild der Animation), h_ap12_*
(Zeit des Auto-Skin), h_mb3_speichern/h_md_karte (Pfad des Laufordners), m_npc (397 statt 367 Figuren - Datenstand), h_modell/m_*/k_modell_leiste
(2-93 Bildpunkte Rauschen). Dazu die Dialogbilder 1920 breit (h_start, h_spielordner, h_mission_wahl, h_mission_neu, h_mission_speichern,
k_pk3, s_pk3_art), nach der Nacharbeit ohne Pfad des Laufs (X:\missionen, X:\zz_taspir_tut.pk3) und mit Markierungen in der alten Groesse.
Bekannt, nicht aus TEST1411e: h_mb7_* (Stand TEST1411c) zeigen im Feld Output folder ebenfalls einen Laufordner.

## Offen

- Handbuch: Texte zu A-P in 6 Sprachen, `gen_handbuch.py --pruefen` gruen. Alle Bilder des Model-Tabs der Gruppen von TEST1411 in 6 Sprachen neu,
  mit TEST1411d auch h_mb5_*, h_ph_*, h_rt_*/h_pr_gang, h_ap6/ap11/ap12/ap13s/ent3a und h_mission_wahl.
- Handbuch-Gruppe mb3: zwei Pruefungen an I/N angepasst (Silhouette ohne Knochen, 3/8 Tasten).
- Handbuchbilder unter 1920 Breite: ausser den sechs Dialogbildern von TEST1411e noch rund 440 je Sprache (b_*, c_*, e_*, k_*, s_*, t*, vg_*, vt_* ...;
  darunter t4_1/t4_4, dieselben Fenster wie h_mission_neu/_speichern).
- Weights: die Zeile "Bone pairs (by names)" zaehlt seit TEST1411e auch Paare von Hand - die Beschriftung nennt nur die Namen.
- h_mb7_*: ob die Handbuchbilder auf md_am_sith (Stand im Handbuch) oder der Testkarte mb7raum (BHED_MB7_BSP wie tools/testrunde.py) entstehen
  sollen, ist offen; mit mb7raum ist die Szene fast schwarz. m_npc zeigt noch 367 Figuren (heute 397).

## Changelog

## English

- Model tab: the model no longer shows twice when zooming, and the object level shows no wireframe (edges only on sub-object levels); ghosts only in the Animation area.
- Model tab: the timeline buttons stand still while the frame number grows.
- Animation: bones are visible, selectable and rotatable from the start like in 3ds Max, without a control rig.
- Bones: one clean box per bone along the chain, twist helpers as small points.
- Timeline and status bar like 3ds Max in every area: time slider above the track bar, one status line, no text under the timeline.
- Right-click in the view: the quad menu of 3ds Max with Move, Rotate and Scale; in Blender mode Transform in the context menu.
- Control rig: a foot platform on the ground like CAT, with roll over heel and toe.
- ViewCube: compass letters behind the cube are hidden and cannot be clicked; the camera no longer swings.
- Labels over the view stay readable on any background (silhouette, white).
- Load .gla...: the list of all .gla files in the game archives, plus Open file... for a loose one.
- Motion panel: Setup Mode and Animation Mode like CAT (Blender: Edit Mode / Pose Mode). Setup shows the setup pose and sets no keys; ghosts and bone display moved to the Display panel.
- Painting: a stroke paints into the texture of every surface under the brush, across images; one undo takes the whole stroke back. The target follows the painted surface; thumbnails, Alt+click and a picker choose it; "Paint target texture only" keeps the old way.
- Weights like the Skin modifier of 3ds Max: select vertices in the panel (Vertices, Select Element, Backface Cull Vertices), the Weight Tool list with typed values, Copy/Paste/Paste-Pos, a dockable Weight Table and Mirror Paste.
- Load mission no longer opens a file dialog: the missions of the game folders appear at once, grouped by archive, with Open folder... and Open archive...; recently opened works without a chosen archive.
- Scene Explorer: the sequences have their own tab with search (Ctrl+F), group and sortable columns name, frames and loop.
- Model tab without a model: the same layout as with one - scene explorer with tabs, search and filters, views with grid, ViewCube and header, the command panel with its tabs and greyed-out groups; NPC browser and Open .glm in the middle of the view.
- Quad menu: submenus open outwards (left quadrants to the left, right ones to the right) and the menu stays inside the window.
- Sequences tab: a row can be dragged onto a slot of the map GLA again; Painting: Show image and Bake to new UVs stand side by side again, the picker below.
- Model tab, 3ds Max mode: Dope sheet and Curve editor from the Animation menu and the action search open the mini curve editor again; choosing an entry of the quad menu closes it, as in 3ds Max.
- Load mission: the mission list is kept on disk - from the second time on it is there at once, also after a restart; a changed archive is read again. While reading, finished maps come at the end of the list and nothing moves under the mouse; when reading is done they go into their groups.
- Model tab, 3ds Max mode: with nothing selected the status line shows X/Y/Z of the mouse point on the grid of the view under the mouse.
- Weights: the bone pairs of the mirror can be changed by hand (a list per row, any two bones in the last row, Pairs from names).

## Deutsch

- Model-Tab: Das Modell erscheint beim Zoomen nicht mehr doppelt, auf der Objektebene ohne Drahtgitter (Kanten nur auf den Unterobjekt-Ebenen); Geister nur im Bereich Animation.
- Model-Tab: Die Knöpfe der Zeitleiste stehen still, wenn die Bildzahl wächst.
- Animation: Knochen sind wie in 3ds Max von Anfang an sichtbar, wählbar und drehbar, ohne Control Rig.
- Knochen: ein sauberer Kasten je Knochen entlang der Kette, Drehhilfen als kleine Punkte.
- Zeitleiste und Statuszeile wie 3ds Max in jedem Bereich: Zeitschieber über der Spurleiste, eine Statuszeile, kein Text unter der Zeitleiste.
- Rechtsklick in die Ansicht: das Quad-Menü von 3ds Max mit Move, Rotate und Scale; im Blender-Modus Transform im Kontextmenü.
- Control Rig: eine Fußplattform am Boden wie CAT, mit Abrollen über Ferse und Zehe.
- ViewCube: Kompassbuchstaben hinter dem Würfel sind verdeckt und nicht klickbar; die Kamera schwingt nicht mehr.
- Beschriftungen über der Ansicht bleiben auf jedem Grund lesbar (Silhouette, Weiß).
- Load .gla...: die Liste aller .gla der Spielarchive, dazu Datei öffnen... für eine lose.
- Motion-Feld: Setup-Modus und Animationsmodus wie CAT (Blender: Bearbeitungs-/Posenmodus). Setup zeigt die Setup-Pose und setzt keine Schlüssel; Geister und Knochen-Darstellung stehen im Reiter Anzeige.
- Painting: Ein Strich malt in die Textur jeder Fläche unter dem Pinsel, über Bilder hinweg; ein Rückgängig nimmt den ganzen Strich. Das Ziel folgt der bemalten Fläche; Vorschaubilder, Alt+Klick und eine Pipette wählen es; "Nur Ziel-Textur bemalen" hält den alten Weg.
- Weights wie der Skin-Modifier von 3ds Max: Ecken im Feld wählen (Ecken, Element wählen, Rückseiten-Ecken ausblenden), die Liste des Weight Tools mit eintippbaren Werten, Kopieren/Einfügen/Einfügen nach Ort, eine andockbare Gewichtstabelle und Spiegeln einfügen.
- Mission laden öffnet keinen Dateidialog mehr: die Missionen der Spielordner stehen sofort da, nach Archiv gruppiert, mit Ordner öffnen... und Archiv öffnen...; zuletzt geöffnet geht auch ohne gewähltes Archiv.
- Szenen-Explorer: Die Sequenzen haben einen eigenen Reiter mit Suche (Strg+F), Gruppe und sortierbaren Spalten Name, Bilder und Schleife.
- Model-Tab ohne Modell: dieselbe Anordnung wie mit Modell - Szenen-Explorer mit Reitern, Suche und Filtern, Ansichten mit Raster, ViewCube und Kopfzeile, das Befehlsfeld mit seinen Reitern und ausgegrauten Gruppen; NPC-Browser und .glm öffnen mitten in der Ansicht.
- Quad-Menü: Untermenüs gehen nach außen auf (linke Viertel nach links, rechte nach rechts), und das Menü bleibt im Fenster.
- Reiter Sequenzen: eine Zeile lässt sich wieder auf einen Platz der Karten-GLA ziehen; Painting: Bild zeigen und Auf neue UVs backen stehen wieder nebeneinander, die Pipette darunter.
- Model-Tab, Modus 3ds Max: Dope Sheet und Kurveneditor aus dem Menü Animation und der Aktionssuche öffnen den Mini Curve Editor wieder; ein gewählter Eintrag des Quad-Menüs schließt es wie in 3ds Max.
- Mission laden: die Missionsliste bleibt auf der Platte gespeichert - ab dem zweiten Mal steht sie sofort da, auch nach einem Neustart; ein verändertes Archiv wird neu gelesen. Während des Lesens kommen fertige Karten ans Ende der Liste, unter der Maus verschiebt sich nichts; ist alles gelesen, kommen sie in ihre Gruppen.
- Model-Tab, Modus 3ds Max: ohne Auswahl zeigt die Statuszeile X/Y/Z des Mauspunkts auf dem Raster der Ansicht unter der Maus.
- Weights: die Knochenpaare des Spiegelns lassen sich von Hand ändern (Liste je Zeile, zwei beliebige Knochen in der letzten Zeile, Paare nach den Namen).

## 中文

- 模型标签: 缩放时模型不再显示两次, 对象层级不显示线框 (只在子对象层级显示边); 残影只在动画区域显示。
- 模型标签: 帧数增加时时间轴按钮保持不动。
- 动画: 与 3ds Max 一样, 骨骼从一开始就可见、可选、可旋转, 无需控制绑定。
- 骨骼: 每根骨骼沿骨骼链一个整洁的方框, 扭转辅助骨骼显示为小点。
- 时间轴和状态栏在所有区域都与 3ds Max 相同: 轨迹栏上方的时间滑块, 一行状态栏, 时间轴下方没有文字。
- 在视图中右键: 3ds Max 的四元菜单, 含移动、旋转、缩放; Blender 模式在上下文菜单中提供变换。
- 控制绑定: 与 CAT 相同的地面脚部平台, 可绕脚跟和脚尖滚动。
- ViewCube: 立方体后面的指南针字母被遮挡且不可单击; 相机不再摆动。
- 视图上方的标签在任何背景上都保持可读 (剪影、白色)。
- 加载 .gla...: 游戏存档中所有 .gla 的列表, 另有 打开文件... 用于单独文件。
- 运动面板: 与 CAT 相同的设置模式和动画模式 (Blender: 编辑模式 / 姿态模式)。设置模式显示设置姿态且不设置关键帧; 残影和骨骼显示移到显示面板。
- 绘制: 一笔绘制到画笔下每个表面的纹理中, 跨越图像; 一次撤销撤回整笔。目标跟随绘制的表面; 缩略图、Alt+单击和吸管可选择目标; "仅绘制目标纹理" 保留旧方式。
- 权重与 3ds Max 的蒙皮修改器相同: 在面板中选择顶点 (顶点、选择元素、背面消隐顶点), 权重工具列表可输入数值, 复制/粘贴/按位置粘贴, 可停靠的权重表和镜像粘贴。
- 加载任务不再打开文件对话框: 游戏文件夹中的任务立即按存档分组显示, 并提供 打开文件夹... 和 打开存档...; 最近打开在未选择存档时也可用。
- 场景资源管理器: 序列有独立的标签页, 带搜索 (Ctrl+F)、分组和可排序的列 名称、帧数和循环。
- 未加载模型的模型标签: 布局与加载后相同 - 带选项卡、搜索和筛选的场景资源管理器, 带网格、ViewCube 和标题栏的视图, 带选项卡和灰显组的命令面板; 视图中央有 NPC 浏览器和打开 .glm。
- 四元菜单: 子菜单向外展开 (左侧象限向左, 右侧向右), 菜单保持在窗口内。
- 序列选项卡: 行又可以拖到地图 GLA 的槽位上; 绘制: 显示图像和烘焙到新 UV 又并排显示, 吸管在下方。
- 模型标签, 3ds Max 模式: 从动画菜单和操作搜索打开摄影表和曲线编辑器时又会打开迷你曲线编辑器; 选择四元菜单中的条目后菜单关闭, 与 3ds Max 相同。
- 加载任务: 任务列表保存在磁盘上 - 从第二次起立即显示, 重启后也是如此; 已更改的存档会重新读取。读取过程中, 已读完的地图排在列表末尾, 鼠标下方的内容不会移动; 全部读完后再归入各自的组。
- 模型标签页, 3ds Max 模式: 未选择任何对象时, 状态栏显示鼠标在其下方视图网格上的点的 X/Y/Z。
- 权重: 镜像的骨骼对可以手动更改 (每行一个列表, 最后一行可配对任意两根骨骼, 按名称配对)。

## 日本語

- モデルタブ: ズーム時にモデルが二重に表示されなくなり、オブジェクトレベルではワイヤーフレームを表示しません (エッジはサブオブジェクトレベルのみ)。ゴーストはアニメーション領域のみ。
- モデルタブ: フレーム番号が増えてもタイムラインのボタンは動きません。
- アニメーション: 3ds Max と同じく、コントロールリグなしで最初からボーンが表示され、選択・回転できます。
- ボーン: チェーンに沿ってボーンごとに整ったボックスが 1 つ、ねじれ補助ボーンは小さな点。
- タイムラインとステータスバーはどの領域でも 3ds Max と同じ: トラックバーの上のタイムスライダー、1 行のステータスバー、タイムラインの下に文章なし。
- ビューで右クリック: 移動・回転・スケール付きの 3ds Max クアッドメニュー。Blender モードではコンテキストメニューにトランスフォーム。
- コントロールリグ: CAT と同じ地面のフットプラットフォーム、かかととつま先で転がせます。
- ViewCube: キューブの後ろの方位文字は隠れてクリックできません。カメラが揺れなくなりました。
- ビュー上のラベルはどの背景でも読めます (シルエット、白)。
- .gla を読み込む...: ゲームアーカイブ内のすべての .gla の一覧と、単独ファイル用の ファイルを開く...。
- モーションパネル: CAT と同じセットアップモードとアニメーションモード (Blender: 編集モード / ポーズモード)。セットアップはセットアップポーズを表示しキーを設定しません。ゴーストとボーン表示は表示パネルへ移動。
- ペイント: ストロークはブラシの下のすべてのサーフェスのテクスチャに、画像をまたいでペイントします。1 回の元に戻すでストローク全体を戻します。ターゲットはペイントしたサーフェスに追従し、サムネイル、Alt+クリック、スポイトで選択できます。「ターゲットテクスチャのみペイント」で従来の方法。
- ウェイトは 3ds Max のスキンモディファイヤと同じ: パネルで頂点を選択 (頂点、要素を選択、背面頂点をカリング)、値を入力できるウェイトツールの一覧、コピー/貼り付け/位置で貼り付け、ドッキング可能なウェイトテーブルとミラー貼り付け。
- ミッションの読み込みでファイルダイアログが開かなくなりました: ゲームフォルダーのミッションがすぐにアーカイブ別に表示され、フォルダーを開く... と アーカイブを開く... があります。最近開いた項目はアーカイブを選ばなくても使えます。
- シーンエクスプローラー: シーケンスに検索 (Ctrl+F)、グループ、並べ替え可能な列 名前・フレーム・ループ付きの独自タブ。
- モデルなしのモデルタブ: 読み込み後と同じ配置 - タブ・検索・フィルター付きのシーン エクスプローラ、グリッド・ViewCube・ヘッダー付きのビュー、タブとグレー表示のグループを持つコマンド パネル。ビュー中央に NPC ブラウザーと .glm を開く。
- クアッド メニュー: サブメニューは外側に開き (左の区画は左へ、右は右へ)、メニューはウィンドウ内に収まります。
- シーケンス タブ: 行を再びマップ GLA のスロットへドラッグできます。ペイント: 画像を表示と新しい UV にベイクが再び並び、スポイトはその下。
- モデルタブ、3ds Max モード: アニメーションメニューとアクション検索のドープシートとカーブエディタで、再びミニカーブエディタが開きます。クアッドメニューの項目を選ぶと、3ds Max と同じくメニューが閉じます。
- ミッションの読み込み: ミッション一覧をディスクに保存します。2 回目からは再起動後でもすぐに表示され、変更されたアーカイブは読み直します。読み込み中、読み終えたマップは一覧の末尾に並び、マウスの下は動きません。すべて読み終えるとグループに整理されます。
- モデル タブ、3ds Max モード: 何も選択していないとき、ステータス行にマウスの下のビューのグリッド上の点の X/Y/Z を表示します。
- ウェイト: ミラーのボーンのペアを手動で変更できます (行ごとのリスト、最後の行で任意の 2 本のボーン、名前でペア)。

## Español

- Pestaña Modelo: el modelo ya no aparece doble al hacer zoom y el nivel de objeto no muestra malla de alambre (aristas solo en los subniveles); fantasmas solo en el área Animación.
- Pestaña Modelo: los botones de la línea de tiempo no se mueven cuando crece el número de fotograma.
- Animación: los huesos se ven, se seleccionan y se giran desde el principio como en 3ds Max, sin control rig.
- Huesos: una caja limpia por hueso a lo largo de la cadena, ayudantes de torsión como puntos pequeños.
- Línea de tiempo y barra de estado como en 3ds Max en todas las áreas: control de tiempo sobre la barra de pistas, una sola línea de estado, sin texto bajo la línea de tiempo.
- Clic derecho en la vista: el menú cuádruple de 3ds Max con Mover, Rotar y Escalar; en el modo Blender, Transformar en el menú contextual.
- Control rig: una plataforma de pie en el suelo como CAT, con giro sobre talón y punta.
- ViewCube: las letras de la brújula detrás del cubo quedan ocultas y no se pueden pulsar; la cámara ya no oscila.
- Las etiquetas sobre la vista se leen sobre cualquier fondo (silueta, blanco).
- Cargar .gla...: la lista de todos los .gla de los archivos del juego, y Abrir archivo... para uno suelto.
- Panel Movimiento: Modo de configuración y Modo de animación como CAT (Blender: Modo Edición / Modo Pose). La configuración muestra la pose de configuración y no pone claves; los fantasmas y la vista de huesos pasan al panel Visualización.
- Pintura: un trazo pinta en la textura de cada superficie bajo el pincel, a través de imágenes; un deshacer quita el trazo entero. El destino sigue a la superficie pintada; miniaturas, Alt+clic y un cuentagotas lo eligen; "Pintar solo la textura de destino" mantiene el modo anterior.
- Pesos como el modificador Piel de 3ds Max: seleccionar vértices en el panel (Vértices, Seleccionar elemento, Descartar vértices traseros), la lista de la Herramienta de peso con valores escritos, Copiar/Pegar/Pegar por posición, una Tabla de pesos acoplable y Pegar simetría.
- Cargar misión ya no abre un diálogo de archivo: las misiones de las carpetas del juego aparecen al momento, agrupadas por archivo, con Abrir carpeta... y Abrir archivo...; lo abierto recientemente funciona sin archivo elegido.
- Explorador de escena: las secuencias tienen su propia pestaña con búsqueda (Ctrl+F), grupo y columnas ordenables nombre, fotogramas y bucle.
- Pestaña Modelo sin modelo: la misma disposición que con uno - explorador de escena con pestañas, búsqueda y filtros, vistas con cuadrícula, ViewCube y cabecera, el panel de comandos con sus pestañas y grupos en gris; Navegador de NPC y Abrir .glm en el centro de la vista.
- Menú cuádruple: los submenús se abren hacia fuera (los cuadrantes izquierdos a la izquierda, los derechos a la derecha) y el menú queda dentro de la ventana.
- Pestaña Secuencias: una fila se puede volver a arrastrar a una ranura de la GLA del mapa; Pintura: Mostrar imagen y Hornear en UV nuevas vuelven a estar juntos, el cuentagotas debajo.
- Pestaña Modelo, modo 3ds Max: Hoja de claves y Editor de curvas desde el menú Animación y la búsqueda de acciones vuelven a abrir el mini editor de curvas; elegir una entrada del menú cuádruple lo cierra, como en 3ds Max.
- Cargar misión: la lista de misiones se guarda en disco; desde la segunda vez aparece al momento, también tras reiniciar; un archivo modificado se vuelve a leer. Mientras se lee, los mapas terminados van al final de la lista y nada se mueve bajo el ratón; al terminar la lectura pasan a sus grupos.
- Pestaña de modelo, modo 3ds Max: sin selección, la línea de estado muestra X/Y/Z del punto bajo el ratón en la cuadrícula de esa vista.
- Pesos: las parejas de huesos de la simetría se pueden cambiar a mano (una lista por fila, dos huesos cualesquiera en la última fila, Parejas por nombre).

## Čeština

- Karta Model: model se při zoomu už nezobrazuje dvakrát a na úrovni objektu bez drátového modelu (hrany jen na podúrovních); duchové jen v oblasti Animace.
- Karta Model: tlačítka časové osy stojí, když roste číslo snímku.
- Animace: kosti jsou jako v 3ds Max od začátku viditelné, vybíratelné a otočné, bez control rigu.
- Kosti: jeden čistý kvádr na kost podél řetězce, pomocné kosti natočení jako malé body.
- Časová osa a stavový řádek jako v 3ds Max v každé oblasti: časový posuvník nad stopovou lištou, jeden stavový řádek, žádný text pod časovou osou.
- Pravý klik do pohledu: čtyřdílné menu 3ds Max s Move, Rotate a Scale; v režimu Blender Transformace v kontextovém menu.
- Control rig: plošina chodidla na zemi jako CAT, s odvalením přes patu a špičku.
- ViewCube: písmena kompasu za krychlí jsou skrytá a nelze na ně kliknout; kamera už nekmitá.
- Popisky nad pohledem zůstávají čitelné na každém pozadí (silueta, bílá).
- Načíst .gla...: seznam všech .gla v archivech hry a Otevřít soubor... pro volný soubor.
- Panel Pohyb: režim nastavení a režim animace jako CAT (Blender: režim úprav / režim pózy). Nastavení ukazuje pozici nastavení a nenastavuje klíče; duchové a zobrazení kostí jsou v panelu Zobrazení.
- Malování: tah maluje do textury každé plochy pod štětcem, přes obrázky; jedno zpět vrátí celý tah. Cíl sleduje malovanou plochu; vybírají ho náhledy, Alt+klik a kapátko; "Malovat jen cílovou texturu" zachová starý způsob.
- Váhy jako modifikátor Skin v 3ds Max: výběr vrcholů v panelu (Vrcholy, Vybrat prvek, Vynechat zadní vrcholy), seznam Weight Tool se zadávanými hodnotami, Kopírovat/Vložit/Vložit podle místa, ukotvitelná tabulka vah a Zrcadlově vložit.
- Načíst misi už neotevírá dialog souboru: mise ze složek hry se zobrazí hned, seskupené podle archivu, s Otevřít složku... a Otevřít archiv...; naposledy otevřené funguje i bez zvoleného archivu.
- Průzkumník scény: sekvence mají vlastní kartu s hledáním (Ctrl+F), skupinou a řaditelnými sloupci název, snímky a smyčka.
- Karta Model bez modelu: stejné uspořádání jako s modelem - průzkumník scény s kartami, hledáním a filtry, pohledy s mřížkou, ViewCube a záhlavím, příkazový panel s kartami a zašedlými skupinami; prohlížeč NPC a Otevřít .glm uprostřed pohledu.
- Čtyřdílné menu: podmenu se otevírají směrem ven (levé kvadranty doleva, pravé doprava) a menu zůstává v okně.
- Karta Sekvence: řádek lze opět přetáhnout na slot GLA mapy; Malování: Zobrazit obrázek a Zapéct do nových UV stojí opět vedle sebe, kapátko pod nimi.
- Karta Model, režim 3ds Max: Dope Sheet a Editor křivek z nabídky Animace a z hledání akcí znovu otevřou mini editor křivek; zvolená položka čtyřdílné nabídky ji zavře jako v 3ds Max.
- Načíst misi: seznam misí se ukládá na disk - od druhého otevření je hned k dispozici, i po restartu; změněný archiv se načte znovu. Během čtení přibývají hotové mapy na konci seznamu a pod myší se nic nehýbe; po dočtení se zařadí do svých skupin.
- Záložka modelu, režim 3ds Max: bez výběru ukazuje stavový řádek X/Y/Z bodu pod myší na mřížce daného pohledu.
- Váhy: páry kostí zrcadlení lze měnit ručně (seznam v každém řádku, libovolné dvě kosti v posledním řádku, Páry podle názvů).
