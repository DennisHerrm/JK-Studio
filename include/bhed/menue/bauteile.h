// bauteile.h - Menue-Editor M2 Punkt 3 und 4: fertige Bauteile.
//
//   - Detailbereich in drei Ebenen nach dem ui_tab/ui_sel-Muster des Movie-
//     Duels-Einstellungsmenues: Reiter (mit Blaettern, wenn mehr Reiter als
//     Platz da sind) -> Unterreiter -> Detail. Alles nur ueber cvarTest/
//     showCvar und setcvar, also in jeder Fassung des Spiels lauffaehig.
//   - Die Bauteil-Bibliothek: Knopf, Schieber mit Cvar, Ja/Nein,
//     Reiterleiste, Detailbereich, Liste - und eigene Vorlagen aus der
//     Auswahl (Dateien im Einstellungsordner).
//
// Eigener Code.
#ifndef BHED_MENUE_BAUTEILE_H
#define BHED_MENUE_BAUTEILE_H

#include "bhed/menue/menue.h"

#include <string>
#include <vector>

namespace bhed::menue {

struct DetailEintrag {
    std::string titel, text;
};
struct DetailReiter {
    std::string titel;
    std::vector<DetailEintrag> unter;
};
struct DetailOptionen {
    Rechteck bereich{20, 40, 600, 400};
    std::vector<DetailReiter> reiter;
    int sichtbar = 4;   // Reiter gleichzeitig; mehr = Pfeile zum Blaettern
    std::string cvarReiter = "ui_tab", cvarSel = "ui_sel", cvarOfs = "ui_tabofs";
    std::string praefix = "dt";
};
// Beispiel nach dem Movie-Duels-Einstellungsmenue (Kategorien, Gruppen, Texte)
[[nodiscard]] DetailOptionen beispielDetail();
// Die itemDefs (Einrueckung mit Tabulatoren wie die Originale)
[[nodiscard]] std::string erzeugeDetailbereich(const DetailOptionen& o, Dialekt d);
// Was onOpen setzen muss, damit der erste Reiter offen ist
[[nodiscard]] std::string detailOnOpen(const DetailOptionen& o);

struct Vorlage {
    std::string id;     // "knopf", "schieber" ... oder Dateiname eigener Vorlagen
    std::string name;   // Anzeige
    std::string text;   // ein oder mehrere itemDef-Bloecke
    bool eigen = false;
};
[[nodiscard]] std::vector<Vorlage> standardVorlagen(Dialekt d);
// Eigene Vorlagen: <ordner>/<name>.vorlage (Text der itemDefs)
[[nodiscard]] std::vector<Vorlage> ladeEigeneVorlagen(const std::string& ordner);
bool speichereVorlage(const std::string& ordner, const std::string& name, const std::string& text);
// Ein Name, der als Dateiname taugt (Buchstaben, Ziffern, _ -)
[[nodiscard]] std::string vorlagenDateiname(const std::string& name);

}  // namespace bhed::menue

#endif  // BHED_MENUE_BAUTEILE_H
