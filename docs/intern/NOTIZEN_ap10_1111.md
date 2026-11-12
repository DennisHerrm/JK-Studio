# Notizen AP10 (11./12.11.): Model-Tab, Bereich Animation - das rechte Feld nach dem Entwurf - MBENTWURF Teil B, rc612

Begonnen von AP10 (Fable 5.2, ein WIP-Stand auf `b19b7f494`: Feld umgebaut, Selbsttest geschrieben, rot gegen das alte Feld 7 OK /
82 FEHLER; Endstand nie gebaut, nie grün), fertig gemacht von Opus 5.5 nach `Y:\werkzeuge\ap10\UEBERGABE.md`: auf karte1 `a679365e1`
(AP3, AP9, AP7, AP6, AP8) gesetzt, gebaut, grün gemacht, Funde der Läufe und Fotos behoben, Handbuchbilder, Notizen.

Auftrag `auftrag_mbentwurf_b_1011.md`, Paket AP10; Maßstab ist der freigegebene Entwurf (`plaene/modellblock_stufe0/entwurf_modelltab.html`,
Bereich Animation, Bilder `e_max_anim_lang.png` / `e_blender_anim_lang.png`), shanks Wünsche vom 04.11. (Presets) und 10.11. („es geht ja nicht
nur um controls like blender oder 3ds max sondern das aussehen“) und die Abgleichtabelle `abgleich_programm_entwurf.md`, Abschnitt 12 (A0a-A11,
A14). Worktree `Y:\p_ap10`, Zweig `p_ap10`. Selbsttest `gui/selbsttest_ap10.inc` (`BHED_EDITORTEST=modellblock`, `BHED_MODELL_NUR=ap10 |
ap10foto`; Handbuch `BHED_EDITORTEST=handbuch`, `BHED_HB_NUR=ap10`), Skripte und Läufe `Y:\werkzeuge\ap10\`.

## Was sich ändert

Das rechte Feld des Bereichs Animation zeichnet über den Baustein von AP3 (`ui::rolloutBeginn` je Gruppe). Vorher (rc611): „Control rig“, IK/FK,
Auto physics, Retargeting, Presets, Pose, Ghosts, Motion paths, Bake, Curves and animation tools, GLA, animation.cfg, Target GLA and slots in
dieser Folge, viele zu. Jetzt in der Folge des Entwurfs, die acht Gruppen ab Werk offen, dahinter (zu) was das Programm mehr kann:

| Gruppe (Entwurf) | Inhalt | Datei |
|---|---|---|
| **Animation** (A1) | Tabelle Name · Ziel-GLA · Plätze belegt/alle (gelb, wenn eine Karten-GLA voll ist); Kurzinfo = Sequenz | `app_modelltab_mb3.cpp` |
| **Rig erstellen** (A2) | Tabelle Skelett · Maßstab (fScale) · Erkannt als; Vorlagen als Wahl in zwei Spalten (Mensch, Rancor, Wampa, Tauntaun, Howler, Sandkreatur, AT-ST, Droiden, Fahrzeuge/Schiffe, Objekte und Geschütze, Eigenes Skelett) und bei mehreren Vorlagen einer Gruppe die Klappliste; Hauptknopf Control Rig erstellen (mit Rig: Neu erkennen); Animation-Rig (nur FK) · Auf Rig legen; Tipp | `app_modelltab_anim.cpp`, `app_modelltab_rig.cpp` |
| **Presets** (A3) | Kategorien als Wahl, Liste mit Quelle · Lizenz rechts, Ziel-Platz als Wahl (Platz des Presets, die Plätze der anderen Presets der Kategorie, der erste freie Kino-Platz), Vorschau auf Figur · Generator …, Hauptknopf Auf Figur legen (Retarget), Tipp; der Generator klappt per Knopf unter der Bibliothek auf | `app_modelltab_retarget.cpp` |
| **Vorhandene Animation bearbeiten** (A4) | Tabelle Quelle · Bilder · Schlüssel; Frames zu Schlüsseln reduzieren · In die GLA backen; Optionen zu; Tipp GLA/XSI | `app_modelltab_kurven.cpp` |
| **Übertragen (Retargeting)** (A5) | Quelle wählen · Animation importieren, Tabelle Quelle/Ziel, Teile Charakterisierungs-Pose · Charakter-Definition als Knöpfe, Ketten als Tabelle (✓ / ähnlich), Schritte als Schalter, Platz, Hauptknopf, Fußrutschen | `app_modelltab_retarget.cpp` |
| **IK/FK** (A6/A7) | Max: Name │ Zustand („FK, zuletzt gedreht“) │ Knopf IK/FK; Blender: Name │ Regler │ IK → FK · FK → IK in einer Zeile; Tipp; das gewählte Glied (Raum, Fußrolle, Einrasten, Schlüssel löschen, Backen) | `app_modelltab_ik.cpp` |
| **Anbauten und Vorschau** (A8) | Tabelle Anbauten · Primär · Sekundär ab Startknochen; Tipp | `app_modelltab_anim.cpp` |
| **Für das Spiel bauen** (A9) | Wurzelweg (Becken erstes → letztes Bild, wie Stufe 7), Ziel-GLA, Hauptknopf, Tipp SET_ORIGIN | `app_modelltab_mb3.cpp` |
| dahinter (A14) | Auto-Physik, Pose, Ghosts, Bewegungspfade, Kurven und Animationswerkzeuge, Posen-Bibliothek, GLA, animation.cfg, Ziel-GLA und Plätze | rig/physik/kurven/mb3 |

Reiter je Gruppe (Baustein `.reiter`): Max Bewegung (alle), Hierarchie (IK/FK, Anbauten), Dienstprogramme (Bauen, Posen-Bibliothek, GLA,
animation.cfg, Plätze); Blender Knochen (alle), Constraints, Daten, Objekt, Werkzeug.

- **Rig-Vorlagen** (`modell::rtVorlagen`, 40 Einträge: die 34 Skelette des Basisspiels und Rigs anderer Programme) mit Gruppe
  (`RtVorlageInfo::gruppe`, Tabelle von Hand `kVorlageGruppe` in `src/modell/retarget.cpp`) und Rollen (`rtVorlageRollen`). Die Vorlage füllt beim
  Rig nur Knochen, die die Erkennung offen lässt oder unklar nennt; Zuordnungen von Hand gelten zuletzt. Für `_humanoid` 53/53 erkannt, 0 unklar
  (Lauf `ap10_g4`), Rancor 55 von 56 mit und ohne Vorlage gleich.
- **Animation-Rig (nur FK)**: das Rig ohne Pol-Ziele und Fußrolle (26 statt 34 Griffe, 0 IK); Hand und Fuß greifen ohne IK; die Gruppe IK/FK
  zeigt dann einen Hinweis statt der Glieder.
- **Auf Rig legen**: `rigReduzieren` über die ganze Sequenz und je Glied `ikAufFkBereich(…, 0)`: die Bilder sind Schlüssel an den Griffen, die
  IK-Ziele folgen an jedem Bild, Anteil 0 - die Pose bleibt (gemessen 0,0479 Einheiten an der Hand, siehe Selbsttest). Ein Rückgängig-Schritt.
- Werkzeuge für die Werkzeugleiste wie Blender (A11: Pose kopieren, Gespiegelt einfügen, Ausdünnen) als Schnittstelle
  `modellAnimWerkzeugZahl/…/Ausloesen` - die Leiste zeichnet AP4.
- Aus dem Feld entfernt: die Gruppe „Bake“ (Inhalt jetzt in A4) und die Knöpfe Dope sheet / Curve editor (stehen in der Zeitleiste, AP1).
  Alles bleibt in „Alle Aktionen suchen“, im Menü Animation und im Rechtsklick (Rig-Tabelle: neu „Animation rig (FK only)“, „Put on rig“).

### Ergänzung am Baustein (AP3, API kompatibel)
`ui::segment(…, breite, bool umbrechen = false)`: mit `umbrechen` (nur mit `spalten > 0`) bricht ein Text in seiner Zelle um, die Reihe wird so
hoch wie ihr längster Text, die Spalten bleiben, solange das breiteste Wort (CJK: Zeichenpaar) passt; jede Zeile waagrecht mittig. Anlass: der
Entwurf zeigt „Mensch (_humanoid + 32 Karten-GLAs)“ zweizeilig in der linken Spalte - ohne fiel die ganze Vorlagen-Wahl auf eine Spalte zurück
(Fund 1). Ohne den Schalter zeichnet `segment` wie vorher (ap3 105/0, ap6-ap9 grün auf diesem Bau). Benutzt von der Vorlagen-Wahl und den
Kategorien der Presets. Eingetragen in `Y:\werkzeuge\ap3\SCHNITTSTELLE.md`.

Texte in `tools/i18n_ap10.py` (Präfix `Az`, 6 Sprachen). Entfernt, weil unbenutzt: `AiBibliothek`, `AiKategorie`, `AiAuflegen`
(`tools/i18n_animimp.py`, die alte Gruppe „Library“), `AzPresetVorschauHint` (der Knopf nimmt `AiAuflegenHint`, gleiche Aussage),
`AzPresetWaehlen` (der Hauptknopf ist ohne Wahl gesperrt). Deutsch „Charakterisierungs-Pose“ → „Pose zur Charakterisierung“ (Fund 3).
lint_i18n ok.

## Funde (rot nachgestellt, behoben, grün)

1. **Vorlagen-Wahl in einer Spalte** (Lauf `ap10_g1`, A2 rot: „mensch 3024/619-3717/667 rancor 3024/667-3717/715“ - jeder Eintrag über die
   ganze Breite): „Human (_humanoid and map GLAs)“ ist breiter als eine halbe Zeile; `ui::segment` fiel dann auf eine Spalte zurück. Behoben
   mit `umbrechen` (oben). Grün in `ap10_g2` („mensch 3024/619-3370/703 rancor 3370/619-3717/703“, zweizeilig wie im Entwurf), in beiden Modi.
2. **Helles Thema: gewähltes Preset unlesbar** (Foto `ap10_f_hell`, erste Aufnahme): das erste helle Thema wählt dunkelblau, die dunkle Schrift
   stand darauf kaum sichtbar. Jetzt hell oder dunkel nach der Helligkeit der Auswahlfarbe (Regel von `ui::segment`), auch Quelle · Lizenz.
   Zweite Aufnahme: lesbar.
3. **Wort mitten im Knopf gebrochen** (Foto `ap10_f_de`): „Charakterisierungs-Po / se“ im Raster von Übertragen. Text „Pose zur
   Charakterisierung“ (bricht an der Leerstelle). Zweite Aufnahme: sauber.
4. **Quelle doppelt** (Foto en): unter der Liste stand „Source: Quaternius … licence CC0 1.0“, obwohl Quelle · Lizenz schon in der Zeile standen.
   Jetzt nur, wenn sie nicht in die Zeile passten (die Mocap-Einträge mit langem Namen).
5. **Ziel-Platz** stand als Eingabefeld da, der Entwurf zeigt eine Wahl (BOTH_WALK1 · BOTH_WALK2 · BOTH_CIN_4). Jetzt als Wahl; ein anderer Name
   geht weiter über das Feld Platz in Übertragen (Tooltip sagt es).

## Selbsttest (rot -> grün)

- Rot gegen das alte Feld (Vorgänger, `ap10_rot1`): **7 OK, 82 FEHLER** (Protokoll `Y:\werkzeuge\ap10\rot1_protokoll.txt`).
- Erster Lauf mit neuem Feld (`ap10_g1`): 63 OK, 6 FEHLER - Fund 1 (A2, beide Modi) und Testfehler: zwei Einträge einer Wahl stoßen
  aneinander (die Prüfung verlangte eine Lücke wie im Raster - neu `segmentNachbarn`, Breiten bis 1 Bildpunkt gleich, weil die Zellgrenzen auf
  ganzen Bildpunkten liegen); „Idle“ ist Kategorie und erstes Preset (die Kategorie ist der Eintrag über der Liste); „Source“ steht in
  Bearbeiten und Übertragen (`imRollIn` sucht in der Gruppe); A4 nahm nur einen Rückgängig-Schritt zurück (das Backen) - jetzt prüft A4 auch
  den Platz des Backens und nimmt Backen und Reduzieren zurück (neue Zeile „zwei Schritte Rückgängig“), A1 erwartet genau 2 belegte Plätze
  (Preset, Übertragung). Keine Prüfung abgeschwächt.
- `ap10_g2` 69/1 (Source), `ap10_g3` **70 OK, 0 FEHLER**; Toleranz „Auf Rig legen: Pose bleibt“ gemessen 0,0479 Einheiten in drei Läufen gleich,
  Grenze von 0,25 (gesetzt) auf 1/16 Einheit verengt (über dem Messwert, halb so weit wie die Summe der Verschiebungsstufen entlang der
  Hand-Kette 8 × 1/64); `ap10_g4` mit der engen Grenze **70 OK, 0 FEHLER**.

## Angepasste vorhandene Selbsttests (nichts gelöscht)

- `gui/selbsttest_mb6.inc` „MB6 Rollout IK/FK“: sucht statt des Hakens „IK enabled“ den Knopf IK/FK je Glied (`FK##azik`/`IK##azik`) und
  „IK/FK einrasten“; rollt die Gruppe über ihre Kopfzeile im rechten Feld ins Bild (`rig3::rollenZu` nahm die erste Beschriftung „IK/FK“ im
  Programm - das gleichnamige Menü Animation > IK/FK; rot `n1_mb_mb6` 37/1, grün `n2_mb_mb6` 38/0).
- `gui/selbsttest_rt.inc` Handbuchschritt: rollt zu „Transfer (retargeting)“ statt „Retargeting“.
- `gui/selbsttest_ai.inc` „X > Preset-Bibliothek öffnen“: sucht „Preview on figure“ (bis rc611 „Put on figure“, dieselbe Funktion).

## Fotos

`Y:\werkzeuge\ap10\fotos\ap10_f_<variante>\` (Selbsttest `ap10foto`, 3818 × 2104 bei Oberfläche 1,5; je Lauf 0 FEHLER): en, de, zh-Hans, ja,
es, cs, hell, klein (1328 × 844 bei 1,0). Seiten des Felds nebeneinander: `Y:\werkzeuge\ap10\vergleich\m_<variante>\`; neben dem Entwurf:
`vergleich\v_en_max.png`, `v_en_blender.png`. Angesehen (alle Seiten des Felds, beide Modi en, Max in allen Sprachen, Blender de/es/ja):
- Max/Blender en: Folge und Form der acht Gruppen wie im Entwurf, Hauptknöpfe blau, Tabellen, Tipps als Kasten; Blender mit runden Panels,
  senkrechter Reiterspalte, IK/FK als Regler mit zwei Knöpfen in einer Zeile.
- de, zh-Hans, ja, es, cs: nichts abgeschnitten; lange Texte brechen um (Fund 3 behoben).
- hell: Fund 2 behoben; Hauptknöpfe und Wahl in der Betonungsfarbe des Themas.
- klein: das Feld zeigt bei 1328 × 844 im Bereich Animation nur etwa zwei Zeilen, weil die volle Zeitleiste (AP1) rund 40 % der Höhe nimmt; die
  Trennkante ist ziehbar. Nicht in AP10 geändert (Rahmen AP3, Zeitleiste AP1) - siehe Offen.

## Handbuch

- Eigene Quelle `tools/handbuch/inhalt_ap10.py` (6 Sprachen): Seite „Model-Tab: das Feld im Bereich Animation“ (Gruppen, Rig, Presets, IK/FK);
  wie AP6-AP9 nicht eingehängt - AP14 führt zusammen (Anleitung im Kopf der Datei). Probe mit vorübergehend eingehängter Seite:
  `gen_handbuch.py --pruefen` 179 Seiten, ok.
- Bilder `h_ap10_max`, `h_ap10_blender` in 6 Sprachen unter `handbuch/bilder/<sprache>/` (2538 × 1472, je Lauf 0 FEHLER; angesehen en Max und
  ja Blender).
- Eingehängte Seite „Animation importieren“ (`inhalt_animimp.py`): der Absatz zur Preset-Bibliothek beschreibt die neue Gruppe (Bibliothek oben,
  Vorschau auf Figur, Ziel-Platz, Auf Figur legen, Generator per Knopf) in 6 Sprachen; `gen_handbuch.py` lädt dafür die Texte von `i18n_ap10`;
  Handbuch neu erzeugt (6 × `modell-animimport.html`, `suche.js`). Das Bild `h_ai_bibliothek` zeigt noch das alte Feld (nimmt AP14 mit allen
  Bildern des Model-Tabs neu auf).
- `gen_handbuch.py --pruefen` ok (Hinweis „2 Einträge in uebersetzung/ nicht mehr gebraucht“ wie auf karte1).

## Schlussrunde des Zweigs

| Schritt | Ergebnis |
|---|---|
| Bau alle Ziele (inkrementell, /W4 /WX) | 0 Fehler, 0 Warnungen (bau_8, alle Ziele, auf dem Endstand) |
| ctest | 257 / 257 |
| `ap10` | 70 / 0 (rot 7 / 82 gegen das alte Feld) |
| Nachbargruppen (Runde `n1_*`, `n2_mb_mb6`) | mb3 22 · rig 27 · mb5 160 · mb6 38 · mb7 25 · ph 30 · rt 20 · rtjka 2 · pr 16 · ai 32 · pfix 18 · blender 68 · funktionen 62 · tasten 44 · mb2 38 · ap1 137 · ap3 105 · ap6 94 · ap7 88 · ap8 104 · ap9 97 · ap12 35 - je 0 FEHLER; karteneditor `rd` 39 / 0 |
| Fotos `ap10foto` | 8 Varianten, je 0 FEHLER |
| Handbuchbilder | 6 Sprachen, je 0 FEHLER |
| Lints (21 aus `lints.sh`, dazu efxflags, fremdprogramm, d3dbesitz, merkmalsbits) | ok bzw. wie karte1: pruefcodes 11, portability 99, efxflags und fremdprogramm mit derselben Meldung |

Bewusst nicht gelaufen (einmal auf dem zusammengeführten Stand): Modus 1, die übrigen modellblock-Gruppen, karteneditor voll, menueeditor,
sauberer Neubau, Sprachläufe. Keine Bildzeit-Messung (das Feld zeichnet dieselben Elemente in anderer Folge; ohne Messung keine Aussage zum Tempo).

## Stand je Zeile der Abgleichtabelle (für den Hauptchat; die Tabelle hat AP10 nicht angefasst)

| Zeile | Stand |
|---|---|
| A0a Aufbau wie 3ds Max | gleich im Feld (Reiter Bewegung, acht Gruppen in der Folge des Entwurfs); vier Ansichten als Vorgabe - R7/AP5 (Alt+W wie bisher); volle Zeitleiste - AP1 |
| A0b Aufbau wie Blender | Feld gleich (Reiter Knochen, Blender-Panels); Werkzeugleiste, Kopf „Pose-Modus“ - AP4 |
| A1 Animation | gleich; bewusst anders: „Ziel-GLA“ statt „Karten-GLA“ (das Ziel ist wählbar: Karten-GLA, _humanoid.gla, eigene GLA) |
| A2 Rig erstellen | gleich; bewusst mehr: Klappliste der Vorlage in Gruppen mit mehreren (Mensch: _humanoid JKA/JK2, HumanIK/Mixamo, Biped, CAT, Unreal, 100STYLE …), Rig-Übersicht, Schlüssel löschen, Stand des Rigs; Text ohne die Zahl „32“ |
| A3 Presets | gleich (Kategorien als Wahl, Liste mit Quelle · Lizenz, Ziel-Platz als Wahl, Vorschau · Generator, Hauptknopf, Tipp); offen: Kategorien nach den Daten (5) statt der acht des Entwurfs (Frage 3); bewusst anders: Regler Tempo/Armschwung im Generator (sie gelten nur für Generator-Gänge, nicht für die Mocap-Dateien der Liste) |
| A4 Vorhandene Animation bearbeiten | gleich; bewusst mehr: Zeile Schlüssel, Optionen (zu) |
| A5 Übertragen | gleich in der Form; bewusst mehr: Sequenz, Vorschau, Schritte, Bericht; der Hauptknopf heißt wie in Stufe 5 je Modus („Auf Platz plotten“ wie MotionBuilder, Blender „Auf Platz backen“) |
| A6 IK/FK (Max) | gleich |
| A7 IK/FK (Blender) | gleich (Regler und zwei Knöpfe in einer Zeile) |
| A8 Anbauten und Vorschau | gleich |
| A9 Für das Spiel bauen | gleich (Wurzelweg gegen unabhängige Rechnung geprüft) |
| A10 Ansicht: Rig | unverändert; Farben nach Seite statt nach Art - Frage 1 |
| A11 Werkzeugleiste | Schnittstelle fertig (drei Werkzeuge mit Symbol, Taste, Aktion); die Leiste zeichnet AP4 |
| A14 mehr als der Entwurf | gleich (alle weiteren Gruppen hinter den acht, je mit Reiter) |

## Offen / Fragen an shank

1. A10: Griffe nach Seite färben (heute: links blau, rechts grün, Mitte gelb - wie Biped) oder nach Art wie im Entwurf (IK gelb, FK grün, Pol
   lila)? Umstellen wäre eine Farbtabelle in `modellRigUeberlagerung`.
2. „Auf Rig legen“ (im Entwurf nicht erklärt): gebaut als „die Animation als Schlüssel an die Griffe, die IK-Ziele folgen, die Pose bleibt“ -
   so gemeint?
3. Presets: Kategorien nach den ausgelieferten Daten (heute 5: Stehen, Gehen, Laufen, Kampf, Sonstiges) oder die acht des Entwurfs auch leer
   zeigen? Die Texte für Rückwärts, Ducken, Springen, Gesten liegen bereit; Umstellen ist eine Liste in `presetKategorien`.
4. Regler Tempo/Armschwung unter der Liste (Entwurf) gelten nur für Generator-Gänge - so lassen (im Generator) oder Generator-Gänge als Einträge
   der Liste zeigen?
- Mindestgröße (1328 × 844): im Bereich Animation nimmt die volle Zeitleiste rund 40 % der Höhe, das rechte Feld zeigt etwa zwei Zeilen
  (AP1/AP3/AP14).
- `tools/modell/rig_vorlagen_gen.py` passt nicht zu `src/modell/rig_vorlagen.inc` (die .inc hat „Unreal Mannequin“ und „100STYLE (BVH)“, der
  Generator nicht) - ein Lauf des Generators würde beide Vorlagen löschen. Nicht angefasst.
- Wünsche an den Baustein `Y:\werkzeuge\ap10\WUNSCH_AP3.md` (6 Punkte, offen); das Umbrechen im Segment hat AP10 selbst ergänzt (oben).
- Handbuchbild `h_ai_bibliothek` und die übrigen Bilder des Bereichs Animation zeigen noch das alte Feld (AP14).

## Changelog

## English

- Model tab, Animation area: the panel on the right follows the approved design - Animation, Create rig, Presets, Edit existing animation, Transfer (retargeting), IK/FK, Attachments and preview, Build for the game; behind them, closed, everything else (auto physics, pose, ghosts, motion paths, curves, pose library, GLA, animation.cfg, target GLA and slots).
- Create rig: shows skeleton, scale and the recognised template; a template can be chosen by group (human, rancor, wampa, tauntaun, howler, AT-ST, droids, vehicles …) or Own skeleton. New: Animation rig (FK only) and Put on rig (the animation becomes keys on the controls, the pose stays).
- Presets: the library comes first - category, list with source and licence, target slot as a choice, Preview on figure and Put on figure (retarget); the generator opens with its button.
- Edit existing animation: Reduce frames to keys and Bake into the GLA in one group with source, frames and keys.
- IK/FK like 3ds Max: per limb its state in words and one IK/FK button; like Blender: slider and both snap buttons in one row.
- Build for the game shows the root travel of the sequence (how far the pelvis moves) and reminds of SET_ORIGIN.
- Light theme: the selected preset is readable again.

## Deutsch

- Model-Tab, Bereich Animation: das Feld rechts folgt dem freigegebenen Entwurf - Animation, Rig erstellen, Presets, Vorhandene Animation bearbeiten, Übertragen (Retargeting), IK/FK, Anbauten und Vorschau, Für das Spiel bauen; dahinter, zugeklappt, alles Weitere (Auto-Physik, Pose, Ghosts, Bewegungspfade, Kurven, Posen-Bibliothek, GLA, animation.cfg, Ziel-GLA und Plätze).
- Rig erstellen: zeigt Skelett, Maßstab und die erkannte Vorlage; eine Vorlage lässt sich nach Gruppe wählen (Mensch, Rancor, Wampa, Tauntaun, Howler, AT-ST, Droiden, Fahrzeuge …) oder Eigenes Skelett. Neu: Animation-Rig (nur FK) und Auf Rig legen (die Animation wird zu Schlüsseln an den Griffen, die Pose bleibt).
- Presets: die Bibliothek steht vorn - Kategorie, Liste mit Quelle und Lizenz, Ziel-Platz als Wahl, Vorschau auf Figur und Auf Figur legen (Retarget); der Generator öffnet sich über seinen Knopf.
- Vorhandene Animation bearbeiten: Frames zu Schlüsseln reduzieren und In die GLA backen in einer Gruppe mit Quelle, Bildern und Schlüsseln.
- IK/FK wie 3ds Max: je Glied der Zustand in Worten und ein Knopf IK/FK; wie Blender: Regler und beide Einrast-Knöpfe in einer Zeile.
- Für das Spiel bauen zeigt den Wurzelweg der Sequenz (wie weit das Becken wandert) und erinnert an SET_ORIGIN.
- Helles Thema: das gewählte Preset ist wieder lesbar.

## 中文

- 模型选项卡的动画区域: 右侧面板按已确认的设计排列 - 动画、创建绑定、预设、编辑现有动画、传递 (重定向)、IK/FK、附加物与预览、为游戏生成; 其后折叠显示其余内容 (自动物理、姿势、残影、运动路径、曲线、姿势库、GLA、animation.cfg、目标 GLA 与槽位)。
- 创建绑定: 显示骨架、比例和识别出的模板; 可按组选择模板 (人类、Rancor、Wampa、Tauntaun、Howler、AT-ST、机器人、载具 …) 或自定义骨架。新增: 动画绑定 (仅 FK) 和放到绑定上 (动画变为控制器上的关键帧, 姿势保持不变)。
- 预设: 动画库在最前 - 类别、带来源和许可的列表、可选的目标槽位、在角色上预览和应用到角色 (重定向); 生成器通过其按钮打开。
- 编辑现有动画: 将帧精简为关键帧和烘焙到 GLA 集中在一个组中, 并显示来源、帧和关键帧。
- 3ds Max 风格的 IK/FK: 每个肢体显示文字状态和一个 IK/FK 按钮; Blender 风格: 滑块和两个对齐按钮在同一行。
- “为游戏生成”显示序列的根位移 (骨盆移动的距离) 并提醒 SET_ORIGIN。
- 浅色主题: 所选预设重新清晰可读。

## 日本語

- モデルタブのアニメーションエリア: 右のパネルが承認済みのデザインに沿って並びます - アニメーション、リグを作成、プリセット、既存のアニメーションを編集、転送 (リターゲット)、IK/FK、取り付けとプレビュー、ゲーム用に作成。その後ろに閉じた状態でその他すべて (オートフィジックス、ポーズ、ゴースト、モーションパス、カーブ、ポーズライブラリ、GLA、animation.cfg、出力先 GLA とスロット)。
- リグを作成: スケルトン、スケール、認識されたテンプレートを表示します。テンプレートはグループ (人型、Rancor、Wampa、Tauntaun、Howler、AT-ST、ドロイド、乗り物 …) または独自スケルトンから選べます。新たにアニメーションリグ (FK のみ) とリグに載せる (アニメーションがコントロールのキーになり、ポーズはそのまま)。
- プリセット: ライブラリが先頭に - カテゴリ、出典とライセンス付きの一覧、選べる出力先スロット、フィギュアでプレビューとフィギュアに載せる (リターゲット)。ジェネレーターはボタンで開きます。
- 既存のアニメーションを編集: 「フレームをキーに削減」と「GLA にベイク」を、ソース・フレーム・キーと一緒に 1 つのグループに。
- 3ds Max 風の IK/FK: 肢ごとに状態を文字で示し、IK/FK ボタンが 1 つ。Blender 風: スライダーと 2 つのスナップボタンを 1 行に。
- 「ゲーム用に作成」はシーケンスのルートの移動 (骨盤の移動距離) を示し、SET_ORIGIN を案内します。
- ライトテーマ: 選択したプリセットが再び読みやすくなりました。

## Español

- Pestaña Model, área Animación: el panel de la derecha sigue el diseño aprobado - Animación, Crear rig, Presets, Editar animación existente, Transferir (retargeting), IK/FK, Acoples y vista previa, Crear para el juego; detrás, cerrado, todo lo demás (física automática, pose, fantasmas, trayectorias, curvas, biblioteca de poses, GLA, animation.cfg, GLA de destino y huecos).
- Crear rig: muestra esqueleto, escala y la plantilla reconocida; la plantilla se puede elegir por grupo (humano, rancor, wampa, tauntaun, howler, AT-ST, droides, vehículos …) o Esqueleto propio. Nuevo: Rig de animación (solo FK) y Poner en el rig (la animación pasa a claves en los controles, la pose se mantiene).
- Presets: la biblioteca va primero - categoría, lista con origen y licencia, hueco de destino como elección, Vista previa en la figura y Poner en la figura (retarget); el generador se abre con su botón.
- Editar animación existente: Reducir fotogramas a claves y Hornear en el GLA en un solo grupo con origen, fotogramas y claves.
- IK/FK como 3ds Max: por extremidad su estado en palabras y un botón IK/FK; como Blender: regulador y los dos botones de ajuste en una fila.
- Crear para el juego muestra el recorrido de la raíz de la secuencia (cuánto avanza la pelvis) y recuerda SET_ORIGIN.
- Tema claro: el preset elegido vuelve a leerse bien.

## Čeština

- Karta Model, oblast Animace: panel vpravo odpovídá schválenému návrhu - Animace, Vytvořit rig, Předvolby, Upravit existující animaci, Přenos (retargeting), IK/FK, Připojené a náhled, Sestavit pro hru; za nimi zavřené vše ostatní (automatická fyzika, póza, duchové, dráhy pohybu, křivky, knihovna póz, GLA, animation.cfg, cílové GLA a sloty).
- Vytvořit rig: ukazuje kostru, měřítko a rozpoznanou šablonu; šablonu lze zvolit podle skupiny (člověk, rancor, wampa, tauntaun, howler, AT-ST, droidi, vozidla …) nebo Vlastní kostra. Nově: Animační rig (jen FK) a Položit na rig (animace se stane klíči na ovladačích, póza zůstane).
- Předvolby: knihovna je vpředu - kategorie, seznam se zdrojem a licencí, cílový slot jako volba, Náhled na postavě a Položit na postavu (retarget); generátor se otevře svým tlačítkem.
- Upravit existující animaci: Zredukovat snímky na klíče a Zapéct do GLA v jedné skupině se zdrojem, snímky a klíči.
- IK/FK jako 3ds Max: u každé končetiny stav slovy a jedno tlačítko IK/FK; jako Blender: posuvník a obě tlačítka přichycení v jednom řádku.
- Sestavit pro hru ukazuje posun kořene sekvence (jak daleko se pánev posune) a připomíná SET_ORIGIN.
- Světlý motiv: zvolená předvolba je opět čitelná.
