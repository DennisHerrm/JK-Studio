# Menue-Editor: Funktionstabelle (rc608 Teil 3b)

Quelle: H = Handbuch, T = Tutorial, N = Notizen (menue1 bis 6, rc608), U = Oberflaeche, V = Wunsch von shank (vision-jka-studio, Auftrag).
Getestet: Name = `BHED_MENUE_NUR=<name>` (echte Klicks und Tasten; "Datei" = Text und Platte verglichen), Foto = nur per Foto, Spiel = im laufenden Spiel.

## Dateien und Projekte
| Funktion | Quelle | getestet wie | Ergebnis |
|---|---|---|---|
| Startbildschirm, Format SP/MP | N | start | gut, 30/30 |
| Neues Projekt, Projekt oeffnen, Speichern, Export | H, T | m5b, tourmenue (Klick mit ersetztem Dialog) | gut |
| Menue aus dem Spiel oeffnen | H, T | m2, hilfe (tut-menue) | gut |
| Datei oeffnen (Dialog) | U | m2 ueber `oeffneDatei`; der Dialog selbst nicht | gut, Dialog nicht bedient |
| Menuesatz oeffnen | H | m2 | gut |
| Speichern, Strg+S (auch im Text-Editor) | H | datei | war nicht da, jetzt gut |
| Speichern unter, Strg+Umschalt+S, Abbrechen, neuer Dateiname | U | datei | war nicht da, jetzt gut |
| Datei von aussen geaendert: Meldung, Laden als ein Schritt, Behalten | N | datei | war nicht da, jetzt gut |
| Eigenes Speichern meldet nichts | N | datei | gut |
| Reiter schliessen mit Frage | H | m6 | gut |
| Sicherung nach Absturz | H | m5b | gut |
| Export pk3, nur Aenderungen | H | m2, m5b | gut |
| Neue Menuedatei steht im Menuesatz | N | satz, Spiel (MD, jasp) | fehlte, jetzt gut |
| Im Spiel testen: Paket, Befehlszeile, oeffnet die offene Seite | H, T | satz, tourmenue, Spiel (MD SP und MP, jasp, jamp, jk2sp, jk2mp) | oeffnete nichts Eigenes, jetzt gut |
| Laufendes Testspiel laedt neu (ui_load) | H | m7 | gut |
| Tastatur: Strg+O, Strg+N | U | fehlen im Menue-Editor, die Knoepfe reichen | bewusst nicht gebaut |

## Flaeche
| Funktion | Quelle | getestet wie | Ergebnis |
|---|---|---|---|
| Klick waehlt, neuer Druck zieht, Schwelle | N | ziehen | gut |
| Griffe und Raender skalieren, Mauszeiger | N | zeiger | gut |
| Zoom, Verschieben, Strg+0, Strg+1 | H | zoom | gut |
| Hilfslinien aus dem Lineal | H | m2 | gut |
| Raster, an Raster, an Kanten | H | m2 | gut |
| Tab-Reihenfolge | H | m6 | gut |
| Referenzbild | H | m6 | gut |
| Alt+Klick durch den Stapel, Alt+Ziehen | N | ziehen | gut |
| Rahmen aufziehen, Umschalt nimmt dazu | V | kontext | fehlte, jetzt gut |
| Strg+A, Strg+X | V | kontext | fehlte, jetzt gut |
| Rechtsklick: Ausschneiden, Kopieren, Einfuegen, Duplizieren, Loeschen, vorn, hinten, Sperren, Ausblenden, Vorlage, Liste unter dem Zeiger | V | kontext | fehlte, jetzt gut |
| Hintergrund ueber den ganzen Schirm laesst Klicks durch | V | kontext | war Fehler, jetzt gut |
| Schloss und Auge bleiben am Element, gelten je Menue | N | kontext | war Fehler, jetzt gut |
| Text direkt schreiben, Doppelklick | N | schreiben | gut |
| Einfuegen auf einer anderen Seite und in einer anderen Datei, auch SP nach MP | V | kontext | Name wurde unnoetig geaendert, jetzt gut |
| Bedienen: Klick, Hover, Skripte, Fade, Transition | H | m2, m4, leiste (Try), tourmenue | gut |
| Zwei Videos halten einander an | N | zweivideos | war anders als im Spiel, jetzt gut |
| Modell-Griffe | N | modell | gut |
| Spalten zuklappen, wenig Platz | N | eng, textbreite | fehlte, jetzt gut |

## Seiten
| Funktion | Quelle | getestet wie | Ergebnis |
|---|---|---|---|
| Neue Seite, Vorlagen | H | m6 | gut, neutrale Namen |
| Menue umbenennen mit open und close, Name belegt | V | seiten | fehlte, jetzt gut |
| Menue duplizieren, loeschen, sortieren | V | seiten | fehlte, jetzt gut |
| Zustandsreiter | H | hierarchie, m5 | gut |

## Teile
| Funktion | Quelle | getestet wie | Ergebnis |
|---|---|---|---|
| Text, Button, Bild, Video, Modell, Ja/Nein, Mehrfachwahl, Schieber, Eingabe, Liste, Taste: einfuegen, ziehen, skalieren, duplizieren, einfuegen, loeschen, sperren, speichern, neu lesen, SP und MP | V | widgets | gut |
| Lautstaerke, Reiterleiste, Detailbereich, Spielerliste | V | widgets, links | gut; umbenennen des ganzen Detailbereichs gibt es nicht, nur je Element |
| Ablegen per Ziehen, Doppelklick | N | ablegen, stapel | gut |
| Rahmen fuer Bild und Video, Auswahl, von Datei | V | rahmen | gut |
| Eigene Vorlagen, umbenennen, loeschen, zurueckholen | V | links | gut |
| Teile-Suche | V | links | gut |

## Properties
| Funktion | Quelle | getestet wie | Ergebnis |
|---|---|---|---|
| Karte: Name, Sichtbar, Sperren, vor, hinter, Duplizieren, Loeschen | V | leiste | gut |
| Feldsuche, File keys, Gruppen, Hilfezeile | V | leiste, leistebreit | gut |
| Felder je Typ (Tabelle in NOTIZEN_rc608, Punkt 14) | N | felder | gut |
| Text, Groesse, Farbe, Ausrichtung, Lage, Stil, Schrift | V | leiste | Umlaute wurden als UTF-8 geschrieben, jetzt gut |
| Bedingung: zeigen, verbergen, benutzbar, nicht benutzbar | V | feldneu | gab nur showCvar, jetzt gut |
| Auswahl der Mehrfachwahl (cvarStrList, cvarFloatList) | V | feldneu | gab es nur im Text, jetzt gut |
| ownerdraw, feeder, elementwidth, elementheight, columns | V | feldneu | gab es nur im Text, jetzt gut |
| Menue: Name, Vollbild, Hintergrund, Text unter dem Zeiger | V | leiste, feldneu | focusColor fehlte, jetzt gut |
| Leer, Sonderzeichen, Unsinn in jedem freien Feld | V | unsinn | 6 Faelle zerstoerten das Menue, 1 Absturz, jetzt gut |
| Zahlen tippen, ziehen, ruecksetzen, zuletzt benutzte Farben | V | leiste | gut |
| Komponenten, Preview cvars, Engine messages | H | leiste, m7, breiten | gut |
| Mehrere gewaehlt: Change all, Anordnen | V | leiste | gut |

## Events
| Funktion | Quelle | getestet wie | Ergebnis |
|---|---|---|---|
| Ausloeser, Schritte in Worten, Add step mit Suche | V | leiste, events, tourmenue | gut |
| Hover-Effekte, Klang, Try on canvas, Try this step | V | leiste | gut |
| Umbenennen zieht Ziele mit, fehlendes Ziel gelb | V | leiste | gut |

## Text-Editor
| Funktion | Quelle | getestet wie | Ergebnis |
|---|---|---|---|
| Tippen, Mehrfachcursor, Falten, Lesezeichen, Minimap | H | m2 | gut |
| Suchen und Ersetzen | H | ersetzen | gut |
| Umbruch, Zoom, Zeiger | V | textbreite, textzoom, zeiger | gut |
| Live-Diagnose, Korrekturen | H | m2 | gut |

## Pruefung
| Funktion | Quelle | getestet wie | Ergebnis |
|---|---|---|---|
| Grenzen je Spiel | N | ctest menue_pruefung, Spiel (jasp 150, jamp 256, jk2sp 128, jk2mp 256, MD SP und MP ueber 600) | falsche Werte fuer Original und JK2, jetzt gemessen |
| Niemand oeffnet das Menue | V | satz | fehlte, jetzt gut |
| Fehlendes Ziel | V | leiste | gut |
| Keine Fokusfarbe, Modell ohne Skin im MP | Spielfoto | tourmenue, ctest | neu |
| Lange Meldungen | N | satz, Foto | wurden abgeschnitten, jetzt gut |
| Liste passt immer zum jetzigen Stand | N | satz | nannte alte Dateien, jetzt gut |
| Menuefluss, Stile, Assistent, Schrift, Texte, Video-Import | H | m6, m3, m4 | gut |

## Hilfe
| Funktion | Quelle | getestet wie | Ergebnis |
|---|---|---|---|
| Tutorial "Dein erstes eigenes Menue" | T | tourmenue, touralle (en, cs, ja bei 100 %; MP-Format mit MD, JKA, JK2), Spiel | neu geschrieben: endet mit einem Menue im Spiel; 200 % unter 4K kommt bis Schritt 7 (Notizen) |
| Tutorial "Ein Menue aendern" | T | tourspiel (echte Klicks; en, cs, ja) | gut |
| Werkzeugleiste und Felder ohne Namen erklaeren sich | V | anfaenger | gut |

## Nicht getestet, mit Grund
- Windows-Dateidialoge: Sie erschienen auf shanks Bildschirm; der Selbsttest ersetzt sie (`setzeProjektDialog`, `setzeSpeichernDialog`, `setzeBildDialog`).
- Klicks im laufenden Spiel: Das Fenster hat keinen Fokus und `in_mouse 0` (Auftrag: keine Eingabe). Im Spiel ist also nur das Bild verglichen; Hover und Klick laufen im Bedienmodus der Vorschau.
- Ton im Spiel und in Tests: stumm nach Auftrag.
