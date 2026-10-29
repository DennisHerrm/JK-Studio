// menueeditor.h - Menue-Editor M1: der Arbeitsbereich "Menu Editor".
//
// Text und Vorschau desselben .menu: links Bauteile und Gliederung, in der
// Mitte die Seiten als Reiter und die Arbeitsflaeche (640x480 wie das Spiel),
// rechts Eigenschaften, Cvars und Meldungen der Engine, unten der Code-Editor.
#ifndef BHED_GUI_MENUEEDITOR_H
#define BHED_GUI_MENUEEDITOR_H

#include <map>
#include <string>
#include <vector>

struct ImFont;

namespace bhed::gui::menueeditor {

[[nodiscard]] bool aktiv();
void zeichne(float hoehe);
// Fuer den Selbsttest und die Tour: Menue aus dem Spiel bzw. Text oeffnen.
bool oeffneAusSpiel(const std::string& spielPfad);
bool oeffneText(const std::string& name, const std::string& text);
// Eine .menu/.txt von der Platte (abgelegt oder ueber den Dialog).
bool oeffneDatei(const std::string& platte);
// M2 Punkt 3: alle Dateien eines Menuesatzes (loadMenu in menus.txt); Zahl der Dateien
int oeffneMenuesatz(const std::string& spielPfad);
[[nodiscard]] std::string letzterSatzWechsel();
[[nodiscard]] std::string laufInfo();   // Selbsttest: aktives Dokument, Satz, Skriptprotokoll
// M2 Ebenenliste: Auge (nur Vorschau), Reihenfolge (verschiebt das itemDef im Text), Alt+Klick-Durchwahl
void verbergeItem(int item, bool verbergen);
bool ebeneAnStelle(int item, int ziel);
void altKlickVorschau(float x, float y);   // Selbsttest: Menue, in dessen Datei zuletzt gewechselt wurde
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
    bool codeFestbreite = false;   // M2: Code-Editor mit Festbreitenschrift
    int kinoBilder = 0;            // M2: gezeigte Bilder aus RoQ-Videos
    int modellAufrufe = 0;         // M2: Zeichenaufrufe der 3D-Modelle (gpu::zeichneMenueFigur)
    std::string modellFehler;      // letzte Meldung dabei
    int klangEreignisse = 0;       // M2 Punkt 5: ausgeloeste Klaenge (im Selbsttest stumm)
    std::string letzterKlang;
    int paketDateien = 0;          // M2: eigene Dateien im Paket (Bilder, Videos, Texte, Schriften)
    std::vector<std::string> paket;   // ihre Spielpfade
    float flaeche[4] = {0, 0, 0, 0};  // Bildvergleich: die 640x480-Flaeche im Fenster (x, y, b, h)
    double bildzeitMs = 0;            // Arbeitszeit des Arbeitsbereichs je Bild (gleitender Mittelwert)
    // M2 Live-Diagnose des aktiven Dokuments
    int diagFehler = 0, diagWarnungen = 0, diagHinweise = 0, diagLaeufe = 0;
    bool diagAktuell = false;             // Ergebnis gehoert zum jetzigen Text
    std::vector<std::string> diagnosen;   // "Zeile: Meldung"
};
[[nodiscard]] Stand stand();
// Befehle wie die Knoepfe (Selbsttest)
void waehle(int item, bool dazu);
void verschiebeAuswahl(float dx, float dy);
void setzeInteraktion(bool an);
void klickVorschau(float x, float y);   // im virtuellen Bildschirm
void setzeCvar(const std::string& name, const std::string& wert);
// Rechte Spalte: 0 Hierarchie, 1 Text, 2 Eigenschaften (gespeichert in den Einstellungen)
void setzeReiter(int reiter);
// Live-Diagnose: die Schnellkorrektur der Meldung Nr. anwenden
bool korrigiereDiagnose(int nr);
[[nodiscard]] int reiter();
[[nodiscard]] std::string cvarWert(const std::string& name);   // Vorschau bzw. Bedienen
bool rueckgaengig();
bool wiederholen();
void fuegeEin(int typ);
int suche(const std::string& muster, bool regex);
int ersetzeAlle(const std::string& muster, const std::string& ersatz, bool regex);
void springeZuItem(int item);
// Items der Auswahl: duplizieren (Strg+D), loeschen (Entf), kopieren (Strg+C), einfuegen (Strg+V)
void dupliziereAuswahl();
void loescheAuswahl();
std::string kopiereAuswahl();
void fuegeItemsEin(const std::string& text);
bool speichereUnter(const std::string& pfad);
bool exportierePk3(const std::string& pfad);
// M2 Punkt 7: nur geaenderte Menues (und das Paket); Pruefung fuer die Fassungen
bool exportiereAenderungen(const std::string& pfad);
[[nodiscard]] int letzterExport();   // Dateien im zuletzt geschriebenen Paket
int pruefeFuerSpiel(bool ja, bool jk2, bool md);   // Befunde aller offenen Menues
// M2 Punkt 8: Testpaket und Befehlszeile (im Selbsttest nie gestartet); "" = nicht moeglich
std::string imSpielTesten();
// M2 Punkt 9: Schrift aus einer TTF ins Paket (fonts/<name>.fontdat + .tga)
bool baueSchriftAusTtf(const std::string& ttf, const std::string& name, int groesse);
// M2 Punkt 6: Sprache der Vorschau, eigene Texte
void setzeVorschauSprache(const std::string& sprache);
bool legeTextAn(const std::string& schluessel, const std::map<std::string, std::string>& texte);
[[nodiscard]] std::string loeseText(const std::string& at);
[[nodiscard]] std::string textPaket();
// M2: Festbreitenschrift des Code-Editors (main_win32.cpp buildFonts)
void setzeCodeSchrift(ImFont* f);
void setzeBalken(bool balken);   // 4:3 mit Balken (true) oder gestreckt
// M2 Punkt 2: Video/GIF als RoQ ins Paket (synchron, fuer Selbsttest); Spielpfad oder ""
std::string importiereVideo(const std::string& platte, int guete, int fps, int maxSeite);
// M2 Punkt 4: Vorlage aus der Bibliothek an (x, y) einsetzen; Auswahl als eigene Vorlage
bool fuegeVorlageEin(const std::string& id, float x, float y);
bool speichereAuswahlAlsVorlage(const std::string& name);
std::vector<std::string> vorlagenIds();
// Datei direkt ins Paket legen (Spielpfad -> Bytes)
void legeInsPaket(const std::string& spielPfad, const std::string& bytes);

}  // namespace bhed::gui::menueeditor

#endif  // BHED_GUI_MENUEEDITOR_H
