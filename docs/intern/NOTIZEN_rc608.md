# Notizen rc608

## Teil 1

### Punkt 1: Startbildschirm des Menue-Editors
- Ohne offenes Menue zeigt der Menue-Editor jetzt dieselbe Seite wie der Mission-Tab: Logo, Titel, Text, Karten, Zeile "Neu hier?". Die Bauteile sind dieselben Funktionen (`startKopf`, `startKarten`, `startKnopfZeile` in `gui/app_mission.cpp`), `drawStartflaeche` baut sich ebenfalls daraus.
- Die Bauteile koennen jetzt, was der Menue-Editor brauchte und der Mission-Tab auch gebrauchen kann: lange Titel brechen um, Karten sind so hoch wie ihr Text, im schmalen Fenster stehen sie untereinander, Knopfzeilen brechen um, bei wenig Hoehe (unter 24 Schrifthoehen) entfaellt das Logo.
- Einzelspieler/Mehrspieler steht ueber den Karten mit einem Satz, was die Wahl festlegt: Ordner (`startMenuePfad`), Format (SP-Namen, MP-Zahlen ueber menudef.h, NOTIZEN_menue1) und die vom Spiel gezeichneten Inhalte (Ownerdraws und Feeder je Dialekt).
- Befund: Der Satz "open a menu from the game with the first button above" war falsch. Der erste Knopf der Leiste ist "Projekt", "Aus dem Spiel" der zweite. Text in allen 6 Sprachen auf "zweiter Knopf" gestellt, der Selbsttest prueft die Reihenfolge der Knoepfe.
- "Neues Projekt" und "Projekt oeffnen" laufen ueber `neuesProjektWaehlen`/`projektOeffnenWaehlen`, gemeinsam fuer Leiste und Startbildschirm. Der Selbsttest ersetzt den Windows-Dialog (`setzeProjektDialog`), sonst waere er nicht ohne Fenster zu pruefen gewesen.
- Tour-Anker: `knopf:MePrjNeu` (tut-menue-eigen, Schritt 2) trifft die Karte, `knopf:MeAusSpiel` (tut-menue, Schritt 2) die Leiste; beides prueft der Selbsttest. Helfer-Orte: unveraendert (195), der Startbildschirm stand nie im Katalog.
- Prueflauf `BHED_MENUE_NUR=start`: 6 Sprachen x (100 %, 200 %) x (grosses, kleinstes erlaubtes Fenster), je Rollen von oben nach unten: jedes Bauteil einmal ganz im Bild, nichts ragt seitlich hinaus, Karten fassen ihre Texte. Rot vorher (kein Bauteil im Fenster `##menuestart`), gruen 30/30.
- Eine Liste "Zuletzt bearbeitet" wie im Mission-Tab gibt es fuer Menue-Projekte nicht, weil das Programm keine fuehrt. Nicht erfunden; waere ein eigener Punkt.

### Punkt 2: Unbekannte Schalter
- Ursache: `argv[1]` galt immer als Datei (`main_win32.cpp`, fuenf eigene Auswertungen). Jetzt `bhed/befehlszeile.h` mit ctest `befehlszeile`: rot 8 Fehler (`--version` -> Datei), gruen.
- Unbekanntes mit `--` wird nie geoeffnet, das Programm geht nicht auf, die Meldung nennt den Schalter, die bekannten Schalter und den Weg ueber einen Pfad (`.\--name`). `--version` nennt die Fassung. Ausgabe in die Umleitung, sonst in die Konsole des Aufrufers, sonst ein Meldungsfenster (nie mit `BHED_KEIN_MELDUNGSFENSTER`).
- Die echte exe wurde nach Auftrag nicht mit diesen Schaltern gestartet: Geprueft ist die Auswertung, nicht die Ausgabe in die Konsole.

### Punkt 6: Auswaehlen und Ziehen
- Der erste Druck waehlt nur aus, auch wenn die Maus wandert. Erst ein neuer Druck auf Gewaehltes zieht, ab einer Schwelle von einer Viertel-Schrifthoehe; ein Griff genauso.
- Strg/Umschalt nehmen nur hinzu oder heraus. Ein Druck auf ein Element der Auswahl zieht die ganze Auswahl; ein Klick ohne Ziehen laesst nur dieses gewaehlt.
- Ueberlappung: Ein Druck meint das vorderste Element, das mit Rahmen und Namen unter dem Zeiger. Alt+Klick geht beim Loslassen durch den Stapel wie bisher; neu: Alt+Ziehen bewegt das Gewaehlte auch verdeckt. Sonst waere ein verdecktes Element nach der neuen Regel nur noch mit den Pfeiltasten zu bewegen.
- Prueflauf `BHED_MENUE_NUR=ziehen`: rot 24 Fehler (frei sprang beim ersten Druck auf 429,312), gruen 26/26.

### Tester: "when I click off, it resets"
- Nachgestellt: Das Ziehen auf der Flaeche blieb immer stehen (MD, JKA, neu, je Dialekt). Der Fehler sass in Properties: `float r[4]` wird in jedem Bild neu aus dem Text gelesen. Ziehen an `rect` behielt nur das letzte Bild (rot 404 statt 444), getippte Zahlen und Texte gingen beim Klick daneben verloren (nur Enter schrieb), und ein Klick auf die Flaeche nahm dem Zugfeld die Aktivitaet, bevor es seinen Wert abliefern konnte.
- Behoben mit einem Halteplatz (`zahlenFeld`, `textFeld`, `haltPruefen`) in Properties, Events und Layout: Der Wert lebt dort, bis er fertig ist, dann ein Schritt, und zwar bevor der Klick Auswahl oder Reiter wechselt. Esc verwirft.
- `Dokument::altStaende`: `neuParsen` gab mitten im Bild die alte `Datei` frei, die Panels lasen danach aus freigegebenem Speicher weiter. Sie lebt jetzt bis zum naechsten Bild.
- Kann ein Zug nicht geschrieben werden (`rect` aus einem Makro), kommt eine Einblendung statt der grauen Zeile in der Leiste.
- Prueflauf `BHED_MENUE_NUR=bleiben`: Ziehen, Klick daneben, alle Reiter, Fokus Text-Editor und Properties, Speichern und neu Laden, auf Originalen und neuen Menues. JK2-Menues nur mit `CFG=m5b/behaved_jk2.cfg`.

### Punkt 0: Undo
- Rot: Strg+Z ging in keinem einzigen Fall der Tabelle (41 von 91 Zeilen rot, dann Absturz), nur die Pfeile der Leiste, und auch die nicht mit offenem Umbenennen-Feld.
- Ursachen: Tasten nur mit Fokus auf der Flaeche und mit Auswahl; das Umbenennen-Feld holte sich in jedem Bild den Fokus zurueck und sperrte damit alles; sein Index galt ueber Reiter hinweg (daher "description" auf der falschen Zeile).
- Absturz: Rueckgaengig nach dem Einfuegen liess die Auswahl auf dem entfernten Element stehen. `neuParsen` prueft die Auswahl jetzt gegen das Menue.
- Neu: `editorTasten` im ganzen Menue-Editor vor den Panels. Ausnahmen: echte Eingabefelder, Text-Editor (gleicher Puffer, eigene Tasten, wirkt erst vor dem naechsten Bild), offene Fenster. Pfeiltasten, Entf, Strg+D/C/V gelten, wo kein Panel mit Werten den Fokus hat.
- Seite und Auswahl gehoeren zum Schritt (`Textpuffer::beiwerkFesthalten`, ctest `menue_text`). Einfuegen aus der Bibliothek war bis zu drei Schritte, jetzt einer.
- Umbenennen (auch Punkt 6 aus Teil 2, damit erledigt): nimmt den Fokus nur am Anfang, endet mit Enter, Esc (ohne Aenderung), Klick daneben, Auswahl- und Reiterwechsel; Getipptes zaehlt, und es trifft das richtige Menue.
- Prueflauf `BHED_MENUE_NUR=undo`: Tabelle 15 Aktionen x 8 Wege, Zeilen `Knopftest: Undo-Tabelle | ...` im Protokoll.
- Das Auge und das Schloss sind kein Schritt: Sie stehen nicht in der Datei.

### Punkt 0b: Typ
- Ursache: Die Schluessel des Modells gelten bei einer Checkbox nicht. `Menu_Parse` liefert false, und `Menu_New` verwirft im Spiel das ganze Menue (OpenJK `code/ui/ui_shared.cpp` und `codemp/ui/ui_shared.c`: kein `Menu_PostParse`, `menuCount` bleibt). Der Leser des Editors hoerte dort ebenfalls auf und liess die Bildschirmlage aus; darum lagen alle Elemente auf 0,0. Die Datei selbst aenderte sich nur in der `type`-Zeile, Rueckgaengig stellte alles her.
- `type` ist in Properties nur noch Anzeige mit Tooltip. `style` und `textalign` koennen ein Menue so nicht zerstoeren (geprueft, alle Werte, beide Dialekte), sie bleiben Auswahl.
- `LeseOptionen::weiterlesen` (nur der Editor): gleiche Meldungen, gleiches Urteil (`Menu::ok` false), aber das Lesen geht in der naechsten Zeile weiter und die Lage stimmt. Ueber der Flaeche steht rot, dass das Spiel das Menue verwerfen wuerde, mit Knopf zur Zeile. Das Lesen wie im Spiel bleibt der Vorgabewert (ctest).
- Prueflauf `BHED_MENUE_NUR=typ`: alle 15 Typen, SP und MP, rot (alle auf 0,0), gruen 88/88.

### Punkt 3: Selbsttest update
- Die Pruefung sucht jetzt `<laufende exe>.alt` und die laufende exe, `behaved.exe` einzeln als Starter. Rot: `alt nein` (Stand 5, 4968 Dateien). Gruen: `alt ja, exe ja, Starter ja`, je einmal mit dem echten rc607 (rund 55 s).

### Punkt 4: Selbsttest Zwischenablage
- Ursache bestaetigt: Das Lesen danach (`echtLesen`) hatte keine Wiederholung; ImGui liefert nichts, solange ein anderes Programm die Ablage offen hat.
- Nachstellen war schwerer als gedacht: `OpenClipboard(NULL)` sperrt andere gar nicht, erst ein Oeffnen mit Fenster. Und auf Zufall gewinnt der Mitleser fast nie. Darum `SP/ab/stoerer.py leser` mit Fenster, gerufen genau nach dem Zurueckschreiben (`platform::zwischenablageStoererRufen`, ohne Stoerer ohne Wirkung). Rot: "danach 0 Bytes", wie am 31.10. Gruen: "danach 85 Bytes". Die Pruefung ist nicht aufgeweicht: Der Inhalt muss weiter gleich sein.
- Die Probe laeuft nur mit `BHED_ABLAGE_PROBE=1` (TESTING.md, Release builds). Ohne Schalter schreibt kein Selbsttest in die Ablage; die Pruefung "liess die Ablage in Ruhe" laeuft immer.

### Punkt 5: JK2 im laufenden Spiel
Steam war angemeldet, vor jedem Start geprueft. `SP/rc608/jk2spiel.py`: Fenster rechts ausserhalb aller Bildschirme, kein Fokus, stumm, in_mouse 0, geschrieben nur nach `Y:\menue_log\rc608jk2`. Der Spielordner ist danach unveraendert (Vergleich der Dateiliste). Leere Konfigs im eigenen Ordner gehen vor denen in GameData: Verglichen wird der Stand des Spiels, nicht shanks Einstellungen.

Wege, die vorher nicht gingen:
- Jedes JK2-MP-Menue: loses `ui/jk2mp/main.menu` im eigenen Ordner, dessen onOpen das Menue oeffnet. Die Ingame-Menues laedt das Spiel sonst erst in einer Partie, darum ein loses `ui/jk2mpmenus.txt` mit ihnen. Gleich fuer SP.
- Datapad: `kejim_base` und `bespin_platform` starten ohne Zwischensequenz davor (kejim_post und yavin_swamp nicht).

Alle 35 JK2-MP-Menues und 17 SP-Menues plus Datapad gegen die Vorschau. Gefunden und behoben (jeweils "rot" auf den Kontaktbogen, ctest wo moeglich):
- Texte: JK2 legt sie in `MENUS0..3` und `INGAMETEXT` ab, die Vorschau fragt nach den Namen von JKA. Darum leer: Ja/Nein ("Lightsaber Only:"), Spieltyp, Skill, Force Mastery, "Server Refresh Time". `jk2Namen` in `spieldaten.cpp`. HUMAN/CLOSED sind in ui.qvm feste Woerter.
- Cvars: Die Vorschau nahm fuer JK2-MP die Werte von JKA ("*Jedi*" statt "noname", ui_netSource leer, darum "Add Favorite" und "Del Favorite" aufeinander). Neu: `cvarvorgaben_jk2.inc`, erzeugt aus dem laufenden Spiel (`cvarlist`), `tools/gen_menue_cvarvorgaben_jk2.py`.
- `uiScript getvideosetup` fehlte in der Vorschau: Die Setup-Seiten zeigten "High Quality", "Off" statt "Custom", "On" (gilt fuer alle Spiele, UI_GetVideoSetup).
- `uiScript updateForceStatus` fehlte: In ingame_player standen "The Force" und "Force Disabled", Apply und Join Game aufeinander. Portiert fuer den Zustand ohne Partie; jk2mp legt dabei ui_about_gametype an.
- Saebelbild: jk2mp zeichnet `menu/art/saber_<farbe>` ueber das ganze Feld, die Nummer ist color1 (0 rot bis 5 lila, im Spiel je Wert nachgesehen). Die Vermutung aus M7 (fx_base mit fx_*, Q3-Erbe) war falsch: ui.qvm laedt sie, zeichnet sie aber nicht.
- Kartenliste leer: JK2 hat seine Karten in `scripts/arenas.txt`.
- Listen abgeschnitten ("Bes" statt "Bespin Streets"): Die Grenze beim Text zaehlt in jk2mp Zeichen (Q3), in JKA Bildpunkte.
- Tastennamen: "R or MWHEELDOWN" (Spiel) statt "R  MWHEELDOWN".
- Name der gewaehlten Figur unter der Kopfliste ("rebel/default"): in JKA steht das in `#if`, jk2mp zeichnet es.
- Force Mastery beim Server: g_maxForceRank (Jedi Knight), nicht der Rang des Spielers.

Kein Unterschied, nur der Augenblick: die Griffe mit dem Ring (im Spiel dreht er sich genauso, Zeitreihe `SP/rc608/jk2/reihe.png`). Kein Vergleich moeglich: `Video Mode` (der Lauf setzt r_mode -1), Spielwerte im Datapad (Beispielwerte der Vorschau), Text und Werte in videodriver (Treiber).

## Teil 2

### Punkt 15: Shader und Bilder
- Ursache: Der Tester hatte nur den Ordner `MD` eingerichtet. Das Spiel liest `base` immer mit (FS_Startup), die Vorschau nicht: 17 Machtsymbole "File not found", graue Kaesten. Genau sein Bild nachgestellt mit `CFG=SP/rc608/nur_md.cfg` (rot: Warnungen 17, davon Bilder 17, Bilder geladen 5; gruen: 0, 0, 22). Gross/Klein war es nicht.
- `spielordnerMitBase` (pk3.h, ctest `menue_spieldaten`): base daneben kommt als niedrigster Rang dazu, in `rescanGamePaths` also fuer das ganze Programm. Die Einstellungen zeigen das ("Wird wie im Spiel mitgelesen") und ordnen die Zahlen jetzt ueber den Ordner zu statt ueber die Stelle.
- `Spieldaten` wie FS_AddGameDirectory: die pk3 eines Ordners vor seinen losen Dateien (`fs_dirbeforepak` 0, vorher umgekehrt), `gibt` und `liste` kennen lose Dateien (lose `shaders/*.shader` wurden vorher nicht gelesen).
- Shaderskripte wie ScanAndLoadShaderFiles/ShaderEntryPtrs_Insert: Reihenfolge von FS_ListFiles, der letzte gilt (vorher alphabetisch).
- `bildRegistriert`: eine Stelle fuer "Griff 0 oder nicht", Vorschau und Tabelle gehen denselben Weg; findet eine Stufe ihr Bild nicht, verwirft ParseStage den ganzen Shader. Das trifft in den Originalen genau `gfx/menus/menu_box1` (`ui/jamp/ingame_callvote.menu`). Diagnose und Pruefung nutzen eine gemeinsame Antwort (`bildBekannt`), auch mit `*white`.
- Tabelle `BHED_MENUE_NUR=shader`, je Spiel gegen ein unabhaengiges Skript (`SP/rc608/bildtabelle.py`, Regeln der Engine auf den pk3): MD 8386 Eintraege / 8214 Treffer / 172 fehlen, JKA 1484 / 1377 / 107, JK2 897 / 895 / 2. Die Fehlzeilen sind gleich, bis auf `menu_box1`, das nur die Vorschau wegen der Stufenregel kennt (das Skript prueft keine Stufen). Fehlend sind vor allem `*`-Namen (Cvars aus einer Siege-Partie), dazu Dateien, die es wirklich nicht gibt (`forcecirclegray`, `gfx/menus/newgame_boxes`, `gfx/menus/tutorial/*`, `menu/art/jk2unknownmap`).
- Spielfotos Movie Duels (`SP/rc608/mdspiel.py`, `SP/rc608/md/`): Forcemenue wie im Spiel, nebeneinander in `SP/rc608/shader/`. Stichprobe fehlendes Bild `ingametutorial`: das Spiel zeichnet denselben grauen Kasten mit weissem Rand. Machtsymbole und Hex-Muster (additiv, Reihenfolge) stimmen, also nichts an den Stufen geaendert.
- Gefunden dabei: WEAPONS ist im Spiel grau (UI_ForcePowerWeaponsButton, `uiScript initallocforcepower`, greyColor in Item_TextColor), jetzt auch in der Vorschau.

### Punkt 17: Ersetzen bei Strg+F
- Rot: `BHED_MENUE_NUR=ersetzen`, 9 Fehler (Feld und Knoepfe nicht da, Strg+F schaltete das Ersetzen sogar ab). Gruen: 7/7. Foto: `SP/rc608/vorher_p17_strgf.png`, `SP/rc608/nachher_p17_strgf.png`.
- Strg+F zeigt jetzt immer Suchen und Ersetzen, mit den Beschriftungen des Skript-Bereichs (`FindReplaceBtn`, `FindReplaceAll`, `FindReplaceAllDocs`). Alle ersetzen: ein Undo-Schritt je Datei, Anzahl in der Einblendung und in der Leiste. Ohne Suchtext grau.
- Strg+H bleibt: gleiche Leiste, der Cursor steht gleich im Ersetzen-Feld. Strg+F ohne Auswahl behaelt den letzten Suchtext (vorher leer).
- "In all open menus" bleibt fuer die Trefferzahl; ersetzt wird in allen Dateien nur ueber den eigenen Knopf, statt ueber den Haken am selben Knopf.

### Punkt 1: Reiter "Text editor"
- `MeReiterText` in allen 6 Sprachen; `BHED_MENUE_NUR=reiter` prueft Name und dass kein Reiter gestaucht ist. Rot: 6 Fehler (Name), gruen 6/6. Schmale Felder: Punkt 13.

### Punkte 16 und 8: Mauszeiger (ein Commit, ein Prueflauf `BHED_MENUE_NUR=zeiger`)
- Geprueft ueber den gesetzten ImGui-Zeiger je Stelle (`zeigerAn`). Rot 23 Fehler, gruen 31/31.
- Text-Editor: ueber dem Text der Text-Cursor, im Zeilenrand und ueber der Minimap der Pfeil. Eingabefelder setzt ImGui selbst, geprueft am Suchfeld.
- Flaeche: 8 Griffe mit dem passenden Pfeil, auf dem gewaehlten ziehbaren Element der Verschiebe-Zeiger, sonst der Pfeil, auch bei gesperrten Elementen. Der Zeiger bleibt beim Ziehen, auch wenn die Maus den Griff verlaesst.
- Neu: Die Raender zwischen den Griffen skalieren (Rand rechts aendert nur die Breite). Vorher konnte man nur an den Griffen ziehen.
- Rand der Flaeche: Rechts und unten bleibt ein Rand von 6 Punkten, der Griff bei x = 640 und y = 480 liegt also ganz im Feld. Griffe gehen vor dem Lineal: Der Griff bei y = 0 lag halb im Lineal und zog eine Hilfslinie statt des Elements. Rahmen und Griffe werden nicht mehr am Rand abgeschnitten.

### Punkt 4: Zoomen und Verschieben
- Strg+Mausrad zoomt um den Zeiger, 25 bis 800 % (Bildpunkte je virtuellem Punkt in x-Richtung), Strg+0 passend, Strg+1 100 %, Mitteltaste oder Leertaste+Ziehen verschiebt, in der Zeile ueber der Flaeche Anzeige und Knoepfe. Die Ansicht haengt am Dokument, ist kein Undo-Schritt und nicht in der Datei. Ein Stueck der Flaeche bleibt immer im Bild.
- Alles Weitere rechnet ueber dieselbe `Abbildung` (Griffe, Treffer, Einrasten, Hilfslinien, Raster, Referenzbild, Gizmo). Griffe bleiben gleich gross. Lineale: Teilung nach Zoom, ohne Zoom wie bisher. Beim Zoomen zeigt ein Rahmen, wo der Schirm endet.
- `BHED_MENUE_NUR=zoom`: rot 60 Fehler, gruen 85/85. Tabelle aus Punkt 8 je Stufe (381 %, 800 %, 25 %, 381 % mit Balken), Ziehen ergibt je Stufe 30 virtuelle Punkte (bei 25 % 28 wegen der Bildpunkte), der Punkt unter dem Zeiger bleibt beim Zoomen stehen.

### Punkt 5: Text direkt schreiben
- Doppelklick auf der Flaeche oeffnet das Schreiben. Gemeint ist das vorderste Element mit Text unter dem Zeiger. Gezeichnet wird das Getippte von der Vorschau selbst (`schreibKopie`), also in Schrift, Groesse, Farbe, Ausrichtung und Umbruch des Spiels, auch die Farbcodes je Spiel. Die Eingabe macht ein unsichtbares ImGui-Feld (Tasten, Auswahl, Ablage, eigenes Rueckgaengig); Cursor und Auswahl werden aus den aufgezeichneten Textzeilen (`TextLauf`) des Malers gerechnet, die Maus setzt sie um.
- Esc verwirft. Enter oder Klick daneben uebernimmt, ein Undo-Schritt, im Text aendert sich nur die Zeile `text`. Strg+Enter macht in Textscroll und umbrochenen Elementen eine neue Zeile. Datei: SP roh im String (COM_ParseExt), " wird in SP beim Tippen nicht angenommen, MP mit Escape (`\n`, `\"`, Botlib-Lexer).
- Textscroll: Der Cursor bleibt sichtbar, Bild-Auf/Ab geht ueber das Feld und das Nachfuehren (`startPos`). Nicht eigens getestet: Bild-Auf/Ab und Kopieren/Einfuegen (der Selbsttest laesst die Ablage in Ruhe).
- `@`-Texte: Es wird der aufgeloeste Text gezeigt. Beim Aendern bleibt das Spiel-Paket, wie es ist. Das Element bekommt einen eigenen Text (`@MYMENU_MENUS_BACK`) im vorhandenen Textpaket, die anderen Sprachen behalten ihre Uebersetzung, die Einblendung sagt das. Rueckgaengig stellt die Zeile her. Abweichung: Aendert man einen schon eigenen Text, aendert sich nur das Textpaket, kein Undo-Schritt; die Einblendung sagt auch das.
- Cvar, Ownerdraw, Feeder, Modell, Bild: eine Einblendung erklaert, woher es kommt.
- Hierarchie: `knopf  ITEM_TYPE_BUTTON  "Back"`, gekuerzt. Properties: das Textfeld steht ganz oben.
- `BHED_MENUE_NUR=schreiben`: rot alle Schritte (ohne Programmteil; Doppelklick tat nichts), gruen 24/24. Foto: `SP/rc608/nachher_p5_schreiben_tippen.png`.
- Antwort an shank ("wenn ich einen text waehle ist er kein Text sondern ein Bild?"): In `ui/ail.menu` (MD) tragen mehrere Elemente den Namen `description`. Das eine ist die Tafel dahinter: WINDOW_STYLE_FILLED mit `background gfx/menus/mission/desc_panel`, also ein Bild ohne Text. Der Text "Around ten years have passed ..." ist echter Text: `@MD_BRIEFINGS_EP2_AIL_DESC` aus `strings/English/MD_BRIEFINGS.str` in `MD_Assets.pk3`, gezeichnet mit Spielschrift vom TEXTSCROLL. Wer neben die Zeilen klickt, trifft die Tafel. Ein Doppelklick nimmt jetzt den Text unter dem Zeiger, und auf einem reinen Bild erklaert er das.

### Punkt 10: Hierarchie nur fuer die offene Seite
- Die Hierarchie zeigt ab Werk nur das `menuDef` des offenen Reiters. Umschalter "Ganze Datei" (Einstellung `menueGanzeDatei`, ab Werk aus). Die Liste folgt der Auswahl auf der Flaeche (rollt hin).
- Zustandsreiter (`ui_sel=0_0`): nur, was dort laut `showCvar`/`hideCvar` sichtbar ist. Detail area: 79 Zeilen im ganzen Menue, 43 im ersten Zustandsreiter; 23 Reiter gibt es weiterhin (das Bauteil ist so gebaut).
- Nebenbefund behoben: Die Fensterleiste nannte im Menue-Editor das Skript ("unnamed.txt"), jetzt `ui/ail.menu * - JK Studio`.
- `BHED_MENUE_NUR=hierarchie`: rot 7 Fehler, gruen 9/9. Fotos: `SP/rc608/vorher_p10_hierarchie_seite.png`, `SP/rc608/nachher_p10_hierarchie_seite.png`.

### Punkt 11: Widgets per Ziehen
- Alle Eintraege oben (auch "Bild hinzufuegen") lassen sich auf die Flaeche ziehen und landen an der Ablegestelle: Rahmen beim Ziehen, Einrasten wie beim Verschieben, richtig bei jeder Zoomstufe, gewaehlt, ein Undo-Schritt. Ein einfacher Klick tut nichts mehr (vorher setzte er ein). Doppelklick setzt in die Mitte, ohne Stapel: der zweite liegt 20, 20 daneben.
- Ein Weg: `ME_VORLAGE` traegt die Bibliothek-Nummer oder das Widget, `setzeAblageEin` macht beides. `fuegeEin` hat keine eigene Logik mehr.
- `BHED_MENUE_NUR=ablegen`: rot 16 Fehler, gruen 17/17.
- Der Dateidialog von "Bild hinzufuegen" ist im Selbsttest ersetzt (`setzeBildDialog`).
- Antwort an shank ("was ist an libary und oben die sachen anders?"): Oben steht je Eintrag ein einzelnes, nacktes Element. Unten stehen fertig eingerichtete Bauteile, teils aus vielen Elementen. Im Einzelnen: "Button" oben ist nur Text ohne Hintergrund mit leerer Aktion; unten dunkler Hintergrund, mittiger Text, Klickton (1 Element). "Yes/No" unterschied sich nur in der Ausrichtung des Textes, darum ist der untere Eintrag entfernt. "Slider with cvar" ist schon mit `s_volume` verbunden. "Tab bar" sind 3 verdrahtete Knoepfe, "Detail area" Reiter mit Unterreitern (viele Elemente plus onOpen), "List" hat Hintergrund, Rahmen und Feeder fuer die Spielermodelle.
- Abschnitte heissen jetzt "Einzelne Elemente" und "Fertige Bauteile" (6 Sprachen), je Eintrag ein Tooltip mit Inhalt, unten mit der Anzahl der Elemente. Eigene Vorlagen stehen unten mit `*`.
- Neue Bild-Elemente tragen weiter `background gfx/menus/menu_box1`: Das gibt es in den Spieldaten nicht, also zeichnet das Spiel den grauen Kasten, bis man ein Bild waehlt.

### Punkt 12: Modell einfuegen und drehen
- Befund im laufenden Spiel (Fotos `SP/rc608/md/sp_jks_modell1.tga`, `sp_jks_modell2.tga`, `mp_jks_modell3.tga`), Ursachen fuer shanks T-Pose und die Seitenansicht:
  - T-Pose: `ItemParse_asset_model_go` setzt die Animation, die zu dem Zeitpunkt schon gelesen ist. Steht `model_g2anim` nach `asset_model`, zeigt das Spiel die T-Pose, nur `isCharacter` stellt sie spaeter. Das MP von OpenJK wertet `model_g2anim` ohne `isCharacter` gar nicht aus. Die Vorschau spielte sie immer: jetzt `Item::g2animOhneWirkung` (Parser, ctest `menue` ...) und Bild 0 wie im Spiel.
  - Seite: Die Vorschau drehte eine Figur mit Skelett um 90 Grad falsch (model_angle 180 ist im Spiel von vorn, 0 von hinten). Unbemerkt, weil die Originalmenues die Figur drehen. Fehler in der Vorschau, behoben.
  - Zwei Items mit gleichem Modell, Skin und Animation teilten sich in der Vorschau ein Bild, das letzte `model_angle` galt fuer beide (auf dem Foto gefunden). Jetzt je Item ein Ziel.
- Vorgabe aus den Menues statt geschaetzt: JKA/MD `ui/jamp/ingame_player2.menu` (rect 200 x 225, fov 50, model_angle 180, g2mins -30 -15 -14, g2maxs 20 15 30), Pose `BOTH_MENUIDLE1` aus `ui/md_mp/ingame_player.menu`, `model_g2anim` vor `asset_model`, Skin dazu. MP von OpenJK: `ui_char_model` mit `isCharacter` und `BOTH_WALK1`. JK2: das Logo wie in `ui/jk2mp/credits.menu`, es kennt in Menues kein Ghoul2.
- Griffe: Reihe aus 4 Knoepfen mit Symbolen (Pfeilbogen, Lupe, Hoehenpfeil, Kegel), 1,15 Knopfhoehen gross (36 gegen 32 Punkte), Tooltip wie gehabt, eigener Mauszeiger. Im Element oben links, passt es nicht, darueber oder darunter; nie auf den Griffen zum Skalieren. Das prueft der Test je Zoomstufe und bei 200 x 40.
- Drehen schreibt nur `model_angle`, ein Undo-Schritt je Zug. `model_rotation` (laufende Drehung) bleibt davon unberuehrt; beides getrennt in Properties: Punkt 14.
- `BHED_MENUE_NUR=modell`: rot 7 Fehler, gruen 9/9. Die Blickrichtung und die Pose sind nur per Foto geprueft (Spiel links, Vorschau rechts: `SP/rc608/nachher_p12_spiel_vorschau.png`).

### Punkt 14: Properties je Element, Animation waehlen
- Properties zeigt je Typ nur, was das Spiel auswertet (`feldGilt` in `src/menue/werkzeuge.cpp`, ctest `menue`). Oben steht, was immer gilt, darunter die Typ-Gruppe. Schluessel, die nicht passen oder kein Feld haben, bleiben in der Datei und stehen unter "Weitere Eintraege in der Datei". Die Vervollstaendigung im Text-Editor filtert im itemDef genauso. Bei den Events gibt es keine Typregel: mouseEnter, action usw. wertet das Spiel fuer jedes nicht dekorative Element aus.
- Tabelle Typ gegen Felder (Gemeinsames gilt immer: name, group, rect, style, visible, background, backcolor, bordercolor, border, Skripte):

| Feld | TEXT | BUTTON | RADIO | CHECKBOX | EDIT | COMBO | LISTBOX | MODEL | OWNERDRAW | NUMERIC | SLIDER | YESNO | MULTI | BIND | TEXTSCROLL |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| text, text2 | ja | ja | ja | ja | ja | ja | - | - | ja | ja | ja | ja | ja | ja | ja |
| textscale, textstyle, font | ja | ja | ja | ja | ja | ja | ja | - | ja | ja | ja | ja | ja | ja | ja |
| textalign | ja | ja | ja | ja | ja | ja | - | - | ja | ja | ja | ja | ja | ja | - |
| textalignx/y | ja | ja | ja | ja | ja | ja | - | - | ja | ja | ja | ja | ja | ja | ja |
| forecolor | ja | ja | ja | ja | ja | ja | ja | nur Style SHADER | ja | ja | ja | ja | ja | ja | ja |
| cvar | ja | - | ja | ja | ja | - | - | - | - | ja | ja | ja | ja | ja | ja |
| feeder, elementwidth/height/type, columns | - | - | - | - | - | - | ja | - | - | - | - | - | - | - | - |
| lineHeight | - | - | - | - | - | - | - | - | - | - | - | - | - | - | ja |
| ownerdraw | - | - | - | - | - | - | - | - | ja | - | - | - | - | - | - |
| maxChars, maxPaintChars | - | - | - | - | ja | - | - | - | - | ja | - | - | - | - | - |
| asset_model, model_angle, model_rotation, model_fovx/y | - | - | - | - | - | - | - | ja | - | - | - | - | - | - | - |
| model_g2*, isCharacter, isSaber | - | - | - | - | - | - | - | ja (nicht JK2) | - | - | - | - | - | - | - |
| model_origin | - | - | - | - | - | - | - | nur JK2 | - | - | - | - | - | - | - |

- Modell-Gruppe: Modell und Skin mit Auswahl aus den Spieldaten, Animation aus der `animation.cfg` des gewaehlten Modells mit Suche, Blickrichtung (`model_angle`) und laufende Drehung (`model_rotation`) als zwei Felder, Hoehe und Abstand ueber `model_g2mins/maxs` wie die Griffe, Blickwinkel, isCharacter, isSaber. Die Vorschau spielt die Animation sofort.
- Die Animation wird vor `asset_model` geschrieben (Punkt 12), sonst spielt das Spiel sie nicht; steht sie falsch, sagt das ein Hinweis, und das Neuwaehlen rueckt die Zeile. Ein Undo-Schritt.
- `BHED_MENUE_NUR=felder`: rot 9 Fehler (altes Programm, alle Felder bei jedem Typ, kein "Animation waehlen"), gruen 19/19. Pose und Ablauf der neu gewaehlten Animation sind nur per Vorschau-Foto geprueft (`SP/rc608/nachher_p14_animation.png`); gegen das Spiel nur fuer `BOTH_STAND1` und `BOTH_MENUIDLE1` (Punkt 12). Eine selbst gebaute `animation.cfg` ist nicht getestet: Die Liste liest dieselbe Datei wie der Lader.
- Part-1-Tests angepasst: `bleiben` (51/51), `typ` (88/88), `undo`. In der Undo-Tabelle fuegt das Widget jetzt per Doppelklick ein (Punkt 11), und der Klick "in das Panel" traf in Properties wegen der neuen Felder einen Knopf; dort klickt er jetzt auf eine Beschriftung. Einmal rot auf der Zeile "Properties rect | Fokus Text-Editor": beim zweiten Lauf gruen, siehe Absturztest.

### Punkt 13: Rechtes Feld nicht zu schmal
- Mindestbreite rechts 16 Schrifthoehen, links 11; die Flaeche behaelt mindestens 20, das Feld laesst sich nicht mehr bis auf 12 Schrifthoehen (und die Flaeche nicht mehr auf nichts) ziehen. Gezogene Werte werden je Bild begrenzt, auch bei kleinerem Fenster.
- Neue Bausteine in Properties und Layout: `feldKopf` (Beschriftung vorn, im schmalen Feld darueber), `zahlenReihe` (x, y, w, h und R, G, B, A beschriftet, brechen um). Knopfreihen brechen um (`daneben`).
- `BHED_MENUE_NUR=breiten`: Bauteil-Rechtecke und `ScrollMax.x` bei 300, 500, 1000 Punkten, 100 % und 200 %, 6 Sprachen, Knopf und Modell gewaehlt, je 72 Zeilen. Unter der Mindestbreite gilt die Mindestbreite: Bei Schrift 24 (Skalierung des Bildschirms) sind das 384 statt 300 Punkte, bei 200 % 768. Dazu am Minimum die Spalten und alle Reiter in jeder Sprache. Rot 54 Fehler (Mindestbreite 10 statt 384), gruen 80/80.
- Reiterleiste: In schmalen Feldern werden die Reiter schmaler (FittingPolicyShrink), statt hinauszuragen; der Tooltip nennt den ganzen Namen.
- Foto: `SP/rc608/nachher_p13_schmal.png`. Vergleich vorher: `SP/rc608/vorher_p13_schmal.png`.

### Punkt 2: Text-Editor zeigt zu wenig
- Das rechte Feld ist ab Werk bis zu 36 Schrifthoehen breit (hoechstens 42 % des Bereichs, vorher 28), die gezogene Breite bleibt gespeichert. Gemessen: 1920 x 1080, 100 %: rechts 30,1, Flaeche 25,6, Text 20,1 Schrifthoehen; 2560 x 1600, 100 %: 36, 46,4, 24; 1920 x 1080, 200 %: 16, 7,7 (369 Bildpunkte), 4,4; 2560 x 1600, 200 %: 16, 13,9, 9. Bei 200 % auf 1920 ist die Flaeche also schmal: Mehr gibt die Breite nicht her, wenn das rechte Feld lesbar bleiben soll (Zoom und Verschieben aus Punkt 4 helfen). Text-Editor und Events teilen sich die Breite.
- Waagerecht: ein richtiger Rollbalken (0,6 Schrifthoehen dick, zum Ziehen), Umschalt+Mausrad ging schon.
- Alt+Z schaltet den Umbruch wie in VS Code, die Datei bleibt. Welche Vorgabe mehr zeigt, ist gemessen: `ui/ingameforceselect.menu` bei Vorgabebreite ohne Umbruch 619 Zeichen im Bild, mit Umbruch 691. Darum ab Werk an. Neuer Schluessel `menueTextUmbruch`, sonst schaltete die alte Einstellung (immer 0 gespeichert) ihn bei allen wieder ab.
- `BHED_MENUE_NUR=textbreite`: rot 7 Fehler, gruen 10/10. Einmal lief der Test nicht zu Ende (Bluescreen 07:05), wiederholt. Foto: `SP/rc608/nachher_p2_texteditor.png`, `SP/rc608/nachher_p2_umbruch.png`.

### Punkt 3: Events neu gestaltet
- Jede Aktion ist ein Block: erste Zeile Befehl und Ziel, Hoch und Loeschen immer an fester Stelle rechts, darunter die Werte. `transition`: Gruppen "Von" und "Nach" (x, y, w, h), dazu Zeit und Schritt; die Woerter in der Datei bleiben in der Folge von Script_Transition. `setitemcolor`: Ziel, Eigenschaft, Farbfeld und R, G, B, A. Klaenge: Beschriftung, Feld (hoechstens 24 Schrifthoehen), Abhoerknopf. Hover-Effekte brechen um. Die Bausteine sind dieselben wie in Properties.
- `BHED_MENUE_NUR=events`: Rechtecke bei 300, 500, 1000 Punkten, 100 %, 200 %, 6 Sprachen (36 Zeilen). Jedes Bauteil im Feld, Hoch und Loeschen paarweise da. Dazu: Das Feld w von "Nach" aendert genau das achte Wort (`transition knopf 10 10 100 30 6 6 77 38 10 1`), Rueckgaengig stellt es her. Rot 37 Fehler (ScrollMax bis 1306), gruen 38/38. Unter der Mindestbreite gilt die Mindestbreite aus Punkt 13.
- Gefunden dabei: `daneben` (SameLine + NewLine) zaehlte den Abstand als Inhalt, das Feld war dadurch 12 Punkte zu schmal. Jetzt misst es vorher.
- Properties nutzt dieselben Bausteine (Punkt 13). Fotos: `SP/rc608/vorher_p3_events_schmal.png`, `SP/rc608/nachher_p3_events_schmal.png`, `SP/rc608/nachher_p3_events_breit.png`.

### Punkt 9: Hintergrund eines leeren Menues
- Nachgestellt im laufenden Spiel (Movie Duels, SP und MP, Fotos): SP `fullScreen 1` ohne background: schwarz; mit `backcolor`: die Farbe; mit gescriptetem Shader: sichtbar; mit einfachem Bild ohne `backcolor`: unsichtbar (Alpha 0, whiteShader-Stufe); `fullScreen 0` aus dem Hauptmenue: dessen Hintergrund. MP `fullScreen 1` ohne background: grauer Standardkasten (`backcolor` ohne Wirkung); mit background: das Bild; `fullScreen 0` aus dem Hauptmenue: schwarz.
- Warum schwarz (shank): `Menu_Paint` in SP zeichnet den Griff mit `backColor`, ohne `backcolor` ist Alpha 0. Dazu laesst `_UI_IsFullscreen` im Vollbild das Spielbild weg. Durchsichtig wird es mit `fullScreen 0`.
- `Maler::menu()` wie `Menu_Paint`: SP mit `backColor`, MP mit Griff 0 und ohne Bild der Standardkasten. Neues MP-Menue bekommt ein schwarzes Element `hintergrund` wie die Vorlagen. Diagnose `VollbildOhneBild` nennt den Kasten. Eigenschaften ohne Auswahl: Schalter Vollbild/Ueberlagerung mit Erklaerung je Spiel. Schachbrett nur Anzeige, abschaltbar, nie in der Datei.
- Rot: ctest `menuetest` 3 Fehler (`SP fullScreen mit backcolor fuellt`, `SP background ohne backcolor: Alpha 0`, `MP fullScreen ohne background: der graue Kasten`), gruen 0. Der Selbsttest `BHED_MENUE_NUR=leer` konnte vorher nicht rot laufen, weil die Schnittstelle fehlte: gruen 5/5 mit MD und JKA. Vergleich Spiel gegen Vorschau nur per Foto: `SP/rc608/nachher_p9_spiel_vorschau.png` (Beschriftung im Testmenue unterscheidet sich, sonst gleich), Schalter `SP/rc608/nachher_p9_schalter.png`. Kein Vorher-Foto; das Vorher zeigt der Rotlauf.
- Offen: im laufenden Spiel (academy1, md_am_sith beginnen mit Zwischensequenz, `+uimenu` oeffnet dann nichts), MP in der Partie, Original-JKA/JK2 (Steam nicht angemeldet).

### Punkt 7: Changelog nach Bereichen
- `tools/changelog.txt`: `### Bereich` je Sprache; jede Sprache einer Fassung muss gleich viele Bereiche haben, sonst bricht `gen_changelog.py` ab. Neu `changelog::gruppen()`. Das Fenster zeigt je Fassung aufklappbare Gruppen, was keinen Bereich hat, steht unter "Allgemein".
- rc607 ist umgestellt (9 Bereiche in 6 Sprachen, dazu der Einleitungssatz unter "Allgemein"). Aeltere Fassungen bleiben flach (Sammelgruppe): Die einmalige Umstellung nimmt einen Vorsatz nur, wenn er in der Fassung mindestens zweimal steht und alle Sprachen gleich viele Bereiche ergeben. Das traf keine aeltere Fassung (rc606: en 2 Gruppen, de 1; rc605: nur en und de haben Vorsaetze), also nichts geraten.
- Release-Notizen: `notizen` behaelt die `### `-Zeilen (Fixture im Selbsttest von `tools/release_paket.py`).
- Rot: `BHED_MENUE_NUR=changelog` 6 Fehler (je Sprache), `release_paket.py selbsttest` 1 Fehler. Gruen: 6/6, 0 Fehler, ctest `changelog_sprachen` gruen (2 neue Pruefungen). Fotos: `SP/rc608/vorher_p7_changelog.png`, `SP/rc608/nachher_p7_changelog.png`.
- Die Zeilen fuer rc608 bleiben bis zum Release in diesen Notizen; beim Eintrag in `tools/changelog.txt` gehoeren sie unter die Bereiche.

### Kleinigkeit: Engine messages
- Antwort an shank: Der Abschnitt zeigt, was der Parser des Spiels beim Lesen zu dieser Datei sagt, Wort fuer Wort; ein Klick springt zur Zeile. Jetzt steht das als Satz da, bei leerer Liste "Keine Meldungen". Lange Meldungen brechen um, statt abgeschnitten zu werden; die ganze Meldung bleibt die Klickflaeche.
- `BHED_MENUE_NUR=breiten`: rot 4 Fehler (`Keine-Meldungen vor 3592 Bildern`, `##meldung` nicht gefunden), danach einmal rot 1 (die Klickflaeche ragte um den halben Abstand hinaus), gruen 0. Fotos: `SP/rc608/vorher_kl_meldungen.png`, `SP/rc608/nachher_kl_meldungen.png`.

### Reste aus Teil 1
- Pfeiltasten und Entf mit Fokus in der Hierarchie: Test in `BHED_MENUE_NUR=hierarchie` (Pfeil 1, Umschalt+Pfeil 10, Rueckgaengig, Entf, Rueckgaengig). Lief beim ersten Mal gruen (14/14): Es war kein Fehler, nur nicht geprueft. Der Test bleibt.
- Tutorial "Dein erstes eigenes Menue": Seit Punkt 11 setzt ein einfacher Klick nicht mehr ein, Schritt 3 hing darum (`BHED_MENUE_NUR=tourmenue` rot 8). Der Text sagt jetzt Doppelklick oder Ziehen, der Test ebenso.

### Abschluss Teil 2: voller Lauf, Handbuchtexte, Reste
- `missionsbau` einmal nach dem Umbau der Start-Bauteile: 95/95, gruen ohne Aenderung.
- Der volle Lauf `menueeditor` enthaelt jetzt alle Schritte von rc608. Erster Lauf: 31 Fehler, alle im Test, keiner im Programm: Abschnitt Layout liegt unter den neuen Feldern und war ausserhalb des Bilds (M5, jetzt wird gerollt); das Klangfeld heisst `##klickklang`, die Beschriftung steht darueber (M5); Griffe des Modells sind eine Reihe oben links (M7); zwei Druecke an derselben Stelle waren ein Doppelklick und schrieben (`ziehFlaeche` wartet jetzt die Doppelklickzeit ab); die Zoomstufen begannen ausserhalb der Mitte, weil die rechte Spalte breiter ist und die Flaeche je Sprache anders breit (jetzt Strg+0, Strg+1, dann `frei` in die Mitte); das Referenzbild aus M6 lag unter allen spaeteren Fotos (wird entfernt).
- Ergebnis am Ende: voller Lauf 1054/1054 mit Movie Duels (en, de, zh-Hans vor der letzten Testaenderung) und mit JKA; ja hatte vor der letzten Testaenderung 4 Fehler in der Zoomstufe 800 % (das Element ragte aus der Flaeche), danach `zoom` mit ja und en 85/85; es wurde abgebrochen, cs lief nicht (Abbruch der Bilder auf Wunsch von shank). JK2: `felder` 18/18, `modell` 9/9, `leer` 5/5, dort zuerst 5 Testfehler (JK2 hat keine Animationsliste, das Logo steht auf 90 Grad). `missionsbau` 95/95, `hierarchie` 14/14, ctest 192/192 (dabei `src/hilfe_orte.inc` neu erzeugt).
- Handbuchtexte in 6 Sprachen stehen (Widgets und fertige Bauteile, Zoom, Mauszeiger, Schreiben an Ort und Stelle, Properties je Typ, Modell-Knoepfe und Reihenfolge von `model_g2anim`, Hierarchie, Events, leeres Menue, Alt+Z, base liest mit, Changelog nach Bereichen). `gen_handbuch.py` liest jetzt auch `i18n_rc608.py`.
- Handbuchbilder: nach Wunsch von shank nicht aufgenommen, sie kommen einmal am Ende von Teil 3, weil dort die rechte Leiste neu gebaut wird. Bis dahin zeigen die Bilder des Menue-Editors den alten Stand (`type` als Auswahl, Reiter "Text", Griffe am Modell auf den Kanten), die Bildunterschrift von `menue_gizmo` beschreibt schon die neue Reihe. Neue Bilder fuer Teil 3 vorgemerkt, die Fotos gibt es schon im Selbsttest: `menue_schreiben_tippen`, `menue_ablegen`, `menue_properties_anim_gewaehlt`, `menue_leer_schalter`, `menue_hierarchie_seite` (ohne die Einblendungen aus dem Schreiben-Test), `menue_events_breit`.
- Tutorials: nach Wunsch von shank in Teil 3 einmal komplett. Hier nur Schritt "Drei Knoepfe" von "Dein erstes eigenes Menue", weil Einzelklick seit Punkt 11 nichts einfuegt und der Test rot blieb.
- Bemerkt, nicht geaendert: Mit Aufloesung "Window" und "Stretch" streckt die Flaeche 640 x 480 auf das Feld. Das war schon so; mit der breiteren rechten Spalte ist die Flaeche ab Werk bei 2538 x 1472 hochkant (1113 x 1173), ein Knopf wirkt also hoeher als im Spiel. "4:3 with bars" oder eine feste Aufloesung zeigen es richtig. Fuer den Neubau in Teil 3 mitbedenken.

## Teil 3a

Oberflaeche des Menue-Editors neu nach `entwurf_panel/index.html`, dazu die linke Leiste (freigegeben: "linke seite passt") und die Reste aus Teil 2. Teil 3b (Gesamtpruefung, Tutorials, Handbuchbilder) bleibt liegen. Vorgaben fuer Fotos: Spielfenster 1920 x 1440, Vergleiche ein Bild je Paar, Einzelbild 1920 breit.

### Rechte Leiste
- Reiter nur noch Properties, Events, Text editor (Nummern bleiben semantisch: 1 Text, 2 Properties, 3 Events). Karte mit Symbol, Name, Typ und einer Knopfreihe ausserhalb des rollenden Teils, Feldsuche, File keys, Gruppen mit Zusammenfassung, Farbzeile, Umschalter, Zahlen tippen oder an der Beschriftung ziehen, Hilfezeile, Fuss mit Component, Preview cvars, Engine messages. Geaenderte Werte markiert und zuruecksetzbar, zuletzt benutzte Farben.
- Ausrichtung: Es gibt keinen Merker. Die Zahlen selbst entscheiden (`textXFolgt`, `textSenkrecht`); wer die Zahl von Hand aendert, hat damit das Folgen abgeschaltet. Beim Skalieren bleibt sie auch senkrecht.
- Senkrecht rechnet jetzt mit den Massen von `RE_Font_DrawString` (`textMasse`: Grundlinie und Oberkante der grossen Buchstaben). `Text_Height` allein traf die grossen Schriften nicht (Spielfoto).
- Events: Ausloeser, Schritte in Worten, Add step, Try. Umbenennen zieht die Ziele mit (`itemUmbenennen`), fehlendes Ziel gelb und in der Pruefung (`zielDa` wie `Menu_ItemsMatchingGroup`).
- Abweichungen vom Entwurf: Fuellung hat zusaetzlich "..." fuer GRADIENT, TEAMCOLOR, CINEMATIC (das Spiel kennt 6 Stile); Rand hat eine 5. Stellung (Verlauf); Bild auch bei FILLED mit vorhandenem background (`Window_Paint`); Anordnen folgt der Lage (vorhandene Layouts, `jkstudio:layout`), nicht der Wahlfolge; Arrange enthaelt zusaetzlich Ausrichten, Verteilen, Gruppieren; das Hierarchie-Auge bleibt reine Vorschau, das Karten-Auge schreibt `visible`; Hover-Klang ist `focusSound`, Klick-Klang `play` in `action`; Loeschen von Vorlagen ueber eine Zeile "Zurueckholen"; ein Hover-Farbfeld zusaetzlich; das Menue hat zusaetzlich den Ausloeser Enter (`onAccept`, den liest das Spiel); die Layout-Knoepfe "Zeig mir" entfallen.

### Bild und Video, Bauteile, linke Leiste
- Bild und Video: erst ein Rahmen (`istBildRahmen`, Video mit Kennung `// jkstudio:rahmen video`, solange leer), Doppelklick oder Knopf oeffnet die Auswahl. Der Merker ist noetig, weil ein leerer Video-Rahmen in der Datei sonst nicht von einem Bild-Rahmen zu unterscheiden ist, und faellt weg, sobald ein Video gewaehlt ist. Video kommt als Shader `menu/video/<name>` mit `videoMap` ins Paket.
- Fertige Teile als Kacheln mit Namen. Hierarchy dauerhaft links, zuklappbar, mit Suche, Auge, Schloss, Strg und Umschalt. Gruppen Show und Player input, Teile-Suche, Hinweis verschwindet nach dem ersten Einfuegen. My templates mit Vorschaubild, Umbenennen, Loeschen (zuruecknehmbar). Save selection nur mit Auswahl. Der doppelte Button-Eintrag ist weg, der Button hat eine sichtbare Flaeche.
- Doppelklick auf eine Kachel: erst beim Loslassen und nur ohne Ziehen. Griff man schnell ein zweites Mal zu und zog, setzte die Kachel zweimal ein (im vollen Lauf aufgefallen). Die linke Spalte gibt nach, bevor die Flaeche unter ihr Mass faellt.
- Text-Editor: Strg+Rad 50 bis 300 %, Strg+0 zurueck, Stufe in den Einstellungen; Treffer, Cursor, Umbruch und Balken je Stufe bei 100 % und 200 % geprueft.
- Flaeche: bleibt immer 4:3. 1920 x 1080 bei 200 %: Die Hoehe begrenzt sie auf 297 x 223; Zoom und Verschieben helfen weiter. Der Test verlangt dort 290 statt der frueheren 320, weil die Flaeche vorher verzerrt war.

### Rahmen uebereinander (shank, mit Foto)
- Ursache: `einrasten` legte die Ecke beim Ablegen exakt auf die eines gleich grossen Elements; der Stapelschutz galt nur fuer Doppelklick. Einfuegen und Duplizieren legten die Kopie immer genau auf das Original.
- `ohneDeckung`: liegt das Neue genau auf einem gleich grossen, rueckt es um 20, 20, bis es frei ist (wie beim Doppelklick). Einrasten an Kanten bleibt, Alt schaltet beides aus, der Rahmen beim Ziehen zeigt die echte Stelle. Einfuegen, Duplizieren und Ruecken sind ein Schritt (`einfuegenOhneDeckung`).
- `rahmenPlatzhalter`: Der Text steht nur, wenn kein spaeterer Rahmen ihn bedeckt. Die Datei bleibt unberuehrt.
- Rot `BHED_MENUE_NUR=stapel`: 3 OK, 5 FEHLER (`Platzhalter fuer 0 1 2 3`, abgelegt auf 100, 100). Gruen 8 von 8, dazu das schnelle Zweitgreifen.

### Spielvergleich (Movie Duels, nur per Foto)
Vergleichsbilder: `SP/rc608/3a/<name>_spiel_vorschau.png`, oben Spiel, unten Vorschau.
- SP Texte und Schriften: gleich. Senkrechte Mitte: vorher sass Schrift 3 falsch, jetzt gleich.
- MP Schriften: Die Vorschau las in Movie Duels den Menuesatz von Jedi Academy. Das Spiel liest `ui/MovieDuels-MP-Menus.txt`; dort ist font 2 `albertus_bold` und font 3 `ergoec`. Rot `mpschrift`: "font 2 = ergoec, font 3 = anewhope". Gruen: "font 2 = albertus_bold, font 3 = ergoec". Foto danach gleich.
- Vorgabe des Knopfs in Movie Duels MP: Ohne Nummer stand dort albertus_bold, fast viermal so gross; der Text ragte aus dem Knopf (Foto). Neue Elemente und fertige Teile tragen dort `font 3` (`vorlageMitSpielSchrift`, ctest `menuewerkzeug`, `menuebauteile`). Sonst gleich: Flaeche, Mitte, Farbe.
- Modell: In SP gleich (Quotient Vorschau zu Spiel 0,92 bis 1,00 bei 1920 Punkten; Hemd, Hose, Kopf). Im MP war die Vorschau 1,6- bis 2-mal zu hell: `codemp/ui` setzt kein Licht am Augpunkt, `code/ui` schon, und Movie Duels hat das Mindestlicht entfernt (`tr_light.cpp`: 150 statt 182). Nach `ModellLicht` Quotient 0,96 bis 1,04. shanks Eindruck "dunkler" im SP-Foto kam vom schwarzen Hintergrund gegen den bunten, das bleibt. Das MP-Licht von OpenJK (182) ist aus dem Code, nicht fotografiert.
- Rahmen ohne Bild: gleich (`gfx/menus/menu_box1`-Rest erledigt: grauer Kasten des Standard-Shaders mit weissem Rand). Video: im MP gleich. Im SP stand es zuerst auf dem ersten Bild, weil das Hauptmenue dahinter offen blieb und selbst ein Video zeigt: Die Engine spielt nur einen Griff zur Zeit (`cin.current_handle`), zwei laufende Videos setzen einander zurueck. Mit geschlossenem Hauptmenue laeuft es. Die Vorschau spielt beide; das ist im Abschnitt Offen notiert.
- Leeres Menue im laufenden Spiel: SP auf `duel_kor`, MP in der Partie auf `mp/ffa3` (`ui_openmenu`, Menueliste in der Partie: `ui/MovieDuels-MP-Ingame.txt`; MP braucht `s_initsound 1` bei Lautstaerke 0, sonst bricht es beim Laden der Karte ab). SP: `fullScreen 0` zeigt das Spiel, `fullScreen 1` schwarz, mit background das Bild, mit backcolor die Farbe. MP: `fullScreen 0` das Spiel, `fullScreen 1` der graue Kasten, auch mit backcolor, mit background das Bild. Alles wie in Teil 2 aus dem Code gebaut. Fotos `SP/rc608/3a/leer_sp_*.png`, `leer_mp_*.png`. Original-JKA: offen, Steam laeuft nicht.

### Weitere Reste
- Eigener @-Text an Ort und Stelle: Rot `cschreiben` (kein Undo-Schritt, Rueckgaengig falsch), gruen `dschreiben`. Kopieren, Einfuegen, Bild auf und ab: beim ersten Lauf gruen, ohne Aenderung.
- Eigene `animation.cfg`: `BHED_MENUE_NUR=eigeneanim` 2 von 2, beim ersten Lauf gruen. Wirkt nur beim ersten Laden der Spieldaten, darum nicht im vollen Lauf.
- `undo` zehnmal nacheinander: zehnmal 609 von 609. Die Zeile "Properties rect | Fokus Text-Editor" trat nicht wieder auf; die Ursache des einen Rots aus Teil 2 ist damit nicht gefunden.
- Echte exe mit unsichtbarer Konsole (`SP/rc608/p_cli_echt.py`, CREATE_NO_WINDOW): `--version` exit 0, Ausgabe in Datei und Konsole; `--gibtesnicht` exit 2 mit Erklaerung in Datei und Konsole. Der Fall ohne Konsole lief nach Auftrag nicht real.

### Laeufe am Ende
- Voller Lauf `menueeditor`: Movie Duels 2246 von 2246, JKA (`SP/rc608/nur_ja.cfg`) 2246 von 2246. Der volle Lauf enthaelt jetzt alle Schritte von Teil 3a ausser `eigeneanim` (2 von 2 als eigener Lauf) und `spielprobe` (nur Dateien und Fotos). `BHED_MENUE_NUR=a+b` kettet Teile; damit liessen sich die Folgefehler im vollen Lauf nachstellen, alle waren Testzustand (gerollte Kachelliste, offene Suchzeile, liegengebliebene Vorlage, offenes Changelog-Fenster), ausser dem Doppeleinsetzen der Kachel und der zu breiten linken Spalte.
- JK2 (`m5b/behaved_jk2.cfg`): `felder` 18 von 18, `modell` 9 von 9, `leer` 5 von 5. Nur diese drei; der volle Lauf lief nicht mit JK2.
- Alle Sprachen: `leistebreit` 684 von 684 (Properties und Events, jedes Element, 300, 500, 1000 Punkte, 100 und 200 %, 6 Sprachen), `events` und `breiten` ebenso. Der volle Lauf selbst lief nur auf Englisch.

### Rot und gruen
Rot gelaufen: `stapel`, `mpschrift`, `schreiben` (eigener @-Text), Modell MP und Knopfvorgabe (Foto und Messung). Der Neubau selbst hatte keinen getrennten Rotlauf vor dem Bau: Die Tests entstanden mit ihm. Auffaelligkeiten fanden `hleistebreit` (Typ-Marke ragte bei 300 Punkten und bei 200 % aus dem Feld: 217 Fehler, danach 684 von 684) und der volle Lauf (Doppeleinsetzen der Kachel).

## Teil 3b

Gesamtpruefung des Menue-Editors, Tutorials, Handbuchbilder. Funktionstabelle: `docs/intern/rc608_funktionstabelle.md`. Undo-Tabelle: `docs/intern/rc608_undo_tabelle.md`. Neue Teile im Selbsttest (`BHED_MENUE_NUR`): kontext, datei, seiten, satz, zweivideos, widgets, unsinn, kodierung, feldneu, undomehr, anfaenger, eng, tourspiel, touralle (Datei `gui/selbsttest_menue_pruefung.inc`).

### Testregel von shank (waehrend dieses Teils gekommen)
Was gruen war, laeuft nicht noch einmal. Kein voller Lauf `menueeditor` waehrend der Arbeit und am Ende von 3b; er kommt einmal in der Endrunde von rc608. Der einzige volle Lauf `v1` wurde deshalb bei "rc608 Breiten" abgebrochen (1794 OK, 2 FEHLER, beide nur alte Erwartungen, Test nachgezogen). Die Teile hinter "Breiten" hatten vorher einzeln gruen.

### Fehler, jeweils rot und gruen
- Neue Bedienung (Rahmen, Rechtsklick, Speichern unter, Aenderung von aussen, Seiten verwalten): mit Schalter im Test einmal rot (kontext, datei, seiten: 31 OK, 69 FEHLER), danach gruen (80 von 80).
- `freierName` haengte immer `_2` an: rot "Folge x,y,a_2", gruen. Darum heisst das erste Duplikat jetzt `button_2` statt `button_3`; zwei alte Testzeilen nachgezogen (Verhalten geaendert, keine Lockerung).
- Absturz bei `background "Gruesse"` mit Umlauten (Latin-1): schmale `fs::path`-Umwandlung im Lesen der Spieldaten, per Minidump gefunden, rot `u1`, gruen `u2`. `utf8Pfad` nimmt jetzt ungueltiges UTF-8 als Bytes.
- Unsinn in Feldern: `cvarTest ""`, `"`, `showCvar "}"`, `cvar ""` zerstoerten das Menue. Eine zentrale Probe in `aendere()` lehnt jede Aenderung ab, die ein lesbares Menue unlesbar macht; Feld leer entfernt die Zeile. Rot, gruen 330 von 330.
- Umlaute aus Properties standen als UTF-8 in der Datei: rot, gruen (`kodierung`).
- Prueffenster: veraltet und abgeschnitten, jetzt live und umbrechend (rot, gruen).
- 1920 x 1080 bei 200 %: Flaeche 297 x 223; jetzt 457 x 343 (linke Spalte zu, Ansicht-Knopf, Grenzen weg). Die Lockerung `textbreite` 320 auf 290 ist zurueck auf 320.
- Spalten falteten im Dock-Layout nicht (`frei` schloss den Dock-Raum aus): rot, gruen (`eng`).
- Zwei Videos zugleich: 0 Bilder in 1,5 s zu zweit, 46 allein, wie `CIN_RunCinematic` (`cin.currentHandle`); rot, gruen (`zweivideos`).
- Ohne `focusColor` verschwand der Text unter dem Zeiger im Spiel und in der Vorschau wie im Spiel; das Startmenue hat die Farbe jetzt, Properties und Pruefung kennen sie.
- Rahmendicke haengt an der Spielaufloesung (`_UI_DrawSides`: `size *= xscale`): Vorschau gleich, gegen Spielfoto.
- JK2 SP leeres Menue schwarz, JKA MP Menue mit eigener Fuellung deckt den grauen Kasten (`Menu_Paint` ruft `Window_Paint` immer): gegen Spielfoto.
- JK2 Datapad-Knopfflaechen: Vorschau jetzt schwarz wie im Spiel. CLOSED im jk2mp-Servermenue: Faktor 0,2 wie im Spiel.
- `autowrapped` und `wrapped` im Einzelspieler: Die Vorschau rechnete wie codemp. code/ui bricht bei Breite - textalignx um, rueckt um Hoehe + 4 vor, endet am Boden mit "...", `wrapped` Hoehe + 5. Rot, gruen im ctest `menue_vorschau`.
- Tutorial-Fenster (`gui/tour.cpp`): lag bei 1920 x 1080 ueber der Liste von Add step (rot `a1`, `a3`) und bei 200 % ueber dem Knopf des Schritts (rot `a5`). Jetzt hoechstens 40 % der Fensterbreite, haelt Klappfenster, die rechte Spalte, wenn der Schritt dort zeigt, und die Elemente auf der Flaeche frei; ohne freie Ecke die Stelle mit der kleinsten Ueberdeckung. Gruen `a15` (en, cs, ja bei 100 %).
- Tutorial: Der Hinweis "gerade verdeckt" war abgeschnitten, er bricht jetzt um. Neues Projekt beginnt immer im Bearbeiten (stand noch Bedienen, schlug der Rahmen in Schritt 4 fehl, rot `a3`, gruen `a15`). Export-Klick ohne Windows-Dialog im Test (`setzeSpeichernDialog`).
- Rechte Spalte: Anderes Element gewaehlt beginnt wieder oben (die Ausloeser lagen sonst ueber dem Rand); die aufgeklappte Liste von Add step rollt ins Bild.
- Grenzen gemessen statt geschaetzt: jasp 150, jamp 256 (OpenJK 512, Warnung `NurOpenJk`), jk2sp 128, jk2mp 256, MD SP und MP 4096 (bis 600 im Spiel gesehen; 4097 meldet "unknown menu keyword", ein Element bei 1024 sichtbar). `MAX_MENUFILE` gilt nur fuer `GetMenuBuffer`/teamFile, `MAX_MENUDEFFILE` ist unbenutzt, darum keine Pruefung der Dateigroesse.

### Punkte aus 3a
- Undo "Properties rect | Fokus Text-Editor": Code gelesen, keine Reihenfolge- oder Zeitabhaengigkeit gefunden. Kandidaten, nicht bewiesen: Der Weg ueber den Text-Editor macht Undo ein Bild spaeter (`undoWunsch`), Fokusverlust des Fensters setzt Tasten zurueck. `BHED_TEST_BREMSE` (unregelmaessige Bildbremse) ist dafuer neu; Lauf `u10` mit 300 ms: alle Wege von "Properties rect" gut, das Rot kam nicht wieder.
- `// jkstudio:rahmen video`: Das Spiel ignoriert den Kommentar; waehlt man ein Video, faellt die Zeile weg. Bleibt so.
- Tutorial "Leuchten beim Hover": Schritt waehlt den Ausloeser, der Text nennt den Hover-Knopf, Tour-Anker `trighover`.
- 50 unbenutzte `Me*`-Texte und 36 Eintraege in es/cs entfernt (mehr als die gemeldeten 25 und 22, gemessen ueber den Generator und `lint_i18n`).
- Zwei Videos: portiert, siehe oben.
- MP-Licht OpenJK (182): Keine OpenJK-exe ohne Steam-Start. Vergleich mit Original-jamp (`ja_mp_jks_modell3`): zweite Figur gleich hell.
- `data/karte/bt/bt-el2.txt`: Kein Test und kein Werkzeug loescht in `data/karte/bt` (nur `app_karte_plugins.cpp` liest). Vermutlich Bluescreen-Folge, nur so erklaerbar, nicht bewiesen.

### Anfaenger-Durchgang: unklar, geaendert
- Hintergrund laesst sich aus Versehen ziehen: Dekoration ueber den ganzen Schirm laesst Klicks durch, Wahl ueber Hierarchie oder Rechtsklick.
- Wie waehle ich mehrere: Rahmen auf der Flaeche, Strg+A.
- Rechtsklick tat nichts: Menue auf Element, Flaeche und Hierarchiezeile, mit Liste von allem unter dem Zeiger.
- Warum oeffnet das Spiel mein Menue nicht: Menueliste im Paket von selbst, Im Spiel testen oeffnet das Menue der offenen Seite, Pruefung "nichts oeffnet dieses Menue".
- Text verschwindet unter dem Zeiger: focusColor im Startmenue und als Feld "Text unter dem Zeiger".
- Was heisst MP-Badge oben: Tipp am Badge.
- Wozu cvarTest und showCvar: eine Zeile "Bedingung" in Worten (zeigen, verbergen, benutzbar, unbenutzbar).
- Mehrfachwahl: Liste aus Text und Wert statt Skript von Hand.
- Listen und Spielinhalt: eigene Gruppe mit Worten fuer ownerdraw, feeder; im SP nur Zahlen, weil das Spiel dort keine Namen kennt.
- Namen in der Landessprache im Menue: neue Teile heissen englisch (back, title, background, my_menu).
- Speichern: Strg+S und Speichern unter fehlten am Knopf.
- Kleines Fenster: Spalten klappen, Ansicht hinter einem Knopf.
Der Teil `anfaenger` prueft zusaetzlich, dass jedes Feld und jeder Leistenknopf einen Tipp hat.

### Abweichungen vom Entwurf (`entwurf_panel/index.html`)
- Speichern unter steht in der Leiste, nicht auf der Karte.
- Gruppen `auswahlen` und `spielinhalt` sind neu (der Entwurf kennt sie nicht); ohne sie gaebe es diese Felder nur im Text-Editor.
- Fuellung, Rand und Bild wie in Teil 3a.

### Tutorials
- "Dein erstes eigenes Menue" (`tut-menue-eigen`, 10 Schritte) und "Ein Menue aus dem Spiel aendern" (`tut-menue`, 5 Schritte, neu mit echten Klicks statt Funktionsaufrufen, `tourspiel`): von Anfang bis Ende per Klick, gruen in en, cs, ja bei 1920 x 1080 und 100 % (Lauf `a15`). Sprachwahl nach der neuen Testregel: laengste Texte und breite Zeichen; de, zh, es liefen nicht einzeln durch.
- Eigenes Menue im MP-Format (`BHED_TOUR_MP=1`) mit Movie Duels, JKA, JK2 durchgelaufen; das Testpaket zeigt im laufenden Spiel die drei Knoepfe: MD SP, MD MP, jasp, jamp, jk2sp, jk2mp. Im MP-Format blieb eine Testzeile rot, die Layout-Pruefung selbst (sie las den Text ohne die Definitionen aus `menudef.h`); sie liest jetzt die Lagen des Editors, das MP-Format lief danach nicht noch einmal. Die SP-Fotos stammen aus einem frueheren Bau dieses Teils.
- Die Texte fordern drei Knoepfe, die Tour geht beim ersten weiter (bewusst: sonst haengt sie, falls jemand nur zwei will).
- 200 %: Der Schirm hier hat 2560 x 1600, das Fenster ist bei 2564 x 1570 am Ende; 4K laesst sich nicht pruefen. Bei 2564 x 1570 und 200 % (cs) kommt der Test bis Schritt 7, dann findet er den Eintrag `exec` in der Liste von Add step nicht (Lauf `a14`, Ursache offen). Bei 1920 x 1080 und 200 % (960 x 540 Punkte) reicht der Platz fuer Tutorial und Spalten zugleich nicht.

### Nur per Foto oder Bildvergleich gegen das Spiel
Spielfotos unter `SP/rc608/3b/orig/` (`SP` = Scratchpad dieser Sitzung): leere Menues, Modell, Rahmendicke, Datapad JK2, jk2mp-Listen, Tutorialmenue im Spiel.
JK2-Datapad: `jk2_sp_dpforce` und `jk2_sp_dpinv` waren falsch (zeigten die Mission, weil `+uimenu` diese Seiten nicht oeffnet) und sind neu in 1920 x 1440. Force ueber `cg_updatedDataPadForcePower1` wie `UI_DataPadMenu`, Inventar ueber ein loses `datapadmission.menu`, das weiterleitet. Vergleich `jk2_dp_force_inventar_spiel_vorschau.png`: Rahmen, Knoepfe, Leiste und Textfeld gleich; Beispielwerte verschieden (Vorschau Force Jump, Spiel Force Heal; Vorschau leeres Inventar, Spiel `give all`).
Die Fotos der Originale haben sonst 1280 x 960: jk2sp, jamp usw. sind nicht DPI-faehig und sehen diesen Schirm (2560 x 1600 bei 150 %) als 1707 x 1067, das Fenster passt dort nur bis zu dieser Groesse. Mit `__COMPAT_LAYER=HighDpiAware` nur fuer den Prozess gehen 1920 x 1440; bestehende Fotos wurden nicht nur deshalb neu gemacht.

### Handbuchbilder
- Alle Bilder des Menue-Editors einmal in 6 Sprachen: `BHED_MENUE_BILDER=1` (Fenster 2560 x 1500, schmale Spalten, die Flaeche ist der groesste Teil des Bilds, gespeichert 2538 x 1444), dazu das Datapad mit den Daten von Jedi Academy und die Tutorialschritte von "Ein Menue aendern" (`BHED_HB_NUR=hilfe2`). `menue_eng_zu` hat bewusst 1898 x 1024: Es zeigt 1920 x 1080.
- Die Testzeilen dieses Laufs schlagen teils fehl (rund 55 je Sprache): Sie sind auf die breite Spalte und das kleinere Fenster ausgelegt. Nur zwei Bilder waren davon betroffen (`menue_leiste_addstep`, `menue_rahmen_wahl_sp`, nach Sichtung erkannt), sie haben den eigenen Teil `bildnach` (0 Fehler). Alle Bilder wurden je Sprache auf Fehlen geprueft, durchgesehen nur eine Auswahl (rund 20 Bilder in en, Stichproben in cs, ja, zh). Die uebrigen sind nicht einzeln angesehen.
- Bewegte Bilder neu mit `BHED_HB_FENSTER=3840x2160@2 BHED_FOTO_NUR=a_`; das Fenster endet auf diesem Schirm bei 2564 x 1570. `a_k_setzen`, `a_k_timen`, `a_roff` haben 1920 Breite. **Unter 1920 bleiben die Ausschnitte kleiner Bereiche:** `a_k_pilot` und `a_kamera` 1191, `a_einmal` 971 (ein Knopf oder eine Liste, kein ganzes Fenster; hochrechnen waere nur Unschaerfe). `a_kamera` gibt es weiter nur auf Englisch. Die Symbole `i_*` bleiben, wie sie waren.
- Anzeige: `stil.css` zeigt die Bilder in voller Spaltenbreite (Spalte 1640 statt Bilder bis 1060), ein Klick oeffnet die volle Groesse, auch bei bewegten Bildern.
- Groesse: Handbuch im Repo 248.2 MB vorher, 280.6 MB nachher (Bilder 223.3 MB, 255.5 MB). Paket (`release_paket.py zip`): 286.5 MB nachher; vorher rund 254 MB (Handbuch und `data/base` als Zip 227,9 MB plus Programme, mit derselben exe gerechnet, kein eigener Bau von rc607).

### Pruefstand am Ende
ctest einmal am Ende: 190 von 192 gruen, `hilfe_orte` (Generator nicht gelaufen) und `menue_bauteile` (alte Namen im Test nach den neutralen Namen) nachgezogen, beide einzeln gruen. Keine Warnung im Bau. Danach nur noch Testschritte fuer Bilder und Namen im Test geaendert. **Nicht im Gesamtlauf bestaetigt (Testregel, Endrunde rc608):** voller Lauf `menueeditor` mit Movie Duels und mit JKA, Gesamtlauf aller Modi. Der abgebrochene Lauf `v1` kam bis "rc608 Breiten"; die spaeteren Aenderungen an `tour.cpp`, `tourKlapptAuf`, Spaltenrollen und `autowrapped` sind nur in `tourspiel`, `touralle`, `eng`, `textbreite`, `m5b`, `undo` (nur Properties rect, mit Bremse) und im ctest gelaufen. Die Texte `MeEigenschaften` und acht weitere wurden entfernt, ohne danach die Sprachlaeufe zu wiederholen (der Bau und `lint_i18n` sagen, dass nichts sie braucht).

### Lockerungen
- Neue Griff-Tests erlauben +-1 Punkt (die Maus liegt auf Bildpunkten, neuer Test, keine alte Stelle).
- Keine alte Teststelle gelockert. `textbreite` wieder 320. Alte Erwartungen geaendert, weil sich das Verhalten geaendert hat: `button_2` statt `button_3`, `letzterExport() == 2 + Listen`.

## Teil 4: Geschuetze

Zweig `pruefung4b` (7 Commits) ist in karte1 (Konflikte: `CMakeLists.txt`, `abwurf.h`, `abwurf.cpp`; `wirfObjekt` merkt seit dem Merge die Bahn s.pos statt des Orts, wie Pruefung 4a es fuer `AbwurfBahn::bei` eingefuehrt hat). Der Entwurf `p4b_t3_spiel_wip.patch` ist eingespielt und fertig gebaut.

- `gui/app_view3d_geschuetz.inc`: `GeschuetzSim` in der Vorschau. Spuren (`schussSpurBei` nach SV_ClipMoveToEntities mit G2_COLLIDE, Normale aus der Kollision, traceLod 10), gi.inPVS, gi.pointcontents, Bolzen und Netze der Geschuetzmodelle, Trefferort aus der getroffenen Flaeche. Regelwerk OpenJK, Movie Duels und Jedi Outcast folgt der Einstellung; SnapVector rundet bei Movie Duels wie x64 `(int)`, sonst wie die x86-exe.
- `hurtTodeBerechnen` laesst die Geschuetze je Bild mit den Figuren laufen und traegt den Schaden aus: Figuren (damageModifier nach Trefferort, Ruestung), andere Geschuetze (TurretPain, turret_die, ObjectDie, CANRESPAWN), func_breakable und misc_model_breakable (`AblaufWelt::schadenBrueche`, Name `\x02<Entity>` im Nachbau und in MoverSim: ohne usescript und USE_NOT_BREAK), misc_trip_mine (Fremdschaden in `fallenKette`), G_RadiusDamage auf alles davon. fx_target_beam trifft jetzt auch Kartenentities. G_UseTargets der Geschuetze laufen ueber den Nachbau. Der Block steht jetzt hinter `loadActorModels`, weil die Spuren die Netze der Figuren brauchen.
- `SET_NOTARGET` und `SET_PLAYER_TEAM` gelten; misc_turret und misc_turretG2 hoeren auf use.
- Zeichnung: s.apos, Knochenwinkel, s.frame, Freigabe; Geschosse je Bild am Bahnort mit verkuerzter Richtung in den ersten 75 ms; Muendungsblitz, Einschlag, Funken, Explosion als Effekte; Klaenge (ping, startup, shutdown, move/turn, fire1).
- **Gefunden im Spielvergleich:** Die Wurzel jedes Ghoul2-Skeletts ist in der Engine nicht die Einheit: `identityMatrix` in `tr_ghoul2.cpp:92` ist (0 -1 0 / 1 0 0 / 0 0 1). Die Figuren hatten das als 90-Grad-Zuschlag in `baueFigurWelt`, die Gegenstaende nicht: Geschuetze zielten eine Vierteldrehung daneben, und alle GLM-Gegenstaende, Abwuerfe und der Bolzen der Ionenkanone lagen seit rc602/rc604 um 90 Grad falsch. Jetzt `g2WurzelWelt` und `g2WurzelBolzen`.
- `figurNetzTreffer` kennt G2_COLLIDE und die groebste LOD; `g2StrahlKollisionen` ist G2_TracePolys fuer Spuren ohne Kasten (Turbolaser). Das doppelte `g2WinkelMatrix` in `geschuetz.cpp` ist weg (das aus `gla.cpp` gilt).

Tests:
- Selbsttest `geschuetze` (neu, je Punkt eine Mission): Rot mit abgeschalteter Simulation 1 OK, 9 FEHLER. Gruen 10 OK, 0 FEHLER. Die eine OK-Zeile im Rotlauf ist die Regelwerkprobe (misc_turretG2 gibt es unter OpenJK nicht), die haengt nicht an der Simulation.
- Ctest `pruefung4b`: Abschriften von pas_think, turret_base_think, turret_aim, G_RunMissile, G_MissileImpact, turret_die, ObjectDie, CANRESPAWN, damageModifier, G2_SegmentTriangleTest. Diese Pruefungen hatten keinen Rotlauf gegen die Bibliothek (die gab es schon); zwei Fehler in meinen eigenen Abschriften wurden dort behoben (AngleNormalize360 schneidet auf 360/65536 ab; die Teststrecke schoss am Kasten vorbei).
- Geaenderte Teststelle: Der Wurf in `pruefung4b` (5c) vergleicht jetzt die Bahn s.pos statt currentOrigin. Gezeichnet wird seit 4a s.pos; im Bild des Abpralls unterscheidet sich das. Keine Lockerung der Formel.
- Toleranz im neuen Selbsttest: 14 Grad x 5 und 3 Grad mit 0,05 Spiel, wegen AngleNormalize360.

Spielvergleich (eigene Testkarten nur im eigenen Ordner: Geometrie der Vorlage, neuer Entity-Lump; Fenster ausserhalb des Sichtbaren):
- Movie Duels, 1920 x 1440, Karte aus md_am_sith mit misc_turret und misc_sentry_turret, der Spieler als Ziel: Geschuetz schwenkt auf den Spieler, Geschosse, Funken am Treffer, Wachgeschuetz mit gruenem Leuchten; Vorschau gleich. Foto `paar_md_turm.png`.
- Original-JKA (Steam angemeldet), Karte aus t3_byss: Der Lauf zeigt auf den Spieler, Modell und Haltung wie in der Vorschau. Das Fenster blieb bei 1280 x 960, auch mit `__COMPAT_LAYER=HighDpiAware`. GameData danach wie vorher (Liste verglichen). Foto `paar_org_byss.png`; die Kamera des Spiels steht nicht deckungsgleich, der Blick im Spiel geht nach oben rechts und der Abstand ist groesser, also nur Ausrichtung und Modell verglichen.

Offen:
- Bilder der Geschosse und Schadenszahlen sind im laufenden Spiel nicht gemessen, nur per Foto angesehen. Ein Treffer an einer bestimmten Stelle des Koerpers (Trefferort aus dem Netz) ist nur gegen die Formel geprueft.
- Figuren und Geschuetz waren in der Vorschau heller als im Spiel: behoben in Teil 5.
- Der Feuerfaecher der Treffer: geklaert in Teil 5.
- Der Spieler ist nur Ziel, wenn die Szene ihn enthaelt (`affect player`); das Spiel hat ihn immer.
- SET_INVISIBLE gilt hier als abwesend; im Spiel bleibt die Figur fuer G_RadiusList da und nur contents 0.
- Nicht nachgebildet: G_StartFlee (KI-Flucht vor dem Wachgeschuetz), Areaportale in gi.inPVS (Tueren), Schwertabwehr von Geschossen (CONTENTS_LIGHTSABER), CG_AddGhoul2Mark (Brandmal auf der Figur), Rueckstoss und Schmerzanimation, paintarget von func_breakable, func_glass als Ziel, DoImpact geworfener misc_model_breakable gegen Figuren (5c-Rest).
- Die 90 Grad gelten jetzt auch fuer alle anderen GLM-Gegenstaende: per Quelle begruendet, per Foto nur fuer die Geschuetze bestaetigt.

## Teil 5: Figurenlicht und Rest aus Pruefung 4a

### Figurenlicht
Ursachen, alle gegen die Quelle und das laufende Spiel:
- `mapBrightness` stand auf 2,0 und `mapMinLight` auf 0,10. Das Spiel hat r_mapOverBrightBits 0 und kein Grundlicht (Cvars im Spiel abgefragt: r_gamma 1, r_intensity 1, r_overBrightBits 0, r_mapOverBrightBits 0, r_ambientScale 0,5, r_directedScale 1). Vorgabe jetzt 1,0 und 0, die Regler bleiben als Hilfe, der Regler hat einen Hinweis.
- `sampleLightGrid` ohne r_ambientScale 0,5. Der Mindestzuschlag war fest 32, Movie Duels nimmt 8 (RF_MINLIGHT 76), OpenJK 32 (RF_MORELIGHT 96). Umgebungslicht jetzt auf 255 geklemmt, totalFactor nur unter 0,99, ein leerer Stil beendet die Stufen, alle Punkte in Waenden geben nur den Zuschlag statt der alten Schattierung, ohne Gitter 150/150 mit der Vorgaberichtung der Sonne. Gittergroesse falsch: OpenJK kein Gitter, Movie Duels R_FindCompilerGridSize.
- `addDynamicLights` rechnete `dl->color * 255`: Das Spiel nimmt `VectorMA(directedLight, d, dl->color)` mit Farbe 0..1. Daher die gelben Baender auf der Figur.
- Die Figur war eine andere: Spieler jetzt nach G_InitPlayerFromCvars, Movie Duels `jedi_hm`, OpenJK `jedi_tf`, beide `|head_a1|torso_a1|lower_a1` (RE_SplitSkins, neu `hautAusDreiTeilen`), g_saber `single_1`, und er haelt das Schwert (ClientSpawn).
- RB_CalcDiffuseColor rechnet in ganzen Bytes: im Shader jetzt auch.
- r_gammaShaders 1 in Movie Duels ("GAMMA: shader" im Protokoll, die 3D-Tabelle steht in der DLL, nicht in unserer Quelle): 64 Stufen ohne Halbtexel-Ausgleich, dunkle Werte fallen um rund 2 Bytes. Formel aus dem Fragmentprogramm der DLL und den Fotos, `spielGammaWert`, nur unter Movie Duels. Die Original-exe meldet "GAMMA: hardware", OpenJK hat es in unserer Quelle nicht.
- Anisotrop 16 im Abtaster ("16 of 16" in Movie Duels und Original).
- SET_MORELIGHT und SET_MORELIGHT_PLAYER wirken jetzt.

Messung `r_debuglight` (LogLight) gegen die Vorschau, je Karte der Spielerstart, Vorschau vorher/nachher/Spiel, amb:
md_am_sith 107,3/40,4/40, duel_kamino 98,7/35,5/35, duel_deathstar 124,8/48,6/48, duel_mustafar 123,5/48,1/48, academy1 132,8/52,6/52, duel_hoth 192,2/82,3/81, t3_byss 176,0/74,2/74. dir und Richtung stimmen auf allen sieben (dir bis 1 Byte, Richtung bis 0,01). Im Original-JKA auf t3_byss: amb 115, dir 384, Formel mit +32: 115,7 und 385,7.

Bildvergleich gv_turm-Geometrie, gleiche Kamera, gleiche Figur, Faktor Vorschau/Spiel (R):
Figur 4,24 -> 0,78; Boden 2,19 -> 1,02; hinterer Boden 2,18 -> 0,99; Boden vorn 2,48 -> 1,11; Wand unten 1,99 -> 0,92; Wand 2,68 -> 1,13; dunkle Wand 3,37 -> 1,05.

Tests: Selbsttest `figurlicht` (neu; Rot 1+7 FEHLER = alle 8, gruen 8/0). Ctest `kartengeometrie`/bspgeotest und `efxtest` mit den neuen Zahlen (efxtest rot 2, gruen). `gpushader` prueft `spielGammaWert`.

Geaenderte Teststellen, keine Lockerung: bspgeotest erwartet jetzt die Engine-Zahlen (207 -> 95,5, 232 -> 108, 32 -> 8), efxtest 417,8 -> 1,6384 und 16320 -> 64. Die alten Sollwerte beschrieben den Fehler.

### Funkenfaecher
Geklaert: weder Zeitpunkt noch Normale. Die Normale ist die des Dreiecks wie im Spiel (TransformPoint dreht in MD nur), der Faecher ist die Line von `turret/flesh_impact`. 20 Bilder im Spiel (`fan_spiel.png`) zeigen den Faecher meist nach rechts wie die Vorschau, aber auch nach oben und links, dazu Funken vom Rumpf: Dort zuckt die Figur (Schmerzanimation, Rueckstoss) und die Geschosse treffen andere Dreiecke. In der Vorschau steht sie still und jeder Schuss trifft dasselbe Dreieck. Grund der Streuung ist also die fehlende Schmerzanimation, die bleibt offen (siehe Teil 4). Gruene und rote Lichter auf dem Boden sind in beiden gleich.

### Totenlauf, PM_FallToDeath, NPC_RemoveBody
- `planeTotenlauf`: PM_DEAD ab dem Tod als `nurOrt`-Weg, PM_DeadMove, ps.friction, PMF_TIME_KNOCKBACK, clipmask ohne MONSTERCLIP/BOTCLIP, erstes Pmove 1 ms. PM_CrashLand einer Leiche: fallsplat ab delta 75, sonst bodyfall_human, FALLDEATH1LAND.
- PM_GroundTraceMissed -> PM_FallToDeath im Wegplaner; der Aufprall ist ein Tod mit TodMittel::Fallen. `*falling1.wav`. Eingaben ueber `fallTodWerte`, gleich fuer Ablauf und Szene.
- `leichenEntfernen` nach der Schwerkraft: BodyRemovalPadTime je Regelwerk, Droiden EF_NODRAW, message, Skript laeuft, 128 am Spieler, InFOVFromPlayerView 110/90, NPC_ClearLOS, nur mit self->enemy.
- Lunge von Vader und Desann in PM_TryRoll (Movie Duels), nur gegen die Quelle gelesen: keine eigene Probe, kein Foto, in_camera kennt der Plan nicht.
- Holster in Movie Duels: `saberHolsterLage` mit allen Plaetzen, Griff nur bei anderer Waffe in der Hand. SET_WEAPON: nur ein NPC verliert dabei das Schwert, der Spieler nicht.

Tests: Ctest `leiche` (neu). Rot: Simulation abgeschaltet, Abbruch beim ersten leeren Plan. Gruen alle. Selbsttests pruefung3 9/0, geschuetze 10/0, figurlicht 8/0. Beim ersten ctest war `bewegung` mit 7 Fehlern rot, weil die Plaene von Ablauf und Szene verschieden waren; behoben, dann gruen.

### Nur per Foto gegen das Spiel
Figur und Karte auf md_am_sith, duel_kamino, duel_deathstar, duel_mustafar, academy1, duel_hoth, t3_byss. Leiche nach `kill` auf der md_am_sith-Geometrie: gleiche Lage, nach 28 s noch da (bestaetigt: kill laesst sie liegen). Griff an der Huefte von vorn.

### Abweichung, gemessen statt gelesen
Griff im Holster an der Huefte und Brust: Die Quelle ergibt eine Drehung um X, damit stuende er nach oben. Im Spielfoto haengt er nach unten, darum die Drehung um die Laengsachse; die Basispose von "ModView internal default" war nicht lesbar. Nur fuer HOLSTER_LHIPF per Foto bestaetigt, andere Plaetze und HOLSTER_BACK nicht.

### Offen aus Teil 5
- Figur im Mittel 0,78 bis 1,06 je Karte: geklaert in Teil 6 (Skalierung des Vergleichsbilds, Figurentexturen auf 256 begrenzt).
- r_DynamicGlow: erledigt in Teil 6.
- Schmerzanimation und Rueckstoss: Teil 6.
- Der Spieler mit Gewehr: erledigt in Teil 6.
- Kasten der Leiche, pitch_roll_for_slope, ps.friction: erledigt in Teil 6.
- Waffen im Holster, AvoidGuns, `*holsterorigin`, STUN_BATON: Teil 6.
- tr.sunDirection: erledigt in Teil 6.
- Untermodelle ohne Entity: erledigt in Teil 6.
- Die Karte ist jetzt in allen Selbsttests dunkler. Der Gesamtlauf steht aus: Prueffotos und Schwellen koennen dort auffallen. Handbuchbilder der Karte zeigen noch die alte Helligkeit, `h_ansicht_einst` noch 2,0x: mit dem Bilderlauf der Endrunde erneuern.

## Teil 6: Restfehler Vorschau gegen Spiel

Alle Vergleichsbilder jetzt ohne Skalierung: Kartenansicht genau 1440 hoch (`BHED_GV_ANSICHT=1920x1440`, das Fenster darf groesser als der Schirm werden), davon die Mitte 1920 breit, Wiedergabe mit Tempo 0, also keine Bedienhilfen. Die Kamera von `viewpos` war abgeschnitten: Nick 0 statt 1 ("-359" ist abgeschnitten), Hoehe + 0,5. Vorher kamen 1,4x hochskalierte Bilder mit Lanczos und 20 Bildpunkte Versatz in den Vergleich: Ein grosser Teil der Abweichungen bei dunklen Werten war das.

### 1 und 4: Mustafar, rotes Feld und Untermodelle
Zwei Ursachen, beide aus der Engine:
- Die grauen Formen waren die Tueren `*91` und `*88` (`textures/plasma_Mustafar/door_1`, func_door): Auf der Testkarte haben sie keine Entity. Nur CG_Mover bringt ein Untermodell ins Bild: `kartenpruefung::brushModellImBild`, dazu trigger_* (SVF_NOCLIENT), rail_track, rail_lane, rail_mover, misc_weather_zone, fx_wind_zone.
- Dahinter stand der Rest des Raums (`basic_white`, Gebiet 9): Die Vorschau kannte keine Gebiete. Neu `bhed/areaportal.h`: CM_AdjustAreaPortalState, CM_FloodAreaConnections, CM_AreasConnected, SV_LinkEntity (areanum, areanum2), R_RecursiveWorldNode mit Gebietsmaske, SV_AddEntitiesToSnapshot. Schalten: Use_BinaryMover (teammaster, solange nicht in MOVER_POS1), func_wall und func_usable (nicht START_OFF), func_breakable, misc_model_breakable; CONTENTS_OPAQUE wie SV_AdjustAreaPortalState (nicht in Jedi Outcast). Die verdeckten Flaechen werden im Indexpuffer der Karte leer gesetzt, nur wenn sich die Maske aendert.
- Das rote Feld selbst ist der Himmel durch das Loch. Der Himmel wird nie verdeckt (RB_StageIteratorSky zeichnet ein Rechteck der Box), die Szene des Himmelsportals hat keine Maske (CG_DrawSkyBoxPortal; auf duel_mustafar steht dort der ganze Himmel).
Pixel 1405,723: Spiel 92,2/22,1/13,8, vorher 11,9/8,0/6,2, jetzt 92,1/22,0/13,8. Saeulenkante 1340,700: Spiel 27,1/24,2/20,6, vorher 7,6/3,9/2,6, jetzt 26,8/24,0/20,4.
Weitere Karten: alle mit Portalen. duel_mustafar hat 13 Gebiete und 15 Portal-Entities, vom Start aus sind 11355 von 12531 Flaechen hinter geschlossenen Tueren. Die echte Karte `duel_mustafar` nachgemessen: Boden 1,00/1,00/1,00, Himmel 1,00/0,99/0,99. Hier fehlen im Bild nur Glut (Wetter) und Figur, die im Spielerstart der Vorschau anderswo steht.
Das Auge ist die Kamera der Ansicht, nicht der Spieler: Sonst blieben die Raeume beim freien Flug dunkel. Abweichung mit Absicht.

### 2: Figur und dunkle Werte
- Figurentexturen waren auf 256 und Kartentexturen auf 512 verkleinert (Editoransicht von frueher): `kBildGrenze` 4096, im Spiel r_picmip 0. Der Stoff ist jetzt so scharf wie im Spiel. Spitze 830 -> 1250 MB auf den Testkarten.
- r_gammaShaders: Die Tabelle steht in der DLL (R_SetColorMappings, 0x180053FF0, disassembliert): `(int)(pow(i/63, 1/r_gamma)*255 + 0.5)` mal 2^overbright, 64 Stufen, GL_LINEAR, CLAMP_TO_EDGE; das Fragmentprogramm ist "TEX r0, ..., RECT; TEX result.color, r0, texture[1], 3D" ohne Halbtexel-Ausgleich. Teil 5 hatte abgerundet.
- **Gefunden: tcGen environment der Karte war die Formel von Quake 3.** JKA rechnet `st = normal.xy*d - 0.5*viewer.xy` (RB_CalcEnvironmentTexCoords). Figuren hatten es richtig, die Karte nicht. Daher die zu hellen Boeden auf Todesstern, Byss und Kamino.
- **Gefunden: Der Weichzeichner des Gluehens rundete die Abstaende**, das Spiel nimmt 0,1 + 0,8 je Durchgang mit GL_LINEAR. Daher die Streifen im Schein.
- r_dynamicGlowBloom ist 0,0 ("the default", im Spiel abgefragt): Der Bloom-Shader der DLL laeuft nicht.

Messung Vorschau gegen Spiel, je Mittel R/G/B, Faktor, Boden links + rechts + Himmel:
| Karte | Boden links | Boden rechts | oben | Figur (Faktor R) |
|---|---|---|---|---|
| Turm | 1,01 1,01 1,01 | 1,00 1,00 1,00 | 1,00 1,01 - | 1,04 (vorher 0,79) |
| Mustafar | 1,00 1,00 1,00 | 1,00 1,00 1,00 | 1,00 1,00 1,00 | 1,05 (vorher 1,15) |
| Akademie | 1,00 1,00 1,00 | 1,00 1,00 1,00 | 1,00 1,00 1,00 | 1,02 |
| Todesstern | 1,00 1,00 1,00 (vorher 2,04) | 0,99 0,99 0,99 (vorher 1,41) | 1,00 1,00 1,00 | 1,07 |
| Byss | 1,02 1,02 1,02 (vorher 1,16) | 1,02 1,02 1,02 (vorher 1,29) | 1,01 1,01 1,01 | 1,08 |
| Kamino | 1,05 1,05 1,06 | 1,05 1,05 1,06 | 1,06 1,04 1,06 | 1,09 |
| Hoth | 1,06 1,06 1,06 | 1,06 1,05 1,06 | 1,01 1,01 1,01 | 1,26 |
Dunkle Werte Mustafar, Perzentile 5/25/50/75/95/99 Spiel gegen Vorschau, alle Kanaele hoechstens 1 Byte auseinander (R 5/6, 10/11, 13/13, 21/20, 25/25, 28/27). Die Figur bleibt 2 bis 9 Prozent heller und steht 4 Bildpunkte tiefer; die Felder enthalten darum etwas Boden, auf Hoth liegt der Ausschnitt nicht deckungsgleich (Kamera: dy 22). Offen: Kamino und Hoth 5 Prozent auf den Boeden, nicht geklaert.

### 3: Schaden an Figuren
`bhed/schmerz.h`, hurtTodeBerechnen, SzenenHilfe und AblaufWelt:
- G_ApplyKnockback vor dem Gottmodus, knockback = damage (hoechstens 200, EXTRA doppelt, CRUSH und NO_KNOCKBACK 0, 2x hoechstens 120, g_knockback 1000, Masse 200, z = dir*... + 20, pm_time 50..200). **Die Bolzen der Geschuetze schieben nicht** (DAMAGE_NO_KNOCKBACK in turret_fire und pas_fire), Teil 5 hat das falsch vermutet. Es stossen G_RadiusDamage und was noch dir hat. `planeStosslauf` (Pmove mit leerem ucmd, PMF_TIME_KNOCKBACK) geht als Weg `stossWege` wie schubWege in Ablauf und Szene; der naechste Plan beginnt dort. Auf einem laufenden Weg wirkt er nicht (der Plan steht vorab).
- **Movie Duels: "Universal directional pain animation" in G_MissileImpacted.** Auch im Gottmodus, 75 %, 2 s Pause, Animation nach Trefferort, Oberkoerper nur 400 ms. Das ist das Zucken auf den 20 Spielbildern. OpenJK kennt das nicht.
- NPC_Pain mit NPC_ChoosePainAnimation (painDebounceTime = PM_AnimLength, ohne Feind immer, Desann halb, in MD auch Vader und CLASS_SITHLORD), PlayerPain (Animation nur bei MOD_CRUSH; Schwert und Faust gibt es hier nicht; sonst nur EV_PAIN). EV_PAIN als Klang nach CG_PainEvent.
Selbsttest `teil6`: Mission mit Wachgeschuetz: Movie Duels 7 Zuckungen, 0 Stoesse; OpenJK 15 Schmerzanimationen. Explosion: Stoss -384/0/260, 266 Einheiten Weg.
Funkenfaecher erneut gegen die 20 Spielbilder (`g6/erg/paar_fan.png`): gleiche Mechanik, aber die Streuung bleibt kleiner. Im Spiel zeigt der Faecher in etwa 7 von 20 Bildern nach oben oder links, in der Vorschau fast immer nach rechts. Grund nicht abschliessend geklaert: Die Bewegung ist jetzt da, sie liefert aber nur in den Zeitfenstern der Animationen andere Dreiecke, und die Zufallsfolge ist nicht die des Spiels. Das bleibt offen.
Nicht nachgebildet: G_CheckKnockdown und G_Knockdown (auch der Knockdown geduckter Figuren in G_MissileImpacted), BSET_PAIN, FL_NO_KNOCKBACK (SET_NO_KNOCKBACK), die Stoesse auf einem laufenden Weg, NPC_GetPainChance mit Feind.

### 5: r_DynamicGlow
Vorgabe des Schalters beim Wechsel des Regelwerks: Movie Duels an, OpenJK und Jedi Outcast aus. Handbuch und Hinweis in 6 Sprachen.

### 6: Spieler mit Gewehr
G_SetWeapon fuehrt beim Spieler ueber CG_ChangeWeapon: nach WEAPON_RAISING ist er in WEAPON_IDLE wie ein NPC (nur der NPC bekommt WEAPON_READY). `npcWaffenTorso` jetzt auch fuer ihn, mit den Laufregeln des Spielers. Foto mit WP_DISRUPTOR (`weapon 4` im Spiel): Haltung deckungsgleich (`g6/erg/paar_holster_waffen.png`).

### 7: Holster
- Basispose von "ModView internal default": FakeGLAFile in tr_model.cpp, Einheit. Der Griff liegt laengs Z, der Bolzen `*blade1` bei +z.
- **Die Drehung der Quelle stimmt (diag 1 -1 -1), es fehlte der Versatz:** G2_Generate_Matrix schreibt `offset` in die vierte Spalte (0 -1 -10 an der Huefte). Der Kommentar in der Quelle von MD ("not used by the renderer") stimmt nicht. Alle Plaetze nach der Quelle, die gemessene Drehung aus Teil 5 ist weg. Foto ohne Waffe in der Hand (`g6/erg/paar_holster_frei.png`): deckungsgleich fuer HOLSTER_LHIPF. Nur LHIPF per Foto, die anderen nach Quelle.
- Die .sab von Movie Duels schreiben "back", "rhip": Der Leser kannte nur HOLSTER_*; "lhipl" ist in der Engine unbekannt (Vorgabe). HOLSTER_BACK war so nie aktiv.
- `*holsterorigin`: Kein Griff hat diesen Bolzen (alle 264 .glm durchsucht).
- STUN_BATON (32) und NOWEAPON (64) am Spielerstart.
- cg_holster.cpp: Plaetze, holster.cfg (eigene oder kyle), Hls_Render, Knochen als Bolzen, WP_SaberHolsterAvoidGuns. **Foto nicht deckungsgleich:** Im Spiel haengen zwei Waffen sichtbar an den Seiten (`paar_holster_waffen.png`, `paar_holster_hinten.png`), in der Vorschau sieht man sie kaum; der Versuch mit vierfachem Versatz zeigte eine Waffe, der Ort stimmt also ungefaehr, die zweite (WP_BATTLEDROID) war nicht zu sehen. Nicht fertig, bleibt offen. cg_holsterHide und die Gegenstandsliste von NPC mit mehreren Waffen fehlen.

### 8: Lunge
in_camera kommt in den Plan: `Ablauf::kameraZeiten`, Stand zu Beginn des Plans, in Ablauf und Szene gleich. Probe und Foto im Spiel nicht nachgeholt: Dazu braucht das Spiel ein kompiliertes Skript (SET_CROUCHED mit Laufziel), die Spielprobe ist offen.

### 9: Leiche
Reibung aus `tod.reibung` in allen Zweigen. pitch_roll_for_slope (`leichenNeigung`, Nick und Roll an der Figur). Kasten aus DeadThink in den Zielen der Geschuetze (eyePoint + 4, mindestens -8). Nicht per Foto (Testkarte eben), nur gegen die Formel; der Kasten gilt nur fuer die Spuren der Geschuetze, nicht fuer die Spur der Leiche selbst; `mins[2]` aus pitch_roll_for_slope fehlt.

### 10: q3map_sun
ParseShader: sun, q3map_sun, q3map_sunExt: `tr.sunDirection` aus Grad und Hoehe, der zuletzt gelesene Shader in der Reihenfolge der Flaechen gilt.

### Tests
- Ctest `schmerz` (neu), `kartenpruefung` ergaenzt. Selbsttest `teil6` (neu).
- Rot: Bild `paar_mustafar_nach.png` aus Teil 5. Selbsttest teil6 erster Lauf 4 OK, 2 FEHLER (0 Zuckungen: die Dauer kam aus Actor::animDauerMs, der nur benutzte Animationen kennt), danach 6 OK, 0 FEHLER. Ctest schmerz erster Lauf rot in zwei eigenen Testzeilen (Eingabewert, Doppelschwert ohne Platz), danach 0 Fehler.
- Gelockert: nichts. `gpushader` prueft weiter die Stufe aus der Tabelle.

### Nur per Foto gegen das Spiel
Alle Karten oben, Griff frei, Waffe in der Hand, Faecher. Movie Duels. Original-JKA: Steam war angemeldet, aber kein neuer Start; das Foto `g5/fotos/o_byss_0.png` aus Teil 5 (1280 x 960) gegen die Vorschau unter OpenJK auf 1280 x 960 verkleinert (`g6/erg/paar_original_byss.png`): Boden 0,64 bis 0,68, oben 0,81, Figur 0,87. Die Vorschau ist also unter OpenJK dunkler als das Original; Ursache nicht geklaert (das Original meldet GAMMA: hardware, r_overBrightBits 0, amb und dir stimmen). OpenJK-Regeln sonst nur gegen die Quelle.

### Offen aus Teil 6
- Waffen im Holster nicht deckungsgleich (siehe 7), zweite Waffe.
- Funkenfaecher: Streuung (siehe 3).
- Kamino und Hoth: Boeden 5 Prozent heller. Figur 2 bis 9 Prozent heller (Hoth nicht deckungsgleich).
- Lunge: Spielprobe und Foto. Knockdown.
- Original-JKA: Vorschau unter OpenJK 0,64 bis 0,81 des Originals, Ursache offen; nur ein Foto.
- duel_mustafar echt: Glut in der Luft (Wetter/Effekt) im Vergleichsbild nicht geprueft.
- Eine Tuer ohne Entity zeigt in der Vorschau den Himmel, im Spiel ein Rechteck der Himmelsbox um die sichtbaren Himmelsflaechen; beides nur auf Testkarten.

## Teil 7: Original-JKA und letzte Abweichungen der Vorschau

Aufnahmen: ungeskaliert 1920 x 1440, Vorschau mit `BHED_GV_ANSICHT`, Skripte in `SP/g7`. Das Original-Foto war bisher falsch: Die Steam-exe laeuft (`__COMPAT_LAYER HighDpiAware`) echt 1920 x 1440, `fenster.ps1` ist nicht DPI-bewusst und bekam 1284 x 987. Neu `g7/foto7.py`: nur der Client-Bereich, 1920 x 1440 auch im Original. Kamera: `viewpos` schneidet zur Null ab, also 76.25 statt 76.5, auf Hoth -161.4 statt -160.5; fovY 64.3727 statt 64.4. Rest-Versatz 0 bis 1 Bildpunkt am Boden, Hoth 4.

### 1: Original-JKA (Steam angemeldet, GameData danach gleich, je Lauf geprueft)
Im Spiel abgefragt: r_gamma 1, r_intensity 1, r_overBrightBits 0, r_DynamicGlow 0, r_picmip Vorgabe 1 (fuer das Foto auf 0 gesetzt), r_ambientScale 0,5, r_directedScale 1, cg_fov 80. `r_mapOverBrightBits` gibt es in der Original-exe nicht (keine Antwort). "GAMMA: hardware w/ 0 overbright bits", "Dynamic Glow: disabled". r_debuglight: t3_byss amb 115 dir 384, Richtung -0,05 0,01 1,00; academy1 amb 94 dir 96, Richtung 0,02 -0,64 0,77.
Vorschau unter OpenJK mit dem Original-`base` gegen Original, Faktor R/G/B:
| Karte | Boden links | Boden rechts | oben | Figur |
|---|---|---|---|---|
| t3_byss | 1,00 1,03 1,01 | 0,99 1,04 1,00 | 0,99 1,00 0,95 | 0,99 1,00 0,98 |
| academy1 | 1,00 1,00 1,00 | 1,00 1,00 1,00 | 1,00 1,00 1,00 | 1,00 1,00 1,00 |
Die Faktoren 0,64 bis 0,87 aus Teil 6 waren Fehler des Fotos. Kein Helligkeitsunterschied, nichts angeglichen. t3_byss G 1,03: vermutlich Texturkompression im Original (R gleich, nur Gruen), nicht geprueft. Das Spiel wechselt beim Stehen zeitweise in eine Pose mit gedrehtem Kopf, das Vergleichsbild zeigt die gerade Haltung (`g7/fotos/o_byss_3.png`).

### 2: Fenster auf t3_byss
Die Fenster sind die Tueren `*70` und `*74` (`byss_gatedoor_onoff`), die Testkarte hat keine Entities dafuer. Probe mit `r_clear 1` im Original: Beide Fenster werden rot, also zeichnet das Spiel dort nichts, die violette und die blaue Flaeche sind alter Inhalt des Bildpuffers. Movie Duels: schwarz. Nur auf der Testkarte.
Vorschau rechts zeigte Sterne: Der Himmel war von der Gebietsmaske ausgenommen, das war falsch (R_RecursiveWorldNode gibt ihn hinter geschlossenen Portalen gar nicht weiter). Jetzt beide Fenster schwarz wie in Movie Duels. Damit das rote Feld auf Mustafar bleibt: Die Portalszene steht wie im Spiel vor der Hauptszene im Bild (`CopyResource` statt Loeschen, nur mit Himmelsportal).

### 3: Haarlinie im Himmel
Abtaster anisotrop und umlaufend, an der Kante zweier Seiten springen die Ableitungen: Das ganze Bild kam in eine Pixelspalte. Jetzt `SampleLevel` Stufe 0 und halber Texel am Rand wie GL_CLAMP_TO_EDGE. Abweichung der Spalte gegen die Nachbarn: Mustafar rot 19,8 -> gruen 0,4 (Spiel 0,33), Turm rot 25,8 -> gruen 0,7.

### 4: Glut
`Weather/duel_mustafar.cfg` (IT_LoadWeatherParms, execq, nur Movie Duels): `r_weather 3`. Probe im Spiel: `g_AllowWeather 0` keine Glut, 1 Glut. In der Quelle wuerde der Befehl mit der Cvar 0 alles loeschen, die exe tut das nicht (Cvar bleibt 0, Glut ist da); das Verhalten ist also gemessen, die Saetze sind aus RE_WorldEffectCommand. Neu: Saetze 0, 1, 2, 3, 4, 7, 8, 9, Zeilen nacheinander, 0 raeumt auch das Wetter der Entities. 5 und 6 haben keinen Zweig. Die Teilchenwolke selbst nach CParticleCloud: Masse je Teilchen, v = Kraft * 0,7 / 0,3 / Masse statt der alten Werte, Ebene +-500 und bis 625 unter der Kamera, Alpha mColor[3], kein erfundenes Taumeln, Sand mit Alpha; ContentsOutside mit den Brushes statt Strahl nach oben.
Zaehlung Glut, ein Bild: Himmel Spiel 9, Vorschau 9; Boden 158/158 (Schwelle 15), 74/59 (Schwelle 25). Eine Wolke reicht also, obwohl IT_LoadWeatherParms zweimal laeuft. Nur ein Bild, Positionen zufaellig. Betrifft auch duel_hoth (1), duel_kamino (2), duel_deathstar (9).

### 5: Waffen im Holster
**Gefunden:** G2API_GetBoltMatrix normiert die drei Zeilen. Die Knochen tragen die 0,64 der .gla: Waffe zu klein, Versatz zu kurz, sie steckte in der Figur. Foto `paar_holster.png`: beide Waffen sichtbar, deckungsgleich mit dem Spiel. Griffe im Holster haengen an Bolzenflaechen und waren nicht betroffen.

### 6: Restliche Faktoren
**Gefunden, zwei Ursachen:**
- Nebel: tr.fogImage ist ein Bild 256 x 32 mit GL_LINEAR, nahe am Auge ist die Wurzel steil, die Rechnung ohne Bild war dort zu stark. Kamino und Hoth haben globalen Nebel. Jetzt je Texel R_FogFactor, abgeschnitten und linear gefiltert.
- Figur: RB_SurfaceGhoul legt die gewichtete Normale ungekuerzt in tess.normal, die Vorschau normierte sie. Bei viel gerichtetem Licht 2 bis 9 Prozent zu hell.
Messung Movie Duels, Faktor Vorschau/Spiel, R G B (vorher aus Teil 6):
| Karte | Boden links | Boden rechts | oben | Figur |
|---|---|---|---|---|
| Turm | 1,00 1,01 1,01 | 1,00 1,00 1,00 | 1,00 1,00 - | 1,00 1,00 1,00 (1,04) |
| Mustafar | 1,00 1,00 1,00 | 1,00 1,00 1,00 | 1,00 1,00 1,00 | 1,04 1,04 1,04 (1,05) |
| Akademie | 1,00 1,00 1,00 | 1,00 1,00 1,00 | 1,00 1,00 1,00 | 1,01 1,01 1,02 (1,02) |
| Todesstern | 1,00 1,00 1,00 | 0,99 0,99 0,99 | 1,00 1,00 1,00 | 1,03 1,03 1,03 (1,07) |
| Byss | 1,02 1,02 1,02 | 1,02 1,02 1,02 | 1,01 1,01 1,01 | 1,03 1,04 1,04 (1,08) |
| Kamino | 1,03 1,03 1,02 (1,05) | 1,03 1,03 1,03 (1,05) | 1,05 1,02 1,04 | 1,01 1,00 1,00 (1,09) |
| Hoth | 1,00 1,00 1,00 (1,06) | 1,00 1,00 1,00 (1,06) | 1,00 1,00 1,00 | 1,03 1,03 1,02 (1,26) |
Der Rest ist unter einem Byte (Byss Boden 17,8 gegen 18,1, Kamino 20,8 gegen 21,4). Im Original ist Byss 1,00, also eher die 64 Stufen von r_gammaShaders; Ursache nicht weiter gesucht. Figur auf Mustafar 1,04 ungeklaert.

### 7: Funkenfaecher
Gleicher Versuch, 20 Bilder `gv_turm`, `paar_fan.png`: Spiel etwa 7 von 20 nach oben oder links, Vorschau 0 von 20. Nicht behoben. Messwerte: In der Vorschau trafen 128 Protokollzeilen genau zwei Punkte und Normalen (misc_turret Kopf 353,6 6859,3 54,1, Normale -0,96 -0,29 0,03; misc_sentry_turret Rumpf 353,0 6854,9 22,5, Normale -0,87 0,45 -0,20). Der Faecher liegt laut .efx laengs der Normalen, die Mechanik stimmt also, im Spiel treffen die Schuesse verschiedene Dreiecke. Turret_aim, Muendungen, random und die Normale aus G2_RadiusTracePolys stichprobenartig gegen die Quelle gelesen, keine Abweichung gefunden, kein vollstaendiger Zeilenvergleich. Es fehlt ein Vergleich der Trefferorte im Spiel, dafuer braeuchte man ein Protokoll im Spiel. Bilder am Rande: Beim nahen Bolzen wird die Figur in der Vorschau stark gruen, im Spiel schwaecher (nicht gemessen).

### 8: Lunge von Vader und Desann
Der Weg ohne Spielordner geht: `bhed compile` nach `fs_homepath/MD/scripts/`, `npc spawn desann des`, `runscript des lunge`; mit g_ICARUSDebug 4 laeuft das Skript nachweislich (SET_FORWARDMOVE, SET_CROUCHED). Der Nachweis der Lunge nicht: Sie hat keine Animation (PM_SaberLungeAttackMove(qtrue): nur 150 vor und 50 hoch), Desann bewegt sich auf den Bildern (`g7/fotos/lunge_*.png`, Zeitlupe 0,1) kaum und ist zu dunkel. Spiel-Beleg bleibt offen, nur gegen die Quelle gelesen.

### 9: Leiche
- **Gefunden:** Die Neigung kam nur mit Totenlauf. Wer auf dem Boden stirbt, hat keinen Lauf und blieb flach (rot: `leicheNick` 0 auf dem Hang, Foto), jetzt immer.
- `mins[2]` aus pitch_roll_for_slope: `leichenSenkung` (12 * |Nick| / 180). Ctest `schmerz`.
- Foto `paar_hang.png` (Testkarte `g7_hang`, duel_hoth-Gelaende, Hang 33 Grad): Die Leiche liegt am Hang. Stelle und Todesanimation sind im Spiel andere (`npc spawn`, zufaellig), also kein Deckungsvergleich. Die fallengelassene Waffe steht in der Vorschau zu hoch ueber dem Hang, nicht untersucht.
- Reibung und Kasten aus DeadThink weiter nur gegen die Quelle.

### 10: Speicher und Ladezeit
Groesste Karte `md_toaj_jedi` (74 MB), ruhig, je Lauf: Spitze / Grafikspeicher (GPU Process Memory) / Lauf gesamt / Bildzeit
| Stufe | Spitze | Grafik | gesamt | Bild |
|---|---|---|---|---|
| vorher Grenze 512 (Probe, Figuren hier auch 512) | 1116 MB | 401 MB | 21 s | 4,9 bis 6,3 ms |
| r_picmip 0 (Vorgabe) | 2541 MB | 807 MB | 22 s | 4,7 bis 6,3 ms |
| r_picmip 1 | 1186 MB | 405 MB | - | 4,8 bis 6,2 ms |
| r_picmip 2 | 863 MB | 304 MB | - | 5,4 bis 6,3 ms |
Neu: Regler "Bildstufe (r_picmip)" im Klappfenster der Ansicht, Upload32: halbiert je Stufe; Vorgabe 0 wie OpenJK und Movie Duels (tr_init.cpp), die Original-exe hat Vorgabe 1 (abgefragt). `nopicmip` der Shader wird nicht beachtet. Wenig Grafikspeicher: nicht nachstellbar auf diesem Rechner (RTX 4070), Windows lagert aus. Aus dem Code: Fehlschlag beim Anlegen einer Textur laesst die Flaeche aus, Geraeteverlust ist behandelt. Nicht im Lauf geprueft.

### Tests
Ctest `gpushader` (Himmel, Nebel, Normale) und `schmerz` ergaenzt, Selbsttest `teil6` 8/0 mit Wetter und Stufe, `figurlicht` 8/0. Rot nur per Foto und Messwert wie oben, die neuen Zeilen in `teil6` und `gpushader` wurden nicht rot gesehen. Ctest einmal: 194 von 195, `gpushader` rot in einer eigenen alten Textzeile (Normale der Laenge null, der geaenderte Ausdruck), angepasst, nur dieser Test wiederholt gruen. Gelockert: In `teil6` fiel "Himmel wird nie verdeckt" weg, das beschrieb den Fehler.

### Nur per Foto gegen das Spiel
Alles in 1, 2, 3, 4, 5, 6, 7, 9. Originalfotos 1920 x 1440, also nicht kleiner.

### Offen aus Teil 7
- Funkenfaecher: siehe 7.
- Lunge: Beleg im Spiel, siehe 8.
- **Neu:** Glas `textures/common/glass_security_chain` auf t3_byss (oben links): Die Vorschau zeigt die Streifen von `etest4` scharf, Original und Movie Duels glatt, Mittel 67,7 gegen 56,9 (Grauwert). Formel RB_CalcEnvironmentTexCoords gleich, Ursache nicht gefunden.
- Kamino 3 Prozent, Figur Mustafar 4 Prozent, unter einem Byte bzw. ungeklaert.
- Fallengelassene Waffe am Hang; Figur stark gruen am nahen Bolzen.
- Himmelsbox: Rechteck um die sichtbaren Himmelsflaechen (RB_ClipSkyPolygons) nicht nachgebildet. Nur bei Loechern in der Karte sichtbar, also auf den Testkarten.
- r_mapOverBrightBits: nicht abfragbar im Original.

## Teil 8: Himmelsbox, Gluehen, Bolzen, Glas

Aufnahmen jetzt wirklich 1920 x 1440: `BHED_GV_ANSICHT` kam auf 2032 bis 2198 Breite (der Schritt im Verhaeltnis schwang ueber), die Mitte hatte zwar die richtige Hoehe, das Gluehen wurde aber breiter gezogen. Jede Runde um die Luecke, Start `BHED_GV_FENSTER=2930x1530`: `Ansicht 623 144 2543 1584`. Skripte und Fotos in `SP/g8`.

### 1: Himmel (Rueckschritt aus Teil 7)
Die Testkarten haben kein `misc_skyportal`, die Copy-Regel aus Teil 7 griff dort nie. **Gefunden:** Das Spiel zeichnet die Himmelsflaechen gar nicht. RB_StageIteratorSky wirft sie mit RB_ClipSkyPolygons auf die Box und zeichnet je Seite das aufgerundete Rechteck (DrawSkyBox, Achtelraster, untere Seite nie) bei qglDepthRange(1, 1). Das rote Feld ist dieses Rechteck durch das Loch, die Byss-Fenster liegen ausserhalb. Neu `bhed/himmelbox.h`: R_MarkLeaves, R_RecursiveWorldNode (5 Ebenen, distanceCull), R_CullSurface, RB_CHECKOVERFLOW, ClipSkyPolygon, AddSkyPolygon, DrawSkyBox. Keine Tiefe, keine Keule, Stufen weiter per Bildpunkt. Hauptszene mit Portal zeichnet keinen Himmel; die Portalszene hat die Gebietsmaske ihres Orts (CM_SnapPVS, Teil 6 hatte keine); RB_BeginDrawingView loescht je Szene mit r_DynamicGlow und globalem Nebel. kSkyPortal ist weg. RB_DrawSun: r_drawSun 0, nichts zu tun. r_clear 0: Wo nichts gezeichnet wird, steht in Movie Duels schwarz (Clear im Gluehdurchgang), im Original alter Bildpuffer (Vorschau schwarz, nicht nachgebildet).
| Stelle | Spiel | Teil 7 | jetzt |
|---|---|---|---|
| Mustafar Feld 1405,723 (21 x 21) | 92,2 22,1 13,8 | 0 0 0 | 92,1 22,0 13,8 |
| Mustafar Saeule 1340,700 | 27,1 24,2 20,6 | - | 26,8 24,0 20,4 |
| Byss Fenster links 810,300 | 0 0 0 | 0 0 0 | 0 0 0 |
| Byss Fenster rechts 1670,380 | 0 0 0 | 0 0 0 | 0 0 0 |
Bildzeit Mustafar 4,3 ms (Teil 6: 4,7 bis 6,3 auf der groessten Karte).

### 2: Figur auf echten Karten
Nur die Aufnahme: SelectSpawnPoint ohne spawntarget nimmt einen zufaelligen info_player_deathmatch (31 auf duel_mustafar, das Spiel stand bei 688 2176, die Vorschau am info_player_start). Mit genau einem Start in beiden (`g8/g8.pk3`: g8_must, g8_stern, g8_lb) steht die Figur deckungsgleich da: Figur 1,08 1,02 1,03 / 1,03 1,03 1,03 / 1,03 1,02 1,03, Boden 1,00. Dabei gefunden: SV_BuildClientSnapshot nimmt das Auge des Spielers, in einer Kamerafahrt die Kamera (CG_CAMERA_POS). Die Vorschau fragte nur die Kamera, der Spieler fiel weg, sobald sie in einem anderen Gebiet stand. Jetzt zaehlt das Auge; beim freien Flug zeigt sich zusaetzlich, was die Kamera sieht (Abweichung mit Absicht, im Spiel gibt es den Flug nicht). Unter OpenJK kommt die Maske der Karte aus dem Schnappschuss, unter Movie Duels aus vieworg (cg_view.cpp): die Vorschau nimmt immer die Kamera.

### 3: Gluehen
Im Spiel abgefragt: r_DynamicGlow 1, Passes 5, Delta 0,8f, Intensity 1,13f, Soft 1, Width 320, Height 240, Bloom 0,0, alles Vorgabe, wie im Code. **Gefunden:** Der erste Durchgang verkleinert nicht per Kastenmittel, er liest den vollen Puffer je Zielpunkt an vier Stellen um 0,1 Bildpunkte (RB_BlurGlowTexture). Eigenes Verkleinern und `glowShrinkPixelShaderHlsl` sind weg. Profil Zeile 560 auf g8_lb, R (G, B gleich gut), Spiel / Teil 7 / jetzt: x 920: 14/24/13, 930: 35/41/35, 950: 70/68/70, 970: 102/93/102, 1000: 100/94/100, 1020: 69/70/69, 1040: 30/41/30, 1050: 12/24/12, 1060: 6/10/6. Der Eindruck "breiter" stimmte nicht: Das Spiel ist nah am Balken heller und aussen schmaler.

### 4: Figur am Bolzen
Lichter gleich (`turret/shot`: Light 80..100, 0 0,502 0). **Gefunden:** Im Einzelspieler liest CG_CalcEntityLerpPositions den lebenden Zustand, cg.time laeuft dem Schnappschuss nach: Ist das Serverbild mit dem Einschlag gelaufen, steht der Bolzen am Einschlag (G_SetOrigin, TR_STATIONARY). Die Vorschau liess ihn bis zum Ende des Bilds weiterfliegen, durch die Figur, 13 Einheiten hinter ihr: von hinten beleuchtet, 250 Gruen. Messung Oberkoerper 880,900-1040,1100, je 70 Bilder im Abstand 15 ms Spielzeit: Spiel G-R hoechstens 0, G hoechstens 88. Vorschau vorher bei 4036 ms 57 59 16 (G-R +2, Faecher-Paar Neon), jetzt G-R hoechstens -6, G hoechstens 68. Foto `g8/erg/paar_bolzen.png`.

### 5: Waffe am Hang
Keine Abweichung. Im Spiel aus zwei Kameras eingemessen (`g8/hang_tri.py`, Ort auf 1 bis 2 Einheiten): Mitte der Waffe 10,7 ueber dem Boden, Vorschau 6,4, je an anderer Stelle (Zufall). Kasten -15..15 aus LaunchItem, G_SetOrigin(endpos) wie in der Quelle. Nichts geaendert.

### 6: Glas
**Gefunden, allgemein:** Der Grundstufe fehlte `tcGen` (`t.texGen` wurde beim Laden nie gesetzt, nur fuer Zusatzstufen). Jede Stufe 0 mit tcGen environment oder vector lag mit den st der Flaeche im Bild. Korrelation Glas mit etest4 unter st: Vorschau 0,49, jetzt -0,08 (Spiel -0,08). Feld 300,20-500,90: Spiel 53,0 55,4 58,9, Teil 7 Grauwert 67,7 gegen 56,9, jetzt 53,1 55,6 59,4.

### 7: Leiche
`leichenKastenOben` und `leichenReibung` aus dem Code gezogen, Ctest `schmerz`: DeadThink, ps.friction, PM_Friction von Hand. Die Rueckname des Kastens bei allsolid fehlt weiter.

### Alle Paare nach den Eingriffen (Faktor R G B)
| Karte | Boden links | Boden rechts | oben | Figur |
|---|---|---|---|---|
| Turm | 1,00 1,01 1,01 | 1,00 1,00 1,00 | 1,00 1,00 - | 1,00 1,00 1,00 |
| Kamino | 1,03 1,03 1,02 | 1,03 1,03 1,03 | 1,05 1,02 1,04 | 1,01 1,00 1,00 |
| Todesstern | 1,00 1,00 1,00 | 0,99 0,99 0,99 | 1,00 1,00 1,00 | 1,03 1,03 1,03 |
| Mustafar | 1,00 1,00 1,00 | 1,00 1,00 1,00 | 1,00 1,00 1,00 | 1,04 1,04 1,04 |
| Akademie | 1,00 1,00 1,00 | 1,00 1,00 1,00 | 1,00 1,00 1,00 | 1,01 1,01 1,02 |
| Hoth | 1,00 1,00 1,00 | 1,00 1,00 1,00 | 1,00 1,00 1,00 | 1,03 1,03 1,02 |
| Byss | 1,02 1,02 1,02 | 1,02 1,02 1,02 | 1,00 1,00 1,00 (vorher 1,01) | 1,03 1,04 1,04 |
| g8_must | 1,00 1,00 1,00 | 1,00 1,00 1,00 | 0,99 0,99 0,98 | 1,08 1,02 1,03 |
| g8_lb | 1,00 1,00 1,00 | 1,00 1,00 1,00 | 1,00 0,97 0,90 | 1,03 1,02 1,03 |
| g8_stern | 1,00 1,00 1,00 | 0,99 0,99 0,99 | 2,52 2,52 2,48 (siehe Offen) | 1,03 1,03 1,03 |

### Tests
Ctest `himmelbox` (neu, 29 Zeilen), `schmerz` (8 neue). Selbsttest `teil6` 11/0 mit drei neuen Zeilen. Rot gesehen mit voruebergehend abgeschalteten Regeln: `himmelbox` 11 FEHL, `schmerz` 8 FEHL, `teil6` 3 FEHLER (Mustafar-Feld, Spieler im anderen Gebiet, Byss-Fenster). Die Spielerzeile war zuerst aus falschem Grund rot (Szene ohne Spieler), danach mit Mission und abgeschaltetem Auge noch einmal rot gesehen. Gelockert: `gpushader`: "das Verkleinern mittelt", "Himmelsportal" entfallen mit dem Code; das Kastenmittel beschrieb den Fehler. Das Gluehen kleiner, heller Flaechen kann dadurch wie im Spiel flimmern (rc597 wollte das vermeiden).

### Nur per Foto gegen das Spiel
Alle Paare oben, Movie Duels, `g8/erg/`. Original-JKA nicht neu aufgenommen. OpenJK-Regeln nur gegen die Quelle.

### Offen aus Teil 8
- **Neu:** duel_deathstar, Falcon (`misc_model_static`, md3 ohne Skript): Unterseite 1400,100 Spiel 20,0 21,5 22,0, Vorschau 112,1 118,7 121,7. Linke Seite gleich (160). Nicht durch Teil 8, nur auf der echten Karte sichtbar. Nicht untersucht.
- Bolzen im Original nicht gemessen. Wolken: FillCloudBox zeichnet laut Quelle auch unter dem Horizont, der Shader verwirft dort (`clip(d.z)`), nicht geaendert.
- Himmel an Brush-Modellen (R_AddBrushModelSurfaces) und dlightMap im Sortierschluessel fehlen in himmelbox (kein Fall auf den Karten gesehen). Karten-Editor (`karte_gpu_win32.cpp`) zeichnet weiter eigene Himmelsflaechen.
- Glas auf Byss: Unterkante hat in der Vorschau kleine Stufen, im Spiel glatt (Foto, nicht gemessen).
- Bildpuffer im Original (alter Inhalt hinter Loechern) nicht nachgebildet.

## Teil 9: Spieler in Cutscenes und Rest der Zeitleiste (abgebrochen, WIP)

Abgebrochen am 02.11. auf Anweisung (shank musste gehen). Die WIP-Commits sind nicht zusammengefasst. Der letzte Stand ist **nicht gebaut**: Die letzte Testaenderung (Motion-Zeile, Zeit 150 statt 600 ms) lief nie. Kein ctest gesamt, keine Lints, kein Neubau. Skripte und Fotos in `SP/g9`.

### Gemessen
- **A Falcon, Ursache:** Jedes .md3 lief mit Eckenfarbe 255 mal Helligkeit, also ohne Licht. Neu: R_SetupEntityLighting und RB_CalcDiffuseColor im Vertexshader (`kModellLicht`, JeModell b7) fuer alle .md3, `lightmap` -1 wie lightmapsNone, misc_model_static mit lightingOrigin origin + zoffset + 1, Achsen samt Massstab. Keine Karten- oder Modellnamen im Code. duel_deathstar 1400,100: Spiel 20,0 21,5 22,0, vorher 112,0 118,6 121,5, jetzt 20,0 21,4 22,0. Feld oben Faktor 2,52 auf 1,00.
- **A Gegenprobe** (nicht zum Finden benutzt): duel_tradefed, mittlere Abweichung ueber das Bild 1,43 1,46 1,46. duel_jeditemple, Spiel / Vorschau: crate_01 7,4 8,3 9,5 / 6,3 7,2 8,3 und 8,8 9,4 11,4 / 9,2 10,1 12,6; statue_guard 37,0 26,6 14,7 / 35,5 25,5 13,9 und 38,8 27,3 12,5 / 39,6 28,2 13,7. Die Kamera dort ist nur auf ganze Einheiten und Grad bekannt und lag nicht deckungsgleich, die Felder sind darum grob.
- **A Stufen am Byss-Glas:** andere Ursache. Spiel: Kanten gemischt, `r_ext_multisample` 8 ist dort Vorgabe (im Spiel abgefragt). Vorschau hat kein MSAA. Nicht gebaut.
- **Neu gefunden:** $lightmap hinter zwei Bildstufen war in die Grundstufe gerechnet (duel_jeditemple: Boden und Saeulen grau statt braun). Neu als eigene letzte Filterstufe, 41 Stufen auf der Karte. **Nach dem Eingriff nicht per Foto geprueft.**
- **B Gluehen:** Vorschau flimmert nicht staerker. Mittlerer Schritt je Bild ueber 32 Bilder, Spiel / Vorschau: Leiste 0,152 / 0,143, Halo ueber der Leiste 0,219 / 0,130, kleines Fenster 2,106 / 0,307, Halo daran 0,950 / 0,137. Die Spielkamera ruckt bei setviewpos, die Spielwerte sind dadurch eher zu hoch. Kein Code geaendert.
- **1 Spieler:** Rot 11 FEHLER, gruen alle Zeilen mit und ohne Schalter, affect davor und danach, ohne Kamera, Export und Undo. Dabei echter Fehler: Der Ablauf kannte `kyle` unter Jedi Outcast nicht als Spieler, `affect` fiel weg; behoben. Selbsttests setzen den Schalter global an, nur `teil9` prueft beide Stellungen.
- **Teil 9 Selbsttest zuletzt 21 OK, 1 FEHLER** (Motion-Zeile, siehe unten). ctest `schmerz` 0 Fehler, `gpudraw` und `gpushader` gruen, je vorher rot.

### Noch nicht rot/gruen gesehen
- 2 `cg_motionBoneComp`: Selbsttest war rot, aber aus falschem Grund (bei 600 ms kein eigener Oberkoerper). Gruen fehlt, Wirkung von 0 bis 3 im Bild nicht belegt. Nur die Rechnung per ctest.
- 7 Foto: `g9/erg/paar_figur_steht.png`, `paar_figur_blick.png`, nur angesehen. Spieler passt dem Eindruck nach. **Der NPC haelt das Schwert anders als im Spiel, offen.** Klingenfarben weichen ab, daher keine Helligkeitswerte. Gehen mit Schwert nicht aufgenommen.

### Teil 11: Karten-Editor wie im Spiel, Punkt 0 (Mission-Vorschau)

Auftrag `auftrag_rc608_11.md`. Skripte und Bilder in `SP/g11` (`strahl.py`, `pvs.py`, `shtext.py`, `bloecke.py`, `md11.py`, `vor11.sh`, `alle11.sh`, `mess11.sh`, `paar11.py`, `tabelle11.py`, `jk2dp.py`), Paare in `SP/g11/erg/`. Spielbilder Movie Duels mit fs_homepath im SP (`g5/home_md`); Original-JKA nicht verglichen (Steam lief erst gegen Ende, dann nur das JK2-Foto).

### 0: duel_jeditemple, Fenster und ferne Waende (Mission-Vorschau, zwei Ursachen, beide allgemein)
- **PVS:** Die Vorschau zeichnete jede Weltflaeche, das Spiel nur die Blaetter, deren Cluster im PVS des Auges liegt (R_MarkLeaves, tr_world.cpp:678 MD). Durch das mittlere Fenster sah man so den Raum hinter dem Himmel (impdetention/metal, Cluster 1254..1285, nicht im PVS von Cluster 2313). `flaechenHinterPortalen` heisst jetzt `flaechenAusserSicht` und prueft PVS und Gebiet zusammen (je Gebiet und Cluster des Auges zwischengespeichert, hoechstens 64). Mitte des Fensters: Spiel 24,7 28,1 37,1 / vorher 42,6 45,2 48,2 / jetzt 24,4 28,4 36,5.
- **Reihenfolge der Stufen:** Die Zusatzstufen deckender Shader liefen in der Gruppe der gemischten Flaechen in Kartenreihenfolge. u_metalorange02 (env, dann GL_SRC_ALPHA GL_SRC_ALPHA, dann $lightmap) lag so um env_glass herum, und das Glas wurde mit Alpha und Lightmap des Metalls malgenommen. Im Spiel ist ein SS_OPAQUE-Shader mit allen Stufen fertig, bevor SS_BLEND0 kommt (R_RadixSort, RB_IterateStagesGeneric). Neu `DrawCall::grundDeckend`: Zusatzstufen deckender Shader bilden eine eigene Gruppe nach den deckenden Grundstufen und vor allem Durchscheinenden, auch in `nurLage`. Fenster rechts Spiel 102,9 97,3 93,6 / vorher 44,0 37,6 33,7 / jetzt 102,9 97,3 93,6; Bogen links 69,5 63,8 59,4 / 29,9 24,2 19,9 / 69,5 63,8 59,4. Ohne Glas (alle env_glass-Flaechen auf einen Punkt gelegt, Spiel und Vorschau) waren beide Bilder schon vorher gleich: Das Glas war die Ursache. 6322 von 9777 Shadern der Spieldaten haben eine deckende erste Stufe und weitere Stufen; betroffen ist jede durchscheinende Flaeche, die in der Karte vor ihnen steht.
- **Ferne Waende:** Mipmaps, Filter und Anisotropie sind es nicht: Im Spiel aendert `r_textureMode GL_LINEAR` (ohne Mipmaps) die fernen Flaechen um hoechstens 1 % (Saeule 30,0 24,6 20,6 / 30,2 24,8 20,7). Was bleibt (Saeule 0,86 bis 0,96, von oben nach unten), sind Patches mit `tcGen environment`: Das Spiel unterteilt Patches mit R_SubdividePatchToGrid und laesst je Bild nach Abstand Zeilen und Spalten weg (RB_SurfaceGrid, r_lodCurveError 250), die Vorschau unterteilt gleichmaessig (`buildMesh`, meshDetail); die Umgebungskoordinaten je Ecke liegen damit anders. Nicht behoben (offen).
- Andere Karten: Alle Paare aus Teil 8 und ga1 neu aufgenommen, Boden und oben 0,99 bis 1,02 wie vorher, Figur 1,00 bis 1,08 wie vorher; Bildzeit 4,2 bis 4,3 ms (Teil 8: 4,2 bis 5,4).

### 1 bis 4: Karten-Editor
**Messweg:** Selbsttest `teil11` mit `BHED_K11_PAARE` (Kamerabild 1920 x 1440 ueber `KameraFenster::messBreite`, Sichtfeld 80 auf der Breite wie cg_fov 80 bei 4:3, Editorhilfen ausgefiltert). Je Paar: `_echt` dekompilierte Karte (Open .bsp mit geschaetzten Lichtern) mit Echtzeit-Licht, `_gebaut` dieselbe nach dem Schnell-Backen im Zeichner des Editors, `_spiel` dasselbe mit dem neuen Schalter, `_vorlage`/`_vorlagespiel` die Original-.bsp als Vorlage (ihre eigenen Lightmaps, wie im Spiel) ohne und mit Schalter. Felder wie in Teil 8: Boden links (0,1000)-(800,1440), Boden rechts (1250,1000)-(1920,1440), oben (0,0)-(1920,400), Mitte (0,400)-(1920,1000), Faktor Editor/Spiel je Kanal.

**Vorher (gemessen):** Der Zeichner des Editors nahm Lightmap und Echtzeit-Licht mal 2 ("wie r_mapOverBrightBits"). duel_jeditemple mit der Kamera von g9_jtb (Rotlauf `k11rot`): Boden rechts 53,7 41,3 17,5 gegen das Spiel 32,1 25,8 13,9 (1,67 1,60 1,26). Stand vor Teil 11 (`k11m1`, nur turm und jtb, Vorlage mit noch mitgezeichneten Pinseln): Vorlage turm Boden 2,26 2,38 2,52 / 2,14 2,24 2,30, jtb 2,98 2,83 2,66 / 2,03 1,91 1,55.

**Weg (gemeinsamer Code):** Keine zweite Formel. "Built map as in the game" (Build-Menue, ab Werk an, gespeichert als BhedBuiltAsInGame) zeichnet die gebaute Karte - das Schnell-Backen, sonst die Vorlage - mit dem Zeichner der Vorschau (`gpu::zeichneKarte`, `zeichneNebel`, `zeichneGluehen`, `spielGammaAnwenden`, Himmelsbox, PVS) in eine eigene Szene (`gpu::szeneWaehlen(1)`: Netzpuffer, Bilder, Lightmaps, Himmel und verdeckte Flaechen werden getauscht wie die Zielsaetze) und einen eigenen Zielsatz (`kZielKarteneditor`). Die Texturen laedt `loadMapTexturesFuer` nach den Regeln der Vorschau (dieselbe Funktion mit umgelenktem Ziel, ohne Gras und Fehlermeldungen). Das Bild kommt mit `kartegpu::bildUnterlegen` unter die Kamera, die gebauten Flaechen schreiben danach nur die Tiefe (`Mischen::NurTiefe`): Auswahl, Griffe, Linien, Filter und Fenster bleiben wie bisher. Lichtstile und `distanceCull` wie loadMapFromBytes. Der Zeichner des Editors selbst (Schalter aus, Echtzeit-Licht) nimmt jetzt `gpu::kSpielLightmapFaktor` (1), dieselbe Zahl wie die Vorgabe der Vorschau (`App::mapBrightness`).

**Nachher, alle Paare (`k11m5`, Faktor Editor/Spiel R G B):**

| Karte | Echtzeit Boden l / r | Schnell-Backen Editor-Zeichner Boden l / r | Schnell-Backen wie im Spiel Boden l / r | Vorlage wie im Spiel Boden l / r / oben / Mitte |
|---|---|---|---|---|
| turm | 2,18 2,08 1,96 / 1,59 1,76 1,76 | 2,18 2,21 2,12 / 1,54 1,74 1,74 | 1,84 1,90 1,81 / 1,50 1,62 1,59 | 1,00 1,01 1,01 / 1,00 1,00 1,00 / 1,00 1,00 - / 1,01 1,02 1,03 |
| kamino | 1,25 1,14 1,04 / 1,29 1,20 1,09 | 1,21 1,11 0,99 / 1,25 1,15 1,03 | 1,40 1,40 1,36 / 1,40 1,40 1,36 | 1,00 1,00 1,00 / 1,00 1,00 1,00 / 1,02 1,00 1,00 / 1,04 1,03 1,02 |
| todesstern | 0,89 0,89 0,89 / 0,58 0,58 0,59 | 0,89 0,89 0,89 / 0,58 0,58 0,59 | 1,11 1,11 1,11 / 1,09 1,09 1,09 | 1,00 1,00 1,00 / 0,99 0,99 0,99 / 1,00 1,00 1,00 / 1,05 1,05 1,04 |
| mustafar | 3,41 2,57 2,20 / 3,25 2,14 1,57 | 3,38 2,61 2,23 / 3,24 2,17 1,58 | 3,29 2,42 1,98 / 3,12 1,94 1,29 | 1,00 1,00 1,00 / 1,00 1,00 1,00 / 1,00 1,00 1,00 / 1,03 1,09 1,11 |
| akademie | 2,74 2,97 2,72 / 2,84 3,09 2,78 | 2,73 2,97 2,71 / 2,84 3,09 2,77 | 3,10 3,14 2,67 / 3,22 3,26 2,72 | 1,00 1,00 1,00 / 1,00 1,00 1,00 / 1,00 1,00 1,00 / 1,01 1,01 1,01 |
| hoth | 1,07 1,07 1,05 / 0,94 0,95 0,92 | 0,45 0,46 0,44 / 0,41 0,42 0,40 | 0,78 0,78 0,78 / 0,71 0,71 0,70 | 1,00 1,00 1,00 / 1,00 1,00 1,00 / 1,00 1,00 1,00 / 1,00 1,00 1,00 |
| byss | 1,08 1,08 1,06 / 1,18 1,18 1,17 | 1,08 1,08 1,06 / 1,18 1,18 1,17 | 1,36 1,36 1,36 / 1,36 1,36 1,36 | 1,02 1,02 1,02 / 1,02 1,02 1,02 / 1,03 1,03 1,03 / 1,01 1,02 1,02 |
| e_must | 2,30 1,75 1,63 / 1,81 1,28 1,16 | 2,01 1,61 1,50 / 1,13 0,97 0,93 | 1,53 1,31 1,23 / 0,95 0,75 0,66 | 1,00 1,00 1,00 / 1,00 1,00 1,00 / 0,99 0,98 0,98 / 0,90 0,86 0,75 |
| e_stern | 0,89 0,89 0,89 / 0,58 0,58 0,59 | 0,89 0,89 0,89 / 0,58 0,58 0,59 | 1,11 1,11 1,11 / 1,09 1,09 1,09 | 1,00 1,00 1,00 / 0,99 0,99 0,99 / 0,54 0,61 0,72 / 0,80 0,83 0,87 |
| e_lb | 1,96 1,89 1,93 / 1,91 1,84 1,87 | 1,97 1,88 1,92 / 1,93 1,84 1,87 | 1,89 1,72 1,72 / 1,86 1,70 1,70 | 1,00 1,00 1,00 / 1,00 1,00 1,00 / 1,00 0,97 0,90 / 0,90 0,91 0,96 |
| jtb | 4,81 5,13 3,88 / 4,07 3,88 3,07 | 4,81 5,12 3,88 / 4,07 3,88 3,07 | 5,89 6,53 6,50 / 5,16 5,23 5,48 | 1,10 1,20 1,15 / 1,00 1,00 1,00 / 0,91 0,99 1,19 / 0,91 0,98 1,18 |
| ga1 | 0,49 0,52 0,80 / 0,49 0,52 0,80 | 0,49 0,52 0,80 / 0,49 0,52 0,80 | 0,48 0,49 0,73 / 0,48 0,49 0,73 | 1,00 1,00 1,00 / 1,00 1,00 1,00 / 0,99 0,98 0,96 / 1,01 1,01 1,01 |

Vollstaendig (auch Vorlage im Editor-Zeichner, oben und Mitte je Zustand): `SP/g11/tab_nachher.md`. Lesart:
- **Gebaut zu Spiel (Darstellung):** Vorlage wie im Spiel: Boden 1,00 bis 1,02 auf allen zwoelf Karten (jtb Boden links 1,10 bis 1,20: dort steht im Spiel der Spieler im Feld). "oben"/"Mitte" weichen ab, wo das Spiel Figur, Effekte oder misc_model_static zeigt (e_stern oben 0,54: der Falke, e_must Mitte 0,75 bis 0,90). Im Zeichner des Editors (Schalter aus) dieselben Vorlagen 0,49 bis 1,49.
- **Gebaut zu Spiel (Bau):** Das Schnell-Backen der dekompilierten Karte mit geschaetzten Lichtern liegt 0,5 bis 6,5 daneben: Es ist nicht der Bau des Kartenautors (andere Lichter, -fast, kein Bounce). Das ist der Bau, nicht die Darstellung.
- **Echtzeit zu gebaut:** auf den meisten Karten gleich (Boden 0,00 bis 0,14 Unterschied im Faktor, wie 9c), groesser auf hoth (1,07 gegen 0,45 im Editor-Zeichner) und kamino oben (1,27 gegen 0,71). Die Echtzeit-Ansicht zeigt nur die erste Stufe je Shader; das Schnell-Backen wie im Spiel weicht darum vom Editor-Zeichner ab (hoth 0,45 -> 0,78, byss 1,08 -> 1,36).

**Bildzeit** (mustafar, Kamera dreht 240 Bilder, ganze Anwendung): Zeichner des Editors mit dem Schnell-Backen 6,94 ms (p99 9,13), "Built map as in the game" 4,84 ms (p99 7,69), davon Spielbild auf dem Prozessor 1,10 ms je Bild. Erstes Spielbild einer neuen .bsp: Aufbau mit Texturen 1,0 bis 3,0 s.

**_noshadersun, _minVertexlight (Befund Teil 10):** Das Echtzeit-Licht laesst mit `_noshadersun` die Sonnen der Shader weg (q3map_skyLight bleibt, light.cpp:593) und gibt Flaechen ohne Lightmap (C_VERTEXLIT: surfaceparm nolightmap/pointlight) `_minvertexlight` als Mindestlicht (light.cpp:2333-2349, lightmaps_ydnar.cpp:3143), sonst `_minlight`. Dabei gefunden, allgemein: q3map2 liest Entity-Schluessel ohne Gross- und Kleinschreibung (CASE_INSENSITIVE_EPAIRS, q3map2.h:109), das Echtzeit-Licht las sie genau; duel_geonosis_arena schreibt `_minVertexlight`. Alle Schluessel des Echtzeit-Lichts jetzt wie q3map2 (`wertQ3`). Gegenprobe ga1 (Geonosis-Arena, `_noshadersun 1`): Echtzeit und Schnell-Backen derselben Karte gleich (Boden 0,49 0,52 0,80 beide); mit der Himmelssonne 170 waere die Echtzeit-Ansicht heller als der Bau.

### Tests
- ctest `schmerz` (PVS, 3 Zeilen), `gpudraw` (Stufenfolge, 2), `karteechtlicht` (_noshadersun, _minvertexlight, Gross-/Kleinschreibung, 3): je einmal rot (Regel zurueckgenommen bzw. genau verglichen), dann gruen. ctest einmal am Ende: 195 von 196, `handbuch_sprachen` fehlte die Anmeldung von `i18n_rc608_11` in gen_handbuch.py, danach gruen.
- Selbsttest `teil11` (neu, ohne Umgebungsvariable): duel_jeditemple aus den Spieldaten als Vorlage, Kamera wie das Spielbild. 3 OK, 0 FEHLER (`k11gruen`): Boden 32,0 25,8 13,9 (Spiel 32,1 25,8 13,9); Fenster rechts 102,9 97,3 93,6, Bogen 69,5 63,8 59,4, Mitte 24,4 28,3 36,5, 30959 Flaechen ausserhalb des PVS; Editor-Zeichner Boden 26,8 20,6 8,7. Rotlauf `k11rot` mit Stufenfolge, PVS und Faktor 2 zurueckgenommen: Fenster und Editor-Zeichner rot (44,0 37,6 33,7 / 53,7 41,3 17,5); die Boden-Zeile des Spielbilds war dort gruen, ihr Rot ist der Editor-Zeichner (ohne Spielbild gab es kein anderes Bild).
- Selbsttest karteneditor: nur die Groesse des Build-Menues angepasst (+24 fuer den neuen Eintrag), der Lauf gehoert in die Endrunde.
- Lints: lint_sources, lint_fassung, lint_i18n, lint_i18n_erzeugt, lint_cbuffer, lint_gapp ok; lint_d3dbesitz 15 wie vorher; lint_settings wie vorher; lint_portability 117 (eine weniger, areaportal.cpp). Bau 0 Warnungen.

### Nur per Foto gegen das Spiel
Mission-Vorschau: `SP/g11/erg/paar_jtb.png`, `paar_m_*.png`. Karten-Editor: `SP/g11/erg/k11_<karte>_<zustand>.png` (oben Spiel, unten Editor).

### JK2-Datapad (Zusatz waehrend Teil 11, Steam angemeldet, vor dem Start geprueft)
bespin_platform statt kejim_base, `give all`, Datapad Waffen, `dpweapnext` (`SP/g11/jk2dp.py`, eigener Ordner, GameData danach gleich, Dateiliste verglichen). Gegen die Vorschau mit derselben Waffe (`BHED_MENUE_VERGLEICH_WAFFE`, neu): `SP/g11/erg/jk2_datapad_w1_spiel_vorschau.png` (Lichtschwert), `_w2` (Bryar), `_w3` (E-11). Gefunden (nicht behoben): Die Vorschau setzt die Zeilen enger und die Schrift breiter als das Spiel. Zeilenabstand bei 1280 x 960: Spiel 38, Vorschau 36 Bildpunkte (19 gegen 18 virtuelle Punkte); Textbreite: "Primary Fire - Slow, single shot with moderate accuracy and great damage." Spiel 977, Vorschau 987 Bildpunkte (+1,0 %), Bryar "hand-held ... accuracy." 973 gegen 986 (+1,3 %). Darum bricht die Vorschau frueher um: E-11 "despite its / small size" statt "despite its small / size", Lichtschwert "energy blade / of great" wie in Teil 10b. Ursache vermutlich RE_Font_HeightPixels/RE_Font_StrLenPixels der Original-exe (Rundung je Zeichen); die OpenJK-Fassung ist 1:1 uebernommen und trifft das Original nicht.

### Offen aus Teil 11
- Patches wie im Spiel unterteilen (R_SubdividePatchToGrid, LOD je Bild nach r_lodCurveError): ferne env-Patches auf duel_jeditemple 0,86 bis 0,96.
- Spielbild des Editors ohne Figuren, Effekte, misc_model_static (Licht ueber das Gitter), Wetter, Surface-Sprites, misc_skyportal, Mover-Stellungen (Brush-Modelle stehen, wo q3map2 sie abgelegt hat).
- Echtzeit-Ansicht: nur erste Stufe je Shader (der Tooltip sagt es), kein Bounce/Filter/Supersampling.
- JK2-Datapad: Zeilenabstand und Textbreite (siehe oben).
- Original-JKA fuer Teil 11 nicht aufgenommen.
- Messung "vorher" ueber alle Karten nur fuer den Editor-Zeichner mit Faktor 2 auf jtb; die anderen Karten haben im alten Stand dieselbe Formel (Faktor 2).

## Offen
- 3 Reiten: ctest und Selbsttest gruen (Abstand zum Fahrzeug 2,8), nicht gegen das Spiel. Nur im Bild; Ziele und Ton rechnen mit dem Stand aus dem Skript. Fehlen: Absteigeort, Neigung des Fahrzeugs, Fahrzeug unter NPC-Steuerung, Angriffe und Lehnen beim Reiten. In den installierten Skripten kommt SET_VEHICLE nicht vor.
- 4 `cg_turnAnims`: rot 0, gruen 2 Bilder, nicht gegen das Spiel. Drehen wird im Servertakt (50 ms) erkannt.
- 5 Blick: rot 0, gruen 8,3 frueh, 72,2 spaet. Nur per Foto, Richtung passt, nicht gemessen. Bildtakt com_maxfps 125 aus OpenJK sys_main.cpp:162; in Movie Duels nicht belegt. Skip: Nick im Kampf (Gegner). Figur wird je Schritt neu berechnet, Leistung nicht gemessen.
- 6 `g_ActivateAnimationStyle`: Schalter, Meistersatz, Skelettpruefung, Varianten in pmanim. Rest der 662 Stellen in bg_pmove, wp_saber, bg_panimate nicht nachgebildet. `npcSprungAnim` nicht angeschlossen. Nicht gegen das Spiel.
- Handbuchtexte in 6 Sprachen geschrieben, **`gen_handbuch.py` lief nicht**, Seiten nicht erzeugt. Changelog-Zeilen fehlen noch.
- Byss-Lichtleiste im Spiel 55 220 226, Vorschau 56 247 255, nicht untersucht.

## Teil 9b: Rest von Teil 9 (nach dem Neustart)

Auftrag `auftrag_rc608_9b.md`. Skripte und Fotos in `SP/g9` (`karten9b.py`, `jt9c.cfg`, `figur9b.*`), Paare in `SP/g9/erg/`. Spielbilder nur Movie Duels mit fs_homepath im SP (Steam lief nicht).

### 1: Bau, cg_motionBoneComp
Gebaut, `teil9` 22 OK. `cg_motionBoneComp` rot mit abgeschalteter Regel (mapview.cpp, alle Werte gleich behandelt): Kopfort 94,874 bei allen vier Stufen. Gruen: 102,020 / 95,573 / 94,874 / 92,740 (0, 1, 2, 3); die Zeile prueft bei 150 ms mit eigenem Oberkoerper (BOTH_ATTACK2 auf BOTH_STAND1).

### 2: $lightmap hinter zwei Bildstufen
Paar `paar_jt9b_b.png` (g9_jtb, deckungsgleiche Kamera -1250 -20 -1113,75, Blick 90) war nach Teil 9 noch **rot**: Boden 15,6 grau gegen 32,1 25,8 13,9. **Gefunden, allgemein:** Bilder einer Stufe (`map`, `clampMap`, `animMap`) und die Seiten einer Himmelsbox wurden wie Shadernamen gesucht. `textures/jeditemple/u_metalorange02` ist zugleich ein Shader, dessen erste Stufe env_shiny_floor zeigt - die zweite Stufe bekam das Spiegelbild (256x256, Mittel 51) statt des Metalls (512x512, 96 68 48). Die Engine nimmt R_FindImageFile (tr_shader.cpp:1221, 1245, 1295, 1929 MD): neu `loadImageFile`. Boden jetzt 32,0 25,8 13,9 (Selbsttest `teil9`, gegen das Spielbild), im Paar Faktor 1,00 1,00 1,00, Statue 1,00, Saeulen hinten 0,95 0,94 0,92.

### 3: Reiten, cg_turnAnims, CG_UpdateLookAngles, g_ActivateAnimationStyle
Nicht gegen das Spiel geprueft. Reiten und cg_turnAnims brauchen Eingaben im Spielfenster bzw. ein Fahrzeug unter Skriptsteuerung; ohne Fokus laeuft dort keine Taste, und die Testkarte hat kein Wegnetz (siehe 4). Blick und Stil nur gegen die Quelle (ctest, Selbsttest). Punkt 6, Rest der 662 Stellen: Sie waehlen Hiebe, Paraden, Bloecke und Kampfhaltungen gegen einen Gegner (bg_pmove, wp_saber, bg_panimate); die Vorschau rechnet keinen Schwertkampf. Nachgebildet ist, was Haltung und Gang ohne Kampf waehlt (pmanim.cpp).

### 4: Schwert-Oberkoerper
**Gefunden, allgemein:** Die Vorschau las alle .npc-Dateien, auch `ext_data/MD_MP_NPC/jedi.npc` (Mehrspieler: saberColor random, saberStyle zuletzt 1). Das Spiel liest nur `ext_data/npcs` (NPC_LoadParms, NPC_stats.cpp:4733 MD; FS_ListFilteredFiles, files.cpp:2094-2096), in der Reihenfolge der Suchpfade. Rot: NPC_Jedi Typ Jedi, Farbe green, Stil 1, BOTH_SABERFAST_STANCE_JKA. Gruen: yellow, Stil 2, BOTH_STAND2 (PM_ReadyPoseForSaberAnimLevelNPC, bg_pmove.cpp:18781 MD bei g_SerenityJediEngineMode 0, so in der Spiel-Config). Paar `paar_figur9b_steht.png`: Farbe und Hand stimmen, **die Klinge zeigt in der Vorschau senkrecht, im Spiel nach vorn (stark verkuerzt) - offen, Ursache nicht gefunden.** Gehen nicht aufgenommen: Der NPC bewegt sich im Spiel auf der Testkarte g9_figur nicht (kein Wegnetz, f9b_g0..g7 alle im Stand).

### 5: JK2 und Show player
`teil9` laedt kejim_post aus assets0.pk3 (Regelwerk automatisch): kyle ist Spieler mit Modell, Skelett 72 Knochen, verborgen vor und nach der Kamera, mit Schalter vor der Kamera sichtbar. Rot mit abgeschalteter kyle-Regel (Kamera 0, vier Zeilen rot), gruen. Die Selbsttests setzen den Schalter nicht mehr global: nur `teil9` (beide Stellungen) und der Spielvergleich (geschuetze, BHED_GV_*) schalten ihn an. In allen Selbsttests nach Skripten mit Kamera und Spieler gesucht: keiner prueft den Spieler ausserhalb einer Kameraspanne; ob ein voller Lauf trotzdem etwas findet, zeigt erst die Endrunde.

### 6: MSAA
Neu `r_ext_multisample` in den Ansichtseinstellungen, Vorgabe wie das Spiel: Movie Duels 8 (im Spiel abgefragt, rd-rend2/tr_init.cpp:1522 MD), OpenJK 0 (sdl_window.cpp:755), Original-JKA und Jedi Outcast 0. Alle Zielsaetze zeichnen mit Proben; Gluehen, Spielgamma, Umriss, Spiegel und Anzeige lesen das aufgeloeste Bild, die Tiefe fuer die Gizmos kommt ueber `tiefeProbePixelShaderHlsl` (Probe 0). Selbsttest auf duel_mustafar 1350 x 931: 8 Proben, 10471 von 1256850 Bildpunkten anders (0,8 %), mittlere Abweichung 0,06; Bildzeit 7,67 ohne, 8,03 ms mit. Rot mit erzwungener Probe 1. Byss-Glas (`paar_byss_msaa.png`, `paar_byss_ohne.png`, Ausschnitt 4fach `glas_kante_byss.png`): groesster Einzelschritt am Kantensprung im Mittel Spiel 0,52, Vorschau mit 0,60, ohne 0,68; die Stufen an der Unterkante sind weg (angesehen).

### 7: Falcon-Gegenprobe duel_jeditemple
Deckungsgleich ueber Karten mit genau einem Start (g9_jta, g9_jtb; viewpos -250 3750 590 / -1250 -20 -1113). crate_01 15,1 17,4 21,4 / 15,1 17,4 21,5 und 5,6 6,0 7,7 / 5,6 6,1 7,8 (Faktor 1,00); statue_guard 32,8 23,3 12,3 / 32,9 23,4 12,3 (1,00). Ganzes Bild 0,99 1,00 1,00 bzw. 0,98 0,97 0,97.

### Tests
`teil9` 29 OK, 0 FEHLER (neu: MSAA, JK2 mit Spieldaten, NPC_Jedi-Haltung, Boden u_metalorange02). Rot je einmal: MSAA, JK2, NPC_Jedi und cg_motionBoneComp in einem Lauf (7 FEHLER), Boden im eigenen Lauf. ctest `gegenproben`: neue Zeilen r_ext_multisample und Rundlauf der Zahlen; ctest einmal am Ende: 196 von 196.

### Nebenbei gefunden
`cg_motionBoneComp` (Teil 9) wurde ueber die bool-Fassung von `put` gespeichert: 2 und 3 kamen als 1 zurueck. Rot in ctest `gegenproben` (neue Zeile mit 3 und r_ext_multisample 4), gruen mit std::to_string.

### Lints
lint_sources und lint_fassung ok. lint_d3dbesitz 15 statt 14: `g_maskeAufTex` wird wie seine Geschwister in gibUmrissFrei freigegeben, das die Pruefung nicht verfolgt. lint_settings meldet `rExtMultisample` wie schon `bildPicmip` und `schwierigkeit` (std::to_string erkennt sie nicht).

### Nur per Foto gegen das Spiel
Paare in `SP/g9/erg/`, Movie Duels. OpenJK-Regeln nur gegen die Quelle.

### Offen aus Teil 9b
- Klinge des NPC in BOTH_STAND2 (siehe 4).
- g9_jtb: Der Spieler steht im Spiel rund 95 Bildpunkte tiefer im Bild als in der Vorschau, die Kamera ist deckungsgleich (Hang am Statuensockel; PM_AdjustStandAnimForSlope nicht geprueft).
- Saeulen auf duel_jeditemple hinten 0,95 0,94 0,92.
- Byss-Lichtleiste 55 220 226 gegen 56 247 255 weiter offen.
- Figurstufen (Ghoul2) laden ihre Bilder auf eigenem Weg, dort nicht auf dieselbe Verwechslung geprueft.

## Teil 10: Geonosis, Reste aus 9b, Decals, VIS-Rest, JK2-Datapad

Auftrag `auftrag_rc608_10.md`. Skripte und Fotos in `SP/g10` (`karten10.py`, `karten10b.py`, `geo.cfg`, `lauf2.cfg`, `vor10.sh`, `vor_geo.sh`, `vor_rest.sh`, `grid.py`, `ersetz.py`), Paare in `SP/g10/erg/`. Spielbilder nur Movie Duels mit fs_homepath im SP (`g5/home_md`), Steam lief nicht (keine steam.exe). Aufnahme per `screenshot_tga`.

### 0: Reste aus Teil 9b
- **Schwertfarbe des Spielers, gefunden, allgemein:** Der Spieler (und jede Figur mit `playerModel player`) bekommt sein Schwert aus den Cvars: G_InitPlayerFromCvars ruft G_SetSabersFromCVars (g_client.cpp:2488 MD, :1850 OpenJK; fuer NPCs nach NPC_ParseParms, NPC_stats.cpp:4646 MD): saber[0] neu aus g_saber ("single_1"), alle Klingen in g_saber_color ("yellow", g_main.cpp:829-831 MD, :693-695 OpenJK). Die Vorschau nahm single_1 mit dessen `saberColor random` und wuerfelte (violett). Neu `spielerSchwertAusCvars` (src/ausruestung.cpp), Laenge und Radius der .npc fallen wie in WP_SaberParseParms weg. Rot: `Spieler Schwert single_1, Klingen 1: purple`, gruen: `yellow`. Paar `paar_figur.png` (gegen `g9/fotos/f9b_g0.png`): beide Klingen gelb. Licht gemessen: Boden neben dem Spieler Spiel 26,5 22,5 9,2 / Vorschau 19,7 15,3 8,1 (Faktor 0,74 0,68 0,88), Ruecken 58,0 39,8 12,9 / 70,2 45,3 18,2 (1,21 1,14 1,41) - siehe Offen (Schwertlicht).
- **Gegenstand an der Huefte:** kommt von `give all` in `figur9b.cfg` - Movie Duels haengt die Waffen des Inventars ins Holster. Gegenprobe ohne `give all` (`lauf2.cfg`, Bild `f10_g0.png`): beide Hueften leer. Keine Abweichung der Vorschau; ihr Spieler hat ausser dem Schwert kein Inventar.
- **Klinge des NPC in BOTH_STAND2:** nicht gefunden. Gegen die Quelle geprueft: Bolzen der Tagflaeche (G2_ProcessSurfaceBolt2, tr_ghoul2.cpp:2260 ff. MD: Ecken in Dateireihenfolge, Seite 0 laengste, 2 kuerzeste, Ursprung Ecke 2), Klingenrichtung NEGATIVE_X von `*blade1` (cg_players.cpp:13983 MD), Anheften an `*r_hand`, BOTH_STAND2 in beiden animation.cfg gleich (Frame 14917). Griff single_2 aus MD_Hilts.pk3. Offen.
- **g9_jtb (Spieler tiefer):** Der Spieler bekam PM_AdjustStandAnimForSlope nicht - die Vorschau rechnete die Hangbeine nur fuer NPCs. Die Engine laesst den Spieler nur in der ersten Person aus (bg_pmove.cpp:7454 OpenJK, :11497 MD mit BG_AllowThirdPersonSpecialMove; cg_thirdPerson "1" in JKA, cg_main.cpp:414 OpenJK, :623 MD; Jedi Outcast "0" mit cg_saberAutoThird/cg_gunAutoFirst, g_active.cpp:1708-1714). Rot: Beine BOTH_STAND2, gruen: LEGS_LEFTUP1 ab 50 ms. **Der Hoehenunterschied bleibt:** Kopfoberkante Spiel Zeile 981, Vorschau 905 (76 Bildpunkte, vorher gleich) - die Hangbeine sind nicht die Ursache. Offen.
- **Byss-Lichtleiste:** neu aufgenommen mit und ohne r_DynamicGlow (Spiel `byss_g1/g0`, Vorschau mit `BHED_GV_GLUEHEN=0`). Leiste oben (205,0-320,15), Spiel / Vorschau: mit Gluehen 63,7 118,7 147,4 / 63,8 124,2 158,3, ohne 63,2 103,3 118,0 / 63,2 109,4 133,2. Das Gluehen ist nicht die Ursache; die Leiste selbst (Lightmap mal `lights`, `lights_glw` additiv, `env_glass` davor) ist in der Vorschau G 6 %, B 13 % heller und saettigt (hellste 20 Bildpunkte 76 255 255 gegen 71 220 220). Nicht gefunden. Die linke Leiste ohne Glas stimmt (147 164 177 / 145 164 175). Nebenbei: Das PrintWindow-Bild aus Teil 8 war dort rund 4 % dunkler als `screenshot_tga` (141 156 170).
- **Saeulen duel_jeditemple 0,95 0,94 0,92:** nicht neu angegangen.
- **Figurstufen (Ghoul2):** Ihre Bilder laden ueber `ladeStufenBild` nur als Datei (wie R_FindImageFile) - die Verwechslung aus 9b gibt es dort nicht. Verwandt gefunden: Fand eine Figurflaeche ihr Bild nur ueber das Shaderskript, legte die Vorschau es im Zwischenspeicher unter dem SHADERnamen ab; eine Stufe `map <Shadername>` haette dann das Bild des Skripts bekommen, das es als Datei nicht gibt (und eine zweite Figur mit derselben Haut verlor den Alphatest). Jetzt unter dem Namen der gelesenen Datei (gui/app_view3d.cpp). Nicht rot gesehen: In keinem Lauf dieses Teils kam der Weg "ueber Shaderskripte" vor.

### 1: "Schatten von oben" auf Geonosis
Sechs Stellen (`g10_ga1..3` Arena, `g10_gb1..3` Schlacht, je genau ein Spielerstart), je vier Spielbilder im Abstand von 1 s: nichts wandert (mittlere Aenderung 0,00 bis 0,23 von 255; gb2 45,9, weil der Spieler am Hang 1 Einheit rutscht und die Kamera mitnimmt).
- **Was der Schatten ist:** gebackenes Licht der Karte. Der Himmelsshader `textures/md_gd/geonosis` traegt `sun 1 0.584 0.169 170 45 75` (q3map_sun) und `q3map_surfacelight 10`; q3map2 rechnet daraus Lightmap und Lichtgitter (CreateSurfaceLights, light.cpp:590 ff.). Auf der Arena ist das der harte Schattenstreifen der Ringmauer quer ueber den Sand und der helle Keil an der Wand (`paar_ga1.png`), auf der Schlacht das Licht von oben auf Figur und Modellen (Schultern hell, Koerper dunkel, `paar_gb2.png`). Die Vorschau zeigt beides aus denselben Lumps: Boden links und rechts auf allen sechs Stellen Faktor 1,00 1,00 1,00, oben 0,96 bis 1,01, Figur 0,96 bis 1,09 (gb3 1,04 1,07 1,09). Kein cg_shadows-Fleck, keine wandernden Wolken: cg_shadows 1 (Rundschatten) hat die Vorschau schon.
- **Dabei gefunden, allgemein:** Die Zuschauer der Arena sind Brushflaechen mit `textures/md_ga/geonosian1` (ebenso `galler1`): { Bild alphaFunc GE128 } { $lightmap blendFunc GL_ONE GL_ZERO } { Bild blendFunc GL_DST_COLOR GL_ZERO }. GL_ONE GL_ZERO mischt nicht, die Lichtstufe ersetzt die Farbe ("implicitly assume that a GL_ONE GL_ZERO blend mask disables blending", ParseStage; RB_IterateStagesGeneric, tr_shade.cpp); die erste Stufe bleibt nur Abdeckung. Die Vorschau rechnete Bild mal Licht in die Grundstufe und multiplizierte mit der dritten Stufe noch einmal: Bild mal Bild mal Licht, fast schwarz. 86 Shader der Spieldaten haben diese Form (Hoth-Eiszapfen, Kejim, Yavin-Ranken, Gitter, Fenster), bei einem ist das Bild der dritten Stufe ein anderes (`textures/wedge/w_catwalk_r`: Abdeckung aus w_catwalk_r, Farbe aus w_catwalk). Neu `lichtErsetztBildstufen`/`farbeAusStufe` (gui/app_ablage.inc). Rot: `galler1 Zusatz 1 soll 0`, `geonosian1 Zusatz 1 soll 0`; gruen: 0 / 0. Zuschauer im Paar `paar_ga1.png`: Spiel 26,4 13,4 7,7 / vorher 23,8 11,1 5,7 / jetzt 26,4 13,4 7,7; drei Felder unten 1,00 1,00 1,00.
- **Gegenprobe q3map_skyLight** (nicht zum Finden benutzt): mp/siege_hoth (`textures/siege/hoth_sky`, `q3map_skylight 350 6`), zwei Stellen im Freien (`g10_sh1`, `g10_sh2`): Boden 1,00 1,00 1,00, oben 0,99 bis 1,00, Figur 0,98 bis 1,00.
- **Karten-Editor (nur Befund, das Aussehen gehoert in Teil 11):** Das Echtzeit-Licht (src/karte/echtlicht.cpp) wertet `sun`/`q3map_sun`/`q3map_sunExt` (shaderscript.cpp:853), `q3map_surfacelight`, `q3map_skyLight` und `_color`/`ambient`/`_minlight` wie q3map2 aus, **aber nicht `_noshadersun`**: q3map2 laesst damit die Sonnen aller Shader weg (CreateSurfaceLights, light.cpp:593). duel_geonosis_arena als .map bekaeme im Editor die Himmelssonne 170 bei 45/75 Grad, die q3map2 nicht rechnet. `_minVertexlight` liest der Editor ebenfalls nicht.

### 2: Decal-Sonderfaelle
Nicht angegangen (Zeit). Offen wie im Kopf von `src/karte/metaflaechen.inc`: FogDrawSurfaces, Index-Karten, -patchmeta, -maxarea, -np, Klon- und Rueckseiten-Shader, _decal.

### 3: VIS-/Licht-Rest
Stand (aus `NOTIZEN_vis1.md`, vis4, in karte1 seit c809c2f1): Auto mit drittem Kandidaten (GPU plus Nachproben, dd8737f7) ist drin und getestet (ctest karte_gpuwahl, q3map2_gpubake, Selbsttest stufe8g). IlluminateRawLightmap: GPU-Sammelpuffer (642dd15a, 4083fce7). RandomSubsampleRawLuxel: auf dem Prozessor der groesste Abschnitt von "[Final] Extreme Quality", kein bitgleicher schnellerer Weg gefunden. LoadSurfaceExtraFile/SetupSurfaceLightmaps: Laden duel_mos_espa 23,7 bis 25,8 s, kejim_post 1,8 s (unter Last). PointInLeafNum je Nachprobe: nicht angegangen. Die ruhige Endmessung (`SP/vis4/messung.sh`, rund 1 h) und der Bytevergleich mit dem Endstand stehen aus. Nichts geaendert, nichts neu gemessen - fuer rc609.

### 4: JK2-Datapad-Foto
Nicht moeglich: Das Foto braucht das Original-Jedi-Outcast (Steam), steam.exe lief nicht. Offen.

### 5: Reiten und cg_turnAnims gegen das Spiel
Nicht angegangen (Zeit). Fuer den naechsten Teil: cg_turnAnims laesst sich ohne Tasten vergleichen - ein NPC dreht sich per Skript (`SET_ANGLES`), cg_players.cpp:3095 MD spielt die Drehanimation dann ebenso.

### Tests
Selbsttest `teil10` (neu): 3 OK, 0 FEHLER. Rot je einmal: Schwert und Hang in einem Lauf (2 FEHLER, `t10rot`), Lichtstufe im eigenen Lauf (1 FEHLER, `t10rot2`). Paare aus dem Stand vor der Aenderung an den Figurtexturen (die aendert bei 0 Faellen nichts). Der Vergleich (`geschuetze` mit `BHED_GV_*`) nimmt `BHED_GV_GLUEHEN=0` neu an.

### Nur per Foto gegen das Spiel
Alle Paare in `SP/g10/erg/` (Movie Duels). OpenJK-Regeln nur gegen die Quelle.

### Offen aus Teil 10
- Schwertlicht: Das installierte Movie Duels (Update 9) hat `cg_saberLightIntensity 1.2` und `cg_saberLightRadius 1.5` in der Konfiguration, die Quelle unter `SP/mdgit` kennt beide nicht. Boden neben dem Schwert in der Vorschau 0,68 bis 0,88, Figur 1,14 bis 1,41; der Lichthof um die Klinge ist im Spiel breiter. Ohne Quelle nicht nachgebaut.
- Klinge des NPC in BOTH_STAND2, g9_jtb (76 Bildpunkte), Byss-Leiste hinter Glas (siehe 0).
- Karten-Editor: `_noshadersun`, `_minVertexlight` (Teil 11).
- Punkte 2, 3 (Messung), 4, 5.

## Teil 10b: Rest von Teil 10

Auftrag `auftrag_rc608_10b.md`. Skripte und Spielbilder in `SP/g10b` (`md10b.py` mit `HOMEDIR`, `sl.cfg`/`sl2.cfg`, `mess_sl.py`, `dreh*.cfg`, `npc.cfg`, `jtb*.cfg`, `dis1.py` bis `dis6.py`, `glas.py`, `skinsuch.py`, `vor10b.sh`), Paare in `SP/g10b/erg/`. Spielbilder nur Movie Duels mit fs_homepath im SP (`g5/home_md`; frische Installation in `g10b/home_frisch`). Steam lief nicht (keine steam.exe), Original und Jedi Outcast darum nicht.

### 2: Schwertlicht (gefunden, allgemein)
- **Woher die Cvars kommen:** Die installierte `MovieDuels-gamex86_64.dll` (Update 9) traegt `cg_saberLightIntensity` und `cg_saberLightRadius` in ihrer cvarTable mit den Vorgaben "1.070312" und "0.703125" (CVAR_ARCHIVE). Im Spiel mit leerem fs_homepath gelesen: genau diese Werte. Die 1.2 und 1.5 der Aufnahme-Konfiguration stammen aus dem Einstellungsmenue des Spiels (`ui/setup.menu` in `z_zzzz_MD_Jace-Update-9.pk3`: Schieber 0,5 bis 1,5 und 0,5 bis 2,5, Vorschlag 1.2 und 1.5).
- **Regel, benannter Messgrund** (SP/mdgit kennt die Cvars nicht): Disassembliert (capstone) in CG_DoSaberLight (0x2fc6db ff.) und den 14 Klingenfunktionen (z. B. CG_DoSaber/CG_DoSFXSaber 0x2fcfa5 ff.): `R = cg_saberLightRadius > 0 ? Wert : 1`, `I = cg_saberLightIntensity > 0 ? Wert : 1`, Radius mal R, Farbe mal I, dann cgi_R_AddLightToScene. Im Spiel gemessen (`mess_sl.py`, timescale 0, je 5 Bilder gemittelt, Boden links neben dem Spieler): I 0 / 1 / 1,2 / 2 bei R 1,5: 39,1 / 37,1 / 38,7 / 24,7 (bei 2 laeuft die Farbe ueber, der Boden wird violett); R 0 / 1 / 1,5 / 2 bei I 1,2: 21,0 / 21,1 / 39,9 / 61,4. 0 wirkt wie 1.
- **Dabei gefunden, allgemein (die eigentliche Abweichung):** Ein Schwert mit einer oder zwei Klingen gibt JEDER Klinge ihr eigenes Licht ueber CG_DoSaber(..., doLight = !noDlight) (cg_players.cpp:5791-5796 und 6711-6758 OpenJK, 11953-11958 und 14535-14800 MD); CG_DoSaberLight (gemischtes Licht) nimmt das Spiel erst ab drei Klingen (CG_AddSaberBlade "add blended light") oder fuer Griffe ohne Klinge. Die Vorschau nahm das gemischte Licht fuer jedes Schwert: Radius Durchmesser + Zufall*8 und volle Farbe. Jetzt je Klinge: Movie Duels Laenge*2 + Zufall*10 mit Farbe*0,66 (CG_DoSaber und alle Stile von cg_SFXSabers), die instabilen Typen (SABER_UNSTABLE, SABER_STAFF_UNSTABLE, SABER_STAFF_ELECTROSTAFF, SABER_SINGLE_KYLO_REN, SABER_ELECTROSTAFF: CG_DoSaberUnstable) Laenge*1,4 + Zufall*3 mit voller Farbe; OpenJK Laenge*1,4 + Zufall*3; Jedi Outcast Laenge*2 + Zufall*8 (wie bisher). noDlight/noDlight2 der .sab werden gelesen (Saber_ParseNoDLight).
- **Byte der Lichtfarbe:** ProjectDlightTexture2 schreibt die Farbe als `byte colorTemp[4] = Q_ftol(...)` (tr_shade.cpp:684/842 OpenJK, 670/824 MD; in der installierten MovieDuels-rdsp_x86_64.dll ebenso, 0x719a6 ff.: cvttss2si und movzx al). Ueber 255 laeuft sie modulo 256 ueber. Der Lichtshader der Vorschau rechnet jetzt genauso.
- **Einstellungen:** `cgSaberLightIntensity`/`cgSaberLightRadius` (Vorgaben der frischen Installation), Schieber in den Figuren-Einstellungen, nur unter Movie Duels. Der Vergleich nimmt `BHED_GV_SCHWERTLICHT="1.2 1.5"` wie die Aufnahme-Konfiguration.
- **Gegen das Spiel (Paare `paar_sl_i12/i1/i2/r1/r2.png`, Spieler allein, Spiel bei timescale 0):** Boden neben dem Spieler (650,1150-820,1300) Faktor Vorschau/Spiel bei 1,2/1,5: 1,08 1,11 1,04; bei 1/1,5: 1,02 1,03 1,01; bei 2/1,5 (Ueberlauf): 0,96 0,94 0,94; bei 1,2/1: 1,01 1,02 1,00; bei 1,2/2: 0,98 0,98 0,99. Boden rechts (1350,1150-1550,1300) 0,98 bis 1,10. Figur (880,1150-1000,1280) 0,82 0,79 0,82 bei 1,2/1,5, 0,94 bei R 1, 0,99 bei I 2. Vor der Aenderung: violetter Boden bei 1,2/1,5 (Ueberlauf der vollen Farbe), mit dem alten Bild `paar_figur.png` Ruecken 1,21 1,14 1,41.
- Das alte Paar `paar_figur.png` gegen `g9/fotos/f9b_g0.png` (mit `give all` und NPC) taugt fuer das Licht nicht mehr: dort steht der NPC im Spiel anders (siehe 3).

### 9: Figurtexturen ueber ein Shaderskript (Teil 10, jetzt rot gesehen)
In den Spieldaten gesucht (`skinsuch.py`): 21764 Hautzeilen zeigen auf einen Namen, den es nur als Shader gibt. Fall fuer den Test: `md_10thbrother`, `models/players/10thbrother/extras_shiny` (Bild `extras`). Rot mit dem alten Schluessel: "Zwischenspeicher unter Shadername 1, Bildstufe mit Shadername findet 1"; gruen: 0 / 0, unter der Datei 1, alle 5 Flaechen mit Bild.

### 1: cg_turnAnims gegen das Spiel (gefunden, allgemein); Reiten nicht
- **Im Spiel (ohne Fokus, NPC per Skript):** `SET_ANGLES` und danach `SET_DYAW` an einem `npc spawn jedi`, je 30 Bilder mit cg_turnAnims 1 und 0 (`tga8`). Die Huefte unterscheidet sich nicht nur waehrend der Drehung: nach dem SET_ANGLES noch neun Bilder lang (mittlere Abweichung 0,16 bis 0,28), nach dem Ende der SET_DYAW-Drehung bis zum letzten Bild (0,54 bis 0,66), bei unbewegter Gier.
- **Regel:** Der Vergleich `angles[YAW] == cent->pe.legs.yawAngle` (cg_players.cpp:3104 MD) steht hinter CG_PlayerLegsYawFromMovement: die Beine holen den Blick je Clientbild um frameInterpolation des Rests ein (hoechstens 10 Grad, nie mehr als 60 daneben). Die Drehanimation laeuft, solange sich die Beine noch drehen, nicht nur, solange sich die Gier aendert. `figurDrehAnim` rechnet jetzt die Beingier je Clientbild (com_maxfps 125) mit `pmBeinGier`. Rot: Drehanimation 1052-1140 ms bei einer Drehung bis 1100 ms (0 Bilder 64 bis 150 ms danach); gruen: 1060-1252 ms, 10 Bilder.
- **Nicht deckungsgleich gemessen:** Die Dauer im Spiel haengt an den Clientbildern; die Bildfolge im Spiel lief mit 60 Bildern je Sekunde und Haengern beim Schreiben der Bilder. Einzelbilder ohne Haenger (je ein Lauf, `tga9`) trafen die Zeit nicht (der NPC stand noch vor dem SET_ANGLES). Die Laenge des Nachlaufs ist darum nur der Art nach bestaetigt.
- `SET_DYAW` allein drehte den NPC im Spiel bei timescale 0,25 nicht (`tga6`); nach einem SET_ANGLES drehte es ihn zurueck. Nicht weiter verfolgt.
- Reiten: nicht angegangen (Zeit), Fokus nicht gebraucht.

### 3: Klinge des NPC in BOTH_STAND2 (Messwerte, nicht behoben)
- Im Spiel (`npc.cfg`, `tga4`): Der NPC wird von `npc spawn jedi jr` vor dem Spieler gesetzt (442,1 6800,4 24,1, Blick 146) - der NPC_Jedi der Karte g9_figur (460,4 6788,1) erscheint im Spiel nicht. Bei 0,67 s zuendet die Klinge (links unten), ab 1,0 s zeigt sie nach vorn zur Kamera (stark verkuerzt) und bleibt so bis 3,5 s; der NPC bewegt sich nicht zum Wegpunkt.
- Die Klinge des Spielers (von hinten) liegt in Spiel und Vorschau deckungsgleich: Kern y 888-1118 / 892-1121, x 1011-1039 / 1011-1038 (`paar_figur.png`). Die Tagflaechen von saber_1 und saber_2 (MD_Hilts.pk3) sind gleich geformt. Die Abweichung betrifft also die Haltung des NPC, nicht Griff oder Klingenrichtung; Ursache nicht gefunden.

### 4: g9_jtb (Messwerte, nicht behoben)
Im Spiel per `where player`: Ursprung -1250,00 60,00 -1165,89 (Vorschau -1250 60 -1166,01); Kamera per viewpos (-1250 -20 -1113), cg_thirdPersonRange 80, VertOffset 16, cg_fov 80. Hintergrund und naher Boden liegen im Paar auf 1 Bildpunkt deckungsgleich (Kreuzkorrelation 0,98 bis 0,999). Die Figur ist im Spiel gleich breit (Schultern 270 / 275 Bildpunkte), steht aber rund 95 Bildpunkte tiefer: etwa 6 Einheiten. Bodenspur, Ursprung und Kamera sind es nicht; vermutet die Stufe der Hangbeine (PM_FootSlopeTrace waehlt LEGS_LEFTUP1, ein hoeherer Schritt beugt die Knie staerker). Im Spiel nicht pruefbar: cg_debugAnim gibt die Release-Fassung nicht aus (#ifndef FINAL_BUILD).

### 5: Byss-Leiste hinter Glas (nicht behoben)
Das Glas ist Weltgeometrie (134 Flaechen `textures/common/env_glass` in Modell 0), `{ map etest4 blendFunc GL_ONE GL_ONE tcGen environment }` ohne rgbGen (identityLighting), die Vorschau rechnet tcGen environment je Ecke wie RB_CalcEnvironmentTexCoords. Der Unterschied mit Glas ohne Gluehen ist 0 / 6,1 / 15,2 (aus Teil 10). Ursache nicht gefunden.

### 6: Saeulen duel_jeditemple hinten (Messwerte, nicht behoben)
Im Paar `g9/erg/paar_jt9b_b.png`: Saeulen 0,96 bis 0,97, ferne Wand 0,91 0,89 0,87, Fenster 0,88 und 0,53 0,48 0,45, naher Fels 1,00. Die Karte hat keinen Nebel (0 Fogs). Je weiter weg, desto dunkler in der Vorschau: Verdacht auf Mipmaps/Filterung (Spiel r_textureMode GL_LINEAR_MIPMAP_LINEAR, Anisotropie 16 wie die Vorschau, r_simpleMipMaps 1). Die Fenster zeigen in der Vorschau teils schwarze Felder - eigener Fehler, nicht untersucht.

### 7: Decal-Sonderfaelle
Nicht angegangen (Zeit). Offen wie im Kopf von `src/karte/metaflaechen.inc`.

### 8: JK2-Datapad-Foto
Nicht moeglich: steam.exe lief nicht.

### Tests
Selbsttest `teil10b` (neu): 4 OK, 0 FEHLER. Rot je einmal: Schwertlicht-Cvars und Zwischenspeicher in einem Lauf (2 FEHLER, `t10brot`), eigenes Licht je Klinge (1 FEHLER, `t10brot2`), Drehanimation (1 FEHLER, `t10brot4`; die Zeile zaehlt seitdem Bilder 64 bis 150 ms nach dem Ende statt eines Zeitpunkts, der auf ein Bild mit frameInterpolation 0 fiel - dieselbe Messung, rot 0 Bilder). Der Vergleich (`geschuetze`) nimmt `BHED_GV_SCHWERTLICHT` und `BHED_GV_DREHANIM` neu an. ctest einmal am Ende: 196 von 196. lint_sources und lint_fassung ok; lint_settings meldet wie zuvor nur `bildPicmip`, `menueUmbruch`, `rExtMultisample`, `schwierigkeit`. Bau 0 Warnungen des Uebersetzers.

### Nur per Foto gegen das Spiel
Alle Paare in `SP/g10b/erg/` (Movie Duels). OpenJK-Regeln nur gegen die Quelle.

### Offen aus Teil 10b
- Figur im Schwertlicht 0,82 bei 1,2/1,5 (Boden stimmt).
- Licht eines Griffs ohne Klinge (noBlade: CG_DoSaberLight auch fuer die nicht gezeichneten Klingen) und getarnte Schwerter (PW_CLOAKED, CG_DoCloakedSaber) nicht nachgebaut; gemischtes Licht ab drei Klingen ohne noDlight.
- Dauer des Nachlaufs der Drehanimation im Spiel nicht deckungsgleich gemessen; Reiten.
- Klinge des NPC, g9_jtb, Byss-Glas, ferne Flaechen auf duel_jeditemple (und schwarze Fenster), Decal-Sonderfaelle, JK2-Datapad.

## Offen
Aus dem JK2-Vergleich, nicht behoben:
- Datapad: Der Text bricht eine Spur frueher um als im Spiel ("... energy blade / of great" statt "... energy blade of / great"). `CG_DisplayBoxedText` und `RE_Font_StrLenPixels` aus OpenJK sind 1:1 uebernommen und zeigen das Spiel nicht; die Original-exe rechnet demnach anders. Nicht geraten.
- Datapad Force- und Inventar-Seite: Die Vorschau hat im JK2-Format keine Beispiele fuer das Inventar (zeigt EMPTY INVENTORY), die Kraefte haben andere Beispielwerte. Die Eintraege selbst sind darum nicht Stueck fuer Stueck verglichen.
- Datapad: Die rechte Anzeige (Munition und Kraft) sieht in der Vorschau anders aus als im Spiel.
- jk2mp Listen: Die Zeilen stehen im Spiel rund 1,7 virtuelle Punkte hoeher (gemessen; Teil 1 schaetzte 2,7). Ursache nicht gefunden, darum nicht geschoben.
- jk2mp ingame: Join Game und Force Mastery haengen am Spielzustand, sonst gleich.

Weiter offen:
- Modell im MP ohne `model_g2skin`: Spiel dunkel (jamp, MD MP), Vorschau zeigt die Standardhaut. Pruefung nennt es.
- Transition und Fade im Spiel nicht pruefbar (ohne Fokus laeuft kein Tastendruck); Code portiert in `skript.cpp`.
- MP-Licht OpenJK (182) nur gegen das Original verglichen, nicht gegen eine OpenJK-exe.
- Tutorial bei 200 % unter 4K: siehe Abschnitt Tutorials.
- Liste "Zuletzt bearbeitet" fuer Menue-Projekte: gibt es nicht, waere ein eigener Wunsch.
- Der volle Lauf `menueeditor` und der Gesamtlauf aller Modi stehen aus: Endrunde von rc608.

## English
- Menu editor: new start screen in the style of the Mission tab, with the format choice explained.
- Menu editor: a first click only selects an element; a new press on it drags it, so a click no longer shifts anything. Alt+click goes through overlapping elements, Alt+drag moves the selected one even when covered.
- Menu editor: undo and redo work everywhere (Ctrl+Z, Ctrl+Y and the arrows) and also restore the selection. Undoing an insert no longer crashes.
- Menu editor: values changed in Properties, Events and Layout stay when you click elsewhere. Dragging rect in Properties works.
- Menu editor: an element keeps its type. A wrong line in the text no longer throws all elements to 0,0; the editor says which line would make the game drop the menu.
- Menu editor: rename in the hierarchy ends when you click elsewhere and renames the right element.
- Menu editor: all Jedi Outcast multiplayer menus compared with the running game: texts, values, saber picture, map list, force templates and setup pages now match.
- Command line: unknown switches are no longer opened as files; --version names the version.
- Menu editor: images and shaders are found as in the game: base is always read along with a mod folder, also when only the mod folder is set up (force icons no longer empty with Movie Duels)
- Menu editor: Ctrl+F shows find and replace together, as in the script section; Replace All is one undo step and reports the count
- Menu editor: the mouse pointer shows what a press does: resize arrows on the handles and edges of the selected element (edges now resize too), move pointer on it, text cursor over the text editor; handles at the screen border stay reachable
- Menu editor: Ctrl+mouse wheel zooms the canvas around the pointer (25 to 800 %), Ctrl+0 fits, Ctrl+1 is 100 %, middle button or Space+drag moves the view
- Menu editor: double-click an element on the canvas to write its text in place, with the game's font, size, colour and wrapping; Esc discards, Enter or a click elsewhere keeps it as one undo step; texts from the game stay untouched and the element gets its own text
- Menu editor: the hierarchy shows only the open tab (a "Whole file" switch shows every menu of the file); the title bar names the open menu file
- Menu editor: every single element can be dragged onto the canvas and lands where you drop it, with a frame while dragging and snapping; a plain click no longer inserts anything, a double-click inserts without stacking; the two lists are now "Single elements" and "Ready-made parts", with a tooltip on what each entry contains
- Menu editor: a new Model element stands fully visible, from the front, in a standing pose, with values taken from the game's own menus; the turn, distance, height and field-of-view handles are large buttons with clear symbols and never cover the resize handles; the preview shows the direction and the T-pose exactly as the game does
- Menu editor: Properties shows only the fields the game reads for this kind of element; entries that do not apply stay in the file and appear under "More entries in the file"; a model has its own group with model, skin, animation from a searchable list, facing, height, distance and field of view
- Menu editor: the side panels have a minimum width at which every number stays readable; rect and colours show labelled fields that wrap instead of being cut off
- Menu editor: the right panel starts wider, the text editor wraps long lines by default (Alt+Z switches, the file does not change) and has a real horizontal scrollbar
- Menu editor: Events redesigned: every action is a compact block with its up and delete buttons always in the same place; transition shows labelled From and To groups, time and step; everything stays inside the panel at any width
- Menu editor: An empty menu looks as in the game: black in single player, the grey default box in multiplayer; a switch for full screen or overlay explains the difference; a checkerboard marks what shows through, only as a display
- Changelog window: Every version is divided by area with headings you can fold; points without an area stand under General; the release notes use the same headings
- Menu editor: Engine messages says what it shows and "No messages" when the list is empty; long messages wrap and a click on them jumps to the line
- Menu editor: new right panel with Properties, Events and Text editor: a card with name, type and one row of buttons, field search, groups with a summary, numbers you can type or drag by their label, a help line; changed values are marked and can be reset; recently used colours
- Menu editor: Events in plain words: triggers and steps, Add step and Try; renaming an element carries its targets along; a missing target turns yellow and the check names it
- Menu editor: new Image and Video elements start as an empty frame; a double-click chooses the picture or video
- Menu editor: left panel with tiles and names; the hierarchy stays on the left with search, eye, lock, Ctrl and Shift; part search; My templates with preview, rename and delete, which you can take back
- Menu editor: Ctrl+mouse wheel zooms the text editor, Ctrl+0 resets it; the canvas always keeps the shape of the game window
- Menu editor: text alignment stays when an element is resized, also vertically, and now follows the font measures of the game, so the large fonts sit in the middle as in the game
- Menu editor: Movie Duels multiplayer shows the fonts of its own menus; new elements there use the matching font. The figure in multiplayer has the game's light and is no longer too bright
- Menu editor: a new, pasted or duplicated element no longer lands exactly on one of the same size, and frames lying over each other show their hint once; quickly grabbing a tile a second time no longer inserts it twice
- Tutorials: the window keeps clear of open lists, of the panel the step points to and of the elements on the canvas; at 200 % it takes less room
- Menu editor: preview: single-player text that wraps by itself breaks and ends with "..." as in the game
- Menu editor: drag a frame on the canvas to select several elements; Ctrl+A selects all, Ctrl+X cuts
- Menu editor: right-click an element or a hierarchy row for cut, copy, paste, duplicate, delete, front, back, lock, hide and save as template
- Menu editor: a decoration that fills the screen lets clicks through, so a background can no longer be moved by accident
- Menu editor: Save As and Ctrl+S; a line above the canvas appears when another program changed the open file
- Menu editor: right-click a tab to rename, duplicate, delete or move its menu
- Menu editor: a new menu file enters the game's menu list in the package by itself; Test in game opens the menu of the open page at once; the check says when nothing opens a menu
- Menu editor: the first own menu tutorial now ends with a working menu in the game
- Menu editor: limits measured in the games: Jedi Academy 150 elements in single player, 256 in multiplayer (OpenJK 512), Jedi Outcast 128 and 256, Movie Duels 4096
- Menu editor: Properties: conditions show, hide, enable, disable; the choices of a multi choice; list and game-drawn fields; text under the pointer (focusColor); new menus have it, the check names it when it is missing
- Menu editor: nothing typed into a field can destroy a menu any more; what the game could not read is refused with a message; umlauts are written in the encoding of the game; a path with umlauts no longer crashes
- Menu editor: preview as in the game: two videos at once hold each other, border thickness follows the game resolution, a menu with its own fill covers the grey box, Jedi Outcast single player ignores backcolor without an image, CLOSED in the Jedi Outcast server menu is as dark as in the game
- Menu editor: with little room the left panel folds by itself; arrows above the canvas fold both panels; view switches move behind one button
- Menu editor: schemas of new menus and parts use English names (back, title, background, my_menu) in every language
- Manual: images up to 2560 wide, animated images 1920; the pages show them much larger, a click shows them in full size
- Preview: sentry guns, turrets and (under Movie Duels) misc_turretG2 now find figures, aim, fire and do damage by the rules of the chosen game: figures die, turrets, breakables and laser trip mines can be destroyed, and their targets fire. All Ghoul2 items stand the right way round (they were a quarter turn off).
- Preview: from the same check: ZOOM during a camera accel, func_breakable timing, particle traces, floor sabers, thrown breakables, falling vehicles, trigger_hurt and fx_target_beam damage like in the game.
- Preview: light as in the game. Map and characters were far too bright: defaults are now brightness 1.0x and ambient 0, light grid, minimum light per rule set and dynamic lights follow the engine, under Movie Duels also its gamma step.
- Preview: the player has the game's character and saber (model and three skins from the defaults of OpenJK and Movie Duels).
- Preview: corpses slide and fall on, characters fall to their death, corpses disappear by the game's rules; under Movie Duels a saber that is put away hangs in its holster.
- Preview: what lies behind a closed door is not drawn, as in the game (area portals); neither are submodels without an entity.
- Preview: reflective floors (tcGen environment) and the glow halo as in the game, textures at full size, gamma table from Movie Duels.
- Preview: figures flinch when hit and blasts push them away, per rule set. Corpses lie along the slope.
- Preview: the player holds rifles as in the game; glow starts on under Movie Duels and off otherwise; saber hilts in the holster by the engine; guns in holsters under Movie Duels (first version); STUN_BATON at the player start.
- Preview: under Movie Duels the map's weather file applies as in the game (embers on duel_mustafar, snow on duel_hoth, rain on duel_kamino); particles fall at the game's speed.
- Preview: guns in holsters hang at the right place and size; corpses lie along the slope even without falling.
- Preview: fog near the eye and light on figures as in the game; no more hairline in the sky; no sky behind closed doors.
- Preview: new setting Texture detail (r_picmip), each step halves the textures and saves video memory.
- Preview: the sky is drawn as in the game: only the rectangle of the sky box around the visible sky surfaces, far behind everything. Holes in test maps now show exactly what the game shows.
- Preview: dynamic glow as in the game (first blur pass without averaging); the halo around lights matches within 1 to 2 of 255.
- Preview: shaders with tcGen on their first stage now use it (glass on t3_byss).
- Preview: bolts stop at their impact and no longer light a figure green from behind; the player stays visible when the camera is in another area.
- Preview: in a scene with a camera track the player figure is hidden over the whole timeline, except where the script itself moves him outside the camera; Show player in the view settings shows him exactly as in the game.
- Preview: .md3 models are lit by the light grid as in the game; the Falcon on duel_deathstar is no longer far too bright.
- Preview: a lightmap stage after two image stages is drawn last as in the game, and stage images are read as images, never as a shader of the same name: floors and pillars on duel_jeditemple show their own texture instead of the reflection.
- Preview: new setting Edge smoothing (r_ext_multisample) as in the game: 8 samples under Movie Duels, off under OpenJK, the original Jedi Academy and Jedi Outcast.
- Preview: NPCs read their entries only from ext_data/npcs, in the game's order; an NPC_Jedi under Movie Duels has its yellow blade and medium stance again.
- Preview: the game's figure settings cg_motionBoneComp, cg_turnAnims and (Movie Duels) g_ActivateAnimationStyle; the head follows a look target gradually; riders sit on their vehicle; under Jedi Outcast kyle is the player.
- Preview: the player's lightsaber comes from g_saber and g_saber_color as in the game (yellow by default) instead of a random colour.
- Preview: the player also puts his feet on slopes like the game (slope leg animations, third person).
- Preview: a lightmap stage with GL_ONE GL_ZERO after an image stage replaces the colour as in the game; the spectators on duel_geonosis_arena, the icicles on Hoth and 83 other alpha-tested shaders are no longer almost black.
- Preview: lightsaber light as in the game: every blade of a saber with one or two blades casts its own light (Movie Duels: twice the blade length plus flicker, colour times 0.66; OpenJK: 1.4 times the blade length); only sabers with three or more blades share one mixed light.
- Preview: new settings Saber light and Saber light reach (Movie Duels cg_saberLightIntensity and cg_saberLightRadius) with the values of a fresh installation; a light stronger than 1 turns darker on floors and walls close to the blade, as in the game.
- Preview: the turn animation (cg_turnAnims) plays as long as the legs still turn towards the view, also after the figure itself stopped turning, as in the game.
- Preview: surfaces outside the camera's PVS are no longer drawn, as in the game; on duel_jeditemple the rooms behind the windows no longer show through the sky.
- Preview: all stages of an opaque shader are drawn before transparent surfaces, as in the game; glass in front of multi-stage walls (duel_jeditemple windows) is no longer darkened.
- Map editor: new Build > Built map as in the game (on by default): after the quick bake, or with a template .bsp, the camera shows the built map through the Mission tab renderer - all shader stages, sky box, glow, fog, PVS and the game's gamma; selection, handles and lines stay on top.
- Map editor: lightmaps and the real-time light are no longer doubled; floors and walls of the built map now match the game (measured 1.00 to 1.02 on twelve maps).
- Map editor: the real-time light follows _noshadersun and _minvertexlight like q3map2 and reads entity keys regardless of case; its tooltip names what the preview cannot show.

## Deutsch
- Menü-Editor: neuer Startbildschirm im Stil des Mission-Tabs, die Formatwahl ist erklärt.
- Menü-Editor: Der erste Klick wählt ein Element nur aus, erst ein neuer Druck zieht es; ein Klick versetzt nichts mehr. Alt+Klick geht durch überlappende Elemente, Alt+Ziehen bewegt das gewählte auch verdeckt.
- Menü-Editor: Rückgängig und Wiederholen wirken überall (Strg+Z, Strg+Y und die Pfeile) und stellen auch die Auswahl wieder her. Rückgängig nach dem Einfügen stürzt nicht mehr ab.
- Menü-Editor: Werte in Properties, Events und Layout bleiben, wenn du daneben klickst. Ziehen an rect in Properties geht.
- Menü-Editor: Ein Element behält seinen Typ. Eine falsche Zeile im Text wirft nicht mehr alle Elemente auf 0,0; der Editor nennt die Zeile, wegen der das Spiel das Menü verwerfen würde.
- Menü-Editor: Umbenennen in der Hierarchie endet beim Klick daneben und trifft das richtige Element.
- Menü-Editor: alle Mehrspieler-Menüs von Jedi Outcast mit dem laufenden Spiel verglichen: Texte, Werte, Säbelbild, Kartenliste, Force-Vorlagen und Setup-Seiten stimmen jetzt.
- Befehlszeile: Unbekannte Schalter werden nicht mehr als Datei geöffnet, --version nennt die Fassung.
- Menü-Editor: Bilder und Shader werden gefunden wie im Spiel: base wird immer mitgelesen, auch wenn nur der Mod-Ordner eingerichtet ist (Macht-Symbole bei Movie Duels nicht mehr leer)
- Menü-Editor: Strg+F zeigt Suchen und Ersetzen zusammen, wie im Skript-Bereich; Alle ersetzen ist ein Undo-Schritt und meldet die Anzahl
- Menü-Editor: Der Mauszeiger zeigt, was ein Druck tut: Skalierpfeile an Griffen und Rändern des gewählten Elements (auch die Ränder skalieren jetzt), Verschiebe-Zeiger darauf, Text-Cursor im Text-Editor; Griffe am Bildschirmrand bleiben erreichbar
- Menü-Editor: Strg+Mausrad zoomt die Fläche um den Zeiger (25 bis 800 %), Strg+0 passend, Strg+1 100 %, Mitteltaste oder Leertaste+Ziehen verschiebt die Ansicht
- Menü-Editor: Doppelklick auf ein Element schreibt seinen Text direkt auf der Fläche, in Schrift, Größe, Farbe und Umbruch des Spiels; Esc verwirft, Enter oder Klick daneben übernimmt als ein Undo-Schritt; Texte des Spiels bleiben unberührt, das Element bekommt einen eigenen Text
- Menü-Editor: Die Hierarchie zeigt nur den offenen Reiter (der Schalter "Ganze Datei" zeigt alle Menüs der Datei); die Titelleiste nennt die offene Menüdatei
- Menü-Editor: Jedes einzelne Element lässt sich auf die Fläche ziehen und landet an der Ablegestelle, mit Rahmen beim Ziehen und Einrasten; ein einfacher Klick fügt nichts mehr ein, Doppelklick fügt ohne Stapel ein; die beiden Listen heißen jetzt "Einzelne Elemente" und "Fertige Bauteile", mit Tooltip zum Inhalt jedes Eintrags
- Menü-Editor: Ein neues Modell steht ganz sichtbar, von vorn, in einer Standpose, mit Werten aus den Menüs des Spiels; die Griffe für Drehen, Abstand, Höhe und Blickwinkel sind große Knöpfe mit klaren Symbolen und verdecken nie die Griffe zum Skalieren; die Vorschau zeigt Richtung und T-Pose genau wie das Spiel
- Menü-Editor: Properties zeigt nur die Felder, die das Spiel bei dieser Art Element liest; Einträge, die nicht passen, bleiben in der Datei und stehen unter "Weitere Einträge in der Datei"; ein Modell hat eine eigene Gruppe mit Modell, Skin, Animation aus einer Liste mit Suche, Blickrichtung, Höhe, Abstand und Blickwinkel
- Menü-Editor: Die Seitenfelder haben eine Mindestbreite, bei der alle Zahlen lesbar bleiben; rect und Farben zeigen beschriftete Felder, die umbrechen statt abgeschnitten zu werden
- Menü-Editor: Das rechte Feld ist ab Werk breiter, der Text-Editor bricht lange Zeilen ab Werk um (Alt+Z schaltet, die Datei ändert sich nicht) und hat einen richtigen waagerechten Rollbalken
- Menü-Editor: Events neu gestaltet: Jede Aktion ist ein kompakter Block, Hoch und Löschen stehen immer an derselben Stelle; transition zeigt beschriftete Gruppen Von und Nach, Zeit und Schritt; alles bleibt bei jeder Breite im Feld
- Menü-Editor: Ein leeres Menü sieht aus wie im Spiel: schwarz im Einzelspieler, der graue Standardkasten im Mehrspieler; ein Schalter für Vollbild oder Überlagerung erklärt den Unterschied; ein Schachbrett zeigt, was durchscheint, nur als Anzeige
- Changelog-Fenster: Jede Fassung ist nach Bereichen gegliedert, mit Überschriften zum Zuklappen; Punkte ohne Bereich stehen unter Allgemein; die Release-Notizen haben dieselben Überschriften
- Menü-Editor: Engine messages sagt, was es zeigt, und "Keine Meldungen" bei leerer Liste; lange Meldungen brechen um, ein Klick springt zur Zeile
- Menü-Editor: neue rechte Leiste mit Properties, Events und Text editor: Karte mit Name, Typ und einer Knopfreihe, Feldsuche, Gruppen mit Zusammenfassung, Zahlen tippen oder an der Beschriftung ziehen, Hilfezeile; geänderte Werte sind markiert und lassen sich zurücksetzen; zuletzt benutzte Farben
- Menü-Editor: Events in Worten: Auslöser und Schritte, Add step und Try; Umbenennen zieht die Ziele mit; ein fehlendes Ziel wird gelb, und die Prüfung nennt es
- Menü-Editor: Neue Bild- und Video-Elemente beginnen als leerer Rahmen; ein Doppelklick wählt Bild oder Video
- Menü-Editor: linke Leiste mit Kacheln und Namen; die Hierarchie steht links, mit Suche, Auge, Schloss, Strg und Umschalt; Teile-Suche; My templates mit Vorschau, Umbenennen und Löschen, das sich zurücknehmen lässt
- Menü-Editor: Strg+Mausrad zoomt den Text-Editor, Strg+0 setzt zurück; die Fläche behält immer die Form des Spielfensters
- Menü-Editor: Die Textausrichtung bleibt beim Skalieren, auch senkrecht, und folgt jetzt den Schriftmaßen des Spiels, die großen Schriften sitzen wie dort in der Mitte
- Menü-Editor: Movie Duels Multiplayer zeigt die Schriften seiner eigenen Menüs, neue Elemente nehmen dort die passende Schrift. Die Figur hat im Multiplayer das Licht des Spiels und ist nicht mehr zu hell
- Menü-Editor: Ein neues, eingefügtes oder dupliziertes Element landet nicht mehr genau auf einem gleich großen, übereinanderliegende Rahmen zeigen ihren Hinweis nur einmal; schnelles zweites Zugreifen auf eine Kachel setzt nicht mehr zweimal ein
- Tutorials: Das Fenster hält offene Listen, die Spalte des Schritts und die Elemente auf der Fläche frei; bei 200 % braucht es weniger Platz
- Menü-Editor: Vorschau: Selbst umbrechender Text im Einzelspieler bricht um und endet mit "..." wie im Spiel
- Menü-Editor: Auf der Fläche einen Rahmen aufziehen wählt mehrere Elemente; Strg+A wählt alle, Strg+X schneidet aus
- Menü-Editor: Rechtsklick auf ein Element oder eine Hierarchiezeile: Ausschneiden, Kopieren, Einfügen, Duplizieren, Löschen, ganz vorn, ganz hinten, Sperren, Ausblenden, als Vorlage speichern
- Menü-Editor: Eine Dekoration über den ganzen Schirm lässt Klicks durch, ein Hintergrund lässt sich nicht mehr aus Versehen verschieben
- Menü-Editor: Speichern unter und Strg+S; eine Zeile über der Fläche meldet, wenn ein anderes Programm die offene Datei geändert hat
- Menü-Editor: Rechtsklick auf einen Reiter benennt sein Menü um, dupliziert, löscht oder verschiebt es
- Menü-Editor: Eine neue Menüdatei kommt im Paket von selbst in die Menüliste des Spiels; Im Spiel testen öffnet das Menü der offenen Seite sofort; die Prüfung sagt, wenn nichts ein Menü öffnet
- Menü-Editor: Das Tutorial zum ersten eigenen Menü endet jetzt mit einem funktionierenden Menü im Spiel
- Menü-Editor: Grenzen in den Spielen gemessen: Jedi Academy 150 Elemente im Einzelspieler, 256 im Mehrspieler (OpenJK 512), Jedi Outcast 128 und 256, Movie Duels 4096
- Menü-Editor: Properties: Bedingungen zeigen, verbergen, benutzbar, unbenutzbar; die Auswahl einer Mehrfachwahl; Felder für Listen und vom Spiel Gezeichnetes; Text unter dem Zeiger (focusColor); neue Menüs haben sie, die Prüfung nennt es, wenn sie fehlt
- Menü-Editor: Nichts, was man in ein Feld tippt, zerstört mehr ein Menü; was das Spiel nicht lesen könnte, wird mit einer Meldung abgelehnt; Umlaute stehen in der Kodierung des Spiels; ein Pfad mit Umlauten stürzt nicht mehr ab
- Menü-Editor: Vorschau wie im Spiel: zwei Videos zugleich halten einander an, die Rahmendicke folgt der Auflösung des Spiels, ein Menü mit eigener Füllung deckt den grauen Kasten, Jedi Outcast Einzelspieler ignoriert backcolor ohne Bild, CLOSED im Servermenü von Jedi Outcast ist so dunkel wie im Spiel
- Menü-Editor: Bei wenig Platz klappt die linke Spalte von selbst zu; Pfeile über der Fläche klappen beide Spalten; die Ansicht-Schalter wandern hinter einen Knopf
- Menü-Editor: Neue Menüs und Teile tragen englische Namen (back, title, background, my_menu), in jeder Sprache
- Handbuch: Bilder bis 2560 breit, bewegte Bilder 1920; die Seiten zeigen sie viel größer, ein Klick zeigt die volle Größe
- Vorschau: Wachgeschuetze, Geschuetze und (unter Movie Duels) misc_turretG2 finden jetzt Figuren, zielen, feuern und machen Schaden nach den Regeln des gewaehlten Spiels: Figuren sterben, Geschuetze, Zerbrechliches und Laserfallen lassen sich zerstoeren, und ihre Ziele werden ausgeloest. Alle Ghoul2-Gegenstaende stehen richtig herum (sie waren eine Vierteldrehung verdreht).
- Vorschau: aus derselben Pruefung: ZOOM waehrend einer Kamerabeschleunigung, Zeitablauf von func_breakable, Teilchenspuren, liegende Schwerter, geworfene Zerbrechliche, fallende Fahrzeuge, Schaden von trigger_hurt und fx_target_beam wie im Spiel.
- Vorschau: Licht wie im Spiel. Karte und Figuren waren viel zu hell: Vorgabe jetzt Helligkeit 1,0x und Grundlicht 0, Lichtgitter, Mindestlicht je Regelwerk und dynamische Lichter nach der Engine, unter Movie Duels auch dessen Gamma-Stufe.
- Vorschau: Der Spieler trägt die Figur und das Schwert des Spiels (Modell und drei Häute aus den Vorgaben von OpenJK und Movie Duels).
- Vorschau: Leichen gleiten und fallen weiter, Figuren stürzen in den Tod, Leichen verschwinden nach den Regeln des Spiels; unter Movie Duels hängt das weggesteckte Schwert am Holster.
- Vorschau: Was hinter einer geschlossenen Tür liegt, wird wie im Spiel nicht gezeichnet (Area-Portale), Untermodelle ohne Entity ebenfalls nicht.
- Vorschau: Spiegelnde Böden (tcGen environment) und der Schein des Glühens wie im Spiel, Texturen in voller Größe, Gamma-Tabelle aus Movie Duels.
- Vorschau: Figuren zucken bei Treffern und Explosionen stoßen sie weg, je Regelwerk. Leichen liegen am Hang.
- Vorschau: Der Spieler hält Gewehre wie im Spiel; das Glühen beginnt unter Movie Duels an, sonst aus; Griffe im Holster nach der Engine; Waffen im Holster unter Movie Duels (erste Fassung); STUN_BATON am Spielerstart.
- Vorschau: Unter Movie Duels gilt die Wetterdatei der Karte wie im Spiel (Glut auf duel_mustafar, Schnee auf duel_hoth, Regen auf duel_kamino), Teilchen fallen so schnell wie im Spiel.
- Vorschau: Waffen im Holster hängen an der richtigen Stelle und in der richtigen Größe, Leichen liegen auch ohne Sturz am Hang.
- Vorschau: Nebel nahe am Auge und Licht auf Figuren wie im Spiel, keine Haarlinie mehr im Himmel, kein Himmel hinter geschlossenen Türen.
- Vorschau: Neue Einstellung Bildstufe (r_picmip), jede Stufe halbiert die Bilder und spart Grafikspeicher.
- Vorschau: Der Himmel entsteht wie im Spiel: nur das Rechteck der Himmelsbox um die sichtbaren Himmelsflaechen, ganz hinten. Loecher in Testkarten zeigen genau, was das Spiel zeigt.
- Vorschau: Gluehen wie im Spiel (erster Durchgang ohne Mittelwert), der Schein um Lichter stimmt auf 1 bis 2 von 255.
- Vorschau: Shader mit tcGen in der ersten Stufe beachten es jetzt (Glas auf t3_byss).
- Vorschau: Bolzen bleiben am Einschlag stehen und faerben die Figur nicht mehr von hinten gruen; der Spieler bleibt sichtbar, wenn die Kamera in einem anderen Gebiet steht.
- Vorschau: In einer Szene mit Kameraspur ist die Spielerfigur über die ganze Zeitleiste verborgen, außer wo das Skript sie außerhalb der Kamera selbst bewegt; Spieler zeigen in den Ansichtseinstellungen zeigt sie genau wie im Spiel.
- Vorschau: .md3-Modelle bekommen das Licht des Lichtgitters wie im Spiel; der Falke auf duel_deathstar ist nicht mehr viel zu hell.
- Vorschau: Eine Lightmap-Stufe nach zwei Bildstufen kommt wie im Spiel zuletzt, und Bilder einer Stufe werden als Bild gelesen, nie als gleichnamiger Shader: Böden und Säulen auf duel_jeditemple zeigen ihre eigene Textur statt der Spiegelung.
- Vorschau: neue Einstellung Kantenglättung (r_ext_multisample) wie im Spiel: 8 Proben unter Movie Duels, aus unter OpenJK, dem Original-Jedi-Academy und Jedi Outcast.
- Vorschau: NPCs lesen ihre Einträge nur aus ext_data/npcs, in der Reihenfolge des Spiels; ein NPC_Jedi hat unter Movie Duels wieder seine gelbe Klinge und mittlere Haltung.
- Vorschau: die Figuren-Einstellungen des Spiels cg_motionBoneComp, cg_turnAnims und (Movie Duels) g_ActivateAnimationStyle; der Kopf zieht einem Blickziel nach; Reiter sitzen auf ihrem Fahrzeug; unter Jedi Outcast ist kyle der Spieler.
- Vorschau: Das Lichtschwert des Spielers kommt wie im Spiel aus g_saber und g_saber_color (Vorgabe gelb) statt in einer zufaelligen Farbe.
- Vorschau: Auch der Spieler stellt am Hang die Fuesse wie im Spiel (Hang-Beinanimationen, dritte Person).
- Vorschau: Eine Lightmap-Stufe mit GL_ONE GL_ZERO hinter einer Bildstufe ersetzt die Farbe wie im Spiel; die Zuschauer auf duel_geonosis_arena, die Eiszapfen auf Hoth und 83 weitere Shader mit Alphatest sind nicht mehr fast schwarz.
- Vorschau: Schwertlicht wie im Spiel: Jede Klinge eines Schwerts mit einer oder zwei Klingen wirft ihr eigenes Licht (Movie Duels: doppelte Klingenlänge plus Flackern, Farbe mal 0,66; OpenJK: 1,4-fache Klingenlänge); nur Schwerter mit drei oder mehr Klingen teilen sich ein gemischtes Licht.
- Vorschau: neue Einstellungen Schwertlicht und Reichweite Schwertlicht (Movie Duels cg_saberLightIntensity und cg_saberLightRadius) mit den Werten einer frischen Installation; ein Licht über 1 wird auf Boden und Wänden nahe der Klinge dunkler, wie im Spiel.
- Vorschau: Die Drehanimation (cg_turnAnims) läuft, solange sich die Beine noch zum Blick drehen, auch wenn die Figur selbst schon stillsteht, wie im Spiel.
- Vorschau: Flächen außerhalb des PVS der Kamera werden nicht mehr gezeichnet, wie im Spiel; auf duel_jeditemple scheinen die Räume hinter den Fenstern nicht mehr durch den Himmel.
- Vorschau: alle Stufen eines deckenden Shaders kommen vor den durchscheinenden Flächen, wie im Spiel; Glas vor mehrstufigen Wänden (Fenster auf duel_jeditemple) wird nicht mehr abgedunkelt.
- Karten-Editor: neu Build > Gebaute Karte wie im Spiel (ab Werk an): nach dem Schnell-Backen oder mit einer Vorlage-.bsp zeigt die Kamera die gebaute Karte mit dem Zeichner des Mission-Tabs - alle Shader-Stufen, Himmelsbox, Glühen, Nebel, PVS und das Gamma des Spiels; Auswahl, Griffe und Linien bleiben darüber.
- Karten-Editor: Lightmaps und Echtzeit-Licht werden nicht mehr verdoppelt; Böden und Wände der gebauten Karte stimmen jetzt mit dem Spiel überein (gemessen 1,00 bis 1,02 auf zwölf Karten).
- Karten-Editor: das Echtzeit-Licht beachtet _noshadersun und _minvertexlight wie q3map2 und liest Entity-Schlüssel unabhängig von Groß- und Kleinschreibung; sein Tooltip nennt, was die Vorschau nicht zeigen kann.

## 中文
- 菜单编辑器:新的起始界面,风格与任务标签页一致,并说明了格式选择。
- 菜单编辑器:第一次单击只选中元素,再次按住才会拖动,单击不再造成移动。Alt+单击可在重叠元素间切换,Alt+拖动可移动被遮挡的已选元素。
- 菜单编辑器:撤销和重做随处可用(Ctrl+Z、Ctrl+Y 和箭头),并会恢复选择。插入后撤销不再崩溃。
- 菜单编辑器:在属性、事件和布局中更改的值在点击别处后会保留。可以在属性中拖动 rect。
- 菜单编辑器:元素保持其类型。文本中的错误行不再把所有元素移到 0,0;编辑器会指出导致游戏丢弃菜单的那一行。
- 菜单编辑器:层级中的重命名在点击别处时结束,并作用于正确的元素。
- 菜单编辑器:所有 Jedi Outcast 多人菜单已与运行中的游戏对比:文本、数值、光剑图片、地图列表、原力模板和设置页现在一致。
- 命令行:未知开关不再被当作文件打开,--version 显示版本。
- 菜单编辑器:图片和着色器的查找方式与游戏相同:即使只设置了模组文件夹,也始终同时读取 base(Movie Duels 的原力图标不再为空)
- 菜单编辑器:Ctrl+F 同时显示查找和替换,与脚本区域相同;“全部替换”是一个撤销步骤并报告数量
- 菜单编辑器:鼠标指针显示按下后的操作:所选元素的手柄和边缘上显示缩放箭头(边缘现在也可缩放),元素上显示移动指针,文本编辑器上显示文本光标;屏幕边缘的手柄仍可点中
- 菜单编辑器:Ctrl+鼠标滚轮围绕指针缩放画布(25% 到 800%),Ctrl+0 适应,Ctrl+1 为 100%,按住中键或空格键拖动可移动视图
- 菜单编辑器:双击画布上的元素可就地编写其文本,使用游戏的字体、大小、颜色和换行;Esc 放弃,Enter 或单击别处保留(一个撤销步骤);游戏文本保持不变,元素获得自己的文本
- 菜单编辑器:层级只显示打开的选项卡(“整个文件”开关显示文件中的所有菜单);标题栏显示打开的菜单文件
- 菜单编辑器:每个单个元素都可拖到画布上并落在放置位置,拖动时显示框架并吸附;普通单击不再插入,双击插入且不堆叠;两个列表现在称为“单个元素”和“现成组件”,每个条目都有说明内容的提示
- 菜单编辑器:新模型完整可见、正面朝前、站立姿势,数值取自游戏自身的菜单;旋转、距离、高度和视野手柄是带清晰符号的大按钮,不会遮挡缩放手柄;预览显示的方向和 T 姿势与游戏完全相同
- 菜单编辑器:Properties 只显示游戏对此类元素读取的字段;不适用的条目保留在文件中,并显示在“文件中的其他条目”下;模型有自己的分组:模型、皮肤、可搜索列表中的动画、朝向、高度、距离和视野
- 菜单编辑器:侧边面板有最小宽度,所有数字都保持可读;rect 和颜色显示带标签的字段,会换行而不是被截断
- 菜单编辑器:右侧面板默认更宽,文本编辑器默认自动换行(Alt+Z 切换,文件不变),并有真正的水平滚动条
- 菜单编辑器:重新设计 Events:每个动作都是紧凑的块,上移和删除按钮始终在同一位置;transition 显示带标签的“从”“到”分组、时间和步长;任何宽度下所有内容都留在面板内
- 菜单编辑器:空菜单与游戏中一致:单人游戏为黑色,多人游戏为灰色默认方框;全屏或叠加开关说明区别;棋盘格标出透过的部分,仅用于显示
- 更新日志窗口:每个版本按区域划分,标题可折叠;没有区域的条目位于“常规”下;发布说明使用相同的标题
- 菜单编辑器:Engine messages 说明其显示内容,列表为空时显示“没有消息”;长消息会换行,单击可跳到该行
- 菜单编辑器:新的右侧面板,包含 Properties、Events 和 Text editor:卡片显示名称、类型和一排按钮,字段搜索,带摘要的分组,数字可输入或拖动标签修改,帮助行;已更改的值有标记并可重置;最近使用的颜色
- 菜单编辑器:用文字描述 Events:触发器和步骤,Add step 和 Try;重命名元素时目标随之更新;缺失的目标显示为黄色,检查会指出
- 菜单编辑器:新的图像和视频元素先是空框;双击选择图像或视频
- 菜单编辑器:左侧面板使用带名称的图块;层级固定在左侧,带搜索、眼睛、锁、Ctrl 和 Shift;部件搜索;My templates 带预览、重命名和可撤销的删除
- 菜单编辑器:Ctrl+鼠标滚轮缩放文本编辑器,Ctrl+0 复位;画布始终保持游戏窗口的形状
- 菜单编辑器:缩放元素时文字对齐保持不变,垂直方向也一样,并按游戏的字体度量计算,大字体与游戏中一样居中
- 菜单编辑器:Movie Duels 多人模式显示其自身菜单的字体,新元素使用相应字体。多人模式中的人物使用游戏的光照,不再过亮
- 菜单编辑器:新建、粘贴或复制的元素不再正好落在同样大小的元素上,重叠的框只显示一次提示;快速再次抓取图块不再插入两次
- 教程:窗口会避开打开的列表、步骤所指的面板和画布上的元素;在 200% 时占用更少空间
- 菜单编辑器:预览:单人模式中自动换行的文字与游戏一样换行并以 "..." 结束
- 菜单编辑器:在画布上拖出框可选择多个元素;Ctrl+A 全选,Ctrl+X 剪切
- 菜单编辑器:右键单击元素或层级行:剪切、复制、粘贴、复制副本、删除、置于顶层、置于底层、锁定、隐藏、保存为模板
- 菜单编辑器:填满屏幕的装饰让单击穿过,背景不再会被意外移动
- 菜单编辑器:另存为和 Ctrl+S;其他程序更改打开的文件时,画布上方会显示一行提示
- 菜单编辑器:右键单击标签可重命名、复制、删除或移动其菜单
- 菜单编辑器:新菜单文件自动进入包内游戏的菜单列表;“在游戏中测试”会立即打开当前页面的菜单;没有内容打开菜单时检查会提示
- 菜单编辑器:第一个自制菜单教程现在以游戏中可用的菜单结束
- 菜单编辑器:在游戏中测量的限制:Jedi Academy 单人 150 个元素,多人 256 个 (OpenJK 512),Jedi Outcast 128 和 256,Movie Duels 4096
- 菜单编辑器:Properties:条件显示、隐藏、启用、禁用;多选的选项;列表和由游戏绘制的字段;指针下的文字 (focusColor);新菜单已包含,缺失时检查会指出
- 菜单编辑器:在字段中输入的内容不再破坏菜单;游戏无法读取的内容会被拒绝并提示;变音符号按游戏编码写入;带变音符号的路径不再崩溃
- 菜单编辑器:预览与游戏一致:同时两个视频互相卡住,边框粗细随游戏分辨率变化,自带填充的菜单覆盖灰色方框,Jedi Outcast 单人模式无图时忽略 backcolor,Jedi Outcast 服务器菜单中的 CLOSED 与游戏一样暗
- 菜单编辑器:空间不足时左侧面板自动折叠;画布上方的箭头可折叠两侧面板;视图开关移到一个按钮后面
- 菜单编辑器:新菜单和部件在所有语言中都使用英文名称 (back、title、background、my_menu)
- 手册:图片宽度最大 2560,动画图片 1920;页面上显示得更大,单击可查看完整尺寸
- 预览:哨戒炮、炮塔以及(Movie Duels 下的)misc_turretG2 现在会按所选游戏的规则寻找角色、瞄准、开火并造成伤害:角色会死亡,炮塔、可破坏物体和激光绊雷可被摧毁,并触发它们的目标。所有 Ghoul2 物品的朝向都已修正(之前偏了四分之一圈)。
- 预览:同一轮检查还修正了:摄像机加速期间的 ZOOM、func_breakable 的时序、粒子轨迹、地上的光剑、被抛出的可破坏物体、下落的载具,以及 trigger_hurt 和 fx_target_beam 的伤害,均与游戏一致。
- 预览:光照与游戏一致。地图和角色此前过亮:默认亮度现为 1.0x、环境光为 0,光照网格、各规则集的最低光照和动态光按引擎计算,在 Movie Duels 下还包括其伽马步骤。
- 预览:玩家使用游戏中的角色和光剑(模型和三个皮肤来自 OpenJK 与 Movie Duels 的默认值)。
- 预览:尸体会继续滑动和坠落,角色会坠落而死,尸体按游戏规则消失;在 Movie Duels 下收起的光剑挂在剑套上。
- 预览:与游戏一样不绘制关闭的门后面的内容(区域门户),也不绘制没有实体的子模型。
- 预览:反光地面(tcGen environment)和辉光晕与游戏一致,纹理使用完整尺寸,伽马表来自 Movie Duels。
- 预览:角色被击中时会抽搐,爆炸会把他们推开,按规则集。尸体顺着斜坡躺下。
- 预览:玩家持枪姿势与游戏一致;辉光在 Movie Duels 下默认开启,其他默认关闭;剑套中的剑柄按引擎放置;Movie Duels 下枪挂在身上(第一版);玩家起点的 STUN_BATON。
- 预览:在 Movie Duels 下,地图的天气文件与游戏中一样生效(duel_mustafar 的余烬、duel_hoth 的雪、duel_kamino 的雨),粒子以游戏中的速度下落。
- 预览:枪套中的武器位置和大小正确;尸体即使没有坠落也顺着斜坡躺下。
- 预览:靠近视点的雾和角色上的光照与游戏一致;天空中不再有细线;关闭的门后没有天空。
- 预览:新设置“纹理细节 (r_picmip)”,每级将纹理减半并节省显存。
- 预览:天空的绘制方式与游戏相同:只在最后方绘制可见天空面周围的天空盒矩形。测试地图上的洞现在显示的内容与游戏完全一致。
- 预览:动态辉光与游戏相同(第一遍不取平均),灯光周围的光晕误差在 255 中的 1 到 2 以内。
- 预览:第一层带 tcGen 的着色器现在会应用它(t3_byss 上的玻璃)。
- 预览:弹体停在命中点,不再从背后把人物照绿;摄像机位于其他区域时玩家仍然可见。
- 预览:在带有摄像机轨道的场景中,玩家角色在整个时间轴上隐藏,除非脚本在摄像机之外亲自移动他;视图设置中的“显示玩家”会完全按游戏方式显示他。
- 预览:.md3 模型像游戏中一样受光照网格照明;duel_deathstar 上的猎鹰号不再过亮。
- 预览:位于两个图像阶段之后的光照贴图阶段像游戏中一样最后绘制,阶段的图像按图像读取,绝不当作同名着色器:duel_jeditemple 的地板和柱子显示自己的纹理而不是反射。
- 预览:新设置“边缘平滑”(r_ext_multisample),与游戏相同:Movie Duels 下 8 个采样,OpenJK、原版 Jedi Academy 和 Jedi Outcast 下关闭。
- 预览:NPC 只从 ext_data/npcs 读取条目,顺序与游戏相同;Movie Duels 下的 NPC_Jedi 重新拥有黄色光剑和中等架势。
- 预览:游戏的角色设置 cg_motionBoneComp、cg_turnAnims 以及(Movie Duels)g_ActivateAnimationStyle;头部逐渐跟随注视目标;骑手坐在载具上;Jedi Outcast 下 kyle 是玩家。
- 预览:玩家的光剑与游戏一样取自 g_saber 和 g_saber_color(默认黄色),不再是随机颜色。
- 预览:玩家在斜坡上也像游戏中一样放脚(斜坡腿部动画,第三人称)。
- 预览:位于图像阶段之后、使用 GL_ONE GL_ZERO 的光照贴图阶段与游戏一样替换颜色;duel_geonosis_arena 的观众、Hoth 的冰柱以及另外 83 个带 Alpha 测试的着色器不再几乎全黑。
- 预览:光剑光照与游戏相同:一把或两把剑刃的光剑每个剑刃都投射自己的光(Movie Duels:剑刃长度的两倍加闪烁,颜色乘以 0.66;OpenJK:剑刃长度的 1.4 倍);只有三把或更多剑刃的光剑共用一个混合光。
- 预览:新增设置“光剑光照”和“光剑光照范围”(Movie Duels 的 cg_saberLightIntensity 和 cg_saberLightRadius),初始为全新安装的值;大于 1 的光在靠近剑刃的地面和墙壁上会变暗,与游戏相同。
- 预览:转身动画(cg_turnAnims)在双腿仍朝视线方向转动时一直播放,即使角色本身已停止转动,与游戏相同。
- 预览:与游戏一致,不再绘制摄像机 PVS 之外的表面;在 duel_jeditemple 上,窗户后面的房间不再透过天空显示出来。
- 预览:与游戏一致,不透明着色器的所有阶段都在透明表面之前绘制;多阶段墙面前的玻璃(duel_jeditemple 的窗户)不再变暗。
- 地图编辑器:新增 Build > 已构建的地图如游戏中显示(默认开启):快速烘焙之后或使用模板 .bsp 时,摄像机用任务选项卡的渲染器显示已构建的地图——所有着色器阶段、天空盒、辉光、雾、PVS 和游戏伽马;选择、手柄和线条保持在上方。
- 地图编辑器:光照贴图和实时光照不再加倍;已构建地图的地面和墙壁现在与游戏一致(在十二张地图上测得 1.00 到 1.02)。
- 地图编辑器:实时光照像 q3map2 一样遵循 _noshadersun 和 _minvertexlight,并且不区分大小写地读取实体键;其提示说明预览无法显示的内容。

## 日本語
- メニューエディター: ミッションタブと同じスタイルの新しいスタート画面。形式の選択に説明を付けました。
- メニューエディター: 最初のクリックは選択だけで、もう一度押すとドラッグします。クリックで動くことはなくなりました。Alt+クリックで重なった要素を順に選び、Alt+ドラッグで隠れた選択済み要素も動かせます。
- メニューエディター: 元に戻す・やり直しがどこでも働き(Ctrl+Z、Ctrl+Y、矢印)、選択も戻します。挿入後に元に戻しても落ちなくなりました。
- メニューエディター: プロパティ、イベント、レイアウトで変えた値は、別の場所をクリックしても残ります。プロパティの rect のドラッグも動きます。
- メニューエディター: 要素の型は変わりません。テキストの誤った行で全要素が 0,0 に飛ぶことはなくなり、ゲームがメニューを破棄する原因の行を示します。
- メニューエディター: 階層での名前変更は別の場所のクリックで終わり、正しい要素に効きます。
- メニューエディター: Jedi Outcast のマルチプレイメニューをすべて実際のゲームと比較し、テキスト、値、セイバー画像、マップ一覧、フォーステンプレート、設定ページを一致させました。
- コマンドライン: 不明なスイッチをファイルとして開かなくなり、--version でバージョンを表示します。
- メニューエディター: 画像とシェーダーをゲームと同じように探します。MOD フォルダーだけを設定していても base を常に一緒に読み込みます(Movie Duels のフォースアイコンが空になりません)
- メニューエディター: Ctrl+F で検索と置換を一緒に表示します(スクリプト領域と同じ)。すべて置換は 1 回の元に戻す操作で、件数を表示します
- メニューエディター: マウスポインターが押したときの動作を示します。選択した要素のハンドルと縁ではサイズ変更の矢印(縁でもサイズ変更できるようになりました)、要素の上では移動ポインター、テキストエディターではテキストカーソル。画面端のハンドルも届きます
- メニューエディター: Ctrl+マウスホイールでポインターを中心にキャンバスをズーム(25~800%)、Ctrl+0 で全体表示、Ctrl+1 で 100%、中ボタンまたは Space+ドラッグで表示を移動
- メニューエディター: キャンバス上の要素をダブルクリックすると、ゲームのフォント、サイズ、色、折り返しでその場でテキストを書けます。Esc で破棄、Enter または別の場所のクリックで確定(元に戻す 1 回分)。ゲームのテキストはそのままで、要素には独自テキストが付きます
- メニューエディター: 階層には開いているタブのみ表示します(「ファイル全体」スイッチでファイル内のすべてのメニューを表示)。タイトルバーには開いているメニューファイルを表示します
- メニューエディター: 単一の要素はすべてキャンバスへドラッグでき、ドロップした位置に置かれます(ドラッグ中の枠とスナップ付き)。単純なクリックでは挿入されなくなり、ダブルクリックは重ならずに挿入します。2 つのリストは「単一の要素」と「完成部品」になり、各項目に内容のツールチップがあります
- メニューエディター: 新しいモデルは全身が見え、正面を向いた立ちポーズで、値はゲームのメニューから取っています。回転、距離、高さ、視野角のハンドルは分かりやすい記号の大きなボタンで、サイズ変更ハンドルを隠しません。プレビューの向きと T ポーズはゲームと同じです
- メニューエディター: Properties にはゲームがこの種類の要素で読む項目だけを表示します。当てはまらない項目はファイルに残り、「ファイル内のその他の項目」に表示されます。モデルには専用のグループがあり、モデル、スキン、検索可能なリストからのアニメーション、向き、高さ、距離、視野角を設定できます
- メニューエディター: サイドパネルに最小幅があり、すべての数字が読めます。rect と色はラベル付きの欄で表示され、切れずに折り返します
- メニューエディター: 右パネルの初期幅を広げました。テキストエディターは初期設定で長い行を折り返し(Alt+Z で切替、ファイルは変わりません)、横スクロールバーも使えます
- メニューエディター: Events を作り直しました。各アクションはコンパクトなブロックで、上へと削除のボタンは常に同じ位置です。transition はラベル付きの開始・終了グループ、時間、ステップを表示します。どの幅でもすべてパネル内に収まります
- メニューエディター: 空のメニューがゲームと同じ表示になりました。シングルプレイは黒、マルチプレイは灰色の標準ボックスです。全画面とオーバーレイの切り替えが違いを説明し、市松模様が透ける部分を示します(表示のみ)
- 変更履歴ウィンドウ: 各バージョンを分野ごとに分け、見出しは折りたためます。分野のない項目は「全般」にあります。リリースノートも同じ見出しです
- メニューエディター: Engine messages に内容の説明が付き、空のときは「メッセージはありません」と表示します。長いメッセージは折り返し、クリックでその行へ移動します
- メニューエディター:右パネルを刷新(Properties、Events、Text editor)。名前と種類とボタン列のカード、フィールド検索、概要付きグループ、入力またはラベルのドラッグで変更できる数値、ヘルプ行。変更した値には印が付き、元に戻せます。最近使った色も表示します
- メニューエディター:Events を言葉で表示(トリガーとステップ、Add step、Try)。要素の名前を変えると参照先も追従し、見つからない参照先は黄色になり、チェックでも示されます
- メニューエディター:新しい画像要素と動画要素は空の枠から始まり、ダブルクリックで画像や動画を選びます
- メニューエディター:左パネルは名前付きのタイルに。階層は左に常時表示され、検索、目、鍵、Ctrl、Shift が使えます。パーツ検索、プレビュー付きの My templates(名前変更、取り消せる削除)
- メニューエディター:Ctrl+マウスホイールでテキストエディターをズーム、Ctrl+0 で元に戻します。キャンバスは常にゲームウィンドウの形を保ちます
- メニューエディター:要素の拡大縮小でも文字の揃えは縦方向も保たれ、ゲームのフォント寸法に従うので、大きなフォントもゲームと同じく中央に来ます
- メニューエディター:Movie Duels マルチプレイヤーは専用メニューのフォントを表示し、新しい要素も対応するフォントを使います。マルチプレイヤーの人物はゲームと同じ照明になり、明るすぎなくなりました
- メニューエディター:新規・貼り付け・複製した要素が同じ大きさの要素にぴったり重ならなくなり、重なった枠のヒントは 1 回だけ表示されます。タイルを素早くつかみ直しても二重に挿入されません
- チュートリアル: ウィンドウは開いた一覧、手順が指すパネル、キャンバス上の要素を避けます。200% では占める場所が減ります
- メニューエディター: プレビュー: シングルの自動折り返しテキストはゲームと同じく折り返され "..." で終わります
- メニューエディター: キャンバスで枠をドラッグして複数の要素を選択。Ctrl+A ですべて選択、Ctrl+X で切り取り
- メニューエディター: 要素または階層の行を右クリック: 切り取り、コピー、貼り付け、複製、削除、最前面、最背面、ロック、非表示、テンプレートとして保存
- メニューエディター: 画面全体を覆う装飾はクリックを通すので、背景を誤って動かさなくなりました
- メニューエディター: 名前を付けて保存と Ctrl+S。別のプログラムが開いているファイルを変更すると、キャンバスの上に表示します
- メニューエディター: タブを右クリックしてメニューの名前変更、複製、削除、移動
- メニューエディター: 新しいメニューファイルはパッケージ内でゲームのメニューリストに自動で入ります。ゲームでテストは開いているページのメニューをすぐに開きます。どこからも開かれないメニューは検査が知らせます
- メニューエディター: はじめての自作メニューのチュートリアルは、ゲームで動くメニューで終わるようになりました
- メニューエディター: ゲームで測定した制限: Jedi Academy はシングル 150 要素、マルチ 256 (OpenJK 512)、Jedi Outcast は 128 と 256、Movie Duels は 4096
- メニューエディター: Properties: 条件の表示、非表示、使用可、使用不可。複数選択の選択肢。リストとゲームが描く要素の欄。ポインターの下のテキスト (focusColor)。新しいメニューには入っており、ないと検査が知らせます
- メニューエディター: 欄への入力でメニューが壊れなくなりました。ゲームが読めないものはメッセージとともに拒否します。ウムラウトはゲームのエンコードで書き込みます。ウムラウトを含むパスで落ちなくなりました
- メニューエディター: プレビューをゲームと同じに: 同時に 2 つの動画は互いに止まり、枠の太さはゲームの解像度に従い、独自の塗りを持つメニューは灰色のボックスを覆い、Jedi Outcast シングルは画像なしの backcolor を無視し、Jedi Outcast のサーバーメニューの CLOSED はゲームと同じ暗さです
- メニューエディター: スペースが少ないと左パネルが自動で閉じます。キャンバスの上の矢印で両パネルを閉じられます。表示スイッチは 1 つのボタンの中に移ります
- メニューエディター: 新しいメニューとパーツはどの言語でも英語の名前 (back、title、background、my_menu) を使います
- マニュアル: 画像は幅 2560 まで、アニメーション画像は 1920。ページではより大きく表示され、クリックで実寸を表示します
- プレビュー:セントリーガン、タレット、(Movie Duels の)misc_turretG2 が、選んだゲームの規則どおりにキャラクターを探し、狙い、撃ち、ダメージを与えるようになりました。キャラクターは死に、タレット、破壊可能物、レーザートリップマインは破壊でき、そのターゲットが発動します。Ghoul2 のアイテムはすべて正しい向きになりました(4 分の 1 回転ずれていました)。
- プレビュー:同じ検証から、カメラ加速中の ZOOM、func_breakable のタイミング、パーティクルのトレース、床のセーバー、投げられる破壊可能物、落下する乗り物、trigger_hurt と fx_target_beam のダメージがゲームと同じになりました。
- プレビュー: ライティングをゲームと同じに。マップとキャラクターは明るすぎました。既定は明るさ 1.0x、環境光 0 になり、ライトグリッド、ルールセットごとの最低光量、動的ライトはエンジンどおりで、Movie Duels ではそのガンマ処理も行います。
- プレビュー: プレイヤーはゲームのキャラクターとセーバーを使います(モデルと 3 つのスキンは OpenJK と Movie Duels の既定値)。
- プレビュー: 死体は滑り続け落下し、キャラクターは転落死し、死体はゲームのルールで消えます。Movie Duels ではしまったセーバーがホルスターに下がります。
- プレビュー: 閉じたドアの向こうはゲームと同じく描画しません(エリアポータル)。エンティティのないサブモデルも描画しません。
- プレビュー: 反射する床(tcGen environment)とグローのにじみをゲームと同じに。テクスチャは原寸、ガンマ表は Movie Duels のものです。
- プレビュー: キャラクターは被弾でびくっとし、爆発で押しのけられます(ルールセットごと)。死体は斜面に沿って横たわります。
- プレビュー: プレイヤーのライフルの構えをゲームと同じに。グローは Movie Duels では最初からオン、それ以外はオフ。ホルスターのセーバーはエンジンどおり、Movie Duels ではホルスターの武器(初版)、プレイヤー開始時の STUN_BATON。
- プレビュー: Movie Duels ではマップの天候ファイルがゲームと同じように適用されます(duel_mustafar の火の粉、duel_hoth の雪、duel_kamino の雨)。粒子はゲームと同じ速さで落ちます。
- プレビュー: ホルスターの武器が正しい位置と大きさになり、死体は落下しなくても斜面に沿って横たわります。
- プレビュー: 視点近くの霧とキャラクターの光をゲームと同じに。空の細い線がなくなり、閉じたドアの向こうに空が見えなくなりました。
- プレビュー: 新しい設定「テクスチャ詳細 (r_picmip)」。1 段階ごとにテクスチャを半分にし、ビデオメモリを節約します。
- プレビュー: 空はゲームと同じように描かれます。見えている空のサーフェスを囲むスカイボックスの長方形だけを一番奥に描きます。テストマップの穴はゲームとまったく同じに見えます。
- プレビュー: ダイナミックグローがゲームと同じになりました(最初のパスは平均なし)。光の周りのハローは 255 中 1 から 2 以内で一致します。
- プレビュー: 最初のステージに tcGen を持つシェーダーがそれを使うようになりました(t3_byss のガラス)。
- プレビュー: ボルトは着弾点で止まり、キャラクターを背後から緑に照らさなくなりました。カメラが別のエリアにあってもプレイヤーは表示されます。
- プレビュー:カメラトラックのあるシーンでは、スクリプトがカメラの外で自らプレイヤーを動かす部分を除き、タイムライン全体でプレイヤーを表示しません。表示設定の「プレイヤーを表示」でゲームとまったく同じに表示します。
- プレビュー:.md3 モデルがゲームと同じくライトグリッドで照らされます。duel_deathstar のファルコンが明るすぎなくなりました。
- プレビュー:2 つの画像ステージの後のライトマップステージをゲームと同じく最後に描き、ステージの画像は同名のシェーダーではなく画像として読みます。duel_jeditemple の床と柱が反射ではなく自分のテクスチャを表示します。
- プレビュー:新しい設定「エッジの平滑化」(r_ext_multisample)。ゲームと同じく Movie Duels は 8 サンプル、OpenJK、オリジナルの Jedi Academy、Jedi Outcast はオフです。
- プレビュー:NPC は ext_data/npcs からだけ、ゲームと同じ順序で項目を読みます。Movie Duels の NPC_Jedi が再び黄色のブレードと中位の構えになりました。
- プレビュー:ゲームのキャラクター設定 cg_motionBoneComp、cg_turnAnims、(Movie Duels)g_ActivateAnimationStyle。頭が注視先へ徐々に向き、乗り手は乗り物に座り、Jedi Outcast では kyle がプレイヤーです。
- プレビュー:プレイヤーのライトセーバーはゲームと同じく g_saber と g_saber_color(既定は黄色)から決まり、ランダムな色ではなくなりました。
- プレビュー:プレイヤーも坂ではゲームと同じように足を置きます(坂用の脚アニメーション、三人称)。
- プレビュー:画像ステージの後にある GL_ONE GL_ZERO のライトマップステージはゲームと同じく色を置き換えます。duel_geonosis_arena の観客、Hoth のつらら、ほか 83 のアルファテスト付きシェーダーがほぼ真っ黒ではなくなりました。
- プレビュー:ライトセーバーの光をゲームと同じにしました。刃が 1 本または 2 本のセイバーは刃ごとに光を落とします(Movie Duels:刃の長さの 2 倍とちらつき、色は 0.66 倍。OpenJK:刃の長さの 1.4 倍)。刃が 3 本以上のセイバーだけが混ざった光を 1 つ共有します。
- プレビュー:新しい設定「セイバーの光」と「セイバーの光の範囲」(Movie Duels の cg_saberLightIntensity と cg_saberLightRadius)を追加しました。初期値は新規インストールの値です。1 を超える光は、ゲームと同じく刃の近くの床や壁で暗くなります。
- プレビュー:旋回アニメーション(cg_turnAnims)は、キャラクター自身が回り終えた後も、脚が視線の方向へ回っている間は再生されます。ゲームと同じです。
- プレビュー:ゲームと同じく、カメラの PVS の外にある面は描かれなくなりました。duel_jeditemple では窓の奥の部屋が空越しに透けて見えなくなりました。
- プレビュー:ゲームと同じく、不透明シェーダーの全ステージが透明な面より先に描かれます。複数ステージの壁の前のガラス(duel_jeditemple の窓)が暗くなりません。
- マップエディター:新しく Build > ビルドしたマップをゲームと同じ表示に(初期設定でオン):クイックベイクの後、またはテンプレート .bsp があるとき、カメラはミッションタブのレンダラーでビルド済みマップを表示します。シェーダーの全ステージ、スカイボックス、グロー、フォグ、PVS、ゲームのガンマ。選択、ハンドル、線は上に残ります。
- マップエディター:ライトマップとリアルタイムライトが 2 倍にならなくなりました。ビルドしたマップの床と壁がゲームと一致します(12 個のマップで 1.00 から 1.02 を測定)。
- マップエディター:リアルタイムライトは q3map2 と同じく _noshadersun と _minvertexlight に従い、エンティティのキーを大文字小文字に関係なく読みます。ツールチップにプレビューで表示できないものが書かれています。

## Español
- Editor de menús: nueva pantalla de inicio al estilo de la pestaña Misión, con la elección de formato explicada.
- Editor de menús: el primer clic solo selecciona; otra pulsación arrastra, así que un clic ya no mueve nada. Alt+clic recorre los elementos superpuestos y Alt+arrastrar mueve el seleccionado aunque esté tapado.
- Editor de menús: deshacer y rehacer funcionan en todas partes (Ctrl+Z, Ctrl+Y y las flechas) y restauran la selección. Deshacer tras insertar ya no bloquea el programa.
- Editor de menús: los valores de Propiedades, Eventos y Diseño se conservan al hacer clic fuera. Arrastrar rect en Propiedades funciona.
- Editor de menús: un elemento conserva su tipo. Una línea errónea ya no lleva todos los elementos a 0,0; el editor indica la línea por la que el juego descartaría el menú.
- Editor de menús: renombrar en la jerarquía termina al hacer clic fuera y afecta al elemento correcto.
- Editor de menús: todos los menús multijugador de Jedi Outcast comparados con el juego en marcha: textos, valores, imagen del sable, lista de mapas, plantillas de la Fuerza y páginas de configuración ya coinciden.
- Línea de comandos: los interruptores desconocidos ya no se abren como archivos; --version indica la versión.
- Editor de menús: las imágenes y los shaders se encuentran como en el juego: base siempre se lee junto a la carpeta del mod, aunque solo esté configurada esta (los iconos de Fuerza de Movie Duels ya no salen vacíos)
- Editor de menús: Ctrl+F muestra buscar y reemplazar juntos, como en la sección de scripts; Reemplazar todo es un solo paso de deshacer e indica la cantidad
- Editor de menús: el puntero indica qué hará una pulsación: flechas de tamaño en los tiradores y bordes del elemento elegido (ahora los bordes también escalan), puntero de mover sobre él, cursor de texto en el editor de texto; los tiradores en el borde de la pantalla siguen accesibles
- Editor de menús: Ctrl+rueda del ratón acerca el lienzo alrededor del puntero (25 a 800 %), Ctrl+0 ajusta, Ctrl+1 es 100 %, el botón central o Espacio+arrastrar mueve la vista
- Editor de menús: doble clic en un elemento del lienzo para escribir su texto ahí mismo, con la fuente, el tamaño, el color y el ajuste del juego; Esc descarta, Intro o un clic fuera lo conserva como un solo paso de deshacer; los textos del juego no se tocan y el elemento recibe su propio texto
- Editor de menús: la jerarquía muestra solo la pestaña abierta (el interruptor "Archivo completo" muestra todos los menús del archivo); la barra de título nombra el archivo de menú abierto
- Editor de menús: cada elemento suelto se puede arrastrar al lienzo y queda donde lo sueltas, con marco al arrastrar y ajuste; un clic simple ya no inserta nada, el doble clic inserta sin apilar; las dos listas se llaman ahora "Elementos sueltos" y "Piezas listas", con una descripción del contenido de cada entrada
- Editor de menús: un modelo nuevo aparece entero, de frente y en pose de pie, con valores de los propios menús del juego; los tiradores de girar, distancia, altura y campo de visión son botones grandes con símbolos claros y nunca tapan los de tamaño; la vista previa muestra la dirección y la pose en T igual que el juego
- Editor de menús: Properties muestra solo los campos que el juego lee para este tipo de elemento; las entradas que no corresponden permanecen en el archivo y aparecen en "Más entradas en el archivo"; un modelo tiene su propio grupo con modelo, skin, animación de una lista con búsqueda, orientación, altura, distancia y campo de visión
- Editor de menús: los paneles laterales tienen un ancho mínimo en el que todos los números siguen legibles; rect y los colores muestran campos con etiqueta que se ajustan en lugar de cortarse
- Editor de menús: el panel derecho empieza más ancho, el editor de texto ajusta las líneas largas de forma predeterminada (Alt+Z cambia, el archivo no se altera) y tiene una barra de desplazamiento horizontal de verdad
- Editor de menús: Events rediseñado: cada acción es un bloque compacto con los botones de subir y borrar siempre en el mismo sitio; transition muestra grupos Desde y Hasta con etiqueta, tiempo y paso; todo queda dentro del panel a cualquier ancho
- Editor de menús: Un menú vacío se ve como en el juego: negro en un jugador, el cuadro gris predeterminado en multijugador; un interruptor de pantalla completa o superposición explica la diferencia; un tablero marca lo que se transparenta, solo como indicación
- Ventana de cambios: cada versión se divide por áreas con títulos plegables; los puntos sin área están en General; las notas de la versión usan los mismos títulos
- Editor de menús: Engine messages dice qué muestra y "No hay mensajes" si la lista está vacía; los mensajes largos se ajustan y un clic salta a la línea
- Editor de menús: nuevo panel derecho con Properties, Events y Text editor: una tarjeta con nombre, tipo y una fila de botones, búsqueda de campos, grupos con resumen, números que se escriben o se arrastran por la etiqueta, una línea de ayuda; los valores cambiados se marcan y se pueden restablecer; colores usados hace poco
- Editor de menús: Events en palabras: disparadores y pasos, Add step y Try; al renombrar un elemento sus destinos lo siguen; un destino que falta se pone amarillo y la comprobación lo nombra
- Editor de menús: los nuevos elementos de imagen y vídeo empiezan como un marco vacío; un doble clic elige la imagen o el vídeo
- Editor de menús: panel izquierdo con mosaicos y nombres; la jerarquía queda a la izquierda con búsqueda, ojo, candado, Ctrl y Mayús; búsqueda de piezas; My templates con vista previa, renombrar y borrar, que se puede deshacer
- Editor de menús: Ctrl+rueda amplía el editor de texto, Ctrl+0 lo restablece; el lienzo siempre conserva la forma de la ventana del juego
- Editor de menús: la alineación del texto se mantiene al escalar, también en vertical, y sigue las medidas de fuente del juego; las fuentes grandes quedan centradas como allí
- Editor de menús: Movie Duels multijugador muestra las fuentes de sus propios menús y los elementos nuevos usan la adecuada. La figura en multijugador tiene la luz del juego y ya no está demasiado clara
- Editor de menús: un elemento nuevo, pegado o duplicado ya no cae justo sobre otro del mismo tamaño, los marcos superpuestos muestran su aviso una vez; agarrar rápido un mosaico por segunda vez ya no lo inserta dos veces
- Tutoriales: la ventana deja libres las listas abiertas, el panel al que apunta el paso y los elementos del lienzo; al 200 % ocupa menos
- Editor de menús: vista previa: el texto de un jugador con ajuste automático se parte y termina con "..." como en el juego
- Editor de menús: arrastra un marco en el lienzo para seleccionar varios elementos; Ctrl+A selecciona todo, Ctrl+X corta
- Editor de menús: clic derecho en un elemento o una fila de la jerarquía: cortar, copiar, pegar, duplicar, eliminar, al frente, al fondo, bloquear, ocultar y guardar como plantilla
- Editor de menús: una decoración que llena la pantalla deja pasar los clics; un fondo ya no se mueve por accidente
- Editor de menús: Guardar como y Ctrl+S; una línea sobre el lienzo avisa cuando otro programa cambió el archivo abierto
- Editor de menús: clic derecho en una pestaña para renombrar, duplicar, eliminar o mover su menú
- Editor de menús: un archivo de menú nuevo entra solo en la lista de menús del juego dentro del paquete; Probar en el juego abre enseguida el menú de la página abierta; la comprobación avisa si nada abre un menú
- Editor de menús: el tutorial del primer menú propio termina ahora con un menú que funciona en el juego
- Editor de menús: límites medidos en los juegos: Jedi Academy 150 elementos en un jugador, 256 en multijugador (OpenJK 512), Jedi Outcast 128 y 256, Movie Duels 4096
- Editor de menús: Properties: condiciones mostrar, ocultar, activar, desactivar; las opciones de una opción múltiple; campos de listas y de lo que dibuja el juego; texto bajo el puntero (focusColor); los menús nuevos lo tienen y la comprobación avisa si falta
- Editor de menús: lo que se escribe en un campo ya no destruye un menú; lo que el juego no podría leer se rechaza con un mensaje; las diéresis se escriben en la codificación del juego; una ruta con diéresis ya no bloquea el programa
- Editor de menús: vista previa como en el juego: dos vídeos a la vez se detienen entre sí, el grosor del borde sigue la resolución del juego, un menú con relleno propio cubre el cuadro gris, Jedi Outcast un jugador ignora backcolor sin imagen, CLOSED en el menú de servidor de Jedi Outcast es tan oscuro como en el juego
- Editor de menús: con poco espacio el panel izquierdo se pliega solo; las flechas sobre el lienzo pliegan ambos paneles; los interruptores de vista pasan a un botón
- Editor de menús: los menús y piezas nuevos usan nombres en inglés (back, title, background, my_menu) en todos los idiomas
- Manual: imágenes de hasta 2560 de ancho, imágenes animadas de 1920; las páginas las muestran mucho más grandes y un clic las muestra a tamaño completo
- Vista previa: las torretas centinela, las torretas y (con Movie Duels) misc_turretG2 ahora buscan figuras, apuntan, disparan y hacen daño según las reglas del juego elegido: las figuras mueren; las torretas, lo rompible y las minas láser se pueden destruir, y se activan sus objetivos. Todos los objetos Ghoul2 están bien orientados (estaban girados un cuarto de vuelta).
- Vista previa: de la misma revisión: ZOOM durante una aceleración de cámara, tiempos de func_breakable, trazas de partículas, sables en el suelo, rompibles lanzados, vehículos que caen y daño de trigger_hurt y fx_target_beam como en el juego.
- Vista previa: luz como en el juego. El mapa y los personajes eran demasiado claros: ahora por defecto brillo 1,0x y ambiente 0; la rejilla de luz, la luz mínima según el conjunto de reglas y las luces dinámicas siguen al motor, y en Movie Duels también su paso de gamma.
- Vista previa: el jugador lleva el personaje y el sable del juego (modelo y tres pieles según los valores por defecto de OpenJK y Movie Duels).
- Vista previa: los cadáveres siguen deslizándose y cayendo, los personajes caen hasta morir y los cadáveres desaparecen según las reglas del juego; en Movie Duels el sable guardado cuelga de la funda.
- Vista previa: lo que hay detrás de una puerta cerrada no se dibuja, como en el juego (portales de área); tampoco los submodelos sin entidad.
- Vista previa: suelos reflectantes (tcGen environment) y halo del brillo como en el juego, texturas a tamaño completo, tabla de gamma de Movie Duels.
- Vista previa: las figuras se estremecen al recibir impactos y las explosiones las empujan, según el conjunto de reglas. Los cadáveres quedan a lo largo de la pendiente.
- Vista previa: el jugador sostiene los rifles como en el juego; el brillo empieza activado en Movie Duels y desactivado en los demás; empuñaduras en la funda según el motor; armas en la funda en Movie Duels (primera versión); STUN_BATON al inicio del jugador.
- Vista previa: en Movie Duels se aplica el archivo de clima del mapa como en el juego (brasas en duel_mustafar, nieve en duel_hoth, lluvia en duel_kamino); las partículas caen a la velocidad del juego.
- Vista previa: las armas en la funda cuelgan en el lugar y tamaño correctos; los cadáveres quedan en la pendiente aunque no caigan.
- Vista previa: niebla cerca del ojo y luz en las figuras como en el juego; sin línea fina en el cielo; sin cielo tras puertas cerradas.
- Vista previa: nuevo ajuste Detalle de texturas (r_picmip), cada nivel reduce las texturas a la mitad y ahorra memoria de vídeo.
- Vista previa: el cielo se dibuja como en el juego: solo el rectángulo de la caja de cielo alrededor de las superficies de cielo visibles, al fondo. Los agujeros de los mapas de prueba muestran exactamente lo que muestra el juego.
- Vista previa: resplandor dinámico como en el juego (primera pasada sin promedio); el halo de las luces coincide con 1 a 2 de 255.
- Vista previa: los shaders con tcGen en su primera etapa ahora lo usan (cristal en t3_byss).
- Vista previa: los proyectiles se detienen en el impacto y ya no iluminan de verde a la figura por detrás; el jugador sigue visible cuando la cámara está en otra zona.
- Vista previa: en una escena con pista de cámara la figura del jugador queda oculta en toda la línea de tiempo, salvo donde el script la mueve fuera de la cámara; Mostrar jugador en los ajustes de la vista la muestra exactamente como en el juego.
- Vista previa: los modelos .md3 reciben la luz de la rejilla de luz como en el juego; el Halcón de duel_deathstar ya no está demasiado claro.
- Vista previa: una capa de lightmap tras dos capas de imagen se dibuja al final como en el juego, y las imágenes de una capa se leen como imagen, nunca como un shader del mismo nombre: suelos y columnas de duel_jeditemple muestran su propia textura en lugar del reflejo.
- Vista previa: nuevo ajuste Suavizado de bordes (r_ext_multisample) como en el juego: 8 muestras en Movie Duels, desactivado en OpenJK, el Jedi Academy original y Jedi Outcast.
- Vista previa: los NPC leen sus entradas solo de ext_data/npcs, en el orden del juego; un NPC_Jedi en Movie Duels vuelve a tener su hoja amarilla y su postura media.
- Vista previa: los ajustes de figuras del juego cg_motionBoneComp, cg_turnAnims y (Movie Duels) g_ActivateAnimationStyle; la cabeza sigue poco a poco un objetivo de mirada; los jinetes van sentados en su vehículo; en Jedi Outcast kyle es el jugador.
- Vista previa: el sable de luz del jugador sale de g_saber y g_saber_color como en el juego (amarillo por defecto) en lugar de un color aleatorio.
- Vista previa: el jugador también apoya los pies en las pendientes como en el juego (animaciones de piernas en pendiente, tercera persona).
- Vista previa: una etapa de lightmap con GL_ONE GL_ZERO tras una etapa de imagen sustituye el color como en el juego; los espectadores de duel_geonosis_arena, los carámbanos de Hoth y otros 83 shaders con prueba alfa ya no salen casi negros.
- Vista previa: la luz de los sables como en el juego: cada hoja de un sable con una o dos hojas proyecta su propia luz (Movie Duels: el doble de la longitud de la hoja más parpadeo, color por 0,66; OpenJK: 1,4 veces la longitud de la hoja); solo los sables con tres o más hojas comparten una luz mezclada.
- Vista previa: nuevos ajustes Luz del sable y Alcance de la luz del sable (cg_saberLightIntensity y cg_saberLightRadius de Movie Duels) con los valores de una instalación nueva; una luz mayor que 1 se oscurece en suelos y paredes cerca de la hoja, como en el juego.
- Vista previa: la animación de giro (cg_turnAnims) se reproduce mientras las piernas siguen girando hacia la mirada, también cuando la figura ya dejó de girar, como en el juego.
- Vista previa: las superficies fuera del PVS de la cámara ya no se dibujan, como en el juego; en duel_jeditemple las salas detrás de las ventanas ya no se ven a través del cielo.
- Vista previa: todas las etapas de un shader opaco se dibujan antes que las superficies transparentes, como en el juego; el cristal delante de paredes de varias etapas (ventanas de duel_jeditemple) ya no se oscurece.
- Editor de mapas: nuevo Build > Mapa compilado como en el juego (activado por defecto): tras el horneado rápido, o con un .bsp de plantilla, la cámara muestra el mapa compilado con el renderizador de la pestaña Misión: todas las etapas de los shaders, caja de cielo, brillo, niebla, PVS y la gamma del juego; la selección, los tiradores y las líneas quedan encima.
- Editor de mapas: los lightmaps y la luz en tiempo real ya no se duplican; suelos y paredes del mapa compilado coinciden ahora con el juego (medido 1,00 a 1,02 en doce mapas).
- Editor de mapas: la luz en tiempo real respeta _noshadersun y _minvertexlight como q3map2 y lee las claves de entidad sin distinguir mayúsculas; su información indica lo que la vista previa no puede mostrar.

## Čeština
- Editor menu: nová úvodní obrazovka ve stylu karty Mise, volba formátu je vysvětlena.
- Editor menu: první kliknutí prvek jen vybere, teprve nový stisk ho táhne, takže kliknutí už nic neposune. Alt+kliknutí prochází překrývajícími se prvky, Alt+tažení posune vybraný prvek, i když je zakrytý.
- Editor menu: zpět a znovu fungují všude (Ctrl+Z, Ctrl+Y a šipky) a obnovují i výběr. Zpět po vložení už program neshodí.
- Editor menu: hodnoty ve Vlastnostech, Událostech a Rozložení zůstanou po kliknutí jinam. Tažení rect ve Vlastnostech funguje.
- Editor menu: prvek si zachovává svůj typ. Chybný řádek už nepřesune všechny prvky na 0,0; editor uvede řádek, kvůli kterému by hra menu zahodila.
- Editor menu: přejmenování v hierarchii končí kliknutím jinam a týká se správného prvku.
- Editor menu: všechna menu pro více hráčů Jedi Outcast porovnána s běžící hrou: texty, hodnoty, obrázek meče, seznam map, šablony Síly a stránky nastavení nyní souhlasí.
- Příkazový řádek: neznámé přepínače se už neotevírají jako soubory, --version uvede verzi.
- Editor menu: obrázky a shadery se hledají stejně jako ve hře: base se čte vždy, i když je nastavena jen složka modu (ikony Síly v Movie Duels už nejsou prázdné)
- Editor menu: Ctrl+F zobrazí hledání i nahrazení společně, jako v sekci skriptů; Nahradit vše je jeden krok zpět a oznámí počet
- Editor menu: ukazatel myši ukazuje, co stisk udělá: šipky změny velikosti na úchytech a okrajích vybraného prvku (nově mění velikost i okraje), ukazatel přesunu na něm, textový kurzor v textovém editoru; úchyty na okraji obrazovky zůstávají dosažitelné
- Editor menu: Ctrl+kolečko myši přibližuje plochu kolem ukazatele (25 až 800 %), Ctrl+0 přizpůsobí, Ctrl+1 je 100 %, prostřední tlačítko nebo mezerník+tažení posouvá pohled
- Editor menu: dvojklikem na prvek na ploše napíšete jeho text přímo na místě, písmem, velikostí, barvou a zalomením hry; Esc zahodí, Enter nebo klik jinam uloží jako jeden krok zpět; texty hry zůstanou nedotčené a prvek dostane vlastní text
- Editor menu: hierarchie zobrazuje jen otevřenou kartu (přepínač "Celý soubor" zobrazí všechna menu souboru); záhlaví okna uvádí otevřený soubor menu
- Editor menu: každý jednotlivý prvek lze přetáhnout na plochu a zůstane tam, kam ho pustíte, s rámečkem při tažení a přichytáváním; prosté kliknutí už nic nevkládá, dvojklik vkládá bez vrstvení; oba seznamy se nyní jmenují "Jednotlivé prvky" a "Hotové díly", s popiskem obsahu každé položky
- Editor menu: nový model stojí celý viditelný, zepředu, ve stoje, s hodnotami z menu hry; úchyty otočení, vzdálenosti, výšky a zorného pole jsou velká tlačítka s jasnými symboly a nikdy nezakrývají úchyty velikosti; náhled ukazuje směr a pózu T přesně jako hra
- Editor menu: Properties zobrazuje jen pole, která hra u tohoto druhu prvku čte; položky, které neplatí, zůstávají v souboru a jsou v části "Další položky v souboru"; model má vlastní skupinu s modelem, skinem, animací ze seznamu s hledáním, směrem pohledu, výškou, vzdáleností a zorným polem
- Editor menu: postranní panely mají minimální šířku, při které zůstávají všechna čísla čitelná; rect a barvy ukazují popsaná pole, která se zalamují místo oříznutí
- Editor menu: pravý panel je od začátku širší, textový editor ve výchozím stavu zalamuje dlouhé řádky (Alt+Z přepíná, soubor se nemění) a má skutečný vodorovný posuvník
- Editor menu: Events přepracovány: každá akce je kompaktní blok s tlačítky nahoru a smazat vždy na stejném místě; transition ukazuje popsané skupiny Z a Do, čas a krok; vše zůstává v panelu při jakékoli šířce
- Editor menu: Prázdné menu vypadá jako ve hře: černé v jednom hráči, šedý výchozí rámeček ve více hráčích; přepínač celé obrazovky nebo překrytí vysvětluje rozdíl; šachovnice označuje, co prosvítá, jen jako zobrazení
- Okno seznamu změn: Každá verze je rozdělena podle oblastí s nadpisy, které lze sbalit; body bez oblasti jsou pod Obecné; poznámky k vydání mají stejné nadpisy
- Editor menu: Engine messages říká, co ukazuje, a při prázdném seznamu "Žádné zprávy"; dlouhé zprávy se zalamují a kliknutí přejde na řádek
- Editor menu: nový pravý panel s Properties, Events a Text editor: karta s názvem, typem a řadou tlačítek, hledání polí, skupiny se shrnutím, čísla lze psát nebo měnit tažením popisku, řádek nápovědy; změněné hodnoty jsou označené a lze je vrátit; naposledy použité barvy
- Editor menu: Events slovy: spouštěče a kroky, Add step a Try; přejmenování prvku přenese i cíle; chybějící cíl zežloutne a kontrola ho uvede
- Editor menu: nové prvky obrázku a videa začínají jako prázdný rámec; dvojklik vybere obrázek nebo video
- Editor menu: levý panel s dlaždicemi a názvy; hierarchie zůstává vlevo s hledáním, okem, zámkem, Ctrl a Shift; hledání dílů; My templates s náhledem, přejmenováním a mazáním, které lze vrátit
- Editor menu: Ctrl+kolečko přibližuje textový editor, Ctrl+0 ho vrátí; plocha si vždy drží tvar herního okna
- Editor menu: zarovnání textu zůstává při změně velikosti, i svislé, a řídí se mírami písma ze hry; velká písma sedí uprostřed jako ve hře
- Editor menu: Movie Duels multiplayer ukazuje písma svých vlastních menu a nové prvky používají to správné. Postava v multiplayeru má světlo ze hry a už není příliš světlá
- Editor menu: nový, vložený nebo duplikovaný prvek už nepřistane přesně na stejně velkém, překrývající se rámce ukážou nápovědu jednou; rychlé druhé uchopení dlaždice už ji nevloží dvakrát
- Tutoriály: okno nechává volné otevřené seznamy, panel, na který krok ukazuje, a prvky na ploše; při 200 % zabírá méně místa
- Editor menu: náhled: text se samočinným zalamováním pro jednoho hráče se zalamuje a končí "..." jako ve hře
- Editor menu: tažením rámečku na ploše vyberete více prvků; Ctrl+A vybere vše, Ctrl+X vyjme
- Editor menu: pravé tlačítko na prvku nebo řádku hierarchie: vyjmout, kopírovat, vložit, duplikovat, smazat, dopředu, dozadu, zamknout, skrýt a uložit jako šablonu
- Editor menu: ozdoba přes celou obrazovku nechá kliknutí projít, pozadí už omylem neposunete
- Editor menu: Uložit jako a Ctrl+S; řádek nad plochou upozorní, když jiný program změnil otevřený soubor
- Editor menu: pravým tlačítkem na kartě přejmenujete, duplikujete, smažete nebo přesunete její menu
- Editor menu: nový soubor menu se v balíčku sám dostane do seznamu menu hry; Otestovat ve hře hned otevře menu otevřené stránky; kontrola upozorní, když menu nic neotevírá
- Editor menu: tutoriál prvního vlastního menu nyní končí funkčním menu ve hře
- Editor menu: limity změřené ve hrách: Jedi Academy 150 prvků pro jednoho hráče, 256 pro více hráčů (OpenJK 512), Jedi Outcast 128 a 256, Movie Duels 4096
- Editor menu: Properties: podmínky zobrazit, skrýt, povolit, zakázat; možnosti vícenásobné volby; pole seznamů a toho, co kreslí hra; text pod ukazatelem (focusColor); nová menu ji mají a kontrola upozorní, když chybí
- Editor menu: co napíšete do pole, už menu nezničí; co by hra nepřečetla, je odmítnuto se zprávou; přehlásky se zapisují v kódování hry; cesta s přehláskami už program neshodí
- Editor menu: náhled jako ve hře: dvě videa najednou se navzájem zastaví, tloušťka okraje se řídí rozlišením hry, menu s vlastní výplní zakryje šedý rámeček, Jedi Outcast pro jednoho hráče ignoruje backcolor bez obrázku, CLOSED v serverovém menu Jedi Outcast je tmavé jako ve hře
- Editor menu: při nedostatku místa se levý panel sám sbalí; šipky nad plochou sbalí oba panely; přepínače zobrazení se přesunou za jedno tlačítko
- Editor menu: nová menu a díly mají anglické názvy (back, title, background, my_menu) ve všech jazycích
- Příručka: obrázky do šířky 2560, animované 1920; stránky je ukazují mnohem větší, kliknutí v plné velikosti
- Náhled: strážní děla, věže a (v Movie Duels) misc_turretG2 teď hledají postavy, míří, střílejí a zraňují podle pravidel zvolené hry: postavy umírají; věže, rozbitné věci a laserové miny lze zničit a spustí se jejich cíle. Všechny předměty Ghoul2 stojí správně (byly otočené o čtvrt otáčky).
- Náhled: ze stejné kontroly: ZOOM během zrychlení kamery, časování func_breakable, stopy částic, šavle na zemi, házené rozbitné věci, padající vozidla a poškození od trigger_hurt a fx_target_beam jako ve hře.
- Náhled: světlo jako ve hře. Mapa a postavy byly příliš světlé: výchozí je nyní jas 1,0x a okolní světlo 0, světelná mřížka, minimální světlo podle sady pravidel a dynamická světla se řídí enginem, v Movie Duels také jeho krok gamma.
- Náhled: hráč má postavu a meč ze hry (model a tři skiny z výchozích hodnot OpenJK a Movie Duels).
- Náhled: mrtvá těla dál kloužou a padají, postavy se zřítí k smrti a těla mizí podle pravidel hry; v Movie Duels visí odložený meč v pouzdře.
- Náhled: to, co je za zavřenými dveřmi, se jako ve hře nekreslí (portály oblastí); ani podmodely bez entity.
- Náhled: lesklé podlahy (tcGen environment) a záře jako ve hře, textury v plné velikosti, tabulka gamma z Movie Duels.
- Náhled: postavy sebou při zásahu škubnou a výbuchy je odhazují, podle sady pravidel. Mrtvá těla leží podél svahu.
- Náhled: hráč drží pušky jako ve hře; záře začíná v Movie Duels zapnutá, jinak vypnutá; jílce v pouzdře podle enginu; zbraně v pouzdře v Movie Duels (první verze); STUN_BATON na startu hráče.
- Náhled: v Movie Duels platí soubor počasí mapy jako ve hře (žhavé jiskry na duel_mustafar, sníh na duel_hoth, déšť na duel_kamino); částice padají rychlostí jako ve hře.
- Náhled: zbraně v pouzdře visí na správném místě a ve správné velikosti; mrtvá těla leží na svahu i bez pádu.
- Náhled: mlha blízko oka a světlo na postavách jako ve hře; žádná tenká čára na obloze; žádná obloha za zavřenými dveřmi.
- Náhled: nové nastavení Detail textur (r_picmip), každý stupeň zmenší textury na polovinu a šetří grafickou paměť.
- Náhled: obloha se kreslí jako ve hře: jen obdélník skyboxu kolem viditelných ploch oblohy, úplně vzadu. Díry v testovacích mapách ukazují přesně to, co hra.
- Náhled: dynamická záře jako ve hře (první průchod bez průměrování); záře kolem světel souhlasí na 1 až 2 z 255.
- Náhled: shadery s tcGen v první vrstvě jej nyní používají (sklo na t3_byss).
- Náhled: střely se zastaví v místě zásahu a už nebarví postavu zezadu zeleně; hráč zůstává viditelný, i když je kamera v jiné oblasti.
- Náhled: ve scéně se stopou kamery je postava hráče skrytá po celé časové ose, kromě míst, kde ji skript mimo kameru sám pohybuje; Zobrazit hráče v nastavení zobrazení ji ukáže přesně jako ve hře.
- Náhled: modely .md3 dostávají světlo ze světelné mřížky jako ve hře; Falcon na duel_deathstar už není příliš světlý.
- Náhled: vrstva lightmapy po dvou obrazových vrstvách se kreslí jako ve hře poslední a obrázky vrstvy se čtou jako obrázek, nikdy jako stejnojmenný shader: podlahy a sloupy na duel_jeditemple ukazují svou vlastní texturu místo odrazu.
- Náhled: nové nastavení Vyhlazování hran (r_ext_multisample) jako ve hře: 8 vzorků v Movie Duels, vypnuto v OpenJK, původní Jedi Academy a Jedi Outcast.
- Náhled: NPC čtou své položky jen z ext_data/npcs, v pořadí hry; NPC_Jedi v Movie Duels má opět žlutou čepel a střední postoj.
- Náhled: nastavení postav ze hry cg_motionBoneComp, cg_turnAnims a (Movie Duels) g_ActivateAnimationStyle; hlava postupně sleduje cíl pohledu; jezdci sedí na svém vozidle; v Jedi Outcast je hráčem kyle.
- Náhled: světelný meč hráče se jako ve hře řídí g_saber a g_saber_color (výchozí žlutá), už ne náhodnou barvou.
- Náhled: i hráč staví nohy na svahu jako ve hře (animace nohou na svahu, třetí osoba).
- Náhled: stupeň světelné mapy s GL_ONE GL_ZERO za obrazovým stupněm nahrazuje barvu jako ve hře; diváci na duel_geonosis_arena, rampouchy na Hothu a dalších 83 shaderů s alfa testem už nejsou téměř černé.
- Náhled: světlo světelných mečů jako ve hře: každá čepel meče s jednou nebo dvěma čepelemi vrhá vlastní světlo (Movie Duels: dvojnásobek délky čepele plus mihotání, barva krát 0,66; OpenJK: 1,4násobek délky čepele); jen meče se třemi nebo více čepelemi sdílejí jedno smíšené světlo.
- Náhled: nová nastavení Světlo meče a Dosah světla meče (cg_saberLightIntensity a cg_saberLightRadius z Movie Duels) s hodnotami čerstvé instalace; světlo silnější než 1 na podlaze a stěnách u čepele ztmavne, jako ve hře.
- Náhled: animace otáčení (cg_turnAnims) běží, dokud se nohy ještě natáčejí k pohledu, i když se postava sama už přestala otáčet, jako ve hře.
- Náhled: plochy mimo PVS kamery se už nekreslí, jako ve hře; na duel_jeditemple už místnosti za okny neprosvítají oblohou.
- Náhled: všechny fáze neprůhledného shaderu se kreslí před průhlednými plochami, jako ve hře; sklo před vícefázovými stěnami (okna na duel_jeditemple) už neztmavne.
- Editor map: nově Build > Sestavená mapa jako ve hře (ve výchozím stavu zapnuto): po rychlém zapečení nebo se šablonou .bsp ukazuje kamera sestavenou mapu vykreslovačem karty Mise – všechny fáze shaderů, skybox, záři, mlhu, PVS a gamu hry; výběr, úchyty a čáry zůstávají nahoře.
- Editor map: lightmapy a světlo v reálném čase se už nezdvojnásobují; podlahy a stěny sestavené mapy teď odpovídají hře (změřeno 1,00 až 1,02 na dvanácti mapách).
- Editor map: světlo v reálném čase respektuje _noshadersun a _minvertexlight jako q3map2 a čte klíče entit bez ohledu na velikost písmen; jeho nápověda říká, co náhled nedokáže ukázat.
