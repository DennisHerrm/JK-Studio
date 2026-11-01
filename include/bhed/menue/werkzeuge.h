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
// rc608 Teil 3a: Nie unbemerkt deckungsgleich: Liegt r genau auf einem gleich grossen Rechteck, rueckt es um schritt nach rechts unten, bis es frei ist
[[nodiscard]] Rechteck ohneDeckung(Rechteck r, const std::vector<Rechteck>& andere, float schritt = 20.0F);
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
                                      const std::string& einr, const std::string& nl, Spiel spiel = Spiel::JA);   // rc608: spiel entscheidet, was ein neues Modell zeigt

// rc608 Punkt 14: Wertet das Spiel diesen Schluessel bei diesem Elementtyp aus? (ui_shared SP und MP; JK2 kennt kein Ghoul2 im Menue.)
// Tabelle in docs/intern/NOTIZEN_rc608.md. Gemeinsames (name, group, rect, style, visible, background, Farben der Flaeche) gilt immer.
[[nodiscard]] bool feldGilt(const std::string& schluessel, int typ, int style, Spiel spiel);

// Ein neues Item am Ende des Menues (vor dessen schliessender }).
// rc608 Teil 3a: Ausrichtung bleibt beim Skalieren. Die Datei hat nur textalign, textalignx, textaligny wie im Spiel; ob ein
// Text folgt, steht in den Zahlen selbst: Passen sie zur Regel, folgen sie; wer eine andere Zahl tippt, schaltet das damit ab.
// Item_SetTextExtents: x ist bei Mitte und Rechts der Ankerpunkt. y ist der Bezugspunkt von RE_Font_DrawString: Die Grundlinie liegt um "unten" darunter,
// die Oberkante der grossen Buchstaben um "oben" (vorschau.h, textMasse); Text_Height allein traefe die grossen Schriften nicht (im Spiel fotografiert).
constexpr float kTextRand = 6.0F;
[[nodiscard]] float textXFuer(int ausrichtung, float breite);                  // 0 links, 1 Mitte, 2 rechts
[[nodiscard]] float textYFuer(int senkrecht, float hoehe, float oben, float unten);    // 0 oben, 1 Mitte, 2 unten
[[nodiscard]] bool textXFolgt(int ausrichtung, float x, float breite);         // links: nie, dort bleibt die Zahl ohnehin
[[nodiscard]] int textSenkrecht(float y, float hoehe, float oben, float unten);        // 0, 1, 2 oder -1: eine eigene Zahl
// Ein neuer Rahmen wartet auf sein Video (das Spiel liest den Kommentar nicht); mit der Auswahl faellt die Zeile weg.
inline constexpr const char* kVideoRahmenKennung = "// jkstudio:rahmen video";
// Ein Bild ist im Spiel ein Element ohne Text mit WINDOW_STYLE_SHADER; einen eigenen Typ gibt es nicht.
[[nodiscard]] bool istBildRahmen(const Item& it);

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
// rc608 Teil 3b: ganze Menues einer Datei. Loeschen samt Zeile; Tauschen vertauscht ein Menue mit dem folgenden;
// Umbenennen zieht open und close in der ganzen Datei mit. false, wenn das Menue aus einem Makro oder einer anderen Datei kommt.
[[nodiscard]] bool menuLoeschen(const Datei& d, int menu, std::vector<Ersetzung>& out);
[[nodiscard]] bool menuTauschen(const Datei& d, int menu, std::vector<Ersetzung>& out);
[[nodiscard]] bool menuUmbenennen(const Datei& d, int menu, const std::string& neu, std::vector<Ersetzung>& out);
// Der Text eines Menues mit einem anderen Namen (Duplizieren)
[[nodiscard]] std::string menuTextMitName(const Datei& d, int menu, const std::string& name);
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
