# Notizen ENTSCHEID3A (12./13.11.): verbindliche Entscheidungen B7-B13 und C4 im Model-Tab, Testfund mb2 - rc612

Auftrag ENTSCHEID3A (Opus 5.5), Entscheidungen aus `plaene/entscheidungen_1211.md` (Hauptchat im Auftrag von shank, B11 von shank
selbst: „ja“). Worktree `Y:\p_ap6`, Zweig `p_ent3a`; begonnen auf karte1 `1c6172246`, am 12.11. ~23:15 vom Wochenlimit gestoppt (WIP vom
Hauptchat gesichert), am 13.11. fortgesetzt, zuletzt auf karte1 `025357c8f` gesetzt (mit DXR, AP14, KLEIN, TEMPO und ENTSCHEID3B; QUELLEN mit beiden Seiten, i18n und Handbuch neu erzeugt). Nachtrag E3 (unten) vom Hauptchat im Auftrag von shank. Selbsttest `gui/selbsttest_ent3a.inc`
(`BHED_EDITORTEST=modellblock`, `BHED_MODELL_NUR=ent3a | ent3afoto | ent3ahb`), ctest `modell_ent3a`, Skripte und Läufe
`Y:\werkzeuge\ent3a\`.

## Was sich ändert

Neue Dateien: `gui/app_modelltab_ent3a.cpp` (Bausteine B7, B8, B11, B12, C4; Zustand `ModellEnt3a` im eigenen Block am Ende von
`gui/modelltab.h`), `include/bhed/modell/drehmodus.h` + `src/modell/drehmodus.cpp` (B9), `include/bhed/modell/stapelbegleit.h` +
`src/modell/stapelbegleit.cpp` (B12), `tests/modell_ent3a.cpp`, `tools/i18n_ent3a.py` (Präfix `Ea`, 6 Sprachen),
`tools/handbuch/inhalt_ent3a.py`, `gui/selbsttest_ent3a.inc`. Einstellung `modelBlenderToolbarWidth`.

| Punkt | Was | Vorbild / Quelle | Datei |
|---|---|---|---|
| B7 | Wie 3ds Max ist eine eingefrorene Zeile im Szenen-Explorer wählbar (auch Tag, Knochen, Umschalt-Bereich); sie bleibt eingefroren. Tooltip der Zeile sagt es. „Auswahl auftauen“ im Rechtsklick der Zeile (nur wenn Gewähltes eingefroren ist) und in „Alle Aktionen suchen“. In der Ansicht geht der Klick weiter hindurch, der Rahmen lässt Eingefrorenes aus (ENTSCHEID1). Wie Blender (Pfeil „wählbar“ aus) unverändert | Autodesk-Hilfe „Scene Explorer Right-Click Menus“: „Unfreeze Scene Explorer Selection … as it's not possible to select frozen objects in the viewports“ | `waehleFlaeche`, Klickzweige, `frostPunkt`, `aktionen()` (`app_modelltab_explorer.cpp`); `modellEnt3aAuswahlAuftauen` |
| B8 | Wie Blender lässt sich die Werkzeugleiste an ihrer rechten Kante breiter ziehen; über 120 Punkte (Bildpunkte / Oberfläche) zeigt sie die Namen, darunter nur Symbole in der Größe der schmalen Leiste. Breite in den Einstellungen; Doppelklick auf die Kante: schmal. „Werkzeugnamen zeigen“ (ENTSCHEID1) bleibt | Blender 5.0 `space_toolsystem_common.py` `_layout_generator_detect_from_region`: `region.width * view2d_scale / ui_scale > 120` → `show_text` | `modellLinksSpalte`, `leistenKnopf` (`app_modelltab_blender.cpp`); `modellEnt3aLeisteKante` |
| B9 | Seitenleiste (nur wie Blender) bei Knochen/Griff: lokale Werte wie Blenders `matrix_basis` (Traeger^-1 · Welt, Traeger = Elternpose · Elternruhe^-1 · Ruhe; Ruhe = Grundpose der .gla), Drehung im Drehmodus des Knochens (Vorgabe Quaternion (WXYZ) wie Blender, sechs Euler-Reihenfolgen, Achsenwinkel), Auswahl „Rotationsmodus“, Skalierung nur ablesbar. Eingabe wie bisher (ein Rückgängig-Schritt, Grenzen des Griffs) | Blender `PoseBone.rotation_mode`, BLI_math_rotation (eulO_to_mat3, mat3_normalized_to_eulO, mat3_normalized_to_quat, quat_to_axis_angle); belegt gegen Blender 5.0 mathutils (`Y:\werkzeuge\ent3a\blender_dreh.py`, ohne Fenster) | `knochenLokal` (`app_modelltab_blender.cpp`), `bhed/modell/drehmodus.h` |
| B10 | Wie 3ds Max: Strg+Umschalt+Z = Zoom Extents All (alle Ansichten, ganze Figur), Wiederholen Strg+Y; auch wenn der Model-Tab den Tastaturfokus nicht hat. Wie Blender: Strg+Umschalt+Z Wiederholen (Text der Tastenliste jetzt „Wiederholen (Strg+Umschalt+Z)“ statt „(Strg+Y)“). „Zoom Extents All“ nennt die Taste, die Suche zeigt es einmal (nicht mehr zusätzlich in der Navigation) | Autodesk-Hotkeys 3ds Max (Zoom Extents All Ctrl+Shift+Z, Redo Ctrl+Y); Blender `ed.redo` Ctrl+Shift+Z | `maxTasten`, `blenderTasten`, `modellDateiTasten`, `modellNavAktionenSuche`; Texte `MnGrenzenAlle(Hint)`; Handbuch `inhalt_mbnav.py` m5 |
| B11 | Wie 3ds Max haben Edit Poly und Skin im Stapel ein Auge (eigene Namen „Edit Poly an/aus“, „Skin an/aus“). Edit Poly aus: die Fläche wird aus dem Netz gezeichnet, wie es geladen wurde (Einträge darüber rechnen darauf); Skin aus: Knochenmatrizen der Haut = Einheit → Bindepose, die Knochen behalten ihre Pose, Wählen/Bearbeiten wie in der Bindepose. Beides nur Anzeige (kein Rückgängig-Schritt, Speichern gleich), auch im Menü des Stapels und in der Suche. Wie Blender gibt es die Zeilen nicht, die Augen gelten dort nicht | 3ds Max Modifier-Stapel (Glühbirne je Modifier) | `stapelZeile`, `stufenZeile`, `zeichnungBauen` (`app_modelltab_stapel.cpp`), `posieren` (`app_modelltab_ansicht.cpp`) |
| B12 | Der Stapel bleibt in `model.glm.stack` neben der .glm (Text, bei Einträgen „in die Datei“ dazu die .glm ohne Stapel als Basis, damit erneutes Speichern dieselbe .glm ergibt). Die .glm schreibt Speichern wie bisher; Prüfsumme der .glm in der Begleitdatei. Kaputt, neuere Fassung, andere Flächen, .glm außerhalb geändert → Modell lädt ohne Stapel, Meldung mit Grund. Leerer Stapel → eine alte eigene Begleitdatei wird entfernt | Entscheidung B12 | `bhed/modell/stapelbegleit.h`, `modellEnt3aVorLaden/NachLaden/BegleitSchreiben`, `schreibenNach` (`app_modelltab_mb3.cpp`) |
| B13 | Wie 3ds Max steht im Stapel „Unwrap UVW“ über Edit Poly (unter den Einträgen - die rechnen mit den UVs); Klick wechselt in den Bereich UV; im Menü des Stapels und in der Suche. Im Bereich UV kein Stapel (Entwurf `e_max_uv`) | 3ds Max Unwrap UVW | `drawModellStapelGruppe`, `modellStapelMenue`, `modellStapelAktionenSuche` |
| C4 | Esc schließt das Rechtsklick-Menü der Ansicht (auch aus einem Untermenü) und tut sonst nichts (Werkzeug, Navigation, Auswahl bleiben) | 3ds Max Quad-Menü, Blender Kontextmenü | `drawModellKontextMenue`, `modellTasten` (`app_modelltab.cpp`) |

### E3 (Nachtrag, `plaene/entscheidungen_1211.md` E1-E3)

E1 (Skin aus für die ganze Figur) und E2 (zwei Spalten der Leiste erst nach rc612) bleiben wie gebaut. **E3:** die Begleitdatei gilt auch
für .md3-Requisiten (`model.md3.stack`, Basis ist dann die .md3 ohne Stapel; sie trägt beim nächsten Speichern Bilder und Tags), und der
Drehmodus der Knochen steht darin (`dreh <Modus> <Knochen>`, nur wer nicht Quaternion hat; Fassung 2, Kopfzeile `modell`). Ältere
Begleitdateien (Fassung 1, Kopfzeile `glm`, ohne Drehmodus) laden weiter. Ein Modell beginnt beim Öffnen mit Blenders Vorgabe, die
Begleitdatei bringt seine Drehmodi zurück; auch nur ein Drehmodus ohne Stapel bekommt eine Begleitdatei. Die .md3 schreibt Speichern wie
bisher. Nebenbei: das Öffnen einer .md3 merkte den Inhalt beim Laden nicht - Edit Poly hatte bei Requisiten kein Auge (B11); jetzt ja.
Dateien: `stapelbegleit.h/.cpp`, `modellEnt3aVorLaden/BegleitSchreiben`, `schreibenNach` (.md3-Zweig), `modellMd3Oeffnen`.

| E3 | rot | grün |
|---|---|---|
| ctest `modell_ent3a` | 4 FEHLER (Drehmodus schreiben/lesen, nur Drehmodus, .md3 öffnen, .md3 erneut bytegleich); grün schon: Fassung 1 lädt, .md3 bytegleich (Schreiber unverändert) | ALLES OK (modell_* 22 / 22) |
| Selbsttest `ent3a` | 53 OK / 3 FEHLER (Drehmodi nach dem Öffnen, .md3 mit Begleitdatei, .md3 öffnen mit Stapel und Auge) | 56 / 0 (die Prüfzeile B12 nahm noch die Fassung 1 der Kopfzeile an: berichtigt auf jede Fassung). Danach ent3b 38 / 0, ap13s 123 / 0, ap13k 66 / 0, mb2 39 / 0, ctest modell_* 22 / 22; Lints wie karte1 (i18n meldet PrTempoHint unbenutzt - so schon auf karte1 025357c8f, nicht von ENT3A) |

### Testfund mb2 („Füße blau“)

Rot nachgestellt (vor der Änderung, Bau `r2` auf karte1 fe0cbece7 + ENT3A-Bausteine): mb2 38 OK / 1 FEHLER, Protokoll „Fuss 108/116/71“ -
dieselbe Zeile wie auf karte1. Ursache: seit AP13K stehen Knochen im Bild der Ansicht vor der Figur, die Probe in der Mitte von r_leg traf
einen Knochenstab. Änderung: für die Farbprobe sind die Knochen aus (`g_app->modellKnochen`), danach wieder wie vorher. **Nicht schwächer:**
Orte (Ecke des Kopfs, Mitte von r_leg), Bedingungen (Kopf rot, Fuß blau, Gewicht > 0,95) und Schwellen bleiben; die Probe prüft zusätzlich,
dass die Knochen wirklich aus sind, und misst damit genau, was sie messen soll - die Farbe der Haut nach Blenders weight_to_rgb. Grün: mb2
39 / 0 (die neue Zeile „Knochen wieder wie vorher“ zählt mit).

## Selbsttest (rot -> grün)

Roter Lauf = Selbsttest und Bausteine ohne die Anschlüsse (Commit „self test … (red state)“, Bau `r2`); grüne Läufe auf dem Stand der
Abgabe.

| Lauf | Ergebnis |
|---|---|
| `ent3a` rot (`laeufe/rot_ent3a_voll.txt`) | 19 OK / 42 FEHLER - jede Prüfzeile der sieben Punkte rot; grün schon im roten Stand nur, was ohnehin galt (Blender-Outliner, 3ds-Max-Spalte, Strg+Y, „nur Anzeige“, Requisit) |
| `ent3a` g1 (erster grüner Bau) | 44 / 9 - drei Fehler der eigenen Umsetzung, zwei des Tests, siehe Funde |
| `ent3a` Schluss (Bau g5) | 52 / 0 unter Direct3D 12, 51 / 0 unter Direct3D 11 (dieselben Zeilen; Direct3D 12 dazu die Zeile des Abgebers) |
| `mb2` rot / grün | 38 / 1 → 39 / 0 |
| ctest `modell_ent3a` | 1. Lauf 1 FEHLER (Erwartung des Tests falsch: kopierter Inhalt trug schon einen Stapel), danach ALLES OK: 36 Euler, 6 Quaternionen, 6 Achse/Winkel, 8 Rückwege gegen Blender, Begleitdatei (Rundweg bytegleich, Alt, Kaputt, Fassung, Flächen) |

### Funde im ersten grünen Lauf (behoben, grün)

1. **Augen ohne neues Bild** (B11): das Feld steht hinter der Ansicht; `modelDirty` geht am Ende des Bilds aus, die Ansicht posierte und
   zeichnete nicht neu (Skin aus: 0 Bildpunkte anders, Ecke 274 Bildpunkte vom Ort der Datei; Edit Poly aus: 19637 Bildpunkte anders als
   geladen). Jetzt zählt `ModellEnt3a::ansichtStand` jedes Umschalten, die Ansicht posiert und zeichnet neu, wenn er sich ändert.
2. **Doppelklick auf die Kante** (B8): das zweite Drücken zog die Leiste wieder auf die Mausbreite; ein Klick ohne Weg setzte die Breite
   neu. Jetzt zählt der Weg seit dem Anfassen (ab 1 Bildpunkt), nach einem Doppelklick nicht bis zum Loslassen.
3. **Massstab der Grundpose** (B9, Test): die Matrizen der _humanoid.gla tragen den Massstab 0,64 (Ruhe und Pose); die Rechnung des Tests
   kehrte den Traeger nur durch Transponieren um (Ort 2,44-fach falsch). Die Seitenleiste rechnete richtig (volle Umkehrung); der Test
   kehrt jetzt mit Kofaktoren um (`ap4::lokalMatrix`, auch für ap4). Dazu las der Test die Quaternion über die Namen „ X“ (die stehen auch
   bei Ort) und suchte den Drehmodus im Popup statt in der Combo.

## Angepasste vorhandene Selbsttests (nichts gelöscht, keine Prüfung abgeschwächt)

- `selbsttest_ap2.inc` X13 (wie 3ds Max): „der Klick im Baum wählt torso nicht und meldet es“ ist durch B7 überholt → der Klick wählt
  torso, torso bleibt eingefroren. Der Klick in der Ansicht (hindurch) bleibt geprüft.
- `selbsttest_ap4.inc` (ENTSCHEID1, Seitenleiste): die Werte sind lokal (B9) - Becken X lokal eingetippt, Kopf im Drehmodus XYZ Euler
  (Blenders Vorgabe ist Quaternion), Strg+Z gegen die lokalen Werte; „grau == 3“ → 6 (Position 3 und Skalierung 3 nur ablesbar).
- `selbsttest_ap13s.inc`: die Zeilen wie 3ds Max haben „Unwrap UVW“ über Edit Poly (acht Erwartungen).
- `selbsttest_mb1a2.inc` (mb2): siehe Testfund.

## Fotos (angesehen)

Foto-Selbsttest `ent3afoto` (`Y:\werkzeuge\ent3a\fotos\ent3a_f_<variante>`): je Lauf `ent3a_explorer` (3ds Max: torso eingefroren und
gewählt, Rechtsklick mit „Auswahl auftauen“), `ent3a_leiste` (Blender: Leiste 160 Punkte mit Namen), `ent3a_seite`, `ent3a_seite_modi`
(Blender: Seitenleiste für den Griff Becken, Drehmodi aufgeklappt), `ent3a_stapel` (3ds Max: Stapel mit TurboSmooth, Unwrap UVW, Augen; Skin
aus, die Figur in der Bindepose). Läufe en, d3d11, de, ja, hell (3818 × 2104 bei Oberfläche 1,5), klein (1328 × 844, Mindestgröße).
Angesehen: de Explorer, en Leiste, en Seitenleiste mit Modi, en und klein Stapel, de Handbuchbild Seitenleiste. **Fotofund:** die Auswahl
des Drehmodus stand mit Beschriftung links und schnitt „Quaternion (WXYZ)“ ab → jetzt wie Blender ohne Beschriftung über die ganze Breite
(Name im Tooltip); neu aufgenommen (en, ja, klein, Handbuch), angesehen. Bei Mindestgröße kürzt die Zeile „TurboSmooth (nur Vorschau)“ mit
„…“ (wie AP13S, ganzer Name im Tooltip). Direct3D 12 gegen 11 (`bildgleich.py`, en gegen d3d11): 5 Bilder, je 0 oder 1 Bildpunkt um 1/255.

## Handbuch

`tools/handbuch/inhalt_ent3a.py` (6 Sprachen): fünf Bausteine hinter Absätzen anderer Quellen, Bilder `h_ent3a_explorer`, `h_ent3a_leiste`,
`h_ent3a_seite`, `h_ent3a_stapel`. Eingetragen im Register von AP14 (`tools/handbuch/inhalt_modell.py`, ein Block am Ende von QUELLEN):

```
    Hinter("inhalt_ent3a", "bloecke_explorer", "modell-ansehen", "inhalt_ap2", "ebenen"),       # B7 eingefrorene Zeile waehlbar
    Hinter("inhalt_ent3a", "bloecke_bedienung", "modell-bedienung", "inhalt_mb1a2", "b_text"),  # C4 Esc schliesst das Rechtsklick-Menue
    Hinter("inhalt_ent3a", "bloecke_leiste", "modell-blender", "inhalt_ap4", "gruppen"),        # B8 Leiste breiter ziehen
    Hinter("inhalt_ent3a", "bloecke_seite", "modell-blender", "inhalt_ap4", "seite"),           # B9 Seitenleiste bei Knochen
    Hinter("inhalt_ent3a", "bloecke_stapel", "modell-modellieren", "inhalt_ap13s", "bedienen"), # B11, B12, B13 im Modifier-Stapel
```

Berichtigt in vorhandenen Quellen: `inhalt_ap2.py` „ebenen“ (Klick im Baum wählt Eingefrorenes nur wie 3ds Max), `inhalt_mbnav.py` m5
(Strg+Umschalt+Z), `inhalt_ap13s.py` „datei“ (der Stapel steht daneben in model.glm.stack); Oberflächentexte `MnGrenzenAlle(Hint)` und
`StpTipp` in ihren Paketdateien.

## Schlussrunde des Zweigs

Auf dem Stand vor dem letzten Rebase (karte1 `3d0947208`, Bau `r5alles`; nach dem Rebase auf `933dd74f8` - TEMPO berührt nur den Karten-Editor - Bau `r6alles` 0 Warnungen, ent3a 52 / 0, ap13s 123 / 0, ctest `-R modell|handbuch|i18n|lint|stabilsort|nachplatz|settings|quell` 39 / 39; Last: `--parallel 2`, Läufe auf Leerlauf-Priorität, höchstens 2 gleichzeitig):

| Schritt | Ergebnis |
|---|---|
| Bau alle Ziele (inkrementell, /W4 /WX) | 0 Fehler, 0 Warnungen (`r5alles`) |
| ctest (`-j 2`, Leerlauf) | 268 / 268 (darin `modell_ent3a`, `modell_stapel`) |
| `ent3a` | 52 / 0 (Direct3D 12, Bau r5alles); Direct3D 11 51 / 0 (Bau g5, gleicher Code des Zweigs) |
| Gruppen, die der Code berührt (Bau r5alles) | mb2 39 / 0 · ap2 68 / 0 · ap4 106 / 0 · ap13s 123 / 0 · ap13k 66 / 0 · tasten 45 / 0 · nav 134 / 0 |
| Fotos / Handbuchbilder | ent3afoto 6 Varianten je 0 FEHLER; ent3ahb und ap13shb (h_ap13s_max: Stapel mit Augen und Unwrap UVW) in 6 Sprachen je 0 FEHLER |
| Lints (`Y:\werkzeuge\ent3a\lints.sh`) | wie karte1: pruefcodes 11, portability 101, efxflags und fremdprogramm mit derselben Meldung; i18n, settings (117 Schlüssel), headers, imgui_context … ok |
| Handbuch | mit dem Register von AP14 neu erzeugt: modell-ansehen (B7, ap2 „ebenen“, mbnav m5), modell-bedienung (C4), modell-blender (B8, B9), modell-modellieren (B11-B13); `--pruefen` grün, keine offenen {{…}} |

Bewusst nicht gelaufen (einmal auf dem zusammengeführten Stand): Modus 1, die übrigen modellblock-Gruppen (ap3, ap6-ap10, blender, kopf …),
Docks, Sprachläufe, sauberer Neubau. Keine Bildzeit-Messung: die Augen bauen die Zeichnung nur beim Umschalten neu, die Kante der Leiste ist
ein Element je Bild, die Seitenleiste rechnet je Bild eine 3x3-Umkehrung für den gewählten Knochen.

## Entscheidungen (mit Grund)

1. **Skin aus gilt für die ganze Figur** (bestätigt als E1), nicht je Fläche wie bei einem Skin-Modifier je Objekt in 3ds Max: eine .glm ist ein Modell an
   einem Skelett, alle Werkzeuge rechnen in der Pose der Ansicht (eine Pose für alle Flächen); je Fläche risse die Figur an den Nähten auf
   und Bearbeiten in der Bindepose ginge nur für einen Teil. Der Tooltip sagt es.
2. **Augen sind Ansicht, kein Dokument** (wie das Auge im Explorer): kein Rückgängig-Schritt, nicht in der Begleitdatei; ein neues Modell
   beginnt mit allen Augen an. Wie Blender gelten sie nicht (dort gibt es die Zeilen nicht).
3. **Edit Poly aus ohne geladene Fläche**: eine Fläche, die es beim Laden nicht gab (Detach, neues Requisit), hat ein graues Auge mit
   Grund im Tooltip.
4. **Begleitdatei auch für .md3** (E3, nachgetragen).
5. **Basis in der Begleitdatei**, wenn ein Eintrag in die Datei ging: sonst läge das Spiegeln beim Öffnen zweimal auf der Fläche.
6. **Drehmodus je Knochen in der Begleitdatei** (E3, nachgetragen; nach Name).
7. **B7 nur wie 3ds Max**: Blender beschreibt „Disable Selection“ für den 3D-Viewport; der Outliner bleibt, wie ENTSCHEID1/AP2 ihn
   gebaut haben.

## Offen / Fragen an shank

Die drei Fragen sind beantwortet (E1 so lassen, E2 nach rc612, E3 gebaut). Offen: zwei Spalten der Werkzeugleiste zwischen 80 und 120
Punkten wie Blender (nach rc612).

## Changelog

## English

- Model tab, Scene Explorer (3ds Max mode): a frozen row can be selected in the list, as in the Scene Explorer of 3ds Max; "Unfreeze selection" in the right-click menu of the row and in Search all actions unfreezes it. In the view a click still goes through frozen surfaces.
- Toolbar like Blender: drag its right edge to make it wider; wider than 120 points it shows the tool names, as in Blender. The width is kept; a double-click on the edge brings back the narrow toolbar.
- Sidebar like Blender: for a bone or rig control it shows local values (relative to the rest position) and the rotation in the rotation mode of the bone - Quaternion (WXYZ) by default, the six Euler orders or Axis Angle, as in Blender; scale is shown for reading.
- 3ds Max mode: Ctrl+Shift+Z is Zoom Extents All, as in 3ds Max; Redo is Ctrl+Y. In Blender mode Ctrl+Shift+Z stays Redo. The action search shows the right keys.
- Modifier stack (3ds Max mode): Edit Poly and Skin have an eye. Edit Poly off shows the surface as it was loaded, Skin off shows the figure in its bind pose; only the view changes. A row "Unwrap UVW" switches to the UV area.
- The modifier stack is kept next to the model in model.glm.stack and comes back when the model is opened again; the .glm stays byte for byte the same and the game does not read the new file. If the file cannot be read, the model opens without the stack and the status line says why.
- Esc closes the right-click menu of the view, as in 3ds Max and Blender.
- The modifier stack companion file also works for .md3 props (model.md3.stack) and keeps the rotation mode of each bone; older companion files still open.

## Deutsch

- Model-Tab, Szenen-Explorer (3ds-Max-Modus): eine eingefrorene Zeile lässt sich in der Liste wählen, wie im Scene Explorer von 3ds Max; „Auswahl auftauen“ im Rechtsklick-Menü der Zeile und in „Alle Aktionen suchen“ taut sie auf. In der Ansicht geht ein Klick weiter durch eingefrorene Flächen hindurch.
- Werkzeugleiste wie Blender: an ihrer rechten Kante breiter ziehen; breiter als 120 Punkte zeigt sie die Werkzeugnamen, wie Blender. Die Breite bleibt; ein Doppelklick auf die Kante bringt die schmale Leiste zurück.
- Seitenleiste wie Blender: für einen Knochen oder Griff des Rigs zeigt sie lokale Werte (bezogen auf die Ruhelage) und die Drehung im Drehmodus des Knochens - ab Werk Quaternion (WXYZ), die sechs Euler-Reihenfolgen oder Achsenwinkel, wie Blender; die Skalierung steht zum Ablesen da.
- 3ds-Max-Modus: Strg+Umschalt+Z ist Zoom Extents All, wie in 3ds Max; Wiederholen ist Strg+Y. Im Blender-Modus bleibt Strg+Umschalt+Z Wiederholen. Die Aktionssuche nennt die richtigen Tasten.
- Modifier-Stapel (3ds-Max-Modus): Edit Poly und Skin haben ein Auge. Edit Poly aus zeigt die Fläche wie geladen, Skin aus zeigt die Figur in der Bindepose; es ändert sich nur die Anzeige. Eine Zeile „Unwrap UVW“ wechselt in den Bereich UV.
- Der Modifier-Stapel bleibt neben dem Modell in model.glm.stack erhalten und kommt beim nächsten Öffnen wieder; die .glm bleibt Byte für Byte gleich, und das Spiel liest die neue Datei nicht. Lässt sie sich nicht lesen, öffnet sich das Modell ohne Stapel, und die Statuszeile sagt warum.
- Esc schließt das Rechtsklick-Menü der Ansicht, wie in 3ds Max und Blender.
- Die Begleitdatei des Modifier-Stapels gilt auch für .md3-Requisiten (model.md3.stack) und behält den Rotationsmodus jedes Knochens; ältere Begleitdateien öffnen sich weiter.

## 中文

- 模型选项卡, 场景资源管理器 (3ds Max 模式): 冻结的行可以在列表中选择, 与 3ds Max 的场景资源管理器相同; 行右键菜单和“搜索所有操作”中的“解冻所选内容”可将其解冻。在视图中单击仍会穿过冻结的面。
- 像 Blender 的工具栏: 拖动右边缘可加宽; 宽度超过 120 点时显示工具名称, 与 Blender 相同。宽度会保留; 双击边缘恢复窄工具栏。
- 像 Blender 的侧栏: 对于骨骼或绑定控制器显示局部值 (相对于静止位置) 以及按骨骼旋转模式显示的旋转 - 默认四元数 (WXYZ)、六种欧拉顺序或坐标轴角度, 与 Blender 相同; 缩放仅供查看。
- 3ds Max 模式: Ctrl+Shift+Z 为“所有视口最大化显示”, 与 3ds Max 相同; 重做为 Ctrl+Y。Blender 模式下 Ctrl+Shift+Z 仍为重做。操作搜索显示正确的按键。
- 修改器堆栈 (3ds Max 模式): Edit Poly 和 Skin 有眼睛图标。关闭 Edit Poly 按加载时的样子显示面, 关闭 Skin 以绑定姿态显示角色; 只改变显示。“Unwrap UVW”行可切换到 UV 区域。
- 修改器堆栈保存在模型旁的 model.glm.stack 中, 再次打开模型时会恢复; .glm 逐字节保持不变, 游戏不读取新文件。如果无法读取该文件, 模型会在没有堆栈的情况下打开, 状态栏会说明原因。
- Esc 关闭视图的右键菜单, 与 3ds Max 和 Blender 相同。
- 修改器堆栈的伴随文件也适用于 .md3 道具 (model.md3.stack), 并保存每块骨骼的旋转模式; 旧的伴随文件仍可打开。

## 日本語

- モデルタブ、シーン エクスプローラ (3ds Max 風): フリーズした行を一覧で選択できます (3ds Max のシーン エクスプローラと同じ)。行の右クリック メニューと「すべての操作を検索」の「選択をフリーズ解除」でフリーズを解除できます。ビューでのクリックは引き続きフリーズしたサーフェスを通り抜けます。
- Blender 風のツールバー: 右端をドラッグして広げられ、120 ポイントより広いとツール名を表示します (Blender と同じ)。幅は保たれ、端をダブルクリックすると細いツールバーに戻ります。
- Blender 風のサイドバー: ボーンやリグのコントロールについてローカル値 (レスト位置基準) と、ボーンの回転モードでの回転を表示します - 既定はクォータニオン(WXYZ)、6 つのオイラー順序、軸の角度 (Blender と同じ)。スケールは表示のみです。
- 3ds Max 風: Ctrl+Shift+Z は 3ds Max と同じく「全ビューで全体表示」、やり直しは Ctrl+Y です。Blender 風では Ctrl+Shift+Z はやり直しのままです。操作検索は正しいキーを表示します。
- モディファイアスタック (3ds Max 風): Edit Poly と Skin に目のアイコンがあります。Edit Poly をオフにすると読み込んだときのサーフェスを、Skin をオフにするとバインドポーズのフィギュアを表示します。変わるのは表示だけです。「Unwrap UVW」の行で UV エリアに切り替わります。
- モディファイアスタックはモデルの隣の model.glm.stack に保存され、次に開いたときに戻ります。.glm はバイト単位で同じままで、ゲームは新しいファイルを読みません。読めない場合、モデルはスタックなしで開き、ステータス行に理由が表示されます。
- Esc でビューの右クリック メニューを閉じます (3ds Max や Blender と同じ)。
- モディファイアスタックの付随ファイルは .md3 の小道具にも使え (model.md3.stack)、各ボーンの回転モードも保存します。古い付随ファイルも開けます。

## Español

- Pestaña Model, Explorador de escena (modo 3ds Max): una fila congelada se puede seleccionar en la lista, como en el Scene Explorer de 3ds Max; «Descongelar selección» en el menú contextual de la fila y en «Buscar todas las acciones» la descongela. En la vista, un clic sigue atravesando las superficies congeladas.
- Barra de herramientas como Blender: arrastra su borde derecho para ensancharla; con más de 120 puntos muestra los nombres de las herramientas, como Blender. El ancho se conserva; un doble clic en el borde vuelve a la barra estrecha.
- Barra lateral como Blender: para un hueso o control del rig muestra valores locales (respecto a la posición de reposo) y la rotación en el modo de rotación del hueso: por defecto cuaternio (WXYZ), los seis órdenes de Euler o eje y ángulo, como Blender; la escala se muestra para leer.
- Modo 3ds Max: Ctrl+Mayús+Z es Encuadrar todo en todas las vistas, como en 3ds Max; Rehacer es Ctrl+Y. En modo Blender, Ctrl+Mayús+Z sigue siendo Rehacer. La búsqueda de acciones muestra las teclas correctas.
- Pila de modificadores (modo 3ds Max): Edit Poly y Skin tienen un ojo. Con Edit Poly desactivado se ve la superficie tal como se cargó, con Skin desactivado la figura en su pose de enlace; solo cambia la vista. Una fila «Unwrap UVW» cambia al área UV.
- La pila de modificadores se conserva junto al modelo en model.glm.stack y vuelve al abrir el modelo de nuevo; el .glm se mantiene igual byte a byte y el juego no lee el nuevo archivo. Si no se puede leer, el modelo se abre sin la pila y la línea de estado dice por qué.
- Esc cierra el menú contextual de la vista, como en 3ds Max y Blender.
- El archivo acompañante de la pila de modificadores sirve también para accesorios .md3 (model.md3.stack) y guarda el modo de rotación de cada hueso; los archivos antiguos se siguen abriendo.

## Čeština

- Karta Model, Průzkumník scény (režim 3ds Max): zmrazený řádek lze v seznamu vybrat jako ve Scene Exploreru 3ds Max; „Rozmrazit výběr“ v nabídce pravého tlačítka řádku a v „Hledat všechny akce“ jej rozmrazí. V pohledu kliknutí dál prochází zmrazenými plochami.
- Panel nástrojů jako Blender: tažením za pravý okraj se rozšíří; nad 120 bodů ukazuje názvy nástrojů jako Blender. Šířka zůstane; dvojklik na okraj vrátí úzký panel.
- Postranní panel jako Blender: pro kost nebo ovladač rigu ukazuje místní hodnoty (vůči klidové poloze) a rotaci v režimu rotace kosti - výchozí Quaternion (WXYZ), šest pořadí Eulerových úhlů nebo Úhel Osy jako Blender; měřítko je ke čtení.
- Režim 3ds Max: Ctrl+Shift+Z je Přiblížit vše ve všech pohledech jako ve 3ds Max; Znovu je Ctrl+Y. V režimu Blender zůstává Ctrl+Shift+Z Znovu. Hledání akcí ukazuje správné klávesy.
- Zásobník modifikátorů (režim 3ds Max): Edit Poly a Skin mají oko. Vypnutý Edit Poly ukazuje plochu tak, jak byla načtena, vypnutý Skin postavu ve vazební póze; mění se jen zobrazení. Řádek „Unwrap UVW“ přepne do oblasti UV.
- Zásobník modifikátorů zůstává vedle modelu v model.glm.stack a při dalším otevření se vrátí; .glm zůstává bajt po bajtu stejný a hra nový soubor nečte. Pokud ho nejde přečíst, model se otevře bez zásobníku a stavový řádek řekne proč.
- Esc zavře nabídku pravého tlačítka v pohledu jako ve 3ds Max a Blenderu.
- Doprovodný soubor zásobníku modifikátorů platí i pro rekvizity .md3 (model.md3.stack) a uchovává režim rotace každé kosti; starší doprovodné soubory se dál otevřou.
