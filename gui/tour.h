// tour.h - die Tutorials im Programm: Fenster, Hervorhebung, Auswertung
//
// Paket tour (rc601). Der reine Teil (Daten, Pruefungen, Ablauf) steht in
// include/bhed/tour.h; hier die Oberflaeche: das Fenster "###tour" mit der
// Liste und dem laufenden Schritt, die Hervorhebung im Vordergrund (abgedunkelt
// bis auf das Ziel, gelber Rahmen wie im Handbuch), "Zeig es mir" (ein
// Geisterzeiger, der nichts klickt) und das Auswerten der Pruefungen am
// Programmzustand.
#ifndef BHED_GUI_TOUR_H
#define BHED_GUI_TOUR_H

#include "imgui.h"
#include "imgui_internal.h"
#include "bhed/tour.h"

#include <map>
#include <string>
#include <vector>

namespace bhed::gui {

// Jedes Bild, in draw() VOR selbsttest() - der Selbsttest sieht die Knoepfe
// des Fensters dann im selben Bild (das Register leert elemente::bildEnde
// danach).
void tourBild();
// rc601 tour (Review): aus einem Modal heraus (drawEditor, vor EndPopup)
// aufrufen - dann zeichnet es das Tutorial-Fenster im Begin-Stapel des Modals,
// sonst laesst ImGui keine Maus daran (imgui.cpp:5689). tourBild laesst das
// Fenster in diesem Bild dann aus.
void tourImModal();
// Das Fenster mit der Liste der Tutorials oeffnen (Help-Menue, Startflaeche).
void tourFensterOeffnen();
// Eine Tour beginnen (ab Schritt `ab`, 0-basiert); false mit Meldung in der
// Statuszeile, wenn es sie nicht gibt.
bool tourStarten(const std::string& id, int ab = 0);
// Tour beenden (Fortschritt bleibt in den Einstellungen) und Fenster schliessen.
void tourSchliessen();
// Laeuft gerade eine Animation (Puls nach einem Schrittwechsel, "Zeig es mir",
// ein erledigter Schritt vor dem Weiterschalten)? Dann darf das Programm nicht
// ruhen (darfRuhen in app.cpp), sonst ruckelt sie.
bool tourAnimiert();

// H1: die Tutorials und der Suchindex des Handbuchs in der eingestellten
// Sprache (fuer den Helfer, gui/hilfe.cpp).
const bhed::tour::Touren& tourDaten();
const std::vector<bhed::tour::Abschnitt>& tourHandbuch();
// H1: "Zeig es mir" des Helfers - hebt ein Bedienelement hervor (eine Tour aus
// einem Schritt, Weiter schliesst sie). `voraus` wie in bhed/tour.h.
bool tourZeigeOrt(const std::string& titel, const std::string& text, const std::string& ziel, const std::string& voraus);

// H1: Tour-Aktionen ausfuehren (einrichten-Grammatik, bhed/tour.h) - fuer
// die KI-Befehle "script:<aktion>".
void tourAktionen(const std::string& aktionen);

// H1: Messwerte aus allen Bereichen fuer "zahl:<name>" (gui/tour_messwerte.cpp).
// `karte`: auch die des Karten-Editors (nur wenn ein Schritt sie braucht).
void tourMesswerte(std::map<std::string, double>& werte, bool karte, double jetzt);

// Fuer den Selbsttest: was die Tour gerade zeigt.
struct TourSicht {
    bool fensterOffen = false;
    bool aktiv = false;
    bool fertig = false;
    std::string id;
    int schritt = -1;
    int anzahl = 0;
    bool erfuellt = false;
    bool overlay = false;           // Hervorhebung in diesem Bild gezeichnet
    // rc601 tour (Review): was die Abdunklung in diesem Bild ausspart (Ziele,
    // das Fenster, offene Klappfenster und Hinweise).
    std::vector<ImRect> loecher;
    std::vector<ImRect> ziele;      // was hervorgehoben ist (Oeffner oder Ziele)
    bool zeigtOeffner = false;      // Vorbedingung fehlt: der Oeffner wird gezeigt
    bool geist = false;             // "Zeig es mir" laeuft
    bool angemeldet = false;        // hat fuer das naechste Bild Beschriftungen angemeldet
    int touren = 0;                 // geladene Tutorials
    std::string fehler;             // Laden fehlgeschlagen
    std::string ordner;             // wo touren.json gefunden wurde
    std::string hinweis;            // H1: der freundliche Hinweis, der gerade steht
};
TourSicht tourSicht();

}  // namespace bhed::gui

#endif
