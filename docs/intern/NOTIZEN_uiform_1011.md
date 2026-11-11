# Notizen UIFORM (10.11.): Fenster und Formulare gleichmaessig, Mindestgroessen - rc612

shank 10.11. mit Bild des Fensters "Into script": "das muss sauberer gestaltet werden das Fenster. Damit es gleichmaessiger aussieht" -
und mit dem Bild des auf ~135 px zusammengeschobenen Fensters: "und mindestgroessen muessen gecheckt werden von solchen fenstern. Das
bei allen". Zweig `p_uiform` (auf karte1 a00f6cf90). Begonnen von einem Agenten auf Opus (ein WIP-Commit, nie gebaut), fortgesetzt und
ueberarbeitet auf Fable (10.11.), Panel-Anhebung gefunden und behoben auf Opus (11.11. frueh), geprueft und abgeschlossen auf Fable
(11.11.). Hilfsskripte und Protokolle: `Y:\werkzeuge\uif\`.

## Punkt 0: nachgestellt (rot, alter Stand)

Der Selbsttest `BHED_EDITORTEST=uiform` misst am Element-Register und an ImGuis Fenstern, nicht am Baustein; ohne `ui_formular.h`
uebersetzt er auf dem alten Stand. Rot auf a00f6cf90 (Zweig `uif_rot`, Laeufe `uif_rot_ins`, `uif_rot_min`, `uif_rot_fotos`):

- **Into script** (1 OK, 21 FEHLER): linke Feldkanten 910 / 913 / 909 / 878 (Animation, Befehl, Figur, Skript), rechte Kante der
  Figur 1808 statt 1811, Pfeil der Figur 12 Punkte abgesetzt, kein Knopf im Feld, kein Hinweis unter dem Feld, kein Code-Kasten,
  kein Abbrechen. Auf 1 x 1 gesetzt: 1015 x 383 -> 48 x 1324 (die Vorschau bricht Zeichen fuer Zeichen um). In einem 288 Punkte
  schmalen Knoten: Feld 174 Punkte, kein Rollbalken. Enter fuegt nicht ein, Esc schliesst nicht.
- **Mindestgroessen** (46 OK, 24 FEHLER): NPCs 528 x 720 -> 48 x 48, Rig overview 816 x 624 -> 48 x 48, Select Font 1000 x 760 ->
  48 x 48, Undo history 960 x 832 -> 48 x 48, Into script -> 48 x 1324. Fenster mit eigener Grenze (Game folders, Keyboard shortcuts,
  Messages, Scripts together, Files in .pk3) blieben auf ihrer.
- Fotos vorher: `Y:\werkzeuge\uif\fotos\vorher_*.png` (1920 breit, angesehen): Into script mit vier verschiedenen Feldkanten und der
  48 Punkte breite Streifen.

## Punkt 1: der Formular-Baustein (`gui/ui_formular.h/.cpp`)

- `ui::Formular({beschriftungen...}, mindestFeld = 14 Zeichenhoehen)`: eine Beschriftungsspalte (Breite aus der laengsten Beschriftung
  der laufenden Sprache, `cjkTextSize`), `zeile()` setzt die Beschriftung auf die Feldmitte (`AlignTextToFramePadding`) und das Feld an
  die gemeinsame Kante, Breite bis zum rechten Rand, nie unter der Mindestbreite. `zeileOhne()` (Feld ohne Beschriftung),
  `feldKnopf()` (kleiner Knopf mit Symbol und Tooltip IM Feld, links vom Pfeil des Ausklappers), `eingabeMitListe()` (Eingabe und
  Pfeil ohne Abstand - ein Feld wie der Ausklapper, die Liste darunter in Feldbreite), `hinweis()` / `fehler()` unter dem Feld,
  `codeVorschau()` (Kasten im Feld-Stil, Festbreitenschrift des Code-Editors, so hoch wie die Zeilen; zu lange Zeilen rollen
  waagrecht), `knoepfe()`.
- `ui::knopfReihe(haupt, abbrechen, ...)`: unten rechts, Hauptknopf in der Betonungsfarbe des Themas (`ImGuiCol_CheckMark`, Text nach
  Helligkeit weiss oder schwarz), Enter loest ihn aus, Esc bricht ab - im Eingabefeld nimmt Esc erst die Eingabe zurueck.
- Passt zu hellem und dunklem Thema (nur Farben des Themas), skaliert mit der Schriftgroesse (alle Masse in Zeichenhoehen).

## Punkt 2: Into script

`gui/app_modelltab_mb2.cpp` mit dem Baustein: Animation (Ausklapper, im Feld der Knopf "gewaehlte Animation nehmen" mit Tooltip),
Befehl (bei den Haltezeiten darunter die Dauer), Figur (ein Feld mit Liste, Hinweis "leer: ohne affect" darunter), Skript, die Zeile im
Code-Kasten, Meldung (Fehler rot), [Insert] (hervorgehoben) [Cancel] - nach dem Einfuegen heisst der zweite Knopf "Close". Die Hoehe
folgt schwebend dem Inhalt.

## Punkt 3: die Fenster des Programms

Liste: `docs/intern/ui_formulare_liste.md` (97 Begin-Stellen, erzeugt mit `Y:\werkzeuge\uif\liste.py` und `liste_md.py`). Auf den
Baustein umgestellt: Into script. Knopfreihe unten rechts (Hauptknopf hervorgehoben, Enter/Esc): Neue Mission, Mission speichern,
Modell anbauen (nur Abbrechen). Die Dialoge des Karten-Editors sind Nachbauten von NetRadiant-Custom mit Qt-Massen und bleiben.

Suche nach demselben Fehlerbild (`Y:\werkzeuge\uif\muster.py`: Beschriftung als Text, `SameLine()` ohne feste Spalte, dann ein Feld -
mindestens zwei solche Zeilen hintereinander, also ungleiche Feldkanten): ausser Into script nur noch `gui/app_modelltab_mb3.cpp:771`,
die Rollouts des rechten Befehlsfelds (MBENTWURF, hier nicht umgebaut). Die uebrigen Formulare setzen ihre Felder schon an eine feste
Spalte (`SameLine(breite)`, z. B. `app_kamera.cpp`) oder stehen als eigene Tabelle (Ereignis-Editor).

## Punkt 3a: Mindestgroessen

Alle 97 Stellen beginnen ueber `ui::fensterBegin` / `ui::modalBegin`. Regeln (ui_formular.h):

- **Keine eigene Grenze** fuer Fenster, deren Groesse das Programm bestimmt: `AlwaysAutoResize` (35 Stellen), `NoResize` oder
  `SetNextWindowSize` in jedem Bild (19 Stellen - die Dialoge nach NetRadiant-Custom). Sie sind genau wie vorher.
- Sonst **Titel** (Knoepfe, hoechstens 12 Zeichenhoehen Text, wenigstens 10 Zeichenhoehen breit, eine Zeile Inhalt) und **was der
  Inhalt meldet** (Formular: Beschriftung + Mindestfeld + Rand, Hoehe bis unter die Knopfreihe; Knopfreihe).
- In Fenstern mit **Fliesslayout** (33 Stellen: mit senkrechtem, ohne waagrechten Rollbalken) dazu die **Bedienreihen**: ein Element,
  das rechts neben einem anderen steht, oder ein gerahmtes Element (Knopf, Feld, Ausklapper, Haken), das nicht am Zeilenanfang
  beginnt, muss ganz hineinpassen; ein Eingabefeld, das mit dem Fenster schrumpft, behaelt 6 Zeichenhoehen. Listeneintraege,
  Baumknoten und Text verlangen nichts. Gemessen wird ueber die Haken des Element-Registers, und nur in den Bildern, in denen etwas
  rechts hinausragt und der Zustand (Fensterbreite, Inhaltsbreite) noch nicht gemessen ist.
- Was die Bedienreihen verlangen, gilt **nie ueber die groesste Groesse hinaus**, die das Fenster in der Sitzung schon hatte: kein
  Fenster wird dadurch groesser als vorher. Waechst der Bedarf mit dem Fenster mit (dreimal in Folge ragt nach dem Anheben wieder
  etwas hinaus), wird fuer das Fenster nichts mehr angehoben.
- Fenster **ohne Rollbalken**, die ihren Platz selbst aufteilen (8 Stellen: Panels und Tabs in eigenen Fenstern, Zeichenflaechen),
  bekommen Titel und gemeldeten Inhalt - keine gemessenen Reihen (frei gesetzte Elemente hielten sie sonst fest).
- **Im Dock-Knoten keine Mindestgroesse** (Punkt 3b): legt ein Dock-Knoten das Fenster an (angedockt, die Panels im DockSpace der
  Bereiche), bestimmt die Anordnung seine Groesse - ein schmal oder eingeklappt gestelltes Panel bleibt so. Ist der Knoten schmaler,
  als Inhalt oder Bedienreihen brauchen, bekommt ein Fenster mit Fliesslayout einen waagrechten Rollbalken.
- Gespeicherte zu kleine Groessen hebt ImGui im ersten Bild auf Titel und gemeldeten Inhalt an (die Grenze gilt in jedem `Begin`).

### Was am Stand des Vorgaengers geaendert wurde

Der Vorgaenger hob die Mindestgroesse aus dem "Ueberlauf" an (`CursorMaxPos` ueber dem Rand: rechts bei jedem Fenster ohne
waagrechten Rollbalken bis 45 Zeichenhoehen, unten bei jedem Fenster ohne Rollbalken). Das haette Fenster aufgeblaeht:

- Lange Listen- und Baumeintraege zaehlen in `CursorMaxPos` mit: ein Fenster mit einer Liste liesse sich nicht mehr schmaler
  ziehen als sein laengster Eintrag (bis 45 Zeichenhoehen) - auch groesser, als es vorher war.
- Fenster ohne Rollbalken wuechsen um jeden Ueberlauf unten: die Dialoge nach NetRadiant-Custom (Qt-Masse), Panels, Tabs.
- `AlwaysAutoResize`-Fenster bekaemen die Titel-Untergrenze (10 Zeichenhoehen): kleine Rueckfragen wuerden breiter.
- Angedockte Fenster mit langen Eintraegen bekaemen einen waagrechten Rollbalken.

Neu: keine Grenze fuer programm-bemessene Fenster, keine Messung der Hoehe, in der Breite nur Bedienreihen (nicht Listen und Text),
gedeckelt auf die groesste Groesse der Sitzung, mit Schutz gegen Mitwachsen. Dazu am Baustein: Eingabe mit Liste als Eingabe + Pfeil
ohne Abstand (der Text endete sonst unter dem Pfeil), Code-Kasten nur so hoch wie die Zeilen (vorher immer mit Platz fuer den
Rollbalken), `mindestInhalt` ohne den doppelten Rand (waere 2 Punkte hoeher gewesen als das Fenster selbst), die Messstruktur des
Bausteins entfernt (der Test misst unabhaengig am Element-Register und an den Glyphen der Zeichenliste).

## Punkt 3b: Panels wuchsen beim Ein- und Ausklappen eines Felds (11.11.)

Der erste volle karteneditor-Lauf (10.11., abgebrochen) schrieb 27 Zeilen "UIF hebt" fuer Panels des Karten-Editors (Kamera, XY, XZ,
YZ, Konsole, Texturen, Entities), z. B. 294 x 624 -> 320 x 624 und 1954 x 52 -> 1954 x 80: die Titel-Untergrenze (dort 320 x 80) machte
schmal oder eingeklappt gestellte Panels groesser.

- **Ursache** (imgui.cpp gelesen): klappt man ein Feld ein oder aus, baut der Bereich seine Anordnung mit dem DockBuilder neu und setzt
  die Panels per `SetNextWindowDockID` in die neuen Knoten. Kindknoten bekommen ihren Wirt erst, wenn der DockSpace sich das naechste
  Mal aktualisiert (`DockNodeUpdate`: `node->HostWindow = ... ParentNode->HostWindow`); bis dahin laesst `BeginDocked` das Fenster
  ungedockt ("Fast path return", `node->HostWindow == NULL`), `DockIsActive` bleibt falsch und ImGui wendet die Groessengrenzen an
  (angedockt setzt es sie ausser Kraft: Begin, "Docking currently override constraints").
- **Behebung** (`gui/ui_formular.cpp`, `knotenLegt`): legt ein Dock-Knoten das Fenster an - es ist angedockt, oder sein Knoten bzw. der
  Knoten aus `SetNextWindowDockID` hat einen Wirt oder haengt an einem DockSpace -, setzt UIFORM keine Untergrenze (eine eigene Grenze
  der Stelle bleibt wie vor UIFORM). Verworfen: Fenster mit `NoScrollbar` ausnehmen - das traefe auch Panels und Tabs in eigenen
  Fenstern und waere eine Heuristik statt der Ursache.
- **Rot / gruen** (`karteneditor BHED_KARTE_NUR=stufe3`): ohne die Behebung 223 OK / 1 FEHLER mit 27 Zeilen "UIF hebt ... im
  Dock-Knoten" (Lauf uif_r_ke3), mit ihr 224 OK / 0 FEHLER und keine Zeile (uif_g_ke3). Dazu am Ende JEDES Selbsttests die Pruefzeile
  "Mindestgroessen: kein Fenster im Dock-Knoten angehoben" (`ui::knotenAnhebungen`) - jeder Modus hat dadurch eine OK-Zeile mehr.
- **Trifft nur, was ein Knoten anlegt**: ein schwebender Knoten mit einem einzigen Fenster hat keinen Wirt (ImGui `DockNodeUpdate`:
  "Floating window pos/size is authoritative") - dort gilt die Untergrenze weiter. Gruppe `schwebend` des Selbsttests: das Panel XY
  (Map Editor, `dockvorgabe.cpp`), das Panel Script Flow (Mission, `app_dockspace.inc`) und der Tab Model (`tabfenster.cpp`) in
  eigenen Fenstern landen auf 1 x 1 gesetzt auf 320 x 80, 240 x 76 und 240 x 120 (Gegenprobe ohne Mindestgroessen: 48 x 48, 48 x 48,
  48 x 67); wieder angedockt haben die Panels die Groesse ihres Platzes (910 x 1194, 540 x 1064). Lauf uif_g5_schweb 7 OK / 0 FEHLER.
- **Die 6 Zeilen aus Modus 1** (Game folders auf 960 x 384 und 1392 x 384, Keyboard shortcuts 768 x 480, Files in .pk3 864 x 432,
  Messages 480 x 192, Scripts together 720 x 336, je von 40 x 40 bzw. 1392 x 288) kamen nicht von UIFORM: diese Fenster setzen ihre
  Grenze selbst (`SetNextWindowSizeConstraints` mit 40 x 16, 32 x 20, 36 x 18, 20 x 8 und 30 x 14 Zeichenhoehen - bei Schrift 24 genau
  diese Masse; so schon auf a00f6cf90 in `gui/app.cpp` und `gui/app_anordnung.inc`), die Untergrenze von UIFORM war dort 240 x 88.
  Die Zeile "UIF hebt" zaehlt deshalb nur noch, wenn die Mindestgroesse von UIFORM ueber der eigenen Grenze der Stelle liegt.
- **Grenze**: ein schwebender Knoten mit mehreren Fenstern (Reiter) ist ein Fenster von ImGui; seine kleinste Groesse ist ImGuis, die
  Fenster darin rollen waagrecht (Selbsttest insskript: 12 Zeichenhoehen schmaler Knoten, Felder in Mindestbreite).

## Probefenster: die Messung der Bedienreihen (11.11.)

Die Haken der Bedienreihen waren bis dahin in keinem Lauf angesprungen ("Reihen 0"). Gruppe `probe` mit Fenstern bekannten Inhalts:

- Knopfreihe (drei Knoepfe), Knopf hinter Text, lange Listeneintraege, Baumknoten und Text: auf 1 x 1 gesetzt genau so breit, wie
  Knopfreihe und Knopf brauchen (645 x 88), nicht so breit wie der lange Eintrag; Gegenprobe ohne Mindestgroessen: 48 x 48.
- Ein Fenster, das nie so breit war wie seine Reihe, wird nicht groesser (240 x 88); mitwachsender Inhalt: drei Anhebungen, dann Ruhe
  (420 x 88).
- **Echter Fehler, behoben**: ein Eingabefeld, das ganz hinter dem Rand lag (Breite 1), behielt seine 6 Zeichenhoehen nicht - ImGui
  schneidet es nach dem ItemAdd ab, `InputTextEx` kehrt vor seinem ItemInfo-Haken zurueck, die Regel hing aber an diesem Haken.
  Jetzt beim ItemAdd ueber `LastItemData.ItemFlags & ImGuiItemFlags_Inputable` (`hakenArt` entfernt). Rot 4 OK / 2 FEHLER (uif_probe),
  gruen: Fenster 467 x 88, Feld ganz zu sehen.
- **Testfehler, behoben**: die Gegenprobe fand ihre Knoepfe nicht ("Knoepfe nicht gefunden", auch auf 16 Zeichenhoehen: 5 OK / 1
  FEHLER, uif_g_probe). Nicht weil ImGui sie nicht anlegte - ein ganz abgeschnittener Knopf hat im Element-Register keine
  Beschriftung (kein ItemInfo). Die Probefenster suchen jetzt ueber die Kennung, die Gegenprobe schiebt wieder auf 1 x 1 wie die
  Probe selbst. Lauf uif_g5_probe 6 OK / 0 FEHLER.

## Zwei Zeitrennen in vorhandenen Selbsttests (11.11., nicht von UIFORM)

Im abgebrochenen Pool vom 10.11. (vier Selbsttests zugleich, daneben Bauten anderer Agenten) meldeten karteneditor und menueeditor je
einen Fehlschlag, einzeln liefen beide gruen. Beide Tests sind gegen a00f6cf90 unveraendert gewesen; beide Rennen stecken im Test.

- **karteneditor "Cancel build"** ("Klick kam nicht an", "q3map2 beendet"): der Selbsttest laeuft in den Testlaeufen mit
  Leerlauf-Prioritaet (lauf.py), q3map2 startet der Editor mit normaler (`karte_bau_win32.cpp`) und allen Kernen ("32 threads"). Im
  Protokoll von uif_ke steht "build finished: Test langsam, 1 step, 62.3 s" VOR "Klick Cancel build" - das Licht war fertig, der
  Menuepunkt gesperrt. Allein laufend kommt der Klick nach 0,7 bis 1,0 s ("build cancelled ... 0.7 s", uif_r_ke3 / uif_g_ke3).
  Behoben im Test: der Eintrag "Test langsam" rechnet mit `-threads 1` (dem Editor bleiben die uebrigen Kerne); einzeln gruen
  ("build cancelled ... step 1 of 1"), unter Last siehe Schlussrunde. **Nicht nachgestellt**: das Rennen braucht einen Editor, der
  in genau diesem Augenblick langsam ist. Vier Laeufe zugleich mit dem alten Test (Leerlauf, daneben Bauten anderer Agenten) kamen
  in 9 Minuten nur bis Pruefzeile 55 von 224 (Abbrechen ist die 167.) und wurden beendet; ein einzelner Lauf mit dem alten Test
  brach nach 0,9 s ab (uif_rot_cancel). Belegt sind also die Reihenfolge im Protokoll vom 10.11. und die beiden Prioritaeten, nicht
  ein Rotlauf auf Bestellung.
- **Drittes Rennen, nur beobachtet** (nicht UIFORM, an TESTTEMPO): in diesen fuenf Laeufen im Leerlauf schlugen dieselben zwei
  Zeilen fehl - "Maus 3D: Strg + Maus im Freelook = Strafe" und "Strg+Umschalt ..." (im Protokoll "y 0, yaw 0": keine Bewegung).
  Dasselbe Programm bestand sie im Lauf davor (uif_g_ke3), die spaeteren Baue in drei Laeufen ebenso. Gegenprobe mit dem Programm
  des Stands a00f6cf90 (ohne UIFORM, Lauf uif_basis_ke3, zur selben Zeit): dieselben zwei Fehlschlaege. Dazu passt die Regel des
  Freelook (`freelookEndetBeimLoslassen`, wie NRC): der Test schaltet ihn mit einem kurzen Rechtsklick ein; liegen zwischen Druecken
  und Loslassen 300 ms oder mehr (ein langsames Bild genuegt), endet er beim Loslassen wieder. `gui/app_karte_cam.cpp` und
  `src/karte` sind in diesem Zweig unveraendert.
- **menueeditor "M5b ... Sicherung weg"**: nach dem Wiederherstellen ist das Menue ungespeichert; der Sicherungstakt (alle 30 s,
  `sicherungPflegen`) schreibt es wieder in denselben Ordner `.jksautosave`. Faellt der Takt zwischen Klick und Pruefung (0,3 s Pause
  und die Bilder dazwischen, unter Last mehr), gibt es den Ordner wieder und die alte Pruefung `!exists` schlug fehl. Welche der drei
  Bedingungen am 10.11. fehlschlug, schrieb die alte Pruefung nicht ins Protokoll - der Takt ist der belegte Weg, ein gescheitertes
  Loeschen waere der andere. Die Pruefung merkt sich jetzt die Schreibzeit der gefundenen Sicherung: weg oder neu geschrieben ist
  gut, noch die alte ist ein Fehler (mit Protokollzeile, was vorlag). Neu dazu: `sichereJetzt()` (was der Takt tut) schreibt das
  wiederhergestellte Menue wieder in die Sicherung - der Weg des Rennens steht damit in jedem Lauf. `BHED_MENUE_NUR=m5b`: 50 OK / 0
  FEHLER (uif_g_m5b; vorher 49).

## Selbsttest

`BHED_EDITORTEST=uiform`, `BHED_UIF_NUR=probe|schwebend|insskript|mindest|fotos` (`gui/selbsttest_uiform.inc`):

- `insskript`: je Sprache (en, de, zh, ja, es, cs) Feldkanten links und rechts gleich (+-1), Beschriftung auf der Feldmitte (+-1,
  aus den Glyphen der Zeichenliste - ImGui meldet Text nicht ans Register), Figur als ein Feld, Knopf im Feld, Hinweis, Code-Kasten
  in Festbreitenschrift, Knoepfe unten rechts; auf 1 x 1: Felder in Mindestbreite, Knopfreihe ganz sichtbar, nichts rollt; schmal
  angedockt: waagrechter Rollbalken, Felder in Mindestbreite; Enter fuegt ein, Esc schliesst.
- `mindest`: die Fenster aus Events, Einstellungen, About, Files in .pk3, Model und Map Editor - in Ruhe nicht groesser als ohne
  Mindestgroessen (`ui::mindestSchalter`), je Sprache auf 1 x 1: nicht kleiner als Titelzeile und eine Zeile, Bedienreihen ganz im
  Fenster, nicht groesser als vorher, ruhig (kein Pumpen), am Ende so gross wie am Anfang.
- `probe` und `schwebend`: siehe oben.
- Im Selbsttest schreibt jede Anhebung eine Zeile "UIF hebt <Fenster> von ... auf ..." ins Protokoll (nur wenn die Mindestgroesse von
  UIFORM ueber der eigenen Grenze der Stelle liegt; ", im Dock-Knoten", wenn ein Knoten das Fenster anlegt): die vollen Laeufe zeigen
  damit, wo die Mindestgroesse ein Fenster groesser macht.

Gruen einzeln (11.11.): insskript 23 OK / 0 FEHLER (uif_g4_ins), mindest 81 / 0 (uif_g4_min), probe 6 / 0, schwebend 7 / 0. Gemessene
Mindestgroessen (en, Schrift 24): Into script 479 x 365 (Formular), NPCs und Rig overview 240 x 88; Fenster mit eigener Grenze bleiben
auf ihrer.

## Handbuch

- Seite "Fenster anordnen" (`inhalt_dock.py`, es/cs in `inhalt_es.py` / `inhalt_cs.py`): Fenster haben eine Mindestgroesse; angedockte
  Fenster und die Panels einer Anordnung behalten die Groesse ihres Platzes, ist er zu schmal, rollt so ein Fenster.
- Seite "Bedienung" des Model-Tabs (`inhalt_mb1a2.py`): Into script als Formular, Knopf im Feld der Animation, Enter/Esc; Bild
  `h_mb2_insskript` neu in 6 Sprachen (`BHED_HB_NUR=mb1a2`, 3840 x 2160 @ 2).
- Missionsfenster mit der Knopfreihe unten rechts neu in 6 Sprachen aus dem grossen Bildlauf (`Y:\werkzeuge\uif\hb_mission.sh`,
  Missionen auf X:, `BHED_FOTO_NUR=h_mission_,t4_1,t4_4,k_pruefen,e_ent_paket,e_rof_paket`, `BHED_HB_BIS=t4_4`, 3840 x 2160 @ 2):
  `h_mission_neu`, `t4_1`, `h_mission_speichern`, `t4_4`, `k_pruefen`, `e_ent_paket`, `e_rof_paket`.
- `gen_handbuch.py` und `--pruefen`: ok (177 Seiten x 6 Sprachen, es/cs 0 Texte und 0 Bilder noch englisch).
- Angesehen: `h_mb2_insskript` in allen 6 Sprachen (2560 x 1411: die Beschriftungsspalte folgt der Sprache, nichts abgeschnitten)
  und die Missionsfenster in allen 6 Sprachen (Knopfreihe unten rechts, Hauptknopf hervorgehoben, Marken an den Knoepfen). Die
  Missionsfenster sind Ausschnitte der Dialoge im Massstab der bisherigen Bilder (New mission 1054 x 917 statt 988 x 910, Save
  mission 1240 x 666 statt 1239 x 659) - `BHED_HB_FENSTER` vergroessert die Dialoge des grossen Bildlaufs nicht.
- Der grosse Bildlauf legt auch `h_mission_wahl` und die Symboltafel (`ikonen`, daraus die `i_*.png`) ab; beide bleiben, wie sie
  waren (`Y:\werkzeuge\uif\hb_bilder.sh` nimmt nur die Bilder der geaenderten Fenster).

## Schlussrunde

Nach der Regel "Schlussrunde je Zweig ab 11.11." (`auftraege/parallel_1011.md`): Bau ohne Warnungen, ctest, der eigene Selbsttest
und die Gruppen, die den geaenderten Code beruehren. Die volle Runde (karteneditor voll, menueeditor voll, alle modellblock-Gruppen)
laeuft einmal auf dem zusammengefuehrten Stand.

| Schritt | Lauf | Ergebnis |
|---|---|---|
| Bau (sauberer Neubau, vor der Regelaenderung gestartet; /W4 /WX) | `bau_sauber.txt` | 0 Fehler, 0 Warnungen |
| ctest `-j 16` | `ctest.txt` | 245 von 246 bestanden; rot ist `q3map2_teilbau` (2 Faelle des Licht-Cache: "zweiraum, -bounce 2") - der Stand von LCB2 auf a00f6cf90; q3map2, seine Werkzeuge und die Tests sind in diesem Zweig unveraendert |
| uiform, alle Gruppen (probe, schwebend, insskript, mindest) | uif_s_uiform | 114 OK / 0 FEHLER |
| uiform fotos | uif_s_fotos | 1 / 0; die 8 Fotos sind bildpunktgleich mit den angesehenen |
| Modus 1 | uif_s_m1 | 2636 / 0, keine Zeile "UIF hebt" |
| karteneditor `stufe3` | uif_s_ke3b | 224 / 0, keine Zeile "UIF hebt", Abbrechen nach 1,2 s |
| karteneditor `stufe3` unter Last (Leerlauf-Prioritaet, neben Modus 1, uiform und den Docks) | uif_s_ke3 | 222 / 3, keine Zeile "UIF hebt", Abbrechen nach 5,6 s gruen; die 3 Fehlschlaege kommen von anderen Programmen auf dem Rechner (siehe unten) |
| dock1 | uif_s_dock1 | 99 / 0 |
| dock4 | uif_s_dock4 | 116 / 0 |
| tabfenster | uif_s_tabf | 22 / 0 |
| menueeditor `m5b` | uif_s_m5b | 50 / 0 |
| modellblock `mb2` (Into script) | uif_s_mb_mb2 | 38 / 0 |
| modellblock funktionen + tasten (Modell anbauen) | uif_s_mb_fkt | 105 / 0 |
| Handbuchbilder | uif_hb_<sprache>, uif_hbm_<sprache> | je 6 Laeufe, 1 / 0 und 7 / 0 |
| Lints (alle 32, gegen a00f6cf90) | `lints_ergebnis2.txt`, `lints_ergebnis3.txt` | keine neue Beanstandung |

- **Bewusst nicht gelaufen** (volle Runde auf dem zusammengefuehrten Stand): karteneditor voll, menueeditor voll, modellblock
  blender, mb3, rig, mb5, mb6, mb7, ph, rt, ai, uv, md, pt, pfix. Am 10.11. auf einem frueheren Bau gruen: blender 67 / 0, mb3 21 / 0,
  rig 26 / 0, mb5 74 / 0.
- **In keinem Lauf** hat die Mindestgroesse ein Fenster im Dock-Knoten angehoben (Pruefzeile am Ende jedes Laufs). Zeilen "UIF hebt"
  ausserhalb von uiform: zwei in dock1 - das Panel Konsole (500 x 44 -> 500 x 80) und die Ereignisliste (207 x 961 -> 240 x 961)
  in dem Bild, in dem sie aus dem DockSpace in ein eigenes Fenster kommen: schwebend gilt die Untergrenze (gewollt).
- **Die 3 Fehlschlaege von stufe3 unter Last** haengen an Dingen, die es auf dem Rechner nur einmal gibt: "Klick auf [MapFile]:
  ... in der Zwischenablage" (die Zwischenablage teilt sich der Lauf mit allen anderen Programmen; Modus 1 lief daneben und
  benutzt sie auch) und "Port 39000 belegt: gebaut mit 127.0.0.1:39001" / "Port 39001 nicht belegt" (im Protokoll "monitoring on
  127.0.0.1:39002": 39001 hielt ein anderes Programm - von den eigenen Laeufen baut keiner, in ihren Protokollen steht kein
  `-connect`; andere Agenten bauten zu der Zeit Karten). Ohne eigene Laeufe daneben 224 / 0.
  Der karteneditor vertraegt also keine zweite Instanz mit Zwischenablage oder Bau auf dem Rechner - das gehoert zu TESTTEMPO.
- **Geaenderte Testerwartungen**: jeder Selbsttest-Modus eine OK-Zeile mehr (Pruefzeile am Ende); menueeditor M5b eine Zeile mehr
  (die wiederhergestellte Fassung wird wieder gesichert) und "Sicherung weg" laesst eine nach dem Wiederherstellen neu geschriebene
  Sicherung gelten; karteneditor "Test langsam" mit `-threads 1`. Kein Test wurde schwaecher: die Gegenprobe der Probefenster
  schiebt wie die Probe auf 1 x 1.

## Offen

- Ein schwebender Knoten mit mehreren Fenstern (Reiter) hat keine Mindestgroesse aus dem Inhalt: sein Wirt ist ein Fenster von
  ImGui. Fenster mit Fliesslayout rollen darin waagrecht; Panels ohne Rollbalken teilen den Platz selbst auf.
- Das rechte Befehlsfeld des Model-Tabs (Rollouts, `app_modelltab_mb3.cpp:771`) hat dasselbe Fehlerbild wie Into script vorher und
  gehoert zu MBENTWURF - dort den Baustein benutzen.
- Die Dialoge nach NetRadiant-Custom (Qt-Masse) und alle Fenster, die das Programm selbst bemisst, bleiben ohne eigene Grenze.
- Die Zwischenstaende der sechs Commits sind nicht einzeln gebaut; gebaut und geprueft ist der Endstand.
- `q3map2_teilbau` (ctest) ist auf diesem Stand rot - Licht-Cache von LCB2, nicht UIFORM.
- karteneditor unter Last und neben anderen Instanzen: Zwischenablage, Bau-Ports (siehe Schlussrunde) und der kurze Rechtsklick
  fuer den Freelook (300 ms, siehe Zeitrennen) - vorhandene Tests, an TESTTEMPO.
- Handbuchbild `k_pruefen`: der Pfeil der Marke liegt wie bisher ueber dem Knopf Abbrechen (rechts neben Speichern); seit die
  Knoepfe rechts stehen, kaeme er von links besser (Marke in `gui/selbsttest_handbuch.inc`, danach der grosse Bildlauf je Sprache).

## Changelog

## English

- Model tab, "Into script": the window is laid out as a form - one column of labels, all fields start at the same edge and are equally wide, the character is one field with its list, the button "take the selected animation" sits inside the animation field, the hint stands under its field, the line is shown in a code box, and Insert (highlighted) and Cancel are at the bottom right. Enter inserts, Esc closes.
- Windows keep a minimum size: they can no longer be dragged so small that fields shrink to little boxes or buttons are cut off. Rows of controls stay reachable, lists and trees still scroll. A window saved too small is raised when it opens. Docked windows and the panels of a layout keep the size of their place; if the place is too narrow, such a window gets a scroll bar. Dialogs of a fixed size are unchanged.
- "New mission", "Save mission" and "Attach model": the buttons are at the bottom right, the main button is highlighted, Enter confirms and Esc cancels.

## Deutsch

- Model-Tab, „Ins Skript“: das Fenster ist als Formular aufgebaut - eine Spalte für die Beschriftungen, alle Felder beginnen an derselben Kante und sind gleich breit, die Figur ist ein Feld mit Liste, der Knopf „gewählte Animation nehmen“ sitzt im Feld der Animation, der Hinweis steht unter seinem Feld, die Zeile steht in einem Code-Kasten, und Einfügen (hervorgehoben) und Abbrechen sind unten rechts. Enter fügt ein, Esc schließt.
- Fenster haben eine Mindestgröße: sie lassen sich nicht mehr so klein ziehen, dass Felder zu Kästchen schrumpfen oder Knöpfe abgeschnitten sind. Reihen von Bedienelementen bleiben erreichbar, Listen und Bäume rollen weiter. Ein zu klein gespeichertes Fenster wird beim Öffnen angehoben. Angedockte Fenster und die Panels einer Anordnung behalten die Größe ihres Platzes; ist der Platz zu schmal, bekommt so ein Fenster einen Rollbalken. Dialoge mit fester Größe sind unverändert.
- „Neue Mission“, „Mission speichern“ und „Modell anbauen“: die Knöpfe sind unten rechts, der Hauptknopf ist hervorgehoben, Enter bestätigt und Esc bricht ab.

## 中文

- 模型选项卡,“插入脚本”:窗口按表单排布——标签一列,所有字段从同一边缘开始且宽度相同,角色是带列表的一个字段,“使用选中的动画”按钮位于动画字段内,提示在其字段下方,该行显示在代码框中,插入(突出显示)和取消位于右下角。Enter 插入,Esc 关闭。
- 窗口有最小尺寸:不能再拖到字段缩成小方块或按钮被截断那么小。成排的控件始终够得着,列表和树仍可滚动。保存得过小的窗口在打开时会被放大。停靠的窗口和布局中的面板保持其位置的大小;位置太窄时,这样的窗口会出现滚动条。固定大小的对话框不变。
- “新建任务”、“保存任务”和“附加模型”:按钮位于右下角,主按钮突出显示,Enter 确认,Esc 取消。

## 日本語

- モデルタブの「スクリプトへ」: ウィンドウをフォームとして配置しました。ラベルは 1 列、すべての欄が同じ端から始まり同じ幅で、キャラクターはリスト付きの 1 つの欄、「選択中のアニメーションを使う」ボタンはアニメーション欄の中、ヒントは欄の下、行はコードボックスに表示され、挿入 (強調) とキャンセルは右下にあります。Enter で挿入、Esc で閉じます。
- ウィンドウに最小サイズができました。欄が小さな箱に縮んだりボタンが切れたりするほど小さくはドラッグできません。横に並んだ操作部品には常に手が届き、リストやツリーは引き続きスクロールします。小さすぎるサイズで保存されたウィンドウは開くときに広げられます。ドッキングしたウィンドウとレイアウトのパネルはその場所の大きさのままで、場所が狭すぎるとスクロールバーが付きます。固定サイズのダイアログは変わりません。
- 「新しいミッション」「ミッションを保存」「モデルを取り付ける」: ボタンは右下、メインのボタンは強調表示され、Enter で確定、Esc でキャンセルします。

## Español

- Pestaña Model, «Al script»: la ventana se organiza como un formulario: una columna de etiquetas, todos los campos empiezan en el mismo borde y tienen el mismo ancho, el personaje es un solo campo con su lista, el botón «tomar la animación elegida» está dentro del campo de animación, la indicación queda bajo su campo, la línea se muestra en un cuadro de código, e Insertar (resaltado) y Cancelar están abajo a la derecha. Intro inserta, Esc cierra.
- Las ventanas tienen un tamaño mínimo: ya no se pueden arrastrar tan pequeñas que los campos se queden en cajitas o los botones se corten. Las filas de controles siguen al alcance, las listas y los árboles siguen desplazándose. Una ventana guardada demasiado pequeña se amplía al abrirla. Las ventanas acopladas y los paneles de una disposición conservan el tamaño de su sitio; si el sitio es demasiado estrecho, una ventana así recibe una barra de desplazamiento. Los diálogos de tamaño fijo no cambian.
- «Nueva misión», «Guardar misión» y «Acoplar modelo»: los botones están abajo a la derecha, el botón principal está resaltado, Intro confirma y Esc cancela.

## Čeština

- Karta Model, „Do skriptu“: okno je uspořádané jako formulář - jeden sloupec popisků, všechna pole začínají u stejné hrany a jsou stejně široká, postava je jedno pole se seznamem, tlačítko „vzít vybranou animaci“ je uvnitř pole animace, nápověda stojí pod svým polem, řádek je v rámečku s kódem a Vložit (zvýrazněné) a Zrušit jsou vpravo dole. Enter vloží, Esc zavře.
- Okna mají minimální velikost: už je nelze stáhnout tak, aby se pole smrskla na čtverečky nebo se uřízla tlačítka. Řady ovládacích prvků zůstávají na dosah, seznamy a stromy se dál posouvají. Okno uložené příliš malé se při otevření zvětší. Ukotvená okna a panely rozvržení si drží velikost svého místa; je-li místo příliš úzké, dostane takové okno posuvník. Dialogy s pevnou velikostí se nemění.
- „Nová mise“, „Uložit misi“ a „Připojit model“: tlačítka jsou vpravo dole, hlavní tlačítko je zvýrazněné, Enter potvrdí a Esc zruší.
