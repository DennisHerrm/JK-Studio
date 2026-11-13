# Notizen AP14 (12./13.11.): Handbuch des Model-Tabs zusammengeführt, Reste von AP3, Entscheidung B6 - rc612

Auftrag AP14 aus `auftraege/welle_1211.md` Abschnitt 5 (Fable 5.2, nach dem Fable-Limit fortgesetzt von Opus 5.5). Worktree `Y:\p_ap5`,
Zweig `p_ap14`, begonnen auf karte1 `73c18113f`, rebased auf den jeweils aktuellen karte1. Selbsttest neu `gui/selbsttest_ap14.inc`
(`BHED_EDITORTEST=modellblock`, `BHED_MODELL_NUR=ap14 | ap14h | ap14foto`; Handbuch `BHED_EDITORTEST=handbuch`, `BHED_HB_NUR=ap14`), angepasst `ap8` (W2) und `ap10` (A6/A7). Skripte, Läufe,
Fotos: `Y:\werkzeuge\ap14\`.

## Was sich ändert

| Punkt | Was | Datei |
|---|---|---|
| 1 Handbuch-Register | Jede Handbuchquelle des Model-Tabs steht mit **einer Zeile** in `tools/handbuch/inhalt_modell.py` (`QUELLEN`): `Seite("inhalt_x")` für eine eigene Seite (die Stelle in der Liste ist die Stelle im Handbuch), `Anhang("inhalt_x", "<seiten-id>")` für Bausteine an einer vorhandenen Seite, `Teil(...)` nur für ES/CS. `inhalt.py`, `inhalt_es.py`, `inhalt_cs.py` holen alles von dort; `gen_handbuch.py` lädt die Oberflächentexte aus allem, was `tools/gen_i18n.py` in seine Liste nimmt (eine neue `i18n_<kürzel>.py` braucht im Handbuch keine Zeile mehr). Reihenfolge: Rahmen (Überblick, Bedienung, Aufbau wie Blender, Zeitleiste, Dateien), dann die Bereiche wie die Bereichsleiste, zuletzt Animieren im Mission-Tab | `tools/handbuch/inhalt_modell.py`, `inhalt.py`, `inhalt_es.py`, `inhalt_cs.py`, `gen_handbuch.py` |
| 1 Seiten eingehängt | modell-blender (AP4), modell-pruefen (AP5), AP6 → modell-modellieren, AP7 → modell-uv, modell-weights (AP8, ersetzt die Seite von mb1a2), AP9 → modell-painting, modell-animationsfeld (AP10), modell-autoskin (AP12). `inhalt_ap8.py` und `inhalt_ap12.py` lieferten Tupel statt `T(...)` und waren nie eingehängt - behoben | `inhalt_ap*.py` |
| 1 Texte zusammengeführt | modell-ansehen ist der Überblick (eigener Titel, AP5 trug denselben); Tasten wie 3ds Max → modell-bedienung; Symbole/Auge → Abschnitt Explorer; Gruppen und Prüfung → modell-pruefen; Blender-Tasten des Malens → modell-weights. Berichtigt: Ort des Bedienmodus-Schalters, Dopesheet als Spuren der Zeitleiste, Gruppe „Übertragen“ statt Rollout Retargeting, Zeitkonfiguration, Balken der JKA-Grenzen, IK/FK-Zeilen, „Ins Skript“ (Doppelklick öffnet das Fenster), Navigation auf „Modellansicht und Figuren“, Auto-Skin als Gruppe im Feld. Kopfkommentare der Quellen `inhalt_ap1..ap12` nennen jetzt ihre Registerzeile statt „AP14 hängt ein“ | `inhalt_mb1a1.py`, `inhalt_mb1a2.py`, `inhalt_mb2.py`, `inhalt_mb3b.py`, `inhalt_mb4.py`, `inhalt_md1d.py`, `inhalt_rt.py`, `inhalt_mission.py`, `inhalt_animimp.py`, `inhalt_ap*.py` |
| 1 Hilfe-Orte | `tools/gen_hilfe_orte.py` nimmt jede Gruppe (`ui::rolloutBeginn`) und jedes Element mit Beschriftung (`werkzeugKnopf`, `schalter`, `regler`) des rechten Felds auf: **234 neue Orte**, Kette Model-Tab > Bereich > Gruppe > Element. Vorbedingung: Modusleiste → Feld der Bereichsreihe (`gezeichnet:modell.bereich<n>`, neu gemeldet in `drawModellBereiche`; Messwert `modell.bereich<n>` in `tour_messwerte.cpp`) → Überschrift der Gruppe, solange das Element nicht im Feld steht (`zahl:modell.feld.<Kennung>`, ausgewertet in `tour.cpp`; mehrere Gruppen mit `;`, z. B. „Höchstens 4 je Ecke“ wie 3ds Max in Erweitert, wie Blender in Optionen) | `tools/gen_hilfe_orte.py`, `src/hilfe_orte.inc` (erzeugt), `gui/tour.cpp`, `gui/tour_messwerte.cpp`, `gui/app_modelltab_feld.cpp` |
| 1 Hilfe-Orte: zwei Fehler, gefunden mit H | (1) Die Vorbedingung `zahl:modell.feld.<Kennung>` stand mit Großbuchstaben (`MgVier`) - Namen von `zahl:` sind nur a-z, 0-9, '.', '_' (`tour::pruefungBekannt`, ctest tourtest „zahl:Gross abgewiesen“); die Stufe galt deshalb nie, die Tour zeigte endlos die Überschrift der Gruppe (Blender: Klick auf „Optionen“ klappte auf und zu). Jetzt schreibt der Generator die Kennung klein, `tour.cpp` löst sie ohne Groß-/Kleinschreibung auf (`kennungAusKlein`). (2) Die Tour rollte ein Ziel nur einmal je Schritt ins Bild; bei mehreren Stufen lag der Öffner der Gruppe (wie 3ds Max „Erweiterte Parameter“, ganz unten im Feld) außerhalb des Felds, der Klick traf die Zeitleiste. Jetzt rollt sie einmal je gezeigtem Ziel (`gerolltFuer`) | `tools/gen_hilfe_orte.py`, `src/hilfe_orte.inc`, `gui/tour.cpp` |
| 2 Überlagerungen | deutsch „Overlays“ → „Überlagerungen“ (wie Blenders deutsche Oberfläche): `MkBlOverlays`, `MkOrtOverlays`, `MkUmzugBl`, `MkOverlaysHint` (`i18n_mbkopf.py`), `PqOverlays` (`i18n_ap5.py`) und die deutschen Handbuchtexte (`inhalt_ap5.py`, `inhalt_mbkopf.py`, `inhalt_mbraster.py`). es/cs/zh/ja stimmten schon mit Blender. Die Hinweise in `i18n_mbraster.py`, die Blenders englischen Menüweg nennen, bleiben | `tools/i18n_mbkopf.py`, `tools/i18n_ap5.py`, `src/i18n.cpp` (erzeugt) |
| 2 Smear | „Alle Aktionen suchen“: jede Liste zeichnet ihre Zeilen über `modellAktionZeile(text, bereich)`; steht derselbe Text in mehreren Bereichen, trägt die Zeile rechts ihren Bereich („Smear - Weights“, „Smear - Painting“). Wie Blender stand Abspielen/Anhalten zweimal (Leertaste, Umschalt+Leertaste) - jetzt einmal | `gui/app_modelltab.cpp`, `app_modelltab_gewichte.cpp`, `app_modelltab_malen.cpp`, `app_modelltab_blender.cpp`, `app_modelltab_kopf.cpp`, `app_modelltab_zeit.cpp` und die übrigen `*AktionenSuche` |
| 3 wertTabelle | umbrochen wird nur zwischen Wörtern: jede Spalte bleibt so breit wie ihr breitestes Wort (CJK: Zeichenpaar); passen die breitesten Wörter beider Spalten nicht nebeneinander, steht der Wert unter der Beschriftung. Vorher „counterp / art“ (en), „Flächenübergrei / fend“ (de) | `gui/ui_rollout.cpp` (`wertZeile`) |
| 3 Knopfraster | eine Zelle ist mindestens so breit, wie das breiteste Wort ihrer Knöpfe und Schalter im letzten Bild brauchte; sonst nimmt das Raster weniger Spalten (deutsch „Schleifenschnitt“ im schmaleren Feld wie Blender). Der Bedarf hängt nicht von der Spaltenzahl ab, die Wahl schwingt nicht | `ui_rollout.cpp` (`rasterBeginn`, `rasterEnde`, `werkzeugKnopf`, `schalter`) |
| 3 Wunsch 1 Hauptknopf als Umschalter | `KnopfOpt::umschalter`: ein Hauptknopf, der aus ist, zeigt Rahmen und Symbol in der Betonungsfarbe auf dem Grund eines gewöhnlichen Knopfs und behält Platz und Höhe. „Gewichte malen“ benutzt es (vorher war der Knopf aus ein gewöhnlicher, niedrigerer Knopf, der Inhalt darunter sprang) | `ui_rollout.cpp`, `app_modelltab_gewichte.cpp` (`hauptKnopf`) |
| 3 Wunsch 2 gesperrt im Register | `Element::gesperrt` aus `ImGuiItemFlags_Disabled` beim ItemAdd - Selbsttests sehen gesperrte Elemente | `gui/elemente.cpp`, `elemente.h` |
| 3 Wunsch 3 Absatz im Register | `ui::absatz` meldet sich mit seinem Text (Kennung aus Gruppe und Zeilennummer) | `ui_rollout.cpp` |
| 3 Wunsch 4 `aktiv` | `ui::schalter` / `ui::regler` sperren mit `aktiv = false` selbst, der Tooltip erscheint trotzdem. Benutzt von „Höchstens 4 je Ecke (JKA)“ (fest an) und Toleranz (bei Topologie-Spiegeln) | `ui_rollout.cpp`, `app_modelltab_gewichte.cpp` |
| 3 Wunsch 5 | `stapelZeile` nicht gebaut: AP13S baut den Modifier-Stapel gerade selbst | - |
| 3 Wunsch 6 | `segment` mit `*wahl = -1` betont keinen Eintrag - so gelassen, jetzt im Kopf zugesichert und geprüft (U6) | `ui_rollout.h` |
| 4 Reiter bei Mindestgröße (Entscheidung B6) | wie 3ds Max stehen die Reiter des Befehlsfelds unter der Mindesthöhe des Entwurfs (50 Zeichenhöhen) nur als Symbole in **einer** Reihe (die Symbole, die AP3 schon je Reiter hatte; Größe wie Blenders Spalte, schmaler nur, wenn die Reihe sonst nicht passt), der Name steht im Tooltip - wie Blenders Eigenschaften-Reiter. Vorher Text in zwei (en) bis drei (de) Reihen. Darüber bleibt der Text wie im Entwurf. Handbuch: ein Absatz in `tools/handbuch/inhalt_ap14.py` hinter dem Absatz über das Feld (inhalt_ap3) | `gui/ui_rollout.h` (`FeldOpt::symbole`, `FeldProbe::reiterSymbole`, `reiterTooltip`), `gui/ui_rollout.cpp` (`feldBeginn`, `reiterKnopf`), `gui/app_modelltab_feld.cpp` |
| 4 Zeitleiste bei Mindestgröße | unter der Mindesthöhe des freigegebenen Entwurfs (`.app { min-height: 700px }` bei `font: 14px` = 50 Zeichenhöhen) behält die Zeitleiste den Anteil der Fensterhöhe, den sie bei dieser Höhe hat; der Spurbereich wird niedriger und rollt, nie unter 2,5 Zeichenhöhen (die kleinste Einstellung). Der Griff rechnet die Einstellung zurück und folgt der Maus. Über 50 Zeichenhöhen bleibt alles wie bisher | `gui/app_modelltab_zeit.cpp` (`spurHoehe`, `spurEinstellungFuer`, `Masse::fest`) |
| Register nach den Rebases | karte1 `133c47078` brachte AP13S, AP13K, FIX612C und LCFIX, `1c6172246` dazu FIX612B (`Hinter("inhalt_fix612b", "bloecke", "modell-weights", "inhalt_ap8", "sperren")` - FIX612B hatte seinen Abschnitt an die alte Seite modell-weights von inhalt_mb1a2 gehängt, die seit AP8 nicht mehr gilt). Eingetragen: `Anhang("inhalt_ap13s", "modell-modellieren")` (Abschnitt Modifier-Stapel), `Seite("inhalt_ap13k")` (modell-knochen hinter modell-animationsfeld), FIX612C mit der neuen Art `Hinter(quelle, funktion, seite, von, schlüssel)` - Blöcke hinter einen Absatz einer anderen Quelle (`bloecke_aufbau` hinter ap4 „seite“, `bloecke_animationsfeld` hinter ap10 „rig“). LCFIX gehört zum Karten-Editor (karte_teilbau) und bleibt, wie es ist. Hilfe-Orte: `app_modelltab_knochen.cpp` (Gruppe Knochen und Rig-Darstellung) neu; die Gruppe Modifier-Stapel (Titel je Bedienmodus aus einer Variablen) nimmt der Generator nicht auf - sie ist über „Modifier-Stapel zeigen“ in der Suche erreichbar | `tools/handbuch/inhalt_modell.py`, `tools/gen_hilfe_orte.py` |
| 5 Changelog | `docs/intern/NOTIZEN_mbentwurf_1211.md`: die Zeilen aller Pakete (AP1-AP12, ENT1, ENT2, AP14) in sechs Sprachen, nach Bereichen geordnet, erzeugt mit `Y:\werkzeuge\ap14\changelog_bauen.py` | `docs/intern/NOTIZEN_mbentwurf_1211.md` |

## Selbsttests (rot → grün)

Fenster 1328 × 844 bei Oberfläche 1,0 (die kleinste Größe, die das Programm zulässt; `BHED_AP14_FENSTER=1280x720@1.0` landet ebenfalls dort),
Schrift 24 Punkte, Anzeige 35,2 Zeichenhöhen hoch.

| Test | rot | grün |
|---|---|---|
| `ap14` en | Bau `1a8733a40` (Proben ohne Behebungen): 27 OK, **11 FEHLER** (W Modellieren, K Weights Blender, Z ×3, S ×3, U1, U2, U3) | 45 / 0 (`ap14_g5_en`, mit R und H); nach dem Rebase auf `fe0cbece7`: 45 / 0 (`ap14_g13_en`) |
| `ap14` de | 19 OK, **19 FEHLER** (W ×4, K ×6, Z ×3, S ×3, U1, U2, U3) | 45 / 0 (`ap14_g5_de`); nach dem Rebase: 45 / 0 (`ap14_g13_de`; R: Reiter-Reihe 37 Punkte hoch = 1,5 Zeichenhöhen statt 154 im Rot-Bau, Tooltip „Hierarchie“; Z: Gruppe Animation ganz im Feld, ab ihrem Kopf 266 Punkte sichtbar) |
| `ap14` ja | 27 OK, **11 FEHLER** (W Modellieren und Animation, Z ×3, …) | 45 / 0 (`ap14_g5_ja`) |
| `ap14` Mindestgröße 1280×720 angefordert | - | 39 / 0 (`ap14_gruen_1280`, vor R/H) |
| `ap14` R (B6) | Bau mit `o.symbole = false` (`exe_rr`, Läufe `ap14_rr_en/de`): **R ×2 rot** (Reiter in 2 bzw. 3 Reihen, kein Name im Tooltip), deutsch dazu Z „drei Zeilen“ rot | grün in `ap14_g5_*` |
| `ap14` H (Helfer) | mit den Orten von `1a8733a40` (`ap14_hrot_en`): **4 FEHLER** (kein Treffer, Tour bleibt im Tab Script). Mit den neuen Orten noch rot (`ap14_h1`..`h3`): 8 Klicks ohne Ziel - Blender klappte „Optionen“ auf und zu (Kennung groß), 3ds Max klickte in die Zeitleiste (Öffner außerhalb des Felds) | `ap14_h4` 6 / 0 (3 Klicks je Bedienmodus), dann in `ap14_g5_*` |
| ctest `hilfe` | auf dem Zweig (vor dem Rebase): **H2-Liste 40 / 48 = 83,3 %** (Schwelle 85 %); mit den Orten von karte1: 41 / 48 = 85,4 % - die rund 230 neuen Orte verschoben die Gewichte, zh „像在游戏里一样点击菜单按钮“ fiel auf Platz 4 | 42 / 48 = 87,5 % (Wörter, mit denen Leute danach fragen, am Knopf „Menü bedienen“ in `src/hilfe_daten.inc`; de 12/12, zh 9/12; keine neue Lücke gegenüber karte1) |

Messwerte Z (Animation, Spuren offen): vorher Zeitleiste 378 von 844 Punkten = **44,8 %** (ja 416 = 49,3 %), Spuren 192; erlaubt 266 = 31,5 %
(Anteil bei 50 Zeichenhöhen). Nachher 265 = **31,4 %** (ja 292 = 34,6 %, erlaubt 34,7 %), Spuren 79 (ja 68), Einstellung unverändert 192.
Das rechte Feld zeigt ab dem Kopf seiner ersten Gruppe vorher 76 (en) / 36 (de) / 50 (ja) Punkte; nach der Zeitleiste 189 / 149 / 163, mit den
Symbol-Reitern (B6) deutsch so viel wie englisch.

Angepasst, nichts abgeschwächt:
- `ap14` Z „rechtes Feld“: im Rot-Stand prüfte es „die erste Gruppe ganz“ (rot); Fable hat die Prüfung vor dem ersten Grün-Lauf auf
  „Kopf und drei Zeilen“ geändert (die Gruppe Animation ist mit 186 Punkten höher als das Feld bei Mindestgröße). Deutsch fehlte dafür
  **1 Punkt**, solange die Reiter als Text in drei Reihen standen - mit B6 grün, die Prüfung verlangt weiter drei Zeilen.
- `ap8` W2: nach dem Ausschalten bleibt „Gewichte malen“ der Hauptknopf der Gruppe (`hauptKnoepfe == 1` statt 0) - Wunsch 1.
- `ap10` A6/A7: „ohne Rig steht dort der Hinweis als Kasten“ suchte den Hinweis mit `imRoll` im ganzen Feld; seit `ui::absatz` im
  Element-Register steht (Wunsch 3), findet es zuerst denselben Hinweis als Absatz der Gruppe Rig weiter oben (rot in `ap14n_mb_ap10`:
  2 FEHLER). Jetzt `imRollIn(<Gruppe IK/FK>, …)` - die Prüfung meint ausdrücklich den Kasten in IK/FK.
- `ap14` H: nur die beiden Gruppen mit dem Schalter werden zugeklappt und nach der Tour geht es zurück in den Model-Tab - im Rot-Lauf
  liefen U1-U6 sonst im Tab Script bzw. mit zugeklappten Gruppen (Folgefehler, nicht gezählt).

Nachbargruppen (testrunde, 2 gleichzeitig, Leerlauf, Bau vor dem dritten Rebase): mb_funktionen 63/0, mb_tasten 45/0, mb_kopf 117/0,
mb_ap12 36/0, hilfe2 57/0, mb_ap3 106/0, mb_ap1 143/0, mb_ap6 95/0, mb_ap8 105/0, mb_ent2 23/0, mb_ap10 72/0, mb_ap4 106/0, mb_ap5 121/0,
mb_ap13s 123/0, mb_ap9 98/0, mb_ap7 89/0, mb_ap13k 66/0 (17 Läufe, 0 rot); fix612c 36/0, fix612b 39/0. Unter Volllast vorher rot und allein
grün: mb_ap12 (vier Tooltips erst im nächsten Schritt) und mb_ap8 W12 (bekannt, KLEIN Punkt 3). hilfe2 zählt grün; die eine Zeile
„Beenden: Programm läuft nach No weiter“ (Frage des Karten-Editors beim Beenden) steht im Abschluss des Laufs; testrunde wertet ihn grün, mit dem Model-Tab hat sie nichts zu tun. Nach dem Rebase auf `fe0cbece7`: fix612 27/0, HB-Schritt ap14 4/0, mb_funktionen 63/0, mb_tasten 45/0, mb_ap5 121/0; mb_mb2 38/1 („die Füße blau“) - auf karte1 bekannt und nicht von AP14: die Probe auf r_leg trifft die Knochen-Darstellung von AP13K (so in `docs/intern/NOTIZEN_fix612b_1211.md`, Tabelle der Gruppen; vor AP13K war mb_mb2 auf diesem Zweig 39/0). ctest 263 / 263.

## Handbuch

- `python tools/handbuch/gen_handbuch.py --pruefen`: grün (183 Seiten × 6 Sprachen, keine fehlenden Bilder, es/cs 0 Texte englisch). Das Handbuch im Repo (`handbuch/<sprache>/*.html`, `suche.js`) ist mit dem Generator neu erzeugt (neue Seiten modell-blender, modell-pruefen, modell-animationsfeld, modell-autoskin, modell-knochen; in jeder Seite ändert sich nur die Seitenleiste)
- Bilder `h_ap12_max`, `h_ap12_blender` (Seite modell-autoskin, Gruppe Auto-Skin im Feld Weights nach einem Lauf an kyle) in sechs Sprachen,
  `BHED_HB_NUR=ap14` (`Y:\werkzeuge\ap14\hb.sh`): je Sprache 4 / 0 (`ap14_hb_*`, auch „wie Blender steht der Hauptknopf der Gruppe Auto-Skin ganz im Feld“ - im ersten Bild wie Blender war nur der obere Teil der Gruppe zu sehen, jetzt rollt der Schritt die Gruppe nach oben). Angesehen: en wie 3ds Max, de wie Blender
- Alte Bilder des Model-Tabs, die neu aufgenommen werden müssen (nach AP11, Auftrag AP14b): `Y:\werkzeuge\ap14\bilder_neu.md`
  (Abschnitte A alt im Aufbau von rc611, B neuer Explorer mit altem Feld, C deutsch „Überlagerungen“, D aus den Notizen).
- Eine neue Handbuchquelle (AP13K, AP13S, DXR, KLEIN, Fix-Agenten) kommt mit **einer Zeile** in `QUELLEN` von `tools/handbuch/inhalt_modell.py`
  ins Handbuch, z. B. `Anhang("inhalt_ap13k", "modell-ansehen"),` oder `Seite("inhalt_ap13s"),` an der Stelle, an der die Seite stehen soll.
  Quellen außerhalb des Model-Tabs (DXR: Karten-Editor) hängen wie bisher in `inhalt.py`.

## Fotos

`Y:\werkzeuge\ap14\fotos\` (PNG aus den Läufen `ap14_f_*`, `ap14foto`: en Direct3D 12, en Direct3D 11 (`BHED_D3D12=0`), de, ja, helles Thema; je 0 FEHLER). Bei Mindestgröße 1328 × 844 - das ist der geprüfte Fall, darum unter 1920 breit; die Handbuchbilder sind 2538 × 1472. Angesehen: de wie 3ds Max, Animation mit offenen Spuren (sechs Symbol-Reiter in einer Reihe, Gruppe Animation mit Inhalt, Zeitleiste ein Drittel), en Weights „Malen aus“ unter Direct3D 12 und 11: höchstens 1 Farbstufe an einem Punkt verschieden, im Feld keiner.

## Schlussrunde des Zweigs

- Bau ohne Warnungen (alle Ziele, `--parallel 2`, /W4 /WX), nach jedem Rebase neu (`bau_g12`, `bau_g13`).
- Zuletzt rebased auf karte1 `1465cb6a3` (DXR; Konflikte nur in Handbuchdateien, erzeugte Dateien neu): Bau `bau_g14` 0 Warnungen,
  `ap14` en 45 / 0 (`ap14_g14_en`), ctest hilfe, tour, hilfe_orte, handbuch_sprachen, handbuch_suche grün.
- ctest 263 / 263 (nach dem Rebase auf `fe0cbece7`; vorher rot nur `hilfe`, siehe oben).
- Selbsttests: `ap14` en/de/ja, Fotos `ap14foto` (5 Varianten, Direct3D 12 und 11), Handbuchschritte `ap14` in 6 Sprachen; Nachbargruppen wie oben
  (Model-Tab-Gruppen, die `ui_rollout`, Suche, Zeitleiste, Element-Register und Tour berühren, dazu hilfe2, fix612/fix612b/fix612c).
  Bewusst nicht: Karten-Editor, Mission, Menü-Editor (kein Code dort geändert; `elemente.cpp` nur ein zusätzliches Feld, `tour.cpp` rollt je
  Ziel - geprüft durch hilfe2), Sprachläufe es/cs (Regel 11.11.).
- Lints: i18n, i18n_erzeugt, cjkumbruch, headers, imgui_context (eine Falschmeldung durch ein Wort im Kommentar - Kommentar umformuliert),
  imgui_alpha, layout, breitenabzug, hoehe, feldinit, doppelanzeige, sources, settings, abstand, gapp, resize, rueckkopplung: ok; portability
  101 wie karte1. `gen_handbuch.py --pruefen` ok, `gen_hilfe_orte.py --check` ok.

## Offen / Fragen an shank

- Entschieden (B6, `plaene/entscheidungen_1211.md`): Reiter bei Mindestgröße nur als Symbole - umgesetzt (Schwelle: Fensterhöhe unter 50
  Zeichenhöhen, wie die Zeitleiste). Bei einem breiten, aber niedrigen Fenster gilt das ebenso; ein schmales, hohes Feld behält den Text.
- Die Gruppe Modifier-Stapel (AP13S) hat keinen eigenen Hilfe-Ort: ihr Titel kommt je Bedienmodus aus einer Variablen, die der Generator nicht
  liest; erreichbar über „Modifier-Stapel zeigen“ in der Suche.
- mb_mb2 „die Füße blau“ ist auf karte1 rot (AP13K-Knochen über dem Bein, gemeldet von FIX612B) - nicht von AP14.
- Das Wort „Rollout“ steht noch in rund 25 älteren Absätzen (ph, rt, mb4, uv1b, md1d …); falsche Ortsangaben sind berichtigt, das Wort selbst nicht
  überall auf „Gruppe“ umgestellt (sechs Sprachen mit Beugung).
- Die Bilder aus `bilder_neu.md` nimmt AP14b nach AP11 auf.

## Changelog

### English

- Model tab: in a small window the timeline no longer takes almost half of the height - below 50 character heights of window height it keeps its share, the tracks get lower and scroll, and the panels above show more than their headings.
- Model tab, Search all actions: actions with the same name in several areas name their area on the right (Smear - Weights, Smear - Painting); like Blender, Play/Stop stands once.
- Model tab, right panel: value tables, buttons and switches in a narrow panel wrap only between words; a grid of buttons takes fewer columns when a word would not fit.
- Weights: the main button Paint weights keeps its place and height when painting is off (only its frame is highlighted then).
- The helper (What do you want to do?) knows every group, button, switch and slider of the right panel of the Model tab and leads there: Model tab, area, group, element.
- German: the menu Overlays like Blender is called Überlagerungen, as in Blender's German interface.
- Guide: the pages of the Model tab follow the order of the work - overview, controls, layout like Blender, timeline, files, then the areas View/Check, Modeling, UV, Weights, Auto-skin, Painting and Animation; new pages for the layout like Blender, View/Check, the Animation panel and Auto-skin.
- Model tab, right panel: in a small window the tabs of the command panel like 3ds Max stand as symbols in one row, with the name in the tooltip (like Blender's properties tabs) - as text they needed two or three rows there.

### Deutsch

- Model-Tab: in einem kleinen Fenster nimmt die Zeitleiste nicht mehr fast die halbe Höhe - unter 50 Zeichenhöhen Fensterhöhe behält sie ihren Anteil, die Spuren werden niedriger und rollen, und die Felder darüber zeigen mehr als ihre Überschriften.
- Model-Tab, Alle Aktionen suchen: Aktionen, die in mehreren Bereichen gleich heißen, nennen rechts ihren Bereich (Verwischen - Weights, Verwischen - Painting); wie Blender steht Abspielen/Anhalten nur einmal da.
- Model-Tab, rechtes Feld: Wertetabellen, Knöpfe und Schalter brechen in einem schmalen Feld nur zwischen Wörtern um; ein Knopfraster nimmt weniger Spalten, wenn ein Wort sonst nicht passt.
- Weights: der Hauptknopf Gewichte malen behält Platz und Höhe, auch wenn das Malen aus ist (dann ist nur sein Rahmen betont).
- Der Helfer (Was willst du tun?) kennt jede Gruppe, jeden Knopf, Schalter und Regler im rechten Feld des Model-Tabs und führt dorthin: Model-Tab, Bereich, Gruppe, Element.
- Deutsch: das Menü Overlays wie Blender heißt Überlagerungen, wie in Blenders deutscher Oberfläche.
- Handbuch: die Seiten des Model-Tabs stehen in der Reihenfolge der Arbeit - Überblick, Bedienung, Aufbau wie Blender, Zeitleiste, Dateien, dann die Bereiche Ansehen/Prüfen, Modellieren, UV, Weights, Auto-Skin, Painting und Animation; neue Seiten für den Aufbau wie Blender, Ansehen/Prüfen, das Feld der Animation und Auto-Skin.
- Model-Tab, rechtes Feld: in einem kleinen Fenster stehen die Reiter des Befehlsfelds wie 3ds Max als Symbole in einer Reihe, der Name im Tooltip (wie Blenders Eigenschaften-Reiter) - als Text brauchten sie dort zwei oder drei Reihen.

### 中文

- 模型选项卡: 在小窗口中时间轴不再占去近一半的高度 - 窗口高度低于 50 个字符高度时, 时间轴保持其所占比例, 轨道变矮并可滚动, 上方的面板不再只显示标题。
- 模型选项卡, 搜索所有操作: 在多个区域中同名的操作会在右侧标出所属区域 (涂抹 - Weights, 涂抹 - Painting); 与 Blender 相同时, 播放/停止只出现一次。
- 模型选项卡, 右侧面板: 数值表、按钮和开关在窄面板中只在词与词之间换行; 当某个词放不下时, 按钮网格会减少列数。
- Weights: 主按钮“绘制权重”在关闭绘制时保持位置和高度 (此时只突出显示边框)。
- 助手 (你想做什么?) 认识模型选项卡右侧面板的每个分组、按钮、开关和滑块, 并指引到那里: 模型选项卡、区域、分组、元素。
- 德语: 与 Blender 相同的 Overlays 菜单改称 Überlagerungen, 与 Blender 的德语界面一致。
- 手册: 模型选项卡的页面按工作顺序排列 - 概览、操作方式、与 Blender 相同的布局、时间轴、文件, 然后是查看/检查、建模、UV、Weights、自动蒙皮、Painting 和动画各区域; 新增与 Blender 相同的布局、查看/检查、动画面板和自动蒙皮页面。
- 模型选项卡, 右侧面板: 在小窗口中, 类似 3ds Max 的命令面板选项卡以图标排成一行, 名称显示在工具提示中 (与 Blender 的属性选项卡相同) - 以文字显示时需要两到三行。

### 日本語

- モデルタブ: 小さいウィンドウでタイムラインが高さの半分近くを占めなくなりました - ウィンドウの高さが 50 文字分より低いときはタイムラインがその割合を保ち、トラックは低くなってスクロールし、上のパネルは見出しより多くを表示します。
- モデルタブ、すべての操作を検索: 複数のエリアで同じ名前の操作は、右側にエリアを表示します (スミア - Weights、スミア - Painting)。Blender 風では再生/停止は 1 行だけです。
- モデルタブ、右側のパネル: 値の表、ボタン、スイッチは、狭いパネルでも単語の間でだけ折り返します。単語が収まらないときは、ボタンのグリッドの列が減ります。
- Weights: メインボタン「ウェイトペイント」は、ペイントがオフでも位置と高さを保ちます (オフのときは枠だけが強調されます)。
- ヘルパー (何をしたいですか?) は、モデルタブの右側のパネルのすべてのグループ、ボタン、スイッチ、スライダーを知っていて、そこへ案内します: モデルタブ、エリア、グループ、要素。
- ドイツ語: Blender 風のメニュー Overlays は、Blender のドイツ語版と同じ Überlagerungen になりました。
- ガイド: モデルタブのページを作業の順に並べました - 概要、操作、Blender 風のレイアウト、タイムライン、ファイル、続いて 表示/チェック、モデリング、UV、Weights、オートスキン、Painting、アニメーションの各エリア。Blender 風のレイアウト、表示/チェック、アニメーションのパネル、オートスキンのページを追加しました。
- モデルタブ、右側のパネル: 小さいウィンドウでは 3ds Max 風のコマンドパネルのタブをアイコンで 1 列に並べ、名前はツールチップに表示します (Blender のプロパティタブと同じ)。文字では 2 段か 3 段必要でした。

### Español

- Pestaña Model: en una ventana pequeña la línea de tiempo ya no ocupa casi la mitad de la altura: por debajo de 50 alturas de carácter de ventana conserva su proporción, las pistas se hacen más bajas y se desplazan, y los paneles de arriba muestran más que sus títulos.
- Pestaña Model, Buscar todas las acciones: las acciones que se llaman igual en varias áreas indican su área a la derecha (Difuminar - Weights, Difuminar - Painting); como en Blender, Reproducir/Detener aparece una sola vez.
- Pestaña Model, panel derecho: las tablas de valores, los botones y los interruptores de un panel estrecho solo se parten entre palabras; una cuadrícula de botones usa menos columnas cuando una palabra no cabría.
- Weights: el botón principal Pintar pesos conserva su sitio y su altura cuando la pintura está desactivada (entonces solo se resalta su marco).
- El asistente (¿Qué quieres hacer?) conoce cada grupo, botón, interruptor y deslizador del panel derecho de la pestaña Model y lleva hasta allí: pestaña Model, área, grupo, elemento.
- Alemán: el menú Overlays como en Blender se llama Überlagerungen, como en la interfaz alemana de Blender.
- Guía: las páginas de la pestaña Model siguen el orden del trabajo: vista general, controles, disposición como Blender, línea de tiempo, archivos y después las áreas Ver/Comprobar, Modelado, UV, Weights, Auto-skin, Painting y Animación; páginas nuevas para la disposición como Blender, Ver/Comprobar, el panel de Animación y Auto-skin.
- Pestaña Model, panel derecho: en una ventana pequeña las pestañas del panel de comandos como 3ds Max aparecen como símbolos en una sola fila, con el nombre en la información emergente (como las pestañas de propiedades de Blender); como texto necesitaban dos o tres filas.

### Čeština

- Karta Model: v malém okně už časová osa nezabírá skoro polovinu výšky - pod 50 výšek znaku výšky okna si drží svůj podíl, stopy se sníží a rolují a panely nad ní ukazují víc než své nadpisy.
- Karta Model, Hledat všechny akce: akce, které se ve více oblastech jmenují stejně, uvádějí vpravo svou oblast (Rozmazat - Weights, Rozmazat - Painting); jako v Blenderu je Přehrát/Zastavit jen jednou.
- Karta Model, pravý panel: tabulky hodnot, tlačítka a přepínače se v úzkém panelu zalamují jen mezi slovy; mřížka tlačítek použije méně sloupců, když by se slovo nevešlo.
- Weights: hlavní tlačítko Malovat váhy si drží místo i výšku, i když je malování vypnuté (zvýrazněný je pak jen jeho rámeček).
- Pomocník (Co chcete udělat?) zná každou skupinu, tlačítko, přepínač a posuvník v pravém panelu karty Model a dovede tam: karta Model, oblast, skupina, prvek.
- Němčina: nabídka Overlays jako v Blenderu se jmenuje Überlagerungen, jako v německém rozhraní Blenderu.
- Příručka: stránky karty Model jsou v pořadí práce - přehled, ovládání, rozvržení jako Blender, časová osa, soubory, pak oblasti Prohlížení/Kontrola, Modelování, UV, Weights, Auto-skin, Painting a Animace; nové stránky pro rozvržení jako Blender, Prohlížení/Kontrola, panel Animace a Auto-skin.
- Karta Model, pravý panel: v malém okně jsou karty panelu příkazů jako v 3ds Max symboly v jedné řadě, název je v bublinové nápovědě (jako karty vlastností v Blenderu) - jako text potřebovaly dvě až tři řady.
