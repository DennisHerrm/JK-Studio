# Notizen AP3 (11.11.): Rollout-Baustein und Rahmen des rechten Felds des Model-Tabs - MBENTWURF Teil B, rc612

Auftrag `auftrag_mbentwurf_b_1011.md`, Paket AP3; Maßstab ist der freigegebene Entwurf (`plaene/modellblock_stufe0/entwurf_modelltab.html`),
shanks Wünsche vom 04.11. (Bild 1: „kann man das noch schöner machen? einfach benutzerfreundlicher und übersichtlicher“) und vom 10.11.
(„es geht ja nicht nur um controls like blender oder 3ds max sondern das aussehen“) und die Abgleichtabelle
`abgleich_programm_entwurf.md` (Abschnitt 6: F1-F13, F16; Abschnitt 3: R2-R4).
Worktree `Y:\p_ap3`, Zweig `p_ap3`, Basis karte1 `852d12be7`. Selbsttest `gui/selbsttest_ap3.inc` (`BHED_EDITORTEST=modellblock`,
`BHED_MODELL_NUR=ap3 | ap3foto`, Teile mit `BHED_AP3_TEIL=baustein|rahmen|muster`), Skripte und Läufe `Y:\werkzeuge\ap3\`.

## Der Baustein (`gui/ui_rollout.h`, `gui/ui_rollout.cpp`)

Für Welle 2 (AP4-AP10) als Schnittstelle zuerst committet; Kurzfassung und Beispiel in `Y:\werkzeuge\ap3\SCHNITTSTELLE.md`, die
vollständige Beschreibung im Kopf von `gui/ui_rollout.h`. Namensraum `bhed::gui::ui`, eingebunden über `app_internal.h`.

| Baustein | Aufruf | Zeile der Tabelle |
|---|---|---|
| Rahmen mit Reitern | `feldBeginn` / `feldEnde` | F1, F2 |
| Gruppe mit Kurzinfo, Schrittnummer, Kasten | `rolloutBeginn` / `rolloutEnde`, Klasse `Rollout` | F3, F4, F9, F10 |
| Knopfraster, Knopf mit Symbol und Taste, Hauptknopf | `rasterBeginn` / `rasterEnde`, `werkzeugKnopf` | F5, F6 |
| Regler, Formularzeile | `regler`, `reglerInt`, `zeile`, `zeileOhne` | F7 (und der offene Punkt von UIFORM) |
| Werte, Grenz-Balken | `wertTabelle`, `grenzBalken`, `balken` | F8 |
| Tipp, Absatz, Listenzeile | `tipp`, `absatz`, `eintrag` | F11 |
| Schalterliste, Wahl | `schalter` im Raster, `segment` | F12 |
| eingeklappter Unterteil | `mehrBeginn` / `mehrEnde` | Abschnitt 13 („Weitere“, „Erweitert“) |

Wie er arbeitet:
- Eine Gruppe ist gewöhnliches ImGui im selben Fenster (kein Kindfenster): der Inhalt steht eingerückt, der Kasten wird über einen
  eigenen `ImDrawListSplitter` am Ende UNTER den Inhalt gezeichnet. Jede vorhandene Zeichenfunktion passt hinein (z. B.
  `drawModellAutoSkinGruppe()` von AP12, `modellHautZeile()` von AP1).
- Kennung und Zustand einer Gruppe wie bei `ImGui::CollapsingHeader` (`GetID(titel)`, StateStorage des Fensters, im Element-Register
  Openable/Opened): vorhandene Selbsttests finden, klappen und rollen die Gruppen wie bisher.
- Stil je Bedienmodus: wie 3ds Max flache Überschriftsbalken mit +/−, eckig, Beschriftung links; wie Blender abgerundete Kästen
  (0,32 Zeichenhöhen), Pfeil, Beschriftung rechtsbündig vor dem Feld. Farben aus dem Thema (Betonungsfarbe = die des
  Formular-Bausteins), Zustände gut/Warnung/Fehler in Grün/Gelb/Rot je für helles und dunkles Thema.
- Nichts wird abgeschnitten: Texte brechen um (CJK über `textumbruch.h`), das Raster nimmt bei schmalem Feld eine Spalte (Zelle unter
  6 Zeichenhöhen), eine Beschriftung, die nicht in die Spalte passt, rückt über ihr Feld, Titel und Kurzinfo stehen notfalls
  untereinander, die Taste eines Knopfs weicht in den Tooltip.
- Jedes bedienbare Element (auch außerhalb des sichtbaren Bereichs) steht mit seiner Beschriftung im Element-Register.

## Rahmen des rechten Felds (F1, F2, F13) und Kopfzeile des Tabs (R2, R3, R4)

Neue Datei `gui/app_modelltab_feld.cpp`, Zustand und Deklarationen als Block am Ende von `gui/modelltab.h`.
- **Reiter (F1, F2):** wie 3ds Max Titelzeile „Command panel │ <Bereich>“ und die Reiter Create · Modify · Hierarchy · Motion · Display ·
  Utilities als Reihe mit Text; wie Blender „Properties │ <Bereich>“ und die Reiter Tool · Object · Modifiers · Data · Bone ·
  Constraints · Material als senkrechte Symbolspalte (eigene Symbole in `uiicons.cpp`, Name im Tooltip). Der Bereich wählt den Reiter
  des Entwurfs (Ansehen/Prüfen → Display bzw. Object, Animation → Motion bzw. Bone, Modellieren → Modify bzw. Modifiers, UV und Weights
  → Modify bzw. Data, Painting → Modify bzw. Tool).
- **Jede Gruppe gehört zu Reitern:** `ui::RolloutOpt::reiter` über `modellReiter(MbReiter, MbReiterBl)`. Im Reiter, den der Bereich
  vorgibt, stehen wie im Entwurfsbild alle Gruppen des Bereichs untereinander; die anderen Reiter zeigen ihren Teil. Ein Reiter ohne
  Gruppe im Bereich ist gedämpft und nicht wählbar (kein leeres Feld).
- **Breite (F13):** Vorgabe 22 Zeichenhöhen (vorher 17), Trennkante zwischen Ansichten und Feld zum Ziehen (12 bis 44 Zeichenhöhen, die
  Ansicht behält 8), Doppelklick = Vorgabe; Einstellung `modellFeldBreite` (`modelPanelWidth`).
- **Bereichsreihe (R2):** `drawModellBereiche()` zeichnet die sechs Bereiche als eine Reihe gleich breiter Felder (so breit wie der
  längste Name, mindestens 7,5 Zeichenhöhen; der gewählte in der Betonungsfarbe) über der ganzen Breite des Tabs - über Explorer,
  Ansichten und rechtem Feld, rechts neben der Modusleiste (`bereichZeichnen` in `app.cpp`, damit im Hauptfenster, im DockSpace und
  im eigenen Fenster des Tabs). Vorher: Knöpfe in Textbreite in der Ansicht.
- **Hinweis (R3):** daneben je Bedienmodus und Bereich ein kurzer Hinweis (Ansehen/Prüfen, bearbeitende Bereiche, Animation) und
  dahinter die Mausnavigation der Ansicht; passt er nicht, wird auf ganze Abschnitte gekürzt (Rest im Tooltip). Die Tasten je Bereich
  stehen in der Statuszeile (AP1).
- **Schalter Bedienmodus (R4):** steht nicht mehr im Kopf des rechten Felds. Er bleibt in der Statuszeile (AP1) und steht jetzt auch
  im Menü der Kopfzeile der Ansicht ([+] bzw. View → Controls), in den Einstellungen („Model: Controls“) und in „Alle Aktionen suchen“.

## Muster: die Gruppen von Ansehen/Prüfen in der Form des Entwurfs (nur Form; Inhalt bringt AP5)

`gui/app_modelltab.cpp`: `rolloutModell`, `rolloutLod`, `rolloutAnbauten`, `rolloutPruefen`.
- **Model:** Kurzinfo „n triangles“; Werte als Tabelle (File, Animations (GLA), Surfaces, Triangles, Vertices, Bones), darunter
  „n surfaces selected“; Knöpfe im Raster mit Symbol: Other GLA…, Reload textures (Tastenfeld Alt+R nur wie 3ds Max), Photo…
- **Level of detail and skin:** Kurzinfo „LOD n“; Haken Auto, LOD 1-10 als Segment über die Breite (nur vorhandene wählbar), Zeile
  „Drawn: LOD x of y“, Picmip 0-3 als Segment hinter der Beschriftung, Skin-Zeile von AP1.
- **Attachments:** Kurzinfo = Zahl; je Anbau eine Zeile „saber_1.glm → *r_hand“ mit x (Detach), Knopf Detach all, Tipp als Kasten.
- **Check for the game:** Kurzinfo grün „all good“ bzw. rot „n findings“; Zielspiel als Formularzeile, Grenzen des Zielspiels als
  Tabelle (vorher ein Satz), Befunde als gelbe Absätze.
- Reiter: Model und LOD auch unter Data (Blender), Attachments unter Hierarchy bzw. Constraints, Check unter Utilities.

## Offener Punkt von UIFORM: Formularzeilen in `gui/app_modelltab_mb3.cpp` (Gruppe Karten-GLA)

Fehlerbild von „Into script“ (Beschriftung, `SameLine()` ohne feste Spalte, Feld bis zum Rand): nachgestellt und behoben mit
`ui::zeile`. Rot (Lauf `ap3_rot3`, alter Stand der drei Zeilen): Feld Ziel ab x 3101, Feld Karte ab x 3077
(24 Punkte Versatz - die Beschriftungen „Target“ und „Map“ sind ungleich lang). Grün: beide ab 3104 und bis 3733. Die Suche von
UIFORM (`Y:/werkzeuge/uif/muster.py`) nennt `gui/app_modelltab_mb3.cpp:771` auf der Basis und findet in diesem Zweig nichts mehr.

## Bewusst anders als Entwurf bzw. Vorschlag der Tabelle

- **Reiter filtern wirklich** (im Entwurf sind sie nur gezeichnet): im Reiter, den der Bereich vorgibt, stehen ALLE Gruppen des Bereichs
  wie im Entwurfsbild; die übrigen Reiter zeigen ihren Teil (Vorschlag der Tabelle zu F1: jede Gruppe gehört zu einem Reiter). So bleibt
  das Bild des Entwurfs erhalten und die Reiter haben trotzdem eine Wirkung. Frage an shank, ob jede Gruppe nur in einem Reiter stehen soll.
- **Reiter ohne Gruppe im Bereich sind gedämpft und nicht wählbar** (Entwurf: alle gleich gezeichnet) - sonst stünde ein leeres Feld da
  (shank 06.11.: keine leeren Bereiche).
- **Sechs Reiter wie 3ds Max** (Entwurf: fünf; shank 10.11. nennt „[, Dienstprogramme]“): Utilities trägt die Prüfung für das Spiel.
- **Stil wie 3ds Max mit +/−** statt des Pfeils im Entwurfsbild (shank 10.11.: „flache Überschriftsbalken mit +/−“); wie Blender mit Pfeil
  und abgerundeten Kästen. Titel nicht fett: das Programm hat keinen fetten Schriftschnitt.
- **Bereichsreihe (R2)** beginnt wie im Entwurf rechts neben der Modusleiste, steht aber ÜBER den Kopfzeilen der Spalten („Scene
  Explorer“, Werkzeugleiste der Ansicht); die Modusleiste beginnt deshalb eine Zeile tiefer als die Reihe. Die Modusleiste bis ganz nach
  oben zu ziehen, hieße die Spaltentabelle aller Tabs umzubauen (`app.cpp`, drei Anordnungen) - nicht in diesem Paket.
- **Hinweis (R3):** drei Texte je Bedienmodus (Ansehen/Prüfen, bearbeitende Bereiche, Animation) statt zwei im Entwurf, nur mit Tasten, die es
  gibt (im Entwurf: Alt+Z, Umschalt+Alt+Z, Alt+X - fehlen, siehe P9/P10/R14); dahinter die Mausnavigation von MBNAV. In sehr kleinen
  Fenstern entfällt der Hinweis (die Tasten stehen in der Statuszeile).
- **Breite (F13):** 22 Zeichenhöhen, in kleinen Fenstern höchstens 42 % des Platzes von Ansichten und Feld (bei 1328 Punkten Fensterbreite
  blieben der Ansicht sonst 8 Zeichenhöhen).
- **Detailstufe:** „Auto“ (gibt es im Entwurf nicht, das Programm kann mehr) ist ein Haken über der Reihe 1-10; mit Auto ist in der Reihe keine
  Stufe gewählt, ein Klick stellt sie fest.
- **Modell:** die Zahlen (Flächen, Dreiecke, Ecken, Knochen) stehen als Tabellenzeilen (Entwurf: nur Datei und GLA, Dreiecke in der Kurzinfo);
  „Importieren“ und „In neuem Tab öffnen“ bringen AP5/AP11.
- **Prüfung:** die Grenzen als Tabelle statt Balken je Fläche - die Balken der gewählten Fläche (`ui::grenzBalken` ist da) gehören zum Inhalt (AP5, P6).

## Selbsttest (rot -> grün)

`gui/selbsttest_ap3.inc`, 104 Prüfungen in drei Teilen (`BHED_AP3_TEIL`):
- **baustein** (34): ein Probefenster mit allen Bausteinen, je Stil - Gruppe im Element-Register (Openable/Opened), Schrittnummer
  und Kurzinfo rechts im Balken, Kasten um den Inhalt, Stil (Max: Rundung höchstens 2 Punkte; Blender: mindestens 0,3 Zeichenhöhen),
  Hauptknopf über die Breite und höher, Raster mit zwei gleich breiten Spalten und gleich hohen Knöpfen je Reihe (auch wenn einer
  umbricht), Felder und Schieber an einer Kante, Beschriftung links (Max) bzw. rechtsbündig 0,6 Zeichenhöhen vor dem Feld (Blender),
  Schalterliste in zwei Spalten, Segment 1-10, Zeilen von Werten/Balken/Tipp im Element-Register, Klicks (Hauptknopf, Raster,
  gesperrter Knopf, Schalter, Segment, x der Listenzeile, Zuklappen), Feld mit Reitern (Reihe bzw. senkrechte Spalte, belegte
  Reiter, Filter je Reiter, leerer Reiter nicht wählbar), schmales Fenster (11 Zeichenhöhen: eine Spalte, nichts ragt hinaus).
  Neuer Code, daher kein Rot gegen den alten Stand; der erste Lauf fand im Test (nicht im Baustein) fünf Fehler (Ausklapper meldet
  seine Beschriftung nur aufgeklappt; Probefenster für beide Stile dasselbe; Abfrage in einem Bild ohne Probefenster).
- **rahmen** (36): F1, F2, F9, F10, F13, R2, R3, R4 und der Punkt von UIFORM, je Bedienmodus. **Rot** (Lauf `ap3_rot1`: Baustein und
  Rahmenfunktionen gebaut, das Programm zeichnete noch das alte Feld): alle 35 Zeilen rot (F1 8, F2 9, F9/F10 je 1, F13 4, R2 6, R3 2,
  R4 4), dazu zwei Klicks „nicht gefunden“. UIFORM gültig rot erst im Lauf `ap3_rot3` (Feld Ziel ab 3101, Feld Karte ab 3077).
- **muster** (34): F3, F5, F8, F11, F12, F16, F9/F10 an den Gruppen, Reiter je Gruppe (F1/F2). **Rot** (Lauf `ap3_rot4`, Rahmen da,
  Gruppen noch alt): 29 rot, 5 „OK“ - davon vier zu F12 („Klick auf 1 stellt die Stufe fest“, „Auto stellt zurück“), die schon der
  alte Stand erfüllte; sie prüfen seitdem auch den Zustand des Segments und des Hakens (dieser schärfere Stand lief nicht eigens rot).
- **Grün** (Lauf `ap3g3_mb_ap3`, Endstand des Schnittstellen-Commits): 105 OK, 0 FEHLER (104 von AP3).
- Dazwischen gefunden und behoben: mit „Auto“ zeigte das Segment die gezeichnete Stufe als gewählt - ein Klick darauf stellte nichts
  fest (jetzt ist mit Auto keine Stufe gewählt); F13-Test rechnete die Zugstrecke mit der Schrift vor dem Vergrößern (72 statt 108
  Punkte); ein Wert ohne Leerzeichen (GLA-Pfad) wurde mitten im Wort umbrochen (jetzt eigene Zeile darunter); im kleinen Fenster
  nahm das Feld der Ansicht alles bis auf 8 Zeichenhöhen (jetzt höchstens 42 %).

## Angepasste vorhandene Selbsttests (nichts gelöscht)

- `selbsttest_mb1a1.inc`: der Bedienmodus wird über die Statuszeile umgeschaltet (`##mzmodus`, dann der Menüpunkt) statt über die
  entfallenen Knöpfe im rechten Feld.
- `selbsttest_mbkopf.inc`: „das rechte Feld hat keine Gruppe Anzeige mehr“ sucht unter den Gruppen (`mtrollouts`) - „Display“ heißt
  seither auch ein Reiter des Befehlsfelds.
- `selbsttest_mbraster.inc` (in der Schlussrunde rot, 39 OK / 4 FEHLER): „Rechtsklick > Anzeige > Raster“ suchte „Display“ ohne
  Fenster und fand den Reiter statt des Untermenüs - jetzt im Menü gesucht (`rig3::klickMenue`). Danach 41 OK / 0.
- `selbsttest_mb5.inc` (in der Schlussrunde rot, 159 OK / 1 FEHLER, auch in der Wiederholung): „Rechtsklick der Ansicht: Untermenü
  Animationswerkzeuge“ klickte fest auf 85 % / 20 % der Ansicht; im Testfenster (2514 Punkte breit) liegt dieser Punkt auf dem
  ViewCube, seit das Feld 5 Zeichenhöhen breiter ist (Protokoll: unter der Maus `##mkwuerfel`). Der Punkt weicht Kopfzeile und
  ViewCube aus (`modellKopfTreffer`). Danach 160 OK / 0.
- Für Welle 2 und die Zusammenführung: beide Fallen (Reiternamen im Element-Register, schmalere Ansicht) stehen in
  `Y:\werkzeuge\ap3\SCHNITTSTELLE.md`. Gleichlautende Texte geprüft (`Y:\werkzeuge\ap3\gleiche_texte.py`): außer den genannten sucht
  kein Selbsttest einen Text ohne Fenster, der wie ein Reiter heißt.

## Fotos

Angesehen (Foto-Selbsttest `ap3foto`, 3818 x 2104 bei Oberfläche 1,5; Ausschnitte des rechten Felds neben den Entwurfsbildern
`Y:\werkzeuge\mbe\fotos\entwurf\e_max_pruefen_lang.png` / `e_blender_pruefen_lang.png`, Vergleichsbilder
`Y:\werkzeuge\ap3\fotos\vergleich\v_<modus>_<bereich>.png`):
- Probefenster des Bausteins in beiden Stilen, mit Reitern, schmal (`ap3_baustein_*`).
- wie 3ds Max und wie Blender: Ansehen/Prüfen (oben, nach unten gerollt, Reiter Hierarchy / Utilities bzw. Constraints / Data) und
  die übrigen fünf Bereiche im neuen Rahmen (ihre Gruppen noch in alter Form).
- helles Schema (`behaved-classic`: eckig, dunkelblaue Betonung, Grün/Rot der Kurzinfos lesbar), Deutsch, Chinesisch, Japanisch,
  Spanisch, Tschechisch (nichts abgeschnitten; lange Knopftexte brechen um, die Reiter in eine zweite Reihe), kleines Fenster
  1328 x 844 bei Oberfläche 1,0 (Bereichsreihe passt, der Hinweis daneben entfällt, Feld 42 % des Platzes).
- Ablage: `Y:\werkzeuge\ap3\fotos\ap3_f_<variante>\*.png`, Übersichten `sicht_<variante>.png`, `sicht_en_final.png`.
- Gegen den Entwurf bleibt sichtbar anders: +/− statt Pfeil im Max-Stil, Titel nicht fett, sechs statt fünf Reiter, gedämpfte Reiter,
  der Haken „Auto“, die Zahlenzeilen in „Model“, die Prüfung ohne Balken (siehe „Bewusst anders“).

## Handbuch

- Eigene Quelle `tools/handbuch/inhalt_ap3.py` (6 Sprachen), eingehängt in die Seite „Model-Tab: Ansehen und Prüfen“ direkt hinter die
  Überschrift „Befehlsfeld“: das Feld je Bedienmodus mit seinen Reitern, die Form der Gruppen, Breite / Bereichsreihe / Schalter des
  Bedienmodus; Bilder `h_ap3_max`, `h_ap3_blender` in 6 Sprachen (2538 x 1472, angesehen: en wie 3ds Max, ja wie Blender).
- Berichtigt: `inhalt_mb1a1.py` nannte den Bedienmodus wie Blender noch als „kommt in Stufe 1a-2“ - jetzt steht dort, wo man umschaltet.
  Denselben überholten Satz zeigte bis rc611 der Tooltip des Knopfs „like Blender“ im rechten Feld (Text `MbWieBlenderHint`, entfällt).
- `gen_handbuch.py --pruefen`: 0 Texte / 0 Bilder noch englisch.
- **Nicht neu aufgenommen** (AP14 bzw. Zusammenführung): alle übrigen Bilder des Model-Tabs zeigen rechts noch das alte Feld (ohne
  Reiter, 17 Zeichenhöhen breit) und die Bereichsreiter in der Ansicht (z. B. `h_mb1_ansehen`, `h_ap1_max`, `h_ap2_max`, `h_mbkopf_*`).

## Schlussrunde

Nach der Regel „Schlussrunde je Zweig ab 11.11.“: Bau, ctest, der eigene Selbsttest und die Gruppen, die den geänderten Code berühren;
die volle Runde (Neubau, alle Gruppen, Modus 1) läuft einmal auf dem zusammengeführten Stand.

| Schritt | Ergebnis |
|---|---|
| Bau, alle Ziele (inkrementell, /W4 /WX) | 0 Fehler, 0 Warnungen (`bau_alles.log`, nach den zwei Testanpassungen `bau_schluss.log`) |
| ctest (254), 8 gleichzeitig, Rechner bei 100 % Last | 251 bestanden; `q3map2_teilbau_bounce`, `_geometrie`, `_offen` endeten ohne Ausgabe an ihrer Zeitgrenze von 1800 s (kein Fehlschlag; der Zweig ändert nichts an `src/q3map2`, `tests`). Mit demselben Befehl ohne Zeitgrenze wiederholt: alle drei exit 0, „0 Fehlschläge“ (747 s, 840 s, 887 s; `teilbau.txt`, `teilbau_<teil>.out`). Darin `handbuch_sprachen` und die sechs Lint-Tests grün. |
| Lints der geänderten Dateien (Endstand, `lint2_*.txt`) | sources, cjkumbruch, i18n, i18n_erzeugt, gapp, settings, abstand, breitenabzug, hoehe, tabellenid, imgui_alpha, feldinit, resize, doppelanzeige, endtable: ok; pruefcodes 11 Beanstandungen wie die Basis |
| eigener Selbsttest `ap3` (endgültige exe) | 105 OK, 0 FEHLER |
| modellblock, 39 Gruppen grün | funktionen 62 · tasten 44 · mb2 38 · blender 68 · haut 2 · mb3 22 · rig 27 · uv 51 · uvmd3 4 · md 87 · mdkarte 8 · mb7 25 · mb7spiel 5 · mb7textur 2 · ph 30 · rt 20 · rtjka 2 · pr 16 · pt 42 · ptfix 6 · ptfix2 6 (ja) · ptfix3 13 · ptstufen 5 · ptsprung 4 · ai 32 · pfix 18 · pfix2 6 · pfixkarte 6 · bild_jka 2 · bild2_jka 3 · kopfrot 26 · kopf 116 · raster* 41 · nav 133 · mbv 216 · ap1 137 · ap2 64 · ap12 35 · mb5* 160 - je 0 FEHLER (* nach der Testanpassung, siehe oben) |
| modellblock `mb6` | 37 OK, **1 FEHLER** „MB6 Pfad Hand gezogen: der Punkt steht bei +8 unter der Maus“ - Stand der Basis: im Zusammenführungs-Worktree `p_zus` (ohne AP3) in der vollen Runde `tr_` und der Wiederholung `w_` dieselbe Prüfung rot (37 OK / 1 FEHLER, der Punkt bleibt 30/−25 neben der Maus). Nicht von AP3, nicht angefasst. |
| `modell`, `icons` | 15 OK / 0, 1 OK / 0 (acht neue Symbole) |
| Anordnungen (Bereichsreihe in `bereichZeichnen`) | dock1 99 / 0 · dock2mass 30 / 0 · dock4 116 / 0 · tabfenster 22 / 0 |
| `uiform` (Formular-Baustein, Gruppe Karten-GLA) | 114 OK / 0 |
| `karteneditor`, `BHED_KARTE_NUR=rdcjk`, Japanisch (Hinweis im rechten Feld umbrochen) | 14 OK / 0 (Breite 483, zwei Zeilen, leerste 0,96) |
| Foto-Selbsttest `ap3foto` | 8 Varianten (en, de, zh-Hans, ja, es, cs, hell, klein), alle exit 0 / 0 FEHLER |
| Handbuch | `gen_handbuch.py --pruefen`: 6 Sprachen, 0 Texte und 0 Bilder noch englisch |

Läufe: `Y:\p_ap3\build\laeufe\ap3s_summe.txt` (48 Läufe, vor der Testanpassung: 3 rot - mb5, mb6, raster), `ap3t_summe.txt` (mb5, raster,
ap3 danach: 0 rot). Die übrigen 44 grünen Läufe der Runde stammen vom Bau vor der Testanpassung - derselbe Programmcode, geändert sind
nur die zwei Testschritte.

Bewusst NICHT gelaufen (einmal auf dem zusammengeführten Stand): Modus 1, karteneditor (voll), menueeditor, handbuch (voll; nur
`BHED_HB_NUR=ap3` in sechs Sprachen), die Rundläufe je Spiel (uvrl, rl, rrl, kr, irl) und bild/bild2 für md und jk2, die Vorschau-Gruppen,
der saubere Neubau.

Aufgefallen, nicht von AP3: `dock4` und `tabfenster` schreiben nach der Zählung „Beenden: Programm läuft nach "No" weiter (offene Frage
im Map Editor: "Exit Radiant")“ - steht genauso im Release-Lauf `Y:\karte\laeufe\rel611_dock4`.

### Auf karte1 gesetzt (12.11., Opus)

Die vier Commits auf karte1 `a3381725f` neu aufgesetzt (Basis vorher `852d12be7`). Konflikte nur in Listen: `tools/gen_i18n.py`,
`tools/i18n_sprache_es.py`, `tools/i18n_sprache_cs.py`, `tools/handbuch/gen_handbuch.py` (GDPFAD und AP3 je ein Block am Ende, beide
behalten); `include/bhed/i18n.h`, `src/i18n.cpp`, `handbuch/*/suche.js` mit den Generatoren neu erzeugt (6056 Texte; `gen_hilfe_orte.py`
ohne Unterschied).

| Schritt | Ergebnis |
|---|---|
| Bau, alle Ziele (inkrementell, /W4 /WX) | 0 Fehler, 0 Warnungen (`Y:\werkzeuge\ap3\bau_karte1.log`) |
| ctest, 12 gleichzeitig | 256 / 256 bestanden (205 s) |
| Gruppen, die den Model-Tab berühren (`ap3k_summe.txt`) | ap3 105 · kopf 116 · kopfrot 26 · nav 133 · ap2 64 · ap1 137 · mb5 160 · raster 41 · funktionen 62 · tasten 44 · blender 68 · uiform 114 - je 0 FEHLER |
| Lints | i18n, sources, fassung ok; `gen_handbuch.py --pruefen` ok |

## Offen

- Die Gruppen der Bereiche Modellieren, UV, Weights, Painting, Animation zeichnen noch mit `ImGui::CollapsingHeader` und Knöpfen in
  Textbreite (AP6-AP10); sie stehen im Rahmen im vorgegebenen Reiter. Nummerierte Schritte (F4), Hauptknopf (F6), Regler (F7) und
  Schalterlisten (F12) sind im Baustein fertig und im Probefenster geprüft, im Programm aber noch in keiner echten Gruppe.
- Ort des Felds im Modus wie Blender (Outliner darüber, Werkzeugleiste, N-Seitenleiste): AP4.
- Inhalt von Ansehen/Prüfen (Boden und Kamera, Szene, Balken der Prüfung, Skins zweispaltig, LOD über der Zahl wählbar): AP5.
- Ob eine Gruppe auf- oder zugeklappt ist, merkt das Programm wie bisher nur bis zum Beenden.
- Der Hinweiskasten von MBKOPF („display options are now at the top of the view“) hat noch seine alte Form.

## Changelog

## English

- Model tab, right panel: it now follows the control mode in its look. Like 3ds Max it is the command panel with the tabs Create, Modify, Hierarchy, Motion, Display and Utilities; like Blender it is the properties editor with a vertical column of symbols (Tool, Object, Modifiers, Data, Bone, Constraints, Material). Every area opens its tab and shows all its groups there; the other tabs show their part.
- Groups in one form everywhere: a heading that folds, with a short info on the right (triangles, drawn LOD, number of attached models, findings of the check), buttons in a grid of two equal columns with symbol and key, values as a table, limits as bars, hints in a box. In View/Check the groups Model, Level of detail and skin, Attachments and Check for the game already use it.
- The panel is wider (22 instead of 17 character heights) and its width can be dragged at the edge to the views; a double-click restores the default.
- The areas View/Check, Modeling, UV, Weights, Painting and Animation stand as one row of equally wide fields across the whole tab, with a short hint for the control mode and the area next to them.
- The switch like 3ds Max / like Blender has moved out of the panel: it is in the status line of the tab, in the menu of the header of a view, in the settings and in the search for all actions.
- Animation, group for the map GLA: the fields Target, Map and Name now start at the same edge.

## Deutsch

- Model-Tab, rechtes Feld: es folgt jetzt auch im Aussehen dem Bedienmodus. Wie 3ds Max ist es das Befehlsfeld mit den Reitern Erstellen, Ändern, Hierarchie, Bewegung, Anzeige und Dienstprogramme; wie Blender der Eigenschaften-Editor mit einer senkrechten Spalte aus Symbolen (Werkzeug, Objekt, Modifier, Daten, Knochen, Constraints, Material). Jeder Bereich öffnet seinen Reiter und zeigt dort alle seine Gruppen; die anderen Reiter zeigen ihren Teil.
- Gruppen überall in einer Form: eine Überschrift zum Zuklappen mit Kurzinfo rechts (Dreiecke, gezeichnete LOD, Zahl der Anbauten, Befunde der Prüfung), Knöpfe im Raster aus zwei gleich breiten Spalten mit Symbol und Taste, Werte als Tabelle, Grenzen als Balken, Hinweise im Kasten. In Ansehen/Prüfen nutzen die Gruppen Modell, Detailstufe und Skin, Anbauten und Prüfen fürs Spiel sie schon.
- Das Feld ist breiter (22 statt 17 Zeichenhöhen), und seine Breite lässt sich an der Kante zu den Ansichten ziehen; ein Doppelklick stellt die Vorgabe wieder her.
- Die Bereiche Ansehen/Prüfen, Modellieren, UV, Weights, Painting und Animation stehen als eine Reihe gleich breiter Felder über dem ganzen Tab, daneben ein kurzer Hinweis für Bedienmodus und Bereich.
- Der Schalter wie 3ds Max / wie Blender ist aus dem Feld gewandert: er steht in der Statuszeile des Tabs, im Menü der Kopfzeile einer Ansicht, in den Einstellungen und in der Suche nach allen Aktionen.
- Animation, Gruppe für die Karten-GLA: die Felder Ziel, Karte und Name beginnen jetzt an derselben Kante.

## 中文

- 模型选项卡,右侧面板:现在外观也随操作方式而变。类似 3ds Max 时是带有“创建、修改、层次、运动、显示、实用程序”选项卡的命令面板;类似 Blender 时是属性编辑器,左侧有一列竖排图标(工具、物体、修改器、数据、骨骼、约束、材质)。每个区域会打开自己的选项卡并在其中显示全部分组;其他选项卡显示各自的部分。
- 所有分组采用同一种形式:可折叠的标题,右侧有简要信息(三角形数、已绘制的 LOD、挂接模型数、检查结果),按钮排成两列等宽的网格并带图标和按键,数值以表格显示,限制以条形显示,提示放在方框里。在“查看/检查”中,模型、细节级别和皮肤、附加物、针对游戏检查这几个分组已经采用这种形式。
- 面板更宽(22 个字符高度,原为 17),并可在与视图相邻的边缘拖动调整宽度;双击恢复默认值。
- “查看/检查、建模、UV、权重、绘制、动画”这些区域作为一行等宽的格子横跨整个选项卡,旁边是针对操作方式和区域的简短提示。
- “类似 3ds Max / 类似 Blender”的开关已移出面板:它位于选项卡的状态栏、视图标题的菜单、设置以及“搜索所有操作”中。
- 动画,地图 GLA 分组:“目标、地图、名称”三个字段现在从同一条边开始。

## 日本語

- モデルタブの右パネル: 見た目も操作モードに従うようになりました。3ds Max 風では「作成・修正・階層・モーション・表示・ユーティリティ」のタブを持つコマンドパネル、Blender 風では縦一列のアイコン(ツール、オブジェクト、モディファイアー、データ、ボーン、コンストレイント、マテリアル)を持つプロパティエディターです。各エリアは自分のタブを開いて全グループを表示し、ほかのタブにはそれぞれの担当分が表示されます。
- グループの形をすべて統一: 折りたためる見出しと右側の短い情報(三角形数、描画中の LOD、取り付けたモデルの数、チェックの結果)、アイコンとキー付きの同じ幅 2 列のボタングリッド、表形式の値、バー表示の制限、枠付きのヒント。「表示/チェック」ではモデル、詳細レベルとスキン、取り付け、ゲーム向けチェックの各グループがすでにこの形です。
- パネルが広くなり(文字の高さ 17 個分から 22 個分へ)、ビューとの境目をドラッグして幅を変えられます。ダブルクリックで既定に戻ります。
- 「表示/チェック、モデリング、UV、ウェイト、ペイント、アニメーション」の各エリアは、同じ幅のフィールドが並ぶ 1 行としてタブ全体の幅に表示され、その横に操作モードとエリアに応じた短いヒントが出ます。
- 「3ds Max 風 / Blender 風」の切り替えはパネルの外に移りました。タブのステータス行、ビューのヘッダーのメニュー、設定、すべてのアクションの検索にあります。
- アニメーション、マップ GLA のグループ: 「出力先・マップ・名前」のフィールドが同じ位置から始まるようになりました。

## Español

- Pestaña Model, panel derecho: ahora también sigue el modo de control en su aspecto. Como 3ds Max es el panel de comandos con las pestañas Crear, Modificar, Jerarquía, Movimiento, Presentación y Utilidades; como Blender es el editor de propiedades con una columna vertical de símbolos (Herramienta, Objeto, Modificadores, Datos, Hueso, Restricciones, Material). Cada área abre su pestaña y muestra allí todos sus grupos; las demás pestañas muestran su parte.
- Grupos con una misma forma en todas partes: un título que se pliega con un dato breve a la derecha (triángulos, LOD dibujado, número de modelos acoplados, avisos de la comprobación), botones en una cuadrícula de dos columnas iguales con símbolo y tecla, valores en tabla, límites como barras e indicaciones en un recuadro. En Ver/Comprobar ya la usan los grupos Modelo, Nivel de detalle y skin, Acoplados y Comprobar para el juego.
- El panel es más ancho (22 en lugar de 17 alturas de carácter) y su ancho se puede arrastrar en el borde con las vistas; un doble clic restablece el valor predeterminado.
- Las áreas Ver/Comprobar, Modelado, UV, Pesos, Pintura y Animación forman una fila de campos de igual ancho a lo largo de toda la pestaña, con una indicación breve para el modo de control y el área al lado.
- El conmutador como 3ds Max / como Blender ha salido del panel: está en la línea de estado de la pestaña, en el menú de la cabecera de una vista, en los ajustes y en la búsqueda de todas las acciones.
- Animación, grupo del GLA de mapa: los campos Destino, Mapa y Nombre empiezan ahora en el mismo borde.

## Čeština

- Karta Model, pravý panel: nyní se režimem ovládání řídí i vzhledem. Jako 3ds Max je to panel příkazů s kartami Vytvořit, Upravit, Hierarchie, Pohyb, Zobrazení a Nástroje; jako Blender je to editor vlastností se svislým sloupcem symbolů (Nástroj, Objekt, Modifikátory, Data, Kost, Vazby, Materiál). Každá oblast otevře svou kartu a ukáže tam všechny své skupiny; ostatní karty ukazují svou část.
- Skupiny všude ve stejné podobě: nadpis, který lze sbalit, s krátkým údajem vpravo (trojúhelníky, vykreslené LOD, počet připojených modelů, nálezy kontroly), tlačítka v mřížce dvou stejně širokých sloupců se symbolem a klávesou, hodnoty v tabulce, limity jako pruhy, rady v rámečku. V oblasti Prohlížet/Kontrola ji už používají skupiny Model, Úroveň detailu a skin, Připojené a Kontrola pro hru.
- Panel je širší (22 místo 17 výšek znaku) a jeho šířku lze táhnout za hranu u pohledů; dvojklik obnoví výchozí hodnotu.
- Oblasti Prohlížet/Kontrola, Modelování, UV, Váhy, Malování a Animace stojí jako jedna řada stejně širokých polí přes celou kartu, vedle nich krátká nápověda pro režim ovládání a oblast.
- Přepínač jako 3ds Max / jako Blender se z panelu přesunul: je ve stavovém řádku karty, v nabídce záhlaví pohledu, v nastavení a v hledání všech akcí.
- Animace, skupina pro GLA mapy: pole Cíl, Mapa a Jméno teď začínají na stejné hraně.
