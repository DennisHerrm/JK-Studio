// hilfe.h - der Helfer im Programm: "Was willst du tun?" (Stufe H1)
//
// Das Fenster "###hilfe" (F1, Help-Menue): eine Zeile fuer die Frage in
// eigenen Worten, darunter die besten Treffer aus dem Offline-Index
// (include/bhed/hilfe.h) - je Treffer eine kurze Erklaerung, "Zeig es mir"
// (Tour starten bzw. Bedienelement hervorheben) und der Weg ins Handbuch.
// Dazu beim allerersten Start "Was willst du zuerst tun?" und - freiwillig,
// ab Werk aus - die KI-Anbindung (gui/ki.cpp).
#ifndef BHED_GUI_HILFE_H
#define BHED_GUI_HILFE_H

#include <cstddef>
#include <string>
#include <vector>

namespace bhed::gui {

// Jedes Bild (app.cpp, vor tourBild): Helfer- und Erste-Schritte-Fenster.
void hilfeBild();
// Das Fenster oeffnen (F1, Help-Menue), optional mit einer Frage.
void hilfeOeffnen(const std::string& frage = "");
// Das Fenster schliessen.
void hilfeSchliessen();
// Ist das Fenster offen (darf das Programm ruhen? Nein, solange getippt wird)?
[[nodiscard]] bool hilfeOffen();

// "Zeig es mir" fuer einen Eintrag des Index (Kennung wie in bhed/hilfe.h:
// "tour:tut-karte@4", "ort:menue:MenuEdit>AppPrefs", "seite:k-pk3.html").
// false, wenn es die Kennung nicht gibt.
bool hilfeZeige(const std::string& id);

// Fuer den Selbsttest und die KI-Werkzeuge: die besten Treffer einer Frage.
struct HilfeTreffer {
    std::string id;
    std::string art;     // tour, seite, ort
    std::string titel;
    std::string text;
    std::string pfad;
    std::string seite;
};
[[nodiscard]] std::vector<HilfeTreffer> hilfeFrage(const std::string& frage, std::size_t hoechstens = 5);
// Alle Orte (Menuepunkte, Modusleiste, Einstellungen) eines Bereichs ("" = alle).
[[nodiscard]] std::vector<HilfeTreffer> hilfeOrte(const std::string& bereich);

struct HilfeSicht {
    bool offen = false;
    bool ersteSchritte = false;   // "Was willst du zuerst tun?" steht
    std::string frage;
    std::vector<std::string> treffer;   // Kennungen
    std::size_t eintraege = 0;          // Groesse des Index
};
[[nodiscard]] HilfeSicht hilfeSicht();
// Selbsttest: die Frage setzen wie getippt.
void hilfeFrageSetzen(const std::string& frage);
// Selbsttest: "Was willst du zuerst tun?" zeigen/schliessen.
void hilfeErsteSchritte(bool zeigen);

}  // namespace bhed::gui

#endif
