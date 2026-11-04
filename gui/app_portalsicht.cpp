// app_portalsicht.cpp - rc609 P: was Vorschau und Karten-Editor fuer Spiegel und Portalkameras aus einer .bsp brauchen.
//
// Die Wahl der Flaeche und die Lage der zweiten Sicht rechnet bhed/portalsicht (R_MirrorViewBySurface,
// R_GetPortalOrientations, SurfIsOffscreen). Hier: die Flaechen mit `sort portal` (R_PlaneForSurface) mit ihren Blaettern,
// welche davon und welche misc_portal_surface vom Auge aus im Bild sind (R_MarkLeaves, Snapshot), die Hauptsicht als
// PortalAuge und die Sicht als App::NebenSicht. Benutzt von spiegelFuer (app_view3d.cpp) und dem Spielbild des
// Karten-Editors (app_karte_spielbild.cpp).
#include "app_internal.h"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace bhed::gui {

namespace {

// Liegt ein Blatt fuer R_MarkLeaves im Bild: Cluster im PVS des Auges, Gebiet verbunden (areamask)
bool blattImBild(const BspGeometry& geo, const AreaPortale& portale, const BspGeometry::Leaf& auge, const BspGeometry::Leaf& l) {
    return l.cluster >= 0 && (auge.cluster < 0 || geo.clusterVisible(auge.cluster, l.cluster)) &&
           (auge.area < 0 || l.area < 0 || portale.verbunden(auge.area, l.area));
}

}  // namespace

PortalKarte portalKarteAus(const BspGeometry& geo, const TextureSet& bilder, const std::vector<MapEntity>& ents) {
    PortalKarte k;
    std::vector<int> nummer(geo.surfaces.size(), -1);   // BSP-Flaeche -> Index in k.flaechen
    for (std::size_t si = 0; si < geo.surfaces.size(); ++si) {
        const BspSurface& s = geo.surfaces[si];
        // SF_FACE und SF_TRIANGLES; ein Patch (SF_GRID) nimmt die Ebene aus den ersten drei Ecken seines Gitters,
        // die bei einer geraden ersten Zeile auf einer Linie liegen - hier nicht nachgebaut.
        if ((s.type != BspSurface::Type::Planar && s.type != BspSurface::Type::TriangleSoup) || s.numVerts < 3 || s.numIndexes < 3 || s.shader < 0 ||
            static_cast<std::size_t>(s.shader) >= bilder.byShader.size() || !bilder.byShader[static_cast<std::size_t>(s.shader)].istPortal) {
            continue;
        }
        PortalFlaeche f;
        const BspVertex* v = &geo.verts[static_cast<std::size_t>(s.firstVert)];
        for (int q = 0; q < s.numVerts; ++q) {
            f.ecken.push_back({v[q].xyz[0], v[q].xyz[1], v[q].xyz[2]});
            f.normalen.push_back({v[q].normal[0], v[q].normal[1], v[q].normal[2]});
        }
        for (int q = 0; q < s.numIndexes; ++q) { f.indizes.push_back(static_cast<int>(geo.indexes[static_cast<std::size_t>(s.firstIndex + q)])); }
        if (s.type == BspSurface::Type::Planar) {
            // ParseFace (tr_bsp.cpp): plane.normal = lightmapVecs[2], dist = points[0] . normal
            for (int c = 0; c < 3; ++c) { f.ebene[c] = s.ebenenNormale[c]; }
            f.ebene[3] = v[0].xyz[0] * f.ebene[0] + v[0].xyz[1] * f.ebene[1] + v[0].xyz[2] * f.ebene[2];
        } else if (!ebeneAusPunkten(v[f.indizes[0]].xyz, v[f.indizes[1]].xyz, v[f.indizes[2]].xyz, f.ebene)) {
            continue;   // R_PlaneForSurface SF_TRIANGLES: PlaneFromPoints
        }
        f.portalWeite = bilder.byShader[static_cast<std::size_t>(s.shader)].portalWeite;
        f.sortierung = s.shader;
        nummer[si] = static_cast<int>(k.flaechen.size());
        k.flaechen.push_back(std::move(f));
    }
    k.blaetter.resize(k.flaechen.size());
    for (std::size_t bi = 0; bi < geo.leafs.size() && !k.flaechen.empty(); ++bi) {
        const BspGeometry::Leaf& lf = geo.leafs[bi];
        const long long bis = std::min<long long>(static_cast<long long>(lf.firstSurface) + lf.numSurfaces, static_cast<long long>(geo.leafSurfaces.size()));
        for (long long i = std::max(lf.firstSurface, 0); i < bis; ++i) {
            const int s = geo.leafSurfaces[static_cast<std::size_t>(i)];
            if (s >= 0 && static_cast<std::size_t>(s) < nummer.size() && nummer[static_cast<std::size_t>(s)] >= 0) {
                k.blaetter[static_cast<std::size_t>(nummer[static_cast<std::size_t>(s)])].push_back(static_cast<int>(bi));
            }
        }
    }
    if (!k.flaechen.empty()) { k.schirme = portalSchirme(ents, [](const MapEntity& e) { return gespawntImSpiel(e); }); }
    return k;
}

bool portalSichtFuer(const PortalKarte& k, const BspGeometry& geo, const AreaPortale& portale, const Camera& cam, int breite, int hoehe, double zeitMs,
                     PortalSicht& aus) {
    if (k.flaechen.empty() || k.schirme.empty()) { return false; }
    const int blatt = geo.leafAt(cam.pos);
    const bool imBaum = blatt >= 0 && static_cast<std::size_t>(blatt) < geo.leafs.size();
    BspGeometry::Leaf auge{};
    auge.cluster = -1;
    auge.area = -1;
    if (imBaum) { auge = geo.leafs[static_cast<std::size_t>(blatt)]; }
    // R_MarkLeaves/R_RecursiveWorldNode: nur Flaechen aus Blaettern im Bild kommen in die Liste der drawSurfs (ohne Blatt: wie
    // surfaceVisible lieber dabei).
    const auto flaecheImBild = [&](std::size_t i) {
        if (!imBaum || k.blaetter[i].empty()) { return true; }
        return std::any_of(k.blaetter[i].begin(), k.blaetter[i].end(),
                           [&](int b) { return blattImBild(geo, portale, auge, geo.leafs[static_cast<std::size_t>(b)]); });
    };
    // SV_AddEntitiesVisibleFromPoint: ein misc_portal_surface (mins = maxs = 0, ein Blatt) kommt in den Snapshot, wenn sein
    // Gebiet mit dem des Auges verbunden ist und sein Cluster im PVS liegt; ohne Blatt (numClusters 0) nie.
    const auto schirmImBild = [&](std::size_t i) {
        if (!imBaum) { return true; }
        const int b = geo.leafAt(k.schirme[i].ort);
        return b >= 0 && static_cast<std::size_t>(b) < geo.leafs.size() && blattImBild(geo, portale, auge, geo.leafs[static_cast<std::size_t>(b)]);
    };
    return portalSichtWaehlen(portalAugeAus(cam, breite, hoehe, kartenFernFuer(geo)), k.flaechen, k.schirme, zeitMs, aus, schirmImBild, flaecheImBild);
}

PortalAuge portalAugeAus(const Camera& cam, int breite, int hoehe, float fern) {
    PortalAuge a;
    for (int c = 0; c < 3; ++c) { a.ort[c] = cam.pos[c]; }
    cam.forward(a.vorn);
    cam.right(a.rechts);
    cam.up(a.oben);
    a.tanY = std::tan(cam.fovY * 3.14159265F / 360.0F);
    a.tanX = a.tanY * static_cast<float>(breite) / static_cast<float>(std::max(hoehe, 1));
    a.nah = 4.0F;   // r_znear
    a.fern = fern;
    return a;
}

void nebenSichtAus(const PortalSicht& p, App::NebenSicht& ns) {
    ns = App::NebenSicht{};
    std::copy(std::begin(p.welt), std::end(p.welt), ns.welt);
    std::copy(std::begin(p.achsen), std::end(p.achsen), ns.achsen);
    std::copy(std::begin(p.schnitt), std::end(p.schnitt), ns.schnitt);
    std::copy(std::begin(p.auge), std::end(p.auge), ns.auge);
    std::copy(std::begin(p.vorn), std::end(p.vorn), ns.vorn);
    std::copy(std::begin(p.pvsOrt), std::end(p.pvsOrt), ns.pvsOrt);
    ns.umkehren = p.umkehren;
    ns.portal = p.portal;
}

}  // namespace bhed::gui
