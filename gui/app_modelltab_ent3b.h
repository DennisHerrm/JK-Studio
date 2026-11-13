// app_modelltab_ent3b.h - ENTSCHEID3B (13.11., plaene/entscheidungen_1211.md B14-B17, C1, C3): Proben fuer den Selbsttest
//
// Was die Oberflaeche zuletzt gezeigt hat, damit der Selbsttest es gegen den alten und den neuen Stand gleich pruefen kann:
//   Tempo     der Text des Reglers Tempo im Generator (B16) und sein Tooltip
//   Dopesheet die Zeilen des Dopesheets im Bereich Animation (Name, Code der Schluesselwahl, Mitte auf dem Schirm) - B14 (eine
//             Spur IK/FK je Glied) und B17 (die Plattform)
#ifndef BHED_GUI_APP_MODELLTAB_ENT3B_H
#define BHED_GUI_APP_MODELLTAB_ENT3B_H

#include <string>
#include <vector>

namespace bhed::gui {

struct Ent3bDsZeile {
    std::string name;
    int code = -1;     // Knochen, ikWahlCode, ikAnteilCode, kPlattformWahl (rigdaten.h); -2 Zusammenfassung, -9 Glied
    float y = 0.0F;    // Mitte der Zeile auf dem Schirm
};

struct Ent3bProbe {
    std::string tempoText;     // der Wert, wie der Regler Tempo ihn zeigt
    std::string tempoHinweis;  // sein Tooltip
    int tempoBild = -1;        // ImGui-Bild, in dem er gezeichnet wurde
    std::vector<Ent3bDsZeile> dsZeilen;
    int dsBild = -1;
    float dsOben = 0.0F;    // sichtbarer Bereich der Spuren (Schirm), ausserhalb liegen Zeilen weggerollt
    float dsUnten = 0.0F;
};
Ent3bProbe& ent3bProbe();

}  // namespace bhed::gui
#endif
