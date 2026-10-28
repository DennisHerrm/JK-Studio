// menueeditor.h - Menue-Editor M1: der Arbeitsbereich "Menu Editor".
//
// Text und Vorschau desselben .menu: links Bauteile und Gliederung, in der
// Mitte die Seiten als Reiter und die Arbeitsflaeche (640x480 wie das Spiel),
// rechts Eigenschaften, Cvars und Meldungen der Engine, unten der Code-Editor.
#ifndef BHED_GUI_MENUEEDITOR_H
#define BHED_GUI_MENUEEDITOR_H

#include <string>
#include <vector>

namespace bhed::gui::menueeditor {

[[nodiscard]] bool aktiv();
void zeichne(float hoehe);
// Fuer den Selbsttest und die Tour: Menue aus dem Spiel bzw. Text oeffnen.
bool oeffneAusSpiel(const std::string& spielPfad);
bool oeffneText(const std::string& name, const std::string& text);
// Eine .menu/.txt von der Platte (abgelegt oder ueber den Dialog).
bool oeffneDatei(const std::string& platte);
// Zustand fuer den Selbsttest (Wirkungspruefung).
struct Stand {
    int dokumente = 0;
    int menus = 0;
    int items = 0;
    int seite = 0;
    int seiten = 0;          // Reiter ueber der Flaeche
    std::vector<int> auswahl;
    std::string text;
    int meldungen = 0;
    int prims = 0;           // Zeichenbefehle der Vorschau im letzten Bild
    int bilderGeladen = 0;   // Shader mit Textur
    bool interaktion = false;
    std::string cursorZeile; // Text der Zeile unter dem ersten Cursor
};
[[nodiscard]] Stand stand();
// Befehle wie die Knoepfe (Selbsttest)
void waehle(int item, bool dazu);
void verschiebeAuswahl(float dx, float dy);
void setzeInteraktion(bool an);
void klickVorschau(float x, float y);   // im virtuellen Bildschirm
void setzeCvar(const std::string& name, const std::string& wert);
[[nodiscard]] std::string cvarWert(const std::string& name);   // Vorschau bzw. Bedienen
bool rueckgaengig();
bool wiederholen();
void fuegeEin(int typ);
int suche(const std::string& muster, bool regex);
int ersetzeAlle(const std::string& muster, const std::string& ersatz, bool regex);
void springeZuItem(int item);
bool speichereUnter(const std::string& pfad);
bool exportierePk3(const std::string& pfad);

}  // namespace bhed::gui::menueeditor

#endif  // BHED_GUI_MENUEEDITOR_H
