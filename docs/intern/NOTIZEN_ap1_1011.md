# Notizen AP1: Zeitleiste des Model-Tabs nach dem freigegebenen Entwurf, wie 3ds Max bzw. wie Blender (11.11.2026, `Y:\p_mbe`, Zweig `p_ap1`, auf c624e0cb1)

Auftrag MBENTWURF Teil B, Arbeitspaket AP1 (`auftrag_mbentwurf_b_1011.md`, Anhang `auftrag_mbzeit_1011.md`). Umfang: die Zeilen Z1–Z23, R8, A12, A13 der Abgleichtabelle `plaene/modellblock_stufe0/abgleich_programm_entwurf.md`.

shank: „dann ist die Timeline nicht gut. Hätte sie gerne wie in 3ds max oder blender. Je nachdem welchen modi“ – „ich dachte die timeline sieht anders aus? laut dem design“ – „es geht ja nicht nur um controls … sondern das aussehen“.

**Warum die Leiste im Bereich Animation nicht wie im Entwurf aussah:** der Entwurf war für die Zeitleiste nie gebaut worden. `drawModelPanel` zeichnete in allen sechs Bereichen und beiden Bedienmodi dieselben vier Zeilen (Symbolknöpfe, glatter Schieber, Textzeile Primär/Sekundär, Marker-Zeile); die Spuren gab es nur als eigenen Kasten „Dope Sheet“ zwischen Ansichten und Leiste, ab Werk zu, mit eigenem Lineal. Der „Balken mit dem blauen Block“ aus shanks Bild war der Schieberegler des Bilds (`ImGui::SliderInt`, der Block ist sein Griff) – er ist durch die Skala ersetzt.

## Aufbau

- **Neue Datei `gui/app_modelltab_zeit.cpp`:** die ganze Leiste (`drawModellZeitleiste`, Aufruf am Ende von `drawModelPanel` nach der Uhr), ihre Höhe (`modellZeitleisteHoehe`, statt fest vier Zeilen in `gui/app_anordnung.inc`), Zeichner für Skala, Spurgrund, Schlüssel und Abspielkopf (auch vom Dopesheet/Kurveneditor benutzt: ein Lineal), Auto Key mit der „Pose ohne Schlüssel“, Zeitkonfiguration, Menü der Zeitleiste, Keying-Menü, Statuszeile, Tasten, Aktionen für die Suche, `modellHautZeile` (Z20).
- **Von oben nach unten:** Griff (60 px im Entwurf = 4,6 Schrifthöhen) · Knopfreihe · Skala mit Namensspalte (12 Schrifthöhen) · Spuren · Hinweis/Legende · Statuszeile. Alle Maße in Schrifthöhen; die Höhe des Bands wird aus denselben Maßen gerechnet, mit denen gezeichnet wird (auch der Umbruch der Knopfreihe bei schmalem Fenster).
- **Drei Fassungen (Z2):** voll (Animation: Skala mit den Schlüsseln der Auswahl, Spuren), Sequenz (Ansehen/Prüfen: − 1.00× +, Interpolation, Schleife erzwingen, Zeilen Primär/Sekundär), kurz (Modellieren, UV, Weights, Painting).
- **Spuren = Dopesheet (Z15):** `drawDopesheet`/`drawKurven` in `gui/app_modelltab_kurven.cpp` zeichnen jetzt im Band (`drawModellSpurbereich`): oben die Skala (ihre Zeile „Auswahl (Spurleiste)“ / „Zusammenfassung“ ist die bisherige Zusammenfassungszeile), darunter eine Spur je Glied und Knochen. Der Kasten mit Menüleiste unter den Ansichten entfällt; seine Menüs stehen im Menü der Zeitleiste, die Umschaltung Dopesheet/Kurveneditor neben dem Knopf „Spuren“. In der Skala wählt und zieht die Maus oben (Schlüssel) Schlüssel, unten (Zahlen) setzt sie das Bild – wie Spurleiste und Zeitschieber in 3ds Max bzw. Schlüsselbereich und Lineal in Blender.
- **Wie 3ds Max:** Anfang · Bild zurück · Abspielen · Bild vor · Ende · „12 / 119“ ab 0 · Auto Key (N) · Set Key (K) · Zeitkonfiguration; ein feiner Strich je Frame in Skala und Spuren; Schlüssel als Striche, Rot = Bewegung, Grün = Drehung, beides = oben rot, unten grün.
- **Wie Blender:** Anfang · Schlüssel zurück (↓) · Abspielen · Schlüssel vor (↑) · Ende · Aufnahme (Auto-Keying) · Start / Ende / Bild ab 1 · Bildrate; Rauten; K öffnet im Bereich Animation das Keying-Menü; Pos1 über den Spuren zeigt alles; das Menü der Zeitleiste ist gegliedert wie die Kopfzeile der Timeline (Wiedergabe, Keying, Ansicht, Marker).
- **Auto Key (Z6):** an (Vorgabe) = wie bis rc611. Aus: der Zug (Gizmo, G/R, Regler IK-FK und Fußrolle) schreibt seinen Rückgängig-Schritt wie bisher, die Leiste merkt ihn sich als „Pose ohne Schlüssel“. Set Key (K, Blender I) behält sie; ein Bild- oder Sequenzwechsel nimmt die Schritte zurück (`Dokument::abbrechen`), Rückgängig oder ein anderer Befehl beendet den Zustand still.
- **Zeitkonfiguration (Z8):** Bildrate, Zeitanzeige Frames/Sekunden (Skala und Abspielkopf), Tempo, Schleife, Interpolation, Key-Modus, Auto Key und die Zeile der animation.cfg (Anfang, Anzahl, Schleife, fps, Bereichsbalken – bisher vierte Zeile der Leiste, `modellMarkerZeile` unverändert).
- **Statuszeile (R8):** Bedienmodus (Klick: Menü mit beiden Modi) · Tasten des Bereichs · Undo/Suche. Zwölf Texte (zwei Modi × sechs Bereiche); genannt sind nur Tasten, die es im Programm gibt (geprüft gegen `maxTasten`/`blenderTasten`, die Aktionen von UV, Modellieren, Painting, Kurven und `modellGewichteTasten`).
- **Einstellungen:** `modelTimelineTracks` (Spuren offen, ab Werk an), `modelTimelineTracksMax` / `modelTimelineTracksBlender` (Höhe des Spurbereichs je Bedienmodus in Schrifthöhen, Vorgabe 8), `modelTimelineDisplay` (0 Frames, 1 Sekunden).
- **Helfer:** `tools/gen_hilfe_orte.py` kennt die Knöpfe der Zeitleiste (eigener Zeichner `knopf(tr(Str::X), …, tr(Str::Hinweis))`), `src/hilfe_orte.inc` neu erzeugt.

## Entscheidungen

- **Farbe eines Schlüssels:** ein `RigSchluessel` trägt Drehung (Quaternion) und Verschiebung im Rahmen des Knochens. Rot, wenn die Verschiebung nicht null ist (auch jeder Schlüssel einer IK-Spur: ein Ziel im Raum), Grün, wenn die Drehung nicht die Einheit ist; ein Schlüssel ohne beides (Ruhepose) zählt als Drehung. Schwelle 1e-4 je Komponente (0,0001 Einheiten bzw. rund 0,01 Grad) – darunter liegt nur die Rundung der Umrechnung Matrix → Quaternion in float.
- **Zählung:** wie bisher – wie Max ab 0, wie Blender ab 1 (Entscheidung shank 04.11.), GLA-Frame im Tooltip.
- **Spuren ab Werk offen** (Entwurf; der Grund, warum shank im Bereich Animation keine Spurleiste sah, war der zugeklappte Kasten). Der Zustand wird gespeichert.
- **Ausschnitt der Skala:** wie das eingepasste Dopesheet ein Bild Rand links und rechts (im Entwurf liegt Bild 0 genau am Rand und der Schlüssel dort ist halb abgeschnitten); Bild 0 steht in Skala und Spuren an derselben Stelle (ein Lineal).
- **Start/Ende wie Blender** ändern Anfang und Anzahl der Sequenz in der animation.cfg (so sieht es die Zeile „+9“ der Tabelle vor), nicht einen Vorschaubereich.
- **Haut und Flächenzähler (Z20)** stehen in der Gruppe „Detailstufe und Skin“ (`rolloutLod` ruft `modellHautZeile`): die Auswahlliste mit Beschriftung „Skin“ und darunter der Zähler (Klick: Flächenliste ins Detailprotokoll) – Tour k-skins und ihr Selbsttest finden beide dort. Die bisherige zweite Liste der Häute in der Gruppe entfällt (sie war dieselbe Auswahl).

## Abweichungen vom Entwurf (bewusst)

- **Kein Blau für Skalierung** (Z13, Z17): das Programm hat keine Skalierungs-Schlüssel (JKA speichert keine Skalierung, `RigSchluessel` hat keine). Die Legende sagt das („Rot = Bewegung · Grün = Drehung · JKA speichert keine Skalierung“) statt eine Farbe zu erklären, die nie vorkommt; das Warnzeichen entfällt damit.
- **Spurnamen:** „<Figur> · <Glied>“ wie im Entwurf, die Figur ist der Ordner des Modells (kyle). Unter einem Glied stehen wie bisher seine Knochen (aufklappbar) – der Entwurf zeigt nur die Glieder.
- **Mehr in der Knopfreihe als im Entwurf:** die Umschaltung Dopesheet/Kurveneditor (nur bei offenen Spuren), der Knopf mit den drei Strichen (Menü der Zeitleiste – sonst nur per Rechtsklick), der Name der Sequenz (Z21), die Sekunden hinter der Bildanzeige (Z5). Reicht der Platz nicht, entfallen Name und Hinweis, dann bricht die Reihe um.
- **Statuszeile:** die Texte nennen die Tasten, die es gibt. Im Entwurf stehen für Ansehen/Prüfen „Alt+X Durchsicht“ (Max) bzw. „Z Schattierung · Alt+Z Röntgen · Umschalt+Alt+Z Overlays“ (Blender), für Weights „Weight Tool · Strg+Klick“ bzw. „K Sperren-Menü“, für Painting „[ ] Größe“ – diese Tasten gibt es auf diesem Stand nicht (R14, W-Zeilen, T10 anderer Pakete); wer sie baut, ergänzt den Text (`MzSt…` in `tools/i18n_ap1.py`).
- **Blender-Kopfzeile:** der Entwurf zeigt keine Menüs in der Timeline; der Anhang MBZEIT nennt Playback/Keying/View/Marker. Sie stehen als Gruppen im Menü der Zeitleiste (Entwurf geht vor, nichts fehlt).
- **Zeitschieber wie Max** (Anhang: Griff mit „18 / 100“ und Pfeilen): der Entwurf hat stattdessen das Kästchen des Abspielkopfs in der Skala – so gebaut.

## Tasten

| Modus | Taste | Wirkung | neu |
|---|---|---|---|
| wie 3ds Max | N | Auto Key an/aus | ja (A12) |
| wie 3ds Max | K | Set Key (behält auch die Pose ohne Schlüssel) | Taste gab es, Knopf neu |
| wie 3ds Max | , . mit Key-Modus | voriger / nächster Schlüssel | ja |
| wie Blender | ↑ / ↓ | nächster / voriger Schlüssel | ja (A13) |
| wie Blender | K (Bereich Animation) | Keying-Menü | ja (A13) |
| wie Blender | Pos1 über den Spuren | alles zeigen | ja |
| beide | wie bisher | Pos1/Ende, , ., / bzw. Leertaste, ←/→, Umschalt+←/→, I | – |

Kollisionen: keine. N war im Max-Modus frei; ↑/↓ und K (Bereich Animation) im Blender-Modus frei (K ist im Bereich Modellieren das Messer – dort gilt es weiter, das Keying-Menü gibt es nur im Bereich Animation). In den Zweigen p_mbkopf, p_mbnav, p_mbvers, p_mbmal, p_uiform, p_ap2, p_ap12 kommt keine dieser Tasten neu dazu (geprüft am 11.11. per `git diff`).

## Quellen

- Entwurf: `plaene/modellblock_stufe0/entwurf_modelltab.html`, Funktionen `zeitleiste(max)` und `statusZeile(max)`, CSS `.zeit`, `.skala`, `.spuren`, `.griff`.
- 3ds Max: Autodesk-Hilfe „Track Bar“ (Schlüssel für Position, Drehung, Skalierung rot, grün, blau) https://help.autodesk.com/cloudhelp/2017/ENU/3DSMax/files/GUID-A55E4702-263E-4768-9964-5866698784DA.htm und „Using Set Key Mode“ (ohne Set Keys geht die Pose beim Wechsel des Bilds verloren) https://help.autodesk.com/cloudhelp/2018/ENU/3DSMax-Animation/files/GUID-98467413-F295-4862-971A-FB179A7D2E3C.htm
- Blender: Handbuch „Editors > Timeline“ (Kopfzeile View, Marker, Playback, Keyframing, Transport, Bildfelder; Schlüssel als Rauten, Abspielkopf blau, Pos1 = Frame All) https://docs.blender.org/manual/en/latest/editors/timeline.html
- Nichts aus beiden Programmen kopiert: alle Symbole sind in `app_modelltab_zeit.cpp` aus Strichen und Flächen gezeichnet; 3ds Max und Blender wurden nicht gestartet.

## Ergebnisse (rot → grün)

Selbsttest `gui/selbsttest_ap1.inc` (`BHED_EDITORTEST=modellblock`, `BHED_MODELL_NUR=ap1`): Sturmtruppler, BOTH_WALK1 (30 Bilder), Rig, drei Drehschlüssel (rhand, cranium bei +0/+10/+20), ein Bewegungsschlüssel (pelvis +5), einer mit beidem (pelvis +15); jede Prüfung in beiden Bedienmodi, Fenster 2560 × 1440.

- **Rot** (Programm der Basis c624e0cb1 mit dem neuen Test; die acht geänderten Programmdateien aus der Basis, `app_modelltab_zeit.cpp` als leerer Stumpf – `Y:\werkzeuge\ap1\rot_an.py`): **11 OK, 160 FEHLER** (156 Prüfzeilen, 4 Klicks auf Elemente, die es nicht gab). Protokoll `Y:\werkzeuge\ap1\rot1_protokoll.txt`. Jede Zeile Z1–Z21, R8, A12, A13 war in mindestens einer Prüfung rot. Schon im Rotlauf grün (das konnte die alte Leiste auch, bzw. die Prüfung ist die zweite Hälfte eines Paars): Auto Key an = Zug setzt einen bleibenden Schlüssel (2×), K setzt einen Schlüssel, „N schaltet wieder an“ (Stumpf stand auf an), Strg+Z nach dem Ziehen in der Skala (2×, es war nichts gezogen), „Interpolation“ schaltet (2×), „Sekundär aus“ (2×), Blender „keine Striche je Frame“ (0 = 0). Z1 und Z5 stehen in der Tabelle als „gleich“ (Leiste in jedem Bereich, Zählung): ihr Rot kommt daher, dass die Prüfung die Lage der neuen Leiste misst.
- **Grün, erster Lauf** auf dem neuen Programm: 134 OK, 2 FEHLER – beide die Prüfung „Suche kennt die Aktionen der Zeitleiste“: ein Fehler des Tests (Zeilen außerhalb des sichtbaren Teils der Liste tragen im Elementverzeichnis keine Beschriftung); der Test tippt jetzt „Timeline:“ in die Suche.
- **Grün, endgültiger Bau:** **136 OK, 0 FEHLER** (`ap1_g_ap1`).

| Zeile | Was der Test misst (je Modus) |
|---|---|
| Z1 | Leiste in allen sechs Bereichen gezeichnet, ≥ 85 % der Fensterbreite, Oberkante unter den Ansichten |
| Z2 | Fassung je Bereich (Sequenz / kurz ×4 / voll), Knopf Spuren nur in Sequenz und voll, Tempo nur in Sequenz; Höhen kurz < Sequenz < voll |
| Z3 | Griff mittig an der Oberkante; 4 Schrifthöhen hochgezogen = Spurbereich um den Weg höher (±2 px); ganz herunter = Spuren zu, 5 Schlüssel bleiben in der Skala |
| Z4 / Z18 | Reihenfolge der Knopfreihe (Max: … Ende, Bild, Auto Key, Set Key, Zeitkonfiguration; Blender: … Aufnahme, Start, Ende, Bild, Bildrate), eine Zeile; „Ans Ende“ springt; Tooltip erscheint und nennt die Taste; Blender: Schlüssel vor/zurück springen +5/+10/+15; Max: Key-Modus |
| Z5, Z21 | Zählung ab 0 bzw. ab 1; Name der Sequenz in der Reihe |
| Z6, Z7, A12 | Auto Key an: Schlüssel bleibt. N (Blender: Aufnahme-Knopf) schaltet aus: Zug → Pose ohne Schlüssel + Hinweis; Bildwechsel verwirft (Schlüssel weg); Set Key (Blender I) behält; K setzt; N schaltet an |
| Z8 | Knopf öffnet die Zeitkonfiguration mit ##mkanfang/-anzahl/-schleife/-fps/-balken; Sekunden: Kästchen zeigt 0.60 |
| Z9, Z16, Z10 | Ansehen/Prüfen: − / + stellen 1.5 und 0.5, Schleife und Interpolation schalten, zwei Zeilen Primär/Sekundär, „Sekundär aus“, Spuren klappt sie zu; kurze Fassung: nichts davon in der Reihe, dafür im Menü (Rechtsklick auf die Skala und Menü-Knopf) |
| Z11 | Skalenhöhe, ≥ 4 Teilstriche, Kästchen mit der Bildnummer über dem Bild; Klick → Bild 20, Ziehen → Bild 3 |
| Z12 | Max: 30 Striche in der Skala, 30 je Spur; Blender: 0 |
| Z13 | Max: 21 Striche (15 grün, 3 rot, 3 zweifarbig), 0 Rauten; Blender: 21 Rauten, 0 Striche; Klick auf einen Schlüssel in der Skala wählt, Ziehen verschiebt um 3 Frames, Strg+Z |
| Z14 | Text der Namensspalte, Spalte endet am Anfang der Skala, Bild 0 in Skala und Spuren am selben x (±0,01 px), 2 Linien des Abspielkopfs |
| Z15 | Spuren ab Werk offen im Band, ≥ 4 Spuren, kein Fenster „animeditor“ |
| Z17 | Hinweiszeile enthält Legende und Zählung des Modus |
| Z19, A13 | Max: , und . (mit und ohne Key-Modus); Blender: ↑ ↑ ↓, K öffnet das Keying-Menü, „Schlüssel setzen“ darin setzt |
| Z20 | „Skin“ nicht mehr im Band, dafür im Befehlsfeld |
| R8 | Statustext je Bereich = Modus | Tasten des Bereichs | Undo/Suche, unterste Zeile des Bands, im Fenster; Klick auf den Modus → Menü → anderer Modus, Leiste baut um |

## Angepasste Tests (nichts gelöscht)

- `selbsttest_werkzeuge.inc` (Modus `modell`): „Schleife erzwingen“ heißt in der Leiste „Force loop“ (`MzSchleife`).
- `selbsttest_mb2.inc` (Gruppe `mb3`, Marker): der Bereichsbalken steht im Fenster Zeitkonfiguration – der Test öffnet es vor dem Ziehen.
- `selbsttest_mb5.inc`: die Spuren stehen ab Werk offen – vor „Dopesheet über das Menü öffnen“ klappt der Test sie zu.
- `selbsttest_handbuch.inc`: Bild `m_abspielen` markiert „Force loop“, Bild `m_flaechen` den Zähler unter der Hautliste im Befehlsfeld; neue Gruppe `BHED_HB_NUR=ap1`.

## Fotos (angesehen)

Foto-Selbsttest `BHED_MODELL_NUR=ap1foto` (Kyle mit Säbel, BOTH_WALK1 Bild 12, Rig mit Schlüsseln): je Bedienmodus alle sechs Bereiche, dazu Animation mit Spuren zu, Kurveneditor, Zeitkonfiguration, Menü der Zeitleiste und Pose ohne Schlüssel. Jeder Lauf prüft je Bereich und Modus, dass kein Element der Leiste über ihren Rand reicht und die Statuszeile im Fenster liegt (12 Prüfungen).

| Lauf (`Y:\werkzeuge\ap1\fotos.sh`) | Fenster | Ergebnis |
|---|---|---|
| gross (en, dunkel) | 3840 × 2160 bei Oberfläche 1,5 | 12 OK / 0 |
| hell (en, helles Thema) | 1920 × 1080 | 12 OK / 0 |
| de, zh-Hans, ja, es, cs (dunkel) | 1920 × 1080 | je 12 OK / 0 |
| min, min_de, min_ja | 1350 × 900 (Mindestgröße 900 × 600 bei 150 %) | je 12 OK / 0; die Knopfreihe bricht in Ansehen/Prüfen wie Blender (en) bzw. in beiden Modi (de) und in Animation (ja) auf zwei Zeilen um |

Neben das Entwurfsbild gelegt (`Y:\werkzeuge\ap1\vergleich.py`, Entwurf aus `Y:\werkzeuge\mbe\fotos\entwurf2x` über dem Programm, 3840 breit, `Y:\werkzeuge\ap1\fotos\vergleich_ap1_foto_gross\v_<modus>_<bereich>.png`) und angesehen: Animation und Ansehen/Prüfen in beiden Modi, Weights wie 3ds Max, helles Thema (Animation wie 3ds Max), Mindestgröße (Ansehen/Prüfen en und de, Animation ja), Handbuchbild de wie Blender. Danach geändert: Rauten in der Skala auf die Größe des Entwurfs, Symbole der Knopfreihe größer, eingeschaltete Knöpfe in der Akzentfarbe (Entwurf `.knopf.an`), der Bereich außerhalb der Sequenz nur leicht gedämpft, die kurze Fassung nennt die Sequenz nicht doppelt, zusammengehörige Knöpfe (− 1.00× +, Dopesheet | Kurveneditor) brechen gemeinsam um.

Handbuch: `tools/handbuch/inhalt_ap1.py` (Seite modell-zeitleiste, sechs Sprachen), Bilder `h_ap1_max`, `h_ap1_blender`, `h_ap1_pruefen` je Sprache (2538 × 1472, `BHED_EDITORTEST=handbuch`, `BHED_HB_NUR=ap1`, `Y:\werkzeuge\ap1\hb.sh`). Die erzeugten Seiten unter `handbuch/<sprache>/` sind **nicht** neu erzeugt mitgegeben (die neue Seite ändert die Seitenleiste jeder der 178 Seiten je Sprache) – AP14 bzw. das Zusammenführen ruft `python tools/handbuch/gen_handbuch.py`; `--pruefen` ist grün.

## Gegen das Vorbild geprüft / nicht geprüft

- Geprüft gegen den Entwurf: Aufbau, Reihenfolge und Beschriftung der Knopfreihe, Skala, Namensspalte, Spuren, Legende, Statuszeile je Modus und Bereich – per Foto neben dem Entwurfsbild und per Selbsttest (Tabelle oben).
- Geprüft gegen die Dokumentation (nicht gegen die laufenden Programme, sie wurden nicht gestartet): Farben der Spurleiste, Set-Key-Verhalten (3ds Max), Rauten, blauer Abspielkopf, Pos1 = alles zeigen, Gruppen der Kopfzeile (Blender).
- Nicht geprüft: das Verhalten in 3ds Max und Blender selbst (z. B. wie viele Bildpunkte ein Schlüssel der Spurleiste breit ist) – Maße und Abstände folgen dem Entwurf.

## Zusammenführen

Probe-Merge (`git merge-tree`) gegen p_mbkopf, p_mbnav, p_mbvers, p_uiform, p_ap2, p_ap12: Konflikte nur in den gemeinsam berührten Listen, je ein angehängter Block (beide behalten): `gui/modelltab.h` (Ende), `gui/selbsttest.cpp` (Einbindung, Verteilzeile), `tools/gen_i18n.py`, `tools/i18n_sprache_es.py`, `tools/i18n_sprache_cs.py`, `tools/handbuch/gen_handbuch.py`, gegen p_ap2 auch `tools/handbuch/inhalt.py`; dazu die erzeugten `include/bhed/i18n.h`, `src/i18n.cpp`. p_mbmal: kein Konflikt. Danach neu erzeugen: `python tools/gen_i18n.py`, `python tools/gen_hilfe_orte.py`, `python tools/handbuch/gen_handbuch.py`.

## Schlussrunde im Zweig (Regel „Schlussrunde je Zweig ab 11.11.“)

| Schritt | Ergebnis |
|---|---|
| Bau, alle Ziele (inkrementell, /W4 /WX) | 0 Fehler, 0 Warnungen |
| ctest (246) | 245 bestanden; rot `q3map2_teilbau` (2 Fehlschläge im Licht-Cache-Test von q3map2 – fremder Code, kein Bezug zu AP1; im Zweig p_mbkopf auf derselben Basis ebenso rot, `Y:\werkzeuge\mbr\ctest.log`) |
| `ap1` (eigener Selbsttest, endgültige exe) | 136 OK / 0 FEHLER |
| modellblock-Gruppen, die den geänderten Code berühren | mb5 159/0 · mb3 21/0 · rig 26/0 · mb6 37/0 · funktionen 61/0 · tasten 43/0 · blender 67/0 · haut 1/0 · mb2 37/0 · pfix 17/0 · pfix2 5/0 · mb7 24/0 · ph 29/0 · uv 50/0 · md 86/0 · pt 41/0 · bild 1/0 |
| Modus `modell` (Leiste im Werkzeug-Test) | 14 OK / 0 |
| `karteneditor`, `BHED_KARTE_NUR=hilfe2`, `BHED_H2_NUR=skins` (Tour k-skins: Hautliste und Flächenzähler am neuen Ort) | 16 OK / 0 |
| `ap1foto` (10 Läufe: Sprachen, helles Thema, Mindestgröße) | je 12 OK / 0 |
| Handbuch | `gen_handbuch.py --pruefen`: ok (6 Sprachen, 0 Texte und 0 Bilder noch englisch) |
| Lints | alle 32 `tools/lint_*.py` gelaufen: keine Meldung in einer geänderten Datei; `lint_i18n`, `lint_settings`, `lint_sources` ok, `gen_hilfe_orte.py --check` ok |

Die Gruppen liefen auf dem Bau vor der letzten kleinen Änderung (zusammengehörige Knöpfe brechen gemeinsam um – nur `Reihe::zusammen` in `app_modelltab_zeit.cpp`); danach liefen `ap1` und drei Fotoläufe (gross, min_de, min_ja) auf der endgültigen exe.

Bewusst nicht gelaufen (läuft einmal auf dem zusammengeführten Stand): sauberer Neubau, Modus 1, menueeditor, Docks und die modellblock-Gruppen, deren Code AP1 nicht ändert – rundlauf, rigrundlauf, kurvenrundlauf, ikrundlauf, uvrundlauf, uvmd3, mdkarte, mb7textur, rt, rtjka, pr, ai, ptfix, ptfix2, ptfix3, ptstufen, pfixkarte. `bild2` (Pose Model-Tab gegen Mission-Tab über 552 Modelle und 1339 NPC-Typen) habe ich nach 25 Minuten im zweiten Durchgang abgebrochen (eigene PID) – nicht gewertet.

## Bildzeit (Richtwert unter Last)

`Y:\werkzeuge\ap1\messung_final.sh` (selbstständig, in der Messsperre, ohne D3D-Debugschicht): der ganze Lauf `modellleistung` (4 Figuren × 5 Lagen, je 120 Bilder; der Model-Tab steht dabei im Bereich Ansehen/Prüfen, die Zeitleiste in der Fassung Sequenz) einmal mit dem Programm der Basis (alt, `Y:\p_mbe\build\alt`), einmal mit dem neuen.

| 11.11. 14:32–14:41, **unter Last** (13 fremde Bau-Prozesse beim Start, 9 am Ende, 1 fremdes jkstudio) | alt | neu |
|---|---|---|
| Bild-CPU, Mittel je Lage: Median über die 20 Lagen | 1,15 ms | 1,29 ms |
| Bild-CPU, Mittel je Lage: Mittel über die 20 Lagen | 1,18 ms | 1,30 ms |
| Bild-CPU p99 je Lage: Median | 2,17 ms | 3,29 ms |
| Lage „Animation“ je Figur (stormtrooper, jedi_hf, jedi_hm, clonetrooper_p2) | 0,73 / 1,44 / 1,40 / 1,49 ms | 1,01 / 1,21 / 1,47 / 2,07 ms |

Einordnung: ein Richtwert, kein Ergebnis – beide Läufe liefen nacheinander neben fremden Bauten, die Streuung zwischen den Lagen (0,55 bis 1,51 ms im alten Programm) ist größer als der Abstand alt/neu (+0,14 ms im Median). Die Messung deckt die Fassung Sequenz ab, nicht den Bereich Animation mit offenen Spuren (dort zeichnet die Leiste bei 30 Bildern und 5 Spuren laut Selbsttest 30 + 150 feine Striche, 16 Teilstriche und 21 Schlüssel je Bild). Eine Endzahl gehört in ein Ruhefenster: `sh /y/werkzeuge/ap1/messung_final.sh` (RUNDEN=2 für alt/neu/alt/neu, dann in zwei Fenstern).

## Offen

- Die Endzahl der Bildzeit (Ruhefenster) steht aus; unter Last gemessen: +0,14 ms Bild-CPU im Median (Richtwert, siehe oben).
- Aus dem Anhang MBZEIT, im Entwurf nicht enthalten und nicht gebaut: Marker mit Namen (Blender M), Vorschaubereich, Schlüssel mit Umschalt+Ziehen kopieren (Kopieren/Einfügen gibt es im Rechtsklick und in der Suche), Pfeile am Zeitschieber.
- Auto Key aus gilt für Züge an Griffen und die Regler IK-FK/Fußrolle; Pose einfügen, Bindepose und Handposen setzen ihre Schlüssel weiterhin immer.
- Der Schalter des Bedienmodus oben im Befehlsfeld bleibt, bis AP3 ihn dort herausnimmt (R4); die Statuszeile bietet ihn schon an.
- Die Tastenzeile neben den Bereichsreitern (R3) nennt weiter die allgemeinen Tasten (AP3).
- Die übrigen Handbuchseiten des Model-Tabs zeigen in ihren Bildern noch die alte Leiste (AP14 nimmt alle Bilder neu auf); die Seite modell-kurven trägt einen Hinweis.

## Changelog

## English

- Model tab: the timeline now looks like the approved design and follows the chosen controls. Like 3ds Max: To start, Previous frame, Play, Next frame, To end, frame display from 0, Auto Key (N), Set Key (K), Time configuration, a fine line per frame in the scale and in every track, keys as bars in the colours of the track bar (red = move, green = rotate). Like Blender: jumps to the previous and next key (↓ / ↑), record button for auto keying, Start / End / Frame from 1, keys as diamonds, K opens the keying menu.
- Model tab: the tracks (dope sheet, curve editor) are part of the timeline - one ruler, frame 0 at the same place in scale and tracks; the handle above the timeline or the Tracks button opens them, in the area Animation they are open from the start. View/Check shows speed, interpolation, force loop and the primary and secondary sequence; the other areas show the short timeline.
- Model tab: Auto Key can be switched off - a pose then stays without a key until Set Key (K, like Blender I) keeps it; changing the frame discards it, as in 3ds Max and Blender.
- Model tab: status line at the bottom of the timeline (controls - click to switch, keys of the area, undo and search); Time configuration with frame rate, time display in frames or seconds, playback and the animation.cfg line of the sequence; the skin list and the surface counter moved to the group Level of detail and skin.

## Deutsch

- Model-Tab: die Zeitleiste sieht jetzt aus wie im freigegebenen Entwurf und folgt dem Bedienmodus. Wie 3ds Max: Anfang, Bild zurück, Abspielen, Bild vor, Ende, Bildanzeige ab 0, Auto Key (N), Set Key (K), Zeitkonfiguration, ein feiner Strich je Frame in der Skala und in jeder Spur, Schlüssel als Striche in den Farben der Spurleiste (Rot = Bewegung, Grün = Drehung). Wie Blender: Sprünge zum vorigen und nächsten Schlüssel (↓ / ↑), Aufnahme-Knopf für Auto-Keying, Start / Ende / Bild ab 1, Schlüssel als Rauten, K öffnet das Keying-Menü.
- Model-Tab: die Spuren (Dopesheet, Kurveneditor) gehören zur Zeitleiste - ein Lineal, Bild 0 in Skala und Spuren an derselben Stelle; der Griff über der Zeitleiste oder der Knopf Spuren öffnet sie, im Bereich Animation stehen sie ab Werk offen. Ansehen/Prüfen zeigt Tempo, Interpolation, Schleife erzwingen und die Primär- und Sekundärsequenz; die anderen Bereiche zeigen die kurze Zeitleiste.
- Model-Tab: Auto Key lässt sich ausschalten - eine Pose bleibt dann ohne Schlüssel, bis Set Key (K, wie Blender I) sie behält; ein Bildwechsel verwirft sie, wie in 3ds Max und Blender.
- Model-Tab: Statuszeile unten in der Zeitleiste (Bedienmodus - Klick wechselt, Tasten des Bereichs, Undo und Suche); Zeitkonfiguration mit Bildrate, Zeitanzeige in Frames oder Sekunden, Wiedergabe und der Zeile der Sequenz in der animation.cfg; Hautliste und Flächenzähler stehen jetzt in der Gruppe Detailstufe und Skin.

## 中文

- 模型选项卡: 时间轴现在与已确认的设计一致, 并随所选操作方式变化。像 3ds Max: 到开头、上一帧、播放、下一帧、到结尾、从 0 计数的帧显示、Auto Key (N)、Set Key (K)、时间配置, 标尺和每条轨道中每帧一条细线, 关键帧为轨迹栏颜色的竖条 (红 = 移动, 绿 = 旋转)。像 Blender: 跳到上一个和下一个关键帧 (↓ / ↑)、自动关键帧的录制按钮、从 1 计数的起始 / 结束 / 帧、菱形关键帧, K 打开关键帧菜单。
- 模型选项卡: 轨道 (摄影表、曲线编辑器) 成为时间轴的一部分 - 共用一把标尺, 第 0 帧在标尺和轨道中位于同一位置; 时间轴上方的手柄或“轨道”按钮可打开它们, 在动画区域默认打开。“查看/检查”显示速度、插值、强制循环以及主要和次要序列; 其他区域显示简短时间轴。
- 模型选项卡: 可以关闭 Auto Key - 此时姿势没有关键帧, 直到 Set Key (K, 像 Blender 时为 I) 将其保留; 换帧会丢弃它, 与 3ds Max 和 Blender 相同。
- 模型选项卡: 时间轴底部新增状态行 (操作方式 - 单击切换、该区域的按键、撤销和搜索); 时间配置包含帧率、以帧或秒显示时间、播放以及序列在 animation.cfg 中的那一行; 皮肤列表和面计数移到“细节层次和皮肤”组。

## 日本語

- モデルタブ: タイムラインが承認済みのデザインどおりになり、選んだ操作モードに従います。3ds Max 風: 先頭へ、前のフレーム、再生、次のフレーム、末尾へ、0 から数えるフレーム表示、Auto Key (N)、Set Key (K)、時間設定、スケールと各トラックにフレームごとの細い線、キーはトラックバーの色の棒 (赤 = 移動、緑 = 回転)。Blender 風: 前後のキーへのジャンプ (↓ / ↑)、自動キー挿入の録画ボタン、1 から数える開始 / 終了 / フレーム、ひし形のキー、K でキーイングメニュー。
- モデルタブ: トラック (ドープシート、カーブエディター) はタイムラインの一部になりました - 定規は 1 本で、フレーム 0 はスケールとトラックで同じ位置です。タイムライン上のハンドルまたは「トラック」ボタンで開き、アニメーションエリアでは最初から開いています。「表示/確認」は速度、補間、ループを強制、プライマリとセカンダリのシーケンスを表示し、他のエリアは短いタイムラインです。
- モデルタブ: Auto Key をオフにできます - ポーズは Set Key (K、Blender 風では I) で保持するまでキーなしのままで、フレームを変えると破棄されます (3ds Max、Blender と同じ)。
- モデルタブ: タイムラインの一番下にステータス行 (操作モード - クリックで切り替え、エリアのキー、元に戻すと検索)。時間設定にはフレームレート、フレームまたは秒での時間表示、再生、animation.cfg のシーケンスの行があります。スキン一覧とサーフェス数は「詳細レベルとスキン」グループに移りました。

## Español

- Pestaña Model: la línea de tiempo tiene ahora el aspecto del diseño aprobado y sigue los controles elegidos. Como 3ds Max: Al inicio, Fotograma anterior, Reproducir, Fotograma siguiente, Al final, indicador de fotograma desde 0, Auto Key (N), Set Key (K), Configuración de tiempo, una línea fina por fotograma en la escala y en cada pista, claves como barras con los colores de la barra de pistas (rojo = movimiento, verde = rotación). Como Blender: saltos a la clave anterior y siguiente (↓ / ↑), botón de grabación de claves automáticas, Inicio / Fin / Fotograma desde 1, claves como rombos, K abre el menú de claves.
- Pestaña Model: las pistas (hoja de claves, editor de curvas) forman parte de la línea de tiempo: una sola regla, el fotograma 0 en el mismo sitio en la escala y en las pistas; el tirador sobre la línea de tiempo o el botón Pistas las abre, y en el área Animación vienen abiertas. Ver/Comprobar muestra velocidad, interpolación, forzar bucle y las secuencias primaria y secundaria; las demás áreas muestran la línea de tiempo corta.
- Pestaña Model: Auto Key se puede desactivar: la pose queda sin clave hasta que Set Key (K, como Blender I) la conserva; cambiar de fotograma la descarta, como en 3ds Max y Blender.
- Pestaña Model: línea de estado al pie de la línea de tiempo (controles: clic para cambiar, teclas del área, deshacer y buscar); Configuración de tiempo con velocidad de fotogramas, tiempo en fotogramas o segundos, reproducción y la línea de la secuencia en animation.cfg; la lista de skins y el contador de superficies pasan al grupo Nivel de detalle y skin.

## Čeština

- Karta Model: časová osa teď vypadá jako ve schváleném návrhu a řídí se zvoleným ovládáním. Jako 3ds Max: Na začátek, Předchozí snímek, Přehrát, Další snímek, Na konec, zobrazení snímku od 0, Auto Key (N), Set Key (K), Nastavení času, jemná čárka na snímek ve stupnici i v každé stopě, klíče jako proužky v barvách lišty stop (červená = posun, zelená = rotace). Jako Blender: skoky na předchozí a další klíč (↓ / ↑), tlačítko nahrávání pro automatické klíčování, Začátek / Konec / Snímek od 1, klíče jako kosočtverce, K otevře nabídku klíčování.
- Karta Model: stopy (dopesheet, editor křivek) jsou součástí časové osy - jedno pravítko, snímek 0 je ve stupnici i ve stopách na stejném místě; otevře je úchyt nad časovou osou nebo tlačítko Stopy, v oblasti Animace jsou otevřené od začátku. Zobrazit/Zkontrolovat ukazuje rychlost, interpolaci, vynucenou smyčku a primární a sekundární sekvenci; ostatní oblasti ukazují krátkou časovou osu.
- Karta Model: Auto Key lze vypnout - póza pak zůstane bez klíče, dokud ji Set Key (K, jako Blender I) nezachová; změna snímku ji zahodí, jako ve 3ds Max a v Blenderu.
- Karta Model: stavový řádek dole v časové ose (ovládání - kliknutím přepnete, klávesy oblasti, zpět a hledat); Nastavení času se snímkovou frekvencí, časem ve snímcích nebo sekundách, přehráváním a řádkem sekvence v animation.cfg; seznam skinů a počítadlo povrchů jsou nově ve skupině Úroveň detailu a skin.
