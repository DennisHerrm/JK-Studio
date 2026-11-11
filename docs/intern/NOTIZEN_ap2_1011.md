# Notizen AP0 + AP2 (11.11.): Tooltip der Modusleiste, Szenen-Explorer (wie 3ds Max) / Outliner (wie Blender) - MBENTWURF Teil B, rc612

Auftrag `auftrag_mbentwurf_b_1011.md`, Pakete AP0 + AP2; Maßstab ist der freigegebene Entwurf (`plaene/modellblock_stufe0/entwurf_modelltab.html`),
shanks Wünsche vom 04.11. und 10.11. und die Abgleichtabelle `abgleich_programm_entwurf.md` (Abschnitt 4: X1-X28, Abschnitt 14: X26).
Worktree `Y:\p_ap2`, Zweig `p_ap2`, Basis `c624e0cb1`. Selbsttest `gui/selbsttest_ap2.inc` (`BHED_EDITORTEST=modellblock`,
`BHED_MODELL_NUR=ap2 | ap2foto`), Skripte und Läufe `Y:\werkzeuge\ap2\`.

shank 10.11.: „wie die augen und das aufklappen angeordnet ist, sollte wie bei 3ds max sein im 3ds max modus“ - „es geht ja nicht nur um
controls like blender oder 3ds max sondern das aussehen“; der Tooltip „Build scripts ...“ über „By hierarchy / By layers“ ist ein Fehler.

## AP0: Tooltip der Modusleiste (X26)

- **Nachgestellt (rot):** Lauf `ap2_rot` (alter Stand + Selbsttest): der Tooltip stand 0,015 s nach dem Auflegen der Maus da (im naechsten Bild), bei 172/224-982/296 = 810 Punkte = 22,5 Zeichenhoehen breit, quer ueber Suchfeld und erster Zeile des Explorers; „By hierarchy“ und „By layers“ zeigten keinen Tooltip. 3 der 4 Pruefungen rot; gruen war schon vorher „liegt nicht ueber der Zeile By hierarchy / By layers“ (er lag direkt darunter, wie in Teil A gemessen) - diese Pruefung bleibt als Absicherung.
- **Ursache** (Teil A): `drawModeBar` zeigte den Tooltip mit `IsItemHovered()` ohne Verzögerung rechts unter der Maus; 22 Zeichenhöhen breit lag er
  über dem Kopf des Nachbarfelds, im Model-Tab genau auf dem Suchfeld unter „By hierarchy / By layers“ - die beiden Knöpfe selbst hatten keinen.
- **Behoben:** `ImGuiHoveredFlags_ForTooltip` (übliche Verzögerung, ruhende Maus), Text auf 18 Zeichenhöhen umbrochen, mit `SetNextWindowPos` an den
  Knopf geheftet: rechts neben der Leiste, ab 60 % der Knopfhöhe (die Oberkante liegt damit unter der ersten Zeile des Nachbarfelds).
  „By hierarchy“ / „By layers“ (wie Blender „Hierarchy“ / „Collections“) haben eigene Tooltips.
- **Grün:** Lauf `ap2_g4`: Tooltip nach 0,196 s (Verzoegerung von ImGui fuer Tooltips), 246/214-881/322 = 635 Punkte = 17,6 Zeichenhoehen breit, links an der Leiste (Kante 230) mit kleiner Spitze zum Knopf, Oberkante 214 = 60 % der Knopfhoehe (Knopf 158-252; die Zeile „By hierarchy / By layers“ endet bei 198). „By hierarchy“: Tooltip nach 0,196 s unter dem Knopf, umbrochen (787 Punkte). Foto angesehen (`fotos\selbsttest\ap0_tooltip_events.png`).

## AP2: der Baum

Neue Datei `gui/app_modelltab_explorer.cpp` (der alte Explorer aus `gui/app_modelltab.cpp` und die eigene Sequenzliste aus `gui/app_modell.cpp`
sind heraus). Zustand und Schnittstelle als Block am Ende von `gui/modelltab.h` (`ModellExplorerZustand`, `modellExplorer…`), Texte in
`tools/i18n_ap2.py` (6 Sprachen), Symbole in `gui/uiicons.cpp` (eigene Zeichnung: Punkt/Ring für an/aus, Anker, Schneeflocke, Mauszeiger,
Trichter, Abspiel-Dreieck, Stern).

- **Ein Baum** (shank 04.11.): Modell (`kyle/model.glm`) > Flächen (Hierarchie) > Tags (mit Anbauten) > Knochen (Kopf nennt jetzt die GLA) >
  Griffe des Control Rigs > Sequenzen der animation.cfg. Die linke Spalte ist ganz der Explorer (vorher 55 % Baum + Liste „Animations“), ab Werk
  19,5 statt 15 Zeichenhöhen breit (Entwurf), im Andockraum wie bisher ziehbar. Titel des Felds „Scene Explorer“, wie Blender „Outliner“.
- **Zeile wie 3ds Max:** [Pfeil][Auge][Symbol] Name, Spalte „Frozen“ mit der Schneeflocke; Spaltenköpfe (Name, Frozen) in der Breite ziehbar,
  Klick auf „Name“ sortiert auf/ab/zurück zur Hierarchie, Klick auf „Frozen“ holt Eingefrorenes nach oben. Quelle für die Anordnung:
  Autodesk-Hilfe Scene Explorer / Layer Explorer („click the icon at the left side of any item to hide or unhide“, „in the Frozen column, click
  the icon of any item to freeze or unfreeze“, „sort the Scene Explorer listing by clicking a column heading“),
  https://help.autodesk.com/cloudhelp/2017/ENU/3DSMax/files/GUID-8E194B5E-221E-4896-93F8-18072DC6EB82.htm
- **Zeile wie Blender:** [Pfeil][Symbol] Name, rechts in festen Spalten Auge und Pfeil „wählbar“, keine Spaltenköpfe, Filter-Ausklapper neben der
  Suche (Arten, „Bones as a flat list“, „Sort alphabetically“, Spalte „Selectable“, TORSO_ als Sekundär). Quelle: Blender-Handbuch, Outliner >
  Interface (Restriction Columns: „Visibility (eye icon)“, „Selectability (mouse cursor icon)“; Filter: „Sort Alphabetically“),
  https://docs.blender.org/manual/en/4.0/editors/outliner/interface.html . Der ORT (oben rechts) gehört zu AP4.
- **Einzug:** eine Zeichenhöhe je Ebene (Entwurf: 13 px bei 13 px Schrift; vorher 1,3 + Augenspalte links). Hat der tiefste offene Knoten sonst
  keinen Platz für seinen Namen (5 Zeichenhöhen), staucht sich der Einzug bis auf 0,45 - der Baum rollt nie waagerecht. Ein Name, der nicht
  passt, endet mit „...“ und steht ganz im Tooltip. Gemessen (Fenster 3818 x 2104 bei Oberflaeche 1,5, Schrift 36 Punkte, kyle aus Movie Duels: 83 Teile, 44 Tags, 53 Knochen, 2635 Sequenzen): Explorer 19,5 Zeichenhoehen breit, Namensspalte 17,6; Knochen zu: Einzug 1,00, 29 Zeilen im Bild, 6 Namen gekuerzt (alle Kappen mit langen Namen wie `l_arm_cap_l_hand_off`); alle Knochen offen (Tiefe 10): Einzug 0,72, 31 Knochenzeilen im Bild, 0 gekuerzt, kein waagerechtes Rollen. Vorher (Teil A): Einzug 1,3, Inhalt 20,9 breit im 13,1 breiten Feld, 91 von 141 Zeilen ueber dem Rand.
- **Nach Ebenen** (wie Blender: Sammlungen): Körper, Kappen (_off), Tags, Knochen, Anbauten, LOD 2, LOD 3 (weitere Stufen, wenn das Modell sie
  hat), je mit Zahl, Auge und Schneeflocke; darunter Rig und Sequenzen. Das Auge einer LOD-Ebene zeigt diese Detailstufe (noch ein Klick:
  automatisch wie im Spiel). Fußzeile wie im Entwurf.
- **Einfrieren:** je Fläche, Tag, Knochen und je Ebene/Kopf; Kappen ab Werk eingefroren (Entwurf). Eingefrorenes bleibt sichtbar; Klick im Baum
  und in der Ansicht (`flaecheUnter`) wählt es nicht und meldet es (Meldungszeile des Befehlsfelds und Statuszeile), der Auswahlrahmen
  (`flaechenImRahmen`) lässt es aus; was eingefroren wird, ist nicht mehr gewählt. Umschalt+Klick auf die Schneeflocke nimmt die Kinder mit.
- **Sequenzen im Baum:** Zeile „▷ Name  Bilder · loop“, Schloss mit P (primär) / S (sekundär), rotes Warnzeichen bei Bildern hinter dem Ende der
  GLA (Grund im Tooltip). Unter dem Knoten die Klappliste der Gruppe (All / BOTH / TORSO / LEGS / FACE); „Copy“ und „Into script“ stehen immer
  sichtbar im Kopf des Explorers, dazu im Rechtsklick, in „Alle Aktionen suchen“ und per Doppelklick. Gebaut werden nur die sichtbaren Zeilen
  (ImGuiListClipper über den ganzen Baum; im Selbsttest alle Zeilen außer den Sequenzen, damit das Elementverzeichnis sie findet).
- **Filter:** wie 3ds Max eine Symbolreihe unter der Suche (Flächen, Kappen, Tags, Knochen, Sequenzen, angebaute Modelle, Rig-Griffe;
  Strg+Klick: nur diese Art), wie Blender der Ausklapper. Die Suche filtert alle Arten, auch die Sequenzen.
- **Zustandszeichen** vor der Fläche: Punkt = an, Ring = aus, durchgestrichener Ring = aus samt Kindern; Anker hinter dem Namen = Wurzelfläche.
- **Info:** Rechtsklick „Info“ (Fläche: Ecken, Dreiecke, Knochenbezüge, Shader, Eltern, Status; Tag: Lage, Eltern, Anbauten; Knochen: Nummer,
  Eltern, Kinder, Anbauten) und dieselben Angaben im Tooltip der Zeile. Rechtsklick Fläche zusätzlich „Release the root“, Einfrieren.
- **Mehrfachauswahl:** Strg+Klick wie bisher, Umschalt+Klick wählt den Bereich in Baumreihenfolge (ohne Eingefrorenes).
- **Klick:** Knochen blendet die Knochen ein, Tag die Tags (Entwurf); TORSO_-Klick als Sekundär ist ein Schalter (wie ModView), ab Werk aus.

## Bewusst anders als Entwurf bzw. Vorschlag der Tabelle

- **Auge und Schneeflocke an JEDER Zeile, Spaltenkoepfe** (Entwurf: nur je Ebene): shank 10.11. geht vor („wie bei 3ds max“).
- **Kein waagerechter Rollbalken** (Auftrag: „nur wenn noetig“): der Einzug staucht sich statt dessen, Namen enden notfalls mit „...“ und stehen
  im Tooltip - so bleiben Auge, Name und die Spalte „Frozen“ immer im Bild. Die Namensspalte fuellt die Breite; ziehbar ist die Grenze zur
  Spalte „Frozen“ (und das ganze Feld im Andockraum).
- **Spaltenkopf „Frozen“ als Schneeflocke** (Text im Tooltip): das Wort ist in mehreren Sprachen breiter als die Spalte.
- **Knochen als Hierarchie** (Entwurf: flache Liste): bleibt wie bisher (ModView zeigt sie auch); flach ueber Rechtsklick auf „Bones“ bzw. den
  Filter-Ausklapper.
- **Zustandszeichen statt Art-Symbol** vor Flaechen (Entwurf); „Flaeche mit Kindern“ und „Kappe“ erkennt man am Pfeil bzw. am Namen `_off`.
- **Ein eingefrorener Treffer in der Ansicht waehlt nichts** (auch nicht die Flaeche dahinter) und meldet es - wie der Entwurf („Klick auf eine
  eingefrorene Flaeche meldet ...“); 3ds Max wuerde durch Eingefrorenes hindurch waehlen.
- **LOD-Ebenen:** das Auge zeigt die Detailstufe (statt Flaechen auszublenden), keine Schneeflocke; die Flaechen einer LOD-Ebene sind dieselben
  Teile wie in „Body“ / „Caps“. Fehlt die Stufe, steht „This model has no level of detail n“.
- **Sequenzen und Rig-Griffe stehen auch unter „Nach Ebenen“** (ohne Auge/Schneeflocke bzw. mit Auge) - sonst waeren sie dort nicht erreichbar.
- **Gruppe der Sequenzen als Klappliste** unter dem Knoten statt fuenf Knoepfen: passt auch in eine schmale Spalte.
- **Tooltip der Modusleiste** liegt weiter ueber dem Nachbarfeld (rechts neben einer senkrechten Leiste geht es nicht anders), aber unter dessen
  erster Zeile, verzoegert, umbrochen und mit Spitze zum Knopf.
- **Ort im Modus wie Blender** (Outliner oben rechts) und Docking-Layout je Modus: Paket AP4 - hier nur Titel, Umschalter und Zeilenaufbau.
- **Kopieren/Einfuegen** einer Flaeche (X19): Paket AP11 (Modelle in Tabs). Die Gruppe „Auswahl“ im rechten Feld (X18): AP5 - Info hier im
  Rechtsklick und im Tooltip.

## Selbsttest (rot -> grün)

`gui/selbsttest_ap2.inc`, 63 Pruefungen (Lage der gezeichneten Zeilen aus `modellExplorer().lage`: Pfeil, Auge, Symbol, Name, Schneeflocke).
- **Rot** (Lauf `ap2_rot`, alter Explorer mit Stummel-Schnittstelle): 59 von 63 Pruefungen rot, dazu 4 Klicks „nicht gefunden“ (Info, Release the
  root, Filter Tags). Schon vorher gruen: X26 „nicht ueber der Zeile By hierarchy / By layers“ (siehe oben), X3 „By hierarchy / By layers“ im
  Max-Modus (die Zeile X3 betrifft nur die Blender-Woerter - dort rot), und zwei Folgepruefungen, die von selbst wahr waren (X14 „dritter Klick“,
  X11 „geloest“): beide pruefen seitdem auch das Element (Spaltenkoepfe / Ebenenzeile) - dieser schaerfere Stand ist nicht eigens rot gelaufen.
- **Gruen** (Lauf `ap2_g4`, Endstand): 63 OK, 0 FEHLER.
- Dazwischen gefunden und behoben: Tooltips der Umschalter liefen als eine 40 Zeichenhoehen lange Zeile (`SetItemTooltip`) -> umbrochen; im hellen
  Schema schwarze Schrift auf der dunkelblauen Auswahl und ein dunkler Tabellenkopf -> Farben des Farbschemas; im kleinen Fenster 5 Zeilen Fusstext
  ueber 7 Zeilen Baum -> Fusszeilen nur mit Platz; der Filterknopf „Bones“ hiess wie der Knopf der Werkzeugleiste (der Test `modell` klickte den
  falschen) -> „Filter: Bones“; `lint_i18n` nannte „This model has no animations“ als unbenutzt -> die Zeile steht wieder im Baum.

## Angepasste vorhandene Selbsttests (nichts gelöscht)

- `selbsttest_mb1a2.inc`: das Auge einer Zeile aus `modellExplorer().lage` (vorher feste Spalte `modellAugeX`).
- `selbsttest_mbefoto.inc`: Knochen-Knoten über `modellExplorerAufklappen` / `modellExplorerKnochenAlleAuf`, Sequenz im Baum.
- `selbsttest_modell1.inc`, `selbsttest_werkzeuge.inc`: Gruppe TORSO / All über die Klappliste unter „Sequences“ (vorher Knöpfe über der Liste).
- `selbsttest_dock1.inc`: gemessen wird der Explorer statt der Animationsliste.

## Fotos

Angesehen (Ausschnitte der linken Spalte neben dem Entwurfsbild `Y:\werkzeuge\mbe\fotos\ausschnitt\a_max_explorer.png` / `a_blender_explorer.png`):
- wie 3ds Max: nach Hierarchie, nach Ebenen (offen und zugeklappt), Knochen alle offen, Sequenzen mit Marke P und Tooltip, Rig-Griffe, Zustandszeichen,
  Info, Rechtsklick Tag, Tooltip der Modusleiste; wie Blender: Hierarchie, Filter-Ausklapper, Sammlungen.
- helles Schema (`behaved-classic`), kleines Fenster (1328 x 844 bei Oberflaeche 1,0: 12 Zeilen Baum, Fusszeilen entfallen), Deutsch, Chinesisch,
  Japanisch, Spanisch (Tschechisch aufgenommen, Messzeile „0 gekuerzt“ in der Ebenenansicht wie die anderen).
- Ablage: `Y:\werkzeuge\ap2\fotos\selbsttest\*.png` (26, volle Groesse 3818 breit), `...\fotos\programm\p_*.png` (15 Bilder des Foto-Selbsttests
  `mbefoto` mit dem neuen Explorer, gleiche Namen wie in Teil A), Ausschnitte `...\sicht_f\<variante>\`.

## Handbuch

- Eigene Quelle `tools/handbuch/inhalt_ap2.py` (6 Sprachen), eingehaengt in die Seite „Model-Tab: Ansehen und Pruefen“, Abschnitt
  „Szenen-Explorer“: ein Baum, Zeile je Bedienmodus, nach Ebenen / Einfrieren, Hinweis zur Modusleiste; Bilder `h_ap2_max`, `h_ap2_ebenen`,
  `h_ap2_blender` in 6 Sprachen (2538 x 1472, angesehen: en und de).
- Nachgezogen: `inhalt_mb1a1.py` und `inhalt_mission.py` (die Sequenzen stehen im Baum statt in der Liste „Animations“), `inhalt_mb1a2.py`
  („Symbole und Auge“: Zustandszeichen, Auge an der Zeile), Tutorial „Animation abspielen“ (`inhalt_kurz.py`, `inhalt_schritte2.py`,
  `uebersetzung/*/kurz.json`: Suchfeld des Explorers). `gen_handbuch.py --pruefen`: 0 Texte / 0 Bilder noch englisch.
- **Nicht neu aufgenommen** (AP14 bzw. Zusammenfuehrung, weil MBKOPF/MBRASTER dieselben Dateien neu aufgenommen haben): alle uebrigen Bilder des
  Model-Tabs zeigen links noch den alten Explorer mit der Liste „Animations“ (z. B. `h_mb1_ansehen`, `h_mb1_menue`, `h_mb2_auge`, `m_insskript`).

## Schlussrunde

Nach der Regel „Schlussrunde je Zweig ab 11.11.“ (parallel_1011.md): nur Bau, ctest, der eigene Selbsttest und die Gruppen, die den geänderten
Code berühren; die volle Runde (Neubau, alle Gruppen, Modus 1) läuft einmal auf dem zusammengeführten Stand. Alles auf demselben Bau (Endstand).

| Schritt | Ergebnis |
|---|---|
| Bau (inkrementell, alle Ziele, /W4 /WX) | 0 Fehler, 0 Warnungen (`bau_alles.log`) |
| ctest (246) | 245 bestanden; rot nur `q3map2_teilbau` („2 Fehlschläge“, 1201 s) - Stand der Basis (LCB2 arbeitet daran, in den Nachbarzweigen ebenso rot), kein Bezug zu diesem Paket (kein Unterschied in `src/q3map2`, `tests`) |
| eigener Selbsttest `ap2` | 63 OK, 0 FEHLER (`ap2_g4`) |
| modellblock `funktionen` | 61 OK, 0 FEHLER (Explorer: Rechtsklick wie ModView, Suche) |
| modellblock `tasten` | 43 OK, 0 FEHLER (H = Suchfeld) |
| modellblock `mb2` | 37 OK, 0 FEHLER (Symbole, Auge) |
| modellblock `blender` | 67 OK, 0 FEHLER (H / Alt+H / Umschalt+H) |
| modellblock `haut` | 1 OK, 0 FEHLER |
| modellblock `mb3` | 21 OK, 0 FEHLER (Sequenz auf Platz ziehen, Sequenz ins Bild) |
| modellblock `rig` | 26 OK, 0 FEHLER (Griffe im Explorer) |
| modellblock `mb6` | 37 OK, 0 FEHLER (Rechtsklick am Griff) |
| modellblock `mbefoto` | 0 FEHLER (Foto-Lauf aus Teil A mit dem neuen Explorer) |
| Modus `modell` | 14 OK, 0 FEHLER (Sequenz anklicken, Gruppe TORSO, Ins Skript) |
| Modus `dock2mass` | 29 OK, 0 FEHLER (linke Spalte im Andockraum und ohne) |
| Handbuch `gen_handbuch.py --pruefen` | 0 Texte / 0 Bilder noch englisch; Bilder `BHED_HB_NUR=ap2` in 6 Sprachen |
| Lints (alle `tools/lint_*.py`) | keine Meldung zu geänderten Dateien; `lint_i18n` ok (0), `lint_imgui_context` ok, `lint_sources` ok, `lint_endtable` ok, `lint_tabellenid` ok |
| private Adressen in Log und Diff | 0 |

**Bewusst nicht gelaufen** (berühren den geänderten Code nicht bzw. gehören zur vollen Runde): modellblock `bild`, `bild2`, `rundlauf`, `rigrundlauf`,
`mb5`, `kurvenrundlauf`, `ikrundlauf`, `mb7`, `mb7textur`, `ph`, `rt`, `rtjka`, `pr`, `ai`, `uv`, `uvmd3`, `uvrundlauf`, `md`, `mdkarte`, `pt`, `ptfix*`,
`ptstufen`, `pfix*`; Modus 1, `handbuch` ganz, `karteneditor`, `menueeditor`, sauberer Neubau.

**Bildzeit (Richtwert unter Last, keine Endzahl):** `sh messung_final.sh last2`, 11.11. 13:14, in der Messsperre, ohne D3D-Debugschicht, alt
(`Y:\p_ap2\build\alt` = Basis + Selbsttest) gegen neu, ganzer Lauf `modellleistung` (4 Figuren x 5 Lagen, je 120 Bilder); waehrenddessen liefen
16 bis 23 fremde Bau-Prozesse. Die Streuung ist groesser als jeder moegliche Unterschied: der Menue-Editor (unberuehrter Code) misst alt 1,63 / neu
3,32 ms Bild-CPU; stormtrooper ruhig alt 2,19 / neu 1,57, mit Animation alt 2,17 / neu 1,67, jedi_hf ruhig alt 4,78 / neu 9,68 (p99 103: ein
Ausreisser), clonetrooper_p2 Knochen alt 30,22 / neu 7,93; Summe der 23 Mittelwerte alt 130,0 / neu 117,2 ms. Daraus folgt nur: kein grober
Rueckschritt sichtbar. Der neue Baum baut im Alltag nur die sichtbaren Zeilen (rund 30 statt aller rund 150 des alten Baums; die Sequenzen wie
bisher ueber ImGuiListClipper), die Info einer Zeile (zaehlt Ecken und Knochenbezuege) erst unter der Maus. **Endzahl im Ruhefenster:**
`Y:\werkzeuge\ap2\messung_final.sh` (selbststaendig, `RUNDEN=2` fuer alt/neu/alt/neu, je Lauf bis 290 s; Ergebnis `messung_final.txt`).

## Offen

- Ort des Outliners im Modus wie Blender (oben rechts), Docking-Layout je Modus: AP4.
- Kopieren/Einfuegen von Flaechen zwischen Modellen: AP11. Gruppe „Auswahl“ im rechten Feld: AP5.
- Knochen: Mehrfachauswahl (Umschalt/Strg) - die Bereiche Weights/Animation arbeiten heute mit einem Knochen (`knochenWahl`).
- Eigene Ebenen (anlegen, umbenennen, Zeilen hineinziehen) - der Entwurf nennt nur die festen Ebenen.
- Uebrige Handbuchbilder des Model-Tabs (siehe Handbuch).
- Die zwei nachgeschaerften Folgepruefungen (X14, X11) sind nicht eigens gegen den alten Stand rot gelaufen.

## Changelog

## English

- Model tab, scene explorer: one tree for everything - the model, its surfaces with their hierarchy, tags with attached models, bones, rig handles and the sequences of the animation.cfg (the separate animation list is gone). The column is as wide as in the approved design.
- Like 3ds Max: the arrow, the eye and the symbol stand directly in front of the name, each level is indented by one symbol, a "Frozen" column with a snowflake, column headings that sort and can be dragged. Like Blender: an outliner with the eye and the "selectable" arrow in columns on the right and a filter popover; the switch reads Hierarchy / Collections.
- Freezing: frozen surfaces, tags and bones stay visible but cannot be selected in the tree or in the view; the caps are frozen from the start. "By layers" shows Body, Caps, Tags, Bones, Attachments and the further LODs, each with an eye and a snowflake.
- Sequences in the tree show the lock P / S for the primary and secondary sequence and a warning when their frames lie behind the end of the GLA; group list, copy the name, into the script.
- The tree never scrolls sideways: deep bone chains move closer together, long names end with "..." and stand in full in the tooltip together with the info of the row. Filter by kind, Shift+click selects a range, "Info" in the right-click menu.
- The tooltips of the mode bar on the left no longer pop up over the neighbouring panel: they appear after a short rest, next to the bar at the height of the button.

## Deutsch

- Model-Tab, Szenen-Explorer: ein Baum für alles - das Modell, seine Flächen mit ihrer Hierarchie, Tags mit angebauten Modellen, Knochen, Rig-Griffe und die Sequenzen der animation.cfg (die eigene Animationsliste entfällt). Die Spalte ist so breit wie im freigegebenen Entwurf.
- Wie 3ds Max: Pfeil, Auge und Symbol stehen direkt vor dem Namen, jede Ebene ist um ein Symbol eingerückt, Spalte „Eingefroren“ mit Schneeflocke, Spaltenköpfe zum Sortieren und Ziehen. Wie Blender: ein Outliner mit Auge und Pfeil „wählbar“ rechts in Spalten und einem Filter-Ausklapper; der Umschalter heißt Hierarchie / Sammlungen.
- Einfrieren: eingefrorene Flächen, Tags und Knochen bleiben sichtbar, lassen sich aber in Baum und Ansicht nicht wählen; die Kappen sind von Anfang an eingefroren. „Nach Ebenen“ zeigt Körper, Kappen, Tags, Knochen, Anbauten und die weiteren LODs, jede mit Auge und Schneeflocke.
- Sequenzen im Baum tragen das Schloss P / S für Primär- und Sekundärsequenz und ein Warnzeichen, wenn ihre Bilder hinter dem Ende der GLA liegen; Gruppenliste, Namen kopieren, ins Skript.
- Der Baum rollt nie seitwärts: tiefe Knochenketten rücken enger zusammen, lange Namen enden mit „...“ und stehen ganz im Tooltip, zusammen mit der Info der Zeile. Filter nach Art, Umschalt+Klick wählt einen Bereich, „Info“ im Rechtsklick-Menü.
- Die Tooltips der Modusleiste links springen nicht mehr über das Nachbarfeld: sie erscheinen nach kurzer Ruhe neben der Leiste auf Höhe des Knopfs.

## 中文

- 模型选项卡,场景资源管理器:一棵树包含一切——模型、带层级的面、带挂接模型的标签、骨骼、绑定控制柄以及 animation.cfg 的序列(单独的动画列表已取消)。该列与已批准的设计一样宽。
- 与 3ds Max 相同:箭头、眼睛和符号紧挨在名称前,每一级缩进一个符号,带雪花的“已冻结”列,可排序、可拖动的列标题。与 Blender 相同:大纲视图,眼睛和“可选”箭头位于右侧的列中,并带筛选弹出面板;切换按钮为 层级 / 集合。
- 冻结:冻结的面、标签和骨骼保持可见,但在树和视图中都无法选择;封口从一开始就是冻结的。“按图层”显示身体、封口、标签、骨骼、挂接物和其他 LOD,每层都有眼睛和雪花。
- 树中的序列用锁 P / S 标出主序列和次序列,当帧位于 GLA 末尾之后时显示警告;分组列表、复制名称、写入脚本。
- 树从不横向滚动:较深的骨骼链会排得更紧,较长的名称以“...”结尾,并与该行的信息一起完整显示在工具提示中。按种类筛选,Shift+单击选择一个范围,右键菜单中有“信息”。
- 左侧模式栏的工具提示不再弹到相邻面板上:鼠标稍作停留后,它出现在栏旁、与按钮等高的位置。

## 日本語

- モデルタブのシーンエクスプローラー: すべてを 1 本のツリーに - モデル、階層付きのサーフェス、取り付けたモデル付きのタグ、ボーン、リグのハンドル、animation.cfg のシーケンス (別のアニメーション一覧はなくなりました)。列の幅は承認済みのデザインと同じです。
- 3ds Max と同じ: 矢印・目・記号が名前のすぐ前に並び、階層ごとに記号 1 つ分だけ字下げ、雪の結晶の「フリーズ」列、並べ替えとドラッグができる列見出し。Blender と同じ: 目と「選択可能」の矢印が右の列にあるアウトライナーとフィルターのポップオーバー。切り替えは 階層 / コレクション。
- フリーズ: フリーズしたサーフェス、タグ、ボーンは表示されたままですが、ツリーでもビューでも選択できません。キャップは最初からフリーズされています。「レイヤー順」はボディ、キャップ、タグ、ボーン、取り付け、その他の LOD を、それぞれ目と雪の結晶付きで表示します。
- ツリーのシーケンスにはプライマリ / セカンダリを示す錠 P / S と、フレームが GLA の終わりを超える場合の警告が付きます。グループ一覧、名前のコピー、スクリプトへ。
- ツリーは横にスクロールしません: 深いボーンの連なりは間隔が詰まり、長い名前は「...」で終わって、行の情報と一緒にツールチップに全体が表示されます。種類でフィルター、Shift+クリックで範囲選択、右クリックメニューに「情報」。
- 左のモードバーのツールチップが隣のパネルの上に飛び出さなくなりました: 少し止まると、バーの横・ボタンの高さに表示されます。

## Español

- Pestaña Model, explorador de escena: un árbol para todo: el modelo, sus superficies con su jerarquía, tags con modelos acoplados, huesos, controles del rig y las secuencias del animation.cfg (desaparece la lista de animaciones aparte). La columna es tan ancha como en el diseño aprobado.
- Como 3ds Max: la flecha, el ojo y el símbolo están justo delante del nombre, cada nivel se sangra un símbolo, columna «Congelado» con copo de nieve, encabezados de columna que ordenan y se pueden arrastrar. Como Blender: un listado con el ojo y la flecha «seleccionable» en columnas a la derecha y un desplegable de filtro; el conmutador dice Jerarquía / Colecciones.
- Congelar: las superficies, tags y huesos congelados siguen visibles, pero no se pueden seleccionar en el árbol ni en la vista; las tapas están congeladas desde el principio. «Por capas» muestra Cuerpo, Tapas, Tags, Huesos, Acoples y los demás LOD, cada una con ojo y copo de nieve.
- Las secuencias del árbol llevan el candado P / S de la secuencia primaria y secundaria y una advertencia cuando sus fotogramas quedan detrás del final del GLA; lista de grupos, copiar el nombre, al script.
- El árbol nunca se desplaza de lado: las cadenas de huesos profundas se juntan más, los nombres largos terminan en «...» y aparecen enteros en la descripción emergente junto con la información de la fila. Filtro por tipo, Mayús+clic selecciona un rango, «Información» en el menú del clic derecho.
- Las descripciones emergentes de la barra de modos de la izquierda ya no saltan sobre el panel de al lado: aparecen tras una breve pausa, junto a la barra a la altura del botón.

## Čeština

- Karta Model, průzkumník scény: jeden strom pro všechno - model, jeho povrchy s hierarchií, tagy s připojenými modely, kosti, úchyty rigu a sekvence z animation.cfg (samostatný seznam animací zmizel). Sloupec je tak široký jako ve schváleném návrhu.
- Jako 3ds Max: šipka, oko a symbol stojí přímo před názvem, každá úroveň je odsazena o jeden symbol, sloupec „Zmrazeno“ s vločkou, záhlaví sloupců, která řadí a dají se táhnout. Jako Blender: outliner s okem a šipkou „lze vybrat“ ve sloupcích vpravo a s vyskakovacím filtrem; přepínač se jmenuje Hierarchie / Kolekce.
- Zmrazení: zmrazené povrchy, tagy a kosti zůstávají viditelné, ale ve stromu ani v pohledu je nelze vybrat; záslepky jsou zmrazené od začátku. „Podle vrstev“ ukazuje Tělo, Záslepky, Tagy, Kosti, Připojené a další LOD, každou s okem a vločkou.
- Sekvence ve stromu nesou zámek P / S pro primární a sekundární sekvenci a výstrahu, když jejich snímky leží za koncem GLA; seznam skupin, kopírování názvu, do skriptu.
- Strom se nikdy neposouvá do strany: hluboké řetězce kostí se srazí blíž k sobě, dlouhé názvy končí „...“ a celé stojí v bublině spolu s informacemi o řádku. Filtr podle druhu, Shift+klik vybere rozsah, „Informace“ v nabídce pravého tlačítka.
- Bubliny lišty režimů vlevo už nevyskakují přes sousední panel: objeví se po krátké pauze vedle lišty ve výšce tlačítka.
