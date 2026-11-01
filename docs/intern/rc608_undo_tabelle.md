# Menue-Editor: Undo-Tabelle (rc608 Teil 3b)

Erzeugt aus dem Lauf `u1` (`BHED_MENUE_NUR=undo`, Zeilen "Knopftest: Undo-Tabelle" im Log). Jede Zelle: nach der Aktion auf diesem Weg zurueck (Strg+Z bzw. Menue) und wiederholt (Strg+Y bzw. Menue), jeweils byte-gleich mit dem Stand davor bzw. danach. "gut/gut" = beides richtig. Dazu je Aktion: Sie ist genau ein Schritt. Ueber Reiterwechsel, nach Speichern und mit mehreren Dateien pruefen die Teile `undo` (Wege am Ende der Liste) und `undomehr`.

Wege:

- W1: Strg, Fokus Flaeche, mit Auswahl
- W2: Strg, Fokus Flaeche, ohne Auswahl
- W3: Strg, Fokus Hierarchie
- W4: Strg, Fokus Properties
- W5: Strg, Fokus Events
- W6: Strg, Fokus Text-Editor
- W7: Pfeile der Leiste
- W8: Pfeile, Umbenennen offen

| Aktion | ein Schritt | W1 | W2 | W3 | W4 | W5 | W6 | W7 | W8 |
|---|---|---|---|---|---|---|---|---|---|
| Ziehen | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Skalieren | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Pfeiltaste | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Widget einfuegen | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Bibliothek einfuegen | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Loeschen | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Duplizieren | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Properties rect | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Properties name | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Properties visible | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Properties style | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Events | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Hierarchie umbenennen | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Ebene nach vorn | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Ausrichtung | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Fuellung | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Reihenfolge eines Schritts | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Schritt hinzufuegen | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Umbenennen in der Karte | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Zuruecksetzen | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Change all | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Anordnen | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Rahmen einfuegen | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Fertiges Teil einfuegen | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Rechtsklick ganz nach vorn | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Rechtsklick ausschneiden | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Strg+X | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Strg+V | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Menue umbenennen | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Menue duplizieren | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |
| Text-Editor | gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut | gut/gut |

Summe der Zellen: 496 richtig, 0 falsch. Gesamtzeilen im Log: 745.

Das eine Rot "Properties rect | Fokus Text-Editor" aus Teil 2 und 3a kam nicht wieder. Im Code gibt es dafuer keine Reihenfolgeabhaengigkeit; siehe Notizen, Teil 3b.
