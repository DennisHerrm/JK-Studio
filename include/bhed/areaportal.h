// areaportal.h - die Gebiete (Areas) einer Karte und ihre Portale wie die Engine (cm_test.cpp).
//
// q3map2 teilt die Karte an Brushes mit CONTENTS_AREAPORTAL in Gebiete. Am Anfang ist jedes Portal
// zu; eine Entity, deren Kasten zwei Gebiete beruehrt (SV_LinkEntity: areanum, areanum2), macht es
// auf und zu (SV_AdjustAreaPortalState): Tueren beim Oeffnen, func_wall und func_usable beim
// Abschalten, func_breakable beim Zerbrechen. Der Renderer zeichnet ein Blatt nur, wenn sein Gebiet
// mit dem des Auges verbunden ist (R_RecursiveWorldNode, areamask), der Server schickt nur Entities
// aus verbundenen Gebieten (SV_AddEntitiesToSnapshot).
#ifndef BHED_AREAPORTAL_H
#define BHED_AREAPORTAL_H

#include "bhed/bspgeo.h"
#include "bhed/bsp.h"

#include <cstdint>
#include <string>
#include <vector>

namespace bhed {

inline constexpr std::uint32_t kInhaltUndurchsichtig = 0x00008000U;   // CONTENTS_OPAQUE

class AreaPortale {
public:
    void anlegen(int gebiete);
    // CM_AdjustAreaPortalState: Zaehler je Paar, nie unter 0.
    void setze(int a, int b, bool offen);
    // CM_AreasConnected
    [[nodiscard]] bool verbunden(int a, int b) const;
    [[nodiscard]] int gebiete() const { return zahl_; }

private:
    int zahl_ = 0;
    std::vector<int> portale_;
    std::vector<int> flut_;   // CM_FloodAreaConnections: floodnum
};

// SV_LinkEntity: die Gebiete des Kastens, Blaetter in der Reihenfolge von CM_BoxLeafnums_r.
struct AreaPaar {
    int a = -1;
    int b = -1;
};
[[nodiscard]] AreaPaar kastenGebiete(const BspGeometry& g, const float mins[3], const float maxs[3]);
[[nodiscard]] int punktGebiet(const BspGeometry& g, const float p[3]);

// R_MarkLeaves (tr_world.cpp): Je Flaeche 1, wenn kein Blatt mit ihr zu zeichnen ist - sein Cluster liegt nicht im
// PVS des Auges (R_ClusterPVS) oder sein Gebiet ist nicht mit dem des Auges verbunden (areamask). Das gilt auch fuer
// den Himmel. Das Auge im Festen (Gebiet -1): kein Gebiet verdeckt (CM_WriteAreaBits); Cluster -1 oder eine Karte
// ohne Sichttabelle: kein Cluster verdeckt (tr.viewCluster == -1, novis).
[[nodiscard]] std::vector<char> flaechenAusserSicht(const BspGeometry& g, const AreaPortale& p, int auge, int cluster);

// Die Entities, die ein Portal schalten koennen. Art: wie sie es tun.
struct PortalEntity {
    enum class Art { Mover, Abschaltbar, Zerbrechlich };
    int modell = 0;   // die Zahl aus "*N"
    Art art = Art::Mover;
    AreaPaar gebiete;
};
// `spawnt`: gibt es diese Entity im Spiel (Schwierigkeit, Spawntabelle)?
// mitUndurchsichtig: SV_AdjustAreaPortalState prueft CONTENTS_OPAQUE (nicht in Jedi Outcast).
[[nodiscard]] std::vector<PortalEntity> portalEntities(const MapData& karte, const BspGeometry& g, bool mitUndurchsichtig,
                                                       bool (*spawnt)(const MapEntity&));

}  // namespace bhed
#endif
