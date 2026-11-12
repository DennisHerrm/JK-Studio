# Notizen AP4 (12.11.): Model-Tab, Aufbau wie Blender - Werkzeugleiste, Outliner oben rechts, Seitenleiste - MBENTWURF Teil B, rc612

Paket AP4 aus `auftrag_mbentwurf_b_1011.md` (Opus 5.5, ein Agent): die Zeilen R16-R18, X2, X15 (Ort), X28, F2, F17, P0b, P8, M0b,
U0b, W0b, T0b, A0b, A11 der Abgleichtabelle `plaene/modellblock_stufe0/abgleich_programm_entwurf.md`. Maßstab: der freigegebene Entwurf
(`entwurf_modelltab.html`, Bedienmodus „wie Blender“: `#links` = Werkzeugleiste, `#rechts` = Outliner mit 44 % über den Eigenschaften,
Kopf der Ansicht „Benutzer-Perspektive · <Modus>“, unten rechts „N Seitenleiste · T Werkzeuge“, `werkzeugeModus()` je Bereich) und shank
10.11.: „es geht ja nicht nur um controls like blender oder 3ds max sondern das aussehen“. Worktree `Y:\p_ap4`, Zweig `p_ap4` auf karte1
`798eb2b8b`. Selbsttest `gui/selbsttest_ap4.inc` (`BHED_EDITORTEST=modellblock`, `BHED_MODELL_NUR=ap4 | ap4foto`; Handbuch
`BHED_EDITORTEST=handbuch`, `BHED_HB_NUR=ap4`), Skripte und Läufe `Y:\werkzeuge\ap4\`.

## Was sich ändert

Neue Datei `gui/app_modelltab_blender.cpp` (Zustand `ModellBlender`/`modellBl()` im eigenen Block am Ende von `gui/modelltab.h`).
Wie 3ds Max bleibt alles, wie es war (links Szenen-Explorer, rechts nur das Befehlsfeld, Kopf `[+] [Ansicht] [Standard]`, T = Ansicht
Oben, N = Auto Key).

| Teil | wie Blender | Vorbild / Quelle | Datei |
|---|---|---|---|
| **Werkzeugleiste (T)** | linke Spalte des Model-Tabs (2,6 Zeichenhöhen, Entwurf 44 px bei 13 px Schrift): Auswählen (B), Bewegen (G), Drehen (R), Skalieren (S), Trennstrich, darunter die Werkzeuge des Bereichs; das gewählte in der Betonungsfarbe, Tooltip mit Name, Taste, Hinweis; T blendet aus (ein schmaler Streifen mit Pfeil bleibt) und ein; Rechtsklick: Werkzeugleiste / Seitenleiste | Blender-Handbuch „3D Viewport > Toolbar“; `blender_default.py` km_view3d_generic: T `wm.context_toggle space_data.show_region_toolbar` | `app_modelltab_blender.cpp`, linke Spalte `app_anordnung.inc` (`drawEbenenInhalt`), Breite `app_internal.h` (`ebenenSpalte` → `modellLinksSpalte`) |
| Werkzeuge je Bereich | Ansehen/Prüfen: Messen, Fläche wählen, Knochen wählen, Foto · Modellieren: Extrudieren (E), Einfügen (I), Fase (Strg+B), Schleifenschnitt (Strg+R), Messer (K), Zusammenführen (M) · UV: Naht (Strg+E), Abwickeln (U), Entspannen, Packen (Strg+P) · Weights: Pinsel, Weichzeichnen, Verlauf, Gewicht holen (Umschalt+X) · Painting: Malen, Füllen, Klonen, Verwischen, Maske · Animation: Pose kopieren, Gespiegelt einfügen, Ausdünnen (Schnittstelle `modellAnimWerkzeug*` von AP10) | Entwurf `werkzeugeModus()`; Aktionen und Tasten aus den Tabellen der Bereiche (AP6/AP7/AP8/AP9/AP10) - dieselbe Aktion wie Knopf im Feld, Taste, Suche, Rechtsklick | `app_modelltab_blender.cpp` |
| **Kopf der Ansicht** | „View · Benutzer-Perspektive · <Modus> v“: Objektmodus, Bearbeitungsmodus (Modellieren, UV), Weight Paint, Texture Paint, Pose-Modus; Klick öffnet die Moduswahl (wie Strg+Tab, mit dem Bereich daneben) und wechselt den Bereich; in einer schmalen Ansicht fällt erst der Name, dann der Modus weg | Blender: Modus-Ausklapper im Kopf, `view3d.object_mode_pie_or_toggle` | `modellBlModusKopf`, Aufruf `app_modelltab_kopf.cpp` (Blender-Zweig von `drawModellKopfzeile`) |
| **Outliner oben rechts** | der Baum von AP2 (Titel „Outliner“, Umschalter rechts) über dem Eigenschaften-Editor von AP3, 44 % der Spaltenhöhe, Trennkante ziehbar | Entwurf `outlinerKlein` (flex 0 0 44 %) | `drawModellBlOutliner`, Aufruf `drawModellArbeitsflaeche` (`app_modelltab_ansicht.cpp`) |
| **Seitenleiste (N)** | rechts neben den Ansichten (15 Zeichenhöhen, höchstens ein Drittel), senkrechte Reiter „Objekt“ / „Ansicht“ wie Blender (Item / View): Auswahl (Fläche: Position und Abmessungen des Kastens in der Pose; Knochen bzw. Griff: Position und Drehung XYZ in der Pose), letzte Messung; Sichtfeld, Projektion, Ausgewählte/Alles einpassen | Blender-Handbuch „3D Viewport > Sidebar“; N `space_data.show_region_ui`; Euler XYZ wie Blenders Vorgabe | `drawModellSeitenleiste` |
| Hinweis | unten rechts „N Seitenleiste · T Werkzeuge“ (außer Ansehen/Prüfen, dort steht laut Entwurf die Detailstufe) | Entwurf | `modellBlUeberlagerung` |
| **Messen** (P8) | zwei Klicks auf das Modell oder Ziehen von einem Punkt zum anderen; Punkt = vorderstes Dreieck unter der Maus, darin perspektivisch richtig (Anteile der Ecken durch ihre Tiefe w); Strecke gelb mit Abstand in Spieleinheiten, auch in der Meldung und der Seitenleiste; wie 3ds Max über „Alle Aktionen suchen“ und den Rechtsklick („Abstand messen“, 3ds Max Measure Distance) | Blender-Werkzeug „Measure“ | `punktUnter`, `modellBlKlick` (Aufruf in `eineAnsicht`) |
| Knochen wählen | zeigt die Knochen; ein Klick wählt den Knochen, dessen Gelenk am nächsten liegt (höchstens eine Zeichenhöhe; ohne versteckte und eingefrorene), auch im Outliner | - | `knochenUnter` |
| Foto | dieselbe Aufnahme wie „Foto …“ der Gruppe Modell (1920 breit, Dateidialog) | - | `fotoSpeichern` |
| **Anordnung je Bedienmodus** | jeder Bedienmodus behält seine Fensteranordnung (`Settings::fenster` „b2@max“ / „b2@blender“, nicht in der Liste der benannten); beim ersten Wechsel gilt die laufende weiter | - | `dock::modusAnordnung` (`fenster_dock.cpp`), Aufruf `modellBedienungSetzen` |
| Umschalten ohne Verlust | Auswahl, Knochen, Sequenz, Bild, Bereich, die Leisten bleiben; das Panel der linken Spalte heißt wie Blender „Werkzeugleiste“ (Fenster-Menü, eigenes Fenster) | - | `modellLinksTitel` (`fenster_dock.cpp`, `app_dockspace.inc`) |

Finden (shank 06.11.): T und N in der Statuszeile nicht nötig (Hinweis in der Ansicht, Tooltip, Rechtsklick), „Alle Aktionen suchen“
(Werkzeugleiste (T), Seitenleiste (N) wie Blender; Abstand messen, Knochen wählen in beiden Modi), Rechtsklick der Ansicht (dieselben),
Tooltip an jedem Werkzeug.

Sechs neue Symbole am Ende von `enum class UiIcon` (`BlAuswahl`, `BlSkalieren`, `BlMessen`, `BlAbwickeln`, `BlSeitenleiste`,
`BlWerkzeugleiste`), 50 Texte in `tools/i18n_ap4.py` (Präfix `Bz`, 6 Sprachen); vorhandene Texte für Modi und Werkzeuge
(`MbBlObjektModus`, `MbBlEditModus`, die Namen der Aktionstabellen). In fremden Dateien nur je eine Zeile bzw. der genannte Bereich:
`drawModellArbeitsflaeche` und zwei Zeilen in `eineAnsicht` (`app_modelltab_ansicht.cpp`), linke Spalte (`app_anordnung.inc`),
`ebenenSpalte` (`app_internal.h`), `modellBedienungSetzen`/`modellTasten`/Suche/Rechtsklick (`app_modelltab.cpp`, je eine Zeile), Blender-Zweig
des Kopfs (`app_modelltab_kopf.cpp`), Panel-Titel (`fenster_dock.cpp`, `app_dockspace.inc`).

Baustein von AP3 (`gui/ui_rollout.*`): **unverändert**. AP4 zeichnet keine Gruppen des Felds; die Wünsche von AP8/AP10
(`WUNSCH_AP3.md`) betreffen Gruppen, die AP4 nicht zeichnet - keiner umgesetzt (in `Y:\werkzeuge\ap3\SCHNITTSTELLE.md` vermerkt).

## Funde (rot → grün)

1. **Kleines Fenster: Werkzeuge unter dem Rand** (Foto `ap4_f_klein`, 1328 × 844 bei Oberfläche 1,0: im Bereich Modellieren standen nur
   sieben von zehn Knöpfen da, Zusammenführen usw. lagen ungesehen unter dem Rand). Jetzt werden die Knöpfe kleiner, bis höchstens
   1,3 Zeichenhöhen, und stehen mittig; reicht auch das nicht, rollt die Leiste mit dem Mausrad. Neue Prüfzeile im Bildlauf `ap4foto`
   („alle Knöpfe der Werkzeugleiste stehen ganz in ihrer Spalte“, je Bereich wie Blender): klein 7/0, en 7/0, hell 7/0.
2. **„-0.00“ in der Seitenleiste** (Foto en, Position X der Fläche torso). Werte, die auf zwei Stellen null sind, stehen als 0.00
   (Drehung: 0.0°). Zweite Aufnahme (ja, cs): 0.00.
3. **Kopf einer schmalen Ansicht** (vier Ansichten, kleines Fenster): Name und Modus hätten Overlays/Shading überdeckt (dort rückt das
   Feld nach rechts über den Rand). Jetzt fällt zuerst „Benutzer-Perspektive ·“, dann der Modus weg (`modellBlModusKopf`).
4. Lint `imgui_context` (1 Beanstandung nach Fund 1: SetCursorPosX vor dem Knopf) - jetzt `ImGui::Indent`, ok.

## Selbsttest (rot → grün)

- Rot gegen die alte Oberfläche (dieselben Texte, Kopfdateien und der Test, die Oberfläche unverändert; Lauf `ap4rot`): **15 OK,
  71 FEHLER** (Protokoll `Y:\werkzeuge\ap4\rot1_zeilen.txt`). Grün waren davon nur, was es schon gab: der Max-Modus unverändert (3), X15/F2
  (Bausteine von AP2/AP3), „kein Hinweis in Ansehen/Prüfen“, „N noch einmal blendet aus“, die Vorgabe Pinsel in Weights/Painting, „wie
  Blender wieder ausgeblendet“ und „Band in beiden Modi da“ (ohne Anordnung je Modus trivial), T wie 3ds Max, „nennt nicht wie 3ds Max“ (2).
- Erster Lauf mit der neuen Oberfläche (`ap4g1`): **82 OK, 0 FEHLER** (die vier Zeilen weniger sind die „Klick nicht gefunden“ des roten
  Laufs). Nach Fund 1-3, dem Sichtfeld im Kopf (P0b) und dem Handbuchschritt: `g5`, `n3`, `end` je **82 OK, 0 FEHLER**.
- Je Zeile: Ort (Rechtecke der Kindfenster `mtwerkzeugleiste`, `mtansichten`, `mtexplorer`, `mtbefehlsfeld`, `mtseitenleiste`, Probe
  `modellBl()`), Element (Folge der Knöpfe je Bereich, Trennstrich, Kopf „Benutzer-Perspektive · <Modus>“, Hinweis N/T) und Bedienung
  (Klick auf ein Werkzeug und seine Wirkung im Dokument, T, N, Pfeil im Streifen, Moduswahl, Trennkante, Reiter der Seitenleiste).
- Messen geprüft gegen eine unabhängige Rechnung: beide Punkte, zurück auf den Schirm gerechnet, liegen unter 1 Bildpunkt vom Klick (Lauf
  `end`: 0,67 / 0,35 px - ImGui legt die Mausposition auf ganze Bildpunkte, AddMousePosEvent), der Abstand ist ihre Strecke (1e-4); Knochen wählen: das Gelenk des gewählten Knochens liegt unter 1 Bildpunkt vom geklickten.

## Angepasste vorhandene Selbsttests (nichts gelöscht, keine Prüfung abgeschwächt)

Ursache aller sechs: der Aufbau wie Blender nach dem Entwurf - das Eigenschaften-Feld steht unter dem Outliner und ist niedriger (44 % /
56 %), die linke Spalte ist schmal, Ansichten und UV-Editor rücken nach links (Lauf `n1`: mb_uv 49/2, mb_ap2 60/4, mb_ap3 104/5, mb_ap6 93/3,
mb_ap8 99/14, mb_ap10 66/7; die Klicks fielen unter das Feld auf die Zeitleiste bzw. neben den Outliner).

- `gui/selbsttest_ap3.inc`: neuer Helfer `insFeld(label)` - eine Beschriftung des Felds vor dem Klick ganz ins Bild rollen (steht sie schon
  im Bild, bleibt die Rolle); F12 rollt vor den Klicks auf „1“ und „Auto“.
- `gui/selbsttest_ap6.inc` (M5 Geometrie bearbeiten), `gui/selbsttest_ap8.inc` (Auto-Normalisieren ×2, Sperren, Entsperren, Übernehmen),
  `gui/selbsttest_ap10.inc` (Control Rig erstellen ×2, Animation-Rig): `ap3::insFeld` vor dem Klick.
- `gui/selbsttest_ap2.inc`: im Abschnitt wie Blender den Outliner auf 80 % gezogen (die Zeile torso und die Fußzeile „Nach Ebenen“
  brauchen Platz), am Ende zurück; X2 prüft den Titel über dem Baum („Outliner“) und dass die linke Spalte wie Blender „Werkzeugleiste“
  heißt (vorher: Titel der linken Spalte = „Outliner“).
- `gui/selbsttest_uv.inc`: nach dem Wechsel zu wie Blender die Maus wieder auf die Leinwand (sie steht dort weiter links).

## Fotos

`Y:\werkzeuge\ap4\fotos\ap4_f_<variante>\` (Selbsttest `ap4foto`, 3818 × 2104 bei Oberfläche 1,5; je Lauf 7 OK, 0 FEHLER): en, de,
zh-Hans, ja, es, cs, hell, klein (1328 × 844 bei 1,0) - je Bedienmodus alle sechs Bereiche, dazu wie Blender Seitenleiste mit Messung,
Werkzeugleiste aus, Moduswahl offen, Tooltip. Entwurf über Programm: `Y:\werkzeuge\ap4\vergleich\v_<en|de>_<modus>_<bereich>.png` (2400 breit).
Angesehen: Blender en alle Bereiche, Max en Modellieren (unverändert), de Modellieren, ja und cs Seitenleiste, es Ansehen/Prüfen, zh Weights,
hell Ansehen/Prüfen, klein Modellieren (vor und nach Fund 1), die Vergleichsbilder Ansehen/Prüfen und Weights wie Blender.
- Wie Blender: Aufbau wie im Entwurf - Werkzeugleiste links mit eigenen Symbolen und Trennstrich, eine große Ansicht mit „View · Benutzer-
  Perspektive · <Modus> v“ und unten rechts „N Seitenleiste · T Werkzeuge“, Outliner oben rechts, darunter die Eigenschaften mit der
  senkrechten Reiterspalte. Der Max-Modus ist Bild für Bild wie vorher.
- Alle Sprachen: nichts abgeschnitten; CJK in den senkrechten Reitern der Seitenleiste gedreht wie in Blender.
- hell: gewähltes Werkzeug in der Betonungsfarbe des Themas, Symbole lesbar.
- Abweichungen vom Entwurf, die nicht AP4 gehören: Modell-Tabs über der Ansicht (R5, AP11), Hinweis „LOD 1 · 3412 Dreiecke …“ in
  Ansehen/Prüfen (R10, AP5), Knochen als Oktaeder in Weight Paint (Anzeige), vier Ansichten im Max-Modus (R7).

## Handbuch

- Eigene Quelle `tools/handbuch/inhalt_ap4.py` (6 Sprachen): Seite „Model-Tab: der Aufbau wie Blender“ (Werkzeugleiste je Bereich,
  Modus/Outliner/Eigenschaften, Seitenleiste und Messen, Anordnung je Bedienmodus); wie AP6-AP10 nicht eingehängt - AP14 führt zusammen
  (Anleitung im Kopf der Datei).
- Bilder `h_ap4_max`, `h_ap4_blender`, `h_ap4_seitenleiste` in 6 Sprachen unter `handbuch/bilder/<sprache>/`.

## Schlussrunde des Zweigs

| Schritt | Ergebnis |
|---|---|
| Bau alle Ziele (inkrementell, /W4 /WX) | 0 Fehler, 0 Warnungen (`bau_alles`, danach `bau_end` mit Fund 4) |
| ctest | 257 / 257 |
| `ap4` | 82 / 0 (rot 15 / 71 gegen die alte Oberfläche) |
| `ap4foto` | 8 Varianten, je 7 / 0 |
| Nachbargruppen Lauf `n1` (Bau `g2`) | ap1 137 · ap7 88 · ap9 97 · ap12 35 · kopf 116 · kopfrot 26 · nav 133 · raster 41 · mbv 216 · blender 68 · mb2 38 · mb3 22 · mb5 160 · mb6 38 · mb7 25 · funktionen 62 · tasten 44 · rig 27 · md 87 · pt 42 · ph 30 · rt 20 · ai 32 · pfix 18 · haut 2 · dock1 99 · dock2mass 30 · dock4 116 · dock5 27 · tabfenster 22 - je 0 FEHLER; rot: uv 49/2, ap2 60/4, ap3 104/5, ap6 93/3, ap8 99/14, ap10 66/7 (Testfolge des neuen Aufbaus, siehe oben) |
| nach der Anpassung, Lauf `n2` | uv 51 · ap2 64 · ap3 105 · ap6 94 · ap8 104 · ap10 70 - je 0 FEHLER |
| nach Kopf/Leiste, Lauf `n3` | ap4 82 · kopf 116 · kopfrot 26 · nav 133 · blender 68 · raster 41 - je 0 FEHLER |
| Handbuchbilder | 6 Sprachen, je 1 / 0 |
| Lints (21 aus `lints.sh`, dazu efxflags, fremdprogramm, d3dbesitz, merkmalsbits) | ok bzw. wie karte1: pruefcodes 11, portability 99, efxflags und fremdprogramm mit derselben Meldung |

Bewusst nicht gelaufen (einmal auf dem zusammengeführten Stand): Modus 1, die übrigen modellblock-Gruppen (Rundläufe, uvmd3, mdkarte,
pt-Varianten …), karteneditor, menueeditor, sauberer Neubau, Sprachläufe. Keine Bildzeit-Messung: die Werkzeugleiste zeichnet höchstens
zehn Knöpfe, die Seitenleiste nur, wenn sie an ist; ohne Messung keine Aussage zum Tempo.

## Stand je Zeile der Abgleichtabelle (für den Hauptchat; die Tabelle hat AP4 nicht angefasst)

| Zeile | Stand |
|---|---|
| R16 Aufbau wie Blender (Rahmen) | gleich: Werkzeugleiste links, eine Ansicht mit Kopf „Benutzer-Perspektive · <Modus>“, Outliner oben rechts, Eigenschaften mit senkrechten Reitern darunter, N-Seitenleiste, Umschalten ohne Verlust, Anordnung je Bedienmodus; Max unverändert. Offen und nicht AP4: Modell-Tabs (R5/AP11) |
| R17 Werkzeugleiste (T) | gleich (44 px → 2,6 Zeichenhöhen, Auswählen/Bewegen/Drehen/Skalieren, Trennstrich, Werkzeuge des Bereichs, T, eigene Symbole); mehr: Rechtsklick, Streifen mit Pfeil, passt sich kleinen Fenstern an |
| R18 Seitenleiste (N) | gleich (Angaben der Auswahl, Lage/Drehung, Ansicht; N); Reiter Objekt/Ansicht senkrecht wie Blender |
| X2 Ort und Titel | gleich (oben rechts, „Outliner“, 44 %, Trennkante ziehbar) |
| X15 Outliner-Spalten (Ort) | gleich (Spalten von AP2, jetzt am Ort oben rechts) |
| X28 Aufbau wie Blender (Outliner) | gleich |
| F2 Eigenschaften mit Reitern | gleich (Baustein AP3) |
| F17 Aufbau wie Blender (rechtes Feld) | gleich (Outliner darüber, Eigenschaften mit Reitern, Panels von AP3/AP6-AP10) |
| P0b Aufbau wie Blender | gleich (Werkzeugleiste Messen · Fläche wählen · Knochen wählen · Foto, „Benutzer-Perspektive · Objektmodus · FOV“, Outliner, Reiter Objekt); offen und nicht AP4: Modell-Tabs (R5), LOD-Hinweis (R10/AP5) |
| P8 Werkzeugleiste des Bereichs | gleich; Messen neu (zwei Klicks oder Ziehen, auch wie 3ds Max über Suche/Rechtsklick „Abstand messen“) |
| M0b Aufbau wie Blender | gleich (Extrudieren, Einfügen, Fase, Schleifenschnitt, Messer, Zusammenführen; „Bearbeitungsmodus“); bewusst anders: deutsch „Bearbeitungsmodus“ statt „Edit-Modus“ und „Zusammenführen“ statt „Verschmelzen“ - die vorhandenen Texte des Programms |
| U0b Aufbau wie Blender | gleich (Naht, Abwickeln, Entspannen, Packen; Bearbeitungsmodus; UV-Fenster links von AP7) |
| W0b Aufbau wie Blender | gleich (Pinsel, Weichzeichnen, Verlauf, Gewicht holen; Kopf „Weight Paint“); offen und nicht AP4: Oktaeder-Knochen in Weight Paint (Anzeige) |
| T0b Aufbau wie Blender | gleich (Malen, Füllen, Klonen, Verwischen, Maske; „Texture Paint“) |
| A0b Aufbau wie Blender | gleich (Werkzeugleiste, „Pose-Modus“, Reiter Knochen, Timeline von AP1) |
| A11 Werkzeugleiste | gleich (Pose kopieren · Gespiegelt einfügen · Ausdünnen über `modellAnimWerkzeug*`; Gespiegelt ist gesperrt ohne kopierte Pose) |

## Offen / Fragen an shank

1. Seitenleiste: die Werte sind zum Ablesen (wie Blenders Item-Panel zeigt, aber hier ohne Eingabe). Sollen Position/Drehung eines Griffs dort
   auch eingegeben werden können (Blender: ja)? Heute geht das über G/R mit Zahl bzw. das Gizmo.
2. Deutsche Modusnamen: im Entwurf „Edit-Modus“, im Programm schon „Bearbeitungsmodus“ (Kontextmenü, Gruppentitel) - so lassen?
3. Die Werkzeugleiste hat feste Werkzeuge je Bereich (Entwurf). Blender lässt Werkzeuge mit langem Klick in Gruppen wählen und blendet mit
   Rechtsklick „Werkzeugnamen zeigen“ ein - gewünscht?
- Auto Key steht ab Werk noch an (Foto Max) - das stellt ENTSCHEID1 um, nicht AP4.
- Handbuchseite `modell-blender` ist nicht eingehängt (AP14).

## Changelog

## English

- Model tab like Blender: the column on the left is the toolbar (T) with Select, Move, Rotate, Scale and the tools of the area - Measure, Select surface, Select bone, Photo; Extrude, Inset, Bevel, Loop Cut, Knife, Merge; Mark seam, Unwrap, Relax, Pack; Brush, Blur, Gradient, Sample weight; Draw, Fill, Clone, Smear, Mask; Copy pose, Paste mirrored, Reduce.
- Like Blender the outliner is at the top right above the properties editor; the edge between them can be dragged.
- The header of the view names the mode like Blender (Object Mode, Edit Mode, Weight Paint, Texture Paint, Pose Mode); a click on it changes the area.
- N opens the sidebar: location and rotation or dimensions of the selection, the last measurement, field of view and projection.
- New tool Measure: two clicks on the model or a drag show the distance in game units; like 3ds Max as Measure distance in Search all actions and the context menu.
- Each control mode keeps its own arrangement of the windows; switching keeps selection, sequence and frame. The layout like 3ds Max is unchanged.

## Deutsch

- Model-Tab wie Blender: die linke Spalte ist die Werkzeugleiste (T) mit Auswählen, Bewegen, Drehen, Skalieren und den Werkzeugen des Bereichs - Messen, Fläche wählen, Knochen wählen, Foto; Extrudieren, Einfügen, Fase, Schleifenschnitt, Messer, Zusammenführen; Naht, Abwickeln, Entspannen, Packen; Pinsel, Weichzeichnen, Verlauf, Gewicht holen; Malen, Füllen, Klonen, Verwischen, Maske; Pose kopieren, Gespiegelt einfügen, Ausdünnen.
- Wie Blender steht der Outliner oben rechts über dem Eigenschaften-Editor; die Kante dazwischen lässt sich ziehen.
- Der Kopf der Ansicht nennt wie Blender den Modus (Objektmodus, Bearbeitungsmodus, Weight Paint, Texture Paint, Pose-Modus); ein Klick darauf wechselt den Bereich.
- N öffnet die Seitenleiste: Position und Drehung bzw. Abmessungen der Auswahl, die letzte Messung, Sichtfeld und Projektion.
- Neues Werkzeug Messen: zwei Klicks auf das Modell oder Ziehen zeigen den Abstand in Spieleinheiten; wie 3ds Max als Abstand messen in Alle Aktionen suchen und im Rechtsklick.
- Jeder Bedienmodus behält seine eigene Fensteranordnung; beim Wechsel bleiben Auswahl, Sequenz und Bild. Der Aufbau wie 3ds Max bleibt unverändert.

## 中文

- 模型选项卡像 Blender 时: 左侧一栏是工具栏 (T), 含选择、移动、旋转、缩放以及该区域的工具 - 测量、选择面、选择骨骼、照片; 挤出、插入、倒角、循环切割、切刀、合并; 标记接缝、展开、松弛、打包; 笔刷、模糊、渐变、采样权重; 绘制、填充、克隆、涂抹、遮罩; 复制姿势、镜像粘贴、精简。
- 像 Blender 时大纲视图位于右上方的属性编辑器之上; 两者之间的边界可以拖动。
- 视图标题像 Blender 一样显示模式 (物体模式、编辑模式、权重绘制、纹理绘制、姿态模式); 单击即可切换区域。
- 按 N 打开侧栏: 所选内容的位置和旋转或尺寸、最近一次测量、视野和投影。
- 新工具“测量”: 在模型上单击两次或拖动即可显示以游戏单位表示的距离; 像 3ds Max 时以“测量距离”出现在“搜索所有操作”和右键菜单中。
- 每种操作模式都保留自己的窗口布局; 切换时所选内容、序列和帧保持不变。像 3ds Max 的布局保持不变。

## 日本語

- Blender 風のモデルタブ: 左の列はツールバー (T) で、選択、移動、回転、拡大縮小とエリアのツールが並びます - メジャー、サーフェスを選択、ボーンを選択、写真、押し出し、インセット、ベベル、ループカット、ナイフ、マージ、シームを設定、展開、リラックス、パック、ブラシ、ぼかし、グラデーション、ウェイトを取得、ドロー、塗りつぶし、クローン、スミア、マスク、ポーズをコピー、ミラー貼り付け、間引き。
- Blender 風ではアウトライナーが右上、プロパティエディターの上にあります。間の境界はドラッグできます。
- ビューのヘッダーは Blender と同じくモードを示します (オブジェクトモード、編集モード、ウェイトペイント、テクスチャペイント、ポーズモード)。クリックするとエリアが切り替わります。
- N でサイドバーが開きます: 選択の位置と回転または寸法、最後の測定、視野角と投影。
- 新しいツール「メジャー」: モデル上で 2 回クリックするかドラッグすると、ゲーム単位の距離を表示します。3ds Max 風では「距離を測定」として「すべてのアクションを検索」と右クリックメニューにあります。
- 各操作モードはウィンドウの配置をそれぞれ保持します。切り替えても選択、シーケンス、フレームは保持されます。3ds Max 風のレイアウトは変わりません。

## Español

- Pestaña Model como Blender: la columna de la izquierda es la barra de herramientas (T) con Seleccionar, Mover, Rotar, Escalar y las herramientas del área - Medir, Seleccionar superficie, Seleccionar hueso, Foto; Extruir, Insertar, Bisel, Corte en bucle, Cuchillo, Fusionar; Marcar costura, Desplegar, Relajar, Empaquetar; Pincel, Desenfocar, Degradado, Muestrear peso; Dibujar, Rellenar, Clonar, Difuminar, Máscara; Copiar pose, Pegar reflejada, Reducir.
- Como Blender, el Outliner está arriba a la derecha sobre el editor de propiedades; el borde entre ambos se puede arrastrar.
- La cabecera de la vista nombra el modo como Blender (Modo Objeto, Modo Edición, Pintura de pesos, Pintura de texturas, Modo Pose); un clic cambia el área.
- N abre la barra lateral: ubicación y rotación o dimensiones de la selección, la última medición, campo de visión y proyección.
- Nueva herramienta Medir: dos clics en el modelo o un arrastre muestran la distancia en unidades del juego; como 3ds Max, como Medir distancia en Buscar todas las acciones y en el menú contextual.
- Cada modo de control conserva su propia disposición de ventanas; al cambiar se mantienen la selección, la secuencia y el fotograma. La disposición como 3ds Max no cambia.

## Čeština

- Karta Model jako Blender: levý sloupec je panel nástrojů (T) s Vybrat, Posunout, Otočit, Změnit měřítko a nástroji oblasti - Měřit, Vybrat plochu, Vybrat kost, Fotka; Vysunout, Vsadit, Zkosení, Řez smyčkou, Nůž, Sloučit; Označit šev, Rozbalit, Uvolnit, Zabalit; Štětec, Rozostřit, Přechod, Odebrat váhu; Kreslit, Vyplnit, Klonovat, Rozmazat, Maska; Kopírovat pózu, Vložit zrcadlově, Zredukovat.
- Jako Blender je Outliner vpravo nahoře nad editorem vlastností; hranu mezi nimi lze táhnout.
- Záhlaví pohledu uvádí režim jako Blender (Objektový režim, Editační režim, Malování vah, Malování textur, Režim pózy); kliknutí přepne oblast.
- N otevře postranní panel: umístění a rotace nebo rozměry výběru, poslední měření, zorné pole a projekce.
- Nový nástroj Měřit: dvě kliknutí na model nebo tažení ukážou vzdálenost v herních jednotkách; jako 3ds Max jako Změřit vzdálenost ve Vyhledat všechny akce a v místní nabídce.
- Každý režim ovládání si drží vlastní rozvržení oken; při přepnutí zůstane výběr, sekvence i snímek. Rozvržení jako 3ds Max se nemění.
