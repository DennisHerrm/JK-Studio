// modell/dokument.h - das Modelldokument des Model-Tabs (Modell-Block 1a-1)
//
// shank, 03.11.2026: "Ansonsten ist alles von scratch fuer JKA." Das Dokument
// ist das eigene Netz des Model-Tabs, aus dem gezeichnet und spaeter
// modelliert, abgewickelt, gewichtet und gemalt wird:
//
//   Inhalt      ein Modell: Name, Pfad der .gla, Knochenzahl, LOD-Zahl, Teile
//   Teil        eine GLM-Flaeche mit ihrer Hierarchie (Eltern, Kinder),
//               Flaggen, Shader und je LOD-Stufe einem Netz; Tags sind Teile
//               mit der Flagge G2SURFACEFLAG_ISBOLT (mdx_format.h)
//   Netz        die Ecken der Flaeche, wie die .glm sie fuehrt (Ort, Normale,
//               UV, vier Knochen mit Gewichten), und die Dreiecke als
//               HALBKANTEN; Ecken mit gleichem Ort sind ein PUNKT (die .glm
//               teilt Ecken an UV-Naehten, die Nachbarschaft gilt am Ort)
//   Dokument    Inhalt mit Rueckgaengig/Wiederholen
//
// Geladen wird ueber die vorhandenen Leser (readGlm je LOD-Stufe, readMd3).
// zuGlm gibt fuer jede Stufe genau das GlmModel zurueck, das readGlm fuer
// dieselbe Stufe liefert - der Rundlauf Laden -> Dokument -> Zeichnen ergibt
// dasselbe Bild wie der Weg ohne Dokument.
#ifndef BHED_MODELL_DOKUMENT_H
#define BHED_MODELL_DOKUMENT_H

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "bhed/glm.h"
#include "bhed/md3.h"

namespace bhed::modell {

// Eine Ecke der .glm (mdxmVertex_t + mdxmVertexTexCoord_t, mdx_format.h).
// Hoechstens vier Knochen je Ecke (iMAX_G2_BONEWEIGHTS_PER_VERT, mdx_format.h:85
// OpenJK) - mehr kann die Datei nicht tragen. Die Knochen sind aufgeloest
// (Index ins Skelett), wie in GlmVertex.
struct Ecke {
    float ort[3]{};
    float normale[3]{};
    float uv[2]{};
    std::uint8_t knochen[4]{};
    float gewicht[4]{};
};

// Eine Halbkante: vom Punkt ihrer Ecke zum Punkt der naechsten Kante im
// selben Dreieck. Dreieck t besteht aus den Kanten 3t, 3t+1, 3t+2.
// Modell-Block 1a-2: ein Knochen mit Gewicht in einer Malliste (siehe gewichte.h).
struct KnochenGewicht {
    int knochen = 0;
    float gewicht = 0.0F;
};
using GewichtListe = std::vector<KnochenGewicht>;

struct Halbkante {
    int ecke = -1;       // Index in Netz::ecken (die Ecke am Anfang)
    int punkt = -1;      // Netz::punktVon[ecke]
    int naechste = -1;   // naechste Kante im Dreieck
    int gegen = -1;      // Gegenkante im Nachbardreieck, -1 = Rand
};

struct Netz {
    std::vector<Ecke> ecken;
    std::vector<int> punktVon;     // je Ecke ihr Punkt (Ecken mit gleichem Ort)
    int punktZahl = 0;
    std::vector<Halbkante> kanten;
    // Kanten, an denen mehr als zwei Dreiecke oder zwei gleich gerichtete
    // haengen - sie bleiben ohne Gegenkante und zaehlen hier.
    int nichtMannigfaltig = 0;
    // Modell-Block 1a-2: waehrend des Malens je Ecke eine Liste mit beliebig
    // vielen Knochen (leer = es wird nicht gemalt, es gelten die Ecken).
    // Gezeichnet wird, was das Spiel daraus machte (gewichte.h, fuersSpiel).
    std::vector<GewichtListe> mal;

    [[nodiscard]] int dreiecke() const noexcept { return static_cast<int>(kanten.size() / 3U); }
    // Ecken und Dreiecke (drei Eckindizes je Dreieck) uebernehmen und die
    // Halbkanten aufbauen. Ungueltige Indizes werden zu 0 (wie readGlm).
    void aufbauen(std::vector<Ecke> e, const std::vector<std::uint32_t>& indizes);
    // Die Dreiecke als Eckindizes, in der Reihenfolge der Datei.
    [[nodiscard]] std::vector<std::uint32_t> indizes() const;
    [[nodiscard]] int randKanten() const noexcept;
    // Ecken an einer offenen Kante (ModView "crack highlight":
    // Model_EnsureGenerated_VertEdgeInfo, r_surface.cpp).
    [[nodiscard]] std::vector<char> randEcken() const;
    // Die Halbkanten-Invarianten; false mit Grund in `fehler`.
    [[nodiscard]] bool pruefen(std::string* fehler = nullptr) const;
};

struct Teil {
    std::string name;
    std::string shader;
    std::uint32_t flaggen = 0;
    int eltern = -1;
    std::vector<int> kinder;
    std::vector<Netz> stufen;   // je LOD-Stufe ein Netz

    // G2SURFACEFLAG_ISBOLT (mdx_format.h): ein Tag - nur ein Dreieck, nie gezeichnet.
    [[nodiscard]] bool istTag() const noexcept { return (flaggen & GlmSurface::kFlagBolt) != 0U; }
};

enum class Herkunft : std::uint8_t { Leer, Glm, Md3 };

struct Inhalt {
    Herkunft herkunft = Herkunft::Leer;
    std::string name;
    std::string glaPfad;
    int knochenZahl = 0;
    int stufenZahl = 0;
    bool jk2Umgewandelt = false;
    bool jkaUmgewandelt = false;
    std::vector<Teil> teile;

    [[nodiscard]] bool leer() const noexcept { return teile.empty(); }
    [[nodiscard]] int teilIndex(const std::string& name) const;
};

// Alle LOD-Stufen einer .glm. false: Stufe 0 laesst sich nicht lesen.
[[nodiscard]] bool ausGlm(const std::string& bytes, Inhalt& out, std::string* fehler = nullptr);
// Nur die Stufe eines schon gelesenen GlmModel (wenn die Bytes nicht mehr da
// sind): eine Stufe, die Kinder aus den Eltern.
void ausGlmModell(const GlmModel& m, Inhalt& out);
// Ein .md3 (ein Bild): jede Flaeche ein Teil ohne Eltern, starr (Knochen 0
// mit Gewicht 1), eine Stufe.
void ausMd3(const Md3Model& m, Inhalt& out, int bild = 0);
// Eine Stufe zum Zeichnen - fuer eine .glm gleich dem, was readGlm fuer
// dieselbe Stufe liefert (Stufe 0 samt groebster Stufe in grobVerts).
[[nodiscard]] GlmModel zuGlm(const Inhalt& in, int stufe);

// Das Dokument: ein Inhalt mit Rueckgaengig und Wiederholen.
//
// Jede Aenderung laeuft ueber aendern(); vorher wird der Inhalt gemerkt
// (Schnappschuss, hoechstens kTiefe Schritte). `stand` zaehlt jede Aenderung,
// auch Rueckgaengig - wer etwas aus dem Inhalt ableitet, baut neu, wenn er
// sich aendert.
class Dokument {
public:
    static constexpr std::size_t kTiefe = 32;

    [[nodiscard]] const Inhalt& inhalt() const noexcept { return jetzt_; }
    [[nodiscard]] int stand() const noexcept { return stand_; }
    void laden(Inhalt neu);
    void aendern(const std::string& was, const std::function<void(Inhalt&)>& f);
    // Den Inhalt ohne neuen Rueckgaengig-Schritt aendern - die Vorschau eines
    // Ziehens, dessen Schritt aendern() beim ersten Bewegen angelegt hat.
    void vorschau(const std::function<void(Inhalt&)>& f);
    bool rueckgaengig();
    // mb1a2: den letzten Schritt verwerfen (zurueck, ohne Wiederholen) - ein abgebrochenes G/R/S.
    bool abbrechen();
    bool wiederholen();
    [[nodiscard]] bool kannRueckgaengig() const noexcept { return !zurueck_.empty(); }
    [[nodiscard]] bool kannWiederholen() const noexcept { return !vor_.empty(); }
    [[nodiscard]] std::string rueckgaengigName() const { return zurueck_.empty() ? std::string() : zurueck_.back().first; }
    [[nodiscard]] std::string wiederholenName() const { return vor_.empty() ? std::string() : vor_.back().first; }

private:
    Inhalt jetzt_;
    std::vector<std::pair<std::string, Inhalt>> zurueck_;
    std::vector<std::pair<std::string, Inhalt>> vor_;
    int stand_ = 0;
};

}  // namespace bhed::modell
#endif
