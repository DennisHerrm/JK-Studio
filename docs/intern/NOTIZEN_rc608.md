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

## Offen
Aus dem JK2-Vergleich, nicht behoben (Kontaktboegen in `SP/rc608/jk2/`):
- Datapad: Die ausgeschnittenen Knopfflaechen (WEAPONS, INVENTORY, FORCE) sind in der Vorschau grau mit Verlauf, im Spiel schwarz. `datapad2.tga` ist dort voll durchsichtig (Alpha 0), `frame_pic` zeichnet schwarz davor; was das Grau zeichnet, ist nicht gefunden.
- Datapad: Der Text bricht eine Spur frueher um als im Spiel ("... energy blade / of great" statt "... energy blade of / great").
- Datapad Force-Seite: Nachgeholt auf kejim_base; im Spiel ohne Kraefte leer, in der Vorschau mit Beispielkraeften. Rahmen gleich, die Kraefte selbst sind so nicht verglichen.
- jk2mp Listen: Die Zeilen stehen im Spiel rund 2,7 virtuelle Punkte hoeher (createserver, Kartenliste). Ursache nicht gefunden, darum nicht geschoben.
- jk2mp createserver: "CLOSED" ist im Spiel dunkler als in der Vorschau.
- jk2mp ingame: Die Leiste ist auf dem Foto weiss verrauscht, weil das Menue ohne Partie geoeffnet wurde; im Spiel so nicht pruefbar ohne Tastendruck (kein Fokus).
- JK2-Ingame-Menues mit Spielzustand (Abstimmung, Spielerliste) sind ohne Partie verglichen.

Weiter offen:
- Die Ausgabe von `--version` und unbekannten Schaltern ist nach Auftrag nicht mit der echten exe gelaufen (Konsole, Umleitung, Meldungsfenster).
- Liste "Zuletzt bearbeitet" fuer Menue-Projekte: gibt es nicht, waere ein eigener Wunsch.
- Teil 2, Punkt 6 (Umbenennen) ist hier erledigt, die uebrigen Punkte von Teil 2 nicht begonnen.

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
