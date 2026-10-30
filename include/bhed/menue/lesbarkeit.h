// lesbarkeit.h - Menue-Editor M6 Punkt 9: Lesbarkeit eines Menues - zu kleine Schrift, zu wenig
// Kontrast auf gefuellten Flaechen, Texte, die in einer Sprache des Spiels nicht in ihr Feld passen
// (in einer anderen aber schon). Gemessen mit der Textbreite und -hoehe der Vorschau
// (RE_Font_StrLenPixels/HeightPixels wie im Spiel); die Schwellen sind Richtwerte des Editors
// (keine Engine-Grenzen): Texthoehe 8 Punkte im 640x480-Bildschirm, Kontrast 3:1 (WCAG 2).
//
// Eigener Code.
#ifndef BHED_MENUE_LESBARKEIT_H
#define BHED_MENUE_LESBARKEIT_H

#include "bhed/menue/menue.h"
#include "bhed/menue/vorschau.h"

#include <string>
#include <vector>

namespace bhed::menue {

inline constexpr float kMindestTexthoehe = 8.0F;
inline constexpr float kMindestKontrast = 3.0F;

enum class LesbarArt { SchriftKlein, KontrastGering, TextZuLang };

struct LesbarkeitsBefund {
    LesbarArt art = LesbarArt::SchriftKlein;
    int item = -1;
    float wert = 0, grenze = 0;   // Hoehe/Kontrast/Breite und die Schwelle bzw. Feldbreite
    std::string sprache;          // TextZuLang: in welcher Sprache
};

struct SprachTexte {
    std::string sprache;
    const Texte* texte = nullptr;
};

// Kontrastverhaeltnis nach WCAG 2 (relative Leuchtdichte, 1 bis 21).
[[nodiscard]] float kontrast(const Farbe& a, const Farbe& b);

// Befunde fuer die Items eines Menues mit Text; u wie die Vorschau (Schriften, Cvars), sprachen = alle
// Sprachen der Spieldaten (leer: keine Laengenpruefung).
[[nodiscard]] std::vector<LesbarkeitsBefund> lesbarkeit(const Datei& d, int menu, const Umgebung& u, const std::vector<SprachTexte>& sprachen);

}  // namespace bhed::menue

#endif  // BHED_MENUE_LESBARKEIT_H
