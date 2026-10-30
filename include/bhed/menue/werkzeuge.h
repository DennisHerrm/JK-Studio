// werkzeuge.h - Menue-Editor M1 Punkt 4: Werkzeuge der Arbeitsflaeche -
// Einrasten, Ausrichten/Verteilen, Items einfuegen/kopieren/loeschen/
// verschieben (Ebenen), Gruppieren. Alles ueber Quelltext-Spannen: nur die
// betroffenen Zeilen aendern sich.
//
// Eigener Code.
#ifndef BHED_MENUE_WERKZEUGE_H
#define BHED_MENUE_WERKZEUGE_H

#include "bhed/menue/menue.h"

#include <string>
#include <vector>

namespace bhed::menue {

struct Hilfslinie {
    bool senkrecht = true;   // x = wert, sonst y = wert
    float wert = 0;
    float von = 0, bis = 0;  // Ausdehnung zum Zeichnen
};

struct Einrasten {
    float dx = 0, dy = 0;               // Korrektur der Verschiebung
    std::vector<Hilfslinie> linien;
};

// Rastet r (schon verschoben) an Raster (raster > 0) und an Kanten/Mitten der
// anderen Rechtecke und des Bildschirms ein (Toleranz in virtuellen Punkten).
[[nodiscard]] Einrasten einrasten(const Rechteck& r, const std::vector<Rechteck>& andere, float raster, float toleranz,
                                  bool anRaster, bool anKanten);

enum class Ausrichtung { Links, Rechts, Oben, Unten, MitteH, MitteV };
// Richtet alle an der ersten aus (wie die gaengigen Editoren: am "Anker").
void ausrichten(std::vector<Rechteck>& r, Ausrichtung a);
// Gleiche Abstaende zwischen den Rechtecken (mindestens drei).
void verteilen(std::vector<Rechteck>& r, bool waagrecht);

// Vorlage fuer ein neues Item des Typs (ITEM_TYPE_*), passend zum Dialekt,
// eingerueckt mit einr (eine Zeile pro Schluesselwort).
[[nodiscard]] std::string itemVorlage(int typ, Dialekt d, const std::string& name, const Rechteck& r,
                                      const std::string& einr, const std::string& nl);

// Ein neues Item am Ende des Menues (vor dessen schliessender }).
[[nodiscard]] bool itemEinfuegen(const Datei& d, int menu, const std::string& itemText, std::vector<Ersetzung>& out);
// Item-Block samt Einrueckung und Zeilenende entfernen.
[[nodiscard]] bool itemLoeschen(const Datei& d, int menu, int item, std::vector<Ersetzung>& out);
// Der Text eines Items (fuer Kopieren), ab Zeilenanfang bis Zeilenende.
[[nodiscard]] std::string itemText(const Datei& d, int menu, int item);
// Item um eine Stelle nach vorn (spaeter gezeichnet, ueber den anderen) oder
// nach hinten verschieben - die Engine zeichnet in Dateireihenfolge.
[[nodiscard]] bool itemVerschieben(const Datei& d, int menu, int item, bool nachVorn, std::vector<Ersetzung>& out);
// M2 (Ebenenliste): das itemDef an die Stelle ziel der Reihenfolge setzen (0 = zuerst
// gezeichnet, ganz hinten). Der Block zieht mit seinen eigenen Zeilen um, Kommentare
// zwischen den Bloecken bleiben stehen; eine Ersetzung (ein Rueckgaengig-Schritt).
[[nodiscard]] bool itemAnStelle(const Datei& d, int menu, int item, int ziel, std::vector<Ersetzung>& out);
// Name fuer eine Kopie: name_2, name_3 ... (frei im Menue)
// M6 Punkt 2: die Items neu auf Plaetze verteilen - an die Stelle von Item plaetze[k] kommt der
// itemDef-Block von Item neu[k] (gleich viele, jedes einmal); die anderen Items bleiben stehen.
[[nodiscard]] bool itemsNeuOrdnen(const Datei& d, int menu, const std::vector<int>& plaetze, const std::vector<int>& neu,
                                  std::vector<Ersetzung>& out);
// M6 Punkt 8: die Schluessel, in denen sich zwei Items unterscheiden (Werte wie im Text; fehlende
// und zusaetzliche Schluessel zaehlen mit), und ein Item durch einen fertigen itemDef-Text ersetzen.
[[nodiscard]] std::vector<std::string> itemUnterschiede(const Datei& a, const Item& ia, const Datei& b, const Item& ib);
[[nodiscard]] bool itemErsetzen(const Datei& d, int menu, int item, const std::string& neuerText, std::vector<Ersetzung>& out);
[[nodiscard]] std::string freierName(const Menu& m, const std::string& basis);
// In einem kopierten Item-Text name ersetzen (oder einfuegen).
[[nodiscard]] std::string umbenennen(const std::string& itemText, const std::string& neuerName, Dialekt d);

}  // namespace bhed::menue

#endif  // BHED_MENUE_WERKZEUGE_H
