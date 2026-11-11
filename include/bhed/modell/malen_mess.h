// MBMAL: wohin die Zeit beim Malen geht - Zaehler und Zeitsummen fuer den Selbsttest ptlang (shank 10.11.: "wenn ich lange
// painte, dann wird es laggy und die fps geht runter"). Gezaehlt wird nur, solange `an` gesetzt ist; die Stoppuhr kostet sonst
// einen Vergleich. Summen seit dem letzten Leeren (messung() = Messung{} mit an = true).
#ifndef BHED_MODELL_MALEN_MESS_H
#define BHED_MODELL_MALEN_MESS_H

#include <chrono>

namespace bhed::modell::texmalen {

struct Messung {
    bool an = false;
    // --- je Tupfer (malen.cpp) ---
    long long tupfer = 0;             // tupfer3d + tupfer2d
    long long tupferLeer = 0;         // davon ohne ein Texel (in der Luft)
    long long dreieckeGeprueft = 0;   // Dreiecke, deren Kugel gegen den Tupfer geprueft wurde
    long long dreieckeNah = 0;        // davon in Reichweite (ihre Texel werden durchlaufen)
    long long texelGeprueft = 0;      // Texel dieser Dreiecke
    long long treffer = 0;            // Texel mit Abfall > 0
    long long rechteckTexel = 0;      // Flaeche des geschriebenen Rechtecks (sichern, Ausbluten, Zusammensetzen, Hochladen)
    long long kachelnGesichert = 0;   // Kacheln, die ein Tupfer fuer das Rueckgaengig kopiert hat
    double suchenMs = 0.0;            // Dreiecke in Reichweite suchen und ihre Texel sammeln
    double sichernMs = 0.0;
    double werkzeugMs = 0.0;          // das Werkzeug je Treffer
    double blutenMs = 0.0;
    double zusammenMs = 0.0;
    double strichEndeMs = 0.0;        // schrittEnde: Kacheln nachher, Verlauf kuerzen
    // --- je Mausereignis und Bild (app_modelltab_malen.cpp) ---
    long long ereignisse = 0;         // modellMalenAnsicht mit mindestens einem Tupfer
    double wegRuhe = 0.0;             // Weg der Tupfer in der Ruhelage (Summe der Abstaende zwischen zwei Tupfern eines Strichs)
    double wegRuheMax = 0.0;          // der groesste Sprung
    long long ereignisseWeit = 0;     // Spruenge ueber mehr als zehn Pinselradien zwischen zwei Tupfern (ohne Tupfer dazwischen)
    int letzteFlaeche = -1;           // die Flaeche des vorigen Tupfers
    int sprungFlaechen[2] = {-1, -1}; // der groesste Sprung: von Flaeche, nach Flaeche
    double sprungVon[3]{};            // und von wo nach wo in der Ruhelage
    double sprungNach[3]{};
    long long strahlen = 0;           // Strahlen gegen das Netz der Pose (MBMAL Punkt 5: einer je Tupfer am Schirm)
    long long tupferDaneben = 0;      // Tupferstellen am Schirm ohne Flaeche des Bilds (Hintergrund, anderes Bild): nichts gemalt
    double trefferMs = 0.0;           // Strahl gegen das Netz der Pose, Leinwand, Ruhelage, zugewandte Dreiecke
    double ansichtMs = 0.0;           // modellMalenAnsicht ganz (mit den Tupfern und dem Hochladen)
    double hochladenMs = 0.0;         // anzeigeNachfuehren (Bildpunkte der Vorschau, Grafikkarte)
    long long hochTexel = 0;          // dabei hochgeladene Flaeche (Stufe 0, je Bild der Vorschau)
    double maskeMs = 0.0;             // maskeFlaechenNeu
    long long maskeGelesen = 0;       // wie oft (jedes Mal alle Texel der Maske)
    double vergleichMs = 0.0;         // bemalt gegen das Original
    double editorMs = 0.0;            // drawModellMalEditor ganz
    double bereichMs = 0.0;           // drawModellMalenBereich ganz
};

[[nodiscard]] Messung& messung() noexcept;

// addiert die Zeit ihres Blocks auf ein Feld der Messung (nur wenn sie an ist)
class Stoppuhr {
  public:
    explicit Stoppuhr(double& ziel) noexcept : ziel_(messung().an ? &ziel : nullptr) {
        if (ziel_ != nullptr) { ab_ = std::chrono::steady_clock::now(); }
    }
    ~Stoppuhr() { halt(); }
    // vor dem Ende des Blocks anhalten
    void halt() noexcept {
        if (ziel_ != nullptr) { *ziel_ += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - ab_).count(); }
        ziel_ = nullptr;
    }
    Stoppuhr(const Stoppuhr&) = delete;
    Stoppuhr& operator=(const Stoppuhr&) = delete;

  private:
    double* ziel_;
    std::chrono::steady_clock::time_point ab_{};
};

}  // namespace bhed::modell::texmalen
#endif
