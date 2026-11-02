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
- Figur im Mittel 0,78 bis 1,06 je Karte, nicht fest: Ursache nicht gefunden (Pose oder Mip-Stufe, die Lichtwerte selbst stimmen).
- r_DynamicGlow: Movie Duels 1, Original 0 ("Dynamic Glow: disabled"). Die Vorschau hat den Schalter unabhaengig vom Regelwerk an.
- Schmerzanimation und Rueckstoss (siehe Funkenfaecher).
- Der Spieler mit Gewehr haelt es in der Vorschau seitlich, im Spiel im Anschlag (die Torso-Haltung gilt nur fuer NPCs). Gefunden am Holsterfoto.
- Kasten der Leiche aus DeadThink (maxs[2] aus eyePoint), pitch_roll_for_slope (geneigte Leiche), die Wurf-Zweige ausser dem Kantenwurf setzen ps.friction nicht.
- Waffen im Holster (cg_holster.cpp) und WP_SaberHolsterAvoidGuns; `*holsterorigin`; STUN_BATON am Spielerstart.
- tr.sunDirection ohne Gitter ist die Vorgabe, q3map_sun nicht gelesen.
- Testkarten ohne Entities: Die Vorschau zeichnet Untermodelle ohne Entity, das Spiel nicht (Tueren auf t3_byss, die Nebelwand auf duel_hoth). Auf den echten Karten gibt es den Fall nicht; duel_hoth echt: gleich.
- Die Karte ist jetzt in allen Selbsttests dunkler. Der Gesamtlauf steht aus: Prueffotos und Schwellen koennen dort auffallen. Handbuchbilder der Karte zeigen noch die alte Helligkeit, `h_ansicht_einst` noch 2,0x: mit dem Bilderlauf der Endrunde erneuern.

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
