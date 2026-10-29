// layout.h - Menue-Editor M5 Punkt 1: Layouts wie in Qt (nebeneinander, untereinander,
// Raster, zwei Spalten mit Teiler), mit Abstand, Raendern, Spacern und Ausrichtung.
// Eigener Code.
//
// Die Engine kennt keine Layouts: ins .menu kommen nur feste rects. Die Layout-Angaben
// stehen als Kommentarzeile im menuDef, die das Spiel ueberliest (COM_ParseExt
// ueberspringt //-Kommentare) und der Editor wieder liest:
//   // jkstudio:layout name=leiste type=hbox frame=panel spacing=8 margins=4,4,4,4 halign=center valign=fill items=a,*,b,@innen
// items: Item-Namen, "*" bzw. "*2" = Spacer (Dehnung 1 bzw. 2), "@name" = verschachteltes Layout.
// Ohne Layout bleibt die Datei byte-gleich.
#ifndef BHED_MENUE_LAYOUT_H
#define BHED_MENUE_LAYOUT_H

#include "bhed/menue/menue.h"

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace bhed::menue {

enum class LayoutArt { Waagrecht, Senkrecht, Raster, Teiler };
enum class LayoutAusrichtung { Anfang, Mitte, Ende, Fuellen };

struct LayoutKind {
    std::string name;      // Item- oder Layoutname (leer bei Spacern)
    bool spacer = false;
    bool layout = false;   // "@name"
    float dehnung = 1.0F;  // Spacer: Anteil am freien Platz
};

struct Layout {
    std::string name;
    LayoutArt art = LayoutArt::Waagrecht;
    std::string rahmenItem;          // frame=<item>: dessen rect ist die Flaeche
    Rechteck rahmen;                 // rect=x,y,w,h (ohne frame bzw. als Merkwert)
    float abstand = 0;               // spacing
    float rand[4] = {0, 0, 0, 0};    // margins links, oben, rechts, unten
    LayoutAusrichtung h = LayoutAusrichtung::Anfang, v = LayoutAusrichtung::Anfang;
    int spalten = 2;                 // Raster
    float teilung = 0.5F;            // Teiler: Anteil der linken Spalte
    float griff = 6;                 // Teiler: Breite des Teilers
    bool versteckteAuslassen = false;   // hidden=skip: versteckte Kinder nehmen keinen Platz
    std::vector<LayoutKind> kinder;
    std::uint32_t anfang = 0, ende = 0;   // Kommentarzeile im Text (ab Zeilenanfang bis nach dem Zeilenende)
};

// Alle Layout-Zeilen eines Menues (in Textreihenfolge).
[[nodiscard]] std::vector<Layout> liesLayouts(const Datei& d, int menu);
// Eine Zeile lesen ("// jkstudio:layout ..."); false, wenn es keine Layout-Zeile ist.
[[nodiscard]] bool liesLayoutZeile(const std::string& zeile, Layout& out);
// Der Kommentar ohne Einrueckung und Zeilenende.
[[nodiscard]] std::string layoutZeile(const Layout& l);

// Was das Rechnen von aussen braucht.
struct LayoutGroessen {
    std::function<bool(const std::string& item, Rechteck& r)> rect;   // rect wie im Text; false = unbekannt
    std::function<float(const std::string& item)> textBreite;         // breitester Text aller Sprachen (0 = kein Text)
    std::function<bool(const std::string& item)> sichtbar;            // fuer hidden=skip (leer = alle sichtbar)
};

struct LayoutErgebnis {
    std::map<std::string, Rechteck> rects;   // neue rects der Kinder (und verschachtelter Rahmen)
    std::map<std::string, Rechteck> flaechen;   // Flaeche je Layout (fuer die Anzeige)
    std::vector<std::string> warnungen;       // Item-Namen, deren Text breiter ist als ihr rect
    std::vector<std::string> fehlend;         // Kinder, die es im Menue nicht gibt
};

// Rechnet alle Layouts (aeussere zuerst; verschachtelte bekommen ihre Flaeche vom Eltern-Layout).
[[nodiscard]] LayoutErgebnis rechneLayouts(const std::vector<Layout>& ls, const LayoutGroessen& g);

// Layout-Zeile einfuegen (neu: vor dem ersten itemDef des Menues, sonst vor der schliessenden }),
// ersetzen (gleicher Name) oder - mit loeschen - entfernen.
[[nodiscard]] bool layoutSchreiben(const Datei& d, int menu, const Layout& l, bool loeschen, std::vector<Ersetzung>& out);

// Zu welchem Layout gehoert ein Item (als Kind oder Rahmen)? -1 = keinem.
[[nodiscard]] int layoutVon(const std::vector<Layout>& ls, const std::string& item);

// Ein freier Layoutname: layout1, layout2 ...
[[nodiscard]] std::string freierLayoutName(const std::vector<Layout>& ls);

}  // namespace bhed::menue

#endif
