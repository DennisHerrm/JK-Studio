// modell/ablage.h - Flaechen von einem Modell in ein anderes kopieren (AP11, Abgleich R5/X19; shank 04.11.: "Kopieren/Einfuegen,
// Modelle in Tabs")
//
// Die Ablage haelt Kopien der Teile mit allen Detailstufen, ihren Shader (bzw. das Bild, das die Haut der Quelle zeigt), den
// Modifier-Stapel und das Skelett der Quelle als NAMEN - die Ecken einer .glm nennen Knochen nur als Index in die GLA ihres Modells
// (mdxmVertex_t, mdx_format.h), ein anderes Modell hat eine andere GLA oder dieselben Knochen in anderer Reihenfolge. Einfuegen loest
// die Namen im Skelett des Ziels auf, wie Ghoul2 einen Knochen sucht (G2_Find_Bone: Q_stricmp, gross/klein egal).
//
// Was das Ziel nicht hat, geht nicht still verloren: ein fehlender Knochen gibt sein Gewicht an den naechsten Elternknochen, den das
// Ziel kennt (die Summe je Ecke bleibt), Detailstufen ueber der Zahl des Ziels fallen weg, fehlende werden mit der groebsten der
// Quelle gefuellt, ein vorhandener Name bekommt eine Nummer - der Bericht nennt jedes davon. Die Grenzen des Zielspiels (Ecken,
// Dreiecke, Knochen je Flaeche, Gewichte je Ecke) prueft pruefeMass fuer jede neue Flaeche in jeder Stufe.
#ifndef BHED_MODELL_ABLAGE_H
#define BHED_MODELL_ABLAGE_H

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "bhed/modell/dokument.h"
#include "bhed/modell/g2.h"

namespace bhed::modell {

struct FlaechenAblage {
    std::vector<Teil> teile;              // Kopien mit allen Stufen; Teil::eltern = Index in `teile` (-1: oberste Flaeche der Ablage)
    std::vector<std::string> bilder;      // je Teil das Bild, das die Quelle zeigt (ihre Haut; leer: der Shader der Datei)
    std::vector<std::string> knochen;     // das Skelett der Quelle (Index wie Ecke::knochen); leer: ohne Skelett (.md3, starr)
    std::vector<int> knochenEltern;       // je Knochen der Eltern (-1: Wurzel)
    std::string quelle;                   // Name des Quellmodells fuer Hinweis und Meldung ("kyle/model.glm")
    int stufen = 0;                       // Detailstufen der Quelle
    [[nodiscard]] bool leer() const noexcept { return teile.empty(); }
};

// Die Teile `wahl` (Indizes in in.teile; doppelte und ungueltige fallen weg) in die Ablage, in der Reihenfolge des Modells. Ein Kind
// bleibt unter seinem Eltern, wenn beide gewaehlt sind. Was nur zur Datei der Quelle gehoert (Rohfelder, Ecknummern der .md3,
// Kinderliste der Datei), faellt weg - der Schreiber baut es fuer das Ziel neu. bilder: je Teil der Quelle (Index wie in.teile).
[[nodiscard]] FlaechenAblage ablageAus(const Inhalt& in, const std::vector<int>& wahl, const std::vector<std::string>& bilder,
                                       const std::vector<std::string>& knochen, const std::vector<int>& knochenEltern,
                                       const std::string& quelle);

struct EinfuegeBericht {
    std::vector<int> neu;                                          // die neuen Teile im Ziel (Reihenfolge der Ablage)
    std::vector<std::pair<std::string, std::string>> umbenannt;    // Name in der Quelle -> Name im Ziel
    std::vector<std::pair<std::string, std::string>> ersatz;       // fehlender Knochen (mit Gewicht) -> Knochen des Ziels
    int stufenWeg = 0;            // so viele Detailstufen der Quelle hat das Ziel nicht
    int stufenAufgefuellt = 0;    // so viele Detailstufen des Ziels hat die Quelle nicht (gefuellt mit ihrer groebsten)
    bool quelleOhneSkelett = false;   // die Quelle ist starr: alle Ecken an Knochen 0 des Ziels
    bool zielOhneSkelett = false;     // das Ziel ist eine .md3: die Ecken starr an Knochen 0
    std::vector<Befund> befunde;      // Grenzen des Zielspiels je neuer Flaeche und Stufe
};

enum class EinfuegeFehler : std::uint8_t { Keiner, AblageLeer, ZielLeer, ZielOhneSkelett, ElternUngueltig };

// Die Ablage als Kinder von `eltern` (-1: oberste Ebene) ans Ende von ziel.teile. zielKnochen: das Skelett des Ziels (dessen GLA);
// leer bei einer .glm lehnt ab (ZielOhneSkelett - ohne Namen keine Zuordnung), eine .md3 nimmt die Flaechen starr. Zwei Knochen
// der Quelle, die auf denselben des Ziels fallen, werden ein Gewicht (hoechstens vier je Ecke wie iMAX_G2_BONEWEIGHTS_PER_VERT).
// Modifier bekommen neue Kennungen im Ziel. Namen hoechstens 63 Zeichen (MAX_QPATH mit der Null). Gibt bei einem Fehler nichts ein.
[[nodiscard]] EinfuegeFehler einfuegen(Inhalt& ziel, const FlaechenAblage& a, int eltern, const std::vector<std::string>& zielKnochen,
                                       const Grenzen& g, EinfuegeBericht& bericht);

}  // namespace bhed::modell
#endif
