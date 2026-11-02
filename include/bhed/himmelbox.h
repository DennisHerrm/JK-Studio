// himmelbox.h - Welche Teile der Himmelsbox zeichnet das Spiel?
//
// Die Himmelsflaechen der Karte werden im Spiel nie selbst gezeichnet. RB_StageIteratorSky wirft ihre
// Dreiecke mit RB_ClipSkyPolygons auf die sechs Seiten der Box, merkt sich je Seite den Kasten der Treffer
// und zeichnet dann nur dieses Rechteck, auf ein Achtel der Seite aufgerundet, ganz hinten im Tiefenpuffer
// (qglDepthRange 1 1). Daraus folgt alles, was man im Bild sieht:
//   - Durch ein Loch in der Karte sieht man den Himmel, wenn es im Rechteck liegt, sonst den Bildpuffer.
//   - Himmelsflaechen hinter einem geschlossenen Portal zaehlen nicht (R_MarkLeaves).
//   - Die untere Seite gibt es nicht (DrawSkyBox: "still don't want to draw the bottom").
// Hier stehen R_MarkLeaves, R_RecursiveWorldNode, R_CullSurface, RB_CheckOverflow, ClipSkyPolygon,
// AddSkyPolygon und die Rundung aus DrawSkyBox (tr_world.cpp, tr_surface.cpp, tr_sky.cpp).
#pragma once

#include "bhed/areaportal.h"
#include "bhed/bspgeo.h"

#include <cstdint>
#include <vector>

namespace bhed {

// Was die Engine je Sicht braucht.
struct HimmelSicht {
    float auge[3]{};     // tr.ori.viewOrigin (im Spiegel der gespiegelte Ort)
    float pvsOrt[3]{};   // tr.viewParms.pvsOrigin
    float vorn[3]{};     // viewParms.ori.axis[0]
    float links[3]{};    // axis[1]
    float hoch[3]{};     // axis[2]
    float fovX = 90.0F;
    float fovY = 90.0F;
    float distanceCull = 12000.0F;   // tr.distanceCull, worldspawn "distanceCull"
    const AreaPortale* portale = nullptr;   // nullptr: die Karte hat keine Gebiete
    int gebiet = -1;     // CM_SnapPVS(vieworg): -1 sieht alle
};

// Ein gezeichnetes Rechteck auf einer Seite, s und t in -1..1 wie in MakeSkyVec.
struct HimmelRechteck {
    int shader = 0;   // Nummer in BspGeometry::shaders
    int seite = 0;    // 0 +x, 1 -x, 2 +y, 3 -y, 4 +z
    float s0 = 0.0F;
    float t0 = 0.0F;
    float s1 = 0.0F;
    float t1 = 0.0F;
};

// R_CullSurface: Wie der Shader der Himmelsflaeche keult. Kein Himmel: kKeinHimmel.
enum HimmelKeule : std::uint8_t { kHimmelVorn, kHimmelHinten, kHimmelBeide, kKeinHimmel };

class HimmelBox {
public:
    // keulen: je Shader der Karte. Die Dreiecke kommen aus dem Netz der Karte (Patches sind dort zerlegt).
    void anlegen(const BspGeometry& g, const BspMesh& netz, const std::vector<HimmelKeule>& keulen);
    [[nodiscard]] bool leer() const noexcept { return flaechen_.empty(); }
    // Je Bild. Rechtecke in der Reihenfolge der Zeichenaufrufe der Engine.
    void rechne(const HimmelSicht& sicht, std::vector<HimmelRechteck>& aus);
    // Die Flaechen, die in der letzten Rechnung dazu kamen (Pruefung).
    [[nodiscard]] std::size_t flaechenImBild() const noexcept { return imBild_.size(); }

private:
    struct Flaeche {
        BspSurface::Type art = BspSurface::Type::Bad;
        int shader = 0;
        int nebel = -1;
        int ecken = 0;
        int indizes = 0;
        HimmelKeule keule = kHimmelVorn;
        float normale[3]{};
        float abstand = 0.0F;
        float mins[3]{};
        float maxs[3]{};
        std::uint32_t ersteDreieck = 0;
        std::uint32_t dreiecke = 0;
    };
    struct Ebene {
        float normale[3]{};
        float abstand = 0.0F;
    };
    void geheDurch(int knoten, int ebenenBits);
    void blattHinzu(int blatt);
    [[nodiscard]] bool flaecheKeulen(const Flaeche& f) const;

    const BspGeometry* g_ = nullptr;
    std::vector<Flaeche> flaechen_;
    std::vector<float> dreiecke_;                // je Dreieck neun Zahlen
    std::vector<int> nachBlatt_;                 // Himmelsflaechen je Blatt: Anfang in blattFlaechen_
    std::vector<int> blattFlaechen_;
    std::vector<int> himmelBlaetter_;
    std::vector<int> elternKnoten_;              // je Knoten
    std::vector<int> elternBlatt_;               // je Blatt
    std::vector<char> knotenMitHimmel_;
    std::vector<char> blattMitHimmel_;
    std::vector<std::uint32_t> knotenMarke_;
    std::vector<std::uint32_t> blattMarke_;
    std::vector<std::uint32_t> flaecheMarke_;
    std::uint32_t marke_ = 0;
    bool alleSichtbar_ = false;
    Ebene frustum_[5];
    float auge_[3]{};
    std::vector<int> imBild_;                    // in der Reihenfolge von R_AddWorldSurface
};

// MakeSkyVec: Punkt auf der Box, relativ zum Auge.
void himmelEcke(int seite, float s, float t, float boxGroesse, float aus[3]);

// Faellt diese Blickrichtung in eines der Rechtecke? (Gegenstueck zur Projektion in AddSkyPolygon)
[[nodiscard]] bool himmelDeckt(const std::vector<HimmelRechteck>& rechtecke, const float richtung[3]);

// Die Rundung aus DrawSkyBox, Eingabe wie nach RB_ClipSkyPolygons. false: Dort wird nichts gezeichnet.
[[nodiscard]] bool himmelRunden(float smin, float tmin, float smax, float tmax, float& s0, float& t0, float& s1, float& t1);

}  // namespace bhed
