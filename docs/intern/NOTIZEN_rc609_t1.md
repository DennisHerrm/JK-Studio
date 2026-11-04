# Notizen rc609 T1: Tabs als eigene Fenster

Wunsch shank (03.11.): "dann mach auch das man tabs als eigene Fenster öffnen kann das man in behaved und in mission gleichzeitig arbeiten kann wenn man die auf 2 Bildschirme zieht". Ein ganzer Tab (Events, Mission, Model, Map Editor, Menu Editor) wird ein eigenes Windows-Fenster; beide Tabs sind gleichzeitig bedienbar.

## Stufe 1: Bestandsaufnahme `leftMode`

Stand 6e4892d2: 542 Treffer in `gui/` (`grep -c leftMode`), davon 117 ausserhalb der Selbsttests. `leftMode` ist der Tab des Hauptfensters (0 Events, 1 Mission, 2 Model, 3 Map Editor, 4 Menu Editor, `gui/app_internal.h:426`).

Arten:
- **Z** Zeichnen: Anordnung und Inhalt des Tabs, der gerade zeichnet.
- **M** Menueleiste: Inhalt des Menues haengt am Tab.
- **K** Tastenkuerzel und Eingabe: wohin eine Taste, ein Mausrad oder eine abgelegte Datei geht.
- **U** Undo: welcher Stapel (laeuft ueber K und M).
- **F** Fokus: "welcher Tab ist gemeint", ohne zu zeichnen (Helfer F1, KI-Anbindung, Hervorhebung).
- **W** Wechsel: setzt `leftMode` (zu einem Tab gehen).
- **G** global: Bildzustand, der einmal je Bild gilt (Moduswechsel, Abspielen, Leerlauf).
- **E** Einstellungen: Anteil je Tab (`splitMap/Model/Events`) und Anordnung je Bereich.
- **S** Selbsttest, Tour, Messwerte.

### Ausserhalb der Selbsttests (117)

| Datei | Zeilen | Art | Was |
|---|---|---|---|
| `app.cpp` | 5470-5471 | K | Strg+Rad vergroessert, nicht im Map/Menu Editor |
| `app.cpp` | 5502, 5551 | Z, M | NRC-Menueleiste und Reiterleiste der Skripte je Tab |
| `app.cpp` | 5735-5753 | G | Moduswechsel einmal je Bild (Einschwingbild, Abspielen anhalten beim Verlassen von Mission) |
| `app.cpp` | 5790 | S | Groessen ins Detailprotokoll |
| `app.cpp` | 5995-6838 (19) | Z, E | Spalten, Tabellen-ID je Tab, Anteile je Tab, Band unter den Spalten, Debug-Fenster |
| `app.cpp` | 3797, 5034 | W | Modell laden -> Model; Menue-Projekt -> Menu Editor |
| `app_anordnung.inc` | 1369-1541 (12) | Z | `computeLayout` je Tab (Knopfspalte, Kopf, Band, Reiterzeile) |
| `app_anordnung.inc` | 1707, 1718 | Z, W | Modusleiste: hervorgehoben, Klick wechselt |
| `app_anordnung.inc` | 1798-2171 (14) | Z | linke Spalte, Ansicht, Ebenen je Tab |
| `app_anordnung.inc` | 3026, 3308 | Z | Felder der Aufteilung nur bei Events |
| `app_anordnung.inc` | 4781, 4787 | Z | Statuszeile je Tab |
| `app_anordnung.inc` | 4998, 5455 | M | Menue "Aufteilen" nur bei Events; Menue "Fenster" des Bereichs |
| `app_dock.inc` | 11-64 (7) | Z | Panels des Tabs (`p.bereich != bereich`), Spalten ohne herausgenommene Panels |
| `app_dockspace.inc` | 73-75, 427 | Z | DockSpace des Tabs (Wurzel global je Bereich: `wurzelId`, `ImHashStr`) |
| `app_ansichten.cpp` | 946, 1048 | W | Graph: Entity -> Mission |
| `app_ansichten.cpp` | 1167 | G | Spuren-Ansicht treibt die Uhr, wenn Mission nicht zeichnet |
| `app_ansichten.cpp` | 1423 | Z | Felder der Aufteilung |
| `app_enttext.cpp` | 693 | K, F | Entity-Text hat den Fokus |
| `app_enttext.cpp` | 716 | Z | Entity-Text nur im Mission-Tab |
| `app_karte.cpp` | 365 | Z, K | `karteneditor::aktiv()`: zeichnet und nimmt NRC-Tasten (`tastenVerarbeiten`, im Zeichnen) |
| `app_karte_logik.cpp` | 79 | W | Skript aus der Karte -> Events |
| `app_karte_texturliste.cpp` | 343 | Z | Texturen der Karte nur im Map Editor |
| `app_kartemission.cpp` | 301 | G | Dialog "Bezuege nach dem Nachladen" (wird nach den Tabs gezeichnet) |
| `app_lumpedit.cpp` | 318-1094 (7) | Z, K | Shader-Panel: Dokument des Tabs (Mission/Map Editor), Fokus |
| `app_menue.cpp` | 7045 | Z, K | `menueeditor::aktiv()`: zeichnet und nimmt Tasten (`editorTasten`, im Zeichnen) |
| `app_mission.cpp` | 1044 | K | Entf und Strg+D auf Entities |
| `app_mission.cpp` | 4094, 4696 | W | neue/geoeffnete Mission -> Mission |
| `app_view3d.cpp` | 1897 | W | Modell laden -> Model |
| `app_view3d_spiel.inc` | 4933 | W | Mission gewaehlt -> Mission |
| `app_zeitleiste.cpp` | 1772 | Z | Erklaerung in der Zeitleiste |
| `app_ablage.inc` | 58, 60 | K | abgelegte Datei: Modell in den Map Editor, Menue in den Menu Editor |
| `fenster_dock.cpp` | 192 | M, E | Anordnung speichern unter (Bereich) |
| `fenster_dock.h` | 32 | - | Kommentar |
| `hilfe.cpp` | 141-311 (4) | F | Helfer gewichtet Treffer des Tabs |
| `ki.cpp` | 251, 360 | F, W | KI liest und setzt den Tab |
| `tour.cpp` | 307, 472, 580, 1160 | S, W | Tour: Zustand, 3D-Ziel, Aktion "Modus", Menue-Tutorials |
| `tour_messwerte.cpp` | 212-291 (13) | S | Messwerte der Touren |
| `app_internal.h` | 426 | - | Feld |

Dazu gleichbedeutend: `karteneditor::aktiv()` / `menueeditor::aktiv()` in `handleShortcuts` und `handleTreeKeys` (`app.cpp:2370, 2736`, K), Fenstertitel (`app_ablage.inc:4180`, Hauptfenster) und Leerlauf (`app_ablage.inc:4228`, G).

### Selbsttests (425)

Groesste: `selbsttest_handbuch.inc` 97, `selbsttest_menue.inc` 49, `selbsttest_vorschau_b.inc` 45, `selbsttest_dock1.inc` 26, `selbsttest_programm.inc` 17, `selbsttest_dock4.inc` 16, `selbsttest_menue_rc608.inc` 15, `selbsttest_menue_pruefung.inc` 14, `selbsttest_hilfe2.inc` 13, `selbsttest_vorschau_a.inc` 12, `selbsttest_menue_leiste.inc` 11, `selbsttest_bspmerge.inc` 10, Rest 1-7 je Datei. Fast alle setzen den Tab (`g_app->leftMode = n`, W) oder pruefen ihn (S) - immer den des Hauptfensters, zwischen zwei Bildern.

### Folgerung fuer Stufe 2
- `leftMode` bleibt das Feld, das alle lesen. Zwischen den Bildern (Selbsttests, Touren, W) ist es der Tab des Hauptfensters. Beim Zeichnen eines eigenen Fensters steht darin dessen Tab (Z, E, M ohne Aenderung an den 117 Stellen).
- Pro Fenster gehalten (sonst laeuft es zwischen zwei Fenstern im selben Bild hin und her): `letzterModus`, `moduswechselBild`, `letzteFensterB`, `fensterStabilSeit`, `modeBarW`, die Wahl der Reiterleiste (`imguiWahl`, bisher `static`).
- K: Kuerzel (`handleShortcuts`, `handleTreeKeys`) laufen mit dem Tab des Fensters mit Fokus. NRC-Tasten und Menu-Editor-Tasten werden im Zeichnen verarbeitet - sie gelten nur, wenn ihr Fenster den Fokus hat.
- W: Ein Wechsel zu einem Tab, der in einem eigenen Fenster steht, holt dieses Fenster nach vorn, statt das Hauptfenster umzuschalten.
- G: "Mission zeichnet" heisst "Mission steht in irgendeinem Fenster" (`tabSichtbar(1)`), nicht `leftMode == 1`.
