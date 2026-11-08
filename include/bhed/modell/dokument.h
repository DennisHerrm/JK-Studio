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
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "bhed/glm.h"
#include "bhed/md3.h"
#include "bhed/modell/animcfg.h"
#include "bhed/modell/rigdaten.h"

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
    // Stufe 2 (GLM-Schreiber): die Gewichtsfelder, wie sie in der Datei standen (mdxmVertex_t
    // uiNmWeightsAndBoneIndexes und BoneWeightings[4], mdx_format.h). Die Datei traegt dort mehr als das Spiel liest
    // (das letzte Gewicht, unbenutzte Felder); der Schreiber nimmt sie, solange knochen/gewicht dazu passen.
    std::uint32_t dateiPack = 0;
    std::uint8_t dateiGewicht[4]{};
    bool ausDatei = false;
    // Modell-Block 1b: die Nummer der Ecke in der Datei (Kopien an UV-Naehten behalten die ihrer Vorlage; der
    // MD3-Schreiber nimmt von dort Ort und Normale jedes Bilds). -1: keine Ecke einer Datei.
    std::int32_t dateiIndex = -1;
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
    // Stufe 2: die Knochenverweise der Flaeche (mdxmSurface_t ofsBoneReferences) in der Reihenfolge der Datei - mit
    // den Nummern der Datei und so, wie readGlm sie aufloest (nach dem Umlegen JK2 <-> JKA). Leer: nicht aus einer .glm.
    std::vector<std::int32_t> dateiRefs;
    std::vector<std::int32_t> dateiRefsAufgeloest;
    bool ausDatei = false;   // die Flaeche dieser Stufe stand so in einer .glm (Rohfelder gelesen)
    // Modell-Block 1b: der UV-Arbeitsstand (nicht in der Datei - JKA kennt je Ecke genau einen UV-Satz): Naht-Marken je
    // Halbkante, ob eine Marke aus der Datei kam, Pins je Halbkante, "frei" je Dreieck (seit dem Laden bearbeitet). Leer =
    // die Flaeche war noch nicht im UV-Bereich (bhed/modell/wof/uv.h, netzAus).
    std::vector<std::uint8_t> uvNaht;
    std::vector<std::uint8_t> uvDateiNaht;
    std::vector<std::uint8_t> uvPin;
    std::vector<std::uint8_t> uvFrei;
    // Modell-Block 1d: je Dreieck das Vieleck, zu dem es gehoert (Modellieren arbeitet auf Vielecken wie 3ds Max Editable
    // Poly; JKA kennt nur Dreiecke). Nicht in der Datei; leer oder falsch lang = jedes Dreieck ein Vieleck.
    std::vector<int> polyVon;

    [[nodiscard]] int dreiecke() const noexcept { return static_cast<int>(kanten.size() / 3U); }
    // Ecken und Dreiecke (drei Eckindizes je Dreieck) uebernehmen und die
    // Halbkanten aufbauen. Ungueltige Indizes werden zu 0 (wie readGlm).
    void aufbauen(std::vector<Ecke> e, const std::vector<std::uint32_t>& indizes);
    // Modell-Block 1d: mit festen Punkten je Ecke (das Arbeitsnetz des Modellierens kennt Punkte, die am selben Ort
    // getrennt bleiben: Detach als Element, Extrude mit Hoehe 0). `punkte` gleich lang wie `e`, Nummern ab 0.
    void aufbauen(std::vector<Ecke> e, const std::vector<std::uint32_t>& indizes, const std::vector<int>& punkte);
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
    // Stufe 2: Name und Shader als die 64 Bytes der Datei (MAX_QPATH, hinter der Null steht in vielen Raven-Dateien
    // noch Rest) - der Schreiber nimmt sie, solange der Name gleich ist. Leer: nicht aus einer .glm.
    std::string dateiName;
    std::string dateiShader;
    // Die Kinderliste der Datei (childIndexes): Movie Duels nahdar fuehrt in hips ein Kind zweimal.
    std::vector<int> dateiKinder;
    std::uint32_t flaggen = 0;
    int eltern = -1;
    // pfix M1: die Flaeche der gelesenen .md3, aus der der Teil stammt (readMd3 ueberspringt leere), -1 neu
    int dateiFlaeche = -1;
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
    // Modell-Block 1b: die gelesene .md3 (der Schreiber nimmt Bilder, Tags und unveraenderte Flaechen von dort; geteilt,
    // damit die Rueckgaengig-Schritte sie nicht kopieren). Leer: keine .md3.
    std::shared_ptr<const std::string> md3Datei;
    // Stufe 2: Kopf der Datei roh (Name und Pfad der .gla je 64 Bytes, numBones vor dem Umlegen).
    std::string dateiName;
    std::string dateiGla;
    int dateiKnochen = 0;
    // Stufe 2: die animation.cfg der GLA (Archivpfad und Zeilen). Die Marker der Zeitleiste sind diese Zeilen; sie
    // aendern sich ueber das Dokument (ein Rueckgaengig-Schritt je Aenderung).
    std::string cfgPfad;
    AnimCfg cfg;
    // Stufe 3: Schluessel des Control Rigs und Zuordnungen von Hand (nicht in der .glm; gebacken wird in eine GLA).
    RigDaten rig;
    // REST N10: die Belegung der Plaetze der Ziel-GLA (Platzname -> Quelle, Karten-GLA BOTH_CIN_n, _humanoid, eigene GLA),
    // damit Rueckgaengig sie mitnimmt (bis rc610 im Model-Tab neben dem Dokument). Beim Laden eines anderen Modells wird
    // sie uebernommen (die Quellen kommen aus vielen Modellen); die GLA-Kopien selbst liegen in ModellTab::kgQuellen.
    std::map<std::string, PlatzQuelle> kinoBelegung;
    // Stufe 3: zaehlt jede Aenderung, die das Netz betrifft (aendern/vorschau mit netz = true, laden). Rig-Schluessel
    // aendern ihn nicht - die Anzeige baut nur neu, wenn er wechselt. Rueckgaengig bringt den Stand mit zurueck.
    int netzStand = 0;

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
    // PTFIX3 P23: zaehlt nur laden() - ein neu geladenes Modell, auch dieselbe Datei noch einmal (wer am Modell haengt, z. B. die
    // Leinwaende des Paintings, verwirft dann seinen Stand; der Pfad allein bleibt beim Neuladen gleich)
    [[nodiscard]] int ladeStand() const noexcept { return ladeStand_; }
    void laden(Inhalt neu);
    // netz = false: die Aenderung betrifft nur das Rig (netzStand bleibt, die Anzeige baut nicht neu).
    // pfix H1: die Kennung des neuen Schritts - wer einen Schritt offen haelt (Werkzeugkaestchen, Regler, Zug), bricht mit ihr ab.
    std::uint64_t aendern(const std::string& was, const std::function<void(Inhalt&)>& f, bool netz = true);
    // Den Inhalt ohne neuen Rueckgaengig-Schritt aendern - die Vorschau eines
    // Ziehens, dessen Schritt aendern() beim ersten Bewegen angelegt hat.
    void vorschau(const std::function<void(Inhalt&)>& f, bool netz = true);
    bool rueckgaengig();
    // mb1a2: den letzten Schritt verwerfen (zurueck, ohne Wiederholen) - ein abgebrochenes G/R/S.
    bool abbrechen();
    // pfix H1: nur den Schritt `schritt` verwerfen - liegt er nicht mehr oben (Rueckgaengig, ein anderer Befehl), nichts.
    bool abbrechen(std::uint64_t schritt);
    bool wiederholen();
    [[nodiscard]] bool kannRueckgaengig() const noexcept { return !zurueck_.empty(); }
    [[nodiscard]] bool kannWiederholen() const noexcept { return !vor_.empty(); }
    [[nodiscard]] std::string rueckgaengigName() const { return zurueck_.empty() ? std::string() : zurueck_.back().first; }
    [[nodiscard]] std::string wiederholenName() const { return vor_.empty() ? std::string() : vor_.back().first; }

private:
    Inhalt jetzt_;
    struct Schritt {
        std::string first;   // Name
        Inhalt second;       // der Inhalt davor
        std::uint64_t kennung = 0;
    };
    std::vector<Schritt> zurueck_;
    std::vector<Schritt> vor_;
    std::uint64_t kennungZaehler_ = 0;
    int stand_ = 0;
    int netzZaehler_ = 0;
    int ladeStand_ = 0;
};

}  // namespace bhed::modell
#endif
